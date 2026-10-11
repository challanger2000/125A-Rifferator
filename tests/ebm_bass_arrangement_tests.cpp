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

    // Unset harmony falls back to the configured EBM root (not MIDI zero).
    BassPlan defaults;
    defaults.mode=BassMode::Ebm;
    defaults.tension=Tension::Stable;
    defaults.sequence.root=52;
    Arrangement emptyGuitar;
    prepare(emptyGuitar,defaults);
    assert(emptyGuitar.midi.bass.size()==256);
    for(const auto& n:emptyGuitar.midi.bass) assert(n.note==40);
    // MIDI root zero is a genuine note and must not be mistaken for "unset".
    defaults.harmonicRoots[0]=0;
    prepare(emptyGuitar,defaults);
    for(const auto& n:emptyGuitar.midi.bass) assert(n.note==0);
    // HYBRID changes harmony without inheriting guitar pauses.
    defaults.mode=BassMode::Hybrid;
    defaults.harmonicRoots.fill(52);
    defaults.harmonicRoots[7]=57;
    prepare(emptyGuitar,defaults);
    assert(emptyGuitar.midi.bass.size()==256);
    assert(emptyGuitar.midi.bass[6*16].note==40);
    assert(emptyGuitar.midi.bass[7*16].note==45);
    assert(emptyGuitar.midi.bass[8*16].note==40);
    for (unsigned i=1;i<emptyGuitar.midi.bass.size();++i) {
      assert(emptyGuitar.midi.bass[i].startTick>emptyGuitar.midi.bass[i-1].startTick);
      assert(emptyGuitar.midi.bass[i-1].endTick<=emptyGuitar.midi.bass[i].startTick);
      assert(emptyGuitar.midi.bass[i].endTick<=emptyGuitar.periodTicks);
    }
    // Invalid geometry must never corrupt a prepared arrangement.
    const auto before=emptyGuitar.midi.bass;
    defaults.ticksPerStep=1000001;
    prepare(emptyGuitar,defaults);
    assert(emptyGuitar.midi.bass.size()==before.size());

    // Silent EBM still supplies a valid 16-bar transport period for a fresh arrangement.
    BassPlan silent;
    silent.mode=BassMode::Ebm;
    silent.ticksPerStep=120;
    silent.sequence.gatePermille=0;
    Arrangement silence;
    silence.midi.guitar.push_back({0,100,40,90,120});
    prepare(silence,silent);
    assert(silence.periodTicks==30720);
    assert(silence.midi.bass.empty());
    assert(silence.midi.guitar.size()==1);
    // Invalid silent geometry must leave the original arrangement untouched.
    silent.ticksPerStep=-1;
    prepare(silence,silent);
    assert(silence.periodTicks==30720);

    BassPlan arc;
    arc.mode=BassMode::Ebm;
    arc.tension=Tension::Stable;
    arc.sequence.velocity=90;
    Arrangement shaped;
    prepare(shaped,arc);
    const auto baseline=shaped.midi.bass;
    arc.dynamics=96;
    prepare(shaped,arc);
    assert(shaped.midi.bass.size()==baseline.size());
    bool altered=false;
    for(unsigned i=0;i<baseline.size();++i) {
      const auto& original=baseline[i];
      const auto& modified=shaped.midi.bass[i];
      assert(original.startTick==modified.startTick);
      assert(original.endTick==modified.endTick);
      assert(original.note==modified.note);
      const int bar=static_cast<int>(original.startTick/3840);
      if(bar<4) assert(modified.velocity<=original.velocity);
      if(bar>=12) assert(modified.velocity>=original.velocity);
      if(modified.velocity!=original.velocity) altered=true;
    }
    assert(altered);
    const auto repeat=shaped.midi.bass;
    prepare(shaped,arc);
    for(unsigned i=0;i<repeat.size();++i)
      assert(repeat[i].velocity==shaped.midi.bass[i].velocity);
    arc.dynamics=0;
    prepare(shaped,arc);
    for(unsigned i=0;i<baseline.size();++i)
      assert(baseline[i].velocity==shaped.midi.bass[i].velocity);

}
