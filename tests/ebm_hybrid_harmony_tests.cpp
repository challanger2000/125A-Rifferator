#include "../src/core/EbmBassArrangement.hpp"
#include <cassert>
#include <array>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
  BassPlan p;
  p.mode=BassMode::Hybrid;
  p.sequence.style=::rifferator::ebm::Style::Classic;
  p.sequence.root=52;
  p.harmonicRoots.fill(52);
  p.harmonicRoots[4]=55;
  p.harmonicRoots[8]=0; // MIDI zero is a valid, explicitly selected root
  p.harmonicRoots[12]=255; // unspecified: fallback to sequence.root
  Arrangement a;
  prepare(a,p);
  assert(a.periodTicks==61440);
  bool first=false,fourth=false,zero=false,fallback=false;
  for(const auto& n:a.midi.bass) {
    if(n.startTick==0) {assert(n.note==40);first=true;}
    if(n.startTick==4*3840) {assert(n.note==43);fourth=true;}
    if(n.startTick==8*3840) {assert(n.note==0);zero=true;}
    if(n.startTick==12*3840) {assert(n.note==40);fallback=true;}
  }
  assert(first&&fourth&&zero&&fallback);
  BassPlan e=p;
  e.mode=BassMode::Ebm;
  Arrangement b;
  prepare(b,e);
  for(const auto& n:b.midi.bass)
    if(n.startTick==4*3840)assert(n.note==40);
}
