#pragma once
#include "../product/RifferatorState.hpp"
#include "Parameters.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
namespace a125::riff::vst {
using namespace Steinberg;
using namespace Steinberg::Vst;
static const FUID ProcessorUID(0x125A7151,0x52494646,0x45524154,0x00010001);
static const FUID ControllerUID(0x125A7152,0x52494646,0x45524154,0x00010001);
class Processor final:public AudioEffect {
public:
 Processor();
 static FUnknown* createInstance(void*){return static_cast<IAudioProcessor*>(new Processor);}
 tresult PLUGIN_API initialize(FUnknown*) override;
 tresult PLUGIN_API setupProcessing(ProcessSetup&) override;
 tresult PLUGIN_API setActive(TBool) override;
 tresult PLUGIN_API setProcessing(TBool) override;
 tresult PLUGIN_API canProcessSampleSize(int32) override;
 tresult PLUGIN_API getState(IBStream*) override;
 tresult PLUGIN_API setState(IBStream*) override;
 tresult PLUGIN_API process(ProcessData&) override;
private:
 product::Snapshot snapshot_{};
 MidiRuntime runtime_{};
 double sampleRate_{48000.};
 bool cleanupPending_{false};
};
class Controller final:public EditController {
public:
 static FUnknown* createInstance(void*){return static_cast<IEditController*>(new Controller);}
 tresult PLUGIN_API initialize(FUnknown*) override;
 tresult PLUGIN_API setComponentState(IBStream*) override;
 tresult PLUGIN_API setState(IBStream*) override;
 tresult PLUGIN_API getState(IBStream*) override;
 IPlugView* PLUGIN_API createView(FIDString) override;
private:
 product::Snapshot visual_{};
};
}
