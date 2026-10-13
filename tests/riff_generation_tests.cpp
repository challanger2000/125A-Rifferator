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
 // User-requested BAB ba-ba gallop: 3 PM attacks / quarter note,
 // starts on 16th steps 0,2,3 and repeats every four 16th steps.
 std::vector<Step> input(16,Step{Articulation::Open,3,true,false});
 input[5]={Articulation::Harmonic,7,true,true};
 auto gallop=generateGuitar(input,options,GenerationCommand::Gallop);
 auto repeat=generateGuitar(input,options,GenerationCommand::Gallop);
 for(int i=0;i<16;++i) {
   assert(same(gallop[i],repeat[i])); // Never random, never a hidden subdivision.
   if(i==5){assert(same(gallop[i],input[i]));continue;}
   const bool attack=(i%4)!=1;
   assert(gallop[i].articulation==(attack?Articulation::PalmMute:Articulation::Off));
   assert(gallop[i].degree==1);
   assert(!gallop[i].doubled);
 }
 // The first pulse is at step 0, followed by steps 2 and 3,
 // i.e. 1/8 plus 1/16 plus 1/16, never 1/32 doubles.
 Config cfg;
 auto clean=makeGallop(std::vector<Step>(16,Step{Articulation::Off,1,false,false}));
 auto notes=render(clean,cfg);
 assert(notes.size()==12);
 assert(notes[0].startTick==0 && notes[1].startTick==480 &&
        notes[2].startTick==720 && notes[3].startTick==960);
 auto none=generateGuitar(steps,options,GenerationCommand::Variate,0);
 for(int i=0;i<16;++i)assert(same(none[i],steps[i]));
}
