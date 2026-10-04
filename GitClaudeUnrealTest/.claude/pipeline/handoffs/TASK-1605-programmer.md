# Handoff — TASK-1605 — [RULE-CHANGE-TOOLING-FIX] the five tools fixed per `qa/TASK-1604.md`

Author: gameplay-programmer · 2026-10-04 · marker `TASK-1605-RULE-CHANGE-TOOLING-FIX` · law: `SC-§143` cl. 3/4 (CONVENTIONS.md core), `SC-§39` (`law/WR.md`: a check that cannot fail proves nothing), `SC-§118` cl. 10 (`law/TL.md`), `TL-§6` (`law/TL.md`: Tools/**/*.ps1 and *.py are CODE).
Gate: `TASK-1606`. Status flipped by this handoff: `TASK-1605` → `ready-for-qa` (the row's `status:` line only).

Posture: every claim below was MEASURED in this lane (shell held). Fences kept: `--apply` / `--rows` / `split --apply` were never run against the live tree; the one live write is the owed `--reindex` on `CONVENTIONS.md`, byte-identical (§5); `stop_editor.ps1` and `launch_editor.ps1` were never invoked in any mode (editor PID 30480 is not mine); `sync_mirrors.ps1` ran only with `-WhatIf`; no git; `.claude/pipeline/archive/` and `law/` untouched (103 archive files before and after; `law/` mtimes unchanged).

## 1. Files touched (final state)

| File | sha256 (first 16) | lines | CRLF | non-ASCII lines (`rg '[^\x00-\x7F]'`, direct path) |
|---|---|---|---|---|
| `Tools/archive_board.py` | `9fa9cf5f3e592b8a` | 512 | 512/512 | 6 (positive control for the class) |
| `Tools/split_conventions.py` | `6cbd152ea1572095` | 358 | 358/358 | 20 (positive control for the class) |
| `Tools/launch_editor.ps1` | `941df2a8d936b493` | 141 | 141/141 | **0** |
| `Tools/sync_mirrors.ps1` | `b183bba5e2f2de36` | 104 | 104/104 | **0** |
| `Tools/stop_editor.ps1` | `6e3d92f39c2b1cee` | 189 | 189/189 | **0** — **NOT edited** (sha equals the `TASK-1604` review copy; its three NITs are declared, §2) |

All five were CRLF before and are CRLF after (CR count == line count). Both `.py` parse under `C:\Python314\python.exe -W error` (no SyntaxWarning; docstrings are raw so `C:\Python314\python.exe` can appear in them). `ROOT = Path(__file__).resolve().parents[1]` is unchanged in both `.py`; no `--root` flag; no new write target outside `.claude/pipeline/`.

## 2. Finding-by-finding disposition (`qa/TASK-1604.md` — the `## Notes for the manager` numbers, then each entry)

| # | Severity | Finding (file:line in the REVIEWED copy) | Disposition | Where (file:line in the FIXED copy) |
|---|---|---|---|---|
| 1 | BLOCKER | `archive_board.py:168-170, 183-186, 229, 207-239` — not re-run safe (numbers restart, `write_bytes` truncates, INDEX rebuilt from this run only, second `## Archive`) | **FIXED** (B1 a–e) | numbering continues per prefix: `next_numbers()` `:169-186` (register = `archive/INDEX.md`, disk when absent — see §3.1 for why); refuse-overwrite before any write: `:429-433`; INDEX by scan: `scan_archive()` `:232-241`, `index_entry()` `:217-229`, `index_text()` `:244-257`, built in memory `:437-438`, rebuilt from the disk scan and compared `:489-493`, written `:495`; `## Archive` refreshed in place: `refresh_archive_section()` `:278-289`, `:444-447`; created only when absent: `archive_section_lines()` `:260-275`, `:448-459`; count asserted == 1: `count_archive_sections()` `:321`, `:473-475`; header comment now names the source section: `archive_header()` `:189-194` |
| 2 | BLOCKER | `split_conventions.py:211-215` (+ `:138-148`) — tautological exit-2 check; `--reindex` unchecked | **FIXED** (B2 a–e) | checker functions `:185-228` (`first_diff`, `check_file_equals`, `check_file_appended`, `check_core_nonindex`, `check_core_index`); in-memory checks before any write `:297-305` (lossless split, partition, NNN destination clash); law files read back right after each write `:316-325` with rollback `:345-355`; core written LAST `:333`, read back `:334-338` (non-index bytes == original minus moved minus old index; index section == generated), restore on failure `:337`; `--reindex` `:231-251`: non-index identity before/after + index identity, exit 2 and restore on a difference `:244-248`; every failure message names the file and the first differing byte offset |
| 3 | WARN | `split_conventions.py:36-44` — `KEEP_PREFIXES` lacks the `SC-§143` section; `nn` restarts at 0 | **FIXED** (W1) | `"⚖️ LAW MAINTENANCE"` at `:53`; `next_nn()` `:89-97` seeds from the highest `NNN-` in `law/` (`:277` uses it); docstring `:13-16` |
| 4 | WARN | `archive_board.py:157-162, 206, 218-219` — `--apply` without `--rows` with empty pass 1 → `min([])` traceback | **FIXED** (B1 f) | `:393-394` prints `pass 1 found nothing; pass --rows for pass 2`, returns 1, before any write; `EXIT CODES` `:51-52` documents it |
| 5 | WARN | `archive_board.py:242-256` — count-only check, substring id test, runs after the writes | **FIXED** (B1 e) | line-multiset conservation `check_conservation()` `:292-302`, called in memory `:470-472`; `check_readback()` `:305-318` compares sha256 AND bytes of the moved text with the archive file READ BACK (header stripped) and matches ids at heading level via `TASK_RE` (`ids_in()` `:133-144`), called `:484-487`; board written LAST `:502`, read back and sha-compared `:503-505` |
| 6 | WARN | `sync_mirrors.ps1:35, 80-86` — exit 4 honest only from a console host | **FIXED as specified** (see §3.4: the old shape did NOT reproduce in my harness, so this is defensive) | `:87-93`: `$savedEap` saved, `Continue` set, `& powershell @auraArgs`, `$LASTEXITCODE` read immediately, EAP restored; header `:27-29` |
| 7 | WARN | `launch_editor.ps1:60` — StrictMode trap on the registry read defeats the stock-path fallback | **FIXED** (measured: old shape THREW `PropertyNotFoundException`, new shape falls through) | `:63-66`: `$props = Get-ItemProperty …`; `$dir=''`; `if ($null -ne $props -and $null -ne $props.PSObject.Properties['InstalledDirectory'])` |
| 8 | WARN | `sync_mirrors.ps1:57` — destination guard is `-PathType Container` only | **FIXED** (measured: a folder without `.obsidian` → exit 2) | `:64-67` requires `<vault>\.obsidian` as a container, else `exit 2` "is not a vault (no .obsidian folder); nothing copied"; header `:26` |
| 9 | NIT ×11 | as listed | see below | |
| NIT | launch `:16, 29` header wording | **FIXED** | `:16-17` "MCP answered 17 s after launch on the first measured run, TASK-1602; earlier boots took 45-140 s" (ASCII hyphen, not the row's en dash — the ASCII law wins); `:30-34` exit 0 = "the process is alive and MCP answered an HTTP request … 'MCP answers' is not 'editor idle', SC-118 cl. 10" |
| NIT | launch `:90` WebException guard | **TAKEN** (declared-optional; one line in a file already open; measured old shape THREW) | `:96-97` `($_.Exception -is [System.Net.WebException]) -and ($null -ne $_.Exception.Response)` |
| NIT | launch `:117, 121, 122` `{1:n0}` culture formatting | **FIXED** (measured: `'{0:n0}' -f 1034.6` → `1,035` in en-US) | `:123` `$secs = [int][math]::Round($sw.Elapsed.TotalSeconds)`; `{1}` at `:125, :129, :130` |
| NIT | launch `:107-108` `-WhatIf` before the exe check; code 7 shared | **FIXED** (order) / **documented** (code) | `:114` `Test-Path` now precedes `:115` `-WhatIf`; `:34` documents "7 unexpected error, or the editor exe is not on disk (checked before -WhatIf returns)" — the code is kept at 7 (callers branch 0 vs non-0; a new code would change the contract) |
| NIT | stop `:81-85` `$null` CommandLine → OTHER | **declared NIT, not fixed** (file not opened; safe direction) | — |
| NIT | stop `:84-85, 139` HEADLESS kill of another project's commandlet | **declared NIT, not fixed** | — |
| NIT | stop `:170-174` exit 6 compares sha only | **declared NIT, not fixed** | — |
| NIT | archive `:182` dead statement | **gone** — the rows loop was rewritten (`:405-426`); no dead statement remains | |
| NIT | archive `:120-128` `seen_task_section` redundancy | **declared NIT, not fixed** — pass-1 logic kept byte-for-byte in behaviour (`:364-376`) so the move set is unchanged | |
| NIT | archive `:47` emoji-prefixed task headings are not rows — document | **FIXED** (documented) | docstring `:20-23` |
| NIT | archive `:53-56` `normalise_status` keeps struck words | **declared NIT, not fixed** (⛔ behaviour; conservative direction; left as the row says) | `:81-84` unchanged |
| NIT | `python` floor in both USAGE lines | **TAKEN** (one line each) | `archive_board.py:46`, `split_conventions.py:33`: "Python 3.7+; allow-listed interpreter: C:\Python314\python.exe" (no walrus or other 3.8+ construct is used) |
| NIT | sync `:71, 84` `-WhatIf` tally | **TAKEN** (declared-optional; three lines in a file already open) | `:72` `$would = 0`, `:78` `$would++`, `:96` summary prints `{3} would-copy (-WhatIf)`; header `:25-26` "under -WhatIf: 0 regardless, with the would-copy count printed" |

No finding silently dropped: 2 BLOCKER fixed, 6 WARN fixed, 11 NIT = 6 fixed/taken + 1 gone by rewrite + 4 declared (three in `stop_editor.ps1`, one `seen_task_section`, plus `normalise_status` = 5 declared; the count line: 11 NITs = launch 4 fixed/taken · stop 3 declared · archive `:182` gone, `:47` documented, `:120-128` declared, `:53-56` declared · floor taken · sync tally taken).

## 3. Design notes QA should read before the code

### 3.1 The number register is `archive/INDEX.md`, not the raw directory listing (B1 a vs B1 b)
My first implementation seeded NNN from the highest number ON DISK (the report's literal suggestion). On the rig that made the refuse-overwrite check **unreachable**: pre-creating the planned `TASKBOARD-rows-089-…` simply bumped the next number to `090` and the run went through (quoted in §4.0 below) — the exact "check that cannot fail" shape `SC-§39` forbids, and the row's (ii) proof could never have fired. The shipped design: `next_numbers()` reads the filenames `INDEX.md` lists (the register of what the tool itself wrote, regenerated from the disk scan on every successful run, so it is in sync with the disk after every run; `INDEX.md` absent ⇒ the highest on disk). A file at a computed name that nonetheless exists on disk is therefore one the register does not know (a crashed run, a hand-copied file) and is refused before any write. (B1 a) holds in substance — a run never WRITES a number that is on disk — and (B1 b) is reachable, which (ii) proves. The INDEX prose and the board's `## Archive` text say exactly this ("file numbers continue from the highest listed here and a file on disk is never overwritten").

### 3.2 `## Archive` is refreshed in place, not replaced
`Archive` stays in `KEEP_HEADINGS` (so passes 1/2 skip it); `refresh_archive_section()` rewrites only the `Last run …` line (adds one if missing), and the canonical section is inserted after `## Task template` only when the board has none (run G). The `Last run` line now reads `Last run <date>: the archive holds N files, M rows (TASK-a..TASK-b); this run added k file(s).` — totals over the whole archive (from the scan), not this run's. The multiset check accounts for the one removed line and the added lines exactly.

### 3.3 INDEX "From section" for the 86 legacy rows files
A rows file holds no section heading. New files carry it in their header comment (`archive_header()`: `… section "<heading>" on <date> by Tools/archive_board.py …`); for the 86 pre-TASK-1605 rows files the scan falls back to the previous `INDEX.md` row (`legacy_index_sections()`), which is stable run to run. Run A's INDEX reproduced the 103 legacy rows byte-identically (§4.1).

### 3.4 WARN 6 did not reproduce — the fix is in, and defensive
Harness (§6.2): a native child `powershell -File child.ps1` that does `Write-Error … -ErrorAction Continue; exit 4`, called from a parent under `Set-StrictMode -Version Latest` + `$ErrorActionPreference='Stop'` + `try/catch`, with the parent's own stderr captured (`2>&1 | Out-String` from the agent's PowerShell host). OLD shape: `LASTEXITCODE read = 4`, exit 4 — the catch was NOT hit. NEW shape: identical. So the "NativeCommandError → catch → 7" path did not occur under this capturing host (PowerShell 5.1 only wraps native stderr into records when it is itself reading that stream, which the un-captured `& powershell` call in `sync_mirrors.ps1` does not ask it to do). The row mandates the fix regardless; it is implemented as specified and cannot make anything worse (`Continue` makes any such record non-terminating). The header sentence I added is true under either behaviour.

### 3.5 Rig relocation (the one deviation from the row's letter)
The session scratchpad path is 170 chars; `<scratchpad>/t1605/.claude/pipeline/archive/` is 184 and the longest archive filename is 82 ⇒ 267 chars > Windows `MAX_PATH` (260) for Python's CRT — `FileNotFoundError` on a file `ls` can see. First attempt crashed in the in-memory phase (nothing written; scratch board sha unchanged). The rig was rebuilt at `C:\Users\wesel\AppData\Local\Temp\t1605` (archive path 64 chars; sub-rig `t1605b` for run G) — still a temp location, never the live tree; the layout alone (`ROOT` from `__file__`) redirects every write. The live archive path is ~170 chars and unaffected. Both rigs are left on disk for QA.

### 3.6 Two dated `_Archive note_` lines under one heading after a second pass-2 run
Run C added a second note under RULE-CHANGE TOOLING (`:` "archive notes under RULE-CHANGE TOOLING now: 2"). Each note names its own file and ids; `SC-§143` cl. 3's "one italic note" is per run. Not merged; flagged for the manager if one note per section is wanted.

## 4. Scratch-tree runs — quoted input and output (rig `C:\Users\wesel\AppData\Local\Temp\t1605`; tools sha-identical to live: `archive_board.py 9fa9cf5f3e592b8a`, `split_conventions.py 6cbd152ea1572095`; board copy `a13c757bbb4d1734` == live at copy time; 103 archive files, 56 law files, `CONVENTIONS.md 5f0d95de…`)

### 4.0 The first design's failure (disk-max numbering) — why §3.1 exists
```
== (ii) CONTROLLED NEGATIVE: pre-create the planned rows target TASKBOARD-rows-089-RULE-CHANGE-TOOLING-TASK-1604-TASK-1605-TASK-1606-The-QA-gat.md
$ python Tools/archive_board.py --apply --rows   (rig, controlled negative)
archive dir: next numbers TASKBOARD-018 / TASKBOARD-rows-090
  ok: TASKBOARD-018-THE-BOT-SWITCH-...  sha256(moved) == sha256(read back) == b2772bfdd97e895a  rows=1
  ok: TASKBOARD-rows-090-RULE-CHANGE-TOOLING-...  sha256(moved) == sha256(read back) == 7de6e0cce6c3b887  rows=1
applied: 2 new archive file(s); INDEX lists 108 files; board now 21785 lines ...
exit=0
.claude/pipeline/TASKBOARD.md: FAILED   .claude/pipeline/archive/INDEX.md: FAILED   (shas changed = the refusal never fired)
```

### 4.1 (i) plan → `--apply --rows` (A) → `--apply --rows` again (B) — final design
```
$ python Tools/archive_board.py   (plan)
board: 21891 lines, 104 sections
archive dir: next numbers TASKBOARD-018 / TASKBOARD-rows-087 (register: INDEX.md)
pass 1 (sections): 0 sections, 0 lines, 0 rows
pass 2 (rows):     2 sections touched, 4 terminal rows, 66 lines
  rows   : TASKBOARD-rows-087-THE-BOT-SWITCH-TASK-1600-TASK-1601-TASK-1602-TASK-1603-A-dev.md  move=3 keep-live=1 TASK-1601..TASK-1603
  rows   : TASKBOARD-rows-088-RULE-CHANGE-TOOLING-TASK-1604-TASK-1605-TASK-1606-The-QA-gat.md  move=1 keep-live=2 TASK-1604..TASK-1604
(dry run; --apply runs pass 1, --apply --rows runs both)      exit=0

$ python Tools/archive_board.py --apply --rows   (RUN A)
  ok: TASKBOARD-rows-087-...  sha256(moved) == sha256(read back) == 0584c487cb193217  rows=3
  ok: TASKBOARD-rows-088-...  sha256(moved) == sha256(read back) == 47f1be65a928992f  rows=1
applied: 2 new archive file(s); INDEX lists 105 files; board now 21829 lines (5999781 bytes); sha256 4ff04f9b11bf6ec6      exit=0
=== after A: files=105 INDEX rows=105 '## Archive' count=1 board sha=4ff04f9b11bf6ec6 lines=21829
196:Last run 2026-10-04: the archive holds 105 files, 908 rows (TASK-0012..TASK-1604); this run added 2 file(s).
--- INDEX: 103 legacy rows vs pre-run INDEX:  byte-identical
header of rows-087: <!-- ARCHIVED from .claude/pipeline/TASKBOARD.md section "🎮 THE BOT SWITCH — **`TASK-1600` · ... " on 2026-10-04 by Tools/archive_board.py. ...

$ python Tools/archive_board.py --apply --rows   (RUN B: board unchanged)
archive dir: next numbers TASKBOARD-018 / TASKBOARD-rows-089 (register: INDEX.md)
pass 2 (rows):     0 sections touched, 0 terminal rows, 0 lines
nothing archivable      exit=1
=== after B: board sha=4ff04f9b11bf6ec6 INDEX sha=e62364302e69118b files=105 — all 105 run-A files unchanged (sha256sum -c)
```
Then two rig-only status flips (`TASK-1600` → `done` so THE BOT SWITCH is all-terminal → pass 1; `TASK-1605` → `done` → pass 2), plan: `section: TASKBOARD-018-THE-BOT-SWITCH-…  rows=1` · `rows   : TASKBOARD-rows-089-RULE-CHANGE-TOOLING-…  move=1 keep-live=1`.

### 4.2 (ii) the controlled negative — final design
```
== pre-create the planned target .claude/pipeline/archive/TASKBOARD-rows-089-RULE-CHANGE-TOOLING-TASK-1604-TASK-1605-TASK-1606-The-QA-gat.md
archive files before: 106
$ python Tools/archive_board.py --apply --rows   (controlled negative)
archive dir: next numbers TASKBOARD-018 / TASKBOARD-rows-089 (register: INDEX.md)
  section: TASKBOARD-018-THE-BOT-SWITCH-...  rows=1 TASK-1600..TASK-1600 29 lines
  rows   : TASKBOARD-rows-089-RULE-CHANGE-TOOLING-...  move=1 keep-live=1 TASK-1605..TASK-1605
SANITY FAIL: would overwrite C:\Users\wesel\AppData\Local\Temp\t1605\.claude\pipeline\archive\TASKBOARD-rows-089-RULE-CHANGE-TOOLING-TASK-1604-TASK-1605-TASK-1606-The-QA-gat.md
exit=2
board sha + INDEX sha UNCHANGED (sha256sum -c of the pre-run pair)
archive files after: 106 (105 + the pre-created one); TASKBOARD-018-* written? 0
pre-created file still reads: pre-existing file - must survive untouched
all 105 run-A files unchanged
```
Zero bytes written: the board, INDEX and every archive file are sha-unchanged, the non-clashing `018` target was not written either, and the pre-created file kept its content.

### 4.3 (i) continued — `--apply --rows` (C) · `--apply` alone (D, finding 4) · `--apply --rows` (F)
```
$ python Tools/archive_board.py --apply --rows   (RUN C, after removing the pre-created file)
  ok: TASKBOARD-018-THE-BOT-SWITCH-...  sha256(moved) == sha256(read back) == b2772bfdd97e895a  rows=1
  ok: TASKBOARD-rows-089-RULE-CHANGE-TOOLING-...  sha256(moved) == sha256(read back) == 7de6e0cce6c3b887  rows=1
applied: 2 new archive file(s); INDEX lists 107 files; board now 21785 lines (5977614 bytes); sha256 396bf2b9e4cb9434      exit=0
=== after C: files=107 INDEX rows=107 '## Archive' count=1 — all 105 run-A files unchanged
196:Last run 2026-10-04: the archive holds 107 files, 910 rows (TASK-0012..TASK-1605); this run added 2 file(s).
TASKBOARD-018-…: header starts <!-- ARCHIVED from .claude/pipeline/TASKBOARD.md section "🎮 THE BOT SWITCH — ... ; line 2: ## 🎮 THE BOT SWITCH — **`TASK-1600` ...

(rig flip: TASK-1607 → done; THE BOT-DISABLED RECIPE keeps TASK-1608 live → pass 2 only, pass 1 empty)
$ python Tools/archive_board.py --apply   (RUN D: --apply alone)
  rows   : TASKBOARD-rows-090-THE-BOT-DISABLED-RECIPE-TASK-1607-TASK-1608-RCP-vsbot-bot-di.md  move=1 keep-live=1
pass 1 found nothing; pass --rows for pass 2      exit=1
board + INDEX unchanged; files=107

$ python Tools/archive_board.py --apply --rows   (RUN F)
  ok: TASKBOARD-rows-090-...  sha256(moved) == sha256(read back) == 977ac690075819a0  rows=1
applied: 1 new archive file(s); INDEX lists 108 files; board now 21772 lines (5969789 bytes); sha256 2bb3ee08b774b89e      exit=0
=== after F: files=108 INDEX rows=108 '## Archive' count=1 — all 107 run-A/C files unchanged
196:Last run 2026-10-04: the archive holds 108 files, 911 rows (TASK-0012..TASK-1607); this run added 1 file(s).
```
Run G (sub-rig `t1605b`, live board copy with its 8-line `## Archive` section deleted, count 0): `applied: 2 new archive file(s); INDEX lists 105 files; board now 21829 lines … exit=0` → `'## Archive' count=1; inserted at line 190, after the section [## Task template]`; `Last run 2026-10-04: the archive holds 105 files, 908 rows (TASK-0012..TASK-1604); this run added 2 file(s).`

Summary of (i): after every run the `## Archive` heading count was 1 (A, B, C, D, F, G); INDEX rows == `archive/TASKBOARD-*.md` count after every writing run (105/105, 107/107, 108/108, 105/105); every run-1 file's sha256 unchanged after runs 2–4 (`sha256sum -c` lists of 105 and 107); the tool printed `sha256(moved) == sha256(read back) == <hash>` for every file it wrote; numbers continued 087, 088 → 018, 089 → 090 with no reuse.

### 4.4 (iii) `split_conventions.py` plan → `--apply` → `--reindex` → `--reindex` (rig)
```
$ python Tools/split_conventions.py   (plan)
CONVENTIONS.md: 481 lines, 31 sections -> keep 31 / move 0
  kept core: 481 lines
nothing to move      exit=0                       <- the '⚖️ LAW MAINTENANCE' section is NOT in the move list
$ python Tools/split_conventions.py --apply
applied: 0 destination file(s) in ...\t1605\.claude\pipeline\law; CONVENTIONS.md now 481 lines (non-index bytes 98845 B == original minus moved minus old index); namespaces indexed: 31      exit=0
  after --apply:      5f0d95de4c3f65b7e45148fcfb521d8bda10488921f3f9f1ede0cad4fdff02b5  law files=56
$ python Tools/split_conventions.py --reindex   (1st)
reindexed: 56 law files; CONVENTIONS.md now 481 lines; index 65 lines; non-index bytes unchanged (98845 B, sha256 579d1b8741dc7d79)      exit=0
  after --reindex #1: 5f0d95de4c3f65b7e45148fcfb521d8bda10488921f3f9f1ede0cad4fdff02b5
$ python Tools/split_conventions.py --reindex   (2nd)
reindexed: 56 law files; CONVENTIONS.md now 481 lines; index 65 lines; non-index bytes unchanged (98845 B, sha256 579d1b8741dc7d79)      exit=0
  after --reindex #2: 5f0d95de4c3f65b7e45148fcfb521d8bda10488921f3f9f1ede0cad4fdff02b5
```
W1 positive control (driver importing the rig's module, same partition code with and without the new entry): `with '⚖️ LAW MAINTENANCE' in KEEP_PREFIXES -> move list: []` · `without it (the TASK-1604 shape) -> move list: ['⚖️ LAW MAINTENANCE (2026-10-04)']` · `next_nn() from the scratch law dir: 27`.

### 4.5 (iv) positive controls — each checker called from a scratch driver, unflipped then with ONE byte flipped
```
check_file_equals   unflipped        : OK
check_file_equals   one byte flipped : drv\LAWX.md differs at byte 30 (expected 43 B sha256 c2776aad6e3d8499, read 43 B sha256 22921b33daa6d00e)
check_file_appended unflipped        : OK
check_file_appended tail byte flipped: drv\LAWX.md tail differs from the moved section at file byte 42 (section 18 B, file 52 B)
check_file_appended prev byte flipped: drv\LAWX.md pre-existing bytes changed at byte 24
check_core_nonindex unflipped (rig CONVENTIONS.md): OK
check_core_nonindex one byte flipped in a kept section ('A'->'@' of 'Asset'): ...\t1605\.claude\pipeline\CONVENTIONS.md non-index bytes differ at byte 207 (expected 98845 B sha256 579d1b8741dc7d79, read 98845 B sha256 639546aecac91c92)
one byte flipped INSIDE the index -> check_core_nonindex: OK (the non-index part is unchanged, as it should be)
                                   -> check_core_index   : ...\CONVENTIONS.md index section differs from the generated index at byte 842 (generated 10752 B, read 10752 B)
rig CONVENTIONS.md restored byte-for-byte: True | check_core_nonindex: OK

check_readback unflipped         : OK
check_readback one byte flipped  : TASKBOARD-rows-999-drv.md: read-back differs from the moved bytes at byte 47 (moved 58 B sha256 ac9976e724ff6865, read 58 B sha256 fcf1d3b03426b391)
check_readback wrong expected id : TASKBOARD-rows-999-drv.md: heading-level ids read back [4242] != moved [4243]
check_readback substring trap (expects TASK-424, file holds TASK-4242): TASKBOARD-rows-999-drv.md: heading-level ids read back [4242] != moved [424]
check_conservation unflipped (b,c moved, note added)     : OK
check_conservation same-length corruption (d -> X)       : line multiset differs by 2 line(s); first: 'd' (original 4 + added 1 vs new 3 + moved 2 + removed 0)
check_conservation a line silently dropped (d missing)   : line multiset differs by 1 line(s); first: 'd' (original 4 + added 1 vs new 2 + moved 2 + removed 0)
```
Every checker reports OK unflipped and FAIL with the first differing byte offset (or the id mismatch) flipped; the substring trap the report named (`TASK-424` vs `TASK-4242`) is rejected.

## 5. The live `--reindex` (owed by `SC-§143` cl. 4 for today's `law/TL.md` edit) — the ONLY write to `CONVENTIONS.md` by this row
Permitted after §4.4's pair came out byte-identical. Pre-run backup copy taken to the scratchpad (`CONVENTIONS.pre-reindex.md`).
```
sha256 BEFORE: 5f0d95de4c3f65b7e45148fcfb521d8bda10488921f3f9f1ede0cad4fdff02b5  (109597 B)
$ C:\Python314\python.exe Tools/split_conventions.py --reindex   (LIVE)
reindexed: 56 law files; CONVENTIONS.md now 481 lines; index 65 lines; non-index bytes unchanged (98845 B, sha256 579d1b8741dc7d79)      exit=0
sha256 AFTER : 5f0d95de4c3f65b7e45148fcfb521d8bda10488921f3f9f1ede0cad4fdff02b5  (109597 B)
RESULT: byte-identical (before == after)
```
Nothing to restore. The index was already true for the `law/TL.md` edit (no heading changed), as the dispatch expected.

## 6. The `.ps1`: ASCII, parser, PS 5.1 repros, live `-WhatIf`

### 6.1 ASCII-only + CRLF (table in §1): `rg -c '[^\x00-\x7F]'` by direct path → `stop_editor.ps1` 0 · `launch_editor.ps1` 0 · `sync_mirrors.ps1` 0; positive control: the same pattern fires on `archive_board.py` (6 lines) and `split_conventions.py` (20 lines). CR count == line count on all five.

### 6.2 Harness `powershell -NoProfile -ExecutionPolicy Bypass -File <scratchpad>\ps_checks.ps1` (PowerShell 5.1.26100.9444, Windows 10.0.26200, culture en-US)
```
== (1) Parser.ParseFile on the three scripts (error count each) + positive control
  stop_editor.ps1      parse errors: 0
  launch_editor.ps1    parse errors: 0
  sync_mirrors.ps1     parse errors: 0
  positive control (deliberate syntax error): parse errors: 2; first: Unexpected token '{' in expression or statement.
== (2) WARN 6 repro: native child writes Write-Error to stderr and exits 4; parent under 'Stop', stderr captured
  OLD shape: LASTEXITCODE read = 4      OLD shape exit: 4  (expected by the finding: 7)   <- did NOT reproduce, see §3.4
  NEW shape: LASTEXITCODE read = 4      NEW shape exit: 4  (expected: 4)
== (3) WARN 7 repro: registry property read under StrictMode on a key WITHOUT an InstalledDirectory value (HKCU:\Software)
  OLD shape: THREW PropertyNotFoundException -> outer catch -> exit 7, fallback unreachable
  NEW shape: no throw, dir=[] -> stock-path fallback reachable
== (4) NIT launch :117-122 repro: culture formatting at >= 1000 s
  '{1:n0}' with 1034.6 -> 1,035
  [int][math]::Round(1034.6) with '{1}' -> 1035
== (5) NIT launch :90 repro: .Response on a non-WebException under StrictMode
  OLD shape: inner THREW PropertyNotFoundException -> would surface as exit 7 mid-wait
  NEW shape: false, no throw
== (6) live: sync_mirrors.ps1 -WhatIf
  sync_mirrors: no change   C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\GameDevSetup.md
  sync_mirrors: no change   C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\Aura AI for Unreal — Integration Plan.md
  aura_sync: ... no change (INDEX_IGNORE.txt 4133 B, project_memory.txt 5286 B, source == dest sha256) ... aura_sync: summary - 0 copied, 2 unchanged, 0 mismatched
  sync_mirrors: aura_sync.ps1 exit 0
  sync_mirrors: summary - 0 copied, 2 unchanged, 0 mismatched, 0 would-copy (-WhatIf)
  sync_mirrors.ps1 -WhatIf exit: 0
== (7) live: sync_mirrors.ps1 -WhatIf -VaultPath <existing folder WITHOUT .obsidian>
  sync_mirrors: C:\Users\wesel\AppData\Local\Temp\t1605_ps is not a vault (no .obsidian folder); nothing copied
  exit: 2
  control: does the real vault carry .obsidian? True
```

## 7. Exit-code contracts (headers kept true)
- `archive_board.py` 0 / 1 (nothing to archive — now also the `--apply`-alone empty pass 1) / 2 (sanity; the board is never changed on a 2; this run's archive files are removed, a half-written INDEX restored) / 64. Measured: 0 (A, C, F, G), 1 (B, D), 2 (ii), 64 not exercised (unchanged code path).
- `split_conventions.py` 0 / 2 (sanity; this run's writes rolled back, `CONVENTIONS.md` restored) / 64. Measured: 0 (plan, `--apply`, `--reindex` ×3); 2 reachable — proven by the driver on the same functions the exit-2 paths call (§4.5), not by a corrupted end-to-end run.
- `launch_editor.ps1` 0 / 2 / 5 / 6 / 7 unchanged in number; `0`'s wording now matches what is tested; `7` now also covers the exe-missing path explicitly (code unchanged). Not executed (fence).
- `sync_mirrors.ps1` 0 / 2 / 3 / 4 / 7 unchanged in number; `2` now also "not a vault"; `0` under `-WhatIf` documented. Measured: 0 (live `-WhatIf`), 2 (`-VaultPath` without `.obsidian`).

## 8. Not examined / limitations
- `launch_editor.ps1` and `stop_editor.ps1` were never executed in any mode (fence; editor PID 30480 live). Their edits are proven by: the 5.1 parser (0 errors, control 2), ASCII/CRLF measurement, and the shape-level repros in §6.2 (registry guard, WebException guard, culture formatting) — not by running the scripts. The `-WhatIf`/`Test-Path` reorder is read-verified only.
- WARN 6's failure mode did not reproduce (§3.4); the fix is implemented as the row mandates, and I claim no measured defect behind it.
- `split_conventions.py`'s end-to-end exit-2 path was not driven by a corrupted disk; the checker functions it calls were driven red by the (iv) driver. The two in-memory pre-write checks (lossless split, partition) are invariant assertions — structurally true of today's code and there to catch a future edit of the loop — the read-back checks are the ones that fail on disk truth.
- The rig ran at `%TEMP%\t1605` / `t1605b`, not under the session scratchpad (§3.5, MAX_PATH).
- Runs in the rig used rig-only status flips (`TASK-1600`, `1605`, `1607` → `done`) to make a second run move something; the live board was never flipped except this row's own `status:` line.
- The live `--reindex` was the only live write; `archive/`, `law/`, `TASKBOARD.md` (beyond this row's status line), `settings.local.json`, `Source/` untouched; no git.
- Two `_Archive note_` lines under one heading after two pass-2 runs (§3.6) — left for the manager.
