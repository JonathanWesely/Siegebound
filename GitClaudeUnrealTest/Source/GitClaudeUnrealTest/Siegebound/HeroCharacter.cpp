// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HeroCharacter.h"

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
		UGameplayStatics::ApplyDamage(Target, MeleeDamage, GetController(), this, UDamageType::StaticClass());
		bDealtDamage = true;
	}

	// dealing damage re-arms the out-of-combat regen delay (GDD §3.1); a whiff does not
	if (bDealtDamage)
	{
		LastCombatTime = Now;
	}
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
}
