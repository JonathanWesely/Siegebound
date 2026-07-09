# TASK-107 Handoff — BP_Building_CrystalTower (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-08
- status: complete — data-only child BP created, compiled clean (warnings-as-errors), saved not-dirty, every value readback-verified post-compile AND post-save. Zero graph edits (parent lineage carries all behavior). No Git, no C++ compile, no TASKBOARD.md edit (reported to orchestrator per board-write-race doctrine).
- editor: left UP. `IsPIERunning` → **false** before any mutation (checked first; no PIE started — TASK-109 owns PIE). Editor mutations limited to this ONE new asset: create + 2 property sets + compile + save. No modal blocked any call.

## What was built

One data-only Blueprint, exactly the TASK-035 BP_Building_ArrowTower pattern (parent verified by readback from the ArrowTower BP itself, not assumed):

| Item | Value | Verified how |
|------|-------|--------------|
| Asset | `/Game/Blueprints/Buildings/BP_Building_CrystalTower` | create returned the ref; exists on disk; saved |
| Parent | `ATower` (`/Script/GitClaudeUnrealTest.Tower`) — **identical to BP_Building_ArrowTower's parent** (BlueprintTools.get_parent on ArrowTower → `/Script/GitClaudeUnrealTest.Tower`; same call on the new BP → same) | readback both BPs |
| CDO `CardID` | `CrystalTower` (character-exact vs the DT_Cards row name) | post-compile get_properties |
| CDO `Team` | `Blue` (inherited C++ default, NOT overridden — TASK-035 deliberate non-action carried forward) | post-compile get_properties |
| `VisualMesh.StaticMesh` | `/Game/Meshes/SM_CrystalTower.SM_CrystalTower` | post-compile get_properties on the component CDO |
| `VisualMesh.OverrideMaterials` | **length-1 array**: `[0] = /Game/Materials/Instances/MI_TeamColor_Blue` — slot 0 ONLY | post-compile get_properties |
| Slot 1 (M_CrystalGlow) | **NOT overridden** — override array stops at index 0, so slot 1 falls through to the mesh-authored `/Game/Materials/M_CrystalGlow` (TASK-105 contract: crystals glow the same cyan for both teams; BeginPlay recolor law touches slot 0 only) | override array length + mesh StaticMaterials readback |
| `bCanEverAffectNavigation` | `true` (inherited from ABuilding C++, same as ArrowTower — no BP pin needed for towers) | component readback |
| `ChainBounceRadius` | `350` (inherited ATower C++ mechanic-rule default, M5 ruling 9 — not touched) | CDO readback |

**No stats typed into the BP.** No HP/damage/range/cadence/chain literals anywhere on the CDO — everything binds from the DT_Cards `CrystalTower` row (150 HP / 15 dmg / 800 range / 1.5 s / ChainTargets 3 / ChainFalloff 5, live since TASK-104) via `ABuilding` BeginPlay. Chain behavior needs no BP work: TASK-103's ATower code gates the instant chain zap on the row's `ChainTargets > 0` (Tower.h — "still NO new class"), so the ATower parent + CardID is the complete wiring.

## Compile / save state

- `compile_blueprint` with `warnings_as_errors: true` → returned null (clean, no errors/warnings).
- `save_assets` → true; `is_dirty` → **false** post-save. Every table value above was re-read AFTER the save (disk-backed CDO).

## Soft-class-path + ghost-path resolution checks (dead-card window CLOSED)

Code composes these strings (verified in source, not assumed):
- Spawn class: `/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C` — SiegePlayerController.cpp:1781 (player placement) and SiegeBotController.cpp:1014 (bot).
- Placement ghost mesh: `/Game/Meshes/SM_%s.SM_%s` — SiegePlayerController.cpp:2051.

Readbacks against the composed CrystalTower strings:
1. `/Game/Blueprints/Buildings/BP_Building_CrystalTower.BP_Building_CrystalTower_C` **loads** (ObjectTools resolved the generated class at exactly that path).
2. `search_subclasses(ATower, "CrystalTower")` returns exactly `BP_Building_CrystalTower_C` — registered as a live ATower subclass.
3. `/Game/Meshes/SM_CrystalTower.SM_CrystalTower` **loads** (AssetTools.load_asset) — the ghost resolves.
4. Mesh `StaticMaterials` readback: slot 0 = `MI_TeamColor_Blue` (slot name `MI_TeamColor_Blue`), slot 1 = `M_CrystalGlow` — matches the TASK-105 slot contract, so the slot-0-only override is aimed at the right slot.

## Notes for QA

- Verify on fresh load: parent `/Script/GitClaudeUnrealTest.Tower`, CardID `CrystalTower`, VisualMesh = SM_CrystalTower, OverrideMaterials length exactly 1 (index 0 = MI_TeamColor_Blue). A length-2 array with a null at [1] would break the crystal glow — it is length 1.
- Collision profile BlockAll / QueryAndPhysics and nav-affecting are inherited from ABuilding C++ (Building.cpp constructor), same as ArrowTower — deliberately not pinned on the BP.
- Runtime chain-fire demonstration is deliberately deferred to TASK-109 (dispatch constraint: no PIE between TASK-104 and this task closing the window; 109 owns the M5 exit-criteria PIE).
- New on disk for build-master to commit (rides TASK-109's batch): `Content/Blueprints/Buildings/BP_Building_CrystalTower.uasset` + this handoff. The UE Git provider may auto-stage the .uasset on save — left as-is.
