#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace a125::rifferator {
enum class Articulation : std::uint8_t { Off, Noise, PalmMute, Soft, Open, Harmonic };
struct Step {
  Articulation articulation{Articulation::Off};
  int degree{1};
  bool doubled{false};
  bool locked{false};
};
struct MidiNote {
  int64_t startTick{},endTick{};
  int note{},velocity{};
  int64_t boundaryTick{};
};
struct Config {
  int rootMidi{40}; // E2 with MIDI C4=60
  std::array<int,7> scale{0,2,3,5,7,8,10};
  int guitarGate{95};
  int ticksPerStep{240};
};
inline int degreeOffset(int degree,const std::array<int,7>& scale) noexcept {
  const int z=degree-1;
  int oct=z/7,idx=z%7;
  if(idx<0){idx+=7;--oct;}
  return oct*12+scale[static_cast<std::size_t>(idx)];
}
inline int guitarStepVelocity(int articulation) noexcept {
  // Default Heavier7Strings profile. PM is overridden by the editable range.
  constexpr std::array<int,6> values{0,20,50,95,115,127};
  return values[static_cast<std::size_t>(std::clamp(articulation,0,5))];
}
inline std::vector<MidiNote> render(const std::vector<Step>& steps,const Config& cfg) {
  std::vector<MidiNote> out;
  if(steps.empty()||cfg.ticksPerStep<=0)return out;
  for(std::size_t i=0;i<steps.size();++i) {
    const auto& s=steps[i];
    if(s.articulation==Articulation::Off)continue;
    const int64_t begin=static_cast<int64_t>(i)*cfg.ticksPerStep;
    const int count=s.doubled?2:1;
    const int pitch=std::clamp(cfg.rootMidi+degreeOffset(s.degree,cfg.scale),0,127);
    for(int hit=0;hit<count;++hit) {
      const int64_t t=begin+cfg.ticksPerStep*hit/count;
      const int64_t boundary=begin+cfg.ticksPerStep*(hit+1)/count;
      const int64_t off=std::max(t+1,t+(boundary-t)*cfg.guitarGate/100);
      out.push_back({t,off,pitch,guitarStepVelocity(static_cast<int>(s.articulation)),boundary});
    }
  }
  return out;
}
} // namespace a125::rifferator
