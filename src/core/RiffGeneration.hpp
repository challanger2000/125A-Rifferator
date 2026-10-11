#pragma once
#include "RiffRandom.hpp"
#include "DrumCompanion.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace a125::rifferator {
// Pure, offline commands. Never call these from the audio process callback.
enum class GenerationCommand { Generate, Variate };
inline std::vector<Step> generateGuitar(const std::vector<Step>& original,
    RandomOptions options, GenerationCommand command, int variationPercent=30) {
  if(command==GenerationCommand::Generate) return randomize(original,options);
  // VARIATE is a conservative mutation, not a re-generation.
  const auto proposal=randomize(original,options);
  auto result=original;
  std::uint32_t seed=options.seed^0xA125F12Du;
  for(std::size_t i=0;i<result.size();++i) {
    if(result[i].locked) continue;
    if(drawPercent(seed)<std::clamp(variationPercent,0,100))
      result[i]=proposal[i];
  }
  return result;
}
struct DrumLocks {
  std::array<bool,16> kick{},snare{},hat{};
};
struct DrumGenerationOptions {
  std::uint32_t seed{0x125A2026u};
  int variationPercent{25};
  int kickDensity{25};
  int snareDensity{12};
  int hatDensity{50};
};
inline DrumPattern generateDrums(const DrumPattern& original,
    const DrumLocks& locks, DrumGenerationOptions options,
    GenerationCommand command) noexcept {
  DrumPattern result=original;
  std::uint32_t seed=options.seed;
  auto processRow=[&](std::array<bool,16>& row,
                      const std::array<bool,16>& locked,int density) {
    for(int i=0;i<16;++i) {
      // Consume a constant sequence regardless of lock configuration.
      const bool proposal=drawPercent(seed)<std::clamp(density,0,100);
      const bool mutate=drawPercent(seed)<std::clamp(options.variationPercent,0,100);
      if(locked[i]) continue;
      if(command==GenerationCommand::Generate || mutate)row[i]=proposal;
    }
  };
  processRow(result.kick,locks.kick,options.kickDensity);
  processRow(result.snare,locks.snare,options.snareDensity);
  processRow(result.hat,locks.hat,options.hatDensity);
  // Preserve global enable; the user controls it explicitly.
  return result;
}
} // namespace a125::rifferator
