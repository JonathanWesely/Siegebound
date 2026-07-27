# TASK-329 — Castle Meshy rebuild (BUILDING variant) — art-director handoff

**Status:** GENERATION DONE (re-verify → Meshy image3d → Stage-2 refine, all headless). All sources staged on disk at the EXISTING Castle raw paths (clean same-path overwrite). **UE import NOT done — editor-gated, build-master owns it (TASK-330).** NO editor / MCP / Blueprint / Git / TASKBOARD / CONVENTIONS touched.
**Date:** 2026-07-27 · Law: CONVENTIONS **"Castle remaster — the BUILDING same-path variant + the CRUMBLE-DERIVATION law"**, "Textured mesh law" (BUILDING path), "Meshy second engine (M7.5)".

---

## STEP 0 — INDEPENDENT RE-VERIFY (Jonathan: don't take the audit on trust) → **AUDIT CONFIRMED, EXACTLY**

I wrote my own measurement script rather than reuse the TASK-309 audit's, and **validated the method before trusting any Castle number**: it reproduces the pipeline's own recorded `albedo_delight.mean_linear_after` to 4 dp on Footman (0.1639), Knight (0.1639) and Cleric (0.3614). Footman's UV-normalised value comes out **0.2536** — my implementation independently reproduces the law's own published baseline.

| Re-verify item | Audit claim | My measurement | Verdict |
|---|---|---|---|
| (a) mean linear albedo — raw all-pixel | 0.0078 | **0.0078** | ✅ exact |
| (a) mean linear albedo — UV-normalised | 0.0653 | **0.0653** (coverage 11.98 %) | ✅ exact |
| (b) `albedo_delight` block in `refine_report.json` | absent | **absent** — report `started_utc 2026-07-08T06:02:02Z`, predates TASK-193 | ✅ confirmed |
| (d) triangle count | ~40 k | **40,000** | ✅ confirmed |
| (d) `lod_count` | 1 | *not re-verified — needs the editor (out of scope); carried forward for TASK-330 readback* | ⚠️ carried |
| (e) material wiring | `MI_Castle_PBR` → `T_Castle_D/N/ORM` | **confirmed** from the `.uasset` strings: master `/Game/Materials/M_AssetPBR`, params `BaseColor`/`Normal`/`ORM`, slots `[TeamRegion, CastlePBR]`, `bBuildNanite: false` | ✅ confirmed |

**Robustness:** the UV-normalised figure is not a threshold artefact — recomputed under four independent coverage-mask definitions it lands 0.0640–0.0689 every time.

**The number that settles it:** the **p99 of the entire shipped albedo texture is 0.117** — the brightest 1 % of the castle is still darker than the Footman's *mean*. Under the Cycles beauty rig it retained **4.3 %** of the concept's luminance (luma retention 0.0425).

⚠️ **One honest deviation, recorded so nobody quotes it as confirmed:** on metric (c) chroma retention I measure **0.31×** (flat-lit preview) / 0.60× (beauty render) vs the audit's **0.20×**. That is a render/mask-choice difference, not a contradiction of the finding. Chroma is **not** a stop-gating criterion — only (a) and (b) are, and both reproduced exactly — so I proceeded.

---

## STEP 1 — MESHY gate
Token env-only (redacted; never echoed, never logged, never on argv). `--check` **PASSED** — key valid, API reachable, balance **2506** credits. Norton TLS handled by reusing `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`.

- **Meshy task id:** `019fa28a-133c-7a88-8815-55f90cec6ae7` · engine `meshy-i23d` · mode `image3d`
- **Credits: 30 consumed — 2506 → 2476 remaining.**
- Concept `Inbox/Castle.png` verified **byte-identical** to the approved `Content/RawAssets/Concepts/Castle.png` (sha256 `c711bca712440ef1…`). No concept-gen. Concept already in place at the accepted path — no copy needed.
- Donor `Cache/Castle/meshy_raw.glb` (28,459,864 bytes). The old `trellis_raw.glb` is left intact as the zero-credit fallback.

---

## STEP 2 — Stage-2 refine (BUILDING path)

**Brightness profile PINNED** in `pipeline_manifest.json` → `assets.Castle.albedo_delight = {ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` — the locked Footman-validated fleet profile (`d7254da`), i.e. **precisely the stage that had never run on this asset**. Also pinned `engine: "meshy-i23d"` and lowered `tri_budget` 40000 → **20000**.

`refine_report.json` now carries the block: **`mean_linear_before 0.0236 → mean_linear_after 0.1123`**, p99 0.9198, shoulder-compressed 5.1 %. **Zero warnings.**

### Acceptance — before / after (measure, don't eyeball)

| Metric | Shipped (before) | Rebuilt (after) | Floor | Result |
|---|---|---|---|---|
| Mean linear albedo — **raw** | 0.0078 | **0.1123** (14.4×) | 0.164 | ⚠️ **below — see ruling request** |
| Mean linear albedo — **UV-normalised** | 0.0653 | **0.5847** (9.0×) | 0.2536 | ✅ **PASS at 2.31×** |
| UV coverage | 11.98 % | 19.20 % | — | — |
| p99 linear | 0.117 | 0.9216 | — | full headroom restored |
| Mean chroma (covered) | 0.0261 | **0.1653** (6.3×) | — | ✅ |
| Chroma retention vs concept | 0.31× | **1.21×** | 0.28× | ✅ PASS at 4.3× the floor |
| Luma retention vs concept | 0.43× | **1.01×** | — | ✅ within **1.3 %** of the concept |

### ⚠️ RULING REQUESTED — the DUAL floor's *raw* arm is not reachable by any building

The rebuilt Castle **misses the 0.164 raw arm (0.1123)** while clearing the UV-normalised arm at 2.31×. Before treating that as a failure, I measured the whole shipped set on both metrics:

| asset | raw | coverage | UV-norm | raw ≥ 0.164 | uv ≥ 0.2536 |
|---|---|---|---|---|---|
| **Castle (REBUILT)** | **0.1123** | 19.2 % | **0.5847** | fail | **PASS** |
| Wall | 0.0810 | 33.1 % | 0.2451 | fail | fail |
| Barracks | 0.0576 | 21.1 % | 0.2724 | fail | PASS |
| BombTower | 0.0523 | 22.6 % | 0.2314 | fail | fail |
| ArrowTower | 0.0408 | 20.4 % | 0.1996 | fail | fail |
| DeepMine | 0.0368 | 23.1 % | 0.1596 | fail | fail |
| BallistaTower | 0.0314 | 20.5 % | 0.1531 | fail | fail |
| Footman (baseline) | 0.1639 | 64.6 % | 0.2536 | fail* | PASS |
| Knight | 0.1639 | 66.1 % | 0.2478 | fail* | fail |
| Ogre | 0.1416 | 58.5 % | 0.2420 | fail | fail |
| Cleric | 0.3614 | 69.5 % | 0.5203 | PASS | PASS |

\* 0.1639 vs a 0.164 floor — the baseline asset itself only *defines* the line, it does not clear it.

**Two conclusions:**
1. **No building has ever reached 0.164 raw** (previous best: Wall 0.0810). The rebuilt Castle at 0.1123 is now **the brightest building in the project, by 39 % over Wall**. The raw arm is arithmetically unreachable for a building: at 19.2 % coverage, raw 0.164 requires a covered-texel mean of **0.854** — a near-white texture. This is exactly the coverage confound the law itself documents.
2. Several **signed-off** assets (Ogre, Knight, Wall) fail one or both arms as written.

**Recommendation (manager's call, not mine):** treat the **UV-normalised 0.2536 arm as the operative acceptance floor for buildings**, with raw reported for continuity. I have NOT altered the pinned profile to chase the raw number — doing so would bleach the asset and contradict the eyeball evidence below.

**Is it now too bright?** On the fleet scale 0.5847 covered is the highest of any asset (above Cleric's 0.5203). But against the actual art-direction target it is near-exact: **luma retention vs the concept is 1.0133×** and chroma retention **1.21×**. It matches the concept's own luminance to within 1.3 %. Zero-cost temper lever if the in-engine verify disagrees: `gamma 0.65 / gain 1.0`, Stage-2 re-run only (~23 s, **no credits**, donor GLB cached).

### Geometry — final vs shipped

| | Shipped | Rebuilt | Note |
|---|---|---|---|
| Triangles | 40,000 | **19,995** (budget 20,000) | **-50 %**; structural debt cleared |
| Verts (welded) | — | **9,792** | healthy weld (≠ tris×3) — the unweld failure mode is absent at source |
| Bounds (UE) | 813.13 × 819.13 × 897.21 | **814.52 × 820.56 × 894.87** | within ±10 % of 814.5 × 820 × 900 ✅ (conform reports `within_tolerance: true`) |
| min-Z (ground-centre) | 0.445 | **0.561** | ground-centre origin preserved ✅ |
| UV layer | `UVMap` | **`UVMap`** ✅ | |
| Slots (ORDER-CRITICAL) | `[TeamRegion, CastlePBR]` | **`[TeamRegion, CastlePBR]`** ✅ | |
| UCX hulls | 11 | **11** (`UCX_SM_Castle_00..10`) | authored from manifest |
| Nanite | OFF | OFF (import-time flag) | |
| Textures | 2048² D/N/ORM | **2048² D/N/ORM** ✅ | building resolution, not the units' 1024² |

**Landmark silhouette preserved:** the tri budget halved but the read *improved* — the top view now shows a clean rectangular curtain-wall plan with round corner towers on all four corners, an interior courtyard, a central keep/cathedral mass, and a protruding gatehouse porch at the front. The old 40 k mesh spent its budget on a melted blob. **No case for exceeding 20 k.**

**Facing verified (not assumed):** the pipeline's contract is `front = -Y`. I measured the exported FBX's area-weighted footprint: the centred gatehouse porch protrudes at **-Y (front)**, front curtain wall at y ≈ -269, back wall at y ≈ +395. `pre_rotate_z_deg` stays **0.0** — no rotation needed.

---

## ⚠️ FINDING for TASK-330 — the authored UCX hulls no longer match the new wall planes

The 11 `ucx.boxes` were tuned in **TASK-087 against the OLD TRELLIS mesh** ("wall planes: front y_norm 0.17–0.28, back 0.79–0.87, west x 0.15–0.25, east 0.88–0.97"). The Meshy mesh has different internal massing. Area-weighted measurement of the new FBX (face centroids, wall band z 40–280 UE):

| side | authored hull (outer face) | **new visual wall** | delta |
|---|---|---|---|
| front (-Y) | -270 (`wall_front`) | ≈ **-269** | ✅ matches |
| east (+X) | +386 (`wall_east`) | ≈ **+382** | ✅ matches |
| **west (-X)** | **-286.5** (`wall_west`) | ≈ **-372** | ⚠️ **hull ~86 uu inside the wall** |
| **back (+Y)** | **+306** (`wall_back`) | ≈ **+395** | ⚠️ **hull ~89 uu inside the wall** |
| gatehouse porch | -329 | ≈ -385 | ~56 uu — *consistent with the deliberate TASK-087 "rock ramp uncollided" trade* |

**Consequence:** on the **west and back** sides units can visually penetrate the curtain wall by ~85–90 uu before colliding. Cosmetic, not game-breaking.

**I deliberately did NOT re-author the hulls, and this is a judgement call TASK-330/the manager should make consciously:** widening them to match the walls **enlarges the placement dead-zone**, and the M1 lesson plus the TASK-087 note ("front-most collision now y=-328 vs blockout -410: the M1 dead-zone SHRINKS, gold-node/miner clearance improves") show that shrinkage was *deliberate and gameplay-motivated*. `CastlePlinthClearance` (420) is computed off this footprint. That is a gameplay trade, not an art-only fix. **If ruled in favour of matching:** it is a `pipeline_manifest.json` `ucx.boxes` edit + a free Stage-2 re-run (~23 s, no Meshy credits) — route it back to art-director, do not hand-edit collision in the editor.

---

## Team region (CRITICAL — differs from units)

`refine_report.team_region`: **3,460 faces / 12.91 % of surface area** (cap 40 %), 1 selector (`roofs`: box z 0.30–1.0, +Z normal `min_dot 0.10`). Was 6,438 faces / 6.76 % on the old mesh — the share roughly doubled because the new massing has far more genuine roof/spire area.

**Deliberate choice, verified in the beauty preview:** the band lands on the **tower cones, keep/cathedral roofs and wall-walk tops** — the classic top-down RTS team read — and is cleanly isolated from the walls, gatehouse and courtyard. 12.9 % is a strong, unambiguous read from the arena camera without swamping the sandstone identity.

**Relevant audit precedent:** on ArrowTower/Barracks the "bleached roof" turned out to *be* the TeamRegion slot, not wash-out. The Castle's blue roofs in `preview_beauty_cycles.png` are **the same thing — placeholder `TeamRegion` colour, not a bake defect.** Do not "fix" them.

**Slot order is load-bearing — verified from source, not convention:**
- `ACastle::ApplyTeamVisuals()` (`Castle.cpp:148`) → `CastleMesh->SetMaterial(0, Material)` — **slot 0 is the team recolor.**
- The in-code comment "SM_Castle has a single material slot (TASK-013 spec)" is **stale** — the mesh has had two slots since M7. The code is correct; the comment is not.

---

## Pre-import eyeball gate — **PASS**
Read `refine_report.json` (zero warnings) and viewed all five previews against `Content/RawAssets/Concepts/Castle.png`.

- **Colour — the failure is fixed.** Warm tan/honey **sandstone** walls, grey-slate roofs, green mossy rock base. This is the concept's palette. The shipped asset was a near-black mass; this is not.
- **Silhouette:** Gothic cathedral keep inside a crenellated curtain wall with round corner towers and a front gatehouse porch — reads as the concept from every angle, and the gate/wall read gameplay depends on is present and *better defined* than the 40 k original.
- **Caveat (non-blocking):** surfaces remain somewhat soft/melted — the voxel remesh at 4.0 uu on a 900-uu landmark cannot hold the concept's window tracery or crenellation crispness. This is a pipeline-resolution limit, not a regression: it is strictly sharper than what ships today at half the triangles.
- **Caveat (non-blocking):** sits on the bright side of the fleet (0.5847 covered) though within 1.3 % of the concept's own luminance. Temper lever recorded above if the in-engine verify disagrees.

**Verdict: PASS.** Ship it to TASK-330.

### Preview paths (to show Jonathan)
- Flat-lit true-albedo (best colour read): `Tools/ArtPipeline/Cache/Castle/previews/preview_front.png` · `preview_threequarter.png`
- Plan / footprint read: `…/previews/preview_top.png`
- Beauty (Cycles, TeamRegion placeholder blue visible): `…/previews/preview_beauty_cycles.png`
- Concept: `Content/RawAssets/Concepts/Castle.png`

### How generated (reproducible)
- Stage 1: `uv run meshy_generate.py --mode image3d Castle` → `Cache/Castle/meshy_raw.glb` (exit 0, 30 credits).
- Stage 2: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Castle --input Cache/Castle/meshy_raw.glb` → exit 0, **22.8 s**.

---

# TURNKEY same-path IMPORT recipe (TASK-330, build-master, editor-gated)

**Serialized, EXCLUSIVE editor session, Simulate STOPPED during every import.** **NEVER delete+recreate — same-path OVERWRITE preserves `ACastle`, `CastleAnchor_Blue/Red`, both `L_Arena` win-condition instances, the crumble chain and every soft ref.** `L_Arena` is **never saved**.

### Sources on disk (all overwritten, verified present)
| Source | Size |
|---|---|
| `Content/RawAssets/Castle.fbx` | 843,644 B (was 1,684,972) |
| `Content/RawAssets/Textures/Castle/T_Castle_D.png` | 2048², 1,911,663 B |
| `Content/RawAssets/Textures/Castle/T_Castle_N.png` | 2048², 1,848,088 B |
| `Content/RawAssets/Textures/Castle/T_Castle_ORM.png` | 2048², 1,646,522 B |

### 🚨 STEP 0 — THE TEXTURE TRAP (read this first; it will silently ship the old look)

`Tools/reimport_meshes.py::_ensure_textures_and_mi()` **early-returns when the MI already exists**:

```python
mi_pkg = PBR_MI_FMT % card_id
if unreal.EditorAssetLibrary.does_asset_exist(mi_pkg):
    result["notes"].append("MI already exists (%s) -- texture/MI step skipped." % mi_pkg)
    return mi_pkg
```

`MI_Castle_PBR` **does** exist. So running that script alone reimports the geometry + LODs + collision but **skips the textures entirely** — new UVs on the OLD dark textures, i.e. the exact scramble this whole task exists to prevent, on the main castle. **Import the three textures explicitly (same-path, `replace_existing`) as a separate step.** Do NOT delete the MI to force the branch — the MI already points at those texture objects by path, so overwriting the texture assets in place propagates for free and touches no refs.

### 1. Textures → `/Game/Textures/` (OVERWRITE in place)
- `T_Castle_D` ← `…/T_Castle_D.png` — **sRGB ON**, `TC_DEFAULT`
- `T_Castle_N` ← `…/T_Castle_N.png` — **LINEAR / sRGB OFF**, `TC_NORMALMAP`
- `T_Castle_ORM` ← `…/T_Castle_ORM.png` — **LINEAR / sRGB OFF**, `TC_MASKS`

### 2. `MI_Castle_PBR` — do NOT recreate
Already correct: master `/Game/Materials/M_AssetPBR`, `BaseColor`=T_Castle_D, `Normal`=T_Castle_N, `ORM`=T_Castle_ORM (verified in the `.uasset`). Textures overwritten in place ⇒ nothing to rewire. Path stays `/Game/Materials/Instances/MI_Castle_PBR`.

### 3. Static mesh `SM_Castle`
- Import `Content/RawAssets/Castle.fbx` **OVERWRITING `/Game/Meshes/SM_Castle` at the same path** (`AssetImportTask(replace_existing=True, automated=True)` — `Tools/reimport_meshes.py` is the proven lane; MCP `import_file` cannot overwrite).
- **Nanite OFF.** Import Normals (`bRecomputeNormals=false`). Collision: the FBX carries `UCX_SM_Castle_00..10`; the script also authors the manifest boxes deterministically. **NO convex decomposition on a building.**
- Slots EXACTLY, in order: **slot 0 `TeamRegion` → `MI_TeamColor_Blue` (design-time default); slot 1 `CastlePBR` → `MI_Castle_PBR`.** FBX slots already `[TeamRegion, CastlePBR]` → 1:1.
- **Known commandlet limitation:** the post-reimport build resets slot→material pointers, so finalize the per-slot assignment over MCP in the relaunched editor (`Tools/reimport_apply_materials_mcp.py`). Slot NAMES, geometry, Nanite-off and collision DO persist.

### 4. LOD chain — **target 3, NOT 4. Do NOT apply `LargeProp`.**
Castles are the landmark-silhouette exception to the M7.6 Classic-LOD law: explicit **LOD1 50 % @ screen 0.4 / LOD2 25 % @ 0.15, no LOD3** ⇒ readback **`lod_count == 3`**.
**Already implemented — use it, don't hand-roll:** `Tools/reimport_meshes.py` has `CASTLE_CLASS_CARD_IDS = ["Castle", "Castle_Crumble01", "Castle_Crumble02", "Castle_Crumble03"]` and `CASTLE_LOD_CHAIN = [(1.00,1.00), (0.50,0.40), (0.25,0.15)]`, and clears `lod_group` to `"None"` first (explicit reduction only applies with no group set).

### 5. HARD LOD readback gate (the `SM_MilitiaMob` lesson)
Read back **LOD0 welded vert count AND per-LOD triangle counts** — `lod_count` alone does not catch it.
- **Expected LOD0: ~19,995 tris / ~9,792 verts** (measured on the FBX pre-import). LOD1 ≈ 10 k, LOD2 ≈ 5 k.
- **HARD FAILURE ⇒ restore last-known-good `SM_Castle`, do NOT commit, report to the manager:** any 0-triangle LOD, or `verts == tris × 3` (unweld). The source FBX is cleanly welded (9,792 ≪ 59,985), so an unweld would be introduced by the import, not inherited.
- Verify in **Simulate**, never PIE-in-viewport. Never import while Simulate runs (package saves are silently blocked).

### 6. Also verify
- Both castles still team-identify (slot 0 recolor) — locate by `TActorIterator<ACastle>` / class filter, **never by actor label** (level labels are `Castle_0`/`Castle_1`, and `Castle_Red` carries yaw 180).
- Ground-centre origin: min-Z 0.561 uu — the castle should sit ON the plinth, not floating/sunk.
- Optional: check west/back wall clipping per the UCX finding above.

---

# 🚨 REQUIRED FOLLOW-UP — TASK-331 (crumble re-derivation). NOT optional.

**I independently verified the trap rather than taking it on trust** (read-only `.uasset` string inspection):
- `SM_Castle_Crumble01` still carries **`Factory_RootNode.SM_Castle`** internally — proof it is a byte-copy duplicate of the OLD `SM_Castle`.
- Its slots are `[TeamRegion, CastlePBR]` with **BOTH** pre-assigned to `MI_Castle_Crumble01` at design time.
- `M_CastleCrumble` samples **`/Game/Textures/T_Castle_D`, `_N`, `_ORM`** — the exact three textures TASK-330 overwrites.

⇒ After TASK-330, the OLD crumble geometry (old UVs) is paired with the NEW textures ⇒ **scrambled textures at the 75 / 50 / 25 % damage states on the win-condition actor.** Shipping 330 without 331 is a visible regression.

### What TASK-331 needs from this task
- **New UV layout / topology:** single UV layer **`UVMap`**, 2048² bakes, **19,995 tris / 9,792 verts**, UV coverage **19.2 %** — a completely different layout from the old 40 k mesh. There is no partial-compatibility path: the crumbles must be re-derived from the rebuilt mesh.
- **Recipe (TASK-157 verbatim):** duplicate the **REBUILT** `SM_Castle` over `SM_Castle_Crumble01/02/03` at their same `/Game/Meshes/` paths, then re-assign `MI_Castle_Crumble0N` to **BOTH slots** of its stage mesh. `M_CastleCrumble` and the crumble MIs are **NOT re-authored** — they pick up the new albedo for free through the same-path textures.
- **⚠️ WHY "BOTH SLOTS" IS LOAD-BEARING — verified in C++, and it is easy to get wrong:** `ACastle::ApplyCrumbleStage()` (`Castle.cpp:326`) sets **slot 0 only**:
  ```cpp
  CastleMesh->SetStaticMesh(CrumbleMesh);
  CastleMesh->SetMaterial(0, CrumbleMaterial);   // slot 0 ONLY — slot 1 is never touched at runtime
  ```
  Slot 1 therefore comes from **the crumble asset's own saved material**. A fresh duplicate of the rebuilt `SM_Castle` inherits slot 1 = `MI_Castle_PBR`, so unless it is explicitly re-assigned the damaged castle renders **crumble material on the roofs and pristine castle material on the walls** — a half-crumbled look, with no error logged.
- Crumble MIs live at **`/Game/Materials/`** (NOT `Instances/`) — `ACastle` composes `/Game/Materials/MI_Castle_Crumble0%d`; the code path wins over the prefix-table folder row.
- Crumble variants must **preserve the UCX footprint** (C++ comment at `Castle.cpp:317` — "visual swap only … collision/placement/pathing untouched"). Duplicating the rebuilt mesh inherits the new hulls automatically.
- `ACastle::ResetCastle()` → `ApplyTeamVisuals()` restores the pristine `SM_Castle` — so the main mesh must remain at its path (it does; same-path overwrite).
- The crumbles are already in `CASTLE_CLASS_CARD_IDS`, so they get the 3-LOD castle chain too, not `LargeProp`.

---

## Files changed by this task (art sources + manifest only — NO Git, NO editor)
- `Content/RawAssets/Castle.fbx` *(overwritten)*
- `Content/RawAssets/Textures/Castle/T_Castle_{D,N,ORM}.png` *(overwritten)*
- `Tools/ArtPipeline/pipeline_manifest.json` — Castle: `+engine "meshy-i23d"`, `tri_budget 40000→20000` (+note), `+albedo_delight` locked profile (+note)
- `Tools/ArtPipeline/Cache/Castle/` — `meshy_raw.glb` (new), `state.json`, `refine_report.json`, `previews/*` (regenerated)
- `Content/RawAssets/Concepts/Castle.png` — **unchanged**, already the accepted concept (sha-verified against the Inbox copy)
