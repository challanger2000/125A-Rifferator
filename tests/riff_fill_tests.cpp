#include "../src/core/RiffFill.hpp"
#include <cassert>
using namespace a125::rifferator;
int main(){
 std::vector<Step> base(16,Step{Articulation::PalmMute,1,false,false});
 base[14]={Articulation::Open,3,false,true};
 FillOptions o;o.intensityPercent=100;o.everyBars=16;
 const auto standard=applyFill(base,o,14);
 const auto fill=applyFill(base,o,15);
 for(int i=0;i<16;++i) {
  assert(standard[i].degree==base[i].degree);
  if(i<8)assert(fill[i].degree==base[i].degree);
 }
 assert(fill[14].degree==3);
 assert(fill[14].articulation==Articulation::Open);
 o.intensityPercent=0;
 const auto off=applyFill(base,o,15);
 for(int i=0;i<16;++i)assert(off[i].degree==base[i].degree);
}
