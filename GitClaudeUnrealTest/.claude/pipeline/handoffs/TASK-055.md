# TASK-055 — Standard keywords (Charge / Slayer / Swarm-data) + damage-output centralization + AoE radial helper + Sapper suicide (handoff)

**Status:** ready-for-qa · file-only C++ · no compile, no Git, no board edit · layered on qa-passed TASK-054. TASK-057's files (Barracks/DeepMine/SiegePlayerState) untouched. TASK-056/058 seams documented below (they depend on symbols added here).

## Files touched / created
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` / `.cpp` — the shared `FSiegeCombatStatics::ApplyRadialDamage` static (AoE). No Build.cs change (same module, only Core/Engine deps already linked; not a UObject — no reflection).
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` / `.cpp` — centralized damage OUTPUT, Charge, Slayer, War Banner aura hook, Sapper suicide detonation. Reads `bCharge/bSlayer/bSuicide/AoERadius` (TASK-053 row columns); uses `USiegeDamageType_Siege` (TASK-054).

Swarm (`SwarmCount`) is intentionally NOT consumed here — it is a spawn-side multi-spawn, owned by TASK-059 (player) / TASK-060 (bot). This task only ensures the column is in the data (already true, TASK-053). The title's "Swarm" is the keyword *set*; the multi-spawn lives downstream.

## 1. Centralized damage OUTPUT (the composition seam)
`ASummonedUnit::PerformAttack` now computes **once**, before the melee/ranged branch:
```
const float OutputDamage = ComputeOutputDamage(Target);   // = row Damage × Charge × Slayer × Aura
```
- Melee branch: `ApplyDamage(Target, OutputDamage, GetController(), this, MeleeDamageType)` (type unchanged from TASK-054: Siege → `USiegeDamageType_Siege`, else base `UDamageType`).
- Ranged branch: `FireProjectileAt(Target, OutputDamage)` — new `float DamageAmount` param plumbed into `InitProjectile` (aura buffs a Longbowman's shot too).
- **Siege 200% is NOT an output multiplier** — it stays fortification-side via the damage TYPE in `ACastle`/`ABuilding::TakeDamage` (TASK-054). Composing it here would double-count; it is deliberately excluded from `ComputeOutputDamage`.
- **Non-regression:** for a non-keyword, un-auraed unit all three factors are exactly `1.0`, so `ComputeOutputDamage` returns `AttackDamage` **bit-for-bit** → Footman/Archer/Knight/Ogre/Cleric/miner melee + ranged paths are byte-identical to TASK-054. The Standard `UpdateState` body below the profile dispatch is untouched.

`ComputeOutputDamage(const AActor* Target)` (non-const — consumes the charge):
1. Charge: `if (bCharge && bChargePrimed) { Output *= ChargeMultiplier; bChargePrimed=false; ChargeMoveElapsed=0; }`
2. Slayer: `if (bSlayer && GetTargetMaxHP(Target) >= SlayerHPThreshold) Output *= SlayerMultiplier;` (`bSlayer` short-circuits `GetTargetMaxHP` off the non-Slayer hot path).
3. Aura: `Output *= AuraDamageMultiplier;` (1.0 when no aura).

`GetTargetMaxHP` (static): casts to ASummonedUnit / ACastle / ABuilding / AHeroCharacter → their `GetMaxHP()`; unknown → 0 (no Slayer bonus).

## 2. CHARGE (bCharge, Cavalry) — UPROPERTY `ChargeMoveSeconds` 2.f // GDD §3.0, `ChargeMultiplier` 2.f
- `TrackChargeMovement()` is called at the **top of every UpdateState** (before the profile dispatch). It is a **no-op unless `bCharge`**, so every non-Cavalry unit and the Miner subclass are byte-unchanged.
- While `State==Advance` and `GetVelocity().SizeSquared() > ChargeMoveSpeedThreshold²` → accumulate `StateCheckInterval`; at `>= ChargeMoveSeconds` set `bChargePrimed`. A stall (blocked, velocity ~0 while advancing) or `State==Idle` resets `ChargeMoveElapsed` + clears `bChargePrimed` (momentum lost).
- **Deliberately does NOT reset on `State==Attack`** — tracking and the state dispatch share one `UpdateState`, so resetting on the reach-range check would race the very hit that should be charged. The primed flag is instead consumed exactly once in `ComputeOutputDamage`.
- `ChargeMoveSpeedThreshold` (50 u/s, UPROPERTY) is an **implementation detail** for blocked/stopped detection on the 0.25 s timer — NOT a GDD stat, documented as such.
- **Acceptance:** Cavalry (Damage 20) advances continuously ≥2 s, first hit `ComputeOutputDamage` → 40, consumes; next hit → 20 until it moves 2 s again.

## 3. SLAYER (bSlayer, Pikeman) — UPROPERTY `SlayerMultiplier` 2.f, `SlayerHPThreshold` 150.f // GDD §3.0
- ×2 vs any target with `MaxHP >= 150`. **Acceptance:** Pikeman (Damage 30, Standard/melee, base type) → 60 vs a 200-HP Knight (`ASummonedUnit::GetMaxHP()` 200 ≥ 150), 30 vs an 80-HP Footman.

## 4. AURA HOOK (War Banner, for TASK-058)
```
UFUNCTION(BlueprintCallable) void SetAuraDamageBonus(float Bonus, float Duration);
```
- `Bonus 0.20` ⇒ `AuraDamageMultiplier = 1.20` for `Duration` s, then a one-shot timer → `EndAuraDamageBuff()` resets to **exactly 1.0**.
- **Drift-free discipline (stronger than ApplyMoveSpeedBuff's cache-once):** the multiplier is stored SEPARATELY from the row-bound `AttackDamage` and composed only at strike time — the base is never mutated in place, so there is nothing to drift; expiry always restores the literal `1.0`. Re-applying **refreshes-not-stacks** (writes the multiplier directly from `Bonus`, never off the buffed value; re-arms the single timer). Guards dead/frozen; non-positive Bonus/Duration clears immediately.
- **FreezeAI clears it** (added `EndAuraDamageBuff()` next to `EndMoveSpeedBuff()`); `EndPlay` clears `AuraDamageBuffTimerHandle`.

## 5. AoE radial helper — the EXACT signature TASK-056 consumes
`Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h`:
```cpp
class GITCLAUDEUNREALTEST_API FSiegeCombatStatics
{
public:
    static void ApplyRadialDamage(
        UWorld* World,
        AController* InstigatorController,
        ETeamId Team,                       // the ATTACKER's team; same-team actors are never damaged
        const FVector& Center,
        float Radius,
        float Damage,
        TSubclassOf<UDamageType> DamageTypeClass);
};
```
Call form: `FSiegeCombatStatics::ApplyRadialDamage(World, InstigatorController, Team, Center, Radius, Damage, DamageTypeClass);`
- Damages **all ENEMY combat actors** (ITeamAgent whose `GetTeamId() != Team`) within Radius. **No friendly fire — the Team filter is the authority**, not the receiver's instigator chain (so a tower-fired projectile with a null controller still can't friendly-fire).
- **Distance is closest-point** (`ActorGetDistanceToCollision(Center, ECC_Pawn, …)`, fallback actor origin) so a blast at a fortification's WALL reaches the castle/building even though its origin is hundreds of units past the wall. (Single documented call site mirroring `ASummonedUnit::GetDistanceToTarget`; folds into the qa/TASK-026 NIT-4 consolidation on the wave that owns hero+projectile+unit together — not a new gratuitous mirror.)
- Each hit routes through the target's own `TakeDamage`, so **per-fortification scaling still applies** (Siege → 200% vs castle/buildings). `DamageCauser` is null on purpose; `InstigatorController` is pass-through for attribution.
- `AGoldNode` does not implement ITeamAgent → never caught in a blast.
- No-op on null World / Radius ≤ 0 / Damage ≤ 0; null type defaults to base `UDamageType`.

**TASK-056 reuse note:** for the Bomb Tower AoE projectile, on impact (AoERadius > 0) call this with `Center = impact point`, `Team = firing team`, `Damage = row Damage`, `DamageTypeClass = USiegeDamageType_Projectile`; keep the single-target path for AoERadius == 0. `InstigatorController` can be the projectile's instigator's controller or null (the Team filter covers friendly-fire either way).

## 6. SUICIDE (bSuicide, Sapper — a Siege+bSuicide unit)
- `UpdateStateSiege`: on reaching attack range, `if (bSuicide) { Detonate(); return; }` **instead of** `EnterAttack()` — so the attack timer never arms and there are **no repeat hits**. bSuicide is a Siege-unit keyword per the data (Sapper Profile=Siege), so the trigger lives in the Siege path and the Standard body is untouched.
- `Detonate()` → `ApplyDetonation()` then `HandleDeath()` (destroys). `ApplyDetonation()` calls `FSiegeCombatStatics::ApplyRadialDamage(World, GetController(), Team, GetActorLocation(), AoERadius, AttackDamage, USiegeDamageType_Siege::StaticClass())`.
- **On death too:** `HandleDeath` calls `ApplyDetonation()` first (before teardown) when `bSuicide`, so a Sapper shot down en route still explodes. `bDetonated` guards a **single** blast whether triggered by contact or death.
- **Nothing hardcoded:** `AttackDamage` (row Damage 80) and `AoERadius` (row 250) are bound from DT_Cards in `LoadStatsAndStart`.
- The blast uses raw row `AttackDamage`, **not** `ComputeOutputDamage` — a one-shot AoE is not a repeatable single-target attack, so Charge/Slayer/aura don't compose into it (per-target Slayer is undefined for AoE). Documented interpretation; flag if design wants the aura to buff it.
- **Acceptance:** Sapper reaches a wall/castle, one 80-damage blast in 250 (Siege-typed → 160 to castle/buildings, 80 to units), then dies. Single detonation.

## New row-bound members + UPROPERTYs (SummonedUnit)
- Transient, bound in `LoadStatsAndStart`: `bCharge`, `bSlayer`, `bSuicide`, `AoERadius` (defaults false/0 leave core cards unchanged).
- UPROPERTY // GDD: `ChargeMoveSeconds=2`, `ChargeMultiplier=2`, `SlayerMultiplier=2`, `SlayerHPThreshold=150`, plus `ChargeMoveSpeedThreshold=50` (impl detail, not a stat).
- Plain state: `ChargeMoveElapsed`, `bChargePrimed`, `AuraDamageMultiplier=1`, `bAuraDamageBuffActive`, `AuraDamageBuffTimerHandle`, `bDetonated`.

## What QA should scrutinize
- **C4457/58/59 shadow scan:** new locals — `Output`, `Target`(param), `Unit`/`Castle`/`Building`/`Hero`(in GetTargetMaxHP), `World`, `OutputDamage`, `DamageAmount`(param), `Bonus`/`Duration`(params); statics-file params `World`/`InstigatorController`/`Team`/`Center`/`Radius`/`Damage`/`DamageTypeClass` and locals `TeamAgents`/`Candidate`/`Agent`/`ClosestPoint`/`Distance`. **None shadow** `Owner`/`Instigator`/`Controller`/`PlayerState`. `InstigatorController` was named to avoid the `Instigator` shadow; the static's `Team` param is a free-function param (no member — per the task note, fine). `SpawnParameters.Owner/Instigator` in FireProjectileAt are pre-existing struct-field writes (not shadows).
- **Byte-identical non-regression:** the ONLY change to shared code paths for non-keyword units is (a) one `TrackChargeMovement()` call at the top of `UpdateState` (early-returns on `!bCharge`) and (b) `ApplyDamage`/`FireProjectileAt` now take `ComputeOutputDamage(Target)` which returns `AttackDamage` unchanged for them. The Standard `UpdateState` body, Siege body (Ogre path), Support body, TASK-020 lunge, TASK-042 move-speed buff, and the -90° yaw VisualMesh are otherwise untouched.
- **Single-blast + re-entrancy:** `bDetonated` guards contact-vs-death double-blast; `Detonate` guards `bDead`; the blast excludes the sapper itself (same-team filter + IsValid); `Destroy()` from within the state-timer callback matches the existing `PerformAttack`→`HandleDeath`→`Destroy` pattern.
- **Charge timing (0.25 s granularity):** consumed in `ComputeOutputDamage`, not reset on entering Attack, to avoid the same-tick race; the primed run survives into the first hit because the unit is still moving on the reach-range check (structure targets move at full speed to the wall). Noted for playtest.

## Constraints honored
Files only — no compile, no Git, no TASKBOARD edit (orchestrator owns the board). qa-passed TASK-054 Siege/Support code + Standard byte-behavior preserved. TASK-057 files not touched.
