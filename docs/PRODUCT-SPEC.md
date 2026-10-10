# 125A Rifferator V1 — Product Contract (development)
Status: SPECIFIED, not built or QA approved.

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
No build, validator or host QA claims exist yet.
