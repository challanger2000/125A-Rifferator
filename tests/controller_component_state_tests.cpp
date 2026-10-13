#ifdef _WIN32
void* moduleHandle=nullptr; // SDK standalone EXE test, not shipping VST3.
#endif
#include "../src/RifferatorFactory.cpp"
#include "public.sdk/source/common/memorystream.h"
#include <cassert>
#include <cmath>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
int main(){
 Controller c;
 assert(c.initialize(nullptr)==Steinberg::kResultOk);
 Processor p;
 MemoryStream state;
 assert(p.getState(&state)==Steinberg::kResultOk);
 assert(state.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(c.setComponentState(&state)==Steinberg::kResultOk);
 assert(std::abs(c.getParamNormalized(kRootPitch)-40./127.)<1e-5);
 assert(std::abs(c.getParamNormalized(kGuitarGate)-.95)<1e-5);
 MemoryStream zoom;
 assert(c.getState(&zoom)==Steinberg::kResultOk);
 assert(zoom.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(c.setState(&zoom)==Steinberg::kResultOk);
 assert(c.terminate()==Steinberg::kResultOk);
}
