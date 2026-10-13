# VST3 integration (experimental, no VSTGUI)
Steinberg SDK v3.8.1_build_84. 1 MIDI event input, 1 MIDI event output, no audio processing. Processor uses host PPQ / NoteOn-NoteOff trigger with bounded core runtime, source-state codec and host parameter queues. Controller exposes global 100–106, plus ON 1000–1127, STOP 1200–1327, TYPE 1400–1527, DEGREE 1600–1727, LOCK 1800–1927 (647 parameters total). The editor is **not implemented**. No UI or host claim until VSTGUI and Studio One acceptance. Parameter queues apply last value per block (not sample-accurate automation), and further event-bus/lifecycle tests are required. Old Rifferator code remains untouched.

Windows compile first attempt [#38391923331](https://github.com/challanger2000/125A-Rifferator/actions/runs/38391923331) failed specifically because a125::riff::Event shadowed Steinberg::Vst::Event at MIDI input/output. The fix fully qualifies SDK event types. All other observed compiler diagnostic errors were consequential. The next targeted Windows build must establish whether linking and runtime packaging work. New normalized parameter-ID regression exercises all 128 × 5 step controls and locking.

## Windows native build evidence (2026-10-13)
Source `ba3207cd97cb9c0f64f07bd16cbc6e7e804cf4ec`: [GitHub Actions #38392621834](https://github.com/challanger2000/125A-Rifferator/actions/runs/38392621834) **Windows x64 Release VST3 LINK PASS, 4/4 CTest PASS, artifact ZIP uploaded (ID 11809317427, 398141 bytes)**. This confirms that the built plug-in module exists, not that it loads/sends events in Studio One. Steinberg Validator, Editor Lifecycle, host-parameter automation and Studio One still OPEN. The next CI stage adds independent Steinberg Validator execution and branded package integrity checks.

## Latest native VST3 verification (2026-10-13)

- Branch: rebuild/v1.0.0-clean; source commit: e4804275e26c0b320d2086763169c847b5a4581e.
- [Windows SDK/Validator CI #38393090916](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916): full workflow SUCCESS.
- Windows 2022 / MSVC Release: real Windows x64 VST3 compiled and linked: BUILDS PASS.
- Four independent Windows Release CTest targets: **4/4 PASS** (assertions kept enabled).
- Independent Steinberg VST3 Validator against staged, branded native bundle: **537 tests passed, 0 failed**.
- Verified experimental artifact: [Rifferator-V1-EXPERIMENTAL-Windows-x64](https://github.com/challanger2000/125A-Rifferator/actions/runs/38393090916/artifacts/11810467004). Artifact ID 11810467004; ZIP 398192 bytes; SHA256 b7239140e59ccb9608b8863069c74888f83897f211838376b4db25408946789c.
- **NOT VERIFIED / NOT IMPLEMENTED:** VSTGUI editor/zoom/inputs, full Controller-to-GUI traceability, actual Studio One MIDI routing, real host automation/state recall, user musical acceptance, 125A Plugin Tester. No FINAL packaging or claim.
