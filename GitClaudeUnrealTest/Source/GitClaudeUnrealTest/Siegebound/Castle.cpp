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
	//~
	//~ ⭐ TASK-634 (GH-R7, 2026-08-18) — THE INTERIOR WAS REDESIGNED. TASK-629 rebuilt
	//~ the inside as ONE hollow grand hall (as-built readback: handoffs/TASK-629-artist.md
	//~ §2/§3 — MEASURED, not projected). The provenance above stays as the BIRTH
	//~ record; from here on the AUTHORITY for interior wall/floor planes is 629's
	//~ readback. Constants the new geometry invalidated are RE-DERIVED below and each
	//~ says so on its own line; every unmarked constant was AUDITED and verified to
	//~ HOLD (the per-constant verdict table lives in handoffs/TASK-634-programmer.md).
	//~ This is a GEOMETRY re-derivation, not a WR-§2 scale row — the scale never moved.

	/** Interior floor height, mesh-local (WR-§1: the 3× castle's z 58 × 3; TASK-629 as-built: measured flat 174.00–174.69 end-to-end — HOLDS, TASK-634 audit). */
	constexpr float InteriorFloorZ = 174.f;

	/**
	 *  Grand-hall DESIGN clear height (WR-§1 minimum: 520 × 3). ⚠️ TASK-634: the
	 *  as-built 629 hall clears MORE — flat ceiling z 2160 ⇒ 1986 uu measured — so
	 *  this constant is the WR-§1 design MINIMUM the torch mount height derives
	 *  from, deliberately NOT an as-built readback (GH-R7 pins the derived mount
	 *  z 954 as a design input the redesign preserved; see TorchWallMountZ).
	 */
	constexpr float HallClearHeightZ = 1560.f;

	/**
	 *  THE ONE MOUNT HEIGHT EVERY TORCH USES — the midpoint of the WR-§1 DESIGN
	 *  clear height, i.e. 780 uu above the interior floor. One number, one
	 *  derivation — RE-VERIFIED against the TASK-629 as-built hall (TASK-634):
	 *    • GH-R7 pinned z 954 as a DESIGN INPUT of the redesign, and 629 §3
	 *      measured it good: the hall walls are vertical 174 → 2160, so 954 is
	 *      flat solid wall on EVERY hall wall plane ("mount z 954 has flat wall
	 *      everywhere"). The as-built ceiling (2160) is higher than the design
	 *      minimum, so 954 now sits below the as-built mid-height — that is torch
	 *      FEEL, which is Jonathan's flagged tunable (TASK-571), not a derivation
	 *      error; the derivation basis stays the WR-§1 minimum.
	 *    • The GATE CORRIDOR is an ARCH, not a box — its walls are vertical from
	 *      the floor to the springline, and 629 kept the spring at z 1410
	 *      UNCHANGED: 174 < 954 < 1410, so the corridor torch is on flat wall
	 *      with 456 uu of wall still above it.
	 *    • It puts each light pool's centre 780 above the floor (the LIVE light
	 *      sits at 855 — BP_Torch's TorchLightRelativeOffset (50, 0, 75), the
	 *      TASK-617 amended offset). A torch's attenuation radius is ATorch's /
	 *      BP_Torch's own tunable and is deliberately NOT duplicated here; the
	 *      pool-coverage math for the NEW hall at the live TASK-620 values is in
	 *      handoffs/TASK-634-programmer.md and cited at the anchors below.
	 */
	constexpr float TorchWallMountZ = InteriorFloorZ + 0.5f * HallClearHeightZ; // 954 — HOLDS (TASK-634 audit; GH-R7 design pin, 629-measured flat wall at every site)

	//~ THE GRAND HALL — one hollow volume, the TASK-629 redesign (as-built,
	//~ MEASURED: handoffs/TASK-629-artist.md §2). X span RETAINED from the old
	//~ hall_main (2910 wide); Y grew north, 270..990 → 240..1380 (1140 deep, was
	//~ 720); floor flat 174, ceiling flat 2160 (clear 1986). No partitions, no
	//~ columns, no annex (H1's one-volume default). Re-derived by TASK-634 (GH-R7).
	constexpr float HallMinX = -1920.f;                         // HOLDS (629: hall x −1920..990)
	constexpr float HallMaxX = 990.f;                           // HOLDS (now ALSO the hall's east WALL plane — torch anchor 5)
	constexpr float HallMinY = 240.f;                           // RE-DERIVED 270 → 240 (629: measured south wall plane y 240)
	constexpr float HallMaxY = 1380.f;                          // RE-DERIVED 990 → 1380 (629: measured north wall plane y 1380)
	constexpr float HallCentreX = 0.5f * (HallMinX + HallMaxX); // −465 (unchanged)
	constexpr float HallCentreY = 0.5f * (HallMinY + HallMaxY); // 810 (was 630)
	constexpr float HallThirdX = (HallMaxX - HallMinX) / 3.f;   // 970 (unchanged)

	//~ ⛔ hall_east — the old east annex (AnnexMaxX 1680 / AnnexCentreY 780) — DIED
	//~ WITH TASK-629: the redesign filled the annex void SOLID (629 §5.3, H1's
	//~ one-volume default), so its two constants are DELETED by TASK-634, not
	//~ re-derived — the old torch anchor 5 that sat on its far wall was inside
	//~ masonry and moved to the hall's own east wall (see the anchor block).

	//~ gate_corridor — the vaulted passage from the gate arch into the hall.
	//~ TASK-629 re-cut the old arch profile +3 uu per side (§5.2, the de-graze):
	//~ wall planes now x −735 / +771 (1506 wide, was 18 ∓ 750 = −732/+768); span
	//~ y −1140..+510 UNCHANGED (length 1650, centre −315); spring z 1410 UNCHANGED.
	constexpr float CorridorWestWallX = -735.f;                 // RE-DERIVED −732 → −735 (629: measured corridor west wall plane)
	constexpr float CorridorCentreY = -315.f;                   // HOLDS (629: corridor y −1140..510, unchanged)

	//~ ---- the actor classes the castle furnishes itself with (BP first, C++ fallback) ----
	const TCHAR* TorchBlueprintPath = TEXT("/Game/Blueprints/BP_Torch.BP_Torch_C");                   // WR-§4
	const TCHAR* CommanderNpcBlueprintPath = TEXT("/Game/Blueprints/BP_CommanderNpc.BP_CommanderNpc_C"); // WR-§5

	//~ ========= F1 DISCOVERABILITY FURNISHING (TASK-661; VID-001 branch (i)) =========
	//~ CASTLE-MESH-LOCAL geometry of the entry problem, from the 656 MEASUREMENT
	//~ record (handoffs/TASK-656-buildmaster.md §1/§2 — live==manifest at every
	//~ probed face) per the RELAYED-DIAGNOSIS law: every figure below was
	//~ recomputed against that handoff's tables, never copied from a dispatch
	//~ transcription. Frame: the castle origin is GROUND-CENTRE (WR-§0), local
	//~ z 0 = the arena grass, the gate corridor mouth is on the local −Y side,
	//~ and local +X is the face a straight run from the Blue spawn hits (656 §1,
	//~ the d1 line). ⛔ F1-R3: these constants place VISUALS ONLY — no seal, hull
	//~ or collision figure is authored or altered anywhere in this block.

	/** The d1 design seal: skirt_toe_01's east face plane (656 §2 face bisection — EMPTY at x ≥ 3665, Castle_0 at x ≤ 3660; manifest 3657.5, live == manifest). Referenced for DERIVING visual poses only; the seal itself is untouched (F1-R3). */
	constexpr float SealFaceLocalX = 3657.5f;

	/** Hero capsule radius (Ø84×192 — the GH-R9 measured capsule, 656 §2's instrument). */
	constexpr float HeroCapsuleRadius = 42.f;

	/** The hero's east-face stop lane: seal face + one capsule radius = 3699.5 — 656 measured the VID-001 stop centre at exactly this x (capsule FREE at 3710, BLOCKED at 3690). The trample path's east leg runs down this lane, so the trail begins under the hero's own feet. */
	constexpr float HeroStopLaneX = SealFaceLocalX + HeroCapsuleRadius; // 3699.5

	/** The visual rim crest VID-001's hero jumped at (656 §1: chartreuse rim x ≈ 3645..3655, top 95–96, spanning y −1000..+800 across the face width). The toe rocks stand ON this crest. */
	constexpr float SealRimCrestX = 3650.f;
	constexpr float SealRimTopZ = 95.f;    // measured rim top at the crest; the 92..101 spread across the width sinks/floats a rock base ≤ 6 uu — natural for fieldstone
	constexpr float SealRimMinY = -1000.f; // rim span, south end (656 §1, the x 3600 line probe)
	constexpr float SealRimMaxY = 800.f;   // rim span, north end

	/** Toe-rock picket spacing — the activation ruling verbatim: "spacing ≈ 200 with yaw jitter". */
	constexpr float ToeRockSpacingY = 200.f;

	/** Rock count = the rim span walked at the picket spacing, both ends inclusive: (800 − (−1000)) ÷ 200 + 1 = 10. */
	constexpr int32 ToeRockCount = static_cast<int32>((SealRimMaxY - SealRimMinY) / ToeRockSpacingY) + 1;

	/** Deterministic yaw jitter step for the rocks — the golden angle: no two neighbours share a facing and no repeat period is visible in a 10-rock line. Baked into the CDO anchor array ONCE, so both castles and both machines get the identical picket by construction (the Tier-C symmetry law). */
	constexpr float ToeRockYawStepDeg = 137.5f;

	/** The toe-ring wrap lane: 656 §2 measured the wrap clear SOUTH of y −3692.5, and −3700 is the proven route's own southmost station line (the 16/16 EMPTY battery starts at y −3700) — 7.5 uu of margin inside measured-clear ground. */
	constexpr float WrapLaneY = -3700.f;

	/** South channel half-width: the mouth spans x −1470..+1470 (656 names / the 626 route tables). */
	constexpr float ChannelHalfWidthX = 1470.f;

	/** The mouth's knee-step line: the ~48–52-uu visual step sits in the y −3650..−3600 band (656 §1), the tan ramp 48→142 behind it. Path and banners stop SOUTH of this line — 656 §4: the plaza/doorway already reads as a door once SEEN; only the approach needs dressing. */
	constexpr float MouthStepLineY = -3650.f;

	//~ Gate banner pair — the ruling pins "markers at the mouth (x ±1470 line)".
	//~ ⚠️ DECLARED SC-§15 DEPARTURE from the literal (±1470, −3650) point, on
	//~ measured geometry: ±1470 IS the channel edge where the flank knolls begin
	//~ (245–468 uu over head, 656 §1), so a pole base AT the line risks standing
	//~ in knoll toe; and −3650 is the step band's own start. Each banner is
	//~ pulled 70 uu INTO the channel and 25 uu SOUTH onto the flat approach —
	//~ still "the ±1470 line" to any approaching eye, and provably on ground the
	//~ 656 full-width mouth-line down-traces (x −1500..+1500) measured clear.
	constexpr float GateBannerEdgeInsetX = 70.f;
	constexpr float GateBannerAbsX = ChannelHalfWidthX - GateBannerEdgeInsetX; // 1400
	constexpr float GateBannerY = MouthStepLineY - 25.f;                       // −3675 — flat approach, south of the step band

	//~ Trample path chain — pitch and lift. The ribbon segment's own LENGTH is
	//~ TASK-657's to author (tileable along local X; NOT pinned by the cross-lane
	//~ contract), so the chain is authored to be correct at ANY length: at the
	//~ 600 pitch a shorter segment reads as a worn dashed trail and a longer one
	//~ as a continuous road, and overlapping segments can never z-fight because
	//~ every chain anchor adds a monotonic 0.25-uu stagger on top of the +2
	//~ support lift — no two segments are ever coplanar.
	constexpr float TramplePitch = 600.f;
	constexpr float TrampleLiftZ = 2.f;       // the ruling's "z +2 over measured support"
	constexpr float TrampleStaggerZ = 0.25f;  // per-chain-index anti-coplanar stagger (13 anchors ⇒ max lift 2 + 12×0.25 = 5.0)
	constexpr int32 TrampleSouthLegCount = 6; // east leg y 0 → −3000 at the 600 pitch (then the corner at −3700)
	constexpr int32 TrampleWestLegCount = 5;  // wrap leg x 3099.5 → 699.5 (corner x minus 1..5 pitches; then the turn-in at x 0)

	//~ ---- the F1 mesh assets (TASK-657 authors all four IN PARALLEL — soft, null-safe; the code lands first BY DESIGN, the F1-R2 ruling) ----
	const TCHAR* GateBannerMeshPath = TEXT("/Game/Meshes/SM_Castle_GateBanner.SM_Castle_GateBanner");
	const TCHAR* TramplePathMeshPath = TEXT("/Game/Meshes/SM_Castle_TramplePath.SM_Castle_TramplePath");
	const TCHAR* ToeRockMesh01Path = TEXT("/Game/Meshes/SM_Castle_ToeRock01.SM_Castle_ToeRock01");
	const TCHAR* ToeRockMesh02Path = TEXT("/Game/Meshes/SM_Castle_ToeRock02.SM_Castle_ToeRock02");
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

	// ---- THE SIX TORCH ANCHORS (RE-DERIVED for the 629 hollow hall — TASK-634) ----
	// Rotation is what aims the torch OFF the wall: SM_Torch's origin is its
	// WALL-MOUNT FACE and the mesh extends along its own +X into the room
	// (TASK-556 / WR-§4), so yaw points +X away from the masonry. Every anchor sits
	// EXACTLY ON a wall plane TASK-629 MEASURED (§2/§3 readback: north y 1380,
	// south y 240, east x 990, corridor west x −735 — each verified planar,
	// vertical, and solid at z 954): an inset would float the torch. Scale stays 1
	// — a torch's SIZE is TASK-556's mesh, not this array's business.
	//
	// SPACING, AND THE CHECK IT WAS DERIVED FROM (re-run for the 2910 × 1140 hall
	// in handoffs/TASK-634-programmer.md): the LIVE light sits 855 above the floor
	// (BP_Torch's (50, 0, 75) offset, TASK-617) and the LIVE attenuation is
	// BP_Torch's 1900 (TASK-620's post-save readback — the C++ 1200 default is
	// superseded on the live surface), so the floor pool reaches
	// √(1900² − 855²) ≈ 1697 uu. Three north-wall torches a THIRD apart (970) plus
	// the south/east singles put every point of the hall ≤ ~1240 uu from its
	// nearest anchor — continuous overlap, the full 1140-uu depth covered from the
	// walls. ⛔ The attenuation radius itself is deliberately NOT duplicated into
	// this file — the figures above are CITATIONS of 620's live readback and 634's
	// math, never a second authority; the tunable stays ATorch's/BP_Torch's
	// (SC-§34's stale-constant hazard).
	TorchAnchors.Reserve(6);

	// 1–3: the grand hall's NORTH wall (y = HallMaxY — the 629 wall plane at 1380;
	// the old y-990 wall is GONE, the hall grew 390 uu north), each at the centre
	// of one third of the hall's length, all facing −Y into the room. TASK-629 §3
	// suggested exactly these three sites; the third-centre arithmetic reproduces
	// them because the hall's X span did not change.
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 0.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (−1435, 1380, 954)
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 1.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (−465, 1380, 954) — the hall's own centre line
	TorchAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallMinX + 2.5f * HallThirdX, HallMaxY, TorchWallMountZ))); // (+505, 1380, 954)

	// 4: the hall's SOUTH wall (y = HallMinY — the 629 plane at 240, was 270),
	// mirroring anchor 1, facing +Y. ⚠️ IT IS THE ONLY third-centre THAT WALL HAS:
	// the 1506-wide gate corridor punches through the south wall from x −735 to
	// +771 (629 §2), which swallows the other two (−465 and +505 both fall inside
	// the punch-through). Placing it anyway is deliberate — without it every torch
	// in the room is on one wall. 629 §3's suggested south site, verbatim.
	TorchAnchors.Add(FTransform(FRotator(0.f, 90.f, 0.f), FVector(HallMinX + 0.5f * HallThirdX, HallMinY, TorchWallMountZ))); // (−1435, 240, 954)

	// 5: the hall's EAST wall (x = HallMaxX) at the hall's own Y centre, facing −X.
	// RE-DERIVED by TASK-634: the old site was the east ANNEX's far wall
	// (+1680, 780) and TASK-629 filled the annex SOLID (§5.3) — the old anchor now
	// sits inside masonry. 629 §3's suggested replacement site, verbatim: east
	// wall x 990 at (990, 810), wall verified planar at z 954.
	TorchAnchors.Add(FTransform(FRotator(0.f, 180.f, 0.f), FVector(HallMaxX, HallCentreY, TorchWallMountZ))); // (990, 810, 954)

	// 6: the GATE CORRIDOR's west wall (the 629 plane at x −735, was −732) at its
	// mid-length, facing +X. One pool spans the passage's full 1650-uu length
	// (the y span is UNCHANGED by 629); the corridor is 1506 wide, and at the LIVE
	// TASK-620 attenuation the single pool now reaches even the far corridor
	// corners (≈1674 uu ≤ the ≈1697 floor-pool radius — TASK-634 math), closing
	// the old east-half coverage hole AT CUTOFF (delivered brightness out there is
	// still falloff + hall spill).
	// ⚠️ FLAGGED, STILL THE FIRST THING TO ADD IF THE PASSAGE READS DARK: the
	// mirrored anchor (+771, −315, 954) yaw 180 completes a facing pair. It is left
	// out only because MaxTorchesPerCastle is 6 by law; adding it is one array
	// entry plus one cap bump, both EditDefaultsOnly, no recompile (TASK-571 is
	// Jonathan's feel pass).
	TorchAnchors.Add(FTransform(FRotator(0.f, 0.f, 0.f), FVector(CorridorWestWallX, CorridorCentreY, TorchWallMountZ))); // (−735, −315, 954)

	// ---- THE COMMANDER NPC ANCHOR ----
	// A FLOOR point in the grand hall: X = the hall's own centre; Y = the hall's
	// own centre too — the 629 hall grew north (240..1380), which moved HallCentreY
	// to exactly 810, the y this anchor has ALWAYS held; Z = the interior floor
	// (SK_Sorcerer is feet-origin and SM_WarTable is floor-contact-origin, so
	// relative Z 0 on both — TASK-559). Yaw −90 turns him to face −Y, i.e. toward
	// the gate corridor the player walks in through.
	//
	// ⭐ TASK-634 (GH-R7): the anchor VALUE (−465, 810, 174) is BYTE-IDENTICAL to
	// pre-redesign — 629 §3 MEASURED it interior with 570 uu min horizontal
	// clearance (probed 24 directions × z 300/600/954) — but the old EXPRESSION
	// 0.5×(HallCentreY + HallMaxY) would now compute 1095 under the new bounds, so
	// ONLY the arithmetic is re-based on the new hall constants; the spawned
	// transform does not move by a single unit.
	//
	// ⭐ WHY THE ANCHOR IS THE COMMANDER AND NOT THE TABLE, AND WHY THAT MATTERS:
	// ACommanderNpc places its war table a fixed distance along the actor's own +X.
	// This anchor deliberately does NOT transcribe that distance — instead it is
	// chosen so the placement is ROBUST to it. At TASK-559's shipped 200 uu the
	// table lands at y ≈ 610 (629 §3 measured it: 370 uu clear of the south wall);
	// and the anchor stays legal for ANY forward offset below 300 uu (the table
	// stays north of the corridor mouth at y 510) and below 570 uu (it stays
	// inside the hall, y > 240 — was 540 against the old y-270 wall).
	// ⇒ if that constant is ever tuned, this anchor does not silently go stale.
	//
	// ⛔ CHECKED AGAINST THE TWO PLACES HE MAY NOT STAND (WR-§5 / spec item 2,
	// re-run against 629 §2):
	//   • the GATE CORRIDOR occupies y −1140..+510 (UNCHANGED); he is at y 810,
	//     300 uu north of its mouth.
	//   • the APPROACH/THRESHOLD (the apron + the 148/161 landings) ends at
	//     y −1460 and the ramp itself is outside the shell below the gate arch
	//     (y ≤ −2100); he is on the flat 174 hall floor, 2,270 uu north of the
	//     threshold's end.
	// ⛔ And he is not in the doorway either: the corridor mouth overlaps the hall
	// only up to y 510.
	CommanderNpcAnchor = FTransform(FRotator(0.f, -90.f, 0.f), FVector(HallCentreX, HallCentreY, InteriorFloorZ)); // (−465, 810, 174) — value HOLDS (GH-R7); expression re-based by TASK-634

	// ========= F1 DISCOVERABILITY FURNISHING (TASK-661; VID-001 branch (i)) =========
	// The mesh soft-refs. TASK-657 authors the assets IN PARALLEL to this code —
	// SpawnDiscoverabilityFurnishings resolves each null-safe (missing = family
	// skipped, one log line), so this code compiles, ships and runs correctly
	// BEFORE any asset exists and the families simply appear when 657's import
	// lands. MIs are deliberately NOT referenced here: the meshes carry their
	// materials from import (the 661 names block soft-references SM_ paths only).
	GateBannerMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(GateBannerMeshPath));
	TramplePathMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TramplePathMeshPath));
	ToeRockMeshAsset01 = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(ToeRockMesh01Path));
	ToeRockMeshAsset02 = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(ToeRockMesh02Path));

	// ---- GATE BANNER PAIR — the mouth markers (activation ruling item (a)) ----
	// One either side of the channel mouth at (±1400, −3675, 0): |x| = the 1470
	// channel half-width minus the 70 flank-knoll inset, y = 25 south of the
	// −3650 step line, z 0 = the flat approach the −3700 route station stands on
	// (derivation + the SC-§15 departure note at the constants). Yaw −90 points
	// +X — the banner's FACE, the TASK-657 pivot contract — due SOUTH at the
	// open field, so the pair reads on the approach and frames the one door.
	GateBannerAnchors.Reserve(2);
	GateBannerAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(-GateBannerAbsX, GateBannerY, 0.f))); // west of the mouth (−1400, −3675, 0)
	GateBannerAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f), FVector(+GateBannerAbsX, GateBannerY, 0.f))); // east of the mouth (+1400, −3675, 0)

	// ---- TRAMPLE PATH CHAIN — spawn line → around the toe ring → the mouth (ruling item (b)) ----
	// 13 anchors in WALK ORDER, every yaw aiming +X along the direction of
	// travel (the tileable-along-X contract); z = TrampleLiftZ +
	// TrampleStaggerZ × chain index (the +2 support lift plus the anti-coplanar
	// stagger — see the constants).
	TramplePathAnchors.Reserve(TrampleSouthLegCount + 1 + TrampleWestLegCount + 1);
	int32 TrampleChainIndex = 0;

	// Leg 1 — SOUTH down the hero's own stop lane: x 3699.5 (seal face 3657.5 +
	// capsule radius 42 — the exact line VID-001's hero walked), y 0, −600, …
	// −3000. The first segment starts AT the measured stop point (+3699.5, 0):
	// the trail begins under the hero's feet and leads away. Yaw −90 ⇒ +X = −Y.
	for (int32 SouthSegIndex = 0; SouthSegIndex < TrampleSouthLegCount; ++SouthSegIndex)
	{
		TramplePathAnchors.Add(FTransform(FRotator(0.f, -90.f, 0.f),
			FVector(HeroStopLaneX, -TramplePitch * static_cast<float>(SouthSegIndex), TrampleLiftZ + TrampleStaggerZ * static_cast<float>(TrampleChainIndex))));
		++TrampleChainIndex;
	}

	// The SE corner at (3699.5, −3700): the south→west turn, yaw −135 = the
	// diagonal between the two legs' headings. On the wrap lane, south of the
	// measured −3692.5 toe-ring bound.
	TramplePathAnchors.Add(FTransform(FRotator(0.f, -135.f, 0.f),
		FVector(HeroStopLaneX, WrapLaneY, TrampleLiftZ + TrampleStaggerZ * static_cast<float>(TrampleChainIndex))));
	++TrampleChainIndex;

	// Leg 2 — WEST along the wrap: y −3700 (south of the measured-clear −3692.5
	// bound, ON the proven route's own station line), x = corner x minus 1..5
	// pitches ⇒ 3099.5, 2499.5, 1899.5, 1299.5, 699.5. Yaw 180 ⇒ +X = world −X.
	for (int32 WestSegIndex = 0; WestSegIndex < TrampleWestLegCount; ++WestSegIndex)
	{
		TramplePathAnchors.Add(FTransform(FRotator(0.f, 180.f, 0.f),
			FVector(HeroStopLaneX - TramplePitch * static_cast<float>(WestSegIndex + 1), WrapLaneY, TrampleLiftZ + TrampleStaggerZ * static_cast<float>(TrampleChainIndex))));
		++TrampleChainIndex;
	}

	// The TURN-IN at (0, −3700): the channel's centre lane (x 0 — the exact lane
	// 656 walked 16/16 EMPTY), yaw +90 ⇒ +X = +Y = due north THROUGH the mouth.
	// The chain's last segment is the arrow at the ramp foot; whatever length
	// 657 ships, any overrun past y −3650 vanishes under/into the step mass —
	// the trail runs to the door's own threshold and no further (the ramp needs
	// no dressing, 656 §4).
	TramplePathAnchors.Add(FTransform(FRotator(0.f, 90.f, 0.f),
		FVector(0.f, WrapLaneY, TrampleLiftZ + TrampleStaggerZ * static_cast<float>(TrampleChainIndex))));

	// ---- TOE ROCK PICKET — the east seal line reads as what it is (ruling item (c)) ----
	// Ten rocks ON the rim crest: x 3650 (the measured 3645..3655 crest), y
	// −1000, −800, … +800 (the full measured rim span at the ≈200 ruling
	// spacing, ends inclusive), z 95 = the measured rim top — each ground-
	// contact pivot STANDS ON the very lip the hero jumped at, putting the
	// silhouette exactly where the refusal happens. Yaw walks the golden angle
	// per rock (deterministic — baked into this CDO array once, identical on
	// every machine and both castles). The 01/02 mesh alternation happens at
	// spawn, not here: an anchor is a pose, never an asset choice.
	ToeRockAnchors.Reserve(ToeRockCount);
	for (int32 RockIndex = 0; RockIndex < ToeRockCount; ++RockIndex)
	{
		const float RockYawDeg = FMath::Fmod(ToeRockYawStepDeg * static_cast<float>(RockIndex), 360.f);
		ToeRockAnchors.Add(FTransform(FRotator(0.f, RockYawDeg, 0.f),
			FVector(SealRimCrestX, SealRimMinY + ToeRockSpacingY * static_cast<float>(RockIndex), SealRimTopZ)));
	}
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
	// transform, never the actor's location: the whole point of a relative anchor is
	// that any castle pose comes along for free (the GetInteriorAnchorLocation
	// reasoning, applied to a transform instead of a point). FTransform composition
	// is local-then-parent. (TASK-637 comment rider, GH-R13: this comment used to
	// justify the composition with "Castle_Red is placed at yaw 180" — STALE: BOTH
	// castle actors sit at yaw 0 and both gates face world −Y, measured live at
	// TASK-617 C1. The composition is correct at ANY pose, which is the real reason
	// it is written this way.)
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

	// TASK-661: the F1 discoverability set rides this SAME pass — spawned here so
	// it inherits the identical lifecycle (BeginPlay + both Play-Again edges via
	// ApplyDestroyedState) with zero new call sites; torn down with the torches
	// in DestroyCastleFurnishings, which this function already ran above.
	SpawnDiscoverabilityFurnishings();

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

	// TASK-661: the F1 discoverability set — COMPONENTS, not actors, so the
	// teardown is DestroyComponent (detach + unregister + GC-unroot via the
	// array Reset). Destroyed, never pooled — the same reasoning as the torches
	// above; a Play-Again restore gets an exactly-fresh set.
	for (const TObjectPtr<UStaticMeshComponent>& FurnishingMesh : SpawnedDiscoverabilityMeshes)
	{
		UStaticMeshComponent* FurnishingMeshPtr = FurnishingMesh.Get();
		if (IsValid(FurnishingMeshPtr))
		{
			FurnishingMeshPtr->DestroyComponent();
		}
	}
	SpawnedDiscoverabilityMeshes.Reset();
}

void ACastle::SpawnDiscoverabilityFurnishings()
{
	// TASK-661 (the F1-R2 activation ruling; VID-001 branch (i)) — the castle
	// signposts its own door. The caller is SpawnCastleFurnishings, which has
	// already cleared the previous set and checked the world/mesh, but this
	// function re-checks its own precondition anyway so it can never come to
	// depend on the caller's ordering.
	if (!CastleMesh)
	{
		return;
	}

	int32 BannerCount = 0;
	int32 PathCount = 0;
	int32 RockCount = 0;

	// ---- gate banners (the mouth markers) ----
	// Two null paths per the property doc (the ATorch law): CLEARED = silent
	// opt-out; SET BUT UNRESOLVABLE = ONE log line and the family is skipped —
	// the EXPECTED state until TASK-657's import lands. Log, not Warning, on
	// purpose: an always-firing warning is a warning everyone learns to ignore
	// (the ResolveTorchClass reasoning, verbatim).
	if (!GateBannerMeshAsset.IsNull())
	{
		if (UStaticMesh* BannerMesh = GateBannerMeshAsset.LoadSynchronous())
		{
			for (const FTransform& BannerAnchor : GateBannerAnchors)
			{
				// bCastShadow true: a banner pole is scenery and should shadow.
				if (SpawnDiscoverabilityMesh(BannerMesh, BannerAnchor, true))
				{
					++BannerCount;
				}
			}
		}
		else if (!bLoggedGateBannerMeshMissing)
		{
			bLoggedGateBannerMeshMissing = true;
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ACastle '%s': gate banner mesh '%s' unavailable — family skipped (TASK-657 authors it in parallel; the castle plays exactly as it does today)."),
				*GetNameSafe(this), *GateBannerMeshAsset.ToString());
		}
	}

	// ---- trample path (the guided route) ----
	if (!TramplePathMeshAsset.IsNull())
	{
		if (UStaticMesh* PathMesh = TramplePathMeshAsset.LoadSynchronous())
		{
			for (const FTransform& PathAnchor : TramplePathAnchors)
			{
				// bCastShadow false: a 2-uu-high flat ribbon's shadow buys
				// nothing and is acne fuel under the low grazing moonlight.
				if (SpawnDiscoverabilityMesh(PathMesh, PathAnchor, false))
				{
					++PathCount;
				}
			}
		}
		else if (!bLoggedTramplePathMeshMissing)
		{
			bLoggedTramplePathMeshMissing = true;
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ACastle '%s': trample path mesh '%s' unavailable — family skipped (TASK-657 authors it in parallel; the castle plays exactly as it does today)."),
				*GetNameSafe(this), *TramplePathMeshAsset.ToString());
		}
	}

	// ---- toe rocks (the seal-line dressing) ----
	// The family spawns if EITHER mesh resolves: SM_Castle_ToeRock02 is OPTIONAL
	// by the 657 names block, so odd anchors quietly fall back to 01 (and vice
	// versa) — a missing optional never thins the picket. Logged only when at
	// least one path is SET and NEITHER resolves; both cleared = the silent
	// opt-out, exactly the two-null-paths split above.
	UStaticMesh* RockMesh01 = ToeRockMeshAsset01.IsNull() ? nullptr : ToeRockMeshAsset01.LoadSynchronous();
	UStaticMesh* RockMesh02 = ToeRockMeshAsset02.IsNull() ? nullptr : ToeRockMeshAsset02.LoadSynchronous();
	if (RockMesh01 || RockMesh02)
	{
		for (int32 RockAnchorIndex = 0; RockAnchorIndex < ToeRockAnchors.Num(); ++RockAnchorIndex)
		{
			// Alternate 01/02 by parity for variety; each parity falls back to
			// the other mesh when its own is missing (never a null pick — the
			// enclosing branch guarantees at least one resolved).
			UStaticMesh* PickedRockMesh = ((RockAnchorIndex % 2) == 1 && RockMesh02) ? RockMesh02 : (RockMesh01 ? RockMesh01 : RockMesh02);

			// bCastShadow true: rocks are scenery — their shadow is silhouette.
			if (SpawnDiscoverabilityMesh(PickedRockMesh, ToeRockAnchors[RockAnchorIndex], true))
			{
				++RockCount;
			}
		}
	}
	else if ((!ToeRockMeshAsset01.IsNull() || !ToeRockMeshAsset02.IsNull()) && !bLoggedToeRockMeshMissing)
	{
		bLoggedToeRockMeshMissing = true;
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ACastle '%s': no toe rock mesh resolves ('%s' / '%s') — family skipped (TASK-657 authors them in parallel; the castle plays exactly as it does today)."),
			*GetNameSafe(this), *ToeRockMeshAsset01.ToString(), *ToeRockMeshAsset02.ToString());
	}

	// One line per pass so TASK-659's live re-verify is a log read. The torch
	// furnishing line above is an existing instrument surface (TASK-569's PIE
	// matrix greps it) and stays byte-identical; this family gets its OWN line.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ACastle '%s': F1 discoverability set — %d/%d banners, %d/%d path segments, %d/%d toe rocks spawned (TASK-661; every component forced NoCollision — GH-R9)."),
		*GetNameSafe(this),
		BannerCount, GateBannerAnchors.Num(),
		PathCount, TramplePathAnchors.Num(),
		RockCount, ToeRockAnchors.Num());
}

UStaticMeshComponent* ACastle::SpawnDiscoverabilityMesh(UStaticMesh* Mesh, const FTransform& Anchor, bool bCastShadow)
{
	if (!Mesh || !CastleMesh)
	{
		return nullptr;
	}

	// Auto-unique name (no explicit FName): these components are re-created on
	// every Play-Again restore, and re-using explicit names over just-destroyed
	// pending-kill siblings is a rename-collision hazard for zero benefit —
	// nothing ever addresses them by name.
	UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
	if (!MeshComponent)
	{
		return nullptr;
	}

	MeshComponent->SetStaticMesh(Mesh);

	// Movable: runtime-created, and a Movable child under the Static level-
	// placed castle root is the legal attach direction (the reverse is the
	// engine's mobility warning). They never actually move — they ride the attach.
	MeshComponent->SetMobility(EComponentMobility::Movable);

	// ⛔⛔ THE GH-R9 ENFORCEMENT SITE (TASK-661; the activation ruling's belt and
	// braces, verbatim): collision is disabled CODE-SIDE on every spawned
	// component REGARDLESS of the asset. TASK-657's meshes are collisionless by
	// design (ucx: null declared in the manifest — the belt); these four lines
	// hold even against a mis-authored import (the braces). Profile first, then
	// the explicit SetCollisionEnabled(NoCollision) call the ruling names;
	// overlap events and nav relevancy switched off so the entry chain's
	// collision AND navmesh records stay byte-identical on every route (the
	// GateBlockerVolume inert-setup pattern).
	MeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);
	MeshComponent->SetCastShadow(bCastShadow);

	// CASTLE-MESH-relative attach: the anchor IS the relative transform, so any
	// castle pose comes along for free — the SpawnCastleFurnishings composition
	// reasoning without the composition, because a component takes the relative
	// form directly. SetupAttachment is the pre-registration lane;
	// RegisterComponent then creates the render state.
	MeshComponent->SetupAttachment(CastleMesh);
	MeshComponent->SetRelativeTransform(Anchor);
	MeshComponent->RegisterComponent();

	SpawnedDiscoverabilityMeshes.Add(MeshComponent);
	return MeshComponent;
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
		// SLOT 0 ONLY — and slot-0-only is the recorded CONTRACT (TASK-634 slot-audit
		// addendum, per TASK-630 §3): SM_Castle's slots are [0 TeamRegion, 1 CastlePBR]
		// as shipped and [0 TeamRegion, 1 CastlePBR, 2 CastleInteriorPBR] from
		// TASK-633's import. This write recolors the TeamRegion slot; slots >= 1 are
		// NEVER written at runtime — they render whatever the SAVED mesh asset binds,
		// which is exactly what lets the appended interior slot ship with zero code
		// change. (The claim that stood here — "SM_Castle has a single material slot
		// (TASK-013 spec)" — was stale twice over: the mesh has been two-slot since
		// the TeamRegion split, three-slot from 633. The same mesh still serves both
		// teams.)
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
		// ⭐ THE SLOT CONTRACT, RECORDED HERE PER TASK-630 §3 (TASK-634 slot-audit
		// addendum): this write — like EVERY runtime material write on the castle —
		// touches SLOT 0 ONLY. Slots >= 1 always render what the SAVED stage mesh
		// binds: [0 TeamRegion, 1 CastlePBR/crumble, 2 CastleInteriorPBR] after
		// TASK-633's import. ⛔ The old board line "ApplyCrumbleStage writes both
		// slots by index" NEVER matched this code (630 verified it at source); the
		// crumble look on slots >= 1 is the artist's DESIGN-TIME binding on each
		// SM_Castle_Crumble0N (630 §7.4 — what is saved is what renders mid-match).
		// A future runtime write to slots >= 1 must re-open this contract
		// explicitly, never assume it.
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
	// THE ACTOR TRANSFORM, never ActorLocation + offset (header doc): a non-zero
	// relative anchor has to ROTATE with the castle if a level edit ever yaws one,
	// or the "deeper into the keep" direction inverts on the re-posed side. At the
	// shipped ZeroVector default this returns the actor's own location on both
	// castles, which is the ground-centre origin = the interior floor's centre.
	// (TASK-637 comment rider, GH-R13: this comment and the placement note below
	// used to state "Castle_Red is placed at yaw 180" as the live map — STALE:
	// BOTH castle actors sit at yaw 0 and both gates face world −Y, measured live
	// at TASK-617 C1. TASK-218's yaw-180 plan is history, not the map.)
	//
	// RESOLVED WORLD POINTS at the measured L_Arena placement (Castle_Blue
	// (−25000, 0, 0) and Castle_Red (+25000, 0, 0), BOTH yaw 0 — TASK-617 C1):
	// Blue (−25000, 0, 0), Red (+25000, 0, 0) — identical under any yaw while the
	// anchor stays ZeroVector. Reported in handoffs/TASK-398-programmer.md;
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
