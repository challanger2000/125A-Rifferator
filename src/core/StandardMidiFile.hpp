#pragma once
#include "Pattern.hpp"
#include <cstdint>
#include <vector>
namespace a125::riff {
inline void big16(std::vector<std::uint8_t>& out,std::uint16_t value){
  out.push_back(static_cast<std::uint8_t>(value>>8));
  out.push_back(static_cast<std::uint8_t>(value));
}
inline void big32(std::vector<std::uint8_t>& out,std::uint32_t value){
  big16(out,static_cast<std::uint16_t>(value>>16));
  big16(out,static_cast<std::uint16_t>(value));
}
inline void vlq(std::vector<std::uint8_t>& out,std::uint32_t value){
  std::uint8_t bytes[5]{};int count=0;
  bytes[count++]=static_cast<std::uint8_t>(value&0x7f);
  while((value>>=7)!=0)
    bytes[count++]=static_cast<std::uint8_t>(0x80u|(value&0x7f));
  while(count>0)out.push_back(bytes[--count]);
}
// Standard MIDI File Format 0, one MIDI channel and 960 PPQ.
// Only returns bytes, not disk IO. The non-realtime Controller owns the save dialog.
template<class VelocityPolicy>
inline std::vector<std::uint8_t> exportMidi(const Pattern& pattern,const Settings& settings,VelocityPolicy&& policy){
  const auto events=compileOneCycle(pattern,settings,policy);
  if(!settings.valid()||!pitchInScale(settings.rootMidi,settings))return {};
  std::vector<std::uint8_t> track;
  std::int64_t previous=0;
  for(const auto& e:events){
    vlq(track,static_cast<std::uint32_t>(e.tick-previous));
    previous=e.tick;
    track.push_back(e.noteOn?0x90:0x80);
    track.push_back(static_cast<std::uint8_t>(e.pitch));
    track.push_back(static_cast<std::uint8_t>(e.velocity));
  }
  vlq(track,static_cast<std::uint32_t>(pattern.lengthTicks()-previous));
  track.insert(track.end(),{0xff,0x2f,0x00});
  std::vector<std::uint8_t> result{'M','T','h','d'};
  big32(result,6);big16(result,0);big16(result,1);big16(result,kPpq);
  result.insert(result.end(),{'M','T','r','k'});
  big32(result,static_cast<std::uint32_t>(track.size()));
  result.insert(result.end(),track.begin(),track.end());
  return result;
}
} // namespace a125::riff
