// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "Building.generated.h"

class UDataTable;
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
 *    no friendly fire — the ACastle/ASummonedUnit receiver pattern), takes
 *    LISTED damage from everything else (the §3.0 projectile-50% scaling lives
 *    ONLY in ACastle::TakeDamage — M2 ruling), and at 0 HP destroys the actor
 *    (crumble FX is M7). Destroy() unregisters VisualMesh from the navigation
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
class GITCLAUDEUNREALTEST_API ABuilding : public AActor, public ITeamAgent
{
	GENERATED_BODY()

public:

	ABuilding();

	//~ Begin ITeamAgent interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent interface

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
	 *  friendly fire, GDD §3.0). Buildings take LISTED damage from every damage
	 *  type — the §3.0 damage-vs-castle scaling applies ONLY in ACastle (M2
	 *  ruling / TASK-026). At 0 HP the actor is destroyed (§3.7; crumble FX M7).
	 */
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

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

protected:

	/** Binds the card stats from DT_Cards (HP; subclasses hook OnStatsLoaded for more). */
	virtual void BeginPlay() override;

	/**
	 *  Subclass stat hook, called exactly once, right after the base bound HP
	 *  from the row — the single table load is shared. The base implementation
	 *  is EMPTY on purpose: plain buildings (Wall) bind nothing further and run
	 *  no timers (qa/TASK-021 WARN-1). ATower overrides this to bind
	 *  Damage/Range/Cadence and arm its fire loop behind the Cadence > 0 guard.
	 */
	virtual void OnStatsLoaded(const FCardRow& Row);

	/**
	 *  Root, collision, and visual slot in one (§3.7 "physically collides").
	 *  Explicit BlockAll + navigation-relevant — see the class comment. The BP
	 *  child assigns SM_<CardID> and the team material (TASK-035); no mesh is
	 *  set in C++ (CONVENTIONS: component named exactly VisualMesh).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Building")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** DT_Cards row name whose stats drive this building (BP children preset it: ArrowTower / Wall, TASK-035). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Building")
	FName CardID = NAME_None;

	/** Team this building fights for. Spawner sets it per building (player placements are Blue, TASK-030). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Card stat table (GDD §3.0). Resolved null-safe at BeginPlay — same soft path as ASummonedUnit (TASK-004). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Building")
	TSoftObjectPtr<UDataTable> CardTableAsset;

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

	/** True once the card stats were bound from DT_Cards. */
	bool bStatsLoaded = false;
};
