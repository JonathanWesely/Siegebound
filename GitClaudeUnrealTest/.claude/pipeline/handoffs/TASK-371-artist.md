# TASK-371 — [AG-A4] UE import: `SM_Sorcerer` + `T_Sorcerer_{D,N,ORM}` + `MI_Sorcerer_PBR`

**Agent:** art-director
**Date:** 2026-08-02
**Editor/MCP:** up (PID 23372). PIE confirmed stopped throughout.
**Status:** ready-for-integration. All five assets imported, configured, verified by hard readback, and saved (`is_dirty = false` on each).
**Constraints honored:** no Git · no C++ · no save-all · `L_Arena` never opened or saved · no skeletal mesh, no anims, no card art, no `ABP_Sorcerer`.

> 🔴 **ONE FINDING THAT CHANGES A TASK-375 ASSUMPTION — read §6 before dispatching TASK-375.** The Sorcerer's mesh faces the
> **OPPOSITE** way from every other unit in the fleet. This is measured, not inferred, from a controlled 4-way comparison.

---

## 1. Assets created — exactly five, no strays

| Asset | Path | Source |
|---|---|---|
| Diffuse | `/Game/Textures/T_Sorcerer_D` | `Content/RawAssets/Textures/Sorcerer/T_Sorcerer_D.png` |
| Normal | `/Game/Textures/T_Sorcerer_N` | `…/T_Sorcerer_N.png` |
| ORM | `/Game/Textures/T_Sorcerer_ORM` | `…/T_Sorcerer_ORM.png` |
| Material instance | `/Game/Materials/Instances/MI_Sorcerer_PBR` | new, from `/Game/Materials/M_AssetPBR` |
| Static mesh | `/Game/Meshes/SM_Sorcerer` | `Content/RawAssets/Sorcerer.fbx` |

`find_assets("/Game", "Sorcerer")` returns **exactly these five and nothing else.** The FBX was imported with
`import_materials = false, import_textures = false`, so the importer created **zero junk Material/Texture assets** — a real risk on this
path, since the FBX names its slots after materials that do not exist in `/Game`.

`get_dependencies(SM_Sorcerer)` = `[MI_TeamColor_Blue, MI_Sorcerer_PBR]` (+ `/Script/NavigationSystem`). No dangling or unexpected refs.

**Pre-flight confirmed this was a genuine FIRST import:** `find_assets("/Game", "Sorcerer")` returned `[]` before I started, so there was no
same-path overwrite to preserve and no prior asset to protect.

---

## 2. Textures — sampler-type sweep, matched 1:1 to the shipped fleet

I did not invent these settings; I read them off `T_Wizard_{D,N,ORM}` first and matched exactly.

| Asset | `sRGB` | `compressionSettings` | `lODGroup` | size |
|---|---|---|---|---|
| `T_Sorcerer_D` | **`true`** ✅ | `TC_Default` | `TEXTUREGROUP_World` | 1024 × 1024 |
| `T_Sorcerer_N` | `false` | **`TC_Normalmap`** | `TEXTUREGROUP_WorldNormalMap` | 1024 × 1024 |
| `T_Sorcerer_ORM` | **`false`** ✅ | **`TC_Masks`** | `TEXTUREGROUP_World` | 1024 × 1024 |

Every row above is a **post-set readback**, not the value I sent. The two that matter most for correctness — `_D` sRGB **ON** and `_ORM`
sRGB **OFF / linear** — are both confirmed. `TC_Masks` additionally forces the ORM to sample unfiltered-linear per channel, which is the
whole point of packing occlusion/roughness/metallic together.

**Textures were imported EXPLICITLY**, one call each, per the texture-skip trap in the law.

---

## 3. `MI_Sorcerer_PBR`

| | |
|---|---|
| Path | `/Game/Materials/Instances/MI_Sorcerer_PBR` |
| Parent (readback) | **`/Game/Materials/M_AssetPBR`** ✅ |
| `BaseColor` (readback) | `/Game/Textures/T_Sorcerer_D` ✅ |
| `Normal` (readback) | `/Game/Textures/T_Sorcerer_N` ✅ |
| `ORM` (readback) | `/Game/Textures/T_Sorcerer_ORM` ✅ |

`list_parameters(M_AssetPBR)` returns exactly `[BaseColor (Texture), ORM (Texture), Normal (Texture)]` — the three names in the spec are the
master's complete parameter set, so there is no fourth parameter left unset and no misnamed one silently ignored. Folder and parent match the
`MI_Wizard_PBR` precedent (also parented to `M_AssetPBR`, also in `/Game/Materials/Instances/`).

---

## 4. `SM_Sorcerer` — hard readback of every acceptance criterion

| Criterion | Required | **Readback** | Verdict |
|---|---|---|---|
| Path | `/Game/Meshes/SM_Sorcerer` | `/Game/Meshes/SM_Sorcerer` | ✅ (placement-ghost string contract) |
| Triangles (LOD 0) | ≤ 15,000 | **15,000** | ✅ exactly at budget |
| **Nanite** | **OFF** | `is_nanite_enabled` → **`false`** | ✅ |
| `lODGroup` | `LargeProp` | **`LargeProp`** | ✅ |
| LOD chain | classic LODs | **`lod_count` = 4**; thresholds `[2.0, 0.3127, 0.1690, 0.0992]` strictly descending | ✅ |
| **Material slots, IN ORDER** | `[TeamRegion, SorcererPBR]` | **`["TeamRegion", "SorcererPBR"]`** | ✅ **verified by readback, not assumed** |
| Slot 0 material | `MI_TeamColor_Blue` | `/Game/Materials/Instances/MI_TeamColor_Blue` | ✅ |
| Slot 1 material | `MI_Sorcerer_PBR` | `/Game/Materials/Instances/MI_Sorcerer_PBR` | ✅ |
| Collision | ≤ 4 simple hulls | **exactly 4 `convexElems`**, 16 verts each; 0 sphere/box/sphyl/capsule elems | ✅ |
| Origin | feet-centre | bounds min Z **0.0586**, X −45.69…45.29, Y −39.16…39.21 | ✅ |

**Slot names matched the FBX 1:1 with zero renaming needed** — as TASK-370 predicted, but I confirmed it by reading the slots off the imported
asset *before* assigning anything, exactly as instructed rather than trusting the prediction.

**Bounds cross-check against TASK-370's report** (which measured the FBX in Blender, independently of UE):

| axis | TASK-370 (Blender) | UE import readback |
|---|---|---|
| min Z | 0.059 | **0.0586** |
| X | −45.69 … 45.29 | **−45.693 … 45.287** |
| Y | −39.21 … 39.16 | **−39.157 … 39.212** |

X and Z agree to 3 decimal places. **Y is mirrored** (Blender −39.21/+39.16 ↔ UE −39.157/+39.212) — that is the expected FBX
handedness/axis conversion on import, not a defect, and it is worth knowing alongside §6.

**Collision hull coverage** (elem bounding boxes, Z range) — the four hulls tile the whole figure rather than clumping:
base/feet `−1.3…24.0` · legs/lower body `10.3…94.7` · torso `86.9…123.4` · head+antlers `116.9…183.0`.

---

## 5. ✅ Visual gate — I could actually SEE this one

Unlike TASK-368's WidgetBlueprint, **`CaptureAssetImage` DOES support StaticMeshes**, so this import got a real eyeball check, not just
property readback. I rendered `SM_Sorcerer`'s thumbnail and reviewed it.

**What the render confirms:**
- **The textures are live and on-palette** — jade/teal robe, cream cowl, tan antlers, pale stone plates. **Not grey, not white, not the
  checkerboard default**, so `MI_Sorcerer_PBR`'s three texture params genuinely resolve through `M_AssetPBR`.
- **`TeamRegion` renders BLUE and it is on the two shoulder plates** — visually confirming both that `MI_TeamColor_Blue` landed on slot 0 and
  that TASK-370's "two symmetric islands on the plate tops" assertion survived the round trip into UE.
- **Zero team paint on the head or antlers** — the antlers read tan/bone and the skull reads pale green, exactly as TASK-370 asserted
  (the Archer bare-HEAD lesson holds).
- Identity intact: antler crown, cream cowl/scarf, moss-green robe with ragged hem, sash, boots.

Preview kept at `…/scratchpad/SM_Sorcerer_thumb.png` (scratch, not committed).

---

## 6. 🔴 THE FACING FINDING — the fleet convention does NOT hold for the Sorcerer

**The brief's carry-forward said:** `Sorcerer.fbx` fronts **+Y**, *"consistent with the documented fleet convention, so `BP_Unit_Sorcerer`'s
`VisualMesh` yaw −90 should be right"* — and asked me to note the observed facing so TASK-375 verifies empirically rather than trusting the
convention. **I did better than note it: I measured it against the fleet, and the convention does not hold.**

**Method — a controlled A/B/C/D.** `CaptureAssetImage` uses the *same fixed default thumbnail camera* for every StaticMesh, so rendering four
units and comparing which face you see is a valid relative test of source-FBX yaw. I rendered three shipped, known-good units plus the Sorcerer:

| Mesh | What the default thumbnail shows | Evidence in the render |
|---|---|---|
| `SM_Footman` | **FRONT** | face, spear, shield presented toward camera, blue helm |
| `SM_Cleric` | **FRONT** | hooded face, staff raised, tome, blue shoulder cape |
| `SM_Wizard` | **FRONT** | face under hood, fireball in the raised hand, red orb staff |
| **`SM_Sorcerer`** | **🔴 BACK** | back of the skull, antler crown from behind, **no mask/face**, **monolith hidden**, sash down the back |

**3 of 3 shipped units face front. The Sorcerer is the only one facing backward — it is rotated ~180° about Z relative to the entire fleet.**

**What this proves and what it doesn't.** It proves the *relative* rotation: whatever yaw makes the Cleric/Footman/Wizard face correctly will
leave the Sorcerer facing **backwards**. It does **not** independently establish the absolute world axis — I cannot read the thumbnail
camera's world orientation from here, so I am not claiming "+Y" or "−Y" as a measured fact.

**Consequence for TASK-375 (and why this matters now):** the premise that yaw **−90** "should be right because the convention holds" is
**not supported by the evidence** — the Sorcerer needs roughly an **extra 180°** relative to the fleet value, i.e. about **+90**. TASK-375
should treat **+90 as the leading candidate and −90 as the suspect one**, and must still confirm in the viewport. Note this independently
corroborates TASK-373's observation that the Sorcerer's first card render came out as the back of the skull with the monolith hidden — the
same root cause, seen through a different lens.

> **This is exactly the failure `BP_Unit_Wizard` shipped and needed a follow-up task to fix.** Catching it here costs one number; catching it
> at playtest costs a correction round. Worth noting: `SM_Wizard`'s *mesh* renders front-facing, so the Wizard's historical facing bug lived
> at the Blueprint yaw, not in its FBX — the Sorcerer's problem is the opposite and lives in the mesh itself.

---

## 7. ⚠️ FLAGS

### 7a. The `reimport_meshes.py` texture-skip trap — does NOT apply today, WILL apply to any Sorcerer rebuild
`_ensure_textures_and_mi` **early-returns when the MI already exists.** Today that was harmless: this was a first import, `MI_Sorcerer_PBR`
did not exist, and I imported all three textures explicitly by hand anyway. **From now on it is armed** — `MI_Sorcerer_PBR` exists, so a future
`reimport_meshes.py` run on the Sorcerer will **silently skip re-importing `T_Sorcerer_{D,N,ORM}`** and quietly reuse the current bakes even if
`Content/RawAssets/Textures/Sorcerer/*.png` has been regenerated. **Any Sorcerer re-bake must re-import the three textures explicitly (or
delete the MI first).** Recording this per the brief.

### 7b. `SM_Wizard` is a fleet OUTLIER on `lODGroup` — the Sorcerer follows the fleet, not the Wizard
The board specified `lod_group='LargeProp'`. Before applying it I checked whether the shipped fleet actually agrees, because the most obvious
reference asset disagrees:

| mesh | `lODGroup` | `lod_count` |
|---|---|---|
| `SM_Cleric` | `LargeProp` | 4 |
| `SM_Ogre` | `LargeProp` | — |
| `SM_Longbowman` | `LargeProp` | — |
| `SM_MilitiaMob` | `LargeProp` | — |
| **`SM_Wizard`** | **`None`** | **1** | ⚠️ **outlier** |
| **`SM_Sorcerer` (this task)** | **`LargeProp`** | **4** | ✅ follows the fleet |

**I followed the board and the fleet majority.** Flagging that `SM_Wizard` has **no LOD group and only LOD 0** — it is missing its classic-LOD
chain entirely, which looks like the same M7.6 gap that produced its facing follow-up. **Not mine to fix in this task** (it is a shipped asset
outside TASK-371's scope) but it is a real, cheap perf/consistency defect worth a manager decision.

### 7c. Static vs rigged FBX paths (carried forward from TASK-370 §7b, unchanged)
`SM_Sorcerer` was imported from **`Content/RawAssets/Sorcerer.fbx`** (the Stage-2 static FBX, where all shipped static assets live).
`Content/RawAssets/Characters/Sorcerer.fbx` is the **RIGGED** FBX from `rig_character.py` and is TASK-372's input — a different file.
Not a defect; do not "reconcile" them.

---

## 8. Concept-fidelity numbers, quoted back from TASK-370 §5b

These were measured at the pre-import gate, not re-derived here — the imported asset is the same FBX those numbers describe:

| metric | value | bar |
|---|---|---|
| UV-normalised mean linear albedo | **0.4377** | ≥ 0.2536 ✅ (73% headroom) |
| luma retention vs alpha-masked concept | **0.9983** | 0.85–1.25, target ≈1.0 ✅ |
| chroma retention | **0.8596** | floor 0.60 ✅ |
| dominant-cluster hue shift | **0.75°** | ≤ 20° ✅ |

---

## 9. Save state

`save_assets` called with the **five asset paths named explicitly — never save-all, `L_Arena` never touched.**
`is_dirty` re-checked individually afterwards: `SM_Sorcerer` **false** · `MI_Sorcerer_PBR` **false** · `T_Sorcerer_D` **false** ·
`T_Sorcerer_ORM` **false**.

---

## 10. What downstream needs from me

- **TASK-372 (editor half):** `SM_Sorcerer` is in place; the skeletal path is untouched by this task. Bind `SK_Sorcerer` to the **existing**
  `/Game/Characters/SK_Footman_Skeleton` — never create a skeleton. Rigged FBX is the *other* file (§7c).
- **TASK-375 (`BP_Unit_Sorcerer`):** ⚠️ **read §6.** `VisualMesh` → `/Game/Meshes/SM_Sorcerer`; slot 0 `TeamRegion` already carries
  `MI_TeamColor_Blue` as the design-time placeholder, so the BeginPlay team recolour has the right slot to overwrite. **Yaw: treat +90 as the
  leading candidate, −90 as suspect, and confirm in the viewport.**
- **TASK-373 (card art):** untouched by this task, as instructed.
- **Nothing was placed in any level and no Blueprint was edited** — integration is not my role.


---
---

# Section 6 REFINED - 2026-08-02, see **`TASK-375-facing-fix.md`**

**Section 6's measurement was RIGHT and its caution was exactly right.** The controlled 4-way thumbnail proved the
*relative* rotation (3/3 shipped units front, Sorcerer back) and explicitly declined to claim an absolute world
axis - that restraint was correct, because the absolute story turned out to be the interesting part.

**What Section 6 could not see:** the Sorcerer's **conformed Blender** orientation was always correct and
fleet-conformant (its Stage-2 `preview_front.png` shows the FRONT, exactly like the Footman's and Cleric's cached
previews). The divergence is introduced **at export**, by the TASK-348 `_ue_handedness_precomp` MIRROR-FIX
(2026-07-28): the Sorcerer is the **first UNIT exported after it**, so it alone arrives in UE un-mirrored.

**Section 4's "Y is mirrored" bounds observation was the smoking gun all along** - FBX-on-disk
`Y -39.212..+39.157` vs UE readback `Y -39.157..+39.212` is an *exact* negation, with X matching identically.
That is UE's RH->LH import negation, and for the pre-fix fleet nothing pre-compensates it.

**Consequence for the `+90` recommendation in Sections 6/10:** it was the correct call for the asset as it then
stood, and TASK-375 shipped it. It has since been **superseded** - the source was fixed instead, and
`BP_Unit_Sorcerer`'s `VisualMesh` is back to the fleet `-90`. **Section 8's concept-fidelity numbers were
re-measured after the re-bake** (UV-norm 0.4377 -> **0.4403**, luma 0.9983 -> **0.9811**, chroma 0.8596 ->
**0.9292**).

**Section 7a's texture-skip trap warning was heeded exactly as written:** the re-import imported
`T_Sorcerer_{D,N,ORM}` **explicitly and unconditionally**, bypassing `reimport_meshes.py`'s `MI already exists`
early-return, so the fresh bakes genuinely landed. That flag did its job.
