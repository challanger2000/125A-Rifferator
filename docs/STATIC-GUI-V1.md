# Static UI design contract (VSTGUI NOT YET BUILT)

Read and use authoritative sources: challanger2000/125A-Branding/125A_Logo_Master_FINAL.svg; approved 125A-Knob-Designer reusable controls. Never redraw logo or substitute old bitmap.

Canvas **1160×570 at 100% / 1740×855 at 150%**, leaving practical headroom on a 1920×1080 display for Studio One chrome. Actual host window size and UI scale remain NOT VERIFIED.

Coordinates (left, top, width, height):
- Header/logo/title/Zoom: (24,12,1112,62)
- BAR COUNT / KEY / SCALE / ROOT NOTE / PM VELOCITY / FILL: (24,92,1112,72)
- BAR NAVIGATION / step numbers 1..16: (24,174,1112,43)
- Articulation labels: (24,224,144,240)
- 16×5 matrix (pitch/velocity top to bottom): (180,224,800,240)
  - Column i: x=180+50*i; 48px cell width, 2px gap.
  - Row j: y=224+48*j; 46px cell height, 2px gap.
  - Rows: PINCH, OPEN, PALM MUTE, NOISE, STOP (STOP separate from velocity ranking).
- Right side STEP ON, LOCK and scale-note legend: (992,224,144,240)
- Actions GENERATE / VARIATE / CLEAR / EXPORT: (24,488,1112,58)

One bar visible at once. Show pitch name in activated note cells (not STOP). SHIFT+left-click enters in-cell diatonic note selector; regular left click handles articulation. STEP ON and LOCK are separate. Empty steps must be visually distinct from STOP. Min interactive hit area 40×40 at 100%. Audit CViewContainer nested bounds and hit overlap recursively, each button's on/off visual state, zoom host resize and 20-cycle editor lifecycle.

This file is the **geometry contract**, not a claimed finished static mockup. Complete branded static render using authoritative master assets before implementing VSTGUI.

## Actual VSTGUI implementation stage
Interactive RifferatorPanel maps the previously specified 16×5 cells to one on-screen bar and dispatches internally by fixed geometry. Standalone geometry contract covers all cells, 40-pixel-plus hitboxes on global actions, BAR and STEP ON/LOCK, and note-picker overlay. Original branding master is copied unmodified to resource/ and rendered by the separately verified High Gain Guitar Finisher vector-logo view. VSTGUI output, Studio One hit coordinates and lifecycle remain NOT VERIFIED until Windows build and host test.

## Picker layout repair (2026-10-13)
The earlier 15-column overlay at x=180..983, y=422..467 concealed STOP cells. Now the 15 diatonic choices temporarily replace only the right-hand detail panel at x=992..1136, y=224..464 in a 3-column × 5-row arrangement. Every one of the 16 STOP hitboxes remains visible/clickable while note selection is open. Fixed layout, no new controls or audio behavior.
