#include "../src/core/BlockScheduler.hpp"
#include "../src/core/EbmBassArrangement.hpp"
#include "../src/core/EbmSchedulerAdapter.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
  Pair follow;
  // Guitar sustains beyond the first callback. Bass has its own independent pitch.
  follow.guitar.push_back({0,960,40,110,960});
  follow.bass.push_back({0,200,28,92,240});
  LiveControls live;
  live.guitarArticulation.fill(3);
  live.guitarDegree.fill(1);
  live.guitarSteps.fill(true);
  BlockScheduler shared;
  EbmSchedulerAdapter independent;
  std::vector<ScheduledMidiEvent> out;
  auto sink=[&](const ScheduledMidiEvent& e) {out.push_back(e);return true;};
  shared.process(follow,0.,0.125,512,3840,sink,true,live);
  int followOn=0,guitarOn=0;
  for(const auto& e:out) {
    if(e.bus==1&&e.on) {++followOn;assert(e.pitch==28);}
    if(e.bus==0&&e.on)++guitarOn;
  }
  assert(followOn==1 && guitarOn==1);
  out.clear();
  // Switching ownership flushes only bus 1: guitar remains sounding.
  assert(shared.flushBus(1,sink,0));
  assert(independent.patternChanged(sink));
  assert(out.size()==1 && out[0].bus==1 && !out[0].on);
  out.clear();
  Arrangement ebm;
  BassPlan plan;
  plan.mode=BassMode::Ebm;
  plan.sequence.root=48;
  plan.sequence.style=::rifferator::ebm::Style::Classic;
  plan.sequence.velocity=76;
  plan.sequence.gatePermille=850;
  prepare(ebm,plan);
  assert(!ebm.midi.bass.empty());
  // The shared scheduler must never emit follow bass when EBM owns bus 1.
  live.externalBassMode=true;
  shared.process(follow,0.125,0.5,512,3840,sink,true,live);
  for(const auto& e:out)assert(e.bus!=1);
  out.clear();
  assert(independent.process(ebm,0.,0.25,512,true,sink));
  int ebmOns=0;
  for(const auto& e:out) {
    assert(e.bus==1);
    if(e.on) {
      ++ebmOns;
      assert(e.velocity==ebm.midi.bass[0].velocity);
      assert(e.pitch==ebm.midi.bass[0].note);
    }
  }
  assert(ebmOns==1);
  out.clear();
  // A failed bass NoteOff leaves it pending; guitar bus is never involved.
  bool rejected=false;
  auto rejectOnce=[&](const ScheduledMidiEvent& e) {
    if(e.bus==1&&!e.on&&!rejected){rejected=true;return false;}
    out.push_back(e);return true;
  };
  assert(!independent.patternChanged(rejectOnce));
  assert(independent.patternChanged(sink));
  assert(rejected);
  assert(out.size()==1 && out[0].bus==1 && !out[0].on);
  out.clear();
  live.externalBassMode=false;
  shared.process(follow,0.5,0.625,512,3840,sink,true,live);
  for(const auto& e:out)assert(e.bus!=1 || e.pitch==28);
}
