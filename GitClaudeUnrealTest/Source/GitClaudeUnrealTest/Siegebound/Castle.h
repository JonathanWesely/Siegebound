// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Castle.generated.h"

class ACastle;
class UCameraShakeBase;
class UMaterialInterface;
class USiegeHitFlashComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UUserWidget;
class UWidgetComponent;

/**
 *  Broadcast exactly once when a castle's HP reaches 0 (GDD §3.9).
 *  DestroyedCastle is the castle that fell; CastleTeam is its team —
 *  the OTHER team is the match winner. ASiegeGameMode (TASK-006)
 *  subscribes to this on every ACastle at BeginPlay.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleDestroyed, ACastle*, DestroyedCastle, ETeamId, CastleTeam);

/**
 *  Broadcast on every ACTUAL CurrentHP change (playtest R1 finding 2, TASK-018):
 *  after damage is applied in TakeDamage — NEVER for ignored friendly fire,
 *  which changes nothing — plus in ResetCastle and once at BeginPlay (seed).
 *  UI consumers must still seed from GetCurrentHP()/GetMaxHP() FIRST and bind
 *  second (qa/TASK-005-report.md major 2); UCastleHealthBarWidget::InitForCastle
 *  does exactly that.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleHPChanged, float, CurrentHP, float, MaxHP);

/**
 *  Siegebound castle (GDD §3.9). One per team, placed at the CastleAnchor
 *  TargetPoints in L_Arena at integration (Castle_Blue / Castle_Red).
 *
 *  - Does not attack; it is a 2000 HP objective.
 *  - No friendly fire: damage whose instigator is on the same team is ignored (§3.0).
 *  - At 0 HP it broadcasts OnCastleDestroyed exactly once, hides, and stops colliding.
 *  - ResetCastle() (Play Again, §3.9) restores full HP, visibility, and collision.
 *  - Mesh and per-team material are soft references resolved null-safe in
 *    OnConstruction — the art assets are produced in parallel and may not exist yet.
 *  - Overhead HP bar (playtest R1 finding 2, TASK-018): screen-space HPBarWidget
 *    component; its widget class is soft-resolved null-safe at BeginPlay
 *    (WBP_CastleHealthBar, built in TASK-019 — a missing asset is a silent
 *    no-op). Hidden on destruction, shown again by ResetCastle().
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ACastle : public AActor, public ITeamAgent
{
	GENERATED_BODY()

public:

	ACastle();

	/** Fired exactly once when this castle is destroyed. Win-condition hook for ASiegeGameMode (TASK-006). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleDestroyed OnCastleDestroyed;

	/** Fired on every actual HP change, in ResetCastle, and once at BeginPlay (seed). Drives WBP_CastleHealthBar (TASK-018/019). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleHPChanged OnCastleHPChanged;

	//~ Begin ITeamAgent interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent interface

	/**
	 *  Applies incoming damage. Same-team instigators are ignored entirely (no friendly
	 *  fire, §3.0). Damage-vs-fortification scaling (§3.0) is read from
	 *  DamageEvent.DamageTypeClass: USiegeDamageType_Siege = 200% (Siege units batter
	 *  the castle, TASK-054), USiegeDamageType_Projectile (and subclasses) = 50% (the
	 *  anti-sniping rule), melee/default/untyped = 100%. Units and the hero always take
	 *  listed damage (no scaling). Returns the SCALED amount the castle actually took.
	 *  USiegeDamageType_Spell (and subclasses) = 50% (spells vs the castle, GDD §3.0/§3.11 — TASK-098).
	 */
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Play Again (§3.9): restores MaxHP, visibility, and collision, and re-arms the destroyed event. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Castle")
	void ResetCastle();

	/**
	 *  Heals this castle by Total HP spread evenly over Duration seconds (Masons
	 *  Instant — GDD §4, TASK-059). A repeating timer delivers the heal in fixed
	 *  HealTickInterval ticks; each tick is CLAMPED to MaxHP and broadcasts
	 *  OnCastleHPChanged on any actual change, so the HP bar (TASK-018) tracks the
	 *  repair live. A second call while a heal runs ADDS Total to the remaining pool
	 *  and re-derives the per-tick rate over the new Duration (Masons restack). The
	 *  stream self-terminates when the pool empties OR HP reaches MaxHP (never over
	 *  MaxHP), and is cancelled by HandleDestroyed()/ResetCastle(). No-op on a
	 *  destroyed castle or a non-positive Total/Duration. The heal MAGNITUDES belong
	 *  to the caller (ASiegePlayerController's Masons UPROPERTYs, CONVENTIONS
	 *  mechanic-rule) — this stays a generic heal-over-time receiver.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Castle")
	void HealOverTime(float Total, float Duration);

	/** Current hit points, in [0, MaxHP]. HUD/QA hook (TASK-011). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points (2000 per GDD §3.9). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	float GetMaxHP() const { return MaxHP; }

	/** True once the castle has been destroyed and until ResetCastle(). Unit targeting (TASK-004) should skip destroyed castles. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	bool IsCastleDestroyed() const { return bDestroyed; }

	/** Resolves the soft-referenced mesh and per-team material, null-safe. */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Seeds CurrentHP from MaxHP, fires the OnCastleHPChanged seed broadcast, and initializes the HP bar widget (null-safe). */
	virtual void BeginPlay() override;

	/** Static mesh root. Mesh asset assigned null-safe in OnConstruction from CastleMeshAsset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UStaticMeshComponent> CastleMesh;

	/** Screen-space overhead HP bar (DrawSize 256x32 at Z+1050 — the castle mesh is 900 tall). Widget class resolved null-safe at BeginPlay from HPBarWidgetClass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UWidgetComponent> HPBarWidget;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based (composes cleanly with the crumble MI swap), null-safe. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/** Which team owns this castle. Set per level instance (Castle_Blue = Blue, Castle_Red = Red). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	ETeamId Team = ETeamId::Blue;

	/** Maximum hit points (GDD §3.9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "1"))
	float MaxHP = 2000.0f;

	/** Seconds between heal-over-time ticks (Masons repair, TASK-059) — impl detail, not a GDD stat. Smaller = smoother bar; the total/duration are the caller's. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle", meta = (ClampMin = "0.05"))
	float HealTickInterval = 0.2f;

	/** Castle blockout mesh (TASK-013). May not exist yet — resolved null-safe in OnConstruction. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UStaticMesh> CastleMeshAsset;

	/** Team material applied when Team == Blue (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialBlue;

	/** Team material applied when Team == Red (TASK-012). Null-safe. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftObjectPtr<UMaterialInterface> TeamMaterialRed;

	/** HP bar widget class, /Game/UI/WBP_CastleHealthBar (TASK-019). Missing asset = silent no-op, never a crash. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftClassPtr<UUserWidget> HPBarWidgetClass;

	/**
	 *  §6 screen shake on castle hits (TASK-158): a brief camera shake on the LOCAL
	 *  player controller each time the castle takes ACTUAL damage. Default is the
	 *  READ-ONLY Variant_Combat donor BP_CameraShake_Hit_Enemy (CONVENTIONS
	 *  template-donor rule) — a BP may retarget it to a dedicated BP_CameraShake_CastleHit.
	 *  Soft, null-safe: a missing class = no shake, never a crash.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Castle|Visuals")
	TSoftClassPtr<UCameraShakeBase> CastleHitCameraShake;

	//~ §3.9 castle crumble stage thresholds (TASK-157) — fractions of MaxHP, fired ONCE each
	//~ on the way DOWN, in order. Play Again re-arms them (ResetCastle). // GDD §3.9 (75/50/25%)

	/** Crumble stage 1 threshold (fraction of MaxHP). // GDD §3.9 — 75% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction1 = 0.75f; // GDD §3.9

	/** Crumble stage 2 threshold (fraction of MaxHP). // GDD §3.9 — 50% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction2 = 0.50f; // GDD §3.9

	/** Crumble stage 3 threshold (fraction of MaxHP). // GDD §3.9 — 25% */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Crumble", meta = (ClampMin = "0", ClampMax = "1"))
	float CrumbleFraction3 = 0.25f; // GDD §3.9

private:

	/** Loads (if available) and applies the castle mesh and the Team-appropriate material. Never crashes on missing assets. */
	void ApplyTeamVisuals();

	/** Resolves HPBarWidgetClass null-safe (missing = silent no-op), assigns it to HPBarWidget, and calls InitForCastle on the created UCastleHealthBarWidget. */
	void InitHPBarWidget();

	/** Single-fire destruction: guards on bDestroyed, hides the actor, disables collision, broadcasts OnCastleDestroyed. */
	void HandleDestroyed();

	/**
	 *  §3.9 castle crumble (TASK-157): advances CrumbleStage while CurrentHP has crossed
	 *  the next 75/50/25% threshold ON THE WAY DOWN — firing each stage EXACTLY once, in
	 *  order (a single big hit that crosses two thresholds fires both). Never retreats, so
	 *  a heal-back-up never un-crumbles or re-arms a passed stage (only ResetCastle re-arms).
	 *  Called from TakeDamage after CurrentHP is lowered.
	 */
	void UpdateCrumbleStages();

	/**
	 *  Applies one crumble stage (1..3): swaps the castle mesh AND/OR material to the
	 *  damaged variant (soft /Game/Meshes/SM_Castle_Crumble0N + /Game/Materials/MI_Castle_Crumble0N,
	 *  whichever resolves — null-safe, missing keeps the current look) and bursts NS_CastleDebris
	 *  at the castle. VISUAL ONLY — the crumble mesh variants must preserve the UCX footprint
	 *  (art contract) so placement/pathing are untouched.
	 */
	void ApplyCrumbleStage(int32 Stage);

	/** Masons heal-over-time tick (TASK-059): delivers HealPerTick clamped to MaxHP, broadcasts OnCastleHPChanged on change, self-stops at MaxHP or when the pool empties. */
	void HandleHealTick();

	/** Cancels any running heal-over-time and zeroes its pool (called by HandleDestroyed and ResetCastle). Null-safe/idempotent. */
	void StopHealOverTime();

	/**
	 *  Resolves the attacking team from a damage event's instigator chain:
	 *  the instigating controller's pawn, then the damage causer itself, then
	 *  the causer's instigator pawn (projectiles, M2). Returns false if no
	 *  ITeamAgent is found (e.g. world damage), in which case damage applies.
	 */
	static bool TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam);

	/** Current hit points. Mutated only by TakeDamage and ResetCastle. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 2000.0f;

	/** True after OnCastleDestroyed has fired; re-armed only by ResetCastle(). Guarantees the event fires exactly once. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	bool bDestroyed = false;

	/** Highest crumble stage fired so far (0 = pristine, 1/2/3 = 75/50/25% crossed). Monotonic; reset to 0 by ResetCastle. (TASK-157) */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	int32 CrumbleStage = 0;

	/** HP still to be delivered by the running Masons heal-over-time (0 = none). Mutated only by HealOverTime / HandleHealTick / StopHealOverTime. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealRemaining = 0.0f;

	/** HP delivered per heal tick, derived from Total/Duration on HealOverTime (TASK-059). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealPerTick = 0.0f;

	/** Repeating heal-over-time timer handle (Masons repair). Cleared by StopHealOverTime. */
	FTimerHandle HealTimerHandle;
};
