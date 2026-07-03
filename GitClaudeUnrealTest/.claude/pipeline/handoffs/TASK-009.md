# TASK-009 Handoff — Input assets + BP_HeroCharacter (editor)

- author: gameplay-programmer
- date: 2026-07-02
- status: complete, all assets created via Unreal MCP, saved to disk (no Git per constraints)

## Assets created (all new; no existing asset modified)

| Asset | Type | Notes |
|-------|------|-------|
| `/Game/Input/Actions/IA_Sprint` | InputAction | Boolean, no triggers/modifiers |
| `/Game/Input/Actions/IA_Attack` | InputAction | Boolean, no triggers/modifiers |
| `/Game/Input/Actions/IA_Card1` | InputAction | Boolean, no triggers/modifiers |
| `/Game/Input/Actions/IA_CancelPlace` | InputAction | Boolean, no triggers/modifiers |
| `/Game/Input/IMC_Hero` | InputMappingContext | 11 mappings, see below |
| `/Game/Blueprints/BP_HeroCharacter` | Blueprint (parent `AHeroCharacter`) | compiled clean with warnings-as-errors |

Verified NOT dirtied/modified: IMC_Default, IMC_MouseLook, IA_Move, IA_Look, IA_Jump, IA_MouseLook, BP_ThirdPersonCharacter.

## IMC_Hero key mappings

UE 5.8 stores mappings in `DefaultKeyMappings.Mappings` (not the legacy `Mappings` array — that is empty in IMC_Default too; QA should not flag it).

| Action | Key | Modifiers |
|--------|-----|-----------|
| IA_Jump | SpaceBar | — |
| IA_Move | W | SwizzleAxis (Order YXZ) |
| IA_Move | S | SwizzleAxis (YXZ) + Negate (X,Y,Z) |
| IA_Move | A | Negate (X,Y,Z) |
| IA_Move | D | — |
| IA_Look | Mouse2D | Negate (Y only) |
| IA_Sprint | LeftShift | — |
| IA_Attack | LeftMouseButton | — |
| IA_Card1 | One | — |
| IA_CancelPlace | RightMouseButton | — |
| IA_CancelPlace | Escape | — |

Move/mouse modifier setup replicated 1:1 from the template (IMC_Default's WASD block; IMC_MouseLook's Mouse2D Negate-Y). Gamepad/arrow-key mappings were NOT copied — spec asked for WASD/mouse/space only.

## BP_HeroCharacter property assignments (all verified non-null after compile)

| UPROPERTY slot | Assigned |
|----------------|----------|
| HeroMappingContext | /Game/Input/IMC_Hero |
| SprintAction | /Game/Input/Actions/IA_Sprint |
| AttackAction | /Game/Input/Actions/IA_Attack |
| JumpAction (inherited) | /Game/Input/Actions/IA_Jump |
| MoveAction (inherited) | /Game/Input/Actions/IA_Move |
| LookAction (inherited) | /Game/Input/Actions/IA_Look |
| MouseLookAction (inherited) | /Game/Input/Actions/IA_MouseLook |

Mesh (CharacterMesh0), copied exactly from BP_ThirdPersonCharacter: SkeletalMeshAsset = `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`, AnimClass = `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C`, RelativeLocation (0, 0, -89), RelativeRotation (pitch 0, yaw -90, roll 0).

C++ defaults confirmed on the CDO: Team = Blue, WalkSpeed = 500, SprintSpeed = 750. HeroMappingContext is added ONLY by AHeroCharacter::NotifyControllerChanged at priority 1 (per TASK-003) — the BP adds no context anywhere else.

## Design decision QA should review

**Mouse2D is mapped to IA_Look (not IA_MouseLook), and IA_MouseLook is NOT mapped in IMC_Hero.** Rationale: the template binds BOTH LookAction and MouseLookAction to the same Look handler; mapping Mouse2D to both actions would double-apply camera rotation. Spec says "IA_Look (mouse XY)", so IA_Look carries the mouse (with the template's Negate-Y so mouse-up looks up). MouseLookAction = IA_MouseLook mirrors the template BP's slot assignment (per the taskboard handoff-note) and is harmless — it simply never fires under IMC_Hero.

## For TASK-011 / integration

1. **IA_Card1 and IA_CancelPlace exist and are already key-mapped in IMC_Hero**, but no controller UPROPERTY references them yet — `handoffs/TASK-007.md` was not written at the time of this task (TASK-007 in-progress). Wiring ASiegePlayerController's slots to these two assets happens at TASK-011/integration per the plan.
2. **Do not let the shipping setup add IMC_MouseLook or IMC_Default alongside IMC_Hero** (the template AGitClaudeUnrealTestPlayerController adds DefaultMappingContexts at priority 0). IMC_Hero already covers move/look/jump; adding IMC_MouseLook too would double-fire mouse look (IA_MouseLook + IA_Look both hitting the Look handler). ASiegePlayerController should leave the template's context arrays empty.
3. PIE smoke test was skipped: L_Arena's game mode is still the template default (DefaultPawnClass = BP_ThirdPersonCharacter), so the hero would not spawn without editing world settings — outside this task's touch-scope. The 500/750 walk/sprint check falls to TASK-006/integration when SiegeGameMode sets DefaultPawnClass = BP_HeroCharacter.
4. All six assets saved to disk (`Content/Input/Actions/IA_*.uasset`, `Content/Input/IMC_Hero.uasset`, `Content/Blueprints/BP_HeroCharacter.uasset`) — ready for build-master to commit after QA.
