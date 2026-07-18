# TASK-190 — Handoff (art-director): CrystalTower emissive glow pass

**Date:** 2026-07-17
**Author:** art-director
**Status:** ready-for-integration → build-master (asset-only; NO code, NO compile needed)

Resolves the TASK-172/173 flag *"CrystalTower emissive not preserved"* (also listed as TASK-189 follow-up
#4 "CrystalTower emissive pending"). The M7 TRELLIS reimport collapsed `SM_CrystalTower` to 2 slots
`[TeamRegion, CrystalTowerPBR]`, dropping the blockout-era dedicated crystal emissive slot; the shared
master `M_AssetPBR` has NO emissive parameter and no `T_CrystalTower_E` was baked — so the crystal read
flat blue (metallic reflection) instead of GLOWING.

## Approach (why this one)
The crystal is NOT a separable face-set in the refined mesh (only 2 material slots, the crystal lives
inside `CrystalTowerPBR`), so per-face assignment is impossible. Texture-channel masks don't isolate it
cleanly either: base-color albedo is near-black everywhere (mean ~0.04 — the crystal's blue comes from
metallic reflection, not albedo), and the metallic channel reads high across the whole rendered surface
(a metallic mask lit the entire tower — verified, rejected). The reliable isolator is the crystal's
POSITION: it sits at the top of the 487-unit-tall mesh. So the glow is localised by a **local-height
SmoothStep mask** in the material — robust to placement, scale, and rotation (local space).

Kept it **isolated**: all changes live in `M_CrystalGlow` (which pre-existed with **0 referencers**) +
a new instance + the mesh slot-1 pointer. The shared `M_AssetPBR` master was NOT touched (no blast radius
onto the other 12 PBR assets). Slot 0 team-recolor contract untouched.

## What changed

### `M_CrystalGlow` (MODIFIED — `/Game/Materials/M_CrystalGlow`)
Graph fully rebuilt from a flat-emissive placeholder into a **lit PBR + masked-emissive** material:
- **BaseColor** ← `T_CrystalTower_D` (SAMPLERTYPE_Color)
- **Normal** ← `T_CrystalTower_N` (SAMPLERTYPE_Normal)
- **Roughness / Metallic / AO** ← `T_CrystalTower_ORM` .G / .B / .R (SAMPLERTYPE_Masks) — so the stone
  body renders EXACTLY as the old `MI_CrystalTower_PBR` did.
- **EmissiveColor** ← `SmoothStep(Min=CrystalHeightStart, Max=CrystalHeightEnd, Value=LocalPosition.Z)
  × EmissiveColor × EmissiveStrength`.
- 4 tunable params (group "Crystal Glow"): `CrystalHeightStart`=340, `CrystalHeightEnd`=420,
  `EmissiveColor`=(0.05, 0.60, 1.00) linear cyan, `EmissiveStrength`=9 (master default).
- ShadingModel `MSM_DefaultLit`, BlendMode `BLEND_Opaque` (unchanged). Recompiled clean (no errors/warnings).

### `MI_CrystalGlow` (NEW — `/Game/Materials/Instances/MI_CrystalGlow`)
- Parent = `M_CrystalGlow`. One override: **`EmissiveStrength` = 12** (brighter beacon for daylight;
  hue stays cyan, doesn't fully white-clip). All other params inherit the master.

### `SM_CrystalTower` (MODIFIED — `/Game/Meshes/SM_CrystalTower`)
- Slot 1 `CrystalTowerPBR` → **`MI_CrystalGlow`** (was `MI_CrystalTower_PBR`).
- Slot 0 `TeamRegion` → `MI_TeamColor_Blue` — **UNCHANGED** (team recolor contract intact; the M3
  runtime swaps this slot to `MI_TeamColor_<Team>` in BeginPlay — I did not touch it).
- Geometry, collision, Nanite-off, tri count — all UNCHANGED (material pointer only).

`MI_CrystalTower_PBR` is now unreferenced by the mesh but left in place (harmless; still a valid asset).

## Verification (readback + eyeball)
- Material recompile raised no error; emissive + all 5 PBR outputs confirmed connected via `get_property_input`.
- Slots read back: `[TeamRegion → MI_TeamColor_Blue, CrystalTowerPBR → MI_CrystalGlow]`. MI `EmissiveStrength`=12.
- **Asset thumbnail:** crystal cluster + collar glow cyan with a bloom halo; stone body reads as carved
  stone (gold door accent intact). (`02_AFTER_asset_thumbnail_glow.png`)
- **In-scene real-lighting capture** (temp actor at origin in L_Arena, deleted after; level NOT saved):
  crystal glows cyan against the grass/sky, stone base preserved. (`03_AFTER_inscene_reallighting_glow.png`)
- Before (flat blue): `01_BEFORE_flatblue_crystal.png`.
- Screenshots dir: `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/Screenshots/M7_CrystalGlow/`
- All 3 assets `is_dirty`=false (saved).

## For build-master (stage/commit these 3 assets — I do NOT touch Git)
- `Content/Materials/M_CrystalGlow.uasset` (MODIFIED)
- `Content/Materials/Instances/MI_CrystalGlow.uasset` (NEW)
- `Content/Meshes/SM_CrystalTower.uasset` (MODIFIED — material-pointer only)
- Optional evidence (not required for the game): `Saved/Screenshots/M7_CrystalGlow/*` (Saved/ is usually git-ignored — commit only if the team tracks screenshots).
- No source-file (`Content/RawAssets/`) changes — that lane was off-limits (DeepMine+Wall generating there) and needed nothing.

## Notes / flags for integration
- **Glow is localised, not whole-body** (the higher-quality target, not the stopgap): only the top
  crystal + collar emit. Tunable live via `MI_CrystalGlow` params if Jonathan wants it stronger/higher/wider —
  `EmissiveStrength` (currently 12), `CrystalHeightStart/End` (340/420), `EmissiveColor` (cyan).
- **In-game intensity:** at the M7 PPV (manual-locked exposure, bloom threshold 0.85) the crystal will
  bloom more than in the bright editor viewport — same behavior GoldNode shows (its core clips toward
  white at that exposure, TASK-176 note). If it reads too hot in Jonathan's playtest, drop `EmissiveStrength`
  on `MI_CrystalGlow` — no recompile.
- **Card-art / preview:** `T_CardArt_CrystalTower` (hand UI) is unchanged and independent — not in scope.
- Lane isolation honored: no `Content/RawAssets/`, no `CardArt/`, no level saved, no C++/BP, `M_AssetPBR` untouched.
