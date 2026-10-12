#include <cassert>
#include <fstream>
#include <regex>
#include <string>
#include <set>
#include <map>
#include <iterator>
int main(){
  std::ifstream file("resource/rifferator.uidesc");
  assert(file.good());
  const std::string xml((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
  assert(xml.find("size=\"1120, 696\"")!=std::string::npos);
  // DOUBLE must be produced once, in the live scheduler, never pre-rendered.
  std::ifstream processorFile("src/RifferatorProcessor.cpp");
  assert(processorFile.good());
  const std::string processor((std::istreambuf_iterator<char>(processorFile)),
                               std::istreambuf_iterator<char>());
  assert(processor.find("step.doubled=false;")!=std::string::npos);

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
  assert(xml.find("control-tag=\"PmMinValue\" origin=\"157, 382\" size=\"48, 24\"")!=std::string::npos);
  assert(xml.find("control-tag=\"PmMaxValue\" origin=\"296, 382\" size=\"48, 24\"")!=std::string::npos);

  // GUI wiring regression: every visible switch must have the controller
  // handler that publishes its VST3 parameter edit (not a dead decoration).
  std::ifstream controllerFile("src/RifferatorFactory.cpp");
  assert(controllerFile.good());
  const std::string controller((std::istreambuf_iterator<char>(controllerFile)),
                               std::istreambuf_iterator<char>());
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

  // GENERATE resets previous DOUBLE triggers only after the LOCK guard.
  const auto generateStart=controller.find("if(control->getTag()==700)");
  const auto generateEnd=controller.find("const int tag=control->getTag();",generateStart);
  assert(generateStart!=std::string::npos && generateEnd!=std::string::npos);
  const auto generateCode=controller.substr(generateStart,generateEnd-generateStart);
  assert(generateCode.find("kGuitarLockBase+i")<generateCode.find("edit(kGuitarDoubleBase+i,0.)"));
  assert(generateCode.find("edit(kGuitarDoubleBase+i,0.)")!=std::string::npos);

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
  // 150% must produce integral host window dimensions (1680 x 1044).
  assert((1120*3)%2==0 && (696*3)%2==0);
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

  // The root editor is 696 pixels high and must not clip a direct child.
  std::regex child(R"RE(<view[^>]*origin="([0-9]+), ([0-9]+)"[^>]*size="([0-9]+), ([0-9]+)")RE");
  for(auto it=std::sregex_iterator(xml.begin(),xml.end(),child);it!=std::sregex_iterator();++it){
    int x=std::stoi((*it)[1]),y=std::stoi((*it)[2]);
    int w=std::stoi((*it)[3]),h=std::stoi((*it)[4]);
    assert(x+w<=1120 && y+h<=696);
  }
}
