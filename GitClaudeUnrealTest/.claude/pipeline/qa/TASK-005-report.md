# QA Report — TASK-005 — Gold economy on PlayerState (C++)
Verdict: PASS

Reviewed: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.h`, `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp`
Against: TASKBOARD TASK-005 spec + names block, CONVENTIONS.md, GDD §3.2, handoff `.claude/pipeline/handoffs/TASK-005.md`
Blockers: 0 · Majors: 2 (cross-task wiring warnings, not code defects) · Minors: 3

## Spec compliance (verified)

- **Start 50:** `BeginPlay` → `ResetGold()` → `SetGold(StartingGold=50)`. Correct.
- **+2 gold / 1.0 s:** repeating timer (`StartIncomeTimer`, first fire at t=1.0 s) adds `GoldPerTick=2` → ~10 gold over 5 s. Correct.
- **Hard cap 999 / floor 0:** all writes funnel through the single private `SetGold()` with `FMath::Clamp(NewGold, 0, MaxGold)` — no path can bypass the cap. Correct.
- **SpendGold refusal:** gated on `CanAfford` (rejects negative Cost and Cost > Gold); on refusal returns false with no state change and no broadcast; `Gold - Cost >= 0` is guaranteed by the gate, so gold can never go negative. Correct.
- **ResetGold:** back to `StartingGold` AND restarts the income timer via `SetTimer` on the same handle (replaces, never stacks). Correct.
- **Delegate:** `FOnGoldChanged` dynamic multicast, `BlueprintAssignable`, broadcast on every actual value change. See ruling below.
- **Timer lifecycle:** `EndPlay` clears `GoldTickTimerHandle` before `Super::EndPlay`. Correct signature (`const EEndPlayReason::Type`), correct `override`.
- **UE 5.8 API:** no deprecated/removed API. Delegate macro, `UPROPERTY`/`UFUNCTION` specifiers, timer API, and `GENERATED_BODY` placement are all current. `SiegePlayerState.generated.h` is last include; cpp includes own header first; `GITCLAUDEUNREALTEST_API` matches the module; `"Siegebound/SiegePlayerState.h"` resolves via the module-root public include path in GitClaudeUnrealTest.Build.cs. No new module dependencies needed.
- **Conventions:** class name, file location, delegate name, and category naming all match the names block character-for-character. No UObject-pointer members, so the TObjectPtr rule is vacuously satisfied.
- **TODO(M2)** for overtime doubling present in `HandleGoldTick` as required.

## Ruling on the flagged design decision (broadcast only on actual change)

**ACCEPTED.** GDD §3.2's acceptance criterion is "the HUD counter matches the underlying value at all times" — an invariant about value equality, not event counting. Since every real value change broadcasts and a clamped no-op write does not change the value, a listener seeded from `GetGold()` can never diverge from the stored value. Suppressing the once-per-second no-op broadcast while pinned at 999 is also a small perf win. The spec line "fires on EVERY change" is satisfied under the only reading that matters (change = value actually changed). **This acceptance is conditional on the HUD seeding from `GetGold()` on construct — see Finding 2.**

## Findings

1. **[major — cross-task warning, TASK-006]** `SiegePlayerState.cpp:47-55` (`ResetGold`) + handoff claim. The handoff states ResetGold's timer restart means "PlayAgain's 'clear all timers' step cannot permanently kill income." That is only true if `PlayAgain()` clears timers BEFORE calling `ResetGold()`. The TASK-006 board spec lists the steps as "...ResetGold(), destroy+respawn the hero at start, **clear all timers**, remove end screen" — implemented literally in that order, the clear-all step would kill the freshly restarted income timer: gold income stops forever, no further broadcasts, HUD frozen. **Binding note for TASK-006:** call `ResetGold()` AFTER any global timer clear, or clear timers selectively (never the PlayerState's). No change required in TASK-005 code.
2. **[major — cross-task warning, TASK-011]** The broadcast-on-actual-change design has one concrete failure mode if the HUD does not seed itself: a widget constructed (or reconstructed after Play Again / match end) while gold is pinned at 999 receives no broadcasts and shows a stale/placeholder value indefinitely. The handoff already directs TASK-011 to initialize from `GetGold()` on construct, but the TASK-011 board spec currently only says "bound to FOnGoldChanged." **Manager should append the `GetGold()` seed requirement to TASK-011's spec** so the contract lives on the board, not only in a handoff file.
3. **[minor]** `SiegePlayerState.h:96` — `int32 Gold = 50;` duplicates StartingGold's default as a magic number. If a BP subclass edits `StartingGold`, `GetGold()` returns 50 between construction and BeginPlay. Harmless in practice (BeginPlay resets before any HUD exists); consider initializing to 0 or assigning from `StartingGold` in a constructor. Not required.
4. **[minor]** `SiegePlayerState.h` — `FTimerHandle` is only available transitively (PlayerState.h → Actor.h). IWYU-clean would add `#include "Engine/TimerHandle.h"`. Compiles fine in 5.8 as-is. Not required.
5. **[minor — info]** `SiegePlayerState.cpp:17-25` — the `EndPlay` timer clear is technically redundant (TimerManager drops timers bound to dying UObjects), but explicit cleanup is good hygiene. Keep it.

## Notes for build-master

- Compile-safe as written: no Build.cs changes, no new module deps, no editor assets referenced.
- After compiling, sanity-check the acceptance behaviors in PIE once TASK-006/007/011 land: starts at 50, ~+10 over 5 s, pins at 999, SpendGold(3) at 2 gold returns false with no broadcast.
- Carry Finding 1 (PlayAgain ordering) and Finding 2 (HUD GetGold seed) forward — they are wiring constraints on TASK-006 and TASK-011, not defects here.
