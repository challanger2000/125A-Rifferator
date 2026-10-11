#include "../src/RifferatorProcessor.hpp"
#include "../src/RifferatorParameters.hpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/common/memorystream.h"
#ifdef NDEBUG
#error Regression asserts disabled in CI
#endif
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdint>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
int main(){
  Processor original;
  MemoryStream full;
  assert(original.getState(&full)==Steinberg::kResultOk);
  assert(full.getSize()>100);
  assert(full.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
  Processor restored;
  assert(restored.setState(&full)==Steinberg::kResultOk);
  MemoryStream again;
  assert(restored.getState(&again)==Steinberg::kResultOk);
  assert(again.getSize()==full.getSize());
  assert(std::memcmp(again.getData(),full.getData(),full.getSize())==0);
  // Truncated stream must fail; subsequent state must stay unchanged.
  MemoryStream truncated(full.getData(),full.getSize()-1);
  assert(restored.setState(&truncated)!=Steinberg::kResultOk);
  MemoryStream retained;
  assert(restored.getState(&retained)==Steinberg::kResultOk);
  assert(retained.getSize()==full.getSize());
  assert(std::memcmp(retained.getData(),full.getData(),full.getSize())==0);
  // Legacy V6 projects end after BassMode and EBMStyle, i.e. 19 ints
  // before current V8's chords enabled/mode, 16 roots and quality.
  assert(full.getSize()>=76);
  const auto legacySize=static_cast<std::size_t>(full.getSize())-19u*sizeof(std::int32_t);
  auto* data=reinterpret_cast<const unsigned char*>(full.getData());
  std::vector<unsigned char> legacy(data,data+legacySize);
  legacy[4]=6;legacy[5]=legacy[6]=legacy[7]=0;
  MemoryStream old(legacy.data(),static_cast<Steinberg::TSize>(legacy.size()));
  Processor migrated;
  assert(migrated.setState(&old)==Steinberg::kResultOk);
  MemoryStream upgraded;
  assert(migrated.getState(&upgraded)==Steinberg::kResultOk);
  assert(upgraded.getSize()==full.getSize());
// V8 non-default full snapshot: validate every serialized feature class,
// including MIDI register controls that a default-only roundtrip misses.
  {
    std::vector<unsigned char> changed(data,data+full.getSize());
    auto write32=[&](std::size_t index,std::int32_t value) {
      assert((index+1)*4<=changed.size());
      std::memcpy(changed.data()+index*4,&value,4);
    };
    std::int32_t count=0;
    std::memcpy(&count,changed.data()+8,4);
    assert(count>=1 && count<=32);
    write32(3,52);   // Root E3
    write32(4,117);  // Bass velocity
    write32(5,-24);  // Bass offset
    write32(6,58);   // Guitar gate
    write32(7,72);   // Bass gate
    write32(9,0);    // Bass disabled
    write32(10,75);  // Fill intensity
    const std::size_t afterSteps=11u+static_cast<std::size_t>(count)*4u;
    write32(afterSteps,0);     // Drums disabled
    write32(afterSteps+1,0);   // First kick disabled
    const std::size_t guitarOn=afterSteps+49;
    write32(guitarOn+0,0);
    const std::size_t art=guitarOn+16;
    write32(art+0,4);
    const std::size_t deg=art+16;
    write32(deg+0,7);
    const std::size_t locks=deg+16;
    write32(locks+0,1);
    const std::size_t bassMode=locks+16;
    write32(bassMode,2);       // HYBRID
    write32(bassMode+1,3);     // EBM style
    write32(bassMode+2,1);     // Chords enabled
    write32(bassMode+3,1);     // Chords root-only
    write32(bassMode+4,45);    // First bar explicit root
    write32(bassMode+20,4);    // Sus4 chord quality
    MemoryStream custom(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    Processor customProcessor;
    assert(customProcessor.setState(&custom)==Steinberg::kResultOk);
    MemoryStream customSaved;
    assert(customProcessor.getState(&customSaved)==Steinberg::kResultOk);
    assert(customSaved.getSize()==static_cast<Steinberg::TSize>(changed.size()));
    assert(std::memcmp(customSaved.getData(),changed.data(),changed.size())==0);
    // Invalid later field: do not publish any partially applied state.
    write32(bassMode+20,99);
    MemoryStream corrupt(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    assert(customProcessor.setState(&corrupt)!=Steinberg::kResultOk);
    MemoryStream afterReject;
    assert(customProcessor.getState(&afterReject)==Steinberg::kResultOk);
    assert(afterReject.getSize()==customSaved.getSize());
    assert(std::memcmp(afterReject.getData(),customSaved.getData(),customSaved.getSize())==0);
  }

// Legacy V1 snapshot with ONE step must restore safely and survive subsequent
// VST3 parameter processing for the highest addressed guitar step (index 15).
  {
    std::vector<unsigned char> small(data,data+full.getSize());
    auto w=[&](std::size_t word,std::int32_t value) {
      assert((word+1)*4<=small.size());
      std::memcpy(small.data()+word*4,&value,4);
    };
    std::int32_t originalCount=0;
    std::memcpy(&originalCount,small.data()+8,4);
    assert(originalCount==16);
    // V1: magic/version/count + 7 settings + 1 step (four words).
    // V2 introduced the fill word, so remove it before extracting V1 step.
    small.erase(small.begin()+10u*sizeof(std::int32_t),
                small.begin()+11u*sizeof(std::int32_t));
    small.resize(14u*sizeof(std::int32_t));
    w(1,1);w(2,1);
    MemoryStream oneStep(small.data(),static_cast<Steinberg::TSize>(small.size()));
    Processor shortPattern;
    assert(shortPattern.setState(&oneStep)==Steinberg::kResultOk);
    // Exercise the actual Processor::process automation path on step 16.
    // Previously this indexed steps_[15] in a restored one-element vector.
    Steinberg::Vst::ParameterChanges changes(3);
    auto queueValue=[&](Steinberg::Vst::ParamID id,double normalized) {
      Steinberg::int32 queueIndex=0,pointIndex=0;
      auto* queue=changes.addParameterData(id,queueIndex);
      assert(queue!=nullptr);
      assert(queue->addPoint(0,normalized,pointIndex)==Steinberg::kResultOk);
    };
    queueValue(kGuitarArticulationBase+15,4./5.);
    queueValue(kGuitarDegreeBase+15,20./28.);
    queueValue(kGuitarLockBase+15,1.);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=0;
    block.inputParameterChanges=&changes;
    assert(shortPattern.process(block)==Steinberg::kResultOk);
    MemoryStream saved;
    assert(shortPattern.getState(&saved)==Steinberg::kResultOk);
    std::int32_t savedCount=0;
    std::memcpy(&savedCount,static_cast<const char*>(saved.getData())+8,4);
    assert(savedCount==1);
    // State V8 contains fixed-size 16-step live controls after the one
    // legacy pattern step. Verify all three high-index edits survived.
    const auto* words=static_cast<const char*>(saved.getData());
    auto wordAt=[&](std::size_t index) {
      std::int32_t v=0;
      assert((index+1)*4<=static_cast<std::size_t>(saved.getSize()));
      std::memcpy(&v,words+index*4,4);
      return v;
    };
    const std::size_t base=11u+4u+49u;
    assert(wordAt(base+16u+15u)==4);
    assert(wordAt(base+32u+15u)==7);
    assert(wordAt(base+48u+15u)==1);
  }

  // Regression: project recall must apply every stored drum step to the LIVE
  // MIDI masks. A byte-identical getState roundtrip alone missed this defect.
  {
    std::vector<unsigned char> state(data,data+full.getSize());
    auto put=[&](std::size_t i,std::int32_t v){
      assert((i+1)*4<=state.size());std::memcpy(state.data()+i*4,&v,4);
    };
    const std::size_t drums=11u+16u*4u;
    put(drums,1); // enable drums
    for(std::size_t i=0;i<48;++i)put(drums+1+i,0);
    put(drums+1,1); // only kick at step 0; all hats/snare OFF
    MemoryStream saved(state.data(),static_cast<Steinberg::TSize>(state.size()));
    Processor replay;
    assert(replay.setState(&saved)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=6000;
    assert(replay.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.projectTimeMusic=0.;ctx.tempo=120.;
    Steinberg::Vst::EventList emitted(512);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=6000;block.processContext=&ctx;block.outputEvents=&emitted;
    assert(replay.process(block)==Steinberg::kResultOk);
    int kickOns=0,otherDrumOns=0;
    for(Steinberg::int32 i=0;i<emitted.getEventCount();++i){
      Steinberg::Vst::Event e{};
      assert(emitted.getEvent(i,e)==Steinberg::kResultOk);
      if(e.busIndex==2 && e.type==Steinberg::Vst::Event::kNoteOnEvent){
        if(e.noteOn.pitch==36)++kickOns;else ++otherDrumOns;
      }
    }
    assert(kickOns==1 && otherDrumOns==0);
  }
}
