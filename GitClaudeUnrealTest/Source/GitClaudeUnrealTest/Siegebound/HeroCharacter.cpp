// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HeroCharacter.h"

#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "GitClaudeUnrealTest.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): Team replication registration
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channel for the capsule stamp
#include "Siegebound/SiegeNetLimits.h" // M8 (TASK-356 loop-1): the ONE arena relevancy constant (Tier B)
#include "Siegebound/SiegePlayerState.h" // M8 (TASK-356): PossessedBy team resolve (doc §2.3)
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"

namespace
{
	/** Priority of IMC_Hero on the input subsystem — above any template context (which the template controllers add at 0). */
	constexpr int32 HeroMappingContextPriority = 1;

	//~ §6 hero audio soft-ref paths (TASK-179) — null-safe; the sounds arrive in TASK-180.
	const TCHAR* HeroSwingSoundPath = TEXT("/Game/Audio/S_HeroSwing"); // every swing past cooldown
	const TCHAR* HeroHitSoundPath = TEXT("/Game/Audio/S_HeroHit");     // a swing that damaged >= 1 enemy

	/** Height above the hero origin for its floating damage number. */
	constexpr float HeroDamageNumberHeightZ = 110.f;

	//~ Instant hero-upgrade CardIDs (TASK-058) — must match the DT_Cards row names (CONVENTIONS: CardID = row name).
	const FName UpgradeCardID_SharpenedBlade(TEXT("SharpenedBlade"));
	const FName UpgradeCardID_PlateArmor(TEXT("PlateArmor"));
	const FName UpgradeCardID_SwiftBoots(TEXT("SwiftBoots"));
	const FName UpgradeCardID_WarBanner(TEXT("WarBanner"));
}

AHeroCharacter::AHeroCharacter()
{
	// needed for out-of-combat HP regen
	PrimaryActorTick.bCanEverTick = true;

	// ⚖️ NET RELEVANCY — TIER B (CONVENTIONS NET RELEVANCY LAW; TASK-356 loop-1).
	// A hero already replicates (APawn's constructor sets bReplicates, engine
	// Pawn.cpp:86). A player's OWN pawn is owner-relevant regardless of distance,
	// but the ENEMY hero must read across the whole 500 m arena — under the
	// engine's default 150 m cull it would pop in and out at range, and its
	// replicated Team (the CASTLE-3X gate-channel truth) would arrive late.
	// The distance comes from the ONE arena constant — NEVER a per-class literal
	// (that drift trap is exactly why Tier B is defined this way): when the arena
	// grows, SiegeNet::ArenaRelevancyDistance changes and this follows.
	// UE 5.5+ API: the raw NetCullDistanceSquared field is deprecated for public
	// access — the setter is the supported path.
	SetNetCullDistanceSquared(SiegeNet::ArenaRelevancyDistanceSquared);

	// GDD §3.1 base walk speed (BeginPlay re-applies in case a blueprint tweaks WalkSpeed)
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	// M6.6 climbable-terrain tuning (TASK-141): raise step height / walkable-floor angle / jump so the
	// hero comfortably WALKS (not jumps) up the ≤30° hill flanks to a flat crown. BeginPlay re-applies
	// so a BP_HeroCharacter tweak survives — mirrors the WalkSpeed pattern above / ApplyMovementSpeed.
	ApplyTerrainMovementTuning();

	// Overhead health bar (TASK-130 castle-parity REBUILD): one screen-space, team-tinted PUSH
	// bar. ADDITIVE to the hero's own WBP_HUD HP (M1) — the component binds this hero's OnHPChanged
	// delegate (GetMaxHP() is already the EFFECTIVE Plate-Armor max), and the widget class
	// soft-resolves null-safe at the component's own BeginPlay (WBP_CombatantHealthBar, TASK-131).
	// Attached to the capsule root. No poll timer.
	HPBarWidget = CreateDefaultSubobject<UCombatantHealthBarComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(GetCapsuleComponent());

	// §6 hit-flash (TASK-154): flashes the template skeletal GetMesh() white on every
	// actual damage event (driven from TakeDamage). Overlay-based, null-safe.
	HitFlashComponent = CreateDefaultSubobject<USiegeHitFlashComponent>(TEXT("HitFlashComponent"));

	// data contract (TASK-058 names block): stack caps resolve from MaxCopies in this table
	// at ApplyUpgrade time, never from code (GDD §3.0). Mirrors ASummonedUnit's CardTableAsset.
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));
}

void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// TASK-349 (CONVENTIONS "Castle 3× HOLLOW" team-gating, hero ruling — default,
	// FLAGGED to Jonathan): the hero is a team combatant, so his capsule is
	// re-typed to the team object channel exactly like every ASummonedUnit — the
	// ENEMY castle's GateBlockerVolume physically stops him at the gate while his
	// OWN gate ignores him. Object type ONLY (response matrix untouched): melee
	// distance math (ECC_Pawn), camera probes, and world blocking are
	// byte-identical. No nav filter here — the hero is player-driven, never
	// pathfinds (the nav lane is units-only). Null-safe; team read via the
	// ITeamAgent contract. Reverting to hero-raids is this one line.
	if (UCapsuleComponent* HeroCapsule = GetCapsuleComponent())
	{
		HeroCapsule->SetCollisionObjectType(SiegeTeamObjectChannel(GetTeamId()));
	}

	// hard-resolve DT_Cards once (TASK-058): ApplyUpgrade reads MaxCopies from it for the stack cap.
	// DT_Cards is small and already resident by the time a hero exists (units hard-reference it);
	// a missing table is logged and refuses upgrade plays rather than guessing a cap (GDD §3.0).
	CachedCardTable = CardTableAsset.IsNull() ? nullptr : CardTableAsset.LoadSynchronous();
	if (!CachedCardTable)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AHeroCharacter '%s': DT_Cards ('%s') unavailable at BeginPlay — hero-upgrade stack caps cannot be resolved; upgrade plays will be refused until it loads."),
			*GetNameSafe(this), *CardTableAsset.ToString());
	}

	// no upgrades own the fresh hero yet, so effective == base here; use the effective accessors
	// anyway so the initial state is identical to a respawn re-apply (single code path).
	bSprinting = false;
	CurrentHP = GetEffectiveMaxHP();
	// Push init HP to the overhead bar (TASK-130 push model); reconciles the component's
	// InitForCombatant seed (run during Super::BeginPlay above) to the effective max.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
	ApplyMovementSpeed();

	// M6.6 (TASK-141): re-apply the climbable-terrain tunables so a BP_HeroCharacter tweak survives
	// (mirrors ApplyMovementSpeed's ctor->BeginPlay re-apply pattern directly above).
	ApplyTerrainMovementTuning();

	// far in the past: the first swing is never cooldown-blocked and a below-max hero regens immediately
	LastMeleeTime = -1.0e9;
	LastCombatTime = -1.0e9;
}

void AHeroCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// out-of-combat regen (GDD §3.1): 5 HP/s starting 8 s after last taking OR dealing damage, stops at max.
	// Cap is the EFFECTIVE max (base + Plate Armor bonus, TASK-058) — never the raw base.
	const float EffectiveMaxHP = GetEffectiveMaxHP();
	if (!bDead && CurrentHP < EffectiveMaxHP)
	{
		const UWorld* World = GetWorld();
		if (World && (World->GetTimeSeconds() - LastCombatTime) >= RegenDelay)
		{
			CurrentHP = FMath::Min(CurrentHP + (RegenRate * DeltaSeconds), EffectiveMaxHP);
			// Push the regen to the overhead bar each frame it ticks (TASK-130 push model — the
			// outer guard ensures CurrentHP actually rose). Mirrors ACastle::HandleHealTick.
			OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
		}
	}
}

void AHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// M8 hero team (TASK-356 doc §2.3/D4 — the audit-§1b#4 fix: NOTHING ever
	// assigned a hero team). Server-side by engine contract; the seat latch
	// (InitNewPlayer) settled the PS team BEFORE RestartPlayer, so this read is
	// authoritative. Ordering (addendum §1): possession precedes the pawn's
	// FIRST net update (spawn + possess in one server frame; the NetDriver
	// replicates at frame end), so the initial bunch already carries the correct
	// Team to client proxies. Warn + keep-default when unresolvable (a
	// non-player possession — defensive).
	const ASiegePlayerState* SiegePS = NewController ? NewController->GetPlayerState<ASiegePlayerState>() : nullptr;
	if (SiegePS)
	{
		Team = SiegePS->GetTeam();
	}
	else
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("AHeroCharacter '%s': PossessedBy could not resolve an ASiegePlayerState — Team stays %s (default; doc §2.3)."),
			*GetNameSafe(this), Team == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
	}

	// RE-STAMP the CASTLE-3X capsule channel (addendum §1): the server-side
	// BeginPlay stamp ran BEFORE possession (SpawnDefaultPawnFor begins play,
	// then Possess) with the pre-assign Team. Idempotent — the host re-applies
	// Blue; the client's hero corrects to Red server-side in the same frame.
	if (UCapsuleComponent* HeroCapsule = GetCapsuleComponent())
	{
		HeroCapsule->SetCollisionObjectType(SiegeTeamObjectChannel(GetTeamId()));
	}
}

void AHeroCharacter::OnRep_Team()
{
	// M8 (TASK-356 — the addendum-§1 resolution of the doc's §7.1 reserved seam):
	// the CLIENT proxy re-stamps its CASTLE-3X capsule channel with the
	// replicated truth, closing the BeginPlay-vs-rep ordering class — the
	// predicted hero's gate collision now always matches the server's
	// (own gate passes, enemy gate blocks; no rubber-band). Idempotent.
	if (UCapsuleComponent* HeroCapsule = GetCapsuleComponent())
	{
		HeroCapsule->SetCollisionObjectType(SiegeTeamObjectChannel(GetTeamId()));
	}

	UE_LOG(LogSiegeNet, Log,
		TEXT("AHeroCharacter '%s': Team replicated: %s — capsule channel re-stamped (M8 addendum §1)."),
		*GetNameSafe(this), Team == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
}

void AHeroCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The ONE P1 hero property (doc §3.6): plain — assigned once at possession
	// (before first rep on the normal path), and the OnRep re-stamp makes every
	// ordering edge self-correcting. Full hero replication (HP/upgrades/melee
	// RPC) is P2.
	DOREPLIFETIME(AHeroCharacter, Team);
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

		// Rally active ability (GDD §4) — bound to IA_Rally / key Q via BP_HeroCharacter in TASK-048
		if (RallyAction)
		{
			EnhancedInputComponent->BindAction(RallyAction, ETriggerEvent::Started, this, &AHeroCharacter::Rally);
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("AHeroCharacter '%s': RallyAction not assigned (expected /Game/Input/Actions/IA_Rally via BP_HeroCharacter, TASK-048) — rally input disabled."), *GetNameSafe(this));
		}
	}
}

void AHeroCharacter::StartSprint()
{
	if (!bDead)
	{
		// track the sprint state so ApplyMovementSpeed picks the sprint tier; effective speed
		// composes Swift Boots (TASK-058) on top of the base SprintSpeed (base never mutated).
		bSprinting = true;
		ApplyMovementSpeed();
	}
}

void AHeroCharacter::StopSprint()
{
	bSprinting = false;
	ApplyMovementSpeed();
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

	// §6 hero-swing audio (TASK-179): the whoosh on EVERY swing past the cooldown (hit or
	// whiff), matching the montage. Null-safe until S_HeroSwing lands (TASK-180).
	USiegeFeedbackLibrary::PlayWorldSound(this, HeroSwingSoundPath, MyLocation);

	// ── M8 combat-authority gate (TASK-356 doc §3.6/D4/§4.2): DAMAGE only ever
	// applies on the authority. On a CLIENT hero the swing montage + whoosh above
	// still play (local feedback), but the target sweep below never runs — a
	// client-side ApplyDamage on the castle would write a local HP value under
	// the replicated one (visible flicker-then-snap lie), and the honest melee
	// relay (ServerMeleeAttack) is P2's combat-authority work. The P1 Red client
	// therefore deals NO damage — the D5 observer posture. Standalone/host:
	// authority ⇒ everything below byte-identical.
	if (!HasAuthority())
	{
		return;
	}

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

		// hero as instigator/causer so receivers (castle, units) can attribute team (GDD §3.0).
		// EFFECTIVE melee = base MeleeDamage + Sharpened Blade bonus (TASK-058) — composed live,
		// base never mutated, so it persists through respawn and resets with the stacks.
		const float DamageApplied = UGameplayStatics::ApplyDamage(Target, GetEffectiveMeleeDamage(), GetController(), this, UDamageType::StaticClass());
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

	// §6 hero-hit audio (TASK-179): the meaty impact when the swing damaged >= 1 enemy
	// (separate from the always-on swing whoosh above). Null-safe until S_HeroHit lands.
	if (bAnyEnemyDamaged)
	{
		USiegeFeedbackLibrary::PlayWorldSound(this, HeroHitSoundPath, MyLocation);
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

	CurrentHP = FMath::Clamp(CurrentHP - ActualDamage, 0.f, GetEffectiveMaxHP());

	// Push the damage to the overhead bar BEFORE any death handling below (ACastle::TakeDamage
	// parity — listeners see the value, and HandleDeath re-broadcasts 0 + hides).
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// §6 damage feedback (TASK-154/156) — ACTUAL damage only (friendly fire returned above;
	// regen never routes here). Flash the hero white ~0.1 s and float the dealt amount over
	// the hero, tinted by team. Both null-safe; run before HandleDeath so the death still flashes.
	if (HitFlashComponent)
	{
		HitFlashComponent->TriggerFlash();
	}
	USiegeFeedbackLibrary::ShowDamageNumber(this, ActualDamage,
		GetActorLocation() + FVector(0.f, 0.f, HeroDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));

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

	// Push the 0-HP to the overhead bar, then HIDE it (TASK-130 push model): a screen-space
	// widget component does NOT follow SetActorHiddenInGame (ACastle::HandleDestroyed parity),
	// so hide explicitly. ResetHero re-shows it on respawn.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
	if (HPBarWidget)
	{
		HPBarWidget->HideBar();
	}

	// stop and freeze movement (also clears a held sprint). Upgrades PERSIST through death
	// (stacks are untouched here); the effective resting speed still composes Swift Boots.
	bSprinting = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->MaxWalkSpeed = GetEffectiveWalkSpeed();

	// pause the War Banner aura while dead/hidden — the stack persists and ResetHero re-arms
	// the pulse on respawn (TASK-058 persistence). Clears the timer only, never the stack.
	StopWarBannerAura();

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

	// Upgrades PERSIST through death (GDD §3.10, TASK-058): the stack counts were NOT cleared
	// by HandleDeath, so re-apply their cumulative mods onto the freshly-restored base here —
	// this is the death→respawn persistence hook. Full HP is the EFFECTIVE max (base + Plate
	// Armor), so a Plate-Armored hero respawns at, e.g., 300/300.
	CurrentHP = GetEffectiveMaxHP();

	// Refill the overhead bar to full and re-show it (TASK-130 push model — counterpart of
	// HandleDeath's hide; ACastle::ResetCastle parity).
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
	if (HPBarWidget)
	{
		HPBarWidget->ShowBarIfEnabled();
	}

	// restore visibility and collision
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// restore movement; effective walk speed re-applies Swift Boots (base never mutated).
	bSprinting = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ApplyMovementSpeed();

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

	// re-arm the War Banner aura pulse if it is still owned (paused by HandleDeath); no-op otherwise.
	StartWarBannerAura();

	// re-push the current upgrade loadout so a HUD rebuilt around the respawn (TASK-064) is accurate.
	BroadcastUpgradesChanged();
}

EHeroUpgradeResult AHeroCharacter::ApplyUpgrade(FName UpgradeCardID)
{
	// Only the four Instant upgrades are valid here (GDD §3.10/§4). An unknown CardID is a
	// caller error — refuse with no side effects so the play is refunded net-zero (§3.0).
	const bool bKnownUpgrade =
		UpgradeCardID == UpgradeCardID_SharpenedBlade ||
		UpgradeCardID == UpgradeCardID_PlateArmor ||
		UpgradeCardID == UpgradeCardID_SwiftBoots ||
		UpgradeCardID == UpgradeCardID_WarBanner;
	if (!bKnownUpgrade)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AHeroCharacter '%s': ApplyUpgrade('%s') is not a known hero upgrade (expected SharpenedBlade/PlateArmor/SwiftBoots/WarBanner) — refused."),
			*GetNameSafe(this), *UpgradeCardID.ToString());
		return EHeroUpgradeResult::RefusedInvalidCard;
	}

	// The stack cap is the card's MaxCopies from DT_Cards (never hardcoded, GDD §3.0). A missing
	// table/row yields 0 — refuse rather than guess (mirrors the unit's "no table → no stats").
	const int32 StackCap = GetStackCapForUpgrade(UpgradeCardID);
	if (StackCap <= 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AHeroCharacter '%s': ApplyUpgrade('%s') could not resolve a stack cap from DT_Cards (MaxCopies) — refused."),
			*GetNameSafe(this), *UpgradeCardID.ToString());
		return EHeroUpgradeResult::RefusedInvalidCard;
	}

	// Over-cap: refuse so TASK-059 shows "… at max stacks" and refunds with NO spend (§3.10).
	if (GetUpgradeStackCount(UpgradeCardID) >= StackCap)
	{
		return EHeroUpgradeResult::RefusedAtMaxStacks;
	}

	// Add the stack, then apply the per-stack effect. Base stats are NEVER mutated — every bonus
	// derives live from the (now incremented) stack count, so respawn re-application is automatic
	// and there is nothing to drift (the Rally/aura cache-once discipline, applied to the hero).
	if (UpgradeCardID == UpgradeCardID_SharpenedBlade)
	{
		++SharpenedBladeStacks; // melee bonus is live via GetEffectiveMeleeDamage()
	}
	else if (UpgradeCardID == UpgradeCardID_PlateArmor)
	{
		++PlateArmorStacks; // raises GetEffectiveMaxHP() by MaxHPBonus
		// heal by exactly one stack's MaxHPBonus, clamped to the NEW effective max (GDD §3.10:
		// "heals 100"). Skipped while dead — a corpse is not revived; respawn heals to full max.
		if (!bDead)
		{
			CurrentHP = FMath::Min(CurrentHP + MaxHPBonus, GetEffectiveMaxHP());
			// Plate Armor raised BOTH CurrentHP and the effective max — push so the overhead
			// bar's denominator and fill both update (TASK-130 push model).
			OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
		}
	}
	else if (UpgradeCardID == UpgradeCardID_SwiftBoots)
	{
		++SwiftBootsStacks;
		ApplyMovementSpeed(); // push the new effective walk/sprint into the movement component
	}
	else // WarBanner
	{
		++WarBannerStacks;
		StartWarBannerAura(); // begins pulsing SetAuraDamageBonus on friendlies (no-op while dead — resumes on respawn)
	}

	BroadcastUpgradesChanged();
	return EHeroUpgradeResult::Applied;
}

void AHeroCharacter::ResetUpgrades()
{
	// Full reset to the base hero (GDD §3.9 Play Again). Called by the match-reset owner
	// (ASiegeGameMode::PlayAgain — see handoffs/TASK-058.md; wired outside this task's files).
	SharpenedBladeStacks = 0;
	PlateArmorStacks = 0;
	SwiftBootsStacks = 0;
	WarBannerStacks = 0;

	// aura off, speed back to base, and drop any Plate-Armor HP overflow above the new base max.
	StopWarBannerAura();
	ApplyMovementSpeed();
	CurrentHP = FMath::Min(CurrentHP, GetEffectiveMaxHP());
	// Resetting upgrades lowered the effective max — push so the overhead bar's denominator
	// (and any clamped-down current) update (TASK-130 push model).
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	BroadcastUpgradesChanged();
}

void AHeroCharacter::ApplyMovementSpeed()
{
	// single place that writes MaxWalkSpeed from the effective speeds — walk vs sprint per the
	// held-sprint flag. Base WalkSpeed/SprintSpeed are never mutated; Swift Boots composes here.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bSprinting ? GetEffectiveSprintSpeed() : GetEffectiveWalkSpeed();
	}
}

void AHeroCharacter::ApplyTerrainMovementTuning()
{
	// M6.6 climbable-terrain margins (TASK-141): push the step-up, walkable-floor angle, and jump
	// apex onto the movement component so the hero comfortably walks up the ≤30° hill flanks to a
	// flat crown (the ≤30° faces are already climbable — this is comfort/margin only; GravityScale
	// and AirControl are deliberately untouched). Hero-prefixed members avoid shadowing the
	// component's identically-named fields. WalkableFloorAngle goes through SetWalkableFloorAngle so
	// the cached WalkableFloorZ (the value used at runtime) recomputes — never write the field raw.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxStepHeight = HeroMaxStepHeight;
		MoveComp->SetWalkableFloorAngle(HeroWalkableFloorAngle);
		MoveComp->JumpZVelocity = HeroJumpZVelocity;
	}
}

void AHeroCharacter::StartWarBannerAura()
{
	// aura only runs while the upgrade is owned AND the hero is alive; it resumes on respawn
	// (ResetHero calls this) after HandleDeath paused it.
	if (bDead || WarBannerStacks <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// idempotent re-arm (cap 1 means at most one owner, but ResetHero may re-call): clear then set.
	World->GetTimerManager().ClearTimer(WarBannerAuraTimerHandle);

	// buff in-range friendlies immediately, then keep pulsing every interval.
	PulseWarBannerAura();
	World->GetTimerManager().SetTimer(WarBannerAuraTimerHandle, this, &AHeroCharacter::PulseWarBannerAura, WarBannerPulseInterval, /*bLoop=*/ true);
}

void AHeroCharacter::StopWarBannerAura()
{
	// clears the pulse timer ONLY — never the stack (persistence). The units' own aura windows
	// then self-expire via SetAuraDamageBonus's one-shot timer (TASK-055), restoring exactly 1.0.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarBannerAuraTimerHandle);
	}
}

void AHeroCharacter::PulseWarBannerAura()
{
	if (bDead || WarBannerStacks <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector MyLocation = GetActorLocation();
	const float AuraRadiusSquared = WarBannerAuraRadius * WarBannerAuraRadius;

	// window slightly longer than the pulse interval so a unit that stays in range never flickers
	// between pulses; a unit that walks out keeps the bonus for at most this window, then it
	// self-expires on the unit's side (SetAuraDamageBonus refresh-not-stack contract, TASK-055).
	const float PulseDuration = WarBannerPulseInterval * 2.f;

	// mirror Rally's iterate-friendlies-in-radius loop (TASK-042): GetAllActorsOfClass(ASummonedUnit)
	// already excludes the hero, castles and buildings; the team check drops enemy units; a friendly
	// miner (an ASummonedUnit) is harmlessly included (its combat machine is sealed). SetAuraDamageBonus
	// itself no-ops on dead/match-end-frozen units, so a match-end aura pulse buffs nobody.
	TArray<AActor*> UnitActors;
	UGameplayStatics::GetAllActorsOfClass(World, ASummonedUnit::StaticClass(), UnitActors);
	for (AActor* UnitActor : UnitActors)
	{
		ASummonedUnit* FriendlyUnit = Cast<ASummonedUnit>(UnitActor);
		if (!FriendlyUnit || FriendlyUnit->IsUnitDead() || FriendlyUnit->GetTeamId() != Team)
		{
			continue;
		}

		if (FVector::DistSquared(MyLocation, FriendlyUnit->GetActorLocation()) > AuraRadiusSquared)
		{
			continue;
		}

		FriendlyUnit->SetAuraDamageBonus(WarBannerDamageBonus, PulseDuration);
	}
}

int32 AHeroCharacter::GetStackCapForUpgrade(FName UpgradeCardID) const
{
	if (const UDataTable* CardTable = CachedCardTable)
	{
		static const FString Context(TEXT("AHeroCharacter::GetStackCapForUpgrade"));
		if (const FCardRow* Row = CardTable->FindRow<FCardRow>(UpgradeCardID, Context, /*bWarnIfRowMissing=*/ false))
		{
			return Row->MaxCopies;
		}
	}
	return 0; // table/row unavailable — caller refuses; the cap is never guessed (GDD §3.0)
}

int32 AHeroCharacter::GetUpgradeStackCount(FName UpgradeCardID) const
{
	if (UpgradeCardID == UpgradeCardID_SharpenedBlade) { return SharpenedBladeStacks; }
	if (UpgradeCardID == UpgradeCardID_PlateArmor)     { return PlateArmorStacks; }
	if (UpgradeCardID == UpgradeCardID_SwiftBoots)     { return SwiftBootsStacks; }
	if (UpgradeCardID == UpgradeCardID_WarBanner)      { return WarBannerStacks; }
	return 0;
}

int32 AHeroCharacter::GetUpgradeStackCap(FName UpgradeCardID) const
{
	return GetStackCapForUpgrade(UpgradeCardID);
}

void AHeroCharacter::BroadcastUpgradesChanged()
{
	OnHeroUpgradesChanged.Broadcast(SharpenedBladeStacks, PlateArmorStacks, SwiftBootsStacks, WarBannerStacks);
}
