#include "RifferatorProcessor.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"

namespace {
const Steinberg::FUID processorUid(0x125A4601,0x52494646,0x45524154,0x4F520100);
const Steinberg::FUID controllerUid(0x125A4602,0x52494646,0x45524154,0x4F520100);
class Controller final : public Steinberg::Vst::EditControllerEx1 {
public:
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
