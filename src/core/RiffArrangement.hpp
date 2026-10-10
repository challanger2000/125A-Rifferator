#pragma once
#include "RiffFill.hpp"
#include <cstdint>
#include <vector>

namespace a125::rifferator {
// Prepared off the audio thread. The runtime scheduler repeats this complete
// phrase; it must use its full duration (bars * steps * ticksPerStep).
struct Arrangement {
  Pair midi{};
  std::int64_t periodTicks{};
};
inline Arrangement arrange(const std::vector<Step>& base, const Config& config,
                           const FillOptions& fills, int bars=16) {
  Arrangement result;
  if(base.empty() || config.ticksPerStep<=0 || bars<1 || bars>64)
    return result;
  const auto barTicks=static_cast<std::int64_t>(base.size())*config.ticksPerStep;
  result.periodTicks=barTicks*bars;
  for(int bar=0;bar<bars;++bar) {
    const auto steps=applyFill(base,fills,bar);
    auto notes=render(steps,config);
    const auto offset=barTicks*bar;
    for(auto& n:notes.guitar) {
      n.startTick+=offset;n.endTick+=offset;
      result.midi.guitar.push_back(n);
    }
    for(auto& n:notes.bass) {
      n.startTick+=offset;n.endTick+=offset;
      result.midi.bass.push_back(n);
    }
  }
  return result;
}
} // namespace a125::rifferator
