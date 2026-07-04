// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HeroCharacter.h"

#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GitClaudeUnrealTest.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

namespace
{
	/** Priority of IMC_Hero on the input subsystem — above any template context (which the template controllers add at 0). */
	constexpr int32 HeroMappingContextPriority = 1;
}

AHeroCharacter::AHeroCharacter()
{
	// needed for out-of-combat HP regen
	PrimaryActorTick.bCanEverTick = true;

	// GDD §3.1 base walk speed (BeginPlay re-applies in case a blueprint tweaks WalkSpeed)
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// far in the past: the first swing is never cooldown-blocked and a below-max hero regens immediately
	LastMeleeTime = -1.0e9;
	LastCombatTime = -1.0e9;
}

void AHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// out-of-combat regen (GDD §3.1): 5 HP/s starting 8 s after last taking OR dealing damage, stops at max
	if (!bDead && CurrentHP < MaxHP)
	{
		const UWorld* World = GetWorld();
		if (World && (World->GetTimeSeconds() - LastCombatTime) >= RegenDelay)
		{
			CurrentHP = FMath::Min(CurrentHP + (RegenRate * DeltaSeconds), MaxHP);
		}
	}
}

void AHeroCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// add the hero mapping context ourselves: ASiegePlayerController (TASK-007) does not add
	// contexts the way the template controllers do. Null-safe at every step — the raw C++
	// class (game mode fallback pawn) has no context assigned and must still run.
	if (HeroMappingContext)
	{
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
				{
					Subsystem->AddMappingContext(HeroMappingContext, HeroMappingContextPriority);
				}
			}
		}
	}
}

void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// template binds Jump/Move/Look (actions assigned on the blueprint in TASK-009)
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Sprinting — hold to sprint (GDD §3.1)
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AHeroCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHeroCharacter::StopSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AHeroCharacter::StopSprint);
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("AHeroCharacter '%s': SprintAction not assigned (expected /Game/Input/Actions/IA_Sprint via BP_HeroCharacter, TASK-009) — sprint disabled."), *GetNameSafe(this));
		}

		// Melee attack (GDD §3.1)
		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AHeroCharacter::DoMeleeAttack);
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("AHeroCharacter '%s': AttackAction not assigned (expected /Game/Input/Actions/IA_Attack via BP_HeroCharacter, TASK-009) — melee input disabled."), *GetNameSafe(this));
		}
	}
}

void AHeroCharacter::StartSprint()
{
	if (!bDead)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AHeroCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AHeroCharacter::DoMeleeAttack()
{
	// placement mode owns the LMB (TASK-007); a suppressed swing must not consume the cooldown
	if (bDead || bMeleeSuppressed)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// rate limit: one swing per MeleeCooldown seconds (GDD §3.1: 0.5 s)
	const double Now = World->GetTimeSeconds();
	if ((Now - LastMeleeTime) < MeleeCooldown)
	{
		return;
	}
	LastMeleeTime = Now;

	// swing feedback (TASK-016, playtest R1 finding 1): montage on EVERY swing that
	// passes the cooldown — hit or whiff. Suppressed/dead swings returned above and
	// never reach here. VISUAL ONLY: damage below is applied immediately this frame
	// and never waits on anim notifies. Null-safe — nothing is wired until TASK-017.
	// PlayAnimMontage itself jumps to the start section when one is set
	// (NAME_None plays from the montage start).
	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage, 1.0f, AttackMontageSection);
	}

	const FVector MyLocation = GetActorLocation();

	// facing in the horizontal plane (character yaw; bOrientRotationToMovement keeps pitch/roll at 0)
	FVector Facing = GetActorForwardVector();
	Facing.Z = 0.f;
	Facing = Facing.GetSafeNormal();
	if (Facing.IsNearlyZero())
	{
		Facing = GetActorForwardVector();
	}
	const float MinCosAngle = FMath::Cos(FMath::DegreesToRadians(MeleeHalfAngleDegrees));

	// hit ALL enemy team agents within MeleeRange and inside the ±MeleeHalfAngleDegrees cone (GDD §3.1)
	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	bool bDealtDamage = false;

	// TASK-016 feedback-only tracking: true when >= 1 target's TakeDamage actually
	// applied damage (receivers return 0 for ignored hits). Deliberately separate from
	// bDealtDamage, which stays byte-identical to M1 for the regen re-arm below.
	bool bAnyEnemyDamaged = false;

	for (AActor* Target : TeamAgents)
	{
		if (!IsValid(Target) || Target == this)
		{
			continue;
		}

		// no friendly fire (GDD §3.0)
		const ITeamAgent* TargetAgent = Cast<ITeamAgent>(Target);
		if (!TargetAgent || TargetAgent->GetTeamId() == Team)
		{
			continue;
		}

		// range: measure to the closest point on the target's collision so large bodies
		// (the castle: ~800x800 footprint, origin at center) are reachable from their walls.
		// ECC_Pawn is blocked by both pawn capsules and default static mesh collision.
		// Falls back to the actor origin when no usable collision exists (e.g. SM_Castle
		// not yet imported — the castle resolves its mesh null-safe, TASK-002/013).
		FVector ClosestPoint = FVector::ZeroVector;
		float Distance = Target->ActorGetDistanceToCollision(MyLocation, ECC_Pawn, ClosestPoint);
		if (Distance < 0.f)
		{
			ClosestPoint = Target->GetActorLocation();
			Distance = static_cast<float>(FVector::Dist(MyLocation, ClosestPoint));
		}
		if (Distance > MeleeRange)
		{
			continue;
		}

		// cone: horizontal angle from facing to the hit point; a target we are standing
		// inside/overlapping has no defined direction and counts as in the cone
		FVector ToTarget = ClosestPoint - MyLocation;
		ToTarget.Z = 0.f;
		if (ToTarget.SizeSquared() > UE_KINDA_SMALL_NUMBER)
		{
			if (FVector::DotProduct(Facing, ToTarget.GetSafeNormal()) < MinCosAngle)
			{
				continue;
			}
		}

		// hero as instigator/causer so receivers (castle, units) can attribute team (GDD §3.0)
		const float DamageApplied = UGameplayStatics::ApplyDamage(Target, MeleeDamage, GetController(), this, UDamageType::StaticClass());
		bDealtDamage = true;

		// impact feedback (TASK-016): puff at the exact point we struck — ClosestPoint is
		// the closest point on the target's collision, already fallen back to the actor
		// location above when no usable collision exists — but only for targets that
		// really took damage (e.g. a destroyed castle returns 0 and gets no puff).
		if (DamageApplied > 0.f)
		{
			bAnyEnemyDamaged = true;
			if (HitImpactEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, HitImpactEffect, ClosestPoint);
			}
		}
	}

	// dealing damage re-arms the out-of-combat regen delay (GDD §3.1); a whiff does not
	if (bDealtDamage)
	{
		LastCombatTime = Now;
	}

	// hit feedback (TASK-016): one camera shake per swing when >= 1 enemy was actually
	// damaged, on the local player controller (M1 is local-only; an AI/unpossessed hero
	// simply has no APlayerController and no shake). Null-safe until TASK-017 wires it.
	if (bAnyEnemyDamaged && HitCameraShake)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->ClientStartCameraShake(HitCameraShake);
		}
	}
}

void AHeroCharacter::Rally()
{
	// dead hero has no abilities; ResetHero re-enables Rally on respawn
	if (bDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// cooldown gate (GDD §4: 20 s). A press on cooldown is a no-op — but we still
	// emit the optional refusal broadcast so a HUD can flash the remaining time.
	const double Now = World->GetTimeSeconds();
	const double SinceLastRally = Now - LastRallyTime;
	if (SinceLastRally < RallyCooldown)
	{
		const float CooldownRemaining = static_cast<float>(RallyCooldown - SinceLastRally);
		OnRallyStateChanged.Broadcast(/*bReady=*/ false, CooldownRemaining);
		return;
	}
	LastRallyTime = Now;

	// Buff every friendly (same-team) summoned unit within RallyRadius: +RallySpeedBonus
	// move speed for RallyDuration seconds (GDD §4 — units ONLY, never the hero, never
	// enemy units). GetAllActorsOfClass(ASummonedUnit) already excludes the hero, castles
	// and buildings; the team check drops enemy units. AMinerUnit is an ASummonedUnit
	// subclass, so friendly miners are included (a harmless temporary walk boost).
	const FVector MyLocation = GetActorLocation();
	const float RallyRadiusSquared = RallyRadius * RallyRadius;
	const float SpeedMultiplier = 1.f + RallySpeedBonus;

	TArray<AActor*> UnitActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASummonedUnit::StaticClass(), UnitActors);
	for (AActor* UnitActor : UnitActors)
	{
		ASummonedUnit* FriendlyUnit = Cast<ASummonedUnit>(UnitActor);
		if (!FriendlyUnit || FriendlyUnit->IsUnitDead() || FriendlyUnit->GetTeamId() != Team)
		{
			continue;
		}

		// center-to-center range: units are small pawn capsules, so the simple distance
		// is the natural "within 600" test (unlike the melee, which uses closest-point
		// for the castle's huge footprint).
		if (FVector::DistSquared(MyLocation, FriendlyUnit->GetActorLocation()) > RallyRadiusSquared)
		{
			continue;
		}

		FriendlyUnit->ApplyMoveSpeedBuff(SpeedMultiplier, RallyDuration);
	}

	// start the cooldown and tell listeners Rally is now unavailable; OnRallyReady
	// broadcasts (true, 0) when RallyCooldown elapses (GDD §4).
	OnRallyStateChanged.Broadcast(/*bReady=*/ false, RallyCooldown);
	World->GetTimerManager().SetTimer(RallyCooldownTimerHandle, this, &AHeroCharacter::OnRallyReady, RallyCooldown, /*bLoop=*/ false);
}

void AHeroCharacter::OnRallyReady()
{
	// cooldown elapsed — Rally is usable again (GDD §4). CooldownRemaining = 0.
	OnRallyStateChanged.Broadcast(/*bReady=*/ true, 0.f);
}

float AHeroCharacter::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bDead)
	{
		return 0.f;
	}

	// no friendly fire (GDD §3.0)
	if (IsFriendlyDamage(EventInstigator, DamageCauser))
	{
		return 0.f;
	}

	const float ActualDamage = Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.f)
	{
		return 0.f;
	}

	CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.f, MaxHP);

	// taking damage re-arms the out-of-combat regen delay (GDD §3.1)
	if (const UWorld* World = GetWorld())
	{
		LastCombatTime = World->GetTimeSeconds();
	}

	if (CurrentHP <= 0.f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

void AHeroCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	// Deliberately NOT calling Super: AActor::FellOutOfWorld() destroys the
	// actor, and the hero must survive falling off the world (TASK-024, M1
	// carry-over — GDD §3.1: hero death never loses the match).

	// Already dead: the hidden corpse can sit below KillZ until the respawn
	// teleport (the engine re-invokes this per movement tick down there) —
	// the respawn timer is already running, or the match ended and PlayAgain
	// owns restoration. Nothing to do either way.
	if (bDead)
	{
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] Hero fell out of the world (KillZ) — routing through the standard death -> 5 s respawn path (GDD §3.1) instead of AActor's Destroy()."),
		*GetNameSafe(this));

	// The exact combat-death path: exactly-once side effects (hide, stop
	// movement — which also ends the fall — disable input/collision) plus the
	// OnHeroDied broadcast that drives the game mode's 5 s respawn (TASK-006).
	HandleDeath();
}

bool AHeroCharacter::IsFriendlyDamage(AController* EventInstigator, AActor* DamageCauser) const
{
	// prefer the damage causer's team (hero melee and unit attacks pass the attacking actor)
	if (const ITeamAgent* CauserAgent = Cast<ITeamAgent>(DamageCauser))
	{
		return CauserAgent->GetTeamId() == Team;
	}

	// fall back to the instigating controller's pawn
	if (EventInstigator)
	{
		if (const ITeamAgent* InstigatorAgent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			return InstigatorAgent->GetTeamId() == Team;
		}
	}

	// unattributable damage (world hazards etc.) is not friendly — it applies
	return false;
}

void AHeroCharacter::HandleDeath()
{
	// death side effects run exactly once per death
	if (bDead)
	{
		return;
	}
	bDead = true;
	CurrentHP = 0.f;

	// stop and freeze movement (also clears a held sprint)
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// hide and disable collision (GDD §3.1)
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// disable player input; passing null disables for all player controllers, which is the
	// desired behavior if we are somehow unpossessed at the moment of death
	DisableInput(Cast<APlayerController>(GetController()));

	// notify listeners (the game mode owns respawn timing, TASK-006)
	OnHeroDied.Broadcast(this);
}

void AHeroCharacter::ResetHero()
{
	bDead = false;
	CurrentHP = MaxHP;

	// restore visibility and collision
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// restore movement at base walk speed
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// restore input (mirrors HandleDeath's DisableInput)
	EnableInput(Cast<APlayerController>(GetController()));

	// fresh cooldown/regen state: can swing immediately; full HP so regen is idle anyway
	LastMeleeTime = -1.0e9;
	LastCombatTime = -1.0e9;

	// fresh Rally state (TASK-042): drop any pending cooldown so a respawned hero can
	// Rally immediately, and tell listeners it is ready (mirrors the melee/regen reset).
	GetWorldTimerManager().ClearTimer(RallyCooldownTimerHandle);
	LastRallyTime = -1.0e9;
	OnRallyStateChanged.Broadcast(/*bReady=*/ true, 0.f);
}
