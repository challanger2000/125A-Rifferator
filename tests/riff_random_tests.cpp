#include "../src/core/RiffRandom.hpp"
#include <cassert>
using namespace a125::rifferator;
int main(){
 std::vector<Step> base(32,Step{Articulation::PalmMute,1,false,false});
 base[2]={Articulation::Open,4,true,true};
 RandomOptions options;options.randomPitch=true;
 auto a=randomize(base,options),b=randomize(base,options);
 assert(a.size()==32&&b.size()==32);
 for(int i=0;i<32;++i){
  assert(a[i].articulation==b[i].articulation);
  assert(a[i].degree==b[i].degree);
  assert(a[i].doubled==b[i].doubled);
 }
 assert(a[2].articulation==Articulation::Open&&a[2].degree==4&&a[2].locked);
 Config cfg;
 for(const auto& n:render(a,cfg))assert(n.velocity>0&&n.velocity<=127);
 options.densityPercent=0;
 const auto silent=randomize(base,options);
 for(int i=0;i<32;++i)if(i!=2)assert(silent[i].articulation==Articulation::Off);
}
