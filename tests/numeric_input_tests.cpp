#include "../src/RifferatorNumericEdit.hpp"
#include <cassert>
using namespace a125::rifferator;
int main(){
 int out=-1;
 assert(noteName(40)=="E2");
 assert(noteName(43)=="G2");
 assert(noteName(60)=="C4");
 assert(parseNote("G2",out)&&out==43);
 assert(parseNote("F#2",out)&&out==42);
 assert(parseNote("Bb2",out)&&out==46);
 assert(parseNote("E2",out)&&out==40);
 assert(parseNote("40",out)&&out==40);
 assert(!parseNote("H2",out));
 assert(!parseNote("C33",out));
 assert(parseInteger("100%",out)&&out==100);
 assert(parseInteger(" -13 ",out)&&out==-13);
 assert(!parseInteger("35banana",out));
}
