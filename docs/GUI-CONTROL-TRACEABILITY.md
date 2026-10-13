# GUI action-to-parameter traceability (first VSTGUI source)

Geometry is defined in src/vst3/UiGeometry.hpp and checked by tests/gui_geometry_tests.cpp (5th CTest). A custom CView draws and hit-tests the controls. A click in the grid uses BAR × 16 + local step index, not a scrolling frame-local MIDI clock.

| Interactive area | Controller routing | VST3 automation ID(s) | Downstream effect |
| --- | --- | --- | --- |
| BAR count | Controller::click(Bars) -> publish | 103 | Pattern 1/2/4/8 bars |
| KEY | Controller::click(Key) -> publish | 100,102 | Tonal key and valid tonic-root |
| SCALE | Controller::click(Scale) -> publish | 101 | Major/natural minor pitch mode |
| ROOT NOTE | Controller::click(Root) -> publish | 102 | Scale-member root |
| PM VELOCITY | Controller::click(Pm) -> publish | 104 | Heavier7Strings PM profile |
| FILL INTENSITY | Controller::click(Fill) -> publish | 105 | Future generate/variate pitch variation density |
| TRIGGER | Controller::click(Mode) -> publish | 106 | Absolute PPQ/MIDI NOTE |
| BAR prev/next | Controller local bar_ | editor-only | Changes shown 16-step window only |
| 5×16 grid | Controller::click(Cell) | 1000+step;1200+step;1400+step | ON, STOP, four H7 articulations |
| SHIFT+left-click cell and picker | Controller::click(Picker) | 1600+step;1000+step | Relative diatonic pitch; activates step |
| STEP ON | Controller::click(StepOn) | 1000+step | On/off selected step |
| LOCK | Controller::click(Lock) | 1800+step | Prevents edits and generator replacement |
| GENERATE | Controller::click(Generate) | changed pattern step IDs | Deterministic H7 pattern generation |
| VARIATE | Controller::click(Variate) | changed pattern step IDs | Modify unlocked pattern |
| CLEAR | Controller::click(Clear) | changed pattern step IDs | Clears unlocked cells |
| EXPORT MIDI | Controller::exportMidiFromUI | no host parameter | GUI-thread dialog, SMF0 file write |
| ZOOM | Controller::click(Zoom) | controller editor state only | VSTGUI 100/150% host editor scaling |

Lifecycle: Controller::createCustomView builds branded panel and vector logo view, willClose nulls all editor pointers, UI setState/getState retain zoom/bar. Host mutation enters setParamNormalized, then requests redraw. Parameter edits use beginEdit / setParamNormalized / performEdit / endEdit. Processor applies bounded inputParameterChanges and releases held MIDI when pattern changes.

**Open issues (updated for V1):** Shift-picker and export were visually/user-tested in Studio One; 20 native editor attach cycles and actual VST3 MIDI/KEY host automation passed earlier independent host QA. GUI picker overlap was resolved and Windows geometry-tested. Controller-only seed/zoom/bar RGU2 state with RGU1 migration and locked-step host readback is in pending CI. Real Studio One host automation and project resave/reopen, 125A Plugin Tester and subjective musical release approval remain separate gates. SOURCE PRESENCE IS NOT DAW PASS.

## Direct header controls (pending native CI and Studio One acceptance)
The previous repeated-click behavior of KEY, ROOT NOTE, PATTERN, SCALE, PM VELOCITY and FILL INTENSITY is replaced. Native VSTGUI menus: KEY 12 direct choices, SCALE two choices, PATTERN 1/2/4/8 and ROOT note by grouped octave with every currently in-scale MIDI note 0..127 plus a tonic-octave-2 reset option. PM (30–83) and FILL (0–100%) select the desired absolute value by horizontal click, with visible strips and tested endpoint mapping. Ctrl+left-click returns to defined defaults (KEY E and ROOT E2; SCALE natural minor; ROOT tonic octave 2 for the active key; 1 bar, PM 30, FILL 0, transport mode, zoom 100%). The existing VST3 parameter IDs and host publication are unchanged. Native menu click and user-visible appearance still require actual Studio One acceptance after CI.

## 2026-10-13 element audit — STOP, Ctrl and matrix contracts (CI PENDING)
New `src/vst3/UiStepPolicy.hpp` centralizes all five row-click transitions and the Ctrl-reset applicability. Repeated clicking a STOP toggles it off (prior bug: could only set, never clear). Ctrl+left-click on an unlocked matrix cell resets the entire selected step to neutral/default; on STEP ON it turns that step off; on LOCK it unlocks. Locked cells still reject edits. Ctrl on non-value commands such as Generate/Variate/Clear/Export and BAR navigation is no longer swallowed and proceeds normally. Every matrix row for 1/2/4/8 bars and every active step (up to 128) now has two-click ON/OFF test coverage, including LOCK/reset boundaries. MIDI compile regression checks all four H7 articulation velocities 127/115/30/20 for the actual row mapping. Source-level and SDK-independent evidence, not a Studio One mouse gesture PASS.
