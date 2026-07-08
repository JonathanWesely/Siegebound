// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Projectile.h"

#include "CollisionQueryParams.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Siegebound/SiegeCombatStatics.h"

AProjectile::AProjectile()
{
	// genuinely per-tick by spec (GDD §3.0 homing): every tick the projectile
	// re-aims at the target's CURRENT location. Cheap — a handful of live
	// projectiles, one closest-point query each.
	PrimaryActorTick.bCanEverTick = true;

	// ~5 s safety cap (TASK-026 spec): a projectile that somehow never impacts
	// always despawns. Engine-managed (AActor lifespan) — no custom timer to clear.
	InitialLifeSpan = 5.f;

	// plain scene root: SpawnActor applies the spawn TRANSFORM — including its
	// scale (1) — to the root component, which would stomp a scaled mesh root
	// back to a 100 uu ball. The visual hangs below with a fixed relative scale.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// visual-only sphere: NoCollision + no overlaps + no nav (same setup as the
	// unit VisualMesh, TASK-004). Impacts are the distance-based reach test in
	// Tick, never physics — which is the §3.0 "no collision with friendlies"
	// guarantee: this actor cannot PHYSICS-collide with anything. (Terrain/
	// obstacle death, TASK-094, is a tag-filtered segment TRACE in Tick — it
	// needs no collision on this actor either.)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCanEverAffectNavigation(false);
	// engine sphere is 100 uu across — 0.15 scale ≈ a 15 uu projectile (TASK-026 spec)
	VisualMesh->SetRelativeScale3D(FVector(0.15f));

	// content contract (TASK-026 names block / CONVENTIONS team contract), all
	// resolved null-safe in ApplyTeamVisuals: the engine sphere always exists;
	// the MI instances are M1 assets (TASK-012) but a missing asset must still
	// never crash (house rule).
	SphereMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	TeamMaterialBlue = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	TeamMaterialRed = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));
	// §3.8 ranged-hit telegraph (TASK-026 names block): Variant_Combat donor,
	// READ-ONLY — soft-referenced, never edited (CONVENTIONS template-donor rule).
	ImpactEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/Variant_Combat/VFX/NS_Damage.NS_Damage")));
}

void AProjectile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTeamVisuals();
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// resolve the impact effect ONCE (TASK-020 pattern) — never per impact. In
	// practice NS_Damage is already resident (the hero hard-references it via
	// TASK-016/017 and units cache it via TASK-020), making this a lookup, not a
	// disk load. Cleared-in-BP (IsNull) is a silent designer opt-out.
	if (!ImpactEffect.IsNull())
	{
		CachedImpactEffect = ImpactEffect.LoadSynchronous();
		if (!CachedImpactEffect)
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("AProjectile '%s': impact effect '%s' failed to load — hits will show no impact VFX."),
				*GetNameSafe(this), *ImpactEffect.ToString());
		}
	}
}

void AProjectile::InitProjectile(ETeamId InTeam, AActor* InTarget, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass, float InAoERadius)
{
	// one shot, one projectile: re-arming a live projectile is a shooter bug
	if (bInitialized)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AProjectile '%s': already initialized — re-init ignored (fire a new projectile per shot)."),
			*GetNameSafe(this));
		return;
	}
	bInitialized = true;

	Team = InTeam;
	Damage = InDamage;

	// TASK-056: > 0 makes the impact a radial blast (Bomb Tower row AoERadius 250);
	// 0 (the default, and every M2/M3 shot) keeps the single-target path exactly
	// as today. A stray negative is clamped to 0 (treated as single-target).
	AoERadius = FMath::Max(InAoERadius, 0.f);

	// null damage type: fall back to base UDamageType EXPLICITLY (castles read
	// untyped as 100%, TASK-026 castle scaling) rather than relying on
	// ApplyDamage's internal fallback. Ranged attackers pass
	// USiegeDamageType_Projectile (TASK-027/028) so the castle scales to 50%.
	DamageTypeClass = InDamageTypeClass;
	if (!DamageTypeClass)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AProjectile '%s': no damage-type class given — using base UDamageType (castles take 100%%; ranged attackers should pass USiegeDamageType_Projectile)."),
			*GetNameSafe(this));
		DamageTypeClass = UDamageType::StaticClass();
	}

	if (IsValid(InTarget))
	{
		Target = InTarget;
		LastKnownAimPoint = InTarget->GetActorLocation();
	}
	else
	{
		// shooters re-validate their target at fire time (TASK-027/028), so this
		// is a shooter bug — tolerate it: aim at our own location so the first
		// tick's lost-target reach test expires the projectile harmlessly.
		// Destroy() here would be unsafe for deferred-spawn callers that have not
		// FinishSpawning'd yet.
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AProjectile '%s': InitProjectile with no valid target — expiring on first tick."),
			*GetNameSafe(this));
		Target = nullptr;
		LastKnownAimPoint = GetActorLocation();
	}

	// the team is only known now: (re)apply the team color. Plain-SpawnActor
	// callers ran OnConstruction (default team) before this; deferred callers run
	// it after (FinishSpawning). Both end up correct — ApplyTeamVisuals is
	// idempotent and reads the current Team.
	ApplyTeamVisuals();
}

void AProjectile::ApplyTeamVisuals()
{
	if (!VisualMesh)
	{
		return;
	}

	// LoadSynchronous() returns nullptr for unset paths and missing assets alike;
	// either way we skip the assignment (never crash — the ACastle pattern).
	if (UStaticMesh* Mesh = SphereMeshAsset.LoadSynchronous())
	{
		if (VisualMesh->GetStaticMesh() != Mesh)
		{
			VisualMesh->SetStaticMesh(Mesh);
		}
	}

	const TSoftObjectPtr<UMaterialInterface>& TeamMaterial = (Team == ETeamId::Red) ? TeamMaterialRed : TeamMaterialBlue;
	if (UMaterialInterface* Material = TeamMaterial.LoadSynchronous())
	{
		// the engine sphere has a single material slot
		VisualMesh->SetMaterial(0, Material);
	}
}

void AProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// mis-use guard: spawned but never armed (InitProjectile is the spawner
	// contract, TASK-027/028) — expire instead of flying nowhere forever.
	if (!bInitialized)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AProjectile '%s': ticking without InitProjectile — expiring."),
			*GetNameSafe(this));
		Destroy();
		return;
	}

	// homing (GDD §3.0): while the target lives, chase its CURRENT location; once
	// it is gone (units Destroy() on death), keep flying to the last known
	// location and expire there harmlessly (TASK-026 spec). A destroyed-but-
	// extant castle or a dead-but-hidden hero remains a flight goal — their
	// TakeDamage guards zero the hit, so that impact is equally harmless
	// (no damage, no puff; see HandleImpact).
	const bool bTargetLive = IsValid(Target);
	if (bTargetLive)
	{
		LastKnownAimPoint = Target->GetActorLocation();
	}

	const FVector MyLocation = GetActorLocation();
	const float Step = Speed * DeltaSeconds;
	// the step term keeps the reach test from overshooting at low frame rates:
	// anything we would fly past this tick counts as reached
	const float ReachDistance = FMath::Max(ImpactRadius, Step);

	if (bTargetLive)
	{
		// reach test against the CLOSEST POINT on the target's collision (house
		// pattern, TASK-003/004): the castle's ~800x800 base impacts at its
		// walls, not its origin — and the closest point is the impact-VFX point.
		FVector ImpactPoint = FVector::ZeroVector;
		if (GetDistanceToTarget(MyLocation, Target, ImpactPoint) <= ReachDistance)
		{
			HandleImpact(ImpactPoint);
			return;
		}
	}
	else if (FVector::Dist(MyLocation, LastKnownAimPoint) <= ReachDistance)
	{
		// mid-flight target loss (§3.0/TASK-026): reached the last known
		// location — expire harmlessly. No damage, no puff.
		Destroy();
		return;
	}

	// straight homing step at Speed (1500 u/s, §3.0) toward the aim point.
	const FVector Direction = (LastKnownAimPoint - MyLocation).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		// standing exactly on the aim point — the reach tests resolve next tick
		return;
	}

	const FVector ProposedLocation = MyLocation + Direction * Step;

	// M4.5 terrain law (TASK-094, ruling 8): trace the FULL travel segment for
	// walkable terrain (tag "Terrain") and obstacle footprints (tag "Obstacle" —
	// Fab amendment: trees AND rocks). Covering the whole segment means no
	// tunneling through a trunk at 1500 u/s at any frame rate. On a tagged hit
	// the projectile is simply DESTROYED: zero damage, no AoE, no attribution or
	// friendly-fire side effects — an arrow dying on the hillside when its target
	// moved behind a hill IS the high-ground value (ruling 2). Untagged blockers
	// (walls, buildings, castles, the legacy ArenaGround slab) never match, so
	// every shipped interaction — archer-behind-own-wall fly-through, homing,
	// target-overlap damage, §3.0 castle scaling — stays behavior-equivalent.
	// ORDER matters and is deliberate: the target reach test above already ran,
	// so a projectile that REACHES its target this tick impacts it exactly as
	// shipped even when that target hugs a hill flank.
	FHitResult EnvironmentHit;
	if (FindEnvironmentImpact(MyLocation, ProposedLocation, EnvironmentHit))
	{
		UE_LOG(LogGitClaudeUnrealTest, VeryVerbose,
			TEXT("AProjectile '%s': flight blocked by terrain/obstacle '%s' — destroyed with zero damage (M4.5 ruling 8)."),
			*GetNameSafe(this), *GetNameSafe(EnvironmentHit.GetActor()));

		// one-line reuse of the existing cached impact puff (the spec-allowed VFX
		// option) so the player can SEE arrows dying on hills and trunks — a
		// deliberate, flagged deviation from the damage-landed puff gate below
		// (handoffs/TASK-094.md). Missing/cleared effect = no VFX, never a crash.
		if (CachedImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CachedImpactEffect, EnvironmentHit.ImpactPoint);
		}

		// zero damage, no AoE: neither HandleImpact branch runs — the projectile
		// just stops existing (§3.0 destroyed-on-impact, harmless flavor).
		Destroy();
		return;
	}

	// bSweep stays false: physics never moves this actor — environment death is
	// the tag-filtered segment trace above, target impact is the reach test.
	// Blockout tier; arcing over walls is an M7 feel call.
	SetActorLocation(ProposedLocation, /*bSweep=*/ false);
	// cosmetic on a sphere; correct heading for the future M7 arrow-mesh swap
	SetActorRotation(Direction.Rotation());
}

void AProjectile::HandleImpact(const FVector& ImpactPoint)
{
	// TASK-056 AoE branch (Bomb Tower): AoERadius > 0 resolves the impact as a
	// RADIAL blast at the impact point INSTEAD of a single-target hit. The
	// friendly-fire authority is ApplyRadialDamage's own Team filter (TASK-055) —
	// enemies within the radius only, never a friendly — so the single-target
	// same-team gate below is not needed here (and the homed Target is not used
	// for damage; the blast is location-based). Each caught enemy is routed
	// through its own TakeDamage, so per-fortification scaling still applies.
	// This branch is only ever reached when AoERadius > 0; the single-target path
	// below is UNCHANGED (byte-for-byte) for every AoERadius == 0 shot.
	if (AoERadius > 0.f)
	{
		FSiegeCombatStatics::ApplyRadialDamage(
			GetWorld(), GetInstigatorController(), Team, ImpactPoint, AoERadius, Damage, DamageTypeClass);

		// §3.8/§3.7 impact telegraph: the existing NS_Damage donor at the blast
		// point. Unconditional (unlike single-target's damage-landed gate) — a
		// blast is a blast even when it catches nothing, and ApplyRadialDamage
		// returns no total to gate on. Missing/cleared effect = no VFX, never a crash.
		if (CachedImpactEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CachedImpactEffect, ImpactPoint);
		}

		// §3.0: destroyed on impact, same as the single-target path.
		Destroy();
		return;
	}

	// §3.0 no friendly fire, belt-and-braces: shooters only fire at enemies
	// (TASK-027/028 acquisition) and receiver-side checks ignore same-team hits
	// where the instigator chain resolves — but a projectile must never damage a
	// same-team actor even when NEITHER holds (non-pawn shooter + buggy aim).
	// Native cast is valid: UTeamAgent is NotBlueprintable (TASK-001 ruling).
	const ITeamAgent* TargetAgent = Cast<ITeamAgent>(Target.Get());
	if (TargetAgent && TargetAgent->GetTeamId() == Team)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("AProjectile '%s': reached SAME-TEAM target '%s' — expiring harmlessly (shooters must only fire at enemies, GDD §3.0)."),
			*GetNameSafe(this), *GetNameSafe(Target.Get()));
		Destroy();
		return;
	}

	// team attribution (TASK-002 receiver chain): this projectile is
	// DamageCauser; its InstigatorController is EventInstigator — pawn shooters
	// (TASK-028 units) set FActorSpawnParameters::Instigator so steps 1 and 3 of
	// the receivers' chain resolve their team. Non-pawn shooters (TASK-027
	// towers) resolve as unattributable, which APPLIES per the receivers'
	// contract — correct here, because this projectile only ever damages the
	// enemy it was fired at (gate above).
	const float DamageApplied = UGameplayStatics::ApplyDamage(Target.Get(), Damage, GetInstigatorController(), this, DamageTypeClass);

	// §3.8 ranged telegraph baseline (TASK-026): impact puff at the impact
	// point, only when damage actually LANDED — a receiver that zeroed the hit
	// (destroyed castle, dead hero) gets no puff (TASK-016/020 return-value
	// pattern).
	if (DamageApplied > 0.f && CachedImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CachedImpactEffect, ImpactPoint);
	}

	// §3.0: destroyed on impact — landed or zeroed alike.
	Destroy();
}

float AProjectile::GetDistanceToTarget(const FVector& From, const AActor* InTarget, FVector& OutClosestPoint)
{
	OutClosestPoint = FVector::ZeroVector;

	if (!InTarget)
	{
		return TNumericLimits<float>::Max();
	}

	// closest point on the target's collision — mirror of ASummonedUnit::
	// GetDistanceToTarget (TASK-004; that file belongs to TASK-028 this wave, so
	// the helper is mirrored here like the TryGetInstigatorTeam/TryGetDamageTeam
	// pair — keep the two in sync by hand). ECC_Pawn is blocked by pawn capsules
	// and by the castle's BlockAll mesh (TASK-002).
	const float Distance = InTarget->ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint);
	if (Distance < 0.f)
	{
		// no usable collision (mesh not imported, collision disabled) — actor
		// origin fallback for both the distance and the impact point
		OutClosestPoint = InTarget->GetActorLocation();
		return static_cast<float>(FVector::Dist(From, OutClosestPoint));
	}
	return Distance;
}

bool AProjectile::FindEnvironmentImpact(const FVector& TraceStart, const FVector& TraceEnd, FHitResult& OutHit) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// exact tag contract (CONVENTIONS "Arena terrain & environment (M4.5)", set
	// by TASK-095): "Terrain" = ALL walkable ground + hills; "Obstacle" = ALL
	// trees + rocks (Fab amendment — one tag covers every obstacle type, so new
	// obstacle kinds never require code changes here).
	static const FName TerrainTagName(TEXT("Terrain"));
	static const FName ObstacleTagName(TEXT("Obstacle"));

	// MULTI object-type trace: unlike a channel trace it does NOT stop at the
	// first blocking primitive, so an untagged wall standing just in front of a
	// tagged hill can never mask that hill inside one tick's segment. Terrain,
	// trees, rocks, walls, buildings and castles are all static-mesh geometry
	// (object type WorldStatic; WorldDynamic added defensively for any movable-
	// mobility conform). Pawn/Vehicle object types are NOT queried — units and
	// the hero never even appear in the hit list, preserving §3.0.
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	// SIMPLE collision (bTraceComplex = false) is load-bearing twice over:
	// tree/rock conforms carry footprint-only simple hulls (the canopy has NO
	// simple collision, so shots through the canopy PASS — ruling 6), while the
	// walkable terrain is Use-Complex-Collision-As-Simple, which answers simple
	// queries with its tri-mesh anyway (ruling 3). Self is ignored on principle
	// (the visual mesh is NoCollision and untraceable regardless).
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SiegeProjectileTerrain), /*bTraceComplex=*/ false, this);

	TArray<FHitResult> Hits;
	if (!World->LineTraceMultiByObjectType(Hits, TraceStart, TraceEnd, ObjectParams, QueryParams))
	{
		return false;
	}

	// nearest tagged hit wins — an explicit min-scan rather than trusting the
	// engine's hit ordering. O(hit-result): one tick's segment (~25-50 units at
	// 1500 u/s) crosses a handful of primitives at most, never the whole world.
	const FHitResult* NearestTaggedHit = nullptr;
	for (const FHitResult& Hit : Hits)
	{
		// null-safe tag lookup: a component's owner can be dying mid-frame
		const AActor* HitActor = Hit.GetActor();
		if (!HitActor || !(HitActor->ActorHasTag(TerrainTagName) || HitActor->ActorHasTag(ObstacleTagName)))
		{
			continue;
		}
		if (!NearestTaggedHit || Hit.Time < NearestTaggedHit->Time)
		{
			NearestTaggedHit = &Hit;
		}
	}

	if (NearestTaggedHit)
	{
		OutHit = *NearestTaggedHit;
		return true;
	}
	return false;
}
