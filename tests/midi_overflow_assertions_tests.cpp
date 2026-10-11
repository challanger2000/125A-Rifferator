#include "../src/core/BlockScheduler.hpp"
#ifdef NDEBUG
#error Regression tests must compile with assertions enabled in Release.
#endif
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  Pair huge;
  huge.guitar.reserve(700);
  for(int i=0;i<700;++i)
    huge.guitar.push_back({0,200,40,110,240});
  LiveControls live;
  live.guitarSteps.fill(false);live.guitarSteps[0]=true;
  live.guitarArticulation[0]=2;
  BlockScheduler scheduler;
  std::vector<ScheduledMidiEvent> emitted;
  auto sink=[&](ScheduledMidiEvent e){emitted.push_back(e);return true;};
  scheduler.process(huge,0.,0.25,960,3840,sink,true,live);
  // No partial MIDI batches on overflow.
  assert(emitted.empty());
  assert(scheduler.flush(sink,0));
  assert(emitted.empty());
}
