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
  // Gate-to-zero must release a note currently sounding, not only block new ons.
  events.clear();
  assert(adapter.flush(sink,0));
  adapter.reset();
  assert(adapter.process(arrangement,0.,0.125,512,true,sink,0,100));
  bool sounding=false;
  for(const auto& e:events)if(e.on)sounding=true;
  assert(sounding);
  events.clear();
  assert(adapter.process(arrangement,0.125,0.25,512,true,sink,0,0));
  bool sawRelease=false;
  for(const auto& e:events) {
    if(!e.on && e.bus==1)sawRelease=true;
    assert(!e.on);
  }
  assert(sawRelease);
  events.clear();
  assert(adapter.process(arrangement,0.25,0.375,512,true,sink,0,0));
  assert(events.empty());
  // HYBRID follows a true bar-level harmonic change, not every chromatic note.
  HarmonyMap harmony;
  harmony.roots[1]=41; // C to C# as a new harmonic root in bar 2
  events.clear();
  adapter.reset();
  assert(adapter.process(arrangement,4.,4.25,960,true,sink,0,100,&harmony,40));
  bool shifted=false;
  for(const auto& e:events)if(e.on) {
    assert(e.pitch==41);
    shifted=true;
  }
  assert(shifted);
  events.clear();
  assert(adapter.flush(sink));
  adapter.reset();
  assert(adapter.process(arrangement,4.,4.25,960,true,sink,0,100,nullptr,40));
  bool unchanged=false;
  for(const auto& e:events)if(e.on) {
    assert(e.pitch==40);
    unchanged=true;
  }
  assert(unchanged);
  events.clear();
  assert(adapter.flush(sink,0));
  adapter.reset();
  assert(adapter.process(arrangement,0.,0.25,960,true,sink,0,100,nullptr,40,nullptr,120));
  bool stronger=false;
  for(const auto& e:events)if(e.on) {
    assert(e.velocity==std::clamp(arrangement.midi.bass.front().velocity+20,1,127));
    stronger=true;
  }
  assert(stronger);
}

