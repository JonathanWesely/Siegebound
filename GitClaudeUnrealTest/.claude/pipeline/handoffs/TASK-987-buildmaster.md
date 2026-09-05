# TASK-987 — THE COMMIT HOST — build-master handoff

**Date:** 2026-09-04 · **Agent:** build-master
**Verdict:** ✅ **SHIPPED.** ONE compile · ONE **EXECUTED** suite · ONE commit. All three spent, none wasted.
**Commit:** **`1a457df`** · 81 files · 31,573 insertions / 599 deletions · `main` **20 ahead** of `origin/main` · ⛔ **NOT PUSHED.**

---

## 1. The quoted `Result:` line, with its log line number

Build log: `<scratchpad>/TASK-987-build.log` (45 lines total).

```
43:Result: Succeeded
```

`grep -c "Result: Failed"` = **0**.

> **The raw exit code was `0` and it is NOT the evidence.** `Build.bat` returns 0 on a failed build.
> Line 43 is the evidence. The exit code was read, printed, and discarded.

## 2. Diagnostic counts (all from the log)

| Pattern | Count |
|---|---|
| `error C` | **0** |
| `error LNK` | **0** |
| `: error` | **0** |
| `: warning` | **0** |
| `0x800711C7` (Smart App Control) | **0** |

**19 actions**, 16 compiles, 15.18 s in the UBA local executor, 17.82 s total.

**The build did real work — this was not a no-op.** The 16 compiles include every file this batch
touched (`SiegeFogRefusalTest.cpp`, `SiegeCardRosterTest.cpp`, `SiegeCardArtRosterTest.cpp`,
`SiegeGameMode.cpp`, `SiegePlayerController.cpp`, `SummonedUnit.cpp`, and 10 unity/module blobs),
followed by `[17/19] Link … .lib` and `[18/19] Link … .dll`.

**The stale-DLL lie, ruled out by measurement:**

| | Value |
|---|---|
| DLL after TASK-1020's diagnostic build | 8,624,640 bytes |
| DLL after **this** build | **8,652,288 bytes** (+27,648) |
| mtime | 2026-09-04 20:58:26 · **age 21 s** at check |

**SAC did not fire.** Not a Jonathan-only blocker today.

## 3. ⭐⭐ THE EXECUTED SUITE — the first execution in this entire batch

Command: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<scratchpad>/suite-987.log`

| | |
|---|---|
| **`Result={Success}`** | ⭐ **475** |
| **`Result={Fail}`** | ⭐ **0** |
| `Test Started` / `Test Completed` | 475 / 475 |

**The log is real, and I checked that before trusting its zero:** `suite-987.log` = **6,083 lines /
1,031,416 bytes**, carrying genuine `Test Started` / `Test Completed` / `BeginEvents` traffic. This is
the specific trap the row named — a predecessor once read a 15-line build preamble as a clean run.

### 3.1 ⛔ ZERO UNEXPLAINED MISMATCH — the derivation closed exactly

I derived my own expected total at my own instant from the rows actually in my pathspec, then
executed. **I did not publish a remembered absolute, and I did not adopt the executed number as
truth without first predicting it.**

| step | delta | running |
|---|---|---|
| baseline (pre-`1003`, declared) | — | `445 / 34` |
| `TASK-1003` | +1 case | `446 / 34` |
| `TASK-998` (`SiegeFogVolumeTest.cpp`) | +7 cases, +1 file | `453 / 35` |
| `TASK-999` (`SiegeCardGlossaryTest.cpp`) | +4 cases, +1 file | `457 / 36` |
| `TASK-982` (`SiegeBrightSunTest.cpp`) | +8 cases, +1 file | `465 / 37` |
| `TASK-989` + `TASK-991` (`SiegeFogRefusalTest.cpp`) | **+10** cases, +1 file | **`475 / 38`** |

**Predicted `475 / 38`. Measured 475 declared in the tree. Executed 475.** All three agree.
The `+10` is exactly `TASK-991`'s 4 + `TASK-989`'s 6, as the dispatch stated.

### 3.2 Both pre-explained mismatches behaved as predicted — and nothing else appeared

1. **The `TASK-1000` rename reconciled to `+0`.** The old name `IsNotLiveUntilTask839` occurs
   **0 times** in the suite log. Exactly **5** `Siegebound.Notice.*` cases ran, matching the 5
   declared in `SiegeUnitNoticeRangeTest.cpp`. **No test was lost.** A name-keyed instrument would
   have reported `−1 / +1`; a count-keyed one reported nothing, as the row said it would.
2. **The card roster's `32 → 34` growth was suite-neutral.** No red in either roster gate.

**Nothing else was unaccounted for.** There is no figure in this report I adopted to paper over a gap.

### 3.3 ⭐ `W-5` — the `AddInfo` line, VERBATIM, and it is LIVE rather than vacuous

Suite log **line 5492**:

```
Roster derivation: 34 rows read, 6 bRanged, 0 populated NoticeRange cell(s). LEASH 8000.0 > NOTICE 5000.0 > LONGEST FIRING 3600.0 (`Longbowman`).
```

**Not blank, not zeroed** ⇒ the ordering pin is derived from a roster it actually read, so the pass
is not vacuous. Against the clause's *"EXPECTED TODAY"* text the single deviation is `32 rows` ⇒
**`34 rows`**, which is the `Fog` + `BrightSun` growth this very commit ships. **Not a finding.**
`6 bRanged` and `LEASH 8000.0 > NOTICE 5000.0 > LONGEST FIRING 3600.0 (Longbowman)` match exactly.

### 3.4 🧑 `J-F20` UPHELD — the retired `3600` was NOT written

- `Docs/Data/cards.csv` field 32 (`NoticeRange`): **34 data rows, 34 blank, 0 populated** — measured.
- The suite's own independent derivation agrees: **`0 populated NoticeRange cell(s)`**.
- ⛔ **`handoffs/TASK-993-programmer.md` §9.6's boldfaced order to `set_rows {"Longbowman": {"noticeRange": 3600}}` was NOT executed.** It is superseded; executing it would have shipped a retired value and inverted his ruling. **No `noticeRange` write was made to `DT_Cards` by me.**
- Clause **(6a) was read as HISTORY**, together with (6b), as the map instructed. Its struck text would have introduced the very defect it was written to prevent.

## 4. The gate audit (clause 6e) — every row in the pathspec

**Question asked of every row: is there a QA verdict written AFTER this diff existed?**

| row | subject | gate | verdict |
|---|---|---|---|
| `TASK-839` (safe half) | `SiegeCombatStatics` · `SpellLibrary` · `CardRow.h` | `qa/TASK-1015.md` | PASS · 0 blockers |
| `TASK-840` | the `Fog` card row | `qa/TASK-850.md` | PASS · 0 blockers |
| `TASK-979` | notice-range constants + accessors | `qa/TASK-979.md` (loop 2) | PASS · 0 blockers |
| `TASK-981` | the fog curve | `qa/TASK-981.md` | PASS · 0 blockers |
| `TASK-982` | BrightSun spell + `CardRow.h` item (0) | `qa/TASK-986.md` | PASS · 0 blockers |
| `TASK-983` | the `BrightSun` card row | `qa/TASK-1012.md` | PASS · 0 blockers |
| `TASK-984` | `T_CardArt_BrightSun` | art — `ready-for-integration` | ✅ |
| `TASK-842` | `T_CardArt_Fog` | art — `ready-for-integration` | ✅ |
| `TASK-989` | fog refusal | `qa/TASK-990.md` | PASS · 0 blockers |
| `TASK-991` | `AFogVolume` refusal lane | `qa/TASK-992.md` | PASS · 0 blockers |
| `TASK-993` | the `NoticeRange` channel | `qa/TASK-995.md` | PASS · 0 blockers |
| `TASK-997` | items (5a) + (5c) | `qa/TASK-995.md` | PASS · 0 blockers |
| `TASK-998` | the new `UCLASS` + 7 tests | `qa/TASK-1011.md` | PASS · 0 blockers |
| `TASK-999` | the glossary | `qa/TASK-1013.md` | PASS · 0 blockers |
| `TASK-1000` | the `CardRow.h` doc block + rename | `qa/TASK-1014.md` **§ LOOP 2** | PASS · 0 blockers |
| `TASK-1003` | constants + ordering pin | `qa/TASK-1006.md` | PASS · 0 blockers |
| `TASK-1004` | `cards.csv` cells | `qa/TASK-1006.md` | PASS · 0 blockers |
| `TASK-1005` | `DT_Cards` write-back | `qa/TASK-1006.md` | PASS · 0 blockers |
| `TASK-957` ⭐ | `SiegeCardArtRosterTest.cpp` | `qa/TASK-958.md` | PASS · 0 blockers |
| `TASK-964` ⭐ | `SiegeCardRosterTest.cpp` | `qa/TASK-965.md` | PASS · 0 blockers |

⛔ **`qa/TASK-1014.md` reads `FAIL` at its top.** That is **loop 1**. Its **§ LOOP 2** re-gate returned
**0 BLOCKERS · 2 WARN · 4 NIT** and flipped `TASK-1000` to `qa-passed`. Named here because the file's
first line, read alone, would stop this commit on a row that had already been cleared.

✅ **`TASK-996` checked before compiling, as instructed:** verdict **PASS — 0 BLOCKER**. It refutes
nothing, so the three rows amended on it (`979`/`980`/`985`) stand.

**No row in the pathspec failed the audit. Nothing was committed without a PASS.**

## 5. ⭐ Ride-alongs — derived from the actual file diff, not from the clause

**`TASK-840` (the `Fog` row) rides along and is LICENSED** by `qa/TASK-850.md` (PASS, 0 blockers,
scoped *"gate over TASK-840 ALONE"*). Not hunk-staged around; no `git add -p`.

**`TASK-831` (the Witch row) is NOT a ride-along — I measured rather than assumed.** Its row is
already **at `HEAD`**: `git show HEAD:…/cards.csv | grep -c "^Witch,"` = **1**. The clause flagged it as
an unverified candidate; it is verified now, and it is already shipped.

🚨 **TWO RIDE-ALONGS THE ROW DID NOT LIST — found by deriving instead of reading:**

- **`TASK-957`** — `Tests/SiegeCardArtRosterTest.cpp`, a **new file absent at `HEAD`**.
- **`TASK-964`** — `Tests/SiegeCardRosterTest.cpp`, modified (1 → 2 declared cases).

**Both name `TASK-961` as their ship host, and `TASK-961` was never dispatched** — its status is still
`boarded`, even though all four of its blockers (`958`/`960`/`963`/`965`) now read PASS. Their work has
been sitting uncommitted since 2026-09-03. **Both carry a PASS with 0 blockers, so both ship licensed.**

⛔ **Leaving `SiegeCardArtRosterTest.cpp` out was not an option:** it is the art gate that requires the
`CardArt` cell to *resolve*, and this commit adds two cards pointing at two new textures. Omitting it
would have committed a green-looking gate whose test file does not exist in the repository —
precisely the failure clause (6f)(i) describes. **See §9 for the follow-up this creates.**

## 6. The commit

```
1a457df62e900ccfe3e4ea1a52f5171156ea1ef1
1a457df  TASK-987: the fog batch ships — Fog and BrightSun become real cards,
         and the notice-range channel lands (TASK-839..1005)
```

**81 files** · 31,573 insertions / 599 deletions · **`main` 20 ahead of `origin/main`** · ⛔ **NOT PUSHED** (Jonathan has not asked).

### 6.1 ⛔ The atomic three — all three, verified landed AFTER the commit

`git diff --cached --name-only` named all three before the commit, and `git diff HEAD` is **empty**
for all three after it:

| file | `HEAD` vs tree, post-commit |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` | ✅ identical |
| `Docs/Data/cards.csv` | ✅ identical |
| `Content/Data/DT_Cards.uasset` | ✅ identical |

⚠️⚠️ **AN INSTRUMENT TRAP I HIT AND CAUGHT — RECORDED BECAUSE IT PRODUCES A FALSE ALARM, NOT A FALSE PASS.**
My first post-commit check was `git show HEAD:<path> | sha256sum` vs `sha256sum <path>`, and it reported
**all three DIVERGED**. Both halves of that were instrument error:
`git show` returns the **LFS pointer text** for `DT_Cards.uasset` (never the binary), and it returns
**LF-normalised** bytes for the two text files whose working copies are CRLF.
⇒ ⛔ **`git show … | sha256sum` is the WRONG instrument for an LFS file or a CRLF text file.** The right
ones are `git diff HEAD` (empty ⇒ identical) and, for the asset, the **pointer's own `oid`**.
⇒ ⚖️ ***A verification method that cannot distinguish "changed" from "stored differently" will
manufacture a finding at the worst possible moment — immediately after a commit, when the obvious
reading is that the commit went wrong.***

### 6.2 ⛔ `DT_Cards.uasset` — hashed by me, before and after, never by size

| | value |
|---|---|
| sha256 **before** the commit | `aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b` |
| sha256 **after** the commit | `aa2c5bc18322343f1caaaa4ad453aacc0bde6bd8a3b0f41f1f4365d2d9dedf5b` |
| LFS pointer **at `HEAD`** | `oid sha256:aa2c5bc1…` · `size 46510` |
| sha256 at `HEAD` before this commit | `236f0e2d1d967f0f…` (**differs** ⇒ it genuinely needed staging) |

**The asset that is committed IS the asset on disk.** This figure is measured, not relayed from a gate —
per `SC-§71b`/`SC-§78`, neither `manager` nor `qa-reviewer` holds `Bash`, so every asset claim upstream
was accepted-as-declared. This one is not.

### 6.3 ⛔ LFS sweep — oid vs `sha256`, JOINED reported beside MISMATCHES

| | count |
|---|---|
| **JOINED** | ⭐ **5** |
| **MISMATCHES** | ⭐ **0** |
| non-LFS | 0 |

`DT_Cards.uasset` · `T_CardArt_BrightSun.uasset` · `T_CardArt_Fog.uasset` · `BrightSun.png` · `Fog.png`.
**The zero is provably not vacuous** — a sweep that joins nothing reports the same `0` as one that
matches everything, so the join count is stated beside it. `T_CardArt_BrightSun` = `15570b04…` and
`T_CardArt_Fog` = `513322e7…`, matching the artist's declared values **by my own measurement**.

## 7. The derived staged set — 81 files

⛔ **Derived from `git status --porcelain` at commit time, never from a hand-written list.** The
10 untracked paths under `Source/`/`Content/` (excluding `FogArea`) were enumerated by the tool, and
the set is **all untracked paths, not "untracked test files"** — which is why the two `.png` sources
and the texture are in it.

**Source — new (8):** `Siegebound/FogVolume.h` · `Siegebound/FogVolume.cpp` ·
`Tests/SiegeBrightSunTest.cpp` · `Tests/SiegeCardArtRosterTest.cpp` · `Tests/SiegeCardGlossaryTest.cpp` ·
`Tests/SiegeFogRefusalTest.cpp` (⭐ the 10 tests) · `Tests/SiegeFogVolumeTest.cpp` ·
`Tests/SiegeUnitNoticeRangeTest.cpp`

**Source — modified (14):** `CardRow.h` · `DeckBuilderWidget.cpp` · `SiegeCombatStatics.cpp` ·
`SiegeFogStatics.cpp` · `SiegeFogStatics.h` · `SiegeGameMode.cpp` · `SiegePlayerController.cpp` ·
`SiegePlayerController.h` · `SpellLibrary.cpp` · `SummonedUnit.cpp` · `SummonedUnit.h` ·
`Tests/SiegeCardRosterTest.cpp` · `Tests/SiegeFogClampTest.cpp` · `Tests/SiegeFogTest.cpp`

**Data (2):** `Docs/Data/cards.csv` · `Content/Data/DT_Cards.uasset`

**Art (4):** `Content/UI/CardArt/T_CardArt_BrightSun.uasset` · `Content/UI/CardArt/T_CardArt_Fog.uasset` ·
`Content/RawAssets/CardArt/BrightSun.png` · `Content/RawAssets/CardArt/Fog.png`
*(`Content/RawAssets/CardArt/` was already tracked — 32 sibling PNGs at `HEAD` — so these belong here.)*

**Pipeline records (53):** `TASKBOARD.md` · `CONVENTIONS.md` · `footage/VID-005-*.md` ·
32 handoffs · 19 QA verdicts. These are the gate evidence for this very commit; the backlog had
stopped at `TASK-928`.

⛔ **`git diff --cached --name-only` was read back before committing** and confirmed all three atomic
files, `T_CardArt_BrightSun.uasset`, and all six new test files were actually staged. **A clean
`git status` was not accepted as that check** — it cannot distinguish *staged* from *never-modified*.

## 8. `git status` after — and the two deliberate residuals

```
## main...origin/main [ahead 20]
 M .claude/agents/qa-reviewer.md
 M .claude/pipeline/TASKBOARD.md
```

Untracked: **27 files, all under `Content/FogArea/`** — and nothing else.

| residual | why it is not in the commit |
|---|---|
| `Content/FogArea/**` (27) | ⛔ **NAMED AND LEFT** per clause (4) — awaiting 🧑 `J-F11` via `TASK-927`. No commit, no `.gitignore` edit, no delete, no move, `BP_FogArea` never opened. |
| `.claude/pipeline/TASKBOARD.md` | The one-line hash append. **Unavoidable**: the board is *inside* the commit, so `1a457df` could not exist when the status line was written. Only `TASK-987`'s own `status:` line was touched. |
| `.claude/agents/qa-reviewer.md` | ⚠️ **DELIBERATELY EXCLUDED, DISCLOSED.** It adds `Edit` to qa-reviewer's toolset — an **agent permission change**, outside this row's scope and not something a build-master should fold silently into a code commit. It is already live on disk (`qa/TASK-1014.md` §L2.8 records the grant taking effect); committing it is a records decision for 🧑 Jonathan or the manager, not for me. |

⛔ **No `git checkout --` / `restore` / `stash` / `reset` / `clean` was run at any point.** No `git add -A`.
No `git add -p`. No amend. **No push.**

## 9. 🚩 FOLLOW-UPS FOR THE MANAGER

1. ⭐⭐ **`TASK-961` is stranded and should be closed or cancelled.** It is still `boarded` while all four
   of its blockers read PASS, and **its two subjects shipped here instead** (`TASK-957`, `TASK-964`).
   Its other two lanes (`TASK-960`, `TASK-962`/`963`) may still hold uncommitted work — **someone should
   check whether anything else of `TASK-961`'s is sitting in the tree unshipped.** *A commit host that is
   never dispatched leaves PASS-gated work invisible: the board says "passed", the tree says "present",
   and nothing says "unshipped".*
2. **`TASK-957`/`TASK-964`'s board rows still read `ready-for-qa` / `qa-passed`** and now need flipping to
   `done` against `1a457df`. I flipped only `TASK-987`'s own `status:` line, as instructed.
3. ✅ **Clause (6i) is DISCHARGED, not deferred — `TASK-1016`'s contingency does NOT fire.**
   `TASK-982` item (0) **did** land. `CardRow.h:67` now reads *"Fog: raises the WORLD-GLOBAL fog, which
   then lifts on its own"*, with lines 69–71 explicitly recording that the old *"for `EffectDuration`"*
   claim is **"FALSE AS A MECHANISM"** because `RaiseFog()` takes no argument and the row's
   `EffectDuration` cell is not read. **No known-false sentence shipped in this commit.**
4. **`.claude/agents/qa-reviewer.md` needs a ruling** (see §8) — commit the `Edit` grant, or revert it.
5. **`TASK-980` remains BLOCKED** and contributed **zero net source lines**; nothing of its shipped here.

## 10. Editor state

⛔ **Left UP: PID `6032`, `Responding=True`, window title `GitClaudeUnrealTest - Unreal Editor`.**

Closed by name under the standing grant before the compile (PID `24284`, `Stop-Process -Force`) because
`TASK-998` adds a **new `UCLASS` in new files** and Live Coding cannot absorb that. **No save prompt was
accepted.**

⛔ **`L_Arena` was NEVER saved** — mtime `2026-08-27 15:05:16`, 605,044 bytes, **identical before and after**.

⛔ **The `Restore Packages` modal was ruled out by WINDOW-TITLE ENUMERATION, not by the port.** A full
`EnumWindows` sweep over every visible `UnrealEditor` window returned **exactly one**:
`[GitClaudeUnrealTest - Unreal Editor]`. No modal, no restore prompt. *(Had one appeared it would have
been DECLINED — it restores the exact unsaved state the kill discarded.)*
