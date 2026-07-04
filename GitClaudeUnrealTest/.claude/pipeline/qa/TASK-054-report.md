# QA Report — TASK-054

**Verdict: PASS**
**Counts: 0 BLOCKER · 0 WARN · 3 NIT**
Pre-compile gate (M4 batch compile at TASK-068). Files reviewed, not built.

Scope: `DamageTypes.h/.cpp` (USiegeDamageType_Siege), `Castle.h/.cpp` (Siege 200%),
`Building.h/.cpp` (Siege 200%), `SummonedUnit.h/.cpp` (Siege + Support profiles, heal
system, FreezeAI extension). Cross-checked `CardRow.h` (ECardProfile), `cards.csv`
(Ogre/Sapper/Cleric/Miner rows), `MinerUnit.h` (Profile=None seal).

## Findings

- [NIT] SummonedUnit.cpp:951 — The Siege damage-TYPE tag is selected only on the melee
  branch of `PerformAttack`. A future `Profile=Siege` card with `bRanged=true` would fire
  a `USiegeDamageType_Projectile` shot and NOT get the 200% fortification scaling. No
  current card is affected (Ogre/Sapper are both melee, `bRanged=false`), so this is a
  forward-looking note only. If a ranged Siege card ever ships, route its projectile type
  through the same Profile check.
- [NIT] SummonedUnit.cpp:754 vs :788 — `StartHealing` arms the heal timer at
  `FMath::Max(SupportHealInterval, 0.05f)` while `PerformHeal` heals
  `AttackDamage * SupportHealInterval` (raw). The two only diverge if `SupportHealInterval`
  drops below 0.05, which the UPROPERTY `ClampMin=0.05` prevents — so the 8 HP/s rate is
  exact today. Cosmetically the per-tick amount could reuse the clamped value for
  belt-and-suspenders consistency.
- [NIT] SummonedUnit.cpp:1194 (`IsTargetAlive`) — building targets are not checked against
  `IsBuildingDestroyed()` (they fall to the "unknown ITeamAgent = alive" branch). Harmless:
  `IsValid` closes the post-Destroy window, `ABuilding::TakeDamage`'s `bDestroyed` guard
  zeroes any same-frame hit, and the Siege re-scan filters `IsBuildingDestroyed()` each
  0.25 s check. Pre-existing M2 pattern for tower/wall targets — no regression.

## Interpretation-call rulings (2)

1. **Siege targets nearest enemy building MAP-WIDE (no aggro gate) — ACCEPT.**
   Spec §3.8 / TASK-054 says "nearest enemy ABuilding in path, else the enemy castle."
   Reading "in path" as "nearest map-wide" is sound: in the standard arena the nearest
   enemy structure is the forward wall/tower on the Ogre's line to the castle, satisfying
   the acceptance criterion ("walks past units, hits the first wall/tower then the castle").
   The only divergence is a far off-axis building that happens to be marginally closer — a
   minor tactical wrinkle, correctly flagged for playtest. Accept.

2. **Support fallback follows COMBAT units only (Standard OR Siege; not Support/Miner) —
   ACCEPT.** Spec says "follow the nearest friendly COMBAT unit." Excluding other Clerics
   and Miners matches the literal wording and prevents healer-trailing-healer / healer-to-
   backline-miner loops. Note: a DAMAGED friendly of any profile (incl. a hurt Miner or
   Cleric) is still healed and followed when in range — the combat-only filter applies to
   the follow FALLBACK only, which is correct. Accept.

## Non-regression confirmation (highest priority)

- **Standard / melee / ranged units — CONFIRMED byte-preserved.** The Profile dispatch is
  the first thing in `UpdateState` (SummonedUnit.cpp:393–402) and `return`s before the
  untouched Standard body for Profile==Siege/Support. Profile==Standard and Profile==None
  (miners) fall straight through to the unchanged M1/M2 body. `Profile` is bound in
  `LoadStatsAndStart` (line 355) BEFORE the one synchronous `UpdateState()` (line 375), so
  the first decision already routes correctly and the Miner's single synchronous update
  runs the Standard body exactly as in M3.
- **PerformAttack damage-type — CONFIRMED identical for non-Siege.** The new ternary
  (line 951) yields `TSubclassOf<UDamageType>(UDamageType::StaticClass())` for every
  non-Siege profile — the exact class M1 passed. Melee/lunge (TASK-020), ranged/projectile
  (TASK-028), the -90° VisualMesh yaw, and the TASK-042 move-speed buff are all untouched.
- **Miner (AMinerUnit) — CONFIRMED unaffected.** Profile=None → Standard body; the
  `StateCheckInterval=0` / `AggroRadius=0` / `ClearAllTimersForObject` seals are unchanged.
  No edit to MinerUnit.
- **Castle scaling — CONFIRMED byte-preserved.** New `IsChildOf(USiegeDamageType_Siege)`
  → ×2.0 `if` sits BEFORE the M2 Projectile ×0.5 `else if`; Siege and Projectile are
  disjoint direct subclasses of UDamageType, so order is cosmetic. Melee/default stays
  ×1.0; the M2 projectile-50% branch and the ScaledDamage return are unchanged.
- **Building scaling — CONFIRMED correct.** Single `IsChildOf(USiegeDamageType_Siege)`
  → ×2.0 branch added; every other type (incl. Projectile) still takes LISTED damage
  (projectile-50% stays castle-only per the M2 ruling). `TakeDamage` now applies and
  returns `ScaledDamage`; for all non-Siege damage `ScaledDamage == ActualDamage`, so HP
  math and the return value are byte-identical for every M2/M3 attacker. Only Siege hits
  report/apply 2×. All return-value consumers (`SummonedUnit::PerformAttack` puff gate,
  `Projectile` impact) are unaffected for non-Siege.

## Profile verification

- **SIEGE (Ogre/Sapper):** `UpdateStateSiege` ignores all pawns — acquisition is
  `FindNearestEnemyBuilding()` (TActorIterator<ABuilding>, skips friendly via
  `GetTeamId()==Team` and destroyed via `IsBuildingDestroyed()`; covers ATower and every
  ABuilding subclass at runtime) → else `FindNearestEnemyCastle()`. No friendly-building
  targeting. CurrentTarget recomputed each check (re-routes on wall fall / new placement).
  Melee tags `USiegeDamageType_Siege`. Data confirms Ogre/Sapper Profile=Siege, bRanged=false.
- **SUPPORT (Cleric):** `UpdateStateSupport` never calls EnterAttack — the attack timer is
  never armed; State only ever Advance/Idle. `FindNearestDamagedFriendly` = nearest friendly
  ASummonedUnit with `CurrentHP<MaxHP` within AttackRange(400), self excluded, enemies
  excluded, unbound 0/0 units excluded. Heal math: `AttackDamage(8) × SupportHealInterval(0.1)`
  = 0.8/tick at 10 Hz = 8 HP/s, rate-invariant; `ApplyHealing` clamps to MaxHP (no overheal);
  `PerformHeal` re-validates alive/friendly/damaged/in-range every tick. Follow = damaged
  friendly else `FindNearestFriendlyCombatUnit` (Standard/Siege only); lone Cleric → EnterIdle.
  Data confirms Cleric Profile=Support, Damage=8, Range=400.
- **FreezeAI:** calls `StopHealing()` + nulls `SupportHealTarget` (lines 229–230); Siege
  pathing / Support following stop via the `StateTimerHandle` clear + `StopMovement`.
  `HealTimerHandle` cleared in `EndPlay` (line 160) and `HandleDeath` (via StopHealing,
  line 1178). `PerformHeal`/`ApplyHealing` gated on bDead/!bStatsLoaded/bAIFrozen. No
  residual healing/pathing after freeze.

## Standard checks

- **C4457/C4458/C4459 shadow scan — CLEAN.** New locals (World, MyLocation, Structure,
  HealTarget, FollowGoal, Unit, Building, Best*, Distance, Amount, MeleeDamageType,
  IncomingDamageType, ScaledDamage, InstigatorTeam/AttackerTeam) none shadow inherited
  reflected members (Owner/PlayerState/Instigator/Controller) or the class member `Team`.
  `SpawnParameters.Owner/.Instigator` are struct-field writes, not locals.
- **Deprecated UE 5.8 APIs — NONE.** New code uses only stable APIs (TActorIterator,
  UGameplayStatics::ApplyDamage, DamageEvent.DamageTypeClass.Get(), UClass::IsChildOf,
  GetWorldTimerManager, TWeakObjectPtr). No deprecated/removed calls introduced.
- **Null-safety — CLEAN.** All new iterators null-check GetWorld() and IsValid() each
  candidate; PerformHeal re-validates the weak SupportHealTarget; UpdateStateSiege/Support
  guard null goals with EnterIdle.
- **Header/cpp consistency — CLEAN.** All 9 new declarations (UpdateStateSiege,
  UpdateStateSupport, FindNearestEnemyBuilding, FindNearestDamagedFriendly,
  FindNearestFriendlyCombatUnit, StartHealing, StopHealing, PerformHeal, ApplyHealing)
  have matching definitions; includes present (Building.h, DamageTypes.h, EngineUtils.h,
  TimerManager.h, GameFramework/DamageType.h).
- **Same-class private access** (`Unit->Profile` in FindNearestFriendlyCombatUnit) is legal
  C++ (class-based access control, even on an AMinerUnit instance).
- **Performance:** all scans run on the 0.25 s state timer (not per-tick); PerformHeal at
  10 Hz does one closest-point test + getters — same order as the existing AcquireTarget /
  FindNearestEnemyCastle scans. No per-tick or hot-path load introduced.

## TASK-055 seam (confirmed clean)

- Damage-OUTPUT multipliers compose into the AMOUNT at SummonedUnit.cpp:954
  (`ApplyDamage(Target, AttackDamage, ...)`); TASK-054 selects only the TYPE
  (`MeleeDamageType`) and passes raw `AttackDamage`. TASK-055 replaces `AttackDamage` with
  the composed value and keeps the type selection — Siege 200% is fortification-side (the
  type), NOT an output multiplier, so no double-count.
- New profile helpers sit between `FindNearestEnemyCastle` and `EnterAttack`; dispatch is
  the first thing in UpdateState. Sapper (Profile=Siege, bSuicide) currently does normal
  Siege-tagged melee; TASK-055 swaps that for the single `ApplyRadialDamage` detonation
  (reusing `USiegeDamageType_Siege` added here). Seam is clean.

## Notes for build-master (on PASS)

- No new files beyond the class pairs already listed; USiegeDamageType_Siege is the only
  new reflected type (a bare UDamageType subclass — UHT generates it, no logic).
- Compiles as part of the M4 batch at TASK-068. TASK-055 serializes AFTER this on the
  shared SummonedUnit files — do not build TASK-054 in isolation if 055 has already landed.
- FindNearestEnemyBuilding's coverage of ABarracks/ADeepMine (TASK-057) is correct IFF
  TASK-057 makes them ABuilding subclasses — verify at the 057 integration, not here.
