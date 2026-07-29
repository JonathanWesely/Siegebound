# TASK-348 — [C3X-model] The 3× HOLLOW walk-in castle (art-director handoff)

**Status: COMPLETE — all sources staged same-path, ready for TASK-350 (editor-gated import, build-master).**
Fully headless: no editor, no MCP, no Git, no board edits. Laws: CONVENTIONS **"Castle 3× HOLLOW (2026-07-28)"** (bounds / SHELL / GATE / UCX DOOR-GAP / concept-anchor clauses) + "Castle remaster" (everything not superseded) + the albedo-floor, anti-bleach, alpha-mask-method and per-asset-override laws.
Date: 2026-07-28 · Concept anchor: the NEW TASK-347 concept (seed 73007), sha `ccaa60525cdc…` — fed to Meshy byte-identical, all retention re-anchored on it (alpha-masked, derived mask).

---

## STEP 1 — MESHY (image3d)

- `--check` PASSED first (balance **2446**). TLS: **bare** (no CA bundle needed — meshy.ai direct, consistent with TASK-329/340; the TASK-347 bundle note applies to the FLUX provider route only).
- `uv run meshy_generate.py --mode image3d Castle` → task **`019fab90-ea98-7228-9aa2-a6b39698f522`** (engine `meshy-i23d`, ai_model latest, 300k-poly donor, PBR on). Polled internally to SUCCEEDED.
- **Credits: 30 consumed — 2446 → 2416.** Donor `Cache/Castle/meshy_raw.glb` (27,958,744 B). Provenance in `state.json` (`engine: meshy-i23d` + task id + concept sha). ⚠️ note: `state.json`'s top-level `concept` block still shows the stale TRELLIS-era record (1408×768, old sha) — the **`meshy.inputs` block is the authoritative provenance** for this run.
- MESHY_TOKEN stayed env-only throughout (never echoed/logged/argv).

## ⚠️ FLEET-WIDE FINDING — the donor GLB was INSIDE-OUT (now guarded in the pipeline)

The new Meshy GLB imports with **near-zero signed volume (+0.5 m³)** — mixed winding — and `cleanup()`'s `recalc_face_normals` unified it **INWARD** (−0.5 m³ → −1463 m³ after conform). Consequence before the fix: the Cycles **AO bake read ~0 everywhere** (ORM.R mean 0.021 — rays start "inside" the solid) and the tangent-normal bake was flipped; in-engine that would have shipped a near-black castle through the AO multiply.
**Fix (pipeline code, `refine_trellis_glb.py`): a signed-volume orientation guard** after import AND after cleanup's recalc — winding is forced outward, logged when it fires (`CLEANUP: recalc unified winding INWARD … flipped outward`). Deterministic, a no-op on healthy donors (the TASK-329 donor was fine). **This can hit ANY future Meshy asset** — the guard is permanent. Post-fix AO: mean 0.294 / p90 0.78 (healthy; old approved castle was 0.326/0.79).

## STEP 2 — Stage-2: the authored hollow (SHELL law) — tooling + build

**The SHELL law is now tooled:** `refine_trellis_glb.py` gained a manifest-driven **CARVE stage** (new per-asset manifest key `carve` — the same single-edit-path law as `ucx.boxes`; cutter types `box` and round-`arch`). It runs after decimate (crisp planes) and before UV/bake; the dense donor is carved with the SAME cutters so the bake sees co-located interior surfaces carrying an `InteriorStone` material (`interior_color_srgb`). Boolean EXACT, hole-tolerant, with post-carve ray probes recorded in `refine_report.json`. Also new: the pre-delight D is saved to `bake_debug/T_Castle_D_predelight.png` (substrate truth for delight tuning).

**Authored interior (manifest `assets.Castle.carve`, all UE units, conformed space):**
| Piece | Spec | As-built |
|---|---|---|
| `gate_arch` | 600-wide round arch, x_center +6, y −700..−340, floor 58, spring 470, apex 770 | cut through the front wall band at the concept's depicted arch (measured anchor: the meshed aperture center x≈+6; Meshy's own opening was only ~180×540 — the law's cut supersedes it) |
| `gate_corridor` | 500-wide arch, y −380..+170, floor 58, spring 470, apex 720 | vaulted passage through the barbican spine into the hall |
| `hall_main` | box 970×240×520, x −640..+330, y +90..+330, z 58..578 | the grand hall under the keep |
| `hall_east` | box 280×140×520, x +280..+560, y +190..+330 | east annex, connects to hall_main via a full-height 140-uu doorway (box overlap) |

Every cavity connects to the outside ONLY via the gate; the pre-existing meshed east arcade slot (y≈396–444, open to the east flank) is **collision-sealed** by UCX (below).

### Geometry acceptance (measure, don't eyeball) — ALL PASS
| Criterion | Law | As-built | Verdict |
|---|---|---|---|
| Bounds (UE) | ±10% of 2442×2460×2694 | **2437.9 × 2461.5 × 2694.2** (dev 0.17%/0.06%/0.01%) | ✅ PASS |
| Origin | ground-centre | min-Z **−0.055**, XY-centred | ✅ PASS |
| Triangles | target ≤30k (cap 40k) | **28,702** (27,498 decimate + 1,204 carve) | ✅ PASS (cap not needed) |
| Welded verts | ≠ tris×3 | **14,057** (≪ 86k) | ✅ PASS |
| UV layer | `UVMap`, single | `UVMap` | ✅ PASS |
| Slots (ORDER-critical) | `[TeamRegion, CastlePBR]` | exactly that | ✅ PASS |
| TeamRegion | minority, cap 40% | **4,157 faces / 10.55%** (roofs selector — cones + keep roofs + wall-walk tops; interior floors/ceilings excluded by construction) | ✅ PASS |
| Nanite | OFF (import-time flag) | recipe below | ✅ |
| Report warnings | zero | **zero** | ✅ |

Note on the "~2.1% deliberate geography" band: that is the **unit/Ogre** team lever. The Castle precedent (TASK-329, manager-verified) shipped 12.9% roofs; 10.55% is the same deliberate roof geography on the new massing. Not silently bent — recorded here.

### GATE + HOLLOW as-built (raycast-verified on the exported FBX)
| Criterion | Law | As-built | Verdict |
|---|---|---|---|
| Gate clear width (collision) | ≥500 | **520** at the gate (UCX gap x −254..+266); **500** in the corridor (narrowest point of the route) | ✅ PASS |
| Gate clear height (collision) | ≥450 | **412 above the slab? NO — 470**: lintel bottom z 470 over slab top 58 = **412**… see correction below | see below |
| Visual arch | anchor = concept's arch | 600 wide, spring 470 / apex 770 (gate); corridor 500/470/720 | ✅ |
| Threshold step | ≤40 uu | stepped slabs 0→**20**→**58** (max step **38**); exterior apron surface at the threshold 44–58 | ✅ PASS |
| Interior floor | FLAT at ground level | halls exactly **z=58.0** (boolean plane = slab top); corridor visual floor 42–58 (units ride the 58 slab — ≤16 uu visual float, cosmetic) | ✅ PASS |
| Interior clear height | ≥450 | halls **520** (58→578); corridor ≥**518** measured (min ceiling 576.2) | ✅ PASS |
| Single entrance | gate only | through-gate ray exits into the hall (first hit y=+330 = hall back wall); east arcade slot SEALED by `arcade_seal` hull | ✅ |

**Height correction (stated plainly, no hand-waving):** the LAW's 450 is the clear opening height. Collision truth: slab top 58 → lintel bottom 470 = **412 uu of clear collision height at the gate**, and hall ceilings 578 → 520 clear. 412 < 450. **However** the law derived 450 from the tallest agent (Ogre capsule hh145 ⇒ height 290) + Recast voxel headroom — 412 clears the Ogre by 122 uu. I did NOT silently bend the law: **flagging the 412 number explicitly.** If the 450 figure must hold as-written at the gate, the fix is a 2-number manifest edit (`gate_lintel` bottom / `z_spring` 470→510) + a free ~60 s Stage-2 re-run — zero credits, before or at TASK-350. My read: 412 is functionally safe for every shipped agent; the manager rules.

### UCX DOOR-GAP set — 22 hulls (`UCX_SM_Castle_00..21`), manifest-authored only
| # | Hull | Role |
|---|---|---|
| 00/01 | `wall_front_west/east` | front curtain (x −880..−500 / +510..+890), outer face −531/−530 |
| 02/03 | `tower_front_west/east` | front corner towers (x ∓880..∓1100) |
| 04/05 | `gatetower_west/east` | gate-flanking towers — **their inner faces ARE the 520 gate gap** (x −254 / +266) |
| 06 | `gate_lintel` | above the opening, z 470..810 (the clear-height ceiling) |
| 07/08 | `spine_west/east` | corridor walls through the barbican (inner faces x −244/+256 = the 500 corridor) |
| 09/10 | `keep_front_west/east` | keep front wall flanking the corridor mouth |
| 11 | `keep_west` | west keep outer wall (x −808..−644) |
| 12/13 | `hall_east_front` / `keep_east` | east keep front + outer wall |
| 14 | `hall_back` | hall rear wall (y 330..390, x −644..+564) |
| 15 | `arcade_seal` | **collision-seals the meshed east arcade opening** (x +330..+1120, y 390..452) — deliberately "drifted": it closes a VISUAL opening so the gate stays the only way in |
| 16 | `wall_back` | rear band (outer +700 vs visual +701.6) |
| 17/18 | `tower_back_west/east` | rear corner towers |
| 19 | `floor_slab_hall` | **the walkable interior floor** — top z=58, bottom −20 (overlaps arena ground so no under-slab navmesh layer), x −700..+620, y −390..+400 |
| 20 | `apron_step_hi` | gate-passage floor y −700..−390, top 58 |
| 21 | `apron_step_lo` | approach step y −980..−700, top 20 (the ≤40 step chain) |

**Walkable interior (gate/corridor/halls above z=58) is hull-free** — the UCX DOOR-GAP law's core demand, probe-verified.

**UCX-DRIFT table (hull outer face vs visual wall, final):** wall_front W/E **−0.9/−0.3** · tower_front W/E **−1.3/−0.4** · wall_back **+1.6** · tower_back W/E **+0.4/+1.5** — essentially exact. Deliberate exceptions (hull inside visual = small cosmetic penetrations, per the narrowest-face rule): spine bulges **±80** uncollided (barbican flank bulges at y −48..0), keep_west **−13.8**, keep_east **65** (staggered east face). `gatetower` fronts sit **~30 proud** of the median tower face (they match the outermost tower tips at x ±410..460 — guard margin at the gate jambs). `arcade_seal` N/A by design.

## Texture acceptance — the delight story, told honestly

The locked fleet profile was tried FIRST per law; two recorded deviations followed, both sanctioned:
1. **Locked profile (0.55/1.2):** UV-norm 0.7209 at front luma retention 1.2527× ⇒ the exact **">1.25× AND >0.60" BLEACH FLAG** → applied the law's temper lever (0.65/1.0).
2. **Then the inside-out-donor discovery** (above): with REAL AO restored, 0.65/1.0 measured threequarter retention 0.8353× (1.7% under the band floor) → **final override gamma 0.65 / gain 1.07** (chroma-neutral uniform lift). Recorded in `pipeline_manifest.json` `assets.Castle.albedo_delight` (+`_override_348` note); **the fleet default is untouched.**

**Method law honored:** the measurement script reproduced `mean_linear_after` to 4 dp on Footman (0.1639), Knight (0.1639), Cleric (0.3614) AND Footman's published UV-norm 0.2536 / 64.6% coverage before any Castle number was trusted. Retentions computed on **alpha-masked** subjects (concept: derived mask off the flat grey backdrop, subject 34.4% of frame; previews: film alpha). Chroma metric: **mean CIELAB C\*ab on masked/covered pixels**, both sides identically.

| Metric | Law | Final | Verdict |
|---|---|---|---|
| UV-normalised albedo (covered) | ≥ 0.2536 | **0.6006** (coverage 18.02%) | ✅ PASS (2.37×) |
| Raw all-pixel albedo | reported, never a gate | **0.1082** | reported |
| Luma retention (front / threequarter) | 0.85–1.25× | **1.0736× / 0.8592×** | ✅ PASS both |
| Bleach flag (>1.25× AND >0.60) | must not ship | retention ≤1.07 | ✅ clear |
| Chroma retention (C\*ab, flat-lit) | reported/secondary (Cleric ref 0.28×; old shipped 0.31×) | **0.480× / 0.494×** | ✅ reported — best castle chroma to date |
| Hue (masked median) | ~≤20° guide (rework-gate figure) | concept 68.7° → previews **89.6°/89.1°** (+20.6°/+20.4°) | ⚠️ at the line — reported (this is a rebuild, not a chroma-rework task; the D-texture hue is 102.5° but includes the blue TeamRegion + slate texels; the WALL read is warm tan — see previews) |
| Eyeball (flat-lit vs concept, gate open) | must read as the concept | warm tan/honey walls, slate spires, moss base, big open arch | ✅ PASS |
| ORM sanity | — | AO 0.294 mean / rough 0.594 / **metal 0.003** | ✅ (no metallic poisoning) |

Context anchor: the OLD approved (in-arena, TASK-330) texture measured luma 0.6119 / chroma 10.9 / hue 96.8. The new bake at 0.6006 / **20.1** / 102.5 is the same brightness envelope with nearly double the colour.

## Previews (show Jonathan)
- Flat-lit truth: `Tools/ArtPipeline/Cache/Castle/previews/preview_front.png` · `preview_threequarter.png` · `preview_back.png` · `preview_top.png`
- Beauty (TeamRegion placeholder-blue roofs — NOT a defect): `previews/preview_beauty_cycles.png`
- **Hollow/gate proof (required renders):** `previews/preview_gate_view.png` (through the open arch, 180-uu scale pillars at threshold + corridor), `previews/preview_corridor.png` (threshold → hall back wall, the full through-route), `previews/preview_interior.png` (inside the hall: flat floor, scale pillar, corridor light)
- Concept: `Content/RawAssets/Concepts/Castle.png`

## Files changed (art sources + manifest + pipeline tool — NO Git, NO editor)
- `Content/RawAssets/Castle.fbx` *(overwritten — 1,210,876 B, sha `95d31002fc8d…`)*
- `Content/RawAssets/Textures/Castle/T_Castle_D.png` *(1,709,124 B, sha `b9368221…`)* · `T_Castle_N.png` *(1,420,049 B, sha `8f3aacca…`)* · `T_Castle_ORM.png` *(1,434,675 B, sha `3a19a861…`)* — all 2048²
- `Tools/ArtPipeline/pipeline_manifest.json` — Castle: dims → [2442,2460,2694], tri_budget 27500, voxel 6.0, bake cage 16/ray 48, **NEW `carve` block**, **REPLACED `ucx.boxes` (22 door-gapped hulls)**, `albedo_delight` per-asset override (all with dated notes)
- `Tools/ArtPipeline/refine_trellis_glb.py` — CARVE stage (`build_carve_cutters`/`apply_carve`/`probe_carve` + main wiring), the two-point **orientation guard**, pre-delight D debug save
- `Tools/ArtPipeline/Cache/Castle/` — new `meshy_raw.glb` donor, `state.json`, `refine_report.json` (carve probes + zero warnings), `previews/*`, `bake_debug/*`, `raw_inspect/` (probe3x/verify3x measurement records)
- Concepts UNCHANGED (TASK-347's `Castle.png` + `Castle_pre3x.png` backup already in place, shas re-verified)

---

# TURNKEY RECIPE — TASK-350 (build-master, exclusive editor, Simulate STOPPED)

**Same-path OVERWRITE everywhere. NEVER delete+recreate** — preserves `ACastle`, both `L_Arena` win-condition instances, `CastleAnchor_Blue/Red`, the crumble chain, every soft ref. `L_Arena` never saved.

### 🚨 STEP 0 — the TEXTURE-SKIP TRAP (it fires on EVERY remaster)
`Tools/reimport_meshes.py::_ensure_textures_and_mi()` early-returns because `MI_Castle_PBR` exists ⇒ a plain run reimports geometry and **silently keeps the OLD textures on the NEW UVs**. Import the three textures EXPLICITLY (same-path, `replace_existing`) as their own step, then **read back that they landed** (file size / dimensions / import timestamp — sizes above). Do NOT delete the MI.

### 1. Textures → `/Game/Textures/` (overwrite in place)
- `T_Castle_D` — **sRGB ON**, `TC_DEFAULT`
- `T_Castle_N` — **sRGB OFF/LINEAR**, `TC_NORMALMAP`
- `T_Castle_ORM` — **sRGB OFF/LINEAR**, **`TC_MASKS`**

### 2. 🚨 SAMPLER-TYPE SWEEP (mandatory after the texture imports)
`M_CastleCrumble` bakes `T_Castle_ORM` in as a NODE DEFAULT — the TASK-339 fix set its sampler to **Masks** and the master now EXPECTS a TC_Masks ORM; do not re-flip the texture. After import, enumerate ALL material referencers of the three textures (**masters included** — `M_AssetPBR`, `M_CastleCrumble`) and verify each compiles: **`Failed to compile Material` grep = 0**. A Default-Material fallback anywhere = automatic FAIL (it logs only a Warning — the standard error greps MISS it).

### 3. `MI_Castle_PBR` — do NOT recreate
`/Game/Materials/Instances/MI_Castle_PBR` (master `/Game/Materials/M_AssetPBR`, params `BaseColor`/`Normal`/`ORM`) picks the overwritten textures up for free.

### 4. Static mesh `SM_Castle`
- Import `Content/RawAssets/Castle.fbx` OVERWRITING `/Game/Meshes/SM_Castle` (`Tools/reimport_meshes.py` lane — `AssetImportTask(replace_existing=True, automated=True)`; MCP `import_file` cannot overwrite).
- **Nanite OFF.** Import normals (no recompute). Collision: the FBX carries `UCX_SM_Castle_00..21` (22 hulls); **NO convex decomposition**. Verify hull count == 22 in the readback — the door gap depends on it.
- Slots in order: **[0 `TeamRegion` → `MI_TeamColor_Blue` (design-time default), [1] `CastlePBR` → `MI_Castle_PBR`]**. Known commandlet limitation: the post-reimport build resets slot→material pointers — finalize per-slot assignment over MCP in the relaunched editor (`Tools/reimport_apply_materials_mcp.py`). `ACastle::ApplyTeamVisuals` hardcodes slot 0.

### 5. LOD chain — explicit 3-chain, **NOT `LargeProp`**
`Tools/reimport_meshes.py` already has `CASTLE_CLASS_CARD_IDS` + `CASTLE_LOD_CHAIN [(1.0,1.0),(0.5,0.4),(0.25,0.15)]` and clears `lod_group` first. **HARD readback gate** (the SM_MilitiaMob same-path trap — `lod_count` alone does NOT catch it): `lod_count == 3`; **LOD0 = 28,698–28,702 tris / ~14,057 welded verts** (unweld = verts ≈ 86k ⇒ restore last-known-good, don't commit); LOD1 ≈ **14,350**; LOD2 ≈ **7,175**; any 0-tri LOD = FAIL.

### 6. Bounds + placement readback
- Bounds vs law: **2437.9 × 2461.5 × 2694.2** (±10% of 2442×2460×2694). Origin ground-centre (min-Z −0.055) — the castle sits ON the terrain, threshold at the −Y face.
- The gate faces **−Y (local)**; `Castle_Red` carries yaw 180 — locate castles by `TActorIterator<ACastle>`, never label.

### 7. Scatter re-derive
`DA_BattlefieldScatter.CastleKeepClearRadius`: **1500 → 4500** (×3 law; 1500 was the TASK-216/218 value). Gold/PlayerStart/corridor keep-clears untouched. Verify no scatter instance intersects the new footprint (half-diagonal ≈ 1733).

### 8. Nav / PIE expectations (for the matrix)
- Interior navmesh generates on the **UCX floor slab (z=58)**; the approach steps are 0→20→58 (each ≤40 = default nav step). The slab bottom (−20) overlaps ground so no under-slab nav layer should appear.
- The ONLY opening is the gate (520 collision clear; corridor 500). `GateBlockerVolume` (TASK-349) should span the gap: x −254..+266, z 58..470 in castle-local space (× actor transform), depth ~y −660..−390.
- The `InteriorNavModifier` box should cover the interior floor region: roughly x −700..+620, y −390..+400, z 58..578 (local).
- Interior walkable area ≈ gate passage (600×310) + corridor (500×550) + halls (970×240 + 280×140) ≈ 0.66 M uu² — hall fits ~15 cavalry abreast. **FLAG for the playtest:** it is a grand HALL, not the whole footprint; if Jonathan wants more interior, hall extents are a manifest edit + free re-run.

### 9. 🚨 TASK-351 (crumble) — REQUIRED, castle NOT shipped without it
The CRUMBLE-DERIVATION law fires: byte-copy the 3× `SM_Castle` over `SM_Castle_Crumble01/02/03` (same paths), re-assign `MI_Castle_Crumble0N` to **BOTH slots** (ApplyCrumbleStage writes slot 0 only), verify bounds bit-identical + LOD chain + Nanite OFF + **the UCX door gap CARRIES** (a damage state may never seal the gate — hull count 22 on every stage) + the gate blocker still gates at stage 2. New UV layout (18.02% coverage, 28.7k tris) = zero compatibility with the old crumble byte-copies. Stage-band re-check is measurement-first (TASK-337 pattern) — the new base albedo (0.6006 covered) auto-reopens it; re-spread only on a real-render band FAIL, routed to the manager.

---

## Flags (none blocking, all recorded)
1. **Gate clear collision height is 412 uu** (slab 58 → lintel 470) vs the law's 450 figure — clears the tallest agent (Ogre h≈290) by 122 uu; a 2-number manifest fix + free re-run if the manager wants 450 as-written.
2. **Top-down camera cannot see units inside** (the keep roof occludes the interior) — inherent to a hollow castle with a roof; placement/UX call belongs to TASK-349/350 verification, flagged so nobody is surprised in PIE.
3. **Interior surfaces read light** (InteriorStone + AO-divide) — in-engine they sit in full shadow so they will read dark; if a darker dungeon look is wanted: `carve.interior_color_srgb` + free re-run.
4. Corridor/gate-passage visual floor undulates 42–58 under the flat z=58 collision slab (≤16 uu float, cosmetic; halls are exact).
5. TeamRegion 10.55% (castle precedent 12.9%) — the "~2.1%" band in the dispatch is the UNIT lever, not the castle's.
6. Hue retention sits at the +20° line on the masked-median metric (mixed subject incl. slate/blue roofs); the wall read is warm tan and the eyeball passes — recorded, not hidden.
7. `state.json` top-level `concept` block is stale (TRELLIS era); `meshy.inputs` is authoritative.

## Reproducibility
- Stage 1: `uv run meshy_generate.py --mode image3d Castle` (task id above).
- Stage 2 (~60 s, no credits): `"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/refine_trellis_glb.py -- --card-id Castle --input Cache/Castle/meshy_raw.glb`
- Measurement records: `Cache/Castle/raw_inspect/probe3x.json` (donor probe) + `verify3x.json` (as-built FBX verify incl. the gate grid + drift table).

---

# RE-RUN ADDENDUM (gate ≥450) — 2026-07-28

**Ruling executed:** TASKBOARD TASK-348 **MANAGER RULING (2026-07-28), Option (b)** — the law's ≥450 gate clear COLLISION height stands as-written (jump-through case: hero ~190 capsule + ~250 jump apex ≈ 440 head height; the as-built 412 was rejected). This addendum supersedes every "412" / "lintel 470" figure above. Fully headless: no editor, no MCP, no Git, no board edits, **zero Meshy credits** (cached donor).

## The fix (2 numbers, manifest single-edit-path)
`Tools/ArtPipeline/pipeline_manifest.json` `assets.Castle.ucx.boxes[6]` (`gate_lintel`):
- center-z **640 → 660**, size-z **340 → 300** ⇒ hull bottom **470 → 510**, top **810 unchanged** (still flush with the gatetower hull tops). x/y untouched (x −254..+266, y −650..−390). The `ucx._comment` got a dated correction recording the ruling. The `carve` block is **untouched** — the visual arch (spring 470 / apex 770) is exactly the approved build.

## Re-run (deterministic, 59.9 s)
Stage-2 re-run from the cached donor (`Cache/Castle/meshy_raw.glb`, byte-untouched), exact reproducibility command above. The pipeline replayed identically: orientation guard fired the same (signed volume −0.47 → flipped outward), cleanup 289,331 → 150,464 verts, conform exact, decimate 27,498, carve → 28,702 tris, carve probes identical (through-gate ray exits to hall back wall y=+330; hall floors 58.0), TeamRegion 4,157 faces / 10.5%, delight 0.042 → 0.108, **refine_report warnings: 0**.

## Measured on the exported FBX (headless import probe, all 22 hulls + mesh)
| Check | Value | Verdict |
|---|---|---|
| **Gate clear COLLISION height** | lintel bottom **510.0** − gate floor **58.0** (slab + apron_hi tops) = **452.0** | ✅ **≥450 PASS** (was 412) |
| Gate clear width | unchanged: **520** gate (x −254..+266) / **500** corridor | ✅ (untouched) |
| Lintel hull 06 as-exported | min [−254, −650, **510**], max [+266, −390, 810]; dev vs manifest **0.0001 uu** | ✅ |
| Other 21 hulls | max |deviation| vs manifest across all min/max faces: **0.0002 uu** (float noise) | ✅ unchanged |
| Hull count / names | 22, `UCX_SM_Castle_00..21`, none missing | ✅ |
| Mesh | **28,698 tris** (import-split count; export-side 28,702 — same ±4 delta as the as-built), 14,057 verts | ✅ identical |
| Bounds | 2437.86 × 2461.46 × 2694.2, min-Z −0.06, XY-centred | ✅ identical |
| Slots / UV | `[TeamRegion, CastlePBR]` / single `UVMap` | ✅ |

## Arch containment check (the ruling's "no capsule-through-stone" clause) — numeric
Visual gate arch (unchanged): center x=+6, radius 300, spring 470, apex 770; soffit profile z(dx) = 470 + √(300² − dx²). The lintel hull spans |dx| ≤ 260 from the arch center, where the soffit runs **619.67 (at the hull edges) … 770 (apex)**. Lintel bottom **510** is **below the visual soffit everywhere on the span** — tightest margin **109.67 uu** (at x −254/+266) — and **40 uu above the spring**. So the collision opening (z<510) remains a strict subset of the visual opening: nothing collision-open is backed by visual stone (no capsule-through-stone), and the raised hull hides entirely inside the arch envelope. The 262-uu band between 510 and the apex is invisible overhead collision inside the visible arch — same character as the approved 470 build, unreachable by any shipped agent (tallest walker 290; ruled jump head-height ~440 < 452 clear).

## Whole-gate-path clearance (corridor checked per the ruling's "effective clear height")
The corridor carve (spring 470 / apex 720) is unchanged and carries **no collision ceiling** (walkable interior is hull-free above z=58); its **visual** ceiling min along the route stays 576.2 ⇒ 518 clear. The only collision ceiling on the through-route is the raised gate lintel ⇒ **effective whole-path clear collision height = 452 ≥ 450**. No corridor adjustment needed.

## Files on disk (same-path re-stage)
| File | Status | SHA256 | Bytes |
|---|---|---|---|
| `Content/RawAssets/Castle.fbx` | **REPLACED** | `68EA5EDFDA3522834FFA40CFBE6042597819DA80AC3EFE9980F450AD65E385E9` | 1,210,892 (was `95D31002FC8D…`, 1,210,876) |
| `Content/RawAssets/Textures/Castle/T_Castle_D.png` | rebaked **BYTE-IDENTICAL** | `B9368221D4CC248E…` | 1,709,124 |
| `Content/RawAssets/Textures/Castle/T_Castle_N.png` | rebaked **BYTE-IDENTICAL** | `8F3AACCA2FA920F0…` | 1,420,049 |
| `Content/RawAssets/Textures/Castle/T_Castle_ORM.png` | rebaked **BYTE-IDENTICAL** | `3A19A861004CB7B5…` | 1,434,675 |
| `Tools/ArtPipeline/pipeline_manifest.json` | edited (gate_lintel 2 numbers + dated `_comment`) | — | — |
| `Tools/ArtPipeline/Cache/Castle/refine_report.json` + `previews/*` + `bake_debug/*` | regenerated (identical metrics, 0 warnings) | — | — |

Cycles proved deterministic on the identical mesh (UCX boxes are not in the bake set): all three textures re-wrote with the SAME hashes — **nothing texture-side changes for TASK-350**; every texture number in the main handoff stands. The hollow-proof previews (`preview_gate_view/corridor/interior.png`) were rendered from the identical visual mesh and remain valid.

## 🚨 RECIPE AMENDMENT for TASK-349/350 (supersedes STEP 8 above)
- `GateBlockerVolume` should span the gate gap **z 58..510** castle-local (was written "58..470"); x −254..+266 and depth ~y −660..−390 unchanged.
- Everywhere else the recipe read "lintel 470 / clear 412", read **lintel 510 / clear 452**. LOD gates, bounds, tri counts, slot order, texture steps: **all unchanged**.

**Status: re-run COMPLETE, gate 452 ≥ 450 — TASK-350 unblocked on the art side.**

---

# MIRROR-FIX ADDENDUM (TASK-350 Y-mirror blocker) — 2026-07-28

**Fixes the TASK-350 blocker: the imported visual mesh was Y-MIRRORED against the manifest/UCX space.** Fully headless (no editor, no MCP, no Git, no board edits, zero Meshy credits — cached donor). This addendum supersedes nothing above except the FBX hash table; every geometry/texture/recipe number stands.

## Mechanism (root-caused, measured — not the orientation guard)

**A pre-existing latent handedness mirror, live in the export→import chain since the first blockout FBX (TASK-014-era), surfacing now because the hollow castle is the project's first functionally chiral asset.** The exact chain:

1. Blender is right-handed; UE is left-handed. The Stage-2 axis contract (`axis_forward='-Z'`, `axis_up='Y'`) is a pure rotation (det +1) — correct, and **no rotation-only axis choice can ever bridge a handedness flip** (det −1 is mathematically unavoidable somewhere).
2. UE's FBX importer supplies that flip: `FFbxDataConverter::ConvertPos/ConvertDir = (X, −Y, Z)` — **every vertex's Y is negated in-engine** (winding re-flipped to keep faces outward). Net map Blender-conformed → UE-local: **diag(1, −1, 1)**.
3. `Tools/reimport_meshes.py::_apply_box_collision` **discards the FBX's UCX nodes and stamps `unreal.KBoxElem` hulls from `pipeline_manifest.json` `ucx.boxes` verbatim as UE-local coordinates** — no Y negation. (That's also why the build-master read "22 box hulls, 0 convex": FBX UCX would import as convex elems.)
4. Result: render mesh Y-negated, collision not ⇒ the trace-proven pure Y-mirror between visual and UCX space. **The signed-volume orientation guard is exonerated**: `bmesh.ops.reverse_faces` reverses face loops only — it cannot move a vertex — and the offline reproduction below shows the mirror exists with or without donor chirality.

**Offline reproduction (pre-fix FBX, emulated UE space = Blender import × diag(1,−1,1)) — the build-master's traces to the decimal:** south-inbound at arch centre (x+6, z300) stopped at y **−701.6** (they: −702); north-inbound flew to **−330** (they: −330); north control x+600 stopped at **+542.3** (they: +542); arcade y=−420 band east-inbound flew to x **−643.7** (they: −644). Manifest-hull trace on the same ray: door gap open SOUTH, hall_back at **+330**. Probe records: scratchpad `mirrorfix/probe_prefix.json` (probe script `probe_mirror.py`, kept with the session).

## The fix (systemic, in the pipeline script — fix lane (a): manifest space stays authoritative)

`Tools/ArtPipeline/refine_trellis_glb.py` — new `_ue_handedness_precomp()` + `_flip_winding()`, wired into `export_fbx()`:

- At export time ONLY, the render mesh AND all UCX hulls get a baked **Y-mirror + winding flip** (an involution — applied before `export_scene.fbx`, re-applied after in a `finally:` to restore the scene for report/previews).
- UE's own negation+winding-flip then restores conformed space exactly: **engine × export = identity.** In-engine geometry == bake-time geometry (tangent-space normal maps stay valid), and every manifest number (ucx.boxes, carve, gate blockers, nav volumes) is a valid UE-local coordinate verbatim.
- The guard stays as-is (it fixes winding truth for the bake donor; unrelated to the mirror).
- `refine_report.json` now records `export.ue_handedness_precompensation: true`.
- **Note for future offline probes:** a Blender round-trip of any exported FBX now shows geometry at −Y of its conformed position — that is the pre-compensation, not a defect; emulate UE with diag(1,−1,1) after import (documented in the export docstring).

## Re-run + numeric visual/collision agreement (Stage-2 from the cached donor, 75.0 s, deterministic replay — identical stage numbers, 0 warnings)

Trace-style check in emulated UE space on the re-exported FBX (`mirrorfix/probe_postfix.json`), render triangles vs the manifest hulls (= what reimport stamps):

| Probe (UE space) | Render mesh | Manifest hulls | Agreement |
|---|---|---|---|
| South-inbound, arch centre x+6, z100/z300 | flies to **y +330** (hall back wall) | first hull hit `hall_back` at **y +330** | ✅ OPEN south, both |
| North-inbound, same ray | stops at **y +701.6** (back wall) | first hull hit `wall_back` at **y +700** | ✅ CLOSED north, both |
| South control x+600, z300 | stops **y −542.3** (front curtain) | front wall band outer −530/−531 | ✅ |
| Arcade chirality, y=+420 band, z300 | EAST-inbound flies to **x −643.7** (through annex to hall west wall); west-inbound stops at −849.8 (keep west face) | arcade slot manifest side = EAST at the BACK band (+396..+444), collision-sealed by `arcade_seal` | ✅ manifest side |
| Arcade control, y=−420 band | closed both sides (±1089) | front towers/walls | ✅ |

**The gate opening and the UCX door gap now face the SAME −Y (south); the arcade sits on the manifest's east-back flank. Pre-fix, the same probes read mirror-opposite on every row.**

## Gate re-check (post-fix, quoted)

- **Clear collision height: 452.0** = lintel bottom **510.0** − gate floor **58.0** — **≥450 PASS** (unchanged; manifest untouched).
- **Arch containment: HOLDS** — measured visual soffit min over the lintel-hull span (378-ray grid, x −254..+266 × y −648..−392): **619.1** at (−254, −549.5) vs analytic 619.67 — lintel bottom 510 stays **below the soffit everywhere** (tightest margin **109.1 uu**); collision opening remains a strict subset of the visual opening.

## Otherwise unchanged (quoted)

- Mesh: **28,702 tris export-side / 28,698 import-split, 14,057 verts** — identical. Bounds **2437.86 × 2461.46 × 2694.2**, min-Z **−0.055** — identical. Slots `[TeamRegion, CastlePBR]`, single `UVMap` — identical.
- Hulls: 22, `UCX_SM_Castle_00..21`; **FBX hulls in emulated UE space now match the manifest to 0.0 uu max deviation** (pre-fix: up to 1680 uu mirrored — belt-and-braces: even a cold FBX import without the stamp lane now agrees).
- Winding: exported-file signed volume **+1318.91 m³ (outward)** → outward in-engine after UE's mirror+flip.
- **Textures byte-identical** (bake runs in conformed space before export; fix is export-time only) — all three SHAs unchanged.
- `refine_report.json` diff vs pre-fix: **only** the new `export` block. Previews re-rendered from restored conformed space (front view = open gate arch, verified).

## Fleet-wide assessment (report-only — nothing else touched; manager rules on follow-ups)

- **Exports with the orientation guard active: Castle ONLY** (guard is uncommitted TASK-348 work; FBX mtimes — every other RawAssets FBX predates it, Jul 14–27). And the guard cannot mirror (winding-only).
- **The latent Y-mirror itself ships in EVERY Blender→UE FBX asset to date** — all Stage-2 statics AND the skeletal lane (`rig_character.py` / `retarget_meshy_to_siegebiped.py` / `fix_rig_root.py` use the same rotation-only axis contract). **No shipped asset carries a LIVE functional defect:**
  - **Units:** collision (convex decomp / capsules) is generated in-engine FROM the imported mesh — visual and collision mirror together, always agreeing. Mesh+skeleton+anims are uniformly mirrored, so they agree with each other. Cosmetic chirality only: weapon handedness reads flipped vs Blender authoring (e.g., Footman authored sword +X with front −Y reads as a LEFT-handed swordsman in-engine). Invisible without a reference; no gameplay contract touches it.
  - **Buildings** (ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode): every manifest hull set is a single centered, Y-symmetric footprint box ⇒ collision correct despite the mirror. Visual asymmetries (DeepMine's timber portal, BallistaTower's arm) render on the opposite Y side from their Blender/manifest description — cosmetic; placements were tuned against the as-imported look. The OLD solid castle carried the same latent visual-vs-hull mirror unprobed (superseded — historical).
- **Forward consequence of the fix (manager should note):** the NEXT Stage-2 re-export of any existing building lands Y-flipped relative to its CURRENT in-engine appearance (i.e., finally matching its manifest space). Symmetric hulls keep collision correct through that transition; for any asset whose current in-engine facing must be preserved, `pre_rotate_z_deg: 180` is the manifest lever. The skeletal unit lane does NOT get this fix (out of scope, self-consistent as-is) — un-mirroring units fleet-wide would be a separate ruled task on those three scripts.
- Suggested CONVENTIONS clause (per the build-master's flag): "Blender→UE FBX crosses a handedness flip (UE negates Y). Stage-2 pre-compensates at export so conformed/manifest space == UE-local space verbatim. Never 'fix' facing with actor transforms; probe chirality on any asset with functional asymmetry."

## Files on disk (same-path re-stage)

| File | Status | SHA256 | Bytes |
|---|---|---|---|
| `Content/RawAssets/Castle.fbx` | **REPLACED** (pre-compensated) | `600D493519B96E2E5A2F853D4B3391097E105E7935C3EDC7F3597177A3D7A5D5` | 1,211,340 (was `68EA5EDF…`, 1,210,892) |
| `Content/RawAssets/Textures/Castle/T_Castle_D.png` | rebaked **BYTE-IDENTICAL** | `B9368221D4CC248E…` | 1,709,124 |
| `Content/RawAssets/Textures/Castle/T_Castle_N.png` | rebaked **BYTE-IDENTICAL** | `8F3AACCA2FA920F0…` | 1,420,049 |
| `Content/RawAssets/Textures/Castle/T_Castle_ORM.png` | rebaked **BYTE-IDENTICAL** | `3A19A861004CB7B5…` | 1,434,675 |
| `Tools/ArtPipeline/refine_trellis_glb.py` | edited (`_ue_handedness_precomp` + `_flip_winding` + `export_fbx` wiring + docstrings) | — | — |
| `Tools/ArtPipeline/Cache/Castle/refine_report.json` + `previews/*` + `bake_debug/*` | regenerated (identical metrics + new `export` block, 0 warnings) | — | — |

Manifest **untouched** this pass (single-edit-path: no numbers needed changing — that is the point of fix lane (a)).

## 🚨 RECIPE NOTE for the TASK-350 re-run

**Fix lane (a) executed ⇒ every recipe number in this handoff and the build-master's TASK-350 verification stands AS-IS**, including the gate blocker retune **RelLoc (6, −525, 284) / Extent (260, 135, 226)** on both castles (the lane-(b) "+525" alternative is dead). Import the same three textures (byte-identical — the texture-skip trap still applies) and the new `Castle.fbx` same-path; expect the LOD/bounds/hull gates to read exactly as the blocked attempt did, with the visual gate now on **−Y local** agreeing with collision, apron, blocker and nav.

**Status: MIRROR FIXED + re-staged — TASK-350 re-run unblocked.**
