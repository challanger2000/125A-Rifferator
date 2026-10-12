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
    // Prepare one neutral candidate for every live step; a previously OFF
    // step must become playable immediately when selected in the GUI.
    // Actual articulation, pitch, velocity and rests are determined by
    // LiveControls, not stale arrangement snapshots.
    auto candidates=steps_;
    for(auto& step:candidates) {
      step.articulation=Articulation::PalmMute;
      step.degree=1;
      // DOUBLE belongs exclusively to the live block scheduler.
      // Pre-rendering two hits here and retriggering each would output four.
      step.doubled=false;
    }
    fillVariants_[static_cast<std::size_t>(index)]=arrange(candidates,config_,options,16);
  }
}
Processor::Processor() {
  setControllerClass(controllerId);
  steps_.resize(16);
  for(int i=0;i<16;++i) steps_[i].articulation=Articulation::PalmMute;
  for(int i=0;i<16;++i) {
    live_.guitarSteps[static_cast<std::size_t>(i)]=(i%2)==0;
    live_.guitarArticulation[static_cast<std::size_t>(i)]=2;
    live_.guitarDegree[static_cast<std::size_t>(i)]=1;
  }
  prepareFillVariants();
  // Immutable style snapshots: construction is off the process callback.
  for(int i=0;i<5;++i) {
    ebm_integration::BassPlan plan;
    plan.mode=ebm_integration::BassMode::Ebm;
    plan.sequence.style=static_cast<::rifferator::ebm::Style>(i);
    plan.sequence.root=52; // Existing guitar-root baseline is 40; bass octave -12.
    ebm_integration::prepare(ebmPrepared_[static_cast<std::size_t>(i)],plan);
  }
  DrumPattern candidates;
  candidates.kick.fill(true);candidates.snare.fill(true);candidates.hat.fill(true);
  drumNotes_=renderDrums(candidates,16);
  scheduler_.setDrums(&drumNotes_);
  live_.chordsEnabled=true;
  chordNotes_=makeChordCandidates();
  scheduler_.setChords(&chordNotes_);
  live_.drumsEnabled=drumPattern_.enabled;
  live_.drumSteps={drumPattern_.kick,drumPattern_.snare,drumPattern_.hat};
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
  addEventOutput(STR16("Chords Out"),1,kAux,BusInfo::kDefaultActive);
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
tresult PLUGIN_API Processor::setProcessing(TBool state) {
  // The base AudioEffect implementation returns kNotImplemented. This MIDI-only
  // processor has no processing-start allocation or worker lifecycle to manage.
  // Note flushing for transport changes and deactivation remains in process()/setActive().
  (void)state;
  return kResultOk;
}
tresult PLUGIN_API Processor::process(ProcessData& data) {
  if(data.inputParameterChanges) {
    Config next=config_;
    FillOptions nextFill=fill_;
    bool changed=false;
    // Guitar matrix edits can change the follow-bass and inferred chord root,
    // but cannot affect the independent drum bus.
    const auto guitarHarmonyChanged=[&]() noexcept {
      guitarFlushPending_=true;
      bassFlushPending_=true;
      chordFlushPending_=true;
    };
    // VST3 parameter queues have no semantic ordering across different IDs.
    // Resolve final LOCK states before applying ANY matrix edits. A locked
    // step is immutable within this entire callback, regardless of whether
    // its LOCK queue appears before or after articulation/degree/DOUBLE.
    for(int i=0;i<data.inputParameterChanges->getParameterCount();++i) {
      auto* queue=data.inputParameterChanges->getParameterData(i);
      if(!queue || queue->getPointCount()<=0)continue;
      const auto id=queue->getParameterId();
      if(id<kGuitarLockBase || id>=kGuitarLockBase+16)continue;
      int32 offset=0;ParamValue value=0;
      if(queue->getPoint(queue->getPointCount()-1,offset,value)!=kResultOk ||
         !std::isfinite(value))continue;
      const auto step=static_cast<std::size_t>(id-kGuitarLockBase);
      live_.guitarLocks[step]=value>=0.5;
      if(step<steps_.size())steps_[step].locked=live_.guitarLocks[step];
    }
    for(int i=0;i<data.inputParameterChanges->getParameterCount();++i) {
      auto* queue=data.inputParameterChanges->getParameterData(i);
      if(!queue || queue->getPointCount()<=0)continue;
      int32 offset=0;ParamValue value=0;
      if(queue->getPoint(queue->getPointCount()-1,offset,value)!=kResultOk ||
         !std::isfinite(value))continue;
      value=std::clamp(value,0.,1.);
      const int v=static_cast<int>(std::lround(value*100.));
      const auto id=queue->getParameterId();
      if(id==kPmMin || id==kPmMax) {
        const int velocity=std::clamp(static_cast<int>(std::lround(value*126.))+1,1,127);
        if(id==kPmMin)live_.pmMin=std::min(velocity,live_.pmMax);
        else live_.pmMax=std::max(velocity,live_.pmMin);
        continue;
      }
      if(id==kChordsEnabled) {
        const bool on=value>=0.5;
        if(live_.chordsEnabled!=on)chordFlushPending_=true;
        live_.chordsEnabled=on;
        continue;
      }
      if(id==kChordsQuality) {
        const int quality=std::clamp(static_cast<int>(std::lround(value*4.)),0,4);
        if(live_.chordQuality!=quality)chordFlushPending_=true;
        live_.chordQuality=quality;
        continue;
      }
      if(id==kChordsMode) {
        const bool root=value>=0.5;
        if(live_.chordsRootOnly!=root)chordFlushPending_=true;
        live_.chordsRootOnly=root;
        continue;
      }
      if(id>=kHybridRootBase && id<kHybridRootBase+16) {
        const auto bar=static_cast<std::size_t>(id-kHybridRootBase);
        const int root=std::clamp(static_cast<int>(std::lround(value*128.)),0,128);
        if(live_.harmony.roots[bar]!=root) {
          chordFlushPending_=true;
          // HYBRID derives bus-1 pitch from this exact harmony map. Its
          // sounding bass note must be released before applying the new root.
          // FOLLOW and standalone EBM bass do not use explicit bar roots.
          if(activeBassMode_==2)bassFlushPending_=true;
        }
        live_.harmony.roots[bar]=root;
        continue;
      }
      if(id==kBassMode) {
        const int mode=std::clamp(static_cast<int>(std::lround(value*2.)),0,2);
        if(bassMode_!=mode) {bassMode_=mode;bassSwitchPending_=true;}
        continue;
      }
      if(id==kEbmStyle) {
        const int style=std::clamp(static_cast<int>(std::lround(value*4.)),0,4);
        if(ebmStyle_!=style) {ebmStyle_=style;bassSwitchPending_=true;}
        continue;
      }
      if(id>=kGuitarDoubleBase && id<kGuitarDoubleBase+16) {
        const auto index=static_cast<std::size_t>(id-kGuitarDoubleBase);
        if(!live_.guitarLocks[index] && index<steps_.size()) {
          const bool doubled=value>=0.5;
          if(steps_[index].doubled!=doubled) {
            guitarFlushPending_=true;
            // DOUBLE is shared by guitar and FOLLOW bass. When an
            // already-playing step changes, both sets of held notes must
            // receive their NoteOff. EBM/HYBRID own an independent rhythm.
            if(activeBassMode_==0)bassFlushPending_=true;
          }
          steps_[index].doubled=doubled;
        }
        continue;
      }
      if(id>=kGuitarLockBase && id<kGuitarLockBase+16)
        continue; // applied atomically before step edits above
      if(id>=kGuitarArticulationBase && id<kGuitarArticulationBase+16) {
        const auto index=static_cast<std::size_t>(id-kGuitarArticulationBase);
        const int articulation=std::clamp(static_cast<int>(std::lround(value*5.)),0,5);
        if(!live_.guitarLocks[index] && live_.guitarArticulation[index]!=articulation) {
          live_.guitarArticulation[index]=articulation;
          if(index<steps_.size())steps_[index].articulation=static_cast<Articulation>(articulation);
          guitarHarmonyChanged();
        }
        continue;
      }
      if(id>=kGuitarDegreeBase && id<kGuitarDegreeBase+16) {
        const auto index=static_cast<std::size_t>(id-kGuitarDegreeBase);
        const int degree=std::clamp(static_cast<int>(std::lround(value*28.))-13,-13,15);
        if(!live_.guitarLocks[index] && live_.guitarDegree[index]!=degree) {
          live_.guitarDegree[index]=degree;
          if(index<steps_.size())steps_[index].degree=degree;
          guitarHarmonyChanged();
        }
        continue;
      }
      if(id>=kGuitarStepBase && id<kGuitarStepBase+16) {
        const int step=static_cast<int>(id-kGuitarStepBase);
        const bool enabled=value>=0.5;
        if(live_.guitarLocks[static_cast<std::size_t>(step)])continue;
        if(live_.guitarSteps[static_cast<std::size_t>(step)]!=enabled)
          guitarHarmonyChanged();
        live_.guitarSteps[static_cast<std::size_t>(step)]=enabled;
        continue;
      }
      if(id>=kDrumStepBase && id<kDrumStepBase+48) {
        const int index=static_cast<int>(id-kDrumStepBase);
        const bool active=value>=0.5;
        if(live_.drumSteps[static_cast<std::size_t>(index/16)][static_cast<std::size_t>(index%16)]!=active) {
          live_.drumSteps[static_cast<std::size_t>(index/16)][static_cast<std::size_t>(index%16)]=active;
          drumFlushPending_=true;
        }
        auto* row=index<16?&drumPattern_.kick:index<32?&drumPattern_.snare:&drumPattern_.hat;
        (*row)[static_cast<std::size_t>(index%16)]=active;
        continue;
      }
      if(id==kDrumEnabled) {
        const bool active=value>=0.5;
        if(live_.drumsEnabled!=active)drumFlushPending_=true;
        live_.drumsEnabled=active;drumPattern_.enabled=active;
        continue;
      }
      switch(id) {
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
      if(next.guitarGate!=config_.guitarGate)guitarFlushPending_=true;
      if(next.bassGate!=config_.bassGate ||
         next.bassOn!=config_.bassOn ||
         next.bassVelocity!=config_.bassVelocity)bassFlushPending_=true;
      if(next.rootMidi!=config_.rootMidi)guitarHarmonyChanged();
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
      if(newFillIndex!=fillIndex_) {
        fillIndex_=newFillIndex;
        guitarFlushPending_=true;
        bassFlushPending_=true;
      }
      fill_.intensityPercent=nextFill.intensityPercent;
    }
  }
  if(data.numSamples<=0 || !data.outputEvents)return kResultOk;
  const auto* ctx=data.processContext;
  const bool running=ctx && (ctx->state&ProcessContext::kPlaying)!=0;
  const bool havePosition=ctx && (ctx->state&ProcessContext::kProjectTimeMusicValid)!=0;
  const bool haveTempo=ctx && (ctx->state&ProcessContext::kTempoValid)!=0 &&
    ctx->tempo>0 && processSetup.sampleRate>0;
  const double start=havePosition?ctx->projectTimeMusic:0.;
  const double end=start+(haveTempo?double(data.numSamples)*ctx->tempo/
    (60.*processSetup.sampleRate):0.);
  const auto skipUnscheduledBlock=[&]() noexcept {
    if(!running || !havePosition || !haveTempo)return;
    scheduler_.advanceSkippedBlock(start,end);
    ebmScheduler_.advanceSkippedBlock(start,end);
  };
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
  if(bassSwitchPending_) {
    // Never activate a second bus-1 producer before the first one is silent.
    if(!scheduler_.flushBus(1,sink,0) || !ebmScheduler_.patternChanged(sink)) {
      skipUnscheduledBlock();return kResultOk;
    }
    activeBassMode_=bassMode_;
    bassSwitchPending_=false;
    // Both bass producers have released their old notes. Scheduling can
    // continue safely in this SAME callback so project recall or a live mode
    // switch never drops the first 16th/kick/chord at the transport boundary.
  }
  if(flushOnNextProcess_) {
    if(!scheduler_.flush(sink,0) || !ebmScheduler_.flush(sink,0)) {
      skipUnscheduledBlock();return kResultOk;
    }
    flushOnNextProcess_=false;
    drumFlushPending_=false;
    chordFlushPending_=false;
    guitarFlushPending_=false;
    bassFlushPending_=false;
  } else {
    // Local edits must only release notes from their own output bus.
    // Failed NoteOffs remain pending until a future callback accepts them.
    if(drumFlushPending_) {
      if(!scheduler_.flushBus(2,sink,0)) {
        skipUnscheduledBlock();return kResultOk;
      }
      drumFlushPending_=false;
    }
    if(chordFlushPending_) {
      if(!scheduler_.flushBus(3,sink,0)) {
        skipUnscheduledBlock();return kResultOk;
      }
      chordFlushPending_=false;
    }
    if(guitarFlushPending_) {
      if(!scheduler_.flushBus(0,sink,0)) {
        skipUnscheduledBlock();return kResultOk;
      }
      guitarFlushPending_=false;
    }
    if(bassFlushPending_) {
      // Bus 1 has two mutually exclusive MIDI producers. Flush both on a
      // change to the shared guitar guide or any bass control.
      if(!scheduler_.flushBus(1,sink,0) || !ebmScheduler_.flush(sink,0)) {
        skipUnscheduledBlock();return kResultOk;
      }
      bassFlushPending_=false;
    }
  }
  const auto& active=fillVariants_[static_cast<std::size_t>(fillIndex_)];
  // Derive a stable harmonic anchor from the actual live guitar step controls.
  // Explicit per-bar harmony edits override this automatic result.
  const int autoRoot=inferredGuitarRoot(live_.guitarSteps,live_.guitarArticulation,
                                       live_.guitarDegree,config_.scale,config_.rootMidi);
  HarmonyMap resolvedHarmony=live_.harmony;
  for(auto& root:resolvedHarmony.roots)if(root==128)root=autoRoot;
  LiveControls routing=live_;
  routing.absoluteRoot=config_.rootMidi;
  routing.scale=config_.scale;
  for(int i=0;i<16 && i<static_cast<int>(steps_.size());++i)
    routing.guitarDouble[static_cast<std::size_t>(i)]=steps_[static_cast<std::size_t>(i)].doubled;
  routing.bassOffset=config_.bassOffset;
  routing.preparedRoot=preparedRootMidi_;
  routing.harmony=resolvedHarmony;
  routing.externalBassMode=activeBassMode_!=0;
  scheduler_.process(active.midi,start,end,data.numSamples,
    active.periodTicks,sink,
    running&&havePosition&&haveTempo,routing);
  if(activeBassMode_!=0 && live_.bassEnabled) {
    // HYBRID falls back to explicit constant root until song-map input exists.
    const auto& prepared=ebmPrepared_[static_cast<std::size_t>(ebmStyle_)];
    ebm_integration::GuitarGuide guide;
    guide.enabled=&live_.guitarSteps;
    guide.articulation=&live_.guitarArticulation;
    guide.degrees=&live_.guitarDegree;
    guide.scale=config_.scale;
    guide.guitarRoot=config_.rootMidi;
    if(!ebmScheduler_.process(prepared,start,end,data.numSamples,
         running&&havePosition&&haveTempo,sink,
         config_.rootMidi-40,config_.bassGate,
         activeBassMode_==2?&resolvedHarmony:nullptr,config_.rootMidi,
         &guide,config_.bassVelocity))
      flushOnNextProcess_=true;
  } else if(!ebmScheduler_.flush(sink,0)) {
    flushOnNextProcess_=true;
  }
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
  const uint32_t version=9;
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
  // V3: three 16-step GM rows and enabled flag.
  if(!writeExact(stream,static_cast<int32>(drumPattern_.enabled?1:0)))return kResultFalse;
  for(const auto& row:{&drumPattern_.kick,&drumPattern_.snare,&drumPattern_.hat})
    for(bool hit:*row)if(!writeExact(stream,static_cast<int32>(hit?1:0)))return kResultFalse;
  for(bool active:live_.guitarSteps)
    if(!writeExact(stream,static_cast<int32>(active?1:0)))return kResultFalse;
  for(int value:live_.guitarArticulation)
    if(!writeExact(stream,static_cast<int32>(value)))return kResultFalse;
  for(int value:live_.guitarDegree)
    if(!writeExact(stream,static_cast<int32>(value)))return kResultFalse;
  for(bool locked:live_.guitarLocks)
    if(!writeExact(stream,static_cast<int32>(locked?1:0)))return kResultFalse;
  if(!writeExact(stream,static_cast<int32>(bassMode_)) ||
     !writeExact(stream,static_cast<int32>(ebmStyle_)))return kResultFalse;
  if(!writeExact(stream,static_cast<int32>(live_.chordsEnabled?1:0)) ||
     !writeExact(stream,static_cast<int32>(live_.chordsRootOnly?1:0)))return kResultFalse;
  for(int root:live_.harmony.roots)
    if(!writeExact(stream,static_cast<int32>(root)))return kResultFalse;
  if(!writeExact(stream,static_cast<int32>(live_.chordQuality)))return kResultFalse;
  if(!writeExact(stream,static_cast<int32>(live_.pmMin)) ||
     !writeExact(stream,static_cast<int32>(live_.pmMax)))return kResultFalse;
  return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream* stream) {
  uint32_t magic=0,version=0;
  int32 count=0;
  if(!readExact(stream,magic)||!readExact(stream,version)||
     !readExact(stream,count)||magic!=0x31524641u||(version!=1&&version!=2&&version!=3&&version!=4&&version!=5&&version!=6&&version!=7&&version!=8&&version!=9)||
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
  DrumPattern parsedDrums=DrumPattern::standard();
  if(version>=3) {
    int32 enabled=0;
    if(!readExact(stream,enabled)||enabled<0||enabled>1)return kResultFalse;
    parsedDrums.enabled=enabled!=0;
    for(auto* row:{&parsedDrums.kick,&parsedDrums.snare,&parsedDrums.hat})
      for(auto& hit:*row) {
        int32 value=0;
        if(!readExact(stream,value)||value<0||value>1)return kResultFalse;
        hit=value!=0;
      }
  }
  std::array<bool,16> restoredGuitar{};
  if(version>=4) {
    for(auto& active:restoredGuitar) {
      int32 v=0;
      if(!readExact(stream,v)||v<0||v>1)return kResultFalse;
      active=v!=0;
    }
  } else {
    for(int i=0;i<16;++i)
      restoredGuitar[static_cast<std::size_t>(i)]=
        i<static_cast<int>(parsed.size()) &&
        parsed[static_cast<std::size_t>(i)].articulation!=Articulation::Off;
  }
  std::array<int,16> restoredArt{};
  std::array<int,16> restoredDegree{};
  std::array<bool,16> restoredLocks{};
  if(version>=5) {
    for(auto& v:restoredArt) {
      int32 read=0;
      if(!readExact(stream,read)||read<0||read>5)return kResultFalse;
      v=read;
    }
    for(auto& v:restoredDegree) {
      int32 read=0;
      if(!readExact(stream,read)||read<-13||read>15)return kResultFalse;
      v=read;
    }
    for(auto& v:restoredLocks) {
      int32 read=0;
      if(!readExact(stream,read)||read<0||read>1)return kResultFalse;
      v=read!=0;
    }
  } else {
    for(int i=0;i<16;++i) {
      const auto& step=parsed[static_cast<std::size_t>(i%parsed.size())];
      restoredArt[static_cast<std::size_t>(i)]=static_cast<int>(step.articulation);
      restoredDegree[static_cast<std::size_t>(i)]=step.degree;
      restoredLocks[static_cast<std::size_t>(i)]=step.locked;
    }
  }
  int32 restoredBassMode=0,restoredEbmStyle=0;
  if(version>=6 &&
     (!readExact(stream,restoredBassMode)||restoredBassMode<0||restoredBassMode>2||
      !readExact(stream,restoredEbmStyle)||restoredEbmStyle<0||restoredEbmStyle>4))
    return kResultFalse;
  int32 restoredChords=0,restoredChordMode=0;
  HarmonyMap restoredHarmony;
  if(version>=7) {
    if(!readExact(stream,restoredChords)||restoredChords<0||restoredChords>1||
       !readExact(stream,restoredChordMode)||restoredChordMode<0||restoredChordMode>1)return kResultFalse;
    for(auto& root:restoredHarmony.roots) {
      int32 value=0;
      if(!readExact(stream,value)||value<0||value>128)return kResultFalse;
      root=value;
    }
  }
  int32 restoredChordQuality=0;
  if(version>=8 && (!readExact(stream,restoredChordQuality)||restoredChordQuality<0||restoredChordQuality>4))return kResultFalse;
  int32 restoredPmMin=30,restoredPmMax=50;
  if(version>=9 && (!readExact(stream,restoredPmMin) ||
      !readExact(stream,restoredPmMax) || restoredPmMin<1 ||
      restoredPmMax>127 || restoredPmMin>restoredPmMax))return kResultFalse;
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
  drumPattern_=parsedDrums;
  // The live scheduler reads these masks, not the serialized DrumPattern.
  // Without this sync, project recall restores the state but plays old steps.
  live_.drumsEnabled=drumPattern_.enabled;
  live_.drumSteps={drumPattern_.kick,drumPattern_.snare,drumPattern_.hat};
  drumNotes_=renderDrums(drumPattern_,16);
  scheduler_.setDrums(&drumNotes_);
  fill_.intensityPercent=restoredFill;
  fillIndex_=std::clamp((restoredFill+5)/10,0,10);
  prepareFillVariants();
  live_.chordsEnabled=restoredChords!=0;
  live_.chordsRootOnly=restoredChordMode!=0;
  live_.chordQuality=restoredChordQuality;
  live_.harmony=restoredHarmony;
  bassMode_=restoredBassMode;
  ebmStyle_=restoredEbmStyle;
  bassSwitchPending_=true;
  live_.guitarSteps=restoredGuitar;
  live_.guitarArticulation=restoredArt;
  live_.guitarDegree=restoredDegree;
  live_.guitarLocks=restoredLocks;
  // Preserve separately serialized Step snapshots verbatim. V5+ live
  // guitar arrays intentionally coexist with the historical step records;
  // overwriting either breaks lossless project roundtrips.
  live_.pmMin=restoredPmMin;
  live_.pmMax=restoredPmMax;
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
