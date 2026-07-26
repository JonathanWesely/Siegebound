// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "CaptureZone.generated.h"

class ACaptureZone;
class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;

/**
 *  Ownership of the mid capturable zone (W1-PREP additions 3, TASK-260).
 *  Latched member CaptureOwner — declared here per the enum-in-owning-header law.
 *  NOT named `Owner` (AActor::Owner shadow law). Initial state is Neutral.
 */
UENUM(BlueprintType)
enum class ECaptureState : uint8
{
	Neutral,
	Blue,
	Red
};

/**
 *  Fired whenever CaptureOwner actually changes AND unconditionally on
 *  ResetCaptureZone (reset-path broadcast, CONVENTIONS delegate law). Carries the
 *  zone and its new owner so a HUD/VFX listener needs no follow-up query.
 *  UI/VFX hook — nothing binds it this pass (delegate law FOn<Owner><Event>).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCaptureZoneOwnerChanged, ACaptureZone*, Zone, ECaptureState, NewOwner);

/**
 *  Siegebound capturable mid zone (W1-PREP additions 3, TASK-260 — Jonathan's
 *  2026-07-23 directive: shrink each side's spawn area to a castle-box and add a
 *  single capturable square straddling the centerline). ONE level instance
 *  `CaptureZone_Center` sits at world origin (0,0,0); its box straddles X=0 and
 *  is the SAME size as each side's spawn box (ZoneHalfExtent default (840,840)).
 *
 *  CAPTURE STATE MACHINE (server/authoritative — the eval runs only on
 *  HasAuthority(), consistent with ASiegeGameState/ASiegeGameMode being the
 *  local-authority owners of match state; M8 multiplayer must revisit
 *  replicating CaptureOwner + the tint, mirroring the SiegeGameState note):
 *  every CaptureEvalInterval (0.5 s) a repeating timer counts the friendly vs
 *  enemy UNITS inside the box — "unit" = any ASummonedUnit (incl. AMinerUnit)
 *  OR the AHeroCharacter, team read via ITeamAgent::GetTeamId. Buildings,
 *  towers, castles, and gold nodes are EXCLUDED (they are not units). Each
 *  interval, with blue = Blue-team units inside and red = Red-team units inside:
 *    - blue > 0 && red == 0  -> CaptureOwner = Blue
 *    - red  > 0 && blue == 0  -> CaptureOwner = Red
 *    - blue > 0 && red  > 0   -> CONTESTED
 *    - blue == 0 && red == 0  -> EMPTY: CaptureOwner UNCHANGED (latches last owner)
 *
 *  CONTESTED behavior — Jonathan RULING 2026-07-23 (overrides the board's earlier
 *  STICKY default): a contested zone NEUTRALIZES ("live tug-of-war, must be
 *  held"). The bNeutralizeWhenContested toggle is kept so the alternative
 *  (sticky) survives as an option, but it DEFAULTS TRUE = the shipped behavior.
 *
 *  On any actual change the owner-tint decal updates and OnCaptureOwnerChanged
 *  broadcasts. The DecalComponent soft-loads /Game/Materials/M_CaptureZone
 *  (DeferredDecal, TASK-263) at BeginPlay and drives its `ZoneColor` vector
 *  param by owner via a MID — null-safe: a missing material means no visual, the
 *  mechanic still runs, logged once.
 *
 *  SPAWN-ENABLE API (TASK-261 player / TASK-262 bot consume it): a point is a
 *  legal capture-zone spawn for a team iff it is inside the box AND CaptureOwner
 *  matches that team — CanTeamSpawnHere(Team, Point). IsPointInZone / GetCaptureOwner
 *  / GetZoneHalfExtent let the two controllers run the identical "is this in the
 *  mid zone" test.
 *
 *  Play Again resets CaptureOwner -> Neutral via ResetCaptureZone(), called by
 *  ASiegeGameMode::PlayAgain the same way ACastle::ResetCastle is (§3.9 reset
 *  path). Null-safe everywhere; the eval never crashes on an empty world.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ACaptureZone : public AActor
{
	GENERATED_BODY()

public:

	ACaptureZone();

	/** Broadcast on every actual owner change and unconditionally on reset. HUD/VFX hook — nothing binds it this pass. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Capture")
	FOnCaptureZoneOwnerChanged OnCaptureOwnerChanged;

	/**
	 *  2D (XY) box test about the actor origin against ZoneHalfExtent — Z is
	 *  ignored (a spawn/region test). The single geometry test TASK-261/262
	 *  share so "is this point in the mid zone" is identical on both sides.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Capture")
	bool IsPointInZone(const FVector& Point) const;

	/** Current latched owner of the zone (Neutral until a side captures). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Capture")
	ECaptureState GetCaptureOwner() const { return CaptureOwner; }

	/**
	 *  THE spawn-enable seam for the player + bot placement code (TASK-261/262):
	 *  true iff Point is inside the zone box AND CaptureOwner matches Team
	 *  (Blue<->Blue, Red<->Red). Neutral owner => false for both sides (nobody
	 *  may spawn in an unheld mid zone).
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Capture")
	bool CanTeamSpawnHere(ETeamId Team, const FVector& Point) const;

	/** Zone half-extent (XY) so callers can run the mid-zone bounds test consistently (TASK-261/262). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Capture")
	FVector2D GetZoneHalfExtent() const { return ZoneHalfExtent; }

	/**
	 *  Play Again (§3.9): forces CaptureOwner back to Neutral, re-tints the decal
	 *  gray, and broadcasts OnCaptureOwnerChanged UNCONDITIONALLY (reset-path
	 *  broadcast, CONVENTIONS delegate law) so any HUD/VFX listener snaps back to
	 *  Neutral. Mirrors how ACastle::ResetCastle is driven from the game-mode
	 *  reset loop. Safe before or after BeginPlay; idempotent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Capture")
	void ResetCaptureZone();

	/** Keeps the editor decal footprint matched to ZoneHalfExtent while placed/previewed (null-safe). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Soft-loads the decal material + starts the authoritative capture-eval timer. */
	virtual void BeginPlay() override;

	/** Clears the capture-eval timer so it never outlives the actor. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Scene root — the decal attaches here and projects down onto the terrain. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Capture")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 *  Owner-tint decal. Projects straight down (relative pitch -90 => facing -Z)
	 *  onto the ground, scaled to the zone footprint. Material soft-loaded at
	 *  BeginPlay; a MID drives the `ZoneColor` param by owner. Null-safe.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Capture")
	TObjectPtr<UDecalComponent> ZoneDecal;

	/**
	 *  Half-extent (XY) of the capturable box, centered on the actor origin.
	 *  Default (840,840) = "same size as the spawnable region on either side"
	 *  (Jonathan) = 2x the castle footprint. FLAGGED tunable.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture")
	FVector2D ZoneHalfExtent = FVector2D(840.f, 840.f);

	/** Seconds between capture evaluations (GDD-style mechanic rule, not a card stat). Default 0.5 s. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture", meta = (ClampMin = "0.05"))
	float CaptureEvalInterval = 0.5f;

	/**
	 *  CONTESTED policy — Jonathan RULING 2026-07-23 (overrides the board's
	 *  earlier STICKY default): TRUE => a contested zone (both teams >= 1 inside)
	 *  neutralizes to Neutral each eval ("live tug-of-war, must be held"). FALSE
	 *  => sticky (contested leaves the owner unchanged; a side only loses it when
	 *  the OTHER side meets the capture condition alone). DEFAULTS TRUE = shipped.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture")
	bool bNeutralizeWhenContested = true;

	/**
	 *  Decal projection half-depth (the decal's local X after the -90 pitch =
	 *  world -Z reach). Generous so the tint reaches the terrain surface across
	 *  the arena's hills whether the ground is above or below the zone origin.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture", meta = (ClampMin = "1"))
	float DecalProjectionDepth = 1024.f;

	/** Soft reference to the owner-tint decal material (TASK-263). Null-safe: missing => no visual, mechanic still runs. */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Capture")
	TSoftObjectPtr<UMaterialInterface> ZoneDecalMaterialAsset;

	/** Exact vector param the MID drives by owner (TASK-263 contract). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture")
	FName ZoneColorParamName = TEXT("ZoneColor");

private:

	/** Repeating capture-eval body: counts friendly vs enemy units in the box and applies the transition rules. Authority-only. */
	void EvaluateCapture();

	/** Sets CaptureOwner and, on an actual change, re-tints the decal + broadcasts. */
	void SetCaptureOwner(ECaptureState NewOwner);

	/** Maps a team identity to its capture-state (Blue->Blue, Red->Red) — the spawn-enable + count bucket. */
	static ECaptureState TeamToState(ETeamId Team);

	/** Pushes the owner color onto the decal MID (Neutral gray / Blue / Red per TASK-263). No-op without a MID. */
	void ApplyOwnerColorToDecal();

	/** (Re)applies DecalSize from ZoneHalfExtent + DecalProjectionDepth so instance edits take. Null-safe. */
	void ApplyDecalFootprint();

	/** Latched owner (NOT `Owner` — AActor::Owner shadow law). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Capture", meta = (AllowPrivateAccess = "true"))
	ECaptureState CaptureOwner = ECaptureState::Neutral;

	/** Lazy MID over the decal material — the tint write target. Transient: rebuilt per session. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ZoneDecalMID;

	/** Looping capture-eval timer (CaptureEvalInterval), armed at BeginPlay on authority. */
	FTimerHandle CaptureEvalTimerHandle;

	/** One-shot guard for the missing-material warning. */
	bool bWarnedMissingMaterial = false;
};
