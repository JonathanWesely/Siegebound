# TASK-302 — `SK_Wizard` skeletal rig + LOD recipe + anim clips (W-UNIT-1) — art-director handoff

**Status:** BLENDER-HEADLESS RIG DONE, all assets staged on disk. **UE import NOT done — deliberately deferred (editor-gated).** NO editor / MCP / Blueprint / Git / deck-builder touched.
**Date:** 2026-07-26 · **Branch:** m7.6-arena10x

## Scope executed vs deferred
- DONE here (NO-EDITOR, headless Blender 5.1): rigged `SM_Wizard` → `SK_Wizard` on the shared **SiegeBiped** skeleton via `rig_character.py`; authored + exported the 4 anim clips; emitted the SK-LOD recipe; passed the pre-import eyeball gate on every preview.
- DEFERRED (coordinator dispatches separately — editor occupied by the in-flight deck-builder fix): the **Stage-3 UE import** into `/Game/Characters/`. Full turnkey recipe in "UE import recipe" below.

## Assets produced (staged, on disk)
| File | Purpose | Notes |
|---|---|---|
| `Content/RawAssets/Characters/Wizard.fbx` | Rigged skeletal FBX (object `SK_Wizard` + armature `Footman_Rig`) | 824,924 bytes; 14,999 tris / 7,502 verts; 21-bone SiegeBiped; slots `[TeamRegion, WizardPBR]`; `UVMap`; envelope skin **0.0% unweighted** |
| `Content/RawAssets/Characters/Wizard.lod.json` | SK-LOD recipe sidecar (LOD1 50%@0.4 / LOD2 20%@0.15 + URO) | 881 bytes; schema `siege_sk_lod_recipe_v1`; deterministic |
| `Content/RawAssets/Characters/Anims/Wizard_Idle.fbx` | Idle clip (60f @30fps, loop) | shared-skeleton, procedural |
| `Content/RawAssets/Characters/Anims/Wizard_Walk.fbx` | Walk clip (30f, loop) | stride cycle + arm counter-swing |
| `Content/RawAssets/Characters/Anims/Wizard_Attack.fbx` | Attack clip (40f, one-shot) | **`cast`/hurl** — staff-raise then forward drive (NOT a bow-draw) |
| `Content/RawAssets/Characters/Anims/Wizard_Death.fbx` | Death clip (48f, one-shot) | stagger → backward collapse, holds final pose |
| `Tools/ArtPipeline/Cache/Wizard/rig/rig_report.json` | Machine report | bones/skin/anims/material+UV readback; **zero warnings** |
| `Tools/ArtPipeline/Cache/Wizard/rig/previews/*.png` | Eyeball-gate previews | bind stills, 8-angle turntable, per-anim contact strips + PNG frame seqs |

Also updated (pipeline manifest, NOT editor/Git): `Tools/ArtPipeline/rig_manifest.json` — added the `Wizard` asset entry (modeled on the Cleric robed-caster sibling; `weapon_side:r`, `attack_style:cast`).

## How generated (reproducible)
`"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/rig_character.py -- --card-id Wizard` — from the repo root. Exit 0, 40.7 s. NO network/HF call (procedural rig; no cert bundle or token needed). Input = the TASK-301 static `Content/RawAssets/Wizard.fbx` (610,908 bytes, unchanged).

## Skeleton (RECORDED per CONVENTIONS)
Rigged on the **SHARED SiegeBiped** (21 bones, UE-mannequin names), armature OBJECT exported as the constant **`Footman_Rig`** root (TASK-212 law) — so UE binds `SK_Wizard` to the EXISTING shared skeleton `/Game/Characters/SK_Footman_Skeleton` clean, no missing-bones warning. NOT a bespoke skeleton. The adaptive height/half-width fit absorbed the caster's outstretched fireball arm + held staff (measured height 181.94 UE = 1.82 m, depth 99.25, shoulder_half 60.85, hip_half 53.73). Skinning fell to the deterministic **envelope** path (bone-heat first, as designed) — 0.0% unweighted, full coverage.

## Animations — AUTHORED per-unit (roster convention), on the shared skeleton
Per the roster convention, `rig_character.py` **authors** procedural per-unit clips `A_Wizard_{Idle,Walk,Attack,Death}` on SiegeBiped (every roster unit gets its own `A_<CardID>_*` this way; they are retargetable on the shared skeleton). NOT shared-in-place — each is its own exported clip.
- **Attack = `cast`/hurl (spec acceptance MET):** the `cast` style hits the script's raise-arm-then-drive-forward branch (Cleric robed-caster precedent), reading as a staff-raise spellcast/hurl — deliberately NOT `thrust` (the Archer/Longbowman bow-draw read the spec bars). `weapon_side:r` drives the staff arm (staff in the right hand; fireball cradled in the left palm per TASK-301) up-and-forward = iconic caster silhouette at RTS cam.
- **FLAG (Cleric/Archer flag class, NON-blocking):** a true two-hand fireball-hurl (left-hand throw) or a higher-fidelity Meshy cast clip (the Archer `Archery_Shot_1` lane) is an optional tuning follow-up — NOT run here (this task is `rig_character.py`-only, zero Meshy credits). The procedural `cast` reads correctly for the baseline.

## Eyeball gate — PASS (all previews viewed)
- **Bind pose (front/side/¾ + 8-angle turntable):** robed hooded fireball-caster; gnarled staff with the red ember-crystal orb held in the RIGHT hand; glowing fireball cradled in the open LEFT palm; blue (placeholder) **TeamRegion mantle isolated on the shoulders/hood-cape** (the sole recolor region — hood + face spared, matching TASK-301); red singed hems; feet planted. Faithful to `Concepts/Wizard.png`. Clean bind deformation, no skinning explosion.
- **Idle:** gentle breathing/weight sway, staff + fireball held steady — reads.
- **Walk:** striding cycle, pelvis bob, arms counter-swing, props stay attached — reads (robe hem is envelope-stiff, the accepted roster tier; real polish is the optional Meshy lane).
- **Attack:** staff-arm winds back then drives up-and-forward with a forward torso lean — reads as a spellcast/hurl, decisively not a bow-draw.
- **Death:** stagger → rotate back about the feet → collapse onto back, arms fly out, holds final pose — reads.
- Staff (right hand) + fireball (left palm) stay rigidly attached to the hand bones through every clip; blue TeamRegion mantle stays isolated throughout.

---

## UE import recipe (turnkey — for the deferred editor-gated dispatch)
Serialized, EXCLUSIVE editor session. Do NOT delete+recreate any asset (preserve refs). Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" + "Wizard unit" skeletal clause + the M7.6 SK-unit LOD/URO law.

### 1. Skeletal mesh — `SK_Wizard`
- Import `Content/RawAssets/Characters/Wizard.fbx` → `/Game/Characters/SK_Wizard`.
- **Skeleton: BIND to the EXISTING `/Game/Characters/SK_Footman_Skeleton`** (the shared SiegeBiped — root bone `Footman_Rig`). Do NOT create a new skeleton. The FBX carries the exact 21-bone `Footman_Rig`-rooted contract, so the bind is clean (no missing-bones warning — the TASK-212 law).
- **Nanite OFF.** Import Normals (not compute) to preserve the baked N.
- **Material slots — assign EXACTLY, in order (same two-slot contract as `SM_Wizard`):** slot 0 `TeamRegion` → `MI_TeamColor_<Team>` (the BeginPlay recolor drives the cream mantle; import-time default `MI_TeamColor_Blue` per the fleet), slot 1 `WizardPBR` → `MI_Wizard_PBR`. FBX slot names are already `[TeamRegion, WizardPBR]` (verified), so they map 1:1. `MI_Wizard_PBR` is produced by the TASK-301 static-mesh import (Stage-3) — sequence this AFTER TASK-301's import so the MI exists; if not yet present, import it per TASK-301's recipe first.

### 2. Anim sequences — `A_Wizard_{Idle,Walk,Attack,Death}`
- Import the 4 FBXs from `Content/RawAssets/Characters/Anims/Wizard_*.fbx` → `/Game/Characters/Anims/A_Wizard_{Idle,Walk,Attack,Death}`, ALL against the SAME `SK_Footman_Skeleton`.
- **root-motion OFF + force_root_lock** (root travel is carried on the root bone by design — the lock neutralizes it; the fleet standard).
- **Functionally consumed by code** (SummonedUnit.cpp `CacheActionAnimations` → `PlayAnimation`, single-node): `A_Wizard_Attack` (played on attack tick) + `A_Wizard_Death` (played on death, holds final pose). `A_Wizard_Idle`/`A_Wizard_Walk` are imported for roster consistency + a future dedicated ABP, but in-match idle/walk is driven by the shared ABP's locomotion (see §3) — import them anyway to keep the roster complete.

### 3. AnimBlueprint — NONE authored (shared-ABP fallback, roster pattern)
- **Do NOT author `ABP_Wizard`.** `ASummonedUnit::ResolveSkeletalVisual` (SummonedUnit.cpp:309-333) resolves the AnimClass by: (1) prefer `/Game/Characters/ABP_<CardID>`, (2) ELSE fall back to the ONE shared `SharedLocomotionAbpPath = /Game/Characters/ABP_Footman.ABP_Footman_C`. Only `ABP_Footman` exists on disk; all 11 SK fleet units use this shared fallback. The Wizard shares `SK_Footman_Skeleton`, so `ABP_Footman`'s velocity-driven idle/walk drives it correctly with ZERO new asset. This IS the roster's ABP pattern (CONVENTIONS: "a shared ABP may back all humanoid units"). A dedicated `ABP_Wizard` is a future-only option; not needed now.

### 4. SK-LOD chain — apply the recipe `Content/RawAssets/Characters/Wizard.lod.json`
- Apply LOD1 **50% tris @ screenSize 0.4** / LOD2 **20% @ 0.15** to `SK_Wizard` via `SkeletalMeshEditorSubsystem.regenerate_lod` + per-LOD reduction (`number_of_triangles_percentage`, `screen_size`) — the same reduction contract `reimport_meshes.py::_apply_lods` uses for SMs (per TASK-288/289). Editor-python only (NOT MCP-reachable per TASK-289 blocker A) → run via a headless `-run=pythonscript` commandlet OR Jonathan's manual editor step.
- **Conflict-free:** the Wizard is a NEW unit (W-UNIT-1), NOT part of the M7.5 fleet — so applying its LOD chain at first-import has no binary-merge conflict (unlike the deferred fleet SK-LOD apply, TASK-289 blocker B).
- **URO: ALREADY LIVE in C++** — `EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations = true` are set on `SkeletalVisualMesh` in the `ASummonedUnit` constructor (SummonedUnit.cpp:155-156, TASK-285) for EVERY unit incl. the Wizard. The recipe's `component_defaults` are already satisfied — the importer only needs the LOD **chain**.

### 5. No BP / code / CSV work for the SK path
- `ASummonedUnit::ResolveSkeletalVisual` auto-composes `/Game/Characters/SK_Wizard` + the ABP from the `Wizard` CardID at BeginPlay — no CSV column, no code change (per the SkeletalMeshComponent swap contract).
- **Float-fix is automatic:** SummonedUnit.cpp:304-307 pins `SkeletalVisualMesh` to the static `VisualMesh`'s authored Z (`VisualMeshBaseRelativeLocation`) from code, so the Wizard SK grounds correctly with NO per-BP Z offset (the Archer/Ogre first-import float trap is permanently closed in code).

### 6. Acceptance checks (import-side)
- `SK_Wizard` LOD count **> 1** (readback after §4 — expect 3: LOD0/1/2).
- Bind to `SK_Footman_Skeleton` with **no missing-bones warning**.
- In PIE: the Wizard spawns as the animated skeletal mesh (idle/walk from shared ABP), plays `A_Wizard_Attack` on its attack tick and `A_Wizard_Death` on death; slot-0 team recolor tints the mantle Blue/Red.
- RateScale vs cards.csv (Wizard Cadence 1.6): `A_Wizard_Attack` is 40f @30fps ≈ 1.33 s — comfortably inside the 1.6 s cadence; no aggressive rate needed (unlike the Archer bow-shot). Death 48f ≈ 1.6 s vs the ~2.0 s destroy hold — fine.

## Downstream / sequencing
- Feeds **TASK-304** (build-master assemble: this import serializes behind TASK-301's static-mesh import — `MI_Wizard_PBR` must exist first — and the M7.7 / TASK-297 editor-priority ladder; never during Jonathan's PIE).
- New files this task (for build-master's commit set): `Content/RawAssets/Characters/Wizard.fbx`, `Content/RawAssets/Characters/Wizard.lod.json`, `Content/RawAssets/Characters/Anims/Wizard_{Idle,Walk,Attack,Death}.fbx`, + the `Tools/ArtPipeline/rig_manifest.json` edit. (`Cache/Wizard/rig/**` is gitignored/disposable.)
- **No editor / MCP / Blueprint / Git / deck-builder widget touched.** Staged and ready for the editor-gated import dispatch.
