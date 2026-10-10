#include "../src/core/RiffFill.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  std::vector<Step> base(16,Step{Articulation::PalmMute,1,false,false});
  base[14]={Articulation::Open,3,false,true};
  FillOptions o; o.intensityPercent=100; o.everyBars=16;
  auto normal=applyFill(base,o,14);
  assert(normal.size()==base.size());
  for(std::size_t i=0;i<base.size();++i) assert(normal[i].degree==base[i].degree);
  auto fill=applyFill(base,o,15);
  auto fillAgain=applyFill(base,o,15);
  for(std::size_t i=0;i<8;++i) assert(fill[i].degree==base[i].degree);
  assert(fill[14].degree==3 && fill[14].articulation==Articulation::Open);
  for(std::size_t i=0;i<fill.size();++i) {
    assert(fill[i].degree==fillAgain[i].degree);
    assert(fill[i].articulation==fillAgain[i].articulation);
    assert(fill[i].degree>=1 && fill[i].degree<=7);
  }
  o.intensityPercent=0;
  auto disabled=applyFill(base,o,15);
  for(std::size_t i=0;i<base.size();++i) assert(disabled[i].degree==base[i].degree);
  Config cfg; auto events=render(fill,cfg);
  assert(events.bass.size()==events.guitar.size());
}
