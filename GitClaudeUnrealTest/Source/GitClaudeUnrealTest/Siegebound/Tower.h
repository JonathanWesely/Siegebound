// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Siegebound/Building.h"
#include "UObject/SoftObjectPtr.h"
#include "Tower.generated.h"

class UNiagaraSystem;

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
 *  - Chain zap (TASK-101, M5 ruling 9 — CrystalTower, still NO new class): a
 *    row with ChainTargets > 0 (CrystalTower — 150 HP, 15 damage, 800 range,
 *    1.5 s, ChainTargets 3, ChainFalloff 5) fires an INSTANT chain zap instead
 *    of a projectile. Primary = the same nearest-enemy acquisition as every
 *    other tower; the zap then bounces to up to ChainTargets−1 further enemies,
 *    each the nearest not-yet-hit enemy within ChainBounceRadius of the
 *    PREVIOUS target (measured target-to-target, never tower-to-target); hit n
 *    (0-indexed) takes Damage − n×ChainFalloff, floored at 0 (15/10/5 with the
 *    CrystalTower row); no friendly fire, no target hit twice per zap; damage
 *    tagged USiegeDamageType_Projectile (tower attack family). Fewer enemies
 *    in bounce reach = a shorter chain — never a re-search from the tower.
 *    /Game/VFX/NS_ChainZap spawns at every chain hit (soft path, null-safe,
 *    log-once; art lands in TASK-108). Arrow/Bomb/Ballista rows author
 *    ChainTargets 0, so their fire path is byte-for-byte unchanged.
 *  - Freeze gate (TASK-101, M5 ruling 14): ALL firing — projectile AND chain —
 *    gates on !IsFrozen() (TASK-099's ABuilding spell-freeze API, FrostNova).
 *    The cadence timer keeps looping while frozen; the tower just skips its
 *    shots and resumes on the first cadence tick after the freeze expires
 *    (freeze pauses the attack cadence, it never tears down the loop).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ATower : public ABuilding
{
	GENERATED_BODY()

public:

	/** Sets the NS_ChainZap soft path default (only chain rows ever load it). */
	ATower();

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

	/**
	 *  Chain bounce search radius: each bounce acquires the nearest not-yet-hit
	 *  enemy within this distance of the PREVIOUS target (M5 ruling 9 —
	 *  target-to-target, never tower-to-target). Mechanic RULE, not a card stat
	 *  (GDD §4 Chain leaves it unspecified; manager-defined = 350), so it lives
	 *  here as a UPROPERTY default per the Rally/Charge-magnitude precedent and
	 *  never enters cards.csv. Only read when the row's ChainTargets > 0.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower", meta = (ClampMin = "0"))
	float ChainBounceRadius = 350.f; // GDD §4 Chain, manager-defined (M5 ruling 9)

	/**
	 *  Per-hit chain zap burst (TASK-101 names block): /Game/VFX/NS_ChainZap,
	 *  authored by art TASK-108. Soft path set in the constructor, resolved and
	 *  cached ONCE in OnStatsLoaded — and only for chain rows (ChainTargets > 0);
	 *  Arrow/Bomb/Ballista towers never touch it. Missing/unbuilt asset = one
	 *  warning, zaps still deal damage with no visual, never a crash.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Tower")
	TSoftObjectPtr<UNiagaraSystem> ChainZapEffect;

private:

	/** Cadence callback: freeze-gate, then acquire the nearest valid enemy and fire (chain zap when the row chains, projectile otherwise) — or idle and let the loop re-scan next cadence. */
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

	/**
	 *  The tower targeting gate, factored out of AcquireTarget so the chain
	 *  bounce search (TASK-101) shares it instead of hand-mirroring (the
	 *  qa/TASK-026 NIT-4 "no more mirrors" discipline): valid + alive + the §3.7
	 *  POSITIVE class gate (ASummonedUnit incl. subclasses, AHeroCharacter —
	 *  castles, buildings, and future ITeamAgent types can never qualify) +
	 *  enemy-team only (§3.0 no friendly fire). Distance is deliberately NOT in
	 *  here — the primary ring gate and the bounce radius gate differ per call
	 *  site.
	 */
	bool IsAcquirableEnemy(const AActor* Candidate) const;

	/** Spawns one AProjectile at the muzzle and arms it via InitProjectile (TASK-026 contract, non-pawn shooter form). */
	void FireProjectileAt(AActor* Target);

	/**
	 *  TASK-101 (M5 ruling 9): resolves one INSTANT chain zap — no projectile
	 *  actor. Selects the whole chain from a single fire-time snapshot of live
	 *  enemies (PrimaryTarget, then up to AttackChainTargets−1 bounces, each the
	 *  nearest not-yet-hit enemy within ChainBounceRadius of the PREVIOUS
	 *  target; no candidates in reach = the chain just ends short), THEN applies
	 *  hit n = AttackDamage − n×AttackChainFalloff (floored at 0) to each in
	 *  bounce order, tagged USiegeDamageType_Projectile, with this tower as
	 *  DamageCauser (an ITeamAgent — receivers resolve the tower's team, so
	 *  their same-team gate backstops the enemy-only selection). NS_ChainZap
	 *  spawns at every chain hit (null-safe).
	 */
	void FireChainZapAt(AActor* PrimaryTarget);

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

	/**
	 *  Total targets hit per attack, from the card row (TASK-101 / M5 ruling 9;
	 *  CrystalTower 3, 0 = not a chain tower). > 0 swaps FireProjectileAt for
	 *  the instant FireChainZapAt on every shot; 0 (Arrow/Bomb/Ballista) leaves
	 *  the projectile path byte-for-byte unchanged.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	int32 AttackChainTargets = 0;

	/**
	 *  Flat damage lost per bounce, from the card row (TASK-101 / M5 ruling 9;
	 *  CrystalTower 5 ⇒ 15/10/5 with Damage 15). Hit n (0-indexed) takes
	 *  AttackDamage − n×AttackChainFalloff, floored at 0. Only read when
	 *  AttackChainTargets > 0.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Tower", meta = (AllowPrivateAccess = "true"))
	int32 AttackChainFalloff = 0;

	/**
	 *  Hard cache of ChainZapEffect, resolved ONCE in OnStatsLoaded and only for
	 *  chain rows (the AProjectile::CachedImpactEffect / TASK-020 pattern) —
	 *  keeps the Niagara system alive against GC and avoids per-zap sync loads.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedChainZapEffect;

	/** Drives ScanAndFire every AttackCadence seconds, armed once in OnStatsLoaded (Cadence > 0 rows only). */
	FTimerHandle FireTimerHandle;
};
