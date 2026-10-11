# 125A Rifferator — Studio One host/editor QA (V1 development)

Status: **NOT VERIFIED IN HOST** until a real Windows Studio One session completes every gate below. Steinberg validator and headless CTest do not render or attach an editor.

## Test build

Use the Windows x64 VST3 artifact attached to the latest successful `Rifferator V1 controlled verification` workflow on `develop/v1.0.0`. Do not install a stale artifact or the removed hostless test executable.

## Host test procedure

1. Close Studio One, install the development `Rifferator.vst3` bundle in the Windows VST3 folder, launch Studio One and rescan. Verify exactly one **125A Rifferator** instance.
2. Open the editor. Confirm no crash, clipped controls, overlapping labels or missing 16x6 Guitar grid, 16 pitch controls, 16 locks, Bass, Chords and three Drum rows.
3. Change guitar step 1 between REST, PALM MUTE and OPEN. Verify exactly one selected articulation per column, REST disables it, selecting PM/OPEN reenables it and produces MIDI events on Guitar output.
4. Set LOCK for two steps; invoke GENERATE repeatedly. Verify locked articulation/degree/enable never change; unlocked steps change; active highlights always match the resulting state.
5. Change Bass mode FOLLOW → EBM → HYBRID, Bass ON/OFF; check selection indicators and live bus-2 MIDI output. Set Chords enable, quality MINOR/MAJOR/POWER/SUS2/SUS4 and CHORD/ROOT; check fourth MIDI output.
6. Confirm all three Drum GM rows (36/38/42) toggle and remain synchronized when transport stops, restarts and loops.
7. Click 100% and 150% zoom. Verify labels and controls remain readable and clickable and editor does not extend beyond the usable screen; 150% nominal content 1680×1043 px plus host chrome.
8. Close and reopen the editor at least **20 times**. After each reopen, verify zoom, highlighted states, and clicking PM and Bass/Chords modes. Report any black/blank editor, freeze or lost control response.
9. Duplicate the plugin editor only if the host allows two editors for the same plugin instance; confirm the first editor still works after second closes.
10. Save the Studio One song with deliberately nondefault Guitar, Bass, Drums and Chords settings; quit and relaunch. Compare every visible state, MIDI output, zoom and locked-step behavior after song restore.
11. Exercise Studio One parameter automation for Bass Mode, chord quality and Guitar Articulation; confirm all visual states update without touching the GUI.
12. Test Stop, loop, loop-off, seek and song close while notes are held. There must be no hanging notes, host errors or residual MIDI events.

## Record results

For each gate record PASS / FAIL / NOT TESTED, Studio One version, Windows scaling, 100/150% zoom, plugin artifact run ID, screenshots at each zoom and precise steps to reproduce a failure.

**Freigabe:** Requires all host gates PASS **and** independent visual approval. Build/Validator successes are necessary, not sufficient. Do not label this a completed automated editor lifecycle test.
