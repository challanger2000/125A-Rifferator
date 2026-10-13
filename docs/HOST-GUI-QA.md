# 125A Rifferator — guitar-only Studio One acceptance

Status: **OPEN — not yet host-tested**. The successful four-bus CI #38299593419 is not evidence for this refactor.

Record exact commit, workflow run, Windows display scale, Studio One version, plugin bundle path, sample instrument and configured velocity articulation ranges.

1. Check one `Guitar Out` event bus and no Bass/Drums/Chords. Open the real editor at 100% and 150%, capture images, verify 16-step rows and every control geometry/hitbox.
2. Verify the per-step NOTE LENGTH buttons show 1/16 and 1/8, can be toggled at both zoom levels, and persist after save/reopen. Manually set 1/8 at step 1, OFF at step 2, then 1/16 at 3 and 4. Compare with matching piano-roll notes; check gates, event offsets, successive identical PM pitch NoteOff behavior, crossing bar boundaries and STOP. Also test 1/8 + DOUBLE separately.
3. Click and type PM MIN/MAX, ROOT NOTE E2/F#2/G2, GTR GATE 1%, FILL 0% and 100%, and per-step degrees incl. locked step. Confirm value, actual MIDI result, host automation and project restore. Check invalid inputs, Ctrl+Click reset where supported.
3. Run 16th-note PM continuously at 120–160 BPM for hundreds of bars; verify emitted velocities remain in the configured instrument's *actual* PM band, NoteOn/NoteOff parity and no shortened ghost triggers.
4. Manually draw a *short* PM gallop with STEPs 1/3/4 ON and 2 OFF, and immediately a different phrase afterward. Verify this happens only where programmed; no global GALLOP button. In Heavier7Strings AUTO, compare a single PM with one step's DOUBLE: two hits should be half-step apart, both within the configured PM MIN/MAX range, including a narrow equal MIN/MAX value. No forced Kontakt velocity and no CC40. Also test locked steps, OPEN, active STOP vs inactive OFF, GENERATE/VARIATE/CLEAR, and live root/gate changes.
5. Stop → restart → loop repeatedly → loop OFF → play to end → stop; seek/jump, automation and varying DAW buffer sizes. Trace active note state and ensure no stuck notes or accidental PM re-articulation.
6. Save and reopen V10 project; import an older multi-bus V9 project. Guitar settings must persist, obsolete companion notes never emitted.
7. Close/reopen editor 20 times at both zoom factors. Then complete 125A Plugin Tester editor lifecycle, I/O events, host torture, state/automation and Steinberg Validator.

Record each gate independently as PASS/FAIL/NOT VERIFIED. A green build is not GUI acceptance.
