// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/CommanderNpc.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"

namespace
{
	/** The re-used avatar body — the SHIPPED fleet rig (WR-§5 / D5 ruling: zero credits, zero rig work). */
	const TCHAR* CommanderAvatarMeshPath = TEXT("/Game/Characters/SK_Sorcerer.SK_Sorcerer");

	/** The war table (authored TASK-556, imported TASK-566 — absent as this file is written; the resolve is null-safe). */
	const TCHAR* CommanderWarTableMeshPath = TEXT("/Game/Meshes/SM_WarTable.SM_WarTable");

	/**
	 *  The looping IDLE clip for the avatar body, played SINGLE-NODE.
	 *
	 *  ⛔⛔ THIS CONSTANT REPLACES A SHARED-ANIMBP PATH THAT WAS A MEASURED
	 *  DEFECT, AND THE REPLACEMENT IS STRUCTURAL, NOT COSMETIC (TASK-591 /
	 *  SC-§35). The retired line assigned /Game/Characters/ABP_Footman — the
	 *  whole rigged fleet's locomotion AnimBlueprint — onto AvatarAnimClassAsset.
	 *  That ABP's EventGraph calls TryGetPawnOwner(), which returns None on a
	 *  non-Pawn owner, and ACommanderNpc is an AActor. Both of its
	 *  "Set GroundSpeed" and "Set bIsMoving" nodes therefore failed EVERY FRAME,
	 *  ON EVERY INSTANCE: 1,806 Blueprint runtime errors in 49 s with two
	 *  commanders alive. It compiled clean, passed the whole automation suite and
	 *  cleared two code-review gates, because the broken contract is a BLUEPRINT
	 *  GRAPH's assumption about its owner's class — a thing no C++ compiler, no
	 *  headless test and no code read can see.
	 *
	 *  ⭐ WHY A UAnimSequence AND NOT A DIFFERENT ABP: a single-node anim instance
	 *  HAS NO GRAPH, so it cannot ask anything about its owner, so it is
	 *  STRUCTURALLY INCAPABLE of that defect class. That is the reason for the
	 *  shape (SC-§35 item 2), ⛔ not convenience. It is also the SAME API the
	 *  fleet already uses for its per-unit clips (ASummonedUnit::
	 *  PlaySkeletalAttackAnim / PlaySkeletalDeathAnim, SummonedUnit.cpp).
	 *
	 *  ⚠️ A HYPOTHESIS, DECLARED AS ONE (SC-§20) — ⛔ NOT A MEASUREMENT: this clip
	 *  is EXPECTED to sit on the same SK_Footman_Skeleton as SK_Sorcerer, because
	 *  the whole rigged fleet is retargeted onto that one skeleton and this
	 *  sequence sits beside A_Footman_Idle in the same folder. ⛔ NOBODY HAS
	 *  OPENED THE ASSET TO CHECK, and this comment does not pretend otherwise.
	 *  ApplyAvatarAnimation therefore verifies the skeleton ITSELF and degrades to
	 *  ref pose on a mismatch — see that function for why the engine's own
	 *  single-node path will NOT do that for us.
	 */
	const TCHAR* CommanderAvatarIdleAnimPath = TEXT("/Game/Characters/Anims/A_Sorcerer_Idle.A_Sorcerer_Idle");

	/**
	 *  Yaw applied to the avatar mesh so it faces the actor's forward (+X).
	 *
	 *  ⚠️ NOT AN ARBITRARY NUMBER — IT IS THE FLEET'S ONE BAKED FORWARD. Every
	 *  rigged Siegebound character goes through Tools/ArtPipeline/rig_character.py
	 *  onto the shared SK_Footman_Skeleton, which arrives as UE-local +Y
	 *  (measured in-engine, TASK-326). Actor forward is +X and
	 *  Rot(θ)·(0,1,0) = (−sinθ, cosθ) = (1,0,0) ⇒ θ = −90. It is the SAME
	 *  constant, for the SAME reason, as ASiegePlayerController::GhostYawOffset
	 *  and ASummonedUnit::SkeletalVisualYawOffset. Without it the commander
	 *  stands SIDEWAYS to the room TASK-562 aims him at.
	 *
	 *  Applied ONCE, in the constructor, as the component template's relative
	 *  rotation — so BP_CommanderNpc's component override wins cleanly if the
	 *  artist ever needs a different pose. ⛔ Deliberately NOT re-applied at
	 *  runtime, which would stomp that override.
	 */
	constexpr float CommanderAvatarYawOffset = -90.f;

	/**
	 *  How far in front of the commander (actor +X, floor plane) the war table
	 *  sits, in uu.
	 *
	 *  ⚠️ AN INVENTED NUMBER, DECLARED AS ONE — and, like InteractRadius, a
	 *  HUMAN-SCALE one (SC-§34's exemption / WR-§1): 200 uu ≈ 2 m is
	 *  "arm's-length plus" for a person standing at a table, and it does NOT
	 *  scale with the 9× castle. It is comfortably inside the 400 uu
	 *  InteractRadius, so a player who has walked to the table is by
	 *  construction in range of the commander.
	 *
	 *  Set as the component template's relative location in the constructor,
	 *  which makes the BLUEPRINT the tuning surface (BP_CommanderNpc,
	 *  TASK-568) rather than a runtime write that would stomp it. FLAGGED for
	 *  the art/integration pass once SM_WarTable's real footprint exists —
	 *  today its dimensions are unknown to this file by design.
	 */
	constexpr float CommanderWarTableForwardOffset = 200.f;
}

ACommanderNpc::ACommanderNpc()
{
	// No per-frame work at all: this actor is a body and a table. The war map's
	// proximity gate is POLLED BY THE CALLER (TASK-563) through IsPlayerInRange,
	// which keeps the cost on the one client that can actually open a map
	// instead of on two actors every frame (the house never-per-tick law).
	PrimaryActorTick.bCanEverTick = false;

	// ⚖️ NET RELEVANCY TIER C — NOT REPLICATED (WR-§8). bReplicates is
	// deliberately left at the AActor default (false) and is never touched:
	// ACastle::BeginPlay runs on both machines, so each builds its own local
	// commander and pushes the castle's already-replicated Team into it. See
	// the class doc for the full rationale. This actor adds no replicated
	// property, no new replicated class, no new relevancy tier and no RPC.

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// The avatar body. NoCollision + no nav influence: this prop stands inside
	// the castle interior, which is navigable space the units spawn into, and a
	// blocking primitive there is a traversability hazard (the AGoldNode
	// "blocks NOTHING" posture). Movable mobility is REQUIRED, not cosmetic — a
	// Static-mobility component refuses SetSkeletalMeshAsset once the world has
	// begun play, which would break the deferred-asset runtime resolve below.
	AvatarMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AvatarMesh"));
	AvatarMesh->SetupAttachment(SceneRoot);
	AvatarMesh->SetMobility(EComponentMobility::Movable);
	AvatarMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	AvatarMesh->SetGenerateOverlapEvents(false);
	AvatarMesh->SetCanEverAffectNavigation(false);
	AvatarMesh->SetRelativeRotation(FRotator(0.f, CommanderAvatarYawOffset, 0.f));

	// Anim-tick thrift, mirroring the fleet law (TASK-285): stop evaluating the
	// pose entirely while the commander is not rendered, and throttle the pose
	// rate when he is small on screen. SAFE unconditionally here for a stronger
	// reason than it is on units — this actor has NO gameplay behaviour keyed to
	// its pose at all: no root motion, no AnimNotify, no attack cadence, no
	// movement. The only thing that can lag is a visible idle.
	AvatarMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	AvatarMesh->bEnableUpdateRateOptimizations = true;

	// The war table, on the floor plane in front of the commander. Same
	// NoCollision / no-nav posture and the same Movable requirement (a Static
	// component refuses SetStaticMesh after BeginPlay).
	WarTableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarTableMesh"));
	WarTableMesh->SetupAttachment(SceneRoot);
	WarTableMesh->SetMobility(EComponentMobility::Movable);
	WarTableMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WarTableMesh->SetGenerateOverlapEvents(false);
	WarTableMesh->SetCanEverAffectNavigation(false);
	WarTableMesh->SetRelativeLocation(FVector(CommanderWarTableForwardOffset, 0.f, 0.f));

	// Visual contract: soft paths, never hard references. SK_Sorcerer ships
	// today; SM_WarTable does not exist yet (TASK-556 authors it, TASK-566
	// imports it) and this class is written to survive its absence.
	AvatarMeshAsset = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(CommanderAvatarMeshPath));
	WarTableMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(CommanderWarTableMeshPath));
	AvatarIdleAnimAsset = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(CommanderAvatarIdleAnimPath));

	// ⛔⛔ AvatarAnimClassAsset IS DELIBERATELY LEFT EMPTY, AND ⛔ THE EMPTINESS IS
	// THE FIX (TASK-591 / SC-§35) — it is ⛔ NOT an oversight and ⛔ must not be
	// "completed". This comment exists precisely BECAUSE the three assignments
	// above make a missing fourth one look like a slip: the last AnimBlueprint
	// assigned here (ABP_Footman) cost 1,806 Blueprint runtime errors in 49 s,
	// because its graph resolves its owner as a Pawn and this actor is an AActor.
	// ⛔ Anything ever assigned here MUST be an ABP whose graph makes NO Pawn
	// assumption about its owner. See the field's header doc for both failure
	// cases; the commander's idle now comes from AvatarIdleAnimAsset, which runs
	// no graph at all.
}

void ACommanderNpc::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Editor preview only — SILENT. OnConstruction re-runs on every property
	// tweak, so warning here would spam the log while an artist nudges the
	// anchor. The runtime resolve at BeginPlay is the one that reports.
	ResolveAvatarMesh(/*bWarnIfMissing=*/ false);
	ResolveWarTableMesh(/*bWarnIfMissing=*/ false);
}

void ACommanderNpc::BeginPlay()
{
	Super::BeginPlay();

	// Runtime resolve for instances created before the assets existed (the
	// normal case for SM_WarTable until TASK-566 imports it). Each miss is
	// logged exactly once and leaves an invisible-but-functional NPC — the
	// proximity gate and EnemyRevealCost do not depend on any mesh.
	ResolveAvatarMesh(/*bWarnIfMissing=*/ true);
	ResolveWarTableMesh(/*bWarnIfMissing=*/ true);

	// ⛔⛔ ANIMATION IS APPLIED HERE AND ⛔ NOWHERE ELSE — in particular ⛔ NEVER
	// from OnConstruction (TASK-591 (d)). OnConstruction re-runs on every property
	// tweak in the editor, so a construction-script side effect that STARTS
	// PLAYBACK is editor-time behaviour nobody asked for plus a package-dirtying
	// risk. This single call site is also what makes the ladder's fall-through log
	// structurally once-per-spawn: there is no second door to it. Ordered AFTER
	// ResolveAvatarMesh because every rung needs the skeletal mesh already set.
	ApplyAvatarAnimation();
}

void ACommanderNpc::InitCommanderNpc(ETeamId InTeam)
{
	// THE TEAM PUSH. ACastle threads its OWN Team here (TASK-562); this actor
	// never derives the answer (class doc: a nearest-castle search would
	// silently pick the enemy castle on the mirrored side).
	CommanderTeam = InTeam;

	// Grep-able, two lines per match. The diagnostic for the one thing that can
	// go wrong across the 559/562 seam: a commander reporting the WRONG team
	// stands in the right hall but refuses its own player's map.
	const FVector Location = GetActorLocation();
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("[%s] CommanderNpcInit team=%s P=(%.0f, %.0f, %.0f) interactRadius=%.0f revealCost=%d"),
		*GetNameSafe(this), (InTeam == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"),
		Location.X, Location.Y, Location.Z, InteractRadius, EnemyRevealCost);
}

bool ACommanderNpc::IsPlayerInRange(const FVector& PlayerLocation) const
{
	// 2D (XY), squared — the house arena metric (AGoldNode::FindBestMineFor,
	// AAncientGround, the miner's arrival test). Z is ignored so the raised
	// interior floor never makes the table unreachable. A non-positive radius
	// answers false for every point (an empty disc contains nothing), which is
	// also how a designer disables the gate on an instance.
	if (InteractRadius <= 0.f)
	{
		return false;
	}

	const float DistSq = static_cast<float>(FVector::DistSquared2D(GetActorLocation(), PlayerLocation));
	return DistSq <= FMath::Square(InteractRadius);
}

ACommanderNpc* ACommanderNpc::FindCommanderNpcForTeam(UWorld* World, ETeamId Team)
{
	if (!World)
	{
		return nullptr; // no world (CDO/test context) — never a crash
	}

	for (TActorIterator<ACommanderNpc> It(World); It; ++It)
	{
		ACommanderNpc* Npc = *It;
		if (IsValid(Npc) && Npc->GetCommanderTeam() == Team)
		{
			return Npc; // exactly one per team by construction (one per castle)
		}
	}

	// No commander for this team: the castle's spawn was refused, the map has
	// no castles, or this is a fallback boot. The caller degrades to today's
	// behaviour (no map gate to satisfy) rather than crashing.
	return nullptr;
}

void ACommanderNpc::ResolveAvatarMesh(bool bWarnIfMissing)
{
	if (!AvatarMesh)
	{
		return; // no component (should be impossible past construction) — nothing to do
	}

	USkeletalMesh* Resolved = AvatarMeshAsset.LoadSynchronous();
	if (!Resolved)
	{
		// A cleared soft ref is a deliberate designer opt-out (the
		// AttackImpactEffect IsNull pattern) and is never worth a warning.
		if (bWarnIfMissing && !bWarnedMissingAvatarMesh && !AvatarMeshAsset.IsNull())
		{
			bWarnedMissingAvatarMesh = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] CommanderNpc: avatar mesh '%s' did not resolve — the commander is invisible; the war-map gate and EnemyRevealCost still work."),
				*GetNameSafe(this), *AvatarMeshAsset.ToString());
		}
		return;
	}

	// SetSkeletalMeshAsset is the current (non-deprecated) UE5 setter.
	AvatarMesh->SetSkeletalMeshAsset(Resolved);

	// ⛔ NO ANIMATION IS TOUCHED HERE, AND THAT IS DELIBERATE (TASK-591 (d)):
	// this function is SHARED with OnConstruction, i.e. the editor-preview path.
	// The whole anim ladder lives in ApplyAvatarAnimation and runs from BeginPlay
	// only. ⛔ Do not re-add an anim call to this function — it would put playback
	// back into the construction script, which is the thing (d) forbids.
}

void ACommanderNpc::ApplyAvatarAnimation()
{
	// ⛔⛔ THE THREE-RUNG PRECEDENCE LADDER (TASK-591 (c) / SC-§35). Every rung is
	// null-safe and the final rung is "do nothing", so the worst reachable outcome
	// is a commander standing in REF POSE — a purely COSMETIC downgrade. The
	// war-map proximity gate, InteractRadius and EnemyRevealCost do not depend on
	// the pose in any way, which is what makes ref pose an acceptable floor.
	if (!AvatarMesh)
	{
		return; // no component (should be impossible past construction) — nothing to do
	}

	// ── RUNG 1 ─ an explicitly-assigned AnimBlueprint wins, if a designer ever
	// sets one. ⛔ DORMANT BY DESIGN: AvatarAnimClassAsset ships EMPTY (see the
	// constructor and the field's header doc), so this branch runs on no shipped
	// commander. It is kept as the escape hatch for a future, purpose-built
	// ABP_Commander whose graph makes NO Pawn assumption about its owner.
	if (UClass* const AnimClass = AvatarAnimClassAsset.LoadSynchronous())
	{
		AvatarMesh->SetAnimInstanceClass(AnimClass);
		return;
	}

	// ── RUNG 2 ─ the single-node idle: the sanctioned shape for a NON-PAWN prop
	// that must breathe (SC-§35 item 2). Four things must hold, and each failure
	// falls through to rung 3 rather than being papered over.
	USkeletalMesh* const AvatarSkeletalMesh = AvatarMesh->GetSkeletalMeshAsset();
	if (!AvatarSkeletalMesh)
	{
		// No body at all. ResolveAvatarMesh has ALREADY reported that, once, at
		// Warning — a second line here would be the same fact said twice.
		return;
	}

	if (AvatarIdleAnimAsset.IsNull())
	{
		// A cleared soft ref is a deliberate designer opt-out (the
		// AttackImpactEffect IsNull pattern) and is never worth a log line.
		return;
	}

	UAnimSequence* const IdleSequence = AvatarIdleAnimAsset.LoadSynchronous();
	if (!IdleSequence)
	{
		// ⚠️ Log, ⛔ never Warning, and structurally ONCE — this function has a
		// single call site, in BeginPlay, so there is no per-frame path to it. A
		// ref-pose commander is a cosmetic downgrade, not a fault.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] CommanderNpc: idle sequence '%s' did not resolve — the commander stands in REF POSE (cosmetic only; the war-map gate and EnemyRevealCost are unaffected)."),
			*GetNameSafe(this), *AvatarIdleAnimAsset.ToString());
		return;
	}

	// ⛔⛔ THE SKELETON CHECK IS OURS TO MAKE — ⛔ IT IS NOT THE ENGINE'S, AND THAT
	// IS READ AT THE SOURCE RATHER THAN ASSUMED. UE 5.8's
	// UAnimSingleNodeInstance::SetAnimationAsset nulls the asset only when the
	// sequence has NO skeleton AT ALL; it does ⛔ NOT compare the sequence's
	// skeleton against the component's. An incompatible clip would therefore be
	// handed to the pose evaluator instead of refused. TASK-591 requires a
	// mismatch to degrade to ref pose and ⛔ never to crash, so the comparison
	// happens HERE, before PlayAnimation is ever called.
	//
	// ⚠️ Deliberately an EXACT identity test, and deliberately ⛔ NOT either of
	// the engine's skeleton-compatibility APIs — BOTH were considered and each
	// was DECLINED for its own reason (qa/TASK-592.md WARN-1):
	//   (a) USkeleton::IsCompatibleForEditor — UNAVAILABLE here: all three
	//       overloads sit inside Skeleton.h's #if WITH_EDITORONLY_DATA block,
	//       and this path runs at runtime in a non-editor build.
	//   (b) USkeleton::IsCompatibleMesh — AVAILABLE at runtime (it is declared
	//       OUTSIDE the editor-only block, and USkeletalMesh derives from
	//       USkinnedAsset, so IdleSequence->GetSkeleton()->IsCompatibleMesh(
	//       AvatarSkeletalMesh) would compile in a shipping build) — and STILL
	//       DECLINED: it is deliberately PERMISSIVE (bone-name matching plus an
	//       optional parent-chain walk against a percentage threshold — read at
	//       the engine source, not assumed), which is the OPPOSITE of the
	//       conservative direction this guard chose on purpose.
	// Identity is the CONSERVATIVE direction: a merely "compatible" skeleton is
	// refused and falls to ref pose, which is the safe way to be wrong. The
	// whole rigged fleet is retargeted onto the single SK_Footman_Skeleton, so
	// identity is the EXPECTED match and the conservatism is expected to cost
	// nothing. ⛔ Do NOT "complete" this guard by swapping the identity test for
	// IsCompatibleMesh — the permissive direction is a RECORDED REJECTION, not a
	// gap left for want of a runtime API.
	const USkeleton* const MeshSkeleton = AvatarSkeletalMesh->GetSkeleton();
	if (!MeshSkeleton || IdleSequence->GetSkeleton() != MeshSkeleton)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] CommanderNpc: idle sequence '%s' is on skeleton '%s' but avatar mesh '%s' uses skeleton '%s' — REF POSE (cosmetic only; the war-map gate and EnemyRevealCost are unaffected)."),
			*GetNameSafe(this), *AvatarIdleAnimAsset.ToString(),
			*GetNameSafe(IdleSequence->GetSkeleton()),
			*GetNameSafe(AvatarSkeletalMesh), *GetNameSafe(MeshSkeleton));
		return;
	}

	// ✅ SC-§35 item 2's sanctioned shape, both halves written out. PlayAnimation
	// re-asserts single-node mode internally, so the explicit SetAnimationMode is
	// THE LAW'S SHAPE MADE VISIBLE rather than a functional necessity — it is kept
	// because the next reader must be able to see, in one line and without opening
	// the engine, that this component runs NO ANIM GRAPH and therefore asks
	// NOTHING about its owner's class.
	AvatarMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	AvatarMesh->PlayAnimation(IdleSequence, /*bLooping=*/ true);

	// ── RUNG 3 ─ ref pose. It is every early return above, and it is deliberately
	// NOT an else-branch: there is no "apply ref pose" call to make. Doing nothing
	// IS the rung.
}

void ACommanderNpc::ResolveWarTableMesh(bool bWarnIfMissing)
{
	if (!WarTableMesh)
	{
		return; // no component (should be impossible past construction) — nothing to do
	}

	UStaticMesh* Resolved = WarTableMeshAsset.LoadSynchronous();
	if (!Resolved)
	{
		if (bWarnIfMissing && !bWarnedMissingWarTableMesh && !WarTableMeshAsset.IsNull())
		{
			bWarnedMissingWarTableMesh = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] CommanderNpc: war table mesh '%s' did not resolve — no table prop; the war map itself is a UI widget and is UNAFFECTED (TASK-556 authors the mesh, TASK-566 imports it)."),
				*GetNameSafe(this), *WarTableMeshAsset.ToString());
		}
		return;
	}

	WarTableMesh->SetStaticMesh(Resolved);
}
