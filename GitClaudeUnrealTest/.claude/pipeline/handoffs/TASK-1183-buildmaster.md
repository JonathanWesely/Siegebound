# TASK-1183 — [SUITE-RUNNER-TRACKED-SHIP] + TASK-1180 — build-master

**2026-09-09.** One compile, one commit, and **a runner that did not ship.**

> ## ⛔ THIS FILE WAS BORN OUTSIDE ITS OWN COMMIT
> `ea7b4d2` was made **before** this handoff existed, because the handoff must carry the run
> evidence and the run evidence is what decided the commit's contents. **This file is UNTRACKED as
> written.** It is the next host's to stage — exactly as `handoffs/TASK-1175b-buildmaster.md` was
> mine (cl. 7a orphan, and it shipped in `ea7b4d2`).

> ## 🚨 THE HEADLINE, BECAUSE IT INVERTS THE ROW'S EXPECTED OUTCOME
> **THE FOG PROSE LANE SHIPPED. THE SUITE RUNNER DID NOT.**
> `TASK-1183` was boarded as the runner's commit host. **Run #19 — the acceptance run, the one
> `TASK-1182` said 52 self-test cases and 16 mutants could not buy — exposed a real defect.**
> That is **routing rule 6**: back to `gameplay-programmer`, no commit of the runner. The fog lane
> was independent and clear, so it shipped alone.

---

## 0. COMMIT

**`ea7b4d2`** — `main` **9 ahead / 0 behind**, ⛔ **NOT PUSHED.**

`git show --stat HEAD`:

    commit ea7b4d2446548445a12045c4c11fe7f88386b5e5
    Author: Jonathan Wesely <wesely.jonathan@gmail.com>
    Date:   Wed Sep 9 05:02:13 2026 -0700

        TASK-1180: the fog prose lane ships and the suite runner does not - #19 made a bound
        kill a real editor for the first time, and the kill report named the process that died
        while missing the one that lived (TASK-1177/1178/1179 shipped; TASK-1181 back to the
        programmer under routing rule 6)

     .../.claude/pipeline/CONVENTIONS.md                | 113 +++-
     GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  | 156 ++++-
     .../pipeline/handoffs/TASK-1173-programmer.md      |  37 +-
     .../pipeline/handoffs/TASK-1175b-buildmaster.md    | 287 +++++++++
     .../.claude/pipeline/handoffs/TASK-1177-artist.md  | 425 ++++++++++++
     .../pipeline/handoffs/TASK-1178-programmer.md      | 710 +++++++++++++++++++++
     ...7-CARD-peak-vs-trough-2.94x-nothing-changed.png |   3 +
     ...1177-visibility-swings-3x-at-a-fixed-camera.png |   3 +
     .../.claude/pipeline/qa/TASK-1174-report.md        |  52 +-
     .../.claude/pipeline/qa/TASK-1179-report.md        | 689 ++++++++++++++++++++
     .../GitClaudeUnrealTest/Siegebound/FogVolume.cpp   |  25 +-
     .../Siegebound/Tests/SiegeFogVisualTest.cpp        |  93 ++-
     12 files changed, 2546 insertions(+), 47 deletions(-)

⭐ **The two PNGs show `3 +` — three lines each. That is the LFS pointer, not the image.** Verified
by oid below, never by size.

---

## 1. COMPILE — **`Result: Succeeded`**

Parsed **out of the log**, never from `$LASTEXITCODE` (which lies on this project's builds —
`ue-build-bat-exit-code-lies`). Grep for `Result:` returned exactly one line, at log line 28.

- **`Result: Succeeded`** · total execution **5.17 s** · **zero** errors, **zero** warnings
  (`grep -niE "error|failed|fatal|0x8007"` over the whole log: **no hits**).
- ⭐ **Independent evidence the changed bytes really went through the compiler**, rather than being
  skipped as unchanged — UBT's own adaptive-unity line:
  `[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: FogVolume.cpp, SiegeFogVisualTest.cpp`
  followed by `[1/5] Compile [x64] SiegeFogVisualTest.cpp` and `[2/5] Compile [x64] FogVolume.cpp`.
  **Exactly the two files QA gated, and no others.** (`SC-§27`: a comment still changes compiled bytes.)
- ⛔ **`"COOKED TARGET NOT COMPILED"`** — this was `GitClaudeUnrealTestEditor Win64 Development`
  only. No cook, no package, no shipping target.

---

## 2. SUITE — **`555 / 555`**, and it is a MEASUREMENT twice over

| run | N / M | wall | exit |
|---|---|---|---|
| **#16** (`-Porcelain`, default bounds) | **555 / 555** | 42 s | 0 |
| **#19 first attempt** (bounds too loose to trip) | **555 / 555** | 44 s | 0 |

**The hand-typed lane's last measured figure at `4a3da63` is `555 / 555`.** Mine matches it exactly,
on two independent launches. ⇒ ⭐ **`TASK-1178`'s declared suite delta of ZERO stops being a
derivation and becomes an observation.** A delta would have been a finding; there is none.

`RUNNER_STARTED=555 · RUNNER_COMPLETED=555 · RUNNER_SUCCESS=555 · RUNNER_FAIL=0 · RUNNER_SKIPPED=0`,
`discovered 555`, echo `1 / 1`.
Log: `Saved/Logs/run_suite_bounded_suite_20260909-045216.log` (untracked — `Saved/` is gitignored;
it is the successor to `suite-1167rev2.log` and it is **equally unreproducible**, `L2.8 i`).

⛔ **`Source/` diff shape confirmed against git, not relayed:** `FogVolume.cpp` **+16/−9**,
`SiegeFogVisualTest.cpp` **+79/−14**, and `git status` over `Source/` listed **exactly those two**.
**Comment/literal-only, zero behaviour change** — QA's executable-hunk count of 0 stands.

---

## 3. THE FULL RUN TABLE — 15 no-editor + 7 editor, OBSERVED vs EXPECTED

**A — no editor (these gate the rest). 15 / 15 MATCHED.**

| # | run | expected | observed | |
|---|---|---|---|---|
| 1 | `-SelfTest` | 0, `52 / 52` | **0, `52 / 52`** | ✅ QA derived 52 by hand from the file; agreement means the file did not move under its own gate |
| 2 | `green-suite.log` | 0 | **0** | ✅ |
| 3 | `red-suite.log` | 5 | **5** | ✅ a red suite reads red |
| 4 | `skipped-suite.log` | 0 + `[2 skipped, 0 not-run]` | **0**, reason carried it | ✅ W-2's regression is real |
| 5 | `zero-started.log` | 2 | **2**, named `SC-95 cl. 1` | ✅ |
| 6 | `zero-started-filtered.log` | 3 | **3** | ✅ |
| 7 | `result-absent.log` | 4 | **4** | ✅ |
| 8 | `count-mismatch.log` | 8 | **8** | ✅ |
| 9 | `w9-cmd-semicolon.log` | 2, names `SC-116 W9` | **2**, named | ✅ |
| 10 | `green-commands.log` | 0, terminator **seen** | **0**, `RUNNER_TERMINATOR_ECHO=… seen` | ✅ |
| 11 | `no-terminator-echo.log` | 0, terminator **NOT SEEN** | **0**, `… NOT SEEN` | ✅ **B-1 stays closed** |
| 12 | absent path | 7 | **7** | ✅ |
| 13 | `-LogPath` a `.uasset` | 64, asset intact | **64** ×2, sha256 **UNMOVED** | ✅ see §5 |
| 14 | both `-DryRun` | 0 each | **0 / 0**, both lines exact | ✅ see §4 |
| 15 | `-File … -Commands A,B,C` | **64**, not 2 | **64** | ✅ R2.7a's refusal seen firing |

**B — editor.**

| # | run | expected | observed | |
|---|---|---|---|---|
| **16** | first real suite | 0 | **0, `555 / 555`**, 42 s | ✅ **matches the hand-typed lane** |
| 17 | STALL on a healthy suite? | — | **no trip** | ✅ bounds correctly tuned for 42 s |
| **18** | command lane first dispatch | 0, ≈12 s | **0, 12 s exactly**, echo `3 / 3` | ✅ |
| **19** | **a bound must kill** | **6** | **6** | ⛔ **exit right, GUARD DEFECTIVE — §6** |
| 20 | B-2 live check | agree | `$LASTEXITCODE=6` **and** `RUNNER_EXIT=6` | ✅ **B-2 closed by execution** |
| 21 | W-15 skim risk | — | `RUNNER_BOUND=` + `RUNNER_LOG_VERDICT=8`, labelled | ✅ not mistakable for green |
| 22 | `.cmdline` sidecar + BOM | first char `C` | ⛔ **NOT CHECKED — UNEXERCISED** | see §8 |

### ⭐ #18 answers B-1 by OBSERVATION, as ordered

The log **this run produced** does carry the echo:

    [2026.09.09-11.53.35:385][  1]Cmd: QUIT_EDITOR

⛔ And it carries **six** `Cmd:` lines for **three** commands — `MAP LOAD`, `MAP CHECKDEP`, our three,
the terminator. ⇒ **cl. 4(c)(i) is now measured in the COMMAND lane too:** a line-counting guard
really would fail a healthy run.

---

## 4. THE TWO `-DryRun` LINES, DIFFED AGAINST `CONVENTIONS.md:4443-4444`

**Suite lane, emitted:**

    -ExecCmds="Automation RunTests Siegebound;Quit"

**Law, `CONVENTIONS.md:4443`:** `-ExecCmds="Automation RunTests Siegebound;Quit"` ⇒ ✅ **identical,
character for character. The `;` survived.**

**Command lane, emitted:**

    -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR"

**Law, `CONVENTIONS.md:4444`:** `-ExecCmds="CmdA,CmdB,CmdC,QUIT_EDITOR"` ⇒ ✅ **the exact shape, and
on the exact three commands cl. 2 records as its measured instance.** Commas, and `QUIT_EDITOR`,
never `Quit`.

---

## 5. #13 — THE ONE LINE THAT CAN DAMAGE THE REPO

⚠️ **QA's literal path `Content\Meshes\SM_Rock_01.uasset` DOES NOT EXIST on disk** — a refusal on an
absent path proves nothing about deletion, so I ran a **real** asset as well.

| target | exists before | exit | exists after | sha256 |
|---|---|---|---|---|
| `Content\Meshes\SM_Rock_01.uasset` (QA's literal) | **False** | **64** | — | — |
| `Content\Meshes\SM_Archer.uasset` (**live control**) | **True** | **64** | **True** | `E9EA136D…CE175A8` → **IDENTICAL** |

⇒ `Resolve-SafeLogPath` refuses before `Remove-Item` is reachable. **W-3 holds against a real asset.**

---

## 6. ⛔ #19 — THE KILL PATH RAN, AND THE KILL REPORT IS WRONG

**Prescribed bounds did not trip.** `-OverallSeconds 45` vs a suite that finishes in **44 s** ⇒
exit 0, a genuine `555 / 555`. **That is bound tuning, not a tool defect** (`L2.9` #17, inverted).
Re-ran at `-OverallSeconds 20`, which must trip mid-run.

**Result: `BOUND TRIPPED: OVERALL bound 20s exceeded (elapsed 21s)`.**

| reading | value |
|---|---|
| **`$LASTEXITCODE`** | **6** |
| **`RUNNER_EXIT`** | **6** |
| agree? | ✅ **YES — B-2 is closed BY EXECUTION, not by reading** |
| `RUNNER_BOUND` | `OVERALL bound 20s exceeded (elapsed 21s)` |
| `RUNNER_LOG_VERDICT` | `8` (labelled *for information only*) |
| partial counts | `134` started / `133` completed — correctly refused as a result (`SC-§114`) |

### 6a. Survivor corroboration — the script was RIGHT that the PID died

⛔ QA warned: *"do not trust a `SURVIVED` verdict without corroborating it a second way."* I used
three, because the script's own claim was the thing under test:

| reader | result |
|---|---|
| script's own probe (`$proc.Refresh(); $proc.HasExited`) | `PID 13816 confirmed gone (HasExited).` |
| **1** `Get-Process -Id 13816` (the *known-false* reader) | **RESOLVED** the PID, `HasExited=True` ⇒ ⭐ **the false-red QA described, reproduced live** |
| **2** `Get-CimInstance Win32_Process -Filter ProcessId=13816` | **nothing** ⇒ gone. ✅ **positive control: the same query on my own PID returned my process**, so the reader worked |
| **3** fresh name census in a different process | PID 13816 **absent** |

⇒ ✅ **The rev-2 `HasExited` probe is CORRECT and is now measured against a real editor.**

### 6b. ⛔ BUT — the blocker, `run_suite_bounded.ps1:1567`

Three consecutive lines of the script's own output:

      PID 13816 confirmed gone (HasExited).
      !! 1 engine process(es) still running after the kill: UnrealEditor-Cmd(PID 13816)
      !! Some may be Jonathan's. NOT killing them.

The orphan census is built with **`Get-Process -Name`** — **the exact reader the comment nine lines
above it (`:1548-1555`) declares wrong, for exactly the reason it gives.** `$proc` is still in scope
holding the handle that keeps a corpse resolvable.

- ⛔ **(a) FALSE POSITIVE:** it names the PID the line above just buried.
- ⛔ **(b) FALSE NEGATIVE:** the **one real orphan** — `CrashReportClientEditor` **PID 13296**,
  started 04:55:38, alive at that instant — was **not named**. `'CrashReportClient'` cannot
  prefix-match a longer name. **Controlled on a live process, not reasoned:**
  `-Name 'powershell'` → **4** · `-Name 'powershel'` → **0** · `-Name 'powershel*'` → **4**.

⇒ ⚖️ ***It reports the process that died, misses the process that lived, and then says "Some may be
Jonathan's."*** Nothing of Jonathan's was running.

**Why it blocks:** it is the **W-7 guard #19 exists to exercise**; `SHIP-§9` says a guard seen only
to pass is `NOT MEASURED`; and it is **W-14's shape** — the fix applied to one organ (`:1560`) and
not its sibling nine lines down. **Fifth null-reading defect of the session, second in this function.**

**FIX (small, two parts):** exclude the killed PID (or filter on `HasExited`); **wildcard every
name**. And add **the case that FAILS**: assert the killed PID is ABSENT, and that a longer-named
process IS found.

---

## 7. GIT HYGIENE

### 7a. Reader control FIRST (`SC-§102`) — and the trap is real, in BOTH directions

- `git rev-parse --show-toplevel` = **`C:/GitProjects/GitHub/GitClaudeUnrealTesting`** — ⭐ **one
  level ABOVE the project dir.** `--show-prefix` = `GitClaudeUnrealTest/`.
- ⛔ **`git status --porcelain` prints ROOT-relative paths (`GitClaudeUnrealTest/…`) while pathspecs
  from this cwd must be CWD-relative.** Copying a status line into a pathspec **answers with
  silence**. Measured:

| form | `git ls-files -- <path>` |
|---|---|
| `.claude/pipeline/TASKBOARD.md` (cwd-relative) | **1 hit** ✅ positive control |
| `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md` | **0 hits** ⛔ **SILENCE** |

- ⛔ **And `git show :<path>` is the INVERSE — it demands the ROOT-relative form.** Positive control
  printed a real pointer; negative control (`…TASK-9999-DELIBERATELY-ABSENT.md`) failed loudly with
  *"does not exist (neither on disk nor in the index)"*, so absence is distinguishable from
  mis-anchoring.

### 7b. Existence check before staging (`SC-§106`), with a MISSING negative control

All 12 staged paths returned **EXISTS**. The deliberate 13th row,
`.claude/pipeline/qa/TASK-9999-DELIBERATELY-ABSENT.md`, returned **MISSING** ⇒ **the checker can say
no**, so the twelve EXISTS readings mean something.

### 7c. LFS — by oid vs sha256, ⛔ NEVER by size

| file | index pointer oid | working-file sha256 | |
|---|---|---|---|
| `TASK-1177-CARD-peak-vs-trough-2.94x-nothing-changed.png` | `b0cce4e5…b8bb8187` | `b0cce4e5…b8bb8187` | ✅ **MATCH** |
| `TASK-1177-visibility-swings-3x-at-a-fixed-camera.png` | `102154f6…784f2d47` | `102154f6…784f2d47` | ✅ **MATCH** |

Both index entries begin `version https://git-lfs.github.com/spec/v1`. ⭐ **Control:** the same reader
on `handoffs/TASK-1177-artist.md` returned **plain markdown**, not a pointer — so the reader
distinguishes LFS from non-LFS rather than printing pointers indiscriminately.

### 7d. Fences

| fence | result |
|---|---|
| `Content/Maps/L_Arena.umap` | ✅ `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — **before AND after**, identical, and identical to the artist's recorded hash |
| `Content/**` · `Config/**` · `Saved/**` · `testvideo/**` | ⛔ **ZERO staged** |
| index after commit | ✅ **empty** |
| push | ⛔ **NONE.** `main` 9 ahead / 0 behind |
| `TASKBOARD.md` | ✅ **`Edit` tool only**, one `- status:` line per row, **7 rows** — re-read at the last instant before staging (two lanes edited it tonight) |
| `CONVENTIONS.md` | ⛔ **not edited by me** (`SC-§82`) — arrived dirty from another lane and was staged as-is |
| verification | ✅ **the COMMIT** (`git show --stat HEAD`), never the index (`§25c`) |

---

## 8. ⛔ WHAT STAYED DIRTY, AND WHOSE IT IS

    ?? .claude/pipeline/handoffs/TASK-1181-programmer.md
    ?? .claude/pipeline/qa/TASK-1182-report.md
    ?? Tools/SuiteRunnerFixtures/
    ?? Tools/run_suite_bounded.ps1

**All four are the withheld runner lane — `gameplay-programmer`'s, via `TASK-1181` rev-3.** They ship
together when the blocker in §6b is fixed. Nothing else is dirty.

Plus **this file**, untracked, mine, born outside its own commit (see the banner).

### ⛔ UNEXERCISED, in those words

- **#22, the `.cmdline` sidecar and its BOM (`N-1`): NOT CHECKED.** The PC is on a sleep timer and I
  spent the remaining budget on the blocker. **Unexercised — not "fine".**
- ⛔ **NOT STRUCK, all the manager's under `SC-§82`:** `TL-§6`'s `NOT MEASURED` bullet ·
  `CONVENTIONS.md:4895` · `CONVENTIONS.md:5961` · `TASKBOARD.md:1735`'s *"does not exist"* site ·
  the script's own `SANCTIONED IS NOT EXERCISED` header block.
  ⭐ **And #19 did not earn the strike in any case, because it failed.**

---

## 9. 🙋 FOR THE MANAGER

1. 🚨 **`TASK-1177` ask (B) is MEASURED NOT CLOSED — `min/max = 0.3399`, a 2.94× visibility swing at
   a fixed camera with every value frozen.** ⛔ **Nothing in `ea7b4d2` fixes it and nothing in it
   claims to.** The evidence shipped; the repair is yours to board (`SC-§100`).
2. ⛔ **A tension I created and will not resolve myself:** `CONVENTIONS.md`'s cl. 4(c) text about
   *"`TASK-1181`'s runner as it ACTUALLY IMPLEMENTS IT"* is now committed while
   **`Tools/run_suite_bounded.ps1` is not in the tree.** ⚠️ That is B-1's own shape — law describing
   an implementation a reader cannot open — **inverted**: `TASKBOARD.md:1735`'s *"this file does not
   exist"* is, as of `ea7b4d2`, **true again**. I could not split those two files by lane. **Yours.**
3. ⚠️ `TASK-1179` WARN-2 (`SiegeFogVisualTest.cpp:1505`) **shipped open**, exactly as the reviewer
   recommended — board it with the `headless`/universal-sentence sweep, not as its own row.
4. ⭐ **A candidate law, offered not asserted** (`SC-§82`): ***the wrapper is part of the
   instrument.*** `powershell -Command "& script"` mashes every non-zero exit to **1**; `-File` is
   faithful but silently truncates an array parameter to its first element and slides the rest onto
   positional parameters. **Both failure modes produce a plausible number from a run that did
   something else** — and `1` is the undocumented code W-11 warns about, manufactured *outside* the
   script. **An exit code is evidence only once the reader that carried it has a positive and a
   negative control.**
