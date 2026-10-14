#pragma once
#include "Parameters.hpp"

namespace a125::riff::vst {
// Read the final normalized point of each host queue without allocating.
// A paste may override LOCK only inside its destination bar and only with a
// complete, readable, valid set of final LOCK values in the same callback.
template<class ReadPoint>
void applyParameterChanges(product::Snapshot& candidate,int count,ReadPoint readPoint) {
 int pasteBar=-1;
 for(int i=0;i<count;++i){
  std::uint32_t id=0;double value=0.;
  if(readPoint(i,id,value)&&id==PasteTransaction&&
     std::isfinite(value)&&value>=0.&&value<=1.)
   pasteBar=q(value,15)%kMaxBars;
 }
 bool locks[kStepsPerBar]{};
 if(pasteBar>=0&&pasteBar<candidate.pattern.bars){
  for(int i=0;i<count;++i){
   std::uint32_t id=0;double value=0.;
   if(readPoint(i,id,value)&&std::isfinite(value)&&value>=0.&&value<=1.&&
      id>=Lock+static_cast<std::uint32_t>(pasteBar*kStepsPerBar)&&
      id<Lock+static_cast<std::uint32_t>((pasteBar+1)*kStepsPerBar))
    locks[id-Lock-static_cast<std::uint32_t>(pasteBar*kStepsPerBar)]=true;
  }
  for(bool present:locks)if(!present){pasteBar=-1;break;}
 }else pasteBar=-1;
 if(pasteBar>=0)
  for(int j=0;j<kStepsPerBar;++j)
   candidate.pattern.steps[static_cast<std::size_t>(pasteBar*kStepsPerBar+j)].locked=false;
 for(int pass=0;pass<2;++pass){
  for(int i=0;i<count;++i){
   std::uint32_t id=0;double value=0.;
   if(!readPoint(i,id,value)||id==PasteTransaction)continue;
   const bool isLock=id>=Lock&&id<Lock+kMaxSteps;
   const auto base=(id/200u)*200u,index=id-base;
   const bool pasteCell=pasteBar>=0&&
       (base==On||base==Stop||base==Type||base==Degree||base==Lock)&&
       index>=static_cast<std::uint32_t>(pasteBar*kStepsPerBar)&&
       index<static_cast<std::uint32_t>((pasteBar+1)*kStepsPerBar);
   const int wantedPass=pasteCell?(isLock?1:0):(isLock?0:1);
   if(pass==wantedPass)apply(candidate,id,value);
  }
 }
}
} // namespace a125::riff::vst
