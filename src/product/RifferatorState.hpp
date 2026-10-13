#pragma once
#include "../core/MidiRuntime.hpp"
#include "../instruments/Heavier7Strings.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace a125::riff::product {

// Product-specific, SDK-free binary state format. This belongs OUTSIDE the
// reusable MIDI core because it includes H7-specific PM velocity limits.
// VST3 Processor getState/setState will adapt this codec to IBStream.
struct Snapshot {
  Pattern pattern{};
  Settings musical{};
  int pmVelocity{30};
  SyncMode syncMode{SyncMode::Transport};
  std::uint32_t seed{0x125A2026u};
  bool valid() const noexcept {
    if(!pattern.valid()||!musical.valid()||
       !pitchInScale(musical.rootMidi,musical)||
       pmVelocity<30||pmVelocity>83||
       (syncMode!=SyncMode::Transport&&syncMode!=SyncMode::MidiNote))return false;
    for(const auto& step:pattern.steps){
      if(step.articulation>3||step.degree < -14||step.degree>14)return false;
    }
    return true;
  }
};

constexpr std::size_t kStateHeaderSize=20;
constexpr std::size_t kStateStepSize=3;
constexpr std::size_t kStateByteCount=kStateHeaderSize+kMaxSteps*kStateStepSize+4;
static_assert(kStateByteCount==408,"V1 state wire size changed unexpectedly");
using StateBytes=std::array<std::uint8_t,kStateByteCount>;

inline std::uint32_t checksum(const StateBytes& bytes) noexcept {
  std::uint32_t result=2166136261u; // FNV-1a, accidental corruption detection
  for(std::size_t i=0;i<kStateByteCount-4;++i){
    result^=bytes[i];
    result*=16777619u;
  }
  return result;
}
inline void put32(StateBytes& out,std::size_t offset,std::uint32_t v) noexcept {
  for(int i=0;i<4;++i)out[offset+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(v>>(i*8));
}
inline std::uint32_t get32(const StateBytes& in,std::size_t offset) noexcept {
  return static_cast<std::uint32_t>(in[offset])|
    (static_cast<std::uint32_t>(in[offset+1])<<8)|
    (static_cast<std::uint32_t>(in[offset+2])<<16)|
    (static_cast<std::uint32_t>(in[offset+3])<<24);
}
inline bool encode(const Snapshot& snapshot,StateBytes& destination) noexcept {
  if(!snapshot.valid())return false;
  StateBytes tmp{};
  tmp[0]='R';tmp[1]='F';tmp[2]='R';tmp[3]='1';
  tmp[4]=1;tmp[5]=0; // major/minor codec version: 1
  tmp[6]=0;tmp[7]=0; // reserved, must remain zero
  tmp[8]=static_cast<std::uint8_t>(snapshot.pattern.bars);
  tmp[9]=static_cast<std::uint8_t>(snapshot.musical.key);
  tmp[10]=static_cast<std::uint8_t>(snapshot.musical.scale);
  tmp[11]=static_cast<std::uint8_t>(snapshot.musical.rootMidi);
  tmp[12]=static_cast<std::uint8_t>(snapshot.musical.fillIntensity);
  tmp[13]=static_cast<std::uint8_t>(snapshot.pmVelocity);
  tmp[14]=static_cast<std::uint8_t>(snapshot.syncMode);
  tmp[15]=0;
  put32(tmp,16,snapshot.seed);
  for(std::size_t i=0;i<kMaxSteps;++i){
    const auto& step=snapshot.pattern.steps[i];
    const auto offset=kStateHeaderSize+i*kStateStepSize;
    tmp[offset]=static_cast<std::uint8_t>((step.on?1u:0u)|
                  (step.locked?2u:0u)|(step.stop?4u:0u));
    tmp[offset+1]=step.articulation;
    tmp[offset+2]=static_cast<std::uint8_t>(step.degree+14);
  }
  put32(tmp,kStateByteCount-4,checksum(tmp));
  destination=tmp;
  return true;
}

// Atomically rejects partial, future-version, checksum-invalid, or semantically
// impossible buffers. No field in 'destination' changes on any decoding error.
inline bool decode(const std::uint8_t* data,std::size_t size,Snapshot& destination) noexcept {
  if(!data||size!=kStateByteCount)return false;
  StateBytes tmp{};
  for(std::size_t i=0;i<kStateByteCount;++i)tmp[i]=data[i];
  if(tmp[0]!='R'||tmp[1]!='F'||tmp[2]!='R'||tmp[3]!='1'||
     tmp[4]!=1||tmp[5]!=0||tmp[6]!=0||tmp[7]!=0||tmp[15]!=0)return false;
  if(get32(tmp,kStateByteCount-4)!=checksum(tmp))return false;
  Snapshot candidate;
  candidate.pattern.bars=tmp[8];
  candidate.musical.key=tmp[9];
  if(tmp[10]>1)return false;
  candidate.musical.scale=static_cast<Scale>(tmp[10]);
  candidate.musical.rootMidi=tmp[11];
  candidate.musical.fillIntensity=tmp[12];
  candidate.pmVelocity=tmp[13];
  if(tmp[14]>1)return false;
  candidate.syncMode=static_cast<SyncMode>(tmp[14]);
  candidate.seed=get32(tmp,16);
  for(std::size_t i=0;i<kMaxSteps;++i){
    const auto offset=kStateHeaderSize+i*kStateStepSize;
    const auto flags=tmp[offset];
    if((flags&0xf8u)!=0||tmp[offset+2]>28)return false;
    auto& step=candidate.pattern.steps[i];
    step.on=(flags&1u)!=0;
    step.locked=(flags&2u)!=0;
    step.stop=(flags&4u)!=0;
    step.articulation=tmp[offset+1];
    step.degree=static_cast<int>(tmp[offset+2])-14;
  }
  if(!candidate.valid())return false;
  destination=candidate;
  return true;
}
} // namespace a125::riff::product
