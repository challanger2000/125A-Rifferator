# 125A Rifferator V1 — Studio One acceptance (PARTIALLY VERIFIED)

The current Windows bundle is experimental and not a final release. This is a human host QA checklist, not a pass claim.

## Studio One observations (user-confirmed, 2026-10-13)

The previous "NOT EXECUTED" heading referred to the original untested checklist. The user has independently confirmed these **specific** observations in Studio One:

- VST3 discovered; branded GUI rendered; 150% zoom worked without visible clipping.
- GENERATE PATTERN populated the correct visible 16-step bar. OPEN POWERCHORD row editing and SHIFT+left-click note selection changed Step 4 to G2.
- LOCK protected Step 4 from manual edits, subsequent GENERATE, and CLEAR. Unlocked steps were cleared.
- EXPORT MIDI wrote a valid SMF, and the exported one-note riff imported into Studio One at Step 4 with its expected duration.
- Direct VST3 MIDI routing to Heavier7Strings produced audible, individually rhythmic attacks (not merely an exported file). Playback stopped on transport STOP without a hanging note.
- Two distinct bars were audible in a 2-bar configuration. Exact wrap after bar 2 was **not** explicitly confirmed.

Not yet user-confirmed: 4/8 bar host playback, 20× host editor lifecycle, actual Studio One project recall, host-sample-accurate automation, all four H7 articulation velocity mappings, repeated MIDI trigger and transport seeking, 125A Plugin Tester full QA. None is silently promoted to PASS by source-level testing.

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

## Follow-up automated QA (2026-10-13)
[Windows run #38414680086](https://github.com/challanger2000/125A-Rifferator/actions/runs/38414680086) confirmed 6/6 native CTest, full 537/537 Steinberg Validator, 20 HWND editor cycles, and non-default 8-bar Processor and Controller state roundtrip including 150% zoom, BAR 8, selected locked notes, trigger mode and PM velocity. The modal pitch picker no longer geometrically overlays STOP, proven by deterministic hitbox tests; its appearance and clicking in actual Studio One remain unverified. This evidence supplements, but does **not** replace, the outstanding Studio One and 125A Plugin Tester release requirements.
 
## Windows event/automation host verification

The [native Windows VST3 QA #38417386319](https://github.com/challanger2000/125A-Rifferator/actions/runs/38417386319) additionally passed actual ProcessData MIDI output and parameter-queue automation: PM65/OPEN115, STOP and transport NoteOff, KEY/ROOT transposition, no unnecessary reattack for unchanged ON, and NoteOn/NoteOff trigger sample offsets. This does not claim a Studio One-specific automation pass or full 125A Plugin Tester approval.

## Controller state v2 regression gate (pending)
New RGU2 Controller state saves zoom, selected BAR and GENERATE/VARIATE RNG seed; old RGU1 zoom+BAR state is still accepted. Native SDK host harness checks invalid/truncated state cannot modify an existing correct state, and rejects LOCK-protected host parameter changes without corrupting VST3 parameter readback. Studio One resave/reopen still requires user acceptance.

## Release-audit host data (latest experimental build)
[Windows CI #38424202886](https://github.com/challanger2000/125A-Rifferator/actions/runs/38424202886) confirmed SDK-native RGU1→RGU2 controller state migration (seed + zoom + BAR), 8-bar processor state, locked-step parameter readback, 20 Editor cycles, 44.1/96 kHz varying sample block realtime/offline MIDI balance, 6/6 CTest and 537/537 Validator. The remaining real Studio One actions—install that exact build, verify resave/reopen of all bars/LOCK/seed-related Generate continuity, live host automation and actual H7 articulations—are NOT VERIFIED. Do not mark complete based on SDK-host evidence.

## Revised header UX (2026-10-13, NOT YET HOST-VERIFIED)
Single-click native selection menus now replace repeated-click cycling for KEY / SCALE / PATTERN / ROOT NOTE; ROOT has octave submenus and tonic-octave-2 reset to recover from the topmost MIDI pitches (such as G9). PM and FILL use absolute horizontal value strips (click anywhere in the field). Ctrl + left-click on the header controls resets defined defaults. Verify actual Studio One menus, single-click selection, 100/150% zoom pointer alignment, popup cancellation, ROOT recovery from extreme note, and parameter automation before marking PASS.

## User control regression after reported GUI workflow problems — PENDING
STOP grid cells now toggle ON/OFF with repeated clicks; Ctrl+click of an unlocked cell clears it, Ctrl+STEP ON resets OFF and Ctrl+LOCK unlocks while respecting locked-step invariants. Ctrl+commands no longer become inert. Pure controller-policy and MIDI-event regressions (CI pending) check 5×16 hitbox geometry, 1/2/4/8 bars, all four Heavier7Strings velocity modes and STOP. Actual Studio One cursor/click tests, 100/150% zoom, menu selection/cancellation and project reopen must be performed against the new build before release.

## Official 125A tester technical gate — now PASS, 2026-10-13
[Private run #38436972585](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585) tested exact V1 code `8b70827f` with the real portable Tester v0.2.9: **30 PASS / 0 WARN / 0 FAIL**. [Full diagnostic artifact](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585/artifacts/11833722284). This resolves only the previously open **technical full 125A Tester gate**. The above user-operated Studio One checklist is still separately open: actual new popup menus, STOP off/on, Ctrl default resets, 100/150% hit alignment, saved/loaded DAW project and musical H7 acceptance.

## Aktuelle GUI-Nacharbeit (aus Screenshot 2026-10-13) — NICHT ABGENOMMEN
PM Velocity und Fill Intensity sind jetzt breit sichtbare Slider mit Click+Drag und Mausrad ±1; X, ESC und Außenklick schließen die Shift-Notenauswahl ohne Änderung. Der TRANSPORT/MIDI-NOTE-Modus liegt in der BAR-Leiste, der Shift-Hinweis innerhalb der Statusleiste. Windows-CTest und VST3-Build neu ausführen; anschließend real in Studio One bei 100/150% testen: Drag bis zu Min/Max, Scroll ±1, Reset, Picker X/Escape/Außenklick, Note setzen, MODE umschalten ohne darunterliegende Step-Bedienung. Nur abgenommene Schritte als PASS markieren.

## Current build bef805d — manual Studio One GUI gate
Automated windows build [Windows #38441617503](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503) and official tester [125A Tester #38442103633](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38442103633) both PASS. User should install [Windows VST3 artifact #11836576122](https://github.com/challanger2000/125A-Rifferator/actions/runs/38441617503/artifacts/11836576122) and check in a single short session: (1) drag PM and FILL tracks and nudge by mousewheel ±1 (including Ctrl-click actual default), (2) Shift-click step and cancel note picker via X, ESC and outside click without modifying riff, (3) verify mode switch is at right end of BAR toolbar and toggles TRANSPORT/MIDI NOTE, (4) verify footer hint now inside status region and overall presentation at 100/150%. These are **NOT YET USER-CONFIRMED**; do not mark pass solely from screenshot or Windows CI.
