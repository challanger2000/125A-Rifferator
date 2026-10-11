# EBM bass engine – isolated experimental module

Status: IMPLEMENTED AS STANDALONE ALGORITHM; NOT YET INTEGRATED OR HOST-TESTED.
Development branch: feature/ebm-bass-engine. Do not merge into develop/v1.0.0 while guitar work is ongoing.

## Design boundaries
- Core is header-only, event-model agnostic, with no changes to guitar processor, MIDI buses, factory, GUI, VST3 state or CMake.
- Pure MIDI note-pattern generation; no audio synthesis, DSP or preset copying.
- Settings specify MIDI root, 16/32 sixteenth-note slots, style, velocity, gate, variation, seed.
- Chromatic adjacent semitone on tonic (e.g. C-C#-C), octave, rolling and syncopated patterns.
- 0 gate suppresses all notes; 1000 permille = 100% of step duration; actual note-off scheduling must be implemented and tested during integration.
- Engine's deterministic output is not a claim of rhythm/transport synchronization. Pattern generation does not schedule MIDI events.

## Integration requirements
- Keep existing follow-guitar bass semantics unchanged by default.
- New mode choices proposed: FOLLOW, EBM, HYBRID (HYBRID requires musical/harmonic transition rules).
- Verify single vs dual VST3 event-bus routing and alternate channel split/SMF export in actual DAWs.
- Before state/GUI wiring, agree parameter IDs, migration/defaults and processing lifecycle with the owner of the active guitar development branch.
- Test note-on/off ordering, rest/tie behavior, repeat boundaries, transport seek/stop, deterministic seed and project recall.
- Validate output across varying block sizes, 32/64-bit host processing and tempo changes. No QA pass claim yet.
- Documentation and algorithm are original 125A work, not transcribed or copied Tonepusher presets or sequences.

## Standalone test
`c++ -std=c++23 -Wall -Wextra -pedantic tests/ebm_bass_engine_tests.cpp -o ebm_bass_engine_tests && ./ebm_bass_engine_tests`

## Optional rhythmic development
- Settings.rhythmVariation (0..100) controls deterministic offbeat omissions independently of pitch variation. At zero, prior groove behavior is unchanged.
- Steps at multiples of four remain anchored; rests can only affect offbeat positions 1 and 3 within each quarter-note beat. No new notes or timing overlaps are introduced.
- This parameter is intentionally internal until coordinated host/state/GUI integration; it does not alter any existing shared parameter IDs.
