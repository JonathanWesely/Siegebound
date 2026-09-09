<#
.SYNOPSIS
  TASK-1193 - validates ship.ps1's B2-SUITE parser on BOTH sides (SHIP-9c).

  Get-LogMatches and Get-SuiteVerdict are lifted OUT OF ship.ps1 BY AST - the
  exact text in the file is what runs here, never a retyped copy (SHIP-0).  The
  harness runs under the same Set-StrictMode -Version Latest as ship.ps1:214,
  because without it the defect this task fixed is invisible.

  Sides exercised:
    * positive control  - the PRE-FIX organ (verbatim from 2cc8213) must still
                          crash on this machine, or the harness cannot see the
                          defect it claims to test (SC-39 / SC-114 cl. 3a)
    * green             - green-suite.log (20/0)  => PASS   [+ a real run dir]
    * red               - red-suite.log   (18/2)  => STOP, fail count + names
    * null              - result-absent.log, zero-started.log, an EMPTY file and
                          a MISSING file => STOP "suite produced no results",
                          and STILL a STOP at -Baseline 0 (the @()-wrap trap)
    * the -Simple census - every -Simple call site in ship.ps1 with its needle;
                          any needle carrying a regex escape is flagged

  Exit 0 = every expectation met; exit 2 = at least one MISMATCH.

.EXAMPLE
  powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\b2_verdict_check.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Packaging\Fixtures\b2_verdict_check.ps1 -RealLogDir C:\...\packagedZIPofGame\.ship\20260909-200755 -RealExpect 555
#>
param(
    # TASK-1195 loop 1: this harness lives at Tools/Packaging/Fixtures/ - the ship
    # lane's fixture home (TL-6 table / SHIP-9c cl. 6) - so ship.ps1 is one level up.
    [string] $ShipScript      = (Join-Path $PSScriptRoot '..\ship.ps1'),
    # The log corpus stays in Tools/SuiteRunnerFixtures/: that directory is the
    # RUNNER's (TASK-1185/1189 write it); this harness only READS it, never writes.
    [string] $FixtureDir      = (Join-Path $PSScriptRoot '..\..\SuiteRunnerFixtures'),
    [string] $RealLogDir      = '',
    [int]    $RealExpect      = -1,
    [int]    $FixtureBaseline = 20
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
    $verdict = 'STOP'; if ($V.Ok) { $verdict = 'PASS' }
    "  {0,-7} [{1}] B2-SUITE  {2}" -f $Side, $verdict, $V.Evidence
    if ($V.Remedy) { "                 REQUIRED TO CLEAR: {0}" -f $V.Remedy }
}

"b2_verdict_check.ps1  PS {0}  StrictMode=Latest  ship.ps1={1}  fixtures={2}" -f $PSVersionTable.PSVersion, $ShipScript, $fx
''
'--- 1. LIFT THE FUNCTIONS OUT OF ship.ps1 BY AST (exact file text, not a copy) ---'
$tokens = $null; $errors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($ShipScript, [ref]$tokens, [ref]$errors)
if ($errors.Count -gt 0) { throw ('ship.ps1 does not parse: ' + (($errors | ForEach-Object { $_.Message }) -join '; ')) }
"  ship.ps1 parses clean: {0} parse errors" -f $errors.Count
$fns = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.FunctionDefinitionAst] }, $true)
foreach ($name in @('Get-LogMatches', 'Get-SuiteVerdict')) {
    $fn = $fns | Where-Object { $_.Name -eq $name } | Select-Object -First 1
    if ($null -eq $fn) { throw "function $name not found in $ShipScript" }
    . ([scriptblock]::Create($fn.Extent.Text))
    "  defined {0} from ship.ps1:{1}-{2}" -f $name, $fn.Extent.StartLineNumber, $fn.Extent.EndLineNumber
}
$bm = [regex]::Match((Get-Content -LiteralPath $ShipScript -Raw), '(?m)^\$SUITE_BASELINE\s*=\s*(\d+)')
if (-not $bm.Success) { throw 'SUITE_BASELINE not found in ship.ps1' }
$RealBaseline = [int]$bm.Groups[1].Value
"  ship.ps1 SUITE_BASELINE = {0}" -f $RealBaseline

$green   = Join-Path $fx 'green-suite.log'
$red     = Join-Path $fx 'red-suite.log'
$absent  = Join-Path $fx 'result-absent.log'
$zero    = Join-Path $fx 'zero-started.log'
$missing = Join-Path $fx 'this-log-does-not-exist-1193.log'
$empty   = Join-Path $env:TEMP ('b2-empty-{0}.log' -f [guid]::NewGuid().ToString('N'))
Set-Content -LiteralPath $empty -Value '' -NoNewline
foreach ($p in @($green, $red, $absent, $zero)) { if (-not (Test-Path -LiteralPath $p)) { throw "fixture missing: $p" } }

''
'--- 2. POSITIVE CONTROL: the PRE-FIX organ, verbatim from ship.ps1@2cc8213 :755-771 ---'
function Get-LogMatches-Old {
    param([Parameter(Mandatory=$true)][string[]] $Paths, [Parameter(Mandatory=$true)][string] $Pattern, [switch] $Simple)
    $res = @()
    foreach ($p in $Paths) {
        if (-not (Test-Path -LiteralPath $p)) { continue }
        if ($Simple) { $res += @(Select-String -LiteralPath $p -Pattern $Pattern -SimpleMatch -ErrorAction SilentlyContinue) }
        else         { $res += @(Select-String -LiteralPath $p -Pattern $Pattern -ErrorAction SilentlyContinue) }
    }
    return $res
}
$escN = @(Select-String -LiteralPath $green -Pattern 'Result=\{Success\}' -SimpleMatch).Count
$litN = @(Select-String -LiteralPath $green -Pattern 'Result={Success}'   -SimpleMatch).Count
$rxN  = @(Select-String -LiteralPath $green -Pattern 'Result=\{Success\}').Count
"  green-suite.log: -SimpleMatch 'Result=\{{Success\}}' = {0}   -SimpleMatch 'Result={{Success}}' = {1}   regex 'Result=\{{Success\}}' = {2}" -f $escN, $litN, $rxN
Check 'defect 1: escaped needle under -SimpleMatch matches NOTHING; literal == regex' (($escN -eq 0) -and ($litN -gt 0) -and ($litN -eq $rxN)) ("escaped={0} literal={1} regex={2}" -f $escN, $litN, $rxN)
$threw = $false; $msg = ''
try { $null = (Get-LogMatches-Old -Paths @($green) -Pattern 'Result=\{Success\}' -Simple).Count } catch { $threw = $true; $msg = $_.Exception.Message }
Check 'defect 2: OLD organ, ZERO hits, .Count THROWS (the live B2 crash)' $threw ("threw=" + $threw + " : " + $msg)
$threw = $false; $msg = ''
try { $null = (Get-LogMatches-Old -Paths @($green) -Pattern 'tests performed' -Simple).Count } catch { $threw = $true; $msg = $_.Exception.Message }
Check 'defect 2 sibling: OLD organ, ONE hit, .Count THROWS ($perf / $deckHits species)' $threw ("threw=" + $threw + " : " + $msg)

''
'--- 3. REPAIRED ORGAN: Get-LogMatches from the file returns an ARRAY for 0 / 1 / N hits and a missing path ---'
$z = Get-LogMatches -Paths @($green)   -Pattern 'NO-SUCH-NEEDLE-1193' -Simple
$o = Get-LogMatches -Paths @($green)   -Pattern 'tests performed'     -Simple
$n = Get-LogMatches -Paths @($red)     -Pattern 'Result={Fail}'       -Simple
$m = Get-LogMatches -Paths @($missing) -Pattern 'anything'            -Simple
Check 'zero hits    -> Object[] Count 0' (($z -is [array]) -and ($z.Count -eq 0)) ("type={0} Count={1}" -f $z.GetType().Name, $z.Count)
Check 'one hit      -> Object[] Count 1, indexable' (($o -is [array]) -and ($o.Count -eq 1)) ("type={0} Count={1} line={2}" -f $o.GetType().Name, $o.Count, $o[$o.Count - 1].LineNumber)
Check 'N hits       -> Object[] Count N' (($n -is [array]) -and ($n.Count -eq 2)) ("type={0} Count={1} lines={2}" -f $n.GetType().Name, $n.Count, (($n | ForEach-Object { $_.LineNumber }) -join ','))
Check 'missing path -> Object[] Count 0 (no throw)' (($m -is [array]) -and ($m.Count -eq 0)) ("type={0} Count={1}" -f $m.GetType().Name, $m.Count)

''
"--- 4. VERDICTS from Get-SuiteVerdict (fixture baseline {0}) ---" -f $FixtureBaseline
$vg = Get-SuiteVerdict -Paths @($green) -Baseline $FixtureBaseline;  Show 'GREEN' $vg
Check 'GREEN fixture -> PASS 20 Success / 0 Fail' ($vg.Ok -and ($vg.Success -eq 20) -and ($vg.Fail -eq 0) -and ($vg.Performed -eq 20) -and (-not $vg.NoResults)) ("Ok={0} Performed={1} Success={2} Fail={3}" -f $vg.Ok, $vg.Performed, $vg.Success, $vg.Fail)
$vr = Get-SuiteVerdict -Paths @($red) -Baseline $FixtureBaseline;    Show 'RED' $vr
Check 'RED fixture   -> STOP with Fail=2 and both names' ((-not $vr.Ok) -and ($vr.Fail -eq 2) -and ($vr.Success -eq 18) -and ($vr.FailNames.Count -eq 2) -and (-not $vr.NoResults)) ("Ok={0} Performed={1} Success={2} Fail={3} names={4}" -f $vr.Ok, $vr.Performed, $vr.Success, $vr.Fail, ($vr.FailNames -join ','))
$va = Get-SuiteVerdict -Paths @($absent) -Baseline $FixtureBaseline; Show 'NULL-A' $va
Check 'NULL result-absent.log -> STOP "suite produced no results"' ((-not $va.Ok) -and $va.NoResults -and ($va.Evidence -like '*suite produced no results*')) ("Ok={0} NoResults={1} LogsRead={2}/{3}" -f $va.Ok, $va.NoResults, $va.LogsRead, $va.LogsGiven)
$vz = Get-SuiteVerdict -Paths @($zero) -Baseline $FixtureBaseline;   Show 'NULL-Z' $vz
Check 'NULL zero-started.log  -> STOP "suite produced no results"' ((-not $vz.Ok) -and $vz.NoResults -and ($vz.Evidence -like '*suite produced no results*')) ("Ok={0} NoResults={1}" -f $vz.Ok, $vz.NoResults)
$ve = Get-SuiteVerdict -Paths @($empty) -Baseline $FixtureBaseline;  Show 'NULL-E' $ve
Check 'NULL empty file        -> STOP "suite produced no results"' ((-not $ve.Ok) -and $ve.NoResults) ("Ok={0} NoResults={1} LogsRead={2}/{3}" -f $ve.Ok, $ve.NoResults, $ve.LogsRead, $ve.LogsGiven)
$vm = Get-SuiteVerdict -Paths @($missing) -Baseline $FixtureBaseline; Show 'NULL-M' $vm
Check 'NULL missing file      -> STOP, reports 0 of 1 logs read' ((-not $vm.Ok) -and $vm.NoResults -and ($vm.LogsRead -eq 0) -and ($vm.LogsGiven -eq 1)) ("Ok={0} NoResults={1} LogsRead={2}/{3}" -f $vm.Ok, $vm.NoResults, $vm.LogsRead, $vm.LogsGiven)
$v0 = Get-SuiteVerdict -Paths @($absent) -Baseline 0;                 Show 'NULL@0' $v0
Check 'THE TRAP: null log at -Baseline 0 is STILL a STOP (a zero is ruled, not baselined)' ((-not $v0.Ok) -and $v0.NoResults) ("Ok={0} NoResults={1}" -f $v0.Ok, $v0.NoResults)
$vd = Get-SuiteVerdict -Paths @($green, $green) -Baseline $FixtureBaseline; Show 'DUP' $vd
Check 'same log given twice -> counts NOT doubled (max per log)' ($vd.Ok -and ($vd.Success -eq 20) -and ($vd.LogsRead -eq 2)) ("Success={0} LogsRead={1}/{2}" -f $vd.Success, $vd.LogsRead, $vd.LogsGiven)

if ($RealLogDir) {
    ''
    "--- 5. THE REAL RUN: {0}  (script baseline {1}) ---" -f $RealLogDir, $RealBaseline
    $real = @((Join-Path $RealLogDir 'automation.log'), (Join-Path $RealLogDir 'suite.out.log'), (Join-Path $RealLogDir 'suite.err.log'))
    foreach ($p in $real) { "  {0,-12} {1}" -f $(if (Test-Path -LiteralPath $p) { ('{0:N0} B' -f (Get-Item -LiteralPath $p).Length) } else { 'MISSING' }), $p }
    $vR = Get-SuiteVerdict -Paths $real -Baseline $RealBaseline;      Show 'REAL' $vR
    $cond = $vR.Ok -and ($vR.Fail -eq 0) -and ($vR.Success -ge $RealBaseline) -and ($vR.Success -eq $vR.Performed) -and ($vR.LogsRead -eq 3)
    if ($RealExpect -ge 0) { $cond = $cond -and ($vR.Success -eq $RealExpect) -and ($vR.Performed -eq $RealExpect) }
    Check ('REAL green run -> PASS' + $(if ($RealExpect -ge 0) { " at exactly $RealExpect / 0" } else { '' })) $cond ("Ok={0} Performed={1} Success={2} Fail={3} LogsRead={4}/{5}" -f $vR.Ok, $vR.Performed, $vR.Success, $vR.Fail, $vR.LogsRead, $vR.LogsGiven)
}

''
'--- 6. CENSUS: every -Simple call site in ship.ps1 and the needle it carries (a regex escape under -Simple is the defect species) ---'
$cmds = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.CommandAst] }, $true)
$assigns = $ast.FindAll({ param($n) $n -is [System.Management.Automation.Language.AssignmentStatementAst] }, $true)
function Resolve-Needle($arg) {
    if ($arg -is [System.Management.Automation.Language.StringConstantExpressionAst]) { return @($arg.Value) }
    if ($arg -is [System.Management.Automation.Language.VariableExpressionAst]) {
        $vn = $arg.VariablePath.UserPath
        foreach ($a in $assigns) {
            if (($a.Left -is [System.Management.Automation.Language.VariableExpressionAst]) -and ($a.Left.VariablePath.UserPath -eq $vn)) {
                $rhs = $a.Right
                if ($rhs -is [System.Management.Automation.Language.CommandExpressionAst]) { $rhs = $rhs.Expression }
                if ($rhs -is [System.Management.Automation.Language.StringConstantExpressionAst]) { return @($rhs.Value) }
                if ($rhs -is [System.Management.Automation.Language.ArrayLiteralAst]) { return @($rhs.Elements | ForEach-Object { $_.Value }) }
                if ($rhs -is [System.Management.Automation.Language.ArrayExpressionAst]) {
                    $out = @()
                    foreach ($st in $rhs.SubExpression.Statements) {
                        $ex = $st.PipelineElements[0].Expression
                        if ($ex -is [System.Management.Automation.Language.ArrayLiteralAst]) { foreach ($e in $ex.Elements) { $out += $e.Value } }
                        else { $out += $ex.Value }
                    }
                    return ,$out
                }
            }
        }
        return @("<unresolved: `$$vn>")
    }
    return @("<unresolved: " + $arg.Extent.Text + ">")
}
$flagged = 0; $sites = 0
foreach ($c in $cmds) {
    $cname = $c.GetCommandName()
    if ($cname -notin @('Get-LogMatches', 'Test-LogPattern', 'Select-String')) { continue }
    $hasSimple = $false; $needleArg = $null
    for ($i = 1; $i -lt $c.CommandElements.Count; $i++) {
        $el = $c.CommandElements[$i]
        if ($el -is [System.Management.Automation.Language.CommandParameterAst]) {
            if ($el.ParameterName -in @('Simple', 'SimpleMatch')) { $hasSimple = $true }
            if (($el.ParameterName -eq 'Pattern') -and (($i + 1) -lt $c.CommandElements.Count)) { $needleArg = $c.CommandElements[$i + 1] }
        }
    }
    if (-not $hasSimple) { continue }
    $sites++
    $needles = Resolve-Needle $needleArg
    foreach ($nd in $needles) {
        $isEscaped = ($nd -match '\\[{}()\[\]sdwSDW.*+?|^$\\]')
        $mark = '   ok  '; if ($isEscaped) { $mark = 'DEFECT '; $flagged++ }
        if ($nd -like '<unresolved*') { $mark = 'MANUAL ' }
        "  {0} ship.ps1:{1,-5} {2,-16} {3}" -f $mark, $c.Extent.StartLineNumber, $cname, $nd
    }
}
"  {0} -Simple site(s); {1} needle(s) carry a regex escape under -Simple" -f $sites, $flagged
Check 'census: no regex-escaped needle under -Simple remains' ($flagged -eq 0) ("flagged=" + $flagged)

Remove-Item -LiteralPath $empty -Force -ErrorAction SilentlyContinue
''
if ($script:Bad -gt 0) { "RESULT: {0} MISMATCH(ES) - the gate is NOT validated" -f $script:Bad; exit 2 }
'RESULT: ALL EXPECTATIONS MET on both sides'
exit 0
