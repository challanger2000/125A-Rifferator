#pragma once
#include "Pattern.hpp"
#include <array>
namespace a125::riff {
class BarClipboard {
public:
 bool copy(const Pattern& p,int bar) noexcept {
  if(!p.valid()||bar<0||bar>=p.bars)return false;
  for(int i=0;i<kStepsPerBar;++i) copied_[i]=p.steps[bar*kStepsPerBar+i];
  has_=true;return true;
 }
 bool paste(Pattern& p,int bar) const noexcept {
  if(!has_||!p.valid()||bar<0||bar>=p.bars)return false;
  for(int i=0;i<kStepsPerBar;++i)p.steps[bar*kStepsPerBar+i]=copied_[i];
  return true;
 }
 bool available() const noexcept {return has_;}
private:
 std::array<Step,kStepsPerBar> copied_{};
 bool has_{false};
};
} // namespace a125::riff
