#pragma once
#include "UiGeometry.hpp"
#include "../core/Pattern.hpp"

namespace a125::riff::vst::ui {

// Explicit GUI reset contract: commands/navigation remain operative with Ctrl.
inline bool supportsReset(Target target) noexcept {
 switch(target){
  case Target::Bars:case Target::Key:case Target::Scale:case Target::Root:
  case Target::Pm:case Target::Fill:case Target::Mode:case Target::Zoom:
  case Target::StepOn:case Target::Lock:case Target::Cell:
   return true;
  default:return false;
 }
}
inline bool resetStep(Pattern& pattern,int index) noexcept {
 if(index<0||index>=pattern.activeSteps())return false;
 auto& step=pattern.steps[static_cast<std::size_t>(index)];
 if(step.locked)return false;
 step=Step{}; // Neutral default: no note, no STOP, no articulation/degree.
 return true;
}
inline bool toggleStep(Pattern& pattern,int index,int row) noexcept {
 if(index<0||index>=pattern.activeSteps()||row<0||row>4)return false;
 auto& step=pattern.steps[static_cast<std::size_t>(index)];
 if(step.locked)return false;
 if(row==4){
  const bool already=step.on&&step.stop;
  step.on=!already;
  step.stop=!already;
  return true;
 }
 static constexpr std::uint8_t types[4]{3,2,0,1}; // H7 rows
 const bool already=step.on&&!step.stop&&step.articulation==types[row];
 step.stop=false;
 step.articulation=types[row];
 step.on=!already;
 return true;
}
} // namespace a125::riff::vst::ui
