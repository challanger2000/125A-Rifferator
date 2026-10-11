#include "../src/core/BlockScheduler.hpp"
#include "../src/core/DrumCompanion.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main() {
  std::vector<Step> steps(16);
  steps[0].articulation=Articulation::PalmMute;
  const auto pair=render(steps,Config{});
  const int64_t period=16*240;
  BlockScheduler scheduler;
  std::vector<ScheduledMidiEvent> emitted;
  auto sink=[&](const ScheduledMidiEvent& e) {emitted.push_back(e);return true;};
  scheduler.process(pair,0.,0.1,4800,period,sink,true);
  assert(emitted.size()==2 && emitted[0].on && emitted[1].on);
  emitted.clear();
  scheduler.process(pair,0.1,0.2,4800,period,sink,false);
  assert(emitted.size()==2);
  for(const auto& e:emitted) assert(!e.on && e.sampleOffset==0);
  emitted.clear();
  scheduler.process(pair,0.,0.1,4800,period,sink,true);
  assert(emitted.size()==2);
  emitted.clear();
  scheduler.process(pair,16.,16.1,4800,period,sink,true);
  int on=0,off=0;
  for(const auto& e:emitted) {if(e.on)++on;else ++off;}
  assert(on==2 && off==2); // seek: panic old notes, then restart
  emitted.clear();
  scheduler.process(pair,16.1,16.2,4800,period,sink,false);
  assert(emitted.size()==2);
  scheduler.reset();
  emitted.clear();
  scheduler.process(pair,0.,0.1,4800,period,sink,true);
  emitted.clear();
  bool reject=true;
  auto rejectSink=[&](const ScheduledMidiEvent& e) {
    if(!e.on && reject)return false;
    emitted.push_back(e);
    return true;
  };
  scheduler.process(pair,0.1,0.2,4800,period,rejectSink,false);
  assert(emitted.empty());
  reject=false;
  assert(scheduler.flush(rejectSink,0));
  assert(emitted.size()==2);
  for(const auto& e:emitted)assert(!e.on);
  // Host rejects a NoteOff on STOP, then transport restarts. The first
  // recovery callback must deliver only releases and never new NoteOns.
  {
    BlockScheduler retry;
    std::vector<ScheduledMidiEvent> observed;
    auto accept=[&](const ScheduledMidiEvent& e){observed.push_back(e);return true;};
    retry.process(pair,0.,0.1,4800,period,accept,true);
    assert(!observed.empty());
    observed.clear();
    int failures=0;
    auto rejectOff=[&](const ScheduledMidiEvent& e){
      if(!e.on){++failures;return false;}
      observed.push_back(e);return true;
    };
    retry.process(pair,0.1,0.2,4800,period,rejectOff,false);
    assert(failures>0);
    observed.clear();
    retry.process(pair,4.,4.1,4800,period,accept,true);
    assert(!observed.empty());
    for(const auto& e:observed)assert(!e.on);
    observed.clear();
    retry.process(pair,4.,4.1,4800,period,accept,true);
    int noteOns=0;
    for(const auto& e:observed)if(e.on)++noteOns;
    assert(noteOns==2);
  }
  // Three-bus integration: drum pitches must not transpose and must panic.
  BlockScheduler threeBus;
  auto drumNotes=renderDrums(DrumPattern::standard(),16);
  threeBus.setDrums(&drumNotes);
  LiveControls shifted;
  shifted.rootSemitones=7;
  emitted.clear();
  threeBus.process(pair,0.,0.1,4800,16*16*240,sink,true,shifted);
  bool foundKick=false,foundHat=false;
  for(const auto& e:emitted) if(e.bus==2 && e.on) {
    assert(e.pitch==36 || e.pitch==38 || e.pitch==42);
    if(e.pitch==36)foundKick=true;
    if(e.pitch==42)foundHat=true;
  }
  assert(foundKick && foundHat);
  emitted.clear();
  threeBus.process(pair,0.1,0.2,4800,16*16*240,sink,false,shifted);
  for(const auto& e:emitted)assert(!e.on);

}
