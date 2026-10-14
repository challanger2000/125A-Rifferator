#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "../product/RifferatorState.hpp"
namespace a125::riff::vst {
constexpr std::uint32_t PasteTransaction=200,Key=100,ScaleId=101,Root=102,Bars=103,Pm=104,Fill=105,Mode=106;
constexpr std::uint32_t On=1000,Stop=1200,Type=1400,Degree=1600,Lock=1800;
inline int q(double v,int steps) noexcept {return std::isfinite(v)?static_cast<int>(std::floor(std::clamp(v,0.,1.)*steps+0.5)):-1;}
inline double n(int value,int min,int max) noexcept {return double(value-min)/double(max-min);}
// The selected root must always belong to the active scale. Resolve incompatible
// KEY/SCALE/ROOT automation deterministically in BOTH Processor and Controller.
inline void snapRoot(Settings& settings) noexcept {
 if(pitchInScale(settings.rootMidi,settings))return;
 const int original=settings.rootMidi;
 for(int distance=1;distance<=12;++distance){
  if(original+distance<=127&&pitchInScale(original+distance,settings)){
   settings.rootMidi=original+distance;return;
  }
  if(original-distance>=0&&pitchInScale(original-distance,settings)){
   settings.rootMidi=original-distance;return;
  }
 }
}
inline bool apply(product::Snapshot& s,std::uint32_t id,double v) noexcept {
 if(!std::isfinite(v)||v<0.||v>1.)return false;
 if(id==Key){
  const int previous=s.musical.key;
  const bool rootOnTonic=s.musical.rootMidi%12==previous;
  const int next=q(v,11);
  int delta=next-previous;
  if(delta>6)delta-=12;
  if(delta< -6)delta+=12;
  s.musical.key=next;
  if(rootOnTonic)
   s.musical.rootMidi=std::clamp(s.musical.rootMidi+delta,0,127);
  snapRoot(s.musical);
 }else if(id==ScaleId){
  s.musical.scale=static_cast<Scale>(q(v,1));
  snapRoot(s.musical);
 }else if(id==Root){
  s.musical.rootMidi=q(v,127);
  snapRoot(s.musical);
 }
 else if(id==Bars){static constexpr int b[]={1,2,4,8};s.pattern.bars=b[q(v,3)];}
 else if(id==Pm)s.pmVelocity=q(v,53)+30;
 else if(id==Fill)s.musical.fillIntensity=q(v,100);
 else if(id==Mode)s.syncMode=static_cast<SyncMode>(q(v,1));
 else {
  const auto base=(id/200u)*200u,index=id-base;
  if(index>=kMaxSteps)return false;
  auto& step=s.pattern.steps[index];
  if(base==Lock){step.locked=q(v,1)!=0;return true;}
  if(step.locked)return false;
  if(base==On)step.on=q(v,1)!=0;
  else if(base==Stop)step.stop=q(v,1)!=0;
  else if(base==Type)step.articulation=static_cast<std::uint8_t>(q(v,3));
  else if(base==Degree)step.degree=q(v,28)-14;
  else return false;
 }
 return true;
}
inline double read(const product::Snapshot& s,std::uint32_t id) noexcept {
 if(id==Key)return n(s.musical.key,0,11);
 if(id==ScaleId)return static_cast<int>(s.musical.scale);
 if(id==Root)return n(s.musical.rootMidi,0,127);
 if(id==Bars)return (s.pattern.bars==8?3.:s.pattern.bars==4?2.:s.pattern.bars==2?1.:0.)/3.;
 if(id==Pm)return n(s.pmVelocity,30,83);
 if(id==Fill)return n(s.musical.fillIntensity,0,100);
 if(id==Mode)return static_cast<int>(s.syncMode);
 const auto base=(id/200u)*200u,index=id-base;
 if(index>=kMaxSteps)return 0.;
 const auto& step=s.pattern.steps[index];
 if(base==On)return step.on?1.:0.;
 if(base==Stop)return step.stop?1.:0.;
 if(base==Type)return n(step.articulation,0,3);
 if(base==Degree)return n(step.degree,-14,14);
 if(base==Lock)return step.locked?1.:0.;
 return 0.;
}
}
