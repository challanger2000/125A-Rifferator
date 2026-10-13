#include "vst3/UiGeometry.hpp"
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
