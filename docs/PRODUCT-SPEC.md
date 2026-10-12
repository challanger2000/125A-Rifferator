# 125A Rifferator V1 — Product Contract (development)
Status: **ACTIVE V1 PRODUCT CONTRACT / DEVELOPMENT**. Windows development
build and automated core/GUI-layout tests passed CI #38293244458
(29/29 CTest, Steinberg Validator 537/537), but **real Studio One GUI,
MIDI bus routing, 125A Plugin Tester and full release QA are NOT VERIFIED**.
The requirements below remain binding until implemented or explicitly
re-scoped by the product owner; a green CI result never removes them.

## Implementation tracking (as of CI #38293244458)

| V1 requirement | Current development evidence | Release disposition |
| --- | --- | --- |
| 16-step Guitar grid, PM/OPEN/NOISE/SOFT/HARMONIC, STOP, DEGREE, LOCK, DOUBLE | Controller parameter commands, scheduler, state and MIDI event regressions in 29/29 passing CTests | Automated PASS; Studio One GUI/target sample-library check pending |
| Guitar GENERATE/VARIATE and CLEAR | Seeded core generator connected to GUI; conservative VARIATE preserves unchanged STOP and LOCK; CI PASS | Automated PASS; Studio One click/highlight behavior pending |
| FOLLOW, EBM, HYBRID bass; EBM style/gate/velocity | Dedicated bus 1, Controller GUI actions and Processor regressions; CI PASS | Automated PASS; Studio One 4-bus routing pending |
| Three GM Drum rows, global ON/OFF, STANDARD/GENERATE/VARIATE | Dedicated bus 2, GUI/controller/seed regressions; CI PASS | Automated PASS; real host check pending |
| Chords and quality/mode (additional current feature) | Dedicated bus 3 with MIDI/state regressions; CI PASS | Automated PASS; real host check pending |
| PM MIN/MAX, Root MIDI, Guitar/Bass Gate, Fill intensity | Front-panel parameter controls and processor regression; CI PASS | Automated PASS; target instrument/musical validation pending |
| 100/150% GUI, zoom state | Versioned controller state tested; UIDESC template and bundle resources validated in CI | **HOST-GUI BLOCKER** until actual Studio One native editor opens and reopens; 150% content is 1680×1044 before host chrome |
| 2–32 steps, 1/4–1/32 selectable clock grid and paging | 16-step front panel; internal source includes length-generic arrangement functions but no full user-facing 32-step editor/rate controls | **NOT IMPLEMENTED as specified** |
| Independent seeded randomization toggles for rhythm/articulation/pitch/fill | Generator core supports options; GUI exposes Generate/Variate with fixed configured choices, not independent switches | **NOT IMPLEMENTED as specified** |
| Fill every 4/8/16 bars user choice | Core FillOptions supports interval; exposed GUI controls intensity only | **NOT IMPLEMENTED as specified** |
| Velocity profile editor/save profiles, user bass octave offset | Core contains velocity bands/offset; GUI exposes PM bounds and bass velocity but not all per-articulation thresholds, profile save, or octave offset editor | **NOT IMPLEMENTED as specified** |
| Separate Guitar/Bass SMF export | No approved end-to-end export implementation demonstrated | **NOT VERIFIED / NOT IMPLEMENTED** |
| Drum LOCK independently per step (GUI-specific contract) | Generator accepts locks in core; V1 UI has no drum lock controls/persisted drum lock state | **NOT IMPLEMENTED** |

See `docs/RELEASE-AUDIT-V1.md` for defect/evidence tracking and
`docs/HOST-GUI-QA.md` for the exact native DAW acceptance protocol.



## Purpose
A compact metal/NDH MIDI riff generator with guitar as master pattern and a derived monophonic bass output. No audio synthesis and no copied Kontakt scripts. Musical inspiration: Thrash DI riffmaker.

## Step model
- 2–32 steps, default 16; selectable 1/4, 1/8, 1/16, 1/32; beat/transport synchronization.
- Each step is ON/OFF plus one articulation: PN (picking noise), PM (palm mute), SP (soft pick), PC (hard/open pick), PH (pinch harmonic). OFF is deliberate silence.
- Independent DOUBLE creates two consecutive strokes within one occupied step. For DOUBLE, both strokes remain inside their substep, with distinct note-offs.
- Per-step optional pitch as integer scale degree relative to selected key/scale, default unison (degree 1). Chromatic overrides are not part of the V1 core.
- Gate is note duration bounded by the next stroke OR start of a deliberate REST, whichever is earlier; global guitar gate default 95%, bass gate default 95%. At zero gate, suppress the note entirely; at 100% note-off at the boundary before any same-note note-on.
- No notes stuck after stop, jump, seek, reactivation, pattern edit or preset load.
- Randomization is seeded and reproducible; independent toggles for rhythm, articulations, pitch and fills. Preserve locked steps.
- Main riff can remain constant; optional fill at every 4/8/16 bars, density and pitch governed by musical scale. Fill 0% OFF.
- MIDI preset/save/restore and separate Guitar/Bass SMF export planned.

## Default Heavier7Strings articulation profile
Requires VEL. MUTING enabled in the instrument, per its manufacturer documentation:
- PN: 1–29 (default 20)
- PM: 30–83 (default 50)
- SP: 84–111 (default 95)
- PC: 112–126 (default 115)
- PH: 127
These are MIDI velocity bands, NOT guaranteed power-chord voicings: instrument determines sample/voicing. All thresholds and velocities are user-configurable and saved as profiles.

## Bass behavior
- Guitar is the single authoritative pattern. Every guitar edit, randomization and fill automatically derives matching bass notes.
- Bass emits monophonic roots, not powerchords, on a dedicated logical MIDI output. Default velocity 100. Bass octave offset configurable; actual range instrument-dependent.
- Bass follows active PM/SP/PC steps and pitched fills; PN and PH default to no bass note. DOUBLE inherits guitar stroke timing unless later explicitly configured.
- Note that separate logical MIDI outputs do NOT imply that any VST3 host can route two event buses. Bus arrangements and Studio One routing must be tested before making a compatibility promise. A channel-split/export fallback may be required.

## QA gates
Verify event ordering, boundary cases and rest handling, DOUBLE, articulation velocity confinement, deterministic randomness, scale pitch, bass follower invariant, offline vs realtime, state/preset recall, tempo/position jumps, MIDI event bus negotiation, VST3 Validator and 125A release QA.
Current automated Windows evidence is recorded above; **no completed Studio One host QA or 125A release approval exists yet**.
