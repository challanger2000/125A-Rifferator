# Bass engine integration contract (draft V1)

Status: development interface proposal; not an integrated EBM implementation.

## Ownership and separation
- Guitar riff generation remains the source of harmonic events.
- FOLLOW stays available and deterministic; never overwrite its implementation.
- EBM is an independently generated rhythm/pitch stream; guitar REST steps do not mute EBM.
- HYBRID uses the EBM rhythm while responding to guitar harmony at explicitly defined change boundaries.
- Only the integration branch may reconcile external bass work; do not merge the parallel branch blindly.

## Shared musical clock
- Authoritative source: host PPQ, tempo, 4/4 meter when requested, transport state.
- Tick resolution: 960 PPQ; sixteenth note = 240 ticks.
- A 16-bar phrase has 61440 ticks in 4/4; preserve cycle boundaries and fills.
- The host scheduler owns final sample offsets and NoteOn/NoteOff delivery.
- Stop, seek, discontinuity, preset switch, and switching bass modes require panic / safe off delivery.

## Required interface (data level)
- Input: mode FOLLOW | EBM | HYBRID; riff's harmonic root and any harmonic change points; meter/phrase length; deterministic seed; intensity and rhythmic options.
- Output: bounded, ordered MIDI note events (tick onset, gate end/boundary, MIDI pitch, velocity) on the Bass logical event bus (1).
- Pitch range must be clamped and documented; no hidden shift in octaves.
- EBM and HYBRID must not depend on guitar rests for their rhythmic pattern.
- Harmonic change points are explicit metadata, not guesses from the last struck guitar note.

## Integration and release gates
1. Independent bass tests and deterministic seed regression.
2. Interface contract review, no ABI or state-version assumptions.
3. Precompute immutable bass arrangements off the realtime thread.
4. Validate FOLLOW/EBM/HYBRID changes: all sounding old notes receive NoteOff.
5. Test 32/64-bit process, all sample rates and variable block sizes.
6. Confirm two-output routing in Studio One. VST3 validation alone does not establish DAW routing.
7. Save/restore mode and bass pattern seed in versioned project state.
8. No main or Final modification until all relevant 125A QA passes.

## Musical intent
Industrial/NDH example: 128 BPM, 4/4. Guitar palm-mutes with rhythmic rests
against persistent EBM bass eighths; chromatic neighbor pitches only when
deliberately allowed by the selected harmonic-tension behavior.
