#include "core/MidiRuntime.hpp"
#include "instruments/Heavier7Strings.hpp"
#ifdef NDEBUG
#error CORE_TEST_ASSERT_DISABLED: assert-based QA cannot run with NDEBUG enabled.
#endif
#include <cassert>
#include <vector>
using namespace a125::riff;
using namespace a125::riff::heavier;

static Pattern riff(){
  Pattern p;
  Step pm;pm.on=true;pm.articulation=code(Articulation::PalmMute);
  Step op=pm;op.articulation=code(Articulation::OpenPowerchord);
  Step stop=pm;stop.stop=true;
  assert(p.setStep(0,pm)&&p.setStep(4,op)&&p.setStep(10,stop));
  return p;
}
int main(){
  Settings s;
  auto p=riff();
  MidiRuntime runtime;
  std::vector<MidiOut> emitted;
  auto collect=[&](MidiOut e){emitted.push_back(e);return true;};
  Block a{24000,48000.,120.,0.,true,true,true};
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==1&&emitted[0].noteOn&&emitted[0].sampleOffset==0);
  assert(emitted[0].pitch==40&&emitted[0].velocity==30);
  emitted.clear();
  a.projectPpq=1.;
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==2&&!emitted[0].noteOn&&emitted[1].noteOn);
  assert(emitted[0].sampleOffset==0&&emitted[1].sampleOffset==0);
  assert(emitted[1].velocity==115);
  emitted.clear();
  a.playing=false;
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==1&&!emitted[0].noteOn);
  assert(runtime.activePitch()==-1);

  // A seek while playing must flush before starting at the new phase.
  emitted.clear();a.playing=true;a.projectPpq=0.;
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  emitted.clear();a.projectPpq=2.5; // STOP at step 10
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==1&&!emitted[0].noteOn);
  assert(runtime.activePitch()==-1);

  // Sustained notes across a loop boundary: do not fabricate a release at BAR 1.
  runtime.reset();p=Pattern{};
  Step note;note.on=true;note.articulation=code(Articulation::PalmMute);
  assert(p.setStep(5,note));
  note.articulation=code(Articulation::OpenPowerchord);
  assert(p.setStep(15,note));
  emitted.clear();a=Block{40000,48000.,120.,3.75,true,true,true};
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==3);
  assert(emitted[0].noteOn&&emitted[0].sampleOffset==0);
  assert(!emitted[1].noteOn&&emitted[1].sampleOffset==36000);
  assert(emitted[2].noteOn&&emitted[2].sampleOffset==36000);
  assert(emitted[0].pitch==emitted[1].pitch);

  // MIDI NoteOn starts from step zero at the exact input sample offset.
  // Matching NoteOff ends the riff, even with a sounding note.
  runtime.reset();runtime.setMode(SyncMode::MidiNote);p=Pattern{};
  Step triggerStep;triggerStep.on=true;triggerStep.articulation=code(Articulation::PalmMute);
  assert(p.setStep(0,triggerStep)&&p.setStep(2,triggerStep));
  emitted.clear();
  TriggerEvent inputs[]{{100,48,true},{12500,48,false}};
  a=Block{13000,48000.,120.,0.,false,false,true};
  assert(runtime.process(p,s,a,inputs,2,Velocity{},collect));
  assert(emitted.size()==4);
  assert(emitted[0].noteOn&&emitted[0].sampleOffset==100);
  assert(!emitted[1].noteOn&&emitted[1].sampleOffset==12100);
  assert(emitted[2].noteOn&&emitted[2].sampleOffset==12100);
  assert(!emitted[3].noteOn&&emitted[3].sampleOffset==12500);
  assert(!runtime.triggerHeld()&&runtime.activePitch()==-1);
  emitted.clear();
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.empty());

  // Unmatched NoteOff must not terminate a running riff.
  runtime.reset();
  TriggerEvent pulse[]{{20,40,true},{50,41,false}};
  emitted.clear();
  a.numSamples=100;
  assert(runtime.process(p,s,a,pulse,2,Velocity{},collect));
  assert(runtime.triggerHeld()&&runtime.activePitch()==40);
  emitted.clear();
  TriggerEvent off[]{{3,40,false}};
  assert(runtime.process(p,s,a,off,1,Velocity{},collect));
  assert(!runtime.triggerHeld()&&runtime.activePitch()==-1);
  assert(emitted.size()==1&&!emitted[0].noteOn&&emitted[0].sampleOffset==3);

  // Output rejecting the NoteOff cannot be followed by a new NoteOn.
  runtime.reset();runtime.setMode(SyncMode::Transport);p=riff();
  emitted.clear();a=Block{24000,48000.,120.,0.,true,true,true};
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  bool rejectFirstOff=true;
  emitted.clear();a.projectPpq=1.;
  auto backpressure=[&](MidiOut e){
    if(!e.noteOn&&rejectFirstOff){rejectFirstOff=false;return false;}
    emitted.push_back(e);return true;
  };
  assert(!runtime.process(p,s,a,nullptr,0,Velocity{},backpressure));
  assert(emitted.empty()&&runtime.activePitch()==40);
  // Next callback retries the missing release at sample zero.
  emitted.clear();a.projectPpq=2.;
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(!emitted.empty()&&!emitted.front().noteOn);
  assert(runtime.activePitch()==-1);

  // No independent DOUBLE, GALLOP or fixed-duration note generation.
  // Each new note/STOP must release the previous one before a new attack.
  runtime.reset();p=riff();
  emitted.clear();a=Block{48000,48000.,120.,0.,true,true,true};
  assert(runtime.process(p,s,a,nullptr,0,Velocity{},collect));
  assert(emitted.size()==3);
  assert(emitted[0].noteOn&&emitted[0].sampleOffset==0);
  assert(!emitted[1].noteOn&&emitted[1].sampleOffset==24000);
  assert(emitted[2].noteOn&&emitted[2].sampleOffset==24000);
}
