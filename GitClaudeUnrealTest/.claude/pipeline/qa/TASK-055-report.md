# QA Report — TASK-055

Verdict: **PASS**
Counts: 0 BLOCKER · 0 WARN · 3 NIT

Scope reviewed (pre-compile, files only):
- `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` / `.cpp` (NEW — `FSiegeCombatStatics::ApplyRadialDamage`)
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` / `.cpp` (ComputeOutputDamage, Charge, Slayer, aura hook, Sapper detonate, centralized OUTPUT)

Cross-checked dependency symbols: `ETeamId` / `ITeamAgent` / `UTeamAgent` (TeamId.h), `USiegeDamageType_Siege` / `_Projectile` (DamageTypes.h), `FCardRow::bCharge/bSlayer/bSuicide/AoERadius` (CardRow.h — all present, TASK-053), `GetMaxHP()` on Castle/Building/Hero/Unit, `AProjectile::InitProjectile(ETeamId, AActor*, float, TSubclassOf<UDamageType>)` (matches the new 2-arg `FireProjectileAt`).

---

## Critical confirmations (explicit)

**1. Non-keyword / no-aura NON-REGRESSION — CONFIRMED bit-for-bit.**
`ComputeOutputDamage` for a unit with `bCharge=false`, `bSlayer=false`, no aura:
`Output = AttackDamage;` → charge block skipped (`bCharge` false) → slayer block skipped (`bSlayer` short-circuits, `GetTargetMaxHP` never called) → `Output *= AuraDamageMultiplier` where `AuraDamageMultiplier == 1.0f`.
Multiplying an IEEE-754 float by exactly `1.0f` is an exact operation (no rounding), so the return is byte-identical to `AttackDamage`. All factors are `float` — no int/float mixing anywhere in the chain (`ChargeMultiplier`, `SlayerMultiplier`, `AuraDamageMultiplier` are all `float`). The melee path passes this into `ApplyDamage` and the ranged path into `FireProjectileAt(Target, OutputDamage)`→`InitProjectile(..., DamageAmount, ...)` — both identical to the pre-TASK-055 value. The only added shared-path code is one `TrackChargeMovement()` call at the top of `UpdateState`, which early-returns on `!bCharge` (line 1067). Footman/Archer/Knight/Ogre(Siege)/Cleric(Support)/miner melee+ranged bodies, the TASK-054 damage-TYPE selection `(Profile==Siege)?Siege:UDamageType`, the TASK-020 lunge, the TASK-042 move-speed buff, and the -90° VisualMesh rest pose are all untouched in behavior.

**2. PerformAttack centralization — CONFIRMED.**
`OutputDamage` is computed once (line 1020) and feeds BOTH branches: melee `ApplyDamage(Target, OutputDamage, GetController(), this, MeleeDamageType)` and ranged `FireProjectileAt(Target, OutputDamage)`. The ranged path now correctly carries the modified damage into the projectile. The TASK-054 Siege-vs-default damage-TYPE selection is preserved in the melee branch; Siege 200% is NOT re-applied in `ComputeOutputDamage` (correctly left fortification-side via the damage type), so no double-count.

**3. CHARGE single-hit — CONFIRMED (exactly one doubled hit per charge; never zero in the common case, never repeated).**
Prime: `TrackChargeMovement` accumulates `StateCheckInterval` while `State==Advance` and `velocity² > ChargeMoveSpeedThreshold²`; sets `bChargePrimed` at `>= ChargeMoveSeconds`. Stall (moving-but-slow while advancing) or `Idle` resets `ChargeMoveElapsed` + clears `bChargePrimed`. `State==Attack` deliberately leaves the flag untouched (avoids the same-tick reset race). Consume: `ComputeOutputDamage` applies `×ChargeMultiplier`, then `bChargePrimed=false` and `ChargeMoveElapsed=0` — so the very next hit reverts to base and a re-prime requires a fresh 2 s of movement. During sustained Attack `TrackChargeMovement` neither sets nor resets, so no re-prime/double-count. On first engagement `EnterAttack` fires `PerformAttack` immediately (FirstDelay ≤ 0 from `LastAttackTime = -1e9`), so the charged hit lands on the reach tick while the flag is still primed. See NIT-1 for the one timing caveat.

**4. SLAYER null-safety — CONFIRMED.**
`GetTargetMaxHP` (static) handles null→0, then `Cast` to ASummonedUnit / ACastle / ABuilding / AHeroCharacter → each `GetMaxHP()` (all const, all present); unknown→0 (no bonus). ATower resolves through the ABuilding cast (inherited GetMaxHP). Gate `bSlayer && GetTargetMaxHP(Target) >= SlayerHPThreshold(150)`; `bSlayer` short-circuits the HP read off the non-Slayer hot path. Matches acceptance: Pikeman(30) → 60 vs 200-HP Knight, 30 vs 80-HP Footman.

**5. DRIFT-FREE aura — CONFIRMED.**
`SetAuraDamageBonus` writes `AuraDamageMultiplier = 1.f + Bonus` DIRECTLY from the bonus (never off the buffed value) → refresh-not-stack; re-arms the single `AuraDamageBuffTimerHandle`. `EndAuraDamageBuff` resets to the literal `1.f`. The multiplier is stored SEPARATELY from the row-bound `AttackDamage` and composed only at strike time — the base is never mutated in place, so there is nothing to drift and expiry always restores exactly 1.0 (a stronger guarantee than ApplyMoveSpeedBuff's cache-once). Guards dead/frozen; non-positive Bonus clears immediately. `FreezeAI` calls `EndAuraDamageBuff()` (line 229); `EndPlay` clears the timer handle (line 162). Signature `void SetAuraDamageBonus(float Bonus, float Duration)` (BlueprintCallable) matches the TASK-058 War Banner seam exactly.

**6. ApplyRadialDamage NO friendly fire — CONFIRMED.**
The `Agent->GetTeamId() == Team` filter is the SOLE friendly-fire authority (Team = attacker's team) — it excludes same-team actors before `ApplyDamage` is ever called, independent of the receiver's instigator chain, so a null-controller caller (TASK-056 tower) still cannot friendly-fire. Distance is closest-point (`ActorGetDistanceToCollision(Center, ECC_Pawn, ...)`, actor-origin fallback on `< 0`) so a wall-adjacent blast reaches a large-footprint castle/building. Each hit routes through the target's own `TakeDamage`, so Siege 200% still fires castle/building-side. Null-safe: `World` guarded, `IsValid(Candidate)` guarded, `Radius<=0`/`Damage<=0` no-op, null type → base `UDamageType`. `AGoldNode` is not an ITeamAgent → never enumerated. The Sapper itself is same-team → excluded from its own blast.

**7. Sapper SINGLE detonation — CONFIRMED.**
`UpdateStateSiege` calls `Detonate()` (not `EnterAttack`) on reaching range → the attack timer never arms → no repeat hits. `HandleDeath` also calls `ApplyDetonation()` (before `bDead`/`Destroy`, so a valid location+controller is read) when `bSuicide`, so a Sapper shot down en route still explodes. `bDetonated` guards `ApplyDetonation` and `bDead` guards `Detonate`/`HandleDeath`:
- contact path: Detonate→ApplyDetonation(blast, sets bDetonated)→HandleDeath→ApplyDetonation returns early (bDetonated) — ONE blast.
- death path: HandleDeath→ApplyDetonation(blast)→Destroy — ONE blast; UpdateState is gated on `bDead` and timers are cleared, so no later contact trigger.
Values `AttackDamage`(80) and `AoERadius`(250) are DT_Cards-bound in `LoadStatsAndStart`; blast is `USiegeDamageType_Siege` (160 to castle/buildings, 80 to units).

---

## Findings

- **[NIT] SummonedUnit.cpp:1078 (TrackChargeMovement) / 505 (reach check)** — Charge "reach-tick" timing edge. On a *unit* target `EnterAdvance` stops at `max(AttackRange*0.8, 40)` with `bStopOnOverlap`, so a Cavalry that begins final deceleration on the exact tick its distance first crosses `AttackRange` could dip below `ChargeMoveSpeedThreshold(50 u/s)` and have `TrackChargeMovement` reset the prime one tick before the hit consumes it (a "zero" rather than a double). In practice the stop point (0.8×Range) sits *inside* AttackRange, so on the reach tick the pawn is still moving well above 50 u/s and the charge lands — and against structures it moves at full speed flush to the wall. No code defect; documented in the handoff. Recommend the playtest explicitly verify the "first post-charge hit = 40" acceptance against a *unit* target, not only a structure.

- **[NIT] SummonedUnit.cpp:1387 (ApplyDetonation)** — the Sapper blast uses raw row `AttackDamage`, NOT `ComputeOutputDamage`, so Charge/Slayer/aura do not compose into the one-shot AoE. This matches the spec ("Damage 80") and the handoff's documented interpretation (per-target Slayer is undefined for an AoE). Flagged only so design can revisit if an aura-buffed suicide is ever wanted (it would be the TASK-058 War-Banner interaction).

- **[NIT] SiegeCombatStatics.cpp:57-63** — the closest-point distance block is a new hand-mirror of `ASummonedUnit::GetDistanceToTarget` / `AProjectile::GetDistanceToTarget` (the qa/TASK-026 NIT-4 "three mirrors, never a fourth" item). The handoff acknowledges this and this very statics file is the natural consolidation home. Recommendation stands: on the wave that owns hero + projectile + unit together, fold all mirrors into a shared static HERE and delete the copies — do not add a fifth.

---

## Standard scan

- **C4457/58/59 shadow** — clean. New locals/params (`Output`, `Target`, `Unit/Castle/Building/Hero`, `World`, `OutputDamage`, `DamageAmount`, `Bonus`, `Duration`, and the statics-file `World/InstigatorController/Team/Center/Radius/Damage/DamageTypeClass` + `TeamAgents/Candidate/Agent/ClosestPoint/Distance`) shadow no member of ASummonedUnit nor any inherited `Owner`/`Instigator`/`Controller`/`PlayerState`. `FSiegeCombatStatics` is a member-less non-UObject, so its `Team` param cannot shadow a member. `SpawnParameters.Owner/Instigator` are struct-field writes, not declarations.
- **Deprecated UE 5.8 APIs** — none. `ActorGetDistanceToCollision`, `GetAllActorsWithInterface`, `ApplyDamage`, `GetWorldTimerManager().SetTimer/ClearTimer/IsTimerActive`, `FMath::Square`, `ECC_Pawn`, `UE_PI`, `UE_KINDA_SMALL_NUMBER`, `TSubclassOf`, `TNumericLimits` all current.
- **Header/cpp consistency** — every new declaration (`SetAuraDamageBonus`, `EndAuraDamageBuff`, `ComputeOutputDamage`, `TrackChargeMovement`, `Detonate`, `ApplyDetonation`, `GetTargetMaxHP`, and the widened `FireProjectileAt(AActor*, float)`) has a matching definition; the sole `FireProjectileAt` caller passes the new arg. `ApplyRadialDamage` decl (.h) and def (.cpp) match parameter-for-parameter. All new members declared in the header.
- **Includes** — `SiegeCombatStatics.cpp` includes World/Actor/DamageType/GameplayStatics/EngineTypes/TeamId (covers `UWorld`, `ActorGetDistanceToCollision`, `UDamageType`, `GetAllActorsWithInterface`+`ApplyDamage`, `ECC_Pawn`, `UTeamAgent`/`ITeamAgent`/`ETeamId`). `AController` is only forwarded (pointer pass-through — no full type needed). `SummonedUnit.cpp` adds `Siegebound/SiegeCombatStatics.h` (line 28) and already has `DamageTypes.h` for `USiegeDamageType_Siege`.
- **No Build.cs change** — correct: `FSiegeCombatStatics` is a plain non-reflected static in the same module using only already-linked Core/Engine deps.
- **GC safety** — no new UObject* raw member holding; `AuraDamageMultiplier`/charge state are plain scalars; `bDetonated` a plain bool. Timers cleared in EndPlay/FreezeAI/HandleDeath.

---

## Notes for build-master (if PASS)

- No new module dependency; no Build.cs edit. Part of the M4 batch compile (TASK-068). This is the SummonedUnit serial hub — TASK-056 links `FSiegeCombatStatics::ApplyRadialDamage` and TASK-058 calls `ASummonedUnit::SetAuraDamageBonus`, both of whose signatures are verified here and should compile against downstream unchanged.
- Playtest watch item (NIT-1): confirm a Cavalry's first post-charge hit deals 2× against a *unit* target (structure case is robust).
