# TASK-005 Handoff — Gold economy on PlayerState (gameplay-programmer)

## Files created

- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp`

No other files touched. No Build.cs changes needed (uses only Core/CoreUObject/Engine, and the
`GitClaudeUnrealTest` module root is already a public include path, so downstream code includes it
as `#include "Siegebound/SiegePlayerState.h"`). Did NOT create/edit `TeamId.h`/`CardRow.h`
(TASK-001 owns those); this class needs neither.

## Class

`ASiegePlayerState : APlayerState` — `UCLASS()`, exported with `GITCLAUDEUNREALTEST_API`.

## Exact signatures downstream tasks bind to

Delegate type (declared in SiegePlayerState.h):

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);
```

Members:

```cpp
UPROPERTY(BlueprintAssignable, Category = "Siegebound|Gold")
FOnGoldChanged OnGoldChanged;                                  // HUD (TASK-011) binds here

UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
int32 GetGold() const;                                         // HUD seeds initial display

UFUNCTION(BlueprintPure, Category = "Siegebound|Gold")
bool CanAfford(int32 Cost) const;                              // PlayerController (TASK-007) gate

UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
bool SpendGold(int32 Cost);                                    // PlayerController (TASK-007) on confirm

UFUNCTION(BlueprintCallable, Category = "Siegebound|Gold")
void ResetGold();                                              // GameMode (TASK-006) PlayAgain
```

## Behavior contract

- Gold starts at 50 (`StartingGold`, EditDefaultsOnly). Passive income +2 (`GoldPerTick`) every
  1.0 s (`GoldTickInterval`) via a repeating timer started in `BeginPlay` (through `ResetGold`).
- Hard cap 999 (`MaxGold`); floor 0. Every write goes through the single private `SetGold()`
  which clamps then broadcasts — no path can skip the cap or the delegate.
- `OnGoldChanged` fires on every ACTUAL value change. A write that clamps back to the current
  value (e.g. income tick while pinned at 999, or `SpendGold(0)`) does not broadcast — the HUD
  value therefore always equals the stored value. **TASK-011: initialize the gold text from
  `GetGold()` on widget construct**, since PlayerState BeginPlay may run before the HUD binds.
- `SpendGold` refuses (returns false, no change, no broadcast) if Cost is negative or > Gold;
  gold never goes negative. `CanAfford` also treats negative Cost as false so the two agree.
- `ResetGold()` sets gold back to `StartingGold` AND restarts the income timer, so TASK-006's
  PlayAgain "clear all timers" step cannot permanently kill income. `SetTimer` on the same
  handle replaces the old timer — repeated resets never stack ticks. Timer cleared in `EndPlay`.
- Overtime (7:00 income doubling) is deferred to M2 — marked `TODO(M2)` in `HandleGoldTick`.

## For QA to scrutinize

- Delegate broadcasts only on real change (design decision, see above) — verify this reading of
  "every mutation fires OnGoldChanged" (a clamped no-op write is not a mutation).
- Tunables (50/2/1.0/999) are EditDefaultsOnly UPROPERTYs matching GDD §3.2 defaults, per the
  M1 manager decision that non-card stats live as C++ UPROPERTY defaults.
- Not compiled (build-master's job). No replication — M1 is local-only; M8 revisits.
