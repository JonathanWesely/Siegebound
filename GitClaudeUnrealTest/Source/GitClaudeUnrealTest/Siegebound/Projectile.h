// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Projectile.generated.h"

class UDamageType;
class UMaterialInterface;
class UNiagaraSystem;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 *  Siegebound homing projectile (GDD §3.0, TASK-026). Fired by ranged units
 *  (TASK-028 Archer) and towers (TASK-027 Arrow Tower).
 *
 *  - Homing: every tick it flies at Speed (1500 u/s, §3.0) toward the target
 *    actor's CURRENT location. If the target dies mid-flight, it continues to
 *    the last known location and expires there harmlessly (no damage, no VFX).
 *  - Impact = a per-tick reach test against the CLOSEST POINT on the intended
 *    target's collision (the house pattern from hero melee / unit attacks,
 *    TASK-003/004) — within max(ImpactRadius, this tick's travel step). There
 *    is NO physics collision: the visual mesh is NoCollision, so the projectile
 *    can never collide with friendlies (§3.0), and its intended target is the
 *    ONLY actor it can damage. Large targets — the castle above all — impact at
 *    their WALLS rather than their origin, and the closest point doubles as the
 *    impact-VFX point (TASK-020 pattern).
 *    ⭐ THE "~800x800 base" FIGURE THIS LINE USED TO QUOTE IS RETIRED AS A
 *    NUMBER, NOT AS A REASON: the castle measured ~814x814 at the M1 blockout,
 *    ~2438x2462 after Castle-3× and ~7314x7384 at the 9× castle (CONVENTIONS
 *    WR-§0), so the transcribed figure rotted across two resizes. NOTHING BROKE,
 *    and the reason is structural — GetDistanceToTarget calls
 *    ActorGetDistanceToCollision, which measures against the target's LIVE
 *    collision, so the reach test is bounds-aware and SELF-DERIVING and followed
 *    both resizes by itself. The MECHANISM was never the stale part. ⛔ Do not
 *    re-introduce a castle size here — it would rot again at the next resize
 *    and it is not read by anything.
 *  - Terrain law (M4.5, TASK-094 / ruling 8): every tick the FULL travel
 *    segment is line-traced (multi, object types WorldStatic + WorldDynamic,
 *    SIMPLE collision) and the projectile is DESTROYED — zero damage, no AoE,
 *    no attribution — at the nearest hit whose actor carries tag "Terrain"
 *    (walkable ground + hills) or "Obstacle" (tree trunks + rocks — Fab
 *    amendment; exact strings, set by TASK-095). Untagged blockers (walls,
 *    buildings, castles) never match, so projectiles keep flying through them
 *    exactly as shipped (archer-behind-own-wall comp, §3.0 castle scaling).
 *    A homing projectile whose target moves behind a hill legitimately dies
 *    on the hillside — that IS the physical high-ground value (ruling 2);
 *    target ACQUISITION stays range-only (no LOS checks anywhere).
 *  - On impact: applies Damage via ApplyDamage with the damage-type class given
 *    at InitProjectile (USiegeDamageType_Projectile from ranged attackers — the
 *    castle scales it to 50% on ITS side, TASK-026/M2 ruling), then destroys
 *    itself (§3.0 "destroyed on impact"). A same-team target is never damaged
 *    (belt-and-braces §3.0 gate over the shooter's own enemy-only acquisition).
 *  - AoE variant (TASK-056, Bomb Tower): when InitProjectile is armed with
 *    AoERadius > 0, the impact resolves as a RADIAL blast at the impact point
 *    (FSiegeCombatStatics::ApplyRadialDamage — enemies within the radius only,
 *    never a friendly, TASK-055) INSTEAD of the single-target hit, then destroys
 *    itself and spawns the same NS_Damage donor. AoERadius == 0 (the default, and
 *    every M2/M3 Archer/Longbowman/tower shot) is the UNCHANGED single-target path.
 *  - Team attribution for receiver-side no-friendly-fire checks (TASK-002
 *    chain) is INSTIGATOR-plumbed: ApplyDamage passes GetInstigatorController()
 *    as EventInstigator and this projectile as DamageCauser, and the receivers'
 *    step-3 fallback reads DamageCauser->GetInstigator(). PAWN shooters
 *    (TASK-028 units) must therefore set FActorSpawnParameters::Instigator =
 *    themselves when spawning. Non-pawn shooters (TASK-027 towers) have no
 *    instigator pawn — their hits resolve as unattributable, which APPLIES per
 *    the receivers' contract; that is safe because this projectile only ever
 *    damages the enemy target it was fired at.
 *  - Deliberately NOT an ITeamAgent: ASummonedUnit::AcquireTarget targets every
 *    alive enemy ITeamAgent in aggro (GetAllActorsWithInterface), so an
 *    ITeamAgent projectile would be acquired and attacked by enemy units.
 *    The team lives in a plain property instead (GetTeam()).
 *  - Lifetime cap: InitialLifeSpan = 5 s (spec safety) — a projectile that
 *    somehow never impacts always despawns.
 *  - Visual: engine sphere (/Engine/BasicShapes/Sphere) at ~0.15 scale with the
 *    team color instance (MI_TeamColor_Blue/_Red). All soft refs resolve
 *    null-safe — a missing asset is never a crash (M1 house style).
 *
 *  Spawners (TASK-027/028): plain SpawnActor + InitProjectile immediately
 *  after, or SpawnActorDeferred + InitProjectile + FinishSpawning — both work.
 *  A projectile that never gets InitProjectile expires on its first tick.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AProjectile : public AActor
{
	GENERATED_BODY()

public:

	AProjectile();

	/**
	 *  Arms the projectile (TASK-026 names block): the firing team, the actor to
	 *  home onto, the damage to deliver, and the damage-type class it is tagged
	 *  with (ranged attackers pass USiegeDamageType_Projectile::StaticClass();
	 *  null falls back to base UDamageType = 100% vs castle). Call once, right
	 *  after SpawnActor (or between SpawnActorDeferred and FinishSpawning) —
	 *  re-initialization is ignored with a warning: fire a NEW projectile per
	 *  shot. A null/dead target is tolerated (warned): the projectile expires
	 *  harmlessly on its first tick.
	 *
	 *  InAoERadius (TASK-056, default 0): > 0 makes the impact a RADIAL blast at
	 *  the impact point (Bomb Tower — row AoERadius 250) via ApplyRadialDamage
	 *  INSTEAD of the single-target hit; 0 (every M2/M3 shot — Archer, Longbowman,
	 *  Arrow/Ballista Tower) keeps the single-target behavior byte-for-byte. The
	 *  defaulted param leaves all existing 4-arg callers unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Projectile")
	void InitProjectile(ETeamId InTeam, AActor* InTarget, float InDamage, TSubclassOf<UDamageType> InDamageTypeClass, float InAoERadius = 0.f);

	/** Team this projectile fights for (set by InitProjectile). NOT ITeamAgent by design — see the class comment. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Projectile")
	ETeamId GetTeam() const { return Team; }

	/** Homing flight + impact reach test (see class comment). The projectile is genuinely per-tick by spec (§3.0 homing). */
	virtual void Tick(float DeltaSeconds) override;

	/** Resolves the soft-referenced sphere mesh and team material, null-safe (mirrors ACastle). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Caches the impact effect once (TASK-020 pattern — no per-impact sync load). */
	virtual void BeginPlay() override;

	/**
	 *  Plain scene root: the SPAWN transform (scale 1) applies to the root
	 *  component, so the visual's fixed 0.15 scale must live on a CHILD or the
	 *  shooter's spawn call would stomp it back to a 100 uu ball.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Projectile")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 *  Visual sphere under the root. NoCollision: hits are the distance-based
	 *  reach test in Tick, never physics — the §3.0 "no collision with
	 *  friendlies" guarantee. Engine sphere (100 uu) at 0.15 scale ≈ 15 uu.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** Flight speed in units/second. GDD §3.0: projectiles travel at 1500 u/s — a global projectile rule, not a card stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Projectile", meta = (ClampMin = "0"))
	float Speed = 1500.f;

	/**
	 *  Reach distance for the impact test: the projectile impacts when the
	 *  closest point on the target's collision is within max(ImpactRadius, this
	 *  tick's travel step) — the step term prevents overshooting at low frame
	 *  rates. Visual/feel tolerance, not a GDD stat.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Projectile", meta = (ClampMin = "0"))
	float ImpactRadius = 30.f;

	/**
	 *  Impact puff at the point of impact on a DAMAGING hit (§3.8 ranged-hit
	 *  telegraph baseline, TASK-026). Defaults to the READ-ONLY Variant_Combat
	 *  donor NS_Damage — referenced, never edited (CONVENTIONS template-donor
	 *  rule). Resolved and cached ONCE at BeginPlay; a missing asset is logged
	 *  and means no VFX, never a crash. Harmless expiry spawns nothing.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

	/** Engine sphere for the blockout visual (TASK-026 names block). Resolved null-safe in OnConstruction. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Projectile|Visuals")
	TSoftObjectPtr<UStaticMesh> SphereMeshAsset;

	/** Material applied when Team == Blue (CONVENTIONS team contract). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Projectile|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialBlue;

	/** Material applied when Team == Red (CONVENTIONS team contract). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Projectile|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialRed;

private:

	/** Loads (if available) and applies the sphere mesh and the Team-appropriate material. Never crashes on missing assets. */
	void ApplyTeamVisuals();

	/**
	 *  Resolves the impact: same-team gate (§3.0 — expire harmlessly, never
	 *  damage a friendly), ApplyDamage with the carried damage type and the
	 *  instigator plumbing, impact puff only when damage actually landed
	 *  (TASK-016/020 pattern), then Destroy (§3.0 destroyed on impact).
	 */
	void HandleImpact(const FVector& ImpactPoint);

	/**
	 *  Distance from a point to the CLOSEST POINT on the target's collision —
	 *  mirror of ASummonedUnit::GetDistanceToTarget (TASK-004; that header
	 *  belongs to TASK-028, so the helper is mirrored here like the
	 *  TryGetInstigatorTeam/TryGetDamageTeam precedent). Falls back to the actor
	 *  origin when no usable collision exists.
	 */
	static float GetDistanceToTarget(const FVector& From, const AActor* InTarget, FVector& OutClosestPoint);

	/**
	 *  M4.5 terrain law (TASK-094, ruling 8): multi line trace over one tick's
	 *  travel segment (object types WorldStatic + WorldDynamic, SIMPLE collision
	 *  — tree canopies have no simple hulls and stay pass-through per ruling 6,
	 *  while Complex-As-Simple terrain still resolves per ruling 3), keeping
	 *  only the NEAREST hit whose actor carries tag "Terrain" or "Obstacle"
	 *  (exact strings — CONVENTIONS "Arena terrain & environment (M4.5)"
	 *  contract, set by TASK-095). Untagged blockers (walls, buildings,
	 *  castles) never match — shipped fly-through behavior preserved. Cost:
	 *  one trace + an O(hit-result) tag scan per tick; never a world scan.
	 *  Returns true with the tagged hit in OutHit; false otherwise (no world,
	 *  no hits, no tagged hits — null-safe throughout).
	 */
	bool FindEnvironmentImpact(const FVector& TraceStart, const FVector& TraceEnd, FHitResult& OutHit) const;

	/** Firing team, set by InitProjectile. Runtime-only — projectiles are never level-placed. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Projectile", meta = (AllowPrivateAccess = "true"))
	ETeamId Team = ETeamId::Blue;

	/** Actor this projectile homes onto. May die mid-flight — then LastKnownAimPoint carries the flight. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Projectile", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> Target;

	/** Damage delivered on impact, as given by the shooter (from ITS card row — this actor never reads DT_Cards). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Projectile", meta = (AllowPrivateAccess = "true"))
	float Damage = 0.f;

	/** Damage-type class tagged onto the hit (USiegeDamageType_Projectile from ranged attackers; castle-side scaling reads it). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Projectile", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UDamageType> DamageTypeClass;

	/**
	 *  Splash radius for the impact (TASK-056; Bomb Tower row AoERadius 250). > 0
	 *  turns the impact into a RADIAL blast at the impact point (FSiegeCombatStatics::
	 *  ApplyRadialDamage — enemies within the radius only, no friendly fire) INSTEAD
	 *  of a single-target hit; 0 (the default, and every M2/M3 shot) keeps the
	 *  unchanged single-target behavior. Set by InitProjectile from the shooter's
	 *  card row — this actor never reads DT_Cards.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Projectile", meta = (AllowPrivateAccess = "true"))
	float AoERadius = 0.f;

	/**
	 *  Hard cache of ImpactEffect, resolved ONCE at BeginPlay (TASK-020
	 *  pattern) — keeps the Niagara system alive against GC and avoids
	 *  per-impact sync loads.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedImpactEffect;

	/** Target's location as of the last tick it was alive — the flight continues here when the target dies mid-flight (§3.0). */
	FVector LastKnownAimPoint = FVector::ZeroVector;

	/** True once InitProjectile ran. An un-initialized projectile expires (warned) on its first tick. */
	bool bInitialized = false;
};
