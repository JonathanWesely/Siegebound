# TASK-252 — D-SPELL-VISIBILITY deck tweak (FrostNova → player default, Lightning ×2 → Bot Defensive Economy)

**Agent:** gameplay-programmer · **Date:** 2026-07-22 (overnight window) · **Status:** ready-for-qa (QA-light: data + one constructor-array edit, no logic; orchestrator-proxied diff review per spec)

---

## DONOR CHOICES + REASONING (Jonathan: review these first)

### 1. Player default deck (cards.csv DeckCount): FrostNova 0 → 1, donor = Footman 12 → 11

- **Why FrostNova ×1 (not ×2):** the D-SPELL-VISIBILITY flag Jonathan ruled on proposed "FrostNova **+1** in the player default" verbatim; his "go ahead and implement the deck tweak you mentioned" endorses that exact shape. ×1 is the smallest balance perturbation for a morning review, and the deck cycles over a full match so a 1-of is reachable. Trivial to bump to 2 later if it shows up too rarely (MaxCopies 3 leaves headroom).
- **Why Footman is the donor:** it is the largest (12), cheapest (3-gold), most redundant stack in the deck — pure generic filler with no combo/synergy loss. −1 moves its draw share 24% → 22%. Every alternative is worse: Fireball/Cleric/Longbowman/Ogre are 2-of showcase cards (halving one is very noticeable, and cutting **Fireball** would *reduce* spell visibility — the opposite of this task's purpose); the 3-of mid-tiers (Knight/Miner/ArrowTower/MilitiaMob/Pikeman/Cavalry) are already lean; Wall 4 → 3 dents the only pathing-block utility.
- **Balance delta:** avg deck cost 5.08 → 5.14 (one 3-cost slot became a 6-cost slot). Footman stays the largest stack (11) and stays ≤ MaxCopies 12.

### 2. Bot "Defensive Economy" deck (C++ BotDecks[1]): Lightning 0 → 2, donor = Wall 10 → 8

- **Lightning ×2 into THIS deck** is the recorded M6 QA recommendation verbatim (spec default) — this closes the old M6 open checkpoint item ("bot decks spell-free"). ×2 = Lightning's MaxCopies cap, exactly met (IsDeckLegal checks `Running > MaxCopies`, so 2/2 passes).
- **Why Wall is the donor:** biggest (10), cheapest (4-gold) stack in the deck; zero damage output lost; single-donor keeps the diff minimal. The fortress identity survives intact — still 8 walls + all four tower types + full economy (Miner 4/DeepMine 2/Barracks 3 untouched). Alternatives all cut something the deck is *about*: ArrowTower −2 loses DPS, Knight −2 loses its only line unit, Miner −2 guts "full economy", Cleric/Barracks/CrystalTower −2 removes ⅔ of a 3-of, Longbowman 1 → 0 deletes a card type.
- **Thematic fit:** Lightning is the bot decision loop's rule-3b "tower-killer" — the defensive/economy bot getting the anti-siege tool is coherent.
- **Balance delta:** avg deck cost 6.86 → 7.02 (header comment updated). Sum stays 50 (8+8+6+4+4+4+3+3+3+2+2+2+1).

---

## Exact diffs (TASK-252 only)

### Docs/Data/cards.csv — exactly 2 cells (DeckCount column only)
- Footman row: `...Basic melee line unit,12,false...` → `...Basic melee line unit,11,false...`
- FrostNova row: `...castle unaffected (GDD 4),0,false...` → `...castle unaffected (GDD 4),1,false...`
- **TASK-248's SpellDelivery column preserved byte-exact** (header + trailing commas + `HeroLine` on Fireball/FrostNova untouched; verified via Import-Csv readback: FrostNova SpellDelivery still `HeroLine`). LF line endings preserved (string-replacement edit, no rewrite).

### Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp — 3 hunks, constructor only
1. M6 comment block (lines ~213–224): caps list gains `Lightning 2`; "bot-PLAYABLE types" sentence amended to include the rule-3 castable Spells (it was stale the moment a Spell entered a deck) + TASK-252 provenance.
2. `[1] DEFENSIVE ECONOMY` header: avg cost ~6.86 → ~7.02; `Entry(TEXT("Wall"), 10)` → `Entry(TEXT("Wall"), 8)` with donor note.
3. New `Entry(TEXT("Lightning"), 2)` inserted between DeepMine and Longbowman (count-descending order preserved).

NOTE for QA: the working-tree diff of this .cpp also shows a 4th hunk at line ~629 (rule-3b comment `400` → `700`) — that is **TASK-248's** already-reviewed stale-comment sweep, pre-existing before TASK-252 started. Not mine.

## Verification (read-only, no compile)

- cards.csv: DeckCount sum = **50** (scripted Import-Csv check); all DeckCount ≤ MaxCopies; Footman 11/12, FrostNova 1/3.
- Aggro Rush: sum **50**, untouched. Defensive Economy: sum **50**, 13 entries, Wall 8/10 cap, Lightning 2/2 cap.
- `UDeckLibrary::IsDeckLegal` trace: per-entry FindRow (Lightning row exists in DT_Cards **today**, MaxCopies 2), aggregate cap check (2 ≤ 2 passes — comparison is `>`), `TotalCount() == SiegeLegalDeckSize (50)`. Both decks pass. No CardType restriction in the validator — Spells are deck-legal.
- LogSiegeBot deck naming: `BeginPlay` logs `ChosenDeck.DeckName`; both names unchanged (`Bot Aggro Rush` / `Bot Defensive Economy`) — the grep-able deck-select line is intact. Rule 3b (`EvaluateDecisions` ~line 629) already handles Lightning-in-hand; it was dead only because no curated deck carried the card.
- Player path: `UDeckComponent::BuildAndShuffle` fallback builds `Row.DeckCount` copies per DT_Cards row → FrostNova ×1 enters the player default **after the DT_Cards reimport** (below).

## Lane ruling (recorded per dispatch)

`SiegeBotController.cpp` is **byte-identical between main (`b90157e`) and branch tip (`ec7a271`)** — blob `e48be2c` on both sides; the file is absent from the full main↔branch diffstat (only the **.h** diverges, which I did not touch). Same situation as TASK-248, even stronger (whole file identical, not just the edit region) → **the edit is lane-neutral**: it can ride the final overnight main data commit or the branch pre-W1 bounce; whichever side takes it, the other merges clean. No divergence ruling needed.

**Side observation (not my scope, for build-master's merge-gate notes):** main's .cpp blob already references `BotCastleSpawnOffset` while main's **.h** still declares `BotCenterlineSpawnX` — main tip likely does not compile standalone as-is. Pre-existing; unrelated to this edit.

## Dependencies (who owes what before this is live in play)

1. **Compile (bot half):** BotDecks is C++ constructor data → Lightning-in-bot-deck goes live at the **next compile = the pre-W1 build-master bounce** (the same bounce TASK-250's qa-passed compile already owes; no separate compile window exists tonight — the art pair holds the editor). Bot half needs ONLY the compile: IsDeckLegal passes against the CURRENT DT_Cards (Lightning row + MaxCopies 2 already live in the table).
2. **DT_Cards reimport (player half):** FrostNova ×1 / Footman 11 sit in cards.csv only until reimport. NOT done by me — editor is live and the art pair holds the next editor window; per spec the reimport folds into that window or the final overnight commit's editor slot. Until then the player default builds the old 50 (harmless; no PIE between reimport and window-end checks per spec).
3. **Commit:** rides the final overnight commit with explicit pathspecs (`Docs/Data/cards.csv` + `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`) — note both files also carry TASK-248's already-qa-passed deltas in the same working tree; a single pathspec commit carries 248+252 together (both reviewed).

## Files touched

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Docs\Data\cards.csv` (2 cells)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeBotController.cpp` (constructor data + comments)

---

## DT_Cards REIMPORT DONE (player half live) — appended by art-director, 2026-07-22 ~03:15 overnight window

Dependency §2 above is CLOSED. Executed in the art pair's editor window, AFTER all PIE verify runs (spec's "no PIE between reimport and window end" honored — zero PIE after the reimport).

- **Lane:** the proven python `AssetImportTask` reimport — UE official remote execution (re-enabled in-memory via the TASK-221 §2 one-call recipe; reverts on editor restart), payload `t252_reimport_dtcards.py` via the session `ue_exec.py`. `CSVImportFactory` + `automated_import_settings.import_row_struct=CardRow`, `replace_existing=True`, destination `/Game/Data/DT_Cards`.
- **Reference-safe:** post-import `load_asset` returned the SAME object (`same_object=True`) — in-place reimport, no delete+recreate; WBP_HUD/WBP_CardHand refs intact by construction.
- **Verified live (DataTableTools readback, all 28 rows):** FrostNova `DeckCount=1` ✓, Footman `DeckCount=11` ✓, **sum(DeckCount)=50** ✓, all counts ≤ MaxCopies ✓. Bonus: TASK-248's SpellDelivery column landed with it — `HeroLine` on Fireball+FrostNova, `Auto` (unset default) on the other 26 ✓. Lightning row intact (AoERadius 700, MaxCopies 2 — bot half unchanged, lives in C++).
- **Saved:** `/Game/Data/DT_Cards` via save_assets → `Content/Data/DT_Cards.uasset` now modified in the working tree, rides the final overnight commit with this task's pathspecs.

