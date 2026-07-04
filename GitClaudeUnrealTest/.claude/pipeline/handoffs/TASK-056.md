# TASK-056 handoff — Bomb Tower (AoE) + Ballista Tower (min-range blind spot)

**Status:** ready-for-qa · files only, no compile, no Git, no board edit.
**Author:** gameplay-programmer · M4 wave 4 · 2026-07-04

## Class choice (RECORD FOR TASK-063): row-driven ATower, NO new subclasses
Both towers are **plain `ATower`** driven entirely by DT_Cards. No new class was created — behavior did not demand it. The M2 acquire/idle loop is shared; two extra row cells reshape it:
- `AoERadius > 0` (BombTower 250) → the shot is an **AoE projectile** (blasts every enemy in the radius at the impact point).
- `MinRange > 0` (BallistaTower 300) → **blind-spot acquire** (nearest enemy inside `[MinRange, Range]`, closer targets ignored).

**→ TASK-063: `BP_Building_BombTower` and `BP_Building_BallistaTower` both parent `ATower` directly** (like `BP_Building_ArrowTower`). Each BP only presets `CardID` (BombTower / BallistaTower) + `SM_<CardID>` mesh + `MuzzleOffset`; nothing stat-like on the BP. Rows already exist in `Docs/Data/cards.csv` (BombTower: 180 HP / 25 dmg / 800 range / 2.5 s / AoERadius 250; BallistaTower: 120 HP / 45 dmg / 1400 range / 3.0 s / MinRange 300).

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/Projectile.h`
- `Source/GitClaudeUnrealTest/Siegebound/Projectile.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tower.h`
- `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp`

No other files touched. Did **not** touch `SummonedUnit.*` (TASK-055) or `SiegeCombatStatics.*` (only CALL `ApplyRadialDamage`), and did **not** touch any TASK-058 file (`HeroCharacter.*`, `HeroUpgradeComponent.*`).

## AProjectile AoE support — the InitProjectile change
- `InitProjectile` gained a **trailing defaulted param**: `..., float InAoERadius = 0.f`. This is one function, not an overload — a defaulted trailing param keeps every existing 4-arg caller compiling and behaving byte-for-byte:
  - `ASummonedUnit.cpp:1208` (Archer, TASK-055's file — **not edited**) still calls the 4-arg form → `AoERadius == 0` → single-target.
  - `ATower::FireProjectileAt` now passes `AttackAoERadius` (0 for Arrow/Ballista, 250 for Bomb).
- New private member `float AoERadius = 0.f` (set from `FMath::Max(InAoERadius, 0.f)` — negatives clamped to 0 = single-target).
- `HandleImpact` gained an **AoE branch at the very top** (before the single-target same-team gate):
  ```
  if (AoERadius > 0.f) { ApplyRadialDamage(World, GetInstigatorController(), Team, ImpactPoint, AoERadius, Damage, DamageTypeClass); spawn NS_Damage @ ImpactPoint; Destroy(); return; }
  ```
  Signature used exactly as TASK-055 froze it: `(World, InstigatorController, Team, Center, Radius, Damage, DamageTypeClass)`.
- Added `#include "Siegebound/SiegeCombatStatics.h"` to `Projectile.cpp`.

## Bomb / Ballista behavior
- **Bomb Tower:** ArrowTower loop unchanged for acquisition (nearest enemy unit/hero within Range 800, MinRange 0 so no blind spot). Fires every Cadence 2.5 s. The fired projectile carries AoERadius 250 → on impact, `ApplyRadialDamage` deals 25 (Projectile type) to every enemy within 250 of the impact point → a 4-unit cluster all take 25 from one blast. No friendly fire (the radial helper's own Team filter is the sole authority).
- **Ballista Tower:** `AcquireTarget` adds one term — `DistSq < MinRangeSq → skip`. Picks the nearest enemy in `[MinRange 300, Range 1400]`. A target at 1200 → hit for 45 (single-target, AoERadius 0). A target at 250 → skipped; if it is the only candidate, returns nullptr → idle, re-scan next cadence. Cadence 3.0 s.
- Both read **all** stats from DT_Cards (`Row.Damage/Range/Cadence/AoERadius/MinRange`); nothing hardcoded.

## How single-target / ArrowTower stays UNCHANGED (byte-for-byte)
- The AoE branch is only entered when `AoERadius > 0`. For every AoERadius==0 shot (Archer, Longbowman, ArrowTower, Ballista) execution falls straight through to the original same-team gate + `ApplyDamage` + damage-landed VFX gate + `Destroy` — literally the pre-TASK-056 code, unmodified.
- `AcquireTarget`: `MinRangeSq == 0` for Arrow/Bomb, so the added `DistSq < MinRangeSq` term is never true (DistSq ≥ 0). Acquisition is identical.
- `FireProjectileAt`: the only change is the extra 5th arg; for Arrow it's `AttackAoERadius == 0`.
- `SummonedUnit.cpp` Archer path is completely untouched (defaulted param).

## Things QA should scrutinize
1. **Defaulted-param vs overload:** confirm the trailing `float InAoERadius = 0.f` on the BlueprintCallable `InitProjectile` is acceptable (single UFUNCTION, non-breaking; no second same-named UFUNCTION, which would be a UHT error). All existing 4-arg call sites verified to still resolve.
2. **AoE VFX is unconditional** (spawns whenever `CachedImpactEffect` valid), unlike single-target's damage-landed gate — deliberate: `ApplyRadialDamage` returns no total to gate on, and per spec the impact donor spawns on the blast.
3. **Target-death-mid-flight for the Bomb Tower:** if the homed target dies before the projectile reaches it, the projectile expires harmlessly at LastKnownAimPoint (Tick's lost-target path) and **no blast fires** — same flight semantics as single-target, consistent by design (re-fire next cadence). Not a regression; matches the M2 "lost target = harmless expiry" contract.
4. **MinRange metric:** the blind-spot check uses origin-to-origin distance (same metric + `MinRangeSq` mirror as the existing Range gate), not closest-point — consistent with the existing acquisition rationale (qa/TASK-026 NIT-4: no new closest-point mirror). ~35uu capsule vs a 300 blind spot is deliberate feel noise.
5. **No friendly fire on the blast:** enforced solely by `ApplyRadialDamage`'s `GetTeamId() == Team` filter (TASK-055, qa-passed) — the projectile's own same-team gate is intentionally bypassed on the AoE path since the blast is location-based, not target-based.
6. **C4458 shadow scan:** new symbols are `InAoERadius` (param), `AoERadius` (AProjectile member), `AttackAoERadius`/`AttackMinRange` (ATower members), `MinRangeSq` (local) — none shadow an inherited reflected UPROPERTY (Owner/Instigator/Controller/etc.). `Team` is used as the inherited member directly, no local of that name.
7. **No Build.cs change** — `FSiegeCombatStatics` is same-module (Core/Engine already linked).
