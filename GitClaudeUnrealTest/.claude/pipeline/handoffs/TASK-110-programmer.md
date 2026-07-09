# TASK-110 handoff — Overhead health-bar system (C++, files only)

**Status:** ready-for-qa
**Scope:** files only. NO editor/MCP, NO compile, NO git, NO new HP fields. Zero behavior change to combat/stats. ACastle and AGoldNode untouched.

## Files touched

### New files
- `Source/GitClaudeUnrealTest/Siegebound/HealthBarTarget.h` — header-only `IHealthBarTarget` / `UHealthBarTarget` UINTERFACE (the TeamId.h one-concept-header precedent).
- `Source/GitClaudeUnrealTest/Siegebound/HealthBarComponent.h` / `.cpp` — `UHealthBarComponent` (UWidgetComponent subclass), the poll-driven bar.
- `Source/GitClaudeUnrealTest/Siegebound/UnitHealthBarWidget.h` / `.cpp` — `UUnitHealthBarWidget` (UUserWidget subclass), the C++ base TASK-111 reparents WBP_UnitHealthBar to.

### Modified files (interface impl + one `HPBarWidget` component each, in the CONSTRUCTOR)
- `SummonedUnit.h` / `.cpp` — `+ public IHealthBarTarget`; `HPBarWidget` created in ctor, attached to the capsule.
- `Building.h` / `.cpp` — `+ public IHealthBarTarget`; `HPBarWidget` created in ctor, attached to VisualMesh (root).
- `HeroCharacter.h` / `.cpp` — `+ public IHealthBarTarget`; `HPBarWidget` created in ctor, attached to the capsule.

No changes to Build.cs — `UMG`/`Slate` were already dependencies.

## Surface TASK-111 (art) and TASK-112 (build) compile/consume — exact signatures

### `IHealthBarTarget` (pure-virtual const C++ interface, the ITeamAgent shape, NOT BlueprintNativeEvent)
```cpp
UINTERFACE(MinimalAPI, NotBlueprintable) class UHealthBarTarget : public UInterface { GENERATED_BODY() };
class IHealthBarTarget {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Siegebound|Health") virtual float GetHealthCurrent() const = 0;
    UFUNCTION(BlueprintCallable, Category="Siegebound|Health") virtual float GetHealthMax() const = 0;
    UFUNCTION(BlueprintCallable, Category="Siegebound|Health") virtual bool  IsHealthBarActorAlive() const = 0;
};
```
Implemented (inline, forwarding to the EXISTING getters — no new HP state):
- `ASummonedUnit` → `GetCurrentHP()` / `GetMaxHP()` / `!IsUnitDead()`
- `ABuilding` → `GetCurrentHP()` / `GetMaxHP()` / `!IsBuildingDestroyed()`
- `AHeroCharacter` → `GetCurrentHP()` / `GetMaxHP()` (already the EFFECTIVE Plate-Armor max) / `!IsDead()`

### `UUnitHealthBarWidget : public UUserWidget` — the two BIEs WBP_UnitHealthBar must implement (FLOAT PARAMS ONLY)
```cpp
UFUNCTION(BlueprintImplementableEvent, Category="Siegebound|UI") void OnHPChanged(float CurrentHP, float MaxHP);
UFUNCTION(BlueprintImplementableEvent, Category="Siegebound|UI") void SetTeamColor(float R, float G, float B);
```
TASK-111 reparents the WBP_CastleHealthBar donor duplicate to `UUnitHealthBarWidget` and implements: `OnHPChanged` → ProgressBar `SetPercent(CurrentHP/MaxHP)` guard `MaxHP>0`; `SetTeamColor` → tint the FILL brush from the RGB.

### `UHealthBarComponent : public UWidgetComponent` — EditDefaultsOnly tunables (exact CONVENTIONS names/defaults)
| Property | Type | Default |
|---|---|---|
| `HealthBarWidgetClass` | `TSoftClassPtr<UUserWidget>` | `/Game/UI/WBP_UnitHealthBar.WBP_UnitHealthBar_C` |
| `PollInterval` | `float` | `0.15` (ClampMin 0.02) |
| `bShowHealthBar` | `bool` | `true` |
| `BarHeightZ` | `float` | `120` |
| `BlueBarColor` | `FLinearColor` | `(0.05, 0.30, 1.00)` |
| `RedBarColor` | `FLinearColor` | `(1.00, 0.10, 0.05)` |
Widget space = **Screen**, DrawSize = **90×12**, collision = NoCollision.

## Poll mechanism
- Constructor: sets Screen space, DrawSize 90×12, NoCollision, starts hidden, seeds the default soft widget class.
- `BeginPlay`: applies relative Z = `BarHeightZ` (after BP overrides land); soft-resolves `HealthBarWidgetClass` (null → silent no-bar, logged ONCE run-wide via a function-local static, timer never armed, never a crash); `SetWidgetClass` → caches the `UUnitHealthBarWidget`; pushes `SetTeamColor` ONCE from the owner's `ITeamAgent::GetTeamId` (Red→RedBarColor, else BlueBarColor); arms a repeating `PollInterval` timer (clamped `>0`, first fire immediate).
- `PollHealth` (each `PollInterval`): reads the owner as `IHealthBarTarget`. `bShouldShow = bShowHealthBar && IsHealthBarActorAlive() && (Current < Max − 0.01)`. Not-show → hide (own `bBarShown` latch, no redundant render dirties); show → `SetVisibility(true)` + `OnHPChanged(Current, Max)`. Statless actor (Max≤0) stays hidden by the same test.
- `EndPlay`: clears the poll timer.

## Flagged decisions for QA
1. **Interface carries `UFUNCTION(BlueprintCallable)` on each pure-virtual const method** — deliberately mirrors `ITeamAgent::GetTeamId` character-for-character (the spec's "the ITeamAgent C++-interface shape"). NotBlueprintable + BlueprintCallable pure-virtual is the proven ITeamAgent pattern; implementers override in plain C++ with no UFUNCTION (same as ACastle/ASummonedUnit do for GetTeamId). Not a BlueprintNativeEvent.
2. **Static (constructor) vs BeginPlay init split.** Widget space / DrawSize / NoCollision / initial-hidden / default soft class → constructor (static, reusable per instance, the ACastle pattern of configuring the WidgetComponent up front). `BarHeightZ` relative-Z, class resolve, team-tint push, timer arm → BeginPlay (they depend on BP-tunable values / a game world). Spec said "on BeginPlay/register"; the static ones are equivalent done in the ctor.
3. **`WBP_UnitHealthBar.WBP_UnitHealthBar_C` soft path** (the `_C` runtime generated-class suffix) mirrors `ACastle::HPBarWidgetClass` = `WBP_CastleHealthBar.WBP_CastleHealthBar_C`. TASK-111 must author the asset at `/Game/UI/WBP_UnitHealthBar`.
4. **Hide-on-death latency is one poll (≤0.15 s) for the HERO only.** Units/buildings are DESTROYED on death (component + bar vanish instantly). The hero HIDES (SetActorHiddenInGame) and is not destroyed, and screen-space widget components don't follow actor-hidden state (the ACastle note), so the poll's `!IsDead()` check hides it within one tick. Imperceptible; satisfies "hides on death."
5. **Own `bBarShown` latch instead of `IsVisible()`** — avoids any dependency on engine `USceneComponent::IsVisible()`/hidden-in-game semantics for screen-space widgets; deterministic show/hide, minimal render-state dirties.
6. **"Log once" = run-wide** via a function-local `static bool` (one line total for a missing WBP), not per-instance (would be 60+ lines). Level = `Log`.
7. **Epsilon = 0.01** for the full-HP hide test (`Current < Max − 0.01`) — guards a 1-px sliver from float drift at exactly full HP.
8. **`bShowHealthBar` default true**; miners inherit the bar and can be suppressed per-BP with zero code (manager ruling 1).

## Shadow-scan notes (C4457/58/59 — MANDATORY)
- `UHealthBarComponent` members are `HealthBarWidgetClass`, `PollInterval`, `bShowHealthBar`, `BarHeightZ`, `BlueBarColor`, `RedBarColor`, `BarWidget`, `bBarShown`, `PollTimerHandle` — NONE collide with `UWidgetComponent` members (notably it has `WidgetClass`, `Widget`, `Space`, `DrawSize`, `TintColorAndOpacity`, `BackgroundColor`; mine are all distinctly named). No local/member is named `Owner`, `Instigator`, or `Slot`.
- The `HPBarWidget` member is added to three DIFFERENT classes; none of their parent chains (ACharacter/AActor/AGitClaudeUnrealTestCharacter) declare `HPBarWidget` (verified). ACastle's `HPBarWidget` is on an unrelated class — no shadow.

## What QA should scrutinize
- The interface UFUNCTION-on-pure-virtual pattern (flag 1) compiles the same as ITeamAgent.
- No new HP field anywhere; interface methods only forward to existing getters.
- Null-safety: missing widget class, non-target owner, mis-authored widget — all no-crash paths.
- ACastle / AGoldNode NOT modified; ACastle's own `HPBarWidget` untouched.

## Build-fix loop 1 (2026-07-09)

**Root cause (build-master's diagnosis, confirmed):** `HeroCharacter.cpp:52` did
`HPBarWidget->SetupAttachment(GetCapsuleComponent())` while `UCapsuleComponent` was only
forward-declared in that translation unit, so the compiler could not see it derives from
`USceneComponent` and rejected the implicit upcast (C2664). `SummonedUnit.cpp` does the identical
call and compiled clean **because its TU includes the CapsuleComponent header** (SummonedUnit.cpp:6).

**The fix (single line, include-only — no logic changed):** added to `HeroCharacter.cpp`, placed in
the existing alphabetical include block after `Camera/CameraShakeBase.h` and before
`EnhancedInputComponent.h` — exactly mirroring SummonedUnit.cpp's include style/ordering:
```cpp
#include "Components/CapsuleComponent.h"
```
The `SetupAttachment(GetCapsuleComponent())` call is UNCHANGED (it shifts from line 52 to line 53
purely from the inserted include). No other file touched. QA-passed logic is intact.

**Sibling upcast sweep (as required):** grepped all TASK-110 changed `.cpp` files for
`SetupAttachment(` / `GetCapsuleComponent()` / `SetRootComponent(`:
- `HeroCharacter.cpp` — the one bug, now fixed.
- `SummonedUnit.cpp:73,84` — same `SetupAttachment(GetCapsuleComponent())`; already includes
  `Components/CapsuleComponent.h` (line 6). Compiled clean.
- `Building.cpp:29,55` — attaches `HPBarWidget`/root to `VisualMesh` (a `UStaticMeshComponent`),
  NOT the capsule; compiled clean.
- `HealthBarComponent.cpp` / `UnitHealthBarWidget.cpp` — no capsule/component upcast.

**Conclusion:** `HeroCharacter.cpp` was the ONLY TASK-110 file with the missing-header upcast, matching
build-master's finding. (The other project files that upcast `GetCapsuleComponent()` — Barracks.cpp,
SiegePlayerController.cpp — already include the header and are outside the TASK-110 file set.) Ready to
re-run phase A.
