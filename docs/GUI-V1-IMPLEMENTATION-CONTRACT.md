# 125A RIFFERATOR — THE INDUSTRIAL RIFF MACHINE
## V1 editor specification / implementation contract

Status: GUI design specification only; not an implemented or validated editor.
Reference: user-provided Thrash DI Riffmaker screenshot (matrix interaction only), 125A Midiator Final VSTGUI uidesc (editor architecture, zoom and DETAILS), 125A-Branding authoritative SVG geometry, 125A-Knob-Designer reusable controls.

### Constraints
- Guitar is primary editor; Bass and Drums are compact secondary bands.
- Native content size **1120 × 670 px at 100%**, **1680 × 1005 px at 150%**. No DAW border included; must be tested at 1920×1080 before approval.
- Avoid exceeding 1005 pixels in editor content at 150%; no OS title bar included in this estimate.
- Exact approved logo must be imported from `125A-Branding/125A_Logo_Master_FINAL.svg`; no substitute font, redraw, or generated approximation.
- Prefer 125A Knob Designer assets where knobs are needed; step matrix itself uses discrete buttons.
- No GUI drawing/edits from the audio thread; no allocation in processor process().
- Double-click/reset semantics and parameter normalization follow 125A Engineering.
- Every active clickable step requires distinct active/inactive, locked/unlocked, focus/hover and accessible text semantics.

### 100%-coordinate layout (units px)
| Region | x | y | w | h |
| --- | ---: | ---: | ---: | ---: |
| Header / logo / brand | 16 | 12 | 1088 | 68 |
| Guitar matrix / primary | 16 | 92 | 1088 | 350 |
| Guitar GENERATE / VARIATE / LOCK | 16 | 452 | 1088 | 58 |
| Bass band / DETAILS selector | 16 | 520 | 1088 | 52 |
| Drums band / DETAILS selector | 16 | 580 | 1088 | 52 |
| Footer / zoom / version | 16 | 638 | 1088 | 20 |

### Guitar matrix
- Left row-label gutter = 126 px. Sixteen cells occupy x=160..1056 with 16 columns, 56 px pitch; for 32 steps, paginate BANK A/B rather than halving readability. Matrix remains present when Bass/Drums details open.
- Every column represents one Step. Articulation is a **single enum**, never simultaneous conflicting row booleans: REST, NOISE, PALM MUTE, SOFT, OPEN, HARMONIC.
- Clicking the same selected articulation toggles REST only when explicitly allowed; avoid ambiguity by providing a dedicated REST row.
- Pitch row shows 1-based scale degree and a bounded editor control, not an undifferentiated bar graph; doubled flag uses its own concise subdivision control.
- LOCK applies to one whole Guitar Step (articulation, degree, doubled), not just articulation.
- Generate and Variate operate on unlocked Steps only; do not edit locked values, ever.
- Generate and Variate are **commands** that require an explicit non-realtime controller->processor message or safe deferred operation. They MUST NOT allocate in process() or use continuous automatable parameter toggles as command delivery.

### Bass band
- FOLLOW / EBM / HYBRID are mutually exclusive choices, not three simultaneous layers.
- Bass engine is developed independently in another workstream. Do not import or modify it here until tests and integration contract are satisfied.
- DETAILS opens Bass-specific settings in a reversible view while keeping Guitar context.

### Drums band
- Third VST3 output bus index 2, GM notes 36 Kick, 38 Snare, 42 Closed Hat.
- DETAILS panel has 3 × 16 clickable step cells and independent drum ON/OFF; 1/8 or 1/16 hat default button plus STANDARD BEAT reset.
- 48 step host parameters kDrumStepBase..247 + kDrumEnabled=106 are the established event-level route. UI must use their current normalized values and controller edit lifecycle (beginEdit, performEdit, endEdit).
- The GM drum layer remains a 1-bar repeated companion over the whole 16-bar guitar cycle.
- STANDARDBEAT and GENERATE/VARIATE actions must not bypass protected lock states.

### Transport / state / QA acceptance
- Record and reload GUI-visible steps and locks exactly; project load must not display stale state.
- Test 100%/150% zoom, hide/show details, resize/reopen, mouse click location, host automation, transport loop, play/stop and NoteOff on all 3 buses.
- Test 16/32 step paging without changing generated MIDI on inaccessible pages.
- GUI can be called PASS only after editor lifecycle tests and actual Studio One verification, beyond the 537 validator assertions.
