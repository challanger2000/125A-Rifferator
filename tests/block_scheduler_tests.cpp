#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  std::vector<Step> steps(16,Step{Articulation::Off,1,false,false});
  steps[0]={Articulation::PalmMute,1,false,false};
  steps[4]={Articulation::Open,3,true,false};
  Config config;
  auto pattern=render(steps,config);
  BlockScheduler scheduler;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  // 16 1/16th steps => four quarters; no shortening due to trailing rest.
  const int64_t period=16*240;
  scheduler.process(pattern,0.0,1.0,48000,period,sink,true);
  int onG=0,onB=0;
  for(const auto& e:events) if(e.on) { if(e.bus==0)++onG;else ++onB; }
  assert(onG==1 && onB==1);
  events.clear();
  scheduler.process(pattern,1.0,2.0,48000,period,sink,true);
  for(const auto& e:events) assert(e.sampleOffset>=0 && e.sampleOffset<48000);
  events.clear();
  scheduler.process(pattern,2.0,3.0,48000,period,sink,false);
  for(const auto& e:events) assert(!e.on);
  events.clear();
  scheduler.process(pattern,4.0,5.0,48000,period,sink,true);
  assert(!events.empty());

  scheduler.reset();
  events.clear();
  scheduler.process(pattern,0.0,1.0,48000,period,sink,true);
  bool rejected=false;
  auto rejectOff=[&](const ScheduledMidiEvent& e) {
    if(!e.on && !rejected) { rejected=true; return false; }
    events.push_back(e); return true;
  };
  scheduler.process(pattern,1.0,2.0,48000,period,rejectOff,false);
  assert(rejected);
  int recoveredOffs=0;
  auto recovery=[&](const ScheduledMidiEvent& e) {
    if(!e.on) ++recoveredOffs;
    return true;
  };
  assert(scheduler.flush(recovery,0));
  assert(recoveredOffs>=1);
  scheduler.reset();
}
