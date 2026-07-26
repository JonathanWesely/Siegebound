# TASK-256 — [T-D] Bot: mine-aware economy rules (handoff)

**Author:** gameplay-programmer · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x` (SiegeBotController.{h,cpp} branch-owned per the M7.6 ownership extension)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`, `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp` — nothing else.
**NOT compiled** (TASK-258 batch compile, by spec). Depends on TASK-253's `AGoldNode::FindBestMineFor` (already in the working tree).

## What changed

### Deleted (plan §1e)
- `FVector GetGoldNodeRedLocation() const` — declaration, doc, and definition (was `SiegeBotController.cpp:1010`, the last `AGoldNode::GetTeam()` caller flagged in the TASK-253 handoff).
- `FVector GoldNodeRedFallbackLocation` UPROPERTY (+24,200 fallback). Grep-verified: zero references remain anywhere in `Source/`. NOTE: if any Blueprint subclass ever serialized an override of this property, it drops silently on next load (same benign pattern as TASK-253's removed `Team`).

### Rule 2 (ECONOMY) rewritten — the ONLY rule touched
Both sub-rules now sit inside a `if (UWorld* World = GetWorld())` scope that computes, ONCE per rule-2 tick:
`AGoldNode* BestMine = AGoldNode::FindBestMineFor(World, BotTeam, GetCastleRedLocation())` — THE single finder (tier-1 nearest mineable, tier-2 nearest enemy-occupied wait target, null = all depleted/none), exactly as the spec pins.

**Rule 2a (Miner):** gate is now `BestMine && AliveMinerCount < TargetMinerCount && CanAddMiner()` — the two TASK-046 gates byte-for-byte PLUS the mine gate.
- **Finder-null ⇒ 2a skipped ENTIRELY** (never buy a doomed miner). Falls through to 2b, then rules 3–5, unchanged.
- Spawn point: `MineLocation + normalize2D(CastleRed − MineLocation) × MinerNodeApproachOffset` (400uu short of the mine on its own-castle side), `Z = mine Z`, then **own-half clamp** (`Desired.X = BotHalfBoundaryX` when off-half), then the existing `ComputeValidBotSpawnPoint` ring/nav machinery — untouched.
- Degenerate guard: mine exactly at the castle point ⇒ zero-length approach vector ⇒ Desired = mine location; the existing widening ring walks it clear of the plinth keep-out.

**Rule 2b (Deep Mine):** anchored at the SAME `BestMine` result (same own-half clamp), else fallback `CastleRed + towardCenterSign × BotCastleSpawnOffset` (castle-front). Deliberately NO finder-null skip — a Deep Mine needs no gold node, so it stays the bot's all-depleted endgame economy (plan §T-F(7): base + overtime + DeepMine keep the endgame winnable). Card search, spawn call, clearance handling all byte-identical.

### Skip logging — once per state change, never per tick
New private latch `bool bLoggedMineLockout = false` (header-documented):
- finder null + latch clear → ONE `LogGitClaudeUnrealTest` Log line ("rule 2a SKIPPED … never buy a doomed miner"), latch set;
- finder non-null + latch set → ONE recovery line ("mine '%s' available again — rule 2a re-enabled"), latch cleared.
- Both lines are deliberately OFF LogSiegeBot (the M3 one-LogSiegeBot-line-per-FIRED-rule law — a skip is a non-decision diagnostic, per the LogSiegeBot header doc).
- Latch NOT reset in `ResetBot()` (kept out of scope): it self-heals — the first rule-2 tick of a fresh match observes the re-scattered mines and logs one truthful recovery line. Transitions occurring while an intruder camps the half are logged late (on the next clear tick) — the latch state is only observable when rule 2 evaluates; accepted.

### Decision-trace (LogSiegeBot) changes — still exactly one line per fired rule
- Rule 2a line now names the target: `played Miner '%s' (cost %d) toward mine '<name>' at (x,y,z) — miners now n/m, gold a->b`.
- Rule 2b line now names its anchor: `… near mine '<name>' …` or `… castle-front (no available mine) …`.
- Rules 1/3a/3b/4/5 trace lines byte-identical.

### Comment sweep
- Header rule-2 summary (EvaluateDecisions doc), `TargetMinerCount` doc, `MinerNodeApproachOffset` doc rewritten to best-available-mine language; the one remaining "GoldNode_Red" string is the deliberate historical note "formerly GoldNode_Red-relative" in the offset doc.
- cpp rule-2 banner documents the finder tiers and the TASK-257 node deletions.

## Load-bearing interpretation (QA please ratify) — the own-half clamp is EXPLICIT
The dispatch says the spawn point goes "through the existing ComputeValidBotSpawnPoint (own-half clamp yields cross-field walks…)". As built, `ComputeValidBotSpawnPoint` REJECTS off-half candidates (`IsBotHalfPointClear`) rather than clamping, and its ring tops out at 1,100uu — an unclamped Blue-half desired point (mines sit at |X| ≥ 1,500, most far deeper) would fail every candidate and stall rule 2a FOREVER, breaking T-F(9) ("bot reaches 3 arrived miners") whenever the best mine is Blue-half. So I pre-clamp `Desired.X` to `BotHalfBoundaryX` before the call: the miner spawns at the centerline-nearest own-half point and its own AI (TASK-254 FindBestMineFor retarget) WALKS it cross-field to the mine — which is precisely the documented-correct cross-field behavior. ComputeValidBotSpawnPoint itself is untouched.

## Deviations (each justified)
1. **2b fallback offset = `BotCastleSpawnOffset` (1,750).** Plan says "castle − X·offset" without naming the knob. `MinerNodeApproachOffset` (400) would put the desired point INSIDE the plinth keep-out (420 half-extent); `BotCastleSpawnOffset` is the class's one castle-front anchor knob and clears every keep-out. 
2. **Recovery line added** (lockout release). "Log once per state change" reads as both edges; a lockout line with no matching recovery would make T-F(9) log-reading ambiguous. Both edges latch-guarded — never per-tick.
3. **2b also own-half clamps its mine anchor** (spec only spells the clamp out for 2a). Without it a Blue-half best mine would stall 2b exactly like 2a; a building parked across the centerline would also violate the never-spawn-on-Blue-half law.

## Self-audit (performed)
- `git diff` hunk audit: my cpp hunks are exactly the rule-2 block + the GetGoldNodeRedLocation deletion; hunks @214/@243/@259 (TASK-252 deck edits) and @626 (Lightning AoERadius comment) are the PRE-EXISTING uncommitted branch state, untouched. TASK-216's castle-spawn rework (rules 1/4 castle-front geometry) untouched.
- Rules 1, 3a, 3b, 4, 5, defense/intruder scan, spell target finders, `ComputeValidBotSpawnPoint`, `IsBotHalfPointClear`, `SpawnBotCardActor`, `ResolveBotCardActorClass`, `ResetBot` — byte-identical.
- No shadowing: rule-2's `World` local is scoped inside `if (!NearestIntruder)` and dies before rule 3's own `if (UWorld* World = GetWorld())`; house `UWorld* World` pattern (CONVENTIONS C4458 law).
- Includes: `Siegebound/GoldNode.h` already included (line 17) — now used for the static finder; no include changes needed.
- `FString AnchorDesc` built BEFORE the UE_LOG (no ternary-in-varargs lifetime games).

## QA pointers
- Verify the 2a gate order can't buy a miner when `BestMine == nullptr` (the SKIP semantics) and that 2b still fires in that state.
- Verify one-per-state-change latch logic (both edges) and that neither line is on LogSiegeBot.
- Verify approach math: 2D normalize, Z from mine, `IsNearlyZero` degenerate branch, clamp AFTER offset.
- FindBestMineFor contract per handoffs/TASK-253-programmer.md (tier-1/tier-2/null; From = castle location — matches the spec's `FindBestMineFor(BotTeam, castle)`).
