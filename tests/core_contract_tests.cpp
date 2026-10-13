#include "core/Pattern.hpp"
#include "core/StandardMidiFile.hpp"
#include <cassert>
#include <vector>
using namespace a125::riff;
static Pattern sample(){
  Pattern p;assert(p.setBars(2));
  Step pm;pm.on=true;pm.articulation=Articulation::PalmMute;
  Step open=pm;open.articulation=Articulation::OpenPowerchord;
  Step stop=pm;stop.articulation=Articulation::Stop;
  assert(p.setStep(0,pm));assert(p.setStep(4,open));
  assert(p.setStep(10,stop));assert(p.setStep(16,pm));
  return p;
}
int main(){
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
  const auto notes=compileOneCycle(p,s);
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
  p=sample();p.setLock(4,true);applyFill(p,100,123);
  const auto filled=compileOneCycle(p,s);
  int hits=0;for(const auto& e:filled)if(e.noteOn)++hits;
  assert(hits==3&&p.steps[4].degree==0);
  assert(p.steps[0].degree!=0&&p.steps[16].degree!=0);
  p=sample();applyFill(p,0,123);
  assert(p.steps[0].degree==0&&p.steps[16].degree==0);
  Pattern a,b;a.setBars(8);b.setBars(8);
  generate(a,s,555);generate(b,s,555);
  for(int i=0;i<128;++i){
    assert(a.steps[i].on==b.steps[i].on);
    assert(a.steps[i].degree==b.steps[i].degree);
    assert(a.steps[i].articulation==b.steps[i].articulation);
  }
  const auto midi=exportMidi(sample(),s);
  assert(midi.size()>30);
  assert(midi[0]=='M'&&midi[1]=='T'&&midi[2]=='h'&&midi[3]=='d');
  assert(midi[10]==0&&midi[11]==1&&midi[12]==3&&midi[13]==0xc0);
  assert(midi[midi.size()-3]==0xff&&midi[midi.size()-2]==0x2f);
}
