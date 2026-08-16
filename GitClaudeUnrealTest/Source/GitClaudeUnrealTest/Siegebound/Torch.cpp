// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Torch.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "GitClaudeUnrealTest.h"

ATorch::ATorch()
{
	// No per-frame work and no timer either: a torch is a mesh and a light, and
	// neither of them changes after BeginPlay. The v1 flame is the emissive
	// slot-1 material, NOT an animated flicker (WR-§4 defers Niagara), so there
	// is nothing for a tick to drive. (TASK-004 never-per-tick law.)
	PrimaryActorTick.bCanEverTick = false;

	// ⚖️ NET RELEVANCY TIER C — NOT REPLICATED. bReplicates is deliberately left
	// at the AActor default (false) and is never touched: ACastle spawns an
	// identical set on both machines from its own anchors, so a torch is a pure
	// local projection of already-replicated castle state. See the class doc.
	// This class adds no replicated property, no new replicated class, no new
	// relevancy tier and no RPC (WR-§8).

	// Scenery, not a combatant: ApplyDamage routes are refused engine-side
	// before any TakeDamage runs. Deliberately does NOT implement ITeamAgent —
	// unit acquisition scans ITeamAgent actors, so implementing it would send
	// enemy units into a castle to attack the furniture.
	SetCanBeDamaged(false);

	// Root visual. NoCollision + no overlaps + no navigation relevance: a wall
	// torch must never block the interior units have to walk through, and must
	// never carve the navmesh of the room it exists to light.
	// Mobility stays Movable (the USceneComponent default): a Static-mobility
	// component refuses SetStaticMesh once the world has begun play, which would
	// break the deferred-asset resolve in BeginPlay below.
	TorchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TorchMesh"));
	SetRootComponent(TorchMesh);
	TorchMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	TorchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TorchMesh->SetGenerateOverlapEvents(false);
	TorchMesh->SetCanEverAffectNavigation(false);

	// The interior light, attached to the mesh so it travels with the prop when
	// ACastle attaches the whole actor to CastleMesh at a TorchAnchors transform.
	TorchLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TorchLight"));
	TorchLight->SetupAttachment(TorchMesh);

	// ⛔⛔ MOVABLE, BY LAW (WR-§4), AND THE REASON IS STRUCTURAL: this light is
	// SPAWNED AT RUNTIME by ACastle, so it does not exist when lighting is built
	// and has no lightmap entry to sample — a Static light here would emit
	// nothing at all. The only Static/Stationary path is a lighting build plus a
	// save of L_Arena, which WR-§3 forbids outright.
	// ⚠️ MOBILITY IS SET HERE AND ONLY HERE, deliberately: it is a registration-
	// time property, not a runtime tunable, so it does NOT belong in the
	// ctor+BeginPlay re-apply helper below. If D3 ever rules otherwise, this is
	// the one line that flips (and read the class doc's warning about
	// SetAttenuationRadius silently no-opping under Stationary/Static first).
	TorchLight->SetMobility(EComponentMobility::Movable);

	// Visual contract: soft path, never a hard reference. SM_Torch is authored by
	// TASK-556 and imported by TASK-566 — it does not exist yet, and this actor
	// resolves it null-safely and survives its absence.
	TorchMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_Torch.SM_Torch")));

	// Constructor half of the ctor -> BeginPlay re-apply pattern
	// (AHeroCharacter::ApplyTerrainMovementTuning precedent).
	ApplyTorchLightTuning();
}

void ATorch::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Editor-time resolve so a placed/previewed BP_Torch is visible and lit in
	// the viewport. Silent: OnConstruction re-runs on every editor property
	// tweak, so warning here would spam (AGoldNode::OnConstruction precedent).
	ResolveTorchMesh(/*bWarnIfMissing=*/ false);

	// Bounds may have just become known (or changed): re-derive the light
	// position from the art rather than from a transcribed offset.
	PositionLightAtFireBowl();

	// Re-apply so an edited BP_Torch default previews immediately instead of
	// showing whatever the C++ constructor set.
	ApplyTorchLightTuning();
}

void ATorch::BeginPlay()
{
	Super::BeginPlay();

	// Runtime resolve for torches spawned before SM_Torch existed (and for
	// level-loaded actors, which do not re-run construction scripts). This is
	// the ONE path that warns, and it warns at most once per actor.
	ResolveTorchMesh(/*bWarnIfMissing=*/ true);

	// Derived from whatever mesh actually ended up on the component — including
	// one a designer assigned directly in BP_Torch instead of through
	// TorchMeshAsset, which is why this is a peer step and not a branch inside
	// ResolveTorchMesh.
	PositionLightAtFireBowl();

	// BeginPlay half of the re-apply pattern: a BP_Torch tweak takes effect
	// without a restart instead of being frozen at the constructor's values.
	ApplyTorchLightTuning();
}

void ATorch::ApplyTorchLightTuning()
{
	if (!TorchLight)
	{
		return;
	}

	// ⛔ UNITS FIRST, AND IT IS NOT COSMETIC: ULocalLightComponent's constructor
	// leaves IntensityUnits at ELightUnits::Unitless, and
	// UPointLightComponent::ComputeLightBrightness scales unitless by 16 and
	// candelas by 10000. Writing a candela number into a unitless field would
	// therefore render it ~625x too dim, which presents as "the torches do not
	// work" rather than as a units bug. TorchIntensity is documented in candelas,
	// so the component is told so explicitly.
	TorchLight->SetIntensityUnits(ELightUnits::Candelas);
	TorchLight->SetIntensity(TorchIntensity);

	// ⚠️ Under Movable this always lands. Under Stationary/Static on a REGISTERED
	// component it silently no-ops (AreDynamicDataChangesAllowed with
	// bIgnoreStationary = false) — see the class doc's D3 note.
	TorchLight->SetAttenuationRadius(TorchAttenuationRadius);

	// bSRGB = true is the round-trip-stable choice: the component stores an
	// FColor and reads it back through the sRGB-decoding FLinearColor(FColor)
	// constructor, so the light's LINEAR colour ends up as the authored triplet
	// and the details-panel swatch matches what was written here.
	TorchLight->SetLightColor(TorchLightColor, /*bSRGB=*/ true);

	// ⛔⛔ SHADOWS OFF IS THE LAW (WR-§4), NOT A DESIGNER PREFERENCE, so it is
	// asserted on EVERY apply rather than once in the constructor: a ticked
	// "Cast Shadows" box on BP_Torch is deliberately overridden. Shadows off is
	// what makes Movable cheap — the shadow-depth pass is the expensive half,
	// and it is the half that would scale with MaxTorchesPerCastle x 2 castles.
	// ⛔ THIS IS THE ONE LINE D3 FLIPS.
	TorchLight->SetCastShadows(false);
}

void ATorch::ResolveTorchMesh(bool bWarnIfMissing)
{
	// Cleared-in-editor (IsNull) is a silent designer opt-out — the
	// AttackImpactEffect pattern (TASK-020). A mesh-less torch is still a valid
	// light source, so this is a supported state, not a failure.
	if (!TorchMesh || TorchMeshAsset.IsNull())
	{
		return;
	}

	if (UStaticMesh* LoadedMesh = TorchMeshAsset.LoadSynchronous())
	{
		// SetStaticMesh self-no-ops on the same mesh; the explicit check keeps
		// repeated OnConstruction runs from touching the render state at all.
		if (TorchMesh->GetStaticMesh() != LoadedMesh)
		{
			TorchMesh->SetStaticMesh(LoadedMesh);
		}
	}
	else if (bWarnIfMissing && !bWarnedMissingMesh)
	{
		// Once per actor, by design: SM_Torch is authored by TASK-556 and
		// imported by TASK-566, so a missing asset is an EXPECTED intermediate
		// state of this repository, not a defect to shout about every frame.
		bWarnedMissingMesh = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ATorch '%s': mesh '%s' failed to load — the torch is invisible but still lights the room (light unaffected)."),
			*GetNameSafe(this), *TorchMeshAsset.ToString());
	}
}

void ATorch::PositionLightAtFireBowl()
{
	// SC-§34's structural escape, applied: DERIVE THE OFFSET FROM THE ASSET,
	// never transcribe one. SM_Torch's origin is its WALL-MOUNT FACE (TASK-556 /
	// WR-§4), so the mesh extends away from the wall and upward; its local
	// bounding box therefore already describes where the fire bowl is:
	//   XY = the box centre  -> out into the room, off the wall plane
	//   Z  = the box maximum -> the top of the shaft, i.e. the bowl
	// When TASK-556 re-authors the torch at a different size, the light follows
	// the art with no code change and nothing to go stale.
	if (!TorchLight || !TorchMesh)
	{
		return;
	}

	// ⭐ THE MEASURED VALUE WINS. TASK-556 measures the flame's own bbox centre
	// (`flame_centre_uu`) and names it as this light's home; that number lands
	// here as a BP_Torch default at integration, with no recompile. Until then
	// the field is zero and the derivation below runs. ⛔ The measurement is NOT
	// transcribed into this file — it belongs to a task that has not published
	// its handoff yet, and copying it now would freeze a number that can still
	// change.
	if (!TorchLightRelativeOffset.IsNearlyZero())
	{
		TorchLight->SetRelativeLocation(TorchLightRelativeOffset);
		return;
	}

	const UStaticMesh* const ResolvedMesh = TorchMesh->GetStaticMesh();
	if (!ResolvedMesh)
	{
		// No bounds to derive from. The light stays at the component default
		// (0,0,0) = the wall-mount point, which still lights the room because
		// shadow casting is off. Silent — ResolveTorchMesh already reported the
		// missing asset once, and a second line for the same cause is noise.
		return;
	}

	const FBox LocalBounds = ResolvedMesh->GetBoundingBox();
	if (!LocalBounds.IsValid)
	{
		// A degenerate/empty bounding box would produce a meaningless centre;
		// leave the light where it is rather than move it somewhere invented.
		return;
	}

	const FVector BoxCentre = LocalBounds.GetCenter();
	TorchLight->SetRelativeLocation(FVector(BoxCentre.X, BoxCentre.Y, LocalBounds.Max.Z));
}
