#pragma once
#include "RiffEngine.hpp"
#include <array>
#include <vector>
#include <algorithm>
namespace a125::rifferator {
struct HarmonyMap {
  // 128 means follow global root. 0..127 are explicit MIDI roots.
  std::array<int,16> roots{};
  HarmonyMap() noexcept {roots.fill(128);}
};
inline std::vector<MidiNote> makeChordCandidates() {
  std::vector<MidiNote> notes;
  notes.reserve(48);
  for(int bar=0;bar<16;++bar) {
    const int64_t start=static_cast<int64_t>(bar)*3840;
    for(int interval : {0,3,7})notes.push_back({start,start+3840,40+interval,90,start+3840});
  }
  return notes;
}
inline int chordOutputPitch(int basePitch,int bar,const HarmonyMap& harmony,int globalRoot) noexcept {
  if(bar<0||bar>=16)return std::clamp(basePitch,0,127);
  const int root=harmony.roots[static_cast<std::size_t>(bar)]==128?
    globalRoot:harmony.roots[static_cast<std::size_t>(bar)];
  return std::clamp(basePitch+root-40,0,127);
}
} // namespace a125::rifferator
