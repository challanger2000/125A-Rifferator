#include "../src/core/EbmBassArrangement.hpp"
#include "../src/core/EbmSchedulerAdapter.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
  BassPlan plan;
  plan.mode=BassMode::Ebm;
  plan.sequence.style=::rifferator::ebm::Style::Classic;
  plan.sequence.root=52;
  plan.sequence.velocity=70;
  plan.sequence.gatePermille=1000;
  Arrangement arrangement;
  prepare(arrangement,plan);
  assert(!arrangement.midi.bass.empty());
  assert(arrangement.midi.bass.front().note==40);
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  EbmSchedulerAdapter adapter;
  assert(adapter.process(arrangement,0.,0.25,960,true,sink,5,50));
  int ons=0,offs=0;
  for(const auto& e:events) {
    assert(e.bus==1);
    assert(e.pitch==45);
    if(e.on){++ons;assert(e.velocity==arrangement.midi.bass.front().velocity);}
    else ++offs;
  }
  assert(ons==1 && offs==1);
  events.clear();
  assert(adapter.flush(sink,0));
  adapter.reset();
  assert(adapter.process(arrangement,0.,0.25,960,true,sink,5,0));
  assert(events.empty());
  events.clear();
  assert(adapter.process(arrangement,0.,0.25,960,true,sink,-100,50));
  for(const auto& e:events)assert(e.pitch==0);
  assert(!events.empty());
}
