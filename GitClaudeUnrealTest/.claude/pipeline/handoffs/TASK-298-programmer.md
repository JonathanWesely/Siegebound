# TASK-298 handoff — Wizard fireball AoE gameplay (gameplay-programmer)

**Status:** ready-for-qa · file-only (no compile, no editor, no Git, no MCP)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — ONLY.

Two surgical, additive, backward-compatible changes. Nothing else in either file changed. No existing signature changed.

## Change 1 — AoE pass-through (the one required behavioral change)
`ASummonedUnit::FireProjectileAt` was omitting the 5th `InitProjectile` arg, so every ranged unit was single-target.

**Before** (SummonedUnit.cpp ~L2128):
```cpp
Projectile->InitProjectile(Team, Target, DamageAmount, USiegeDamageType_Projectile::StaticClass());
```
**After** (SummonedUnit.cpp, now ~L2137):
```cpp
Projectile->InitProjectile(Team, Target, DamageAmount, USiegeDamageType_Projectile::StaticClass(), AoERadius);
```
`AoERadius` is the EXISTING row-bound member (`SummonedUnit.cpp:980` `AoERadius = Row->AoERadius;`; declared `SummonedUnit.h` ~L876, default `0.f`). Nothing new bound.

**Why the single-target path is unchanged when radius == 0:** `AProjectile::InitProjectile(ETeamId, AActor*, float, TSubclassOf<UDamageType>, float InAoERadius = 0.f)` (Projectile.h:105). The 5th param already defaults to `0.f`; the header docs (Projectile.h:98-102, 229-232) state radius `0` "keeps the unchanged single-target behavior" and only `> 0` routes through `FSiegeCombatStatics::ApplyRadialDamage`. Archer/Longbowman rows have `AoERadius 0`, ArrowTower/Ballista/Crystal/Bomb-tower callers use their own tower code (not this method). So passing `AoERadius` here is **byte-for-byte identical to omitting it** for every existing `ASummonedUnit` ranged caller (all have `AoERadius 0`). Only a row with `AoERadius > 0` (the Wizard, 250) now splashes — via the proven TASK-056 Bomb-Tower `HandleImpact`→`ApplyRadialDamage` path (enemies in radius only, no friendly fire, castle-side 50% projectile scaling preserved).

## Change 2 — optional per-unit ProjectileClass (additive, null-safe fallback)
**Header (SummonedUnit.h):**
- Forward decl added among the `A*` classes: `class AProjectile;` (alphabetical, after `class ACastle;`). `TSubclassOf<AProjectile>` as a UPROPERTY needs only a forward declaration; UHT reflects it by name.
- New member added right after the design-time `CardTableAsset` property:
```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")
TSubclassOf<AProjectile> ProjectileClass;   // default null
```
`EditDefaultsOnly` (design-time, set on the BP) — deliberately NOT `Transient`/`VisibleInstanceOnly` (it is not row-bound). Category `Siegebound|Unit` matches every other design-time UPROPERTY in this header. Named `ProjectileClass` per CONVENTIONS Wizard block; BP_Unit_Wizard will set it to `BP_Projectile_Fireball`.

**cpp (SummonedUnit.cpp, in FireProjectileAt):** the spawn class is now chosen null-safe, then the SAME `SpawnActor<AProjectile>` runs:
```cpp
const TSubclassOf<AProjectile> SpawnClass = ProjectileClass ? ProjectileClass : AProjectile::StaticClass();
if (AProjectile* Projectile = World->SpawnActor<AProjectile>(SpawnClass, FTransform(FireRotation, MuzzleLocation), SpawnParameters))
```
When `ProjectileClass` is null (every existing unit) `SpawnClass == AProjectile::StaticClass()` — identical to the old hardcoded spawn. `SpawnActor<AProjectile>(SpawnClass, ...)` returns `AProjectile*` because any override is an `AProjectile` subclass.

## Coding-law checks
- **No shadowing:** `ProjectileClass` is not an inherited reflected member (ACharacter/APawn have no such property); the local `SpawnClass` shadows nothing.
- **Complete-type include:** `AProjectile` is already fully included in the .cpp (`#include "Siegebound/Projectile.h"`, SummonedUnit.cpp:33) — `AProjectile::StaticClass()`, the `TSubclassOf` bool test, and `SpawnActor<AProjectile>` all see the complete type. The header only forward-declares (sufficient for the member).
- **No new AoE routine:** reuses the existing `InitProjectile`→`ApplyRadialDamage` machinery only.

## What QA should scrutinize (TASK-298-QA)
1. Confirm no existing `ASummonedUnit` ranged caller has `AoERadius > 0` today (Archer/Longbowman = 0 in cards.csv) ⇒ single-target unchanged.
2. Confirm the radial path is the existing one (Projectile.cpp `HandleImpact`), not a re-implementation, and has no friendly-fire hole.
3. Confirm `ProjectileClass` null-safe fallback = `AProjectile::StaticClass()`.
4. TRUTH gate: a `bRanged` row with `AoERadius > 0` genuinely splashes, so M7.7 `GetCardDescription` "splash radius 250" is truthful.

Downstream: TASK-304 (build-master) owns the compile + BP_Unit_Wizard authoring (`ProjectileClass = BP_Projectile_Fireball`, `VisualMesh = SM_Wizard`).

## QA-fix — loop 1 (2026-07-26)
QA blocker (qa/TASK-298.md): the `SpawnClass` ternary mixed `TSubclassOf<AProjectile>` and `UClass*` → ambiguous common type under UE 5.8 `/permissive-` (`/Zc:ternary`), MSVC **C2445**, compile-stop. One-line fix (SummonedUnit.cpp:2126), matching codebase precedent `BattlefieldScatter.cpp:1038`:
- Before: `const TSubclassOf<AProjectile> SpawnClass = ProjectileClass ? ProjectileClass : AProjectile::StaticClass();`
- After:  `const TSubclassOf<AProjectile> SpawnClass = ProjectileClass ? ProjectileClass.Get() : AProjectile::StaticClass();`
`.Get()` makes both branches `UClass*` → unambiguous; behavior identical (null-safe fallback to base `AProjectile`). Nothing else changed — the AoERadius pass-through, the `ProjectileClass` property/forward-decl/include, and the spawn guard are byte-identical to the QA-passed state. Also corrected the board spec (TASKBOARD.md:690) to the `.Get()` form so a re-read can't reintroduce it.
