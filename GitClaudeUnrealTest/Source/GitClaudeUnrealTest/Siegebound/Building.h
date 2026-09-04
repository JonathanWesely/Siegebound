// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/TeamId.h"
#include "Building.generated.h"

class UDataTable;
class UCombatantHealthBarComponent;
class USiegeHitFlashComponent;
class USiegeMeshJuiceComponent;
class UStaticMeshComponent;
struct FCardRow;

/**
 *  Siegebound placeable building base (GDD §3.7, TASK-027). Wall is this class
 *  directly (BP_Building_Wall); Arrow Tower is the ATower subclass
 *  (BP_Building_ArrowTower). Placed on the owner's half by the card-play flow
 *  (TASK-030).
 *
 *  - Stats: HP binds at BeginPlay from the /Game/Data/DT_Cards row named
 *    CardID — NEVER hardcoded (GDD §3.0; ArrowTower 150, Wall 300). A missing
 *    table/row/CardID is logged as an error and the building stands with no
 *    stats — CurrentHP 0, so it dies to the first enemy hit rather than
 *    standing invincible (the ASummonedUnit failure-mode precedent).
 *  - Destructible (§3.7): TakeDamage ignores same-team damage entirely (§3.0
 *    no friendly fire — the ACastle/ASummonedUnit receiver pattern), scales
 *    USiegeDamageType_Siege to 200% (Siege units batter fortifications — M4
 *    ruling, TASK-054) and takes LISTED damage from every other type (the §3.0
 *    projectile-50% scaling stays castle-ONLY — M2 ruling), and at 0 HP destroys
 *    the actor (crumble FX is M7). Destroy() unregisters VisualMesh from the navigation
 *    octree, so the dynamic navmesh heals and units path through the gap a
 *    dead wall leaves.
 *  - Stationary, tickless: the BASE never starts a timer of any kind —
 *    qa/TASK-021-report.md WARN-1 is binding: Wall's Cadence is 0 by design,
 *    and FTimerManager::SetTimer with a rate <= 0 CLEARS a timer instead of
 *    scheduling it. Only ATower arms a cadence loop, behind its own
 *    Cadence > 0 guard, via the OnStatsLoaded hook.
 *  - Collision (§3.7 "physically collides"): VisualMesh is the root with an
 *    EXPLICIT BlockAll profile (the ACastle precedent, qa/TASK-002) — blocks
 *    Pawns so units/hero physically stop, and it keeps the closest-point reach
 *    tests of hero melee / unit attacks / projectiles working against
 *    buildings (they query ECC_Pawn — TASK-003/004/026; qa/TASK-026 ruling 2
 *    counts on "buildings block Pawns"). bCanEverAffectNavigation(true) makes
 *    a runtime-placed wall dirty the nav octree so — with
 *    RuntimeGeneration=Dynamic (Config/DefaultEngine.ini, this task) — the
 *    navmesh carves and units reroute (§3.7 "dynamically updates the navmesh").
 *  - VisualMesh carries NO mesh in C++ — the BP child assigns SM_<CardID> and
 *    the team material (TASK-035; CONVENTIONS per-card visual contract).
 *
 *  Spawners (TASK-030): prefer SpawnActorDeferred → InitBuilding(Team, CardID)
 *  → FinishSpawning (the TASK-007 unit pattern); InitBuilding also late-binds
 *  after a plain SpawnActor. BP children preset their CardID, so level-placed
 *  instances bind with no call at all.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ABuilding : public AActor, public ITeamAgent, public IHealthBarProvider
{
	GENERATED_BODY()

public:

	ABuilding();

	//~ Begin ITeamAgent interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent interface

	/** Fired on every ACTUAL HP change (spawn-init, damage, destruction) — drives the overhead bar (UCombatantHealthBarWidget) via the castle-parity PUSH model (TASK-130, mirrors FOnCastleHPChanged). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Building")
	FOnCombatantHPChanged OnHPChanged;

	//~ Begin IHealthBarProvider Interface (TASK-130 push model) — forwards to the EXISTING getters + the OnHPChanged delegate; adds NO HP state.
	virtual FOnCombatantHPChanged& GetHPChangedDelegate() override { return OnHPChanged; }
	virtual float GetHealthCurrent() const override { return GetCurrentHP(); }
	virtual float GetHealthMax() const override { return GetMaxHP(); }
	virtual bool IsHealthBarActorAlive() const override { return !IsBuildingDestroyed(); }
	//~ End IHealthBarProvider Interface

	/**
	 *  Spawner hook (TASK-030, mirroring the TASK-007/ASummonedUnit::InitUnit
	 *  pattern): sets the team and the card row this building's stats come from.
	 *  Call between SpawnActorDeferred and FinishSpawning (preferred), or right
	 *  after a plain SpawnActor — if BeginPlay already ran without a usable
	 *  CardID, this binds the stats now. The CardID cannot be changed once stats
	 *  are bound (Team still updates). BP children (TASK-035) preset their
	 *  CardID, so spawners may also spawn those and call this for the Team alone.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Building")
	void InitBuilding(ETeamId InTeam, FName InCardID);

	/**
	 *  Applies incoming damage. Same-team damage is ignored entirely (no
	 *  friendly fire, GDD §3.0). USiegeDamageType_Siege scales to 200% (Siege
	 *  units batter fortifications — M4 ruling, TASK-054); every other type takes
	 *  LISTED damage (the §3.0 projectile-50% rule stays castle-ONLY — M2 ruling /
	 *  TASK-026). Returns the SCALED amount applied. At 0 HP the actor is
	 *  destroyed (§3.7; crumble FX M7).
	 */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 *  FrostNova spell freeze (TASK-099, M5 ruling 5; called by USpellLibrary on
	 *  enemy buildings in the reticle radius — the castle is NOT an ABuilding and
	 *  deliberately carries NO freeze API). STATE ONLY here (ruling 14): this
	 *  latches IsFrozen() for Seconds and nothing else — the tower fire-gate that
	 *  consumes it lands in TASK-101 (ATower checks !IsFrozen() on both its
	 *  projectile and chain paths). Refresh-not-stack (ruling 5): re-applying
	 *  arms the single expiry timer for max(remaining, Seconds) — never additive.
	 *
	 *  MATCH-END PRECEDENCE (ruling 5): the match-end freeze silences towers by
	 *  clearing ALL their timers (ASiegeGameMode::FreezeWorldAtMatchEnd), which
	 *  also clears this expiry — a spell-frozen tower stays IsFrozen() until Play
	 *  Again destroys it, and no expiry can fire post-match. On non-tower
	 *  buildings an expiry after match end only flips this state flag — it
	 *  resumes NOTHING (state-only), so precedence holds there too. No-op on
	 *  destroyed buildings and non-positive Seconds. Virtual so a subclass with
	 *  its own drives (e.g. a future Barracks spawn-pause) can extend it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Building")
	virtual void ApplyFreeze(float Seconds);

	/** True while a FrostNova spell freeze is active (TASK-099). TASK-101 gates ALL tower firing on !IsFrozen(). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	bool IsFrozen() const { return bSpellFrozen; }

	/** Current hit points, in [0, MaxHP]. PIE verification hook (TASK-010/018 precedent). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	float GetCurrentHP() const { return CurrentHP; }

	/** Maximum hit points, bound from the DT_Cards row (ArrowTower 150, Wall 300). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	float GetMaxHP() const { return MaxHP; }

	/** True once HP reached 0 (the actor is being destroyed — buildings don't rebuild). Closes the same-frame window before Destroy lands. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	bool IsBuildingDestroyed() const { return bDestroyed; }

	/** Card row this building's stats were (or will be) bound from. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	FName GetCardID() const { return CardID; }

	//~ ══════════════════════════════════════════════════════════════════════════════════════
	//~  STACK-§ — TOWER STACKING: the two series, the exclusion predicate, the upgrade
	//~  (TASK-812). ⛔ NO placement code lives here — the ghost, the blue state, the cost and
	//~  the wheel are TASK-813 / TASK-815 and stay in ASiegePlayerController.
	//~ ══════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  ⭐⭐ THE HEIGHT SERIES — ⛔ ADDITIVE, ⛔ NOT DOUBLING, AND THE ARITHMETIC IS **RULED**
	 *  (STACK-§1, ruling J-0): returns `min(1 + UpgradeCount, MaxMultiplier)`.
	 *  Every upgrade adds ⭐ ONE MORE COPY OF THE **ORIGINAL** height — ×2, ×3, ×4, ×5 — and
	 *  the series SATURATES at the cap.
	 *
	 *  ⚠️⚠️ DO ⛔ NOT "CORRECT" THIS TOWARD JONATHAN'S SUMMARY SENTENCE ("it basically gains
	 *  **twice the current height** every upgrade"). That is not a taste call, and the
	 *  argument is his own cap:
	 *
	 *    ⛔ Under DOUBLING the reachable heights are ×2, ×4, ×8, ×16 — his stated maximum of
	 *       ⭐ "5 times taller" is ⛔ **NEVER REACHED**, so the cap sentence he wrote would
	 *       describe a state the game can ⛔ never enter.
	 *    ✅ Under THIS series ×5 lands ⛔ EXACTLY, on the 4th upgrade, and it matches all four
	 *       terms he enumerated one sentence earlier ("twice as tall … 3 times taller … 4
	 *       times taller … and so on").
	 *
	 *  ⇒ one reading makes his cap sentence mean something and the other makes it dead text.
	 *  🧑 The loose sentence is FLAGGED to him as `J-0` rather than silently discarded — one
	 *  word switches height to doubling if that is what he meant. ⭐ Note precisely: the
	 *  HEALTH half of that same summary sentence is CORRECT under both readings and is
	 *  shipped verbatim — see StackHealthMultiplier.
	 *
	 *  ⭐ INTEGER ARITHMETIC, then ONE widening to float: the cap must land on the ruled
	 *  multiple exactly, and a float accumulation cannot promise that. A negative
	 *  UpgradeCount is not a state the game can enter (StackUpgradeCount only ever
	 *  increments from 0) but this seam is public and pure, so it answers for the whole
	 *  int32 domain instead of trusting its callers: no upgrades ⇒ exactly 1.0.
	 *
	 *  ⛔⛔ THE CEILING IS A **PARAMETER**, AND THAT IS ⛔ NOT A TIDY-UP (STACK-§7's amended
	 *  row, STACK-§10, ruling J-13). ⚠️⚠️ THIS SIGNATURE WAS ⛔ ONE PARAMETER UNTIL 2026-09-03
	 *  AND THE OLD SHAPE HAD A DEFECT THAT ⛔ NOTHING COULD SEE: it read the cap off
	 *  `GetDefault<ABuilding>()`, i.e. ⛔ ALWAYS `ABuilding`'s CDO and ⛔ never the calling
	 *  instance's class. ⇒ a subclass that set `MaxStackHeightMultiplier` in its constructor
	 *  would have been ⛔ READ RIGHT PAST — the series would have kept climbing to the base
	 *  class's ceiling while every readback of the subclass's own tunable reported the number
	 *  the designer typed. ⭐ For `AClimbableTower` that is not a cosmetic slip: past its
	 *  MEASURED ceiling the deck-breach window can no longer cross the deck slab and the tower
	 *  becomes ⛔ unclimbable (STACK-§10 cl. 1). ⇒ ⛔ **the second parameter is the ⛔ ONLY
	 *  thing that makes a per-class ceiling REAL**, and collapsing it back to one parameter
	 *  silently restores the defect.
	 *
	 *  ⭐ THE CAP IS STILL STORED IN EXACTLY ⛔ ONE PLACE — the `MaxStackHeightMultiplier`
	 *  UPROPERTY below, reached by callers through GetMaxStackHeightMultiplier(). ⛔ This
	 *  function holds ⛔ no copy of it and ⛔ no fallback literal: passing a garbage cap
	 *  degrades to the identity series, ⛔ never to a restated `5`.
	 *
	 *  ⛔ public, plain C++ static, ⛔ NOT a UFUNCTION, exactly TWO parameters, ⛔ none
	 *  defaulted (SC-§33, STACK-§7). ⛔ No world, ⛔ no actor INSTANCE and now ⛔ not even a
	 *  CDO read ⇒ headlessly testable with ⛔ no PIE, the
	 *  ASummonedUnit::HeightAdvantageMultiplier / FSiegeMapMark::MakeSymbol precedent, and
	 *  ⭐ a test can now pass a cap the project does not ship and assert the SHAPE rather than
	 *  the literal (SC-§37). Tested in Tests/SiegeBuildingStackTest.cpp.
	 */
	static float StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier);

	/**
	 *  ⭐ THE HEALTH SERIES — ⛔ MULTIPLICATIVE and ⛔ UNCAPPED (STACK-§1): returns
	 *  `StackHealthStep ^ UpgradeCount`, i.e. ×1.5 of the CURRENT health per upgrade.
	 *
	 *  ✅ THIS HALF NEEDED NO RULING — HIS OWN NUMBERS CONFIRM IT TO THE DIGIT: he wrote
	 *  "2.25 times the original health" (= 1.5²) and "3.375 times the health" (= 1.5³), and
	 *  ⭐ BOTH readings of his sentence agree here. It is shipped verbatim.
	 *
	 *  ⛔⛔ AND IT IS ⛔ DELIBERATELY A DIFFERENT KIND OF SERIES FROM THE HEIGHT ONE — one
	 *  saturates, one does not, and his closing sentence is what says so: "at some point if
	 *  they keep upgrading it would only upgrade health by 1.5 times and not height." ⇒ at
	 *  the height cap this keeps climbing, which is exactly what makes the click still worth
	 *  making (STACK-§5 `J-6`).
	 *
	 *  ⭐ COMPUTED BY REPEATED MULTIPLICATION, ⛔ NOT FMath::Pow: 1.5 and its low powers are
	 *  EXACTLY representable in binary32 (numerator 3ⁿ, denominator 2ⁿ ⇒ exact through
	 *  n = 15), so the shipped terms are his numbers bit-for-bit rather than powf's rounding
	 *  of them. The loop stops early on a non-finite accumulator so a pathological
	 *  UpgradeCount can never spin.
	 *
	 *  ⛔ public, plain C++ static, ⛔ NOT a UFUNCTION, exactly ONE parameter, ⛔ none
	 *  defaulted (SC-§33, STACK-§7). Tested in Tests/SiegeBuildingStackTest.cpp.
	 */
	static float StackHealthMultiplier(int32 UpgradeCount);

	/**
	 *  ⛔⛔ THE **WHEEL** (X/Y) PREDICATE, AND ⛔ NOTHING ELSE SINCE 2026-09-03 (STACK-§8).
	 *  May this building's FOOTPRINT be scaled in X and Y by the placement wheel
	 *  (ASiegePlayerController::ApplyPlacementFootprintWheel / CanCardActorScaleFootprint)?
	 *  Default true. ⛔ AClimbableTower overrides FALSE and ⛔ that exclusion is NOT reopened:
	 *  an X/Y scale of SM_WatchTower moves the LadderFoot/LadderTop sockets ⛔ sideways, off
	 *  the climb line TOWER-§8.3 pinned, and fires TOWER-§8.5a's voiding condition ⇒ the climb
	 *  stops working entirely (STACK-§2). ⛔ NOT a style choice, ⛔ NOT a name check.
	 *
	 *  ⛔⛔⛔ THIS PREDICATE IS ⛔ NO LONGER THE STACK GATE, AND CONSULTING IT FROM A STACK
	 *  SITE IS AN ⛔ AUTOMATIC FAIL (STACK-§7's amended row, STACK-§8 cl. 3). ⚠️⚠️ IT WAS THE
	 *  STACK GATE UNTIL 2026-09-03, AND THAT IS THE ⛔ DEFECT 🧑 JONATHAN FILMED: the height
	 *  (Z) question and the footprint (X/Y) question were ⛔ ONE virtual answering ⛔ BOTH, so
	 *  the ⛔ one building in the project that refuses the wheel also refused to be stacked,
	 *  and the ONLY combination that could reach the refusal in normal play was ⛔ exactly the
	 *  one he tried: a WatchTower card on his own WatchTower. ⇒ ⭐ the height question now has
	 *  its OWN virtual — see CanStackHeight() below.
	 *
	 *  ⛔⛔ A `CardID == "WatchTower"` STRING COMPARE ANYWHERE IN THE PLACEMENT PATH IS AN
	 *  AUTOMATIC FAIL. ⚖️ The next climbable building must be protected by ⛔ INHERITING,
	 *  ⛔ not by somebody remembering a paragraph — which is the entire reason this is a
	 *  virtual on the base rather than a check at the call site.
	 */
	virtual bool CanScaleFootprint() const { return true; }

	/**
	 *  ⭐⭐ THE **STACK** (Z) PREDICATE — ⛔ BORN 2026-09-03 (STACK-§8 cl. 3, STACK-§7's new
	 *  row). May this building's HEIGHT be grown by the STACK-§ upgrade? Default true.
	 *
	 *  ⛔⛔⛔ IT IS A ⛔ SIBLING OF CanScaleFootprint(), ⛔ NEVER AN ALIAS AND ⛔ NEVER A
	 *  WRAPPER. `{ return CanScaleFootprint(); }` here would reproduce the shipped defect
	 *  behind a new name and is an ⛔ AUTOMATIC FAIL (STACK-§8 cl. 3). ⛔ Two virtuals, ⛔ two
	 *  INDEPENDENT answers — and the proof that they are independent is that they ⛔ DISAGREE
	 *  on AClimbableTower, which refuses the wheel and ⭐ ACCEPTS the stack.
	 *
	 *  ⛔ WHY THE TWO QUESTIONS ARE GENUINELY DIFFERENT, MEASURED AND ⛔ NOT ASSERTED
	 *  (TASK-941, ruled as STACK-§10): a Z-only scale is ⛔ not the same transform as an X/Y
	 *  one for a ladder that is a VERTICAL feature. The climb line's STANDOFF — the number
	 *  TOWER-§8.3 gates on — was measured at Z-scale n ∈ {1..5} and it ⭐ IMPROVES
	 *  monotonically (103.32 → 117.15 uu, clearing the ≥ 98.0 gate at every n), and the rung
	 *  PLANE's −22.0 uu depth — the thing the hands grip — survives with a bounded +3.0 %
	 *  drift. ⇒ ⛔ STACK-§2's "a non-uniform scale preserves nothing" is MEASURED FALSE for
	 *  the Z axis. An X/Y scale has ⛔ no such measurement and is ⛔ not reopened.
	 *
	 *  ⚠️⚠️ WHAT DOES BIND IS THE ⛔ CEILING, ⛔ NOT THE PREDICATE, and it lives in
	 *  MaxStackHeightMultiplier per class rather than in a second `false` here — see
	 *  AClimbableTower's override and the tunable's own comment for the arithmetic.
	 *
	 *  ⛔ Same structural discipline as its sibling: a virtual on the base, so the NEXT
	 *  climbable building inherits the answer, and ⛔ no CardID string compare anywhere.
	 */
	virtual bool CanStackHeight() const { return true; }

	/**
	 *  This building's own height ceiling, in multiples of its AUTHORED height — the ⛔ ONE
	 *  storage of it (the MaxStackHeightMultiplier UPROPERTY), read through one accessor so
	 *  callers never need a second copy. Passed to StackHeightMultiplier by every consumer,
	 *  which is what makes the ceiling PER CLASS rather than game-wide (STACK-§10 cl. 2).
	 */
	int32 GetMaxStackHeightMultiplier() const { return MaxStackHeightMultiplier; }

	/** Upgrades applied to this building (STACK-§1's `n`) — the ⛔ ONE source of truth both multipliers derive from. 0 on a freshly placed building. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	int32 GetStackUpgradeCount() const { return StackUpgradeCount; }

	/**
	 *  ⭐⭐ THE UPGRADE, APPLIED — the ⛔ ONE mutator of StackUpgradeCount, and the ⛔ ONE
	 *  place a stacked building's height and health change. Returns true if the upgrade
	 *  landed. Called by the server-side confirm path (TASK-813); ⛔ this class starts
	 *  nothing and polls nothing.
	 *
	 *  WHAT IT DOES, IN ORDER:
	 *    1. ⛔ REFUSES on !HasAuthority(), on a destroyed building, and on
	 *       ⛔ !CanStackHeight() — ⛔ the STACK (Z) predicate, ⛔ NOT CanScaleFootprint(),
	 *       which is the WHEEL's since STACK-§8. ⭐ The predicate is re-asked HERE as well
	 *       as in the placement path on purpose: a future caller that forgets STACK-§2 must
	 *       ⛔ still be unable to grow a building that refuses it. Two independent
	 *       mechanisms, not one.
	 *    2. HEIGHT — VisualMesh's ⛔ **Z ONLY** (STACK-§5 `J-4`, his own words "keeping the
	 *       same width and length": X/Y belong to the placement wheel and are inherited
	 *       VERBATIM), recomputed as `AuthoredHeightScaleZ × StackHeightMultiplier(n, cap)`
	 *       from the AUTHORED baseline ⇒ ⛔ no float accumulation, and the cap lands on the
	 *       ruled multiple exactly however many times this runs. ⭐ The cap passed is ⛔ THIS
	 *       INSTANCE'S own MaxStackHeightMultiplier, which is what makes the ceiling PER
	 *       CLASS (STACK-§10 cl. 2).
	 *    3. HEALTH (STACK-§5 `J-10`) — `MaxHP ×= 1.5`, then `CurrentHP += (NewMax − OldMax)`.
	 *       ⭐ It grants the ⛔ NEW hit points; it does ⛔ NOT repair existing damage. ⚖️ A
	 *       full heal would make the upgrade a repair tool, which is the Masons card's job,
	 *       and would make upgrading strictly better than defending.
	 *    4. Pushes the result through the ⛔ EXISTING OnHPChanged delegate — ⛔ no second
	 *       push, ⛔ no direct widget call (the TASK-130 push model, unchanged).
	 *    5. Calls the OnStackUpgradeApplied() subclass hook — ⛔ ONLY on the success path,
	 *       ⛔ after the transform is written. This base class knows ⛔ nothing about what a
	 *       subclass needs to re-arm; see that hook's own comment.
	 *
	 *  ⭐ COLLISION AND NAVMESH COME FOR FREE and are ⛔ not re-derived here: VisualMesh is
	 *  the root with BlockAll + bCanEverAffectNavigation(true), so a scaled component carves
	 *  a scaled hole with ⛔ zero new code (STACK-§3, measured).
	 *
	 *  ⛔ DELIBERATELY ⛔ NOT A UFUNCTION, unlike this class's other mutators. M8 (STACK-§7):
	 *  StackUpgradeCount is ⛔ AUTHORITATIVE GAME STATE — it drives MaxHP — so it is
	 *  SERVER-SET at confirm and ⛔ the client may ⛔ never author it. The HasAuthority guard
	 *  is the belt; ⛔ not shipping a Blueprint-callable entry point onto it is the braces.
	 */
	bool ApplyStackUpgrade();

protected:

	/** Binds the card stats from DT_Cards (HP; subclasses hook OnStatsLoaded for more). */
	virtual void BeginPlay() override;

	/** Clears the spell-freeze expiry timer (TASK-099) — no dangling expiry on a destroyed building (belt-and-braces; ATower::EndPlay chains here via Super). */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  Subclass stat hook, called exactly once, right after the base bound HP
	 *  from the row — the single table load is shared. The base implementation
	 *  is EMPTY on purpose: plain buildings (Wall) bind nothing further and run
	 *  no timers (qa/TASK-021 WARN-1). ATower overrides this to bind
	 *  Damage/Range/Cadence and arm its fire loop behind the Cadence > 0 guard.
	 */
	virtual void OnStatsLoaded(const FCardRow& Row);

	/**
	 *  ⭐⭐ SUBCLASS HOOK, CALLED BY ApplyStackUpgrade ⛔ ONLY WHEN AN UPGRADE ACTUALLY
	 *  LANDED, ⛔ after the Z transform and the HP push. The base implementation is EMPTY on
	 *  purpose — the OnStatsLoaded precedent directly above, same shape and same reason.
	 *
	 *  ⛔⛔ WHY A HOOK RATHER THAN LETTING A SUBCLASS OVERRIDE ApplyStackUpgrade ITSELF:
	 *  ApplyStackUpgrade is the ⛔ ONE authoritative mutator of StackUpgradeCount/MaxHP and
	 *  its M8 discipline (server-only, ⛔ deliberately not a UFUNCTION, ⛔ deliberately not
	 *  virtual) is the belt and braces STACK-§7 asked for. ⇒ a subclass extends the
	 *  CONSEQUENCES of an upgrade and ⛔ never its RULES.
	 *
	 *  ⚠️⚠️ AND THE REASON THIS HOOK EXISTS AT ALL, SO NOBODY DELETES IT AS DEAD WEIGHT: a
	 *  runtime rescale of the root scene component refreshes ⛔ that component's navigation
	 *  octree entry and ⛔ nothing else. A subclass carrying a `UActorComponent`-derived
	 *  navigation element — which is ⛔ NOT a USceneComponent and therefore ⛔ NOT reached by
	 *  USceneComponent::PropagateTransformUpdate — has to re-arm it itself, or the element
	 *  stays registered at the pre-upgrade geometry. ⛔ That failure is ⛔ INVISIBLE to a
	 *  player-controlled test: only the AI path consults the navmesh.
	 *
	 *  ⛔ This base class names ⛔ no navigation type and holds ⛔ no navigation call — the
	 *  knowledge belongs to whichever subclass has the element (STACK-§10 cl. 5).
	 */
	virtual void OnStackUpgradeApplied();

	/**
	 *  Root, collision, and visual slot in one (§3.7 "physically collides").
	 *  Explicit BlockAll + navigation-relevant — see the class comment. The BP
	 *  child assigns SM_<CardID> and the team material (TASK-035); no mesh is
	 *  set in C++ (CONVENTIONS: component named exactly VisualMesh).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Building")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** Overhead poll-driven health bar (M5.5, TASK-110): hide-at-full, team-tinted. Added at the base so ATower/ABarracks/ADeepMine/Wall inherit it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Building")
	TObjectPtr<UCombatantHealthBarComponent> HPBarWidget;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based, null-safe. Inherited by ATower/ABarracks/ADeepMine/Wall. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/** §6 procedural transform juice (M7, TASK-155): spawn squash-and-stretch (all buildings) + tower recoil (ATower calls PlayRecoil). Protected so ATower reaches it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeMeshJuiceComponent> MeshJuiceComponent;

	/** DT_Cards row name whose stats drive this building (BP children preset it: ArrowTower / Wall, TASK-035). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Building")
	FName CardID = NAME_None;

	/** Team this building fights for. Spawner sets it per building (player placements are Blue, TASK-030). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Card stat table (GDD §3.0). Resolved null-safe at BeginPlay — same soft path as ASummonedUnit (TASK-004). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Building")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	/**
	 *  ⭐ THE HEIGHT CEILING, IN MULTIPLES OF THE AUTHORED HEIGHT (STACK-§1 / STACK-§7) —
	 *  Jonathan's "the maximum height it can reach is 5 times taller", as a number he can
	 *  retune rather than a literal buried in the series.
	 *
	 *  ⛔ INTEGER on purpose, and the reason is the whole ruling: the height series is
	 *  ADDITIVE, so the cap is REACHED EXACTLY — on the (Cap − 1)ᵗʰ upgrade — and an integer
	 *  ceiling is the only kind that can promise "exactly".
	 *
	 *  ⚠️ THE CONSEQUENCE OF RETUNING IT (HIGH-§1): this moves the upgrade at which height
	 *  STOPS growing and health starts being the whole purchase. Raising it to 8 makes a
	 *  ×8 tower legal — with the ×5 UV stretch (STACK-§3 `J-11`, shipped as-is and named)
	 *  getting proportionally worse — and moves the HUD cap note TASK-813 shows. Lowering it
	 *  to 1 disables stacked HEIGHT entirely while leaving the health ladder untouched, which
	 *  is a coherent state, ⛔ not a broken one.
	 *
	 *  ⭐⭐ PER **CLASS**, ⛔ NO LONGER GAME-WIDE (STACK-§7's amended row, STACK-§10 cl. 2,
	 *  ruling J-13) — AMENDED 2026-09-03. Each class sets its own value in its CONSTRUCTOR
	 *  and StackHeightMultiplier is HANDED that value by its callers, which read it from the
	 *  instance through GetMaxStackHeightMultiplier(). ⛔ There is still exactly ONE storage
	 *  of the ceiling — this UPROPERTY — and ⛔ a second copy of it anywhere is a FAIL.
	 *
	 *  ⚠️⚠️ THE OLD COMMENT HERE SAID THE SERIES READ THIS OFF "THIS CLASS'S CDO" AND WOULD
	 *  IGNORE A SUBCLASS'S VALUE. ⛔ THE FIRST HALF WAS ⛔ FALSE AS WRITTEN AND THE SECOND WAS
	 *  ⛔ TRUE BY ACCIDENT: the series read `GetDefault<ABuilding>()` — ⛔ ALWAYS the BASE
	 *  class's CDO — so a subclass ceiling was silently discarded. That is fixed, and the
	 *  fix is the whole reason StackHeightMultiplier took a second parameter.
	 *
	 *  ⛔⛔ AND THE HAZARD THE PER-CLASS SHAPE OPENS, DECLARED RATHER THAN DEFENDED WITH A
	 *  SECOND LITERAL (HIGH-§1): this stays `EditDefaultsOnly`, so a **Blueprint child can
	 *  now RAISE it and be OBEYED**. For an ordinary building that is a balance knob. For a
	 *  CLIMBABLE one it is ⛔ not: past the ceiling its own class measured, the deck-breach
	 *  window can no longer cross the deck slab, the ascent sweep jams below it, and the deck
	 *  becomes UNREACHABLE (STACK-§10 cl. 1). ⇒ ⛔ **a climbable class's ceiling is DERIVED
	 *  from STACK-§10's arithmetic, ⛔ never bumped by hand and ⛔ never raised in a .uasset.**
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Building", meta = (ClampMin = "1"))
	int32 MaxStackHeightMultiplier = 5;

	/**
	 *  ⭐ THE HEALTH STEP PER UPGRADE (STACK-§1 / STACK-§7) — his "1.5 times the health",
	 *  compounding. ⛔⛔ **UNCAPPED BY DESIGN, AND THAT IS HIS EXPLICIT WORD**: "there is no
	 *  maximum on the health." ⇒ there is deliberately ⛔ NO MaxStackHealthMultiplier beside
	 *  this, and inventing one would be silently softening his number.
	 *
	 *  ⚠️ THE CONSEQUENCE OF RETUNING IT (HIGH-§1): it compounds, so a small nudge is not
	 *  small far up the ladder — 1.5 gives ×3.375 at three upgrades, 2.0 gives ×8. Because
	 *  the height cap saturates and this does not, this number alone decides whether stacking
	 *  stays worth its gold past the cap. Setting it to 1.0 makes upgrades past the cap
	 *  literally free of effect, which is a design choice, ⛔ not a crash.
	 *
	 *  ⚠️⚠️ READ OFF `ABuilding`'s **CDO** by StackHealthMultiplier, which keeps its ⛔ ONE
	 *  parameter — ⭐ and that asymmetry with MaxStackHeightMultiplier above is ⛔ deliberate,
	 *  ⛔ not an oversight left behind by the 2026-09-03 amendment. The HEIGHT ceiling went
	 *  per class because a MEASUREMENT forced it: past a climbable tower's own ceiling its
	 *  deck stops being reachable (STACK-§10). ⛔ Nothing analogous exists for health — it is
	 *  UNCAPPED by his explicit word, it touches ⛔ no geometry, and ⛔ no measurement asked
	 *  for a per-class step. ⇒ this one stays ⭐ GAME-WIDE ("there is no maximum on the
	 *  health" is a sentence about the game, ⛔ not a per-card stat), and a BP child that
	 *  re-authored it would be IGNORED by the series. ⛔ Do ⛔ not "finish the refactor" by
	 *  making this per class as well — that would be inventing a rule ⛔ nobody ruled.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Building", meta = (ClampMin = "1.0"))
	float StackHealthStep = 1.5f;

private:

	/**
	 *  Loads DT_Cards and binds the CardID row's stats (HP → max/current HP),
	 *  then fires the OnStatsLoaded subclass hook. Missing table/row/CardID:
	 *  logs an error and leaves the building statless — stats are never
	 *  hardcoded (GDD §3.0).
	 */
	void LoadStats();

	/**
	 *  TASK-044 (CONVENTIONS Team contract): overrides VisualMesh slot 0 with the
	 *  MI_TeamColor matching the building's ACTUAL Team, so a Red-spawned building
	 *  (the M3 bot's tower/wall) recolors at runtime without a Red BP duplicate. The
	 *  BP-authored MI_TeamColor_Blue is only the design-time placeholder — a Blue
	 *  building re-applies the identical Blue instance, so M2 Blue visuals stay
	 *  byte-for-byte. The two MI instances resolve through cached function-local
	 *  statics (never a hot-path load) and are null-safe (a missing asset leaves the
	 *  authored slot, never a crash). Cosmetic only: slot 0 material, nothing else.
	 */
	void ApplyTeamMaterial();

	/** Single-fire destruction: guards on bDestroyed, then destroys the actor (§3.7; the navmesh heals via component unregistration). */
	void HandleDestroyed();

	/**
	 *  Spell-freeze expiry (TASK-099): drops the spell-frozen state — nothing
	 *  more (the building freeze is STATE ONLY; the tower fire path re-checks
	 *  IsFrozen() every shot in TASK-101, so there is nothing to resume here).
	 *  Idempotent timer callback for ApplyFreeze.
	 */
	void EndSpellFreeze();

	/**
	 *  Resolves the attacking team from a damage event — same chain as
	 *  ACastle::TryGetInstigatorTeam / ASummonedUnit::TryGetDamageTeam
	 *  (TASK-002/004): instigating controller's pawn, then the damage causer,
	 *  then the causer's instigator pawn. False if no team found (world damage,
	 *  tower-fired projectiles), in which case damage applies.
	 */
	static bool TryGetDamageTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam);

	/** Maximum hit points, bound from the card row at BeginPlay (ArrowTower 150, Wall 300). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	float MaxHP = 0.f;

	/** Current hit points. Mutated only by TakeDamage. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 0.f;

	/** True from HP hitting 0; destruction side effects run exactly once. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	bool bDestroyed = false;

	/** True while a FrostNova spell freeze is active (TASK-099) — the state TASK-101's tower fire-gate reads through IsFrozen(). */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	bool bSpellFrozen = false;

	/**
	 *  ⭐⭐ STACK-§1's `n` — upgrades applied to THIS building, and the ⛔ ONE SOURCE OF TRUTH
	 *  both series derive from. ⛔ NO second copy of the height scale or the health scale is
	 *  stored anywhere, and ⛔ nothing caches a multiplier: everything is recomputed from this
	 *  integer, which is why the cap can be reached exactly and why a retune of either tunable
	 *  moves live buildings' arithmetic rather than only new ones'.
	 *
	 *  ⛔ M8 (STACK-§7): AUTHORITATIVE GAME STATE — it drives MaxHP — so ONLY the server-side
	 *  ApplyStackUpgrade writes it, and ⛔ the client may ⛔ never author it. ⛔ No new RPC and
	 *  ⛔ no new relevancy tier are added: the resulting HP travels on the ALREADY-SHIPPED
	 *  OnHPChanged push, and the ghost/blue state/wheel are client-local PRE-gate.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	int32 StackUpgradeCount = 0;

	/**
	 *  The building's ORIGINAL VisualMesh Z scale — the baseline StackHeightMultiplier
	 *  multiplies. ⛔ NOT a cached multiplier and ⛔ not a second copy of the height scale:
	 *  it is the "original height" the ruled series is defined AGAINST ("+1× the ORIGINAL per
	 *  upgrade"), without which an additive series cannot be expressed as a transform at all.
	 *
	 *  ⭐ CAPTURED LAZILY, ON THE FIRST UPGRADE (StackUpgradeCount == 0), ⛔ not at BeginPlay —
	 *  and that is deliberate rather than lazy: it makes the baseline independent of the
	 *  spawn/scale ordering the placement path happens to use, and it is safe because the
	 *  placement wheel scales ⛔ X/Y ONLY (STACK-§4), so Z at the first upgrade IS the
	 *  authored Z. 1.0 until then, so an un-upgraded building never depends on it.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Building", meta = (AllowPrivateAccess = "true"))
	float AuthoredHeightScaleZ = 1.f;

	/**
	 *  Drives EndSpellFreeze once the freeze elapses; re-armed at
	 *  max(remaining, new) on re-apply (refresh-not-stack, M5 ruling 5, TASK-099).
	 *  Cleared by EndPlay — and, on towers, by the match-end freeze's
	 *  ClearAllTimersForObject sweep (precedence: a match-end-silenced tower's
	 *  spell freeze never expires).
	 */
	FTimerHandle SpellFreezeTimerHandle;

	/** True once the card stats were bound from DT_Cards. */
	bool bStatsLoaded = false;
};
