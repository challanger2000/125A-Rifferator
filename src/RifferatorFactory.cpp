#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
#include "pluginterfaces/base/ibstream.h"
#include <array>
#include <algorithm>
#include "public.sdk/source/vst/vstparameters.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/uidescription/uiattributes.h"
#include <cstring>

namespace {
const Steinberg::FUID processorUid(0x125A4601,0x52494646,0x45524154,0x4F520100);
const Steinberg::FUID controllerUid(0x125A4602,0x52494646,0x45524154,0x4F520100);
class Controller final : public Steinberg::Vst::EditControllerEx1,
                         public VSTGUI::VST3EditorDelegate,
                         public VSTGUI::IControlListener {
public:
  VSTGUI::CView* verifyView(VSTGUI::CView* view,
                            const VSTGUI::UIAttributes&,
                            const VSTGUI::IUIDescription*,
                            VSTGUI::VST3Editor*) override {
    if(auto* control=dynamic_cast<VSTGUI::CControl*>(view)) {
      if(control->getTag()>=500 && control->getTag()<596)
        control->setListener(this);
    }
    return view;
  }
  void valueChanged(VSTGUI::CControl* control) override {
    if(!control || control->getValueNormalized()<=0.5f)return;
    const int tag=control->getTag();
    if(tag<500||tag>=596)return;
    const int step=(tag-500)%16;
    const int art=(tag-500)/16;
    const auto id=a125::rifferator::kGuitarArticulationBase+step;
    if(getParamNormalized(a125::rifferator::kGuitarLockBase+step)>=0.5)return;
    beginEdit(id);
    performEdit(id,static_cast<double>(art)/5.);
    endEdit(id);
  }
  Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override {
    if(name && std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0) {
      auto* editor=new VSTGUI::VST3Editor(this,"RifferatorView","rifferator.uidesc");
      editor->setDelegate(this);
      editor->setZoomFactor(1.);
      return editor;
    }
    return nullptr;
  }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
    using namespace Steinberg::Vst;
    if(EditControllerEx1::initialize(context)!=Steinberg::kResultOk) return Steinberg::kResultFalse;
    parameters.addParameter(STR16("Guitar Gate"),STR16("%"),0,0.95,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarGate);
    parameters.addParameter(STR16("Bass Gate"),STR16("%"),0,0.95,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassGate);
    {
      auto* mode=new StringListParameter(STR16("Bass Mode"),a125::rifferator::kBassMode);
      mode->appendString(STR16("FOLLOW"));
      mode->appendString(STR16("EBM"));
      mode->appendString(STR16("HYBRID"));
      parameters.addParameter(mode);
      auto* style=new StringListParameter(STR16("EBM Style"),a125::rifferator::kEbmStyle);
      style->appendString(STR16("CLASSIC"));
      style->appendString(STR16("CHROMATIC"));
      style->appendString(STR16("OCTAVE"));
      style->appendString(STR16("INDUSTRIAL"));
      style->appendString(STR16("ROLLING"));
      parameters.addParameter(style);
    }
    parameters.addParameter(STR16("Bass Enabled"),nullptr,1,1.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassEnabled);
    parameters.addParameter(STR16("Root MIDI"),nullptr,127,40.0/127.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kRootPitch);
    parameters.addParameter(STR16("Bass Velocity"),nullptr,126,99.0/126.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassVelocity);
    parameters.addParameter(STR16("Fill Intensity"),STR16("%"),0,0.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kFillIntensity);
    parameters.addParameter(STR16("Guitar Step 01"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+0);
    parameters.addParameter(STR16("Guitar Step 02"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+1);
    parameters.addParameter(STR16("Guitar Step 03"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+2);
    parameters.addParameter(STR16("Guitar Step 04"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+3);
    parameters.addParameter(STR16("Guitar Step 05"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+4);
    parameters.addParameter(STR16("Guitar Step 06"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+5);
    parameters.addParameter(STR16("Guitar Step 07"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+6);
    parameters.addParameter(STR16("Guitar Step 08"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+7);
    parameters.addParameter(STR16("Guitar Step 09"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+8);
    parameters.addParameter(STR16("Guitar Step 10"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+9);
    parameters.addParameter(STR16("Guitar Step 11"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+10);
    parameters.addParameter(STR16("Guitar Step 12"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+11);
    parameters.addParameter(STR16("Guitar Step 13"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+12);
    parameters.addParameter(STR16("Guitar Step 14"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+13);
    parameters.addParameter(STR16("Guitar Step 15"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+14);
    parameters.addParameter(STR16("Guitar Step 16"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarStepBase+15);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 01"),a125::rifferator::kGuitarArticulationBase+0);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 01"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+0);
    parameters.addParameter(STR16("Guitar Lock 01"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+0);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 02"),a125::rifferator::kGuitarArticulationBase+1);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 02"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+1);
    parameters.addParameter(STR16("Guitar Lock 02"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+1);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 03"),a125::rifferator::kGuitarArticulationBase+2);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 03"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+2);
    parameters.addParameter(STR16("Guitar Lock 03"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+2);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 04"),a125::rifferator::kGuitarArticulationBase+3);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 04"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+3);
    parameters.addParameter(STR16("Guitar Lock 04"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+3);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 05"),a125::rifferator::kGuitarArticulationBase+4);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 05"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+4);
    parameters.addParameter(STR16("Guitar Lock 05"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+4);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 06"),a125::rifferator::kGuitarArticulationBase+5);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 06"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+5);
    parameters.addParameter(STR16("Guitar Lock 06"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+5);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 07"),a125::rifferator::kGuitarArticulationBase+6);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 07"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+6);
    parameters.addParameter(STR16("Guitar Lock 07"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+6);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 08"),a125::rifferator::kGuitarArticulationBase+7);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 08"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+7);
    parameters.addParameter(STR16("Guitar Lock 08"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+7);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 09"),a125::rifferator::kGuitarArticulationBase+8);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 09"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+8);
    parameters.addParameter(STR16("Guitar Lock 09"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+8);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 10"),a125::rifferator::kGuitarArticulationBase+9);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 10"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+9);
    parameters.addParameter(STR16("Guitar Lock 10"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+9);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 11"),a125::rifferator::kGuitarArticulationBase+10);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 11"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+10);
    parameters.addParameter(STR16("Guitar Lock 11"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+10);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 12"),a125::rifferator::kGuitarArticulationBase+11);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 12"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+11);
    parameters.addParameter(STR16("Guitar Lock 12"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+11);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 13"),a125::rifferator::kGuitarArticulationBase+12);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 13"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+12);
    parameters.addParameter(STR16("Guitar Lock 13"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+12);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 14"),a125::rifferator::kGuitarArticulationBase+13);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 14"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+13);
    parameters.addParameter(STR16("Guitar Lock 14"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+13);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 15"),a125::rifferator::kGuitarArticulationBase+14);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 15"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+14);
    parameters.addParameter(STR16("Guitar Lock 15"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+14);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 16"),a125::rifferator::kGuitarArticulationBase+15);
      art->appendString(STR16("REST"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=3./5.;
      art->setNormalized(3./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 16"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+15);
    parameters.addParameter(STR16("Guitar Lock 16"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+15);
    parameters.addParameter(STR16("Drums Enabled"),nullptr,1,1.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumEnabled);
    parameters.addParameter(STR16("Kick 01"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+0);
    parameters.addParameter(STR16("Kick 02"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+1);
    parameters.addParameter(STR16("Kick 03"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+2);
    parameters.addParameter(STR16("Kick 04"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+3);
    parameters.addParameter(STR16("Kick 05"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+4);
    parameters.addParameter(STR16("Kick 06"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+5);
    parameters.addParameter(STR16("Kick 07"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+6);
    parameters.addParameter(STR16("Kick 08"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+7);
    parameters.addParameter(STR16("Kick 09"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+8);
    parameters.addParameter(STR16("Kick 10"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+9);
    parameters.addParameter(STR16("Kick 11"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+10);
    parameters.addParameter(STR16("Kick 12"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+11);
    parameters.addParameter(STR16("Kick 13"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+12);
    parameters.addParameter(STR16("Kick 14"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+13);
    parameters.addParameter(STR16("Kick 15"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+14);
    parameters.addParameter(STR16("Kick 16"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+15);
    parameters.addParameter(STR16("Snare 01"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+16);
    parameters.addParameter(STR16("Snare 02"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+17);
    parameters.addParameter(STR16("Snare 03"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+18);
    parameters.addParameter(STR16("Snare 04"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+19);
    parameters.addParameter(STR16("Snare 05"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+20);
    parameters.addParameter(STR16("Snare 06"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+21);
    parameters.addParameter(STR16("Snare 07"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+22);
    parameters.addParameter(STR16("Snare 08"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+23);
    parameters.addParameter(STR16("Snare 09"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+24);
    parameters.addParameter(STR16("Snare 10"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+25);
    parameters.addParameter(STR16("Snare 11"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+26);
    parameters.addParameter(STR16("Snare 12"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+27);
    parameters.addParameter(STR16("Snare 13"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+28);
    parameters.addParameter(STR16("Snare 14"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+29);
    parameters.addParameter(STR16("Snare 15"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+30);
    parameters.addParameter(STR16("Snare 16"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+31);
    parameters.addParameter(STR16("Hat 01"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+32);
    parameters.addParameter(STR16("Hat 02"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+33);
    parameters.addParameter(STR16("Hat 03"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+34);
    parameters.addParameter(STR16("Hat 04"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+35);
    parameters.addParameter(STR16("Hat 05"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+36);
    parameters.addParameter(STR16("Hat 06"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+37);
    parameters.addParameter(STR16("Hat 07"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+38);
    parameters.addParameter(STR16("Hat 08"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+39);
    parameters.addParameter(STR16("Hat 09"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+40);
    parameters.addParameter(STR16("Hat 10"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+41);
    parameters.addParameter(STR16("Hat 11"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+42);
    parameters.addParameter(STR16("Hat 12"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+43);
    parameters.addParameter(STR16("Hat 13"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+44);
    parameters.addParameter(STR16("Hat 14"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+45);
    parameters.addParameter(STR16("Hat 15"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+46);
    parameters.addParameter(STR16("Hat 16"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kDrumStepBase+47);
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
       magic!=0x31524641u || (version!=1 && version!=2 && version!=3 && version!=4 && version!=5 && version!=6) ||
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
    std::array<uint32_t,49> drumValues{};
    if(version>=3) {
      uint32_t enabled=0;
      if(!read32(enabled)||enabled>1)return kResultFalse;
      drumValues[0]=enabled;
      for(int i=0;i<48;++i) {
        uint32_t active=0;
        if(!read32(active)||active>1)return kResultFalse;
        drumValues[static_cast<std::size_t>(i+1)]=active;
      }
    }
    std::array<uint32_t,16> guitarActive{};
    if(version>=4) {
      for(auto& value:guitarActive)if(!read32(value)||value>1)return kResultFalse;
    }
    std::array<uint32_t,16> guitarArt{},guitarDegrees{},guitarLocks{};
    if(version>=5) {
      for(auto& value:guitarArt)if(!read32(value)||value>5)return kResultFalse;
      for(auto& value:guitarDegrees) {
        if(!read32(value))return kResultFalse;
        const auto degree=static_cast<int32_t>(value);
        if(degree<-13||degree>15)return kResultFalse;
      }
      for(auto& value:guitarLocks)if(!read32(value)||value>1)return kResultFalse;
    }
    uint32_t restoredMode=0,restoredStyle=0;
    if(version>=6 && (!read32(restoredMode)||restoredMode>2||
                      !read32(restoredStyle)||restoredStyle>4))
      return kResultFalse;
    // The processor owns the pattern. The controller only synchronizes
    // the automatable parameter values presented to the host/GUI.
    setParamNormalized(a125::rifferator::kBassMode,restoredMode/2.);
    setParamNormalized(a125::rifferator::kEbmStyle,restoredStyle/4.);
    setParamNormalized(a125::rifferator::kRootPitch,double(values[0])/127.);
    setParamNormalized(a125::rifferator::kBassVelocity,double(values[1]-1)/126.);
    setParamNormalized(a125::rifferator::kGuitarGate,double(values[3])/100.);
    setParamNormalized(a125::rifferator::kBassGate,double(values[4])/100.);
    setParamNormalized(a125::rifferator::kBassEnabled,values[6]?1.:0.);
    setParamNormalized(a125::rifferator::kFillIntensity,double(fill)/100.);
    if(version>=4) {
      for(int i=0;i<16;++i)setParamNormalized(a125::rifferator::kGuitarStepBase+i,guitarActive[static_cast<std::size_t>(i)]?1.:0.);
    }
    if(version>=5) {
      for(int i=0;i<16;++i) {
        const auto j=static_cast<std::size_t>(i);
        setParamNormalized(a125::rifferator::kGuitarArticulationBase+i,guitarArt[j]/5.);
        setParamNormalized(a125::rifferator::kGuitarDegreeBase+i,
                           (static_cast<int32_t>(guitarDegrees[j])+13)/28.);
        setParamNormalized(a125::rifferator::kGuitarLockBase+i,guitarLocks[j]?1.:0.);
      }
    }
    if(version>=3) {
      setParamNormalized(a125::rifferator::kDrumEnabled,drumValues[0]?1.:0.);
      for(int i=0;i<48;++i)setParamNormalized(a125::rifferator::kDrumStepBase+i,drumValues[static_cast<std::size_t>(i+1)]?1.:0.);
    }
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
