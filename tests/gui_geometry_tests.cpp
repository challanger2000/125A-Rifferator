#include "vst3/UiGeometry.hpp"
#include "vst3/UiStepPolicy.hpp"
#include "instruments/Heavier7Strings.hpp"
#include "vst3/Parameters.hpp"
#include <cassert>
using namespace a125::riff::vst::ui;
int main(){
 for(int bar=0;bar<8;++bar){
  (void)bar;
  for(int row=0;row<5;++row)
   for(int c=0;c<16;++c){
    const auto result=hit(180+50*c+24,224+48*row+23,false);
    assert(result.target==Target::Cell&&result.column==c&&result.row==row);
   }
 }
 assert(hit(180,224,false).target==Target::Cell);
 assert(hit(979,462,false).target==Target::None); // cell gutter
 assert(hit(180,464,false).target==Target::None);
 // Picker must never obstruct any STOP cell.
 for(int c=0;c<16;++c){
  const auto stop=hit(180+50*c+24,224+48*4+23,true);
  assert(stop.target==Target::Cell&&stop.column==c&&stop.row==4);
 }
 // All 15 diatonic choices remain mouse-accessible.
 for(int i=0;i<15;++i){
  const auto choice=hit(997+(i%3)*45+21,248+(i/3)*43+20,true);
  assert(choice.target==Target::Picker&&choice.column==i);
 }
 assert(hit(979,426,true).target==Target::None);
 assert(hit(1009,430,false).target==Target::Mode);
 assert(hit(1009,430,true).target==Target::Picker);
 assert(hit(1009,430,true).column==12);
 // All header fields open direct menus / select an absolute numeric value.
 assert(hit(75,131,false).target==Target::Bars);
 assert(hit(240,131,false).target==Target::Key);
 assert(hit(390,131,false).target==Target::Scale);
 assert(hit(565,131,false).target==Target::Root);
 assert(hit(720,131,false).target==Target::Pm);
 assert(hit(960,131,false).target==Target::Fill);
 assert(numericValue(Target::Pm,0)==30);
 assert(numericValue(Target::Pm,671)==30);
 assert(numericValue(Target::Pm,861)==83);
 assert(numericValue(Target::Pm,5000)==83);
 assert(numericValue(Target::Fill,0)==0);
 assert(numericValue(Target::Fill,895)==0);
 assert(numericValue(Target::Fill,1115)==100);
 assert(numericValue(Target::Fill,5000)==100);
 for(int x=672;x<=1115;++x){
  assert(numericValue(Target::Pm,x)>=30&&numericValue(Target::Pm,x)<=83);
  assert(numericValue(Target::Fill,x)>=0&&numericValue(Target::Fill,x)<=100);
 }
 for(int x=672;x<861;++x)
  assert(numericValue(Target::Pm,x)<=numericValue(Target::Pm,x+1));
 for(int x=895;x<1115;++x)
  assert(numericValue(Target::Fill,x)<=numericValue(Target::Fill,x+1));
 assert(numericValue(Target::Bars,100)==-1);
 // Regression for the ROOT trap: even the highest E-minor scale note
 // (G9/MIDI 127) must be recoverable by the direct tonic-octave-2 reset.
 {
  a125::riff::product::Snapshot extreme;
  using namespace a125::riff::vst;
  assert(apply(extreme,Root,n(127,0,127)));
  assert(extreme.musical.rootMidi==127);
  assert(apply(extreme,Root,n(36+extreme.musical.key,0,127)));
  assert(extreme.musical.rootMidi==40&&extreme.valid());
 }


 // All five rows and all 128 step positions: toggle ON then OFF.
 for(int bars : {1,2,4,8})for(int row=0;row<5;++row){
  a125::riff::Pattern pattern;
  assert(pattern.setBars(bars));
  for(int i=0;i<pattern.activeSteps();++i){
   assert(toggleStep(pattern,i,row));
   const auto& step=pattern.steps[static_cast<std::size_t>(i)];
   assert(step.on);
   assert(step.stop==(row==4));
   assert(toggleStep(pattern,i,row));
   assert(!step.on&&!step.stop);
   assert(resetStep(pattern,i));
   assert(!step.on&&!step.stop&&step.degree==0&&step.articulation==0);
  }
 }
 // Crucial STOP regression: clicking an already active STOP must remove it.
 {
  a125::riff::Pattern pattern;
  assert(toggleStep(pattern,3,4));
  assert(pattern.steps[3].on&&pattern.steps[3].stop);
  assert(toggleStep(pattern,3,4));
  assert(!pattern.steps[3].on&&!pattern.steps[3].stop);
  // Switching an existing STOP to a guitar articulation must clear STOP.
  assert(toggleStep(pattern,3,4));
  assert(toggleStep(pattern,3,1));
  assert(pattern.steps[3].on&&!pattern.steps[3].stop&&
         pattern.steps[3].articulation==2);
  assert(pattern.setLock(3,true));
  const auto before=pattern.steps[3];
  assert(!toggleStep(pattern,3,4));
  assert(!resetStep(pattern,3));
  assert(pattern.steps[3].on==before.on&&
         pattern.steps[3].stop==before.stop&&
         pattern.steps[3].locked);
  assert(pattern.setLock(3,false));
  assert(resetStep(pattern,3));
  assert(!pattern.steps[3].on&&!pattern.steps[3].locked);
 }
 // Every control is classified for Ctrl+click, rather than treating Ctrl
 // as an unconditional blocker of transport/navigation/export/commands.
 for(const auto target : {Target::Bars,Target::Key,Target::Scale,Target::Root,
                          Target::Pm,Target::Fill,Target::Mode,Target::Zoom,
                          Target::StepOn,Target::Lock,Target::Cell})
  assert(supportsReset(target));
 for(const auto target : {Target::None,Target::Prev,Target::Next,
                          Target::Generate,Target::Variate,Target::Clear,
                          Target::Export,Target::Picker})
  assert(!supportsReset(target));
 // H7: activating one articulation row must emit the exact target
 // note velocity; another click turns it off without a stuck MIDI event.
 {
  a125::riff::Pattern p;
  a125::riff::Settings s;
  for(int row=0;row<4;++row){
   assert(toggleStep(p,0,row));
   const auto events=a125::riff::compileOneCycle(
        p,s,a125::riff::heavier::Velocity{});
   assert(events.size()==2);
   const int velocity[4]={127,115,30,20};
   assert(events[0].noteOn&&events[0].velocity==velocity[row]);
   assert(!events[1].noteOn);
   assert(toggleStep(p,0,row));
   assert(a125::riff::compileOneCycle(
        p,s,a125::riff::heavier::Velocity{}).empty());
  }
 }
 assert(hit(1100,40,false).target==Target::Zoom);
 assert(hit(34,510,false).target==Target::Generate);
 assert(hit(720,510,false).target==Target::Export);
 assert(hit(1010,284,false).target==Target::StepOn);
 assert(hit(1010,340,false).target==Target::Lock);
 for(int y=0;y<570;++y)for(int x=0;x<1160;++x){
  const auto h=hit(x,y,false);
  if(h.target==Target::Cell){
   assert(h.column>=0&&h.column<16&&h.row>=0&&h.row<5);
  }
 }
}
