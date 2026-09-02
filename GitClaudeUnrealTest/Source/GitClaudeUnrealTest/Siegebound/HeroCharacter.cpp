// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/HeroCharacter.h"

#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h" // TASK-790 (VIS-§3): complete type for GetFollowCamera()->SetRelativeLocation — the min-arm floor's ONE write
#include "Camera/CameraShakeBase.h"
#include "CollisionQueryParams.h" // TASK-790: FCollisionQueryParams / SCENE_QUERY_STAT for the blocker-identity sweep (the Projectile.cpp:431 idiom)
#include "CollisionShape.h" // TASK-790: FCollisionShape::MakeSphere — the sweep reproduces SpringArmComponent.cpp:197 exactly
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h" // TASK-512: complete type for GetGameInstance()->GetSubsystem<>() (Actor.h:3772 forward-declares UGameInstance)
#include "Engine/HitResult.h" // TASK-790: FHitResult for the camera blocker-identity sweep
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h" // TASK-778: TActorIterator<AClimbableTower> — the cadence-limited tower scan the contact poll walks (the shipped ABuilding-iteration idiom, SiegeGameMode.cpp:1342 / SummonedUnit.cpp:2474)
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/GameModeBase.h" // TASK-748: HasMatchEnded() — the recall channel's match-end exit
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h" // TASK-790 (VIS-§3): complete type for the INHERITED CameraBoom — ProbeSize, IsCollisionFixApplied, SocketName
#include "GitClaudeUnrealTest.h"
#include "InputAction.h" // TASK-748: complete type for the IA_Recall soft-resolve
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): Team replication registration
#include "NiagaraComponent.h" // TASK-748: complete type for the R-3 channel tell's teardown
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/ClimbableTower.h" // TASK-778 (CONTACT-§4.1): the contact gate the hero POLLS — the tower owns the radius, the cone, the dwell and K-C's latch; this class owns ⛔ none of them
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h" // TASK-512: USiegeKeyboardLayoutSubsystem::GetPositionalContext — the positional remap's ONE call site
#include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channel for the capsule stamp
#include "Siegebound/SiegeNetLimits.h" // M8 (TASK-356 loop-1): the ONE arena relevancy constant (Tier B)
#include "Siegebound/SiegePlayerState.h" // M8 (TASK-356): PossessedBy team resolve (doc §2.3)
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "Siegebound/SiegeSpawnConstants.h" // TASK-778: SiegeSpawn::DefaultCapsuleHalfHeight — used ONLY as a null-capsule fallback (⛔ it is NOT the hero's dimension; its own comment calls it a fallback)
#include "Siegebound/SummonedUnit.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h" // TASK-778: FindFProperty<FFloatProperty> — the ONE shipped ladder rate is READ, ⛔ never copied onto this class

namespace
{
	/** Priority of IMC_Hero on the input subsystem — above any template context (which the template controllers add at 0). */
	constexpr int32 HeroMappingContextPriority = 1;

	//~ §6 hero audio soft-ref paths (TASK-179) — null-safe; the sounds arrive in TASK-180.
	const TCHAR* HeroSwingSoundPath = TEXT("/Game/Audio/S_HeroSwing"); // every swing past cooldown
	const TCHAR* HeroHitSoundPath = TEXT("/Game/Audio/S_HeroHit");     // a swing that damaged >= 1 enemy

	/** Height above the hero origin for its floating damage number. */
	constexpr float HeroDamageNumberHeightZ = 110.f;

	//~ ─── Ladder climb (TASK-778, `CONTACT-§3`) ─────────────────────────────────────────────
	//~ ⛔ NEITHER OF THESE IS A GAMEPLAY TUNABLE AND NEITHER IS AUTHORED: `CONTACT-§7` `K-5` names
	//~ exactly THREE tunables for this feature (radius, cone, dwell) and ALL THREE live on
	//~ `AClimbableTower`. These two are file-scope implementation constants — the
	//~ `FSiegeLadderContactStatics::MinContactSpeedUU` idiom: a mechanism's own cadence, ⛔ not a
	//~ rule anybody tunes. They are `constexpr` here rather than UPROPERTYs precisely so that
	//~ nothing on this class can be mistaken for a second copy of a tower's number.

	/**
	 *  How often the watchdog wakes while a climb runs (`CONTACT-§3.5`).
	 *  ⭐ 0.25 s is the cadence `ASummonedUnit`'s always-on `StateTimerHandle` already runs at, so
	 *  the two drivers are watched at the same resolution — ⛔ not a felt number.
	 *  ⚠️ CONSEQUENCE: raising it lengthens how long a hero whose Tick has stopped hangs in
	 *  `MOVE_Flying` past its budget; lowering it buys nothing, because the budget itself
	 *  (~14.1 s at the shipped line and rate) is what decides when a climb is abandoned.
	 */
	constexpr float LadderClimbWatchdogIntervalSeconds = 0.25f;

	/**
	 *  How often the hero rebuilds its `AClimbableTower` list.
	 *  ⚠️ CONSEQUENCE, and it is the whole reason the number is written down: a tower built while
	 *  the hero is already standing at its ladder is not pollable until the next scan, so this is
	 *  the WORST-CASE delay before a brand-new tower can be walked into. At 1 s that is invisible
	 *  next to the 0.35 s dwell the player must then hold anyway. ⛔ Lowering it toward a per-frame
	 *  `TActorIterator` is the thing to avoid: the scan is O(actors in the level).
	 */
	constexpr double LadderTowerScanIntervalSeconds = 1.0;

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

	// TASK-790 (VIS-§3): push HeroCameraProbeSize onto the INHERITED CameraBoom. Same ctor+BeginPlay
	// re-apply idiom as the two lines above. ⛔ The base class's CameraBoom already EXISTS here —
	// AGitClaudeUnrealTestCharacter's constructor ran to completion before this body started — so
	// GetCameraBoom() is valid, and the value it writes is what the CDO (and therefore every
	// automation-test readback of "survives construction") reports.
	ApplyHeroCameraTuning();

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

	// TASK-790 (VIS-§3): and the camera probe for the same reason — a ctor-only write would bake the
	// C++ default into the CDO and silently ignore a BP_HeroCharacter feel-pass edit of the tunable.
	ApplyHeroCameraTuning();

	// TASK-790: capture the follow camera's AUTHORED relative location ONCE, here, before
	// TickHeroCameraCollision has ever run (Tick cannot precede BeginPlay). It is the zero point
	// the min-arm floor offsets from, so a BP-authored over-the-shoulder framing survives the fix
	// and switching the fix off returns the camera to EXACTLY where the Blueprint put it.
	// ⛔ Not runtime state and ⛔ nothing unwinds it: it is a constant read of a design-time value.
	if (const UCameraComponent* const HeroFollowCamera = GetFollowCamera())
	{
		HeroCameraBaseRelativeLocation = HeroFollowCamera->GetRelativeLocation();
	}

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

	// TASK-778 (CONTACT-§3): drive an in-flight climb, then — only when none is running — ask the
	// towers whether this body is walking into one of their ladders. Both self-guard, so a frame
	// with no climb and no tower costs one bool test and one empty array walk.
	//
	// ⚠️ ORDER IS DELIBERATE: the DRIVER runs BEFORE the poll, so a climb that ends on this frame
	// (arrival, release, match end) cannot be re-entered by the poll in the SAME frame. The
	// tower's own K-C re-arm latch is the rule that keeps it out on later frames; this ordering is
	// what keeps it out on THIS one.
	TickLadderClimb(DeltaSeconds);
	PollLadderContact(DeltaSeconds);

	// TASK-790 (VIS-§3): keep the third-person camera off the pawn's back when the spring arm's
	// collision probe collapses against climbable geometry. ⛔ Placed AFTER the climb pair and
	// deliberately OUTSIDE any climb guard — the collapse is NOT climb-specific (this class had no
	// camera code at all before it, so the defect is the inherited boom meeting tall geometry) and
	// scoping it to a climb would leave the identical blindness on the walk-up that VID-004 caught.
	// Self-guarding and stateless; on a frame with no collision fix it is one bool read.
	TickHeroCameraCollision();

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
	// ⛔ TASK-778 (`CONTACT-§7` K-4) ADDS ITS ONE TERM TO THIS SAME EXISTING GUARD — the
	// TOWER-§9.2 / RECALL-§ R-5 idiom, THIRD application, and ⛔ still no new guard point and ⛔ no
	// new suppression mechanism. A climbing hero cannot attack; it is attackable throughout, and
	// that half gets ⛔ NO code by design (a climber is an ordinary live hero at an ordinary world
	// location, and nothing here narrows anyone's acquisition).
	//
	// ⭐ THE DISARM TERM IS THE SAME BOOL THE DRIVER RUNS ON, so it ends on the exact frame
	// the climb does — ⛔ no decay timer, ⛔ no grace window, ⛔ no lingering penalty. And a
	// refused swing still does not consume the cooldown, so nothing is carried out of the climb.
	//
	// ⚠️ DECLARED FOR QA: `TOWER-§9` was ruled about UNITS. Extending the disarm to the HERO is a
	// PROCEEDING DEFAULT (`CONTACT-§7` K-4), ⛔ not Jonathan's own ruling — flagged in the handoff.
	if (bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState) || IsClimbing())
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

	// ⛔ LADDER EXIT H-4 of 10 (TASK-778, `CONTACT-§3.1`) — the hero died mid-climb.
	//
	// ⚠️⚠️ AND IT IS ⛔ NOT ALREADY COVERED BY THE `DisableMovement()` A FEW LINES BELOW, WHICH IS
	// EXACTLY THE TRAP: that call meets the MOVEMENT-MODE half of the hazard and ⛔ nothing else.
	// The climb STATE would stay armed, the driver would keep steering a dead, hidden body up a
	// ladder line every frame, the watchdog timer would keep firing on the world, and `IsClimbing()`
	// would keep the melee guard disarmed into the respawn. ⇒ the abort goes HERE, beside the
	// recall's own death exit, and it runs BEFORE `DisableMovement()` so the final resting state of
	// a corpse is `MOVE_None` (this restores the mode; the line below then disables it).
	// ⭐ Idempotent with H-5: a death that also un-possesses reaches both, and the teardown's
	// exactly-once latch makes the second one inert.
	AbortLadderClimb();

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

	// ⛔ LADDER EXIT H-6 of 10 (TASK-778) — THE BACKSTOP, AND IT IS DECLARED AS ONE.
	// ⚠️ This function already writes `MOVE_Walking` below, so the MODE half would self-heal here
	// — ⛔ but leaning on that would be wrong twice over: it is ~180 s downstream of the death
	// (`GHOST-§0`'s respawn delay), so a hero rescued only here has been broken for three minutes;
	// and it would leave the state, the driver and the watchdog running for that whole time.
	// ⭐ H-4 and H-5 are the real exits. This one is belt, and it costs one bool test.
	// ⚠️ BEFORE the respawn's own deliberate walking-mode write below, so the restore this abort
	// performs is then overwritten by it — ⛔ never the other way round.
	AbortLadderClimb();

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

void AHeroCharacter::ApplyHeroCameraTuning()
{
	// TASK-790 (VIS-§3): the CONFIGURATION half of the V2 fix — one field on the INHERITED boom.
	// ⛔ bDoCollisionTest is deliberately LEFT ALONE (true): turning the probe off would let the
	// camera sit inside every wall in the map, and that shape was refused outright. ⛔ ProbeChannel
	// is likewise left at ECC_Camera — retargeting it would need a new collision channel in
	// DefaultEngine.ini, which is outside this task's fence and would change what EVERY camera in
	// the project collides with.
	if (USpringArmComponent* const Boom = GetCameraBoom())
	{
		Boom->ProbeSize = HeroCameraProbeSize;
	}
}

float AHeroCharacter::ComputeCameraPushOutLocalX(float FixedArmLengthUU, float NaturalArmLengthUU, float MinArmLengthUU)
{
	// ⭐ THE WHOLE DECISION, AND IT IS FOUR LINES OF ARITHMETIC WITH NO ENGINE IN IT (TASK-790).
	//
	// ⛔ TWO CEILINGS, AND BOTH ARE LOAD-BEARING RATHER THAN DEFENSIVE:
	//   • MinArmLengthUU <= 0 disables the floor by NUMBER (the second off-switch), and a
	//     non-positive natural arm means there is no arm to reason about at all.
	//   • The floor is CLAMPED TO THE NATURAL ARM. Without that clamp, a feel pass that set
	//     MinCameraArmLengthUU above the boom's TargetArmLength would push the camera FURTHER
	//     from the hero than open ground ever does — a fix that breaks the uncollided case.
	if (MinArmLengthUU <= 0.f || NaturalArmLengthUU <= 0.f)
	{
		return 0.f;
	}

	const float Floor = FMath::Min(NaturalArmLengthUU, MinArmLengthUU);

	// ⭐ The ordinary frame returns EXACTLY zero: the arm is already at or beyond the floor, so the
	// engine's answer is passed through untouched and the camera carries no offset at all.
	if (FixedArmLengthUU >= Floor)
	{
		return 0.f;
	}

	// Negative: socket +X points AT the pawn (SpringArmComponent.cpp:185 places the camera at
	// ArmOrigin - DesiredRot.Vector() * Len), so away-from-the-pawn is -X.
	return -(Floor - FixedArmLengthUU);
}

void AHeroCharacter::TickHeroCameraCollision()
{
	USpringArmComponent* const Boom = GetCameraBoom();
	UCameraComponent* const Camera = GetFollowCamera();
	if (!Boom || !Camera)
	{
		return;
	}

	// ⭐⭐ ONE LOCAL, ONE WRITE, ONE EXIT — AND THAT SHAPE IS THE ANSWER TO "WHAT UNWINDS THIS?".
	// The offset is a LOCAL that starts at zero on EVERY frame and is written to the camera on
	// EVERY frame, so the "not pushing" state is re-established unconditionally the moment any
	// condition below stops holding. ⇒ ⛔ there is ⛔ NOTHING for a climb exit to revert, ⛔ no
	// entry owed in the ten-exit teardown, and ⛔ no way to leave the camera stuck pushed. That is
	// deliberate: a camera left permanently ignoring the tower is exactly the class of bug this
	// wave has been fighting, and the cure is to have no persistent decision at all.
	float PushOutX = 0.f;
	const UWorld* const World = GetWorld();

	// ⛔ THE GATES, CHEAPEST FIRST. `bIgnoreClimbableGeometryForCamera == false` is the OFF-SWITCH
	// and short-circuits everything ⇒ today's exact behaviour, restorable mid-playtest without a
	// build. `IsCollisionFixApplied()` is the boom's own report that its probe displaced the
	// camera this frame; in open ground it is false and this whole function is one bool read.
	// ⚠️ THE BOOM TICKS IN TG_PostPhysics (SpringArmComponent.cpp:22) and this actor ticks in
	// TG_PrePhysics, so the reads below are LAST frame's resolved arm. That one-frame latency is
	// accepted and STATED rather than hidden: the defect it answers lasted ≈2 s, and the offset is
	// re-based onto the CURRENT socket transform by the boom's own update either way.
	if (World && bIgnoreClimbableGeometryForCamera && Boom->bDoCollisionTest && Boom->IsCollisionFixApplied())
	{
		// The two ends of the boom's own sweep, READ FROM THE COMPONENT rather than re-derived, so
		// this cannot drift from SpringArmComponent.cpp:190-197 if TargetOffset/SocketOffset are
		// ever used. GetSocketLocation is where the probe actually left the camera.
		const FVector ArmOrigin = Boom->GetComponentLocation() + Boom->TargetOffset;
		const FVector UnfixedLoc = Boom->GetUnfixedCameraPosition();
		const FVector FixedLoc = Boom->GetSocketLocation(USpringArmComponent::SocketName);

		const float NaturalArm = static_cast<float>(FVector::Dist(ArmOrigin, UnfixedLoc));
		const float FixedArm = static_cast<float>(FVector::Dist(ArmOrigin, FixedLoc));

		// Is the collapse even bad enough to matter? A wall that trims 400 to 260 is the spring arm
		// doing its job and is left alone. Computed through the PURE function so the suite asserts
		// the same arithmetic the runtime uses — ⛔ never a second copy of it.
		const float Candidate = ComputeCameraPushOutLocalX(FixedArm, NaturalArm, MinCameraArmLengthUU);
		if (!FMath::IsNearlyZero(Candidate))
		{
			// ⛔ AND ONLY NOW DO WE PAY FOR A QUERY. The boom does not record WHAT it hit, so the
			// only way to answer "was it climbable geometry?" is to reproduce its sweep: same
			// origin, same endpoint, same sphere, same channel, same single ignored actor
			// (SpringArmComponent.cpp:194 ignores GetOwner() and nothing else). ⭐ Reaching this
			// line requires a LIVE collision fix that is ALSO shorter than the floor — the state
			// VID-004 caught — so in ordinary play this runs zero times per frame, which is the
			// reason it is allowed to live in Tick at all.
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HeroCameraProbe), /*bTraceComplex=*/ false, this);
			FHitResult Hit;
			World->SweepSingleByChannel(Hit, ArmOrigin, UnfixedLoc, FQuat::Identity, Boom->ProbeChannel,
				FCollisionShape::MakeSphere(Boom->ProbeSize), QueryParams);

			// ⛔ THE NARROWNESS IS THE WHOLE RULING: only an AClimbableTower buys the floor. A rock,
			// a castle wall or a crate collapses the arm EXACTLY as it does today, so this change
			// cannot regress a camera situation that was not the reported defect. A miss (possible
			// on the one-frame-stale read, or if the blocker moved) also leaves PushOutX at zero.
			if (Hit.bBlockingHit && Hit.GetActor() && Hit.GetActor()->IsA(AClimbableTower::StaticClass()))
			{
				PushOutX = Candidate;
			}
		}
	}

	// ⛔ OFFSET FROM THE AUTHORED LOCATION, ⛔ never from an assumed zero: BP_HeroCharacter may have
	// moved the follow camera off the socket (over-the-shoulder framing), and clobbering that would
	// be a silent art regression. HeroCameraBaseRelativeLocation is captured ONCE at BeginPlay,
	// before this function has ever written, so PushOutX == 0 restores the authored value EXACTLY.
	Camera->SetRelativeLocation(HeroCameraBaseRelativeLocation + FVector(PushOutX, 0.f, 0.f));
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

	// ⛔ LADDER EXIT H-7 of 10 (TASK-778, `CONTACT-§3.4` bullet 2) — A CLIMB MAY ⛔ NOT SURVIVE A
	// RECALL TELEPORT, and this is the one recall end that teleports.
	//
	// ⚠️ IT MUST RUN ***BEFORE*** THE BROADCAST BELOW, not after: the destination owner MOVES this
	// actor inside that call, and a live `MOVE_Flying` drive would then keep interpolating toward a
	// ladder 500 m away — dragging the hero back out of its own keep along a line whose endpoints
	// are now nonsense. Ending first means the body arrives home walking.
	//
	// ⭐⭐ NEAR-UNREACHABLE, AND ⛔ KEPT ANYWAY. `CONTACT-§3.4` MEASURED the third direction:
	// the recall's own cancel reads ⛔ neither input ⛔ nor velocity — it reads POSITION
	// (`HasLeftAnchor`, a full 3D `DistSquared` vs `RecallMoveCancelToleranceUU` = 25 uu, ticked
	// unconditionally), so a scripted climb leaves the anchor ball within ~0.07 s at 350 uu/s and
	// CANCELS the channel long before it can complete. ⇒ reaching this line needs a completion and
	// a climb-start on the same frame, ahead of the cancel. ⚖️ An exit you cannot reach is free;
	// an exit you removed is a hang.
	AbortLadderClimb();

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

	// ⛔ LADDER EXIT H-9 of 10 (TASK-778) — this actor is leaving play (level travel, teardown,
	// destroy). ⚠️ BEFORE Super, while the movement component and the world timer manager are both
	// still valid: the teardown restores the mode and clears the watchdog, and a timer left armed
	// on a destroyed actor is the dangling-handle class this class already clears elsewhere.
	AbortLadderClimb();

	Super::EndPlay(EndPlayReason);
}

// ═════════════ RECALL REGION END ═════════════

// ═════════════ LADDER CLIMB REGION BEGIN ═════════════
//
//  ⭐⭐ THE HERO'S LADDER CLIMB (TASK-778; CONVENTIONS `CONTACT-§3`, `TOWER-§8.4(B)`/`§8.5`/`§8.5a`)
//
//  Jonathan, verbatim (2026-09-01): "lets just make sure the playable character and any units can
//  climb the ladder by simplying walking up to it and walking against it."
//
//  ⛔⛔ WHAT THIS REGION IS ***NOT***, SO THE SHAPE IS NOT MISREAD:
//    • ⛔ It authors ⛔ NO rules. Every decision — admission, the endpoint lift, the deck-breach
//      window, arrival, the budget, the exactly-once latch — is `FSiegeLadderClimbStatics`,
//      CONSUMED UNCHANGED (`CONTACT-§2`'s ruling: a statics LIFT plus a PER-CLASS DRIVER, ⛔ not a
//      component and ⛔ not a shared base class, because the EXITS are the risk surface and a
//      component cannot intercept `HandleDeath`, `UnPossessed`, `EndRecall` or `EndPlay`).
//    • ⛔ It owns ⛔ NO contact tunable. The radius, the intent cone, the dwell and `K-C`'s re-arm
//      latch are `AClimbableTower`'s (`WR-§5`) and this class asks a question rather than
//      answering one.
//    • ⛔ It adds ⛔ NO key, ⛔ no binding, ⛔ no prompt and ⛔ no cancel-key handler of any kind
//      (`K-A`: it is AUTOMATIC — *"simply by walking against it"* — and `AS-§6` A-2 is
//      untouchable). ⚠️ The suite asserts all four of those absences over this region by name,
//      which is why they are described here rather than quoted.
//    • ⛔ It adds ⛔ NO animation (`CONTACT-§5` — Jonathan's explicit waiver) and ⛔ REMOVES none:
//      the units' shipped climb clip stays imported, committed and wired to their ABP, and UNITS
//      KEEP IT. ⭐ The waiver is a licence ⛔ not to build; it is ⛔ not an instruction to remove.
//
//  ⚠️⚠️ AND THE NUMBERS ARE ***RE-DERIVED FROM THIS HERO'S OWN CAPSULE***, ⛔ NEVER INHERITED FROM
//  THE UNIT (`CONTACT-§3.3`). They differ, and the difference is the whole reason the law demands
//  it: hero capsule r 42 / hh 96 (`GitClaudeUnrealTestCharacter.cpp:18`) vs the unit's 34 / 88.
//    · endpoint lift        96.0 uu   (⛔ never 88, ⛔ never `SiegeSpawn::DefaultCapsuleHalfHeight`,
//                                      which its own comment calls a FALLBACK)
//    · deck-breach ceiling  3 × 96 = 288 uu of Z ⇒ 296.86 uu of line = 24.00% of the 1,236.9 uu
//                                      ascent (the unit's row is 264 ⇒ 272.13 ⇒ 22.00%)
//    · watchdog budget      4 × 1,236.9 / 350 = 14.14 s against a 3.53 s ascent
//  ⛔ ⛔ NONE of these is typed as a literal anywhere below: the half-height is read from the
//  capsule and everything else is derived from it inside the shipped statics.
//
// ═════════════════════════════════════════════════════════════════════════════════════════════

bool AHeroCharacter::IsClimbing() const
{
	// ONE bool backs the climb AND the K-4 disarm — see the header. There is no second flag
	// anybody could forget to clear and no timer that could outlive the cause.
	return LadderClimb.bActive;
}

bool AHeroCharacter::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)
{
	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	if (!Movement)
	{
		// ⛔ Refuse rather than arm: with no movement component there is no MOVE_Flying to enter
		// and no mode to restore, so an "active" climb would be a hero that never moves and never
		// ends. Unreachable for a spawned ACharacter; guarded because the alternative is the hang.
		return false;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		// ⛔ NO WATCHDOG, NO CLIMB. The 0.25 s belt is an FTimerManager entry and the timer manager
		// lives on the WORLD — so a world-less hero could be armed into MOVE_Flying with nothing
		// able to abandon it. Refusing here is the only honest answer.
		return false;
	}

	// ⛔ THE FIFTH REFUSAL REASON, HERO-ONLY AND DECLARED IN THE HEADER (`CONTACT-§3.4` bullet 1):
	// a hero may ⛔ NOT START a climb while a recall channel is running. ⚠️ This is ⛔ NOT provided
	// by the measured position-cancel — that rule ends a CHANNEL when a climb moves the body; it
	// says nothing about starting one, and a climb that began mid-channel would be relying on a
	// cancel landing first to be correct. ⭐ Checked BEFORE `Begin`, so a refusal still changes
	// NOTHING.
	if (IsRecalling())
	{
		return false;
	}

	// ⛔ THE ONE SHIPPED RATE, READ — ⛔ never a second copy on this class (`CONTACT-§8`).
	float ClimbSpeedUU = 0.f;
	if (!TryResolveLadderClimbSpeedUU(ClimbSpeedUU))
	{
		return false;
	}

	// ⭐⭐ THE CAPSULE HALF-HEIGHT COMES FROM ***THIS HERO'S*** CAPSULE, ⛔ NEVER FROM A LITERAL AND
	// ⛔ NEVER FROM THE UNIT'S 88 (`TOWER-§8.5a` clause 6 / `CONTACT-§3.3` #1 — `TOWER-§7`'s
	// "geometry comes FROM the thing" applied again). It is what lifts the SURFACE-space sockets
	// into CAPSULE-CENTRE space so the hero finishes STANDING ON the deck instead of buried one
	// half-height inside the slab. ⭐ A BP child that resizes its capsule stays correct for free.
	// ⚠️ `SiegeSpawn::DefaultCapsuleHalfHeight` appears ONLY as the null-capsule fallback — its own
	// comment calls it a fallback, and this diagnosis proved it is 8 uu wrong for this pawn.
	const float CapsuleHalfHeightUU = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: SiegeSpawn::DefaultCapsuleHalfHeight;

	// ⛔ "CHANGES NOTHING ON REFUSAL" is enforced inside the pure half, in ONE place, so it cannot
	// be half-kept. ⚠️⚠️ THE TWO FROZEN TERMS ARE ***MAPPED***, ⛔ NEVER PASSED THROUGH
	// (`CONTACT-§3.2` — typing `false` is ⛔ not neutral: it asserts no such freeze can exist for
	// this pawn, and a pawn entering MOVE_Flying mid-freeze is the exact hang this machinery
	// prevents):
	//
	//   bAIFrozen    ⇒ `IsMatchOver()` (`HeroCharacter.cpp`, this file). ⭐ A REAL mapping, ⛔ not
	//                  a stand-in: `ASiegeGameMode::FreezeWorldAtMatchEnd` (`SiegeGameMode.cpp:616`)
	//                  is what sets `bAIFrozen` on units (`:632`), and the hero's shipped
	//                  equivalent of that same event is this predicate — the one the recall
	//                  channel's own match-end exit already reads. ⛔ A hero that could start a
	//                  climb during the end screen is `GHOST-§4`'s input-dead catastrophe with a
	//                  new cause.
	//   bSpellFrozen ⇒ ⛔ NOTHING CORRESPONDS, AND THAT IS ***MEASURED***, ⛔ not assumed. Frost is
	//                  the only freeze in the game and it applies `ApplyFreeze` to `ASummonedUnit`
	//                  and `ABuilding` ONLY — `SpellLibrary.cpp:304-320` ends with the shipped
	//                  comment *"every other ITeamAgent (castle, hero) is excluded by ruling 5 — no
	//                  branch on purpose"*, and `SpellLineSweep.cpp:240-249` has the same two
	//                  branches. Grepped: `HeroCharacter.{h,cpp}` contain ZERO occurrences of
	//                  Freeze / Frozen / Stun. ⇒ `false` here is a REPORTED FINDING with its
	//                  evidence, ⛔ not a value typed in to make the call compile. ⚠️ The day a
	//                  spell CAN freeze the hero, this argument acquires that state — and the
	//                  handoff says so.
	if (!FSiegeLadderClimbStatics::Begin(LadderClimb, bDead, /*bAIFrozen=*/ IsMatchOver(),
		/*bSpellFrozen=*/ false, FromWorld, ToWorld, ClimbSpeedUU, CapsuleHalfHeightUU))
	{
		return false;
	}

	// ══ FROM HERE ON IsClimbing() IS TRUE — THE K-4 DISARM IS LIVE ═══════════════════════════
	// ⭐ The hero is ATTACKABLE throughout and that half gets ⛔ NO code, deliberately: a climber is
	// an ordinary live hero at an ordinary world location, and nothing in this region narrows any
	// acquisition gate anywhere.

	LadderClimbResolvedSpeedUU = ClimbSpeedUU;

	// ⭐ MOVE_Flying — the ONE shipped mode that accepts vertical motion without new physics code,
	// and the licence is MEASURED (`TOWER-§8.5`): `ConstrainInputAcceleration` plane-projects
	// steering input ONLY when `IsMovingOnGround() || IsFalling()`, and flying is neither, so the
	// vertical component of our steer SURVIVES. Flying also disables gravity, which is what a climb
	// wants. MaxFlySpeed caps the ascent at the shipped rate; it is saved and restored EXACTLY.
	LadderClimbSavedMaxFlySpeed = Movement->MaxFlySpeed;
	Movement->MaxFlySpeed = FMath::Max(ClimbSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU);
	Movement->StopMovementImmediately(); // ⛔ no inherited walk/sprint velocity carried into the ascent
	Movement->SetMovementMode(MOVE_Flying);

	// ⭐ SEED THE HOLD-TO-CLIMB READ, or exit H-3 would fire on frame one and the climb would be
	// unstartable. This is honest rather than convenient: the tower admitted this body only after
	// the INTENT term held CONTINUOUSLY for the full dwell, so "the player is steering into the
	// ladder" is a fact that was just measured — the seed carries it across the one frame before
	// the next `DoMove` arrives, and nothing sustains it after that except real input.
	LadderClimbSteerWorld = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);
	LadderClimbSteerWorld.Z = 0.f;
	LadderClimbSteerFrame = GFrameCounter;

	// ⭐⭐ THE WATCHDOG THIS CLASS DOES NOT OTHERWISE HAVE (`CONTACT-§3.5`) — armed HERE and
	// cleared by every one of the ten exits. The deadline is the climb's OWN budget expressed on
	// the WORLD clock, so a Tick that stops (a disabled actor tick, a paused component, a driver
	// nobody re-asserts) cannot postpone it: an FTimerManager entry lives on the world and ⛔ cannot
	// be killed by an actor tick-flag write.
	LadderClimbWatchdogDeadlineSeconds = World->GetTimeSeconds() + static_cast<double>(LadderClimb.TimeoutSeconds);
	GetWorldTimerManager().SetTimer(LadderClimbWatchdogTimerHandle, this,
		&AHeroCharacter::OnLadderClimbWatchdog, LadderClimbWatchdogIntervalSeconds, /*bLoop=*/ true);

	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("AHeroCharacter '%s': ladder climb STARTED — %.1f uu of line, budget %.2f s, capsule half-height %.1f (read from the capsule), deck-breach window %.1f uu."),
		*GetNameSafe(this), LadderClimb.LengthUU, LadderClimb.TimeoutSeconds, LadderClimb.CapsuleHalfHeightUU, LadderClimb.DeckBreachUU);

	return true;
}

void AHeroCharacter::AbortLadderClimb()
{
	// ⛔ LADDER EXIT H-2 of 10 — and H-10 as well: `AClimbableTower::EndPlay` reaches this exact
	// function through `ILadderClimber` when the tower dies under a climber it started. Under the
	// old ramp that case was FREE (the floor vanished and CharacterMovement dropped the occupant to
	// MOVE_Falling by itself); a MOVE_Flying hero will ⛔ NOT fall, so the abort is REQUIRED there.
	// ⛔ TAKES NO REASON, AND THAT IS PINNED (`TOWER-§8.4(B)`): the teardown is REASON-AGNOSTIC.
	// Idempotent — the teardown's latch makes a call on a non-climbing hero a silent no-op.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeHeroLadderExit::Abort);
}

void AHeroCharacter::EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason)
{
	// ⭐⭐ THE EXACTLY-ONCE LATCH IS CONSUMED FIRST, BEFORE ANY EFFECT. That ordering is what makes
	// a double exit inert — death then EndPlay is the ORDINARY case, ⛔ not an edge one — and what
	// makes a re-entrant call from anything this function touches return immediately.
	if (!FSiegeLadderClimbStatics::End(LadderClimb))
	{
		return;
	}

	// ⛔ THE WATCHDOG DIES WITH THE CLIMB, ON EVERY EXIT. A looping timer that outlived its climb
	// would fire on a walking hero forever — and would eventually abort a climb it did not start.
	GetWorldTimerManager().ClearTimer(LadderClimbWatchdogTimerHandle);
	LadderClimbWatchdogDeadlineSeconds = 0.0;

	// ⛔⛔ THE RESTORE. THIS IS THE LINE THE WHOLE FEATURE TURNS ON: MOVE_Flying ignores gravity, so
	// an exit that skips it leaves the PLAYER'S OWN BODY hanging in mid-air forever.
	// `SetDefaultMovementMode` is this project's shipped idiom for exactly this
	// (`ASummonedUnit::EndLadderClimb`, `EndSpellFreeze`), and it does the right thing in mid-air:
	// with no movement base it goes straight to MOVE_Falling rather than spending a frame
	// pretending to walk. ⇒ the hero DROPS from wherever it is and survives — there is ⛔ no fall
	// damage anywhere in Siegebound (`TOWER-§4a`, measured).
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->MaxFlySpeed = LadderClimbSavedMaxFlySpeed; // EXACT restore, zero residual
		Movement->SetDefaultMovementMode();
	}

	// WHOLE-STATE reset, never a bare flag clear: a stale steer or a stale saved speed left behind
	// is exactly the residue the TASK-020 zero-drift contract forbids. (`LadderClimb` itself was
	// whole-struct reset inside `End` above.)
	LadderClimbSavedMaxFlySpeed = 0.f;
	LadderClimbResolvedSpeedUU = 0.f;
	LadderClimbSteerWorld = FVector::ZeroVector;
	LadderClimbSteerFrame = 0;

	if (Reason == ESiegeHeroLadderExit::Watchdog)
	{
		// ⚠️ THE WATCHDOG IS THE ONE EXIT THAT MEANS SOMETHING IS WRONG — every other reason is
		// ordinary gameplay, so this is the only one that logs at Warning (`TOWER-§8.5a` clause 7:
		// a stall before the window opens must fail LOUDLY). The hero has just been DROPPED rather
		// than stranded, so this is a diagnosis rather than a failure.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AHeroCharacter '%s': ladder climb ABANDONED by the watchdog — it did not reach the top inside its budget. The hero has been DROPPED (movement mode restored), ⛔ not stranded and ⛔ not handed the deck it failed to reach. Check for geometry blocking the climb line, or a LadderTop the capsule cannot reach."),
			*GetNameSafe(this));
	}

	// ⭐ WHICH OF THE TEN ENDED IT, AND WHETHER IT ARRIVED — at Verbose, because every reason except
	// the watchdog above is ordinary gameplay. ⚠️ This is ⛔ not decoration: the ten exits cannot be
	// driven headlessly (a world-less hero crashes in the movement component), so TASK-780's PIE
	// session has ⛔ no other way to see WHICH exit fired. ⭐ The shipped `StaticEnum` +
	// null-guard idiom (`SiegeAssistantCommand.cpp:338`), so a stripped enum degrades to a number
	// rather than crashing a log line.
	const UEnum* const ExitEnum = StaticEnum<ESiegeHeroLadderExit>();
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("AHeroCharacter '%s': ladder climb ENDED — reason %s, reached the top: %s. Movement mode restored, state cleared, watchdog killed."),
		*GetNameSafe(this),
		ExitEnum ? *ExitEnum->GetNameStringByValue(static_cast<int64>(Reason)) : *FString::FromInt(static_cast<int32>(Reason)),
		bReachedTop ? TEXT("YES") : TEXT("no"));

	// ⭐⭐ THE COMPLETION SIGNAL — **LAST, AND EXACTLY ONCE** (TASK-787, `CONTACT-§12.3`).
	//
	// ⚖️ THIS COMMENT REPLACES TASK-778's DECLARED BLOCKER RATHER THAN DELETING IT, BECAUSE THE
	// BLOCKER IS WHAT BOUGHT THE FIX. It read: *"⛔ NO COMPLETION DELEGATE IS BROADCAST, AND THAT IS
	// DECLARED RATHER THAN FORGOTTEN … the tower's completion signal is
	// `ASummonedUnit::OnLadderClimbEnded`, which this class does ⛔ not have."* That was TRUE when
	// written and it was the honest half of a refusal to self-start; `CONTACT-§12` ratified the
	// refusal as LAW and closed the seam properly — the delegate moved to `LadderClimber.h`, the
	// interface gained an accessor, and this class gained the INSTANCE.
	//
	// ⛔⛔ WHY IT IS HERE AND ⛔ NOWHERE ELSE, AND WHY THE ORDER IS THE WHOLE POINT: the tower binds
	// this delegate BEFORE it calls `BeginLadderClimb` and releases the occupancy slot when it
	// fires. ⇒ a hero that never broadcast would hold that slot FOREVER and BRICK the ladder for
	// every later climber, hero or unit (`CONTACT-§12.1`).
	//   • LAST — after the movement mode is restored and after every field is cleared, so a listener
	//     that inspects this hero never sees it mid-teardown. The unit's own pinned ordering.
	//   • AFTER THE LATCH — `FSiegeLadderClimbStatics::End` above already consumed it and returned,
	//     so a listener that re-enters `AbortLadderClimb()` from inside this broadcast is INERT
	//     rather than recursive. ⭐ Exactly-once is INHERITED from the ten-exits-one-teardown
	//     property, ⛔ NOT re-latched here: there is deliberately ⛔ no second flag.
	// ⭐ `this` converts implicitly to the delegate's `ACharacter*` — the `CONTACT-§4.4` widening,
	// cashed in. ⛔ ONE delegate TYPE serves both pawns; a hero-only one is refused (`§12.4`).
	OnLadderClimbEnded.Broadcast(this, bReachedTop);
}

void AHeroCharacter::TickLadderClimb(float DeltaSeconds)
{
	if (!LadderClimb.bActive)
	{
		return;
	}

	// ⛔ LADDER EXIT H-8 of 10 — the match ended under the climb. ⭐ Checked FIRST, exactly as the
	// recall channel checks its own match-end exit first: nothing should still be driving a body up
	// a tower under the end screen, and this inherits the shipped rule rather than inventing a
	// second one. (`bAIFrozen`'s hero mapping, `CONTACT-§3.2`.)
	if (IsMatchOver())
	{
		EndLadderClimb(/*bReachedTop=*/ false, ESiegeHeroLadderExit::MatchEnd);
		return;
	}

	// ⛔ LADDER EXIT H-3 of 10 — `K-B` HOLD-TO-CLIMB, and it is the hero's MOST FREQUENT exit, so
	// it is checked before anything can move the body this frame. ⭐ The verb that STARTS the climb
	// is the verb that SUSTAINS it, which is what makes the rule self-documenting; releasing (or
	// steering away) ends it and the hero DROPS — free, because there is no fall damage.
	if (!IsLadderClimbInputHeld())
	{
		EndLadderClimb(/*bReachedTop=*/ false, ESiegeHeroLadderExit::InputReleased);
		return;
	}

	const FVector Here = GetActorLocation();

	bool bReachedTop = false;
	bool bTimedOut = false;
	if (!FSiegeLadderClimbStatics::Advance(LadderClimb, Here, DeltaSeconds, bReachedTop, bTimedOut))
	{
		if (bReachedTop)
		{
			// ⭐ LAND EXACTLY ON THE DECK, ⛔ NOT WHEREVER THIS FRAME'S STEP HAPPENED TO STOP.
			// `ArrivalTarget` is the destination surface plus one capsule half-height — THIS hero's
			// half-height — so it finishes STANDING ON the deck. Non-swept for the same reason the
			// last stretch was: the target is on the far side of the slab.
			// ⛔⛔ ONLY ON A REAL ARRIVAL (`TOWER-§8.5a` clause 5): a timed-out climb drops from
			// where it actually is and is ⛔ NEVER handed the deck it failed to reach. ⚖️ A watchdog
			// that teleports its casualty to the destination is not a watchdog.
			SetActorLocation(FSiegeLadderClimbStatics::ArrivalTarget(LadderClimb), /*bSweep=*/ false);
		}

		// ⛔ LADDER EXIT H-1 of 10 (arrival) — or the declared watchdog reached through the state's
		// own budget. Arrival is the ONLY exit that reports bReachedTop, and it is also where the
		// K-4 disarm releases: the same `End()` that clears bActive clears `IsClimbing()`, so the
		// hero can swing again on this very frame. ⛔ No decay timer, ⛔ no grace window.
		EndLadderClimb(bReachedTop, bReachedTop ? ESiegeHeroLadderExit::Arrival : ESiegeHeroLadderExit::Watchdog);
		return;
	}

	const FVector Direction = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);

	if (FSiegeLadderClimbStatics::ShouldSweep(LadderClimb, Here))
	{
		// ── THE ORDINARY ~76% OF THE LINE: SWEPT MOVEMENT THROUGH THE MOVEMENT COMPONENT ───────
		// The capsule, the sweep and depenetration all still apply. ⛔ NOT a SetActorLocation lerp
		// of the traversal: that would drag the capsule through the tower body and through other
		// bodies, and it is the mechanism `NAV-§` refuses on principle (`TOWER-§8.5`).
		// ⚠️ bForce = true, for the same measured reason the unit's driver passes it:
		// `Internal_AddMovementInput` DROPS the vector whenever `IsMoveInputIgnored()`, and a
		// scripted traversal whose completion the tower is waiting on must ⛔ not be silently
		// suppressible — the body would float until the watchdog dropped it.
		AddMovementInput(Direction, 1.f, /*bForce=*/ true);
		return;
	}

	// ── ⚠️⚠️ THE DECK-BREACH WINDOW — A NON-SWEPT ***CONTINUOUS DRIVE***, AND IT IS THE ONLY WAY
	//    THE FEATURE REACHES THE DECK AT ALL (`TOWER-§8.5a`, granted on TASK-737's measurement:
	//    `LadderTop` is pinned 150 uu INSIDE a solid deck slab, so a swept move stalls ~131 uu
	//    BELOW it — silently, with every exit still perfectly correct) ─────────────────────────
	//
	// ⛔⛔ ITS LICENCE IS `TOWER-§8.3`'s ≥56 uu STANDOFF AND ⛔ NOTHING ELSE, AND FOR THIS CAPSULE
	// THAT LICENCE WAS ***VOID***: r 42 leaves 51.624 uu, 4.376 SHORT (`CONTACT-§3.3` #4). ⚖️ That
	// is `K-1`, and Jonathan ruled ⭐ OPTION A — the LADDER MOVES OUTWARD (TASK-783), which restores
	// the standoff for the hero without touching this code.
	// ⇒ ⛔⛔ NOTHING BELOW HARDCODES A SOCKET, AN ENDPOINT, A DISTANCE OR THE SIZE OF THAT MOVE, and
	// ⛔ this comment deliberately quotes ⛔ no figure for it either: the first estimate was ~4.6 uu
	// and the art lane is building a different one, so a number written here would be `CONTACT-§10.1`
	// cite-rot on delivery. Both world points arrive as PARAMETERS, read at runtime from the link the
	// tower armed from the mesh's own sockets ⇒ the translation is INVISIBLE here, whatever its size,
	// which is exactly why option A costs this file zero lines.
	//
	// ⭐ SAME RATE, SAME LINE, JUST NO SWEEP: a CONTINUOUS drive, ⛔ not a teleport and ⛔ not a lerp
	// of the traversal. ⚠️ Velocity is zeroed first or the two drivers fight: `PhysFlying` would
	// keep sweeping the capsule from residual velocity (`BrakingDecelerationFlying` is 0, so it
	// never decays) and re-jam it against the slab being stepped through.
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	const float StepUU = FMath::Max(LadderClimbResolvedSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU)
		* FMath::Max(DeltaSeconds, 0.f);
	SetActorLocation(Here + Direction * StepUU, /*bSweep=*/ false);
}

bool AHeroCharacter::IsLadderClimbInputHeld() const
{
	// ── (1) IS THERE A STEER AT ALL THIS FRAME? ───────────────────────────────────────────────
	// ⚠️ ENHANCED INPUT NEVER DELIVERS A "RELEASED" CALL FOR AN AXIS: on release, `DoMove` simply
	// STOPS ARRIVING. So the release is detected by AGE, and the age is measured in FRAMES rather
	// than seconds deliberately — ⛔ a seconds threshold would be a fourth tunable nobody ruled,
	// and this needs none.
	// ⭐ ONE frame of tolerance, and it is load-bearing: the player controller's input processing
	// and this pawn's Tick are both in TG_PrePhysics and their relative order is ⛔ NOT guaranteed,
	// so a strict same-frame test would end climbs at random on whichever ordering ran the pawn
	// first. Two frames (~33 ms at 60 Hz) is the worst-case lag before a real release is honoured.
	if (GFrameCounter > LadderClimbSteerFrame + 1)
	{
		return false;
	}

	// ── (2) IS THAT STEER STILL INTO THE LADDER? ──────────────────────────────────────────────
	// ⛔⛔ A ***SIGN TEST***, ⛔ NOT A CONE — AND THAT IS DELIBERATE, ⛔ not laziness: the intent
	// cone is `AClimbableTower::LadderContactIntentCos` and `CONTACT-§8` forbids a second copy of a
	// tower tunable on this class. A dot > 0 needs ⛔ no number at all, so there is nothing here to
	// drift from the tower's cone. ⚖️ It is also the right RULE for a SUSTAIN as opposed to an
	// ENTRY: entering should be deliberate (a 60° cone held for 0.35 s); staying on a ladder should
	// only require that the player has not turned away from it.
	// ⭐ Measured against the CLIMB LINE's own horizontal direction, which is correct in BOTH
	// directions by construction: on an ascent it points at the tower, on a descent it points back
	// out at the ladder foot — the same two ends the `BothWays` link and `bDeckIsAtEnd` resolve.
	FVector LineHorizontal = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);
	LineHorizontal.Z = 0.f;
	if (LineHorizontal.IsNearlyZero())
	{
		// A perfectly vertical line has no horizontal bearing to steer at, so ANY steer sustains
		// it. ⛔ The alternative (refusing) would make a vertical ladder unclimbable for a reason no
		// player could see. The pinned line is 76°, so this is a guard, ⛔ not the shipped path.
		return true;
	}

	FVector SteerHorizontal = LadderClimbSteerWorld;
	SteerHorizontal.Z = 0.f;
	if (SteerHorizontal.IsNearlyZero())
	{
		return false;
	}

	return FVector::DotProduct(LineHorizontal.GetSafeNormal(), SteerHorizontal.GetSafeNormal()) > 0.f;
}

void AHeroCharacter::DoMove(float Right, float Forward)
{
	if (LadderClimb.bActive)
	{
		// ⛔⛔ CAPTURED AND ⛔ NOT FORWARDED — see the header for why this is two requirements in one
		// override. The world-space frame is rebuilt exactly as `Super::DoMove` builds it (control
		// yaw ⇒ forward/right), so what is captured is the SAME vector the movement component would
		// have received, ⛔ not an approximation of it.
		if (const AController* const OwningController = GetController())
		{
			const FRotator YawRotation(0.f, OwningController->GetControlRotation().Yaw, 0.f);
			const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

			LadderClimbSteerWorld = (ForwardDirection * Forward) + (RightDirection * Right);
			LadderClimbSteerFrame = GFrameCounter;
		}

		// ⛔ NO `Super::DoMove` — ONE STEERING AUTHORITY (`NAV-§3`). In MOVE_Flying the template's
		// two `AddMovementInput` calls are unconstrained 3D flight and would pull the capsule off
		// the pinned line, where there is no deck-breach window and no arrival.
		return;
	}

	Super::DoMove(Right, Forward);
}

void AHeroCharacter::OnLadderClimbWatchdog()
{
	// ⭐ THE BELT, AND IT IS INDEPENDENT OF EVERYTHING THAT DRIVES THE CLIMB: this runs off the
	// WORLD's timer manager, so it survives an actor tick that is disabled, starved or never
	// re-asserted — the exact failure a driver copied from the unit's would have no answer to,
	// because the unit's cover comes from an always-on 0.25 s poll this class does not have.
	if (!LadderClimb.bActive)
	{
		// Belt for the belt: a handle that somehow outlived its climb clears itself rather than
		// looping forever on a walking hero.
		GetWorldTimerManager().ClearTimer(LadderClimbWatchdogTimerHandle);
		return;
	}

	if (bDead)
	{
		// ⚠️ A DEAD HERO STILL IN A CLIMB MEANS EXIT H-4 DID NOT RUN — a death path that bypassed
		// `HandleDeath` entirely. ⛔ Do not trust that it never happens; end it here. Reported as
		// the death exit because that is what it IS, ⛔ not as a timeout.
		EndLadderClimb(/*bReachedTop=*/ false, ESiegeHeroLadderExit::Death);
		return;
	}

	const UWorld* const World = GetWorld();
	if (World && World->GetTimeSeconds() < LadderClimbWatchdogDeadlineSeconds)
	{
		return;
	}

	// ⛔ THE BUDGET IS SPENT (or the world is gone). Drop the hero WHERE IT IS — `TOWER-§8.5a`
	// clause 5 — with the one Warning this feature ever logs.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeHeroLadderExit::Watchdog);
}

bool AHeroCharacter::TryResolveLadderClimbSpeedUU(float& OutClimbSpeedUU)
{
	// ⛔⛔ THERE IS DELIBERATELY ⛔ NO `LadderClimbSpeedUU` ON THIS CLASS. `CONTACT-§8` pins it:
	// *"`LadderClimbSpeedUU` is READ FROM the shipped value, ⛔ never a second copy on the hero"* —
	// ⚖️ two speed properties is how they drift, and this one is Jonathan's exposure lever
	// (`TOWER-§9.3` prices the whole climb in Longbowman shots against it).
	//
	// ⚠️⚠️ AND THE READ IS BY REFLECTION FOR A ***MEASURED*** REASON, ⛔ not for cleverness: the ONE
	// shipped value is `protected` on `ASummonedUnit` (`SummonedUnit.h:1115`, re-grepped by TASK-787
	// after its delegate move shortened that file — `CONTACT-§10.1`), and widening its access would
	// be an edit to another task's file. Reflection reads the CDO's authored default without
	// touching it. ⭐ Once per climb, ⛔ never per frame (the value is cached for the climb).
	//
	// ⚖️⭐ THE MANAGER RULED ON THIS, AND THE RULING IS RECORDED HERE RATHER THAN LEFT AS THE OLD
	// SUGGESTION (`CONTACT-§10.1`, applied to my own comment): TASK-778 wrote that `CONTACT-§8`'s
	// *"read from the TOWER's shipped value"* was wrong — ✅ the finding was CORRECT and the law was
	// REPAIRED to match the code. ⛔⛔ BUT THE "CLEAN LANDING" THIS COMMENT USED TO PROPOSE — *move
	// the property onto `AClimbableTower`* — IS ⛔ RULED **OUT** (`CONTACT-§12.4`): it would make the
	// climb rate PER-TOWER, a design change nobody asked for (a climb rate is a PAWN stat), and it
	// would spend 🧑 Jonathan's `TOWER-§9.3` exposure lever, which prices the whole climb in
	// Longbowman shots against the UNIT's value. ⇒ ⛔ do ⛔ not re-propose it; this reflection read
	// is the shipped answer and the suite guards it.
	static const FName ShippedRatePropertyName(TEXT("LadderClimbSpeedUU"));

	const FFloatProperty* const RateProperty =
		FindFProperty<FFloatProperty>(ASummonedUnit::StaticClass(), ShippedRatePropertyName);
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();

	if (!RateProperty || !UnitDefaults)
	{
		// ⛔ REFUSE THE CLIMB. ⛔ No fallback number is invented, and that is the whole design of
		// this function: a made-up rate would either crawl (a 1 uu/s "safe" default turns a 3.5 s
		// ascent into a 20-minute one that no watchdog would cut short, because the budget scales
		// with the rate) or silently become a SECOND copy of the tunable — the exact thing the law
		// forbids. ⚠️ The suite guards this: `SiegeHeroLadderClimbTest` asserts the property exists
		// under this exact name, so a rename goes RED there rather than going quiet here.
		if (!bWarnedLadderRateUnresolved)
		{
			bWarnedLadderRateUnresolved = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AHeroCharacter '%s': the shipped ladder rate ('%s' on ASummonedUnit) could not be resolved, so hero ladder climbs are REFUSED. ⛔ No rate is invented here — see CONTACT-§8. Restore the property on ASummonedUnit under that exact name; do NOT move it onto AClimbableTower (ruled out at CONTACT-§12.4) and never add a second copy."),
				*GetNameSafe(this), *ShippedRatePropertyName.ToString());
		}
		return false;
	}

	OutClimbSpeedUU = RateProperty->GetPropertyValue_InContainer(UnitDefaults);
	return true;
}

void AHeroCharacter::RefreshNearbyClimbableTowers()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	const double NowSeconds = World->GetTimeSeconds();
	if (NowSeconds < NextClimbableTowerScanSeconds)
	{
		return;
	}
	NextClimbableTowerScanSeconds = NowSeconds + LadderTowerScanIntervalSeconds;

	// ⭐ The shipped building-iteration idiom (`SiegeGameMode.cpp:1342`, `SummonedUnit.cpp:2474`),
	// rate-limited because this one is reached from a Tick rather than from an event. ⛔ There is
	// deliberately ⛔ NO distance filter here: filtering would need a radius, the radius belongs to
	// the tower, and a second copy of it on this class is exactly what `CONTACT-§8` forbids. The
	// tower's own cheap 2D term costs one squared-distance compare per tower per frame and drops a
	// passer-by immediately — which is what it was built to do.
	NearbyClimbableTowers.Reset();
	for (TActorIterator<AClimbableTower> It(World); It; ++It)
	{
		NearbyClimbableTowers.Add(*It);
	}
}

void AHeroCharacter::PollLadderContact(float DeltaSeconds)
{
	// ⭐⭐ WHY THIS POLL LIVES IN `Tick` AND ⛔ NOT ON A TIMER — ***MEASURED***, ⛔ NOT ASSUMED,
	// BECAUSE THE UNIT SIDE HAS EXACTLY THIS TRAP AND IT IS INVISIBLE IN REVIEW.
	//
	// ⚠️⚠️ `ASummonedUnit` ships `PrimaryActorTick.bCanEverTick = true` WITH
	// `bStartWithTickEnabled = FALSE` (`SummonedUnit.cpp:140-141`), and its ONLY tick-flag writer is
	// `RefreshActorTickEnabled` — `WantsActorTick(bLungeActive, LadderClimb.bActive)`
	// (`SummonedUnit.cpp:3662`). ⇒ a unit that is neither lunging nor climbing is ⛔ NOT TICKING, i.e.
	// ⛔ NOT TICKING IN EXACTLY THE STATE A CLIMB MUST START FROM: a poll placed in its `::Tick`
	// would compile, review clean, pass the suite and ⛔ NEVER RUN.
	//
	// ✅⭐ THE HERO IS THE OPPOSITE CASE, AND HERE IS THE EVIDENCE RATHER THAN THE ASSUMPTION:
	//   • this class sets `PrimaryActorTick.bCanEverTick = true` in its constructor and ⛔ never
	//     sets `bStartWithTickEnabled = false` ⇒ it ticks from spawn;
	//   • grepped `Source/`: the ONLY runtime `SetActorTickEnabled` call in the entire project is
	//     `SummonedUnit.cpp:3662`, where a UNIT writes its OWN flag ⇒ ⛔ NOTHING anywhere can
	//     disable a hero's tick — ⛔ not death (it hides, disables input and collision, and stops
	//     movement, but ⛔ never the tick), ⛔ not the ghost hand-off, ⛔ not match end;
	//   • and it is proven by shipped behaviour rather than only by grep: the RECALL channel's
	//     movement-cancel is serviced from this same `Tick` UNCONDITIONALLY (`R-2`) and Jonathan has
	//     played it.
	// ⇒ ✅ `Tick` is the correct home for BOTH the driver (which must be per-frame to move a capsule
	// smoothly) and this poll. ⚠️ And the case the evidence cannot cover — a tick that stops for a
	// reason none of us predicted — is exactly what the watchdog is for, and the watchdog is an
	// `FTimerManager` entry on the WORLD precisely so it cannot be taken down with the tick.

	// ⛔ A dead hero asks nothing, and a climbing hero has nothing to ask: the tower's own occupancy
	// slot would refuse the second entry anyway (`TOWER-§10` L-1), so this is cheaper AND it keeps
	// this class from touching contact bookkeeping it does not own.
	if (bDead || LadderClimb.bActive)
	{
		return;
	}

	// ⛔ `CONTACT-§3.4` bullet 1, expressed where it costs nothing: a hero may not START a climb
	// while a recall channel is running, so it does not ask. ⭐ `BeginLadderClimb` carries the same
	// rule as a REFUSAL (defence in depth — a caller that is not this poll still cannot get in).
	if (IsRecalling())
	{
		return;
	}

	RefreshNearbyClimbableTowers();

	for (int32 Index = NearbyClimbableTowers.Num() - 1; Index >= 0; --Index)
	{
		AClimbableTower* const Tower = NearbyClimbableTowers[Index].Get();
		if (!Tower)
		{
			// Prune on use, exactly as the tower prunes its own contact table: a tower destroyed
			// mid-match must ⛔ not be polled, and a list that only grows is a leak nobody notices.
			NearbyClimbableTowers.RemoveAtSwap(Index);
			continue;
		}

		// ⭐⭐ THE ONE LINE THIS WHOLE CALL SITE EXISTS FOR — **THE PAWN ASKS, THE TOWER DECIDES**.
		// ⛔ Everything that could be a tunable is on the other side of this call: the radius, the
		// 60° cone, the 0.35 s dwell, `K-C`'s re-arm latch, `CanTeamAscend` (`T-3` — ⛔ no hero
		// exemption, an ENEMY hero may not climb your tower either) and the single occupancy slot.
		// ⛔ This class holds ⛔ NONE of them, and the suite asserts that by reflection.
		const AClimbableTower::ELadderContactVerdict Verdict = Tower->TryBeginContactClimb(this, DeltaSeconds);

		if (Verdict == AClimbableTower::ELadderContactVerdict::Climb)
		{
			// The traversal is ALREADY RUNNING and that tower ALREADY holds the occupancy slot.
			// ⛔ Nothing to do here and ⛔ nothing to start — asking a second tower on the same
			// frame could only produce a LadderBusy.
			break;
		}

		// ⭐⭐ EVERY OTHER VERDICT IS A REFUSAL THIS CLASS ACCEPTS IN SILENCE, AND THAT IS THE WHOLE
		// SHAPE OF THE CALL SITE: **THE PAWN ASKS, THE TOWER DECIDES.** ⛔ No log line — this runs
		// out of `Tick`, so one line per refusal is one per frame forever.
		//
		// ⚖️ TASK-787 REMOVED A `NotAnAdmittedClimber` BRANCH THAT USED TO LIVE HERE, AND ITS
		// REMOVAL IS A ⛔ CONSEQUENCE, ⛔ NOT A TIDY-UP. TASK-778 could only ASK and be refused: the
		// tower's identity term admitted `ASummonedUnit` ONLY, so it latched a one-shot Warning
		// naming the closed seam. `CONTACT-§12` widened that term to `ILadderClimber` — which
		// `AHeroCharacter` implements as a COMPILE-TIME BASE — so a hero can no longer produce that
		// verdict at all. ⚖️ `SC-§36` INVERTED: a warning that ⛔ cannot fire is indistinguishable
		// from one that works, and this one would have read as a live diagnostic forever. ⛔ The
		// reasoning is not lost; it is `CONTACT-§12`, which is where a cross-file law belongs.
		// ⚠️ The verdict itself is ⛔ NOT gone from the enum and ⛔ must not be: an `ACharacter` that
		// is not an `ILadderClimber` — a future spectator body — is still refused BY IDENTITY.
		//
		// 📌 AND THIS PARAGRAPH IS SAFE TO WRITE, WHICH TOOK A CHANGE TO THE **PROBE** RATHER THAN
		// TO THE PROSE: `SiegeHeroLadderClimbTest` test 20 row (d) asserts this function no longer
		// BRANCHES on that verdict, and it now counts occurrences in CODE ONLY
		// (`CountOccurrencesInCode`). ⚖️ A scanner that counted comments would have forced this call
		// site to choose between explaining why the branch went and passing its own test — and the
		// explanation would have lost. The prose is the guard; the instrument got smarter.
	}
}

void AHeroCharacter::UnPossessed()
{
	// ⛔ LADDER EXIT H-5 of 10 — THE INDEPENDENT BELT (see the header for why it is ⛔ not a
	// duplicate of H-4). ⚠️ BEFORE Super, while `GetController()` and the movement component are
	// both still coherent. Idempotent with every other exit by the teardown's latch, and it costs
	// one bool test on every ordinary possession change.
	AbortLadderClimb();

	Super::UnPossessed();
}

// ═════════════ LADDER CLIMB REGION END ═════════════
