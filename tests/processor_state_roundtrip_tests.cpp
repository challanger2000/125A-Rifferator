#include "../src/RifferatorProcessor.hpp"
#include "../src/RifferatorState.hpp"
#include "public.sdk/source/common/memorystream.h"
#include <cassert>
#include <array>
#include <cstring>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
int main(){
 Processor p;
 MemoryStream s;
 assert(p.getState(&s)==Steinberg::kResultOk);
 assert(s.getSize()>100);
 assert(s.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 GuitarSnapshot snap;
 assert(readSnapshot(&s,snap));
 assert(snap.root==40&&snap.gate==95&&snap.pmMin==30);
 snap.root=43;snap.gate=1;snap.fill=70;snap.pmMin=27;snap.pmMax=38;
 snap.enabled[4]=true;snap.step[4]={Articulation::Open,4,true,true};
 snap.eighth[4]=true;snap.eighth[0]=true;
 MemoryStream changed;
 assert(writeSnapshot(&changed,snap));
 assert(changed.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(p.setState(&changed)==Steinberg::kResultOk);
 MemoryStream roundtrip;
 assert(p.getState(&roundtrip)==Steinberg::kResultOk);
 assert(roundtrip.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 GuitarSnapshot restored;
 assert(readSnapshot(&roundtrip,restored));
 assert(restored.root==43&&restored.gate==1&&restored.step[4].locked);
 assert(restored.pmMin==27&&restored.pmMax==38);
 assert(restored.eighth[0]&&restored.eighth[4]&&!restored.eighth[5]);
 // Legacy V1..V9 reader: V9 includes discarded bass, drums and chords fields.
 MemoryStream legacy;
 const auto put=[&](std::int32_t v){assert(stateWrite(&legacy,v));};
 put(0x31524641);put(9);put(16);
 for(int v:{40,100,-12,95,95,240,1})put(v);
 put(0);
 for(int i=0;i<16;++i){put(2);put(1);put(0);put(0);}
 for(int i=0;i<49;++i)put(i==0?1:0);
 for(int i=0;i<16;++i)put(i%2==0);
 for(int i=0;i<16;++i)put(2);
 for(int i=0;i<16;++i)put(1);
 for(int i=0;i<16;++i)put(0);
 put(0);put(0);put(1);put(0);
 for(int i=0;i<16;++i)put(128);
 put(0);put(30);put(50);
 assert(legacy.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 GuitarSnapshot converted;
 assert(readSnapshot(&legacy,converted));
 assert(converted.root==40&&converted.pmMax==50&&converted.enabled[2]);
 for(bool eighth:converted.eighth)assert(!eighth);
 // Prior guitar-only V10 states have five integers per step, no LEN.
 MemoryStream v10;
 assert(stateWrite(&v10,static_cast<std::uint32_t>(0x31524641)));
 assert(stateWrite(&v10,static_cast<std::uint32_t>(10)));
 assert(stateWrite(&v10,static_cast<std::int32_t>(16)));
 for(int v:{40,95,0,30,40})assert(stateWrite(&v10,static_cast<std::int32_t>(v)));
 for(int i=0;i<16;++i)for(int v:{1,2,1,0,0})
   assert(stateWrite(&v10,static_cast<std::int32_t>(v)));
 assert(v10.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 GuitarSnapshot oldGuitar;
 assert(readSnapshot(&v10,oldGuitar));
 for(bool eighth:oldGuitar.eighth)assert(!eighth);
 assert(legacy.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(p.setState(&legacy)==Steinberg::kResultOk);
 // Malformed/truncated projects do not partially overwrite live state.
 MemoryStream bad;
 assert(stateWrite(&bad,static_cast<std::uint32_t>(0x31524641)));
 assert(bad.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(p.setState(&bad)!=Steinberg::kResultOk);
}
