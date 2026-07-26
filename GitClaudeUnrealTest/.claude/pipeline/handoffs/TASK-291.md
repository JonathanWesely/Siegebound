# TASK-291 handoff — M7.6 Phase 5 placement (build-master)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · **HEAD at start:** `1df47b6` · editor placement + git, NO compile.
Placed the TASK-290 dressing into `L_Arena` + fog polish. **NOTE:** the editor was on `L_MainMenu` (art-director's TASK-290 prep) — I loaded `L_Arena` first, so the dressing is in the correct level.

## Placement summary (26 actors, seeded one-shot fixed layout)
Live nav bounds read: `NavMeshBoundsVolume` = X∈±28000, Y∈±12500 → the vista ring at ±34000/±20000 is safely OUTSIDE the play field. All 26 spawned as plain `StaticMeshActor`s (NoCollision baked by TASK-290), 0 spawn failures.

- **Vista ring — 16 instances** (`SM_Vista_01/04` grey peaks 12–14×, `SM_Vista_02/03` green ridges/foothills 8×), on the far perimeter well outside the field:
  - Far X-ends (behind castles): (±34000, 0), (±32000, ±9000) — 6 peaks
  - Long sides: (±22000, ±20000), (0, ±20500) — 6 ridges/mix
  - Corners: (±30000, ±18000) — 4 tall peaks
  - Varied yaw + radius. **Per-instance vista flags SET on all 16** (verified `vista_flag_ok=16`): `castShadow=false`, `bVisibleInRayTracing=false`, `bCanEverAffectNavigation=false`.
- **POI landmarks — 8 instances** (`SM_POI_01..04`, scale 2.5–3.5×), off the Y≈0 lane at |Y| 3.5k–7k, ≥~6–7k from either castle: (−15000,6000),(−9000,−6500),(−19000,−3500),(−5500,7000),(15000,−6000),(9000,6500),(19000,3500),(5500,−7000). `bCanEverAffectNavigation=false` set; NoCollision baked. snap_to_ground.
- **Gold-node props — 2 instances** (`SM_GoldNodeProp`, scale 1.75×) mid-field flanking the origin capture zone: (0, +5000), (0, −5000). VISUAL only (plain StaticMeshActors, NOT AGoldNode/ACaptureZone). NoCollision.

## Fog / light polish
Existing `ExponentialHeightFog_0` already had `startDistance=10000` (gameplay within 10 km stays clear, the far ring fogs = vista depth). Modest tune for more atmospheric fade on the ring: `fogDensity 0.008→0.012`, `fogMaxOpacity 0.85→0.92` (startDistance/heightFalloff kept). `DirectionalLight_0` (intensity 11) left AS-IS — it's tuned, and touching the main light risks the gameplay look; the fog is the vista-depth lever. Final look = Jonathan's pixel-check.

## Traversability — UNCHANGED (verified)
Dressing is NoCollision + `bCanEverAffectNavigation=false` → cannot affect nav (no Build>Navigation needed). PIE across fresh generates (seeds 2138636033, 1633272449): **Traversability CONFIRMED — Blue→Red + 6 mine paths, 0 culls** each. Scatter behavior byte-identical to TASK-289b (its ×3 stands).

## ⚠ Findings for Jonathan (W-gate / Phase 6) — non-fatal, gameplay intact
1. **`LogUnrealMath: Error: TMatrix InverseFast … non-invertible matrix → NaN`** now fires at PIE first-frame (same point as the pre-existing TASK-287 LWC ensure `DoubleFloat.cpp:19` / `Matrix.h:468`). This is the 10×-arena **Large-World-Coordinates matrix-precision** family, and the vista ring at extreme coordinates (±34000, up to 14× scale) likely **exacerbates** it (it was not in the TASK-287/289 error scans; the LWC ensure itself predates the vistas, so a fully "clean" Message Log is not achievable by adjusting the dressing alone). Non-fatal (robustness log, not a crash), traversability + gameplay CONFIRMED. **Mitigation options (Jonathan's call):** pull the vista ring inward / cap vista scale, and/or an LWC project setting (tile size). This is the standing W2/W3 rendering WATCH, now with the InverseFast NaN added.
2. **`LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget`** — a HUD/UI focus error, UNRELATED to the dressing (dressing is world meshes). Likely pre-existing HUD or tied to the parked deck-builder WBP; flag to the HUD/deck-builder owner, not this task.
3. Vista upgrade path stands (TASK-290 §4): real Nanite Megascans cliffs (or the in-project 1.4M-tri `highpoly_rocks_free_download`) can swap in later, non-breaking.

## Commit
`Content/Maps/L_Arena.umap` + the 9 `SM_Vista/POI/GoldNodeProp` `.uasset` (LFS) + board/handoff. **Branch-owned-only diff verified** — NO fleet code / DA / SummonedUnit / SiegeBotController / gameplay source touched. `DeckBuilderWidget`/`WBP_DeckBuilder` (parked) + all M7.5 `SK_`/`SM_` fleet untouched/unstaged. No push. **⚠ Phase 6 (capstone playtest + merge-to-main) is Jonathan's gate — NOT started here.**
