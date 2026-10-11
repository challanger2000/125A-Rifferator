#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <algorithm>
#include <cmath>
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/ibstream.h"
#include <array>
#include <cstdint>

namespace a125::rifferator {
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
const FUID controllerId(0x125A4602,0x52494646,0x45524154,0x4F520100);
}
void Processor::prepareFillVariants() {
  for(int index=0;index<11;++index) {
    FillOptions options=fill_;
    options.intensityPercent=index*10;
    fillVariants_[static_cast<std::size_t>(index)]=arrange(steps_,config_,options,16);
  }
}
Processor::Processor() {
  setControllerClass(controllerId);
  steps_.resize(16);
  for(int i=0;i<16;i+=2) steps_[i].articulation=Articulation::PalmMute;
  prepareFillVariants();
  drumNotes_=renderDrums(drumPattern_,16);
  scheduler_.setDrums(&drumNotes_);
}
FUnknown* Processor::createInstance(void*) {
  return static_cast<IAudioProcessor*>(new Processor);
}
tresult PLUGIN_API Processor::initialize(FUnknown* context) {
  const auto status=AudioEffect::initialize(context);
  if(status!=kResultOk)return status;
  addEventInput(STR16("MIDI In"),16,kMain,BusInfo::kDefaultActive);
  addEventOutput(STR16("Guitar Out"),1,kMain,BusInfo::kDefaultActive);
  addEventOutput(STR16("Bass Out"),1,kAux,BusInfo::kDefaultActive);
  addEventOutput(STR16("Drums Out"),1,kAux,BusInfo::kDefaultActive);
  return kResultOk;
}
uint32 PLUGIN_API Processor::getProcessContextRequirements() {
  return IProcessContextRequirements::kNeedTempo |
    IProcessContextRequirements::kNeedProjectTimeMusic |
    IProcessContextRequirements::kNeedTransportState;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
  return size==kSample32 || size==kSample64 ? kResultTrue:kResultFalse;
}
tresult PLUGIN_API Processor::setActive(TBool active) {
  if(!active)flushOnNextProcess_=true;
  return AudioEffect::setActive(active);
}
tresult PLUGIN_API Processor::process(ProcessData& data) {
  if(data.inputParameterChanges) {
    Config next=config_;
    FillOptions nextFill=fill_;
    bool changed=false;
    for(int i=0;i<data.inputParameterChanges->getParameterCount();++i) {
      auto* queue=data.inputParameterChanges->getParameterData(i);
      if(!queue || queue->getPointCount()<=0)continue;
      int32 offset=0;ParamValue value=0;
      if(queue->getPoint(queue->getPointCount()-1,offset,value)!=kResultOk ||
         !std::isfinite(value))continue;
      value=std::clamp(value,0.,1.);
      const int v=static_cast<int>(std::lround(value*100.));
      switch(queue->getParameterId()) {
        case kGuitarGate:next.guitarGate=v;changed=true;break;
        case kBassGate:next.bassGate=v;changed=true;break;
        case kBassEnabled:next.bassOn=value>=0.5;changed=true;break;
        case kRootPitch:next.rootMidi=static_cast<int>(std::lround(value*127.));changed=true;break;
        case kBassVelocity:next.bassVelocity=1+static_cast<int>(std::lround(value*126.));changed=true;break;
        case kFillIntensity:nextFill.intensityPercent=v;changed=true;break;
        default:break;
      }
    }
    if(changed) {
      // Gate, root and bass changes use only fixed-size transforms at render
      // time. The fill generation remains a non-realtime operation.
      if(next.guitarGate!=config_.guitarGate || next.bassGate!=config_.bassGate ||
         next.rootMidi!=config_.rootMidi || next.bassOn!=config_.bassOn ||
         next.bassVelocity!=config_.bassVelocity)
        flushOnNextProcess_=true;
      live_.guitarGate=next.guitarGate;
      live_.bassGate=next.bassGate;
      live_.bassEnabled=next.bassOn;
      live_.bassVelocity=next.bassVelocity;
      live_.rootSemitones=next.rootMidi-preparedRootMidi_;
      config_.guitarGate=next.guitarGate;
      config_.bassGate=next.bassGate;
      config_.bassOn=next.bassOn;
      config_.bassVelocity=next.bassVelocity;
      config_.rootMidi=next.rootMidi;
      const int newFillIndex=std::clamp((nextFill.intensityPercent+5)/10,0,10);
      if(newFillIndex!=fillIndex_) { fillIndex_=newFillIndex;flushOnNextProcess_=true; }
      fill_.intensityPercent=nextFill.intensityPercent;
    }
  }
  if(data.numSamples<=0 || !data.outputEvents)return kResultOk;
  auto sink=[&](const ScheduledMidiEvent& midi) {
    Event e{};
    e.busIndex=midi.bus;
    e.sampleOffset=midi.sampleOffset;
    e.type=midi.on?Event::kNoteOnEvent:Event::kNoteOffEvent;
    if(midi.on) {
      e.noteOn.channel=0;
      e.noteOn.pitch=static_cast<int16>(midi.pitch);
      e.noteOn.velocity=static_cast<float>(midi.velocity)/127.0f;
      e.noteOn.noteId=-1;
    } else {
      e.noteOff.channel=0;
      e.noteOff.pitch=static_cast<int16>(midi.pitch);
      e.noteOff.noteId=-1;
    }
    return data.outputEvents->addEvent(e)==kResultOk;
  };
  if(flushOnNextProcess_) {
    if(scheduler_.flush(sink,0))flushOnNextProcess_=false;
    else return kResultOk; // Reattempt off events; do not add new notes.
  }
  const auto* ctx=data.processContext;
  const bool running=ctx && (ctx->state&ProcessContext::kPlaying)!=0;
  const bool havePosition=ctx && (ctx->state&ProcessContext::kProjectTimeMusicValid)!=0;
  const bool haveTempo=ctx && (ctx->state&ProcessContext::kTempoValid)!=0 &&
    ctx->tempo>0 && processSetup.sampleRate>0;
  const double start=havePosition?ctx->projectTimeMusic:0.;
  const double end=start+(haveTempo?double(data.numSamples)*ctx->tempo/
    (60.*processSetup.sampleRate):0.);
  const auto& active=fillVariants_[static_cast<std::size_t>(fillIndex_)];
  scheduler_.process(active.midi,start,end,data.numSamples,
    active.periodTicks,sink,
    running&&havePosition&&haveTempo,live_);
  return kResultOk;
}

namespace {
template<class T> bool writeExact(Steinberg::IBStream* stream,const T& value) {
  if(!stream)return false;
  Steinberg::int32 count=0;
  return stream->write(const_cast<T*>(&value),sizeof(T),&count)==Steinberg::kResultOk &&
    count==sizeof(T);
}
template<class T> bool readExact(Steinberg::IBStream* stream,T& value) {
  if(!stream)return false;
  Steinberg::int32 count=0;
  return stream->read(&value,sizeof(T),&count)==Steinberg::kResultOk &&
    count==sizeof(T);
}
}
tresult PLUGIN_API Processor::getState(IBStream* stream) {
  const uint32_t magic=0x31524641u; // AFR1
  const uint32_t version=2;
  const int32 count=static_cast<int32>(steps_.size());
  if(!writeExact(stream,magic)||!writeExact(stream,version)||!writeExact(stream,count))
    return kResultFalse;
  const std::array<int32,7> settings{
    config_.rootMidi,config_.bassVelocity,config_.bassOffset,
    config_.guitarGate,config_.bassGate,config_.ticksPerStep,
    config_.bassOn?1:0};
  for(auto v:settings) if(!writeExact(stream,v))return kResultFalse;
  if(!writeExact(stream,static_cast<int32>(fill_.intensityPercent)))return kResultFalse;
  for(const auto& step:steps_) {
    const std::array<int32,4> values{
      static_cast<int32>(step.articulation),step.degree,
      step.doubled?1:0,step.locked?1:0};
    for(auto v:values)if(!writeExact(stream,v))return kResultFalse;
  }
  return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream* stream) {
  uint32_t magic=0,version=0;
  int32 count=0;
  if(!readExact(stream,magic)||!readExact(stream,version)||
     !readExact(stream,count)||magic!=0x31524641u||(version!=1&&version!=2)||
     count<1||count>32)return kResultFalse;
  std::array<int32,7> settings{};
  for(auto& v:settings)if(!readExact(stream,v))return kResultFalse;
  if(settings[0]<0||settings[0]>127||
     settings[1]<1||settings[1]>127||
     settings[2]<-48||settings[2]>48||
     settings[3]<0||settings[3]>100||
     settings[4]<0||settings[4]>100||
     settings[5]<1||settings[5]>960||
     settings[6]<0||settings[6]>1)return kResultFalse;
  int32 restoredFill=0;
  if(version>=2 && (!readExact(stream,restoredFill)||restoredFill<0||restoredFill>100))return kResultFalse;
  std::vector<Step> parsed(static_cast<std::size_t>(count));
  for(auto& step:parsed) {
    std::array<int32,4> values{};
    for(auto& v:values)if(!readExact(stream,v))return kResultFalse;
    if(values[0]<0||values[0]>5||values[1]<-13||values[1]>15||
       values[2]<0||values[2]>1||values[3]<0||values[3]>1)return kResultFalse;
    step.articulation=static_cast<Articulation>(values[0]);
    step.degree=values[1];
    step.doubled=values[2]!=0;
    step.locked=values[3]!=0;
  }
  Config next=config_;
  next.rootMidi=settings[0];
  next.bassVelocity=settings[1];
  next.bassOffset=settings[2];
  next.guitarGate=settings[3];
  next.bassGate=settings[4];
  next.ticksPerStep=settings[5];
  next.bassOn=settings[6]!=0;
  // Commit only after complete state validation. No partial modification.
  // Construct fill variants outside the audio process callback.
  steps_=std::move(parsed);
  config_=next;
  fill_.intensityPercent=restoredFill;
  fillIndex_=std::clamp((restoredFill+5)/10,0,10);
  prepareFillVariants();
  live_.guitarGate=config_.guitarGate;
  live_.bassGate=config_.bassGate;
  live_.bassEnabled=config_.bassOn;
  live_.bassVelocity=config_.bassVelocity;
  preparedRootMidi_=config_.rootMidi;
  live_.rootSemitones=0;
  flushOnNextProcess_=true;
  return kResultOk;
}

}
