#include "core/MidiRuntime.hpp"
#include "instruments/Heavier7Strings.hpp"
#include "product/RifferatorState.hpp"
#ifdef NDEBUG
#error CORE_TEST_ASSERT_DISABLED: Release must execute all assertions.
#endif
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
using namespace a125::riff;
struct Timed {
 std::int64_t sample{};
 MidiOut e{};
};
static std::vector<Timed> render(const Pattern& p,const Settings& s) {
 MidiRuntime engine;
 std::vector<Timed> events;
 const std::int64_t cycle=static_cast<std::int64_t>(p.bars)*96000;
 const std::int64_t end=3*cycle+4096;
 const std::array<int,6> sizes{{257,1024,31,4093,1,97}};
 std::int64_t pos=0;std::size_t block=0;
 while(pos<end) {
  const int n=static_cast<int>(std::min<std::int64_t>(sizes[block++%sizes.size()],end-pos));
  Block data{n,48000.,120.,static_cast<double>(pos)/24000.,true,true,true};
  auto sink=[&](MidiOut e){
   assert(e.sampleOffset>=0&&e.sampleOffset<n);
   events.push_back({pos+e.sampleOffset,e});return true;
  };
  assert(engine.process(p,s,data,nullptr,0,heavier::Velocity{},sink));
  pos+=n;
 }
 assert(engine.activePitch()>=0);
 return events;
}
static void check(int bars) {
 Settings settings;Pattern p;
 assert(p.setBars(bars));
 Step first;first.on=true;first.articulation=heavier::code(heavier::Articulation::PalmMute);
 Step middle=first;middle.degree=1;
 Step last=first;last.degree=2;last.locked=true;
 last.articulation=heavier::code(heavier::Articulation::OpenPowerchord);
 assert(p.setStep(0,first));
 assert(p.setStep(bars*8,middle));
 assert(p.setStep(bars*16-1,last));
 product::Snapshot source;source.pattern=p;source.musical=settings;source.seed=0x9ac6125au;
 product::StateBytes bytes{};
 assert(product::encode(source,bytes));
 product::Snapshot restored;
 assert(product::decode(bytes.data(),bytes.size(),restored));
 assert(restored.pattern.bars==bars&&restored.pattern.steps[bars*16-1].locked);
 assert(restored.pattern.steps[bars*16-1].degree==2&&restored.seed==source.seed);
 const auto result=render(restored.pattern,restored.musical);
 const std::int64_t cycle=static_cast<std::int64_t>(bars)*96000;
 std::vector<std::int64_t> expected;
 for(int c=0;c<3;++c){
  expected.push_back(c*cycle);
  expected.push_back(c*cycle+cycle/2);
  expected.push_back((c+1)*cycle-6000);
 }
 expected.push_back(3*cycle);
 assert(result.size()==expected.size()*2-1);
 for(std::size_t i=0;i<expected.size();++i){
  const auto& on=result[i*2];
  assert(on.e.noteOn&&on.sample==expected[i]);
  assert(on.e.velocity==(i%3==2?115:30));
  if(i>0){
   const auto& off=result[i*2-1];
   assert(!off.e.noteOn&&off.sample==expected[i]);
   assert(off.e.pitch==result[(i-1)*2].e.pitch);
  }
 }
 // A STOP at the last step releases the held note, but never attacks.
 Pattern stop=p;
 stop.steps[bars*16-1].locked=false;
 stop.steps[bars*16-1].stop=true;
 bool sawStop=false;
 for(const auto& e:render(stop,settings)){
  if(e.sample==cycle-6000){
   assert(!e.e.noteOn);
   sawStop=true;
  }
 }
 assert(sawStop);
 // Generate, fill, and clear may never overwrite a LOCK in the last bar.
 Pattern generated=p;
 heavier::generate(generated,settings,0x1234);
 assert(generated.steps[bars*16-1].locked&&generated.steps[bars*16-1].degree==2);
 auto count=[&](const Pattern& pattern) {
  int n=0;
  for(int i=0;i<pattern.activeSteps();++i)
   if(pattern.steps[i].on&&!pattern.steps[i].stop)++n;
  return n;
 };
 const int attacks=count(generated);
 applyFill(generated,settings,100,0x5678);
 assert(count(generated)==attacks&&generated.steps[bars*16-1].degree==2);
 generated.clear();
 assert(!generated.steps[0].on&&!generated.steps[bars*8].on);
 assert(generated.steps[bars*16-1].on&&generated.steps[bars*16-1].locked);
}
int main(){check(4);check(8);}
