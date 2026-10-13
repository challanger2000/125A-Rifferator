#include "Plugin.hpp"
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
    if(queue->getPoint(queue->getPointCount()-1,sample,value)==kResultOk)
      apply(candidate,id,value);
   }
  }
 }
 if(candidate.valid()){
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
 for(ParamID id=Key;id<=Mode;++id)setParamNormalized(id,read(visual_,id));
 const ParamID bases[]={On,Stop,Type,Degree,Lock};
 for(const auto base:bases)
  for(ParamID i=0;i<kMaxSteps;++i)
   setParamNormalized(base+i,read(visual_,base+i));
 return kResultOk;
}
tresult PLUGIN_API Controller::setState(IBStream* stream){
 // GUI-only persistence begins only when the branded static design is integrated.
 return stream?kResultOk:kInvalidArgument;
}
tresult PLUGIN_API Controller::getState(IBStream* stream){
 return stream?kResultOk:kInvalidArgument;
}
IPlugView* PLUGIN_API Controller::createView(FIDString){
 // No unapproved placeholder editor: VSTGUI comes after static design acceptance.
 return nullptr;
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
