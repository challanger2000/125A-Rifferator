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
}
