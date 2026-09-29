<#
.SYNOPSIS
  TASK-1567 - validates ship.ps1's C2-COOK-BP-ERRORS verdict on all three sides
  (SHIP-9c cl. 6: green / red / EMPTY, one triple per log-parsing gate, on
  captured or truncated REAL logs - TL-6).

  Get-LogMatches and Get-CookBlueprintVerdict are lifted OUT OF ship.ps1 BY AST:
  the exact text in the file is what runs here, never a retyped copy (SHIP-0).
  Exactly ONE definition of each must exist or the harness refuses to run
  (qa/TASK-1196-report.md NIT 8).  It runs under the same
  Set-StrictMode -Version Latest as ship.ps1.

  Each fixture is passed the way the gate passes $clogs - a PAIR (stdout log,
  stderr log) - with the 0-byte empty.log as the stderr member, because every
  real ship's cook.err.log on disk is 0 bytes.  Verdict AND reason are asserted:
    * green  cook-bp-green.log     + empty.log  => PASS / NO-BLUEPRINT-ERROR
    * red    cook-bp-error-red.log + empty.log  => STOP / BLUEPRINT-ERROR
    * null   empty.log             + empty.log  => STOP / SUMMARY-ABSENT
  A fourth case pairs the two real captures with each other (TASK-1587,
  qa/TASK-1568.md WARN-2), so the gate sees a needle line AND a summary line:
    * pair   cook-bp-error-red.log + cook-bp-green.log => STOP / BLUEPRINT-ERROR,
             summary count 1.  The 'Errorx' needle mutant and a summary-first
             order swap each flip THIS case's VERDICT to PASS (SHIP-9), where the
             triple sees the first only as a changed reason and the second not
             at all.
  Optional: -RealLogDir <a ship run dir> also runs the verdict on that run's
  real cook.out.log + cook.err.log pair and expects PASS / NO-BLUEPRINT-ERROR.

  The fixtures carry no header on purpose (a header would stop them being real
  logs).  Their source file, line range and sha256 live in
  .claude/pipeline/handoffs/TASK-1567-programmer.md.

  WRITES NOTHING.  There is no temp file: the null side is the tracked 0-byte
  empty.log, which is what b2_verdict_check.ps1's temp file stood in for, so
  NIT 9's try/finally has nothing to clean here.

  Exit 0 = every expectation met; exit 2 = at least one MISMATCH.

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\check_cook_bp_verdict.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\check_cook_bp_verdict.ps1 -RealLogDir C:\...\packagedZIPofGame\.ship\20260910-063540
#>
param(
    # This harness lives at Tools/Packaging/Fixtures/ (the ship lane's fixture
    # home, TL-6 / SHIP-9c cl. 6), so ship.ps1 is one level up and the
    # fixtures sit beside it.
    [string] $ShipScript = (Join-Path $PSScriptRoot '..\ship.ps1'),
    [string] $FixtureDir = $PSScriptRoot,
    [string] $RealLogDir = ''
)
Set-StrictMode -Version Latest      # SAME mode as ship.ps1 - not optional
$ErrorActionPreference = 'Stop'
$ShipScript = (Resolve-Path -LiteralPath $ShipScript).Path
$fx = (Resolve-Path -LiteralPath $FixtureDir).Path
$script:Bad = 0

function Check([string]$Case, [bool]$Cond, [string]$Detail) {
    if ($Cond) { "  [OK]       {0}  --  {1}" -f $Case, $Detail }
    else       { $script:Bad++; "  [MISMATCH] {0}  --  {1}" -f $Case, $Detail }
}
function Show([string]$Side, [hashtable]$V) {
    $mark = 'STOP'; if ($V.Ok) { $mark = 'PASS' }
    "  {0,-6} [{1}] C2-COOK-BP-ERRORS  {2}" -f $Side, $mark, $V.Evidence
    if ($V.Remedy) { "                 REQUIRED TO CLEAR: {0}" -f $V.Remedy }
}
function Facts([hashtable]$V) {
    "Verdict={0} Reason={1} Ok={2} BpErrorCount={3} Quoted={4} SummaryCount={5} SummaryErrors={6} LogsRead={7}/{8}" -f `
        $V.Verdict, $V.Reason, $V.Ok, $V.BpErrorCount, $V.Quoted.Count, $V.SummaryCount, $V.SummaryErrors, $V.LogsRead, $V.LogsGiven
}

"check_cook_bp_verdict.ps1  PS {0}  StrictMode=Latest  ship.ps1={1}  fixtures={2}" -f $PSVersionTable.PSVersion, $ShipScript, $fx
''
'--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---'
$tokens = $null; $errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($ShipScript, [ref]$tokens, [ref]$errors)
if ($errors.Count -gt 0) { throw ('ship.ps1 does not parse: ' + (($errors | ForEach-Object { $_.Message }) -join '; ')) }
"  ship.ps1 parses clean: {0} parse errors" -f $errors.Count
$fns = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)
foreach ($name in @('Get-LogMatches', 'Get-CookBlueprintVerdict')) {
    $defs = @($fns | Where-Object { $_.Name -eq $name })
    # NIT 8: 'Select-Object -First 1' would silently pick one of two
    # same-named definitions.  Exactly one, or the harness does not run.
    if ($defs.Count -ne 1) { throw ("expected exactly 1 definition of {0} in {1}, found {2}" -f $name, $ShipScript, $defs.Count) }
    . ([scriptblock]::Create($defs[0].Extent.Text))
    "  defined {0} from ship.ps1:{1}-{2} (exactly 1 definition)" -f $name, $defs[0].Extent.StartLineNumber, $defs[0].Extent.EndLineNumber
}

$green = Join-Path $fx 'cook-bp-green.log'
$red   = Join-Path $fx 'cook-bp-error-red.log'
$empty = Join-Path $fx 'empty.log'
foreach ($p in @($green, $red, $empty)) { if (-not (Test-Path -LiteralPath $p)) { throw "fixture missing: $p" } }
foreach ($p in @($green, $red, $empty)) { "  fixture {0,7:N0} B  {1}" -f (Get-Item -LiteralPath $p).Length, $p }
Check 'empty.log is 0 bytes (else the null side is not the null side)' ((Get-Item -LiteralPath $empty).Length -eq 0) ("Length={0}" -f (Get-Item -LiteralPath $empty).Length)

''
'--- 2. THE TRIPLE: verdict AND reason, each fixture passed as the gate passes $clogs (stdout, stderr) ---'
$vg = Get-CookBlueprintVerdict -Paths @($green, $empty); Show 'GREEN' $vg
Check 'GREEN cook-bp-green.log     -> PASS / NO-BLUEPRINT-ERROR, summary quoted' `
    (($vg.Verdict -eq 'PASS') -and ($vg.Reason -eq 'NO-BLUEPRINT-ERROR') -and $vg.Ok -and ($vg.BpErrorCount -eq 0) -and ($vg.SummaryCount -eq 1) -and ($vg.SummaryErrors -eq 0) -and ($vg.LogsRead -eq 2)) `
    (Facts $vg)
$vr = Get-CookBlueprintVerdict -Paths @($red, $empty); Show 'RED' $vr
Check 'RED   cook-bp-error-red.log -> STOP / BLUEPRINT-ERROR, 4 hits quoted, all BP_Basic_Movement' `
    (($vr.Verdict -eq 'STOP') -and ($vr.Reason -eq 'BLUEPRINT-ERROR') -and (-not $vr.Ok) -and ($vr.BpErrorCount -eq 4) -and ($vr.Quoted.Count -eq 4) -and (@($vr.Quoted | Where-Object { $_ -notlike '*BP_Basic_Movement*' }).Count -eq 0)) `
    (Facts $vr)
$vn = Get-CookBlueprintVerdict -Paths @($empty, $empty); Show 'NULL' $vn
Check 'NULL  empty.log             -> STOP / SUMMARY-ABSENT (a zero is a verdict, never a pass)' `
    (($vn.Verdict -eq 'STOP') -and ($vn.Reason -eq 'SUMMARY-ABSENT') -and (-not $vn.Ok) -and ($vn.BpErrorCount -eq 0) -and ($vn.SummaryCount -eq 0) -and ($vn.LogsRead -eq 2)) `
    (Facts $vn)

''
'--- 2b. THE PAIR: the two real captures as ONE (stdout, stderr) pair - the VERDICT discriminates, not only the reason ---'
# TASK-1587 (qa/TASK-1568.md WARN-2).  The triple's RED side carries no summary
# line, so the 'Errorx' needle mutant only moves its REASON (BLUEPRINT-ERROR ->
# SUMMARY-ABSENT, still STOP), and a swap that checks the summary first and
# passes on it passes all three.  Here the red capture is the stdout member and
# the green capture the stderr member, so the gate reads 4 needle lines AND one
# summary line: only the needle-first order stops it.  No new bytes, no
# synthetic file - the same two tracked fixtures, paired.  This pair is NOT a
# real cook's shape (a real one carrying these errors would print
# 'Failure - N error(s)'); it is two real captures combined so that both
# branches the order decides between are armed at once.
$vp = Get-CookBlueprintVerdict -Paths @($red, $green); Show 'PAIR' $vp
Check 'PAIR  cook-bp-error-red.log + cook-bp-green.log -> STOP / BLUEPRINT-ERROR with the summary present (the needle is read first)' `
    (($vp.Verdict -eq 'STOP') -and ($vp.Reason -eq 'BLUEPRINT-ERROR') -and (-not $vp.Ok) -and ($vp.BpErrorCount -eq 4) -and ($vp.Quoted.Count -eq 4) -and ($vp.SummaryCount -eq 1) -and ($vp.SummaryErrors -eq 0) -and ($vp.LogsRead -eq 2)) `
    (Facts $vp)

if ($RealLogDir) {
    ''
    "--- 3. THE REAL RUN: {0} ---" -f $RealLogDir
    $real = @((Join-Path $RealLogDir 'cook.out.log'), (Join-Path $RealLogDir 'cook.err.log'))
    foreach ($p in $real) { "  {0,-12} {1}" -f $(if (Test-Path -LiteralPath $p) { ('{0:N0} B' -f (Get-Item -LiteralPath $p).Length) } else { 'MISSING' }), $p }
    $vR = Get-CookBlueprintVerdict -Paths $real; Show 'REAL' $vR
    Check 'REAL cook log pair -> PASS / NO-BLUEPRINT-ERROR (the "cannot say" branch is NOT taken on a real green cook)' `
        (($vR.Verdict -eq 'PASS') -and ($vR.Reason -eq 'NO-BLUEPRINT-ERROR') -and $vR.Ok -and ($vR.LogsRead -eq 2)) `
        (Facts $vR)
}

''
if ($script:Bad -gt 0) { "RESULT: {0} MISMATCH(ES) - the gate is NOT validated" -f $script:Bad; exit 2 }
'RESULT: ALL EXPECTATIONS MET on all three sides and the pair'
exit 0
