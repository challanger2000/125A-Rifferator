# MIDI subsystem separation — work-in-progress

Current branch is a **core prototype**; isolation is not a claim of complete architecture. Plan for VST3 integration:

1. **Generic library**: immutable pattern snapshot, absolute PPQ event visitor, note lifetime, NoteOff-first ordering, Standard MIDI File encoder, and real-time bounded block interface. Generic code must NOT know Heavier7Strings velocity bands, internal powerchord voices or pick direction.
2. **Heavier7Strings adapter**: encode one note per attack; map NOISE=20, PM=30..83, OPEN=115, PINCH=127. Note STOP is release, not pitch. Instrument expected settings: One Key Fifth / Velocity Muting / AUTO. Verify with real H7.
3. **VST3 Processor**: bounded events, send failures, current note IDs, NoteOn/NoteOff trigger, transport jump and loop/restart safety; no callback allocations or MIDI file I/O.
4. **VST3 Controller / GUI**: static-first design, correct VST3 edit lifecycle, note selection, state/automation and UI feedback, 100/150% host zoom.

**Known stage-one debt:** Pattern.hpp currently combines generic temporal logic with a preliminary H7 velocity function and simple guitar-specific Generate/Variate rules. These must be extracted to a separate instrument policy before VST3 integration and before claiming universal engine complete. Do not transplant this mixed header unexamined into any future EBM bass product.
