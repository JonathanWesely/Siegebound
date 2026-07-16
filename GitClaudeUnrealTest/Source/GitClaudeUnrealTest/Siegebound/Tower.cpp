// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Tower.h"

#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeMeshJuiceComponent.h"
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

namespace
{
	/** §6 projectile-fire audio (TASK-179) — null-safe soft path; the sound arrives in TASK-180. */
	const TCHAR* TowerProjectileFireSoundPath = TEXT("/Game/Audio/S_ProjectileFire");

	/**
	 *  Floor for POSITIVE cadence cells only (mirror of ASummonedUnit's
	 *  MinAttackCadence, TASK-004): a 0.001 s row would be a 1000 shots/s
	 *  runaway. A Cadence <= 0 is NEVER floored into validity — it means
	 *  "this card does not attack" (Wall, Miner) and must not schedule at all
	 *  (qa/TASK-021-report.md WARN-1).
	 */
	constexpr float MinTowerCadence = 0.05f;
}

ATower::ATower()
{
	// TASK-101 names block: the chain-hit burst authored by art TASK-108
	// (CONVENTIONS "Spells & Set III (M5)": Crystal Tower's chain visual is
	// /Game/VFX/NS_ChainZap). Soft path only — resolved in OnStatsLoaded, and
	// ONLY for chain rows, so the three projectile towers never load it. The
	// asset may not exist yet (art runs in parallel to the same spec):
	// missing = one warning + no visual, never a crash.
	ChainZapEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/VFX/NS_ChainZap.NS_ChainZap")));
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

	// TASK-101 chain columns (M5 ruling 9, same row-driven-variant discipline as
	// TASK-056 — still no new class): ChainTargets > 0 (CrystalTower 3) swaps the
	// projectile for an instant chain zap; ChainFalloff (CrystalTower 5) is the
	// flat damage lost per bounce (15/10/5 with Damage 15). Both default 0 for
	// Arrow/Bomb/Ballista, leaving their fire path byte-for-byte unchanged.
	// Columns land in FCardRow via TASK-097 (pinned names, same compile batch).
	AttackChainTargets = Row.ChainTargets;
	AttackChainFalloff = Row.ChainFalloff;

	// data smells, surfaced loudly and never fudged (the LoadStats/§3.0
	// discipline — stats are data; code never silently "repairs" them):
	// a negative falloff would GROW damage per bounce (CrystalTower authors 5);
	// chain + AoERadius on one row is contradictory — the chain path wins and
	// the splash cell is ignored (no CrystalTower cell authors both).
	if (AttackChainTargets > 0 && AttackChainFalloff < 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': row '%s' has ChainFalloff %d < 0 — each bounce would deal MORE damage (CrystalTower authors 5). Check Docs/Data/cards.csv."),
			*GetNameSafe(this), *CardID.ToString(), AttackChainFalloff);
	}
	if (AttackChainTargets > 0 && AttackAoERadius > 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATower '%s': row '%s' authors BOTH ChainTargets %d and AoERadius %.1f — a chain tower fires no projectile, so the splash cell is ignored (chain wins). Check Docs/Data/cards.csv."),
			*GetNameSafe(this), *CardID.ToString(), AttackChainTargets, AttackAoERadius);
	}

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

	// chain rows resolve their zap visual ONCE, here (the AProjectile
	// BeginPlay/TASK-020 cache pattern) — never per zap. Placed after the
	// cadence guard on purpose: a tower that can never fire needs no VFX. The
	// three projectile towers (ChainTargets 0) never load it. Missing asset
	// (TASK-108 authors it in parallel) = this ONE warning; zaps still deal
	// full damage with no visual, never a crash. Cleared-in-BP (IsNull) is a
	// silent designer opt-out.
	if (AttackChainTargets > 0 && !ChainZapEffect.IsNull())
	{
		CachedChainZapEffect = ChainZapEffect.LoadSynchronous();
		if (!CachedChainZapEffect)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ATower '%s': chain zap effect '%s' failed to load — chain hits will show no VFX (art TASK-108 authors /Game/VFX/NS_ChainZap)."),
				*GetNameSafe(this), *ChainZapEffect.ToString());
		}
	}

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

	// TASK-101 freeze gate (M5 ruling 14): ALL tower firing — projectile AND
	// chain — sits behind this single choke point, gated on TASK-099's
	// ABuilding spell-freeze API (FrostNova; the API task stays out of this
	// file per ruling 14, this task consumes it). The cadence loop keeps
	// ticking while frozen — a frozen tower skips its shots (it doesn't even
	// scan) and resumes on the first cadence tick after the freeze expires:
	// freeze pauses the attack cadence, it never tears down the timer. A tower
	// that is never frozen sees a constant false here — Arrow/Bomb/Ballista
	// behavior is unchanged (task acceptance item 4).
	if (IsFrozen())
	{
		return;
	}

	AActor* Target = AcquireTarget();
	if (!Target)
	{
		// idle — the loop re-scans next cadence (TASK-027 spec)
		return;
	}

	// §6 tower recoil on fire (TASK-155): kick the VisualMesh back opposite the fire
	// direction and ease it home before the next cadence. Applied HERE (before the
	// delivery branch) so BOTH projectile and chain shots recoil, using the tower→target
	// direction. Cosmetic — targeting/damage below are untouched; null-safe.
	if (MeshJuiceComponent)
	{
		MeshJuiceComponent->PlayRecoil(Target->GetActorLocation() - GetActorLocation());
	}

	// TASK-101 (M5 ruling 9): a chain row (CrystalTower, ChainTargets 3) zaps
	// instantly instead of firing a projectile — same acquisition, same
	// cadence, different delivery. ChainTargets 0 (Arrow/Bomb/Ballista) keeps
	// the projectile path byte-for-byte unchanged.
	if (AttackChainTargets > 0)
	{
		FireChainZapAt(Target);
	}
	else
	{
		FireProjectileAt(Target);
	}
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
		// valid + alive + §3.7 class gate + enemy-only — the shared tower
		// targeting gate (factored out for the TASK-101 chain bounce search;
		// the checks are the exact ones that used to live inline here).
		if (!IsAcquirableEnemy(Candidate))
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

bool ATower::IsAcquirableEnemy(const AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return false;
	}

	// §3.7 "targets units/hero" — POSITIVE class gate: only summoned units
	// (subclasses included: TASK-025 miners are raidable investments, §3.3)
	// and the hero qualify; castles, buildings (self included), and any
	// future ITeamAgent type are excluded by default. Liveness per type is
	// a partial mirror of ASummonedUnit::IsTargetAlive restricted to the
	// two classes a tower may target: a dead hero is HIDDEN, not destroyed
	// (TASK-003), and dying units flag bDead before their Destroy lands
	// (TASK-004 same-frame window). The chain bounce search (TASK-101) shares
	// this gate — the zap bounces to exactly what the tower may target (§3.7),
	// so a chain can never reach a castle or a building either.
	if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate))
	{
		if (Unit->IsUnitDead())
		{
			return false;
		}
	}
	else if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Candidate))
	{
		if (Hero->IsDead())
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// no friendly fire (GDD §3.0): enemies only. Native cast is valid —
	// UTeamAgent is NotBlueprintable (TASK-001 ruling).
	const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
	return Agent && Agent->GetTeamId() != Team;
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

	// §6 projectile-fire audio (TASK-179): world one-shot at the muzzle, null-safe.
	USiegeFeedbackLibrary::PlayWorldSound(this, TowerProjectileFireSoundPath, MuzzleLocation);
}

void ATower::FireChainZapAt(AActor* PrimaryTarget)
{
	UWorld* World = GetWorld();
	if (!World || !PrimaryTarget)
	{
		return;
	}

	// ---- phase 1: select the WHOLE chain from one fire-time snapshot --------
	// The zap is INSTANT (M5 ruling 9 — no projectile actor), so target
	// selection reads the world exactly once, at fire time. Damage lands in
	// phase 2, AFTER selection is complete: a mid-chain cascade death (hit 1
	// kills a Sapper whose suicide blast kills the would-be hit 2) can never
	// re-shape a zap that conceptually already happened — the receiver's own
	// dead-gate zeroes that hit instead (dying units flag bDead, TASK-004).
	// Deterministic by construction: nearest-first with FVector::DistSquared,
	// same gather-then-filter pattern as AcquireTarget (a handful of bounces
	// over a dozen agents on a 1.5 s cadence — trivially cheap).
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	// bounce candidates: everything the tower may target (§3.7 class gate +
	// liveness + enemy-only — the SAME IsAcquirableEnemy gate as the primary
	// acquisition, shared not mirrored), minus the primary itself ("no target
	// hit twice per zap"). Range from the TOWER is deliberately not checked
	// here: only the primary is range-gated (AcquireTarget); bounces are gated
	// by ChainBounceRadius from the PREVIOUS target (ruling 9), so a chain may
	// legally step outside the tower's own 800 ring.
	TArray<AActor*> Candidates;
	Candidates.Reserve(TeamAgents.Num());
	for (AActor* Candidate : TeamAgents)
	{
		if (Candidate != PrimaryTarget && IsAcquirableEnemy(Candidate))
		{
			Candidates.Add(Candidate);
		}
	}

	TArray<AActor*> ChainHits;
	ChainHits.Reserve(AttackChainTargets);
	ChainHits.Add(PrimaryTarget);

	const double BounceRadiusSq = FMath::Square(static_cast<double>(ChainBounceRadius));
	// the bounce search origin walks the chain: ALWAYS the previous target's
	// position, never the tower's (acceptance item: "bounce measured from the
	// previous target, not the tower").
	FVector PreviousLocation = PrimaryTarget->GetActorLocation();

	while (ChainHits.Num() < AttackChainTargets)
	{
		AActor* NextTarget = nullptr;
		double NextDistSq = TNumericLimits<double>::Max();
		int32 NextIndex = INDEX_NONE;

		for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
		{
			// origin-to-origin, matching the primary acquisition's metric (the
			// qa/TASK-026 NIT-4 "no fourth closest-point mirror" ruling holds
			// here too: every legal chain target is a pawn whose ~35 uu capsule
			// is noise against a 350-unit bounce radius).
			const double DistSq = FVector::DistSquared(PreviousLocation, Candidates[CandidateIndex]->GetActorLocation());
			if (DistSq <= BounceRadiusSq && DistSq < NextDistSq)
			{
				NextTarget = Candidates[CandidateIndex];
				NextDistSq = DistSq;
				NextIndex = CandidateIndex;
			}
		}

		if (!NextTarget)
		{
			// nothing unhit within ChainBounceRadius of the previous target —
			// the chain ends short (ruling 9: fewer enemies = a shorter chain;
			// never a re-search from the tower, never a wasted re-scan).
			break;
		}

		ChainHits.Add(NextTarget);
		// "no target hit twice per zap": a selected target leaves the pool.
		Candidates.RemoveAtSwap(NextIndex);
		PreviousLocation = NextTarget->GetActorLocation();
	}

	// ---- phase 2: apply, in bounce order -------------------------------------
	for (int32 HitIndex = 0; HitIndex < ChainHits.Num(); ++HitIndex)
	{
		AActor* HitActor = ChainHits[HitIndex];
		// a cascade death between phase 2 hits (see above) can tear an actor
		// down mid-loop — a torn-down actor gets neither damage nor VFX.
		if (!IsValid(HitActor))
		{
			continue;
		}

		// ruling 9 falloff: hit n (0-indexed) takes Dmg − n×ChainFalloff,
		// floored at 0 — CrystalTower's 15/5 row lands 15/10/5. AttackDamage and
		// AttackChainFalloff are row cells (GDD §3.0: never hardcoded).
		const float HitDamage = FMath::Max(AttackDamage - static_cast<float>(HitIndex) * static_cast<float>(AttackChainFalloff), 0.f);

		// tagged USiegeDamageType_Projectile — the tower attack family (ruling
		// 9), so the castle-side §3.0 50% rule stays honest even though a chain
		// can never reach a castle (§3.7 class gate). EventInstigator is a
		// tower's usual null (non-pawn, no controller — the TASK-026/027
		// non-pawn shooter form); DamageCauser is the TOWER itself, an
		// ITeamAgent — receivers resolve the attacking team from it (their
		// documented step-2 chain), so their same-team gate actively backstops
		// the enemy-only selection above (STRONGER attribution than the
		// projectile path's unattributable-and-apply; flagged in the handoff).
		if (HitDamage > 0.f)
		{
			UGameplayStatics::ApplyDamage(HitActor, HitDamage, GetInstigatorController(), this, USiegeDamageType_Projectile::StaticClass());
		}

		// the zap burst at EVERY chain hit (spec item 2): with no projectile to
		// watch, NS_ChainZap is the attack's only read (§6 one-frame
		// readability) — so it spawns per hit regardless of what the receiver
		// did with the damage (unlike the projectile puff's damage-landed gate;
		// flagged in the handoff). Null cache (asset missing or floor-zeroed
		// row) = no visual, never a crash.
		if (CachedChainZapEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, CachedChainZapEffect, HitActor->GetActorLocation());
		}
	}
}
