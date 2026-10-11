#include "../src/core/IndustrialGuitarGroove.hpp"
#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  const auto riff=makeIndustrialGuitarGroove();
  assert(riff.enabled[0]&&riff.articulation[0]==4);
  assert(riff.enabled[1]&&riff.articulation[1]==2);
  assert(!riff.enabled[3]);
  assert(makeIndustrialGuitarGroove(85).articulation[14]==4);
  for(int i=0;i<16;++i){
    assert(riff.degrees[i]==1);
    if(i%4==3)assert(!riff.enabled[i]);
    if(i%4==0){assert(riff.articulation[i]==4);assert(riff.velocity[i]==127);}
    if(i%4==1||i%4==2)assert(riff.articulation[i]==2);
  }
  assert(!makeIndustrialGuitarGroove(49).enabled[15]);
  assert(makeIndustrialGuitarGroove(50).enabled[15]);
  Pair pair;
  pair.guitar.push_back({0,200,40,110,240});
  BlockScheduler scheduler;
  LiveControls live;
  live.guitarSteps.fill(false);
  live.guitarSteps[0]=true;
  live.guitarArticulation[0]=4;
  live.guitarDegree[0]=1;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  scheduler.process(pair,0.,0.25,960,61440,sink,true,live);
  bool root=false,fifth=false;
  for(const auto& e:events)if(e.on){if(e.pitch==40)root=true;if(e.pitch==47)fifth=true;assert(e.bus==0);}
  assert(root&&fifth);
  // Both notes have already received their scheduled NoteOff within
  // this complete quarter-note block; flush should emit nothing further.
  root=false;fifth=false;
  for(const auto& e:events)if(!e.on){if(e.pitch==40)root=true;if(e.pitch==47)fifth=true;}
  assert(root&&fifth);
  events.clear();
  assert(scheduler.flush(sink,0));
  assert(events.empty());
}
