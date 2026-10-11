#pragma once
#include "RiffEngine.hpp"
#include "ChordCompanion.hpp"
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
  int guitarGate{95}; int bassGate{95}; int rootSemitones{};
  int bassVelocity{100}; bool bassEnabled{true}; bool drumsEnabled{true}; bool externalBassMode{false}; bool chordsEnabled{false}; bool chordsRootOnly{false}; int chordQuality{0}; bool chordsAuto{true};
  HarmonyMap harmony{};
  std::array<std::array<bool,16>,3> drumSteps{};
  std::array<bool,16> guitarSteps{};
  std::array<int,16> guitarArticulation{};
  std::array<int,16> guitarDegree{};
  std::array<bool,16> guitarLocks{};
  LiveControls() noexcept {
    for(int i=0;i<16;++i) {
      drumSteps[0][i]=i%4==0;
      drumSteps[1][i]=i==4||i==12;
      drumSteps[2][i]=true;
      guitarSteps[i]=true;
      guitarArticulation[i]=3;
      guitarDegree[i]=1;
    }
  }
};
class BlockScheduler {
public:
  static constexpr int kCapacity=1024;
  void reset() noexcept { active_={}; haveExpected_=false; }
  void setDrums(const std::vector<MidiNote>* notes) noexcept { drums_=notes; }
  void setChords(const std::vector<MidiNote>* notes) noexcept { chords_=notes; }
  template<class Sink>
  void process(const Pair& pattern, double startPpq, double endPpq,
               int blockSize, int64_t patternTicks, Sink&& sink, bool playing,
               LiveControls live={}) noexcept {
    if(blockSize<=0) return;
    if(!playing || !std::isfinite(startPpq) || !std::isfinite(endPpq) ||
       endPpq<=startPpq) {
      if(!flush(sink,0)) return;
      haveExpected_=false;
      return;
    }
    if(haveExpected_ && std::abs(startPpq-expectedPpq_)>0.002) {
      if(!flush(sink,0)) return;
    }
    expectedPpq_=endPpq; haveExpected_=true;
    // Rendered note positions are in MIDI ticks (960 per quarter note).
    const int64_t period=patternTicks;
    if(period<=0) return;
    std::array<ScheduledMidiEvent,kCapacity> events{};
    int count=0;
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
        if(last-first>64) { flush(sink,0); return; }
        for(int64_t n=first;n<=last;++n) {
          const double cycle=periodPpq*static_cast<double>(n);
          auto push=[&](double t,bool on) {
            if(t<startPpq || t>=endPpq || count>=kCapacity) return;
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
                // Currently natural-minor scale. Base arrangement is root degree 1.
                constexpr std::array<int,7> scale{0,2,3,5,7,8,10};
                pitch+=degreeOffset(degree,scale);
                const int articulation=live.guitarArticulation[static_cast<std::size_t>(step)];
                if(bus==0) {
                  constexpr std::array<int,6> velocities{0,20,50,95,115,127};
                  velocity=velocities[static_cast<std::size_t>(articulation)];
                }
              }
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
          push(cycle+static_cast<double>(endTick)/960.0,false);
          push(cycle+phase,true);
        }
      }
    };
    collect(pattern.guitar,0); collect(pattern.bass,1);
    if(drums_)collect(*drums_,2);
    if(chords_)collect(*chords_,3);
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
      if(!e.on && !active_[static_cast<std::size_t>(e.bus)][static_cast<std::size_t>(e.pitch)]) continue;
      if(sink(e)) active_[static_cast<std::size_t>(e.bus)][static_cast<std::size_t>(e.pitch)]=e.on;
      else { flush(sink,e.sampleOffset); return; }
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
  double expectedPpq_{};
};
} // namespace a125::rifferator
