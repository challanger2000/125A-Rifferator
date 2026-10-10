#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
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
    parameters.addParameter(STR16("Guitar Gate"),STR16("%"),0,0.95,0,a125::rifferator::kGuitarGate);
    parameters.addParameter(STR16("Bass Gate"),STR16("%"),0,0.95,0,a125::rifferator::kBassGate);
    parameters.addParameter(STR16("Bass Enabled"),nullptr,1,1.0,0,a125::rifferator::kBassEnabled);
    parameters.addParameter(STR16("Root MIDI"),nullptr,127,40.0/127.0,0,a125::rifferator::kRootPitch);
    parameters.addParameter(STR16("Bass Velocity"),nullptr,126,99.0/126.0,0,a125::rifferator::kBassVelocity);
    parameters.addParameter(STR16("Fill Intensity"),STR16("%"),0,0.0,0,a125::rifferator::kFillIntensity);
    return Steinberg::kResultOk;
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
