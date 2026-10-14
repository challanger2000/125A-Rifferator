#include "vst3/ParameterChanges.hpp"
#include <cassert>
#include <limits>
#include <vector>
#ifdef NDEBUG
#error Assertions must remain enabled
#endif
using namespace a125::riff;
using namespace a125::riff::vst;
struct Point {std::uint32_t id;double value;bool readable=true;};
static void run(product::Snapshot& state,const std::vector<Point>& points){
 applyParameterChanges(state,static_cast<int>(points.size()),
  [&](int i,std::uint32_t& id,double& value){
   id=points[i].id;value=points[i].value;return points[i].readable;
  });
 assert(state.valid());
}
static std::vector<Point> paste(){
 std::vector<Point> points{{PasteTransaction,n(1,0,15)},{On+16,1.}};
 for(int i=0;i<kStepsPerBar;++i)points.push_back({Lock+16+i,1.});
 return points;
}
int main(){
 // Real paste overrides the destination's old locks and restores copied locks.
 product::Snapshot s;s.pattern.bars=2;s.pattern.steps[16].locked=true;
 run(s,paste());assert(s.pattern.steps[16].on&&s.pattern.steps[16].locked);
 // Simultaneous automation outside the pasted bar must still apply LOCK first.
 for(bool initiallyLocked:{false,true}){
  s={};s.pattern.bars=2;s.pattern.steps[0].locked=initiallyLocked;
  auto points=paste();points.push_back({On,1.});
  points.push_back({Lock,initiallyLocked?0.:1.});
  run(s,points);
  assert(s.pattern.steps[0].on==initiallyLocked);
  assert(s.pattern.steps[0].locked!=initiallyLocked);
 }
 // Unreadable, missing and malformed LOCK payloads must never bypass locks.
 for(int failure=0;failure<5;++failure){
  s={};s.pattern.bars=2;s.pattern.steps[16].locked=true;
  auto points=paste();
  if(failure==0)points.pop_back();
  if(failure==1)points.back().readable=false;
  if(failure==2)points.back().value=std::numeric_limits<double>::quiet_NaN();
  if(failure==3)points.back().value=-0.1;
  if(failure==4)points.back().value=1.1;
  run(s,points);assert(!s.pattern.steps[16].on&&s.pattern.steps[16].locked);
 }
 // Invalid marker values cannot be clamped into a legitimate transaction.
 for(double marker:{-0.1,1.1,std::numeric_limits<double>::infinity()}){
  s={};s.pattern.bars=2;s.pattern.steps[16].locked=true;
  auto points=paste();points[0].value=marker;
  run(s,points);assert(!s.pattern.steps[16].on&&s.pattern.steps[16].locked);
 }
 s={};s.pattern.steps[0].locked=true;
 run(s,{{PasteTransaction,0.},{On,1.}});
 assert(!s.pattern.steps[0].on&&s.pattern.steps[0].locked);
}
