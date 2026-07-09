# TASK-120 — M6 final assembly: deck-builder PIE verification + ONE M6 commit + m6-testable branch

**Agent:** build-master
**Date:** 2026-07-09
**HEAD at start:** 9a8a75f
**Commit:** `<M6 batch commit — see "The commit" below / Slack 🔧 thread / orchestrator report>`
**Branch cut:** `m6-testable` at that commit (no push)
**Result:** PASS — M6 committed. Machine-verified the whole non-interactive slice; the click-only + savegame-author items are WATCH (owed to Jonathan on an unlocked desktop), never faked.

---

## Desktop-lock mode: LOCKED
`LogonUI.exe` present on the Active console session (PID 30352) → the secure/lock desktop is up → **SendInput is blocked**. No live clicking. Compounding tooling limits discovered this session (they define the WATCH boundary):
- The Unreal-MCP build exposes **no UFUNCTION-invocation tool** (ObjectTools is get/set-properties only) and **no console/exec tool**.
- The Programmatic Python sandbox allows only `math/copy/json/re/time/datetime` + `execute_tool` — **no `import unreal`**.

Consequence: I cannot headlessly (a) drive the `WBP_DeckBuilder` grid, (b) call `UDeckBuilderWidget::SaveDeckAs/SetActiveDeck` to author a `SiegeDecks.sav`, or (c) issue the TASK-121 exec cheats. Everything reachable via PIE-BeginPlay, DataTable/property/graph readback, and Build.bat WAS machine-verified. The bot exercises the **same override BUILD path** the player's active-saved-deck would, purely at BeginPlay, so the deck-feed *mechanism* is proven live even though the player's SaveGame *source* variant is owed.

## M6 exit-criteria results

| Criterion | Result | Evidence |
|---|---|---|
| Compile clean, warnings-as-errors | **VERIFIED** | Build.bat → "Target is up to date / Result: Succeeded". Source byte-unchanged since TASK-117's 0-warn / 0-error / 0-C4458 build, so no re-bounce needed. |
| DT_Cards curated 50-card DeckCount (sum 50, caps) | **VERIFIED (live readback)** | DataTableTools: 28 rows, `sum(DeckCount)=50`, `0` cap violations. Dist: Footman12 Archer8 Knight3 Miner3 ArrowTower3 Wall4 MilitiaMob3 Pikeman3 Cavalry3 Longbowman2 Cleric2 Ogre2 Fireball2. |
| Deck-builder screen exists, reparented to `UDeckBuilderWidget` | **VERIFIED** | `get_parent(WBP_DeckBuilder)` = `/Script/GitClaudeUnrealTest.DeckBuilderWidget` (the TASK-111 silent-reparent crux held). `WBP_DeckCardTile` present (both committed). |
| AddCopy caps at MaxCopies; GetTotalCount/GetAverageCost track; IsCurrentDeckLegal true only at 50 | **WATCH (owed)** | No headless way to call the widget API on a locked desktop. Code is qa-passed — TASK-116 QA verified AddCopy data-driven cap, RemoveCopy floor, library-delegated legality/avg-cost in code. |
| SaveDeckAs writes SaveGame; **persistence across restart** (LoadGameFromSlot readback) | **WATCH (owed) — HEADLINE** | Cannot trigger `SaveDeckAs` headlessly (no click / no exec / no unreal-python). `USiegeDeckSaveGame` reflects live (SaveGame subclass; SavedDecks + ActiveDeckName + shared SlotName/UserIndex consts). TASK-113 QA verified the round-trip vs UE 5.8 engine source. Live save→relaunch→reload owed to Jonathan. |
| Active saved deck feeds the **player's** match (DeckComponent readback) | **WATCH (player source) / MECHANISM VERIFIED** | Player-from-SaveGame needs an authored `.sav` I can't create headlessly. The identical override build path is machine-verified via the bot ↓. |
| No legal saved deck → fallback to curated DeckCount default (byte-identical to TASK-114) | **VERIFIED (live PIE)** | Pristine PIE L_Arena: `SiegePlayerController_0: built a 50-card draw pile from 13 card rows (GDD §3.4)` — DeckCount default, **no** override line for the player (exactly the pre-M6 path). |
| Bot plays 1 of its 2 distinct curated decks, random pick logged on LogSiegeBot | **VERIFIED (live PIE + readback)** | LogSiegeBot: `chose curated deck 1 of 2 'Bot Defensive Economy' (50 cards, avg cost 6.86)`. CDO readback `BotDecks` = 2 distinct legal 50-card decks (Aggro Rush 9 entries / Defensive Economy 12 entries). |
| Override BUILD path (SetPendingDeckList → BuildAndShuffle from pending) | **VERIFIED (live PIE)** | `SiegeBotController_0: built a 50-card draw pile from the pending OVERRIDE deck 'Bot Defensive Economy' (12 entries) — DeckCount column bypassed`. Corroboration: the bot then played a **Longbowman** (a card only in that deck). |
| Main-menu button enabled + opens WBP_DeckBuilder; Play/Sandbox/Quit still bound | **VERIFIED (graph readback)** | WBP_MainMenu EventGraph: `SetIsEnabled …true`, label "Deck Builder", `OnClicked_Event_9 → RemoveFromParent(self) + CreateWidget(WBP_DeckBuilder_C) + AddToViewport`; Play→`StartMatch`, Sandbox→`StartSandboxMatch`, Quit→`QuitGame 0`. |
| Live 28-tile grid render/feel + click-through nav | **WATCH (owed to Jonathan)** | Locked desktop. Carries the TASK-118 M7-polish flags: saved-deck is a **text readout** (not clickable rows); **no ScrollBox** (eyeball 28-tile overflow); NameInput is an EditableText. TASK-119 flag: `CreateWidget.OwningPlayer` left null (wire `GetOwningPlayer` if PIE shows a focus/click issue). |
| TASK-121 cheats wired + shipping-safe | **VERIFIED (wiring); live exec WATCH** | Player-controller CDO `CheatClass = /Script/GitClaudeUnrealTest.SiegeCheatManager`; `USiegeCheatManager` reflects (CheatManager subclass, non-shipping-only by construction). Live `SummonTestUnit/ApplyTestDamage/AddTestGold` exec owed (no console/exec MCP tool). |
| StopPIE clean | **VERIFIED** | `IsPIERunning=false` after StopPIE. No SaveGame authored (Saved/SaveGames absent). No actors placed; PIE ran in its transient UEDPIE world only — L_Arena / L_MainMenu assets NOT dirtied. |

## SaveGame-persistence proof (what IS and ISN'T established)
- **Established live:** the persistence *reader* works — `USiegeDeckSaveGame` is a live SaveGame subclass with the persisted schema, and both consumers (player `ASiegePlayerController::BeginPlay`, bot) load/validate it null-safe (player's no-file path proven this session).
- **Established in code (qa-passed):** TASK-113 QA verified `SaveGameToSlot`/`LoadGameFromSlot("SiegeDecks", 0)` round-trips `TArray<FDeckList>` with no silent data loss vs UE 5.8 engine source.
- **OWED to Jonathan (headline, unlocked desktop):** author a named deck in the live screen → Save → quit & relaunch the editor → reopen the deck builder and confirm the deck is still listed (and/or set it active, start Play-vs-Bot, and see the player's draw pile built from it). Blocked here only by the locked-desktop + no-exec/no-unreal-python tooling, not by any code fault.

## Deck-feed proof (live PIE, one pristine L_Arena run)
```
LogGitClaudeUnrealTest: UDeckComponent on 'SiegeBotController_0': pending override deck 'Bot Defensive Economy' set (12 entries, 50 cards) — validated at the next BuildAndShuffle (M6 TASK-114).
LogGitClaudeUnrealTest: UDeckComponent on 'SiegeBotController_0': built a 50-card draw pile from the pending OVERRIDE deck 'Bot Defensive Economy' (12 entries) — DeckCount column bypassed (M6 TASK-114).
LogGitClaudeUnrealTest: UDeckComponent on 'SiegePlayerController_0': built a 50-card draw pile from 13 card rows (GDD §3.4).
LogSiegeBot: [Bot SiegeBotController_0] Deck select: chose curated deck 1 of 2 'Bot Defensive Economy' (50 cards, avg cost 6.86) — pushed as pending override.
LogSiegeBot: [Bot SiegeBotController_0] Rule 4 (Attack): played unit 'Longbowman' (cost 6) at centerline …
```
Player = DeckCount fallback (13 non-zero rows → 50); Bot = override from a random curated deck (bypasses DeckCount). Both draw piles are exactly 50. The override path that fed the bot is the same one that will feed the player from a saved active deck.

## Residue log
- **`Content/UI/WBP_CastleHealthBar.uasset` — boot-resave residue, EXCLUDED (left dirty & UNSTAGED).** LFS oid changed (`5e897f6…`→`e69e761…`) but size byte-identical (46171); its last intentional edit was M1 (`4f95730`), and no M5.5/M6 task touched it — a lazy editor re-serialization from opening/PIE'ing L_Arena. `git checkout` to revert it was **blocked by the editor asset lock** (the running editor holds it, referenced by the loaded L_Arena → "unable to unlink … Invalid argument"). It is therefore left dirty but was **never staged** (explicit-path staging, never `git add -A`), so it is **excluded from the M6 commit** — the material point. It will re-appear clean once the editor closes, or the orchestrator can revert it then.
- **No SaveGame residue** — `Saved/SaveGames/` never existed and I authored nothing (Saved/ is gitignored regardless).
- **No level residue** — placed zero verification actors; PIE used its transient world; L_Arena/L_MainMenu `.umap` not modified.
- **DT_Cards.uasset** dirty is legitimate (TASK-117's in-editor DeckCount reimport) and rides this commit; `cards.csv` DeckCount-only byte-diff already confirmed at TASK-117.
- Staging strategy: `git reset` → revert residue → stage the explicit M6 set + sweep `.claude/pipeline/**`; verified staged == intended before commit.

## The commit
ONE commit on `main` — the whole M6 batch:
- **Source/** (new: DeckTypes.h, DeckLibrary.{h,cpp}, SiegeDeckSaveGame.{h,cpp}, DeckBuilderWidget.{h,cpp}, SiegeCheatManager.{h,cpp}; modified: DeckComponent.{h,cpp}, SiegeBotController.{h,cpp}, SiegePlayerController.cpp)
- **Content/UI/** WBP_DeckBuilder.uasset + WBP_DeckCardTile.uasset + WBP_MainMenu.uasset
- **Docs/Data/cards.csv** + **Content/Data/DT_Cards.uasset**
- **.claude/pipeline/** TASKBOARD, CONVENTIONS, all swept M6 (+ trailing M5/M5.5) handoffs & qa reports
- Message: `TASK-113..121: M6 deck-builder meta — WBP_DeckBuilder, USaveGame named decks, curated default + 2 bot decks, debug cheats`

Then `git branch m6-testable <hash>` (milestone-branch-preservation). **No push** (not main, not the branch).

## Follow-ups surfaced to the orchestrator/manager (not fixed here)
1. **Headline SaveGame round-trip + full deck-builder UX are a human WATCH** — owed to Jonathan on an unlocked desktop (see the two WATCH sections). Not a code fault; a tooling/desktop-lock limit.
2. **TASK-118 M7-polish flags** ride forward: saved-deck text-readout (not clickable rows), no ScrollBox (28-tile overflow risk), EditableText name field.
3. **TASK-119 flag:** `CreateWidget.OwningPlayer` null on the menu→deck-builder open (and the symmetric Back) — trivial `GetOwningPlayer` wire if PIE shows a focus/click issue.
4. **TASK-114 JONATHAN CHECKPOINT ITEM (open):** the two bot decks are spell-free, so the M5 bot Fireball/Lightning rules (rule 3) can never fire in an M6 match — the bot-spell feature is unverifiable until a bot deck carries a spell. QA's reversible suggestion: add Lightning ×2 to Defensive Economy (drop Longbowman ×1 + Wall ×1 → still legal 50). Bot design = Jonathan's call.
5. **Cheat execs unexercised** — TASK-121 cheats are wired + shipping-safe but no headless console exists to fire them this session; first live use owed.
