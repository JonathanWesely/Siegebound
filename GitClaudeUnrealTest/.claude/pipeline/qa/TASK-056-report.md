# QA Report — TASK-056

**Task:** New towers: Bomb Tower (AoE) + Ballista Tower (min-range blind spot) (C++)
**Milestone:** M4 · reviewed on `main` · pre-compile gate (M4 batch compile at TASK-068)
**Files reviewed:** `Projectile.h`/`.cpp`, `Tower.h`/`.cpp`
**Reviewer:** qa-reviewer · 2026-07-04

## Verdict: PASS

Blockers: 0 · Warnings: 0 · Nits: 2 (informational)

Code is NOT compiled — this is the pre-compile safety gate. Findings are from static review against UE 5.8 + the M4 design rulings + the TASK-056 spec/handoff (row-driven `ATower`, no subclass).

---

## Critical-focus confirmations (highest priority)

### 1. Single-target / M2-M3 NON-REGRESSION — CONFIRMED
- `InitProjectile` gained a **single trailing defaulted param** `float InAoERadius = 0.f` (header decl `Projectile.h:94`; cpp def `Projectile.cpp:86` correctly omits the default). It is ONE `UFUNCTION`, not an overload — no second same-named reflected function, so no UHT collision. Default lives only on the declaration (correct C++).
- **Every existing 4-arg caller still binds unchanged.** Full-tree grep for `InitProjectile` returns exactly two call sites: `SummonedUnit.cpp:1208` (Archer/Longbowman path — 4-arg form, untouched) and `Tower.cpp:246` (this task, 5-arg). The Archer 4-arg call resolves to `InAoERadius = 0.f` → `AoERadius == 0` → single-target. No other unit/bot ranged-fire caller exists.
- **AoE branch is entered ONLY when `AoERadius > 0`.** `HandleImpact` (`Projectile.cpp:249`) gates the radial path behind `if (AoERadius > 0.f)`. `AoERadius` is set from `FMath::Max(InAoERadius, 0.f)` (`Projectile.cpp:104`), so a stray negative clamps to 0 = single-target; the branch can never fire for a default/negative arm.
- Every `AoERadius == 0` shot falls straight through to the ORIGINAL single-target path (`Projectile.cpp:268-302`): same-team gate → `ApplyDamage` → damage-landed VFX gate → `Destroy`. That block is byte-for-byte the pre-TASK-056 code — no line inside it was altered.

### 2. ArrowTower behavior-unchanged — CONFIRMED
- ArrowTower row has `AoERadius == 0` and `MinRange == 0`. In `AcquireTarget`, `MinRangeSq = FMath::Square(0) = 0` (`Tower.cpp:130`), so the added term `DistSq < MinRangeSq` is `DistSq < 0` — impossible (DistSq ≥ 0). Acquisition is identical to M2/M3.
- In `FireProjectileAt`, ArrowTower passes `AttackAoERadius == 0` → single-target projectile. The new `MinRange` acquire term cannot change Arrow/Bomb acquisition.

## Behavior checks

### Bomb Tower — CORRECT
- Acquires nearest enemy unit/hero ≤ Range 800 (`MinRange 0`, no blind spot), fires every Cadence 2.5 s (looping timer, armed once behind the `Cadence > 0` guard `Tower.cpp:66-88`).
- `FireProjectileAt` passes `AttackAoERadius` (250) into `InitProjectile` (`Tower.cpp:246`); on impact the `AoERadius > 0` branch calls the **qa-passed** `FSiegeCombatStatics::ApplyRadialDamage(World, GetInstigatorController(), Team, ImpactPoint, AoERadius, Damage, DamageTypeClass)` — helper is CALLED, not modified. Signature matches the frozen TASK-055 declaration exactly (7 args, verified against `SiegeCombatStatics.h:59-66`).
- ImpactPoint is the closest point on the homed target's collision, so the primary target sits ~0 uu from the blast center and is always caught (25, Projectile type). A 4-unit cluster within 250 all take 25 from one blast — anti-swarm intent met. Per-target `TakeDamage` routing preserves per-receiver scaling.

### Ballista Tower — CORRECT
- `AcquireTarget` adds exactly one term to the skip condition (`Tower.cpp:188`): `DistSq > RangeSq || DistSq < MinRangeSq || DistSq >= BestDistSq`. `MinRangeSq = Square(300) = 90000`.
- Target at 250 → `DistSq 62500 < 90000` → skipped; if it is the only candidate, returns `nullptr` → idle, re-scan next cadence (`ScanAndFire` `Tower.cpp:101-106`). Target at 1200 → eligible, hit for 45 single-target (AoERadius 0). Cadence 3.0 s.
- **Squared-distance units are consistent:** both the max-range gate (`RangeSq = Square(AttackRange)`) and the blind-spot term (`MinRangeSq = Square(AttackMinRange)`) square the same origin-to-origin `DistSquared(MyLocation, Candidate->GetActorLocation())`. Ring boundaries are inclusive on both ends (`< MinRangeSq` skip, `> RangeSq` skip) → `[MinRange, Range]`, matching the handoff. The new term does not disturb the max-range gate.
- Degenerate-row guard: `MinRange >= Range` is logged, not clamped (`Tower.cpp:54-59`) — data stays authoritative; BallistaTower 300/1400 passes clean.

### No friendly fire — CONFIRMED
- Bomb blast: `InitProjectile(Team, ...)` sets the projectile's `Team` to the firing tower's team (`Tower.cpp:246`); `HandleImpact` passes that same `Team` to `ApplyRadialDamage`, whose own `GetTeamId() == Team` filter is the sole authority and excludes same-team actors. The projectile's single-target same-team gate is intentionally bypassed on the location-based AoE path (documented, correct).
- Single-target path keeps its existing same-team gate (`Projectile.cpp:273-281`) unchanged.

### AoE-on-impact / lifetime — CONFIRMED
- On blast: spawns the `NS_Damage` donor at ImpactPoint (unconditional, guarded by `CachedImpactEffect` non-null) then `Destroy()` + `return` (`Projectile.cpp:258-265`). In-flight homing and lost-target harmless expiry are untouched (`Tick` `Projectile.cpp:191-236`).
- Null-safety: `GetWorld()` null → `ApplyRadialDamage` is a documented no-op and `SpawnSystemAtLocation` is null-World-safe; `GetInstigatorController()` null (tower has no instigator pawn) is an accepted `ApplyRadialDamage` arg (attribution optional, Team is the FF authority). `CachedImpactEffect` null-checked before spawn.

## Standard scans
- **C4458 shadow scan (Owner/Instigator):** clean. New symbols — `InAoERadius` (param), `AoERadius` (`AProjectile` member), `AttackAoERadius`/`AttackMinRange` (`ATower` members), `RangeSq`/`MinRangeSq`/`BestDistSq`/`DistSq`/`Best` (locals) — none shadow an inherited reflected member. `FireProjectileAt` uses `SpawnParams.Owner` (a struct field, not a local named `Owner`); no local named `Owner`/`Instigator` exists. `Team`/`CardID` are used as the inherited protected `ABuilding` members directly, no local shadows.
- **Deprecated UE 5.8 APIs:** none introduced. `ApplyRadialDamage` (project static), `UNiagaraFunctionLibrary::SpawnSystemAtLocation`, `UGameplayStatics::ApplyDamage`, `GetWorldTimerManager().SetTimer`, `GetAllActorsWithInterface`, `FMath::Square/Max`, `GetInstigatorController` — all current. `ActorGetDistanceToCollision` (pre-existing helper) unchanged and valid.
- **Header/cpp consistency:** `InitProjectile` decl/def match (default only on decl). `ATower` declares and defines `EndPlay`, `OnStatsLoaded`, `ScanAndFire`, `AcquireTarget`, `FireProjectileAt` + members `AttackAoERadius`/`AttackMinRange`. `OnStatsLoaded(const FCardRow&) override` matches the `ABuilding` base virtual.
- **Symbols verified to exist:** `FCardRow::AoERadius` / `::MinRange` (CardRow.h), `ApplyRadialDamage` (SiegeCombatStatics.h), `ASummonedUnit::IsUnitDead()`, `AHeroCharacter::IsDead()`, `USiegeDamageType_Projectile`, `ABuilding::IsBuildingDestroyed()`.
- **Build.cs:** no change needed — `FSiegeCombatStatics` is same-module; `Projectile.cpp` already adds `#include "Siegebound/SiegeCombatStatics.h"` (`Projectile.cpp:16`). `Tower.cpp` needs no new include (it only forwards `AttackAoERadius` through `InitProjectile`; the AoE call lives in `Projectile.cpp`).
- **Row-driven / TASK-063:** confirmed both towers stay plain `ATower` — no new class. `BP_Building_BombTower` and `BP_Building_BallistaTower` must both parent `ATower` directly (like `BP_Building_ArrowTower`), presetting only CardID + mesh + MuzzleOffset. Rows already present in `Docs/Data/cards.csv` per handoff.

## Findings
- [NIT] Projectile.cpp:249-266 — The Bomb AoE reuses `ApplyRadialDamage`, whose documented contract hits EVERY enemy `ITeamAgent` within radius by closest-point — including enemy buildings/castle that happen to sit within 250 uu of the impact point. This is intended splash of the approved helper (spec: "deals to ALL enemies in 250"), not friendly fire (same-team excluded) and not out of spec, but designers should note the Bomb tower chips adjacent enemy fortifications for Projectile-type (listed, no Siege 200% bonus). Informational only.
- [NIT] Projectile.cpp:215-221 — If the Bomb tower's homed target dies mid-flight, the projectile expires harmlessly at LastKnownAimPoint with NO blast (the M2 lost-target contract; the AoE branch is only reached on a live-target reach hit). Consistent by design (re-fire next cadence), not a regression. Informational only.

## Notes for build-master (if PASS)
- Pre-compile PASS only — no compile was run (M4 batch compile is TASK-068). Fold TASK-056 into that batch.
- No new module dependency; no Build.cs edit. `Projectile.cpp` include for `SiegeCombatStatics.h` is present.
- Depends on the qa-passed TASK-055 `FSiegeCombatStatics::ApplyRadialDamage` being in the same compile set (it is, M4). Depends on DT_Cards carrying the `AoERadius`/`MinRange` columns (present in `CardRow.h`; rows in `Docs/Data/cards.csv` per handoff — re-import DT_Cards if the CSV changed).
- TASK-063 (BP parenting) is the downstream art/BP task: `BP_Building_BombTower` + `BP_Building_BallistaTower` both parent `ATower` directly.
