# TASK-010 Handoff — BP_Unit_Footman (editor)

- author: gameplay-programmer
- date: 2026-07-02
- status: complete — BP created, compiled clean, saved; PIE-simulate verified (no Git per task constraints)

## Asset

- `/Game/Blueprints/Units/BP_Unit_Footman` (on disk: `Content/Blueprints/Units/BP_Unit_Footman.uasset`, saved, not dirty)
- Parent class: `/Script/GitClaudeUnrealTest.SummonedUnit` (verified via asset registry `NativeParentClass` and BlueprintTools get_parent)
- Data-only Blueprint (`IsDataOnly = True`) — no graphs added, no components added, defaults only
- Compiled with warnings-as-errors: clean (no errors, no warnings; zero LogGitClaudeUnrealTest entries during the session)

## Defaults set (CDO, verified by readback after compile + save)

| Where | Property | Value |
|---|---|---|
| CDO | CardID | `Footman` |
| CDO | Team | `Blue` (C++ default, set explicitly) |
| CollisionCylinder (capsule root) | CapsuleHalfHeight | 90.0 |
| CollisionCylinder | CapsuleRadius | 40.0 |
| VisualMesh | StaticMesh | `/Game/Meshes/SM_Footman` |
| VisualMesh | OverrideMaterials[0] | `/Game/Materials/Instances/MI_TeamColor_Blue` |
| VisualMesh | RelativeLocation | (0, 0, **-90**) — feet-center origin mesh dropped to capsule bottom |
| VisualMesh | RelativeRotation | yaw **-90** per TASK-014 facing note (mesh faces actor +X) |

- VisualMesh collision left at the C++ NoCollision profile — NOT touched (TASK-004 contract).
- **Nothing stat-like set** — MaxHP/CurrentHP/AttackDamage/AttackRange/AttackCadence/State are all Transient and read 0/Idle on the editor instance; they bind from DT_Cards only at BeginPlay (§3.0 verified below).

## PIE verification (L_Arena, Simulate-In-Editor)

Method: temporarily placed one instance at (-1000, 0, 92) in the editor level, ran **Simulate** (no player pawn needed; AI ticks), queried the PIE-world instance (`UEDPIE_0_L_Arena`), stopped PIE, deleted the temp actor. **L_Arena was NOT saved** — the add/remove pair leaves the map file byte-identical on disk; if the editor later prompts to save L_Arena, discarding is safe and correct.

Runtime values on the PIE instance (all resolved from `/Game/Data/DT_Cards` row `Footman`, nothing hardcoded):

| Check | Result |
|---|---|
| GetMaxHP / GetCurrentHP backing fields | **80 / 80** |
| CharacterMovement MaxWalkSpeed | **400** |
| AttackDamage / AttackRange / AttackCadence | 12 / 120 / 1.0 |
| CardID / Team | Footman / Blue |
| AIController possession | `AIController_0` present in PIE world (only pawn in level; AutoPossessAI worked for a level-placed unit) |
| Data-table load errors | none — zero LogGitClaudeUnrealTest log entries |
| Visual (viewport capture) | blue-tinted mesh, feet on ground, faces +X (sword in right hand) — the -90 VisualMesh yaw is correct |

## No-enemy-castle behavior (RECORD for integration)

L_Arena currently contains **no ACastle actors** (only the two TargetPoint anchors), so this exercised TASK-004's no-castle path:

- The unit binds stats normally, then `UpdateState` finds no target and no standing enemy castle and calls `EnterIdle()` — **State = `Idle`, velocity zero, position unchanged over several seconds of simulation**.
- This is **silent by design**: no log line fires for the no-castle case (TASK-004 comment: "no target and no standing enemy castle (destroyed → the match is over): stand down"). Absence of warnings/errors is the healthy signature, not a fault.
- The state machine keeps re-checking every 0.25 s, so **the moment Castle_Red is placed at integration the unit will acquire it and advance** — no respawn or re-init needed. Integration should expect placed/spawned Footmen to stand still until both castles exist.
- Attack-on-castle (12 dmg / 1.0 s) could NOT be observed — there is no castle to hit until integration places Castle_Blue/Castle_Red per the M1 assembly note. That part of the TASK-010 acceptance transfers to the integration check.

## Notes / minor observations

1. Character spawned 2 units above ground rested at Z=92 (capsule half-height 90 + CMC floor-snap tolerance ~2). Cosmetically the unit still reads as standing on the floor; TASK-007 spawns at trace-hit + half-height so this is a non-issue in the real flow.
2. The mesh's 147-unit X extent (sword/shield) overhangs the radius-40 capsule by design — collision is torso-only, matching the hero/mannequin convention.
3. Team default Blue is inherited from C++ and also stamped on the BP CDO; TASK-007's `InitUnit(Team, "Footman")` remains the authoritative setter for spawned units per the TASK-004 contract.

## Scope touched

Created `/Game/Blueprints/Units/` + `BP_Unit_Footman` and this handoff file only. L_Arena opened for PIE but left unsaved and content-identical (temp actor removed). No other assets, code, config, or TASKBOARD edits.
