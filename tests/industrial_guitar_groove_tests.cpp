#include "../src/core/IndustrialGuitarGroove.hpp"
#include "../src/core/BlockScheduler.hpp"
#include <cassert>
#include <vector>
using namespace a125::rifferator;
int main(){
  const auto riff=makeIndustrialGuitarGroove();
  assert(riff.enabled[0]&&riff.articulation[0]==4);
  assert(riff.enabled[1]&&riff.articulation[1]==2);
  assert(!riff.enabled[3]);
  assert(makeIndustrialGuitarGroove(85).articulation[14]==4);
  for(int i=0;i<16;++i){
    assert(riff.degrees[i]==1);
    if(i%4==3)assert(!riff.enabled[i]);
    if(i%4==0){assert(riff.articulation[i]==4);assert(riff.velocity[i]==127);}
    if(i%4==1||i%4==2)assert(riff.articulation[i]==2);
  }
  // Audible instrument articulations must follow the selected row, not the
  // position of the step. Check each Heavier7Strings velocity band.
  constexpr int expected[6]{0,20,50,95,115,127};
  for(int step=0;step<16;++step)for(int art=0;art<6;++art)
    assert(guitarStepVelocity(step,art)==expected[art]);
  // Full MIDI-path regression: each UI articulation selection must change
  // the actual NoteOn velocity emitted by the scheduler, not just a helper.
  for(int art=0;art<6;++art) {
    Pair one;
    one.guitar.push_back({0,200,40,110,240});
    LiveControls selected;
    selected.guitarSteps.fill(false);
    selected.guitarSteps[0]=true;
    selected.guitarArticulation[0]=art;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(one,0.,0.25,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,selected);
    int ons=0;
    for(const auto& e:actual)if(e.bus==0 && e.on) {
      ++ons;assert(e.velocity==expected[art]);
    }
    assert(ons==(art==0?0:1));
  }
  // UP/DOUBLE: two successive strikes at half-step spacing, not a chord.
  {
    Pair one;one.guitar.push_back({0,200,40,110,240});
    LiveControls selected;
    selected.guitarSteps.fill(false);selected.guitarSteps[0]=true;
    selected.guitarArticulation[0]=2;selected.guitarDouble[0]=true;
    selected.pmMin=30;selected.pmMax=40;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(one,0.,0.25,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,selected);
    std::vector<int> ons,offs,velocities;
    for(const auto& e:actual)if(e.bus==0 && e.pitch==40) {
      if(e.on){assert(e.velocity>=30 && e.velocity<=40);velocities.push_back(e.velocity);}
      (e.on?ons:offs).push_back(e.sampleOffset);
    }
    assert(ons.size()==2 && offs.size()==2);
    assert(velocities.size()==2 && velocities[0]==40 && velocities[1]==37);
    assert(ons[0]==0 && ons[1]>=479 && ons[1]<=480);
    assert(ons[0]<offs[0] && offs[0]<=ons[1] && ons[1]<offs[1]);
  }
  // A sustained OPEN note must still retrigger at half a 16th step.
  {
    Pair longNote;longNote.guitar.push_back({0,800,40,115,960});
    LiveControls live;
    live.guitarSteps.fill(false);live.guitarSteps[0]=true;
    live.guitarArticulation[0]=4;live.guitarDouble[0]=true;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(longNote,0.,1.,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,live);
    std::vector<int> on,off;
    for(const auto& e:actual)if(e.bus==0)
      (e.on?on:off).push_back(e.sampleOffset);
    assert(on.size()==2 && off.size()==2);
    assert(on[0]==0 && on[1]==120);
    assert(off[0]<=120 && off[1]<=240);
  }
  // FIFTH is already sampled by the guitar library: one pitch per attack.
  {
    Pair one;one.guitar.push_back({0,200,40,110,240});
    for(bool doubled:{false,true}) {
      LiveControls selected;
      selected.guitarSteps.fill(false);selected.guitarSteps[0]=true;
      selected.guitarArticulation[0]=4;selected.guitarDouble[0]=doubled;
      BlockScheduler engine;
      std::vector<ScheduledMidiEvent> actual;
      engine.process(one,0.,0.25,960,61440,
        [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,selected);
      int ons=0,offs=0;
      for(const auto& e:actual)if(e.bus==0) {
        assert(e.pitch==40); // no synthetic fifth at pitch 47
        (e.on?ons:offs)++;
      }
      assert(ons==(doubled?2:1) && offs==(doubled?2:1));
    }
  }
  // An out-of-period STOP must not wrap into an eight-step pattern.
  {
    Pair held;held.guitar.push_back({0,1800,40,115,1920});
    LiveControls controls;
    controls.guitarSteps.fill(false);
    controls.guitarSteps[0]=true;controls.guitarArticulation[0]=4;
    controls.guitarSteps[12]=true;controls.guitarArticulation[12]=0;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(held,0.,2.,1920,1920,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,controls);
    for(const auto& e:actual)
      assert(!(e.bus==0 && !e.on && e.sampleOffset==960));
  }
  // Retrigger of one pitch must not kill another sustained guitar pitch.
  {
    Pair overlap;
    overlap.guitar.push_back({0,950,40,115,960});
    overlap.guitar.push_back({0,950,45,115,960});
    overlap.guitar.push_back({240,450,40,115,480});
    LiveControls c;c.guitarSteps.fill(false);
    c.guitarSteps[0]=true;c.guitarArticulation[0]=4;
    c.guitarSteps[1]=true;c.guitarArticulation[1]=4;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(overlap,0.,1.,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,c);
    int otherOn=0,otherOff=0;
    for(const auto& e:actual)if(e.bus==0 && e.pitch==45) {
      if(e.on)++otherOn;
      else {++otherOff;assert(e.sampleOffset>=940);}
    }
    assert(otherOn==1 && otherOff==1);
  }
  // STOP releases a long OPEN at step 2; an empty step does not.
  {
    Pair held;held.guitar.push_back({0,950,40,115,960});
    held.bass.push_back({0,950,28,100,960});
    LiveControls controls;
    controls.guitarSteps.fill(false);
    controls.guitarSteps[0]=true;controls.guitarArticulation[0]=4;
    controls.guitarSteps[1]=true;controls.guitarArticulation[1]=0;
    BlockScheduler engine;
    std::vector<ScheduledMidiEvent> actual;
    engine.process(held,0.,0.5,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,controls);
    bool stopped=false;
    for(const auto& e:actual)if(e.bus==0 && !e.on && e.pitch==40 && e.sampleOffset==480)stopped=true;
    assert(stopped);
    // A guitar STOP must never terminate the held bass on bus 1.
    for(const auto& e:actual)
      assert(!(e.bus==1 && !e.on && e.sampleOffset==480));
    controls.guitarSteps[1]=false;
    engine.reset();actual.clear();
    engine.process(held,0.,0.5,960,61440,
      [&](ScheduledMidiEvent e){actual.push_back(e);return true;},true,controls);
    for(const auto& e:actual)assert(!(e.bus==0 && !e.on && e.sampleOffset==480));
  }
  assert(!makeIndustrialGuitarGroove(49).enabled[15]);
  assert(makeIndustrialGuitarGroove(50).enabled[15]);
  Pair pair;
  pair.guitar.push_back({0,200,40,110,240});
  BlockScheduler scheduler;
  LiveControls live;
  live.guitarSteps.fill(false);
  live.guitarSteps[0]=true;
  live.guitarArticulation[0]=4;
  live.guitarDegree[0]=1;
  std::vector<ScheduledMidiEvent> events;
  auto sink=[&](const ScheduledMidiEvent& e){events.push_back(e);return true;};
  scheduler.process(pair,0.,0.25,960,61440,sink,true,live);
  int notesOn=0,notesOff=0;
  // FIFTH is produced inside the sampled guitar library, not as extra MIDI.
  for(const auto& e:events) {
    assert(e.bus==0 && e.pitch==40);
    if(e.on)++notesOn;else ++notesOff;
  }
  assert(notesOn==1 && notesOff==1);
  // Trigger already received its scheduled NoteOff; flush must be empty.
  events.clear();
  assert(scheduler.flush(sink,0));
  assert(events.empty());
}
