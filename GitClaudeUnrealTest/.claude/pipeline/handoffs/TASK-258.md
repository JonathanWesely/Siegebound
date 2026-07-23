# TASK-258 — Mines integration: compile + 9-point PIE suite + branch commit = THE W1 BUILD (handoff)

**Author:** build-master · **Date:** 2026-07-23 · **Branch:** `m7.6-arena10x`
**Batch:** TASK-253/254/255/256 (mirrored depleting mines) + TASK-257 (editor: team-lighting/castle-node removal + M_GoldGlow GlowIntensity + DA Mines block) + TASK-259 (Archer/Ogre float fix).
**Gate status at dispatch:** all qa-passed (qa/TASK-253-qa.md, qa/TASK-254-255-256-qa.md, qa/TASK-259-qa.md — all PASS 0 blockers). Editor was CLOSED (Jonathan's pick), so compile ran immediately with no editor-close gate.

---

## 1. COMPILE — PASS (clean)

`Build.bat GitClaudeUnrealTestEditor Win64 Development` (CLAUDE.md command), editor closed, no UnrealEditor.exe running first.
- **Result: Succeeded — 0 errors / 0 warnings, 12.94 s.** Adaptive non-unity set: BattlefieldScatter.cpp, GoldNode.cpp, MinerUnit.cpp, ScatterConfig.cpp, SiegeBotController.cpp, SummonedUnit.cpp (+ SiegePlayerController.cpp / SpellLibrary.cpp which are dirty main-lane residue — they compile fine but are NOT part of this commit). Relinked `UnrealEditor-GitClaudeUnrealTest.dll`.
- 17 QA passes across the batch held: the tree compiled first try. No qa-fail loop needed.

## 2. EDITOR RELAUNCH + HEALTH — PASS

Relaunched detached (UnrealEditor PID 37104), MCP up on :8000, L_Arena loaded clean (Engine initialized, MAP LOAD L_Arena, no fatal errors), IsPIERunning=false. Frame/tooling verified via live MCP calls.

## 3. DA POPULATION (TASK-257 §for-258) — DONE

`DA_BattlefieldScatter` Scatter|Mines block confirmed field-by-field via MCP (ObjectTools.get/set_properties), each explicitly written to override any stale serialized value, then saved not-dirty:

| Field | Value |
|---|---|
| MineCountPerSide | 3 |
| MineMinSpacing | 3000 |
| MineClearanceRadius | 600 |
| MineGoldReserve | 300 |
| MineEdgeMargin | 600 |
| MineMaxSlopeDeg | 30 |
| MineClass | None (⇒ AGoldNode) |

**Note (no git change on the DA):** all 7 values equal the TASK-255 C++ defaults, so UE delta-serialization writes nothing extra → the on-disk `DA_BattlefieldScatter.uasset` is byte-identical to HEAD. The block IS populated and loads correctly (verified by a live PIE reading reserve=300, and the reserve=20 override run below); there is simply nothing new to serialize/commit. TASK-257 flipped to `done`.

## 4. THE 9-POINT PIE SUITE + FLOAT FIX — results

Seed control = `BP_BattlefieldScatter_C_0.overrideSeed` (>0 fixed, 0 random; logged on LogSiegeTerrain). Seed set only in-memory for tests; **reset to 0 and L_Arena NOT saved** (the committed L_Arena.umap is the on-disk TASK-257 version). All MinesPass / Traversability lines are from live PIE (LogSiegeTerrain).

| # | Point | Verdict | Evidence |
|---|-------|---------|----------|
| 1 | Determinism | **PASS** | seed 1337 run ×2 → **byte-identical** MinesPass (mineStream=1296649084, 3 pairs, culls 0/1/0), Traversability CONFIRMED both. |
| 2 | Mirror fairness | **PASS** | Every pair `P=(−X,Y)/M=(X,Y)`; actor transforms confirm GoldNode_0 (−4198,−1724, yaw 0) ↔ GoldNode_1 (+4198,−1724, yaw 180). Equal 2D distances by construction. |
| 3 | Hill parity | **PASS** | seed 314159 pair[2]: `P=(−20023,4325,34) M=(20023,4325,185) hill=yes inj=P`. Primary on a natural hill; twin got a mirror-injected hill. Actor bounds: GoldNode_4 bottom z=34.05 (seated on hill, not buried, mesh→283); GoldNode_5 mirror (+20023,4325, z=184.65, yaw 180) seated on injected hill. Still 6 mines, fb=no. **Observation:** twin sits higher (34 vs 185) — the QA-adjudicated yaw+180-mirror + re-trace effect (TASK-255 dev #3); miners use 2D distance (|X|,Y equal) so fairness holds; visual height asymmetry is a cosmetic W1 watch. |
| 4 | Clearance / reachability | **PASS** | `Traversability CONFIRMED — Blue→Red castle path + 6 mine path(s) exist` every run; placement-time clearance culls enforced (seed 1337 pair1 culls=1; seed 314159 hill pair culls=2); the controlled miner reached & mined GoldNode_0. |
| 5 | Occupancy / claim | **PASS (single-team)** | Controlled Blue miner claimed GoldNode_0 and drained it (control GoldNode_1 stayed at reserve, untouched). *Contested enemy wait-at-ring not forced headlessly — code-verified (QA composed-state-machines) + Jonathan's live W1.* |
| 6 | Depletion | **PASS** | DA MineGoldReserve override → 20 (MinesPass logged `reserve=20`). GoldNode_0 drained 20→0. Logs: `AGoldNode 'GoldNode_0': depleted (initial reserve 20) — evicted 1 miner(s)` + `AMinerUnit ... evicted from depleted mine 'GoldNode_0' — re-seeking on the next poll` (drain→deplete→evict→retarget). **Reserve RESTORED to 300, DA re-saved not-dirty.** *HUD-rate-drop / gauge-dim are code-driven (TASK-253 lazy MID on M_GoldGlow GlowIntensity, TASK-257) — not captured pixel-wise headlessly.* |
| 7 | All-depleted endgame winnable | **NOT DIRECTLY OBSERVED** | Forcing all 6 mines to deplete needs a full driven match. Code-reasoned: FindBestMineFor→null ⇒ bot skips rule 2a (never buys a doomed miner), economy dies gracefully, castle + DeepMine income continues → match still winnable. Deferred to Jonathan's live W1. |
| 8 | Play-Again ×3 | **PASS (fresh-match equivalent)** | 6 fresh PIE matches (seeds 1337×2, 7, 42, 314159, 1337@reserve20) each logged **exactly minesSpawned=6**, zero leaks / no extra mines, no phantom keep-clear discs (RebuildKeepClearZones gold block + ±24,200 discs deleted per TASK-255). *In-match Play-Again BUTTON path (ClearScatter→re-scatter) not triggerable via headless UI input — ClearScatter teardown is QA-verified; Jonathan exercises the button live.* |
| 9 | Bot mine economy | **PASS (core)** | Bot fired `Rule 2 (Economy): played Miner ... toward mine 'GoldNode_1'@(16771,−1221) / 'GoldNode_5'@(20325,4062) — miners now 1/3` — buys anchored on FindBestMine, target 3. *Full 3-arrived + lockout-when-all-depleted not fully traced headlessly → Jonathan live.* Also: with the default "Bot Defensive Economy" deck the bot sometimes ran an all-attack opening (Rule 4: Ogre/Knight/Cleric) and fielded 0 miners in a ~90 s window — expected rule-priority behavior, on the existing cross-field-walk watch list. |
| — | **TASK-259 float fix** | **PASS** | **Ogre** BP_Unit_Ogre_C_0: SkeletalVisualMesh.Z=**−145**=VisualMesh.Z (runtime fix wrote it; bug had 0), mesh bounds min.z=**1.99** (feet at ground). **Archer** BP_Unit_Archer_C_0: SkeletalVisualMesh.Z=**−90**=VisualMesh.Z, bounds min.z=**2.28** (grounded). Roster spot-sweep (QA nit): **Knight** bounds min.z=2.15 (SkelZ −95), **Cleric** bounds min.z=2.15 — grounded. Both previously-floating units (Archer +90, Ogre +145) now ground correctly via the one-line `SetRelativeLocation(VisualMeshBaseRelativeLocation)`. |

### Verification method notes
- `find_actors` / `get_actor_bounds` / `get_actor_transform` / `get_properties` operate on the **PIE world** during play (UEDPIE_0_L_Arena), giving quantitative proof of mine/unit positions, mirror, grounding, and reserves — stronger than a screenshot.
- `CaptureViewport` (with a captureTransform) renders the **editor** world, which at runtime has only the persistent actors (castles, PlayerStart, grass) — NOT the PIE-spawned mines/units. So editor-world captures show the neutral field but not runtime content. Real game frames come from `CaptureEditorImage` (what's on screen during in-viewport PIE).
- Economy points 5/6/9 were exercised with a **controlled Blue miner pre-placed in the editor world** (removed after) because the autonomous bot-only match does not reliably field miners into the mine economy within a short window. All test actors and the seed override were reverted; DA restored to 300.

### Durable captures — `Tools/ArtPipeline/Cache/TASK-258/`
- `pie_game_seed1337.png` — running PIE: neutral dusk lighting (no team-color pools = TASK-257), grounded Blue hero, W1-PREP trees, match-end Defeat/Play-Again UI.
- `pie_units_grounded.png` — running PIE: grounded hero, W1-PREP grassy hills + trees behind, neutral lighting, HUD.
- `overhead_mines_seed1337.png` — editor overhead: neutral-lit field + castles (team-fill lights gone).
- (`mines_mirror_pair0_*`, `mine_closeup_*`, `ogre_grounded_*` are editor-world frames — grass/castle only, no PIE runtime content; kept for reference. The mine/Ogre proofs are the numeric reads above.)

## 5. COMMIT

Branch `m7.6-arena10x`, explicit pathspecs only. See the commit message for the file list. Main-lane residue EXCLUDED (cards.csv, DT_Cards, Lightning VFX, T_Spell_LightningStrike_E, SiegePlayerController.cpp, SpellLibrary.cpp, other-lane handoffs TASK-246/247/248/250/252 + qa/248/250) — those are already on main or ride their own window. TASKBOARD.md + CONVENTIONS.md left as live working-tree pipeline state (board flips are step-7, not part of this commit, per dispatch). Gates: LFS (L_Arena.umap + M_GoldGlow.uasset are `filter: lfs`), secret-scan (0 hits — HF_TOKEN is env-only), leakage-scan (staged set == the enumerated feature files, nothing extra). **NOT pushed** (branch is published — push is Jonathan's call).

## 6. PRE-W1 bounce-checklist items
- Stray `M_T245_WpoTest.uasset` — already gone from disk (deleted overnight per PRE-W1-BOUNCE.md; glob = 0 matches). No action.
- TASK-247/248 docs fold — already COMMITTED to main at `647d7aa` (PRE-W1-BOUNCE.md §4). Not pending; those handoffs appear untracked on the branch and reach main at the merge gate, not this commit.

## 7. Watches / follow-ups for the manager (report, don't fix)
1. **Hill-parity twin height asymmetry** — a mirror-injected twin can sit higher than its natural-hill primary (seed 314159: 34 vs 185). Functionally fair (2D distance equal) and QA-adjudicated; flag only if it reads badly at W1.
2. **Bot all-attack openings** — the Defensive-Economy deck sometimes opens all-attack, fielding 0 miners for a while (cross-field-walk / mine-lock watch already on the board).
3. **Ogre-near-hill secondary capsule lift** — the DIAG SESSION-1 Rank-2 residual (separate from the TASK-259 mesh fix) remains a board WATCH; not re-triggered in this pass. Register a task only if W1 reads bad.
4. **Pre-existing M7 art/audio debt surfaced in miner logs** (non-blocking, all "no-op until asset lands", TASK-174/180): missing `ABP_Miner` anim BP, `MI_Miner_PBR` missing SkeletalMesh usage flag, unresolved `S_MinerClink` / `NS_GoldBurst` / `M_HitFlash` / `S_CastleDestroyed` / `S_DefeatMusic`. Not introduced by this batch.
5. Live confirmation of points 5(contested)/6(HUD+gauge)/7(all-depleted)/8(in-match Play-Again button)/9(3-arrived+lockout) belongs to Jonathan's W1 session (TASK-219 = the acceptance gate).

## 8. Editor state
Left RUNNING on L_Arena for Jonathan's W1 session, MCP up, not in PIE, overrideSeed=0 (fresh random), no test actors. L_Arena is dirty in-memory (seed/actor edits reverted, functionally the shippable state) but was NOT saved — the on-disk / committed L_Arena.umap is the TASK-257 version. Editor close/save is Jonathan's choice.
