# TASK-534 — `ASiegeUnitAIController::OnMoveCompleted` — handoff to QA (TASK-537)

**Status:** `ready-for-qa` · **Gate:** TASK-537 · **Compiled:** NO (TASK-538 owns the only compile) · **Git:** untouched · **Editor/MCP/PIE:** untouched

## M8 DECLARATION (verbatim, NAV-§11)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

Reason: AI controllers are server-only in M8 P1. The one member added (`LastBlockedLogTimeSeconds`) is a plain unreflected `double` on a server-side controller — not a `UPROPERTY`, so it cannot be replicated by accident.

## Files touched — TWO, both inside my exclusive lane

- `Source/GitClaudeUnrealTest/Siegebound/SiegeNavAreas.h` (+43)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeNavAreas.cpp` (+101)

⛔ No new class, no new file. ⛔ No `Build.cs` change (`AIModule` + `NavigationSystem` are already `PublicDependencyModuleNames`, verified). ⛔ No `.umap` write. ⛔ `SummonedUnit` / `MinerUnit` / `BattlefieldScatter` / `Tests/` untouched (TASK-532/533/535/536). ⛔ `ApplyTeamNavigationFilter`, the `UNavArea_*` / `UNavFilter_Team*` types, the `ECC_SiegeTeam*` aliases and `AIControllerClass` (`SummonedUnit.cpp:106`) all byte-unchanged.

## 1. The override, exactly as written

```cpp
virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
```

Character-for-character the `NAV-§8` pin, **public**, on the existing `ASiegeUnitAIController`.

Body, in order: `Super::OnMoveCompleted(RequestID, Result)` unconditionally first → early-return unless `Result.Code == EPathFollowingResult::Blocked` → early-return unless `Cast<ASummonedUnit>(GetPawn())` succeeds → throttled `LogSiegeStuck` line → `Unit->NotifyMoveBlocked()` as the **last statement**.

## 2. ⛔ NO MOVEMENT IS ISSUED FROM THIS FUNCTION — explicit statement (NAV-§3)

**`ASiegeUnitAIController::OnMoveCompleted` issues no move of any kind.** There is no `MoveTo*`, no `MoveToActor`, no `MoveToLocation`, no `StopMovement`, no `RequestMove`, no focus change, and no other steering call anywhere in the function. It records and returns. The engine's verdict is **evidence, not an action**; only `ASummonedUnit::TickStuckWatchdog` may steer.

Mechanical check QA can repeat:
```
grep -n "MoveTo\|StopMovement\|SetFocus\|RequestMove\|K2_Move\|ClearFocus" Source/GitClaudeUnrealTest/Siegebound/SiegeNavAreas.cpp
```
→ exactly one hit, `:87`, inside a **pre-existing comment** on `ApplyTeamNavigationFilter`. Zero hits in `OnMoveCompleted`.

`NotifyMoveBlocked()` is TASK-532's; per `NAV-§3` it may advance the stall clock but may never fire a rung and never issues a move. Nothing in my lane bypasses the cooldown or the monotonic-level brakes.

## 3. ⚠️ THE OVERLOAD — which one I took, and how I confirmed it

I took **the `FPathFollowingResult` form**, `AIController.h:230`. Confirmed first-hand against the installed engine at `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\AIModule\Classes\AIController.h`:

```
230:	AIMODULE_API virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
233:	AIMODULE_API virtual void OnMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);
```
with `:232` carrying `UE_DEPRECATED_FORGAME(4.13, "This function is now deprecated, please use version with EPathFollowingResultDetails parameter.")`. The deprecated attribute sits on the **`EPathFollowingResult::Type`** form; I did not touch it.

**And I confirmed the deprecation is LIVE for this module, not inert** — `UnrealBuildTool/Configuration/UEBuildModuleCPP.cs:1693`:
```
Definitions.Add($"UE_DEPRECATED_FORGAME={(Rules.bTreatAsEngineModule ? "UE_EMPTY_FUNCTION" : "UE_DEPRECATED")}");
```
`GitClaudeUnrealTest` is a project module (`bTreatAsEngineModule == false`), so `UE_DEPRECATED_FORGAME` expands to `UE_DEPRECATED` → `[[deprecated]]` → C4996 had I overridden `:233`. The trap is real and it is avoided.

**`using Super::OnMoveCompleted;` is deliberately NOT present.** Rationale: the remedy is conditional in the spec ("*if* MSVC reports a hidden-overload warning"), I cannot compile, and adding it speculatively **names the deprecated overload in this module** — the one thing the law forbids. The hidden-overload diagnostics (C4263/C4264) are off-by-default in MSVC and UBT does not enable them. ➡️ **If TASK-538's compile emits C4263/C4264 or `-Woverloaded-virtual` on this class, the one-line fix is to add `using Super::OnMoveCompleted;` in the `public:` block — do NOT implement the deprecated overload.**

**Why the `Super` call is mandatory, verified:** `AIController.cpp:985-989` reads
```cpp
void AAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	ReceiveMoveCompleted.Broadcast(RequestID, Result.Code);
	OnMoveCompleted(RequestID, Result.Code);
}
```
i.e. the base drives the Blueprint `MoveCompleted` delegate and forwards to the (empty) deprecated overload. Skipping it would break BP/brain notifications with no visible link to this change. My `Super::` call is first and unconditional.

## 4. The log line — format, category, verbosity, throttle

**Category: `LogSiegeStuck`**, per `NAV-§7` and this task's `names:` block, with the pinned **`blocked:`** token.

> ⚠️ **DISCREPANCY REPORTED, NOT SILENTLY RESOLVED.** My dispatch prompt said to use `LogSiegeNavDiag`. `TASKBOARD.md:7125` (clause 3), the `names:` block at `:7133` and `CONVENTIONS NAV-§7` all say the **stuck** lines go on **`LogSiegeStuck`** with the `blocked:` token; `LogSiegeNavDiag` is the **telemetry** category owned by TASK-529. I followed the board + CONVENTIONS, which are the contract. Verified landed: `SiegeStuckStatics.h:73` `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeStuck, Log, All)` / `SiegeStuckStatics.cpp:11` `DEFINE_LOG_CATEGORY(LogSiegeStuck)`. The prompt's warning about `LogSiegeNav` vs `LogSiegeNet` is real but does not apply to my lane.

Emitted at **`Log` verbosity** (category default is `Log`, so it prints at default verbosity — the gate will see it). Deliberately not `Warning`: a blocked verdict is now an *expected, handled* event that the ladder resolves, and warning-level noise for a handled condition is bad hygiene.

Format (pure ASCII by design — this file is UTF-8 with no BOM and this string is the gate's grep target):
```
[Stuck] blocked: unit='%s' team=%s result=%s reqId=%u loc=%s - engine path-following block detection fired (agent moved under BlockDetectionDistance across its sample window). Evidence only: no move is issued from OnMoveCompleted.
```
Sample runtime shape:
```
LogSiegeStuck: [Stuck] blocked: unit='BP_Unit_Knight_C_2' team=Blue result=Blocked[Blocked] reqId=17 loc=X=1240.00 Y=-980.00 Z=92.00 - engine path-following block detection fired (...)
```
⭐ It satisfies **both** greps: the pinned `blocked:` token *and* the literal `Blocked` (from `FPathFollowingResult::ToString()`, `PathFollowingComponent.cpp:76-80`, which prints `Blocked[Blocked]` for this code).

**Throttle:** `static constexpr double SiegeBlockedLogThrottleSeconds = 10.0;` — a named constant, no bare literal at the call site. Per-unit, because every unit auto-possesses its own controller instance (`SummonedUnit.cpp:105-106`), so the per-controller stamp `LastBlockedLogTimeSeconds` *is* a per-unit rate limit. Negative sentinel = "never logged", so a unit's **first** block always prints.

**Why 10 s and not 5 s:** the engine cannot re-declare `Blocked` for one unit faster than `BlockDetectionInterval 0.5 × BlockDetectionSampleCount 10 = 5.0 s` plus the 0.25 s `UpdateState` poll that re-issues and refills the sample buffer — a ~5.25 s floor on the natural event period. **A window at or below that floor would throttle nothing.** At 10 s a permanently wedged unit prints ~half as often as it wedges; worst case for a fully wedged 120-unit army is ~12 lines/s instead of ~23.

⛔ **The throttle governs the LOG ONLY.** `NotifyMoveBlocked()` is called on **every** `Blocked` verdict, outside the throttled block. Throttling the evidence feed would silently starve the watchdog — QA should confirm this is still true after any edit.

## 5. Null-safety, includes, standing C++ sweep

- `GetPawn()` may be null at unpossess/teardown and the cast legitimately fails for non-`ASummonedUnit` pawns (the hero) → `if (!Unit) return;`, quietly, no log. **`AMinerUnit` derives from `ASummonedUnit`, so the single cast covers the whole fleet.**
- `GetWorld()` guarded via `if (const UWorld* World = GetWorld())`; a null world skips the *line* only.
- **Complete-type include law:** the header needs only the forward declaration `AIController.h:40` already provides (`struct FPathFollowingResult;`) — a const-ref *parameter* in a *declaration* needs no definition, and `AIController.h` can never disappear because it declares the base class. The `.cpp` adds the three includes it genuinely uses: `Navigation/PathFollowingComponent.h` (for `Result.Code` and `EPathFollowingResult::Blocked` — `PathFollowingComponent.h:53` and `:121`), `Siegebound/SiegeStuckStatics.h` (`LogSiegeStuck`), `Siegebound/SummonedUnit.h` (`Cast<>` + `NotifyMoveBlocked`). No TU that merely includes `SiegeNavAreas.h` pays for them.
- **No header cycle:** `SummonedUnit.h` does not include `SiegeNavAreas.h` (checked).
- **No shadowing** of any inherited reflected member: `LastBlockedLogTimeSeconds` is unique in the hierarchy and is not a `UPROPERTY`.
- **No most-vexing-parse:** the only declarations are `ASummonedUnit* Unit = Cast<...>(...)`, `const double NowSeconds = ...` and the `if`-init `const UWorld* World = GetWorld()`, all copy-initialized.
- Line endings CRLF-consistent (168/168 and 162/162); no mixed-EOL diff noise.

## 6. Link contract — verified against LANDED code, not just the pin

TASK-531 and TASK-532 landed while I was working, so I checked the real symbols rather than trusting the pin:

| symbol | pin | landed | ✅ |
|---|---|---|---|
| `LogSiegeStuck` | `SiegeStuckStatics.h` | `SiegeStuckStatics.h:73` (`Log, All`), defined `.cpp:11` | ✅ |
| `ASummonedUnit::NotifyMoveBlocked()` | public, no args | `SummonedUnit.h:332`, nearest specifier `public:` at `:122` | ✅ |
| `FPathFollowingResult::Code` | — | `PathFollowingComponent.h:123` `TEnumAsByte<EPathFollowingResult::Type> Code` | ✅ |
| `EPathFollowingResult::Blocked` | — | `PathFollowingComponent.h:60` | ✅ |
| `FAIRequestID::GetID()` | — | `AITypes.h:427`, returns `uint32` → `%u` | ✅ |
| `ASummonedUnit::GetTeamId()` | — | `SummonedUnit.h:126`, public const | ✅ |

## 7. ⚠️ WHAT QA SHOULD SCRUTINISE

1. ⭐ **That `NotifyMoveBlocked()` is outside the throttle.** The single most likely way a later edit breaks this feature is someone "tidying" the notify into the logging block. Then the ladder gets one signal per 10 s instead of every verdict.
2. **The `Super::` call is first and unconditional** — including on the non-`Blocked` early-return path.
3. **Only `Blocked` is acted on.** `Aborted` fires every time a unit re-targets (a new request aborts the previous); acting on it would manufacture stalls.
4. ⚠️ **Bounded known limitation, declared not buried:** the throttle stamp lives on the controller. If a controller were ever re-possessed onto a different pawn (unit pooling), the new unit's first blocked line could be suppressed for up to 10 s. This project does not pool controllers — `AutoPossessAI::PlacedInWorldOrSpawned` gives each unit its own, destroyed with the pawn — so it cannot occur today. Recorded so it is not "found" later.
5. **`FPathFollowingResult::ToString()` allocates an `FString`** — it is built inside the throttled branch only, never on the hot path.
6. **The 10 s window is my judgement call**, not a pinned number. If QA or Jonathan wants more resolution at the playtest, it is a one-line change at `SiegeNavAreas.cpp:32`.

## 8. ⚠️ ONE SPEC CLAIM I COULD NOT CONFIRM — reported, and it changes nothing I did

The spec (and `NAV-§8`) states *"warnings are errors in this build"* as the reason to avoid the deprecated overload. **I could not verify that premise.** Neither `Source/GitClaudeUnrealTest.Target.cs` nor `Source/GitClaudeUnrealTestEditor.Target.cs` sets `bWarningsAsErrors` or `CppCompileWarningSettings.DeprecationWarningLevel`, and UBT's default is `[BasicWarningLevelDefault(WarningLevel.Warning)]` (`CppCompileWarnings.cs:1279`) → `/wd4996`-class **warning**, not `/we4996`.

⇒ The **instruction is right and I followed it** — override the non-deprecated form — but the stated **consequence** is overstated: overriding `:233` would emit C4996 as a *warning*, not a hard error, in this project's current configuration. Flagging it so no future task cites "warnings are errors here" as an established fact without checking. Nothing in my code depends on which it is.
