#pragma once
#include "RiffRandom.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace a125::rifferator {

struct FillOptions {
  int everyBars{16};       // 0 disables; 4, 8, or 16
  int intensityPercent{0}; // 0 is exact bypass
  int stepsPerBar{16};
  int maxScaleDegree{5};
  std::uint32_t seed{0x125A2026u};
};

// Returns a single bar with an optional ending fill. Only edits the final
// bar of each cycle; never affects an earlier bar or a locked step.
// All pitch values remain integer degrees of the caller's chosen scale.
inline std::vector<Step> applyFill(const std::vector<Step>& base,
                                  const FillOptions& o, int barIndex) {
  auto result=base;
  if(result.empty() || o.everyBars<=0 || o.intensityPercent<=0 ||
     (barIndex+1)%o.everyBars!=0) return result;
  const int end=static_cast<int>(result.size());
  const int bar=std::clamp(o.stepsPerBar,1,end);
  const int start=end-bar;
  std::uint32_t rng=o.seed^static_cast<std::uint32_t>(barIndex+1)*0x9E3779B9u;
  // Modify only the latter half of the ending bar; earlier motif remains.
  const int fillStart=start+bar/2;
  for(int i=fillStart;i<end;++i) {
    auto& s=result[static_cast<std::size_t>(i)];
    if(s.locked) continue;
    if(drawPercent(rng)>=std::clamp(o.intensityPercent,0,100)) continue;
    // Fills remain predominantly playable muted/open guitar notes.
    s.articulation=drawPercent(rng)<50?Articulation::PalmMute:Articulation::Open;
    s.degree=1+static_cast<int>(nextRandom(rng)%
      static_cast<std::uint32_t>(std::clamp(o.maxScaleDegree,1,7)));
  }
  return result;
}
} // namespace a125::rifferator
