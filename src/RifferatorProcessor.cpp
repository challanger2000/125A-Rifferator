#include "RifferatorProcessor.hpp"
#include "RifferatorParameters.hpp"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <algorithm>
#include <cmath>

namespace a125::rifferator {
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {const FUID controllerId(0x125A4602,0x52494646,0x45524154,0x4F520100);}
Processor::Processor() {
  setControllerClass(controllerId);
  config_.rootMidi=state_.root;config_.guitarGate=state_.gate;
  prepareVariants();
  live_.root=state_.root;live_.gate=state_.gate;
  live_.pmMin=state_.pmMin;live_.pmMax=state_.pmMax;
  for(int i=0;i<16;++i) {
    live_.enabled[i]=state_.enabled[i];
    live_.articulation[i]=static_cast<int>(state_.step[i].articulation);
    live_.degree[i]=state_.step[i].degree;
    live_.locked[i]=state_.step[i].locked;
    live_.doubled[i]=state_.step[i].doubled;
    live_.eighth[i]=state_.eighth[i];
  }
}
void Processor::prepareVariants() {
  std::vector<Step> candidates(16);
  for(auto& st:candidates){st.articulation=Articulation::PalmMute;st.degree=1;}
  for(int i=0;i<11;++i) {
    FillOptions options=fill_;
    options.intensityPercent=i*10;
    variants_[static_cast<std::size_t>(i)]=arrange(candidates,config_,options,16);
  }
  live_.preparedRoot=config_.rootMidi;
}
FUnknown* Processor::createInstance(void*) {return static_cast<IAudioProcessor*>(new Processor);}
tresult PLUGIN_API Processor::initialize(FUnknown* context) {
  if(AudioEffect::initialize(context)!=kResultOk)return kResultFalse;
  addEventInput(STR16("MIDI In"),16,kMain,BusInfo::kDefaultActive);
  addEventOutput(STR16("Guitar Out"),1,kMain,BusInfo::kDefaultActive);
  return kResultOk;
}
uint32 PLUGIN_API Processor::getProcessContextRequirements() {
  return IProcessContextRequirements::kNeedTempo|
    IProcessContextRequirements::kNeedProjectTimeMusic|
    IProcessContextRequirements::kNeedTransportState;
}
tresult PLUGIN_API Processor::canProcessSampleSize(int32 size) {
  return size==kSample32||size==kSample64?kResultTrue:kResultFalse;
}
tresult PLUGIN_API Processor::setActive(TBool active) {
  if(!active)flushPending_=true;
  return AudioEffect::setActive(active);
}
tresult PLUGIN_API Processor::setProcessing(TBool) {return kResultOk;}
tresult PLUGIN_API Processor::process(ProcessData& data) {
  if(data.inputParameterChanges) {
    // A LOCK is final for the whole block, regardless of queue enumeration.
    for(int i=0;i<data.inputParameterChanges->getParameterCount();++i) {
      auto* q=data.inputParameterChanges->getParameterData(i);
      if(!q||q->getPointCount()<=0)continue;
      const auto id=q->getParameterId();
      if(id<kGuitarLockBase||id>=kGuitarLockBase+16)continue;
      int32 offset=0;ParamValue v=0;
      if(q->getPoint(q->getPointCount()-1,offset,v)!=kResultOk||!std::isfinite(v))continue;
      const int step=static_cast<int>(id-kGuitarLockBase);
      state_.step[step].locked=v>=0.5;
      live_.locked[step]=v>=0.5;
    }
    for(int i=0;i<data.inputParameterChanges->getParameterCount();++i) {
      auto* q=data.inputParameterChanges->getParameterData(i);
      if(!q||q->getPointCount()<=0)continue;
      int32 offset=0;ParamValue v=0;
      if(q->getPoint(q->getPointCount()-1,offset,v)!=kResultOk||!std::isfinite(v))continue;
      v=std::clamp(v,0.,1.);
      const auto id=q->getParameterId();
      if(id>=kGuitarLockBase&&id<kGuitarLockBase+16)continue;
      if(id>=kGuitarStepBase&&id<kGuitarStepBase+16) {
        const int step=static_cast<int>(id-kGuitarStepBase);
        if(live_.locked[step])continue;
        const bool on=v>=.5;
        if(live_.enabled[step]!=on)flushPending_=true;
        live_.enabled[step]=on;state_.enabled[step]=on;continue;
      }
      if(id>=kGuitarArticulationBase&&id<kGuitarArticulationBase+16) {
        const int step=static_cast<int>(id-kGuitarArticulationBase);
        if(live_.locked[step])continue;
        const int art=static_cast<int>(std::lround(v*5.));
        if(live_.articulation[step]!=art)flushPending_=true;
        live_.articulation[step]=art;state_.step[step].articulation=static_cast<Articulation>(art);continue;
      }
      if(id>=kGuitarDegreeBase&&id<kGuitarDegreeBase+16) {
        const int step=static_cast<int>(id-kGuitarDegreeBase);
        if(live_.locked[step])continue;
        const int degree=static_cast<int>(std::lround(v*28.))-13;
        if(live_.degree[step]!=degree)flushPending_=true;
        live_.degree[step]=degree;state_.step[step].degree=degree;continue;
      }
      if(id>=kGuitarDoubleBase&&id<kGuitarDoubleBase+16) {
        const int step=static_cast<int>(id-kGuitarDoubleBase);
        if(live_.locked[step])continue;
        const bool doubled=v>=.5;
        if(live_.doubled[step]!=doubled)flushPending_=true;
        live_.doubled[step]=doubled;state_.step[step].doubled=doubled;continue;
      }
      if(id>=kGuitarLengthBase&&id<kGuitarLengthBase+16) {
        const int step=static_cast<int>(id-kGuitarLengthBase);
        if(live_.locked[step])continue;
        const bool eighth=v>=.5;
        if(live_.eighth[step]!=eighth)flushPending_=true;
        live_.eighth[step]=eighth;state_.eighth[step]=eighth;continue;
      }
      if(id==kRootPitch) {
        const int root=static_cast<int>(std::lround(v*127.));
        if(state_.root!=root)flushPending_=true;
        state_.root=root;live_.root=root;continue;
      }
      if(id==kGuitarGate) {
        const int gate=static_cast<int>(std::lround(v*100.));
        if(state_.gate!=gate)flushPending_=true;
        state_.gate=gate;live_.gate=gate;continue;
      }
      if(id==kFillIntensity) {
        const int fill=static_cast<int>(std::lround(v*100.));
        const int next=std::clamp((fill+5)/10,0,10);
        if(fillIndex_!=next)flushPending_=true;
        fillIndex_=next;state_.fill=fill;continue;
      }
      if(id==kPmMin||id==kPmMax) {
        const int vel=1+static_cast<int>(std::lround(v*126.));
        if(id==kPmMin){state_.pmMin=std::min(vel,state_.pmMax);live_.pmMin=state_.pmMin;}
        else {state_.pmMax=std::max(vel,state_.pmMin);live_.pmMax=state_.pmMax;}
        continue;
      }
    }
  }
  if(data.numSamples<=0||!data.outputEvents)return kResultOk;
  auto sink=[&](const ScheduledMidiEvent& midi) noexcept -> bool {
    Event e{};
    e.busIndex=0;
    e.sampleOffset=midi.sampleOffset;
    e.type=midi.on?Event::kNoteOnEvent:Event::kNoteOffEvent;
    if(midi.on) {
      e.noteOn.channel=0;e.noteOn.pitch=static_cast<int16>(midi.pitch);
      e.noteOn.velocity=static_cast<float>(midi.velocity)/127.f;
      e.noteOn.noteId=-1;
    } else {
      e.noteOff.channel=0;e.noteOff.pitch=static_cast<int16>(midi.pitch);
      e.noteOff.noteId=-1;
    }
    return data.outputEvents->addEvent(e)==kResultOk;
  };
  if(flushPending_) {
    if(!scheduler_.flush(sink,0))return kResultOk;
    flushPending_=false;
  }
  const auto* ctx=data.processContext;
  const bool playing=ctx&&(ctx->state&ProcessContext::kPlaying)!=0;
  const bool havePos=ctx&&(ctx->state&ProcessContext::kProjectTimeMusicValid)!=0;
  const bool haveTempo=ctx&&(ctx->state&ProcessContext::kTempoValid)!=0&&
     ctx->tempo>0&&processSetup.sampleRate>0;
  const double start=havePos?ctx->projectTimeMusic:0.;
  const double end=start+(haveTempo?double(data.numSamples)*ctx->tempo/
      (60.*processSetup.sampleRate):0.);
  scheduler_.process(variants_[static_cast<std::size_t>(fillIndex_)].guitar,
     start,end,data.numSamples,variants_[static_cast<std::size_t>(fillIndex_)].periodTicks,
     sink,playing&&havePos&&haveTempo,live_);
  return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream* stream) {
  return writeSnapshot(stream,state_)?kResultOk:kResultFalse;
}
tresult PLUGIN_API Processor::setState(IBStream* stream) {
  GuitarSnapshot incoming;
  if(!readSnapshot(stream,incoming))return kResultFalse;
  state_=incoming;
  config_.rootMidi=state_.root;
  config_.guitarGate=state_.gate;
  fill_.intensityPercent=state_.fill;
  prepareVariants();
  fillIndex_=std::clamp((state_.fill+5)/10,0,10);
  live_.root=state_.root;live_.gate=state_.gate;
  live_.pmMin=state_.pmMin;live_.pmMax=state_.pmMax;
  for(int i=0;i<16;++i) {
    live_.enabled[i]=state_.enabled[i];
    live_.articulation[i]=static_cast<int>(state_.step[i].articulation);
    live_.degree[i]=state_.step[i].degree;
    live_.doubled[i]=state_.step[i].doubled;
    live_.eighth[i]=state_.eighth[i];
    live_.locked[i]=state_.step[i].locked;
  }
  flushPending_=true;
  return kResultOk;
}
} // namespace a125::rifferator
