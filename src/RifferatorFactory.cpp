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
#include "vstgui/lib/controls/ctextedit.h"
#include "RifferatorNumericEdit.hpp"
#include "RifferatorState.hpp"
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
      // The core Step::Off does not distinguish an intentionally active
      // STOP command from an empty cell. VARIATE must not silently erase
      // an unchanged STOP (the note-off command) just because the core
      // proposal left that step's articulation unchanged.
      const bool preserveStop=command==GenerationCommand::Variate &&
        current[j].articulation==Articulation::Off &&
        getParamNormalized(kGuitarStepBase+i)>=0.5 &&
        step.articulation==Articulation::Off;
      publish(kGuitarStepBase+i,
              (step.articulation!=Articulation::Off || preserveStop)?1.:0.);
      publish(kGuitarArticulationBase+i,
              static_cast<int>(step.articulation)/5.);
      publish(kGuitarDegreeBase+i,(step.degree+13)/28.);
      // GENERATE starts a new pattern with no stale double strokes.
      // VARIATE preserves the existing stroke subdivision.
      if(command==GenerationCommand::Generate)publish(kGuitarDoubleBase+i,0.);
      if(command==GenerationCommand::Generate)publish(kGuitarLengthBase+i,0.);
    }
  }
  unsigned drumGenerationCount_{};



  VSTGUI::VST3Editor* activeEditor_{};
  VSTGUI::VST3Editor* viewOwner_{};
  // Guitar-only editor: no secondary page.
  std::array<VSTGUI::CControl*,25> commandButtons_{};
  void refreshCommands() {
    for(int tag:{701,702}) if(auto* c=commandButtons_[static_cast<std::size_t>(tag-700)]){
      c->setValueNormalized(zoomFactor_==(tag==701?1.:1.5)?1.f:0.f);
      c->invalid();
    }
  }
  std::array<VSTGUI::CControl*,16> stepOnButtons_{};
  std::array<VSTGUI::CControl*,16> lockButtons_{};
  std::array<VSTGUI::CControl*,16> doubleButtons_{};
  std::array<VSTGUI::CControl*,16> lengthButtons_{};
  void refreshSwitch(Steinberg::Vst::ParamID id) {
    using namespace a125::rifferator;
    VSTGUI::CControl* control=nullptr;
    if(id>=kGuitarStepBase && id<kGuitarStepBase+16)control=stepOnButtons_[id-kGuitarStepBase];
    else if(id>=kGuitarLockBase && id<kGuitarLockBase+16)control=lockButtons_[id-kGuitarLockBase];
    else if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16)control=doubleButtons_[id-kGuitarDoubleBase];
    else if(id>=kGuitarLengthBase && id<kGuitarLengthBase+16)control=lengthButtons_[id-kGuitarLengthBase];
    if(control){
      const bool on=getParamNormalized(id)>=0.5;
      control->setValueNormalized(on?1.f:0.f);
      if(id>=kGuitarLengthBase && id<kGuitarLengthBase+16)
        if(auto* button=dynamic_cast<VSTGUI::CTextButton*>(control))
          button->setTitle(on?"1/8":"1/16");
      control->invalid();
    }
  }
  // One-to-one text-edit tags: 912 Root, 913 Gate, 915 Fill.
  std::array<VSTGUI::CTextEdit*,3> musicalDisplays_{};
  std::array<VSTGUI::CTextEdit*,2> pmDisplays_{};
  static Steinberg::Vst::ParamID musicalParam(int index) noexcept {
    using namespace a125::rifferator;
    constexpr Steinberg::Vst::ParamID ids[]{kRootPitch,kGuitarGate,kFillIntensity};
    return ids[std::clamp(index,0,2)];
  }
  static int musicalValue(int index,double normalized) noexcept {
    return std::clamp(static_cast<int>(std::lround(std::clamp(normalized,0.,1.)*(index==0?127.:100.))),0,index==0?127:100);
  }
  static double musicalNormalized(int index,int value) noexcept {
    return static_cast<double>(std::clamp(value,0,index==0?127:100))/(index==0?127.:100.);
  }
  void applyMusicalStepCommand(int command) {
    const int index=(command==900||command==901)?0:
      (command==902||command==903)?1:(command==906||command==907)?2:-1;
    if(index<0)return;
    const auto id=musicalParam(index);
    const int old=musicalValue(index,getParamNormalized(id));
    const int next=std::clamp(old+(command%2==0?-1:1),0,index==0?127:100);
    if(next==old)return;
    const auto norm=musicalNormalized(index,next);
    beginEdit(id);setParamNormalized(id,norm);performEdit(id,norm);endEdit(id);
  }
  void refreshMusicalDisplays() {
    for(int i=0;i<3;++i)if(auto* edit=musicalDisplays_[static_cast<std::size_t>(i)]) {
      const int value=musicalValue(i,getParamNormalized(musicalParam(i)));
      const std::string label=i==0?a125::rifferator::noteName(value):std::to_string(value)+"%";
      edit->setText(label.c_str());edit->invalid();
    }
  }
  void refreshPmDisplays() {
    const Steinberg::Vst::ParamID ids[2]{a125::rifferator::kPmMin,a125::rifferator::kPmMax};
    for(int i=0;i<2;++i)if(auto* edit=pmDisplays_[static_cast<std::size_t>(i)]) {
      const int value=1+static_cast<int>(std::lround(getParamNormalized(ids[i])*126.));
      edit->setText(std::to_string(value).c_str());edit->invalid();
    }
  }
  bool editTextValue(int tag,const std::string& text) {
    using namespace a125::rifferator;
    int value=0;
    Steinberg::Vst::ParamID id=0;
    if(tag==912||tag==913||tag==915) {
      const int index=tag==912?0:tag==913?1:2;
      if(!(index==0?parseNote(text,value):parseInteger(text,value))) {
        refreshMusicalDisplays();return true;
      }
      value=std::clamp(value,0,index==0?127:100);
      id=musicalParam(index);
      const auto norm=musicalNormalized(index,value);
      beginEdit(id);setParamNormalized(id,norm);performEdit(id,norm);endEdit(id);
      refreshMusicalDisplays();return true;
    }
    if(tag==884||tag==885) {
      if(!parseInteger(text,value)) {refreshPmDisplays();return true;}
      id=tag==884?kPmMin:kPmMax;
      const auto other=id==kPmMin?kPmMax:kPmMin;
      const int limit=1+static_cast<int>(std::lround(getParamNormalized(other)*126.));
      value=std::clamp(value,1,127);
      value=id==kPmMin?std::min(value,limit):std::max(value,limit);
      const auto norm=(value-1)/126.;
      beginEdit(id);setParamNormalized(id,norm);performEdit(id,norm);endEdit(id);
      refreshPmDisplays();return true;
    }
    if(tag>=850&&tag<866) {
      const int step=tag-850;
      if(getParamNormalized(kGuitarLockBase+step)>=.5||!parseInteger(text,value)) {
        refreshDegree(step);return true;
      }
      value=std::clamp(value,-13,15);
      id=kGuitarDegreeBase+step;
      const auto norm=(value+13)/28.;
      beginEdit(id);setParamNormalized(id,norm);performEdit(id,norm);endEdit(id);
      refreshDegree(step);return true;
    }
    return false;
  }
  std::array<VSTGUI::CTextEdit*,16> degreeDisplays_{};
  void refreshDegree(int step) {
    if(step<0 || step>=16)return;
    if(auto* button=degreeDisplays_[static_cast<std::size_t>(step)]) {
      const int degree=std::clamp(static_cast<int>(std::lround(
        getParamNormalized(a125::rifferator::kGuitarDegreeBase+step)*28.))-13,-13,15);
      button->setText(std::to_string(degree).c_str());
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
    else if(id>=kGuitarLengthBase && id<kGuitarLengthBase+16)
      step=static_cast<int>(id-kGuitarLengthBase);
    if(step>=0 && getParamNormalized(kGuitarLockBase+step)>=0.5) {
      refreshGuitarStep(step);
      if(id>=kGuitarStepBase && id<kGuitarStepBase+16)refreshSwitch(id);
      if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16)refreshSwitch(id);
      if(id>=kGuitarLengthBase && id<kGuitarLengthBase+16)refreshSwitch(id);
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
      for(int i=0;i<3;++i)if(id==musicalParam(i)) {
        refreshMusicalDisplays();break;
      }
      if(id>=a125::rifferator::kGuitarStepBase && id<a125::rifferator::kGuitarStepBase+16)refreshGuitarStep(static_cast<int>(id-a125::rifferator::kGuitarStepBase));
      if(id>=a125::rifferator::kGuitarArticulationBase && id<a125::rifferator::kGuitarArticulationBase+16)refreshGuitarStep(static_cast<int>(id-a125::rifferator::kGuitarArticulationBase));
      if((id>=a125::rifferator::kGuitarStepBase && id<a125::rifferator::kGuitarStepBase+16) ||
         (id>=a125::rifferator::kGuitarLockBase && id<a125::rifferator::kGuitarLockBase+16) ||
         (id>=a125::rifferator::kGuitarDoubleBase && id<a125::rifferator::kGuitarDoubleBase+16) ||
         (id>=a125::rifferator::kGuitarLengthBase && id<a125::rifferator::kGuitarLengthBase+16))refreshSwitch(id);
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
      lengthButtons_.fill(nullptr);
      pmDisplays_.fill(nullptr);
      musicalDisplays_.fill(nullptr);
    }
  }
  // Keep host context-menu zoom, header buttons, and persisted controller
  // zoom state synchronized. This callback is invoked by VST3Editor when
  // its internal zoom genuinely changes, including via the native menu.
  void onZoomChanged(VSTGUI::VST3Editor*,double factor) override {
    if(std::isfinite(factor) && factor>0.) {
      zoomFactor_=factor>=1.25?1.5:1.;
      refreshCommands();
    }
  }
  VSTGUI::CView* verifyView(VSTGUI::CView* view,
                            const VSTGUI::UIAttributes& attributes,
                            const VSTGUI::IUIDescription*,
                            VSTGUI::VST3Editor* editor) override {
    if(viewOwner_!=editor) {
      for(auto& row:guitarCells_)row.fill(nullptr);
      commandButtons_.fill(nullptr);
      degreeDisplays_.fill(nullptr);
      stepOnButtons_.fill(nullptr);
      lockButtons_.fill(nullptr);
      doubleButtons_.fill(nullptr);
      lengthButtons_.fill(nullptr);
      pmDisplays_.fill(nullptr);
      musicalDisplays_.fill(nullptr);
      viewOwner_=editor;
    }
    if(auto* control=dynamic_cast<VSTGUI::CControl*>(view)) {
      const auto tag=control->getTag();
      if((tag>=500&&tag<596)||(tag==700||tag==701||tag==702||tag==721||tag==722)||
         (tag>=300&&tag<316)||(tag>=432&&tag<480)||
         (tag>=800&&tag<832)||(tag>=850&&tag<866)||
         (tag>=880&&tag<886)||(tag==900||tag==901||tag==902||tag==903||tag==906||tag==907)||
         tag==912||tag==913||tag==915)
        control->setListener(this);
      if(tag==912||tag==913||tag==915) {
        musicalDisplays_[static_cast<std::size_t>(tag==912?0:tag==913?1:2)]=
          dynamic_cast<VSTGUI::CTextEdit*>(control);
        refreshMusicalDisplays();
      }
      if(tag>=884 && tag<886) {
        pmDisplays_[static_cast<std::size_t>(tag-884)]=dynamic_cast<VSTGUI::CTextEdit*>(control);
        refreshPmDisplays();
      }
      if(tag>=300 && tag<316) {stepOnButtons_[tag-300]=control;refreshSwitch(tag);}
      if(tag>=432 && tag<448) {lockButtons_[tag-432]=control;refreshSwitch(tag);}
      if(tag>=448 && tag<464) {doubleButtons_[tag-448]=control;refreshSwitch(tag);}
      if(tag>=464 && tag<480) {lengthButtons_[tag-464]=control;refreshSwitch(tag);}
      if(control->getTag()>=850 && control->getTag()<866) {
        degreeDisplays_[static_cast<std::size_t>(control->getTag()-850)]=dynamic_cast<VSTGUI::CTextEdit*>(control);
        refreshDegree(control->getTag()-850);
      }
      if(control->getTag()==701 || control->getTag()==702) {
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
    if(auto* edit=dynamic_cast<VSTGUI::CTextEdit*>(control))
      if(editTextValue(command,edit->getText().getString()))return;
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
    if((command>=300 && command<316) ||
       (command>=432 && command<480)) {
      const auto id=static_cast<Steinberg::Vst::ParamID>(command);
      // Locked steps reject ON/OFF, DOUBLE and NOTE LENGTH edits.
      if(((command>=300 && command<316) || (command>=448 && command<480)) &&
         getParamNormalized(a125::rifferator::kGuitarLockBase+
           (command>=464?command-464:command>=448?command-448:command-300))>=0.5) {
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
        const auto length=kGuitarLengthBase+step;
        if(getParamNormalized(length)>=0.5) {
          beginEdit(length);setParamNormalized(length,0.);
          performEdit(length,0.);endEdit(length);
        }
        refreshGuitarStep(step);
      }
      return;
    }

    const auto send=[&](Steinberg::Vst::ParamID id,double value) {
      beginEdit(id);
      setParamNormalized(id,value);
      performEdit(id,value);
      endEdit(id);
    };
    if(command==900||command==901||command==902||command==903||command==906||command==907) {
      applyMusicalStepCommand(command);
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
      // AspectRatioVST3Editor recalculates zoom during onSize() and can
      // overwrite the factor requested by these fixed 100/150% buttons.
      // Normal VST3Editor owns its requested zoom and resizes through
      // VSTGUI's ordinary host IPlugFrame::resizeView handshake.
      auto* editor=new VSTGUI::VST3Editor(this,"RifferatorView","rifferator.uidesc");
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
    parameters.addParameter(STR16("PM MAX"),nullptr,126,39.0/126.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kPmMax);
    parameters.addParameter(STR16("Guitar Gate"),STR16("%"),0,0.95,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarGate);
    parameters.addParameter(STR16("Root Note"),nullptr,127,40.0/127.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kRootPitch);

    parameters.addParameter(STR16("Fill Intensity"),STR16("%"),0,0.0,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kFillIntensity);
    for(int i=0;i<16;++i) {
      parameters.addParameter(STR16("Guitar DOUBLE"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarDoubleBase+i);
      parameters.addParameter(STR16("Note 1/8"),nullptr,1,0.,Steinberg::Vst::ParameterInfo::kCanAutomate,a125::rifferator::kGuitarLengthBase+i);
    }
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

    return Steinberg::kResultOk;
  }
  // VST3 stores edit-controller state independently from the processor's
  // musical project snapshot. Preserve the user's 100/150% GUI zoom across
  // Studio One project reopen without changing the existing processor ABI.
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* stream) override {
    using namespace Steinberg;
    if(!stream)return kResultFalse;
    constexpr uint32 magic=0x315A4652u; // "RFZ1" (little endian)
    constexpr uint32 version=2u;
    const uint32 zoom=zoomFactor_>=1.25?150u:100u;
    const uint32 fields[4]{magic,version,zoom,0u};
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
    if(fields[0]!=0x315A4652u || (fields[1]!=1u && fields[1]!=2u) ||
       (fields[2]!=100u && fields[2]!=150u))return kResultFalse;
    // Legacy v1 stored only zoom. New v2 independently stores the selected
    // GUI page so Studio One can restore the user's working surface.
    uint32 page=0;
    if(fields[1]>=2u) {
      int32 read=0;
      if(stream->read(&page,sizeof(page),&read)!=kResultOk ||
         read!=sizeof(page) || page>1u)return kResultFalse;
    }
    // Publish only after every field was read and validated. A truncated
    // project cannot corrupt zoom/page or a currently attached editor.
    zoomFactor_=fields[2]==150u?1.5:1.0;
    // Saved legacy Rhythm page is ignored in guitar-only mode.
    auto* editor=viewOwner_?viewOwner_:activeEditor_;
    if(editor && std::abs(editor->getZoomFactor()-zoomFactor_)>1.e-9)
      editor->setZoomFactor(zoomFactor_);
    refreshCommands();
    return kResultOk;
  }
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* stream) override {
    using namespace a125::rifferator;
    GuitarSnapshot snapshot;
    if(!readSnapshot(stream,snapshot))return Steinberg::kResultFalse;
    for(int i=0;i<16;++i)
      EditControllerEx1::setParamNormalized(kGuitarLockBase+i,0.);
    EditControllerEx1::setParamNormalized(kRootPitch,snapshot.root/127.);
    EditControllerEx1::setParamNormalized(kGuitarGate,snapshot.gate/100.);
    EditControllerEx1::setParamNormalized(kFillIntensity,snapshot.fill/100.);
    EditControllerEx1::setParamNormalized(kPmMin,(snapshot.pmMin-1)/126.);
    EditControllerEx1::setParamNormalized(kPmMax,(snapshot.pmMax-1)/126.);
    for(int i=0;i<16;++i) {
      const auto& st=snapshot.step[i];
      EditControllerEx1::setParamNormalized(kGuitarStepBase+i,snapshot.enabled[i]?1.:0.);
      EditControllerEx1::setParamNormalized(kGuitarArticulationBase+i,static_cast<int>(st.articulation)/5.);
      EditControllerEx1::setParamNormalized(kGuitarDegreeBase+i,(st.degree+13)/28.);
      EditControllerEx1::setParamNormalized(kGuitarDoubleBase+i,st.doubled?1.:0.);
      EditControllerEx1::setParamNormalized(kGuitarLengthBase+i,snapshot.eighth[i]?1.:0.);
      EditControllerEx1::setParamNormalized(kGuitarLockBase+i,st.locked?1.:0.);
    }
    refreshPmDisplays();refreshMusicalDisplays();
    for(int i=0;i<16;++i){refreshGuitarStep(i);refreshDegree(i);
      refreshSwitch(kGuitarStepBase+i);refreshSwitch(kGuitarLockBase+i);refreshSwitch(kGuitarDoubleBase+i);
      refreshSwitch(kGuitarLengthBase+i);}
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
