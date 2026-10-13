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
 for(int i=0;i<16;++i)assert(c.getParamNormalized(kGuitarLengthBase+i)<0.5);
 // Simulate the VSTGUI callback used by native zoom controls. Project
 // state must save exactly the applied factor and recreate it at next open.
 c.onZoomChanged(nullptr,1.5);
 MemoryStream zoom150;
 assert(c.getState(&zoom150)==Steinberg::kResultOk);
 assert(zoom150.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 std::uint32_t fields[4]{};
 for(auto& v:fields)assert(stateRead(&zoom150,v));
 assert(fields[2]==150);
 c.onZoomChanged(nullptr,1.);
 assert(zoom150.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 assert(c.setState(&zoom150)==Steinberg::kResultOk);
 MemoryStream restored;
 assert(c.getState(&restored)==Steinberg::kResultOk);
 assert(restored.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 for(auto& v:fields)assert(stateRead(&restored,v));
 assert(fields[2]==150);
 c.onZoomChanged(nullptr,1.);
 MemoryStream zoom100;
 assert(c.getState(&zoom100)==Steinberg::kResultOk);
 assert(zoom100.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
 for(auto& v:fields)assert(stateRead(&zoom100,v));
 assert(fields[2]==100);
 // Both display factors must reach the VSTGUI object; the real resize still
 // requires a DAW host frame and is an independent Studio One QA gate.
 auto* view=c.createView(Steinberg::Vst::ViewType::kEditor);
 assert(view);
 auto* vstgui=dynamic_cast<VSTGUI::VST3Editor*>(view);
 assert(vstgui!=nullptr);
 assert(dynamic_cast<VSTGUI::AspectRatioVST3Editor*>(view)==nullptr);
 assert(std::abs(vstgui->getZoomFactor()-1.)<1e-5);
 vstgui->setZoomFactor(1.5);
 assert(std::abs(vstgui->getZoomFactor()-1.5)<1e-5);
 view->release();
 assert(c.terminate()==Steinberg::kResultOk);
}
