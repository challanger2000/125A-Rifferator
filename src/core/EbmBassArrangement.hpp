#pragma once
#include "EbmBassEngine.hpp"
#include "RiffArrangement.hpp"
#include <array>
#include <algorithm>
#include <cstdint>
#include <utility>

namespace a125::rifferator::ebm_integration {
namespace ebm = ::rifferator::ebm;
enum class BassMode : uint8_t { Follow, Ebm, Hybrid };
enum class Tension : uint8_t { Stable, Tension, Aggressive };
struct BassPlan {
    BassMode mode{BassMode::Follow};   // Preserves existing bass by default
    Tension tension{Tension::Tension};
    ebm::Settings sequence{};
    int ticksPerStep{240};             // 1/16 at 960 PPQ
    int bars{16};
    int bassOctaveOffset{-12};
    // Explicit harmonic roots: absolute MIDI pitch, one per bar.
    // EBM ignores later roots; Hybrid consults root at each bar.
    std::array<uint8_t,16> harmonicRoots{};
    uint8_t phraseVariation{0}; // 0: locked four-bar motifs
    uint8_t fillIntensity{0};   // 0: no fills; 100: strongest turnaround
    uint8_t fillEveryBars{4};  // 4, 8 or 16; other values disable fills
};
inline int controlledInterval(int raw, Tension tension, uint32_t hash) noexcept {
    if (raw==0 || raw==12 || raw==7) return raw;
    if (tension==Tension::Stable) return 0;
    if (tension==Tension::Tension)
        return (hash%4u==0u) ? raw : 0; // sparse controlled chromatic motion
    return raw;                         // deliberately pronounced friction
}
// Off-audio-thread only: builds the bass section of an existing Arrangement.
// Guitar notes and FOLLOW bass remain unchanged. Pattern period is bars*16 steps.
// VST3 host event scheduling continues through existing BlockScheduler.
inline void prepare(Arrangement& arrangement, const BassPlan& p) {
    if(p.mode==BassMode::Follow) return;
    if(p.bars!=16 || p.ticksPerStep<=0) return;
    if(p.sequence.gatePermille==0) { arrangement.midi.bass.clear(); return; }
    std::vector<MidiNote> bass;
    bass.reserve(16u*32u);
    auto sequence=p.sequence;
    const auto stepTicks=static_cast<int64_t>(p.ticksPerStep);
    const auto barTicks=stepTicks*16;
    for(int bar=0;bar<16;++bar) {
        const bool fillBar=(p.fillEveryBars==4 || p.fillEveryBars==8 || p.fillEveryBars==16)
            && ((bar+1)%p.fillEveryBars==0) && p.fillIntensity>0;
        sequence.seed=p.sequence.seed+(p.phraseVariation ? static_cast<uint32_t>(bar/4) : 0u);
        sequence.variation=static_cast<uint8_t>(std::min<unsigned>(p.sequence.variation,p.phraseVariation));
        sequence.root=static_cast<uint8_t>(std::clamp<int>(
          p.mode==BassMode::Hybrid ? p.harmonicRoots[bar] : p.harmonicRoots[0],0,127));
        sequence.steps=16;
        const auto pattern=ebm::generate(sequence);
        for(unsigned i=0;i<pattern.count;++i) {
            const auto& n=pattern.notes[i];
            const auto hash=ebm::mix32(sequence.seed ^ (static_cast<uint32_t>(bar)*17u+n.step));
            const auto raw=static_cast<int>(n.pitch)-sequence.root;
            const auto interval=controlledInterval(raw,p.tension,hash);
            // Last-quarter turnaround: no extra notes, no interruption of groove.
            // Accents and optional octave lift preserve rhythmic independence.
            const bool turnaround=fillBar && n.step>=12;
            const auto fillHash=ebm::mix32(p.sequence.seed ^ (static_cast<uint32_t>(bar)*131u+n.step));
            const bool lift=turnaround && (fillHash%100u)<std::min<unsigned>(p.fillIntensity,100u)/2u;
            const auto note=clampMidi(static_cast<int>(sequence.root)+interval+(lift?12:0)+p.bassOctaveOffset);
            const int accent=turnaround ? static_cast<int>(std::min<unsigned>(p.fillIntensity,100u))/10 : 0;
            const int velocity=std::clamp(static_cast<int>(n.velocity)+accent,1,127);
            const auto start=barTicks*bar+stepTicks*n.step;
            if(n.gatePermille==0) continue;
            // Per-note gate maps to the MIDI lifetime. During integration,
            // set the scheduler's live bassGate to 100% in EBM/HYBRID.
            const auto duration=std::min(stepTicks,std::max<int64_t>(1,
                stepTicks*static_cast<int64_t>(n.gatePermille)/1000));
            const auto off=start+duration;
            bass.push_back({start,off,note,velocity,off});
        }
    }
    arrangement.midi.bass=std::move(bass);
    arrangement.periodTicks=barTicks*16;
}
} // namespace a125::rifferator::ebm_integration
