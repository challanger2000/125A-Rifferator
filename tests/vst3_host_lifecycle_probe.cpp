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
  if(info.category()==kVstAudioEffectClass){audioClass=info;haveAudio=true;}
  if(info.category()==kVstComponentControllerClass){editClass=info;haveController=true;}
 }
 if(!haveAudio||!haveController){std::cerr<<"[FAIL] Missing VST3 component/controller\n";return 2;}
 auto component=factory.createInstance<IComponent>(audioClass.ID());
 auto controller=factory.createInstance<IEditController>(editClass.ID());
 if(!component||!controller||
    component->initialize(host)!=kResultOk||
    controller->initialize(host)!=kResultOk){
   std::cerr<<"[FAIL] VST3 initialization\n";return 2;
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
 MemoryStream privateState;
 if(controller->getState(&privateState)!=kResultOk||privateState.getSize()<=0){
  std::cerr<<"[FAIL] Controller private state save\n";return 2;
 }
 privateState.seek(0,IBStream::kIBSeekSet,nullptr);
 if(controller->setState(&privateState)!=kResultOk){
  std::cerr<<"[FAIL] Controller private state restore\n";return 2;
 }
 std::cout<<"[PASS] Fresh-instance Processor/Controller state restore\n";
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
