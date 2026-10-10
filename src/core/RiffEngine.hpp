#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace a125::rifferator {
enum class Articulation : std::uint8_t { Off, Noise, PalmMute, Soft, Open, Harmonic };
struct Step {
  Articulation articulation{Articulation::Off};
  int degree{1};         // 1-based scale degree, 1 means root
  bool doubled{false};
  bool locked{false};
};
struct MidiNote { int64_t startTick{}, endTick{}; int note{}, velocity{}; };
struct Pair { std::vector<MidiNote> guitar, bass; };
struct Config {
  int rootMidi{40};      // E2; adjusted to guitar library range by user
  std::array<int,7> scale{0,2,3,5,7,8,10}; // natural minor
  std::array<int,5> velocity{20,50,95,115,127};
  int bassVelocity{100};
  int bassOffset{-12};
  int guitarGate{95}, bassGate{95};
  int ticksPerStep{240}; // 960 PPQ at 1/16
  bool bassOn{true};
};
inline int clampMidi(int x) { return std::clamp(x,0,127); }
inline int degreeOffset(int degree, const std::array<int,7>& scale) {
  // Euclidean floor division for negative scale degrees.
  const int z=degree-1;
  int oct=z/7, idx=z%7;
  if(idx<0) { idx+=7; --oct; }
  return oct*12+scale[static_cast<std::size_t>(idx)];
}
inline int velocityFor(Articulation a, const Config& c) {
  switch(a) {
    case Articulation::Noise: return c.velocity[0];
    case Articulation::PalmMute: return c.velocity[1];
    case Articulation::Soft: return c.velocity[2];
    case Articulation::Open: return c.velocity[3];
    case Articulation::Harmonic: return c.velocity[4];
    default: return 0;
  }
}
inline bool bassFollows(Articulation a) {
  return a==Articulation::PalmMute || a==Articulation::Soft || a==Articulation::Open;
}
// Deterministic offline event representation. Live VST3 scheduling is a separate adapter.
inline Pair render(const std::vector<Step>& steps, const Config& cfg) {
  Pair out;
  if(steps.empty() || cfg.ticksPerStep<=0) return out;
  const auto ticks=static_cast<int64_t>(cfg.ticksPerStep);
  for(std::size_t i=0;i<steps.size();++i) {
    const auto& s=steps[i];
    if(s.articulation==Articulation::Off) continue;
    const int64_t begin=static_cast<int64_t>(i)*ticks;
    const int count=s.doubled?2:1;
    const int pitch=clampMidi(cfg.rootMidi+degreeOffset(s.degree,cfg.scale));
    for(int hit=0; hit<count; ++hit) {
      const int64_t t=begin+ticks*hit/count;
      const int64_t boundary=begin+ticks*(hit+1)/count;
      const auto gate=[&](int percent) {
        return t + (boundary-t)*std::clamp(percent,0,100)/100;
      };
      const int vel=velocityFor(s.articulation,cfg);
      if(cfg.guitarGate>0 && vel>0) {
        auto off=gate(cfg.guitarGate);
        if(off<=t) off=t+1;
        out.guitar.push_back({t,off,pitch,clampMidi(vel)});
      }
      if(cfg.bassOn && cfg.bassGate>0 && bassFollows(s.articulation)) {
        auto off=gate(cfg.bassGate);
        if(off<=t) off=t+1;
        out.bass.push_back({t,off,clampMidi(pitch+cfg.bassOffset),clampMidi(cfg.bassVelocity)});
      }
    }
  }
  return out;
}
} // namespace a125::rifferator
