# Verified 125A MIDI reference inventory (development research)

All references are read from the repositories named below. They are research baselines, not proof of Rifferator compatibility, and no Final repo has been modified.

- `125A-Midiator-Final` — current-release marker V1.0.0, Windows verified build #268, source SHA `0a656a4b8631db165191391c4a1bbdfd82b26fa6`. Relevant: multiple event outputs, project/state versioning, GuitarBrain, BassBrain, RiffEngine, deterministic arrangement tests.
- `125A-Arporator-Final` — relevant: step scheduler, seeded variation, gate, note ID tracking and event lifetime. Must resolve its exact authoritative release branch before copying implementation.
- `125A-Phraseator-Final` — CURRENT.md identifies V1.0.0 / SDK 3.8.1_build_84; relevant scheduling and note-tracker tests. This is an instrument/sampler, so audio engine should not be transplanted.
- `Repeaterator-Final` — relevant: DAW-tempo sync, repeat timing, per-note Note-Off and sample-offset handling.
- `LFOator-Final` — relevant: process-context transport/tempo requirements, sample-offset event ordering.
- `Humanizerator-Final` — relevant: timing, velocity and note-length humanization, pending Note-Off queue. For Rifferator, velocity humanization must NEVER leave the selected guitar articulation's assigned range.
- `Drifterator-Final` — relevant: bar/beat position, time-signature and transport discontinuity handling, deterministic musical positioning. Rifferator does NOT require pitch bend for its numeric scale-degree pitch selection.

## Rifferator integration decisions
1. Maintain one authoritative guitar step pattern; derive bass after every change.
2. Preserve deterministic generation by seeded RNG and frozen locked steps.
3. Use safe Note-Off ordering and state cleanup on stop, transport jumps, pattern changes, and reset.
4. Differentiate musical patterns from physical MIDI event emission (sample-accurate scheduler).
5. Verify Studio One's actual multi-bus VST3 routing before claiming simultaneous Guitar/Bass routing works. No unverified compatibility claims.
6. Consult current Final-branch/release markers again before reusing code; research alone is not an implementation or QA PASS.
7. Use current Steinberg SDK as reference; independently verify multi-event-bus and host behavior.

Status: inventory complete; MIDI host-adapter implementation and tests pending.
