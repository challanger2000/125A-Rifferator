#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  std::vector<Step> steps(16,Step{Articulation::Off,1,false,false});
  steps[0]={Articulation::PalmMute,1,false,false};
  steps[4]={Articulation::Open,3,true,false};
  Config config;
  auto pattern=render(steps,config);
  BlockScheduler scheduler;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  // 16 1/16th steps => four quarters; no shortening due to trailing rest.
  const int64_t period=16*240;
  scheduler.process(pattern,0.0,1.0,48000,period,sink,true);
  int onG=0,onB=0;
  for(const auto& e:events) if(e.on) { if(e.bus==0)++onG;else ++onB; }
  assert(onG==1 && onB==1);
  events.clear();
  scheduler.process(pattern,1.0,2.0,48000,period,sink,true);
  for(const auto& e:events) assert(e.sampleOffset>=0 && e.sampleOffset<48000);
  events.clear();
  scheduler.process(pattern,2.0,3.0,48000,period,sink,false);
  for(const auto& e:events) assert(!e.on);
  events.clear();
  scheduler.process(pattern,4.0,5.0,48000,period,sink,true);
  assert(!events.empty());

  scheduler.reset();
  events.clear();
  scheduler.process(pattern,0.0,0.1,4800,period,sink,true);
  bool rejected=false;
  auto rejectOff=[&](const ScheduledMidiEvent& e) {
    if(!e.on) { rejected=true; return false; }
    events.push_back(e); return true;
  };
  scheduler.process(pattern,0.1,0.2,4800,period,rejectOff,false);
  assert(rejected);
  int recoveredOffs=0;
  auto recovery=[&](const ScheduledMidiEvent& e) {
    if(!e.on) ++recoveredOffs;
    return true;
  };
  assert(scheduler.flush(recovery,0));
  assert(recoveredOffs>=1);

  // Runtime gate and transpose must not modify precomputed MIDI vectors.
  events.clear();
  LiveControls live;
  live.guitarGate=50;
  live.bassGate=25;
  live.rootSemitones=2;
  live.bassVelocity=110;
  BlockScheduler liveScheduler;
  liveScheduler.process(pattern,0.0,0.25,12000,period,sink,true,live);
  assert(!events.empty());
  for(const auto& e:events) if(e.on) {
    assert(e.pitch==42 || e.pitch==30);
    if(e.bus==1) assert(e.velocity==110);
  }
  events.clear();
  live.bassEnabled=false;
  liveScheduler.reset();
  liveScheduler.process(pattern,0.0,0.25,12000,period,sink,true,live);
  for(const auto& e:events) assert(e.bus==0);
  // Minimum positive gate must terminate rather than leaving stuck notes.
  events.clear();
  BlockScheduler shortGate;
  LiveControls tiny;
  tiny.guitarGate=1; tiny.bassGate=1;
  shortGate.process(pattern,0.0,0.25,48000,period,sink,true,tiny);
  int shortOn=0,shortOff=0;
  for(const auto& e:events) {if(e.on)++shortOn;else ++shortOff;}
  assert(shortOn==2 && shortOff==2);
  // Extreme but finite PPQ positions must not overflow integer cycle indices.
  // Active notes are released and no new note-ons are emitted.
  BlockScheduler extreme;
  events.clear();
  extreme.process(pattern,0.,0.1,4800,period,sink,true);
  assert(!events.empty());
  events.clear();
  extreme.process(pattern,1.0e16,1.0e16+1.,4800,period,sink,true);
  assert(!events.empty());
  for(const auto& e:events)assert(!e.on);
  events.clear();
  extreme.process(pattern,1.0e16,1.0e16+1.,4800,period,sink,true);
  assert(events.empty());
  // Rejecting the protective NoteOff must keep the active note for retry.
  extreme.reset();
  events.clear();
  extreme.process(pattern,0.,0.1,4800,period,sink,true);
  events.clear();
  bool rejectedExtreme=false;
  auto rejectExtreme=[&](const ScheduledMidiEvent& e){
    if(!e.on){rejectedExtreme=true;return false;}
    events.push_back(e);return true;
  };
  extreme.process(pattern,1.0e16,1.0e16+1.,4800,period,rejectExtreme,true);
  assert(rejectedExtreme);
  int recoveredExtreme=0;
  auto acceptExtreme=[&](const ScheduledMidiEvent& e){if(!e.on)++recoveredExtreme;return true;};
  extreme.process(pattern,1.0e16,1.0e16+1.,4800,period,acceptExtreme,true);
  assert(recoveredExtreme>=1);
  scheduler.reset();
}
