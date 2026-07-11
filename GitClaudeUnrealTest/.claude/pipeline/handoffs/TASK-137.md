# TASK-137 PART 2 — battlefield scatter assembly (build)

**Owner:** build-master  **Date:** 2026-07-11  **Result:** CORE MECHANICS DONE + VERIFIED; two upstream/environment BLOCKERS on the visual review. **NO COMMIT** (held for Jonathan per coordinator).

## DONE + VERIFIED
### DA_BattlefieldScatter (`/Game/Data/DA_BattlefieldScatter`, USiegeScatterConfig) — created + populated
7 layers (WARN-2 satisfied — no mesh shared across layers). Config-wide: `bMirrorSymmetric=false`, `CorridorHalfWidth=800`, `ArenaHalfExtent=(8600,2400)`, keep-clear castle 900 / node 500 / start 700.

| Layer | Meshes (TASK-135 curated) | Count | Scale | Bias | Spacing | Blocking |
|---|---|---:|---|---|---:|:--:|
| Trees | 12 mobile `SM-Mobile_Tree_1..12` | 55 | 0.8–1.2 | Edge | 600 | ✓ |
| Rocks | 10 medium `SM_Rock_[1,5,4,7,8,2,6,10,9,19]` | 60 | 0.5–1.5 | Edge | 350 | ✓ |
| Boulders | 2 hero `SM_Rock_[11,20]` | 6 | 0.8–1.3 | Edge | 900 | ✓ |
| Hill | `stone_hill` | 4 | 10–15× | Edge | 2500 | ✓ |
| Slabs | `SM_Rock_[31..35]` | 8 | 4–7× | Edge | 1500 | ✓ |
| Grass | 10 `SM_Grass_[1,7,3,5,10,14,27,23,17,9]` | 2500 | 0.7–1.5 | Whole | 120 | ✗ |
| Plants | 5 `SM_Plant_[6,3,8,12,15]` | 400 | 0.7–1.5 | Whole | 200 | ✗ |

Populated verified live via `LogSiegeTerrain` (all layers place their target counts each match). (`get_properties` shallow-reads the nested struct fields as null — NOT a set failure; the scatter log is the ground truth.)

### Scatter actor placed — via a BLUEPRINT SUBCLASS (necessary, note this)
`ASiegeBattlefieldScatter::ScatterConfig` is **EditDefaultsOnly** → it CANNOT be set on a plain placed C++ instance (`set_properties` refuses it; confirmed the actor is otherwise settable via `OverrideSeed`). So I created **`/Game/Blueprints/BP_BattlefieldScatter`** (subclass), set `ScatterConfig=DA_BattlefieldScatter` on its CDO (read-back confirmed), compiled it, and placed it in L_Arena as `BP_BattlefieldScatter_C_0` (label "BattlefieldScatter"). The eventual commit must include `BP_BattlefieldScatter.uasset` alongside `DA_BattlefieldScatter.uasset` + `L_Arena.umap`.

### Tree collision (QA WARN-1) — DATA fix chosen (trunk-capsule not MCP-authorable)
StaticMeshTools exposes only `remove_collisions` / `generate_convex_collisions` — **no capsule/box primitive, no trace-flag/complexity setter** — so a slim trunk capsule can't be authored via MCP. Took the coordinator's second sanctioned option: keep trees in-place with existing collision + the DATA fix — widened `CorridorHalfWidth` to 800, set all blocking layers to **EdgeBias**, and capped tree scale (0.8–1.2). Correctness gate (traversability) passes; the canopy-footprint refinement (trunk capsule) is a follow-up (hand-author in-editor or a programmer task).

### TRAVERSABILITY (#1 gate) — CONFIRMED across 3 fresh seeds
| Seed | Corridor | Result |
|---|---|---|
| 1357423105 | 700 (pre-tune) | CONFIRMED after 4 culls (69 instances) |
| 845309121 | 800 (tuned) | CONFIRMED after 3 culls (52) |
| 775380097 | 800 (tuned) | CONFIRMED after 1 cull (14) |
Different seeds → different layouts (re-scatter proven). Every match the deterministic corridor + nav-cull guarantee yields a Blue→Red path; the tuning reduced cull reliance. Bot units spawn + march the field each match. (Re-scatter driven by StopPIE→StartPIE fresh seeds; the in-game "Play Again" button path was not clicked — needs UI input.)

## BLOCKERS / GAPS (honest)
1. **GROUND MATERIAL `M_BattlefieldGround` is MISSING.** `find_assets` for "Battlefield"/"BattlefieldGround" across the whole project returns only DA_BattlefieldScatter — the material art's TASK-135 handoff claims at `/Game/Materials/M_BattlefieldGround` does NOT exist. It was almost certainly authored in memory and lost to an editor bounce before art saved it. **Could not apply it** (the OverrideMaterials set failed — not a valid MaterialInterface). Ground stays default grey; the grass scatter provides vegetation on top but the base isn't grassy. → **route to art-director: re-author AND SAVE M_BattlefieldGround**, then re-apply.
2. **Populated-field SCREENSHOTS not obtainable headlessly this pass.** The scatter HISMs are runtime-only (PIE world); `CaptureViewport` renders the EDITOR world (no scatter — `battlefield_populated_1.png` is the bare editor corridor, disregard). A GDI grab of the PIE viewport needs the editor foregrounded — **off-limits: Jonathan is actively playing League of Legends** (foregrounding disrupts his game; a full-screen grab captures his screen — happened once on TASK-136, disregarded). → the visual review needs a foreground/non-gaming session (Jonathan at the machine, or coordinator captures live).
3. **FPS not meaningfully measurable.** The PIE world comes up at `max tick rate 3` — the editor is throttled to ~3 Hz because it's backgrounded behind Jonathan's game. The ~3 FPS measured is the throttle cap, NOT real performance. Real FPS is a human watch at Jonathan's foreground playtest (heightened per the §6 concern — 133 blocking + high-poly + dynamic-nav rebuild is costly; the nav-settle cull spike is a noticeable hitch even ignoring throttle).
4. **Traversability leans on the cull** (14–69 blocking instances culled per seed to open the lane) — large obstacles (hills 10–15×, slabs 4–7×) in the narrow ±2400 field intrude toward the lane. Guarantee HOLDS (always confirmed, under the 5-cull cap), but the field near the lane thins. Further tuning (fewer/smaller hills+slabs, or a wider Y field) would reduce cull reliance. Quality item, not a gate failure.

## State
- Saved (surgical): `DA_BattlefieldScatter`, `BP_BattlefieldScatter`, `L_Arena`. NO commit, NO git. Jonathan's imports / `.uproject` untouched. Editor left running (PID 3788) for follow-up.
- New asset for the eventual commit set: `BP_BattlefieldScatter.uasset` (subclass) + `DA_BattlefieldScatter.uasset` + `L_Arena.umap`.

## Recommendation
Scatter mechanics + traversability are solid and verified. The VISUAL review Jonathan needs is blocked on (1) the missing ground material and (2) a foreground/non-gaming session to screenshot the populated PIE field. Suggest: art-director re-authors+SAVES `M_BattlefieldGround`; then a short follow-up applies it and captures the live battlefield when Jonathan can view the editor — then the held PART1+PART2 commit.
