# TASK-1333 — HEADER-ADDRESS-GUARD-HOST — build-master handoff

Commit **`8cbd5d1`**, 5 files, **never pushed**. `main` 11 → **12 ahead**; `origin/main` still
`d818b5e`, re-read *after* the commit.

Gate satisfied **before** 5c, not assumed: `qa/TASK-1332-report.md` `Verdict:` = **PASS**,
**0 BLOCKER** · 2 WARN · 6 NIT · 1 declared residual. Grepped the **token** (`SC-§126` cl. 10).

---

## 1. (1) THE PARSE CONTROL — derived at my instant, and its FIRING BEHAVIOUR

The board deliberately wrote no number. I derived one:

```
$ grep -n '^#>' Tools/run_suite_bounded.ps1
238  322  382  418  525  594  653  689  808  897  964  1035  1065  1145
$ grep -n '^<#' Tools/run_suite_bounded.ps1
1  313  372  405  520  586  649  676  797  891  951  1032  1057  1089

census:  <# = 14   #> = 14     file = 1948 lines
```

**Final close = `:1145`. Pair count = 14** — *not* 13, so I was reading the post-`TASK-1331`
file and the STOP condition did not occur. Header close = `:238` (first `^#>`), **derived**.

`TASK-1329` measured 13 pairs / final `:1065` / 1825 lines. 1948 − 1825 = **123** = exactly this
diff's insertions. Two independent censuses, one instant apart, agreeing on the delta.

### Three parses, quoted

```
BASELINE (tracked file, untouched)            ParseFile errors = 0

CONTROL (DERIVED final #> :1145 broken)       ParseFile errors = 1
  line 1089: The terminator '#>' is missing from the multiline comment.

HAZARD CONTROL (inherited :1065 broken)       ParseFile errors = 0
```

The correct control's error lands on the **opener** `:1089`, because there is no later `<#` for
the runaway comment to re-pair with.

The hazard control returns **a silent zero**. Breaking `:1065` lets the runaway re-pair against
the `<#` at `:1089` and close at `:1145`; the file still terminates. **Same file, same kind of
edit, opposite verdict.** Had I inherited the row's template number, my control would have
reported "clean" while being structurally incapable of failing — `SHIP-§9`, and the
`TASK-1318` CONTROL A shape measured a third time.

`SHA256 BEFORE = E07515B16B4B3241D0BA862CA691DB52B353FAA2087F21DF514544908DFA31C6`
`SHA256 AFTER  = E07515B16B4B3241D0BA862CA691DB52B353FAA2087F21DF514544908DFA31C6` — identical.
The tracked file was never written; both controls ran on scratchpad copies, both deleted.

---

## 2. (2) + (2b) THE `-SelfTest` LEDGER — and `SC-§129`'s first invocation

**This run is the first invocation under `SC-§129`** (marker
`SC-129-KEY-THE-TRIGGER-ON-TOUCHES-NOT-ON-CODE`), which keys the trigger on *any row whose diff
**touches** `Tools/run_suite_bounded.ps1`* — never on "changes this file's code", because all
three rot events were **prose-only** edits and a code-keyed trigger would have caught none of
them while feeling correct. This row's diff touches the file; the row cannot execute; so its
**host** runs it. That is what makes the guard **not inert at birth**.

I did not inherit `TASK-1331`'s 54 → 55. I **measured the baseline**: `git show HEAD:…` into a
scratchpad tree, copied `SuiteRunnerFixtures` beside it (fixtures resolve off `$PSScriptRoot`),
and executed `-SelfTest` on the pre-change blob.

```
BASELINE (pre-TASK-1331 blob)   SELFTEST   54 / 54 cases produced their EXPECTED result.
POST     (working file)         SELFTEST   55 / 55 cases produced their EXPECTED result.
```

### The whole ledger, by name (`SC-§104`), all 55 `yes`

**1. Fixture corpus** — `green-suite.log` · `red-suite.log` · `skipped-suite.log` ·
`zero-started.log` · `zero-started-filtered.log` · `result-absent.log` · `count-mismatch.log` ·
`w9-cmd-semicolon.log` · `green-commands.log` · `no-terminator-echo.log` ·
`__does_not_exist__.log` *(11)*

**2. Bound arithmetic** — overall bound tripped · boot bound: command never echoed · stall
bound: log stopped growing · healthy run is NOT killed · slow boot inside the bound survives *(5)*

**3. Lane construction** — suite value is `'Automation RunTests Siegebound;Quit'` · suite value
honours a sub-group filter · command value is `'A,B,QUIT_EDITOR'` (COMMAS + terminator) · a
caller-supplied `'Quit'` is DROPPED, QUIT_EDITOR appended · caller separator `';'` inside a
command is REFUSED (exit 64) · caller separator `','` inside a command is REFUSED (exit 64) · an
empty command list is REFUSED (exit 64) · a command list of ONLY terminators is REFUSED (exit
64) · W-4: a `-Filter` carrying `';'` is REFUSED · W-4: a `-Filter` carrying `','` is REFUSED ·
W-4: a `-Filter` carrying a quote is REFUSED · W-4: an empty `-Filter` is REFUSED *(12)*

**4. Expected echoes** — suite expects exactly 1 echo, without `";Quit"` · B-1: command lane
requires 2 echoes and NOT the terminator · symmetry: `Get-ExpectedEchoes` refuses `';'` like
`New-ExecCmdsValue` · symmetry: `Get-ExpectedEchoes` refuses `','` like `New-ExecCmdsValue` ·
symmetry: `Get-ExpectedEchoes` refuses a mangled `-Filter` *(5)*

**5. Command line** — `New-EditorCommandLine` returns `[string]`, not `[string[]]` ·
`-ExecCmds="Automation RunTests Siegebound;Quit"` survives verbatim · paths quoted;
`-nullrhi`/`-unattended` present · command lane emits the COMMA form with QUIT_EDITOR *(4)*

**5b. Aura exclusion** — `-DisablePlugins=Aura` present verbatim in BOTH lanes · the flag guard
REFUSES 4 degenerate lines, still accepts the real one *(2)*

**6. Bound merge** — B-2: green log + tripped bound → exit 6, NOT 0 · no bound tripped → the log
verdict is untouched *(2)*

**7. `-LogPath` safety** — `'Saved\Logs\run.log'` · `'C:\proj\Saved\Logs\run.log'` ·
`'Content\Meshes\SM_Rock_01.uasset'` REFUSED · `'Saved\Logs\run.txt'` REFUSED ·
`'C:\Windows\Temp\run.log'` REFUSED · `'Saved\..\Content\run.log'` REFUSED · `''` REFUSED *(7)*

**8. Input tolerance** — `Get-CmdEchoes` survives BLANK LINES · `Get-CmdEchoes` survives an EMPTY
array · `Test-SuiteCounts` survives BLANK LINES and still counts · zero echoes is NOT a pass;
missing terminator is reported · B-1: terminator echo is corroboration, never a required match ·
W-10: incremental log reader advances and never re-reads · ⭐ **`SC-126: header grows no
rot-prone line address`** *(7)*

The **new** case reports: *"header derived as lines 1..238; 6 exhibit line(s) excluded by
substring, never by address."* Its own derivation of `1..238` **agrees with mine, independently
arrived at** — I derived the header close by grepping `^#>`, it derives it via `Parser::ParseFile`.

**Reconciliation.** Every one of the 54 baseline names appears verbatim in the 55-case ledger,
same section, same order, still `yes`. The delta is exactly one case. This is corroborated
structurally: **the diff has ZERO deletions**, so no pre-existing case *can* have been dropped
or altered — a stronger guarantee than a name diff, because it forecloses the failure rather
than merely failing to observe it.

`-SelfTest` is process-pure (no spawn constructs) but reads a fixture corpus — disk, not process.

---

## 3. THE GATE'S DECLARED RESIDUAL — CLOSED

`TASK-1332` had no `Bash`, so it could not run `git diff` and could not prove the two claimed
hunks were the *only* hunks; an equal-line-count substitution below `:238` was invisible to
every check it ran. It refused to route git through the read-only inspector. **That refusal was
correct, and the residual it declared instead of guessing is exactly what let me close it.**

```
$ git diff -U0 -- .../run_suite_bounded.ps1 | grep '^@@'
@@ -1088,0 +1089,113 @@ function Add-ThrowCase {
@@ -1490,0 +1604,10 @@ function Invoke-SelfTest {

hunk count                              = 2   (exactly the two claimed)
numstat                                 = 123 insertions / 0 deletions
hunks at or beyond the header's :238    = 0   (both start at 1089 / 1604)
```

**A substitution is ruled out by construction: it requires a deletion, and there are zero
deleted lines anywhere in the diff.** 113 + 10 = 123 = numstat, so the hunks account for the
whole change with nothing left over.

Corroborated independently by content rather than by position:

```
sha256(lines 1..238) on HEAD           = 072f55ceb8bc4ad189fcacdbabf5e97c6e912028150c3891e19ef55f08593a2e
sha256(lines 1..238) on working copy   = 072f55ceb8bc4ad189fcacdbabf5e97c6e912028150c3891e19ef55f08593a2e
```

The header block is **byte-identical**. `TASK-1336` check (5) starts satisfied.

---

## 4. (3) ONE SUITE RUN THROUGH THE EDITED RUNNER

```
log   Saved/Logs/run_suite_bounded_suite_20260920-015622.log   (7204 lines)
PID 27200, boot observed 8 s, wall clock 48 s
RUNNER_EXIT=0  RUNNER_STARTED=561  RUNNER_COMPLETED=561  RUNNER_SUCCESS=561  RUNNER_FAIL=0
RUNNER_SKIPPED=0  RUNNER_ECHO_MATCHED=1/1
```

`$LASTEXITCODE` was 0 and is **recorded, not consulted** (`SC-§95`).

### The triple

| signature | count |
|---|---|
| `HTTP 401` | **0** |
| `401 Unauthorized` | **0** |
| `Unauthorized` (case-insensitive) | **0** |
| `LogAura` (any line) | **0** |
| `Result={Fail}` | **0** → red list is **EMPTY** |

⚠️ The bare-digit grep `grep -c '401'` returned **7**. All seven are the log's **frame counter**
in the timestamp prefix — `[2026.09.20-08.56.37:045][401]LogSiegeAssistant:`. **This is now the
seventh consecutive log where the digits lie and the signature truth is 0.**

**Delta 0 vs the `9d80505` baseline (561/561), reconciled BY NAME:** `TASK-1331` adds no C++
automation test — a PowerShell `-SelfTest` case lives outside the UE automation suite entirely —
so the added-name set and the removed-name set are both **empty**, and 561 = 561.

---

## 5. (4) THE TWO SKIPPED LEGS, DECLARED

**No C++ compile:** the diff is PowerShell in a tool — no module, no header, no `.uasset` — so
there is nothing for UBT to build. **No 5b:** the row carries no runtime acceptance criterion, so
`VER-§5` cl. 2 routes it straight to 5c. A skipped leg that is not declared reads as a forgotten one.

---

## 6. (2c) THE WARNs ARE NOT MINE

`TASK-1332` left 2 WARN / 6 NIT, boarded at `TASK-1335` → `1336` → `1337`. **I did not hand-edit
the header or the helper to discharge them**, and the byte-identical header hash above is the
proof rather than the promise. The gate's own words apply: a host "just fixing" this header would
be the seventh hand-edit wearing a build-master's hat. **No WARN blocks this commit**, and I did
not treat any as though it did.

---

## 7. (5) THE COMMIT — pathspec derived at my instant

Git root is one level up at `C:\GitProjects\GitHub\GitClaudeUnrealTesting` (`SC-§102`). Every
anchor proven before staging: tracked paths with `git ls-files --error-unmatch`, untracked with
`git ls-files -o --exclude-standard`.

- **`CONVENTIONS.md` re-measured CLEAN** ⇒ `TL-§5e` cl. 7b **did not apply**. The last host read
  it clean too; I did not inherit that — I re-measured, and it could have changed.
- **cl. 7a orphan measured, not assumed:** `handoffs/TASK-1316-buildmaster.md` —
  `git log --all -- <path>` returned **empty** (never committed on any ref) and `ls-files -o`
  listed it ⇒ a genuine orphan, swept in.
- **Hard fences:** `SiegePlayerController.cpp`, `Content/UI/WBP_VictoryScreen.uasset` and
  `qa/TASK-1314-verify.md` were all measured **clean** — nothing of `TASK-1314`'s chain was dirty,
  so there was nothing to hold back and nothing staged.
- **Index checked before staging and found empty** — the UE Git plugin had not auto-staged, and
  the editor was down for the whole commit, so nothing could stage mid-flight. Verified on the
  **commit**, never on the index.

```
$ git show --numstat HEAD          (SC-§128 — numstat, not --stat's line total)
8cbd5d1
3     3   GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
176   0   GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1316-buildmaster.md
343   0   GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1331-programmer.md
396   0   GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1332-report.md
123   0   GitClaudeUnrealTest/Tools/run_suite_bounded.ps1
```

`run_suite_bounded.ps1` lands at exactly the **123 / 0** measured pre-commit. Committed with
`-F <file>` — `git commit -- <paths> -m <msg>` eats the `-m` as a pathspec. **Never pushed.**

---

## 8. (6) THE THREE FLIPS — after the commit, `Edit` only

`TASK-1331` · `TASK-1332` · `TASK-1333`. **Committed before editing the board.** No `replace_all`
(`SC-§127`); three single `Edit`s on anchors verified unique by `grep -c` first (each returned
exactly 1); each read back as **state** by re-deriving the `- status:` line from the row, not by
trusting the edit's own success return. Board diff is **3 insertions / 3 deletions, 3 hunks** —
one flip per row, and `8cbd5d1` appears exactly 3 times.

---

## 9. ENVIRONMENT

Editor censused **by command line** at my instant (`SC-§118` cl. 8 — a census is an instant, not
a window): **one** process, PID 22560,
`"…\UnrealEditor.exe" "…\GitClaudeUnrealTest.uproject"` — **`-game` token ABSENT** ⇒ not
Jonathan's. Closed under the standing grant for the suite run; **left down**. No `-game` instance
existed at any point, so none was killed or driven.

---

## 10. STATE LEFT BEHIND — dirty BY DESIGN for `TASK-1337`

- `TASKBOARD.md` — the three flips above, uncommitted (the established host-to-host pattern)
- `handoffs/TASK-1333-buildmaster.md` — this file, untracked

Both are the cl. 7a orphans the **next** host sweeps, exactly as `TASK-1316` left me its own.

**Written:** this handoff · `TASKBOARD.md` (three flips).
**Committed (`8cbd5d1`):** the five paths above.
**Read, never written:** `qa/TASK-1332-report.md` · `handoffs/TASK-1331-programmer.md` ·
`handoffs/TASK-1316-buildmaster.md` · `Tools/run_suite_bounded.ps1` (parse controls ran on
copies).
**Scratchpad, deleted after use:** two parse-control copies · the baseline tree
(`baseline/Tools/` + fixture corpus).

**This unblocks `TASK-1335`. I did not start it.**
