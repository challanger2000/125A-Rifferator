#pragma once
#include "vstgui/lib/cview.h"
namespace a125::riff::vst {
class Controller;
class RifferatorPanel final:public VSTGUI::CView {
public:
 RifferatorPanel(const VSTGUI::CRect& r,Controller* owner);
 void draw(VSTGUI::CDrawContext*) override;
 VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint&,
                              const VSTGUI::CButtonState&) override;
private:
 Controller* owner_;
};
}
