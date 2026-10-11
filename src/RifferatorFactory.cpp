#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
#include "pluginterfaces/base/ibstream.h"
#include <array>
#include <algorithm>
#include "public.sdk/source/vst/vstparameters.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"

namespace {
const Steinberg::FUID processorUid(0x125A4601,0x52494646,0x45524154,0x4F520100);
const Steinberg::FUID controllerUid(0x125A4602,0x52494646,0x45524154,0x4F520100);
class Controller final : public Steinberg::Vst::EditControllerEx1 {
public:
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
    using namespace Steinberg::Vst;
    if(EditControllerEx1::initialize(context)!=Steinberg::kResultOk) return Steinberg::kResultFalse;
    parameters.addParameter(STR16("Guitar Gate"),STR16("%"),0,0.95,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarGate);
    parameters.addParameter(STR16("Bass Gate"),STR16("%"),0,0.95,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassGate);
    parameters.addParameter(STR16("Bass Enabled"),nullptr,1,1.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassEnabled);
    parameters.addParameter(STR16("Root MIDI"),nullptr,127,40.0/127.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kRootPitch);
    parameters.addParameter(STR16("Bass Velocity"),nullptr,126,99.0/126.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassVelocity);
    parameters.addParameter(STR16("Fill Intensity"),STR16("%"),0,0.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kFillIntensity);
    return Steinberg::kResultOk;
  }
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override {
    using namespace Steinberg;
    using namespace Steinberg::Vst;
    if(!state)return kResultFalse;
    auto read32=[&](uint32_t& v) {
      int32 read=0;
      return state->read(&v,sizeof(v),&read)==kResultOk && read==sizeof(v);
    };
    uint32_t magic=0,version=0,count=0;
    if(!read32(magic)||!read32(version)||!read32(count) ||
       magic!=0x31524641u || (version!=1 && version!=2 && version!=3) ||
       count<1 || count>32) return kResultFalse;
    std::array<uint32_t,7> values{};
    for(auto& v:values)if(!read32(v))return kResultFalse;
    uint32_t fill=0;
    if(version>=2 && !read32(fill))return kResultFalse;
    if(values[0]>127 || values[1]<1 || values[1]>127 ||
       values[3]>100 || values[4]>100 || values[6]>1 ||
       fill>100)return kResultFalse;
    // Validate the entire serialized riff before publishing parameter values.
    // A truncated pattern must never partially restore the controller.
    for(uint32_t i=0;i<count;++i) {
      std::array<uint32_t,4> step{};
      for(auto& value:step)if(!read32(value))return kResultFalse;
      const int32_t degree=static_cast<int32_t>(step[1]);
      if(step[0]>5 || degree<-13 || degree>15 ||
         step[2]>1 || step[3]>1)return kResultFalse;
    }
    if(version>=3) {
      uint32_t enabled=0;
      if(!read32(enabled)||enabled>1)return kResultFalse;
      for(int i=0;i<48;++i) {
        uint32_t active=0;
        if(!read32(active)||active>1)return kResultFalse;
      }
    }
    // The processor owns the pattern. The controller only synchronizes
    // the automatable parameter values presented to the host/GUI.
    setParamNormalized(a125::rifferator::kRootPitch,double(values[0])/127.);
    setParamNormalized(a125::rifferator::kBassVelocity,double(values[1]-1)/126.);
    setParamNormalized(a125::rifferator::kGuitarGate,double(values[3])/100.);
    setParamNormalized(a125::rifferator::kBassGate,double(values[4])/100.);
    setParamNormalized(a125::rifferator::kBassEnabled,values[6]?1.:0.);
    setParamNormalized(a125::rifferator::kFillIntensity,double(fill)/100.);
    return kResultOk;
  }
  static Steinberg::FUnknown* createInstance(void*) {
    return static_cast<Steinberg::Vst::IEditController*>(new Controller);
  }
};
}
using namespace Steinberg;
using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF("125A","https://github.com/challanger2000/125A-Rifferator","")
DEF_CLASS2(INLINE_UID_FROM_FUID(processorUid),PClassInfo::kManyInstances,
 kVstAudioEffectClass,"125A Rifferator",Vst::kDistributable,
 Vst::PlugType::kInstrumentSynth,"1.0.0",kVstVersionString,
 a125::rifferator::Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(controllerUid),PClassInfo::kManyInstances,
 kVstComponentControllerClass,"125A Rifferator Controller",0,"",
 "1.0.0",kVstVersionString,Controller::createInstance)
END_FACTORY
