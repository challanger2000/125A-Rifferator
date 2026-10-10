#pragma once
#include "RiffEngine.hpp"
#include <array>
#include <cmath>
#include <cstdint>

// Zero-allocation block scheduler: render() is used only off the audio callback.
// The VST3 adapter supplies the authoritative host PPQ and sample rate.
namespace a125::rifferator {
struct ScheduledMidiEvent {
  int bus{}, sampleOffset{}, pitch{}, velocity{}, noteId{-1};
  bool on{};
};
class BlockScheduler {
public:
  static constexpr int kCapacity=256;
  void reset() noexcept { active_={}; haveExpected_=false; }
  template<class Sink>
  void process(const Pair& pattern, double startPpq, double endPpq,
               int blockSize, Sink&& sink, bool playing) noexcept {
    if(blockSize<=0) return;
    if(!playing || !std::isfinite(startPpq) || !std::isfinite(endPpq) ||
       endPpq<=startPpq) {
      flush(sink,0);
      haveExpected_=false;
      return;
    }
    if(haveExpected_ && std::abs(startPpq-expectedPpq_)>0.002) flush(sink,0);
    expectedPpq_=endPpq; haveExpected_=true;
    // Rendered note positions are in MIDI ticks (960 per quarter note).
    const int64_t period=patternPeriod(pattern);
    if(period<=0) return;
    std::array<ScheduledMidiEvent,kCapacity> events{};
    int count=0;
    auto collect=[&](const std::vector<MidiNote>& notes,int bus) {
      for(const auto& note:notes) {
        const double periodPpq=static_cast<double>(period)/960.0;
        const double phase=static_cast<double>(note.startTick)/960.0;
        const int64_t first=static_cast<int64_t>(std::floor((startPpq-phase)/periodPpq))-1;
        const int64_t last=static_cast<int64_t>(std::ceil((endPpq-phase)/periodPpq))+1;
        // protect realtime complexity on corrupt/huge host jumps
        if(last-first>64) continue;
        for(int64_t n=first;n<=last;++n) {
          const double cycle=periodPpq*static_cast<double>(n);
          auto push=[&](double t,bool on) {
            if(t<startPpq || t>=endPpq || count>=kCapacity) return;
            const auto offset=static_cast<int>(std::floor(
              (t-startPpq)/(endPpq-startPpq)*blockSize));
            events[static_cast<std::size_t>(count++)]={
              bus,offset,note.note,on?note.velocity:0,-1,on};
          };
          push(cycle+static_cast<double>(note.endTick)/960.0,false);
          push(cycle+phase,true);
        }
      }
    };
    collect(pattern.guitar,0); collect(pattern.bass,1);
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
    }
  }
  template<class Sink> void flush(Sink&& sink,int offset) noexcept {
    for(int bus=0;bus<2;++bus) for(int pitch=0;pitch<128;++pitch) {
      auto& active=active_[static_cast<std::size_t>(bus)][static_cast<std::size_t>(pitch)];
      if(active && sink(ScheduledMidiEvent{bus,offset,pitch,0,-1,false}))active=false;
    }
  }
private:
  // Explicit configured pattern length must be passed by the adapter: this
  // scheduler currently uses the final note end as a provisional bound.
  // A full-grid silence at the tail requires its full pattern period.
  static int64_t patternPeriod(const Pair& p) noexcept {
    int64_t maxTick=0;
    for(const auto& n:p.guitar) if(n.endTick>maxTick)maxTick=n.endTick;
    for(const auto& n:p.bass) if(n.endTick>maxTick)maxTick=n.endTick;
    return ((maxTick+239)/240)*240;
  }
  std::array<std::array<bool,128>,2> active_{};
  bool haveExpected_{};
  double expectedPpq_{};
};
} // namespace a125::rifferator
