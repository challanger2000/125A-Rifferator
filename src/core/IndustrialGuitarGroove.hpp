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
  // Three deterministic, musically distinct one-bar Industrial/NDH
  // patterns. Downbeat anchors stay intact; PM/rest placement varies.
  const unsigned variant=variation>=85?2u:variation>=50?1u:0u;
  if(variant==1) {
    g.enabled[3]=true; g.articulation[3]=2; g.velocity[3]=87;
    g.enabled[7]=true; g.articulation[7]=2; g.velocity[7]=96;
    g.enabled[10]=false; g.articulation[10]=0; g.velocity[10]=0;
    g.enabled[15]=true; g.articulation[15]=2; g.velocity[15]=108;
  } else if(variant==2) {
    g.enabled[1]=false; g.articulation[1]=0; g.velocity[1]=0;
    g.enabled[3]=true; g.articulation[3]=2; g.velocity[3]=110;
    g.enabled[5]=false; g.articulation[5]=0; g.velocity[5]=0;
    g.enabled[7]=true; g.articulation[7]=4; g.velocity[7]=118;
    g.enabled[9]=false; g.articulation[9]=0; g.velocity[9]=0;
    g.enabled[11]=true; g.articulation[11]=2; g.velocity[11]=95;
    g.enabled[14]=true; g.articulation[14]=4; g.velocity[14]=127;
    g.enabled[15]=true; g.articulation[15]=2; g.velocity[15]=105;
  }
  return g;
}
inline int guitarStepVelocity(int step,int articulation) noexcept {
  if(articulation<=0)return 0;
  constexpr std::array<int,16> velocities{127,68,104,90,127,72,108,90,127,65,100,90,127,80,112,90};
  return velocities[static_cast<std::size_t>(std::clamp(step,0,15))];
}
} // namespace a125::rifferator
