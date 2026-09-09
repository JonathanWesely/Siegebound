# TASK-1193 (B2 fix) — programmer handoff

**Status: ready-for-qa** (board row not edited — the manager is boarding it; this file is the link target).
Written by gameplay-programmer, 2026-09-09. Rebound from `qa/TASK-1193-cook.md` (routing rule 6).

## 0. One paragraph

`ship.ps1`'s B2-SUITE gate crashed in its own code on a green suite (`UNEXPECTED-ERROR`, exit 1) on every live `/ship` since the file was born (`2cc8213`). Two defects, both fixed, plus one more that the validation harness caught in **my own first revision** before it shipped: (1) the needles `'Result=\{Success\}'` / `'Result=\{Fail\}'` were regex-escaped but matched with `-Simple` (`-SimpleMatch`), so they looked for a literal backslash and matched 0 of 555 lines; (2) `Get-LogMatches` returned `$res` bare, which the pipeline unrolls — zero hits reach the caller as `$null` and ONE hit as a bare `MatchInfo`, and `.Count` THROWS on both under the script's `Set-StrictMode -Version Latest`. Fix: the organ now returns `,$res` (an array for 0/1/N hits, curing every caller at once), the B2 parse is lifted into a pure `Get-SuiteVerdict` with literal needles under `-Simple`, and a zero-hit read is now a **named STOP** (`NOT MEASURED - suite produced no results`) ruled independently of the baseline clause, never a pass and never an exception. Validated by `Tools/SuiteRunnerFixtures/b2_verdict_check.ps1`, which lifts the functions out of `ship.ps1` by AST (never retyped) and runs them under the same StrictMode: **real green run 555/0 → PASS · red fixture 18/2 → STOP with names · null (4 variants, and at baseline 0) → STOP**. A four-mutant set proves the harness goes red when the fix is broken.

## 1. The defects, measured (PS 5.1.26100.9168, `Set-StrictMode -Version Latest`)

| # | Defect | Site (pre-fix) | Measurement |
|---|---|---|---|
| 1 | Regex-escaped needle under `-SimpleMatch` | `:2236`, `:2237` | On `green-suite.log`: `-SimpleMatch 'Result=\{Success\}'` = **0**; `-SimpleMatch 'Result={Success}'` = 20; regex `'Result=\{Success\}'` = 20. Build-master measured the same on the real log (0 vs 555). |
| 2 | `.Count` on a pipeline-unrolled empty/single result throws | `Get-LogMatches :770` and callers `:2235`, `:2236`, `:2238`, `:2666` | Old organ, zero hits: `.Count` → `The property 'Count' cannot be found on this object`. **Old organ, ONE hit: same throw** (a bare `MatchInfo` has no `.Count` under StrictMode) — so `$perf` (`:2235`) survived live only because BOTH `automation.log` and `suite.out.log` carried the "tests performed" line (2 hits → array). |
| 2b | A zero ruled as a pass (the trap the task named) | `:2238` | With the crash gone, `Success == 0 && Fail == 0` only stopped via `$successCount -ge $SUITE_BASELINE`; lowering the baseline to 0 would have made a never-ran suite green. Now `$noResults` is ruled explicitly and first. |
| 3 (mine, rev 1) | `@( Get-LogMatches ... )` NESTS the comma-returned array | my rev-1 `Get-SuiteVerdict` | Measured: `@(g0).Count = 1, elem0type=Object[]` for BOTH an empty and a 2-element comma-returned array ⇒ zero Fail hits would read `Fail=1` and `$hit.Line` throws. Caught by the harness on the first run of the function; the `@()` wraps are removed and the reason is recorded in the file. |
| 4 (found, fixed) | Counts summed across `$slogs` | `:2236` | `automation.log` (the `-abslog`) and `suite.out.log` (redirected stdout) carry the SAME lines ⇒ the old expression would have reported **1110** Success for a 555 suite had it ever run. Counts are now the MAX per log; evidence states `max per log` and `k of n logs read`. |

## 2. What changed — `Tools/Packaging/ship.ps1` (98→ +103 / −8 lines; file stays LF, no BOM)

Authoritative diff: `git diff -- GitClaudeUnrealTest/Tools/Packaging/ship.ps1` (git root is one level up — `SC-§102`). Hunks: `@@ -767,7 +767,95 @@` and `@@ -2231,14 +2319,21 @@`. Final text of every changed region:

### 2a. `Get-LogMatches` tail (`:770-780`) — the organ

Removed: `    return $res`. Added:
```powershell
    # TASK-1193: ALWAYS hand back an ARRAY.  A bare 'return $res' unrolls through
    # the pipeline: zero hits reach the caller as $null and ONE hit as a bare
    # MatchInfo, and under Set-StrictMode -Version Latest (:214) '.Count' THROWS
    # on both (measured, PS 5.1.26100.9168).  That is what turned a GREEN suite
    # into 'SHIP RESULT: STOP at UNEXPECTED-ERROR' at B2 - and 'Result={Fail}'
    # matching NOTHING is the NORMAL path of a green run.  The unary comma wraps
    # the array so it survives the pipeline whole: 0 / 1 / N hits => Object[] of
    # Count 0 / 1 / N, every time, for every caller ($perf in B2 and $deckHits in
    # C3 carried the same latent throw for a log with exactly one matching line).
    return ,$res
```

### 2b. New `Get-SuiteVerdict` (`:782-859`, inserted between `Get-LogMatches` and `Get-Sha256OfString`)

```powershell
function Get-SuiteVerdict {
    param(
        [Parameter(Mandatory=$true)][string[]] $Paths,
        [Parameter(Mandatory=$true)][int]      $Baseline
    )
    $SUCCESS_NEEDLE = 'Result={Success}'
    $FAIL_NEEDLE    = 'Result={Fail}'
    $existing  = @($Paths | Where-Object { Test-Path -LiteralPath $_ })
    $success   = 0
    $fail      = 0
    $performed = 0
    $failNames = @()
    # No @() around Get-LogMatches, deliberately: it already returns an array
    # (the ',' in its return), and @() around a command that emits ONE array
    # object NESTS it - zero hits would arrive as a 1-element array holding an
    # empty array (Count 1, no .Line), i.e. a green suite would read Fail=1.
    # Measured by b2_verdict_check.ps1 on this function's first run (rev 1).
    foreach ($p in $existing) {
        $s = Get-LogMatches -Paths @($p) -Pattern $SUCCESS_NEEDLE -Simple
        $f = Get-LogMatches -Paths @($p) -Pattern $FAIL_NEEDLE    -Simple
        if ($s.Count -gt $success) { $success = $s.Count }
        if ($f.Count -gt $fail)    { $fail    = $f.Count }
        foreach ($hit in $f) {
            if ($hit.Line -match 'Name=\{([^}]*)\}') { $failNames += $Matches[1] }
        }
    }
    $failNames = @($failNames | Select-Object -Unique)
    $perf = Get-LogMatches -Paths $Paths -Pattern '(\d+)\s+tests?\s+performed'
    if ($perf.Count -gt 0) { $performed = [int]$perf[$perf.Count - 1].Matches[0].Groups[1].Value }

    $noResults = (($success -eq 0) -and ($fail -eq 0))
    $ok = (-not $noResults) -and ($fail -eq 0) -and ($performed -ge $Baseline) -and ($success -ge $Baseline)
    $logsRead = ('{0} of {1} logs read' -f $existing.Count, $Paths.Count)
    $remedy = ''
    if ($noResults) {
        $evidence = ('NOT MEASURED - suite produced no results: 0 {0} and 0 {1} lines ({2} performed; {3})' -f `
                     $SUCCESS_NEEDLE, $FAIL_NEEDLE, $performed, $logsRead)
        $remedy   = 'A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.'
    } else {
        $evidence = ('{0} performed / {1} Success / {2} Fail (baseline {3}; {4}; max per log)' -f `
                     $performed, $success, $fail, $Baseline, $logsRead)
        if ($failNames.Count -gt 0) { $evidence += (' - failing: ' + (@($failNames | Select-Object -First 8) -join ', ')) }
        if ($fail -gt 0) {
            $remedy = 'Any failing test STOPS the ship. Fix the test or the code - there is no skip flag.'
        } elseif (-not $ok) {
            $remedy = ('The suite ran below the {0}-test baseline. A shrunken suite is a STOP, not a pass; find the tests that went missing.' -f $Baseline)
        }
    }
    return @{
        Ok = $ok; NoResults = $noResults
        Performed = $performed; Success = $success; Fail = $fail; FailNames = $failNames
        LogsRead = $existing.Count; LogsGiven = $Paths.Count
        Evidence = $evidence; Remedy = $remedy
    }
}
```
(The 23-line law comment above the function in the file explains the two rules and the max-per-log choice.)

### 2c. B2 gate (`:2321-2336`) — consumes the verdict; nothing retyped

Removed (the 8 original lines `:2234-2241`): `$perf = ...`, `if ($perf.Count ...)`, `$successCount = (... 'Result=\{Success\}' -Simple).Count`, `$failLines = ... 'Result=\{Fail\}' -Simple`, `$ok2 = ($failLines.Count -eq 0) -and ...`, and the old `-Evidence`/`-Remedy` arguments. Added:
```powershell
    $slogs = @($suiteLog, $r2.Out, $r2.Err)            # unchanged line
    # TASK-1193: the parse lives in Get-SuiteVerdict (beside Get-LogMatches) so
    # it can be validated against a real log without a cook (SHIP-9c).  Literal
    # needles under -Simple; a zero is a NAMED STOP, never a pass and never an
    # UNEXPECTED-ERROR; counts are max-per-log because automation.log and
    # suite.out.log carry the same lines.  Nothing below retypes the verdict.
    $v2 = Get-SuiteVerdict -Paths $slogs -Baseline $SUITE_BASELINE
    $suiteTotal = $v2.Performed
    $ok2 = $v2.Ok -and (-not $r2.TimedOut)
    $remedy2 = $v2.Remedy
    if ($r2.TimedOut) {
        $remedy2 = ('The suite did not finish inside {0} minutes and was killed, so these counts are PARTIAL. ' -f $SUITE_TIMEOUT_MIN) + $v2.Remedy
    }
    Assert-Gate -Id 'B2-SUITE' -Ok $ok2 `
        -Evidence ("{0}; timedOut={1}" -f $v2.Evidence, $r2.TimedOut) `
        -Remedy $remedy2 | Out-Null
```
Semantics preserved: `$suiteTotal` still feeds the state file (`suiteTotal`) and the commit message; `$SUITE_BASELINE` (143) and `$SUITE_TIMEOUT_MIN` untouched; the `-DryRun` and `$reuse` branches untouched; `Assert-Gate` untouched.

### 2d. Nothing else in the file was changed. (`git diff --stat`: 1 file, the two hunks above.)

## 3. Sibling census — every `-Simple`/`-SimpleMatch` use and every `.Count` on a `Get-LogMatches`/`Select-String` result

Mechanically enumerated by the harness (section 6, AST walk) and read by hand. Line numbers are post-fix.

| Site | Needle / usage | Ruling |
|---|---|---|
| `:746` `Test-LogPattern` body, `:765` `Get-LogMatches` body — `Select-String -SimpleMatch` | needle is the `$Pattern` parameter | **correct by construction** — these are the helpers; the species lives at callers |
| `:823`, `:824` `Get-LogMatches ... -Simple` (new) | `'Result={Success}'`, `'Result={Fail}'` | **fixed** — literal needles under `-Simple` (defect 1) |
| `:2304` `Test-LogPattern -Simple` | `'0x800711C7'` | correct — literal, no metachar, boolean use |
| `:2559`, `:2560` `Test-LogPattern -Simple` | `'BUILD SUCCESSFUL'`, `'BUILD FAILED'` | correct — literal, boolean use |
| `:2653` `Test-LogPattern -Simple` | `$BOOT_MARK_DECK` = `'draw pile from'` | correct — literal, boolean use |
| `:2766` `Test-LogPattern -Simple` | `$BOOT_MARK_HERO` = `'Castle-relative hero start for'` | correct — literal, boolean use |
| `:2772` `foreach ($hit in (Get-LogMatches -Pattern $pat -Simple))` | `$pat` ∈ `$BOOT_FAIL_PATTERNS` (`:376`) = `'not found'`, `'unavailable'`, `'continuing without a HUD'` | correct — all literal (harness marks it MANUAL because `$pat` is a loop variable; resolved by reading `:376`); consumed by `foreach`, which is null-safe; no `.Count` on it |
| `:833` `$perf.Count` (moved into `Get-SuiteVerdict`) | regex `'(\d+)\s+tests?\s+performed'`, no `-Simple` | pattern correct; **species-2 sibling fixed by the organ** — it was latent live (single-log case throws), now array for 0/1/N |
| `:2758-2762` `$deckHits = Get-LogMatches -Pattern $BOOT_RE_DECK` then `$deckHits.Count`, `$deckHits[$deckHits.Count - 1]` | regex, no `-Simple` | pattern correct; **species-2 sibling fixed by the organ** (a boot log with exactly ONE "built a N-card draw pile" line — the normal case — would have thrown) — no call-site change needed or made |
| `:2780` `$bad.Count` | `$bad = @()` local + `+=` | correct — a locally assigned array is never pipeline-unrolled |
| `:1617` `[bool](Select-String ... -List)` | regex on `DefaultEngine.ini` | correct — bool cast is null-safe |
| `:1761-1766` `$gdm = Select-String ... -List` then `if ($gdm) { $gdm.Matches[0] }` | regex | correct — guarded before member access |
| `:2225-2226` `Test-LogPattern` | regex `'Result:\s*Succeeded'` / `'Result:\s*Failed'`, no `-Simple` | correct — regex with the regex matcher |

Harness census line: **`10 -Simple site(s); 0 needle(s) carry a regex escape under -Simple`**.

## 4. Validation — `SHIP-§9c`, both sides, on real artefacts (verbatim harness output)

Command: `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\SuiteRunnerFixtures\b2_verdict_check.ps1 -RealLogDir C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755 -RealExpect 555`

```
b2_verdict_check.ps1  PS 5.1.26100.9168  StrictMode=Latest  ship.ps1=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1

--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---
  ship.ps1 parses clean: 0 parse errors
  defined Get-LogMatches from ship.ps1:755-780
  defined Get-SuiteVerdict from ship.ps1:805-859
  ship.ps1 SUITE_BASELINE = 143

--- 2. POSITIVE CONTROL: the PRE-FIX organ, verbatim from ship.ps1@2cc8213 :755-771 ---
  green-suite.log: -SimpleMatch 'Result=\{Success\}' = 0   -SimpleMatch 'Result={Success}' = 20   regex 'Result=\{Success\}' = 20
  [OK]       defect 1: escaped needle under -SimpleMatch matches NOTHING; literal == regex  --  escaped=0 literal=20 regex=20
  [OK]       defect 2: OLD organ, ZERO hits, .Count THROWS (the live B2 crash)  --  threw=True : The property 'Count' cannot be found on this object. Verify that the property exists.
  [OK]       defect 2 sibling: OLD organ, ONE hit, .Count THROWS ($perf / $deckHits species)  --  threw=True : The property 'Count' cannot be found on this object. Verify that the property exists.

--- 3. REPAIRED ORGAN: Get-LogMatches from the file returns an ARRAY for 0 / 1 / N hits and a missing path ---
  [OK]       zero hits    -> Object[] Count 0  --  type=Object[] Count=0
  [OK]       one hit      -> Object[] Count 1, indexable  --  type=Object[] Count=1 line=52
  [OK]       N hits       -> Object[] Count N  --  type=Object[] Count=2 lines=14,16
  [OK]       missing path -> Object[] Count 0 (no throw)  --  type=Object[] Count=0

--- 4. VERDICTS from Get-SuiteVerdict (fixture baseline 20) ---
  GREEN   [PASS] B2-SUITE  20 performed / 20 Success / 0 Fail (baseline 20; 1 of 1 logs read; max per log)
  [OK]       GREEN fixture -> PASS 20 Success / 0 Fail  --  Ok=True Performed=20 Success=20 Fail=0
  RED     [STOP] B2-SUITE  20 performed / 18 Success / 2 Fail (baseline 20; 1 of 1 logs read; max per log) - failing: CredentialHashContract, GuestFallbackIsBareConstants
                 REQUIRED TO CLEAR: Any failing test STOPS the ship. Fix the test or the code - there is no skip flag.
  [OK]       RED fixture   -> STOP with Fail=2 and both names  --  Ok=False Performed=20 Success=18 Fail=2 names=CredentialHashContract,GuestFallbackIsBareConstants
  NULL-A  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 1 of 1 logs read)
                 REQUIRED TO CLEAR: A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.
  [OK]       NULL result-absent.log -> STOP "suite produced no results"  --  Ok=False NoResults=True LogsRead=1/1
  NULL-Z  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 1 of 1 logs read)
                 REQUIRED TO CLEAR: A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.
  [OK]       NULL zero-started.log  -> STOP "suite produced no results"  --  Ok=False NoResults=True
  NULL-E  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 1 of 1 logs read)
                 REQUIRED TO CLEAR: A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.
  [OK]       NULL empty file        -> STOP "suite produced no results"  --  Ok=False NoResults=True LogsRead=1/1
  NULL-M  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 0 of 1 logs read)
                 REQUIRED TO CLEAR: A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.
  [OK]       NULL missing file      -> STOP, reports 0 of 1 logs read  --  Ok=False NoResults=True LogsRead=0/1
  NULL@0  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 1 of 1 logs read)
                 REQUIRED TO CLEAR: A zero is not green. Either the suite never ran or this gate is reading the wrong file: check the -abslog path and read suite.out.log for a crash before the first test. There is no skip flag.
  [OK]       THE TRAP: null log at -Baseline 0 is STILL a STOP (a zero is ruled, not baselined)  --  Ok=False NoResults=True
  DUP     [PASS] B2-SUITE  20 performed / 20 Success / 0 Fail (baseline 20; 2 of 2 logs read; max per log)
  [OK]       same log given twice -> counts NOT doubled (max per log)  --  Success=20 LogsRead=2/2

--- 5. THE REAL RUN: C:/GitProjects/GitHub/GitClaudeUnrealTesting/packagedZIPofGame/.ship/20260909-200755  (script baseline 143) ---
  1,184,405 B  C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755\automation.log
  364,009 B    C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755\suite.out.log
  0 B          C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755\suite.err.log
  REAL    [PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log)
  [OK]       REAL green run -> PASS at exactly 555 / 0  --  Ok=True Performed=555 Success=555 Fail=0 LogsRead=3/3

--- 6. CENSUS: every -Simple call site in ship.ps1 and the needle it carries (a regex escape under -Simple is the defect species) ---
  MANUAL  ship.ps1:746   Select-String    <unresolved: $Pattern>
  MANUAL  ship.ps1:765   Select-String    <unresolved: $Pattern>
     ok   ship.ps1:823   Get-LogMatches   Result={Success}
     ok   ship.ps1:824   Get-LogMatches   Result={Fail}
     ok   ship.ps1:2304  Test-LogPattern  0x800711C7
     ok   ship.ps1:2559  Test-LogPattern  BUILD SUCCESSFUL
     ok   ship.ps1:2560  Test-LogPattern  BUILD FAILED
     ok   ship.ps1:2653  Test-LogPattern  draw pile from
     ok   ship.ps1:2766  Test-LogPattern  Castle-relative hero start for
  MANUAL  ship.ps1:2772  Get-LogMatches   <unresolved: $pat>
  10 -Simple site(s); 0 needle(s) carry a regex escape under -Simple
  [OK]       census: no regex-escaped needle under -Simple remains  --  flagged=0

RESULT: ALL EXPECTATIONS MET on both sides
harness exit=0
```

The three sides the task named, in one line each — **green (real):** `555 performed / 555 Success / 0 Fail` → PASS · **red:** `20 performed / 18 Success / 2 Fail ... - failing: CredentialHashContract, GuestFallbackIsBareConstants` → STOP · **null:** `NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines` → STOP (never PASS, including at `-Baseline 0`).

### 4a. The validator validated (`SHIP-§9g`): four mutants of the fixed `ship.ps1`, each run through the same harness

| Mutant | What it breaks | Harness result |
|---|---|---|
| M1 `return ,$res` → `return $res` | the organ (defect 2 regression) | **RED, exit 1** — harness throws at its own section-3 `.Count` check (`b2_verdict_check.ps1:108`) |
| M2 needles → `'Result=\{Success\}'` / `'Result=\{Fail\}'` | defect 1 regression | **RED, exit 2, 5 mismatches** — GREEN, RED, DUP and REAL all read `Success=0`; census flags 2 |
| M3 `$ok = (-not $noResults) -and ...` → `$ok = ...` | the zero rule (defect 2b) | **RED, exit 2, 1 mismatch** — caught ONLY by the `-Baseline 0` trap case (`Ok=True NoResults=True`), which is why that case exists |
| M4 max-per-log → `$success += $s.Count` | double counting | **RED, exit 2, 2 mismatches** — DUP reads 40, REAL reads `Success=1110` |

Mutants and their runs live in the session scratchpad (not the repo); the mutation recipe is four one-line string substitutions and is reproducible from this table.

## 5. What I could NOT verify without a live `/ship` (declared, `SC-§39.1`)

- The gate binding `:2327-2336` (`$v2` → `Assert-Gate`) was not executed end-to-end inside the script: the harness runs the two functions the gate consumes, and the binding is 10 lines read by eye. `ship.ps1` parses clean (0 AST errors) with the change.
- `$r2.TimedOut` path of the new remedy text — no timed-out suite exists to feed it.
- That `Assert-Gate`'s `STOP` record/summary renders the longer evidence string acceptably.
- Whether `$SUITE_BASELINE = 143` is what the lane wants for a 555-test suite — stale but **not mine to change** (another gate parameter; `SC-§100`).

## 6. Notes for QA / build-master

- **New tracked file:** `Tools/SuiteRunnerFixtures/b2_verdict_check.ps1` (pure ASCII, 0 parse errors) — `TL-§6`: `Tools/**/*.ps1` is code and the evidence corpus lives under `Tools/SuiteRunnerFixtures/`, so the harness is committed beside the fixtures rather than dying with the scratchpad. It never touches `Tools/run_suite_bounded.ps1`.
- **Commit pathspecs** (from the git root, one level up — `SC-§102`): `GitClaudeUnrealTest/Tools/Packaging/ship.ps1`, `GitClaudeUnrealTest/Tools/SuiteRunnerFixtures/b2_verdict_check.ps1`, this handoff. The working tree also carries other lanes' dirt (`CONVENTIONS.md`, `TASKBOARD.md`, `CLAUDE.md`, `handoffs/TASK-1193-buildmaster.md`, `qa/TASK-1193-cook.md`) — none of it is mine.
- `git diff` prints `warning: LF will be replaced by CRLF` for `ship.ps1` — the file was LF-only before my edit and is LF-only after (autocrlf noise, pre-existing).
- **`SHIP-§7` blind spot is NOT closed by this row** — `-DryRun` still records B2 as `PLAN` and cannot reach the parser. Recommended follow-up row (not done, out of scope): have `-DryRun` invoke `b2_verdict_check.ps1` (or at minimum print the B2 contract), so the next B2 regression cannot hide behind `DRYRUN-OK`.
- Re-run `/ship` (invocation 1) once QA passes: PHASE A+B are ~2 min; B2 should now print `[PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log); timedOut=False`.

## 7. Files touched (absolute)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1` — modified (organ `:770-780`, new `Get-SuiteVerdict` `:782-859`, B2 gate `:2321-2336`)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\SuiteRunnerFixtures\b2_verdict_check.ps1` — new
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\.claude\pipeline\handoffs\TASK-1193-b2fix-programmer.md` — this file

Not touched: `Source/**`, the editor, any build, Git, the board, `Tools/run_suite_bounded.ps1`.


---

## Loop 1 — `qa/TASK-1196-report.md` FAIL (1 BLOCKER, placement only) — 2026-09-09, gameplay-programmer

**Status: ready-for-qa** (board row not edited — the host flips `TASK-1195` under `SC-§103`). Everything above this rule is the original handoff, preserved verbatim; where this section contradicts §6/§7 above (the harness path, the commit pathspecs), **this section wins**.

### L1.1 The blocker, and what moved

`b2_verdict_check.ps1` was written into `Tools/SuiteRunnerFixtures/`, which row 1195 marks `⛔ NEVER` (`TASKBOARD:2053`, `:2055`); `TL-§6` (`CONVENTIONS:6018`) and `SHIP-§9c` cl. 6 (`:7291`) fix the ship lane's fixture home at `Tools/Packaging/Fixtures/`. My `:6030` citation was the bounded runner's own harness bullet, not the ship lane's — QA is right.

- **Moved:** `Tools/SuiteRunnerFixtures/b2_verdict_check.ps1` → **`Tools/Packaging/Fixtures/b2_verdict_check.ps1`** (directory created; plain filesystem move — the file was untracked, and no Git was used). Name kept as the orchestrator specified (QA NIT 10's `<verb>_<noun>` form was not applied; the destination name was given to me).
- **NOT moved — nothing in `Tools/SuiteRunnerFixtures/`:** all four logs the harness reads (`green-suite.log`, `red-suite.log`, `result-absent.log`, `zero-started.log`) are **tracked** (swept in by `8c444ca`) and belong to the runner's corpus (`TASK-1181`/`1185`/`1189`). I did not author them. The harness reads them **by path, read-only**, which is exactly the split `TL-§6 :6018` describes. `Tools/SuiteRunnerFixtures/` is back to its ten tracked logs and nothing else (`git status` shows no `??` under it).

### L1.2 Harness self-location — the two fixes QA named (+ the docstring and header that name the same locations)

| Where (new line) | Was | Now |
|---|---|---|
| `:31` (was `:29`) | `$ShipScript = (Join-Path $PSScriptRoot '..\Packaging\ship.ps1')` | `$ShipScript = (Join-Path $PSScriptRoot '..\ship.ps1')` — the harness is now *inside* `Packaging\` |
| `:34` (new) | — | `[string] $FixtureDir = (Join-Path $PSScriptRoot '..\..\SuiteRunnerFixtures')` — QA's suggested parameter; overridable, defaults to the runner's corpus, read-only |
| `:42` (was `:37`) | `$fx = $PSScriptRoot` | `$fx = (Resolve-Path -LiteralPath $FixtureDir).Path` |
| `:25`, `:26` (were `:25-26`) | `.EXAMPLE` invocations `-File Tools\SuiteRunnerFixtures\b2_verdict_check.ps1` | `-File Tools\Packaging\Fixtures\b2_verdict_check.ps1` — the docstring names the harness's own location, so it is part of the self-location fix |
| `:55` (was `:50`) | header printed `ship.ps1=<resolved>` | header prints `ship.ps1=<resolved>  fixtures=<resolved>` — so the run itself states **both** directories it resolved, which is the evidence this loop is for |

Two comment lines above each of `$ShipScript` and `$FixtureDir` record why (the `TL-§6` split). Net: the `param()` block grew by 5 lines, so every later line number in the original §4a shifts by +5 (e.g. mutant M1's "throws at `:108`" is now `:113`). `Get-LogMatches` / `Get-SuiteVerdict` / the gate: **untouched**, per QA finding 1's last sentence.

### L1.3 `ship.ps1` — one line, `:788`, and nothing else

```diff
@@ -785,7 +785,7 @@
 # exists to detect, on real artefacts - this one was, on the real green run's
 # automation.log (555/0 => PASS), Tools/SuiteRunnerFixtures/red-suite.log (18/2
 # => STOP) and result-absent.log (0/0 => STOP), by
-# Tools/SuiteRunnerFixtures/b2_verdict_check.ps1, which lifts THIS text out of
+# Tools/Packaging/Fixtures/b2_verdict_check.ps1, which lifts THIS text out of
 # THIS file by AST and never retypes it (SHIP-0).  The gate prints exactly the
```

Proof it is the only change this loop: I snapshotted `ship.ps1` to the scratchpad **before** editing and diffed after — one hunk, one line, a comment. Byte-level replace with an assert of exactly one occurrence; file stays LF-only, no BOM; 186,204 → 186,203 bytes. sha256 **before** `22eb09d74e688b5901f13cb47eb6b8eba8a6678dbe5686986ae89161c707742e` → **after** `f3902fae8b1f302e30dafe8d6d85f7b1ffe28bb188921e371b46ff0ff45d1bf9`. `:786` (`Tools/SuiteRunnerFixtures/red-suite.log`) is deliberately unchanged — that fixture really does live there. The parser (`:755-780`, `:805-859`), the gate binding `:2321-2336`, `$SUITE_BASELINE = 143` (`:298`), the `Success == Performed` question, per-log printing: **not touched** (`TASK-1198`'s / the manager's, `SC-§100`).

### L1.4 Re-run from the new home — the header + the three verdicts (verbatim)

Command (cwd `GitClaudeUnrealTest/`, the `-File` path absolute):
`powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\b2_verdict_check.ps1 -RealLogDir C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\.ship\20260909-200755 -RealExpect 555`

```
b2_verdict_check.ps1  PS 5.1.26100.9168  StrictMode=Latest  ship.ps1=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\Packaging\ship.ps1  fixtures=C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\SuiteRunnerFixtures

--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---
  ship.ps1 parses clean: 0 parse errors
  defined Get-LogMatches from ship.ps1:755-780
  defined Get-SuiteVerdict from ship.ps1:805-859
  ship.ps1 SUITE_BASELINE = 143
```

```
  REAL    [PASS] B2-SUITE  555 performed / 555 Success / 0 Fail (baseline 143; 3 of 3 logs read; max per log)
  RED     [STOP] B2-SUITE  20 performed / 18 Success / 2 Fail (baseline 20; 1 of 1 logs read; max per log) - failing: CredentialHashContract, GuestFallbackIsBareConstants
  NULL-A  [STOP] B2-SUITE  NOT MEASURED - suite produced no results: 0 Result={Success} and 0 Result={Fail} lines (0 performed; 1 of 1 logs read)
```

`RESULT: ALL EXPECTATIONS MET on both sides` · `harness exit=0` · every `[OK]` in §4 above reproduced (positive control still throws `The property 'Count' cannot be found on this object`; NULL-Z / NULL-E / NULL-M / NULL@0 all STOP; DUP `Success=20 LogsRead=2/2`; census `10 -Simple site(s); 0 … regex escape`). The full transcript is identical to §4 except the header's added `fixtures=` field and the `ship.ps1=` path now being reached via `..\ship.ps1`.

### L1.5 Mutant retention (QA WARN 4, `TL-§6 :6030`)

**EVIDENCE NOT RETAINED — CLAIMS ARE DECLARED.** The four mutants of §4a and their runs were scratchpad-only and are not landed beside the harness; they were **not re-run this loop** (the parser did not change). The recipe is the four one-line substitutions in the §4a table, and the harness's `-ShipScript <path>` parameter is the lane for re-running them against a mutated copy.

### L1.6 Routed / not done here (so the next reader does not look for them)

- WARN 2 (`Success == Performed`), WARN 3 (per-log counts), WARN 5 (`$SUITE_BASELINE = 143`): `TASK-1198` / manager — **not touched**, by instruction.
- NIT 6 (three stale pre-fix line numbers in §3 above: read `:1617`→`:1705`, `:1761-1766`→`:1849-1854`, `:2225-2226`→`:2310-2311`): acknowledged; the original section is preserved unedited, this line is the correction.
- NIT 7 (the "verbatim" positive control is retyped), NIT 8 (`Select-Object -First 1` → assert one definition), NIT 9 (`try/finally` around the temp file): harness-only hardening, **not done** — outside the remedy as issued; can be folded into whichever row owns the harness next.

### L1.7 For the host (`TASK-1197`)

- Pathspecs from the git root (`SC-§102`): `GitClaudeUnrealTest/Tools/Packaging/ship.ps1` · **`GitClaudeUnrealTest/Tools/Packaging/Fixtures/b2_verdict_check.ps1`** · this handoff · `qa/TASK-1196-report.md` · `qa/TASK-1193-cook.md`. **Never** `Tools/SuiteRunnerFixtures/**` (it is clean of my files now). §6/§7 above naming `Tools/SuiteRunnerFixtures/b2_verdict_check.ps1` are superseded.
- `git status --porcelain -- GitClaudeUnrealTest/Tools/` at my instant:
```
M GitClaudeUnrealTest/Tools/Packaging/ship.ps1
?? GitClaudeUnrealTest/Tools/Packaging/Fixtures/
```
- No `Source/**`, no editor, no compile, no Git, no board edit this loop.
