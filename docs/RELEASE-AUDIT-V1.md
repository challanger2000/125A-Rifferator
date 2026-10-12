# 125A Rifferator V1.0.0 — Release-level audit

**Decision: BLOCKED / NOT FINAL.** This is a source-and-CI evidence ledger, not an assertion of a completed Studio One host test.

## Baseline
- Repository: `challanger2000/125A-Rifferator`, branch `develop/v1.0.0`.
- Earlier pre-audit baseline: workflow #38165400049, 29/29 CTest PASS, Steinberg validator 537/537 PASS.
- Latest independently confirmed green development baseline: workflow [#38255712530](https://github.com/challanger2000/125A-Rifferator/actions/runs/38255712530), commit `1b1ff8ab5296a1293509f23b7b3e830f4f5c627f`. This verifies automated CI, **not** the attached Studio One editor.
- Windows delivery bundle renamed `125A Rifferator.vst3`; matching internal binary and packaged `rifferator.uidesc` verified by CI.
- User reports the intended editor **does not appear in Studio One**, including after #38164929473. The host GUI defect remains OPEN, irrespective of SDK validator pass.
- The current editor now uses `AspectRatioVST3Editor`, as does the working 125A Midiator. No VST3 host-attached GUI rendering PASS has been established for Rifferator.

## Confirmed release blockers and inconsistencies
1. **HOST-GUI — FAIL / blocker.** Visual editor appearance, lifecycle and editor reopen have not passed Studio One acceptance; user reports the expected surface missing. Must reproduce and inspect actual attached window and resource resolution. Earlier hostless CTest segfaulted and was withdrawn; it does not constitute host verification.
2. **SPEC-DRIFT — FAIL / blocker to signed-off feature scope.** `docs/PRODUCT-SPEC.md` still claims 2–32 steps, step-rate selection, independent randomization, and export/profile functionality as V1, whereas the present primary editor and MIDI engine operate on fixed 16-step grids. **Per-step DOUBLE is implemented** and covered by core/VST3 tests. Either implement each promised function with tests or explicitly amend and approve a narrower V1 scope before release. Do not advertise unimplemented features.
3. **REALTIME-LEGACY — automated PASS, host pending.** `Processor::process` formerly wrote `steps_[index]` for high step indices after restoring legacy projects with fewer than 16 steps. Bounds guards and a dedicated **V1 one-step state + high-index automation + V9 save** regression now pass CI. This does not verify reopening a legacy song inside Studio One.
4. **HOST-MIDI — NOT VERIFIED.** Four VST3 event output buses are declared and deterministic multi-bus scheduling passes CTest. Studio One routing to external or multi-timbral instruments, host bus activation, program/plugin reload and stuck-note torture remain unverified.
5. **STATE/AUTOMATION — PARTIAL PASS.** Processor **V9** serialization/restore and Controller V9 normalized parameter sync have automated regressions. GUI zoom had no independent controller-state persistence and is being added in the current unverified batch. Arbitrary automation point density, interleaved restoration and host project reopen need further checks.
6. **TRANSPORT — PARTIAL PASS.** Deterministic Stop/Restart and note-flush scenarios pass core tests. Actual DAW loop/jump/offline, block-size change and external bass mode switch under load need an independent host gate.
7. **GUI VISUAL — NOT VERIFIED.** The actual template is **1120×696 px**; 150% becomes **1680×1044 px** before Studio One window chrome, exceeding the earlier `GUI-V1-IMPLEMENTATION-CONTRACT.md` 1120×670 / 1680×1005 limit. GUI must be measured at 1920×1080 with native scaling and controls inspected.
8. **RELEASE PACKAGE — PARTIAL PASS.** Bundle name, one binary, `rifferator.uidesc` copied and template name checked in workflow. This does not prove GUI resource loading/attachment or 125A Plugin Tester pass.
9. **PRODUCT-QUALITY — NOT VERIFIED.** Generated riff variety, PM/Power articulations with target sampled guitar, bass/harmony arrangement and practical playability require MIDI capture and listening through real target libraries.
10. **PLATFORM — NOT VERIFIED beyond Windows CI.** No Mac compatibility claim. No independently verified Studio One version/Windows DPI host snapshot.

## 2026-10-12 — PM dynamics and FOLLOW DOUBLE verification

- Latest **confirmed** green development baseline before this change:
  [CI #38249744123](https://github.com/challanger2000/125A-Rifferator/actions/runs/38249744123).
- The guitar DOUBLE timing already emitted two separate attacks. The FOLLOW
  bass previously emitted **only one** attack in the same occupied step,
  contrary to the product's bass-follows-DOUBLE rhythm contract. The live
  scheduler now emits two bass attacks and paired NoteOffs inside the step.
  EBM/HYBRID independent bass generators remain single-owner bus 1.
- PM MIN/MAX previously clamped a constant MIDI velocity 50, making typical
  edits that still included 50 ineffective. PM velocity now varies
  deterministically across the 16-step bar inside the configured interval:
  step positions modulo 4 receive (100%, 25%, 75%, 50%) of the
  MIN-to-MAX span. DOUBLE's second guitar stroke remains slightly softer
  but cannot leave the chosen PM zone. No RNG/allocations enter the
  realtime callback.
- Automated evidence added to `industrial_guitar_groove_tests.cpp` and
  `processor_state_roundtrip_tests.cpp`: paired Guitar/FOLLOW Bass
  NoteOn/NoteOff, gate and onsets, PM MIN/MAX velocity endpoints, disabled
  and independent bass exclusion, and actual Processor event-list output.
- **Status:** IMPLEMENTED / new CI and real Studio One tests pending.
  Studio One multi-output routing, editing and musical review are
  **NOT VERIFIED**; do not infer FINAL from successful CTest/validator.

## 2026-10-12 — MIDI automation collision and DOUBLE-release regression

- Green baseline prior to this bundle: CI #38251033396, commit
  `a8b6c6598b65959ce0a6ab1b4e71be9848daccec`.
- **Found regression after adding FOLLOW DOUBLE:** changing DOUBLE on a sounding
  step released Guitar only, even though it also changes FOLLOW Bass rhythmic
  output. Corrected: flush Guitar + FOLLOW Bass, but not independent EBM/HYBRID
  Bass. A VST3 Processor event regression checks the held-bass NoteOff and a
  separate EBM test checks for no unrelated NoteOff.
- **Cross-queue automation ordering:** a LOCK queue and the same step's
  articulation, degree, ON or DOUBLE queue could be processed in either order,
  changing the musical result. The Processor now resolves each step's final
  LOCK value before step edits. A final LOCK=ON blocks all edits in that
  callback, regardless of queue order; final LOCK=OFF permits edits. Multiple
  test permutations compare serialized output byte fields. The legacy
  short-pattern test explicitly performs legitimate edits before LOCK in a
  separate callback.
- This defines **block-level final-LOCK semantics**; it is not evidence of
  sample-accurate cross-parameter automation. The real host's Controller /
  Processor synchronization on simultaneously automated controls remains a
  Studio One acceptance check and is not labelled PASS.
- **Status:** IMPLEMENTED / CI verification pending for this bundle.
  GUI attachment, real MIDI bus routing and full release acceptance remain
  **NOT VERIFIED**.

## 2026-10-12 — CI #38252685179 backpressure assertion correction

- Windows CI #38252685179 completed 28/29 CTests; the only failure was
  `rifferator_processor_state` at the backpressure regression. The test
  still asserted **no FOLLOW Bass NoteOff** after a live DOUBLE edit.
- This contradicted the newly adopted contract: DOUBLE changes both
  Guitar and FOLLOW Bass MIDI timing, and therefore both buses must release
  their held notes before the next event. The EBM/HYBRID bass remains
  independent.
- The regression now explicitly checks that the retry callback releases
  Guitar, FOLLOW Bass (pitch 28), remaining Drum and Chord notes, while
  the already accepted kick 36 NoteOff is **not** sent a second time.
- This was a stale test oracle, not evidence that releasing FOLLOW Bass is
  incorrect. Keep release semantics and associated assertions aligned when
  a feature's MIDI ownership changes.
- **Status:** new CI pending; no completed DAW or release QA verdict.

## 2026-10-12 — CI #38253543438 FILL/LOCK test setup correction

- CI #38253543438 failed only `rifferator_processor_state` at
  `ng==1 && nb==1` in the FILL/LOCK end-to-end test; 28/29 CTests
  otherwise passed.
- The fixture had begun enabling/editing an inactive step **in the same
  parameter block that set LOCK=ON**. The Processor's deliberately strict
  final-block LOCK precedence correctly rejected the edits, leaving the
  fixture's step OFF. This contradicted the fixture's expectation that
  Guitar and FOLLOW Bass NoteOns would be emitted.
- Correct fixture sequence: apply FILL, activate step, set DEGREE in an
  initial parameter callback; optionally apply LOCK in a second callback;
  then process the selected FILL onset and inspect both actual MIDI buses.
  An independent queue-permutation regression separately validates the
  LOCK precedence rule.
- **Status:** correction committed; fresh Windows CI result pending.
  No claim of real DAW host lifecycle validation.

## 2026-10-12 — HYBRID harmony root and held-note ownership

- Most recent successful regression baseline: [CI #38254546977](https://github.com/challanger2000/125A-Rifferator/actions/runs/38254546977), commit `56a70ae1136c58138a8345f6ad1310b4b9864ac5`.
- **Found:** Automating `kHybridRootBase` changed the chord root and also the HYBRID bass's computed MIDI pitch. Only the Chords output (bus 3) received an immediate held-note flush; the HYBRID Bass output (bus 1) could retain its previous pitch until natural note end.
- **Corrected:** Explicit harmony-root changes release Chords, and additionally release Bass if the *currently active* bass mode is HYBRID. FOLLOW and independent EBM Bass must not be cut by an unrelated chord/harmony-root parameter edit; guitar and drums are unaffected.
- **Regression:** A real VST3 Processor/host EventList test starts held Chord and Bass voices, automates the same explicit root under FOLLOW/EBM/HYBRID, then compares NoteOffs on all four event buses.
- **Status:** implementation and regression added; this batch is **CI pending**. Real Studio One multi-bus/GUI QA is not verified.

## 2026-10-12 — Controller zoom state and host acceptance

- **Found:** 100%/150% editor zoom was held only in a transient
  `Controller::zoomFactor_` variable; the VST3 Controller did not implement
  `getState/setState`. Reopening a saved song with a fresh controller could
  reset the editor to 100% even if the session had been saved at 150%.
- **Implemented:** independent versioned Controller state with three 32-bit
  words (magic `RFZ1`, version 1, zoom 100 or 150); complete validation
  before changing UI state, reapplication to an attached editor, and update
  of its zoom indicators. Processor V9 musical snapshot remains unchanged,
  preserving musical preset/project state layout.
- **Automated tests:** Controller default 100%, 150% save/restore, back to
  100%, invalid magic, unknown version, invalid zoom, truncated stream and
  null stream. Malformed input cannot change the prior zoom or musical
  parameter values.
- **Status:** new CI **pending**, and real Studio One project save/reopen
  including 150% selection is **NOT VERIFIED**. At 150%, nominal editor
  height is 1044 px **plus host chrome**, potentially exceeding a
  1920×1080 screen. Do not claim full visual fit or lifecycle PASS until
  measured in the DAW.

## Required completion gates
- Fix host GUI visibility with reproducible diagnosis rather than speculative resource changes; 20 consecutive open/close/reopen cycles and restore.
- Compare the editor creation/resource contracts against authoritative 125A Midiator Final and validate expected studio rendering.
- Add deterministic legacy-short-state + 16-step automation test, fast automation including note termination and malformed MIDI state tests.
- Establish agreed V1 scope (implement or remove unsupported items from product promises) and align docs/gui.
- Complete MIDI bus routing/transport/automation/state tests in Studio One including save/reopen and repeated song load.
- Pass 125A Plugin Tester, real Editor Lifecycle, event I/O and offline/lifecycle torture. Maintain CI Steinberg Validator + CTest passes.
- Package only the exact tested artifact and freeze source SHA before creating Final repository/release.

**No FINAL or market-ready PASS while any blocking row remains.**
