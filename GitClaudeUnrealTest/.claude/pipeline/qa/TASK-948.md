# QA Report — TASK-948 — THE SPAWNABLE-ROSTER GATE

**Verdict: ✅ PASS — ⛔ 0 BLOCKERS** · 4 WARN · 5 NIT
**Gates:** `TASK-947` ⛔ **ALONE** (`SC-§29` ledger at §8)
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` (NEW, 546 lines, sole deliverable)
**Date:** 2026-09-03 · qa-reviewer · read-only

---

## 0. ⛔⛔ WHAT I COULD AND COULD NOT EXECUTE — ⛔ ABOVE THE VERDICT, ⛔ NOT IN A FOOTNOTE (`TL-§5c` cl. 5(a))

⛔ **I HAVE NO SHELL, NO COMPILE, NO EDITOR, NO MCP AND NO GIT.** My tool set is read/grep/glob/write + Slack. ⛔ **I did not compile this file and I did not run the suite.** Reporting a green I did not watch would be this batch's own defect one level up.

| | what it is | who owes it |
|---|---|---|
| ⛔ **DECLARED (the author's, unwatched by me)** | the file compiles · the test registers · the compiled test's own RED→GREEN | ⭐ **`TASK-949`** |
| ⛔ **DECLARED (the author's, unwatched by me)** | `git status --porcelain -- Source/` returning ⛔ ONE `??` line | ⭐ **`TASK-949`** item (4), in its own `git status` |
| ✅ **RE-DERIVED BY ME, BY HAND** (`TL-§5c` cl. 5(b)) | every count · every path · every API signature · every census · both controls · the whole derivation chain | this report, §1–§6 |

⇒ ⛔ **THE EXECUTION DUTY TRANSFERS BY NAME TO `TASK-949`** (`TL-§5c` cl. 5(c)). ⛔ **What remains unexecuted is named in §7 and it is not small.**

⚠️ **INSTRUMENT CAVEAT I APPLIED TO MYSELF:** the `Grep` tool renders some `//` comment lines with a leading `\`. `qa/TASK-950.md` §"A THIRD INSTRUMENT FAILURE" measured this already. ⛔ **I re-opened every such line at source (`SiegePlayerController.cpp:4596-4604` among them) and confirmed the file is `//` and correct. ⛔ I report no phantom blocker.**

---

## 1. ⛔⛔ ITEM (2) — THE VACUOUS-GREEN QUESTION. ⛔ THE BLOCKER-GRADE ONE. ⛔ ADJUDICATED: ⛔ **IT CANNOT PASS VACUOUSLY.**

⛔ **I did not take the author's word. I enumerated every path to a green bar over a zero-row walk and traced each one through the source.**

| # | route to a zero-row / truncated walk | what actually happens | line |
|---|---|---|---|
| 1 | CDO unreachable | `AddError` + **`return false`** | :311-316 |
| 2 | `CardTableAsset` renamed **or retyped** | `AddError` + **`return false`** | :319-325 |
| 3 | `CardTableAsset` unset on the CDO | `AddError` + **`return false`** | :327-332 |
| 4 | the table does not load | `AddError` + **`return false`** | :334-339 |
| 5 | table loads with the **wrong row struct** | `AddError` + **`return false`** | :341-346 |
| 6 | table loads, **0 rows** | `TotalRows>0` RED · `SpawnableRows>0` RED · **positive control RED ×2** | :490, :493, :511, :514 |
| 7 | every row `FindRow`-null | `AddError` per row **and** counters stay 0 ⇒ route 6 fires too | :383-387 |
| 8 | table loads, **all rows non-spawnable** | `SpawnableRows>0` RED · **positive control RED ×2** | :493, :511, :514 |

⇒ ⛔⛔ **THERE IS NO PATH TO A VACUOUS GREEN. ⛔ Item (2) is satisfied, and it is satisfied OVER-DETERMINED — the four self-checks pre-empt the most likely CAUSES of an empty walk before the walk begins, and each returns `false` rather than continuing.** ⭐ **That last property is the one that matters: an unreadable instrument and a clean roster do not produce the same bar here, which is the exact confusion `SC-§39` exists for.**

### ⛔ AND NOW THE PART THE BRIEF DEMANDED — ⛔ ARE THE FIVE TELLS REAL, AND ARE ANY TWO THE SAME TELL WEARING DIFFERENT NAMES?

⛔ **I checked each against the vacuity defect specifically, and the honest answer is NOT the one the handoff wrote.**

| assertion | line | fires on an EMPTY walk? | independent? |
|---|---|---|---|
| `TotalRows > 0` | :490 | ✅ yes | ⚠️ **NO — strictly subsumed.** `TotalRows==0 ⟹ SpawnableRows==0`, so it can **never fire alone.** It is a finer-grained **diagnostic message**, not a second tell. |
| `SpawnableRows > 0` | :493 | ✅ yes | ✅ **YES — the live vacuity tell.** Fires where `TotalRows>0` cannot (a table of 32 spells). |
| partition `Spawn+Excl == Total` | :496 | ⛔ **NO** — `0+0==0` **passes** | ✅ yes, but for a **different property** (a silently dropped row) |
| `ProbesExecuted == SpawnableRows` | :504 | ⛔ **NO** — `0==0` **passes** | ⚠️ **discriminates NOTHING today** — self-declared at :499-503 |
| `DistinctPaths == SpawnableRows` | :507 | ⛔ **NO** — `0==0` **passes** | ✅ yes, **different property** (a path collision) |
| **POSITIVE CONTROL walked** | :511 | ✅ **yes** | ✅ **YES, and it is the STRONGEST** — it demands a **specific named real row** was reached, not merely "some row" |
| **POSITIVE CONTROL resolved** | :514 | ✅ yes | ✅ yes — demands the probe returned **PRESENT** for that row |

- **[WARN W-1] `handoffs/TASK-947-programmer.md` §4 — *"Three independent tells for one vacuous-green"* is ⛔ OFF BY ONE, and the arithmetic is checkable.** Against **vacuity specifically** there are **TWO** independent tells (`SpawnableRows > 0`, and the two-part positive control), plus the four hard self-checks. `TotalRows > 0` **cannot fire without `SpawnableRows > 0` firing**, so it is one tell with two messages. Three of the five "count assertions" **pass at 0==0** and are not vacuity tells at all — they are (correctly, and correctly labelled in the handoff's own per-row table) tells for **other** properties. ⛔ **This changes NOTHING about the verdict — the property item (2) demands holds, twice over, with or without the redundant row.** ⚖️ **It is recorded because the claim is the thing the next reader inherits, and a reader who deletes `SpawnableRows > 0` believing `TotalRows > 0` covers it would delete the only live one.**

### ⭐⭐ AND THE NEGATIVE CONTROL'S REAL FUNCTION — ⛔ IT IS ⛔ NOT A VACUITY TELL, AND SAYING SO IS THE POINT

`TestFalse(... DoesPackageExist(BogusComposed.PackageName))` at :521 fires when the **probe goes blind and starts answering PRESENT for everything.** ⛔ **That is a failure mode NONE of the seven above can see** — a blind-present probe produces a perfect green on all 22 real rows. ⭐ **The handoff's §5 claim that this runs on every single green pass is TRUE and I verified it: it sits after the loop on the unconditional path, with no early return between it and `return true`.** ⇒ ⛔ **every green this file ever emits carries, inside itself, a live demonstration that the probe can still answer ABSENT.** ✅ **This is the best thing in the file.**

---

## 2. ⛔⛔ THE `TestEqual(SpawnableRows, 22)` REFUSAL — ⛔ RULED, AS INSTRUCTED. ⭐ **THE AUTHOR IS RIGHT. ⛔ NO LITERAL BELONGS HERE.**

⛔ **First, the claim, MEASURED rather than taken** (`SC-§40` cl. 9). I grepped the file for every candidate literal:

```
$ grep -n '\b(13|21|22|23|32|10)\b' Tests/SiegeCardRosterTest.cpp
25, 65, 76, 77, 78, 120, 157, 483, 484, 485      <- TEN hits, ⛔ ALL TEN inside COMMENTS
```
⇒ ⛔⛔ **ZERO literal counts in executable code.** ⭐ **The only place `22` appears near an assertion is :484, which is the author NAMING the shape he refused:** `` `TestEqual(SpawnableRows, 22)` would be the TASK-874 trap rebuilt inside the very gate written to close it. `` ✅ **Verified, not relayed.**

### ⚖️ MY RULING, AND ⛔ WHY THIS CASE DOES ⛔ NOT DIFFER FROM `TASK-874`

⛔ **A literal `22` would be a defect, and the argument is arithmetic rather than aesthetic:**

1. ⛔ **It would go red for the WRONG REASON.** Card #23 lands ⇒ this file fails **while the roster is perfectly healthy.** The obvious repair is to bump the number, and the number is bumped by the person **least likely to check the 23 rows.** ⇒ **`TASK-874`'s trap, verbatim, inside its own cure.**
2. ⛔ **It would buy ZERO discrimination it does not already have.** Everything a literal `22` would catch — a dropped row, a truncated walk, a skipped probe — is **already caught** by the partition, the probe count and `SpawnableRows > 0`, **without** a maintenance obligation.
3. ⭐ **AND THE DECIDING ONE, which is a property of THIS gate and not a general preference: this file's ENTIRE PURPOSE is to be correct about a roster NOBODY RE-READS.** A gate that must be hand-edited every time the roster grows is a gate that **will be hand-edited by someone who is not reading it**, which is the precise mechanism that shipped a 50-gold card that cannot spawn.
4. ⚖️ **`SC-§40` cl. 10 already governs it** — *relationships, not literals* — and `SC-§37` says the same from the other end: **measure the property, never restate the value.**

⛔ **`TASK-874` had to DELETE a hardcoded 13-name roster rather than bump it to 14. ⭐ This file was written so there is nothing to delete.** ✅ **The refusal is correct and it is the right call for the right reason.**

---

## 3. ⭐⭐ THE DERIVATION CHAIN — ⛔ EVERY LINK RE-DERIVED AT SOURCE, ⛔ NOT ONE RELAYED

### (a) ⛔⛔ THE LOAD-BEARING ONE: ⛔ IS IT ⛔ THE SAME `UDataTable` `ResolveCardRow` LOADS? ⭐ **YES. ⛔ MEASURED.**

| | the shipped path | the gate |
|---|---|---|
| source of the path | `ASiegePlayerController::CardTableAsset` (`.h:1697`, `UPROPERTY(EditDefaultsOnly)`, `TSoftObjectPtr<UDataTable>`) | ⛔ **the same member**, read off the CDO by reflection (`:319`) |
| the value | ctor `SiegePlayerController.cpp:212` ⇒ **`/Game/Data/DT_Cards.DT_Cards`** | ⛔ **never typed** — `ToSoftObjectPath()` off the CDO's own value (`:327`) |
| the load | `ResolveCardRow` `:4579` `CardTableAsset.LoadSynchronous()` | `:334` `TSoftObjectPtr<UDataTable>(CardTablePath).LoadSynchronous()` |
| the row read | `:4586` `FindRow<FCardRow>(…, bWarnIfRowMissing=false)` | `:380` **identical call shape** |

⇒ ⛔⛔ **SAME PATH ⇒ SAME PACKAGE ⇒ SAME `UDataTable` OBJECT. ⭐ THE GATE WALKS THE ASSET THE GAME LOADS, ⛔ NOT `cards.csv`.** ✅ **The handoff's claim — *"a row present in the asset but absent from `cards.csv` is still walked"* — is TRUE, and it is the difference between gating the game and gating a spreadsheet.** ⭐ **It also means this file does NOT inherit the reimport residual its neighbour `SiegeAssistantSelectionTest.cpp:187-194` declares. Confirmed by reading both.**

### (b) THE ECONOMY→BUILDING EXCEPTION — ⛔ REFLECTION, ⛔ TYPE-CHECKED, ⛔ NOT A SECOND COPY

`FindNameArrayField` (`:199-207`) checks the `FArrayProperty` **and** `Inner->IsA<FNameProperty>()` — ⛔ **the type, not just the name**, exactly as claimed. Miss ⇒ `AddError` + `return false` (`:353-357`). ✅ **The shipped list is `SiegePlayerController.h:1663` `= { FName(TEXT("DeepMine")) }` and the word `DeepMine` appears ⛔ NOWHERE in the test file.** ⭐ **This is the identical technique the shipped `SiegeAssistantSelectionTest.cpp:278-285` (TASK-874) already uses and which is in the executed 432 ⇒ the compile risk on this half is near nil.**

### (c) ⛔⛔ THE ABSENT `default:` — ⛔ VERIFIED, ⛔ CORRECT, ⛔ AND IT MUST BE PRESERVED

`ClassifyRow` `:224-245`. ⛔ **I read the enum myself: `CardRow.h:16-25` declares `UENUM(BlueprintType) enum class ECardType : uint8 { Unit, Building, Economy, Spell, HeroUpgrade, Utility }` — SIX enumerators. The switch names ALL SIX and carries ⛔ NO `default:` label.** ✅ **Confirmed at source, character by character.**

⛔ **AND I TRACED WHAT HAPPENS IF THE COMPILE-TIME HALF EVER FAILS TO FIRE, because that is the part the claim rests on:** an unnamed value falls past every `case` to the `Unclassified` initializer at `:222` ⇒ `:401` `AddError` + `continue` ⇒ **RED at run time**, ⛔ **and the row is counted as NEITHER spawnable nor excluded, so the partition assertion at `:496` goes red as a second, independent tell.** ⭐ **The author claims that pairing at `:403-405` and it is true.**

- **[WARN W-2] The *"compile-time `-Wswitch` failure"* half of the claim is ⛔ TOOLCHAIN-CONDITIONAL and I could not settle it without a compile.** Clang treats `-Wswitch` as on-by-default; MSVC's equivalent is the unhandled-enumerator warning, which is **not on at every level** and this project's `Build.cs` sets ⛔ **no** warning configuration (grepped: zero hits for `CppCompileWarnings`/`bWarningsAsErrors`/`4062` anywhere under `Source/`). ⇒ ⛔ **on the Win64/MSVC standing configuration a 7th enumerator may compile silently.** ⭐ **This does NOT weaken the mechanism — the runtime tell above fires either way, twice — but the handoff and the code comment state the compile-time half as unconditional, and `SHIP-§9c` cl. 3 says an unstated configuration is an untested one.**

- ⛔⛔ **MY RULING ON THE `default:` QUESTION THE AUTHOR ASKED (his §9.3): ⛔ NO. ⛔ DO NOT ADD ONE. ⛔ A future edit adding `default: Category = ESpawnCategory::NotSpawnable;` would ⛔ SILENTLY SWALLOW every unrouted card type into the excluded bucket — ⛔ the partition would balance, the walk would look healthy, and a whole new card family would be invisible to the gate written to find exactly that.** ⭐ **It would disarm BOTH halves at once. The absent label is load-bearing and I am recording it here so the next reviewer meets this ruling instead of "fixing" it.**

### (d) THE BASE CLASSES — ⛔ SYMBOLS (`SC-§38`)

`ABuilding::StaticClass()` / `ASummonedUnit::StaticClass()` at `:267`/`:273`. ⛔ **No strings.** ✅

---

## 4. ⛔⛔⭐ THE HOLE `TASK-950` FOUND IN THE BRIEF — ⛔ **RULED: ⛔ CLOSED. ⛔ THE GATE PINS ALL THREE PROPERTIES.**

`qa/TASK-950.md` §6, verbatim: *"a BP that exists at the right path but has the wrong parent fails identically to an absent one, and my filesystem census CANNOT see it. ⭐ `TASK-947`'s test can and should assert the parent, not merely the path."*

⛔ **I traced the loop body line by line rather than trusting the header comment:**

| property | how it is pinned | line | vs. the shipped resolver |
|---|---|---|---|
| **1. path resolution** | `FPackageName::DoesPackageExist(PackageName)` — ⛔ correctly given a **LONG PACKAGE NAME**, not an object path | `:428` | — |
| **2. class load** | `TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous()` + `TestNotNull` | `:445-448` | ⛔⛔ **BYTE-IDENTICAL to `SiegePlayerController.cpp:4632`** |
| **3. PARENT** | `TestTrue(… ActorClass->IsChildOf(Composed.RequiredBase))` | `:455-456` | ⛔⛔ **the same predicate as `:4633`** |

⇒ ⛔⛔ **THE WRONG-PARENT WORLD IS EXCLUDED BY ASSERTION, ON EVERY SPAWNABLE ROW, INCLUDING THE WITCH — ⛔ and it is `TASK-948`'s own evidence, ⛔ NOT `TASK-946`'s.** ⭐ **`WITCH-§6`'s own ruling is the standard this must be judged against and it is met verbatim:** *"AN EXISTENCE CENSUS AND A CONTRACT TEST ARE NOT THE SAME GATE, AND THE CHEAPER ONE IS NOT THE ONE THAT PROVES THE CARD IS PLAYABLE."* ⛔ **This is a contract test.**

⭐ **AND IT CLOSES THE THIRD DOOR THE DIAGNOSIS COULD ONLY MARK *"Possible"*:** `handoffs/STACK-BUGS-diagnosis.md` §2 row (c) — *"Class loads but `!IsChildOf(RequiredBase)` — wrong parent class | Possible; not the case here."* ⛔ **`TestTrue(… IsChildOf …)` converts that from *"possible"* into *"measured on every run, forever."***

- **[NIT N-1] The scope sentence UNDERSTATES the gate, and it understates it on exactly the property `TASK-950` flagged.** The header (`:53-54`) and the handoff §6 both publish *"the asset the law names EXISTS at the path the law names"* — a pure **existence** claim. ⛔ **A future reviewer reading only the scope sentence would conclude the wrong-parent hole is still open. It is not.** Understating scope is the **safe** direction under `SC-§49` (the defect that law names is publishing at a **wider** scope than measured), so this is a NIT and not a WARN — but the sentence should gain *"…and derives the base class the shipped resolver requires"* on a future edit. ⛔ **No edit on this row. Recorded so `TASK-949` and the manager have the true scope in writing.**

---

## 5. ⛔ ITEMS (1), (3), (4), (5), (6) — RULED, EACH WITH MY OWN MEASUREMENT

### ITEM (1) — `SHIP-§9`: ⛔ IS THERE A RED TRANSCRIPT AND DOES IT NAME `Witch`? ✅ **YES, AND IT IS THE STRONGEST FORM AVAILABLE.**

⛔ **RUN 1, 19:00:57 — a FREE, REAL, UNSYNTHESISED RED, captured BEFORE `TASK-946` landed**, naming `Witch` explicitly with its composed path. ⛔ **I corroborated that the absence was genuine rather than staged, from three records I read myself:** `STACK-BUGS-diagnosis.md` §2.2 (*"21 of 22 present. Exactly one missing"*), `CONVENTIONS.md:8071` (*"`git log --all` on that path is EMPTY — it was never tracked, in any commit, ever"*, positive control `BP_Unit_Sorcerer → 80c47e8`), and `qa/TASK-950.md` §1(a) (an independent hand census reproducing 22/21/1 **to the unit**).

⭐⭐ **AND THE RARE PART: `TASK-946` LANDED MID-TASK AT 19:02 AND THE RED→GREEN TRANSITION WAS CAUGHT LIVE ON THE SAME INSTRUMENT, THE ONLY CHANGE BEING THE ASSET.** ⛔ **That is `SHIP-§9c` cl. 1 satisfied in its strongest available form — not *"it failed when I broke it"* but *"it failed, then the real repair landed and it passed."***

⛔ **RUN 3 — the discrimination proof on `ZzNoSuchCardZz`, and `SC-§40` cl. 10 is satisfied because the absence was MEASURED, not assumed** (0/0/0/0 against a `Sorcerer` positive control of 1/3/1/27). ⭐ **I RE-RAN THAT ZERO MYSELF WITH MY OWN INSTRUMENT AND ITS OWN POSITIVE CONTROL** (§5, ITEM 3 below).

### ITEM (3) — `SC-§39`: ⛔ BOTH CONTROLS. ✅ **PRESENT, AND I PROVED THE NEGATIVE ONE'S ZERO.**

- **POSITIVE — `Sorcerer` (`:154`):** asserted **walked** (`:511`) and **resolved** (`:514`). ✅ `Content/Blueprints/Units/BP_Unit_Sorcerer.uasset` **exists** — my own glob. ✅ `Sorcerer` is a `Unit` row in `cards.csv:31` — my own read.
- **NEGATIVE — `ZzNoSuchCardZz` (`:161`):** asserted **not a row** (`:517`) and its composed path **does not resolve** (`:521`).
  ⛔⛔ **I PROVED THE ZERO RATHER THAN QUOTING IT** (`SC-§39`, and the brief's *"prove every zero"*):
  ```
  $ grep -rc "ZzNoSuchCardZz" <repo>      ->  3 files, 11 occurrences:
        TASKBOARD.md:1 · Tests/SiegeCardRosterTest.cpp:3 · handoffs/TASK-947-programmer.md:7
     ⇒ ZERO in Content/**  ·  ZERO in Docs/Data/cards.csv  ·  ZERO in DT_Cards.uasset
  $ grep -c "Sorcerer" Content/Data/DT_Cards.uasset   ->  3   <- ⛔ THE INSTRUMENT'S OWN POSITIVE CONTROL
  ```
  ✅ **My instrument was shown able to return non-zero on the same binary before I trusted its zero.** ⭐ **And it agrees with the diagnosis's independent name-table probe (`Witch` 4 · `Sorcerer` 3 · bogus 0) — two instruments, two authors, same answer.**
- ⚠️ **The declared cost is real and correctly declared at `:150-152`: if `Sorcerer` is ever retired the control goes red and a new one must be chosen.** ⛔ **That is what a control costs and it must not be "fixed" by deleting it.**

### ITEM (4) — `SC-§49`: ⛔ IS THE SCOPE SENTENCE PRESENT ⛔ AND TRUE? ✅ **BOTH.**

- **PRESENT:** in the test's **own header** `:51-72` **and** in `handoffs/TASK-947-programmer.md` §6. ✅ Both, as required.
- **THE PREDICATE IS QUOTED VERBATIM** (`SC-§49` cl. 4(a)) at `:48-49`. ⛔ **I diffed it against `TASKBOARD.md:17177` character by character: identical.** ⛔ **No paraphrase. The paraphrase IS the defect and there isn't one.**
- **TRUE:** ✅ **I verified the composer-drift residual is REAL by reading both sides.** The gate composes its own strings at `:265-272`, **independently** of `ResolveCardActorClass`. ⛔ **If `SiegePlayerController.cpp:4612/4618` changed its folder, prefix or `_C`, this file would keep composing the CONVENTIONS path, keep finding the assets and STAY GREEN while every card failed to spawn.** ⛔ **The header says exactly that, in those terms, and does not soften it.**
- ⚖️ **The mitigating measurement is sound and I endorse it:** a composer drift breaks **every** card at once and is instantly visible; **one** absent asset breaks one card, is invisible to the 21 working ones, and **survived a QA declaration, a commit and a playtest.** ⛔ **The silent case is the one this row closes.** ⭐ **See §6 proposal 1 — the residual is bigger than the handoff measured, and that raises the value of the repair rather than lowering the verdict.**

### ITEM (5) — ⛔ ZERO SHIPPED `Source/**` EDITS ⇒ ⛔ NO `TASK-942` COLLISION. ⚠️ **NOT PROVABLE BY ME. ⛔ TWO INDEPENDENT INSTRUMENTS AGREE; ⛔ THE PROOF TRANSFERS.**

⛔ **I hold no Git. I cannot reproduce a `git status`, and I will not report one I did not run.** ⭐ **What I CAN measure, I did, with the instrument's own positive control:**

```
$ grep -rn "TASK-947" Source/                      -> 3 hits, ⛔ ALL in Tests/SiegeCardRosterTest.cpp
$ grep -rc "Siegebound.CardRoster|FSiegeCardRoster…" Source/  -> 4 hits, ⛔ ALL in the same ONE file
```
⇒ ⛔ **the row left no marker in any shipped `Source/**` file, and the instrument is proven able to return non-zero (it found 3 and 4).** ⛔ **This is corroboration, ⛔ NOT proof of a zero diff — a shipped file could have been edited without a marker.**

- **[WARN W-3] ⛔ THE ONLY BINDING CHECK IS `TASK-949` ITEM (4)'s OWN `git status`, AND IT MUST NOT ADOPT THE HANDOFF'S SPELLING.** `handoffs/TASK-947-programmer.md` §1 carries a live warning I verified is worth carrying forward: ⛔ **the git root is `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, ONE LEVEL ABOVE the project dir**, so repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix — ⛔ **and `TASK-949`'s `names:` block writes them WITHOUT it.** ⛔ **Build-master: confirm in your own `git status`; adopt neither spelling from prose** (`SC-§40` cl. 1).
- ⭐ **Structural corroboration, which is the strongest thing I have:** `TASK-942` is still `boarded` and **not dispatchable** (blocked on `TASK-941`), so `SiegePlayerController.{h,cpp}` has had **no competing writer**. And the deliverable is a **new file in its own unique namespace** — see N-2.

### ITEM (6) — `TL-§5b` / `TL-§5c`. ✅ **COMPLIANT. ⛔ AND THE RECONCILIATION BALANCES EXACTLY.**

⛔ **I ran the census myself at the mandated scope and needle** (`TL-§5b` 2/2c — ⛔ the **scope** is the load-bearing half):
```
scope  Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp   needle  ^IMPLEMENT_SIMPLE_AUTOMATION_TEST
   ->  433 across 32 files          (SiegeCardRosterTest.cpp contributes exactly 1, at line-start)
```
| | figure | source |
|---|---|---|
| last **EXECUTED** | **432 `Result={Success}` / 0 `Result={Fail}`** at `d101b1e` | ⛔ **not mine — declared, and I did not watch it** |
| author's declared delta | **`+1`** | handoff §8 |
| **my fresh census** | **433 across 32 files** | ⛔ **mine, this instant** |

⇒ ⛔⛔ **`432 + 1 = 433`. ⭐ THE `TL-§5b` cl. 3 EQUATION BALANCES TO THE UNIT ⇒ ⛔ NO UNDECLARED TEST FILE LANDED.**

- ✅ **`TL-§5c` cl. 1: the handoff writes `declared`, ⛔ never `N/N`, and says plainly it did not compile or run the suite.** ✅ **`TL-§5b` cl. 1: the two figures are published WITH their scope AND their file count and are explicitly labelled *"stale by construction — re-census, do NOT reconcile to it."*** ⛔ **They are therefore ⛔ NOT absolutes carried as expectations, and item (6) does ⛔ NOT fail.** ⭐ **This is the compliant shape, not a violation of it.**
- ✅ **`TL-§5d`: THE COMMIT HOST IS A ROW, NAMED, AND I VERIFIED IT ON THE BOARD.** `TASK-949` is a boarded `build-master` row whose item (4) pathspec **already lists `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` by name** (`TASKBOARD.md:17216`). ⛔ **Satisfies `TL-§5e` cl. 1 exhaustively: an ID, assignee build-master, `blocked-by` = the minimum set (this report's PASS).** ⛔⛔ **THE FILE IS UNTRACKED. ⛔ IF `TASK-949` DOES NOT TAKE IT, THE GATE DOES NOT EXIST AND NOTHING WILL EVER GO RED TO SAY SO.**

---

## 6. ⚖️ THE TWO PROPOSALS — ⛔ RULED, ⛔ NOT IMPLEMENTED. ⛔ **I RE-MEASURED BOTH AND BOTH CLAIMS UNDERSTATE THEIR OWN CASE.**

### ⭐⭐ PROPOSAL 1 — THE SHARED PATH-COMPOSER. ⛔ **RULED: IT SHOULD HAPPEN. ⛔ AND THE DRIFT SURFACE IS NOT THREE COPIES WIDE — ⛔ IT IS FIVE.**

⛔ **The handoff §7 measured THREE (controller · cheat manager · the test). ⛔ I censused the composition BY ITS SHAPE across all of `Source/` rather than taking the count, and found TWO MORE PRODUCTION SITES:**

```
$ grep -rn 'BP_Unit_%s|BP_Building_%s' Source/
  SiegePlayerController.cpp:4612  Buildings   <- ResolveCardActorClass      (shipped)
  SiegePlayerController.cpp:4618  Units       <- ResolveCardActorClass      (shipped)
  SiegeBotController.cpp:1647     Buildings   <- ResolveBotCardActorClass   (shipped)  ⛔ NOT COUNTED
  SiegeBotController.cpp:1648     Units       <- ResolveBotCardActorClass   (shipped)  ⛔ NOT COUNTED
  SiegeCheatManager.cpp:259       Units       <- SummonTestUnit             (shipped)
  Barracks.cpp:202                Units       <- the Barracks spawner       (shipped)  ⛔ NOT COUNTED
  Tests/SiegeCardRosterTest.cpp:266/272       <- the gate                              (new)
```

⇒ ⛔⛔ **FIVE SITES. ⛔ FOUR OF THEM ARE SHIPPED PRODUCTION SPAWN PATHS.** ⭐ **And the two the handoff missed are the two that matter most for the argument:**
- ⛔ **`SiegeBotController.cpp:1641-1655` is a near-verbatim CLONE of `ResolveCardActorClass` — same two format strings, same `IsChildOf` guard, same refusal log.** ⇒ ⛔ **a Buildings-folder rename silently breaks BOT placement on a path no player-side gate touches.**
- ⛔ **`Barracks.cpp:202` composes the unit path to spawn `SpawnCardID` (Footman) from a shipped BUILDING.** ⇒ ⛔ **a prefix drift breaks the Barracks spawner independently of everything else.**

⚖️ **DOES IT CHANGE THE CALCULUS? ⛔ YES, AND DECISIVELY — ⛔ BUT ⛔ NOT FOR THIS ROW.**
- ✅ **The refusal at boarding was CORRECT and I endorse it.** `TASK-947` item (4) fenced it, it is a `SiegePlayerController.cpp` write, and taking it would have serialised two independent lanes for no gain today. ⭐ **The author refused it, said so, and proposed it as a manager row — which is exactly the shape `SC-§27` asks for. ⛔ A self-serve rewrite here would have been a FAIL.**
- ⛔ **AND `SHIP-§9c` cl. 2 is the law that should carry the proposal:** *"PREFER THE PREDICATE THE CONSUMER ITSELF USES."* ⛔ **This gate uses an INDEPENDENT copy — which is precisely why it cannot see a composer drift.** ⭐ **The extraction converts it from asset-side-only into asset-side AND composer-side, and it now consolidates FOUR production consumers instead of two.**
- 📌 **FOR THE MANAGER, ⛔ NOT BOARDED BY ME:** the row is worth more than the handoff argued. ⛔ **It must land AFTER `TASK-942`** (same file), and its census must be the **shape** grep above, ⛔ **never the handoff's count of three** (`SC-§38`: the symbol is the key, and a hand-carried count rots).

### ⭐ PROPOSAL 2 — THE `T_CardArt_<CardID>` GATE. ⛔ **RULED: ⛔ ENDORSED, AND IT IS ⛔ STRONGER THAN THE HANDOFF ARGUED — ⛔ WITH ONE COST THE HANDOFF DID NOT NAME.**

⛔ **I verified all three legs at source rather than relaying them:**
1. ✅ **`cards.csv` carries `CardArt` as a real DATA COLUMN** (header field 24), e.g. `/Game/UI/CardArt/T_CardArt_Witch.T_CardArt_Witch` — and `CardRow.h:204` types it `TSoftObjectPtr<UTexture2D>`. ⇒ ⛔ **the gate would walk the AUTHORED reference, never a reconstructed one.**
2. ✅ **`UCardHandWidget::ResolveCardArtTexture` (`CardHandWidget.cpp:445-491`) degrades to text-only on `THREE` separate misses** — no row (`:453`), unset cell (`:468`), **unresolvable path (`:487`)** — each `return nullptr` with a once-per-CardID warning and ⛔ **never a crash.** ⇒ ⛔⛔ **`SC-§50` cl. 2's graceful-degrade-hides-the-gap pattern is LIVE ON THAT PATH, VERBATIM. ⭐ It is the Witch defect's exact shape on a second surface.**
3. ⭐⭐ **AND THE LEG THE HANDOFF DID NOT CLAIM, WHICH IS THE BEST ARGUMENT FOR IT: the consumer's own predicate IS `Row->CardArt.LoadSynchronous()` (`:474`) — ⛔ THERE IS NO COMPOSER TO DRIFT.** ⇒ ⛔ **a `CardArt` gate satisfies `SHIP-§9c` cl. 2 BY CONSTRUCTION and carries ⛔ NONE of the composer-drift residual this gate must declare.** ⛔ **On that axis it is strictly stronger than the row I am passing.**

⚠️ **THE COST NOBODY NAMED, AND IT MUST GO IN THE SPEC OR THE ROW SHIPS UNVALIDATED:** ⛔ **`qa/TASK-950.md` §1(b) measured `Content/UI/CardArt/*.uasset` = 32/32, one per CSV row, INCLUDING `T_CardArt_Witch`.** ⇒ ⛔⛔ **THAT GATE WOULD BE BORN GREEN. ⛔ THERE IS NO FREE REAL RED WAITING FOR IT, unlike this row, which had one sitting in the tree.** ⇒ ⛔ **`SHIP-§9` would have to be satisfied by SYNTHESIS — point it at a measured-absent row, show the red, revert, paste the transcript — exactly what RUN 3 did here.** ⛔ **A row boarded without that instruction produces a gate that has only ever been seen passing, which `SHIP-§9c` cl. 1 rules indistinguishable from no gate.**

📌 **BOTH ARE FOR THE MANAGER. ⛔ I report, I do not board** (`SC-§27`).

---

## 7. ⛔⛔ WHAT REMAINS UNEXECUTED — ⛔ NAMED, AS THE VERDICT REQUIRES

⛔ **The `SHIP-§9` evidence is unusually strong and I confirmed all three runs against the records. ⛔ BUT IT IS NOT WHAT IT WOULD BE IF THE SUITE HAD RUN, AND THE AUTHOR SAID SO ABOVE THE CLAIM RATHER THAN IN A FOOTNOTE** (`TL-§5c` cl. 5(a)) — ⭐ **which is the behaviour that law was bought to produce, and I am crediting it explicitly.**

⛔ **THE THREE TRANSCRIPTS ARE AN INDEPENDENT SHELL MIRROR OF THE PREDICATE. ⛔ THEY ARE NOT THE SUITE.** ⇒ ⛔ **NOTHING BELOW IS EVIDENCED BY THEM, AND `TASK-949` OWES EVERY LINE:**

1. ⛔ **THE FILE HAS NEVER BEEN COMPILED.** Every API in §9 below is verified against the installed UE 5.8 headers by hand; ⛔ **hand-verification is not a compile.**
2. ⛔ **THE COMPILED TEST HAS NEVER RUN. ⛔ There is no `Result={Success}` / `Result={Fail}` pair for it, and `433` is a DECLARED census — ⛔ not a pass count.**
3. ⛔⛔ **THE MIRROR AND THE COMPILED TEST DO NOT READ THE SAME ROSTER SOURCE, AND THIS IS THE SHARPEST GAP.** The shell mirror walked **`Docs/Data/cards.csv`**; the C++ walks **`DT_Cards.uasset`** (§3(a)). ⭐ **Choosing the asset is CORRECT — it is what the game loads — but it means the executed 32/22/10 is one degree removed from what the compiled test will count.** ⇒ ⛔ **if `DT_Cards` has drifted from the CSV, `TASK-949`'s first compiled run could produce a FALSE RED on a stale asset row.** ⚠️ **Build-master: if a card fails that is NOT in `cards.csv`, that is a REIMPORT gap, ⛔ not a missing Blueprint — do not burn a QA loop on it.**
4. ⛔ **`SHIP-§9c` cl. 3 — the configuration.** The gate has been exercised under a **shell mirror**, ⛔ **never under its own `EditorContext | EngineFilter` automation runner.** ⛔ **An unstated configuration is an untested one; this one is stated, and untested.**
5. ⛔ **THE `git status` PROVING ZERO SHIPPED `Source/**` EDITS** — §5 item (5), plus the git-root prefix trap in **W-3**.
6. ⛔⛔ **THE FILE IS UNTRACKED** (`TL-§5d`). ⛔ **`TASK-949`'s pathspec must take it or the gate does not exist.**

⇒ ⭐ **`TASK-949` item (1) asks for the compiled RED→GREEN as its integration check. ⛔ The transition is ALREADY OBSERVED ON THE PREDICATE and the author did not have to arrange it — ⛔ but the COMPILED half is still owed, and it is the half that proves the file this report passes actually builds.**

---

## 8. ⛔ `SC-§29` COVERAGE LEDGER

**THIS REPORT COVERS: ⛔ `TASK-947` ⛔ ALONE** — the single new file `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardRosterTest.cpp` and `handoffs/TASK-947-programmer.md`.

**THIS REPORT DOES ⛔ NOT COVER:** ⛔ `TASK-946` / `BP_Unit_Witch` itself (art skips QA; closed on its own row's read-back) · ⛔ the stack lane (`TASK-941`/`942`/`943`) · ⛔ `TASK-950`'s census (its own report) · ⛔ any compile, any suite execution, any Git state · ⛔ `DT_Cards`/`cards.csv` content · ⛔ the two proposals as implementations (⚖️ **ruled, ⛔ never boarded — rows are the manager's**).

**READ-ONLY, MEASURED BY ME:** `Tests/SiegeCardRosterTest.cpp` (full) · `SiegePlayerController.{h,cpp}` (`ResolveCardActorClass` `:4594`, `IsBuildingCard` `:4644`, `ResolveCardRow` `:4577`, ctor `:212`, `.h:1663`/`:1697` — **by symbol**, `SC-§38`) · `CardRow.h` · `SiegeBotController.cpp:1641` · `SiegeCheatManager.cpp:259` · `Barracks.cpp:202` · `CardHandWidget.cpp:445-491` · `Tests/SiegeAssistantSelectionTest.cpp` (house pattern) · `Docs/Data/cards.csv` (all 33 lines) · `Content/Data/DT_Cards.uasset` (binary probe) · `Content/Blueprints/**` (glob) · `CONVENTIONS.md` `SC-§49`/`SC-§50`/`TL-§5`–`§5e`/`SHIP-§9`/`WITCH-§6` · `TASKBOARD.md` `TASK-946`–`951` · `qa/TASK-950.md` · `handoffs/STACK-BUGS-diagnosis.md` §2 · UE 5.8 engine headers (`PackageName.h:450` · `Class.h:4519` · `UnrealType.h:811/2780/7418` · `AutomationTest.h:1985/2369/2459/2605`).

⛔ **NO FILE WAS EDITED except this report. ⛔ NO ROW WAS BOARDED. ⛔ NO GIT OPERATION. ⛔ THE EDITOR (PID 40528) WAS NOT TOUCHED AND NO MCP CALL WAS MADE.**

---

## 9. ⛔ ENGINE-API SURFACE — ⛔ EVERY SIGNATURE RE-VERIFIED AGAINST THE INSTALLED UE 5.8 HEADERS (⛔ not against the handoff)

| call | verified at | verdict |
|---|---|---|
| `FPackageName::DoesPackageExist(const FString&, FString*=nullptr, bool=true)` | `Misc/PackageName.h:450` | ✅ present, ⛔ **no deprecation** |
| `UClass::GetDefaultObject(bool=true)` → `UObject*` | `UObject/Class.h:4519` | ✅ ⛔ **not deprecated in 5.8** |
| `FindFProperty<T>(const UStruct*, const TCHAR*, EFieldIterationFlags=Default)` | `UObject/UnrealType.h:7418` | ✅ (`Default == IncludeSuper`) |
| `FObjectPropertyBase::PropertyClass` is `TObjectPtr<UClass>` ⇒ `.Get()` | `UnrealType.h:2780` | ✅ **the `.Get()` is correct and necessary** |
| `ContainerPtrToValuePtr<T>(UObject const*) const` → `T const*` | `UnrealType.h:811` | ✅ const overload exists |
| `TestNotNull(const FString&, const ValueType*)` → **`bool`** | `AutomationTest.h:2459` | ✅ returns bool ⇒ the `if (!TestNotNull(...))` at `:447` is valid |
| `TestEqual(const TCHAR*, int32, int32)` · `TestTrue/TestFalse(const FString&, bool)` | `:1985` · `:2605`/`:2369` | ✅ all present |
| `EAutomationTestFlags::EditorContext \| EngineFilter` | ⛔ **the house pattern** — identical in all 31 sibling test files | ✅ |
| `TSoftClassPtr<AActor>(FSoftObjectPath(…)).LoadSynchronous()` | ⛔ **byte-identical to shipped `SiegePlayerController.cpp:4632`** | ✅ strongest possible precedent |
| `StaticEnum<ECardType>()` · `GetNameStringByValue(int64)` | ⛔ **shipped precedent at `SiegeAssistantSelectionTest.cpp:257`** (in the executed 432) | ✅ |

⛔ **ZERO deprecated or removed APIs found.** ⚠️ ⛔ **This is hand-verification, ⛔ not a compile — see §7.1.**

---

## 10. FINDINGS

### ⛔ BLOCKERS — **NONE**

### ⚠️ WARN

- **[WARN W-1]** `handoffs/TASK-947-programmer.md` §4 — *"Three independent tells for one vacuous-green"* is **off by one**. `TotalRows > 0` (`:490`) is **strictly subsumed** by `SpawnableRows > 0` (`:493`) and can never fire alone; three of the five count assertions **pass at `0==0`** and are not vacuity tells at all (they are, correctly, tells for other properties). ⛔ **The property item (2) demands still holds, twice over.** **Fix:** none on this row — a future edit should say *"two independent tells plus four hard self-checks."* ⛔ **Do not delete `SpawnableRows > 0`; it is the live one.**
- **[WARN W-2]** `SiegeCardRosterTest.cpp:213-218` + handoff §2 — the ***"compile-time `-Wswitch` failure"*** claim is **toolchain-conditional** and unverifiable without a compile. This project sets **no** warning configuration (zero hits for `CppCompileWarnings`/`bWarningsAsErrors`/`4062` under `Source/`), and MSVC's unhandled-enumerator warning is not on at every level. ⛔ **The MECHANISM is unaffected — a 7th enumerator lands in `Unclassified` → `AddError` → RED, and the partition goes red as a second tell — but the comment states the compile-time half unconditionally.** **Fix:** soften to *"a compile-time failure under Clang; a hard run-time RED under any toolchain."* ⛔ **Not a defect in the gate.**
- **[WARN W-3]** ⛔ **FOR `TASK-949`, AND IT IS A LIVE TRAP:** the git root is **one level ABOVE** the project dir, so repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix — ⛔ **and `TASK-949`'s `names:` block writes them WITHOUT it.** **Fix:** derive the pathspec from your **own** `git status`; adopt neither the board's nor the handoff's spelling (`SC-§40` cl. 1).
- **[WARN W-4]** ⛔ **THE MIRROR READ `cards.csv`; THE COMPILED TEST READS `DT_Cards.uasset`** (§7.3). The C++ choice is **correct**, but it means a `DT_Cards`-vs-CSV reimport drift could produce a **false red** on `TASK-949`'s first compiled run. **Fix:** build-master — if a card goes red that is **not in `cards.csv`**, that is a **reimport gap**, ⛔ **not a missing Blueprint; do not route it back as a QA loop.**

### 📌 NIT

- **[NIT N-1]** `:53-54` + handoff §6 — the scope sentence **understates** the gate: it publishes a pure **existence** claim while the file also asserts the **class load** and the **parent** (`:445`, `:456`). ⛔ A reader of the scope sentence alone would conclude `TASK-950`'s wrong-parent hole is open. **It is closed.** Understating is the safe direction under `SC-§49`; **fix on a future edit**, not this row.
- **[NIT N-2]** ✅ **Unity-build safety — checked, and clean.** All nine helpers live inside `namespace SiegeCardRosterTestFixture`, which I censused as **unique across all 37 fixture namespaces** in `Tests/`; the `using namespace` at `:304` is **function-scoped**. ⛔ Relevant because the neighbouring `SiegeAssistantSelectionTest.cpp` declares a **same-named** `FindNameArrayField` — ⭐ **the namespace is what keeps them from colliding in one unity blob. Do not flatten it.**
- **[NIT N-3]** `:538` — the summary line says *"have NO actor Blueprint at their composed CONVENTIONS path"*, but `UnresolvedRows` also counts the **package-present-but-`_C`-failed-to-load** branch (`:450`). Two different defects, one sentence. **Fix:** *"…do not resolve to a loadable actor class."*
- **[NIT N-4]** `:447` — the assertion message *"an asset present but a `_C` that will not load is non-null in the editor and null at play time"* is confusingly worded and, read literally, inverted. The **assertion** is right; only the prose is muddled.
- **[NIT N-5]** ⛔ **A latent scope boundary worth one sentence in the header:** the roster is read off **`ASiegePlayerController`'s own CDO**. If a **Blueprint subclass** of the controller were ever introduced with an overridden `CardTableAsset`, this gate would walk the C++ default rather than the shipped value. ✅ **Measured today: no such BP exists** (glob of `Content/**/*PlayerController*.uasset` returns only the four untouched UE template variants). ⛔ **Currently inert by coincidence of content, not structurally impossible** — `TL-§5b` 2a's exact distinction.

---

## 11. ⛔ NOTES FOR BUILD-MASTER (`TASK-949`)

1. ⛔⛔ **YOU OWE THE ENTIRE §7 LIST. ⛔ THIS GATE HAS NEVER BEEN COMPILED AND ITS TEST HAS NEVER RUN.** ⛔ **`433` is `declared`, ⛔ NOT a pass count. ⛔ Re-census; do NOT reconcile to it.**
2. ⭐ **Your item (1) integration check is already evidenced ON THE PREDICATE (RED 19:00:57 → GREEN 19:02).** ⛔ **Report the COMPILED pair, and report BOTH states.**
3. ⛔ **`Build.bat` returns 0 on a FAILED build. ⛔ PARSE THE LOG for `Result: Failed`. ⛔ NEVER trust `$LASTEXITCODE`.**
4. ⛔⛔ **THE FILE IS UNTRACKED. ⛔ IF YOUR COMMIT DOES NOT TAKE IT, THE GATE DOES NOT EXIST** (`TL-§5d`).
5. ⚠️ **W-3 (the git-root prefix) and W-4 (a `DT_Cards` false red) are both aimed at you. Read them before you stage.**
6. ⛔ **Expect a `TestTrue` failure message naming a card ⇒ that is a MISSING ASSET, ⛔ never a reason to weaken the assertion** (`SC-§50` cl. 4, and the file says so at `:437`).

---

*qa-reviewer · 2026-09-03 · ⛔ read-only · ⛔ no shell, no compile, no editor, no MCP, no Git · gates `TASK-947` ALONE · ⛔ 0 blockers · positive control `Sorcerer` ✅ present in `cards.csv:31` + `BP_Unit_Sorcerer.uasset` + 3 hits in `DT_Cards.uasset` · negative control `ZzNoSuchCardZz` ✅ re-measured 0 across `Content/**`, `Docs/**` and every `Source/` file but the test itself, on an instrument shown able to return non-zero*
