# QA Report — TASK-1234 (gate over TASK-1219)
subject: TASK-1219 — `Tools/aura_sync.ps1` (AURA-SYNC-SCRIPT)
reviewer: qa-reviewer · 2026-09-13 · law: `SC-§71b` · `SC-§106` · `TL-§6`
Verdict: PASS

Blockers 0 · Warns 3 · Nits 4

## Provenance of this verdict (read first)
- Instruments used: `Read` on the script (189 lines), the handoff, `TASKBOARD.md` rows 1219/1234 + the section constraints, plan item 4 + the *Corrections* table, `.gitignore` :107-111, and the first lines of both sources and both live destinations. `Grep` over the script for forbidden tokens.
- Instruments NOT held: no `Bash`/PowerShell — I did **not run the script, compute any hash, or run `git status`**. Every hash and exit code below is **DECLARED by TASK-1219's handoff**, not measured by me. `TASK-1240` (host) owes the re-run per `SC-§106`/`SC-§71b`; where disk and the handoff disagree at that instant, disk wins.
- `unreal_inspector` not applicable (no engine artefact in this row).

## Acceptance — TASK-1219 row (1)–(6) and TASK-1234 row (1)–(5)

| # | Check | Result | Basis |
|---|---|---|---|
| 1234-1 | Source + dest paths equal `names:` verbatim | ✅ | `:68-69` `Docs\AuraIndexIgnore.txt` → `Saved\.Aura\INDEX_IGNORE.txt`, `Docs\AuraProjectMemory.md` → `Saved\.Aura\project_memory.txt` (backslash form of the row's four pinned names, case-exact) |
| 1234-2 | `$PSScriptRoot`-relative, no drive letter | ✅ | `:61-63` `$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path`; `Grep` for `[A-Z]:\\` in code → only the `.EXAMPLE` prose. cwd never read. `.Path` makes every path absolute, which is why the .NET `ReadAllBytes` (cwd-relative in .NET) is safe |
| 1234-3 | Missing-source exit BEFORE any copy | ✅ | `:97-109` tests every source, prints `MISSING SOURCE`, `Write-Error -ErrorAction Continue`, `exit 2` — before `New-Item` (`:116`) and before the copy loop (`:128`). `Saved/.Aura/` is not even created on that branch |
| 1234-4 | No `Set-ExecutionPolicy`, no network, no `.claude/skills` | ✅ | `Grep` `Set-ExecutionPolicy\|Invoke-WebRequest\|Invoke-RestMethod\|\.claude/skills\|System\.Net\|WebClient\|Start-Process\|Invoke-Expression\|iex` → 0 hits. No env reads, no secrets, no argv |
| 1234-5 | Six EXECUTED verdicts quoted with strings | ✅ (declared) | Handoff §2 table: six rows, each with exit code + hash string(s); §3 real run quotes both 64-hex pairs and byte counts. Labelled "EXECUTED"/"quoted, not fabricated". See WARN-2 for one label inconsistency |
| 1219-1 | run once → both present, byte-identical | ✅ declared | §3: `56da74eb…ec0e` src=dst (3309 B), `2c195b8a…ece8b` src=dst (3469 B). My `Read` of the first 8/5 lines of each live dest matches its source — text-level only |
| 1219-2 | run twice → "no change" | ✅ declared + code | `:141-143` equal hash → `no change`, no write, mtime untouched; `:180-182` summary line. Handoff T2 + real runs #2/#3 |
| 1219-3 | delete one source → non-zero naming it, other dest untouched | ✅ declared + code | Code per 1234-3. Handoff T3: exit 2, tampered other-dest hash `553280c6…01a1` before = after; T3b both missing, `Saved/` not created |
| 1219-4 | `git status --porcelain` nothing under `Saved/` | ✅ declared | `[]` quoted; `check-ignore` → `.gitignore:111:Saved/` — I confirmed `.gitignore:111` is `Saved/` by `Read` |
| 1219-5 | `-WhatIf` supported or declined | ✅ supported | `:52` `SupportsShouldProcess`; `:115` and `:146` both writers behind `$PSCmdlet.ShouldProcess`; header `:32-33` declares it |
| 1219-6 | no exec-policy / network / skills | ✅ | as 1234-4 |

## Tooling checklist (Tools/** = CODE)
- **Idempotent:** ✅ SHA-256 compare before copy (`:132-143`); equal → no write.
- **Write confinement:** ✅ the only writers are `New-Item` (`:116`, target `$DestDir`) and `Copy-Item` (`:147`, target `$Dst`); both derive from `$ProjectRoot + 'Saved\.Aura'`. No `Out-File`/`Set-Content`/`Remove-Item`. Nothing under `Docs/`, `Source/`, `Content/`, `Config/`. Paths carry no user input, so no escape vector.
- **PS 5.1 compatibility:** ✅ `Grep` `&&|\|\||\?\?|-AsHashtable` → 0 hits; no ternary; `-f` formatting, `-replace`, `[CmdletBinding(SupportsShouldProcess = $true)]` all 5.1-valid. `New-Item` correctly uses `-Path` (5.1 has no `-LiteralPath` on it); `Test-Path`/`Copy-Item`/`Get-Item` use `-LiteralPath`.
- **Secrets / network / policy:** ✅ none.
- **Hashing replacement (`Get-Sha256Hex` :72-88):** ✅ `[System.Security.Cryptography.SHA256]::Create()` — the abstract factory (not `SHA256Managed`), so it also works under a FIPS-enforced policy; `try/finally { $Sha.Dispose() }` disposes on every path including the `return`; `ReadAllBytes` is acceptable at ≤ a few KB (sources are 3309 B / 3469 B); lowercase hex via `BitConverter`. No ShouldProcess path exists in .NET, so `-WhatIf` cannot poison it. Under `-WhatIf`: `Test-Path`/`Get-Item`/the hasher are read-only, both writers are guarded, per-file rows print `would copy (WhatIf)` with `dest (absent)` — declared executed as T0/T0b and the real `-WhatIf`.
- **Exit codes:** `exit N` inside `try` is not caught by `catch` (PowerShell `exit` is not an exception); `Write-Error -ErrorAction Continue` under `$ErrorActionPreference='Stop'` is the correct way to emit a named non-terminating error and still reach the documented exit — declared measured (exit 0/2/4).

## Findings
- [WARN] `Tools/aura_sync.ps1:128-148` — fail-closed covers a MISSING source only (`Test-Path -PathType Leaf`). A source that exists but cannot be READ (locked/ACL) on the SECOND pair throws inside the loop AFTER the first pair has already been copied → exit 4 with a partial set. Exotic for two committed text files, but the spec sentence is "never writes a partial set". Suggested fix (cheap, ≤ KB files): hash every source in step 1 (right after the existence check) into `$Pair.SrcHash`, so any read failure also exits before the first `Copy-Item`. Not a blocker: the acceptance is written against absence, and absence is handled correctly.
- [WARN] `handoffs/TASK-1219-programmer.md:31` (evidence labelling) — T0 is captioned "template pre-seeded" but quotes dest-before = dest-after = `56da74eb…ec0e`, which is the hash of the COMMITTED `Docs/AuraIndexIgnore.txt` (§3), while "Aura's real template" is `d2c5d9d1…6cf2` (1500 B) at :7 and :27. The two "What if: … Copy" lines are consistent with a dest that differs from the SCRATCH sources (`32fbde9f…`), so the WhatIf-writes-nothing measurement itself stands; only the word "template" is wrong (or the hash is misquoted). Host re-run of `-WhatIf` (`TASK-1240`) settles which — either way the script's behaviour is unaffected.
- [WARN] evidence currency — the §3 hash pairs are the sources at the programmer's instant (~21:12). `TASK-1217` is still `ready-for-qa` (`TASK-1232` open); if its gate moves `Docs/AuraIndexIgnore.txt`, the live `Saved/.Aura/INDEX_IGNORE.txt` goes stale until the host re-runs. The handoff says this itself (:46). `TASK-1240` must run the script AFTER `TASK-1232` closes and quote fresh pairs (`SC-§106`: last declared hash, disk wins).
- [NIT] `handoffs/TASK-1219-programmer.md:5` says "160 lines"; the file is 189 lines. Cosmetic.
- [NIT] `Tools/aura_sync.ps1:61-63` — root resolution sits OUTSIDE the `try`; if the body is ever pasted into an interactive console (`$PSScriptRoot` empty) `Join-Path ''` throws an uncaught error → exit 1 instead of the documented 4. Documented invocation is `-File`, where `$PSScriptRoot` is always set. Optional: move `:61-70` inside the `try`.
- [NIT] `-Confirm` (declared honoured, `:33`): answering No to "Create directory" and Yes to the copy makes `Copy-Item` fail on the absent parent → exit 4 rather than a clean message; and `Copy-Item` itself will prompt a second time under `-Confirm` (script scope lowers `$ConfirmPreference`). Harmless; `-WhatIf` is the lane that matters and it is clean.
- [NIT] on a post-copy hash mismatch (`:153-156`) the loop continues to the next pair before `exit 3`. Acceptable — exit is still non-zero and the summary counts it; noting only so nobody reads "mismatch" as "stopped".

## Not measured by me (host owes, TASK-1240)
1. Run `powershell -NoProfile -File Tools\aura_sync.ps1` from a cwd OUTSIDE the repo (e.g. `%TEMP%`) AFTER `TASK-1232` lands; quote both `sha256 : source` / `sha256 : dest` pairs and the exit code; run again and quote `0 copied, 2 unchanged`.
2. `-WhatIf` once with the live tree: expect two `no change` rows (or `would copy` if a source moved), exit 0, and unchanged dest mtimes.
3. `git status --porcelain -- Saved` → empty, quoted.
4. Independent `Get-FileHash -Algorithm SHA256` on the four files — the script's own hasher is not its own witness.

## Notes for build-master (PASS)
- Script is text-level clean for PS 5.1; the .NET hasher is the correct fix for the `SupportsShouldProcess` + `Get-FileHash` leak and is properly disposed.
- Nothing under `Saved/` is ever staged (STANDING EXCLUSION REGISTRY); commit only `Tools/aura_sync.ps1` + the handoff under `TASK-1240`.
- The two `Docs/` sources are other rows' artefacts (`TASK-1217`/`1218`) — their gates decide their content, not this report.
