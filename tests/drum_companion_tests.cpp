#include "../src/core/DrumCompanion.hpp"
#include "../src/core/BlockScheduler.hpp"
#include <cassert>
using namespace a125::rifferator;
int main() {
  auto p=DrumPattern::standard(true);
  auto notes=renderDrums(p,16);
  assert(notes.size()==(4+2+16)*16);
  int kick=0,snare=0,hat=0;
  for(const auto& n:notes) {
    assert(n.startTick<n.endTick && n.endTick<=61440);
    if(n.note==36)++kick;
    if(n.note==38)++snare;
    if(n.note==42)++hat;
  }
  assert(kick==64 && snare==32 && hat==256);
  auto eighth=renderDrums(DrumPattern::standard(false),1);
  assert(eighth.size()==14);
  p.kick[0]=false;p.hat[0]=false;
  assert(renderDrums(p,1).size()==20);
  p.enabled=false;assert(renderDrums(p,16).empty());
  DrumPattern candidates;
  candidates.kick.fill(true);candidates.snare.fill(true);candidates.hat.fill(true);
  auto all=renderDrums(candidates,16);
  BlockScheduler scheduler;scheduler.setDrums(&all);
  Pair noGuitar;
  LiveControls live;
  int hits=0;
  scheduler.process(noGuitar,0.,0.24,9600,16*16*240,
    [&](const ScheduledMidiEvent& e) {if(e.on && e.bus==2)++hits;return true;},
    true,live);
  assert(hits>0);
  scheduler.reset();hits=0;
  live.drumsEnabled=false;
  scheduler.process(noGuitar,0.,0.24,9600,16*16*240,
    [&](const ScheduledMidiEvent& e) {if(e.on && e.bus==2)++hits;return true;},
    true,live);
  assert(hits==0);
  scheduler.reset();hits=0;
  live.drumsEnabled=true;
  for(auto& row:live.drumSteps)row.fill(false);
  scheduler.process(noGuitar,0.,0.24,9600,16*16*240,
    [&](const ScheduledMidiEvent& e) {if(e.on && e.bus==2)++hits;return true;},
    true,live);
  assert(hits==0);
}
