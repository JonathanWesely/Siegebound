# TASK-630 — [GH-5] INTERIOR UV + 4096² BAKES — the re-opened texture gates, measured (art-director handoff)

**Status: COMPLETE — Cache-side only. Ready for TASK-632 (crumble trio) and TASK-633 (import).**
Fully headless Blender 5.1.2 `--background --factory-startup`. **No Content/ write, no manifest edit, no editor/MCP, no git, no Meshy (GH-R4).**
Laws: GH-R1/R5 · the `albedo_delight` DEFAULT-TRAP law (profile PINNED) · the building method law (instrument validated 4/4 to 4 dp) · the anti-bleach guard · cp1252 (`encoding=` on every text open in the task scripts). Date: 2026-08-18.

---

## THE ONE-LINE RESULT

The 3,555 new interior faces are UV-unwrapped into their own full 0–1 atlas and baked to a fresh **4096² `T_Castle_Interior_{D,N,ORM}` set** (coursed 0.9 × 0.45 m warm-grey block walls with ~2.5 cm mortar, 0.6 m flagstone floor) under the **pinned LOCKED delight profile** — while the exterior is untouched to the byte: **exterior UV loops sha256-identical on the saved .blend, and all 7 retained exterior texture/FBX artifacts hash byte-identical** (GH-R5 retention gate PASS). One declared structural change rides with it: **material slot 2 `CastleInteriorPBR` was APPENDED** (see §3 — flagged to TASK-634 per the spec's own clause, never silently).

---

## 1. DELIVERABLES (v3 — final)

| artifact | path | sha256 |
|---|---|---|
| updated blend | `Tools/ArtPipeline/Cache/Castle/castle_redesign_v1.blend` | `dd29effed485377a3d32639ced19d246f98fee6fe44a55938eb34dc2b4474df5` (was 629's `71571edb…`) |
| interior albedo (post-delight, sRGB) | `Tools/ArtPipeline/Cache/Castle/redesign_textures/T_Castle_Interior_D.png` | `544a199f298c648f1f10a7bf49d2725ba980dcb3ff5dad60ce5d3a2ebf7f7619` |
| interior normal (tangent, linear) | `…/redesign_textures/T_Castle_Interior_N.png` | `64c1dba2acb9d7f5e01cf381f873973225e875fcdb378b4aa6a096f849f32dc0` |
| interior ORM (R=AO, G=rough, B=metal 0, linear) | `…/redesign_textures/T_Castle_Interior_ORM.png` | `582ed6952271037f564048f7a929661f69f30878f22db82f6d27bd46a35f5bf8` |
| debug set (pre-delight D · AO · R · exact coverage MASK · intended-read reference card) | `…/redesign_textures/debug/` | — |
| textured interior previews ×4 (eyeballed) | `…/redesign_textures/previews/int_{hall_to_door,hall_to_north,mouth_to_hall,floor_east}.png` | — |
| reproducibility scripts + reports | `…/Cache/Castle/t630_uv_bake.py` · `t630_rebake_ao.py` · `t630_material_v2.py` · `t630_material_v3.py` (the shipped material) · `t630_measure_gates.py` · `t630_report_fix.py` → `t630_{survey,bake_report,rebake_report,material_v2_report,material_v3_report,gates_report}.json` | — |

### ⛔ NOT TOUCHED
`Content/**` (all 7 retained artifacts re-hashed byte-identical, §5.4) · `pipeline_manifest.json` · any `.uasset` · `L_Arena` · git · TASKBOARD · Meshy.

---

## 2. UV — THE EXTERIOR IS BYTE-STABLE, THE INTERIOR HAS ITS OWN ATLAS

- **Unwrap set:** exactly the 3,555 `interior_redesign` faces that carried the CastlePBR parking slot (629 §6). The other 182 interior faces are TeamRegion high-roof membranes — **audited: center-z 2,300–4,870 uu, all above sightlines, left untouched on slot 0** (629's "invisible" claim verified, not assumed).
- **Method:** edit-mode bmesh selection (explicit down-flush) → `uv.smart_project` (angle 66°, island margin 0.005) → shape-aware `uv.pack_islands` re-pack, interior islands only, into the full 0–1 range of the SAME `UVMap` layer. Because slot 2 samples its own texture set, interior islands legally share 0–1 with the exterior layout without any conflict.
- **⭐ GH-R1 PROOF (the load-bearing number):** sha256 over the packed float32 loop-UVs of all **58,543 loops of the 19,513 keep-set faces**: `28db3863…dbc61` before edit == after edit == **recomputed on the SAVED .blend**. Vertex-position hash equally identical (this task moved zero geometry). First-run honesty: the object-mode selection recipe DID leak the unwrap onto exterior faces — the snapshot guard caught it and **aborted before save**; the shipped blend is from the fixed bmesh recipe only.
- **Density (declared):** interior UV coverage **0.203** of 4096² ⇒ **~20 texels/m (0.20 texels/uu, ~5 cm/texel)** over 8,725 m² of interior surface (the tagged set includes buried fill-plate faces, which is why coverage cannot go higher). That is still ~4× finer than the exterior's effective 2048 density on this castle. Exact bake-mask covered fraction 0.1992 matches.

## 3. ⚠️ THE SLOT TABLE — ONE APPENDED SLOT, FLAGGED LOUD (the spec's own escape clause, not a silent ship)

| index | slot name | faces | material intent at import |
|---|---|---|---|
| 0 | `TeamRegion` | 4,318 (incl. the 182 membranes) | `MI_TeamColor_<Team>` — unchanged |
| 1 | `CastlePBR` | 15,195 (exterior only now) | `MI_Castle_PBR` → retained `T_Castle_{D,N,ORM}` — unchanged |
| 2 | `CastleInteriorPBR` | 3,555 (**appended**) | **NEW** `MI_Castle_Interior_PBR` from master `M_AssetPBR` → the 4096² interior set |

- **Why the spec's "slot count + order unchanged" default was impossible to honor while also delivering its own ordered 4096² interior set:** one UE material slot samples one texture set; a fresh interior set + byte-identical exterior textures + no exterior re-bake (GH-R5/H2 NO) cannot share 2 slots. Appending (never inserting) keeps indices 0/1 and every pre-existing face assignment byte-stable.
- **Runtime safety, verified at source BEFORE authoring (not assumed):** every runtime material write in `Castle.cpp` is slot-0-only — `ApplyTeamVisuals` (`:701`), `ApplyCrumbleStage` (`:979` — the board line "writes both slots by index" does NOT match the code). Slots ≥1 always come from the saved mesh asset, so slot 2 renders its design-time MI through team recolor, crumble swap, and `ResetCastle` with **zero code change**.
- **⇒ TASK-634 DUTY (the flagged CODE DEPENDENCY):** audit that no other site enumerates/indexes castle material slots (e.g. `GetNumMaterials` loops, destroyed-state paths) and record the 3-slot contract beside the `ApplyCrumbleStage` doc.

## 4. THE BAKE (recipe + declared departures)

Mirrors `refine_trellis_glb.py::bake_all`: Cycles CPU (the pipeline ruling), DIFFUSE `pass_filter={COLOR}`, ROUGHNESS, NORMAL tangent, numpy ORM pack R=AO/G=Rough/B=0. Departures, each declared: **self-bake** on an interior-only temp copy (no dense donor ⇒ no cage/ray), **margin 32 px** (16@2048 scaled to 4096), **AO via AmbientOcclusion-node→EMIT with distance pinned 2.5 m = 250 uu** — a closed interior needs a finite AO radius (the convex fleet exteriors never did), and the node input is version-proof where `world.light_settings` is not.

**⭐ THE ITERATION LEDGER (3 attempts, each failure named — the next baker must not rediscover these):**

| attempt | failed on | mechanism | fix that carried forward |
|---|---|---|---|
| *(pre-bake)* UV run 1 | GH-R1 snapshot guard, **aborted before save** | object-mode `select` flags do not survive the edit-mode selection flush — `smart_project` leaked onto exterior faces | edit-mode bmesh selection with explicit `select_set` down-flush + in-run snapshot compare |
| **v1 bake** | anti-bleach 1.92× / instrumented AO defect | the AO pass ran with the ORIGINAL SM_Castle still render-visible, **coincident** with the bake copy → every AO ray died at ~0 (AO mean **0.1197**) → the delight AO-divide floor-clamped into a ~4× lift → chalk (L* 82); the near-black ORM.R would also have crushed the interior in-engine | hide ALL other objects from render during bakes + the AO-node distance lane ⇒ AO covered mean **0.7339** (p5 0.008 crevices, p95 1.000 open walls) |
| **v2 bake** | the EYEBALL gate (numbers passed) | voronoi-cell stone read as white crazy-paving mosaic, not masonry; 0.38 covered albedo read ivory under warm light | v3: coursed Brick-texture walls (0.9 × 0.45 m running bond, ~2.5 cm mortar, axis-selected mapping per wall orientation), voronoi kept only for the flagstone floor, palette ~30% darker + warmer |
| **v3 bake** | — SHIPPED | all gates §5 + eyeball §6 | — |

**Delight (the DEFAULT-TRAP law honored — pinned, never the script default):** `{ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` + shoulder 0.80 / max_out 0.98 also pinned. v3 metrics (all-pixel, the pipeline's own metric): mean linear **0.0318 → 0.2050**, p99 0.6514, shoulder fraction 0.0000.

## 5. THE RE-OPENED GATES — MEASURED (GH-R5; scope = exactly the re-exported set)

**5.0 Instrument validation FIRST (building method law):** recomputed `mean_linear_after` from the shipped D PNGs vs each asset's recorded `refine_report.json` value — **Footman 0.1639 Δ0.0000 · Knight 0.1639 Δ0.0000 · Cleric 0.3614 Δ0.0000 · Castle 0.1082 Δ0.0000 — 4/4 PASS to 4 dp.** Decode byte-identical to `refine_trellis_glb.py::_srgb_to_linear`; covered texel = any nonzero stored channel (the fleet rule); chroma metric named: mean CIELAB C*ab (D65) on covered texels.

**5.1 Albedo floor — ✅ PASS.** UV-normalised covered mean linear **0.3545** vs floor **0.2536** (40% headroom). Raw all-pixel **0.2050**, reported-only per the amended gate. (Exact-bake-mask covered mean **0.3092** — margin texels inflate the nonzero-rule number; both stated.)

**5.2 Anti-bleach — ⚠️ DECLARED BAND-FLAG on one of two masks, operative guard NOT tripped, eyeball gate carries it.** No interior concept PNG exists, so the reference is a DECLARED intent card (`debug/intended_stone_reference.png`): the authored palette pushed through the pinned delight at AO=1, area-weighted 78/7/15 stone/mortar/floor — mean linear 0.2532. Against it: luma retention **1.2214 (exact bake mask — IN band)** / **1.4008 (nonzero rule — over band)** vs 0.85–1.25. The nonzero overshoot is structural, not chalk: the reference assumes AO=1 while the shipped delight divides by the real AO≈0.73 field (≈1.2× mean lift — the step doing exactly its job), and the 32 px margin texels (58% of nonzero pixels at this coverage) skew bright. **The LAW's bleach trigger (retention >1.25× AND UV-norm >0.60) is NOT tripped — UV-norm 0.3545.** Baked read: mean L* 62–65, mid-grey stone. Previews eyeballed (§6); Jonathan's eye at the 636 checkpoint stays the final authority.

**5.3 Chroma-fidelity — ✅ PASS (secondary/non-gating for a build with no concept).** Baked C*ab **4.44** vs intended 3.95 ⇒ retention **1.12×**, hue delta **0.4°** (84.0° vs 83.6° — the warm-grey read held exactly). Nothing near the 0.60×/1.35×/20° fences.

**5.4 Retention (exterior byte-identity) — ✅ PASS, 7/7.** sha256 before == after across the whole task: `Content/RawAssets/Textures/Castle/T_Castle_{D,N,ORM}.png` (`b9368221…`/`8f3aacca…`/`3a19a861…`), `Content/Textures/T_Castle_{D,N,ORM}.uasset` (`24f2137a…`/`9b1debd6…`/`aab969b4…`), `Content/RawAssets/Castle.fbx` (`8b96f2b6…ae7b2` — retires at 633, not here). GH-R5's exterior scope never re-opened: zero exterior texels re-delivered.

**5.5 ORM sanity:** covered AO mean 0.7339 · roughness 0.757–0.898 · metallic max 0.0000.

## 6. EYEBALL GATE — ✅ v3 PASSES (previews rendered from the shipped PNGs, Cycles, warm interior light rig)

`redesign_textures/previews/int_{hall_to_door,hall_to_north,mouth_to_hall,floor_east}.png`, eyeballed against 629's untextured baseline. v3 reads as BUILT masonry: running-bond block courses on every wall orientation (the axis-selected mapping holds through the arch and jambs), dark mortar seams, flagstone floor, AO grounding the wall-floor junctions; the through-door view is a lit room, not a void. **Two honest notes for Jonathan's 636 eye:** (a) under the warm preview rig the walls sit on the BRIGHT side of mid-grey (measured albedo L* 62 — deliberately the safe failure direction for this project; the one-lever darken is the v3 palette × a constant + a ~15 min re-run); (b) at full hall distance the 0.45 m courses read fine-grained — if he wants chunkier castle blocks, `BRICK_W`/`ROW_H` in `t630_material_v3.py` are the two constants. **Pre-existing observation (NOT introduced, NOT changed — GH-R1):** a few toe rocks at the gate mouth are TeamRegion-slotted faces of the shipped exterior and render team color in-engine (the colored blobs in the door view). If that read bothers him it is a slot-assignment tweak on SHIPPED faces = its own ruling, not smuggled into this task.

## 7. WHAT TASK-633'S IMPORT MUST CARRY (the contract)

1. **Textures:** import `redesign_textures/T_Castle_Interior_{D,N,ORM}.png` → `/Game/Textures/T_Castle_Interior_{D,N,ORM}` — D sRGB ON; N linear (sRGB OFF, normal compression); **ORM `TC_Masks`/linear, sRGB OFF** — then re-verify every material referencer still compiles (the SAMPLER-TYPE trap; `M_AssetPBR`'s node default is the NeutralORM so the master is safe, but check).
2. **Material:** create **`MI_Castle_Interior_PBR`** from master `/Game/Materials/M_AssetPBR` (params `BaseColor`/`Normal`/`ORM` → the three new textures). The retained `MI_Castle_PBR` is NOT touched.
3. **Mesh:** the FBX export (633 owns it) must carry slot order **`[TeamRegion, CastlePBR, CastleInteriorPBR]`** and re-apply the `ue_handedness_precomp` mirror; after the same-path reimport, bind slot 2 → `MI_Castle_Interior_PBR` and read back all three slots by name+index. Exterior texture assets are NOT reimported (the texture-skip trap is irrelevant here BY DESIGN — the exterior set must stay untouched; import the interior set explicitly).
4. **TASK-632 (crumble trio) duty:** each crumble stage inherits 3 slots from the redesign blend — bind **all three** on each stage mesh at design time (`MI_Castle_Crumble0N` on slots 0/1 per `WR-§1`; slot 2 = `MI_Castle_Crumble0N` too, OR an interior-crumble MI if authored — `ApplyCrumbleStage` never writes slot 2 at runtime, so whatever is saved is what renders mid-match).
5. **TASK-634 (flagged code dependency):** §3 audit duty — verify no code path enumerates castle slots by count; record the 3-slot contract.

## 8. WHAT I DID NOT DO, ON PURPOSE

⛔ No FBX export (633 owns it; `Castle.fbx` hash-verified untouched). ⛔ No exterior re-bake, no exterior UV/texel/slot change of any kind (H2 default NO honored; the D2 fold-in stays Jonathan's open question). ⛔ No manifest/board/git/editor/MCP/Meshy. ⛔ Did not re-slot the 182 high membranes or the team-colored mouth rocks (both shipped reads, both declared). ⛔ Did not chase coverage past 0.203 — the buried fill-plate faces cap it; 5 cm/texel interior vs the exterior's coarser read is the right spend.
