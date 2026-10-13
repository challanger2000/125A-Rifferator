#pragma once
#include "vstgui/lib/cview.h"
namespace a125::riff::vst {
class Controller;
class RifferatorPanel final:public VSTGUI::CView {
public:
 RifferatorPanel(const VSTGUI::CRect& r,Controller* owner);
 void draw(VSTGUI::CDrawContext*) override;
 void onMouseWheelEvent(VSTGUI::MouseWheelEvent&) override;
 int32_t onKeyDown(VSTGUI::VstKeyCode&) override;
 VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,
                              const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseMoved(VSTGUI::CPoint&,
                              const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseUp(VSTGUI::CPoint&,
                              const VSTGUI::CButtonState&) override;
 VSTGUI::CMouseEventResult onMouseCancel() override;
private:
 Controller* owner_;
 bool draggingNumeric_{false};
};
}
