// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "AncientGround.generated.h"

class UDecalComponent;
class UMaterialInterface;
class USceneComponent;

/**
 *  Siegebound ANCIENT GROUND (batch ANCIENT-GROUNDS, TASK-359 — Jonathan's
 *  2026-08-01 directive: "a new ancient grounds area of interest the size of the
 *  mid capture zone, one per side placed symmetrically"). A square terrain
 *  objective that PERMANENTLY strengthens friendly units standing on it while a
 *  friendly Sorcerer stands there too.
 *
 *  THE MECHANIC — once per BoostTickInterval (1 s), a single ASummonedUnit sweep:
 *    1. Count the live ancient-ground empowerers (Sorcerers) inside the box, PER
 *       TEAM. An empowerer is NEVER an occupant — a sorcerer never self-boosts.
 *    2. Every other live, boost-eligible unit inside the box is an occupant.
 *    3. Each occupant gains SorcererCount[its OWN team] permanent damage stacks.
 *  FRIENDLY-ONLY (Jonathan ruling): a Blue sorcerer boosts only Blue units, so a
 *  CONTESTED ground empowers BOTH sides at once through their own sorcerers —
 *  it is a shared resource, never a captured one. Per-sorcerer stacking: two
 *  friendly sorcerers on the ground grant 2 stacks/s. A zero grant makes no call
 *  at all (no spurious FOnCombatantDamageBoostChanged broadcast).
 *
 *  ⚠️ MUST NOT DERIVE FROM ACaptureZone — LOAD-BEARING, DO NOT "REFACTOR" THIS.
 *  TActorIterator<ACaptureZone> is a UNIT-SPAWN-ELIGIBILITY GATE
 *  (SiegePlayerController.cpp:3559, SiegeBotController.cpp:1183/1380 — the bot
 *  takes the FIRST instance it finds) plus the play-again reset
 *  (SiegeGameMode.cpp:835). Subclassing ACaptureZone would silently make every
 *  ancient ground spawnable-in AND let a randomly-placed ground win the bot's
 *  first-instance race. So the decal + 2D-box boilerplate below is a DELIBERATE
 *  2-MIRROR of ACaptureZone (CaptureZone.cpp:59-216).
 *  🔧 KNOWN DEBT (recorded in BOTH headers per CONVENTIONS §2): the decal
 *  footprint/soft-load/2D-box-test code is duplicated between ACaptureZone and
 *  AAncientGround. If a third zone-shaped actor ever appears, the fix is to lift
 *  the SHARED HALF into a UZoneFootprintComponent (or a static helper) that
 *  BOTH actors compose — never a shared base class, for the reason above.
 *  ⚠️ PAIRED TUNABLE: ZoneHalfExtent (840,840) is deliberately IDENTICAL to
 *  ACaptureZone::ZoneHalfExtent ("the size of the mid capture zone"). Changing
 *  one without the other breaks Jonathan's directive — change both together.
 *
 *  ⚖️ NET RELEVANCY TIER: **C — NOT REPLICATED** (declared per the CONVENTIONS
 *  NET RELEVANCY LAW declaration duty). `bReplicates` stays at the AActor
 *  default (false) and is never set. Rationale: this actor holds NO replicated
 *  truth. Both machines construct an IDENTICAL pair locally from
 *  ASiegeBattlefieldScatter's already-Tier-A replicated ChosenSeed /
 *  GenerationIndex, exactly like the mine pair (AGoldNode precedent). The state
 *  it produces — PermanentDamageStacks — lives on ASummonedUnit and replicates
 *  with the unit fleet in M8 P2; the ground itself is a pure local projection of
 *  a seed. No collision primitive and no nav geometry either (SceneRoot +
 *  UDecalComponent, which derives from USceneComponent, not UPrimitiveComponent),
 *  so it cannot affect the traversability guarantee.
 *
 *  ⚠️ AUTHORITY IS PUSHED, NEVER READ — THE SINGLE MOST DANGEROUS SPOT IN THIS
 *  FEATURE. Because the ground is spawned LOCALLY ON THE CLIENT (deterministically,
 *  from the replicated seed) it keeps ROLE_Authority there, so the engine's
 *  `HasAuthority()` query returns TRUE on the client too — the CONVENTIONS M8
 *  "narrow principle". Guarding the boost tick on it would not disable the sim;
 *  it would SILENTLY RUN A ROGUE CLIENT-SIDE BOOST SIM that diverges from the
 *  server's. So: the scatter calls InitAncientGround(bAuthoritativeGenerate),
 *  threading ITS OWN authority decision, and ApplyBoostTick gates on the STORED
 *  bAuthoritativeBoost flag ONLY. This mirrors
 *  ASiegeBattlefieldScatter::RunScatterPasses(Seed, bAuthoritativeGenerate).
 *  The flag DEFAULTS FALSE (fail-closed: an un-Init'd ground never boosts), and
 *  the engine's authority query appears NOWHERE in AncientGround.cpp — that is a
 *  QA-checked invariant, not a preference.
 *
 *  NO Reset...() AND NO SiegeGameMode EDIT, BY DESIGN (do not file this as a
 *  missing reset): the ground LATCHES NO STATE — no owner, no progress, no
 *  accumulator — so there is nothing to reset. Play Again step 7 already calls
 *  ClearScatter() + GenerateScatter(), which destroys this pair and re-places a
 *  fresh one for free.
 *
 *  The decal soft-loads /Game/Materials/M_AncientGround (TASK-374) and is
 *  NULL-SAFE: a missing material means no visual, THE MECHANIC STILL RUNS, and
 *  the miss is logged exactly once.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AAncientGround : public AActor
{
	GENERATED_BODY()

public:

	AAncientGround();

	/**
	 *  THE AUTHORITY PUSH (CONVENTIONS §2 law, PINNED signature). Called by
	 *  ASiegeBattlefieldScatter::PlaceAncientGrounds immediately after spawn,
	 *  threading the SAME bAuthoritativeGenerate flag RunScatterPasses already
	 *  carries. Passing false leaves the ground fully visual: the decal still
	 *  draws, the boost tick becomes a no-op.
	 *  ⚠️ This is the ONLY way bAuthoritativeBoost is ever set. The actor MUST
	 *  NOT derive its own answer — see the class doc.
	 *  Safe before or after BeginPlay; safe to call more than once.
	 */
	void InitAncientGround(bool bAuthoritative);

	/**
	 *  2D (XY) box test about the actor origin against ZoneHalfExtent — Z is
	 *  IGNORED (a region test, exactly like ACaptureZone::IsPointInZone, so a
	 *  unit on a slight rise inside the footprint still counts).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|AncientGround")
	bool IsPointInZone(const FVector& Point) const;

	/** Zone half-extent (XY) — PAIRED TUNABLE with ACaptureZone::ZoneHalfExtent (see class doc). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|AncientGround")
	FVector2D GetZoneHalfExtent() const { return ZoneHalfExtent; }

	/**
	 *  THE ancient-ground finder (TASK-418; signature PINNED character-for-character
	 *  by CONVENTIONS "In-match LLM command assistant (v1, text-only) — 2026-08-02"
	 *  §9). Faithful mirror of AGoldNode::FindBestMineFor (GoldNode.h:153) reduced to
	 *  its single tier: IsValid, nearest wins on SQUARED 2D distance, strict < so the
	 *  first-found ground keeps an exact tie. TActorIterator's order is stable for a
	 *  fixed world, so the answer does not churn between calls.
	 *
	 *  ⚠️ NO TEAM PARAMETER, AND THAT IS DELIBERATE — DO NOT ADD ONE LATER. An ancient
	 *  ground is TEAM-NEUTRAL: a contested ground empowers BOTH sides at once through
	 *  their own sorcerers (the shipped FRIENDLY-ONLY boost law, class doc above), so
	 *  there is no such thing as "our" ground to filter for. NEAR vs FAR is resolved
	 *  entirely by the CALLER's From — under the 180°-rotational-symmetry law there are
	 *  exactly two grounds and they are rotational twins, so passing the OWN castle
	 *  location yields `ancient_ground_near` and the ENEMY castle location yields
	 *  `ancient_ground_far`: the two distinct grounds, exactly, by construction and
	 *  with zero extra state.
	 *
	 *  The pair is SCATTER-SPAWNED, never level-placed, so the positions change every
	 *  match and on every Play Again — always ask, never cache the result.
	 *
	 *  A null World, or a world holding no grounds at all (a fallback scatter, a future
	 *  map without them), answers nullptr and NEVER crashes — this class's standing
	 *  "missing => pre-feature behavior, never a crash" discipline. Purely additive and
	 *  read-only: it mutates nothing, arms nothing, and reads no authority.
	 */
	static AAncientGround* FindNearestAncientGround(UWorld* World, const FVector& From);

	/** Keeps the editor decal footprint matched to ZoneHalfExtent while placed/previewed (null-safe). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Soft-loads the decal material and arms the boost timer (the TICK BODY is what gates on authority, not the arming). */
	virtual void BeginPlay() override;

	/** Clears the boost timer so it never outlives the actor (TASK-004 timer-hygiene law). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Scene root — the decal attaches here and projects down onto the terrain. No collision, no nav. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|AncientGround")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 *  Rune decal. Projects straight down (relative pitch -90 => facing -Z) onto
	 *  the ground, scaled to the zone footprint. Material soft-loaded at
	 *  BeginPlay. No MID: the ground is team-neutral, so nothing drives a colour
	 *  param at runtime — M_AncientGround's authored `GroundColor` default (jade)
	 *  IS the shipped look (TASK-374).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|AncientGround")
	TObjectPtr<UDecalComponent> GroundDecal;

	/**
	 *  Half-extent (XY) of the boost box, centered on the actor origin.
	 *  Default (840,840) = "the size of the mid capture zone" (Jonathan).
	 *  ⚠️ PAIRED TUNABLE with ACaptureZone::ZoneHalfExtent — change both. FLAGGED.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|AncientGround")
	FVector2D ZoneHalfExtent = FVector2D(840.f, 840.f);

	/**
	 *  Seconds between boost evaluations. MECHANIC RULE — a UPROPERTY default,
	 *  NEVER a cards.csv column (CONVENTIONS "mechanic rules aren't card stats";
	 *  GDD §3.0 now states that law in its own right, and the DESIGN home of
	 *  record is GDD §3.12 "Ancient Grounds & the Sorcerer", which names this
	 *  1 s tick as one of the three engine tunables — the ENGINEERING law still
	 *  lives at CONVENTIONS §2). 1.0 s is the design unit: each friendly
	 *  sorcerer on the ground grants exactly ONE stack per second. Changing it
	 *  re-scales the whole boost rate. FLAGGED tunable.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|AncientGround", meta = (ClampMin = "0.05"))
	float BoostTickInterval = 1.0f;

	/**
	 *  Decal projection half-depth (the decal's local X after the -90 pitch =
	 *  world -Z reach). Generous so the runes reach the terrain surface across
	 *  the arena's undulation whether the ground is above or below the origin.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|AncientGround", meta = (ClampMin = "1"))
	float DecalProjectionDepth = 1024.f;

	/** Soft reference to the rune decal material (TASK-374). Null-safe: missing => no visual, MECHANIC STILL RUNS, logged once. */
	UPROPERTY(EditAnywhere, Category = "Siegebound|AncientGround")
	TSoftObjectPtr<UMaterialInterface> GroundDecalMaterialAsset;

	/**
	 *  ⚠️ Explicitly 0.001, NOT the engine default 0.01 — at this arena's
	 *  zoomed-out RTS framing the decal's screen footprint drops below the
	 *  default threshold and the engine culls it entirely (the ground would
	 *  vanish exactly when the player is looking at the whole battlefield).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|AncientGround", meta = (ClampMin = "0.0"))
	float DecalFadeScreenSize = 0.001f;

	/**
	 *  Decal sort order — higher draws later (on top). 10 puts the ancient
	 *  ground ABOVE M_CaptureZone (which leaves SortOrder at the default 0) for
	 *  the case where a ground is placed near the centerline and the two
	 *  footprints touch (CONVENTIONS §2 Material clause).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|AncientGround")
	int32 DecalSortOrder = 10;

private:

	/**
	 *  Repeating boost body (BoostTickInterval). ⚠️ Gates on bAuthoritativeBoost
	 *  ONLY — see the class doc for why reading engine authority here would be a
	 *  rogue client-side sim rather than a guard.
	 */
	void ApplyBoostTick();

	/** (Re)applies DecalSize/FadeScreenSize/SortOrder from the tunables so instance edits take. Null-safe. */
	void ApplyDecalFootprint();

	/**
	 *  THE ONLY authority signal this actor has. PUSHED by the scatter via
	 *  InitAncientGround; never derived. Defaults FALSE so an un-Init'd ground
	 *  fails CLOSED (no boost) rather than open.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|AncientGround", meta = (AllowPrivateAccess = "true"))
	bool bAuthoritativeBoost = false;

	/** Looping boost timer (BoostTickInterval), armed at BeginPlay; cleared in EndPlay. */
	FTimerHandle BoostTickTimerHandle;

	/** One-shot guard for the missing-material warning. */
	bool bWarnedMissingMaterial = false;
};
