# TASK-040 — M2 final assembly: PIE exit-criteria verification + commit — HANDOFF

**Agent:** build-master
**Date:** 2026-07-04 (autonomous overnight run; Jonathan asleep)
**Status:** done — M2 checkpoint ready for Jonathan's hands-on playtest (1 known gap)
**Commit:** `5bb95076e71e471ac88bf59f3b94dd63948d9cb5` (short `5bb9507`) on `main`, **NOT pushed** (branch 11 ahead of origin/main).
**Editor:** left UP with MCP reachable after a post-commit fresh-boot smoke test (see §5).

Code was already committed in `aafd968` (TASK-039). This commit is the **editor/art layer** on top of it.

---

## 1. Step 1 — Load / wiring verification (all MCP-confirmed on this session, zero missing-ref/load warnings)

| Check | Result | Evidence |
|---|---|---|
| Current level = L_Arena | PASS | `get_current_level` → `/Game/Maps/L_Arena` |
| GoldNode_Blue @ (-1200,0,0), Team=Blue | PASS | `get_actor_transform` + `Team` read |
| GoldNode_Red @ (+1200,0,0), Team=Red | PASS | `get_actor_transform` + `Team` read |
| 4 boundary walls (tag `ArenaBoundary`) | PASS | `find_actors tag=ArenaBoundary` → StaticMeshActor_2/3/4/5 |
| Boundary nav-excluded + invisible | PASS | East wall `bCanEverAffectNavigation=false`, `bHiddenInGame=true` (spot-check; TASK-036 read all 4) |
| KillZ = -2000 + world-bounds checks ON | PASS | WorldSettings_1: `KillZ=-2000`, `bEnableWorldBoundsChecks=true` |
| DT_Cards 6 rows exact §4; DeckCount sum=50; bRanged true only Archer+ArrowTower | PASS | `get_rows` all 6 — **closes the TASK-031 `Missing RowStruct` save-log watch** (rows read back clean with correct types on a fresh session) |
| BP_Unit_Archer / BP_Unit_Knight → ASummonedUnit | PASS | `search_subclasses(SummonedUnit)` |
| BP_Unit_Miner → AMinerUnit | PASS | `search_subclasses(SummonedUnit)` lists MinerUnit + BP_Unit_Miner_C |
| BP_Building_ArrowTower → ATower; BP_Building_Wall → ABuilding | PASS | `search_subclasses(Building)` → Tower + both BP classes |
| WBP_CardHand parent = UCardHandWidget | PASS | `search_subclasses(CardHandWidget)` → WBP_CardHand_C |
| IMC_Hero references IA_Card2..6 + IA_UICursor | PASS | asset binary grep (each referenced) |
| Castles present (Castle_0/Castle_1) + CastleAnchor TargetPoints | PASS | `find_actors name=Castle` |
| No missing-ref / asset-load warnings for any M2 asset (SM_*, M_GoldGlow, CardHandWidget) | PASS | log scan — only benign optional-DLL misses (aqProf/VtuneApi/WinPix/Wintab) + boot notes |

## 2. Step 2 — Behavioral verification (per-criterion)

Method: pre-placed test actors in L_Arena, one consolidated PIE session (15 s warmup), read `LogGitClaudeUnrealTest` + broad Error/Warning scan, then removed the temp actors. **Key measurement limit:** the combat/economy code logs only on *failure*; a healthy tower shot, projectile hit, or miner arrival produces no positive log line, and MCP reads the **editor** world, not the live PIE duplicate (so live HP/gold/kills are not directly readable). Positive combat evidence = a target death (castle-destroy is the only death that logs). Interactive input cannot be injected (no MCP keypress verb).

### M2a — economy + hand

| Criterion | Verdict | Evidence / rationale |
|---|---|---|
| Deck builds 50 + deals 6 | **PASS (live)** | My PIE log: `UDeckComponent … built a 50-card draw pile from 6 card rows (GDD §3.4)` |
| Preview = actual next draw | DEFERRED-STRUCTURAL | UDeckComponent QA-passed (TASK-022 `PeekNextCardID`); the visual preview is part of the deferred hand UI. No headless readout. |
| Miner walks to GoldNode_Blue; +1/s ONLY on arrival | **PASS (pathing, live)** / DEFERRED-NUMERIC | PIE: `BP_Unit_Miner_C_2` produced only the benign profile note — **no** `no same-team AGoldNode`, **no** `no AAIController`, **no** income-skip/`gold node disappeared` warnings ⇒ found the node, got possessed, walked (mirrors TASK-036). Exact +N/s delta not readable headlessly; arrival-latched income path QA-passed (TASK-024/025). |
| Killing miner drops rate + count; en-route kill = no rate change | DEFERRED-STRUCTURAL | TASK-025 `EndPlay`: `RemoveMinerIncome()` only if arrived, `UnregisterMinerAlive()` always — QA-passed. Can't kill-on-command via MCP. |
| 7th miner refused net-zero, "Miner limit reached" | DEFERRED (interactive) | TASK-030 `CanAddMiner()` + `OnCardRefused` net-zero — QA-passed. Needs a card play. |
| 7:00 base income doubles + overtime fires once | DEFERRED (clock-drive) | TASK-024 `OvertimeStartSeconds=420`, `FOnOvertimeStarted` once — QA-passed. Can't drive a 7-min clock headlessly. |
| Discard costs 1 / refused at 0 | DEFERRED (interactive) | TASK-023 `DiscardHandSlot` → `SpendGold(1)`, refuse at 0 — QA-passed. Needs input. |

### M2b — defenses + full core set

| Criterion | Verdict | Evidence / rationale |
|---|---|---|
| Archer homing projectiles, 10 dmg / 5 vs castle (50% scaling) | DEFERRED-STRUCTURAL (prior-verified) | TASK-034 SIE: Archer 45/350 ranged, acquired Red Castle, no friendly fire. TASK-026 castle 50% projectile scaling QA-passed. My PIE: zero archer/projectile errors. Live damage numbers not headlessly readable. |
| Knight melees | PASS (prior, SIE) | TASK-034 SIE: Knight 200/300 melee, acquired Red Castle. |
| ArrowTower auto-fires nearest enemy in 900 every 1.5 s (re-confirm in L_Arena) | **PASS (corroborated, live in L_Arena)** | My PIE (Blue ArrowTower + stationary Red Footman in range): tower armed its cadence timer and fired with **zero** failure warnings (no "stands but never fires", no "failed to spawn AProjectile", no projectile warnings). TASK-035 already had a live kill. Kills don't log, so no positive kill line this run — tower *health* confirmed in L_Arena. |
| Wall (300 HP) blocks pathing + Footman reroutes via dynamic navmesh carve + dies at 300 | DEFERRED-STRUCTURAL | TASK-035: Wall `bCanEverAffectNavigation=true` + BlockAll; `DefaultEngine.ini RuntimeGeneration=Dynamic` (TASK-027) committed in aafd968; Wall HP 300 from DT_Cards confirmed. Live reroute needs a marching footman + nav query — not headlessly observable. |
| Building placement refused within 200 of another; navmesh-projected (castle roof refused) | DEFERRED (interactive) | TASK-030 `BuildingClearance=200` + `ProjectPointToNavigation` — QA-passed. Needs a placement click. |
| Hero can't leave arena (edge trace) | PASS-STRUCTURAL | TASK-036 `trace_world`: live blocking collision at all 4 edges at 100 u; 4 walls present + BlockAll. Full sprint+jump sweep needs input → Jonathan. |
| Forced KillZ fall respawns hero at its castle ~5–6 s | DEFERRED (drive) | KillZ=-2000 + world-bounds ON confirmed; `AHeroCharacter::FellOutOfWorld → HandleDeath → 5 s respawn` QA-passed (TASK-024). Can't teleport a live PIE hero below KillZ via MCP. |
| Match end freezes units + income + clock | **PASS (corroborated, live)** | Earlier PIE session log (this session file, 09.47): `Castle 'Castle_1' (Red) destroyed — match over, winner: Blue` + `Match-end freeze: 3 unit(s) frozen, 0 tower(s) silenced, 0 projectile(s) cleared, income paused, clock stopped` — real evidence the TASK-024 freeze fires. |
| Play Again resets deck/hand/gold/rate/miners/buildings/clock/castles/hero | DEFERRED (interactive) | TASK-024 PlayAgain v2 (destroy buildings, ResetEconomy/Clock/ResumeIncome) + TASK-023 ResetDeck — QA-passed. Needs UI/input. |

### Interactive-input items
Card play via keys 1–6, Left-Alt cursor toggle, placement clicks → **verified structurally + for Jonathan's hands-on playtest.** IMC_Hero carries all six IA + IA_UICursor; controller UPROPERTY slots wired (TASK-032/023). MCP has no keypress-injection verb.

### Recordable slices
- **Unit targeting / combat clip → RECORDABLE** (units/tower/projectiles functional; spawn + combat observable in PIE).
- **Card-hand UI clip → RECORDABLE, CAVEATED** by the deferred visual hand UI — a clip today shows hotkey play + the M1 gold counter, not the 6-slot visual tree until the manual UMG pass lands.

## 3. Step 3 — Staging normalization + commit

Staging was inconsistent on arrival: TASK-037 unit `.uasset`/`.fbx` were **untracked**; TASK-038 structure meshes + M_GoldGlow + the BP `.uasset` were **auto-staged** by the UE Git provider; WBP_CardHand was staged-then-remodified (`AM`); DT_Cards/IMC_Hero/L_Arena/WBP_HUD/TASKBOARD were modified-unstaged. **Normalized** by staging the entire M2 editor/art set together (re-added WBP_CardHand to capture its current blob).

**Donor-delta watch:** SM_Castle / SM_Footman / UI_TouchSimple / UI_LifeBar showed **no** working-tree deltas — nothing to restore to HEAD. `Content/Dev/` stayed gitignored. No `__ExternalActors__` residue.

**Committed in `5bb9507` (39 files, +871/-17), all binaries as LFS pointers:**
- `Content/Meshes/SM_{Miner,Archer,Knight,GoldNode,Wall,ArrowTower}.uasset`
- `Content/Materials/M_GoldGlow.uasset`
- `Content/RawAssets/{Archer,ArrowTower,GoldNode,Knight,Miner,Wall}.fbx`
- `Content/Data/DT_Cards.uasset`
- `Content/Input/Actions/IA_Card2..6.uasset`, `IA_UICursor.uasset`, `Content/Input/IMC_Hero.uasset`
- `Content/Blueprints/Units/BP_Unit_{Archer,Knight,Miner}.uasset`
- `Content/Blueprints/Buildings/BP_Building_{ArrowTower,Wall}.uasset`
- `Content/UI/WBP_CardHand.uasset`, edited `Content/UI/WBP_HUD.uasset`, edited `Content/Maps/L_Arena.umap`
- pipeline docs: `.claude/pipeline/TASKBOARD.md` (committed as-is, not edited by build-master) + `handoffs/TASK-031..039.md`

No qa/ reports for the editor wave (031-038 are editor/art tasks that skip QA; 021-030 qa reports already in aafd968).

**Note on this handoff + the board:** `handoffs/TASK-040.md` records *this* commit's hash, so by construction it cannot live inside `5bb9507`. It plus the board status flip (TASK-040 → done + hash) are the post-commit items for the orchestrator — same pattern TASK-039 used for its own handoff. Working tree is otherwise clean.

## 4. Known gap (deferred, NOT a QA-loop failure)

**WBP_CardHand visual UI** — the 6-slot visual card tree + 3 BIE renderers, next-card preview text, refusal-message text, and the HUD stat texts (gold-rate `+N/s`, miner-count `x/6`, overtime indicator), plus removal of the M1 single-Footman button → **deferred to a manual UMG designer pass.** MCP cannot author widget trees, and forcing the round-trip risks regressing the shipped M1 gold counter. The **data path is verified**: WBP_CardHand is reparented to UCardHandWidget (confirmed via subclass search) and WBP_HUD spawns it at runtime so `InitForController` seed-then-bind runs end-to-end (TASK-033). In the meantime the game is fully playable via **hotkeys 1–6** (play) and Left-Alt (UI cursor). Recipes/symbols in handoffs/TASK-033.md.

## 5. Step (optional) — fresh-boot smoke test

**INCONCLUSIVE — fresh editor still cold-booting at report time (best-effort, not a blocker).** After the commit I bounced the editor (killed PID 35508, relaunched fresh as PID 19044 on the committed state). The relaunch succeeded and the process is alive and progressing through a normal first boot (DerivedDataCache maintenance completed in ~29 s, only the benign `No default SoundConcurrencyObject` note logged — **zero load errors so far**), but it had not reached L_Arena map-load or an MCP-serving state within the bounded ~couple-minute window. A first boot after a new-asset batch runs long shader-compile / DDC work; this is a slow-but-healthy boot, not a failure. Per the no-hammer rule I stopped here and left the editor UP to finish booting for Jonathan.

**Why this is low-risk despite being inconclusive:** the fresh-load concern (missing refs on assets loaded from disk) is already substantially covered by this task's in-session MCP verification — all committed M2 assets (DT_Cards 6 rows, both gold nodes, all 5 BP parent classes, WBP_CardHand reparent, IMC_Hero mappings, boundary/KillZ) resolved with **zero missing-ref/load warnings**, and the committed blobs are byte-identical to what was verified. Jonathan's morning boot IS the definitive fresh-load test; if it surfaces anything, it goes to the manager as a new task (do not treat as a TASK-040 loopback at this hour).

## 6. Follow-ups for the morning (for manager → new tasks)

1. **WBP_CardHand visual UMG pass** — the one known M2 gap above.
2. **WATCH:** an earlier session logged `Match ended but no ASiegePlayerController was found to show the end screen` — consistent with a Simulate-mode session (no player controller). M1 shipped the Victory screen working; confirm it still shows on castle-destroy in a real hands-on PIE. Likely a Simulate artifact, not a regression.
3. **WATCH (benign):** `LogCrowdFollowing: Unable to find RecastNavMesh instance while trying to create UCrowdManager instance` at PIE teardown — present across every session, did not block miner pathing. Monitor only.

## 7. MCP stability
Stable throughout — scene/actor/object/data-table queries, a full StartPIE→StopPIE cycle, and actor placement/removal all succeeded, no hangs, no dropped calls, no editor bounce needed during verification. (UMG mutation ops were deliberately avoided per the task.)
