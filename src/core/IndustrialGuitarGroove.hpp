#pragma once
#include "RiffEngine.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
namespace a125::rifferator {
// 16-step industrial guitar generator. No melody: root notes, deliberate rests,
// palm mutes, and accented power chords. Locked cells are never modified.
struct IndustrialGuitarGroove {
  std::array<bool,16> enabled{};
  std::array<int,16> articulation{};
  std::array<int,16> degrees{};
  std::array<int,16> velocity{};
};
inline IndustrialGuitarGroove makeIndustrialGuitarGroove(unsigned variation=0) noexcept {
  IndustrialGuitarGroove g;
  constexpr std::array<int,16> accents{127,68,104,0,127,72,108,0,127,65,100,0,127,80,112,0};
  for(int step=0;step<16;++step) {
    const bool rest=accents[step]==0;
    g.enabled[step]=!rest;
    g.degrees[step]=1;
    g.articulation[step]=rest?0:(step%4==0?4:2); // open fifth, otherwise PM
    g.velocity[step]=accents[step];
  }
  // Deterministic sparse turnaround only at end of bar.
  if(variation>0) {
    const auto intensity=std::min(variation,100u);
    if(intensity>=50) {
      g.enabled[15]=true;g.articulation[15]=2;g.velocity[15]=100;
    }
    if(intensity>=85) {
      g.enabled[14]=true;g.articulation[14]=4;g.velocity[14]=127;
    }
  }
  return g;
}
inline int guitarStepVelocity(int step,int articulation) noexcept {
  if(articulation<=0)return 0;
  constexpr std::array<int,16> velocities{127,68,104,90,127,72,108,90,127,65,100,90,127,80,112,90};
  return velocities[static_cast<std::size_t>(std::clamp(step,0,15))];
}
} // namespace a125::rifferator
