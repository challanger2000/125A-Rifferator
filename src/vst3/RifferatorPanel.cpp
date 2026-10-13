#include "RifferatorPanel.hpp"
#include "Plugin.hpp"
#include "UiGeometry.hpp"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cfont.h"
#include "vstgui/lib/cbuttonstate.h"
#include "vstgui/lib/events.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace a125::riff::vst {
namespace {
using namespace VSTGUI;
constexpr CColor back{13,21,28,255},panel{23,33,41,255},dark{19,28,35,255},
 border{77,97,107,255},text{227,234,239,255},muted{153,172,184,255},
 red{145,62,58,255},redBorder{223,154,145,255},openBlue{51,92,113,255},
 palm{104,69,66,255},noise{79,71,81,255};
CRect rect(int x,int y,int w,int h){return CRect(x,y,x+w,y+h);}
void fill(CDrawContext* dc,CRect r,const CColor& bg,const CColor& edge){
 dc->setFillColor(bg);dc->setFrameColor(edge);dc->setLineWidth(1.);
 dc->drawRect(r,kDrawFilledAndStroked);
}
void label(CDrawContext* dc,const std::string& s,CRect r,
           const CColor& color,bool center=false){
 dc->setFont(kNormalFontSmall);
 dc->setFontColor(color);
 dc->drawString(s.c_str(),r,center?kCenterText:kLeftText);
}
std::string pitchName(int pitch){
 static const char* notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 if(pitch<0||pitch>127)return "--";
 return std::string(notes[pitch%12])+std::to_string(pitch/12-1);
}
const char* chordName(int note){
 static const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
 return names[(note+12)%12];
}
}
RifferatorPanel::RifferatorPanel(const VSTGUI::CRect& size,Controller* owner):
 VSTGUI::CView(size),owner_(owner) {
 setTransparency(false);
 setWantsFocus(true);
}
VSTGUI::CMouseEventResult RifferatorPanel::onMouseDown(
    VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons){
 if(!owner_||!(buttons&VSTGUI::kLButton))return VSTGUI::kMouseEventNotHandled;
 const int x=static_cast<int>(pos.x-getViewSize().left);
 const int y=static_cast<int>(pos.y-getViewSize().top);
 const auto target=ui::hit(x,y,owner_->picker()>=0).target;
 if(!(buttons&VSTGUI::kControl)&&owner_->picker()<0 &&
    (target==ui::Target::Pm||target==ui::Target::Fill)){
  owner_->beginNumericDrag(x,y);
  draggingNumeric_=true;
  return VSTGUI::kMouseEventHandled;
 }
 owner_->click(x,y,(buttons&VSTGUI::kShift)!=0,
                 (buttons&VSTGUI::kControl)!=0);
 return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}
VSTGUI::CMouseEventResult RifferatorPanel::onMouseMoved(
    VSTGUI::CPoint& pos,const VSTGUI::CButtonState& buttons){
 if(!owner_||!draggingNumeric_)return VSTGUI::kMouseEventNotHandled;
 if(!(buttons&VSTGUI::kLButton)){
  owner_->endNumericDrag();
  draggingNumeric_=false;
  return VSTGUI::kMouseEventHandled;
 }
 owner_->updateNumericDrag(static_cast<int>(pos.x-getViewSize().left));
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult RifferatorPanel::onMouseUp(
    VSTGUI::CPoint&,const VSTGUI::CButtonState&){
 if(!owner_||!draggingNumeric_)return VSTGUI::kMouseEventNotHandled;
 owner_->endNumericDrag();
 draggingNumeric_=false;
 return VSTGUI::kMouseEventHandled;
}
VSTGUI::CMouseEventResult RifferatorPanel::onMouseCancel(){
 if(draggingNumeric_&&owner_)owner_->endNumericDrag();
 draggingNumeric_=false;
 return VSTGUI::kMouseEventHandled;
}
void RifferatorPanel::onMouseWheelEvent(VSTGUI::MouseWheelEvent& event){
 if(!owner_||event.deltaY==0.)return;
 const int x=static_cast<int>(event.mousePosition.x-getViewSize().left);
 const int y=static_cast<int>(event.mousePosition.y-getViewSize().top);
 const auto target=ui::hit(x,y,owner_->picker()>=0).target;
 if(target!=ui::Target::Pm&&target!=ui::Target::Fill)return;
 owner_->nudgeNumeric(x,y,event.deltaY>0.?1:-1);
 event.consumed=true;
}
int32_t RifferatorPanel::onKeyDown(VSTGUI::VstKeyCode& key){
 if(key.virt==VSTGUI::VKEY_ESCAPE&&owner_&&owner_->cancelPicker())return 1;
 return 0;
}
void RifferatorPanel::draw(VSTGUI::CDrawContext* dc) {
 using namespace VSTGUI;
 const auto& s=owner_->shown();
 const auto& pat=s.pattern;
 const auto& music=s.musical;
 const int bar=owner_->bar(),selected=owner_->focused();
 dc->setDrawMode(kAntiAliasing);
 fill(dc,rect(0,0,1160,570),back,back);
 fill(dc,rect(9,8,1142,554),panel,border);
 fill(dc,rect(20,18,1120,61),back,border);
 label(dc,"RIFFERATOR",rect(181,25,450,29),text);
 label(dc,"V1  /  HEAVIER7STRINGS  /  MIDI RIFF GENERATOR",
       rect(181,53,570,18),muted);
 fill(dc,rect(972,25,152,40),red,redBorder);
 label(dc,std::string("ZOOM ")+(owner_->zoom()>1.25?"150%":"100%"),
       rect(977,34,144,19),text,true);
 fill(dc,rect(23,87,1113,79),dark,border);
 struct Selector {int x,w;const char* heading;std::string value;};
 const Selector selectors[]={
  {34,134,"PATTERN  ▼",std::to_string(pat.bars)+" BARS"},
  {180,116,"KEY  ▼",chordName(music.key)},
  {308,174,"SCALE  ▼",music.scale==Scale::Major?"MAJOR":"NATURAL MINOR"},
  {494,152,"ROOT NOTE  ▼",pitchName(music.rootMidi)},
  {658,214,"PM VELOCITY",std::to_string(s.pmVelocity)},
  {884,242,"FILL INTENSITY",std::to_string(music.fillIntensity)+" %"}
 };
 for(const auto& t:selectors){
  fill(dc,rect(t.x,100,t.w,55),back,border);
  label(dc,t.heading,rect(t.x+12,104,t.w-20,15),muted);
  label(dc,t.value,rect(t.x+12,120,t.w-20,18),text);
 }
 // Drag is a single VST3 automation gesture; wheel steps one integer.
 const int pmX=671+(s.pmVelocity-30)*190/53;
 const int fillX=895+music.fillIntensity*220/100;
 label(dc,"DRAG / WHEEL +/-1",rect(739,120,122,18),muted,true);
 label(dc,"DRAG / WHEEL +/-1",rect(996,120,118,18),muted,true);
 fill(dc,rect(671,142,191,12),dark,border);
 fill(dc,rect(671,142,1+(s.pmVelocity-30)*190/53,12),red,redBorder);
 fill(dc,rect(std::clamp(pmX-5,671,851),139,11,17),redBorder,redBorder);
 fill(dc,rect(895,142,221,12),dark,border);
 fill(dc,rect(895,142,1+music.fillIntensity*220/100,12),red,redBorder);
 fill(dc,rect(std::clamp(fillX-5,895,1105),139,11,17),redBorder,redBorder);
 fill(dc,rect(23,174,1113,43),panel,border);
 label(dc,"BAR",rect(37,186,42,22),text);
 fill(dc,rect(82,177,40,40),dark,border);label(dc,"<",rect(82,187,40,23),text,true);
 label(dc,std::to_string(bar+1)+" / "+std::to_string(pat.bars),rect(119,185,36,19),text,true);
 fill(dc,rect(155,177,40,40),dark,border);label(dc,">",rect(155,187,40,23),text,true);
 for(int c=0;c<16;++c)
  label(dc,std::to_string(c+1),rect(180+c*50,186,48,26),muted,true);
 // Playback trigger belongs to the BAR/transport toolbar.
 fill(dc,rect(992,177,144,40),dark,border);
 label(dc,s.syncMode==SyncMode::Transport?"TRANSPORT":"MIDI NOTE",
       rect(994,185,140,22),text,true);
 fill(dc,rect(23,219,1113,250),back,border);
 const char* captions[]={"PINCH / HARMONICS","OPEN POWERCHORD","PALM MUTE","NOISE","STOP"};
 const int tech[]={3,2,0,1,-1};
 const CColor palette[]={red,openBlue,palm,noise,red};
 for(int row=0;row<5;++row){
  label(dc,captions[row],rect(34,228+row*48,138,26),text);
  for(int col=0;col<16;++col){
   const auto& st=pat.steps[static_cast<std::size_t>(bar*16+col)];
   const bool active=st.on&&(row==4?st.stop:(!st.stop&&st.articulation==tech[row]));
   const auto area=rect(180+50*col,224+48*row,48,46);
   fill(dc,area,active?palette[row]:dark,
        bar*16+col==selected?redBorder:border);
   if(active){
    const auto value=row==4?"STOP":pitchName(pitchForDegree(st.degree,music));
    label(dc,value,rect(181+50*col,234+row*48,46,27),text,true);
   }
   if(st.locked&&row==0)label(dc,"L",rect(208+50*col,224,17,17),redBorder,true);
  }
 }
 fill(dc,rect(992,224,144,240),panel,border);
 label(dc,"SELECTED STEP",rect(1003,234,124,19),muted);
 label(dc,std::to_string(selected%16+1)+" / "+std::to_string(bar+1),
       rect(1007,255,112,21),text,true);
 const auto& focus=pat.steps[static_cast<std::size_t>(selected)];
 fill(dc,rect(1008,276,112,42),focus.on?red:dark,redBorder);
 label(dc,focus.on?"STEP ON":"STEP OFF",rect(1008,285,112,23),text,true);
 fill(dc,rect(1008,322,112,42),focus.locked?red:dark,border);
 label(dc,focus.locked?"LOCKED":"LOCK",rect(1008,329,112,23),text,true);
 label(dc,std::string(chordName(music.key))+
       (music.scale==Scale::Major?" MAJOR":" MINOR"),rect(1002,376,133,19),muted);
 std::string notes;
 for(int i=0;i<7;++i){
  const int v=(music.key+intervals(music.scale)[static_cast<std::size_t>(i)])%12;
  if(i)notes+=" ";
  notes+=chordName(v);
 }
 label(dc,notes.substr(0,16),rect(1002,399,133,20),text);
 if(notes.size()>16)label(dc,notes.substr(16),rect(1002,420,133,20),text);
 fill(dc,rect(23,483,1113,61),dark,border);
 struct Footer {int x,w;const char* title;};
 const Footer footer[]={{34,235,"GENERATE PATTERN"},{283,168,"VARIATE"},
 {465,215,"CLEAR PATTERN"},{694,219,"EXPORT MIDI"}};
 for(const auto& item:footer){
  fill(dc,rect(item.x,493,item.w,44),item.x==34?red:panel,
       item.x==34?redBorder:border);
  label(dc,item.title,rect(item.x,504,item.w,21),text,true);
 }
 label(dc,std::string("STATUS: ")+owner_->notification(),rect(916,494,218,22),muted,true);
 label(dc,"SHIFT+CLICK: CHOOSE NOTE",rect(916,518,218,19),muted,true);
 if(owner_->picker()>=0){
  // Modal right-panel picker leaves every grid row and STOP field exposed.
  fill(dc,rect(992,224,144,240),panel,redBorder);
  label(dc,"CHOOSE NOTE",rect(997,227,102,19),text,true);
  fill(dc,rect(1102,225,31,23),red,redBorder);
  label(dc,"X",rect(1102,225,31,23),text,true);
  for(int i=0;i<15;++i){
   const int degree=i-7;
   const auto name=pitchName(pitchForDegree(degree,music));
   const int x=997+(i%3)*45,y=248+(i/3)*43;
   fill(dc,rect(x,y,43,41),dark,border);
   label(dc,name,rect(x,y+8,43,22),text,true);
  }
 }
 setDirty(false);
}
} // namespace a125::riff::vst
