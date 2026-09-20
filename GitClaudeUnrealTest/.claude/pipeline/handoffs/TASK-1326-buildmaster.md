# Handoff — TASK-1326 (SUITE-RUNNER-CITATION-HOST)

author: build-master · 2026-09-19
subject row: `TASK-1324` **only** · marker `TASK-1326-SUITE-RUNNER-CITATION-HOST`
gate: `TASK-1325` (`Verdict:` = **PASS**, 0 BLOCKER) · host: this row
commit: **`f5697f3`** · `main` **8 ahead** of origin, **not pushed**

---

## 0. THE BLOCKER, DISCHARGED BY TOKEN — NEVER BY LINE ADDRESS

```
$ grep -n "Verdict:" .claude/pipeline/qa/TASK-1325-report.md
6:Verdict: **PASS** — 0 BLOCKER · 1 WARN · 2 NIT
364:**Verdict: PASS.** 0 BLOCKER · 1 WARN · 2 NIT.
```

Two lines carry the token and they agree. `SC-§126` cl. 10's point is exactly this: the position
was unconstrained (line 6, under a title), and grepping the token found it anyway.

---

## 1. SPEC (1) — THE PARSE, AND THE CONTROL THAT MAKES THE ZERO MEAN SOMETHING

### 1a. The real file

```
SHA256-BEFORE: 9EB6E605BE7DEDC9A58FF1291AFFFE7B5AC7DD1D1F80F8031F01C3E16349DD76
[System.Management.Automation.Language.Parser]::ParseFile(<tracked file>, [ref]$toks, [ref]$errs)
  $errs.Count = 0
  $toks.Count = 9598
```

### 1b. The final `#>`, RE-MEASURED at my instant rather than inherited

The row and the gate both said `:1061`. I did not use their number — I re-grepped, and it happened
to still hold. **That it agreed is a fact about today, not a licence to inherit it tomorrow.**

```
$ grep -n '#>' Tools/run_suite_bounded.ps1
234 318 378 414 521 590 649 685 804 893 960 1031 1061      (13 hits)
$ grep -c '<#' Tools/run_suite_bounded.ps1
13
```

13 openers, 13 terminators, the last at **`:1061`**. `:234` is the **header's own** — the forbidden one.

### 1c. The control FIRED — and the error text says *why* the other form cannot

Scratchpad copy only; `:1061` changed from `#>` to `# >`:

```
CONTROL errs.Count = 1
  ERR @line 1053 col 1 [MissingTerminatorMultiLineComment]
      The terminator '#>' is missing from the multiline comment.
```

⭐ **Read the line number in the error, because it is the whole argument.** The error is reported
against **`:1053`** — the **opening `<#` of the last block** — precisely because there is **no later
`<#` for the runaway comment to re-pair with**. Break the header's `#>` at `:234` instead and the
runaway swallows text only as far as the *next* of the 13 openers, closes there, and the parser
returns **0**. Same edit, same file, opposite verdict: one control detects the failure it claims
to detect, the other detects nothing and reports success. `SHIP-§9`, demonstrated rather than cited.

### 1d. Restored

```
control copy still present? False
SHA256-AFTER : 9EB6E605BE7DEDC9A58FF1291AFFFE7B5AC7DD1D1F80F8031F01C3E16349DD76   (IDENTICAL)
REAL-FILE errs.Count (re-check) = 0
```

The tracked file was never written. Hash identical either side of the control.

---

## 2. SPEC (2) — ONE SUITE RUN THROUGH THE EDITED RUNNER

```
log     Saved/Logs/run_suite_bounded_suite_20260919-152207.log
PID 13560, boot observed 12 s, wall clock 54 s
RUNNER_EXIT=0  RUNNER_STARTED=561  RUNNER_COMPLETED=561  RUNNER_SUCCESS=561  RUNNER_FAIL=0
RUNNER_SKIPPED=0   RUNNER_ECHO_MATCHED=1/1
**** TEST COMPLETE. EXIT CODE: 0 ****
```

`$LASTEXITCODE` was **0** and is **recorded, not consulted** — the verdict above is parsed from the
log, per the script's own `.NOTES` and `SC-§95`.

### 2a. THE TRIPLE

| signature | count |
|---|---|
| `HTTP 401` | **0** |
| `Unauthorized` (case-insensitive) | **0** |
| `401 Unauthorized` | **0** |
| `LogAura` (any line) | **0** |
| `Result={Fail}` | **0** |
| `LogAutomationController: Error` | **0** |
| **red count, WITH NAMES** | **0 — and the name list is empty, printed to prove it** |

### 2b. 🚨 THE DIGIT TRAP, RE-MEASURED ON MY OWN LOG

```
$ grep -c '401' run_suite_bounded_suite_20260919-152207.log
15
```

**Fifteen.** Every hit is a frame counter or a millisecond:

```
22.22.25:516][401]LogAutomation
22.22.25:517][401]LogSiegeAssis
9.19-22.22.34:401][855]LogAutom
```

⚖️ ***This project has now recorded 6, 8, 9, 14 and 15 from `grep -c '401'` — five different
answers, on five different logs, every one of them wrong, and the true value was 0 on all five.
The number is not drifting; it is noise that has never once been signal. Count the signature.***

### 2c. DELTA 0, RECONCILED **BY NAME**, NEVER BY TOTAL (`SC-§104`)

Baseline = `TASK-1318`'s own pass, `…_suite_20260919-130423.log`:

```
distinct Path={…} names  NEW  = 561
distinct Path={…} names  BASE = 561
ADDED vs baseline        = (none)
REMOVED vs baseline      = (none)
byte-identity of the two name sets = IDENTICAL
```

Two totals matching is a coincidence a renamed test would survive. Byte-identical name sets is not.

---

## 3. SPEC (3) — THE TWO LEGS I DID **NOT** RUN, SAID OUT LOUD

- **No C++ compile.** The whole diff is two hunks inside `.SYNOPSIS`/`.NOTES`. Zero executable
  lines, no module, no header, no `.uasset`. There is nothing for UBT to build.
- **No 5b verify leg.** `TASK-1324` carries no runtime acceptance criterion — it is a comment in a
  tracked tool — so `VER-§5` cl. 2 routes it straight to 5c.

A skipped leg that is not declared reads as a forgotten one.

---

## 4. ⭐ THE RESIDUAL `TASK-1325` HOMED TO ME — MEASURED AWAY, NOT INHERITED

The gate wrote, honestly, that it could not exclude *"an equal-line-count substitution **below
`:234`**, invisible to every structural check available to a reviewer without a diff"* — `Bash` was
disabled in its session and qa-reviewer holds no git access by role design.

I hold the instrument it lacked. I read the diff **whole**, and then measured its reach rather than
eyeballing it:

```
$ git show --format= -U0 HEAD -- .../run_suite_bounded.ps1 | grep '^@@'
@@ -128 +128 @@
@@ -171,2 +171,8 @@

furthest NEW-file line touched                 = 178
hunks starting at or beyond :234               = 0
```

⇒ **The change is entirely inside the header block comment. The diff touches zero lines at or
beyond `:234`.** There is no region below the header for a same-line-count substitution to hide in,
so the residual is not "spot-checked clean" — it is **empty by construction**. Answering the worry
with the complement is stronger than answering it with a sample, which is the only move a spot
check could have made.

The two hunks, for the record: `:128` swaps *"two lines earlier"* → *"a few lines earlier"* (NIT-1,
the count that was measured wrong), and `:171-178` replaces the dead `CONVENTIONS.md:5093` address
with the substring recipe.

---

## 5. SPEC (4) — THE COMMIT

`f5697f3`, by pathspec, from the **git root one level up** (`SC-§102`). Every anchor was proven
with `git ls-files --error-unmatch` (tracked) or `git ls-files -o --exclude-standard` (untracked)
**before** the commit, because a mis-anchored pathspec in this repo answers with silence, not error.

```
 .../.claude/pipeline/CONVENTIONS.md                |  19 +-
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  |  99 +++++-
 .../pipeline/handoffs/TASK-1324-programmer.md      | 356 +++++++++++++++++++
 .../.claude/pipeline/qa/TASK-1325-report.md        | 375 +++++++++++++++++++++
 GitClaudeUnrealTest/Tools/run_suite_bounded.ps1    |  12 +-
 5 files changed, 848 insertions(+), 13 deletions(-)
```

Verified on the **commit** (`git show --stat HEAD`), never on the index — the UE Git plugin
auto-stages saved assets and the index is not evidence of what shipped. The index was empty before
I touched it, and the only two `git add` calls were the two explicit untracked file paths.

**`CONVENTIONS.md` rode along on purpose** (`TL-§5e` cl. 7b): the manager's new `SC-§126` cl. 7/9
and the `DECK-§9` repairs were sitting uncommitted. Law that reads as absent at HEAD while present
on disk is the stranding condition that has already cost this project a wave.

**Not pushed.** `main` is 8 ahead of origin and stays there until Jonathan asks.

### 5a. THE OTHER CHAIN — LEFT DIRTY, ASSERTED AS **STATE**

The working tree carried two independent chains. A tidy-looking pathspec is not proof the other one
was spared, so I asserted it both ways:

```
ABSENT from the commit  : Source/.../SiegePlayerController.cpp        (negative assertion)
ABSENT from the commit  : .claude/pipeline/handoffs/TASK-1314-programmer.md

porcelain AFTER the commit:
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1314-programmer.md
```

`TASK-1314` is live and **ungated**; its host is `TASK-1316`. Committing it would have shipped code
with no PASS behind it. Under `TL-§5e` cl. 7a-v it is **scheduled, not orphaned**.

The cl. 7a sweep (`--untracked-files=all --ignored`, scoped to `.claude/pipeline/`) produced nothing
beyond the five known paths — no new orphan appeared inside my window.

---

## 6. EDITOR LIFECYCLE — CENSUS BY COMMAND LINE, AT MY OWN INSTANT

| instant | census | action |
|---|---|---|
| before | `UnrealEditor.exe` **PID 3692**, GUI, plain `.uproject`, **`-game` absent** ⇒ ours | closed under the standing grant |
| after close | **FIELD CLEAR**, zero `UnrealEditor*`; **no headless auto-launch appeared** | ran the suite |
| after commit | relaunched ⇒ **PID 12964**, same binaries (nothing was compiled) | restored to the state I found |

The dispatch named PID 3692 and I re-censused anyway before acting: a census is an instant, not a
window. **Why closed rather than run alongside:** the runner's own W-8 guard warns that a second
instance contends for `Saved/`, DDC and the asset registry, and `TASK-1318`'s 561 baseline — the
set I reconcile against — was measured field-clear. Same recipe, same conditions (`SC-§95`).

---

## 7. 🙋 FOLLOW-UP FOR THE MANAGER — THE ROW'S OWN DISEASE, RECURRING IN A **COUNT**

**Not mine to fix. Not a blocker. Nothing in `TASK-1324` was done wrong.**

`TASK-1324` replaced a rotting address with a rot-proof recipe, and shipped the recipe's expected
result beside it:

```
    grep -n "THE FILE EXISTS" .claude/pipeline/CONVENTIONS.md    (one hit)
```

I ran that recipe **verbatim** at my instant. It returns **two**:

```
4466:  ... THE LEDGER FOR ONE SENTENCE — `THE FILE EXISTS` — IN ONE WORKING DAY: ...
5152:  ... "THE FILE EXISTS" IS NOT "THE TOOL RUNS" ...
```

`:5152` is the original. **`:4466` is the manager's brand-new ledger *about* that very sentence —
which quotes it — written into `CONVENTIONS.md` after the gate measured `1`, and committed in this
same commit `f5697f3`.**

⚖️ ***The gate was right at its instant and the comment is wrong at mine, and neither is a defect.
Writing a law that QUOTES a substring is itself an act that changes that substring's hit count. A
substring citation cannot rot into the wrong place — that is the whole win of `TASK-1324` and it
holds — but its HIT COUNT still rots, and it rots for the most ironic reason available: someone
documenting the rot.***

**Suggested shape, by analogy with `TASK-1327`'s RULING 2: DELETE the parenthetical `(one hit)`,
never refresh it.** The recipe resolves either way; the count is the only part of the new text that
can be false. Homing it is the manager's call — a host nominates, it never assigns (the whole
`TASK-1310 → 1326` chain exists because two declared gaps had no rows and shipped).

---

## 8. FLIPS — THREE (`SC-§103`), `Edit` ONLY, AFTER THE COMMIT

| row | now |
|---|---|
| `TASK-1324` | ✅ **done — COMMITTED `f5697f3`** |
| `TASK-1325` | ✅ **done** — report now in git at `f5697f3`; its homed residual marked discharged |
| `TASK-1326` | ✅ **done — COMMITTED `f5697f3`** |

Made with `Edit` against re-grepped anchors, never a whole-file write (`SC-§120`; this board has
been truncated once by a Python `open(...,'w')`). Two intended anchors turned out **non-unique** —
`TASK-1329` is a near-clone host row sharing `TASK-1326`'s `parallel-safe` and `spec` opener
verbatim — so the third flip was anchored on the `blocked-by` line instead, which is unique. A
`replace_all` there would have flipped an undispatched row.

A concurrent `qa-reviewer` (`TASK-1315`) wrote to `TASKBOARD.md` inside my window, as expected. My
edits applied cleanly and I asserted the result as **state** — all three rows re-grepped and read
back — not as a line count (`SC-§104`).

⚠️ **This handoff and the three flips are UNCOMMITTED by the row's own ordering** (*"COMMIT BEFORE
YOU EDIT"*): both carry `f5697f3` and so cannot live inside it. They are a normal cl. 7a orphan for
the next host's sweep, exactly as `TASK-1318`'s handoff was — recorded here so nobody reads them as
forgotten.
