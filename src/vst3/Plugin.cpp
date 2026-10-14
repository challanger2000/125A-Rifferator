#include "Plugin.hpp"
#include "RifferatorPanel.hpp"
#include "BrandLogoView.hpp"
#include "UiGeometry.hpp"
#include "UiStepPolicy.hpp"
#include "../core/StandardMidiFile.hpp"
#include "vstgui/uidescription/uiattributes.h"
#include "vstgui/lib/cfileselector.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/controls/coptionmenu.h"
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
#include <string>

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
 int pasteBar=-1;
 if(data.inputParameterChanges){
  auto* changes=data.inputParameterChanges;
  for(int32 i=0;i<changes->getParameterCount();++i){
   auto* queue=changes->getParameterData(i);
   if(!queue||queue->getParameterId()!=PasteTransaction||queue->getPointCount()==0)continue;
   int32 offset=0;ParamValue value=0.;
   if(queue->getPoint(queue->getPointCount()-1,offset,value)==kResultOk)
    pasteBar=q(value,15)%8;
  }
  // A recalled or automated marker alone must never disable LOCKs.
  // Require the complete destination BAR lock payload in this same block.
  bool lockPayload[kStepsPerBar]{};
  if(pasteBar>=0&&pasteBar<candidate.pattern.bars){
   for(int32 i=0;i<changes->getParameterCount();++i){
    auto* queue=changes->getParameterData(i);
    if(!queue||queue->getPointCount()==0)continue;
    const auto id=queue->getParameterId();
    if(id>=Lock+static_cast<ParamID>(pasteBar*kStepsPerBar)&&
       id<Lock+static_cast<ParamID>((pasteBar+1)*kStepsPerBar))
     lockPayload[id-Lock-static_cast<ParamID>(pasteBar*kStepsPerBar)]=true;
   }
   for(bool present:lockPayload)if(!present){pasteBar=-1;break;}
  }else pasteBar=-1;
  if(pasteBar>=0)
   for(int j=0;j<kStepsPerBar;++j)
    candidate.pattern.steps[static_cast<std::size_t>(pasteBar*kStepsPerBar+j)].locked=false;
 }
 if(data.inputParameterChanges){
  auto* changes=data.inputParameterChanges;
  // LOCK first prevents same-block user edits from silently modifying a locked step.
  for(int pass=0;pass<2;++pass){
   for(int32 i=0;i<changes->getParameterCount();++i){
    auto* queue=changes->getParameterData(i);
    if(!queue||queue->getPointCount()==0)continue;
    const auto id=queue->getParameterId();
    const bool isLock=(id>=Lock&&id<Lock+kMaxSteps);
    if(id==PasteTransaction)continue;
    if(pasteBar>=0){
     if((pass==0)==isLock)continue; // explicit paste: payload before final locks
    }else if((pass==0)!=isLock)continue;
    int32 sample=0;ParamValue value=0.;
    if(queue->getPoint(queue->getPointCount()-1,sample,value)==kResultOk){
      // A live STEP/LOCK edit updates the next grid event, not the MIDI trigger.
      apply(candidate,id,value);
    }
   }
  }
 }
 if(candidate.valid()){
  // Only pitch-basis changes need a voice release. Do not reset held_
  // or phaseTicks_ in MIDI NOTE when live-editing any pattern or velocity.
  releasePitchOnly_=
      candidate.musical.key!=snapshot_.musical.key||
      candidate.musical.rootMidi!=snapshot_.musical.rootMidi||
      candidate.musical.scale!=snapshot_.musical.scale;
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
 if(releasePitchOnly_&&!cleanupPending_){
  if(!runtime_.releaseVoice(0,sink))return kResultFalse;
 }
 releasePitchOnly_=false;
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
 add(STR16("Bar Paste Transaction"),PasteTransaction,0,15,0);
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
 componentRestored_=true;
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
// UI-only Controller state v2 also persists the deterministic GENERATE seed.
// The 12-byte legacy format (v1) is accepted without changing Processor state.
tresult PLUGIN_API Controller::setState(IBStream* stream){
 if(!stream)return kInvalidArgument;
 std::uint32_t values[3]{};
 int32 bytes=0;
 if(stream->read(values,sizeof(values),&bytes)!=kResultOk||
    bytes!=sizeof(values)||
    (values[0]!=0x52475531u&&values[0]!=0x52475532u)||
    (values[1]!=100&&values[1]!=150)||values[2]>=kMaxBars)
  return kResultFalse;
 std::uint32_t nextSeed=seed_;
 if(values[0]==0x52475532u){
  int32 readSeed=0;
  if(stream->read(&nextSeed,sizeof(nextSeed),&readSeed)!=kResultOk||
     readSeed!=sizeof(nextSeed))
   return kResultFalse;
 }
 // No GUI/controller field changes until validation and all reads complete.
 zoom_=values[1]/100.;
 bar_=static_cast<int>(values[2]);
 seed_=nextSeed;
 if(componentRestored_)bar_=std::min(bar_,visual_.pattern.bars-1);
 focused_=bar_*16;
 pickerStep_=-1;
 if(editor_)editor_->setZoomFactor(zoom_);
 redraw();
 return kResultOk;
}
tresult PLUGIN_API Controller::getState(IBStream* stream){
 if(!stream)return kInvalidArgument;
 const std::uint32_t values[4]={0x52475532u,zoom_>1.25?150u:100u,
                                 static_cast<std::uint32_t>(bar_),seed_};
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
 if(editor_==editor){
  endNumericDrag(); // Finish any pending host automation gesture.
  panel_=nullptr;editor_=nullptr;pickerStep_=-1;
 }
}
void Controller::redraw(){
 if(panel_)panel_->invalid();
}
tresult PLUGIN_API Controller::setParamNormalized(ParamID tag,ParamValue value){
 if(tag==PasteTransaction)return EditController::setParamNormalized(tag,value);
 const auto result=EditController::setParamNormalized(tag,value);
 if(result!=kResultOk)return result;
 if(!apply(visual_,tag,value)){
  // Rejected LOCK-protected edits must not leave stale host parameter values.
  EditController::setParamNormalized(tag,read(visual_,tag));
  redraw();
  return kResultFalse;
 }
 const double effective=read(visual_,tag);
 if(std::abs(effective-value)>1.e-8)
  EditController::setParamNormalized(tag,effective);
 if(tag==Key||tag==ScaleId)
  EditController::setParamNormalized(Root,read(visual_,Root));
 bar_=std::clamp(bar_,0,visual_.pattern.bars-1);
 if(focused_>=visual_.pattern.activeSteps())focused_=bar_*16;
 redraw();
 return result;
}
void Controller::publish(ParamID tag,double value){
 // Publish the quantized, validated value that the GUI actually displays.
 const int previousRoot=visual_.musical.rootMidi;
 if(!apply(visual_,tag,value))return;
 const double normalized=read(visual_,tag);
 beginEdit(tag);
 EditController::setParamNormalized(tag,normalized);
 performEdit(tag,normalized);
 endEdit(tag);
 if((tag==Key||tag==ScaleId)&&visual_.musical.rootMidi!=previousRoot){
  const double rootValue=read(visual_,Root);
  beginEdit(Root);
  EditController::setParamNormalized(Root,rootValue);
  performEdit(Root,rootValue);
  endEdit(Root);
 }
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
// Native VSTGUI popup menus allow a single direct choice; the Root menu is
// grouped by octave so users can recover instantly even from MIDI G9.
void Controller::openChoiceMenu(ParamID id,int x,int y){
 if(!editor_||!editor_->getFrame()||!panel_)return;
 auto* menu=new VSTGUI::COptionMenu;
 const auto add=[&](VSTGUI::COptionMenu* to,const std::string& title,int value,
                   bool current=false){
  auto* item=new VSTGUI::CMenuItem(title.c_str(),value);
  if(current)item->setChecked(true);
  to->addEntry(item);
 };
 static constexpr const char* names[12]={
  "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
 };
 if(id==Key){
  for(int i=0;i<12;++i)add(menu,names[i],i,i==visual_.musical.key);
 }else if(id==ScaleId){
  add(menu,"MAJOR",0,visual_.musical.scale==Scale::Major);
  add(menu,"NATURAL MINOR",1,visual_.musical.scale==Scale::NaturalMinor);
 }else if(id==Bars){
  for(int bars : {1,2,4,8})add(menu,std::to_string(bars)+" BARS",bars,
                             bars==visual_.pattern.bars);
 }else if(id==Root){
  // Reset always uses the active key's tonic in octave 2; E2 at defaults.
  add(menu,std::string("RESET TO ")+names[visual_.musical.key]+"2",
      36+visual_.musical.key);
  for(int octave=-1;octave<=9;++octave){
   auto* group=new VSTGUI::COptionMenu;
   for(int pitch=0;pitch<=127;++pitch){
    if(pitch/12-1!=octave||!pitchInScale(pitch,visual_.musical))continue;
    add(group,std::string(names[pitch%12])+std::to_string(octave),pitch,
        pitch==visual_.musical.rootMidi);
   }
   if(group->getNbEntries()>0){
    menu->addEntry(group,(std::string("OCTAVE ")+std::to_string(octave)).c_str());
   }
   group->forget();
  }
 }
 VSTGUI::CPoint at(static_cast<VSTGUI::CCoord>(x),
                    static_cast<VSTGUI::CCoord>(y));
 panel_->localToFrame(at);
 if(!menu->popup(editor_->getFrame(),at,[this,id](VSTGUI::COptionMenu* opened){
   int index=-1;
   auto* selected=opened->getLastItemMenu(index);
   if(selected&&index>=0){
    auto* item=selected->getEntry(index);
    if(item&&item->getTag()>=0){
     const int choice=item->getTag();
     if(id==Bars){
      const int ordinal=choice==1?0:choice==2?1:choice==4?2:3;
      publish(Bars,n(ordinal,0,3));
      if(bar_>=choice)bar_=choice-1;
      focused_=bar_*16;
      pickerStep_=-1;
     }else if(id==Key)publish(Key,n(choice,0,11));
     else if(id==ScaleId)publish(ScaleId,n(choice,0,1));
     else if(id==Root)publish(Root,n(choice,0,127));
    }
   }
   opened->forget();
   redraw();
 })){
  menu->forget();
 }
}
bool Controller::cancelPicker(){
 if(pickerStep_<0)return false;
 pickerStep_=-1;
 status_="NOTE SELECTION CANCELLED";
 redraw();
 return true;
}
void Controller::beginNumericDrag(int x,int y){
 const auto target=ui::hit(x,y,false).target;
 using ui::Target;
 if(target!=Target::Pm&&target!=Target::Fill)return;
 endNumericDrag();
 activeNumericParam_=target==Target::Pm?static_cast<int>(Pm):
                                           static_cast<int>(Fill);
 beginEdit(static_cast<ParamID>(activeNumericParam_));
 updateNumericDrag(x);
}
void Controller::updateNumericDrag(int x){
 if(activeNumericParam_<0)return;
 using ui::Target;
 const bool pm=activeNumericParam_==static_cast<int>(Pm);
 const auto target=pm?Target::Pm:Target::Fill;
 const int value=ui::numericValue(target,x);
 const auto id=static_cast<ParamID>(activeNumericParam_);
 const double normalized=pm?n(value,30,83):n(value,0,100);
 if(std::abs(read(visual_,id)-normalized)<1.e-9)return;
 EditController::setParamNormalized(id,normalized);
 performEdit(id,normalized);
 apply(visual_,id,normalized);
 redraw();
}
void Controller::endNumericDrag(){
 if(activeNumericParam_<0)return;
 endEdit(static_cast<ParamID>(activeNumericParam_));
 activeNumericParam_=-1;
}
void Controller::nudgeNumeric(int x,int y,int delta){
 const auto target=ui::hit(x,y,false).target;
 using ui::Target;
 if(target==Target::Pm)
  publish(Pm,n(ui::numericNudge(target,visual_.pmVelocity,delta),30,83));
 else if(target==Target::Fill)
  publish(Fill,n(ui::numericNudge(target,visual_.musical.fillIntensity,delta),0,100));
}
void Controller::click(int x,int y,bool shift,bool control){
 const auto hit=ui::hit(x,y,pickerStep_>=0);
 using ui::Target;
 // Picker is modal: X, Escape, or an outside click cancels without
 // committing a pitch or activating controls beneath it.
 if(ui::shouldCancelPicker(pickerStep_>=0,hit.target)){
  cancelPicker();
  return;
 }
 if(hit.target==Target::PickerCancel){cancelPicker();return;}
 if(hit.target==Target::None)return;
 // 125A default-reset gesture. Other controls must NOT become inert
 // just because Ctrl is held (previously swallowed Export/Generate/etc.).
 if(control&&ui::supportsReset(hit.target)){
  if(hit.target==Target::Key){
   publish(Key,n(4,0,11));
   publish(Root,n(40,0,127));
  }else if(hit.target==Target::Scale)publish(ScaleId,1.);
  else if(hit.target==Target::Root)
   publish(Root,n(36+visual_.musical.key,0,127));
  else if(hit.target==Target::Bars){
   publish(Bars,0.);
   bar_=0;focused_=0;pickerStep_=-1;
  }else if(hit.target==Target::Pm)publish(Pm,n(30,30,83));
  else if(hit.target==Target::Fill)publish(Fill,0.);
  else if(hit.target==Target::Mode)publish(Mode,0.);
  else if(hit.target==Target::Zoom){
   zoom_=1.;
   if(editor_)editor_->setZoomFactor(zoom_);
  }else if(hit.target==Target::StepOn){
   if(visual_.pattern.steps[static_cast<std::size_t>(focused_)].locked)
    status_="STEP LOCKED";
   else publish(On+focused_,0.);
  }else if(hit.target==Target::Lock)publish(Lock+focused_,0.);
  else if(hit.target==Target::Cell){
   focused_=bar_*16+hit.column;
   auto reset=visual_;
   if(ui::resetStep(reset.pattern,focused_)){
    publishPattern(reset);
    status_="STEP RESET";
   }else status_="STEP LOCKED";
   pickerStep_=-1;
  }
  redraw();
  return;
 }
 if(hit.target==Target::Zoom){
  zoom_=zoom_>1.25?1.:1.5;
  if(editor_)editor_->setZoomFactor(zoom_);
 }else if(hit.target==Target::Prev||hit.target==Target::Next){
  const int move=hit.target==Target::Prev?-1:1;
  bar_=(bar_+visual_.pattern.bars+move)%visual_.pattern.bars;
  focused_=bar_*16;
  pickerStep_=-1;
 }else if(hit.target==Target::CopyBar){
   status_=clipboard_.copy(visual_.pattern,bar_)?"BAR COPIED":"COPY FAILED";
 }else if(hit.target==Target::PasteBar){
   auto updated=visual_;
   if(!clipboard_.paste(updated.pattern,bar_)){
    status_="COPY A BAR FIRST";
   }else{
    const auto previous=visual_;
    visual_=updated;
    pasteToggle_=!pasteToggle_;
    const double marker=n(bar_+(pasteToggle_?8:0),0,15);
    auto send=[this](ParamID tag,double value){
     beginEdit(tag);
     EditController::setParamNormalized(tag,value);
     performEdit(tag,value);
     endEdit(tag);
    };
    send(PasteTransaction,marker);
    for(int j=0;j<kStepsPerBar;++j){
     const int index=bar_*kStepsPerBar+j;
     const auto& old=previous.pattern.steps[static_cast<std::size_t>(index)];
     const auto& now=visual_.pattern.steps[static_cast<std::size_t>(index)];
     const ParamID cell=static_cast<ParamID>(index);
     if(old.on!=now.on)send(On+cell,read(visual_,On+cell));
     if(old.stop!=now.stop)send(Stop+cell,read(visual_,Stop+cell));
     if(old.articulation!=now.articulation)send(Type+cell,read(visual_,Type+cell));
     if(old.degree!=now.degree)send(Degree+cell,read(visual_,Degree+cell));
     // Marker temporarily unlocks target BAR: publish every final LOCK bit.
     send(Lock+cell,read(visual_,Lock+cell));
    }
    status_="BAR PASTED";
   }
 }else if(hit.target==Target::Bars){
  openChoiceMenu(Bars,x,y);
 }else if(hit.target==Target::Key){
  openChoiceMenu(Key,x,y);
 }else if(hit.target==Target::Scale){
  openChoiceMenu(ScaleId,x,y);
 }else if(hit.target==Target::Root){
  openChoiceMenu(Root,x,y);
 }else if(hit.target==Target::Pm){
  publish(Pm,n(ui::numericValue(Target::Pm,x),30,83));
 }else if(hit.target==Target::Fill){
  publish(Fill,n(ui::numericValue(Target::Fill,x),0,100));
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
  auto next=visual_;
  if(ui::toggleStep(next.pattern,focused_,hit.row)){
   publishPattern(next);
   status_=next.pattern.steps[static_cast<std::size_t>(focused_)].on?
             (hit.row==4?"STOP SET":"STEP SET"):
             (hit.row==4?"STOP CLEARED":"STEP CLEARED");
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
  else if(hit.target==Target::Variate){
   if(!heavier::variate(generated.pattern,generated.musical,++seed_)){
    status_="NOTHING UNLOCKED TO VARIATE";
    redraw();return;
   }
  }else generated.pattern.clear();
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
