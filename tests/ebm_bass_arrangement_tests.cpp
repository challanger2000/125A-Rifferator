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
    plan.sequence.gatePermille=0;
    prepare(source,plan);
    assert(source.midi.bass.empty());
    assert(source.midi.guitar.size()==1);
    plan.sequence.gatePermille=850;
    plan.tension=Tension::Aggressive;
    plan.mode=BassMode::Ebm;
    plan.sequence.variation=100;
    plan.phraseVariation=0;
    prepare(source,plan);
    assert(source.midi.bass.size()==256);
    for (int i=0;i<16;++i)
      assert(source.midi.bass[i].note==source.midi.bass[4*16+i].note);
    plan.phraseVariation=100;
    prepare(source,plan);
    const auto first=source.midi.bass;
    prepare(source,plan);
    assert(source.midi.bass.size()==first.size());
    for (unsigned i=0;i<first.size();++i) {
      assert(source.midi.bass[i].note==first[i].note);
      assert(source.midi.bass[i].startTick==first[i].startTick);
    }

    plan.sequence.gatePermille=500;
    prepare(source,plan);
    for(const auto& n:source.midi.bass) {
      assert(n.endTick-n.startTick==120);
      assert(n.boundaryTick==n.endTick);
    }
    plan.sequence.gatePermille=1000;
    prepare(source,plan);
    for(const auto& n:source.midi.bass) assert(n.endTick-n.startTick==240);
    plan.sequence.gatePermille=1;
    prepare(source,plan);
    for(const auto& n:source.midi.bass) assert(n.endTick-n.startTick==1);

    // Fills never alter note timing, rhythm or guitar notes.
    plan.sequence.gatePermille=850;
    plan.sequence.variation=0;
    plan.phraseVariation=0;
    plan.fillIntensity=0;
    prepare(source,plan);
    const auto unfilled=source.midi.bass;
    plan.fillIntensity=100;
    plan.fillEveryBars=4;
    prepare(source,plan);
    assert(source.midi.bass.size()==unfilled.size());
    bool accented=false;
    for (unsigned i=0;i<unfilled.size();++i) {
      const auto& before=unfilled[i];
      const auto& after=source.midi.bass[i];
      assert(before.startTick==after.startTick);
      assert(before.endTick==after.endTick);
      const auto bar=static_cast<int>(before.startTick/3840);
      if(bar%4!=3 || before.startTick%3840 < 2880) {
        assert(before.note==after.note);
        assert(before.velocity==after.velocity);
      } else if(after.velocity>before.velocity) accented=true;
    }
    assert(accented);
    assert(source.midi.guitar.size()==1);
    // Identical seed, same output; invalid fill division disables fills.
    const auto withFill=source.midi.bass;
    prepare(source,plan);
    for(unsigned i=0;i<withFill.size();++i) {
      assert(withFill[i].note==source.midi.bass[i].note);
      assert(withFill[i].velocity==source.midi.bass[i].velocity);
    }
    plan.fillEveryBars=5;
    prepare(source,plan);
    for(unsigned i=0;i<unfilled.size();++i) {
      assert(unfilled[i].note==source.midi.bass[i].note);
      assert(unfilled[i].velocity==source.midi.bass[i].velocity);
    }
}
