# TASK-1270 — [DECK-ILLEGAL-ACTIVE] programmer handoff

Status: `ready-for-qa` (gate `TASK-1287`, host `TASK-1289`). No compile, no Live Coding, no editor lifecycle, no asset saves, nothing staged, no Git.

## 1. The measured route — HOW a 68-card deck got saved (deliverable 1)

**Answer: it is a CODE path, and it is a DESIGNED one — the builder's "+" button, one auto-save per press. Not the migration.**

The row's premise ("the builder's '+' greys at `SiegeLegalDeckSize` so 68 was not built by '+'") is wrong in one word: the "+" greys **per card**, not per deck. The chain, quoted:

1. **The "+" grey is per-card.** `DeckBuilderWidget.cpp` `UDeckBuilderWidget::GetCardMaxCopies` (the `UNCAP-§4` compat shim):
   ```cpp
   // ... so the "+" greys exactly at 50-of-one-card.
   const FCardRow* Row = ResolveCardRow(CardID);
   return Row ? SiegeLegalDeckSize : 0;
   ```
   `WBP_DeckCardTile` greys "+" when `GetCountOf(CardID) >= GetCardMaxCopies(CardID)` — i.e. at 50 copies **of that card**. Jonathan's deck1 is 8+28+3+3+8+6+6+6: every entry is far below 50, so "+" never greyed for any of them.

2. **`AddCopy` has NO deck-total guard, on purpose.** `DeckBuilderWidget.cpp` `UDeckBuilderWidget::AddCopy`:
   ```cpp
   // ... Deliberately NO add-time deck-total guard either (U4):
   // over-50 WORKING decks were already reachable and are handled by the live
   // x/50 counter + the exactly-50 legality gate.
   ```
   Law: `CONVENTIONS.md` `UNCAP-§4` — *"It gains NOTHING: ⛔ NO new deck-total guard at add time (U4 default …)"*.

3. **Every successful add auto-saves.** `AddCopy` ends with `PersistWorkingDeck();` → `SaveDeckAs(USiegeDeckSaveGame::MakeFixedDeckName(EditingDeckIndex));` (`DECK-§4`: *"Every working-deck mutation persists IMMEDIATELY to the editing slot"*).

4. **`SaveDeckAs` has no legality gate, by contract.** `DeckBuilderWidget.h`, the `SaveDeckAs` doc: *"No legality gate here — the §8 guide never blocks saving; 'Play with this deck' is the only 50-card gate."* The only refusals in `SaveDeckAs` are empty name / no save object / `SaveGameToSlot` failure.

5. **And the one 50-card gate that used to exist was CUT.** `CONVENTIONS.md` `DECK-§4(c)`: *"⚠️ Named consequence: no UI surface hard-enforces the 50-card rule anymore — `TotalText` "n/50" remains the feedback and the match-side legality check remains the enforcement"* (the `PlayBtn` legality enable-gate was removed when Play was rewired straight to `StartMatch`).

**Migration ruled OUT by measurement, not by argument:** `handoffs/TASK-674-buildmaster.md` row *"Legacy-`"Active"`→deck1 migration (DECK-§4d)"* — *"his real legacy save (one 50-card `"Active"` deck) → `deck1:50`"* on 2026-08-27. `MigrateToFixedSlots` moves `FDeckList`s by value (`SiegeDeckSaveGame.cpp`, clauses 1–3 are `MoveTemp` of whole decks) and never touches a `Cards` array, so it cannot change a count. deck1 was 50 the day it was migrated; the 68 arrived afterwards, through steps 1–4, and the first Warning in his own session is `…backup-2026.09.09-21.01.37.log:2457`.

**Why the seam is NOT closed at the add/save site:** under auto-save, every intermediate deck (the 30-card deck you are halfway through building) is saved; a legality gate on `SaveDeckAs` would make incremental building impossible, and an add-time cap contradicts the standing `UNCAP-§4` U4 ruling. The designed invariant is "a deck may be anything while edited; only a legal deck may PLAY". The defect was that the ACTIVATION step (the one place the player says "this is my match deck") did not enforce it either, and the match then hid the fallback. That is where the two fixes land.

## 2. What changed (diff file list)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` | + `static bool TryActivateSavedDeck(USiegeDeckSaveGame&, const UDataTable*, const FString& Name, FString& OutCanonicalName, FString& OutRefusalReason)` (public, C++-only) · + BIE `OnDeckActivationRefused(const FString& DeckName, const FString& Reason)` · doc riders on `SetActiveDeckBySlot` / `SetActiveDeck` |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | `SetActiveDeck`: the inline name lookup replaced by the gate call; on refusal one Warning (the refused-save idiom) + `OnDeckActivationRefused`, `return` before any write · + the gate's definition after `SetActiveDeck` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | + free function `SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(const FString&, int32)` (external linkage, directly above `BeginPlay`) · the `:308–313` fallback arm (now `:336–369`) additionally composes the notice and broadcasts it next tick via `BroadcastRefusal` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` | + include `DeckBuilderWidget.h` · + forward-declare of the notice composer · + 2 helpers (the 68-card shape) · + 2 tests (suite +2) |
| `.claude/pipeline/handoffs/TASK-1270-programmer.md` | this file |
| `.claude/pipeline/TASKBOARD.md` | my row's `status:` line only |

`DeckLibrary.*` diff = 0 lines. `Saved/SaveGames/` untouched. `KBD-§` / `IMC_*` untouched. `Config/SiegeCloudDev.ini` never opened.

## 3. Deliverable (2) — refuse at activation

`UDeckBuilderWidget::SetActiveDeck` now runs, BEFORE any write:

```cpp
FString CanonicalName;
FString RefusalReason;
if (!TryActivateSavedDeck(*SaveObj, ResolveCardTable(), Name, CanonicalName, RefusalReason))
{
	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("UDeckBuilderWidget::SetActiveDeck('%s'): %s — refused; the active deck stays '%s'."),
		*Name, *RefusalReason, *SaveObj->ActiveDeckName);
	OnDeckActivationRefused(Name, RefusalReason);
	return;
}
```

`TryActivateSavedDeck` = (1) the shipped case-insensitive exists-check, (2) `UDeckLibrary::IsDeckLegal` on THAT saved deck (reused, not duplicated; `OutRefusalReason` = its `OutReason` verbatim), (3) legal ⇒ `Save.ActiveDeckName = canonical`, in memory. `SetActiveDeck`'s persist / `OnDeckModelChanged` / `RefreshDeckBarStates` tail is unchanged and runs on success only. `SetActiveDeckBySlot` (the right-click lane) delegates to `SetActiveDeck` unchanged, so it inherits the gate.

**Expected PIE log line on a right-click of the 68-card deck1** (acceptance 2):
`Warning: UDeckBuilderWidget::SetActiveDeck('deck1'): Deck has 68 cards — a legal deck is exactly 50 (GDD 3.4). — refused; the active deck stays '<current>'.`
Nothing is written, so `GetActiveDeckIndex` (which derives the rim from the saved `ActiveDeckName`) returns the same index and the orange rim does not move. `OnDeckModelChanged` is deliberately not fired (broadcast-on-success-only).

## 4. Deliverable (3) — say so at match start

**NSLOCTEXT key + text, verbatim:**
`NSLOCTEXT("Siegebound", "DeckNotice_IllegalActiveDeck", "Deck '{0}' has {1} cards — playing the default deck")`
Rendered for the measured case: **`Deck 'deck1' has 68 cards — playing the default deck`**

Arm: the existing Warning stays byte-identical; after it, the notice is composed and broadcast **next tick** through `BroadcastRefusal` (the `RefuseCardPlay` sibling → `OnCardRefused` → `UCardHandWidget::HandleCardRefused` → `OnCardRefusedMessage`, the ~2 s show-then-hide in `WBP_CardHand`). The firing site also logs, so the verifier has a line:
`Log: ASiegePlayerController 'SiegePlayerController_0': HUD notice broadcast (TASK-1270): "Deck 'deck1' has 68 cards — playing the default deck"`

## 5. Tests (suite +2, `Siegebound.Deck.*`, same file, same offline lane: in-memory save, transient table, zero disk, zero network, no widget instance)

- **`Siegebound.Deck.IllegalDeckCannotBecomeActive`** — migrated save, deck2 = 51 Footman, deck3 = 50 Footman, deck4 = the real 68-card shape, deck5 empty. Refusals (51 / 68 / empty / unknown name / null table) each assert STATE: `ActiveDeckName` byte-identical (`deck1`), `FindFixedDeckIndex(ActiveDeckName)` still 0 (the rim's source of truth), the whole save deep-equal to its pre-image (`StatesEqual`), reason contains `exactly 50` / `68 cards` / `no saved deck`. Success: `"DECK3"` activates → canonical `deck3`, rim index 2, ONLY `ActiveDeckName` changed; a refusal after the success leaves `deck3` in place.
- **`Siegebound.Deck.IllegalActiveDeckMatchStartNotice`** — the arm's condition on the measured input (68-card shape is illegal by `IsDeckLegal`, reason names `68 cards`), then the exact string for (`deck1`, 68) via `TestEqualSensitive`, no `{0}`/`{1}` leak, and a second pair (`deck7`, 49) proving both arguments are interpolated, not baked.

**Not assertable in this lane, declared:** the BeginPlay arm firing the broadcast exactly once. It needs a world plus the player's deck slot (the controller resolves the slot through `USiegeAccountSubsystem`, whose `SetSlotNameForAutomationTests` redirects the ACCOUNT registry slot only — `GetDeckSlotName()` still composes `SiegeDecks[_<guid>]`, so no test can steer the controller at a scratch deck slot without a new seam on a file this row may not widen). The PIE verifier leg (`TASK-1289` cl. 5) observes it from the log line in §4 (exactly one per match start) and the HUD text.

## 6. Things QA should scrutinize (declared deviations and edges)

1. **`SiegePlayerController.cpp` write outside the literal `:308–313` arm:** the notice composer is a free function placed directly above `BeginPlay` (the `SiegeboundCardGlossary::AppendSpellLines` precedent — external linkage so the test can forward-declare it without a header write; the header is not on the write list). The arm itself is the only behavioural change in the file.
2. **Deferred broadcast:** the arm runs BEFORE `TryInitHUD` in the same `BeginPlay`, so a synchronous `BroadcastRefusal` would fire into an unbound `OnCardRefused` — the player would see nothing again. Deferred with `FTimerManager::SetTimerForNextTick(TFunction<void(void)>&&)` (`TimerManager.h:270`) + a `TWeakObjectPtr` self. Standalone/host: bound by next tick. Client-side PS-retry edge (HUD deferred further): the notice can be missed — declared, not hidden.
3. **Null card table now refuses activation** (`IsDeckLegal`'s own null contract: illegal with reason). Before, `SetActiveDeck` activated regardless; the match would have fallen back anyway. Behaviour change only in a broken-content state.
4. **The not-found log text changed shape:** was `…: no saved deck with that name — save it first (SaveDeckAs).`, now `…: no saved deck with that name — save it first (SaveDeckAs) — refused; the active deck stays '<x>'.` (the reason string is composed by the gate). No test or code depends on the old text (grep).
5. **`OnDeckActivationRefused` is a NEW BlueprintImplementableEvent, unbound in `WBP_DeckBuilder`** (no asset edit this wave). It is not a widget and not a toast class; the Warning log line is the guaranteed refusal surface today. Binding it in the WBP is the zero-code way to show the reason on screen — the manager's call whether to board that.
6. **The em dash** in the NSLOCTEXT is the literal U+2014 character, the same practice as every other `TEXT()` em dash in this module (proven to compile and print by the very Warning this row quotes). Test and source agree byte-for-byte.
7. **The row's premise correction** (§1): "+" greys per card, not per deck. The row's own spec for (2)/(3) is unaffected by it.
8. **Notice count vs reason:** the notice always prints `TotalCount()` (the row's pinned shape). For an illegal deck whose violation is not the count (unknown CardID / negative count at exactly 50), the notice would still read "has 50 cards"; the Warning beside it carries the precise reason.

## 7. `git status --porcelain` at handoff time (git root is one level up, `SC-§102`)

```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h          <- NOT mine: TASK-1271 (sibling programmer)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1271-programmer.md              <- NOT mine: TASK-1271
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md              <- this file (added after the listing above was taken)
```

Mine: the four `Siegebound/` files named in §2, this handoff, and my row's status line on the board. The `ClimbableTower.h` + `TASK-1271-programmer.md` entries are the concurrent TASK-1271 lane, in the same working tree by the manager's one-wave decision.

## Loop 1 fix (2026-09-14)

Trigger: `qa/TASK-1270-verify.md` = `VERIFY-FAILED` (QA loop 1 of 3). Log half observed (one `HUD notice broadcast (TASK-1270)` line per match start); HUD half absent (`WBP_CardHand_C_0.RefusalText` = `TextBlock_21` read `Text: ""`, `Collapsed` at 11 points, t=1.64 s..77.5 s; a positive control painted the same slot). Scope per the manager's LOOP-1 AMENDMENT on the row: delivery may live in `SiegePlayerController.{h,cpp}` and `CardHandWidget.{h,cpp}`; +1 delivery test. No compile, no Live Coding, no editor lifecycle, no `.uasset`, nothing staged, no Git writes, `Saved/SaveGames/` untouched, `ClimbableTower.h` untouched.

### L1.1 The ordering — MEASURED vs ENGINE-SOURCE vs HYPOTHESIS

| # | claim | class | evidence |
|---|---|---|---|
| O1 | `WBP_HUD`'s `Construct` does NOT create the hand; its `Tick` does: `Event Tick` → `IfThenElse(NOT Card Hand Spawned)` → `Set Card Hand Spawned=true` → `CreateWidget(Class=WBP_CardHand_C, Owning Player=GetOwningPlayer)` → `AddToViewport` → … | MEASURED | `unreal_inspector get_asset_graph /Game/UI/WBP_HUD.WBP_HUD`, strands `Tick` + `Construct`, read this loop (read-only) |
| O2 | `WBP_CardHand` `Construct` = `BuildHandTree` → `SetVisibility(Not Hit-Testable Self Only)` → `Cast GetOwningPlayer → SiegePlayerController` → `InitForController` (the last node). `BuildHandTree` creates the `RefusalText` TextBlock (font 20, `Collapsed`) and sets the `Refusal Text` variable BEFORE `InitForController` runs | MEASURED | `get_asset_graph /Game/UI/WBP_CardHand.WBP_CardHand`, strands `Construct`, `BuildHandTree`, `OnCardRefusedMessage` (handler = `IsValid(Refusal Text)` → `SetText` → `SetVisibility(HitTestInvisible)` → `RetriggerableDelay 2.0` → `SetVisibility(Collapsed)`) |
| O3 | `UCardHandWidget::InitForController` is the ONLY binder of `OnCardRefused` in C++ (`CardHandWidget.cpp:90` `AddUniqueDynamic`, `:255` `RemoveDynamic`; no other `OnCardRefused.Add*` in `Source/`) and NO Blueprint binds it: across `Content/**/*.uasset` + `*.umap` the only asset naming `OnCardRefused*` is `WBP_CardHand.uasset`, and only as `OnCardRefusedMessage` (3 hits, the BIE) | MEASURED | grep of `Source/` for `OnCardRefused.(Add/Remove/IsBound/Broadcast/Clear)`; binary grep of `Content/` (`*.uasset`: 1 file, `*.umap`: 0 files); name variants in that file = `3 OnCardRefusedMessage`, nothing else |
| O4 | In `ASiegePlayerController::BeginPlay` the illegal-deck arm runs BEFORE `TryInitHUD()`, which creates `WBP_HUD` synchronously on standalone/host (PlayerState present on the first check) | MEASURED (source) | `SiegePlayerController.cpp` arm `:336–364`, `TryInitHUD();` `:394`, `CreateWidget` + `AddToViewport` inside `TryInitHUD` |
| O5 | Loop 0's next-tick broadcast fired in the SAME frame as the map load: a2 `[2026.09.15-04.35.11:237][377] LogLoad: Took 0.694130 seconds to LoadMap(/Game/Maps/L_Arena)` → `[…:238][377]` LLM budget lines → `[…:251][377] … Sandbox: granted the Blue player 9999 starting gold` → `[…:251][377] … HUD notice broadcast (TASK-1270): "Deck 'deck1' has 51 cards — playing the default deck"`; no line in between names the hand (it logs nothing on a successful bind) | MEASURED (log) | `Saved/Logs/GitClaudeUnrealTest.log` lines 3195–3214, read this loop (PID 13388's log); a1 `[726]` likewise per the verify report |
| O6 | Within one frame the world, including timers, ticks BEFORE any widget ticks: `FEngineLoop::Tick` calls `GEngine->Tick(...)` (`LaunchEngineLoop.cpp:5859`; `UWorld::Tick` → `GetTimerManager().Tick(DeltaSeconds)` at `LevelTick.cpp:1816`) and only later `FSlateApplication::Get().Tick(… TimeAndWidgets)` (`LaunchEngineLoop.cpp:5991`), which is where `SObjectWidget::Tick` → `WidgetObject->NativeTick` (`SObjectWidget.cpp:128`) runs `WBP_HUD`'s BP `Tick` | ENGINE SOURCE (read in the UE 5.8 `Engine/Source`, not runtime-measured) | files and lines named |
| O7 | Therefore the loop-0 broadcast in frame [377]'s world tick hit `OnCardRefused` before `WBP_HUD`'s first Tick could create and bind `WBP_CardHand` — `OnCardRefused.IsBound()` was false at that instant | HYPOTHESIS (the conclusion of O1–O6). `IsBound()` at the broadcast instant and the exact frame the hand bound ([377] Slate phase or [378]) were NOT read at runtime: no log line exists at the bind, and this lane drives no PIE | — |

The verifier's H1 holds in every link that can be read offline (O1–O6). The WARN-1 remedy is refuted by O1 + O4: `HUDWidget` is non-null from `BeginPlay`; the hand is not.

### L1.2 The shape chosen: (A) — a mailbox on the controller, spent by the listener when it binds

- `ASiegePlayerController::QueueMatchStartNotice(const FText&)` holds ONE notice (an empty FText is ignored).
- `ASiegePlayerController::DeliverPendingMatchStartNotice()`: if a notice is held AND `OnCardRefused.IsBound()`, copy it, CLEAR it, log the byte-identical `HUD notice broadcast (TASK-1270)` line, call `BroadcastRefusal(Notice)` (the same M2 lane loop 0 used), return true. Otherwise change nothing and return false.
- Callers: (1) the illegal-deck arm in `BeginPlay`, right after queueing (a hold today, because nothing is bound yet, O3/O4); (2) `UCardHandWidget::InitForController`, right AFTER its `:90` bind. Delivery lands at the LATER of (notice raised, hand bound), in either order, exactly once. It is cleared on delivery, so a re-init, a second hand or a HUD rebuild finds nothing.

**Why (A) and not (B) "re-arm until `IsBound()`":** (B) is a poll with a guessed tick budget. The hand appears on `WBP_HUD`'s first painted Tick, and a load hitch or the client PS-retry (up to `MaxHUDInitAttempts = 120` ticks before `WBP_HUD` even exists) can push that arbitrarily late, so any N can expire silently. A give-up Warning that nobody sees is the loop-0 failure again. (A) is event-driven: it fires on the bind itself, so it cannot be early and cannot time out. It needs no timer and no weak capture, and it closes `qa/TASK-1287-report.md` WARN-1 (the late client HUD) with the same mechanism instead of leaving it declared. **Why no Blueprint change:** the bind is already in C++ (`InitForController`), so the flush sits one line below it and no `.uasset` is touched. **Why the `IsBound()` guard lives inside Deliver** (rather than relying on "the widget calls it after binding"): it makes "never spend a notice on an empty channel" a property of the controller instead of every caller's ordering, and it is what makes the reverse order (a listener bound before the notice is raised) deliver immediately. It is sound today because the hand is the only binder (O3); the header contract names that dependency.

### L1.3 The diff (loop-1 hunks only; line numbers in the working tree after the edit)

| File | Lines | What / why |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | `:345–364` (was `:345–368`) | The arm: the `SetTimerForNextTick` lambda and its `TWeakObjectPtr` are replaced by `QueueMatchStartNotice(SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(ActiveName, ActiveDeck->TotalCount())); DeliverPendingMatchStartNotice();`, and the comment now states the measured ordering. The existing Warning (`:336–338`) and the composer call are unchanged, and the arm is still the only place a notice is queued. |
| same file | `:6152–6209` (new, directly after `BroadcastRefusal`) | Definitions of `QueueMatchStartNotice`, `DeliverPendingMatchStartNotice`, `HasPendingMatchStartNotice`, `GetPendingMatchStartNotice`. **Outside the original `:308–313` fence, as amendment (a) allows:** the listener has to be able to call the delivery, so it cannot live inside `BeginPlay`. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | `:233–258` (public, right after `FOnCardRefused OnCardRefused;`) | The four declarations plus the contract doc. Public because `UCardHandWidget` calls Deliver; C++-only, ⛔ not UFUNCTIONs. |
| same file | `:3478–3480` (private, after `HUDWidget`) | `FText PendingMatchStartNotice;` — ⛔ not a UPROPERTY (an FText holds no UObject reference and is never saved or replicated). |
| `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp` | `:91–100` (after the `:90` bind, end of `InitForController`) | `ObservedController->DeliverPendingMatchStartNotice();` — AFTER the bind, never before (before the bind it would broadcast to nobody). |
| same file | `:163–168` (`HandleCardRefused`, before the BIE) | `++ReceivedRefusalCount; LastReceivedRefusal = Reason;` — records the receipt so an offline test can prove a message REACHED the hand (the BIE has no C++ body). The 1:1 `OnCardRefusedMessage(Reason);` line is untouched and still appears exactly once on a code line (`SiegeFogRefusalTest.cpp:717` counts it). |
| `Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h` | `:149–158` (public, after the `OnCardRefusedMessage` BIE) | `int32 GetReceivedRefusalCount() const` / `const FString& GetLastReceivedRefusal() const` — inline, C++-only, ⛔ not UFUNCTIONs, never drive display. |
| same file | `:313–318` (private, end of class) | `int32 ReceivedRefusalCount = 0;` · `FString LastReceivedRefusal;` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` | `:7`, `:10`, `:14–15` | Includes: `CardHandWidget.h`, `DeckComponent.h`, `SiegePlayerController.h`, `UObject/StrongObjectPtr.h`. |
| same file | `:1036–1037` | CASE 2's doc: "next tick, once the HUD is bound" → "HELD on the controller until the hand binds — the delivery is CASE 3 below". |
| same file | `:1090–1206` | The new test (L1.4). |

Byte-identical, re-checked: the `NSLOCTEXT("Siegebound", "DeckNotice_IllegalActiveDeck", "Deck '{0}' has {1} cards — playing the default deck")` composer (`:258–266`, not touched); the log format string `TEXT("ASiegePlayerController '%s': HUD notice broadcast (TASK-1270): \"%s\"")`, moved from the lambda into `DeliverPendingMatchStartNotice` (`GetNameSafe(Self)` → `GetNameSafe(this)` prints the same name); the existing `is not legal` Warning. A legal deck never queues, so every Deliver is a silent no-op: no log line, no HUD text (row (d)'s premise in the test). `DeckLibrary.*` 0 lines · `DeckBuilderWidget.*` 0 loop-1 lines · `ClimbableTower.h` not written · no `.uasset` · `KBD-§` / `IMC_*` untouched · `Saved/SaveGames/` untouched.

Timing change the verifier will see: the `HUD notice broadcast (TASK-1270)` line now logs when the hand binds (the Slate phase of the load frame, or the next frame), not in the load frame's world tick. Still exactly one per match start.

### L1.4 The test (suite +1 this loop ⇒ the pre-1270 baseline +3, per amendment (b))

**`Siegebound.Deck.IllegalDeckNoticeReachesALateListenerExactlyOnce`** (`SiegeDeckSlotsTest.cpp:1090–1206`). World-free, the house idiom (`NewObject<ASiegeGhostPawn>` in `SiegeGhostPawnTest.cpp:644`, `NewObject<UCardHandWidget>` in `SiegeCardHandKeyLabelTest.cpp:647`): two transient `ASiegePlayerController`s and three REAL `UCardHandWidget` listeners bound through the shipped `InitForController`. No world, no BeginPlay, no save slot, zero disk. Every row reads STATE (`SC-§104`):

- SELF-CHECK: the notice is `Deck 'deck1' has 51 cards — playing the default deck` (the verifier's save). PREMISE: a fresh controller is not pending, `OnCardRefused.IsBound()` is false, Deliver returns false.
- **(a) Raised with nobody listening** (BeginPlay's exact pair, Queue then Deliver): Deliver returns **false**, the notice is still pending, and the held text equals the notice. *(This is the loop-0 moment; a Deliver without the `IsBound` guard fails here.)*
- **(b) The hand subscribes AFTER:** `IsBound()` is true; the hand's `GetReceivedRefusalCount() == 1` and `GetLastReceivedRefusal()` equals the exact text; the controller is no longer pending and its held text is empty. *(A widget that does not flush after binding, or flushes before binding, fails here: count 0, still pending.)*
- **(c) Exactly once:** re-initialising the same hand leaves its count at 1; a second hand that binds later has count 0; a direct Deliver returns false; the counts stay 1 and 0. *(Without clear-on-delivery this fails with count 2.)*
- **(d) Reverse order:** a hand bound first with nothing held receives 0 (the legal-deck case); then Queue + Deliver returns **true**, that hand's count is 1 with the exact text, and nothing is pending.
- **(e) An empty FText is not a notice:** not pending, Deliver returns false, count unchanged.
- Expected Warnings, derived rather than guessed: `has no ASiegePlayerState` exactly 4 times (one per `InitForController` on a world-free controller — there is no PlayerState outside a world). `has no UDeckComponent` ×4 is expected ONLY if `FindComponentByClass<UDeckComponent>()` returns null world-free. The engine's `UActorComponent::PostInitProperties` → `AddOwnedComponent` and `AActor::PostInitProperties` → `ResetOwnedComponents` say the component will be found, so that expectation should stay unarmed; the test decides at runtime.

**Not assertable offline, named rather than skipped:** (1) the Blueprint half, i.e. `WBP_CardHand`'s handler writing `RefusalText` (graph O2; no C++ body). (2) The Log-verbosity line count. In UE 5.8 `UE_LOG` dispatches an `FLogRecord` (`StructuredLog.cpp` `BasicLogV` → `DispatchStaticLogRecord`), and `FAutomationTestMessageFilter::SerializeRecord` (`AutomationTest.cpp:305–323`) only matches expected messages on Warning/Error records, so a `Log`-level `AddExpectedMessagePlain` could never be counted. The receipt state above takes its place. **Verifier observables for the re-verify (branch (a), his 51-card deck1):** `get_widget_property_in_pie` `WBP_CardHand` → `TextBlock_21` `Text` == `Deck 'deck1' has 51 cards — playing the default deck` from the arena's first seconds (the text survives the 2 s collapse), and exactly ONE `HUD notice broadcast (TASK-1270)` line per match start.

### L1.5 Things QA should scrutinize

1. **New public surface** (all C++-only, no reflection): 4 controller methods, 2 hand getters, 3 private members. The hand's receipt fields exist for the test; they cost one integer and one string write per refusal (refusals are rare, not per-frame).
2. **`IsBound()` counts stale entries** (`ScriptDelegates.h:1171` is `InvocationList.Num() > 0`). This doesn't matter at either call site today: BeginPlay runs on a fresh controller (empty list), and InitForController calls Deliver right after its own `AddUniqueDynamic`. A future second binder of `OnCardRefused` would make BeginPlay's Deliver spend the notice on that binder instead of waiting for the hand; the header contract says so.
3. **No hand ever binds** (e.g. `WBP_HUD` is missing, or a server-side copy of a REMOTE client's controller runs this arm): the notice is held silently for the controller's lifetime and no `HUD notice broadcast` line is logged (loop 0 logged it into the void on such controllers). The `is not legal` Warning still fires. The held FText costs nothing worth noting.
4. **Delivery during `WBP_CardHand`'s `Construct`:** the BIE handler runs re-entrantly from inside `Construct` (via `InitForController`, its last node). `RefusalText` has already been created and assigned by `BuildHandTree` (O2), and the handler has its own `IsValid` guard. The `RetriggerableDelay` latent action is registered from there — the same path every refusal takes, just earlier.
5. **World-free `NewObject<ASiegePlayerController>` in a test** is new for this class (precedents: `ASiegeGhostPawn`, `ABuilding`, `ASiegePlayerState`, `UCardHandWidget`). Its constructor only creates the `DeckComponent` and `AssistantComponent` subobjects (no loads, no logs, no world), and none of the three classes overrides `PostInitProperties` or `BeginDestroy`. It first runs in build-master's suite.
6. **Source-scan tests re-checked against the new text:** `SiegeFogRefusalTest.cpp:717` (`OnCardRefusedMessage(Reason);` == 1 in `CardHandWidget.cpp`) is still 1. `SiegePlacementTest.cpp`'s card-bar zeros (`NativeOnMouseButtonDown`, `RequestDiscardAll`, `DiscardEntireHand` in `CardHandWidget.{h,cpp}`) are still 0. None of the controller-header needles (`WatchTower`, `EKeys::H`, `GetPositionalKey`, `PreventionSeconds`, `FogPrevent`, `WouldBeWindow`, `BrightSunWindow`, `GroupRadiusMax = 5000.f`, `static FText WholeSecondsText(float Seconds);`) were added.

### L1.6 `git status --porcelain` after the loop-1 edit (git root is one level up, `SC-§102`)

```
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp           <- loop 1 (new this loop)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.h             <- loop 1 (new this loop)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h             <- NOT mine: TASK-1271
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp        <- loop 0, unchanged this loop
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h          <- loop 0, unchanged this loop
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp    <- loop 0 + loop 1
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h      <- loop 1 (new this loop)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp <- loop 0 + loop 1
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1270-programmer.md                 <- this file
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1271-programmer.md                 <- NOT mine
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1289-buildmaster.md                <- NOT mine
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1270-verify.md                           <- the verifier's
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1271-verify.md                           <- NOT mine
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1287-report.md                           <- QA's
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1288-report.md                           <- NOT mine
```

Nothing staged. Loop-1 files written: `SiegePlayerController.cpp`, `SiegePlayerController.h`, `CardHandWidget.cpp`, `CardHandWidget.h`, `Tests/SiegeDeckSlotsTest.cpp`, this handoff, and my row's `status:` line.
