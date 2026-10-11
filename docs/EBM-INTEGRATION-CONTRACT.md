# 125A Rifferator – EBM integration contract

Status: ISOLATED EBM MODULE READY FOR COORDINATED INTEGRATION; NO MERGE, VST3 INTEGRATION OR DAW QA YET.
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
- Arrangement is prepared OFF the audio thread and retains independent guitar/bass MidiNote vectors. The isolated EbmSchedulerAdapter sends EBM/HYBRID bass notes on logical bus 1; guitar scheduling remains under the existing host path. Do NOT schedule the same bass notes through both adapters.
- 16 bars, sixteen 1/16 slots per bar, 960 PPQ, default step ticks=240.
- Host transport and lifecycle handling remain responsibilities of the existing scheduler and VST3 adapter.
- Sequence generation is deterministic from seed. Fill accents and four-stage 16-bar rhythmic development are implemented and covered by isolated regression tests.
- Realtime safety: prepare allocates, therefore must NEVER be called from process() or the audio thread. Use prepared snapshot handoff and existing lifecycle safeguards during integration.
- Existing BlockScheduler applies a global bass gate and replaces bass velocity. EBM/HYBRID must route through EbmSchedulerAdapter or an equivalent verified mode-aware path that preserves both per-note gate and velocity; FOLLOW retains its existing global controls. Do not feed EBM bass into both schedulers. No processor integration has yet been made.
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
- The current shared BlockScheduler forwards LiveControls.bassVelocity for every bass NoteOn, overriding MidiNote.velocity. EbmSchedulerAdapter preserves the generated per-note velocity, so integration must select the independent adapter for EBM/HYBRID (or implement a tested equivalent) and must not run shared bass scheduling simultaneously. Do not claim host-audible bass dynamics before this routing change.
- The scheduler currently treats active pitches as booleans; test repeated same-pitch steps at 100% gate, NoteOff-before-NoteOn ordering, transport stop/seek/loop flush and host event rejection at integration.
- The EBM per-note duration is already encoded in boundaryTick, and must be combined with LiveControls.bassGate=100 only for EBM/HYBRID.
- Standalone midi-audit tests cover all five styles, three tension modes, six roots including MIDI 0/127, four gates, phrase variation, fills, dynamics and deterministic regeneration. This is an offline invariants audit, NOT an operational host MIDI-output test.

## Hot arrangement replacement protocol
- Before replacing the immutable prepared Arrangement during continuous playback, call EbmSchedulerAdapter::patternChanged(sink) to flush active Bass NoteOffs (bus 1) and reset expected PPQ tracking.
- If the host rejects a NoteOff, patternChanged returns false; retry before switching patterns. Do not use reset() on sounding notes, because reset() deliberately clears tracking without outputting MIDI.
- Do not mutate/swap the arrangement while process() is reading it; coordinate immutable snapshot publication off the audio thread.
- Host DAW lifecycle and VST3 output-bus integration still require separate end-to-end validation.

## Silent-mode lifecycle
- For a valid 16-bar EBM/HYBRID plan with gatePermille=0, prepare() must clear bass notes AND retain the intended 16-bar period. Invalid geometry must leave the entire Arrangement unchanged.
- Runtime transitions to a silent prepared snapshot require patternChanged() before publication; silence alone does not deliver NoteOffs for previously sounding notes.

## Optional four-stage arrangement evolution
- BassPlan.arrangementEvolution (0..100) applies deterministic rhythmic contrast across four 4-bar sections. The first four bars preserve the existing motif; bars 5–8 introduce some offbeat rests; bars 9–12 create the sparsest break; bars 13–16 restore the driving rhythm.
- All quarter-note anchors remain; fill and dynamics controls stay independently usable. Value 0 is fully backward-compatible and adds no rhythmic change. This is a bass-only, off-thread precomputation feature; not wired to shared parameter IDs or VST3 state.

## Host rejection recovery interlock
- When a NoteOn/NoteOff sink rejects an event in the emission stage, the adapter enters recoveryPending. The next processing callback retries active NoteOffs only and emits no NoteOns; if any NoteOff is still refused, recoveryPending remains set. Once all are accepted, that callback returns false and transport continuity resets; new NoteOns can resume on the following callback.
- This is an isolated EBM scheduler policy, not a modification of the shared guitar scheduler. The VST3 host adapter must respect process() returning false.

## Coordinated integration handoff — isolated module accepted
- **Source branch:** `feature/ebm-bass-engine` only. This handoff authorizes review and planned integration, NOT a merge or changes to `develop/v1.0.0` without the user's explicit permission.
- **Core:** `src/core/EbmBassEngine.hpp`; **offline composition:** `src/core/EbmBassArrangement.hpp`; **realtime MIDI:** `src/core/EbmSchedulerAdapter.hpp`.
- **Validation:** six compiled C++23 test programs in `.github/workflows/ebm-bass-tests.yml`; latest verified successful head `3392dcbb9e0fff4ee7c5b4a47dd75181ef9d3905`. Documentation changes do not themselves constitute a new runtime validation.
- **Audio callback:** call only `EbmSchedulerAdapter::process()` on an immutable, safely published prepared snapshot. NEVER call `prepare()` there. Verify actual VST3 processor and host callbacks for allocations/locks; isolated tests do not cover them.
- **Hot swap:** call `patternChanged(sink)` while the previous snapshot remains valid; on rejection retain the old snapshot and retry. A failed `process()` must not be treated as successful emission. Do not call `reset()` while notes may be sounding.
- **MIDI mode selection:** FOLLOW retains existing scheduling; EBM/HYBRID use exactly one bass output path on logical bus 1. Preserve `MidiNote.velocity` and precomputed gate. Guitar bus 0 must remain unchanged.
- **Song harmony:** HYBRID needs explicit per-bar harmonic roots from a separately agreed song source; do not infer them from arbitrary guitar pitches. EBM holds its selected root.
- **Parameter/state:** allocate new IDs, defaults and versioned serialization only in the coordinated integrating branch, preserving old project recall and FOLLOW defaults. Design mode GUI only after the implementation owner confirms layout.
- **Host acceptance gates:** VST3 compilation; Steinberg Validator; Studio One routing on all declared event outputs; actual repeated note at 100% gate; loop/seek/stop/hot-swap; dynamic velocity/gate; tempo changes; buffer sizes; plugin reload/project restore; regression against guitar and FOLLOW bass; offline and real-time behavior. Cubase must be tested separately before declaring support.
- **Performance caveat:** isolated process callback had zero heap allocations in instrumented tests, not a full WCET bound or host-level realtime certification. MIDI event collection uses a bounded 512-event local buffer, and over-capacity input deliberately fails closed.
