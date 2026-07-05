# TASK-069 — M4 final assembly: expanded-roster PIE verification + commit (build-master handoff)

**Date:** 2026-07-05
**Result:** M4 editor/art INTEGRATED + committed on `main` (parent `65861ce`). NOT pushed. **M4 CHECKPOINT READY FOR JONATHAN'S PLAYTEST.**
**Editor:** left UP (cold-booted this task, PID 10932, MCP healthy, L_Arena loaded clean-of-warm-dirt).

## Commit
- **M4 editor/art integration commit** on `main`, parent `65861ce` (TASK-068 code batch). Hash reported to the orchestrator + posted in 🔧 Build & Git. NOT pushed (branches `m2-testable`/`m3-testable` untouched).
- COMMITTED: 11 `Content/Meshes/SM_*.uasset` (Cavalry/Pikeman/Sapper/MilitiaMob/Ogre/Cleric/Longbowman/BombTower/BallistaTower/Barracks/DeepMine) + 11 `Content/RawAssets/*.fbx`; `Content/Data/DT_Cards.uasset` (22-row Set II, save-clean); 7 `Content/Blueprints/Units/BP_Unit_*.uasset` + 4 `Content/Blueprints/Buildings/BP_Building_*.uasset`; `Content/UI/WBP_HUD.uasset` (v4 upgrade row); handoffs `TASK-061..069.md`.
- EXCLUDED (deliberate): `TASKBOARD.md` (orchestrator owns the board per dispatch). No donor re-saves (SM_Castle/SM_Footman/UI_TouchSimple/UI_LifeBar unmodified). `Content/Dev/` stays gitignored. **L_Arena.umap NOT committed** (see Step 4 finding — it is git-clean and I did not save it).

## DT_Cards 22-row reconfirm (TASK-061 auto-reimport WATCH — CLOSED)
Re-verified on BOTH the warm editor and again after a cold boot: `list_rows` = exactly the 22 expected rows in order (no legacy/dupes); `get_rows` all §4 character-exact; **DeckCount sum = 50** (core 20: Footman6/Archer4/Knight2/Miner3/ArrowTower2/Wall3; Set II units 14 = 2 each; buildings/econ/util 10 = 2 each; upgrades 6: Blade2/Plate2/Boots1/Banner1), each ≥1 and ≤ MaxCopies; keyword columns set ONLY where §4 specifies (bRanged Archer/ArrowTower/Longbowman/BombTower/BallistaTower; bCharge Cavalry; bSlayer Pikeman; bSuicide Sapper; swarmCount 4 MilitiaMob; aoERadius 250 Sapper+BombTower; minRange 300 BallistaTower; spawnCardId Footman + spawnInterval 8 + lifetime 60 Barracks; profiles Siege Sapper/Ogre, Support Cleric). **is_dirty = false** on DT_Cards AND WBP_HUD on both warm + cold boot → the committed blobs are the verified state; the editor's live CSV auto-reimport left the asset clean.

## Per-criterion verdict table

### Step 1 — Load / wiring (zero missing-ref warnings) — PASS
| Item | Verdict | Evidence |
|---|---|---|
| DT_Cards resolves all 22 rows | PASS | list_rows=22, get_rows §4-exact, sum50, save-clean (warm+cold) |
| 7 unit BPs reparent + resolve meshes (incl. custom SM_MilitiaMob/SM_Longbowman) | PASS | find_assets shows all 7; TASK-062 CDO readback (ASummonedUnit parent, mesh, custom meshes, DT_Cards stat bind); ran in PIE with zero missing-class warnings |
| 4 building BPs reparent + resolve meshes | PASS | find_assets shows all 4; TASK-063 CDO readback (ATower/ABarracks/ADeepMine + mesh); bot played all 4 in PIE, composed soft-class paths resolved |
| WBP_HUD upgrade row loads | PASS | is_dirty=false; loaded in PIE with zero WBP_HUD compile/ICE errors this session (the 00.31 ICE block is stale/superseded per TASK-064) |
| Hero-upgrade delegate wired | PASS | code @65861ce: `FOnHeroUpgradesChanged`, `ApplyUpgrade`→`EHeroUpgradeResult` (refund), `ResetUpgrades`, `GetUpgradeStackCap`; TASK-064 HUD seed-then-bind |
| Zero missing-ref warnings | PASS | PIE error scan clean of missing-ref/Accessed-None; only benign known lines (DeepMine `CardType 2 Economy expected Building` — TASK-057 known-benign; end-screen `InputMode:UIOnly focus Non-Focusable` — benign) |

### Step 2 — §9-4 + §4 slice
**Bot Set II (idle-player PIE, LogSiegeBot) — PASS**
| Line | Verdict | Evidence (LogSiegeBot / LogGitClaudeUnrealTest, my session 00.53) |
|---|---|---|
| Bot reaches economy then attacks with growing Set II incl. Ogre push | **PASS** | Rule 2 Miner ×2 + Deep Mine economy; Rule 3 Attack repeatedly played **Ogre (cost 12)** plus Cavalry/Pikeman/Sapper/Longbowman/Cleric/MilitiaMob/Archer/Footman — Ogre push confirmed, multiple Ogres |
| Discards hero-upgrade/Instant cards (logged) | **PASS** | Rule 4 (Cycle) discarded **WarBanner**, **Masons**, **SharpenedBlade** (all HeroUpgrade/Utility) |
| Never unaffordable | **PASS** | every play logs `gold X->Y` with Y≥0 and X≥cost; no negative/over-spend |
| Never Blue-half | **PASS** | all attack spawns at X=350 (Red side of center X=0); miner X=800, Deep Mine X=1200 — all Red half |
| MilitiaMob → 4 units | DEFERRED (data-backed) | bot played MilitiaMob; DT_Cards swarmCount=4 passed to `SpawnBotCardActor`/`SpawnUnitSwarm` (QA-passed TASK-059); per-unit PIE-world 4-count not read via MCP → visual confirm at Jonathan's playtest |

**Keywords / profiles (isolated in-combat multiplier reads) — DEFERRED-to-playtest (each backed by a specific code QA PASS + BP flag/stat bind)**
| Line | Verdict | Backing |
|---|---|---|
| Cavalry Charge 2× (first post-move hit) | DEFERRED | QA PASS TASK-055 "ONE-hit charge, bChargePrimed consumed once"; BP bCharge bound (TASK-062); bot played Cavalry |
| Pikeman Slayer 2× vs ≥150 HP | DEFERRED | QA PASS TASK-055 "×2 vs MaxHP≥150"; BP bSlayer bound; bot played Pikeman |
| Sapper suicide AoE detonates once (Siege 2× vs structures) | DEFERRED | QA PASS TASK-055 "SINGLE detonation (bDetonated+bDead guards)"; BP bSuicide+AoE250 bound; bot spawned Sappers into the assault |
| Ogre/Sapper Siege ignore units → hit buildings/castle at 200% | PARTIAL-PASS + DEFERRED | integrated: bot Ogres+Sappers drove Blue castle 2000→0 in ~41s (consistent w/ Siege 200%); QA PASS TASK-054 "castle Siege×2 before Projectile×0.5, building Siege×2 branch"; isolated 200% number read deferred |
| Cleric heals a damaged friendly + never attacks | DEFERRED | QA PASS TASK-054 "heals 8/s ≤400 clamped, never attacks, FreezeAI stops healing"; BP Support bound; bot played Cleric |
| Longbowman fires at long range | DEFERRED | DT_Cards range 1200 + bRanged; BP bound; bot played Longbowman |

**New buildings — DEFERRED-to-playtest (code QA PASS + data bound; live per-building behavior needs an interactive defender or a >60 s match)**
| Line | Verdict | Backing |
|---|---|---|
| Bomb Tower AoE-splashes a cluster (2.5 s) | DEFERRED | DT_Cards AoERadius250/cadence2.5; QA PASS TASK-056; BP ATower (TASK-063) |
| Ballista blind-spot ignores inside-300 (3.0 s) | DEFERRED | DT_Cards MinRange300/cadence3.0; QA PASS TASK-056; BP |
| Barracks spawns a Footman every 8 s + self-destructs at 60 s | PARTIAL-PASS + DEFERRED | spawn-timer arm + FreezeAI confirmed (match-end log "1 barracks frozen"); bot played Barracks; 60 s self-destruct not observed (idle matches end <60 s); Barracks.cpp logs "Lifetime (60 s) elapsed — self-destructing" |
| Deep Mine raises rate +2/s on placement | PARTIAL-PASS + DEFERRED | bot played Deep Mine (Rule 2); DeepMineIncome=2 bound (TASK-063), income registered silently (no rate-delta log); rate read not taken via MCP; QA PASS TASK-057 |

**Hero upgrades (interactive) — DEFERRED-to-Jonathan; WIRING = PASS**
| Line | Verdict | Backing |
|---|---|---|
| apply/stack/cap/refund + persist through death + reset on Play Again + HUD row updates | DEFERRED (wiring PASS) | delegate `FOnHeroUpgradesChanged` + `ApplyUpgrade`(refund `EHeroUpgradeResult`) + `ResetUpgrades` + `GetUpgradeStackCap` @65861ce; TASK-064 HUD seed-then-bind + boot-verified zero errors. No MCP card-inject → live pip/stack/refund is Jonathan's playtest |

**Deck — PASS**
| Line | Verdict | Evidence |
|---|---|---|
| M4 test deck deals 50/6 with Set II reachable | **PASS** | deck-build log at my session start: `UDeckComponent ... built a 50-card draw pile from 22 card rows` for BOTH `SiegeBotController_0` AND `SiegePlayerController_0`; Set II reachable proven by the bot playing the full Set II roster + discarding the upgrade cards |

**Bonus closes:** the real-PIE Defeat screen fired (`ASiegePlayerController: match ended — winner Red`) — closes the lingering M2 "no PlayerController to show end screen" WATCH (that only happened under Simulate). Match-end freeze contract PASS (`N units frozen, M towers silenced, K barracks frozen, P projectiles cleared, income paused, clock stopped, bot loop stopped`).

### Step 3 — Commit — done (see Commit section). NO push.

### Step 4 — Cold-boot transient-Blue-unit WATCH — **FAIL / FINDING (root-caused; follow-up for manager)**
Bounced to a fresh editor (moved `Saved/Autosaves/PackageRestoreData.json` aside → scratchpad, gitignored, no residue; force-killed PID 6172, relaunched detached; clean ~27 s boot, no Restore modal). On the cold load: **L_Arena is_dirty=false (clean vs disk) yet `find_actors` returns 3 stray actors baked into the committed L_Arena.umap:**
- `BP_Unit_Footman_C_1` (Blue) — the "transient Blue unit": at match start it advances on the Red castle and triggers the bot's Rule 1 (Defend) at t≈2 s.
- `BP_Unit_Miner_C_2` (Blue) — a stray economy unit.
- `BP_Building_ArrowTower_C_1` — a stray Blue tower.

**Root cause:** NOT a PlayAgain/world-teardown code bug. These are editor-verification actors placed during **M2** and saved into the level; `git log` shows L_Arena.umap last touched at **40b69ef / 5bb9507 (M2)**. The persistent level is therefore NOT clean — the M3 TASK-052 WATCH ("transient Blue unit at match start") = this committed residue, and it recurs on a cold boot.

**Not fixed here (deliberate):** TASK-069's scope + commit set do not include L_Arena, and the M-series "don't change the playtest environment without sign-off" gate applies. **Recommend a dedicated cleanup task** (build-master or gameplay-programmer): open L_Arena, remove the 3 stray actors (`BP_Unit_Footman_C_1`, `BP_Unit_Miner_C_2`, `BP_Building_ArrowTower_C_1`), re-save, PIE-verify a clean match start (bot opens with economy, no Rule 1 Defend at t=0), commit L_Arena.umap. Trivial, but it modifies the shipped map so it should be its own gated change.

### Recordable slice (Ogre push vs Bomb Tower defense)
PARTIAL: the **Ogre push is vividly recordable** — the bot-driven idle match is a full expanded-roster clip (repeated Ogre waves + Cavalry/Sapper/Pikeman/Longbowman/Cleric/MilitiaMob crushing the undefended castle). The "vs Bomb Tower defense" half needs an interactive defender placing a Bomb Tower (no MCP card-inject) → Jonathan's playtest.

## Net
- **All M4 FEATURES pass or are cleanly deferred — zero M4-feature FAILs.** Load/wiring, DT_Cards 22-row, deck 50/6, bot Set II slice (Ogre push + upgrade discards + gold-safe + Red-half), and match-end freeze/Defeat screen are PASS. Keyword/profile/new-building/hero-upgrade behaviors are DEFERRED-to-playtest, each backed by a specific code QA PASS + BP/data bind (MCP has no card-play/keypress injection to drive them deterministically in a live-ticking match).
- **One genuine follow-up FINDING:** committed L_Arena residue (3 stray M2 actors) = the transient-Blue-unit WATCH, now root-caused → recommend a cleanup task.
- Editor left UP (cold, clean, L_Arena loaded) for Jonathan's playtest.

## Constraints honored
No code edits. No TASKBOARD.md edit (orchestrator owns the board). No push. Donors untouched, Content/Dev/ ignored, no L_Arena save. Handoffs 061..069 committed.
