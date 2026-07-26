# QA Report — TASK-298

Verdict: PASS (re-verify, QA loop 2 — 2026-07-26)

Scope: pre-compile review (build-master compiles in TASK-304). Files: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}`. Cross-checked against `Projectile.h` (`InitProjectile` signature + AoE docs), `Docs/Data/cards.csv`, and the CONVENTIONS "Wizard unit — AoE fireball caster (2026-07-26)" block.

## Re-verify (loop 2) — the loop-1 BLOCKER is fixed
- RESOLVED · SummonedUnit.cpp:2126 — now reads
  `const TSubclassOf<AProjectile> SpawnClass = ProjectileClass ? ProjectileClass.Get() : AProjectile::StaticClass();`
  Both ternary operands are now `UClass*` (`ProjectileClass.Get()` returns `UClass*`; `AProjectile::StaticClass()` returns `UClass*`), so there is a single unambiguous common type — the C2445 ambiguous-conditional-expression error is gone. The `UClass*` result assigns cleanly to `const TSubclassOf<AProjectile>` via the converting constructor, and null-safety is unchanged (a null `ProjectileClass` yields `nullptr` from `.Get()`, so the ternary condition still selects the `AProjectile::StaticClass()` fallback). Matches this codebase's own precedent (`BattlefieldScatter.cpp:1038`).
- DIFF-CHECK — nothing else changed. `FireProjectileAt` (SummonedUnit.cpp:2098-2141) is byte-identical to the loop-1 state except that single `.Get()` on line 2126: the AoERadius pass-through (2136), the spawn guard (2127), the pawn-shooter `Instigator`/`Owner`/`AlwaysSpawn` params, and all comments are unchanged. The header is untouched — forward-decl `class AProjectile;` (SummonedUnit.h:15), the `ProjectileClass` UPROPERTY (SummonedUnit.h:372-373), and the complete-type `#include "Siegebound/Projectile.h"` (SummonedUnit.cpp:33) are all identical to loop 1. No new shadow, include, or deprecated-API issue introduced (`.Get()` is the standard, non-deprecated `TSubclassOf` accessor).

## Findings
- No blockers, warnings, or nits remain.

## Verified good (carried from loop 1, re-confirmed)
- **AoERadius==0 invariant — HOLDS.** 5th param defaults `0.f` (Projectile.h:105); only `> 0` routes to `FSiegeCombatStatics::ApplyRadialDamage` (no new AoE routine — the shipped TASK-056 path is reused); the passed value is the row-bound member `AoERadius = Row->AoERadius` (SummonedUnit.cpp:980), declared `float AoERadius = 0.f;` (SummonedUnit.h:887). Archer (col 19 = 0) and Longbowman (col 19 = 0) carry `AoERadius 0` in cards.csv, so `InitProjectile(..., AoERadius)` is byte-for-byte identical to omitting the arg for them. `FireProjectileAt` is reached only under `if (bRangedAttack)` (SummonedUnit.cpp:1927/1935), and among `ASummonedUnit` rows only the Wizard is `bRanged` + `AoERadius 250`. Towers fire via the separate `ATower` path (Tower.cpp:337); the Sapper is `bRanged false` / suicide-melee and detonates via `ApplyDetonation`, never reaching `FireProjectileAt`. No non-Wizard unit can obtain a non-zero radius.
- **TRUTH gate** — a `bRanged` unit with `AoERadius > 0` genuinely splashes through the proven `HandleImpact`→`ApplyRadialDamage` path (enemies-in-radius only, no friendly fire, castle-side 50% projectile scaling preserved), so M7.7's auto-generated "splash radius 250" description is truthful.
- **ProjectileClass hook** — UPROPERTY specifiers sane (EditDefaultsOnly, BlueprintReadOnly, Category `Siegebound|Unit`), defaults null, GC-safe (TSubclassOf/UPROPERTY); complete-type usage confined to the .cpp; spawn guard `if (AProjectile* Projectile = World->SpawnActor<AProjectile>(SpawnClass, ...))` correct; world/target null-guarded (2100-2104).
- **No deprecated UE 5.8 APIs**, no stray comment terminators, no parked/dead code, no changed existing signatures.

## Notes for build-master
Ready for the TASK-304 compile. The change is a clean single-target/splash pass-through plus a null-safe per-unit projectile-class hook — nothing else in the TU needs attention. Compile-verify at TASK-304 as scheduled; pair with the TASK-299 `DT_Cards` reimport.

---
## History
- **Loop 1 (2026-07-26) — FAIL · 1 BLOCKER:** SummonedUnit.cpp:2126 mixed `TSubclassOf<AProjectile>` and `UClass*` in the ternary → ambiguous conditional-expression result → MSVC C2445 under UE 5.8's `/permissive-` + `/Zc:ternary`. Prescribed fix: add `.Get()` (or type the local `UClass*`). All other checks passed. Routed back to gameplay-programmer.
- **Loop 2 (2026-07-26) — PASS:** the exact `.Get()` fix was applied; diff-checked that it is the only delta. Blocker cleared.
