// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SummonedUnit.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "TimerManager.h"

namespace
{
	/**
	 *  Acceptance radius when advancing on a castle. The castle's box collision is huge
	 *  (~800x800 footprint), so overlap-based reach tests against its bounding CYLINDER
	 *  (radius ~566) would stop the unit well outside attack range on a flat wall face.
	 *  Instead the move targets the castle origin with bStopOnOverlap = false and relies
	 *  on the partial path ending at the nav edge flush against the castle's walls.
	 */
	constexpr float StructureMoveAcceptanceRadius = 50.f;

	/** A looping timer needs a strictly positive rate; guards a zero/negative Cadence cell. */
	constexpr float MinAttackCadence = 0.05f;
}

ASummonedUnit::ASummonedUnit()
{
	// state machine runs on a ~0.25 s timer (TASK-004 spec) — never per-tick.
	// Tick exists SOLELY for the attack-lunge visual (TASK-020): it starts
	// disabled, is enabled only while a <= 0.8×Cadence lunge cycle animates,
	// and is re-disabled at every cycle end. Gameplay logic never ticks.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// AI-driven navmesh walker: the default AAIController possesses us whether
	// the unit was placed in a level or spawned by the card play (TASK-007)
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	// face where we walk, not where the controller looks
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.f, 500.f, 0.f);
	}

	// visual slot for the blueprint child (TASK-010 assigns /Game/Meshes/SM_Footman);
	// mesh intentionally unset in C++. The capsule owns all collision — the visual
	// must neither collide nor carve the navmesh under our own feet.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetCapsuleComponent());
	VisualMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);

	// data contract (TASK-004 names block): stats resolve from this table at BeginPlay,
	// never from code (GDD §3.0). The table is imported in TASK-008 and may not exist yet.
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));

	// blockout impact puff (TASK-020 names block): Variant_Combat donor, READ-ONLY —
	// soft-referenced, never edited (CONVENTIONS template-donor rule). Resolved and
	// cached once at BeginPlay; a BP child may retarget or clear it.
	AttackImpactEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Variant_Combat/VFX/NS_Damage.NS_Damage")));
}

void ASummonedUnit::BeginPlay()
{
	Super::BeginPlay();

	// TASK-020: cache the BP-authored rest pose ONCE, post-construction (BP defaults
	// and construction scripts have run by now — BP_Unit_Footman offsets the mesh down
	// by the capsule half-height, TASK-010). The lunge only ever writes Base + f(elapsed)
	// or exactly Base, so the pose cannot drift no matter how many cycles run.
	// !bVisualMeshBaseCached guard: qa/TASK-020-report.md WARN-1, folded in on this
	// touch per its instruction — a re-BeginPlay after a mid-lunge stream-out must not
	// recapture a lunging pose as the new base. The first BeginPlay is unaffected.
	if (VisualMesh && !bVisualMeshBaseCached)
	{
		VisualMeshBaseRelativeLocation = VisualMesh->GetRelativeLocation();
		bVisualMeshBaseCached = true;
	}

	// TASK-020: resolve the impact effect ONCE — never per attack (no sync-load hitch
	// on the cadence). In practice NS_Damage is already resident by the time a unit
	// spawns (the hero hard-references it via TASK-016/017), making this a lookup,
	// not a disk load. Cleared-in-BP (IsNull) is a silent designer opt-out.
	if (!AttackImpactEffect.IsNull())
	{
		CachedAttackImpactEffect = AttackImpactEffect.LoadSynchronous();
		if (!CachedAttackImpactEffect)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASummonedUnit '%s': impact effect '%s' failed to load — attacks will show no impact VFX."),
				*GetNameSafe(this), *AttackImpactEffect.ToString());
		}
	}

	LoadStatsAndStart();
}

void ASummonedUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ASummonedUnit::InitUnit(ETeamId InTeam, FName InCardID)
{
	Team = InTeam;

	if (bStatsLoaded)
	{
		if (CardID != InCardID)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASummonedUnit '%s': InitUnit card '%s' ignored — stats already bound to '%s' (bind happens once at spawn)."),
				*GetNameSafe(this), *InCardID.ToString(), *CardID.ToString());
		}
		return;
	}

	CardID = InCardID;

	// plain SpawnActor + InitUnit: BeginPlay already ran (and idled without a CardID) — bind now.
	// SpawnActorDeferred callers hit the bStatsLoaded == false, !HasActorBegunPlay() path and
	// BeginPlay does the bind after FinishSpawning.
	if (HasActorBegunPlay())
	{
		LoadStatsAndStart();
	}
}

void ASummonedUnit::FreezeAI()
{
	// idempotent; a dying unit already ran the same shutdown in HandleDeath
	if (bAIFrozen || bDead)
	{
		return;
	}
	bAIFrozen = true;

	// stop the brain: the acquire/state decisions and the attack cadence. The flag
	// additionally gates LoadStatsAndStart/UpdateState/PerformAttack, so a frozen
	// unit can never be restarted (e.g. by a late InitUnit on a never-bound unit).
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

	// cancel any in-flight lunge: VisualMesh back to EXACTLY the cached rest pose
	// and tick off (TASK-020 zero-drift contract — zero residual offset)
	StopAttackLunge();

	// stop the walk. This also covers subclasses' moves (AMinerUnit's gold-node
	// walk, TASK-025): StopMovement aborts whatever path request is in flight.
	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}

	// park as Idle until destroyed (match-end freeze, TASK-024 contract)
	State = ESummonedUnitState::Idle;
	CurrentTarget = nullptr;
	CurrentMoveGoal = nullptr;
}

void ASummonedUnit::LoadStatsAndStart()
{
	// bAIFrozen: a match-end-frozen unit stays parked (TASK-028) — even a late
	// InitUnit on a never-bound unit must not start the state machine.
	if (bDead || bStatsLoaded || bAIFrozen)
	{
		return;
	}

	// GDD §3.0: stats live in the data table, NEVER in code. Missing anything = log and idle.
	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASummonedUnit '%s': card table '%s' not found (imported from Docs/Data/cards.csv in TASK-008) — unit idles."),
			*GetNameSafe(this), *CardTableAsset.ToString());
		return;
	}

	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASummonedUnit '%s': CardID is not set (spawner calls InitUnit, TASK-007; BP_Unit_Footman sets Footman, TASK-010) — unit idles."),
			*GetNameSafe(this));
		return;
	}

	const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("ASummonedUnit::LoadStatsAndStart"), /*bWarnIfRowMissing=*/ false);
	if (!Row)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASummonedUnit '%s': row '%s' not found in '%s' — unit idles."),
			*GetNameSafe(this), *CardID.ToString(), *CardTable->GetName());
		return;
	}

	// bind the card stats (TASK-004 spec: HP → max/current, Speed → MaxWalkSpeed,
	// Damage/Range/Cadence → attack; TASK-028: bRanged → delivery). Values are
	// applied as authored (§3.0).
	MaxHP = Row->HP;
	CurrentHP = MaxHP;
	AttackDamage = Row->Damage;
	AttackRange = Row->Range;
	AttackCadence = FMath::Max(Row->Cadence, MinAttackCadence);
	bRangedAttack = Row->bRanged;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Row->Speed;
	}

	if (Row->HP <= 0.f || Row->Speed <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASummonedUnit '%s': row '%s' has HP %.1f / Speed %.1f — is this card really a unit? (check Docs/Data/cards.csv)"),
			*GetNameSafe(this), *CardID.ToString(), Row->HP, Row->Speed);
	}

	// M1 implements the Standard profile only; Siege/Support come with M2/M4 cards.
	// TODO(M2): dispatch on Row->Profile once more profiles exist.
	if (Row->Profile != ECardProfile::Standard)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASummonedUnit '%s': row '%s' has profile %d but only Standard is implemented in M1 — running Standard behavior."),
			*GetNameSafe(this), *CardID.ToString(), static_cast<int32>(Row->Profile));
	}

	bStatsLoaded = true;

	// state machine (GDD §3.8): first decision now, then every StateCheckInterval seconds
	UpdateState();
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ASummonedUnit::UpdateState, StateCheckInterval, /*bLoop=*/ true);
}

void ASummonedUnit::UpdateState()
{
	// bAIFrozen is defense-in-depth (TASK-028): FreezeAI clears this timer, but a
	// frozen unit must make no decisions even if something ever re-armed it.
	if (bDead || !bStatsLoaded || bAIFrozen)
	{
		return;
	}

	const FVector MyLocation = GetActorLocation();

	// Reacquire/leash (GDD §3.8): drop a dead/destroyed target, or one beyond LeashRange
	if (CurrentTarget && (!IsTargetAlive(CurrentTarget) || GetDistanceToTarget(MyLocation, CurrentTarget) > LeashRange))
	{
		CurrentTarget = nullptr;
	}

	// Acquire: nearest alive enemy within AggroRadius, re-evaluated every check so the
	// unit always fights the nearest threat (§3.8 "acquire nearest enemy within 600").
	// When nothing is inside aggro, a still-leashed CurrentTarget keeps being chased.
	if (AActor* Acquired = AcquireTarget())
	{
		CurrentTarget = Acquired;
	}

	// Advance goal: the acquired target, else the nearest standing enemy castle
	AActor* Goal = CurrentTarget;
	if (!Goal)
	{
		Goal = FindNearestEnemyCastle();
	}

	if (!Goal)
	{
		// no target and no standing enemy castle (destroyed → the match is over): stand down
		EnterIdle();
		return;
	}

	if (CurrentTarget && GetDistanceToTarget(MyLocation, CurrentTarget) <= AttackRange)
	{
		EnterAttack();
	}
	else
	{
		EnterAdvance(Goal);
	}
}

AActor* ASummonedUnit::AcquireTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	const FVector MyLocation = GetActorLocation();

	// bucket winners: preferred = units/hero (pawns); other = buildings/castle (non-pawns)
	AActor* BestPawn = nullptr;
	float BestPawnDist = TNumericLimits<float>::Max();
	AActor* BestOther = nullptr;
	float BestOtherDist = TNumericLimits<float>::Max();

	for (AActor* Candidate : TeamAgents)
	{
		if (Candidate == this || !IsTargetAlive(Candidate))
		{
			continue;
		}

		// no friendly targets (GDD §3.0). Native cast is valid: UTeamAgent is NotBlueprintable (TASK-001).
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent || Agent->GetTeamId() == Team)
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Candidate);
		if (Distance > AggroRadius)
		{
			continue;
		}

		if (Candidate->IsA<APawn>())
		{
			if (Distance < BestPawnDist)
			{
				BestPawn = Candidate;
				BestPawnDist = Distance;
			}
		}
		else if (Distance < BestOtherDist)
		{
			BestOther = Candidate;
			BestOtherDist = Distance;
		}
	}

	if (!BestOther)
	{
		return BestPawn;
	}
	if (!BestPawn)
	{
		return BestOther;
	}
	if (BestPawnDist <= BestOtherDist)
	{
		return BestPawn;
	}

	// nearest candidate is a building/castle: if the best unit/hero stands within
	// TieBreakDistance of it, prefer the unit/hero (GDD §3.8 Standard tie-break)
	const float PairDistance = GetDistanceToTarget(BestPawn->GetActorLocation(), BestOther);
	return (PairDistance <= TieBreakDistance) ? BestPawn : BestOther;
}

AActor* ASummonedUnit::FindNearestEnemyCastle() const
{
	const FVector MyLocation = GetActorLocation();

	ACastle* BestCastle = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	// actor iteration per spec — two castles in M1, trivially cheap on a 0.25 s timer
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		ACastle* Castle = *It;
		if (!IsValid(Castle) || Castle->GetTeamId() == Team || Castle->IsCastleDestroyed())
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Castle);
		if (Distance < BestDistance)
		{
			BestCastle = Castle;
			BestDistance = Distance;
		}
	}

	return BestCastle;
}

void ASummonedUnit::EnterAttack()
{
	if (State != ESummonedUnitState::Attack)
	{
		State = ESummonedUnitState::Attack;
		CurrentMoveGoal = nullptr;
		if (AAIController* AI = GetAIController())
		{
			AI->StopMovement();
		}
	}

	FaceTarget(CurrentTarget);

	if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
	{
		const UWorld* World = GetWorld();
		if (!World)
		{
			return;
		}

		// honor the cadence across target swaps / range flapping: if the cooldown from the
		// last landed hit has already elapsed, hit now; otherwise wait out the remainder.
		const double Now = World->GetTimeSeconds();
		float FirstDelay = static_cast<float>(static_cast<double>(AttackCadence) - (Now - LastAttackTime));
		if (FirstDelay <= 0.f)
		{
			PerformAttack();
			FirstDelay = AttackCadence;
		}
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ASummonedUnit::PerformAttack, AttackCadence, /*bLoop=*/ true, FirstDelay);
	}
}

void ASummonedUnit::EnterAdvance(AActor* Goal)
{
	if (State == ESummonedUnitState::Attack)
	{
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		StopAttackLunge(); // leaving Attack: mesh back to EXACTLY the rest pose (TASK-020)
	}
	State = ESummonedUnitState::Advance;

	AAIController* AI = GetAIController();
	if (!AI)
	{
		if (!bWarnedNoAIController)
		{
			bWarnedNoAIController = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASummonedUnit '%s': no AAIController possessing the unit — cannot move. Check AutoPossessAI / world settings."),
				*GetNameSafe(this));
		}
		return;
	}
	bWarnedNoAIController = false;

	// only (re)path when the goal changed or the last move finished/failed — MoveToActor
	// tethers the path to a moving goal actor, so a live chase needs no re-requests
	const bool bGoalChanged = (CurrentMoveGoal != Goal);
	if (bGoalChanged || AI->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		EPathFollowingRequestResult::Type Result;
		if (Goal->IsA<APawn>())
		{
			// chasing a unit/hero: stop once their capsule is comfortably inside attack range
			Result = AI->MoveToActor(Goal, FMath::Max(AttackRange * 0.8f, 40.f), /*bStopOnOverlap=*/ true);
		}
		else
		{
			// castle/building: bStopOnOverlap would test against the bounding cylinder of a
			// huge box and stop out of range — walk the partial path flush to its walls instead
			Result = AI->MoveToActor(Goal, StructureMoveAcceptanceRadius, /*bStopOnOverlap=*/ false,
				/*bUsePathfinding=*/ true, /*bCanStrafe=*/ true, /*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);
		}
		CurrentMoveGoal = Goal;

		if (Result == EPathFollowingRequestResult::Failed && bGoalChanged)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASummonedUnit '%s': MoveToActor toward '%s' failed — is the NavMeshBoundsVolume covering L_Arena (TASK-015)?"),
				*GetNameSafe(this), *GetNameSafe(Goal));
		}
	}
}

void ASummonedUnit::EnterIdle()
{
	if (State == ESummonedUnitState::Idle)
	{
		return;
	}
	State = ESummonedUnitState::Idle;
	CurrentMoveGoal = nullptr;

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	StopAttackLunge(); // leaving Attack (or defensive from Advance): exact rest pose (TASK-020)
	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}
}

void ASummonedUnit::PerformAttack()
{
	// bAIFrozen is defense-in-depth (TASK-028): FreezeAI clears the attack timer
	if (bDead || !bStatsLoaded || bAIFrozen)
	{
		return;
	}

	AActor* Target = CurrentTarget;
	if (!IsTargetAlive(Target))
	{
		return; // the next state check clears the target and resumes Advance
	}

	// range re-check also yields the contact point for the impact VFX (TASK-020):
	// the closest point on the target's collision to us, already fallen back to the
	// target's actor location when it has no usable collision
	FVector ImpactPoint = FVector::ZeroVector;
	if (GetDistanceToTarget(GetActorLocation(), Target, ImpactPoint) > AttackRange)
	{
		return; // drifted out of range between checks — no hit, the state check re-chases
	}

	FaceTarget(Target);

	if (bRangedAttack)
	{
		// ranged delivery (TASK-028): the cadence hit launches a homing projectile
		// instead of applying melee damage — the damage lands when the projectile
		// impacts (ACastle halves projectile-typed damage on ITS side, §3.0). NO
		// lunge and NO melee puff for ranged attacks: the projectile and its own
		// impact VFX are the telegraph (GDD §3.8 / TASK-028 spec).
		FireProjectileAt(Target);
	}
	else
	{
		// melee delivery — the M1/TASK-020 path, unchanged (TASK-028 headline rule).
		// team attribution (TASK-002 castle contract): this unit as DamageCauser AND its
		// controller as EventInstigator, so receivers resolve our team either way (GDD §3.0).
		// TASK-020 only CAPTURES the return value — arguments and timing are unchanged.
		const float DamageApplied = UGameplayStatics::ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass());

		// attack feedback (TASK-020): the swing (lunge) plays on every executed cadence hit;
		// the impact puff only when damage actually landed — a receiver that zeroed the hit
		// (e.g. a castle destroyed this same tick) gets no puff. Same return-value reading
		// as the hero's TASK-016 flagged decision 1.
		StartAttackLunge();
		if (DamageApplied > 0.f && CachedAttackImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CachedAttackImpactEffect, ImpactPoint);
		}
	}

	// stamped for ranged shots exactly like melee hits — the cadence gate in
	// EnterAttack stays honest across target swaps for both delivery modes
	if (const UWorld* World = GetWorld())
	{
		LastAttackTime = World->GetTimeSeconds();
	}
}

void ASummonedUnit::FaceTarget(const AActor* Target)
{
	if (!Target)
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.Rotation());
	}
}

void ASummonedUnit::FireProjectileAt(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return;
	}

	// pawn-shooter rule (TASK-026 contract): Instigator = this, so receivers'
	// no-friendly-fire checks resolve our team through the TASK-002 chain
	// (instigating controller's pawn / damage causer's instigator pawn).
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	// the projectile carries no collision (TASK-026) — never let spawn adjustment
	// nudge it away from the capsule it deliberately spawns inside of
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// spawn at the unit (spec), aimed at the target — the aim is cosmetic, the
	// projectile re-aims at the target's CURRENT location every tick (TASK-026).
	// Scale 1: the projectile's visual carries its own fixed child scale.
	const FVector MuzzleLocation = GetActorLocation();
	const FVector ToTarget = Target->GetActorLocation() - MuzzleLocation;
	const FRotator FireRotation = ToTarget.IsNearlyZero() ? GetActorRotation() : ToTarget.Rotation();

	if (AProjectile* Projectile = World->SpawnActor<AProjectile>(AProjectile::StaticClass(), FTransform(FireRotation, MuzzleLocation), SpawnParameters))
	{
		// own team, current target, row Damage, projectile-typed — ACastle applies
		// the §3.0 50% on ITS side (TASK-026); units/hero take the listed damage.
		Projectile->InitProjectile(Team, Target, AttackDamage, USiegeDamageType_Projectile::StaticClass());
	}
}

void ASummonedUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// tick exists ONLY for the lunge visual (TASK-020) — see the constructor note
	UpdateLunge(DeltaSeconds);
}

void ASummonedUnit::StartAttackLunge()
{
	if (!VisualMesh || !bVisualMeshBaseCached)
	{
		return; // no rest pose to return to — never move the mesh without one
	}

	// one cycle per cadence hit, never longer than 0.8 × Cadence, so the mesh is
	// guaranteed back at rest before the next hit (hits are >= Cadence apart via
	// the LastAttackTime gate in EnterAttack). AttackCadence >= MinAttackCadence,
	// so the clamp can never produce a zero-length cycle on its own.
	const float CycleDuration = FMath::Min(AttackLungeDuration, 0.8f * AttackCadence);
	if (CycleDuration <= UE_KINDA_SMALL_NUMBER || FMath::IsNearlyZero(AttackLungeDistance))
	{
		return; // degenerate cycle — designer disabled the lunge
	}

	// (re)start from the exact rest pose: even a defensive mid-cycle restart can
	// never accumulate drift, because the pose is only ever written as Base + f
	VisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation);
	LungeCycleDuration = CycleDuration;
	LungeElapsed = 0.f;
	bLungeActive = true;
	SetActorTickEnabled(true);
}

void ASummonedUnit::StopAttackLunge()
{
	// restore EXACTLY the cached BP-authored pose (TASK-020 zero-drift contract);
	// idempotent — restoring an already-resting mesh writes the same value
	if (bVisualMeshBaseCached && VisualMesh)
	{
		VisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation);
	}
	bLungeActive = false;
	LungeElapsed = 0.f;
	SetActorTickEnabled(false);
}

void ASummonedUnit::UpdateLunge(float DeltaSeconds)
{
	if (!bLungeActive)
	{
		SetActorTickEnabled(false); // stray tick with no cycle running — go back to sleep
		return;
	}

	LungeElapsed += DeltaSeconds;
	if (!VisualMesh || LungeElapsed >= LungeCycleDuration)
	{
		StopAttackLunge(); // cycle end: exact rest pose, tick off
		return;
	}

	// sine ease, out and back: 0 → AttackLungeDistance at the half cycle → 0.
	// The offset is applied to the RELATIVE location, which lives in the parent
	// CAPSULE's axes — local +X is actor forward (the unit faces its target via
	// FaceTarget), so this is correct for either team/facing and is untouched by
	// the mesh's own -90° import-fix yaw (handoffs/TASK-014.md).
	const float Alpha = LungeElapsed / LungeCycleDuration;
	const float Offset = AttackLungeDistance * FMath::Sin(UE_PI * Alpha);
	VisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation + FVector(Offset, 0.f, 0.f));
}

AAIController* ASummonedUnit::GetAIController() const
{
	return Cast<AAIController>(GetController());
}

float ASummonedUnit::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	// no friendly fire (GDD §3.0): same-team damage is ignored entirely. Super is skipped
	// so damage delegates never observe friendly hits — the pattern QA approved on ACastle.
	ETeamId AttackerTeam = ETeamId::Blue;
	if (TryGetDamageTeam(EventInstigator, DamageCauser, AttackerTeam) && AttackerTeam == Team)
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.f)
	{
		return 0.f;
	}

	CurrentHP = FMath::Max(CurrentHP - ActualDamage, 0.f);

	if (CurrentHP <= 0.f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

bool ASummonedUnit::TryGetDamageTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam)
{
	// mirror of ACastle::TryGetInstigatorTeam (TASK-002) — keep the chains in sync

	// 1) the instigating controller's pawn (hero melee and unit attacks report their controller)
	if (EventInstigator)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// 2) the damage causer itself (hero and units pass themselves)
	if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser))
	{
		OutTeam = Agent->GetTeamId();
		return true;
	}

	// 3) the causer's instigator pawn (covers projectiles once M2 adds them)
	if (DamageCauser)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser->GetInstigator()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// no team resolvable (e.g. world damage) — caller applies the damage
	return false;
}

void ASummonedUnit::HandleDeath()
{
	// death side effects run exactly once
	if (bDead)
	{
		return;
	}
	bDead = true;
	CurrentHP = 0.f;

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

	// death restores the exact rest pose before the actor goes away (TASK-020 contract)
	StopAttackLunge();

	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}

	// units don't respawn (GDD §3.8 / TASK-004 spec): remove the actor. The default
	// AAIController carries no PlayerState, so AController::PawnPendingDestroy destroys
	// it alongside us — no controller leak across repeated summons.
	Destroy();
}

bool ASummonedUnit::IsTargetAlive(const AActor* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}

	// destroyed castles must be skipped explicitly (TASK-002 handoff: distance-based
	// acquisition cannot rely on their disabled collision alone)
	if (const ACastle* Castle = Cast<ACastle>(Target))
	{
		return !Castle->IsCastleDestroyed();
	}

	// a dead hero is hidden, not destroyed (TASK-003) — do not attack the corpse
	if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Target))
	{
		return !Hero->IsDead();
	}

	// dying units destroy themselves, but the flag closes the same-frame window
	if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Target))
	{
		return !Unit->IsUnitDead();
	}

	// unknown ITeamAgent types (M2 towers/walls) have no death API yet — treat as alive
	return true;
}

float ASummonedUnit::GetDistanceToTarget(const FVector& From, const AActor* Target)
{
	FVector ClosestPointUnused = FVector::ZeroVector;
	return GetDistanceToTarget(From, Target, ClosestPointUnused);
}

float ASummonedUnit::GetDistanceToTarget(const FVector& From, const AActor* Target, FVector& OutClosestPoint)
{
	OutClosestPoint = FVector::ZeroVector;

	if (!Target)
	{
		return TNumericLimits<float>::Max();
	}

	// closest point on the target's collision, mirroring the hero melee (TASK-003):
	// the castle's origin sits at the center of an ~800x800 footprint and would never
	// come within Range/AggroRadius of a unit standing at its walls. ECC_Pawn is blocked
	// by pawn capsules and by the castle's BlockAll mesh (TASK-002). The closest point
	// doubles as the impact-VFX contact point (TASK-020).
	const float Distance = Target->ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint);
	if (Distance < 0.f)
	{
		// no usable collision (e.g. SM_Castle not imported yet, TASK-013) — actor origin
		// fallback for both the distance and the contact point (TASK-020 spec fallback)
		OutClosestPoint = Target->GetActorLocation();
		return static_cast<float>(FVector::Dist(From, OutClosestPoint));
	}
	return Distance;
}
