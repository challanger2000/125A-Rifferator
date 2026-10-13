#include "../src/core/RiffGeneration.hpp"
#include <cassert>
using namespace a125::rifferator;
static bool same(const Step& a,const Step& b) {
 return a.articulation==b.articulation&&a.degree==b.degree&&
   a.doubled==b.doubled&&a.locked==b.locked;
}
int main(){
 std::vector<Step> steps(16,Step{Articulation::PalmMute,1,false,false});
 steps[5]={Articulation::Open,4,true,true};
 RandomOptions options;options.seed=12345;
 for(auto mode:{GenerationCommand::Generate,GenerationCommand::Variate}){
  auto a=generateGuitar(steps,options,mode,30),b=generateGuitar(steps,options,mode,30);
  assert(a.size()==16);
  for(int i=0;i<16;++i)assert(same(a[i],b[i]));
  assert(same(a[5],steps[5]));
 }
 auto none=generateGuitar(steps,options,GenerationCommand::Variate,0);
 for(int i=0;i<16;++i)assert(same(none[i],steps[i]));
}
