#include "../src/RifferatorProcessor.hpp"
#include "../src/RifferatorParameters.hpp"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/common/memorystream.h"
#ifdef NDEBUG
#error Regression asserts disabled in CI
#endif
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdint>
using namespace a125::rifferator;
using Steinberg::MemoryStream;
int main(){
  Processor original;
  MemoryStream full;
  assert(original.getState(&full)==Steinberg::kResultOk);
  assert(full.getSize()>100);
  assert(full.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
  Processor restored;
  assert(restored.setState(&full)==Steinberg::kResultOk);
  MemoryStream again;
  assert(restored.getState(&again)==Steinberg::kResultOk);
  assert(again.getSize()==full.getSize());
  assert(std::memcmp(again.getData(),full.getData(),full.getSize())==0);
  // Truncated stream must fail; subsequent state must stay unchanged.
  MemoryStream truncated(full.getData(),full.getSize()-1);
  assert(restored.setState(&truncated)!=Steinberg::kResultOk);
  MemoryStream retained;
  assert(restored.getState(&retained)==Steinberg::kResultOk);
  assert(retained.getSize()==full.getSize());
  assert(std::memcmp(retained.getData(),full.getData(),full.getSize())==0);
  // Legacy V6 projects end after BassMode and EBMStyle, i.e. 19 ints
  // before current V8's chords enabled/mode, 16 roots and quality.
  assert(full.getSize()>=76);
  const auto legacySize=static_cast<std::size_t>(full.getSize())-19u*sizeof(std::int32_t);
  auto* data=reinterpret_cast<const unsigned char*>(full.getData());
  std::vector<unsigned char> legacy(data,data+legacySize);
  legacy[4]=6;legacy[5]=legacy[6]=legacy[7]=0;
  MemoryStream old(legacy.data(),static_cast<Steinberg::TSize>(legacy.size()));
  Processor migrated;
  assert(migrated.setState(&old)==Steinberg::kResultOk);
  MemoryStream upgraded;
  assert(migrated.getState(&upgraded)==Steinberg::kResultOk);
  assert(upgraded.getSize()==full.getSize());
// V8 non-default full snapshot: validate every serialized feature class,
// including MIDI register controls that a default-only roundtrip misses.
  {
    std::vector<unsigned char> changed(data,data+full.getSize());
    auto write32=[&](std::size_t index,std::int32_t value) {
      assert((index+1)*4<=changed.size());
      std::memcpy(changed.data()+index*4,&value,4);
    };
    std::int32_t count=0;
    std::memcpy(&count,changed.data()+8,4);
    assert(count>=1 && count<=32);
    write32(3,52);   // Root E3
    write32(4,117);  // Bass velocity
    write32(5,-24);  // Bass offset
    write32(6,58);   // Guitar gate
    write32(7,72);   // Bass gate
    write32(9,0);    // Bass disabled
    write32(10,75);  // Fill intensity
    const std::size_t afterSteps=11u+static_cast<std::size_t>(count)*4u;
    write32(afterSteps,0);     // Drums disabled
    write32(afterSteps+1,0);   // First kick disabled
    const std::size_t guitarOn=afterSteps+49;
    write32(guitarOn+0,0);
    const std::size_t art=guitarOn+16;
    write32(art+0,4);
    const std::size_t deg=art+16;
    write32(deg+0,7);
    const std::size_t locks=deg+16;
    write32(locks+0,1);
    const std::size_t bassMode=locks+16;
    write32(bassMode,2);       // HYBRID
    write32(bassMode+1,3);     // EBM style
    write32(bassMode+2,1);     // Chords enabled
    write32(bassMode+3,1);     // Chords root-only
    write32(bassMode+4,45);    // First bar explicit root
    write32(bassMode+20,4);    // Sus4 chord quality
    MemoryStream custom(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    Processor customProcessor;
    assert(customProcessor.setState(&custom)==Steinberg::kResultOk);
    MemoryStream customSaved;
    assert(customProcessor.getState(&customSaved)==Steinberg::kResultOk);
    assert(customSaved.getSize()==static_cast<Steinberg::TSize>(changed.size()));
    assert(std::memcmp(customSaved.getData(),changed.data(),changed.size())==0);
    // Invalid later field: do not publish any partially applied state.
    write32(bassMode+20,99);
    MemoryStream corrupt(changed.data(),static_cast<Steinberg::TSize>(changed.size()));
    assert(customProcessor.setState(&corrupt)!=Steinberg::kResultOk);
    MemoryStream afterReject;
    assert(customProcessor.getState(&afterReject)==Steinberg::kResultOk);
    assert(afterReject.getSize()==customSaved.getSize());
    assert(std::memcmp(afterReject.getData(),customSaved.getData(),customSaved.getSize())==0);
  }

// Legacy V1 snapshot with ONE step must restore safely and survive subsequent
// VST3 parameter processing for the highest addressed guitar step (index 15).
  {
    std::vector<unsigned char> small(data,data+full.getSize());
    auto w=[&](std::size_t word,std::int32_t value) {
      assert((word+1)*4<=small.size());
      std::memcpy(small.data()+word*4,&value,4);
    };
    std::int32_t originalCount=0;
    std::memcpy(&originalCount,small.data()+8,4);
    assert(originalCount==16);
    // V1: magic/version/count + 7 settings + 1 step (four words).
    // V2 introduced the fill word, so remove it before extracting V1 step.
    small.erase(small.begin()+10u*sizeof(std::int32_t),
                small.begin()+11u*sizeof(std::int32_t));
    small.resize(14u*sizeof(std::int32_t));
    w(1,1);w(2,1);
    MemoryStream oneStep(small.data(),static_cast<Steinberg::TSize>(small.size()));
    Processor shortPattern;
    assert(shortPattern.setState(&oneStep)==Steinberg::kResultOk);
    // Exercise the actual Processor::process automation path on step 16.
    // Previously this indexed steps_[15] in a restored one-element vector.
    Steinberg::Vst::ParameterChanges changes(2);
    auto queueValue=[&](Steinberg::Vst::ParamID id,double normalized) {
      Steinberg::int32 queueIndex=0,pointIndex=0;
      auto* queue=changes.addParameterData(id,queueIndex);
      assert(queue!=nullptr);
      assert(queue->addPoint(0,normalized,pointIndex)==Steinberg::kResultOk);
    };
    queueValue(kGuitarArticulationBase+15,4./5.);
    queueValue(kGuitarDegreeBase+15,20./28.);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=0;
    block.inputParameterChanges=&changes;
    assert(shortPattern.process(block)==Steinberg::kResultOk);
    // Lock in a separate callback; simultaneous LOCK+edits intentionally
    // resolve to the final lock state and would reject the edits.
    Steinberg::Vst::ParameterChanges lockStep(1);
    Steinberg::int32 qi=0,pi=0;
    auto* lockQueue=lockStep.addParameterData(kGuitarLockBase+15,qi);
    assert(lockQueue && lockQueue->addPoint(0,1.,pi)==Steinberg::kResultOk);
    block.inputParameterChanges=&lockStep;
    assert(shortPattern.process(block)==Steinberg::kResultOk);
    MemoryStream saved;
    assert(shortPattern.getState(&saved)==Steinberg::kResultOk);
    std::int32_t savedCount=0;
    std::memcpy(&savedCount,static_cast<const char*>(saved.getData())+8,4);
    assert(savedCount==1);
    // State V8 contains fixed-size 16-step live controls after the one
    // legacy pattern step. Verify all three high-index edits survived.
    const auto* words=static_cast<const char*>(saved.getData());
    auto wordAt=[&](std::size_t index) {
      std::int32_t v=0;
      assert((index+1)*4<=static_cast<std::size_t>(saved.getSize()));
      std::memcpy(&v,words+index*4,4);
      return v;
    };
    const std::size_t base=11u+4u+49u;
    assert(wordAt(base+16u+15u)==4);
    assert(wordAt(base+32u+15u)==7);
    assert(wordAt(base+48u+15u)==1);
  }

  // Host automation of an already locked step must be rejected by the
  // Processor just as it is by the Controller. Saving the project must retain
  // the original guitar ACTIVE, articulation, DEGREE and DOUBLE values.
  {
    std::vector<unsigned char> snapshot(data,data+full.getSize());
    auto write=[&](std::size_t word,std::int32_t value){
      assert((word+1)*4<=snapshot.size());
      std::memcpy(snapshot.data()+word*4,&value,4);
    };
    const std::size_t doubleWord=11u+2u;
    const std::size_t activeWord=11u+16u*4u+49u;
    write(doubleWord,0);
    write(activeWord,0);
    write(activeWord+16u,4);
    write(activeWord+32u,7);
    write(activeWord+48u,1);
    MemoryStream lockedState(snapshot.data(),
      static_cast<Steinberg::TSize>(snapshot.size()));
    Processor locked;
    assert(locked.setState(&lockedState)==Steinberg::kResultOk);
    Steinberg::Vst::ParameterChanges changes(4);
    auto edit=[&](Steinberg::Vst::ParamID id,double value){
      Steinberg::int32 qi=0,pi=0;
      auto* q=changes.addParameterData(id,qi);
      assert(q && q->addPoint(0,value,pi)==Steinberg::kResultOk);
    };
    edit(kGuitarStepBase,1.);
    edit(kGuitarArticulationBase,.4);
    edit(kGuitarDegreeBase,0.);
    edit(kGuitarDoubleBase,1.);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=0;
    block.inputParameterChanges=&changes;
    assert(locked.process(block)==Steinberg::kResultOk);
    MemoryStream saved;
    assert(locked.getState(&saved)==Steinberg::kResultOk);
    assert(saved.getSize()==static_cast<Steinberg::TSize>(snapshot.size()));
    assert(std::memcmp(saved.getData(),snapshot.data(),snapshot.size())==0);
  }

  // Different parameter queues have no reliable cross-ID ordering. Both
  // orders of LOCK + guitar edits must produce IDENTICAL state: locking a
  // step rejects all new edits; unlocking it permits the same edits.
  // This is the per-block final-lock policy (not sample-accurate lock timing).
  {
    auto run=[&](bool lockedInitially,bool lockFirst) {
      Processor processor;
      Steinberg::Vst::ProcessData block{};
      block.numSamples=0;
      if(lockedInitially) {
        Steinberg::Vst::ParameterChanges initial(1);
        Steinberg::int32 qi=0,pi=0;
        auto* q=initial.addParameterData(kGuitarLockBase,qi);
        assert(q && q->addPoint(0,1.,pi)==Steinberg::kResultOk);
        block.inputParameterChanges=&initial;
        assert(processor.process(block)==Steinberg::kResultOk);
      }
      Steinberg::Vst::ParameterChanges edits(5);
      auto edit=[&](Steinberg::Vst::ParamID id,double value) {
        Steinberg::int32 qi=0,pi=0;
        auto* q=edits.addParameterData(id,qi);
        assert(q && q->addPoint(0,value,pi)==Steinberg::kResultOk);
      };
      if(lockFirst)edit(kGuitarLockBase,lockedInitially?0.:1.);
      edit(kGuitarStepBase,0.);
      edit(kGuitarArticulationBase,.8);
      edit(kGuitarDegreeBase,20./28.);
      edit(kGuitarDoubleBase,1.);
      if(!lockFirst)edit(kGuitarLockBase,lockedInitially?0.:1.);
      block.inputParameterChanges=&edits;
      assert(processor.process(block)==Steinberg::kResultOk);
      MemoryStream snapshot;
      assert(processor.getState(&snapshot)==Steinberg::kResultOk);
      const auto* bytes=static_cast<const char*>(snapshot.getData());
      const auto word=[&](std::size_t offset) {
        std::int32_t v=0;
        assert((offset+1)*4<=static_cast<std::size_t>(snapshot.getSize()));
        std::memcpy(&v,bytes+offset*4,4);return v;
      };
      const std::size_t base=11u+16u*4u+49u;
      return std::array<std::int32_t,5>{
        word(base),word(base+16u),word(base+32u),word(base+48u),word(13u)
      };
    };
    const auto lockedFirst=run(false,true);
    const auto lockedLast=run(false,false);
    assert(lockedFirst==lockedLast);
    assert((lockedFirst==std::array<std::int32_t,5>{1,2,1,1,0}));
    const auto unlockedFirst=run(true,true);
    const auto unlockedLast=run(true,false);
    assert(unlockedFirst==unlockedLast);
    assert((unlockedFirst==std::array<std::int32_t,5>{0,4,7,0,1}));
  }

  // Fast host automation contract: this V1 processor applies the last valid
  // point in each parameter queue once per block, including zero-sample flushes.
  // Lock/step interleaving must not corrupt serialized state.
  {
    Processor automated;
    Steinberg::Vst::ParameterChanges changes(5);
    auto push=[&](Steinberg::Vst::ParamID id,int offset,double value){
      Steinberg::int32 queueIndex=0,pointIndex=0;
      auto* q=changes.addParameterData(id,queueIndex);
      assert(q && q->addPoint(offset,value,pointIndex)==Steinberg::kResultOk);
    };
    push(kGuitarGate,0,.10);push(kGuitarGate,16,.75);
    push(kBassGate,0,.05);push(kBassGate,16,.63);
    push(kGuitarStepBase,0,0.);push(kGuitarStepBase,16,1.);
    push(kGuitarArticulationBase,0,.2);push(kGuitarArticulationBase,16,.8);
    push(kGuitarLockBase,16,1.);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=0;block.inputParameterChanges=&changes;
    assert(automated.process(block)==Steinberg::kResultOk);
    MemoryStream saved;
    assert(automated.getState(&saved)==Steinberg::kResultOk);
    const auto* bytes=static_cast<const char*>(saved.getData());
    auto getWord=[&](std::size_t index){
      assert((index+1)*4<=static_cast<std::size_t>(saved.getSize()));
      std::int32_t value=0;std::memcpy(&value,bytes+index*4,4);return value;
    };
    assert(getWord(6)==75 && getWord(7)==63);
    const std::size_t base=11u+16u*4u+49u;
    assert(getWord(base)==1);               // step remains at its default
    assert(getWord(base+16u)==2);           // same-block LOCK wins over articulation
    assert(getWord(base+48u)==1);           // final lock
    Processor replay;
    assert(saved.seek(0,Steinberg::IBStream::kIBSeekSet,nullptr)==Steinberg::kResultOk);
    assert(replay.setState(&saved)==Steinberg::kResultOk);
    MemoryStream again;
    assert(replay.getState(&again)==Steinberg::kResultOk);
    assert(saved.getSize()==again.getSize());
    assert(std::memcmp(saved.getData(),again.getData(),saved.getSize())==0);
  }

  // Real VST3 processor path: PM velocity endpoints must change the output
  // NoteOn velocity and FOLLOW bass must get exactly two DOUBLE attacks.
  // Only a single MIDI pitch is sent per guitar attack (library voicings).
  {
    Processor playing;
    Steinberg::Vst::ParameterChanges params(4);
    auto edit=[&](Steinberg::Vst::ParamID id,double value){
      Steinberg::int32 qi=0,pi=0;
      auto* q=params.addParameterData(id,qi);
      assert(q && q->addPoint(0,value,pi)==Steinberg::kResultOk);
    };
    edit(kPmMin,34./126.); // PM MIN 35
    edit(kPmMax,39./126.); // PM MAX 40
    edit(kGuitarStepBase+1,1.);
    edit(kGuitarDoubleBase,1.);
    Steinberg::Vst::ProcessData paramBlock{};
    paramBlock.numSamples=0;paramBlock.inputParameterChanges=&params;
    assert(playing.process(paramBlock)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=12000;
    assert(playing.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.projectTimeMusic=0.;ctx.tempo=120.;
    Steinberg::Vst::EventList output(512);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=12000;block.processContext=&ctx;
    block.outputEvents=&output;
    assert(playing.process(block)==Steinberg::kResultOk);
    std::vector<int> guitarAt,bassAt,guitarVelocity;
    int guitarOff=0,bassOff=0;
    for(Steinberg::int32 i=0;i<output.getEventCount();++i) {
      Steinberg::Vst::Event e{};
      assert(output.getEvent(i,e)==Steinberg::kResultOk);
      if(e.busIndex==0) {
        if(e.type==Steinberg::Vst::Event::kNoteOnEvent) {
          assert(e.noteOn.pitch==40);
          guitarAt.push_back(e.sampleOffset);
          guitarVelocity.push_back(static_cast<int>(std::lround(e.noteOn.velocity*127.)));
        } else if(e.type==Steinberg::Vst::Event::kNoteOffEvent)++guitarOff;
      }
      if(e.busIndex==1) {
        if(e.type==Steinberg::Vst::Event::kNoteOnEvent) {
          assert(e.noteOn.pitch==28);
          assert(static_cast<int>(std::lround(e.noteOn.velocity*127.))==100);
          bassAt.push_back(e.sampleOffset);
        } else if(e.type==Steinberg::Vst::Event::kNoteOffEvent)++bassOff;
      }
    }
    assert((guitarAt==std::vector<int>{0,3000,6000}));
    assert((bassAt==std::vector<int>{0,3000,6000}));
    assert((guitarVelocity==std::vector<int>{40,39,36}));
    assert(guitarOff==3 && bassOff==3);
  }

  // DOUBLE is shared by Guitar and FOLLOW Bass only. In EBM mode a
  // DOUBLE edit must not interrupt the independently sounding bass.
  {
    Processor playing;
    Steinberg::Vst::ParameterChanges mode(1);
    Steinberg::int32 qi=0,pi=0;
    auto* selection=mode.addParameterData(kBassMode,qi);
    assert(selection && selection->addPoint(0,.5,pi)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessData zero{};
    zero.numSamples=0;zero.inputParameterChanges=&mode;
    assert(playing.process(zero)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=600;
    assert(playing.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.projectTimeMusic=0.;ctx.tempo=120.;
    Steinberg::Vst::ProcessData block{};
    block.numSamples=600;block.processContext=&ctx;
    Steinberg::Vst::EventList first(512);
    block.outputEvents=&first;
    assert(playing.process(block)==Steinberg::kResultOk);
    bool started=false;
    for(Steinberg::int32 i=0;i<first.getEventCount();++i) {
      Steinberg::Vst::Event e{};
      assert(first.getEvent(i,e)==Steinberg::kResultOk);
      if(e.busIndex==1 && e.type==Steinberg::Vst::Event::kNoteOnEvent)
        started=true;
    }
    assert(started);
    Steinberg::Vst::ParameterChanges dbl(1);
    qi=0;pi=0;
    auto* q=dbl.addParameterData(kGuitarDoubleBase,qi);
    assert(q && q->addPoint(0,1.,pi)==Steinberg::kResultOk);
    Steinberg::Vst::EventList later(512);
    block.outputEvents=&later;
    block.inputParameterChanges=&dbl;
    ctx.projectTimeMusic=.025;
    assert(playing.process(block)==Steinberg::kResultOk);
    for(Steinberg::int32 i=0;i<later.getEventCount();++i) {
      Steinberg::Vst::Event e{};
      assert(later.getEvent(i,e)==Steinberg::kResultOk);
      assert(!(e.busIndex==1 && e.type==Steinberg::Vst::Event::kNoteOffEvent &&
               e.sampleOffset==0));
    }
  }

  // End-to-end VST3 event test: every edit releases exactly those note
  // buses whose musical data changes, never unrelated held voices.
  // Exercise actual ProcessData parameter queues and EventList outputs.
  for(int scenario=0;scenario<16;++scenario) {
    const bool changeDrums=scenario==0 || scenario==2 || scenario==4 || scenario==14;
    const bool changeChords=scenario==1 || scenario==2 || scenario==4 || scenario==15;
    const bool changeDouble=scenario==3 || scenario==4;
    const bool changeGuitarMatrix=scenario>=5 && scenario<=7 || scenario==14;
    const bool changeRoot=scenario==12;
    const bool changeGuitarGate=scenario==8;
    const bool changeBass=scenario>=9 && scenario<=11 || scenario==15;
    const bool changeFill=scenario==13;
    const bool expectGuitar=changeDouble || changeGuitarMatrix ||
       changeRoot || changeGuitarGate || changeFill;
    const bool expectBass=changeDouble || changeGuitarMatrix || changeRoot ||
       changeBass || changeFill;
    const bool expectChords=changeChords || changeGuitarMatrix || changeRoot;
    Processor playing;
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=600;
    assert(playing.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.tempo=120.;
    Steinberg::Vst::ProcessData block{};
    block.numSamples=600;block.processContext=&ctx;
    Steinberg::Vst::EventList first(512);
    block.outputEvents=&first;
    ctx.projectTimeMusic=0.;
    assert(playing.process(block)==Steinberg::kResultOk);
    bool seenOn[4]{};
    for(Steinberg::int32 i=0;i<first.getEventCount();++i){
      Steinberg::Vst::Event e{};
      assert(first.getEvent(i,e)==Steinberg::kResultOk);
      if(e.type==Steinberg::Vst::Event::kNoteOnEvent)
        seenOn[e.busIndex]=true;
    }
    assert(seenOn[0] && seenOn[1] && seenOn[2] && seenOn[3]);

    Steinberg::Vst::ParameterChanges edits(3);
    auto change=[&](Steinberg::Vst::ParamID id,double value) {
      Steinberg::int32 queueIndex=0,pointIndex=0;
      auto* q=edits.addParameterData(id,queueIndex);
      assert(q && q->addPoint(0,value,pointIndex)==Steinberg::kResultOk);
    };
    if(changeDrums)change(kDrumStepBase,0.);
    if(changeChords)change(kChordsQuality,.25);
    if(changeDouble)change(kGuitarDoubleBase,1.);
    if(scenario==5)change(kGuitarStepBase,0.);
    if(scenario==6)change(kGuitarArticulationBase,.8);
    if(scenario==7 || scenario==14)change(kGuitarDegreeBase,17./28.);
    if(changeGuitarGate)change(kGuitarGate,.5);
    if(scenario==9 || scenario==15)change(kBassGate,.5);
    if(scenario==10)change(kBassVelocity,.5);
    if(scenario==11)change(kBassEnabled,0.);
    if(changeRoot)change(kRootPitch,52./127.);
    if(changeFill)change(kFillIntensity,.5);
    Steinberg::Vst::EventList second(512);
    block.outputEvents=&second;
    block.inputParameterChanges=&edits;
    ctx.projectTimeMusic=.025;
    assert(playing.process(block)==Steinberg::kResultOk);
    bool seenOff[4]{};
    for(Steinberg::int32 i=0;i<second.getEventCount();++i){
      Steinberg::Vst::Event e{};
      assert(second.getEvent(i,e)==Steinberg::kResultOk);
      if(e.type!=Steinberg::Vst::Event::kNoteOffEvent || e.sampleOffset!=0)continue;
      assert(e.busIndex>=0 && e.busIndex<4);
      seenOff[e.busIndex]=true;
    }
    assert(seenOff[0]==expectGuitar);
    assert(seenOff[1]==expectBass);
    assert(seenOff[2]==changeDrums);
    assert(seenOff[3]==expectChords);
  }

  // Backpressure regression: if the host accepts only one NoteOff during
  // a combined edit, retry ONLY the remaining outstanding bus releases
  // in the next callback. Never lose a release or duplicate the accepted one.
  {
    Processor playing;
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=600;
    assert(playing.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.tempo=120.;
    Steinberg::Vst::ProcessData block{};
    block.numSamples=600;block.processContext=&ctx;
    Steinberg::Vst::EventList first(512);
    block.outputEvents=&first;
    ctx.projectTimeMusic=0.;
    assert(playing.process(block)==Steinberg::kResultOk);
    Steinberg::Vst::ParameterChanges edits(3);
    auto change=[&](Steinberg::Vst::ParamID id,double value) {
      Steinberg::int32 qi=0,pi=0;
      auto* q=edits.addParameterData(id,qi);
      assert(q && q->addPoint(0,value,pi)==Steinberg::kResultOk);
    };
    change(kDrumStepBase,0.);
    change(kChordsQuality,.25);
    change(kGuitarDoubleBase,1.);
    Steinberg::Vst::EventList limited(1);
    block.outputEvents=&limited;
    block.inputParameterChanges=&edits;
    ctx.projectTimeMusic=.025;
    assert(playing.process(block)==Steinberg::kResultOk);
    assert(limited.getEventCount()==1);
    Steinberg::Vst::Event firstOff{};
    assert(limited.getEvent(0,firstOff)==Steinberg::kResultOk);
    assert(firstOff.type==Steinberg::Vst::Event::kNoteOffEvent &&
           firstOff.busIndex==2 && firstOff.noteOff.pitch==36);
    Steinberg::Vst::EventList retry(512);
    block.outputEvents=&retry;
    block.inputParameterChanges=nullptr;
    ctx.projectTimeMusic=.05;
    assert(playing.process(block)==Steinberg::kResultOk);
    bool off[4]{};
    bool duplicate=false;
    for(Steinberg::int32 i=0;i<retry.getEventCount();++i) {
      Steinberg::Vst::Event e{};
      assert(retry.getEvent(i,e)==Steinberg::kResultOk);
      if(e.type!=Steinberg::Vst::Event::kNoteOffEvent ||
         e.sampleOffset!=0)continue;
      off[e.busIndex]=true;
      if(e.busIndex==2 && e.noteOff.pitch==36)duplicate=true;
    }
    assert(off[0] && off[2] && off[3] && !off[1] && !duplicate);
  }

  // End-to-end live LOCK: pre-rendered FILL transposition must be masked
  // on both real guitar and follow-bass VST3 outputs, while the GUI-selected
  // musical degree remains active.
  {
    std::vector<Step> reference(16);
    for(auto& step:reference){step.articulation=Articulation::PalmMute;step.degree=1;}
    Config config;
    FillOptions fill;
    fill.intensityPercent=100;
    const auto arrangement=arrange(reference,config,fill,16);
    int step=-1,delta=0;
    std::int64_t onset=0;
    for(const auto& note:arrangement.midi.guitar) {
      if(note.startTick<15*3840 || note.startTick>=16*3840 ||
         note.note==config.rootMidi)continue;
      const int candidate=static_cast<int>((note.startTick/240)%16);
      if(candidate<8)continue;
      step=candidate;delta=note.note-config.rootMidi;
      onset=note.startTick;break;
    }
    assert(step>=8 && step<16 && delta!=0);
    auto playFill=[&](bool locked) {
      Processor playing;
      Steinberg::Vst::ParameterChanges changes(4);
      auto edit=[&](Steinberg::Vst::ParamID id,double value){
        Steinberg::int32 qi=0,pi=0;
        auto* q=changes.addParameterData(id,qi);
        assert(q && q->addPoint(0,value,pi)==Steinberg::kResultOk);
      };
      edit(kFillIntensity,1.);
      edit(kGuitarStepBase+step,1.);
      edit(kGuitarDegreeBase+step,16./28.); // degree 3
      edit(kGuitarLockBase+step,locked?1.:0.);
      Steinberg::Vst::ProcessData setupBlock{};
      setupBlock.numSamples=0;
      setupBlock.inputParameterChanges=&changes;
      assert(playing.process(setupBlock)==Steinberg::kResultOk);
      Steinberg::Vst::ProcessSetup setup{};
      setup.processMode=Steinberg::Vst::kRealtime;
      setup.symbolicSampleSize=Steinberg::Vst::kSample32;
      setup.sampleRate=48000.;setup.maxSamplesPerBlock=6000;
      assert(playing.setupProcessing(setup)==Steinberg::kResultOk);
      Steinberg::Vst::ProcessContext ctx{};
      ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
                Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
                Steinberg::Vst::ProcessContext::kTempoValid;
      ctx.tempo=120.;ctx.projectTimeMusic=static_cast<double>(onset)/960.;
      Steinberg::Vst::EventList outgoing(512);
      Steinberg::Vst::ProcessData block{};
      block.numSamples=6000;block.processContext=&ctx;block.outputEvents=&outgoing;
      assert(playing.process(block)==Steinberg::kResultOk);
      int guitar=-1,bass=-1,ng=0,nb=0;
      for(Steinberg::int32 i=0;i<outgoing.getEventCount();++i) {
        Steinberg::Vst::Event event{};
        assert(outgoing.getEvent(i,event)==Steinberg::kResultOk);
        if(event.type!=Steinberg::Vst::Event::kNoteOnEvent)continue;
        if(event.busIndex==0){++ng;guitar=event.noteOn.pitch;}
        if(event.busIndex==1){++nb;bass=event.noteOn.pitch;}
      }
      assert(ng==1 && nb==1);
      return std::array<int,2>{guitar,bass};
    };
    const auto unlocked=playFill(false);
    const auto locked=playFill(true);
    assert(unlocked[0]==43+delta && unlocked[1]==31+delta);
    assert(locked[0]==43 && locked[1]==31);
  }

  // Regression: project recall must apply every stored drum step to the LIVE
  // MIDI masks. A byte-identical getState roundtrip alone missed this defect.
  {
    std::vector<unsigned char> state(data,data+full.getSize());
    auto put=[&](std::size_t i,std::int32_t v){
      assert((i+1)*4<=state.size());std::memcpy(state.data()+i*4,&v,4);
    };
    const std::size_t drums=11u+16u*4u;
    put(drums,1); // enable drums
    for(std::size_t i=0;i<48;++i)put(drums+1+i,0);
    put(drums+1,1); // only kick at step 0; all hats/snare OFF
    MemoryStream saved(state.data(),static_cast<Steinberg::TSize>(state.size()));
    Processor replay;
    assert(replay.setState(&saved)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessSetup setup{};
    setup.processMode=Steinberg::Vst::kRealtime;
    setup.symbolicSampleSize=Steinberg::Vst::kSample32;
    setup.sampleRate=48000.;setup.maxSamplesPerBlock=6000;
    assert(replay.setupProcessing(setup)==Steinberg::kResultOk);
    Steinberg::Vst::ProcessContext ctx{};
    ctx.state=Steinberg::Vst::ProcessContext::kPlaying |
              Steinberg::Vst::ProcessContext::kProjectTimeMusicValid |
              Steinberg::Vst::ProcessContext::kTempoValid;
    ctx.projectTimeMusic=0.;ctx.tempo=120.;
    Steinberg::Vst::EventList emitted(512);
    Steinberg::Vst::ProcessData block{};
    block.numSamples=6000;block.processContext=&ctx;block.outputEvents=&emitted;
    // Project recall must produce MIDI in its FIRST callback. Bass ownership
    // handoff may release old notes, but cannot drop the downbeat entirely.
    assert(replay.process(block)==Steinberg::kResultOk);
    int kickOns=0,otherDrumOns=0,guitarOns=0;
    for(Steinberg::int32 i=0;i<emitted.getEventCount();++i){
      Steinberg::Vst::Event e{};
      assert(emitted.getEvent(i,e)==Steinberg::kResultOk);
      if(e.type!=Steinberg::Vst::Event::kNoteOnEvent)continue;
      if(e.busIndex==0)++guitarOns;
      if(e.busIndex==2){
        if(e.noteOn.pitch==36)++kickOns;else ++otherDrumOns;
      }
    }
    assert(kickOns==1 && otherDrumOns==0 && guitarOns>0);
  }
}
