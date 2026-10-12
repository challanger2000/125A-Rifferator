// Real controller parameter and serialized-component-state regression.
// No VSTGUI editor is created: native-window lifecycle belongs to host QA.
#ifdef _WIN32
void* moduleHandle=nullptr;
#endif
#include "../src/RifferatorFactory.cpp"
#include "public.sdk/source/common/memorystream.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
static void near(double a,double b) {assert(std::abs(a-b)<0.00001);}
int main() {
  Controller controller;
  assert(controller.initialize(nullptr)==Steinberg::kResultOk);
  Processor processor;
  MemoryStream normal;
  assert(processor.getState(&normal)==Steinberg::kResultOk);
  std::vector<unsigned char> snapshot(reinterpret_cast<unsigned char*>(normal.getData()),
     reinterpret_cast<unsigned char*>(normal.getData())+normal.getSize());
  auto read32=[&](int index) {std::int32_t v=0;std::memcpy(&v,snapshot.data()+index*4,4);return v;};
  auto write32=[&](int index,std::int32_t v) {assert(index>=0 && (index+1)*4<=static_cast<int>(snapshot.size()));std::memcpy(snapshot.data()+index*4,&v,4);};
  int count=read32(2);assert(count>=1 && count<=32);
  write32(3,51);write32(4,115);write32(6,72);write32(7,63);write32(9,0);write32(10,80);
  const int afterSteps=11+4*count;
  write32(afterSteps,0);write32(afterSteps+1,0);
  const int active=afterSteps+49,art=active+16,degree=art+16,locks=degree+16,bass=locks+16;
  write32(active,0);write32(art,4);write32(degree,7);write32(locks,1);
  write32(bass,2);write32(bass+1,3);write32(bass+2,1);write32(bass+3,1);
  write32(bass+4,53);write32(bass+20,4);
  MemoryStream state(snapshot.data(),static_cast<Steinberg::TSize>(snapshot.size()));
  assert(controller.setComponentState(&state)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kRootPitch),51./127.);
  near(controller.getParamNormalized(kBassVelocity),114./126.);
  near(controller.getParamNormalized(kGuitarGate),.72);
  near(controller.getParamNormalized(kBassGate),.63);
  near(controller.getParamNormalized(kBassEnabled),0.);
  near(controller.getParamNormalized(kFillIntensity),.8);
  near(controller.getParamNormalized(kDrumEnabled),0.);
  near(controller.getParamNormalized(kDrumStepBase),0.);
  near(controller.getParamNormalized(kGuitarStepBase),0.);
  near(controller.getParamNormalized(kGuitarArticulationBase),.8);
  near(controller.getParamNormalized(kGuitarDegreeBase),20./28.);
  near(controller.getParamNormalized(kGuitarLockBase),1.);
  near(controller.getParamNormalized(kBassMode),1.);
  near(controller.getParamNormalized(kEbmStyle),.75);
  near(controller.getParamNormalized(kChordsEnabled),1.);
  near(controller.getParamNormalized(kChordsMode),1.);
  near(controller.getParamNormalized(kChordsQuality),1.);
  near(controller.getParamNormalized(kHybridRootBase),53./128.);
  // Locked-step DAW automation must NOT diverge from the processor: every
  // guitar matrix field stays unchanged, while the lock itself can be toggled.
  assert(controller.setParamNormalized(kGuitarStepBase,1.)==Steinberg::kResultOk);
  assert(controller.setParamNormalized(kGuitarArticulationBase,.4)==Steinberg::kResultOk);
  assert(controller.setParamNormalized(kGuitarDegreeBase,0.)==Steinberg::kResultOk);
  assert(controller.setParamNormalized(kGuitarDoubleBase,1.)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kGuitarStepBase),0.);
  near(controller.getParamNormalized(kGuitarArticulationBase),.8);
  near(controller.getParamNormalized(kGuitarDegreeBase),20./28.);
  near(controller.getParamNormalized(kGuitarDoubleBase),0.);
  assert(controller.setParamNormalized(kGuitarLockBase,0.)==Steinberg::kResultOk);
  assert(controller.setParamNormalized(kGuitarArticulationBase,.4)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kGuitarArticulationBase),.4);
  assert(controller.setParamNormalized(kGuitarLockBase,1.)==Steinberg::kResultOk);
  // Controller's public host automation entry point updates normalized values.
  assert(controller.setParamNormalized(kBassMode,.5)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kBassMode),.5);
  assert(controller.setParamNormalized(kChordsQuality,.25)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kChordsQuality),.25);
  assert(controller.setParamNormalized(kGuitarArticulationBase+1,.4)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kGuitarArticulationBase+1),.4);
  // A second *valid* project must fully replace prior locked state. Restoring
  // step parameters before clearing locks silently lost user values.
  write32(active,1);write32(art,5);write32(degree,-4);write32(locks,1);
  write32(13,1); // first Step's DOUBLE snapshot (word 11 + offset 2)
  MemoryStream nextProject(snapshot.data(),static_cast<Steinberg::TSize>(snapshot.size()));
  assert(controller.setComponentState(&nextProject)==Steinberg::kResultOk);
  near(controller.getParamNormalized(kGuitarStepBase),1.);
  near(controller.getParamNormalized(kGuitarArticulationBase),1.);
  near(controller.getParamNormalized(kGuitarDegreeBase),9./28.);
  near(controller.getParamNormalized(kGuitarDoubleBase),1.);
  near(controller.getParamNormalized(kGuitarLockBase),1.);
  const double baseline=controller.getParamNormalized(kBassMode);
  write32(bass+20,99); // reject malformed trailing field atomically
  MemoryStream corrupt(snapshot.data(),static_cast<Steinberg::TSize>(snapshot.size()));
  assert(controller.setComponentState(&corrupt)!=Steinberg::kResultOk);
  near(controller.getParamNormalized(kBassMode),baseline);
  assert(controller.terminate()==Steinberg::kResultOk);
}
