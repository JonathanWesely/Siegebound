# TASK-136 PART 1 — 4× battlefield reshape + M6.5 scatter-code compile (build)

**Owner:** build-master  **Date:** 2026-07-10  **Result:** DONE (verified) — **NO COMMIT this pass** (coordinator gates the commit until TASK-137 scatter + Jonathan's approval).

## Step 1 — Compile (TASK-133 + TASK-134): PASS
- exit 0 · 0 warn · 0 err · DLL **20:16:34 (2,074,112 B) → 23:31:46 (2,145,792 B)**
- New classes confirmed in the DLL: `SiegeScatterConfig`, `SiegeBattlefieldScatter` (→ `USiegeScatterConfig`/`ASiegeBattlefieldScatter` exist in-editor, unblocking TASK-137).

## Step 2 — Relaunch: clean
Editor detached PID 3788, MCP responding (`IsPIERunning`→false), title `GitClaudeUnrealTest - Unreal Editor` — no Restore-Packages modal.

## Step 3 — L_Arena reshape (symmetric about X=0), via Unreal MCP
Actor moves (before → after), identified by X-sign:

| Actor (level name) | Role | Before | After |
|---|---|---|---|
| Castle_0 | Castle_Blue | (−2000,0,0) yaw0 | **(−8000,0,0)** yaw0 |
| Castle_1 | Castle_Red | (2000,0,0) yaw180 | **(8000,0,0) yaw180** |
| TargetPoint_0 | CastleAnchor_Blue | (−2000,0,0) | **(−8000,0,0)** |
| TargetPoint_1 | CastleAnchor_Red | (2000,0,0) | **(8000,0,0)** |
| GoldNode_0 | Gold Blue | (−1200,0,0) | **(−7200,0,0)** |
| GoldNode_1 | Gold Red | (1200,0,0) | **(7200,0,0)** |
| PlayerStart_0 | hero spawn | (−1400,0,100) | **(−6800,0,100)** |

**Gotcha recorded:** `ActorTools.set_actor_transform` resets UNSPECIFIED transform fields to identity (NOT "don't change" as the doc implies). Caught it on verify — Castle_1's yaw (180→0) and ArenaGround's Z (−50→0) got reset by location-only/scale-only calls; both re-set with full transforms and re-verified. All other moved actors were yaw0/scale1 so unaffected.

### Widened bounds (to contain the 18000-wide field)
| Actor | Before | After |
|---|---|---|
| ArenaGround (SMA_1) | scale(64,32,1)@z−50 → ±3200/±1600 | **scale(180,48,1)@z−50 → bounds ±9000 X / ±2400 Y, top z0** |
| ArenaBoundary_East (SMA_2) | X+3250, scaleY34 | **X+8800, scaleY48** |
| ArenaBoundary_West (SMA_3) | X−3250, scaleY34 | **X−8800, scaleY48** |
| ArenaBoundary_North (SMA_4) | Y+1650, scaleX66 | **Y+2400, scaleX180** |
| ArenaBoundary_South (SMA_5) | Y−1650, scaleX66 | **Y−2400, scaleX180** |
| NavMeshBoundsVolume_0 | scale(34,18,5) → ±3400/±1800 | **scale(90,24,5) → bounds ±9000 X / ±2400 Y / ±500 Z** |

KillZ **unchanged** (−2000, vertical) per spec.

### Step 3b — DYNAMIC navmesh (mandatory): DONE + verified
- `RecastNavMesh-Default` `RuntimeGeneration = Dynamic` (set via `ObjectTools.set_properties`, **read-back confirmed** `{"RuntimeGeneration":"Dynamic"}`).
- Runtime rebuild confirmed in PIE log: `Recreating dtNavMesh instance (RecastNavMesh-Default) … serialized maxTiles 84 vs calculated 285` — the navmesh rebuilt for the enlarged bounds.
- **Test-obstacle reroute PROVEN:** placed a temporary 600×600×300 blocking cube at the center lane (0,0,150). In PIE the bot's Ogre spawned at Y=−95 (INSIDE the cube's ±300 footprint) and **detoured to Y=+348** around it while traversing to the Blue castle; the match still completed with the cube in the lane (units routed around, not through). **Test cube REMOVED** after (verified: `find_actors "TASK136"` → empty).

## Step 4 — PIE functional verification: ALL PASS
Method: in-viewport PIE + MCP actor-transform tracking + log (no foreground/screenshot — Jonathan is actively gaming; kept it non-disruptive).
- **Hero spawns at new PlayerStart:** `BP_HeroCharacter_C_0` at **(−6800, 0, 98.15)** — exact.
- **End-to-end traversal at 4×:** bot Red units (Knight, Ogre) marched from centerline to **X≈−7500** (at the Blue castle); **match COMPLETED**: `Castle 'Castle_0' (Blue) destroyed — match over, winner: Red` (~85 s march start→win = the expected ~4× longer).
- **Economy on moved nodes:** Miner toward `GoldNode_Red (6800→7200)`, Deep Mine `near GoldNode_Red (7200,0,10)`.
- **No fall-throughs / stuck / path failures:** KillZ/OutOfWorld/no-path scan returned none; all tracked units at Z≈87–147 (on the ground).
- Play-Again reset NOT explicitly re-tested this pass (needs UI/cheat interaction); the win-path completing exercises match end.

## Step 5 — Screenshot
`…/scratchpad/arena_4x.png` — in-engine `CaptureViewport` bird's-eye from behind the Blue castle down the +X corridor: shows the long enlarged field with the distant Red castle and a coordinate grid (walls at Y±24 m, field to ±80 m). Rendered via MCP (NOT a GDI full-screen grab — a full-screen grab captured Jonathan's foreground LoL game once and was disregarded; I did not foreground the editor over his game).

## State / constraints
- **NO COMMIT, NO GIT this pass.** Jonathan's imports (`Content/Fab/**`, marketplace folders, `.uproject`) untouched.
- L_Arena **saved** (surgical `save_assets` on `/Game/Maps/L_Arena` only) so the reshape persists for TASK-137 and avoids a Restore-Packages trap on any future bounce.
- Editor left RUNNING (PID 3788) for TASK-137. Editor camera left at the bird's-eye vantage (harmless).
