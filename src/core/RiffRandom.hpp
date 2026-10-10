#pragma once
#include "RiffEngine.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace a125::rifferator {
struct RandomOptions {
  std::uint32_t seed{0x125A2026u};
  int densityPercent{82};
  int pmPercent{68};
  int noisePercent{5};
  int harmonicPercent{2};
  bool randomRhythm{true};
  bool randomArticulation{true};
  bool randomPitch{false};
  int maxScaleDegree{5};
};
inline std::uint32_t nextRandom(std::uint32_t& x) noexcept {
  if(!x) x=0x125A2026u;
  x^=x<<13; x^=x>>17; x^=x<<5;
  return x;
}
inline int drawPercent(std::uint32_t& seed) noexcept {
  return static_cast<int>(nextRandom(seed)%100u);
}
inline std::vector<Step> randomize(const std::vector<Step>& input, const RandomOptions& o) {
  std::vector<Step> output=input;
  std::uint32_t seed=o.seed;
  for(auto& step:output) {
    if(step.locked) continue;
    if(o.randomRhythm && drawPercent(seed)>=std::clamp(o.densityPercent,0,100)) {
      step.articulation=Articulation::Off;
      continue;
    }
    if(o.randomArticulation) {
      const int pn=std::clamp(o.noisePercent,0,100);
      const int ph=std::clamp(o.harmonicPercent,0,100-pn);
      const int pm=std::clamp(o.pmPercent,0,100-pn-ph);
      const int r=drawPercent(seed);
      step.articulation = r<pn?Articulation::Noise:
                          r<pn+ph?Articulation::Harmonic:
                          r<pn+ph+pm?Articulation::PalmMute:Articulation::Open;
    } else if(o.randomRhythm && step.articulation==Articulation::Off) {
      step.articulation=Articulation::PalmMute;
    }
    if(o.randomPitch && step.articulation!=Articulation::Off) {
      step.degree=1+static_cast<int>(nextRandom(seed)%static_cast<std::uint32_t>(std::clamp(o.maxScaleDegree,1,7)));
    }
  }
  return output;
}
} // namespace a125::rifferator
