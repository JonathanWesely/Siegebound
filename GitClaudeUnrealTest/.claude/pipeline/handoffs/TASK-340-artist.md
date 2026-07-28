# TASK-340 — Ogre remake attempt #3 (measure → fresh Meshy → floor-gated Stage-2 → re-rig) — art-director handoff

**Status:** GENERATION DONE, ALL ACCEPTANCE FLOORS PASS (measured, not eyeballed). All sources staged on disk at the EXISTING Ogre raw paths (clean same-path overwrite). **UE import NOT done — editor-gated, build-master owns it (TASK-341).** NO editor / MCP / Blueprint / Git / TASKBOARD / CONVENTIONS touched.
**Date:** 2026-07-27 · Law: CONVENTIONS "Fleet Meshy remaster" (same-path + shared-skeleton + preserve-anims + **PER-ASSET `albedo_delight` OVERRIDE**), "Meshy second engine (M7.5)", "Castle remaster" amended ALBEDO ACCEPTANCE FLOOR (UV-norm arm), M7.6 SK-unit LOD law. Board: "## OGRE-REMAKE" rulings 1–9.

---

## STEP 0 — MEASURE THE SHIPPED STATE (ruling 4: steers Stage 2, did not gate the remake)

**Method validated FIRST (method law):** my script reproduces the pipeline's recorded `albedo_delight.mean_linear_after` to 4 dp on Footman **0.1639**, Knight **0.1639**, Cleric **0.3614**, and independently reproduces the law's own Footman UV-normalised baseline **0.2536**. Decode is byte-identical to `refine_trellis_glb.py` (`_srgb_to_linear`); covered = any nonzero texel (bake background is exact black).
**Retention-method validation (recorded honestly):** concept luma/chroma retention is flat-lit-preview-vs-concept with a corner-sampled background mask. Threshold **bg-distance > 0.06** reproduces the recorded Castle rebuilt retention (**1.0155** vs TASK-329's 1.0133 — 0.2 % off) and is the method used everywhere below. My first-guess stricter mask (0.12) FAILED that validation by 11 % (it excludes bright subject pixels near the bg tone) and briefly mis-steered one temper trial (pass 2/3 below) — discarded, documented, no deliverable was cut from it.

### The "before" table — shipped `T_Ogre_*` (the 42d2ab2 / TASK-316 state)

| Metric | Shipped value | Gate | Verdict |
|---|---|---|---|
| (a) mean linear albedo — raw | **0.1416** (recorded 0.1416 — exact) | reported, never a gate | — |
| (a) UV coverage | **58.52 %** | — | — |
| (a) mean linear albedo — **UV-normalised** | **0.2420** | ≥ 0.2536 | ❌ **FAIL — the before-evidence.** (Board's "≈0.21" expectation was slightly off; 0.2420 matches TASK-329's fleet table and FAILS the floor either way.) |
| (b) p99 linear / shoulder-clamped | **0.6376 / 0.22 %** (recorded 0.6345/0.22 %) | — | headroom was fine; darkness is not clamp-side |
| (c) luma retention vs concept | **0.8124** | 0.85–1.25 | ❌ **FAIL on the DARK side** — the shipped bake is ~19 % darker than its concept, view-weighted. Consistent with Jonathan's eye. |
| (c) concept's OWN palette luma | fg mean linear luma **0.1082** (validated mask) | — | intrinsically the darkest unit palette measured (Castle concept: 0.2256) — but NOT so dark that the gates conflict; see STEP 2 |
| (d) chroma retention vs concept | **1.1149** | secondary | saturation was concept-faithful; the failure is luminance + the dark chip-speckle baked into the atlas |
| (d) ORM — AO mean (covered / raw) | **0.5415 / 0.3169** | — | **MID-FLEET** (Footman 0.5866, Knight 0.5253, Wizard 0.5301, Cleric 0.4692) — **the darkness is NOT materially ORM-side** |
| (d) ORM — Metallic mean (covered) | **0.0033** | — | ≈0, no spurious metallic darkening |
| (e) team-region coverage | **1.33 % / 187 faces** (recorded) | 1.7–2.5 % | ❌ FAIL — the folded TASK-324 lever |

**Where the darkness actually lives (the diagnosis that steered Stage 2):** not ORM (mid-fleet AO, zero metallic), not clamp (0.22 % shoulder). It is the **D bake**: the atlas sat 4.8 % under the UV-norm floor AND the view-weighted read sat 19 % under the concept, dragged down by heavy dark chip/speckle baked into the old donor's texture. ⇒ the new bake needed a **cleaner donor + a brighter-than-locked delight profile**; no ORM-side fix required (and none was made — the new AO/Metallic land 0.5364/0.006, same family).

---

## STEP 1 — Fresh Meshy image-to-3D (ruling 3, ≈30 credits APPROVED)

- Token env-only (`MESHY_TOKEN`, redacted everywhere, never on argv/in files). `--check` PASSED before spend.
- **Meshy task id: `019fa629-462f-7d44-87c2-e544b2d54d70`** · engine `meshy-i23d` · mode `image3d`.
- **Credits: 30 consumed — balance 2476 → 2446.**
- Concept fed UNCHANGED (ruling 2): `Inbox/Ogre.png` verified **byte-identical** to approved `Content/RawAssets/Concepts/Ogre.png` (sha256 `323d0204…b7`, matches the TASK-316 record). Concept already at the accepted path — no copy needed.
- New donor: `Cache/Ogre/meshy_raw.glb` (27,686,080 B). **TASK-316's donor preserved** as `Cache/Ogre/meshy_raw_task316_42d2ab2.glb` (zero-credit fallback). TLS via the existing `Cache/_certs/win-ca-bundle.pem` (`SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`) — the TASK-329 playbook, no Norton exclusion touched.

## STEP 2 — Stage-2 refine, UNIT path, GATED ON RULING 6 IN FULL

Headless: `blender.exe --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Ogre --input Cache/Ogre/meshy_raw.glb` (12–13 s per pass; all re-runs free, zero credits).

### Iteration record (nothing hidden — pass 2/3 were the mis-steered mask's doing)
| Pass | Profile (`albedo_delight`) | Team selector | UV-norm | Retention (validated) | Team % | Outcome |
|---|---|---|---|---|---|---|
| 1 | LOCKED fleet `{1.0, 0.25, γ0.55, g1.2}` | shipped (z 0.66–0.84) | 0.2707 ✅ | 0.8549 ✅ (0.6 % over the dark edge) | 1.3 % ❌ | floor passes on the fresh donor even under the locked profile — but retention hugs the dark edge and Jonathan's complaint IS darkness |
| 2 | g 1.14 (temper — **wrong direction**, steered by the failed 0.12 mask) | z 0.62–0.86 + min_dot 0.50 | — | — | 2.8 % ❌ over-ceiling | both changes reverted |
| 3 | g 1.14 | z 0.62–0.86, min_dot 0.55 | 0.2572 ✅ | 0.8346 ❌ dark | 2.4 % ✅ | confirmed temper is backwards; mask re-validated against the Castle precedent → 0.06 mask adopted |
| **4 FINAL** | **`{ao_divide 1.0, ao_floor 0.25, gamma 0.53, gain 1.26}`** | **z 0.62–0.86, min_dot 0.55** | **0.2983** ✅ | **0.9399** ✅ | **2.36 %** ✅ | **ALL GATES PASS** |

### THE ACCEPTANCE TABLE — every ruling-6 floor, before vs after (measure, don't eyeball)

| Ruling-6 gate | Shipped (before) | **Remake (after)** | Floor | Verdict |
|---|---|---|---|---|
| (a) UV-normalised mean linear albedo | 0.2420 | **0.2983** (+23 %) | **≥ 0.2536** | ✅ **PASS at 1.18×** (a same-as-before bake could not — the shipped state FAILS) |
| (a) raw mean linear albedo (reported) | 0.1416 | **0.1826** (+29 %) | reported only | brightest this asset has ever measured; above the Footman baseline 0.1639 |
| (a) UV coverage | 58.52 % | 61.21 % | — | — |
| (b) concept luma retention | 0.8124 (FAIL-dark) | **0.9399** | **0.85–1.25**, target ≈1.0 | ✅ **PASS** — 10.6 % above the dark edge, 33 % below the bleach ceiling |
| (c) gate conflict? | — | **NO** — both (a) and (b) pass simultaneously with margin | concept governs on conflict | ✅ not triggered; no manager flag needed. The palette is genuinely dark-ish (0.1082) but the gates are jointly reachable |
| (d) chroma retention (secondary) | 1.1149 | **1.2337** | reported (Cleric 0.28× ref) | ✅ the "weakest colour read" unit now carries slightly MORE saturation than its concept |
| (e) team region | 1.33 % / 187 faces | **2.36 % / 317 faces** | **1.7–2.5 %**, target ≈2.1 % | ✅ **PASS** — single shoulder-cap band on the pauldron/shoulder line, no helm dome, no face/horn paint, no silhouette striping (beauty + bind previews) |
| bleach flag (>1.25× AND >0.60 UV-norm) | — | 0.9399 / 0.2983 | — | ✅ nowhere near |
| p99 / shoulder | 0.6376 / 0.22 % | **0.7749 / 0.85 %** | — | full highlight headroom, no wash-out |
| ORM AO / Metallic (covered) | 0.5415 / 0.0033 | **0.5364 / 0.006** | — | mid-fleet, unchanged family — confirms the fix was D-side, as diagnosed |

### Per-asset `albedo_delight` OVERRIDE — recorded per the NEW law (ruling 5)
Pinned in `Tools/ArtPipeline/pipeline_manifest.json` → `assets.Ogre.albedo_delight` (with a full `_note` explaining the derivation and the mask-validation episode):
```
ao_divide_strength 1.0 · ao_floor 0.25 · gamma 0.53 (was 0.55) · gain 1.26 (was 1.2)
```
Rationale: the LOCKED fleet profile is PROVEN insufficient for this palette (it shipped 0.2420 at TASK-316 and, on the fresh donor, leaves retention hugging the 0.85 dark edge). γ0.53/g1.26 lifts the mossy mids/darks more than the highlights — toward the band's ≈1.0 target without bleaching the top end. Team selector tune also recorded in the manifest (`team_region._verified` appended): **z-band 0.66–0.84 → 0.62–0.86** (the exact lever the TASK-316 handoff pre-recorded), min_dot kept 0.55 after a 0.50 trial measured 2.8 % (over-ceiling).

### Pre-import eyeball gate — **PASS** (previews viewed against `Concepts/Ogre.png`)
- **Colour:** mossy-green skin with pink inner-ear/facial detail, warm brown weathered leather, bone-white tusks/spikes, grey stone maul — the concept's palette, emphatically not the shipped muddy grey-green. Dark chip-speckle vastly reduced vs the shipped bake (a little remains — pipeline-resolution characteristic, strictly better than shipped).
- **Silhouette:** horned/tusked brute, spiked pauldron, chest straps, wide skull-hung belt, tattered skirt, bone-and-stone maul in the right fist, big bare feet. Reads as the concept front and three-quarter.
- **Geometry:** 14,999 tris (budget 15,000) · **7,470 welded verts** (≪ tris×3 — the unweld failure mode absent at source) · `UVMap` ✅ · bounds UE [261.25 × 164.86 × 289.48], Z height-fit ✅ · min_z **+0.012 uu** — feet-centre, effectively exact. Dims warn (X/Y vs the humanoid blockout guess) is the KNOWN benign Ogre-breadth warn (TASK-316 §warn-only; `fit_mode=height` matched Z).
- **Verdict: PASS. Ship it to TASK-341.**

## STEP 3 — Re-rig headless onto the SHARED skeleton (Stage-B recipe)

`blender.exe --background --factory-startup --python-exit-code 1 --python rig_character.py -- --card-id Ogre --no-anim-fbx` → exit 0, 51.8 s, Blender 5.1.2.

| Check | Result |
|---|---|
| Rig report **TOP-LEVEL `skeleton`** (the authoritative field) | **`SiegeBiped`** ✅ — the shared contract. (`armature.skeleton` reads "Ogre" — that is the documented COSMETIC card-id, never an asset name) |
| Armature object / bone count | **`Footman_Rig` / 21 bones** ✅ (the shared `SK_Footman_Skeleton` contract; NO bespoke skeleton created — a stray skeleton asset appearing at import is a HARD FAIL, ruling 8) |
| Skinning | **`auto_heat`, 0.00 % unweighted** — better than TASK-316's 0.08 % |
| Height handling | rig MEASURE height 289.5 uu — the tall/nonstandard proportions went through the known height handling ✅ |
| `A_Ogre_{Idle,Walk,Attack,Death}` raw FBXs | **byte-untouched** (mtimes still 2026-07-21 21:08) via `--no-anim-fbx`; report outputs carry `mesh_fbx` ONLY ✅ — in-engine anims + shared ABP PRESERVED, nothing to reimport |
| SK-LOD sidecar | `Characters/Ogre.lod.json` regenerated — schema `siege_sk_lod_recipe_v1`, LOD1 50 %@0.4 / LOD2 20 %@0.15 + URO flags ✅ |
| FBX integrity | `Kaydara FBX Binary` magic ✅ · SK 14,999 tris / 7,470 verts · slots `[TeamRegion, OgrePBR]` ✅ |
| Bind previews | intact silhouette, feet planted, no exploded verts, team band visible on shoulders (`Cache/Ogre/rig/previews/bind_front.png`) ✅ |

## Assets staged (same-path overwrite — the exact TASK-316 names, nothing new)

| File | Size | Purpose |
|---|---|---|
| `Content/RawAssets/Ogre.fbx` | 615,228 B | raw STATIC mesh → `SM_Ogre` (ghost + null-safe fallback) |
| `Content/RawAssets/Textures/Ogre/T_Ogre_D.png` | 1,008,337 B | 1024², base colour (sRGB ON at import) |
| `Content/RawAssets/Textures/Ogre/T_Ogre_N.png` | 808,411 B | 1024², normal (LINEAR) |
| `Content/RawAssets/Textures/Ogre/T_Ogre_ORM.png` | 972,488 B | 1024², AO/Rough/Metal (LINEAR, sRGB OFF, TC_Masks) |
| `Content/RawAssets/Characters/Ogre.fbx` | 852,460 B | rigged SKELETAL mesh → `SK_Ogre` (armature `Footman_Rig`, 21 bones) |
| `Content/RawAssets/Characters/Ogre.lod.json` | 875 B | SK-LOD recipe (regenerated) |
| `Tools/ArtPipeline/pipeline_manifest.json` | — | Ogre block: per-asset `albedo_delight` override + team z-band tune, both `_note`d (NEW law) |

Cache (gitignored, evidence + Jonathan's review): `Cache/Ogre/refine_report.json` (new) · `refine_report_task316.json` + `previews_task316_shipped/` (the SHIPPED state, preserved for the before/after gallery) · `previews/` (new: front/back/threequarter/top/beauty) · `rig/` (report + bind/turntable/contact previews) · `meshy_raw.glb` (new donor) + `meshy_raw_task316_42d2ab2.glb` (old donor, fallback) · `state.json` (meshy-i23d, task id).

**Before/after for Jonathan:** `Cache/Ogre/previews_task316_shipped/preview_front.png` (shipped) vs `Cache/Ogre/previews/preview_front.png` (remake) vs `Content/RawAssets/Concepts/Ogre.png` (approved concept).

---

# TURNKEY same-path IMPORT recipe (TASK-341, build-master, editor-gated) — zero judgement calls

**Serialized, EXCLUSIVE editor session; Simulate STOPPED before every import/save (lane-knowledge 8). NEVER delete+recreate — same-path OVERWRITE preserves every ref (`BP_Unit_Ogre`, `DT_Cards`, ghost `/Game/Meshes/SM_Ogre` string, CardID-composed SK resolve, ABP/anim bindings). `L_Arena` never saved.**

### 🚨 Trap 0 — THE TEXTURE-SKIP TRAP (law)
`Tools/reimport_meshes.py::_ensure_textures_and_mi()` early-returns because `MI_Ogre_PBR` exists ⇒ a plain mesh-reimport run SILENTLY SKIPS the textures — new UVs on the OLD dark atlas, shipping attempt #3 with attempt #2's pixels. **Import the three textures EXPLICITLY (same-path, `replace_existing`) as their own step and READ BACK size/timestamp** (new D = 1,008,337 B, 2026-07-27). Do NOT delete the MI — texture overwrite propagates for free.

### 1. Textures → `/Game/Textures/` (overwrite in place)
- `T_Ogre_D` ← `Content/RawAssets/Textures/Ogre/T_Ogre_D.png` — **sRGB ON**, TC_Default
- `T_Ogre_N` ← `…/T_Ogre_N.png` — **LINEAR / sRGB OFF**, TC_Normalmap
- `T_Ogre_ORM` ← `…/T_Ogre_ORM.png` — **LINEAR / sRGB OFF**, TC_Masks

### 2. 🚨 SAMPLER-TYPE TRAP sweep (law, after step 1 — the M_CastleCrumble lesson)
Enumerate **ALL material referencers of `T_Ogre_*` — MASTERS included** (`M_AssetPBR` expected; verify nothing else node-defaults these textures) and verify each still COMPILES. **`Failed to compile Material` grep = 0 hits on a fresh load** — a Default-Material fallback anywhere is an automatic verification FAIL. Add the grep to the Message-Log sweep.

### 3. `MI_Ogre_PBR` — do NOT recreate
Master `/Game/Materials/M_AssetPBR`, params `BaseColor`=T_Ogre_D, `Normal`=T_Ogre_N, `ORM`=T_Ogre_ORM. Textures overwritten in place ⇒ nothing to rewire.

### 4. Static mesh `SM_Ogre`
- Import `Content/RawAssets/Ogre.fbx` OVERWRITING `/Game/Meshes/SM_Ogre` at the same path (`AssetImportTask(replace_existing=True)`; `Tools/reimport_meshes.py` is the proven lane — MCP `import_file` cannot overwrite). **Nanite OFF.** Import normals. Collision per the TASK-037 unit recipe (≤4 hulls).
- Slots EXACTLY, in order: **0 `TeamRegion` → `MI_TeamColor_Blue` (design default), 1 `OgrePBR` → `MI_Ogre_PBR`.** FBX slots already `[TeamRegion, OgrePBR]` → 1:1. (Commandlet limitation: finalize slot→MI pointers over MCP post-relaunch, `Tools/reimport_apply_materials_mcp.py`.)
- **SM-LOD readback gate (the `SM_MilitiaMob` trap, TASK-320/322 — its signature exactly):** read back **LOD0 welded verts AND per-LOD triangle counts**. Expected LOD0 **14,999 tris / ~7,470 verts** (measured on the FBX); fleet unit chain ⇒ expected per-LOD tris ≈ **14999 / 7500 / 3750 / 1874**. Any 0-triangle LOD or `verts == tris×3` (≈44,997 unweld) = **HARD FAILURE: restore last-known-good (`42d2ab2`), do NOT commit, report.**

### 5. Skeletal mesh `SK_Ogre` (the runtime visual)
- Import `Content/RawAssets/Characters/Ogre.fbx` OVERWRITING `/Game/Characters/SK_Ogre` at the same path. **SKELETON GATE: bind to the EXISTING `/Game/Characters/SK_Footman_Skeleton`** (root `Footman_Rig`; the FBX carries the exact 21-bone SiegeBiped contract ⇒ clean bind). `SK_Footman_Skeleton` must remain the **SOLE** skeleton in `/Game/Characters` — **a new/stray skeleton asset is a HARD FAILURE** (restore, do not commit). `SK_Ogre_Skeleton` does not exist and never did. **Nanite OFF.** Same two slots `[TeamRegion, OgrePBR]`.
- **Anims + ABP: PRESERVED, do NOT touch.** No reimport of `/Game/Characters/Anims/A_Ogre_{Idle,Walk,Attack,Death}`, no ABP edit (raw anim FBXs are byte-untouched — nothing new exists to import). After the SK overwrite confirm all four still bind (same skeleton ⇒ they must) and TICK live (bone-delta sample, not a screenshot).

### 6. SK-LOD chain — REGENERATE post-import (same-path reimport drops to LOD0-only)
Apply `Content/RawAssets/Characters/Ogre.lod.json` via `SkeletalMeshEditorSubsystem.regenerate_lod` (TASK-297 recipe; property is `num_of_triangles_percentage` — the TASK-297 NIT): **LOD1 50 %@0.4 / LOD2 20 %@0.15.** Readback **`lod_count == 3` + per-LOD verts** (TASK-297 precedent for the old mesh: 21899/12257/6638 — new counts will differ, expect ≈7470-verts-LOD0 scale). URO flags live in C++ (`SummonedUnit.cpp:155-156`) — nothing to set on the asset.

### 7. FACING / GROUNDING FENCE (ruling 7 — CLOSED, observe only)
Author **NO component transform** — yaw (−90) and Z are C++-derived (`ResolveSkeletalVisual`). **Ogre capsule half-height is 145** — read `GetScaledCapsuleHalfHeight()`, never assume 90. In Simulate merely OBSERVE: live Ogre faces travel direction, feet ground ~2.1–2.4 cm fleet-standard.

### 8. Verify in Simulate on `L_Arena` (never PIE-in-viewport)
(a) Ogre reads VIVID matching `Concepts/Ogre.png` — capture the side-by-side AND the TASK-310-gallery framing so before(0.1416-era)/after is direct; (b) team recolour on a RED bot Ogre — the region is now **2.36 %** (vs the near-invisible 1.33 %) on the pauldron/shoulder band; (c) preserved anims tick (live bone-delta); (d) Message Log clean (ensure/AccessedNone/Fatal = 0) **AND** the step-2 `Failed to compile Material` grep = 0.

### 9. ONE commit on main, explicit pathspecs, NO push
`SM_/SK_/T_Ogre*/MI_Ogre_PBR` uassets + the raw FBX/PNG/lod.json sources + `pipeline_manifest.json` + board/handoff docs. `git diff --stat` must show nothing foreign. `L_Arena` NEVER saved; `git reset --hard`/`git clean -fd` BANNED.

**WATCH (ruling 9 — say it in the TASK-341 handoff too):** Jonathan's playtest eye is the final authority. If attempt #3 STILL reads dark to him, the numbers above go back to the manager for adjudication — no fourth silent re-run.

---

## Discipline
NO editor / MCP / Unreal import / Git / TASKBOARD / CONVENTIONS writes. Meshy spend exactly the approved ~30 credits (2476 → 2446); all Stage-2/rig iterations were free local re-runs. `MESHY_TOKEN` never echoed, never written, never on argv. Files written: the 6 staged sources above, the manifest Ogre block, gitignored Cache artifacts, this handoff.
