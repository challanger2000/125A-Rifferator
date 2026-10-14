#include "core/Pattern.hpp"
#include "core/BarClipboard.hpp"
#include "instruments/Heavier7Strings.hpp"
#include "core/StandardMidiFile.hpp"
#ifdef NDEBUG
#error CORE_TEST_ASSERT_DISABLED: assert-based QA cannot run with NDEBUG enabled.
#endif
#include <cassert>
#include <vector>
using namespace a125::riff;
using namespace a125::riff::heavier;
static Pattern sample(){
  Pattern p;assert(p.setBars(2));
  Step pm;pm.on=true;pm.articulation=code(Articulation::PalmMute);
  Step open=pm;open.articulation=code(Articulation::OpenPowerchord);
  Step stop=pm;stop.stop=true;
  assert(p.setStep(0,pm));assert(p.setStep(4,open));
  assert(p.setStep(10,stop));assert(p.setStep(16,pm));
  return p;
}
int main(){
  // One-bar copy is a frozen snapshot, not an alias into the source pattern.
  for(int bars : {2,4,8}){
   Pattern p;assert(p.setBars(bars));
   BarClipboard clip;assert(!clip.available()&&!clip.paste(p,1));
   Step special;special.on=true;special.locked=true;special.articulation=2;special.degree=5;
   p.steps[1]=special;
   Step stop;stop.on=true;stop.stop=true;stop.locked=true;
   p.steps[15]=stop;
   assert(clip.copy(p,0)&&clip.available());
   p.steps[1]=Step{}; // source changes after COPY must not affect clipboard
   p.steps[16+1].locked=true;
   assert(clip.paste(p,1));
   assert(p.steps[17].on&&p.steps[17].locked&&
          p.steps[17].articulation==2&&p.steps[17].degree==5);
   assert(p.steps[31].stop&&p.steps[31].locked);
   assert(!p.steps[1].on);
   assert(!clip.paste(p,bars)&&!clip.copy(p,-1));
  }

  Settings s;
  assert(s.valid()&&pitchInScale(40,s));
  assert(pitchForDegree(0,s)==40&&pitchForDegree(1,s)==42);
  assert(pitchForDegree(2,s)==43&&pitchForDegree(-1,s)==38);
  assert(!pitchInScale(41,s));
  for(int bars:{1,2,4,8}){
    Pattern p;assert(p.setBars(bars));
    assert(p.activeSteps()==bars*16&&p.lengthTicks()==bars*3840);
  }
  Pattern p;assert(!p.setBars(3)&&!p.setStep(16,Step{}));
  p=sample();
  const auto notes=compile(p,s);
  assert(notes.size()==6);
  assert(notes[0].tick==0&&notes[0].noteOn&&notes[0].velocity==30);
  assert(notes[1].tick==960&&!notes[1].noteOn);
  assert(notes[2].tick==960&&notes[2].noteOn&&notes[2].velocity==115);
  assert(notes[3].tick==2400&&!notes[3].noteOn);
  assert(notes[4].tick==3840&&notes[4].noteOn);
  assert(notes[5].tick==7680&&!notes[5].noteOn);
  std::vector<Event> found;
  visitAbsoluteEvents(notes,p.lengthTicks(),p.lengthTicks()+950,p.lengthTicks()+970,
                      [&](Event e){found.push_back(e);});
  assert(found.size()==2);
  assert(found[0].tick==p.lengthTicks()+960&&!found[0].noteOn);
  assert(found[1].tick==p.lengthTicks()+960&&found[1].noteOn);
  assert(p.setLock(4,true));p.clear();
  assert(p.steps[4].locked&&p.steps[4].on&&!p.steps[0].on&&!p.steps[16].on);
  assert(!p.setStep(4,Step{}));
  p=sample();p.setLock(4,true);applyFill(p,s,100,123);
  const auto filled=compile(p,s);
  int hits=0;for(const auto& e:filled)if(e.noteOn)++hits;
  assert(hits==3&&p.steps[4].degree==0);
  assert(p.steps[0].degree!=0&&p.steps[16].degree!=0);
  p=sample();applyFill(p,s,0,123);
  assert(p.steps[0].degree==0&&p.steps[16].degree==0);
  Pattern a,b;a.setBars(8);b.setBars(8);
  generate(a,s,555);generate(b,s,555);
  for(int i=0;i<128;++i){
    assert(a.steps[i].on==b.steps[i].on);
    assert(a.steps[i].degree==b.steps[i].degree);
    assert(a.steps[i].articulation==b.steps[i].articulation);
  }
  const auto midi=exportMidi(sample(),s,Velocity{});
  assert(midi.size()>30);
  assert(midi[0]=='M'&&midi[1]=='T'&&midi[2]=='h'&&midi[3]=='d');
  assert(midi[10]==0&&midi[11]==1&&midi[12]==3&&midi[13]==0xc0);
  assert(midi[midi.size()-3]==0xff&&midi[midi.size()-2]==0x2f);
  // Live loop must not cut off a long note at the barline if early steps
  // are empty. It ends at the first active step of the NEXT cycle.
  Pattern looping;looping.setBars(1);
  Step note;note.on=true;note.articulation=code(Articulation::PalmMute);
  assert(looping.setStep(5,note));
  note.articulation=code(Articulation::OpenPowerchord);
  assert(looping.setStep(15,note));
  const auto clipped=compile(looping,s);
  const auto cyclic=compileLoop(looping,s);
  assert(clipped.back().tick==looping.lengthTicks());
  assert(cyclic.back().tick==looping.lengthTicks()+5*kTicksPerStep);
  std::vector<Event> join;
  visitAbsoluteEvents(cyclic,looping.lengthTicks(),looping.lengthTicks()+5*kTicksPerStep,
      looping.lengthTicks()+5*kTicksPerStep+1,[&](Event e){join.push_back(e);});
  assert(join.size()==2&&!join[0].noteOn&&join[1].noteOn);

  // The universal pattern core must allow an arbitrary instrument's technique
  // code; velocity mapping is supplied by policy and knows nothing about H7.
  struct BassPolicy {
    int operator()(std::uint8_t code,const Settings&) const noexcept {
      return code==77?66:80;
    }
  };
  Pattern generic;
  Step ebm;
  ebm.on=true;ebm.articulation=77;
  assert(generic.setStep(0,ebm));
  const auto genericEvents=compileOneCycle(generic,s,BassPolicy{});
  assert(genericEvents.size()==2&&genericEvents[0].velocity==66);
  // H7 PM velocity remains an independent instrument setting, bounded 30..83.
  const auto strongPm=compile(sample(),s,Velocity{83});
  assert(strongPm[0].velocity==83);
  assert(!Velocity{29}.valid()&&!Velocity{84}.valid());

  // Zero FILL must not make VARIATE a no-op. For each pattern size and seed,
  // actual MIDI note velocity or timing must change; no fake GUI-only state.
  for(int bars : {1,2,4,8})for(std::uint32_t seed=1;seed<=100;++seed){
    Pattern original;assert(original.setBars(bars));
    Step pm;pm.on=true;pm.articulation=code(Articulation::PalmMute);
    for(int i=0;i<original.activeSteps();i+=4)
      assert(original.setStep(i,pm));
    Pattern candidate=original;
    Settings noFill=s;noFill.fillIntensity=0;
    const auto before=compileOneCycle(original,noFill,Velocity{});
    assert(variate(candidate,noFill,seed));
    const auto after=compileOneCycle(candidate,noFill,Velocity{});
    bool audible=false;
    assert(after.size()==before.size());
    for(std::size_t i=0;i<before.size();++i)
      if(before[i].tick!=after[i].tick ||
         before[i].pitch!=after[i].pitch ||
         before[i].velocity!=after[i].velocity)audible=true;
    assert(audible);
    Pattern sameSeed=original;
    assert(variate(sameSeed,noFill,seed));
    for(int i=0;i<original.activeSteps();++i){
      assert(sameSeed.steps[i].on==candidate.steps[i].on);
      assert(sameSeed.steps[i].degree==candidate.steps[i].degree);
      assert(sameSeed.steps[i].articulation==candidate.steps[i].articulation);
    }
  }
  {
    Pattern allLocked;Step pm;pm.on=true;
    for(int i=0;i<allLocked.activeSteps();++i)
      assert(allLocked.setStep(i,pm)&&allLocked.setLock(i,true));
    assert(!variate(allLocked,s,42));
    for(int i=0;i<allLocked.activeSteps();++i)
      assert(allLocked.steps[i].on&&allLocked.steps[i].locked);
    Pattern empty;assert(variate(empty,s,42));
    const auto generated=compileOneCycle(empty,s,Velocity{});
    assert(!generated.empty()&&generated.front().noteOn);
  }

}
