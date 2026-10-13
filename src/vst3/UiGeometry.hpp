#pragma once
#include <algorithm>
namespace a125::riff::vst::ui {
constexpr int width=1160,height=570,gridLeft=180,gridTop=224;
enum class Target { None,Zoom,Bars,Key,Scale,Root,Pm,Fill,Prev,Next,StepOn,Lock,Mode,Generate,Variate,Clear,Export,Cell,Picker,PickerCancel };
struct Hit {Target target{Target::None};int column{-1};int row{-1};};
// Direct horizontal selection for PM VELOCITY and FILL. All values are
// reachable, endpoints included, independent of GUI zoom.
inline int numericNudge(Target type,int current,int delta) noexcept {
 if(type==Target::Pm)return std::clamp(current+delta,30,83);
 if(type==Target::Fill)return std::clamp(current+delta,0,100);
 return current;
}
inline int numericValue(Target type,int x) noexcept {
 if(type==Target::Pm){
  constexpr int start=671,end=861;
  const int px=std::clamp(x,start,end);
  return 30+((px-start)*53+(end-start)/2)/(end-start);
 }
 if(type==Target::Fill){
  constexpr int start=895,end=1115;
  const int px=std::clamp(x,start,end);
  return ((px-start)*100+(end-start)/2)/(end-start);
 }
 return -1;
}
inline bool inside(int x,int y,int left,int top,int w,int h) noexcept {
 return x>=left&&x<left+w&&y>=top&&y<top+h;
}
inline bool shouldCancelPicker(bool open,Target target) noexcept {
 return open&&target!=Target::Picker;
}
inline Hit hit(int x,int y,bool picker) noexcept {
 if(x<0||y<0||x>=width||y>=height)return {};
 // Picker replaces the right detail panel, never covers STOP cells.
 if(picker&&inside(x,y,1102,225,31,23))return {Target::PickerCancel};
 if(picker&&inside(x,y,997,248,133,213)){
  const int c=(x-997)/45,r=(y-248)/43;
  if(c<3&&r<5&&x<997+c*45+43&&y<248+r*43+41)
   return {Target::Picker,r*3+c,0};
  return {};
 }
 if(inside(x,y,180,224,800,240)){
  const int c=(x-180)/50,r=(y-224)/48;
  if(c<16&&r<5&&x<180+c*50+48&&y<224+r*48+46)
   return {Target::Cell,c,r};
  return {};
 }
 if(inside(x,y,972,25,152,40))return {Target::Zoom};
 if(inside(x,y,34,100,134,55))return {Target::Bars};
 if(inside(x,y,180,100,116,55))return {Target::Key};
 if(inside(x,y,308,100,174,55))return {Target::Scale};
 if(inside(x,y,494,100,152,55))return {Target::Root};
 if(inside(x,y,658,100,214,55))return {Target::Pm};
 if(inside(x,y,884,100,242,55))return {Target::Fill};
 if(inside(x,y,82,177,40,40))return {Target::Prev};
 if(inside(x,y,155,177,40,40))return {Target::Next};
 if(inside(x,y,1008,276,112,42))return {Target::StepOn};
 if(inside(x,y,1008,322,112,42))return {Target::Lock};
 if(inside(x,y,992,177,144,40))return {Target::Mode};
 if(inside(x,y,34,493,235,44))return {Target::Generate};
 if(inside(x,y,283,493,168,44))return {Target::Variate};
 if(inside(x,y,465,493,215,44))return {Target::Clear};
 if(inside(x,y,694,493,219,44))return {Target::Export};
 return {};
}
} // namespace a125::riff::vst::ui
