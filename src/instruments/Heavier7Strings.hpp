#pragma once
#include "../core/Pattern.hpp"
namespace a125::riff::heavier {
// Heavier7Strings velocity-muting profile; ONE KEY FIFTH, VEL.MUTING, AUTO are
// configured in the external instrument, not by transmitting extra voices/CC.
inline int velocity(Articulation a,const Settings& s) noexcept {
  switch(a){
    case Articulation::Noise:return 20;
    case Articulation::PalmMute:return s.pmVelocity;
    case Articulation::OpenPowerchord:return 115;
    case Articulation::PinchHarmonic:return 127;
    case Articulation::Stop:return 0;
  }
  return 0;
}

inline std::vector<Event> compile(const Pattern& p,const Settings& s){
  return compileOneCycle(p,s,velocity);
}
inline std::vector<Event> compileLoop(const Pattern& p,const Settings& s){
  return compileLoopCycle(p,s,velocity);
}
inline void generate(Pattern& pattern,const Settings& settings,std::uint32_t seed) noexcept {
  Random rng{seed};
  for(int i=0;i<pattern.activeSteps();++i){
    auto& step=pattern.steps[static_cast<std::size_t>(i)];
    if(step.locked)continue;
    const int beat=i%4,roll=rng.range(100);
    step.on=(beat==0||roll<38);
    step.articulation=beat==0&&roll<25?Articulation::OpenPowerchord:Articulation::PalmMute;
    step.degree=0;
  }
  applyFill(pattern,settings,settings.fillIntensity,seed^0x6d2b79f5U);
}
inline void variate(Pattern& pattern,const Settings& settings,std::uint32_t seed) noexcept {
  Random rng{seed};
  for(int i=0;i<pattern.activeSteps();++i){
    auto& s=pattern.steps[static_cast<std::size_t>(i)];
    if(s.locked||!s.on||s.articulation==Articulation::Stop)continue;
    if(rng.range(100)<15)s.articulation=Articulation::PalmMute;
  }
  applyFill(pattern,settings.fillIntensity,seed^0x9e3779b9U);
}

} // namespace a125::riff::heavier
