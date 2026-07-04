# TASK-036 — L_Arena v2: gold nodes, arena boundary, KillZ — HANDOFF

**Agent:** gameplay-programmer
**Date:** 2026-07-04 (autonomous overnight run; serial editor wave, exclusive editor access)
**Status:** ready-for-qa
**Level SAVED:** `/Game/Maps/L_Arena` (is_dirty=false after final save). Editor left **UP**, PIE stopped, MCP reachable — for TASK-033/040.

All edits are editor/MCP scene edits in L_Arena. **No C++, no compile, no Git, no TASKBOARD edit.** No unrelated donors touched.

---

## 1. Gold nodes (GDD §5 / CONVENTIONS arena contract)

Two `AGoldNode` (`/Script/GitClaudeUnrealTest.GoldNode`) instances placed:

| Actor label | Internal name | Team | Location | Mesh (resolved) | Slot 0 material |
|---|---|---|---|---|---|
| `GoldNode_Blue` | `GoldNode_0` | **Blue** | (-1200, 0, 0) | `/Game/Meshes/SM_GoldNode` | `/Game/Materials/M_GoldGlow` |
| `GoldNode_Red` | `GoldNode_1` | **Red** | (+1200, 0, 0) | `/Game/Meshes/SM_GoldNode` | `/Game/Materials/M_GoldGlow` |

- 800 units in front of each castle (Blue castle -2000 → node -1200; Red castle +2000 → node +1200), clearing the ~410 castle-plinth radius.
- `Team` set per instance and read back: Blue node = `Blue`, Red node = `Red`.
- `NodeMesh` component resolved `SM_GoldNode` via the class `OnConstruction` path (verified `StaticMesh = /Game/Meshes/SM_GoldNode.SM_GoldNode`, `bVisible=true`, `bHiddenInGame=false`).
- Confirmed the asset's slot 0 is `M_GoldGlow` (emissive warm-yellow HDR from TASK-038) — so both nodes render glowing. TASK-038 already verified `M_GoldGlow` visibly emissive in-viewport.
- Actor bounds of GoldNode_Blue: X≈[-1295,-1105], Y[-100,100], Z[0,249.4] — matches TASK-038's SM_GoldNode (190×200×249, ground-center origin), base sitting on the ground (Z=0). Nodes carry NO collision (by AGoldNode design — must not block miner arrival or carve navmesh).

## 2. Arena boundary (M1 carry-over)

Ground `ArenaGround` (StaticMeshActor_1) is the 6400×3200 slab: X[-3200,3200], Y[-1600,1600], top surface Z=0. Four invisible collision walls fully enclose it:

| Wall | Internal name | Center | Bounds (min→max) |
|---|---|---|---|
| East | StaticMeshActor_2 | (3250, 0, 800) | X[3200,3300] Y[-1700,1700] Z[-200,1800] |
| West | StaticMeshActor_3 | (-3250, 0, 800) | X[-3300,-3200] Y[-1700,1700] Z[-200,1800] |
| North | StaticMeshActor_4 | (0, 1650, 800) | X[-3300,3300] Y[1600,1700] Z[-200,1800] |
| South | StaticMeshActor_5 | (0, -1650, 800) | X[-3300,3300] Y[-1700,-1600] Z[-200,1800] |

**Implementation:** each wall is a `StaticMeshActor` using `/Engine/BasicShapes/Cube` (100³ unit cube), scaled to a thin tall slab. Per-wall StaticMeshComponent settings:
- Collision: the engine Cube ships `collisionProfileName = BlockAll`, `collisionEnabled = QueryAndPhysics`, `objectType = ECC_WorldStatic` — blocks the Pawn (hero) out of the box; left as-is.
- `bHiddenInGame = true` — invisible in PIE/game (visible only as a faint box in the editor viewport).
- **`bCanEverAffectNavigation = false`** — the walls do NOT carve the navmesh (the key nav-exclusion requirement). Verified read-back on every wall.
- Actor tag `ArenaBoundary` on all four (identification / future queries).

**Geometry rationale:**
- Inner faces are flush to the ground edges: East/West inner face at X=±3200, North/South inner face at Y=±1600; the 100-thick body extends OUTWARD only. So the full walkable interior is unobstructed.
- N/S walls span X[-3300,3300] and E/W walls span Y[-1700,1700], so the four walls overlap at all corners — no corner gaps.
- Vertical: Z[-200,1800] → **1800 units of headroom above the ground (≥1000 required)**, and the base dips to Z=-200 (below the slab's -100 bottom) so there's no gap at the floor. Open top (no ceiling) — gameplay not capped.

**Implementation note for QA:** spec said "invisible blocking volumes." I used invisible collision-boxed `StaticMeshActor`s rather than literal `ABlockingVolume` brushes because (a) brush volumes spawned via MCP `add_to_scene_from_class` come with a degenerate/empty brush (no reliable way to author brush geometry through the MCP surface), whereas (b) an engine-cube StaticMeshActor gives deterministic, precisely-scalable box collision with fully MCP-settable `bHiddenInGame` / `bCanEverAffectNavigation` — and QA/build can verify all of it via properties. Gameplay outcome is identical (invisible, blocks the pawn, no nav carve). Flag if you want true `ABlockingVolume`s instead — cheap to swap.

## 3. KillZ

`WorldSettings.KillZ = -2000` (was the default -1048575). Read back `KillZ = -2000`, `bEnableWorldBoundsChecks = true` (so a pawn below -2000 actually triggers `FellOutOfWorld`). Saved with the level.

The runtime effect is code that already shipped (TASK-024, commit aafd968): `AHeroCharacter::FellOutOfWorld` deliberately does NOT call `Super` (which would `Destroy()` the hero); instead it routes through `HandleDeath()` → `OnHeroDied` broadcast → `ASiegeGameMode` 5 s respawn timer → `RestoreHeroAtStart` (PlayerStart at (-1400,0,100), else own-castle side). Net: a KillZ fall = standard death → hero back at its castle in ~5–6 s (GDD §3.1).

---

## 4. Verifications performed here

- **Boundary blocking (static spot-check):** `trace_world` horizontally at Z=100 (above the ground) from just inside each edge outward through each wall — all four hit at exactly distance 100, i.e. the wall's collision starts exactly at the ground edge (X=±3200 / Y=±1600). Confirms live blocking collision on all four sides at pawn height.
- **Navmesh coverage + miner pathing (PIE spot-check):** placed a temporary Blue `BP_Unit_Miner` at (-600, 400, 0) (Team=Blue, CardID=Miner, AutoPossessAI=PlacedInWorldOrSpawned), ran a 10 s PIE session, read `LogGitClaudeUnrealTest`:
  - The current miner (`BP_Unit_Miner_C_1`) logged **only** the benign `profile 0 … running Standard behavior` note. **No** `no same-team AGoldNode` error, **no** `MoveToActor … failed — is the NavMeshBoundsVolume covering L_Arena` warning, **no** `no AAIController` warning. ⇒ it found GoldNode_Blue, was possessed by an AI controller, and issued a successful navmesh move. **Navmesh covers the Blue-half route to GoldNode_Blue.**
  - Contrast/sanity: a *stale earlier* session (`BP_Unit_Miner_C_0`, before nodes existed) DID log the `no same-team AGoldNode … miner idles` error — proving that error fires when nodes are absent, and its absence now is meaningful.
  - No `AGoldNode … mesh failed to load` warnings ⇒ both gold nodes resolved SM_GoldNode cleanly in PIE.
  - Deck built clean: `UDeckComponent … built a 50-card draw pile from 6 card rows`.
  - Cross-category `Error:` scan for the session: only unrelated environmental noise — `LogAudioMixerWasapi … AUDCLNT_E_DEVICE_INVALIDATED` (no audio device in this headless-ish session) and an old `LogModelContextProtocol` session-reinit from before this task. Nothing from L_Arena content/nav/assets.
  - Temp miner deleted after the test; scene re-verified (no `Miner` actors) and level re-saved clean.
- **Editor-world vs PIE-world limitation (why some timings are deferred):** MCP `get_actor_transform` on the miner read (-600,400,0) unchanged during PIE — MCP operates on the **editor** world, not the live PIE duplicate. So I cannot measure the live walk-time-in-seconds or drive a live KillZ teleport through MCP. The **pathing health** (the thing that matters for nav) is confirmed via the global output log, which is world-agnostic.

## 5. Deferred to TASK-040 (integrated PIE)

- **Exact miner walk time to GoldNode_Blue in seconds.** Note: the spec's "~10 s per §3.3" is the canonical near-castle spawn; this spot-check spawned at (-600,400), only ~721 units (2D) from the node — expect roughly ~2–4 s for THIS spawn, not 10 s. §3.3's ~10 s applies to a castle-adjacent spawn. TASK-040's interactive PIE should record the real number for the card-spawned case.
- **Forced KillZ fall → 5–6 s respawn timing** (needs live PIE-world hero manipulation; preconditions all set/verified here: KillZ=-2000, world-bounds-checks ON, FellOutOfWorld→HandleDeath→5 s respawn code present).
- **Full "hero can't escape from any edge at sprint+jump" sweep** (per the verification-boundary in the spec — I verified collision exists at all four edges via trace; the exhaustive movement sweep is TASK-040's).
- **Red-half navmesh** confirmed only by symmetry here (Blue-half confirmed live). Geometry is mirror-symmetric, the NavMeshBoundsVolume (±3400/±1800) is unchanged and spans both halves, and the walls are nav-disabled — so Red-half coverage is expected identical. TASK-040 should confirm both halves in its full run.

## 6. Placement dead-zone note (M2 watch item)

GoldNode_Blue at (-1200,0) is 800 units from the Blue castle (-2000,0), clearing the ~410 plinth radius (≈390 units of open ground between the plinth edge and the node). The miner pathed to the node without any partial-path/failure warning, so no pathing dead-zone was observed near the castle in this spot-check. Card-**placement** feel near the castle/plinth is a design/playtest item and cannot be measured headlessly — leaving it as an M2 watch item per spec (mesh rework only if a playtest demands it).

## 7. MCP stability

Stable across the entire task — scene/actor/object/asset ops AND a full StartPIE→StopPIE cycle all succeeded, no hangs, no dropped calls. (The one `LogModelContextProtocol` error in the log predates this task's work.) No blocker.

## Assets / actors touched
- `/Game/Maps/L_Arena` — added: GoldNode_0 (GoldNode_Blue), GoldNode_1 (GoldNode_Red), StaticMeshActor_2/3/4/5 (ArenaBoundary_East/West/North/South); WorldSettings KillZ=-2000. Saved. (UE Git provider may auto-stage the .umap — TASK-040 commits.)
- Referenced (not modified): `/Game/Meshes/SM_GoldNode`, `/Game/Materials/M_GoldGlow`, `/Engine/BasicShapes/Cube`, `/Game/Blueprints/Units/BP_Unit_Miner` (temp instance placed+deleted for the pathing test).
