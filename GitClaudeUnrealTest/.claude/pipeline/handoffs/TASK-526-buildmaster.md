# TASK-526 — build-master handoff (batch ASSISTANT-EXCLUDE, TASK-516..524)

**Verdict: ✅ GREEN — `Result: Succeeded`, 0 errors / 0 warnings, 53/53 tests pass, committed.**
**Date:** 2026-08-05 · **Agent:** build-master · **Gate:** `qa/TASK-525.md` (PASS — 0 BLOCKER / 7 WARN / 11 NIT)
⛔ **NOTHING WAS PUSHED.**

---

## 1. Git pre-flight — RE-RUN FIRST-HAND (`SC-§9`), and it AGREED with the board

Both readings taken by me, before anything else, and again immediately before staging.

| check | command | observed |
|---|---|---|
| working tree | `git status --porcelain` | 18 modified + 11 untracked — **all of them this batch's own work**, nothing foreign |
| ahead/behind | `git rev-list --left-right --count origin/main...main` | `0	2` ⇒ **`main` 2 AHEAD of `origin/main`, 0 behind, UNPUSHED** |
| HEAD | `git log --oneline -3` | `f0b21b6` · `97bd887` · `ddc6115` |

⇒ The two unpushed commits are the KEYBOARD-LAYOUT pair `97bd887` + `f0b21b6`, exactly as the board asserted. **No Jonathan self-commit intervened since dispatch.** ⛔ The push remains his; standing law honoured.

## 2. 🔒 Seal check — the criterion QA could not discharge (W-8)

`git status --porcelain Docs/Data/` — run by me, twice (before the compile and again before staging):

```
 M GitClaudeUnrealTest/Docs/Data/assistant_eval_dev.csv
```

⇒ ✅ **ONE file modified, and it is the dev corpus.** `assistant_eval_holdout2.csv` is **byte-untouched and SEALED** (`CONVENTIONS.md:1417`); the other holdout likewise shows no modification. **W-8 is DISCHARGED ON A GIT READING, first-hand.** The seal was never opened.

## 3. Compile

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development \
  -project=".../GitClaudeUnrealTest.uproject" -waitmutex
```

| field | value |
|---|---|
| **verdict (parsed from the log, NOT `$LASTEXITCODE`)** | **`Result: Succeeded`** |
| errors | **0** |
| warnings | **0** |
| UBT total execution time | **20.29 s** (UBA executor 16.29 s; wall-clock 20.74 s) |
| actions | 19 |
| relink | ✅ **both link steps ran** — `[17/19] Link UnrealEditor-GitClaudeUnrealTest.lib` and `[18/19] Link UnrealEditor-GitClaudeUnrealTest.dll`, then `WriteMetadata GitClaudeUnrealTestEditor.target` |

**Baseline comparison:** TASK-514 was `Result: Succeeded`, 0/0, 19.64 s. **This gate: `Result: Succeeded`, 0/0, 20.29 s.** No regression.

⛔ **`$LASTEXITCODE` was 0 — and that is worth nothing** (Build.bat returns 0 on a failed build). The verdict above is parsed from the `Result:` line. There is exactly one such line and it says `Succeeded`.

**Environment faults that did NOT occur:** no ~2 s `0x800711C7` death ⇒ **Smart App Control is not blocking**. No Live Coding mutex contention — **the editor was already closed when I arrived** (`Get-Process UnrealEditor*` ⇒ none). ⛔ **I did not close it, did not force-kill anything, and drove no unprompted close.**

**The compile watch list, item by item:**
- ⭐ **`NativeOnPreviewKeyDown` — the project's FIRST Slate key override — COMPILED CLEAN, zero warnings.** `FReply` / `FKeyEvent` / `FGeometry` all resolved; the includes TASK-519 added (`InputCoreTypes.h` at `SiegeAssistantConsoleWidget.h:7-12`) were sufficient. **No signature mismatch against `UUserWidget`.** The headline feature builds.
- `SiegeAssistantValidateSelection`'s **trailing-defaulted 4th parameter** compiled with **both** call shapes live — `SiegeAssistantComponent.cpp:987` (4-arg) and the six `Tests/SiegeAssistantGrammarTest.cpp` sites. The default is declared in the header only and is **not** repeated in the `.cpp`; QA's item 4 was correct.
- `FSiegeAssistantCommand`'s sixth field and the grammar's `except` rule: clean.
- ✅ ⛔ **NO `Build.cs` CHANGE WAS NEEDED OR MADE.** QA criterion 13 predicted this and it held.
- `Tests/SiegeAssistantSelectionTest.cpp` — QA's #1 predicted failure site, ~1800 lines of reflection + `TStrongObjectPtr` + subsystem construction — **compiled clean.**
- ⛔ **No foreign diagnostic.** `Plugins/SiegeLlama/` was already up to date and contributed **zero** actions; nothing to route to the FINE-TUNE board. **The quiet-module premise held.**
- ⛔ **I authored NO compile fix**, so `SC-§27`'s diff-scoped-verdict obligation never triggered.

## 4. Tests — 53/53, ZERO failures

PowerShell (⛔ **not Git Bash** — MSYS mangles the leading-slash object path), TASK-470's proven recipe **with the mandatory `-ini:` override**:

```
UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Siegebound" -unattended -nopause -nullrhi \
  -testexit="Automation Test Queue Empty" \
  "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry"
```

`LogAutomationCommandLine: ...Automation Test Queue Empty **53 tests performed.**` · duration **21.30 s** · **`Success = 53`, non-Success = 0.**

**Expected 40 + 13 = 53. Got 53.** ⇒ ⛔ **No test failed to register** (the count-below-53 hazard QA named did not occur).

🔒 **`L_Arena` NEVER OPENED, NEVER SAVED** — SHA256 `B3DBC5D9…F8268` **identical before and after** the run. The `-ini:` override did its job; the startup map was `/Engine/Maps/Entry.Entry`.

### 4a. ⭐ THE TWO SORCERER INSTRUMENTS — reported individually, by name, as required

These are the **only** instruments in the project that can see Jonathan's Sorcerer defect. The eval corpus structurally cannot assert it (its `t0` fixture prints all 13 kinds unconditionally, so the collapse never happens on that lane).

| test | result |
|---|---|
| ⭐ **`Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds`** | ✅ **PASS** |
| ⭐ **`Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol`** | ✅ **PASS** |

⭐ **AND THEY DID NOT PASS VACUOUSLY — the collapse genuinely fired against the SHIPPED builder, and the log proves it.** `CollapseNamesTheHiddenKinds` printed:

> `Forced collapse: 1 kind(s) printed in full, 12 named on `other_kinds:`. Zone C is 863 chars. Collapse line: `other_kinds: archer, knight, miner, militiamob, pikeman, sapper, cavalry, longbowman, cleric, ogre, wizard, **sorcerer** (108 units)`.`

⇒ **The Sorcerer's symbol reaches the model on a collapsed board.** That is the repair, observed at the artifact rather than asserted. `ShrinkLoopNeverHidesASymbol`'s 122-rung sweep additionally drove **100** distinct `Snapshot roster TRUNCATED` events through the real `USiegeAssistantSnapshot::BuildZoneC` — the shrink loop was exercised hard and never dropped a symbol. Runtime was not slow enough to note.

### 4b. The 13 NEW tests — `Siegebound.Assistant.Selection.*`, all ✅ PASS

`RosterShowsAllThirteenKinds` · `CollapseNamesTheHiddenKinds` · `FixedKeyOrderSurvives` · `ShrinkLoopNeverHidesASymbol` · `ExclusionParses` · `ExclusionArityRefused` · `ExclusionConflictRefused` · `ExclusionRefusedOnArmyWideIntents` · `ExclusionSymbolsRefused` · `PreExistingWhoShapesUnchanged` · `ValidatorExclusionArgumentIsLoadBearing` · `GrammarAdmitsExceptOnlyWithKinds` · `PositionalAcceptKeyResolves`

**13 registered, 13 green.**

### 4c. The 4 RE-BASED tests — ✅ PASS **ON THEIR NEW TERMS, verified at the assertion, not merely green**

⛔ The whole point of this section is that a reader can see these were **re-based, not silenced.** I checked each assertion's literal.

| test | result | the NEW term it passed on |
|---|---|---|
| **`ZoneA.TwoLaneByteEquality`** | ✅ PASS | Log, live: *"shipped: **5424** chars / 5424 UTF-8 bytes (named baseline 5424, 2026-08-04); spike lane: **5116** chars / 5116 UTF-8 bytes (frozen at 5116); declared **D4 delta: 308**."* The frozen spike fixture is intact and the shipped/spike divergence is still asserted REAL. |
| **`ZoneA.MeasuredCharCount`** | ✅ PASS | `ShippedZoneAChars = 5424` (`SiegeAssistantZoneATest.cpp:287`), asserted exactly at `:602-603`. **5116 → 5424 re-base confirmed.** |
| **`ZoneA.NullVocabularyIsNotTheMeasuredLane`** | ✅ PASS | Derives from the same constant — re-based to `ShippedZoneAChars`, not the spike's. ⭐ **This is the third broken test TASK-523 found by READING THE CODE rather than by anyone's grep**; had it been left on `SpikeLaneZoneAChars` it would have compiled, run, and been wrong by exactly 308. |
| **`Grammar.SelectionCap`** | ✅ PASS | `who` alternatives asserted **`== 4`** exactly (`SiegeAssistantGrammarTest.cpp:700-702`, *"4 alternatives since TASK-518; was 3"*). ⭐ **RE-BASED, NOT LOOSENED** — the comment at `:673-680` explicitly refuses the `>= 3` "repair" that would silently admit a fifth `who` shape. `SpikeLaneZoneAChars = 5116` stays untouched at `:263`. |

### 4d. Regression check — the pre-existing 40

**53 − 13 new = 40 pre-existing, all green.** Previous run was 40/40. ⇒ **Zero regressions.**

By group: `Assistant.Command` 4 · `Assistant.Grammar` 7 · `Assistant.Guard` 8 · `Assistant.Selection` **13 (new)** · `Assistant.Snapshot` 1 · `Assistant.Vocabulary` 1 · `Assistant.ZoneA` 5 · `Input.*` 7 · `Settings.*` 7.

⚠️ The `Input.*` 7 (the keyboard-layout batch) and `Settings.*` 7 are all green — the trailing-default parameter and the sixth `FSiegeAssistantCommand` field disturbed nothing upstream.

## 5. ⛔ WHAT THIS GREEN SUITE DOES **NOT** PROVE — the reporting duties `qa/TASK-525.md` imposes

I am discharging these here so the checkpoint cannot restate the batch more strongly than the evidence.

- ⛔ **Exclusion refusal is NOT "measured on the corpus."** (W-4) Three of the four new refusal rows (DEV-28/29/30) **cannot fail a silent drop** — `SpikeEval`'s `bWantsClarify` branch is unconditionally `true`. Only **DEV-32** is falsifiable. The runner fix is a boarded follow-up.
- ⛔ **A green `DEV-31` is NOT evidence the Sorcerer collapse leg is fixed.** (W-5) The collapse has never happened on the eval lane, even in principle. **Only §4a's two tests see it** — which is exactly why I reported them by name.
- ⛔ **Any dev score after 2026-08-04 is NOT comparable to one before it** (25 → 32 rows). A ladder graph spanning this date without a break is a false trend.
- ⛔ **A green suite proves NOTHING about:** the `Z` key actually firing (no headless Slate focus), the executor's exclusion filter on real actors (no world), the `Cancelled` transcript line, or the on-screen prompt. **All four are TASK-527's — Jonathan's.**

## 6. ⚠️ NO PIE SESSION OCCURRED ⇒ `zoneB_chars` WAS NOT CAPTURED

Spec item (6). The run was `-nullrhi` headless automation; **no PIE session happened at all.** Grep for the literal `FIRST LIVE CAPTURE` across the full 453 KB log: **zero hits.**

⇒ ⛔ **TASK-528 STAYS BLOCKED, and that is the correct outcome.** ⛔ **I changed no constant** — `ZoneBCharReserve` remains **192**, `SnapshotTrimBudgetChars` **1085**, `MaxRosterKinds` **13**, all untouched.

📌 ⚠️ **AND A DERIVED FIGURE WAS AVAILABLE AND I REFUSED IT.** The suite log carries `Roster 183 chars of a 213-char budget (SnapshotTrimBudgetChars 1085, ZoneBCharReserve 192)` 100 times over. **That is a TEST-FIXTURE board, not a live capture** — it is precisely the substitution `AS-§12g` forbids (*a derived figure is not a measurement*). **It is recorded here as an observation and is NOT a discharge of TASK-528's precondition.**

## 7. Commit

⛔ **Staged by EXPLICIT FILE PATH ONLY — never `git add -A`, never a directory pathspec** (GIT HAZARD LAW (d); this index is recorded as auto-staged/hostile by the editor's Git provider). Staged set verified with `git diff --cached --name-only` before committing.

### ⭐ **COMMIT: `5a07b96b89f5c1cee9ab83123205418b2e732f55` (`5a07b96`)**
`TASK-526: ASSISTANT-EXCLUDE lands — exclusion grammar, 13-kind roster, Z-accepts confirm (TASK-516..525)`

**29 files** — 16 source (`Siegebound/*.{h,cpp}` + the three `Tests/`, of which `SiegeAssistantSelectionTest.cpp` is NEW) · `Docs/Data/assistant_eval_dev.csv` · `TASKBOARD.md` · `CONVENTIONS.md` · the nine `TASK-51x/52x-programmer.md` handoffs · `qa/TASK-525.md`.

📌 **This handoff lands in the immediately following commit on `main`** — a file cannot carry the hash of the commit that contains it, and ⛔ amending `5a07b96` to inject it would rewrite a commit whose hash is already reported. The two commits are the batch.

⛔ **NOTHING WAS PUSHED.** `main` is now **4 ahead of `origin/main`** (the 2 KEYBOARD-LAYOUT commits + these 2). **The push is Jonathan's call, standing law.**

## 8. M8 / replication line — **VERBATIM, as required**

> **adds no replicated property, no new replicated class, no new relevancy tier.**

## 9. Follow-ups I observed (for the manager to board — I did not act on any of them)

1. **Owed to the manager by `qa/TASK-525.md` and still open:** `W-2` (amend `AS-§20.3` with the count-magnitude qualifier; re-rate TASK-528 as materially more urgent) · `W-3` (board the deferred-cancel gap **and put it on TASK-527's question list** — `OnConsoleCancelled` has zero live broadcasters, so a latched `Deferred` order cannot be dropped from the console) · `W-6` (board "the 4th parameter becomes REQUIRED when M8 P2 opens the wire path") · `W-7` (amend three `names:` blocks + RULING 1's table) · `N-8` (write the grammar↔Zone-A mirror law into `AS-§9c` / `AS-§20`).
2. **`W-1` is doc-only and I deliberately did NOT ride it on this gate.** The preview tunnel's undocumented second precondition (`CurrentWidget.Widget->IsEnabled()` at `SlateApplication.cpp:5027`) still lacks its one-sentence header note. Authoring it here would have made this build-master a code author and triggered `SC-§27`. **It needs a one-line boarded task.**
3. ⛔ **TASK-527 is now UNBLOCKED and it is the gate that matters.** Everything this batch can prove is proven; the three defects Jonathan reported are only *measurable* here, not *felt*. The compile and the suite say the mechanism is right — **only he can say the sentences work.**

## 10. Verification trail

- Build log: `…/scratchpad/TASK-526-build.log`
- Automation log (453,648 bytes, 53 results parsed): `…/scratchpad/TASK-526-automation.log`
- UBT log: `C:\Users\wesel\AppData\Local\UnrealBuildTool\Log.txt` — 0 errors, 0 warnings
