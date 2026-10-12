#include "../src/core/RiffArrangement.hpp"
#include "../src/core/BlockScheduler.hpp"
#ifdef NDEBUG
#error CI assertions disabled
#endif
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  std::vector<Step> base(16);
  for(auto& s:base){s.articulation=Articulation::PalmMute;s.degree=1;}
  Config cfg;
  FillOptions plain;plain.intensityPercent=0;
  FillOptions filled;filled.intensityPercent=100;filled.everyBars=16;
  const auto dry=arrange(base,cfg,plain,16);
  const auto wet=arrange(base,cfg,filled,16);
  assert(dry.midi.guitar.size()==wet.midi.guitar.size());
  LiveControls live;
  live.guitarArticulation.fill(2);
  live.guitarSteps.fill(true);
  live.absoluteRoot=40;
  live.preparedRoot=40;
  auto render=[&](const Arrangement& a) {
    BlockScheduler s;
    std::vector<ScheduledMidiEvent> ev;
    auto sink=[&](ScheduledMidiEvent e){ev.push_back(e);return true;};
    s.process(a.midi,62.,64.,2048,a.periodTicks,sink,true,live);
    std::vector<int> pitches;
    for(auto e:ev)if(e.bus==0&&e.on)pitches.push_back(e.pitch);
    return pitches;
  };
  auto p0=render(dry),p1=render(wet);
  assert(!p0.empty());
  assert(p0.size()==p1.size());
  bool difference=false;
  for(std::size_t i=0;i<p0.size();++i)if(p0[i]!=p1[i])difference=true;
  assert(difference);
  // Live LOCK must suppress precomputed final-bar FILL pitches immediately.
  // The prepared phrase is immutable on the audio thread, so the runtime
  // must mask the cached variation instead of regenerating the phrase.
  int target=-1,preparedDelta=0;
  int64_t startTick=0;
  for(const auto& note:wet.midi.guitar) {
    if(note.startTick<15*3840 || note.startTick>=16*3840)continue;
    const int step=static_cast<int>((note.startTick/240)%16);
    if(step<8 || note.note==cfg.rootMidi)continue;
    target=step;startTick=note.startTick;
    preparedDelta=note.note-cfg.rootMidi;
    break;
  }
  assert(target>=8 && target<16 && preparedDelta!=0);
  LiveControls controls;
  controls.guitarSteps.fill(false);
  controls.guitarSteps[static_cast<std::size_t>(target)]=true;
  controls.guitarArticulation[static_cast<std::size_t>(target)]=2;
  controls.guitarDegree[static_cast<std::size_t>(target)]=3; // user degree stays intact
  controls.absoluteRoot=40;
  controls.preparedRoot=40;
  auto pitchesFor=[&](bool locked) {
    controls.guitarLocks[static_cast<std::size_t>(target)]=locked;
    BlockScheduler scheduler;
    std::vector<ScheduledMidiEvent> events;
    const double ppq=static_cast<double>(startTick)/960.;
    scheduler.process(wet.midi,ppq,ppq+.25,960,wet.periodTicks,
      [&](ScheduledMidiEvent e){events.push_back(e);return true;},true,controls);
    int guitar=-1,bass=-1,ng=0,nb=0;
    for(const auto& e:events)if(e.on) {
      if(e.bus==0){guitar=e.pitch;++ng;}
      if(e.bus==1){bass=e.pitch;++nb;}
    }
    assert(ng==1 && nb==1);
    return std::array<int,2>{guitar,bass};
  };
  const int expected=40+degreeOffset(3,controls.scale);
  const auto unlocked=pitchesFor(false);
  const auto locked=pitchesFor(true);
  assert(unlocked[0]==expected+preparedDelta);
  assert(unlocked[1]==expected+preparedDelta-12);
  assert(locked[0]==expected);
  assert(locked[1]==expected-12);
}
