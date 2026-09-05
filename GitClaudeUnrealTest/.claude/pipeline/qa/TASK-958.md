# QA Report — TASK-958 — THE CARD-ART ROSTER GATE

**Verdict: ✅ PASS — ⛔ 0 BLOCKERS** · 4 WARN · 6 NIT
**Gates:** `TASK-957` ⛔ **ALONE** (`SC-§29` ledger at §9)
**Subject:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp` (NEW, 775 lines + trailing, 1 test declaration, sole deliverable) + `handoffs/TASK-957-programmer.md`
**Date:** 2026-09-03 · qa-reviewer · read-only

---

## 0. ⛔⛔ WHAT I COULD AND COULD NOT EXECUTE — ⛔ ABOVE THE VERDICT (`TL-§5c` cl. 5(a))

⛔ **I HAVE NO SHELL, NO GIT, NO COMPILE, NO EDITOR AND NO MCP.** My tool set is read/grep/glob/write + Slack. ⛔ **I did not compile this file, I did not run the suite, and I could not run `git status` or `sha256sum`.** Reporting a green I did not watch would be this batch's own defect one level up.

⚠️ **`SC-§55` APPLIED TO MYSELF, FIRST THING.** My session-start `gitStatus` block is **Castle-era** (`MI_Castle_Interior_Crumble*`, `SM_Castle*`, `A ` staged entries, `TASK-626`/`629`–`634` handoffs). ⛔ **It is stale, it is not this batch, and I used not one byte of it.** Every zero below is from an instrument I ran at my own instant, with its own positive control on the same subject.

| | what it is | who owes it |
|---|---|---|
| ✅ **RE-DERIVED BY ME** | the `TASK-957` marker census + its positive control · the file-attribution ledger · the asset-path-literal census (0 vs 32) · the declaration census · the born-green bijection (32↔32, both directions, by name) · all 32 `cards.csv` `CardArt` cells · the consumer's predicate at source · the synthetic's absence in `DT_Cards` + its positive control · the WBP override zero + an **in-file** positive control · three LFS digests · the whole tell arithmetic · every engine signature | this report, §1–§7 |
| ⛔ **DECLARED (the author's, unwatched by me)** | the `git status` sweeps · the shell-mirror transcript · the `f050caf` endpoint of the delta | ⭐ **`TASK-961`** |
| ⛔ **STILL OWED BY ANYONE** | ⛔ **a compile, and a single executed run of this file — green or red** | ⭐ **`TASK-961`** — §7 |

⚠️ **INSTRUMENT CAVEAT I HIT AND CONTROLLED.** The `Grep` tool renders some `/**` doc-comment openers with a leading `\` (measured live on `CardHandWidget.h:279`), and it collapses binary matches to **matching lines**, not occurrences. ⛔ **I opened every load-bearing hit at source and I name the counting mode wherever I publish a binary figure** (`SC-§39.1`).

---

## 1. ⛔⛔ ITEM (6) — THE ATTRIBUTION QUESTION. ⛔ THE ONE THAT WOULD HAVE PRODUCED A **WRONG FAIL**. ⛔ ADJUDICATED: ⭐ **NOT `TASK-957`'s. ⛔ ZERO SHIPPED `Source/**` EDITS.**

⛔ **My spec item (6) calls a `SiegePlayerController.{h,cpp}` write a BLOCKER. That file IS dirty in the tree. ⛔ I did not take the handoff's word that it belongs to someone else — I measured the attribution three independent ways, and I state the ledger explicitly as the dispatch requires.**

### (a) ⛔ THE MARKER CENSUS, POSITIVE-CONTROLLED

```
grep -rn "TASK-957"  Source/
   -> 3 hits, ⛔ ALL THREE in Tests/SiegeCardArtRosterTest.cpp  (:23, :39, :87)

grep -rl "TASK-941|TASK-942|TASK-947|TASK-964"  Source/       <- ⛔ THE INSTRUMENT'S POSITIVE CONTROL
   -> 5 files: Building.h · ClimbableTower.h · Tests/SiegeBuildingStackTest.cpp
               Tests/SiegeCardRosterTest.cpp · Tests/SiegeCardArtRosterTest.cpp

grep -rc "SiegeCardArtRoster"  <repo>
   -> 3 files: TASKBOARD.md (2) · the new .cpp (4) · handoffs/TASK-957-programmer.md (5)
      ⛔ ZERO in any other Source/ file
```
✅ **The instrument is proven able to find a task marker inside a shipped header before I trusted its zero.**

### (b) ⭐⭐ THE DECISIVE ONE — ⛔ `SiegePlayerController.cpp` CONTAINS **NO CARD-ART CODE AT ALL**

```
grep -c "CardArt"  Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp   ->  ⛔ 0
grep -rc "CardArt" Source/                                                            ->  119 across 37 files
grep -c 'TEXT("/Game' Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp ->  32   <- same file, non-zero
```
⇒ ⛔⛔ **The word `CardArt` does not appear once in `SiegePlayerController.cpp`, on an instrument that returns 119 across the module and 32 for a different needle in that same file.** ⭐ **A card-art row cannot have edited a file that contains no card-art code.** ⛔ **This is a structural argument, not a marker argument, and it is the strongest one available to me.**

### (c) ⛔ THE POSITIVE ATTRIBUTION — ⛔ THE FILES BELONG TO THE **STACK LANE**

`Building.h:270` carries `(TASK-941, ruled as STACK-§10)` and `Tests/SiegeBuildingStackTest.cpp:1194` carries `TASK-941`. ⛔ **`TASK-942` is the live stacking row and the board names `Building.{h,cpp}` · `ClimbableTower.{h,cpp}` · `SiegePlayerController.cpp` as its files.**

### ⚖️ **MY EXPLICIT ATTRIBUTION LEDGER — AS THE DISPATCH DEMANDS**

| path | attributed to | how I attributed it |
|---|---|---|
| ⭐ `Tests/SiegeCardArtRosterTest.cpp` (`??`) | ⛔ **`TASK-957` — its SOLE deliverable** | 3/3 of the repo's `TASK-957` markers live here; unique namespace + class + test name (7 occurrences, 1 file) |
| `SiegePlayerController.cpp` (`M`) | ⛔ **NOT `TASK-957`** — the stack lane | ⛔ **`CardArt` = 0 in it** (control 119/32) · 0 `TASK-957` markers · board assigns it to `TASK-942` |
| `Building.{h,cpp}` · `ClimbableTower.{h,cpp}` (`M`) | ⛔ **NOT `TASK-957`** — the stack lane | `TASK-941` marker at `Building.h:270`; board assigns them to `TASK-942` |
| `Tests/SiegeBuildingStackTest.cpp` | ⛔ **NOT `TASK-957`** — the stack lane | `TASK-941` marker at `:1194`; holds 14 declarations (see `W-3`) |

⇒ ⛔⛔ **ITEM (6) DOES NOT FAIL. ⛔ `TASK-957`'s diff is EXACTLY ONE `??` UNTRACKED PATH.**

⚠️ **AND THE HONEST LIMIT, KEPT FROM `qa/TASK-948.md` §5: ⛔ a marker census is CORROBORATION, ⛔ not proof of a zero diff — a shipped file could be edited without leaving a marker.** ⛔ **The binding check is `TASK-961`'s own live `git status`, and `W-4` is aimed at it.** ⭐ **But (b) is not a marker argument, and (b) alone is sufficient for `SiegePlayerController.cpp`.**

📌 **FOR `TASK-961`, IN ONE LINE: build your pathspec from your OWN `git status` and do NOT sweep those five `M` paths in on `TASK-957`'s account.**

---

## 2. ⛔⛔ ITEM (1) — THE BORN-GREEN BLOCKER. ⛔ ADJUDICATED: ⭐ **TWO SYNTHESISED REDS. ⛔ EACH NAMES A CardID. ⛔ AND BOTH ARE ALSO PERMANENT.**

### 2.1 ⛔ FIRST — I RE-MEASURED BORN-GREEN MYSELF RATHER THAN INHERITING IT (`SC-§40` cl. 3)

⛔ **I did not take `TASK-950`'s 32/32 and I did not take `TASK-957`'s re-measurement either.**

| instrument (mine, this instant) | result |
|---|---|
| `Docs/Data/cards.csv` — **I read all 34 lines** | **32** data rows; field 24 is `CardArt`; ⛔ **0 empty cells** |
| every cell conforms to `/Game/UI/CardArt/T_CardArt_<CardID>.T_CardArt_<CardID>` | ⛔ **32/32**, 0 non-conforming |
| distinct `CardArt` paths | **32 of 32** |
| `Content/UI/CardArt/*.uasset` — **my own glob** | **32** |
| ⭐ **bijection by NAME, BOTH directions** | ⛔ **EXACT — 0 orphans either way**, including `T_CardArt_Witch` and `T_CardArt_Sorcerer` |
| `DT_Cards.uasset` binary — `Sorcerer` (positive control) | **3 matching lines** ⇒ ⛔ the instrument is not blind |

⇒ ⛔⛔ **32/32 PRESENT. ⛔ THERE IS NO FREE RED. ⭐ `SC-§51` cl. 6 BINDS, AND THE HANDOFF'S CENSUS IS CORRECT TO THE UNIT.**

⭐ **AND THE SCOPE CLAIM IT RESTS ON IS TRUE, MEASURED BY ME:** the CSV types are **13 Unit · 2 Economy · 7 Building · 5 Spell · 4 HeroUpgrade · 1 Utility = 32.** ⇒ ⛔ **the sibling gate's 22-row spawnable filter would leave TEN card faces ungated (5 Spell + 4 HeroUpgrade + 1 Utility).** ⛔ **The wider denominator is not scope creep — it is the correct denominator, and the file says so at `:96`–`:101`.**

### 2.2 ⛔ THE TWO REDS — ⛔ BOTH PRESENT, ⛔ BOTH NAME `Witch`

`handoffs/TASK-957-programmer.md` §3.2, verbatim transcript:
- ⛔ **RUN 2 = RED**, `FAIL card 'Witch' — its DT_Cards CardArt cell is UNSET` ⇒ **finding (a)**, names a CardID ✅
- ⛔ **RUN 3 = RED**, `FAIL card 'Witch' — CardArt '…T_CardArt_WitchQqSyntheticAbsentQq' does NOT resolve` ⇒ **finding (b)**, names a CardID ✅
- ⭐ **RUN 4 = the harness self-control**, printing both `PASS SYNTHETIC` and `FAIL SYNTHETIC` ⇒ ⛔ **RUN 2/3's reds are measured failures, not a reporter that can only print one word** (`SC-§39`)
- ⭐ **RUN 1 = GREEN on the shipped polarity**, plus a discrimination line proving one probe returned two different answers in one run

⇒ ⛔ **TWO REDS. ⛔ EACH NAMES A CardID. ⛔ ITEM (1)'s FLOOR IS MET.**

⚠️ **IT IS A SHELL MIRROR, ⛔ NOT THE COMPILED SUITE — and the author says so ABOVE the claim** (§0, and again in §3.2's opening line: *"DECLARED AS A SUBSTITUTE FOR THE COMPILED RUN, NOT AS IT (`SC-§54` cl. 4)"*). ⛔ **That is `SC-§54` cl. 4's required shape exactly: declared AS a substitute, in the same breath as the claim and above it, never folded into it.**

⚖️ **AND THE RULING MUST BE CONSISTENT WITH ITS SIBLING: `qa/TASK-965.md` §4.1 ruled the identical situation NOT a blocker, on the ground that `TASK-964`'s own spec forbade the only instrument that could produce a compiled observation.** ⛔ **`TASK-957` item (7) fences *"any compile · any editor/MCP"* in the same words.** ⇒ ⛔ **Failing this row for not executing what its own spec forbade would punish the exact honesty `SC-§54` was written to buy, and would teach the next agent to fold the substitute into the claim. ⛔ NOT A BLOCKER.**

### 2.3 ⭐⭐ AND THE HALF THAT IS BETTER THAN THE TRANSCRIPT — ⛔ **THE REDS ARE PERMANENT, AND THE INVERSION IS REAL. ⛔ I VERIFIED IT LINE BY LINE.**

⛔ **The dispatch's warning — *"a control that turns the suite red is a bug wearing a control's clothes"* — is the thing I checked hardest. It is clean.**

| state | line | input | asserted outcome | polarity |
|---|---|---|---|---|
| **C (control)** | `:683`–`:688` | the donor **untouched** | `Resolved`, texture **non-null** | shipped |
| **A** | `:693`–`:702` | `CardArt` **cleared** on a **stack copy** | ⭐ asserts `UnsetCell` + `TestNull` | ⛔ **INVERTED — asserts the MISS** |
| **B** ⭐⭐ | `:714`–`:727` | `CardArt` → **derived, measured-absent** path on a **stack copy** | ⭐ asserts `UnresolvablePath` + `TestNull` | ⛔ **INVERTED — asserts the MISS** |

⛔ **I censused every `AddError` in the file and opened all six:** `:411` `:420` `:427` `:434` `:440` (five self-checks, each followed by `return false`), `:497` (`FindRow` null — a real degrade branch), `:534` (real unset finding), `:559` (real unresolvable finding), `:663` (donor self-check). ⛔ **NOT ONE fires on an expected miss.** ✅ **The synthesis block is GREEN *because* the predicate answered MISS. ⛔ It will not turn `TASK-961`'s suite red.**

⛔ **The bogus path is DERIVED, never composed** (`:710`–`:712`): the donor's own `GetLongPackageName()` + `GetAssetName()` + the suffix ⇒ same folder, same prefix, guaranteed well-formed, **no path literal enters the file**, and the control follows the art folder if it moves. ⭐ **Verified at source.**

### 2.4 ⭐ THE SYNTHETIC'S ABSENCE — ⛔ RE-PROVEN BY ME, WITH A POSITIVE CONTROL ON THE **SAME** INSTRUMENT

```
grep -rc "QqSyntheticAbsentQq" <repo>  -> 9 hits / 2 files: the new .cpp (4) + the handoff (5)
                                          ⛔ ZERO in Content/**, ZERO in Docs/**, ZERO in every other Source/ file
grep -c  "QqSyntheticAbsentQq"  Content/Data/DT_Cards.uasset  -> ⛔ 0
grep -c  "Sorcerer"             Content/Data/DT_Cards.uasset  ->    3   <- ⛔ THE SAME BINARY, NON-ZERO
```
✅ **The zero is measured on an instrument shown able to return non-zero on the same subject.** ⭐ **And the substring hazard that nearly caught `TASK-964` is genuinely dodged: `QqSyntheticAbsentQq` is a substring of nothing in the tree and nothing in the tree is a substring of it** — its only 9 occurrences are its own declarations. ⛔ **The `ZzNoSuchCardZz` / `ZzNoSuchCard` trap (22 hits / 25 hits, same 5 files) does not recur here.**

---

## 3. ⛔⛔ ITEM (2) — WAS `DT_Cards.uasset` OR `cards.csv` WRITTEN ON DISK? ⛔ ADJUDICATED: ⭐ **NO — AND THE FILE IS STRUCTURALLY INCAPABLE OF IT.**

⛔ **I hold no Git and I will not report a `git status` I did not run.** ⭐ **What I could measure, I did — five ways, and one of them is stronger than a `git status`:**

**(a) ⭐⭐ THE CODE CANNOT WRITE THE ASSET, AND THAT IS A STRUCTURAL FACT RATHER THAN AN OBSERVATION.** `CardTable` is `const UDataTable* const` (`:431`); `DonorRow` is `const FCardRow* const` (`:667`); both syntheses are **by-value stack copies** — `FCardRow SyntheticUnsetRow = *DonorRow;` (`:693`) and `FCardRow SyntheticBogusRow = *DonorRow;` (`:714`). ⛔ **There is no non-const handle to the table anywhere in the file, no `MarkPackageDirty`, no `SavePackage`, no `IFileManager`, no rename and no move.** ⇒ ⛔ **`SC-§54` cl. 3 is satisfied BY METHOD, and the method is enforced by the type system.**

**(b) ⛔ NO RESIDUE OF A SYNTHESIS ANYWHERE ON DISK.** `QqSyntheticAbsentQq` = **0** in `Content/**`, **0** in `Docs/**`, **0** in `DT_Cards.uasset` (positive control `Sorcerer` = 3 on the same binary). ⛔ **An imperfectly-reverted disk edit would have left exactly that residue. There is none.**

**(c) ⛔ `cards.csv` IS INTACT — I READ ALL 34 LINES.** 32 rows, all `CardArt` cells set and conforming, `Witch` at `:33` and `Sorcerer` at `:31` both present and correct.

**(d) ⛔ `Content/UI/CardArt/` HOLDS EXACTLY 32 FILES** in an exact by-name bijection with the 32 rows, **zero orphans in both directions.** ⛔ **Nothing was moved aside, stubbed or renamed.**

**(e) ⭐ THE DIGESTS, CORROBORATED FROM AN INSTRUMENT THE AUTHOR DID NOT USE.** `*.uasset` is LFS-tracked ⇒ an LFS object is named by the sha256 of its own content. I globbed the store at the **repo root** (one level above the project dir):
```
.git/lfs/objects/ed/ff/edffce064bff1a510c62e1f9b9720a05a16c4b1c5df51fe267a28eb10810252a  <- DT_Cards.uasset        EXISTS
.git/lfs/objects/cf/c6/cfc6baea25e75f7ed56c107b07abc4faeed9945d90962b46cc55970520f8e761  <- T_CardArt_Sorcerer     EXISTS
.git/lfs/objects/b4/f3/b4f375305aea3ea812e25fb0e30bfabd39f82d21a64fe52b02298c3223fec2ab  <- BP_Unit_Witch.uasset   EXISTS
```
⭐⭐ **THE WITCH DIGEST IS CHARACTER-FOR-CHARACTER the one `TASK-949` recorded at four points before the refusal, the one `TASK-964` re-read, and the one `qa/TASK-965.md` §1(a) corroborated in this same store.** ⇒ ⛔⛔ **THE REFUSED MOVE STILL HAS NOT HAPPENED — NOT UNDER THIS TASK ID AND NOT UNDER ANY OTHER** (`SC-§54` cl. 1). ⭐ **This is the oid-vs-sha256 discipline, not a size check** — and the dispatch was right that it is the correct check.
⚠️ **Its honest limit, stated: an LFS object's presence proves that content was committed at some point; it is CONSISTENT with an unmodified worktree file but is not proof of one.** ⛔ **Combined with (a)–(d) it is decisive; alone it would not be.**

**(f) ⭐ AND THE RUN-TIME TELL.** `:738` asserts `DonorRow->CardArt.ToSoftObjectPath() == PositiveControlArtPath` **after** both syntheses, against a snapshot copied at `:527`. ⛔ **It fires alone if the synthesis ever starts mutating the loaded DataTable — the only tell for `SC-§54` cl. 3 compliance at run time, and it is real.**

### ⚖️ ⛔ THE ONE SCOPED-RATHER-THAN-ABSOLUTE CLAIM — ⛔ RULED: ⭐ **HONEST, NOT AN ESCAPE HATCH.**

The author declares (handoff §1, and again in the file at `:646`–`:652`) that `LoadSynchronous()` warms `FSoftObjectPtr`'s **`mutable`** weak cache, so the walk does mutate that cache on the loaded rows in memory. ⛔ **I verified it at source: `SoftObjectPtr.h:82` `LoadSynchronous()` calls `ToSoftObjectPath().TryLoad(...)` and caches on a `mutable` member.**
⚖️ **RULING: the scoping is HONEST.** It changes no serialized value, does not dirty the package, never reaches disk, and it is **exactly what the shipped widget does on every hand refresh** (`CardHandWidget.cpp:474`). ⛔ **The claim about the ASSETS AND THE INDEX is absolute and it holds.** ⭐ **Declaring a caveat nobody would have found, at the synthesis site AND in the handoff, is the behaviour `TL-§5c` cl. 5(a) exists to buy — I am crediting it, not penalising it.**

⇒ ⛔ **ITEM (2) SATISFIED. NOT A BLOCKER.** ⚠️ **The binding `git status` half is not mine and I do not claim it — `W-4`.**

---

## 4. ⛔ ITEM (3) — THE TELL ARITHMETIC. ⛔ AUDITED WITH `SC-§51` cl. 2's STRICT TEST, ⛔ NOT ACCEPTED. ⭐ **THE PUBLISHED NUMBERS ARE CORRECT.**

⛔ **I traced every subsumption claim through the control flow rather than reading the ledger.**

### LEDGER (a) — VACUITY. ⛔ **CLAIMED: ONE. ⛔ MEASURED: ONE. ✅ AGREED.**

| assertion | line | my trace of "can it fire ALONE?" | verdict |
|---|---|---|---|
| `TotalRows > 0` | `:580` | `TotalRows == 0` ⇒ loop never assigns ⇒ `bPositiveControlWalked/Set/Resolved` all stay `false` ⇒ `:621`/`:624`/`:627` all fire | ⛔ **subsumed** |
| `ProbesExecuted > 0` | `:583` | ⛔ **there is NO `continue` between `++TotalRows` (`:501`) and `++ProbesExecuted` (`:515`)** ⇒ the two are equal by construction today ⇒ same chain | ⛔ **subsumed** |
| `CONTROL WALKED` | `:621` | `bPositiveControlSet` is only assigned inside `if (bIsPositiveControl)` ⇒ not walked ⇒ stays `false` ⇒ `:624` fires | ⛔ **subsumed** |
| `CONTROL SET` | `:624` | an unset control cell `continue`s at `:536` **before** `:552` ⇒ `bPositiveControlResolved` stays `false` ⇒ `:627` fires | ⛔ **subsumed** |
| ⭐ **`CONTROL RESOLVED`** | `:627` | ⭐ **control walked, cell set, texture will not load** — the other four all pass in that world | ✅ ⛔ **THE LIVE ONE** |

⭐⭐ **AND THE DIFFERENCE FROM THE SIBLING'S *TWO* IS STRUCTURAL, NOT STYLISTIC — ⛔ I CHECKED IT RATHER THAN ACCEPTING THE EXPLANATION.** `qa/TASK-948.md` `W-1` found `SpawnableRows > 0` live because the sibling **has a type filter**, so a table of 32 Spells makes `SpawnableRows > 0` fire where `TotalRows > 0` cannot. ⛔ **This file has NO filter** (verified: `:472` `GetRowNames()`, `:490`–`:565` no `CardType` guard anywhere in the loop) ⇒ ⛔ **that separation genuinely does not exist here.** ✅ **Publishing "two" would have been copying the sibling's homework, and the author said so.**

### LEDGER (b) — INSTRUMENT CONTROLS (`SC-§39`). ⛔ **CLAIMED: THREE. ⛔ MEASURED: THREE. ✅ AGREED.**

| control | line | the world in which it ALONE fires — **my trace** |
|---|---|---|
| POSITIVE | `:627` | as ledger (a). ⚠️ **The same assertion as (a)'s live member — the author states the two ledgers touch here rather than hiding it. Correct disclosure** (`SC-§51` cl. 4) |
| SYNTHESISED (a) | `:699`–`:702` | ⛔ **the `IsNull()` branch is "simplified" away while the loader still works**: a cleared cell then classifies `UnresolvablePath` ⇒ `:701` fires while all 32 rows pass, `UnsetCells == 0` passes, and `:726` passes. ✅ **independent** |
| ⭐⭐ SYNTHESISED (b) / NEGATIVE | `:724`–`:727` | ⛔⛔ **`LoadSynchronous` gone BLIND, answering non-null for everything**: every row `Resolved`, every count healthy, every partition balanced, ledger (a) silent, `:701` still passes (the `IsNull()` branch returns before the load) — ⛔ **and `:726` fires ALONE.** ✅ ⛔ **THE LOAD-BEARING ONE, AND IT IS THE ONLY ONE THAT SEES THAT WORLD** |

⇒ ⛔⛔ **`SC-§51` cl. 3 IS LIVE ON THIS FILE, EXACTLY AS THE HEADER CLAIMS: A FULL ROW SET AND A BLIND PROBE PRODUCE THE SAME GREEN, AND ONLY `:726` DISTINGUISHES THEM.** ✅ **Item (4) satisfied — both controls exist and the negative one is identified as load-bearing, at its site (`:205`–`:211`, `:724`) and in the handoff.**

⛔ **DISCRIMINATION (`:734`) is correctly NOT counted.** ⛔ **I verified the subsumption:** if all three outcome assertions pass then `Resolved`/`UnsetCell`/`UnresolvablePath` are pairwise distinct by construction ⇒ `:736` cannot fire. ✅ **Subsumed, kept, not counted — `SC-§51` cl. 2 obeyed in both halves (not deleted, not counted).**

### LEDGER (c) — OTHER PROPERTIES. ⛔ **SIX, NONE COUNTED AS A VACUITY TELL. ✅ `SC-§51` cl. 1 OBEYED.**

`:589` probe-per-row (`0==0` ✓) · `:592` outcome partition (`0+0+0==0` ✓) · `:595` row-name coverage (`0==0` ✓) · `:608` art-path distinctness (`0==0+0` ✓) · `:466` card-table parity · `:738` non-destructiveness. ⛔ **None is claimed as a vacuity tell and each is named with the property it covers at its site.** (See `N-2` on the phrasing of the last two.)

### ⛔ AND THE QUESTION BEHIND ITEM (3): ⛔ **CAN IT PASS VACUOUSLY? ⛔ NO. ⛔ I ENUMERATED EVERY ROUTE.**

| route to an empty / truncated walk | what actually happens | line |
|---|---|---|
| widget CDO unreachable | `AddError` + **`return false`** | `:409`–`:413` |
| `CardTableAsset` renamed **or retyped** | `AddError` + **`return false`** (the lookup checks the **pointee class**, `:369`) | `:418`–`:422` |
| `CardTableAsset` unset on the CDO | `AddError` + **`return false`** | `:425`–`:429` |
| the table does not load | `AddError` + **`return false`** | `:432`–`:436` |
| wrong row struct | `AddError` + **`return false`** | `:438`–`:443` |
| controller CDO / property unreachable | `TestNotNull` RED + **`return false`** | `:453`, `:460` |
| table loads with **0 rows** | `:580` `:583` `:621` `:624` `:627` all RED, then `:660` RED + `AddError` + **`return false`** | — |
| every row `FindRow`-null | `AddError` per row **and** counters stay 0 ⇒ the row above fires too | `:497` |

⇒ ⛔⛔ **THERE IS NO PATH TO A VACUOUS GREEN. ⭐ An unreadable instrument and a healthy roster do NOT produce the same bar here — which is the exact confusion `SC-§39` exists for.**

---

## 5. ⚖️⛔ **THE DECLARED WIDENING — RULED. ⭐ IN SCOPE. ⛔ IT STAYS. ⛔ DO NOT REVERT IT.**

**What it is:** the roster is read off **`UCardHandWidget`'s CDO** (the card-art consumer) rather than the controller's as the dispatch said; the controller's is **also** read and the two are **asserted equal** (`:466`).

⚖️ **RULING — ⛔ IN SCOPE, AND ⛔ NOT MERELY TOLERATED BUT ⛔ CORRECT:**

1. ⭐⭐ **THE DECIDING REASON: THE ROW'S OWN TOP-CITED LAW REQUIRES IT.** `SHIP-§9c` cl. 2 — quoted in `TASK-957` item (1) and reproduced verbatim in the file header — says **prefer the predicate the CONSUMER itself uses.** ⛔ **The card-art consumer is `UCardHandWidget`, not `ASiegePlayerController`.** ⇒ ⛔ **reading the roster off the controller's CDO would have walked a table the card-face path does not necessarily use, in the one row whose entire thesis is *"use the consumer's own predicate."*** ⭐ **The author obeyed the LAW where the row's hint pointed elsewhere, and DECLARED the difference. That is `SC-§58` cl. 5's etiquette applied to a law-vs-hint tension: SURFACE the constraint, do not dissolve it.**
2. ⛔ **IT IS INSIDE THE ROW'S DECLARED SURFACE.** `TASK-957`'s own `names:` block lists **`CardHandWidget.cpp`'s `ResolveCardArtTexture` (⛔ by SYMBOL)** as read-only input. ⛔ **The widget was never out of scope.**
3. ✅ **IT COSTS NOTHING FENCED.** Zero shipped-file writes, zero `Content/**`, zero new API, zero new includes beyond ones already used in compiled sibling tests, and it lives entirely inside the row's SOLE permitted file.
4. ⭐ **IT WAS DECLARED, NOT SLIPPED IN** (`SC-§29`), with a stated two-line revert — the shape the pipeline is supposed to reward.
5. ⛔ **AND IT PINS SOMETHING REAL.** If the widget's and the controller's table paths diverged, the card FACE would read art from a different table than the SPAWN path resolves against, and **every existing gate would stay green.** ⛔ **I verified both literals at source: `CardHandWidget.cpp:23` and `SiegePlayerController.cpp:212`, byte-identical `TEXT("/Game/Data/DT_Cards.DT_Cards")`.**

⇒ ⛔⛔ **RULED IN SCOPE. ⛔ `TASK-961`: DO NOT REVERT IT, AND DO NOT LET A LATER TIDY-UP DO SO.**

⚠️ **BUT WITH `W-1` ATTACHED, AND IT IS THE SAME CLASS OF ERROR `qa/TASK-948.md` FOUND ON THE COMPOSER — see §8.**

---

## 6. ⛔⛔⭐⭐ **THE `SC-§56` VARIANT — ⛔ CONFIRMED, ⛔ MECHANISM VERIFIED AGAINST THE LAW'S OWN TEXT, AND ⛔ YES, THE SECTION NEEDS REWRITING.**

**The claim:** `git ls-tree -r <commit> --full-name -- <root-relative-path>` returns **EMPTY, exit 0, empty `stderr`.**

### 6.1 ⛔ THE MECHANISM — ⛔ CONFIRMED, AND IT IS A **DIFFERENT** MECHANISM FROM THE DOCUMENTED TRAP

⛔ **`--full-name` and `--full-tree` do two different things, and only one of them touches the pathspec:**

| flag | what it changes | effect on a root-relative pathspec run from the project dir |
|---|---|---|
| `--full-name` | ⛔ **the OUTPUT format only** — print paths relative to the repo root instead of the cwd | ⛔ **NONE.** The pathspec is still resolved against the **cwd** ⇒ `GitClaudeUnrealTest/Source/…` is sought at `GitClaudeUnrealTest/GitClaudeUnrealTest/Source/…` ⇒ ⛔ **matches nothing** |
| ⭐ `--full-tree` | ⛔ **operates as if run from the repo ROOT** — the **PATHSPEC** becomes root-relative. ⭐ **It IMPLIES `--full-name`** | ✅ **matches, and the output is root-relative for the follow-on `git show`** |

⇒ ⛔ **the author's three-variant table (A: 32 ✅ · B `--full-name`: 0 ⛔ · C `--full-tree`: 32 ✅) is exactly what git's semantics predict.** ✅ **CONFIRMED.**

### 6.2 ⛔⛔ **AND IT DEFEATS `SC-§56` cl. 3's REMEDY. ⛔ I CHECKED THE LAW'S OWN WORDS.**

`SC-§56` cl. 3 (read at source, `CONVENTIONS.md:4291`): *"a probe whose empty output is load-bearing must **capture `stderr`** or **check the exit status**. An unexamined `stderr` is where measurements go to become zeros."*

⛔⛔ **THAT REMEDY WORKS ONLY BECAUSE THE DOCUMENTED CASE — `git show <commit>:<path>` — EMITS `fatal:` AND EXITS NON-ZERO.** ⛔ **Here git is not failing.** A pathspec that matches nothing is a **legitimate empty result**: ⛔ **exit 0, `stderr` empty, answer confidently wrong.** ⇒ ⛔⛔ **there is nothing to capture and nothing to check. ⭐ ONLY cl. 5's POSITIVE CONTROL catches this one** — and it did: it handed the author a clean-looking `433 → 0` before it was controlled.

### 6.3 ⭐⭐ **THE STING IS REAL, AND I AM STATING IT PLAINLY: THE DOCUMENTED FIX CREATES A WORSE VARIANT OF THE TRAP.**

⛔ **`--full-name` is precisely what `TASK-964` added to fix the `git show` half.** ⇒ ⛔ **fixing the OUTPUT half of `SC-§56` re-arms the PATHSPEC half — and re-arms it in the SILENT form, which the documented detector cannot see.** ⛔ **This is the FIFTH occurrence of this family in one day and the FIRST that is invisible to the prescribed remedy.**

### ⚖️ **MY RULING FOR THE MANAGER (⛔ I report, ⛔ I do not edit law — `SC-§27`): ⛔ `SC-§56` NEEDS AMENDING, AND HERE IS THE CLEAVAGE LINE THAT MAKES IT GENERAL.**

⭐⭐ **THE RIGHT GENERALISATION IS NOT "git ls-tree is special" — IT IS THE FORM OF THE ARGUMENT:**

> ⛔ **`<commit>:<path>` is a TREE-ENTRY LOOKUP ⇒ a miss is an ERROR (`fatal:`, non-zero, LOUD).**
> ⛔ **`-- <pathspec>` is a FILTER ⇒ a miss is an EMPTY RESULT (exit 0, empty `stderr`, SILENT).**

⛔ **The same silent failure therefore also afflicts `git log -- <root-relative-path>` and `git grep … -- <root-relative-path>` run from the project dir** — which matters, because cl. 2 currently reassures the reader that *"`git log -- <relpath>` … resolve[s] against the CWD and **WORK[S]**."* ⛔ **It works for a cwd-relative path and returns a silent, confident, wrong zero for a root-relative one.** ⭐ **Three concrete amendments:**

1. ⛔ **cl. 3 must be SCOPED to the loud family** — *"capture `stderr` / check the exit status"* is the remedy for `git show` / `git cat-file`, ⛔ **and is INERT for every `--`-pathspec command.**
2. ⛔ **cl. 4 gains form (c): `git ls-tree --full-tree`** — with the warning in the same sentence that ⛔ **`--full-name` is NOT a substitute (it changes the OUTPUT, not the PATHSPEC), and combining `--full-name` with a root-relative pathspec is the silent variant.**
3. ⛔⛔ **cl. 5 is PROMOTED from a detection *duty* to THE PRIMARY remedy for the pathspec family** — ⛔ **because for that family it is the ONLY one.** ⭐ **cl. 6's *"treat every zero as guilty until controlled"* is the section's real load-bearing sentence and this variant proves it.**

📌 **`SC-§57` cl. 3's corroboration shape held again and I re-measured it independently: the card-art side is an exact 32↔32 by-name bijection with zero orphans in both directions** — the *absence* of the discrepancy that section was bought by.

---

## 7. ⛔⛔ WHAT REMAINS UNEXECUTED — ⛔ NAMED, AS THE VERDICT REQUIRES

⛔ **The evidence on this row is unusually strong for a born-green gate, and the author declared the shortfall ABOVE the claim rather than in a footnote.** ⛔ **BUT:**

1. ⛔⛔ **THE FILE HAS NEVER BEEN COMPILED.** Every API is hand-verified — mine as well as his (§8.5) — and ⛔ **hand-verification is not a compile.**
2. ⛔⛔ **THE TEST HAS NEVER RUN, IN ANY POLARITY.** There is no `Result={Success}`/`Result={Fail}` pair for `Siegebound.CardRoster.EveryCardRowResolvesItsCardArtTexture`, and it has never run under its own `EditorContext | EngineFilter` runner (`SHIP-§9c` cl. 3 — an unstated configuration is an untested one; this one is stated and untested).
3. ⛔ **THE TWO REDS ARE A SHELL MIRROR OF THE PREDICATE, NOT THE SUITE.** The mirror read `cards.csv` + the filesystem; the compiled test reads `DT_Cards.uasset`. ⭐ **Choosing the asset is CORRECT** (it is what the game loads) **but it is one degree removed** — the same `W-4` shape `qa/TASK-948.md` raised, and my own CSV↔disk join says it has still not materialised.
4. ⛔ **THE BINDING `git status`** proving `Content/Data/`, `Docs/Data/`, `Content/UI/` clean and only the one `??` under `Source/**` — §3, `W-4`.
5. ⛔⛔ **THE FILE IS UNTRACKED** (`TL-§5d`). ⛔ **`TASK-961`'s pathspec must take `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp` or the gate does not exist and nothing will ever go red to say so.**

---

## 8. ⛔ FINDINGS

### ⛔ BLOCKERS — **NONE**

### ⚠️ WARN

- **[WARN W-1]** ⛔⛔ **`SiegeCardArtRosterTest.cpp:141`–`:144` + `handoffs/TASK-957-programmer.md` §2 — the claim that `/Game/Data/DT_Cards` is *"written TWICE in shipped C++"* is ⛔ WRONG. ⛔ I MEASURED **NINE**.**
  ```
  grep -rn 'TEXT("/Game/Data/DT_Cards' Source/
    Building.cpp:76 · CardHandWidget.cpp:23 · DeckBuilderWidget.cpp:271 · DeckComponent.cpp:18
    HeroCharacter.cpp:143 · SiegeAssistantSnapshot.cpp:140 · SiegeBotController.cpp:215
    SiegePlayerController.cpp:212 · SummonedUnit.cpp:232                       ⇒ ⛔ NINE SITES
  ```
  ⛔ **The PARITY ASSERTION AT `:466` IS STILL CORRECT AND STILL WORTH HAVING — it pins the two copies the card-face-vs-spawn defect actually runs through.** ⛔ **What is wrong is the PROSE: it tells a future reader the DT_Cards drift surface is two wide and fully watched, when it is nine wide and 2/9 watched.** ⚖️ **This is the identical error class `qa/TASK-948.md` §7 found on the path composer (the handoff said THREE copies; the measured answer was FIVE) — ⛔ a hand-carried count, published as a fact, that rots and under-reports** (`SC-§38`). **Fix (comment-only, future edit):** *"…is written NINE times in shipped C++ as measured 2026-09-03; this assertion pins the two the card face and the spawn path use."* 📌 **FOR THE MANAGER, ⛔ NOT BOARDED BY ME: a `FSiegeCardDataStatics`-shaped constant for the card-table path is a `TASK-959`-sized row of its own, and it is NINE consumers wide, four of them AI/economy paths no player-side gate touches.**
- **[WARN W-2]** ⛔ **AN UNDECLARED FOURTH RESIDUAL (R4): THERE IS A ⛔ SECOND SHIPPED CARD-ART CONSUMER AND THE FILE'S RESIDUAL LIST DOES NOT MENTION IT.** `UDeckBuilderWidget::GetCardArtTexture` (`DeckBuilderWidget.cpp:715`–`:755`) has the **identical three-branch degrade** — `!Row` / `Row->CardArt.IsNull()` (`:729`) / `Row->CardArt.LoadSynchronous()` null (`:743`) — with its own once-per-CardID `WarnedCardArtIDs` warning and never a crash. ⭐ **The direction is SAFE: because the predicate and the data column are the same, this gate in fact covers the deck-browser faces too, so the published scope UNDERSTATES the gate** (`SC-§49`'s safe direction). ⛔ **But `UDeckBuilderWidget::CardTableAsset` (`:271`) is a THIRD independently-written table path that the parity assertion at `:466` does NOT include** ⇒ **if the deck builder's copy moved, the browser's card faces would break with this gate green.** **Fix (future edit):** add R4 to the header naming the second consumer, and either widen the parity assertion or declare the gap. ⛔ **Non-blocking; understating is the safe direction.**
- **[WARN W-3]** ⛔⛔ **THE DECLARATION CENSUS `435 / 33` IS ⛔ ALREADY STALE. ⛔ I MEASURE `439 / 33` AT MY OWN INSTANT.**
  ```
  scope  Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp   needle  ^IMPLEMENT_SIMPLE_AUTOMATION_TEST
     ->  ⛔ 439 across 33 files   (SiegeCardArtRosterTest.cpp contributes exactly 1, at line-start)
  ```
  | | figure | source |
  |---|---|---|
  | last **EXECUTED** | `433 Result={Success}` / `0 Result={Fail}` at `09b9b50` | ⛔ not mine — declared, unwatched by me |
  | `qa/TASK-965.md`'s census | 434 / **32** | another reviewer, earlier instant |
  | `TASK-957`'s declared endpoint | 435 / **33** | correct **at its instant** |
  | ⭐ **my fresh census** | ⛔ **439 / 33** | ⛔ mine, this instant |
  ⭐⭐ **THE FILE COUNT AGREES EXACTLY (33) ⇒ `TASK-957` ADDED EXACTLY ONE FILE, WHICH IS THE HALF THAT MATTERS FOR ITS CLAIM.** ⛔ **The `+4` in declarations landed in an EXISTING file and is the CONCURRENT STACK LANE's:** `Tests/SiegeBuildingStackTest.cpp` now holds **14** declarations and carries a `TASK-941` marker at `:1194`; it holds **zero** `TASK-957` markers. ⇒ ⛔ **NOT a `TL-§5b` violation and NOT an undeclared test file from this row** — it is `TL-§5b` cl. 3's *"stale before the ink dries"* happening live, exactly as that clause predicts. ✅ **`TL-§5c` cl. 1 is satisfied: the handoff writes `declared`, never `N/N`, and states in capitals that it did not compile or run the suite. Item (7) does NOT fail.** **Fix:** ⛔ **`TASK-961`: expect ~439+, ⛔ NOT 435 — re-census at YOUR instant and reconcile to nothing.**
- **[WARN W-4]** ⛔ **FOR `TASK-961`, AND IT IS A LIVE TRAP THREE WAYS.** ⛔ **(a)** The binding `git status` proving `Content/Data/` · `Docs/Data/` · `Content/UI/` clean and only ONE `??` under `Source/**` is **NOT provable by me** (no Git) — §3 is method + five measurements, not a status. ⛔ **(b)** Derive your pathspec from your OWN status: **the git root is one level ABOVE the project dir**, so repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix (`qa/TASK-948.md` `W-3`; I re-confirmed the layout by globbing `.git/HEAD` at the parent and finding no `.git` under the project). ⛔ **(c)** ⛔ **Do NOT trust your context's `gitStatus` snapshot** (`SC-§55` — mine was Castle-era with `A ` staged entries; **assume yours is too**). ⛔ **And do NOT stage the five stack-lane `M` paths on this row's account** (§1).

### 📌 NIT

- **[NIT N-1]** `:660`–`:665` — on an **already-red** run (the positive control's art genuinely fails to load) the SYNTHESIS PREMISE fires and **`return false`s before either permanent control executes.** ⛔ **This is CORRECT — the controls exist to validate a GREEN, and the load-bearing blind-probe world still reaches them (a blind loader makes `bPositiveControlResolved` true, so `:726` fires as designed).** ⛔ **Recorded so a future reader does not "fix" it by moving the synthesis above the walk, which would cost the single-variable A/B/C property the donor choice buys.**
- **[NIT N-2]** `:216`–`:221` — ledger (c) says all six *"each pass at `0 == 0`."* ⛔ **Four of them do and are count assertions; TABLE PARITY (`:466`) and NON-DESTRUCTIVENESS (`:738`) are not count assertions at all** — they pass on an empty roster for a different reason (nothing to disagree, nothing to mutate). ⭐ **The classification is RIGHT (neither is a vacuity tell); only the justification is loose.** **Fix:** *"…none of them can fire on an empty walk."*
- **[NIT N-3]** `handoffs/TASK-957-programmer.md` §3.4 publishes `QqSyntheticAbsentQq` as **0 files / 0 hits**. ⛔ **That was measured at `f050caf` BEFORE the file was authored; it now reads 9 hits / 2 files** (the test file's 4 declarations + the handoff's 5). ⭐ **The absence claim is still TRUE where it matters — 0 in `Content/**`, 0 in `Docs/**`, 0 in `DT_Cards.uasset` (control `Sorcerer` = 3 on the same binary), 0 in every other `Source/` file** — but a reader reproducing the "0/0" will conclude their instrument is wrong. **Fix:** date the figure and scope it, per `SC-§53` cl. 3.
- **[NIT N-4]** ⛔ **BINARY-PROBE COUNTING MODE, `SC-§39.1`.** My `DT_Cards.uasset` figures are **matching LINES** on a blob (`T_CardArt_` = 3 lines, `Sorcerer` = 3 lines), not occurrences; the handoff publishes **occurrence** figures (`T_CardArt_` 64, the mount+folder 32, `Sorcerer` 4). ⛔ **Both are non-zero where they must be and zero where they must be, so every control holds — but the two documents will look like they disagree about one asset.** ⚠️ **`qa/TASK-965.md` `N-3` recorded the identical hazard 500 lines apart in one file.** **Fix:** name the mode beside the number.
- **[NIT N-5]** ⭐ **A CREDIT AND A STRENGTHENING, NOT A DEFECT.** R3's claim (`:119`–`:125`) that `WBP_CardHand.uasset` carries no `CardTableAsset` override was positive-controlled against **`CardHandWidget.h`** — a *different file*. ⛔ **I ran a stronger one: on the SAME binary, `CardTableAsset` = 0 while `CardArt` = 8.** ✅ **The zero is confirmed on an instrument shown able to return non-zero on the very subject it is measuring.** **Fix:** adopt the in-file control on a future edit.
- **[NIT N-6]** The handoff §2 reports the file as **776 lines**; my reader shows content through `:775` with `#endif` at `:775`. Trailing-newline / counting-mode difference, ⛔ **no action.**

### ⭐ EXPLICITLY **NOT** FINDINGS — ⛔ RULED, DO NOT FLAG

- ⛔ **The duplicated `CardTypeSymbol` / `FindSoftObjectField` helpers.** ⛔ **RULED at boarding** — extracting a shared header would put this row inside a file `TASK-959` is rewriting; the identical ruling was made for `TASK-962` item (7) and `TASK-963` item (7). ✅ **I verified the namespace keeps them apart: `SiegeCardArtRosterTestFixture` / `FSiegeCardArtRosterTest` / the test-name string measure `7 occurrences, 1 file` across all of `Source/` ⇒ unity-build safe. ⛔ DO NOT FLATTEN THE NAMESPACE.**
- ⛔ **The absent `default:` in `CardArtOutcomeSymbol` (`:346`–`:355`).** ⭐ **CORRECT and deliberate** — all three enumerators named, no `default:` label, and an unnamed value falls to an explicit `<ECardArtOutcome value nobody named>` return rather than being folded into one of the three. ⛔ **`SC-§51` cl. 5 obeyed; do not "tidy" it.**

---

## 8.5 ⛔ ENGINE-API SURFACE — ⛔ EVERY SIGNATURE RE-VERIFIED BY ME AGAINST THE INSTALLED UE 5.8 HEADERS (⛔ not against the handoff)

| call | verified at | verdict |
|---|---|---|
| `TSoftObjectPtr<T>::LoadSynchronous() const` → `T*` | `SoftObjectPtr.h:547` | ✅ ⛔ **takes NO argument — byte-identical to the consumer's `CardHandWidget.cpp:474`** |
| `IsNull()` · `ToSoftObjectPath()` | `:592` · `:604` | ✅ |
| `TSoftObjectPtr& operator=(const TSoftObjectPtr&)` · `operator=(FSoftObjectPath)` | `:185`/`:186` · `:417` | ✅ **both used (`:694`, `:715`); `:417` is an exact match and wins over every conversion candidate — no ambiguity** |
| `FSoftObjectPath(const FString&)` — ⛔ **non-explicit** | `SoftObjectPath.h:68` | ✅ (and `:712` is direct-init, so it would compile even if explicit) |
| `GetLongPackageName()` · `GetAssetName()` · `IsNull()` · `operator!=` | `:252` · `:265` · `:342` · `:387` | ✅ all four used |
| `TestTrue(const TCHAR*/const FString&, bool)` → **bool** | `AutomationTest.h:2603`/`:2605` | ✅ **returns bool ⇒ `if (!TestTrue(...))` at `:660` is valid** |
| `TestNotNull(const TCHAR*/const FString&, const ValueType*)` → **bool** | `:2449`/`:2459` | ✅ **both overloads used (`:453`, `:669`) and both return bool** |
| `TestNull(const FString&, const void*)` → bool | `:2532` | ✅ |
| `TestEqual(const TCHAR*, const int32, const int32)` · `(const TCHAR*, const FString&, const FString&)` | `:1985` · `:1997` | ✅ **`:1997` binds by reference and beats the `FStringView` (`:1996`) and template (`:2186`) candidates — no ambiguity at `:466`** |
| `AddExpectedMessagePlain(FString, ELogVerbosity::Type, EAutomationExpectedMessageFlags::MatchType, int32)` | `:1796` | ✅ **the 4-arg form is unambiguous vs the 3-arg overload at `:1821`**; ⭐ **`Occurrences < 0` documented at `:1794` as *"silently ignored"* ⇒ `:678` CANNOT turn the suite red if the message never appears** |
| ⭐ **the same 4-arg call shape in the EXECUTED suite** | `SiegeCardHandKeyLabelTest.cpp:644` · `SiegeAssistantSelectionTest.cpp:1478/1679/1831/1946/2049/2969` | ✅ ⛔ **SEVEN compiled precedents — the only novelty is passing a `const TCHAR* const` variable instead of a `TEXT()` literal (identical type)** |
| `EAutomationTestFlags::EditorContext \| EAutomationTestFlags::EngineFilter` | ⛔ **439 occurrences across all 33 test files** | ✅ **the house pattern, including the 433 that compiled at `09b9b50`** |
| the three project includes (`Siegebound/CardHandWidget.h`, `CardRow.h`, `SiegePlayerController.h`) | already used, same spelling, in 6 compiled sibling test files | ✅ **include-path risk near nil** |
| `FindFProperty<FSoftObjectProperty>` + `PropertyClass.Get()` + `ContainerPtrToValuePtr<T>(const UObject*) const` | `UnrealType.h` | ✅ **byte-identical to the sibling, which compiled at `09b9b50`** |
| `UClass::GetDefaultObject()` · `StaticEnum<ECardType>()` · `GetNameStringByValue(int64)` | `Class.h` · shipped precedent | ✅ **not deprecated in 5.8** |

⛔ **ZERO deprecated or removed APIs.** ⭐ **AND THE ONE NON-OBVIOUS RISK, CHECKED AT SOURCE:** the deliberate failing load emits an engine **Warning**, and `FAutomationTestFramework::bElevateLogWarningsToErrors` (`AutomationTest.h:1610`, read via `ElevateLogWarningsToErrors()` at `:1921`) has ⛔ **ZERO project overrides — I grepped the whole repo and the only three hits are pipeline documents.** ⇒ ⛔ **a Warning cannot fail this test even undeclared, and it is declared anyway with `Occurrences = -1`.** ✅ **The `:678` line is safe in both directions.**
⚠️ ⛔ **THIS IS HAND-VERIFICATION, ⛔ NOT A COMPILE — §7.1.**

---

## 8.6 ⛔ NOTES FOR BUILD-MASTER (`TASK-961`)

1. ⛔⛔ **YOU OWE THE ENTIRE §7 LIST. ⛔ THIS FILE HAS NEVER BEEN COMPILED AND ITS TEST HAS NEVER RUN, IN ANY POLARITY.**
2. ⛔⛔ **THE FILE IS UNTRACKED. ⛔ IF YOUR PATHSPEC DOES NOT TAKE `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp`, THE GATE DOES NOT EXIST** (`TL-§5d`). ⛔ **Derive the spelling from your own `git status` — `W-4(b)`.**
3. ⛔⛔ **DO NOT STAGE THE STACK LANE'S FILES ON THIS ROW'S ACCOUNT.** `SiegePlayerController.cpp` · `Building.{h,cpp}` · `ClimbableTower.{h,cpp}` · `Tests/SiegeBuildingStackTest.cpp` are **`TASK-942`'s** (§1). ⛔ **An unaccounted staged path is REPORTED, ⛔ NEVER `git reset`** (`§25b` cl. R).
4. ⛔⛔ **`Content/**` MUST BE ABSENT FROM THIS LANE'S DIFF** — above all `Content/Data/DT_Cards.uasset`, `Docs/Data/cards.csv` and `Content/UI/CardArt/**`. ⛔ **If any appears, STOP AND REPORT: that would be the refused move, or a disk-written synthesis, having happened after all** (`SC-§54` cl. 3, and your row's own SECOND AMENDMENT RIDER).
5. ⛔ **`Build.bat` returns 0 on a FAILED build. ⛔ PARSE THE LOG for `Result: Failed`. ⛔ NEVER trust `$LASTEXITCODE`.** ⛔ **Check the log's SIZE and content before trusting any zero in it.**
6. ⛔ **EXPECT ~`439`+ DECLARATIONS / 33 FILES — ⛔ NOT `435`, and ⛔ NOT `433`.** ⛔ **Every figure you have been handed is a DECLARATION CENSUS at an expired instant, ⛔ not a pass count. Re-census at your own instant; reconcile to nothing** (`TL-§5b`, `W-3`).
7. ⭐⭐ **CAPTURE AND PASTE, VERBATIM, THE FIVE `AddInfo` LINES THIS TEST PRINTS** (`:744`, `:764`, `:766`, `:768`, `:770`) — they carry the row count, the per-type census and **both synthesised outcomes by name**:
   ```
   SYNTHESISED RED (a) — ResolveCardArt('Sorcerer' with a CLEARED cell) = UNSET CELL   [in-memory stack copy; no asset touched]
   SYNTHESISED RED (b) — ResolveCardArt('Sorcerer' -> '…QqSyntheticAbsentQq') = UNRESOLVABLE PATH   [in-memory stack copy; no asset touched]
   SYNTHESIS CONTROL      — ResolveCardArt('Sorcerer' UNTOUCHED) = RESOLVED
   ```
   ⛔ **If any two of those three read the SAME outcome, or if a line is absent from the log, the predicate is stuck or the block did not run — ⛔ that is a FINDING, not a formatting detail.** ⛔ **This is still NOT a compiled red bar; say so in your handoff rather than letting the log stand in for one.**
8. ⛔ **Expect a `TestTrue` failure naming a card ⇒ that is an UNSET CELL or a MISSING TEXTURE, ⛔ never a reason to weaken the assertion** (`SC-§50` cl. 4; the file says so at `:534` and `:559`). ⛔ **And an UNSET CELL and an UNRESOLVABLE PATH have DIFFERENT REPAIRS — the cell vs the asset. Do not collapse them.**
9. ⚠️ **The compile risk is LOW and here is why, so you can weigh a failure correctly:** every API has a compiled precedent in the executed suite (§8.5), the file adds **no new engine surface**, and the fixture namespace/class/test-name are **unique across `Source/`** (7 occurrences, 1 file). ⛔ **A failure here would most likely be a unity-build or macro collision, not an API error.**

---

## 8.7 ⛔ NOTE FOR `TASK-959` (the path-composer extraction) — ⭐ YOUR NUMBER, RE-MEASURED BY ME

```
grep -rc 'TEXT("/Game'  Source/
   Tests/SiegeCardArtRosterTest.cpp          ->  ⛔ 0     <- ⭐ ABSENT FROM THE RESULT SET ENTIRELY
   SiegePlayerController.cpp                 ->     32    <- ⛔ THE POSITIVE CONTROL
```
⛔⛔ **`TASK-957` ADDED ZERO ASSET-PATH LITERALS TO EXECUTABLE CODE. ⛔ THERE IS NO SIXTH COPY. ⛔ Your *"a sixth copy is a FINDING"* clause is NOT tripped by this row, and the `32` you will census in `SiegePlayerController.cpp` is intact.** ⭐ **The reason is structural, not disciplinary: the card-art path is a DATA COLUMN, and the file's one derived path (`:710`–`:712`) is built from the donor's own authored `GetLongPackageName()`/`GetAssetName()` rather than typed.**
📌 **But read `W-1`: the DT_Cards TABLE path is a separate, NINE-copy drift surface. ⛔ It is NOT yours on this row** (your row is `BP_Unit_`/`BP_Building_`), ⛔ **and I have not boarded it — it is the manager's.**

---

## 9. ⛔ `SC-§29` COVERAGE LEDGER

**THIS REPORT COVERS: ⛔ `TASK-957` ⛔ ALONE** — the single **new, untracked** file `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardArtRosterTest.cpp` and `handoffs/TASK-957-programmer.md`.

**THIS REPORT DOES ⛔ NOT COVER:** ⛔ **the concurrent STACK lane (`TASK-941`/`942`) and every file I attributed to it in §1 — `SiegePlayerController.cpp`, `Building.{h,cpp}`, `ClimbableTower.{h,cpp}`, `Tests/SiegeBuildingStackTest.cpp`** · ⛔ `TASK-947`/`948` and `TASK-964`/`965` as implementations (their own reports; I re-used their rulings only where consistency demanded it and said so) · ⛔ `TASK-959` as an implementation (§8.7 is a measurement, ⛔ not a ruling) · ⛔ `TASK-962`/`963` · ⛔ the card art ITSELF (art skips QA) · ⛔ `DT_Cards`/`cards.csv` CONTENT as a design question · ⛔ any compile, any suite execution, any Git state (⛔ **I hold none of the three**) · ⛔ the `SM_<CardID>` census (deferred, needs a CONVENTIONS ruling first) · ⛔ **editing `CONVENTIONS.md` — §6 is a REPORT to the manager, ⛔ not an amendment** (`SC-§27`).

**READ-ONLY, MEASURED BY ME:** `Tests/SiegeCardArtRosterTest.cpp` (⛔ **all 775 lines**) · `CardHandWidget.h:236`–`:285` + `CardHandWidget.cpp:445`–`:491` (`ResolveCardArtTexture`, ⛔ **by SYMBOL**, `SC-§38`) · `CardHandWidget.cpp:23` · `SiegePlayerController.cpp:212` + its `CardArt` and `TEXT("/Game` censuses · `DeckBuilderWidget.cpp:715`–`:755` · `Building.h:270` · `Tests/SiegeBuildingStackTest.cpp` (declaration + marker census) · `Docs/Data/cards.csv` (⛔ **all 34 lines, typed and joined**) · `Content/Data/DT_Cards.uasset` (binary probe, both polarities) · `Content/UI/CardArt/**` (⛔ **globbed, named and joined 32↔32**) · `Content/UI/WBP_CardHand.uasset` (binary, with in-file control) · `.git/lfs/objects/{ed/ff, cf/c6, b4/f3}/…` · `TASKBOARD.md` `TASK-957`–`963` · `qa/TASK-948.md` (full) · `qa/TASK-965.md` (full) · `handoffs/TASK-957-programmer.md` (full) · `CONVENTIONS.md` `SC-§51`–`SC-§58`, `TL-§5`–`§5b` (⛔ read at source, ⛔ not cited from memory) · UE 5.8 headers `AutomationTest.h` (`:1610` `:1790`–`:1821` `:1921` `:1985`–`:2003` `:2186` `:2449` `:2459` `:2530`–`:2532` `:2603`–`:2605`) · `SoftObjectPtr.h` (`:82` `:185`–`:186` `:417` `:547` `:592` `:604`) · `SoftObjectPath.h` (`:68` `:252` `:265` `:342` `:387`).

⛔ **NO FILE WAS EDITED except this report. ⛔ NO ROW WAS FLIPPED BY ME — ⛔ I hold no line-editing tool and I will ⛔ NOT whole-file `Write` a `TASKBOARD.md` under a live concurrent writer (4550 lines landed during `TASK-957`'s row); ⛔ the exact status lines were returned to the orchestrator.** ⛔ **NO GIT OPERATION. ⛔ NO COMPILE. ⛔ THE EDITOR WAS NOT TOUCHED AND NO MCP CALL WAS MADE** (⛔ it is wedged on a modal awaiting Jonathan; ⛔ **a zero from a wedged instrument is not a measurement**).

---

*qa-reviewer · 2026-09-03 · ⛔ read-only · ⛔ no shell, no Git, no compile, no editor, no MCP · gates `TASK-957` ALONE · ⛔ **0 blockers** · synthesised reds ⭐ **TWO**, each naming `Witch`, both also PERMANENT and INVERTED · shipped `Source/**` edits by this row ⭐ **0** (the five `M` paths ruled the STACK lane's, §1) · asset-path literals added ⭐ **0** (control 32) · born-green re-measured ⭐ **32↔32 exact bijection, 0 orphans both directions** · synthetic absence ⭐ **0 in `DT_Cards`** (control `Sorcerer` = 3, same binary) · Witch digest ⭐ **unchanged, LFS object present** · vacuity tells ⭐ **1** · instrument controls ⭐ **3**, negative load-bearing · declarations ⭐ **439/33 at MY instant** (⛔ UNEXECUTED) · widening ⚖️ **RULED IN SCOPE — do not revert** · ⛔ the compile and the first executed run remain OWED to `TASK-961`*
