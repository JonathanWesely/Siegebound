# TASK-944 — build-master handoff

> ## COMMIT `84eec02` — **the stack split is in `HEAD`. A WatchTower CAN now be stacked, to ×2.**
> `84eec02897857f3fd52f0185b13dfe5e69d8a849` · `main` **19 ahead** of `origin/main`, **NOT pushed.**
> Base was `f050caf` (**verified myself** — `SC-§55`; the relayed "18 ahead" was correct at my instant).

**Gate honoured:** `qa/TASK-943.md` = **PASS, 0 blockers.** Nothing was committed before I read it.

---

## 1. THE BUILD — `Result: Succeeded`, PARSED, NOT INFERRED

Verbatim, line 49 of the build log:

```
Result: Succeeded
```

- **Exit code was `0` and I did not use it** — the UE `Build.bat` exit-code law: `Build.bat` returns
  `0` on a **failed** build. The `Result:` line is the evidence.
- **`error C####` / `error LNK####` census: `0`.** No `0x800711C7`, so Smart App Control is off today.
- Log (51 lines, whole file):
  `…/scratchpad/build_944.log`
- **The compile covered files that are NOT in this commit** — `SiegeCardRosterTest.cpp` and
  `SiegeCardArtRosterTest.cpp` both appear in the compile manifest (`[16/24]`, `[19/24]`). That is
  correct — a compile measures the working tree — but it means **"it compiled" is a statement about a
  tree wider than `84eec02`.** Declared rather than glossed (`TL-§5d`).

---

## 2. THE SUITE — **EXECUTED**, `439 / 0`, WITH THE POSITIVE CONTROL

```
Result={Success} = 439
Result={Fail}    = 0
LogAutomationCommandLine: Display: ...Automation Test Queue Empty 439 tests performed.
LogExit: Display: **** TestExit: Automation Test Queue Empty ****
```

**POSITIVE CONTROL ON THE ZERO** (`SC-§39`) — a synthetic file carrying one `Result={Fail}` row and one
`Result={Success}` row was read by the **same** `Select-String` parser:

| corpus | `Result={Fail}` | `Result={Success}` |
|---|---|---|
| synthetic control | **1** ✅ | **1** ✅ |
| real suite log | **0** | **439** |

⇒ **the parser is live, not blind.** The engine's own tally (`439 tests performed`) independently
corroborates the row count.

### THE INSTRUMENT FAILED TWICE BEFORE IT WORKED — AND BOTH FAILURES LOOKED LIKE SUCCESS

This is the finding of the row, and it is the exact shape the dispatch warned about.

| attempt | what happened | what it would have "read" |
|---|---|---|
| 1 | `-abslog=$log` **unquoted** in PowerShell 5.1 ⇒ flag **silently dropped**, no log file written. I then found `Saved/Logs/GitClaudeUnrealTest.log` with a fresh mtime and nearly parsed it. | **`0` Success / `0` Fail** — and that file was a **stale editor session** (header `19:44:47`, EOS heartbeats, **zero** `LogAutomation` lines, one `Log file open` header). **An empty log reads as a clean run.** |
| 2 | same flag, plus stdout redirected to a file | **14 lines** — the UBT `ValidatePlatforms` preamble **only**. Exit `0`. Zero successes AND zero failures. |
| 3 | `-abslog="$log"` **quoted** | **5,827 lines**, the real rows. |

⇒ **`-abslog` needs quotes under PowerShell 5.1; unquoted it is dropped with no error, exit `0`, empty
`stderr`.** Same disease as `SC-§56`: a well-formed, confident, wrong answer with no error surface.
**What caught it was refusing to publish a zero without proving the rows were in the file I parsed** —
I isolated the mechanism with a fast `-run=pythonscript` probe (8.8 s) before spending another suite run.
📌 **Recommend a `TL-§5c` rider: quote `-abslog`, and assert `LOG_LINES > 0` AND `tests performed` is
present before parsing.** A run that produces **no log** is indistinguishable from a green one.

### WARN-3 — `ConfigureLadderLink()` HEADLESS FOR THE FIRST TIME EVER: **it did not redden**

`Siegebound.BuildingStack.AClimbableTowerAcceptsTheHeightUpgradeAndSaturatesAtItsOwnPerClassCeiling`
completed `Result={Success}` in **33 ms**. No nav warnings, no crash. The harness took it.

### THE TWO INVERTED TESTS — **BY NAME, BOTH GREEN IN THE NEW POLARITY**

| test | path | result |
|---|---|---|
| StackTest 10 | `Siegebound.BuildingStack.AClimbableTowerAcceptsTheHeightUpgradeAndSaturatesAtItsOwnPerClassCeiling` | ✅ **`Result={Success}`** |
| PlacementTest 12 | `Siegebound.Placement.AClimbableTowerYieldsTheBlueUpgradeStateWhileStillRefusingTheFootprintWheel` | ✅ **`Result={Success}`** |

**Negative control on the inversion:** the pre-inversion names are **ABSENT** from the log —
`AClimbableTowerRefuses…` = **0 hits**, `NeverYieldsTheBlue…` = **0 hits**. ⇒ the old rows are gone, not
merely renamed alongside survivors.

### THE DELTA — **ASSERTED AS A DELTA, NEVER AS A MATCHING ABSOLUTE**

Last **executed** figure of record: **`433 / 0` at `09b9b50`.** Now: **`439 / 0`.** Gross **`+6`**.

| file | `HEAD` (`f050caf`) | worktree | Δ | owner |
|---|---|---|---|---|
| `SiegeBuildingStackTest.cpp` | 10 | **14** | **+4** | ⭐ **this batch** |
| `SiegePlacementTest.cpp` | 31 | 31 | **0** | this batch — test 12 **inverted, not added** |
| `SiegeCardRosterTest.cpp` | 1 | 2 | +1 | **`TASK-964`** — NOT in this commit |
| `SiegeCardArtRosterTest.cpp` | *absent* | 1 | +1 | **`TASK-957`** — NOT in this commit |

> ### **MY DELTA IS `+4`.** The other `+2` is two lanes that are still dirty and gated to their own host.

- **`HEAD` census before:** `433` declarations across `32` files. **After `84eec02`:** **`437` across `32`**
  — `433 + 4`, exactly the declared delta, measured on the committed tree.
- **Worktree census:** `439` across `33` files — which is also what **ran**.
- ⇒ **`TL-§5d`, stated plainly: the suite that returned `439/0` measured a tree WIDER than this commit.**
  The two extra files are independent (content census: **zero** stack symbols in either), so they cannot
  have masked a stack failure — but the honest sentence is **`439` is the working tree's number, `437` is
  `84eec02`'s.**
- **Instrument controls on the census** (`SC-§56`): positive — `git show HEAD:<known path>` resolved;
  negative — a bogus path returned **`fatal: … does not exist in 'HEAD'`** (the LOUD family, as intended).
  I used the `<commit>:<path>` lookup family with repo-root-absolute paths **deliberately**, so a miss
  would be fatal rather than a silent zero.

---

## 3. THE PATHSPEC — DERIVED FROM MY OWN `git status`, NOT ADOPTED

⚠️ **`SC-§55`: my session-start `gitStatus` snapshot was WHOLLY STALE** — it described a Castle/FBX batch
with `A ` staged entries, none of which exist in the live tree. **I ignored it entirely.**

**Row's `names:` list (10) vs what I committed (12) — the two additions were FORCED, not chosen:**

| # | path (repo-relative) | source |
|---|---|---|
| 1-2 | `Source/…/Siegebound/Building.{h,cpp}` | row |
| 3 | `Source/…/Siegebound/ClimbableTower.h` | row |
| **4** | **`Source/…/Siegebound/ClimbableTower.cpp`** | ⛔ **FORCED — `qa/TASK-943.md` note 1** |
| 5-6 | `Source/…/Siegebound/SiegePlayerController.{h,cpp}` | row |
| 7 | `Source/…/Siegebound/Tests/SiegeBuildingStackTest.cpp` | row |
| **8** | **`Source/…/Siegebound/Tests/SiegePlacementTest.cpp`** | ⛔ **FORCED — `qa/TASK-943.md` note 1** |
| 9-10 | `.claude/pipeline/handoffs/TASK-94{1,2}-programmer.md` | row |
| 11 | `.claude/pipeline/handoffs/TASK-956-buildmaster.md` | row (`TL-§5d` provenance) |
| 12 | `.claude/pipeline/qa/TASK-943.md` | row |

**I confirmed both forced entries are genuinely build-breakers rather than taking the note on trust:**
- `ClimbableTower.cpp` — **`+57` lines, the constructor ceiling** (`MaxStackHeightMultiplier = 2`) plus the
  full `40n ≤ HH` derivation as a comment. Omitting it ships a **header that declares a ceiling nothing
  sets**.
- `SiegePlacementTest.cpp` — **22** stack-symbol occurrences and the inverted test 12. Omitting it ships a
  **RED suite** at `HEAD`.

**`SC-§56` POSITIVE CONTROL ON THE STAGE** — because a `-- <pathspec>` miss is silent (exit `0`, empty
`stderr`): after `git add` I diffed **intended vs actually staged, both directions**.
`staged = 12` · `intended-but-not-staged = ∅` · `staged-but-not-intended = ∅` · post-commit
`git show --name-only` = **the same 12**. Existence of all 12 on disk was checked before staging.
(cwd was the repo root, so root-relative == cwd-relative for the filter family.)

---

## 4. `§25b` cl. R — **ALL 57 DIRTY PATHS ASSIGNED. NOTHING UNACCOUNTED.**

| class | count | attributed to | how |
|---|---|---|---|
| **mine** | **12** | `TASK-944` | content: every one carries stack code or is a named record |
| `CONVENTIONS.md`, `TASKBOARD.md` | 2 | **manager** | spec item (7) excludes them |
| `.md` orphans in `handoffs/`, `qa/`, `footage/` | 14 | **`TASK-951`** (trailing doc-host) | non-mine `.md` |
| `Content/FogArea/**` | **27** | **`TASK-927`** | untracked binaries; **this row made zero `Content/` writes** |
| `Tests/SiegeCardRosterTest.cpp` | 1 | **`TASK-964`** | ⭐ **content: 0 stack symbols, card-roster code** |
| `Tests/SiegeCardArtRosterTest.cpp` | 1 | **`TASK-957`** | ⭐ **content: 0 stack symbols, 88 card-art hits** |

⇒ `12 + 2 + 14 + 27 + 1 + 1 = **57**`. **Attributed by CONTENT, corroborated by marker, confirmed by live
status** (`SC-§59`), in that order — **never by testimony.**

**WHAT I FOUND STAGED: NOTHING.** The index was **empty** before I touched it (checked explicitly —
`git diff --cached --name-only` returned nothing). ⇒ **no editor auto-add fired this time**, and no
unaccounted staged path had to be reported-and-left. The editor was down for the whole row, which is
consistent.

### ⚠️ STOP-THE-LINE REPORT TO THE MANAGER — `Content/FogArea/` HAS NO HOST

**27 untracked binary paths** under `GitClaudeUnrealTest/Content/FogArea/` (a Blueprint, 9 Data assets,
`Maps/Overview.umap`, 10 materials, 6 textures). **They belong to no row's pathspec that I can see.**
`TASK-951` excludes non-`.md` at its item (4)(d) — **correctly**, and that exclusion must NOT be widened:
`*.png`/`*.uasset` are LFS patterns and a doc-host stops on `lfs`. So this is the **same shape** as the
`VID-005` PNGs, which got the separate named host `TASK-966`.
⇒ **They need a NAMED host row. I did not absorb them and I did not `git reset` anything.**

---

## 5. LFS AND SECRETS

- **`git check-attr filter`** on all 12 ⇒ **`unspecified` ×12.** No `lfs`. No stop.
  **Positive control:** the same probe on `Content/FogArea/Materials/M_FogArea.uasset` returns **`lfs`**
  ⇒ the probe **can** say `lfs` and chose not to (`SC-§39`).
- **`§25b` cl. S — the binary sweep's subject set is EMPTY, and I MEASURED that rather than assuming it.**
  Every one of the 12 was tested for binary-ness via git's own numstat; **zero binaries.** No oid-vs-sha256
  join to run — and no join that could report `0 mismatches` from `0 joined rows`.
- **Secret sweep** over the full staged diff + the four `.md` bodies (**3,470 lines**): `hf_*`, `HF_TOKEN=`,
  `sk-*`, `ghp_*`, `xox[baprs]-`, PEM private keys, `AKIA…`, quoted `password=` ⇒ **0 hits.**
  **Positive control:** a synthetic `export HF_TOKEN=hf_…` line read **1** under the same regex.

---

## 6. THE COMMIT MESSAGE — THE SURFACE NO GATE COULD CHECK

`STACK-§10` cl. 2 binds the message, and `qa/TASK-943.md` note 4 named it as the one thing QA could not
verify. **Verified from the stored commit object, not from my draft:**

```
His spec said ×2→×5; ×2 is a MEASURED shortfall, not a design choice.
```

- Names **`TASK-941..944`** and **`TASK-956`** (whose read-back licensed the number).
- States the licensed ceiling **`×2`** and writes out the `40n ≤ HH` ⇒ `n ≤ 2.2` arithmetic.
- Names **`STACK-§10`** as the law the batch bought.
- States **plainly** that the player **can** now stack a WatchTower.
- ⛔ **"capped at 2 for balance" appears NOWHERE** — I checked the message (`0` hits for *"for balance"*)
  and the tree (`0` hits across `Source/`). The single occurrence of the word *balance* in the message is
  the sentence **refuting** that framing.

---

## 7. 📌 DISCLOSED, NOT FIXED — **THIS COMMIT MAKES A HELP SCREEN LIE**

`SiegeControlsHelpWidget.cpp:690-695` still tells the player:

> *"ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB… Hovering one shows RED
> with 'That building cannot be stacked'."*

**As of `84eec02` that is FALSE, and NO test goes red** — the help test pins id, category and key, never
the prose. **The file is CLEAN** (unmodified, already at `HEAD`) ⇒ it is not in this commit, and the
falsehood is a **consequence** of the commit rather than a change in it.
⛔ **I did not edit it** — the replacement wording is a player-facing UX call, not a refactor's, and a
manager row is being boarded. ⭐ **It ships knowingly and disclosed, in the commit message itself** —
the same discipline the war-map right-click sentence got in `e9df584`.
⇒ ⛔ **The batch is NOT closed until that row is boarded.**

---

## 8. WHAT I DID NOT DO

- ⛔ **NO PUSH.** `main` is **19 ahead**, `0` behind. Jonathan has not asked in Claude Code.
- ⛔ **I did NOT flip the board.** `TASK-944` item (7) puts `TASKBOARD.md`/`CONVENTIONS.md` **out of
  scope**, and both are dirty under the manager's hand right now. ⇒ **the `done` flip on `TASK-944` is
  owed to the manager** — flagged rather than silently taken, because a concurrent write would race.
- ⛔ No editor, no MCP, no `L_Arena`, no `Content/**` write, no `.uasset` touched.
- ⛔ Wrote no code and no art. Did not fix `SiegeControlsHelpWidget.cpp`. Did not widen `TASK-951`'s
  exclusion. Did not `git reset` or otherwise tidy anything I do not own.
- ⛔ Did not amend to include **this file** — it is the minted orphan; **`TASK-951` takes it**
  (`TL-§5e` cl. 7).

---

## 9. FOLLOW-UPS FOR THE MANAGER

1. ⛔ **The `Cards.StackUpgrade` help text** — already `ROUTED-2` in `qa/TASK-943.md`; **board it before
   `TASK-945` is declared satisfied.** It will otherwise tell Jonathan on screen that the thing he is
   about to watch work cannot be done.
2. ⚠️ **`Content/FogArea/` (27 untracked binaries) needs a NAMED host row** — §4 above. Do **not** widen
   `TASK-951`'s non-`.md` exclusion to absorb them.
3. 📌 **`TL-§5c` rider: quote `-abslog`** and require `LOG_LINES > 0` + `tests performed` present before
   any suite figure is published. §2 above; it cost this row two runs and would have produced a
   confident `0/0`.
4. ⭐ **`TASK-945` (Jonathan's climb sitting) is UNBLOCKED** by `84eec02`. `STACK-§10` cl. 5's sentence
   belongs on its acceptance row **verbatim**: *a hero-only playtest would pass while the bot's units
   silently fail to path up* — so that sitting must watch an **AI unit** climb a **stacked** tower, not
   only the hero.

---

## Artifacts

- Build log: `…/scratchpad/build_944.log` (51 lines, `Result: Succeeded` at line 49)
- Suite log: `C:\Temp\t944\suite-944.log` (5,827 lines, `439 tests performed`)
- Parser control: `C:\Temp\t944\control-fail.log` · failed-attempt stdout: `C:\Temp\t944\stdout-944.txt`
