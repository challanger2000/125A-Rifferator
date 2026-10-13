#include "core/StandardMidiFile.hpp"
#include "instruments/Heavier7Strings.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
using namespace a125::riff;
using namespace a125::riff::heavier;

// Independently decode a standard MIDI file byte-stream. Do NOT use the
// production SMF VLQ or byte-order helpers; tests assert absolute MIDI times.
static std::uint16_t be16(const std::vector<std::uint8_t>& d,std::size_t i){
  assert(i+1<d.size());
  return static_cast<std::uint16_t>((d[i]<<8)|d[i+1]);
}
static std::uint32_t be32(const std::vector<std::uint8_t>& d,std::size_t i){
  assert(i+3<d.size());
  return (static_cast<std::uint32_t>(d[i])<<24)|
         (static_cast<std::uint32_t>(d[i+1])<<16)|
         (static_cast<std::uint32_t>(d[i+2])<<8)|d[i+3];
}
static std::uint32_t getVlq(const std::vector<std::uint8_t>& d,
                            std::size_t& i,std::size_t end){
  std::uint32_t n=0;
  for(int j=0;j<4;++j){
    assert(i<end);
    const auto byte=d[i++];
    n=(n<<7)|(byte&0x7f);
    if((byte&0x80)==0)return n;
  }
  assert(false && "invalid VLQ length");
  return 0;
}
struct Decoded {
  std::uint32_t tick;
  int status,pitch,velocity;
};
static std::vector<Decoded> decode(const std::vector<std::uint8_t>& midi,
                                   std::uint32_t expectedLength){
  assert(midi.size()>=26);
  assert(midi[0]=='M'&&midi[1]=='T'&&midi[2]=='h'&&midi[3]=='d');
  assert(be32(midi,4)==6);
  assert(be16(midi,8)==0); // format 0
  assert(be16(midi,10)==1);
  assert(be16(midi,12)==960);
  assert(midi[14]=='M'&&midi[15]=='T'&&midi[16]=='r'&&midi[17]=='k');
  const std::size_t end=22u+be32(midi,18);
  assert(end==midi.size());
  std::size_t index=22;
  std::uint32_t tick=0;
  std::vector<Decoded> result;
  bool sawEnd=false;
  while(index<end){
    tick+=getVlq(midi,index,end);
    assert(index<end);
    const int status=midi[index++];
    if(status==0xff){
      assert(index+1<end&&midi[index]==0x2f&&midi[index+1]==0);
      index+=2;
      sawEnd=true;
      break;
    }
    assert(status==0x80||status==0x90);
    assert(index+1<end);
    const int pitch=midi[index++],v=midi[index++];
    assert(pitch>=0&&pitch<=127&&v>=0&&v<=127);
    result.push_back({tick,status,pitch,v});
  }
  assert(sawEnd&&index==end&&tick==expectedLength);
  return result;
}
int main(){
  Settings settings;
  Pattern p;
  assert(p.setBars(2));
  Step pm;pm.on=true;pm.articulation=code(Articulation::PalmMute);
  Step op=pm;op.articulation=code(Articulation::OpenPowerchord);
  Step stop=pm;stop.stop=true;
  Step harm=pm;harm.articulation=code(Articulation::PinchHarmonic);
  assert(p.setStep(0,pm));
  assert(p.setStep(8,op));
  assert(p.setStep(12,stop));
  assert(p.setStep(16,harm));
  const auto bytes=exportMidi(p,settings,Velocity{30});
  const auto notes=decode(bytes,7680);
  assert(notes.size()==6);
  assert(notes[0].tick==0&&notes[0].status==0x90&&notes[0].velocity==30);
  assert(notes[1].tick==1920&&notes[1].status==0x80);
  assert(notes[2].tick==1920&&notes[2].status==0x90&&notes[2].velocity==115);
  assert(notes[3].tick==2880&&notes[3].status==0x80);
  assert(notes[4].tick==3840&&notes[4].status==0x90&&notes[4].velocity==127);
  assert(notes[5].tick==7680&&notes[5].status==0x80);
  // Empty pattern must still have a valid end-of-track at exact bar length.
  Pattern empty;
  assert(empty.setBars(8));
  assert(decode(exportMidi(empty,settings,Velocity{}),30720).empty());
  // Boundary note must receive NoteOff at the exact end of 8-bar export.
  assert(empty.setStep(127,pm));
  const auto edge=decode(exportMidi(empty,settings,Velocity{83}),30720);
  assert(edge.size()==2);
  assert(edge[0].tick==30480&&edge[0].velocity==83&&edge[0].status==0x90);
  assert(edge[1].tick==30720&&edge[1].status==0x80);
  // Malformed in-memory bar count must never dereference 128-element array.
  empty.bars=99;
  assert(!empty.valid()&&exportMidi(empty,settings,Velocity{}).empty());
}
