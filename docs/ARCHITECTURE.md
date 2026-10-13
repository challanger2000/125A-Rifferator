# MIDI subsystem separation — work-in-progress

Current branch is a **core prototype**; isolation is not a claim of complete architecture. Plan for VST3 integration:

1. **Generic library**: immutable pattern snapshot, absolute PPQ event visitor, note lifetime, NoteOff-first ordering, Standard MIDI File encoder, and real-time bounded block interface. Generic code must NOT know Heavier7Strings velocity bands, internal powerchord voices or pick direction.
2. **Heavier7Strings adapter**: encode one note per attack; map NOISE=20, PM=30..83, OPEN=115, PINCH=127. Note STOP is release, not pitch. Instrument expected settings: One Key Fifth / Velocity Muting / AUTO. Verify with real H7.
3. **VST3 Processor**: bounded events, send failures, current note IDs, NoteOn/NoteOff trigger, transport jump and loop/restart safety; no callback allocations or MIDI file I/O.
4. **VST3 Controller / GUI**: static-first design, correct VST3 edit lifecycle, note selection, state/automation and UI feedback, 100/150% host zoom.

**Current separation:** Generic timing, pattern state, diatonic degree logic, NoteOff ordering, live loop spans and Standard MIDI File encoding live under src/core. The H7 velocity map and current Generate/Variate articulation policy now live in src/instruments/Heavier7Strings.hpp. A further boundary is needed before universal reuse: the shared Pattern/Settings still name H7 articulation choices and PM velocity. Keep them isolated from future EBM bass until a genuinely generic StepAction/InstrumentPolicy contract is regression-tested.
