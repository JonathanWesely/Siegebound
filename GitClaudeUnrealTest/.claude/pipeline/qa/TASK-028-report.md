# QA Report — TASK-028 — Summoned unit v2: ranged attacks + FreezeAI (C++)
Verdict: PASS

- reviewer: qa-reviewer
- date: 2026-07-03
- reviewed: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (both end-to-end — heightened-scrutiny dispatch: the implementing run was flagged "safety classifier unavailable")
- against: TASKBOARD TASK-028 spec + names block, "M2 manager decisions" (damage-typing ruling, execution gates), CONVENTIONS.md (card-stat law, damage-type registry, category/naming), handoffs/TASK-028.md (11 flagged decisions), handoffs/TASK-026.md + Projectile.h (AProjectile contract), handoffs/TASK-021.md + CardRow.h (bRanged semantics), qa/TASK-020-report.md + qa/TASK-004-report.md (audited M1 baseline guarantees), qa/TASK-026-report.md (rulings 1/3/10, NIT-4, WARN-1), DamageTypes.h, SLACK.md
- Blockers: 0 · Warnings: 0 · Nits: 2

## Headline concern — M1 Footman (bRanged=false) regression check: PASS

QA has no Git access, so "byte-identical" was verified structurally: every line-referenced landmark in the qa/TASK-020 and qa/TASK-004 audits was re-located in the current file and reconciled against the handoff's claimed insertions. All ~25 landmarks map at exactly the predicted offsets (+2 after the two new includes; +5 after the 3 BeginPlay comment lines; +43 after FreezeAI (31 lines + blank) + LoadStatsAndStart/UpdateState comment+gate lines + the bRanged bind; +44 inside PerformAttack; +92 after FireProjectileAt (32 lines + blank)) with unchanged content — e.g. TASK-020-era :495 ApplyDamage → :551, :507-510 stamp → :566-569, :547-551 lunge guards → :639-643, :685 death restore → :777, :748-756 origin fallback → :841-847. No unexplained insertion or deletion exists anywhere in either file.

Melee execution path, statement-by-statement (PerformAttack :511-570):
- Early-outs unchanged: `bDead || !bStatsLoaded` (+ always-false `|| bAIFrozen`) :514; `!IsTargetAlive(Target)` :520-523; range gate `> AttackRange` with the impact-point out-param :528-532; `FaceTarget(Target)` :534.
- Melee branch (else, :545-562): `ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass())` :551 — argument-for-argument the QA-020-audited call; `StartAttackLunge()` :557; puff gated on `DamageApplied > 0.f && CachedAttackImpactEffect` at the same `ImpactPoint` :558-561; unconditional `LastAttackTime` stamp :566-569.
- The only new instructions on a melee unit's executed path are three always-false/no-op boolean tests: `bAIFrozen` early-out terms (:189, :262, :514), `if (bRangedAttack)` :536 (false — Footman/Knight rows carry false per handoffs/TASK-021.md), and BeginPlay's `!bVisualMeshBaseCached` :95 (false at first BeginPlay; flag set :98).
- Untouched and verified untouched at fixed offsets: constructor, EndPlay, InitUnit, AcquireTarget (incl. pawn-preference tie-break), FindNearestEnemyCastle, EnterAttack cadence gate (FirstDelay honoring LastAttackTime :421-439), EnterAdvance (incl. both MoveToActor forms + Attack-exit StopAttackLunge :447), EnterIdle, FaceTarget, Tick, StartAttackLunge, StopAttackLunge, UpdateLunge, GetAIController, TakeDamage (friendly-fire mirror intact), TryGetDamageTeam, HandleDeath, IsTargetAlive, both GetDistanceToTarget bodies.
- Aggro 600 / leash 900 / tie-break 100 / 0.25 s interval: same pre-existing §3.8 UPROPERTY defaults, unmodified. DT_Cards reads: same single FindRow at bind time (:212); the one added bind is `bRangedAttack = Row->bRanged` :229. Grep confirms no stat-like literal added anywhere (Archer's 45/350/700/1.2/10 appear only in comments); the ranged path reuses row-bound AttackDamage/AttackRange/AttackCadence.

## FreezeAI contract (TASK-024 consumer): VERIFIED

- **Idempotent:** early-out on `bAIFrozen` :156.
- **bDead interaction:** early-out on `bDead` :156 — HandleDeath (:763-788) already performs a superset shutdown (both timers, StopAttackLunge, StopMovement, Destroy).
- **Permanent:** `bAIFrozen` gates LoadStatsAndStart :189, UpdateState :262, PerformAttack :514. Coverage is complete: both timers cleared :165-166; EnterAttack/EnterAdvance/EnterIdle are reachable only from gated UpdateState; StartAttackLunge only from gated PerformAttack's melee branch; a late InitUnit funnels into gated LoadStatsAndStart; a stray tick self-disables via UpdateLunge's `!bLungeActive` path :669-671. Nothing can restart a frozen unit.
- **Exact rest-pose restore mid-lunge:** unconditional `StopAttackLunge()` :170 → writes exactly `VisualMeshBaseRelativeLocation` and disables tick :658-664 — zero residual offset (TASK-020 zero-drift contract, drift structurally impossible: base-only writes).
- **StopMovement:** null-safe on the AIController :174-177 — aborts any in-flight move request, which is what also stops AMinerUnit's gold-node walk (base-class claim is sound: MoveToActor requests live on the controller).
- **Parks Idle:** direct `State = Idle` + `CurrentTarget/CurrentMoveGoal = nullptr` :180-182 (see ruling 2).
- **Never-bound units safe:** ClearTimer on unset handles is a no-op; the rest-pose cache exists from BeginPlay.
- **Frozen units stay damageable/destroyable:** TakeDamage ungated — spec freezes AI, not invulnerability. qa/TASK-026 WARN-1 carry-forward stands: in-flight AProjectiles at match end are TASK-024's cleanup.
- **Truly inert today:** grep across Source/ — `FreezeAI`/`bAIFrozen`/`IsAIFrozen` appear ONLY in SummonedUnit.h/.cpp (declaration, definition, comments). Zero callers until TASK-024; existing BPs predate the symbol and cannot invoke it.

## Ranged path: VERIFIED

- **Cadence honesty:** `LastAttackTime` stamped in the shared tail :564-569 for both delivery modes; EnterAttack's FirstDelay gate untouched — target swaps / range flapping cannot machine-gun ranged shots any more than melee.
- **FaceTarget before fire:** :534, shared, ahead of the branch.
- **Spawn:** `FireProjectileAt` :587-618 — null guards on World and Target :589-592; `Owner = this`, `Instigator = this` (:598-600 — the TASK-026 pawn-shooter rule; `this` is APawn-derived so receiver chain steps 1/3 resolve), `AlwaysSpawn` :603; spawn at `GetActorLocation()` scale-1 :608-612; spawn result null-checked via if-initializer :612; exactly ONE `InitProjectile(Team, Target, AttackDamage, USiegeDamageType_Projectile::StaticClass())` :616 — signature verified character-exact against Projectile.h:82; USiegeDamageType_Projectile verified in DamageTypes.h:39 (castle-side 50% per the M2 damage-typing ruling — nothing scaled unit-side, correct).
- **No per-shot sync loads:** FireProjectileAt performs zero asset resolution (the projectile's own per-spawn soft-ref lookups are TASK-026's already-accepted NIT-2, outside this task's files).
- **No lunge / no melee puff on ranged shots:** StartAttackLunge and the NS_Damage puff live exclusively in the melee else-branch — spec-exact ("the projectile + its impact effect are the telegraph").
- **Range/aggro/leash:** shared row-Range gate :529 (Archer 700 from the row), aggro 600 / leash 900 untouched; qa/TASK-026 ruling 1 honored — projectiles are not ITeamAgent, so acquisition correctly gained no arrow filtering.
- **UE 5.8 API:** `FActorSpawnParameters` Owner/Instigator/SpawnCollisionHandlingOverride, `ESpawnActorCollisionHandlingMethod::AlwaysSpawn`, `UWorld::SpawnActor<T>(UClass*, FTransform const&, const FActorSpawnParameters&)`, `FTransform(FRotator, FVector)` — all current, nothing deprecated. Includes `Siegebound/DamageTypes.h` + `Siegebound/Projectile.h` inserted alphabetically, both files exist and are qa-passed. UFUNCTION virtual BlueprintCallable + BlueprintPure inline getter are UHT-legal; new members follow house style (`bRangedAttack` UPROPERTY Transient stat-mirror; `bAIFrozen` plain bool, bDead style — POD, no GC concern).

## Findings

1. **[NIT]** SummonedUnit.cpp:609-610 vs handoffs/TASK-028.md flagged decision 7 — the handoff says the initial aim is a "flat vector to target", but the code does NOT flatten Z (`ToTarget = Target->GetActorLocation() - MuzzleLocation` — full 3D, includes pitch; contrast FaceTarget :580 which zeroes Z). This is the single handoff/code divergence found in the sweep. Behaviorally inert — the projectile re-aims at the target's current location every tick (TASK-026), so the spawn rotation is cosmetic for at most one frame, and the degenerate fallback to GetActorRotation() :610 is intact. Recording to correct the paper trail; no code change requested.
2. **[NIT]** SummonedUnit.cpp:612-617 — a null return from SpawnActor (engine-exhaustion tier under AlwaysSpawn) is a fully silent skip. Accepted per flagged decision 9 (cadence keeps ticking; no cadence-rate log spam), but a one-shot warning flag (the `bWarnedNoAIController` pattern) would make a systemic spawn failure diagnosable. Fold into the next SummonedUnit.cpp touch — do NOT respin.

Closed this cycle: **qa/TASK-020-report.md WARN-1** — the `!bVisualMeshBaseCached` BeginPlay guard (:95-99) is exactly the first-listed fix, applied per that report's build-master note 4 ("fold into the next SummonedUnit.cpp touch"). First BeginPlay unaffected; EndPlay left byte-identical.

## Flagged-decision rulings (handoffs/TASK-028.md — all 11, by number)

1. **bAIFrozen gates on LoadStatsAndStart/UpdateState/PerformAttack — PASS.** Verified at :189/:262/:514; restart-surface coverage is complete (see FreezeAI section); always-false short-circuit terms until FreezeAI runs — provably inert for M1 units. The "idles until destroyed" hard contract justifies gating beyond timer-clearing.
2. **FreezeAI parks State=Idle + nulls CurrentTarget/CurrentMoveGoal via direct writes — PASS.** Direct writes are actually required, not just preferred: EnterIdle's `State == Idle` early-return (:496-499) would skip the work on an already-idle unit, and EnterIdle never clears CurrentTarget at all — a frozen unit reporting Attack/a stale target through GetUnitState() would be a debugging trap. Also sidesteps QA-020 NIT-1's early-return concern entirely (StopAttackLunge is called unconditionally).
3. **FreezeAI early-outs on bDead — PASS.** HandleDeath runs the identical-plus shutdown in the same frame; freezing a dying actor would only set a flag on a corpse. Side effect (IsAIFrozen() stays false during the destroy frame) is harmless and consistent with "True once FreezeAI ran".
4. **virtual + BlueprintCallable FreezeAI; IsAIFrozen() getter — ACCEPTED.** One symbol beyond the names block ("function FreezeAI"), recorded per the handoff's own flag. Virtual is load-bearing for TASK-025's documented extension ("must also stop its walk"; plain C++ override, Super first — correctly documented); BlueprintCallable matches the InitUnit surface TASK-024 may drive; the getter follows the IsUnitDead/GetUnitState/GetTeam (TASK-026) PIE-hook precedent. No respin.
5. **WARN-1 fold-in as the guard form — PASS.** Exactly the option qa/TASK-020 WARN-1 listed first and the only zero-executable-path-change form for M1 (EndPlay byte-identical). WARN-1 closed.
6. **Delivery fork after the shared early-outs/range-gate/FaceTarget — PASS.** Ranged inherits the exact audited melee gating (target-alive, row Range, cadence honesty) — the strongest possible reading of "same state machine". The engage-600/fire-to-700/leash-900 consequence is the spec's own acceptance sentence; the 0.8×Range move acceptance (560 vs 700 for Archer) even provides re-engage hysteresis. No thrash path found.
7. **Spawn at GetActorLocation(), no muzzle-offset UPROPERTY — ACCEPTED.** "At the unit" character-exact; capsule center is sane for a blockout archer; initial aim cosmetic (projectile re-aims per tick). See NIT-1 for the handoff's "flat vector" wording inaccuracy — behavior fine, paper trail corrected here.
8. **AProjectile::StaticClass() spawned directly, no TSubclassOf knob — ACCEPTED.** Spec/names say "uses AProjectile"; a designer class property is unspecced and trivially additive later. Consistent with the tower-side contract shape in handoffs/TASK-026.md.
9. **Failed SpawnActor = silent skip, LastAttackTime still stamped — ACCEPTED.** Under AlwaysSpawn a null return is world-teardown/exhaustion tier; stamping keeps the cadence gate honest rather than machine-gunning retries; log-spam avoidance is legitimate. NIT-2 asks for a one-shot warning on a future touch.
10. **NIT-4 consolidation deferred with a TODO — PASS.** qa/TASK-026-report.md NIT-4/ruling 10 explicitly scheduled consolidation for "when SummonedUnit.h ownership next frees up" via a wave owning all three files (HeroCharacter.cpp + frozen Projectile.cpp + a new shared file — all outside this task's SummonedUnit-only scope). The header TODO (:314-317) records the debt and the "never add a fourth mirror" law; verified no fourth mirror was created (the ranged path reuses the existing member helper through the shared range gate).
11. **CachedAttackImpactEffect still resolved at BeginPlay for ranged units — ACCEPTED.** BeginPlay resolves the effect before LoadStatsAndStart binds the row (bRangedAttack unknowable at resolve time); reordering melee BeginPlay flow to skip it would violate the byte-identical rule for a one-lookup saving on an already-resident asset (QA-020 ruling 5 economics). BP archer children retain the IsNull designer opt-out.

## Out-of-scope edit sweep (heightened scrutiny — "safety classifier unavailable" run)

**Result: CLEAN.** The diff surface of the two in-scope files is exactly what handoffs/TASK-028.md claims, and nothing else.

- **Method:** full end-to-end read of both files; landmark-offset reconciliation against the qa/TASK-020 + qa/TASK-004 line-referenced audits (every landmark at its predicted offset, insertion inventory fully enumerated: 2 includes, 3 BeginPlay comment lines + 1 guard term, FreezeAI 31 lines, 3 comment lines + 1 gate term + 1 bind in LoadStatsAndStart, 2 comment lines + 1 gate term in UpdateState, PerformAttack branch restructure + comments + 1 gate term, FireProjectileAt 32 lines, header: doc bullets/enum comment/FreezeAI/IsAIFrozen/FireProjectileAt decl/TODO/bRangedAttack/bAIFrozen); symbol grep across Source/ (new symbols exist nowhere outside these two files).
- **No suspicious constructs:** no file/network/process access, no exec or console-command hooks, no new soft paths (constructor byte-consistent with the QA-020 audit), no CVars, no new delegates or timers, no reflection oddities, no include beyond the two required project headers. All content changes trace to the spec, the 11 flagged decisions, or the QA-directed WARN-1 fold-in. The one handoff/code text divergence is NIT-1 (benign).
- **Claimed-untouched files:** CardRow.h/Projectile.h/DamageTypes.h re-read this session and consistent with their own qa-passed audits; SummonedUnit.* contains zero references to DeckComponent/Building/Tower/DefaultEngine.ini.
- **Adjacent observation (NOT this task's diff, for the orchestrator):** `Tower.h`, `Tower.cpp`, and `Building.cpp` now exist on disk while the board's TASK-027 note still says "Building.h with NO Building.cpp, no Tower files" — TASK-027's resumed agent has evidently progressed past that note (Tower.cpp has its own unrelated `FireProjectileAt`; name collision is class-private, harmless). Not attributable to TASK-028. Reconcile the TASK-027 board note before its QA dispatch.
- **Caveat (standing):** QA has no Git access; final byte-level confirmation is build-master's live `git diff` (below). The conversation-start git snapshot visible to this QA session was stale (it listed none of the M2 source files that demonstrably exist), so do not trust cached status output.

## Notes for build-master

1. **Batch compile at TASK-039:** TASK-028 compiles only alongside TASK-026 (Projectile/DamageTypes includes) and TASK-021 (CardRow.h bRanged) — all qa-passed in the working tree. NOTE: UBT globs all of Source/, and Building.cpp/Tower.h/Tower.cpp now exist — do NOT run the batch compile until TASK-027 is qa-passed too, or the batch fails on unreviewed code.
2. **Live git-diff sanity check (heightened-scrutiny gate):** SummonedUnit.h/.cpp hunks must match handoffs/TASK-028.md §Files exactly; run a fresh `git status` (QA's snapshot was stale). Flag any extra hunk in ANY file back to QA before compiling.
3. **PIE verification (TASK-039/040):** melee regression first — footman vs Red castle: 12/hit at 1.0 s, lunge + puff, exact rest pose after sustained attacking (must be indistinguishable from the running M1 build). Then Archer acceptance after TASK-031/034: 45 HP / 350 speed, engages at 600, fires every 1.2 s from ≤700, projectile 10 dmg (5 vs castle), leash 900, NO lunge/melee puff on shots. FreezeAI is verifiable only after TASK-024 lands (IsAIFrozen is the PIE hook).
4. Nits require no action this cycle; NIT-2's one-shot warning folds into the next SummonedUnit.cpp touch (same convention as prior WARN fold-ins).
5. Commit message when this integrates: mention "closes qa/TASK-020 WARN-1".

## Board status (for the orchestrator to proxy — QA did not edit TASKBOARD.md)

`- status: qa-passed (qa/TASK-028-report.md PASS — 0 blockers / 0 warnings / 2 nits; 11/11 flagged decisions ruled; heightened-scrutiny out-of-scope sweep CLEAN; closes qa/TASK-020 WARN-1; compile gated on TASK-039 batch)`

## Slack

Verdict summary posted to ⚙️ Dev & QA (channel C0BF0QZP3CN, thread 1783116269.740549), prefix `🔍 QA:` — see thread for ts; if the post is absent, this report section is the fallback text of record.
