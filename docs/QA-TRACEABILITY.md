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
| PM VELOCITY | Settings/velocity | CORE IMPLEMENTED; UI/automation NOT IMPLEMENTED |
| FILL 0..100 changes only pitch | applyFill | CORE TESTED; UI NOT IMPLEMENTED |
| GENERATE and VARIATE | generate/variate | CORE IMPLEMENTED; broader musical QA pending |
| PPQ absolute transport clock | visitAbsoluteEvents / MidiRuntime absolute grid | CORE IMPLEMENTED, new realtime regressions CI PENDING; VST3 adapter NOT VERIFIED |
| MIDI NOTE trigger NoteOn/NoteOff | MidiRuntime::process, TriggerEvent | CORE RUNTIME IMPLEMENTED; VST3 binding NOT VERIFIED |
| EXPORT MIDI Standard MIDI File | exportMidi | HEADER/END-TRACK CORE TESTED; independent SMF reader pending |
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

Source separation checked: Pattern and MidiRuntime do not name H7 performance techniques; the H7 adapter declares the named mapping and PM velocity policy. Regression with arbitrary non-H7 technique code will run in the next core CI. This does not certify the eventual EBM bass product.

Additional independent Standard MIDI File decoder regression (header, VLQ timing, NoteOff-first order, 8-bar boundary and malformed pattern rejection) staged for the next core CI.

Product-state SDK-free V1 codec: fixed 408-byte endian-stable snapshot includes all 128 cells, bars, key/scale/root, FILL, H7 PM velocity, MIDI trigger mode and seed. Reject unsupported version, CRC/FNV mismatch, malformed values and truncated data *atomically*. Tests pending next core CI. Controller-only BAR selection and zoom state remain separate and not yet implemented.
