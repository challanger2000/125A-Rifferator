#pragma once
#include "RiffRandom.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace a125::rifferator {
enum class GenerationCommand {Generate,Variate};
inline std::vector<Step> generateGuitar(const std::vector<Step>& old,
  RandomOptions options,GenerationCommand command,int variationPercent=30) {
  if(command==GenerationCommand::Generate)return randomize(old,options);
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
