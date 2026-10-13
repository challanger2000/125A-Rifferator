#pragma once
#include "RiffEngine.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <vector>

namespace a125::rifferator {
struct ScheduledMidiEvent {
  int bus{},sampleOffset{},pitch{},velocity{},noteId{-1};
  bool on{};
};
struct GuitarLive {
  int root{40},preparedRoot{40},gate{95},pmMin{30},pmMax{40};
  std::array<int,7> scale{0,2,3,5,7,8,10};
  std::array<bool,16> enabled{},locked{},doubled{};
  std::array<int,16> articulation{},degree{};
  GuitarLive() noexcept {
    for(int i=0;i<16;++i) {
      enabled[i]=i%2==0; articulation[i]=2;degree[i]=1;
    }
  }
};
class BlockScheduler {
public:
  static constexpr int kCapacity=1024;
  template<class Sink> bool flush(Sink&& sink,int offset) noexcept {
    bool ok=true;
    for(int pitch=0;pitch<128;++pitch) if(active_[pitch]) {
      if(sink(ScheduledMidiEvent{0,offset,pitch,0,-1,false})) active_[pitch]=false;
      else ok=false;
    }
    return ok;
  }
  void reset() noexcept {active_.fill(false);haveExpected_=false;recover_=false;}
  template<class Sink>
  void process(const std::vector<MidiNote>& notes,double start,double end,
               int samples,int64_t periodTicks,Sink&& sink,bool playing,
               const GuitarLive& live) noexcept {
    if(samples<=0)return;
    if(recover_) {
      if(flush(sink,0)){recover_=false;haveExpected_=false;}
      return; // Do not dispatch new NoteOns until releases have succeeded.
    }
    if(!playing||!std::isfinite(start)||!std::isfinite(end)||end<=start||
       periodTicks<=0||std::abs(start)>1.e12||std::abs(end)>1.e12) {
      if(!flush(sink,0))recover_=true;
      haveExpected_=false;
      return;
    }
    if(haveExpected_&&std::abs(start-expected_)>0.002)
      if(!flush(sink,0)){recover_=true;return;}
    haveExpected_=true;expected_=end;
    std::array<ScheduledMidiEvent,kCapacity> events{};
    int count=0;bool overflow=false;
    const double period=static_cast<double>(periodTicks)/960.;
    auto push=[&](double when,int pitch,int vel,bool on) noexcept {
      if(when<start||when>=end)return;
      if(count==kCapacity){overflow=true;return;}
      const int offset=std::clamp(static_cast<int>(std::floor((when-start)/(end-start)*samples)),0,samples-1);
      events[static_cast<std::size_t>(count++)]={0,offset,pitch,on?vel:0,-1,on};
    };
    for(const auto& n:notes) {
      const int step=static_cast<int>((n.startTick/240)%16);
      if(step<0||step>=16||!live.enabled[step])continue;
      const int art=live.articulation[step];
      if(art<=0||art>5||live.gate==0)continue;
      const int delta=live.locked[step]?0:n.note-live.preparedRoot;
      const int pitch=std::clamp(live.root+degreeOffset(live.degree[step],live.scale)+delta,0,127);
      int vel=guitarStepVelocity(art);
      if(art==2) {
        const int lo=std::clamp(live.pmMin,1,127);
        const int hi=std::clamp(live.pmMax,lo,127);
        constexpr int accent[4]{4,1,3,2};
        vel=lo+(hi-lo)*accent[step%4]/4;
      }
      const int64_t t=n.startTick;
      int64_t boundary=n.boundaryTick;
      // Only OPEN/FIFTH can sustain across empty steps.
      if(art==4&&!live.doubled[step]) {
        for(int distance=1;distance<=16;++distance) {
          const int candidate=(step+distance)%16;
          if(live.enabled[candidate]) {
            boundary=t+static_cast<int64_t>(distance)*240;
            break;
          }
        }
      } else boundary=std::min<int64_t>(boundary,t+240);
      const int attacks=live.doubled[step]?2:1;
      for(int hit=0;hit<attacks;++hit) {
        const int64_t begin=t+(attacks==2?120*hit:0);
        const int64_t limit=attacks==2?t+120*(hit+1):boundary;
        const int64_t off=std::max(begin+1,begin+(limit-begin)*live.gate/100);
        // DOUBLE is a rhythmic subdivision, not a Kontakt sample-articulation
        // instruction. Both hits must retain the current instrument's PM
        // velocity range; Heavier7Strings chooses UP/DOWN in AUTO itself.
        // A small secondary accent is constrained to the user-set PM range.
        const int lo=std::clamp(live.pmMin,1,127);
        const int hi=std::clamp(live.pmMax,lo,127);
        const int v=(art==2&&hit==1)
          ?std::clamp(vel-std::min(4,(hi-lo)/3),lo,hi)
          :vel;
        const double phase=static_cast<double>(begin)/960.;
        const double release=static_cast<double>(off)/960.;
        const int64_t first=static_cast<int64_t>(std::floor((start-phase)/period))-1;
        const int64_t last=static_cast<int64_t>(std::ceil((end-phase)/period))+1;
        if(last-first>64){overflow=true;break;}
        for(int64_t cycle=first;cycle<=last;++cycle) {
          const double anchor=period*static_cast<double>(cycle);
          push(anchor+release,pitch,0,false);
          push(anchor+phase,pitch,v,true);
        }
      }
      if(overflow)break;
    }
    // STOP is a deliberate active step; OFF is an inactive step.
    // STOP must release every held guitar pitch, including across loops.
    if(!overflow)for(int step=0;step<16;++step) {
      if(!live.enabled[step]||live.articulation[step]!=0)continue;
      const double phase=static_cast<double>(step)*.25;
      const double loop=4.;
      const int64_t first=static_cast<int64_t>(std::floor((start-phase)/loop))-1;
      const int64_t last=static_cast<int64_t>(std::ceil((end-phase)/loop))+1;
      if(last-first>64){overflow=true;break;}
      for(int64_t cycle=first;cycle<=last;++cycle)
        push(static_cast<double>(cycle)*loop+phase,-1,0,false);
    }
    if(overflow) {
      if(!flush(sink,0))recover_=true;
      haveExpected_=false;
      return;
    }
    // Stable ordering: release or STOP before attack at coincident offsets.
    for(int i=1;i<count;++i) {
      const auto key=events[static_cast<std::size_t>(i)];
      int j=i-1;
      while(j>=0&&(events[static_cast<std::size_t>(j)].sampleOffset>key.sampleOffset||
        (events[static_cast<std::size_t>(j)].sampleOffset==key.sampleOffset&&
         events[static_cast<std::size_t>(j)].on&&!key.on))) {
        events[static_cast<std::size_t>(j+1)]=events[static_cast<std::size_t>(j)];
        --j;
      }
      events[static_cast<std::size_t>(j+1)]=key;
    }
    for(int i=0;i<count;++i) {
      const auto e=events[static_cast<std::size_t>(i)];
      if(e.pitch==-1) {
        if(!flush(sink,e.sampleOffset)){recover_=true;return;}
        continue;
      }
      if(!e.on&&!active_[e.pitch])continue;
      if(e.on&&active_[e.pitch]) {
        if(!sink(ScheduledMidiEvent{0,e.sampleOffset,e.pitch,0,-1,false})){
          recover_=true;return;
        }
        active_[e.pitch]=false;
      }
      if(sink(e))active_[e.pitch]=e.on;
      else {recover_=true;return;}
    }
  }
private:
  std::array<bool,128> active_{};
  double expected_{};
  bool haveExpected_{},recover_{};
};
} // namespace a125::rifferator
