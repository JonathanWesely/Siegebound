# TASK-632 — [GH-7] THE CRUMBLE TRIO RE-DERIVATION — three damage stages of the NEW castle, visual-only, footprint-stable (art-director handoff)

**Status: COMPLETE — Cache-side only, ready for TASK-633 (same-path import + collision apply).**
Fully headless Blender 5.1.2 `--background --factory-startup`. **No Content/ or /Game/ write, no manifest edit, no editor/MCP, no git, no Meshy, no TASKBOARD edit.**
Laws: GH-R1/R6 · `W6-R2` · `WR-§1` (crumble clause) · `TL-§2` · the CRUMBLE-DERIVATION law · cp1252 (script pure ASCII, `encoding=` on every text open). Date: 2026-08-18.

---

## THE ONE-LINE RESULT

`SM_Castle_Crumble01/02/03` are re-derived from `castle_redesign_v1.blend` (sha `dd29effe…74df5`, identity-gated pre-edit) as **geometry-byte-equal stage meshes of the redesigned hollow castle** — per-stage geometry sha256 IDENTICAL to pristine, dims/min-Z/tris/verts anchor-exact, all THREE material slots `[TeamRegion, CastlePBR, CastleInteriorPBR]` carried, the manifest v3 66-hull set embedded as `UCX_` nodes (roundtrip AABB max dev **0.0003 uu** vs manifest), aperture and hall-walkability probe-identical to the pristine at every stage — staged in Cache as `.blend` + `.fbx`, nothing imported.

## 0. ⚖️ THE DESIGN-LANGUAGE ADJUDICATION (why the stages are geometry-identical, stated so nobody reads it as missing damage)

The shipped trio's design language is **material-driven damage staging on geometry-identical meshes**:
- CONVENTIONS "CRUMBLE-DERIVATION LAW": the stages are *"byte-copy DUPLICATES of `SM_Castle`"*; the 75/50/25 reads come from `MI_Castle_Crumble0N` (`Darken`/`ScorchAmount`/`CharColor`/`RoughBoost` staging, the TASK-157 spread).
- `handoffs/TASK-586-buildmaster.md` §119: *"Geometry, LODs, bounds and collision are identical across all four — that is precisely what `WR-§1` wants."*
- The TASK-331 stage-verification renders show it in pixels: stage 3 is a fully STANDING castle, charred near-black — zero geometric demolition.

This derivation reproduces that language against the redesigned mesh. It is also the only shape that satisfies `W6-R2`'s footprint clause EXACTLY: geometric damage under invariant collision would create visual-vs-collision mismatch (invisible walls / walk-through rubble) — the defect family this project has spent the CASTLE-REPAIRS and GREAT-HALL waves killing. The board-specced requirement *"interior damage reads must show the NEW hollow hall"* is satisfied by construction: every stage IS the new hollow hall.

## 1. DELIVERABLES (all under `Tools/ArtPipeline/Cache/Castle/`)

| artifact | bytes | sha256 |
|---|---|---|
| `castle_redesign_crumble01.blend` | 1,278,267 | `c63e37b099f605baf752f19bfa4eb003c0b28e7e6c51cdca3b2f98c3fc24e821` |
| `castle_redesign_crumble02.blend` | 1,278,147 | `58b4e2c04b9b34a65010040b366651959fba053dae82c167cc866fd4918f4cdb` |
| `castle_redesign_crumble03.blend` | 1,278,139 | `cf01472039276860b8339c37cca15ceaf1762fde1f0e9807f405c4f65a80fdbe` |
| `castle_redesign_crumble01.fbx` | 1,318,156 | `4ac13ea5b2e106f94bdc460ec81d0efbae0f8dcdaa7e0fa616ddfec274abe325` |
| `castle_redesign_crumble02.fbx` | 1,318,156 | `65062cb54e623b81689fb985484c190cd8cc1f2faa7dc296e48790a5e8d87493` |
| `castle_redesign_crumble03.fbx` | 1,318,156 | `c30dde41a76cf5976db1236e351ac24833bed5e917639047e1adf4f36d3abf79` |
| `t632_crumble_derive.py` (reproducible, the whole task) | 23,206 | `c6c14f11…3ac5a941` |
| `t632_report.json` (every number below, machine-readable) | 15,256 | `3bffc160…1ab36f4174` |
| `previews_crumble/` — 9 renders, eyeballed: `crumble0N_{ext_front_west, int_hall_to_door, int_mouth_to_hall}.png` (629's exact camera poses/Workbench rig — directly comparable to the 629 baselines) | — | — |
| `t632_derive_log.txt` | — | — |

### ⛔ NOT TOUCHED
`Content/**` · `pipeline_manifest.json` (read-only; sha re-verified) · `castle_redesign_v1.blend` (read-only; sha re-verified) · any `.uasset` · `L_Arena` · git · TASKBOARD · editor/MCP.

## 2. IDENTITY GATES (nothing derived from an unverified input)

| gate | measured | expected | verdict |
|---|---|---|---|
| source blend sha256 | `dd29effed485377a3d32639ced19d246f98fee6fe44a55938eb34dc2b4474df5` | 630's recorded v3 | ✅ |
| manifest sha256 | `ee8e23571aeb5b662eb18846170cc9db1eaa9937cc1417fd21e2c07a24303254` | 631 §6.4's recorded file hash | ✅ byte-intact |
| manifest `ucx.boxes` count | **66** | 66 (v3) | ✅ |
| pristine anchor (dims · min-Z · tris · verts · slots · per-slot faces) | 7313.576 × 7384.367 × 8082.610 · −0.1657 · 26,517 · 13,052 · `[TeamRegion, CastlePBR, CastleInteriorPBR]` · [4,318 / 15,195 / 3,555] | 629 §4 + 630 §3 verbatim | ✅ |
| UV layer | `UVMap` (sole) | 630 contract | ✅ |

**Honest note on the canonical v3-66 hash:** I attempted to reproduce 631's canonical-JSON sha `ebdd54ff…65ed` from the recipe as written (8 format variants: flat/nested, int/float rot, compact/spaced JSON) — **none matched.** The byte gate I ship instead is the WHOLE-FILE manifest sha above (stronger); `ebdd54ff…65ed` remains the editor-side post-apply readback target computed by 633's own instrument per 631 §6.3 — my failure to guess 631's exact serialization changes nothing about that contract.

## 3. PER-STAGE READBACK (all three measured independently; the table is one row because every number is identical — that identity is the deliverable)

| readback | Crumble01 == Crumble02 == Crumble03 | law | verdict |
|---|---|---|---|
| dims (uu) | **7313.576 × 7384.367 × 8082.610** — extent-equal to pristine | dims == pristine | ✅ |
| min-Z | −0.1657 | == pristine | ✅ |
| tris / verts / polys | 26,517 / 13,052 / 23,068 | == pristine | ✅ |
| **geometry sha256** (verts + loops + material indices) | `2858a910b598…` — **IDENTICAL ×4 (pristine + 3 stages)** | footprint-stable, byte-proven | ✅ |
| slots | `[TeamRegion, CastlePBR, CastleInteriorPBR]`, faces [4,318 / 15,195 / 3,555] | 630 §3 3-slot contract | ✅ |
| **aperture, visual open span by z** (probe-identical ×4) | z300: −670…670 · z800/z1400: −730…770 · z1800: −620…660 · z2100: −280…310 · z2250: −300…250 | 629 §4 table (±10 grid step) — **the gate stays open at every stage** | ✅ |
| gate aperture / lintel (collision) | **1500 gap / lintel 1530 — manifest-side, IDENTICAL by construction** (same 66-hull set every stage; `gate_lintel` + jambs byte-carried) | the standing aperture law | ✅ |
| **hall walkability** (180-column grid + centre route, per stage) | floor **174.00–174.00 flat, 0 off-spec columns**; min clear height **1986.0**; centre-route max riser **13.0**; spots: (0,−1750) 148.0 · (0,−1540) 161.0 · (0,−1295) 174.0 · (0,700) 174.0 · (910,1330) 174.0 · (−1890,1330) 174.0 · commander (−465,810) 174.0 — all expect-exact | hollow + walkable at every stage | ✅ |
| UCX embed | 66 `UCX_SM_Castle_Crumble0N_00..65` box nodes from manifest v3, in a viewport-visible `UCX_v3_embed` collection (render-hidden) | 629 §6 export duty; `TL-§2` authority unchanged | ✅ |
| removed on derive | the 25 historical `UCX_SM_Castle_*` imports (they may NOT ride into a stage FBX — the unit-hull/stale-25 trap) | `TL-§2` | ✅ |

## 4. ROUNDTRIP VERIFICATION (each exported FBX re-imported via the conformed recipe, fresh scene)

| readback | Crumble01 / 02 / 03 | verdict |
|---|---|---|
| render nodes | exactly 1, named `SM_Castle_Crumble0N` | ✅ |
| dims / min-Z | 7313.576 × 7384.367 × 8082.610 / −0.1657 | ✅ |
| hulls | 66, **AABB max deviation vs manifest 0.0003 uu** | ✅ |
| slots | `[TeamRegion, CastlePBR, CastleInteriorPBR]` in order | ✅ |
| aperture z300 / sill / hall floor | −670…670 / 148.0 / 174.0 | ✅ |
| tris | 26,508 vs source 26,517 — see the declared finding below | ✅ (gated on the measured mechanism) |

**⚠️ DECLARED FINDING — the 9-triangle importer cull (measured mechanism, not hand-waved):** the FBX on disk carries all 26,517 triangles; Blender's importer hygiene pass drops exactly 9. Reproduced deterministically: triangulate the source mesh + run `Mesh.validate()` → 26,517 → 26,508, **culled 9** — the same degenerate/duplicate sliver class 629 §5.5 declared (196 film edges / 248 inherited non-manifold, all buried in masonry; area census: 6 tris < 1e-12 m², 18 < 1e-8 m²). Zero-extent, zero-visual, zero-walkability impact (every probe above is post-cull identical). **For 633:** UE's importer may keep or cull these independently — gate the import on DIMS/slots/aperture, not on an exact LOD0 tri count; a readback of 26,517 or 26,508 (±the same class) is the same mesh.

## 5. ⭐ THE `W6-R2` STATEMENT FOR TASK-633'S APPLY

1. **Collision never varies across damage stages.** The authority for ALL FOUR meshes is manifest v3's 66-box `ucx.boxes` (file byte-verified this task: `ee8e2357…3254`). Apply is the 631 §6 contract verbatim: visual imports first, then `_apply_box_collision()` staged clear-then-fill on `BodySetup_0` of `SM_Castle` + all three crumbles in the SAME session; per-stage canonical readback **`ebdd54ff7888b38ba7f803c4f27228ebad776a2b4af2fb6c5a963ecc570565ed` string-IDENTICAL ×4** is the commit gate.
2. The 66 `UCX_` nodes embedded in each stage FBX are belt-and-braces for the interim import state ONLY (they land as convex on import; dev ≤ 0.0003 uu vs manifest) — `TL-§2`: the manifest apply supersedes them same-session; the interim state is never committed (GH-R6). ⛔ Never `reimport_meshes.py::main()`'s collision branch on the crumbles (the unit-hull trap).
3. Because stage geometry is byte-equal to pristine, **no per-stage geometric exception exists** — 631 §7's derivability premise holds exactly as written.

## 6. THE SLOT/MATERIAL BINDING CONTRACT FOR 633 (design-time; `ApplyCrumbleStage` writes slot 0 only at runtime)

| slot | name (in every stage FBX) | design-time binding 633 must save |
|---|---|---|
| 0 | `TeamRegion` | `MI_Castle_Crumble0N` (`WR-§1`) |
| 1 | `CastlePBR` | `MI_Castle_Crumble0N` (`WR-§1` — a missed slot ships a half-crumbled castle with no error logged) |
| 2 | `CastleInteriorPBR` | see the recommendation below; 630 §7.4 pre-authorizes either lane |

**⭐ Slot-2 recommendation (measured fact underneath it):** `M_CastleCrumble` samples its textures through **`MaterialExpressionTextureSampleParameter2D`** (verified by uasset string inspection — the samples are PARAMETERS, alongside scalar params `Darken`/`ScorchAmount`/`CharColor`/`RoughBoost`/`Ember*`). An interior crumble set is therefore **MI-only work, zero graph edits**: instance `M_CastleCrumble` → override the texture params to `T_Castle_Interior_{D,N,ORM}` → copy the stage scalar values from `MI_Castle_Crumble0N` → bind to slot 2 per stage. Without it, the lawful fallback (slot 2 → `MI_Castle_Crumble0N`) makes interior faces sample the EXTERIOR atlas at interior UV islands — a scrambled-stone read inside the hall mid-match, worst at stage 1 (`Darken 0.80` barely masks it). **Declared trade, 633's pick + manager naming ping (`MI_Castle_Interior_Crumble01/02/03` would be new asset names); Jonathan's eye at 636 arbitrates the read.** Exterior stage legibility is NOT re-opened: `T_Castle_D` is byte-identical (630 §5.4), and the spread check is base-relative.

## 7. WHAT 633/636 MUST VERIFY LIVE (beyond their own specs)

- Import each stage FBX same-path over `/Game/Meshes/SM_Castle_Crumble0N` (⛔ never delete+recreate), `combine_meshes=True`, `generate_lightmap_u_vs=False`, `auto_generate_collision=False`; Nanite OFF; the castle 3-LOD chain (`[1.0, 0.4, 0.15]`); readback bounds **7313.576 × 7384.367 × 8082.611** ×3 (the TASK-567 §6 acceptance table updated to 3 slots).
- All THREE slots read back by name+index per stage after binding (the 630 §7.3 pattern).
- Aperture spot-check per stage post-apply: gate gap 1500 / lintel 1530 (collision), through-door walk line clear.
- 636's predicted-trace table (631 §5) applies to the crumble stages VERBATIM — same hulls, same geometry.
- Mid-match stage-swap eyeball at the 636 checkpoint: interior shows the hollow hall at every stage (the GH-R6 stale-interior defect this task exists to kill), slot-2 read acceptable to Jonathan.

## 8. ATTEMPT LEDGER (loop discipline: 3 attempts, each failure named)

| attempt | failed on | mechanism | fix carried forward |
|---|---|---|---|
| 1 | roundtrip: 0 hulls in FBX | `Collection.hide_viewport=True` before export ⇒ `select_set()` never took ⇒ `use_selection` exported only the render mesh | hulls stay viewport-visible (render-hidden only); hide nothing before export |
| 2 | roundtrip tri gate 26,508 vs 26,517 | importer cull is `Mesh.validate()` hygiene, not a pure area threshold — my threshold-guess instrument was wrong, not the mesh | reproduce the exact mechanism (triangulate + validate = culled 9) and gate on it |
| 3 | — | **COMPLETE**, all gates + eyeball | — |

## 9. WHAT I DID NOT DO, ON PURPOSE

⛔ No geometric damage authoring (§0 adjudication — the shipped language is material staging; geometry divergence under invariant collision manufactures the sink/float defect class). ⛔ No FBX/texture/uasset write under `Content/` (633 owns the write-window; `Castle.fbx` untouched — its sha retires at 633, not here). ⛔ No manifest edit, no editor/MCP, no git, no board edit, no Meshy. ⛔ No MI authoring (editor-side; contract handed instead). ⛔ Did not chase the 9-triangle cull past its measured mechanism (the TASK-567 §5.2 "do not chase the delta" precedent). ⛔ Did not re-litigate the canonical-hash serialization — the file hash is the byte gate; 633's editor readback is the canonical instrument.
