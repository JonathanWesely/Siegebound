#Requires -Version 5.1
<#
.SYNOPSIS
    One session-start step for every hand-maintained mirror copy (2026-10-04 rule change):
      Docs\GameDevSetup.md                       -> <vault>\GameDevSetup.md
      Docs\Aura AI for Unreal - Integration Plan  -> <vault>\(same name, with the em dash)
      Docs\AuraIndexIgnore.txt + AuraProjectMemory.md -> Saved\.Aura\  (delegated to Tools\aura_sync.ps1)
    The repo copies are the source. Idempotent (SHA-256 compare), fails closed, ASCII-only.

.DESCRIPTION
    Two of these mirrors drifted when they were copied by hand. This script replaces the chore.
    The vault path comes from -VaultPath, else the JONWES_VAULT environment variable, else the
    machine default. A machine without the vault exits 2 and copies nothing.

.PARAMETER VaultPath
    The Obsidian vault folder. Default: $env:JONWES_VAULT, else C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault.
.PARAMETER WhatIf
    Report what would be copied; write nothing (also passed through to aura_sync.ps1).

.EXAMPLE
    powershell -NoProfile -File Tools\sync_mirrors.ps1
    powershell -NoProfile -File Tools\sync_mirrors.ps1 -WhatIf

.NOTES
    Exit codes: 0 every mirror matches its source (under -WhatIf: 0 regardless, with the would-copy count
                printed) - 2 vault missing, not an Obsidian vault (no <vault>\.obsidian folder), or a source
                missing - 3 a copy did not verify - 4 aura_sync.ps1 failed (its own exit code is printed;
                the child runs with ErrorActionPreference Continue so its stderr lines cannot abort this
                script under a capturing host) - 7 unexpected error
#>
[CmdletBinding()]
param(
    [string] $VaultPath = '',
    [switch] $WhatIf
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$ToolsDir = $PSScriptRoot
$ProjectRoot = (Resolve-Path (Join-Path $ToolsDir '..')).Path
if ([string]::IsNullOrEmpty($VaultPath)) { $VaultPath = $env:JONWES_VAULT }
if ([string]::IsNullOrEmpty($VaultPath)) { $VaultPath = 'C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault' }

$EmDash = [string][char]0x2014
$AuraPlanName = 'Aura AI for Unreal ' + $EmDash + ' Integration Plan.md'

$Pairs = @(
    @{ Source = Join-Path $ProjectRoot 'Docs\GameDevSetup.md'; Dest = Join-Path $VaultPath 'GameDevSetup.md' },
    @{ Source = Join-Path $ProjectRoot ('Docs\' + $AuraPlanName); Dest = Join-Path $VaultPath $AuraPlanName }
)

function Get-Sha256Hex([string]$Path) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try { return ([System.BitConverter]::ToString($sha.ComputeHash([System.IO.File]::ReadAllBytes($Path))) -replace '-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}

try {
    if (-not (Test-Path -LiteralPath $VaultPath -PathType Container)) {
        Write-Host "sync_mirrors: vault not found at $VaultPath (set JONWES_VAULT or pass -VaultPath); nothing copied"
        exit 2
    }
    if (-not (Test-Path -LiteralPath (Join-Path $VaultPath '.obsidian') -PathType Container)) {
        Write-Host "sync_mirrors: $VaultPath is not a vault (no .obsidian folder); nothing copied"
        exit 2
    }
    $missing = @()
    foreach ($p in $Pairs) { if (-not (Test-Path -LiteralPath $p.Source -PathType Leaf)) { $missing += $p.Source } }
    if ($missing.Count -gt 0) { foreach ($m in $missing) { Write-Host "sync_mirrors: MISSING SOURCE $m" }; exit 2 }

    $copied = 0; $unchanged = 0; $bad = 0; $would = 0
    foreach ($p in $Pairs) {
        $srcHash = Get-Sha256Hex $p.Source
        $dstHash = '(absent)'
        if (Test-Path -LiteralPath $p.Dest -PathType Leaf) { $dstHash = Get-Sha256Hex $p.Dest }
        if ($dstHash -eq $srcHash) { $unchanged++; Write-Host ("sync_mirrors: no change   {0}" -f $p.Dest); continue }
        if ($WhatIf) { $would++; Write-Host ("sync_mirrors: would copy  {0} -> {1}" -f $p.Source, $p.Dest); continue }
        Copy-Item -LiteralPath $p.Source -Destination $p.Dest -Force
        $after = Get-Sha256Hex $p.Dest
        if ($after -eq $srcHash) { $copied++; Write-Host ("sync_mirrors: copied      {0} -> {1}" -f $p.Source, $p.Dest) }
        else { $bad++; Write-Host ("sync_mirrors: HASH MISMATCH after copy {0}" -f $p.Dest) }
    }

    $auraArgs = @('-NoProfile', '-File', (Join-Path $ToolsDir 'aura_sync.ps1'))
    if ($WhatIf) { $auraArgs += '-WhatIf' }
    # Under 'Stop', a capturing host turns the child's Write-Error stderr lines into terminating NativeCommandError
    # records before $LASTEXITCODE can be read; run the native child under 'Continue' and restore afterwards.
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & powershell @auraArgs
    $auraExit = $LASTEXITCODE
    $ErrorActionPreference = $savedEap
    Write-Host ("sync_mirrors: aura_sync.ps1 exit {0}" -f $auraExit)

    Write-Host ("sync_mirrors: summary - {0} copied, {1} unchanged, {2} mismatched, {3} would-copy (-WhatIf)" -f $copied, $unchanged, $bad, $would)
    if ($bad -gt 0) { exit 3 }
    if ($auraExit -ne 0) { exit 4 }
    exit 0
}
catch {
    Write-Host ("sync_mirrors: unexpected error - {0}" -f $_.Exception.Message)
    exit 7
}
