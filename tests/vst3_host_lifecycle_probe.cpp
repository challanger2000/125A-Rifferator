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
#include "../src/product/RifferatorState.hpp"
#include "../src/vst3/Parameters.hpp"
#include <cmath>
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
 // Also exercise the UI-only 150% zoom and BAR 8 across getState/setState.
 std::uint32_t customUi[3]{0x52475531u,150u,7u};
 MemoryStream customUiIn(customUi,sizeof(customUi));
 if(controller->setState(&customUiIn)!=kResultOk){
  std::cerr<<"[FAIL] Controller custom zoom/BAR restore\n";return 2;
 }
 MemoryStream privateState;
 if(controller->getState(&privateState)!=kResultOk||
    privateState.getSize()!=sizeof(customUi)||
    std::memcmp(privateState.getData(),customUi,sizeof(customUi))!=0){
  std::cerr<<"[FAIL] Controller zoom/BAR state save mismatch\n";return 2;
 }
 privateState.seek(0,IBStream::kIBSeekSet,nullptr);
 if(controller->setState(&privateState)!=kResultOk){
  std::cerr<<"[FAIL] Controller private state restore\n";return 2;
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
