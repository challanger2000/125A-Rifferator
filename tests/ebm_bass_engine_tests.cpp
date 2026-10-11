#include "../src/core/EbmBassEngine.hpp"
#include <cassert>
using namespace rifferator::ebm;
int main() {
    Settings s;
    auto a=generate(s), b=generate(s);
    assert(a.steps==16 && a.count==16);
    for(unsigned i=0;i<a.count;++i) {
        assert(a.notes[i].pitch==b.notes[i].pitch);
        assert(a.notes[i].step==b.notes[i].step);
        assert(a.notes[i].gatePermille==850);
        assert(a.notes[i].pitch==36 || a.notes[i].pitch==37);
    }
    s.style=Style::Industrial;
    a=generate(s);
    assert(a.count==11);
    for(unsigned i=0;i<a.count;++i) assert(a.notes[i].pitch<=43);
    s.style=Style::Octave;
    a=generate(s);
    assert(a.count==14);
    for(unsigned i=0;i<a.count;++i) assert(a.notes[i].pitch==36 || a.notes[i].pitch==48);
    s.steps=32; s.style=Style::Chromatic; s.variation=100;
    a=generate(s); b=generate(s);
    assert(a.count==32);
    for(unsigned i=0;i<a.count;++i) assert(a.notes[i].pitch==b.notes[i].pitch);
    s.gatePermille=0;
    assert(generate(s).count==0);
    s.gatePermille=1000; s.root=127; s.velocity=127;
    a=generate(s);
    for(unsigned i=0;i<a.count;++i) {
        assert(a.notes[i].pitch<=127 && a.notes[i].velocity<=127);
        assert(a.notes[i].gatePermille==1000);
    }
}
