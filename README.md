# 125A RIFFERATOR

**THE INDUSTRIAL RIFF MACHINE**

Development V1.0.0 on `develop/v1.0.0` — guitar-first Windows VST3 MIDI generator with **four separate event outputs** (Guitar, Bass, Drums, Chords), each using MIDI channel 1.

- **GUITAR:** 16-step articulation matrix, live root MIDI, guitar/bass gate and fill-intensity controls, per-step pitch/LOCK; seeded GENERATE and conservative VARIATE (30% mutation) on the controller thread. 100%/150% GUI zoom.
- **BASS:** FOLLOW, EBM and HYBRID modes, plus direct MIDI velocity, gate and EBM style controls.
- **DRUMS:** Editable 16-step GM Kick 36 / Snare 38 / Closed Hat 42, with an explicit Drums ON/OFF switch.
- **CHORDS:** Guitar-following output with chord-quality and root-only modes.

## Development and verification status

The Windows CI compiles the plugin, runs deterministic core/GUI-layout regression tests and the Steinberg validator. A green CI run does **not** establish a host-attached GUI lifecycle PASS or visual approval.

The earlier standalone editor create/release test was **removed from CTest** because it produced a segmentation fault outside a correctly initialized native VST3 host. It is **not** evidence of a verified GUI lifecycle and must not be restored as an equivalent test.

**Real host acceptance protocol:** [Studio One GUI/lifecycle QA](docs/HOST-GUI-QA.md).

Development build artifacts are attached to successful GitHub Actions runs. The latest independently confirmed CI baseline is [#38260745327](https://github.com/challanger2000/125A-Rifferator/actions/runs/38260745327), validating seeded GENERATE/VARIATE. The additional seven GUI control groups are **not verified by CI yet**. Do not ship a Final or Gumroad package before the Studio One host protocol, agreed V1 scope and 125A release QA pass.

See [bass integration contract](docs/BASS-INTEGRATION-CONTRACT.md).
