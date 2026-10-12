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
