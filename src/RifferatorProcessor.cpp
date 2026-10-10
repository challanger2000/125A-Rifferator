#include "RifferatorProcessor.hpp"
#include "pluginterfaces/vst/ivstevents.h"
#include <cstdint>

namespace a125::rifferator {
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
const FUID controllerId(0x125A4602,0x52494646,0x45524154,0x4F520100);
}
Processor::Processor() {
  setControllerClass(controllerId);
  steps_.resize(16);
  for(int i=0;i<16;i+=2) steps_[i].articulation=Articulation::PalmMute;
  rendered_=render(steps_,config_);
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
    scheduler_.flush(sink,0);
    flushOnNextProcess_=false;
  }
  const auto* ctx=data.processContext;
  const bool running=ctx && (ctx->state&ProcessContext::kPlaying)!=0;
  const bool havePosition=ctx && (ctx->state&ProcessContext::kProjectTimeMusicValid)!=0;
  const bool haveTempo=ctx && (ctx->state&ProcessContext::kTempoValid)!=0 &&
    ctx->tempo>0 && processSetup.sampleRate>0;
  const double start=havePosition?ctx->projectTimeMusic:0.;
  const double end=start+(haveTempo?double(data.numSamples)*ctx->tempo/
    (60.*processSetup.sampleRate):0.);
  scheduler_.process(rendered_,start,end,data.numSamples,
    int64_t(steps_.size())*config_.ticksPerStep,sink,
    running&&havePosition&&haveTempo);
  return kResultOk;
}
}
