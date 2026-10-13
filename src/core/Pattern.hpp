#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
namespace a125::riff {
constexpr int kPpq=960, kTicksPerStep=240, kStepsPerBar=16, kMaxBars=8, kMaxSteps=128;
enum class Scale : std::uint8_t { Major,NaturalMinor };
struct Step {
  bool on{false},locked{false},stop{false};
  // Opaque 0..255 technique ID owned solely by the selected instrument.
  std::uint8_t articulation{0};
  int degree{0};
};
struct Pattern {
  int bars{1};
  std::array<Step,kMaxSteps> steps{};
  constexpr int activeSteps() const noexcept {return bars*kStepsPerBar;}
  constexpr int lengthTicks() const noexcept {return activeSteps()*kTicksPerStep;}
  static constexpr bool validBars(int count) noexcept {
    return count==1||count==2||count==4||count==8;
  }
  bool valid() const noexcept {
    if(!validBars(bars))return false;
    for(int i=0;i<activeSteps();++i) {
      const auto& step=steps[static_cast<std::size_t>(i)];
      if(step.degree < -14||step.degree>14)return false;
    }
    return true;
  }
  bool setBars(int count) noexcept {
    if(!validBars(count))return false;
    bars=count;return true;
  }
  bool setStep(int index,const Step& value) noexcept {
    if(index<0||index>=activeSteps()||steps[static_cast<std::size_t>(index)].locked||
      value.degree < -14||value.degree>14)return false;
    steps[static_cast<std::size_t>(index)]=value;return true;
  }
  bool setLock(int index,bool lock) noexcept {
    if(index<0||index>=activeSteps())return false;
    steps[static_cast<std::size_t>(index)].locked=lock;return true;
  }
  void clear() noexcept {
    for(int i=0;i<activeSteps();++i)
      if(!steps[static_cast<std::size_t>(i)].locked)
        steps[static_cast<std::size_t>(i)]=Step{};
  }
};
struct Settings {
  int key{4}; // E
  Scale scale{Scale::NaturalMinor};
  int rootMidi{40}; // E2, middle C=60
  int fillIntensity{0};
  bool valid() const noexcept {
    return key>=0&&key<12&&(scale==Scale::Major||scale==Scale::NaturalMinor)&&
      rootMidi>=0&&rootMidi<=127&&
      fillIntensity>=0&&fillIntensity<=100;
  }
};
inline const std::array<int,7>& intervals(Scale scale) noexcept {
  static constexpr std::array<int,7> major{{0,2,4,5,7,9,11}};
  static constexpr std::array<int,7> minor{{0,2,3,5,7,8,10}};
  return scale==Scale::Major?major:minor;
}
inline bool pitchInScale(int midi,const Settings& s) noexcept {
  if(!s.valid()||midi<0||midi>127)return false;
  const int chroma=(midi-s.key+120)%12;
  const auto& notes=intervals(s.scale);
  return std::find(notes.begin(),notes.end(),chroma)!=notes.end();
}
inline int pitchForDegree(int degree,const Settings& s) noexcept {
  if(degree < -14||degree>14||!pitchInScale(s.rootMidi,s))return -1;
  int result=s.rootMidi;
  if(degree>0){
    for(int i=0;i<degree;++i){
      do{++result;}while(result<=127&&!pitchInScale(result,s));
      if(result>127)return -1;
    }
  }else{
    for(int i=0;i>degree;--i){
      do{--result;}while(result>=0&&!pitchInScale(result,s));
      if(result<0)return -1;
    }
  }
  return result;
}
struct Event {
  std::int64_t tick{0};
  int pitch{0},velocity{0};
  bool noteOn{false};
};
inline bool eventLess(const Event& a,const Event& b) noexcept {
  return a.tick==b.tick?a.noteOn<b.noteOn:a.tick<b.tick;
}
// Offline/controller-thread compilation, not called on the real-time callback.
// OFF steps extend the note. STOP releases; any next note releases first.
// One-cycle export ends at boundary so imported MIDI cannot leave hanging notes.
template<class VelocityPolicy>
inline std::vector<Event> compileOneCycle(const Pattern& pattern,const Settings& s,VelocityPolicy&& velocityPolicy){
  std::vector<Event> output;
  if(!pattern.valid()||!s.valid()||!pitchInScale(s.rootMidi,s))return output;
  output.reserve(static_cast<std::size_t>(pattern.activeSteps())*2);
  int heldPitch=-1;
  for(int i=0;i<=pattern.activeSteps();++i){
    const bool boundary=i==pattern.activeSteps();
    const Step step=boundary?Step{}:pattern.steps[static_cast<std::size_t>(i)];
    if(!boundary&&!step.on)continue;
    const auto tick=static_cast<std::int64_t>(i)*kTicksPerStep;
    if(heldPitch>=0)output.push_back({tick,heldPitch,0,false});
    heldPitch=-1;
    if(boundary||step.stop)continue;
    const int p=pitchForDegree(step.degree,s);
    if(p<0){output.clear();return output;}
    output.push_back({tick,p,velocityPolicy(step.articulation,s),true});
    heldPitch=p;
  }
  std::stable_sort(output.begin(),output.end(),eventLess);
  return output;
}
// Live loop is cyclic: last sounding note remains held over empty leading
// steps of the next cycle, until its first active Note or STOP releases it.
template<class VelocityPolicy>
inline std::vector<Event> compileLoopCycle(const Pattern& pattern,const Settings& s,
                                           VelocityPolicy&& velocityPolicy){
  auto result=compileOneCycle(pattern,s,velocityPolicy);
  if(result.empty()||result.back().noteOn||result.back().tick!=pattern.lengthTicks())return result;
  for(int i=0;i<pattern.activeSteps();++i){
    if(pattern.steps[static_cast<std::size_t>(i)].on){
      result.back().tick+=static_cast<std::int64_t>(i)*kTicksPerStep;
      break;
    }
  }
  return result;
}

// Absolute host PPQ visitor; host adapter will restrict invocation to bounded blocks.
// Does not reset musical phase when processing seeks, loops or varying blocks.
template<class Emit>
void visitAbsoluteEvents(const std::vector<Event>& cycle,std::int64_t period,
                         std::int64_t begin,std::int64_t end,Emit&& emit){
  if(period<=0||end<=begin)return;
  auto floorDiv=[](std::int64_t n,std::int64_t d)noexcept{
    const auto q=n/d;return q-(n%d<0?1:0);
  };
  const auto first=floorDiv(begin,period)-1;
  const auto last=floorDiv(end-1,period);
  for(auto id=first;id<=last;++id)
    for(const auto& e:cycle){
      const auto absolute=id*period+e.tick;
      if(absolute>=begin&&absolute<end)emit(Event{absolute,e.pitch,e.velocity,e.noteOn});
    }
}
struct Random {
  std::uint32_t state;
  explicit Random(std::uint32_t seed) noexcept:state(seed?seed:1){}
  std::uint32_t next() noexcept {
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
  }
  int range(int upper) noexcept {
    return static_cast<int>(next()%static_cast<std::uint32_t>(upper));
  }
};
// FILL NEVER adds attacks. It only changes in-scale degree on existing ON cells.
inline void applyFill(Pattern& pattern,const Settings& settings,int intensity,std::uint32_t seed) noexcept {
  if(!pattern.valid()||!settings.valid())return;
  Random rng{seed};intensity=std::clamp(intensity,0,100);
  for(int i=0;i<pattern.activeSteps();++i){
    auto& s=pattern.steps[static_cast<std::size_t>(i)];
    const bool change=rng.range(100)<intensity;
    const int movement=rng.range(2)?1:-1;
    if(!s.locked&&s.on&&!s.stop&&change){
      const int candidate=std::clamp(s.degree+movement,-14,14);
      if(pitchForDegree(candidate,settings)>=0)s.degree=candidate;
    }
  }
}
} // namespace a125::riff
