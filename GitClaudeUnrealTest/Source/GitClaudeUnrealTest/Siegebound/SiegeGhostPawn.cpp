// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeGhostPawn.h"

#include "Animation/AnimationAsset.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h" // complete type for GetGameInstance()->GetSubsystem<>() (Actor.h forward-declares UGameInstance)
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GitClaudeUnrealTest.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "Siegebound/SiegeNetLimits.h"

namespace
{
	/**
	 *  ⛔ THE SAME PRIORITY THE HERO USES (`HeroCharacter.cpp:39`,
	 *  `HeroMappingContextPriority = 1`), mirrored rather than shared because that
	 *  constant is file-local to a file TASK-749 may ⛔ not edit.
	 *  ⚠️ IT MUST STAY EQUAL TO THE HERO'S: the ghost adds the SAME context
	 *  (IMC_Hero) and a different priority would silently re-order the enhanced-
	 *  input stack across a death, which is the class of bug `GHOST-§4` is about.
	 */
	constexpr int32 GhostMappingContextPriority = 1;
}

ASiegeGhostPawn::ASiegeGhostPawn()
{
	// ⛔ NO TICK. This pawn polls nothing, scans nothing and schedules nothing —
	// it is driven entirely by player input and by TASK-750's lifecycle calls.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// ═══ G-2 COLLISION — the one detail that decides whether this works at all ═══
	if (UCapsuleComponent* GhostCapsule = GetCapsuleComponent())
	{
		// The hero's own capsule size (AGitClaudeUnrealTestCharacter ctor:
		// InitCapsuleSize(42.f, 96.f)) — G-1 "similar to their original body", so
		// the ghost fits through exactly what the hero fits through.
		GhostCapsule->InitCapsuleSize(42.f, 96.f);

		// ⛔⛔ OBJECT TYPE STAYS ECC_Pawn — READ THE HEADER BEFORE CHANGING THIS.
		// G-2's "blocks WorldStatic" is a RESPONSE ruling, ⛔ not an object-type
		// ruling. Re-typing this to ECC_WorldStatic would insert the ghost into
		// AProjectile::FindTerrainHit's WorldStatic/WorldDynamic object trace
		// (Projectile.cpp:434) and hand a dead player a projectile shield.
		// ⛔ And it is deliberately ⛔ NOT a team channel: AHeroCharacter re-stamps
		// its capsule to SiegeTeamObjectChannel(GetTeamId()) (HeroCharacter.cpp:113)
		// because those are the COMBATANT BODY channels. A ghost is not a combatant
		// body and must never wear one. (Declared consequence: castle gate blockers
		// block only the enemy team channel — Castle.cpp:606-608 — so they do not
		// stop the ghost. Flagged for TASK-755; G-3 makes the intrusion inert.)
		GhostCapsule->SetCollisionObjectType(ECC_Pawn);

		// Ignore EVERYTHING, then block exactly one channel. Authored in this
		// order deliberately: an allow-list can never acquire a response by
		// accident when a new channel is added to the project later.
		GhostCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);

		// ✅ BLOCK WorldStatic — the terrain, walls and castle geometry. This is
		// what makes the ghost WALK THE GROUND instead of falling through the world,
		// and it is what makes G-1's "no wall-pass" true: the castle's real walls
		// are WorldStatic and genuinely stop it.
		GhostCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

		// ⛔⛔ PAWN STAYS IGNORE, AND THIS IS A DESIGN REQUIREMENT, ⛔ NOT AN
		// OVERSIGHT: a ghost that blocked pawns would be a free BODY-BLOCK WALL
		// handed to a dead player — a combat effect on a pawn specified to have
		// none (G-2). It is restated here so the line is never "fixed".
		GhostCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		// Query AND physics: CharacterMovement stops at blocking geometry through
		// query sweeps, so QueryOnly would let the ghost sink through the floor.
		GhostCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// ⛔ Raises no overlap events and affects no navigation: the ghost is not a
		// trigger, not an objective, and must never re-cut the navmesh the living
		// fleet is pathing on while it walks around.
		GhostCapsule->SetGenerateOverlapEvents(false);
		GhostCapsule->SetCanEverAffectNavigation(false);
	}

	// ═══ G-1 MOVEMENT — the hero's rig, mirrored ═══
	// AGitClaudeUnrealTestCharacter ctor:20-33. MaxWalkSpeed is deliberately NOT
	// set here: BeginPlay DERIVES it from AHeroCharacter so the number cannot drift.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* GhostMovement = GetCharacterMovement())
	{
		GhostMovement->bOrientRotationToMovement = true;
		GhostMovement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

		// ⛔ G-1 "no flight": the ghost is a WALKING pawn under normal gravity.
		// ⛔ Do not set bCanFly, and do not change DefaultLandMovementMode.
		// ⚠️ These are FIELD ASSIGNMENTS, deliberately — ⛔ NOT SetMovementMode(),
		// which resolves against CharacterOwner and must not be called from a
		// constructor where the owner is not yet established.
		GhostMovement->GravityScale = 1.0f;
		GhostMovement->DefaultLandMovementMode = MOVE_Walking;
		GhostMovement->NavAgentProps.bCanFly = false;
		GhostMovement->NavAgentProps.bCanSwim = false;

		// ⛔ The ghost must not shove the living fleet around — the physics half of
		// the same body-block concern the capsule's ECC_Pawn=Ignore response
		// handles on the query side. Both halves are needed: Ignore stops the
		// sweep, this stops the impulse.
		GhostMovement->bEnablePhysicsInteraction = false;
	}

	// ═══ G-1 VISION — the hero's camera rig, mirrored exactly ═══
	// AGitClaudeUnrealTestCharacter ctor:39-47. ⛔ Identical values on purpose:
	// G-1 forbids EXTENDED vision, so a longer arm or a raised camera would be a
	// scouting buff granted as a reward for dying.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// ═══ G-4 ENEMY-VISIBLE — the mesh is seen by everyone ═══
	if (USkeletalMeshComponent* GhostMeshComponent = GetMesh())
	{
		// ⛔ NEVER owner-only and ⛔ never owner-hidden: Jonathan's explicit words
		// are "The enemy should be able to see this ghost as well." Both flags are
		// set EXPLICITLY rather than left at their defaults so the intent is
		// readable and so FSiegeGhostPawnEnemyVisibleTest can assert it.
		// ⚠️ Deliberately the OPPOSITE of MARK-§ M-3 (map marks are NOT enemy-
		// visible). ⭐ The two are stated together here so nobody ever "makes them
		// consistent" — the contrast is the ruling, not an accident.
		GhostMeshComponent->SetOwnerNoSee(false);
		GhostMeshComponent->SetOnlyOwnerSee(false);

		// The body is a visual only: it carries no collision of its own (the
		// capsule above is the entire physical presence of this pawn).
		GhostMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		GhostMeshComponent->SetGenerateOverlapEvents(false);
		GhostMeshComponent->SetCanEverAffectNavigation(false);
	}

	// ═══ M8 NET RELEVANCY — TIER B (GHOST-§6; CONVENTIONS NET RELEVANCY LAW) ═══
	// ⛔ bReplicates is NOT set here: APawn's own constructor already sets it
	// (engine Pawn.cpp:86) — mirrored from AHeroCharacter.cpp:61, which says so
	// for the same reason. Setting it again would imply this class opted in.
	// A ghost must read across the whole 500 m arena because G-4 requires the
	// ENEMY to see it; under the engine's default 150 m cull it would pop in and
	// out at range. Same tier as the hero, deliberately: the ghost stands in for
	// the hero, so it must be visible wherever the hero would have been.
	// The distance comes from the ONE arena constant — ⛔ NEVER a per-class
	// literal: when the arena grows, SiegeNet::ArenaRelevancyDistance changes and
	// this follows. UE 5.5+ API: the raw field is deprecated for public access,
	// so the setter is the supported path.
	// ⛔ NO RPC and ⛔ NO replicated property is authored in this batch (ACC-§8).
	SetNetCullDistanceSquared(SiegeNet::ArenaRelevancyDistanceSquared);
}

void ASiegeGhostPawn::BeginPlay()
{
	Super::BeginPlay();

	// ═══ G-1: THE HERO'S OWN MOVEMENT SPEED — DERIVED, ⛔ NEVER DUPLICATED ═══
	// ⭐ Reading the hero's class default is the point: G-1 says "THE HERO'S OWN",
	// so a hand-typed 500.f here would be a second source of truth that silently
	// stops matching the day the hero is retuned. There is deliberately ⛔ no
	// designer override property — an override is exactly how a ghost acquires the
	// speed buff G-1 forbids.
	// ⚠️ The CDO is read, ⛔ not the dead hero instance: the CDO carries
	// SwiftBootsStacks = 0 (HeroCharacter.h:574), so this resolves to the base
	// WalkSpeed (500.f, HeroCharacter.h:406) and the ghost is deliberately ⛔ NOT
	// buffed by the items the corpse was carrying.
	if (UCharacterMovementComponent* GhostMovement = GetCharacterMovement())
	{
		if (const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>())
		{
			// ⚠️ GetWalkSpeed() is the PUBLIC wrapper (HeroCharacter.h) over the protected
			// GetEffectiveWalkSpeed() — the same pairing as GetMaxHP()/GetEffectiveMaxHP().
			// ASiegeGhostPawn does ⛔ not derive from AHeroCharacter, so the protected form is
			// unreachable from here (TASK-761, C2248). The VALUE is byte-identical: the wrapper
			// is an inline `return GetEffectiveWalkSpeed();` and composes nothing extra.
			const float HeroWalkSpeed = HeroDefaults->GetWalkSpeed();

			// Guard the derivation rather than trusting it: a zero/negative value
			// would produce a ghost that cannot move at all for the whole respawn
			// delay, which is indistinguishable to the player from the input-dead
			// failure GHOST-§4 is about. Fall back to the movement component's own
			// default and say so.
			if (HeroWalkSpeed > 0.f)
			{
				GhostMovement->MaxWalkSpeed = HeroWalkSpeed;
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegeGhostPawn '%s': AHeroCharacter's default walk speed resolved to %.2f (expected > 0) — keeping MaxWalkSpeed %.2f. G-1 parity is NOT guaranteed."),
					*GetNameSafe(this), HeroWalkSpeed, GhostMovement->MaxWalkSpeed);
			}
		}
	}

	// ═══ G-4: APPEARANCE ═══
	if (USkeletalMeshComponent* GhostMeshComponent = GetMesh())
	{
		if (!GhostMesh.IsNull())
		{
			if (USkeletalMesh* const LoadedMesh = GhostMesh.LoadSynchronous())
			{
				// SetSkeletalMeshAsset is the current (non-deprecated) UE5 setter —
				// the house convention (CommanderNpc.cpp:280, SummonedUnit.cpp:555).
				GhostMeshComponent->SetSkeletalMeshAsset(LoadedMesh);
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegeGhostPawn '%s': GhostMesh '%s' failed to load — the ghost will be INVISIBLE, which violates G-4 (\"the enemy should be able to see this ghost\")."),
					*GetNameSafe(this), *GhostMesh.ToString());
			}
		}
		else
		{
			// ⚠️ NOT a silent default. G-4 is one of Jonathan's four explicit
			// requirements and an unset mesh breaks it outright, so this is a
			// WARNING with the reason attached — never a quiet invisible ghost.
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeGhostPawn '%s': GhostMesh is not assigned (expected on BP_SiegeGhostPawn) — the ghost is INVISIBLE and G-4 (\"the enemy should be able to see this ghost as well\") is NOT satisfied."),
				*GetNameSafe(this));
		}

		// ⛔ THE MATERIAL IS DELIBERATELY ⛔ NOT APPLIED HERE (TASK-758) — it is the
		// seat of the team tint, so it belongs to ApplyGhostTeamAppearance(), which
		// runs at the instant the team is KNOWN. See the tail of this function.

		// ⛔⛔ SC-§35 / GHOST-§4 — THE ANIM-OWNER LAW, DISCHARGED BY CONSTRUCTION.
		// ⭐ SINGLE-NODE PLAYBACK, and it is chosen precisely because it is
		// STRUCTURALLY INCAPABLE of the defect class SC-§35 was bought with:
		// there is ⛔ NO Anim Blueprint here, so there is ⛔ no TryGetPawnOwner()
		// that could resolve against the wrong owner class, and no ABP can be
		// compiled against an assumed owner that this pawn is not.
		// 📌 THE OWNER-CLASS STATEMENT THE LAW REQUIRES, MADE EXPLICITLY EVEN
		// THOUGH THIS SHAPE CANNOT TRIP IT: the animation below is driven by
		// UAnimSingleNodeInstance, whose owner is this ASiegeGhostPawn — an
		// ACharacter and therefore an APawn, so any future ABP swapped in here
		// may assume a pawn owner and TryGetPawnOwner() WILL resolve.
		// ⛔ Do not replace this with a TSoftClassPtr<UAnimInstance> without
		// re-reading SC-§35 and re-stating the assumed owner at the assignment.
		if (!GhostIdleAnimation.IsNull())
		{
			if (UAnimationAsset* const LoadedAnimation = GhostIdleAnimation.LoadSynchronous())
			{
				GhostMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				GhostMeshComponent->PlayAnimation(LoadedAnimation, /*bLooping=*/ true);
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegeGhostPawn '%s': GhostIdleAnimation '%s' failed to load — the ghost renders in its reference pose (visible; G-4 still satisfied)."),
					*GetNameSafe(this), *GhostIdleAnimation.ToString());
			}
		}
	}

	// ═══ TASK-758: THE TEAM-DEPENDENT HALF — DELIBERATELY ⛔ NOT WRITTEN INLINE ═══
	// ⚠️ AT THIS INSTANT THE TEAM IS ⛔ NOT KNOWN. BeginPlay fires inside SpawnActor
	// and the lifecycle owner calls InitializeGhost only AFTER SpawnActor returns
	// (measured: SiegeGameMode.cpp:823 → :842) ⇒ GhostTeam is still its declaration
	// default here, and ⛔ ANY team-dependent line written above would be silently
	// wrong for Red on every single death. ⇒ this call is EXPECTED TO NO-OP in the
	// shipped ordering, and InitializeGhost's own call drives the work a moment later.
	//
	// ⭐ IT IS STILL HERE, AND THAT IS THE POINT: it covers the OTHER ordering — a
	// deferred spawn that sets the team BEFORE FinishSpawning — where InitializeGhost
	// runs first and the mesh above did not yet exist for it to stamp. The two call
	// sites TOGETHER are correct no matter which one runs last.
	// ⛔ DO NOT DELETE EITHER CALL, AND ⛔ DO NOT "SIMPLIFY" THEM INTO ONE.
	ApplyGhostTeamAppearance();
}

void ASiegeGhostPawn::ApplyGhostTeamAppearance()
{
	// ═══════════════════════════════════════════════════════════════════════════
	// ⛔⛔ THE ONE PLACE TEAM-DEPENDENT INITIALISATION MAY LIVE (TASK-758).
	// The defect this function exists to make IMPOSSIBLE, stated once so it is not
	// re-introduced: team-dependent work placed in BeginPlay reads GhostTeam BEFORE
	// anyone has set it, so it is always Blue — correct-looking, compiling, silently
	// wrong for Red, and invisible to review.
	// ═══════════════════════════════════════════════════════════════════════════

	// ⛔⛔ THE GATE, AND IT IS THE WHOLE FIX. Nothing below runs until the team is
	// actually KNOWN. ⚠️ Note what this is NOT: it is not `GhostTeam == Blue`, which
	// cannot distinguish "a Blue ghost" from "nobody told me yet" — ETeamId has no
	// unset value (TeamId.h), and that ambiguity IS the defect.
	if (!bGhostTeamAssigned)
	{
		return;
	}

	// ⛔⛔ THERE IS DELIBERATELY NO "if (bTeamAppearanceApplied) return;" HERE.
	// The re-run is ⛔ not redundancy — it is what makes the InitializeGhost-before-
	// BeginPlay ordering correct, because the skeletal mesh whose slots are stamped
	// below does not exist until BeginPlay has assigned it. An early-out would
	// freeze the first, mesh-less run and ship an untinted ghost.
	// ⭐ FSiegeGhostPawnTeamStepIsRerunnableTest goes red the day one is added.

	if (USkeletalMeshComponent* const GhostMeshComponent = GetMesh())
	{
		if (!GhostMaterial.IsNull())
		{
			if (UMaterialInterface* const LoadedMaterial = GhostMaterial.LoadSynchronous())
			{
				// Every slot: a partially-tinted ghost reads as a rendering bug.
				const int32 MaterialSlotCount = GhostMeshComponent->GetNumMaterials();
				for (int32 SlotIndex = 0; SlotIndex < MaterialSlotCount; ++SlotIndex)
				{
					GhostMeshComponent->SetMaterial(SlotIndex, LoadedMaterial);
				}

				// 📌 ⭐ THE TEAM TINT'S SEAT, AND IT IS A SEAT ⛔ NOT A PROMISE.
				// If the G-4 readability tint — "whose ghost is that?" — is ever
				// ruled in, it is decided HERE and ⛔ nowhere else, whichever shape it
				// takes (a per-team material instance chosen from GhostTeam, or a
				// dynamic instance parameter driven from it), because this is the only
				// point in this class where the team is GUARANTEED known.
				// ⛔⛔ IT IS ⛔ NOT AUTHORED, ⛔ NOT SCHEDULED AND ⛔ NOT AWAITED.
				// ⛔ No line here references a team material, ⛔ nothing is left
				// unwired, and ⛔ this comment binds no task (SC-§36: naming a future
				// task as the binder is a DEBT, not a wiring). ⚠️ The tint is a
				// FLAGGED, undecided ruling — a decision to ship one must be BOARDED.
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegeGhostPawn '%s': GhostMaterial '%s' failed to load — falling back to the mesh's own materials (still visible, just not ghostly)."),
					*GetNameSafe(this), *GhostMaterial.ToString());
			}
		}
	}

	// 📌 THE RECORD OF **WHAT THE TEAM ACTUALLY WAS WHEN THE WORK HAPPENED** —
	// ⛔ not test scaffolding. It has two shipped consumers: NotifyControllerChanged
	// reads the gate to catch a ghost possessed without ever being initialised, and
	// this pair is the only way the ordering itself is OBSERVABLE (SC-§37 — where
	// correctness is invisible to review, MEASURE the property). Under the old
	// ordering this would have recorded Blue on a Red ghost and nothing could see it.
	AppliedAppearanceTeam = GhostTeam;
	bTeamAppearanceApplied = true;
}

bool ASiegeGhostPawn::HasAppliedTeamAppearance(ETeamId& OutAppliedTeam) const
{
	// ⚠️ OutAppliedTeam is written unconditionally so no caller can read an
	// uninitialised local, but it is MEANINGLESS when this returns false — ETeamId
	// has no unset value, so the bool is the only honest answer to "has it run yet?".
	OutAppliedTeam = AppliedAppearanceTeam;
	return bTeamAppearanceApplied;
}

void ASiegeGhostPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// ═══════════════════════════════════════════════════════════════════════════
	// ⚠️⚠️ THE HIGHEST-CONSEQUENCE FUNCTION IN THIS FILE (`GHOST-§4`).
	//
	// MEASURED, ⛔ not assumed (`HeroCharacter.cpp:228-280`): the input mapping
	// context is added by the **HERO PAWN**, not by ASiegePlayerController — whose
	// own comment states it "does not add contexts the way the template
	// controllers do". ⇒ the moment the controller stops possessing the hero,
	// NOTHING is re-adding IMC_Hero.
	//
	// ⇒ A ghost that did not do this itself would leave the player unable to move,
	// unable to issue a single order, and unable to open the war map or the
	// assistant console — for the ENTIRE HeroRespawnDelay. That is precisely the
	// "boots the arena input-dead" failure GHOST-§4 exists to prevent, and at
	// 180 seconds it would be 36× more expensive than the shipped 5 s ever was.
	//
	// ⛔ CURSOR AND INPUT-MODE OWNERSHIP IS NOT TOUCHED HERE. HELP-§5 binds it to
	// ApplyCursorInputState() and NOWHERE else; adding a mapping context does not
	// change the input MODE, and this file contains no SetInputMode, no
	// bShowMouseCursor and no EnableInput/DisableInput call. The enable/disable
	// ORDERING around possession is TASK-750's, by design.
	// ═══════════════════════════════════════════════════════════════════════════

	// ═══ TASK-758 — THE "NOBODY EVER INITIALISED ME" DETECTOR ═══
	// ⚠️ SC-§36's lesson wearing this class's costume. The ordering gate above is
	// correct — the team-dependent step refuses to run until the team is known — but
	// a step that quietly NEVER runs is exactly the silent omission that law was
	// bought with, and the better the guard, the quieter the omission.
	// ⇒ THE ONE MOMENT IT IS CATCHABLE is possession by a real player, which the
	// lifecycle owner performs AFTER InitializeGhost (measured: SiegeGameMode.cpp:842
	// → :848). ⇒ this line is SILENT in the shipped path and LOUD the day a caller
	// forgets. ⛔ Guarded on a real APlayerController so the CDO, the raw C++ class,
	// an unpossessed instance and the un-possess notification all stay quiet.
	if (!bGhostTeamAssigned && Cast<APlayerController>(GetController()) != nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeGhostPawn '%s': POSSESSED BY A PLAYER BUT InitializeGhost() WAS NEVER CALLED — the ghost's team is still the declaration default and its team-dependent appearance has NOT been applied (G-4 readability). Call InitializeGhost(Team) after SpawnActor and BEFORE Possess."),
			*GetNameSafe(this));
	}

	if (!GhostMappingContext)
	{
		// ⚠️ Warn rather than fail quietly: an unassigned context is a fully
		// input-dead ghost, and the player cannot tell that from a crash.
		// Guarded on having a real player controller so the raw C++ class and any
		// unpossessed/server-side instance stay silent.
		if (Cast<APlayerController>(GetController()) != nullptr)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegeGhostPawn '%s': GhostMappingContext is not assigned (expected /Game/Input/IMC_Hero via BP_SiegeGhostPawn) — the player has NO INPUT while this ghost is possessed."),
				*GetNameSafe(this));
		}
		return;
	}

	// The hero's exact guard chain, mirrored: context -> APlayerController ->
	// ULocalPlayer -> UEnhancedInputLocalPlayerSubsystem. Null-safe at every step.
	if (const APlayerController* const PC = Cast<APlayerController>(GetController()))
	{
		if (const ULocalPlayer* const LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* const Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				// ─── POSITIONAL KEYBOARD LAYOUT (KBD-§5 / KBD-§6) ───
				// ⛔ THE PLACEMENT IS PART OF THE CORRECTNESS, ⛔ NOT A STYLE CHOICE
				// (SC-§21): this resolve sits in the INNERMOST scope, beside the
				// AddMappingContext call it feeds, because NotifyControllerChanged
				// also runs on the SERVER for a remote client's pawn — hoisting it
				// above the GetLocalPlayer() check would probe an OS keyboard
				// layout on behalf of a machine that is not there.
				// ⚠️ WITHOUT THIS, A DVORAK PLAYER'S GHOST WOULD BE WASD-BROKEN
				// while their living hero was fine — a bug invisible to every
				// reviewer on a QWERTY host.
				// ⛔ FAIL-SAFE (KBD-§5): no GameInstance or no subsystem ⇒
				// ContextToApply stays GhostMappingContext and behaviour is
				// byte-identical to this feature not existing.
				const UInputMappingContext* ContextToApply = GhostMappingContext;
				if (const UGameInstance* const GameInstance = GetGameInstance())
				{
					if (USiegeKeyboardLayoutSubsystem* const LayoutSubsystem = GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>())
					{
						ContextToApply = LayoutSubsystem->GetPositionalContext(GhostMappingContext);
					}
				}

				// ⛔ ADD, never a clear-then-add: the ghost is ADDED to the existing
				// input composition and NEVER re-orders it (GHOST-§4). Adding the
				// same context the hero used, at the same priority, leaves the
				// enhanced-input stack in the shape it was already in.
				Subsystem->AddMappingContext(ContextToApply, GhostMappingContextPriority);
			}
		}
	}
}

void ASiegeGhostPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// ═══════════════════════════════════════════════════════════════════════════
	// ⭐⭐ "CANNOT ATTACK" IS ENFORCED HERE BY **OMISSION**, AND THAT IS THE WHOLE
	//     MECHANISM — there is ⛔ no attack-suppression branch anywhere.
	//
	// MEASURED (`HeroCharacter.cpp:282-321`): IA_Attack -> DoMeleeAttack,
	// IA_Sprint and IA_Rally are bound on the HERO PAWN's input component. This
	// pawn binds ⛔ ONLY Move and Look ⇒ while the ghost is possessed there is no
	// attack binding IN EXISTENCE to press, and no rally, and no sprint.
	// ⛔ DO NOT ADD ONE, and ⛔ do not "restore parity" with the hero's bindings.
	//
	// ⭐ AND G-3's POWERS NEED NO BINDING HERE AT ALL: the unit orders, the war
	// map, the assistant console and the controls overlay are bound on the PLAYER
	// CONTROLLER (`SiegePlayerController.cpp:464-626`), which does not change
	// during a possession swap. They work the moment GhostMappingContext is added
	// above. ⛔ Do not mirror them onto this pawn — a second binding for the same
	// action is a double-fire, not a feature.
	//
	// ⛔ G-5 (card play) is the FLAGGED-TO-JONATHAN row and its proceeding default
	// is NO. The card slot actions are controller-bound, so this file neither
	// grants nor blocks them; ⛔ do not build either side of that ruling here.
	// ═══════════════════════════════════════════════════════════════════════════
	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegeGhostPawn '%s': no UEnhancedInputComponent — the ghost cannot be driven. Siegebound is built on Enhanced Input."),
			*GetNameSafe(this));
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASiegeGhostPawn::Move);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeGhostPawn '%s': MoveAction not assigned (expected /Game/Input/Actions/IA_Move via BP_SiegeGhostPawn) — the ghost cannot walk."),
			*GetNameSafe(this));
	}

	// Both look actions, exactly as the template binds them (gamepad + mouse).
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASiegeGhostPawn::Look);
	}

	if (MouseLookAction)
	{
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ASiegeGhostPawn::Look);
	}

	if (!LookAction && !MouseLookAction)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeGhostPawn '%s': neither LookAction nor MouseLookAction is assigned (expected IA_Look / IA_MouseLook via BP_SiegeGhostPawn) — the ghost cannot look around, so G-3's \"look at the map\" companion (looking at the FIELD) is degraded."),
			*GetNameSafe(this));
	}
}

void ASiegeGhostPawn::Move(const FInputActionValue& Value)
{
	// Control-rotation-relative planar movement — byte-identical in behaviour to
	// AGitClaudeUnrealTestCharacter::DoMove (`:93-111`), which is what the hero
	// uses. G-1: the ghost moves the way the body did.
	if (const AController* const MyController = GetController())
	{
		const FVector2D MovementVector = Value.Get<FVector2D>();

		const FRotator Rotation = MyController->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASiegeGhostPawn::Look(const FInputActionValue& Value)
{
	// Mirrors AGitClaudeUnrealTestCharacter::DoLook (`:113-121`).
	// ⛔ G-1 "no extended vision": no FOV change, no arm extension, no unlocked
	// pitch — the ghost sees exactly what the hero saw.
	if (GetController() != nullptr)
	{
		const FVector2D LookAxisVector = Value.Get<FVector2D>();

		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASiegeGhostPawn::InitializeGhost(ETeamId InTeam)
{
	// ⛔⛔ IDENTITY ONLY — ⛔ NEVER A COMBAT AFFILIATION.
	// This does ⛔ not stamp a collision channel (the capsule stays ECC_Pawn, see
	// the constructor), does ⛔ not register with any team system, and is ⛔ not
	// exposed through ITeamAgent. It exists because G-4 makes the ghost
	// enemy-visible and an observer needs to know WHOSE ghost they are looking at.
	GhostTeam = InTeam;

	// ═══ TASK-758 — THE ORDERING FIX, AND THIS IS ITS HINGE ═══
	// ⭐⭐ THE TEAM IS KNOWN FROM THIS LINE ONWARD, AND IN THE SHIPPED LIFECYCLE
	// THIS IS THE FIRST INSTANT AT WHICH THAT IS TRUE — BeginPlay has already been
	// and gone (it fires inside SpawnActor; measured at SiegeGameMode.cpp:823 → :842).
	// ⇒ the gate opens here…
	bGhostTeamAssigned = true;

	// …and the team-dependent work is driven HERE, where the answer is REAL, rather
	// than at a moment that merely LOOKS like startup. ⛔ Do not remove this call and
	// ⛔ do not move the work it performs back into BeginPlay: that is precisely the
	// defect TASK-756 found, and FSiegeGhostPawnTeamKnownBeforeTeamWorkTest fails
	// loudly if either happens.
	ApplyGhostTeamAppearance();

	// ⚠️ STATED HONESTLY RATHER THAN OVERSOLD (a comment that overstates what the
	// code does is the drift defect this project keeps paying for), AND AMENDED BY
	// TASK-758 SO IT STAYS TRUE: the ONLY thing that reads GhostTeam today is
	// ApplyGhostTeamAppearance() above, and all it does with it is RECORD which team
	// the appearance step observed — ⛔ the material it stamps does not yet vary by
	// team. ⇒ the value is still stored for two named, already-identified consumers:
	//   1. the G-4 readability tint — an observer who can see a ghost needs to know
	//      WHOSE it is. ⛔ No art task in this batch produces a team-tinted ghost
	//      material, so the tint is an OWED art dependency, flagged in the handoff,
	//      and its code seat is marked in ApplyGhostTeamAppearance().
	//   2. the M8 replication shape reserved in the header (GHOST-§6 / ACC-§8) —
	//      a client cannot tint what it was never told.
	// ⛔ Nothing else may start reading it without re-reading GHOST-§1 first: the
	// moment this becomes a friend/foe input, the ghost has a combat identity.
}

void ASiegeGhostPawn::RetireGhost()
{
	// ⚠️ IDEMPOTENT BY CONTRACT so TASK-750 never needs a "did I already retire
	// it?" flag — and because there are TWO legitimate callers (respawn at
	// HeroRespawnDelay, and match end per GHOST-§2's inherited rule) that can both
	// fire for the same ghost when a match ends near the respawn boundary.
	if (bRetired)
	{
		return;
	}
	bRetired = true;

	// ⚠️ CALLER ORDERING, RESTATED BECAUSE GETTING IT WRONG IS EXPENSIVE
	// (GHOST-§4): TASK-750 must re-possess the hero FIRST and retire the ghost
	// SECOND. Retiring a still-possessed pawn leaves the controller pawnless,
	// which is the input-dead failure in a different costume. This function
	// deliberately does ⛔ NOT un-possess: possession ordering is the lifecycle
	// owner's, not this pawn's.
	if (const AController* const MyController = GetController())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegeGhostPawn '%s': RetireGhost() called while STILL POSSESSED by '%s'. The hero should be re-possessed FIRST (GHOST-§4); destroying a possessed pawn leaves the controller pawnless."),
			*GetNameSafe(this), *GetNameSafe(MyController));
	}

	// ⛔ NO TIMER IS CLEARED HERE BECAUSE THIS CLASS OWNS NONE. GHOST-§2's
	// QA-binding policy — clear ONLY the handles the owning class owns, never a
	// world-wide clear — is satisfied vacuously and deliberately: the ghost
	// schedules nothing, so a 180 s lifetime cannot leak a handle through it.
	Destroy();
}
