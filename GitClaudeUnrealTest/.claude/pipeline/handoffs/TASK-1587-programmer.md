# TASK-1587 — SHIP-GATE-FOLLOWUPS — programmer handoff

Marker `TASK-1587-SHIP-GATE-FOLLOWUPS`. Date 2026-09-28. Author: gameplay-programmer. Gate: `TASK-1588`. Host: `TASK-1580`.

All four items are done, and **no verdict logic moved**. Both arms were run and seen red on the fourth case's **verdict**, then restored byte-equal to (0). Both harnesses exit 0 on the final bytes. `check-attr` reads `unset` on the three `.log` fixtures and `unspecified` on the control.

**Declared runs (`SC-§71a`).** Everything below comes from these read-only or offline commands:
- `sha256sum`;
- `git --no-optional-locks` `rev-parse`, `hash-object`, `ls-files -s`, `diff -U0` and `check-attr`;
- the two fixture harnesses under Windows PowerShell 5.1, against log FILES only;
- a Python 3.14 mutation script that rewrote the working copy of `ship.ps1` and restored it from a byte copy (§2).

I ran no mutating git, no `ship.ps1`, no cook, no build and nothing Unreal. A `playtest-verifier` holds PIE on PID 12112, and I did not touch it.

## 0. Start state (spec (0)): all equal, so no STOP

| file | expected | measured |
|---|---|---|
| `Tools/Packaging/ship.ps1` | `02c261f6…3debd` | `02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd` |
| `Tools/Packaging/Fixtures/check_cook_bp_verdict.ps1` | `b48e4369…c9c7` | `b48e4369ea8a7690f896ef2b0660a2a59f316f35f1826f6e522986de0b73c9c7` |
| `Tools/Packaging/Fixtures/cook-bp-green.log` | `b430b2f4…d444` | `b430b2f4013a7064d729be4acaad4d4920b420f4639089b6b7bbcc7cc19fd444` |
| `Tools/Packaging/Fixtures/cook-bp-error-red.log` | `2b0fa374…1b40` | `2b0fa374e85549c7f808606623124e03f911cfe61e9d19d23d28774143281b40` |
| `Tools/Packaging/Fixtures/empty.log` | `e3b0c442…b855` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `.claude/commands/ship.md` | HEAD blob | `hash-object` `124fe7593db0680c169c12e4000e1d30c320c4d0` = `ls-files -s` `124fe7593db0680c169c12e4000e1d30c320c4d0` |
| root `.gitattributes` | HEAD blob | `hash-object` `59cc7f952786365f1e713daaefd0d52b96ae0e26` = `ls-files -s` `59cc7f952786365f1e713daaefd0d52b96ae0e26` |

- HEAD is `9b82e8d99e8b09447f2bcdb13a31db5c0f2e88e4`.
- `git rev-parse --show-toplevel` returns `C:/GitProjects/GitHub/GitClaudeUnrealTesting`, one level above the project. So the file I edited is the git root's `.gitattributes`.
- Line endings at start: `ship.ps1`, the harness and `ship.md` are LF-only. `.gitattributes` is CRLF (9 CR, 9 LF). Every edit kept each file's convention (§6).
- **The harness's start bytes are proven, not only asserted.** I did not byte-copy the harness before editing it. Afterwards I reversed my three edits on a copy (`scratchpad/unedit_harness.py`) and got `b48e4369…c9c7`: `equal-to-START=True`. So the diff in §1 is exactly my change.

## 1. Item (1): the fourth case

### The new case, in full (`check_cook_bp_verdict.ps1`, `diff -U0` from the proven start bytes)

```
@@ -18,0 +19,7 @@
+  A fourth case pairs the two real captures with each other (TASK-1587,
+  qa/TASK-1568.md WARN-2), so the gate sees a needle line AND a summary line:
+    * pair   cook-bp-error-red.log + cook-bp-green.log => STOP / BLUEPRINT-ERROR,
+             summary count 1.  The 'Errorx' needle mutant and a summary-first
+             order swap each flip THIS case's VERDICT to PASS (SHIP-9), where the
+             triple sees the first only as a changed reason and the second not
+             at all.
@@ -102,0 +110,17 @@
+''
+'--- 2b. THE PAIR: the two real captures as ONE (stdout, stderr) pair - the VERDICT discriminates, not only the reason ---'
+# TASK-1587 (qa/TASK-1568.md WARN-2).  The triple's RED side carries no summary
+# line, so the 'Errorx' needle mutant only moves its REASON (BLUEPRINT-ERROR ->
+# SUMMARY-ABSENT, still STOP), and a swap that checks the summary first and
+# passes on it passes all three.  Here the red capture is the stdout member and
+# the green capture the stderr member, so the gate reads 4 needle lines AND one
+# summary line: only the needle-first order stops it.  No new bytes, no
+# synthetic file - the same two tracked fixtures, paired.  This pair is NOT a
+# real cook's shape (a real one carrying these errors would print
+# 'Failure - N error(s)'); it is two real captures combined so that both
+# branches the order decides between are armed at once.
+$vp = Get-CookBlueprintVerdict -Paths @($red, $green); Show 'PAIR' $vp
+Check 'PAIR  cook-bp-error-red.log + cook-bp-green.log -> STOP / BLUEPRINT-ERROR with the summary present (the needle is read first)' `
+    (($vp.Verdict -eq 'STOP') -and ($vp.Reason -eq 'BLUEPRINT-ERROR') -and (-not $vp.Ok) -and ($vp.BpErrorCount -eq 4) -and ($vp.Quoted.Count -eq 4) -and ($vp.SummaryCount -eq 1) -and ($vp.SummaryErrors -eq 0) -and ($vp.LogsRead -eq 2)) `
+    (Facts $vp)
+
@@ -116 +140 @@
-'RESULT: ALL EXPECTATIONS MET on all three sides'
+'RESULT: ALL EXPECTATIONS MET on all three sides and the pair'
```

- **Unchanged:** the three existing cases, the AST lift and the REAL section. The `--- 3. THE REAL RUN` heading keeps its number; the new section is `2b`, so nothing existing is renumbered.
- **Two edits beyond the case itself, both mine to declare:**
  - a header bullet documenting the case;
  - the RESULT line's wording, since "all three sides" would be false with four checks.
- Nothing parses the RESULT text. Grep for `ALL EXPECTATIONS MET` outside `.claude/pipeline` finds only the two harnesses' own lines.
- **The assertion carries the spec's items and more.** The spec asks for `STOP` / `BLUEPRINT-ERROR` with summary count 1. I also assert:
  - `Ok` is false;
  - `BpErrorCount` is 4, `Quoted` is 4 and `SummaryErrors` is 0 (the summary read is the green capture's line);
  - `LogsRead` is 2.

### The arms (`SHIP-§9`): each run on the (0) bytes of `ship.ps1` in the working copy, then restored

**Method, declared.**
- `scratchpad/arm1587.py backup` refuses unless `ship.ps1` hashes to `02c261f6…`, then byte-copies it to `ship.ps1.orig`.
- Each arm builds its mutant from `.orig`, so arms never stack.
- The swap cuts the three branch bodies out of the function's own text; it does not retype them. It refuses unless each marker occurs exactly once, in order, and the bodies are A=`BLUEPRINT-ERROR`, B=`SUMMARY-ABSENT`, C=PASS.
- `restore` byte-copies `.orig` back, re-hashes, and exits 3 if the hash is not START.
- Each arm ran inside `try { mutate; harness } finally { restore }`.
- **These arms ran before the item (3) comment edits.** So "restored and re-hashed equal to (0)" is literally true at each restore.

**Arm (i), the needle mutant.** `$BP_ERROR_NEEDLE = 'LogBlueprint: Errorx'`, at `ship.ps1:906`, mutant sha `a659fb81…d282`.

```
  [OK]       GREEN ... Verdict=PASS Reason=NO-BLUEPRINT-ERROR ...
  [MISMATCH] RED   cook-bp-error-red.log -> STOP / BLUEPRINT-ERROR, 4 hits quoted, all BP_Basic_Movement  --  Verdict=STOP Reason=SUMMARY-ABSENT Ok=False BpErrorCount=0 Quoted=0 SummaryCount=0 SummaryErrors=-1 LogsRead=2/2
  [OK]       NULL  ...
  PAIR   [PASS] C2-COOK-BP-ERRORS  verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 'LogBlueprint: Errorx' lines (2 of 2 logs read); cook summary [cook-bp-green.log:11] LogInit: Display: Success - 0 error(s), 3 warning(s) (its error count 0 is printed, not gated)
  [MISMATCH] PAIR  cook-bp-error-red.log + cook-bp-green.log -> STOP / BLUEPRINT-ERROR with the summary present (the needle is read first)  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=0 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
RESULT: 2 MISMATCH(ES) - the gate is NOT validated
HARNESS EXIT=2
restored ship.ps1 sha256=02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd  equal-to-START=True
```

- The fourth case's **verdict** flips to `PASS`. That is the false PASS WARN-2 said the triple could not show.
- RED still catches it only by reason, as it did before.

**Arm (ii), the order swap: the summary check moved ahead of the needle check.** Mutant sha `38194fff…c31a`. Its chain heads:

```
  ship.ps1:924: if ($sumHits.Count -gt 0) {        <- PASS body
  ship.ps1:930: } elseif ($bpHits.Count -gt 0) {   <- BLUEPRINT-ERROR body
  ship.ps1:939: } else {                           <- SUMMARY-ABSENT body
```

(`:918` is the unrelated `if ($sumHits.Count -gt 0) {` that reads the summary line. It is not part of the chain.)

```
  ship.ps1 parses clean: 0 parse errors
  [OK]       GREEN ... Verdict=PASS Reason=NO-BLUEPRINT-ERROR ...
  [OK]       RED   ... Verdict=STOP Reason=BLUEPRINT-ERROR Ok=False BpErrorCount=4 Quoted=4 SummaryCount=0 ...
  [OK]       NULL  ... Verdict=STOP Reason=SUMMARY-ABSENT ...
  PAIR   [PASS] C2-COOK-BP-ERRORS  verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 'LogBlueprint: Error' lines (2 of 2 logs read); cook summary [cook-bp-green.log:11] LogInit: Display: Success - 0 error(s), 3 warning(s) (its error count 0 is printed, not gated)
  [MISMATCH] PAIR  cook-bp-error-red.log + cook-bp-green.log -> STOP / BLUEPRINT-ERROR with the summary present (the needle is read first)  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=4 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
RESULT: 1 MISMATCH(ES) - the gate is NOT validated
HARNESS EXIT=2
restored ship.ps1 sha256=02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd  equal-to-START=True
```

- The triple stays 3/3. This is measured confirmation of QA ruling 2: the triple cannot see this swap.
- Only the fourth case catches it, and it catches it on the **verdict**.
- Note that the mutant's PASS evidence prints "0 … lines" while `BpErrorCount=4`. The PASS body's text hard-codes `0`. That is correct for the real code, where PASS is reachable only with 0 hits; it is only the mutant that makes it lie.

**Supplementary run, not a row arm (declared so it is not mistaken for one).** "Summary check ahead of the needle check" has a second, literal reading: exchange the two conditions in place, giving `if (summary absent) SUMMARY-ABSENT elseif (needle) BLUEPRINT-ERROR else PASS`. I ran it the same way (mutant `05ef8a87…0f3a`):
- **RED** `[MISMATCH]`: reason `SUMMARY-ABSENT`;
- **PAIR** `[OK]`: `STOP`/`BLUEPRINT-ERROR`;
- exit 2; restored `equal-to-START=True`.

That reading never yields a false PASS, and the existing RED case already catches it by reason. The row's arm (ii) is the reading that must "make the fourth case read PASS", and that is the one run above.

### Green after the arms

The green run on (0) with the new case: all five `[OK]` (empty-size, GREEN, RED, NULL, PAIR) plus REAL `[OK]`, `RESULT: ALL EXPECTATIONS MET on all three sides and the pair`, `EXIT=0`. The final-bytes transcript is in §5.

## 2. Item (2): the `ship.md` gate-table row

Raw output of `git --no-optional-locks diff -U0 -- .claude/commands/ship.md` (read-only):

```
diff --git a/GitClaudeUnrealTest/.claude/commands/ship.md b/GitClaudeUnrealTest/.claude/commands/ship.md
index 124fe75..ee009ec 100644
--- a/GitClaudeUnrealTest/.claude/commands/ship.md
+++ b/GitClaudeUnrealTest/.claude/commands/ship.md
@@ -102,0 +103 @@ What it does, in order — **each one a STOP**:
+| C | `C2-COOK-BP-ERRORS` | Reads **the same cook logs** as `C2-UAT-LOG` (UAT's captured stdout + stderr) for a Blueprint compile error (`PKG-§14`: anything in a `-COOKDIR` cooks whether or not a map references it). **Three outcomes:** any `LogBlueprint: Error` line ⇒ **STOP `BLUEPRINT-ERROR`**, quoting the first 5 hits · otherwise no `Success - …` / `Failure - …` cook summary line ⇒ **STOP `SUMMARY-ABSENT`** (a log that cannot say is not a clean log) · otherwise **PASS**, quoting the summary (its error count is printed, not gated). ⚠️ **What it cannot claim:** that the **cooker** writes that line for a broken Blueprint in a `-COOKDIR` is ⛔ **not measured** (the red fixture is an editor log, `VER-§12` cl. 7g); and by an **engine-source reading**, not a run, an Error-level cook error forces the cook's exit code to 1, so in today's recipe **`C2-UAT-LOG` STOPs first** — this gate is defence in depth for a swallowed cook failure |
```

It is one pure insertion, directly after the `C1-COOK` / `C2-UAT-LOG` row (`:102`). Nothing else in the file moved. Git also printed the expected autocrlf warning: `LF will be replaced by CRLF the next time Git touches it`.

## 3. Item (3): comment and remedy text in `ship.ps1`

`diff -U0` from the (0) bytes (`scratchpad/ship.ps1.orig`, sha `02c261f6…`) to the final file:

```
@@ -868 +868,5 @@
-# SUMMARY-ABSENT.  The gate prints exactly the strings this returns.
+# SUMMARY-ABSENT.  A PAIR case (TASK-1587; not the owed "fourth side" below)
+# runs the red and green captures together => STOP BLUEPRINT-ERROR with a
+# summary present, so a broken needle or a summary-first order flips that
+# case's VERDICT to PASS (SHIP-9).  The gate prints exactly the strings this
+# returns.
@@ -873 +877,28 @@
-# error in the cook.
+# error line in the cook - but see THE PREMISE, RECONCILED, below.
+#
+# THE PREMISE, RECONCILED (TASK-1587, qa/TASK-1568.md WARN-1).  This is a
+# READING of the UE 5.8 engine source, NOT a measurement:
+#   - An Error-level Blueprint compile error in the cook is counted as an
+#     error, so the commandlet prints 'Failure - N error(s), M warning(s)' and
+#     its exit code is forced to 1 (LaunchEngineLoop.cpp: "Return an non-zero
+#     code if errors were logged and UseCommandletResultAsExitCode is false").
+#     UAT then throws "Cook failed." (CookCommand.Automation.cs; this recipe
+#     does not pass -IgnoreCookErrors), never prints BUILD SUCCESSFUL, and
+#     C2-UAT-LOG STOPs before this gate runs.
+#   - So in today's recipe the BLUEPRINT-ERROR branch is DEFENCE IN DEPTH, for
+#     a cook failure that gets swallowed.  The branches reachable live are
+#     PASS and SUMMARY-ABSENT, and the SUMMARY-ABSENT branch is coverage of its
+#     own: no other gate asks whether the cook log carries a summary at all.
+#   - A Blueprint that logs nothing at Error level is invisible to both gates.
+#   - TRIP-WIRE: if -IgnoreCookErrors is ever added to the recipe, or UAT's
+#     cook exit policy changes, this gate becomes the only guard: gate the
+#     summary's error count then (deferred 2026-09-28, TASK-1584).
+#
+# UE's THREE summary shapes (LaunchEngineLoop.cpp, all 'LogInit: Display:'):
+#   "Success - %d error(s), %d warning(s)"  - the commandlet returned 0, no errors
+#   "Failure - %d error(s), %d warning(s)"  - the commandlet returned 0, errors
+#   "With %d error(s), %d warning(s)"       - "Commandlet->Main return this
+#                                              error code: %d" printed first
+# SUMMARY_PATTERN matches the first two only.  The third reads SUMMARY-ABSENT,
+# which fails CLOSED (a STOP); by the reading above its non-zero exit makes
+# C2-UAT-LOG STOP first anyway.
@@ -879,5 +910,5 @@
-#   2. Otherwise, no cook summary line ('Success - N error(s), M warning(s)'
-#      or 'Failure - ...') => STOP SUMMARY-ABSENT.  A log that cannot say is
-#      not a log that said "clean": a cook that died before its summary and a
-#      gate reading the wrong file both land here (SC-114: a zero is a verdict,
-#      never a pass).
+#   2. Otherwise, no 'Success - ...' or 'Failure - ...' summary line
+#      => STOP SUMMARY-ABSENT.  A log that cannot say is not a log that said
+#      "clean": a cook that died before its summary, a summary in the third
+#      shape, and a gate reading the wrong file all land here (SC-114: a zero
+#      is a verdict, never a pass).
@@ -886 +917,2 @@
-# Blueprint compile errors, and that count covers every log category.
+# Blueprint compile errors, and that count covers every log category (see the
+# TRIP-WIRE above for when that must change).
@@ -938 +970 @@
-        $remedy   = ("The cook log carries no 'Success/Failure - N error(s), M warning(s)' summary line, so it cannot say whether a Blueprint failed to compile. A zero is not green: either the cook commandlet died before its summary or this gate is reading the wrong file. Read: {0}. A BS_ERROR Blueprint in the editor also wedges PIE (VER-12 cl. 7g). There is no skip flag." -f $logList)
+        $remedy   = ("The cook log carries no 'Success/Failure - N error(s), M warning(s)' summary line, so it cannot say whether a Blueprint failed to compile. A zero is not green. Possible causes: the cook commandlet died before its summary; it returned a non-zero code and printed UE's third summary shape, 'With N error(s), M warning(s)', which this gate does not read as a summary (look for 'Commandlet->Main return this error code' just above it); or this gate is reading the wrong file. Read: {0}. A BS_ERROR Blueprint in the editor also wedges PIE (VER-12 cl. 7g). There is no skip flag." -f $logList)
```

**"No verdict logic moves", checked as a property, not a count.**
1. Across the whole diff from (0), changed lines that are neither `#` comments nor the one `$remedy = ("The cook log carries no …` line: **0**.
2. Each logic line was present with non-zero hits, and hashed identical in (0) and final:
   - `$BP_ERROR_NEEDLE = 'LogBlueprint: Error'` (1 = 1);
   - `$SUMMARY_PATTERN = '(Success|Failure) - (\d+) error\(s\), (\d+) warning\(s\)'` (1 = 1);
   - `if ($bpHits.Count -gt 0) {` (1 = 1);
   - `} elseif ($sumHits.Count -eq 0) {` (1 = 1);
   - every `$verdict  =` (3 = 3) and `$reason   =` (3 = 3);
   - `Ok = ($verdict -eq 'PASS'); …` (1 = 1);
   - both `Get-LogMatches` calls (1 = 1 each).
3. **The remedy's string still has only the `{0}` placeholder.** The added text has no braces, so `-f` cannot throw. The NULL case's transcript in §5 prints the new remedy in full.
4. **The third shape still reads `SUMMARY-ABSENT`,** because the regex is unchanged. I did not add a fixture for it: that would be new bytes, and the fence forbids them.

**Citations, by text.** I read the Launcher install, read-only, and quoted from it by text rather than by line number:
- `LaunchEngineLoop.cpp`: the three format strings, `"Commandlet->Main return this error code: %d"`, and the engine comment `"Return an non-zero code if errors were logged and UseCommandletResultAsExitCode is false"` (the engine's own "an", kept verbatim). At today's install they sit at `:4194-4209` and `:4217-4224`, which agrees with QA.
- `CookCommand.Automation.cs`: `"Cook failed."` and `Params.IgnoreCookErrors`.

The premise bullet is labelled a READING, as the row requires.

**The wording fix I made after the first draft, declared.** My first draft said "A fourth case" a few lines above the pre-existing rider "OWED, NOT DONE: the fourth side is the next real /ship". A reader could take those for the same thing. I reworded **my** sentence to `A PAIR case (TASK-1587; not the owed "fourth side" below)` and left the rider untouched. Both harnesses were re-run on the final bytes (§5).

**Raw `git --no-optional-locks diff -U0 -- Tools/Packaging/ship.ps1` against HEAD (read-only).** It carries `TASK-1567`'s gated insertions with mine inside them. The hunk headers:

```
@@ -860,0 +861,126 @@ function Get-SuiteVerdict {
@@ -2354,0 +2481 @@ if ($DryRun) {
@@ -2435,0 +2563 @@ if ($DryRun) {
@@ -2563,0 +2692,7 @@ if ($DryRun) {
```

- `grep -c '^-[^-]'` on it gives **0** deleted content lines.
- So `qa/TASK-1568.md` §4 point 3's property still holds for the host: four hunks, every header `-N,0 +M[,K]`, no `-` content lines.
- The first hunk grew from `+94` to `+126` (+32 = 4 + 27 + 0 + 1 from my comment hunks; the remedy edit is a same-count swap).
- The full raw text is too long to repeat here; the host re-reads it at its instant. Its `TASK-1587` portion is exactly the `diff -U0` from (0) above.

## 4. Item (4): the root `.gitattributes` line and its `check-attr` proof

Raw output of `git --no-optional-locks diff -U0 -- ../.gitattributes` (read-only):

```
diff --git a/.gitattributes b/.gitattributes
index 59cc7f9..7debe51 100644
--- a/.gitattributes
+++ b/.gitattributes
@@ -9,0 +10 @@
+GitClaudeUnrealTest/Tools/Packaging/Fixtures/*.log -text
```

- **The nine LFS lines are byte-identical.** `head -n 9 .gitattributes | sha256sum` gives `dd00d4c8…caf1a`, which is the sha of the whole pre-edit file.
- **Line endings:** the new line ends CRLF like its neighbours (10 CR, 10 LF, 449 bytes).

**`check-attr`, read-only, declared.** Run from the project directory:

Before the edit, as the baseline:
```
$ git --no-optional-locks check-attr text -- Tools/Packaging/Fixtures/cook-bp-green.log Tools/Packaging/Fixtures/cook-bp-error-red.log Tools/Packaging/Fixtures/empty.log Tools/Packaging/Fixtures/b2_verdict_check.ps1
Tools/Packaging/Fixtures/cook-bp-green.log: text: unspecified
Tools/Packaging/Fixtures/cook-bp-error-red.log: text: unspecified
Tools/Packaging/Fixtures/empty.log: text: unspecified
Tools/Packaging/Fixtures/b2_verdict_check.ps1: text: unspecified
```

After the edit:
```
$ git --no-optional-locks check-attr text -- Tools/Packaging/Fixtures/cook-bp-green.log Tools/Packaging/Fixtures/cook-bp-error-red.log Tools/Packaging/Fixtures/empty.log Tools/Packaging/Fixtures/b2_verdict_check.ps1
Tools/Packaging/Fixtures/cook-bp-green.log: text: unset
Tools/Packaging/Fixtures/cook-bp-error-red.log: text: unset
Tools/Packaging/Fixtures/empty.log: text: unset
Tools/Packaging/Fixtures/b2_verdict_check.ps1: text: unspecified
exit=0
$ git --no-optional-locks check-attr -a -- Tools/Packaging/Fixtures/cook-bp-error-red.log Tools/Packaging/Fixtures/check_cook_bp_verdict.ps1
Tools/Packaging/Fixtures/cook-bp-error-red.log: text: unset
exit=0
```

- The three fixtures moved from `unspecified` to `unset`. The `.ps1` control stayed `unspecified`, as did the harness, which `-a` lists with no attributes.
- So the prefixed pattern resolves against the git root, and it matches only `*.log` in that folder.

## 5. Final harness transcripts (spec (5), on the final bytes)

```
$ powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\check_cook_bp_verdict.ps1 -RealLogDir C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540
--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---
  ship.ps1 parses clean: 0 parse errors
  defined Get-LogMatches from ship.ps1:755-780 (exactly 1 definition)
  defined Get-CookBlueprintVerdict from ship.ps1:934-985 (exactly 1 definition)
  [OK]       empty.log is 0 bytes (else the null side is not the null side)  --  Length=0
--- 2. THE TRIPLE: verdict AND reason, each fixture passed as the gate passes $clogs (stdout, stderr) ---
  [OK]       GREEN cook-bp-green.log     -> PASS / NO-BLUEPRINT-ERROR, summary quoted  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=0 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
  [OK]       RED   cook-bp-error-red.log -> STOP / BLUEPRINT-ERROR, 4 hits quoted, all BP_Basic_Movement  --  Verdict=STOP Reason=BLUEPRINT-ERROR Ok=False BpErrorCount=4 Quoted=4 SummaryCount=0 SummaryErrors=-1 LogsRead=2/2
  [OK]       NULL  empty.log             -> STOP / SUMMARY-ABSENT (a zero is a verdict, never a pass)  --  Verdict=STOP Reason=SUMMARY-ABSENT Ok=False BpErrorCount=0 Quoted=0 SummaryCount=0 SummaryErrors=-1 LogsRead=2/2
--- 2b. THE PAIR: the two real captures as ONE (stdout, stderr) pair - the VERDICT discriminates, not only the reason ---
  [OK]       PAIR  cook-bp-error-red.log + cook-bp-green.log -> STOP / BLUEPRINT-ERROR with the summary present (the needle is read first)  --  Verdict=STOP Reason=BLUEPRINT-ERROR Ok=False BpErrorCount=4 Quoted=4 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
--- 3. THE REAL RUN: C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540 ---
  REAL   [PASS] C2-COOK-BP-ERRORS  verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 'LogBlueprint: Error' lines (2 of 2 logs read); cook summary [cook.out.log:586] LogInit: Display: Success - 0 error(s), 3 warning(s) (its error count 0 is printed, not gated)
  [OK]       REAL cook log pair -> PASS / NO-BLUEPRINT-ERROR (the "cannot say" branch is NOT taken on a real green cook)  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=0 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
RESULT: ALL EXPECTATIONS MET on all three sides and the pair
EXIT=0
```

This excerpt keeps the `[OK]`/`[MISMATCH]`, RESULT and section lines. The full-verbosity run just before the wording fix printed every `Show` line and the new SUMMARY-ABSENT remedy in full, for example:

```
REQUIRED TO CLEAR: The cook log carries no 'Success/Failure - N error(s), M warning(s)' summary line, so it cannot say whether a Blueprint failed to compile. A zero is not green. Possible causes: the cook commandlet died before its summary; it returned a non-zero code and printed UE's third summary shape, 'With N error(s), M warning(s)', which this gate does not read as a summary (look for 'Commandlet->Main return this error code' just above it); or this gate is reading the wrong file. Read: …\empty.log + …\empty.log. A BS_ERROR Blueprint in the editor also wedges PIE (VER-12 cl. 7g). There is no skip flag.
```

`b2_verdict_check.ps1`, final bytes:

```
$ powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\b2_verdict_check.ps1
  [OK] x 16 (defect 1, defect 2, defect 2 sibling, zero/one/N/missing, GREEN, RED, NULL x4, THE TRAP, DUP, census)
     ok   ship.ps1:946   Get-LogMatches   LogBlueprint: Error
  11 -Simple site(s); 0 needle(s) carry a regex escape under -Simple
  [OK]       census: no regex-escaped needle under -Simple remains  --  flagged=0
RESULT: ALL EXPECTATIONS MET on both sides
EXIT=0
```

The census still resolves the needle, now at `:946` after the comment grew. It is not count-pinned, as `qa/TASK-1568.md` noted.

## 6. sha256 before and after, every file touched (plus the untouched fixtures)

| file | before | after | line endings after |
|---|---|---|---|
| `Tools/Packaging/ship.ps1` | `02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd` | `3b36633924c39f0f3675a98b1069be649e97fd8a200aca609a778a766cde48d7` | LF only (0 CR / 3364 LF), ASCII only |
| `Tools/Packaging/Fixtures/check_cook_bp_verdict.ps1` | `b48e4369ea8a7690f896ef2b0660a2a59f316f35f1826f6e522986de0b73c9c7` | `d49f47b2d230a7f6e6f0d25e613bcd4f71733513640dcf99c38d66d5f31faeab` | LF only (0 CR / 141 LF), ASCII only |
| `.claude/commands/ship.md` | `0d07b7152e1b85d0da8cc0f2643da49046185ea99ab48acb52c77808235da5b1` (blob `124fe75`) | `1cb5b1aaa87c5aa49795d02ed9dc1251c9cadc8899194e38bff67808da00c5eb` | LF only (0 CR / 332 LF) |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\.gitattributes` | `dd00d4c8fc4e433527ba154188b5bc2d44df090a961ab3e29c77ef5c63bcaf1a` (blob `59cc7f9`) | `a3a52ab3bb7198ccf25f9a86e2adf1778951a7adc56cad989357e1c39b3c984b` | CRLF (10 / 10) |
| `cook-bp-green.log` (NOT touched) | `b430b2f4…d444` | `b430b2f4013a7064d729be4acaad4d4920b420f4639089b6b7bbcc7cc19fd444` | unchanged |
| `cook-bp-error-red.log` (NOT touched) | `2b0fa374…1b40` | `2b0fa374e85549c7f808606623124e03f911cfe61e9d19d23d28774143281b40` | unchanged |
| `empty.log` (NOT touched) | `e3b0c442…b855` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | unchanged |
| `b2_verdict_check.ps1` (NOT touched, the control) | `61bc217a…ace7` | `61bc217ad635670ce21ca7c77b7fc5cd649ecf210cbd20dd632d8cf86f74ace7` | unchanged |

Transient mutant shas, which were never left on disk: arm (i) `a659fb81ce157d18f00563c07132bc5d2d848487335bac4ef8dc6685fa00d282`; arm (ii) `38194fff14c2a39e17e5ddea21e7473f4c7293540d0d01a6ff0e6d8fac7fc31a`; supplementary `05ef8a871d246090017950ba0b422ab420e4d6a46ce3ebf79c08ed8dc8920f3a`.

The board changed only in this row's `status:` line.

## 7. What QA should scrutinize

1. **Arm (ii)'s reading of "order swap".** I used `if (summary present) PASS elseif (needle) BLUEPRINT-ERROR else SUMMARY-ABSENT`. That is the swap QA ruling 2 described ("passes 3/3"), and the only one that makes the fourth case read PASS. The literal condition exchange is recorded as supplementary (§1). It is caught by RED's reason and never false-PASSes.
2. **The premise bullet's claim, "prints 'Failure - N error(s), M warning(s)'".** It holds only when the cook commandlet's `Main` returns 0. If `Main` returns non-zero, the third shape prints instead. Either way the exit is ≥ 1, so the conclusion (C2-UAT-LOG STOPs first) is unaffected. Both are labelled a reading.
3. **The pair's assertion includes `SummaryErrors -eq 0`.** That pins which summary was read (the green capture's). If the green fixture were ever swapped for a real `Failure - N` capture, that clause would need to change.
4. **The new `ship.md` row cites `PKG-§14` and `VER-§12` cl. 7g** in the `§` style of that file. `ship.ps1` keeps its own ASCII `PKG-14` / `VER-12` style.

## Not examined / limitations

- **A real cook was not run** (forbidden). The pair is two real captures combined, not a real cook's shape. A real cook with these errors would print `Failure - N` and, by the reading, never reach this gate.
- **The third summary shape is not exercised by any fixture.** Adding one would mean new bytes, which the fence forbids. "It reads SUMMARY-ABSENT" rests on the unchanged regex, by reading: `(Success|Failure) - ` cannot match `With `.
- **One engine branch sits outside the row.** When the commandlet runs without an error count, it prints only `LogInit: Display: Finished.` (`LaunchEngineLoop.cpp`, the `else` beside the count block). By QA's reading, the cook sets `ShowErrorCount`, so this is off-path. It would also read SUMMARY-ABSENT (fail closed). I did not add it to the comment, because the row asks for three shapes.
- **The engine chain in the comment was read, not run:** UAT's exit-code handling, the stdout filter and `-IgnoreCookErrors`. Of `qa/TASK-1568.md`'s chain I re-read only `LaunchEngineLoop.cpp:4140-4231` and grepped `CookCommand.Automation.cs` for `Cook failed.` and `IgnoreCookErrors`.
- **Clone behaviour after the `.gitattributes` line was not tested.** `check-attr` proves the attribute resolves. That a fresh checkout now keeps `cook-bp-error-red.log` at `2b0fa374…` (no CRLF conversion under `-text`) is git's documented behaviour, not a run: no clone or renormalise was done, per the fence.
- **The red fixture's index blob is not re-examined here.** The fixtures are untracked (`ls-files` lists only `b2_verdict_check.ps1` in that folder), so the first `git add` by the host will already see `-text`. No `--renormalize` is needed or was run.
- **The ⚙️ Dev & QA Slack thread was not read** before posting.
