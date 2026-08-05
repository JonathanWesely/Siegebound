// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SummonedUnit.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/DataTable.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NavFilters/NavigationQueryFilter.h" // TASK-349 loop-2 B2: GetQueryFilter for the filter-aware march-goal projection
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h" // TASK-349 loop-2 B2: UNavigationSystemV1::ProjectPointToNavigation (filter overload)
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeCombatStatics.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeMeshJuiceComponent.h"
#include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channels + ASiegeUnitAIController (complete types for the gating stamp)
#include "Siegebound/SiegePlayerController.h" // W1 TASK-275: reads the latched Shield Wall command (GetCurrentCommand/HasIssuedCommand); TASK-344: resolves the live group (FindUnitGroup) — complete type needed for the const calls
#include "Siegebound/SiegeStuckStatics.h" // TASK-532: the pure stall ladder + LogSiegeStuck (also reached via SummonedUnit.h, which needs the complete types for its members — explicit per IWYU)
#include "Siegebound/UnitCommand.h" // TASK-344: FSiegeUnitGroup complete type (UpdateStateGrouped reads its zones/type) — explicit include, not just via the controller header
#include "TimerManager.h"

namespace
{
	//~ M7 §6 juice/feel soft-ref paths (TASK-158/179) — null-safe, resolved through
	//~ USiegeFeedbackLibrary; the assets arrive later (TASK-174/180) and no-op until then.
	const TCHAR* GoldBurstVFXPath = TEXT("/Game/VFX/NS_GoldBurst");   // TASK-158: coin burst on unit death
	const TCHAR* UnitSpawnSoundPath = TEXT("/Game/Audio/S_UnitSpawn");        // TASK-179
	const TCHAR* ProjectileFireSoundPath = TEXT("/Game/Audio/S_ProjectileFire"); // TASK-179

	//~ TASK-159/165: M7 shared-locomotion fallback AnimBP. MCP can't author per-unit
	//~ AnimBlueprints without freezing the editor, so any rigged unit that lacks its own
	//~ /Game/Characters/ABP_<CardID> falls back to this ONE shared ABP. Every rigged unit
	//~ shares the SK_Footman_Skeleton / SiegeBiped rig, so ABP_Footman's velocity-driven
	//~ idle/walk locomotion drives any of them. Points at the _C generated-class path.
	//~ Swap this single constant to a dedicated ABP_SiegeUnit once one is authored.
	const TCHAR* SharedLocomotionAbpPath = TEXT("/Game/Characters/ABP_Footman.ABP_Footman_C");

	/** Height above a unit's origin for its floating damage number (roughly over the head). */
	constexpr float UnitDamageNumberHeightZ = 110.f;
}

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

	/**
	 *  Shield Wall HOLD arrival tolerance (W1 TASK-275): a unit within this 2D distance of
	 *  the hold point, with no enemy inside the hold disc, is considered "gathered" and
	 *  holds (EnterIdle) instead of re-issuing a move. Comfortably above the
	 *  StructureMoveAcceptanceRadius so a MoveToLocation that acceptance-stops reads as
	 *  arrived, and generous enough that a loose swarm settles around the point. Impl
	 *  detail (like StructureMoveAcceptanceRadius), not a GDD stat.
	 */
	constexpr float HoldArrivalTolerance = 150.f;

	/**
	 *  TASK-532 — upper bound on ONE stuck-watchdog delta. A CLAMP, deliberately NOT an
	 *  FSiegeStuckTuning field: NAV-§7 pins that struct's eight fields as the ONLY tunables
	 *  this feature adds, and this is a safety rail rather than a feel knob.
	 *
	 *  Why it is needed: the watchdog's delta comes from the WORLD CLOCK (the miner's
	 *  StateCheckInterval-0 seal forbids the alternative), but UpdateState does not run
	 *  while a unit is spell-frozen — ApplyFreeze clears StateTimerHandle for the whole
	 *  freeze (TASK-099, up to 4 s). Without this, the FIRST poll after EndSpellFreeze
	 *  would hand the ladder a multi-second delta and could jump straight to Abandon,
	 *  skipping rungs. One state poll is 0.25 s, so anything beyond this is a GAP in the
	 *  poll, not elapsed stall time, and must not be charged to the unit as stall.
	 */
	constexpr float MaxStuckDeltaSeconds = 1.f;

	/**
	 *  TASK-532 — the pinned NAV-§7 `action=` token for a rung. File-local on purpose:
	 *  nothing outside this translation unit logs a rung, and ESiegeStuckAction is
	 *  deliberately NOT a UENUM (NAV-§8), so there is no reflected name to fall back on.
	 */
	const TCHAR* StuckActionToken(ESiegeStuckAction Action)
	{
		switch (Action)
		{
		case ESiegeStuckAction::Sidestep:       return TEXT("Sidestep");
		case ESiegeStuckAction::WidenAndRepath: return TEXT("WidenAndRepath");
		case ESiegeStuckAction::Abandon:        return TEXT("Abandon");
		default:                                return TEXT("None");
		}
	}
}

ASummonedUnit::ASummonedUnit()
{
	// state machine runs on a ~0.25 s timer (TASK-004 spec) — never per-tick.
	// Tick exists SOLELY for the attack-lunge visual (TASK-020): it starts
	// disabled, is enabled only while a <= 0.8×Cadence lunge cycle animates,
	// and is re-disabled at every cycle end. Gameplay logic never ticks.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// AI-driven navmesh walker: the AI controller possesses us whether the unit
	// was placed in a level or spawned by the card play (TASK-007). TASK-349:
	// ASiegeUnitAIController — behaviorally a plain AAIController whose sole
	// addition is the public team nav-filter setter (SiegeNavAreas.h), which
	// ApplyTeamGatingProfile pushes so this unit never paths into the enemy
	// castle's interior (CONVENTIONS "Castle 3× HOLLOW" team-gating law).
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ASiegeUnitAIController::StaticClass();

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

	// Overhead health bar (TASK-130 castle-parity REBUILD): one screen-space, team-tinted
	// PUSH bar per unit. The component binds this unit's OnHPChanged delegate (seed-then-bind)
	// and pushes the team tint; the widget class is soft-resolved null-safe at its own BeginPlay
	// (WBP_CombatantHealthBar, TASK-131). AMinerUnit inherits this instance. NO poll timer —
	// updates arrive when the unit broadcasts OnHPChanged.
	HPBarWidget = CreateDefaultSubobject<UCombatantHealthBarComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(GetCapsuleComponent());

	// M7 skeletal runtime visual (TASK-159): OPTIONAL, empty + hidden by default.
	// The capsule owns all collision — like the static VisualMesh, this must neither
	// collide nor carve the navmesh; it only becomes visible if SK_<CardID> resolves
	// (ResolveSkeletalVisual). The static VisualMesh backs the ghost + the null-safe
	// fallback; this backs the animated runtime (CONVENTIONS SkeletalVisualMesh contract).
	SkeletalVisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalVisualMesh"));
	SkeletalVisualMesh->SetupAttachment(GetCapsuleComponent());
	SkeletalVisualMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SkeletalVisualMesh->SetGenerateOverlapEvents(false);
	SkeletalVisualMesh->SetCanEverAffectNavigation(false);
	SkeletalVisualMesh->SetVisibility(false);

	// M7.6 Phase-2 URO / anim-tick perf (TASK-285, CONVENTIONS "Arena 10× scale-up &
	// LOD/perf" SK-unit URO law): at 10× field scale most of the ~10× unit fleet is
	// off-screen at any moment, so make an unrendered unit's animation cost near-zero.
	// OnlyTickPoseWhenRendered stops evaluating the pose entirely while the mesh is not
	// rendered (accepted off-screen anim pop at the gameplay cam), and URO throttles the
	// pose-tick RATE for distant/rarely-rendered units that ARE visible.
	//
	// SAFE unconditionally: these flags touch ONLY this cosmetic SkeletalVisualMesh
	// component's POSE tick. Every gameplay-critical path is decoupled from the pose —
	// movement is CharacterMovementComponent + AAIController MoveTo (nav), never root
	// motion (none in this TU); aggro/target acquisition is distance math on the
	// StateTimerHandle→UpdateState loop; attack cadence + damage delivery are the
	// AttackTimerHandle→PerformAttack timer (ApplyDamage / FireProjectileAt applied
	// DIRECTLY, never via an AnimNotify — there are none). So an off-screen unit still
	// marches, acquires, and hits on schedule; only its visible pose lags. This is a
	// SEPARATE subobject from the Character's animation Mesh (unused here), which makes
	// the decoupling structural. NOTE: SetVisibleInRayTracing(false) is the reserved
	// W2/W3 EMERGENCY lever and is deliberately NOT applied here (TASK-285 scope).
	SkeletalVisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	SkeletalVisualMesh->bEnableUpdateRateOptimizations = true;

	// §6 juice components (TASK-154/155): the shared hit-flash + transform-juice, added
	// once here so AMinerUnit inherits them. The flash gathers meshes at its BeginPlay and
	// is driven from TakeDamage; the juice is pointed at the active visual mesh + squashed
	// once at LoadStatsAndStart. Both null-safe and inert until triggered.
	HitFlashComponent = CreateDefaultSubobject<USiegeHitFlashComponent>(TEXT("HitFlashComponent"));
	MeshJuiceComponent = CreateDefaultSubobject<USiegeMeshJuiceComponent>(TEXT("MeshJuiceComponent"));

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

	// TASK-349 (CONVENTIONS "Castle 3× HOLLOW" team-gating): capsule object-type
	// stamp + AI nav filter for the ACTUAL Team, same team-set discipline as the
	// material apply above. PossessedBy re-applies for late AutoPossessAI
	// possession; a post-BeginPlay InitUnit team update re-applies again.
	ApplyTeamGatingProfile();

	LoadStatsAndStart();
}

void ASummonedUnit::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// TASK-349: AutoPossessAI possession can land AFTER BeginPlay (the miner's
	// controller poll exists for exactly that gap), and the nav-filter half of the
	// gating profile needs the live controller — re-apply here. Idempotent; Team
	// is authoritative by possession time on every spawn path (deferred spawns run
	// InitUnit before FinishSpawning; placed units carry their instance value).
	ApplyTeamGatingProfile();
}

void ASummonedUnit::ApplyTeamGatingProfile()
{
	// PHYSICAL lane: re-type the capsule's collision OBJECT channel by team.
	// SetCollisionObjectType changes the object type ONLY — the capsule keeps its
	// Pawn-profile response matrix, so every response-based query in the codebase
	// (ActorGetDistanceToCollision on ECC_Pawn, pawn-vs-pawn blocking, floor
	// sweeps) behaves byte-identically. What changes: the enemy castle's
	// GateBlockerVolume (Block on this channel) now stops this body at the gate,
	// and the own castle's blocker (Ignore) lets it through.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionObjectType(SiegeTeamObjectChannel(Team));
	}

	// PATHING lane: the possessing controller adopts the team's query filter as
	// its default, so every existing MoveToActor/MoveToLocation (FilterClass null,
	// untouched call sites) excludes the ENEMY castle's interior area. Null-safe:
	// no controller yet (pre-possession BeginPlay) or a non-Siege controller (a BP
	// pinning plain AAIController) simply gets no filter — pre-feature pathing,
	// with the physical gate still holding the line.
	if (ASiegeUnitAIController* SiegeAI = Cast<ASiegeUnitAIController>(GetController()))
	{
		SiegeAI->ApplyTeamNavigationFilter(Team);
	}
}

void ASummonedUnit::ApplyTeamMaterial()
{
	// TASK-044 — CONVENTIONS Team contract: the bot reuses the player's BP_Unit_*
	// assets (authored with the Blue placeholder material); this overrides slot 0 by
	// the ACTUAL Team so a Red-spawned unit reads red with no Red BP duplicate. Blue
	// re-applies the identical MI_TeamColor_Blue, so Blue-side visuals are unchanged.
	// TASK-159: slot 0 is written on the ACTIVE visual — the skeletal mesh once the M7
	// swap took, else the static VisualMesh (the swap contract's "recolor targets
	// SkeletalVisualMesh slot 0"). The SK mesh keeps the same [TeamRegion, <CardID>PBR]
	// two-slot contract as its SM source, so slot-0 recolor is identical on either path.
	UMeshComponent* ActiveMesh = GetActiveVisualMesh();
	if (!ActiveMesh)
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
		ActiveMesh->SetMaterial(0, ResolvedTeamMat);
	}
}

UMeshComponent* ASummonedUnit::GetActiveVisualMesh() const
{
	// The skeletal runtime once the M7 swap took (TASK-159), else the static VisualMesh
	// — both are UMeshComponent, so team recolor (SetMaterial) and the spawn-squash
	// target (a USceneComponent) work uniformly on either.
	if (bUsingSkeletalVisual && SkeletalVisualMesh)
	{
		return SkeletalVisualMesh;
	}
	return VisualMesh;
}

void ASummonedUnit::ResolveSkeletalVisual()
{
	// M7 skeletal swap (TASK-159), the class of change that tripped TASK-110 — so:
	// complete-type includes for USkeletalMeshComponent / USkeletalMesh / UAnimInstance
	// are pulled in at the top of this TU, and every step is null-guarded.
	if (bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone())
	{
		return; // already swapped, no component, or no CardID to compose from — keep the static VisualMesh
	}

	// Compose /Game/Characters/SK_<CardID> from the CardID (mirrors the static
	// /Game/Meshes/SM_<CardID> string law). A unit with no rig yet (the normal
	// pre-batch case) resolves nullptr here and silently keeps the static mesh —
	// deliberately NOT logged (60+ un-rigged units must not spam).
	const FString CardIdString = CardID.ToString();
	const FString SkPath = FString::Printf(TEXT("/Game/Characters/SK_%s.SK_%s"), *CardIdString, *CardIdString);
	const TSoftObjectPtr<USkeletalMesh> SkSoft{ FSoftObjectPath(SkPath) };
	USkeletalMesh* SkeletalAsset = SkSoft.LoadSynchronous();
	if (!SkeletalAsset)
	{
		return; // no SK_<CardID> — byte-for-byte today's static-mesh behavior
	}

	// Skeletal runtime IS available: swap it in. SetSkeletalMeshAsset is the current
	// (non-deprecated) setter in UE5. The mesh carries its own [TeamRegion, <CardID>PBR]
	// materials; the team recolor below (via the LoadStatsAndStart re-apply) overrides slot 0.
	SkeletalVisualMesh->SetSkeletalMeshAsset(SkeletalAsset);

	// FLOAT-FIX v2 (DIAG-floating-units SESSION-3, SYSTEMIC): ground the skeletal mesh by
	// DERIVING its offset from the capsule + the mesh's OWN bounds — never from a per-BP
	// hand-authored Z. The SK feet sit at the mesh pivot (feet-origin: local min-Z ≈ 0), so a
	// component left at capsule-center renders the whole body one capsule-half-height ABOVE the
	// grounded capsule (Archer +90 / Ogre +145 / Wizard +~90). CharacterMovement floors the
	// CAPSULE, so its bottom (actor-relative Z = −HalfHeight) sits ON the ground; we want the
	// mesh's LOWEST point to land there: componentZ + meshLocalMinZ == −HalfHeight, i.e.
	// componentZ = −HalfHeight − meshLocalMinZ.
	//
	// WHY v1 (TASK-259) was not enough: it copied the STATIC VisualMesh's BP-authored Z
	// (VisualMeshBaseRelativeLocation) onto this component — which merely RELOCATED the "each BP
	// must hand-author −HalfHeight" trap from this component onto the static one. BP_Unit_Wizard's
	// static VisualMesh.Z was never offset (TASK-302 §5/§6 + the TASK-304 authoring spec both
	// assumed the fix was automatic), so the copy propagated 0 and the Wizard floated exactly like
	// the old Archer/Ogre. Deriving from the CAPSULE closes the trap permanently: the Wizard, the
	// 11 feet-origin Meshy rebuilds, AND any future unit ground with ZERO per-BP capsule/Z
	// authoring — even if a BP never resizes the capsule (feet still land on the floored capsule
	// bottom; capsule size then only affects collision, not the visual float).
	//
	// NO REGRESSION for the current fleet: every rigged unit is feet-origin (SK bounds bottom ≈ Z0
	// — DIAG table: Footman origin.z 89.85 / extent.z 89.83, Archer 90.00 / 89.95, Ogre 143.93 /
	// 144.09), so meshLocalMinZ ≈ 0 and GroundedLoc.Z resolves to −HalfHeight = their existing
	// authored −90 / −90 / −145 (byte-equivalent within <0.2 uu, invisible). A NON-feet-origin
	// future mesh is handled for free by using the actual bounds; RelativeScale3D.Z keeps it
	// correct under any per-BP mesh scale. GetCapsuleComponent() is the ACharacter root (never
	// null — the if is defense-in-depth). Only Z is touched; the authored X/Y is preserved. The
	// static VisualMesh / lunge base (VisualMeshBaseRelativeLocation) / blockout-fallback paths
	// are deliberately untouched (the static mesh is hidden once this SK swap takes).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		const FBoxSphereBounds SkBounds = SkeletalAsset->GetBounds(); // ref-pose local bounds
		const float MeshMinZ = (SkBounds.Origin.Z - SkBounds.BoxExtent.Z) * SkeletalVisualMesh->GetRelativeScale3D().Z;
		FVector GroundedLoc = SkeletalVisualMesh->GetRelativeLocation(); // keep authored X/Y
		GroundedLoc.Z = -HalfHeight - MeshMinZ; // mesh's lowest point → capsule bottom (= floor)
		SkeletalVisualMesh->SetRelativeLocation(GroundedLoc);
	}

	// FACING-FIX (TASK-326/327, SYSTEMIC): the OTHER HALF of the same authoring trap the grounding
	// block above closes. Z was derived in C++ so no BP has to hand-author it; YAW was left to the
	// BP — and the three units whose SkeletalVisualMesh was never authored at all (Archer, Ogre,
	// Wizard) sat at the constructor default (0,0,0) on BOTH. Yaw 0 leaves the mesh's baked forward
	// on the actor's RIGHT, i.e. the unit walks SIDEWAYS — and because facing comes from
	// bOrientRotationToMovement and attack aim is the same actor rotation, the 90° error is rigid
	// across marching, attacking and death alike.
	//
	// The fleet bakes ONE forward: all 12 units are rigged through Tools/ArtPipeline/rig_character.py
	// (front on Blender -Y) onto the ONE shared SK_Footman_Skeleton, which arrives as UE-local +Y
	// (measured in-engine, TASK-326: 12/12 on the shared skeleton; raw SkeletalMeshActors at actor
	// yaw 0 all face a +Y camera). Actor forward is +X and Rot(θ)·(0,1,0) = (-sinθ, cosθ) = (1,0,0)
	// ⇒ θ = -90 — the same constant, for the same reason, as ASiegePlayerController::GhostYawOffset,
	// which is why the placement ghost has never mis-faced even for units whose BP yaw is 0.
	//
	// ABSOLUTE assignment, NEVER additive: `+=` would take the 9 correctly-authored units from -90
	// to -180 and regress the whole fleet. Overwriting is the ONLY formulation that is a no-op for
	// the 9 (they measure exactly (0,-90,0), so this writes back a component-wise identical rotator
	// — no transform delta, no render/bounds/attachment change) AND the fix for the 3. Only .Yaw is
	// touched; authored pitch/roll are read and written back untouched, exactly as the grounding
	// block preserves authored X/Y. Un-rigged units returned at the guards above (:266 / :279) and
	// never reach here; SkeletalVisualMesh is non-null by construction past the :266 guard. The
	// static VisualMesh, the lunge base (VisualMeshBaseRelativeLocation), the juice/flash paths and
	// the placement ghost are all deliberately untouched.
	FRotator FacingRot = SkeletalVisualMesh->GetRelativeRotation(); // keep authored pitch/roll
	FacingRot.Yaw = SkeletalVisualYawOffset;                        // fleet forward: mesh +Y → actor +X
	SkeletalVisualMesh->SetRelativeRotation(FacingRot);

	// AnimClass resolution (TASK-159 + shared-ABP fallback, TASK-165 rig-import chain):
	//   1. Prefer a per-unit /Game/Characters/ABP_<CardID> (the _C generated-class path) —
	//      future dedicated ABPs still take priority the moment they're authored.
	//   2. ELSE fall back to the ONE shared SharedLocomotionAbpPath (ABP_Footman). MCP can't
	//      create per-unit AnimBlueprints without freezing the editor, so this M7 shared
	//      locomotion drives every rigged unit off the common SiegeBiped rig until per-unit
	//      ABPs exist (all units share SK_Footman_Skeleton, so its idle/walk applies).
	//   3. ELSE (NEITHER resolves) leave the skeletal mesh with no anim instance — it shows
	//      in its ref pose, null-safe, never a crash (matches the soft-ref discipline above).
	const FString AbpPath = FString::Printf(TEXT("/Game/Characters/ABP_%s.ABP_%s_C"), *CardIdString, *CardIdString);
	const TSoftClassPtr<UAnimInstance> PerUnitAbpSoft{ FSoftObjectPath(AbpPath) };
	UClass* AnimClass = PerUnitAbpSoft.LoadSynchronous();
	if (!AnimClass)
	{
		const TSoftClassPtr<UAnimInstance> SharedAbpSoft{ FSoftObjectPath(FString(SharedLocomotionAbpPath)) };
		AnimClass = SharedAbpSoft.LoadSynchronous();
	}
	if (AnimClass)
	{
		SkeletalVisualMesh->SetAnimInstanceClass(AnimClass);
	}
	// TASK-165: remember the locomotion ABP so RestoreLocomotionAnim can swap it back after a
	// single-node attack clip. Null when neither ABP resolved (ref-pose case) — the attack
	// trigger is gated on this being set so it can always cleanly return to locomotion.
	LocomotionAnimClass = AnimClass;

	// TASK-165: resolve A_<CardID>_{Attack,Death} now that the rig took (mirrors the SK/SM path).
	CacheActionAnimations();

	// Make the skeletal the runtime visual, hide the static (the ghost still resolves
	// the static SM_<CardID> — CONVENTIONS parity rule, unchanged).
	SkeletalVisualMesh->SetVisibility(true);
	if (VisualMesh)
	{
		VisualMesh->SetVisibility(false);
	}
	bUsingSkeletalVisual = true;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASummonedUnit '%s': skeletal runtime SK_%s active (M7 TASK-159) — static VisualMesh hidden; the placement ghost still uses SM_%s."),
		*GetNameSafe(this), *CardIdString, *CardIdString);
}

void ASummonedUnit::CacheActionAnimations()
{
	// TASK-165 attack/death anim trigger (CODE-ONLY — no ABP/montage asset edit, avoiding the
	// AnimBlueprint MCP minefield): resolve A_<CardID>_{Attack,Death} by null-safe soft path
	// composed from the CardID (mirrors the SK_<CardID> / SM_<CardID> string law in
	// ResolveSkeletalVisual). Called only after the skeletal swap took, so un-rigged units never
	// run this. Brace-init the TSoftObjectPtr locals (the vexing-parse guard, CONVENTIONS).
	// A missing clip caches nullptr and the matching trigger no-ops — never a crash.
	if (CardID.IsNone())
	{
		return; // no CardID to compose from — leave both clips null (trigger no-ops)
	}

	const FString CardIdString = CardID.ToString();

	const FString AttackPath = FString::Printf(TEXT("/Game/Characters/Anims/A_%s_Attack.A_%s_Attack"), *CardIdString, *CardIdString);
	const TSoftObjectPtr<UAnimSequence> AttackSoft{ FSoftObjectPath(AttackPath) };
	CachedAttackAnim = AttackSoft.LoadSynchronous();

	const FString DeathPath = FString::Printf(TEXT("/Game/Characters/Anims/A_%s_Death.A_%s_Death"), *CardIdString, *CardIdString);
	const TSoftObjectPtr<UAnimSequence> DeathSoft{ FSoftObjectPath(DeathPath) };
	CachedDeathAnim = DeathSoft.LoadSynchronous();
}

void ASummonedUnit::PlaySkeletalAttackAnim()
{
	// TASK-165: single-node PlayAnimation of A_<CardID>_Attack over the locomotion ABP for the
	// clip's length, restored to locomotion by RestoreLocomotionAnim (timer / leaving Attack).
	// Null-safe / no-op unless the skeletal runtime, the resolved attack clip, AND a locomotion
	// ABP to return to are all present — the last guard guarantees we can always cleanly restore
	// (a rig with NO ABP at all keeps its ref pose rather than freezing on the attack frame).
	if (bDead || !bUsingSkeletalVisual || !SkeletalVisualMesh || !CachedAttackAnim || !LocomotionAnimClass)
	{
		return;
	}

	// PlayAnimation puts the component in single-node mode and plays the clip directly,
	// overriding the ABP for the duration (the code-only path — no montage slot needed).
	SkeletalVisualMesh->PlayAnimation(CachedAttackAnim, /*bLooping=*/ false);

	// restore locomotion when the clip ends. A faster next attack (before this fires) re-arms
	// the SAME timer and simply restarts the clip — reading as continuous swings — so the ABP
	// only returns once attacks actually stop. Floor at MinAttackCadence so a ~0-length clip
	// still yields a valid one-shot timer.
	const float RestoreDelay = FMath::Max(CachedAttackAnim->GetPlayLength(), MinAttackCadence);
	GetWorldTimerManager().SetTimer(AttackAnimRestoreTimerHandle, this, &ASummonedUnit::RestoreLocomotionAnim, RestoreDelay, /*bLoop=*/ false);
}

void ASummonedUnit::RestoreLocomotionAnim()
{
	// TASK-165: swap the locomotion ABP back onto SkeletalVisualMesh after a single-node attack
	// clip so velocity-driven idle/walk resume. Called by the restore timer AND on leaving Attack
	// (EnterAdvance/EnterIdle/FreezeAI). A dead unit is skipped — death HOLDS its final pose.
	GetWorldTimerManager().ClearTimer(AttackAnimRestoreTimerHandle);
	if (bDead || !bUsingSkeletalVisual || !SkeletalVisualMesh || !LocomotionAnimClass)
	{
		return;
	}

	// SetAnimInstanceClass reinitializes the ABP when coming OUT of single-node mode, and the
	// engine cheaply early-outs when the same ABP is already running in blueprint mode — so this
	// is safe to call on every Attack exit, not only after a clip actually played.
	SkeletalVisualMesh->SetAnimInstanceClass(LocomotionAnimClass);
}

float ASummonedUnit::PlaySkeletalDeathAnim()
{
	// TASK-165: play A_<CardID>_Death once (single-node, non-looping) — it freezes on the final
	// frame, the held death pose — and deliberately does NOT restore locomotion. Returns the
	// capped destroy-defer seconds; 0 means no skeletal death clip (caller destroys immediately).
	if (!bUsingSkeletalVisual || !SkeletalVisualMesh || !CachedDeathAnim)
	{
		return 0.f;
	}

	// death overrides any in-flight attack clip and its restore — it must HOLD, not return to walk
	GetWorldTimerManager().ClearTimer(AttackAnimRestoreTimerHandle);
	SkeletalVisualMesh->PlayAnimation(CachedDeathAnim, /*bLooping=*/ false);

	// small + capped (spec) so a long or mis-authored clip can never linger a corpse indefinitely
	return FMath::Min(CachedDeathAnim->GetPlayLength(), DeathAnimMaxHoldSeconds);
}

void ASummonedUnit::FinishDeathDestroy()
{
	// TASK-165: the death-anim hold elapsed — complete the deferred removal. The unit has been
	// logically dead (bDead) and non-colliding throughout the hold, so nothing targeted it.
	Destroy();
}

void ASummonedUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(MoveSpeedBuffTimerHandle); // TASK-042: no dangling buff-restore on a destroyed unit
	GetWorldTimerManager().ClearTimer(HealTimerHandle); // TASK-054: no dangling Support heal on a destroyed unit
	GetWorldTimerManager().ClearTimer(AuraDamageBuffTimerHandle); // TASK-055: no dangling aura-restore on a destroyed unit
	GetWorldTimerManager().ClearTimer(CombatBuffTimerHandle); // TASK-099: no dangling combat-buff restore on a destroyed unit
	GetWorldTimerManager().ClearTimer(SpellFreezeTimerHandle); // TASK-099: no dangling spell-freeze expiry on a destroyed unit
	GetWorldTimerManager().ClearTimer(AttackAnimRestoreTimerHandle); // TASK-165: no dangling anim restore on a destroyed unit
	GetWorldTimerManager().ClearTimer(DeathDestroyTimerHandle); // TASK-165: no dangling deferred death-destroy

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
	// TASK-349: the team gating profile (capsule channel + nav filter) tracks the
	// same late-team rule — a re-teamed body must swap gate sides too.
	if (HasActorBegunPlay())
	{
		ApplyTeamMaterial();
		ApplyTeamGatingProfile();
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

	// TASK-165 (rigged): end any in-flight attack clip and return to idle locomotion (velocity 0),
	// clearing the restore timer so a match-end-frozen unit never holds a mid-swing pose. No-op for
	// static-mesh units. Death is separate — a frozen unit is parked alive, not dead.
	RestoreLocomotionAnim();

	// end any active move-speed buff (TASK-042): clear its timer and restore the
	// base speed EXACTLY, so a match-end freeze leaves zero residual walk speed
	EndMoveSpeedBuff();

	// end any active War Banner damage aura (TASK-055): clear its timer and restore the
	// damage-output multiplier to EXACTLY 1.0, so a frozen unit deals only its base row
	// Damage (zero residual buff). Same drift-free discipline as the move-speed buff.
	EndAuraDamageBuff();

	// end any active Battle Cry combat buff (TASK-099): clear its timer and restore the
	// walk speed and attack cadence EXACTLY (its attack-timer re-arm is a no-op here —
	// the attack timer was already cleared above). Zero residual at match end.
	EndCombatBuff();

	// spell-freeze precedence handoff (TASK-099, M5 ruling 5): the match-end freeze
	// WINS. Wipe the spell-freeze state and its expiry timer so no expiry can ever
	// fire post-match — and EndSpellFreeze additionally refuses to resume a
	// bAIFrozen unit even if it were somehow invoked (triple guard with the
	// ApplyFreeze no-op on frozen units).
	GetWorldTimerManager().ClearTimer(SpellFreezeTimerHandle);
	bSpellFrozen = false;

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

	// STUCK WATCHDOG RESET — exit 4 of 4 (TASK-532, NAV-§3). A match-end-frozen unit is
	// parked forever, so a surviving lease would be inert here; it is cleared anyway
	// because "the lease is cleared on every exit" is a rule that only holds if it has no
	// exceptions to remember. Reset also drops the stall anchor, so the PIE readback of a
	// frozen unit never shows a half-climbed ladder.
	SidestepLeaseRemaining = 0.f;
	FSiegeStuckStatics::Reset(StuckState);
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

	// Capture the resting base speed ONCE per speed-buff EPISODE. TASK-099 widened the
	// episode to cover BOTH speed buffs (Rally and Battle Cry): the base is captured
	// only while NEITHER is active, so neither buff can ever capture the other's
	// already-buffed speed — that is exactly the drift the TASK-020 lunge lesson warns
	// against. The base is only ever a resting MaxWalkSpeed and is restored EXACTLY
	// when the LAST active speed buff ends (RefreshComposedMoveSpeed).
	if (!bMoveSpeedBuffActive && !bCombatBuffActive)
	{
		MoveSpeedBuffBaseSpeed = Movement->MaxWalkSpeed;
	}
	bMoveSpeedBuffActive = true;

	// No self-stacking: Rally's multiplier is written directly from the arg (never
	// compounded), so re-applying only REFRESHES the same-magnitude boost. The walk
	// speed composes with the Battle Cry combat buff (TASK-099): Base × Rally × Combat.
	MoveSpeedBuffMultiplier = Multiplier;
	RefreshComposedMoveSpeed();

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
	// idempotent: clear the timer either way, recompose only when a buff is live so the
	// base speed is written back EXACTLY once (zero residual drift, TASK-020 contract)
	GetWorldTimerManager().ClearTimer(MoveSpeedBuffTimerHandle);

	if (!bMoveSpeedBuffActive)
	{
		return;
	}
	bMoveSpeedBuffActive = false;
	MoveSpeedBuffMultiplier = 1.f;

	// TASK-099 composition: restores EXACTLY the shared episode base when the Battle
	// Cry combat buff is also inactive; otherwise drops to Base × CombatMoveMult so
	// ending Rally never clobbers a still-running Battle Cry (and vice versa).
	RefreshComposedMoveSpeed();
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

bool ASummonedUnit::CanReceiveDamageBoost() const
{
	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — the occupant predicate AAncientGround's
	// boost tick reads. Deliberately excludes the three units whose damage routes through
	// NEITHER compose point (ComputeOutputDamage / ApplyDetonation), so stacks on them would
	// be a number that changes nothing AND a boost bar that lies:
	//   • the Sorcerer — CanEverAttack() false (it also never self-boosts, but the ground
	//     skips empowerers before it ever gets here; this is the second, independent reason);
	//   • the Miner    — row Damage 0 (its combat machine is structurally sealed anyway);
	//   • the Cleric   — Profile Support, whose row Damage is a HEAL RATE, not attack damage
	//     (boosting it would silently buff healing through a damage mechanic).
	// A Sapper IS eligible (Siege profile, Damage 80): dying is how it attacks, and
	// ApplyDetonation composes the multiplier — see that function.
	return !bDead
		&& CanEverAttack()
		&& AttackDamage > 0.f
		&& Profile != ECardProfile::Support;
}

void ASummonedUnit::AddPermanentDamageStacks(int32 Stacks)
{
	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4). AUTHORITY IS BY CONSTRUCTION — the sole
	// gameplay caller is AAncientGround's boost tick, which is already gated on its PUSHED
	// bAuthoritativeBoost flag (the ground never reads HasAuthority(), because it is spawned
	// LOCALLY on clients from the replicated seed and keeps ROLE_Authority there). Adding a
	// HasAuthority() guard here would buy nothing in M8 P1, where units are server-only.
	if (Stacks <= 0)
	{
		return; // a zero grant (no friendly sorcerer in the ground) must not touch the bar
	}

	const int32 NewStacks = FMath::Clamp(PermanentDamageStacks + Stacks, 0, MaxPermanentDamageStacks);
	if (NewStacks == PermanentDamageStacks)
	{
		return; // already at the +400% cap: NO actual change, so NO broadcast (the OnHPChanged discipline)
	}

	PermanentDamageStacks = NewStacks;

	// broadcast on EVERY actual mutation — miss one and the boost bar is stale forever
	OnDamageBoostChanged.Broadcast(GetDamageBoostPercent());
}

void ASummonedUnit::ClearPermanentDamageStacks()
{
	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — the RESET path, so the broadcast is
	// UNCONDITIONAL (mirroring HandleDeath's unconditional 0-HP push): the rigged-death path
	// defers Destroy by up to DeathAnimMaxHoldSeconds, and without this push a boosted corpse
	// would hold a full boost bar for those 2 s. Also keeps the widget honest if a caller
	// clears an already-zero unit.
	PermanentDamageStacks = 0;
	OnDamageBoostChanged.Broadcast(GetDamageBoostPercent());
}

void ASummonedUnit::ApplyFreeze(float Seconds)
{
	// MATCH-END PRECEDENCE (M5 ruling 5): a match-end-frozen unit stays parked — a
	// spell freeze on top would be meaningless and its expiry a resume hazard, so
	// refuse outright. Dead/never-bound units have no AI to pause (a pre-bind unit's
	// state machine starts at LoadStatsAndStart — freezing "nothing" and then
	// resuming into a started machine would be incoherent). Non-positive Seconds is
	// a defensive no-op (FrostNova's EffectDuration is 4).
	if (bDead || bAIFrozen || !bStatsLoaded || Seconds <= 0.f)
	{
		return;
	}

	// refresh-not-stack (ruling 5): the single expiry timer is re-armed at
	// max(remaining, new) — a shorter re-freeze never TRIMS a longer one, and
	// nothing ever adds. GetTimerRemaining is only read while the timer is live
	// (it returns -1 otherwise).
	float RemainingFreeze = 0.f;
	if (GetWorldTimerManager().IsTimerActive(SpellFreezeTimerHandle))
	{
		RemainingFreeze = GetWorldTimerManager().GetTimerRemaining(SpellFreezeTimerHandle);
	}
	const float FreezeSeconds = FMath::Max(RemainingFreeze, Seconds);

	bSpellFrozen = true;

	// pause the brain and the cadence — the FreezeAI body, minus permanence: the
	// state (acquire) and attack timers stop, any in-flight lunge cancels to the
	// EXACT cached rest pose (TASK-020 zero-drift contract), and Support healing
	// stops (a frozen Cleric mends no one; the next post-freeze state check
	// re-acquires a heal target from scratch).
	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	StopAttackLunge();
	StopHealing();
	SupportHealTarget = nullptr;

	// stop the walk AND disable the movement component: StopMovement aborts the
	// in-flight path request, and MOVE_None makes the pause hold even against a
	// subclass drive that re-issues a move on its own timer (AMinerUnit's arrival
	// poll re-paths via EnsureWalkingToNode — its file is outside this task's set,
	// so the pause is enforced at the component the walk cannot bypass).
	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->DisableMovement();
	}

	// park Idle for the duration; the post-freeze state loop reacquires from
	// scratch (CurrentMoveGoal cleared so the resume re-paths cleanly). Note the
	// Idle park also drops CHARGE momentum on the first post-freeze state check
	// (TrackChargeMovement's Idle branch) — a frozen Cavalry rebuilds from zero.
	State = ESummonedUnitState::Idle;
	CurrentTarget = nullptr;
	CurrentMoveGoal = nullptr;

	// STUCK WATCHDOG RESET — ⚠️ THE DECLARED FIFTH EXIT (TASK-532). The spec names four
	// (EnterIdle, EnterAttack, HandleDeath, FreezeAI); this one was found by tracing the
	// lease and is REPORTED, NOT BURIED — QA rules on it.
	// Why it belongs: ApplyFreeze parks the unit Idle by writing State DIRECTLY, so it
	// never routes through EnterIdle — and EnterIdle's own `State == Idle` early-out (:2696)
	// means a later EnterIdle could not clean up after it either. It then clears
	// StateTimerHandle, so UpdateState stops running and the lease cannot even drain. A
	// spell-frozen unit would thaw with a live lease and steer to a SidestepGoal chosen
	// before the freeze, ignoring its standing body for up to SidestepLeaseSeconds. Same
	// rule as the other four, same two lines.
	SidestepLeaseRemaining = 0.f;
	FSiegeStuckStatics::Reset(StuckState);

	GetWorldTimerManager().SetTimer(SpellFreezeTimerHandle, this, &ASummonedUnit::EndSpellFreeze, FreezeSeconds, /*bLoop=*/ false);
}

void ASummonedUnit::EndSpellFreeze()
{
	// idempotent: clear the timer either way, resume only from a live spell freeze
	GetWorldTimerManager().ClearTimer(SpellFreezeTimerHandle);

	if (!bSpellFrozen)
	{
		return;
	}
	bSpellFrozen = false;

	// MATCH-END PRECEDENCE (M5 ruling 5): NEVER resume a match-end-frozen (or dead,
	// or somehow unbound) unit. FreezeAI already wipes this timer and flag, so this
	// gate is defense-in-depth — the ruling's "a spell-freeze expiry must never
	// resume a match-end-frozen actor", enforced even against a stray invocation.
	if (bDead || bAIFrozen || !bStatsLoaded)
	{
		return;
	}

	// resume: movement mode back to the class default (walking — the exact inverse
	// of ApplyFreeze's DisableMovement), then re-arm the state loop. Deliberately
	// NO synchronous UpdateState here (unlike LoadStatsAndStart): the first
	// post-freeze decision lands on the next timer tick (<= StateCheckInterval),
	// which (a) keeps AMinerUnit's seals intact — its StateCheckInterval is 0, so
	// this SetTimer CLEARS rather than schedules (the documented seal #1
	// mechanism) and no stray castle-bound Advance is ever issued to a miner,
	// whose own arrival poll re-issues the gold-node walk instead — and (b) never
	// applies damage synchronously from inside this expiry callback. Combat units
	// reacquire from scratch within a quarter second; a pre-freeze attack
	// cooldown resumes through EnterAttack's LastAttackTime gate (the freeze
	// wall-clock already covered it).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetDefaultMovementMode();
	}

	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ASummonedUnit::UpdateState, StateCheckInterval, /*bLoop=*/ true);
}

void ASummonedUnit::ApplyCombatBuff(float MoveSpeedMult, float AttackSpeedMult, float Seconds)
{
	// dead units are being destroyed and match-end-frozen units stay parked (the
	// ApplyMoveSpeedBuff guard); never-bound units would capture a pre-row walk
	// speed as the episode base and buff a cadence that is not bound yet — refuse
	// (the resolver only ever targets live, spawned units). A spell-FROZEN unit IS
	// buffable: freeze and buff are independent effects from opposite casters and
	// the buff's duration burns down in wall-clock time either way (ruling 6).
	if (bDead || bAIFrozen || !bStatsLoaded)
	{
		return;
	}

	// a non-positive duration is "end any active buff now" (defensive — Battle Cry
	// always passes EffectDuration = 8)
	if (Seconds <= 0.f)
	{
		EndCombatBuff();
		return;
	}

	// shared speed-buff EPISODE base (TASK-042/099 composition): capture the resting
	// walk speed only while NEITHER speed buff is active — see ApplyMoveSpeedBuff.
	if (!bMoveSpeedBuffActive && !bCombatBuffActive)
	{
		if (const UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			MoveSpeedBuffBaseSpeed = Movement->MaxWalkSpeed;
		}
	}

	// Self-refresh non-stacking (ruling 6): both multipliers are written DIRECTLY
	// from the args (never compounded off an already-buffed value) — a re-apply only
	// refreshes the same magnitudes and the window. Non-positive multipliers
	// sanitize to exactly 1 (a defensive no-op factor, never a zeroed speed).
	bCombatBuffActive = true;
	CombatBuffMoveSpeedMult = (MoveSpeedMult > 0.f) ? MoveSpeedMult : 1.f;
	CombatBuffAttackSpeedMult = (AttackSpeedMult > 0.f) ? AttackSpeedMult : 1.f;

	// take effect NOW: recompose the walk speed (Base × Rally × Combat) and re-arm a
	// live attack loop at the new effective cadence — a unit mid-Attack speeds up
	// immediately instead of waiting for its next Attack entry.
	RefreshComposedMoveSpeed();
	RearmAttackTimerAtEffectiveCadence();

	// (re)arm the single one-shot expiry with a fresh window — refresh, never stack
	GetWorldTimerManager().SetTimer(CombatBuffTimerHandle, this, &ASummonedUnit::EndCombatBuff, Seconds, /*bLoop=*/ false);
}

void ASummonedUnit::EndCombatBuff()
{
	// idempotent: clear the timer either way, restore only when a buff is live so
	// the recompose/re-arm run EXACTLY once per episode (zero residual drift)
	GetWorldTimerManager().ClearTimer(CombatBuffTimerHandle);

	if (!bCombatBuffActive)
	{
		return;
	}
	bCombatBuffActive = false;
	CombatBuffMoveSpeedMult = 1.f;
	CombatBuffAttackSpeedMult = 1.f;

	// restore both halves EXACTLY: the walk speed recomposes (back to the shared
	// episode base when Rally is also inactive, else Base × RallyMult), and a live
	// attack loop re-arms at the plain row cadence — expiry never leaves a fast
	// loop running (both calls are no-ops when nothing is live to restore).
	RefreshComposedMoveSpeed();
	RearmAttackTimerAtEffectiveCadence();
}

void ASummonedUnit::RefreshComposedMoveSpeed()
{
	// The ONE walk-speed writer for the buff system (TASK-042/099): every write is
	// SharedBase × RallyMult × CombatMoveMult, or EXACTLY the shared base when no
	// speed buff is active — so no apply/refresh/expiry ordering of Rally and
	// Battle Cry can ever drift the resting speed (TASK-020 discipline). Only ever
	// called from the buff Apply/End paths, which guarantee the base was captured.
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	if (!bMoveSpeedBuffActive && !bCombatBuffActive)
	{
		Movement->MaxWalkSpeed = MoveSpeedBuffBaseSpeed;
		return;
	}

	float ComposedSpeed = MoveSpeedBuffBaseSpeed;
	if (bMoveSpeedBuffActive)
	{
		ComposedSpeed *= MoveSpeedBuffMultiplier;
	}
	if (bCombatBuffActive)
	{
		ComposedSpeed *= CombatBuffMoveSpeedMult;
	}
	Movement->MaxWalkSpeed = ComposedSpeed;
}

float ASummonedUnit::GetEffectiveAttackCadence() const
{
	// Battle Cry (TASK-099): +X% attack speed = row Cadence ÷ (1 + X). Floored at
	// MinAttackCadence — a looping timer needs a strictly positive rate, and
	// AttackCadence itself was already floored at bind time. Multiplier is
	// sanitized strictly positive at apply time.
	if (bCombatBuffActive)
	{
		return FMath::Max(AttackCadence / CombatBuffAttackSpeedMult, MinAttackCadence);
	}
	return AttackCadence;
}

void ASummonedUnit::RearmAttackTimerAtEffectiveCadence()
{
	// only a LIVE attack loop is re-armed — outside Attack there is nothing to
	// re-rate, and EnterAttack composes the effective cadence itself on entry.
	if (!GetWorldTimerManager().IsTimerActive(AttackTimerHandle))
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// honor the cooldown from the last landed hit under the NEW cadence (the
	// EnterAttack gate, re-run): an already-elapsed cooldown fires on the next
	// timer tick — deliberately never synchronously, so a buff API call can never
	// re-enter combat code mid-resolve. 0.01 s is the "now" floor.
	const float EffectiveCadence = GetEffectiveAttackCadence();
	const double Now = World->GetTimeSeconds();
	float FirstDelay = static_cast<float>(static_cast<double>(EffectiveCadence) - (Now - LastAttackTime));
	FirstDelay = FMath::Max(FirstDelay, 0.01f);
	GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ASummonedUnit::PerformAttack, EffectiveCadence, /*bLoop=*/ true, FirstDelay);
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
	// Push spawn-init HP to the overhead bar (TASK-130 push model). The bar's component may
	// bind before stats load (component BeginPlay runs during Super::BeginPlay); this broadcast
	// fills it to full in that case, and its InitForCombatant seed covers the reverse order.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
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

	// M7 skeletal swap (TASK-159): CardID is guaranteed bound here (both the deferred
	// and the late-InitUnit paths reach LoadStatsAndStart with a CardID), so this is the
	// one correct place to try SK_<CardID>. If it takes, re-apply the team recolor so
	// slot 0 lands on the now-active SKELETAL mesh (ApplyTeamMaterial in BeginPlay ran
	// before this and colored the static mesh; this second apply targets the skeletal).
	ResolveSkeletalVisual();
	if (bUsingSkeletalVisual)
	{
		ApplyTeamMaterial();
	}

	// §6 spawn squash-and-stretch (TASK-155): point the juice at the ACTIVE visual mesh
	// (static or skeletal) and pop it once. Null-safe (no target = no-op).
	if (MeshJuiceComponent)
	{
		MeshJuiceComponent->SetTargetMesh(GetActiveVisualMesh());
		MeshJuiceComponent->PlaySpawnSquash();
	}

	// §6 unit-spawn audio (TASK-179): world one-shot at the unit, null-safe until S_UnitSpawn lands.
	USiegeFeedbackLibrary::PlayWorldSound(this, UnitSpawnSoundPath, GetActorLocation());

	bStatsLoaded = true;

	// ⚠️ THE SPAWN AUTO-ENROLL (TASK-396, CONVENTIONS §2 — the DEFAULT-STANCE law).
	// Placed HERE and not at the tail of BeginPlay, for four measured reasons:
	//   (1) Profile is bound ~50 lines above and IsFollowCommandEligible() reads it.
	//       At the tail of BeginPlay on the plain-SpawnActor path Profile is still the
	//       CONSTRUCTOR default (Standard) — an Ogre would enroll before its row said
	//       Siege.
	//   (2) bStatsLoaded is true here, so "eligible" is answered from real card data
	//       rather than from constructor defaults.
	//   (3) LoadStatsAndStart is reached from BOTH spawn shapes — BeginPlay (the
	//       SpawnActorDeferred + InitUnit + FinishSpawning path that EVERY shipped
	//       spawner uses: SpawnUnitSwarm, ABarracks, SummonTestUnit) AND the late
	//       InitUnit bind after a plain SpawnActor. It is still ONE insertion point
	//       on the UNIT, which is what the spec requires.
	//   (4) It runs BEFORE the synchronous UpdateState below, so a follower's VERY
	//       FIRST decision is already the follow body — no wasted castle-bound
	//       Advance on the spawn frame.
	//
	// ⚠️ THIS CALL IS INSIDE ASummonedUnit::BeginPlay's SYNCHRONOUS STACK on the path
	// every shipped spawner takes, and TASK-398's miner spawn-default window
	// (AMinerUnit::bSpawnFollowEnrollWindowClosed, closed on the statement right after
	// Super::BeginPlay returns) is scoped to exactly that stack. KNOWN RESIDUAL, stated
	// rather than papered over: on the LATE-InitUnit path the window is already closed,
	// so a miner spawned that way WOULD auto-enroll. No shipped spawner uses it for a
	// unit (grep: there is no plain SpawnActor of ASummonedUnit or any subclass), so it
	// is unreachable today — but if one is ever added, the miner ruling is what breaks,
	// and the fix belongs in AMinerUnit (widen the window), not in a second base gate.
	// The early-out guards inside cover the bDead / bAIFrozen / not-eligible /
	// no-controller cases silently.
	TryAutoEnrollInFollowGroup();

	// state machine (GDD §3.8): first decision now, then every StateCheckInterval seconds
	UpdateState();
	GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ASummonedUnit::UpdateState, StateCheckInterval, /*bLoop=*/ true);
}

void ASummonedUnit::TryAutoEnrollInFollowGroup()
{
	// ⚠️ THE DEFAULT-STANCE LAW (CONVENTIONS §2; Jonathan-confirmed, and recorded as a
	// deliberate change to the core game loop, NOT a side effect): every
	// follow-eligible Blue unit spawns FOLLOWING, on EVERY spawn path, and nothing
	// player-side auto-engages any more — the player personally orders every fight.
	// Siege units (Ogre/Sapper) and the entire bot/Red side are UNAFFECTED, because
	// the eligibility predicate excludes them.
	//
	// Enrollment is UNCONDITIONAL: a unit spawned after the player pressed T still
	// spawns following (reinforcements do NOT inherit the last order — the flagged,
	// accepted ergonomic consequence).
	//
	// EVERY refusal below is SILENT and degrades to today's behavior. This runs on
	// every unit spawn, so a missed enroll must never be a crash and never a stall.
	if (bDead || !bStatsLoaded || bAIFrozen)
	{
		return;
	}

	// ⚠️ ONE GATE, AND IT MUST STAY ONE GATE — IsFollowCommandEligible() and nothing
	// else. That is a CROSS-TASK CONTRACT, not a style choice:
	//   • Siege (Ogre/Sapper) and every Red/bot unit are excluded by the base
	//     predicate, which is what keeps them auto-marching exactly as today.
	//   • THE MINER SPAWN-DEFAULT RULING (manager ruling 7, CONVENTIONS §5 — a miner
	//     SPAWNS MINING) is delivered ENTIRELY by AMinerUnit::CanFollowHero(), which
	//     answers its EditDefaultsOnly bFollowOnSpawn switch for exactly as long as
	//     ASummonedUnit::BeginPlay is on the stack. So the refusal happens right here,
	//     through this predicate, and the miner joins no group AND no Members array —
	//     the part that matters, since a stale Members entry would make the
	//     controller's Contains() idempotence early-out swallow every future C press.
	//   • A SECOND, base-side miner carve-out was drafted and DELIBERATELY REMOVED:
	//     it would have silently defeated bFollowOnSpawn = true, the one line Jonathan
	//     flips at his playtest gate. One decision, one owner.
	// Adding a second condition here, or deferring this call past BeginPlay's
	// synchronous stack, re-opens that ruling — AMinerUnit::BeginPlay carries a
	// Warning tripwire for precisely that regression.
	if (!IsFollowCommandEligible())
	{
		return;
	}

	// M8 TEAM LAW: the owning-team resolve, never GetFirstPlayerController(). Null
	// (no controller yet — placement/possession ordering, or a Red unit, which cannot
	// reach here anyway) simply means this unit runs its normal body.
	UWorld* const World = GetWorld();
	ASiegePlayerController* const PC = ASiegePlayerController::FindControllerForTeam(World, Team);
	if (!PC)
	{
		return;
	}

	// ONE call, and the controller owns everything downstream of it (TASK-395's
	// stated seam): lazy creation of the ONE default follow group, the steal out of
	// any other group, the once-computed golden-angle station, AssignCommandGroup and
	// the prune. This unit never creates a group, never computes a station and never
	// touches DefaultFollowGroupId.
	PC->EnrollInDefaultFollowGroup(this);
}

void ASummonedUnit::UpdateState()
{
	// bAIFrozen is defense-in-depth (TASK-028): FreezeAI clears this timer, but a
	// frozen unit must make no decisions even if something ever re-armed it.
	// bSpellFrozen mirrors it for the resumable FrostNova freeze (TASK-099).
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen)
	{
		return;
	}

	// CHARGE bookkeeping (TASK-055): accumulate uninterrupted-movement time so the first attack
	// after >= ChargeMoveSeconds gets the ×ChargeMultiplier bonus. No-op for non-charge units
	// (guarded by bCharge), so every non-Cavalry unit — and the Miner subclass — is byte-unchanged.
	// Runs before the profile dispatch so it tracks regardless of profile.
	TrackChargeMovement();

	// STUCK WATCHDOG (TASK-532; CONVENTIONS NAV-§3): ONE line, and its placement is
	// load-bearing for exactly the reason TrackChargeMovement's is — it sits ABOVE the
	// follow hoist and ABOVE the profile dispatch, so Standard, Siege, Support, Follow and
	// Hold/Ambush are all covered by this single call and no rescue has to be scattered
	// into the individual bodies. The freeze/death early-out is above us, so a dead,
	// frozen or spell-frozen unit never reaches it. ZERO new timers: this rides the
	// 0.25 s StateTimerHandle poll that already exists (the no-double-driver law).
	// The delta is the WORLD CLOCK, never StateCheckInterval — AMinerUnit seals that to 0.
	TickStuckWatchdog(ConsumeStuckDeltaSeconds());

	// ⚠️ THE SIDESTEP LEASE (TASK-532, NAV-§3) — this early-out is what makes the Sidestep
	// rung work at all. EnterAdvanceToLocation NULLS CurrentMoveGoal (:2675), so WITHOUT
	// this the next poll's bGoalChanged (:2533) would immediately re-issue
	// EnterAdvance(real goal) and cancel the sidestep 0.25 s after it started — the unit
	// would stay wedged and all the ladder would have achieved is one extra path request.
	//
	// It deliberately RE-ISSUES NOTHING: the sidestep's MoveToLocation was issued ONCE
	// when the lease was armed and path-following is already carrying it out. Re-calling
	// EnterAdvanceToLocation here every poll would be a mill in the making — its guard
	// falls through on GetMoveStatus() == Idle (:2677), so a sidestep goal the navmesh
	// rejects would re-request 4×/s for the whole lease and blow the NAV-§3 worst case of
	// ≤1 extra path request per unit per second. Suppressing the dispatch is the entire
	// job; the ladder still runs above (the lease is drained there), so a unit that is
	// STILL stalled escalates to WidenAndRepath on schedule, which supersedes the lease.
	if (SidestepLeaseRemaining > 0.f)
	{
		return;
	}

	// ── FOLLOW dispatch — HOISTED ABOVE THE PROFILE DISPATCH (TASK-396) ────────
	// CONVENTIONS "FOLLOW command … (2026-08-02)" §4 "DISPATCH POINT
	// (load-bearing)". THIS HOIST IS THE ONE STRUCTURAL CHANGE TO THIS FUNCTION,
	// and it is the reason a following CLERIC works at all: Support (and Siege)
	// RETURN out of the profile dispatch immediately below, so the shipped group
	// dispatch further down is never reached for them. A follow group must dispatch
	// whatever the profile, so it is tested FIRST. Hold/Ambush dispatch stays
	// exactly where it always was and every other branch is byte-identical.
	//
	// A unit in a live HOLD/AMBUSH group falls straight through here untouched and
	// is run by the shipped dispatch below — the only cost is one extra resolve per
	// tick for a zone-ordered unit (FindControllerForTeam is silent and
	// allocation-free; FindUnitGroup is a small array scan).
	//
	// A DEAD id SELF-HEALS here exactly as it always did below — clear it and fall
	// through in the SAME tick, never a stall. Doing it here as well is NOT
	// redundant: a Support unit can now hold a group id (Follow widened to the
	// Cleric) and the block below is unreachable for it, so this is the only
	// self-heal such a unit ever gets. Siege units are still never assigned an id.
	if (CommandGroupId != INDEX_NONE)
	{
		UWorld* const FollowWorld = GetWorld();
		const ASiegePlayerController* FollowPC =
			ASiegePlayerController::FindControllerForTeam(FollowWorld, Team);
		const FSiegeUnitGroup* FollowGroup = FollowPC ? FollowPC->FindUnitGroup(CommandGroupId) : nullptr;
		if (FollowGroup)
		{
			if (FollowGroup->Type == ESiegeGroupCommandType::Follow)
			{
				UpdateStateFollow(*FollowGroup);
				return;
			}
			// a live HOLD/AMBUSH group: leave it entirely to the shipped dispatch below
		}
		else
		{
			ClearCommandGroup();
		}
	}

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

	// ── Group orders (TASK-344) ────────────────────────────────────────────────
	// Per-unit HOLD/AMBUSH assignments dispatch ABOVE the stance gate below —
	// grouped behavior is per-unit and cannot live in the team-stance body. Only
	// the controller's eligibility-gated AssignCommandGroup ever sets the id
	// (Standard + Blue only), so this never fires for Siege/Support/miners or
	// bot/Red units — their paths above and below are byte-unchanged. A DEAD id
	// (group released by T/E, all-dead pruned, steal-emptied, or Play-Again
	// reset) SELF-HEALS: clear it and fall through to the stance gate THIS same
	// tick — never a stall.
	if (CommandGroupId != INDEX_NONE)
	{
		// M8 (TASK-356 doc §3.7 — the ruling-4 named offender #1, `3068286`):
		// the FIRST-controller poll is replaced by the OWNING-TEAM resolve — this
		// unit reads ITS OWN team's controller, never "controller 0". Standalone:
		// one (local, Blue) PC and only Blue units carry a group id ⇒ the resolve
		// returns exactly the controller the old call did (doc §10). A null
		// resolve behaves exactly like the old null-first-controller (the
		// self-heal below).
		UWorld* const GroupWorld = GetWorld();
		const ASiegePlayerController* GroupPC =
			ASiegePlayerController::FindControllerForTeam(GroupWorld, Team);
		const FSiegeUnitGroup* Group = GroupPC ? GroupPC->FindUnitGroup(CommandGroupId) : nullptr;
		if (Group)
		{
			UpdateStateGrouped(*Group);
			return;
		}
		ClearCommandGroup();
	}

	// ── Shield Wall unit commands (W1 TASK-275) ────────────────────────────────
	// The local human player (Blue — CONVENTIONS team contract: "player is always
	// ETeamId::Blue") may latch a stance (Attack/Hold/Defend) that reshapes THIS
	// Standard body's target + march goal. The gate is deliberately NARROW so nothing
	// else changes: ONLY the player's own Blue *Standard* units (miners are Profile
	// None → excluded, so their body stays untouched), and ONLY after the player's
	// first command (HasIssuedCommand). Bot/Red units NEVER read the command, and
	// player units before the first press fall straight through to the LEGACY body
	// below — which is therefore byte-for-byte unchanged whenever this gate is false.
	// Re-evaluated every check, so a mid-flight stance change re-targets next tick (the
	// stance is LIVE). Freeze gating is upstream (the early-out above), so a frozen unit
	// never reaches here.
	if (Profile == ECardProfile::Standard && Team == ETeamId::Blue)
	{
		// M8 (TASK-356 doc §3.7 — ruling-4 named offender #2): the stance read
		// resolves THIS unit's owning-team controller instead of controller 0.
		// The `Team == Blue` gate above deliberately STAYS (P2 scope, doc §2.4 —
		// the Red human's stance surface lands with its Server RPC). Standalone:
		// same single Blue PC as before (doc §10).
		if (UWorld* const CmdWorld = GetWorld())
		{
			if (const ASiegePlayerController* PC = ASiegePlayerController::FindControllerForTeam(CmdWorld, Team))
			{
				if (PC->HasIssuedCommand())
				{
					UpdateStateStandardCommanded(*PC);
					return;
				}
			}
		}
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

void ASummonedUnit::UpdateStateStandardCommanded(const ASiegePlayerController& PC)
{
	// Reached ONLY through the UpdateState gate (Profile==Standard, Team==Blue,
	// PC.HasIssuedCommand()). Re-evaluates the stance every tick, so the unit adopts a
	// mid-flight command change on its next state check (the stance is LIVE).
	const FVector MyLocation = GetActorLocation();

	switch (PC.GetCurrentCommand())
	{
	case ESiegeUnitCommand::Defend:
	{
		// DEFEND: fight only enemies within the defend disc of the OWN castle, else fall
		// back toward home. No own castle (destroyed → the match is over): stand down.
		ACastle* OwnCastle = FindOwnCastle();
		if (!OwnCastle)
		{
			EnterIdle();
			break;
		}

		CurrentTarget = AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), DefendRadius);

		if (CurrentTarget)
		{
			if (GetDistanceToTarget(MyLocation, CurrentTarget) <= AttackRange)
			{
				EnterAttack();
			}
			else
			{
				EnterAdvance(CurrentTarget);
			}
		}
		else
		{
			// no attacker near home: the own castle is the fall-back goal (an actor →
			// EnterAdvance stops flush at its walls; we never attack our own castle,
			// since EnterAttack fires only on an enemy CurrentTarget).
			EnterAdvance(OwnCastle);
		}
		break;
	}
	case ESiegeUnitCommand::Hold:
		// DEFENSIVE fall-through (TASK-344): the team-wide HOLD stance is
		// SUPERSEDED by the per-unit group orders (UpdateStateGrouped) and nothing
		// latches it any more — SetUnitCommand(Hold) has no remaining caller. The
		// enum member stays declared (WBP_HUD's stance switch pins depend on the
		// byte layout), so a stale/out-of-contract Hold value simply behaves as
		// ATTACK instead of dereferencing the deleted hold-point state.
	case ESiegeUnitCommand::Attack:
	default:
	{
		// ATTACK: mirrors the legacy Standard body EXACTLY — the leash/reacquire + AcquireTarget
		// + the in-aggro attack/advance below, AND the no-in-aggro march goal (the stable enemy
		// castle). TASK-282 (arena final-approach halt fix): the prior box-defender-FIRST goal
		// substitution (FindNearestEnemyInSpawnBox, gated within EnemyBaseEngageRadius by TASK-280)
		// is REMOVED. That box turns over every bot wave, so its nearest-to-self result FLIPPED
		// every 0.25 s state tick — EnterAdvance re-pathed each tick and the unit milled near the
		// radius ("stopped just short"); and while ANY box defender remained the castle was never
		// the sustained goal/CurrentTarget, so EnterAttack on the castle never fired (the bot
		// endlessly repopulates its box, so the castle was never reached). Marching the stable
		// castle instead makes the WHOLE ATTACK approach the proven-good legacy castle-kill
		// (runtime-verified on this build: a full-field marcher drove the enemy castle to 0 HP /
		// destroyed): AcquireTarget still engages any defender that enters AggroRadius on the way,
		// and at the wall the castle is acquired as CurrentTarget and attacked. This also SUBSUMES
		// the TASK-280 anti-freeze — the goal is now the stable castle across the entire approach,
		// not just mid-field. A null/destroyed enemy castle leaves Goal null ⇒ EnterIdle (match over).
		if (CurrentTarget && (!IsTargetAlive(CurrentTarget) || GetDistanceToTarget(MyLocation, CurrentTarget) > LeashRange))
		{
			CurrentTarget = nullptr;
		}

		if (AActor* Acquired = AcquireTarget())
		{
			CurrentTarget = Acquired;
		}

		AActor* Goal = CurrentTarget;
		if (!Goal)
		{
			Goal = FindNearestEnemyCastle();
		}

		if (!Goal)
		{
			// no target and no standing enemy castle (destroyed → the match is over): stand down
			EnterIdle();
			break;
		}

		if (CurrentTarget && GetDistanceToTarget(MyLocation, CurrentTarget) <= AttackRange)
		{
			EnterAttack();
		}
		else
		{
			EnterAdvance(Goal);
		}
		break;
	}
	}
}

void ASummonedUnit::UpdateStateGrouped(const FSiegeUnitGroup& Group)
{
	// Reached ONLY through the UpdateState group dispatch (CommandGroupId resolved
	// to a live group on the local controller). Priority ladder (CONVENTIONS
	// "Group orders" behavior law): enemies in the ATTACK zone → else enemies in
	// the POSITION zone → else walk to the per-unit station and wait. Anti-thrash
	// STICKINESS (the TASK-280/282 freeze lesson): the current target is KEPT
	// while alive and zone-valid — acquisition runs ONLY when target-less, so the
	// goal can never flip every 0.25 s tick the way the retired box-first search
	// did. There is deliberately NO LeashRange here: the zones ARE the leash for
	// HOLD, and AMBUSH's whole point is the unbounded chase.
	const FVector MyLocation = GetActorLocation();

	// a dead/destroyed target is dropped for BOTH types
	if (CurrentTarget && !IsTargetAlive(CurrentTarget))
	{
		CurrentTarget = nullptr;
	}

	// zone validity of a surviving target (2D disc tests, matching
	// AcquireEnemyNearPoint's candidate-location disc filter)
	if (CurrentTarget && Group.Type == ESiegeGroupCommandType::Hold)
	{
		const FVector TargetLocation = CurrentTarget->GetActorLocation();
		const bool bTargetInAttackZone =
			FVector::DistSquared2D(TargetLocation, Group.AttackCenter) <= FMath::Square(Group.AttackRadius);
		const bool bTargetInPositionZone =
			FVector::DistSquared2D(TargetLocation, Group.PositionCenter) <= FMath::Square(Group.PositionRadius);

		if (!bTargetInAttackZone && !bTargetInPositionZone)
		{
			// HOLD leash: the tick the target exits BOTH zones it is dropped —
			// disengage; target-less below, the unit returns toward its station.
			// AMBUSH deliberately skips this whole block while a live target
			// exists (the chase-to-the-kill leash-exemption): it keeps the
			// target until the kill, then the ladder resumes.
			CurrentTarget = nullptr;
		}
		else if (!bTargetInAttackZone)
		{
			// single MONOTONE upgrade (HOLD only): a position-tier target yields
			// to an attack-zone enemy the moment one exists. The step only ever
			// goes position→attack — an attack-tier target is never downgraded —
			// so the tiers cannot oscillate (the anti-ping-pong guarantee).
			if (AActor* UpgradeTarget = AcquireEnemyNearPoint(Group.AttackCenter, Group.AttackRadius))
			{
				CurrentTarget = UpgradeTarget;
			}
		}
	}

	// acquisition ONLY when target-less (stickiness): tier 1 = the attack zone,
	// tier 2 = the position zone. AMBUSH acquires through the same tiers — its
	// exemption above only governs when an already-held target is RELEASED.
	//
	// ══ ANCIENT GROUNDS ATTACK SEAL — GUARD 2 of 3 (TASK-360, CONVENTIONS §3) ══
	// A unit that can never attack acquires NOTHING here: the target is FORCED null
	// instead of running the two AcquireEnemyNearPoint tiers, so the ladder falls straight
	// through to tier-3 station-keeping below. That is precisely the "commandable but never
	// fights" behavior — WITHOUT this a grouped sorcerer would walk to an enemy and stand
	// there (EnterAdvance on a live target), which reads as a bug even though guard 1 keeps
	// it from ever swinging. Forcing null every tick also makes the stickiness/HOLD-upgrade
	// blocks ABOVE unreachable for a sealed unit (they are all gated on a non-null
	// CurrentTarget), so the single monotone-upgrade AcquireEnemyNearPoint call up there
	// can never fire for one either.
	if (!CanEverAttack())
	{
		CurrentTarget = nullptr;
	}
	else
	{
		if (!CurrentTarget)
		{
			CurrentTarget = AcquireEnemyNearPoint(Group.AttackCenter, Group.AttackRadius);
		}
		if (!CurrentTarget)
		{
			CurrentTarget = AcquireEnemyNearPoint(Group.PositionCenter, Group.PositionRadius);
		}
	}

	if (CurrentTarget)
	{
		if (GetDistanceToTarget(MyLocation, CurrentTarget) <= AttackRange)
		{
			EnterAttack();
		}
		else
		{
			EnterAdvance(CurrentTarget);
		}
		return;
	}

	// tier 3 — no eligible enemy in either zone: advance to the per-unit station
	// (PositionCenter + the confirm-time sunflower offset — the spread that kills
	// mill-at-one-point) and wait. The existing 150 uu HoldArrivalTolerance reads
	// "gathered"; EnterAdvanceToLocation carries the TASK-275 kite-fix, so a
	// stale actor-chase can never strand the return leg.
	const FVector Station = Group.PositionCenter + GroupStationOffset;
	if (FVector::DistSquared2D(MyLocation, Station) <= FMath::Square(HoldArrivalTolerance))
	{
		EnterIdle();
	}
	else
	{
		EnterAdvanceToLocation(Station);
	}
}

void ASummonedUnit::UpdateStateFollow(const FSiegeUnitGroup& Group)
{
	// THE FOLLOW BODY (TASK-396; CONVENTIONS "FOLLOW command … (2026-08-02)" §4).
	// Reached ONLY through the hoisted follow dispatch in UpdateState — this unit's
	// CommandGroupId resolved to a LIVE group whose Type is Follow.

	// Structural tripwire. The Group parameter carries no zones (a follow group owns
	// no ground: zero radii, zero centers, null marker decals), so this is the only
	// thing read off it. Unreachable by construction — the ONE caller already tested
	// the type — and it exists so a future refactor cannot silently route a zone
	// group into the never-attack body and strand its units on the hero.
	if (Group.Type != ESiegeGroupCommandType::Follow)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASummonedUnit '%s': UpdateStateFollow reached with a non-Follow group (type %d) — the UpdateState dispatch contract is broken."),
			*GetNameSafe(this), static_cast<int32>(Group.Type));
		return;
	}

	// ══ THE NEVER-ATTACK SEAL — PER BODY, NOT PER CLASS (Jonathan's ruling iii) ══
	// The target is FORCED null every tick and this function calls NONE of
	// AcquireTarget / AcquireEnemyNearPoint / EnterAttack. The only two movement
	// calls it can ever make are EnterAdvanceToLocation and EnterIdle, and BOTH of
	// those clear AttackTimerHandle and stop the lunge on the way out of Attack — so
	// a unit that was mid-fight when it was circled stands down on its first follow
	// tick. Belt: PerformAttack re-validates CurrentTarget and returns on null, so
	// even a timer still in flight in the ≤0.25 s enroll window lands no hit.
	//
	// ⚠️ DELIBERATELY NOT CanEverAttack(): that seal is per-CLASS and PERMANENT
	// (ASorcererUnit, AMinerUnit). A following Footman must become a normal attacker
	// the instant its group is released, so this seal has to live in the STANCE body.
	CurrentTarget = nullptr;

	// A FOLLOWING CLERIC STILL HEALS (manager ruling 9 — healing is not attacking,
	// and an escorting medic is the obvious intent of a support unit told to follow).
	// This is the heal-TARGETING half of UpdateStateSupport, shared verbatim; the
	// heal itself runs on its own timer (PerformHeal), so it is entirely decoupled
	// from movement and a following Cleric mends at exactly the shipped rate. Cost:
	// one FindNearestDamagedFriendly per state tick per following Cleric — the same
	// query, at the same cadence, a non-following Cleric already runs.
	if (Profile == ECardProfile::Support)
	{
		UpdateSupportHealTargeting();
	}

	// ⚠️ THE ANCHOR IS RESOLVED LIVE, EVERY TICK, AND NEVER CACHED (CONVENTIONS §4).
	// The respawn path may hand back a DIFFERENT pawn actor, so a cached pointer
	// would escort a corpse forever; resolving live is exactly what makes hero
	// respawn work for free. GetFirstPlayerController() is BANNED (M8 TEAM LAW) —
	// the reach-path is FindControllerForTeam(World, Team), the same owning-team
	// resolve the dispatch above used to find this group.
	UWorld* const FollowWorld = GetWorld();
	const ASiegePlayerController* const PC =
		ASiegePlayerController::FindControllerForTeam(FollowWorld, Team);
	AActor* const Anchor = PC ? PC->GetFollowAnchor() : nullptr;

	if (!Anchor)
	{
		// HERO-DEATH RULING (manager ruling 8, CONVENTIONS §4): no live hero ⇒ HOLD
		// POSITION. No target, no march, no attack — and resume the instant a live
		// pawn resolves again, including a brand-new post-respawn one. Rejected and
		// recorded so they are not re-derived: marching to the corpse (a conga line
		// to a body deep in enemy territory), and falling back to Defend (that would
		// make followers fight, breaking ruling iii). Dropping the goal latch is what
		// guarantees the resume tick always issues its own fresh move.
		bHasFollowGoalLocation = false;
		EnterIdle();
		return;
	}

	// The station is the LIVE anchor position plus this unit's own sunflower offset,
	// which the controller computed ONCE at enroll (per-unit scalars only — no arrays
	// on units). It is recomputed here every tick precisely because the anchor moves.
	const FVector Station = Anchor->GetActorLocation() + GroupStationOffset;
	const FVector MyLocation = GetActorLocation();

	// Arrived: the shipped 150 uu HoldArrivalTolerance reads "gathered" — the same
	// constant and the same 2D test as the group tier-3 station keeping above.
	if (FVector::DistSquared2D(MyLocation, Station) <= FMath::Square(HoldArrivalTolerance))
	{
		bHasFollowGoalLocation = false;
		EnterIdle();
		return;
	}

	// ══ ANTI-REPATH — A HARD REQUIREMENT, NOT POLISH (manager ruling 10) ══
	// EnterAdvanceToLocation's OWN re-path guard is a 1 uu Equals test (bPointChanged,
	// further down this file), so a station recomputed from a WALKING hero clears it
	// on every single 0.25 s tick. Re-issuing a move at a moving goal every tick is
	// exactly the mill that produced TASK-280 ("units freeze just past midfield") and
	// TASK-282 ("halt just short of the castle"): each request restarts path
	// following before the previous one produced meaningful motion.
	//
	// THE LAW: re-issue ONLY once the recomputed station has drifted further than
	// FollowRepathTolerance (250 uu — TASK-395's tunable, READ OFF THE RESOLVED
	// CONTROLLER and never re-declared here) FROM THE GOAL WE LAST ISSUED. Comparing
	// against the last ISSUED goal is the point of the latch: comparing hero
	// positions would reset on every re-path and can oscillate.
	//
	// The second half of the gate is "…and we are still walking to it". A completed
	// or failed move leaves path following Idle, and without this term a follower
	// that finished its leg while the hero drifted less than the band would stand
	// still forever, up to a whole band short of its station. Re-issuing from a
	// STOPPED unit cannot mill — the very next tick it is moving again and inside the
	// band, so that request gets its full run.
	// PC is guaranteed non-null here: a null PC produced a null Anchor above, which
	// already returned. The tolerance is READ, never copied into this class — one
	// feel-pass value on the controller drives every follower (CONVENTIONS §8).
	const float RepathTolerance = PC->FollowRepathTolerance;
	const AAIController* const AI = GetAIController();
	const bool bStillWalking = AI && AI->GetMoveStatus() != EPathFollowingStatus::Idle;
	if (bHasFollowGoalLocation
		&& bStillWalking
		&& FVector::DistSquared2D(Station, LastFollowGoalLocation) <= FMath::Square(RepathTolerance))
	{
		return; // inside the band and still walking — let the in-flight path run
	}

	LastFollowGoalLocation = Station;
	bHasFollowGoalLocation = true;

	// EnterAdvanceToLocation carries the TASK-275 kite-fix and projects the
	// destination to the navmesh, which is why the enroll-time station offset is
	// deliberately NOT nav-projected by the controller (it is an offset from a point
	// that moves — a projection taken at enroll would be meaningless).
	EnterAdvanceToLocation(Station);
}

void ASummonedUnit::AssignCommandGroup(int32 GroupId, const FVector& StationOffset)
{
	// controller-pushed at the stage-3 confirm (TASK-344). The offset arrives
	// precomputed and nav-projected — this unit never recomputes it.
	CommandGroupId = GroupId;
	GroupStationOffset = StationOffset;

	// a NEW order replaces the old behavior (the release law): drop any current
	// target so the next state tick (≤0.25 s) re-targets from the NEW zones —
	// without this, an AMBUSH group inheriting a stale far-away chase target
	// would pursue something the player never circled.
	CurrentTarget = nullptr;

	// TASK-396: the follow body's anti-repath latch is PER-ORDER state — a new order
	// (or a re-station) invalidates the goal it was measured against. Dropping it
	// here guarantees the first tick under the new order always issues its own move,
	// so a stale in-band comparison can never keep a unit walking toward the previous
	// order's point. Scalar write only: nothing but UpdateStateFollow reads it, so
	// HOLD/AMBUSH behavior is unchanged.
	bHasFollowGoalLocation = false;
}

void ASummonedUnit::ClearCommandGroup()
{
	CommandGroupId = INDEX_NONE;
	GroupStationOffset = FVector::ZeroVector;
	bHasFollowGoalLocation = false; // TASK-396 — see AssignCommandGroup
}

bool ASummonedUnit::CanFollowHero() const
{
	// CONVENTIONS §3 table (class identity — the shipped CanEverAttack() idiom, NOT a
	// cards.csv column and NOT a Profile change): Standard (every combat unit) PLUS
	// Support (the Cleric — Jonathan widened Follow to it). Siege is excluded, and
	// that exclusion is the whole reason the Ogre and the Sapper keep auto-marching
	// exactly as they do today. AMinerUnit overrides to true (TASK-397).
	return Profile == ECardProfile::Standard || Profile == ECardProfile::Support;
}

bool ASummonedUnit::CanTakeZoneOrders() const
{
	// CONVENTIONS §3 table: Standard ONLY. The Cleric is FOLLOW-ONLY this pass
	// (manager ruling — zone orders would mean reshaping UpdateStateSupport's heal
	// body, which Jonathan did not ask for), and Siege takes no orders at all.
	// AMinerUnit overrides to true (TASK-397).
	return Profile == ECardProfile::Standard;
}

bool ASummonedUnit::IsFollowCommandEligible() const
{
	// CONVENTIONS §3 / the §7 pin: the class predicate plus the shipped
	// team/alive/frozen gate. The resumable spell freeze deliberately does NOT
	// exclude — a frozen-but-thawing unit may be circled and obeys once it wakes
	// (UpdateState's early-out covers the frozen window).
	return CanFollowHero()
		&& Team == ETeamId::Blue
		&& !bDead
		&& !bAIFrozen;
}

bool ASummonedUnit::IsGroupCommandEligible() const
{
	// the recorded exclusion law (CONVENTIONS "Group orders"), NARROWED to ZONE
	// ORDERS by TASK-396: R (Hold) / F (Ambush) only. Name and signature are KEPT
	// because the controller's stage-1 select sweep and the §7 pinned registry both
	// depend on them — the C-key surface is IsFollowCommandEligible() above.
	// Standard units qualify; Siege (Ogre/Sapper) and Support (the Cleric, which is
	// FOLLOW-ONLY this pass) do not; the Miner now does, through its
	// CanTakeZoneOrders() override (TASK-397). bot/Red units never participate. The
	// resumable spell freeze does NOT exclude.
	return CanTakeZoneOrders()
		&& Team == ETeamId::Blue
		&& !bDead
		&& !bAIFrozen;
}

AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> TeamAgents;
	UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

	const FVector MyLocation = GetActorLocation();
	const float RadiusSq = Radius * Radius;

	// Identical bucketing/tie-break to AcquireTarget — the ONLY change is the eligibility
	// gate (a 2D disc anchored on Center, not AggroRadius-from-self). Nearest-to-SELF is
	// still the selection metric so the Standard tie-break behavior stays consistent.
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

		// no friendly targets (GDD §3.0). Native cast is valid: UTeamAgent is NotBlueprintable.
		const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
		if (!Agent || Agent->GetTeamId() == Team)
		{
			continue;
		}

		// disc filter: the candidate's LOCATION must lie within Radius (2D) of Center
		if (FVector::DistSquared2D(Candidate->GetActorLocation(), Center) > RadiusSq)
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Candidate);
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

	const float PairDistance = GetDistanceToTarget(BestPawn->GetActorLocation(), BestOther);
	return (PairDistance <= TieBreakDistance) ? BestPawn : BestOther;
}

ACastle* ASummonedUnit::FindOwnCastle() const
{
	const FVector MyLocation = GetActorLocation();

	ACastle* BestCastle = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	// mirror of FindNearestEnemyCastle but SAME-team (Team == ours), skipping destroyed
	for (TActorIterator<ACastle> It(GetWorld()); It; ++It)
	{
		ACastle* Castle = *It;
		if (!IsValid(Castle) || Castle->GetTeamId() != Team || Castle->IsCastleDestroyed())
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
	// TASK-396: the heal-TARGETING half moved VERBATIM into UpdateSupportHealTargeting
	// so the FOLLOW body can share it (a following Cleric still heals — manager ruling
	// 9). Statement order is unchanged, so this function is behaviorally identical;
	// FaceTarget deliberately did NOT move (it is wanted here, where the Cleric walks
	// AT its patient, and not in the follow body, where it would fight the movement
	// orientation).
	ASummonedUnit* HealTarget = UpdateSupportHealTargeting();

	if (HealTarget)
	{
		FaceTarget(HealTarget); // cosmetic: look at who we are mending
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

ASummonedUnit* ASummonedUnit::UpdateSupportHealTargeting()
{
	// EXTRACTED VERBATIM from UpdateStateSupport by TASK-396 so the FOLLOW body can
	// share it: A FOLLOWING CLERIC STILL HEALS (manager ruling 9 — healing is not
	// attacking). These four statements, in this order, are exactly what
	// UpdateStateSupport ran before the extraction; the caller re-adds FaceTarget.
	ASummonedUnit* HealTarget = FindNearestDamagedFriendly();
	SupportHealTarget = HealTarget;

	if (HealTarget)
	{
		StartHealing();  // arm/keep the heal timer — PerformHeal does the work
	}
	else
	{
		StopHealing();   // nobody hurt in range — stop the heal cadence
	}

	return HealTarget;
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
	// defense-in-depth (the PerformAttack gate): FreezeAI/HandleDeath clear this
	// timer; ApplyFreeze clears it too (TASK-099 — a frozen Cleric mends no one)
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen)
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
	// negative "heals". Clamp to MaxHP (no overheal), then push the change to the overhead
	// bar (TASK-130 push model — units NOW carry an OnHPChanged delegate).
	if (bDead || bAIFrozen || !bStatsLoaded || Amount <= 0.f)
	{
		return;
	}

	CurrentHP = FMath::Min(CurrentHP + Amount, MaxHP);
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());
}

void ASummonedUnit::EnterAttack()
{
	// ══ ANCIENT GROUNDS ATTACK SEAL — GUARD 1 of 3 (TASK-360, CONVENTIONS §3) ══
	// The STRUCTURAL chokepoint: all four attack entries (the legacy Standard body, the
	// Shield Wall commanded body, UpdateStateGrouped, UpdateStateSiege) funnel through here,
	// so this is the guard that actually seals the machine. A unit that can never attack
	// STANDS DOWN — EnterIdle() rather than a silent `return` — which keeps the state machine
	// honest: it clears any attack timer, stops movement and parks State at Idle instead of
	// leaving a sealed unit stuck in a stale Advance/Attack state that TrackChargeMovement
	// and the bar/debug readbacks would then misreport. EnterIdle is early-out idempotent
	// (it returns immediately when already Idle) and never calls back into EnterAttack, so
	// there is no recursion and no per-tick churn.
	// ⚠️ Without this a Cadence-0 Sorcerer row would arm a 0.05 s looping attack timer — 20
	// hits/s (see CanEverAttack()'s header comment).
	if (!CanEverAttack())
	{
		EnterIdle();
		return;
	}

	if (State != ESummonedUnitState::Attack)
	{
		State = ESummonedUnitState::Attack;
		CurrentMoveGoal = nullptr;

		// STUCK WATCHDOG RESET — exit 2 of 4 (TASK-532, NAV-§3). The unit stopped walking
		// and started swinging: any sidestep in flight is over, and the stall clock must
		// not carry into the next advance (an attacker standing still for 6 s is doing its
		// job, not stalling — and the StopMovement below makes GetMoveStatus() Idle, so
		// the ladder reads it as not-advancing anyway; this keeps the state honest rather
		// than relying on that second-order fact). Inside the transition guard on purpose:
		// it mirrors the CurrentMoveGoal clear beside it, and a lease can only be live on
		// a unit that was Advancing (EnterAdvanceToLocation sets State = Advance to arm it).
		SidestepLeaseRemaining = 0.f;
		FSiegeStuckStatics::Reset(StuckState);

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
		// TASK-099: the cadence is the EFFECTIVE one (row Cadence ÷ Battle Cry attack-speed
		// multiplier — plain row cadence with no buff, so M1..M4 units are byte-unchanged).
		const double Now = World->GetTimeSeconds();
		const float EffectiveCadence = GetEffectiveAttackCadence();
		float FirstDelay = static_cast<float>(static_cast<double>(EffectiveCadence) - (Now - LastAttackTime));
		if (FirstDelay <= 0.f)
		{
			PerformAttack();
			FirstDelay = EffectiveCadence;
		}
		GetWorldTimerManager().SetTimer(AttackTimerHandle, this, &ASummonedUnit::PerformAttack, EffectiveCadence, /*bLoop=*/ true, FirstDelay);
	}
}

void ASummonedUnit::EnterAdvance(AActor* Goal)
{
	if (State == ESummonedUnitState::Attack)
	{
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		StopAttackLunge(); // leaving Attack: mesh back to EXACTLY the rest pose (TASK-020)
		RestoreLocomotionAnim(); // TASK-165 (rigged): end any attack clip so walk resumes; no-op for static units
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
			// huge box and stop out of range — walk flush to its walls instead.
			//
			// TASK-349 loop-2 B2 (march-freeze fix): with the hollow 3× castle the goal
			// ACTOR's origin sits on ENEMY-interior navmesh, which the mover's team
			// default filter (null FilterClass below) EXCLUDES — MoveToActor's goal-poly
			// resolution fails outright and bAllowPartialPath cannot rescue a goal that
			// never resolves to a poly (the 123-unit freeze). So resolve the march goal
			// ourselves: nearest point on the structure's blocking collision to THIS
			// unit (per-unit near-wall spread, exactly the pre-3× partial-path
			// behavior), projected to the nearest poly the mover's OWN filter allows
			// (wall-base ring / gate apron for the enemy castle; the own interior for
			// own-team goals). The MOVE still runs under the team default filter, so
			// the no-enemy-pathing-inside guarantee is byte-untouched — only the GOAL
			// changed from an unreachable poly to a reachable one. Structures are
			// static, so losing MoveToActor's moving-goal tether costs nothing, and
			// this whole branch still runs ONLY on goal change / idle (the enclosing
			// gate) — never per tick (TASK-280/282 thrash law). Projection failure or
			// a missing nav system degrades to the legacy MoveToActor (pre-feature
			// behavior + the warn below; null-safety law).
			FVector MarchPoint = FVector::ZeroVector;
			if (ResolveStructureMarchPoint(*Goal, MarchPoint))
			{
				Result = AI->MoveToLocation(MarchPoint, StructureMoveAcceptanceRadius, /*bStopOnOverlap=*/ false,
					/*bUsePathfinding=*/ true, /*bProjectDestinationToNavigation=*/ false, /*bCanStrafe=*/ true,
					/*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);
			}
			else
			{
				Result = AI->MoveToActor(Goal, StructureMoveAcceptanceRadius, /*bStopOnOverlap=*/ false,
					/*bUsePathfinding=*/ true, /*bCanStrafe=*/ true, /*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);
			}
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

bool ASummonedUnit::ResolveStructureMarchPoint(const AActor& Goal, FVector& OutMarchPoint) const
{
	// TASK-349 loop-2 B2 — full rationale on the header decl. Null-safe ladder:
	// any missing piece returns false and the caller degrades to the legacy
	// MoveToActor (pre-feature behavior).
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	const ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance() : nullptr;
	if (!NavData)
	{
		return false;
	}

	// (1) Nearest point on the goal's ECC_Pawn-blocking collision to THIS unit —
	// the same closest-point convention every range check in this codebase uses,
	// so the resolved march target is the very wall face the attack math will
	// measure against. Per-unit: each attacker resolves ITS near wall (the pre-3×
	// partial-path spread — no single-point pile-up). No blocking collision =>
	// actor-origin fallback (shared convention; plain field buildings project
	// trivially from either).
	FVector NearPoint = FVector::ZeroVector;
	if (Goal.ActorGetDistanceToCollision(GetActorLocation(), ECC_Pawn, NearPoint) < 0.f)
	{
		NearPoint = Goal.GetActorLocation();
	}

	// (2) Project under the mover's OWN team filter — the SAME filter the move
	// request itself will use (null degrades to the navdata default filter, i.e.
	// the pre-feature unfiltered projection). The projected poly is by
	// construction a poly this unit is ALLOWED to path to, so goal resolution can
	// never fail the way the raw castle origin does.
	TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr;
	if (const AAIController* AI = GetAIController())
	{
		FilterClass = AI->GetDefaultNavigationFilterClass();
	}
	const FSharedConstNavQueryFilter QueryFilter = UNavigationQueryFilter::GetQueryFilter(*NavData, this, FilterClass);

	FNavLocation Projected;
	if (!NavSys->ProjectPointToNavigation(NearPoint, Projected, StructureGoalProjectionExtent, NavData, QueryFilter))
	{
		// nothing allowed within the extent (e.g. a goal buried deep inside the
		// ENEMY interior — unreachable by design): let the caller take the legacy
		// path rather than invent a far-away goal the unit cannot fight from.
		return false;
	}

	OutMarchPoint = Projected.Location;
	return true;
}

void ASummonedUnit::EnterAdvanceToLocation(const FVector& Point)
{
	// Point variant of EnterAdvance (W1 TASK-275, Shield Wall HOLD): marches to a world
	// LOCATION rather than an actor. Attack-exit bookkeeping is identical to EnterAdvance.
	if (State == ESummonedUnitState::Attack)
	{
		GetWorldTimerManager().ClearTimer(AttackTimerHandle);
		StopAttackLunge(); // leaving Attack: mesh back to EXACTLY the rest pose (TASK-020)
		RestoreLocomotionAnim(); // TASK-165 (rigged): end any attack clip so walk resumes; no-op for static units
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

	// Clear the ACTOR goal so a later EnterAdvance always re-paths (its goal-changed test
	// reads CurrentMoveGoal). (Re)path only when the target point moved meaningfully or the
	// last move finished/failed — the EnterAdvance re-path discipline, applied to a point.
	// QA delta (TASK-275, HOLD kite-out fix): EnterAdvance(AActor*) does NOT invalidate
	// bHasMoveGoalLocation/CurrentMoveGoalLocation, so a return to the SAME hold point right
	// after an actor-move (in-disc enemy chased, then it left the disc while still Moving)
	// would see bPointChanged==false and non-Idle status and SKIP re-issuing — leaving the
	// stale MoveToActor(enemy) live and kiting the unit out of position. Force a re-path when
	// the last move was an ACTOR move (captured BEFORE we null CurrentMoveGoal).
	const bool bWasActorMove = (CurrentMoveGoal != nullptr);
	CurrentMoveGoal = nullptr;
	const bool bPointChanged = !bHasMoveGoalLocation || !CurrentMoveGoalLocation.Equals(Point, 1.f);
	if (bWasActorMove || bPointChanged || AI->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(Point, StructureMoveAcceptanceRadius,
			/*bStopOnOverlap=*/ false, /*bUsePathfinding=*/ true, /*bProjectDestinationToNavigation=*/ true,
			/*bCanStrafe=*/ true, /*FilterClass=*/ nullptr, /*bAllowPartialPath=*/ true);
		CurrentMoveGoalLocation = Point;
		bHasMoveGoalLocation = true;

		if (Result == EPathFollowingRequestResult::Failed && bPointChanged)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASummonedUnit '%s': MoveToLocation toward %s failed — is the NavMeshBoundsVolume covering L_Arena (TASK-015)?"),
				*GetNameSafe(this), *Point.ToString());
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

	// STUCK WATCHDOG RESET — exit 1 of 4 (TASK-532, NAV-§3). Standing down ends the
	// sidestep: without this the lease would keep UpdateState returning early and steer a
	// unit that was just told to hold. Reset drops the anchor and the clocks too, so the
	// NEXT advance starts its ladder from zero rather than inheriting a stall that ended
	// here. Placed after the `State == Idle` early-out above deliberately — an
	// already-Idle unit cannot be holding a lease (arming it goes through
	// EnterAdvanceToLocation, which sets State = Advance).
	SidestepLeaseRemaining = 0.f;
	FSiegeStuckStatics::Reset(StuckState);

	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	StopAttackLunge(); // leaving Attack (or defensive from Advance): exact rest pose (TASK-020)
	RestoreLocomotionAnim(); // TASK-165 (rigged): back to idle locomotion; no-op for static units
	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}
}

void ASummonedUnit::PerformAttack()
{
	// bAIFrozen is defense-in-depth (TASK-028): FreezeAI clears the attack timer;
	// bSpellFrozen mirrors it for the resumable FrostNova freeze (TASK-099 —
	// ApplyFreeze also clears the timer, so this is a belt-and-braces gate).
	// ══ ANCIENT GROUNDS ATTACK SEAL — GUARD 3 of 3 (TASK-360, CONVENTIONS §3) ══
	// !CanEverAttack() joins the same defense-in-depth gate: guard 1 means the timer that
	// calls this can never be armed for a sealed unit, so this is the belt-and-braces layer
	// that also covers any FUTURE caller of PerformAttack (a serialized BP value, a new
	// state body) — the cost of missing it is a Cadence-0 unit landing 20 hits/s.
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || !CanEverAttack())
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

		// TASK-165: a rigged ranged unit still animates its attack (e.g. bow draw) — the
		// single-node clip is the feedback. Null-safe no-op for a non-rigged Archer (there was
		// never a lunge for ranged, so nothing regresses). No lunge/puff either way (TASK-028).
		PlaySkeletalAttackAnim();
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

		// attack feedback: the swing plays on every executed cadence hit; the impact puff only
		// when damage actually landed — a receiver that zeroed the hit (e.g. a castle destroyed
		// this same tick) gets no puff. Same return-value reading as the hero's TASK-016 dec. 1.
		// TASK-165: RIGGED units (skeletal visual active) play A_<CardID>_Attack — the TASK-020
		// procedural lunge would move the HIDDEN static mesh (invisible), so it is SKIPPED for
		// them and the anim is the feedback. Non-rigged / static-mesh units keep the lunge as the
		// fallback (a rigged unit whose attack clip didn't resolve just plays no swing — null-safe).
		if (bUsingSkeletalVisual)
		{
			PlaySkeletalAttackAnim();
		}
		else
		{
			StartAttackLunge();
		}
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

float ASummonedUnit::ConsumeStuckDeltaSeconds()
{
	// ⛔ THE WORLD CLOCK, NEVER StateCheckInterval (TASK-532, NAV-§3). AMinerUnit seals that
	// field to 0 (MinerUnit.cpp:61) and TASK-533 drives this same watchdog from
	// UpdateMining, so a delta read from it would be silently ZERO on every miner: the
	// ladder would never advance a rung and the feature would be a no-op on exactly the
	// unit class most likely to walk into a rock. This is a NAMED helper rather than two
	// inlined clock reads for that reason — one clock, one place to be wrong.
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	// static_cast: GetTimeSeconds() is float today, but narrowing it explicitly costs
	// nothing and this build treats warnings as errors.
	const float Now = static_cast<float>(World->GetTimeSeconds());

	// FIRST CALL ON THIS UNIT — there is no previous tick to measure from, so the tick is
	// INERT (the spec's explicit case). Latch the timestamp and charge the unit nothing.
	if (LastStuckTickTimeSeconds <= 0.f)
	{
		LastStuckTickTimeSeconds = Now;
		return 0.f;
	}

	// Clamped BOTH ways. Lower: a world clock that went backwards (PIE restart / time
	// reset) must never run the ladder in reverse. Upper: a GAP in the poll — a spell
	// freeze cleared StateTimerHandle, or a hitch — is not elapsed stall time and must not
	// be charged to the unit as though it had stood still through it.
	const float Delta = FMath::Clamp(Now - LastStuckTickTimeSeconds, 0.f, MaxStuckDeltaSeconds);
	LastStuckTickTimeSeconds = Now;
	return Delta;
}

void ASummonedUnit::TickStuckWatchdog(float DeltaSeconds)
{
	// The caller already clamps, but TASK-533 drives this too: Evaluate must never see a
	// negative delta, and a 0 delta must be inert rather than an assert.
	const float Delta = FMath::Max(DeltaSeconds, 0.f);

	// THE LEASE DRAINS ON THE SAME CLOCK AS THE LADDER, unconditionally and BEFORE the
	// evaluation — so a lease expires on schedule even on the polls where the unit is
	// moving fine and the ladder returns None immediately.
	if (SidestepLeaseRemaining > 0.f)
	{
		SidestepLeaseRemaining = FMath::Max(SidestepLeaseRemaining - Delta, 0.f);
	}

	// bAdvancing = "this unit has an ACTIVE path-following request THIS poll" — the shipped
	// idiom, the same GetMoveStatus() read the anti-repath gate at :2534 uses. ⛔ This stops
	// the ladder "rescuing" units that were TOLD to stand still: a holding, stationed or
	// idle-by-design unit has no live request, so Evaluate returns None however long it has
	// been motionless. No controller at all is likewise not-advancing.
	const AAIController* const AI = GetAIController();
	const bool bAdvancing = (AI != nullptr) && (AI->GetMoveStatus() != EPathFollowingStatus::Idle);

	// Captured BEFORE Evaluate: the Abandon rung RESETS the state inside Evaluate (NAV-§3),
	// so reading StalledSeconds afterwards would log 0.00 for the one rung that matters
	// most. On every escalating path Evaluate accumulates the delta first, so this is
	// exactly the value it compared against its thresholds.
	const float StalledAtEscalation = StuckState.StalledSeconds + Delta;

	// The whole per-unit cost of this feature: one DistSquared, one float compare, two adds
	// and a uint8 compare, with zero allocations and zero world queries — on the same
	// 0.25 s poll that already runs a full-world GetAllActorsWithInterface in AcquireTarget.
	const ESiegeStuckAction Action = FSiegeStuckStatics::Evaluate(
		bAdvancing,
		GetActorLocation(),
		static_cast<float>(GetVelocity().SizeSquared()),
		Delta,
		StuckTuning,
		StuckState);

	if (Action == ESiegeStuckAction::None)
	{
		return; // the overwhelmingly common path: moving normally, or idle by design
	}

	// ⛔ ONE LINE PER ESCALATION, NEVER ONE PER POLL (NAV-§7) — at 120 units a per-poll log
	// IS a cost, and this feature's whole claim is that it is cheap. It is throttled
	// STRUCTURALLY rather than by a timer: the ladder's own brakes (EscalationCooldown plus
	// the MONOTONIC EscalationLevel) cap it at three lines per stall.
	// Logged HERE rather than inside HandleStuckEscalation so AMinerUnit's override
	// (TASK-533) emits the same pinned tokens without having to remember to.
	// level= is the ACTION's ordinal, which IS the rung index by construction
	// (None 0 / Sidestep 1 / WidenAndRepath 2 / Abandon 3) and therefore survives the
	// Abandon reset that has already zeroed StuckState.EscalationLevel by this point.
	UE_LOG(LogSiegeStuck, Log, TEXT("escalate: unit='%s' level=%d action=%s stalled=%.2f"),
		*GetNameSafe(this), static_cast<int32>(Action), StuckActionToken(Action), StalledAtEscalation);

	HandleStuckEscalation(Action);
}

void ASummonedUnit::HandleStuckEscalation(ESiegeStuckAction Action)
{
	switch (Action)
	{
	case ESiegeStuckAction::Sidestep:
	{
		// The sidestep is perpendicular to the direction of TRAVEL, so it needs the goal
		// the unit is actually trying to reach: the live actor goal, else the last point
		// goal, else our own location — a degenerate input FSiegeStuckStatics answers with
		// a stable fallback axis rather than a NaN (its pinned contract).
		FVector StalledGoal = GetActorLocation();
		if (CurrentMoveGoal)
		{
			StalledGoal = CurrentMoveGoal->GetActorLocation();
		}
		else if (bHasMoveGoalLocation)
		{
			StalledGoal = CurrentMoveGoalLocation;
		}

		// ⭐ Attempt parity picks the side, and BOTH terms are load-bearing for different
		// reasons:
		//   • SidestepAttemptCount++ ALTERNATES THIS UNIT'S OWN SIDE ACROSS SUCCESSIVE
		//     STALLS — sidestep left, and if the unit wedges again, sidestep right. It is a
		//     free-running member of THIS class rather than a field of FSiegeStuckState
		//     precisely because Reset clears that struct on every Abandon and every
		//     re-anchor (FSiegeStuckStatics::Reset), so nothing kept there survives a stall.
		//     ⛔ Deriving this term from StuckState.EscalationLevel — the first shipped
		//     version — is a per-unit CONSTANT: Evaluate assigns the level BEFORE its
		//     switch, so it is ALWAYS exactly 1 here, and brake 2 lets Sidestep fire only
		//     once per stall. A unit wedged on a rock's left face then stepped into that
		//     rock every stall, forever (qa/TASK-537.md, the BLOCKER).
		//   • GetUniqueID() % 2 DE-CORRELATES NEIGHBOURS, so a clump wedged on the SAME rock
		//     does not all sidestep the same way into each other — one stuck unit becoming
		//     several. It is a fixed per-unit offset, so it shifts the phase without ever
		//     defeating the alternation above.
		// The post-increment wraps at 255 -> 0; harmless, because only the parity is read and
		// 256 is even (see the member's comment). Non-negative by construction, so the
		// callee's parity test is well defined either way.
		const int32 Attempt = static_cast<int32>(SidestepAttemptCount++)
			+ static_cast<int32>(GetUniqueID() % 2);

		SidestepGoal = FSiegeStuckStatics::ComputeSidestepGoal(
			GetActorLocation(), StalledGoal, StuckTuning.SidestepDistance, Attempt);

		// ARM THE LEASE BEFORE ISSUING, so the state is consistent whatever the request
		// returns, then make EXACTLY ONE path request through the existing point mover.
		// UpdateState's lease early-out re-issues nothing, so this single request is the
		// rung's entire cost — which is what keeps the NAV-§3 worst case at ≤1 extra path
		// request per unit per second.
		SidestepLeaseRemaining = StuckTuning.SidestepLeaseSeconds;
		EnterAdvanceToLocation(SidestepGoal);
		break;
	}

	case ESiegeStuckAction::WidenAndRepath:
	{
		// The ladder has climbed PAST the sidestep, so the sidestep is over: drop the lease
		// FIRST, or UpdateState's early-out would suppress the very re-path this rung
		// exists to cause — and because EscalationLevel is monotonic the rung would never
		// fire again. This ordering is the difference between a working rung and a no-op.
		SidestepLeaseRemaining = 0.f;

		// A GENUINE re-path toward the ORIGINAL goal, by invalidating the "we already asked
		// for this" bookkeeping — NAV-§3's own sanctioned mechanism ("clear CurrentMoveGoal
		// so the next EnterAdvance* really re-issues"). Expressed as invalidation rather
		// than a widened radius threaded through the movement calls because the law forbids
		// widening anything that PERSISTS past the stall, and every acceptance radius in
		// this file is a shared constant the normal path reads too.
		// All three latches, because three different bodies own them and any one of them
		// left set would swallow the re-issue: the actor gate (:2533), the point gate
		// (:2676-2677), and the follow body's own LastFollowGoalLocation drift latch.
		// ⛔ THE STANDING ORDER IS UNTOUCHED — CommandGroupId, the group and CurrentTarget
		// all survive; the unit re-paths to the SAME goal, it does not get a new one.
		CurrentMoveGoal = nullptr;
		bHasMoveGoalLocation = false;
		bHasFollowGoalLocation = false;
		break;
	}

	case ESiegeStuckAction::Abandon:
	{
		SidestepLeaseRemaining = 0.f;

		// Six seconds of not moving while genuinely trying to: this goal is not reachable
		// right now. Drop the move goal AND the acquired target, then stand down. EnterIdle
		// is the mechanism NAV-§3 names, and clearing CurrentTarget is what makes the next
		// poll a RE-TARGET instead of a re-chase of the very thing we just failed to reach.
		// ⛔ NOT a cancellation of the standing ORDER: CommandGroupId is untouched, so a
		// grouped unit is re-dispatched by its group on the very next poll and an ungrouped
		// one re-acquires through the normal AcquireTarget path. ⛔ The unit is never left
		// inert forever — EnterIdle is a stand-down, not a stop — and ⛔ it never enters
		// Attack from here.
		// ⛔ No Reset call: Evaluate ALREADY reset the state when it returned Abandon
		// (NAV-§3's rung table). The explicit clears below still earn their place because
		// EnterIdle early-outs when the unit is already Idle.
		CurrentMoveGoal = nullptr;
		bHasMoveGoalLocation = false;
		bHasFollowGoalLocation = false;
		CurrentTarget = nullptr;
		EnterIdle();
		break;
	}

	case ESiegeStuckAction::None:
	default:
		// TickStuckWatchdog never dispatches None; enumerated so the switch is total and a
		// future rung cannot be added to the enum and silently fall through here.
		break;
	}
}

void ASummonedUnit::NotifyMoveBlocked()
{
	// ⛔⛔ EVIDENCE, NOT ACTION (NAV-§3, the no-double-driver law) — read the header comment
	// before adding ANYTHING to this function. It may not issue a move and it may not call
	// HandleStuckEscalation. It records what the engine observed and leaves the decision to
	// the next TickStuckWatchdog, which is the ONE driver — so the escalation still passes
	// through EscalationCooldown and the monotonic EscalationLevel. The tempting "we
	// already know it is blocked, just re-path here" is the TASK-280/282 mill wearing a
	// rescue's clothes: two code paths re-pathing one UPathFollowingComponent.
	if (bDead || bAIFrozen || bSpellFrozen)
	{
		return; // a unit that makes no decisions collects no evidence
	}

	// Anchor first if the ladder has never seen this unit: Evaluate's cheap path re-anchors
	// (and zeroes the clocks) for a unit with no anchor, which would discard the clock bump
	// below on the very next poll.
	if (!StuckState.bHasAnchor)
	{
		StuckState.ProgressAnchor = GetActorLocation();
		StuckState.bHasAnchor = true;
	}

	// Advance the stall clock TO the first rung's threshold — never PAST it, and never
	// backwards (FMath::Max). So this can only ever pull the FIRST escalation forward by
	// one poll; it can never skip Sidestep and jump a unit straight to Abandon.
	StuckState.StalledSeconds = FMath::Max(StuckState.StalledSeconds, StuckTuning.SidestepSeconds);
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

	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — COMPOSE POINT 1 of 2. The permanent stacking
	// boost is applied at STRIKE TIME exactly like the aura above and is likewise stored
	// separately from AttackDamage, which is NEVER mutated in place (the house buff law). This
	// ONE insertion covers BOTH delivery modes — melee (ApplyDamage below in PerformAttack) and
	// ranged (the composed OutputDamage carried into FireProjectileAt) — and every keyword unit,
	// because they all funnel through this function. Multiplicative and EXACTLY 1.0 at zero
	// stacks, so an unboosted unit still returns AttackDamage bit-for-bit (the M1/M2
	// non-regression this function's contract promises).
	Output *= GetPermanentDamageMultiplier();

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

	// TASK-298: spawn the per-unit ProjectileClass override when authored (the Wizard's
	// BP_Projectile_Fireball), else the base AProjectile — null-safe fallback keeps every
	// existing ranged unit (ProjectileClass null) byte-for-byte identical to before.
	const TSubclassOf<AProjectile> SpawnClass = ProjectileClass ? ProjectileClass.Get() : AProjectile::StaticClass();
	if (AProjectile* Projectile = World->SpawnActor<AProjectile>(SpawnClass, FTransform(FireRotation, MuzzleLocation), SpawnParameters))
	{
		// own team, current target, the CENTRALIZED output damage (TASK-055: row Damage × aura,
		// etc. — for a plain Archer this is exactly the row Damage), projectile-typed — ACastle
		// applies the §3.0 50% on ITS side (TASK-026); units/hero take the listed damage.
		// TASK-298: pass the row-bound AoERadius as the 5th arg. 0 (Archer/Longbowman/tower
		// callers) takes the UNCHANGED single-target path; > 0 (Wizard 250) resolves the impact
		// as FSiegeCombatStatics::ApplyRadialDamage in AProjectile::HandleImpact — the proven
		// TASK-056 Bomb-Tower splash (enemies in radius only, no friendly fire, castle 50% kept).
		Projectile->InitProjectile(Team, Target, DamageAmount, USiegeDamageType_Projectile::StaticClass(), AoERadius);

		// §6 projectile-fire audio (TASK-179): world one-shot at the muzzle, null-safe.
		USiegeFeedbackLibrary::PlayWorldSound(this, ProjectileFireSoundPath, MuzzleLocation);
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
	// the LastAttackTime gate in EnterAttack). TASK-099: clamps against the
	// EFFECTIVE cadence, so a Battle Cry-hastened unit's lunge still completes
	// before its faster next hit (>= MinAttackCadence either way, so the clamp
	// can never produce a zero-length cycle on its own).
	const float CycleDuration = FMath::Min(AttackLungeDuration, 0.8f * GetEffectiveAttackCadence());
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

	// Push the damage to the overhead bar BEFORE any death handling (ACastle::TakeDamage
	// parity — listeners see the 0-HP value before HandleDeath tears the actor down).
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// §6 damage feedback (TASK-154/156) — only on ACTUAL damage (friendly fire + heals
	// never reach here). Flash the active visual white ~0.1 s; float the dealt amount
	// over the unit, tinted by team. Both null-safe (no art = no-op) and run before
	// HandleDeath so the killing blow still flashes/pops.
	if (HitFlashComponent)
	{
		HitFlashComponent->TriggerFlash();
	}
	USiegeFeedbackLibrary::ShowDamageNumber(this, ActualDamage,
		GetActorLocation() + FVector(0.f, 0.f, UnitDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));

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

	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — COMPOSE POINT 2 of 2, and a DELIBERATE
	// behavior change to a shipped unit. This path passed RAW AttackDamage, bypassing the
	// ComputeOutputDamage chokepoint entirely; dying IS how a Sapper attacks, so an unboosted
	// blast from a fully-boosted Sapper would be a visible lie. AttackDamage itself is still
	// never mutated — the multiplier composes into a LOCAL, exactly as everywhere else, and is
	// exactly 1.0 for every unboosted Sapper (so the shipped 80-damage blast is unchanged).
	// Charge/Slayer/Aura are deliberately NOT retro-applied here: that is a separate
	// pre-existing gap, recorded in the handoff, not fixed by this task.
	const float BlastDamage = AttackDamage * GetPermanentDamageMultiplier();

	// AoE at our feet: row Damage (Sapper 80) over row AoERadius (250), Siege-typed so ACastle/
	// ABuilding scale it to 200% (TASK-054); enemies only, no friendly fire. Nothing hardcoded —
	// AttackDamage and AoERadius are bound from the DT_Cards row. Our controller is the instigator
	// for attribution; the shared helper's Team filter is the friendly-fire authority (TASK-056
	// reuses this same call for the Bomb Tower).
	FSiegeCombatStatics::ApplyRadialDamage(World, GetController(), Team, GetActorLocation(),
		AoERadius, BlastDamage, USiegeDamageType_Siege::StaticClass());
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

	// Push the final 0-HP to the overhead bar (TASK-130 push model — covers death paths that
	// do not route through TakeDamage, e.g. Sapper suicide). The actor is destroyed below,
	// taking the bar with it, so no explicit hide is needed.
	OnHPChanged.Broadcast(CurrentHP, GetMaxHP());

	// ANCIENT GROUNDS (TASK-360, CONVENTIONS §4) — the boost is lost ON DEATH (Jonathan's
	// directive: "stays with units even after they leave, until they die"), and this push is
	// REQUIRED, not cosmetic: the rigged-death path below defers Destroy by up to
	// DeathAnimMaxHoldSeconds (2 s), so without it a boosted corpse would hold a full boost bar
	// through its whole death anim. Placed AFTER the bSuicide ApplyDetonation above on purpose —
	// a boosted Sapper's death blast is still boosted (compose point 2), and only then does the
	// unit lose its stacks. Play Again needs ZERO work (its step 2 destroys every unit); the
	// match-end freeze deliberately does NOT reset (a permanent boost survives to the end screen).
	ClearPermanentDamageStacks();

	// §6 gold-coin burst on unit death (TASK-158): cosmetic only, NO gold mutation.
	// Hooked into THIS single death choke (covers combat death, Sapper suicide, the
	// PlayAgain sweep, KillZ). Null-safe until NS_GoldBurst lands (TASK-174). Spawned
	// BEFORE Destroy() so the world + location are valid.
	USiegeFeedbackLibrary::SpawnNiagara(this, GoldBurstVFXPath, GetActorLocation());

	GetWorldTimerManager().ClearTimer(StateTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);
	GetWorldTimerManager().ClearTimer(AttackAnimRestoreTimerHandle); // TASK-165: no attack-anim restore during the death hold
	StopHealing(); // TASK-054: a dying Cleric heals no one

	// STUCK WATCHDOG RESET — exit 3 of 4 (TASK-532, NAV-§3). The StateTimerHandle clear
	// above already stops the watchdog, so this is the zero-residual half of the contract:
	// a rigged unit's Destroy is deferred by up to DeathAnimMaxHoldSeconds (2 s), and a
	// corpse must not spend that window holding a live lease or a half-climbed ladder that
	// a debug readback would report. Same discipline as the StopAttackLunge below it.
	SidestepLeaseRemaining = 0.f;
	FSiegeStuckStatics::Reset(StuckState);

	// death restores the exact rest pose before the actor goes away (TASK-020 contract)
	StopAttackLunge();

	if (AAIController* AI = GetAIController())
	{
		AI->StopMovement();
	}

	// TASK-165 skeletal death anim: a rigged unit plays A_<CardID>_Death (single-node, non-looping)
	// and HOLDS its final pose; the Destroy is deferred a small capped moment so the anim reads.
	// A non-rigged unit — or a rigged unit with no resolved death clip — returns 0 and is destroyed
	// immediately, byte-for-byte the M1..M6 behavior. ShouldHoldDeathAnim() lets AMinerUnit opt out
	// entirely so its §3.3 economy bookkeeping (EndPlay) stays prompt (no hold-window delay).
	const float DeathHoldSeconds = ShouldHoldDeathAnim() ? PlaySkeletalDeathAnim() : 0.f;
	if (DeathHoldSeconds > 0.f)
	{
		// The corpse lingers ONLY to finish its death anim. Freeze it in place first: DisableMovement
		// (MOVE_None) so the character does NOT fall — disabling capsule collision below removes the
		// floor the CharacterMovementComponent stands on, which would otherwise drop the body under
		// gravity through the hold. Then disable actor collision so the corpse never blocks a living
		// unit's path (a dead unit is already skipped by IsTargetAlive) and hide the now-0-HP overhead
		// bar. The skeletal mesh keeps animating (its own component tick is independent of the capsule
		// and actor tick). EndPlay clears the timer if something removes the actor first (match reset /
		// KillZ Destroy). The §6 gold-burst already fired above, so the death still reads on immediate
		// removal too.
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		SetActorEnableCollision(false);
		if (HPBarWidget)
		{
			HPBarWidget->SetVisibility(false);
		}
		GetWorldTimerManager().SetTimer(DeathDestroyTimerHandle, this, &ASummonedUnit::FinishDeathDestroy, DeathHoldSeconds, /*bLoop=*/ false);
		return;
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
