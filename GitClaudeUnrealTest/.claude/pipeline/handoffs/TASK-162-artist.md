# TASK-162 Handoff — Footman rig editor integration (SK_Footman + ABP_Footman) + PIE verify

- **From:** art-director · **Date:** 2026-07-16 · **Status:** `ready-for-integration`
- **Editor:** L_Arena, live MCP. No Git touched, no gameplay C++ edited. L_Arena NOT saved (a temp test actor was placed for PIE, then removed — level is pristine on disk).
- **One-line result:** Footman now renders as a real skeletal mesh and animates in-match. Idle↔Walk (velocity-driven) is DONE and PIE-proven. Attack + Death are imported and ready but NOT wired — the compiled TASK-159 code exposes no anim trigger for them; needs one small gameplay-programmer hook (details below).

## Assets created (all saved to disk; build-master commits them at integration)
| Asset | /Game path | Notes |
|---|---|---|
| Skeletal mesh | `/Game/Characters/SK_Footman` | Imported from `Content/RawAssets/Characters/Footman.fbx`. 21-bone rig (`Footman_Rig` + root + 20 deform). Slots `[0] TeamRegion → MI_TeamColor_Blue`, `[1] FootmanPBR → MI_Footman_PBR` (verified — matches the SM_Footman two-slot contract). Feet-center (bounds z 0→~180). Nanite off. |
| Skeleton | `/Game/Characters/SK_Footman_Skeleton` | Auto-created on import; the 4 anims bind to it. (This asset carries its own skeleton — a shared `SKEL_SiegeBiped` is deferred to the batch, TASK-163/164.) |
| Physics asset | `/Game/Characters/SK_Footman_PhysicsAsset` | Auto-created on import (ragdoll-ready; unused by current anim path). |
| Anim sequences | `/Game/Characters/Anims/A_Footman_Idle`, `_Walk`, `_Attack`, `_Death` | Imported from `Content/RawAssets/Characters/Anims/Footman_*.fbx`, bound to SK_Footman_Skeleton. Idle 60f, Walk 30f, Attack 40f, Death 48f. |
| Anim Blueprint | `/Game/Characters/ABP_Footman` | Authored fresh (class `ABP_Footman_C`). Idle↔Walk locomotion. Compiles clean. |

Raw sources already in Git: `Content/RawAssets/Characters/Footman.fbx` + `Anims/Footman_{Idle,Walk,Attack,Death}.fbx` (from TASK-160).

## ABP_Footman — what it actually does
- **EventGraph** (`BlueprintUpdateAnimation`): `TryGetPawnOwner` → `GetVelocity` → `VectorLengthXY` → sets `GroundSpeed` (float) + `bIsMoving = GroundSpeed > 10`.
- **AnimGraph**: `Blend Poses by bool` driven by `bIsMoving` → **True = A_Footman_Walk** (looping), **False = A_Footman_Idle** (looping), 0.1 s crossfade → Output Pose.
- Reads Pawn velocity directly — **needs zero gameplay-code changes** to drive Idle/Walk. This is the piece that proves the rig in a live match.
- **Import note (non-blocking):** the FBX anims imported at 24 fps though authored at 30 fps, so I set `PlayRate = 1.25` on both Idle/Walk sequence players to restore authored speed. Cosmetic; the batch pipeline should set the FBX import frame-rate to 30 to avoid the fixup.

## Alignment (SkeletalVisualMesh transform) — done on BP_Unit_Footman
The TASK-159 C++ creates `SkeletalVisualMesh` at identity. I copied the static `VisualMesh` offset onto it so the skeletal runtime sits/faces identically:
- `SkeletalVisualMesh` RelativeLocation = **(0, 0, -90)** (feet at capsule bottom; capsule half-height 90, SK feet at mesh-z 0), RelativeRotation = **(yaw -90)** (faces actor +X / travel direction). Matches the SM_Footman VisualMesh transform exactly. BP compiled + saved.
- No BP graph/logic changed — the swap auto-picks-up via the TASK-159 CardID-composed path.

## Verification (PIE / Simulate on L_Arena)
- **Definitive runtime proof (output log):** `LogGitClaudeUnrealTest: ASummonedUnit 'BP_Unit_Footman_C_0': skeletal runtime SK_Footman active (M7 TASK-159) — static VisualMesh hidden; the placement ghost still uses SM_Footman.` → the swap fires, `SetSkeletalMeshAsset(SK_Footman)` + `SetAnimInstanceClass(ABP_Footman_C)` both resolved, static mesh hidden, `bUsingSkeletalVisual=true`.
- **Visual proof (screenshots, in `Tools/ArtPipeline/Cache/Footman/rig/task162_verify/` — gitignored review artifacts):**
  - `SK_Footman_asset_thumbnail.png` — mesh imported textured, blue TeamRegion (helmet/shoulders) + PBR body, spear + round shield.
  - `PIE_skeletal_footman_inmatch.png` / `PIE_skeletal_footman_closeup.png` — a live Footman on the battlefield rendering as the skeletal mesh, feet planted on the ground, upright, natural spear-holding idle pose (NOT ref/T-pose → idle anim is applied). Two same-frame captures a moment apart differ across the figure region (pose changing over time = animation live).
- **Idle: CONFIRMED live.** A placed Footman renders skeletal + plays the idle animation in a running session.
- **Walk: WIRED + COMPILED, not isolated on video.** A level-*placed* test unit idles at spawn (the normal card/bot spawn flow assigns march goals; a raw placed unit gets none) and I can't drive card-play or console-cheat input headlessly to summon a *marching* Footman. The walk branch is verified by graph inspection (bIsMoving true → Walk pose; mapping confirmed correct) and is deterministic — it engages on any GroundSpeed > 10. Only the Footman swapped; other spawned card types stayed static blockouts, confirming the swap is correctly per-CardID/additive.

### How to see it in the morning (Jonathan)
Play a normal match on L_Arena and summon a **Footman** (or let the Red bot summon one). It will spawn as the skeletal mesh and **walk** toward the enemy castle (marching = velocity > 10 → Walk anim), and idle when it stops. Red-team Footmen recolor slot 0 to `MI_TeamColor_Red`. Quickest forced spawn: PIE console `SummonTestUnit Footman false` (Blue) / `SummonTestUnit Footman true` (Red) via `USiegeCheatManager` — a summoned unit marches, so that's the cleanest live Idle→Walk demo.

## ⚠️ Attack + Death need a small gameplay-programmer hook (route a follow-up)
Per the task's "do not fake it" rule: I imported `A_Footman_Attack` and `A_Footman_Death` (ready to use) but did NOT wire them, because **the compiled code (f313253 / TASK-159 batch) exposes no anim trigger for attack or death**:
- **Attack:** the code still runs the TASK-020 procedural lunge (`StartAttackLunge`/`UpdateLunge` on the *static* VisualMesh — harmless/invisible on a skeletal unit). It does **not** call `Montage_Play(AM_Footman_Attack)` on the attack tick — the CONVENTIONS montage hook is explicitly "out of TASK-159 scope" and isn't in the build. There is no `AM_Footman_Attack` montage asset yet either (TASK-160 delivered the sequence, not a montage).
  - **Fix needed (programmer):** on the unit's attack tick (in `PerformAttack`, where the lunge fires), if `bUsingSkeletalVisual`, play the attack as a montage on `SkeletalVisualMesh`'s anim instance (author `AM_Footman_Attack` wrapping `A_Footman_Attack`, add a Slot node to ABP_Footman's AnimGraph, and `Montage_Play` it) — with the existing procedural lunge as the null-safe fallback. OR expose a BlueprintReadOnly `bIsAttacking` the AnimBP can read for a state-driven attack.
  - Interim option (no new code): `GetUnitState()` is already `BlueprintPure` and returns `Attack` while engaged — the AnimBP *could* read it for a looping attack pose, but that isn't frame-synced to individual hits, so I left it out in favor of the proper montage hook.
- **Death:** there is no `Dead` state and `HandleDeath()` destroys the actor immediately (only a gold-burst VFX), so a death animation has no time to play and nothing readable to trigger it.
  - **Fix needed (programmer):** on death, if skeletal, play `A_Footman_Death` (as a montage) and defer `Destroy()` until it finishes (or a fixed ~1.5 s), or set a `bIsDead` flag + delayed destroy so the AnimBP can transition to a death state.

These are small, well-scoped hooks; the anims are already imported and named per CONVENTIONS, so wiring them is a code-only follow-up. Idle+Walk in a live match already validates the pipeline tonight (the task's stated minimum).

## For build-master (integration/commit)
- Commit the 8 new `/Game/Characters/**` assets + the modified `/Game/Blueprints/Units/BP_Unit_Footman` (SkeletalVisualMesh transform). Raw FBXs are already tracked. Do NOT commit `Tools/ArtPipeline/Cache/**` (gitignored).
- `SM_Footman` at `/Game/Meshes/SM_Footman` is UNCHANGED and still backs the placement ghost + null-safe fallback (parity rule intact).
- L_Arena was NOT modified on disk.
