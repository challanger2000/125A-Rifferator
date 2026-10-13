# Guitar-only GUI implementation contract

Status: source redesign, DAW acceptance pending.

One editor `1120×640`, zoom 100/150%. No page tabs. Complete 16-column matrix includes STEP ON, STOP, NOISE, PALM MUTE, SOFT, OPEN/FIFTH, HARMONIC, editable DEGREE, DOUBLE and LOCK. Separate panel for PM MIN/MAX, ROOT NOTE, GTR GATE and FILL INTENSITY; GENERATE, CLEAR, VARIATE remain visible.

**Numeric values are keyboard editable** using `CTextEdit`, not read-only buttons:
- PM MIN/MAX: 1..127, constrained to ordered interval.
- ROOT NOTE: note names or MIDI numbers, C4=60.
- GTR GATE/FILL: 0..100%.
- Per-step degree: -13..15 (locked steps reject edits).
All edits issue VST3 beginEdit/set/performEdit/endEdit; DAW automation updates the text. Invalid text restores the last valid value. Rendered controls must be tested at both zoom levels in Studio One.

Theme/logo policy: use authoritative 125A-Branding master, never redraw the logo. Rifferator's prior title-only product header does not justify substituting a pseudo-logo.

**No host GUI PASS** until repeated native editor attachment, click hitboxes, keyboard editing and reopening in Studio One are tested.
