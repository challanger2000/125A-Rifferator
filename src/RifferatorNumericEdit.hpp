#pragma once
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>
namespace a125::rifferator {
inline bool parseInteger(std::string s,int& out) {
  while(!s.empty()&&(std::isspace(static_cast<unsigned char>(s.back()))||s.back()=='%'))s.pop_back();
  const char* begin=s.c_str();char* end=nullptr;
  const long v=std::strtol(begin,&end,10);
  if(end==begin)return false;
  while(*end&&std::isspace(static_cast<unsigned char>(*end)))++end;
  if(*end!='\0'||v<-10000||v>10000)return false;
  out=static_cast<int>(v);return true;
}
inline std::string noteName(int midi) {
  constexpr const char* names[12]{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
  const int v=std::clamp(midi,0,127);
  return std::string(names[v%12])+std::to_string(v/12-1);
}
inline bool parseNote(std::string s,int& out) {
  if(parseInteger(s,out))return out>=0&&out<=127;
  for(char& c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  if(s.empty())return false;
  const char* letters="CDEFGAB";
  constexpr int semis[7]{0,2,4,5,7,9,11};
  const char* where=std::strchr(letters,s[0]);
  if(!where)return false;
  int p=semis[where-letters];std::size_t i=1;
  if(i<s.size()&&(s[i]=='#'||s[i]=='B')){p+=s[i]=='#'?1:-1;++i;}
  int octave=0;
  if(!parseInteger(s.substr(i),octave))return false;
  out=12*(octave+1)+p;
  return out>=0&&out<=127;
}
} // namespace a125::rifferator
