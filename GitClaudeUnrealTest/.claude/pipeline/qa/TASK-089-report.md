# QA Report — TASK-089
Verdict: PASS

Reviewer: qa-reviewer, 2026-07-08. Scope: pre-compile inspection of the gold-balance change
(StartingGold 10 + base income 1 gold per 2 s) against the TASKBOARD chain spec (10 points,
manager rulings 1–6) and handoffs/TASK-089.md. Files reviewed: SiegePlayerState.h,
SiegePlayerState.cpp, SiegeGameMode.cpp (3 comment hunks), plus READ-ONLY context
DeepMine.h, SiegeGameMode.h, SiegeGameState.h. QA has no Git access by design — byte-identity
claims were verified by content inspection + historical pipeline quotes; the true diff is
re-checkable by build-master at commit time (carry-forward CF-7).

Counts: 0 BLOCKER / 1 WARN / 3 NIT. All five flagged decisions ACCEPTED.

## Spec compliance (10-point contract)

1. **StartingGold 10 + Gold init 10, keep-in-sync comments** — PASS. h:274 `StartingGold = 10`
   and h:341 `Gold = 10`, reciprocal keep-in-sync comments on both (h:272, h:339).
   ResetGold() (cpp:79) seeds from StartingGold — Play Again needs no change, confirmed.
2. **GoldPerTick 1, redefined doc** — PASS. h:278 `GoldPerTick = 1`; doc (h:276) redefines it
   as per-BASE-INCOME-GRANT, notes the overtime doubling, keeps GDD §3.2 "(amended)" + the
   2026-07-08 directive tag.
3. **NEW BaseIncomeTickPeriod** — PASS. h:280-282: `UPROPERTY(EditDefaultsOnly,
   Category = "Siegebound|Gold", meta = (ClampMin = "1")) int32 BaseIncomeTickPeriod = 2;`
   — specifier/category/clamp/default character-for-character per the names block.
   GoldTickInterval untouched at 1.0f (h:286).
4. **HandleGoldTick decomposition** — PASS (cpp:121-161). bIncomePaused gate kept (cpp:126,
   and it returns BEFORE the counter increment, so a restarted-while-paused timer cannot
   advance parity — good). Per-tick grant = MinerIncomeCount*MinerGoldPerTick +
   FlatIncomePerTick every tick; base added only on the grant tick with the overtime latch
   read LIVE inside the grant branch (cpp:152-154 — resolved per-grant, not per-tick; minor
   perf plus). Exactly ONE SetGold per tick (cpp:160); the only `Gold` assignment in the
   .cpp is inside SetGold (cpp:116) — choke-point law intact; zero-grant tick is a no-op
   no-broadcast (SetGold change detection). Comments state GetGoldRate is no longer called
   for accrual, and it genuinely is not.
   Cadence math verified: period 2 → counter 1,2(grant,zero),1,2(grant)… = exactly +1 per
   2 ticks pre-overtime, +1×2=+2 per 2 ticks in overtime. Period 1 (legacy) → counter hits
   threshold every tick = every-tick grant, correct. Mid-match period shrink → `>=` re-syncs
   on the very next tick with a single grant (see F1); period growth → next grant lands when
   the counter reaches the new threshold, no double grant possible (stored counter range is
   [0, period-1]).
5. **Counter reset determinism** — PASS. `BaseIncomeTickCounter = 0` in ResetEconomy
   (cpp:351) with placement rationale documented. Verified the ACTUAL Play Again order in
   SiegeGameMode.cpp: ResetClock (L583) → per-PS ResetEconomy (L605) → ResetGold (L606,
   restarts timer) → ResumeIncome (L607, SetTimer replaces same-frame — no phase drift):
   first post-reset base grant lands exactly on tick BaseIncomeTickPeriod (~2 s). Boot path:
   member initializer 0 + BeginPlay→ResetGold→StartIncomeTimer → first grant at t≈2 s.
   Explicit ruling requested by the dispatch: there is NO path producing a double grant or a
   3-second first grant post-reset. A hypothetical ResumeIncome-without-ResetEconomy caller
   would resume mid-parity (worst case: grant 1 tick after resume) — that is a legitimate
   continuation of a paused phase, and no such caller exists in the shipped flow.
6. **GetGoldRate display semantics** — PASS (cpp:173-198, h:137-153). `FMath::
   DivideAndRoundUp(EffectiveBase, BaseIncomeTickPeriod)` with EffectiveBase = GoldPerTick ×
   (overtime? multiplier : 1), latch read live. Defaults: (1,2)→1 pre-OT, (2,2)→1 OT exact.
   Edge configs: GoldPerTick 3/period 2 → pre-OT 2 (true 1.5, rounded up), OT (6,2)→3 exact;
   GoldPerTick 0 → 0. Value is purely state-derived (no tick parity input) — stable, never
   alternates, RefreshGoldRate change detection unaffected. Round-up display law stated in
   both header and cpp docs. DivideAndRoundUp is valid, non-deprecated UE 5.8 API for int32.
7. **Delegate/API contract** — PASS. FOnGoldChanged DECLARE (h:17) is character-identical to
   the line quoted in handoffs/TASK-005.md; FOnGoldRateChanged DECLARE (h:30) carries the
   historical `int32, NewRate` contract (no historical byte quote exists in the pipeline —
   see CF-7). SpendGold/AddGold/AddIncome/RemoveIncome/PauseIncome/ResumeIncome bodies match
   their documented pre-change behavior exactly. Zero UMG/widget files in scope.
8. **Explicitly unchanged** — PASS by inspection: GoldTickInterval 1.0f (h:286),
   OvertimeIncomeMultiplier 2 (h:298), MinerGoldPerTick 1 (h:302), MaxGold 999 (h:290),
   MaxActiveMiners 6 (h:294), DeepMine.h DeepMineIncome 2 (L66), SiegeGameMode.h
   SandboxStartingGold 9999 (L271).
9. **Comment hygiene** — PASS on the enumerated list (class doc h:42-61 rewritten to the
   10/1-per-2-s economy; cpp accrual/display/reset comments corrected; SiegeGameMode.cpp
   hunks at L104-106, L575-580, L865-871 are genuine `//` lines — a Grep rendering artifact
   initially mimicked stray backslashes; raw Read confirmed clean bytes). See WARN-1 for
   now-false comments the enumeration missed.
10. **Shadow law (C4457/58/59)** — PASS, independent scan (see F4).

## Findings

- [WARN] Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp:596,613,627 — three now-false
  gold comments survive in a file this task touched for comment hygiene: L596 "ResetGold (back
  to 50; restarts income — M1 law)", L613 "the bot's gold-to-50", L627 "ResetGold'd every
  player state back to 50" (StartingGold is now 10). Not a spec violation — spec point 9
  enumerated only ~106/~579/~867 and those were done — but it is inconsistent with the F3
  hygiene standard applied elsewhere. Comment-only, zero compile/runtime risk, does NOT block.
  Fix: fold "50 → StartingGold (10)" into the next programmer touch of this file (or a
  zero-risk comment fixup if the orchestrator wants a clean snapshot before TASK-090 commits).
- [NIT] Now-false gold comments in files this task was RIGHT not to touch (outside the spec's
  file list): SiegeGameMode.h:105 ("back to 50"), SiegeGameMode.h:341 ("normal +2/s economy
  stands"), SiegeBotController.h:106 ("gold back to 50"), SiegeGameState.h:41-42 ("reads
  IsOvertimeActive() LIVE in GetGoldRate(), so accrual can never desync" — the accrual live
  read now lives in HandleGoldTick; GetGoldRate is display). Log for the next touch of each.
- [NIT] SiegePlayerState.cpp:190 — the /0 guard on DivideAndRoundUp rests solely on ClampMin
  metadata (editor-input-only; a C++ subclass or future Config exposure could still hand it 0).
  Optional hardening: divide by `FMath::Max(1, BaseIncomeTickPeriod)`. Default 2 + no config
  flag makes this theoretical today.
- [NIT] Pathological-editor-value overflow (GoldPerTick near INT32_MAX × multiplier) is
  unguarded — pre-existing exposure, unchanged by this task, SetGold's MaxGold clamp bounds
  the stored result. No action.

## Flagged-decision rulings (F1–F5)

- **F1 (`>=` counter threshold): ACCEPT** — behavior-identical to `==` in every reachable
  flow (stored counter never exceeds period-1 under a fixed period); it self-heals a live
  period shrink with exactly one grant instead of stalling base income forever. It masks no
  reset bug: both reset paths (initializer, ResetEconomy) zero the counter, and the paused
  gate precedes the increment so parity cannot advance while frozen.
- **F2 (FOnGoldRateChanged doc prose): ACCEPT** — spec 7's "byte-identical" binds the
  DECLARE/signature contract, which is intact; spec 6 MANDATES documenting the display
  semantics, and the delegate prose was the now-false line. Leaving "gold added per income
  tick" would have shipped a lying contract comment.
- **F3 (extra stale-comment corrections, values untouched): ACCEPT** — OvertimeIncomeMultiplier
  confirmed 2 (h:298); every corrected comment was factually false post-change; same
  rationale as F2. (WARN-1 notes where this standard was not carried through.)
- **F4 (shadow self-scan): ACCEPT — independently confirmed.** BaseIncomeTickPeriod /
  BaseIncomeTickCounter collide with no reflected member on the APlayerState/AActor lineage
  (Score/PlayerId/PlayerNamePrivate/bIsABot/StartTime; Owner/Instigator/Tags/…). Locals
  TickGrant/EffectiveBase/BaseRate/SiegeGameState/bOvertime shadow nothing; the
  SiegeGameState-never-GameState local naming is the established C4458-avoidance pattern
  (reaffirmed at SiegeGameMode.cpp:853-855). Edited function bodies scanned clean.
- **F5 (worktree residue not mine): ACCEPT** — pre-existing residue adjudicated into TASK-090
  by ruling 9; nothing in it belongs to this diff. Note: QA's session git snapshot showed
  WBP_MainMenu.uasset dirty but did NOT show the BP_Unit_Footman/Archer deltas F5 lists —
  reconcile at the bounce window (CF-7).

## Notes for build-master (carry-forwards for TASK-090 — runtime/Git proofs QA cannot do)

- CF-1: PIE boot — gold seeds 10; base drip +1 exactly every 2 s over ≥10 s; exactly ONE
  OnGoldChanged per grant tick (choke-point proof).
- CF-2: HUD "+1/s" pre-overtime is EXPECTED (ruling-4 round-up display; true base 0.5/s) —
  record, do not flag.
- CF-3: Overtime 7:00 — base becomes +2 per 2 s (=1/s); the overtime INDICATOR fires while
  the rate text stays "+1/s" in a miner-less economy. The SILENT OnGoldRateChanged is correct
  behavior (display base 1→1), not a missed broadcast.
- CF-4: Miner arrival — +1 every 1 s tick immediately (independent of base cadence); rate
  text +2/s. DeepMine +2/s unchanged.
- CF-5: Play Again — gold 10; FIRST post-reset base grant lands ~2 s after reset (this is the
  runtime proof of the ResetEconomy counter reset); match-end income freeze unregressed.
- CF-6: Sandbox — AddGold(9999) clamps to 999 (pre-existing, unchanged; log line says so).
- CF-7: Git-side verification at commit time — confirm via `git diff` that (a) the
  SiegeGameMode.cpp hunks are comment-only (my inspection says yes but QA has no Git), (b)
  neither delegate DECLARE line appears in the diff (byte-identity proof), (c) the worktree
  residue matches ruling 9 expectations (QA's snapshot and F5 disagreed on the BP_Unit_*
  deltas — reconcile before the single commit).
- CF-8: Bot-rush re-measure per ruling 7 (prior marks ~48 s / ~33 s) — the slower shared
  economy should move this number; record in the ledger.
- WARN-1 is NOT a commit blocker; it may ride as-is (comments only).
