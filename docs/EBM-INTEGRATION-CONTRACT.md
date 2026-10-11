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
