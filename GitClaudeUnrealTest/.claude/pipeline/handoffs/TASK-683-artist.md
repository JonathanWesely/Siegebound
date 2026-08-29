# TASK-683 handoff — [WM-A1] the three war-map POI icon textures (art-director, 2026-08-29)

- **Status: COMPLETE.** All three textures authored, imported, verified, and saved. Board flip to
  `ready-for-integration` is the orchestrator's (per dispatch); I did not touch TASKBOARD.
- **Law followed:** `WM-§1` (naming family, white-on-transparent, 128x128, sRGB ON, Texture Group UI,
  display-only boundary untouched — no material, no MI, no WBP edit, no mesh work, and nothing here is
  clickable or nameable). J2 default styles. Zero Meshy/credit spend — authored procedurally (PIL 12.3.0,
  supersampled 512 -> LANCZOS 128).
- **Editor:** was DOWN at session entry (no UnrealEditor process; dispatch's PID 21224 gone). LAUNCHED
  detached per the TASK-061 lesson (`Start-Process`), now **UP as PID 20132** and LEFT UP. MCP verified at
  `http://127.0.0.1:8000/mcp`; **node identity proven before any editor API**: port 8000's owning PID =
  20132 = the editor process itself ("GitClaudeUnrealTest - Unreal Editor"), zero `-game` clients on the box.
- 🔒 **`L_Arena` ledger INTACT:** SHA256 `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58`
  measured after all saves — byte-identical to the dispatch ledger. Editor log carries ZERO
  `Saving Package: /Game/Maps` lines.

## Assets — exact pinned names/paths (the contract TASK-685's soft refs point at)

| asset (`/Game/UI/WarMap/`) | disk `.uasset` | SHA256 (uasset, post-save) | bytes |
|---|---|---|---|
| `T_WarMap_Icon_Mine` | `Content/UI/WarMap/T_WarMap_Icon_Mine.uasset` | `4ac8fcddf08fdec078f6cadaf1e5b73e3bb00c41668b698ab8c986c07fb6a9fc` | 15,339 |
| `T_WarMap_Icon_AncientGround` | `Content/UI/WarMap/T_WarMap_Icon_AncientGround.uasset` | `6a11abc6b88b37629ca2e6cdd48927fdff26c5c8175105530a344db3cbf052cb` | 17,300 |
| `T_WarMap_Icon_Castle` | `Content/UI/WarMap/T_WarMap_Icon_Castle.uasset` | `94859080e0f02e7d1c0f14a49e4ff0d41531b7b2954ebbfac426e0a04b339e03` | 13,123 |

**Raw PNG sources** (`Content/RawAssets/Textures/WarMap/`, new folder, untracked in git at my exit):

| png | SHA256 | bytes |
|---|---|---|
| `T_WarMap_Icon_Mine.png` | `042a570ed6981d16c6041fa5adf6b6232f0dbafba3968bcd0aef8b36976b6efb` | 2,732 |
| `T_WarMap_Icon_AncientGround.png` | `38ff2e25f230359e53a93379d7a154c513e8bbeb1313e8785df7b8b9ebb304bc` | 4,264 |
| `T_WarMap_Icon_Castle.png` | `cb24cfc41f0d033efb9e9c4b801083aab96c770cc0a21f784633dacf80e30c5b` | 920 |

## Import settings — read back per asset via ObjectTools AFTER set, before save (not assumed)

All three identical: **`SRGB = true` · `LODGroup = TEXTUREGROUP_UI` · `CompressionSettings = TC_Default`**
(default compression per the CardArt-precedent law row) · **128x128** confirmed via `TextureTools.get_size`
per asset. Import defaults arrived as `TEXTUREGROUP_World`; only `LODGroup` needed changing.

## Glyphs — one line each + final texel coverage (alpha-derived, measured)

- **Mine — a pickaxe:** crescent head with downward-tapering tips + slim haft, rotated 45 deg. Alpha bbox
  (24,8)-(126,111) of 128^2; opaque coverage **19.2%**.
- **AncientGround — a stone/rune ring:** 8 discrete round monoliths in a circle, top-down, NO connecting
  annulus (a first draft's annulus+studs read as a GEAR at 24 px and was rejected at the eyeball gate).
  Alpha bbox (8,8)-(120,120); opaque coverage **28.5%**.
- **Castle — a keep:** wide crenellated block (3 merlons, shallow gaps) with a small punched gate arch. A
  first draft read as the letter "H" at 24 px (gaps too deep, gate too big) and was reworked. Alpha bbox
  (21,30)-(107,120); opaque coverage **36.6%**.

**24 px legibility:** verified on a rendered 24 px contact sheet over a dark map-tint ground — three
mutually unmistakable silhouettes (diagonal pick / dotted ring / crenellated block). White-on-transparent
discipline: RGB is pure white on EVERY texel including alpha=0 ones, so runtime tinting (685's castle
team-tint via the `W4-R5` accessors) picks up zero fringe color.

## Integration notes for 685/689

- Shape rides in RGB+alpha; **no baked color anywhere** — tint at draw time is safe.
- In-engine spot check: `CaptureAssetImage` on `T_WarMap_Icon_Mine` renders the white glyph over the
  transparency checkerboard (alpha imported correctly, no premultiply fringe). The other two went through
  the byte-identical pipeline and readbacks; their pre-import 128/24 px renders were eyeballed instead —
  final on-map look at draw scale is TASK-690 pixel territory.
- Saves: explicit per-asset `save_assets` calls, one path each, never a save-all. Editor log shows exactly
  these three `Saving Package: /Game/UI/WarMap/...` lines (plus TASK-687's WBP save — see its handoff).

## Fences ledger

⛔ No Source/ edit · no compile · no git commands run (status/diff READS only, for this ledger) · no
TASKBOARD write · no `L_Arena` save · no foreign-WBP save · zero credit spend. **Foreign dirt observed,
NOT touched, for 689:** `Config/DefaultEngine.ini` (M, mtime 12:34:04 — 25 min BEFORE my editor launch at
12:59:21, not my session's) and `Source/.../WarMapWidget.h` (M — the parallel TASK-684 lane), plus
pre-existing TASKBOARD/CONVENTIONS mods and the editor-integration auto-staging of my three new uassets
(`A ` rows; I ran zero git write commands — the known hostile-index behavior, TASK-568 §6 precedent).
