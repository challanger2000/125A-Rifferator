#include "vst3/ParameterChanges.hpp"
#include <cassert>
#include <limits>
#include <algorithm>
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

 // Host queue enumeration order cannot change simultaneous pitch automation.
 std::vector<Point> pitch{{Key,n(6,0,11)},{ScaleId,0.},{Root,n(41,0,127)}};
 do {
  s={};
  run(s,pitch);
  assert(s.musical.key==6&&s.musical.scale==Scale::Major&&
         s.musical.rootMidi==41);
 } while(std::next_permutation(pitch.begin(),pitch.end(),
   [](const Point& a,const Point& b){return a.id<b.id;}));

 // KEY before SCALE also gives one deterministic implicit-root result when
 // no explicit ROOT point exists in the block.
 for(bool reverse:{false,true}){
  s={};
  std::vector<Point> keyScale{{Key,n(6,0,11)},{ScaleId,0.}};
  if(reverse)std::reverse(keyScale.begin(),keyScale.end());
  run(s,keyScale);
  assert(s.musical.key==6&&s.musical.scale==Scale::Major&&
         s.musical.rootMidi==42);
 }

 // The host-restored marker, not an unpersisted Controller boolean, drives the
 // transaction toggle. The first paste after reload must always change value.
 double marker=0.;
 for(int bar:{0,0,3,3,7,7}){
  const double next=nextPasteMarker(bar,marker);
  assert(next!=marker);
  assert(q(next,15)%kMaxBars==bar);
  marker=next;
  // Simulate project reload: only the normalized VST3 value survives.
  const double afterReload=nextPasteMarker(bar,marker);
  assert(afterReload!=marker&&q(afterReload,15)%kMaxBars==bar);
 }
}
