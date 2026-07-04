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
#include "Materials/MaterialInterface.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeCombatStatics.h"
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

	// TASK-044 (CONVENTIONS Team contract): recolor VisualMesh to the ACTUAL Team.
	// The deferred spawn sets Team before BeginPlay (InitUnit → FinishSpawning,
	// TASK-030/046), so the correct team material lands here; AMinerUnit inherits
	// this via Super::BeginPlay. Purely cosmetic — the -90° yaw rest pose, the
	// TASK-020 lunge, and the melee/ranged attack paths are untouched (slot 0 only).
	ApplyTeamMaterial();

	LoadStatsAndStart();
}

void ASummonedUnit::ApplyTeamMaterial()
{
	// TASK-044 — CONVENTIONS Team contract: the bot reuses the player's BP_Unit_*
	// assets (authored with the Blue placeholder material); this overrides slot 0 by
	// the ACTUAL Team so a Red-spawned unit reads red with no Red BP duplicate. Blue
	// re-applies the identical MI_TeamColor_Blue, so Blue-side visuals are unchanged.
	if (!VisualMesh)
	{
		return;
	}

	// cached static resolve (spec): the two MI instances resolve ONCE per process and
	// are shared by every unit/miner — never a per-attack/per-frame load. The paths are
	// the CONVENTIONS Team contract. LoadSynchronous re-resolves through the soft path
	// if GC ever unloaded them and returns nullptr for a missing asset — in which case
	// the slot is left as authored (null-safe, never a crash — the AProjectile::
	// ApplyTeamVisuals pattern, mirrored; keep the MI paths in sync by hand).
	static const TSoftObjectPtr<UMaterialInterface> BlueTeamMaterial(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	static const TSoftObjectPtr<UMaterialInterface> RedTeamMaterial(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));

	const TSoftObjectPtr<UMaterialInterface>& TeamMat = (Team == ETeamId::Red) ? RedTeamMaterial : BlueTeamMaterial;
	if (UMaterialInterface* ResolvedTeamMat = TeamMat.LoadSynchronous())
	{
		VisualMesh->SetMaterial(0, ResolvedTeamMat);
	}
}

void ASummonedUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(MoveSpeedBuffTimerHandle); // TASK-042: no dangling buff-restore on a destroyed unit
	GetWorldTimerManager().ClearTimer(HealTimerHandle); // TASK-054: no dangling Support heal on a destroyed unit
	GetWorldTimerManager().ClearTimer(AuraDamageBuffTimerHandle); // TASK-055: no dangling aura-restore on a destroyed unit

	Super::EndPlay(EndPlayReason);
}

void ASummonedUnit::InitUnit(ETeamId InTeam, FName InCardID)
{
	Team = InTeam;

	// TASK-044: keep VisualMesh's team color matched to a late/updated Team. Deferred
	// spawns (InitUnit before FinishSpawning — the TASK-030/046 path) run this
	// pre-BeginPlay (HasActorBegunPlay() false) and BeginPlay does the single apply; a
	// plain SpawnActor + InitUnit (or a post-bind Team update) re-applies for the now-
	// current Team. Idempotent and null-safe; runs even on the stats-already-bound path.
	if (HasActorBegunPlay())
	{
		ApplyTeamMaterial();
	}

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

	// end any active move-speed buff (TASK-042): clear its timer and restore the
	// base speed EXACTLY, so a match-end freeze leaves zero residual walk speed
	EndMoveSpeedBuff();

	// end any active War Banner damage aura (TASK-055): clear its timer and restore the
	// damage-output multiplier to EXACTLY 1.0, so a frozen unit deals only its base row
	// Damage (zero residual buff). Same drift-free discipline as the move-speed buff.
	EndAuraDamageBuff();

	// stop Support healing (TASK-054): clear the heal timer and drop the heal
	// target, so a match-end-frozen Cleric mends no one. (Siege pathing decisions
	// and Support follow decisions already stopped with the StateTimerHandle clear
	// above — UpdateState makes no more calls — and the StopMovement below aborts
	// the in-flight Siege advance / Support follow path.)
	StopHealing();
	SupportHealTarget = nullptr;

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

void ASummonedUnit::ApplyMoveSpeedBuff(float Multiplier, float Duration)
{
	// a dying unit is being destroyed, and a match-end-frozen unit stays parked
	// (TASK-028) — neither should take a Rally buff. Rally never targets these
	// (the hero skips dead units), but guard defensively so the API is safe anywhere.
	if (bDead || bAIFrozen)
	{
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	// Capture the resting base speed ONCE per buff episode. A refresh while the buff
	// is already active must NOT recapture the already-buffed speed — that is exactly
	// the drift the TASK-020 lunge lesson warns against. The base is only ever taken
	// from the resting (unbuffed) MaxWalkSpeed and restored EXACTLY on end.
	if (!bMoveSpeedBuffActive)
	{
		MoveSpeedBuffBaseSpeed = Movement->MaxWalkSpeed;
		bMoveSpeedBuffActive = true;
	}

	// No stacking: the buffed speed is always Base × Multiplier from the stored base,
	// so re-applying only REFRESHES (never compounds) the same-magnitude boost.
	Movement->MaxWalkSpeed = MoveSpeedBuffBaseSpeed * Multiplier;

	// (Re)arm the restore timer with a fresh Duration — this is the refresh (never a
	// stack, since a single one-shot handle is reused). A non-positive Duration is a
	// defensive immediate restore (Rally always passes RallyDuration = 5 s > 0).
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(MoveSpeedBuffTimerHandle, this, &ASummonedUnit::EndMoveSpeedBuff, Duration, /*bLoop=*/ false);
	}
	else
	{
		EndMoveSpeedBuff();
	}
}

void ASummonedUnit::EndMoveSpeedBuff()
{
	// idempotent: clear the timer either way, restore only when a buff is live so the
	// base speed is written back EXACTLY once (zero residual drift, TASK-020 contract)
	GetWorldTimerManager().ClearTimer(MoveSpeedBuffTimerHandle);

	if (!bMoveSpeedBuffActive)
	{
		return;
	}
	bMoveSpeedBuffActive = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MoveSpeedBuffBaseSpeed;
	}
}

void ASummonedUnit::SetAuraDamageBonus(float Bonus, float Duration)
{
	// a dying unit is being destroyed, and a match-end-frozen unit stays parked (TASK-028) —
	// neither takes an aura. War Banner never targets these, but guard so the TASK-058 hook is
	// safe to call from anywhere (mirrors ApplyMoveSpeedBuff's dead/frozen guard).
	if (bDead || bAIFrozen)
	{
		return;
	}

	// a non-positive bonus is a clear (no negative "buff"); apply-then-immediately-restore for a
	// non-positive duration is meaningless — treat both as "end any active aura now".
	if (Bonus <= 0.f)
	{
		EndAuraDamageBuff();
		return;
	}

	// Refresh-not-stack: the multiplier is written DIRECTLY from the bonus (never compounded off
	// the already-buffed value), so re-applying only refreshes the same-magnitude bonus. Because
	// the multiplier is stored separately from AttackDamage and reset to the literal 1.0 on expiry,
	// there is no base to drift — a stronger guarantee than the move-speed buff's cache-once.
	AuraDamageMultiplier = 1.f + Bonus;
	bAuraDamageBuffActive = true;

	// (re)arm the single one-shot restore timer with a fresh Duration — this is the refresh, never
	// a stack (one handle is reused). War Banner always passes a positive Duration.
	if (Duration > 0.f)
	{
		GetWorldTimerManager().SetTimer(AuraDamageBuffTimerHandle, this, &ASummonedUnit::EndAuraDamageBuff, Duration, /*bLoop=*/ false);
	}
	else
	{
		EndAuraDamageBuff();
	}
}

void ASummonedUnit::EndAuraDamageBuff()
{
	// idempotent: clear the timer either way; reset the multiplier to EXACTLY 1.0 only when an
	// aura is live, so the base row Damage composes un-multiplied again (zero drift — the
	// multiplier is never read to compute a new one, so restore is always exact).
	GetWorldTimerManager().ClearTimer(AuraDamageBuffTimerHandle);

	if (!bAuraDamageBuffActive)
	{
		return;
	}
	bAuraDamageBuffActive = false;
	AuraDamageMultiplier = 1.f;
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
	// TASK-054: bind the targeting profile. UpdateState dispatches Siege/Support;
	// Standard (and None — e.g. miners, whose combat machine AMinerUnit seals)
	// runs the M1/M2 Standard body. Bound BEFORE the synchronous UpdateState()
	// below so the very first decision already routes on the correct profile.
	Profile = Row->Profile;
	// TASK-055: bind the Standard-keyword flags + AoE radius from the row (never hardcoded — GDD §3.0).
	// Sparse columns: defaults (false / 0) leave every core/M1/M2 card byte-unchanged.
	bCharge = Row->bCharge;
	bSlayer = Row->bSlayer;
	bSuicide = Row->bSuicide;
	AoERadius = Row->AoERadius;
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

	// (TASK-054: Standard, Siege, and Support are all implemented now — the old
	// "only Standard implemented" warning is gone. Profile bound above; None runs
	// the Standard body, which is inert for the Miner subclass by construction.)

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

	// CHARGE bookkeeping (TASK-055): accumulate uninterrupted-movement time so the first attack
	// after >= ChargeMoveSeconds gets the ×ChargeMultiplier bonus. No-op for non-charge units
	// (guarded by bCharge), so every non-Cavalry unit — and the Miner subclass — is byte-unchanged.
	// Runs before the profile dispatch so it tracks regardless of profile.
	TrackChargeMovement();

	// Profile dispatch (TASK-054): Siege and Support run their own targeting;
	// Standard — and None (miners, whose combat machine AMinerUnit otherwise
	// seals) — fall through to the M1/M2 Standard body below, byte-for-byte
	// unchanged. The Miner's one synchronous UpdateState still runs this body
	// (Profile None), acquisition-dead via AggroRadius 0 (its class contract).
	if (Profile == ECardProfile::Siege)
	{
		UpdateStateSiege();
		return;
	}
	if (Profile == ECardProfile::Support)
	{
		UpdateStateSupport();
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

void ASummonedUnit::UpdateStateSiege()
{
	// Siege profile (GDD §3.8: Ogre, Sapper) — IGNORE units and the hero entirely;
	// batter structures. Target the nearest enemy ABuilding (walls, towers, and any
	// ABuilding subclass), else the enemy castle. Re-evaluated every check, so a
	// fallen wall or a freshly-placed closer one re-routes the unit, and a structure
	// destroyed mid-attack drops through to the castle on the next check.
	const FVector MyLocation = GetActorLocation();

	AActor* Structure = FindNearestEnemyBuilding();
	if (!Structure)
	{
		Structure = FindNearestEnemyCastle();
	}
	CurrentTarget = Structure;

	if (!CurrentTarget)
	{
		// no standing enemy structure at all (buildings down + castle destroyed → match over)
		EnterIdle();
		return;
	}

	if (GetDistanceToTarget(MyLocation, CurrentTarget) <= AttackRange)
	{
		// SUICIDE (TASK-055, Sapper): reaching attack range triggers a SINGLE AoE detonation
		// (row Damage over AoERadius, Siege-typed) instead of the normal Siege melee — then the
		// unit dies. No EnterAttack, so the attack timer never arms and there are no repeat hits.
		// (bSuicide is a Siege-unit keyword per the data — Sapper is Profile=Siege — so this is
		// the correct home for the trigger; the Standard body stays untouched.)
		if (bSuicide)
		{
			Detonate();
			return;
		}
		EnterAttack();
	}
	else
	{
		EnterAdvance(CurrentTarget);
	}
}

void ASummonedUnit::UpdateStateSupport()
{
	// Support profile (GDD §3.8: Cleric) — NEVER attacks. Heal the nearest DAMAGED
	// friendly within Range continuously, and follow the nearest friendly (the
	// damaged one if any, else the nearest friendly combat unit). Following and
	// healing are decoupled: movement keeps the Cleric near the line while the heal
	// timer mends whoever is hurt and in range.
	ASummonedUnit* HealTarget = FindNearestDamagedFriendly();
	SupportHealTarget = HealTarget;

	if (HealTarget)
	{
		StartHealing();         // arm/keep the heal timer — PerformHeal does the work
		FaceTarget(HealTarget); // cosmetic: look at who we are mending
	}
	else
	{
		StopHealing();          // nobody hurt in range — stop the heal cadence
	}

	// Follow goal: the damaged friendly if one exists, else the nearest friendly
	// combat unit to escort. A lone Cleric (no friendly at all) stands down.
	AActor* FollowGoal = HealTarget;
	if (!FollowGoal)
	{
		FollowGoal = FindNearestFriendlyCombatUnit();
	}

	if (!FollowGoal)
	{
		EnterIdle();
		return;
	}

	// NEVER Attack: EnterAdvance toward a pawn stops ~0.8×Range short (comfortably
	// inside the 400 heal ring), so the Cleric trails its escort without colliding.
	// State stays Advance/Idle for Support — EnterAttack is never called here.
	EnterAdvance(FollowGoal);
}

AActor* ASummonedUnit::FindNearestEnemyBuilding() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	ABuilding* BestBuilding = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	// actor iteration (the FindNearestEnemyCastle pattern) — covers ABuilding and
	// every subclass (ATower, and the M4 spawner/economy buildings) at runtime.
	for (TActorIterator<ABuilding> It(World); It; ++It)
	{
		ABuilding* Building = *It;
		if (!IsValid(Building) || Building->GetTeamId() == Team || Building->IsBuildingDestroyed())
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Building);
		if (Distance < BestDistance)
		{
			BestBuilding = Building;
			BestDistance = Distance;
		}
	}

	return BestBuilding;
}

ASummonedUnit* ASummonedUnit::FindNearestDamagedFriendly() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	ASummonedUnit* BestUnit = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* Unit = *It;
		if (Unit == this || !IsValid(Unit) || Unit->IsUnitDead())
		{
			continue;
		}

		// friendly only — a Cleric never heals enemies (the no-friendly-fire mirror)
		if (Unit->GetTeamId() != Team)
		{
			continue;
		}

		// must be DAMAGED, and carry a real HP pool (an unbound 0/0 unit is skipped)
		if (Unit->GetMaxHP() <= 0.f || Unit->GetCurrentHP() >= Unit->GetMaxHP())
		{
			continue;
		}

		// within heal range (row Range — Cleric 400); closest-point, like combat reach
		const float Distance = GetDistanceToTarget(MyLocation, Unit);
		if (Distance > AttackRange)
		{
			continue;
		}

		if (Distance < BestDistance)
		{
			BestUnit = Unit;
			BestDistance = Distance;
		}
	}

	return BestUnit;
}

ASummonedUnit* ASummonedUnit::FindNearestFriendlyCombatUnit() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	ASummonedUnit* BestUnit = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* Unit = *It;
		if (Unit == this || !IsValid(Unit) || Unit->IsUnitDead())
		{
			continue;
		}

		if (Unit->GetTeamId() != Team)
		{
			continue;
		}

		// a COMBAT unit actually fights (Standard or Siege) — never another Support
		// (Cleric) or a non-combat Miner (Profile None), so Clerics do not trail each
		// other or a miner when nobody is hurt. Same-class read of the private Profile.
		if (Unit->Profile != ECardProfile::Standard && Unit->Profile != ECardProfile::Siege)
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Unit);
		if (Distance < BestDistance)
		{
			BestUnit = Unit;
			BestDistance = Distance;
		}
	}

	return BestUnit;
}

void ASummonedUnit::StartHealing()
{
	// idempotent (the EnterAttack cadence-timer pattern): keep ONE looping heal
	// timer running while a damaged friendly stays in range. Rate is clamped
	// strictly positive at arm time (SetTimer with <= 0 would CLEAR, not schedule —
	// the qa/TASK-021 WARN-1 guard rule for looping timers).
	if (!GetWorldTimerManager().IsTimerActive(HealTimerHandle))
	{
		GetWorldTimerManager().SetTimer(HealTimerHandle, this, &ASummonedUnit::PerformHeal,
			FMath::Max(SupportHealInterval, 0.05f), /*bLoop=*/ true);
	}
}

void ASummonedUnit::StopHealing()
{
	GetWorldTimerManager().ClearTimer(HealTimerHandle);
}

void ASummonedUnit::PerformHeal()
{
	// defense-in-depth (the PerformAttack gate): FreezeAI/HandleDeath clear this timer
	if (bDead || !bStatsLoaded || bAIFrozen)
	{
		return;
	}

	ASummonedUnit* HealTarget = SupportHealTarget.Get();

	// re-validate every tick — the target may have moved out of range, topped off,
	// or died since the last (slower) state check that acquired it.
	if (!IsValid(HealTarget) || HealTarget == this
		|| HealTarget->IsUnitDead()
		|| HealTarget->GetTeamId() != Team
		|| HealTarget->GetMaxHP() <= 0.f
		|| HealTarget->GetCurrentHP() >= HealTarget->GetMaxHP()
		|| GetDistanceToTarget(GetActorLocation(), HealTarget) > AttackRange)
	{
		return; // the next state check re-acquires or calls StopHealing
	}

	// continuous heal (GDD §3.8): row Damage HP/sec, delivered per tick as
	// rate × SupportHealInterval; ApplyHealing clamps to MaxHP (no overheal).
	HealTarget->ApplyHealing(AttackDamage * SupportHealInterval);
}

void ASummonedUnit::ApplyHealing(float Amount)
{
	// no reviving the dead, no healing a match-end-frozen or never-bound unit, no
	// negative "heals". Units carry no HP-changed delegate (only ACastle does), so
	// there is nothing to broadcast — just clamp to MaxHP (no overheal).
	if (bDead || bAIFrozen || !bStatsLoaded || Amount <= 0.f)
	{
		return;
	}

	CurrentHP = FMath::Min(CurrentHP + Amount, MaxHP);
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

	// CENTRALIZED damage OUTPUT (TASK-055): dealt = row Damage × Charge × Slayer × Aura, composed
	// once here so every modifier stacks in ONE place for both delivery modes. Siege 200% is NOT an
	// output multiplier — it is applied fortification-side by the damage TYPE in ACastle/ABuilding
	// TakeDamage (TASK-054), so it is never double-counted here. For a non-keyword unit with no aura
	// every factor is exactly 1.0, so OutputDamage == AttackDamage bit-for-bit and the M1/M2 melee +
	// ranged paths stay byte-identical.
	const float OutputDamage = ComputeOutputDamage(Target);

	if (bRangedAttack)
	{
		// ranged delivery (TASK-028): the cadence hit launches a homing projectile
		// instead of applying melee damage — the damage lands when the projectile
		// impacts (ACastle halves projectile-typed damage on ITS side, §3.0). NO
		// lunge and NO melee puff for ranged attacks: the projectile and its own
		// impact VFX are the telegraph (GDD §3.8 / TASK-028 spec). The projectile
		// carries the composed OutputDamage (aura buffs a Longbowman's shot too).
		FireProjectileAt(Target, OutputDamage);
	}
	else
	{
		// melee delivery — the M1/TASK-020 path. Standard/Support pass base
		// UDamageType (byte-identical to M1 — 100% vs castle); Siege units (Ogre,
		// Sapper) tag their melee with USiegeDamageType_Siege so ACastle/ABuilding
		// scale it to 200% (TASK-054). Units and the hero take the listed amount.
		// team attribution (TASK-002 castle contract): this unit as DamageCauser AND its
		// controller as EventInstigator, so receivers resolve our team either way (GDD §3.0).
		// TASK-020 only CAPTURES the return value — timing and the lunge/puff below are unchanged.
		const TSubclassOf<UDamageType> MeleeDamageType = (Profile == ECardProfile::Siege)
			? TSubclassOf<UDamageType>(USiegeDamageType_Siege::StaticClass())
			: TSubclassOf<UDamageType>(UDamageType::StaticClass());
		const float DamageApplied = UGameplayStatics::ApplyDamage(Target, OutputDamage, GetController(), this, MeleeDamageType);

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

void ASummonedUnit::TrackChargeMovement()
{
	if (!bCharge)
	{
		return; // only Charge units (Cavalry) track — everyone else, incl. the Miner, is byte-unchanged
	}

	// "Uninterrupted movement" (GDD §3.0): accumulate real advancing time. A stall (blocked
	// against a wall — velocity ~0 while advancing) or a full stop (Idle) breaks the run and
	// drops any earned-but-unspent charge; momentum must be rebuilt from zero. We deliberately do
	// NOT reset on entering Attack: tracking and the state dispatch share this one UpdateState, so
	// resetting here would race the very hit that should be charged. The charge is instead consumed
	// exactly once in ComputeOutputDamage (the first PerformAttack of the engagement).
	if (State == ESummonedUnitState::Advance)
	{
		if (GetVelocity().SizeSquared() > FMath::Square(ChargeMoveSpeedThreshold))
		{
			ChargeMoveElapsed += StateCheckInterval;
			if (ChargeMoveElapsed >= ChargeMoveSeconds)
			{
				bChargePrimed = true;
			}
		}
		else
		{
			// blocked / stalled mid-advance: lose momentum
			ChargeMoveElapsed = 0.f;
			bChargePrimed = false;
		}
	}
	else if (State == ESummonedUnitState::Idle)
	{
		// stood down entirely: lose momentum
		ChargeMoveElapsed = 0.f;
		bChargePrimed = false;
	}
	// State == Attack: leave the primed flag for ComputeOutputDamage to consume.
}

float ASummonedUnit::ComputeOutputDamage(const AActor* Target)
{
	// Base is the row Damage bound at LoadStatsAndStart (never hardcoded, GDD §3.0). Output composes
	// the Standard keyword multipliers in ONE place: Charge (spent here), Slayer (target-HP gated),
	// and the War Banner aura. Siege 200% is NOT composed here — it is applied fortification-side by
	// the damage TYPE (TASK-054), so composing it here would double-count. For a non-keyword,
	// un-auraed unit every factor is exactly 1.0, so this returns AttackDamage bit-for-bit.
	float Output = AttackDamage;

	// CHARGE (Cavalry): the FIRST attack after >= ChargeMoveSeconds of continuous movement deals
	// ×ChargeMultiplier; consumed here so the next hit reverts to base until momentum is rebuilt.
	if (bCharge && bChargePrimed)
	{
		Output *= ChargeMultiplier;
		bChargePrimed = false;
		ChargeMoveElapsed = 0.f;
	}

	// SLAYER (Pikeman): ×SlayerMultiplier vs any target whose MaxHP >= SlayerHPThreshold (150).
	// The bSlayer short-circuit keeps GetTargetMaxHP off the hot path for every non-Slayer unit.
	if (bSlayer && GetTargetMaxHP(Target) >= SlayerHPThreshold)
	{
		Output *= SlayerMultiplier;
	}

	// WAR BANNER AURA: temporary additive output multiplier (1.0 = none). Stored separately from
	// AttackDamage and reset to exactly 1.0 on expiry, so it can never drift the row-bound base.
	Output *= AuraDamageMultiplier;

	return Output;
}

float ASummonedUnit::GetTargetMaxHP(const AActor* Target)
{
	// Slayer HP gate: the max HP of the known combat receiver types. Unknown types return 0 (no
	// Slayer bonus) — Slayer only ever fires against a resolvable MaxHP >= threshold.
	if (!Target)
	{
		return 0.f;
	}
	if (const ASummonedUnit* Unit = Cast<ASummonedUnit>(Target))
	{
		return Unit->GetMaxHP();
	}
	if (const ACastle* Castle = Cast<ACastle>(Target))
	{
		return Castle->GetMaxHP();
	}
	if (const ABuilding* Building = Cast<ABuilding>(Target))
	{
		return Building->GetMaxHP();
	}
	if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(Target))
	{
		return Hero->GetMaxHP();
	}
	return 0.f;
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

void ASummonedUnit::FireProjectileAt(AActor* Target, float DamageAmount)
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
		// own team, current target, the CENTRALIZED output damage (TASK-055: row Damage × aura,
		// etc. — for a plain Archer this is exactly the row Damage), projectile-typed — ACastle
		// applies the §3.0 50% on ITS side (TASK-026); units/hero take the listed damage.
		Projectile->InitProjectile(Team, Target, DamageAmount, USiegeDamageType_Projectile::StaticClass());
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

void ASummonedUnit::Detonate()
{
	// single detonation on reaching a structure (UpdateStateSiege): blast then die. The bDead
	// guard prevents re-entry from an already-dying unit; ApplyDetonation's bDetonated guard
	// prevents a double-blast with the death-triggered path in HandleDeath.
	if (bDead)
	{
		return;
	}
	ApplyDetonation();
	HandleDeath(); // clears timers, stops movement, restores the rest pose, destroys — no repeat attacks
}

void ASummonedUnit::ApplyDetonation()
{
	if (bDetonated)
	{
		return; // exactly ONE blast whether triggered by contact (Detonate) or by death (HandleDeath)
	}
	bDetonated = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// AoE at our feet: row Damage (Sapper 80) over row AoERadius (250), Siege-typed so ACastle/
	// ABuilding scale it to 200% (TASK-054); enemies only, no friendly fire. Nothing hardcoded —
	// AttackDamage and AoERadius are bound from the DT_Cards row. Our controller is the instigator
	// for attribution; the shared helper's Team filter is the friendly-fire authority (TASK-056
	// reuses this same call for the Bomb Tower).
	FSiegeCombatStatics::ApplyRadialDamage(World, GetController(), Team, GetActorLocation(),
		AoERadius, AttackDamage, USiegeDamageType_Siege::StaticClass());
}

void ASummonedUnit::HandleDeath()
{
	// death side effects run exactly once
	if (bDead)
	{
		return;
	}

	// SUICIDE (TASK-055, Sapper): a Sapper killed BEFORE it reaches a structure (shot down en
	// route) still explodes on death — a single blast (bDetonated guards against doubling with a
	// contact-triggered Detonate). Runs BEFORE the actor is torn down so the AoE reads a valid
	// location + controller. No-op for every non-Sapper unit (bSuicide false).
	if (bSuicide)
	{
		ApplyDetonation();
	}

	bDead = true;
	CurrentHP = 0.f;

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	StopHealing(); // TASK-054: a dying Cleric heals no one

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
