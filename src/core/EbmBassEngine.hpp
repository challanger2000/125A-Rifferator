#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace rifferator::ebm {
enum class Style : uint8_t { Classic, Chromatic, Octave, Industrial, Rolling };
struct Note {
    uint8_t step{};
    uint8_t pitch{};
    uint8_t velocity{};
    uint16_t gatePermille{};
};
struct Pattern {
    std::array<Note,32> notes{};
    uint8_t count{};
    uint8_t steps{16};
};
struct Settings {
    Style style{Style::Chromatic};
    uint8_t root{36};                // MIDI C2 (scientific pitch notation)
    uint8_t steps{16};               // 16 or 32 sixteenth-note slots
    uint8_t velocity{100};
    uint16_t gatePermille{850};       // 0..1000, 0 suppresses note
    uint8_t variation{0};            // 0..100, motif-preserving pitch substitutions
    uint8_t rhythmVariation{0};      // 0..100, optional deterministic offbeat rests
    uint32_t seed{1};
};
constexpr uint32_t mix32(uint32_t x) noexcept {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
    x *= 0x846ca68bu; x ^= x >> 16; return x;
}
constexpr int interval(Style style, unsigned slot) noexcept {
    constexpr std::array<int,16> classic{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    constexpr std::array<int,16> chromatic{0,0,1,0,0,1,0,0,0,0,1,0,0,1,0,0};
    constexpr std::array<int,16> octave{0,0,12,0,0,12,0,0,0,0,12,0,12,0,0,0};
    constexpr std::array<int,16> industrial{0,0,0,1,0,0,7,0,0,0,0,1,0,0,7,0};
    constexpr std::array<int,16> rolling{0,0,0,1,0,7,0,0,0,0,1,0,7,0,0,0};
    switch(style) {
        case Style::Classic: return classic[slot%16];
        case Style::Chromatic: return chromatic[slot%16];
        case Style::Octave: return octave[slot%16];
        case Style::Industrial: return industrial[slot%16];
        case Style::Rolling: return rolling[slot%16];
    }
    return 0;
}
constexpr bool active(Style style, unsigned slot) noexcept {
    if (style == Style::Industrial) return (slot%16 != 1 && slot%16 != 4 && slot%16 != 7 && slot%16 != 10 && slot%16 != 13);
    if (style == Style::Classic) return slot%2 == 0;
    if (style == Style::Octave) return slot%16 != 7 && slot%16 != 15;
    return true;
}
inline Pattern generate(Settings cfg) noexcept {
    Pattern result;
    result.steps = cfg.steps == 32 ? 32 : 16;
    const auto gate=std::min<uint16_t>(cfg.gatePermille,1000);
    if (!gate) return result;
    for (unsigned i=0;i<result.steps;++i) {
        const unsigned motif=i%16;
        if (!active(cfg.style,motif)) continue;
        const uint32_t noise=mix32(cfg.seed ^ (i*0x9e3779b9u));
        // Preserve beats 1 and 3 and downbeat anchors. Only selected offbeats
        // may rest; zero retains the established groove byte-for-byte.
        const unsigned rhythm=std::min<unsigned>(cfg.rhythmVariation,100u);
        if (rhythm && (motif%4u==1u || motif%4u==3u) &&
            (mix32(cfg.seed ^ (i*0x85ebca6bu) ^ 0xa55a5aa5u)%100u)<rhythm/3u)
            continue;
        // Intentional sparse substitutions only; unchanged root on most notes.
        int semitones=interval(cfg.style,motif);
        if (cfg.variation && (noise%100u) < std::min<unsigned>(cfg.variation,100)/4u)
            semitones=(semitones==0 ? 1 : 0);
        const int midi=std::clamp<int>(static_cast<int>(cfg.root)+semitones,0,127);
        const int accent=(motif%4==0 ? 8 : 0);
        result.notes[result.count++]=Note{
            static_cast<uint8_t>(i),static_cast<uint8_t>(midi),
            static_cast<uint8_t>(std::clamp<int>(static_cast<int>(cfg.velocity)+accent,1,127)),gate
        };
    }
    return result;
}
} // namespace rifferator::ebm
