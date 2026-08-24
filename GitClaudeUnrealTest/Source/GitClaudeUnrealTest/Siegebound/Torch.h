// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/Color.h"
#include "UObject/SoftObjectPtr.h"
#include "Torch.generated.h"

class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 *  Siegebound INTERIOR TORCH (batch WAR ROOM, TASK-558 — Jonathan's directive
 *  "add some lighting in the middle of it maybe via torches"; law: CONVENTIONS
 *  WR-§4). A wall-mounted light prop and NOTHING else: one static mesh (bracket
 *  + shaft + fire bowl) and one shadowless point light. No tick, no timer, no
 *  collision, no navmesh footprint, no gameplay state, no replicated truth.
 *
 *  ⛔ NEVER LEVEL-PLACED. ACastle spawns and attaches its own torches at
 *  BeginPlay from TorchAnchors and destroys them with itself (TASK-562, WR-§4's
 *  placement + teardown clauses). Three load-bearing reasons live in that task's
 *  header: L_Arena is under standing orders never to be saved; both castles get
 *  identical furnishing by construction; the anchors travel with the castle
 *  actor forever. This class has NO opinion about where it is placed — it is a
 *  prop that lights whatever room it is put in, and TASK-562 is its only caller.
 *
 *  ⛔⛔ LIGHT MOBILITY = MOVABLE, CastShadows = false. THIS IS NOT A PREFERENCE
 *  AND THE REASON IS STRUCTURAL (WR-§4):
 *    - A RUNTIME-SPAWNED light CANNOT use a baked static shadow map. It does not
 *      exist when lighting is built, so there is no lightmap entry for it and no
 *      static shadow to sample; Static mobility would give a torch that emits
 *      nothing at all.
 *    - The only Static/Stationary path is a lighting build PLUS a save of
 *      L_Arena, which is exactly what WR-§3 forbids (the one-time nav-data-only
 *      save exception was granted for TASK-350 and EXPIRED at that commit).
 *    - SHADOWS OFF IS WHAT MAKES MOVABLE CHEAP. A movable point light with no
 *      shadow casting costs a light-volume pass and no shadow-depth rendering;
 *      the shadow-depth pass is the expensive half, and it is the half that
 *      would scale with MaxTorchesPerCastle × 2 castles.
 *
 *  ⚠️ FLAGGED FOR JONATHAN — DECISION D3 (torch light cost) IS NOT RULED. The
 *  static-vs-stationary-vs-movable question is his, and NO FPS BASELINE EXISTS
 *  FOR THIS PROJECT (M7's TASK-183 never ran), so this class quotes no frame
 *  rate and claims no measurement (WR-§4 perf clause). What ships is the CHEAP
 *  default the law names, and it stays cheap to flip: mobility is one line in
 *  the constructor, shadow casting is one line in ApplyTorchLightTuning(), and
 *  the three feel tunables below are EditDefaultsOnly so BP_Torch can be tuned
 *  without a recompile.
 *  ⚠️ AND THE FLIP IS NOT FREE, WHICH IS THE PART WORTH KNOWING BEFORE RULING:
 *  ULocalLightComponent::SetAttenuationRadius calls
 *  AreDynamicDataChangesAllowed with bIgnoreStationary = false, so on a
 *  REGISTERED Stationary or Static component it SILENTLY NO-OPS. Under either
 *  of those mobilities the BeginPlay re-apply of TorchAttenuationRadius stops
 *  working (the constructor-time apply still lands, because the component is not
 *  registered yet) — a designer would tune the radius and see nothing change at
 *  runtime. Movable is the only mobility for which every tunable below is
 *  honestly live.
 *
 *  ⚖️ NET RELEVANCY TIER: **C — NOT REPLICATED** (declared per the CONVENTIONS
 *  NET RELEVANCY LAW declaration duty and WR-§8; "there is nothing to declare"
 *  only counts when it is stated). `bReplicates` stays at the AActor default
 *  (false) and is never set. This class adds NO replicated property, NO new
 *  replicated class, NO new relevancy tier and NO RPC. Rationale, and it is the
 *  AAncientGround / AGoldNode precedent exactly: a torch holds no gameplay
 *  truth. Both machines build an identical set locally because ACastle spawns
 *  them from its own EditDefaultsOnly anchors on both machines — a pure local
 *  projection of already-replicated castle state, so replicating the prop would
 *  buy nothing and cost bandwidth.
 *
 *  ⛔ NO NIAGARA, BY RULING (WR-§4 flame clause). v1's flame IS the emissive
 *  material on the mesh's slot 1 (/Game/Materials/M_TorchFlame, authored by
 *  TASK-556) plus this point light. A Niagara NS_TorchFlame is RECORDED AS A
 *  FOLLOW-UP and is deliberately not started here: MCP Niagara authoring is a
 *  known-limited lane, and a flicker particle is not what the directive asked
 *  for. This file references no Niagara type.
 *
 *  ASSET CONTRACT — /Game/Meshes/SM_Torch, soft-referenced and null-safe. The
 *  mesh is authored by TASK-556 and imported by TASK-566, so at the moment this
 *  class is written the asset DOES NOT EXIST YET. A failed resolve logs exactly
 *  once and leaves an inert, invisible actor that still lights the room — never
 *  a crash, never an ensure, never a repeated log line (the AGoldNode
 *  deferred-asset pattern). Clearing TorchMeshAsset in a child/instance is a
 *  silent designer opt-out (the AttackImpactEffect IsNull pattern, TASK-020).
 *
 *  ⚠️ ONE DECLARED ADDITION OVER THE TASK SPEC (SC-§15 — declared, not smuggled):
 *  the spec names three tunables (TorchIntensity, TorchAttenuationRadius,
 *  TorchLightColor) and this class ships a FOURTH EditDefaultsOnly field,
 *  TorchLightRelativeOffset. It is not a feel knob — it is a CROSS-TASK SEAM,
 *  and its own doc comment carries the whole reason. Short version: TASK-556
 *  MEASURES where this light belongs and says so in its script; that measurement
 *  must be able to land without a recompile and without this file transcribing a
 *  number owned by a task still in flight. Default zero = derive from the asset,
 *  so the shipped behaviour is unchanged if nobody ever sets it.
 *
 *  COLLISION: NONE, DELIBERATELY. NoCollision profile, no overlap events, no
 *  navigation relevance. A wall torch that blocked would be a hazard inside the
 *  one interior units must be able to walk through, and it would carve the
 *  navmesh of the room it exists to light. SetCanBeDamaged(false) — scenery is
 *  not a combatant, and this actor deliberately does NOT implement ITeamAgent
 *  (that interface is what unit acquisition scans; implementing it would make
 *  enemy units walk into a castle to attack the furniture). Environment props
 *  are team-NEUTRAL (the M4.5 precedent) — there is no Team member here.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ATorch : public AActor
{
	GENERATED_BODY()

public:

	ATorch();

	/** Editor-time mesh resolve + light preview so a placed/previewed BP_Torch looks right in the viewport (null-safe, silent — AGoldNode::OnConstruction precedent). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Runtime mesh resolve (warns once if missing) + the tunable re-apply, so an editor-tweaked value takes effect without a restart. */
	virtual void BeginPlay() override;

	/**
	 *  Root visual: the bracket + shaft + fire bowl. Mesh comes from
	 *  TorchMeshAsset — never hard-referenced.
	 *
	 *  ⚠️ THE MESH'S ORIGIN IS ITS WALL-MOUNT FACE (TASK-556 / WR-§4), not its
	 *  floor contact. That is load-bearing for TASK-562: a torch anchor is a
	 *  point ON A WALL with the transform's rotation facing into the room, and a
	 *  floor-origin torch would bury itself in the masonry.
	 *
	 *  Mobility stays Movable (the USceneComponent default): a Static-mobility
	 *  component refuses SetStaticMesh once the world has begun play, which would
	 *  break the deferred-asset runtime resolve below.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Torch")
	TObjectPtr<UStaticMeshComponent> TorchMesh;

	/**
	 *  The interior light. ⛔ Movable + CastShadows = false by law — see the
	 *  class doc for why neither is a preference.
	 *
	 *  Its relative location is DERIVED FROM THE RESOLVED MESH rather than typed
	 *  as a number (SC-§34's structural escape: "derive at runtime from the asset
	 *  instead of transcribing a number" — a value read from the bounds cannot go
	 *  stale when TASK-556 re-authors the torch), unless TorchLightRelativeOffset
	 *  is set, which overrides it outright. See PositionLightAtFireBowl().
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Torch")
	TObjectPtr<UPointLightComponent> TorchLight;

	/**
	 *  Light intensity IN CANDELAS (the unit is asserted explicitly in
	 *  ApplyTorchLightTuning — ULocalLightComponent's own default is
	 *  ELightUnits::Unitless, so a candela number written into a unitless field
	 *  would be ~625× too dim and would read as "the torches do not work").
	 *
	 *  ⚠️ FEEL TUNABLE, FLAGGED FOR JONATHAN'S PLAYTEST PASS (TASK-571). The
	 *  ARITHMETIC is derived; the FEEL is not measured and is not claimed to be.
	 *  Derivation, so the number is checkable rather than invented: the engine's
	 *  own point-light default is 5000 UNITLESS, and UPointLightComponent::
	 *  ComputeLightBrightness scales unitless by ×16 and candelas by ×10000
	 *  (100 cm² → m²) ⇒ the engine default is EXACTLY 8 cd at a 1000 uu
	 *  attenuation radius. This torch's radius is 1200 uu, and illuminance falls
	 *  off as 1/d², so preserving the engine default's brightness at the
	 *  attenuation edge takes 8 × (1200/1000)² = 11.52 cd, rounded to 12.
	 *  ⇒ "as bright as an engine-default point light, at a 1.2× bigger radius."
	 *  ⚠️ STATED PRECISELY, BECAUSE IT IS EASY TO OVER-READ: what that preserves
	 *  is the brightness AT THE EDGE of the pool. Closer in, this torch is 1.44×
	 *  an engine-default point light — deliberately, since the fire bowl is meant
	 *  to read as a source, not as ambient fill. Raise or lower it if the hall
	 *  reads wrong at gameplay camera distance; that is his call, and no frame
	 *  rate or luminance has been measured here to support any other number.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Torch", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "100.0"))
	float TorchIntensity = 12.0f;

	/**
	 *  Attenuation radius in uu — 1200 by law (WR-§4 tunables row).
	 *
	 *  ⚠️ FEEL TUNABLE, FLAGGED FOR JONATHAN. Scale sanity, because it is easy to
	 *  misread: the AS-BUILT grand hall is 2910 × 1140 uu with 1986 uu clear
	 *  height — flat floor z 174 end-to-end, flat ceiling z 2160, MEASURED in
	 *  handoffs/TASK-629-artist.md (WR-§1's 1560 is the design MINIMUM the build
	 *  clears, not the ceiling) — so 1200 uu is a pool of light around each
	 *  torch rather than whole-room coverage — the room is lit by SEVERAL
	 *  torches, which is what MaxTorchesPerCastle (TASK-562) exists to bound.
	 *  (TASK-637 comment rider, GH-R13: the old "≈2910 × 720 / ≈1560 clear"
	 *  figures here were the pre-redesign carve, stale since the TASK-629
	 *  interior rebuild; the pool-not-floodlight argument only got STRONGER in
	 *  the deeper hall.)
	 *  ⛔ NOT a WR-§2 ledger row: this constant is BORN at the 9× scale, it was
	 *  not derived from the old castle's size, so there is nothing stale in it.
	 *  TASK-557 owns the re-derivation ledger; this task adds no row to it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Torch", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float TorchAttenuationRadius = 1200.0f;

	/**
	 *  Warm firelight colour (1.00, 0.72, 0.42) per WR-§4.
	 *
	 *  ⚠️ FEEL TUNABLE, FLAGGED FOR JONATHAN. Applied through
	 *  ULightComponent::SetLightColor with bSRGB = true, which is the
	 *  round-trip-stable choice: the component stores an FColor, and
	 *  ULightComponentBase::GetLightColor reads it back through the sRGB-decoding
	 *  FLinearColor(FColor) constructor — so encoding on the way in means the
	 *  value a designer sees in the details panel is the value written here, and
	 *  the light's LINEAR colour is the authored triplet.
	 *  ⛔ NOT the shipped team palette and NOT a team tint: environment props are
	 *  NEUTRAL (the M4.5 precedent). BlueBarColor / RedBarColor are the combatant
	 *  health-bar palette and have no business here.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Torch")
	FLinearColor TorchLightColor = FLinearColor(1.00f, 0.72f, 0.42f, 1.0f);

	/**
	 *  Where TorchLight sits, relative to the mesh. ⭐ ZERO (the default) MEANS
	 *  "DERIVE IT FROM THE RESOLVED MESH" — see PositionLightAtFireBowl(). A
	 *  non-zero value is used verbatim and the derivation is skipped.
	 *
	 *  ⚠️ THIS FIELD EXISTS FOR ONE NAMED REASON, AND IT IS A CROSS-TASK SEAM,
	 *  NOT A KNOB SOMEBODY WANTED. TASK-556 authors SM_Torch procedurally and
	 *  MEASURES the flame's own bounding-box centre, recording it as
	 *  `flame_centre_uu` — its script says in terms that this is "the point
	 *  ATorch::TorchLight should sit at". That number is a MEASUREMENT owned by
	 *  the art lane, it is not final until that task's handoff publishes it, and
	 *  ⛔ it is deliberately NOT transcribed here: reading an in-flight parallel
	 *  task's working file as a source of truth is exactly the coupling the
	 *  pipeline forbids. So the shipped default derives an approximation from the
	 *  whole mesh's bounds (no invented number, self-maintaining), and THIS FIELD
	 *  is where the measured value lands at integration — a BP_Torch default
	 *  edit, no recompile, no code change.
	 *  ⚠️ THE RESIDUAL, STATED SO NOBODY HAS TO REDISCOVER IT: the bounds
	 *  derivation puts the light above and behind the flame rather than inside
	 *  it. Against the torch geometry that exists today that is a ~32 uu error on
	 *  a 1200 uu attenuation radius (≈2.6 %), and the flame reads regardless
	 *  because it is EMISSIVE and this light casts no shadows. It is a polish
	 *  item, not a defect.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Torch")
	FVector TorchLightRelativeOffset = FVector::ZeroVector;

	/**
	 *  Soft reference to the torch visual: /Game/Meshes/SM_Torch (slot 0 =
	 *  M_Torch, slot 1 = the emissive M_TorchFlame, both baked into the imported
	 *  asset by TASK-566). Null-safe on load; clearing it in a child/instance is
	 *  a silent designer opt-out.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Torch")
	TSoftObjectPtr<UStaticMesh> TorchMeshAsset;

private:

	/**
	 *  THE SINGLE WRITER of every light tunable, called from BOTH the constructor
	 *  AND BeginPlay (and from OnConstruction for the editor preview) — the
	 *  AHeroCharacter::ApplyTerrainMovementTuning / ApplyMovementSpeed precedent,
	 *  so a BP_Torch tweak takes effect without a restart instead of being frozen
	 *  at whatever the C++ constructor happened to set.
	 *
	 *  ⛔ CastShadows = false is asserted HERE, every time, not just once in the
	 *  constructor: it is a LAW (WR-§4), not a designer preference, so a ticked
	 *  "Cast Shadows" box on BP_Torch is deliberately overridden rather than
	 *  honoured. If D3 ever rules the other way, THIS is the one line that flips.
	 */
	void ApplyTorchLightTuning();

	/**
	 *  Loads TorchMeshAsset onto TorchMesh if resolvable. bWarnIfMissing is a
	 *  REQUIRED parameter, never a trailing default (SC-§33's structural
	 *  preference, and the reason is the law's own — a required
	 *  parameter is audited by the compiler, which is exactly the property a
	 *  default trades away); it is true only on the BeginPlay path, because
	 *  OnConstruction re-runs on every editor property tweak and warning there
	 *  would spam the log.
	 */
	void ResolveTorchMesh(bool bWarnIfMissing);

	/**
	 *  Puts TorchLight at the fire bowl WITHOUT INVENTING A NUMBER
	 *  (SC-§34's structural escape). A non-zero TorchLightRelativeOffset wins
	 *  outright — that is the seam a MEASURED value lands in. Otherwise the
	 *  bowl is approximated from the resolved mesh: the bowl is the top of a wall
	 *  bracket whose origin is the wall-mount face, so XY = the local
	 *  bounding-box centre (which is out into the room, because the mesh extends
	 *  away from the wall it is mounted on), Z = the top of that box.
	 *
	 *  ⇒ When TASK-556 re-authors SM_Torch at a different size, the light follows
	 *  the art with no code change and no stale transcribed offset. A missing or
	 *  unresolved mesh leaves the light at the component default (0,0,0) — i.e.
	 *  at the wall-mount point, which still lights the room because shadow casting
	 *  is off. Silent: an unresolved mesh is already reported once by
	 *  ResolveTorchMesh, and a second log line for the same cause is noise.
	 *
	 *  Called as a PEER STEP after ResolveTorchMesh rather than from inside it,
	 *  so it also covers a mesh a designer assigned directly on the component in
	 *  BP_Torch (with TorchMeshAsset cleared — the supported opt-out path).
	 */
	void PositionLightAtFireBowl();

	/** One-shot guard for the missing-mesh warning (the log fires once per actor, never per frame and never per construction). */
	bool bWarnedMissingMesh = false;
};
