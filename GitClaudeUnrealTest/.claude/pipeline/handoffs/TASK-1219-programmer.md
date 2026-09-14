# TASK-1219 — AURA-SYNC-SCRIPT — programmer handoff (2026-09-13)

marker `TASK-1219-AURA-SYNC-SCRIPT` · gate `TASK-1234` · host `TASK-1240` · law: plan item 4 + Corrections row 3 · `TL-§6` · `SC-§68`

Files written: `Tools/aura_sync.ps1` (NEW, 160 lines, ASCII) · this handoff. Board: only my row's `status:` flipped. No editor, no compile, no Git command other than `git status` / `git check-ignore` (read-only). No `Source/**`, `Content/**`, `Config/**`, `Docs/**`.

**Real-run side effect, intended:** the script was run once for real (sources existed by the time I finished — TASK-1217/1218 had landed). That overwrote Aura's own seeded template at `Saved/.Aura/INDEX_IGNORE.txt` (1500 B, sha256 `d2c5d9d1…6cf2`) with the committed list (3309 B) and created `Saved/.Aura/project_memory.txt` (3469 B). Overwriting Aura's template is the point of the row — `Docs/` is canonical, `Saved/` is regenerated. Nothing else in `Saved/.Aura/` was touched (`Skills/`, `project.json`, `CrashReportState.json`, `PythonRequirementsChecksum.txt` keep their 20:45–20:46 mtimes).

## 1. What the script does (`Tools/aura_sync.ps1`)

- `[CmdletBinding(SupportsShouldProcess)]`, `Set-StrictMode -Version Latest`, `$ErrorActionPreference = 'Stop'`, whole body in `try/catch` → exit 4 on anything unexpected.
- **Root resolution:** `$ProjectRoot = Resolve-Path (Join-Path $PSScriptRoot '..')`. Caller's cwd is never read (every test below was run with cwd = `%TEMP%`). No drive letter anywhere.
- **Pairs (pinned):** `Docs/AuraIndexIgnore.txt → Saved/.Aura/INDEX_IGNORE.txt` · `Docs/AuraProjectMemory.md → Saved/.Aura/project_memory.txt`.
- **Fail closed:** step 1 tests EVERY source before ANY write. Any missing → each printed as `MISSING SOURCE <path>`, a `Write-Error` naming all of them, `exit 2`. `Saved/.Aura/` is not even created in that branch (T3b).
- **Idempotent:** per pair, sha256 of source and (if present) destination; equal → `no change`, nothing written, mtime untouched (T2). Different → `Copy-Item -Force`, re-hash the destination, require equality (else `exit 3`).
- **Output per file:** action · source · dest · bytes · `sha256 : source` · `sha256 : dest`. Summary line `N copied, M unchanged, K mismatched`; when N=0 and M=2 an explicit `no change - every destination already matches its source.` line. Exit 0.
- **`-WhatIf`: SUPPORTED** (declared in the header). `New-Item` and `Copy-Item` are both behind `$PSCmdlet.ShouldProcess`.
- Hashing is done with `[System.Security.Cryptography.SHA256]` + `[IO.File]::ReadAllBytes`, NOT `Get-FileHash` — see §3.
- Exit codes documented in `.NOTES`: 0 ok · 2 missing source · 3 post-copy hash mismatch · 4 unexpected.

## 2. Executed acceptance (all under `%TEMP%\claude\…\scratchpad\`, PS 5.1.26100.9444, `powershell.exe -NoProfile -File …`, cwd deliberately `%TEMP%`)

Scratch layout = `aura_proj/{Tools/aura_sync.ps1 (copy), Docs/<2 scratch sources>, Saved/.Aura/INDEX_IGNORE.txt (copy of Aura's real template, to prove the overwrite)}`. Harness: `scratchpad/aura_sync_tests.ps1` (not in the repo).

| # | Spec item | Verdict | Evidence |
|---|---|---|---|
| 1 | run once → both present, byte-identical | ✅ | T1: exit 0, `2 copied`; independent re-hash in the harness: `INDEX_IGNORE.txt` src=dst=`32fbde9f…ca8c` EQUAL=True · `project_memory.txt` src=dst=`dbcb34f9…f986` EQUAL=True. Pre-state template `d2c5d9d1…6cf2` (1500 B) → overwritten. |
| 2 | run twice → "no change" | ✅ | T2: exit 0, `no change` ×2, `0 copied, 2 unchanged`, the explicit no-change line; dest mtime unchanged from T1. |
| 3 | delete one source → non-zero exit naming it, other dest untouched | ✅ | T3: deleted `Docs/AuraProjectMemory.md` AND appended `tampered` to the other dest first (so a wrong copy would be detectable). Exit **2**, `MISSING SOURCE …\Docs\AuraProjectMemory.md` printed + in the error. Other dest hash before/after `553280c6…01a1` = `553280c6…01a1` UNTOUCHED=True; `project_memory.txt` from T1 not deleted. T3b (both missing, no `Saved/`): exit 2, both named, `Saved/` NOT created. T3c (fresh layout, no `Saved/`): dir created, 2 copied, exit 0. |
| 4 | `git status --porcelain` shows nothing under `Saved/` | ✅ | After the real run: `git status --porcelain -- Saved => []` (empty). `git check-ignore -v` → `GitClaudeUnrealTest/.gitignore:111:Saved/` for both dests. (Whole-tree status at that instant listed only the concurrent rows' files + `Tools/aura_sync.ps1` `??` — nothing under `Saved/`.) |
| 5 | `-WhatIf` supported or declined | ✅ supported | T0 (template pre-seeded): two `What if: … Copy from …` lines, `0 copied, 0 unchanged`, exit 0, dest hash before = after `56da74eb…ec0e`, `project_memory.txt` still absent. T0b (no `Saved/`): `What if: Create directory` printed, `Saved/` NOT created, per-file rows show `would copy (WhatIf)` with `dest (absent)`. |
| 6 | no execution-policy change, no network, no `.claude/skills` reference | ✅ | `Select-String` over the script for the execution-policy cmdlet, `Invoke-WebRequest`, `Invoke-RestMethod`, `.claude/skills`, `&&`, `??` → 0 hits in code. (An earlier revision named two of the tokens inside the header comment; reworded so a QA grep does not false-positive.) |

## 3. The real run (sources present by the time I finished — quoted, not fabricated)

```
aura_sync: project root = C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest
aura_sync: copied   Docs\AuraIndexIgnore.txt  -> Saved\.Aura\INDEX_IGNORE.txt    bytes 3309
    sha256 : source 56da74eb754ccb2511bb6b2fd5aa5607c874fbf591495630c1332acb7f70ec0e
    sha256 : dest   56da74eb754ccb2511bb6b2fd5aa5607c874fbf591495630c1332acb7f70ec0e
aura_sync: copied   Docs\AuraProjectMemory.md -> Saved\.Aura\project_memory.txt  bytes 3469
    sha256 : source 2c195b8a6a63fd75310e16b2e20ce540c79611a2dbe2acb3a136bf831afece8b
    sha256 : dest   2c195b8a6a63fd75310e16b2e20ce540c79611a2dbe2acb3a136bf831afece8b
aura_sync: summary - 2 copied, 0 unchanged, 0 mismatched      EXIT 0
```
Runs #2 and #3: `0 copied, 2 unchanged`, exit 0. Real `-WhatIf`: same, nothing written. Independent `Get-FileHash` after: all four hashes as above. ⚠️ These hashes are the sources **at my instant (2026-09-13 ~21:12 local)**; TASK-1217/1218 were still parallel rows, so if either source is edited again before `TASK-1240`, the host re-runs the script and the hashes move — that is the script working, not a discrepancy.

## 4. What QA should scrutinize

- **The `-WhatIf` defect I hit and fixed (worth a read):** with `SupportsShouldProcess`, PS 5.1 leaks the script's WhatIf state into `Get-FileHash`'s internal provider calls (`What if: Performing "Retrieve the value for property 'ProviderPath'"`) and it returns an object with no `Hash` → exit 4. A local `$WhatIfPreference = $false` inside the helper did NOT stop it (measured, second failed run). The fix is .NET hashing, which has no ShouldProcess path. Both failures and the pass are in my transcript; the harness output is reproducible via the scratch script.
- `Write-Error … -ErrorAction Continue` under `$ErrorActionPreference='Stop'` is deliberate: it prints the named-source error to stderr without throwing into the `catch` (which would exit 4 instead of the documented 2). Verified: exit 2 in T3/T3b.
- `exit N` inside a `try` in a script file: PS 5.1 honours it (measured exit codes 0/2/4 across the runs).
- Harness helper was originally named `H` — that is PowerShell's alias for `Get-History` and the first run crashed on it; renamed to `Sha`. Harness-only, not in the shipped script.
- `TL-§6` note: the harness lives in the scratchpad, not `Tools/` — the row asked for executed evidence, not a tracked test runner. If `TASK-1234` wants it tracked, that is a board edit (`SC-§100`).
