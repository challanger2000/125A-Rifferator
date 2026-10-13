# 125A Rifferator V1 — CLEAN REBUILD

**Current development branch:** rebuild/v1.0.0-clean. **Status:** isolated C++ CORE PROTOTYPE, NOT YET VST3, NOT FINAL.

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
