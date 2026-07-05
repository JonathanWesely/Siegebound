# TASK-070 — L_Arena stray-actor cleanup (editor/MCP work) — handoff

**Date:** 2026-07-04
**Assignee:** gameplay-programmer
**Result:** DONE — L_Arena cleaned, saved, and PIE-verified clean-of-strays. Ready for QA / build-master to commit the single `L_Arena.umap` change.
**Editor:** left UP (PID 19464), MCP healthy, PIE stopped, current level back on `/Game/Maps/L_Arena` for TASK-041.

## What changed
Removed **3 stray M2 verification actors** (`TM040_*` labels — placed during TASK-040 assembly and committed into the map ever since; root-caused in `handoffs/TASK-069.md`) from the persistent level `/Game/Maps/L_Arena`. This is an editor/MCP-only change: **one asset touched — `Content/Maps/L_Arena.umap`.** No code, no compile, no Git, no TASKBOARD edit. No other asset re-saved.

## Actors removed (confirmed by class + label + transform before deletion)
| Outliner name | Label | World location | Why it was a stray |
|---|---|---|---|
| `BP_Unit_Footman_C_1` | `TM040_RedTarget` | (-150, 700, 100) | The "transient Blue unit" — a Blue Footman on the bot's half; at match start it advanced and tripped the bot's Rule 1 (Defend) at t≈2s. |
| `BP_Unit_Miner_C_2` | `TM040_Miner` | (-950, 0, 100) | Stray Blue economy unit on the Blue half. |
| `BP_Building_ArrowTower_C_1` | `TM040_Tower` | (-600, 700, 0) | Stray Blue ArrowTower on the Blue half. |

All three carried the `TM040_` prefix and sat on the Blue (X-negative) half — unambiguous verification residue, exact-name matches to the task spec (no suffix variance). `remove_from_scene` returned `true` for all three.

## Intended actors confirmed STILL PRESENT (27 actors post-cleanup, none disturbed)
- **Castles:** `Castle_0` = label **Castle_Blue**, `Castle_1` = label **Castle_Red**
- **Gold nodes:** `GoldNode_0` = label **GoldNode_Blue**, `GoldNode_1` = label **GoldNode_Red**
- **Geometry:** `StaticMeshActor_0..5` (6 static meshes — ground/floor + boundary walls; all preserved), `Brush_1` (default brush)
- **Spawn/markers:** `PlayerStart_0`, `TargetPoint_0`, `TargetPoint_1`, `DecalActor_0` (centerline decal)
- **Lighting/sky:** `DirectionalLight_0`, `SkyAtmosphere_0`, `SkyLight_0`, `ExponentialHeightFog_0`, `VolumetricCloud_0`
- **Nav:** `NavMeshBoundsVolume_0`, `RecastNavMesh-Default`, `AbstractNavData-Default`
- **Engine-managed defaults:** `WorldSettings_1`, `BuoyancyManager_0`, `DefaultPhysicsVolume_0`, `GameplayDebuggerPlayerManager_0`, `ChaosDebugDrawActor`

Zero legitimate arena actor removed or modified.

## Save
- `is_dirty(/Game/Maps/L_Arena)` = **true** immediately after the 3 removals (change registered).
- `save_assets(["/Game/Maps/L_Arena"])` = **true** (targeted save — ONLY L_Arena; no donor/mesh re-saves).
- `is_dirty` = **false** post-save (persisted to disk).
- Post-PIE `is_dirty` = **false** again (PIE did not re-dirty the persistent level).

## Clean-start PIE verification (PASS)
Started PIE in L_Arena (in-viewport, 7s warmup), idle player, read the log, then StopPIE.
- PIE world was copied from the saved editor world: `LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/Maps/L_Arena.L_Arena` — i.e. it loaded the CLEANED level.
- **No stray at t=0:** grep of the full session log for `Footman_C_1|Miner_C_2|ArrowTower_C_1|TM040` → **0 matches**. None of the removed instances exist in the PIE world.
- **No Rule-1 defend at match start:** grep for `Rule 1|Defend|intruder` → **0 matches** across the whole session. The bot's half was clear, so it never defended.
- **Bot opened with an offensive play, not a defensive one:** first `LogSiegeBot` line (t≈2s) was `Rule 3 (Attack): played unit 'Ogre'`, then Rule 2 economy (Miner/Deep Mine), Rule 3 attacks, and a Rule 4 cycle-discard. Every bot spawn was on the Red half (X=350 centerline / X≥800 economy). The match then ran to a normal end (`Castle_0` (Blue) destroyed — idle player, winner Red; match-end freeze contract fired: 13 units frozen, income paused, clock stopped, bot loop stopped).
  - Note vs the spec's expected wording ("should open with economy"): the bot actually opened with **attack** because it had enough starting gold to afford an Ogre. The load-bearing acceptance criterion — **no Rule-1 defensive play at match start** — is satisfied (the bot only defends when an enemy unit is on its half, and there no longer is one at t=0). Flagging for transparency; not a defect.

## Post-PIE persistent-level re-verification (find_actors)
- `find_actors(name="BP_")` on the persistent level → **[]** (zero BP_Unit / BP_Building actors remain).
- Full `find_actors` → 27 actors, all intended (list above), no strays.
- `IsPIERunning` = false; `get_current_level` = `/Game/Maps/L_Arena`.

## For QA / build-master
- Single-file change to commit: **`Content/Maps/L_Arena.umap`** (the re-saved, stray-free map). `git status` should show only this map dirty (plus this handoff). No code, no other assets.
- The **M3 transient-Blue-unit WATCH is closed** by this change — its root cause (committed M2 residue) is removed and the clean start is PIE-proven.
- Editor left UP (PID 19464) for TASK-041; do not bounce it on my account.

## Constraints honored
No Git. No compile. No TASKBOARD.md edit (orchestrator flips the board). Saved ONLY L_Arena — no donor/mesh re-saves. Editor left running for TASK-041.
