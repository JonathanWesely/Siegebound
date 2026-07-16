# TASK-160 Handoff — Rig SPIKE: skeletal rig/anim tooling stood up + proven on Footman (art)

- **From:** art-director
- **Date:** 2026-07-15
- **Status:** `ready-for-integration` — **GATED at TASK-161** (Jonathan eyeball). NO editor import here (that is TASK-162, blocked on TASK-161 approval + TASK-159 compiled via TASK-182). No Unreal editor touched; no Git.
- **Result:** the skeletal pipeline is stood up FROM SCRATCH and proven end-to-end on the Footman — game-ready `SM_Footman` → shared biped rig → bone-heat skin (0% unweighted) → Idle/Walk/Attack/Death → UE-importable `SK_Footman` FBX + 4 anim FBXs + a preview packet. This is the static-Ogre-proof equivalent for rigging.

## NEW TOOLING (extends `Tools/ArtPipeline/`, mirrors the refine-script conventions)
- **`Tools/ArtPipeline/rig_character.py`** — headless Blender rig/anim tool (sibling of `refine_trellis_glb.py`). Same structure: `--card-id` argparse, `OutputGuard` write-confinement (Cache/<CardID>/rig/ + Content/RawAssets/Characters/ only; hard-fails on CardArt), staged pipeline, `rig_report.json`. Runs HEADLESS (`blender.exe --background`) — NOT the 30 s MCP bridge.
  - Invocation: `"<blender.exe>" --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/rig_character.py -- --card-id Footman`
  - Stages: IMPORT (preserve two-slot + UVMap verbatim) → MEASURE adaptive anchors → build shared `SiegeBiped` armature fitted to the mesh → SKIN (bone-heat, deterministic segment-distance ENVELOPE fallback) → ANIMATE (Idle/Walk/Attack/Death) → EXPORT (rest mesh FBX + per-anim FBX, axis contract IDENTICAL to the static pipeline) → PREVIEW → REPORT.
- **`Tools/ArtPipeline/rig_manifest.json`** — per-asset rig params + the shared `SiegeBiped` skeleton spec (21 bones, UE-mannequin-style names, normalized proportions). Separate from `pipeline_manifest.json` so the rig tooling is self-contained and doesn't perturb the mesh manifest.

### Approach (records for the batch — TASK-163/164)
- **Shared skeleton `SiegeBiped` → UE `SKEL_SiegeBiped`.** 21 bones: `root`(non-deform) + `pelvis` + `spine_01/02/03` + `neck_01` + `head` + `[clavicle/upperarm/lowerarm/hand]_l|r` + `[thigh/calf/foot]_l|r`. Bone NAMES + hierarchy are the retarget contract — every humanoid unit gets the IDENTICAL skeleton so UE shares/retargets ONE skeleton and locomotion is authored once. Bone POSITIONS are computed per-asset from the MEASURED mesh (adaptive height + per-region half-widths) × normalized proportions, so the same spec fits a 149-tall militiaman and a 208-tall cavalry rider. `_l` = +X (character LEFT), `_r` = −X (the −X weapon side).
- **Skinning = Blender bone-heat automatic weights** (`ARMATURE_AUTO`) with a deterministic **segment-distance envelope fallback** if heat fails or leaves >2% unweighted. On the Footman heat gave **0.0% unweighted** (watertight voxel-remeshed mesh → clean heat solve). The fallback guarantees the batch never hard-fails on a bad mesh.
- **Animation** authored as pose-bone keyframes computed in WORLD axes (via each bone's rest matrix), so motion is orientation-correct regardless of bone roll. Per-card knobs: `weapon_side` (r for Footman = −X spear arm) + `attack_style` (thrust|swing|overhead|cast|mine).
- **Export axis contract = the static pipeline's** (`axis_forward='-Z', axis_up='Y', apply_unit_scale`, `add_leaf_bones=False`) so `SK_Footman` stands + faces IDENTICALLY to `SM_Footman` (required for the swap).

### Two Blender-5.1 gotchas handled (record for the batch)
1. **Slotted Actions (Blender 4.4+):** `action.fcurves` is gone (use layers→strips→channelbags); and assigning `animation_data.action` does NOT bind the channels — you MUST set `animation_data.action_slot` or the pose renders/**bakes as REST**. `rig_character.py.bind_action()` handles it; without it the anim FBXs would silently be static. (This bit twice during the spike; the fix is in-tool.)
2. **This Blender build has NO FFMPEG/GIF output** — the image-format enum has no movie formats. Preview CLIPS are therefore delivered as per-anim **PNG frame SEQUENCES** + horizontal **contact strips**. Real playback is judged in-editor at TASK-162. (If Jonathan wants mp4s pre-gate, a machine with ffmpeg can stitch `previews/seq/<anim>/frame_*.png` trivially.)

## FOOTMAN DELIVERABLES (all verified by headless round-trip re-import)
### Raw rigged FBX → `Content/RawAssets/Characters/` (raw-asset rule; for the TASK-162 editor import)
| File | Contents (re-import verified) |
|---|---|
| `Footman.fbx` | armature `Footman_Rig` (21 bones), mesh **SK_Footman** 15000 tris, **20 vertex groups** + armature modifier, slots **[TeamRegion, FootmanPBR]**, **UVMap**, feet-center. This is `SK_Footman`. |
| `Anims/Footman_Idle.fbx` | baked action, 219 fcurves, frames 1–61 (loop) |
| `Anims/Footman_Walk.fbx` | baked action, 219 fcurves, frames 1–31 (loop) |
| `Anims/Footman_Attack.fbx` | baked action, 219 fcurves, frames 1–41 (probe: weapon arm moves 15.7 cm) |
| `Anims/Footman_Death.fbx` | baked action, 219 fcurves, frames 1–49 |

Anim ranges match the report: Idle 60f / Walk 30f / Attack 40f / Death 48f @ 30 fps. The FBX "take" imports as `Footman_Rig|Scene` — cosmetic; TASK-162 names the sequences `A_Footman_Idle/Walk/Attack/Death` and the montage `AM_Footman_Attack` (montage wraps the Attack sequence — NOT a Blender export).

### Two-slot material contract PRESERVED
`SK_Footman` keeps `[0] TeamRegion, [1] FootmanPBR]` in order + `UVMap` (verbatim from `SM_Footman`), so the BeginPlay slot-0 team recolor and `MI_Footman_PBR` reassignment work identically on the skeletal path at TASK-162.

## ⬛ EYEBALL-GATE PACKET FOR JONATHAN (TASK-161) — what to judge
All in `Tools/ArtPipeline/Cache/Footman/rig/previews/` (Cache is gitignored — these are review artifacts, not committed):
- **Orientation + silhouette + skin at rest:** `bind_threequarter.png`, `bind_front.png`, `bind_side.png`, and `turntable_strip.png` (8 angles). Footman stands upright, feet planted, helmet + shoulders carry the blue TeamRegion (correct Footman recipe), spear in the −X hand + round shield on the other arm. Silhouette reads clean all the way around.
- **The animations (contact strips = 7 poses L→R across the loop):** `contact_idle.png` (subtle breathing/sway), `contact_walk.png` (alternating stride, arm counter-swing, pelvis bob, knees bend), `contact_attack.png` (spear wind-up → up-and-forward strike → recover), `contact_death.png` (stagger/recoil → tip back → collapse to prone, knees buckle).
- **Full clip frames (scrub for smoothness):** `previews/seq/{idle,walk,attack,death}/frame_####.png`.
- **Report:** `Tools/ArtPipeline/Cache/Footman/rig/rig_report.json` — skinning 0.0% unweighted, no warnings.

**Judge:** (a) ORIENTATION — does SK_Footman face/stand right (it matches SM_Footman's −Y front)? (b) SILHOUETTE at the §6 bar? (c) SKIN WEIGHTS — do limbs deform without tearing at the extremes (walk stride / attack thrust / death buckle)? (d) ATTACK — reads as a spear strike? **Note:** the diffuse is the same dark/moody TRELLIS read as the Ogre (Stage-1 characteristic, not a rig issue). The attack currently reads as a raise-and-drive spear strike; the exact feel is tunable per-card via `attack_style` — flag if you want a flatter forward thrust.

**Gate outcomes:** APPROVE → unblocks TASK-162 (editor import + ABP) AND the batch (TASK-163/164). TUNE → I iterate `rig_character.py`/`rig_manifest.json` (free headless re-run, no HF quota — rigging is 100% local) and re-post. NEVER batch-rig on an un-approved spike.

## Coordination with TASK-159 (SkeletalVisualMesh swap path — gameplay-programmer)
Assets are authored to FIT that contract (I did NOT edit gameplay code): TASK-162 imports `SK_Footman` → `/Game/Characters/SK_Footman` and `ABP_Footman` → `/Game/Characters/ABP_Footman`, which the TASK-159 code composes by CardID (`/Game/Characters/SK_<CardID>` + `/Game/Characters/ABP_<CardID>`). The static `SM_Footman` stays at `/Game/Meshes/SM_Footman` (unchanged) to back the placement ghost + null-safe fallback — I did not touch it.

## Commit manifest (for build-master TASK-183 — NO Git in my lane)
- NEW code/tooling: `Tools/ArtPipeline/rig_character.py`, `Tools/ArtPipeline/rig_manifest.json` (both are CODE → full QA gate applies before the TASK-183 commit, per the tooling law).
- NEW raw assets: `Content/RawAssets/Characters/Footman.fbx` + `Content/RawAssets/Characters/Anims/Footman_{Idle,Walk,Attack,Death}.fbx` (raw-asset rule; committed alongside the future .uasset).
- `Cache/Footman/rig/**` is gitignored — do NOT commit (review artifacts only).
- Lane isolation honored: no CardArt paths, no /Game/ writes, no level/BP/C++ edits, `pipeline_manifest.json` untouched by the rig work (rig uses `rig_manifest.json`).
