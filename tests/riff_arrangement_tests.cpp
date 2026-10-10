#include "../src/core/RiffArrangement.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  std::vector<Step> base(16,Step{Articulation::Off,1,false,false});
  base[0]={Articulation::PalmMute,1,false,false};
  base[15]={Articulation::Open,1,false,false};
  FillOptions fill;fill.everyBars=16;fill.intensityPercent=0;
  Config config;
  auto a=arrange(base,config,fill,16);
  assert(a.periodTicks==61440);
  assert(a.midi.guitar.size()==32 && a.midi.bass.size()==32);
  assert(a.midi.guitar[0].startTick==0);
  assert(a.midi.guitar[2].startTick==3840);
  fill.intensityPercent=100;
  auto b=arrange(base,config,fill,16);
  auto again=arrange(base,config,fill,16);
  assert(b.periodTicks==a.periodTicks);
  assert(b.midi.guitar.size()==again.midi.guitar.size());
  assert(b.midi.bass.size()==again.midi.bass.size());
  for(std::size_t i=0;i<a.midi.guitar.size()-2;++i)
    assert(a.midi.guitar[i].startTick==b.midi.guitar[i].startTick);
  for(std::size_t i=0;i<b.midi.guitar.size();++i)
    assert(b.midi.guitar[i].startTick==again.midi.guitar[i].startTick);
}
