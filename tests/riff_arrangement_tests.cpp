#include "../src/core/RiffArrangement.hpp"
#include <cassert>
using namespace a125::rifferator;
int main() {
 std::vector<Step> base(16,Step{Articulation::Off,1,false,false});
 base[0].articulation=Articulation::PalmMute;
 base[15].articulation=Articulation::Open;
 FillOptions fill;fill.intensityPercent=0;
 Config cfg;const auto a=arrange(base,cfg,fill,16);
 assert(a.periodTicks==61440);
 assert(a.guitar.size()==32);
 assert(a.guitar[0].startTick==0);
 assert(a.guitar[2].startTick==3840);
 fill.intensityPercent=100;
 const auto b=arrange(base,cfg,fill,16);
 assert(b.periodTicks==a.periodTicks);
 assert(b.guitar.size()>=a.guitar.size());
}
