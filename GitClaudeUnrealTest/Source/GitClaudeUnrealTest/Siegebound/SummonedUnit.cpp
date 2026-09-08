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
#include "Siegebound/ClimbableTower.h" // TASK-784 (CONTACT-§4.1): TryBeginContactClimb + GetLadderLink — complete type needed for the ASK. ⛔ .cpp ONLY; the header holds nothing but a forward declaration, because this feature adds ⛔ no member of that type (or of any type)
#include "Siegebound/DamageTypes.h"
#include "Siegebound/CombatantHealthBarComponent.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/Projectile.h"
#include "Siegebound/SiegeCombatStatics.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeInvisibilityStatics.h" // TASK-829 (WITCH-§6): FSiegeInvisibilityStatics::ApplyVeil / ApplyBreak — the ONLY two writers of bIsInvisible (also reached via SummonedUnit.h, which needs the complete type for BreakInvisibility's parameter — explicit per IWYU, the SiegeStuckStatics.h precedent below)
#include "Siegebound/SiegeLadderClimbStatics.h" // TASK-776 (CONTACT-§2): the climb's pure rules + FSiegeLadderClimbState (also reached via SummonedUnit.h, which needs the complete type for its member — explicit per IWYU, the SiegeStuckStatics.h precedent below)
#include "Siegebound/SiegeMeshJuiceComponent.h"
#include "Siegebound/SiegeNavAreas.h" // TASK-349: team object channels + ASiegeUnitAIController (complete types for the gating stamp)
#include "Siegebound/SiegeSpawnConstants.h" // TASK-738: DefaultCapsuleHalfHeight — the null-capsule fallback for the climb's surface->centre lift
#include "Siegebound/SiegePlayerController.h" // W1 TASK-275: reads the latched Shield Wall command (GetCurrentCommand/HasIssuedCommand); TASK-344: resolves the live group (FindUnitGroup) — complete type needed for the const calls
#include "Siegebound/SiegeStuckStatics.h" // TASK-532: the pure stall ladder + LogSiegeStuck (also reached via SummonedUnit.h, which needs the complete types for its members — explicit per IWYU)
#include "Siegebound/UnitCommand.h" // TASK-344: FSiegeUnitGroup complete type (UpdateStateGrouped reads its zones/type) — explicit include, not just via the controller header
#include "TimerManager.h"
#include "UObject/UObjectHash.h" // TASK-784 (CONTACT-§4.1): ForEachObjectOfClass — the CLASS-HASH bucket walk that finds the live ladders WITHOUT the TActorIterator level scan every other finder in this file pays

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

	/**
	 *  ⭐⭐ THE WITCH'S ROW NAME (TASK-830; `WITCH-§6` pins the `CardID` ⛔ character-for-character).
	 *  ⛔ ONE spelling, in ⛔ one place: `ASummonedUnit::IsVeilCaster()` is its ⛔ only reader, and
	 *  it is the ⛔ same name that selects `/Game/Blueprints/Units/BP_Unit_Witch` at spawn and the
	 *  `Witch` row `TASK-831` writes into `DT_Cards`. ⛔ A second literal anywhere would be a second
	 *  spelling of an identity, which is the class of defect a rename ⛔ half-fixes.
	 *  ⛔ Deliberately ⛔ NOT an `EditDefaultsOnly` property: mechanic identity is ⛔ not a card stat
	 *  (the `CanEverAttack()` law), and a designer-editable identity is a mechanic that can be
	 *  ⛔ switched off from a details panel.
	 */
	const FName WitchCardID(TEXT("Witch"));
}

namespace
{
	/**
	 *  Acceptance radius when advancing on a castle. The castle's box collision is huge
	 *  (7313.7 x 7384.5 uu footprint), so overlap-based reach tests against its bounding
	 *  CYLINDER (radius ~5175) would stop the unit well outside attack range on a flat
	 *  wall face. Instead the move targets the castle origin with bStopOnOverlap = false
	 *  and relies on the partial path ending at the nav edge flush against the walls.
	 *  ⚠️ CITATION CORRECTED BY TASK-574 (uncited instance of WR-§2b row G, found by this
	 *  task's SC-§22 shape sweep): both figures quoted the M1 castle ("~800x800", cylinder
	 *  "~566") and were two remasters stale. ⛔ THE 50 uu VALUE IS DELIBERATELY UNCHANGED
	 *  (SC-§34 (ii)): it is a MOVE-ACCEPTANCE tolerance keyed to a unit's body and its
	 *  path-follow granularity, not to any castle dimension — and the argument the comment
	 *  makes gets STRONGER as the castle grows, never weaker.
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
	// Tick starts disabled and is enabled only while one of its TWO drivers wants
	// it (RefreshActorTickEnabled): the <= 0.8×Cadence attack-lunge visual
	// (TASK-020), and the ladder ascent (TASK-738), which must steer the movement
	// component every frame. Both are bounded episodes; the state machine, the
	// acquisition and the attack cadence still never tick.
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
	//
	// ⛔ SC-§35 item 1 — THE OWNER-CLASS CONTRACT, the half the paragraph above does not
	// state (added by TASK-596, from qa/TASK-592.md R7): BOTH classes resolved below — the
	// composed per-unit ABP_<CardID> and the shared ABP_Footman fallback — ASSUME A PAWN
	// OWNER. ABP_Footman's EventGraph drives its Set GroundSpeed / Set bIsMoving nodes
	// through UAnimInstance::TryGetPawnOwner(), which returns None on a non-Pawn owner.
	// ASummonedUnit is an ACharacter (see the class declaration in SummonedUnit.h), i.e. a
	// Pawn — which is exactly why ABP_Footman is a LEGAL value HERE and this assignment is
	// CORRECT. Assigning either class to a non-Pawn actor is the exact defect SC-§35 was
	// written for: 1,806 Blueprint runtime errors in 49 s with 2 NPCs alive, through a
	// clean compile, 111/111 tests and two QA gates. The composed path below is the shape
	// future unit authors will copy — if the copying class is NOT a Pawn, the repair is
	// made at that consumer (single-node playback, SC-§35 items 2 + 3), ⛔ never by
	// editing the shared ABP_Footman.
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

	// TASK-830 (WITCH-§4): no dangling veil cast on a destroyed unit — and, more to the point,
	// no stale IncomingWitchCaster left on a SURVIVING subject. Both directions are weak and
	// would self-heal, but a subject carrying a back-pointer to a destroyed witch would be
	// skipped by every other witch's "not-currently-being-veiled" term until something polled it.
	CancelWitchCast(TEXT("the witch was destroyed"));
	InterruptIncomingWitchCast(TEXT("the subject being veiled was destroyed"));

	// ══ LADDER EXIT 7 of 8 — THIS UNIT IS TORN DOWN (TASK-738, TOWER-§8.5) ═══════════════════
	// ⭐ AND IT IS ALSO THE INDEPENDENT BELT FOR EXIT 8, THE TOWER DYING MID-CLIMB. TOWER-§8.5
	// says AClimbableTower::EndPlay must abort every climber it started — ⛔ but it also says do
	// NOT rely on that alone, and this is why: the tower's occupant bookkeeping is the tower's,
	// and a climber it lost track of would otherwise be a unit with a dangling delegate binding
	// and no completion signal. Ordinary teardown for a non-climbing unit costs one bool test.
	// ⚠️ BEFORE Super::EndPlay — the actor must still be intact when the delegate fires.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::EndPlay);

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
	//
	// ⚠️⚠️ TASK-923 — A ⛔ FORWARD HAZARD ON THIS BRANCH, ⛔ PLACED HERE BECAUSE THIS IS WHERE THE
	// NEXT EDITOR WILL BE STANDING. ⛔ Prose only; ⛔ nothing below changed. ApplyTeamMaterial
	// writes an ⛔ OPAQUE MI_TeamColor_<Team> to slot 0, so running it on a ⛔ VEILED unit would
	// punch an opaque patch through the veil — ⛔ exactly WITCH-§5's "opaque chrome hat over a
	// ghostly body", arriving through a door nobody was watching.
	// ✅ ⛔ UNREACHABLE TODAY, ⛔ MEASURED rather than assumed: BOTH shipped InitUnit callers
	// (ABarracks::SpawnUnit and ASiegePlayerController's deferred spawn) are
	// SpawnActorDeferred → InitUnit → FinishSpawning, so HasActorBegunPlay() is ⛔ FALSE and this
	// branch has ⛔ ZERO reachable call sites. A veil additionally requires a 3-second witch cast,
	// which cannot land before the unit exists. ⇒ ⛔ no guard is shipped: one here would be a
	// ⛔ THIRD site at which the veil changes a unit's look, which WITCH-§6 refuses.
	// ⛔⛔ IF A "re-team a LIVE unit" PATH IS EVER ADDED, ⛔ THIS IS THE LINE THAT NEEDS THE VEIL
	// CONSULT — ⛔ and it is a WITCH-§ amendment (the third site has to be RULED), ⛔ never a quiet
	// `if (IsInvisible())` dropped in here.
	// ⚠️ THE ACCESSOR SPELLING ABOVE IS ⛔ DELIBERATE AND ⛔ LOAD-BEARING, ⛔ not a style choice:
	// SiegeAcquisitionFunnelTest pins a PROSE-IMMUNITY row over this file asserting that its veil
	// needles read IDENTICALLY with comment-skipping on and off. One of those needles is an open
	// paren immediately followed by the raw flag name — so writing the RAW FLAG inside parentheses
	// ⛔ anywhere in this file, ⛔ including in a comment like this one, turns a suite row RED with
	// nothing in the behaviour to explain it. ⛔ Name the accessor in prose, never the field.
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

	// ══ LADDER EXIT 5 of 8 — THE MATCH-END FREEZE (TASK-738, TOWER-§8.5) ═════════════════════
	// ⚠️⚠️ THIS ONE WOULD BE INVISIBLE WITHOUT THE RESTORE, AND IT IS THE CLEAREST CASE IN THE
	// WHOLE SET: FreezeAI stops timers, stops the walk and parks the unit Idle — it does ⛔ NOT
	// touch the movement mode at all. A climber frozen at match end would therefore keep
	// MOVE_Flying, and MOVE_Flying ignores gravity, so it would HANG IN THE AIR over the end
	// screen with every other line of this function perfectly correct.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::MatchEndFreeze);

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

	// stop the veil cast (TASK-830, WITCH-§4) — the exact mirror of the two lines above, for the
	// exact same reason: a match-end-frozen witch veils no one. ⛔ WITHOUT THIS the cast timer is
	// the ONE timer in this function that would survive the freeze and fire onto the end screen,
	// because it is not StateTimerHandle and not AttackTimerHandle. Both ends, so no frozen
	// subject is left marked as spoken-for.
	CancelWitchCast(TEXT("the match-end freeze"));
	InterruptIncomingWitchCast(TEXT("the match-end freeze"));

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

	// ══ LADDER EXIT 6 of 8 — THE SPELL FREEZE (TASK-738, TOWER-§8.5) ═════════════════════════
	// ⚠️ ORDER IS LOAD-BEARING, THE SAME WAY IT IS IN HandleDeath: this runs BEFORE the
	// DisableMovement() below, so the freeze's deliberate MOVE_None wins over our restore.
	// ⚠️ DECLARED RESIDUAL, ⛔ NOT A DEFECT: a unit frost-frozen mid-ascent is therefore parked
	// in MOVE_None IN MID-AIR for the freeze's duration, and drops when EndSpellFreeze calls
	// SetDefaultMovementMode. That is the shipped freeze contract applied to a new surface
	// ("the pause holds"), ⛔ not a hang: the climb is over, IsClimbing() is false, the unit is
	// re-armed, and a mode restore is guaranteed by the expiry timer — with FreezeAI's
	// match-end precedence as the backstop if the match ends first.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::SpellFreeze);

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

	// ⛔ THE CAST DIES, IT DOES NOT PAUSE (TASK-830, WITCH-§4). A frost-frozen witch is parked for
	// the duration, and a cast that resumed on thaw would have channelled through the freeze —
	// which is a longer effective cast the player never sees. An interrupted cast costs ⛔ nothing,
	// so she simply re-acquires on her first post-freeze poll (the shipped Cleric contract: "the
	// next post-freeze state check re-acquires a heal target from scratch").
	// ⛔ ONE end only, deliberately: this is a spell freeze on ONE actor. A frozen SUBJECT is still
	// a legal subject — she is unharmed and still channelling, and being frozen is being ACTED
	// UPON, which WITCH-§4 does not list among its cancels.
	CancelWitchCast(TEXT("the caster was frozen"));

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
	// ⭐⭐ TASK-979 — THE PER-UNIT NOTICE CHANNEL, bound here and deliberately BESIDE
	// `AttackRange = Row->Range` above: they are the two halves of a card's reach, and a reader
	// who finds one must find the other. Before this line AggroRadius was a flat profile
	// constant that no card could influence, which is why a per-card reach needed a CHANNEL
	// rather than a number (FOG-§9.9).
	// ⛔ The row is applied through ResolveNoticeRadiusUU and NOT written straight over the
	// member: that function carries the class seal (a non-positive class default ignores the
	// row, so a card cell can never un-seal AMinerUnit/ASorcererUnit) and the refusal to clamp
	// (a finite positive cell comes back out UNCHANGED — ⛔ neither clamped down nor widened up).
	// ⛔ The class default is read off THIS INSTANCE'S CLASS, never GetDefault<ASummonedUnit>() —
	// the Building.cpp:302 defect.
	// 📌 SPARSE, AND ⛔ RE-STATED BECAUSE THE OLD SENTENCE IS FALSE IN ITS PREMISE (TASK-1003):
	// it said *"cards.csv has no NoticeRange column yet"*. ⛔ The column EXISTS now (TASK-993
	// added it) — what is empty is every CELL in it. TASK-993's one populated cell was the
	// Longbowman's 3600, and 🧑 his 5000 ruling retired it (TASK-1004 blanks it: at a 5000 class
	// default a 3600 cell would make that card notice LESS than everyone else, inverting the
	// exception it existed for). ⇒ every row deserializes 0, every unit resolves to its class
	// default, this line is a ⛔ behaviour-preserving no-op for the whole shipped roster, and the
	// 600 → 5000 change is the CDO default ALONE. ⭐ An entirely sparse column is the CORRECT
	// shape, ⛔ not an unfinished one: the channel is what ships, and a future card opts in from
	// its own row with zero code.
	AggroRadius = ResolveNoticeRadiusUU(GetClassDefaultEngagementRadiusUU(), Row->NoticeRange);
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
	// ══ THE SELF-HEALING CLIMB DRIVER (TASK-760, closing TASK-741's W-1) ═════════════════════
	// ⚠️⚠️ THE GAP IT CLOSES IS IN THE **NET**, ⛔ NOT IN THE CODE. TickLadderClimb advances the
	// timeout clock AND is the only thing that can end a hung climb ⇒ THE WATCHDOG RIDES THE VERY
	// DRIVER IT WATCHES. Any external write of the actor tick flag — a BP_Unit_* child, a level
	// Blueprint, tomorrow's C++ site — leaves the climber in MOVE_Flying FOREVER, and the watchdog
	// CANNOT fire to rescue it because it is on the dead driver. That is precisely the
	// StopAttackLunge defect TASK-738 found and fixed; fixing the one known writer closed the
	// CAUSE, and this closes the CLASS.
	//
	// ⭐⭐ THE PLACEMENT ABOVE THE FENCE IS THE WHOLE POINT, ⛔ NOT a formatting choice: the fence
	// below early-outs on IsClimbing(), so a self-heal one line lower would be DEAD CODE for the
	// exact case it exists to rescue. It also sits above `!bStatsLoaded`, which BeginLadderClimb
	// deliberately does not test (TOWER-§8.4(B)'s four pinned reasons), so no armed climb can ever
	// be fenced out of its own rescue.
	//
	// ⭐ IT RIDES A DRIVER THE HAZARD CANNOT REACH, AND ADDS ⛔ ZERO NEW TIMERS: StateTimerHandle's
	// 0.25 s poll is an FTimerManager entry on the WORLD, which knows nothing about
	// PrimaryActorTick, and BeginLadderClimb clears AttackTimerHandle ONLY — so this poll keeps
	// landing for the whole climb. Every other clear site (EndPlay, HandleDeath, FreezeAI,
	// ApplyFreeze) is itself one of the eight exits, so it has already ended the climb and this
	// test is false there. ⇒ a killed tick flag is restored within one poll, ~0.25 s.
	//
	// ⛔ RefreshActorTickEnabled, ⛔ NEVER a bare SetActorTickEnabled(true): the composed predicate
	// is the OR of the two drivers, so this re-assert can only ever write the value the writer
	// would have written anyway. A bare `true` would fight whatever legitimately disabled the tick
	// and re-introduce the exact coupling TASK-738 removed.
	// Cost: one bool test per poll per unit.
	if (LadderClimb.bActive)
	{
		RefreshActorTickEnabled();
	}

	// bAIFrozen is defense-in-depth (TASK-028): FreezeAI clears this timer, but a
	// frozen unit must make no decisions even if something ever re-armed it.
	// bSpellFrozen mirrors it for the resumable FrostNova freeze (TASK-099).
	//
	// ══ THE TRAVERSAL FENCE (TASK-738, TOWER-§8.5) — ⛔ AND IT IS NOT A FOURTH ATTACK GUARD ══
	// ⚠️ READ THIS BEFORE FILING IT AS ONE: the TOWER-§9 disarm is the three CanEverAttack()
	// points (EnterAttack / UpdateStateGrouped / PerformAttack) and nowhere else. THIS term is a
	// MOVEMENT concern, and without it the feature does not work at all: a state poll landing
	// mid-ascent would run the standing body, acquire a target and call EnterAdvance — handing
	// the pawn to PATH FOLLOWING while it is in MOVE_Flying. Path following writes the same input
	// vector the climb steers with (UPathFollowingComponent::FollowPathSegment ->
	// RequestPathMove -> AddInputVector), so the unit would be dragged horizontally off its own
	// ladder by a second steering authority — the exact double-drive NAV-§3 forbids.
	// ⭐ It is also what makes the disarm CHEAP: a climbing unit runs no decision loop at all,
	// so the three guard points are belt rather than the mechanism.
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || IsClimbing())
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
	//
	// ⚠️ TASK-784 HOISTED THIS INTO A LOCAL, AND IT IS A ⛔ BEHAVIOURALLY NEUTRAL EDIT WORTH ONE
	// SENTENCE: ConsumeStuckDeltaSeconds is a CONSUMING read (it latches LastStuckTickTimeSeconds),
	// so calling it a second time for the contact poll would hand that poll a ~0 delta and the
	// tower's dwell would never advance. ⭐ ONE clock read, ONE value, TWO consumers — which is
	// exactly what the "one clock, one place to be wrong" note on that helper asks for. The
	// watchdog receives the identical value it always did.
	const float PollDeltaSeconds = ConsumeStuckDeltaSeconds();
	TickStuckWatchdog(PollDeltaSeconds);

	// ⭐⭐ THE WITCH'S CAST DRIVER (TASK-830, WITCH-§4) — ONE line, and its placement is the
	// SAME argument TickStuckWatchdog's and the contact trigger's carry, applied to a third
	// feature: it sits ABOVE the follow hoist and ABOVE the profile dispatch, so Standard,
	// Siege, Support, Follow and Hold/Ambush are all covered from here and the cast does not
	// have to be scattered into five bodies. That is what makes his "controllable by all
	// commands" cost zero per-body wiring — a witch under ANY order, or none, casts identically.
	// It also sits ABOVE the contact-climb and sidestep-lease early-outs on purpose: a witch
	// stalled on a rock or standing at a ladder is still a witch, and her in-flight cast must
	// keep being VALIDATED (a subject that dies while she is wedged must still cancel it).
	// ⛔ ZERO new timers — this rides the 0.25 s StateTimerHandle poll that already exists.
	// Cost for every non-witch in the game: one IsVeilCaster() compare per poll.
	UpdateWitchCast();

	// ⭐⭐ THE CONTACT TRIGGER'S CALL SITE (TASK-784, CONTACT-§4.1) — ONE line, placed for EXACTLY
	// the reason TickStuckWatchdog's is: it sits ABOVE the follow hoist and ABOVE the profile
	// dispatch, so Standard, Siege, Support, Follow and Hold/Ambush are all covered from here and
	// no ask has to be scattered into the individual bodies. It sits ABOVE the sidestep-lease
	// early-out too — a unit being rescued from a rock is still a unit standing at a ladder, and
	// climbing IS an escape (BeginLadderClimb clears the lease and resets the stall state itself,
	// in its own step (5)).
	//
	// ⛔⛔ THE RETURN IS ⛔ NOT OPTIONAL, AND IT IS THE SECOND HALF OF "ONE STEERING AUTHORITY":
	// BeginLadderClimb's step (4) has already called AAIController::StopMovement(), so falling
	// through to the profile dispatch below would run EnterAdvance/EnterAttack and hand the pawn
	// straight back to path following — which writes the same input vector the climb steers with
	// (the NAV-§3 double-drive). ⭐ Every LATER poll is fenced out by the IsClimbing() early-out
	// above; this return is what covers the ONE poll the climb starts on.
	// ⛔ No second StopMovement is issued here — the climb's own driver owns that.
	// ZERO new timers: this rides the 0.25 s StateTimerHandle poll that already exists.
	if (TryContactClimbAtNearestLadder(PollDeltaSeconds))
	{
		return;
	}

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
		// ⭐ THE WITCH IS ECardProfile::Support (TASK-830 chose the shipped Cleric profile rather
		// than inventing a fourth — she never attacks, she is follow-eligible for free, and the
		// Cleric's "walk at the friendly you are working on" body is her body with one noun
		// changed). ⛔ But she does NOT run the Cleric's heal loop, and she must be able to reach
		// the ZONE-ORDER dispatch below — that dispatch is the ONLY thing that can give her a real
		// FSiegeUnitGroup::PositionCenter/PositionRadius, i.e. the position circle WITCH-§4 rules
		// her targeting on. ⇒ she falls THROUGH this branch; the Cleric's path is byte-unchanged.
		if (!IsVeilCaster())
		{
			UpdateStateSupport();
			return;
		}
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

	// ── THE WITCH'S OWN BODY (TASK-830, WITCH-§4) ──────────────────────────────
	// Reached only by a veil caster with NO live zone order — a grouped one returned above,
	// through the SHIPPED sorcerer path (CanEverAttack() is false for her, so
	// UpdateStateGrouped's guard 2 forces her target null and she station-keeps inside her own
	// position circle). ⛔ PLACED ABOVE THE STANCE GATE AND THE LEGACY BODY DELIBERATELY: both
	// of those ACQUIRE ENEMIES and march at them, and a 0-damage support unit dropped into that
	// machine would walk into the enemy fleet and stand there. Her body reads the T/E stance
	// itself, for the one rung where it can mean something (see UpdateStateWitch step 3).
	if (IsVeilCaster())
	{
		UpdateStateWitch();
		return;
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

	// Reacquire/leash (GDD §3.8): drop a dead/destroyed target, or one beyond the leash.
	// ⛔⛔ SITE 1 of 2 — GetEffectiveLeashRangeUU(), ⛔ NOT the raw LeashRange member (TASK-979
	// item 6a). Read the two `if`s below as one statement: the drop has NO return and NO branch
	// after it, so the acquire that follows runs in the SAME 0.25 s poll. Against the retired
	// flat leash of 900, a notice radius of 5000 makes everything released here immediately
	// re-takeable eight lines down and the leash silently stops existing. The effective leash
	// keeps the ORDERING (leash > notice) at any notice radius, which is what makes those two
	// `if`s safe to leave adjacent — ⛔ and it keeps it by DERIVATION, so a future card with a
	// NoticeRange above 5333.33 uu is covered here with no edit to this site.
	// ⭐⭐⭐ RETENTION SITE 1 of 2 IS NOW FOG-CLAMPED (TASK-1008). 🧑 "yes clamp retention under
	// fog" (FOG-§9.11): the bound handed to the compare is the effective leash passed through the
	// ONE unit-side ceiling — ⛔ BIT-IDENTICAL in clear weather (8000 today) and ⛔ 609.6 with the
	// fog up. ⛔ It is ⛔ NOT `min(LeashRange, effective notice)`: that form was MEASURED to cut
	// the CLEAR-WEATHER leash to the notice radius and to put leash EQUAL to notice, re-creating
	// the drop-then-re-acquire thrash the effective leash exists to close.
	// ⭐ AND THIS IS THE HALF THAT CARRIES MELEE. A Footman's AttackRange is 120, so the firing
	// gate below is a NO-OP for him in both fog states — only this line can stop a melee unit
	// charging 1500 uu at something the fog says it cannot see (FOG-§9.7a).
	if (CurrentTarget && (!IsTargetAlive(CurrentTarget) || GetDistanceToTarget(MyLocation, CurrentTarget) > ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())))
	{
		CurrentTarget = nullptr;
	}

	// Acquire: nearest alive enemy within AggroRadius, re-evaluated every check so the
	// unit always fights the nearest threat (§3.8 "acquire nearest enemy within" the notice
	// radius — 5000 by default since 🧑 his J-F28 ruling, not the original 600).
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

	// ⭐⭐ FIRING GATE 1 of 6, FOG-CLAMPED (TASK-1008). 🧑 "no ranged units will be able to fire
	// onto anything above 20 feet away" — applied as a SHARED CEILING via `min` against this
	// unit's OWN AttackRange, ⛔ never by collapsing notice and firing into one number
	// (FOG-§9.10a). ⛔ `min(120, 609.6) = 120` at every melee site, in both fog states: that
	// no-op is the CORRECT answer here and the reason retention above is a separate gate.
	if (CurrentTarget && GetDistanceToTarget(MyLocation, CurrentTarget) <= ApplyFogVisionCeilingUU(AttackRange))
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

	// ⭐ SITE 1 of 9 (TASK-828, WITCH-§1): the enumerate-and-team-filter step now lives in
	// FSiegeCombatStatics::GatherHostileAgents. What used to be here — the world enumeration,
	// the IsValid guard folded into IsTargetAlive, the Cast<ITeamAgent> and the
	// `GetTeamId() == Team` compare — is that function, verbatim and in the same order.
	// ⛔ THIS UNIT'S OWN filtering did NOT move: AggroRadius, the pawn/non-pawn bucketing and
	// the TieBreakDistance rule are all still right here, which is the whole point.
	const FVector MyLocation = GetActorLocation();

	// ⭐⭐ TASK-838 (FOG-§7 ROW 1 — VISION / ACQUISITION): this gather is an ACT OF SEEING, so it
	// hands the funnel the two facts only this unit knows — where it is looking FROM, and the
	// reach it is looking WITH. ⛔ It performs no clamp, consults no ceiling and names no fog
	// symbol: for an ACQUISITION the ceiling is applied inside the funnel (FOG-§6). A per-site
	// clamp here would be the forgotten-guard-point failure the funnel exists to prevent, and
	// SiegeAcquisitionFunnelTest test 9 fails the build over it.
	// ⚠️⚠️ AND THIS SITE IS ⛔ DELIBERATELY NOT ROUTED THROUGH THE UNIT-SIDE CEILING TASK-1008
	// ADDED — its ABSENCE here is the design, not an omission. This gather already receives the
	// ceiling from the funnel, so calling the unit-side door as well would clamp the same reach
	// TWICE: harmless arithmetically (`min` is idempotent) and ⛔ fatal structurally, because it
	// makes the funnel look optional and puts a second guard point on the acquisition path.
	// ⇒ ⭐ THE RULE THE WHOLE FILE OBEYS: ⛔ a reach the FUNNEL can see is clamped by the FUNNEL;
	// ⛔ only reaches the funnel structurally cannot see — FIRING, RETENTION, and the commanded
	// lane's self-notice bound, which gathers UNBOUNDED around a point — go through the unit-side
	// chokepoint. ⛔ The exemption is expressed by WHICH FUNCTION a site calls, never by an `if`.
	// ⚠️⚠️ AMENDED 2026-09-04 (TASK-979, re-valued by TASK-1003) — AND THE AMENDMENT IS THE
	// POINT. This paragraph read *"AggroRadius is the GDD §3.8 PROFILE CONSTANT 600 … so this
	// site already sits INSIDE the 609.6 ceiling and fog does not narrow it."* ⛔ THAT IS NOW
	// FALSE IN BOTH CLAUSES: the notice radius is 5000 (🧑 his J-F28 ruling — *"5000 with no fog
	// and still 609 under fog"* — which superseded the 2000 he ruled earlier the same day), so
	// `min(5000, 609.6) = 609.6` and ⛔ FOG NOW BITES THIS SITE HARDER THAN ANY OTHER — which is
	// exactly what he asked for ("the noticing range is reduced to 609 in fog for NON-RANGED
	// UNITS as well", J-F21).
	// ⭐⭐ THE OLD PARAGRAPH'S CLOSING SENTENCE IS WHY NOTHING HAD TO BE REWIRED: it kept handing
	// the query over anyway, *"because it must stay correct if AggroRadius is ever retuned above
	// the ceiling"*. It was — ⛔ TWICE on 2026-09-04, 600 → 2000 → 5000 — and the site needed
	// ⛔ zero fog code on either occasion, because the ceiling arrives through the ACQUISITION
	// chokepoint (FOG-§7). ⭐ That is the argument being VERIFIED rather than merely re-asserted: a site
	// that had opted out "because the numbers happen to line up today" would have had to be
	// found and repaired twice in one day, and nothing here moved at all.
	// 📌 MEASURED CONSEQUENCE, recorded beside the cause: at 600 the funnel's per-candidate cut
	// was provably SKIPPED (SiegeCombatStatics.cpp:207's strict `<`, `600 < 600` false); at 5000
	// it RUNS, i.e. an ActorGetDistanceToCollision per candidate per unit at 4 Hz under fog —
	// work this path has never performed. The ENUMERATION is unchanged (`1.00×`): the funnel
	// takes no radius at all. ⛔ The cut itself is now 87.8% of the radius (609.6 / 5000), ⛔ not
	// the 69.5% recorded at the retired 2000.
	const FSiegeVisionQuery Vision = FSiegeVisionQuery::SeeingFrom(MyLocation, AggroRadius);

	TArray<AActor*> HostileAgents;
	FSiegeCombatStatics::GatherHostileAgents(World, Team, HostileAgents, ESiegeVeilPolicy::SuppressVeiled, &Vision);

	// bucket winners: preferred = units/hero (pawns); other = buildings/castle (non-pawns)
	AActor* BestPawn = nullptr;
	float BestPawnDist = TNumericLimits<float>::Max();
	AActor* BestOther = nullptr;
	float BestOtherDist = TNumericLimits<float>::Max();

	for (AActor* Candidate : HostileAgents)
	{
		// `Candidate == this` is now provably redundant — self is always on `Team`, so the
		// gatherer already dropped it — and it is KEPT anyway: it costs one pointer compare on
		// a 0.25 s poll and it is the kind of guard that becomes load-bearing again the day
		// someone changes what "hostile" means. IsTargetAlive still carries the per-type
		// liveness rules (destroyed castle / hidden dead hero / bDead same-frame window).
		if (Candidate == this || !IsTargetAlive(Candidate))
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

		// TASK-574 (CONVENTIONS WR-§2b row B): the disc is still CENTRED on the castle,
		// but its RADIUS is now DERIVED — the castle's live colliding half-width plus the
		// authored DefendRadius band. ⛔ DefendRadius is NO LONGER a centre radius and is
		// never passed here directly: at the 9× castle a 2,500 centre disc lay entirely
		// inside the keep, so DEFEND could never acquire the besiegers standing at the
		// gate. ResolveDefendEngagementRadius owns the 0-band seal, the
		// bOnlyCollidingComponents query and the null/degenerate fallback.
		CurrentTarget = AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), ResolveDefendEngagementRadius(OwnCastle));

		if (CurrentTarget)
		{
			// ⭐⭐ FIRING GATE 2 of 6, FOG-CLAMPED (TASK-1008) — the DEFEND stance. ⛔ NOT exempted:
			// exempting one stance would make clear weather and fog differ on that stance alone,
			// which is the split item 6b exists to prevent. The ORDER is untouched — a defender
			// under fog stays assigned, holds its ground and goes blind; it never walks home.
			if (GetDistanceToTarget(MyLocation, CurrentTarget) <= ApplyFogVisionCeilingUU(AttackRange))
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
		// ⛔⛔ SITE 2 of 2 for the leash (TASK-979 item 6a). This body "mirrors the legacy Standard
		// body EXACTLY", and that includes the drop-then-re-acquire adjacency — so fixing only
		// UpdateState would have fixed half the game: every Blue Standard unit under a player
		// command runs THIS copy instead. Same reasoning, same accessor; see site 1.
		// ⭐⭐⭐ RETENTION SITE 2 of 2 IS NOW FOG-CLAMPED (TASK-1008), for the same reason and
		// through the same one ceiling. ⛔ This is the ONLY commanded lane that reads a leash at
		// all: HOLD / AMBUSH / FOLLOW return from UpdateState before either site and therefore
		// cannot execute one on any path, which is why AMBUSH's unbounded chase survives this row
		// ⛔ WITHOUT an exemption branch (FOG-§9.11 — a structural exception, never an `if`).
		if (CurrentTarget && (!IsTargetAlive(CurrentTarget) || GetDistanceToTarget(MyLocation, CurrentTarget) > ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())))
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

		// ⭐⭐ FIRING GATE 3 of 6, FOG-CLAMPED (TASK-1008) — the commanded ATTACK path, which
		// "mirrors the legacy Standard body EXACTLY" and therefore mirrors its ceiling too.
		if (CurrentTarget && GetDistanceToTarget(MyLocation, CurrentTarget) <= ApplyFogVisionCeilingUU(AttackRange))
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
	// ⭐⭐ AND TASK-1008 DID ⛔ NOT CHANGE THAT, WHICH IS THE POINT WORTH WRITING DOWN. That row
	// wired 🧑 "yes clamp retention under fog" at the TWO leash sites — ⛔ neither of which is
	// here. This body still holds ⛔ ZERO distance-from-self drop terms, so a HOLD unit keeps its
	// zone leash and an AMBUSH unit keeps its unbounded chase ⛔ in fog exactly as in sunshine.
	// ⛔ The exemption needed ⛔ NO branch: UpdateState returns into this function before either
	// leash site, so this lane structurally CANNOT execute one. ⛔ Do not "complete" the fog work
	// by adding a drop term here — 🧑 "commanded units DO NOT LOSE THEIR COMMANDS."
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
	//
	// ══ THE TOWER-§9 CLIMB DISARM RIDES THIS SAME POINT — GUARD 2 of 3 (TASK-738) ════════════
	// Same two-term gate as guard 1: the permanent class seal OR'd with the transient climb.
	// ⚠️ HONEST NOTE FOR QA, BECAUSE IT WOULD OTHERWISE READ AS DEAD CODE: this body is
	// UNREACHABLE during a climb — UpdateState's own early-out (the traversal fence, see its
	// comment) returns before the group dispatch. The term is here because TOWER-§9.2 names
	// these three points and because the shipped seal is defence-in-depth at all three; it is
	// ⛔ not the mechanism that stops a climber attacking. If it ever DOES become reachable, the
	// sealed behaviour (target forced null, fall through to station-keeping) is the right one.
	if (!FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing()))
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
		// ⭐⭐ FIRING GATE 4 of 6, FOG-CLAMPED (TASK-1008) — and this is the ONE fog term the
		// zone-ordered lane receives. ⛔ NOTHING ELSE HERE MOVES: this body still has ⛔ zero
		// distance-drop terms, so a HOLD unit keeps its zone leash and an AMBUSH unit keeps its
		// deliberately unbounded chase. 🧑 "commanded units DO NOT LOSE THEIR COMMANDS" — under
		// fog a grouped unit holds its target and its station and simply cannot SHOOT past
		// 609.6, which is the card without the order being abandoned (FOG-§9.11).
		if (GetDistanceToTarget(MyLocation, CurrentTarget) <= ApplyFogVisionCeilingUU(AttackRange))
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

	// ══ LADDER EXIT 3 of 8 — A NEW ORDER ABORTS THE CLIMB (TASK-738, TOWER-§10 row L-4) ══
	// ⭐ Jonathan's escape hatch, and it is the counterweight that keeps TOWER-§9's disarm from
	// being a death sentence: a player who sees the arrows coming can bail, the unit DROPS from
	// wherever it is, and the drop costs nothing (⛔ no fall damage, TOWER-§4a).
	// ⚠️ TOWER-§10 also names this the traversal's MOST LIKELY exit path, which is why the
	// movement-mode restore had to be bulletproof here first.
	// ⛔ THIS SITE AND NOT ClearCommandGroup(): a RELEASE is not a new order, and worse,
	// ClearCommandGroup is also reached by UpdateState's null-group SELF-HEAL — a climber whose
	// group evaporated would be dropped off the ladder by a bookkeeping tidy-up nobody ordered.
	// Idempotent on a non-climbing unit, so the ordinary 100-unit confirm pays one bool test.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::NewOrder);

	// ⭐ "A CAST ALSO CANCELS IF … THE WITCH IS ORDERED AWAY" (WITCH-§4, TASK-830) — and ⛔ THIS
	// site, not a poll, is where that rule belongs: a new order changes the position circle
	// underneath her, so the subject she is channelling on may not be in the NEW circle at all.
	// Cancelling on the ⛔ PRESS rather than on the next ≤0.25 s validation pass is also what
	// makes it read as the player's own decision instead of a lag.
	// ⛔ THIS IS ⛔ NOT A VEIL BREAK, AND THE TWO SITES ARE ADJACENT SO IT IS SAID HERE: being
	// ORDERED is ⛔ not acting (WITCH-§2 lane 4 — an invisible unit its own player cannot command
	// is a BUG), so a veiled unit that is re-circled ⛔ stays veiled. Only the CAST dies.
	// ⛔ THIS SITE AND NOT ClearCommandGroup(), the same reasoning the ladder exit above states:
	// a RELEASE is not a new order, and ClearCommandGroup is also the null-group SELF-HEAL — a
	// witch whose group evaporated would lose her cast to a bookkeeping tidy-up nobody ordered.
	CancelWitchCast(TEXT("the witch was given a new order"));

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
	//
	// ⭐⭐ WIDENED FOR THE WITCH ONLY (TASK-830, WITCH-§4) — and this term is ⛔ REQUIRED BY THE
	// FEATURE rather than a convenience. Her targeting circle IS
	// FSiegeUnitGroup::PositionCenter/PositionRadius, and the R/F stage-3 confirm is the ⛔ ONLY
	// thing in the game that ever writes those. ⇒ without this the position circle could ⛔ never
	// be non-zero for a witch, WITCH-§4's central ruling would be ⛔ unreachable code, and every
	// cast would silently take the J-W5 ungrouped fallback forever.
	// ⛔ THE CLERIC IS ⛔ NOT AFFECTED: IsVeilCaster() is false for it and for every other shipped
	// unit, so the manager's FOLLOW-ONLY ruling stands exactly as written — the heal body it was
	// protecting is not reshaped, because the witch does not run it (see UpdateSupportHealTargeting).
	// ⛔ Safe because she cannot fight: CanEverAttack() is false for her, so a zone-ordered witch
	// takes UpdateStateGrouped's sealed tier-3 station-keeping path, never its acquisition tiers.
	return Profile == ECardProfile::Standard || IsVeilCaster();
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

	// ⭐ SITE 2 of 9 (TASK-828, WITCH-§1) — same lift as AcquireTarget above, same guarantee:
	// only the enumerate-and-team-filter step moved. The DISC gate, the bucketing and the
	// TieBreakDistance rule stay here.
	const FVector MyLocation = GetActorLocation();

	// ⭐⭐ TASK-838 (FOG-§7 ROW 1) — AND THIS IS THE SITE WHERE FOG ACTUALLY BITES A UNIT.
	// ⛔ THE VISION QUERY STAYS UNBOUNDED, and the choice is still forced rather than convenient:
	// this site's ZONE eligibility is a 2D disc around a COMMANDED POINT (`Center`/`Radius`, the
	// zone order), which is not a reach from the unit at all, so there is no self-range to hand to
	// the FUNNEL. Unbounded returns bit-identically with fog off (nothing cut) and becomes the
	// ceiling under fog — so a commanded unit stops acquiring across a zone it can no longer see
	// into, which IS the card.
	// ⛔ The ORIGIN is the UNIT, never `Center`: the question fog answers is "what can THIS UNIT
	// see", not "what is near the flag".
	//
	// ⚠️⚠️ AMENDED 2026-09-04 (TASK-979 item 6b) — ⛔ ONE CLAUSE OF THIS PARAGRAPH IS NOW STALE AND
	// IS STRUCK RATHER THAN LEFT LYING. It read: *"Handing over some other number (AggroRadius,
	// AttackRange) would narrow a commanded unit's pick ⛔ WITH FOG OFF, i.e. a shipped behaviour
	// change wearing a fog card's commit message."*
	// ⭐ THE OBSERVATION WAS CORRECT AND JONATHAN THEN RULED THE NARROWING **IN**, by name: *"if an
	// enemy unit walks into the circle that a commanded unit is supposed to be guarding but that
	// enemy unit is outside the range in which they can notice them due to fog OR ANYTHING, the
	// commanded unit still will not be able to detect them."* ⇒ the clear-weather narrowing is the
	// SPEC now, not the hazard.
	// ⛔⛔ AND THE OLD CLAUSE'S REAL WARNING IS HONOURED EXACTLY, WHICH IS WHY THE BOUND IS WHERE IT
	// IS: it warned against a behaviour change *wearing a fog card's commit message*. So the bound
	// is a SITE-LOCAL distance cut on a COMBAT row (TASK-979), applied below the loop and ⛔ NOT by
	// changing this query — the funnel still receives `SeeingFromUnbounded` and the fog lane is
	// untouched. ✅ TASK-1008 has now routed that already-existing bound through the unit-side
	// ceiling; ⛔ THIS QUERY IS STILL UNBOUNDED and must stay that way — the clamp is on the
	// self-distance cut below, never on the gather.
	const FSiegeVisionQuery Vision = FSiegeVisionQuery::SeeingFromUnbounded(MyLocation);

	TArray<AActor*> HostileAgents;
	FSiegeCombatStatics::GatherHostileAgents(World, Team, HostileAgents, ESiegeVeilPolicy::SuppressVeiled, &Vision);

	const float RadiusSq = Radius * Radius;

	// ⭐⭐ TASK-979 item (6b) — THE COMMANDED-LANE NOTICE BOUND, AND IT IS THE GENERAL FORM.
	// ⚖️ Jonathan, verbatim: "commanded units DO NOT LOSE THEIR COMMANDS in fog, however if an
	// enemy unit walks into the circle that a commanded unit is supposed to be guarding but that
	// enemy unit is outside the range in which they can notice them ⛔ DUE TO FOG OR ANYTHING,
	// the commanded unit still will not be able to detect them."
	// ⛔⛔ "OR ANYTHING" IS WHY THIS LIVES HERE AND NOT ON THE FOG ROW: the bound is a property of
	// the ENGAGEMENT RADIUS, so it must hold in CLEAR WEATHER too. Boarded on the fog card alone,
	// a commanded unit would see its whole circle in sunshine and part of it in fog for no reason
	// a future reader could reconstruct. ✅ TASK-1008 routed THIS existing read through the
	// unit-side ceiling; it did ⛔ not add the bound, and the bound must ⛔ never be re-added
	// beside it — one term, ceilinged, not two terms racing.
	// ⛔ THE ORDER ASSIGNMENT IS UNTOUCHED — this cuts TARGET ACQUISITION only. A unit told to
	// guard a circle stays assigned, holds its station and goes blind; it does not abandon the
	// zone. (UpdateStateGrouped's deliberate absence of a distance-drop path is likewise
	// untouched: nothing here drops a held target.)
	// ⚠️⚠️ THE MEASURED CONSEQUENCE — ⛔ RE-MEASURED AT HIS 5000 (TASK-1003), AND THE FINDING IT
	// USED TO CARRY IS ⛔ RESOLVED RATHER THAN RE-NUMBERED. This lane had NO notice bound at all —
	// it gathers via SeeingFromUnbounded and filtered only by zone membership — so this is a
	// bound being INTRODUCED, not tightened. ⛔ The paragraph that stood here said a legal maximum
	// guard circle was *"2.5× wider than the 2000 default ⇒ a unit standing at its centre is
	// blind to ~84% of its own circle's AREA"*. ✅ USiegePlayerController::GroupRadiusMax is
	// 5000 and the notice radius is now ⛔ ALSO 5000, so that unit covers ⛔ 100% of its circle
	// and is blind to ⛔ 0% of it. ⭐ That is 🧑 J-F28 answered EXACTLY, and it is why his number
	// is 5000 and not a rounder one (FOG-§9.11).
	// ⛔⛔ THE TWO 5000s ARE ⛔ NOT THE SAME NUMBER (FOG-§9.5): when the guard-circle cap moves,
	// unit eyesight need ⛔ not, and vice versa. ⛔ GroupRadiusMax is QUOTED here and deliberately
	// ⛔ NOT retuned, ⛔ not substituted, and ⛔ not merged into one symbol.
	// ⛔⛔ AND IT IS FOUR CALLERS IN **TWO** FAMILIES, NOT ONE (qa/TASK-979 WARN-1 — the figure
	// above sizes only the GROUPED circle). The second family is the DEFEND stance
	// (UpdateStateStandardCommanded), whose disc is ResolveDefendEngagementRadius() = the
	// castle's live colliding half-width + DefendRadius ⇒ ≈3656.85 + 1281 ≈ 4937.9 uu at the
	// shipped 9× castle — read back from that function's own derivation log, not assumed.
	// That is 0.99× this default ⇒ ⛔ that disc is now covered in full too.
	// ⚠️⚠️ ⛔ BUT THE DISCS ARE NOT THE ONLY MEASUREMENT, AND THE OTHER ONE ⛔ DID NOT CLEAR —
	// this lane still has a SCAR: TASK-574 exists because DEFEND acquired nothing and every
	// defender walked home while the castle was battered.
	// ⭐ WHAT TASK-574 REPAIRED IS INTACT — its defect was a disc lying ENTIRELY INSIDE THE KEEP
	// (besiegers at the gate unreachable AT ANY RANGE); this bound is a REACH limit, so a
	// defender standing at the battered face still acquires there and the fallback is unchanged.
	// ⚠️ WHAT IS STILL OPEN: the castle footprint is ≈7313.7 x 7384.5 uu, so two units on
	// opposite faces are ≈7313.7 uu apart — ⛔ 1.46× this radius (it was 3.66× at the retired
	// 2000) — and the far-face besieger is ⛔ still not noticed. ⛔ Do not read "J-F28 is
	// resolved" as "the DEFEND gap is closed": only the guard circle moved into the clear.
	// ⛔ DECLINED HERE ON PURPOSE, NOT MISSED: an exemption would make clear weather differ from
	// fog on one stance (the exact split item 6b exists to prevent), and re-pointing the defender
	// at the battered face is a MOVEMENT change — this cuts ACQUISITION ONLY. ⇒ J-F28 carries the
	// number to Jonathan; TASK-987 carries the regression watch. A finding, never "flaky".
	// ⭐ Sealed classes get defence-in-depth for free rather than a behaviour change: AMinerUnit/
	// ASorcererUnit carry AggroRadius 0, so this cut rejects everything — and their real seal
	// (CanEverAttack(), guard 2 in UpdateStateGrouped) already forces the target null before this
	// function is ever reached for them.
	// ⭐⭐⭐ NOTICE BOUND — THE NINTH AND LAST FOG-CLAMPED REACH (TASK-1008). The bound itself was
	// INTRODUCED by TASK-979 and is unconditional ("due to fog OR ANYTHING"); this row does the
	// one thing both paragraphs above promised and routes THAT EXISTING READ through the ceiling.
	// ⛔ It does not add the bound, and it must never be re-added beside this one.
	// ⛔ HOISTED ABOVE THE LOOP DELIBERATELY: one ceiling evaluation per call, ⛔ not per
	// candidate — the reach is a property of the UNIT, so recomputing it per candidate would buy
	// nothing and pay a fog-state read for every hostile on the field.
	// ⭐⭐ THIS IS THE HALF THAT REFUSES THE INTRUDER: an enemy standing INSIDE a commanded unit's
	// guard circle but beyond 609.6 uu from that unit is not acquired under fog, while the ORDER
	// itself survives untouched (🧑 "commanded units DO NOT LOSE THEIR COMMANDS in fog, however
	// if an enemy unit walks into the circle … outside the range in which they can notice them
	// due to fog OR ANYTHING, the commanded unit still will not be able to detect them").
	const float NoticeRadiusUU = ApplyFogVisionCeilingUU(GetEngagementRadiusUU());

	// Identical bucketing/tie-break to AcquireTarget — the eligibility gate is now TWO terms:
	// the 2D disc anchored on Center (the zone order) AND the notice bound from self above.
	// Nearest-to-SELF is still the selection metric so the Standard tie-break behavior stays
	// consistent.
	AActor* BestPawn = nullptr;
	float BestPawnDist = TNumericLimits<float>::Max();
	AActor* BestOther = nullptr;
	float BestOtherDist = TNumericLimits<float>::Max();

	for (AActor* Candidate : HostileAgents)
	{
		// redundant-but-kept self guard + the per-type liveness rules (see AcquireTarget)
		if (Candidate == this || !IsTargetAlive(Candidate))
		{
			continue;
		}

		// disc filter: the candidate's LOCATION must lie within Radius (2D) of Center
		if (FVector::DistSquared2D(Candidate->GetActorLocation(), Center) > RadiusSq)
		{
			continue;
		}

		const float Distance = GetDistanceToTarget(MyLocation, Candidate);

		// ⭐⭐ THE NOTICE BOUND (TASK-979 item 6b — see the paragraph above the loop). Measured
		// from SELF with the same bounds-aware metric AcquireTarget uses, ⛔ never from Center:
		// the question is "can THIS UNIT notice it", not "is it near the flag".
		// ⛔ It is a SECOND term beside the zone disc and replaces neither — a candidate must be
		// in the commanded circle AND within this unit's reach.
		// ⛔ NOT gated on bRangedAttack, and that is a fence rather than an omission: bRanged is
		// the PROJECTILE-DELIVERY flag, so CrystalTower ships bRanged=FALSE at Range 800 and any
		// `if (bRangedAttack)` here would let it straight through (FOG-§9.8c).
		if (Distance > NoticeRadiusUU)
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

float ASummonedUnit::ResolveDefendEngagementRadius(const ACastle* OwnCastle)
{
	// ── CONTRACT 1: A BAND OF 0 MEANS "NO ACQUISITION", AND IT IS TESTED FIRST, BEFORE
	//    ANY GEOMETRY IS TOUCHED. ASorcererUnit's constructor sets DefendRadius = 0 as
	//    half 1 of its never-attacks seal ("at 0 the disc is empty, so a sorcerer under
	//    DEFEND falls back to marching home"), and that sentence must stay true after the
	//    semantic change. A naive max(BoxExtent) + band would turn that 0 into ≈3,657 uu
	//    at the 9× castle and hand a unit that cannot attack a real target disc — the
	//    highest-value trap in this change. The early-out keeps 0 meaning EXACTLY what it
	//    meant before this function existed: AcquireEnemyNearPoint gets 0, its disc filter
	//    admits nobody, and the unit marches home. Tested as <= 0 rather than == 0 so a
	//    hand-authored negative can never resolve into a live radius either.
	if (DefendRadius <= 0.f)
	{
		return 0.f;
	}

	// ── CONTRACT 3a: no castle to measure ⇒ the authored value is used as a plain CENTRE
	//    radius (exactly the pre-TASK-574 behaviour), never 0, never a crash.
	//    ⚠️ DEFENSIVE AND DECLARED AS SUCH: the only shipped caller
	//    (UpdateStateStandardCommanded's DEFEND branch) EnterIdle()s on a null own castle
	//    BEFORE it reaches this call, so this branch is unreachable from today's single
	//    call site. It is kept because the null-safety contract belongs to the function
	//    that does the dereference, not to its callers.
	if (!IsValid(OwnCastle))
	{
		if (!bLoggedDefendBandFallback)
		{
			bLoggedDefendBandFallback = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] DEFEND band: no own castle to measure — falling back to the authored %.0f uu used as a plain centre radius."),
				*GetNameSafe(this), DefendRadius);
		}
		return DefendRadius;
	}

	// ── CONTRACT 2: bOnlyCollidingComponents = true, and it is load-bearing. The castle's
	//    HP-bar widget sits ≈9,450 uu above the keep at the 9× scale, and a render-bounds
	//    query would fold that height into the extent. What DEFEND cares about is the
	//    footprint a besieger is physically STOPPED by — the same query, for the same
	//    reason, as ASiegeGameMode::GetHeroStartTransform branch 3 (the shipped model for
	//    deriving a castle-relative distance instead of transcribing one).
	FVector CastleBoundsOrigin = FVector::ZeroVector;
	FVector CastleBoxExtent = FVector::ZeroVector;
	OwnCastle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, CastleBoundsOrigin, CastleBoxExtent);

	// max of the two horizontal half-extents: the disc is centred on the castle, so it has
	// to clear the WIDEST face or a besieger on that side is still inside the dead zone.
	// FVector components are DOUBLE in UE5 and FMath::Max is a single-type template, so
	// the max is taken in double and converted ONCE (CONVENTIONS compile traps — mixing
	// double and float in FMath::Max fails template deduction).
	const float CastleHalfWidth = static_cast<float>(FMath::Max(CastleBoxExtent.X, CastleBoxExtent.Y));

	// ── CONTRACT 3b: degenerate/unresolvable bounds (mesh not streamed in yet, collision
	//    stripped) ⇒ same fallback as 3a. ⛔ Never a zero radius by accident.
	if (CastleHalfWidth <= UE_KINDA_SMALL_NUMBER)
	{
		if (!bLoggedDefendBandFallback)
		{
			bLoggedDefendBandFallback = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("[%s] DEFEND band: own castle %s reported degenerate colliding bounds (half-extent %.1f x %.1f) — falling back to the authored %.0f uu used as a plain centre radius."),
				*GetNameSafe(this), *GetNameSafe(OwnCastle), CastleBoxExtent.X, CastleBoxExtent.Y, DefendRadius);
		}
		return DefendRadius;
	}

	const float EngagementRadius = CastleHalfWidth + DefendRadius;

	// One Log line per unit lifetime, never re-armed (the AMinerUnit::bLoggedInteriorAnchor
	// idiom). This is the ONLY place the live castle half-width and the resolved radius are
	// both readable back from a PIE log with no editor probe, and the PIE acceptance rows
	// for this change are graded on it: at the 3× castle it prints 1218.95 + 1281 = 2500.0
	// (byte-identical to the shipped behaviour), at the 9× castle ≈3656.85 + 1281 ≈ 4937.9.
	if (!bLoggedDefendBandDerived)
	{
		bLoggedDefendBandDerived = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("[%s] DEFEND band resolved: castle %s measured colliding half-width %.2f + authored band %.0f (past the wall face) => engagement radius %.2f uu."),
			*GetNameSafe(this), *GetNameSafe(OwnCastle), CastleHalfWidth, DefendRadius, EngagementRadius);
	}

	return EngagementRadius;
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

	// ⭐⭐ FIRING GATE 5 of 6, FOG-CLAMPED (TASK-1008) — and it is ROUTED rather than exempted
	// even though it is INERT on shipped data (Sapper/Ogre Range 120 ⇒ `min(120, 609.6) = 120`).
	// ⛔ Exempting an inert site turns a structural rule into a LIST, and the next Siege card
	// with a longer reach would be the hole. ⚠️ It is also the SUICIDE trigger, so a clamp that
	// ever did bite here would delay a detonation rather than merely a swing — one more reason it
	// goes through the same ceiling as everything else instead of being reasoned about locally.
	if (GetDistanceToTarget(MyLocation, CurrentTarget) <= ApplyFogVisionCeilingUU(AttackRange))
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
	// ⛔⛔ THE WITCH IS ECardProfile::Support AND SHE IS ⛔ NOT A HEALER (TASK-830, WITCH-§4).
	// ⚠️⚠️ THIS GUARD IS ⛔ NOT TIDINESS — WITHOUT IT THE CARD IS ⛔ BROKEN, AND BROKEN IN THE ONE
	// WAY THAT LOOKS LIKE A DIFFERENT BUG. Her row Damage is 0 (she does not attack), and the heal
	// RATE is the row Damage — so she would arm the heal timer beside any damaged friendly and
	// PerformHeal would call BreakInvisibility(Heal) ⛔ every 0.1 s to deliver ⛔ zero HP. A veiled
	// witch would therefore un-veil herself instantly, ⛔ for an act with no observable effect, and
	// the report would read "the witch cannot stay invisible" with nothing in the heal code wrong.
	// ⭐ PLACED HERE RATHER THAN AT THE TWO CALL SITES ON PURPOSE: this function has exactly two
	// callers (UpdateStateSupport and the FOLLOW body), and a following witch is the ⛔ common case
	// — Follow is the spawn default. One guard, both roads.
	// ⛔ StopHealing() and not a bare return: if she were ever bound to a Cleric-shaped row first
	// and re-bound later, a timer armed by that earlier tick must not survive.
	// The Cleric's four statements below are byte-unchanged, and this is false for every one of them.
	if (IsVeilCaster())
	{
		StopHealing();
		SupportHealTarget = nullptr;
		return nullptr;
	}

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

	// ══ VEIL BREAK — `Heal` (TASK-829; WITCH-§3 "heal", WITCH-§3a's direction rule) ═══════════
	// ⛔⛔ DIRECTION IS LOAD-BEARING AND IT IS THE EASY BUG: the ⛔ HEALER breaks. The ⛔ PATIENT
	// does ⛔ NOT — ApplyHealing (just below) is the RECEIVER side, and a veiled unit that gets
	// mended by a friendly Cleric ⛔ STAYS VEILED. Being healed is being ACTED UPON, and his rule
	// breaks the veil on ⛔ ACTING. ⛔ Do not "balance" this by adding a break there.
	// ⭐ PLACED AFTER every early-out above and immediately before the act: a Cleric whose target
	// died, topped off or drifted out of range this tick returns without healing and ⛔ without
	// un-veiling. The break is the ACT, ⛔ not the intention.
	// ⚠️ WHY THE STATE MACHINE IS NOT THE PREDICATE (WITCH-§3a): this runs on its own
	// HealTimerHandle while `State == Advance`, so a guard written as `State == Attack` would
	// ⛔ LEAK a veiled healer — a following Cleric heals all the way across the field.
	BreakInvisibility(ESiegeVeilBreakReason::Heal);

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

// ═════════════════════════════════════════════════════════════════════════════════════════
//  THE VEIL — the two doors (TASK-829; law WITCH-§3 / WITCH-§6), and since TASK-923 the two
//  EDGES THEY PAINT ON (law WITCH-§5's application clause)
//
//  ⛔⛔ ZERO RULES LIVE HERE. Every RULE about the veil lives in FSiegeInvisibilityStatics,
//  where a headless test can hold it; these are the seams that bind that rule to an actor's
//  `bIsInvisible`. ⛔ Do not add a condition, a timer, a cooldown or a "was visible" cache to
//  either — WITCH-§6 forbids the cache by name, and ApplyBreak's true-exactly-once edge is
//  what makes one unnecessary.
//
//  ⭐⭐ WHAT TASK-923 ADDED, AND WHY IT IS ⛔ NOT "logic" CREEPING BACK IN: each door now hands
//  its EDGE to one material helper (ApplyVeilMaterial / ClearVeilMaterial, below). The edge was
//  ALREADY computed — ApplyVeil returns the false→true transition and ApplyBreak the true→false
//  one — so the swap needed ⛔ no new state, ⛔ no cache and ⛔ no tick. It reads `bIsInvisible`
//  exactly ⛔ zero times: the doors' own return values ARE the signal.
//  ⛔⛔ THE MATERIAL IS A ⛔ CONSEQUENCE OF THE FLAG AND ⛔ NEVER AN INPUT TO IT. ⛔ Nothing
//  anywhere may ask "which material is on slot 0" to decide whether a unit is veiled, and a
//  material that fails to resolve ⛔ does not change what GrantInvisibility returns.
//
//  ⭐ THE PLACEMENT OF THE *CALLS* IS THE WHOLE FEATURE, AND IT IS ⛔ NOT HERE. WITCH-§3a
//  measured that for THREE of the six verbs the obvious wiring point is the WRONG one; each
//  call site carries its own trap comment. This is only the door they all go through.
// ═════════════════════════════════════════════════════════════════════════════════════════

bool ASummonedUnit::GrantInvisibility()
{
	// The ONLY write-true door on this class. TASK-830's witch calls it on cast COMPLETION
	// (see the header for why it landed with 829 rather than with 830).
	const bool bNewlyVeiled = FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible);

	// ⭐⭐ TASK-923 (WITCH-§5): the FALSE→TRUE edge — and ⛔ only the edge — paints the veil.
	// ⛔ Guarded rather than unconditional on purpose: a second cast on an ALREADY-veiled unit
	// must be a total no-op (WITCH-§4's "never target an already-invisible unit" belt), and an
	// unguarded repaint would quietly make the wasted cast cost a full material re-stamp.
	if (bNewlyVeiled)
	{
		ApplyVeilMaterial();
	}

	// ⛔ The RETURN IS THE FLAG'S EDGE, ⛔ never the material's. A missing MI_Unit_Invisible
	// leaves a unit that is genuinely veiled to enemy acquisition and merely LOOKS normal —
	// which is the ruled failure direction (see ApplyVeilMaterial). Reporting false here would
	// make the material a second source of truth about the veil, which WITCH-§6 forbids.
	return bNewlyVeiled;
}

void ASummonedUnit::BreakInvisibility(ESiegeVeilBreakReason Reason)
{
	// ⛔⛔ THE ⛔ ONE WRITE-FALSE DOOR. WITCH-§6: an inlined `bIsInvisible = false` ANYWHERE is an
	// automatic QA FAIL, so this delegation — and never an assignment — is the whole body.
	// Cheap and side-effect-free on the overwhelmingly common case (an unveiled unit), which
	// matters because these calls sit on per-cadence attack and per-tick mining paths.
	if (!FSiegeInvisibilityStatics::ApplyBreak(bIsInvisible, Reason))
	{
		return; // the unit was already visible — no edge, no log, no work
	}

	// ⭐ THE EDGE FIRES ⛔ EXACTLY ONCE PER VEIL (ApplyBreak's true→false transition), which is
	// precisely why WITCH-§6 can ban a "was visible" cache: the one-shot side effects hang off
	// this branch instead of off a remembered previous value.
	// ⚠️ VERBOSE, ⛔ not Log: a fleet-wide veil break would otherwise spam the match log, and this
	// line's job is forensic — it turns the bug report "invisibility is broken" into "the Sapper's
	// blast un-veiled it", which is the difference between a hunt and a fix.
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASummonedUnit '%s' (CardID '%s', team %d): the veil BROKE — reason '%s' (WITCH-§3; permanent, only a NEW witch cast can re-veil it)."),
		*GetNameSafe(this), *CardID.ToString(), static_cast<int32>(Team),
		FSiegeInvisibilityStatics::ToString(Reason));

	// ⭐⭐ TASK-923 (WITCH-§5 / WITCH-§6's restore row) — THE MATERIAL SWAP-BACK, ⛔ WIRED.
	// ⛔ This line is HALF the feature, ⛔ not a postscript. ⚖️ A veil that never REVERTS is worse
	// than one that never applies: Jonathan's rule is that it reverts PERMANENTLY on any act other
	// than walking, so a stuck veil makes all SIX break reasons above silently dead — every one of
	// them would clear the flag and change ⛔ nothing the player can see.
	// ⛔ It sits on THIS branch and nowhere else, so it inherits the edge's exactly-once guarantee
	// for free. ⛔ Do ⛔ not hoist it above the early-out: an unveiled unit taking its ordinary
	// per-cadence attack would then re-stamp its own materials on every swing.
	ClearVeilMaterial();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  THE VEIL'S LOOK — one helper per EDGE (TASK-923; law WITCH-§5's application clause,
//  WITCH-§6's swap-seam + restore rows)
//
//  ⛔⛔ ONE THING TO KEEP STRAIGHT BEFORE EDITING EITHER: NEITHER OF THESE DECIDES ANYTHING.
//  `bIsInvisible` is the one source of truth; these two only paint the decision the two doors
//  above already made, and they are called from ⛔ those two places and ⛔ nowhere else.
//  ⇒ ⛔ NO tick, ⛔ NO timer, ⛔ NO BeginPlay branch, ⛔ NO second bool, ⛔ NO third site.
// ═════════════════════════════════════════════════════════════════════════════════════════

void ASummonedUnit::ApplyVeilMaterial()
{
	// The ACTIVE visual: the skeletal runtime once the M7 swap took, else the static VisualMesh
	// — both UMeshComponent, so ONE code path covers both. Null-safe exactly like
	// ApplyTeamMaterial above; the shape is deliberately copied rather than reinvented.
	UMeshComponent* ActiveMesh = GetActiveVisualMesh();
	if (!ActiveMesh)
	{
		return;
	}

	// Cached static resolve — the SHIPPED idiom from ApplyTeamMaterial, 30-odd lines up: ONE
	// TSoftObjectPtr resolved once per process and shared by every veiled unit in the match, never
	// a per-cast load. LoadSynchronous re-resolves through the soft path if GC ever unloaded it.
	// ⛔ The path is WITCH-§6's pinned name, character-for-character.
	static const TSoftObjectPtr<UMaterialInterface> VeilMaterial(FSoftObjectPath(TEXT("/Game/Materials/MI_Unit_Invisible.MI_Unit_Invisible")));

	UMaterialInterface* const ResolvedVeil = VeilMaterial.LoadSynchronous();
	if (!ResolvedVeil)
	{
		// ⛔⛔ THE FAILURE DIRECTION IS ⛔ RULED, AND IT IS ⛔ RESOLVE-BEFORE-WRITE THAT ENFORCES IT.
		// Returning HERE — before a single slot has been touched — is what guarantees the unit
		// stays fully VISIBLE rather than half-painted. ⛔ Never hide the mesh, ⛔ never blank a
		// slot, ⛔ never SetVisibility(false) as a fallback. ⚖️ A veil that fails to a VISIBLE unit
		// is a missing effect; one that fails to an INVISIBLE-BUT-SOLID unit is an unkillable ghost
		// that still blocks placement and still deals damage — WITCH-§0 refuses exactly that.
		// ⚠️ Warning, ⛔ not Verbose, and it names the path so the fix is the message. It cannot
		// spam: a veil costs a 3-second cast and one witch veils one unit at a time (his sentence),
		// so this fires at most once per completed cast, never on a per-tick or per-attack path.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASummonedUnit '%s' (CardID '%s'): veil material '%s' did not resolve — the unit is VEILED to enemy acquisition but LOOKS NORMAL (WITCH-§5; the ruled failure direction is a visible unit, never a hidden solid one)."),
			*GetNameSafe(this), *CardID.ToString(), *VeilMaterial.ToString());
		return;
	}

	// ⛔⛔ EVERY SLOT. ⛔ NEVER SLOT 1. ⛔ THIS LOOP IS THE ONE THING THAT, GOT WRONG, SHIPS A
	// VISIBLY BROKEN FEATURE RATHER THAN A MISSING ONE. ApplyTeamMaterial writes
	// MI_TeamColor_<Team> to ⛔ SLOT 0 of the two-slot [TeamRegion, <CardID>PBR] contract, so a
	// slot-1-only swap leaves slot 0 ⛔ fully OPAQUE. TASK-833 measured team_region at 1.7–4.1% of
	// a normal unit — ⛔ but ⛔ 18% ON THE WITCH, ⛔ and it is her ⛔ HAT BRIM ⇒ an ⛔ opaque chrome
	// hat floating over a ghostly body. ⚖️ The unit whose entire card is invisibility is the unit
	// the naive implementation breaks WORST.
	// ⛔ DRIVEN BY GetNumMaterials(), ⛔ never by a hard-coded index and ⛔ never assuming the count
	// is 2 — the same whole-body conclusion TASK-756 reached for the ghost, for the same reason
	// (ASiegeGhostPawn's team step, and ASiegePlayerController's placement ghost, both loop here).
	const int32 SlotCount = ActiveMesh->GetNumMaterials();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		ActiveMesh->SetMaterial(SlotIndex, ResolvedVeil);
	}
}

void ASummonedUnit::ClearVeilMaterial()
{
	// ⛔⛔ RE-DERIVED, ⛔ NEVER CACHED — and the SECOND reason is the one that is easy to miss:
	// GetActiveVisualMesh() can return a ⛔ DIFFERENT COMPONENT than it did at veil time, because
	// ResolveSkeletalVisual latches bUsingSkeletalVisual. A cached component pointer, or a cached
	// TArray<UMaterialInterface*> of "the originals", would be restored onto the ⛔ WRONG mesh.
	// (The first reason is the ordinary one: a cache is a second source of truth about something
	// already derivable — the same species WITCH-§6 bans as the "was visible" cache.)
	if (UMeshComponent* ActiveMesh = GetActiveVisualMesh())
	{
		// ⛔ STEP 1 — drop the veil override on EVERY slot the apply could have written, so each
		// falls back to the ⛔ ASSET's authored material.
		// ⭐ MEASURED, ⛔ not assumed, because a setter's silence is not evidence (SC-§39.1):
		// UMeshComponent::SetMaterial(i, nullptr) writes OverrideMaterials[i] = nullptr rather than
		// removing the entry, and BOTH readers treat a NULL entry as "no override" —
		// FStaticMeshComponentHelper::GetMaterial and FSkinnedMeshComponentHelper::GetMaterial each
		// test `OverrideMaterials.IsValidIndex(i) && OverrideMaterials[i]` and fall through to the
		// static mesh's / skinned asset's own material. ⇒ GetMaterial(i) after this loop returns the
		// ASSET's authored material, on either component type.
		// ⚠️ WHY NOT EmptyOverrideMaterials(): it ALSO resets MaterialSlotsOverlayMaterial, a
		// different array this feature never wrote. ⛔ Clearing state we did not set is how a fix
		// grows a side effect. This loop touches exactly what the apply loop touched, and it is
		// visibly its mirror image, which is worth more here than one fewer line.
		const int32 SlotCount = ActiveMesh->GetNumMaterials();
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			ActiveMesh->SetMaterial(SlotIndex, nullptr);
		}
	}

	// ⛔⛔ STEP 2 — ⛔ NOT OPTIONAL, AND ⛔ NOT A REPAINT FOR TIDINESS. Step 1 alone is a blanket
	// restore-to-default, which ⛔ DROPS THE TEAM COLOUR: slot 0's MI_TeamColor_<Team> is a RUNTIME
	// override (TASK-044 — the bot reuses the player's Blue-authored BP_Unit_* assets), so a RED
	// unit would come back BLUE. ⚖️ A blue enemy standing in your half reads as a far worse bug
	// than the one this function is fixing.
	// ⛔ Through the SHIPPED ApplyTeamMaterial and ⛔ never a second implementation: it recomputes
	// the instance from the CURRENT Team, which is precisely why re-deriving beats any cache — a
	// unit re-teamed while veiled comes back in its NEW colour with nothing here knowing about it.
	// ⛔ Called UNCONDITIONALLY (it takes its own null-safe GetActiveVisualMesh early-out), so slot
	// 0 can never be skipped by a guard that only the clear loop above needed.
	ApplyTeamMaterial();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  THE WITCH'S 3-SECOND INTERRUPTIBLE CAST (TASK-830; law WITCH-§4 / WITCH-§6)
//
//  Jonathan, the half of his sentence this block IS:
//    "...will always cast the invisible spell on the nearest visible unit that is within
//     their position circle, however unlike the cleric and sorcerer spells which are
//     instant, this spell has an animation that lasts 3 seconds, which can be interupted
//     if the witch or unit that is turning invisible are attacked. The witch can only make
//     one unit at a time invisible."
//
//  ⛔⛔ THE ONE THING TO KEEP STRAIGHT WHILE READING ANY OF IT: the CAST has a duration; the
//  ⛔ VEIL DOES NOT. Everything below times, validates, cancels and completes a CAST. The
//  moment CompleteWitchCast calls GrantInvisibility the clock is ⛔ out of the story forever
//  — WITCH-§3's "permanently" is his word, and nothing in this block can undo that flag.
//  ⛔ The only route back to visible is BreakInvisibility, from one of the WITCH-§3 acts.
//
//  ⚖️ AND THE SECOND: DAMAGE INTERRUPTS A ⛔ CAST. It does ⛔ NOT break a ⛔ VEIL. WITCH-§3
//  states both rules on one row and warns, in writing, not to merge them — a merge would make
//  every veiled unit clipped by a stray AoE permanently visible, which is not what he wrote.
//  ⇒ ⛔ there is not one BreakInvisibility call anywhere in this block except the ONE on the
//  witch herself, on a ⛔ SUCCESSFUL cast, because a successful cast is her ACTING (J-W3).
// ═════════════════════════════════════════════════════════════════════════════════════════

bool ASummonedUnit::IsVeilCaster() const
{
	// Class identity resolved off the ROW NAME — see the header for why this one predicate reads
	// the CardID instead of being a subclass override, and for the one-line override a future
	// AWitchUnit would ship instead. NAME_None (unbound units, every CDO) is false.
	return CardID == WitchCardID;
}

bool ASummonedUnit::IsCastingVeil() const
{
	// ⭐ THE LATCH IS THE LIVE TIMER, ⛔ not a bool beside it. His "one at a time" then has ⛔ one
	// representation, and the state that says "a cast is running" is the same state that ⛔ makes
	// it run — a bool could be left true by an early return and would seal the witch forever.
	// Safe pre-BeginPlay and on a torn-down world: an unarmed handle simply reads inactive.
	return GetWorldTimerManager().IsTimerActive(WitchCastTimerHandle);
}

void ASummonedUnit::UpdateWitchCast()
{
	// ⛔ ONE compare is what every non-witch in the game pays for this feature, on a 0.25 s poll.
	if (!IsVeilCaster())
	{
		return;
	}

	// The circle is resolved ⛔ ONCE per poll and shared by both branches, so the "is my subject
	// still in it?" test and the "who is in it?" search can ⛔ never disagree about where it is.
	FVector CircleCenter = FVector::ZeroVector;
	float CircleRadius = 0.f;
	ResolveWitchPositionCircle(CircleCenter, CircleRadius);

	// ── (1) A CAST IN FLIGHT IS ⛔ VALIDATED, ⛔ NEVER RESTARTED ──────────────────────────────
	// ⭐⭐ "ONE AT A TIME" (his words, J-W7) IS THIS `return`, and it is the whole of that rule:
	// while the timer is live this function ⛔ cannot reach the acquire below, so a second cast is
	// ⛔ unrepresentable rather than merely guarded against.
	// ⚠️ J-W7 also fixes what "one at a time" does ⛔ NOT mean: a witch who veils A, finishes, then
	// veils B leaves ⛔ BOTH invisible. Nothing here ever touches a ⛔ previously veiled unit — the
	// link to A is dropped the instant A's cast completes.
	if (IsCastingVeil())
	{
		// WITCH-§4's three non-damage cancels, all three of which are ⛔ properties of the SUBJECT
		// and are therefore ⛔ all one predicate: it died, it was veiled by somebody else in the
		// meantime, or it walked out of the circle. (The witch being ⛔ ordered away is the fourth
		// and it is ⛔ not polled — AssignCommandGroup cancels at the order itself, so the cast
		// dies on the ⛔ press rather than up to 0.25 s later.)
		if (!IsWitchVeilCandidate(WitchCastTarget.Get(), CircleCenter, CircleRadius))
		{
			CancelWitchCast(TEXT("the subject died, was veiled by another witch, or left the position circle"));
		}
		return;
	}

	// ── (2) ACQUIRE AND BEGIN ────────────────────────────────────────────────────────────────
	// ⛔ No cooldown between casts and ⛔ none is wanted: an interrupted cast costs ⛔ nothing
	// (WITCH-§4 — no veil, no partial state, no cost), so she simply re-acquires on the next poll.
	if (ASummonedUnit* const Subject = FindWitchVeilTarget(CircleCenter, CircleRadius))
	{
		BeginWitchCast(Subject);
	}
}

bool ASummonedUnit::ResolveWitchPositionCircle(FVector& OutCenter, float& OutRadius) const
{
	// ── THE J-W5 FALLBACK, WRITTEN ⛔ FIRST so every early return below is already correct ─────
	// "the ungrouped fallback is the card's own Range column" — centred on ⛔ HERSELF, which is the
	// Cleric's shipped shape verbatim (FindNearestDamagedFriendly ranks within AttackRange of the
	// healer). AttackRange IS the bound row Range; the constant is only the Range-0 backstop.
	OutCenter = GetActorLocation();
	OutRadius = (AttackRange > 0.f) ? AttackRange : WitchVeilRadiusFallbackUU;

	if (CommandGroupId == INDEX_NONE)
	{
		return false;
	}

	// The live group, resolved every call and ⛔ never cached — the shipped rule for this struct
	// (the array mutates on confirm/steal/prune, so a cached pointer dangles). M8 TEAM LAW: the
	// OWNING-TEAM controller resolve, ⛔ never GetFirstPlayerController().
	UWorld* const World = GetWorld();
	const ASiegePlayerController* const PC = ASiegePlayerController::FindControllerForTeam(World, Team);
	const FSiegeUnitGroup* const Group = PC ? PC->FindUnitGroup(CommandGroupId) : nullptr;
	if (!Group)
	{
		return false; // a dead id — UpdateState's own self-heal clears it on this same tick
	}

	// ⚠️⚠️ THE TRAP, AND IT IS THE ⛔ COMMON CASE RATHER THAN AN EDGE ONE: a FOLLOW group reuses
	// this struct UNCHANGED and carries PositionRadius == 0 with PositionCenter == ZeroVector
	// (UnitCommand.h says so at the struct). ⛔ Follow is ALSO the SPAWN DEFAULT for every
	// follow-eligible Blue unit, and the witch is follow-eligible (Support) — so a freshly played
	// witch ⛔ HAS a group, and a check of `Group != nullptr` alone would centre her circle on the
	// ⛔ WORLD ORIGIN and she would never veil anybody, ⛔ silently, forever.
	// ⭐ Both terms are kept: the TYPE says what the group means, the RADIUS is the structural
	// belt for any future zero-radius zone.
	if (Group->Type == ESiegeGroupCommandType::Follow || Group->PositionRadius <= 0.f)
	{
		return false;
	}

	// ⭐ WITCH-§4's ruling, and this pair of lines is the whole of it: his "position circle" is the
	// group order's STAGE-2 POSITION zone — ⛔ never MARK-§'s war-map circle_1..9 (widget space),
	// and ⛔ never the ATTACK zone (that is the tier-1 ENGAGE trigger, and a witch never engages).
	OutCenter = Group->PositionCenter;
	OutRadius = Group->PositionRadius;
	return true;
}

bool ASummonedUnit::IsWitchVeilCandidate(const ASummonedUnit* Candidate, const FVector& CircleCenter, float CircleRadius) const
{
	// ⛔ Never herself (WITCH-§4 / J-W6 default NO — she is a support unit, not a stealth unit),
	// and never a dead or half-destroyed one.
	if (!IsValid(Candidate) || Candidate == this || Candidate->IsUnitDead())
	{
		return false;
	}

	// FRIENDLY only. "make them invisible" is a gift; a veiled ENEMY would be a bug that hides the
	// other side's army from its own player.
	if (Candidate->GetTeamId() != Team)
	{
		return false;
	}

	// ══ ⭐⭐ "the nearest ⛔ VISIBLE unit" — HIS WORD, READ THROUGH THE ⛔ ONE SHIPPED RULE ══════
	// ⛔⛔ THIS IS THE LINE THAT IS EASY TO GET BACKWARDS, SO THE REASONING IS WRITTEN OUT.
	// "Visible" here means ⛔ NOT ALREADY VEILED — do not spend three seconds re-veiling somebody
	// who is already invisible. The shipped rule for that is FSiegeCombatStatics::IsAgentVisibleTo
	// (TASK-829's, the ⛔ ONE veil consult in the project), and it takes the team ⛔ DOING THE
	// LOOKING.
	// ⚠️⚠️ MEASURED, AND IT IS WHY THE ARGUMENT IS THE ⛔ ENEMY'S TEAM AND ⛔ NOT `Team`: the
	// predicate checks SAME-TEAM ⛔ FIRST AND UNCONDITIONALLY (WITCH-§2 lane 4 — an invisible unit
	// its own player cannot see is a BUG), so IsAgentVisibleTo(Team, Candidate) is ⛔ TRUE FOR
	// EVERY FRIENDLY, veiled or not, and would filter ⛔ nothing at all. Asking it through the
	// ⛔ enemy's eyes is the ⛔ only question that has a veil in its answer — and it is the ⛔ same
	// sentence the card is: ⭐ she veils the units the ENEMY CAN STILL SEE.
	// ⛔ Deliberately ⛔ NOT `Candidate->IsInvisible()`: that would be a SECOND expression of a rule
	// WITCH-§1 exists to hold at ⛔ ONE, and the day "visible" grows a term this site would ⛔ not
	// inherit it. ETeamId has exactly two values (TeamId.h), so this mapping is total.
	const ETeamId EnemyTeam = (Team == ETeamId::Blue) ? ETeamId::Red : ETeamId::Blue;
	if (!FSiegeCombatStatics::IsAgentVisibleTo(EnemyTeam, Candidate))
	{
		return false;
	}

	// ⛔ "not-currently-being-veiled" (WITCH-§4's fourth term). Two witches must not both burn
	// three seconds on the same unit — the second one's cast would land on an already-veiled
	// subject and be wasted. ⭐ The link is WEAK, so a witch who died mid-cast leaves this ⛔ null
	// and her abandoned subject is immediately available again rather than sealed forever.
	// ⛔ `!= this` matters: our OWN in-flight cast must not disqualify its own subject, or the
	// validation pass in UpdateWitchCast would cancel the cast it is validating on its first tick.
	const ASummonedUnit* const OtherCaster = Candidate->IncomingWitchCaster.Get();
	if (OtherCaster && OtherCaster != this)
	{
		return false;
	}

	// ⛔ INSIDE THE CIRCLE — a 2D disc, the shipped zone-membership idiom (UpdateStateGrouped's
	// own zone tests and AcquireEnemyNearPoint's candidate filter both read exactly this way).
	// 2D and not 3D deliberately: FSiegeUnitGroup documents both radii as "2D disc", and a
	// height term would make a unit on a tower ledge un-veilable for a reason nobody drew.
	return FVector::DistSquared2D(Candidate->GetActorLocation(), CircleCenter) <= FMath::Square(CircleRadius);
}

ASummonedUnit* ASummonedUnit::FindWitchVeilTarget(const FVector& CircleCenter, float CircleRadius) const
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	// ⭐ THE CLERIC'S SHIPPED SHAPE, ⛔ NOT A NEW ONE (TASK-830 spec (1)): the same
	// TActorIterator<ASummonedUnit> sweep, the same strict-improvement tie-break and the same
	// closest-point distance function FindNearestDamagedFriendly uses, with the ⛔ predicate
	// swapped from "damaged" to WITCH-§4's.
	// ⛔ NOT routed through FSiegeCombatStatics::GatherFriendlyAgents, and the reason is measured:
	// that gather returns AActor* over every ITeamAgent (castles, buildings, the hero) and this
	// search wants ⛔ units only — it would cost a Cast per candidate to get back to where this
	// iterator already starts. ⚠️ It is also the ⛔ FRIENDLY lane, which WITCH-§2 rules is ⛔ NEVER
	// veil-suppressed, so nothing is being smuggled around WITCH-§1's funnel: the veil term in
	// this search is the explicit IsAgentVisibleTo call inside IsWitchVeilCandidate.
	ASummonedUnit* BestUnit = nullptr;
	float BestDistance = TNumericLimits<float>::Max();

	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		ASummonedUnit* const Unit = *It;
		if (!IsWitchVeilCandidate(Unit, CircleCenter, CircleRadius))
		{
			continue;
		}

		// ⭐ "NEAREST" IS MEASURED FROM THE ⛔ WITCH, ⛔ not from the circle's centre. His sentence
		// is "walk up to nearby units": the subject she picks is the one she is closest to, and the
		// circle is the ⛔ leash on that choice rather than the origin of it. Ranking from the
		// centre instead would send her past a unit at her elbow to one across the zone.
		const float Distance = GetDistanceToTarget(MyLocation, Unit);
		if (Distance < BestDistance)
		{
			BestUnit = Unit;
			BestDistance = Distance;
		}
	}

	return BestUnit;
}

void ASummonedUnit::BeginWitchCast(ASummonedUnit* Subject)
{
	if (!IsValid(Subject))
	{
		return;
	}

	// ⛔ NOTHING OBSERVABLE HAPPENS HERE, AND THAT IS WITCH-§4 RATHER THAN THRIFT: an interrupted
	// cast must leave ⛔ no veil, ⛔ no partial state and ⛔ no cost, so the ⛔ only way to guarantee
	// that is for the start to have nothing to undo. ⛔ No gold is spent (the card was paid for at
	// PLAY time), ⛔ no flag is set on the subject, ⛔ no veil is granted "optimistically".
	WitchCastTarget = Subject;
	Subject->IncomingWitchCaster = this;

	// ⛔ ONE-SHOT (bLoop=false). A looping cast timer would re-veil the same subject every three
	// seconds forever, which is a cooldown wearing a cast's clothes — WITCH-§3 forbids exactly that.
	// The floor mirrors StartHealing's: SetTimer with a non-positive rate ⛔ CLEARS instead of
	// scheduling (qa/TASK-021 WARN-1), which would latch IsCastingVeil() false while
	// WitchCastTarget stayed set — a witch who can never finish and never re-acquire.
	GetWorldTimerManager().SetTimer(WitchCastTimerHandle, this, &ASummonedUnit::CompleteWitchCast,
		FMath::Max(WitchCastSeconds, 0.05f), /*bLoop=*/ false);

	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASummonedUnit '%s' (Witch, team %d): veil cast BEGUN on '%s' — %.2f s, interruptible by damage to either of them (WITCH-§4)."),
		*GetNameSafe(this), static_cast<int32>(Team), *GetNameSafe(Subject), FMath::Max(WitchCastSeconds, 0.05f));
}

void ASummonedUnit::ClearWitchCastChannel()
{
	// ⛔ THE ⛔ ONE TEARDOWN. Both exits from a cast (cancelled, completed) come through here, so
	// the caster->subject and subject->caster halves of the link can ⛔ never be released by one
	// path and left dangling by the other.
	GetWorldTimerManager().ClearTimer(WitchCastTimerHandle);

	if (ASummonedUnit* const Subject = WitchCastTarget.Get())
	{
		// ⛔ ONLY if it still points at US. A second witch that has since begun her own cast on this
		// subject owns the back-pointer now, and clearing it would silently hand her subject to a
		// third witch while she is still channelling.
		if (Subject->IncomingWitchCaster.Get() == this)
		{
			Subject->IncomingWitchCaster = nullptr;
		}
	}

	WitchCastTarget = nullptr;
}

void ASummonedUnit::CancelWitchCast(const TCHAR* Reason)
{
	// Idempotent and free for every unit that is not a witch mid-cast, which is why the interrupt
	// sites can call it unconditionally instead of each re-deriving whether there is anything to do.
	if (!IsCastingVeil() && !WitchCastTarget.IsValid())
	{
		return;
	}

	const FString SubjectName = GetNameSafe(WitchCastTarget.Get());
	ClearWitchCastChannel();

	// ⛔⛔ NOTE WHAT IS ⛔ NOT HERE, because its absence is the rule: there is ⛔ no
	// BreakInvisibility call on this path. An interrupted cast never veiled anybody, so there is
	// nothing to break — and the WITCH's own veil is broken by a ⛔ SUCCESSFUL cast only (J-W3),
	// since an interrupted cast is not an act that completed.
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASummonedUnit '%s' (Witch, team %d): veil cast on '%s' INTERRUPTED — %s. No veil, no partial state, no cost (WITCH-§4)."),
		*GetNameSafe(this), static_cast<int32>(Team), *SubjectName, Reason);
}

void ASummonedUnit::InterruptIncomingWitchCast(const TCHAR* Reason)
{
	// The SUBJECT half of his "interupted if the witch OR unit that is turning invisible are
	// attacked". Costs one weak-pointer test on units nobody is casting on, which is all of them
	// almost all of the time.
	if (ASummonedUnit* const Caster = IncomingWitchCaster.Get())
	{
		Caster->CancelWitchCast(Reason); // clears BOTH ends through the one teardown
	}

	// Belt for the one shape the line above cannot fix: a back-pointer left by a witch who was
	// destroyed (weak ⇒ already null here) or whose channel was torn down without us. A stale
	// non-null link would make this unit permanently invisible to every future witch's
	// "not-currently-being-veiled" term, which is a silent un-targetability rather than a crash.
	IncomingWitchCaster = nullptr;
}

void ASummonedUnit::CompleteWitchCast()
{
	// Reached ⛔ only from the one-shot timer, i.e. WitchCastSeconds elapsed with ⛔ no interrupt.
	ASummonedUnit* const Subject = WitchCastTarget.Get();

	// ⛔ The same defense-in-depth gate PerformHeal and PerformAttack carry: every cancel site
	// clears this timer, and this is the belt in case one ever does not.
	const bool bCasterFit = !bDead && bStatsLoaded && !bAIFrozen && !bSpellFrozen;

	// ⭐ ONE LAST VALIDATION AGAINST THE ⛔ LIVE CIRCLE. The 0.25 s poll can be up to a quarter of a
	// second stale, and the subject may have died, drifted out or been veiled by another witch in
	// that window — WITCH-§4 says those cancel, so they must cancel here too rather than "nearly".
	FVector CircleCenter = FVector::ZeroVector;
	float CircleRadius = 0.f;
	ResolveWitchPositionCircle(CircleCenter, CircleRadius);
	const bool bSubjectFit = IsWitchVeilCandidate(Subject, CircleCenter, CircleRadius);

	// Teardown FIRST: every exit below must leave a clean channel, and the two calls after it are
	// side effects on OTHER state, so nothing here can be left half-done by an early return.
	ClearWitchCastChannel();

	if (!bCasterFit || !bSubjectFit)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASummonedUnit '%s' (Witch, team %d): veil cast expired on an INELIGIBLE subject '%s' — no veil (WITCH-§4)."),
			*GetNameSafe(this), static_cast<int32>(Team), *GetNameSafe(Subject));
		return;
	}

	// ══ ⭐⭐⭐ THE FEATURE. THE ⛔ ONE CALLER OF GrantInvisibility IN THE PROJECT ════════════════
	// TASK-829 shipped that door with ⛔ zero callers and said so out loud: without this line
	// `bIsInvisible` can ⛔ never be true and the suppression branch inside GatherHostileAgents is
	// ⛔ unreachable code. ⛔ This is the ⛔ only write-true site's ⛔ only site.
	// ⛔ Do ⛔ NOT add a second one — the veil flag is private and unreflected precisely so that a
	// grep for this name is the ⛔ complete list of ways a unit can BECOME invisible.
	const bool bVeiled = Subject->GrantInvisibility();

	// ══ ⭐ HER OWN CAST BREAKS HER OWN VEIL — J-W3, AND ⛔ ONLY ON A COMPLETED CAST ════════════
	// WITCH-§3 lists the witch's cast among the six acts: ⛔ she is ACTING. Consistency beats
	// special-casing, and a self-veiling witch who never broke would be ⛔ permanently
	// untargetable-by-acquisition — the exact outcome WITCH-§0 refuses.
	// ⛔ PLACED HERE AND ⛔ NOWHERE ELSE: an INTERRUPTED cast produces no veil, no partial state,
	// no cost — ⛔ and therefore no break. The act is the landing, never the attempt.
	// ⛔ It is idempotent on the overwhelmingly common case: a witch who was never veiled herself
	// takes ApplyBreak's no-edge early-out and this line costs one bool test.
	BreakInvisibility(ESiegeVeilBreakReason::Cast);

	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASummonedUnit '%s' (Witch, team %d): veil cast COMPLETED on '%s' (newly veiled: %s). Permanent until one of the WITCH-§3 acts breaks it."),
		*GetNameSafe(this), static_cast<int32>(Team), *GetNameSafe(Subject), bVeiled ? TEXT("yes") : TEXT("no - it was already veiled"));
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  THE CAST'S ⛔ READ-ONLY SURFACE (TASK-830 item (8); law WITCH-§9.1 / §9.2 / §9.3 / §9.6)
//
//  ⛔ ZERO BEHAVIOUR LIVES HERE. Three functions, all const, that only ANSWER — nothing below
//  starts, cancels, completes or times anything. The cast state machine is above; this is the
//  window the UI lane (TASK-860/861) looks through, and it exists in THIS task only because
//  TASK-860 cannot re-open this file (serialised, contended, one compile slot per re-entry).
//
//  ⭐⭐ TWO-ENDED, AND THAT WORD IS LOAD-BEARING. WITCH-§9.2 refuses the cheap default — one bar
//  on the caster, which is what every game ships — because it fails requirement 2, and
//  requirement 2 is Jonathan's own sentence: "...can be interupted if the witch OR unit that is
//  turning invisible are attacked." ⇒ the tell must appear on ⛔ EXACTLY the two actors you can
//  attack to break the cast, so a player who has never read a tooltip can read the counter off
//  the screen. Both of them answer these two questions about THEMSELVES.
//
//  ⛔⛔ ONE CLOCK, TWO READERS — the shape that makes it safe. The subject ⛔ PULLS through its
//  own back-pointer; the witch ⛔ NEVER PUSHES a percent onto the subject's widget (WITCH-§9.3
//  forbids that by name). A push is a second source of truth, and it ⛔ STRANDS A BAR on the
//  subject the moment the witch dies mid-cast — which the interrupt rule makes the ⛔ COMMON
//  case, since killing the caster IS the counterplay. Both links are weak, so the subject's bar
//  goes down on the SAME frame the witch does, for free and with no teardown to remember.
//
//  ⛔⛔ AND THE ONE NUMBER THIS BLOCK MUST ⛔ NOT READ: the denominator is the ⛔ LIVE TIMER'S OWN
//  RATE, ⛔ never the tunable. Two independent reasons, both real:
//    (1) BeginWitchCast arms with FMath::Max(<the tunable>, 0.05f), so a row tuned BELOW the
//        floor would make the tunable the ⛔ WRONG denominator — the fill would run past 100%
//        and the clamp would hide the disagreement rather than surface it. The timer's rate is
//        the number the cast is ACTUALLY running on, which is the number the bar must show.
//    (2) SiegeInvisibilityTest's cast-duration row pins ⛔ EVERY read of that tunable in this
//        file inside BeginWitchCast (it asserts the in-body count EQUALS the whole-file count).
//        A read here would turn that row ⛔ RED — in a test named for the cast's DURATION, which
//        names neither this surface nor the task that added it.
//  ⚖️ And the pin is ⛔ RIGHT rather than merely in the way: its own comment refuses "a countdown
//  displayed and then acted on", which is precisely how a CAST duration becomes a VEIL duration.
// ═════════════════════════════════════════════════════════════════════════════════════════

const ASummonedUnit* ASummonedUnit::ResolveCastClockOwner() const
{
	// ⛔⛔ THE CLASS-DEFAULT-OBJECT GUARD, AND IT IS ⛔ NOT DEFENSIVE PADDING — it is the ⛔ only
	// thing standing between this surface and a null dereference. AActor::GetWorld() returns
	// nullptr for a CDO ⛔ BY CONSTRUCTION (its RF_ClassDefaultObject early-out), and
	// AActor::GetWorldTimerManager() is a bare `GetWorld()->GetTimerManager()`.
	// ⚠️ THIS SURFACE IS ON IHealthBarProvider, so it is reachable from anything holding a unit
	// ⛔ CLASS rather than a unit — and the FIRST caller in the tree is the test proving a
	// non-witch answers false, which has ⛔ no world to spawn one in (the house rule bans
	// SpawnActor/CreateWorld under Siegebound/Tests/). Without this line that test ⛔ CRASHES
	// the runner rather than failing it.
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}

	// ── (1) THE WITCH END ─────────────────────────────────────────────────────────────────────
	// IsVeilCaster() first purely as the cheap out: one FName compare, so the shipped fleet never
	// reaches a timer-manager lookup on a path the UI polls. It is ⛔ not load-bearing for
	// correctness — no non-witch can ever arm that handle — only for cost.
	// ⚠️ HER OWN CAST WINS when a unit is somehow both ends at once (a second witch may legally
	// target a witch, since IsWitchVeilCandidate only refuses the caster HERSELF). One bar, one
	// event, and it is the one she can act on — stated because it is a real reachable state.
	if (IsVeilCaster() && IsCastingVeil())
	{
		return this;
	}

	// ── (2) THE SUBJECT END — ⛔ PULLED, ⛔ never pushed ───────────────────────────────────────
	// One weak-pointer test for every unit nobody is casting on, which is all of them almost all
	// of the time. A destroyed witch reads null here with no teardown having to have run.
	const ASummonedUnit* const Caster = IncomingWitchCaster.Get();
	if (Caster == nullptr || Caster->GetWorld() == nullptr)
	{
		return nullptr;
	}

	// ⛔⛔ BOTH HALVES OF THE LINK MUST STILL AGREE, and asking only the back-pointer is the bug
	// that would ship: ClearWitchCastChannel deliberately releases a subject's back-pointer ONLY
	// when it still points at the clearing witch, so a subject re-targeted by a SECOND witch
	// keeps a link the first witch no longer owns. Reading the clock off a caster whose forward
	// pointer has moved on paints a bar for a cast that is no longer aimed here.
	if (Caster->WitchCastTarget.Get() != this || !Caster->IsCastingVeil())
	{
		return nullptr;
	}

	return Caster;
}

bool ASummonedUnit::IsCastInProgress() const
{
	// The GATE the cast row's collapse is driven from (WITCH-§9.3: the container is Collapsed
	// while this is false — ⛔ collapsed rather than hidden, or a laid-out empty row would move
	// the HP bar on every unit in the game).
	// ⛔⛔ DELIBERATELY ⛔ NOT `GetCastProgressPercent() > 0.f`: a cast that has just begun reads
	// 0%, so that phrasing would keep the bar hidden for the first poll of every cast — the exact
	// moment WITCH-§9.1 requirement 1 ("a cast is RUNNING, on THIS one") exists to serve. The two
	// accessors are independent questions sharing ⛔ one resolver, never one derived from the other.
	return ResolveCastClockOwner() != nullptr;
}

float ASummonedUnit::GetCastProgressPercent() const
{
	const ASummonedUnit* const ClockOwner = ResolveCastClockOwner();
	if (ClockOwner == nullptr)
	{
		return 0.f;
	}

	// ⛔ DERIVED PER CALL, ⛔ NEVER STORED — and that is what makes WITCH-§4's "an interrupt leaves
	// no partial state" true of the ⛔ TELL as well as of the veil. The one teardown clears the
	// timer, so the very next read is 0 with ⛔ nothing to reset and ⛔ nobody to remember to reset
	// it. A cached percent would survive the cancel and freeze the bar mid-flight — a bar that
	// says a cast is still running after the player already interrupted it.
	const FTimerManager& CastClock = ClockOwner->GetWorldTimerManager();
	const float Elapsed = CastClock.GetTimerElapsed(ClockOwner->WitchCastTimerHandle);
	const float Rate = CastClock.GetTimerRate(ClockOwner->WitchCastTimerHandle);

	// Both accessors answer -1.f for a handle the manager does not know. ⛔ Unreachable while the
	// resolver above requires a LIVE timer, and kept anyway because the failure is silent and
	// backwards: a negative rate divides into a percent that reads FULL, i.e. the instrument
	// would report "this cast is about to land" at the exact moment it does not exist.
	if (Rate <= 0.f || Elapsed < 0.f)
	{
		return 0.f;
	}

	// ⭐⭐ 0..100, ⛔ NEVER 0..1 — the shipped boost-percent convention (WITCH-§9.3 pins it), with
	// the widget dividing by 100 on its side exactly as the HP row already does.
	// ⛔ THE CLAMP IS THE GUARANTEE THE SURFACE CANNOT LIE IN THE ONE DIRECTION THAT MATTERS: it
	// can never report more than 100, so "the fill reached the end" is only ever produced by a
	// cast that ran its whole window — which is what leaves COMPLETED distinguishable from BROKEN
	// (WITCH-§9.1 row 4, the requirement that has ⛔ no tell at all today).
	return FMath::Clamp(100.f * Elapsed / Rate, 0.f, 100.f);
}

void ASummonedUnit::UpdateStateWitch()
{
	// Reached from UpdateState ⛔ only for a veil caster carrying ⛔ no live ZONE order — a grouped
	// witch is run by the ⛔ shipped UpdateStateGrouped instead, where CanEverAttack() false forces
	// her target null (its guard 2) and she station-keeps at PositionCenter + her sunflower offset.
	// ⭐ That is the ⛔ sorcerer's shipped "commandable but never fights" path, reused with ⛔ zero
	// new code in that function — which is also what makes her HOLD/AMBUSH behaviour identical to
	// a unit type Jonathan has already played.

	// ══ THE NEVER-ATTACK SEAL — PER BODY, the UpdateStateFollow idiom ════════════════════════
	// This body calls ⛔ none of AcquireTarget / AcquireEnemyNearPoint / EnterAttack, and forces
	// the target null every tick so nothing downstream (the bars, the debug readbacks,
	// TrackChargeMovement) can read a stale one off a unit that will never swing.
	CurrentTarget = nullptr;

	// (1) ⭐ THE UNIT SHE IS VEILING IS THE THING SHE WALKS AT — his "walk up to nearby units and
	//     make them invisible", and the Cleric's shipped shape exactly (EnterAdvance toward a pawn
	//     stops ~0.8xRange short, comfortably inside her own circle, so she closes without
	//     colliding). ⛔ Walking does ⛔ NOT interrupt the cast: WITCH-§3's carve-out is walking.
	AActor* Goal = WitchCastTarget.Get();

	// (2) Nobody to veil: escort the line, so she is WHERE the units that need veiling are. Same
	//     query, same cadence, as the shipped Cleric's follow goal.
	if (!Goal)
	{
		Goal = FindNearestFriendlyCombatUnit();
	}

	// (3) ⚖️ THE LATCHED T/E STANCE — his "controllable by all commands", for the ⛔ one case the
	//     two commands can actually mean something to a unit that cannot fight. The stance gate in
	//     UpdateState is Profile==Standard and a Support unit has ⛔ never entered it, so the read
	//     is done ⛔ here rather than by widening that gate and handing a 0-damage unit the whole
	//     acquire-and-march machine. It is deliberately the ⛔ LAST rung: a witch with a subject or
	//     an escort keeps doing her job under every stance, and the stance only decides where a
	//     ⛔ LONE witch walks — Defend falls back to the own castle, Attack pushes at the enemy one
	//     (the same two goals UpdateStateStandardCommanded uses, via the same two finders).
	//     ⛔ Blue-only, mirroring the shipped gate's own team term.
	if (!Goal && Team == ETeamId::Blue)
	{
		if (UWorld* const CmdWorld = GetWorld())
		{
			if (const ASiegePlayerController* const PC = ASiegePlayerController::FindControllerForTeam(CmdWorld, Team))
			{
				if (PC->HasIssuedCommand())
				{
					Goal = (PC->GetCurrentCommand() == ESiegeUnitCommand::Defend)
						? static_cast<AActor*>(FindOwnCastle())
						: FindNearestEnemyCastle();
				}
			}
		}
	}

	// (4) A lone witch with no command stands down — the shipped lone-Cleric behaviour.
	if (!Goal)
	{
		EnterIdle();
		return;
	}

	EnterAdvance(Goal);
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
	//
	// ══ THE TOWER-§9 CLIMB DISARM RIDES THIS SAME POINT — GUARD 1 of 3 (TASK-738) ════════════
	// Jonathan: "they should be attackable while climbing, but they can't attack back."
	// ⭐ TWO INDEPENDENT TERMS, and keeping them independent is the requirement, not a style
	// choice: CanEverAttack() is the `const` CLASS-IDENTITY seal (permanent, per class), and
	// IsClimbing() is a TRANSIENT per-instance state that lasts about three and a half seconds.
	// ⛔ Overriding CanEverAttack() to implement the disarm would have made a Sorcerer's
	// permanent inability and a Footman's brief climb indistinguishable in the code — TOWER-§9.2
	// requires that distinction to survive. ⛔ It is also NOT a fourth guard point and NOT a new
	// suppression mechanism: the same three shipped points, with one more term.
	// ⭐ Standing DOWN to Idle rather than returning is correct for a climber too — it clears any
	// attack timer and parks the state machine honestly, and none of EnterIdle's work touches the
	// movement mode, so the ascent is unaffected.
	if (!FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing()))
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
	//
	// ══ THE TOWER-§9 CLIMB DISARM RIDES THIS SAME POINT — GUARD 3 of 3 (TASK-738) ════════════
	// ⭐ AND THIS IS THE ONE THAT WOULD ACTUALLY CATCH A SHOT. A unit that was mid-fight when
	// its climb began has a LOOPING AttackTimerHandle in flight; BeginLadderClimb clears it (the
	// structural half of the disarm), and this is the belt that covers any future re-arm — a new
	// state body, a serialized BP value, a subclass poll. ⛔ NOT a fourth guard point: the same
	// shipped line, with the transient climb term beside the permanent class seal.
	if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen
		|| !FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing()))
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
	// ⭐⭐⭐ FIRING GATE 6 of 6, FOG-CLAMPED (TASK-1008) — ⛔ AND THIS IS THE ONE THAT ACTUALLY
	// STOPS THE ARROW. The five gates above decide whether to ENTER the attack state; this one
	// runs on the attack CADENCE timer and is the last thing between a held target and a shot.
	// 🧑 "no ranged units will be able to fire onto anything above 20 feet away" is enforced HERE
	// on every cadence tick, which is what makes fog a STANDING CONDITION rather than an
	// acquisition-time filter (FOG-§9.7) — an implementation that filtered only acquisition would
	// pass every ordinary-case test while leaving engaged units shooting through the fog.
	// ⚠️ DECLARED RESIDUAL (FOG-§9.7a, unchanged): an arrow ALREADY IN THE AIR still lands. The
	// gate is the decision to FIRE, never the projectile.
	FVector ImpactPoint = FVector::ZeroVector;
	if (GetDistanceToTarget(GetActorLocation(), Target, ImpactPoint) > ApplyFogVisionCeilingUU(AttackRange))
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
		//
		// ══ VEIL BREAK — `Attack`, RANGED delivery (TASK-829; WITCH-§3, first in his list) ═════
		// ⭐ THE VEIL BREAKS WHEN THE SHOT IS ⛔ LOOSED, ⛔ not when the target was chosen.
		// WITCH-§3a: acquiring a target is ⛔ NOT acting — a veiled Archer may pick a target,
		// cross the field, close to range and draw its bow, and ⛔ stay veiled. A break at
		// AcquireTarget would un-veil the ⛔ entire approach and ⛔ delete the card.
		// ⚠️ Deliberately a SECOND call rather than one before the branch: WITCH-§3a's measured
		// ledger names the two delivery modes as two sites, and a reviewer verifying by symbol
		// must find a break adjacent to ⛔ each act. They are mutually exclusive at runtime.
		BreakInvisibility(ESiegeVeilBreakReason::Attack);
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

		// ══ VEIL BREAK — `Attack`, MELEE delivery (TASK-829; WITCH-§3, first in his list) ══════
		// ⭐ THE VEIL BREAKS WHEN THE ⛔ BLOW LANDS, and this is the line where it lands.
		// ⚠️ MEASURED AND ⛔ DELIBERATELY NOT HERE: the Cavalry CHARGE wind-up
		// (TrackChargeMovement) is ⛔ WALKING — it only accumulates a multiplier that
		// ComputeOutputDamage consumes above. A veiled Knight ⛔ stays veiled while it runs; it
		// un-veils on the strike, ⛔ not when the horse starts.
		// ⚠️ Placed BEFORE ApplyDamage on purpose: the receiver's TakeDamage can re-acquire
		// within this same call stack, and it must see a unit that has already revealed itself
		// rather than one that reveals a line later.
		BreakInvisibility(ESiegeVeilBreakReason::Attack);

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

	// static_cast: ⛔ CORRECTED 2026-09-04 (TASK-1000) — this used to say GetTimeSeconds() "is
	// float today", and that is FALSE. UWorld::GetTimeSeconds() returns DOUBLE (UE 5.8,
	// Engine/Classes/Engine/World.h), so this is a REAL narrowing, not a defensive no-op: it is
	// written explicitly because the build treats warnings as errors, and it is the same LWC
	// idiom this file already uses on FVector components at the castle-bounds read above.
	// ⚠️ The precision is fine for what this measures — a per-unit stuck-watchdog delta over a
	// match-length clock, never an absolute timestamp compared across sessions.
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
	// 0.25 s poll that already runs a full-world interface enumeration in AcquireTarget
	// (TASK-828: now via FSiegeCombatStatics::GatherHostileAgents — same one scan per poll).
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

// ---------------------------------------------------------------------------
// ═══ NOTICE / ENGAGEMENT RADIUS — the per-unit channel (TASK-979 / TASK-1003) ═══
// 🧑 Jonathan, verbatim 2026-09-04: "lets fix it by changing the notice radius for
// all units to 5000 with no fog and still 609 under fog" (J-F28) and "lets make the
// leash radius 8000" (J-F27). ⛔ BOTH supersede his own earlier sentences the SAME
// DAY ("within 2000 units instead of 600 units" … "make the 3600 the NOTICE range
// for the longbowman"), which are kept in the header so a reader who remembers them
// finds their replacement.
// The 5000 is the DEFAULT, ⛔ never a cap. ⚠️ The Longbowman cell that used to
// justify this being a channel is RETIRED by the same ruling (TASK-1004 blanks it),
// so the column ships ENTIRELY SPARSE — ⭐ and the channel is STILL the deliverable:
// it is what lets a future card carry its own reach with zero code (FOG-§9.9).
// ---------------------------------------------------------------------------

float ASummonedUnit::ResolveNoticeRadiusUU(float ClassDefaultRadiusUU, float RowNoticeRangeUU)
{
	// ⛔⛔ THE SEAL FIRST, AND THE ORDER IS LOAD-BEARING. AMinerUnit and ASorcererUnit write
	// AggroRadius = 0.f in their CONSTRUCTORS as a class contract ("never attacks"). If the row
	// were consulted before this test, a card cell — data, reviewed by nobody — could hand a
	// sealed unit a live acquisition radius. Written as !(x > 0) rather than (x <= 0) so a NaN
	// class default, which fails every comparison, also lands here instead of falling through
	// into the row branch.
	if (!(ClassDefaultRadiusUU > 0.f))
	{
		return ClassDefaultRadiusUU;
	}

	// ⭐⭐ THE ROW WINS, ⛔ UNCLAMPED AND ⛔ UNWIDENED. This branch is a PASS-THROUGH IN BOTH
	// DIRECTIONS, and since TASK-1003 that is two separate refusals rather than one:
	//   • ⛔ NO FMath::Min against UnitEngagementRadiusUU — 5000 is the DEFAULT, not a cap. ⚠️ At
	//     5000 ⛔ no shipped card sits above it, so a `min` here is INERT against today's roster
	//     and would go GREEN against every data-derived assertion. The refusal is carried by a
	//     SYNTHETIC value above the default and by a structural probe, ⛔ never by a card.
	//   • ⛔ NO FMath::Max either — that is the spelling his 5000 made dangerous. A cell BELOW
	//     the class default (or any "normalise the sparse cell up to the default" pass) must come
	//     back out as itself, or the column stops being an opt-in channel and becomes a floor
	//     nobody voted for.
	// Either would be silent — no error, no log (SC-§60). The finiteness test is a math guard on
	// a corrupt cell, not a ceiling: it can only ever fall BACK to a number the class already had.
	if (RowNoticeRangeUU > 0.f && FMath::IsFinite(RowNoticeRangeUU))
	{
		return RowNoticeRangeUU;
	}

	// the sparse case — a blank/0 cell is "use the class default", which is how ~every card in
	// the table carries 5000 without twenty copies of the number existing anywhere. ⭐ Since
	// TASK-1004 this is the case for EVERY row in the game — the column is entirely sparse.
	return ClassDefaultRadiusUU;
}

float ASummonedUnit::ResolveEffectiveLeashRangeUU(float LeashRangeUU, float NoticeRadiusUU, float MarginMultiplier)
{
	// A non-finite notice radius cannot produce an ordering, so the raw leash is returned
	// untouched — the same fail-toward-the-shipped-value direction the fog seam takes.
	if (!FMath::IsFinite(NoticeRadiusUU))
	{
		return LeashRangeUU;
	}

	// ⛔ FLOORED AT 1.0, and it is the rule rather than paranoia: a margin below 1 would put the
	// leash INSIDE the notice radius and re-create the exact inversion this function closes —
	// UpdateState drops at > leash and AcquireTarget re-takes at <= notice, with no return
	// between them, so any overlap is a target released and re-taken in one 0.25 s poll. A NaN
	// multiplier also lands on 1.0 here (it fails the > test).
	const float SafeMultiplier = (MarginMultiplier > 1.f && FMath::IsFinite(MarginMultiplier)) ? MarginMultiplier : 1.f;

	// ⭐ FMath::Max, so the shipped LeashRange is a FLOOR and never a ceiling: at the pre-TASK-979
	// pair (900 / 600 × 1.5) this returns max(900, 900) = 900, BIT-IDENTICALLY the number the
	// game once shipped — which is what makes this a mechanism change rather than a balance one.
	// ⭐⭐ AT TODAY'S VALUES (TASK-1003) THE ⛔ FLOOR WINS AND THE PRODUCT IS INERT, AND THAT IS
	// ⛔ NOT A REASON TO COLLAPSE THIS EXPRESSION: max(8000, 5000 × 1.5 = 7500) = ⛔ 8000, 🧑 his
	// J-F27 number EXACTLY. The multiplier's ⛔ only remaining job is the FUTURE-CARD case — above
	// a NoticeRange of `LeashRange / MarginMultiplier` = 8000 / 1.5 = ⛔ 5333.33 uu the max flips
	// to the product (a 6000 cell ⇒ max(8000, 9000) = 9000) and the ordering re-derives itself
	// with ⛔ zero edits. ⛔ Hard-code a bare 8000 and that card inverts the ordering SILENTLY,
	// bringing back FOG-§9.8b's drop-then-re-acquire thrash with ⛔ no diff to point at.
	// ⇒ ⚖️ his 8000 sets the FIRST TERM; ⛔ it does not replace the expression.
	return FMath::Max(LeashRangeUU, NoticeRadiusUU * SafeMultiplier);
}

float ASummonedUnit::GetEffectiveLeashRangeUU() const
{
	return ResolveEffectiveLeashRangeUU(LeashRange, AggroRadius, LeashMarginMultiplier);
}

// ---------------------------------------------------------------------------
// ═══ THE ONE UNIT-SIDE FOG CHOKEPOINT (TASK-1008) ═══
// 🧑 Jonathan, verbatim 2026-09-04: "fog should make units DROP existing targets if they are
// outside the 609 range" and "yes clamp retention under fog" (FOG-§9.11's retention clause).
// ⛔ Before this row the fog ceiling bound ACQUISITION ONLY: a unit that already held a target
// chased it to the full 8000 leash and fired at its full card Range, in fog it provably could
// not see through. ⛔ That is not a number bug — the funnel runs at GATHER time and is
// structurally incapable of being asked "do I STILL hold this?" between gathers.
// ⛔ THE ONE FUNCTION BELOW IS THE WHOLE ANSWER, and its shape is the argument: it names the
// fog rule ONCE for this entire class, does no arithmetic, holds no state and takes no branch.
// ⛔ Nine reach sites call it; NONE of them may clamp for itself (FOG-§9.6 / FOG-§7's structural
// law — the exemption is expressed by WHICH FUNCTION a site calls, never by an `if`).
// ---------------------------------------------------------------------------

float ASummonedUnit::ApplyFogVisionCeilingUU(float RequestedReachUU) const
{
	// ⛔⛔ THE ONLY FOG-RULE CALL IN THIS FILE, AND IT IS PINNED AT ONE BY A MACHINE:
	// Tests/SiegeAcquisitionFunnelTest.cpp test 9 counts the seam's name here and requires
	// EXACTLY ONE occurrence, INSIDE this body, via an authorised-chokepoint table that carries a
	// written reason per entry. ⛔ A second call anywhere in ASummonedUnit is the forgotten-
	// guard-point failure the whole funnel exists to prevent, and it turns that row RED.
	// ⛔ Do NOT "fix" such a red by relaxing the token list — route the new site through HERE.
	//
	// ⛔ NO LOCAL `min`, NO ceiling literal, NO fog-state read, NO bRangedAttack gate (that flag
	// is PROJECTILE DELIVERY, so CrystalTower ships bRanged=false at Range 800 and would walk
	// straight through such a gate — FOG-§9.8c). The seam owns all of it and hands back a REACH:
	// the state itself never crosses its boundary in either direction, so no caller here can
	// learn the weather, branch on it, or become a second door.
	//
	// ⛔ NEVER CACHE THE RETURN (FOG-§9.6). Fog rises and clears between polls, so the value must
	// be asked for again every evaluation. The measured cost of that liveness is one
	// TActorIterator<AFogVolume> walk per call inside AFogVolume::Find; if it ever bites, the
	// cache belongs THERE, never here.
	return FSiegeCombatStatics::ResolveFogClampedReachUU(GetWorld(), RequestedReachUU);
}

float ASummonedUnit::GetClassDefaultEngagementRadiusUU() const
{
	// ⛔⛔ THIS INSTANCE'S CLASS, ⛔ NEVER GetDefault<ASummonedUnit>(). Building.cpp:302 is this
	// project's already-shipped instance of the other spelling — a per-class ceiling read off
	// the BASE CDO, so every subclass value is read straight past. Here the base says 5000 while
	// AMinerUnit's and ASorcererUnit's CDOs say 0: reading the base would stamp 5000 over both
	// class seals, and a Blueprint that set its own AggroRadius would lose it the same way.
	if (const UClass* const MyClass = GetClass())
	{
		if (const ASummonedUnit* const ClassDefaults = MyClass->GetDefaultObject<ASummonedUnit>())
		{
			return ClassDefaults->AggroRadius;
		}
	}

	// impossible in practice; returning the live member keeps the resolver a no-op rather than
	// letting a null class silently mean "unsealed".
	return AggroRadius;
}

// ---------------------------------------------------------------------------
// ═══ HIGH GROUND — the elevation damage bonus (TASK-724; HIGH-§1/§2/§3) ═══
// Jonathan, verbatim: "make their attacks deal more damage the higher elevation
// they are. I would say that for every 5 feet that their elevation increases,
// their damage multiplier increases by 10%."
// ---------------------------------------------------------------------------

float ASummonedUnit::HeightAdvantageMultiplier(float AttackerZ, float TargetZ, float StepUU, float BonusPerStep)
{
	// ⛔ THE ZERO-DIVIDE GUARD, APPLIED BEFORE THE DIVISION (the HeightToBrightness /
	// MinArenaHalfExtentUu doctrine): HeightBonusStepUU is an EditDefaultsOnly float a
	// designer can zero, and an unguarded divide would put inf or NaN straight into a damage
	// number. Written as !(StepUU > 0) rather than (StepUU <= 0) so a NaN step — which fails
	// EVERY comparison — also lands here instead of propagating. A disabled step means NO
	// bonus (exactly 1.0), never an explosion.
	if (!(StepUU > 0.f))
	{
		return 1.f;
	}

	// ⭐ HEIGHT ABOVE THE TARGET, ⛔ NOT absolute world Z (HIGH-§2, his row R-1), and the
	// FMath::Max is the whole "bonus only when positive" rule: level ground and shooting
	// UPWARD both fall to 0 here and return EXACTLY 1.0. ⛔ There is deliberately no negative
	// branch — he asked for a bonus, and inventing a low-ground malus is inventing a mechanic.
	const float HeightAdvantageUU = FMath::Max(0.f, AttackerZ - TargetZ);

	// ⭐ CONTINUOUS (linear), ⛔ NOT stepped (his row R-4) — no FMath::FloorToFloat here, on
	// purpose: a floored rule puts invisible breakpoints on a hillside a player cannot see,
	// cannot aim for and cannot learn. ADDITIVE and UNCOMPOUNDED (the plain reading of "+10%
	// per 5 feet"): two steps is ×1.20, ⛔ not ×1.21. ⛔ AND NO CLAMP — he did not ask for a
	// cap (his row R-3), and the worst case (×2.44) was computed and handed to him instead.
	return 1.f + BonusPerStep * (HeightAdvantageUU / StepUU);
}

float ASummonedUnit::ComputeOutputDamage(const AActor* Target)
{
	// Base is the row Damage bound at LoadStatsAndStart (never hardcoded, GDD §3.0). Output composes
	// the Standard keyword multipliers in ONE place: Charge (spent here), Slayer (target-HP gated),
	// and the War Banner aura. Siege 200% is NOT composed here — it is applied fortification-side by
	// the damage TYPE (TASK-054), so composing it here would double-count. For a non-keyword,
	// un-auraed MELEE unit every factor is exactly 1.0, so this returns AttackDamage bit-for-bit.
	// ⚠️ THE WORD "MELEE" IN THAT SENTENCE IS LOAD-BEARING AS OF TASK-724 and is not decoration:
	// the high-ground factor at the bottom of this function is the ONE factor that can be non-1.0
	// on a keyword-free unit — and it can only ever be so for a bRangedAttack unit that genuinely
	// stands above its target. (The M7.7 lesson: prose that restates a behaviour drifts from it,
	// so this line was corrected in the same edit that made it necessary.)
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

	// ⭐ HIGH GROUND (TASK-724, HIGH-§2/§3) — THE ELEVATION BONUS'S ONE AND ONLY COMPOSE
	// POINT. ⛔ A second application site anywhere in the codebase is a QA blocker: it lands
	// here for the same reason the Ancient-Grounds factor above does — this ONE insertion
	// covers BOTH delivery modes, since a ranged unit's projectile damage is this very
	// composed value carried into FireProjectileAt.
	//
	// ⛔ GATED ON THE ALREADY-SHIPPED bRangedAttack MEMBER (bound from Row->bRanged at
	// LoadStatsAndStart), ⛔ NOT on a CardID list and ⛔ NOT on a name check — so the shipped
	// set is exactly the three ranged UNITS the data names (Archer / Wizard / Longbowman) and
	// a FUTURE ranged card inherits this behaviour from its own row with zero code. Melee
	// units skip the branch entirely, which is what keeps this function's contract literally
	// true: it still returns AttackDamage BIT-FOR-BIT for a non-keyword, un-auraed melee unit.
	// The hero is not an ASummonedUnit and never reaches this function at all (HIGH-§4).
	//
	// ⛔⛔ BOTH Z VALUES ARE GetActorLocation().Z, ON BOTH SIDES, NO EXCEPTIONS — a mixed
	// convention (origin vs capsule vs bounds) is how a sign error hides. ⚠️ THE KNOWN
	// CONSEQUENCE, STATED RATHER THAN DISCOVERED LATER: a large-footprint target (a castle)
	// reports its ORIGIN Z, which sits at its base, so a unit on level ground beside a castle
	// reads as ABOVE it. Same convention on both sides is what makes the difference mean
	// something at all.
	//
	// ⛔ NO TOWER AWARENESS OF ANY KIND — no bIsOnATower flag, no occupancy lookup, no tower
	// header included — and ⛔ NO read of the war map's elevation bake (WM-§8d / SHIP-§9: that
	// bake clamps at 1,000 uu and would silently stop scaling on exactly the towers and tall
	// hills this feature exists for). A unit on a hill and a unit on a tower at the same Z
	// therefore deal IDENTICAL damage, which is what lets TOWER-§ work for free.
	//
	// A null Target yields ×1.0 by skipping the branch — there is no height advantage over
	// nothing, and PerformAttack never reaches here with one anyway.
	if (bRangedAttack && Target != nullptr)
	{
		Output *= HeightAdvantageMultiplier(
			GetActorLocation().Z,
			Target->GetActorLocation().Z,
			HeightBonusStepUU,
			HeightBonusPerStep);
	}

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

	// TWO drivers, and the tick is enabled while EITHER wants it (RefreshActorTickEnabled).
	// UpdateLunge early-returns when no cycle is running, so the climb still gets its frame —
	// ⛔ do not fold these into an if/else.
	// ⚠️ TASK-784 DELIBERATELY ADDED ⛔ NOTHING HERE. The contact trigger's poll would have been a
	// natural third line and would have NEVER RUN: this tick is OFF for any unit that is neither
	// mid-lunge nor already climbing, which is exactly the state a contact climb starts from. It
	// rides StateTimerHandle instead — see TryContactClimbAtNearestLadder.
	UpdateLunge(DeltaSeconds);      // the lunge visual (TASK-020)
	TickLadderClimb(DeltaSeconds);  // the ladder ascent (TASK-738)
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
	RefreshActorTickEnabled(); // TASK-738: the composed predicate — the climb may also want the tick
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
	// ⚠️ TASK-738: was SetActorTickEnabled(false). StopAttackLunge runs on EVERY attack exit —
	// EnterIdle, EnterAdvance, FreezeAI, ApplyFreeze, HandleDeath — and a climb begun on a unit
	// that was mid-swing would have had its per-frame driver switched off underneath it, leaving
	// it in MOVE_Flying with no way to arrive and no way to fall. The OR is the whole fix.
	RefreshActorTickEnabled();
}

void ASummonedUnit::UpdateLunge(float DeltaSeconds)
{
	if (!bLungeActive)
	{
		// stray tick with no cycle running — go back to sleep, ⛔ but ONLY if the climb driver
		// is not the reason we are ticking (TASK-738). A bare SetActorTickEnabled(false) here
		// would silently kill an in-flight climb's driver and hang the unit in MOVE_Flying.
		RefreshActorTickEnabled();
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

void ASummonedUnit::RefreshActorTickEnabled()
{
	// ⛔ THE ONLY WRITER OF THIS ACTOR'S TICK FLAG (TASK-738). Two drivers share it and neither
	// may switch it off on the other's behalf — see the header comment for the exact regression.
	// ⭐ TASK-760: the DECISION moved to the pure statics so a headless test can drive its truth
	// table (this actor cannot be instantiated in one). ⛔ The expression is UNCHANGED — it is
	// still exactly `bLungeActive || LadderClimb.bActive`, in that order, with no new term.
	//
	// ⭐ TASK-784 CONSIDERED A THIRD TERM HERE AND DID ⛔ NOT ADD ONE. The contact trigger needs a
	// poll that runs while a unit is merely WALKING, and this predicate is false in exactly that
	// state — so arming the tick from it would have meant re-introducing a fleet-wide tick that
	// TASK-738/760 deliberately removed. The poll rides StateTimerHandle instead, which is the
	// same independent driver TASK-760's self-heal rides and for the same reason.
	SetActorTickEnabled(FSiegeLadderClimbStatics::WantsActorTick(bLungeActive, LadderClimb.bActive));
}

bool ASummonedUnit::TryContactClimbAtNearestLadder(float DeltaSeconds)
{
	// ── ⛔ THE FIND, AND IT IS ⛔ NOT A SCAN OF ALL ACTORS ──────────────────────────────────────
	// ⚠️⚠️ DELIBERATELY ⛔ NOT THE TActorIterator IDIOM THIS FILE USES FIVE TIMES (AcquireTarget,
	// FindNearestEnemyCastle, FindOwnCastle, FindNearestEnemyBuilding, AcquireEnemyNearPoint), and
	// the departure is MEASURED rather than stylistic: TActorIterator walks EVERY actor of EVERY
	// level and class-tests each one, so at 120 units × 4 polls/s it would add well over a million
	// class tests per second for a card that is usually not even in play. ForEachObjectOfClass is a
	// hash-bucket lookup — UObjectHash.cpp:1885-1887 does one TMap::Find per class and then iterates
	// ONLY that class's own instance list — so the cost here is O(live AClimbableTowers), which is
	// ZERO in every match where nobody played the card.
	//
	// ⛔⛔ COLLECT FIRST, ACT SECOND, AND THAT IS AN ENGINE REQUIREMENT ⛔ NOT A PREFERENCE:
	// UObjectHash.h:243 — "the operation must not modify UObject hash maps so it can not create,
	// rename or destroy UObjects" — and the callback runs under FHashTableLock. TryBeginContactClimb
	// reaches BeginLadderClimb, SetMovementMode and a BP-observable movement-mode change, ⛔ none of
	// which may run inside that lock. TInlineAllocator<4> keeps the collection allocation-free for
	// any plausible number of towers.
	const UWorld* const World = GetWorld();
	if (!World)
	{
		return false;
	}

	TArray<AClimbableTower*, TInlineAllocator<4>> Ladders;
	ForEachObjectOfClass(AClimbableTower::StaticClass(),
		[&Ladders, World](UObject* Object)
		{
			AClimbableTower* const Tower = Cast<AClimbableTower>(Object);

			// ⚠️ THE WORLD FILTER IS MANDATORY, ⛔ not defensive padding: the class hash spans EVERY
			// loaded world, so a level-placed tower sitting in the EDITOR world would otherwise be
			// handed to a PIE unit that can never reach it.
			if (IsValid(Tower) && !Tower->IsActorBeingDestroyed() && Tower->GetWorld() == World)
			{
				Ladders.Add(Tower);
			}
		});

	if (Ladders.Num() == 0)
	{
		// ⭐ THE OVERWHELMINGLY COMMON PATH, AND THE WHOLE COST ARGUMENT: no WatchTower has been
		// played, so this poll ends here having done one hash lookup that found an empty bucket.
		return false;
	}

	// ── WHICH LADDER TO ASK — THE NEAREST, BY THE SAME 2D METRIC THE TOWER'S OWN PROXIMITY TERM
	//    USES, AGAINST THE NEARER OF ITS TWO ENDPOINTS ──────────────────────────────────────────
	// ⛔ ONE tower is asked per poll, ⛔ never all of them: asking several would have this unit
	// feeding a dwell into several contact tables at once and the winner would be decided by
	// iteration order rather than by proximity.
	// ⛔⛔ AND THIS IS THE ONLY DECISION THIS FUNCTION MAKES. There is ⛔ no team test, ⛔ no
	// eligibility test and ⛔ no proximity THRESHOLD here — a pawn-side copy of the team gate would
	// be the silent back door around Jonathan's T-3 ruling that CONTACT-§4.3 exists to forbid, and
	// a pawn-side radius would be a fourth tuning number that could silently become tighter than
	// LadderContactRadiusUU and delete the feature with every tower test still green. ⇒ ⛔ NOT ONE
	// FLOAT IS AUTHORED IN THIS FUNCTION; the nearest tower is asked unconditionally and the tower
	// answers TooFar if it is not near enough.
	AClimbableTower* Nearest = nullptr;
	double NearestDistSq = 0.0;
	const FVector Here = GetActorLocation();

	for (AClimbableTower* const Tower : Ladders)
	{
		// ⭐ THE LINK IS THE ONE OBJECT HOLDING THE ARMED LINE — the same source
		// AClimbableTower::TryBeginContactClimb reads its own endpoints back from, so the choice
		// and the test can never disagree about where the ladder is. ⛔ Not the sockets again,
		// ⛔ not the actor origin, and ⛔ not a cached copy: TASK-783 is moving the ladder mesh right
		// now, and reading the link at runtime is what makes this code indifferent to that.
		const UClimbableTowerLadderLink* const Link = Tower->GetLadderLink();
		if (!Link)
		{
			continue; // a CDO-shaped tower; unreachable on a constructed one
		}

		// Both endpoints, because K-C arms the trigger at BOTH: a unit standing ON THE DECK is at
		// the top and walking into the ladder descends.
		const double DistSq = FMath::Min(
			FVector::DistSquared2D(Here, Link->GetStartPoint()),
			FVector::DistSquared2D(Here, Link->GetEndPoint()));

		if (!Nearest || DistSq < NearestDistSq)
		{
			Nearest = Tower;
			NearestDistSq = DistSq;
		}
	}

	if (!Nearest)
	{
		return false;
	}

	// ── ⭐⭐ THE ASK — **THE PAWN ONLY ASKS** ──────────────────────────────────────────────────
	// ⛔ THE VERDICT IS NOT BRANCHED ON BEYOND "did a climb start", AND THE ABSENCE OF THE REST OF
	// THAT BRANCH IS THE FEATURE. Every verdict has already been fully acted on by the time it
	// gets back here:
	//   • Climb    — the traversal is ALREADY RUNNING (the tower called BeginLadderClimb, which
	//                stopped path following itself) and the tower ALREADY holds its occupancy slot.
	//   • Declined — the tower ALREADY undid its own binding; ⛔ there is no climb to tear down and
	//                ⛔ no ninth exit for this task to invent.
	//   • TooFar / NotHeadingIn / Dwelling / Disarmed / WrongTeam / LadderBusy / NotASummonedUnit —
	//                refusals that changed nothing but that tower's own dwell bookkeeping.
	// ⇒ a richer pawn-side reaction could only ever be a SECOND copy of a decision the tower has
	// already made (WR-§5).
	//
	// ⛔ NO LOG LINE HERE — the tower logs the two EVENTS (an admission, and a declined Begin) and
	// nothing else, deliberately: this is a per-unit poll and a line per refusal would be four per
	// unit per second, forever.
	//
	// ⚠️ DeltaSeconds is the caller's world-clock delta, ⛔ never StateCheckInterval, and it is the
	// value the tower integrates its dwell out of. Its ~0.25 s granularity is the DECLARED cost of
	// riding this driver — measured in full in Tests/SiegeLadderClimbTest.cpp tests 15 and 16, and
	// ⛔ not fixable from here: the actor tick, the only finer driver available, is OFF in exactly
	// the state a contact climb starts from.
	return Nearest->TryBeginContactClimb(this, DeltaSeconds)
		== AClimbableTower::ELadderContactVerdict::Climb;
}

bool ASummonedUnit::IsClimbing() const
{
	// ONE bool backs the climb AND the TOWER-§9 disarm, which is what makes "the disarm ends the
	// INSTANT the unit reaches the deck" true by construction rather than by discipline: there is
	// no second flag anybody could forget to clear, and no timer that could outlive the cause.
	return LadderClimb.bActive;
}

bool ASummonedUnit::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)
{
	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	if (!Movement)
	{
		// ⛔ Refuse rather than arm: without a movement component there is no MOVE_Flying to
		// enter and no mode to restore, so an "active" climb here would be a unit that never
		// moves and never ends. Unreachable for a spawned ACharacter; guarded because the cost
		// of the alternative is the hang this whole feature is shaped around.
		return false;
	}

	// ⛔ CHANGES NOTHING ON REFUSAL — the pinned contract's guarantee, enforced inside the pure
	// half so it cannot be half-kept: every write below happens only after Begin returned true.
	// ⭐ THE CAPSULE HALF-HEIGHT COMES FROM THE CAPSULE, ⛔ NEVER FROM A LITERAL — the TOWER-§7
	// principle ("geometry a code path needs comes FROM the thing, not from a duplicated number")
	// applied a third time. It is what lifts the surface-space sockets into capsule-centre space,
	// and a BP child that resized its capsule stays correct for free.
	const float CapsuleHalfHeightUU = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		: SiegeSpawn::DefaultCapsuleHalfHeight;

	if (!FSiegeLadderClimbStatics::Begin(LadderClimb, bDead, bAIFrozen, bSpellFrozen,
		FromWorld, ToWorld, LadderClimbSpeedUU, CapsuleHalfHeightUU))
	{
		return false;
	}

	// ══ FROM HERE ON IsClimbing() IS TRUE — THE TOWER-§9 DISARM IS LIVE ══════════════════════
	// Jonathan, verbatim: "they should be attackable while climbing, but they can't attack back."
	//
	// ⭐ AND THE "ATTACKABLE" HALF IS FREE AND GETS ⛔ NO CODE, DELIBERATELY (TOWER-§9.2): a
	// climber is an ordinary live ASummonedUnit at an ordinary world location. The acquisition
	// gate is `Candidate != this && IsTargetAlive(Candidate) && enemy team && inside a 2D disc`
	// (AcquireTarget / AcquireEnemyNearPoint; TASK-828 moved the "enemy team" term into
	// FSiegeCombatStatics::GatherHostileAgents without changing it) — it reads no movement mode, no Z, and no unit
	// state but IsUnitDead(). Nothing below narrows it, and nothing below is allowed to.

	// (1) THE CADENCE IS THE ONLY THING THAT CAN STILL LAND A HIT, so it stops here. This is the
	//     STRUCTURAL half of the disarm (the ApplyFreeze idiom); the three CanEverAttack() guard
	//     points are the seal that also covers any future caller.
	GetWorldTimerManager().ClearTimer(AttackTimerHandle);

	// (2) A mid-swing lunge must not ride up the ladder — exact rest pose (TASK-020 zero drift).
	StopAttackLunge();

	// (3) ⛔ HEALING IS NOT ATTACKING AND IS DELIBERATELY UNTOUCHED. A climbing Cleric still
	//     mends, exactly as a FOLLOWING Cleric still mends (UpdateStateFollow's shipped ruling).
	//     Stopping it would be extending his ruling into something he did not ask for, which
	//     TOWER-§9 forbids in both directions. DECLARED for QA rather than decided silently.

	// (4) ONE STEERING AUTHORITY. Path following must let go before we drive the capsule, or two
	//     drivers write the same input vector (the NAV-§3 no-double-driver law).
	if (AAIController* const AI = GetAIController())
	{
		AI->StopMovement();
	}

	// (5) ⚠️ THE SIXTH SIDESTEP-LEASE CLEAR SITE — DECLARED, ⛔ NOT BURIED (the ApplyFreeze
	//     "declared fifth exit" precedent, NAV-§3). It belongs for that exact reason: UpdateState
	//     early-outs for the whole climb, so TickStuckWatchdog never runs and the lease can never
	//     DRAIN — a unit that landed would resume with a live lease and steer to a SidestepGoal
	//     chosen before it left the ground. Reset also drops the stall anchor, so the ascent is
	//     never charged to the unit as stall time.
	SidestepLeaseRemaining = 0.f;
	FSiegeStuckStatics::Reset(StuckState);

	// (6) The walk is over; park the machine the way ApplyFreeze does (State written directly —
	//     ⛔ NOT EnterIdle(), whose `State == Idle` early-out would skip the CurrentMoveGoal clear
	//     on an already-idle unit).
	//     ⛔⛔ CurrentTarget IS DELIBERATELY **KEPT** — and this is the one place ApplyFreeze's
	//     pattern is NOT copied. TOWER-§9.2 rules that the disarm ends the INSTANT the deck is
	//     reached; dropping the target here would force a re-acquire on landing and cost the unit
	//     up to one 0.25 s state poll of enforced silence. That is a lingering penalty by another
	//     name, and he ruled against exactly that.
	State = ESummonedUnitState::Idle;
	CurrentMoveGoal = nullptr;

	// (7) ⭐ MOVE_Flying — THE ONE SHIPPED MODE THAT ACCEPTS VERTICAL MOTION WITHOUT NEW PHYSICS
	//     CODE, and the licence is MEASURED (TOWER-§8.5): ConstrainInputAcceleration
	//     plane-projects steering input ONLY when IsMovingOnGround() || IsFalling()
	//     (CharacterMovementComponent.cpp:8121-8131). Flying is neither, so the vertical
	//     component of our steer SURVIVES — and flying also disables gravity, which is exactly
	//     what a climb wants. MaxFlySpeed caps the ascent at the shipped rate; it is saved and
	//     restored exactly, and nothing else in this project writes it.
	LadderClimbSavedMaxFlySpeed = Movement->MaxFlySpeed;
	Movement->MaxFlySpeed = FMath::Max(LadderClimbSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU);
	Movement->StopMovementImmediately(); // no inherited walk velocity carried into the ascent
	Movement->SetMovementMode(MOVE_Flying);

	// ⭐⭐ TASK-803 INSTRUMENTATION (1 of 2) — **THE ENTRY CROSS-TRACK OFFSET**, the number
	// `TASK-805`'s PIE row exists to read and which ⛔ nothing in this project logged before.
	// ⭐ The unit's twin of `AHeroCharacter::BeginLadderClimb`'s block — the same measurement, kept
	// on BOTH pawns because `CONTACT-§14`'s merged defect admits BOTH from the same 350 uu disc.
	//
	// ⛔⛔ IT IS **ONE** SAMPLE, AND THE BOARD'S ORIGINAL ROW ("Y at 3–4 points up the line") IS
	// ⛔ WITHDRAWN RATHER THAN SHIPPED SHORT: `CONTACT-§14.1` proves — five ways — that
	// `Y(top) == Y(entry)` EXACTLY, so four samples would return the identical number four times.
	// The OTHER end of the signal is the arrival-pop line in `TickLadderClimb` below.
	//
	// ⭐ MEASURED AS A SIGNED PERPENDICULAR DISTANCE FROM THE **CLIMB LINE**, ⛔ not as a raw world
	// Y (the castles ship ROTATED, so world Y is not tower-local Y in general; the line's own
	// horizontal normal is correct in every frame). For the shipped watchtower the two coincide.
	// ⛔ LOG-ONLY: every local below is read by the log line and by ⛔ nothing else — deleting this
	// whole block changes ⛔ no behaviour. ⛔ Verbose, ⛔ never Warning and ⛔ never on-screen.
	// ⭐ THE FREE CONTACT-vs-LINK DISCRIMINATOR: `AClimbableTower::TryBeginContactClimb` logs
	// `"CONTACT climb started"` immediately after this function returns true and the ordered nav-LINK
	// path logs ⛔ nothing on start ⇒ this line WITH that follow-up is contact, WITHOUT it is the
	// link. ⛔ No new log is needed on either, and none is added.
	{
		const FVector EntryWorld = GetActorLocation();
		const FVector ClimbAlong = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);
		const FVector CrossAxis = FVector::CrossProduct(FVector::UpVector, ClimbAlong).GetSafeNormal();

		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASummonedUnit '%s': ladder climb ENTRY OFFSET — cross-track %+.2f uu (signed, across the ladder's clear opening: 0 = dead centre, the stiles are at ±66.0, this capsule's side margin is 32.0). Entry %s, line start %s. ⭐ CONTACT-§14.2 admits up to ±277.8 uu here; TASK-803's steer is what nulls it — see the ARRIVAL POP line for whether it did."),
			*GetNameSafe(this),
			FVector::DotProduct(EntryWorld - LadderClimb.Start, CrossAxis),
			*EntryWorld.ToString(), *LadderClimb.Start.ToString());
	}

	RefreshActorTickEnabled();
	return true;
}

void ASummonedUnit::AbortLadderClimb()
{
	// EXIT 2 of 8 — and EXIT 8 as well: AClimbableTower::EndPlay calls this on every climber it
	// started when the tower dies mid-climb (TOWER-§10 L-5). Under the ramp that case was FREE —
	// the floor vanished and CharacterMovement dropped to MOVE_Falling by itself. A MOVE_Flying
	// climber will ⛔ NOT fall, so the abort is REQUIRED there. Idempotent: the teardown's latch
	// makes a call on a non-climbing unit a silent no-op.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::Abort);
}

void ASummonedUnit::EndLadderClimb(bool bReachedTop, ESiegeLadderExit Reason)
{
	// ⭐⭐ THE EXACTLY-ONCE LATCH IS CONSUMED FIRST, BEFORE ANY EFFECT. That ordering is what
	// makes a double exit inert (HandleDeath then EndPlay is the ordinary case, ⛔ not an edge
	// one) and what makes a listener that calls AbortLadderClimb from inside the broadcast below
	// return immediately instead of recursing.
	if (!FSiegeLadderClimbStatics::End(LadderClimb))
	{
		return;
	}

	// ⛔⛔ THE RESTORE. THIS IS THE LINE THE WHOLE FEATURE TURNS ON: MOVE_Flying ignores gravity,
	// so an exit that skips it leaves a unit hanging in mid-air forever. SetDefaultMovementMode
	// is the project's shipped idiom for exactly this (EndSpellFreeze uses it), and it does the
	// right thing in mid-air: with no movement base it goes straight to MOVE_Falling rather than
	// spending a frame pretending to walk (CharacterMovementComponent.cpp:1326-1338). ⇒ the unit
	// DROPS from wherever it is and survives — there is ⛔ no fall damage in Siegebound
	// (TOWER-§4a, measured). ⚠️ TOWER-§4a's declared residual (landing in a footprint whose
	// navmesh has not regenerated) stands unchanged and unsolved by design — ⛔ no nav-projecting
	// teleport is added here.
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->MaxFlySpeed = LadderClimbSavedMaxFlySpeed; // EXACT restore, zero residual
		Movement->SetDefaultMovementMode();
	}
	LadderClimbSavedMaxFlySpeed = 0.f;

	// The climb no longer wants the tick; the lunge might.
	RefreshActorTickEnabled();

	if (Reason == ESiegeLadderExit::Timeout)
	{
		// ⚠️ THE WATCHDOG IS THE ONE EXIT THAT MEANS SOMETHING IS WRONG — every other reason is
		// ordinary gameplay, so this is the only one that logs. A blocked sweep (geometry between
		// the ladder and the deck) is the expected cause, and the unit has just been dropped
		// rather than left hanging, so this is a diagnosis rather than a failure.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASummonedUnit '%s': ladder climb ABANDONED by the watchdog — it did not reach the top inside its budget. The unit has been dropped (movement mode restored), not stranded. Check for geometry blocking the climb line, or a LadderTop socket the capsule cannot reach."),
			*GetNameSafe(this));
	}

	// ⚠️ BROADCAST LAST — after the mode is restored, so AClimbableTower's listener (which hands
	// the unit back to path following via FinishUsingCustomLink) never observes a half-torn-down
	// unit. This delegate is the tower's ONLY completion signal, which is what keeps that class
	// tickless and timerless.
	OnLadderClimbEnded.Broadcast(this, bReachedTop);
}

void ASummonedUnit::TickLadderClimb(float DeltaSeconds)
{
	if (!LadderClimb.bActive)
	{
		return;
	}

	const FVector Here = GetActorLocation();

	bool bReachedTop = false;
	bool bTimedOut = false;
	if (!FSiegeLadderClimbStatics::Advance(LadderClimb, Here, DeltaSeconds, bReachedTop, bTimedOut))
	{
		// ⭐ LAND EXACTLY ON THE DECK, ⛔ NOT WHEREVER THIS FRAME'S STEP HAPPENED TO STOP.
		// ArrivalTarget is the destination surface plus one capsule half-height, so the unit
		// finishes STANDING ON the deck. Non-swept for the same reason the last stretch was: the
		// target is on the far side of the slab. ⛔ Only on a real arrival — a timed-out climb
		// must drop from where it actually is, never be handed the deck it failed to reach.
		if (bReachedTop)
		{
			// ⭐⭐ TASK-803 INSTRUMENTATION (2 of 2) — **THE ARRIVAL POP, MEASURED ⛔ BEFORE IT
			// HAPPENS.** `ArrivalTarget` is `State.End`, a point ⛔ ON the climb line, and this snap
			// is ⛔ UNSWEPT ⇒ before TASK-803 a unit admitted off-line paid its ENTIRE accumulated
			// cross-track offset — up to ~247 uu — LATERALLY, in ONE FRAME (`CONTACT-§14.3`).
			// ⭐ WHAT TASK-805 READS THIS FOR: with the cross-track term live the HORIZONTAL figure
			// should be ~1 uu or less; tens or hundreds means the steer did ⛔ not converge.
			// ⛔ LOG-ONLY: `ArrivalPop` is read by the log line and by nothing else, and
			// `ArrivalTarget` is a pure function called ONCE here instead of twice — ⛔ no behaviour
			// change, ⛔ Verbose, ⛔ never Warning and ⛔ never on-screen.
			const FVector ArrivalWorld = FSiegeLadderClimbStatics::ArrivalTarget(LadderClimb);
			const FVector ArrivalPop = ArrivalWorld - Here;

			UE_LOG(LogGitClaudeUnrealTest, Verbose,
				TEXT("ASummonedUnit '%s': ladder climb ARRIVAL POP — horizontal %.3f uu (this is the residual CROSS-TRACK error), |ΔY(world)| %.3f uu, total %.3f uu. From %s to %s, UNSWEPT. ⭐ CONTACT-§14.3: under ~1 uu means TASK-803's steer converged; tens or hundreds means it did not."),
				*GetNameSafe(this), ArrivalPop.Size2D(), FMath::Abs(ArrivalPop.Y), ArrivalPop.Size(),
				*Here.ToString(), *ArrivalWorld.ToString());

			SetActorLocation(ArrivalWorld, /*bSweep=*/ false);
		}

		// EXIT 1 of 8 (arrival) — or the declared watchdog. Arrival is the ONLY exit that reports
		// bReachedTop, and it is also where the TOWER-§9 disarm releases: the same End() that
		// clears bActive clears IsClimbing(), so the unit is armed again on this very frame.
		// ⛔ No decay timer, ⛔ no grace window, ⛔ no lingering penalty — the vulnerability was
		// the climb (TOWER-§9.2).
		EndLadderClimb(bReachedTop, bTimedOut ? ESiegeLadderExit::Timeout : ESiegeLadderExit::Arrival);
		return;
	}

	const FVector Direction = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);

	if (FSiegeLadderClimbStatics::ShouldSweep(LadderClimb, Here))
	{
		// ── THE ORDINARY ~78% OF THE LINE: SWEPT MOVEMENT THROUGH THE MOVEMENT COMPONENT ──────
		// The capsule, the sweep and depenetration all still apply. ⛔ NOT a SetActorLocation /
		// TeleportTo lerp of the traversal: that would drag the capsule through the tower body
		// and through other units, and it is the mechanism NAV-§ refuses on principle
		// (TOWER-§8.5). The direction is the ONE straight segment, so the path cannot desync from
		// itself; MaxFlySpeed caps the rate.
		//
		// ⚠️ bForce = true, AND IT IS DELIBERATE — ⛔ not copied from path following, which passes
		// false. APawn::Internal_AddMovementInput drops the vector whenever IsMoveInputIgnored(),
		// and that returns TRUE for a pawn with no controller at all — a real window here, because
		// AutoPossessAI possession can land after BeginPlay (the miner's controller poll exists for
		// exactly that reason). A scripted traversal whose completion the tower is WAITING ON must
		// not be silently suppressible: the unit would float until the watchdog dropped it.
		//
		// ⭐⭐ TASK-803 (`CONTACT-§14`) — **THE CROSS-TRACK TERM, AND IT IS SWAPPED IN HERE AND AT
		// EXACTLY ONE OTHER PLACE IN THE PROJECT** (`AHeroCharacter::TickLadderClimb`'s twin of this
		// line). `SteerDirection` aims at a look-ahead point ON the climb line, so a unit the
		// contact trigger admitted from anywhere in its **350 uu 2D disc** CONVERGES onto the line
		// as it climbs instead of riding its entry offset to the deck and paying the whole thing in
		// one unswept frame at the arrival snap (`§14.2`/`§14.3`). The unit's side margin is 32.0 uu
		// (r 34 against a 132.0 uu clear opening) and the admitted band reaches ±277.8.
		// ⛔⛔ `Direction` ABOVE IS **NOT** REASSIGNED AND **NOT** REDEFINED: it is still
		// `ClimbDirection`, and the DECK-BREACH step below still uses it, exactly as `§14.5`
		// requires. ⭐ A unit that entered dead centre gets a BYTE-IDENTICAL climb.
		AddMovementInput(FSiegeLadderClimbStatics::SteerDirection(LadderClimb, Here), 1.f, /*bForce=*/ true);
		return;
	}

	// ── ⚠️⚠️ THE DECK-BREACH WINDOW — A NON-SWEPT CONTINUOUS DRIVE, AND IT IS THE ONLY WAY THE
	//    FEATURE WORKS AT ALL (TASK-737's measurement; full reasoning at
	//    FSiegeLadderClimbStatics::DeckBreachCapsuleHalfHeights and ::ShouldSweep) ──────────────
	// `LadderTop` is pinned 150 uu INSIDE a solid deck slab, so the end of this line is inside
	// geometry. A swept move collides with the slab's underside and STALLS — silently, with every
	// one of the eight exits still perfectly correct and the unit simply stopping short forever.
	//
	// ⭐ SAME RATE, SAME LINE, JUST NO SWEEP: a CONTINUOUS drive, ⛔ not a teleport and ⛔ not a
	// lerp of the whole traversal. A unit that popped 270 uu up a ladder would read as broken —
	// which is the exact failure the climb clip (TASK-733) exists to prevent.
	//
	// ⚠️ Velocity is zeroed first, or the two drivers fight: PhysFlying would keep sweeping the
	// capsule from its residual velocity (BrakingDecelerationFlying is 0, so it never decays) and
	// re-jam it against the slab we are stepping through. With Velocity and Acceleration at zero,
	// PhysFlying moves nothing and this is the only thing touching the capsule.
	if (UCharacterMovementComponent* const Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	const float StepUU = FMath::Max(LadderClimbSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU)
		* FMath::Max(DeltaSeconds, 0.f);
	SetActorLocation(Here + Direction * StepUU, /*bSweep=*/ false);
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
	//
	// ⭐⭐⭐ TASK-931 (WITCH-§2, WITCH-§3, 🧑 J-W18) — ⛔ ONE PER-VIEWER CONSULT, ⛔ TWO TELLS.
	// ⛔⛔ WHY THE CONSULT IS HERE AND ⛔ NOT INSIDE EITHER FEEDBACK CALL, AND IT IS A MEASUREMENT
	// RATHER THAN A PREFERENCE: TriggerFlash and ShowDamageNumber each have ⛔ FOUR callers —
	// ABuilding, ACastle, AHeroCharacter and this one — and WITCH-§6 rules ASummonedUnit the ⛔ ONLY
	// veilable class. ⇒ a consult written ⛔ inside either helper would ⛔ silently reach buildings,
	// castles and the hero, i.e. it would suppress feedback for three actor kinds that ⛔ cannot be
	// veiled and whose viewers therefore have nothing to be protected from. ⛔ The veilable SITE is
	// the only place the question is even well-posed, and putting it here leaves both helpers
	// exactly as their other three callers found them.
	// ⭐ ONE `if` FOR BOTH TELLS, ⛔ NOT TWO ONE-LINE HIDES. They fire on the same edge, in the same
	// block, on the same condition; two independent guards would be two things to keep in agreement
	// and the second one is the one somebody forgets when the predicate moves.
	//
	// ⛔⛔ THIS IS ⛔ NOT A VEIL BREAK AND ⛔ NOT A DAMAGE CHANGE. `ActualDamage` was already applied
	// above, `CurrentHP` is already reduced, `OnHPChanged` has already fired and every consumer
	// downstream is untouched: a veiled unit still ⛔ takes the hit, still ⛔ dies from it and still
	// ⛔ stays veiled (WITCH-§3 — being hit is not acting). ⛔ Only the two COSMETICS a viewer who
	// cannot see the unit has no business receiving are withheld.
	// ⚖️ AND THEY ARE THE WORST TWO IN THE GAME: the AoE lane is the ⛔ one lane WITCH-§2 deliberately
	// leaves un-suppressed (J-W2), so this fires ⛔ exactly when a hidden push is being flushed — the
	// moment the veil is doing its most important work — and the damage number is worse than the
	// flash, because it outlasts it, it is UI-bright, its tint leaks ⛔ WHOSE unit it is and its
	// digits leak ⛔ HOW MUCH HP IT JUST LOST.
	// ⭐ The OWNER keeps both: the predicate answers TRUE for every same-team query (WITCH-§2 lane
	// 4), so his own veiled unit still flashes and still floats its numbers.
	if (FSiegeCombatStatics::IsAgentVisibleToLocalViewer(GetWorld(), this))
	{
		if (HitFlashComponent)
		{
			HitFlashComponent->TriggerFlash();
		}
		USiegeFeedbackLibrary::ShowDamageNumber(this, ActualDamage,
			GetActorLocation() + FVector(0.f, 0.f, UnitDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));
	}

	// ══ ⭐⭐ THE WITCH-CAST INTERRUPT (TASK-830; WITCH-§4) — ⛔ AND IT IS ⛔ NOT A VEIL BREAK ═════
	// ⛔⛔ READ THE DISTINCTION BEFORE EDITING EITHER LINE, BECAUSE WITCH-§3 STATES IT AND THEN
	// WARNS, IN WRITING, NOT TO MERGE THEM: taking damage ⛔ INTERRUPTS AN IN-PROGRESS CAST; it
	// ⛔ DOES NOT BREAK AN EXISTING VEIL. His rule breaks the veil on ⛔ ACTING, and being hit is
	// not acting. ⇒ ⛔ there is deliberately ⛔ no BreakInvisibility call in this function, and a
	// "for symmetry" one added here would make ⛔ every veiled unit clipped by a stray AoE it
	// cannot even see coming ⛔ permanently visible — which is not what he wrote.
	//
	// ⭐ TWO CALLS, ⛔ NEITHER REDUNDANT, because his sentence names ⛔ BOTH actors: "can be
	// interupted if ⛔ THE WITCH ⛔ OR ⛔ UNIT THAT IS TURNING INVISIBLE are attacked". The first
	// kills the cast ⛔ THIS unit is performing (we are the witch); the second kills the cast being
	// performed ⛔ ON this unit by someone else (we are the subject). ⛔ Different objects, and no
	// single call can cover both. Both are no-ops for every unit that is neither.
	//
	// ⭐ PLACED AFTER the friendly-fire refusal and the `ActualDamage <= 0` early-out above, so a
	// blocked or zero hit ⛔ cannot cancel a cast; and ⛔ BEFORE HandleDeath below, so a killing
	// blow interrupts before the death path runs (HandleDeath cancels too — idempotent).
	CancelWitchCast(TEXT("the witch was attacked"));
	InterruptIncomingWitchCast(TEXT("the subject being veiled was attacked"));

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
	//
	// ══ VEIL BREAK — `Attack`, THE SAPPER (TASK-829; ⛔ WITCH-§3a TRAP 1 of 3) ═════════════════
	// ⛔⛔⛔ THIS IS THE SITE THE OBVIOUS WIRING MISSES, AND MISSING IT ⛔ IS THE BUG REPORT
	// "invisibility is broken". `ApplyDetonation` is ⛔ NOT reached through `PerformAttack` — it
	// has its ⛔ OWN two entries: contact, via `Detonate()` from `UpdateStateSiege` (guarded by
	// bSuicide), and death, via `HandleDeath()`. ⇒ a break placed ⛔ only in PerformAttack lets a
	// ⛔ VEILED SAPPER BREACH A BUILDING ⛔ WHILE STILL INVISIBLE.
	// ⭐ THE BREAK LIVES ⛔ HERE — at the blast, inside the one function ⛔ both entries share —
	// rather than at either entry, so a ⛔ third entry added later inherits it for free.
	// ⚠️ ORDER, AND IT IS DELIBERATE: on the death path HandleDeath calls this FIRST, so a veiled
	// Sapper's log reads `Attack` (its blast is what revealed it) and the later `Death` break is
	// an idempotent no-op. ⭐ That is the truthful reason, and it is why ApplyBreak is monotone.
	// ⚠️ ⛔ THE BLAST ITSELF IS ⛔ EXEMPT FROM VEIL SUPPRESSION IN THE ⛔ OTHER DIRECTION
	// (WITCH-§2 / J-W2): ApplyRadialDamage passes ESiegeVeilPolicy::IncludeVeiled, so this blast
	// still catches ⛔ VEILED VICTIMS. ⭐ Two different rules meeting on one line — the Sapper
	// reveals ITSELF by acting; its victims are ⛔ acted upon and are caught without being seen.
	BreakInvisibility(ESiegeVeilBreakReason::Attack);

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

	// ══ VEIL BREAK — `Death` (TASK-829; WITCH-§3 ruling J-W4) ════════════════════════════════
	// ⭐ The veil is ⛔ CLEARED, not "broken": ⛔ NO CORPSE IS INVISIBLE. A rigged unit's body is
	// held on screen for up to DeathAnimMaxHoldSeconds below, so a veil that survived death would
	// be a visible-to-nobody corpse lying in the field for two seconds.
	// ⛔ IT IS AN ENUMERATOR RATHER THAN A BARE ASSIGNMENT FOR EXACTLY ONE REASON (WITCH-§6): an
	// inlined `bIsInvisible = false` anywhere is an automatic QA FAIL. Death clears the flag
	// through the ⛔ SAME one door as every other reason, so a grep for BreakInvisibility is the
	// complete list of ways a unit loses its veil.
	// ⚠️ PLACED ⛔ AFTER the bSuicide ApplyDetonation above, ⛔ not before: a veiled Sapper that
	// blows up on death should log `Attack`, because its blast is what revealed it. This call is
	// then an idempotent no-op — ApplyBreak is monotone and reports no second edge.
	BreakInvisibility(ESiegeVeilBreakReason::Death);

	// ══ THE WITCH-CAST CHANNEL DIES WITH THE ACTOR (TASK-830; WITCH-§4) ══════════════════════
	// Both ends, for the same both-actors reason TakeDamage carries: a dying WITCH abandons her
	// cast, and a dying SUBJECT ends the cast being channelled onto it ("a cast also cancels if
	// the target dies"). ⛔ Weak pointers would make both self-heal on the next poll anyway — this
	// is the explicit teardown so the subject's back-pointer is released ⛔ now rather than up to
	// 0.25 s later, during which no other witch could have started on it.
	// ⛔ NOT a veil break: BreakInvisibility(Death) above already did that, once, through the one
	// door. These two touch the CAST channel only.
	CancelWitchCast(TEXT("the witch died"));
	InterruptIncomingWitchCast(TEXT("the subject being veiled died"));

	// ══ LADDER EXIT 4 of 8 — DEATH MID-CLIMB (TASK-738, TOWER-§8.5) ══════════════════════════
	// ⚠️ PLACED HERE, EARLY, AND THE ORDER IS LOAD-BEARING: the rigged-death path below calls
	// DisableMovement() (MOVE_None) so the corpse does not sink through its own disabled capsule
	// during the death-anim hold. This restore must therefore run BEFORE it, or the two writes
	// fight and the corpse's mode is whichever ran last. Running first, death's MOVE_None wins —
	// which is what we want, on a ladder or on the ground.
	// ⭐ It also releases the tower's occupant slot through OnLadderClimbEnded: TOWER-§10 L-1
	// records that whether a unit DYING mid-climb reaches UNavLinkCustomComponent's own
	// OnLinkMoveFinished is UNMEASURED, so this broadcast is the signal the tower can rely on.
	EndLadderClimb(/*bReachedTop=*/ false, ESiegeLadderExit::Death);

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
			// ⛔ HideBar(), ⛔ NEVER SetVisibility() — qa/TASK-801 B-1, and this is the ONE line of this
			// file TASK-791 owns. Since TASK-791 this component has a SECOND visibility writer (the
			// occlusion poll, running on the tick it was already required to run). HideBar() LATCHES the
			// owner's intent, which is the outer AND that poll composes with; a raw SetVisibility(false)
			// leaves the latch reading "shown", so the next poll ≈150 ms later recomputes visible and
			// puts a 0-HP bar back over the corpse for the whole death-anim hold. ⛔ Do not "simplify"
			// this back to SetVisibility — the two calls looked equivalent before TASK-791 and are not now.
			HPBarWidget->HideBar();
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
	// the castle's origin sits at the center of a 7313.7 x 7384.5 uu footprint and would
	// never come within Range/AggroRadius of a unit standing at its walls (⚠️ citation
	// corrected by TASK-574 — this line quoted the M1 castle's "~800x800"; the reasoning
	// is unchanged and only became more true). ECC_Pawn is blocked
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
