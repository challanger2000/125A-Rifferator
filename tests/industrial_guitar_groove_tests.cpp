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
  // Audible instrument articulations must follow the selected row, not the
  // position of the step. Check each Heavier7Strings velocity band.
  constexpr int expected[6]{0,20,50,95,115,127};
  for(int step=0;step<16;++step)for(int art=0;art<6;++art)
    assert(guitarStepVelocity(step,art)==expected[art]);
  // Full MIDI-path regression: each UI articulation selection must change
  // the actual NoteOn velocity emitted by the scheduler, not just a helper.
  for(int art=0;art<6;++art) {
    Pair one;
    one.guitar.push_back({0,200,40,110,240});
    LiveControls selected;
    selected.guitarSteps.fill(false);
    selected.guitarSteps[0]=true;
    selected.guitarArticulation[0]=art;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(one,0.,0.25,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,selected);
    int ons=0;
    for(const auto& e:actual)if(e.bus==0 && e.on) {
      ++ons;assert(e.velocity==expected[art]);
    }
    assert(ons==(art==0?0:art==4?2:1));
  }
  // UP/DOUBLE: two successive strikes at half-step spacing, not a chord.
  {
    Pair one;one.guitar.push_back({0,200,40,110,240});
    LiveControls selected;
    selected.guitarSteps.fill(false);selected.guitarSteps[0]=true;
    selected.guitarArticulation[0]=2;selected.guitarDouble[0]=true;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(one,0.,0.25,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,selected);
    std::vector<int> ons,offs;
    for(const auto& e:actual)if(e.bus==0 && e.pitch==40)
      (e.on?ons:offs).push_back(e.sampleOffset);
    assert(ons.size()==2 && offs.size()==2);
    assert(ons[0]==0 && ons[1]>=479 && ons[1]<=480);
    assert(ons[0]<offs[0] && offs[0]<=ons[1] && ons[1]<offs[1]);
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
