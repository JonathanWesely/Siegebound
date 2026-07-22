// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/TeamId.h"
#include "SpellLineSweep.generated.h"

class USceneComponent;

/**
 *  Hero-origin LINE spell delivery actor (TASK-236, CONVENTIONS "Spell
 *  delivery overhaul (2026-07-21)"). Spawned ONLY by
 *  USpellLibrary::ResolveSpell when a spell's effective delivery is HeroLine
 *  (Fireball + FrostNova today); it carries the effect along a horizontal line
 *  in the air from the caster's origin toward the aim direction, then destroys
 *  itself. Spells still have NO actor BLUEPRINTS (the M5 law) — this is a
 *  transient runtime carrier, exactly like AProjectile, never level-placed and
 *  never a card actor.
 *
 *  Delivery model (recorded design decision, TASK-236): FAST-TRAVEL SWEEP with
 *  PER-SEGMENT application — the "front" advances from the origin to LineRange
 *  over TravelDuration seconds (900 uu / 0.3 s = 3000 uu/s, twice projectile
 *  flight speed — reads as a fast bolt), and every tick each enemy whose
 *  collision lies within LineHalfWidth of the already-swept segment receives
 *  the effect EXACTLY ONCE (AppliedTargets dedupe). Chosen over an
 *  instant-sweep so damage lands in sync with the projectile visual TASK-238
 *  attaches; near targets are hit a few frames before far ones.
 *
 *  Hitbox = the segment [Origin, Origin + AimDirection * front]: a LINE IN THE
 *  AIR at the origin's height (hero capsule center ~ chest; bot castle muzzle
 *  — see CastleMuzzleHeight). Per-candidate reach = closest point on the
 *  segment to the candidate, then the house closest-point-on-collision measure
 *  from there (qa/TASK-026 convention) vs LineHalfWidth — so ground units'
 *  capsules and large fortification footprints are caught naturally.
 *  Deliberately NO line-of-sight blocking: the line passes through walls and
 *  units and applies to EVERYTHING in the corridor, consistent with the M5
 *  ground circle (which ignored LOS) and the M4.5 "target acquisition stays
 *  range-only" posture.
 *
 *  Effect application (magnitudes/costs UNCHANGED — the overhaul law):
 *  - AoEDamage (Fireball): row Damage via ApplyDamage tagged
 *    USiegeDamageType_Spell to every live enemy ITeamAgent on the line —
 *    units, hero, buildings, AND the castle, which scales it to 50% on its own
 *    side (§3.11) exactly as the old radial blast did. Null
 *    instigator/causer on purpose: the TEAM FILTER here is the friendly-fire
 *    authority (the ApplyRadialDamage design, §3.0).
 *  - Freeze (FrostNova): ApplyFreeze(row EffectDuration) on live enemy
 *    ASummonedUnits and ABuildings only — castle and hero excluded by the M5
 *    ruling-5 type filter, unchanged.
 *
 *  Match-end law: ASiegeGameMode::FreezeWorldAtMatchEnd destroys in-flight
 *  AProjectiles so nothing lands under the Victory screen; it predates this
 *  class and cannot know it, so the sweep SELF-GATES — every tick it checks
 *  HasMatchEnded() and destroys itself without further application. Play Again
 *  needs no reset hook: the actor lives <= TravelDuration (2 s failsafe
 *  lifespan) and a match must END before PlayAgain can run, which the gate
 *  already covers.
 *
 *  Deliberately NOT an ITeamAgent (the AProjectile precedent): an ITeamAgent
 *  sweep would be acquired and attacked by enemy units. No collision, no
 *  visual mesh — the look is the NS_Spell_<CardID> system the resolver spawns
 *  at the muzzle; this actor's root transform tracks the advancing front each
 *  tick as the TRAVEL SEAM for TASK-238 (an attached travel VFX can ride it).
 *
 *  A sweep that never receives InitLineSweep expires (warned) on its first
 *  tick — the un-initialized-projectile rule.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASpellLineSweep : public AActor
{
	GENERATED_BODY()

public:

	ASpellLineSweep();

	/**
	 *  Arms the sweep: the CardID (log lines), a value SNAPSHOT of the card row
	 *  (Damage/EffectDuration/SpellEffect — the TargetingRow snapshot pattern;
	 *  FCardRow holds no hard UObject pointers, so a plain member is safe), the
	 *  caster's team (the friendly-fire authority), the line origin, and the
	 *  aim direction (must arrive normalized + horizontal — the resolver
	 *  flattens it). Call once between SpawnActorDeferred and FinishSpawning
	 *  (the InitUnit/InitBuilding contract); re-initialization is ignored with
	 *  a warning.
	 */
	void InitLineSweep(FName InCardID, const FCardRow& InRow, ETeamId InCasterTeam, const FVector& InOrigin, const FVector& InAimDirection);

	/** Advances the sweep front, applies the effect to newly covered enemies, self-destroys at full range (see class comment). */
	virtual void Tick(float DeltaSeconds) override;

	/**
	 *  Line length in units — how far "a short distance in front of the
	 *  player" reaches (Jonathan directive verbatim). 900 ≈ the pre-overhaul
	 *  comfortable placement reach; FLAGGED PLAYTEST NUMBER (the reach nerf vs
	 *  the old anywhere-on-map reticle is the recorded WATCH at TASK-240).
	 *  // GDD §4 (spell delivery overhaul 2026-07-21) — mechanic rule, not a CSV column
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Spell", meta = (ClampMin = "0"))
	float LineRange = 900.f; // GDD §4 — flagged playtest number

	/**
	 *  Half-width of the line hitbox in units (closest-point-on-collision
	 *  reach from the swept segment). FLAGGED PLAYTEST NUMBER — line-vs-circle
	 *  coverage is the overhaul law's playtest WATCH, not a balance change.
	 *  // GDD §4 (spell delivery overhaul 2026-07-21) — mechanic rule, not a CSV column
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Spell", meta = (ClampMin = "0"))
	float LineHalfWidth = 100.f; // GDD §4 — flagged playtest number

	/**
	 *  Seconds for the sweep front to travel from the origin to LineRange
	 *  (recorded design decision: short travel, per-segment application — see
	 *  class comment). <= 0 degrades to an instant full-length sweep on the
	 *  first tick. // GDD §4 (spell delivery overhaul 2026-07-21)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Spell", meta = (ClampMin = "0"))
	float TravelDuration = 0.3f; // GDD §4 — flagged playtest number

	/**
	 *  Z lift applied to the BOT's castle origin (the bot has no hero — its
	 *  line spells fire from its own castle, the overhaul law's flagged design
	 *  default). 150 keeps the line at hero-chest-comparable height so ground
	 *  units' capsules stay inside LineHalfWidth — a battlement-height muzzle
	 *  would fly OVER them. Read by USpellLibrary via GetDefault so every
	 *  line-delivery number shares this one home.
	 *  // GDD §4 (spell delivery overhaul 2026-07-21)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Spell", meta = (ClampMin = "0"))
	float CastleMuzzleHeight = 150.f; // GDD §4 — flagged playtest number

protected:

	/** Plain scene root — the actor's transform tracks the sweep front (the TASK-238 travel seam); no collision, no visual. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Spell")
	TObjectPtr<USceneComponent> SceneRoot;

private:

	/** Applies the spell effect to every not-yet-applied enemy within LineHalfWidth of the segment [LineOrigin, LineOrigin + AimDirection * FrontDistance]. */
	void ApplyLineEffectUpTo(float FrontDistance);

	/**
	 *  Per-target effect dispatch on Row.SpellEffect (AoEDamage / Freeze — the
	 *  two line-delivered effects; see class comment for the exact semantics).
	 *  Returns true when the effect actually landed (counted), false for
	 *  excluded types and dead targets (still marked processed — never
	 *  re-tested).
	 */
	bool ApplyEffectToTarget(AActor* Target) const;

	/**
	 *  Distance from a point to the CLOSEST POINT on the target's collision,
	 *  actor-origin fallback — the shared measurement convention (qa/TASK-026
	 *  ruling). Mirrored here like AProjectile::GetDistanceToTarget mirrors it
	 *  (the qa/TASK-026 NIT-4 mirror debt — folds into the ONE shared helper on
	 *  the wave that owns all mirrors; flagged in handoffs/TASK-236.md).
	 */
	static float DistanceToTargetCollision(const FVector& From, const AActor* Target);

	/** Card whose spell this sweep carries — names the log lines (the effect and magnitudes come from Row). */
	FName CardID;

	/** Row snapshot taken at InitLineSweep (plain value member — the TargetingRow precedent; no hard UObject pointers in FCardRow). */
	FCardRow Row;

	/** Caster's team — the friendly-fire authority (§3.0): same-team agents are never touched. */
	ETeamId CasterTeam = ETeamId::Blue;

	/** Line start point in world space (hero capsule center / bot castle muzzle — resolved by USpellLibrary). */
	FVector LineOrigin = FVector::ZeroVector;

	/** Normalized HORIZONTAL aim direction (flattened by the resolver — the line stays at the origin's height). */
	FVector AimDirection = FVector::ForwardVector;

	/** Seconds since the sweep started (drives the front: LineRange * Elapsed / TravelDuration, clamped). */
	float ElapsedSeconds = 0.f;

	/** Actors already processed (applied OR excluded) — the exactly-once guarantee. Weak: a mid-sweep death never dangles. */
	TSet<TWeakObjectPtr<AActor>> AppliedTargets;

	/** Targets the effect actually landed on (the completion log's count). */
	int32 AppliedCount = 0;

	/** True once InitLineSweep ran. An un-initialized sweep expires (warned) on its first tick. */
	bool bInitialized = false;
};
