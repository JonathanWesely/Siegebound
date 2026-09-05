# QA Report — TASK-965 — THE GATE OVER THE OWED COMPILED RED

**Verdict: ✅ PASS — ⛔ 0 BLOCKERS** · 4 WARN · 6 NIT
**Gates:** `TASK-964` ⛔ **ALONE** (`SC-§29` ledger at §9)
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` (MODIFIED, 817 lines, 2 test declarations, sole deliverable) + `handoffs/TASK-964-programmer.md`
**Date:** 2026-09-03 · qa-reviewer · read-only

---

## 0. ⛔⛔ WHAT I COULD AND COULD NOT EXECUTE — ⛔ ABOVE THE VERDICT (`TL-§5c` cl. 5(a))

⛔ **I HAVE NO SHELL, NO GIT, NO COMPILE, NO EDITOR AND NO MCP.** My tool set is read/grep/glob/write + Slack. ⛔ **I did not compile this file, I did not run the suite, and I could not run `git status` or `sha256sum`.** Reporting a green I did not watch, or a `git status` I did not run, would be this batch's own defect one level up.

⚠️ **`SC-§55` APPLIED TO MYSELF, FIRST THING.** My environment preamble carries a `gitStatus` block listing **Castle-era files with `A ` staged entries** (`MI_Castle_Interior_*`, `Castle.fbx`, `TASK-626`/`629`–`634` handoffs). ⛔ **It is stale, it is not this batch, and I did not use one byte of it.** Every zero below is from an instrument I ran at my own instant, with its own positive control.

| | what it is | who owes it |
|---|---|---|
| ✅ **RE-DERIVED BY ME** | the `default:` census + its positive control · the `-Wswitch` site census · the `/Game/Blueprints` copy census · the `AddError` census · the declaration census (worktree endpoint) · the synthetic's absence in `DT_Cards` + its positive control · the substring hazard · the 22-row join · the git-root location · the Witch digest in the LFS object store | this report |
| ⛔ **DECLARED (the author's, unwatched by me)** | `git status` over `Source/` and `Content/` · the `09b9b50` endpoint of the delta · "the include block is byte-identical" · the shell-mirror transcript | ⭐ **`TASK-961`** |
| ⛔ **STILL OWED BY ANYONE** | ⛔ **a compiled RED BAR** | ⭐ **`TASK-961`** — §4 |

---

## 1. ⛔⛔ ITEM (1) — THE METHOD QUESTION. ⛔ THE BLOCKER-GRADE ONE. ⛔ ADJUDICATED: ⭐ **SYNTHETIC INPUT. ⛔ NO ASSET WAS DISTURBED.**

⛔ **I hold no Git, so I cannot reproduce a `git status`. ⭐ What I CAN measure I did, and the three measurements below are mine, not relayed:**

**(a) ⭐⭐ THE DIGEST IS CORROBORATED FROM AN INSTRUMENT THE AUTHOR DID NOT USE.** `*.uasset` is LFS-tracked (`.gitattributes:1`, `filter=lfs`) ⇒ an LFS object is **named by the sha256 of its own content**. I globbed the object store:

```
.git/lfs/objects/b4/f3/b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab   <- EXISTS
```

⇒ ⛔ **the digest `TASK-949` recorded at four points before the refusal, the digest `TASK-964` re-read, and a real object in the LFS store are the SAME 64 hex characters.** ⭐ **This is the oid-vs-sha256 discipline, not a size check.**

**(b) ⛔ NO RESIDUE OF A MOVE ANYWHERE IN THE REPO.** `**/BP_Unit_Witch*` globbed from the **repo root** (`C:/GitProjects/GitHub/GitClaudeUnrealTesting`, i.e. above the project dir) returns ⛔ **EXACTLY ONE PATH**: `Content/Blueprints/Units/BP_Unit_Witch.uasset`. ⛔ No `.bak`, no `.orig`, no temp copy, no second location. ⭐ **A move that was performed and reversed would have had to leave nothing — but a move that was performed and NOT fully reversed is exactly what this glob would catch, and it caught nothing.**

**(c) ⛔ THE ASSET IS PRESENT AND THE ROSTER JOINS PERFECTLY — see §3, which is the strongest single piece of evidence in this report that nothing was moved aside.**

⚖️ **RULING: ⭐ THE RED WAS SOUGHT FROM A SYNTHETIC ABSENT INPUT, ⛔ NOT FROM A DISTURBED REAL ASSET.** ⛔ **No `Content/**` write, no move, no rename, no stub, no restore is visible to any instrument I hold.** ⭐ **`SC-§54` cl. 3 is satisfied by METHOD, and the fence that produced this row is intact.**
⚠️ **The binding `git status` half is NOT mine and I do not claim it** — it transfers to `TASK-961` (§8.1).

---

## 2. ⛔⛔ ITEMS (3) + (4) — ⛔ THE SAME SYMBOLS, AND ⛔ IT INVERTS RATHER THAN RE-RUNS. ⭐ **BOTH SATISFIED, MEASURED.**

### (a) ⛔ THE SYMBOLS — ⛔ THE SAME ONES THE GREEN TEST CALLS (`SC-§38`)

I traced the new test's call order at source (`:713`–`:797`) rather than taking §2 of the handoff:

`FindSoftObjectField` (`:690`) → `FindNameArrayField` (`:706`) → `FindRow<FCardRow>` (`:723`/`:725`) → **`ClassifyRow`** (`:742`/`:752`) → **`ComposeActorClassPath`** (`:763`/`:764`) → **`FPackageName::DoesPackageExist`** (`:787`/`:788`)

⛔ **Every one is the same fixture symbol the sibling walk calls** (`:364`, `:396`, `:424`, `:443`, `:464`, `:472`). ⛔ **It hand-builds no path.** ⭐ **MEASURED, not asserted:**

```
grep -n '/Game/'  SiegeCardRosterTest.cpp
   123 198 199 297 298 309 310 315 316 374    <- ALL pre-existing, ALL above the new test (starts :589)
   626                                        <- the ONLY hit in the new region, and it is the COMMENT
                                                 SAYING there is no /Game/ literal
grep -c '/Game/Blueprints'  ->  9   (123 198 199 297 298 309 310 315 316)  -> ⛔ ZERO in the new test
```

⇒ ⛔⛔ **ZERO path-composer copies were added. ⭐ `TASK-959` — that is your number, re-measured by me at my instant: the composition still lives in exactly ONE function (`ComposeActorClassPath`, `:302`–`:321`), and the new test CONSUMES it.**

### (b) ⛔⛔ IT INVERTS. ⛔ IT DOES NOT `AddError` ON THE EXPECTED ABSENCE. ⭐ **CENSUSED.**

```
grep -n 'AddError'  SiegeCardRosterTest.cpp
  sibling walk : 358 367 374 381 387 399 429 450 481          (9)
  doc comments : 614 (x2), 784                                (3 — prose, not calls)
  NEW TEST     : 685 693 701 709 733 747 758                  (7)
```

⛔ **I opened all seven.** Every one is a **SELF-CHECK** failure — CDO unreachable (`:685`) · `CardTableAsset` renamed/retyped (`:693`) · table failed to load (`:701`) · `BuildingEconomyCardIDs` unreachable (`:709`) · **row probe blind** (`:733`) · synthetic not routed spawnable (`:747`) · positive control no longer spawnable (`:758`). ⛔ **Every one is followed by `return false`.** ⛔ **NOT ONE fires on the expected absence.**

⛔ **The load-bearing line is `TestFalse(…, bAbsentResolves)` at `:790` where `bAbsentResolves = FPackageName::DoesPackageExist(SyntheticComposed.PackageName)` (`:787`) — the IDENTICAL expression the sibling asserts TRUE at `:472`/`:474`.**

⇒ ⛔⛔ **THE SUITE STAYS GREEN, AND IT IS GREEN ⛔ BECAUSE THE PROBE ANSWERED ABSENT.** ⭐ **Item (4) satisfied: this is a control, not a bug wearing a control's clothes.** ⛔ **It will not block `TASK-961`.**

### (c) ⭐ THE TWO LEDGERS, AUDITED SEPARATELY (`SC-§51` cl. 4)

| ledger | assertion | line | the world in which it ALONE fires |
|---|---|---|---|
| **(a) reachability / premise** | `SyntheticCategory == UnitActor` | `:744` | `ClassifyRow` stops routing `Unit` → an actor path ⇒ the control would stand for a red that could never happen |
| **(a)** | positive control routes spawnable | `:754` | `Sorcerer` is retired or retyped |
| **(a)** | the two composed names DIFFER | `:775` | a composer that ignores its argument ⇒ discrimination below goes vacuous |
| **(b) instrument — row probe** | `SyntheticRow == nullptr` | `:727` | the synthetic becomes a real card (liveness) |
| **(b) instrument — row probe** | `PositiveRow` non-null | `:730` | `FindRow` answers null for **everything** |
| **(b) instrument — existence probe** | `TestFalse(bAbsentResolves)` | `:790` | the probe goes blind and answers PRESENT for everything |
| **(b) instrument — existence probe** | `TestTrue(bPresentResolves)` | `:793` | ⛔ **the probe goes blind and answers ABSENT for everything — the ONLY mode in which `:790` could pass vacuously** |
| **(b) discrimination** | `bAbsentResolves != bPresentResolves` | `:796` | both probes stuck at the same polarity in one run |

⛔ **Two ledgers, kept apart, each tell with a world it alone fires in.** ⭐ **`SC-§51` cl. 1–4 satisfied — and unlike `TASK-947`'s handoff, this author did not merge them into one headline number.**

---

## 3. ⭐⭐ THE FINDING I WEIGHED MOST — ⛔ THE SUGGESTED SYNTHETIC WAS ITSELF A TRAP. ⛔ **THE REJECTION IS CORRECT AND I PROVED IT BOTH WAYS.**

**The board suggested `ZzNoSuchCard`. It is a strict SUBSTRING of the fixture's existing `ZzNoSuchCardZz`.** ⛔ **I did not take that on the author's word — I ran both patterns over the whole project at my own instant:**

```
grep -rc 'ZzNoSuchCardZz'  ->  22 matches / 5 files   TASKBOARD 1 · qa/TASK-948 4 · SiegeCardRosterTest.cpp 4 · TASK-964 handoff 6 · TASK-947 handoff 7
grep -rc 'ZzNoSuchCard'    ->  25 matches / 5 files   ⛔ THE SAME FIVE FILES
```

⇒ ⛔⛔ **A grep establishing the "absence" of `ZzNoSuchCard` returns 22 hits that belong to the EXISTING constant.** ⭐ **Its measured absence would have been UNMEASURABLE BY THE INSTRUMENT ESTABLISHING IT — the substring hazard sitting inside the synthetic value chosen to prove an absence.** ⛔ **Taking the board's suggestion would have shipped a negative control whose "measured absent" was a false zero.** ⚖️ **The refusal to adopt a relayed value and re-measure instead is `SC-§40` cl. 3 working exactly as written, and it is the best thing in this row.**

### ⛔ AND THE SUBSTITUTION ITSELF, RE-MEASURED BY ME WITH A POSITIVE CONTROL ON EVERY INSTRUMENT (`SC-§39`)

| instrument (mine, this instant) | `ZzNoSuchCardZz` | ⭐ `Sorcerer` (positive control) |
|---|---|---|
| `Content/Data/DT_Cards.uasset` (binary) | ⛔ **0** | **3** — ⛔ the instrument is proven able to return non-zero **on the same binary** |
| `Docs/Data/cards.csv` | ⛔ **0** | **1** (`:31`) |
| `Content/Blueprints/**` filenames | ⛔ **0** | **1** (`BP_Unit_Sorcerer.uasset`) |
| files under `Source/` | **1** ⚠️ | the test file itself, which DECLARES it — correct and expected |
| ⛔ anywhere under `Content/**` or `Docs/**` | ⛔ **0** | — |

✅ **The zero is MEASURED, on an instrument shown able to return non-zero before I trusted it.**

### ⭐⭐ AND THE JOIN THAT VALIDATES THE MIRROR'S MAPPING FAR HARDER THAN `22 = 22`

The handoff argues its mapping is validated because the compiled gate published **22 spawnable / 0 unresolved** and it finds **22** Blueprints on disk. ⛔ **Cardinality alone would also match with one extra and one missing.** ⭐ **So I ran the JOIN, by name, both directions:**

- **My own `cards.csv` census (all 32 rows):** 13 `Unit` · 2 `Economy` · 7 `Building` · 10 excluded (5 `Spell` + 4 `HeroUpgrade` + 1 `Utility`) = **32**, spawnable **22** ⇒ ⛔ **character-for-character the compiled run's own `32 read / 22 probed (13 Unit, 2 Economy, 7 Building) / 10 EXCLUDED`.**
- **Building-path CardIDs** (7 `Building` + `DeepMine`, the `BuildingEconomyCardIDs` exception) = **8** ⇒ disk has **exactly 8** `BP_Building_*`, and the **name sets are identical**: ArrowTower · Wall · BombTower · BallistaTower · Barracks · CrystalTower · WatchTower · DeepMine.
- **Unit-path CardIDs** (13 `Unit` + `Miner`, the Economy card NOT in the exception list) = **14** ⇒ disk has **exactly 14** `BP_Unit_*`, and the **name sets are identical**: Footman · Archer · Knight · MilitiaMob · Pikeman · Sapper · Cavalry · Longbowman · Cleric · Ogre · Wizard · Sorcerer · **Witch** · Miner.

⇒ ⛔⛔ **22 composed paths, 22 assets, ⛔ ZERO missing and ⛔ ZERO extra, on a one-for-one name join I performed myself.** ⭐ **This does four things at once: it validates the mirror's mapping (item 3's substitute), it independently reproduces the compiled gate's `0 unresolved`, it confirms `BP_Unit_Witch.uasset` is present and joined (item 1), and it confirms `DT_Cards` had NOT drifted from `cards.csv` at `09b9b50` — which is `qa/TASK-948.md` `W-4`'s predicted false-red, ⛔ still not materialising.**

---

## 4. ⛔⛔⭐⭐ ITEM (2) — THE OWED COMPILED RED. ⛔ **RULED: NOT A BLOCKER. ⛔ AND ⛔ NOT DISCHARGED EITHER. ⛔ IT STAYS OWED, AND IT TRANSFERS BY NAME.**

### 4.1 ⛔ WHY IT IS NOT A BLOCKER, AND THE REASONING IS THE JUDGEMENT CALL IN THIS ROW

Item (2) of my spec: *"IS THERE A VERBATIM RED TRANSCRIPT FROM THE INVERTED ASSERTION? A description of a red is not a red. Absent ⇒ BLOCKER."*

- ✅ **A verbatim transcript EXISTS** (handoff §3.2): RUN 1 GREEN on the shipped polarity, **RUN 2 RED on the inverted polarity** (`INVERTED — the absent card's composed path DOES resolve (expected 1, got 0)`), plus ⭐ **RUN 3, a self-control proving the harness can print `FAIL`** — so RUN 2's red is a measured failure and not a printer that cannot print anything else.
- ⛔ **It is a SHELL MIRROR, not the compiled binary, and the author says so ABOVE the claim** (handoff §0 and §3.2, both before any result). ⭐ **That is `SC-§54` cl. 4's required shape exactly: declared AS a substitute, in the same breath as the claim and above it, never folded into it.**
- ⛔⛔ **AND THE DECIDING POINT, WHICH IS ABOUT THE SPEC AND NOT ABOUT THE AUTHOR: `TASK-964` item (3) demanded an OBSERVED red while item (6) fenced *"any compile · any editor/MCP"*.** ⛔ **The row asked for an observation and forbade the only instrument that can produce one.** ⭐ **The author named the contradiction rather than papering over it (`TL-§5c` cl. 5(a)), and did not quietly redefine item (3).** ⚖️ **Failing a row for not executing what its own spec forbade would punish the exact honesty `SC-§54` was written to buy — and would teach the next agent to fold the substitute into the claim instead of declaring it.**

⇒ ⛔ **NOT A BLOCKER.**

### 4.2 ⛔ WHAT IS ⛔ ACTUALLY DISCHARGED, AND ⛔ WHAT IS NOT — ⛔ STATED SEPARATELY BECAUSE THEY ARE DIFFERENT DEBTS

| debt | status | why |
|---|---|---|
| ⭐ **THE METHOD** (`SC-§54` cl. 2/3 — a method that does not require the refused move) | ✅ ⛔ **DISCHARGED** | measured-absent synthetic input, zero assets touched (§1) |
| ⭐ **PERMANENCE** (`SC-§54` cl. 3(c)) | ✅ ⛔ **DISCHARGED, AND BETTER THAN THE ALTERNATIVE** | a moved asset buys ONE transcript that decays into a screenshot; this buys an assertion that re-proves the probe can say NO on **every** run |
| ⭐ **"PROVE YOUR CONTROL CAN FAIL" FOR THE NEW TEST** (`SHIP-§9`, `SC-§51` cl. 6) | ✅ ⛔ **DISCHARGED STRUCTURALLY** | ⛔ the positive control at `:793` **IS** the inverted assertion, through the SAME symbol, in the SAME run ⇒ **a probe stuck at EITHER polarity takes this test red**, and `:796` closes the remaining direction. ⭐ This is a compiled, permanent, re-runnable form of the one-off source-edit transcript |
| ⛔⛔ **A COMPILED RED ⛔ BAR** | ⛔ **NOT DISCHARGED. ⛔ STILL OWED.** | see below |

⛔⛔ **AND I AM GOING TO BE PRECISE ABOUT THE LAST ROW, BECAUSE THE HANDOFF'S STRONGEST CLAIM IS ALSO ITS SOFTEST.** The handoff says the compiled run will now print `DoesPackageExist(…ZzNoSuchCardZz) = ABSENT` on every pass, so *"the red-producing condition is now observed and printed on every compiled green pass."*

⚠️ **I checked what that is actually worth, and it is worth LESS than it reads — because the sibling walk ALREADY did the load-bearing half at `09b9b50`.** `:564`–`:566` composes the **same** synthetic path through the **same** symbol and already asserts `TestFalse(DoesPackageExist(...))` — ⛔ **and that line compiled and ran GREEN in `TASK-949`'s executed suite.** ⇒ ⛔ **the compiled binary has ALREADY been observed evaluating `DoesPackageExist` to FALSE on a composed unit path.** ⭐ **What `TASK-964` adds on top of that is real and it is not nothing — the opposite polarity on the same symbol in the same run, the discrimination assertion, the reachability premise, the `FindRow` control, and the two published `AddInfo` lines — but it is ⛔ NOT a red bar, and it was never going to be one.**

⚖️ **RULING: ⛔ THE COMPILED RED BAR REMAINS OWED. ⛔ It does NOT evaporate with this row** (`SC-§54` cl. 2). ⛔ **It transfers BY NAME to `TASK-961`'s executed run** (`TL-§5c` cl. 5(c)) — see §8.
⭐ **AND THE ROW HAS BOUGHT SOMETHING THAT MAKES THE REMAINING DEBT CHEAP: the red is now reachable WITHOUT touching an asset and WITHOUT a permission fence** — `:793`, `:790` and `:796` are three lines whose polarity a future row can flip in `Source/` alone, which is exactly the shape `TASK-964` item (3) wanted and could not execute.

⛔ **ONE THING I CHECKED SPECIFICALLY BECAUSE IT WOULD BE THE WORST FAILURE HERE: is there a RESIDUAL INVERSION left in the tree?** ⛔ **No.** The three polarities at `:790`/`:793`/`:796` are `TestFalse(bAbsentResolves)` / `TestTrue(bPresentResolves)` / `TestTrue(bAbsentResolves != bPresentResolves)` — ⛔ **read at source, character by character, and they are the SHIPPED polarity, not the inverted one.** ⛔ **And the sibling walk's eight count/control assertions (`:534` `:537` `:540` `:548` `:551` `:555` `:558` `:561` `:565`) are all present and NONE is weakened** — I diffed them against `qa/TASK-948.md` §1's enumeration line by line; the only change is a uniform line-number shift from the header growing.

---

## 5. ⛔ ITEM (5) — ⛔ A ⛔ SEPARATE, ⛔ NON-BLOCKING LEDGER LINE: ⛔ THE `-Wswitch` RIDER

⚖️ **RIDER VERDICT: ✅ COLLECTED — with one WARN. ⛔ This CANNOT change `TASK-964`'s verdict** (`SC-§29`), except through (c), which is blocker-grade and passes.

### (a) ⛔ ALL SITES BY ⛔ SYMBOL CENSUS, ⛔ NOT BY THE BOARD'S COUNT. ✅ **2 — AND I AGREE.**

```
grep -rn '\-Wswitch'  Source/
  Tests/SiegeCardRosterTest.cpp:94    <- file header, item (b)
  Tests/SiegeCardRosterTest.cpp:254   <- the ClassifyRow doc comment
  ⇒ 2 sites, 1 file, ZERO elsewhere under Source/
```
✅ **Agrees with the board's count and with the author's. Censused by symbol, as required.**

### (b) ⛔ IS THE AMENDED SENTENCE `SC-§53` cl. 3 SHAPED — ⛔ PAST TENSE + ⛔ DATE + ⛔ TOOLCHAIN? ✅ **YES, AT BOTH SITES.**

Both carry **"AS MEASURED 2026-09-03 AT `09b9b50`"** + the hash + **"MSVC 14.50"** named + the `Build.cs` zero-warning-configuration measurement + ⭐ **"THE RUN-TIME TELL IS THE LOAD-BEARING HALF HERE."** ⛔ **The claim moved; the mechanism did not** — see (c).

### (c) ⛔⛔ WAS A `default:` LABEL ADDED? ⛔ **NO. ⛔ ZERO. ⛔ MEASURED AND POSITIVE-CONTROLLED.**

```
grep -c '^\s*default\s*:'  Tests/SiegeCardRosterTest.cpp      ->  ⭐ 0
grep -c '^\s*default\s*:'  SiegePlayerController.cpp          ->     7   ⛔ THE INSTRUMENT'S OWN POSITIVE CONTROL
```
⭐ **My regex is proven able to SEE a `default:` label before I trusted its zero.**

⚠️⚠️ **AND I FELL INTO THE TRAP THE HANDOFF NAMED, WHICH IS WHY IT WAS WORTH NAMING:** a bare `grep -n 'default:'` on the file returns hits at **`:91`, `:245`, `:247`** — ⛔ **all three are COMMENTS, and two of them are the prose DOCUMENTING THE ABSENCE** (`:245` *"NOTE THE ABSENT `default:` LABEL"*, `:247` the forbidden `default: Category = NotSpawnable;` quoted inside the prohibition). ⛔ **A reviewer who ran the bare grep and stopped would report a violation that does not exist.** ⛔ **THE LABEL COUNT IS 0.**

⛔ **AND THE MECHANISM IS INTACT, WHICH IS THE HALF THAT MATTERS.** I re-read `CardRow.h:17-25` myself: `ECardType` still declares **SIX** enumerators (`Unit` `Building` `Economy` `Spell` `HeroUpgrade` `Utility`). The switch at `:268`–`:289` names **all six**, carries **no `default:`**, and `Category` is still initialised to `Unclassified` at `:266` ⇒ the run-time path (`:445`–`:453` `AddError` + `continue`, and the partition assertion at `:540` as the second independent tell) is **unchanged**. ✅ **`qa/TASK-948.md` §(c)'s preservation ruling is ENFORCED, not reopened** — and the author went further and cited that ruling BY NAME at `:246`, so the next tidy-up meets the ruling instead of the temptation. ⭐ **That is the right instinct and I am recording it as a credit.**

---

## 6. ⛔ ITEM (6) — `TL-§5b` / `TL-§5c`. ✅ **COMPLIANT. ⛔ AND THE EQUATION BALANCES TO THE UNIT.**

⛔ **I ran the census myself at the mandated scope and needle:**
```
scope  Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp   needle  ^IMPLEMENT_SIMPLE_AUTOMATION_TEST
   ->  434 across 32 files      (SiegeCardRosterTest.cpp contributes exactly 2, both at line-start: :341, :667)
```

| | figure | source |
|---|---|---|
| last **EXECUTED** | **433 `Result={Success}` / 0 `Result={Fail}`** at `09b9b50` | ⛔ **not mine — declared by `TASK-949`, unwatched by me** |
| author's declared delta | **`+1`, UNEXECUTED** | handoff §6 |
| **my fresh census** | **434 across 32 files** | ⛔ **mine, this instant** |

⇒ ⛔⛔ **`433 + 1 = 434`. ⭐ THE `TL-§5b` cl. 3 EQUATION BALANCES ⇒ ⛔ NO UNDECLARED TEST FILE LANDED, AND THE FILE COUNT DID NOT MOVE (32 → 32) ⇒ the new declaration is in an EXISTING file, as specified.**

✅ **`TL-§5c` cl. 5(a): the handoff labels `434` a DECLARATION CENSUS and says ⛔ in capitals that it did NOT run the suite; the board row status line repeats it.** ✅ **`TL-§5b`: no absolute is carried as an expectation — both figures are published with scope, file count and endpoint.** ⛔ **Item (6) does NOT fail.**

### ⚠️ THE FOURTH OCCURRENCE OF THE GIT-ROOT TRAP — ⛔ CONFIRMED, AND ⛔ WHETHER I HIT IT

⛔ **I could not hit the git-root trap itself, because I hold no Git — and I will not claim a hazard I could not meet.** ⭐ **But its PREMISE is measurable without Git and I measured it two independent ways:**

```
glob  C:/GitProjects/GitHub/GitClaudeUnrealTesting/.git/HEAD          -> EXISTS
glob  C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/.git/**  -> ⛔ NO FILES FOUND
glob  C:/GitProjects/GitHub/GitClaudeUnrealTesting/.gitattributes     -> EXISTS (9 LFS patterns)
```
⇒ ⛔⛔ **THE GIT ROOT IS ⛔ ONE LEVEL ABOVE THE `.uproject`, CONFIRMED BY ME.** ⛔ **`git ls-tree` prints cwd-relative paths and `git show HEAD:<path>` demands root-relative ones ⇒ the 32 silent failures summing to a confident `0` are fully explained, and `--full-name` is the correct fix.** ✅ **I accept the corrected `+1 → 434/32` — and note that my own independent census of the worktree endpoint lands on the same 434, which is a second instrument agreeing with the corrected run.**
⭐ **AND I DID MEET THE FAMILY, TWICE, IN THIS REVIEW:** the `default:` comment false-positive (§5(c)) and the `/Game/` comment false-positive at `:626` (§2(a)) — ⛔ **both are "the sentence documenting an absence reads as a presence", both would have produced a confident wrong answer, and both were caught only because I opened the hits instead of counting them.** ⚠️ **That is now the same class of instrument failure five times in one day; the general rule it keeps re-teaching is `SC-§39.1` — ⛔ report a census's CARDINALITY beside its result, and open the hits.**

---

## 7. ⚠️ THE AMENDMENT BEYOND THE NAMED RIDER — ⛔ **RULED: ⛔ IN SCOPE. ⛔ IT STAYS.**

**What it is:** the file header's `THIS TEST IS RED TODAY` paragraph, rewritten to `THIS TEST WAS RED WHEN IT WAS WRITTEN` (`:138`–`:166`).

⚖️ **RULING — ⛔ IN SCOPE, ⛔ CORRECT, AND ⛔ IT WOULD HAVE BEEN A FINDING IF IT HAD BEEN LEFT:**
1. ⛔ **The old text was a present-tense claim that is now measurably FALSE.** It read *"`BP_Unit_Witch` is absent as this file is written… The row for `Witch` MUST fail."* ⛔ **`BP_Unit_Witch.uasset` is on disk (I globbed it), it is joined to its row (§3), and the gate went GREEN at `09b9b50`.** ⇒ ⭐ **`SC-§53` cl. 1 VERBATIM: the repair landed in one commit and the paragraph describing the pre-repair state did not — ⛔ in the very file this row edits, with `SC-§53` among its cited laws.**
2. ⛔ **`SC-§53` cl. 2 makes it a DUTY, not a liberty:** the description must be amended in the same batch or boarded as a named row. ⛔ **This row is the only writer of this file in the batch, and `TASK-959` rewrites it next.** ⇒ **leaving it would have orphaned it.**
3. ✅ **It is comment-only, it is inside the row's SOLE permitted file, and it was DECLARED rather than discovered** (`SC-§29`).
4. ⛔⛔ **I checked the two things that would have made it a BLOCKER, and both are clean:**
   - ⭐ **THE VERBATIM PREDICATE QUOTE AT `:48`–`:49` IS UNPERTURBED.** I diffed it against the board's own words at `TASKBOARD.md:17190` (`TASK-947` item (1)): *"WALK EVERY `DT_Cards` ROW OF A SPAWNABLE `CardType` (Unit · Economy · Building) AND ASSERT ITS COMPOSED CONVENTIONS PATH RESOLVES."* ⛔ **Identical. `SC-§49` cl. 4(a) intact — the paraphrase IS the defect and there still isn't one.**
   - ⭐ **THE PROHIBITION SURVIVED AND WAS NOT SOFTENED** (`:146`–`:148`): *"IF IT GOES RED AGAIN, DO NOT 'FIX' IT BY WEAKENING THE ASSERTION, SKIPPING THE ROW OR ALLOWLISTING A CARD ID — the only correct repair is the missing asset."*
5. ⭐⭐ **AND THE PART THAT DECIDED IT FOR ME: at `:658`–`:665` the author was ALSO offered a stale-looking number — the header's `950d8c5` reading of `0` under `Source/`, which now reads `1` — and ⛔ REFUSED to "correct" it, on the grounds that a past-tense, correctly-dated measurement is NOT stale.** ⛔ **That is `SC-§53` cl. 3's explicit warning applied against the author's own interest, in the same diff.** ⚖️ **An author who amends the false present-tense sentence AND declines to touch the true past-tense one has understood the law rather than pattern-matched it.**

⚠️ **One defect survives inside the cure — see `W-1`. It is a NIT-grade sentence in a WARN-grade position and it does not change this ruling.**

---

## 8. ⛔ FINDINGS

### ⛔ BLOCKERS — **NONE**

### ⚠️ WARN

- **[WARN W-1]** `SiegeCardRosterTest.cpp:146` — ⛔ **the cure re-introduces a milder form of the disease.** The rewritten paragraph closes with **"⇒ THE GATE IS GREEN TODAY AND MUST STAY THAT WAY"** — ⛔ **an undated present-tense claim, in the paragraph rewritten to remove an undated present-tense claim, in a file whose cited law is `SC-§53`.** ⭐ The measured half immediately above it (`:143`–`:145`) is correctly past-tense, hashed (`09b9b50`) and dated, so the damage is confined to one clause. **Fix (future edit, comment-only):** *"as measured at `09b9b50` it was GREEN, and it must stay that way."* ⛔ **Non-blocking; ⛔ do NOT re-open the file for this alone — fold it into `TASK-959`'s rewrite of this file.**
- **[WARN W-2]** `SiegeCardRosterTest.cpp:94` + `:254` — ⛔ **the amended `-Wswitch` sentence still overstates the compile-time half by one notch, which is the exact thing the rider was collected to stop.** Both sites say a 7th enumerator is a **"COMPILE error"/"COMPILE failure"** under Clang. ⛔ **`-Wswitch` is a default-ON *warning*; it is an *error* only under `-Werror` / `-Werror=switch`** — and this project was measured (by `qa/TASK-948.md` `W-2`, re-cited here) to set **no** warning configuration at all, so neither `-Werror` nor MSVC `4062`/`4061` is in play. ⇒ ⛔ **on the standing toolchain the compile-time half does not merely "MAY NOT FIRE" — it would not be an error even on Clang.** ⭐ **The conclusion the sentence draws is UNAFFECTED and correct** (*the run-time tell is the load-bearing half*), which is why this is a WARN and not a blocker. **Fix:** *"under Clang `-Wswitch` (on by default) WARNS on a 7th enumerator, and is a compile error only under `-Werror`."*
- **[WARN W-3]** ⛔ **THE COMPILED RED BAR IS STILL OWED AND IT IS NOW `TASK-961`'s** (§4). ⛔ The transcript in hand is a **shell mirror**, correctly declared as a substitute above the claim. ⛔ **Nobody has yet seen any assertion in this file go RED on a compiled run, and this row did not change that.** **Fix:** §8.1 item 2 — it costs `TASK-961` nothing but attention.
- **[WARN W-4]** ⛔ **FOR `TASK-961`, AND IT IS A LIVE TRAP TWICE OVER:** the binding `git status` proving ⛔ `Content/**` clean and ⛔ only `Tests/SiegeCardRosterTest.cpp` modified under `Source/**` is **NOT provable by me** (no Git). ⛔ **Derive it from your OWN live `git status`** — ⛔ **not from the handoff's spelling** (the git root is one level above the project ⇒ repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix, `qa/TASK-948.md` `W-3`), and ⛔ **not from your context's `gitStatus` snapshot** (`SC-§55` — mine was Castle-era and carried `A ` staged entries; ⛔ **assume yours is too**).

### 📌 NIT

- **[NIT N-1]** `handoffs/TASK-964-programmer.md` §4 — the recorded instrument trap says *"a bare `grep -c 'default:'` on the file reads **1**… that hit is the comment at **line 213**."* ⛔ **My instrument reads 3 matching lines (`:91`, `:245`, `:247`), and `:213` in the delivered file carries no `default:` at all.** ⭐ **The trap is REAL and naming it was right; its numbers are stale (pre-final-edit) and they UNDERSTATE the false-positive count.** ⛔ Harmless direction — the label count is `0` on both readings — but a reader reproducing the "1" will conclude their instrument is wrong.
- **[NIT N-2]** `SiegeCardRosterTest.cpp:626` — the new test's doc comment contains the string **`/Game/...`** inside the sentence *"contains NO `/Game/...` literal"*. ⛔ **A `/Game/` grep over the new region returns exactly one hit, and that hit is the denial itself** — the same self-referential-comment false positive as the `default:` case, in the same diff, on a different needle. **Fix (optional):** write it as *"no `/Game`-rooted literal"*.
- **[NIT N-3]** `SiegeCardRosterTest.cpp:661` publishes `Sorcerer` in `DT_Cards.uasset` as **4**, while `:136` publishes **3** for the same asset and **my own instrument reads 3**. ⛔ **Almost certainly matching-lines vs occurrences on a binary blob, not an asset change** (nothing in this batch may touch `DT_Cards`, and `:136` is hash-pinned to a different tree). ⚠️ **But two different numbers for one measurement of one asset, 500 lines apart in one file, invite a reader to infer the asset changed.** Both are non-zero ⇒ the control holds either way. **Fix:** name the counting mode (*"4 occurrences / 3 matching lines"*).
- **[NIT N-4]** `SiegeCardRosterTest.cpp:697`–`:703` — the new test omits two of the sibling's self-checks: `CardTablePath.IsNull()` (`:372`) and `GetRowStruct() == FCardRow::StaticStruct()` (`:385`). ⛔ **Neither is a hole:** an unset path falls to the load failure at `:701` (RED, with an empty path in the message), and a wrong row struct makes `FindRow<FCardRow>` return null for **everything**, which the ⭐ ROW-PROBE POSITIVE CONTROL at `:730` catches by design and reports as *"the row probe is blind"*. ⇒ **a message-precision loss, not a detection loss.** ⛔ **Recorded so a future reader does not "fix" it by pasting the sibling's checks in and quietly duplicating what the control already does.**
- **[NIT N-5]** ⛔ **The absence half of the new test OVERLAPS the sibling's existing negative control at `:564`–`:566`**, which composes the same synthetic path through the same symbol and already ran **compiled and green** at `09b9b50`. ⭐ **The new test is a strict superset and is NOT redundant** — the positive polarity, the discrimination assertion, the reachability premise, the `FindRow` control and the published `AddInfo` lines are all new. ⚠️ **Recorded because it bounds what the row bought** (§4.2) and because a future tidy-up that deletes one as "duplicate" must delete the SIBLING'S one-liner, ⛔ never the controlled version.
- **[NIT N-6]** `handoffs/TASK-964-programmer.md` §7 reports the file as **817 lines**; my reader shows content through `:817` with a trailing line at `818`. Trailing-newline / counting-mode difference, no action.

---

## 8.1 ⛔ NOTES FOR BUILD-MASTER (`TASK-961`)

1. ⛔⛔ **YOU OWE THE `git status`.** ⛔ `Content/**` must be **clean of any move/delete/modify attributable to this row** — and per `TASK-964`'s own sweep the only `Content/` dirt should be the **pre-existing `Content/FogArea/**` set** (`TASK-927`, ⛔ **not this row's, ⛔ and not yours to stage**; the author measured 27 paths where `TASK-949` measured 24 — different instant, ⛔ report your own number). ⛔ `Source/**` must show **ONLY** `Tests/SiegeCardRosterTest.cpp` as `M`. ⛔ **Derive both from your own live status — `SC-§55`, your snapshot lies.**
2. ⭐⭐ **THE OWED COMPILED RED IS NOW YOURS (`W-3`), AND YOUR RUN CAN PAY MOST OF IT FOR FREE.** ⛔ **Capture and paste, verbatim, the two `AddInfo` lines the new test prints:**
   ```
   NEGATIVE CONTROL — DoesPackageExist('/Game/Blueprints/Units/BP_Unit_ZzNoSuchCardZz') = ABSENT   [synthetic, measured absent]
   NEGATIVE CONTROL — DoesPackageExist('/Game/Blueprints/Units/BP_Unit_Sorcerer') = PRESENT   [positive control]
   ```
   ⛔ **If BOTH read the same value, or if either line is absent from the log, the test did not run or the probe is stuck — ⛔ that is a finding, ⛔ not a formatting detail.** ⛔ **This is still NOT a red bar; say so in your handoff rather than letting the log stand in for one.**
3. ⛔ **`Build.bat` returns 0 on a FAILED build. ⛔ PARSE THE LOG for `Result: Failed`. ⛔ NEVER trust `$LASTEXITCODE`.** ⛔ **And check the log's SIZE and content before trusting any zero in it** — `TASK-949` recorded that discipline and it is why its zero was believable.
4. ⛔ **EXPECT `434` DECLARATIONS / 32 FILES, and expect the executed figure to be `434 Success / 0 Fail`.** ⛔ **`434` is a DECLARATION CENSUS in every document you have been handed, ⛔ NOT a pass count — re-census at your instant, ⛔ do not reconcile to it** (`TL-§5b`).
5. ⚠️ **The compile risk on this diff is LOW and here is why, so you can weigh a failure correctly:** the new test adds **no `#include`**, introduces **no new API**, and every call it makes (`FindRow<FCardRow>`, `TestTrue/TestFalse/TestNotNull`, `FPackageName::DoesPackageExist`, `FString::Contains`, `GetNameSafe`, `StaticEnum<ECardType>`) **already exists in the compiled-and-executed sibling in the same file**. ⛔ **A compile failure here would therefore most likely be a UNITY-BUILD or macro-name collision, not an API error** — the new fixture class `FSiegeCardRosterAbsentCardIDNegativeControlTest` is unique across `Source/` (I censused it: 2 occurrences, 1 file) and both test-name strings differ.
6. ⛔ **If a card goes red that is NOT in `cards.csv`, that is a REIMPORT gap, ⛔ not a missing Blueprint — do not burn a QA loop on it** (`qa/TASK-948.md` `W-4`). ⭐ **It did not materialise at `09b9b50` and my own `cards.csv`↔disk join (§3) still balances 22/22 today.**

## 8.2 ⛔ NOTE FOR `TASK-959` (the path-composer extraction)

⭐ **Re-measured by me at my instant, since this is the number your row turns on:** `/Game/Blueprints` appears **9** times in `SiegeCardRosterTest.cpp`, ⛔ **all 9 above line 316**, ⛔ **ZERO in the new test (`:667`–`:815`)**. The composition still lives in exactly ONE function, `ComposeActorClassPath` (`:302`–`:321`). ⛔ **`TASK-964` added no sixth copy and your *"a sixth copy is a FINDING"* clause is not tripped.** ⚠️ **The file is now 817 lines with TWO test declarations — ⛔ your rewrite must re-point BOTH consumers, and the second one (`:763`/`:764`) calls the composer TWICE in one line pair.**

---

## 8.3 ⚖️ ⛔ NOT A RULING — ⛔ ONE MEASUREMENT THAT FELL OUT OF MY JOIN, ⛔ FOR `TASK-967`'s OWNER

⛔ **`TASK-967` is explicitly out of my scope (item (7)) and I am not adjudicating it.** ⛔ **But §3's name join answers its premise in one line, so it would be `SC-§29`-noncompliant to sit on it:** `handoffs/TASK-949-buildmaster.md` §7(2) flags *"14 unit Blueprints on disk vs 13 `CardType Unit` rows"*. ⛔ **The fourteenth is `BP_Unit_Miner`, whose row is `CardType` `Economy` (`cards.csv:5`) and which `ClassifyRow` routes down the UNIT path because it is NOT in `BuildingEconomyCardIDs`.** ⇒ **the sets join exactly 14↔14 and 8↔8 with zero orphans on either side.** ⛔ **Whether that fully closes `TASK-967` is ⛔ that row's call, ⛔ not mine, and this line ⛔ cannot change any verdict in this report.**

---

## 9. ⛔ `SC-§29` COVERAGE LEDGER

**THIS REPORT COVERS: ⛔ `TASK-964` ⛔ ALONE** — the single modified file `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` and `handoffs/TASK-964-programmer.md`.

**THIS REPORT DOES ⛔ NOT COVER:** ⛔ any compile, any suite execution, any Git state (⛔ **I hold none of the three**) · ⛔ `TASK-946`/`BP_Unit_Witch` as an asset · ⛔ `TASK-957` · ⛔ `TASK-959` as an implementation · ⛔ `TASK-962`/`963` · ⛔ `TASK-967` (§8.3 is a measurement, ⛔ not a ruling) · ⛔ the stack lane · ⛔ `Content/FogArea/**` (`TASK-927`) · ⛔ the `TASK-927` unaccounted staged set · ⛔ re-litigating the `default:` ruling (⛔ **ENFORCED, ⛔ not reopened**).

**READ-ONLY, MEASURED BY ME:** `Tests/SiegeCardRosterTest.cpp` (⛔ **all 817 lines**) · `SiegePlayerController.cpp` (`default:` control only) · `CardRow.h:17-25` · `Docs/Data/cards.csv` (⛔ **all 32 rows, typed and joined**) · `Content/Data/DT_Cards.uasset` (binary probe, both polarities) · `Content/Blueprints/Units/**` + `Buildings/**` (globbed, ⛔ **named and joined**) · `.gitattributes` · `.git/lfs/objects/b4/f3/…` · `TASKBOARD.md` `TASK-963`–`966` + `:17190` · `qa/TASK-948.md` (full) · `handoffs/TASK-964-programmer.md` (full) · `handoffs/TASK-949-buildmaster.md` (§1/§4/§7) · `CONVENTIONS.md` `SC-§51`–`SC-§55` · `SLACK.md` registry.

⛔ **NO FILE WAS EDITED except this report. ⛔ NO ROW WAS FLIPPED BY ME — ⛔ I hold no line-editing tool and I will ⛔ NOT whole-file `Write` a board taking concurrent writes; ⛔ the exact status lines were returned to the orchestrator.** ⛔ **NO GIT OPERATION. ⛔ NO COMPILE. ⛔ THE EDITOR WAS NOT TOUCHED AND NO MCP CALL WAS MADE** (⛔ it is wedged on a modal awaiting Jonathan; ⛔ **a zero from a wedged instrument is not a measurement**).

---

*qa-reviewer · 2026-09-03 · ⛔ read-only · ⛔ no shell, no Git, no compile, no editor, no MCP · gates `TASK-964` ALONE · ⛔ 0 blockers · method ruled ⭐ SYNTHETIC · `default:` labels ⭐ 0 (control 7) · `-Wswitch` sites ⭐ 2 · path-composer copies added ⭐ 0 · declarations ⭐ 434/32 (⛔ UNEXECUTED) · roster join ⭐ 22↔22, 0 orphans · ⛔ the compiled RED BAR remains OWED to `TASK-961`*
