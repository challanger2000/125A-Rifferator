#include "../src/RifferatorProcessor.hpp"
#include "public.sdk/source/common/memorystream.h"
#ifdef NDEBUG
#error Regression asserts disabled in CI
#endif
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdint>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
int main(){
  Processor original;
  MemoryStream full;
  assert(original.getState(&full)==Steinberg::kResultOk);
  assert(full.getSize()>100);
  assert(full.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
  Processor restored;
  assert(restored.setState(&full)==Steinberg::kResultOk);
  MemoryStream again;
  assert(restored.getState(&again)==Steinberg::kResultOk);
  assert(again.getSize()==full.getSize());
  assert(std::memcmp(again.getData(),full.getData(),full.getSize())==0);
  // Truncated stream must fail; subsequent state must stay unchanged.
  MemoryStream truncated(full.getData(),full.getSize()-1);
  assert(restored.setState(&truncated)!=Steinberg::kResultOk);
  MemoryStream retained;
  assert(restored.getState(&retained)==Steinberg::kResultOk);
  assert(retained.getSize()==full.getSize());
  assert(std::memcmp(retained.getData(),full.getData(),full.getSize())==0);
  // Legacy V6 projects end after BassMode and EBMStyle, i.e. 19 ints
  // before current V8's chords enabled/mode, 16 roots and quality.
  assert(full.getSize()>=76);
  const auto legacySize=static_cast<std::size_t>(full.getSize())-19u*sizeof(std::int32_t);
  auto* data=reinterpret_cast<const unsigned char*>(full.getData());
  std::vector<unsigned char> legacy(data,data+legacySize);
  legacy[4]=6;legacy[5]=legacy[6]=legacy[7]=0;
  MemoryStream old(legacy.data(),static_cast<Steinberg::TSize>(legacy.size()));
  Processor migrated;
  assert(migrated.setState(&old)==Steinberg::kResultOk);
  MemoryStream upgraded;
  assert(migrated.getState(&upgraded)==Steinberg::kResultOk);
  assert(upgraded.getSize()==full.getSize());
// V8 non-default full snapshot: validate every serialized feature class,
// including MIDI register controls that a default-only roundtrip misses.
  {
    std::vector<unsigned char> changed(data,data+full.getSize());
    auto write32=[&](std::size_t index,std::int32_t value) {
      assert((index+1)*4<=changed.size());
      std::memcpy(changed.data()+index*4,&value,4);
    };
    std::int32_t count=0;
    std::memcpy(&count,changed.data()+8,4);
    assert(count>=1 && count<=32);
    write32(3,52);   // Root E3
    write32(4,117);  // Bass velocity
    write32(5,-24);  // Bass offset
    write32(6,58);   // Guitar gate
    write32(7,72);   // Bass gate
    write32(9,0);    // Bass disabled
    write32(10,75);  // Fill intensity
    const std::size_t afterSteps=11u+static_cast<std::size_t>(count)*4u;
    write32(afterSteps,0);     // Drums disabled
    write32(afterSteps+1,0);   // First kick disabled
    const std::size_t guitarOn=afterSteps+49;
    write32(guitarOn+0,0);
    const std::size_t art=guitarOn+16;
    write32(art+0,4);
    const std::size_t deg=art+16;
    write32(deg+0,7);
    const std::size_t locks=deg+16;
    write32(locks+0,1);
    const std::size_t bassMode=locks+16;
    write32(bassMode,2);       // HYBRID
    write32(bassMode+1,3);     // EBM style
    write32(bassMode+2,1);     // Chords enabled
    write32(bassMode+3,1);     // Chords root-only
    write32(bassMode+4,45);    // First bar explicit root
    write32(bassMode+20,4);    // Sus4 chord quality
    MemoryStream custom(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    Processor customProcessor;
    assert(customProcessor.setState(&custom)==Steinberg::kResultOk);
    MemoryStream customSaved;
    assert(customProcessor.getState(&customSaved)==Steinberg::kResultOk);
    assert(customSaved.getSize()==static_cast<Steinberg::TSize>(changed.size()));
    assert(std::memcmp(customSaved.getData(),changed.data(),changed.size())==0);
    // Invalid later field: do not publish any partially applied state.
    write32(bassMode+20,99);
    MemoryStream corrupt(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    assert(customProcessor.setState(&corrupt)!=Steinberg::kResultOk);
    MemoryStream afterReject;
    assert(customProcessor.getState(&afterReject)==Steinberg::kResultOk);
    assert(afterReject.getSize()==customSaved.getSize());
    assert(std::memcmp(afterReject.getData(),customSaved.getData(),customSaved.getSize())==0);
  }

// Legacy V1 snapshot with ONE step must restore safely and survive subsequent
// VST3 parameter processing for the highest addressed guitar step (index 15).
  {
    std::vector<unsigned char> small(data,data+full.getSize());
    auto w=[&](std::size_t word,std::int32_t value) {
      assert((word+1)*4<=small.size());
      std::memcpy(small.data()+word*4,&value,4);
    };
    std::int32_t originalCount=0;
    std::memcpy(&originalCount,small.data()+8,4);
    assert(originalCount==16);
    // V1 ends directly after the serialized step array.
    small.resize((11u+4u)*sizeof(std::int32_t));
    w(1,1);w(2,1);
    MemoryStream oneStep(small.data(),static_cast<Steinberg::TSize>(small.size()));
    Processor shortPattern;
    assert(shortPattern.setState(&oneStep)==Steinberg::kResultOk);
    MemoryStream saved;
    assert(shortPattern.getState(&saved)==Steinberg::kResultOk);
    std::int32_t savedCount=0;
    std::memcpy(&savedCount,static_cast<const char*>(saved.getData())+8,4);
    assert(savedCount==1);
  }

}
