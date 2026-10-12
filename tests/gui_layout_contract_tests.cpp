#include <cassert>
#include <fstream>
#include <regex>
#include <string>
#include <set>
#include <map>
#include <iterator>
#include <array>
#include <utility>
#include <vector>
#include <algorithm>
int main(){
  std::ifstream file("resource/rifferator.uidesc");
  assert(file.good());
  const std::string xml((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
  assert(xml.find("size=\"1120, 640\"")!=std::string::npos);
  // DOUBLE must be produced once, in the live scheduler, never pre-rendered.
  std::ifstream processorFile("src/RifferatorProcessor.cpp");
  assert(processorFile.good());
  const std::string processor((std::istreambuf_iterator<char>(processorFile)),
                               std::istreambuf_iterator<char>());
  assert(processor.find("step.doubled=false;")!=std::string::npos);
  // Legacy Step snapshots and the separately stored live matrix must survive
  // project restore without destructive reconciliation.
  assert(processor.find("Preserve separately serialized Step snapshots verbatim")!=std::string::npos);


  assert(xml.find("title=\"DEGREE\"")!=std::string::npos);
  // Signed degrees down to -13 need more than the former 19-pixel readout.
  for(int i=0;i<16;++i) {
    const auto name="control-tag=\"DegreeValue"+std::to_string(i)+"\"";
    const auto begin=xml.find(name);
    assert(begin!=std::string::npos);
    const auto end=xml.find("/>",begin);
    assert(end!=std::string::npos);
    assert(xml.substr(begin,end-begin).find("size=\"28, 19\"")!=std::string::npos);
    assert(xml.substr(begin,end-begin).find("mouse-enabled=\"false\"")!=std::string::npos);
  }

  // Every previously host-only core musical control must have two visible
  // buttons and a non-clickable live value display in the bundled VSTGUI.
  // All values are implemented with the *existing* VST3 parameter IDs.
  for(int i=0;i<6;++i) {
    const std::string bases[6]{"Root","GtrGate","BassGate",
                               "Fill","BassVelocity","EbmStyle"};
    for(const auto* suffix:{"Minus","Plus","Value"}) {
      const auto control=bases[i]+suffix;
      const int tag=std::string(suffix)=="Value"?912+i:
        900+2*i+(std::string(suffix)=="Plus"?1:0);
      const auto definition="<control-tag name=\""+control+"\" tag=\""+
        std::to_string(tag)+"\"/>";
      assert(xml.find(definition)!=std::string::npos);
      const auto used="control-tag=\""+control+"\"";
      const auto pos=xml.find(used);
      assert(pos!=std::string::npos);
      const auto end=xml.find("/>",pos);
      assert(end!=std::string::npos);
      const auto viewStart=xml.rfind("<view",pos);
      assert(viewStart!=std::string::npos && viewStart<pos);
      const auto view=xml.substr(viewStart,end-viewStart);
      assert(view.find("class=\"CTextButton\"")!=std::string::npos);
      if(std::string(suffix)=="Value")
        assert(view.find("mouse-enabled=\"false\"")!=std::string::npos);
    }
  }
  assert(xml.find("<control-tag name=\"DrumsEnabledAction\" tag=\"724\"/>")!=std::string::npos);
  assert(xml.find("control-tag=\"DrumsEnabledAction\" origin=\"908, 346\"")!=std::string::npos);
  // Three active drum commands must occupy separate non-overlapping
  // positions in the fixed header band and have unique controller tags.
  for(const auto& spec:std::array<std::pair<const char*,int>,3>{{
      {"StandardDrums",725},{"GenerateDrums",726},{"VariateDrums",727}}}) {
    const auto tag=std::string("<control-tag name=\"")+spec.first+
      "\" tag=\""+std::to_string(spec.second)+"\"/>";
    assert(xml.find(tag)!=std::string::npos);
    assert(xml.find(std::string("control-tag=\"")+spec.first+
      "\" origin=\"")!=std::string::npos);
  }
  assert(xml.find("control-tag=\"StandardDrums\" origin=\"347, 346\" size=\"174, 34\"")!=std::string::npos);
  assert(xml.find("control-tag=\"GenerateDrums\" origin=\"534, 346\" size=\"174, 34\"")!=std::string::npos);
  assert(xml.find("control-tag=\"VariateDrums\" origin=\"721, 346\" size=\"174, 34\"")!=std::string::npos);

  assert(xml.find("control-tag=\"ClearPattern\"")!=std::string::npos);
  // Numerical PM readouts are labels, not misleading interactive switches.
  for(const auto* name:{"PmMinValue","PmMaxValue"}) {
    const auto begin=xml.find(std::string("control-tag=\"")+name+"\" origin=");
    assert(begin!=std::string::npos);
    const auto end=xml.find("/>",begin);
    assert(end!=std::string::npos);
    assert(xml.substr(begin,end-begin).find("mouse-enabled=\"false\"")!=std::string::npos);
  }
  // 1..127 must fit without clipping in both velocity displays.
  assert(xml.find("control-tag=\"PmMinValue\" origin=\"75, 425\" size=\"47, 27\"")!=std::string::npos);
  assert(xml.find("control-tag=\"PmMaxValue\" origin=\"195, 425\" size=\"47, 27\"")!=std::string::npos);

  // GUI wiring regression: every visible switch must have the controller
  // handler that publishes its VST3 parameter edit (not a dead decoration).
  std::ifstream controllerFile("src/RifferatorFactory.cpp");
  assert(controllerFile.good());
  const std::string controller((std::istreambuf_iterator<char>(controllerFile)),
                               std::istreambuf_iterator<char>());
  assert(controller.find("applyMusicalStepCommand(command);")!=std::string::npos);
  assert(controller.find("toggleDrumsEnabled();")!=std::string::npos);
  assert(controller.find("if(command==725)")!=std::string::npos);
  assert(controller.find("if(command==726 || command==727)")!=std::string::npos);
  assert(controller.find("runDrumGeneration(command==726?")!=std::string::npos);
  assert(controller.find("publishDrumSteps(generateDrums(")!=std::string::npos);
  assert(controller.find("publishDrumSteps(a125::rifferator::DrumPattern::standard())")!=std::string::npos);
  assert(controller.find("(control->getTag()>=700 && control->getTag()<=729)")!=std::string::npos);
  assert(controller.find("if(id==musicalParam(i))")!=std::string::npos);
  assert(controller.find("refreshMusicalDisplays();")!=std::string::npos);
  assert(controller.find("control->getTag()==724")!=std::string::npos);
  // New view cache must be invalidated on both close and owner switch.
  const auto viewStart=controller.find("VSTGUI::CView* verifyView(");
  const auto viewEnd=controller.find("void valueChanged(",viewStart);
  assert(viewStart!=std::string::npos && viewEnd!=std::string::npos);
  assert(controller.substr(viewStart,viewEnd-viewStart).find(
    "musicalDisplays_.fill(nullptr);")!=std::string::npos);
  assert(controller.find("kRootPitch,kGuitarGate,kBassGate,kFillIntensity,kBassVelocity,kEbmStyle")!=std::string::npos);


  assert(controller.find("command==721")!=std::string::npos);
  // PM bounds must agree between host display and processor after automation
  // and the restore order must support ranges above and below defaults.
  assert(controller.find("std::min(value,bound):std::max(value,bound)")!=std::string::npos);
  assert(controller.find("restoreMin();restoreMax();")!=std::string::npos);
  assert(controller.find("restoreMax();restoreMin();")!=std::string::npos);

  // Legacy versions predate the explicit guitar arrays: GUI must recover
  // articulation, degree, lock and step activity from saved Step records.
  for(const auto* field:{"storedArt[j]!=0","storedDegree[legacy]","storedLocks[legacy]","storedArt[legacy]"})
    assert(controller.find(field)!=std::string::npos);
  // Locked STEP and DOUBLE toggles must revert locally, not only in processor.
  assert(controller.find("command>=448?command-448:command-300")!=std::string::npos);
  assert(controller.find("refreshSwitch(id);")!=std::string::npos);

  // Both commands must invoke the existing seeded guitar generation
  // engine, never the former three-pattern demo rotation. The controller
  // applies results through VST3 parameter edits outside process().
  assert(xml.find("<control-tag name=\"GuitarGenerate\" tag=\"700\"/>")!=std::string::npos);
  assert(xml.find("<control-tag name=\"GuitarVariate\" tag=\"722\"/>")!=std::string::npos);
  assert(xml.find("control-tag=\"GuitarVariate\" origin=\"535, 478\" size=\"235, 39\"")!=std::string::npos);
  const auto generation=controller.find("void runGuitarGeneration(");
  assert(generation!=std::string::npos);
  const auto generationEnd=controller.find("VSTGUI::VST3Editor* activeEditor_",generation);
  assert(generationEnd!=std::string::npos);
  const auto generationCode=controller.substr(generation,generationEnd-generation);
  assert(generationCode.find("generateGuitar(current,options,command,30)")!=std::string::npos);
  assert(generationCode.find("if(current[j].locked)continue;")!=std::string::npos);
  assert(generationCode.find("options.randomPitch=false;")!=std::string::npos);
  // An unchanged active STOP must not collapse to an empty cell during
  // conservative VARIATE; it is a distinct live MIDI NoteOff command.
  assert(generationCode.find("const bool preserveStop=")!=std::string::npos);
  assert(generationCode.find("publish(kGuitarDoubleBase+i,0.)")!=std::string::npos);
  assert(controller.find("if(command==700 || command==722)")!=std::string::npos);
  // Drum commands 725..727 extend the command listener range beyond Guitar VARIATE 722.
  assert(controller.find("(control->getTag()>=700 && control->getTag()<=729)")!=std::string::npos);
  assert(controller.find("makeIndustrialGuitarGroove(")==std::string::npos);
  assert(controller.find("kGuitarLockBase+step")!=std::string::npos);
  for(const auto* expr: {"command>=200 && command<248",
                         "command>=300 && command<316",
                         "command>=432 && command<464",
                         "beginEdit(id);setParamNormalized(id,value);performEdit(id,value);endEdit(id)",
                         "refreshSwitch(id)"})assert(controller.find(expr)!=std::string::npos);

  assert(xml.find("title=\"PITCH\"")==std::string::npos);
  // A selected matrix cell must be able to clear its own active step.
  assert(controller.find("std::abs(editor->getZoomFactor()-zoomFactor_)>1.e-9")!=std::string::npos);
  assert(controller.find("current==art")!=std::string::npos);
  assert(controller.find("setParamNormalized(id,0.);performEdit(id,0.)")!=std::string::npos);

  for(int step=0;step<16;++step) {
    const auto tag=std::string("<control-tag name=\"GDouble")+std::to_string(step)+"\" tag=\""+std::to_string(448+step)+"\"/>";
    assert(xml.find(tag)!=std::string::npos);
    const auto button=std::string("control-tag=\"GDouble")+std::to_string(step)+"\"";
    assert(xml.find(button)!=std::string::npos);
  }
  // Guitar matrix is six exclusive articulation rows per 16 steps.
  // Its toggle surface is kept stateful; the controller explicitly restores
  // the selected cell on redundant OFF callbacks.
  for(int row=0;row<6;++row)for(int step=0;step<16;++step) {
    const auto tag="control-tag=\"GCell"+std::to_string(row)+"_"+std::to_string(step)+"\"";
    const auto pos=xml.find(tag);
    assert(pos!=std::string::npos);
    const auto end=xml.find("/>",pos);
    assert(end!=std::string::npos);
    assert(xml.substr(pos,end-pos).find("kick-style=\"false\"")!=std::string::npos);
  }
  // 150% must produce integral host window dimensions (1680 x 960).
  assert((1120*3)%2==0 && (640*3)%2==0);
  std::regex tagDef(R"RE(<control-tag name="([^"]+)" tag="([0-9]+)")RE");
  std::regex viewTag(R"RE(<view[^>]*control-tag="([^"]+)")RE");
  std::set<std::string> tags;
  std::set<std::string> viewTags;
  std::map<std::string,int> counts;
  for(auto it=std::sregex_iterator(xml.begin(),xml.end(),tagDef);it!=std::sregex_iterator();++it){
    const auto tag=(*it)[1].str();
    assert(tags.insert(tag).second);
    counts[tag]=std::stoi((*it)[2].str());
  }
  for(auto it=std::sregex_iterator(xml.begin(),xml.end(),viewTag);it!=std::sregex_iterator();++it){
    const auto tag=(*it)[1].str();
    assert(tags.count(tag)==1);
    assert(viewTags.insert(tag).second);
  }
  assert(viewTags.size()>=160);
  for(int n=0;n<16;++n){
    for(int row=0;row<6;++row){
      const auto name="GCell"+std::to_string(row)+"_"+std::to_string(n);
      assert(viewTags.count(name)==1);
      assert(counts.at(name)==500+row*16+n);
    }
    for(int row=0;row<3;++row){
      const auto name="Drum"+std::to_string(row)+"_"+std::to_string(n);
      assert(viewTags.count(name)==1);
      assert(counts.at(name)==200+row*16+n);
    }
    assert(viewTags.count("GLock"+std::to_string(n))==1);
    for(const auto* prefix: {"DegreeMinus","DegreeValue","DegreePlus"})
      assert(viewTags.count(std::string(prefix)+std::to_string(n))==1);
    assert(counts.at("DegreeMinus"+std::to_string(n))==800+n);
    assert(counts.at("DegreePlus"+std::to_string(n))==816+n);
    assert(counts.at("DegreeValue"+std::to_string(n))==850+n);
    assert(viewTags.count("GDegree"+std::to_string(n))==0);
  }
  for(int tag=701;tag<=702;++tag)assert(viewTags.count("Action"+std::to_string(tag))==1);
  for(int tag=710;tag<=720;++tag)assert(viewTags.count("Action"+std::to_string(tag))==1);
  assert(xml.find("pending")==std::string::npos);
  // The 100/150 controls must be actually registered and present exactly once.
  assert(counts.at("Action701")==701 && counts.at("Action702")==702);
  assert(counts.at("GuitarGenerate")==700);

  // The 640px root editor must not clip any direct or nested child.
  std::regex child(R"RE(<view[^>]*origin="([0-9]+), ([0-9]+)"[^>]*size="([0-9]+), ([0-9]+)")RE");
  for(auto it=std::sregex_iterator(xml.begin(),xml.end(),child);it!=std::sregex_iterator();++it){
    int x=std::stoi((*it)[1]),y=std::stoi((*it)[2]);
    int w=std::stoi((*it)[3]),h=std::stoi((*it)[4]);
    assert(x+w<=1120 && y+h<=640);
  }
  // A flat bounds check cannot catch the original Rifferator failure:
  // the DOUBLE row extended beyond its 345px guitar parent, while the
  // PM controls overlapped it. Validate nested geometry in every page.
  struct Rect {int x,y,w,h; std::string page; std::string name;};
  std::vector<Rect> stack{{0,0,1120,640,"header","root"}};
  std::vector<Rect> interactive;
  const std::regex viewTokens(R"RE(<view\b[^>]*\/>|<view\b[^>]*>|<\/view>)RE");
  const std::regex originRe(R"RE(origin="([0-9]+), ([0-9]+)")RE");
  const std::regex sizeRe(R"RE(size="([0-9]+), ([0-9]+)")RE");
  const std::regex ctagRe(R"RE(control-tag="([^"]+)")RE");
  for(auto i=std::sregex_iterator(xml.begin(),xml.end(),viewTokens);
      i!=std::sregex_iterator();++i) {
    const std::string node=(*i)[0].str();
    if(node=="</view>") {
      assert(stack.size()>1);
      stack.pop_back();
      continue;
    }
    std::smatch p,sz,tag;
    assert(std::regex_search(node,p,originRe));
    assert(std::regex_search(node,sz,sizeRe));
    const int lx=std::stoi(p[1]),ly=std::stoi(p[2]);
    const int w=std::stoi(sz[1]),h=std::stoi(sz[2]);
    const Rect& parent=stack.back();
    assert(lx>=0 && ly>=0 && w>0 && h>0);
    assert(lx+w<=parent.w && ly+h<=parent.h);
    std::string page=parent.page;
    if(node.find("rifferator-page=\"guitar\"")!=std::string::npos)
      page="guitar";
    else if(node.find("rifferator-page=\"rhythm\"")!=std::string::npos)
      page="rhythm";
    Rect rect{parent.x+lx,parent.y+ly,w,h,page,""};
    if(std::regex_search(node,tag,ctagRe)) {
      rect.name=tag[1].str();
      interactive.push_back(rect);
    }
    if(node.size()<2 || node.substr(node.size()-2)!="/>")
      stack.push_back(rect);
  }
  assert(stack.size()==1);
  assert(interactive.size()>=280);
  // A hidden page occupies the same coordinates by design; compare only
  // controls on the same visible page. Any colliding interactive surfaces
  // would be unclickable/misleading despite VST3 validator passing.
  for(std::size_t i=0;i<interactive.size();++i)
    for(std::size_t j=i+1;j<interactive.size();++j) {
      const auto& a=interactive[i];
      const auto& b=interactive[j];
      if(a.page!=b.page)continue;
      const int dx=std::min(a.x+a.w,b.x+b.w)-std::max(a.x,b.x);
      const int dy=std::min(a.y+a.h,b.y+b.h)-std::max(a.y,b.y);
      assert(dx<=0 || dy<=0);
    }
  for(const auto& button:interactive) {
    if(button.name=="GuitarPage" || button.name=="RhythmPage")
      assert(button.y+button.h<=106); // no transparent page over header
    if(button.name.rfind("GDouble",0)==0)
      assert(button.y>=458 && button.y+button.h<=491);
  }
  assert(xml.find("rifferator-page=\"guitar\" origin=\"0, 106\" size=\"1120, 534\"")!=std::string::npos);
  assert(xml.find("rifferator-page=\"rhythm\" origin=\"0, 106\" size=\"1120, 534\"")!=std::string::npos);
  assert(xml.find("<control-tag name=\"GuitarPage\" tag=\"728\"/>")!=std::string::npos);
  assert(xml.find("<control-tag name=\"RhythmPage\" tag=\"729\"/>")!=std::string::npos);
  assert(controller.find("attributes.getAttributeValue(\"rifferator-page\")")!=std::string::npos);
  assert(controller.find("if(command==728 || command==729)")!=std::string::npos);
  assert(controller.find("guitarPageView_->setVisible(!rhythmPageSelected_)")!=std::string::npos);
  assert(controller.find("rhythmPageView_->setVisible(rhythmPageSelected_)")!=std::string::npos);

}
