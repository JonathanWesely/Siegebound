# QA Report — TASK-253 (AGoldNode → neutral depleting claimable mine, branch C++)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-22. LAWS-UNCHANGED review mode; STATIC ONLY (tree intentionally
does not compile until TASK-254/255/256 land — no compile expectations, TASK-258 batches it).
Files: `GoldNode.{h,cpp}` on the `m7.6-arena10x` checkout. Reviewed against the W1-PREP appendix-2
board spec, CONVENTIONS "Mirrored depleting mines" + the lane amendment, plan §1a, and
handoffs/TASK-253-programmer.md. Board untouched.

Blockers: 0 · Warnings: 0 · Nits: 2

## 1. THE LAWS — held byte-faithfully (the review mode's core mandate) ✓

All verified in the constructor/lifecycle block (GoldNode.cpp:15-43):
`SetCollisionProfileName(NoCollision)` + `SetCollisionEnabled(NoCollision)` +
`SetGenerateOverlapEvents(false)` + `SetCanEverAffectNavigation(false)` (:35-38);
`SetCanBeDamaged(false)` (:24); NO ITeamAgent (plain AActor, with the acquisition-scan rationale
in the class doc); soft `/Game/Meshes/SM_GoldNode.SM_GoldNode` resolve (:42) with the
OnConstruction-silent (:52) / BeginPlay-warn-once (:61, `bWarnedMissingMesh`) split and the
IsNull designer-opt-out preserved; Movable-mobility rationale kept; `bCanEverTick = false` (:19 —
drain is timer-only). The ResolveNodeMesh body matches the qa-passed original's shape (same-mesh
no-op check included). The corridor-mines-allowed ruling's dependency (NoCollision keeps
traversability safe) is explicitly documented at the law block.

## 2. Claim atomicity — no wedge path ✓

- `TryRegisterArrivedMiner` orders correctly: IsValid gate → **CompactArrivedMiners FIRST**
  (:133) → `CanTeamMine` → Contains-idempotency (prevents double-drain) → 0→1 claim + timer.
- **The phantom-claim question, traced:** a claim can only exist alongside a running drain timer
  (claim set exclusively at 0→1 next to `StartDrainTimer`; released at all three empty sites —
  Unregister, compaction, Deplete — each of which also stops the timer). So if every occupant
  dies silently (no unregister), the stale weak entries hold a phantom claim for AT MOST one
  drain interval: the next `HandleDrainTick` (≤1 s away, guaranteed armed) compacts → empties →
  releases claim + stops timer inside `CompactArrivedMiners`. Any earlier contact heals faster:
  an arriving enemy miner's `TryRegister` compacts first and takes the mine over immediately.
  **The claim cannot wedge.** Residual transient: `FindBestMineFor` may classify a
  phantom-claimed mine as tier-2 for up to that same ≤1 s window (it calls `CanTeamMine` without
  compacting — a const/static path shouldn't mutate) — self-heals on the same clock, and the
  arriving miner's registration succeeds regardless. Recorded as expected behavior, not a defect.
- Drain accounting: stale sweep BEFORE the count (:242-244) — dead miners never drain; the
  documented ≤n-gold independent-clock tolerance matches the CONVENTIONS law text.

## 3. Deplete() ordering + re-entrancy ✓

Order verified (:264-307): idempotence guard → **latch + zero + stop timer** → **snapshot
(`MoveTemp`) + registry Reset + claim Reset** → per-miner `NotifyMineDepleted` → `OnMineDepleted`
broadcast → gauge ember → one Log line. Re-entrancy is genuinely safe: a notified miner calling
back `UnregisterArrivedMiner` hits the empty-registry early-return (:165-168, zero mutation);
`FindBestMineFor` skips the mine on `bDepleted` (:202); `TryRegister` refuses via `CanTeamMine`.
Iteration runs over the LOCAL snapshot, never the live array — no mutation-during-iteration is
possible even in principle. Eviction fires only for still-valid miners, once each (the registry
is duplicate-free by the Contains guard). The handoff's pinned call-site guarantees for TASK-254
match the code exactly.

## 4. FindBestMineFor — tier logic + determinism ✓

Depleted/empty mines skipped early; tier-1 = `CanTeamMine` nearest; the refused-but-live remainder
is necessarily enemy-occupied (the only remaining refusal cause) = tier-2 wait target; tier-1
beats tier-2 at any distance; nullptr = all-depleted endgame — exactly the spec. Determinism on
ties: strict `<` keeps the first-found candidate, and `TActorIterator` order is stable within a
fixed world — no churn between equal options across polls in a match (the mirror-symmetric
equal-distance case resolves consistently). 2D metric matches the miner arrival metric
(hill-placed mines don't skew "nearest"); `DistSquared2D`'s double return explicitly cast
(:211), `TNumericLimits<float>::Max()` init — no truncation warning.

## 5. Timer lifecycle ✓

`EndPlay`: stop timer + registry/claim reset, deliberately no notifications (ClearScatter/
Play-Again teardown rationale documented — miners are already dead per plan; stragglers re-seek
on their poll). `InitMine`: unconditional stop+reset (safe pre-BeginPlay — `StopDrainTimer`
null-guards the world), re-latch, derive `bDepleted`, gauge + broadcast. `StartDrainTimer`: no
re-arm mid-cycle (`IsTimerActive` guard — claim churn can't stretch the drain window, keeping
the income-clock pairing honest). A depleted mine can never re-arm (registration refused). The
looping handle cannot outlive the actor.

## 6. The five deviations — adjudicated

1. **SiegePlayerState.h cross-note deferred — ACCEPT (correct lane discipline).** The file is
   main-lane FROZEN this batch; violating the freeze for a comment would be the real error.
   GoldNode.h carries the full paired-tunable law text naming `MinerGoldPerTick`, cites
   CONVENTIONS, and records the reconcile destination inline (:218-220). **Merge-gate checklist
   gains the PlayerState-side one-liner** (joins SiegeBotController.h:247 from TASK-248).
2. **EditDefaultsOnly on `DrainPerMinerPerSecond` — ACCEPT.** Matches its pair's edit level; a
   per-instance override would silently break the paired law on one mine.
3. **Extra accessors — ACCEPT.** Additive, null-risk-free; `GetOccupyingTeam` correctly C++-only
   (TOptional not reflectable — not a UFUNCTION).
4. **`static constexpr` 1.0 s cadence — ACCEPT, the STRONGER reading of the law.** The paired
   tunable pairs per-second VALUES against 1.0 s ticks on both ends; a tunable cadence would add
   a third degree of freedom that could break the pairing while both "values" still matched.
   Hard-pinning one side enforces the law. [NIT-1] the residual: if
   `ASiegePlayerState::GoldTickInterval` is itself a tunable on the frozen main-lane class, the
   Phase-6 cross-note should pair the CADENCE too, not just the per-tick value — add that to the
   same merge-gate one-liner.
5. **Category rename → `Siegebound|Mine` — ACCEPT.** Cosmetic; the two level instances'
   serialized `Team` property drops silently on load (standard removed-property behavior) and
   both instances are deleted at TASK-257 anyway.

## 7. Pinned cross-task contracts — accurate, and COMPLETE ✓

Grep-verified all three GetTeam callers at exactly the claimed sites — `MinerUnit.cpp:452`
(inside `FindNearestSameTeamGoldNode`, dies at TASK-254), `BattlefieldScatter.cpp:656`
(RebuildKeepClearZones gold block, deleted at TASK-255), `SiegeBotController.cpp:1017`
(`GetGoldNodeRedLocation`, deleted at TASK-256) — and confirmed NO hidden fourth caller (the
remaining `GetTeam()` hits are AProjectile/ASiegePlayerState members, unaffected). The pinned
`AMinerUnit::NotifyMineDepleted(AGoldNode*)` declaration matches the call at :296. The
known-breakage list is exhaustive.

## Hygiene ✓

Includes complete for every used type (TimerManager, MaterialInstanceDynamic, MinerUnit —
GetTeamId + the pinned call, EngineUtils, World, CollisionProfile; header: TimerHandle, Optional,
TeamId). Shadow scan clean (Team member gone; house `World` locals; static finder). UPROPERTY
hygiene good (Transient on the latched denominator + MID; ClampMin where meaningful;
BlueprintPure queries; BlueprintAssignable delegates — the `FOnMine*` names deviate from the
generic `FOn<Owner><Event>` pattern but are the EXACT names the board spec + CONVENTIONS clause
pin, so the specific law wins). Gauge is genuinely never load-bearing: every guard is a silent
no-op, divide-by-zero guarded, lazy-MID retry handles the deferred mesh resolve, intensity
modulation only (glows-regardless-of-team law held). The TASK-257 coordination flag (GlowIntensity
must be authored as a NEW default-1.0 multiplicative scale) is a genuinely useful disambiguation
of the CONVENTIONS wording — endorse it.

## Findings

- [NIT-1] (see deviation 4) Phase-6 merge-gate cross-note should pair the drain/income CADENCE
  alongside the per-tick value.
- [NIT-2] The ≤1 s phantom-claim window (silent-death → next drain tick) can transiently
  misclassify a mine as tier-2 in `FindBestMineFor`. Self-healing and harmless (registration on
  contact always heals instantly); recording so TASK-254's wait-mode review doesn't rediscover
  it as a surprise.

## For downstream

- **TASK-254:** the API contract as documented is exactly what the code does; rely on the
  Deplete() call-site guarantees verbatim. NIT-2 is the one transient to know about.
- **TASK-255:** `InitMine` is safe before or after BeginPlay (verified); EndPlay covers the
  ClearScatter teardown.
- **TASK-258 (batch compile):** expected breakage list confirmed complete — the three callers +
  the missing `NotifyMineDepleted` declaration are the only compile blockers by inspection.
- **Merge gate:** SiegePlayerState.h cross-note (value + cadence) joins the checklist.
