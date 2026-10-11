#include "../src/core/BlockScheduler.hpp"
#include "../src/core/RiffArrangement.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  Config cfg;
  cfg.rootMidi=40;
  std::vector<Step> candidates(16);
  for(auto& s:candidates){s.articulation=Articulation::PalmMute;s.degree=1;}
  auto arrangement=arrange(candidates,cfg,FillOptions{},16);
  assert(arrangement.midi.guitar.size()==256);
  LiveControls controls;
  controls.absoluteRoot=47;
  controls.guitarSteps.fill(false);
  controls.guitarArticulation.fill(0);
  BlockScheduler scheduler;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](ScheduledMidiEvent e){events.push_back(e);return true;};
  scheduler.process(arrangement.midi,0.,0.25,960,arrangement.periodTicks,sink,true,controls);
  assert(events.empty());
  scheduler.reset();events.clear();
  controls.guitarSteps[0]=true;
  controls.guitarArticulation[0]=2;
  controls.guitarDegree[0]=1;
  scheduler.process(arrangement.midi,0.,0.25,960,arrangement.periodTicks,sink,true,controls);
  int noteOns=0,noteOffs=0;
  for(auto e:events)if(e.bus==0){if(e.on){assert(e.pitch==47);++noteOns;}else ++noteOffs;}
  assert(noteOns==1 && noteOffs==1);
  scheduler.reset();events.clear();
  controls.guitarSteps[0]=false;
  controls.guitarArticulation[0]=0;
  scheduler.process(arrangement.midi,0.,0.25,960,arrangement.periodTicks,sink,true,controls);
  for(auto e:events)assert(e.bus!=0);
}
