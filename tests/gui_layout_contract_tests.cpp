#include <cassert>
#include <fstream>
#include <regex>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <iterator>
#include <algorithm>
struct Rect{int x{},y{},w{},h{};};
static std::pair<int,int> xy(const std::string& tag,const char* key) {
 const std::regex r(std::string(key)+R"REGEX(="([0-9]+), ([0-9]+)")REGEX");
 std::smatch m;assert(std::regex_search(tag,m,r));
 return {std::stoi(m[1].str()),std::stoi(m[2].str())};
}
int main(){
 std::ifstream f("resource/rifferator.uidesc");assert(f.good());
 const std::string xml((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
 assert(xml.find("size=\"1120, 640\"")!=std::string::npos);
 assert(xml.find("BASS / CHORDS / DRUMS")==std::string::npos);
 assert(xml.find("BASS GATE")==std::string::npos);
 std::regex defs(R"REGEX(<control-tag name="([^"]+)" tag="([0-9]+)")REGEX");
 std::map<std::string,int> byName;std::set<int> ids;
 for(std::sregex_iterator it(xml.begin(),xml.end(),defs),end;it!=end;++it){
  assert(byName.emplace((*it)[1].str(),std::stoi((*it)[2].str())).second);
  assert(ids.insert(std::stoi((*it)[2].str())).second);
 }
 const std::vector<std::string> edit{"RootValue","GtrGateValue","FillValue","PmMinValue","PmMaxValue"};
 assert(byName.count("GuitarGallop")&&byName["GuitarGallop"]==723);
 assert(xml.find("class=\\\"CTextButton\\\" control-tag=\\\"GuitarGallop\\\"")!=std::string::npos);
 for(const auto& name:edit) {
  assert(byName.count(name));
  assert(xml.find("class=\"CTextEdit\" control-tag=\""+name+"\"")!=std::string::npos);
 }
 for(int i=0;i<16;++i) {
  for(int row=0;row<6;++row)
   assert(byName.count("GCell"+std::to_string(row)+"_"+std::to_string(i)));
  for(const std::string prefix:{"GuitarActive","GDouble","GLock","DegreeValue"})
   assert(byName.count(prefix+std::to_string(i)));
  assert(xml.find("class=\"CTextEdit\" control-tag=\"DegreeValue"+std::to_string(i)+"\"")!=std::string::npos);
 }
 // Parse nested VSTGUI view hierarchy, not only the root bounds.
 // A control must fit *every* immediate CViewContainer ancestor.
 std::vector<Rect> stack{{0,0,1120,640}},interactive;
 const std::regex views(R"REGEX(<\/?view\b[^>]*>)REGEX");
 const std::regex tagRx(R"REGEX(control-tag="([^"]+)")REGEX");
 std::set<std::string> seenViews;
 for(std::sregex_iterator it(xml.begin(),xml.end(),views),end;it!=end;++it) {
  const std::string t=it->str();
  if(t.rfind("</view",0)==0){assert(stack.size()>1);stack.pop_back();continue;}
  assert(!stack.empty());
  const auto [x,y]=xy(t,"origin");
  const auto [w,h]=xy(t,"size");
  const auto parent=stack.back();
  assert(x>=0&&y>=0&&w>0&&h>0);
  assert(x+w<=parent.w&&y+h<=parent.h);
  const Rect rect{parent.x+x,parent.y+y,w,h};
  std::smatch m;
  if(std::regex_search(t,m,tagRx)) {
   const std::string name=m[1].str();
   assert(byName.count(name));
   assert(seenViews.insert(name).second);
   for(const auto& old:interactive)
     assert(std::min(rect.x+rect.w,old.x+old.w)<=std::max(rect.x,old.x)||
            std::min(rect.y+rect.h,old.y+old.h)<=std::max(rect.y,old.y));
   interactive.push_back(rect);
  }
  if(t.size()<2||t.substr(t.size()-2)!="/>")
    stack.push_back(rect);
 }
 assert(stack.size()==1);
 assert(seenViews.size()==byName.size());
 assert(interactive.size()==213);
 std::ifstream processorFile("src/RifferatorProcessor.cpp");
 const std::string p((std::istreambuf_iterator<char>(processorFile)),std::istreambuf_iterator<char>());
 assert(p.find("addEventOutput(STR16(\"Guitar Out\")")!=std::string::npos);
 assert(p.find("addEventOutput(STR16(\"Bass Out\")")==std::string::npos);
 assert(p.find("addEventOutput(STR16(\"Drums Out\")")==std::string::npos);
}
