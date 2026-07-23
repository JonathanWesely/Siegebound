// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/Actor.h"
#include "Misc/Optional.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "GoldNode.generated.h"

class AGoldNode;
class AMinerUnit;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

/**
 *  Fired exactly once when the mine's reserve hits 0 (the depletion latch).
 *  Registered miners have ALREADY been evicted (NotifyMineDepleted) when this
 *  broadcasts. HUD backlog hook (W1-PREP, TASK-253) — nothing binds it yet.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMineDepleted, AGoldNode*, DepletedMine);

/**
 *  Fired whenever GoldReserve actually changes (drain ticks, InitMine).
 *  InitialReserve rides along so a listener can render Reserve/Initial without
 *  a second query. HUD backlog hook (W1-PREP, TASK-253) — nothing binds it yet.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMineReserveChanged, int32, NewReserve, int32, InitialReserve);

/**
 *  Siegebound gold mine (W1-PREP mirrored-mines redesign, TASK-253 — the
 *  as-built amendment to GDD §3.3/§5; law: CONVENTIONS "Mirrored depleting
 *  mines"). NEUTRAL, DEPLETING, CLAIMABLE: no team is baked in — 6 mines
 *  (3/side, exactly mirrored across X=0) are spawned + InitMine()'d each match
 *  by ASiegeBattlefieldScatter (TASK-255), replacing the two castle-adjacent
 *  per-team nodes (deleted from L_Arena in TASK-257).
 *
 *  Occupancy contract (Jonathan ruling: exclusive occupancy):
 *   - The FIRST miner to arrive claims the mine for its team (atomic on the
 *     0 -> 1 registry transition, TryRegisterArrivedMiner). While claimed,
 *     only that team's miners may register; enemy miners WAIT at the ring
 *     (AMinerUnit wait mode, TASK-254) and auto-claim when the last occupant
 *     leaves or dies (UnregisterArrivedMiner releases on empty).
 *   - Any number of same-team miners may stack on one mine; each drains it.
 *
 *  Depletion contract:
 *   - While occupied, a 1 s drain timer subtracts n_miners x
 *     DrainPerMinerPerSecond from GoldReserve. PAIRED-TUNABLE LAW: see the
 *     DrainPerMinerPerSecond doc below — it must equal
 *     ASiegePlayerState::MinerGoldPerTick.
 *   - At 0 the mine latches bDepleted (one-way for this mine's lifetime;
 *     Play Again destroys and respawns fresh mines via ClearScatter), stops
 *     the timer, evicts every registered miner via
 *     AMinerUnit::NotifyMineDepleted, broadcasts OnMineDepleted, and dims the
 *     glow gauge. Depleted mines refuse all registration.
 *   - Accepted quirk (plan-of-record): occupied mines keep draining briefly
 *     after match end (miners are frozen standing at the ring) — cosmetic,
 *     reset by Play Again; keeps ASiegeGameMode untouched.
 *
 *  Reserve gauge (zero-UI reserve signal): a LAZY transient MID on slot 0
 *  (M_GoldGlow) drives scalar param "GlowIntensity" =
 *  Lerp(GlowIntensityDepleted, GlowIntensityFull, Reserve/Initial). Intensity
 *  MODULATION, never material replacement — the "gold nodes carry M_GoldGlow
 *  regardless of team" law (CONVENTIONS Team contract) holds. Null-safe at
 *  every step: missing mesh / missing slot-0 material / missing param (it
 *  arrives with TASK-257) are silent no-ops — the gauge is cosmetic, never
 *  load-bearing.
 *
 *  Finder: FindBestMineFor is THE single mine finder (miner + bot share it) —
 *  tier-1 nearest mine the team can mine now, tier-2 nearest enemy-occupied
 *  non-depleted mine (a wait target), nullptr = everything depleted (the
 *  all-depleted endgame: intended income death).
 *
 *  Standing laws PRESERVED from the original gold node (TASK-025):
 *   - NOT a combatant: deliberately does NOT implement ITeamAgent — unit
 *     acquisition scans ITeamAgent actors (TASK-004), so implementing it
 *     would make enemy units target the mine. The mine is now fully
 *     team-NEUTRAL: the old ownership-metadata Team/GetTeam is REMOVED;
 *     occupancy (OccupyingTeam) is transient claim state, not identity.
 *   - NOT damageable: SetCanBeDamaged(false) — ApplyDamage routes are refused
 *     engine-side. The raidable investment is the MINER, never the mine.
 *   - Blocks NOTHING: the mesh carries no collision at all (NoCollision
 *     profile, no overlaps, no navmesh relevance). Miners must be able to
 *     stand at/inside the mine's footprint, the mine must not carve the
 *     navmesh it is the walk destination of — and corridor mines are ALLOWED
 *     by ruling precisely because NoCollision keeps the traversability
 *     guarantee safe.
 *   - Visual: /Game/Meshes/SM_GoldNode (soft reference; slot 0 = M_GoldGlow
 *     from import; null-safe: a missing mesh is logged once at runtime and
 *     means an invisible-but-functional mine, never a crash). Resolved in
 *     OnConstruction for the editor viewport and again at BeginPlay for
 *     actors created before the mesh existed. The component stays Movable: a
 *     Static-mobility component refuses SetStaticMesh once the world has
 *     begun play, which would break the deferred-asset runtime resolve.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AGoldNode : public AActor
{
	GENERATED_BODY()

public:

	AGoldNode();

	/**
	 *  (Re)initializes the mine for a fresh match: clamps + sets GoldReserve,
	 *  latches InitialGoldReserve (the gauge denominator), derives bDepleted,
	 *  and clears every tenancy artifact (registry, claim, drain timer).
	 *  Called by ASiegeBattlefieldScatter right after spawning each mine
	 *  (TASK-255, InitMine(300) per the CONVENTIONS defaults). Safe before or
	 *  after BeginPlay — it fully resets the mine either way.
	 */
	void InitMine(int32 InReserve);

	/**
	 *  True if MinerTeam may register here RIGHT NOW: not depleted, reserve
	 *  remaining, and either unclaimed or already claimed by MinerTeam
	 *  (same-team miners stack; enemy-claimed = false -> wait mode).
	 */
	bool CanTeamMine(ETeamId MinerTeam) const;

	/**
	 *  Arrival registration — the atomic claim point. Refuses (false) a null/
	 *  dying miner or CanTeamMine(miner's team) == false; otherwise registers
	 *  the miner and, on the 0 -> 1 registry transition, claims the mine for
	 *  that team and starts the 1 s drain timer. Re-registering an already
	 *  registered miner is an idempotent success (defensive double-arrival).
	 *  The caller (AMinerUnit, TASK-254) runs its arrival block ONLY on true;
	 *  false = stand at the ring and let the poll retry.
	 */
	bool TryRegisterArrivedMiner(AMinerUnit* Miner);

	/**
	 *  Departure/death bookkeeping (AMinerUnit EndPlay + eviction seams,
	 *  TASK-254). Removes the miner (and sweeps stale weak entries); when the
	 *  registry empties, releases the team claim and stops the drain timer —
	 *  the mine becomes claimable by either team again. Null-safe, idempotent.
	 */
	void UnregisterArrivedMiner(AMinerUnit* Miner);

	/**
	 *  THE single mine finder, shared by miner retargeting (TASK-254) and bot
	 *  economy rules (TASK-256). Tier-1: nearest (2D, matching the miner
	 *  arrival metric) non-depleted mine with CanTeamMine(Team). Tier-2 (only
	 *  if tier-1 is empty): nearest enemy-occupied non-depleted mine — a WAIT
	 *  target the caller walks to and queues at. Returns nullptr when every
	 *  mine is depleted (or none exist): the all-depleted endgame — callers
	 *  idle/skip, never crash.
	 */
	static AGoldNode* FindBestMineFor(UWorld* World, ETeamId Team, const FVector& From);

	/** True once the reserve hit 0 (one-way latch for this mine's lifetime; fresh mines come from Play Again's re-scatter). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Mine")
	bool IsDepleted() const { return bDepleted; }

	/** Current gold remaining in this mine. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Mine")
	int32 GetGoldReserve() const { return GoldReserve; }

	/** Reserve latched at InitMine/BeginPlay — the gauge/HUD denominator (0 until latched). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Mine")
	int32 GetInitialGoldReserve() const { return InitialGoldReserve; }

	/** True while any team holds the claim (>= 1 registered miner). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Mine")
	bool IsOccupied() const { return OccupyingTeam.IsSet(); }

	/** Claiming team while occupied; unset when free (C++-only — TOptional is not reflectable; BP reads IsOccupied). */
	TOptional<ETeamId> GetOccupyingTeam() const { return OccupyingTeam; }

	/** Depletion broadcast (see the delegate doc). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Mine")
	FOnMineDepleted OnMineDepleted;

	/** Reserve-change broadcast (see the delegate doc). */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Mine")
	FOnMineReserveChanged OnMineReserveChanged;

	/** Resolves the soft mesh in-editor so the mine is visible while placed/previewed (null-safe, silent). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Runtime mesh resolve + gauge-denominator latch for mines that never receive InitMine (hand-placed/debug instances). */
	virtual void BeginPlay() override;

	/** Actor-destroy safety (ClearScatter's Play-Again mine teardown, TASK-255): the looping drain timer must never outlive the mine. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  Root visual. Mesh comes from NodeMeshAsset — never hard-referenced.
	 *  Carries NO collision by design (see class doc).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Mine")
	TObjectPtr<UStaticMeshComponent> NodeMesh;

	/**
	 *  Gold this mine holds at match start. 300 per the CONVENTIONS "Mirrored
	 *  depleting mines" defaults (tune band 250-450; raise to 450 FIRST if
	 *  playtest says matches stall — the all-depleted pacing lever). The
	 *  scatter's InitMine(MineGoldReserve) overwrites this on spawned mines;
	 *  the default serves hand-placed/debug instances.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Mine", meta = (ClampMin = "0"))
	int32 GoldReserve = 300;

	/**
	 *  Gold removed from the reserve PER REGISTERED MINER PER 1 s drain tick.
	 *
	 *  PAIRED-TUNABLE LAW (CONVENTIONS "Mirrored depleting mines"): this value
	 *  MUST equal ASiegePlayerState::MinerGoldPerTick (SiegePlayerState.h —
	 *  gold granted per arrived miner per 1.0 s income tick). The mine's drain
	 *  and the team's income are the two ends of the same faucet: if they
	 *  drift, a mine grants more/less gold than it loses. Tune BOTH together.
	 *  (SiegePlayerState.h is main-lane frozen this batch — its side of the
	 *  cross-note reconciles at the Phase-6 merge; the law lives in
	 *  CONVENTIONS either way.) Residual tolerance, documented + accepted:
	 *  the two 1 s clocks are independent, so a team may bank up to ~n gold
	 *  more or less than the mine drained per depletion (n = miners on it).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Mine", meta = (ClampMin = "0"))
	int32 DrainPerMinerPerSecond = 1;

	/** Latched gauge/HUD denominator (InitMine, or BeginPlay for never-inited mines). Transient: recomputed every session. */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Siegebound|Mine")
	int32 InitialGoldReserve = 0;

	/** Scalar parameter on M_GoldGlow the reserve gauge drives (authored in TASK-257; missing = silent no-op). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Mine")
	FName GlowIntensityParamName = TEXT("GlowIntensity");

	/** GlowIntensity at full reserve. 1.0 = the material's authored look (the param scales the existing emissive). */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Mine", meta = (ClampMin = "0"))
	float GlowIntensityFull = 1.0f;

	/** GlowIntensity at zero reserve — the depleted ember. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Mine", meta = (ClampMin = "0"))
	float GlowIntensityDepleted = 0.05f;

	/**
	 *  Soft reference to the mine visual: /Game/Meshes/SM_GoldNode (slot 0 =
	 *  M_GoldGlow is baked into the imported asset; the gauge only modulates
	 *  its GlowIntensity param via a MID). Null-safe on load; clearing it in a
	 *  child/instance is a silent designer opt-out (the AttackImpactEffect
	 *  IsNull pattern, TASK-020).
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Mine")
	TSoftObjectPtr<UStaticMesh> NodeMeshAsset;

private:

	/** Loads NodeMeshAsset onto NodeMesh if resolvable; optionally warns (once) when it is not. */
	void ResolveNodeMesh(bool bWarnIfMissing);

	/**
	 *  1 s drain body (looping while occupied): sweeps stale weak entries
	 *  (releasing the claim if that empties the registry), subtracts
	 *  n x DrainPerMinerPerSecond (floored at 0), updates gauge + broadcasts
	 *  on actual change, and calls Deplete() when the reserve hits 0.
	 */
	void HandleDrainTick();

	/**
	 *  The depletion latch (idempotent). Order is load-bearing: latch + stop
	 *  timer FIRST, then snapshot + clear registry/claim BEFORE notifying, so
	 *  a notified miner re-entering (UnregisterArrivedMiner / FindBestMineFor)
	 *  observes this mine already empty and depleted. Then per-miner
	 *  AMinerUnit::NotifyMineDepleted (declared by TASK-254), OnMineDepleted
	 *  broadcast, gauge to the depleted ember.
	 */
	void Deplete();

	/** Drops stale (dead-miner) weak entries; if that empties the registry, releases the claim and stops the drain timer. */
	void CompactArrivedMiners();

	/** Arms the looping 1 s drain timer if not already running (null-safe on world). */
	void StartDrainTimer();

	/** Clears the drain timer (null-safe on world; callable from EndPlay/InitMine at any lifecycle point). */
	void StopDrainTimer();

	/**
	 *  Reserve gauge: lazily creates the slot-0 MID (after the mesh resolve —
	 *  retried next call if mesh/material are not there yet) and sets
	 *  GlowIntensityParamName = Lerp(Depleted, Full, Reserve/Initial). Every
	 *  guard is a silent no-op; never load-bearing.
	 */
	void UpdateGlowGauge();

	/**
	 *  Miners currently registered (arrived) at this mine — weak: miner
	 *  lifetime belongs to the world; death normally unregisters via the
	 *  miner's EndPlay, and CompactArrivedMiners sweeps anything that slipped
	 *  through. Invariant (post-compaction): OccupyingTeam.IsSet() <=>
	 *  ArrivedMiners.Num() > 0.
	 */
	TArray<TWeakObjectPtr<AMinerUnit>> ArrivedMiners;

	/** Claiming team while >= 1 miner is registered; unset when free. ETeamId has no None — TOptional carries the unclaimed state (not reflectable; deliberate plain member). */
	TOptional<ETeamId> OccupyingTeam;

	/** One-way depletion latch (reserve hit 0). Depleted mines refuse registration and never rearm; Play Again spawns fresh mines. */
	bool bDepleted = false;

	/** One-shot guard for the missing-mesh warning. */
	bool bWarnedMissingMesh = false;

	/** Looping 1 s drain timer, alive exactly while the mine is occupied. */
	FTimerHandle DrainTimerHandle;

	/** Lazy MID over slot 0 (M_GoldGlow) — the gauge's write target. Transient: rebuilt per session. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMID;

	/**
	 *  Drain cadence. Fixed 1 s BY LAW, not a tunable: DrainPerMinerPerSecond
	 *  is denominated per-second and paired to the 1.0 s income tick
	 *  (ASiegePlayerState::GoldTickInterval) — a different cadence here would
	 *  silently break the paired-tunable law.
	 */
	static constexpr float DrainTickIntervalSeconds = 1.0f;
};
