#pragma once
#include "../core/Pattern.hpp"
namespace a125::riff::heavier {
// All named performance techniques live in the H7 adapter, NOT the generic core.
enum class Articulation : std::uint8_t {
  PalmMute=0, Noise=1, OpenPowerchord=2, PinchHarmonic=3
};
constexpr std::uint8_t code(Articulation art) noexcept {
  return static_cast<std::uint8_t>(art);
}
// H7 requires ONE KEY FIFTH, VEL. MUTING, and AUTO enabled in the instrument.
// Its velocity ranges are deliberately NOT part of generic pattern Settings.
struct Velocity {
  int pmVelocity{30};
  bool valid() const noexcept { return pmVelocity>=30&&pmVelocity<=83; }
  int operator()(std::uint8_t technique,const Settings&) const noexcept {
    if(!valid())return 0;
    switch(static_cast<Articulation>(technique)){
    case Articulation::Noise:return 20;
    case Articulation::PalmMute:return pmVelocity;
    case Articulation::OpenPowerchord:return 115;
    case Articulation::PinchHarmonic:return 127;
  }
    return 0;
  }
};

inline std::vector<Event> compile(const Pattern& p,const Settings& s,Velocity v=Velocity{}){
  return v.valid()?compileOneCycle(p,s,v):std::vector<Event>{};
}
inline std::vector<Event> compileLoop(const Pattern& p,const Settings& s,Velocity v=Velocity{}){
  return v.valid()?compileLoopCycle(p,s,v):std::vector<Event>{};
}
inline void generate(Pattern& pattern,const Settings& settings,std::uint32_t seed) noexcept {
  Random rng{seed};
  for(int i=0;i<pattern.activeSteps();++i){
    auto& step=pattern.steps[static_cast<std::size_t>(i)];
    if(step.locked)continue;
    const int beat=i%4,roll=rng.range(100);
    step.on=(beat==0||roll<38);
    step.stop=false;
    step.articulation=code(beat==0&&roll<25?Articulation::OpenPowerchord:Articulation::PalmMute);
    step.degree=0;
  }
  applyFill(pattern,settings,settings.fillIntensity,seed^0x6d2b79f5U);
}
inline void variate(Pattern& pattern,const Settings& settings,std::uint32_t seed) noexcept {
  Random rng{seed};
  for(int i=0;i<pattern.activeSteps();++i){
    auto& s=pattern.steps[static_cast<std::size_t>(i)];
    if(s.locked||!s.on||s.stop)continue;
    if(rng.range(100)<15)s.articulation=code(Articulation::PalmMute);
  }
  applyFill(pattern,settings,settings.fillIntensity,seed^0x9e3779b9U);
}

} // namespace a125::riff::heavier
