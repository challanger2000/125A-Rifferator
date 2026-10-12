#pragma once
#include "RiffEngine.hpp"
#include "ChordCompanion.hpp"
#include "IndustrialGuitarGroove.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

// Zero-allocation block scheduler: render() is used only off the audio callback.
// The VST3 adapter supplies the authoritative host PPQ and sample rate.
namespace a125::rifferator {
struct ScheduledMidiEvent {
  int bus{}, sampleOffset{}, pitch{}, velocity{}, noteId{-1};
  bool on{};
};
struct LiveControls {
  int pmMin{30}; int pmMax{50};
  int guitarGate{95}; int bassGate{95}; int rootSemitones{}; int absoluteRoot{-1}; int bassOffset{-12}; int preparedRoot{40};
  int bassVelocity{100}; bool bassEnabled{true}; bool drumsEnabled{true}; bool externalBassMode{false}; bool chordsEnabled{false}; bool chordsRootOnly{false}; int chordQuality{0}; bool chordsAuto{true};
  HarmonyMap harmony{};
  std::array<int,7> scale{0,2,3,5,7,8,10};
  std::array<std::array<bool,16>,3> drumSteps{};
  std::array<bool,16> guitarSteps{};
  std::array<int,16> guitarArticulation{};
  std::array<int,16> guitarDegree{};
  std::array<bool,16> guitarLocks{};
  std::array<bool,16> guitarDouble{};
  LiveControls() noexcept {
    for(int i=0;i<16;++i) {
      drumSteps[0][i]=i%4==0;
      drumSteps[1][i]=i==4||i==12;
      drumSteps[2][i]=true;
      guitarSteps[i]=true;
      guitarArticulation[i]=2;
      guitarDegree[i]=1;
    }
  }
};
class BlockScheduler {
public:
  static constexpr int kCapacity=1024;
  void reset() noexcept { active_={}; haveExpected_=false; recoveryPending_=false; }
  void setDrums(const std::vector<MidiNote>* notes) noexcept { drums_=notes; }
  void setChords(const std::vector<MidiNote>* notes) noexcept { chords_=notes; }
  template<class Sink>
  void process(const Pair& pattern, double startPpq, double endPpq,
               int blockSize, int64_t patternTicks, Sink&& sink, bool playing,
               LiveControls live={}) noexcept {
    if(blockSize<=0) return;
    // A rejected host event may have left notes active. Finish releasing them
    // before generating any more NoteOns, even on a continuous timeline.
    if(recoveryPending_) {
      if(flush(sink,0)) { recoveryPending_=false; haveExpected_=false; }
      return;
    }
    if(!playing || !std::isfinite(startPpq) || !std::isfinite(endPpq) ||
       endPpq<=startPpq) {
      if(!flush(sink,0)) { recoveryPending_=true; return; }
      haveExpected_=false;
      return;
    }
    // Bound all float-to-int64 cycle conversions before scheduling. Host values
    // can be finite but far outside the representable cycle-index range.
    // Invalid periods/positions fail closed without starting new notes.
    const int64_t period=patternTicks;
    if(period<=0 || std::abs(startPpq)>1.0e12 || std::abs(endPpq)>1.0e12) {
      if(flush(sink,0))haveExpected_=false;
      else recoveryPending_=true;
      return;
    }
    if(haveExpected_ && std::abs(startPpq-expectedPpq_)>0.002) {
      if(!flush(sink,0)) { recoveryPending_=true; return; }
    }
    expectedPpq_=endPpq; haveExpected_=true;
    std::array<ScheduledMidiEvent,kCapacity> events{};
    int count=0;
    bool eventOverflow=false;
    bool invalidWindow=false;
    auto collect=[&](const std::vector<MidiNote>& notes,int bus) {
      if(bus==1 && (!live.bassEnabled || live.externalBassMode))return;
      if(bus==2 && !live.drumsEnabled)return;
      if(bus==3 && !live.chordsEnabled)return;
      for(const auto& note:notes) {
        if(bus==0 || bus==1) {
          const int step=static_cast<int>((note.startTick/240)%16);
          if(step<0 || !live.guitarSteps[static_cast<std::size_t>(step)] ||
             live.guitarArticulation[static_cast<std::size_t>(step)]==0)continue;
          if(bus==1 && (live.guitarArticulation[static_cast<std::size_t>(step)]==1 ||
                        live.guitarArticulation[static_cast<std::size_t>(step)]==5))continue;
        }
        if(bus==3 && (live.chordsRootOnly || live.chordQuality==2) && note.note==42)continue;
        if(bus==3 && live.chordsRootOnly && note.note!=40)continue;
        if(bus==2) {
          const int row=note.note==36?0:note.note==38?1:note.note==42?2:-1;
          const int step=static_cast<int>((note.startTick/240)%16);
          if(row<0||step<0||!live.drumSteps[static_cast<std::size_t>(row)][static_cast<std::size_t>(step)])continue;
        }
        const double periodPpq=static_cast<double>(period)/960.0;
        const double phase=static_cast<double>(note.startTick)/960.0;
        const int64_t first=static_cast<int64_t>(std::floor((startPpq-phase)/periodPpq))-1;
        const int64_t last=static_cast<int64_t>(std::ceil((endPpq-phase)/periodPpq))+1;
        // protect realtime complexity on corrupt/huge host jumps
        if(last<first || last-first>64) { invalidWindow=true; return; }
        for(int64_t n=first;n<=last;++n) {
          const double cycle=periodPpq*static_cast<double>(n);
          auto push=[&](double t,bool on,int fifth=0,bool secondStroke=false) {
            if(t<startPpq || t>=endPpq)return;
            if(count>=kCapacity) {eventOverflow=true;return;}
            const auto offset=static_cast<int>(std::floor(
              (t-startPpq)/(endPpq-startPpq)*blockSize));
            int pitch=note.note+live.rootSemitones;
            int velocity=note.velocity;
            if(bus==3) {
              const int bar=static_cast<int>(note.startTick/3840)%16;
              const auto quality=static_cast<ChordQuality>(std::clamp(live.chordQuality,0,4));
              const int interval=chordInterval(note.note-40,quality);
              pitch=chordOutputPitch(40+interval,bar,live.harmony,40+live.rootSemitones);
            } else if(bus!=2) {
              const int step=static_cast<int>((note.startTick/240)%16);
              if(step>=0) {
                const int degree=live.guitarDegree[static_cast<std::size_t>(step)];
                // Use the active musical scale, shared with chord/bass routing.
                // Prepared riff notes already contain the scale degree.
                // Live degree control is absolute, not an additional transpose.
                const int busOffset=bus==1?live.bassOffset:0;
                const int preparedPitch=live.preparedRoot+busOffset;
                pitch=(live.absoluteRoot>=0?live.absoluteRoot:40+live.rootSemitones)+degreeOffset(degree,live.scale)+busOffset+(note.note-preparedPitch);
              }
            }
            if(bus==0) {
              velocity=guitarStepVelocity(static_cast<int>((note.startTick/240)%16),live.guitarArticulation[static_cast<std::size_t>((note.startTick/240)%16)]);
              if(live.guitarArticulation[static_cast<std::size_t>((note.startTick/240)%16)]==2) {
                const int lower=std::clamp(live.pmMin,1,127);
                const int upper=std::clamp(live.pmMax,lower,127);
                velocity=std::clamp(velocity,lower,upper);
                if(on && secondStroke)
                  velocity=std::max(lower,velocity-std::min(4,(upper-lower)/3));
              }
              pitch+=fifth;
            }
            if(bus==1)velocity=live.bassVelocity;
            if(bus==2)pitch=note.note;
            events[static_cast<std::size_t>(count++)]={
              bus,offset,std::clamp(pitch,0,127),on?velocity:0,-1,on};
          };
          const int gate=(bus==2 || bus==3)?100:std::clamp(bus==0?live.guitarGate:live.bassGate,0,100);
          const int64_t scaled=note.startTick+(note.boundaryTick-note.startTick)*gate/100;
          const int64_t endTick=std::max(note.startTick+1,scaled);
          if(gate<=0) continue;
          // The second attack retains the selected instrument articulation's
          // velocity zone; the Kontakt reference's fixed 75 would change PM
          // to a different articulation on velocity-switched guitar VSTis.
          // Original Thrash DI UP reference: one normal stroke at the step
          // start, followed by a second stroke halfway through the step.
          // Each gets its own NoteOff and must never spill into the next step.
          if(bus==0 && live.guitarDouble[static_cast<std::size_t>((note.startTick/240)%16)]) {
            // DOUBLE retriggers halfway through its 16th grid step,
            // not halfway through the duration of a sustained OPEN note.
            const int64_t stepEnd=std::min<int64_t>(note.boundaryTick,note.startTick+240);
            const int64_t half=std::max<int64_t>(1,(stepEnd-note.startTick)/2);
            const int64_t midpoint=note.startTick+half;
            const int64_t firstOff=std::max<int64_t>(note.startTick+1,
              std::min<int64_t>(midpoint,note.startTick+half*gate/100));
            const int64_t secondOff=std::max<int64_t>(midpoint+1,
              std::min<int64_t>(stepEnd,midpoint+(stepEnd-midpoint)*gate/100));
            // The guitar library owns PM and FIFTH voicings: trigger one
            // MIDI pitch per attack; never synthesize an extra fifth.
            push(cycle+static_cast<double>(firstOff)/960.0,false);
            push(cycle+phase,true);
            push(cycle+static_cast<double>(secondOff)/960.0,false);
            push(cycle+static_cast<double>(midpoint)/960.0,true,0,true);
            continue;
          }
          if(bus==3) {
            // Sustain a chord voice across bars when its next pitch is
            // unchanged. A new transport start still emits the required On
            // because the runtime active-note state is initially empty.
            const int bar=static_cast<int>(note.startTick/3840)%16;
            const auto quality=static_cast<ChordQuality>(std::clamp(live.chordQuality,0,4));
            const int interval=chordInterval(note.note-40,quality);
            const int current=chordOutputPitch(40+interval,bar,live.harmony,40+live.rootSemitones);
            const int next=chordOutputPitch(40+interval,(bar+1)%16,live.harmony,40+live.rootSemitones);
            if(bar==15 || current!=next)
              push(cycle+static_cast<double>(endTick)/960.0,false);
          } else {
            push(cycle+static_cast<double>(endTick)/960.0,false);
          }
          push(cycle+phase,true);
        }
      }
    };
    collect(pattern.guitar,0); collect(pattern.bass,1);
    // STOP is a command, not an empty step: it releases currently active
    // guitar notes at the exact step boundary. Empty steps emit no command.
    // Insert it into the same sorted event batch so it also works when the
    // preceding NoteOn and this STOP occur within one audio block.
    const double periodPpq=static_cast<double>(period)/960.0;
    for(int step=0;step<16;++step) {
      // Do not wrap STOP commands from steps outside a shorter pattern.
      if(static_cast<int64_t>(step)*240>=period)break;
      if(!live.guitarSteps[static_cast<std::size_t>(step)] ||
         live.guitarArticulation[static_cast<std::size_t>(step)]!=0)continue;
      const double phase=static_cast<double>(step*240)/960.0;
      const int64_t first=static_cast<int64_t>(std::floor((startPpq-phase)/periodPpq))-1;
      const int64_t last=static_cast<int64_t>(std::ceil((endPpq-phase)/periodPpq))+1;
      if(last<first || last-first>64){invalidWindow=true;break;}
      for(int64_t n=first;n<=last;++n) {
        const double t=periodPpq*static_cast<double>(n)+phase;
        if(t<startPpq || t>=endPpq)continue;
        if(count>=kCapacity){eventOverflow=true;break;}
        const int offset=std::clamp(static_cast<int>(std::floor(
          (t-startPpq)/(endPpq-startPpq)*blockSize)),0,blockSize-1);
        events[static_cast<std::size_t>(count++)]={0,offset,-1,0,-1,false};
      }
    }

    if(drums_)collect(*drums_,2);
    if(chords_)collect(*chords_,3);
    if(eventOverflow || invalidWindow) {
      // Do not deliver a partial batch that may have omitted NoteOff events.
      if(!flush(sink,0))recoveryPending_=true;
      haveExpected_=false;
      return;
    }
    // Fail closed on capacity overflow rather than silently losing Note-Offs.
    // The processor should retain a pending recovery flag when a host rejects events.
    // Stable tie order: NoteOff before NoteOn at the same sample and pitch.
    for(int i=1;i<count;++i) {
      auto key=events[static_cast<std::size_t>(i)];int j=i-1;
      while(j>=0 && (events[static_cast<std::size_t>(j)].sampleOffset>key.sampleOffset ||
        (events[static_cast<std::size_t>(j)].sampleOffset==key.sampleOffset &&
         events[static_cast<std::size_t>(j)].on && !key.on))) {
        events[static_cast<std::size_t>(j+1)]=events[static_cast<std::size_t>(j)];--j;
      }
      events[static_cast<std::size_t>(j+1)]=key;
    }
    for(int i=0;i<count;++i) {
      const auto e=events[static_cast<std::size_t>(i)];
      if(e.bus==0 && e.pitch==-1) {
        if(!flushBus(0,sink,e.sampleOffset)){recoveryPending_=true;return;}
        continue;
      }
      if(!e.on && !active_[static_cast<std::size_t>(e.bus)][static_cast<std::size_t>(e.pitch)]) continue;
      if(e.on && e.bus==3 && active_[3][static_cast<std::size_t>(e.pitch)])continue;
      if(e.on && active_[static_cast<std::size_t>(e.bus)][static_cast<std::size_t>(e.pitch)]) {
        if(!flushBus(e.bus,sink,e.sampleOffset)) { recoveryPending_=true; return; }
      }
      if(sink(e)) active_[static_cast<std::size_t>(e.bus)][static_cast<std::size_t>(e.pitch)]=e.on;
      else {
        // The failed event may have been a NoteOff. Preserve pending recovery
        // until the sink accepts the outstanding releases.
        recoveryPending_=true;
        return;
      }
    }
  }
  template<class Sink> bool flushBus(int bus,Sink&& sink,int offset) noexcept {
    if(bus<0 || bus>=4)return false;
    bool complete=true;
    for(int pitch=0;pitch<128;++pitch) {
      auto& active=active_[static_cast<std::size_t>(bus)][static_cast<std::size_t>(pitch)];
      if(active) {
        if(sink(ScheduledMidiEvent{bus,offset,pitch,0,-1,false}))active=false;
        else complete=false;
      }
    }
    return complete;
  }
  template<class Sink> bool flush(Sink&& sink,int offset) noexcept {
    bool complete=true;
    for(int bus=0;bus<4;++bus) for(int pitch=0;pitch<128;++pitch) {
      auto& active=active_[static_cast<std::size_t>(bus)][static_cast<std::size_t>(pitch)];
      if(active) {
        if(sink(ScheduledMidiEvent{bus,offset,pitch,0,-1,false}))active=false;
        else complete=false;
      }
    }
    return complete;
  }
private:
  std::array<std::array<bool,128>,4> active_{};
  const std::vector<MidiNote>* drums_{};
  const std::vector<MidiNote>* chords_{};
  bool haveExpected_{};
  bool recoveryPending_{};
  double expectedPpq_{};
};
} // namespace a125::rifferator
