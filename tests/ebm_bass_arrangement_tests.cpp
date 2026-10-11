#include "../src/core/EbmBassArrangement.hpp"
#include <cassert>
using namespace a125::rifferator;
using namespace a125::rifferator::ebm_integration;
int main() {
    Arrangement source;
    source.periodTicks=61440;
    source.midi.guitar.push_back({0,200,40,50,240});
    source.midi.bass.push_back({0,200,28,100,240});
    BassPlan plan;
    plan.sequence.style=ebm::Style::Chromatic;
    plan.harmonicRoots.fill(40);
    plan.harmonicRoots[8]=41;
    prepare(source,plan); // FOLLOW must be bitwise behaviour-preserving
    assert(source.midi.bass.size()==1 && source.midi.bass[0].note==28);
    plan.mode=BassMode::Ebm;
    plan.tension=Tension::Aggressive;
    prepare(source,plan);
    assert(source.midi.guitar.size()==1 && source.midi.guitar[0].note==40);
    assert(source.midi.bass.size()==256);
    for(const auto& n:source.midi.bass) {
       assert(n.note==28 || n.note==29);
       assert(n.boundaryTick>n.startTick);
    }
    const auto ebmBar8=source.midi.bass[8*16].note;
    plan.mode=BassMode::Hybrid;
    prepare(source,plan);
    assert(source.midi.bass[8*16].note==ebmBar8+1);
    assert(source.midi.bass[0].note==28);
    plan.tension=Tension::Stable;
    prepare(source,plan);
    for(const auto& n:source.midi.bass) assert(n.note==28 || n.note==29);
    assert(source.periodTicks==61440);
}
