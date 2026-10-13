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

2026-10-13: A separate allocation-free, host-neutral MidiRuntime.hpp and MIDI trigger transport regression suite have been introduced. Until the new core CI is green, treat these changes as unverified. No VST3 binary exists yet.

**Core/instrument split:** Pattern technique codes are opaque in the generic timing layer. Heavier7Strings.hpp owns the technique labels and velocity profile; tests exercise an independent synthetic bass velocity mapping.

A fixed-size V1 processor state codec now stores all pattern cells and musical settings with atomic validation; this is still SDK-free and its first CI validation is pending.
