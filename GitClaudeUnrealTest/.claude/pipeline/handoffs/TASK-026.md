# TASK-026 Handoff — Projectile actor + damage types + castle damage scaling (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no TASKBOARD edit per dispatch; M2 wave 1 batch pattern)

## Files

1. `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h` (new) — `USiegeDamageType_Melee`, `USiegeDamageType_Projectile` (pure-tag UDamageType subclasses, no logic; Siege/Spell reserved M4/M5 per CONVENTIONS registry)
2. `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.cpp` (new) — intentionally empty translation unit
3. `Source/GitClaudeUnrealTest/Siegebound/Projectile.h` (new) — `AProjectile`
4. `Source/GitClaudeUnrealTest/Siegebound/Projectile.cpp` (new)
5. `Source/GitClaudeUnrealTest/Siegebound/Castle.h` — `TakeDamage` doc comment only (M1 TODO replaced with the M2 scaling contract + TODO(M4)/TODO(M5))
6. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` — `TakeDamage` scaling block + return value + one include (`Siegebound/DamageTypes.h`). Nothing else in the castle touched: friendly-fire ignore, destroyed-once broadcast, HP-changed broadcast ordering, ResetCastle, BeginPlay seed are all byte-identical to M1.

`GitClaudeUnrealTest.Build.cs` NOT touched — Niagara already a public dependency (TASK-016). `CardRow.h` / `cards.csv` NOT touched (TASK-021 owns them this wave). `SummonedUnit.h/.cpp` NOT touched (TASK-028 owns them).

## Castle scaling semantics (what changed vs M1)

In `ACastle::TakeDamage`, AFTER the unchanged destroyed/<=0/friendly-fire early-outs and the unchanged `Super::TakeDamage` call:

```cpp
float ScaledDamage = ActualDamage;
const UClass* IncomingDamageType = DamageEvent.DamageTypeClass.Get();
if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Projectile::StaticClass()))
{
    ScaledDamage *= 0.5f;   // GDD §3.0 anti-sniping rule
}
CurrentHP = FMath::Max(CurrentHP - ScaledDamage, 0.0f);
...
return ScaledDamage;
```

- `USiegeDamageType_Projectile` (and any subclass) = 50%; melee/default/null/unknown = 100%. Null `DamageTypeClass` (default-constructed FDamageEvent) is explicitly handled = 100%.
- Scaling applied ONLY in ACastle (M2 ruling). Units/hero take listed damage — their TakeDamage overrides were not touched; TASK-027's ABuilding must likewise NOT scale.
- M1 attackers (hero melee, footman) pass `UDamageType::StaticClass()` → 100% path, math and return value bit-identical to M1.

## AProjectile contract — what TASK-028 (ranged units) and TASK-027 (towers) consume

```cpp
// pawn shooter (TASK-028 archer) — inside the unit:
FActorSpawnParameters Params;
Params.Owner = this;
Params.Instigator = this;   // REQUIRED for pawn shooters — see attribution note below
Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // optional; projectile has no collision
if (AProjectile* P = GetWorld()->SpawnActor<AProjectile>(AProjectile::StaticClass(), FTransform(FireRotation, MuzzleLocation), Params))
{
    P->InitProjectile(Team, Target, RowDamage, USiegeDamageType_Projectile::StaticClass());
}

// non-pawn shooter (TASK-027 tower): identical, but leave Params.Instigator unset (towers are not pawns).
```

- `InitProjectile(ETeamId InTeam, AActor* InTarget, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass)` — `BlueprintCallable`, call ONCE, immediately after plain `SpawnActor` (or between `SpawnActorDeferred` and `FinishSpawning` — both supported). Re-init is ignored with a warning: one projectile per shot. Damage comes from the SHOOTER's card row — the projectile never reads DT_Cards.
- Null/dead target at init: warned, projectile expires harmlessly on its first tick (never a crash). Un-init'd projectile: expires (warned) on first tick.
- Behavior: flies at `Speed` (UPROPERTY, default 1500 — GDD §3.0) re-aiming every tick at the target's current location; impact = closest-point-on-collision reach test within max(ImpactRadius 30, this tick's step); on impact applies damage + spawns NS_Damage puff at the impact point (only if damage actually landed) and destroys itself; target dies mid-flight → continues to last known location, expires with no damage/VFX; hard 5 s lifespan cap.
- Spawn transform scale is ignored by the visual (scene root + fixed 0.15-scaled sphere child) — spawn with scale 1, don't compensate.
- `GetTeam()` (BlueprintPure) exposes the firing team for PIE/QA checks.

**Attribution note (load-bearing for TASK-027/028):** receivers resolve the attacker team via the TASK-002 chain (instigator controller's pawn → damage causer → causer's instigator pawn). The projectile passes `GetInstigatorController()` as EventInstigator and itself as DamageCauser. Pawn shooters MUST set `SpawnParameters.Instigator = this` so steps 1/3 resolve. Tower shots resolve as unattributable → receivers apply the damage (their documented contract) — safe because the projectile only ever damages the single enemy it was fired at, plus its own same-team gate (below).

## Flagged decisions — QA must rule on each

1. **AProjectile does NOT implement ITeamAgent (deliberate).** `ASummonedUnit::AcquireTarget` collects candidates via `GetAllActorsWithInterface(UTeamAgent)` and `IsTargetAlive` treats unknown ITeamAgent types as alive — an ITeamAgent projectile would be acquired and attacked by enemy units mid-flight. Team attribution instead uses the spec's instigator plumbing (see above) plus a projectile-side same-team gate. Team is a plain property (`GetTeam()`).
2. **Impact detection is a distance test, not physics.** Per-tick reach test against the closest point on the INTENDED target's collision only (`ActorGetDistanceToCollision`, ECC_Pawn — house pattern from TASK-003/004); `VisualMesh` is full NoCollision (no overlaps, no nav). Consequences accepted as blockout tier: literal "no collision with friendlies" (or anything), flies through world geometry, impacts the castle at its walls with the closest point doubling as the VFX point (TASK-020 pattern). `ImpactRadius` (30, EditAnywhere) is a feel tolerance, not a GDD stat; the max(radius, step) term prevents low-FPS overshoot.
3. **Projectile-side same-team gate at impact.** If the Init'd target turns out same-team (buggy shooter), the projectile expires harmlessly with a warning instead of applying damage — belt-and-braces over receiver-side checks, which can't resolve tower (non-pawn) shooters.
4. **`ACastle::TakeDamage` now returns the SCALED amount** (the HP actually removed) rather than `Super`'s pre-scale value — AActor's "damage actually applied" contract. Melee path returns exactly the M1 value; all known callers only test `> 0` (TASK-016/020 feedback), so behavior is unchanged for them.
5. **Harmless expiries spawn no VFX.** The NS_Damage puff fires only when `ApplyDamage` returned > 0 (mirrors the TASK-016/020 QA-approved return-value pattern). Lost-target expiry, lifetime expiry, same-team gate, and zeroed hits (destroyed castle / dead hero) all despawn silently.
6. **Destroyed-but-extant targets are flown to, not dropped.** A destroyed castle / dead-hidden hero stays `IsValid`, so the projectile completes its flight and "impacts" for 0 damage (their TakeDamage guards) → no puff, despawn. Net-identical to the harmless-expiry rule without duplicating liveness APIs (`IsCastleDestroyed`/`IsDead`/`IsUnitDead`) in a second class.
7. **Lifetime cap via `InitialLifeSpan = 5.f`** (engine-managed, Epic's own FPS-template projectile pattern) instead of a custom timer.
8. **`USiegeDamageType_Melee` is declared but nothing passes it yet.** CONVENTIONS: "melee needs no tag" — existing hero/unit melee keeps `UDamageType::StaticClass()` untouched (M1 byte-identical rule). The class exists for the registry and future explicit tagging; castle math is identical either way.
9. **Scaling uses `IsChildOf`,** so future subclasses of `USiegeDamageType_Projectile` inherit the 50% rule automatically.
10. **`GetDistanceToTarget` helper is hand-mirrored from `ASummonedUnit`** (private static there; its header is TASK-028's file this wave, so no shared-utility refactor). Same precedent as the TryGetInstigatorTeam/TryGetDamageTeam mirror QA accepted in TASK-004.

## Asset references (all soft, null-safe, resolved per M1 house style)

- `/Engine/BasicShapes/Sphere.Sphere` (engine, read-only) — visual, resolved in OnConstruction
- `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue` / `_Red` (TASK-012 assets) — by team, OnConstruction + re-applied at InitProjectile
- `/Game/Variant_Combat/VFX/NS_Damage.NS_Damage` (READ-ONLY donor) — impact puff, cached once at BeginPlay (TASK-020 pattern)
