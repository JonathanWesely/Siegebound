# TASK-687 handoff — [WM-A5] the WBP_WarMap legibility/layout pass (art-director, 2026-08-29)

- **Status: COMPLETE — all three VID-003 defects addressed as slot/size/font property edits on EXISTING
  widgets.** Board flip to `ready-for-integration` is the orchestrator's; I did not touch TASKBOARD.
- **Law followed:** `WM-§4` — ⛔ property edits ONLY. **No widget added / removed / renamed / reparented;
  no binding change; no graph opened; no blueprint compile invoked.** The tree TASK-568's recipe built is
  byte-frozen in shape.
- **The 669 precedent check (verify-the-tree-FIRST) — the tree is REAL, not graph-constructed:**
  `StatusTextBlock` (`/Script/UMG.TextBlock`), `RevealButton` + `CloseButton` (`/Script/UMG.Button`) all
  resolve as `WidgetTree` sub-objects under root `CanvasPanel_0` via ObjectTools reflection; background
  `BackdropBorder` (Border) + labels `RevealLabel`/`CloseLabel` located via the on-disk name-table scan and
  confirmed by reflection reads. Property edits were reachable — **no graph-op lane needed, no stop.**
- **Asset saved:** `/Game/UI/WBP_WarMap` — `is_dirty` true -> explicit single-path `save_assets` ->
  `is_dirty` false. Disk `Content/UI/WBP_WarMap.uasset`, post-save
  SHA256 `6827303b2f5db9db534d94ec304ea949b882e1405ca1e0de072ed797eedc136b` (35,672 bytes; was 33,193).
  The ONLY WBP saved this session. 🔒 `L_Arena` untouched (ledger hash re-measured identical, zero
  `/Game/Maps` save lines — full ledger in TASK-683-artist.md, same session).

## The property table — every change, widget -> property -> old -> new (all values are post-write READBACKS)

| widget / object | property | old | new | VID-003 defect served |
|---|---|---|---|---|
| `StatusTextBlock` | `Font.Size` | 20 | **22** | legibility ("really hard to make out") |
| `StatusTextBlock` | `Font.OutlineSettings.OutlineSize` | 0 | **2** (color already black a=1) | legibility — spec's outline |
| `StatusTextBlock` | `ColorAndOpacity.SpecifiedColor` | (0.95, 0.95, 0.95, 1) | **(1, 1, 1, 1)** | white fill per spec |
| `CanvasPanel_0.CanvasPanelSlot_3` (StatusTextBlock slot) | `LayoutData.Offsets` (pos X, pos Y, size X, size Y) | (0, **-40**, 1200, 104) | (0, **-240**, 1200, 104) | **(a)** status line OFF the deck bar |
| `CanvasPanel_0.CanvasPanelSlot_1` (RevealButton slot) | `LayoutData.Offsets` | (**-176**, 24, **260**, 44) | (**-376**, 24, **340**, 44) | **(b)** label fits chrome · **(c)** off Rally HUD |
| `CanvasPanel_0.CanvasPanelSlot_2` (CloseButton slot) | `LayoutData.Offsets` | (**-24**, 24, 140, 44) | (**-220**, 24, 140, 44) | **(c)** off Rally HUD, pair kept together |
| `RevealLabel` | `Font.Size` | 24 | **20** | **(b)** — the spec's "or reduce its label font" |
| `CloseLabel` | `Font.Size` | 24 | **20** | pair consistency (declared deviation, below) |
| `BackdropBorder` | `BrushColor` | (0.03, 0.04, 0.07, 0.88) | **UNCHANGED (no-op)** | optional darkening: already near-black per the TASK-568 §5.1 recipe — nothing to do, recorded on readback |

Untouched-and-verified riders on `StatusTextBlock`: `AutoWrapText` true · `WrapTextAt` 1180 ·
`Justification` Center · Roboto Bold — the ~150-char TASK-568 sizing note still holds (at 22 Bold the
150-char sentence wraps to 2 lines inside the 104-px, 3-line box). Anchors/alignments of all three slots
unchanged (bottom-center point / top-right point) — only offsets moved.

## The geometry argument (measured off the VID-003 2496x1440 frames, design space = 1080p-high, DPI 1.333)

- **Deck bar band:** top edge ~y 1225 px orig = ~919 design. Old status box bottom edge at -40 = design
  y 1040 -> INSIDE the band (the collision on pixels). New bottom edge at -240 = design y 840 ->
  **~79 design px clear** of the band. The box now sits over the map rect's lower area — backed by the
  dark backdrop (and by 684's dark elevation layer when it lands); C++ paints dots ABOVE children, so a
  dot crossing the text is possible and is the documented cosmetic class (TASK-560 §7), not an overlap
  with other CHROME.
- **Reveal label fit:** "Reveal Enemies (30 Gold)" = 24 chars; at 24 Bold ~322 px vs a 260-px button =
  the measured overflow ("Rev" spilling, "(30 G" truncation). At **20 Bold ~270 px + button padding
  ~294 px inside 340 px** -> ~46 px headroom. Button gap: Reveal right edge -376 vs Close left edge -360
  = 16 px — the pair cannot overlap each other.
- **Rally HUD:** hugs the right corner, ~y 28..101, extending ~160 design px in from the right edge. The
  pair's rightmost point moved from -24 to **-220** -> horizontally clear of the Rally band with 60 px
  margin, still in the top free band (y 24..68, above the map rect top ~130 at 16:9) — chrome stays out
  of the map rect entirely, the TASK-560 corner-anchor principle.

## Deviations declared (SC-§15)

1. **`StatusTextBlock` font 20 -> 22.** 20 already satisfied the ">=20" letter; the bump serves the
   legibility intent the task exists for. Revert = one property write if ruled excess.
2. **`CloseLabel` font 24 -> 20.** The amendment authorizes reducing *Reveal's* label font; I matched
   Close for pair consistency (mismatched 20/24 label sizes on adjacent buttons would read as a defect at
   the next playtest). Same one-write revert if ruled out of scope.
3. **Backdrop tint no-op** — recorded as a decision, not silently skipped.

## What ONLY pixels can verify (for TASK-689's editor pass and TASK-690's eye)

- The rendered fit of the Reveal label inside its 340-px chrome (my fit figure is a font-metric estimate,
  not a Slate text measurement).
- That -240 actually clears the deck bar on the live layout at every resolution/UI scale (the band was
  measured off ONE 2496x1440 clip; the deck bar is a different widget's layout I cannot read from here).
- Outline-2 rendering quality at 22 Bold, and the status line's contrast once 684's elevation texture
  paints under it.
- Runtime repaint health generally — the corruption class the tree-freeze law guards against manifests
  only at runtime. Nothing here touched the tree shape, but the proof is pixels, not readbacks.
- No PIE was started, no viewport was driven, no input lane exists — I claim NOTHING about the running
  game's look.
