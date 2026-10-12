# 125A Rifferator — Studio One host/editor QA (V1 development)

**Status: NOT VERIFIED IN HOST.** Automated CTest and the Steinberg Validator do not attach a native VSTGUI window or prove that Studio One accepts all four VST3 event outputs.

## Verified Windows development build

The currently verified artifact below is the **pre-redesign GUI**. After the two-page fix passes its own Windows CI, use **that new tested artifact** for this visual acceptance; do not retest the known overlapping layout as if it were fixed.

Use the **`rifferator-dev-windows-x64`** artifact attached to the independently confirmed successful [workflow #38293244458](https://github.com/challanger2000/125A-Rifferator/actions/runs/38293244458) on `develop/v1.0.0`, commit `41a2a26df77006e7e1d5e5527ce97759b381e64f`. This run has **29/29 CTest PASS** and **537/537 Steinberg Validator PASS**.

Install the **entire** `125A Rifferator.vst3` bundle, not only its internal DLL. Its internal binary is `Contents/x86_64-win/125A Rifferator.vst3` and its GUI resource is `Contents/Resources/rifferator.uidesc`. The former bare package name `Rifferator.vst3` is an internal build-stage name, **not** the installed artifact name.

## Stage A — Actual native GUI attachment (release blocker)

1. Close Studio One, remove previously installed development copies and install exactly one `125A Rifferator.vst3` under the system VST3 directory. Relaunch and rescan. Record Studio One version, Windows DPI/scaling, monitor resolution, the artifact run and exact plug-in path.
2. Instantiate **125A Rifferator**, open its editor, and capture a screenshot. If only a generic host editor appears or the window is blank/missing, mark **FAIL**. Do **not** report GUI PASS based on the validator or XML checks. Collect the plugin scan/host error details, reproducible steps and screenshots.
3. Inspect the actual **16-column Guitar matrix**: STEPs, STOP, NOISE, PALM MUTE, SOFT, OPEN, HARMONIC, DEGREE, LOCK, DOUBLE and numeric PM MIN/MAX. Distinguish *inactive OFF cell* (no command) from an **active STOP** (sends a Guitar NoteOff at that step). Selecting an active articulation creates MIDI; clicking its selected cell again may clear the step.
4. The new editor separates **GUITAR** (tag 728) from **BASS / CHORDS / DRUMS** (tag 729). Click both pages; verify that switching pages does not change MIDI parameters, discard patterns, cover the tab buttons or leave the inactive page's controls clickable. Test both pages at **100% (1120×640)** and **150% (1680×960)**. At 150%, 960 px is content height only; measure Studio One chrome against the usable 1920×1080 desktop. Verify every control is visible, legible and clickable.
5. Close/reopen the editor **20 consecutive times**, alternating between Guitar and Rhythm pages. After reopening confirm the last selected page, zoom and all highlighted states, and verify that no stale view pointers cause hangs/crashes. If Studio One allows simultaneous editors of the same plugin instance, test closing each while the other remains interactive.

## Stage B — Actual mouse, parameter and generated MIDI checks

Record a screen capture and a destination MIDI/event monitor for **each active output** (VST3 event bus index **0 Guitar**, **1 Bass**, **2 Drums**, **3 Chords**). VST3 buses and MIDI channels are distinct: each declared output uses MIDI channel 1. Do not claim a routing PASS until the host really receives the corresponding events. If Studio One does not expose a bus, record that limitation rather than substituting a different bus.

| Front panel | Host-side action and expected observable result |
| --- | --- |
| Guitar Step, articulation, DEGREE, LOCK, DOUBLE | Selecting PM/OPEN changes the Guitar NoteOn velocity/articulation. LOCK protects active state/articulation/degree/DOUBLE from GENERATE and VARIATE. DOUBLE produces two sequential NoteOn/NoteOff attacks; FOLLOW Bass duplicates timing, not sampled-library fifth voicing. |
| GENERATE GUITAR / VARIATE GUITAR / CLEAR PATTERN | Generate advances the seeded riff; Variate makes more conservative unlocked edits, preserving unchanged active STOP commands; Clear removes **only unlocked** Guitar step activity/DOUBLE. Confirm matching GUI feedback and bus 0/1 MIDI. |
| PM MIN/MAX; Root MIDI; Guitar Gate; Bass Gate; Fill Intensity | Check value displays, endpoints and host automation. Gate **0% suppresses notes**; Fill **0% disables fills**. Validate generated pitches against the declared key/scale and MIDI note monitor. |
| Bass FOLLOW/EBM/HYBRID, Bass ON/OFF, Bass Velocity, EBM Style | Check all modes and five EBM styles, held-bass release when switching modes, and actual bus 1 pitch/velocity/gate output. |
| Chords ON/OFF, MINOR/MAJOR/POWER/SUS2/SUS4, CHORD/ROOT | Confirm bus 3 note pitches and quality, and held-note release on mode/quality changes. |
| Drums ON/OFF and three 16-step GM grids | Verify bus 2 maps to Kick **36**, Snare **38**, Closed Hat **42**. Turning Drums OFF stops new drum notes without muting other buses. |
| STANDARD BEAT / GENERATE DRUMS / VARIATE DRUMS | STANDARD BEAT returns Kick 0/4/8/12, Snare 4/12, Hat all 16 steps. Generate produces new reproducible drum patterns; Variate changes fewer steps. All three preserve global Drums ON/OFF. **Independent drum LOCK is not implemented**; do not test or promise it as a feature. |
| GUI ZOOM 100% / 150%; GUITAR / BASS–CHORDS–DRUMS tabs | Verify click positions, tab switching, inactive-page hit-test suppression, readouts, highlights and v2 Controller page+zoom recall after song save/quit/reopen. |

## Stage C — Transport, lifecycle and project restoration

1. With sustained Guitar, Bass and Chord notes, execute **Stop → Restart → Loop several times → Loop OFF → play to end → Stop**. Also test seek/jump and switching FOLLOW/EBM/HYBRID during active playback. Capture unexpected stuck notes and duplicate NoteOffs.
2. Test changing steps/DOUBLE/LOCK and harmony roots under playback, including rapid DAW automation. Verify unrelated MIDI buses continue and previously sounding notes are released correctly.
3. Save a project with distinct enabled/disabled Guitar steps, LOCK, DOUBLE, PM MIN/MAX, Root MIDI, gate, fill, drum pattern/ON-OFF, EBM style, chord mode/quality and **150% zoom with BASS / CHORDS / DRUMS page selected**. Quit/relaunch Studio One, reopen and compare visual state and **actual MIDI event sequences**, not only parameter values.
4. Run the **125A Plugin Tester** gates (editor lifecycle, I/O/event probe, offline/render + lifecycle torture, project/preset state, automation and no unexpected callback allocations). Record each PASS/FAIL/NOT TESTED separately.

## Recording and sign-off

For every stage, capture: exact tested SHA/build, Studio One version, Windows scaling, whether the editor attached, 100%/150% screenshot, MIDI output-bus activation/routing, reproduced failures and notes on controls that are inaccessible.

**Release decision remains BLOCKED / NOT FINAL** until the actual host shows a functioning editor, the four output-bus behavior and project recall are accepted, the 125A QA gates pass, and the V1 feature scope is reconciled with `docs/PRODUCT-SPEC.md`.