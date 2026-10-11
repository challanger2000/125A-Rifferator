#include "../src/core/EbmBassArrangement.hpp"
#include "../src/core/EbmSchedulerAdapter.hpp"
#include <array>
#include <cassert>
#include <vector>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
  BassPlan p;
  p.mode=BassMode::Ebm;
  p.tension=Tension::Aggressive;
  p.sequence.style=::rifferator::ebm::Style::Chromatic;
  p.sequence.root=52;
  Arrangement arr;
  prepare(arr,p);
  std::array<bool,16> enabled{};
  std::array<int,16> art{},degree{};
  art.fill(3); degree.fill(1);
  enabled[2]=true;
  GuitarGuide guide;
  guide.enabled=&enabled;
  guide.articulation=&art;
  guide.degrees=&degree;
  guide.guitarRoot=40;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  EbmSchedulerAdapter adapter;
  assert(adapter.process(arr,0.5,0.75,960,true,sink,0,100,nullptr,40,&guide));
  bool found=false;
  for(auto e:events)if(e.on){assert(e.pitch==40);found=true;}
  assert(found);
  assert(adapter.flush(sink,0));
  adapter.reset();
  enabled[2]=false;events.clear();
  assert(adapter.process(arr,0.5,0.75,960,true,sink,0,100,nullptr,40,&guide));
  found=false;
  for(auto e:events)if(e.on){assert(e.pitch==41);found=true;}
  assert(found);
  assert(adapter.flush(sink,0));
  adapter.reset();
  enabled[2]=true;degree[2]=2;events.clear();
  assert(adapter.process(arr,0.5,0.75,960,true,sink,0,100,nullptr,40,&guide));
  found=false;
  for(auto e:events)if(e.on){assert(e.pitch==41);found=true;}
  assert(found);
}
