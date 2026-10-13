# 125A Rifferator — guitar-only V1 contract

Status: revised 2026-10-13 after direct Studio One testing exposed unreliable PM timing, stuck companion notes and unusable numeric controls. This supersedes the four-bus provisional scope. No release approval is implied.

## Authoritative behavior

- 16 steps = one 4/4 bar in 1/16ths at host transport tempo; single guitar VST3 MIDI event bus 0 (channel 1).
- On/off per step; articulation mutually exclusive: STOP, NOISE, PM, SOFT, OPEN/FIFTH, HARMONIC. OFF is not STOP. Clicking a selected active cell again disables that step.
- Guitar-generated pitch is ROOT NOTE (MIDI C4=60 convention) plus per-step relative degree in a fixed natural-minor default scale. Step degrees are directly editable in -13..15. No invented chromatic editor or key picker in this revision.
- DOUBLE = exactly two independent sequential attacks within the step; each NoteOn must receive its matching NoteOff. This does *not* synthesize extra powerchord voices; the target sample instrument implements its own fifth/palm articulation.
- OPEN/FIFTH may sustain until next active step (including STOP); PM/other staccato articulations use their step-bound gate. STOP flushes all currently active guitar notes at the exact step boundary. Transport stop/seek and state/parameter edits release sounding notes safely; no arbitrary retry storms.
- PM Velocity MIN/MAX: editable MIDI velocity range 1..127, ordered MIN<=MAX, default 30..40. **Library-specific values must be configured**; velocity maps differ between guitar instruments. Do not silently clamp all guitars to one manufacturer's articulation profile.
- ROOT NOTE direct text input, e.g. E2, G2, F#2; integer MIDI values also accepted. Gate and Fill editable as 0..100%, degree as -13..15; PM bounds as 1..127. Keyboard Enter commits, Escape cancels through VSTGUI text editing. Invalid strings reject; valid out-of-range numbers clamp.
- Preserve locked steps through Generate/Variate/Clear. Generate and Variate are controller-thread operations only.
- VST3 guitar parameter IDs stable. Processor serializes V10 guitar-only; V1..V9 project migration retains guitar settings and ignores obsolete companion state; no bass/chords/drums controls, MIDI buses or generated notes.

## Explicitly out of scope

Bass, Drums, separate Chords, independent EBM, 32-step editor, user-selectable clock subdivision, MIDI export, note-profile persistence, additional guitar sample maps and fully configurable key/scale selector. No unimplemented feature is presented as already shipped.

## Mandatory QA before release

- Automated test: chronological NoteOn/NoteOff parity for repeated PM, STOP, OPEN, DOUBLE, full loops, seek, host-rejected events, 44.1/48k and variable blocks.
- Parameter/UI: numeric edit, display, normalization, default reset, automation and recall; every step and articulation bidirectionally traced.
- Validated layout at 100%/150% with no nested clip; test both with Studio One chrome on 1920x1080.
- Save/restore V10 + old projects; malformed states handled atomically.
- Steinberg Validator and independent 125A Plugin Tester; actual Windows Studio One stop/start/loop regression and musical listening. Until then: **NOT VERIFIED IN DAW / NOT FINAL**.
