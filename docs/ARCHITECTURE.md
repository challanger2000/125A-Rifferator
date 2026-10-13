# MIDI subsystem separation — work-in-progress

Current branch is a **core prototype**; isolation is not a claim of complete architecture. Plan for VST3 integration:

1. **Generic library**: immutable pattern snapshot, absolute PPQ event visitor, note lifetime, NoteOff-first ordering, Standard MIDI File encoder, and real-time bounded block interface. Generic code must NOT know Heavier7Strings velocity bands, internal powerchord voices or pick direction.
2. **Heavier7Strings adapter**: encode one note per attack; map NOISE=20, PM=30..83, OPEN=115, PINCH=127. Note STOP is release, not pitch. Instrument expected settings: One Key Fifth / Velocity Muting / AUTO. Verify with real H7.
3. **VST3 Processor**: bounded events, send failures, current note IDs, NoteOn/NoteOff trigger, transport jump and loop/restart safety; no callback allocations or MIDI file I/O.
4. **VST3 Controller / GUI**: static-first design, correct VST3 edit lifecycle, note selection, state/automation and UI feedback, 100/150% host zoom.

**Current separation:** Generic timing, pattern state, diatonic degree logic, NoteOff ordering, live loop spans and Standard MIDI File encoding live under src/core. The H7 velocity map and current Generate/Variate articulation policy now live in src/instruments/Heavier7Strings.hpp. Technique IDs are opaque in shared Pattern/Settings; named H7 techniques and PM velocity bounds now belong to the H7 adapter. The synthetic bass-policy regression verifies the generic strategy without claiming an EBM product is finished.

## Separated technique policy (rebuild/v1.0.0-clean)
The core Step now stores only `on`, `stop`, `locked`, an opaque technique byte and a diatonic degree. In particular, **no guitar articulation labels or PM velocity bounds remain in the generic core**. The Heavier7Strings adapter defines its own named techniques, user PM velocity bounds 30..83, and H7 generation policy. The runtime receives a velocity policy callable. A separate synthetic-bass policy regression demonstrates that the generic pattern/emission code can encode a non-H7 technique.
