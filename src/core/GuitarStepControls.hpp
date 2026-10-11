#pragma once
#include "RiffEngine.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace a125::rifferator {
// Controller-side discrete step model. Never mutate the offline arrangement
// from the VST3 audio callback. A future non-RT synchronization path is required.
struct GuitarStepControls {
  static constexpr int kSteps=16;
  std::array<Step,kSteps> steps{};
  static GuitarStepControls initial() noexcept {
    GuitarStepControls out;
    for(int i=0;i<kSteps;++i) {
      out.steps[i].articulation=(i%2==0)?Articulation::PalmMute:Articulation::Off;
      out.steps[i].degree=1;
    }
    return out;
  }
  bool setArticulation(int index,Articulation articulation) noexcept {
    if(index<0||index>=kSteps||static_cast<int>(articulation)<0||
       static_cast<int>(articulation)>5)return false;
    auto& step=steps[static_cast<std::size_t>(index)];
    if(step.locked)return false;
    const bool change=step.articulation!=articulation;
    step.articulation=articulation;
    return change;
  }
  bool setDegree(int index,int degree) noexcept {
    if(index<0||index>=kSteps||degree<-13||degree>15)return false;
    auto& step=steps[static_cast<std::size_t>(index)];
    if(step.locked)return false;
    const bool change=step.degree!=degree;
    step.degree=degree;
    return change;
  }
  bool setDoubled(int index,bool doubled) noexcept {
    if(index<0||index>=kSteps)return false;
    auto& step=steps[static_cast<std::size_t>(index)];
    if(step.locked)return false;
    const bool change=step.doubled!=doubled;
    step.doubled=doubled;
    return change;
  }
  void setLock(int index,bool locked) noexcept {
    if(index>=0&&index<kSteps)steps[static_cast<std::size_t>(index)].locked=locked;
  }
};
} // namespace a125::rifferator
