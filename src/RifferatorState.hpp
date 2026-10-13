#pragma once
#include "core/RiffEngine.hpp"
#include "pluginterfaces/base/ibstream.h"
#include <array>
#include <cstdint>
namespace a125::rifferator {
// One serializer shared by Processor and Controller: prevents state disagreement.
// V11 adds per-step note length; V1..V10 stay readable.
struct GuitarSnapshot {
  int root{40},gate{95},fill{0},pmMin{30},pmMax{40};
  std::array<Step,16> step{};
  std::array<bool,16> enabled{},eighth{};
  GuitarSnapshot() noexcept {
    for(int i=0;i<16;++i) {
      step[i].articulation=Articulation::PalmMute;
      enabled[i]=i%2==0;
    }
  }
};
template<class T> inline bool stateRead(Steinberg::IBStream* stream,T& v) {
  if(!stream)return false;
  Steinberg::int32 done=0;
  return stream->read(&v,sizeof(v),&done)==Steinberg::kResultOk&&done==sizeof(v);
}
template<class T> inline bool stateWrite(Steinberg::IBStream* stream,const T& v) {
  if(!stream)return false;
  Steinberg::int32 done=0;
  return stream->write(const_cast<T*>(&v),sizeof(v),&done)==Steinberg::kResultOk&&done==sizeof(v);
}
inline bool validSnapshot(const GuitarSnapshot& s) noexcept {
  if(s.root<0||s.root>127||s.gate<0||s.gate>100||
     s.fill<0||s.fill>100||s.pmMin<1||s.pmMax>127||s.pmMin>s.pmMax)return false;
  for(const auto& st:s.step)if(static_cast<int>(st.articulation)<0||
    static_cast<int>(st.articulation)>5||st.degree< -13||st.degree>15)return false;
  return true;
}
inline bool readSnapshot(Steinberg::IBStream* stream,GuitarSnapshot& output) {
  std::uint32_t magic=0,version=0;
  std::int32_t count=0;
  if(!stateRead(stream,magic)||!stateRead(stream,version)||!stateRead(stream,count)||
     magic!=0x31524641u||version<1||version>11)return false;
  GuitarSnapshot s;
  if(version==10||version==11) {
    if(count!=16)return false;
    std::array<std::int32_t,5> globals{};
    for(auto& v:globals)if(!stateRead(stream,v))return false;
    s.root=globals[0];s.gate=globals[1];s.fill=globals[2];
    s.pmMin=globals[3];s.pmMax=globals[4];
    for(int i=0;i<16;++i) {
      std::array<std::int32_t,6> row{};
      for(int j=0;j<(version==11?6:5);++j)
        if(!stateRead(stream,row[static_cast<std::size_t>(j)]))return false;
      if(row[0]<0||row[0]>1||row[1]<0||row[1]>5||row[2]<-13||
         row[2]>15||row[3]<0||row[3]>1||row[4]<0||row[4]>1||
         row[5]<0||row[5]>1)return false;
      s.enabled[i]=row[0]!=0;s.step[i]={
        static_cast<Articulation>(row[1]),row[2],row[3]!=0,row[4]!=0};
      s.eighth[i]=row[5]!=0;
    }
  } else {
    if(count<1||count>32)return false;
    std::array<std::int32_t,7> cfg{};
    for(auto& v:cfg)if(!stateRead(stream,v))return false;
    if(cfg[0]<0||cfg[0]>127||cfg[1]<1||cfg[1]>127||
       cfg[2]<-48||cfg[2]>48||cfg[3]<0||cfg[3]>100||
       cfg[4]<0||cfg[4]>100||cfg[5]<1||cfg[5]>960||
       cfg[6]<0||cfg[6]>1)return false;
    s.root=cfg[0];s.gate=cfg[3];
    if(version>=2) {
      std::int32_t value=0;
      if(!stateRead(stream,value)||value<0||value>100)return false;
      s.fill=value;
    }
    std::array<Step,32> old{};
    for(int i=0;i<count;++i) {
      std::array<std::int32_t,4> row{};
      for(auto& v:row)if(!stateRead(stream,v))return false;
      if(row[0]<0||row[0]>5||row[1]<-13||row[1]>15||
         row[2]<0||row[2]>1||row[3]<0||row[3]>1)return false;
      old[i]={static_cast<Articulation>(row[0]),row[1],row[2]!=0,row[3]!=0};
    }
    for(int i=0;i<16;++i) {
      s.step[i]=old[i%count];
      s.enabled[i]=old[i%count].articulation!=Articulation::Off;
    }
    if(version>=3) {
      for(int i=0;i<49;++i) {
        std::int32_t v=0;
        if(!stateRead(stream,v)||v<0||v>1)return false;
      }
    }
    if(version>=4)for(int i=0;i<16;++i) {
      std::int32_t v=0;
      if(!stateRead(stream,v)||v<0||v>1)return false;
      s.enabled[i]=v!=0;
    }
    if(version>=5) {
      for(int i=0;i<16;++i) {
        std::int32_t v=0;
        if(!stateRead(stream,v)||v<0||v>5)return false;
        s.step[i].articulation=static_cast<Articulation>(v);
      }
      for(int i=0;i<16;++i) {
        std::int32_t v=0;
        if(!stateRead(stream,v)||v<-13||v>15)return false;
        s.step[i].degree=v;
      }
      for(int i=0;i<16;++i) {
        std::int32_t v=0;
        if(!stateRead(stream,v)||v<0||v>1)return false;
        s.step[i].locked=v!=0;
      }
    }
    if(version>=6)for(int i=0;i<2;++i) {
      std::int32_t v=0;
      if(!stateRead(stream,v)||v<0||v>(i==0?2:4))return false;
    }
    if(version>=7) {
      for(int i=0;i<18;++i) {
        std::int32_t v=0;
        if(!stateRead(stream,v)||v<0||v>(i<2?1:128))return false;
      }
    }
    if(version>=8) {
      std::int32_t v=0;
      if(!stateRead(stream,v)||v<0||v>4)return false;
    }
    if(version>=9) {
      std::int32_t a=0,b=0;
      if(!stateRead(stream,a)||!stateRead(stream,b)||a<1||b>127||a>b)return false;
      s.pmMin=a;s.pmMax=b;
    }
  }
  if(!validSnapshot(s))return false;
  output=s; // No partial mutation on a malformed/short state stream.
  return true;
}
inline bool writeSnapshot(Steinberg::IBStream* stream,const GuitarSnapshot& s) {
  if(!validSnapshot(s))return false;
  const std::uint32_t magic=0x31524641u,version=11;
  const std::int32_t count=16;
  if(!stateWrite(stream,magic)||!stateWrite(stream,version)||!stateWrite(stream,count))return false;
  const std::array<std::int32_t,5> cfg{s.root,s.gate,s.fill,s.pmMin,s.pmMax};
  for(auto v:cfg)if(!stateWrite(stream,v))return false;
  for(int i=0;i<16;++i) {
    const auto& st=s.step[i];
    const std::array<std::int32_t,6> row{
      s.enabled[i]?1:0,static_cast<int>(st.articulation),
      st.degree,st.doubled?1:0,st.locked?1:0,s.eighth[i]?1:0};
    for(auto v:row)if(!stateWrite(stream,v))return false;
  }
  return true;
}
} // namespace a125::rifferator
