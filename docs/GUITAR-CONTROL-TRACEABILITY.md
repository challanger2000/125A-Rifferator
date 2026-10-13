# 125A Rifferator — guitar-only control/requirement traceability

Source redesign staged 2026-10-13. **DAW interactions NOT VERIFIED.** This matrix distinguishes implementation from actual host observations; no blanket GUI PASS.

| Feature (GUI tags) | VST3 ID / controller route | Processor / MIDI outcome | Static test | Host |
| --- | --- | --- | --- | --- |
| 16× STEP ON `GuitarActive0..15` | 300..315, edit callback | Enable/disable one guitar step | GUI structural, processor state | NOT VERIFIED |
| 16× articulation × six rows `GCell0_0..GCell5_15` | 400..415 with 300..315 | One exclusive articulation; STOP releases active guitar notes | 16-step matrix structural, scheduler regression | NOT VERIFIED |
| 16× DOUBLE `GDouble0..15` | 448..463 | Exactly two sequential strokes/step | Scheduler double regression | NOT VERIFIED |
| 16× LOCK `GLock0..15` | 432..447 | Protects whole step vs live edit/generation | Generator and state tests | NOT VERIFIED |
| 16× degree `DegreeValue0..15` plus ± | 416..431; direct `CTextEdit`, commands 800..831 | Relative scale degrees -13..15 | GUI, numeric, state | NOT VERIFIED |
| ROOT NOTE `RootValue` plus ± | 103; direct `CTextEdit`, commands 900/901 | Guitars transpose; E2=40 | Numeric, scheduler, state | NOT VERIFIED |
| GTR GATE `GtrGateValue` plus ± | 100; direct edit, 902/903 | 0 suppresses; 1..100% actual note gate | Scheduler, state, GUI | NOT VERIFIED |
| FILL INTENSITY `FillValue` plus ± | 105; direct edit, 906/907 | 0 disables; bounded prepared fill | Fill tests, state | NOT VERIFIED |
| PM MIN/MAX `PmMinValue`, `PmMaxValue` plus ± | 112/113; direct edit, 880..883 | MIDI velocity band user-configurable 1..127 | PM regression, state | NOT VERIFIED |
| GENERATE `GuitarGenerate` | controller command 700 | New seeded unlocked guitar pattern | Generator determinism/LOCK | NOT VERIFIED |
| VARIATE `GuitarVariate` | command 722 | Bounded unlocked step variation | Generator determinism/LOCK | NOT VERIFIED |
| CLEAR `ClearPattern` | command 721 | Clear unlocked activity and double, retain LOCK | source contract; host required | NOT VERIFIED |
| ZOOM `Action701/702` | Editor zoom 100/150%; separate controller state v2 | No MIDI/state modification | static editor size | NOT VERIFIED |

Unlisted control IDs = FAIL; duplicate tags or overlapping nested hit regions fail GUI CI. State format = V10, legacy V1..V9 accepted via common shared parser. Real mouse/keyboard tests, automation, save/reopen, actual emitted MIDI and 125A Tester still block FINAL.
