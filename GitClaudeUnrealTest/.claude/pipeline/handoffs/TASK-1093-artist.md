# TASK-1093 — [CHAR-BIND] the knight is bound to the hero's OWN skeleton. Nothing was authored; nothing was retargeted.

**art-director · 2026-09-06 · marker `TASK-1093-CHAR-BIND`**
**Status → `ready-for-integration` (host: `TASK-1094`, build-master — the mesh swap on `BP_HeroCharacter` + the PIE walk/fight proof)**

---

## 1. THE BINDING ANSWERS FIRST — READ BACK FROM THE EDITOR, NOT FROM THE IMPORT CALL

Every value below was queried from the saved asset in the running editor (PID 20564) AFTER the save
(`SC-§94` cl. A). The import call's return value was not used as evidence for anything.

| question | read-back |
|---|---|
| **Resolved skeleton** | `SkeletalMeshTools.get_skeleton(SK_MainCharacter)` → **`/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin`**. Asset-registry tag `Skeleton` = the same. |
| **Bone count** | registry tag **`Bones: 89`** on the mesh — the exact bone set `SKM_Quinn_Simple` rides (its own ref skeleton, parsed from its package, is 89 bones). The skeleton `SK_Mannequin` reads **161 bones before AND after** both imports. |
| **Skeleton untouched** | `is_dirty(SK_Mannequin)` = **false** after import #1 and after import #2. `SKM_Quinn_Simple`, `PA_Mannequin`: **false**. Nothing was merged into the vendor skeleton — 89 ⊂ 161 with identical parents (verified on disk before import, and by a probe import that produced an 89-bone hierarchy rooted at `root`, no `Armature` bone). |
| **Retargeter needed?** | **NO.** No `IK_*`, no `RTG_MeshyBiped_to_Mannequin` was created. `RTG_MeshyBiped_to_SiegeBiped` / `IK_MeshyBiped` / `IK_SiegeBiped` / `SK_Footman_Skeleton` / `ABP_Footman` were never opened, never referenced. `get_dependencies(SK_MainCharacter)` = `[SK_Mannequin, PA_Mannequin, MI_MainCharacter_PBR]` — nothing else. |
| **Credits spent** | **0 Meshy credits, 0 HF quota.** The whole lane ran in headless Blender + the live editor. |
| **Triangles / vertices** | tags `Triangles: 23990`, `Vertices: 28086` (render verts; 11,968 source verts), `MaxBoneInfluences: 4`, `LODs: 1` |
| **Nanite** | `naniteSettings.bEnabled` = **false**; tag `NaniteEnabled: False` |
| **Forward axis** | `forwardAxis` = **`Y`** — same as `SKM_Quinn_Simple` |
| **Bounds** | origin `(0.00, -0.14, 95.88)`, extent `(93.53, 26.48, 95.87)` ⇒ **187 × 53 × 192 uu**, feet at z≈0 |
| **Persona viewport** (rung 3, the engine rendering its own achieved state) | *"Previewing Reference Pose · Triangles 23,990 · Vertices 28,086 · UV Channels 1 · Approx Size 187×53×192"* — the knight standing upright, textured, chest cross facing the default front camera. Capture: `.claude/pipeline/playtest-evidence/2026-09-06/MainCharacter_ue_persona_refpose.png` |
| **Material slots** | `get_material_slots` → **`["MainCharacterPBR"]`** → `MI_MainCharacter_PBR`. ⚠️ **`TeamRegion` is ABSENT** — see §5 (it has zero faces by the TASK-1091/1092 decision; the importer drops an empty slot). |
| **Physics asset** | `get_physics_asset` → **`/Game/Characters/Mannequins/Rigs/PA_Mannequin`** (assigned by me; a read-only donor reference — see §5). |
| **Import provenance** | tag `AssetImportData` = `../RawAssets/Characters/MainCharacter.fbx`, `FileMD5 90526b4193054d7e4ba4ea02e10cb73a` — matches the raw FBX on disk (md5 verified). |

### Textures (registry `Dimensions` tag — the honest instrument; `TextureTools.get_size` reports the *resident mip*, which read 32×32 even for shipped `T_Footman_D`)

| asset | Dimensions | Format | SRGB | Compression | LODGroup |
|---|---|---|---|---|---|
| `/Game/Textures/T_MainCharacter_D` | 2048x2048 | DXT1 | True | TC_Default | TEXTUREGROUP_World |
| `/Game/Textures/T_MainCharacter_N` | 2048x2048 | BC5 | False | TC_Normalmap | TEXTUREGROUP_WorldNormalMap |
| `/Game/Textures/T_MainCharacter_ORM` | 2048x2048 | DXT1 | **False** | **TC_Masks** | TEXTUREGROUP_World |

The ORM imported as `sRGB=true / TC_Default` (the importer's default — the same defect `TASK-728` recorded) and was
corrected explicitly, then re-read. `_D` and `_N` imported correctly as-is (`_N` auto-detected as a normal map).

### Material instance
`/Game/Materials/Instances/MI_MainCharacter_PBR` — parent read back `/Game/Materials/M_AssetPBR`; texture params read back
`BaseColor → T_MainCharacter_D`, `Normal → T_MainCharacter_N`, `ORM → T_MainCharacter_ORM`.

---

## 2. WHAT LANDED — exact paths

| asset | path | on disk |
|---|---|---|
| Skeletal mesh | **`/Game/Characters/SK_MainCharacter`** | `Content/Characters/SK_MainCharacter.uasset` (6,594,027 B, 09:48) |
| Base color | **`/Game/Textures/T_MainCharacter_D`** | `Content/Textures/T_MainCharacter_D.uasset` |
| Normal | **`/Game/Textures/T_MainCharacter_N`** | `Content/Textures/T_MainCharacter_N.uasset` |
| ORM | **`/Game/Textures/T_MainCharacter_ORM`** | `Content/Textures/T_MainCharacter_ORM.uasset` |
| Material instance | **`/Game/Materials/Instances/MI_MainCharacter_PBR`** | `Content/Materials/Instances/MI_MainCharacter_PBR.uasset` |

Saved by an **explicit five-path list**, never the empty-list save-all. `is_dirty` = false on all five after the save; `L_Arena` never
opened, never saved — on-disk sha256 `1f78419d88894073…` unchanged before/after, `git status Content/Maps/` empty.

**Raw sources (raw-asset rule of the SK law — declared writes outside the row's enumerated list):**
- `Content/RawAssets/Characters/MainCharacter.fbx` — the rigged bind-pose FBX (1,374,924 B, sha256 `d4582d7d1c1ec309…`, md5 `90526b41…` = the asset's `AssetImportData`). Armature + mesh, 89 bones, 25 vertex groups, slots `[TeamRegion, MainCharacterPBR]`, `UVMap`, 23,990 tris.
- `Content/RawAssets/Concepts/MainCharacter.png` — the accepted concept copied at the pre-import gate (TRELLIS/Stage-3 law), byte-identical to `Tools/ArtPipeline/Inbox/MainCharacter.png` (sha `77f845db…`). Left un-copied by TASK-1091 for exactly this step.
- The static `Content/RawAssets/MainCharacter.fbx` + `Textures/MainCharacter/*.png` from TASK-1091 are untouched.

**Evidence (`.claude/pipeline/playtest-evidence/2026-09-06/`):** `MainCharacter_rig_bind_{front,side,back}.png`, `MainCharacter_rig_bind_{front,side}_bones_xray.png` (bones drawn inside the body), `MainCharacter_rig_testpose_{front,threequarter}.png` (arm down 70°, elbow 80°, knee raised 45°/60°, head 35° — no tearing), `MainCharacter_ue_persona_refpose.png` (the engine's own render).

**Report (gitignored):** `Tools/ArtPipeline/Cache/MainCharacter/rig/rig_report.json` + `refskel_parsed.json` (the parsed ref poses).

---

## 3. HOW IT BINDS — the method, so the next hero remaster does not re-derive it

1. **The skeleton's reference pose was read from the packages, not guessed.** `SK_Mannequin.uasset` (161 bones) and
   `SKM_Quinn_Simple.uasset` (89 bones) were parsed for `FReferenceSkeleton` (bone name / parent / local quat+translation;
   unit-quaternion error 2e-16, scale exactly 1). No property or MCP tool exposes the ref pose; there is no Python door while the
   editor is up; no mannequin FBX exists on this machine. Parser: `refskel_parsed.json`.
2. **The knight's own proportions are lawful.** `SK_Mannequin.boneTree` reads `AnimationScaled` on every body bone (only `root`
   = Animation, 6 finger tips = Skeleton, the 9 ik/helper bones = Animation). That is the mode that already lets the shorter
   `SKM_Quinn_Simple` ride Manny's animations with its own bone lengths — so the knight carries fitted lengths, not Quinn's.
3. **Fit (UE cm):** uniform `s = 1.058` (shoulder height 151.1 vs Quinn 142.8) · shoulders widened to `x = ±22` (measured from the
   torso/arm thickness profile) · arms swung into the concept's T-pose (`upperarm` +47.2°, `lowerarm` +36.7°, minimal-swing, palms
   down as the mesh has them) with `k_arm = 0.945` · legs abducted 4.0° to the knight's 44 cm stance, `k_leg = 1.005` · feet kept
   flat (Quinn's global foot rotation restored) · `ik_foot_*` / `ik_hand_gun` / `ik_hand_*` placed on the fitted feet/hands exactly
   as Quinn's are · every other bone = Quinn's local rotation, translation × s.
4. **Bind frames are exact.** The export armature reproduces the fitted UE frames to 7e-6 (Blender ↔ UE is the reflection
   `(x, −y, z)`, so `R_b = M·R_ue·M`). Bind rotations may differ from the skeleton's — Quinn's already do (hand dquat 0.06) — because
   the animation replaces every local rotation and the skin uses the mesh's own inverse-bind matrices; what must match is the
   pose the mesh is skinned in, and it is.
5. **Skinning:** bone heat on a 25-bone long-bone heat rig (a second armature with joint-to-joint segments; the exact-frame armature
   has 4 cm stub bones that heat cannot use), weights transferred by name; heat left 24 of 11,968 verts unweighted (mesh has 30
   non-manifold edges, 10 islands — the cloak/scabbard shells) → segment-distance envelope for those 24; then a 2-pass Laplacian
   smooth over mesh edges, top-4 influences, renormalised. **Deform set = 25 bones** (pelvis, spine_01–05, neck_01–02, head,
   clavicle/upperarm/lowerarm/hand ×2, thigh/calf/foot/ball ×2). Twist and finger bones exist in the hierarchy but carry no
   weight (the gauntlets are mittens per TASK-1091).
6. **Facing was measured, not assumed:** the chest-cross albedo (red) was sampled on the ±Y chest faces of the imported static FBX
   — front was +Y in the Stage-2 file (its export carries the static path's Y-mirror pre-compensation), so the mesh was rotated 180°
   (a rotation, not a mirror — the scabbard stays on the left hip). Result: front = Blender −Y = UE +Y = `forwardAxis Y`, the
   mannequin's convention, confirmed by the Persona front camera seeing the chest cross.
7. **Export:** the shipped skeletal contract (`axis_forward −Z / axis_up Y`, `add_leaf_bones False`, `primary Y / secondary X`,
   `FBX_SCALE_NONE`) — with ONE deliberate difference, §4. Armature object named `Armature` so UE's Blender hack drops the null
   node (verified: probe import rooted at `root`, 89 bones, no extra bone). Round-trip re-import: 89/89 bones, no extra, no
   missing, no parent mismatch, head error 2.8e-6 m, root bone local scale 1.0, 25 vertex groups, both material slots, `UVMap`.

---

## 4. ⭐⭐ THE DEFECT THAT WAS CAUGHT BEFORE HANDOFF — a 100× root-bone scale, invisible to every property read-back

**The first import passed every property check in §1 and was WRONG.** Skeleton, bone count, bounds (187×53×192), slots, Nanite,
tags — all correct. Only the engine's own rendering showed it: the Persona viewport read *"Approx Size 5,014 × 3,569 × 6,361"*
and drew the knight as a dot in the middle of a 60 m box (`MainCharacter_ue_thumbnail_SK_MainCharacter.png` — the two empty
checkerboard thumbnails — are that state).

**Cause, measured:** Blender's `apply_scale_options='FBX_SCALE_NONE'` bakes the m→cm unit factor (×100) into the ROOT object's
node — the `Armature` null. UE strips that null (the Blender hack) but the scale survives into the first bone: `root` imports with
local scale 100, every descendant's local translation in metres. The composed ref pose is still 190 uu tall (the ×100 and the /100
cancel), the skin still solves to identity in the ref pose — which is why `get_bounds` and the skin looked right — but
`PA_Mannequin`'s bodies attached to a scale-100 bone inflate to ~10 m each (⇒ the 60 m component bounds), and **at runtime every
animation writes `root` back to scale 1 (retarget mode `Animation`), so the animated knight would have shrunk to a 2 cm
figurine** — exactly `TASK-1094`'s rung (c), and it would have looked like a T-pose/explosion bug in someone else's lane.

**Fix:** author at 1 Blender unit = 1 cm with `scene.unit_settings.scale_length = 0.01` (unit factor = exactly 1.0 ⇒ no node carries
a scale). Re-exported, the old asset (no referencers — it was created in this task, so the never-delete-and-recreate law had
nothing to protect) deleted and re-imported at the same path. Read back after: root scale 1.0 in the round-trip, Persona *"Approx
Size 187×53×192"* with the knight visible and upright. **A side benefit that is not a coincidence: bone heat converged at cm scale
(it failed for 100 % of vertices at metre scale on this dense mesh) — the final weights are heat, not envelope.**

⚠️ **Observation for the record, ⛔ not measured, ⛔ not my lane:** `Tools/ArtPipeline/rig_character.py` exports the units with the
same `FBX_SCALE_NONE` contract and its `Footman_Rig` armature node deliberately becomes the units' root BONE — which means
`SK_Footman_Skeleton`'s root may carry that same scale 100. It would be self-consistent there (the anims come through the same
exporter), which is why nothing has broken — but anyone binding a mesh authored at metre scale to that skeleton, or retargeting
mannequin anims onto it, should read the root bone's scale first.

---

## 5. RESIDUALS — declared, not hidden

1. **`TeamRegion` slot is absent** on `SK_MainCharacter` (slots = `[MainCharacterPBR]`). The FBX carries both slots; the slot has
   ZERO faces (TASK-1091 §5b / the TASK-1092 approval: no team tint on the hero, so the helmet is judged as generated) and UE's
   importer creates no slot for a material with no faces. The hero is not a `SummonedUnit`; `BeginPlay` slot-0 recolour never runs
   on him; `BP_HeroCharacter.CharacterMesh0.overrideMaterials` is `[]`. If the hero is ever to be team-tinted, add `team_region`
   selectors in `pipeline_manifest.json`, re-run Stage 2 (0 credits) and re-rig — the slot reappears with faces.
2. **`PA_Mannequin` assigned** (read-only vendor donor; `is_dirty` false). Quinn had it; a skeletal mesh with no physics asset has no
   mesh collision, and I did not want the swap to silently change whatever traces the hero's mesh today. The bodies are Quinn-sized
   (the knight is 6 % taller and broader). `TASK-1094` may keep, clear, or replace — it is one property.
3. **Height 191.75 vs Quinn 180.1.** `CharacterMesh0` relative transform read back `(0, 0, −89)`, yaw −90 — unchanged, and the knight
   uses the same feet-at-z0 convention, so the same offset plants his feet where Quinn's are. His helmet crown sits ~12 uu higher
   than Quinn's head — it may poke above the capsule top; cosmetic, capsule collision unchanged. 🧑 His eye.
4. **Foot IK.** `ik_foot_*` sit on the fitted feet in the ref pose, but the ik bones' retarget mode is `Animation`: at runtime they
   follow the anim's (Manny-sized) values, ±8 uu inboard of the knight's wider stance. Whether `ABP_Unarmed`'s foot IK pulls the
   feet inward is observable only in PIE — rung 4, `TASK-1094` (a) — I have not seen him walk.
5. **Envelope on 24 verts, twist bones unweighted, mitten hands** — forearm twist is carried by `lowerarm`; a wrist roll in
   `AM_ComboAttack` will read slightly stiff. Acceptable at gameplay distance; a later polish row if it shows.
6. **Skirt/cloak** are skinned by heat to thighs (skirt front/back) and spine/pelvis (cloak); a deep lunge in the combo may clip the
   cloak through a calf. Expected for un-simulated cloth; observe.
7. **`importedMaterialSlotName` = `None`** on the one slot — only matters for a future same-path reimport's slot matching; with one
   slot it cannot mis-order.

---

## 6. WHAT `TASK-1094` MUST DO (and what it must not)

- **The swap:** `BP_HeroCharacter` → `CharacterMesh0.skeletalMeshAsset`: `SKM_Quinn_Simple` → **`/Game/Characters/SK_MainCharacter`**.
  Read back today: `animClass = ABP_Unarmed_C`, `animationMode = AnimationBlueprint`, relative `(0,0,−89)` / yaw −90,
  `overrideMaterials []`. ⛔ Touch none of those — the whole point of `CHAR-§4` is that they keep binding because the skeleton is
  the same object.
- **The proof (rung 4, `CHAR-§6`):** PIE — (a) he WALKS under `ABP_Unarmed`, (b) `AM_ComboAttack` plays on him, (c) no T-pose,
  no explosion, no shrink (§4 is exactly the shrink you would be looking for — if he is ever tiny, read the root bone scale),
  no floor sink, (d) the VRAM delta: three 2048² textures (DXT1 + BC5 + DXT1 ≈ 2.7 + 5.3 + 2.7 MB with mips) + a 28k-vert
  skeletal mesh ≈ 12–14 MB resident, on the budget `FIELD-§3` already reports as overdrawn — report it even though it is small.
- **Git:** all six new files are untracked and NOT mine to stage. ⚠️ **Another agent's in-flight `git add` briefly showed
  `SK_MainCharacter.uasset` / `MI_MainCharacter_PBR.uasset` / `T_MainCharacter_ORM.uasset` as staged during this task** (I ran no
  git). At my last check the index oid of `SK_MainCharacter.uasset` equals the working file's — i.e. the current cm-scale version —
  but verify **oid-vs-sha256 (`SC-§68`), never size**, before the commit: a stale index copy would be the §4 root-scale asset.
  LFS filter applies to `.uasset` (`git check-attr` → `lfs`).
- Do not re-save `SKM_Quinn_Simple`, `SK_Mannequin`, `PA_Mannequin`, `ABP_Unarmed`, `AM_ComboAttack` — none are dirty.

---

## 7. FENCES — verified, not assumed

- ✅ Bound to `SK_Mannequin`; `SK_Mannequin` 161 bones before/after, never dirty, never saved. The units' rig and retarget chain untouched.
- ✅ `BP_HeroCharacter` read only (CDO component properties), never written. `Content/Characters/Mannequins/**`, `Content/Variant_Combat/**` read only.
- ✅ `L_Arena` never opened, never dirtied by me, never saved; hash unchanged. No actor was ever placed.
- ✅ Saves: explicit paths only. The empty-list save-all was never called.
- ✅ No git command of any kind. Nothing staged by me.
- ✅ 0 credits. No `RTG_`/`IK_` asset created. No animation authored, modified or reimported.
- ✅ Blender heavy work headless (`--background`); the MCP bridge was not used.
- ✅ Two probe assets (`/Game/Characters/_ProbeTmp/SK_MainCharacter_probe{,_Skeleton}`) were created in memory to read how the
  importer treats the Blender null node, then deleted; the folder never existed on disk.

---

## 8. OPERATIONAL FINDINGS FOR THE ORCHESTRATOR (shared-editor law)

1. ⭐⭐ **A PIE session by ANY agent makes every `EditorAssetSubsystem`-gated MCP call (`exists`, `is_dirty`, `save_assets`,
   `delete`, `move`, `duplicate`, `get_asset_tags`, `get_asset_class`) fail with "Asset does not exist" — for EVERYONE, including
   assets that are on disk.** My first save attempts failed that way during another agent's PIE (not the build-master's — their
   handoff says they never started PIE). It reads exactly like a registry wipe of new assets. Gate every save on
   `IsPIERunning` (that tool works during PIE) and retry in the gap; my scripts now do.
2. The `execute_tool_script` result can be replaced by **another session's tool-error dump** (I received a stack of
   `UEDPIE_0_L_Arena … BP_BattlefieldScatter` property-read errors that were not mine). Re-run an idempotent script; add a marker key.
3. `TextureTools.get_size` reports the resident mip, not the source (32×32 for shipped `T_Footman_D`). The registry
   `Dimensions` tag is the instrument for resolution.
4. `CaptureAssetImage` thumbnails of a fresh skeletal mesh rendered empty twice while the asset was in fact broken (§4) — the
   Persona viewport via `OpenEditorForAsset` + `CaptureEditorImage` is the render that showed the truth. The Skeletal Mesh editor
   tab for `SK_MainCharacter` is still OPEN in the editor (there is no close tool); it holds nothing dirty.

**Ladder (`SC-§94` cl. B):** this handoff reaches rung 3 — the engine's achieved state (skeleton, bones, ref pose rendered at
187×53×192). Rung 4 — *does he walk and fight* — is not reached here and is owned by `TASK-1094`. Rung 5 — *does it look right* —
is 🧑 his.

**Editor left:** PID 20564, MCP `127.0.0.1:8000` up, PIE not running at my last check, nothing dirty of mine, `L_Arena` clean.
