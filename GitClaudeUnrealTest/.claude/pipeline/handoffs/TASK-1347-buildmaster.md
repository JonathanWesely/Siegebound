# TASK-1347 — VERIFY-LANE-ARGUMENT-COUPLING-HOST (build-master)

**Commit host for `TASK-1345`** (the widened `SC-§135` coupling comment in `SiegePlayerController.cpp`).
Gate: `TASK-1346`, `Verdict:` = **PASS**, 0 BLOCKER / 0 WARN / 3 NIT.
Chain run: **5a COMPILE → RELAUNCH → 5c COMMIT**. ⛔ **NO 5b — declared, not omitted.**

Started **2026-09-20**. This file was opened at the **5a leg**, not at 5c (board acceptance (7)).

---

## 0. THE FLIP CAME FIRST (`SC-§134` cl. 7/8)

`TASKBOARD.md:4731` left `backlog` **before `Build.bat` was started**, not after it returned. The line it
carried was **`in-progress` — 5a COMPILE RUNNING**, ⛔ **not `built`** — asserting an outcome before its
evidence exists is a false state. The prior `backlog` wording was **struck, not deleted**, because deleting it
deletes the proof the flip was early.

This is the law's **third** consecutive discharge (`TASK-1343` → `TASK-1340` → this row).

**Anchor collision counts, measured at my own instant (`SC-§127`), not inherited:**

| Anchor | Collisions |
|---|---|
| bare `^- status:` | **1344** |
| `status: backlog` | **156** ⟵ the dispatch inherited **157**; it had already decremented |
| `(⭐ \`TASK-1347\`)` discriminator | **1** ⟵ used |

`Edit` only (`SC-§120`), never `replace_all`. Board was **37,554** lines before and **37,554** after — it did
not shrink. Every flip read back as **state**, never assumed from the tool's success return.

⭐ The `156` is the point of the law. The inherited number was **right when it was written and wrong when I
read it**; a host that had trusted it would have reported a count that no longer described the file.

---

## 1. EDITOR CENSUS — BY COMMAND LINE, RE-MEASURED (`SC-§118` cl. 8)

`TASK-1340` handed over the claim "0 `UnrealEditor*.exe`". I did **not** inherit it — the field refills itself.

```
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'GitClaudeUnrealTest%'
                                       OR Name='UnrealBuildTool.exe'"
```

⇒ **ZERO rows.** No editor, no UBT, and ⛔ **no `-game` instance of 🧑 Jonathan's**. Nothing to close, nothing
to ask about, MCP :8000 down. That is exactly the state 5a wants, and it was **measured**, not assumed.

---

## 2. 5a COMPILE — MANDATORY ON A COMMENT-ONLY DIFF, AND THIS IS WHY

Standing line, `-waitmutex`, ⛔ never Live Coding, ⛔ never `Ctrl+Alt+F11`.

### 2.1 `Result:` PARSED FROM THE LOG (`UE-§ exit-code-lies`)

```
Result: Succeeded
```

`Result: Succeeded` = **1**, `Result: Failed` = **0**, at log line 27. The raw exit code was `0` and is
**recorded as NOT EVIDENCE** — `Build.bat` returns 0 on a failed build.

`0x800711C7` (Smart App Control) = **0 hits**. Total execution time **7.44 s**.

### 2.2 ⭐ PROOF IT REALLY RECOMPILED — THE LEG THAT IS NOT A FORMALITY

A clean compile of **nothing** prints an identical `Result: Succeeded`. Two lines discriminate:

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegePlayerController.cpp
[1/4] Compile [x64] SiegePlayerController.cpp
```

plus `[2/4] Link`, `[3/4] Link`, `[4/4] WriteMetadata` — **4 actions**, not 0.

🚨 **A near-miss worth recording.** The log came back **29 lines in 7 s** — byte-for-byte the *shape* of
`TASK-1343`'s no-op-looking run. I had written down "that is a no-op" before opening the file. It was **not**:
the content proves a real single-TU recompile and relink. ⛔ **The shape of a log is not its content**, and the
`Compile [x64]` requirement exists precisely because the cheap signal (duration, line count, `Result:`) cannot
tell the two apart. Had I reported from the shape I would have been wrong in both directions at once.

### 2.3 WARNING DELTA (`SC-§122` cl. 6)

`grep -ciE 'warning|error'` over the **entire** 29-line log ⇒ **0 hits**.

⛔ **Complement control run** (`SC-§137`): `grep -ciE 'succeeded|compile'` over the same file ⇒ **4**. The probe
is live, not silently broken — a bare `0` from an untested grep is indistinguishable from a broken one.

Baseline: `TASK-1343` recorded absolute **0** (`handoffs/TASK-1343-buildmaster.md` §3.2).
⇒ **Warning delta = 0; absolute = 0.**

---

## 3. RELAUNCH — STATED WITH ITS PROOF

Relaunched GUI on the `.uproject` under the standing grant.

- **PID 27484**, `UnrealEditor.exe`, command line `"...UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
  "...\GitClaudeUnrealTest.uproject"` ⇒ **GUI on the project, mine**, not a `-game` instance.
- **Proof it is on the NEW binaries, not merely "it started":**
  `UnrealEditor-GitClaudeUnrealTest.dll` `LastWriteTime` = **11:41:26**, 9,811,968 bytes;
  process `StartTime` = **11:42:09**. The editor started **43 s after** the binary was written ⇒ it loaded the
  DLL this compile produced. ⛔ A relaunch that merely *happened* proves nothing; the ordering is the evidence.

---

## 4. NO 5b — DECLARED, NOT FORGOTTEN (`VER-§5` cl. 2)

`TASK-1345` carries **no runtime acceptance criterion**. A comment-only diff has nothing runtime-observable to
witness. ⛔ **This is NOT an `UNOBSERVABLE`** — an `UNOBSERVABLE` is a lane that ran and could not see; this
lane was **never owed**. Recorded so a later reader cannot mistake the absence for an omission.

**No suite run owed** either (board `names:`): a comment-only C++ diff changes no test. Baseline stands at
`TASK-1340`'s **561/561** at `d9a98d1`, reconciled **by name** there and untouched by this row. ⛔ I did not
re-run it and I do ⛔ **not** claim a fresh 561 — an unrun suite has no number.

---

## 5. INDEPENDENT RE-VERIFICATION OF THE COMMENT-ONLY CLAIM

I did **not** ship this on the QA report's word. Re-measured at my own instant:

| Probe | HEAD | Worktree |
|---|---|---|
| non-comment projection `sha256` | `ba752038…3f95d2` | `ba752038…3f95d2` |
| projection line count | 4667 | 4667 |
| `diff` of projections | **IDENTICAL** | |

45 added / 21 removed; **non-comment added = 0**, **non-comment removed = 0**. Reproduces `TASK-1346` §2.2's
published hash exactly ⇒ **zero behavioural bytes anywhere in the file**, not merely inside the diff. That is
what made a `Result: Succeeded` sufficient here with no runtime lane behind it.

---

## 6. PATHSPEC — DERIVED AT MY INSTANT, EVERY ANCHOR PROVEN

`SC-§102`: git root is **one level up** at `C:\GitProjects\GitHub\GitClaudeUnrealTesting`; a mis-anchored
pathspec answers with **silence**. Tracked anchors proven with `ls-files --error-unmatch`, untracked with
`ls-files -o`. ⛔ Never `-A`, never `.`, never a bare directory.

| Path | State | How measured |
|---|---|---|
| `Source/.../SiegePlayerController.cpp` | ` M` 45/21 | `--error-unmatch` ⇒ TRACKED |
| `handoffs/TASK-1345-programmer.md` | `??` | `ls-files -o` ⇒ listed; `git log --all` ⇒ **no history** |
| `qa/TASK-1346-report.md` | `??` | `ls-files -o` ⇒ listed; `git log --all` ⇒ **no history** |
| `TASKBOARD.md` | ` M` 2/2 | `--error-unmatch` ⇒ TRACKED |
| `handoffs/TASK-1340-buildmaster.md` | ` M` 61/3 | `git log --all` ⇒ committed at `d9a98d1`, modified after |
| `handoffs/TASK-1347-buildmaster.md` | `??` | this file |

**`CONVENTIONS.md` is NOT in the pathspec — and that is a measurement, not an omission.** `git status
--porcelain` and `git diff --numstat` both return **empty** for it. ⛔ The dispatch warned that `SC-§130` fired
live on the previous host when *section-token* probes read clean because new clauses live inside existing
sections. That hazard is **content-token** shaped; `git status` is **byte-level** and has no such hole, so the
byte-level probe is the one that settles it. `TASK-1340` swept it at `d9a98d1` (40/2).

**`TASK-1340`'s bounded-at-two tail, which named me as its taker — swept, both halves measured:**
its `done` flip at `TASKBOARD.md:4622` and `handoffs/TASK-1340-buildmaster.md`. Both were dirty for exactly
one reason: **a commit hash cannot exist before its commit.**

⛔ **`TASKBOARD.md` diff is 2 hunks and only 2** — `@@ -4622 +4622 @@` (`TASK-1340`'s `done`) and
`@@ -4731 +4731 @@` (my own flip). No other row touched, verified with `git diff -U0 | grep '^@@'`.

**FENCED LANE — `TASK-1338`/`1344`, re-measured, all four read CLEAN** (committed at `d9a98d1`):
`Tools/run_suite_bounded.ps1` · `handoffs/TASK-1338-programmer.md` · `qa/TASK-1339-report.md` ·
`qa/TASK-1344-report.md`. Nothing of that lane is staged or committed here.

**INDEX CHECKED BEFORE ANY `add`** (`git diff --cached --numstat` ⇒ empty). The UE Git plugin auto-stages on
save/import; it had staged nothing, and the commit is by explicit pathspec regardless. ⛔ The index is never
the evidence — the **commit** is (`git show --numstat HEAD`).

`SC-§133` held: I re-censused at my own instant rather than trusting the inherited list. This time **nothing
appeared mid-row** (the previous host saw `qa/TASK-1346-report.md` materialise after its census). ⛔ That the
list happened to match is not a reason to stop re-censusing — it is the one time the check was cheap.

---

## 7. CARRIED, NOT FIXED — FOR THE MANAGER

Pre-existing NITs from `TASK-1346`, ⛔ **not** this row's work, each needing its own row:

1. ⭐ **Conjunct D's truth is owned by THREE code sites in TWO directions**, including
   `Add-SelfTestCase`'s `-Row` branch **~130 lines above** the comment, which drops the detail entirely. A
   future `-Row` on that one call would **falsify the shipped comment with comment, predicate and returns all
   untouched** — the comment would still read true and would not be.
2. The **TOCTOU-throw** case, which does **not** falsify the clause (recorded so it is not re-raised).
3. A bare `(SC-§136)` provenance token on the modality line — zero lines, available to any future touch.
