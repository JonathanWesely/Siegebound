<#
.SYNOPSIS
    Copies the two canonical, committed Aura files into the gitignored Saved/.Aura/
    folder. Idempotent, fails closed, project-root-relative. (TASK-1219)

.DESCRIPTION
    Saved/ is gitignored (.gitignore line 111), so the files Aura reads from
    Saved/.Aura/ cannot be the versioned copies. The versioned copies live in Docs/
    and this script is the ONE sanctioned step that regenerates the live ones:

        Docs/AuraIndexIgnore.txt   -> Saved/.Aura/INDEX_IGNORE.txt
        Docs/AuraProjectMemory.md  -> Saved/.Aura/project_memory.txt

    Every path is resolved from this script's own location ($PSScriptRoot). The
    project root is the parent of Tools/. The caller's working directory is never
    consulted and no drive letter is hard-coded.

    Behaviour, in order:
      1. Resolve both source paths. If ANY source is missing, print the missing
         path(s), write NOTHING, exit 2. (Fail closed - never a partial set.)
      2. Create Saved/.Aura/ if it is absent.
      3. Per pair: hash source and (if present) destination with SHA-256. Equal
         hashes -> "no change", nothing written. Otherwise copy, re-hash the
         destination and require it to equal the source (exit 3 if it does not).
      4. Print, per file: source, destination, byte count, both hashes, action.
         Exit 0 when every destination matches its source.

    Aura itself seeds Saved/.Aura/INDEX_IGNORE.txt with a generic template on
    first index. Overwriting that template with the committed list is the intended
    behaviour of this script - the Docs/ copy is canonical.

    -WhatIf IS SUPPORTED (SupportsShouldProcess): it reports what would be created
    or copied and writes nothing. -Confirm is honoured for the same operations.

    This script never changes the execution policy, touches no network, and
    mirrors nothing else - the integration plan's skills-mirror step was dropped
    because there is nothing to mirror (plan "Corrections to the doc", row 3).

    Windows PowerShell 5.1 compatible: no pipeline-chain operators, no ternary,
    no null-coalescing.

.EXAMPLE
    powershell -NoProfile -File Tools\aura_sync.ps1
    powershell -NoProfile -File Tools\aura_sync.ps1 -WhatIf

.NOTES
    Exit codes:  0 = every destination matches its source (copied or unchanged)
                 2 = a source file is missing (named on stderr; nothing written)
                 3 = a copy was made but the destination hash != source hash
                 4 = unexpected error (message on stderr)
#>
[CmdletBinding(SupportsShouldProcess = $true)]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Paths - everything hangs off the script's own location, never the caller's cwd.
# ---------------------------------------------------------------------------
$ToolsDir    = $PSScriptRoot
$ProjectRoot = (Resolve-Path (Join-Path $ToolsDir '..')).Path
$DestDir     = Join-Path $ProjectRoot 'Saved\.Aura'

# Source (canonical, committed) -> destination (live, gitignored). Pinned by the
# TASKBOARD row; do not rename either side without a board edit.
$Pairs = @(
    @{ Source = Join-Path $ProjectRoot 'Docs\AuraIndexIgnore.txt';  Dest = Join-Path $DestDir 'INDEX_IGNORE.txt' },
    @{ Source = Join-Path $ProjectRoot 'Docs\AuraProjectMemory.md'; Dest = Join-Path $DestDir 'project_memory.txt' }
)

function Get-Sha256Hex {
    param([string]$Path)
    # Hashing is read-only, so it is done with .NET directly rather than
    # Get-FileHash: under -WhatIf, PS 5.1 leaks the script's WhatIf state into
    # Get-FileHash's internal provider calls and it returns no Hash at all
    # (measured: "The property 'Hash' cannot be found on this object"; a local
    # $WhatIfPreference = $false did NOT stop it). .NET has no ShouldProcess path.
    $Sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $Bytes = [System.IO.File]::ReadAllBytes($Path)
        $Hash  = $Sha.ComputeHash($Bytes)
        return ([System.BitConverter]::ToString($Hash) -replace '-', '').ToLowerInvariant()
    }
    finally {
        $Sha.Dispose()
    }
}

try {
    Write-Host "aura_sync: project root = $ProjectRoot"
    Write-Host "aura_sync: destination  = $DestDir"

    # -----------------------------------------------------------------------
    # 1. Fail closed: check EVERY source before writing ANY destination.
    # -----------------------------------------------------------------------
    $Missing = @()
    foreach ($Pair in $Pairs) {
        if (-not (Test-Path -LiteralPath $Pair.Source -PathType Leaf)) {
            $Missing += $Pair.Source
        }
    }
    if ($Missing.Count -gt 0) {
        foreach ($m in $Missing) {
            Write-Host "aura_sync: MISSING SOURCE $m" -ForegroundColor Red
        }
        Write-Error -Message ("aura_sync: {0} source file(s) missing; nothing written. Missing: {1}" -f $Missing.Count, ($Missing -join '; ')) -ErrorAction Continue
        exit 2
    }

    # -----------------------------------------------------------------------
    # 2. Destination folder.
    # -----------------------------------------------------------------------
    if (-not (Test-Path -LiteralPath $DestDir -PathType Container)) {
        if ($PSCmdlet.ShouldProcess($DestDir, 'Create directory')) {
            New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
            Write-Host "aura_sync: created $DestDir"
        }
    }

    # -----------------------------------------------------------------------
    # 3. Per pair: compare, copy only on difference, verify after copy.
    # -----------------------------------------------------------------------
    $Copied    = 0
    $Unchanged = 0
    $Mismatch  = 0

    foreach ($Pair in $Pairs) {
        $Src = $Pair.Source
        $Dst = $Pair.Dest

        $SrcHash  = Get-Sha256Hex -Path $Src
        $SrcBytes = (Get-Item -LiteralPath $Src).Length

        $DstHash = '(absent)'
        if (Test-Path -LiteralPath $Dst -PathType Leaf) {
            $DstHash = Get-Sha256Hex -Path $Dst
        }

        $Action = ''
        if ($DstHash -eq $SrcHash) {
            $Action = 'no change'
            $Unchanged++
        }
        else {
            if ($PSCmdlet.ShouldProcess($Dst, "Copy from $Src")) {
                Copy-Item -LiteralPath $Src -Destination $Dst -Force
                $DstHash = Get-Sha256Hex -Path $Dst
                if ($DstHash -eq $SrcHash) {
                    $Action = 'copied'
                    $Copied++
                }
                else {
                    $Action = 'COPIED BUT HASH MISMATCH'
                    $Mismatch++
                }
            }
            else {
                # -WhatIf: ShouldProcess already printed "What if: ..."
                $Action = 'would copy (WhatIf)'
            }
        }

        Write-Host ("aura_sync: {0}" -f $Action)
        Write-Host ("    source : {0}" -f $Src)
        Write-Host ("    dest   : {0}" -f $Dst)
        Write-Host ("    bytes  : {0}" -f $SrcBytes)
        Write-Host ("    sha256 : source {0}" -f $SrcHash)
        Write-Host ("    sha256 : dest   {0}" -f $DstHash)
    }

    # -----------------------------------------------------------------------
    # 4. Summary + exit.
    # -----------------------------------------------------------------------
    Write-Host ("aura_sync: summary - {0} copied, {1} unchanged, {2} mismatched" -f $Copied, $Unchanged, $Mismatch)
    if ($Mismatch -gt 0) {
        Write-Error -Message 'aura_sync: a destination did not match its source after copy.' -ErrorAction Continue
        exit 3
    }
    if (($Copied -eq 0) -and ($Unchanged -eq $Pairs.Count)) {
        Write-Host 'aura_sync: no change - every destination already matches its source.'
    }
    exit 0
}
catch {
    Write-Error -Message ("aura_sync: unexpected error - {0}" -f $_.Exception.Message) -ErrorAction Continue
    exit 4
}
