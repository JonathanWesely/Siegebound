# TASK-035 Handoff — BP_Building_ArrowTower / BP_Building_Wall (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-04
- status: complete — 2 building BPs created in new folder `/Game/Blueprints/Buildings/`, compiled clean (warnings-as-errors), saved. Reparent + DT_Cards stat + collision + nav all verified; tower-fire demonstrated in Simulate. No Git (TASK-040 commits). TASKBOARD.md NOT edited (reported to orchestrator).
- editor: left UP (PID 35508, aafd968 DLL — never booted/closed). MCP stable throughout: BP create/compile, Object get/set properties, Actor/Scene ops, PIE Simulate, Log reads, Asset saves — zero hiccups. **No UMG ops were performed** (the op class that previously killed the MCP socket was avoided entirely).

## What was built

Two data-only child BPs. Parents are the aafd968-compiled C++ classes; nothing stat-like set on either BP (all HP/range/cadence/damage from `/Game/Data/DT_Cards` at BeginPlay).

| BP | Parent (C++) | CardID | VisualMesh (slot0 mesh) | Slot 0 material | Collision profile | bCanEverAffectNavigation |
|----|--------------|--------|--------------------------|-----------------|-------------------|--------------------------|
| `/Game/Blueprints/Buildings/BP_Building_ArrowTower` | `ATower` (`/Script/GitClaudeUnrealTest.Tower`) | `ArrowTower` | `/Game/Meshes/SM_ArrowTower` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `true` |
| `/Game/Blueprints/Buildings/BP_Building_Wall` | `ABuilding` (`/Script/GitClaudeUnrealTest.Building`) | `Wall` | `/Game/Meshes/SM_Wall` | `MI_TeamColor_Blue` | `BlockAll` (blocks Pawns) | `true` (explicitly pinned on the BP) |

Component addressing: each BP CDO has exactly the single native root `VisualMesh` (from `ABuilding`) — no extra components. Mesh + material set on that component; CardID set on the actor CDO.

### Collision / navigation (§3.7) — how the requirement is met
- The **base `ABuilding` C++ constructor** (Building.cpp lines 34/42) already does `SetCollisionProfileName(BlockAll)` and `SetCanEverAffectNavigation(true)`, so BOTH BPs inherit Pawn-blocking and nav-affecting from C++.
- Per the task's explicit carry-forward (TASK-038 flag), I **additionally set `bCanEverAffectNavigation = true` on `BP_Building_Wall`'s VisualMesh CDO** so the intent is pinned in the asset itself. Verified reads `true` on the saved CDO. The tower inherits `true` too (its 250×250 UCX base is tight, so no placement dead zone — carving under a tight footprint is expected/fine).
- `collisionEnabled = QueryAndPhysics`, `objectType = ECC_WorldStatic`, `collisionProfileName = BlockAll` on both → units/hero physically stop and reach-tests (hero melee / unit attacks / projectiles, all query ECC_Pawn) hit the buildings.

### Deliberate non-actions (QA note)
- **Team** left at the inherited C++ default `Blue` (get_properties reads `Blue`), no explicit override. The task `names:` block does not list Team; player placements set Team via `InitBuilding(Blue,…)` at spawn (TASK-030). Level-placed enemy instances would set Team per-instance.
- **No yaw / Z offset** on the building VisualMesh. Buildings use ground-center-origin meshes (TASK-038) and VisualMesh IS the root, so the default relative transform already puts the mesh base at the actor origin. This is unlike the unit BPs (which need -90 yaw + Z=-HalfHeight because their humanoid mesh is a child of a capsule root). Placement rotation for walls is applied at spawn by the placement controller, so BP default rotation 0 is correct.

## Compile
Both `compile_blueprint` calls ran with `warnings_as_errors: true` → no error raised (returnValue null). CDO/component overrides persisted (re-read post-compile; every value in the table above verified on the disk-backed CDO).

## Runtime verification (Simulate-In-Editor, L_Arena)

Method: placed one instance of each BP (plus one Red enemy Footman near the tower) in editor L_Arena, `StartPIE(bSimulate=true, warmup=5s)`, read the PIE-world (`UEDPIE_0_L_Arena`) instances, `StopPIE`, removed the 3 temp editor actors. **L_Arena left UNSAVED / content-identical** (add+remove pair; discard if ever prompted).

Stats resolved from DT_Cards (nothing hardcoded):

| Instance | CardID/Team | MaxHP/CurrentHP | AttackDamage / Range / Cadence | Spec target | Result |
|----------|-------------|-----------------|--------------------------------|-------------|--------|
| ArrowTower | ArrowTower / Blue | 150 / (42 after combat) | 15 / 900 / 1.5 | 150 HP, 15 dmg, 900 range, 1.5 cad | **PASS** |
| Wall | Wall / Blue | 300 / 300 | — (no attack stats; base building, tickless) | 300 HP | **PASS** |

- **Both spawned without error.** Wall bound 300 HP and stayed 300/300 (no attacker reached it, and the tower correctly ignored it — no friendly fire, tower excludes buildings from targeting).
- **Tower-fire check (bonus, feasible):** a Red-team `BP_Unit_Footman` was placed ~450 units from the Blue tower (well inside 900). During the sim the tower fired projectiles at the nearest enemy and **killed the Footman** — its PIE instance became invalid ("not valid Object" on read), and the tower's projectiles are the ONLY damage source to a Red unit in the scene (the Blue castle does not auto-fire; walls don't fire). The tower itself dropped to 42/150 from the Footman's melee, then stabilized once the Footman died. This exercises the full §3.7 tower loop: nearest-enemy acquisition inside 900 → projectile fire at 15 dmg → kill; and no friendly fire (Blue wall untouched).
- Note: the world keeps ticking during inspection MCP calls, so more than 5 s of sim elapsed by read time (that's why HP moved past a single cadence's worth) — expected, not an anomaly.

## Deferred to TASK-040 (full integrated PIE)
The board's full acceptance needs systems that land at TASK-036/040:
- **Wall reroutes a marching Footman** via the dynamic navmesh carve, and **wall dies at exactly 300 damage** — needs a navmesh rebuild under `RuntimeGeneration=Dynamic` + a pathing unit marching castle-to-castle. This task delivered the BP-side prerequisite (`bCanEverAffectNavigation=true` on the wall mesh, verified); the actual reroute is observable only once TASK-036/040 assemble waves + rebuild nav.
- **Tower "ignores an enemy at 1000"** boundary and the sustained 1.5 s cadence over a full wave — needs proper enemy waves. Demonstrated here only that acquisition/fire/kill works inside range.

## Scope touched
Created `/Game/Blueprints/Buildings/` (new folder) + `BP_Building_ArrowTower.uasset` + `BP_Building_Wall.uasset` (both saved to disk, clean/not-dirty) and this handoff. No C++ compiled, no Git, no TASKBOARD edit, no donor/L_Arena re-save. The 2 new `.uasset` are on disk (untracked / may be auto-staged by the UE Git provider) for TASK-040 to commit.

## Notes for QA / build-master
- Verify on a fresh editor load that both CDOs still read: parent (`ATower`/`ABuilding`), CardID (`ArrowTower`/`Wall`), VisualMesh mesh + slot0 `MI_TeamColor_Blue`, `collisionProfileName=BlockAll`, and `bCanEverAffectNavigation=true` (esp. the Wall — §3.7 nav carve).
- Benign log noise seen this session, unrelated to these BPs: (a) a stale match-end freeze from a PRIOR PIE session (timestamp 09.47, before these BPs existed); (b) one `LogScript: ... GetObjectProperties on BP_Unit_Footman ... could not be read: CollisionCylinder` from my CDO discovery read — harmless. The `Missing RowStruct` log (closed) did not appear.
- `bCanEverAffectNavigation=true` is set both in C++ (base) and pinned on the Wall BP; if a future template/base change flips the default, the Wall BP override keeps §3.7 honest.
