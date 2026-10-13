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

## Experimental Steinberg VST3 adapter
Source-only prototype at src/vst3: 647 parameter IDs (7 globals + 5×128 step controls), event in/out, VST3 component state translation and host PPQ/MIDI trigger runtime. First Windows x64 build is **NOT VERIFIED**. There is **no editor yet**; GUI and DAW QA remain blocking. Linux standalone 4/4 CTest cannot establish Windows VST3 binary PASS. Review docs/VST3-INTEGRATION.md and the experimental CI run before any further compliance claim.

Windows VST3 build #38391923331: COMPILE FAIL, C++ namespace collision (a125::riff::Event vs Steinberg::Vst::Event). SDK configure PASS. Fully-qualified event types staged for next Windows check. Not a release defect escape; this was detected before any host claim.

## 2026-10-13 Windows build QA
Windows MSVC successfully linked the native x64 VST3 in #38392237851, but the workflow failed when Release unit tests compiled with NDEBUG, suppressing assert-based tests and provoking /WX unused-variable diagnostics. The original 4/4 Linux Debug PASS did not validate Windows Release test assertions. Correction: all four CTest targets explicitly undefine NDEBUG at compile time; test files fail compilation if it remains defined. Retest Windows Release before claiming full workflow success.

## Native VST3 compilation evidence
At commit ba3207c, [Windows CI #38392621834](https://github.com/challanger2000/125A-Rifferator/actions/runs/38392621834) compiled and linked the actual x64 VST3 binary, ran 4/4 Release CTest with NDEBUG explicitly off, and published ZIP artifact #11809317427. **BUILDS PASS, WINDOWS CORE TEST PASS.** Host interaction, actual VST3 editor and Steinberg validator are NOT VERIFIED. The next source change adds Validator plus branded bundle packaging. No FINAL claim.

## Latest native VST3 verification (2026-10-13)

- Branch: rebuild/v1.0.0-clean; source commit: e4804275e26c0b320d2086763169c847b5a4581e.
- [Windows SDK/Validator CI #38393090916](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916): full workflow SUCCESS.
- Windows 2022 / MSVC Release: real Windows x64 VST3 compiled and linked: BUILDS PASS.
- Four independent Windows Release CTest targets: **4/4 PASS** (assertions kept enabled).
- Independent Steinberg VST3 Validator against staged, branded native bundle: **537 tests passed, 0 failed**.
- Verified experimental artifact: [Rifferator-V1-EXPERIMENTAL-Windows-x64](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916/artifacts/11810467004). Artifact ID 11810467004; ZIP 398192 bytes; SHA256 b7239140e59ccb9608b8863069c74888f83897f211838376b4db25408946789c.
- **NOT VERIFIED / NOT IMPLEMENTED:** VSTGUI editor/zoom/inputs, full Controller-to-GUI traceability, actual Studio One MIDI routing, real host automation/state recall, user musical acceptance, 125A Plugin Tester. No FINAL packaging or claim.

## Interactive VSTGUI source staging
One custom-drawn VSTGUI panel (1160×570), original logo vector resource, 16×5 step cells, SHIFT+left click diatonic note-picker overlay, BAR prev/next, global selectors, STEP ON/LOCK, GENERATE/VARIATE/CLEAR, EXPORT MIDI dialog and exact 100/150% VST3Editor zoom. All parameters flow through Controller beginEdit/setParamNormalized/performEdit/endEdit. SDK-free hitbox geometry tests added. THIS SOURCE IS NOT YET COMPILED OR STUDIO ONE VERIFIED; the first Windows VSTGUI build must settle integration and resource validation. The controller uses UI-only zoom state and host-independent pattern editing, not the old Rifferator implementation.

Windows GUI MSVC stage #38395611145: new VST3 GUI code compiled/linked, but UI geometry test /WX failed on C4459 variable shadowing. Corrected local parameter names without changing hitbox logic. Also added processor NoteOff cleanup when editing active step parameters, and an embedded UIDESC/SVG bundle audit. Full Windows CI result PENDING for these changes; do not count the failed workflow as PASS.

## First experimental GUI VST3 — Windows evidence (2026-10-13)

- Source and complete GUI: `rebuild/v1.0.0-clean`, SHA `39cd796682b83c2f25d221df15a86ea1541722a6`.
- [GitHub Actions Windows x64 #38396052442](https://github.com/challanger2000/125A-Rifferator/actions/runs/38396052442): **SUCCESS**. Actual VST3 compiled/linked in Release with VSTGUI 4.
- CTest Windows Release: **5/5 PASS**, including `rifferator_gui_geometry`; assertions kept active even under optimized build.
- SDK Steinberg Validator: **537 PASS, 0 FAIL**.
- Embedded package audit: verified original `125A_Logo_Master_FINAL.svg` byte-for-byte by SHA256 comparison with source, valid `rifferator.uidesc`, root template `RifferatorView` size 1160×570 and both custom views.
- GUI prototype includes: bar navigation, editable 16×5 matrix, STEP ON/LOCK, SHIFT+left-click diatonic picker, notes/scale labels, globals (KEY, SCALE, ROOT NOTE, BARS, PM VELOCITY, FILL, TRIGGER), GENERATE/VARIATE/CLEAR, controller-side Standard MIDI File save dialog and 100/150% zoom.
- [Experimental VST3 artifact #11811362702](https://github.com/challanger2000/125A-Rifferator/actions/runs/38396052442/artifacts/11811362702): ZIP 1,649,137 bytes; SHA256 `e662c5190a8f0029a3b6d7ae380f046cafa0fd6f416bd0f54243014a06eb02a7`.
- **Not yet verified**: native host attach/detach 20×, Studio One window rendering and hitbox alignment at 100/150%, realtime automation vs Editor parameters, actual H7 articulation sound/MIDI routing, project state recall, MIDI file import/save dialog, 125A Plugin Tester. This bundle is NOT FINAL.

## 2026-10-13 host probe expansion (PENDING CI)
The new workflow builds the pinned 125A-Plugin-Tester source revision `70aeb0d9` against the same Steinberg 3.8.1 SDK and executes fresh-instance state transfer, repeated module reload, I/O+MIDI-event stress and 4×5=20 native Win32 editor attach/detach cycles. These are host-harness tests, not actual Studio One tests, and success must only be stated after logs show PASS. The Controller now retains saved BAR selection independently of setState/setComponentState call order. Processor only releases sustained notes on an effective step change, not a redundant same-value parameter notification.

## Windows test-inventory correction, run #38406361914
The new 4/8-bar CTest target was registered in CMake and passed 6/6 Linux, but was omitted from the explicit Windows cmake --build target list. Therefore Windows CTest reported NOT RUN (missing .exe) after the VST3 and five other tests compiled/passed. This is a CI build-target inventory defect, not a Rifferator engine failure. The Windows workflow now builds all six CTest targets before running ctest. Required 125A Plugin Tester host probes still pending the corrected Windows run.

## Private QA dependency correction
CI #38406858006 compiled the current VST3 and passed all six Windows CTest tests; a separate 125A Plugin Tester checkout failed because the runner's scoped GitHub token does not have access to the private QA repository. This was an AUTHENTICATION/INFRASTRUCTURE failure, not a plugin test failure. No private 125A tester code has been copied to the public repository. A new original, small SDK-hosted harness is built locally to test fresh-instance processor/controller state and 20 repeated native HWND Editor attach/detach cycles; it is not equivalent to the full 125A Plugin Tester, which remains a separate QA gate.

## Host harness compile gate #38408632263
The full Windows VST3 compiled and all 6/6 Release regressions, GUI packaging checks and validator binary build passed. The newly added SDK-based host harness did not compile because its class-category constants lacked the SDK pluginfactory header (C2065). This was detected before any attach/detach testing; no editor lifecycle PASS was claimed. The missing include has been added; Windows workflow path filters now cover host harness source changes so regression updates do not silently evade CI.
