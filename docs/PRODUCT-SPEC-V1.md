# 125A Rifferator V1 — binding specification (2026-10-13)

This is a **new product codebase**, not a patch to the old develop/v1.0.0 implementation. Approved scope:

- Heavier7Strings with **ONE KEY FIFTH**, **VEL. MUTING** and pick direction **AUTO**. One MIDI note triggers the instrument's own powerchord, with no CC40 and no hand-built fifth.
- Exactly 1, 2, 4 or 8 bars; 16 sixteenth steps per 4/4 bar; exactly **one selected BAR** shown in GUI. Absolute host PPQ for transport. Alternative MIDI NOTE mode: NoteOn starts, matching NoteOff ends the riff, with proper release/panic on stop/seek.
- Articulations (descending velocity): PINCH/HARMONICS 127, OPEN POWERCHORD 115, PALM MUTE configurable 30..83 (default 30), NOISE 20; STOP (release only, not a note). Supported H7 muting bands documented in its user manual sections 6.1, 6.4, 6.5. H7 settings must be checked in real instrument.
- **Every note lasts until the next active note or STOP**. Empty step is a hold, not STOP. No DOUBLE, GALLOP, GATE or per-step LENGTH controls. One-cycle MIDI export terminates the last note at the clip end to avoid a hanging note.
- KEY, SCALE, ROOT NOTE; root must lie within selected scale, relative degrees are always diatonic. Show scale notes and names in active articulation cells. SHIFT+left-click on a cell opens the note selector. Do not rely on right-click, double-click or wheel.
- STEP ON and LOCK per step; locked cells protected through Generate, Variate, Clear and Fill.
- PM VELOCITY, FILL INTENSITY 0–100%, GENERATE PATTERN, VARIATE, CLEAR PATTERN, EXPORT MIDI. **FILL changes only pitches on already active steps, never attack count.** MIDI export is standard SMF format 0, a single track, one channel, 960 PPQ; controller handles save dialog, never audio callback.
- GUI must be spacious, mathematically laid out, stable at 100/150%; static design **before** VSTGUI. Proven working zoom approach inspected in current High Gain Guitar Finisher: VSTGUI::VST3Editor, setAllowedZoomFactors 1.0/1.5, setZoomFactor, controller state persistence, willClose invalidation.
- Generic rhythm/PPQ core independent of Heavier7Strings velocity profile; designed to be reusable for a **separate future** EBM bass instrument, without mixing it into Rifferator.

**Development stage:** isolated C++ core only. Only Major and Natural Minor are in this first core. Precise final scale-choice UI and host behavior must be validated; do not present these as already host-integrated.
