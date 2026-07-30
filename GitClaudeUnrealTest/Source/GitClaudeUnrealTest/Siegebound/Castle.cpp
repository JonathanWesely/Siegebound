// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Castle.h"

#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInterface.h"
#include "NavModifierComponent.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): DOREPLIFETIME registration
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "TimerManager.h"
#include "Siegebound/CastleHealthBarWidget.h"
#include "Siegebound/DamageTypes.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeNavAreas.h"

namespace
{
	//~ M7 §6 juice/feel soft-ref paths (TASK-157/158/179) — null-safe (art arrives in TASK-171-adjacent/174/180).
	const TCHAR* CastleHitSoundPath = TEXT("/Game/Audio/S_CastleHit");             // TASK-179
	const TCHAR* CastleDestroyedSoundPath = TEXT("/Game/Audio/S_CastleDestroyed"); // TASK-179
	const TCHAR* CastleDebrisVFXPath = TEXT("/Game/VFX/NS_CastleDebris");          // TASK-157 debris burst

	/** Height above the castle origin for its floating damage number (clears the ~2694-tall 3× mesh, HP-bar Z parity — re-derived ×3 with the bar by TASK-349). */
	constexpr float CastleDamageNumberHeightZ = 3150.f;
}

ACastle::ACastle()
{
	// Pure event-driven objective — nothing to tick.
	PrimaryActorTick.bCanEverTick = false;

	// M8 (TASK-356 doc §3.1): the level-placed castle replicates its core state
	// (HP / destroyed / crumble stage / Team belt) — the client's level instance
	// matches by name and receives updates; no dormancy tuning in P1 (event-
	// driven writes fit default frequencies, D13). Standalone: no connections ⇒
	// registered-but-never-sent, zero behavior change (doc §10).
	bReplicates = true;

	// ⚖️ NET RELEVANCY — TIER A (CONVENTIONS NET RELEVANCY LAW; TASK-356 loop-1
	// BLOCKER 2 fix). A castle is a match-critical near-singleton (exactly two
	// per match) whose HP/crumble/destroyed truth MUST NOT depend on where a
	// camera is: at the two-client gate the FAR castle sat 488 m from the client
	// — outside UE's default 150 m distance relevancy — and read host 500 /
	// client 2000 while the near castle (12 m) was perfect. Always-relevant is
	// the only correct tier for the actor the WIN CONDITION runs on. Bandwidth
	// is negligible: 2 actors, event-driven writes only (damage/crumble/reset).
	bAlwaysRelevant = true;

	CastleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CastleMesh"));
	SetRootComponent(CastleMesh);
	// Explicit blocking profile (QA TASK-002 WARN, pre-approved fix): TASK-004's unit
	// targeting/blocking contract must not rest on the engine's implicit default.
	CastleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Overhead HP bar (playtest R1 finding 2, TASK-018). Screen space so it reads at
	// any camera angle/distance; relative Z +3150 clears the ~2694-tall 3× castle
	// (TASK-349 re-derivation ×3 of the original 1050-over-900 pair; TASK-350
	// verifies the read at the gameplay camera). The widget CLASS is soft-resolved
	// at BeginPlay (WBP_CastleHealthBar, TASK-019); the bare component always
	// exists and draws nothing.
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(CastleMesh);
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidget->SetDrawSize(FVector2D(256.0f, 32.0f));
	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 3150.0f));
	// UI-only component: never collides, never blocks traces (placement cursor
	// trace TASK-007, unit acquisition TASK-004).
	HPBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// TASK-349 team gating (CONVENTIONS "Castle 3× HOLLOW"), BOTH lanes as ACTOR
	// components so the crumble mesh swap (ApplyCrumbleStage) can never strip them.
	// Constructor: create + make inert. ALL live configuration (size, position,
	// object type, responses, nav area) happens in ConfigureTeamGating at BeginPlay,
	// when Team is authoritative — so an editor-placed castle blocks nothing and
	// marks nothing until play.
	GateBlockerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GateBlockerVolume"));
	GateBlockerVolume->SetupAttachment(CastleMesh);
	// Inert until BeginPlay; the physical lane must NEVER touch navigation (the
	// nav lane is InteriorNavModifier's) and never raises overlap events.
	GateBlockerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateBlockerVolume->SetGenerateOverlapEvents(false);
	GateBlockerVolume->SetCanEverAffectNavigation(false);

	// Nav lane: ctor default = the BLUE interior area (Team defaults Blue), NEVER
	// UNavArea_Null — these areas are normal-cost walkable, so navmesh GENERATION
	// is untouched in every state; only the enemy's query filter excludes them.
	// PostInitializeComponents selects the actual Team's area BEFORE the first
	// generation pass (loop-2 Leg 2, the fresh-build lane); BeginPlay re-asserts
	// (free no-op) then unconditionally refreshes the octree entry (loop-4 B4,
	// the pre-built/saved-tile lane — see ConfigureTeamGating).
	InteriorNavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("InteriorNavModifier"));
	InteriorNavModifier->AreaClass = UNavArea_BlueCastleInterior::StaticClass();
	// Loop-2 B3: give the modifier its OWN nav-octree element (the
	// NavModifierVolume shape) instead of the default attach-to-owner's-root —
	// riding the CastleMesh GEOMETRY element routed our per-hull area list through
	// the engine's raw-geometry GetCollisionAreaClass (the `Areas.Num() <= 1`
	// ensure at RecastNavMeshGenerator.cpp:305, which then honors ONLY Areas[0])
	// and coupled area marking to every mesh-swap/collision-toggle rebuild of
	// that element — the Play-Again enemy-open window's mechanism. Decoupled, the
	// areas apply through dynamic-area marking (multi-area-correct) and the
	// element survives geometry churn. Ctor-safe: pre-registration the internal
	// RefreshNavigationModifiers is a guarded no-op (bRegistered false); the flag
	// lands before OnRegister ever caches a nav parent, so the very first
	// registration is already decoupled.
	InteriorNavModifier->ForceNavigationRelevancy(true);

	// §6 hit-flash (TASK-154): overlay-based white flash on every actual damage event,
	// driven from TakeDamage. Overlay (not slot-swap) so it composes cleanly with the
	// TASK-157 crumble MI swap on this same mesh. Null-safe.
	HitFlashComponent = CreateDefaultSubobject<USiegeHitFlashComponent>(TEXT("HitFlashComponent"));

	// §6 castle-hit screen shake donor (TASK-158): the READ-ONLY Variant_Combat
	// BP_CameraShake_Hit_Enemy (CONVENTIONS template-donor rule). Soft, null-safe; a BP
	// may retarget it to a dedicated BP_CameraShake_CastleHit.
	CastleHitCameraShake = TSoftClassPtr<UCameraShakeBase>(FSoftObjectPath(TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy.BP_CameraShake_Hit_Enemy_C")));

	// Content contract (TASKBOARD TASK-002 names block / CONVENTIONS.md). These assets are
	// produced in parallel (TASK-013 mesh, TASK-012 materials) and are resolved null-safe
	// in OnConstruction — a missing asset must never crash.
	CastleMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_Castle.SM_Castle")));
	TeamMaterialBlue = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	TeamMaterialRed = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));
	// TASK-018 names block: widget asset built in TASK-019 — resolved null-safe at BeginPlay.
	HPBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C")));
}

void ACastle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTeamVisuals();
}

void ACastle::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// TASK-349 loop-2 B3(a): the TEAM's interior area must be on the modifier
	// BEFORE the first navmesh generation pass (Dynamic Recast builds after world
	// init) — the serialized/deferred-set Team is authoritative here, so the Red
	// castle's interior tiles build Red-first-time instead of building Blue (the
	// ctor default) and rebuilding after a BeginPlay flip — that flip was the
	// measured 9.8–11.6 s initial stale window. Null-safe; ConfigureTeamGating at
	// BeginPlay re-asserts the same class (SetAreaClass early-outs on an
	// unchanged value — a free no-op).
	if (InteriorNavModifier)
	{
		InteriorNavModifier->SetAreaClass(SiegeTeamInteriorAreaClass(Team));
	}
}

void ACastle::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	bDestroyed = false;

	// Seed broadcast (TASK-018 spec): listeners that bound before BeginPlay
	// (level BP, game framework) start from the boot values without polling.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	InitHPBarWidget();

	// TASK-349: arm both team-gating lanes from the now-authoritative Team.
	ConfigureTeamGating();
}

void ACastle::ConfigureTeamGating()
{
	// CONVENTIONS "Castle 3× HOLLOW" team-gating law — symmetric by construction:
	// everything below derives from THIS castle's Team, so Castle_Blue and
	// Castle_Red configure mirror-image gates with no hardcoded team branches.
	const ECollisionChannel OwnChannel = SiegeTeamObjectChannel(Team);
	const ECollisionChannel EnemyChannel = SiegeEnemyTeamObjectChannel(Team);

	// PHYSICAL lane — the gate blocker. Object type = the OWN team channel:
	// deliberately NOT WorldStatic/WorldDynamic, so the projectile terrain-impact
	// OBJECT query (AProjectile, WorldStatic+WorldDynamic list) and every other
	// object-type query pass through the gate untouched (spells/projectiles
	// unaffected — body channels only). Response base = Ignore ALL (invisible to
	// cursor/camera/pawn-distance traces); the single Block on the enemy channel
	// is the whole gate: enemy capsules (stamped by ASummonedUnit/AHeroCharacter
	// at BeginPlay) block pairwise, own-team capsules pass on the Ignore.
	if (GateBlockerVolume)
	{
		GateBlockerVolume->SetBoxExtent(GateBlockerExtent);
		GateBlockerVolume->SetRelativeLocation(GateBlockerRelativeLocation);
		GateBlockerVolume->SetCollisionObjectType(OwnChannel);
		GateBlockerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		GateBlockerVolume->SetCollisionResponseToChannel(EnemyChannel, ECR_Block);
		// QueryAndPhysics AFTER the matrix is authored: CharacterMovement stops at
		// blocking geometry via query sweeps, so this is the moment the gate arms.
		// HandleDestroyed's SetActorEnableCollision(false) drops it with the castle
		// (a fallen castle gates nothing) and ResetCastle restores it — the response
		// matrix persists across both.
		GateBlockerVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// PATHING lane — re-assert the team interior area. The class SELECTION
	// happened in PostInitializeComponents (loop-2 Leg 2: before the first
	// generation pass), so this SetAreaClass early-outs unchanged (a free no-op)
	// on the level-load path; it is kept for any spawned-castle path where
	// BeginPlay is the first team-authoritative hook. Cost-1 area: own-team
	// pathing and un-filtered queries (placement nav projection) are
	// byte-identical to plain navmesh; only the ENEMY's UNavFilter_Team* excludes
	// it (SiegeNavAreas one-home).
	if (InteriorNavModifier)
	{
		InteriorNavModifier->SetAreaClass(SiegeTeamInteriorAreaClass(Team));

		// TASK-349 loop-4 B4 (Jonathan-authorized): UNCONDITIONAL octree
		// re-assert + bounds-dirty — the Leg-3 fence applied once at startup.
		// WHY Leg 2 alone was not enough (the FINAL-RUN's 626-sample / 211 s
		// proof): Leg 2 makes the area class correct BEFORE the component's first
		// registration, which is exactly right for FRESHLY GENERATED tiles — but
		// it thereby removed the only POST-registration area CHANGE, so
		// PRE-BUILT tiles (editor-built and SAVED — every real boot lane; the
		// editor world never runs PostInitializeComponents/BeginPlay on level
		// actors, so its tiles are always ctor-Blue on BOTH interiors) were
		// never dirtied and never re-marked: the red hall stayed Blue-open/
		// Red-closed until melee crumble happened to trip the ApplyCrumbleStage
		// fence. The two legs deliberately COEXIST: Leg 2 = generation-time
		// correctness (fresh tiles build team-correct-first-time, no flip
		// window); this refresh = the pre-built-tile re-mark (forces the
		// castle-bounds tiles to rebuild once, gathering the already-correct
		// team area). It runs at BeginPlay ONLY — never at Play-Again
		// (ResetCastle keeps its own proven Leg-3 fence; actors do not re-run
		// BeginPlay at reset), so the reset path is byte-identical to the
		// FINAL-RUN-verified behavior. On the fresh-build lane it costs at most
		// one redundant re-mark of just-built-correct tiles; at no point after
		// PostInitializeComponents can any tile be marked with the WRONG team's
		// area (the class never differs from the team class again). The stale
		// window on the pre-built lane is thereby bounded by the castle-bounds
		// tile-rebuild latency (seconds — the FINAL-RUN's fence-triggered
		// rebuild), which the r5 probe measures against the R2 ≤10 s band.
		InteriorNavModifier->RefreshNavigationModifiers();
	}
}

void ACastle::InitHPBarWidget()
{
	if (!HPBarWidget)
	{
		return;
	}

	// WBP_CastleHealthBar is built in TASK-019 and may not exist yet — a missing
	// class is a SILENT no-op per spec (LoadSynchronous returns nullptr for unset
	// paths and absent assets alike; the bare component simply draws nothing).
	UClass* LoadedWidgetClass = HPBarWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		return;
	}

	// Post-BeginPlay SetWidgetClass triggers the component's InitWidget, creating
	// the user widget instance (components have begun play — Super::BeginPlay ran
	// before this is called).
	HPBarWidget->SetWidgetClass(LoadedWidgetClass);

	// Seed-then-bind is the widget's own job (qa/TASK-005-report.md major 2):
	// InitForCastle pushes the current values FIRST, then binds OnCastleHPChanged.
	// A widget of some other class (mis-authored TASK-019 asset) is skipped, not a crash.
	if (UCastleHealthBarWidget* HealthBar = Cast<UCastleHealthBarWidget>(HPBarWidget->GetWidget()))
	{
		HealthBar->InitForCastle(this);
	}
}

void ACastle::ApplyTeamVisuals()
{
	if (!CastleMesh)
	{
		return;
	}

	// LoadSynchronous() returns nullptr for unset paths and not-yet-imported assets alike;
	// in either case we simply skip the assignment (never crash per spec).
	if (UStaticMesh* Mesh = CastleMeshAsset.LoadSynchronous())
	{
		if (CastleMesh->GetStaticMesh() != Mesh)
		{
			CastleMesh->SetStaticMesh(Mesh);
		}
	}

	const TSoftObjectPtr<UMaterialInterface>& TeamMaterial = (Team == ETeamId::Red) ? TeamMaterialRed : TeamMaterialBlue;
	if (UMaterialInterface* Material = TeamMaterial.LoadSynchronous())
	{
		// SM_Castle has a single material slot (TASK-013 spec); the same mesh serves both teams.
		CastleMesh->SetMaterial(0, Material);
	}
}

float ACastle::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// M8 authority belt (TASK-356 doc §3.1): castle HP is server state. Client
	// call sites are also locked at their sources (the D5 observer posture + the
	// hero melee gate); this is the belt that makes the castle itself refuse.
	// Standalone: authority ⇒ byte-identical.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] TakeDamage refused on a non-authority castle copy — castle HP is server-authoritative (M8 doc §3.1)."),
			*GetNameSafe(this));
		return 0.0f;
	}

	// A destroyed castle absorbs nothing further; the destroyed event can never re-fire.
	if (bDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// No friendly fire (GDD §3.0): ignore damage whose instigator is on our own team.
	ETeamId InstigatorTeam = ETeamId::Blue;
	if (TryGetInstigatorTeam(EventInstigator, DamageCauser, InstigatorTeam) && InstigatorTeam == Team)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// Damage-vs-fortification scaling (GDD §3.0), read from the damage TYPE.
	// USiegeDamageType_Siege — and any subclass — applies at 200% (Siege units,
	// Ogre/Sapper, batter the castle, TASK-054); USiegeDamageType_Projectile at
	// 50% (the anti-sniping rule); USiegeDamageType_Spell at 50% (GDD §3.11 /
	// M5 ruling 3, TASK-098 — CASTLE ONLY: it mirrors the M2 Projectile
	// precedent, NOT the M4 Siege both-rule, so ABuilding takes FULL spell
	// damage and Lightning at 200 kills an Arrow Tower at 150);
	// melee/default/untyped at 100% (melee needs no tag, CONVENTIONS
	// damage-type registry — M1 attackers pass base UDamageType and stay
	// byte-identical). Siege, Projectile, and Spell are mutually disjoint
	// types, so the branch order is irrelevant. Units and the hero take listed
	// damage from everything (scaling is castle/building-only).
	float ScaledDamage = ActualDamage;
	const UClass* IncomingDamageType = DamageEvent.DamageTypeClass.Get();
	if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Siege::StaticClass()))
	{
		ScaledDamage *= 2.0f;
	}
	else if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Projectile::StaticClass()))
	{
		ScaledDamage *= 0.5f;
	}
	else if (IncomingDamageType && IncomingDamageType->IsChildOf(USiegeDamageType_Spell::StaticClass()))
	{
		ScaledDamage *= 0.5f;
	}

	CurrentHP = FMath::Max(CurrentHP - ScaledDamage, 0.0f);

	// Actual HP change -> broadcast (TASK-018). Ignored friendly fire and hits on a
	// destroyed castle returned above WITHOUT touching CurrentHP, so a broadcast here
	// always reports a real change (CurrentHP > 0 and ScaledDamage > 0 guarantee the
	// clamp lowered the value — half of a positive float is still positive). Fired
	// BEFORE HandleDestroyed so listeners see the 0-HP value before the destroyed
	// event hides the bar.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	// §3.9 crumble (TASK-157): advance the 75/50/25% stages on the way down (each once,
	// in order). Runs before the §6 feedback below; the overlay hit-flash sits on TOP of
	// whatever crumble material is now on the mesh, so the two never fight.
	UpdateCrumbleStages();

	// §6 castle damage feedback (TASK-154/156/158/179) — ACTUAL damage only (friendly
	// fire + destroyed-castle hits returned above). All null-safe until the art/audio land.
	if (HitFlashComponent)
	{
		HitFlashComponent->TriggerFlash();
	}
	USiegeFeedbackLibrary::ShowDamageNumber(this, ScaledDamage,
		GetActorLocation() + FVector(0.0f, 0.0f, CastleDamageNumberHeightZ), USiegeFeedbackLibrary::TeamTint(Team));
	USiegeFeedbackLibrary::PlayWorldSound(this, CastleHitSoundPath, GetActorLocation());
	// screen shake <= 0.2 s on the LOCAL player controller (TASK-158): resolve the soft
	// shake class null-safe, then the library kicks the index-0 controller.
	if (UClass* ShakeClass = CastleHitCameraShake.LoadSynchronous())
	{
		USiegeFeedbackLibrary::PlayLocalCameraShake(this, ShakeClass);
	}

	if (CurrentHP <= 0.0f)
	{
		HandleDestroyed();
	}

	// AActor contract: return the damage actually applied — the SCALED amount the
	// castle really took. Melee returns exactly the M1 value; attacker hit feedback
	// (TASK-016/020 puff-on-damage checks) keys off > 0 and is unaffected either way.
	return ScaledDamage;
}

void ACastle::HandleDestroyed()
{
	// Single-fire guard: cumulative overkill, duplicate calls, or re-entrancy
	// during the broadcast can never fire the event twice.
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;

	// A fallen castle stops any in-progress Masons repair (TASK-059) — nothing
	// heals a destroyed objective, and the timer must not tick on a hidden actor.
	StopHealOverTime();

	// Visual/collision half BEFORE broadcasting, so any listener querying this
	// castle during the event already sees it destroyed (GDD §3.9). M8 refactor
	// (doc §3.1): the shared half lives in ApplyDestroyedState — the SAME code
	// OnRep_Destroyed runs on clients, so both machines change state identically.
	ApplyDestroyedState(true);

	// §6 castle-destroyed stinger (TASK-179): a 2D one-shot (the win/loss moment).
	// Null-safe until S_CastleDestroyed lands (TASK-180). Server-local in P1 —
	// the client's end-moment audio is the victory/defeat music via the GameState
	// rep; per-castle client cosmetics are P2 wiring (doc §3.1).
	USiegeFeedbackLibrary::PlaySound2D(this, CastleDestroyedSoundPath);

	OnCastleDestroyed.Broadcast(this, Team);
}

void ACastle::ApplyDestroyedState(bool bNowDestroyed)
{
	// The shared visual/collision half (M8, doc §3.1) — server destroy/reset AND
	// client OnRep both run exactly this. Collision rides the actor state on both
	// machines, which also drops/restores the CASTLE-3X gate blocker with the
	// castle symmetrically (addendum §2 — a fallen castle gates nothing).
	SetActorHiddenInGame(bNowDestroyed);
	SetActorEnableCollision(!bNowDestroyed);

	// Screen-space widget components do NOT follow actor hidden-in-game state
	// (the viewport layer checks component visibility only) — toggle explicitly
	// (TASK-018: the bar disappears/returns with the castle).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(!bNowDestroyed, /*bPropagateToChildren=*/true);
	}
}

void ACastle::OnRep_CurrentHP()
{
	// CLIENT HP display (M8, doc §3.1): the same broadcast every server-side
	// mutation makes — the existing bar/HUD delegate path, zero widget changes.
	// MaxHP is CDO/level-authored identically on both machines (not replicated).
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
}

void ACastle::OnRep_Destroyed()
{
	// CLIENT destroyed-state (M8, doc §3.1): visual/collision only — NEVER the
	// OnCastleDestroyed broadcast (server win-condition hook; the end screen
	// reaches this machine via ASiegeGameState's match-result rep, doc §3.4).
	ApplyDestroyedState(bDestroyed);
}

void ACastle::OnRep_CrumbleStage()
{
	// CLIENT crumble display (M8, doc §3.1 + addendum §3): ApplyCrumbleStage is
	// ABSOLUTE (stage N applied directly — join-in-progress lands the final look
	// in one call; its client-side side effects are cosmetic-only: debris burst,
	// nav re-assert, log). Stage 0 is the Play-Again reset — ApplyCrumbleStage
	// deliberately guards 1..3, so the pristine restore is ApplyTeamVisuals(),
	// exactly what the server's ResetCastle runs.
	if (CrumbleStage > 0)
	{
		ApplyCrumbleStage(CrumbleStage);
	}
	else
	{
		ApplyTeamVisuals();
	}
}

void ACastle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The M8 P1 castle set (TASK-356, doc §3.1). Team is COND_InitialOnly — a
	// belt on a level-authored value that is already identical on both machines
	// (and consumed by client-side team checks / the CASTLE-3X gating config).
	DOREPLIFETIME(ACastle, CurrentHP);
	DOREPLIFETIME(ACastle, bDestroyed);
	DOREPLIFETIME(ACastle, CrumbleStage);
	DOREPLIFETIME_CONDITION(ACastle, Team, COND_InitialOnly);
}

void ACastle::UpdateCrumbleStages()
{
	// §3.9 crumble (TASK-157): advance while the current HP fraction has crossed the NEXT
	// stage's threshold, firing each stage exactly once IN ORDER. A single big hit that
	// crosses 75% and 50% in one blow fires stage 1 then stage 2 this call. CrumbleStage is
	// monotonic (never retreats), so a Masons heal-back-up never un-crumbles or re-arms a
	// passed stage — only ResetCastle re-arms (CrumbleStage = 0). No-op with a zero MaxHP.
	if (MaxHP <= 0.0f || CrumbleStage >= 3)
	{
		return;
	}

	const float Fraction = CurrentHP / MaxHP;
	while (CrumbleStage < 3)
	{
		const int32 NextStage = CrumbleStage + 1;
		const float NextThreshold = (NextStage == 1) ? CrumbleFraction1 : (NextStage == 2) ? CrumbleFraction2 : CrumbleFraction3;
		if (Fraction <= NextThreshold)
		{
			CrumbleStage = NextStage;
			ApplyCrumbleStage(NextStage);
		}
		else
		{
			break;
		}
	}
}

void ACastle::ApplyCrumbleStage(int32 Stage)
{
	if (Stage < 1 || Stage > 3 || !CastleMesh)
	{
		return;
	}

	// Soft, null-safe (composed per stage, resolved via the feedback library's cached
	// log-once resolvers). A missing mesh/material keeps the current look — the debris
	// still bursts so the stage always READS even before the swap art lands (TASK-171/174).
	const FString CrumbleMeshPath = FString::Printf(TEXT("/Game/Meshes/SM_Castle_Crumble0%d"), Stage);
	const FString CrumbleMaterialPath = FString::Printf(TEXT("/Game/Materials/MI_Castle_Crumble0%d"), Stage);

	if (UStaticMesh* CrumbleMesh = USiegeFeedbackLibrary::ResolveStaticMesh(CrumbleMeshPath))
	{
		// VISUAL swap only — the crumble mesh variants must preserve the castle's UCX
		// footprint (art contract, handoff), so collision/placement/pathing are untouched.
		CastleMesh->SetStaticMesh(CrumbleMesh);

		// TASK-349 loop-2 B3(c): the mesh swap dirties this castle's nav tiles —
		// re-assert the interior modifier's octree entry in the SAME frame, so
		// every tile the swap rebuilds gathers the team area (never a window where
		// interior navmesh exists without its area). Belt to the ctor decoupling's
		// braces; null-safe and cheap (a registered-component octree update).
		if (InteriorNavModifier)
		{
			InteriorNavModifier->RefreshNavigationModifiers();
		}
	}
	if (UMaterialInterface* CrumbleMaterial = USiegeFeedbackLibrary::ResolveMaterial(CrumbleMaterialPath))
	{
		CastleMesh->SetMaterial(0, CrumbleMaterial);
	}

	// debris burst at the castle (soft, null-safe until NS_CastleDebris lands).
	USiegeFeedbackLibrary::SpawnNiagara(this, CastleDebrisVFXPath, GetActorLocation());

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s': crumble stage %d (%.0f%% HP threshold, GDD §3.9) — mesh/material swap + debris (visual only; footprint unchanged)."),
		*GetNameSafe(this), Stage, ((Stage == 1) ? CrumbleFraction1 : (Stage == 2) ? CrumbleFraction2 : CrumbleFraction3) * 100.0f);
}

void ACastle::ResetCastle()
{
	// M8 authority guard (TASK-356 doc §3.1): the reset is server state; clients
	// converge via the HP/destroyed/crumble OnReps. Standalone: authority ⇒
	// byte-identical (its one caller is the server-only GameMode anyway).
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] ResetCastle refused on a non-authority castle copy — the server drives resets (M8 doc §3.1)."),
			*GetNameSafe(this));
		return;
	}

	// Play Again (GDD §3.9): cancel any in-progress Masons repair (TASK-059) first,
	// then back to full HP, visible, solid, and armed to fire again. The visual/
	// collision half is the shared ApplyDestroyedState (M8 refactor, doc §3.1) —
	// the same code the client's OnRep_Destroyed runs on its false edge. (It also
	// re-shows the HP bar, absorbing the explicit re-show this function carried.)
	StopHealOverTime();
	bDestroyed = false;
	CurrentHP = MaxHP;
	ApplyDestroyedState(false);

	// §3.9 crumble reset (TASK-157): back to stage 0 and restore the pristine SM_Castle +
	// team material (undo any crumble mesh/material swap), re-arming all thresholds.
	CrumbleStage = 0;
	ApplyTeamVisuals();

	// TASK-349 loop-2 B3(c) — the Play-Again determinism fence: the reset just
	// re-enabled the actor's collision AND swapped the crumbled mesh back to the
	// pristine SM_Castle, both of which dirty this castle's nav tiles for an
	// async rebuild. Re-assert the interior modifier's octree entry in the SAME
	// frame, so the area data is guaranteed present when ANY of those tiles
	// rebuilds — closing the measured +8.5→+29.7 s window where the hall was
	// nav-open to BOTH filters (the enemy-open direction; R2 non-negotiable).
	// With the ctor decoupling the entry also no longer rides the churned
	// geometry element at all; this same-frame refresh makes the ordering
	// explicit rather than incidental. Null-safe.
	if (InteriorNavModifier)
	{
		InteriorNavModifier->RefreshNavigationModifiers();
	}

	// Counterpart of the HandleDestroyed hide — the bar returns with the castle (TASK-018).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(true, /*bPropagateToChildren=*/true);
	}

	// Reset-path broadcast (TASK-018 + CONVENTIONS delegate rules): the bar refills
	// to MaxHP/MaxHP. Unconditional by spec — resetting an undamaged castle still
	// notifies (reset is an explicit reset event, not a suppressed no-op mutation).
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
}

bool ACastle::IsPointInSpawnBox(const FVector& Point) const
{
	// Castle-centered 2D square test (Z ignored). Additive third reader of the (2460,2460)
	// paired-tunable (TASK-349 re-derivation) — does NOT touch the bot's
	// IsPointInBotSpawnBox (TASK-262). W1 TASK-275.
	const FVector Origin = GetActorLocation();
	return FMath::Abs(Point.X - Origin.X) <= SpawnBoxHalfExtent.X
		&& FMath::Abs(Point.Y - Origin.Y) <= SpawnBoxHalfExtent.Y;
}

bool ACastle::TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam)
{
	// 1) The instigating controller's pawn (hero melee reports its controller).
	if (EventInstigator)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// 2) The damage causer itself (summoned units apply damage directly).
	if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser))
	{
		OutTeam = Agent->GetTeamId();
		return true;
	}

	// 3) The causer's instigator pawn (covers projectiles once M2 adds them).
	if (DamageCauser)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser->GetInstigator()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// No team could be resolved (e.g. world/kill-Z damage) — caller applies the damage.
	return false;
}

void ACastle::HealOverTime(float Total, float Duration)
{
	// M8 authority guard (TASK-356 doc §3.1): Masons repair mutates server HP; a
	// client copy refuses (the Masons entry point is also D5-locked at the PC).
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("[%s] HealOverTime refused on a non-authority castle copy — castle HP is server-authoritative (M8 doc §3.1)."),
			*GetNameSafe(this));
		return;
	}

	// A destroyed castle absorbs no repair; a non-positive amount/duration is a
	// caller error (never scheduled — matches the §3.0 "no free effect" discipline).
	if (bDestroyed)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ACastle '%s': HealOverTime ignored — castle is destroyed."), *GetNameSafe(this));
		return;
	}
	if (Total <= 0.0f || Duration <= 0.0f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ACastle '%s': HealOverTime(%.1f over %.1fs) ignored — Total and Duration must be positive."),
			*GetNameSafe(this), Total, Duration);
		return;
	}

	// Add to the running pool and (re)derive the per-tick delivery so the WHOLE
	// remaining pool lands over Duration — a second Masons reinforces the stream
	// (restack) rather than replacing it (TASK-059).
	HealRemaining += Total;
	const float TicksOverDuration = FMath::Max(Duration / HealTickInterval, 1.0f);
	HealPerTick = HealRemaining / TicksOverDuration;

	// Already at full? Nothing to deliver (never over MaxHP) — drop the pool.
	if (CurrentHP >= MaxHP)
	{
		StopHealOverTime();
		return;
	}

	// (Re)arm the repeating tick; SetTimer replaces on the same handle (no stacking).
	GetWorldTimerManager().SetTimer(HealTimerHandle, this, &ACastle::HandleHealTick, HealTickInterval, /*bLoop=*/ true);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s': repairing %.0f HP over %.1fs (%.1f/tick every %.2fs; %.0f/%.0f HP now)."),
		*GetNameSafe(this), Total, Duration, HealPerTick, HealTickInterval, CurrentHP, MaxHP);
}

void ACastle::HandleHealTick()
{
	// Stop the stream if the castle fell, the pool emptied, or HP is already full.
	if (bDestroyed || HealRemaining <= 0.0f || CurrentHP >= MaxHP)
	{
		StopHealOverTime();
		return;
	}

	const float Delta = FMath::Min(HealPerTick, HealRemaining);
	const float NewHP = FMath::Min(CurrentHP + Delta, MaxHP); // clamp — never over MaxHP
	const float Applied = NewHP - CurrentHP;
	CurrentHP = NewHP;
	HealRemaining = FMath::Max(HealRemaining - Delta, 0.0f);

	// Broadcast only on an ACTUAL change (mirrors TakeDamage's real-change contract).
	if (Applied > 0.0f)
	{
		OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
	}

	if (HealRemaining <= 0.0f || CurrentHP >= MaxHP)
	{
		StopHealOverTime();
	}
}

void ACastle::StopHealOverTime()
{
	GetWorldTimerManager().ClearTimer(HealTimerHandle);
	HealRemaining = 0.0f;
	HealPerTick = 0.0f;
}
