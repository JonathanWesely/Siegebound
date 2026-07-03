# TASK-018 Handoff — Castle HP delegate + health-bar widget component (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no Build.cs, no TASKBOARD edit per task constraints)

## Files touched

1. `Source/GitClaudeUnrealTest/Siegebound/Castle.h` — modified
2. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` — modified
3. `Source/GitClaudeUnrealTest/Siegebound/CastleHealthBarWidget.h` — NEW
4. `Source/GitClaudeUnrealTest/Siegebound/CastleHealthBarWidget.cpp` — NEW

NOT touched (per task hard rules): `GitClaudeUnrealTest.Build.cs` (verified read-only: "UMG" already present at line 20 — UWidgetComponent lives in the UMG module, so no new dependency), `HeroCharacter.*`, `SummonedUnit.*`, `TASKBOARD.md`.

## What changed

### ACastle — delegate (spec point 1)

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleHPChanged, float, CurrentHP, float, MaxHP);

UPROPERTY(BlueprintAssignable, Category = "Siegebound|Castle")
FOnCastleHPChanged OnCastleHPChanged;
```

Broadcast points — exactly three, matching spec:
- **TakeDamage**: after `CurrentHP = FMath::Max(CurrentHP - ActualDamage, 0.0f)`, BEFORE the destroyed check (listeners see the 0-HP value before the bar hides). The friendly-fire early-out, the `bDestroyed` early-out, and the `DamageAmount <= 0` early-out all `return` ABOVE the broadcast, so ignored damage produces zero broadcasts. Since the broadcast is only reachable with `CurrentHP > 0` and `ActualDamage > 0`, every broadcast reports a genuinely changed value — no equality check needed.
- **ResetCastle**: unconditional broadcast(MaxHP, MaxHP) after restoring state (reset-path rule, CONVENTIONS delegate section).
- **BeginPlay**: one seed broadcast after `CurrentHP = MaxHP`.

`GetCurrentHP()` / `GetMaxHP()` BlueprintPure already existed from TASK-002 — unchanged, spec satisfied as-is.

### ACastle — HPBarWidget component (spec point 2)

Constructor: `UWidgetComponent` subobject named exactly `HPBarWidget`, attached to `CastleMesh` (root), `EWidgetSpace::Screen`, DrawSize (256, 32), relative location (0, 0, 1050), collision explicitly `NoCollision`.

Widget class: `TSoftClassPtr<UUserWidget> HPBarWidgetClass` (EditDefaultsOnly, category "Siegebound|Castle|Visuals"), constructor default `/Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C`. Resolved in new private `InitHPBarWidget()`, called from `BeginPlay` after the seed broadcast:
- `LoadSynchronous()` null → early return, **no log** (spec: "silent no-op, never a crash"). `TSoftClassPtr<UUserWidget>::LoadSynchronous` also returns null for a class that isn't a UUserWidget subclass, so a mis-authored asset degrades the same way.
- Loaded → `HPBarWidget->SetWidgetClass(LoadedClass)`; because the component has begun play by then (our code runs after `Super::BeginPlay()`), `SetWidgetClass` internally calls `InitWidget()` and creates the instance.
- `Cast<UCastleHealthBarWidget>(HPBarWidget->GetWidget())` succeeds → `InitForCastle(this)`; any other widget class is skipped silently.

Visibility: `HandleDestroyed()` calls `HPBarWidget->SetVisibility(false, true)`; `ResetCastle()` calls `SetVisibility(true, true)` before the reset broadcast.

### UCastleHealthBarWidget (spec point 3) — NEW class

- `UFUNCTION(BlueprintCallable) void InitForCastle(ACastle* Castle)`: null castle → warn (`LogGitClaudeUnrealTest`) + no-op. Otherwise **seed FIRST** — calls `OnHPChanged(Castle->GetCurrentHP(), Castle->GetMaxHP())` immediately — **THEN binds** `OnCastleHPChanged` via `AddUniqueDynamic` (seed-then-bind, qa/TASK-005-report.md major 2).
- `UFUNCTION(BlueprintImplementableEvent) void OnHPChanged(float CurrentHP, float MaxHP)` — float params only (MCP cannot author enum BP params; CONVENTIONS widget rules). TASK-019 implements it as ProgressBar SetPercent(CurrentHP / MaxHP) with a MaxHP > 0 guard.
- Delegate is bound to a protected `UFUNCTION() HandleCastleHPChanged(float, float)` that forwards to `OnHPChanged` (see flagged decision 4).
- `UPROPERTY(Transient) TObjectPtr<ACastle> ObservedCastle` tracks the bound castle so a repeat `InitForCastle` with a DIFFERENT castle removes the old binding first (never two update streams), and a repeat with the SAME castle is a re-seed with no double-bind (`AddUniqueDynamic`).

## Acceptance criteria → where satisfied

| Criterion | Where |
|---|---|
| Compiles/runs with no widget asset | `InitHPBarWidget` early-returns on null class; bare component draws nothing |
| 3 enemy hits = exactly 3 broadcasts, correct values | single broadcast site on the actual-damage path in `TakeDamage` |
| Friendly damage = 0 broadcasts | friendly-fire early-out sits above the broadcast |
| Destroyed → bar hidden | `HandleDestroyed` explicit `SetVisibility(false)` |
| ResetCastle → broadcast(2000, 2000) + bar visible | `ResetCastle` shows the component, then broadcasts unconditionally |

## FLAGGED DECISIONS (QA: please rule on each)

1. **Explicit `SetVisibility` on HPBarWidget instead of relying on `SetActorHiddenInGame`.** Screen-space widget components do not follow actor hidden-in-game state — the viewport screen layer is gated on the COMPONENT's `IsVisible()`, and `SetActorHiddenInGame` does not touch component visibility flags. Without the explicit call the bar would float over an invisible destroyed castle. I believe this is required, not optional; rule if you read the engine differently.
2. **Widget creation relies on `UWidgetComponent::SetWidgetClass` triggering `InitWidget()` when called after the component has begun play.** That is the engine's documented post-BeginPlay behavior; I did not also call `InitWidget()` manually (a second call would recreate the widget). If the class ever gets set BEFORE component BeginPlay this path changes — flag if you want a defensive `if (!HPBarWidget->GetWidget()) HPBarWidget->InitWidget();` after SetWidgetClass.
3. **`UCLASS()` plain, not `UCLASS(Abstract)`, on UCastleHealthBarWidget.** Abstract is the classic choice for C++ widget bases, but TASK-019's MCP reparenting workflow is unproven against abstract parents, and a non-abstract base costs nothing. Deliberate; rule if you want Abstract.
4. **Explicit forwarding handler `HandleCastleHPChanged` instead of `AddDynamic` directly onto the BlueprintImplementableEvent.** Binding a BIE to a dynamic delegate does work (name-based ProcessEvent), but the explicit UFUNCTION handler makes the RemoveDynamic on re-target unambiguous and keeps the BIE a pure view-update hook. No behavior difference.
5. **Re-target support in InitForCastle is beyond the letter of the spec** (spec only demands seed-then-bind). M1 never re-targets a bar, but PlayAgain-era widget churn made defensive rebinding cheap insurance. Flag if you consider it scope creep.
6. **No log when WBP_CastleHealthBar is missing.** Spec says "silent no-op" — this deliberately deviates from the M1 house style of "log once and continue" for missing soft assets (cf. SiegePlayerController's HUD load). Spec wording wins; rule if you want a Verbose-level log instead.
7. **`SetCollisionEnabled(NoCollision)` on HPBarWidget is beyond spec.** Screen-space widget components build no collision geometry anyway, but the explicit call guarantees the component can never interfere with TASK-007's cursor-to-ground trace or TASK-004's acquisition regardless of engine defaults. Same rationale QA pre-approved for CastleMesh's explicit BlockAll in TASK-002.
8. **No explicit unbind on widget destruction (no NativeDestruct override).** Dynamic multicast delegates hold weak references and skip/compact dead listeners; the widget's lifetime is owned by the castle's own component, so castle and bar die together. Deliberate omission.
9. **BeginPlay ordering: the seed broadcast fires BEFORE the widget exists.** The bar does not need that broadcast — `InitForCastle` seeds it directly from the getters. The BeginPlay broadcast serves pre-bound listeners (level BP / framework), per spec. Both orderings are correct for the bar; noting so nobody "fixes" it into a double-seed.

## Notes for TASK-019 (widget asset) and integration

- Reparent the UI_LifeBar duplicate to `UCastleHealthBarWidget`, implement event `OnHPChanged` (CurrentHP, MaxHP floats) → SetPercent(CurrentHP / MaxHP) with a MaxHP > 0 guard. Do NOT call InitForCastle from the widget BP — ACastle::BeginPlay drives initialization.
- No editor-side wiring needed on castle instances: HPBarWidgetClass default already points at `/Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C`.
- Build-master: no Build.cs change from this task; expect a merge-free coexistence with TASK-016's Niagara edit (different files entirely).
