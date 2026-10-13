#include "vst3/UiGeometry.hpp"
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
