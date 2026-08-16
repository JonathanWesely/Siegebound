// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Castle.h"

#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h" // TASK-562: SpawnActor for the castle's own furnishing
#include "EngineUtils.h" // TASK-398: TActorIterator for FindNearestCastleForTeam
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GitClaudeUnrealTest.h"
#include "Materials/MaterialInterface.h"
#include "NavModifierComponent.h"
#include "Net/UnrealNetwork.h" // M8 (TASK-356): DOREPLIFETIME registration
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "TimerManager.h"
#include "Siegebound/CastleHealthBarWidget.h"
#include "Siegebound/CommanderNpc.h" // TASK-562: the commander NPC this castle spawns (TASK-559's class)
#include "Siegebound/DamageTypes.h"
#include "Siegebound/SiegeFeedbackLibrary.h"
#include "Siegebound/SiegeHitFlashComponent.h"
#include "Siegebound/SiegeNavAreas.h"
#include "Siegebound/Torch.h" // TASK-562: the interior torches this castle spawns (TASK-558's class)

namespace
{
	//~ M7 §6 juice/feel soft-ref paths (TASK-157/158/179) — null-safe (art arrives in TASK-171-adjacent/174/180).
	const TCHAR* CastleHitSoundPath = TEXT("/Game/Audio/S_CastleHit");             // TASK-179
	const TCHAR* CastleDestroyedSoundPath = TEXT("/Game/Audio/S_CastleDestroyed"); // TASK-179
	const TCHAR* CastleDebrisVFXPath = TEXT("/Game/VFX/NS_CastleDebris");          // TASK-157 debris burst

	/**
	 *  Height above the castle origin for its floating damage number (clears the
	 *  ~8083-tall 9× mesh, HP-bar Z parity).
	 *
	 *  ⚠️ TASK-557 LEDGER ROW, AND IT IS A SEPARATE ROW FROM THE HP BAR EVEN THOUGH
	 *  IT HOLDS THE SAME NUMBER — CONVENTIONS WR-§2 lists only ACastle::HPBarWidget's
	 *  Z (row 5); this constant is the SC-§22 sweep's find, a SECOND transcription of
	 *  the same derived height in a different file scope. Re-derived ×3 with the bar
	 *  by TASK-349 (1050 → 3150) and again by TASK-557 (3150 → 9450). ⛔ The two MUST
	 *  move together — "HP-bar Z parity" is the whole contract, and a partial edit
	 *  puts the damage numbers and the health bar at visibly different heights while
	 *  compiling perfectly.
	 */
	constexpr float CastleDamageNumberHeightZ = 9450.f;

	//~ ======================= WAR ROOM FURNISHING (TASK-562) =======================
	//~ THE 9× INTERIOR, IN CASTLE-MESH-LOCAL SPACE. Every furnishing anchor below is
	//~ COMPUTED from these figures rather than typed, so the derivation is readable
	//~ instead of trusted (SC-§34's structural preference, applied to authored data:
	//~ the anchors are still transcriptions — an EditDefaultsOnly transform has to be
	//~ — but each one is a NAMED ARITHMETIC EXPRESSION over the room it sits in).
	//~
	//~ ⛔ PROVENANCE, STATED SO IT CAN BE CHECKED RATHER THAN BELIEVED. Primary source
	//~ = handoffs/TASK-348-artist.md's PUBLISHED, COMMITTED carve readbacks for the 3×
	//~ castle, multiplied by 3 under CONVENTIONS WR-§1 ("the SHELL scales"), which
	//~ states the two headline results itself: grand hall 970 × 240 → 2910 × 720, and
	//~ clear height 520 → 1560. Corroborated read-only against the in-tree
	//~ Tools/ArtPipeline/pipeline_manifest.json carve block, which TASK-555 has
	//~ already re-derived to exactly these numbers — ⚠️ CORROBORATION ONLY: that file
	//~ belongs to an in-flight parallel task and is deliberately NOT the source (the
	//~ TASK-558 precedent — an in-flight working file is not a contract).
	//~
	//~   SM_Castle's origin is GROUND-CENTRE (WR-§0), so local Z 0 is the arena floor
	//~   and local Z 174 is the interior floor. The gate corridor mouth is on the
	//~   local −Y side and +Y is "deeper into the keep" (InteriorAnchorRelativeLocation
	//~   and GateBlockerRelativeLocation both state this; TASK-350 PIE-verified it).
	//~
	//~ ⛔ NOT A WR-§2 / SC-§34 LEDGER ROW. Every constant in this block is BORN at the
	//~ 9× scale — none of them existed before this batch and none was derived from the
	//~ old castle's size, so there is nothing stale here to re-derive. TASK-557 owns
	//~ the ledger and this task adds no row to it (and ⛔ re-touches none of its eight
	//~ initialisers).

	/** Interior floor height, mesh-local (WR-§1: the 3× castle's z 58 × 3). */
	constexpr float InteriorFloorZ = 174.f;

	/** Grand-hall clear height (WR-§1: 520 × 3). */
	constexpr float HallClearHeightZ = 1560.f;

	/**
	 *  THE ONE MOUNT HEIGHT EVERY TORCH USES — the midpoint of the hall's clear
	 *  height, i.e. 780 uu above the interior floor. One number, one derivation:
	 *    • It is also the hall carve box's own centre Z, so it is the height at
	 *      which the walls are guaranteed vertical and solid in EVERY interior
	 *      volume, not just the hall.
	 *    • Verified against the GATE CORRIDOR too, which is an ARCH and not a box:
	 *      the corridor's walls are vertical from the floor up to the springline at
	 *      z 1410, and 174 < 954 < 1410, so a corridor torch is on flat wall with
	 *      456 uu of wall still above it.
	 *    • It puts each light pool's centre 780 above the floor. A torch's
	 *      attenuation radius is ATorch's own EditDefaultsOnly tunable and is
	 *      deliberately NOT duplicated here; at its shipped default the pool still
	 *      reaches ≈912 uu horizontally AT FLOOR LEVEL, which is the figure the
	 *      spacing below is checked against (see ACastle::ACastle).
	 */
	constexpr float TorchWallMountZ = InteriorFloorZ + 0.5f * HallClearHeightZ; // 954

	//~ hall_main — the grand hall under the keep. TASK-348: box x −640..+330,
	//~ y +90..+330, z 58..578 ⇒ ×3 below. 2910 × 720 × 1560, exactly WR-§1's figures.
	constexpr float HallMinX = -1920.f;
	constexpr float HallMaxX = 990.f;
	constexpr float HallMinY = 270.f;
	constexpr float HallMaxY = 990.f;
	constexpr float HallCentreX = 0.5f * (HallMinX + HallMaxX); // −465
	constexpr float HallCentreY = 0.5f * (HallMinY + HallMaxY); // 630
	constexpr float HallThirdX = (HallMaxX - HallMinX) / 3.f;   // 970

	//~ hall_east — the east annex, connected to hall_main by a full-height doorway.
	//~ TASK-348: box x +280..+560, y +190..+330 ⇒ ×3: x +840..+1680, y +570..+990.
	constexpr float AnnexMaxX = 1680.f;
	constexpr float AnnexCentreY = 780.f;

	//~ gate_corridor — the vaulted passage from the gate arch into the hall.
	//~ TASK-348: 500-wide arch, y −380..+170 ⇒ ×3: 1500 wide about the mesh's own
	//~ gate centreline x = 18, y −1140..+510. Its side walls are therefore at
	//~ x = 18 ∓ 750.
	constexpr float CorridorWestWallX = -732.f;
	constexpr float CorridorCentreY = -315.f;

	//~ ---- the actor classes the castle furnishes itself with (BP first, C++ fallback) ----
	const TCHAR* TorchBlueprintPath = TEXT("/Game/Blueprints/BP_Torch.BP_Torch_C");                   // WR-§4
	const TCHAR* CommanderNpcBlueprintPath = TEXT("/Game/Blueprints/BP_CommanderNpc.BP_CommanderNpc_C"); // WR-§5
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
	// any camera angle/distance; relative Z +9450 clears the ~8083-tall 9× castle.
	// TASK-557 (CONVENTIONS WR-§2 row 5) DIAGNOSED this Z as C++-AUTHORED HERE — not
	// BP-authored — and therefore re-derived it in code rather than deferring it to
	// the integration task: 1050-over-900 → 3150-over-2694 (TASK-349) → 9450-over-8083,
	// the same ≈1.17× headroom over the mesh top at all three scales. ⛔ The DrawSize
	// (256x32) is DELIBERATELY NOT SCALED — a screen-space widget's size is in SCREEN
	// pixels and has nothing to do with world scale (SC-§34's human-scale exemption,
	// applied to a UI quantity). The widget CLASS is soft-resolved at BeginPlay
	// (WBP_CastleHealthBar, TASK-019); the bare component always exists and draws
	// nothing.
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(CastleMesh);
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidget->SetDrawSize(FVector2D(256.0f, 32.0f));
	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 9450.0f));
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

	// ================= WAR ROOM FURNISHING (TASK-562; WR-§4 + WR-§5) =================
	// The classes. Soft + null-safe, and the fallback is the C++ class rather than
	// "no furniture" — see the two header doc blocks for the two distinct null paths.
	// ⚠️ Neither Blueprint exists yet and no task in this batch authors one, so the
	// FALLBACK is the path that runs today. That is a complete, working torch and a
	// complete, working commander; the Blueprints are the tuning surface, not a
	// requirement.
	TorchClassAsset = TSoftClassPtr<ATorch>(FSoftObjectPath(TorchBlueprintPath));
	CommanderNpcClassAsset = TSoftClassPtr<ACommanderNpc>(FSoftObjectPath(CommanderNpcBlueprintPath));

	// ---- THE SIX TORCH ANCHORS ----
	// Rotation is what aims the torch OFF the wall: SM_Torch's origin is its
	// WALL-MOUNT FACE and the mesh extends along its own +X into the room
	// (TASK-556 / WR-§4), so yaw points +X away from the masonry. Every anchor sits
	// EXACTLY ON the carve-cutter face, i.e. flush on the wall surface: an inset
	// would float the torch, and the cutter faces ARE the wall surfaces (TASK-348's
	// probe hit hall_back at exactly the cutter's y_max). Scale stays 1 — a torch's
	// SIZE is TASK-556's mesh, not this array's business.
	//
	// SPACING, AND THE CHECK IT WAS DERIVED FROM: at TorchWallMountZ the light pool's
	// centre is 780 above the floor, so at ATorch's shipped attenuation default the
	// pool still reaches ≈912 uu horizontally where the floor is. Three torches on
	// the hall's 2910-uu north wall, one at the centre of each THIRD (970 apart),
	// therefore overlap continuously and cover the hall's full length AND its full
	// 720-uu depth from one wall. ⛔ The attenuation radius itself is deliberately
	// NOT duplicated into this file — it is ATorch's EditDefaultsOnly tunable, and a
	// second copy of it here is exactly the stale-constant hazard SC-§34 exists for.
	TorchAnchors.Reserve(6);

	// 1–3: the grand hall's NORTH wall (y = HallMaxY), each at the centre of one
	// third of the hall's length, all facing −Y into the room.
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 0.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (−1435, 990, 954)
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 1.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (−465, 990, 954) — the hall's own centre line
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 2.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (+505, 990, 954)

	// 4: the hall's SOUTH wall, mirroring anchor 1, facing +Y. ⚠️ IT IS THE ONLY
	// third-centre THAT WALL HAS: the 1500-wide gate corridor punches through the
	// south wall from x −732 to +768, which swallows the other two. Placing it
	// anyway is deliberate — without it every torch in the room is on one wall.
	TorchAnchors.Add(FTransform(FRotator(0.f, 90.f, 0.f), FVector(HallMinX + 0.5f * HallThirdX, HallMinY, TorchWallMountZ))); // (−1435, 270, 954)

	// 5: the EAST ANNEX's far wall, at the annex's own Y centre, facing −X.
	// ⚠️ NOT decoration: the annex reaches x +1680 and the nearest hall torch's pool
	// stops ≈263 uu short of that wall at floor level, so without this anchor the
	// annex is the one carved interior volume that is unlit.
	TorchAnchors.Add(FTransform(FRotator(0.f, 180.f, 0.f), FVector(AnnexMaxX, AnnexCentreY, TorchWallMountZ))); // (+1680, 780, 954)

	// 6: the GATE CORRIDOR's west wall at its mid-length, facing +X. One pool spans
	// the passage's full 1650-uu length; the corridor is 1500 wide, so its east half
	// is lit by falloff and by hall spill rather than directly.
	// ⚠️ FLAGGED, AND IT IS THE FIRST THING TO ADD IF THE PASSAGE READS DARK: the
	// mirrored anchor (+768, −315, 954) yaw 180 completes a facing pair. It is left
	// out only because MaxTorchesPerCastle is 6 by law; adding it is one array entry
	// plus one cap bump, both EditDefaultsOnly, no recompile.
	TorchAnchors.Add(FTransform(FRotator(0.f, 0.f, 0.f), FVector(CorridorWestWallX, CorridorCentreY, TorchWallMountZ))); // (−732, −315, 954)

	// ---- THE COMMANDER NPC ANCHOR ----
	// A FLOOR point in the grand hall: X = the hall's own centre; Y = the midpoint of
	// the hall's northern half, which leaves 180 uu of clearance to the back wall;
	// Z = the interior floor (SK_Sorcerer is feet-origin and SM_WarTable is
	// floor-contact-origin, so relative Z 0 on both — TASK-559). Yaw −90 turns him to
	// face −Y, i.e. toward the gate corridor the player walks in through.
	//
	// ⭐ WHY THE ANCHOR IS THE COMMANDER AND NOT THE TABLE, AND WHY THAT MATTERS:
	// ACommanderNpc places its war table a fixed distance along the actor's own +X.
	// This anchor deliberately does NOT transcribe that distance — instead it is
	// chosen so the placement is ROBUST to it. At TASK-559's shipped 200 uu the table
	// lands at y ≈ 610, within 20 uu of the hall's own centre (630); and the anchor
	// stays legal for ANY forward offset below 300 uu (the table stays north of the
	// corridor mouth at y 510) and below 540 uu (it stays inside the hall at y 270).
	// ⇒ if that constant is ever tuned, this anchor does not silently go stale.
	//
	// ⛔ CHECKED AGAINST THE TWO PLACES HE MAY NOT STAND (WR-§5 / spec item 2):
	//   • the GATE CORRIDOR occupies y −1140..+510; he is at y 810, north of it.
	//   • the APPROACH ramp/stair is OUTSIDE the shell, below the gate arch at
	//     y ≤ −2100; he is 2910 uu deeper in and 174 uu up, on the flat hall floor.
	// ⛔ And he is not in the doorway either: the corridor mouth overlaps the hall
	// only up to y 510.
	CommanderNpcAnchor = FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallCentreX, 0.5f * (HallCentreY + HallMaxY), InteriorFloorZ)); // (−465, 810, 174)
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

	// TASK-562 (WR-§4 + WR-§5): the castle furnishes ITSELF — nothing is placed in
	// L_Arena. Runs on the server AND on every client (both build their own Tier-C
	// local set); ⛔ no authority guard, deliberately — see the class doc.
	//
	// ⚠️ THE TEAM-TIMING HAZARD TASK-559 ASKED THIS TASK TO RE-CONFIRM RATHER THAN
	// ASSUME, ANSWERED AT THE CODE: Team is COND_InitialOnly, so on a client
	// BeginPlay could in principle run before initial replication settles — but both
	// castles are LEVEL-PLACED (a module-wide grep finds no SpawnActor<ACastle>
	// anywhere), so each machine's own copy of L_Arena deserializes the correct Team
	// before any replication arrives; the rep is the belt Castle.h documents, not the
	// source. ⭐ AND THE SHIPPED CODE ALREADY DEPENDS ON EXACTLY THIS, EARLIER:
	// PostInitializeComponents selects the team interior nav area from Team one hook
	// BEFORE this one, and TASK-350 PIE-verified the result on both instances. A push
	// at BeginPlay is therefore strictly safer than something already proven in
	// engine. ⛔ IF a runtime-spawned castle is ever added, this assumption must be
	// re-opened — that, not the replication, is the condition it rests on.
	SpawnCastleFurnishings();
}

void ACastle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// TASK-562 belt (see the header): AActor::Destroy DETACHES attached actors
	// instead of destroying them, so an explicit ACastle::Destroy() would strand the
	// furniture. Nothing calls it today; this is here so nothing has to remember not
	// to. World teardown reaches every actor anyway, which makes this a no-op on the
	// ordinary path.
	DestroyCastleFurnishings();

	Super::EndPlay(EndPlayReason);
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

UClass* ACastle::ResolveTorchClass()
{
	// CLEARED = the deliberate designer opt-out: this castle spawns no torches, and
	// says nothing about it (the AttackImpactEffect IsNull pattern, TASK-020).
	// Deliberately DISTINCT from "set but unresolvable", below.
	if (TorchClassAsset.IsNull())
	{
		return nullptr;
	}

	if (UClass* LoadedClass = TorchClassAsset.LoadSynchronous())
	{
		return LoadedClass;
	}

	// SET BUT UNRESOLVABLE ⇒ the raw C++ class, which is a complete working torch
	// (the ASiegeGameMode::ResolveHeroPawnClass fallback shape). Logged ONCE per
	// castle, and at Log rather than Warning ON PURPOSE: BP_Torch's absence is the
	// EXPECTED state right now — no task in the WAR ROOM batch authors it — and a
	// warning that always fires is a warning everyone learns to ignore.
	if (!bLoggedTorchClassFallback)
	{
		bLoggedTorchClassFallback = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ACastle '%s': torch blueprint '%s' unavailable — spawning the C++ ATorch instead (fully functional; the Blueprint is only the tuning surface)."),
			*GetNameSafe(this), *TorchClassAsset.ToString());
	}

	return ATorch::StaticClass();
}

UClass* ACastle::ResolveCommanderNpcClass()
{
	// Same two null paths as ResolveTorchClass — cleared is a silent opt-out,
	// unresolvable falls back to the C++ class with one log line.
	if (CommanderNpcClassAsset.IsNull())
	{
		return nullptr;
	}

	if (UClass* LoadedClass = CommanderNpcClassAsset.LoadSynchronous())
	{
		return LoadedClass;
	}

	if (!bLoggedCommanderNpcClassFallback)
	{
		bLoggedCommanderNpcClassFallback = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ACastle '%s': commander blueprint '%s' unavailable — spawning the C++ ACommanderNpc instead (fully functional; the Blueprint is only the tuning surface)."),
			*GetNameSafe(this), *CommanderNpcClassAsset.ToString());
	}

	return ACommanderNpc::StaticClass();
}

void ACastle::SpawnCastleFurnishings()
{
	UWorld* World = GetWorld();
	if (!World || !CastleMesh)
	{
		return;
	}

	// IDEMPOTENT BY CONSTRUCTION. Every caller (BeginPlay, and both edges of a
	// Play-Again restore) gets exactly one set: clearing first is what makes a
	// redundant call harmless instead of a doubling, and it is cheaper to reason
	// about than a "have I already furnished?" latch that a reset path could desync.
	DestroyCastleFurnishings();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	// AlwaysSpawn, stated ACCURATELY rather than as the usual incantation — I checked
	// the engine rather than assuming, and the honest reason is not the one it looks
	// like: AActor's OWN default is ALREADY AlwaysSpawn (Actor.cpp), so on today's
	// classes this override changes nothing. It is here because the class being
	// spawned is a SOFT REFERENCE somebody will later point at a Blueprint, and
	// SpawnCollisionHandlingMethod is an EditDefaultsOnly property that Blueprint can
	// change — and because APawn's default is AdjustIfPossibleButDontSpawnIfColliding
	// (Pawn.cpp), so anyone who ever "upgrades" ACommanderNpc from AActor to a Pawn
	// would silently get a commander that FAILS TO SPAWN. Every anchor here is a point
	// ON A WALL or on the floor inside the castle's own BlockAll shell, so under either
	// adjusting mode the furniture would be nudged off its authored anchor or dropped
	// entirely. The override makes the castle's placement authoritative whatever the
	// spawned class says. ⇒ No overlap actually needs resolving anyway: both classes
	// ship NoCollision + SetCanEverAffectNavigation(false) (verified in TASK-558/559).
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The anchors are CASTLE-MESH-relative, so compose against the MESH's world
	// transform, never the actor's location: Castle_Red is placed at yaw 180 and the
	// whole point of a relative anchor is that the rotation comes along for free
	// (the GetInteriorAnchorLocation reasoning, applied to a transform instead of a
	// point). FTransform composition is local-then-parent.
	const FTransform CastleMeshTransform = CastleMesh->GetComponentTransform();

	if (UClass* TorchClass = ResolveTorchClass())
	{
		// The cap is a HARD bound on the level's light count, so it is applied to the
		// anchor list rather than trusted to it: adding anchors in a Blueprint can
		// never quietly multiply the lights past MaxTorchesPerCastle, and a cap of 0
		// (or an empty array) simply means no torches.
		const int32 TorchCount = FMath::Min(TorchAnchors.Num(), FMath::Max(MaxTorchesPerCastle, 0));
		SpawnedTorches.Reserve(TorchCount);

		for (int32 AnchorIndex = 0; AnchorIndex < TorchCount; ++AnchorIndex)
		{
			const FTransform TorchWorldTransform = TorchAnchors[AnchorIndex] * CastleMeshTransform;
			// Spawned AT its final transform (not at the origin then moved): ATorch's
			// BeginPlay runs INSIDE SpawnActor, so anything it ever derives from its
			// own transform sees the real one.
			ATorch* Torch = World->SpawnActor<ATorch>(TorchClass, TorchWorldTransform, SpawnParams);
			if (!Torch)
			{
				// A refused spawn is survivable by design: one torch fewer, no crash,
				// and the loop keeps going so a single failure cannot unlight the hall.
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ACastle '%s': torch %d/%d failed to spawn — the castle plays exactly as it does without it."),
					*GetNameSafe(this), AnchorIndex + 1, TorchCount);
				continue;
			}

			// KeepWorldTransform, because the actor is already AT the composed world
			// transform: the attach must preserve it, not re-interpret it as a new
			// relative one. From here the torch tracks the castle forever.
			Torch->AttachToComponent(CastleMesh, FAttachmentTransformRules::KeepWorldTransform);
			SpawnedTorches.Add(Torch);
		}
	}

	if (UClass* CommanderClass = ResolveCommanderNpcClass())
	{
		const FTransform CommanderWorldTransform = CommanderNpcAnchor * CastleMeshTransform;
		ACommanderNpc* Npc = World->SpawnActor<ACommanderNpc>(CommanderClass, CommanderWorldTransform, SpawnParams);
		if (Npc)
		{
			Npc->AttachToComponent(CastleMesh, FAttachmentTransformRules::KeepWorldTransform);

			// ⛔ THE TEAM IS **PUSHED**, NEVER DERIVED (TASK-559's contract, and its
			// header says why): a nearest-castle search would silently pick the ENEMY
			// castle on the mirrored side, and it would make the NPC depend on this
			// class. ⚠️ AND THE ACCESSOR ON THE OTHER SIDE IS GetCommanderTeam(), NOT
			// GetTeamId() — ACommanderNpc deliberately does NOT implement ITeamAgent,
			// because eight shipped call sites read that interface as "legal combat
			// target" and ASummonedUnit::IsTargetAlive treats unknown ITeamAgent types
			// as permanently alive, which would make the commander an unkillable aggro
			// sink standing in the grand hall.
			Npc->InitCommanderNpc(Team);
			SpawnedCommanderNpc = Npc;
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ACastle '%s': commander NPC failed to spawn — the war table is absent and the castle plays exactly as it does today."),
				*GetNameSafe(this));
		}
	}

	// One line per furnishing pass, so TASK-569's PIE matrix ("torches spawn … and do
	// not survive a Play Again as orphans") is a log read rather than an eyeball count.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s' (%s): furnished — %d of %d torch anchors spawned (cap %d), commander %s (WR-§4/WR-§5; nothing placed in L_Arena)."),
		*GetNameSafe(this), (Team == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"),
		SpawnedTorches.Num(), TorchAnchors.Num(), MaxTorchesPerCastle,
		SpawnedCommanderNpc ? TEXT("spawned") : TEXT("absent"));
}

void ACastle::DestroyCastleFurnishings()
{
	// DESTROYED, NEVER POOLED — the ASiegeBattlefieldScatter::ClearScatter lifecycle
	// verbatim. A pooled torch would carry whatever state a future ATorch grows
	// across a match reset; destroying also guarantees the commander's Team push
	// re-runs on a genuinely fresh actor rather than being skipped as "already set".
	for (const TObjectPtr<ATorch>& Torch : SpawnedTorches)
	{
		ATorch* TorchPtr = Torch.Get();
		if (IsValid(TorchPtr))
		{
			TorchPtr->Destroy();
		}
	}
	SpawnedTorches.Reset();

	ACommanderNpc* CommanderPtr = SpawnedCommanderNpc.Get();
	if (IsValid(CommanderPtr))
	{
		CommanderPtr->Destroy();
	}
	SpawnedCommanderNpc = nullptr;
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

	// TASK-562 (WR-§4 teardown clause): the furnishing is SPAWNED ACTORS, not
	// components, so SetActorHiddenInGame/SetActorEnableCollision above do NOT reach
	// it — it needs its own edge handling, and it gets it HERE rather than in
	// HandleDestroyed/ResetCastle for the reason spelled out in the header: those two
	// are authority-only, the furniture is Tier C and exists separately on every
	// machine, and this function is the one BOTH the server paths and the client's
	// OnRep_Destroyed run. A fallen castle keeps no lit torches; a Play-Again restore
	// gets an exactly-fresh set (destroyed, never pooled — the ClearScatter
	// lifecycle), which also re-runs the Team push on the new commander.
	if (bNowDestroyed)
	{
		DestroyCastleFurnishings();
	}
	else
	{
		SpawnCastleFurnishings();
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
	// Castle-centered 2D square test (Z ignored). Additive third reader of the (7380,7380)
	// paired-tunable (TASK-349 re-derived 840 → 2460; TASK-557 re-derived 2460 → 7380
	// for the 9× castle, WR-§2 row 1) — does NOT touch the bot's IsPointInBotSpawnBox
	// (TASK-262). W1 TASK-275.
	const FVector Origin = GetActorLocation();
	return FMath::Abs(Point.X - Origin.X) <= SpawnBoxHalfExtent.X
		&& FMath::Abs(Point.Y - Origin.Y) <= SpawnBoxHalfExtent.Y;
}

FVector ACastle::GetInteriorAnchorLocation() const
{
	// THE ACTOR TRANSFORM, never ActorLocation + offset (header doc): Castle_Red is
	// placed at yaw 180, so a non-zero relative anchor has to ROTATE with the castle
	// or the "deeper into the keep" direction inverts on one side of the map. At the
	// shipped ZeroVector default this returns the actor's own location on both
	// castles, which is the ground-centre origin = the interior floor's centre.
	//
	// RESOLVED WORLD POINTS at the shipped L_Arena placement (Castle_Blue
	// (−25000, 0, 0) yaw 0, Castle_Red (+25000, 0, 0) yaw 180 — TASK-218):
	// Blue (−25000, 0, 0), Red (+25000, 0, 0). Reported in handoffs/TASK-398-programmer.md;
	// the live nav-projection readback belongs to the PIE task, and AMinerUnit logs
	// the resolved point once per miner so that readback is free.
	return GetActorTransform().TransformPosition(InteriorAnchorRelativeLocation);
}

ACastle* ACastle::FindNearestCastleForTeam(UWorld* World, ETeamId Team, const FVector& From)
{
	if (!World)
	{
		return nullptr;
	}

	ACastle* BestCastle = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	// Faithful mirror of the (private) ASummonedUnit::FindOwnCastle — same-team,
	// IsValid, DESTROYED CASTLES SKIPPED (a rubble heap is not a hiding place, and
	// that skip is what delivers CONVENTIONS §5's "own castle destroyed ⇒ idle in
	// place" for free: the caller simply gets nullptr).
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		ACastle* Castle = *It;
		if (!IsValid(Castle) || Castle->GetTeamId() != Team || Castle->IsCastleDestroyed())
		{
			continue;
		}

		// squared 2D distance — the house arena metric (AGoldNode::FindBestMineFor,
		// the miner's arrival test): the arena is flat and height must not skew
		// "nearest". Strict < keeps the first-found castle on an exact tie, so the
		// result is deterministic for a fixed world (there is exactly one standing
		// own castle in every designed flow anyway).
		const float DistSq = static_cast<float>(FVector::DistSquared2D(Castle->GetActorLocation(), From));
		if (DistSq < BestDistSq)
		{
			BestCastle = Castle;
			BestDistSq = DistSq;
		}
	}

	return BestCastle;
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
