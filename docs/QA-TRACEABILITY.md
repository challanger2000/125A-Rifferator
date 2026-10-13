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

## Host test-harness correction, CI #38409370363
Windows VST3 binary and six CTest tests passed, and Steinberg Validator compiled. The original host-side test did not compile because SDK factory category names were inadvertently used as undeclared C++ constants in the standalone harness. Product identity is now selected using the exact registered class names `125A Rifferator` / `125A Rifferator Controller`, instead of relying on a preprocessor factory-registration macro. The native host probe still requires a successful compile and actual 20-cycle run before PASS.

## 2026-10-13 follow-up changes (PENDING CI)
Real-host picker had obstructed STOP. It now replaces the right detail panel, preserving every STOP hitbox; CTest checks all 15 picker cells and all 16 STOP cells with modal picker active. Controller publish now validates/quantizes before emitting edit gestures and sends a paired ROOT parameter edit when GUI KEY/SCALE changes the root. The Controller readback also mirrors snapped ROOT for incoming host changes without generating host feedback edits. Exact Studio One automation and project recall still NOT VERIFIED.

## 2026-10-13 verified follow-up Windows QA — #38414680086

- Source: commit `e84349ba5738d5e66a39aebe042fbbcc72941752` on `rebuild/v1.0.0-clean`.
- [GitHub Actions](https://github.com/challanger2000/125A-Rifferator/actions/runs/38414680086): **SUCCESS** (Windows 2022 x64 native VST3, resources and branding confirmed).
- All six Windows Release CTest regressions: **6/6 PASS**, including 4/8-bar timeline and the picker layout regression for all 15 choices and all 16 unoccluded STOP cells.
- Original VST3 SDK host test: **PASS** non-default 8-bar Processor snapshot (including LOCK in a late bar, PM velocity and MIDI-note trigger mode), Controller normalized readback, saved 150% zoom/BAR 8 controller state, fresh-instance restore; **20/20 native Windows editor attach/detach PASS**.
- Steinberg VST3 Validator: **537 passed, 0 failed**.
- Experimental Windows x64 bundle: [artifact #11824790128](https://github.com/challanger2000/125A-Rifferator/actions/runs/38414680086/artifacts/11824790128), not a release-ready package.
- **Not verified:** changed picker appearance, Studio One automation and save/reload, standalone 125A Plugin Tester I/O/event torture, complete H7 articulation audition and precise 4/8-bar DAW playback. No FINAL gate promotion.

## 2026-10-13 native VST3 event/automation extension (PENDING WINDOWS QA)
A host-driven QA fixture was added to the existing native Windows lifecycle probe, invoking actual IAudioProcessor::process with VST3 EventList, ParameterChanges and ProcessContext. It checks eight-bar saved-state bus topology, PM velocity 65, OPEN 115, STOP releases only, transport STOP note release, KEY automation E→F# and its persisted Root, unchanged STEP ON automation not retriggering, and MIDI NOTE trigger NoteOn(100)/NoteOff(200) with correct emitted sample offsets and release. This is a stronger host-interface probe than core tests but does not substitute for the full 125A Plugin Tester or real Studio One automation checks. No PASS claimed before workflow result.

## Host event-probe precondition correction — #38416726085
The VST3 successfully compiled; all six Windows CTests passed. The new native event probe failed at the first expected transport attack, because it reused the state-recall fixture whose explicitly saved `syncMode=MidiNote` was correct for recall QA but **not** transport QA. This is a host-test fixture mismatch, not evidence of a plugin defect. The test now submits a real Mode=TRANSPORT VST3 parameter queue before expecting PPQ-driven attacks, then separately switches to Mode=MIDI NOTE later. Mode is deliberately changed through the same host automation API under test. The rest of the event/automation suite and validator remain NOT VERIFIED pending the corrected run.

## Native host MIDI + automation verification (2026-10-13)

Source commit `74a28db8bd7b6348d6a63a6008f9da2cc7db3bdf`; [Windows run #38417386319](https://github.com/challanger2000/125A-Rifferator/actions/runs/38417386319): **SUCCESS**.

- Original Windows SDK host probe: **PASS** — actual VST3 ProcessData event output, PM velocity 65, OPEN velocity 115, STOP note release, transport STOP note release, KEY automation E minor→F# minor with correct emitted MIDI pitch and saved ROOT, unchanged STEP ON automation without spurious MIDI reattack, and MIDI NOTE mode NoteOn (offset 100) / NoteOff (offset 200).
- Initial event-probe failure at #38416726085 was a **test setup fault**: previous non-default project state intentionally had MIDI NOTE selected. Test now explicitly automates TRANSPORT before PPQ tests and MIDI NOTE before trigger tests; both pass.
- Non-default eight-bar Processor/Controller state and 150% UI/BAR restore: **PASS**.
- Windows CTest **6/6 PASS**, editor **20/20 PASS**, Steinberg Validator **537/537 PASS**.
- [Experimental Windows build artifact #11824563636](https://github.com/challanger2000/125A-Rifferator/actions/runs/38417386319/artifacts/11824563636).
- Real Studio One project-save/reload, full 125A Plugin Tester I/O and event torture, all H7 articulation audition and precise 4/8-bar Studio One playback are **NOT YET VERIFIED**. This is not FINAL.

## Release-audit patch pending — locked automation & deterministic regeneration
Inspected and reproduced by source trace: `Controller::setParamNormalized` accepted host values into the VST3 parameter cache before the pattern LOCK invariant could reject them; the displayed parameter value could therefore disagree with actual MIDI/GUI. Rejected edits now revert the host parameter cache to the pattern's authoritative normalized value; accepted but snapped ROOT values are likewise synchronized. A native Win32 VST3 host regression verifies locked ON/DEGREE host readback and invalid ROOT snap. The 12-byte UI state (RGU1) previously saved zoom+bar but omitted Controller's `seed_` counter used by Generate/Variate; the 16-byte RGU2 now adds seed with backward-compatible RGU1 read. Native host tests cover RGU1 migration, RGU2 full roundtrip and atomic truncated-state rejection. No new public parameter IDs and no Processor state format change. Verified in Windows run #38424202886 (success).

### Extended host timeline stress (pending verification)
Added native Windows SDK-hosted stress loops for 44.1/96 kHz, realtime and offline process modes, irregular 1/31/127/257/512 sample blocks, explicit pause/stop event flush, per-block event-offset bounds and exact NoteOn/NoteOff balance. Results must be checked in actual Windows Actions logs; this is not a replacement for 125A Plugin Tester, DAW save/reopen or MIDI audio perception.

## Windows host stress and State V2 verification — 2026-10-13
- Commit `9c1a1629ce59f18f0a4ab2afd591aa402d22d974`; [Windows Actions #38424202886](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886): **SUCCESS**.
- CTest Release **6/6 PASS**; Steinberg Validator **537/537 PASS**; native HWND Editor **20/20 PASS**.
- SDK-hosted VST3 check: **PASS** RGU1 (legacy 12-byte) → RGU2 migration, 16-byte zoom/BAR/GENERATOR-seed state roundtrip and atomic truncated-state rejection; Processor 8-bar and LOCK persistence; invalid root snap and rejected locked ON/DEGREE host cache readback.
- Host event stress: **PASS** 44,100 Hz realtime (299 blocks, 3 NoteOn/3 NoteOff), 44,100 Hz offline (299; 3/3), 96,000 Hz realtime (649; 3/3), 96,000 Hz offline (649; 3/3). Each with 1/31/127/257/512 sample blocks, sample offsets within each block, no orphan MIDI notes and transport STOP release.
- VST3 MIDI routing and automation already verified in earlier host and user tests; [Windows experimental artifact #11827572883](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886/artifacts/11827572883).
- **Unverified release gates:** actual Studio One DAW save/reopen after installing the new RGU2 build, real host control-level automation including Ctrl+left-click defaults, full 125A Plugin Tester (private repo inaccessible to this repository's scoped Actions token), 4/8-bar musical host acceptance and full Heavier7Strings articulation listening; NOT FINAL.

## Direct header UX acceptance, build 4b498566 (2026-10-13)
- Problem: ROOT NOTE increment-only became stuck at G9 (MIDI 127), KEY needed 12 successive clicks, other selectors were similarly inefficient.
- Implementation: four native VSTGUI selection menus KEY/SCALE/BARS/ROOT, ROOT grouped in octave submenus with a direct tonic octave-2 reset; two click-to-value strips for PM VELOCITY/FILL; Ctrl+left-click defined defaults. Parameter IDs and Processor state unchanged.
- Automated verification: [Windows CI #38432228284](https://github.com/challanger2000/125A-Rifferator/actions/runs/38432228284) SUCCESS; CTest 6/6 including numeric endpoint/monotonic mapping and MIDI 127 → 40 ROOT reset, native Editor 20/20, Steinberg Validator 537/537, 44.1/96 kHz realtime/offline host event stress PASS. Experimental artifact #11830684020.
- Gate still open: real Studio One native menus, choosing a note inside a submenu, menu cancellation and 100/150% mouse coordinate alignment; do not mark these PASS until user confirms. Studio One save/reopen and private 125A Plugin Tester remain separate gates.

## Per-element GUI audit expansion (2026-10-13, CI PENDING)
Found through end-to-end source trace: STOP in the 5th grid row lacked a toggle-off path, and the top-level Control modifier handler consumed every Ctrl+click on controls that lacked reset behavior. Implementation now uses a single testable policy for all five grid row transitions, neutral matrix reset, and a complete reset applicability classification. Tests now cover 4 bar sizes × 5 row types × every active step up to 128, locked-note rejection, ctrl-click reset vs command fallthrough, and H7 row-to-MIDI velocities. Still pending native host CI, physical 100/150% Studio One pointer testing, popup menus, file-save dialog, DAW recall and the full 125A Plugin Tester; these remain explicitly NOT VERIFIED.

## Official 125A Plugin Tester gate (2026-10-13) — VERIFIED PASS
- The separate private `challanger2000/125A-Plugin-Tester` workflow built the **exact** public Rifferator source commit `8b70827fe1751d2f7deba92cabdeeb43b7203f7a` with pinned SDK 3.8.1, staged the complete Windows x64 bundle, and ran the **original portable 125A Plugin Tester version 0.2.9**, without copying tester source into the Rifferator repository.
- [Private workflow #38436972585](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585) **SUCCESS**. Original report: `RESULT: 30 PASS / 0 WARNING / 0 FAIL`, `RELEASE: PASS` (tester-specific technical classification).
- Explicit report confirms: 647 valid VST3 parameters, 3235 normalized/plain conversion checks; event input (16 channels), event output (1 channel), no audio buses; bidirectional component/controller connection; 408-byte Processor snapshot and 16-byte Controller snapshot save/restore; 5 complete module reloads; 5 editor lifecycle cycles; fresh-instance state; sidechain/bus/event probe; Steinberg validator PASS; **50/50** sample-rate/block-size matrix across 44.1, 48, 88.2, 96 and 192 kHz. Audio-output torture **SKIPPED as not applicable** for MIDI-only component. Five [original QA reports](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585/artifacts/11833722284).
- This clears the formerly unverified 125A Plugin Tester gate. **Does not demonstrate actual Studio One GUI mouse event success** on the latest popup/reset/STOP modifications, DAW session reload or final real Heavier7Strings musical acceptance. Overall Rifferator is **NOT FINAL**.

## GUI controls — full interaction repair, screenshot 2026-10-13 (PENDING)
User specifically reported PM Velocity/FILL intensity hard to set, confusing note picker exit, misplaced SHIFT help, and TRANSPORT/MIDI NOTE cramped in right-hand inspector. Implemented VSTGUI gesture-aware sliders with 12px visible track and thumb, explicit beginEdit/performEdit/endEdit on drag, ±1 mouse wheel precision, reversible Esc/X/outside-click note picker, shifted transport mode to BAR toolbar, and repositioned footer hint. All 54 PM and 101 FILL integer positions are reachable, including both end stops; hit-test assertions prove modal X/STOP and new MODE location. This is a new code change and is NOT yet a proven Windows or real DAW PASS.

## 2026-10-13 VSTGUI Escape adapter compile fix
Windows run [#38441202974](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441202974) compiled the new Rifferator controls until the VSTGUI adapter; MSVC reported C2039 because `VstKeyCode` and `VKEY_ESCAPE` are global namespace SDK symbols, not `VSTGUI::` members. The pure CTest/core regression already passed on Linux. Corrected both identifier namespaces to SDK v3.8.1 canonical definitions; Windows VST3 build and all native probes must rerun. This is a test gate failure, not evidence of GUI acceptance or release readiness.

## 2026-10-13 screenshot-reported GUI repair — FINAL TECHNICAL TEST RESULTS (NOT DAW PASS)
- Tested exact plugin source `bef805d26789c7974285f1c807ce8168280645d7`; [Windows #38441617503](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503) **SUCCESS** (6/6 Windows CTest including all 54 PM and 101 FILL integer values + relocated transport and picker X geometry; 20 Editor cycles; 537 Steinberg Validator tests; 44.1/96 kHz host event stress).
- The first VSTGUI Escape handler version failed the Windows compile #38441202974 because SDK `VstKeyCode` is a global type, not a `VSTGUI::` type. The defect was corrected before this successful build. Never count the failed attempt as PASS.
- Original tester again executed on the same latest plugin source from its private repository: [125A Tester #38442103633](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38442103633), **30 PASS / 0 WARNING / 0 FAIL; RELEASE: PASS** for technical tester; 50/50 samplerate/block setups, 5 editor cycles, I/O+Event, state, reload, official validator. [125A full reports #11837405316](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38442103633/artifacts/11837405316).
- Experimental VST3 to test in Studio One: [Windows VST3 artifact #11836576122](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503/artifacts/11836576122). Real host pointer drag and wheel, modal X/Escape/outside (including 150% zoom), relocated TRANSPORT hit location and visual balance are **not verified by automatic lifecycle/validator**. Complete Studio One project save/reopen and musical H7 acceptance also pending; no overall FINAL claim.

## 2026-10-13 live-edit interruption / inaudible VARIATE / header size (PENDING CI)
- Studio One observation: while `MIDI NOTE` held a riff playing, every STEP add/remove forced a full stop and waited for a new external NoteOn. Root cause in Processor: any meaningful step edit set `cleanupPending_`, which temporarily forced `MidiRuntime` to TRANSPORT for panic release; this erased held trigger and phase. Fixed: no implicit mode switch or cleanup for STEP/LOCK/BARS/PM/FILL updates. KEY/ROOT/SCALE changes release only the currently sounding MIDI voice without dropping trigger ownership or resetting musical phase; actual mode changes, stop/seek and setState retain explicit cleanup. Native SDK-host regression emulates actual held MIDI NoteOn then edits TYPE+1, inserts ON+3, deletes ON+1 while running, asserts no extra releases and exact subsequent sample boundary notes at 6000/12000/18000 samples, then correct original matching NoteOff release.
- VARIATE: former only 15% probability of changing existing articulations to PM and had zero effect for all-PM riffs, especially with FILL=0. Now always changes 1..3 unlocked audible PM↔OPEN accents, or inserts one note into an otherwise empty editable pattern. If no unlocked playable/empty steps, GUI reports exactly that instead of falsely claiming a variation. Core tests compare *actual emitted MIDI pitch/velocity/timing* across 1/2/4/8 bars and 100 seeds at FILL=0, preserve locks and deterministic seed behavior.
- GUI: RIFFERATOR enlarged with native big font and centered on x=580 in 1160-wide header. Subtext now V1.0.0 / MIDI RIFF GENERATOR (Heavier7Strings product mention removed), and ZOOM button reduced to 88×30 at (1036,34). Hitbox regression checks revised zoom bounds and no activation outside.
- **Still NOT VERIFIED** on new commit: Windows build, Steinberg / 125A Plugin Tester, actual Studio One live MIDI continuous playback and subjective VARIATE change. Not FINAL.

## Verified 2026-10-13 live editing / VARIATE / centered header on exact build d6440d20
- [Windows VST3 QA #38444977246](https://github.com/challanger2000/125A-Rifferator/actions/runs/38444977246), **SUCCESS** on [source d6440d20](https://github.com/challanger2000/125A-Rifferator/commit/d6440d205f440d8fc6c8ac01fde970895241638e). 6/6 Windows CTest; core VARIATE at 0% FILL across 1/2/4/8 bars and 100 seeds tested for actual output velocity/timing difference; deterministic seeding/LOCK/empty cases; **20/20** Windows editor attach/detach; Steinberg **537/537**.
- New original SDK-host VST3 regression **PASS**: one held MIDI NoteOn, changed TYPE+1, inserted ON+3, deleted ON+1 while still sounding, with events at exact absolute positions 6000 (release and new attack), 12000 (STOP release), 18000 (new attack); original matching NoteOff properly released the sounding voice. This confirms no forced transport reset on STEP edits at the VST3 host interface. In addition, 44.1/96 kHz realtime/offline variable-block event stress passed 3/3 attacks/releases each.
- [Original 125A Tester #38445480681](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38445480681), original 125A Plugin Tester v0.2.9 built identical source and reported **RESULT 30 PASS / 0 WARNING / 0 FAIL; RELEASE: PASS** within that **technical tester only**, across 50/50 rate-block setups plus real lifecycle, state, MIDI/event, editor, I/O and Steinberg. [Official 125A report #11838451648](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38445480681/artifacts/11838451648).
- [Windows x64 test bundle #11838530699](https://github.com/challanger2000/125A-Rifferator/actions/runs/38444977246/artifacts/11838530699) is the newest experimentally staged Windows artifact. **Studio One user acceptance still required:** sustained MIDI-NOTE loop with step insert/delete and no fresh trigger, audible VARIATE at FILL 0, header center/font/button layout at 100/150%, project resave/reopen and true musical value with H7. Do not label overall product FINAL before these gates.
