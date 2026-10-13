# 125A Rifferator V1 — CLEAN REBUILD

**Current development branch:** rebuild/v1.0.0-clean. **Current status:** working experimental Windows x64 MIDI-only VST3 with VSTGUI, complete technical 125A Plugin Tester PASS (2026-10-13), final Studio One GUI/musical acceptance still open. **NOT FINAL.** Earlier prototype stages below are historical.

This codebase starts from the original minimal main reference ccc92dac (not old Rifferator implementation). The discontinued develop/v1.0.0 and separate feature/ebm-bass-engine branches are preserved unmodified. Do not copy obsolete GUI, transport, state or controller code from those branches.

User-approved scope: docs/PRODUCT-SPEC-V1.md; static-first GUI layout: docs/STATIC-GUI-V1.md; requirement-by-requirement QA: docs/QA-TRACEABILITY.md.

The existing code provides a generic 1/2/4/8 bar MIDI pattern model, velocity articulations for Heavier7Strings, diatonic pitch logic, note release/STOP rules, deterministic FILL/Generate/Variate and an SMF0 *byte encoder*. No VST3 Processor/Controller, editor, actual file-save dialog, Studio One support or FINAL claim is made at this stage.

Local core verification (does not incur GitHub Actions):
- cmake -S . -B build
- cmake --build build
- ctest --test-dir build --output-on-failure

Mandatory general standards in 125A-Engineering START-HERE / ENGINEERING-GUARDRAILS; Branding master + Knob Designer for future GUI. Primary H7 velocity evidence: https://www.search-manual.com/upload/three-body-technology-heavier-7-strings-metal-guitar-virtual-instrument-plug-in-309614-user-manual.pdf §§6.1/6.4/6.5.

2026-10-13: A separate allocation-free, host-neutral MidiRuntime.hpp and MIDI trigger transport regression suite have been introduced. Initial host-neutral core CI is now green (see the exact run below); this does not establish DAW functionality. No VST3 binary exists yet.

**Core/instrument split:** Pattern technique codes are opaque in the generic timing layer. Heavier7Strings.hpp owns the technique labels and velocity profile; tests exercise an independent synthetic bass velocity mapping.

A fixed-size V1 processor state codec now stores all pattern cells and musical settings with atomic validation; this is still SDK-free and its malformed-state regression has passed isolated CTest.

## Current source-level QA

**GitHub Actions [#38389103512](https://github.com/challanger2000/125A-Rifferator/actions/runs/38389103512): 4/4 CTest PASS** at commit `630c2034`. This measures the standalone MIDI core, runtime, standard-MIDI-file format and fixed-size state codec only; no VST3, Windows or GUI status is implied. Release remains blocked until genuine Studio One/125A/Steinberg QA.

**Windows host-adapter source:** Windows x64 VST3 binary successfully compiled/link-checked and 4/4 CTest passed in [GitHub run #38392621834](https://github.com/challanger2000/125A-Rifferator/actions/runs/38392621834). Its experimental downloadable bundle is NOT FINAL and has no VSTGUI editor. Any later build/Validator claims must cite their own exact commit and run.

## Latest native VST3 verification (2026-10-13)

- Branch: rebuild/v1.0.0-clean; source commit: e4804275e26c0b320d2086763169c847b5a4581e.
- [Windows SDK/Validator CI #38393090916](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916): full workflow SUCCESS.
- Windows 2022 / MSVC Release: real Windows x64 VST3 compiled and linked: BUILDS PASS.
- Four independent Windows Release CTest targets: **4/4 PASS** (assertions kept enabled).
- Independent Steinberg VST3 Validator against staged, branded native bundle: **537 tests passed, 0 failed**.
- Verified experimental artifact: [Rifferator-V1-EXPERIMENTAL-Windows-x64](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916/artifacts/11810467004). Artifact ID 11810467004; ZIP 398192 bytes; SHA256 b7239140e59ccb9608b8863069c74888f83897f211838376b4db25408946789c.
- **NOT VERIFIED / NOT IMPLEMENTED:** VSTGUI editor/zoom/inputs, full Controller-to-GUI traceability, actual Studio One MIDI routing, real host automation/state recall, user musical acceptance, 125A Plugin Tester. No FINAL packaging or claim.

Experimental first VSTGUI editor source added on the clean branch. It is NOT host accepted yet; inspect the Windows GUI build/validator result before using any artifact. Studio One manual controls and zoom acceptance remain mandatory.

## First experimental GUI VST3 — Windows evidence (2026-10-13)

- Source and complete GUI: `rebuild/v1.0.0-clean`, SHA `39cd796682b83c2f25d221df15a86ea1541722a6`.
- [GitHub Actions Windows x64 #38396052442](https://github.com/challanger2000/125A-Rifferator/actions/runs/38396052442): **SUCCESS**. Actual VST3 compiled/linked in Release with VSTGUI 4.
- CTest Windows Release: **5/5 PASS**, including `rifferator_gui_geometry`; assertions kept active even under optimized build.
- SDK Steinberg Validator: **537 PASS, 0 FAIL**.
- Embedded package audit: verified original `125A_Logo_Master_FINAL.svg` byte-for-byte by SHA256 comparison with source, valid `rifferator.uidesc`, root template `RifferatorView` size 1160×570 and both custom views.
- GUI prototype includes: bar navigation, editable 16×5 matrix, STEP ON/LOCK, SHIFT+left-click diatonic picker, notes/scale labels, globals (KEY, SCALE, ROOT NOTE, BARS, PM VELOCITY, FILL, TRIGGER), GENERATE/VARIATE/CLEAR, controller-side Standard MIDI File save dialog and 100/150% zoom.
- [Experimental VST3 artifact #11811362702](https://github.com/challanger2000/125A-Rifferator/actions/runs/38396052442/artifacts/11811362702): ZIP 1,649,137 bytes; SHA256 `e662c5190a8f0029a3b6d7ae380f046cafa0fd6f416bd0f54243014a06eb02a7`.
- **Not yet verified**: native host attach/detach 20×, Studio One window rendering and hitbox alignment at 100/150%, realtime automation vs Editor parameters, actual H7 articulation sound/MIDI routing, project state recall, MIDI file import/save dialog, 125A Plugin Tester. This bundle is NOT FINAL.

## Current Windows V1 status (2026-10-13)
This branch now contains a Windows x64 VST3 MIDI instrument and VSTGUI. Earlier "core-only/no VST3" descriptions above are historical. [Windows QA #38411585537](https://github.com/challanger2000/125A-Rifferator/actions/runs/38411585537) on `f871b829` reported 6/6 Windows CTest, 20/20 Win32 editor attach/detach, fresh-instance state transfer, 537/537 Steinberg Validator and branded GUI bundle PASS. The user tested MIDI output directly in Studio One/Heavier7Strings and observed rhythmic attacks, transport STOP, 2 distinct bars, locked generation/clear, scale note edits and SMF export/import. The picker/Root fixes subsequently passed Windows CI [#38414680086](https://github.com/challanger2000/125A-Rifferator/actions/runs/38414680086) on `e84349ba`: 6/6 Release CTest, 20/20 HWND editor cycles, actual non-default eight-bar snapshot/LOCK/PM/trigger and 150% zoom/BAR private state restoration, bundled resources, 537/537 Steinberg Validator. Full 125A Plugin Tester and Studio One save/reopen and automation are still pending. **NOT FINAL.**

## Native VST3 MIDI/automation QA — 2026-10-13

[Windows run #38417386319](https://github.com/challanger2000/125A-Rifferator/actions/runs/38417386319) at source `74a28db8`: successful VST3 build, 6/6 Windows CTest, 20/20 editor lifecycle, non-default eight-bar state, 537/537 validator. A new independent SDK-hosted ProcessData test verified actual MIDI event and parameter automation: PM/OPEN velocities, STOP, transport stop, KEY/ROOT transposition and recall, MIDI NoteOn/NoteOff trigger, and no redundant retrigger on unchanged step edits. Latest [EXPERIMENTAL package](https://github.com/challanger2000/125A-Rifferator/actions/runs/38417386319/artifacts/11824563636). **NOT FINAL:** real Studio One save/reopen and automation tests plus full private 125A Plugin Tester remain separate gates.

## Latest release-audit evidence — 2026-10-13
Source `9c1a1629`: [Windows CI #38424202886](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886) **SUCCESS** — 6/6 CTest, 20/20 native editor cycles, 537/537 Steinberg Validator, RGU1/RGU2 Controller state migration and persisted generator seed, locked-step VST3 automation readback, sample-accurate event bounds and 44.1/96 kHz realtime/offline varying-block host stress with balanced MIDI NoteOn/Off. [Experimental Windows x64 artifact](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886/artifacts/11827572883). The standalone private 125A Plugin Tester and final Studio One save/reload/control QA remain outstanding. **NOT FINAL**.

## Direct header controls — 2026-10-13
In response to Studio One usability feedback (KEY required up to 12 clicks; ROOT trapped at G9), build `4b498566` replaces repeated clicks with VSTGUI choice menus for KEY, SCALE, ROOT NOTE (in-scale pitches by octave, reset tonic in octave 2), and PATTERN (1/2/4/8 bars); PM VELOCITY 30–83 and FILL 0–100% have absolute horizontal click-to-value strips. Ctrl+left-click restores documented header defaults (KEY E / ROOT E2 at factory configuration). [Windows Actions #38432228284](https://github.com/challanger2000/125A-Rifferator/actions/runs/38432228284) passed 6/6 CTest, 20/20 native HWND editor lifecycle, 537/537 Steinberg Validator and all MIDI host stress checks; [EXPERIMENTAL Windows package](https://github.com/challanger2000/125A-Rifferator/actions/runs/38432228284/artifacts/11830684020). Native menu opening/selection and 100/150% pointer alignment in Studio One still await actual user verification; **NOT FINAL**.

## Official 125A Plugin Tester result — latest tested code, 2026-10-13
The original portable tester v0.2.9 has now independently tested source `8b70827f` in its own private GitHub Actions workspace: **30 PASS / 0 WARNING / 0 FAIL, technical RELEASE: PASS**. Confirmed VST3 I/O, Controller/Processor state, reload and Editor lifecycle, dense MIDI events, all 50 rate/block setups and Steinberg validation. [Run #38436972585](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585) · [five reports](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585/artifacts/11833722284). Windows Core/GUI/SDK regression [#38435504929](https://github.com/challanger2000/125A-Rifferator/actions/runs/38435504929) also passed 6/6 CTest and 20/20 Editor cycles. The new STOP toggling and Ctrl-click policy are source-level / automated-policy verified; **real current-build Studio One clicks**, menu closure/selection, DAW save/reload and complete Heavier7Strings articulation review remain **NOT VERIFIED**. Experimental [Windows build #11832697108](https://github.com/challanger2000/125A-Rifferator/actions/runs/38435504929/artifacts/11832697108) is **NOT FINAL**.

## GUI usability build bef805d — verified technical QA 2026-10-13
User screenshot revealed barely usable PM/FILL numeric controls, uncancellable Shift note picker, misplaced help label and TRANSPORT/MIDI NOTE at lower right. Source `bef805d26789c7974285f1c807ce8168280645d7` has click-drag 12px slider track/thumb with real beginEdit/performEdit/endEdit and wheel one-integer steps, picker X/Escape/outside cancellation without note changes, status-footer hint and transport mode in upper BAR toolbar. [Windows #38441617503](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503) **SUCCESS**: 6/6 Windows CTests, 20/20 Editor attach/detach, Steinberg 537/537, sample-rate stress; [125A Tester #38442103633](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38442103633) **SUCCESS**: original 125A Plugin Tester v0.2.9 **30 PASS / 0 WARN / 0 FAIL** on exactly this source. [Windows VST3 artifact #11836576122](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503/artifacts/11836576122); [125A full reports #11837405316](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38442103633/artifacts/11837405316). **NOT FINAL** until current Studio One manual mouse/drag/wheel/ESC/zoom and save/reopen acceptance. The V1 music scale engine still has only Major and Natural Minor; adding modes requires separate complete pitch/State/Generate/Variate tests.
