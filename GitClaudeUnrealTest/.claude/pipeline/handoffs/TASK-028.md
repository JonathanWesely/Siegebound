# TASK-028 Handoff — Summoned unit v2: ranged attacks + FreezeAI (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no board edits — M2 wave 2 gates while Jonathan is away; orchestrator proxies the status flip to ready-for-qa)

## Files

1. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — class doc (+ranged/FreezeAI bullets), Attack enum comment, public `virtual FreezeAI()` + `IsAIFrozen()` (BlueprintCallable/Pure), private `FireProjectileAt(AActor*)`, members `bRangedAttack` (UPROPERTY Transient, house stat-mirror style) + `bAIFrozen` (plain bool, bDead style), qa/TASK-026 NIT-4 consolidation TODO on `GetDistanceToTarget`.
2. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — two includes (`Siegebound/DamageTypes.h`, `Siegebound/Projectile.h`), BeginPlay rest-pose-cache guard (WARN-1 fold-in), `bRangedAttack = Row->bRanged` bind, `bAIFrozen` gates on the three restart surfaces, PerformAttack delivery branch, `FreezeAI()` + `FireProjectileAt()` bodies.

**NOT touched (verified/deliberate):** CardRow.h, Castle.*, Projectile.*, DamageTypes.* (frozen upstream contracts — read-only), Build.cs (Niagara already present; nothing new needed — AProjectile/DamageTypes are project classes), DeckComponent (TASK-022's files), Building/Tower/DefaultEngine.ini (TASK-027's files), no TASKBOARD/CONVENTIONS edits.

## What changed

1. **bRanged read (TASK-021 contract):** `LoadStatsAndStart` binds `bRangedAttack = Row->bRanged` alongside the existing stat binds. Footman/Knight rows carry `false`; Archer `true` (handoffs/TASK-021.md).
2. **Ranged delivery:** `PerformAttack` branches on `bRangedAttack` AFTER the shared early-outs, target-alive check, range gate (row Range — Archer 700), and `FaceTarget`. Ranged branch calls `FireProjectileAt(Target)`: spawns `AProjectile` at `GetActorLocation()` with `Owner = this`, `Instigator = this` (the TASK-026 pawn-shooter rule, so receiver no-friendly-fire checks resolve our team), `AlwaysSpawn`, scale-1 transform aimed at the target (cosmetic — the projectile re-aims per tick), then exactly one `InitProjectile(Team, Target, AttackDamage, USiegeDamageType_Projectile::StaticClass())`. No lunge, no melee puff on the ranged path — the projectile + its impact VFX are the telegraph. `LastAttackTime` is stamped for ranged shots exactly like melee hits, keeping the EnterAttack cadence gate honest for both modes. State machine, aggro 600, leash 900, acquisition, tie-break: untouched (per qa/TASK-026 ruling 1, projectiles are not ITeamAgent, so acquisition needs no arrow filtering — none added).
3. **FreezeAI (TASK-024 contract — see next section):** permanent, idempotent AI stop.
4. **qa/TASK-020-report.md WARN-1 fold-in (QA-directed):** BeginPlay's rest-pose cache is now guarded with `!bVisualMeshBaseCached` — the exact one-line fix WARN-1 named first; QA's build-master note 4 instructed folding it into "the next SummonedUnit.cpp touch", which this is. First BeginPlay unaffected (flag starts false); only the M1-unreachable stream-out re-BeginPlay path changes (no longer recaptures a lunging pose as base).

## FreezeAI contract (what TASK-024 consumes)

```cpp
// ASummonedUnit public API (SummonedUnit.h):
UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
virtual void FreezeAI();          // call once per unit at match end; extra calls are no-ops

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
bool IsAIFrozen() const;          // PIE/QA verification hook

// TASK-024 match-end freeze usage:
for (TActorIterator<ASummonedUnit> It(GetWorld()); It; ++It) { It->FreezeAI(); }
```

Semantics:
- Clears the state (acquire) and attack timers; cancels any in-flight lunge via `StopAttackLunge()` (VisualMesh restored to EXACTLY the cached rest pose, actor tick disabled — zero residual offset); `StopMovement()` on the AIController (aborts whatever move request is in flight — this is what also stops AMinerUnit's gold-node walk, TASK-025); parks `State = Idle`, clears `CurrentTarget`/`CurrentMoveGoal`.
- **Permanent:** `bAIFrozen` additionally gates `LoadStatsAndStart`, `UpdateState`, and `PerformAttack`, so nothing (including a late `InitUnit` on a never-bound unit) can restart a frozen unit. It idles until destroyed.
- Idempotent; early-outs on `bDead` (a dying unit already ran the identical shutdown in `HandleDeath`). Safe on never-bound (idle) units — ClearTimer on unset handles is a no-op.
- Virtual: `AMinerUnit` (TASK-025) can extend it for any subclass-specific timers; the base already stops its walk.
- Frozen units remain damageable/destroyable (spec says freeze AI, not invulnerability); note qa/TASK-026 WARN-1 carry-forward — in-flight AProjectiles at match end are TASK-024's cleanup, not this class's.

## How Footman (bRanged=false) behavior is preserved — the headline QA concern

Exhaustive list of every behavioral delta on the melee path, each provably inert:

- `PerformAttack`: for `bRangedAttack == false` the statement sequence is character-identical to the TASK-020-audited code — FaceTarget → `ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass())` (same args) → `StartAttackLunge()` → puff gated on `DamageApplied > 0` at the same `ImpactPoint` → unconditional `LastAttackTime` stamp. The only new instructions executed are two always-false boolean tests (`bAIFrozen` in the early-out, `if (bRangedAttack)`).
- `LoadStatsAndStart`: one added assignment (`bRangedAttack = Row->bRanged` — false for Footman) and an always-false `bAIFrozen` early-out term. All other binds, warnings, timer start: untouched.
- `UpdateState`: always-false `bAIFrozen` early-out term only. Leash/acquire/advance/attack transitions untouched.
- `BeginPlay`: `!bVisualMeshBaseCached` guard — false at first (and in M1 only) BeginPlay, so the cache executes exactly as before (WARN-1 fold-in above).
- `bAIFrozen` is false for every unit until TASK-024 ships and calls `FreezeAI` at match end — nothing in the current codebase calls it (grep: definition + this handoff only). FreezeAI is additive and inert.
- Untouched functions (zero diff): constructor, EndPlay, InitUnit, AcquireTarget, FindNearestEnemyCastle, EnterAttack, EnterAdvance, EnterIdle, FaceTarget, Tick, StartAttackLunge, StopAttackLunge, UpdateLunge, GetAIController, TakeDamage, TryGetDamageTeam, HandleDeath, IsTargetAlive, both GetDistanceToTarget bodies.
- DT_Cards reads: same single `FindRow` at bind time; no new table reads; no stat-like literals added anywhere (the ranged path reuses row-bound `AttackDamage`/`AttackRange`/`AttackCadence`).

## Flagged decisions for QA (please rule on each)

1. **`bAIFrozen` gates added to LoadStatsAndStart/UpdateState/PerformAttack early-outs** (beyond clearing the timers). Rationale: "unit idles until destroyed" is a hard contract — the gates make the freeze survive a late `InitUnit` on a never-bound unit and any hypothetical timer re-arm. Inert until FreezeAI runs (always-false bool test).
2. **FreezeAI also parks `State = Idle` and nulls `CurrentTarget`/`CurrentMoveGoal`** (spec lists stop/clear/cancel only). A frozen unit reporting `Attack` through `GetUnitState()` with a stale target pointer would be a debugging trap; direct member writes rather than `EnterIdle()` so the freeze does not depend on EnterIdle's State-equality early-return semantics.
3. **FreezeAI early-outs on `bDead`** — HandleDeath already performs the identical shutdown; freezing a dying actor would only set a meaningless flag during the destroy frame.
4. **FreezeAI is `virtual` + BlueprintCallable; `IsAIFrozen()` getter added.** Neither is in the names block (which names only "function FreezeAI"). Virtual is for TASK-025's documented "must also stop its walk" extension point; the getter is the house PIE-verification pattern (IsUnitDead/GetUnitState precedents). One symbol beyond spec — flag if unwanted.
5. **WARN-1 fold-in chose the guard form** (`if (VisualMesh && !bVisualMeshBaseCached)`) over adding `StopAttackLunge()` to EndPlay — the option WARN-1 listed first and the only one with literally zero executable-path change in M1 (EndPlay left byte-identical).
6. **Ranged branch placement:** the delivery fork sits after the SHARED early-outs/range-gate/FaceTarget, so ranged units inherit the exact melee gating (target-alive, row-Range check, cadence). Consequence accepted per spec ("same state machine"): an Archer whose target leaves 700 stops firing but only drops the target beyond the 900 leash — GDD §3.8's engage-at-600/fire-to-700/leash-900 reading.
7. **Projectile spawn point = `GetActorLocation()`** (capsule center, ~chest height), aim = flat vector to target (falls back to actor rotation when degenerate). Spec says "at the unit"; no muzzle-offset UPROPERTY invented. Initial aim is cosmetic — AProjectile re-aims at the target's current location every tick (TASK-026).
8. **`AProjectile::StaticClass()` spawned directly** — no `TSubclassOf<AProjectile>` designer property (spec/names say "uses AProjectile"; adding a class knob is not specced — trivially added later if a task asks).
9. **Failed `SpawnActor` (engine-exhaustion tier, AlwaysSpawn) is a silent skip and still stamps `LastAttackTime`** — cadence keeps ticking rather than machine-gunning retries; no per-shot warning to avoid cadence-rate log spam.
10. **NIT-4 consolidation NOT performed; TODO left** (qa/TASK-026-report.md ruling 10 / NIT-4 permits either). Consolidating the three closest-point mirrors requires touching HeroCharacter.cpp and the frozen Projectile.cpp plus a new shared file — all outside this task's SummonedUnit-only file scope. No fourth mirror created: the ranged path reuses the existing member helper (the shared range gate already yields everything needed).
11. **Ranged units still resolve `CachedAttackImpactEffect` at BeginPlay** even though they never spawn it (BeginPlay runs before the row is known). One already-resident-asset lookup per spawn — the same cost every melee unit pays; skipping it would require reordering BeginPlay (byte-identical-flow rule wins). Archers simply never use it.

## Downstream notes

- **TASK-024:** consume `FreezeAI()` per the contract block above; remember qa/TASK-026 WARN-1 (destroy in-flight `AProjectile`s at match end/PlayAgain — the freeze does not cover them).
- **TASK-025 (AMinerUnit):** base `FreezeAI` stops MoveToActor walks; override it (plain C++ override, no UFUNCTION re-declaration) if the miner adds its own timers, and call `Super::FreezeAI()` first. Also note `bAIFrozen` gates `LoadStatsAndStart` — a frozen-then-Init'd unit stays inert by design.
- **TASK-034 (BP_Unit_Archer):** no new BP assignments needed for ranged — `bRanged` comes from the row via DT_Cards after the TASK-031 reimport; the archer needs only CardID=Archer + SM_Archer like any unit BP.
- **Build/QA:** compiles only alongside TASK-026's files (includes Projectile.h/DamageTypes.h) — both already qa-passed in the working tree; batch-compile order at TASK-039 is safe (no include into TASK-022/027 files, none of theirs into these).
- Not compiled (standing gate — batch compile after round-2 sign-off, TASK-039). TASKBOARD deliberately not edited (orchestrator owns board writes for this dispatch).
