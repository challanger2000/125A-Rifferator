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
}
