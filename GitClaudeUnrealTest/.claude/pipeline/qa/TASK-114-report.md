# QA Report — TASK-114
Verdict: **PASS**

Wire decks into matches (C++, files only): `UDeckComponent::SetPendingDeckList` + IsDeckLegal-gated `BuildAndShuffle` override, player loads active saved deck, bot picks 1 of 2 curated decks. Pre-compile review; rides the TASK-117 batch compile. Files reviewed in full: DeckComponent.h/.cpp, SiegePlayerController.cpp, SiegeBotController.h/.cpp. Cross-read: DeckLibrary.h/.cpp (IsDeckLegal / GetDeckAverageCost), SiegeDeckSaveGame.h, DeckTypes.h, cards.csv.

Blockers: **0** · Warnings: **1** · Nits: **2**

## Findings

### BLOCKERS
- None.

### WARN
- [WARN] SiegeBotController.cpp:230-262 (both `BotDecks` defaults) — **DESIGN, not a code defect: both bot decks are spell-free, so the M5 bot rule-3 (Fireball/Lightning, TASK-102) branch can never fire in normal M6 play.** The code path is correct and null-safe either way; this is a content/coverage trade-off that must go to Jonathan as a checkpoint item. QA recommendation below. Non-blocking.

### NIT
- [NIT] SiegePlayerController.cpp:106-107 — the active-deck load uses `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(...))` with a null-check but **no `UGameplayStatics::DoesSaveGameExist` pre-guard**. This is functionally null-safe (verified against UE 5.8 engine source: `LoadGameFromSlot` returns `nullptr` when the slot is absent — the `ISaveGameSystem::LoadGame` false path — never a crash), so the fallback chain is correct. A `DoesSaveGameExist` pre-guard would only suppress the engine's benign "unable to read slot" log line on first run. Cosmetic; leave as-is or add the guard at TASK-118/120 polish. NOT a blocker.
- [NIT] SiegeBotController — the random deck pick happens at `BeginPlay` only; on **Play Again** `ResetBot()` → `ResetDeck()` → `BuildAndShuffle()` reuses the *persisted* pending deck, so the bot keeps the SAME deck across a rematch (it re-randomizes only on a fresh controller spawn from a new match). This is CORRECT per the persistence law ("same match keeps the same deck") and symmetric with the player keeping their active deck — noted only so the "variety across matches" rationale (ruling 4) is understood to mean *distinct match-starts*, not Play-Again rematches.

## Mandatory scans

**(a) Inherited-reflected-member shadow scan (C4457/58/59): CLEAN.**
- DeckComponent.cpp new identifiers: `CardTable`, `PendingLegalityReason`, `Entry`, `Copy`, `Deck` (param). None shadow a `UActorComponent` inherited reflected member. `CardTable` (local) ≠ `CardTableAsset` (member) — distinct identifier, no member shadow.
- SiegePlayerController.cpp new identifiers: `DeckSave`, `ActiveName`, `ActiveDeck`, `CardTable`, `LegalityReason`, `Candidate`. None shadow `APlayerController`/`AController` reflected members (`PlayerState`/`Pawn`/`Player`/`Instigator`/`Controller`…).
- SiegeBotController.cpp new identifiers: `Entry` (lambda), `Result`, `InCardID`, `InCount`, `AggroDeck`, `FortressDeck`, `DeckIndex`, `ChosenDeck`, `CardTable`, `LegalityReason`. None shadow `AAIController`/`AController` reflected members.

**(b) Complete-type-include scan (C2027/C2227): CLEAN.**
- DeckComponent.h: by-value member `FDeckList PendingDeckList` ⇒ `Siegebound/DeckTypes.h` included (line 8). ✓
- DeckComponent.cpp: `UDeckLibrary::IsDeckLegal` ⇒ `DeckLibrary.h` (line 8); `CardTable->ForeachRow<FCardRow>` ⇒ `Engine/DataTable.h` (line 5) + `CardRow.h` (line 7); `FDeckCardEntry`/`FDeckList` complete via header→DeckTypes.h. ✓
- SiegePlayerController.cpp: `UGameplayStatics::LoadGameFromSlot` ⇒ `Kismet/GameplayStatics.h` (line 21); `USiegeDeckSaveGame` deref/`Cast`/`::SlotName`/`::UserIndex` ⇒ `SiegeDeckSaveGame.h` (line 32); `UDeckLibrary` ⇒ `DeckLibrary.h` (line 29); `FDeckList` complete transitively. `SiegeCheatManager.h` (line 31) still present. ✓
- SiegeBotController.h: `TArray<FDeckList> BotDecks` ⇒ `DeckTypes.h` (line 9). ✓
- SiegeBotController.cpp: `UDeckLibrary::IsDeckLegal`/`GetDeckAverageCost` ⇒ `DeckLibrary.h` (line 16); `FDeckList`/`FDeckCardEntry` complete via header; `UDataTable*` passed by pointer only (Engine/DataTable.h line 5). ✓

No deprecated/removed UE 5.8 APIs. `FMath::RandRange`, `UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex)`, `FindByPredicate`, `TSoftObjectPtr::LoadSynchronous`, `FindRow`/`ForeachRow` are all current.

## Backward-compat (THE CRUX) — traced, byte-identical

`UDeckComponent::BuildAndShuffle()` (DeckComponent.cpp:42) gate is:
`if (bHasPendingDeckList && UDeckLibrary::IsDeckLegal(CardTable, PendingDeckList, PendingLegalityReason))`
- When `bHasPendingDeckList == false`: the `&&` **short-circuits** — `IsDeckLegal` is never called — control goes to `else`; the inner `if (bHasPendingDeckList)` warning is skipped; the exact DeckCount build runs (`if (CardTable)` reusing the once-loaded local, then the identical `ForeachRow<FCardRow>` loop, the identical `!= ExpectedDeckSize` error branch, and the identical missing-table log-and-empty branch). Verified: the unset path is behaviorally identical to the pre-M6 code — the only source change is that `const UDataTable* CardTable = CardTableAsset.LoadSynchronous()` is hoisted once (line 31) and the fallback tests the pre-loaded local. Same load, same result.
- Illegal-but-set path degrades correctly: warning with the validator's reason (line 65), then the identical DeckCount build. Never bricks the deck.
- Missing table: `IsDeckLegal(nullptr, …)` returns false (DeckLibrary.cpp:10-15), falls to `else`, and the DeckCount branch hits the missing-table log-and-empty path — same as today.

**Persistence across ResetDeck()/Play Again:** `ResetDeck()` (line 182) calls `BuildAndShuffle()`; neither clears `bHasPendingDeckList`/`PendingDeckList`. `SetPendingDeckList` (line 191) is the ONLY writer. Confirmed — the same match keeps its deck.

## Player active-deck load (SiegePlayerController::BeginPlay) — verified

- Runs inside the existing `if (DeckComponent)` block, BEFORE `DeckComponent->BuildAndShuffle()` (line 143). ✓
- `Cast<USiegeDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex))` — shared statics, **no re-literals**; cast is null-checked (TASK-113 carry-forward honored). ✓
- Fallback chain all leaves the component unset → curated DeckCount default: no save file (silent), empty `ActiveDeckName` (silent), name-not-found (Warning), found-but-`!IsDeckLegal` (Warning). `SetPendingDeckList` is called ONLY on the found+legal branch. ✓
- The `FDeckList` is copied by value into the component (`SetPendingDeckList` takes `const FDeckList&` and does `PendingDeckList = Deck`), so no reference into the transient SaveGame is retained. ✓
- TASK-121 preservation (mandatory): `#include "Siegebound/SiegeCheatManager.h"` present at line 31; `CheatClass = USiegeCheatManager::StaticClass();` present at line 50 in the constructor (BeginPlay-only edits; constructor untouched). **CONFIRMED intact.** ✓

## TASK-116 carry-forward — ESearchCase ruling

`ActiveDeck = DeckSave->SavedDecks.FindByPredicate([&ActiveName](const FDeckList& Candidate){ return Candidate.DeckName == ActiveName; })` (SiegePlayerController.cpp:112-113).
**Ruling: SAFE / ACCEPTED — case-insensitive as required, on two independent grounds.** (1) `FString::operator==` in UE resolves to `Equals(Rhs, ESearchCase::IgnoreCase)`, so the comparison is ALREADY effectively `IgnoreCase` — an explicit `ESearchCase::IgnoreCase` would be redundant, not additive. (2) TASK-116 stores the canonical `DeckName` when it writes `ActiveDeckName` (via `SetActiveDeck`/`SaveDeckAs`), so the names match character-for-character regardless. No change required.

## Bot decks — independently re-summed against DT_Cards / cards.csv

Card caps/costs read from `Docs/Data/cards.csv`. `IsDeckLegal` caps on the per-CardID aggregate with `Running > MaxCopies` ⇒ **Count == MaxCopies passes** (inclusive) — several entries below sit exactly at the cap and are legal.

**BotDecks[0] "Bot Aggro Rush"** — recomputed:
| CardID | Count | MaxCopies | ≤cap | Cost | Cost×Count |
|--------|------:|----------:|:----:|-----:|-----------:|
| Footman | 12 | 12 | ✓ | 3 | 36 |
| MilitiaMob | 6 | 6 | ✓ | 5 | 30 |
| Pikeman | 6 | 6 | ✓ | 5 | 30 |
| Knight | 6 | 6 | ✓ | 6 | 36 |
| Archer | 6 | 10 | ✓ | 4 | 24 |
| Cavalry | 4 | 4 | ✓ | 7 | 28 |
| Sapper | 4 | 4 | ✓ | 5 | 20 |
| Wall | 4 | 10 | ✓ | 4 | 16 |
| Miner | 2 | 4 | ✓ | 8 | 16 |
| **Sum** | **50** | | | | **236 → avg 4.72** |

Sum = 50 EXACTLY ✓; every Count ≤ MaxCopies ✓; avg 236/50 = 4.72 ✓. **LEGAL.**

**BotDecks[1] "Bot Defensive Economy"** — recomputed:
| CardID | Count | MaxCopies | ≤cap | Cost | Cost×Count |
|--------|------:|----------:|:----:|-----:|-----------:|
| Wall | 10 | 10 | ✓ | 4 | 40 |
| ArrowTower | 8 | 8 | ✓ | 5 | 40 |
| Knight | 6 | 6 | ✓ | 6 | 36 |
| BombTower | 4 | 4 | ✓ | 8 | 32 |
| BallistaTower | 4 | 4 | ✓ | 7 | 28 |
| Miner | 4 | 4 | ✓ | 8 | 32 |
| Barracks | 3 | 3 | ✓ | 10 | 30 |
| CrystalTower | 3 | 3 | ✓ | 9 | 27 |
| Cleric | 3 | 3 | ✓ | 6 | 18 |
| Ogre | 2 | 2 | ✓ | 12 | 24 |
| DeepMine | 2 | 2 | ✓ | 15 | 30 |
| Longbowman | 1 | 4 | ✓ | 6 | 6 |
| **Sum** | **50** | | | | **343 → avg 6.86** |

Sum = 50 EXACTLY ✓; every Count ≤ MaxCopies ✓; avg 343/50 = 6.86 ✓. **LEGAL.**

**Distinctness:** the two decks are genuinely distinct (aggro unit-rush vs tower/wall/economy fortress) and both differ from the TASK-115 player DeckCount default (Footman 12 / Archer 8 / Knight 3 / Miner 3 / ArrowTower 3 / Wall 4 / MilitiaMob 3 / Pikeman 3 / Cavalry 3 / Longbowman 2 / Cleric 2 / Ogre 2 / Fireball 2, sum 50 — independently re-summed = 50). ✓

**Selection + logging:** `FMath::RandRange(0, BotDecks.Num()-1)` under a `BotDecks.Num() > 0` guard (SiegeBotController.cpp:283-285), re-validated via `IsDeckLegal` at pick time, pushed via `SetPendingDeckList`, with exactly ONE grep-able `LogSiegeBot` line per BeginPlay (chosen / illegal-fallback / no-decks — all three variants log one line). Missing/illegal ⇒ DeckCount fallback. ✓

## Rulings on the 5 flagged decisions (handoff)

1. **`FMath::RandRange(0, BotDecks.Num()-1)` + `Num()>0` guard, instead of literal `RandRange(0,1)` — ACCEPTED.** With the 2 defaults this is identical (0/1 inclusive) and the distribution is unchanged. Hardening the upper bound to `Num()-1` plus the `>0` guard is the correct null-safe house rule given `BotDecks` is EditDefaultsOnly (BP-editable to any size). Strictly safer than the literal; spec intent ("random of the configured decks") preserved.

2. **Both bot decks contain only bot-playable types (Unit/Building/Economy), no spells — ACCEPTED AS CODE; escalated as a design item (see WARN + recommendation).** The composition is legal, distinct, and the fallback is correct. The consequence (rule-3 spell branch dormant) is a content call, not a code defect.

3. **New load/pick logic sits inside the existing `if (DeckComponent)` block, no second null-check — ACCEPTED.** The `BuildAndShuffle()` call and its `else` are byte-identical to before; minimal, review-friendly diff; the pre-existing non-null guard correctly covers the new code. No double guard needed.

4. **`PendingDeckList` is a plain (non-UPROPERTY) member — ACCEPTED.** `FDeckList` holds only `FName`/`FString`/`int32` (via `FDeckCardEntry`) — zero UObject references — so it needs no GC tracking and no reflection/serialization. Transient runtime match-start state; a UPROPERTY would be pointless overhead. No GC hazard. Correct.

5. **Player-side "not found"/"illegal" = Warning; no-save-file / empty-ActiveDeckName = silent — ACCEPTED.** Sound logging discipline: the common default-fallback cases stay quiet, while a set-but-broken active deck surfaces a diagnosable Warning (not player-facing). Matches the house pattern.

## Bot-deck-spell recommendation (Jonathan checkpoint item)

Both curated bot decks are spell-free, so the shipped M5 bot rule-3 (TASK-102: Fireball at a clustered push / Lightning at a defended tower) **never executes in M6 matches** — a real, shipped feature gets zero live coverage. The handoff's rationale (cleaner/more efficient bot play; no cycle-waste) is legitimate, and note the bot's castable Fireball/Lightning are HELD-for-target rather than cycled, so a spell in the deck would not thrash rule 5.

**QA recommendation: add one castable spell to the Defensive Economy deck** so the feature is exercised. The handoff's proposed 1-swap is legal and I verified the math: drop `Longbowman 1` + `Wall 1` (→9) and add `Lightning 2` (MaxCopies 2 ✓) → 50 − 1 − 1 + 2 = 50, all caps respected. Lightning suits the control/fortress identity. This is a low-cost, reversible content edit (constructor default, BP-tunable). Because it is a design/content decision, it does NOT gate this task — **flag it to Jonathan at the M6 checkpoint**; either resolution ships correctly.

## Notes for build-master (PASS)
- TASK-114 rides the TASK-117 batch compile (with TASK-113/116/121 + the TASK-115 DT_Cards reimport). No editor/MCP work in this task.
- Warnings-as-errors: both mandatory scans clean; no deprecated APIs; complete-type includes verified. Expect a clean compile of the 5 files.
- Bot deck legality is enforced at runtime via `IsDeckLegal`; if a future BP edits `BotDecks` illegal, the bot degrades to the DeckCount default (logged on LogSiegeBot) — no crash.
- Carry the **bot-deck-spell recommendation** into the M6 checkpoint report for Jonathan (design call, above).
- The TASK-115 carry-forward is unchanged and still owed at TASK-117: git-diff byte-check that cards.csv changed ONLY the DeckCount column + live DT_Cards re-verify before the M6 commit.

QA does not edit code, the engine, Git, or (per this dispatch) TASKBOARD.md. Board status flip to `qa-passed` is relayed to the orchestrator.
