# TASK-249 — M_HillGrass tri-planar grass material + hill-layer OverrideMaterial wire — artist handoff

**Agent:** art-director · **Date:** 2026-07-22 overnight (~02:15–03:15) · **Status:** ready-for-integration (verified live over 3 re-seeded PIE/SIE runs)

## Assets created / modified (exact /Game/ paths, all SAVED)

| Asset | Change |
|---|---|
| `/Game/Materials/M_HillGrass` | NEW master material (this task). Git: `Content/Materials/M_HillGrass.uasset` (editor SCC auto-staged the add — `AM` in the index, build-master handles at commit) |
| `/Game/Data/DA_BattlefieldScatter` | Hill layer `OverrideMaterial=/Game/Materials/M_HillGrass.M_HillGrass`; opt-ins `bAllowOnHills=true` on Trees/Rocks/Grass/Plants (`MaxPlacementSlopeDeg` left at the 35 default on all layers); + the TASK-251 fields (see its handoff) |

Main-lane SM_Hill_01/02/03 assets untouched (lane-clean per spec — override applied at the HISM component level by TASK-250's code). L_Arena untouched (`is_dirty=false` verified; scatter is runtime).

## Material approach (stock nodes ONLY — zero Custom HLSL; compiles in ~1 s)

World-aligned TRI-PLANAR + slope blend, per the CONVENTIONS W1-PREP law (straight XY projection forbidden):

- **Tri-planar BaseColor:** WorldPosition masked to XY/XZ/YZ planes × `GrassTiling` → 3 TextureSamples of one `GrassBaseColorTex` TextureObjectParameter (T_Ground_Grass_C — the SAME texture M_BattlefieldGround uses). Blend weights = pow(abs(VertexNormalWS), `TriplanarSharpness`), normalized by their sum. No smear on any face by construction.
- **Field-matching macro tint:** XY × `MacroTiling` sample's R lerps `TintLush`→`TintDry`, multiplied over the tri-planar color — the exact recipe M_BattlefieldGround uses, so hills patch-match the field's lush/dry variation.
- **Slope blend to dirt/rock:** SmoothStep(`SlopeRockFullNz`=0.5, `SlopeGrassFullNz`=0.8, VertexNormalWS.Z) lerps a Desaturation(0.85)×`RockTint` dirt tone over the grass on steep faces. Full grass ≤~37°, full dirt ≥~60° — hill walkable faces (≤30° law) stay entirely grassy; only cliff-class faces read rocky.
- **Normal:** `GrassNormal` TextureSampleParameter2D (T_Ground_Grass_N, Normal sampler) on the top/XY projection, flattened toward (0,0,1) on steep faces by the same slope mask (kills the stretched-normal streak artifact).
- **Roughness:** `GroundRoughness` scalar.

**Parameter defaults are pinned to MI_BattlefieldGround's LIVE values** (not the master's), so the hill look matches the actual field with no MI needed: GrassTiling 0.0016, MacroTiling 7e-05, TintLush (0.55,1.15,0.38), TintDry (0.95,1.05,0.48), GroundRoughness 0.92. New knobs: TriplanarSharpness 4, RockTint (0.72,0.62,0.50), SlopeRockFullNz 0.5, SlopeGrassFullNz 0.8. All iteration is parameter-level — never a shader recompile. No MI authored (spec wires the master directly; an MI_HillGrass can be layered on later without touching this).

## ⚠ LESSON FOR THE LAW BOOK (manager: consider a CONVENTIONS line)

**A material consumed via `FScatterLayer.OverrideMaterial` MUST have `bUsedWithInstancedStaticMeshes=true`.** A fresh material lacks the usage flag; SetMaterial on a runtime HISM then silently falls back to the engine default WorldGridMaterial (dark gray) — hills rendered gray-charcoal while the material previewed perfectly. Diagnosed live: PIE hill HISMs had `OverrideMaterials[0]=M_HillGrass` (TASK-250's application is CORRECT and verified at component level) yet rendered the fallback; a loud-red param test changed the thumbnail but not the hills; flag set → recompile → hills green on the next seed. The donor path never hit this because SM-asset-slot materials get usage-checked on load. Flag is set + saved on M_HillGrass.

## Verification (3 re-seeded runs, fresh session each)

| Seed | Mode | Hill fill | Opted-in fills | Traversability |
|---|---|---|---|---|
| 471742529 | SIE | 8/8 | Trees 70/70, Rocks 60/60, Grass 2500/2500, Plants 400/400 | CONFIRMED, 0 culls |
| 53151321 | SIE | 8/8 | all 100% | CONFIRMED, 0 culls |
| 1677445121 | PIE (standard) | 8/8 | all 100% | CONFIRMED, 0 culls |

- **Acceptance bar (Jonathan's screenshot):** hills read as grassy terrain CONTINUOUS with the ground — verified up close (climb distance) and at gameplay-camera angles; steep faces show a natural dirt blend, zero smear. From straight overhead the hills now near-camouflage into the field (only shading reveals them) — the exact opposite of the gray "before".
- **TASK-250 NIT-2 runtime assumption CONFIRMED:** grass tufts + plants visibly growing ON hill surfaces in both closeups; tree clusters sitting on hill flanks in the seed-1 overhead. Same-frame instance bodies work.
- **NIT-1 fill-rate watch:** no under-fill anywhere — every layer 100% on all 3 seeds (24 attempts/instance absorbs the over-slope rejections at current densities).
- **Two-pass order visible in logs** (pass 1 Boulders/Hill/Slabs → pass 2 Trees/Rocks/Grass/Plants) — TASK-250 behaving as designed.
- fps: PIE/SIE ran hitch-free; no LogSiegeTerrain warnings/errors any run. (Machine numbers deliberately not cited — W-gate law; Jonathan's W1 watch is the perf verdict.)

## Captures (durable, gitignored Cache — Jonathan's morning W1 evidence)

All under `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\Cache\HillGrass\w1prep\`:

- `before_overhead.png` / `before_hill_closeup.png` — the fail bar recreated (bare gray smeared hill amid grass, tree at floor level beside it; seed 1702611713)
- `after_seed2_bighill_closeup.png` — **the money shot**: fully grassy hill, field-continuous texture, tufts on the mound, second grassy hill behind
- `after_seed2_smallhill.png` — mini-hill (0.4-class) example
- `after_seed1_overhead.png` — trees/props ON hills + size spread (captured pre-usage-flag-fix: hills still gray here — it doubles as defect evidence)
- `after_seed2_overhead.png` — wide shot, green hills blended into the field
- `after_seed3_gameplaycam.png` / `after_seed3_overhead.png` — standard-PIE player view
- `m_hillgrass_thumb.png` — material preview (grass top → dirt steep)

## Integration notes

- Commit set for this task: `Content/Materials/M_HillGrass.uasset` (new, staged `AM`) + `Content/Data/DA_BattlefieldScatter.uasset` (shared with TASK-251) — branch lane `m7.6-arena10x` fold per the board.
- Python remote execution was re-enabled IN-MEMORY only this session (TASK-221 §2 recipe, for the DT_Cards reimport) — no config file written; reverts on editor restart.
- MCP capture note for future editor sessions: `CaptureViewport.captureTransform` overrides the camera in SIE but NOT in standard possessed-pawn PIE — SIE is the screenshot lane.
- Residual (not a blocker): navmesh coverage on a giant (2.5×) crown was not directly spot-checked (no nav-debug MCP surface); castle-to-castle traversability passed 0-cull on all seeds with giants present. Falls to Jonathan's W1 climb/playtest.
