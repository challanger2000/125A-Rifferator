#include "../src/core/EbmBassArrangement.hpp"
#include <cassert>
#include <cstdint>

using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;

static void validate(const Arrangement& a) {
    assert(a.periodTicks==61440);
    int64_t last=-1;
    for(const auto& n:a.midi.bass) {
        assert(n.startTick>last);
        assert(n.startTick>=0);
        assert(n.startTick<a.periodTicks);
        assert(n.endTick>n.startTick);
        assert(n.endTick<=a.periodTicks);
        assert(n.boundaryTick==n.endTick);
        assert(n.note>=0 && n.note<=127);
        assert(n.velocity>=1 && n.velocity<=127);
        last=n.startTick;
    }
}
int main() {
    for(int style=0;style<5;++style) {
        for(int tension=0;tension<3;++tension) {
            for(int root: {0,12,36,60,120,127}) {
                BassPlan p;
                p.mode=BassMode::Hybrid;
                p.sequence.style=static_cast<rifferator::ebm::Style>(style);
                p.tension=static_cast<Tension>(tension);
                p.sequence.root=static_cast<uint8_t>(root);
                p.harmonicRoots.fill(static_cast<uint8_t>(root));
                p.harmonicRoots[8]=static_cast<uint8_t>(127-root);
                p.sequence.variation=100;
                p.phraseVariation=100;
                p.fillIntensity=100;
                p.dynamics=100;
                for(int gate: {1,500,850,1000}) {
                    p.sequence.gatePermille=static_cast<uint16_t>(gate);
                    Arrangement a;
                    prepare(a,p);
                    validate(a);
                    const auto before=a.midi.bass;
                    prepare(a,p);
                    assert(a.midi.bass.size()==before.size());
                    for(std::size_t i=0;i<before.size();++i) {
                        const auto& x=before[i];
                        const auto& y=a.midi.bass[i];
                        assert(x.startTick==y.startTick);
                        assert(x.endTick==y.endTick);
                        assert(x.note==y.note);
                        assert(x.velocity==y.velocity);
                    }
                }
            }
        }
    }
    BassPlan p;
    p.mode=BassMode::Hybrid;
    p.sequence.gatePermille=1000;
    p.harmonicRoots.fill(40);
    p.harmonicRoots[15]=52;
    Arrangement a;
    prepare(a,p);
    validate(a);
    assert(!a.midi.bass.empty());
    assert(a.midi.bass.back().endTick==a.periodTicks);
    p.sequence.gatePermille=0;
    prepare(a,p);
    assert(a.midi.bass.empty());
}
