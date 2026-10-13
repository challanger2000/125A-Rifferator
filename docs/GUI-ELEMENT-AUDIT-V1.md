# 125A Rifferator V1.0.0 — Elementweiser GUI-/MIDI-Audit

**Codebasis:** `rebuild/v1.0.0-clean` @ `8b70827f` (Source); **Status: IN PRÜFUNG — NICHT FINAL**.

Dieses Verzeichnis ist die explizite Kontrolle jedes verfügbaren **Bedienelement-Typs**. Die 5×16 sichtbaren Step-Zellen und alle 128 möglichen Steps werden durch Parameterbereiche und Schleifentests erfasst; eine einzelne exemplarische Koordinate ist kein vollständiger Maustest. **Build/Validator grün ≠ GUI-Maus-PASS in Studio One.**

Legende: `QUELL` = Code und Bindung nachvollzogen, `TEST` = deterministische isolierte Regression, `HOST` = tatsächliche VST3 IComponent/IAudioProcessor Event-/Parameter-Testprobe, `DAW` = beobachteter Studio-One-Test (bei früherem Build separat gekennzeichnet), `OFFEN` = Kette nicht vollständig nachgewiesen.

## Kontrollinventar — exakt alle interaktiven Target-Typen in UiGeometry.hpp

| Element | Eingabe, Parameter-/Controllerweg und Standardwert | Wirkung/Kontrollnachweis | Noch offen |
|---|---|---|---|
| ZOOM 100/150 | Klick → Controller Zoom / Editor Size; Strg-Klick → 100; Controller State RGU2 | QUELL, native HOST 20× Editor und 150-%-State | DAW Klickkoordinaten 100/150, Resize/Reopen beim aktuellen Build |
| PATTERN 1/2/4/8 | Klick → VSTGUI-Auswahl, `Bars=103`, Strg → 1 Takt | TEST 4-/8-Takt-Transport, 1–8 Zustands-Roundtrip; HOST State | DAW Auswahlmenü inkl. Abbruch und Änderung beim Spielen |
| KEY (12) | Klick → Auswahl C…B, `Key=100`; ggf. mitgezogener `Root=102`; Strg → E | HOST Automation Key E→F# mit Root, TEST Tonart-Pitch | DAW jede Auswahlebene, Transposition + Anzeige |
| SCALE (2) | Klick → Major/Natural Minor, `ScaleId=101`, Strg → Natural Minor | QUELL, TEST Tonleiter-/Root-Snap-Regeln | DAW Popup und tonal passende MIDI-Ausgabe |
| ROOT NOTE | Klick → nach Oktave gruppierte in-scale MIDI-Pitches `Root=102`; Strg → Tonika in Oktave 2 (Werkseinstellung E2) | TEST G9/MIDI127 → E2/MIDI40, HOST Root-Snap/State | DAW Submenüs, Cancel/Zoom, gespieltes Gitarrenregister/Extremwerte |
| PM VELOCITY | Horizontaler Klick (30–83), `Pm=104`, Strg →30 | TEST Endpunkt/Monotonie; HOST MIDI 65 und 30, reale NoteOn-Velocity | DAW Eingabegenauigkeit, volle Werteabdeckung, visuelle Skala |
| FILL INTENSITY | Horizontaler Klick (0–100 %), `Fill=105`, Strg →0 | QUELL, TEST Fill verändert nur Pitch, nie Anzahl Anschläge; LOCK geschützt | DAW 0/25/50/75/100 und Variate/Generate klanglich |
| BAR ◀ | Linksklick → lokaler BAR-Index minus 1, modulo Pattern-Länge | QUELL, Controller RGU2 BAR-State | DAW Umbruch 1↔N und sichtbare Step-Zuordnung |
| BAR ▶ | Linksklick → lokaler BAR-Index plus 1, modulo Pattern-Länge | QUELL, Controller RGU2 BAR-State | DAW Umbruch N↔1 und sichtbare Step-Zuordnung |
| 5×16 Step-Raster | Direktklick → `publishPattern`, ON/STOP/TYPE, 16 Steps je angezeigtem Takt | TEST aller 5 Zeilen auf allen aktiven Steps bei 1/2/4/8 Takten (bis zu 128), LOCK | DAW 80 Klickflächen/Zoom; aktuelle neue STOP-Umschaltung |
| PINCH/HARMONICS-Zeile | Raster Zeile 0 → TYPE=3, ON=1; erneut → ON=0; Velocity 127 | TEST `compileOneCycle`, QUELL Controller→Parameter→Processor | Heavier7Strings tatsächlich hörbar, aktuelle DAW |
| OPEN POWERCHORD-Zeile | Raster Zeile 1 → TYPE=2, ON=1; erneut → OFF; Velocity 115 | TEST MIDI-Note; HOST Velocity 115; DAW ältere Version Teilprüfung | DAW aktueller Build, H7 ONE KEY FIFTH |
| PALM MUTE-Zeile | Raster Zeile 2 → TYPE=0, ON=1; erneut → OFF; Velocity 30–83 | TEST MIDI-Note 30; HOST Velocity 65 | DAW aktueller Build, alle PM-Bereiche |
| NOISE-Zeile | Raster Zeile 3 → TYPE=1, ON=1; erneut → OFF; Velocity 20 | TEST MIDI-Note 20 | DAW H7 Noise-Artikulation |
| STOP-Zeile | Raster Zeile 4 → STOP=1+ON=1, erneut → beides 0; kein NoteOn | TEST STOP an/aus; HOST MIDI-Release bereits vorher geprüft | DAW reale STOP-Doppelklicks, kein Hänger |
| Shift+Raster → Notenpicker (15 Felder) | Wahl ±7 diatonische Schritte → `Degree=1600+step`, `On=1000+step` | TEST 15 Rechtecke; alte DAW Version Note G2 nach Auswahl | DAW aktuelle Picker-Position; Abbruch und Grenznoten |
| STEP ON | Klick toggelt `On=1000+step`; Strg → OFF (falls nicht LOCK) | QUELL und HOST Locked-Edit-Ablehnung | DAW Reset/Grenzen und genaues UI-Feedback |
| LOCK | Klick toggelt `Lock=1800+step`; Strg → unlock | TEST LOCK gegen Generate/Clear/Cell-Edit; HOST Locked-Edit-Ablehnung | DAW aktuellen Stand inkl. GUI-Rückmeldung |
| TRANSPORT / MIDI NOTE | Klick toggelt `Mode=106`; Strg → Transport | HOST NoteOn/NoteOff-Trigger samplegenau und TRANSPORT-STOP | DAW Trigger-Modus/Matching NoteOff |
| GENERATE PATTERN | Klick → `heavier::generate` → ON/STOP/TYPE/DEGREE pro Step, RNG RGU2 | TEST Pattern/LOCK, ältere DAW Generation | DAW aktuelle Version + Reload deterministisch |
| VARIATE | Klick → `heavier::variate`, respektiert LOCK und FILL | TEST/QUELL Generatorlogik und MIDI | DAW aktuelle Version inkl. 0 % FILL |
| CLEAR PATTERN | Klick → `pattern.clear`, nur nicht-LOCK; Step-IDs aktualisiert | TEST LOCK/CLEAR, ältere DAW Teilprüfung | DAW aktuelle Version, keine stehenbleibende MIDI-Note |
| EXPORT MIDI | Klick → modaler Save-Dialog → SMF0 PPQ960-Datei | TEST MIDI-Konformität, ältere DAW Export/Import | DAW aktueller Dialog/Dateizugriff/Schließen |
| Strg+Linksklick auf Raster | Reset aller Felder eines **nicht gesperrten** Steps auf neutralen `Step{}`-Standard; gesperrt bleibt unverändert | TEST (alle möglichen Bargrößen), QUELL `publishPattern` | DAW Reset und LOCK-Rückmeldung |
| Strg+Linksklick auf nicht zurücksetzbare Aktionen | Die anderen Befehle (Prev/Next/Generate/Variate/Clear/Export/Picker) müssen normal funktionieren, nicht still verworfen werden | TEST vollständige Klassifikation `supportsReset`; QUELL bedingter Handler | Echte DAW-Klicks mit Strg |

## Nicht durch Build ersetzbare Freigabe-Gates

1. **Native GUI-Menüs und Mausaktionen in Studio One:** alle Auswahlmenüs inkl. Root-Submenüs, Cancel, 100/150% und reale Bedienung der zurücksetzbaren Controls; nicht Teil des bisherigen 20×-Editor-Lifecycle-Probes.
2. **Bidirektionale VST3-Kette:** pro Aktion native Maus → Controller Begin/Perform/End Edit → Processor Parameter → tatsächliche MIDI-Ausgabe → GUI-Anzeige, dazu Projekt speichern/neu laden.
3. **Heavier7Strings-Hörabnahme:** alle vier Articulations/Velocity-Mappings inkl. 20 Noise und 127 Harmonics.
4. **Offizieller 125A Plugin Tester (JETZT BESTANDEN):** Version 0.2.9, Windows [run #38436972585](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585) auf exakt `8b70827f`: **30 PASS / 0 WARNING / 0 FAIL**. Das ist die echte private Testsuite; Audioausgabe-Torture ist für ein reines MIDI-Plugin nicht anwendbar. [Originalberichte](https://github.com/challanger2000/125A-Plugin-Tester/actions/runs/38436972585/artifacts/11833722284). Das ersetzt **nicht** die ausstehenden Studio-One-Mausklick- und musikalischen Tests.

**Keine Final-Freigabe, solange ein Pflichtpfad OFFEN ist.** Die neue Policy-Regression prüft Steuerlogik/Output, aber keine echten Windows-Mausereignisse oder nativen VSTGUI-Popup-Callbacks.

## 2026-10-13 Benutzerfeedback — PM/FILL, Notenpicker, TRANSPORT/MIDI NOTE (CI OFFEN)
Der aktuelle Studio-One-Screenshot zeigt: kaum erkennbaren 5-Pixel-Pegelbalken bei PM/FILL ohne Drag-Geste; einen Notenpicker ohne sichtbare Schließen-Funktion; einen unterhalb der Aktionsleiste stehenden Shift-Hinweis und den gedrängten MODE-Schalter im rechten Step-Detailbereich. Neue Implementierung: breite 12-Pixel-Slider mit Thumb, Click-Drag über den vollständigen 55-Pixel hohen Feldbereich und VST3 beginEdit→performEdit→endEdit, Mausrad ±1 pro Scroll-Ereignis und Ctrl-Reset. Der Picker erhält einen sichtbaren roten X-Button, Escape und Klick außerhalb als Abbrechen **ohne Pattern-Änderung**. MODE liegt nun in der oberen BAR-Leiste bei (992,177,144,40). Der Shift-Hinweis sitzt vollständig innerhalb der unteren Statusleiste. Die geometrischen Regressionen prüfen alle 54/101 numerischen Werte, alle Picker-Hitbereiche, Abbrechen und die neue MODE-Position. Echte VSTGUI-Maus-, Mausrad- und Escape-Bedienung im Host ist vor CI/DAW-Prüfung NICHT BESTANDEN.
