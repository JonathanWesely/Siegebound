// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Castle.generated.h"

class ACastle;
class UBoxComponent;
class UCameraShakeBase;
class UMaterialInterface;
class UNavModifierComponent;
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

	/** Fired on every actual HP change, in ResetCastle, and once at BeginPlay (seed). Drives WBP_CastleHealthBar (TASK-018/019). M8: OnRep_CurrentHP fires this SAME delegate on clients — zero widget changes (doc §3.1). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
	FOnCastleHPChanged OnCastleHPChanged;

	/**
	 *  ⚖️ NET RELEVANCY TIER: **A — `bAlwaysRelevant = true`** (declared per the
	 *  CONVENTIONS NET RELEVANCY LAW declaration duty; set in the constructor).
	 *  Rationale: match-critical near-singleton (two per match) carrying the WIN
	 *  CONDITION's state — its HP/crumble/destroyed truth may never depend on
	 *  camera distance. TASK-357 measured the far castle (488 m) failing under
	 *  the engine's default 150 m relevancy; this is that blocker's fix.
	 */

	/** Registers the M8 P1 castle set — CurrentHP / bDestroyed / CrumbleStage (OnReps) + Team (InitialOnly belt) — doc §3.1 (TASK-356). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

	/**
	 *  True when Point lies within this castle's 2D spawn box — a castle-centered square
	 *  test: |Point.X - ActorX| <= SpawnBoxHalfExtent.X && |Point.Y - ActorY| <=
	 *  SpawnBoxHalfExtent.Y (Z ignored). Used by the Shield Wall ATTACK command
	 *  (ASummonedUnit::FindNearestEnemyInSpawnBox, W1 TASK-275) so player units clear
	 *  enemies massing inside the enemy castle's spawn region before hitting the castle.
	 *  ADDITIVE — this does NOT touch the bot's ASiegeBotController::IsPointInBotSpawnBox
	 *  (TASK-262 logic UNDISTURBED); it is an independent third reader of the same box.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Castle")
	bool IsPointInSpawnBox(const FVector& Point) const;

	/** Resolves the soft-referenced mesh and per-team material, null-safe. */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/**
	 *  TASK-349 loop-2 B3 Leg 2: selects the TEAM's interior nav area on
	 *  InteriorNavModifier here — after the serialized Team is authoritative,
	 *  BEFORE the first navmesh generation pass — so FRESHLY GENERATED interior
	 *  tiles build correct-first-time (never ctor-Blue-then-flip). This is the
	 *  GENERATION-TIME half only: pre-built/saved tiles see no area CHANGE here
	 *  and are re-marked by the loop-4 unconditional BeginPlay refresh in
	 *  ConfigureTeamGating (B4 — the two hooks cover different lanes and both
	 *  must stay; see the InteriorNavModifier doc). BeginPlay's SetAreaClass
	 *  re-assert early-outs on the unchanged value (free no-op, kept for
	 *  spawned-castle paths).
	 */
	virtual void PostInitializeComponents() override;

	/** Seeds CurrentHP from MaxHP, fires the OnCastleHPChanged seed broadcast, and initializes the HP bar widget (null-safe). */
	virtual void BeginPlay() override;

	/** Static mesh root. Mesh asset assigned null-safe in OnConstruction from CastleMeshAsset. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UStaticMeshComponent> CastleMesh;

	/** Screen-space overhead HP bar (DrawSize 256x32 at Z+3150 — the 3× castle is ~2694 tall; re-derived ×3 from the old 1050/900 pair by TASK-349). Widget class resolved null-safe at BeginPlay from HPBarWidgetClass. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	TObjectPtr<UWidgetComponent> HPBarWidget;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based (composes cleanly with the crumble MI swap), null-safe. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/**
	 *  Team gate blocker (TASK-349, CONVENTIONS "Castle 3× HOLLOW" team-gating law,
	 *  PHYSICAL lane): an invisible box spanning the gate opening, configured at
	 *  BeginPlay (ConfigureTeamGating) from THIS castle's Team — object type = the
	 *  OWN team channel, responses = Ignore everything, Block ONLY the enemy team's
	 *  channel. So enemy combatants (units AND the player-driven hero — hero
	 *  ruling) are physically stopped at the gate while the own team walks through
	 *  untouched. An ACTOR component: the crumble mesh swap (ApplyCrumbleStage
	 *  SetStaticMesh) can never strip it. Never affects navigation (the nav lane is
	 *  InteriorNavModifier's job) and is invisible to every trace/overlap path
	 *  (ignore-all base: cursor ECC_Visibility, projectile WorldStatic terrain
	 *  query by OBJECT type, ECC_Pawn distance math all pass through). Sized and
	 *  positioned by the two Gating tunables below, whose defaults are the
	 *  TASK-350 PIE-VERIFIED gate values for the 3× hollow SM_Castle (loop-2
	 *  ride-along — see GateBlockerRelativeLocation for the co-commit reasoning
	 *  and the honest mesh-only-revert residue).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Gating")
	TObjectPtr<UBoxComponent> GateBlockerVolume;

	/**
	 *  Team interior nav marker (TASK-349, PATHING lane): stamps this castle's
	 *  collision bounds with UNavArea_BlueCastleInterior or
	 *  UNavArea_RedCastleInterior (ctor default is the Blue area, NEVER
	 *  UNavArea_Null, so navmesh GENERATION is untouched in every state). The
	 *  areas cost 1 (normal walk) — the OWN team and un-filtered queries
	 *  (placement projection) treat the interior floor as plain navmesh, while the
	 *  ENEMY team's UNavFilter_Team* EXCLUDES it, so enemy AI never paths inside
	 *  (no door pile-up). Actor component — crumble-swap-proof, like the blocker.
	 *
	 *  Nav-area determinism hardening (loop-2 B3 legs 1–3 + loop-4 B4, all four
	 *  deliberately coexisting):
	 *  (1) the ctor calls ForceNavigationRelevancy(true) so this component owns
	 *  its OWN nav-octree element instead of riding the root CastleMesh's GEOMETRY
	 *  element (UNavRelevantComponent attaches to the owner's root by default —
	 *  which routed our per-hull area list through the engine's raw-geometry
	 *  GetCollisionAreaClass, the `Areas.Num() <= 1` ensure site, and coupled the
	 *  area marking to every mesh-swap/collision-toggle rebuild of the geometry
	 *  element; decoupled, the areas apply through the dynamic-area marking path —
	 *  the NavModifierVolume shape — which supports them properly and survives
	 *  geometry churn). FINAL-RUN-confirmed (ensure absent).
	 *  (2) the TEAM area is selected in PostInitializeComponents, BEFORE the first
	 *  navmesh generation pass — GENERATION-TIME correctness: freshly generated
	 *  tiles build team-correct-first-time (no ctor-Blue → Red flip window).
	 *  (3) ResetCastle and ApplyCrumbleStage re-assert the modifier's octree entry
	 *  in the SAME frame as their mesh swap (RefreshNavigationModifiers), so every
	 *  tile rebuilt by the swap gathers the team area — no Play-Again enemy-open
	 *  window. FINAL-RUN-confirmed (the crumble fence re-marked the red hall).
	 *  (4) loop-4 B4 (Jonathan-authorized): ConfigureTeamGating at BeginPlay runs
	 *  an UNCONDITIONAL RefreshNavigationModifiers — the PRE-BUILT-TILE re-mark.
	 *  Leg 2 removed the only post-registration area CHANGE, so editor-built and
	 *  SAVED tiles (every real boot lane; the editor world never runs the runtime
	 *  hooks, its tiles are always ctor-Blue) were never dirtied and stayed stale
	 *  UNBOUNDED (the 626-sample/211 s FINAL-RUN probe). The startup refresh
	 *  forces those tiles to rebuild once against the already-correct team area;
	 *  on the fresh-build lane it is at most one redundant re-mark. Legs 2 and 4
	 *  answer DIFFERENT lanes — do not fold either into the other.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle|Gating")
	TObjectPtr<UNavModifierComponent> InteriorNavModifier;

	/** Which team owns this castle. Set per level instance (Castle_Blue = Blue, Castle_Red = Red). M8 (TASK-356, doc §3.1): replicated COND_InitialOnly as a belt — the value is level-authored identically on both machines already (the CASTLE-3X gating reads it client-side, addendum §2). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle")
	ETeamId Team = ETeamId::Blue;

	/** Maximum hit points (GDD §3.9). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "1"))
	float MaxHP = 2000.0f;

	/**
	 *  Half-extent (XY) of this castle's spawn box, read by IsPointInSpawnBox (W1
	 *  TASK-275, Shield Wall ATTACK command). PAIRED-TUNABLE (3-way law, CONVENTIONS):
	 *  ACastle::SpawnBoxHalfExtent ≡ ASiegePlayerController::SpawnBoxHalfExtent ≡
	 *  ASiegeBotController::SpawnBoxHalfExtent — all default (2460,2460); keep the three
	 *  in lockstep. Re-derived 840 → 2460 by TASK-349 (CONVENTIONS "Castle 3× HOLLOW"
	 *  paired-tunable law: half-extent ≈ the 3× castle's full 2460 width, preserving the
	 *  original intent), which makes the box span the castle's now-walkable INTERIOR —
	 *  spawn-inside works by construction. The value appears in 3 places (flagged in
	 *  CONVENTIONS); a future pass MAY delegate both controllers to this castle helper —
	 *  OUT of scope here. The mid ACaptureZone::ZoneHalfExtent deliberately STAYS (840,840).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Castle", meta = (ClampMin = "0"))
	FVector2D SpawnBoxHalfExtent = FVector2D(2460.f, 2460.f);

	/**
	 *  GateBlockerVolume center, relative to the castle root (TASK-349 Gating
	 *  tunable — component data, so neither the 3× mesh import nor a crumble swap
	 *  can strip it). Default (6, −525, 284) is the TASK-350 PIE-VERIFIED gate
	 *  position for the AUTHORED 3× hollow SM_Castle (the gate corridor mouth on
	 *  the local −Y side; hero blocked at the enemy gate / passing his own proven
	 *  in-engine on both instances), baked as the C++ default in loop 2 on the
	 *  build-master's flag.
	 *
	 *  WHY baking the 3×-derived value is now SAFE where loop-0's was not
	 *  (co-commit reasoning, QA re-check point): TASK-350 commits this code and
	 *  the 3× mesh IN THE SAME SESSION — the defaults and the mesh they were
	 *  measured against land together, so no committed world pairs these values
	 *  with the old solid mesh the way loop-0's uncoordinated (1221, 0, 300)
	 *  default did.
	 *
	 *  HONEST RESIDUE (the failure path's known, accepted cost): a future
	 *  MESH-ONLY revert to the old solid 814.5×820.6×894.9 castle would re-create
	 *  a mis-placed blocker — this box spans Y [−660, −390] against that mesh's
	 *  ±410 half-width, i.e. ~250 uu proud of its −Y face: a 520-wide × 250-deep ×
	 *  452-tall enemy-only bump flush against that wall, holding enemy melee on
	 *  that one strip ~250 uu out of range (localized stall, not match-breaking;
	 *  every other face unaffected). Any such revert must retune these two
	 *  tunables with the mesh. EditAnywhere stays — per-instance facing/offset
	 *  corrections remain TASK-350's lever.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Castle|Gating")
	FVector GateBlockerRelativeLocation = FVector(6.f, -525.f, 284.f);

	/**
	 *  GateBlockerVolume half-extents (TASK-349 Gating tunable). Default
	 *  (260, 135, 226) is the TASK-350 PIE-VERIFIED gate-opening cover for the
	 *  authored 3× castle (X 520 span across the corridor mouth, Y 270 through the
	 *  wall — no capsule tunneling, Z covering floor≈58 up to ≈510, past the
	 *  ≥450 clear height), baked with GateBlockerRelativeLocation in loop 2 (same
	 *  co-commit reasoning and mesh-only-revert residue — see that doc).
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Castle|Gating", meta = (ClampMin = "0"))
	FVector GateBlockerExtent = FVector(260.f, 135.f, 226.f);

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

	/**
	 *  TASK-349 team gating, both lanes, configured from THIS castle's Team at
	 *  BeginPlay (never hardcoded Blue/Red — symmetric for both castles):
	 *  GateBlockerVolume gets its tunable size/position plus the team response
	 *  matrix (object type = own channel; Ignore all, Block ONLY the enemy
	 *  channel), and InteriorNavModifier adopts the team's interior nav area.
	 *  Null-safe on both components — a missing subobject degrades to pre-feature
	 *  behavior, never a crash.
	 */
	void ConfigureTeamGating();

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

	/** Current hit points. Mutated only by TakeDamage and ResetCastle (authority — M8 guards, doc §3.1). Replicated; OnRep_CurrentHP drives the client bar via the existing delegate. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 2000.0f;

	/** True after OnCastleDestroyed has fired; re-armed only by ResetCastle(). Guarantees the event fires exactly once (server). M8: replicated; OnRep_Destroyed applies the client's visual/collision state ONLY — never the win-condition broadcast (doc §3.1). */
	UPROPERTY(ReplicatedUsing = OnRep_Destroyed, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	bool bDestroyed = false;

	/** Highest crumble stage fired so far (0 = pristine, 1/2/3 = 75/50/25% crossed). Monotonic; reset to 0 by ResetCastle. (TASK-157) M8: replicated; ApplyCrumbleStage is ABSOLUTE (addendum §3), so a join-in-progress client lands the final stage in one OnRep. */
	UPROPERTY(ReplicatedUsing = OnRep_CrumbleStage, VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	int32 CrumbleStage = 0;

	/**
	 *  CLIENT HP arrival (M8, doc §3.1): fires the EXISTING OnCastleHPChanged with
	 *  the replicated value — the same broadcast every server-side mutation site
	 *  makes, so the bar/HUD path is byte-identical (zero widget changes).
	 */
	UFUNCTION()
	void OnRep_CurrentHP();

	/**
	 *  CLIENT destroyed-state arrival (M8, doc §3.1; name per the ratified bool
	 *  law — `bDestroyed` → `OnRep_Destroyed`): applies the shared visual/
	 *  collision half (ApplyDestroyedState) for BOTH edges (destroy + Play-Again
	 *  restore). NEVER broadcasts OnCastleDestroyed — that is the server
	 *  win-condition hook; match end reaches this machine via the GameState rep
	 *  (doc §3.4).
	 */
	UFUNCTION()
	void OnRep_Destroyed();

	/**
	 *  CLIENT crumble arrival (M8, doc §3.1 + addendum §3): stage > 0 →
	 *  ApplyCrumbleStage(stage) — absolute, so intermediate stages may be skipped
	 *  safely; stage == 0 (Play-Again reset) → ApplyTeamVisuals() restores the
	 *  pristine mesh + team material (ApplyCrumbleStage guards 1..3 and can never
	 *  un-crumble — the addendum-pinned branch).
	 */
	UFUNCTION()
	void OnRep_CrumbleStage();

	/**
	 *  The shared visual/collision half of destruction (M8 refactor, doc §3.1):
	 *  hide/show the actor, disable/enable collision (the gate blocker rides the
	 *  actor state on both machines — addendum §2), hide/show the HP bar (screen-
	 *  space widgets do not follow actor hidden state — TASK-018). Called by the
	 *  server paths (HandleDestroyed / ResetCastle) AND by OnRep_Destroyed, so
	 *  both machines run the identical state change; the server-only halves
	 *  (heal-stop, sting, win broadcast, HP/crumble resets) stay in their owners.
	 */
	void ApplyDestroyedState(bool bNowDestroyed);

	/** HP still to be delivered by the running Masons heal-over-time (0 = none). Mutated only by HealOverTime / HandleHealTick / StopHealOverTime. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealRemaining = 0.0f;

	/** HP delivered per heal tick, derived from Total/Duration on HealOverTime (TASK-059). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Castle", meta = (AllowPrivateAccess = "true"))
	float HealPerTick = 0.0f;

	/** Repeating heal-over-time timer handle (Masons repair). Cleared by StopHealOverTime. */
	FTimerHandle HealTimerHandle;
};
