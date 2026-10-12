#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
#include "core/IndustrialGuitarGroove.hpp"
#include "core/RiffGeneration.hpp"
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
#include <string>
#include <cmath>

namespace {
const Steinberg::FUID processorUid(0x125A4601,0x52494646,0x45524154,0x4F520100);
const Steinberg::FUID controllerUid(0x125A4602,0x52494646,0x45524154,0x4F520100);
class Controller final : public Steinberg::Vst::EditControllerEx1,
                         public VSTGUI::VST3EditorDelegate,
                         public VSTGUI::IControlListener {
public:
  unsigned generateCount_{};
  // Both GUI commands run on the control thread, never in process().
  // Existing core generation preserves every locked step. The MIDI engine
  // receives only ordinary, stable VST3 step parameter edits.
  void runGuitarGeneration(a125::rifferator::GenerationCommand command) {
    using namespace a125::rifferator;
    std::vector<Step> current;
    current.reserve(16);
    for(int i=0;i<16;++i) {
      const bool enabled=getParamNormalized(kGuitarStepBase+i)>=0.5;
      const int art=std::clamp(static_cast<int>(std::lround(
        getParamNormalized(kGuitarArticulationBase+i)*5.)),0,5);
      const int degree=std::clamp(static_cast<int>(std::lround(
        getParamNormalized(kGuitarDegreeBase+i)*28.))-13,-13,15);
      const bool doubled=getParamNormalized(kGuitarDoubleBase+i)>=0.5;
      const bool locked=getParamNormalized(kGuitarLockBase+i)>=0.5;
      current.push_back({static_cast<Articulation>(enabled?art:0),
                         degree,doubled,locked});

    }
    RandomOptions options;
    // Distinct and reproducible (per session) seeds; no more three-preset
    // rotation. Keep V1 melody root-focused unless pitch randomization is
    // explicitly exposed as a separate user control.
    options.seed=0x125A2026u ^ ((++generateCount_)*0x9E3779B9u);
    options.densityPercent=80;
    options.pmPercent=80;
    options.noisePercent=3;
    options.harmonicPercent=2;
    options.randomRhythm=true;
    options.randomArticulation=true;
    options.randomPitch=false;
    const auto result=generateGuitar(current,options,command,30);
    const auto publish=[&](Steinberg::Vst::ParamID id,double normalized) {
      if(std::abs(getParamNormalized(id)-normalized)<1.e-9)return;
      beginEdit(id);
      setParamNormalized(id,normalized);
      performEdit(id,normalized);
      endEdit(id);
    };
    for(int i=0;i<16;++i) {
      const auto j=static_cast<std::size_t>(i);
      if(current[j].locked)continue;
      const auto& step=result[j];
      publish(kGuitarStepBase+i,step.articulation==Articulation::Off?0.:1.);
      publish(kGuitarArticulationBase+i,
              static_cast<int>(step.articulation)/5.);
      publish(kGuitarDegreeBase+i,(step.degree+13)/28.);
      // GENERATE starts a new pattern with no stale double strokes.
      // VARIATE preserves the existing stroke subdivision.
      if(command==GenerationCommand::Generate)publish(kGuitarDoubleBase+i,0.);
    }
  }
  VSTGUI::VST3Editor* activeEditor_{};
  VSTGUI::VST3Editor* viewOwner_{};
  std::array<VSTGUI::CControl*,21> commandButtons_{};
  void refreshCommands() {
    const auto selected=[&](int tag) -> bool {
      using namespace a125::rifferator;
      if(tag==701||tag==702)return zoomFactor_==(tag==701?1.:1.5);
      if(tag>=710&&tag<=712)return std::clamp(static_cast<int>(getParamNormalized(kBassMode)*2.+0.5),0,2)==tag-710;
      if(tag>=713&&tag<=717)return std::clamp(static_cast<int>(getParamNormalized(kChordsQuality)*4.+0.5),0,4)==tag-713;
      if(tag==718)return getParamNormalized(kChordsEnabled)>=0.5;
      if(tag==719)return getParamNormalized(kBassEnabled)>=0.5;
      if(tag==720)return getParamNormalized(kChordsMode)>=0.5;
      return false;
    };
    for(int tag=701;tag<=720;++tag)if(auto* button=commandButtons_[static_cast<std::size_t>(tag-700)]) {
      button->setValueNormalized(selected(tag)?1.f:0.f);
      button->invalid();
    }
  }
  std::array<VSTGUI::CControl*,16> stepOnButtons_{};
  std::array<VSTGUI::CControl*,16> lockButtons_{};
  std::array<VSTGUI::CControl*,16> doubleButtons_{};
  std::array<VSTGUI::CControl*,48> drumButtons_{};
  void refreshSwitch(Steinberg::Vst::ParamID id) {
    using namespace a125::rifferator;
    VSTGUI::CControl* control=nullptr;
    if(id>=kGuitarStepBase && id<kGuitarStepBase+16)control=stepOnButtons_[id-kGuitarStepBase];
    else if(id>=kGuitarLockBase && id<kGuitarLockBase+16)control=lockButtons_[id-kGuitarLockBase];
    else if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16)control=doubleButtons_[id-kGuitarDoubleBase];
    else if(id>=kDrumStepBase && id<kDrumStepBase+48)control=drumButtons_[id-kDrumStepBase];
    if(control){control->setValueNormalized(getParamNormalized(id)>=0.5?1.f:0.f);control->invalid();}
  }
  std::array<VSTGUI::CTextButton*,2> pmDisplays_{};
  void refreshPmDisplays() {
    const Steinberg::Vst::ParamID ids[2]{a125::rifferator::kPmMin,a125::rifferator::kPmMax};
    for(int i=0;i<2;++i)if(auto* display=pmDisplays_[static_cast<std::size_t>(i)]) {
      const int velocity=1+static_cast<int>(std::lround(getParamNormalized(ids[i])*126.));
      display->setTitle(std::to_string(velocity).c_str());display->invalid();
    }
  }
  std::array<VSTGUI::CTextButton*,16> degreeDisplays_{};
  void refreshDegree(int step) {
    if(step<0 || step>=16)return;
    if(auto* button=degreeDisplays_[static_cast<std::size_t>(step)]) {
      const int degree=std::clamp(static_cast<int>(std::lround(
        getParamNormalized(a125::rifferator::kGuitarDegreeBase+step)*28.))-13,-13,15);
      button->setTitle(std::to_string(degree).c_str());
      button->invalid();
    }
  }
  double zoomFactor_{1.};
  std::array<std::array<VSTGUI::CControl*,16>,6> guitarCells_{};
  void refreshGuitarStep(int step) {
    if(step<0||step>=16)return;
    const auto art=std::clamp(static_cast<int>(getParamNormalized(a125::rifferator::kGuitarArticulationBase+step)*5.+0.5),0,5);
    const bool enabled=getParamNormalized(a125::rifferator::kGuitarStepBase+step)>=0.5;
    for(int row=0;row<6;++row)if(auto* cell=guitarCells_[static_cast<std::size_t>(row)][static_cast<std::size_t>(step)]) {
      cell->setValueNormalized((enabled && row==art)?1.f:0.f);
      cell->invalid();
    }
  }
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID id,Steinberg::Vst::ParamValue value) override {
    using namespace a125::rifferator;
    // A host may automate step controls directly, bypassing valueChanged().
    // The processor already rejects edits to locked steps; prevent the host
    // display/controller from silently drifting away from the MIDI engine.
    int step=-1;
    if(id>=kGuitarStepBase && id<kGuitarStepBase+16)
      step=static_cast<int>(id-kGuitarStepBase);
    else if(id>=kGuitarArticulationBase && id<kGuitarArticulationBase+16)
      step=static_cast<int>(id-kGuitarArticulationBase);
    else if(id>=kGuitarDegreeBase && id<kGuitarDegreeBase+16)
      step=static_cast<int>(id-kGuitarDegreeBase);
    else if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16)
      step=static_cast<int>(id-kGuitarDoubleBase);
    if(step>=0 && getParamNormalized(kGuitarLockBase+step)>=0.5) {
      refreshGuitarStep(step);
      if(id>=kGuitarStepBase && id<kGuitarStepBase+16)refreshSwitch(id);
      if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16)refreshSwitch(id);
      if(id>=kGuitarDegreeBase && id<kGuitarDegreeBase+16)refreshDegree(step);
      return Steinberg::kResultOk;
    }
    // Match the processor's PM interval constraints even for DAW automation.
    // Otherwise the host GUI could display an invalid, non-playable range.
    if(id==a125::rifferator::kPmMin || id==a125::rifferator::kPmMax) {
      const auto other=id==a125::rifferator::kPmMin?
        a125::rifferator::kPmMax:a125::rifferator::kPmMin;
      const auto bound=EditControllerEx1::getParamNormalized(other);
      value=id==a125::rifferator::kPmMin?
        std::min(value,bound):std::max(value,bound);
    }
    const auto result=EditControllerEx1::setParamNormalized(id,value);
    if(result==Steinberg::kResultOk) {
      if(id==a125::rifferator::kPmMin || id==a125::rifferator::kPmMax)refreshPmDisplays();
      if(id==a125::rifferator::kBassMode || id==a125::rifferator::kChordsQuality || id==a125::rifferator::kChordsEnabled || id==a125::rifferator::kBassEnabled || id==a125::rifferator::kChordsMode) refreshCommands();
      if(id>=a125::rifferator::kGuitarStepBase && id<a125::rifferator::kGuitarStepBase+16)refreshGuitarStep(static_cast<int>(id-a125::rifferator::kGuitarStepBase));
      if(id>=a125::rifferator::kGuitarArticulationBase && id<a125::rifferator::kGuitarArticulationBase+16)refreshGuitarStep(static_cast<int>(id-a125::rifferator::kGuitarArticulationBase));
      if((id>=a125::rifferator::kGuitarStepBase && id<a125::rifferator::kGuitarStepBase+16) ||
         (id>=a125::rifferator::kGuitarLockBase && id<a125::rifferator::kGuitarLockBase+16) ||
         (id>=a125::rifferator::kGuitarDoubleBase && id<a125::rifferator::kGuitarDoubleBase+16) ||
         (id>=a125::rifferator::kDrumStepBase && id<a125::rifferator::kDrumStepBase+48))refreshSwitch(id);
      if(id>=a125::rifferator::kGuitarDegreeBase && id<a125::rifferator::kGuitarDegreeBase+16)refreshDegree(static_cast<int>(id-a125::rifferator::kGuitarDegreeBase));
    }
    return result;
  }
  void willClose(VSTGUI::VST3Editor* editor) override {
    if(activeEditor_==editor)activeEditor_=nullptr;
    if(viewOwner_==editor) {
      viewOwner_=nullptr;
      // Never retain pointers after this editor destroys its views.
      for(auto& row:guitarCells_)row.fill(nullptr);
      commandButtons_.fill(nullptr);
      degreeDisplays_.fill(nullptr);
      stepOnButtons_.fill(nullptr);
      lockButtons_.fill(nullptr);
      doubleButtons_.fill(nullptr);
      pmDisplays_.fill(nullptr);
      drumButtons_.fill(nullptr);
    }
  }
  VSTGUI::CView* verifyView(VSTGUI::CView* view,
                            const VSTGUI::UIAttributes&,
                            const VSTGUI::IUIDescription*,
                            VSTGUI::VST3Editor* editor) override {
    if(viewOwner_!=editor) {
      for(auto& row:guitarCells_)row.fill(nullptr);
      commandButtons_.fill(nullptr);
      degreeDisplays_.fill(nullptr);
      stepOnButtons_.fill(nullptr);
      lockButtons_.fill(nullptr);
      doubleButtons_.fill(nullptr);
      pmDisplays_.fill(nullptr);
      drumButtons_.fill(nullptr);
      viewOwner_=editor;
    }
    if(auto* control=dynamic_cast<VSTGUI::CControl*>(view)) {
      if((control->getTag()>=500 && control->getTag()<596) || (control->getTag()>=700 && control->getTag()<=722) || (control->getTag()>=800 && control->getTag()<832) || (control->getTag()>=200 && control->getTag()<248) || (control->getTag()>=300 && control->getTag()<316) || (control->getTag()>=432 && control->getTag()<464) || (control->getTag()>=880 && control->getTag()<884))
        control->setListener(this);
      const auto tag=control->getTag();
      if(tag>=884 && tag<886) {
        pmDisplays_[static_cast<std::size_t>(tag-884)]=dynamic_cast<VSTGUI::CTextButton*>(control);
        refreshPmDisplays();
      }
      if(tag>=200 && tag<248) {drumButtons_[tag-200]=control;refreshSwitch(tag);}
      if(tag>=300 && tag<316) {stepOnButtons_[tag-300]=control;refreshSwitch(tag);}
      if(tag>=432 && tag<448) {lockButtons_[tag-432]=control;refreshSwitch(tag);}
      if(tag>=448 && tag<464) {doubleButtons_[tag-448]=control;refreshSwitch(tag);}
      if(control->getTag()>=850 && control->getTag()<866) {
        degreeDisplays_[static_cast<std::size_t>(control->getTag()-850)]=dynamic_cast<VSTGUI::CTextButton*>(control);
        refreshDegree(control->getTag()-850);
      }
      if(control->getTag()>=701 && control->getTag()<=720) {
        commandButtons_[static_cast<std::size_t>(control->getTag()-700)]=control;
        refreshCommands();
      }
      if(control->getTag()>=500 && control->getTag()<596) {
        const int tag=control->getTag()-500;
        guitarCells_[static_cast<std::size_t>(tag/16)][static_cast<std::size_t>(tag%16)]=control;
        refreshGuitarStep(tag%16);
      }
    }
    return view;
  }
  void valueChanged(VSTGUI::CControl* control) override {
    if(!control)return;
    const int command=control->getTag();
    // A second click on the selected articulation clears the step.
    // Clicking an unselected cell OFF must not erase another selection.
    if(command>=500 && command<596 && control->getValueNormalized()<=0.5f) {
      const int step=(command-500)%16;
      const int art=(command-500)/16;
      using namespace a125::rifferator;
      if(getParamNormalized(kGuitarLockBase+step)>=0.5) {
        refreshGuitarStep(step);return;
      }
      const int current=std::clamp(static_cast<int>(
        std::lround(getParamNormalized(kGuitarArticulationBase+step)*5.)),0,5);
      if(getParamNormalized(kGuitarStepBase+step)>=0.5 && current==art) {
        const auto id=kGuitarStepBase+step;
        beginEdit(id);setParamNormalized(id,0.);performEdit(id,0.);endEdit(id);
      }
      refreshGuitarStep(step);
      return;
    }
    if((command>=200 && command<248) || (command>=300 && command<316) ||
       (command>=432 && command<464)) {
      const auto id=static_cast<Steinberg::Vst::ParamID>(command);
      // A locked guitar step rejects both ON/OFF and DOUBLE edits.
      // Restore the GUI immediately as the processor rejects those edits too.
      if(((command>=300 && command<316) || (command>=448 && command<464)) &&
         getParamNormalized(a125::rifferator::kGuitarLockBase+((command>=448?command-448:command-300)))>=0.5) {
        refreshSwitch(id);
        return;
      }
      const double value=control->getValueNormalized()>=0.5?1.:0.;
      beginEdit(id);setParamNormalized(id,value);performEdit(id,value);endEdit(id);
      return;
    }
    if(control->getValueNormalized()<=0.5f)return;
    if(command>=880 && command<884) {
      const bool isMin=command<882;
      const auto id=isMin?a125::rifferator::kPmMin:a125::rifferator::kPmMax;
      const auto other=isMin?a125::rifferator::kPmMax:a125::rifferator::kPmMin;
      int velocity=1+static_cast<int>(std::lround(getParamNormalized(id)*126.));
      const int limit=1+static_cast<int>(std::lround(getParamNormalized(other)*126.));
      velocity=std::clamp(velocity+((command%2)==0?-1:1),1,127);
      velocity=isMin?std::min(velocity,limit):std::max(velocity,limit);
      const double value=static_cast<double>(velocity-1)/126.;
      beginEdit(id);setParamNormalized(id,value);performEdit(id,value);endEdit(id);
      return;
    }
    if(command>=800 && command<832) {
      const int step=(command-800)%16;
      const auto lock=a125::rifferator::kGuitarLockBase+step;
      if(getParamNormalized(lock)>=0.5)return;
      const auto id=a125::rifferator::kGuitarDegreeBase+step;
      const int degree=std::clamp(static_cast<int>(std::lround(getParamNormalized(id)*28.))-13,-13,15);
      const int next=std::clamp(degree+(command<816?-1:1),-13,15);
      if(next==degree)return;
      const double value=static_cast<double>(next+13)/28.;
      beginEdit(id);setParamNormalized(id,value);performEdit(id,value);endEdit(id);
      return;
    }
    if(command==721) {
      // CLEAR PATTERN preserves locked cells and every global setting.
      for(int step=0;step<16;++step) {
        using namespace a125::rifferator;
        if(getParamNormalized(kGuitarLockBase+step)>=0.5)continue;
        const auto active=kGuitarStepBase+step;
        if(getParamNormalized(active)>=0.5) {
          beginEdit(active);setParamNormalized(active,0.);
          performEdit(active,0.);endEdit(active);
        }
        const auto doubled=kGuitarDoubleBase+step;
        if(getParamNormalized(doubled)>=0.5) {
          beginEdit(doubled);setParamNormalized(doubled,0.);
          performEdit(doubled,0.);endEdit(doubled);
        }
        refreshGuitarStep(step);
      }
      return;
    }
    if(command==701 || command==702) {
      zoomFactor_=command==701?1.:1.5;
      // Use the attached view owner when available, not a potentially stale
      // pointer left by a previously opened host editor.
      auto* editor=viewOwner_?viewOwner_:activeEditor_;
      if(editor && std::abs(editor->getZoomFactor()-zoomFactor_)>1.e-9)
        editor->setZoomFactor(zoomFactor_);
      refreshCommands();
      return;
    }
    const auto send=[&](Steinberg::Vst::ParamID id,double value) {
      beginEdit(id);
      setParamNormalized(id,value);
      performEdit(id,value);
      endEdit(id);
    };
    if(command>=710 && command<=712) {
      send(a125::rifferator::kBassMode,static_cast<double>(command-710)/2.);
      return;
    }
    if(command>=713 && command<=717) {
      send(a125::rifferator::kChordsQuality,static_cast<double>(command-713)/4.);
      return;
    }
    if(command==718) {
      const auto id=a125::rifferator::kChordsEnabled;
      send(id,getParamNormalized(id)>=0.5?0.:1.);
      return;
    }
    if(command==719) {
      const auto id=a125::rifferator::kBassEnabled;
      send(id,getParamNormalized(id)>=0.5?0.:1.);
      return;
    }
    if(command==720) {
      const auto id=a125::rifferator::kChordsMode;
      send(id,getParamNormalized(id)>=0.5?0.:1.);
      return;
    }
    if(command==700 || command==722) {
      runGuitarGeneration(command==700?
        a125::rifferator::GenerationCommand::Generate:
        a125::rifferator::GenerationCommand::Variate);
      return;
    }
    const int tag=control->getTag();
    if(tag<500||tag>=596)return;
    const int step=(tag-500)%16;
    const int art=(tag-500)/16;
    const auto id=a125::rifferator::kGuitarArticulationBase+step;
    if(getParamNormalized(a125::rifferator::kGuitarLockBase+step)>=0.5)return;
    // STOP is an active guitar-only NoteOff command; an empty step is OFF.
    // Both parameters must be published, otherwise a previously silent
    // generated step remains silent after selecting Palm Mute or OPEN.
    const auto activeId=a125::rifferator::kGuitarStepBase+step;
    const double enabled=1.;
    beginEdit(activeId);
    setParamNormalized(activeId,enabled);
    performEdit(activeId,enabled);
    endEdit(activeId);
    beginEdit(id);
    setParamNormalized(id,static_cast<double>(art)/5.);
    performEdit(id,static_cast<double>(art)/5.);
    endEdit(id);
    refreshGuitarStep(step);
  }
  Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override {
    if(name && std::strcmp(name,Steinberg::Vst::ViewType::kEditor)==0) {
      auto* editor=new VSTGUI::AspectRatioVST3Editor(this,"RifferatorView","rifferator.uidesc");
      editor->setMinZoomFactor(1.0);
      editor->setAllowedZoomFactors({1.0,1.5});
      editor->setDelegate(this);
      editor->setZoomFactor(zoomFactor_);
      activeEditor_=editor;
      return editor;
    }
    return nullptr;
  }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override {
    using namespace Steinberg::Vst;
    if(EditControllerEx1::initialize(context)!=Steinberg::kResultOk) return Steinberg::kResultFalse;
    parameters.addParameter(STR16("PM MIN"),nullptr,126,29.0/126.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kPmMin);
    parameters.addParameter(STR16("PM MAX"),nullptr,126,49.0/126.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kPmMax);
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
    {
      auto* quality=new StringListParameter(STR16("Chord Quality"),a125::rifferator::kChordsQuality);
      quality->appendString(STR16("MINOR"));
      quality->appendString(STR16("MAJOR"));
      quality->appendString(STR16("POWER"));
      quality->appendString(STR16("SUS2"));
      quality->appendString(STR16("SUS4"));
      parameters.addParameter(quality);
    }
    parameters.addParameter(STR16("Chords Enabled"),nullptr,1,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kChordsEnabled);
    {
      auto* mode=new StringListParameter(STR16("Chords Output"),a125::rifferator::kChordsMode);
      mode->appendString(STR16("CHORD"));
      mode->appendString(STR16("ROOT"));
      parameters.addParameter(mode);
    }
    parameters.addParameter(STR16("Harmony Bar 01"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+0);
    parameters.addParameter(STR16("Harmony Bar 02"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+1);
    parameters.addParameter(STR16("Harmony Bar 03"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+2);
    parameters.addParameter(STR16("Harmony Bar 04"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+3);
    parameters.addParameter(STR16("Harmony Bar 05"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+4);
    parameters.addParameter(STR16("Harmony Bar 06"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+5);
    parameters.addParameter(STR16("Harmony Bar 07"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+6);
    parameters.addParameter(STR16("Harmony Bar 08"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+7);
    parameters.addParameter(STR16("Harmony Bar 09"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+8);
    parameters.addParameter(STR16("Harmony Bar 10"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+9);
    parameters.addParameter(STR16("Harmony Bar 11"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+10);
    parameters.addParameter(STR16("Harmony Bar 12"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+11);
    parameters.addParameter(STR16("Harmony Bar 13"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+12);
    parameters.addParameter(STR16("Harmony Bar 14"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+13);
    parameters.addParameter(STR16("Harmony Bar 15"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+14);
    parameters.addParameter(STR16("Harmony Bar 16"),nullptr,128,1.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kHybridRootBase+15);
    parameters.addParameter(STR16("Bass Enabled"),nullptr,1,1.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassEnabled);
    parameters.addParameter(STR16("Root MIDI"),nullptr,127,40.0/127.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kRootPitch);
    parameters.addParameter(STR16("Bass Velocity"),nullptr,126,99.0/126.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kBassVelocity);
    parameters.addParameter(STR16("Fill Intensity"),STR16("%"),0,0.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kFillIntensity);
    for(int i=0;i<16;++i)
      parameters.addParameter(STR16("Guitar DOUBLE"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDoubleBase+i);
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
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 01"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+0);
    parameters.addParameter(STR16("Guitar Lock 01"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+0);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 02"),a125::rifferator::kGuitarArticulationBase+1);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 02"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+1);
    parameters.addParameter(STR16("Guitar Lock 02"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+1);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 03"),a125::rifferator::kGuitarArticulationBase+2);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 03"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+2);
    parameters.addParameter(STR16("Guitar Lock 03"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+2);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 04"),a125::rifferator::kGuitarArticulationBase+3);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 04"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+3);
    parameters.addParameter(STR16("Guitar Lock 04"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+3);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 05"),a125::rifferator::kGuitarArticulationBase+4);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 05"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+4);
    parameters.addParameter(STR16("Guitar Lock 05"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+4);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 06"),a125::rifferator::kGuitarArticulationBase+5);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 06"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+5);
    parameters.addParameter(STR16("Guitar Lock 06"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+5);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 07"),a125::rifferator::kGuitarArticulationBase+6);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 07"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+6);
    parameters.addParameter(STR16("Guitar Lock 07"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+6);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 08"),a125::rifferator::kGuitarArticulationBase+7);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 08"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+7);
    parameters.addParameter(STR16("Guitar Lock 08"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+7);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 09"),a125::rifferator::kGuitarArticulationBase+8);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 09"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+8);
    parameters.addParameter(STR16("Guitar Lock 09"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+8);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 10"),a125::rifferator::kGuitarArticulationBase+9);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 10"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+9);
    parameters.addParameter(STR16("Guitar Lock 10"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+9);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 11"),a125::rifferator::kGuitarArticulationBase+10);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 11"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+10);
    parameters.addParameter(STR16("Guitar Lock 11"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+10);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 12"),a125::rifferator::kGuitarArticulationBase+11);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 12"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+11);
    parameters.addParameter(STR16("Guitar Lock 12"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+11);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 13"),a125::rifferator::kGuitarArticulationBase+12);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 13"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+12);
    parameters.addParameter(STR16("Guitar Lock 13"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+12);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 14"),a125::rifferator::kGuitarArticulationBase+13);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 14"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+13);
    parameters.addParameter(STR16("Guitar Lock 14"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+13);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 15"),a125::rifferator::kGuitarArticulationBase+14);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
      parameters.addParameter(art);
    }
    parameters.addParameter(STR16("Guitar Degree 15"),nullptr,28,14./28.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDegreeBase+14);
    parameters.addParameter(STR16("Guitar Lock 15"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLockBase+14);
    {
      auto* art=new StringListParameter(STR16("Guitar Art 16"),a125::rifferator::kGuitarArticulationBase+15);
      art->appendString(STR16("STOP"));
      art->appendString(STR16("NOISE"));
      art->appendString(STR16("PALM"));
      art->appendString(STR16("SOFT"));
      art->appendString(STR16("OPEN"));
      art->appendString(STR16("HARM"));
      art->getInfo().defaultNormalizedValue=2./5.;
      art->setNormalized(2./5.);
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
  // VST3 stores edit-controller state independently from the processor's
  // musical project snapshot. Preserve the user's 100/150% GUI zoom across
  // Studio One project reopen without changing the existing processor ABI.
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* stream) override {
    using namespace Steinberg;
    if(!stream)return kResultFalse;
    constexpr uint32 magic=0x315A4652u; // "RFZ1" (little endian)
    constexpr uint32 version=1u;
    const uint32 zoom=zoomFactor_>=1.25?150u:100u;
    const uint32 fields[3]{magic,version,zoom};
    for(auto field:fields) {
      int32 written=0;
      if(stream->write(&field,sizeof(field),&written)!=kResultOk ||
         written!=sizeof(field))return kResultFalse;
    }
    return kResultOk;
  }
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* stream) override {
    using namespace Steinberg;
    if(!stream)return kResultFalse;
    uint32 fields[3]{};
    for(auto& field:fields) {
      int32 read=0;
      if(stream->read(&field,sizeof(field),&read)!=kResultOk ||
         read!=sizeof(field))return kResultFalse;
    }
    if(fields[0]!=0x315A4652u || fields[1]!=1u ||
       (fields[2]!=100u && fields[2]!=150u))return kResultFalse;
    // Validate everything before publishing any editor/UI changes.
    zoomFactor_=fields[2]==150u?1.5:1.0;
    auto* editor=viewOwner_?viewOwner_:activeEditor_;
    if(editor && std::abs(editor->getZoomFactor()-zoomFactor_)>1.e-9)
      editor->setZoomFactor(zoomFactor_);
    refreshCommands();
    return kResultOk;
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
       magic!=0x31524641u || (version!=1 && version!=2 && version!=3 && version!=4 && version!=5 && version!=6 && version!=7 && version!=8 && version!=9) ||
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
    std::array<uint32_t,32> storedDouble{},storedArt{},storedLocks{};
    std::array<int32_t,32> storedDegree{};
    for(uint32_t i=0;i<count;++i) {
      std::array<uint32_t,4> step{};
      for(auto& value:step)if(!read32(value))return kResultFalse;
      const int32_t degree=static_cast<int32_t>(step[1]);
      if(step[0]>5 || degree<-13 || degree>15 ||
         step[2]>1 || step[3]>1)return kResultFalse;
      storedDouble[i]=step[2];storedArt[i]=step[0];storedDegree[i]=degree;storedLocks[i]=step[3];
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
    uint32_t chordsEnabled=0,chordsMode=0;
    std::array<uint32_t,16> harmonyRoots{};
    harmonyRoots.fill(128);
    if(version>=7) {
      if(!read32(chordsEnabled)||chordsEnabled>1||
         !read32(chordsMode)||chordsMode>1)return kResultFalse;
      for(auto& root:harmonyRoots)if(!read32(root)||root>128)return kResultFalse;
    }
    uint32_t quality=0;
    if(version>=8 && (!read32(quality)||quality>4))return kResultFalse;
    uint32_t pmMin=30,pmMax=50;
    if(version>=9 && (!read32(pmMin)||!read32(pmMax)||
        pmMin<1||pmMax>127||pmMin>pmMax))return kResultFalse;
    // Set the bound that expands the current interval first; this also
    // restores ranges wholly above or below the default 30..50.
    const auto minId=a125::rifferator::kPmMin,maxId=a125::rifferator::kPmMax;
    const auto restoreMin=[&](){setParamNormalized(minId,(pmMin-1)/126.);};
    const auto restoreMax=[&](){setParamNormalized(maxId,(pmMax-1)/126.);};
    if((pmMax-1)/126. < getParamNormalized(minId)) {
      restoreMin();restoreMax();
    } else {
      restoreMax();restoreMin();
    }
    // An older project may have left steps locked in the current controller.
    // After validating the incoming state, clear existing locks first so that
    // full project recall can replace every step even if it was locked before.
    // The incoming lock flags are restored only after their step values.
    for(int i=0;i<16;++i)
      setParamNormalized(a125::rifferator::kGuitarLockBase+i,0.);
    // The processor owns the pattern. The controller only synchronizes
    // the automatable parameter values presented to the host/GUI.
    setParamNormalized(a125::rifferator::kChordsQuality,quality/4.);
    setParamNormalized(a125::rifferator::kChordsEnabled,chordsEnabled?1.:0.);
    setParamNormalized(a125::rifferator::kChordsMode,chordsMode?1.:0.);
    for(int i=0;i<16;++i)
      setParamNormalized(a125::rifferator::kHybridRootBase+i,harmonyRoots[static_cast<std::size_t>(i)]/128.);
    setParamNormalized(a125::rifferator::kBassMode,restoredMode/2.);
    setParamNormalized(a125::rifferator::kEbmStyle,restoredStyle/4.);
    setParamNormalized(a125::rifferator::kRootPitch,double(values[0])/127.);
    setParamNormalized(a125::rifferator::kBassVelocity,double(values[1]-1)/126.);
    setParamNormalized(a125::rifferator::kGuitarGate,double(values[3])/100.);
    setParamNormalized(a125::rifferator::kBassGate,double(values[4])/100.);
    setParamNormalized(a125::rifferator::kBassEnabled,values[6]?1.:0.);
    setParamNormalized(a125::rifferator::kFillIntensity,double(fill)/100.);
    for(int i=0;i<16;++i)
      setParamNormalized(a125::rifferator::kGuitarDoubleBase+i,
        storedDouble[static_cast<std::size_t>(i%count)]?1.:0.);
    for(int i=0;i<16;++i) {
      const auto j=static_cast<std::size_t>(i%count);
      const bool enabled=version>=4?guitarActive[static_cast<std::size_t>(i)]!=0:storedArt[j]!=0;
      setParamNormalized(a125::rifferator::kGuitarStepBase+i,enabled?1.:0.);
    }
    for(int i=0;i<16;++i) {
      const auto j=static_cast<std::size_t>(i);
      const auto legacy=static_cast<std::size_t>(i%count);
      const int32_t degree=version>=5?static_cast<int32_t>(guitarDegrees[j]):storedDegree[legacy];
      setParamNormalized(a125::rifferator::kGuitarArticulationBase+i,
        (version>=5?guitarArt[j]:storedArt[legacy])/5.);
      setParamNormalized(a125::rifferator::kGuitarDegreeBase+i,(degree+13)/28.);
      setParamNormalized(a125::rifferator::kGuitarLockBase+i,
        (version>=5?guitarLocks[j]:storedLocks[legacy])?1.:0.);
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
