# TASK-670 — [DB3-2] THE 10-SLOT MODEL — programmer handoff (2026-08-27)

Status: **ready-for-qa** (TASK-673 reviews the wave; TASK-674 owns the compile slot AFTER 667 — QUIET-MODULE honored, **zero compile run**).

## 1. Diff summary (files touched — exactly the TASK-670 names block, nothing else)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeDeckSaveGame.h` | ADDS the four DECK-§8 statics (`NumFixedDeckSlots` · `MakeFixedDeckName` · `FindFixedDeckIndex` · `MigrateToFixedSlots`) + doc block + M8 line. **No existing field/static moved** (DECK-§1: the class is not restructured; `SlotName`/`UserIndex`/`SavedDecks`/`ActiveDeckName` byte-identical). |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeDeckSaveGame.cpp` | Implements the three functions. New include: `GitClaudeUnrealTest.h` (log category). `MakeFixedDeckName` is the ONE `"deck" + number` composition site (grep §6 proves it). |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` | ADDS `NativeConstruct` override + the four DECK-§8 UFUNCTIONs (public) + private `EditingDeckIndex` (Transient, INDEX_NONE) + private `PersistWorkingDeck()`. **NOT added: `DeckBar` / `RefreshDeckBarStates`** — the registry marks them TASK-671's half; 671 authors against my landed code. Existing API untouched. |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | Implements the five; wires `PersistWorkingDeck()` into the success paths of `AddCopy` / `RemoveCopy` / `LoadDefaultDeck` (refused no-ops return before any broadcast and save nothing). No other function body changed. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp` | **NEW** — 8 offline automation tests, `Siegebound.Deck.*` (see §7). No existing test file edited (665 owns existing-suite edits). |

**Fence proof (session-scoped):** my diff = the five files above ONLY. `git status` also shows `Castle.{h,cpp}` / `SiegeGameMode.{h,cpp}` / `SiegeBotController.{h,cpp}` / `Tests/SiegeCastleTransformTest.cpp` modified — those are **TASK-664/665's pre-existing in-flight ROT edits**, untouched by me (the TASK-667 dated rider already explains the deck-wave lines at reconcile). **`SiegePlayerController.{h,cpp}`: ZERO diff** — the wave's QA criterion; the match reader at `.cpp:241-290` consumes the ten decks through its existing name lookup unchanged (verified by read: `SavedDecks.FindByPredicate(DeckName == ActiveName)` + legality gate + curated fallback — post-migration `ActiveDeckName` is always a canonical fixed name, which that case-sensitive compare matches because migration canonicalizes to lowercase).

## 2. Migration truth table (DECK-§2 as implemented — `USiegeDeckSaveGame::MigrateToFixedSlots`)

| # | Input state | Result | Returns |
|---|---|---|---|
| 1 | Canonical fixed form: exactly 10 decks, slot i named `MakeFixedDeckName(i)` **case-sensitively**, `ActiveDeckName` one of the ten canonical names | untouched (the early-out writes nothing — byte-stable by construction) | **false** |
| 2 | Empty save (fresh boot / fresh account) | 10 EMPTY slots materialized (D1), `ActiveDeckName = "deck1"` (clause 6 fresh default) | true |
| 3 | Legacy deck already bearing a fixed name, any case (e.g. `"Deck3"`), slot unclaimed | keeps its slot (clause 1), name **canonicalized to lowercase** (see §5.3) | true |
| 4 | Previously-ACTIVE legacy deck, non-fixed name | lowest empty slot — normally deck1 (clause 2) | true |
| 5 | Remaining legacy decks | fill remaining empty slots in `SavedDecks` order (clause 3) | true |
| 6 | Legacy decks beyond ten | DROPPED; **one** Warning: `"USiegeDeckSaveGame::MigrateToFixedSlots: dropped %d legacy deck(s) beyond the %d fixed slots: <comma-joined names>"` (clause 4; ASCII-only literal per the module string law — the DECK-§2 cite lives in the comment) | true |
| 7 | `ActiveDeckName` empty / dangling (matches no deck) / its deck overflow-dropped | `ActiveDeckName = "deck1"` (clause 6 fallback) | true |
| 8 | Active legacy deck itself bears a fixed name | clause 1 wins (keeps that slot); `ActiveDeckName` = that slot's canonical name (clause 6) | true |
| 9 | 10 canonical decks in the WRONG array order | migrated (clause 1 re-slots each by name) — not fixed form, mutated | true |

Active-deck match = FIRST case-insensitive name hit (the shipped M6 idiom). Duplicate fixed names (impossible from the shipped overwrite-on-collision writers): first claimant keeps the slot, the duplicate falls to clauses 3/4 — documented in-code, untested (unreachable via shipped writers).

## 3. DECK-§8 conformance table (character-for-character)

| Registry line | Where | Status |
|---|---|---|
| `static constexpr int32 NumFixedDeckSlots = 10;` | SiegeDeckSaveGame.h | verbatim |
| `static FString MakeFixedDeckName(int32 SlotIndex);` (+ registry comment) | SiegeDeckSaveGame.h | verbatim incl. trailing `// 0-based -> "deck1".."deck10"` |
| `static int32   FindFixedDeckIndex(const FString& DeckName);` | SiegeDeckSaveGame.h | verbatim incl. `// case-insensitive; INDEX_NONE when not fixed` |
| `static bool    MigrateToFixedSlots(USiegeDeckSaveGame& Save);` | SiegeDeckSaveGame.h | verbatim incl. `// DECK-§2 mapping; true = mutated (caller persists)` |
| `virtual void NativeConstruct() override;` | DeckBuilderWidget.h | verbatim incl. the registry's trailing comment |
| `UFUNCTION(BlueprintCallable, Category="Siegebound|Deck") void  SelectDeckForEdit(int32 SlotIndex);` | DeckBuilderWidget.h | verbatim, one line, registry spacing + `Category="Siegebound|Deck"` no-space form |
| `UFUNCTION(BlueprintPure,     Category="Siegebound|Deck") int32 GetEditingDeckIndex() const;` | DeckBuilderWidget.h | verbatim |
| `UFUNCTION(BlueprintCallable, Category="Siegebound|Deck") void  SetActiveDeckBySlot(int32 SlotIndex);` | DeckBuilderWidget.h | verbatim |
| `UFUNCTION(BlueprintPure,     Category="Siegebound|Deck") int32 GetActiveDeckIndex() const;` | DeckBuilderWidget.h | verbatim |
| `DeckBar` UPROPERTY + `RefreshDeckBarStates()` | — | **deliberately absent — TASK-671's half** (board task split: "TASK-671: add the DeckBar BindWidgetOptional member + private RefreshDeckBarStates()") |
| `// private: void PersistWorkingDeck();` | DeckBuilderWidget.h private | implemented exactly as pinned: `SaveDeckAs(USiegeDeckSaveGame::MakeFixedDeckName(EditingDeckIndex))` |
| existing API byte-compatible | — | `SaveDeckAs`/`LoadDeck`/`GetSavedDeckNames`/`SetActiveDeck`/`AddCopy`/`RemoveCopy` signatures and bodies' existing behavior unchanged (funnel APPENDED to the three mutators' success paths only) |

## 4. The auto-save funnel + NativeConstruct ordering (DECK-§4)

- ONE funnel: `PersistWorkingDeck()` → `SaveDeckAs(MakeFixedDeckName(EditingDeckIndex))` — the **existing** path, so the ACC-§4 call-time slot seam (profile-scoped when logged in, guest otherwise, D4: guest gets the same ten slots by construction) is **inherited, never reimplemented**. No cached slot anywhere. Callers: successful `AddCopy`, successful `RemoveCopy`, `LoadDefaultDeck` (D9 — Reset persists the curated default to the EDITING slot immediately). Guard: `EditingDeckIndex` out of range ⇒ one Warning, nothing written.
- `NativeConstruct` order is **Super FIRST, deliberately**: `UUserWidget::NativeConstruct` fires the WBP's Event Construct, so the legacy graph-side seed (`LoadDefaultDeck` — the seed-then-bind idiom this WBP was built on) runs BEFORE the model init, cannot auto-save (the guard — `EditingDeckIndex` still INDEX_NONE), and is overwritten by `SelectDeckForEdit(GetActiveDeckIndex())`. Had the model init run before Super, the graph seed would clobber the active slot with the curated default under auto-save on EVERY builder open — data loss. This ordering is the defense.
- Migration persist: `NativeConstruct` persists iff `MigrateToFixedSlots` returned true, through the same call-time seam resolve (`ResolveDeckSlotName(GetGameInstance())`). A failed write is non-fatal by design — the migration is idempotent and re-runs next open.
- **Known cosmetic behavior, declared:** each successful add/remove now fires `OnDeckSlotCountChanged` + `OnDeckModelChanged` (existing) + one more `OnDeckModelChanged` from `SaveDeckAs` inside the funnel. A redundant getter re-read, never a wrong one.

## 5. Deviations / interpretations (SC-§15 — declared, not silent)

1. **`SelectDeckForEdit` performs NO disk write.** DECK-§4's parenthetical "(which persist selection state)" spans `SelectDeckForEdit / SetActiveDeckBySlot`; the ONLY persisted selection the model has is `ActiveDeckName` (DECK-§1 bars new save fields), which `SetActiveDeckBySlot` persists via the existing strict `SetActiveDeck`. The editing selection is transient and derived from active at open (DECK-§3 "opening the builder starts with the ACTIVE deck selected for editing"). A disk write in `SelectDeckForEdit` would have nothing to write. QA to ratify.
2. **No offline test drives the widget's funnel.** The dispatch asked for "the auto-save funnel firing per mutation" as a test; the BOARD spec's test list (authority) does not include it, and it is not safely testable offline: a `NewObject`'d widget has no GameInstance ⇒ the seam fail-safes to the player's REAL guest slot `"SiegeDecks"`, which no test may write, and adding a slot-override seam to the widget would deviate from DECK-§8. Closure: TASK-673 code inspection + TASK-674's live auto-save write verify (already in 674's spec).
3. **Clause-1 canonicalization:** a case-variant fixed-named legacy deck keeps its slot AND its name is rewritten to canonical lowercase. DECK-§2 only says "keep their slot", but the byte form is load-bearing (triple-duty law; the match reader `.cpp:272` and the cloud pull merge both compare case-sensitively). Pinned by `Siegebound.Deck.MigrationFixedNameRespected`.
4. **`MakeFixedDeckName` out-of-range ⇒ empty string** (silent; documented in-header, pinned by test). The registry comment defines only the 0-based happy path.
5. **Tests call `MigrateToFixedSlots` directly.** DECK-§2's "runs from `UDeckBuilderWidget::NativeConstruct` ONLY" is a shipping-surface rule (the point: the match path never migrates); the widget remains the only shipping caller.

## 6. QA greps (run 2026-08-27; re-run to verify)

- `grep -rn '"SiegeDecks' Source/ | grep -v Tests/` ⇒ 2 hits: `SiegeDeckSaveGame.cpp:9` (the ONE definition site) + `SiegeDeckSaveGame.h:13` (its own doc comment). Test-file scratch literal `"SiegeDecks_AutomationScratch"` sits in the ACC-§4 sanctioned tests bucket (and its non-digit suffix can never be a profile slot).
- `grep -rn 'TEXT("deck' Source/ --include=*.cpp --include=*.h | grep -v Tests/` ⇒ only `SiegeDeckSaveGame.cpp:25` composes a deck name; the `SiegeCloudSync.cpp` hits are the `decks` table / `deck_name` column keys, not name composition. DECK-§1's one-composer law holds.
- `git diff --name-only` on `SiegePlayerController.{h,cpp}` ⇒ empty (zero diff).
- **Trailing-default law (SC-§33): nothing owed** — no defaulted parameter was added to any existing function; every addition is a brand-new function.

## 7. NEW test file — declared suite delta for TASK-674

`Tests/SiegeDeckSlotsTest.cpp` adds **8** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, all offline, ZERO network, only disk touch = the scratch slot (janitored in+out):

1. `Siegebound.Deck.FixedNameContract` — ten names byte-exact, index round trip, case-insensitive inverse, 10 non-fixed rejections, out-of-range composer contract.
2. `Siegebound.Deck.MigrationFreshSave` — empty save ⇒ 10 empty slots + active "deck1".
3. `Siegebound.Deck.MigrationLegacyMapping` — active→deck1, others in order with content intact, ActiveDeckName rewritten; + dangling-active fallback sub-case.
4. `Siegebound.Deck.MigrationFixedNameRespected` — legacy "Deck3" keeps slot 2 (canonicalized "deck3"); active fills deck1; next fills deck2.
5. `Siegebound.Deck.MigrationOverflow` — 12 legacy → 10 survive, per-slot mapping asserted, the drop Warning REQUIRED exactly once (`AddExpectedMessagePlain`, the SiegeAssistantSelectionTest idiom).
6. `Siegebound.Deck.MigrationIdempotence` — second call false + deep-state byte-stability; hand-built canonical save false on FIRST call, active choice ("deck7") untouched.
7. `Siegebound.Deck.EmptySlotIllegal` — empty slot TotalCount 0; `UDeckLibrary::IsDeckLegal` false with null table AND with a real empty `FCardRow` table (the 0≠50 clause the D5 match fallback rides — no new code enforces D5, the shipped TASK-114 fallback does).
8. `Siegebound.Deck.ScratchSlotRoundTrip` — full ten-slot save + deck4 content + active "deck4" through `SaveGameToSlot`/`LoadGameFromSlot` on the scratch slot.

**Suite expectation for TASK-674: 667's landed baseline (declared 128) + 8 = 136 total, of which `Siegebound.Deck.*` = 8.** If 667 lands a different baseline, the delta (+8) is the invariant.

## 8. THE SYNC-LANE VERDICT (read raw: `SiegeCloudSync.cpp` push 604-697, pull 390-504)

**NO hidden single-deck assumption — zero sync-code change needed, DECK-§1's letter holds.**

- **Push** (`StartPushPhase` :620-637): iterates `DeckSave->SavedDecks` generically — one `decks?on_conflict=user_id,deck_name` upsert per named deck. Post-migration all ten slots carry names, so **ten rows flow through unchanged** (empty slots push as 0-card payloads — the "10 small rows, free at this scale" DECK-§1 posture). The empty-`DeckName` skip guard can never fire post-migration.
- **Pull** (`HandleDecksFetched` :462-483): merges by CASE-SENSITIVE name, overwrite-on-collision, never removes a local deck, never touches `ActiveDeckName` (D3 exactly — active-deck stays local-only; no `0002` migration, no schema change anywhere in this diff).
- **Declared OBSERVATION (not fixed — law-conformant interaction, flagged for TASK-674's live verify + Jonathan's awareness in the rollout window):** a pre-670 CLOUD row with a non-fixed or case-variant `deck_name` (e.g. `"War Deck"`, `"Deck1"`) pulls in as an ADDITIONAL local entry (case-sensitive merge finds no canonical match), leaving the local save temporarily non-canonical. The NEXT builder open re-runs the idempotent migration, which re-slots it per clauses 1/3 — or, when all ten slots hold content, DROPS it with the clause-4 logged list. That drop is the law's own overflow rule operating across devices; if Jonathan wants pre-670 cloud decks preferred over local empties, that is a rider, not this task.

## 9. M8 DECLARATION (verbatim, per touched file — also in both edited headers + the test header)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All of it is client-local `USaveGame` state and menu-widget model logic.

## 10. QA scrutiny list (TASK-673)

1. Compile-risk sites (⛔ no compile was run — QUIET-MODULE, 674 owns the slot): `UDataTable::RowStruct` direct assignment in test 7 (public UPROPERTY — defensive only, the empty-deck path never calls `FindRow`); `AddExpectedMessagePlain` 4-arg form (cloned from `SiegeAssistantSelectionTest.cpp:735`); `TestEqualSensitive`/`TestNotEqual` members (cloned from `SiegeAccountTest.cpp`); first-ever `NativeConstruct` override on `UDeckBuilderWidget` (UUserWidget's is virtual — standard).
2. The Super-first `NativeConstruct` ordering argument (§4) — please rule on it explicitly; it is the load-bearing defense against the legacy graph seed.
3. §5.1 (`SelectDeckForEdit` writes nothing) and §5.3 (canonicalization) interpretations.
4. The funnel's placement AFTER the existing broadcasts (mutation → broadcasts → persist, same call frame) vs "persists IMMEDIATELY".
5. Migration's fixed-form gate uses CASE-SENSITIVE canonical compare while placement uses case-insensitive matching — intended asymmetry (a case-variant save must migrate so its bytes canonicalize).

## 11. Cross-task FINDING for TASK-669/672 (declared here, routed via orchestrator)

The WBP's Event Construct seed call (`LoadDefaultDeck`, the documented seed-then-bind idiom) is **redundant post-670** — `NativeConstruct` now seeds the model from the active slot — and produces one spurious funnel Warning per builder open (harmless by the §4 ordering, but noisy and misleading in logs). **Recommendation: TASK-672 also cuts the Event-Construct seed node**, per TASK-669's measured record (it is NOT currently in 672's stated cut list). Content-side; not touched by me.
