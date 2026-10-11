#pragma once
#include "RiffEngine.hpp"
#include <array>
#include <cstdint>
#include <vector>
namespace a125::rifferator {
struct DrumPattern {
  std::array<bool,16> kick{},snare{},hat{};
  bool enabled{true};
  static DrumPattern standard(bool sixteenth=true) {
    DrumPattern p;
    for(int i=0;i<16;++i) {
      p.kick[i]=(i%4)==0;
      p.snare[i]=(i==4||i==12);
      p.hat[i]=sixteenth||(i%2)==0;
    }
    return p;
  }
};
inline std::vector<MidiNote> renderDrums(const DrumPattern& p,int bars=16) {
  std::vector<MidiNote> notes;
  if(!p.enabled||bars<1||bars>64)return notes;
  for(int bar=0;bar<bars;++bar) for(int step=0;step<16;++step) {
    const auto onset=static_cast<std::int64_t>(bar*16+step)*240;
    auto emit=[&](bool active,int pitch,int velocity) {
      if(active)notes.push_back({onset,onset+60,pitch,velocity,onset+60});
    };
    emit(p.kick[step],36,110);
    emit(p.snare[step],38,106);
    emit(p.hat[step],42,85);
  }
  return notes;
}
}
