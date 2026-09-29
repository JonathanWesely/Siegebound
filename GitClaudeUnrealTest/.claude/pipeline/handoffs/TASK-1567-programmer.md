# TASK-1567 — SHIP-COOK-BP-ERROR-GATE — programmer handoff

- **Agent:** gameplay-programmer, 2026-09-28. HEAD `9b82e8d` (blocker `TASK-1562` committed). No `/ship` live, no other writer of `ship.ps1` or `Tools/Packaging/Fixtures/` (both untouched since 2026-09-09 before this row).
- **Editor/engine:** not touched. No cook, build, package, `/ship`, Unreal process or MCP call. A `playtest-verifier` was driving PIE on PID 12112 the whole time. Everything below is script text plus offline runs against log FILES.
- **Status set:** `ready-for-qa` → gate `TASK-1568`.
- **Headline:** the gate is written and validated 3/3, and the mutation arm went red. **One (0) quote could NOT be produced: no real cook log on disk carries a `LogBlueprint` line of any verbosity.** That is not one of the row's STOP triggers (both triggers were checked and neither fired), so I went ahead. The finding is declared in §1b, in the function's comment and in the rider (§7). The manager/QA should rule whether it changes anything.

## 1. (0) The route, proven before writing

### 1a. The real cook log and its summary line (STOP trigger checked: did NOT fire)
The gate reads `$clogs = @($r3.Out, $r3.Err)` (`ship.ps1:2652` after the edit), i.e. `<RunLogDir>\cook.out.log` + `cook.err.log`. `RunLogDir = $StagingDir\.ship\<RunStamp>` (`:1531-1533`), and `$StagingDir` resolves to `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame` (the first existing candidate at `:1516-1521`).

On disk, `packagedZIPofGame\.ship\` holds 7 run dirs. Three have a cook pair:

| run | cook.out.log | cook.err.log | summary line | UAT line |
|---|---|---|---|---|
| `20260909-210513` | 3,831 B | 0 B | none | `BUILD FAILED` (:28) |
| `20260910-050628` | 102,293 B | 0 B | `:699` | `BUILD SUCCESSFUL` (:993) |
| **`20260910-063540`** (latest; the run `ship-state.json` names) | **88,190 B**, sha256 `42a60a95dfc5844d29561d5075e27f3aa10b1a47d6ac9e3b878013155e8ba413` | 0 B | **`:586`** | `BUILD SUCCESSFUL` (:880) |

**Quote (summary line):** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540\cook.out.log:586`
```
LogInit: Display: Success - 0 error(s), 3 warning(s)
```
This is the sighting `TASK-1445` recorded. The same line sits at `20260910-050628\cook.out.log:699`.

**How I proved it is a cook log, not just a file with that name:** line 4 is UAT's `Parsing command line: BuildCookRun ... -cook ...` with the eleven `-COOKDIR=` entries. Line 34 is `Commandlet log file is ...\Cook-2026.09.09-23.37.19.txt`, and line 35 is `Running: ...UnrealEditor-Cmd.exe "...GitClaudeUnrealTest.uproject" -run=Cook -Map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena ... -stdout -CrashForUAT -unattended -NoLogTimes`. `LogCook: Display: Done!` is at :577, `Warning/Error Summary (Unique only)` at :580, and `********** COOK COMMAND COMPLETED **********` at :599. `ship-state.json` records `cook: PASS` for this run (head `31edc23`, Shipping).

### 1b. The `LogBlueprint` line: NOT FOUND (declared, not a STOP trigger)
**0 `LogBlueprint` lines** (case-insensitive `blueprint` only matches the `-COOKDIR=...\Content\Blueprints` argument text) in all three cook pairs above. **Why:** the UAT-side stdout carries only Display-and-above lines. Measured on `20260910-063540\cook.out.log` (882 lines): 711 `Log*: Display:`, 3 `Log*: Warning:`, 0 `Log*: Error:`, **0 at Log verbosity**. A Log-verbosity `LogBlueprint` line can therefore never reach it. **What IS measured about the route:** Warning lines from other categories reach this log inline (`LogModelContextProtocol: Warning` :165, `LogNavigation: Warning` :276, :277) and again in its summary block (:582-584). An Error-verbosity `LogBlueprint` line would take that route. That the cooker emits one for a `BS_ERROR` Blueprint is the unmeasured part (rider §7).
**The cooker's own full log is gone:** `-abslog` pointed at `C:\Program Files\Epic Games\UE_5.8\Engine\Programs\AutomationTool\Saved\Cook-2026.09.09-23.37.19.txt`, and that dir now holds only `ResponseFiles\`. This matches `TASK-1441`'s finding.
**Row-text check:** the STOP sentence names exactly two triggers: "No real cook log on disk, or no summary line in it". Neither fired. The `LogBlueprint` quote is an evidence item I cannot supply, so it is declared here and not invented.

## 2. The diff (`git --no-optional-locks diff -U0 -- Tools/Packaging/ship.ps1`, read-only, declared)
`1 file changed, 103 insertions(+)`, 0 deletions, 4 hunks: the function (new `:861-954`), two skip notes, and the gate. No other gate's logic is touched.
```diff
@@ -860,0 +861,94 @@ function Get-SuiteVerdict {
+# (41-line header comment: provenance, the order-is-the-rule, the not-gated count, and the SC-101 rider; see ship.ps1:861-901)
+function Get-CookBlueprintVerdict {
+    param(
+        [Parameter(Mandatory=$true)][string[]] $Paths
+    )
+    $BP_ERROR_NEEDLE = 'LogBlueprint: Error'
+    $SUMMARY_PATTERN = '(Success|Failure) - (\d+) error\(s\), (\d+) warning\(s\)'
+    $QUOTE_MAX       = 5
+    $existing = @($Paths | Where-Object { Test-Path -LiteralPath $_ })
+    $logsRead = ('{0} of {1} logs read' -f $existing.Count, $Paths.Count)
+    $logList  = ($Paths -join ' + ')
+    # No @() around Get-LogMatches (see Get-SuiteVerdict): it already returns an
+    # array for 0 / 1 / N hits, and @() around it would nest that array.
+    $bpHits  = Get-LogMatches -Paths $Paths -Pattern $BP_ERROR_NEEDLE -Simple
+    $sumHits = Get-LogMatches -Paths $Paths -Pattern $SUMMARY_PATTERN
+    $summary       = 'absent'
+    $summaryErrors = -1
+    if ($sumHits.Count -gt 0) {
+        $last          = $sumHits[$sumHits.Count - 1]
+        $summary       = ('[{0}:{1}] {2}' -f (Split-Path -Leaf $last.Path), $last.LineNumber, $last.Line.Trim())
+        $summaryErrors = [int]$last.Matches[0].Groups[2].Value
+    }
+    $quoted = @()
+    if ($bpHits.Count -gt 0) {
+        $quoted = @($bpHits | Select-Object -First $QUOTE_MAX | ForEach-Object {
+            '[{0}:{1}] {2}' -f (Split-Path -Leaf $_.Path), $_.LineNumber, $_.Line.Trim()
+        })
+        $verdict  = 'STOP'
+        $reason   = 'BLUEPRINT-ERROR'
+        $evidence = ("verdict=STOP reason=BLUEPRINT-ERROR - {0} '{1}' line(s) ({2}); first {3}: {4}; cook summary: {5}" -f `
+                     $bpHits.Count, $BP_ERROR_NEEDLE, $logsRead, $quoted.Count, ($quoted -join ' | '), $summary)
+        $remedy   = ("A Blueprint failed to compile in this cook. Inside a COOKDIR it ships whether or not a map references it (PKG-14). Open each Blueprint quoted above, fix the compile error or delete the asset, then re-run the ship. The same BS_ERROR Blueprint also wedges PIE in the editor (VER-12 cl. 7g). Log: {0}. There is no skip flag." -f $logList)
+    } elseif ($sumHits.Count -eq 0) {
+        $verdict  = 'STOP'
+        $reason   = 'SUMMARY-ABSENT'
+        $evidence = ("verdict=STOP reason=SUMMARY-ABSENT - NOT MEASURED: 0 '{0}' lines and no cook summary line matching '{1}' ({2}); a log that cannot say is not a clean log" -f `
+                     $BP_ERROR_NEEDLE, $SUMMARY_PATTERN, $logsRead)
+        $remedy   = ("The cook log carries no 'Success/Failure - N error(s), M warning(s)' summary line, so it cannot say whether a Blueprint failed to compile. A zero is not green: either the cook commandlet died before its summary or this gate is reading the wrong file. Read: {0}. A BS_ERROR Blueprint in the editor also wedges PIE (VER-12 cl. 7g). There is no skip flag." -f $logList)
+    } else {
+        $verdict  = 'PASS'
+        $reason   = 'NO-BLUEPRINT-ERROR'
+        $evidence = ("verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 '{0}' lines ({1}); cook summary {2} (its error count {3} is printed, not gated)" -f `
+                     $BP_ERROR_NEEDLE, $logsRead, $summary, $summaryErrors)
+        $remedy   = ''
+    }
+    return @{
+        Ok = ($verdict -eq 'PASS'); Verdict = $verdict; Reason = $reason
+        BpErrorCount = $bpHits.Count; Quoted = $quoted
+        SummaryLine = $summary; SummaryErrors = $summaryErrors; SummaryCount = $sumHits.Count
+        LogsRead = $existing.Count; LogsGiven = $Paths.Count
+        Evidence = $evidence; Remedy = $remedy
+    }
+}
+
@@ -2354,0 +2449 @@ if ($DryRun) {
+    Note-Skipped 'C2-COOK-BP-ERRORS' 'dry run: not executed'
@@ -2435,0 +2531 @@ if ($DryRun) {
+    Note-Skipped 'C2-COOK-BP-ERRORS' 'reused'
@@ -2563,0 +2660,7 @@ if ($DryRun) {
+    # TASK-1567: the SAME $clogs, read for a Blueprint compile error in the cook
+    # (PKG-14).  Verdict, reason, quoted lines and remedy all come from
+    # Get-CookBlueprintVerdict - see there for what this gate does NOT measure.
+    $vbp = Get-CookBlueprintVerdict -Paths $clogs
+    Assert-Gate -Id 'C2-COOK-BP-ERRORS' -Ok $vbp.Ok `
+        -Evidence $vbp.Evidence `
+        -Remedy $vbp.Remedy | Out-Null
```
The header comment's 41 lines are elided above to keep this readable. They are in the file at `ship.ps1:861-901` and in the full `-U0` output.

**The gate as implemented. Name: `C2-COOK-BP-ERRORS`.** It sits at `:2660-2666`, right after `C2-UAT-LOG` (`:2657-2659`) on the live cook branch, before `$cookVerdict = 'PASS'`, and reads the same `$clogs`. Its conditions, in order:
1. **STOP `BLUEPRINT-ERROR`:** ≥1 line in either log contains the literal `LogBlueprint: Error` (`Get-LogMatches -Simple`, i.e. `Select-String -SimpleMatch`, case-insensitive). Quotes the first ≤5 as `[file:line] text`, plus the summary line or `absent`.
2. **STOP `SUMMARY-ABSENT`:** no BP error line AND no line matching the regex `(Success|Failure) - (\d+) error\(s\), (\d+) warning\(s\)` in either log. Missing files count as unread (`N of 2 logs read` is printed), so two missing logs land here too.
3. **PASS `NO-BLUEPRINT-ERROR`:** otherwise. Quotes the LAST summary line with `[file:line]` and prints its error count as "printed, not gated".

Evidence starts `verdict=<V> reason=<R> - ...`. The remedy names the log path(s) and `VER-12 cl. 7g`. In a live run, `Assert-Gate` throws `SHIP-STOP` on either STOP.
**Skip notes:** dry run → `'dry run: not executed'`, reuse → `'reused'`. Both are the exact reason texts `C2-UAT-LOG` uses on those two paths (its only two `Note-Skipped` sites).
**Notation:** `ship.ps1` is pure ASCII with no BOM (PS 5.1 reads it as ANSI), so `§` is written in the file's own convention: `VER-12 cl. 7g`, like `SC-114` and `PKG-14`. After the edit `file` still reports `ASCII text`, with 0 CR bytes (LF, as before).

## 3. The three fixtures (source, range, sha256; nothing inside the logs)
Slices are split on `\n` only, 1-based and inclusive, and keep every byte (CR, lone LF). Each was cut by a scratchpad Python slicer and then **independently verified with `cmp --ignore-initial=<off>:0 -n <len> <source> <fixture>`: byte-identical**.

| fixture | source file | lines | bytes | source sha256 | fixture sha256 | size | endings |
|---|---|---|---|---|---|---|---|
| `Tools/Packaging/Fixtures/cook-bp-green.log` | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540\cook.out.log` (882 lines) | **576-599** (`---- Finalisation: End ----` → `COOK COMMAND COMPLETED`; keeps the summary block :580-587 and the summary line :586 = fixture line 11) | [55077, 57383) | `42a60a95dfc5844d29561d5075e27f3aa10b1a47d6ac9e3b878013155e8ba413` | `b430b2f4013a7064d729be4acaad4d4920b420f4639089b6b7bbcc7cc19fd444` | 2,306 B / 24 lines | 24 CRLF |
| `Tools/Packaging/Fixtures/cook-bp-error-red.log` | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\Logs\GitClaudeUnrealTest-backup-2026.09.28-06.41.12.log` (5,405 lines) | **4732-4744** (`LogStreaming FlushAsyncLoading` → `LogLinker: Warning VerifyImport /Script/XRBase` → 4 × `LogBlueprint: Error ... BP_Basic_Movement.uasset: [Compiler] ...` (4734, 4735, 4737, 4738) with their 2 `Make sure ...` continuation lines → 5 × `LogBlueprint: Warning`) | [665794, 668998) | `8bbd01de0e3eeddc72877a1e7f6bef317e4227d11bf64cef65cfb0ebe3a58d51` | `2b0fa374e85549c7f808606623124e03f911cfe61e9d19d23d28774143281b40` | 3,204 B / 13 lines | 11 CRLF + 2 lone LF (the engine's embedded message newline, kept as found) |
| `Tools/Packaging/Fixtures/empty.log` | none (0 bytes, the file `TL-§6` names) | — | — | — | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` | 0 B | — |

**Red source identity:** it is the only file in `Saved/Logs/` holding `LogBlueprint: Error`. Its line 1 is `Log file open, 09/26/26 23:05:31`, which equals PID 3108's CreationDate `2026-09-26 23:05:31` (`handoffs/TASK-1557-art.md:14`, `TASK-1439-programmer.md:12`). Line 1549 names the instance `JONATHANWESELY-3108`. The errors are at log clock `2026.09.28-00.12.20:782`, the TASK-1439 load of the known-broken control.
**All three sides were captured from real logs. None is NOT RUN.**

## 4. Harness and transcript (`SC-§71a` declared runs)
`Tools/Packaging/Fixtures/check_cook_bp_verdict.ps1` (new, 7,097 B, ASCII, LF). How it works:
- It lifts `Get-LogMatches` and `Get-CookBlueprintVerdict` out of `ship.ps1` by AST and dot-sources `Extent.Text`, so nothing is retyped.
- It **throws unless there is exactly 1 definition of each** (NIT 8).
- It runs under `Set-StrictMode -Version Latest` with `$ErrorActionPreference = 'Stop'`.
- It passes each fixture as the gate passes `$clogs`, as a pair of (fixture, `empty.log`), because every real `cook.err.log` on disk is 0 bytes. The null side is (`empty.log`, `empty.log`).
- It asserts **verdict AND reason** for each side, plus exact counts.
- Optional `-RealLogDir` runs the real pair of a ship dir, expecting PASS.
- **It writes nothing.** There is no temp file because the null side is the tracked `empty.log`. NIT 9's `try/finally` therefore has nothing to guard: declared, not skipped.

Run 1: `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\check_cook_bp_verdict.ps1 -RealLogDir C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540` (PS 5.1.26100.9444), transcript verbatim with the long RED quote/remedy lines shortened as marked:
```
--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---
  ship.ps1 parses clean: 0 parse errors
  defined Get-LogMatches from ship.ps1:755-780 (exactly 1 definition)
  defined Get-CookBlueprintVerdict from ship.ps1:902-953 (exactly 1 definition)
  fixture   2,306 B  ...\Fixtures\cook-bp-green.log
  fixture   3,204 B  ...\Fixtures\cook-bp-error-red.log
  fixture       0 B  ...\Fixtures\empty.log
  [OK]       empty.log is 0 bytes (else the null side is not the null side)  --  Length=0
--- 2. THE TRIPLE: verdict AND reason, each fixture passed as the gate passes $clogs (stdout, stderr) ---
  GREEN  [PASS] C2-COOK-BP-ERRORS  verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 'LogBlueprint: Error' lines (2 of 2 logs read); cook summary [cook-bp-green.log:11] LogInit: Display: Success - 0 error(s), 3 warning(s) (its error count 0 is printed, not gated)
  [OK]       GREEN cook-bp-green.log     -> PASS / NO-BLUEPRINT-ERROR, summary quoted  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=0 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
  RED    [STOP] C2-COOK-BP-ERRORS  verdict=STOP reason=BLUEPRINT-ERROR - 4 'LogBlueprint: Error' line(s) (2 of 2 logs read); first 4: [cook-bp-error-red.log:3] [2026.09.28-00.12.20:782][994]LogBlueprint: Error: [AssetLog] ...\BP_Basic_Movement.uasset: [Compiler] In use pin  Return Value  no longer exists on node  Is Head Mounted Display Enabled . ... | [cook-bp-error-red.log:4] ... Could not find a function named "IsHeadMountedDisplayEnabled" in 'BP_Basic_Movement'. | [cook-bp-error-red.log:6] ... | [cook-bp-error-red.log:7] ...; cook summary: absent
                 REQUIRED TO CLEAR: A Blueprint failed to compile in this cook. Inside a COOKDIR it ships whether or not a map references it (PKG-14). ... The same BS_ERROR Blueprint also wedges PIE in the editor (VER-12 cl. 7g). Log: ...\cook-bp-error-red.log + ...\empty.log. There is no skip flag.
  [OK]       RED   cook-bp-error-red.log -> STOP / BLUEPRINT-ERROR, 4 hits quoted, all BP_Basic_Movement  --  Verdict=STOP Reason=BLUEPRINT-ERROR Ok=False BpErrorCount=4 Quoted=4 SummaryCount=0 SummaryErrors=-1 LogsRead=2/2
  NULL   [STOP] C2-COOK-BP-ERRORS  verdict=STOP reason=SUMMARY-ABSENT - NOT MEASURED: 0 'LogBlueprint: Error' lines and no cook summary line matching '(Success|Failure) - (\d+) error\(s\), (\d+) warning\(s\)' (2 of 2 logs read); a log that cannot say is not a clean log
                 REQUIRED TO CLEAR: The cook log carries no 'Success/Failure - N error(s), M warning(s)' summary line, ... Read: ...\empty.log + ...\empty.log. A BS_ERROR Blueprint in the editor also wedges PIE (VER-12 cl. 7g). There is no skip flag.
  [OK]       NULL  empty.log             -> STOP / SUMMARY-ABSENT (a zero is a verdict, never a pass)  --  Verdict=STOP Reason=SUMMARY-ABSENT Ok=False BpErrorCount=0 Quoted=0 SummaryCount=0 SummaryErrors=-1 LogsRead=2/2
--- 3. THE REAL RUN: C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260910-063540 ---
  88,190 B     ...\20260910-063540\cook.out.log
  0 B          ...\20260910-063540\cook.err.log
  REAL   [PASS] C2-COOK-BP-ERRORS  verdict=PASS reason=NO-BLUEPRINT-ERROR - 0 'LogBlueprint: Error' lines (2 of 2 logs read); cook summary [cook.out.log:586] LogInit: Display: Success - 0 error(s), 3 warning(s) (its error count 0 is printed, not gated)
  [OK]       REAL cook log pair -> PASS / NO-BLUEPRINT-ERROR (the "cannot say" branch is NOT taken on a real green cook)  --  Verdict=PASS Reason=NO-BLUEPRINT-ERROR Ok=True BpErrorCount=0 Quoted=0 SummaryCount=1 SummaryErrors=0 LogsRead=2/2
RESULT: ALL EXPECTATIONS MET on all three sides
EXIT=0
```
**3 of 3 verdicts and reasons as expected**, plus the real green cook (`20260910-063540`) → PASS. That is the row's worry ("a gate whose cannot-say branch was never run against a real green log may stop every ship") run on the FULL real log, not just the truncation. A second real green, `-RealLogDir ...\20260910-050628`, also gave `REAL [PASS] ... cook summary [cook.out.log:699] LogInit: Display: Success - 0 error(s), 3 warning(s)` and `[OK]`, exit 0.
The red quotes sit at fixture lines 3/4/6/7, which are source lines 4734/4735/4737/4738. Select-String numbering agrees with the `\n` split.

## 5. The mutation arm: red → restored → green
1. In the working copy, `ship.ps1:906` became `$BP_ERROR_NEEDLE = 'LogBlueprint: Errorx'` (Edit tool, one line). Mutant sha256 `a659fb81ce157d18f00563c07132bc5d2d848487335bac4ef8dc6685fa00d282`.
2. Re-run (no `-RealLogDir`): **`[MISMATCH] RED cook-bp-error-red.log -> STOP / BLUEPRINT-ERROR ... -- Verdict=STOP Reason=SUMMARY-ABSENT Ok=False BpErrorCount=0 Quoted=0 SummaryCount=0`**, `RESULT: 1 MISMATCH(ES) - the gate is NOT validated`, **EXIT=2**. GREEN `[OK]` and NULL `[OK]`, as they should be.
   ⚠ **The verdict stayed `STOP` under the mutant. Only the REASON assertion caught it.** A verdict-only harness would have passed a gate whose needle matches nothing, because the red fixture has no summary line and falls through to SUMMARY-ABSENT. That is why the row's "verdict AND reason" is load-bearing, measured here.
3. Restored `'LogBlueprint: Error'` (Edit tool). Re-hash: `02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd`, **equal to the after-sha** (`True`). Re-run: GREEN/RED/NULL `[OK]`, `RESULT: ALL EXPECTATIONS MET`, EXIT=0.

EVIDENCE NOT RETAINED — CLAIMS ARE DECLARED: the mutant working copy itself (it was restored in place, and its sha is recorded above). The arm is a one-token substitution anyone can reproduce.

**Regression (declared extra run):** `Tools/Packaging/Fixtures/b2_verdict_check.ps1` (unmodified) lifts `Get-LogMatches` from this same file. Its `-Simple` census now shows **11** sites, including `ok ship.ps1:914 Get-LogMatches LogBlueprint: Error`, with `0 needle(s) carry a regex escape`. All its checks are `[OK]`, `RESULT: ALL EXPECTATIONS MET on both sides`, EXIT=0. It writes one temp file under `%TEMP%` and removes it (its own `:79-80` / `:202`).

## 6. sha256 before/after, every file touched

| file | before | after |
|---|---|---|
| `Tools/Packaging/ship.ps1` | `f3902fae8b1f302e30dafe8d6d85f7b1ffe28bb188921e371b46ff0ff45d1bf9` | `02c261f6503dd9c975fd9c4e2aef581084ad1508559155fdbfe4eb6c3523debd` |
| `Tools/Packaging/Fixtures/check_cook_bp_verdict.ps1` | absent (new) | `b48e4369ea8a7690f896ef2b0660a2a59f316f35f1826f6e522986de0b73c9c7` |
| `Tools/Packaging/Fixtures/cook-bp-green.log` | absent (new) | `b430b2f4013a7064d729be4acaad4d4920b420f4639089b6b7bbcc7cc19fd444` |
| `Tools/Packaging/Fixtures/cook-bp-error-red.log` | absent (new) | `2b0fa374e85549c7f808606623124e03f911cfe61e9d19d23d28774143281b40` |
| `Tools/Packaging/Fixtures/empty.log` | absent (new) | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| `.claude/pipeline/handoffs/TASK-1567-programmer.md` | absent (new) | (this file) |
| `.claude/pipeline/TASKBOARD.md` | this row's `status:` line only | — |

Untouched, confirmed: `b2_verdict_check.ps1` (`61bc217a...`), `Content/**`, `Source/**`, `CONVENTIONS.md`, `CLAUDE.md`. No git mutation (read-only `diff`/`status`/`log`/`config`/`check-ignore`/`ls-files` only).

## 7. (5) Honesty rider (`SC-§101`), also in the comment above the function (`ship.ps1:888-900`)
- **The red fixture is an EDITOR log.** That the cooker writes the same `LogBlueprint: Error` line for a `BS_ERROR` Blueprint inside a COOKDIR is **NOT measured**. It cannot be measured without a broken asset in a cook folder, and no broken asset is ever committed to get a fixture (`VER-§12` cl. 7g).
- **Stronger than the row assumed:** no real cook log on disk carries a `LogBlueprint` line of any verbosity (§1b). The UAT stdout holds Display/Warning/Error only. So the category's arrival in this log is also unmeasured. Only the Warning/Error route for other categories is measured.
- **OWED, NOT DONE:** the fourth side is the next real `/ship`. Its `C2-COOK-BP-ERRORS` line must read `PASS` with the summary line quoted. Until then, the green side is proven on two real historical cooks (§4), not on a cook run with this gate in it.

## 8. A stricter needle? (the row asks me to say, and it is not my call)
**Yes, in one direction.** Of everything this gate reads, the summary's error count is the only instrument whose route into this log is proven: the line is there, and UE counts every Error-verbosity line of every category into it (`LogBlueprint` included, if the cooker emits one). Gating `N error(s) > 0` would catch the BP compile error class through a proven route, even if the cooker's BP error wording differs from the editor's. The cost: it also stops on non-Blueprint cook errors, which widens his option text. It is printed today, not gated. **The manager rules.** A second, narrower option is to also match `[Compiler]` (the editor wording's tag). I did not add it, because it is equally unmeasured on a cook log.

## 9. What QA should scrutinize
1. **Order and fall-through:** BLUEPRINT-ERROR is checked before SUMMARY-ABSENT, so a BP-error log with no summary reads BLUEPRINT-ERROR (the red fixture is exactly that).
2. **`Get-LogMatches` array contract:** there is no `@()` around it, as in `Get-SuiteVerdict`. `$sumHits[$sumHits.Count - 1].Matches[0].Groups[2]` is only reached when Count > 0, and only on the regex (non-`-Simple`) call, where `Matches` is populated.
3. **StrictMode:** `$quoted`, `$summary` and `$summaryErrors` are initialised before every branch that reads them. `$verdict`, `$reason`, `$evidence` and `$remedy` are assigned in all three branches.
4. **Needle vs matcher:** the needle is literal under `-Simple`, and the summary pattern is a regex without `-Simple`, so the pattern and matcher agree on both calls. The b2 census rules the new `-Simple` site `ok`.
5. **Case:** `-SimpleMatch` is case-insensitive, so `logblueprint: error` also matches (more inclusive, not less).
6. **The summary regex is unanchored.** Each real cook log has exactly 1 match, with 0 in other UAT phases. If a later UAT phase ever printed the same shape, the LAST match is quoted.

## Not examined / limitations
- **The cooker's own `LogBlueprint: Error` wording and verbosity for a `BS_ERROR` Blueprint:** NOT measured (§7). There is no fixture of a cook with a broken asset, by law.
- **The live path of the gate** (inside the cook branch): never executed. No `/ship` was run, by fence. The harness exercises the function, and the 7-line gate binding was checked by eye: it reads `$clogs`, and `-Ok`/`-Evidence`/`-Remedy` come straight from the verdict.
- **`-DryRun` output with the new `PLAN` line:** not run (a dry run is a `/ship` invocation, fenced). The two `Note-Skipped` lines are copies of the adjacent `C2-UAT-LOG` calls with the new Id.
- **`.claude/commands/ship.md` gate table** (`:102`, `C1-COOK / C2-UAT-LOG`) does not list `C2-COOK-BP-ERRORS`. It is outside my WRITES fence, so it is **flagged for the manager, not edited**.
- **Line endings under git:** system `core.autocrlf=true` (`C:/Program Files/Git/etc/gitconfig`) and no `.gitattributes`. On `git add`, both CRLF fixtures are stored LF in the index. A fresh checkout of the green gives CRLF back (sha unchanged). The red's 2 lone LFs come back as CRLF (+2 B), so **the red fixture's sha on a fresh checkout will not equal `2b0fa374...`**, and git warns `LF will be replaced by CRLF` (default `safecrlf=warn`). Verdicts are ending-agnostic: Select-String splits on CRLF and LF alike, and the red hits stay on lines 3/4/6/7. A `Tools/Packaging/Fixtures/*.log -text` attribute would pin the bytes, but that is a repo-config call for the manager/host, not this row.
- **First fixture write was wrong, and I caught it:** my first cut used Git-Bash `sed`, which stripped CRs. The byte-range check reported `DIFFER` for both, and I overwrote them with the byte-exact Python slices verified by `cmp` (§3). The final files are the verified ones. The slicer is `scratchpad/slice.py`, outside the repo and not retained. Its logic is "split on `\n`, keep every byte", and `cmp` is the independent proof.
- **The historical `20260909-210513` (BUILD FAILED) pair** was not run through the verdict (C2-UAT-LOG stops first on it). It has no summary line, so it would read SUMMARY-ABSENT.
- **Multiple summary lines:** only the last is quoted and its count printed. No real log has more than one.
