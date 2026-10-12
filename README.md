# 125A RIFFERATOR

**THE INDUSTRIAL RIFF MACHINE**

Development V1.0.0 on `develop/v1.0.0` — guitar-first Windows VST3 MIDI generator with **four separate event outputs** (Guitar, Bass, Drums, Chords), each using MIDI channel 1.

- **GUITAR:** 16-step articulation matrix, live pitch/step controls and per-step LOCK; seeded GENERATE and conservative VARIATE (30% mutation) commands run on the controller thread. 100%/150% GUI zoom.
- **BASS:** FOLLOW, EBM and HYBRID modes with enable controls.
- **DRUMS:** Editable 16-step GM Kick 36 / Snare 38 / Closed Hat 42.
- **CHORDS:** Guitar-following output with chord-quality and root-only modes.

## Development and verification status

The Windows CI compiles the plugin, runs deterministic core/GUI-layout regression tests and the Steinberg validator. A green CI run does **not** establish a host-attached GUI lifecycle PASS or visual approval.

The earlier standalone editor create/release test was **removed from CTest** because it produced a segmentation fault outside a correctly initialized native VST3 host. It is **not** evidence of a verified GUI lifecycle and must not be restored as an equivalent test.

**Real host acceptance protocol:** [Studio One GUI/lifecycle QA](docs/HOST-GUI-QA.md).

Development build artifacts are attached to successful GitHub Actions runs. The latest confirmed baseline before GUI-generation integration is #38257386913 (29/29 CTest, 537/537 Steinberg validator). The new GENERATE/VARIATE command wiring is **not verified by CI yet**. Do not ship a Final or Gumroad package before the host protocol, scope agreement and 125A release QA pass.

See [bass integration contract](docs/BASS-INTEGRATION-CONTRACT.md).
