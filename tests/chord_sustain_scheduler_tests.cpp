#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  const auto chords=makeChordCandidates();
  BlockScheduler scheduler;
  scheduler.setChords(&chords);
  LiveControls live;
  live.chordsEnabled=true;
  live.chordQuality=0;
  live.harmony.roots.fill(40);
  Pair empty;
  std::vector<ScheduledMidiEvent> e;
  auto sink=[&](ScheduledMidiEvent v){e.push_back(v);return true;};
  const int64_t period=16*3840;
  scheduler.process(empty,0.,1.,1000,period,sink,true,live);
  int on=0;for(auto v:e)if(v.bus==3 && v.on)++on;
  assert(on==3);
  e.clear();
  scheduler.process(empty,1.,4.,3000,period,sink,true,live);
  for(auto v:e)assert(v.bus!=3);
  // Change root at bar 1. Existing chord must end and new chord begin.
  scheduler.reset();
  e.clear();
  live.harmony.roots[1]=43;
  scheduler.process(empty,0.,8.,8000,period,sink,true,live);
  int at4On=0,at4Off=0;
  for(auto v:e)if(v.bus==3 && v.sampleOffset==4000) {
    if(v.on)++at4On;else ++at4Off;
  }
  assert(at4On==3 && at4Off==3);
}
