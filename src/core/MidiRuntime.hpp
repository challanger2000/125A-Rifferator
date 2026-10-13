#pragma once
#include "Pattern.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace a125::riff {

// Host-neutral, allocation-free MIDI event scheduler.
// Only an already-validated immutable Pattern and Settings snapshot is read.
// VST3 adapter must guarantee snapshot ownership while process() runs.
enum class SyncMode : std::uint8_t { Transport, MidiNote };
struct TriggerEvent {
  int sampleOffset{};
  int pitch{};
  bool noteOn{};
};
struct MidiOut {
  int sampleOffset{};
  int pitch{};
  int velocity{};
  bool noteOn{};
};
struct Block {
  int numSamples{};
  double sampleRate{48000.0};
  double tempo{120.0};
  double projectPpq{};   // position in quarter notes, not ticks
  bool playing{};
  bool ppqValid{};
  bool tempoValid{};
};
class MidiRuntime final {
public:
  // Hard real-time upper bounds. Invalid/oversized buffers fail closed.
  static constexpr int kMaxTriggerEvents=128;
  static constexpr int kMaxScheduledBoundaries=512;

  void setMode(SyncMode m) noexcept { mode_=m; }
  SyncMode mode() const noexcept { return mode_; }
  int activePitch() const noexcept { return activePitch_; }
  bool triggerHeld() const noexcept { return held_; }
  // Only call reset() when deactivating / disconnecting the output bus.
  // If a host is still playing, process a stop first to flush the active note.
  void reset() noexcept {
    activePitch_=-1; held_=false;heldPitch_=-1;phaseTicks_=0.;
    haveExpected_=false;recovery_=false;seenMode_=mode_;
  }

  template<class Policy,class Sink>
  bool process(const Pattern& pattern,const Settings& settings,const Block& block,
               const TriggerEvent* triggers,int triggerCount,Policy&& policy,Sink&& sink) noexcept {
    if(block.numSamples<0||!std::isfinite(block.sampleRate)||block.sampleRate<=0.||
       !std::isfinite(block.tempo)||block.tempo<=0.||block.tempo>1000.||
       triggerCount<0||triggerCount>kMaxTriggerEvents||
       (triggerCount>0&&!triggers)||pattern.activeSteps()<16||
       !pattern.valid()||!settings.valid()||
       !pitchInScale(settings.rootMidi,settings)){
      release(0,sink);
      haveExpected_=false;held_=false;
      return false;
    }
    if(recovery_){
      if(!release(0,sink))return false;
      recovery_=false;
    }
    if(mode_!=seenMode_){
      if(!release(0,sink))return false;
      held_=false;phaseTicks_=0.;haveExpected_=false;
      seenMode_=mode_;
    }
    const double rate=block.tempo*static_cast<double>(kPpq)/(60.*block.sampleRate);
    if(!std::isfinite(rate)||rate<=0.)return false;
    if(mode_==SyncMode::Transport){
      held_=false;
      if(!block.playing||!block.ppqValid||!block.tempoValid||
         !std::isfinite(block.projectPpq)||std::abs(block.projectPpq)>1e9){
        haveExpected_=false;
        return release(0,sink);
      }
      if(haveExpected_&&std::abs(block.projectPpq-expectedPpq_)>1.e-5){
        if(!release(0,sink))return false;
      }
      expectedPpq_=block.projectPpq+static_cast<double>(block.numSamples)*rate/kPpq;
      haveExpected_=true;
      return schedule(pattern,settings,block.projectPpq*kPpq,0,
                      block.numSamples,rate,policy,sink);
    }

    haveExpected_=false;
    // Sort tiny input on the stack. NoteOff wins before NoteOn at a shared sample.
    TriggerEvent incoming[kMaxTriggerEvents]{};
    for(int i=0;i<triggerCount;++i){
      const auto e=triggers[i];
      if(e.sampleOffset<0||e.sampleOffset>=block.numSamples||e.pitch<0||e.pitch>127){
        held_=false;heldPitch_=-1;phaseTicks_=0.;
        release(0,sink);return false;
      }
      int pos=i;
      while(pos>0&&(incoming[pos-1].sampleOffset>e.sampleOffset||
          (incoming[pos-1].sampleOffset==e.sampleOffset&&
           incoming[pos-1].noteOn&&!e.noteOn))){
        incoming[pos]=incoming[pos-1];--pos;
      }
      incoming[pos]=e;
    }
    int current=0;
    for(int i=0;i<triggerCount;++i){
      const auto event=incoming[i];
      if(held_&&event.sampleOffset>current){
        if(!schedule(pattern,settings,phaseTicks_,current,
                     event.sampleOffset-current,rate,policy,sink))return false;
        phaseTicks_+=static_cast<double>(event.sampleOffset-current)*rate;
      }
      current=event.sampleOffset;
      if(!event.noteOn){
        if(held_&&event.pitch==heldPitch_){
          if(!release(current,sink))return false;
          held_=false;heldPitch_=-1;phaseTicks_=0.;
        }
      }else{
        if(!release(current,sink))return false;
        held_=true;heldPitch_=event.pitch;phaseTicks_=0.;
      }
    }
    if(held_&&current<block.numSamples){
      if(!schedule(pattern,settings,phaseTicks_,current,
                   block.numSamples-current,rate,policy,sink))return false;
      phaseTicks_+=static_cast<double>(block.numSamples-current)*rate;
      // Keep phase numerically bounded; a sustained trigger loops freely.
      if(phaseTicks_>=pattern.lengthTicks())
        phaseTicks_=std::fmod(phaseTicks_,static_cast<double>(pattern.lengthTicks()));
    }
    return true;
  }

private:
  template<class Sink>
  bool release(int offset,Sink& sink) noexcept {
    if(activePitch_<0)return true;
    if(!sink(MidiOut{offset,activePitch_,0,false})){
      recovery_=true;
      return false;
    }
    activePitch_=-1;
    return true;
  }

  template<class Policy,class Sink>
  bool schedule(const Pattern& p,const Settings& s,double initialTick,int offset,
                int count,double rate,Policy& policy,Sink& sink) noexcept {
    if(count<=0)return true;
    if(!std::isfinite(initialTick)||std::abs(initialTick)>1.e12)return false;
    const double end=initialTick+static_cast<double>(count)*rate;
    const double firstGrid=std::ceil((initialTick-1.e-7)/kTicksPerStep);
    if(firstGrid>static_cast<double>(std::numeric_limits<std::int64_t>::max()/kTicksPerStep)||
       firstGrid<static_cast<double>(std::numeric_limits<std::int64_t>::min()/kTicksPerStep))return false;
    const auto first=static_cast<std::int64_t>(firstGrid);
    const double gridCount=std::max(0.,std::ceil((end-first*kTicksPerStep)/kTicksPerStep));
    if(gridCount>kMaxScheduledBoundaries){
      if(!release(offset,sink))return false;
      return false;
    }
    for(int i=0;i<kMaxScheduledBoundaries;++i){
      const auto grid=first+static_cast<std::int64_t>(i);
      const double tick=static_cast<double>(grid)*kTicksPerStep;
      if(tick>=end-1.e-8)break; // event exactly on next callback belongs to next block
      if(tick<initialTick-1.e-7||grid<0)continue;
      int local=static_cast<int>(std::floor((tick-initialTick)/rate+1.e-8));
      local=std::clamp(local,0,count-1);
      const int sample=offset+local;
      const int stepIndex=static_cast<int>(grid%p.activeSteps());
      const auto& step=p.steps[static_cast<std::size_t>(stepIndex)];
      if(!step.on)continue;
      if(!release(sample,sink))return false;
      if(step.stop)continue;
      const int pitch=pitchForDegree(step.degree,s);
      if(pitch<0||pitch>127)return false;
      const int vel=policy(step.articulation,s);
      if(vel<=0||vel>127)return false;
      if(!sink(MidiOut{sample,pitch,vel,true}))return false;
      activePitch_=pitch;
    }
    return true;
  }
  SyncMode mode_{SyncMode::Transport},seenMode_{SyncMode::Transport};
  int activePitch_{-1},heldPitch_{-1};
  bool held_{false},haveExpected_{false},recovery_{false};
  double phaseTicks_{},expectedPpq_{};
};
} // namespace a125::riff
