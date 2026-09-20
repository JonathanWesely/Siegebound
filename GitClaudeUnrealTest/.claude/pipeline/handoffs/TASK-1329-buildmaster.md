# Handoff — TASK-1329 (SUITE-RUNNER-ASIDE-HOST)

author: build-master · 2026-09-19
subject row: `TASK-1327` **only** · marker `TASK-1329-SUITE-RUNNER-ASIDE-HOST`
gate: `TASK-1328` (`Verdict:` = **PASS**, 0 BLOCKER · 0 WARN · 4 NIT · 1 declared residual)
commit: **`9d80505`** · `main` **9 ahead** of origin, **not pushed**

---

## 0. THE BLOCKER, DISCHARGED BY TOKEN

```
$ grep -n "Verdict:" .claude/pipeline/qa/TASK-1328-report.md
11:**Verdict: PASS** — 0 BLOCKER · 0 WARN · 4 NIT · 1 declared residual (`TASK-1329`'s to close)
393:  `Verdict:`, never a line address** (`SC-§126` cl. 10). It reads `PASS`.
```

Two carriers, both `PASS`. Grepped as a token, never at an address.

---

## 1. SPEC (1) — THE PARSE, AND **BOTH** CONTROLS

### 1a. The live file

```
SHA256 BEFORE = 418C17CD48028B19D0F1CEEC548CDBE5BECC16B046AA271B33C6846209C6DAF8
[Parser]::ParseFile(<tracked file>) -> errors = 0
SHA256 AFTER  = 418C17CD48028B19D0F1CEEC548CDBE5BECC16B046AA271B33C6846209C6DAF8   (IDENTICAL)
```

The tracked file was never written. Both controls ran on scratchpad copies, both deleted.

### 1b. The two delimiters, RE-MEASURED at my instant

```
$ grep -nE '<#|#>' Tools/run_suite_bounded.ps1
1:<#   238:#>   313:<#   322:#>   372:<#   382:#>   405:<#   418:#>   520:<#   525:#>
586:<#   594:#>   649:<#   653:#>   676:<#   689:#>   797:<#   808:#>   891:<#   897:#>
951:<#   964:#>   1032:<#   1035:#>   1057:<#   1065:#>
census: <# = 13   #> = 13     file = 1825 lines
```

Header close = **`:238`**. Final close = **`:1065`**. Strictly interleaved, so the header's
close is **derived, not assumed**. The dispatch named both values and I re-measured anyway;
that they agreed is a fact about today, not a licence to inherit them tomorrow.

### 1c. The RIGHT control — it **FIRED**

Break the **final** `#>` (`:1065`), leave `:238` intact:

```
CONTROL ParseFile errors = 1
  line 1057: The terminator '#>' is missing from the multiline comment.
```

The error lands on **`:1057`** — the *opening* `<#` of the last block — because there is no
later `<#` for the runaway comment to re-pair with.

### 1d. ⭐ The WRONG control — MEASURED, not merely cited

I also broke the header's own `#>` at `:238` on a separate throwaway copy, because a trap
you only quote is a trap you are still one slip away from:

```
WRONG-CONTROL (header :238 broken) ParseFile errors = 0
```

**Zero.** The runaway re-pairs against the `<#` at `:313` and closes at `:322`; every later
pair shifts by one and the file still terminates. Same file, same *kind* of edit, opposite
verdict — one control detects the failure it claims to detect, the other reports success no
matter what. `SHIP-§9`, re-demonstrated at this row's own instant, and `TASK-1318`'s
CONTROL A finding independently replicated.

---

## 2. SPEC (3) — ⭐ THE GATE'S DECLARED RESIDUAL, **CLOSED**

`TASK-1328` held no `Bash` ⇒ no `git diff`. It could show the two *claimed* hunks are
comment-only and inside the header, but **not that they are the only hunks** — an
equal-line-count substitution below `:238` is invisible to every check it could run.

I hold the instrument it lacked, and I answered the worry with the **complement**, not a
sample:

```
$ git diff -U0 -- .../run_suite_bounded.ps1 | grep '^@@'
@@ -87,4 +87,7 @@
@@ -178 +181,2 @@

hunk count                            = 2   (exactly the two claimed)
furthest NEW-file line touched        = 182
hunks at or beyond the header's :238  = 0
numstat                               = 9 insertions / 5 deletions
```

⇒ **The residual is CLOSED. Not "spot-checked clean" — empty by construction:** there is no
region below the header for a same-line-count substitution to hide in. I read the diff whole
and it is two comment hunks: the clause-(a) aside at `:87-93` and the recipe clause at
`:181-182`. The programmer's declared `9/5` matches `--numstat` exactly.

⚠️ `SC-§128` in passing: `--stat` reported `14` for this file. That is **insertions +
deletions** (9+5), not insertions. The law that was minted today off the orchestrator's
`TASK-1314` error applies to my own verification line, so I used `--numstat` for the split
and flagged `--stat`'s number for what it is.

---

## 3. SPEC (2) — ONE SUITE RUN THROUGH THE EDITED RUNNER

```
log     Saved/Logs/run_suite_bounded_suite_20260919-200224.log
PID 8360, boot observed 10 s, wall clock 50 s
RUNNER_EXIT=0  RUNNER_STARTED=561  RUNNER_COMPLETED=561  RUNNER_SUCCESS=561  RUNNER_FAIL=0
RUNNER_SKIPPED=0  RUNNER_ECHO_MATCHED=1/1
```

`$LASTEXITCODE` was 0 and is **recorded, not consulted** (`SC-§95`; Build.bat/runner exit
codes lie — the verdict is parsed from the log).

### 3a. THE TRIPLE

| signature | count |
|---|---|
| `HTTP 401` | **0** |
| `401 Unauthorized` | **0** |
| `Unauthorized` (case-insensitive) | **0** |
| `LogAura` (any line) | **0** |
| `Result={Fail}` | **0** |
| `LogAutomationController: Error` | **0** |
| **red count, WITH NAMES** | **0 — name list printed, empty** |
| `LogAutomationController: Warning` | 59 — **baseline is also 59** |

### 3b. 🚨 THE DIGIT TRAP, ON MY OWN LOG

```
$ grep -c '401' run_suite_bounded_suite_20260919-200224.log
8
```

⚖️ ***Six logs now: 6, 8, 9, 13, 14, 15 — and the true value was 0 on every one. The bare
digit count has never once been signal. Count the signature.***

### 3c. DELTA 0, RECONCILED **BY NAME** (`SC-§104`)

Baseline = `TASK-1326`'s own pass, `…_suite_20260919-152207.log` (`SC-§95`):

```
distinct Path={…} names  NEW  = 561
distinct Path={…} names  BASE = 561
ADDED vs baseline   = (none)
REMOVED vs baseline = (none)
name sets byte-identical = IDENTICAL   (sha256 7ed2943b72ccfeda96222e4fcaf4b3b4…)
```

Two totals matching is a coincidence a renamed test survives. Byte-identical name sets is not.

---

## 4. SPEC (4) — THE TWO LEGS I DID **NOT** RUN, IN ONE LINE

**No C++ compile** — the whole diff is two comment hunks inside the header block, zero
executable lines (first executable token is `:240`), no module, no header, no `.uasset`;
there is nothing for UBT to build. **No 5b verify leg** — `TASK-1327` carries no runtime
acceptance criterion (it is a comment in a tracked tool), so `VER-§5` cl. 2 routes it
straight to 5c. A skipped leg that is not declared reads as a forgotten one.

---

## 5. SPEC (5) — THE COMMIT

**`9d80505`**, by pathspec, from the **git root one level up** (`SC-§102`). Every anchor was
proven before staging — `git ls-files --error-unmatch` for tracked, `git ls-files -o
--exclude-standard` for untracked — because a mis-anchored pathspec in this repo answers
with **silence**, not error. The index was **empty** before I touched it; the only `git add`
calls were three explicit untracked file paths (never `-A`, never `.`, never a directory).

```
 .../.claude/pipeline/CONVENTIONS.md                |  47 +++
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  |  88 ++++-
 .../pipeline/handoffs/TASK-1326-buildmaster.md     | 296 ++++++++++++++
 .../pipeline/handoffs/TASK-1327-programmer.md      | 282 ++++++++++++++
 .../.claude/pipeline/qa/TASK-1328-report.md        | 423 +++++++++++++++++++++
 GitClaudeUnrealTest/Tools/run_suite_bounded.ps1    |  14 +-
 6 files changed, 1133 insertions(+), 17 deletions(-)
```

Verified on the **commit** (`git show --stat HEAD` / `--numstat`), never on the index — the
UE Git plugin auto-stages saved assets and the index is not evidence of what shipped.

**`CONVENTIONS.md` rode along on purpose** (`TL-§5e` cl. 7b): **47 uncommitted insertions**
carrying the manager's `SC-§127` (clone-row anchors), `SC-§128` (the `--stat` phantom),
`VER-§1` cl. 7, `VER-§8` cl. 7/8 and `SC-§118` cl. 9. Law that reads as **absent at HEAD
while present on disk** is the stranding condition that has already cost this project a wave
— and two of those laws (`SC-§127`, `SC-§128`) were load-bearing *inside this very row*.

**Not pushed.** `main` is 9 ahead of origin and stays there until Jonathan asks.

### 5a. ⚖️ THE INHERITED ORPHAN — MY CALL, AND WHY

**`handoffs/TASK-1326-buildmaster.md`: SWEPT IN.**

The dispatch framed this as genuinely two-sided, noting *"the earlier host wrote that it
rides `TASK-1316`."* **I checked the source text and that premise does not hold.** The file's
only mention of `TASK-1316` is at `:217`, and it is about a different artefact entirely —
*"`TASK-1314` is live and ungated; its host is `TASK-1316`"* — a sentence about
`SiegePlayerController.cpp`, not about the handoff. What the handoff says about **itself**
is at `:293-296`:

> *"This handoff and the three flips are UNCOMMITTED by the row's own ordering … They are a
> normal cl. 7a orphan for the **next host's sweep**, exactly as `TASK-1318`'s handoff was —
> recorded here so nobody reads them as forgotten."*

I am the next host. `TL-§5e` cl. 7a says the next host sweeps, the previous host explicitly
routed it that way in writing, and the alternative would have parked a `TASK-1324`-chain
document inside an unrelated `TASK-1314` runtime-gated commit that may not land for some
time. **Swept.** The cl. 7a sweep (`ls-files -o` scoped to `.claude/pipeline/`) produced no
orphan beyond the six known paths; no new one appeared inside my window.

### 5b. 🚨 THE HELD LANE — ASSERTED AS **STATE**, BOTH WAYS

`TASK-1314` is `built` with its runtime criterion **unverified**. Committing any of it would
ship unverified code straight through the hard gate. A tidy-looking pathspec is not proof the
other lane was spared, so I asserted it as a negative *and* as post-commit state:

```
ABSENT from the commit : Source/.../SiegePlayerController.cpp
ABSENT from the commit : .claude/pipeline/handoffs/TASK-1314-programmer.md
ABSENT from the commit : .claude/pipeline/qa/TASK-1315-report.md
ABSENT from the commit : .claude/pipeline/qa/TASK-1314-verify.md

porcelain AFTER the commit:
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp   (+64/-6 uncommitted)
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1314-programmer.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1314-verify.md
?? GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1315-report.md
```

**Held, not orphaned** (`TL-§5e` cl. 7a-v) ⇒ all four are `TASK-1316`'s 5c.

---

## 6. EDITOR LIFECYCLE — CENSUS BY COMMAND LINE, AT MY OWN INSTANT

| instant | census | action |
|---|---|---|
| before | `UnrealEditor.exe` **PID 6764**, GUI, plain `.uproject`, **`-game` absent** ⇒ ours | see the check below, then closed under the standing grant |
| after close | **FIELD CLEAR**, zero `UnrealEditor*`, no headless auto-launch | ran the suite |
| after commit | relaunched ⇒ **PID 22560**, GUI, plain `.uproject`, same binaries (nothing compiled) | restored to the state I found |

The dispatch named PID 6764 and I re-censused anyway — a census is an instant, not a window
(`SC-§118` cl. 8). **Before closing I checked the editor was not carrying a live lane**,
because the dispatch warned a `playtest-verifier` might amend `qa/TASK-1314-verify.md`
mid-window: PIE had torn down 42 minutes earlier (`BeginTearingDown for
/Game/Maps/UEDPIE_0_L_Arena`, then *"Shutting down PIE online subsystems"*), the verify
report's last write was 36 minutes old, and the editor log carried nothing but EOSSDK
heartbeats and a DDC maintenance sweep. Idle, not working. **Why closed rather than run
alongside:** the runner's own W-8 guard warns a second instance contends for `Saved/`, DDC
and the asset registry, and the 561 baseline I reconcile against was measured field-clear —
same recipe, same conditions (`SC-§95`).

---

## 7. FLIPS — THREE (`SC-§103`), `Edit` ONLY, AFTER THE COMMIT

| row | now |
|---|---|
| `TASK-1327` | ✅ **done — COMMITTED `9d80505`** (its stale *"NOT YET COMMITTED"* clause corrected in the same line) |
| `TASK-1328` | ✅ **done** — report in git at `9d80505`; its declared residual marked **discharged with the measurement** |
| `TASK-1329` | ✅ **done — COMMITTED `9d80505`** |

⭐ **`SC-§127` cl. 3 re-measured at my own instant and still live.** Before flipping I counted
the bare anchor:

```
$ grep -c '^- status: backlog — **BOARDED, NOT DISPATCHED.**$'  ->  4
   at :4047  :4147  :4165  :4344      (mine is :4344)
```

**Four.** A `replace_all` would have silently flipped **three other undispatched rows** on top
of mine. All three flips were single `Edit`s anchored on task-ID-bearing text — my own row on
the unique `parallel-safe: **TASK-1329:**` line, never the colliding status line. Read back as
**state**, not as a tally: the count is now **3**, the survivors still sit at `:4047`/`:4147`/
`:4165` untouched, and the board's diff is **4 insertions / 4 deletions** — exactly the four
lines I meant to touch. No whole-file write (`SC-§120`).

---

## 8. 🙋 FOR THE MANAGER — NOMINATING, NEVER ASSIGNING (`SC-§50`)

**Nothing is owed by this row.** Two observations, offered without a suggested home:

1. **The programmer's own §7 nomination still has no row.** `TASK-1327` §7 proposed the only
   shape that ends this chain by construction rather than by another sentence asking to be
   trusted: *a test that greps this header for `:[0-9]{3,4}` outside the `:146` exhibit.*
   Six rows of this chain have now each removed one rotting citation by hand. A machine check
   is the first proposal that would catch the seventh. That is a row, not a remark.

2. **This handoff and the three flips are UNCOMMITTED by the row's own ordering** (*"COMMIT
   BEFORE YOU EDIT"*): all four carry `9d80505` and so cannot live inside it. They are a
   normal `TL-§5e` cl. 7a orphan for the next host's sweep — the identical shape `TASK-1318`
   and `TASK-1326` each left, and which I have just discharged for `TASK-1326`. Recorded here
   so nobody reads them as forgotten.

---

## 9. Files

- **Written:** this handoff · `.claude/pipeline/TASKBOARD.md` (three status flips)
- **Committed (`9d80505`):** `Tools/run_suite_bounded.ps1` · `handoffs/TASK-1327-programmer.md` ·
  `qa/TASK-1328-report.md` · `TASKBOARD.md` · `CONVENTIONS.md` · `handoffs/TASK-1326-buildmaster.md`
- **Read, never written:** `qa/TASK-1328-report.md` · `handoffs/TASK-1327-programmer.md` ·
  `handoffs/TASK-1326-buildmaster.md` · `CONVENTIONS.md`
- **Scratchpad, deleted after use:** two parse-control copies of the runner
