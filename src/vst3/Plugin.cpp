#include "Plugin.hpp"
#include "RifferatorPanel.hpp"
#include "BrandLogoView.hpp"
#include "UiGeometry.hpp"
#include "../core/StandardMidiFile.hpp"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cframe.h"
#include <fstream>
#include <vector>
#include <filesystem>
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace a125::riff::vst {
namespace {
bool loadSnapshot(IBStream* stream,product::Snapshot& output) noexcept {
 if(!stream)return false;
 product::StateBytes buffer{};
 int32 bytes=0;
 if(stream->read(buffer.data(),static_cast<int32>(buffer.size()),&bytes)!=kResultOk||
    bytes!=static_cast<int32>(buffer.size()))return false;
 return product::decode(buffer.data(),buffer.size(),output);
}
bool saveSnapshot(IBStream* stream,const product::Snapshot& source) noexcept {
 if(!stream)return false;
 product::StateBytes buffer{};
 if(!product::encode(source,buffer))return false;
 int32 written=0;
 return stream->write(buffer.data(),static_cast<int32>(buffer.size()),&written)==kResultOk&&
        written==static_cast<int32>(buffer.size());
}
}
Processor::Processor(){setControllerClass(ControllerUID);}
tresult PLUGIN_API Processor::initialize(FUnknown* context){
 const auto result=AudioEffect::initialize(context);
 if(result!=kResultOk)return result;
 addEventInput(STR16("Trigger In"),16,kMain,BusInfo::kDefaultActive);
 addEventOutput(STR16("Guitar Out"),1,kMain,BusInfo::kDefaultActive);
 return kResultOk;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup){
 if(std::isfinite(setup.sampleRate)&&setup.sampleRate>0.)sampleRate_=setup.sampleRate;
 return AudioEffect::setupProcessing(setup);
}
tresult PLUGIN_API Processor::setActive(TBool state){
 if(!state)cleanupPending_=true;
 return AudioEffect::setActive(state);
}
tresult PLUGIN_API Processor::setProcessing(TBool state){
 if(!state)cleanupPending_=true;
 return kResultOk;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size){
 return (size==kSample32||size==kSample64)?kResultTrue:kResultFalse;
}
tresult PLUGIN_API Processor::getState(IBStream* stream){
 return saveSnapshot(stream,snapshot_)?kResultOk:kResultFalse;
}
tresult PLUGIN_API Processor::setState(IBStream* stream){
 product::Snapshot candidate;
 if(!loadSnapshot(stream,candidate))return kResultFalse;
 snapshot_=candidate;
 cleanupPending_=true;
 return kResultOk;
}
tresult PLUGIN_API Processor::process(ProcessData& data){
 if(data.numSamples<0)return kInvalidArgument;
 product::Snapshot candidate=snapshot_;
 bool patternEdited=false;
 if(data.inputParameterChanges){
  auto* changes=data.inputParameterChanges;
  // LOCK first prevents same-block user edits from silently modifying a locked step.
  for(int pass=0;pass<2;++pass){
   for(int32 i=0;i<changes->getParameterCount();++i){
    auto* queue=changes->getParameterData(i);
    if(!queue||queue->getPointCount()==0)continue;
    const auto id=queue->getParameterId();
    const bool isLock=(id>=Lock&&id<Lock+kMaxSteps);
    if((pass==0)!=isLock)continue;
    int32 sample=0;ParamValue value=0.;
    if(queue->getPoint(queue->getPointCount()-1,sample,value)==kResultOk){
      if(apply(candidate,id,value)&&id>=On&&id<Lock+kMaxSteps&&
         !(id>=Lock&&id<Lock+kMaxSteps))patternEdited=true;
    }
   }
  }
 }
 if(candidate.valid()){
  if(patternEdited)cleanupPending_=true;
  if(candidate.pattern.bars!=snapshot_.pattern.bars||
     candidate.musical.key!=snapshot_.musical.key||
     candidate.musical.rootMidi!=snapshot_.musical.rootMidi||
     candidate.musical.scale!=snapshot_.musical.scale||
     candidate.pmVelocity!=snapshot_.pmVelocity)
   cleanupPending_=true;
  snapshot_=candidate;
 }
 auto& s=snapshot_;
 Block b{};
 b.numSamples=data.numSamples;
 b.sampleRate=sampleRate_;
 const auto* context=data.processContext;
 if(context){
  b.tempoValid=(context->state&ProcessContext::kTempoValid)!=0;
  b.ppqValid=(context->state&ProcessContext::kProjectTimeMusicValid)!=0;
  b.playing=(context->state&ProcessContext::kPlaying)!=0;
  if(b.tempoValid)b.tempo=context->tempo;
  if(b.ppqValid)b.projectPpq=context->projectTimeMusic;
 }
 TriggerEvent triggers[MidiRuntime::kMaxTriggerEvents]{};
 int triggerCount=0;
 if(data.inputEvents){
  const int32 total=data.inputEvents->getEventCount();
  if(total>MidiRuntime::kMaxTriggerEvents)return kResultFalse;
  for(int32 i=0;i<total;++i){
   Steinberg::Vst::Event e{};
   if(data.inputEvents->getEvent(i,e)!=kResultOk)continue;
   if(e.type!=Steinberg::Vst::Event::kNoteOnEvent&&e.type!=Steinberg::Vst::Event::kNoteOffEvent)continue;
   const bool on=e.type==Steinberg::Vst::Event::kNoteOnEvent&&e.noteOn.velocity>0.f;
   const int pitch=e.type==Steinberg::Vst::Event::kNoteOnEvent?e.noteOn.pitch:e.noteOff.pitch;
   if(pitch<0||pitch>127||e.sampleOffset<0||e.sampleOffset>=data.numSamples)continue;
   triggers[triggerCount++]={e.sampleOffset,pitch,on};
  }
 }
 auto sink=[&](MidiOut midi)->bool {
  if(!data.outputEvents)return false;
  Steinberg::Vst::Event event{};
  event.busIndex=0;
  event.sampleOffset=midi.sampleOffset;
  if(midi.noteOn){
   event.type=Steinberg::Vst::Event::kNoteOnEvent;
   event.noteOn.channel=0;
   event.noteOn.pitch=static_cast<int16>(midi.pitch);
   event.noteOn.velocity=static_cast<float>(midi.velocity)/127.f;
   event.noteOn.tuning=0.f;
   event.noteOn.length=0;
   event.noteOn.noteId=-1;
  }else{
   event.type=Steinberg::Vst::Event::kNoteOffEvent;
   event.noteOff.channel=0;
   event.noteOff.pitch=static_cast<int16>(midi.pitch);
   event.noteOff.velocity=0.f;
   event.noteOff.noteId=-1;
   event.noteOff.tuning=0.f;
  }
  return data.outputEvents->addEvent(event)==kResultOk;
 };
 if(cleanupPending_){
  // Forced release using the *same* host output list before any new attacks.
  // setActive(false) itself has no output-event sink and must not erase state.
  Block stopped=b;stopped.playing=false;stopped.ppqValid=false;
  runtime_.setMode(SyncMode::Transport);
  if(!runtime_.process(s.pattern,s.musical,stopped,nullptr,0,
       heavier::Velocity{s.pmVelocity},sink))return kResultFalse;
  cleanupPending_=false;
 }
 runtime_.setMode(s.syncMode);
 return runtime_.process(s.pattern,s.musical,b,triggers,triggerCount,
              heavier::Velocity{s.pmVelocity},sink)?kResultOk:kResultFalse;
}

tresult PLUGIN_API Controller::initialize(FUnknown* context){
 const auto result=EditController::initialize(context);
 if(result!=kResultOk)return result;
 auto add=[&](const char16_t* name,ParamID id,int minimum,int maximum,int initial){
  auto* p=new RangeParameter(name,id,STR16(""),
     static_cast<double>(minimum),static_cast<double>(maximum),
     static_cast<double>(initial),maximum-minimum,ParameterInfo::kCanAutomate);
  p->setPrecision(0);
  parameters.addParameter(p);
 };
 add(STR16("Key"),Key,0,11,4);
 add(STR16("Scale"),ScaleId,0,1,1);
 add(STR16("Root Note"),Root,0,127,40);
 add(STR16("Bars"),Bars,0,3,0);
 add(STR16("PM Velocity"),Pm,30,83,30);
 add(STR16("Fill Intensity"),Fill,0,100,0);
 add(STR16("Trigger Mode"),Mode,0,1,0);
 const struct {const char* name;ParamID base;int low,high,initial;} fields[]={
  {"ON",On,0,1,0},{"STOP",Stop,0,1,0},
  {"TYPE",Type,0,3,0},{"DEGREE",Degree,-14,14,0},
  {"LOCK",Lock,0,1,0}
 };
 for(const auto& field:fields)for(int i=0;i<kMaxSteps;++i){
  char ascii[64]{};
  std::snprintf(ascii,sizeof(ascii),"Step %03d %s",i+1,field.name);
  char16_t name[64]{};
  for(std::size_t c=0;c<63&&ascii[c];++c)
   name[c]=static_cast<char16_t>(ascii[c]);
  add(name,field.base+static_cast<ParamID>(i),
      field.low,field.high,field.initial);
 }
 return kResultOk;
}
tresult PLUGIN_API Controller::setComponentState(IBStream* stream){
 product::Snapshot candidate;
 if(!loadSnapshot(stream,candidate))return kResultFalse;
 visual_=candidate;
 bar_=std::clamp(bar_,0,visual_.pattern.bars-1);
 focused_=bar_*16;
 pickerStep_=-1;
 for(ParamID id=Key;id<=Mode;++id)setParamNormalized(id,read(visual_,id));
 const ParamID bases[]={On,Stop,Type,Degree,Lock};
 for(const auto base:bases)
  for(ParamID i=0;i<kMaxSteps;++i)
   setParamNormalized(base+i,read(visual_,base+i));
 return kResultOk;
}
// UI-only state has a separate versioned header: no processor state mutation.
tresult PLUGIN_API Controller::setState(IBStream* stream){
 if(!stream)return kInvalidArgument;
 std::uint32_t values[3]{};
 int32 bytes=0;
 if(stream->read(values,sizeof(values),&bytes)!=kResultOk||
    bytes!=sizeof(values)||values[0]!=0x52475531u||
    (values[1]!=100&&values[1]!=150)||values[2]>=kMaxBars)return kResultFalse;
 zoom_=values[1]/100.;
 bar_=std::min<int>(static_cast<int>(values[2]),visual_.pattern.bars-1);
 focused_=bar_*16;
 pickerStep_=-1;
 if(editor_)editor_->setZoomFactor(zoom_);
 redraw();
 return kResultOk;
}
tresult PLUGIN_API Controller::getState(IBStream* stream){
 if(!stream)return kInvalidArgument;
 const std::uint32_t values[3]={0x52475531u,zoom_>1.25?150u:100u,
                                 static_cast<std::uint32_t>(bar_)};
 int32 written=0;
 return stream->write((void*)values,sizeof(values),&written)==kResultOk&&
        written==sizeof(values)?kResultOk:kResultFalse;
}
IPlugView* PLUGIN_API Controller::createView(FIDString name){
 if(!name||std::strcmp(name,ViewType::kEditor)!=0)return nullptr;
 auto* view=new VSTGUI::VST3Editor(this,"RifferatorView","rifferator.uidesc");
 view->setAllowedZoomFactors(std::vector<double>{1.0,1.5});
 view->setEditorSizeConstrains(VSTGUI::CPoint(1160,570),VSTGUI::CPoint(1160,570));
 editor_=view;
 editor_->setZoomFactor(zoom_);
 return view;
}
VSTGUI::CView* Controller::createCustomView(VSTGUI::UTF8StringPtr name,
    const VSTGUI::UIAttributes& attr,const VSTGUI::IUIDescription*,
    VSTGUI::VST3Editor* editor){
 if(!name||!editor)return nullptr;
 VSTGUI::CPoint origin{0.,0.},size{100.,100.};
 attr.getPointAttribute("origin",origin);
 attr.getPointAttribute("size",size);
 const VSTGUI::CRect area(origin,size);
 if(std::strcmp(name,"RifferatorBrandLogo")==0)return new BrandLogoView(area);
 if(std::strcmp(name,"RifferatorPanel")==0){
   panel_=new RifferatorPanel(area,this);
   return panel_;
 }
 return nullptr;
}
void Controller::willClose(VSTGUI::VST3Editor* editor){
 if(editor_==editor){panel_=nullptr;editor_=nullptr;pickerStep_=-1;}
}
void Controller::redraw(){
 if(panel_)panel_->invalid();
}
tresult PLUGIN_API Controller::setParamNormalized(ParamID tag,ParamValue value){
 const auto r=EditController::setParamNormalized(tag,value);
 if(r==kResultOk){
  apply(visual_,tag,value);
  bar_=std::clamp(bar_,0,visual_.pattern.bars-1);
  if(focused_>=visual_.pattern.activeSteps())focused_=bar_*16;
  redraw();
 }
 return r;
}
void Controller::publish(ParamID tag,double value){
 beginEdit(tag);
 EditController::setParamNormalized(tag,value);
 performEdit(tag,value);
 endEdit(tag);
 apply(visual_,tag,value);
 redraw();
}
void Controller::publishPattern(const product::Snapshot& target){
 const auto previous=visual_;
 visual_=target;
 for(int i=0;i<kMaxSteps;++i){
  const auto& old=previous.pattern.steps[static_cast<std::size_t>(i)];
  const auto& now=visual_.pattern.steps[static_cast<std::size_t>(i)];
  const ParamID cell=static_cast<ParamID>(i);
  if(old.on!=now.on)publish(On+cell,read(visual_,On+cell));
  if(old.stop!=now.stop)publish(Stop+cell,read(visual_,Stop+cell));
  if(old.articulation!=now.articulation)publish(Type+cell,read(visual_,Type+cell));
  if(old.degree!=now.degree)publish(Degree+cell,read(visual_,Degree+cell));
 }
 redraw();
}
void Controller::exportMidiFromUI(){
 if(!editor_||!editor_->getFrame()){status_="NO EDITOR";redraw();return;}
 auto* selector=VSTGUI::CNewFileSelector::create(editor_->getFrame(),
                                                  VSTGUI::CNewFileSelector::kSelectSaveFile);
 if(!selector){status_="SAVE DIALOG FAILED";redraw();return;}
 selector->setTitle("Export Standard MIDI File");
 selector->setDefaultSaveName("125A Rifferator Riff.mid");
 selector->setDefaultExtension(VSTGUI::CFileExtension("MIDI file","mid"));
 if(selector->runModal()&&selector->getNumSelectedFiles()>0){
  const auto* utf8=selector->getSelectedFile(0);
  const auto bytes=exportMidi(visual_.pattern,visual_.musical,
                              heavier::Velocity{visual_.pmVelocity});
  if(utf8&&!bytes.empty()){
   std::ofstream out(std::filesystem::u8path(utf8),std::ios::binary|std::ios::trunc);
   if(out){
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    out.flush();
    status_=out.good()?"MIDI EXPORTED":"EXPORT WRITE FAILED";
   }else status_="EXPORT FILE FAILED";
  }else status_="INVALID MIDI PATTERN";
 }
 selector->forget();
 redraw();
}
void Controller::click(int x,int y,bool shift){
 const auto hit=ui::hit(x,y,pickerStep_>=0);
 using ui::Target;
 if(hit.target==Target::None)return;
 if(hit.target==Target::Zoom){
  zoom_=zoom_>1.25?1.:1.5;
  if(editor_)editor_->setZoomFactor(zoom_);
 }else if(hit.target==Target::Prev||hit.target==Target::Next){
  const int move=hit.target==Target::Prev?-1:1;
  bar_=(bar_+visual_.pattern.bars+move)%visual_.pattern.bars;
  focused_=bar_*16;
  pickerStep_=-1;
 }else if(hit.target==Target::Bars){
  const int b=visual_.pattern.bars;
  const int next=b==8?1:b*2;
  const double index=next==1?0.:next==2?1.:next==4?2.:3.;
  publish(Bars,index/3.);
  if(bar_>=next)bar_=next-1;
  focused_=bar_*16;pickerStep_=-1;
 }else if(hit.target==Target::Key){
  const int newKey=(visual_.musical.key+1)%12;
  // Root must remain in the selected scale; follow the tonic when KEY changes.
  const int root=36+newKey;
  publish(Key,n(newKey,0,11));publish(Root,n(root,0,127));
 }else if(hit.target==Target::Scale){
  publish(ScaleId,visual_.musical.scale==Scale::Major?1.:0.);
 }else if(hit.target==Target::Root){
  const auto pitch=pitchForDegree(1,visual_.musical);
  if(pitch>=0)publish(Root,n(pitch,0,127));
 }else if(hit.target==Target::Pm){
  const int next=visual_.pmVelocity>=83?30:std::min(83,visual_.pmVelocity+5);
  publish(Pm,n(next,30,83));
 }else if(hit.target==Target::Fill){
  const int next=visual_.musical.fillIntensity>=100?0:std::min(100,visual_.musical.fillIntensity+10);
  publish(Fill,n(next,0,100));
 }else if(hit.target==Target::Mode){
  publish(Mode,visual_.syncMode==SyncMode::Transport?1.:0.);
 }else if(hit.target==Target::StepOn){
  if(!visual_.pattern.steps[static_cast<std::size_t>(focused_)].locked)
   publish(On+focused_,visual_.pattern.steps[static_cast<std::size_t>(focused_)].on?0.:1.);
 }else if(hit.target==Target::Lock){
  const auto lock=visual_.pattern.steps[static_cast<std::size_t>(focused_)].locked;
  publish(Lock+focused_,lock?0.:1.);
 }else if(hit.target==Target::Cell){
  focused_=bar_*16+hit.column;
  const auto& st=visual_.pattern.steps[static_cast<std::size_t>(focused_)];
  if(st.locked){status_="STEP LOCKED";redraw();return;}
  if(shift){pickerStep_=focused_;status_="SELECT SCALE NOTE";redraw();return;}
  pickerStep_=-1;
  const int techniques[]={3,2,0,1};
  if(hit.row==4){
   publish(Stop+focused_,1.);
   publish(On+focused_,1.);
  }else{
   const int technique=techniques[hit.row];
   const bool already=st.on&&!st.stop&&st.articulation==technique;
   publish(Stop+focused_,0.);
   publish(Type+focused_,n(technique,0,3));
   publish(On+focused_,already?0.:1.);
  }
 }else if(hit.target==Target::Picker){
  if(pickerStep_>=0){
   const int degree=hit.column-7;
   const int note=pitchForDegree(degree,visual_.musical);
   const int destination=pickerStep_;
   pickerStep_=-1;
   if(note>=0&&!visual_.pattern.steps[static_cast<std::size_t>(destination)].locked){
    publish(Degree+destination,n(degree,-14,14));
    publish(On+destination,1.);
    status_="SCALE NOTE SET";
   }else status_="NOTE OUT OF RANGE";
  }
 }else if(hit.target==Target::Generate||hit.target==Target::Variate||
          hit.target==Target::Clear){
  auto generated=visual_;
  if(hit.target==Target::Generate)heavier::generate(generated.pattern,generated.musical,++seed_);
  else if(hit.target==Target::Variate)heavier::variate(generated.pattern,generated.musical,++seed_);
  else generated.pattern.clear();
  publishPattern(generated);
  status_=hit.target==Target::Generate?"PATTERN GENERATED":
          hit.target==Target::Variate?"PATTERN VARIED":"PATTERN CLEARED";
 }else if(hit.target==Target::Export){
  exportMidiFromUI();
 }
 redraw();
}
} // namespace a125::riff::vst

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace a125::riff::vst;
BEGIN_FACTORY_DEF("125A", "https://github.com/challanger2000/125A-Rifferator", "")
DEF_CLASS2(INLINE_UID_FROM_FUID(ProcessorUID),PClassInfo::kManyInstances,kVstAudioEffectClass,
 "125A Rifferator",Vst::kDistributable,Vst::PlugType::kInstrumentSynth,
 "1.0.0",kVstVersionString,Processor::createInstance)
DEF_CLASS2(INLINE_UID_FROM_FUID(ControllerUID),PClassInfo::kManyInstances,kVstComponentControllerClass,
 "125A Rifferator Controller",0,"","1.0.0",kVstVersionString,Controller::createInstance)
END_FACTORY
