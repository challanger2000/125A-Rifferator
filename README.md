# 125A Rifferator — guitar-only development

**Status: Development / NOT FINAL.** One authoritative 16-step guitar riff generator for Windows VST3. This branch intentionally replaces the previous four-output companion prototype. A successful compile or validator does not establish Studio One or musical acceptance.

## Current product scope

- **One MIDI event output:** Guitar Out (bus 0, MIDI channel 1). No bass, drum or separate chord event outputs; no background EBM engine.
- **16-step guitar matrix:** active/off, STOP, NOISE, PM, SOFT, OPEN/FIFTH, HARMONIC, scale degree, LOCK and DOUBLE.
- **Controls:** PM velocity MIN/MAX (adjust for the *actual* guitar library articulation map), editable ROOT NOTE (E2/G2 etc.), GTR GATE, FILL INTENSITY, editable step degrees, GENERATE / VARIATE / CLEAR.
- **0% gate** suppresses notes; **0% fill** disables fills. STOP is an active NoteOff command; an inactive step is silence, not a STOP.
- **100% and 150% zoom**, one uncluttered page. VST3 parameter IDs for existing guitar functions remain stable.
- **Project migration:** V1–V9 multi-bus states can be read; only the guitar part is restored. New guitar-only state is V10; discarded bass, chords and drums are not written back.

## Development and QA

The git history retains the previous design as the baseline. Current source lives on `develop/v1.0.0` until independently accepted. Follow [125A Engineering START-HERE](https://github.com/challanger2000/125A-Engineering/blob/main/START-HERE.md) and [Guitar-specific host QA](docs/HOST-GUI-QA.md).

CI checks compilation, deterministic scheduler/regressions, state migration, static GUI structure and Steinberg Validator. **Still required:** real Studio One display, note tracing under looping/stop/start, editing all numbers, 20 editor reopen cycles, 125A Plugin Tester, final user sound approval. Never call release-ready without them.

The separate EBM bass instrument is a **future independent project**, not part of this binary.
