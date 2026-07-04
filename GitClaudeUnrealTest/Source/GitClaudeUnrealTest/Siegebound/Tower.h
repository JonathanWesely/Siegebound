// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Siegebound/Building.h"
#include "Tower.generated.h"

/**
 *  Siegebound auto-firing tower (GDD §3.7, TASK-027): an ABuilding that fires
 *  homing projectiles. BP child: BP_Building_ArrowTower (CardID ArrowTower —
 *  row: 150 HP, 15 damage, 900 range, 1.5 s cadence; all read from DT_Cards).
 *
 *  - Every Cadence seconds it acquires the NEAREST enemy that is a unit or
 *    the hero — §3.7 "targets units/hero": ACastle, ABuilding (self and enemy
 *    towers/walls alike), and every other ITeamAgent type are excluded by a
 *    POSITIVE class gate — within Range, and fires one AProjectile at it:
 *    InitProjectile(own team, target, row Damage, USiegeDamageType_Projectile).
 *    The §3.0 50%-vs-castle scaling is castle-side (TASK-026); towers never
 *    target castles, but the type tag stays honest regardless.
 *  - No target in range = idle: the looping timer simply re-scans next cadence
 *    (TASK-027 spec). No persistent target — each shot re-acquires, so a fired
 *    projectile always chases a target that was alive, hostile, and in range
 *    at fire time (the TASK-026 "shooters re-validate at fire time" contract).
 *  - Cadence guard (qa/TASK-021-report.md WARN-1, BINDING): SetTimer with a
 *    rate <= 0 CLEARS a timer instead of scheduling it — and a Cadence <= 0
 *    row means "this card does not attack" anyway (Wall, Miner). Such a row
 *    NEVER arms the loop (logged; the tower stands mute) and is never clamped
 *    into validity. Positive-but-degenerate cadences are floored at 0.05 s
 *    (the ASummonedUnit MinAttackCadence mirror).
 *  - No friendly fire (§3.0): acquisition is enemy-only, and the projectile's
 *    own same-team impact gate backstops it. For a NON-PAWN shooter that gate
 *    is the only receiver-independent protection (qa/TASK-026 ruling 3 —
 *    load-bearing, untouched by this task): a tower has no instigator pawn to
 *    attribute, so receivers resolve tower hits as unattributable-and-apply
 *    (their documented contract). Owner is set on the projectile; Instigator
 *    is deliberately left unset (TASK-026 handoff, non-pawn shooter form).
 *  - Row-driven variants (TASK-056, NO new class): the acquire/idle loop above
 *    is shared; two extra row cells re-shape it. A row with AoERadius > 0
 *    (BombTower — 180 HP, 25 damage, 800 range, 2.5 s, AoERadius 250) fires an
 *    AoE projectile that blasts every enemy within the radius at the impact point
 *    (anti-swarm splash, §3.7). A row with MinRange > 0 (BallistaTower — 120 HP,
 *    45 damage, 1400 range, 3.0 s, MinRange 300) acquires the nearest enemy
 *    WITHIN Range but OUTSIDE the MinRange blind spot (§4); a closer target is
 *    ignored and the loop re-scans next cadence. Both stay plain ATower, so
 *    BP_Building_BombTower/BallistaTower parent ATower directly (TASK-063).
 *    ArrowTower (AoERadius 0, MinRange 0) is behavior-unchanged.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ATower : public ABuilding
{
	GENERATED_BODY()

protected:

	/** Clears the fire timer (runs synchronously inside Destroy — a dead tower can never fire again). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  ABuilding stat hook: binds Damage/Range/Cadence from the row the base
	 *  just loaded HP from, then arms the looping fire timer — ONLY when
	 *  Cadence > 0 (qa/TASK-021 WARN-1). First shot lands one full cadence
	 *  after the stats bind; the timer then loops for the tower's lifetime.
	 */
	virtual void OnStatsLoaded(const FCardRow& Row) override;

	/**
	 *  Where projectiles appear, as a world-space offset above the actor origin
	 *  (yaw-invariant). Purely cosmetic — the projectile re-aims at its target
	 *  every tick (TASK-026 homing), so the muzzle never changes what is hit.
	 *  A feel tolerance like AProjectile::ImpactRadius, not a GDD stat; tune on
	 *  BP_Building_ArrowTower to match SM_ArrowTower's real height (TASK-035).
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower")
	FVector MuzzleOffset = FVector(0.f, 0.f, 200.f);

private:

	/** Cadence callback: acquire the nearest valid enemy and fire — or idle and let the loop re-scan next cadence. */
	void ScanAndFire();

	/**
	 *  Nearest alive enemy unit-or-hero within Range (GDD §3.7) and — when
	 *  AttackMinRange > 0 (BallistaTower, TASK-056) — OUTSIDE the MinRange blind
	 *  spot: a target closer than MinRange is ignored (§4). Positive class gate
	 *  (ASummonedUnit incl. subclasses, AHeroCharacter) — castles, buildings, and
	 *  future non-pawn ITeamAgent types can never be acquired. Returns nullptr when
	 *  nothing valid is in the [MinRange, Range] ring.
	 */
	AActor* AcquireTarget() const;

	/** Spawns one AProjectile at the muzzle and arms it via InitProjectile (TASK-026 contract, non-pawn shooter form). */
	void FireProjectileAt(AActor* Target);

	/** Damage per shot, from the card row (ArrowTower: 15). Carried by the projectile; this actor never re-reads DT_Cards. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	float AttackDamage = 0.f;

	/** Acquisition range in units, from the card row (ArrowTower: 900). Anything beyond is ignored (§3.7). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 0.f;

	/** Seconds between shots, from the card row (ArrowTower: 1.5). <= 0 means the loop was never armed. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	float AttackCadence = 0.f;

	/**
	 *  Splash radius, from the card row (TASK-056; BombTower 250, 0 = single
	 *  target). > 0 makes each shot an AoE projectile that blasts every enemy
	 *  within the radius at the impact point (FSiegeCombatStatics::ApplyRadialDamage,
	 *  carried on the projectile via InitProjectile); 0 (ArrowTower/BallistaTower)
	 *  fires the unchanged single-target projectile.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	float AttackAoERadius = 0.f;

	/**
	 *  Inner blind-spot radius, from the card row (TASK-056; BallistaTower 300,
	 *  0 = none). When > 0, AcquireTarget ignores any enemy CLOSER than this — the
	 *  tower only hits targets in the [MinRange, Range] ring (§4). 0
	 *  (ArrowTower/BombTower) disables the check, so acquisition is unchanged.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	float AttackMinRange = 0.f;

	/** Drives ScanAndFire every AttackCadence seconds, armed once in OnStatsLoaded (Cadence > 0 rows only). */
	FTimerHandle FireTimerHandle;
};
