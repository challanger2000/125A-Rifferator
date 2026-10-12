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
#include <cstdint>
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
  // VST3 controller state is independent of the musical Processor snapshot.
  // GUI zoom must survive project reopen at both supported factors and
  // malformed/truncated state must never overwrite the existing setting.
  MemoryStream zoomDefault;
  assert(controller.getState(&zoomDefault)==Steinberg::kResultOk);
  assert(zoomDefault.getSize()==3*static_cast<Steinberg::TSize>(sizeof(std::uint32_t)));
  std::vector<unsigned char> zoomData(
    reinterpret_cast<const unsigned char*>(zoomDefault.getData()),
    reinterpret_cast<const unsigned char*>(zoomDefault.getData())+zoomDefault.getSize());
  const auto zoomWord=[&](int index) {
    std::uint32_t v=0;
    std::memcpy(&v,zoomData.data()+index*sizeof(v),sizeof(v));return v;
  };
  const auto putZoomWord=[&](int index,std::uint32_t value) {
    std::memcpy(zoomData.data()+index*sizeof(value),&value,sizeof(value));
  };
  assert(zoomWord(0)==0x315A4652u && zoomWord(1)==1u && zoomWord(2)==100u);
  putZoomWord(2,150u);
  MemoryStream zoom150(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()));
  assert(controller.setState(&zoom150)==Steinberg::kResultOk);
  MemoryStream zoomPersisted;
  assert(controller.getState(&zoomPersisted)==Steinberg::kResultOk);
  assert(std::memcmp(zoomPersisted.getData(),zoomData.data(),zoomData.size())==0);
  const auto assertZoom150=[&]() {
    MemoryStream verify;
    assert(controller.getState(&verify)==Steinberg::kResultOk);
    assert(verify.getSize()==static_cast<Steinberg::TSize>(zoomData.size()));
    std::uint32_t v=0;
    std::memcpy(&v,static_cast<const char*>(verify.getData())+2*sizeof(v),sizeof(v));
    assert(v==150u);
  };
  putZoomWord(2,125u);
  MemoryStream invalidZoom(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()));
  assert(controller.setState(&invalidZoom)!=Steinberg::kResultOk);
  assertZoom150();
  putZoomWord(2,150u);putZoomWord(1,2u);
  MemoryStream futureZoom(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()));
  assert(controller.setState(&futureZoom)!=Steinberg::kResultOk);
  assertZoom150();
  putZoomWord(1,1u);putZoomWord(0,0u);
  MemoryStream badMagic(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()));
  assert(controller.setState(&badMagic)!=Steinberg::kResultOk);
  assertZoom150();
  putZoomWord(0,0x315A4652u);
  MemoryStream shortZoom(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()-1));
  assert(controller.setState(&shortZoom)!=Steinberg::kResultOk);
  assertZoom150();
  assert(controller.setState(nullptr)!=Steinberg::kResultOk);
  assertZoom150();
  putZoomWord(2,100u);
  MemoryStream zoom100(zoomData.data(),static_cast<Steinberg::TSize>(zoomData.size()));
  assert(controller.setState(&zoom100)==Steinberg::kResultOk);
  MemoryStream backTo100;
  assert(controller.getState(&backTo100)==Steinberg::kResultOk);
  assert(std::memcmp(backTo100.getData(),zoomData.data(),zoomData.size())==0);
  // Zoom operations must not alter unrelated processor-state parameters.
  near(controller.getParamNormalized(kGuitarLockBase),1.);
  const double baseline=controller.getParamNormalized(kBassMode);
  write32(bass+20,99); // reject malformed trailing field atomically
  MemoryStream corrupt(snapshot.data(),static_cast<Steinberg::TSize>(snapshot.size()));
  assert(controller.setComponentState(&corrupt)!=Steinberg::kResultOk);
  near(controller.getParamNormalized(kBassMode),baseline);
  // Real Controller generation bridge: same source state and same command
  // count must produce identical new 16-step riffs. LOCK protects every
  // field; Generate clears old DOUBLE only on unlocked steps, while Variate
  // preserves existing stroke subdivision and offers conservative mutation.
  {
    Controller first,second;
    assert(first.initialize(nullptr)==Steinberg::kResultOk);
    assert(second.initialize(nullptr)==Steinberg::kResultOk);
    auto init=[&](Controller& c){
      assert(c.setParamNormalized(kGuitarStepBase,1.)==Steinberg::kResultOk);
      assert(c.setParamNormalized(kGuitarArticulationBase,.8)==Steinberg::kResultOk);
      assert(c.setParamNormalized(kGuitarDegreeBase,16./28.)==Steinberg::kResultOk);
      assert(c.setParamNormalized(kGuitarDoubleBase,1.)==Steinberg::kResultOk);
      assert(c.setParamNormalized(kGuitarLockBase,1.)==Steinberg::kResultOk);
      assert(c.setParamNormalized(kGuitarDoubleBase+1,1.)==Steinberg::kResultOk);
    };
    init(first);init(second);
    auto snapshot=[&](Controller& c) {
      std::vector<double> v;
      v.reserve(16*5);
      for(int i=0;i<16;++i)for(auto base:{
          kGuitarStepBase,kGuitarArticulationBase,kGuitarDegreeBase,
          kGuitarLockBase,kGuitarDoubleBase})
        v.push_back(c.getParamNormalized(base+i));
      return v;
    };
    const auto original=snapshot(first);
    first.runGuitarGeneration(GenerationCommand::Generate);
    second.runGuitarGeneration(GenerationCommand::Generate);
    const auto generated=snapshot(first);
    assert(generated==snapshot(second));
    assert(generated!=original); // genuine generation, not a dead button
    for(int i=0;i<5;++i)assert(generated[i]==original[i]); // LOCK holds
    assert(first.getParamNormalized(kGuitarDoubleBase+1)==0.);
    int changed=0;
    for(int i=1;i<16;++i)
      changed+=(first.getParamNormalized(kGuitarStepBase+i)!=
                second.getParamNormalized(kGuitarStepBase+i))?1:0;
    assert(changed==0);
    // A second seed differs, but is still deterministic across controllers.
    first.runGuitarGeneration(GenerationCommand::Generate);
    second.runGuitarGeneration(GenerationCommand::Generate);
    assert(snapshot(first)==snapshot(second));
    assert(snapshot(first)!=generated);
    assert(first.setParamNormalized(kGuitarDoubleBase+1,1.)==Steinberg::kResultOk);
    assert(second.setParamNormalized(kGuitarDoubleBase+1,1.)==Steinberg::kResultOk);
    const auto beforeVariate=snapshot(first);
    first.runGuitarGeneration(GenerationCommand::Variate);
    second.runGuitarGeneration(GenerationCommand::Variate);
    assert(snapshot(first)==snapshot(second));
    for(int i=0;i<5;++i)assert(snapshot(first)[i]==original[i]);
    assert(first.getParamNormalized(kGuitarDoubleBase+1)==1.);
    // Variation may deliberately leave some steps unchanged (30% edit).
    int retained=0;
    const auto afterVariate=snapshot(first);
    for(int i=1;i<16;++i)
      if(afterVariate[static_cast<std::size_t>(i*5)]==
         beforeVariate[static_cast<std::size_t>(i*5)] &&
         afterVariate[static_cast<std::size_t>(i*5+1)]==
         beforeVariate[static_cast<std::size_t>(i*5+1)])++retained;
    assert(retained>=1);
    // STOP is an active NoteOff command, not an empty step. The core
    // generation Step::Off representation cannot distinguish the two.
    // A conservative VARIATE must retain unchanged STOP commands.
    Controller stopController;
    assert(stopController.initialize(nullptr)==Steinberg::kResultOk);
    for(int i=0;i<16;++i) {
      assert(stopController.setParamNormalized(kGuitarStepBase+i,1.)==Steinberg::kResultOk);
      assert(stopController.setParamNormalized(kGuitarArticulationBase+i,0.)==Steinberg::kResultOk);
    }
    stopController.runGuitarGeneration(GenerationCommand::Variate);
    int retainedStops=0;
    for(int i=0;i<16;++i) {
      const bool active=stopController.getParamNormalized(kGuitarStepBase+i)>=0.5;
      const bool stop=stopController.getParamNormalized(kGuitarArticulationBase+i)==0.;
      assert(!stop || active);
      if(stop && active)++retainedStops;
    }
    assert(retainedStops>=1);
    assert(stopController.terminate()==Steinberg::kResultOk);
    assert(first.terminate()==Steinberg::kResultOk);
    assert(second.terminate()==Steinberg::kResultOk);
  }
  assert(controller.terminate()==Steinberg::kResultOk);
}
