# 125A RIFFERATOR — THE INDUSTRIAL RIFF MACHINE
## V1 editor specification / implementation contract

Status: V1 GUI design contract. A two-page VSTGUI redesign has been implemented in the development branch after the first Studio One screenshot exposed clipped/overlapping controls; **new build and actual Studio One acceptance remain pending**.
Reference: user-provided Thrash DI Riffmaker screenshot (matrix interaction only), 125A Midiator Final VSTGUI uidesc (editor architecture, zoom and DETAILS), 125A-Branding authoritative SVG geometry, 125A-Knob-Designer reusable controls.

### Constraints
- Guitar matrix receives a full primary page; Bass, Chords and Drums share a second, more spacious page.
- Revised native content size **1120 × 640 px at 100%**, **1680 × 960 px at 150%**. Studio One chrome is additional; fit and clicks must still be tested at 1920×1080.
- Content height is intentionally capped at **960 pixels at 150%**, leaving limited room for DAW window chrome and the taskbar on a 1080p monitor.
- Exact approved logo must be imported from `125A-Branding/125A_Logo_Master_FINAL.svg`; no substitute font, redraw, or generated approximation.
- Prefer 125A Knob Designer assets where knobs are needed; step matrix itself uses discrete buttons.
- No GUI drawing/edits from the audio thread; no allocation in processor process().
- Double-click/reset semantics and parameter normalization follow 125A Engineering.
- Every active clickable step requires distinct active/inactive, locked/unlocked, focus/hover and accessible text semantics.

### 100%-coordinate layout (latest 2-page design)

| Region | x | y | w | h |
| --- | ---: | ---: | ---: | ---: |
| Main header / product / zoom | 0 | 0 | 1120 | 70 |
| Page tabs (GUITAR / BASS–CHORDS–DRUMS) | 35 | 75 | 442 | 29 |
| Page viewport (both pages) | 0 | 106 | 1120 | 534 |
| Guitar page: full 16-step panel | 22 | 108 | 1076 | 383 |
| Guitar page: PM / Root / Guitar Gate / Bass Gate / Fill | 22 | 495 | 1076 | 76 |
| Guitar page: GENERATE / CLEAR / VARIATE | 35 | 584 | 735 | 39 |
| Rhythm page: Bass (modes, velocity, EBM style) | 22 | 115 | 1076 | 153 |
| Rhythm page: Chords (ON, quality and ROOT) | 22 | 281 | 1076 | 118 |
| Rhythm page: Drums (GM 3×16, STANDARD/GENERATE/VARIATE/ON) | 22 | 407 | 1076 | 226 |

The pages are **mutually exclusive**; a hidden page may never intercept
header navigation, change musical parameters or leave any stale control
pointers after the host closes/reopens the editor.

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
- The Rhythm tab opens the Bass/Chords/Drums page without modifying the Guitar pattern. The active page is saved in independent versioned Controller state.

### Drums band
- Third VST3 output bus index 2, GM notes 36 Kick, 38 Snare, 42 Closed Hat.
- Rhythm page has 3 × 16 clickable GM step cells, Drums ON/OFF, STANDARD BEAT reset and seeded GENERATE/VARIATE; separate hat 1/8 selector remains an unimplemented product-scope item.
- 48 step host parameters kDrumStepBase..247 + kDrumEnabled=106 are the established event-level route. UI must use their current normalized values and controller edit lifecycle (beginEdit, performEdit, endEdit).
- The GM drum layer remains a 1-bar repeated companion over the whole 16-bar guitar cycle.
- STANDARDBEAT and GENERATE/VARIATE actions must not bypass protected lock states.

### Transport / state / QA acceptance
- Record and reload GUI-visible steps and locks exactly; project load must not display stale state.
- Test 100%/150% zoom, switching Guitar/Rhythm pages, resize/reopen, click-hit areas, host automation, transport loop, play/stop and NoteOff on **all four** event buses (Guitar, Bass, Drums, Chords).
- Current UI has 16 active Guitar step columns. 32-step paging, independent randomization toggles and the rate selector are still missing and cannot be claimed as already implemented.
- GUI can be called PASS only after editor lifecycle tests and actual Studio One verification, beyond the 537 validator assertions.
