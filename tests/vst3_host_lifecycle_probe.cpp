// Host-side 125A Rifferator V1 lifecycle regression.
// Original standalone harness using the licensed Steinberg SDK public API.
// Does not depend on the private 125A Plugin Tester repository.
#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/common/memorystream.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "../src/product/RifferatorState.hpp"
#include "../src/vst3/Parameters.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <atomic>
#include <cstring>
#include <iostream>
#include <string>

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace Hosting=VST3::Hosting;
class Frame final : public IPlugFrame {
 std::atomic<uint32> refs_{1};
 public:
 tresult PLUGIN_API queryInterface(const TUID iid,void** result) override {
  if(!result)return kInvalidArgument;
  *result=nullptr;
  if(std::memcmp(iid,IPlugFrame::iid,16)!=0&&
     std::memcmp(iid,FUnknown::iid,16)!=0)return kNoInterface;
  *result=static_cast<IPlugFrame*>(this);
  addRef();
  return kResultOk;
 }
 uint32 PLUGIN_API addRef() override{return ++refs_;}
 uint32 PLUGIN_API release() override {
  const auto n=--refs_;if(!n)delete this;return n;
 }
 tresult PLUGIN_API resizeView(IPlugView*,ViewRect* size) override {
  return size&&size->right>size->left&&size->bottom>size->top?
     kResultTrue:kInvalidArgument;
 }
};
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"[FAIL] Expected VST3 bundle path\n";return 2;}
 std::string error;
 auto module=Hosting::Module::create(argv[1],error);
 if(!module){std::cerr<<"[FAIL] VST3 module "<<error<<"\n";return 2;}
 const auto& factory=module->getFactory();
 auto* host=new HostApplication();
 factory.setHostContext(host);
 Hosting::ClassInfo audioClass;
 Hosting::ClassInfo editClass;
 bool haveAudio=false,haveController=false;
 for(const auto& info:factory.classInfos()){
  if(info.name()=="125A Rifferator"){audioClass=info;haveAudio=true;}
  if(info.name()=="125A Rifferator Controller"){editClass=info;haveController=true;}
 }
 if(!haveAudio||!haveController){std::cerr<<"[FAIL] Missing VST3 component/controller\n";return 2;}
 auto component=factory.createInstance<IComponent>(audioClass.ID());
 auto controller=factory.createInstance<IEditController>(editClass.ID());
 if(!component||!controller||
    component->initialize(host)!=kResultOk||
    controller->initialize(host)!=kResultOk){
   std::cerr<<"[FAIL] VST3 initialization\n";return 2;
 }
 // A default-only roundtrip can miss genuine lost BARs, notes and LOCKs.
 // Install a non-default eight-bar fixture through the actual VST3 interface.
 a125::riff::product::Snapshot fixture;
 if(!fixture.pattern.setBars(8)){std::cerr<<"[FAIL] Fixture bars\n";return 2;}
 fixture.musical.fillIntensity=70;
 fixture.pmVelocity=65;
 fixture.syncMode=a125::riff::SyncMode::MidiNote;
 fixture.pattern.steps[0].on=true;
 fixture.pattern.steps[0].articulation=0; // PM velocity 65
 fixture.pattern.steps[1].on=true;
 fixture.pattern.steps[1].articulation=2; // OPEN velocity 115
 fixture.pattern.steps[1].degree=1; // F#2 in E natural minor
 fixture.pattern.steps[2].on=true;
 fixture.pattern.steps[2].stop=true; // release, no new attack
 auto& guitar=fixture.pattern.steps[4];
 guitar.on=true;guitar.degree=2;guitar.articulation=2;guitar.locked=true;
 auto& lastBar=fixture.pattern.steps[100];
 lastBar.on=true;lastBar.stop=true;lastBar.locked=true;
 a125::riff::product::StateBytes fixtureBytes{};
 if(!a125::riff::product::encode(fixture,fixtureBytes)){
  std::cerr<<"[FAIL] Non-default state fixture encoding\n";return 2;
 }
 MemoryStream fixtureStream(fixtureBytes.data(),static_cast<TSize>(fixtureBytes.size()));
 if(component->setState(&fixtureStream)!=kResultOk){
  std::cerr<<"[FAIL] Processor could not restore non-default fixture\n";return 2;
 }
 MemoryStream serialized;
 if(component->getState(&serialized)!=kResultOk||serialized.getSize()<=0){
  std::cerr<<"[FAIL] Processor getState\n";return 2;
 }
 auto fresh=factory.createInstance<IComponent>(audioClass.ID());
 if(!fresh||fresh->initialize(host)!=kResultOk){
  std::cerr<<"[FAIL] Fresh component initialize\n";return 2;
 }
 serialized.seek(0,IBStream::kIBSeekSet,nullptr);
 if(fresh->setState(&serialized)!=kResultOk){
  std::cerr<<"[FAIL] Processor state restore into fresh instance\n";return 2;
 }
 serialized.seek(0,IBStream::kIBSeekSet,nullptr);
 if(controller->setComponentState(&serialized)!=kResultOk){
  std::cerr<<"[FAIL] Controller setComponentState\n";return 2;
 }
 MemoryStream recovered;
 if(fresh->getState(&recovered)!=kResultOk||
    recovered.getSize()!=static_cast<TSize>(fixtureBytes.size())){
  std::cerr<<"[FAIL] Fresh Processor non-default state re-serialization\n";return 2;
 }
 a125::riff::product::Snapshot decoded;
 if(!a125::riff::product::decode(
       reinterpret_cast<const std::uint8_t*>(recovered.getData()),
       static_cast<std::size_t>(recovered.getSize()),decoded)||
    decoded.pattern.bars!=8||decoded.pmVelocity!=65||
    decoded.syncMode!=a125::riff::SyncMode::MidiNote||
    decoded.musical.fillIntensity!=70||
    !decoded.pattern.steps[4].locked||
    decoded.pattern.steps[4].degree!=2||
    !decoded.pattern.steps[100].stop||
    !decoded.pattern.steps[100].locked){
  std::cerr<<"[FAIL] Non-default Processor state mismatch\n";return 2;
 }
 const auto close=[](double a,double b){return std::abs(a-b)<1.e-7;};
 if(!close(controller->getParamNormalized(a125::riff::vst::Bars),1.)||
    !close(controller->getParamNormalized(a125::riff::vst::Pm),
           a125::riff::vst::n(65,30,83))||
    !close(controller->getParamNormalized(a125::riff::vst::On+4),1.)||
    !close(controller->getParamNormalized(a125::riff::vst::Lock+100),1.)||
    !close(controller->getParamNormalized(a125::riff::vst::Degree+4),
           a125::riff::vst::n(2,-14,14))){
  std::cerr<<"[FAIL] Controller did not reflect restored non-default state\n";return 2;
 }
 // Test new UI state v2 including seed, plus backward-compatibility with
 // the previously shipped experimental 12-byte v1 Controller state.
 std::uint32_t legacyUi[3]{0x52475531u,150u,7u};
 MemoryStream legacyIn(legacyUi,sizeof(legacyUi));
 if(controller->setState(&legacyIn)!=kResultOk){
  std::cerr<<"[FAIL] Controller legacy v1 state migration\n";return 2;
 }
 std::uint32_t customUi[4]{0x52475532u,150u,7u,0xfedcba98u};
 MemoryStream customUiIn(customUi,sizeof(customUi));
 if(controller->setState(&customUiIn)!=kResultOk){
  std::cerr<<"[FAIL] Controller v2 zoom/BAR/generator seed restore\n";return 2;
 }
 MemoryStream privateState;
 if(controller->getState(&privateState)!=kResultOk||
    privateState.getSize()!=sizeof(customUi)||
    std::memcmp(privateState.getData(),customUi,sizeof(customUi))!=0){
  std::cerr<<"[FAIL] Controller v2 GUI state and seed serialization\n";return 2;
 }
 // Truncated v2 state cannot partially update any Controller field.
 std::uint32_t shortUi[3]{0x52475532u,100u,0u};
 MemoryStream shortIn(shortUi,sizeof(shortUi));
 if(controller->setState(&shortIn)==kResultOk){
  std::cerr<<"[FAIL] Truncated UI state accepted\n";return 2;
 }
 MemoryStream afterBad;
 if(controller->getState(&afterBad)!=kResultOk||
    afterBad.getSize()!=sizeof(customUi)||
    std::memcmp(afterBad.getData(),customUi,sizeof(customUi))!=0){
  std::cerr<<"[FAIL] Invalid UI state corrupted prior Controller fields\n";return 2;
 }
 privateState.seek(0,IBStream::kIBSeekSet,nullptr);
 if(controller->setState(&privateState)!=kResultOk){
  std::cerr<<"[FAIL] Controller private v2 state roundtrip\n";return 2;
 }
 // Reject a host edit to locked ON and DEGREES while restoring host readback.
 if(controller->setParamNormalized(a125::riff::vst::On+4,0.)==kResultOk ||
    !close(controller->getParamNormalized(a125::riff::vst::On+4),1.)||
    controller->setParamNormalized(a125::riff::vst::Degree+4,0.)==kResultOk ||
    !close(controller->getParamNormalized(a125::riff::vst::Degree+4),
           a125::riff::vst::n(2,-14,14))){
  std::cerr<<"[FAIL] Host automation changed a locked step readback\n";return 2;
 }
 // Snapped out-of-scale roots must also report the actual corrected pitch.
 if(controller->setParamNormalized(a125::riff::vst::Root,
                                  a125::riff::vst::n(41,0,127))!=kResultOk ||
    !close(controller->getParamNormalized(a125::riff::vst::Root),
           a125::riff::vst::n(42,0,127))){
  std::cerr<<"[FAIL] ROOT out-of-scale snap not reflected in host\n";return 2;
 }
 // Non-default snapshot and zoom/BAR fixtures must survive native host reload.
 std::cout<<"[PASS] Non-default 8-bar Processor/Controller state, LOCK and 150% UI recall\n";
 HINSTANCE instance=GetModuleHandleW(nullptr);
 const wchar_t cls[]=L"RifferatorQaHostWindow";
 WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=DefWindowProcW;
 wc.lpszClassName=cls;
 if(!RegisterClassW(&wc)){
  std::cerr<<"[FAIL] RegisterClassW "<<GetLastError()<<"\n";return 2;
 }
 for(int n=1;n<=20;++n){
  HWND hwnd=CreateWindowExW(0,cls,L"125A QA",WS_OVERLAPPEDWINDOW,
               50,50,1180,620,nullptr,nullptr,instance,nullptr);
  if(!hwnd){std::cerr<<"[FAIL] CreateWindow "<<n<<"\n";return 2;}
  auto* view=controller->createView(ViewType::kEditor);
  if(!view){std::cerr<<"[FAIL] createView "<<n<<"\n";return 2;}
  auto* frame=new Frame;
  if(view->setFrame(frame)!=kResultTrue){
   std::cerr<<"[FAIL] setFrame "<<n<<"\n";return 2;
  }
  if(view->isPlatformTypeSupported(kPlatformTypeHWND)!=kResultTrue){
   std::cerr<<"[FAIL] HWND platform "<<n<<"\n";return 2;
  }
  const auto attached=view->attached(reinterpret_cast<void*>(hwnd),kPlatformTypeHWND);
  if(attached!=kResultTrue){
   std::cerr<<"[FAIL] attached HWND "<<n<<"\n";return 2;
  }
  ViewRect size{};
  if(view->getSize(&size)!=kResultTrue||
     size.right<=size.left||size.bottom<=size.top){
   std::cerr<<"[FAIL] Editor bounds "<<n<<"\n";return 2;
  }
  MSG msg{};
  while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
   TranslateMessage(&msg);DispatchMessageW(&msg);
  }
  if(view->removed()!=kResultTrue){
   std::cerr<<"[FAIL] removed "<<n<<"\n";return 2;
  }
  view->setFrame(nullptr);
  view->release();
  frame->release();
  DestroyWindow(hwnd);
  std::cout<<"[PASS] Editor attach/detach "<<n<<"/20\n";
 }
 // SDK-hosted VST3 MIDI/automation integration: no private QA source is used.
 // This checks actual ProcessData IEventList and IParameterChanges, not the
 // separate MidiRuntime C++ unit tests.
 IAudioProcessor* processor=nullptr;
 if(fresh->queryInterface(IAudioProcessor::iid,
       reinterpret_cast<void**>(&processor))!=kResultOk||!processor){
  std::cerr<<"[FAIL] Fresh Processor IAudioProcessor interface\n";return 2;
 }
 const auto check=[&](bool valid,const char* why){
  if(!valid)std::cerr<<"[FAIL] Host MIDI/automation: "<<why<<"\n";
  return valid;
 };
 if(!check(fresh->getBusCount(kAudio,kInput)==0 &&
            fresh->getBusCount(kAudio,kOutput)==0 &&
            fresh->getBusCount(kEvent,kInput)==1 &&
            fresh->getBusCount(kEvent,kOutput)==1,
            "unexpected bus topology"))return 2;
 if(!check(fresh->activateBus(kEvent,kInput,0,true)==kResultOk &&
            fresh->activateBus(kEvent,kOutput,0,true)==kResultOk,
            "event bus activation"))return 2;
 ProcessSetup setup{};
 setup.processMode=kRealtime;
 setup.symbolicSampleSize=kSample32;
 setup.maxSamplesPerBlock=512;
 setup.sampleRate=48000.;
 if(!check(processor->setupProcessing(setup)==kResultOk &&
            fresh->setActive(true)==kResultOk &&
            processor->setProcessing(true)==kResultOk,
            "processor activation/setup"))return 2;
 EventList incoming(256),outgoing(256);
 ParameterChanges changes(16);
 ProcessContext context{};
 context.state=ProcessContext::kPlaying |
               ProcessContext::kProjectTimeMusicValid |
               ProcessContext::kTempoValid;
 context.sampleRate=48000.;
 context.tempo=120.;
 ProcessData data{};
 data.processMode=kRealtime;
 data.symbolicSampleSize=kSample32;
 data.numSamples=512;
 data.numInputs=0;
 data.numOutputs=0;
 data.inputEvents=&incoming;
 data.outputEvents=&outgoing;
 data.inputParameterChanges=&changes;
 data.processContext=&context;
 const auto processAt=[&](double ppq,bool running) {
  outgoing.clear();
  context.projectTimeMusic=ppq;
  context.state=ProcessContext::kProjectTimeMusicValid |
                ProcessContext::kTempoValid |
                (running?ProcessContext::kPlaying:0u);
  return processor->process(data)==kResultOk;
 };
 // The recall fixture deliberately starts in MIDI NOTE mode. Explicitly
 // automate TRANSPORT=0 before the transport scenarios; never let tests
 // depend on an implicit default after restoring a non-default project.
 int32 modeQueueIndex=0,modePointIndex=0;
 auto* transportQueue=changes.addParameterData(a125::riff::vst::Mode,
                                                 modeQueueIndex);
 if(!check(transportQueue!=nullptr &&
           transportQueue->addPoint(0,0.,modePointIndex)==kResultOk,
           "prepare explicit TRANSPORT mode automation"))return 2;
 const auto is=[&](int idx,bool on,int pitch,int velocity,int offset){
  Event e{};
  if(outgoing.getEvent(idx,e)!=kResultOk)return false;
  if(e.sampleOffset!=offset||e.busIndex!=0)return false;
  if(on)return e.type==Event::kNoteOnEvent &&
              e.noteOn.pitch==pitch &&
              std::abs(e.noteOn.velocity*127.-velocity)<0.02;
  return e.type==Event::kNoteOffEvent&&e.noteOff.pitch==pitch;
 };
 if(!check(processAt(0.,true)&&outgoing.getEventCount()==1 &&
           is(0,true,40,65,0),"Step 1 PalmMute note and velocity"))return 2;
 changes.clearQueue();
 if(!check(processAt(.25,true)&&outgoing.getEventCount()==2 &&
           is(0,false,40,0,0)&&is(1,true,42,115,0),
           "Step 2 changes pitch/articulation with release first"))return 2;
 if(!check(processAt(.5,true)&&outgoing.getEventCount()==1 &&
           is(0,false,42,0,0),"STOP only emits NoteOff"))return 2;
 if(!check(processAt(1.,true)&&outgoing.getEventCount()==1 &&
           is(0,true,43,115,0),"locked OPEN G2 on Step 5"))return 2;
 if(!check(processAt(1.01,false)&&outgoing.getEventCount()==1 &&
           is(0,false,43,0,0),"host transport STOP flushes sounding voice"))return 2;
 std::cout<<"[PASS] Native VST3 MIDI: PM/OPEN velocity, STOP, transport release\n";

 const auto automate=[&](ParamID id,double normalized){
  int32 qidx=0,point=0;
  auto* q=changes.addParameterData(id,qidx);
  return q&&q->addPoint(0,normalized,point)==kResultOk;
 };
 if(!check(automate(a125::riff::vst::Key,
                    a125::riff::vst::n(6,0,11)),
           "send KEY automation"))return 2;
 if(!check(processAt(0.,true)&&outgoing.getEventCount()==1 &&
           is(0,true,42,65,0),
           "KEY automation transposes actual MIDI and velocity"))return 2;
 changes.clearQueue();
 MemoryStream editedState;
 if(!check(fresh->getState(&editedState)==kResultOk,
           "save automated Processor state"))return 2;
 a125::riff::product::Snapshot savedAutomation;
 if(!check(a125::riff::product::decode(
                reinterpret_cast<const std::uint8_t*>(editedState.getData()),
                static_cast<std::size_t>(editedState.getSize()),savedAutomation) &&
           savedAutomation.musical.key==6 &&
           savedAutomation.musical.rootMidi==42 &&
           savedAutomation.pattern.steps[4].locked,
           "KEY automation persists a valid F# minor root"))return 2;
 if(!check(automate(a125::riff::vst::On+0,1.),
           "send unchanged STEP ON automation"))return 2;
 if(!check(processAt(512./24000.,true)&&outgoing.getEventCount()==0,
           "unchanged STEP ON must not retrigger/release"))return 2;
 changes.clearQueue();
 if(!check(automate(a125::riff::vst::Mode,1.),
           "switch to MIDI NOTE trigger mode"))return 2;
 Event on{};on.type=Event::kNoteOnEvent;on.sampleOffset=100;
 on.busIndex=0;on.noteOn.channel=0;on.noteOn.pitch=48;on.noteOn.velocity=1.f;
 Event off{};off.type=Event::kNoteOffEvent;off.sampleOffset=200;
 off.busIndex=0;off.noteOff.channel=0;off.noteOff.pitch=48;
 if(!check(incoming.addEvent(on)==kResultOk&&
           incoming.addEvent(off)==kResultOk,
           "prepare NoteOn/NoteOff input events"))return 2;
 if(!check(processAt(0.,false)&&outgoing.getEventCount()==3 &&
           is(0,false,42,0,0) &&
           is(1,true,42,65,100) &&
           is(2,false,42,0,200),
           "MIDI NOTE mode must trigger and release exactly"))return 2;
 changes.clearQueue();incoming.clear();
 if(!check(processAt(0.,false)&&outgoing.getEventCount()==0,
           "no residual note after MIDI NoteOff"))return 2;
 if(!check(processor->setProcessing(false)==kResultOk &&
           fresh->setActive(false)==kResultOk,
           "host suspend"))return 2;
 // 125A-style bounded host stress: multiple rates, offline/realtime and
 // deliberately irregular 1..512 sample blocks, with actual event-list
 // validation and a final STOP. No synthetic "plugin passed" shortcut.
 for(const double rate : {44100.,96000.}){
  for(const int processMode : {kRealtime,kOffline}){
   ProcessSetup modeSetup{};
   modeSetup.processMode=processMode;
   modeSetup.symbolicSampleSize=kSample32;
   modeSetup.maxSamplesPerBlock=512;
   modeSetup.sampleRate=rate;
   if(!check(processor->setupProcessing(modeSetup)==kResultOk &&
             fresh->setActive(true)==kResultOk &&
             processor->setProcessing(true)==kResultOk,
             "stress setup/activate"))return 2;
   incoming.clear();
   changes.clearQueue();
   if(!check(automate(a125::riff::vst::Mode,0.),
             "stress select TRANSPORT mode"))return 2;
   const int lengths[5]{1,31,127,257,512};
   const std::int64_t end=static_cast<std::int64_t>(rate*1.25);
   std::int64_t pos=0;
   int blocks=0,noteOns=0,noteOffs=0;
   bool noteActive=false;
   while(pos<end){
    data.numSamples=static_cast<int>(
       std::min<std::int64_t>(lengths[blocks%5],end-pos));
    data.processMode=processMode;
    context.sampleRate=rate;
    context.tempo=120.;
    context.projectTimeMusic=static_cast<double>(pos)*2./rate;
    context.state=ProcessContext::kPlaying |
                  ProcessContext::kProjectTimeMusicValid |
                  ProcessContext::kTempoValid;
    outgoing.clear();
    if(!check(processor->process(data)==kResultOk,
              "variable-block callback"))return 2;
    for(int n=0;n<outgoing.getEventCount();++n){
     Event e{};
     if(!check(outgoing.getEvent(n,e)==kResultOk &&
               e.busIndex==0 &&
               e.sampleOffset>=0 &&
               e.sampleOffset<data.numSamples,
               "event must lie inside processing block"))return 2;
     if(e.type==Event::kNoteOnEvent){
      if(!check(!noteActive&&e.noteOn.pitch>=0&&e.noteOn.pitch<=127,
                "duplicate or invalid NoteOn in stress"))return 2;
      noteActive=true;++noteOns;
     }else if(e.type==Event::kNoteOffEvent){
      if(!check(noteActive,"unmatched NoteOff in stress"))return 2;
      noteActive=false;++noteOffs;
     }else{
      if(!check(false,"non-note event on MIDI-only bus"))return 2;
     }
    }
    pos+=data.numSamples;
    ++blocks;
    changes.clearQueue();
   }
   data.numSamples=512;
   context.state=ProcessContext::kProjectTimeMusicValid |
                 ProcessContext::kTempoValid;
   context.projectTimeMusic=static_cast<double>(pos)*2./rate;
   outgoing.clear();
   if(!check(processor->process(data)==kResultOk,
             "transport stop at irregular end"))return 2;
   for(int n=0;n<outgoing.getEventCount();++n){
    Event e{};
    if(!check(outgoing.getEvent(n,e)==kResultOk &&
              e.type==Event::kNoteOffEvent &&
              e.sampleOffset==0 && noteActive,
              "transport cleanup exact offset"))return 2;
    noteActive=false;++noteOffs;
   }
   if(!check(!noteActive && noteOns>=3 && noteOns==noteOffs,
             "balanced MIDI voices and nontrivial attacks"))return 2;
   if(!check(processor->setProcessing(false)==kResultOk &&
             fresh->setActive(false)==kResultOk,
             "stress deactivate"))return 2;
   std::cout<<"[PASS] Host MIDI stress "<<rate<<" Hz "
            <<(processMode==kOffline?"offline":"realtime")
            <<" blocks="<<blocks
            <<" on="<<noteOns<<" off="<<noteOffs<<"\n";
  }
 }
 processor->release();
 std::cout<<"[PASS] Native VST3 KEY automation/ROOT state, redundant parameter, MIDI trigger\n";
 controller->terminate();
 fresh->terminate();
 component->terminate();
 host->release();
 UnregisterClassW(cls,instance);
 std::cout<<"[PASS] 125A host Editor lifecycle 20/20 and state restore\n";
 return 0;
}
#else
int main(){return 0;}
#endif
