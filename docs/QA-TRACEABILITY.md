# V1 control/requirement traceability ledger

**Core implementation is not VST3 host acceptance.** Each required link UI tag -> input gesture -> callback -> parameter -> core/host event -> visual feedback -> state/automation must be verified. Missing chain = NOT VERIFIED or FAIL.

| User-visible requirement | Current path | Status |
| --- | --- | --- |
| 1/2/4/8 bars | Pattern::setBars; grid test | CORE TESTED |
| 16 steps, one BAR, BAR navigation | 16-per-bar model, static GUI contract | BAR GUI NOT IMPLEMENTED |
| STEP ON, LOCK and CLEAR PATTERN | Pattern::setStep/setLock/clear | CORE TESTED; UI NOT IMPLEMENTED |
| PINCH / OPEN / PM / NOISE / STOP | velocity and compileOneCycle | CORE TESTED, H7-specific Velocity policy; actual Heavier7Strings/GUI NOT VERIFIED |
| Empty step sustains; next note or STOP releases | compileOneCycle | CORE TESTED |
| KEY / SCALE / ROOT NOTE, scale notes always visible | Settings, pitchForDegree | CORE TESTED; GUI NOT IMPLEMENTED |
| SHIFT+left-click note chooser | static UI design | NOT IMPLEMENTED |
| PM VELOCITY | Settings/velocity | H7 PM velocity policy TEST PASS; UI/automation NOT IMPLEMENTED |
| FILL 0..100 changes only pitch | applyFill | CORE TESTED; UI NOT IMPLEMENTED |
| GENERATE and VARIATE | generate/variate | CORE IMPLEMENTED; broader musical QA pending |
| PPQ absolute transport clock | visitAbsoluteEvents / MidiRuntime absolute grid | CORE REGRESSION PASS (CI #38389103512); VST3 adapter NOT VERIFIED |
| MIDI NOTE trigger NoteOn/NoteOff | MidiRuntime::process, TriggerEvent | CORE RUNTIME IMPLEMENTED; VST3 binding NOT VERIFIED |
| EXPORT MIDI Standard MIDI File | exportMidi | INDEPENDENT SMF FORMAT-0 PARSER TEST PASS (CI #38389103512); DAW import NOT VERIFIED |
| EXPORT MIDI Save dialog | none | NOT IMPLEMENTED |
| 100/150% zoom | proven High Gain Guitar Finisher source inspected | NOT IMPLEMENTED |
| VST3 buses, state, automation, controller lifecycle | none | NOT IMPLEMENTED |
| Steinberg Validator, 125A Tester, Studio One | none | NOT VERIFIED |

## Mandatory remaining gates
- Static GUI artwork before VSTGUI; complete per-cell geometry + click feedback.
- One VST3 output bus; sample-accurate host scheduling with seek/loop/stop/retrigger/note rejection safety. Never allocate or do file I/O in callback.
- VST3 parameter/state migration, save/reopen, host automation and project recall.
- Export SMF imported/played in Studio One; independent SMF parse and note parity.
- Native VST3 validation, Steinberg Validator, 125A Plugin Tester, 20 GUI attach/reopen cycles, zoom actual Studio One geometry and manual musical verification.
- FINAL remains **BLOCKED** until all applicable gates and Studio One acceptance succeed.

Source separation checked: Pattern and MidiRuntime do not name H7 performance techniques; the H7 adapter declares the named mapping and PM velocity policy. Independent non-H7 technique-policy regression PASS in CI #38388570151. This does not certify the eventual EBM bass product.

Additional independent Standard MIDI File decoder regression (header, VLQ timing, NoteOff-first order, 8-bar boundary and malformed pattern rejection) PASS in CI #38388570151.

Product-state SDK-free V1 codec: fixed 408-byte endian-stable snapshot includes all 128 cells, bars, key/scale/root, FILL, H7 PM velocity, MIDI trigger mode and seed. Reject unsupported version, CRC/FNV mismatch, malformed values and truncated data *atomically*. Malformed-state regression PASS in CI #38389103512. Controller-only BAR selection and zoom state remain separate and not yet implemented.

## Verified automated evidence (2026-10-13)

- **Exact source:** `challanger2000/125A-Rifferator`, `rebuild/v1.0.0-clean`, commit `630c2034a349b4ccdcaa19d5d96a0a511cbe0da6`.
- **Core CTest:** 4/4 PASS, Linux Ubuntu 24.04, GCC 13.3, C++17 Debug, -Wall/-Wextra/-Wpedantic/-Werror. [CI #38389103512](https://github.com/challanger2000/125A-Rifferator/actions/runs/38389103512).
- **Subtests:** `rifferator_core_contract`, `rifferator_midi_runtime`, `rifferator_midi_file_format0`, `rifferator_state_roundtrip`.
- **Evidence limitation:** This is source-level C++ compliance only. No VST3 processor/controller, no Windows native plugin, no Steinberg Validator result, no Editor Lifecycle, no 125A Plugin Tester, no Studio One acceptance. Static SVG is not a functional GUI.
- **State format:** New V1 wire contract intentionally does not attempt to migrate unreleased/abandoned previous Rifferator V1..V11 implementations. Old source preserved in the historic branch.
