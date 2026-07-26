# TASK-216 handoff — Phase-0 C++: constants sweep + bot castle-relative spawn + NavSettleDelay→poll (gameplay-programmer, 2026-07-18)

M7.6 lane note (TASK-214 law): edits made in the SHARED working tree, NO checkout, NO commits — TASK-218 checks out `m7.6-arena10x` and commits these paths by explicit pathspec. NOT compiled here (TASK-218 compiles; Smart-App-Control law). Numeric truth: plan §1c/§1d (`i-am-a-bit-sunny-bird.md`).

## Files touched (the only Source/ deltas in the tree — verified via `git diff --stat -- Source/`)
- `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h`
- `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` (the BotCenterlineSpawnX spawn commits live here — inseparable from the ruling-#1 header change)

## 1) Constants sweep — every value changed, old → new
| File / symbol | Old | New |
|---|---|---|
| ScatterConfig.h `ArenaHalfExtent` | (8600, 3200) | **(26000, 12000)** |
| ScatterConfig.h `CastleKeepClearRadius` | 900 | **1500** |
| ScatterConfig.h `GoldNodeKeepClearRadius` | 500 | **600** |
| ScatterConfig.h `PlayerStartKeepClearRadius` | 700 | **800** |
| ScatterConfig.h `CorridorHalfWidth` | 400 | **1000** (ruling #2 — C++/DA disagreement ends; TASK-218 sets the DA to match) |
| BattlefieldScatter.cpp castle fallbacks (:529/:533) | ±8000 | **±25000** |
| BattlefieldScatter.cpp node fallbacks (:559/:563) | ±7200 | **±24200** |
| BattlefieldScatter.cpp PlayerStart fallback (:581) | −6800 | **−23800** |
| BattlefieldScatter.cpp `ResolveCastleLocation` fallback (:757→now :820) | ±8000 | **±25000** |
| BattlefieldScatter.cpp no-config corridor fallback (RebuildKeepClearZones, was 400) | 400 | **1000** (not on the plan's line list — it is the same ruling-#2 constant's no-config mirror; leaving it at 400 would desync the fallback path) |
| BattlefieldScatter.h `CorridorWidenStep` | 250 | **400** |
| SiegeBotController.h `CastleRedFallbackLocation` | (8000,0,0) | **(25000,0,0)** |
| SiegeBotController.h `GoldNodeRedFallbackLocation` | (7200,0,0) | **(24200,0,0)** |
| SiegeBotController.h `BotCenterlineSpawnX` (350) | mid-field knob | **REMOVED — replaced by `BotCastleSpawnOffset` = 1750** (see §2) |
| BattlefieldScatter.h `NavSettleDelay` (0.75) | fixed delay | **REMOVED — replaced by the nav-idle poll** (see §3) |

KEEP-list audit (untouched, verified): CastleQueryInset 1200; ground-trace ±50000; BotSpawnLaneSpread 900; TowerDefenseStandoff 750; NavProjectionExtent; SwarmSpawnRadius 300 (comment refreshed, value kept); CastlePlinthClearance 420; BuildingClearance 200; MinerNodeApproachOffset 400; MaxReachabilityAttempts 5; MaxPlacementAttemptsPerInstance 24; HeroSpawnCastleOffset (SiegeGameMode untouched); all cards.csv/combat data; Config/DefaultEngine.ini (TASK-217's file); Tools/reimport_meshes.py (TASK-220's file).

Stale comments describing old values updated in all five files (±8000/±7200/−6800 mentions, "centerline" spawn wording, SwarmSpawnRadius rationale, class docs).

## 2) Bot castle-relative attack spawn (Jonathan ruling #1)
- New knob `BotCastleSpawnOffset = 1750.f` (EditDefaultsOnly, ClampMin 0) replaces `BotCenterlineSpawnX = 350.f`. 1750 = midpoint of the spec's ~1,500–2,000 band; clears the 420 plinth keep-out comfortably.
- Both former mid-field sites now compute: `CastleRed = GetCastleRedLocation()` (LIVE actor resolve, fallback (25000,0,0)), then `Desired = CastleRed + (TowardCenterSign * BotCastleSpawnOffset, FRandRange(±BotSpawnLaneSpread), 0)`, `Desired.Z = CastleRed.Z`; `TowardCenterSign = (CastleRed.X >= 0) ? -1 : +1` (robust to a mirrored/moved castle; same sign convention as the miner approach). Existing `ComputeValidBotSpawnPoint` (navmesh projection + own-half + plinth + ring search) is unchanged downstream.
  - Rule 4 ATTACK (SiegeBotController.cpp ~:667) — the mid-field wave commit the ruling targets. Log line now says "castle-front … marching (M7.6 ruling #1)".
  - Rule 1 DEFEND, unit branch (~:450) — SCOPE NOTE for QA: this site shared the SAME `BotCenterlineSpawnX` knob. The acceptance requires "no mid-field materialize path left", so it moved castle-front too (a defender materializing in front of its own castle is strictly more defensive than mid-field; leaving it would have left a mid-field materialize path AND a dead knob). Defensive BUILDING placement (intruder-standoff geometry), miner placement, and tower-standoff logic are byte-identical.
- Ruling + the flagged "adaptive bot spawn positioning by strategy" Standing-backlog follow-up are commented at the knob and the rule-4 site. First contact ~9 min accepted "for now" — W1 (TASK-219) sanity-checks pacing live.

## 3) Nav settle poll (replaces the fixed 0.75 s wait)
- New: `StartNavSettlePoll()` + `PollNavSettle()` on the SAME `TraversabilityTimerHandle` (one timer at a time; EndPlay's existing ClearTimer covers both — the file's timer pattern).
- Flow: GenerateScatter → `ReachabilityAttempt=0; StartNavSettlePoll()`. Poll checks `UNavigationSystemV1::IsNavigationBeingBuilt(World)` every `NavPollInterval` (0.25 s, ClampMin 0.05) until idle → `ValidateTraversability()`. Cap `MaxNavSettleWait` (10 s): still building ⇒ warn on LogSiegeTerrain + proceed anyway (never strands the guarantee). No nav system at poll-start ⇒ single fixed `NavSettleFallbackDelay` (2.0 s) wait then validate (which itself degrades gracefully without nav — unchanged).
- The widening-cull retry inside `ValidateTraversability` also re-enters via `StartNavSettlePoll()` (waits for the post-cull re-carve to settle instead of a blind 0.75 s).
- False-idle guard: the first poll fires one interval AFTER the scatter/cull (never same-frame), by which time the dirty-area rebuild has registered — commented at the call site.
- All three knobs are EditDefaultsOnly UPROPERTYs under `Siegebound|Terrain|Traversability`. Removing the old `NavSettleDelay`/`BotCenterlineSpawnX` UPROPERTYs is serialization-safe (stale values on the L_Arena instance are silently dropped on load; the bot is spawned as the raw C++ class — no BP subclass overrides exist, verified via SiegeGameMode.cpp:48).

## QA scrutiny pointers
- Shadow scan: hoisted `const FVector CastleRed` in rule 1 replaced the building-branch local of the same name (no duplicate declaration remains — verified by read-back); no C4458 UPROPERTY shadowing introduced.
- Includes: no new headers needed — `NavigationSystem.h` (GetCurrent/IsNavigationBeingBuilt) and `TimerManager.h` already included in BattlefieldScatter.cpp.
- Known residual stale comments OUT of scope (other files, main-lane — flagged, not edited): GoldNode.h (±7200/±8000 doc), SiegeGameMode.h/.cpp (±8000/−6800 doc), SiegePlayerController half-comments. Comment-only; no functional constants. Manager may fold into the Phase-6 merge-gate sweep.
- Compile deferred to TASK-218 (Phase-0 integration) per spec.
