// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "ScatterConfig.generated.h"

class AGoldNode;
class UMaterialInterface;
class UStaticMesh;

/**
 *  Region bias for a scatter layer's placement (CONVENTIONS "Battlefield &
 *  procedural terrain (M6.5)" — the `region-bias (whole-field / edge-bias)`
 *  knob). Used by ASiegeBattlefieldScatter to weight where a layer's instances
 *  land within the arena Y band. Purely aesthetic density weighting — it NEVER
 *  overrides the keep-clear / corridor exclusions (those are the traversability
 *  guarantee and always win).
 *   - WholeField : uniform across the field width (default for grass/hills).
 *   - EdgeBias   : denser toward the field's Y edges, thinner near the lane
 *                  (keeps the mid-field readable for the combat corridor).
 *   - CenterBias : denser toward the centerline Y band (still outside the
 *                  reserved corridor — obstacles hug the flanks of the lane).
 */
UENUM(BlueprintType)
enum class EScatterRegionBias : uint8
{
	WholeField,
	EdgeBias,
	CenterBias
};

/**
 *  GLOBAL terrain symmetry mode (CONVENTIONS "Ancient Grounds + Sorcerer + 180°
 *  terrain symmetry (2026-08-01)" §1 — TASK-358). REPLACES the retired
 *  `bool bMirrorSymmetric` X-mirror toggle, which was a FAKE reflection
 *  (translate + yaw+180; a true reflection needs negative scale, which flips
 *  HISM normals/winding) and is dead by law.
 *
 *   - Rotational180 (DEFAULT, and the ONLY legal terrain symmetry on this
 *     project): every terrain pass generates on the BLUE half (X <= 0) and emits
 *     each instance's twin under a proper 180° rigid rotation about the map
 *     center — `loc' = (-X, -Y, Z)`, `yaw' = Fmod(yaw + 180, 360)`, `scale'
 *     = scale` (UNCHANGED). No negative scale, no winding flip, no
 *     approximation. Jonathan's 2026-08-01 directive.
 *     ⚠️ BLUE, never Red: the PlayerStart exists ONLY at (-23800, 0, 100) — there
 *     is no Red-side PlayerStart — so generating Red would rotate a legally
 *     placed prop straight ONTO the hero spawn.
 *   - Asymmetric: the RETIRED M6.5 fully-organic-random full-field placement,
 *     kept ONLY as an explicit off-state. It is NO LONGER the default and
 *     selecting it again needs a NEW Jonathan ruling — never silently restore it.
 */
UENUM(BlueprintType)
enum class EScatterSymmetryMode : uint8
{
	/** 180° rotational symmetry about the map center — generate the Blue half, emit the rotated twin. THE LAW. */
	Rotational180  UMETA(DisplayName = "180° Rotational (law)"),

	/** Retired fully-asymmetric organic random. Needs a fresh Jonathan ruling to select. */
	Asymmetric     UMETA(DisplayName = "Asymmetric (retired — needs a ruling)")
};

/**
 *  One layer of the procedural battlefield scatter (CONVENTIONS "Battlefield &
 *  procedural terrain (M6.5)"). A layer is one logical group — TREES / ROCKS /
 *  HILLS / GRASS — that shares placement rules and a candidate mesh set. The
 *  scatter actor owns ONE HISM per unique mesh (perf law), and each instance of
 *  a layer picks a random mesh from `Meshes`, so a single layer of 8 mesh
 *  variants yields high visual variety from 8 HISMs.
 *
 *  All fields are content, editor-populated on the DataAsset instance
 *  (/Game/Data/DA_BattlefieldScatter, TASK-137) — none is hardcoded in C++.
 */
USTRUCT(BlueprintType)
struct FScatterLayer
{
	GENERATED_BODY()

	/** Human/debug name for the layer (e.g. "Trees", "Rocks", "Hills", "Grass"). Only used in log lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FName LayerName = NAME_None;

	/**
	 *  Candidate meshes for this layer (SOFT so the config never force-loads the
	 *  heavy Fab donors until GenerateScatter resolves them). One HISM is created
	 *  per unique resolved mesh; each placed instance picks a random entry, so
	 *  more meshes = more variety (Jonathan: "use as many assets as possible").
	 *  Null/unresolvable entries are skipped (logged once), never a crash.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

	/** How many instances to ATTEMPT to place for this layer (rejection sampling may place fewer if the field is crowded). Cap per the §6 perf budget — blocking + dynamic-nav is costly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	int32 InstanceCount = 0;

	/** Uniform per-instance scale range [Min, Max]; each instance rolls a uniform scale in this range. Min==Max==1 = no scale variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FVector2D ScaleRange = FVector2D(1.f, 1.f);

	/** Random full 0-360° yaw per instance (natural rotation variety). Pitch/roll stay upright. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bRandomYaw = true;

	/**
	 *  BLOCKING obstacle layer (Jonathan decision #1, 2026-07-10): instances block
	 *  the Pawn channel and set bCanEverAffectNavigation=true so units physically
	 *  block on them AND the (Dynamic) navmesh carves around them. Defaults TRUE
	 *  (the obstacle layers TREES/ROCKS/HILLS) — set FALSE for the GRASS layer
	 *  (pure decoration: NoCollision, no nav effect). Blocking layers ALSO honor
	 *  the keep-clear zones + reserved corridor; the non-blocking GRASS layer
	 *  ignores keep-clear (lush everywhere).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bBlocking = true;

	/** Density weighting across the arena Y band (see EScatterRegionBias). Never overrides keep-clear/corridor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	EScatterRegionBias RegionBias = EScatterRegionBias::WholeField;

	/** Minimum 2D spacing (cm) between two instances of THIS layer — rejection sampling drops candidates closer than this to an already-placed instance of the same layer. 0 = no spacing rule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	float MinSpacing = 300.f;

	/**
	 *  Vertical offset (cm) applied to placed instances after the ground trace —
	 *  a per-mesh origin correction for donor meshes whose pivot is not at the
	 *  base (art-director flags these in handoffs/TASK-135.md; build-master sets
	 *  the value on the DataAsset at TASK-137). 0 for base-pivot meshes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	float ZOffset = 0.f;

	// --- Radius-aware placement (CONVENTIONS "Climbable terrain (M6.6)", TASK-140) ---

	/**
	 *  2D footprint radius (cm) fed to the radius-aware placement guards — the
	 *  keep-clear inflation, the field-edge clamp, and the radius-aware MinSpacing —
	 *  so a WIDE instance centered off-lane no longer sprawls across the reserved
	 *  corridor or interpenetrates a neighbour (the M6.5 14–69-culls/seed root
	 *  cause). 0 (default) = AUTO-derive per instance from the chosen mesh's XY
	 *  bounds × the rolled uniform scale (`FVector2D(Bounds.BoxExtent.X, .Y).Size()
	 *  * Scale`). A value > 0 is an ABSOLUTE override (NOT multiplied by the
	 *  instance scale) for a designer who wants a fixed clearance regardless of the
	 *  scale-range variation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	float FootprintRadius = 0.f;

	// --- Tree collision-proxy (CONVENTIONS "Climbable terrain (M6.6)", TASK-140) ---

	/**
	 *  Optional invisible collision-proxy mesh. When SET, this layer's VISUAL HISM
	 *  carries NO collision + no navigation, and blocking is delegated to a PAIRED
	 *  proxy HISM (exactly one per visual mesh) that renders invisibly and blocks
	 *  the Pawn channel ONLY. Used for trees: the canopy/trunk still render, while a
	 *  slim engine `Cylinder` proxy blocks JUST the trunk footprint — so units route
	 *  around the trunk, not an 8 m canopy nav-blob (the defect that killed TASK-137).
	 *  Null (default) = the layer uses its own geometry for collision/nav per
	 *  bBlocking (rocks / slabs / hills / grass).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	TSoftObjectPtr<UStaticMesh> CollisionProxyMesh;

	/**
	 *  Non-uniform scale for the collision proxy, MULTIPLIED by the instance's
	 *  rolled uniform scale (so a tall thin trunk cylinder wraps a scaled tree).
	 *  Only used when CollisionProxyMesh is set. Default (1,1,1) = the proxy mesh's
	 *  authored size × the instance scale.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FVector CollisionProxyScale = FVector(1.f, 1.f, 1.f);

	/**
	 *  Vertical offset (cm) for the collision proxy, added to the placed instance Z
	 *  and scaled by the instance's uniform scale — raises a centered-pivot proxy
	 *  (e.g. the engine `Cylinder`, whose pivot is at its middle) so its base sits
	 *  at the tree base instead of half-sunk into the ground. Only used when
	 *  CollisionProxyMesh is set. Default 0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	float CollisionProxyZOffset = 0.f;

	// --- W1-PREP: hill-surface placement + material override (CONVENTIONS "Arena
	// --- 10× scale-up & LOD/perf (M7.6)" → "W1-PREP additions", TASK-250) ---

	/**
	 *  Opt-IN: this layer's instances may place ON hill surfaces. Default FALSE —
	 *  a non-opted layer keeps the flat-floor ground trace exactly as before (and
	 *  the hill layer itself must stay false: hills never stack on hills; a
	 *  false-layer also serves as a placement SURFACE for the opted-in layers when
	 *  it is a real-geometry blocker). When TRUE the layer is placed in a SECOND
	 *  pass (after every non-opted layer, so the hills exist to be traced), its
	 *  ground resolve accepts the elevated hill-surface Z, and candidates over a
	 *  hill face steeper than MaxPlacementSlopeDeg are rejected (never buried at
	 *  floor Z inside the hill — the W1-PREP bare-hills defect).
	 *  RECOMMENDED OPT-INS (DA wiring is TASK-249/251's side, not code): GRASS +
	 *  PLANTS first (non-blocking decoration — zero nav/corridor interaction, the
	 *  safe defaults); ROCKS + TREES also legal (blocking laws are UNCHANGED —
	 *  keep-clear/corridor tests still run on their 2D footprint, and their
	 *  nav-relevant HISMs still participate in the reachability validation +
	 *  corridor cull).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bAllowOnHills = false;

	/**
	 *  Max hill-face slope (degrees from horizontal) this layer tolerates when
	 *  bAllowOnHills is true: a candidate over a steeper face is rejected and
	 *  re-rolled. Default 35° — just past the ≤30° climbable-face law
	 *  (CONVENTIONS "Climbable terrain (M6.6)"), so props reach every walkable
	 *  face plus a small margin, while near-vertical flanks stay clean. Ignored
	 *  when bAllowOnHills is false.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0", ClampMax = "89", EditCondition = "bAllowOnHills"))
	float MaxPlacementSlopeDeg = 35.f;

	/**
	 *  Optional material override for this layer's HISMs (CONVENTIONS "W1-PREP
	 *  additions"): when SET, it replaces the donor materials on EVERY slot of the
	 *  layer's visual HISMs (+ the paired collision-proxy HISMs, per the law) via
	 *  SetMaterial at component creation — the SM_ assets themselves are NEVER
	 *  touched (the lane-clean route: main-lane donors stay pristine). Null
	 *  (default) = donor materials, a failed resolve degrades to the donor look
	 *  with a warning — never a crash. TASK-249's tri-planar M_HillGrass rides
	 *  this on the HILLS layer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	TSoftObjectPtr<UMaterialInterface> OverrideMaterial;

	// --- Cull bands + shadow casting (CONVENTIONS "Arena 10× scale-up & LOD/perf
	// --- (M7.6)" → scatter cull-field naming, TASK-284) — the per-layer LOD/perf
	// --- knobs that make the Phase-3 ≈4.9× density fill affordable. Applied at
	// --- HISM creation via SetCullDistances / SetCastShadow in
	// --- ResolveComponentForMesh() AND the tree collision-proxy path. These stay
	// --- DATA populated on DA_BattlefieldScatter at Phase 3; the defaults here are
	// --- the safe "no behavior change" fallbacks (never-cull + hill-style shadows),
	// --- so an unpopulated DA renders exactly as before this task. ---

	/**
	 *  Distance (uu) at which this layer's instances BEGIN to fade/cull — the near
	 *  edge of the cull band fed to UInstancedStaticMeshComponent::SetCullDistances
	 *  (InstanceStartCullDistance). 0 (default), paired with CullEndDistance=0,
	 *  means NEVER culled. Normally < CullEndDistance (the fade band); if it is
	 *  >= CullEndDistance the engine treats the band as a hard pop at CullEndDistance.
	 *  Phase 3 sets the real per-layer bands on the DA (plan §3 table).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Cull", meta = (ClampMin = "0"))
	int32 CullStartDistance = 0;

	/**
	 *  Distance (uu) beyond which this layer's instances are fully culled (not
	 *  drawn) — InstanceEndCullDistance. 0 (default) = NEVER culled (the safe
	 *  no-change fallback; Phase 3 populates the real bands per the plan §3 table).
	 *  Small/dense layers (grass, plants) take a short band so the far field is not
	 *  paying for invisible blades; hills take 0 (CONVENTIONS: "Hills: no cull" —
	 *  their silhouette must read across the 10× field).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Cull", meta = (ClampMin = "0"))
	int32 CullEndDistance = 0;

	/**
	 *  Whether this layer's VISUAL instances cast dynamic shadows (SetCastShadow on
	 *  the visual HISM in ResolveComponentForMesh). Defaults TRUE — the hill/obstacle
	 *  case (CONVENTIONS: "Hills: ... shadows ON (silhouette)"), mirroring bBlocking's
	 *  obstacle-default pattern, so an unpopulated DA keeps today's shadows. Set FALSE
	 *  on the GRASS / PLANTS layers in the DA (CONVENTIONS: "Grass/plants: shadows
	 *  OFF") — thousands of tiny casters are the costliest, least-visible shadows on
	 *  the field. NOTE: the invisible tree collision PROXY never casts a shadow
	 *  regardless of this flag — it is SetVisibility(false) + hard SetCastShadow(false)
	 *  by the proxy contract; this flag drives the VISIBLE tree/rock/hill HISM only.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Cull")
	bool bCastShadows = true;
};

/**
 *  Designer config for the runtime procedural battlefield scatter (CONVENTIONS
 *  "Battlefield & procedural terrain (M6.5)", TASK-134). A UDataAsset so the
 *  layer set + density + placement knobs are CONTENT, not code — the instance
 *  /Game/Data/DA_BattlefieldScatter is populated by build-master (TASK-137) from
 *  the art-director's curated Fab mesh list (TASK-135). ASiegeBattlefieldScatter
 *  reads it at match start; an unset/empty config is a graceful no-op.
 *
 *  Placement is 180°-ROTATIONALLY SYMMETRIC (TASK-358, Jonathan's 2026-08-01
 *  directive — SUPERSEDES the M6.5 asymmetric-organic ruling): every pass draws
 *  on the BLUE half (X <= 0) and emits the twin at (-X, -Y) with yaw+180 and an
 *  UNCHANGED scale. SymmetryMode below is the single global switch; the old
 *  bMirrorSymmetric X-mirror bool is RETIRED (it was a fake reflection).
 *
 *  The keep-clear radii + corridor half-width below are the DATA half of the
 *  NON-NEGOTIABLE traversability guarantee: blocking obstacles are excluded from
 *  the castle pads, the PlayerStart, and the reserved central lane, so a
 *  navigable Blue→Red path always exists. Jonathan can make the field
 *  denser/riskier at playtest by shrinking these. (The old per-team gold-node
 *  pads died with the W1-PREP mirrored-mines redesign, TASK-255 — mines are
 *  spawned BY the scatter itself, NoCollision, and clear their own aprons via
 *  the Scatter|Mines block below.)
 *
 *  The Scatter|AncientGrounds block (TASK-361) is the same idea one step further:
 *  the ancient-ground PAIR is placed by a scatter pass too — random per match, no
 *  L_Arena save, identical on host and client off the replicated seed — so those
 *  bands are DATA here rather than a hand-placed level actor.
 */
UCLASS(BlueprintType)
class GITCLAUDEUNREALTEST_API USiegeScatterConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	/** The scatter layers — TREES / ROCKS / HILLS / GRASS (order does not matter; each is placed independently). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Layers")
	TArray<FScatterLayer> Layers;

	/**
	 *  THE global terrain symmetry switch (TASK-358 — CONVENTIONS "Ancient Grounds
	 *  + Sorcerer + 180° terrain symmetry" §1). Default Rotational180 IS the law:
	 *  every terrain pass (layers, mines, and — TASK-361 — ancient grounds) draws
	 *  on the BLUE half (X <= 0) and emits the rotated twin at (-X, -Y), yaw+180,
	 *  scale unchanged. ONE field for the WHOLE scatter so the grep-able
	 *  `GenerateScatter seed=… mirror=…` reproducibility line can record it in a
	 *  single token.
	 *
	 *  ⚠️ Setting this to Asymmetric restores the RETIRED M6.5 organic-random
	 *  field. That is a FLAGGED decision that needs a NEW Jonathan ruling — his
	 *  2026-08-01 directive replaced the old asymmetric default verbatim.
	 *
	 *  This REPLACES `bool bMirrorSymmetric` (retired X-mirror). Any
	 *  DA_BattlefieldScatter that serialized that bool loses it silently on load
	 *  and takes this field's default — which is the intended end state (the
	 *  shipped DA had bMirrorSymmetric FALSE, i.e. the now-retired asymmetric
	 *  mode, so the default flip TO Rotational180 is exactly the behavior change
	 *  this task ships). Build-master: no DataAsset edit is required.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Placement")
	EScatterSymmetryMode SymmetryMode = EScatterSymmetryMode::Rotational180;

	/**
	 *  Half-extent (cm) of the rectangular scatter region on X (across the
	 *  castles) and Y (field width). M7.6 10× scale-up: default X=26,000 places a
	 *  little past the ±25,000 castles; Y=12,000 sits just inside the ±12,500
	 *  arena floor/walls. The actor clamps to this if the level bounds cannot be
	 *  resolved.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Bounds")
	FVector2D ArenaHalfExtent = FVector2D(26000.f, 12000.f);

	/** Keep-clear radius (cm) around EACH castle (±25000, M7.6) — no blocking obstacle lands inside, so a castle's mouth is never walled. Part of the traversability guarantee. MESH-RELATIVE (sized to the castle footprint, NOT ×3.125-scaled — M7.6 keep-list). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float CastleKeepClearRadius = 1500.f;

	/** Keep-clear radius (cm) around the PlayerStart / hero spawn (≈-23800,0, M7.6) — the hero never spawns inside an obstacle. Mesh-relative, not scaled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float PlayerStartKeepClearRadius = 800.f;

	/**
	 *  Half-width (cm) of the reserved central combat corridor: NO blocking
	 *  obstacle is placed within |Y| <= this across the WHOLE X span. This keeps
	 *  the straight Y≈0 lane between the two castles permanently walkable — the
	 *  deterministic core of the traversability guarantee (the nav reachability
	 *  check is the belt-and-suspenders confirmation on top). Grass ignores it.
	 *  M7.6 ruling #2 (Jonathan, 2026-07-18): 1,000 — a tight canyon on the 10×
	 *  field; this default and DA_BattlefieldScatter now AGREE (the old C++ 400 /
	 *  DA disagreement ends here).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|KeepClear", meta = (ClampMin = "0"))
	float CorridorHalfWidth = 1000.f;

	// --- Rotated depleting mines (W1-PREP, TASK-255 — CONVENTIONS "Mirrored
	// --- depleting mines", AMENDED 2026-08-01 by the 180°-rotational law
	// --- (TASK-358): these numbers are LAW there; tune bands recorded) ---

	/**
	 *  Neutral depleting mines spawned per SIDE each generate (total mines =
	 *  2 × this): each mine is drawn ONCE on the Blue half then rotated 180°
	 *  about the map center — (−X, −Y), yaw 180 (TASK-358; was the X-mirror
	 *  (−X, Y)). Castle-distance sums stay equal by construction (the fairness
	 *  law) because the castles are themselves an exact rotational pair. 0
	 *  disables the mines pass (debug fields only — the shipped default is 3 per
	 *  Jonathan's locked ruling).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0"))
	int32 MineCountPerSide = 3;

	/**
	 *  Minimum 2D center distance (cm) between mine PRIMARIES. The twin and
	 *  cross-pair distances are guaranteed ≥ this FOR FREE by the half-draw
	 *  construction (|X| ≥ max(MineClearanceRadius, this/2) — see PlaceMines),
	 *  so primaries are the only explicit spacing test. TASK-358: the 180°
	 *  rotation only STRENGTHENS both guarantees (dist(P,P′) = 2·|P| ≥ 2·|X|,
	 *  and the cross-pair X terms still ADD because both primaries sit on the
	 *  Blue half) — the proof in PlaceMines is unchanged. Default 3,000 — larger
	 *  than the biggest hill diameter, so two mine sites never share a mound.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0"))
	float MineMinSpacing = 3000.f;

	/**
	 *  Clearance disc radius (cm) enforced around EACH mine of a pair: candidate
	 *  points must keep this disc out of the castle/PlayerStart keep-clear zones,
	 *  and every nav-relevant blocker inside it is DELETED at placement (the
	 *  apron + miner walk-in guarantee — the old GoldNodeKeepClearRadius reborn
	 *  as an ACTIVE clearance; hills exempt by the never-delete-hills rule, grass
	 *  untouched). Also the base radius of the per-mine widening reachability
	 *  cull in ValidateTraversability.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0"))
	float MineClearanceRadius = 600.f;

	/**
	 *  Gold reserve each spawned mine is InitMine()'d with. CONVENTIONS default
	 *  300 (3 miners dry a mine in ~100 s); tune band 250–450 — raise to 450
	 *  FIRST if playtest says matches stall (the all-depleted pacing lever).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0"))
	int32 MineGoldReserve = 300;

	/** Margin (cm) inset from the arena half-extents when drawing mine centers, so a mine's clearance disc never pokes past the field edge into the boundary walls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0"))
	float MineEdgeMargin = 600.f;

	/**
	 *  Max hill-face slope (degrees from horizontal) a mine candidate tolerates
	 *  at EITHER point of its pair — a steeper face at P or P′ re-rolls the
	 *  candidate. 30° = the climbable-face law (CONVENTIONS "Climbable terrain
	 *  (M6.6)"): miners must be able to WALK onto every mine. The deterministic
	 *  fallback slot ignores this gate (it must always seat — economy law).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines", meta = (ClampMin = "0", ClampMax = "89"))
	float MineMaxSlopeDeg = 30.f;

	/**
	 *  Mine actor class PlaceMines spawns; null (default) ⇒ AGoldNode (the
	 *  neutral depleting claimable mine, TASK-253). A subclass hook for a future
	 *  BP/child variant — never a different archetype (the pass calls InitMine
	 *  on it, so it must BE an AGoldNode).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|Mines")
	TSubclassOf<AGoldNode> MineClass;

	// --- ANCIENT GROUNDS (batch ANCIENT-GROUNDS, TASK-361 — CONVENTIONS "Ancient
	// --- Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" §2 "Placement",
	// --- plan §2). The defaults below ARE the law (every one of them is also a
	// --- FLAGGED tunable). PlaceAncientGrounds draws ONE ground on the Blue half
	// --- from these bands and emits its 180° rotational twin at (−X, −Y) — the
	// --- §1 law, so the pass needs no symmetry exception. ---

	/**
	 *  XY half-extent (cm) of an ancient ground's boost footprint — (840, 840) =
	 *  "the size of the mid capture zone" (Jonathan's 2026-08-01 directive).
	 *
	 *  ⚠️ PAIRED TUNABLE, THREE WAYS: this value must equal BOTH
	 *  AAncientGround::ZoneHalfExtent (the actor's own mechanic box + decal size)
	 *  AND ACaptureZone::ZoneHalfExtent. The actor owns the MECHANIC copy; this
	 *  one is the SCATTER's copy — it is what the placement pass uses to keep the
	 *  whole footprint inside the arena (the MaxAbsX/MaxAbsY margin clamp), which
	 *  is a placement concern the actor cannot answer. PlaceAncientGrounds
	 *  compares the two after spawn and logs a Warning on divergence, so the
	 *  duplication can never drift silently. Change all three together.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds")
	FVector2D AncientGroundHalfExtent = FVector2D(840.f, 840.f);

	/**
	 *  Minimum |X| (cm) of an ancient ground's center. 4,000 leaves 2,320 uu clear
	 *  between the mid capture zone (half-extent 840 at the origin) and the
	 *  nearest legal ground edge, so the two decals never touch and the ancient
	 *  ground never reads as a second capture zone.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds", meta = (ClampMin = "0"))
	float AncientGroundMinAbsX = 4000.f;

	/**
	 *  Maximum |X| (cm) of an ancient ground's center. 21,000 keeps the objective
	 *  out of the deep back-field: the castles sit at ±25,000 and the unit spawn
	 *  boxes start at |X| = 22,540, so a ground is always at least 1,540 uu in
	 *  FRONT of the spawn boxes — you fight over it, you do not spawn on it.
	 *  (Additionally clamped down at runtime, if ever needed, so the 840-half
	 *  footprint stays inside ArenaHalfExtent.X — a no-op at these defaults.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds", meta = (ClampMin = "0"))
	float AncientGroundMaxAbsX = 21000.f;

	/**
	 *  Maximum |Y| (cm) of an ancient ground's center: ArenaHalfExtent.Y (12,000)
	 *  − a 1,200 margin. The footprint edge then lands at 11,640 — inside the
	 *  ±12,500 arena ground and well inside the ±13,888 navmesh bounds, so the
	 *  runes never bleed off the playfield onto the boundary walls.
	 *  ⚠️ The RESERVED CORRIDOR (|Y| <= CorridorHalfWidth) is deliberately NOT
	 *  excluded — the SAME ruling as the mines: AAncientGround has no collision
	 *  primitive and no nav geometry, so it cannot touch the traversability
	 *  guarantee, and a lane objective is good contested design. Do not "fix" it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds", meta = (ClampMin = "0"))
	float AncientGroundMaxAbsY = 10800.f;

	/**
	 *  Minimum 2D center distance (cm) an ancient ground keeps from EVERY spawned
	 *  mine, tested at BOTH ends of the pair (P and P′). 1,800 ≈ the ground's own
	 *  1,188 half-diagonal plus the mine's 600 clearance disc, so a gathering box
	 *  and a mining apron never overlap — the two objectives stay legible as
	 *  separate things to fight over. (The mines pass runs FIRST, so the live
	 *  mine set is exactly what this is tested against.)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds", meta = (ClampMin = "0"))
	float AncientGroundMineClear = 1800.f;

	/**
	 *  Radius (cm) of the blocker-clearance disc deleted around EACH end of the
	 *  ancient-ground pair (RemoveBlockingInstancesInDisc at P and at P′), so
	 *  units can actually gather on the runes instead of standing in a thicket.
	 *  1,200 circumscribes the 840×840 half-extent box (half-diagonal 1,188), so
	 *  the whole footprint plus a hair is cleared. Hills are EXEMPT from that cull
	 *  by the never-delete-hills rule — which costs nothing here, because a
	 *  candidate over a hill is rejected outright (flat ground only).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter|AncientGrounds", meta = (ClampMin = "0"))
	float AncientGroundClearRadius = 1200.f;
};
