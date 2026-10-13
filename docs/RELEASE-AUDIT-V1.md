# 125A Rifferator V1.0.0 — Release audit matrix

**Repository:** `challanger2000/125A-Rifferator` · **development branch:** `rebuild/v1.0.0-clean` · **code commit:** `9c1a1629ce59f18f0a4ab2afd591aa402d22d974` · **status:** **NOT FINAL**.

Evidence: [Windows x64 Release build / complete job #38424202886](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886) and [experimental VST3 package](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886/artifacts/11827572883). Existing user Studio One evidence is recorded in [STUDIO-ONE-ACCEPTANCE.md](STUDIO-ONE-ACCEPTANCE.md). A green SDK-host check is not a Studio One acceptance result.

| Requirement | Proven result | Remaining gate |
|---|---|---|
| Windows x64 VST3/SDK build, branded resources | PASS, Actions #38424202886 | None for the build |
| Six deterministic regression suites, including 4/8-bar scheduling | 6/6 PASS | DAW 4/8-bar playback |
| Steinberg VST3 Validator | 537/537 PASS | Distinct 125A gates |
| Native HWND Editor attach/detach | 20/20 PASS | Real Studio One zoom/reopen on latest build |
| 8-bar Processor and Controller project serialization | PASS, native VST3 host | Studio One project save/reopen |
| Controller-only generator seed state | RGU1 backward migration, RGU2 16-byte state, truncation rejection PASS | Generator continuation after actual DAW reload |
| LOCK-protected ON and DEGREE host automation readback | PASS: rejected edits restore actual normalized values | Studio One GUI/automation interaction |
| KEY/SCALE/ROOT consistency and state | SDK-native KEY automation and ROOT snap PASS | Studio One host automation |
| VST3 input/output MIDI buses and Heavier7Strings routing | SDK-native PASS; user heard live rhythms in Studio One | Musical quality/articulations |
| PM / OPEN velocity; STOP/transport release | SDK-native 65/115 and NoteOff PASS; user STOP PASS | H7 all four modes NOISE/PM/OPEN/PINCH |
| MIDI NOTE trigger NoteOn/NoteOff | SDK-native offsets 100/200 PASS | Studio One trigger mode |
| 44.1/96 kHz realtime and offline; block sizes 1/31/127/257/512 | PASS: four cases, 3 on/3 off each | Full third-party torture |
| GUI bar selection, note picker, LOCK, generate, clear, export | Earlier Studio One user confirmations and geometry regressions | Latest picker screenshot, Ctrl+left-click reset policy applicability |
| Full original 125A Plugin Tester v0.2.9 | **PASS: 30/30, zero warnings, zero failures** in private [run #38436972585](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585), source `8b70827f` compiled as Windows VST3 by the tester job; event/I/O, state, editor, module reload, validator and 50/50 block-rate combos verified. MIDI-only: audio-output torture correctly not applicable. [Five diagnostic reports](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585/artifacts/11833722284). | **DONE for technical 125A Tester**, distinct Studio One/musical acceptance still pending |
| Real Studio One project recall / control automation / musical approval | **PARTIALLY VERIFIED** (earlier interaction, export and direct MIDI) | Explicit current-build signoff |

**Decision (2026-10-13 update):** Steinberg, CTests, native host torture and the original **125A Plugin Tester technical gate are all satisfied** for exact source `8b70827f`. The full tester was run **from its private repository**, accessing the public Rifferator source; no private tester code was copied into the public repository. **NOT FINAL** until actual Studio One GUI clicking (notably KEY/ROOT menus, STOP on/off, Ctrl reset and 100/150%-coordinate alignment), project save/reopen, host automation and final H7 musical acceptance are confirmed. Keep version 1.0.0.
