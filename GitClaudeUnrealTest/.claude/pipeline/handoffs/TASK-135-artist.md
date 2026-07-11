# TASK-135 — Battlefield mesh curation + M_BattlefieldGround (art handoff)

Assignee: art-director · Status: ready-for-integration · Date: 2026-07-10
Law: CONVENTIONS "Battlefield & procedural terrain (M6.5)". Feeds **TASK-137** (build-master transcribes the
per-layer lists + params below into `/Game/Data/DA_BattlefieldScatter`).

Editor was OPEN, PIE NOT running (checked `IsPIERunning` = false) — browsing/inspection was read-only; the only
write was authoring `M_BattlefieldGround`. Donors left 100% untouched (soft-reference in place, per Fab quarantine).

All mesh stats below were read live from the editor (StaticMeshTools tri/LOD/bounds + ObjectTools BodySetup/AggGeom
collision). Bounds are LOCAL-space units (1 unit = 1 cm). `zmin`≈0 means pivot sits at the mesh base.

---

## 1. CURATED SCATTER SET (exact soft-ref paths, grouped by layer)

Donor meshes are soft-referenced IN PLACE — NOT conformed/renamed/duplicated (max variety, cheap). Paths are the
object paths for `TSoftObjectPtr<UStaticMesh>` in `DA_BattlefieldScatter`.

### TREES layer — BLOCKING obstacle — 12 variants (PREFER MOBILE / low-poly)
Folder: `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/`  (note the mesh name uses a HYPHEN: `SM-Mobile_Tree_N`)

| Mesh (object path)                                             | Tris | LODs | Pivot | Height (u) |
|---------------------------------------------------------------|-----:|:----:|:-----:|-----------:|
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1`     | 2458 | 1    | base  | 1614 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_2`     | 2025 | 1    | base  | 1385 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_3`     | 2115 | 1    | base  | 1740 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_4`     | 2137 | 1    | base  | 1674 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_5`     | 2283 | 1    | base  | 1601 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_6`     | 2422 | 1    | base  | 1836 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_7`     | 2249 | 1    | base  | 2094 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_8`     | 2213 | 1    | base  | 1933 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_9`     | 2360 | 1    | base  | 1934 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_10`    | 2202 | 1    | base  | 1745 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_11`    | 2291 | 1    | base  | 1771 |
| `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_12`    | 2393 | 1    | base  | 1792 |

- Chosen the MOBILE set over `SM_Highpoly_Tree_*` (those are 6800–7665 tris — ~3× heavier for the SAME 12 shapes;
  wasteful for blocking HISM instances). Mobile = clean pivot at base, ~2000–2460 tris, no per-mesh offset needed.
- NOTE: mobile trees ship with **no LODs** (LOD count = 1). Fine at ~2.2k tris, but cap the instance count (see §3)
  and consider build-master generating 1–2 LODs via `StaticMeshTools.generate_lods` on /Game/ duplicates if FPS drops.
- Recommended per-layer density/scale in §3.
- **COLLISION — FLAGGED, needs a build/programmer decision (see §2).**

### ROCKS layer — BLOCKING obstacle — 12 variants (boulders)
Folder: `/Game/Realistic_Rocks/Meshes/`

| Mesh                                            | Tris  | Width dx (u) | Note |
|-------------------------------------------------|------:|-------------:|------|
| `/Game/Realistic_Rocks/Meshes/SM_Rock_1`        |  3554 |   88 | small boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_5`        |  4240 |  128 | small boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_4`        |  6488 |  112 | flat-ish boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_7`        |  7602 |  174 | med boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_8`        |  9938 |  161 | med boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_2`        | 11270 |  140 | med boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_6`        | 11882 |  144 | med boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_10`       | 16516 |  182 | med-large |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_9`        | 17049 |  168 | tall boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_19`       | 16992 |  288 | large boulder |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_11`       | 18488 |  283 | LARGE hero (cap count) |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_20`       | 24472 |  322 | LARGE hero (cap count) |

- Collision: **OK.** Every rock has 1 imported convex hull hugging its shape (`convexElems=1`). rocks 1–20 use trace
  flag `CTF_UseComplexAsSimple` (queries hit per-poly, but a rock's per-poly ≈ its silhouette → clean blocking + good
  nav-carve; no action required, though build-master MAY flip to `Use Simple As Complex` for a small perf win).
- Deliberately EXCLUDED the very heaviest rocks (SM_Rock_14 = 65976 tris, SM_Rock_16 = 77342 tris) — too costly for
  instanced blockers. Optional small-pebble rocks (SM_Rock_21..30, 600–2500 tris, only ~15–27u) are NOT in the blocking
  set (units step over them); if wanted, add a couple as NON-blocking ground decoration (grass-layer style) or scale
  them 3–5× to be meaningful.

### HILLS layer — BLOCKING obstacle — 6 variants (mounds / rocky outcrops)
Only ONE dedicated "hill" mesh exists and it is heavy + tiny, so I augment it with the flat rock slabs (clean convex
collision, lower tris) scaled up as rocky plateaus/mounds. All units path AROUND these (blocking) — see the walkable note.

| Mesh                                                                        | Tris  | Base size (u) | Suggested scale |
|-----------------------------------------------------------------------------|------:|--------------:|-----------------|
| `/Game/Fab/Stone_Hills_FREE/stone_hill/StaticMeshes/stone_hill`             | 41290 | 201×201×56    | 15–30× → 30–60m mound (LOW count, heaviest) |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_31`                                   | 11154 | 282×286×47    | 5–12× flat rocky plateau |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_32`                                   | 13915 | 250×258×46    | 5–12× flat rocky plateau |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_33`                                   | 14642 | 222×246×39    | 5–12× flat rocky plateau |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_34`                                   | 21962 | 177×180×28    | 5–12× flat rocky plateau |
| `/Game/Realistic_Rocks/Meshes/SM_Rock_35`                                   | 15754 | 256×274×48    | 5–12× flat rocky plateau |

- Collision: **OK.** stone_hill + rocks 31–35 all have 1 convex hull AND trace flag `CTF_UseDefault` (uses the convex
  as simple — clean blocking + nav-carve, the best-behaved obstacle collision in the set). No action required.
- **WALKABLE-HILL caveat (flag to Jonathan):** these are BLOCKING mounds units route AROUND, not walk-OVER. True
  walkable elevation would need a UE Landscape (no MCP sculpt route — M4.5 ruling 3) or purpose-built ramped hill
  meshes; out of scope for M6.5. If Jonathan wants units to climb hills, that is a separate follow-up.

### GRASS layer — NON-blocking decoration (NO collision) — 10 grass + 5 plant accents
Folder: `/Game/Realistic_Grass_and_plant/Meshes/`  — all have **5 LODs** (cheap for dense scatter).

Grass tufts (mix short + tall): `SM_Grass_1` (236t), `SM_Grass_7` (280t), `SM_Grass_3` (288t), `SM_Grass_5` (362t),
`SM_Grass_10` (384t), `SM_Grass_14` (384t), `SM_Grass_27` (384t), `SM_Grass_23` (464t, tall), `SM_Grass_17` (480t,
tall), `SM_Grass_9` (569t, tall).
Full paths: `/Game/Realistic_Grass_and_plant/Meshes/SM_Grass_{1,7,3,5,10,14,27,23,17,9}`

Leafy/fern accents (lower density, for variety): `SM_Plant_6` (448t), `SM_Plant_3` (512t), `SM_Plant_8` (1536t),
`SM_Plant_12` (2286t), `SM_Plant_15` (3438t, lush).
Full paths: `/Game/Realistic_Grass_and_plant/Meshes/SM_Plant_{6,3,8,12,15}`

- Collision: NONE needed (decoration). Keep `bBlocking=false`, NoCollision, `bCanEverAffectNavigation=false`.

---

## 2. COLLISION VERIFICATION (obstacle layers) — result per mesh

Every curated obstacle mesh HAS simple collision (≥1 convex hull). Summary of the trace behavior:

- **ROCKS (all 12):** ✅ 1 convex hull each, hull hugs the rock. rocks 1–20 = `CTF_UseComplexAsSimple`. Usable as-is for
  Pawn-block + nav-carve. No fix required.
- **HILLS (stone_hill + rocks 31–35):** ✅ 1 convex hull each, `CTF_UseDefault` (convex used as simple). Cleanest of all.
  No fix required.
- **TREES (all 12 mobile):** ⚠️ **FLAGGED.** Each has 1 convex hull, BUT the hull is a **FULL-CANOPY blob** (e.g.
  SM-Mobile_Tree_1 hull box ≈ 871×804×1614 u — it wraps the whole tree, not the trunk) AND the trace flag is
  `CTF_UseComplexAsSimple` (queries route to per-poly = trunk + leaf cards). Neither path gives the clean
  **trunk-footprint** block the M6.5 spec prefers:
  - Per-poly (current) → units snag on leaf geometry — messy.
  - Convex hull → an ~8 m-radius invisible block around a ~1 m trunk — units detour around "empty air".
  **Recommended fix at TASK-137 (build-master / programmer — donors are read-only, so work on /Game/ duplicates):**
  duplicate the 12 mobile trees into `/Game/Meshes/BattlefieldTrees/`, then EITHER (preferred) `remove_collisions` +
  `generate_convex_collisions(hull_count=1)` after temporarily is not enough (still canopy) — better to author a slim
  trunk capsule/box (~50–90 u radius, tree-height tall) and set collision complexity to **Use Simple As Complex**; OR
  (cheap fallback) keep the existing convex hull but flip the flag to **Use Simple As Complex** and accept canopy-wide
  blocking. If duplicating trees breaks the "soft-ref in place" goal, that's an accepted trade for the TREE layer only
  (the other 3 layers stay in-place). This is the ONE collision action item for M6.5.

---

## 3. RECOMMENDED per-layer scatter params (starting points for DA_BattlefieldScatter; build-master tunes at TASK-137)

Counts assume the widened ~16000-long field (TASK-136 sets final floor dims — scale counts to final area; blocking +
dynamic-nav is costly, so these are conservative). Keep-clear zones per spec (castle pads r~900 @ ±8000, gold pads
r~500 @ ±7200, PlayerStart/spawn, central lane |Y|≤~400); GRASS ignores keep-clear.

| Layer  | bBlocking | InstanceCount (start) | ScaleRange | Yaw    | MinSpacing (u) | Notes |
|--------|:---------:|----------------------:|-----------|--------|---------------:|-------|
| TREES  | true      | 60–100                | 0.8–1.4   | random | ~600 | ~2.2k tris ea, no LOD → cap count; needs the §2 trunk-collision fix |
| ROCKS  | true      | 80–140                | 0.5–2.0   | random | ~350 | cap the 2 LARGE hero rocks (11,20) to a few each |
| HILLS  | true      | 6–14 total            | see table (stone_hill 15–30×, slabs 5–12×) | random | ~1500 | heaviest tris + big footprint → LOW count |
| GRASS  | false     | 2000–5000 grass + 300–600 plants | 0.7–1.5 | random | ~120 | LOD'd + no collision → cheap; lush everywhere (ignore keep-clear) |

- **Pivots/origins:** all curated meshes sit at ground (zmin ≈ 0 to −80). Place instances at floor Z with NO per-mesh
  offset — rocks/grass modeled slightly sunk read naturally. Only the biggest rocks (SM_Rock_11 zmin −58, SM_Rock_20
  zmin −64) sink ~60 u at scale 1; at their large scales that's negligible/natural. No offsets flagged.
- `bMirrorSymmetric=false` (asymmetric organic random — Jonathan's ruling).

---

## 4. EXCLUSIONS (browsed, deliberately NOT used) — with reasons

- **Megaplant_Library** (`Tree_Norway_Spruce`, `Tree_Japanese_Cypress`): EXCLUDED. Its whole-tree assets are
  **skeletal meshes** (each `Tree_*_A/B/C…` has a companion `_Skeleton`) plus Blueprint/`PVE_`/`Preset_` packed actors —
  NOT `UStaticMesh`, so they cannot feed a HISM (`TSoftObjectPtr<UStaticMesh>`). The `Instances/` folder is
  branch/twig/decoration COMPONENTS, not whole trees. Tree_Pack_1 mobile trees are the correct HISM-ready choice.
- **`/Game/Fab/Rocks/highpoly_rocks_free_download/StaticMeshes/highpoly_rocks_free_download`**: EXCLUDED. It is ONE
  merged mega-asset of an entire rock field — **1,404,928 tris**, bounds ~60000×71400×11800 u. Not a single scatterable
  rock; unusable for instancing. Realistic_Rocks covers rocks with far better variety + budget.
- **`SM_Highpoly_Tree_1..12`**: not used (mobile equivalents chosen — same shapes, ~3× fewer tris).
- **Character packs** (`Prickly_Knight`, `sA_StylizedWizardSet`), **VFX** (`sA_ArcheryVfxPack`, `IceAttack`),
  **castle walls** (`Fab/Megascans/Castle_Wall_*`): excluded per spec.
- **`MedievalCastleEnvironmentAndSiegeWeaponProps`**: considered as dressing, DECLINED for the RANDOM field scatter — a
  catapult/barrel randomly dropped mid-field reads as odd; these are better as hand-placed castle dressing (a possible
  future hand-placement task), not procedural terrain. Kept the scatter on-brief (grass/rocks/trees/hills).

---

## 5. GROUND MATERIAL — `M_BattlefieldGround`

- Path: `/Game/Materials/M_BattlefieldGround` — created, graph authored, compiled clean, verified BaseColor/Normal/
  Roughness outputs are all driven; thumbnail confirms green grass with visible macro variation (§6 compliant: stylized,
  NO flat single color).
- **Recipe:** WORLD-XY (top-down) projected tiling grass, so it maps cleanly on the big flat floor slab regardless of
  the slab's own UVs (avoids the stretch a 0–1 slab UV would cause). Base color = tiling grass texture × a low-frequency
  macro TINT lerp (lush green ↔ warm dry) driven by a large-scale sample → breaks up tiling / no flat color. Normal from
  the grass normal map. (The vertical streaks visible in the SPHERE thumbnail are just world-XY projection on vertical
  faces — a non-issue on the horizontal ground.)
- **Source textures (Tree_Pack_1 grass-ground set, referenced in place):**
  `/Game/Tree_Pack_1/Textures/Ground/T_Ground_Grass_C` (base color) + `T_Ground_Grass_N` (normal).
- **Exposed parameters (for MI hue variants / build-master tuning — no master edit needed):**
  `GrassBaseColor` (Texture), `GrassNormal` (Texture), `MacroMaskTex` (Texture), `GrassTiling` (scalar, 0.0022 ≈ tile
  every ~450 u), `MacroTiling` (scalar, 0.00007), `TintLush` (vector), `TintDry` (vector), `GroundRoughness` (0.92).
- MI hue variants: NOT authored (params make them trivial for build-master if desired) — e.g. an
  `MI_BattlefieldGround_*` in `/Game/Materials/Instances/` tweaking `TintLush/TintDry/GrassTiling`. Optional.
- **Integration (TASK-137, build-master):** apply `M_BattlefieldGround` to the arena floor slab scaled in TASK-136. I
  did NOT place anything in L_Arena and did NOT populate the DataAsset (its class may not be compiled until TASK-136).

---

## 6. Notes for integration (build-master, TASK-137)
- Transcribe the §1 lists + §3 params into `/Game/Data/DA_BattlefieldScatter` (obstacle layers `bBlocking=true`, grass
  `bBlocking=false`, `bMirrorSymmetric=false`).
- Resolve the §2 TREE collision item (trunk capsule on /Game/ duplicates, or accept canopy-wide convex via Use Simple
  As Complex) so tree blocking + nav-carve behave — this is the only obstacle-collision action.
- Apply `M_BattlefieldGround` to the scaled floor slab.
- Mesh names use a hyphen for mobile trees (`SM-Mobile_Tree_N`) — copy paths exactly.

---

## 7. UPDATE 2026-07-11 — M_BattlefieldGround RE-AUTHORED + SAVED + APPLIED (coordinator follow-up)

Root cause of the redo: the original `M_BattlefieldGround` (§5) was authored in-memory but never saved to disk, so it
was LOST when the editor relaunched (build-master TASK-136 relaunch). Ground was still default grey. Fixed:

- **Re-authored** `/Game/Materials/M_BattlefieldGround` — identical design (world-XY tiling grass from
  `T_Ground_Grass_C`/`_N`, low-freq macro tint LERP lush↔dry, all 3 outputs driven, exposed params
  `GrassTiling`/`MacroTiling`/`TintLush`/`TintDry`/`GroundRoughness`).
- **SAVED IMMEDIATELY + read-back verified:** `save_assets` → true; `exists`=true; `is_dirty`=false;
  `get_asset_class`=`Material`; physical file confirmed on disk at `Content/Materials/M_BattlefieldGround.uasset`.
  (`exists_before`=false confirmed the original was indeed lost.)
- **Tuning MI created + saved:** `/Game/Materials/Instances/MI_BattlefieldGround` (instance of the master;
  `is_dirty`=false after save). Nudged greener via MI params (no master recompile): `TintLush`=(0.55,1.15,0.38),
  `TintDry`=(0.95,1.05,0.48), `GrassTiling`=0.0016. Build-master/Jonathan can retune via this MI without touching the
  master.
- **Applied to the ground:** `ArenaGround` (a `/Engine/BasicShapes/Cube` slab, xform loc (0,0,−50) scale (180,48,1) =
  ±9000 X / ±2400 Y, top surface Z=0) — set its StaticMeshComponent `OverrideMaterials[0]` = `MI_BattlefieldGround`.
  **NOTE:** must set the material via the `load_asset` canonical ref (a hand-built refPath string set the slot to
  `None`). L_Arena is a NON-World-Partition level → `save_actor` errors ("not an external actor"); saved via
  `save_assets(['/Game/Maps/L_Arena'])` instead → true, `is_dirty`=false, override persisted after save.
- **In-engine CaptureViewport** (no GDI, non-disruptive; PIE not running): angled overhead shot confirms the ground now
  renders as a GREEN grassy field with macro variation (lush/dry mottling), not grey. Image saved to
  `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\c5eac1f8-51b3-4528-920b-668630c1b7bb\scratchpad\ground_grass.png`.
- Untouched: scatter config / BP_BattlefieldScatter (build-master TASK-137), Jonathan's imports, all donors. No Git.
