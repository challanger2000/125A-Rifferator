#pragma once
#include "../product/RifferatorState.hpp"
#include "../core/BarClipboard.hpp"
#include "Parameters.hpp"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include "vstgui/uidescription/uiattributes.h"
namespace VSTGUI { class CView; class VST3Editor; class CControl; class IUIDescription; }
namespace a125::riff::vst {
class RifferatorPanel;
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
 bool releasePitchOnly_{false};
};
class Controller final:public EditController,public VSTGUI::VST3EditorDelegate {

public:
 static FUnknown* createInstance(void*){return static_cast<IEditController*>(new Controller);}
 tresult PLUGIN_API initialize(FUnknown*) override;
 tresult PLUGIN_API setComponentState(IBStream*) override;
 tresult PLUGIN_API setState(IBStream*) override;
 tresult PLUGIN_API getState(IBStream*) override;
 IPlugView* PLUGIN_API createView(FIDString) override;
 tresult PLUGIN_API setParamNormalized(ParamID,ParamValue) override;
 VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr,
     const VSTGUI::UIAttributes&,const VSTGUI::IUIDescription*,
     VSTGUI::VST3Editor*) override;
 void willClose(VSTGUI::VST3Editor*) override;
 const product::Snapshot& shown() const noexcept {return visual_;}
 int bar() const noexcept {return bar_;}
 int focused() const noexcept {return focused_;}
 int picker() const noexcept {return pickerStep_;}
 double zoom() const noexcept {return zoom_;}
 const char* notification() const noexcept {return status_;}
 void click(int x,int y,bool shift,bool control=false);
 bool cancelPicker();
 void beginNumericDrag(int x,int y);
 void updateNumericDrag(int x);
 void endNumericDrag();
 void nudgeNumeric(int x,int y,int delta);
private:
 void publish(ParamID,double);
 void publishPattern(const product::Snapshot& target);
 void openChoiceMenu(ParamID id,int x,int y);
 void redraw();
 void exportMidiFromUI();

 product::Snapshot visual_{};
 VSTGUI::VST3Editor* editor_{nullptr};
 RifferatorPanel* panel_{nullptr};
 int bar_{0},focused_{0},pickerStep_{-1};
 int activeNumericParam_{-1};
 bool componentRestored_{false};
 double zoom_{1.0};
 BarClipboard clipboard_{};
 bool pasteToggle_{false};
 std::uint32_t seed_{0x125A2026u}; // persisted Controller-only state v2
 const char* status_{"READY"};
};
}
