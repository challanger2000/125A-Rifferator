# Independent release audit — 2026-10-15

## Audit basis (recorded before implementation changes)

- Repository: `challanger2000/125A-Rifferator`
- Audit branch: `audit/v1.0.1-independent-release-2026-10-15`
- Source baseline: `repair/v1.0.1-lock-fix` at
  `048262fa9a0b28558c1590b9bb40baa2dfd316ff`
- Accepted FULL lineage: the source baseline descends directly from the private
  Studio One accepted COPY/PASTE baseline
  `c71d03f14a165c0ce81c2d06dd9475e713656963` and adds the subsequent VST3
  lock-isolation repair plus its regression coverage.
- Earlier `rebuild/v1.0.0-clean` remains the documented fallback lineage; it
  does not contain the accepted COPY/PASTE feature.
- `develop/v1.0.0` and `feature/ebm-bass-engine` are separate discontinued or
  non-integrated implementations and are not release-source candidates.
- No Demo branch, Demo tag, Demo build mode, or shared Demo/FULL binary gate is
  present in the repository refs or this source tree. Therefore there are no
  later Demo fixes to merge and Demo/FULL runtime behavior is NOT VERIFIED / not
  applicable to the current implementation.
- No Git tag, GitHub release marker, `AGENTS.md`, `CURRENT.md`, or
  `CURRENT-RELEASE.md` exists in the fetched repository. The exact customer
  FULL binary provenance is therefore NOT VERIFIED from repository evidence.

This document will be extended with independently reproduced findings,
repairs, measurements, and remaining manual gates. Existing PASS claims are
treated only as historical leads until reproduced or otherwise independently
verified during this audit.

## Confirmed findings and repairs

### Host-dependent simultaneous pitch automation

Before repair, simultaneous KEY, SCALE, and ROOT points were applied in the
order returned by `IParameterChanges`. All six permutations were reproduced;
the same values produced MIDI root 41 in three orders and 42 in the other
three. Parameter-queue enumeration is not a musical ordering contract, so
project playback could differ between hosts or processing runs.

The processor-side queue adapter now applies dependent globals in the stable
order KEY, SCALE, explicit ROOT. The explicit root is snapped against the final
key and scale. A permutation regression covers all six KEY/SCALE/ROOT orders
and both KEY/SCALE orders without an explicit root. Parameter IDs and the state
wire format remain unchanged.

### Oversized VST3 event batch could leave a hanging note

The Processor rejected an input list above the fixed 128-event realtime bound
before it had an output sink available. If the generator already owned an
active output note, that early return left both the note and MIDI-trigger
ownership alive. The bounded rejection path now emits the matching NoteOff at
sample zero, resets ownership only after successful delivery, and still returns
failure for the oversized block. The native SDK host probe now starts a real
generated note, supplies 129 input events, checks the NoteOff, and verifies that
the following block has no residual output.

### First bar paste after project reload could be dropped

The Controller toggled the paste transaction with an unpersisted Boolean while
the actual VST3 marker parameter was restored by the host. After saving with
the high marker bank selected, reopening reset the Boolean and could resend the
same normalized marker on the first paste. Hosts may coalesce an unchanged
parameter value, so the Processor might never see that transaction. Marker
selection now derives from the host-restored parameter value itself, toggles to
the opposite bank, and retains the destination bar. Regression coverage checks
repeated same-bar operations, bar changes, and simulated reloads. No parameter
ID or state format changed.

## Independently executed verification

- Linux GCC 14.2 Debug: 7/7 CTest PASS.
- Linux GCC 14.2 Release with assertions explicitly enabled: 7/7 CTest PASS.
- AddressSanitizer + UndefinedBehaviorSanitizer: 7/7 CTest PASS; no sanitizer
  finding.
- Block-boundary invariance probe: identical MIDI event streams for one-sample
  blocks and irregular `1/31/64/257/1024/4093` blocks at 44.1, 48, and 96 kHz,
  each at 40, 123.45, and 300 BPM over 12 seconds.
- Realtime microbenchmark: 200,000 callbacks using a dense eight-bar pattern
  and `1/32/64/1024` sample blocks. Three repeated runs measured 0 heap
  allocations in the processing region, p50 110 ns, p95 140–146 ns, p99
  164–169 ns. Unpaced maximums were 0.405–1.731 ms; an earlier run had one
  199.5 ms scheduler-preemption outlier. These Linux wall-clock measurements
  are development evidence, not Windows/DAW deadline proof.
- SDK-independent coverage exercises pattern generation/variation, LOCK and
  COPY/PASTE policy, MIDI timing/release/backpressure, 1/2/4/8-bar state,
  malformed state rejection, SMF0 encoding, GUI geometry/hit targets, numeric
  bounds, and parameter mapping.
- GitHub Actions Core run
  [38664652476](https://github.com/challanger2000/125A-Rifferator/actions/runs/38664652476):
  SUCCESS on repaired source commit `fff8deca9236a3c1fd5adf271c37f709bf385ab1`.
- GitHub Actions Windows VST3 run
  [38664652508](https://github.com/challanger2000/125A-Rifferator/actions/runs/38664652508):
  SUCCESS on the same source. This independently covers MSVC Release build,
  all seven CTest targets, bundle/resource checks, 20 editor lifecycle cycles,
  state and native ProcessData host probes including the new 129-event case,
  and Steinberg Validator execution.
- Windows artifact: `Rifferator-V1-EXPERIMENTAL-Windows-x64`, artifact ID
  `11940132568`, 1.58 MB, Actions archive SHA-256
  `0e7c5399757ae3245faaf76f0556f0f9b5033f75f1da7f9085ce90727f8e3198`.
  It remains an audit artifact and is not a commercial release package.
- Original 125A Plugin Tester on the repaired Windows artifact: **PASS**,
  reported by the owner after testing on 2026-10-15. The exact tester version,
  report files, run ID, and individual check counts were not supplied in this
  audit session and therefore remain NOT VERIFIED as archived evidence.

## Control and behavior matrix

| Surface / parameter | Intended effect | Automated evidence | Manual gate |
|---|---|---|---|
| Pattern 1/2/4/8 bars | Active cycle length and bar navigation | State, timeline, geometry tests PASS | Studio One menu/playback |
| Key / scale / root | In-scale pitch basis and transposition | State tests plus all same-block queue permutations PASS | Audible H7 register/menu |
| PM velocity 30–83 | Palm-mute NoteOn velocity | Endpoint/monotonic geometry and MIDI tests PASS | H7 response |
| Fill 0–100 | Degree changes only; no added attacks | Core LOCK/attack-count tests PASS | Musical review |
| Transport / MIDI Note | Host PPQ sync or held-note-owned phase | Runtime and host-probe source coverage; local core PASS | Studio One routing |
| 5×16 grid, STEP ON, STOP, type, degree | Per-step attack/release/articulation/pitch | Core, MIDI, geometry, state tests PASS | Every hit target at 100/150% |
| LOCK | Protect step edits/generation/clear; paste transaction exception | State, core and parameter transaction tests PASS | Host automation feedback |
| Generate / Variate / Clear | Deterministic pattern mutations respecting LOCK | Core tests PASS | Musical acceptance |
| Copy / Paste | Whole-bar replacement including final LOCK values | Core and parameter transaction tests PASS; reload marker repaired | Studio One save/reopen |
| Export MIDI | Valid one-track SMF0 at PPQ 960 with final releases | Independent byte parser/roundtrip tests PASS | Native save dialog/import |
| Zoom 100/150% | Fixed editor scale and controller recall | Geometry/state and Windows-probe source coverage | Native DPI/window review |

The internal `PasteTransaction` parameter has no standalone musical effect; it
coordinates the atomic bar-paste payload and retains its existing ID for
compatibility.

## Gates still NOT VERIFIED in this environment

- Studio One: all clicks/menus/drag/wheel/Escape at 100% and 150%, automation,
  save/reopen, COPY/PASTE immediately after reload, MIDI routing, transport
  start/stop/seek/loop, and repeated editor lifecycle.
- Heavier7Strings audible verification of velocity 20 Noise, PM 30–83, Open
  Powerchord 115, and Pinch/Harmonic 127 with the required H7 options.
- Exact customer FULL binary/source provenance and any Demo/FULL single-binary
  behavior; no Demo implementation exists in the fetched repository.

Until the Windows CI and manual Studio One gates pass, this source is an audit
candidate and must not be labelled `AUDIT PASSED`, FINAL, or release-ready.
