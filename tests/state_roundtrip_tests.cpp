#include "product/RifferatorState.hpp"
#include "vst3/Parameters.hpp"
#ifdef NDEBUG
#error CORE_TEST_ASSERT_DISABLED: assert-based QA cannot run with NDEBUG enabled.
#endif
#include <cassert>
using namespace a125::riff;
using namespace a125::riff::product;
using namespace a125::riff::heavier;

// Independent checksum reconstruction after *deliberately* corrupting a field,
// to ensure semantic/version validators are exercised beyond checksum rejection.
static void refreshHash(StateBytes& b){
  std::uint32_t h=2166136261u;
  for(std::size_t i=0;i<b.size()-4;++i){h^=b[i];h*=16777619u;}
  for(int i=0;i<4;++i)b[b.size()-4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(h>>(8*i));
}
static bool same(const Snapshot& a,const Snapshot& b){
  StateBytes x{},y{};
  return encode(a,x)&&encode(b,y)&&x==y;
}
int main(){
  Snapshot initial;
  initial.pattern.setBars(8);
  initial.musical.key=4;
  initial.musical.scale=Scale::NaturalMinor;
  initial.musical.rootMidi=40;
  initial.musical.fillIntensity=75;
  initial.pmVelocity=83;
  initial.syncMode=SyncMode::MidiNote;
  initial.seed=0x12345678;
  Step pm;pm.on=true;pm.articulation=code(Articulation::PalmMute);
  pm.degree=2;pm.locked=true;
  assert(initial.pattern.setStep(5,pm));
  Step stop;stop.on=true;stop.stop=true;stop.locked=true;
  assert(initial.pattern.setStep(100,stop));
  assert(initial.valid());
  StateBytes bytes{};
  assert(encode(initial,bytes));
  Snapshot restored;
  assert(decode(bytes.data(),bytes.size(),restored));
  assert(same(restored,initial));
  assert(restored.pattern.steps[5].degree==2);
  assert(restored.pattern.steps[100].stop&&restored.pattern.steps[100].locked);

  // Broken buffer may never partially modify a known-good project snapshot.
  const Snapshot untouched=restored;
  auto bad=bytes;
  bad[36]^=0x20;
  assert(!decode(bad.data(),bad.size(),restored));
  assert(same(restored,untouched));
  assert(!decode(bytes.data(),bytes.size()-1,restored));
  assert(!decode(nullptr,bytes.size(),restored));
  assert(same(restored,untouched));

  bad=bytes;bad[4]=2;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // unsupported version
  assert(same(restored,untouched));
  bad=bytes;bad[8]=16;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // disallowed bars
  bad=bytes;bad[13]=29;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // out of H7 PM range
  bad=bytes;bad[10]=7;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // invalid scale choice
  bad=bytes;bad[11]=41;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // F2 not E natural minor
  bad=bytes;bad[14]=3;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // mode
  bad=bytes;bad[20+3*5+2]=255;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // invalid degree
  bad=bytes;bad[20+3*5+1]=200;refreshHash(bad);
  assert(!decode(bad.data(),bad.size(),restored)); // non-H7 style
  assert(same(restored,untouched));

  Snapshot invalid=initial;
  invalid.pattern.bars=3;
  StateBytes sentinel{};
  sentinel.fill(0xa5);
  assert(!encode(invalid,sentinel));
  for(const auto b:sentinel)assert(b==0xa5);
  invalid=initial;invalid.pmVelocity=0;
  assert(!encode(invalid,sentinel));
  invalid=initial;invalid.musical.rootMidi=41;
  assert(!encode(invalid,sentinel));
  // Host VST3 normalized parameter map: all 128 cells / five controls.
  // Test both directions, LOCK protection, and upper-bound indices.
  a125::riff::product::Snapshot mapped;
  assert(mapped.pattern.setBars(8));
  for(std::uint32_t cell=0;cell<128;++cell){
    using namespace a125::riff::vst;
    assert(apply(mapped,On+cell,1.)&&read(mapped,On+cell)==1.);
    assert(apply(mapped,Type+cell,1.)&&read(mapped,Type+cell)==1.);
    assert(apply(mapped,Degree+cell,0.5)&&read(mapped,Degree+cell)==0.5);
    assert(apply(mapped,Stop+cell,1.)&&read(mapped,Stop+cell)==1.);
    assert(apply(mapped,Lock+cell,1.)&&read(mapped,Lock+cell)==1.);
    assert(!apply(mapped,On+cell,0.));
    assert(mapped.pattern.steps[cell].on);
    assert(apply(mapped,Lock+cell,0.)&&read(mapped,Lock+cell)==0.);
    assert(apply(mapped,Stop+cell,0.)&&read(mapped,Stop+cell)==0.);
  }
  assert(!a125::riff::vst::apply(mapped,a125::riff::vst::On+128,1.));
  assert(a125::riff::vst::apply(mapped,a125::riff::vst::Pm,1.));
  assert(mapped.pmVelocity==83);
  assert(mapped.valid());

}
