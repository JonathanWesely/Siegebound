# TASK-054 — Siege + Support profiles, Siege damage type, castle/building 200% (handoff)

**Status:** ready-for-qa · file-only C++ · no compile, no Git, no board edit · TASK-057's files (Barracks/DeepMine/SiegePlayerState) untouched.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h` — added `USiegeDamageType_Siege` (UDamageType subclass, pure tag) + registry comment updates.
- `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.cpp` — comment only (types stay pure tags).
- `Source/GitClaudeUnrealTest/Siegebound/Castle.h` / `Castle.cpp` — added the Siege 200% branch to `ACastle::TakeDamage`.
- `Source/GitClaudeUnrealTest/Siegebound/Building.h` / `Building.cpp` — added the Siege 200% branch to `ABuilding::TakeDamage` + `#include "Siegebound/DamageTypes.h"`.
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` / `SummonedUnit.cpp` — Siege + Support profile branches, heal system, FreezeAI extension.

## 1. Siege damage type (DamageTypes.h)
`USiegeDamageType_Siege : public UDamageType`, no logic — the CONVENTIONS registry tag (manager already activated it in CONVENTIONS §"Damage types"). Doc block updated: consumers are now BOTH `ACastle::TakeDamage` and `ABuilding::TakeDamage`; the "reserved" note keeps only `Spell` (M5).

## 2. Castle & Building 200%
Both `TakeDamage` methods read `DamageEvent.DamageTypeClass.Get()` and branch:
- `ACastle::TakeDamage` (Castle.cpp): **added** `IsChildOf(USiegeDamageType_Siege)` → `×2.0` as an `if` **before** the existing `else if` Projectile `×0.5`. Siege and Projectile are disjoint types so order is cosmetic; melee/default stays `×1.0`. The M2 projectile 50% branch is byte-preserved.
- `ABuilding::TakeDamage` (Building.cpp): previously took listed damage with NO scaling. **Added** a single `IsChildOf(USiegeDamageType_Siege)` → `×2.0` branch; everything else (incl. Projectile) still takes listed damage — the projectile-50% rule stays **castle-only** per the M2 ruling. `TakeDamage` now returns the SCALED amount (was `ActualDamage`) — matches the ACastle contract; a Siege hit reports 2×.
- Both left a `TODO(Spell 50% — M5)` marker.

## 3. ASummonedUnit profiles
`Profile` (ECardProfile) is bound from the row in `LoadStatsAndStart` **before** the first synchronous `UpdateState()`. `UpdateState()` dispatches at the top:
```
if (Profile == Siege)   { UpdateStateSiege();   return; }
if (Profile == Support) { UpdateStateSupport(); return; }
// ...Standard body (unchanged) — None (miners) falls through here too
```

### SIEGE (Ogre, Sapper) — `UpdateStateSiege()`
- Ignores units and the hero entirely: acquisition is `FindNearestEnemyBuilding()` (new; `TActorIterator<ABuilding>`, skips friendly / destroyed — covers ATower and every ABuilding subclass at runtime) → else `FindNearestEnemyCastle()` (existing). No pawn is ever considered.
- `CurrentTarget` is recomputed to the nearest live structure each check, so a fallen wall / a newly-placed closer one re-routes it, and a structure killed mid-attack drops to the castle next check (no reliance on `IsTargetAlive` for buildings — `IsValid` + the `IsBuildingDestroyed` filter cover it).
- Advance/attack reuse the existing `EnterAdvance` (structure branch, non-pawn) / `EnterAttack` / `PerformAttack`.
- **Damage tag:** `PerformAttack` melee branch now selects the type — `Siege → USiegeDamageType_Siege`, else `UDamageType` (base). Ogre/Sapper are melee (`bRanged=false`), so this is their delivery path.

### SUPPORT (Cleric) — `UpdateStateSupport()`
- **Never attacks** — `EnterAttack`/`PerformAttack` are never called for Support; the attack timer never arms.
- Heal target = `FindNearestDamagedFriendly()` (new): nearest friendly `ASummonedUnit`, `CurrentHP < MaxHP`, within `AttackRange` (row 400), excluding self; enemies excluded (no friendly-fire mirror), so it **heals no enemies**.
- Continuous heal on a dedicated **heal timer** (`HealTimerHandle`, `StartHealing`/`StopHealing`/`PerformHeal`) at `SupportHealInterval` (0.1 s). Each tick heals `AttackDamage × SupportHealInterval` → **row Damage (8) HP/sec**, rate-invariant to the interval; `ApplyHealing` clamps to MaxHP (no overheal). `PerformHeal` re-validates the target every tick (alive/friendly/damaged/in-range).
- Follow = the damaged friendly if any, else `FindNearestFriendlyCombatUnit()` (new): nearest friendly with Profile Standard **or** Siege (excludes other Clerics and Miners so healers don't trail each other). Following reuses `EnterAdvance` (pawn branch, stops ~0.8×Range = 320, inside the 400 heal ring). A lone Cleric with no friendly at all → `EnterIdle`.
- `ApplyHealing(float)` is a new public BlueprintCallable heal sink (guards dead/frozen/unbound/≤0; no delegate — units have none).

### Standard — unchanged
The Standard `UpdateState` body is untouched below the dispatch; `PerformAttack` Standard/Support pass `UDamageType::StaticClass()` exactly as before (the ternary yields the identical `TSubclassOf<UDamageType>` for non-Siege), so the M1 melee/lunge and M2 ranged/projectile paths, the -90° yaw VisualMesh, the TASK-020 lunge, and TASK-042's move-speed buff are all behavior-identical. The stale "only Standard implemented in M1" warning was removed (all three profiles now ship; None → Standard body).

### FreezeAI extension
`FreezeAI` now also calls `StopHealing()` and nulls `SupportHealTarget`. Siege pathing decisions and Support follow decisions stop via the existing `StateTimerHandle` clear (no more `UpdateState`), and the in-flight advance/follow path stops via the existing `StopMovement`. `HealTimerHandle` is also cleared in `EndPlay` and `HandleDeath` (a dying Cleric heals no one). `PerformHeal` is gated on `bDead/!bStatsLoaded/bAIFrozen` as defense-in-depth.

## Miner (AMinerUnit) safety
Miner row `Profile=None` → the dispatch falls through to the Standard body, which is acquisition-dead by the subclass's `AggroRadius=0`/`StateCheckInterval=0` seals. `Profile` is bound before the one synchronous `UpdateState`, so the seal behavior is unchanged. No edit to MinerUnit.

## What QA should scrutinize
- **C4458 shadow rule:** new locals are `World`, `MyLocation`, `Structure`, `HealTarget`, `FollowGoal`, `Unit`, `Building`, `Best*`, `Distance`, `Amount`, `MeleeDamageType`, `IncomingDamageType`, `ScaledDamage` — none shadow `Owner`/`PlayerState`/`Instigator`/`Controller`/`Team` or any member (verified). `SpawnParameters.Owner/Instigator` are pre-existing struct-field writes, not shadows.
- **Same-class private access:** `FindNearestFriendlyCombatUnit`/`PerformHeal` read another `ASummonedUnit`'s private `Profile` (legal C++ same-class access); `FindNearestDamagedFriendly`/`PerformHeal` use only public getters.
- **TASK-055 seam (serializes AFTER me on SummonedUnit):** TASK-055 adds Charge/Slayer/Aura damage-output multipliers + Sapper suicide. Structure to build on:
  - Damage-OUTPUT centralization goes in `PerformAttack`'s melee branch. I select the damage-TYPE there via `MeleeDamageType` (Siege vs base) but pass the raw `AttackDamage` as the amount. TASK-055 should compose the multipliers into the amount and keep my type selection (Siege 200% is fortification-side via the type, NOT an output multiplier — do not double-count).
  - New profile helpers live between `FindNearestEnemyCastle` and `EnterAttack` in the .cpp; the profile dispatch is the first thing in `UpdateState`.
  - Sapper is `Profile=Siege` + `bSuicide` — it currently does normal Siege melee (tagged Siege). TASK-055 replaces that with the single detonation on reaching range/death (its `ApplyRadialDamage` uses `USiegeDamageType_Siege`, added here).
- **Interpretation notes (reason, no compile):** Siege targets the nearest enemy building **map-wide** (no aggro gate) — matches "hits the first wall/tower then the castle" in the standard arena; "in path" read as "nearest". Support's out-of-range damaged non-combat friendly (e.g. a distant lone miner with no combat units around) is not chased — the fallback follow is the nearest **combat** unit per spec. Both noted for playtest.
- **Acceptance reasoning:** Ogre — never acquires a pawn, advances on nearest enemy structure, PerformAttack tags Siege → castle/building `×2.0` (500 HP tank soaks the units it passes). Cleric — never enters Attack, heals nearest damaged friendly ≤400 at 8 HP/s clamped to MaxHP, enemy scan excluded, follows combat units otherwise. Standard — dispatch skips both branches; melee/ranged behavior identical.
