# TASK-602 — [ACC-4] Deck lane profile scoping — programmer handoff

- **Task:** TASK-602 — the five enumerated deck-slot call sites route through the `USiegeAccountSubsystem` seam (ACC-§4)
- **Agent:** gameplay-programmer · 2026-08-16
- **Status:** ready-for-qa
- **Law applied:** ACC-§4 (seam law) · ACC-§7 registry block 2 (seam signatures, character-for-character) · SC-§14 (search-tool re-verification) · SC-§33 (trailing-default audit — see below)

## Files touched (the task's names block, nothing else)

1. `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp`
2. `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

No header changes. No new files. No editor/MCP/compile/git (QUIET-MODULE law respected).

## All 5 enumerated sites accounted for (verified by symbol, not offset — lines below are POST-edit)

| Enumerated site (manager pre-flight) | Function | What it is now |
|---|---|---|
| `DeckBuilderWidget.cpp:589` | `SaveDeckAs` → `SaveGameToSlot` | `DeckSlotName` local from `ResolveDeckSlotName(GetGameInstance())`, resolved at call time (now ~:618-619) |
| `DeckBuilderWidget.cpp:677` | `SetActiveDeck` → `SaveGameToSlot` | same shape (now ~:708-709) |
| `DeckBuilderWidget.cpp:744` | `LoadSaveGame` → `DoesSaveGameExist` | ONE resolve at function entry shared with the load below, so the exist-check and the load can never straddle a profile change (now ~:776-780) |
| `DeckBuilderWidget.cpp:752` | `LoadSaveGame` → `LoadGameFromSlot` | uses that same single resolve (now ~:788) |
| `SiegePlayerController.cpp:253` | `BeginPlay` match-side active-deck load | inline call-time resolve: `GetGameInstance()` → `GetSubsystem<USiegeAccountSubsystem>()` → `GetDeckSlotName()`, ternary fail-safe to `USiegeDeckSaveGame::SlotName` (now ~:260-266) |

## The seam mechanics

- **DeckBuilderWidget.cpp** gained ONE file-local anonymous-namespace helper (spec-sanctioned: "a small local helper per file is fine"): `FString ResolveDeckSlotName(const UGameInstance*)` — null GameInstance OR null subsystem ⇒ `USiegeDeckSaveGame::SlotName` (guest fail-safe, ACC-§4); else `GetDeckSlotName()`.
- **SiegePlayerController.cpp** has one site, so it resolves inline with three locals — no helper added, no new symbol at namespace scope.
- ⛔ **No cached slot member, no reload machinery** — every site resolves at call time (ACC-§4 deck-lane clause: `GameInstance` outlives `OpenLevel`, so a menu login is live at the controller's match-start load with zero extra plumbing).
- Registry signatures consumed, character-for-character per ACC-§7: `GetDeckSlotName() const` (called on const pointers — const per registry), `UGameInstance::GetSubsystem<USiegeAccountSubsystem>()`.
- New includes, both files: `Engine/GameInstance.h` (complete type for `GetSubsystem`) + `Siegebound/SiegeAccountSubsystem.h` (TASK-600's header, same-batch — the TASK-442 parallel-header precedent; the file may not exist until TASK-600 lands, which is the batch's stated compile order).

## ⚠️ SC-§14 RECONCILIATION FINDING — 2 sites the enumerated law did not count (reported, not silently absorbed)

The pre-edit re-grep of `SiegeDecks` + `USiegeDeckSaveGame::SlotName` at the artifact found the five enumerated slot-ARGUMENT sites **plus TWO log-string references** the enumeration missed, both inside `SaveDeckAs` (pre-edit `DeckBuilderWidget.cpp:593` and `:599`): the failure-warning and success-log lines print the slot name that was written. Leaving them on the static would make the log LIE about the destination slot whenever a profile is active. **Resolution:** both logs now print the same resolved `DeckSlotName` local used by the `SaveGameToSlot` call one line above — the minimal change that keeps the log truthful; in guest mode they print the identical `"SiegeDecks"` string as before (byte-identical guest output). No other unenumerated site exists — post-edit grep confirms the only remaining `"SiegeDecks"` literal is the definition `SiegeDeckSaveGame.cpp:7` (+ pre-existing doc comments), and the only remaining `::SlotName` reads are the two guest fail-safe branches ACC-§4 itself mandates.

## SC-§33 trailing-default audit

**No new trailing-defaulted parameter exists in this diff.** The only new callable, `ResolveDeckSlotName(const UGameInstance*)`, has one parameter and no default — SC-§33 structurally cannot fire. No existing signature was changed.

## Guest byte-identity argument (QA criterion, ACC-§4/spec (2))

- `USiegeDeckSaveGame::SlotName` / `::UserIndex` statics: **untouched, byte-identical** (`SiegeDeckSaveGame.h/.cpp` not in the diff).
- `UserIndex` is still passed directly at all five sites.
- Guest path (no profile active, or subsystem unresolvable): `GetDeckSlotName()` returns the bare constant per the ACC-§7 contract, and the null-subsystem branch returns the constant directly — same slot string, same user index, same call order, same log text. The only behavioral delta anywhere is the slot STRING when a profile is active, which is the entire point of the task.
- Nothing else in either file was touched (the `LoadOrCreateSaveGame` helper reaches the seam through `LoadSaveGame()` and needed no edit).

## M8 declaration (verbatim, ACC-§9)

Adds **no replicated property, no new replicated class, no new relevancy tier, no RPC.** All account state is CLIENT-LOCAL (`UGameInstanceSubsystem` + local `USaveGame`).

## For QA to scrutinize

- The ACC-§4 bare-slot-literal grep: expect matches ONLY at `SiegeDeckSaveGame.cpp:7`, doc comments, and tests.
- The two log-line changes in `SaveDeckAs` are the SC-§14 finding above — they are slot-name reference sites, inside an enumerated site's function, not "something else touched."
- `GetSubsystem<T>()` is called on a `const UGameInstance*` — `UGameInstance::GetSubsystem` is const in UE5; flag if the engine header disagrees at compile (TASK-606's gate will prove it).
- Cross-check against TASK-600's landed `SiegeAccountSubsystem.h`: `GetDeckSlotName()` must be `const` (it is, per the ACC-§7 registry).
