#Requires -Version 5.1
<#
.SYNOPSIS
    The ONE sanctioned way to launch the Unreal editor from the pipeline (2026-10-04 rule change).
    Census first (never a second GUI editor of this project), start UnrealEditor.exe detached on the
    .uproject, then wait until the built-in MCP server ANSWERS AN HTTP REQUEST on port 8000 (SHIP-9e:
    a listening port is not a live server). ASCII-only on purpose (PS 5.1 BOM-less decoding).

.DESCRIPTION
    Pairs with Tools\stop_editor.ps1. The build-master runs stop -> Build.bat -> launch around every
    C++ compile (CLAUDE.md rule 5a). The engine root is derived from the .uproject's EngineAssociation
    through the registry (HKLM\SOFTWARE\EpicGames\Unreal Engine\<ver>\InstalledDirectory) with the
    stock UE_5.8 path as the fallback, so the script survives a machine move.

.PARAMETER TimeoutSeconds
    How long to wait for MCP to answer. Default 300 (MCP answered 17 s after launch on the first measured
    run, TASK-1602; earlier boots took 45-140 s).
.PARAMETER Port
    The MCP server port. Default 8000 (Editor Preferences -> Model Context Protocol).
.PARAMETER WhatIf
    Print the census and the exact launch line; launch nothing.
.PARAMETER EditorExe
    Override the editor executable path.

.EXAMPLE
    powershell -NoProfile -File Tools\launch_editor.ps1
    powershell -NoProfile -File Tools\launch_editor.ps1 -WhatIf

.NOTES
    Exit codes: 0 the process is alive and MCP answered an HTTP request (prints PID and seconds; that is all
                the script tests - "MCP answers" is not "editor idle", SC-118 cl. 10) - 2 an EDITOR instance of
                this project is already running (PID printed; nothing launched) - 5 process alive but MCP did
                not answer in time - 6 the process exited before MCP answered (read Saved/Logs/<Project>.log)
                7 unexpected error, or the editor exe is not on disk (checked before -WhatIf returns)
    A -game instance alive at launch time is reported and never touched (SC-118).
#>
[CmdletBinding()]
param(
    [int] $TimeoutSeconds = 300,
    [int] $Port = 8000,
    [switch] $WhatIf,
    [string] $EditorExe = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ToolsDir = $PSScriptRoot
$ProjectRoot = (Resolve-Path (Join-Path $ToolsDir '..')).Path
$UProject = Get-ChildItem -LiteralPath $ProjectRoot -Filter '*.uproject' | Select-Object -First 1
if ($null -eq $UProject) { Write-Host "launch_editor: no .uproject under $ProjectRoot"; exit 7 }

function Resolve-EditorExe {
    if (-not [string]::IsNullOrEmpty($EditorExe)) { return $EditorExe }
    $assoc = ''
    try {
        $json = Get-Content -LiteralPath $UProject.FullName -Raw | ConvertFrom-Json
        $assoc = [string]$json.EngineAssociation
    } catch { $assoc = '' }
    if (-not [string]::IsNullOrEmpty($assoc)) {
        $key = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$assoc"
        if (Test-Path $key) {
            # A key without the value (or a $null return) must fall through to the stock path, not throw under StrictMode.
            $props = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue
            $dir = ''
            if ($null -ne $props -and $null -ne $props.PSObject.Properties['InstalledDirectory']) { $dir = [string]$props.InstalledDirectory }
            if (-not [string]::IsNullOrEmpty($dir)) {
                $cand = Join-Path $dir 'Engine\Binaries\Win64\UnrealEditor.exe'
                if (Test-Path -LiteralPath $cand) { return $cand }
            }
        }
    }
    return 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
}

function Get-EditorCensus {
    $rows = @()
    $procs = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" -ErrorAction SilentlyContinue
    foreach ($p in @($procs)) {
        $cmd = [string]$p.CommandLine
        $class = 'OTHER'
        if ($cmd -match '(^|\s)-game(\s|$)') { $class = 'PLAY' }
        elseif ($cmd -match '-run=|-AuraHeadless|-RenderOffScreen|(^|\s)-unattended(\s|$)') { $class = 'HEADLESS' }
        elseif ($cmd.ToLower().Contains($UProject.Name.ToLower())) { $class = 'EDITOR' }
        $rows += [pscustomobject]@{ Pid = [int]$p.ProcessId; Class = $class; CmdLine = $cmd }
    }
    return ,$rows
}

function Test-McpAnswers([int]$P) {
    # Any HTTP response (even 4xx/5xx) means the server is up; a refused connection means it is not.
    try {
        $null = Invoke-WebRequest -Uri ("http://127.0.0.1:{0}/mcp" -f $P) -Method Get -UseBasicParsing -TimeoutSec 5
        return $true
    } catch {
        # Only a WebException carries .Response; reading it on any other exception type throws under StrictMode.
        if (($_.Exception -is [System.Net.WebException]) -and ($null -ne $_.Exception.Response)) { return $true }
        return $false
    }
}

try {
    $exe = Resolve-EditorExe
    $census = Get-EditorCensus
    Write-Host ("launch_editor: census: {0} UnrealEditor.exe process(es)" -f @($census).Count)
    foreach ($r in @($census)) { Write-Host ("  PID {0,-6} {1,-8} cmd: {2}" -f $r.Pid, $r.Class, $r.CmdLine) }
    $running = @($census | Where-Object { $_.Class -eq 'EDITOR' })
    if ($running.Count -gt 0) {
        Write-Host ("launch_editor: an EDITOR instance of this project is already running (PID {0}); nothing launched" -f $running[0].Pid)
        exit 2
    }
    $launchLine = ('"{0}" "{1}"' -f $exe, $UProject.FullName)
    Write-Host "launch_editor: launch line: $launchLine"
    if (-not (Test-Path -LiteralPath $exe)) { Write-Host "launch_editor: editor exe not found: $exe"; exit 7 }
    if ($WhatIf) { Write-Host 'launch_editor: -WhatIf, nothing launched'; exit 0 }

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $proc = Start-Process -FilePath $exe -ArgumentList ('"{0}"' -f $UProject.FullName) -PassThru
    Write-Host ("launch_editor: started PID {0}" -f $proc.Id)
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        $alive = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
        $secs = [int][math]::Round($sw.Elapsed.TotalSeconds)   # culture-safe: no group separator at >= 1000 s
        if ($null -eq $alive) {
            Write-Host ("launch_editor: PID {0} exited after {1} s before MCP answered - read Saved\Logs" -f $proc.Id, $secs)
            exit 6
        }
        if (Test-McpAnswers $Port) {
            Write-Host ("launch_editor: MCP answered on port {0} after {1} s; editor PID {2}" -f $Port, $secs, $proc.Id)
            Write-Host ("LAUNCH_EDITOR: PID={0} SECONDS={1} MCP=http://127.0.0.1:{2}/mcp" -f $proc.Id, $secs, $Port)
            exit 0
        }
        Start-Sleep -Seconds 3
    }
    Write-Host ("launch_editor: PID {0} alive but MCP did not answer within {1} s (Auto Start Server on? port {2}?)" -f $proc.Id, $TimeoutSeconds, $Port)
    exit 5
}
catch {
    Write-Host ("launch_editor: unexpected error - {0}" -f $_.Exception.Message)
    exit 7
}
