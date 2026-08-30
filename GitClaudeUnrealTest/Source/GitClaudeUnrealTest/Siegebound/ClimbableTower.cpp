// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/ClimbableTower.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/SiegeNavAreas.h"

// ⛔⛔ THE INCLUDE LIST IS PART OF THE CONTRACT (HIGH-§3 / TOWER-§ (6)): there is
// deliberately NO "Siegebound/SummonedUnit.h" and NO "Siegebound/Tower.h" here.
// This class never learns that a unit is standing on it, never calls
// HeightAdvantageMultiplier, and shares no code with the auto-firing tower
// family. A hill and a tower at the same Z must deal identical damage, and the
// cheapest guarantee of that is that only ONE rule exists and it lives entirely
// in ASummonedUnit.

AClimbableTower::AClimbableTower()
{
	// ⛔ NOTHING is added to the base's behaviour here beyond the team gate. No
	// tick (ABuilding sets bCanEverTick false and this class keeps it), no timer,
	// no target acquisition, no projectile — TOWER-§3's row ships
	// Damage/Range/Cadence = 0/0/0 and this class has no fire path to arm even if
	// it did not.

	// TEAM GATE, physical lane — the ACastle::GateBlockerVolume pattern
	// (Castle.cpp:300-306) verbatim: create it INERT and let BeginPlay do all live
	// configuration, once Team is authoritative. An editor-placed tower therefore
	// blocks nothing until play, and a deferred-spawned one (InitBuilding →
	// FinishSpawning, the TASK-030 flow) arms with the team it was actually given.
	AscentGateVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AscentGateVolume"));
	AscentGateVolume->SetupAttachment(VisualMesh);
	AscentGateVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AscentGateVolume->SetGenerateOverlapEvents(false);

	// ⛔⛔ THE PHYSICAL LANE MUST NEVER TOUCH NAVIGATION. If this box were
	// navigation-relevant it would carve a 3,000-uu hole out of the battlefield
	// navmesh and strand units — the exact NAV-§ failure this design exists to
	// avoid. The navmesh is ABuilding's VisualMesh's business and nothing else's.
	AscentGateVolume->SetCanEverAffectNavigation(false);
}

void AClimbableTower::BeginPlay()
{
	// ABuilding: team material, spawn squash, and the DT_Cards stat bind (HP from
	// the WatchTower row — GDD §3.0, never hardcoded). Its LoadStats fires the
	// OnStatsLoaded hook, which this class deliberately does NOT override.
	Super::BeginPlay();

	// After Super, because the gate derives entirely from Team and the deferred
	// spawn path sets Team before BeginPlay (TASK-030/046).
	ConfigureAscentGate();
}

ECollisionChannel AClimbableTower::AscentBlockedChannel(ETeamId TowerTeam)
{
	// ⭐ THE SINGLE SOURCE OF TRUTH for the gate. ConfigureAscentGate authors its
	// one ECR_Block from this, and CanTeamAscend predicts from this, so the
	// shipped behaviour and the tested claim cannot drift apart.
	// SiegeNavAreas.h is the one home for the whole team-gating vocabulary.
	return SiegeEnemyTeamObjectChannel(TowerTeam);
}

bool AClimbableTower::CanTeamAscend(ETeamId TowerTeam, ETeamId ClimberTeam)
{
	// A climber is stopped iff its capsule carries the one channel the gate
	// blocks. Own-team capsules meet ECR_Ignore and walk straight through.
	//
	// ⛔ There is no capacity term and no unit-type term in this expression
	// because there are none in the rule (TOWER-§4): ANY own-team unit may
	// ascend, however many are already up there.
	return SiegeTeamObjectChannel(ClimberTeam) != AscentBlockedChannel(TowerTeam);
}

void AClimbableTower::ConfigureAscentGate()
{
	if (!AscentGateVolume)
	{
		return;
	}

	// Symmetric by construction — everything derives from THIS tower's Team, so a
	// Blue and a Red WatchTower configure mirror-image gates with ⛔ no hardcoded
	// team branch.
	const ECollisionChannel OwnChannel = SiegeTeamObjectChannel(Team);
	const ECollisionChannel BlockedChannel = AscentBlockedChannel(Team);

	const float GateTopUU = PlatformHeightUU + AscentGateHeadroomUU;

	// Degenerate tuning leaves the gate INERT and says so, rather than arming an
	// inverted or zero-volume box: a silently wrong gate is worse than an absent
	// one, because an absent one at least matches what the log claims. Failing
	// open here costs the T-3 rule on that tower and ⛔ nothing else — no unit is
	// stranded, because this component never affects navigation.
	if (GateTopUU <= AscentGateFloorUU || AscentGateHalfExtentXY.X <= 0.f || AscentGateHalfExtentXY.Y <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AClimbableTower '%s': ascent gate NOT armed — degenerate tuning (floor %.1f, top %.1f, half-extent %.1f x %.1f). Enemies can climb this tower."),
			*GetName(), AscentGateFloorUU, GateTopUU, AscentGateHalfExtentXY.X, AscentGateHalfExtentXY.Y);
		return;
	}

	const float GateHalfHeightUU = 0.5f * (GateTopUU - AscentGateFloorUU);
	const float GateCentreZ = AscentGateFloorUU + GateHalfHeightUU;

	AscentGateVolume->SetBoxExtent(FVector(AscentGateHalfExtentXY.X, AscentGateHalfExtentXY.Y, GateHalfHeightUU));
	AscentGateVolume->SetRelativeLocation(FVector(0.f, 0.f, GateCentreZ));

	// Object type = the OWN team channel, deliberately NOT WorldStatic/
	// WorldDynamic, so the projectile terrain-impact OBJECT query and every other
	// object-type query pass through untouched (the castle's stated reasoning —
	// body channels only). Base response = Ignore ALL, which makes the box
	// invisible to the cursor trace, the camera, and every ECC_Pawn reach test.
	AscentGateVolume->SetCollisionObjectType(OwnChannel);
	AscentGateVolume->SetCollisionResponseToAllChannels(ECR_Ignore);

	// THE WHOLE GATE, in one line: enemy combatant capsules (stamped by
	// ASummonedUnit/AHeroCharacter at BeginPlay) block pairwise against this box
	// and cannot gain height on the ramp; own-team capsules pass on the Ignore.
	AscentGateVolume->SetCollisionResponseToChannel(BlockedChannel, ECR_Block);

	// QueryAndPhysics AFTER the matrix is authored — CharacterMovement stops at
	// blocking geometry via query sweeps, so this is the moment the gate arms.
	// On destruction the whole actor is destroyed (ABuilding::HandleDestroyed), so
	// the gate leaves with it and any occupants simply fall (TOWER-§4 T-4).
	AscentGateVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
