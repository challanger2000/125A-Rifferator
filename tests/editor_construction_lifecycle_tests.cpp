#include "../src/RifferatorFactory.cpp"
#ifdef NDEBUG
#error Assertions must be active in Release
#endif
#include <cassert>
int main() {
  Controller controller;
  assert(controller.initialize(nullptr)==Steinberg::kResultOk);
  for(int n=0;n<40;++n) {
    auto* view=controller.createView(Steinberg::Vst::ViewType::kEditor);
    assert(view!=nullptr);
    // The lifecycle contract here is create/release without attaching a
    // native window. This is not an editor attached()/removed() test.
    view->release();
    assert(controller.activeEditor_==nullptr || controller.viewOwner_==nullptr);
  }
  assert(controller.terminate()==Steinberg::kResultOk);
}
