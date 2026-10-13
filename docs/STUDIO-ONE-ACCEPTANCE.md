# 125A Rifferator V1 — Studio One acceptance (NOT EXECUTED)

The current Windows bundle is experimental and not a final release. This is a human host QA checklist, not a pass claim.

## Initial smoke and lifecycle
- [ ] Studio One discovers "125A Rifferator" from the **complete** .vst3 directory, not an isolated DLL.
- [ ] Editor opens at 100% as a fully rendered 1160×570 panel (not white/black/cropped).
- [ ] Toggle Zoom 150% → actual 1740×855 editor with correctly aligned clicks, then back to 100%. Confirm zoom is retained after project save/reload.
- [ ] Editor close/reopen 20 times without host freeze, ghost window or resource crash.
- [ ] 16×5 matrix exactly one BAR at a time, BAR prev/next, STEP ON and LOCK effective; pressing near borders must not activate neighbors.

## Musical behavior using Heavier7Strings
- [ ] Enable ONE KEY FIFTH, VELOCITY MUTING, picking AUTO on Heavier7Strings. Route Rifferator Guitar Out to Heavier7Strings input.
- [ ] PALM MUTE 30, OPEN 115, NOISE 20, PINCH 127 actually trigger their intended H7 playing articulations. STOP sends only NoteOff.
- [ ] Empty 1/16 steps HOLD previous note until next ON / STOP. No extra hits or spurious voices in looping.
- [ ] 1/2/4/8 bars loop at correct **absolute** PPQ in DAW transport; test different block sizes and seeking.
- [ ] MIDI NOTE trigger starts exactly on NoteOn and releases/stops on the matching NoteOff. Test stop, restart and mismatching NoteOff.
- [ ] KEY/SCALE/ROOT NOTE update scale notes visibly; cell titles are musical notes; SHIFT+left-click opens note picker and selects a scale member.
- [ ] FILL 0% leaves pitches unchanged; higher FILL alters **only pitch choices**, never number of attacks; LOCK survives GENERATE, VARIATE and CLEAR.

## Export and persistence
- [ ] EXPORT MIDI opens the OS save dialog, writes a valid .mid and imports into Studio One at the same rhythmic positions and articulations.
- [ ] Save/reopen a Studio One project; compare all bars, active cells, locks, key, root, scale, fill, PM velocity, trigger mode and 100/150% zoom.
- [ ] Automation of bars/global/step parameters updates the actual MIDI output and GUI consistently.

**Release gate:** Steinberg Validator PASS and five isolated CTest passes do **not** substitute for this checklist, Editor Lifecycle Probe, and 125A Plugin Tester. No FINAL before positive user approval.

### Automated host probes (pending)
The separate 125A Plugin Tester repository is private and inaccessible to the Rifferator workflow token. The clean V1 now builds an original Steinberg-SDK host lifecycle harness: fresh-instance component/controller state transfer and 20 native Windows editor attach/detach cycles. I/O and event torture from the full 125A Plugin Tester are still NOT VERIFIED. Do not count its inaccessible private workflow as a PASS.
