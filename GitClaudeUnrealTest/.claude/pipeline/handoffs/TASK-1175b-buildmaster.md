# TASK-1175 (CODE CASE) — build-master handoff

⚠️ **BORN OUTSIDE ITS OWN COMMIT.** This file did not exist when `4a3da63` was made and is **not** in it — the dispatch fences my own handoff out of the staging set. It is the same known-orphan shape as `handoffs/TASK-1175-buildmaster.md`, which *was* staged (it was born outside `74baab3`, its own subject commit).

**Commit: `4a3da63`** · `main` **8 ahead**, **UNPUSHED** · base `74baab3` · **row id is `TASK-1175`, unchanged** — see §0.

---

## 0. WHICH ROW THIS IS — RESOLVED AGAINST THE BOARD, NOT AGAINST THE DISPATCH

The dispatch warned that the code case might carry a different id. **It does not.** `TASKBOARD.md:1566` is `TASK-1175 — [FOGGREY-GENERALDATA-SHIP]`, and its **cl. (0) split rule** says in terms: *"this row may run **TWICE**"* — `EITHER TASK-1172` (asset-only) `OR TASK-1173 + TASK-1174 PASS` (code). The asset case closed at `74baab3` this morning; this is the **code case of the same row**. No other id was found for it.

---

## 1. COMPILE — `Result: Succeeded`

Parsed from the log, **not** from the exit code:

```
Result: Succeeded            (build log line 39)
```

- `error C…` = **0** · `warning C…` = **0** · `Result: Failed` = **0**
- Reader controlled: a bogus `^Result: ThisCannotExist` returned **0**.
- ⛔ `Build.bat` returned **exit 0**, and that is **not evidence** (the standing law).
- 14 actions. `SiegeFogVisualTest.cpp` + `FogVolume.cpp` + 9 module TUs compiled; `UnrealEditor-GitClaudeUnrealTest.dll` **relinked** ⇒ the binary genuinely moved, this was not a no-op.

🚨 **`COOKED TARGET NOT COMPILED`** (`SC-§111`). Editor target, Win64 Development, only.

Editor was **down** at row start (verified, no `UnrealEditor.exe`), and is **down** at row end. Jonathan never had to be interrupted.

---

## 2. SUITE — **555 / 555 MEASURED**

| metric | value |
|---|---|
| `Result={Success}` | **555** |
| `Result={Fail}` | **0** |
| Test Started | 555 |
| Test Completed | 555 |
| terminator | `**** TEST COMPLETE. EXIT CODE: 0 ****` |
| reader control `Result={ThisValueCannotExist}` | **0** |

**Exactly the predicted 555** (554 + 1). Reconciled against **this host's own previous measured run** (`554` at `74baab3`), never against a published absolute — QA's §9 cl. 4 noted `TASK-1167` published `556`, which is a *pre-revert* number and correctly not used.

`Siegebound.Fog.RaiseFogIsActuallyExecutedInARealWorldAndTheVisualAppearsThenGoes` ⇒ **`Result={Success}`**.

### ⛔ NO RED — and this was the row's biggest live risk

`LogGitClaudeUnrealTest: Error` count = **0**. None of the six sites (`:912, :955, :1009, :1269, :1388, :1523`) fired.
⛔ **No `AddExpectedError` was added anywhere** — QA's §2 ruling honoured, and it was moot: there was nothing to suppress.

⚠️ One line matches a naive `Error` grep and is a **false positive** — test 10's own `AddInfo` prose *about* the Error site. Recorded so the next host does not re-derive it:
> `LogAutomationController: ⭐ (4) MEASURED off the spawned actor — ACHIEVED scale (640.000, 360.000, 260.000), ACHIEVED Z 7000.0 … The VERDICT on scale is SpawnFogVisual's own Error site, ⛔ not a predicate in this test`

⭐ **First-ever measurement: `TASK-1071`'s scale substitution has NOT returned** — ACHIEVED `(640, 360, 260)`, not `(20, 20, 5)`.

---

## 3. THE CONSOLE LANE — TRANSCRIPT (deliverable (b), never exercised before)

### 3.1 What was actually run

```
UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Siege.Fog.Raise,Siege.Fog.Clear,Siege.Fog.Status,QUIT_EDITOR" \
  -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<log>
```
**Exit 0, self-terminated in 12 seconds.** Dispatch confirmed in the log:
```
Cmd: Siege.Fog.Raise
Cmd: Siege.Fog.Clear
Cmd: Siege.Fog.Status
Cmd: QUIT_EDITOR
```

### 3.2 The transcript

```
Warning: [Siege.Fog.Raise] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('L_Arena'), not a game/PIE world … ⛔ DO NOT SAVE THE MAP (`GFX-§11`)
[FogVolume_0] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object).
[FogVolume_0] Fog INTEGRITY FLOOR engaged — ACHIEVED 'r.VolumetricFog'=1, 'r.VolumetricFog.GridPixelSize'=16, 'r.VolumetricFog.GridSizeZ'=64, height fog volumetric=ON (view distance 6000) …
[FogVolume_0] Fog VISUAL spawned: 'BP_SiegeFog_C_0' — ACHIEVED Z=7000, ACHIEVED scale (640, 360, 260) …
[FogVolume_0] Fog raised for 300 s (refresh, never stack — J-F16); it lifts at world time 300.0.
[Siege.Fog.Raise] AFogVolume::RaiseFog() executed on 'FogVolume_0' and returned TRUE. Fog is now UP. ⛔ This line is the POSITIVE CONTROL for `SC-§113` cl. 3(c): its presence proves the fog path RAN.
Warning: [Siege.Fog.Clear] ⚠️⚠️ ACTING ON AN ⛔ EDITOR WORLD ('L_Arena') …
[FogVolume_0] Fog INTEGRITY FLOOR released — ACHIEVED 'r.VolumetricFog'=1, now owned by SetByScalability …
[FogVolume_0] Fog VISUAL destroyed ('BP_SiegeFog_C_0') …
[Siege.Fog.Clear] AFogVolume::ResetFog() executed on 'FogVolume_0'. Fog is now DOWN and prevention is now DOWN.
[Siege.Fog.Status] 'FogVolume_0' — fog DOWN, prevention DOWN (0 s of prevention remain). Read from the shipped accessors …
```

⇒ **A complete round trip.** `Status` **read the state back** after `Clear` (`fog DOWN`) — a resolved-state assertion, not a call tally (`SC-§104`). ⇒ **`SC-§113` cl. 4 is closed evidentially, not merely structurally.**

### 3.3 🚨🚨 W9 — THE DOCUMENTED INVOCATION DOES NOT WORK, AND IT FAILS SILENTLY

**This is the most valuable thing this row produced and it is a PROSE defect, not a code defect.**

The recipe in `qa/TASK-1174-report.md` §0, `handoffs/TASK-1173-programmer.md` §1(b)/§4, and on the board:
```
-ExecCmds="Siege.Fog.Raise;Siege.Fog.Status;Quit"
```
**Measured: nothing ran.** The engine took the whole string as ONE command name:
```
[2026.09.09-08.30.17:543][  0]Cmd: Siege.Fog.Raise;Siege.Fog.Status;Quit
```
⛔ no fog line · ⛔ **no `Command not recognized`** — no error, no warning · ⛔ the process **never exited** (I killed it at ~10 min).

**Root cause, from engine source — `-ExecCmds` splits on COMMA, never semicolon:**
`Engine/Source/Runtime/Engine/Private/ParseExecCommands.cpp:29`
```cpp
else if (CurrentChar == ',' && !bInQuotes)
```
The `;` idiom works for the **suite** only because `Automation RunTests Siegebound;Quit` is ONE command whose `;` is split by the **`Automation` handler's own** parser. **It is not an `-ExecCmds` feature and does not generalise** — which is exactly the inference QA §1(b) accepted ("reaches it by substituting the command list").

**Second defect — `Quit` does not quit an editor commandlet.** `QUIT`/`EXIT` live in `UGameEngine::Exec` (`GameEngine.cpp:1527`); a commandlet runs `UUnrealEdEngine`, which handles **`QUIT_EDITOR`** (`EditorServer.cpp:5993`). With commas + `Quit`, all three fog commands ran correctly and the process **still hung** — killed at ~9 min, after watching the fog expire naturally at exactly +300 s (`FogDurationSeconds`).

✅ **Working recipe (measured, exits itself in 12 s): COMMAS + `QUIT_EDITOR`.**
⛔ I did **not** edit the programmer's handoff (`SC-§53` cl. 3 — a struck line is named, not quietly overwritten). **Routed to the manager as a prose-fix row.**

⚠️ **Note for whoever fixes it:** the *PIE console* usage in `FOG-§12.5c` cl. 5's recipe (typing `Siege.Fog.Raise` into the in-game console) is **unaffected** — this defect is specific to the `-ExecCmds` command line.

---

## 4. BLOCKER-0 — THE DISCHARGE

```
grep -cE "AFogVolume|Siege\.Fog\." <suite log>     = 8      NON-ZERO
grep -cE "AFogVolume|Siege\.Fog\." <console log>   = 11     NON-ZERO
grep -cE "AFogVolume|Siege\.Fog\." <build log>     = 0      (reader control)
```

**The line that discharges it** — `FogVolume.cpp:338`, inside the **shipped** `FindOrSpawn`, production code:
```
[2026.09.09-08.28.30:126][884]LogGitClaudeUnrealTest: [FogVolume_0] AFogVolume spawned — the fog-state actor now exists for this match (FOG-§10.1: one state object).
```
Plus, in the suite lane, the whole shipped chain: `Fog INTEGRITY FLOOR engaged` → `Fog VISUAL spawned … ACHIEVED scale (640, 360, 260)` → `Fog raised for 300 s` → `Fog INTEGRITY FLOOR released` → `Fog VISUAL destroyed`.

### 🚨 W8 — THE BARE COUNT IS A CONFOUNDED INSTRUMENT (new finding)

**7 of the suite lane's 8 matches are a PRE-EXISTING TEST NAME** — `TheFogStateIsReadInExactlyOnePlaceAndTheSeamConsultsAFogVolume` — which contains the substring `AFogVolume`. Measured: `git grep -c` at **`20c1bea`** finds it in `SiegeFogClampTest.cpp`, i.e. **before `TASK-1173` existed**.

⇒ **A bare `grep -c` would have returned NON-ZERO even with the fog path never executing.** Had I reported "8, non-zero, discharged" and stopped, that would have been a false green of exactly the class `SC-§113` was written about.

✅ **QA's own criterion was correctly specified and is what discriminates:** *"a non-zero count **with at least one quoted `LogGitClaudeUnrealTest:` line**"*.
```
grep "LogGitClaudeUnrealTest:" <suite log> | grep -cE "AFogVolume|Siege\.Fog\."   = 1
```
**Use the filtered form from now on. `SC-§113` cl. 2's original `0` is not comparable to a bare count.**

⚠️ QA's §0 note also held exactly as written: `*GetNameSafe(this)` yields `FogVolume_0`, so `Fog INTEGRITY FLOOR engaged` / `Fog VISUAL spawned` match **neither** alternative. A low count is not "the floor did not run."

---

## 5. THE `--numstat` RESULT — `+686 / −0`, CONFIRMED

**In the worktree, before the commit:**
```
298   0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp
75    0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.h
313   0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp
```
**In the commit itself** — identical. ⛔ **No `−` on any of the three ⇒ no stop-and-escalate.** The author's arithmetic proof that no shipped fog function was rewritten **holds**.

⚠️ **A correction QA could not have caught without Git.** Its §9 cl. 3 prescribes `git diff --numstat 20c1bea HEAD -- <files>`. **That returns EMPTY**, because `HEAD` (`74baab3`) *is* the reverted state and the 686 lines were **uncommitted**. An unwary host would read the empty output as "no diff" and either panic or wave it through. The correct right-hand side pre-commit is the **worktree**.

**Negative controls (`SC-§102`), run BEFORE the real query:**
| control | result |
|---|---|
| mis-anchored pathspec (`Source/…` without the `GitClaudeUnrealTest/` prefix) | **silence, exit 0** |
| correctly-anchored but nonexistent file | **silence, exit 0** |

⇒ confirmed: **a mis-anchored pathspec answers with silence, not an error.**

---

## 6. PATHSPEC + EXISTENCE CHECK (`SC-§106`)

Git root is **`C:/GitProjects/GitHub/GitClaudeUnrealTesting`** — one level above the project dir. Every path below is anchored there.

| path | check |
|---|---|
| `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` | EXISTS |
| `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` | EXISTS |
| `GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp` | EXISTS |
| `GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1174-report.md` | EXISTS |
| `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1173-programmer.md` | EXISTS |
| `GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1175-buildmaster.md` | EXISTS *(known orphan, born outside `74baab3`)* |
| `GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md` | EXISTS |
| **`GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1173-report.md`** | ⛔ **MISSING — negative control, FIRED CORRECTLY** |

⭐ **The MISSING control is not decoration.** It is the same shape as the asset case's near-miss: a QA report named after the **reviewed** row rather than the **gate** row, which has never existed. The gate report is `TASK-1174`, named after the gate (`SC-§106`).

**cl. 7a sweep:** the working tree held nothing else — `CONVENTIONS.md` was **not dirty** and was therefore **not staged** (checked, not assumed).

**Never staged, verified clean at commit time:** `Config/**` · `Tools/**` · `Content/Maps/L_Arena.umap` · `Content/FogArea/**` · `Content/**` at large · `testvideo/**` · **this handoff**.

**LFS: no subject.** Every staged path is text — `git check-attr filter` ⇒ `unspecified` on all. There is **no binary in this commit**, so the oid-vs-sha256 check had nothing to run against. Stated rather than claimed, because a check with no subject is not a check performed.

⚠️ Two of the three pipeline files were **untracked**, so `git commit -- <paths>` rejected them outright (`did not match any file(s) known to git`) and had to be `git add`-ed first. **It errored loudly rather than skipping silently** — worth knowing, because the silent-skip failure mode is the one this project fears.

---

## 7. `git show --stat HEAD`

```
commit 4a3da63ab55b593a9dba62073f1c3c19f255f82a
Author: Jonathan Wesely <wesely.jonathan@gmail.com>
Date:   Wed Sep 9 01:57:08 2026 -0700

    TASK-1173: the fog path was unreachable by every agent in this pipeline, and this is
    the instrument that changes it - proven by making it execute, twice, in two lanes
    (TASK-1174 qa-passed, hosted by TASK-1175 code case)

 7 files changed, 1865 insertions(+), 9 deletions(-)
```
Per-file, from the **commit** (not the index):
```
9     9   GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md
254   0   GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1173-programmer.md
266   0   GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1175-buildmaster.md
650   0   GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1174-report.md
298   0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp
75    0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.h
313   0   GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp
```
The `9/9` on `TASKBOARD.md` is the **inherited** asset-case annotation set, not mine. `git diff -- Source` after the commit is **empty** ⇒ nothing left behind. Index was empty before staging (the UE Git plugin had auto-staged nothing, since the editor never ran a save). **`main` 8 ahead of `origin/main`. NEVER pushed.**

---

## 8. `L_Arena` + WHAT STAYED DIRTY

**`Content/Maps/L_Arena.umap` = `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** — byte-identical to the hash the asset case recorded, **unmoved at 3 checkpoints** (pre-compile, post-suite, post-console-run).

⚠️ **The console lane DID dirty it in memory** — `Siege.Fog.Raise` spawns the state actor into the open editor world and warns loudly that it does so. **Nothing was saved**; the commandlet was killed, which discards. The on-disk hash proves it. ⛔ **Anyone running lane (b) interactively must not press Save All.**

**Still dirty at row end, and whose:**
| path | whose | why |
|---|---|---|
| `.claude/pipeline/TASKBOARD.md` | **mine** | the three post-commit `- status:` lines for `TASK-1173/1174/1175`. A status cannot name the hash of the commit that contains it, so it is written after. **This is the identical hand-forward this row inherited from its own asset case** (`74baab3` contained `TASKBOARD.md`, and its hash annotations were written afterwards). Next host sweeps it. |
| `handoffs/TASK-1175b-buildmaster.md` | **mine** | this file — fenced out of its own commit by the dispatch. |

Nothing else. `Content/`, `Config/`, `Tools/`, `testvideo/` all clean.

---

## 9. 🚨 FOR `TASK-1177`'s OWNER — CARRIED FORWARD, NOT SOLVED

### W6 — AN UNCONTROLLED CONFOUND IN THE `FOG-§12.5c` cl. 5 RECIPE. **DO NOT TAKE THE SERIES WITHOUT READING THIS.**

A **card-raised** fog runs `EnforceFogRenderFloor` and pins, at `ECVF_SetByCode` (`FogVolume.cpp:1337-1344`):
```
r.VolumetricFog = 1 · r.VolumetricFog.GridPixelSize = 16 · r.VolumetricFog.GridSizeZ = 64
```
A **hand-spawned probe** runs **no `AFogVolume` code at all** and therefore **never gets that froxel grid**.

Grid resolution is exactly the kind of parameter that changes the **amplitude or period of a temporal-accumulation artefact** — which is the very thing the ~181 s cycle is suspected to be.

⇒ ⛔ **A row concluding *"the cycle is absent under the card"* could be reading THE GRID, NOT THE ACTUATION** — and would void `FOG-§12.5c` on a confound.

**The fix, and you must say which one you did** (`FOG-§12.5b`):
1. re-take the **probe** series **with** `r.VolumetricFog.GridPixelSize 16` / `GridSizeZ 64` applied, **or**
2. take the **card** series with `bEnforceFogRenderFloor = false` on the CDO.

⭐ **I can now confirm the floor engages, from a real log line** (§3.2) — this is no longer a code reading. The engaged line prints the ACHIEVED grid values, so **step 3 of the recipe gives you the control for free**: capture `Fog INTEGRITY FLOOR engaged` and `Fog VISUAL spawned` before one pixel, and compare the card's ACHIEVED geometry and grid against the probe's.

✅ **Also newly cheap for you:** lane (b) means you no longer need the card to raise fog on demand — but ⛔ use the **PIE console**, not the commandlet, and mind **W9** (commas, not semicolons) if you script it.

### Unsolved, carried forward verbatim

- **W1** — `SiegeFogVisualTest.cpp:1541-1555` — the teardown's `RouteEndPlay` loop is **dead as written**. `bBegunPlay` is never set without a GameMode, so `AFogVolume::EndPlay` never runs at teardown. **Harmless today only because `ResetFog()` (`:1699`) releases unconditionally with no return between it and `RaiseFog()`.** The comment promises a safety net that is not connected. ⛔ Do not silently delete the loop — it becomes correct the moment the world is given `NotifyBeginPlay`.
- **W2** — `SiegeFogVisualTest.cpp:97-100` — the file's **own** header still claims there is no `SpawnActor` anywhere in `Siegebound/Tests/`, while `:1622` spawns one and `:1518` creates a world. In the file this row owns.
- **W5** — `FogVolume.cpp:99-104` — the shared warning says the state actor *"is spawned into the map"*, but `ExecClearFog` (`:166`) uses `Find` and **cannot spawn**. ⭐ **Upgraded from a code reading to a MEASUREMENT this row** — it fired on the Clear path (§3.2, line 2265).
- **W3 / W4** — stale counts in comments ("SEVEN Error sites" — there are six; "Nine files" — ≥15). Remedy: state a predicate, not a number.
- **W9** *(new, mine)* — the `-ExecCmds` recipe defect, §3.3.
- **W8** *(new, mine)* — the confounded discharge grep, §4.
- **The manager still owes a row** for the ~15-file stale *"every test is HEADLESS"* sweep. Correctly declared, correctly not swept here.

---

## 10. WHAT THIS ROW DOES **NOT** CLAIM

- ⛔ **No ask-(B) claim in either direction.** This row did not measure visibility distance and makes no statement about it. `TASK-1177` is open. The only permitted ask-(B) sentence remains the field quote: `density 0.5` · `sharpness 0.1` · `wind Speed 0.5` — a read-read pair, nothing written.
- ⛔ **No ask-(C) claim in either direction.** Nothing here touches colour, and 🧑 Jonathan's eye remains the only instrument for it (`TASK-1159` open).
- ⛔ **Nothing about how the fog LOOKS.** No pixels were measured by this row.
- ⛔ The 300 s wait is not exercised as a *test*; the fog was observed expiring naturally at +300 s in a killed console run, which is an observation, not a gate.
- ⛔ `FOG-§12.5c` cl. 5 is made **answerable**, not **answered** — and W6 must be controlled by whoever answers it.
