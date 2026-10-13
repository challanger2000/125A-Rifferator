#include "../src/core/RiffEngine.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
 Config c;
 std::vector<Step> steps{{Articulation::PalmMute,1,false,false},
  {Articulation::PalmMute,1,true,false},{Articulation::Off,1,false,false},
  {Articulation::Open,3,false,false}};
 const auto out=render(steps,c);
 assert(out.size()==4);
 assert(out[0].startTick==0);
 assert(out[1].startTick==240&&out[2].startTick==360);
 assert(out[3].note==43);
 assert(out[1].endTick<=out[2].startTick);
 for(const auto& n:out)assert(n.endTick>n.startTick);
 assert(degreeOffset(1,c.scale)==0);
 assert(degreeOffset(3,c.scale)==3);
 assert(degreeOffset(-6,c.scale)==-12);
}
