#include "../src/core/RiffRandom.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  std::vector<Step> base(32,Step{Articulation::PalmMute,1,false,false});
  base[2]={Articulation::Open,4,true,true};
  RandomOptions opts;
  opts.randomPitch=true;
  auto a=randomize(base,opts);
  auto b=randomize(base,opts);
  assert(a.size()==32 && b.size()==32);
  for(std::size_t i=0;i<a.size();++i) { assert(a[i].articulation==b[i].articulation && a[i].degree==b[i].degree && a[i].doubled==b[i].doubled); }
  assert(a[2].articulation==Articulation::Open && a[2].degree==4 && a[2].doubled && a[2].locked);
  Config cfg; auto pair=render(a,cfg);
  for(const auto& n:pair.guitar) assert(n.velocity>0 && n.velocity<=127);
  for(const auto& n:pair.bass) assert(n.velocity==100);
  opts.densityPercent=0;
  auto empty=randomize(base,opts);
  for(std::size_t i=0;i<empty.size();++i) if(i!=2) assert(empty[i].articulation==Articulation::Off);
}
