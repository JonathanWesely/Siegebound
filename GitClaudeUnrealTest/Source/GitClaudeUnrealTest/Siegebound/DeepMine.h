// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Siegebound/Building.h"
#include "DeepMine.generated.h"

class ASiegePlayerState;

/**
 *  Siegebound raidable economy building (GDD §8 Deep Mine, TASK-057): an
 *  ABuilding that raises the OWNING team's gold rate the instant it is placed —
 *  no walk (unlike the Miner, which must reach its gold node first). BP child:
 *  BP_Building_DeepMine (CardID DeepMine — row: 200 HP; the +2 gold/s is the
 *  DeepMineIncome UPROPERTY below, NOT a card-stat column, per CONVENTIONS
 *  "mechanic rules are UPROPERTY, not CSV").
 *
 *  - On BeginPlay it resolves the owning team's ASiegePlayerState via
 *    ASiegeGameState::GetPlayerStateForTeam(Team) (the TASK-043 multi-team
 *    accessor — a Blue mine raises only Blue's rate, a Red bot mine only Red's)
 *    and registers +DeepMineIncome through the player state's flat non-miner
 *    income path (AddIncome — SEPARATE from the miner count/cap, so the §3.3
 *    miner economy is untouched and the MaxActiveMiners cap does NOT apply).
 *  - On death (raided to 0 HP, §8) it removes exactly the income it registered
 *    (RemoveIncome), dropping the owner's rate by 2.
 *  - Match-end freeze needs no per-mine hook: the game mode's FreezeWorldAtMatchEnd
 *    calls PauseIncome() on every player state, which stops ALL accrual (base +
 *    miner + deep-mine alike). The registered income stays configured (like an
 *    arrived miner's) and is cleared on Play Again by the building-destroy sweep
 *    + ResetEconomy.
 *  - Destructible + blocks pathing via the ABuilding base (200 HP from the row,
 *    BlockAll collision, navigation-relevant). No mesh/material in C++ — the BP
 *    child assigns SM_DeepMine + the team material (TASK-063).
 *
 *  NOTE: the DeepMine card row is CardType Economy (not Building), so the base
 *  ABuilding::LoadStats logs a one-time "expected Building" CardType warning per
 *  mine — benign: HP still binds from the row (LoadStats binds HP regardless of
 *  CardType). The play/spawn routing that sends an Economy card down the
 *  building path is TASK-059's concern, not this class's.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ADeepMine : public ABuilding
{
	GENERATED_BODY()

public:

	ADeepMine();

	/** True once the +DeepMineIncome was registered on the owning economy (verification hook). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
	bool IsIncomeRegistered() const { return bIncomeRegistered; }

protected:

	/** Binds HP via the base, then registers +DeepMineIncome on the owning team's economy (retrying if the player state is not resolvable yet). */
	virtual void BeginPlay() override;

	/** Removes the registered income (on destruction) and clears the retry poll. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Passive gold-per-tick this mine adds to its owner's rate the instant it is placed (§8: +2 gold/s). Mechanic rule → UPROPERTY, not a CSV column (CONVENTIONS). // GDD §8 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Building", meta = (ClampMin = "0"))
	int32 DeepMineIncome = 2;

	/** Retry cadence for resolving the owning player state when it is not yet in PlayerArray at BeginPlay (an early-spawn ordering edge; never per-tick). */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Building", meta = (ClampMin = "0.05"))
	float IncomeRegisterRetryInterval = 0.25f;

private:

	/** Resolves the owning economy and registers +DeepMineIncome exactly once; arms/keeps a retry poll while unresolvable, self-clearing on success. */
	void TryRegisterIncome();

	/** The owning team's ASiegePlayerState via ASiegeGameState::GetPlayerStateForTeam(Team) — nullptr (logged once) while unresolvable. */
	ASiegePlayerState* ResolveOwningPlayerState();

	/** The player state the income was registered on — cached at registration so death removes it from the SAME economy even if PlayerArray shifted. */
	TWeakObjectPtr<ASiegePlayerState> CachedOwnerState;

	/** True once AddIncome(DeepMineIncome) ran — the register/unregister latch (RemoveIncome fires exactly once, only if this is true). */
	bool bIncomeRegistered = false;

	/** One-shot guard for the no-GameState-yet warning (the retry poll otherwise re-logs it every tick). */
	bool bWarnedNoGameState = false;

	/** One-shot guard for the no-player-state-for-team warning; also stops the pointless re-query of a genuinely mis-teamed mine. */
	bool bWarnedNoTeamPlayerState = false;

	/** Drives TryRegisterIncome until the owning economy resolves, then self-clears (only armed when BeginPlay could not resolve immediately). */
	FTimerHandle IncomeRetryTimerHandle;
};
