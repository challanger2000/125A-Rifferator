#include "../src/core/RiffEngine.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  Config c;
  std::vector<Step> steps={
    {Articulation::PalmMute,1,false,false},
    {Articulation::PalmMute,1,true,false},
    {Articulation::Off,1,false,false},
    {Articulation::Open,3,false,false},
    {Articulation::Noise,1,false,false},
    {Articulation::Harmonic,1,false,false}
  };
  const auto out=render(steps,c);
  assert(out.guitar.size()==6); // 1+2+0+1+1+1
  assert(out.bass.size()==4);   // 1+2+0+1, no noise/harmonics
  assert(out.guitar[0].startTick==0);
  assert(out.guitar[1].startTick==240 && out.guitar[2].startTick==360);
  assert(out.guitar[0].velocity==50);
  assert(out.guitar[3].velocity==115);
  assert(out.guitar[3].note==43); // E + minor third = G
  assert(out.bass[3].note==31);
  assert(out.guitar[1].endTick<=out.guitar[2].startTick);
  for(const auto& n:out.guitar) assert(n.endTick>n.startTick);
  c.guitarGate=0; c.bassGate=0;
  const auto silent=render(steps,c);
  assert(silent.guitar.empty() && silent.bass.empty());
  assert(degreeOffset(1,c.scale)==0);
  assert(degreeOffset(8,c.scale)==12);
  assert(degreeOffset(0,c.scale)==-2);
}
