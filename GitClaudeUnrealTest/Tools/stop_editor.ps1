#Requires -Version 5.1
<#
.SYNOPSIS
    The ONE sanctioned way to close the Unreal editor from the pipeline (SC-118, 2026-10-04 rule change).
    Census by COMMAND LINE, refuse every -game instance, terminate the GUI editor of THIS project by PID,
    prove the never-save law by hash, print the census before and after. ASCII-only on purpose
    (PS 5.1 decodes a BOM-less UTF-8 script as ANSI, which silently corrupts match patterns).

.DESCRIPTION
    UnrealEditor.exe is ONE binary in THREE roles with ONE window title. On 2026-09-09 a kill-by-name
    stopped two of Jonathan's -game play sessions believing each was the relaunched editor. The law since
    then (SC-118): classify every instance by command line, kill by PID only, never by name. Until this
    script existed the permission file still allowed the banned 'Stop-Process -Name UnrealEditor' and
    prompted on the legal 'Stop-Process -Id', so the green path was the illegal one. This script IS the
    legal path, and it is the only editor-stopping command the allow-list grants.

    Classification of each UnrealEditor.exe by its command line:
      PLAY      contains ' -game'                       -> Jonathan's play session. NEVER touched. Reported.
      HEADLESS  contains -run= / -AuraHeadless / -RenderOffScreen / -unattended -> left alone unless -IncludeHeadless.
      EDITOR    names THIS project's .uproject, none of the above -> ours; the standing close/reopen grant covers it.
      OTHER     another project's editor                -> left alone. Reported.

    Default action: terminate the single EDITOR instance (the ruled-correct close when only discardable
    assets are dirty: it eliminates the Slate "Save Content" modal whose Save Selected sits beside
    Don't Save). With -TargetPid, terminate exactly that PID after classifying it. More than one EDITOR
    instance requires -TargetPid. -WhatIf prints the census and exits.

    Never-save proof: the SHA-256, size and mtime of -HashAsset (default Content/Maps/L_Arena.umap) are
    read before and after; a change is exit 6 so the caller cannot miss that an asset was written.

.PARAMETER TargetPid
    Terminate exactly this PID. It must classify as EDITOR (or HEADLESS with -IncludeHeadless).
.PARAMETER WhatIf
    Census only. Terminates nothing.
.PARAMETER RequireAllDown
    After acting, exit 3 if ANY UnrealEditor.exe survives (the cook pre-flight, SHIP-10). Never kills PLAY.
.PARAMETER IncludeHeadless
    Also terminate HEADLESS instances (Aura's auto-launched -AuraHeadless twin, commandlets). Never PLAY.
.PARAMETER WaitSeconds
    How long to wait for a terminated process to exit. Default 60.
.PARAMETER HashAsset
    Project-relative asset whose bytes must not change. '' disables the check.

.EXAMPLE
    powershell -NoProfile -File Tools\stop_editor.ps1 -WhatIf
    powershell -NoProfile -File Tools\stop_editor.ps1
    powershell -NoProfile -File Tools\stop_editor.ps1 -TargetPid 18832
    powershell -NoProfile -File Tools\stop_editor.ps1 -RequireAllDown

.NOTES
    Exit codes: 0 done (census after shows what remains) - 2 no EDITOR instance of this project running
                3 -RequireAllDown and an instance survives (PLAY: ask Jonathan and wait) - 4 refused
                (PID classifies as PLAY/OTHER, or several EDITOR instances without -TargetPid) - 5 process did not
                exit within -WaitSeconds - 6 the hash asset CHANGED (never-save law violated) - 7 unexpected error
    Closing the GUI editor can auto-launch a HEADLESS -AuraHeadless editor minutes later (SC-118 cl. 8);
    re-run with -WhatIf before a compile or a cook and act on what the census shows.
#>
[CmdletBinding()]
param(
    [int] $TargetPid = 0,
    [switch] $WhatIf,
    [switch] $RequireAllDown,
    [switch] $IncludeHeadless,
    [int] $WaitSeconds = 60,
    [string] $HashAsset = 'Content/Maps/L_Arena.umap'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ToolsDir = $PSScriptRoot
$ProjectRoot = (Resolve-Path (Join-Path $ToolsDir '..')).Path
$UProject = Get-ChildItem -LiteralPath $ProjectRoot -Filter '*.uproject' | Select-Object -First 1
if ($null -eq $UProject) { Write-Host "stop_editor: no .uproject under $ProjectRoot"; exit 7 }
$UProjectName = $UProject.Name

function Get-EditorCensus {
    $rows = @()
    $procs = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" -ErrorAction SilentlyContinue
    foreach ($p in @($procs)) {
        $cmd = [string]$p.CommandLine
        $class = 'OTHER'
        if ($cmd -match '(^|\s)-game(\s|$)') { $class = 'PLAY' }
        elseif ($cmd -match '-run=|-AuraHeadless|-RenderOffScreen|(^|\s)-unattended(\s|$)') { $class = 'HEADLESS' }
        elseif ($cmd.ToLower().Contains($UProjectName.ToLower())) { $class = 'EDITOR' }
        $rows += [pscustomobject]@{
            Pid      = [int]$p.ProcessId
            Class    = $class
            Created  = $p.CreationDate
            Parent   = [int]$p.ParentProcessId
            CmdLine  = $cmd
        }
    }
    return ,$rows
}

function Show-Census([string]$Label, $Rows) {
    # An empty census binds to this parameter as $null, and @($null).Count is 1 - normalise first.
    $list = @()
    if ($null -ne $Rows) { $list = @($Rows | Where-Object { $null -ne $_ }) }
    Write-Host ("stop_editor: census {0}: {1} UnrealEditor.exe process(es)" -f $Label, $list.Count)
    foreach ($r in $list) {
        $created = ''
        if ($null -ne $r.Created) { $created = $r.Created.ToString('yyyy-MM-dd HH:mm:ss') }
        Write-Host ("  PID {0,-6} {1,-8} created {2}  parent {3}  cmd: {4}" -f $r.Pid, $r.Class, $created, $r.Parent, $r.CmdLine)
    }
}

function Get-AssetState([string]$Rel) {
    if ([string]::IsNullOrEmpty($Rel)) { return $null }
    $full = Join-Path $ProjectRoot $Rel
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { Write-Host "stop_editor: hash asset absent, check skipped: $Rel"; return $null }
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.IO.File]::ReadAllBytes($full)
        $hex = ([System.BitConverter]::ToString($sha.ComputeHash($bytes)) -replace '-', '').ToLowerInvariant()
    } finally { $sha.Dispose() }
    $item = Get-Item -LiteralPath $full
    return [pscustomobject]@{ Path = $Rel; Sha256 = $hex; Bytes = $item.Length; Mtime = $item.LastWriteTimeUtc.ToString('o') }
}

try {
    $before = Get-EditorCensus
    Show-Census 'BEFORE' $before
    if ($WhatIf) { Write-Host 'stop_editor: -WhatIf, nothing terminated'; exit 0 }

    $targets = @()
    if ($TargetPid -gt 0) {
        $row = @($before | Where-Object { $_.Pid -eq $TargetPid })
        if ($row.Count -eq 0) { Write-Host "stop_editor: REFUSED - PID $TargetPid is not an UnrealEditor.exe process"; exit 4 }
        $cls = $row[0].Class
        $ok = ($cls -eq 'EDITOR') -or ($cls -eq 'HEADLESS' -and $IncludeHeadless)
        if (-not $ok) { Write-Host "stop_editor: REFUSED - PID $TargetPid classifies as $cls (SC-118: a -game session is Jonathan's; OTHER is not ours)"; exit 4 }
        $targets = $row
    } else {
        $editors = @($before | Where-Object { $_.Class -eq 'EDITOR' })
        if ($editors.Count -gt 1) { Write-Host "stop_editor: REFUSED - $($editors.Count) EDITOR instances; pass -TargetPid"; exit 4 }
        $targets = $editors
        if ($IncludeHeadless) { $targets += @($before | Where-Object { $_.Class -eq 'HEADLESS' }) }
        if (@($targets).Count -eq 0) {
            Write-Host 'stop_editor: no EDITOR instance of this project is running (nothing terminated)'
            if ($RequireAllDown -and @($before).Count -gt 0) {
                Write-Host 'stop_editor: -RequireAllDown FAILED - instances survive (PLAY = ask Jonathan and wait; HEADLESS = -IncludeHeadless)'
                exit 3
            }
            exit 2
        }
    }

    $hashBefore = Get-AssetState $HashAsset
    if ($null -ne $hashBefore) { Write-Host ("stop_editor: hash before  {0}  {1} B  {2}  sha256 {3}" -f $hashBefore.Path, $hashBefore.Bytes, $hashBefore.Mtime, $hashBefore.Sha256) }

    foreach ($t in @($targets)) {
        Write-Host ("stop_editor: terminating PID {0} ({1})" -f $t.Pid, $t.Class)
        Stop-Process -Id $t.Pid -Force -ErrorAction Stop
    }
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    foreach ($t in @($targets)) {
        while ((Get-Date) -lt $deadline) {
            $alive = Get-Process -Id $t.Pid -ErrorAction SilentlyContinue
            if ($null -eq $alive) { break }
            Start-Sleep -Milliseconds 500
        }
        $alive = Get-Process -Id $t.Pid -ErrorAction SilentlyContinue
        if ($null -ne $alive) { Write-Host "stop_editor: PID $($t.Pid) still alive after $WaitSeconds s"; exit 5 }
        Write-Host ("stop_editor: PID {0} exited" -f $t.Pid)
    }
    Start-Sleep -Seconds 2

    $hashAfter = Get-AssetState $HashAsset
    if ($null -ne $hashBefore -and $null -ne $hashAfter) {
        Write-Host ("stop_editor: hash after   {0}  {1} B  {2}  sha256 {3}" -f $hashAfter.Path, $hashAfter.Bytes, $hashAfter.Mtime, $hashAfter.Sha256)
        if ($hashAfter.Sha256 -ne $hashBefore.Sha256) { Write-Host 'stop_editor: NEVER-SAVE LAW VIOLATED - the hash asset changed'; Show-Census 'AFTER' (Get-EditorCensus); exit 6 }
        Write-Host 'stop_editor: hash MATCH (never-save law held)'
    }

    $after = Get-EditorCensus
    Show-Census 'AFTER' $after
    if ($RequireAllDown -and @($after).Count -gt 0) {
        Write-Host 'stop_editor: -RequireAllDown FAILED - instances survive (PLAY = ask Jonathan and wait; HEADLESS = -IncludeHeadless)'
        exit 3
    }
    Write-Host 'stop_editor: done'
    exit 0
}
catch {
    Write-Host ("stop_editor: unexpected error - {0}" -f $_.Exception.Message)
    exit 7
}
