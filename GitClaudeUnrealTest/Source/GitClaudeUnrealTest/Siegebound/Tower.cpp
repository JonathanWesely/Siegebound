// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Tower.h"

#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

namespace
{
	/**
	 *  Floor for POSITIVE cadence cells only (mirror of ASummonedUnit's
	 *  MinAttackCadence, TASK-004): a 0.001 s row would be a 1000 shots/s
	 *  runaway. A Cadence <= 0 is NEVER floored into validity — it means
	 *  "this card does not attack" (Wall, Miner) and must not schedule at all
	 *  (qa/TASK-021-report.md WARN-1).
	 */
	constexpr float MinTowerCadence = 0.05f;
}

void ATower::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ATower::OnStatsLoaded(const FCardRow& Row)
{
	Super::OnStatsLoaded(Row);

	// bind the attack stats from the row the base just loaded (GDD §3.0: never
	// hardcoded — ArrowTower authors 15 damage / 900 range / 1.5 cadence in
	// Docs/Data/cards.csv).
	AttackDamage = Row.Damage;
	AttackRange = Row.Range;

	// TASK-056 row-driven variants (no new class): AoERadius > 0 → each shot is an
	// AoE projectile (BombTower 250); MinRange > 0 → blind-spot acquire (Ballista
	// 300). Both default 0 for ArrowTower, leaving its behavior unchanged. Stats
	// from DT_Cards (GDD §3.0), never hardcoded.
	AttackAoERadius = Row.AoERadius;
	AttackMinRange = Row.MinRange;

	// a blind spot that swallows the whole range means the tower can never acquire
	// anything — surface it (don't clamp: stats are data, never fudged in code —
	// the LoadStats/§3.0 discipline). ArrowTower/BombTower (MinRange 0) skip this.
	if (AttackMinRange > 0.f && AttackMinRange >= AttackRange)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': row '%s' MinRange %.1f >= Range %.1f — the blind spot covers the whole range; this tower can never acquire a target (BallistaTower authors 300/1400). Check Docs/Data/cards.csv."),
			*GetNameSafe(this), *CardID.ToString(), AttackMinRange, AttackRange);
	}

	// qa/TASK-021-report.md WARN-1 (BINDING): FTimerManager::SetTimer with a
	// rate <= 0 CLEARS the timer instead of scheduling it — and a Cadence <= 0
	// row means "this card does not attack" anyway. Refuse to arm, and never
	// clamp a non-attacking row into a firing one (a 0.05 s "fix" would be a
	// 20 shots/s bug dressed as a repair).
	if (Row.Cadence <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': row '%s' has Cadence %.2f — a tower needs a positive cadence to fire (non-attacking rows like Wall belong on plain ABuilding). Tower stands but never fires."),
			*GetNameSafe(this), *CardID.ToString(), Row.Cadence);
		return;
	}

	if (Row.Damage <= 0.f || Row.Range <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': row '%s' has Damage %.1f / Range %.1f — this tower can never hurt anything (ArrowTower authors 15/900). Check Docs/Data/cards.csv."),
			*GetNameSafe(this), *CardID.ToString(), Row.Damage, Row.Range);
	}

	AttackCadence = FMath::Max(Row.Cadence, MinTowerCadence);

	// §3.7 "every 1.5 s": ONE looping timer, armed once, never stopped while
	// the tower lives — the callback scans and simply idles when nothing is in
	// range ("no target in range = idle, re-scan next cadence", TASK-027 spec).
	// First shot lands one full cadence after the stats bind: a fresh tower is
	// a 1.5 s commitment, not an instant burst.
	GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATower::ScanAndFire, AttackCadence, /*bLoop=*/ true);
}

void ATower::ScanAndFire()
{
	// belt-and-braces: Destroy() clears the timer synchronously via EndPlay, so
	// a destroyed tower should never get here — but firing from a dead tower
	// would be wrong enough to guard anyway.
	if (IsBuildingDestroyed())
	{
		return;
	}

	AActor* Target = AcquireTarget();
	if (!Target)
	{
		// idle — the loop re-scans next cadence (TASK-027 spec)
		return;
	}

	FireProjectileAt(Target);
}

AActor* ATower::AcquireTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// house acquisition pattern (ASummonedUnit::AcquireTarget, TASK-004):
	// interface-wide gather, then filter. A couple of towers scanning a dozen
	// agents on a 1.5 s cadence — trivially cheap.
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	const FVector MyLocation = GetActorLocation();
	const double RangeSq = FMath::Square(static_cast<double>(AttackRange));
	// TASK-056 blind-spot lower bound (BallistaTower 300): 0 for Arrow/Bomb, so
	// their MinRangeSq is 0 and the DistSq < MinRangeSq term below is never true —
	// their acquisition stays byte-for-byte unchanged.
	const double MinRangeSq = FMath::Square(static_cast<double>(AttackMinRange));

	AActor* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();

	for (AActor* Candidate : TeamAgents)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		// §3.7 "targets units/hero" — POSITIVE class gate: only summoned units
		// (subclasses included: TASK-025 miners are raidable investments, §3.3)
		// and the hero qualify; castles, buildings (self included), and any
		// future ITeamAgent type are excluded by default. Liveness per type is
		// a partial mirror of ASummonedUnit::IsTargetAlive restricted to the
		// two classes a tower may target: a dead hero is HIDDEN, not destroyed
		// (TASK-003), and dying units flag bDead before their Destroy lands
		// (TASK-004 same-frame window).
		if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate))
		{
			if (Unit->IsUnitDead())
			{
				continue;
			}
		}
		else if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Candidate))
		{
			if (Hero->IsDead())
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		// no friendly fire (GDD §3.0): enemies only. Native cast is valid —
		// UTeamAgent is NotBlueprintable (TASK-001 ruling).
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent || Agent->GetTeamId() == Team)
		{
			continue;
		}

		// NEAREST within Range (row: 900), origin-to-origin. Deliberately NOT a
		// fourth hand-mirror of the closest-point helper (qa/TASK-026 NIT-4
		// asked for no more mirrors): every legal target is a PAWN whose
		// capsule radius (~35 uu) is noise against a 900-unit gate, and the
		// projectile's own closest-point impact test (TASK-026) governs the
		// actual hit.
		const double DistSq = FVector::DistSquared(MyLocation, Candidate->GetActorLocation());
		// out of range, inside the blind spot (BallistaTower MinRange 300, §4 —
		// same origin-to-origin metric as the Range gate above), or not the closest
		// so far — skip. MinRangeSq == 0 (Arrow/Bomb) makes the blind-spot term
		// inert (DistSq < 0 is impossible), so their acquisition is unchanged.
		if (DistSq > RangeSq || DistSq < MinRangeSq || DistSq >= BestDistSq)
		{
			continue;
		}

		Best = Candidate;
		BestDistSq = DistSq;
	}

	return Best;
}

void ATower::FireProjectileAt(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	// muzzle: a plain world-space lift above the actor origin (yaw-invariant).
	// Purely cosmetic — the projectile re-aims at the target every tick
	// (TASK-026 homing), so where it starts never changes what it hits.
	const FVector MuzzleLocation = GetActorLocation() + MuzzleOffset;

	// initial facing toward the target — cosmetic too (the projectile owns its
	// rotation from its first tick). A degenerate direction (target exactly at
	// the muzzle) yields ZeroRotator, harmless for a sphere.
	const FRotator FireRotation = (Target->GetActorLocation() - MuzzleLocation).GetSafeNormal().Rotation();

	// TASK-026 handoff contract, NON-PAWN shooter form: Owner is all the
	// attribution a tower has — Instigator stays UNSET (towers are not pawns;
	// there is no pawn it would be honest to attribute). Receivers therefore
	// resolve tower hits as unattributable and APPLY them (their documented
	// contract). Friendly fire is still impossible: acquisition above is
	// enemy-only, and the projectile's own same-team impact gate is the
	// receiver-independent backstop (qa/TASK-026 ruling 3 — load-bearing for
	// towers, untouched by this task).
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // projectile has no collision (TASK-026)

	AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), MuzzleLocation, FireRotation, SpawnParams);
	if (!Projectile)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': failed to spawn AProjectile — shot lost this cadence."),
			*GetNameSafe(this));
		return;
	}

	// arm it exactly once, right after spawn (TASK-026 contract): own team, the
	// freshly acquired target, ROW damage, projectile-typed so the castle-side
	// §3.0 scaling reads it, and the ROW AoERadius (TASK-056) — > 0 (BombTower 250)
	// makes the impact a radial blast, 0 (ArrowTower/BallistaTower) the unchanged
	// single-target hit. The target was re-validated at fire time by construction —
	// AcquireTarget only returns alive, hostile, in-range (and, for Ballista,
	// outside-MinRange) candidates.
	Projectile->InitProjectile(Team, Target, AttackDamage, USiegeDamageType_Projectile::StaticClass(), AttackAoERadius);
}
