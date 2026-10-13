#pragma once
#include "RiffRandom.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace a125::rifferator {
enum class GenerationCommand {Generate,Variate,Gallop};
// Classic gallop: eighth + two sixteenths, repeating on the four beats
// of a 16-step bar. It is THREE attacks on steps 0,2,3 / 4,6,7 / ...
// No artificial 32nd-note DOUBLE or transport-dependent randomization.
// User-locked steps are immutable (including locked STOP commands).
inline std::vector<Step> makeGallop(const std::vector<Step>& original) {
  auto result=original;
  for(std::size_t i=0;i<result.size();++i) {
    auto& step=result[i];
    if(step.locked)continue;
    const bool attack=(i%4)!=1;
    step.articulation=attack?Articulation::PalmMute:Articulation::Off;
    step.degree=1;
    step.doubled=false;
  }
  return result;
}
inline std::vector<Step> generateGuitar(const std::vector<Step>& old,
  RandomOptions options,GenerationCommand command,int variationPercent=30) {
  if(command==GenerationCommand::Generate)return randomize(old,options);
  if(command==GenerationCommand::Gallop)return makeGallop(old);
  const auto proposal=randomize(old,options);
  auto result=old;
  std::uint32_t seed=options.seed^0xA125F12Du;
  for(std::size_t i=0;i<result.size();++i) {
    if(!result[i].locked&&drawPercent(seed)<std::clamp(variationPercent,0,100))
      result[i]=proposal[i];
  }
  return result;
}
} // namespace a125::rifferator
