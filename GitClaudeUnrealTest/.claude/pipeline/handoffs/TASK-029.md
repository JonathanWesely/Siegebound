# TASK-029 Handoff — Card hand widget C++ base: UCardHandWidget

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor/MCP, no Git, no board/CONVENTIONS edits — per M2 dispatch gates)

## Files created (the complete change set)

1. `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h` — UCardHandWidget (UUserWidget), spec functions InitForController / RequestPlaySlot / RequestDiscardSlot, BIEs OnHandSlotUpdated / OnNextCardUpdated / OnCardRefusedMessage.
2. `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp` — implementation.

No other file touched. SiegePlayerController.h/.cpp (TASK-023's concurrent files) NOT touched — this class only `#include`s and calls them. DeckComponent.*, CardRow.h, SiegePlayerState.h read-only upstream contracts untouched. **No Build.cs change needed** — verified: "UMG" (UUserWidget) at Build.cs line 20 since M1, "Engine" (UDataTable) since always.

## What it does (GDD §3.4/§3.5/§3.6/§7)

C++ half of the hand UI. Pulls model state (deck slots + preview from UDeckComponent, gold from ASiegePlayerState, refusals from ASiegePlayerController) and pushes display-ready data to the UMG side through three BlueprintImplementableEvents whose params are int/FString/bool only (CONVENTIONS widget rules — MCP cannot author enum BP params). Costs and DisplayNames are read from /Game/Data/DT_Cards rows at push time (soft path, house LoadSynchronous pattern) — never typed into UMG. UMG buttons route back through two null-safe pass-throughs into the controller, which owns ALL refusal logic.

## Exact contract for TASK-033 (WBP_CardHand duplication/reparent)

### Asset + reparent

- Duplicate a donor per CONVENTIONS (MCP cannot author widget trees from scratch), save as **/Game/UI/WBP_CardHand**, reparent to **UCardHandWidget**.

### Init call site (exact expectation)

- Call **`InitForController(ASiegePlayerController*)`** ONCE, from the widget's **Event Construct**: `Cast<ASiegePlayerController>(GetOwningPlayer())` → `InitForController`. (BlueprintCallable; also callable from the controller side if TASK-023/033 prefer — safe either way.)
- **Init order vs BuildAndShuffle does not matter.** Seed-then-bind + the deck's forced build/reset broadcasts make the widget correct in both orders: constructed before the deck builds, it seeds empty slots and is repainted by the build's own OnDeckHandChanged + forced OnDeckNextCardChanged; constructed after, it seeds the real state (and a never-changing preview still displays — that is what the seed is for).
- Repeat calls are safe: same controller = re-seed, never double-binds (AddUniqueDynamic); different controller = old bindings dropped first (never two update streams).
- Do NOT also bind WBP-side to the deck/gold/refusal delegates — the C++ base is the single update pipe.

### Seed-then-bind order inside InitForController (CONVENTIONS law, qa/TASK-005-report.md major 2)

1. Resolve deck (FindComponentByClass) + player state (GetPlayerState).
2. **SEED**: push `OnHandSlotUpdated` for every slot 0..GetHandSize()-1, then `OnNextCardUpdated(PeekNextCardID row data)`.
3. **THEN bind**: `OnDeckHandChanged`, `OnDeckNextCardChanged`, `OnGoldChanged`, `OnCardRefused` (all AddUniqueDynamic).

Refusal messages are events, not state — nothing to seed; the message line starts hidden.

### BlueprintImplementableEvent signatures (character-exact, implement all three)

```cpp
// One hand slot. Fires per slot on: init seed, every hand change (coarse
// refresh — all 6 re-push), every gold change (affordability re-grey).
// MUST be idempotent (repeat calls with identical data are normal).
void OnHandSlotUpdated(int32 SlotIndex, const FString& CardID, const FString& DisplayName, int32 Cost, bool bAffordable);

// The §3.4 next-card preview. Empty DisplayName = no next card -> hide the
// preview slot. Deliberately NO affordability param (spec: preview is
// name + cost, not a playable button).
void OnNextCardUpdated(const FString& DisplayName, int32 Cost);

// §3.0 refusal surface ("Not enough gold", "Miner limit reached", ...).
// Exactly one call per refused play/discard. Show ~2 s then hide — the
// timing/animation is implemented in UMG (timer or widget anim), not C++.
void OnCardRefusedMessage(const FString& Reason);
```

Param semantics for `OnHandSlotUpdated`:
- `SlotIndex`: 0-based, 0..5 (loop bound is `UDeckComponent::GetHandSize()`, value 6 per GDD §3.4).
- `CardID`: DT_Cards row name as string. **EMPTY string = EMPTY slot → hide the card face.** Never compare against the literal "None". Empty slots are the NORMAL state until the TASK-031 reimport (qa/TASK-021-report.md WARN-2) — not an error.
- `DisplayName`: row's human name ("Arrow Tower"); falls back to the raw CardID string when the row is missing.
- `Cost`: row Cost; 0 for empty slot or missing row.
- `bAffordable`: current gold >= Cost (§3.5 — grey the card when false). Cosmetic only; the controller re-checks authoritatively. Key the hide-empty-frame decision off `CardID` emptiness, NOT off bAffordable (an unaffordable card still shows, greyed).

### Button wiring

- Card slot button (slot N) → **`RequestPlaySlot(N)`** → controller `PlayHandSlot(N)`.
- Discard button (slot N; shows the 1-gold cost per §7) → **`RequestDiscardSlot(N)`** → controller `DiscardHandSlot(N)`.
- No pre-checks needed in UMG — refusals come back as `OnCardRefusedMessage` automatically.

## Cross-task symbol dependencies (QA: cross-check when TASK-023 lands)

This file pair compiles only together with TASK-023's controller additions (batch compile TASK-039 — the designed parallel-dev pattern). Symbols consumed, exactly per the TASK-023 board spec/names block:

1. `ASiegePlayerController::PlayHandSlot(int32)` — called by RequestPlaySlot.
2. `ASiegePlayerController::DiscardHandSlot(int32)` — called by RequestDiscardSlot.
3. `ASiegePlayerController::OnCardRefused` — public `UPROPERTY(BlueprintAssignable)` of a dynamic multicast delegate `FOnCardRefused(const FString& Reason)`; my handler is `void HandleCardRefused(const FString& Reason)` and MUST match TASK-023's param type character-for-character (`const FString&`, not `FText` — note the M1 `OnCardPlayRefused(FName, FText)` is a DIFFERENT, older delegate this widget deliberately does not bind).
4. `UDeckComponent` default subobject on the controller — found via `FindComponentByClass` (see flagged decision 1).

Frozen contracts consumed (already QA-passed): `UDeckComponent::GetHandCardID/PeekNextCardID/GetHandSize` + `OnDeckHandChanged`/`OnDeckNextCardChanged` (TASK-022); `ASiegePlayerState::CanAfford(int32)` + `OnGoldChanged(int32 NewGold)` (TASK-005); `FCardRow::DisplayName/Cost` (TASK-008/021).

## Flagged decisions — QA must rule on each

1. **Deck access via `FindComponentByClass<UDeckComponent>()`** instead of reading TASK-023's `DeckComponent` member directly. TASK-023's names block pins the member's NAME but not its access specifier, and its files are mid-flight in a concurrent agent (frozen to me). FindComponentByClass finds the default subobject regardless of visibility, is null-safe, and keeps working if TASK-023 later adds a getter. Cost: one linear component scan per InitForController call (once per widget lifetime) — nothing per-frame.
2. **Empty-slot signal = EMPTY CardID string, never "None".** `NAME_None.ToString()` returns the literal `"None"`, which UMG could not distinguish from a card actually named None. The spec's FString param forces a convention; empty-string-means-empty is the unambiguous one. Documented in the BIE doc comment and the TASK-033 contract above.
3. **Gold re-grey = full re-push of all 6 slots through the one shared path** (seed, hand-change, gold-change all call the same RefreshAllHandSlots → PushHandSlot). No separate affordability event (the spec pins exactly three BIEs) and no value-filtering of pushes: BIE pushes are view updates, not delegate broadcasts, so the CONVENTIONS broadcast law doesn't demand filtering, and the stateless re-push can never hold a stale grey. Load: ~6 pushes + 6 row lookups per income tick (1/s) — trivial (FindRow is a TMap lookup on an already-loaded table; LoadSynchronous on a loaded soft ref is a cache hit).
4. **Degrade path for missing card data**: missing DT_Cards row → slot shows the raw CardID, Cost 0, bAffordable false (identifiable on screen, unplayable-looking, log names the fault); missing table entirely → same degrade for all card slots. Both warn ONCE (once per unique CardID / once per widget) because pushes recur every second — per-lookup warnings would spam the PIE log.
5. **Null-deck / null-player-state at Init**: warn + skip what cannot be served — no slot/preview pushes without a deck (nothing authoritative to display; UMG keeps its design-time state), affordability renders false without a player state (unknown gold ⇒ conservative grey; the authoritative check is controller-side anyway). The widget never fabricates an OnCardRefusedMessage — the refusal surface belongs to the controller exclusively (keeps "exactly one message per refused action" provable).
6. **Request* pass-throughs do no local validation beyond the null-controller guard** — no affordability pre-check, no slot-range clamp. Spec calls them pass-throughs; the controller/deck own every refusal + range check (deck logs and refuses out-of-range slots). A widget-side pre-check would create a second refusal authority that could disagree with the controller (e.g. race with a same-frame gold change).
7. **Re-target support + AddUniqueDynamic + NO NativeDestruct unbind** — all three copied from the QA-passed UCastleHealthBarWidget precedent (handoffs/TASK-018.md flagged decisions 4/5/8): explicit UFUNCTION handlers forwarding to the BIEs; a different controller drops every old binding first; no destruct-time unbind (dynamic multicast delegates weak-ref and compact dead listeners; the widget lives and dies with the controller's HUD).
8. **Pre-build seed window produces per-slot out-of-range warnings from the DECK, by design.** If TASK-033 wires Init before TASK-023's BuildAndShuffle has run, the seed pass calls GetHandCardID(0..5) on a pre-build EMPTY Hand array — UDeckComponent's documented out-of-range behavior logs one Warning per slot and returns NAME_None, so the widget seeds 6 empty slots and is repainted moments later by the build broadcasts. Harmless, self-healing, and self-diagnosing (the log says exactly what raced). Do NOT "fix" by seeding after bind. If the noise bothers anyone, the fix is call-site ordering (Init after match start), not widget logic.
9. **Plain `UCLASS()`, not `UCLASS(Abstract)`** — same rationale QA accepted in TASK-018 decision 3: the MCP reparenting workflow (TASK-033) is unproven against abstract parents and non-abstract costs nothing.
10. **HandleGoldChanged ignores its NewGold param** — affordability is recomputed through `ASiegePlayerState::CanAfford` inside PushHandSlot so all three refresh triggers share one code path; OnGoldChanged broadcasts post-mutation (SiegePlayerState.cpp SetGold), so CanAfford already sees NewGold. No drift possible.
11. **Constructor uses the `FObjectInitializer` form** (UUserWidget's constructor signature) solely to set the CardTableAsset soft default `/Game/Data/DT_Cards.DT_Cards` — byte-identical pattern to UDeckComponent/SiegePlayerController's table member, EditDefaultsOnly for override.

## Things QA should scrutinize

- Handler/delegate signature matches: `HandleDeckHandChanged()` ↔ FOnDeckHandChanged (no params); `HandleDeckNextCardChanged(FName)` ↔ FOnDeckNextCardChanged(FName); `HandleGoldChanged(int32)` ↔ FOnGoldChanged(int32); `HandleCardRefused(const FString&)` ↔ TASK-023's FOnCardRefused(const FString&) — the last one is cross-task and unverifiable until TASK-023 lands.
- Acceptance walk-through: seed happens strictly before any AddUniqueDynamic (both statements ordered in InitForController); gold flip = HandleGoldChanged → RefreshAllHandSlots → CanAfford(Cost) per slot; one-message-per-refusal = single AddUniqueDynamic binding + 1:1 forward, and re-inits cannot double-bind.
- `UDataTable::FindRow<FCardRow>(FName, context, /*bWarnIfRowMissing*/ false)` — warning suppressed engine-side because this class does its own once-per-CardID logging.
- Not compiled (standing gate — TASK-039 batch compile post-sign-off). Header self-contained; UMG + Engine module types only; no Build.cs change (verified line 20).

## Board note

TASKBOARD.md deliberately not edited (dispatch hard rule; orchestrator owns board writes). Requested status: `backlog` → `ready-for-qa`.

---

## Build-fix (loop 1) — 2026-07-04

Trigger: the M2 batch compile (TASK-039) failed at UnrealHeaderTool, which stopped at the FIRST file:

```
CardHandWidget.h(98):  Error: Function parameter: 'Slot' cannot be defined in 'RequestPlaySlot' as it is already defined in scope 'UWidget' (shadowing is not allowed)
CardHandWidget.h(106): Error: Function parameter: 'Slot' cannot be defined in 'RequestDiscardSlot' as it is already defined in scope 'UWidget' (shadowing is not allowed)
```

Root cause: `UCardHandWidget : UUserWidget : … : UWidget`, and `UWidget` has a reflected `Slot` UPROPERTY. UHT forbids a UFUNCTION parameter shadowing an inherited reflected property.

### Fix applied (scoped, minimal rename `Slot` → `SlotIndex`)

- `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h`
  - line 98: `void RequestPlaySlot(int32 Slot);` → `void RequestPlaySlot(int32 SlotIndex);`
  - line 106: `void RequestDiscardSlot(int32 Slot);` → `void RequestDiscardSlot(int32 SlotIndex);`
- `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp`
  - `RequestPlaySlot(int32 Slot)` → `RequestPlaySlot(int32 SlotIndex)`; body: log arg + `ObservedController->PlayHandSlot(Slot)` → `PlayHandSlot(SlotIndex)`.
  - `RequestDiscardSlot(int32 Slot)` → `RequestDiscardSlot(int32 SlotIndex)`; body: log arg + `ObservedController->DiscardHandSlot(Slot)` → `DiscardHandSlot(SlotIndex)`.

Only the two BlueprintCallable pass-throughs changed. Every other QA'd signature/seam is byte-for-byte unchanged:
- `ASiegePlayerController::PlayHandSlot(int32 Slot)` / `DiscardHandSlot(int32 Slot)` (APlayerController has no reflected `Slot`) — NOT touched.
- The TASK-033 UMG seam calls `RequestPlaySlot` / `RequestDiscardSlot` **by name** — an internal param rename does not affect the node name, pins, or call site. No downstream contract change.
- The doc comments at CardHandWidget.h:93/102 still read `PlayHandSlot(Slot)` / `DiscardHandSlot(Slot)` — those correctly refer to the CONTROLLER's param (which stays `Slot`), so they were left as-is.

### Proactive sibling scan (all M2 UCLASSes — UHT stopped at file #1, rest unverified)

Swept every UFUNCTION signature across the M2 headers for a parameter name that shadows an inherited **reflected** UPROPERTY on its own parent chain (UWidget/UUserWidget, AActor/APawn/ACharacter, APlayerController, APlayerState, AGameModeBase/AGameStateBase, UActorComponent). Method: grepped all UFUNCTION signatures + a targeted sweep for the known-risky names (`Slot`, `Owner`, `Instigator`, `Tags`, `Role`, `InputComponent`, `RootComponent`, `Controller`, `PlayerState`, `Player`, `Visibility`, `Cursor`, `RenderOpacity`, `ToolTipWidget`, `bIsEnabled`, `bIsActive`, `ComponentTags`, …) used as function params.

Result: **NO additional shadow errors found beyond the two known ones.** Every other occurrence of a risky name is on a parent chain that does NOT reflect that name, or is on a non-UFUNCTION method (UHT does not process those). Detail:

- `UDeckComponent::GetHandCardID/ConfirmPlayFromHand/DiscardFromHand(int32 Slot)` — UFUNCTIONs, but parent is `UActorComponent : UObject`, which has NO `Slot` property. Safe. (`UWidget::Slot` is unrelated — DeckComponent is not a widget.)
- `UDeckComponent::MoveHandCardToDiscardAndRedraw(int32 Slot, const TCHAR* Verb)` — `private:`, non-UFUNCTION (a `const TCHAR*` param can't be reflected anyway). Safe.
- `ASiegePlayerController::PlayHandSlot / DiscardHandSlot / OnCardSlotKeyPressed(int32 Slot)` — `Slot` not reflected on `APlayerController : AController : AActor`. Safe (and the first two are QA'd/frozen).
- `UCardHandWidget::InitForController(ASiegePlayerController* Controller)` — `Controller` is NOT a reflected UPROPERTY on `UUserWidget`/`UWidget` (confirmed: UHT errored only on `Slot`, not on this param). Safe. (NB: `Controller` *is* reflected on `APawn` — but no Pawn/Character subclass here has a `Controller` param.)
- `UCardHandWidget::PushHandSlot(int32 Slot)` — `private:`, non-UFUNCTION. Safe.
- `ASiegeGameMode::GetHeroStartTransform(AController* Player, …)` — non-UFUNCTION helper; `Player` not reflected on `AGameModeBase` chain (`Player` lives on APlayerController). Safe.
- `UCastleHealthBarWidget` (UUserWidget) params `Castle`, `CurrentHP`, `MaxHP` — none reflected on UWidget. Safe.
- `AProjectile::InitProjectile(ETeamId InTeam, AActor* InTarget, float InDamage, …)` — all `In`-prefixed; none reflected on AActor. Safe.
- `AHeroCharacter::SetMeleeSuppressed(bool bSuppressed)` — `bSuppressed` not reflected on ACharacter chain. Safe.
- `ASiegePlayerState::CanAfford/SpendGold(int32 Cost)` — `Cost` not reflected on APlayerState. Safe.
- `ASiegeGameMode::OnCastleDestroyedHandler(ACastle*, ETeamId)`, `HandleHeroDied(AHeroCharacter* DeadHero)`, `ASummonedUnit::InitUnit(ETeamId InTeam, FName InCardID)`, `ABuilding::InitBuilding(...)`, `AGoldNode`/`ACastle`/`ASiegeGameState`/`AMinerUnit` UFUNCTIONs — no risky param names; all safe.

Files-only. Did NOT compile (build-master re-runs TASK-039), did NOT run Git, did NOT edit TASKBOARD.md.

---

## Build-fix (loop 2) — C4458 shadow sweep 2026-07-04

Trigger: TASK-039 attempt #2 got past UHT (loop-1 fixed the two UFUNCTION `Slot` params) but the C++ compiler then failed with `error C4458: declaration of 'Slot' hides class member` — UE compiles variable shadowing as a hard error. Loop-1's scan only checked UHT-visible UFUNCTION params and missed these two `.cpp`-internal shadows of `UWidget::Slot` (`UCardHandWidget : UUserWidget : UWidget`).

### Exact errors fixed

- `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp`
  - `:162` — loop var `for (int32 Slot = 0; Slot < SlotCount; ++Slot)` → `for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)`; body `PushHandSlot(Slot)` → `PushHandSlot(SlotIndex)`.
  - `:168` — helper definition `void UCardHandWidget::PushHandSlot(int32 Slot)` → `int32 SlotIndex`; body uses updated: `GetHandCardID(Slot)` → `GetHandCardID(SlotIndex)`, `OnHandSlotUpdated(Slot, FString(), ...)` → `OnHandSlotUpdated(SlotIndex, ...)` (empty-slot branch), and the final `OnHandSlotUpdated(Slot, CardID.ToString(), ...)` → `OnHandSlotUpdated(SlotIndex, ...)`.
- `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h`
  - `:183` — private declaration `void PushHandSlot(int32 Slot);` → `void PushHandSlot(int32 SlotIndex);` (+ its doc comment updated to read `SlotIndex`).

`SlotIndex` verified non-shadowing (not a member on the UWidget chain) and matches the name already used by the loop-1 `RequestPlaySlot`/`RequestDiscardSlot` params and the `OnHandSlotUpdated` BIE param — the whole file now uses `SlotIndex` consistently. No BlueprintCallable/BIE signature changed (the two BlueprintCallable pass-throughs were already `SlotIndex` from loop 1; `PushHandSlot` is a private, non-UFUNCTION helper so no BP node/pin is affected). No seam, stat, or DT_Cards value changed.

### Shared batch-wide shadow sweep

Grepped **every** `.cpp` and `.h` in `Source/GitClaudeUnrealTest/Siegebound/` for local vars / params / loop vars matching any reflected inherited member name across each class's own base chain (`Owner`, `PlayerState`, `Instigator`, `Controller`, `Slot`, `Role`, `RemoteRole`, `Tags`, `InputComponent`, `RootComponent`, `Children`, `Name`, `Outer`, `bReplicates`, `CustomTimeDilation`, plus the UWidget/UUserWidget member set `Visibility`, `Cursor`, `bIsEnabled`, `Clipping`, `ToolTipText/Widget`, `RenderOpacity/Transform`, `Padding`, `ColorAndOpacity`, `ForegroundColor`, `Priority`, `WidgetTree`, `bIsVariable`, `bIsFocusable`).

**Genuine C4458 shadows in the whole module = exactly 6, all now fixed:** the 2 above (CardHandWidget) + the 4 in `MinerUnit.cpp` (TASK-025 handoff: three `ASiegePlayerState* Owner` locals → `OwnerState` at `:125/:231/:313`, and the `for (APlayerState* PlayerState ...)` loop at `:350` → `IterPlayerState`). Every other hit of a risky name is a verified false positive:

- `UDeckComponent` `Slot` params + loop — `UActorComponent : UObject` has no `Slot` member (only `UWidget` does), so no shadow. Confirmed by attempt #2 flagging CardHandWidget's `Slot` but not DeckComponent's.
- `ASiegePlayerController` `Slot` params — no `Slot` on the `APlayerController` chain.
- `UCardHandWidget::InitForController(... Controller)` — `Controller` is not a member of `UUserWidget`/`UWidget` (it is on `APawn`, irrelevant here).
- `SpawnParameters.Owner/.Instigator` (SummonedUnit/Tower) — local struct-field writes, not member shadows. `/*Owner=*/`/`/*Instigator=*/` (PlayerController) — comment labels. `Cast<...>(InputComponent)` — read of the inherited member (local is `EnhancedInputComponent`).
- `Role`/`RemoteRole`/`Tags`/`RootComponent`/`Children`/`bReplicates`/`CustomTimeDilation` — **zero occurrences** module-wide. `Name`/`Outer` — no local decls, and they are `NamePrivate`/`OuterPrivate` internally so wouldn't C4458 regardless. `Priority`/`Visibility`/`Cursor` hits are comment/string text in non-widget classes.

Confirmation: module is shadow-clean; attempt #3 should have zero C4458s.

Files-only. Did NOT compile (build-master re-runs TASK-039 as attempt #3), did NOT run Git, did NOT edit TASKBOARD.md.
