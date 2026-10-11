# 125A Rifferator – EBM integration contract

Status: EXPERIMENTAL ADAPTER COMMITTED; NO VST3 INTEGRATION OR DAW QA YET.
Branch: feature/ebm-bass-engine; owner of develop/v1.0.0 is unaffected.

## Modes
- FOLLOW: existing guitar-derived bass unchanged, default for backward compatibility.
- EBM: own repeating sequence, no dependence on guitar rests; first harmonic root anchors the entire 16-bar arrangement.
- HYBRID: same independently generated EBM rhythm; per-bar explicit harmonic-root input updates pitch without erasing steps or introducing guitar rests.

## Harmonic tension
- STABLE: root/5th/octave; chromatic tension collapses to root.
- TENSION: controlled sparse chromatic passing notes.
- AGGRESSIVE: chromatic notes intentionally retained.

## Shared architectural assumptions
- Arrangement is prepared OFF the audio thread, owns separate guitar/bass MidiNote vectors, and is scheduled by BlockScheduler on logical buses 0 and 1.
- 16 bars, sixteen 1/16 slots per bar, 960 PPQ, default step ticks=240.
- Host transport and lifecycle handling remain responsibilities of the existing scheduler and VST3 adapter.
- Sequence generation deterministic from seed. Fill behavior and richer 16-bar phrase evolution still need dedicated design and tests.
- Realtime safety: prepare allocates, therefore must NEVER be called from process() or the audio thread. Use prepared snapshot handoff and existing lifecycle safeguards during integration.
- Existing scheduler applies a global bass gate. The engine gatePermille now defines a MIDI note's boundaryTick. Integration must set LiveControls.bassGate=100 for EBM/HYBRID to avoid applying both percentages. FOLLOW retains the original global bassGate. No processor integration has yet been made.
- Unlike pattern event independence, MIDI bus host routing is not proven by this adapter; DAW host acceptance needs tests.

## Integration stop lines
Do not touch guitar core, scheduler, processor, factory, GUI, shared parameter IDs or plugin state until merging is explicitly coordinated and compatibility tests are designed.
The harmony array must be constructed from explicit song harmonic roots or an agreed chord-root source; do NOT derive harmonic function from arbitrary guitar note pitches.
The 16-bar assumption is intentional here, to match the product brief; revisit before changing time-signatures or step division.

## Four-bar phrase handling
- With phraseVariation=0, four-bar motifs remain locked throughout 16 bars.
- When enabled, controlled variations occur per four-bar block with deterministic seed changes.
- Gate 0 intentionally silences EBM/HYBRID bass without changing guitar events.

## 16-bar turnaround fills
- fillIntensity=0 disables fills. fillEveryBars 4/8/16 places fills only in the last quarter of the selected bar.
- Fills keep MIDI note count, starts, durations and rests unchanged. Only velocity accent and a deterministic optional octave lift change; guitar is unaffected.
- Invalid fillEveryBars values disable fills safely.
- BassPlan defaults preserve prior sound, fillIntensity=0 and phraseVariation=0.

## Harmony input safety
- harmonicRoots defaults to 255 (unspecified), falling back to sequence.root. This keeps the configured root meaningful when no song map is provided.
- Explicit MIDI note 0 is valid and never treated as absent. EBM reads bar zero only; HYBRID reads each bar.
- Reject impossible step tick geometry before changing an existing arrangement; guitar notes are never rewritten.

## Optional 16-bar dynamics
- dynamics=0 preserves existing velocities. Values 1..100 apply a four-section velocity arc: restrained first four bars, neutral next four, increasing energy in bars 9..16.
- Pitch, rests, note starts, ends, and guitar content are unchanged. Entire arrangement remains prepared outside audio thread.
- This does not replace listening evaluation or host-level MIDI QA.

## MIDI scheduler audit – integration blockers
- The current BlockScheduler forwards LiveControls.bassVelocity for every bass NoteOn, overriding MidiNote.velocity. Therefore the EBM accent, fill and 16-bar dynamics data is currently LOST at the final MIDI output. During coordinated integration, route per-note velocity for EBM/HYBRID while preserving FOLLOW semantics. Do not claim audible bass dynamics before this change.
- The scheduler currently treats active pitches as booleans; test repeated same-pitch steps at 100% gate, NoteOff-before-NoteOn ordering, transport stop/seek/loop flush and host event rejection at integration.
- The EBM per-note duration is already encoded in boundaryTick, and must be combined with LiveControls.bassGate=100 only for EBM/HYBRID.
- Standalone midi-audit tests cover all five styles, three tension modes, six roots including MIDI 0/127, four gates, phrase variation, fills, dynamics and deterministic regeneration. This is an offline invariants audit, NOT an operational host MIDI-output test.
