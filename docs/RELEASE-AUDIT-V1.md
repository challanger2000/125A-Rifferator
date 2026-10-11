# 125A Rifferator V1.0.0 — Release-level audit

**Decision: BLOCKED / NOT FINAL.** This is a source-and-CI evidence ledger, not an assertion of a completed Studio One host test.

## Baseline
- Repository: `challanger2000/125A-Rifferator`, branch `develop/v1.0.0`.
- Most recent validated pre-audit binary: workflow #38165400049, 29/29 CTest PASS, Steinberg validator 537/537 PASS.
- Windows delivery bundle renamed `125A Rifferator.vst3`; matching internal binary and packaged `rifferator.uidesc` verified by CI.
- User reports the intended editor **does not appear in Studio One**, including after #38164929473. The host GUI defect remains OPEN, irrespective of SDK validator pass.
- The Steinberg/VSTGUI editor differs from the working 125A Midiator, which uses `AspectRatioVST3Editor`. A change to that editor was compiled in #38165400049 but **not established as a host fix**.

## Confirmed release blockers and inconsistencies
1. **HOST-GUI — FAIL / blocker.** Visual editor appearance, lifecycle and editor reopen have not passed Studio One acceptance; user reports the expected surface missing. Must reproduce and inspect actual attached window and resource resolution. Earlier hostless CTest segfaulted and was withdrawn; it does not constitute host verification.
2. **SPEC-DRIFT — FAIL / blocker to signed-off feature scope.** `docs/PRODUCT-SPEC.md` still claims 2–32 steps, step-rate selection, per-step DOUBLE, independent randomization and export/profile functionality as V1, whereas present primary editor and MIDI engine operate on fixed 16-step grids. Either implement each promised function with tests or explicitly amend and approve a narrower V1 scope before release. Do not advertise unimplemented features.
3. **REALTIME-LEGACY — corrected, retest pending.** `Processor::process` formerly wrote `steps_[index]` on automation changes for all 16 slots despite serialized V1–V8 count accepting 1–32; legacy states under 16 steps could write past the vector. Added bounds guards in current development branch. Needs a dedicated saved-short-state + automation reproducer.
4. **HOST-MIDI — NOT VERIFIED.** Four VST3 event output buses are declared and deterministic multi-bus scheduling passes CTest. Studio One routing to external or multi-timbral instruments, host bus activation, program/plugin reload and stuck-note torture remain unverified.
5. **STATE/AUTOMATION — PARTIAL PASS.** Processor V8 serialization/restore and Controller V8 normalized parameter sync have automated regressions; arbitrary automation point density, interleaved restoration and host project reopen need further checks.
6. **TRANSPORT — PARTIAL PASS.** Deterministic Stop/Restart and note-flush scenarios pass core tests. Actual DAW loop/jump/offline, block-size change and external bass mode switch under load need an independent host gate.
7. **GUI VISUAL — NOT VERIFIED.** UI metadata says 1120×695; 150% implies 1680×1042.5 before Studio One chrome and exceeds the earlier `GUI-V1-IMPLEMENTATION-CONTRACT.md` 1120×670 / 1680×1005 limit. GUI must be measured at 1920×1080 with native scaling and controls inspected.
8. **RELEASE PACKAGE — PARTIAL PASS.** Bundle name, one binary, `rifferator.uidesc` copied and template name checked in workflow. This does not prove GUI resource loading/attachment or 125A Plugin Tester pass.
9. **PRODUCT-QUALITY — NOT VERIFIED.** Generated riff variety, PM/Power articulations with target sampled guitar, bass/harmony arrangement and practical playability require MIDI capture and listening through real target libraries.
10. **PLATFORM — NOT VERIFIED beyond Windows CI.** No Mac compatibility claim. No independently verified Studio One version/Windows DPI host snapshot.

## Required completion gates
- Fix host GUI visibility with reproducible diagnosis rather than speculative resource changes; 20 consecutive open/close/reopen cycles and restore.
- Compare the editor creation/resource contracts against authoritative 125A Midiator Final and validate expected studio rendering.
- Add deterministic legacy-short-state + 16-step automation test, fast automation including note termination and malformed MIDI state tests.
- Establish agreed V1 scope (implement or remove unsupported items from product promises) and align docs/gui.
- Complete MIDI bus routing/transport/automation/state tests in Studio One including save/reopen and repeated song load.
- Pass 125A Plugin Tester, real Editor Lifecycle, event I/O and offline/lifecycle torture. Maintain CI Steinberg Validator + CTest passes.
- Package only the exact tested artifact and freeze source SHA before creating Final repository/release.

**No FINAL or market-ready PASS while any blocking row remains.**
