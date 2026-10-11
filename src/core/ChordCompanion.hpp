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
enum class ChordQuality : int { Minor=0, Major=1, Power=2, Sus2=3, Sus4=4 };
inline int chordInterval(int voice,ChordQuality quality) noexcept {
  if(voice==0)return 0;
  if(voice==1) {
    switch(quality) {
      case ChordQuality::Major:return 4;
      case ChordQuality::Power:return 7;
      case ChordQuality::Sus2:return 2;
      case ChordQuality::Sus4:return 5;
      default:return 3;
    }
  }
  return 7;
}
// Infer the stable harmonic anchor from the programmed guitar step grid.
// Short offbeat chromatic decorations carry less weight than downbeats.
// Fixed-size, allocation-free: safe for the audio callback.
inline int inferredGuitarRoot(const std::array<bool,16>& enabled,
                              const std::array<int,16>& articulation,
                              const std::array<int,16>& degrees,
                              const std::array<int,7>& scale,
                              int rootMidi) noexcept {
  std::array<int,128> scores{};
  for(int i=0;i<16;++i) {
    if(!enabled[i] || articulation[i]==0 || articulation[i]==1)continue;
    const int pitch=std::clamp(rootMidi+degreeOffset(degrees[i],scale),0,127);
    // A quarter-note downbeat is harmonically stronger than a passing 16th.
    const int weight=(i%4==0?5:i%2==0?2:1);
    scores[static_cast<std::size_t>(pitch)]+=weight;
  }
  int best=std::clamp(rootMidi,0,127),weight=0;
  for(int i=0;i<128;++i)if(scores[i]>weight) {weight=scores[i];best=i;}
  return best;
}
inline std::vector<MidiNote> makeChordCandidates() {
  std::vector<MidiNote> notes;
  notes.reserve(48);
  for(int bar=0;bar<16;++bar) {
    const int64_t start=static_cast<int64_t>(bar)*3840;
    for(int voice=0;voice<3;++voice)notes.push_back({start,start+3840,40+voice,90,start+3840});
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

