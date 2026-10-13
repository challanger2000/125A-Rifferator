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

**Open issues:** Visual layout quality and actual shift modifier dispatch, host automation/instantaneous KEY+ROOT coherence, generator seed persistence across project reloads, controller/process snapshot synchronization, state roundtrip in real host, 20-cycle editor attach and studio-one export all require further tests. SOURCE PRESENCE DOES NOT MEAN HOST PASS.
