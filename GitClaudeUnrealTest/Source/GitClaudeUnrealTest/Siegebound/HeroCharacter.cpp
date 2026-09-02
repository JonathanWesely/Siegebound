// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HeroCharacter.h"

#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h" // TASK-512: complete type for GetGameInstance()->GetSubsystem<>() (Actor.h:3772 forward-declares UGameInstance)
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameModeBase.h" // TASK-748: HasMatchEnded() — the recall channel's match-end exit
#include "GameFramework/PlayerController.h"
#include "GitClaudeUnrealTest.h"
#include "InputAction.h" // TASK-748: complete type for the IA_Recall soft-resolve
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): Team replication registration
#include "NiagaraComponent.h" // TASK-748: complete type for the R-3 channel tell's teardown
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h" // TASK-512: USiegeKeyboardLayoutSubsystem::GetPositionalContext — the positional remap's ONE call site
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

	// TASK-748 (RECALL-§2): the recall key's action asset, soft — TASK-747 creates IA_Recall and
	// maps it to B inside IMC_Hero. Until it lands, SetupPlayerInputComponent resolves null, logs
	// ONE line and leaves the key completely INERT. ⭐ That is the DESIGNED state, not a
	// degradation to fix here, and it is why this feature did not have to wait for the asset
	// (the shipped IA_Cmd* pattern — SiegePlayerController.cpp:228/487).
	RecallActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Recall.IA_Recall")));
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

	// TASK-748: service the recall channel (RECALL-§4). Self-guards on the channel state, so this
	// is a single bool read on every frame no channel is running. Driven from Tick and NOT from a
	// timer deliberately: a timer handle is a second thing to leak, and this state's whole design
	// goal is that there be exactly ONE thing for its seven exits to clear (TOWER-§8's lesson).
	if (const UWorld* RecallWorld = GetWorld())
	{
		TickRecall(RecallWorld->GetTimeSeconds());
	}

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
					// ─── POSITIONAL KEYBOARD LAYOUT (TASK-512, KEYBOARD-LAYOUT batch; CONVENTIONS `KBD-§5`/`KBD-§6`) ───
					// On Dvorak the FKey Windows delivers is LAYOUT-dependent, so every letter binding in
					// IMC_Hero (WASD, Q rally, T/R/E/F/C) lands on the wrong PHYSICAL key. The subsystem hands
					// back a transient duplicate whose `.Key` fields are retargeted to the active layout — or,
					// on a positionally-QWERTY host, THE SAME POINTER, with no duplicate and no allocation.
					//
					// ⛔ THE PLACEMENT IS PART OF THE CORRECTNESS, NOT A STYLE CHOICE (`SC-§21`; `KBD-§9`
					// criterion 10). This resolve sits in the INNERMOST scope of the existing guard chain —
					// HeroMappingContext -> APlayerController -> ULocalPlayer -> UEnhancedInputLocalPlayerSubsystem —
					// beside the AddMappingContext call it feeds. NotifyControllerChanged ALSO RUNS ON THE
					// SERVER FOR A REMOTE CLIENT'S PAWN; hoisting this above the GetLocalPlayer() check would
					// probe an OS keyboard layout on behalf of a machine that is not there.
					//
					// ⛔ FAIL-SAFE (`KBD-§5`, last row): no GameInstance or no subsystem => ContextToApply stays
					// HeroMappingContext and the behaviour is byte-identical to before this feature existed.
					// GetPositionalContext never returns null for a non-null input, and this code does not
					// depend on that trust — a null would simply fail AddMappingContext's own guard, so no
					// redundant branch is added here that would hide a contract violation.
					//
					// ⛔ NO NEW STATE AND NO RE-APPLICATION PATH, DELIBERATELY (`KBD-§6`): on a mid-session
					// Win+Space the subsystem re-targets that cached duplicate IN PLACE and calls
					// RequestRebuildControlMappingsUsingContext, so the pointer handed over here stays valid
					// and current. A cached pointer, a tick, or a delegate binding on AHeroCharacter would
					// throw that property away.
					const UInputMappingContext* ContextToApply = HeroMappingContext;
					if (const UGameInstance* GameInstance = GetGameInstance())
					{
						if (USiegeKeyboardLayoutSubsystem* LayoutSubsystem = GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>())
						{
							ContextToApply = LayoutSubsystem->GetPositionalContext(HeroMappingContext);
						}
					}

					Subsystem->AddMappingContext(ContextToApply, HeroMappingContextPriority);
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

		// Recall channel (TASK-748, RECALL-§2) — IA_Recall, mapped to B inside IMC_Hero by
		// TASK-747. The hard slot wins when a blueprint assigns one; otherwise the soft path
		// resolves. ⛔ NO KEY IS NAMED HERE: the whole IMC_Hero context is retargeted for the
		// active OS layout in NotifyControllerChanged, so B follows Dvorak with zero extra code
		// (KBD-§4 tables all 26 letters). ⛔ And the subsystem's SINGLE-KEY positional lookup is
		// deliberately NOT called for it — that API is for RAW POLLED keys and would
		// DOUBLE-TRANSLATE an already-remapped context key (SiegePlayerController.h:1223-1225):
		// wrong ONLY on a non-QWERTY layout, which is invisible to every reviewer here and
		// immediately visible to Jonathan. The test asserts that call by name in both files.
		if (!RecallAction)
		{
			RecallAction = RecallActionAsset.IsNull() ? nullptr : RecallActionAsset.LoadSynchronous();
		}

		if (RecallAction)
		{
			EnhancedInputComponent->BindAction(RecallAction, ETriggerEvent::Started, this, &AHeroCharacter::HandleRecallInput);
		}
		else
		{
			// ⭐ The DESIGNED compile-time state until TASK-747's asset lands: the key is simply
			// INERT. One line, no crash, and nothing else about the hero changes.
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AHeroCharacter '%s': RecallAction not resolved (expected '%s', created in TASK-747) — the recall key is INERT; every other input is unaffected."),
				*GetNameSafe(this), *RecallActionAsset.ToString());
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
	// placement mode owns the LMB (TASK-007); a suppressed swing must not consume the cooldown.
	//
	// ⛔ TASK-748 (R-5) ADDS ONE TERM TO THIS EXISTING GUARD AND CREATES NO NEW GUARD POINT —
	// TOWER-§9.2's idiom, second application. "During this recall animation the player cannot
	// attack" is a STATE, so it is expressed as one, and the refusal shares the shipped
	// property that a refused swing does not even consume the cooldown: the hero can swing the
	// instant the channel ends, with no debt carried out of it.
	//
	// ⭐⭐ AND THE SHAPE HERE IS THE RULING, NOT A DETAIL: IsAttackDisarmed reads the CHANNEL'S
	// STATE, so the SAME hero answers "disarmed" now and "armed" ten seconds from now. A
	// permanent class-identity seal — a const "can this thing ever attack" query on the class —
	// structurally cannot say that, and using one would make a 10-second channel
	// indistinguishable from a unit that can never attack for as long as it exists.
	if (bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState))
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
		// (the castle — ~814x814 at the M1 blockout, ~2438x2462 after Castle-3× and
		// ~7314x7384 at the 9× castle, origin at center) are reachable from their walls.
		// ⭐ Those figures are HISTORY, not a dependency: ActorGetDistanceToCollision
		// measures against the target's live COLLISION, so it follows the mesh by
		// itself. That is why this line survived both resizes untouched while the
		// old "~800x800" text rotted — the MECHANISM was never the stale part.
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

	// ⛔ RECALL EXIT 4 of 7 — R-1: ONLY DAMAGE THAT LANDS INTERRUPTS (TASK-748).
	// "if they get hit with an attack it interupts the channel" is the player's language for
	// TAKING DAMAGE, and this is the one seam where that is knowable: a friendly-fire hit, a
	// hit on an already-dead hero and any 0-damage event have all returned above, so control
	// only reaches this line for damage that actually reduced HP AFTER mitigation.
	//
	// ⚠️ The predicate call is deliberately NOT collapsed into "we got here, so interrupt": the
	// rule lives in ONE named, testable place so it survives any future change to the early
	// returns above — and so a headless test can assert 0 and -1 do NOT interrupt while 0.0001
	// does, which is a claim that can go red.
	if (FSiegeRecallStatics::DamageInterrupts(ActualDamage))
	{
		EndRecall(ESiegeRecallExit::InterruptedByDamage);
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

	// ⛔ RECALL EXIT 5 of 7 — GHOST-§ G-6: the hero died mid-channel, so the channel ABORTS
	// (TASK-748). A channel that survives its own caster's death is TOWER-§8's hanging-unit
	// class in a new costume, and it would arrive 10 seconds later to teleport and full-heal a
	// corpse. ⭐ This covers the KillZ death too: FellOutOfWorld routes through this exact
	// function rather than through TakeDamage, so exit 4 never sees it.
	// Idempotent with exit 4 — lethal damage reaches both in one call stack and the channel
	// still clears exactly once (EndRecall's first line is the latch).
	EndRecall(ESiegeRecallExit::InterruptedByDeath);

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

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  RECALL — the 10-second channel home (TASK-748, CONVENTIONS `RECALL-§`)
//
//  ⛔⛔ EVERYTHING BETWEEN THE TWO SENTINEL LINES BELOW IS SCANNED BY
//  `Siegebound/Tests/SiegeRecallTest.cpp`, WHICH FAILS THE SUITE IF EITHER OF THE TWO TRAPS
//  `RECALL-§1` NAMES EVER APPEARS HERE. Two consequences bind anyone editing this region:
//
//    1. ⛔ THE DEATH-PATH RESTORE FUNCTION (`ResetHero`) MAY NEVER BE CALLED FROM THIS REGION,
//       and — because the scan is textual — ⛔ ITS NAME MAY NOT APPEAR IN THIS REGION'S PROSE
//       EITHER. Why the ban is real and not ceremonial: that function re-applies the hero's
//       CUMULATIVE upgrade mods onto a freshly-restored base, re-arms the War Banner aura, and
//       restores input that death disabled. On a LIVE hero it therefore DOUBLE-APPLIES every
//       upgrade stack and re-arms a running aura — silently, every single time the player
//       recalls, and it would review as completely correct.
//    2. ⛔ THE HEAL READS `GetEffectiveMaxHP()`. The raw base hit-point field must not be read
//       in this region at all — the scan asserts that every occurrence of that field's name
//       here is part of `GetEffectiveMaxHP` or `GetMaxHP`. Reading the base instead silently
//       under-heals a Plate-Armor hero by up to 200 HP while "completely refill their health"
//       quietly becomes a lie.
//
//  ⚠️ If this region is ever renamed or the sentinels are removed, the test does NOT silently
//  pass — it asserts the sentinels exist, exactly once each and in order, and errors if they do
//  not. A probe that goes stale must fail, never report SAFE (`SHIP-§9c`).
// ═══════════════════════════════════════════════════════════════════════════════════════════

// ═════════════ RECALL REGION BEGIN — SiegeRecallTest.cpp scans between the sentinels ═════════════

bool FSiegeRecallStatics::CanBegin(const FSiegeRecallState& State, bool bDead, bool bMatchEnded)
{
	// All three terms are load-bearing and the test's truth table drops each one in turn:
	// a dead hero has no abilities; a second press is a CANCEL and never a second channel
	// (handled by the caller, but refused here too so no other caller can stack one); and a
	// decided match must not be re-entered by a teleport-and-heal arriving under the end screen.
	return !State.bChannelling && !bDead && !bMatchEnded;
}

FSiegeRecallState FSiegeRecallStatics::Begin(double NowSeconds, const FVector& AnchorLocation)
{
	// ⛔ R-4, AND IT IS THE WHOLE RULING: a FRESH state built from the clock handed in. There is
	// no branch here that could preserve a prior start time, no accumulated-progress field to
	// carry, and nothing to "resume" — his words were "start it from the beginning", so the
	// function that starts a channel is structurally incapable of doing anything else.
	FSiegeRecallState Started;
	Started.bChannelling = true;
	Started.StartTimeSeconds = NowSeconds;
	Started.AnchorLocation = AnchorLocation;
	return Started;
}

FSiegeRecallState FSiegeRecallStatics::Cleared()
{
	return FSiegeRecallState();
}

float FSiegeRecallStatics::ElapsedSeconds(const FSiegeRecallState& State, double NowSeconds)
{
	if (!State.bChannelling)
	{
		return 0.f;
	}

	// Max(0) rather than a raw subtraction: a caller that hands back a clock value older than
	// the start (a level-travel clock reset, a test driving time backwards) gets 0 progress
	// rather than a negative elapsed that would read as "complete" through an unsigned compare.
	return FMath::Max(0.f, static_cast<float>(NowSeconds - State.StartTimeSeconds));
}

float FSiegeRecallStatics::Progress01(const FSiegeRecallState& State, double NowSeconds, float ChannelSeconds)
{
	if (!State.bChannelling)
	{
		return 0.f;
	}

	if (ChannelSeconds <= 0.f)
	{
		return 1.f;
	}

	return FMath::Clamp(ElapsedSeconds(State, NowSeconds) / ChannelSeconds, 0.f, 1.f);
}

bool FSiegeRecallStatics::IsComplete(const FSiegeRecallState& State, double NowSeconds, float ChannelSeconds)
{
	// The FULL duration, inclusive at the boundary. At 9.9 s of a 10 s channel this is FALSE —
	// which is exactly what makes "interrupted at 9.9 s gets nothing" a testable claim rather
	// than a hope.
	return State.bChannelling && ElapsedSeconds(State, NowSeconds) >= ChannelSeconds;
}

bool FSiegeRecallStatics::HasLeftAnchor(const FSiegeRecallState& State, const FVector& CurrentLocation, float ToleranceUU)
{
	if (!State.bChannelling)
	{
		return false;
	}

	// ⚠️ FULL 3D distance, deliberately, not the horizontal projection: falling off the ledge
	// the player was standing on IS leaving the spot, and a channel that survived a fall would
	// arrive from somewhere the player never chose. A negative tolerance is clamped rather than
	// trusted — it would otherwise cancel every channel on its first frame.
	const float SafeToleranceUU = FMath::Max(0.f, ToleranceUU);
	return FVector::DistSquared(State.AnchorLocation, CurrentLocation) > (static_cast<double>(SafeToleranceUU) * static_cast<double>(SafeToleranceUU));
}

bool FSiegeRecallStatics::DamageInterrupts(float AppliedDamage)
{
	// ⛔ R-1: damage that LANDED. Strictly greater than zero — a miss, a fully-mitigated hit and
	// a friendly-fire hit all report 0 applied and leave the channel running.
	return AppliedDamage > 0.f;
}

bool FSiegeRecallStatics::IsAttackDisarmed(const FSiegeRecallState& State)
{
	// ⭐⭐ A FUNCTION OF STATE, AND THAT IS THE RULING (R-5). The same hero answers TRUE while a
	// channel runs and FALSE the instant it ends — including on the frame it is interrupted, so
	// a player who is hit can swing back immediately. A permanent class-identity seal cannot
	// express that, which is why one is not used here.
	return State.bChannelling;
}

bool FSiegeRecallStatics::ExitGrantsArrival(ESiegeRecallExit Exit, bool bDestinationOwnerBound)
{
	// ⭐ EXACTLY ONE of the seven exits, AND ONLY WITH A DESTINATION OWNER BOUND.
	//
	// The second term is the atomicity ruling and it is a deliberate design choice, not a
	// defensive habit: the destination is resolved by the owner of the shipped teleport-home
	// rule, so with nobody bound there is no teleport — and a heal without a teleport would be
	// a free full refill from anywhere on the map, granted by a WIRING GAP. ⚖️ A feature that
	// is inert until it is integrated is a visible bug; a feature that half-fires into an
	// exploit is an invisible one.
	return Exit == ESiegeRecallExit::Completed && bDestinationOwnerBound;
}

FSiegeRecallArrival FSiegeRecallStatics::BuildArrival(ESiegeRecallExit Exit, bool bDestinationOwnerBound, float EffectiveMaxHP)
{
	FSiegeRecallArrival Arrival;
	if (ExitGrantsArrival(Exit, bDestinationOwnerBound))
	{
		Arrival.HealTargetHP = EffectiveMaxHP;
		Arrival.bTeleportHome = true;
	}
	return Arrival;
}

void AHeroCharacter::HandleRecallInput()
{
	// The recall key's whole behaviour, in two lines: pressing it while channelling CANCELS
	// (R-2 — his deliberate exit), pressing it otherwise BEGINS. ⛔ There is no third case and
	// ⛔ no other key reaches this feature: Escape is untouchable (RECALL-§3 / AS-§6 A-2), and
	// this class adds no raw key handler of any kind — no key-down override, no preview
	// override, no Slate reply, no viewport intercept (the test asserts all four by name). Every shipped cancel route (placement, spell
	// targeting, group pick) keeps firing byte-identically while a channel runs.
	if (RecallState.bChannelling)
	{
		EndRecall(ESiegeRecallExit::CancelledByInput); // ⛔ RECALL EXIT 2 of 7
		return;
	}

	BeginRecall();
}

bool AHeroCharacter::BeginRecall()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!FSiegeRecallStatics::CanBegin(RecallState, bDead, IsMatchOver()))
	{
		return false;
	}

	// ⛔ R-4 — FROM ZERO, ALWAYS. The state is REPLACED, never amended, so there is no code path
	// by which a previous channel's progress can survive into this one.
	RecallState = FSiegeRecallStatics::Begin(World->GetTimeSeconds(), GetActorLocation());

	// ⚠️ MOVEMENT IS NOT RESTRICTED HERE AND MUST NEVER BE — that is a different rule and the
	// law names the conflation as the mistake to avoid. The hero keeps its full speed; moving
	// simply ENDS the channel (exit 3). Nothing touches the movement component on this path.

	StartRecallChannelEffect();
	OnRecallStateChanged.Broadcast(/*bChannelling=*/ true, RecallChannelSeconds);
	return true;
}

void AHeroCharacter::CancelRecall()
{
	// The blueprint/UI-facing deliberate cancel. Routes to the same exit the key press does, so
	// there is one cancel meaning and not two.
	if (RecallState.bChannelling)
	{
		EndRecall(ESiegeRecallExit::CancelledByInput);
	}
}

void AHeroCharacter::TickRecall(double NowSeconds)
{
	if (!RecallState.bChannelling)
	{
		return;
	}

	// ⛔ RECALL EXIT 6 of 7 — the match ended under the channel. Checked FIRST so a completion
	// and a match end landing on the same frame can never resolve as an arrival: nothing should
	// teleport or heal after the end screen is up, and the shipped match-end rule is inherited
	// rather than a second one invented.
	if (IsMatchOver())
	{
		EndRecall(ESiegeRecallExit::CancelledByMatchEnd);
		return;
	}

	// ⛔ RECALL EXIT 3 of 7 — R-2 movement cancel, checked BEFORE completion so a player who
	// walks away on the final frame does not still arrive.
	if (FSiegeRecallStatics::HasLeftAnchor(RecallState, GetActorLocation(), RecallMoveCancelToleranceUU))
	{
		EndRecall(ESiegeRecallExit::CancelledByMovement);
		return;
	}

	// ⛔ RECALL EXIT 1 of 7 — the only exit that grants anything.
	if (FSiegeRecallStatics::IsComplete(RecallState, NowSeconds, RecallChannelSeconds))
	{
		EndRecall(ESiegeRecallExit::Completed);
	}
}

void AHeroCharacter::EndRecall(ESiegeRecallExit Exit)
{
	// ⭐⭐ THE IDEMPOTENCY LATCH, AND IT IS WHY "EXACTLY ONCE ON EACH OF THE SEVEN EXITS" IS A
	// STRUCTURAL PROPERTY RATHER THAN A PROMISE: every exit routes through this one function,
	// and this one function refuses to run twice for one channel. Lethal damage genuinely does
	// reach exits 4 and 5 in a single call stack; the second one lands here and returns.
	if (!RecallState.bChannelling)
	{
		return;
	}

	// Decide the arrival BEFORE the state is cleared, and take the heal target from the
	// EFFECTIVE maximum — base plus the Plate-Armor bonus the player actually paid for. This is
	// the single source the class header names for every clamp, every regen cap and every full
	// heal, and it is the reason a Plate-Armored hero arrives at 400/400 rather than at 200/400.
	const bool bDestinationOwnerBound = OnHeroRecallArrived.IsBound();
	const FSiegeRecallArrival Arrival = FSiegeRecallStatics::BuildArrival(Exit, bDestinationOwnerBound, GetEffectiveMaxHP());

	// Clear FIRST, and clear COMPLETELY: one struct assignment plus the tell's teardown is the
	// entire undo list for this feature. Doing it before the broadcasts below also makes the
	// hero re-armed and re-startable from inside any listener, rather than leaving a listener
	// looking at a channel that is finished but still says it is running.
	RecallState = FSiegeRecallStatics::Cleared();
	StopRecallChannelEffect();
	OnRecallStateChanged.Broadcast(/*bChannelling=*/ false, RecallChannelSeconds);

	if (Exit == ESiegeRecallExit::Completed && !bDestinationOwnerBound)
	{
		// The integration gap, said out loud exactly once per hero. The destination belongs to
		// the owner of the shipped teleport-home rule (PlayerStart on the hero's own side, else
		// beside its own castle); with nobody bound there is no destination, so the channel ends
		// having done NOTHING rather than handing out a free refill. ⛔ The location is never
		// guessed here and no fallback location is invented — "hero spawns OUTSIDE the keep"
		// (handoffs/TASK-569-buildmaster.md row (n)) is the recorded cost of guessing it.
		if (!bWarnedRecallDestinationUnbound)
		{
			bWarnedRecallDestinationUnbound = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AHeroCharacter '%s': a recall channel completed but nothing is bound to OnHeroRecallArrived, so no destination could be resolved — the hero was NOT moved and NOT healed. The teleport-home owner must bind this delegate (TASK-750)."),
				*GetNameSafe(this));
		}
	}

	if (!Arrival.bTeleportHome)
	{
		// The six non-granting exits end here: the hero stays exactly where it stands, at
		// exactly the hit points it had, with its cooldowns, upgrades, aura and input untouched.
		return;
	}

	// (1 of 2) THE TELEPORT — performed by the destination owner, which resolves the same start
	// transform the respawn path already uses. ⛔ The hero deliberately learns nothing about
	// where that is.
	OnHeroRecallArrived.Broadcast(this);

	// A listener that killed or destroyed the hero during the broadcast must not then be handed
	// a healed corpse. Defensive — no shipped binder does this.
	if (bDead)
	{
		return;
	}

	// (2 of 2) THE HEAL — "they completely refill their health", and completely means the
	// effective maximum. ⛔ These two effects are the ENTIRE arrival: no cooldown reset, no aura
	// re-arm, no upgrade re-application, no input change, no movement-mode change.
	CurrentHP = Arrival.HealTargetHP;
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
}

bool AHeroCharacter::IsMatchOver() const
{
	// ⚠️ GetAuthGameMode() is NULL on a client — see the M8 declaration in the class comment.
	// On a listen-server host and in standalone (every shipped configuration today) this is
	// authoritative.
	const UWorld* World = GetWorld();
	const AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	return GameMode != nullptr && GameMode->HasMatchEnded();
}

void AHeroCharacter::StartRecallChannelEffect()
{
	// R-3's world-space tell. Null-safe by design: no emitter wired means no tell and an
	// otherwise byte-identical channel. Idempotent — any previous component is torn down first
	// so a restart can never leave two running.
	StopRecallChannelEffect();

	if (!RecallChannelEffect)
	{
		return;
	}

	USceneComponent* AttachTo = GetRootComponent();
	if (!AttachTo)
	{
		return;
	}

	// bAutoDestroy = false: this component's lifetime is the CHANNEL's, and the channel's every
	// exit runs StopRecallChannelEffect. Letting the system decide when to die would put a
	// second, independent lifetime next to a state whose entire design goal is having one.
	RecallChannelEffectComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		RecallChannelEffect, AttachTo, NAME_None,
		FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget, /*bAutoDestroy=*/ false);
}

void AHeroCharacter::StopRecallChannelEffect()
{
	if (RecallChannelEffectComponent)
	{
		RecallChannelEffectComponent->DestroyComponent();
		RecallChannelEffectComponent = nullptr;
	}
}

float AHeroCharacter::GetRecallProgress01() const
{
	const UWorld* World = GetWorld();
	return World ? FSiegeRecallStatics::Progress01(RecallState, World->GetTimeSeconds(), RecallChannelSeconds) : 0.f;
}

float AHeroCharacter::GetRecallRemainingSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World || !RecallState.bChannelling)
	{
		return 0.f;
	}

	return FMath::Max(0.f, RecallChannelSeconds - FSiegeRecallStatics::ElapsedSeconds(RecallState, World->GetTimeSeconds()));
}

void AHeroCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ⛔ RECALL EXIT 7 of 7 — the actor is leaving play (level travel, teardown, destroy).
	// Placed inside the recall region on purpose: its entire content is this feature's teardown,
	// so it belongs where the scan can see it. Runs BEFORE Super, while the tell's component is
	// still valid to destroy.
	EndRecall(ESiegeRecallExit::CancelledByEndPlay);

	Super::EndPlay(EndPlayReason);
}

// ═════════════ RECALL REGION END ═════════════
