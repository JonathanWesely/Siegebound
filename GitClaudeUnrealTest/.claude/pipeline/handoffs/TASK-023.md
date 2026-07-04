# TASK-023 Handoff — PlayerController v2: hand play, discard, refusal messages, input plumbing

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git, no board edits — M2 dispatch gates)

## Files changed (the complete change set)

1. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
2. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

No other file touched. DeckComponent.*, CardRow.h, SiegePlayerState.*, and all other frozen contracts read-only. CardHandWidget.h/.cpp (concurrent TASK-029) NOT touched — but cross-checked read-only: its `PlayHandSlot` / `DiscardHandSlot` call sites, its `OnCardRefused.AddUniqueDynamic` with a `const FString&` handler, and its `FindComponentByClass<UDeckComponent>()` all match this implementation exactly. No Build.cs change (EnhancedInput/UMG already dependencies since M1).

## What it does (spec points 1–5)

1. **DeckComponent** — `CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"))` in the constructor. `BuildAndShuffle()` at match start = `BeginPlay` immediately after Super (see decision 10), BEFORE HUD widget creation. `ResetDeck()` inside `HandleMatchReset()` — the PlayAgain flow (`ASiegeGameMode::PlayAgain` already calls `HandleMatchReset` on every controller, SiegeGameMode.cpp:356).
2. **PlayHandSlot(int32 Slot)** (BlueprintCallable) — resolves the slot's CardID from the deck, then the DT_Cards row; refuses on empty slot, missing row, gold < Cost ("Not enough gold" — grey-out stays the widget's job), or unsupported type. Unit/Building/Economy → records `PendingHandSlot` and routes into the existing M1 `EnterPlacementMode(CardID)` (TASK-030 replaces the internals behind the same entry); Spell → log + refuse (M5); HeroUpgrade/Utility → log + refuse (M4). The card leaves the hand ONLY at CONFIRM: `TryConfirmPlacement` calls `DeckComponent->ConfirmPlayFromHand(PendingHandSlot)` after SpendGold + FinishSpawning. Cancel costs nothing (the hand was never touched).
3. **DiscardHandSlot(int32 Slot)** (BlueprintCallable) — empty-slot pre-check (WARN-1 guard, decision 1), then `SpendGold(DiscardCost=1)` (refused at 0 gold, §3.6), then `DiscardFromHand(Slot)`.
4. **Refusal surface** — `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardRefused, const FString&, Reason)`, member `UPROPERTY(BlueprintAssignable) OnCardRefused`, broadcast on EVERY refused play/discard with the §3.0-style reason. Play refusals also still fire the M1 `OnCardPlayRefused(FName, FText)` (decision 4).
5. **Input plumbing** — UPROPERTY slots `Card2Action..Card6Action` + `UICursorAction` following the exact IA_Card1 pattern (hard slot + soft-path companion + `ResolveInputAction` + null-safe bind). Keys 1..6 → `PlayHandSlot(0..5)` (key 1 via the fallback handler, decision 2; keys 2–6 via one payload-bound handler). IA_UICursor held = GameAndUI + visible cursor + `SetIgnoreLookInput(true)`; released = M1 game-only free-look restored. Placement mode's own cursor behavior unchanged (the two cursor owners compose, decision 11).

Reason strings shipped: "Not enough gold", "No card in that hand slot", "Card data unavailable", "Hero is down", "Invalid placement location" (existing), "Cannot discard while placing a card", "Spells are not available yet", "Instant cards are not available yet". ("Miner limit reached" belongs to TASK-030.)

## Contract for TASK-032 (input assets)

- **IA_Card1's binding-site property names (the spec's "record if it differs" ask): hard slot `Card1Action`, soft companion `Card1ActionAsset` — the new actions follow it 1:1**: `Card2Action`/`Card2ActionAsset` .. `Card6Action`/`Card6ActionAsset`, `UICursorAction`/`UICursorActionAsset` (all Category "Input"; hard slots EditAnywhere+BlueprintReadOnly, soft paths EditDefaultsOnly).
- Constructor-seeded soft paths (exact): `/Game/Input/Actions/IA_Card2.IA_Card2` .. `IA_Card6.IA_Card6`, `/Game/Input/Actions/IA_UICursor.IA_UICursor`. **Creating the assets at those exact paths is sufficient — no editor assignment needed** (M1 pattern; no BP controller subclass exists).
- Map in IMC_Hero: keys `2`..`6` → IA_Card2..6 (key `1` → IA_Card1 already exists), **Left Alt** → IA_UICursor. Plain digital (bool) actions; no special triggers needed — I bind `Started` for press and BOTH `Completed` and `Canceled` for the UICursor release, so a Hold trigger or a focus-loss flush cannot stick the hold.
- Until the assets exist: one warning per unresolved action at SetupInputComponent (6 new lines: IA_Card2..6 + IA_UICursor), every binding skipped null-safe. Nothing else degrades.

## Contract for TASK-033 (hand HUD)

- Buttons call `PlayHandSlot(0..5)` / `DiscardHandSlot(0..5)`; ALL refusal logic is controller-side and comes back through `OnCardRefused(const FString& Reason)` — exactly one broadcast per refused play/discard. (TASK-029's UCardHandWidget already binds/relays these 1:1 — verified against its source.)
- Deck access: BlueprintReadOnly `DeckComponent` property, BlueprintPure `GetDeckComponent()`, or `FindComponentByClass<UDeckComponent>` (TASK-029 uses the latter). **Init order guarantee: the deck is built before any BeginPlay-created widget initializes** (BuildAndShuffle precedes HUD creation inside BeginPlay), so seed-then-bind always seeds a dealt hand; `ResetDeck` fires inside `HandleMatchReset` on Play Again.
- Cursor for clicking cards/discard: hold Left Alt (IA_UICursor). The widget needs no input-mode code of its own.
- **Key-1 fallback retirement**: `OnCard1Pressed`'s empty-hand fallback to `EnterPlacementMode(Card1CardID)` is load-bearing until TASK-031's reimport + TASK-033's HUD replacement; retire it (with manager sign-off) when WBP_HUD's Footman button dies.

## Contract for TASK-030 (placement v2)

Keep the `EnterPlacementMode(FName)` entry signature and the `PendingHandSlot` confirm contract: at commit call `ConfirmPlayFromHand(PendingHandSlot)` when != INDEX_NONE (INDEX_NONE = non-hand M1 play); `ExitPlacementMode` clears it on every exit. Route new refusals (miner cap, clearance) through `RefuseCardPlay` so both delegates fire.

## Flagged decisions — QA must rule on each

1. **WARN-1 gold-leak guard: empty-slot pre-check chosen, NOT spend-then-refund.** `GetHandCardID(Slot).IsNone()` refuses BEFORE `SpendGold(1)`. Why this option: (a) ASiegePlayerState has no refund/AddGold API and this task's file scope forbids adding one — refund was not implementable; (b) pre-check avoids a two-broadcast gold flicker (spend→refund would fire OnGoldChanged twice per refused discard); (c) refusal-before-state-change matches the CONVENTIONS refused-mutation law. The post-spend `DiscardFromHand == false` branch is kept as a loud Error tripwire but is unreachable within one call stack (nothing can mutate the hand between the pre-check and the call).
2. **Key "1" dual behavior** (M1-preservation vs "keys 1–6 attempt the right slots"): `OnCard1Pressed` plays hand slot 0 when it holds a card, else falls back to the M1 `EnterPlacementMode(Card1CardID="Footman")`. In the WARN-2 window (every slot NAME_None until TASK-031) key 1 is byte-identical to M1; with a real 6-card hand slot 0 is never empty (§3.4 immediate redraw), so the fallback can never shadow a hand card and the acceptance holds.
3. **Slot pass-through via the `PendingHandSlot` member, not an EnterPlacementMode signature change.** UFUNCTION overloads are illegal, and adding a defaulted param would invalidate WBP_HUD's existing compiled call node (M1-preservation hard rule). Set by PlayHandSlot immediately before EnterPlacementMode; cleared on failed entry (`!bInPlacementMode` after the call — only the dead-hero refusal can reach that, every other gate is pre-checked in the same stack) and on EVERY placement exit; consumed at confirm AFTER FinishSpawning (unit + gold committed first, then the card leaves the hand — M2 CONFIRM ruling).
4. **RefuseCardPlay broadcasts BOTH delegates** (M1 `OnCardPlayRefused(CardID, FText)` + M2 `OnCardRefused(FString)`); **discard refusals fire ONLY `OnCardRefused`** via the new `BroadcastRefusal` helper — a discard is not a card play, so the M1 delegate's semantics stay clean. "Broadcast on EVERY refused play/discard" is satisfied on the M2 surface.
5. **Post-match and mid-placement `PlayHandSlot` presses are silent ignores** (Verbose log, no broadcast) — mirrors the M1 EnterPlacementMode early-outs and the CONVENTIONS no-broadcast-for-ignored-mutations law. Not player-facing refusals.
6. **`DiscardHandSlot` during placement mode is a broadcast REFUSAL** ("Cannot discard while placing a card"), not a silent ignore: discarding the slot being placed would make the pending confirm consume the REPLACEMENT card (§3.4 refills the slot immediately) — refusing protects slot integrity; cancel is free, then discard.
7. **Affordability gate before type dispatch in PlayHandSlot** (the spec's literal order): an unaffordable Spell refuses "Not enough gold", not the M5 message. EnterPlacementMode re-checks the same gate in the same call stack — cannot disagree, no double broadcast.
8. **Instant/Spell mapping**: `ECardType::Spell` → M5 refusal; `ECardType::HeroUpgrade`, `ECardType::Utility`, and `default` → M4 "Instant" refusal (GDD §3.5 groups Hero Upgrade / Utility as the Instant category; default catches future enum values safely).
9. **`DiscardCost = 1` as an EditDefaultsOnly UPROPERTY** with a GDD §3.6 comment — mechanic rule, not a CSV column (CONVENTIONS registry; same treatment QA passed for HandSize/ExpectedDeckSize/miner cap). The spec's literal `SpendGold(1)` is honored through the default value.
10. **Deck init timing (discharges TASK-022 flagged decision 12)**: "match start" = controller BeginPlay right after Super (PIE start is match start; no pre-match phase exists in M2), ordered before HUD creation for the seed-then-bind guarantee. "PlayAgain flow" = inside `HandleMatchReset`, which `ASiegeGameMode::PlayAgain` already invokes — this doubles as the controller-side deck-reset entry point TASK-024's spec names, with no new API.
11. **`ApplyPlacementInputState(bool)` refactored to `ApplyCursorInputState()`** (private helper — no external contract): computes input state from the two composable cursor owners (`bInPlacementMode`, `bUICursorHeld`); either → GameAndUI + cursor (config byte-identical to M1 placement: DoNotLock, HideCursorDuringCapture false, bEnableClickEvents); neither → GameOnly + hidden. Early-outs on `bMatchEnded` so HandleMatchEnd keeps sole ownership of the UIOnly end-screen state. Automatic consequences: Alt released mid-placement leaves placement's cursor alone; placement exit while Alt is held keeps the cursor up ("placement mode's own cursor behavior unchanged").
12. **IA_UICursor = Started/Completed/Canceled triple binding + `bUICursorHeld` guard.** `SetIgnoreLookInput` is counter-based and floor-clamped (verified in the installed 5.8 engine, Controller.cpp:190-193) — the guard pairs +1/-1 exactly once regardless of double press/release delivery.
13. **Stuck-hold recovery**: UIOnly input (match end) can swallow the action's release event, so `ClearUICursorHold()` runs in HandleMatchEnd (primary, before the UIOnly switch), HandleMatchReset (belt-and-braces before play resumes), and EndPlay (teardown symmetry). A physically still-held Alt after reset re-arms on its next press — same rule QA accepted for sprint (qa/TASK-003-report.md nit 5).
14. **qa/TASK-007-report.md NIT-2 fixed on this touch** (as that report requested): the dead-hero refusal in EnterPlacementMode now broadcasts `RefuseCardPlay(CardID, "Hero is down")` like every other player-facing refusal.
15. **No-PlayerState paths remain log-only Errors** (no OnCardRefused): engine misconfiguration, not a §3.0 player-facing refusal — matches the M1 QA-passed behavior.
16. **Melee-suppression LAW re-audited** (qa/TASK-003-report.md warning 2): NO new suppression sites and NO new exit paths — PlayHandSlot enters placement only through EnterPlacementMode, every exit still funnels through ExitPlacementMode (whose release-before-early-out is untouched), and the UICursor hold never touches suppression. The QA-verified seven exit paths stand unchanged.
17. **`GetDeckComponent()` additive BlueprintPure getter** beyond the names block (TASK-029/033 convenience; TASK-029's FindComponentByClass also works). Precedent: TASK-022 decision 4 (GetHandSize) ruled PASS. Inline with a forward-declared return type — the ACharacter::GetCharacterMovement engine pattern.
18. **Empty-slot play refusals pass `CardID = NAME_None` to OnCardPlayRefused** (the slot has no card; the reason text carries the message). Harmless to the optional M1 HUD binding.

## Things QA should scrutinize

- **Payload BindAction**: `BindAction(Action, ETriggerEvent::Started, this, &ThisClass::OnCardSlotKeyPressed, ActionIndex + 1)` — the VarTypes overload verified in the installed engine (EnhancedInputComponent.h:480-492, `DEFINE_BIND_ACTION(FEnhancedInputActionHandlerSignature)`); handler signature `void(int32)` receives only the payload. `static_cast<int32>(UE_ARRAY_COUNT(...))` avoids the signed/unsigned C4018 in the loop bound.
- **M1 preservation walk**: WBP_HUD button → EnterPlacementMode("Footman") — signature and behavior untouched, PendingHandSlot stays INDEX_NONE so confirm is byte-identical to M1; key 1 in the WARN-2 window → fallback → same call. Input transitions with bUICursorHeld=false are equivalent to the old ApplyPlacementInputState(true/false). The only intended M1 behavior delta: decision 14 (dead-hero refusal now broadcasts).
- **WARN-1 acceptance walk (§3.6)**: discard at >=1 gold → pre-check passes, SpendGold(1) deducts exactly 1, DiscardFromHand moves + redraws; at 0 gold → SpendGold refuses, OnCardRefused("Not enough gold"), no state change; empty slot → refused BEFORE SpendGold, zero gold moved.
- **`const FString&` dynamic-delegate param** — reflection-legal, matches TASK-029's already-written handler exactly.
- Not compiled (standing gate — TASK-039 batch compile). No Build.cs change; header self-contained.

## Board note

TASKBOARD.md deliberately not edited (dispatch hard rule; orchestrator owns board writes). Requested status: `in-progress` -> `ready-for-qa`.
