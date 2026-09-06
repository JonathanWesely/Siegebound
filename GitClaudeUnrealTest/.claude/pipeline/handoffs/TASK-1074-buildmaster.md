# TASK-1074 — [FOGSCALE-SHIP] · build-master handoff

**Marker:** `TASK-1074-HOST` · **Date:** 2026-09-05 · **Agent:** build-master
**Gate:** `qa/TASK-1073.md` — **PASS · 0 BLOCKER · 3 WARN · 4 NIT** (verified on disk, authoritative).
**Subject:** `TASK-1072`'s working-tree fix for the fog-visual scale clobber.
**Downstream:** ⭐ **`TASK-1075` — and THAT row closes this lane, not this commit.**

---

## 0. HEADLINE

**The red was witnessed. All three mutations, not just the required one.** The author declared
*"NO WITNESSED RED"* and transferred the duty here; it is now discharged, **executed, not predicted**.

**Suite: `496 / 0` — EXECUTED.** The author's `494 ⇒ 496 (+2)` was DECLARED-NOT-EXECUTED. It is now
executed and it is **exactly correct**.

**Commit:** see §7.

---

## 1. `HEAD` AS A RELATION (`SC-§89` rule 1)

```
git merge-base --is-ancestor c6bb376 HEAD   ⇒ exit 0   ✅ PASSES
```
`HEAD` at start = `c6bb376` (`TASK-1070`). The relayed literal is genuinely an ancestor; no literal
was invented.

---

## 2. THE DESTRUCTIVE TRAP — THE PROTOCOL, AND IT HELD 3/3

All three subject files were **uncommitted at `HEAD`**. ⛔ A `git restore` / `git checkout --` after a
mutation would have deleted **the entire fix**, not the mutation. Those commands were never issued.

**Scratch copies taken BEFORE the first mutation**, under
`…/scratchpad/T1074/`, with `sha256` recorded:

| File | `sha256` — **BEFORE mutation** | `sha256` — **AFTER final restore** | Verdict |
|---|---|---|---|
| `FogVolume.cpp` | `17598ac040ba174eae4405d4ddfae8837e951e63312e127cc90b1347627c2001` | `17598ac040ba174eae4405d4ddfae8837e951e63312e127cc90b1347627c2001` | ✅ **BYTE-EXACT** |
| `FogVolume.h` | `a27f7b09852af2254e6b256dab5cdc387b1ac7f7f9a00fe231758919c8859e38` | `a27f7b09852af2254e6b256dab5cdc387b1ac7f7f9a00fe231758919c8859e38` | ✅ **BYTE-EXACT** |
| `Tests/SiegeFogVisualTest.cpp` | `76422f8fefc466f2b90a6b47c88b685992e46fa3bb83439336a4e61876bf65ff` | `76422f8fefc466f2b90a6b47c88b685992e46fa3bb83439336a4e61876bf65ff` | ✅ **BYTE-EXACT** |

Equality proven **twice**, by `sha256` **and** by `cmp` (`SC-§68` — by hash, never by size).
Each of the three mutant states produced a **different** hash, so the mutations demonstrably landed:
A `9a114d25…`, C `23c2d202…`, B `43e1dfbc…`.

### ⭐ MY OWN TRAP FROM `TASK-1070`, AND HOW IT WAS DEFEATED

Last run a `cp -p` restore **preserved mtime**, UBT judged the object current, and returned
`Result: Succeeded` in 0.91 s **with the mutant still linked**. This run: restores used `cp`
**without** `-p`, plus an explicit `touch`, and **every** compile's `[n/N]` list was read.
⇒ `FogVolume.cpp` appears **at `[2/12]` in the clean build** (§4). A green `Result:` line was never
treated as evidence on its own.

---

## 3. 🚨 THE WITNESSED RED — ALL THREE MUTATIONS, VERBATIM

Method per mutation: mutate → **full compile** → **full bounded suite** → record → restore → re-hash.

### MUTATION A (**REQUIRED**) — delete `Spawned->SetActorScale3D(RequestedScale3D);` (line 640)

Compiles (QA's call was right — `RequestedScale3D` keeps 3 other uses): `Result: Succeeded`,
`FogVolume.cpp` at `[2/12]`.

**RED: `495 / 1`**, exit code `-1`.
**Failing test:** `Siegebound.Fog.TheSpawnSiteForcesTheScaleAndLogsWhatItGotRatherThanWhatItAsked`

**Exactly the 3 assertions QA predicted**, verbatim:

```
Error: Expected '⭐⭐⭐ THE REPAIR: the spawn site FORCES the derived scale onto the actor after
  spawning it, exactly once. ⛔ Without it the engine substitutes the vendor template's scale and
  there is NO FOG (TASK-1071)' to be 1, but it was 0.   [SiegeFogVisualTest.cpp(950)]
Error: Expected '⭐⭐ THE CORRECTION HAPPENS BEFORE THE MEASUREMENT — a readback taken first would
  measure the engine's substitute and scream on every cast forever' to be 1, but it was 0.
                                                          [SiegeFogVisualTest.cpp(1012)]
Error: ⛔ Ordering marker 'Spawned->SetActorScale3D(RequestedScale3D);' is gone — the ordering probe
  is stale, so it FAILS.                                  [SiegeFogVisualTest.cpp(1013)]
```

⇒ **The fix is load-bearing and its guard is live.** Including the stale-marker `AddError` — the
ordering probe fails **loudly** rather than silently passing when its anchor disappears.

### MUTATION C (optional, run anyway — **and it was the most valuable one**)

`FogVisualScaleMatches`'s final return → `return true;` (detector always answers YES).
Compiles: `Result: Succeeded`. **RED: `495 / 1`.**
**Failing test:** `Siegebound.Fog.TheScaleReadbackRejectsTheEngineSubstitutionThatMadeTheFogInvisibleThreeTimes`

**Exactly the 4 `TestFalse` rows QA predicted**, and — precisely as predicted — **the 2 NaN rows
stayed green**, because the `ContainsNaN` short-circuit sits above the mutated return:

```
Error: Expected '⭐⭐⭐ THE ENGINE'S SUBSTITUTION IS REJECTED: the shipped detector says NO when
  handed the vendor template scale the engine really substituted (TASK-1071 §2). ⛔ A build where
  this is YES is a build with no fog and a green suite' to be false.   [line 872]
Error: Expected '⭐ …and a perturbation well outside it does NOT — the tolerance is a rounding
  allowance, not a blindfold' to be false.                             [line 892]
Error: Expected '⛔ A mismatch on Y ALONE is caught (a per-axis check, never a magnitude
  comparison)' to be false.                                            [line 894]
Error: Expected '⛔ A mismatch on Z ALONE is caught — Z is the axis the vendor template misses by
  the widest factor, and a short box is a fog ceiling nobody asked for' to be false. [line 903]
```

⭐ **Why I ran an "optional" mutation:** test 6 is the lane's only *genuinely executed* detector, and
**nobody had ever seen it say NO.** The file's own comment states the principle —
*"A detector nobody has ever seen say NO is not a detector."* It has now been seen. This is the
mutation that proves test 6 is not a green ornament.

### MUTATION B (optional) — `AchievedLocation.Z,` → `SpawnTransform.GetLocation().Z,`

Per **WARN-3**, the now-orphaned local `const FVector AchievedLocation` was deleted **in the same
mutation**. Compiles: `Result: Succeeded`. **RED: `495 / 1`**, same test as A.

```
Error: Expected '⭐⭐ …and the LOCATION too — the old line asserted a Z it had likewise never looked
  at, and half a readback is the same bug with better odds' to be true.        [line 969]
Error: Expected '⭐⭐ …and the LOCATION half of that lie is gone entirely: the log's Z comes from the
  ACTOR, never from the spawn transform' to be 0, but it was 1.                [line 976]
Error: Expected '⭐ …and the ACHIEVED Z does too' to be true.                   [line 985]
```

⚠️ **DECLARED DIFFERENCE FROM THE PREDICTION, REPORTED NOT SMOOTHED:** QA predicted **2** assertions;
**3** fired. The extra one (line 969) is caused by **my WARN-3-mandated deletion of the orphaned
local**, which is part of the mutation, **not** a property of the shipped diff. The prediction was
right about the diff; it simply did not count the remedy it had itself prescribed.

---

## 4. COMPILE — CLEAN, AND WHAT UBT ACTUALLY DID

The editor was **UP, PID 29128**, `L_Arena` **dirty in memory**. Closed under the standing grant
(killed by name ⇒ the in-memory state was **discarded**, never saved; **no save prompt was accepted**).

**`L_Arena` on-disk `sha256`, both sides of the close:**
`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — **matches the pin `1f78419d…5622`,
before and after.** `git status -- Content/Maps/` is **empty**. ⇒ never saved.

**Final clean build — the `[n/N]` list, read rather than assumed:**

```
[1/12] Compile [x64] SiegeFogVisualTest.cpp
[2/12] Compile [x64] FogVolume.cpp
[3/12] Compile [x64] Module.GitClaudeUnrealTest.11.cpp
[4/12] Compile [x64] Module.GitClaudeUnrealTest.9.cpp
[5/12] Compile [x64] Module.GitClaudeUnrealTest.12.cpp
[6/12] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[7/12] Compile [x64] Module.GitClaudeUnrealTest.7.cpp
[8/12] Compile [x64] Module.GitClaudeUnrealTest.5.cpp
[9/12] Compile [x64] Module.GitClaudeUnrealTest.6.cpp
[10/12] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[11/12] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[12/12] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
```

**Both subject translation units are in the list ⇒ the binary under test is the restored fix.**

**`Result: Succeeded`** — parsed from the log, **never** from `$LASTEXITCODE` (`Build.bat` returns
exit 0 on a failed build). **Zero warnings, zero errors.**

---

## 5. SUITE — BOUNDED, NOTHING SKIPPED (`SC-§87`)

Bounds **ARMED**: `1500` overall / `420` boot / `180` stall. **No test was skipped, filtered or
excluded**; the runner was the same one the `494` baseline used.

```
UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit"
  -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<scratchpad>/suite-clean.log
```

### EXECUTED: **`496 / 0`** · verdict **`EXITED`** at **37.9 s** — the bound never fired

| runner verdict field | value (**verbatim, uncurated**) |
|---|---|
| `VERDICT` | `EXITED` |
| `TEST_STARTED` | **496** |
| `TEST_COMPLETE` | **496** |
| `SUCCESS` | **496** |
| `FAIL` | **0** |
| `OTHER_RESULT` | 0 |
| **`N / M`** | **`496 / 0`** |
| `QueueEmpty` | **`NO`** |
| `TESTCOMPLETE` | `**** TEST COMPLETE. EXIT CODE: 0 ****` |
| `PROC_EXITCODE` | 0 |

**`QueueEmpty: NO` recorded uncurated** as `SHIP-§9f` / `TASK-1060` evidence — the same probe-wording
artefact `TASK-1056` and `TASK-1070` declared: this run **self-terminated** with an explicit
`EXIT CODE: 0` and never prints the phrase `Automation Test Queue Empty`. **It is not evidence of a
hang.** The decisive evidence is independent of it: **496 started = 496 completed = 496 Success,
0 Fail.**

**Delta:** baseline **`494 / 0`** (`TASK-1070`, executed at `c6bb376`) → **`496`**, **`+2`** = the two
new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in `SiegeFogVisualTest.cpp`. **No other movement.** Both new
tests `Result={Success}`. The author's and QA's `+2` was DECLARED-NOT-EXECUTED; **it is now EXECUTED
and correct.**

---

## 6. ⛔ MY OWN CLAUSE-6 READ-BACK RE-DERIVATION — **NOT** INHERITED FROM QA (`SC-§92` cl. 5)

I opened the sites myself.

- **Spawn argument** (`FogVolume.cpp:607`): `World->SpawnActor<AActor>(VisualClass, **SpawnTransform**, SpawnParams)`
- **Logged expressions** (`689-699`), in the order they appear on the line:

| Position | Logged expression | Derived from | Same symbol as the spawn arg? |
|---|---|---|---|
| 1st | `AchievedLocation.Z` | `Spawned->GetActorLocation()` (654) | ⛔ **NO** — read off the actor |
| 2nd–4th | `AchievedScale3D.{X,Y,Z}` | `Spawned->GetActorScale3D()` (653) | ⛔ **NO** — read off the actor |
| trailing | `RequestedScale3D.{X,Y,Z}` | `SpawnTransform.GetScale3D()` (620) | ✅ yes — but **explicitly labelled `(requested …)`** and it **trails** |

⇒ ⚖️ **MY VERDICT: the instrument is a MEASUREMENT, not an echo. `SC-§94` cl. A does not fire.**
The leading numbers are a function of the world. The commit proceeds.

Corroborated by the mutation evidence rather than by reading alone: **mutation B — which makes the
log echo the request — goes RED.** That is the clause tested rather than asserted.

---

## 7. STAGING + COMMIT

**Staged BY NAMED PATH — never by directory, no `git add Source/`, no `-a`, no `reset`** (`SC-§77a`).

`Source/` set **derived from `handoffs/TASK-1072-programmer.md`'s WRITES list** (not from the board
row), and the dirty `Source/` set was read **twice** and was **identical** — exactly these three, so
**no fourth `Source/` path existed** and the module fence released:

1. `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp`
2. `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h`
3. `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp`

Pipeline records:

4. `.claude/pipeline/TASKBOARD.md`
5. `.claude/pipeline/CONVENTIONS.md`
6. `.claude/pipeline/handoffs/TASK-1071-artist-diagnosis.md`
7. `.claude/pipeline/handoffs/TASK-1072-programmer.md`
8. `.claude/pipeline/handoffs/TASK-1074-buildmaster.md` (this file)
9. `.claude/pipeline/qa/TASK-1073.md`

⛔ **NAMED-AND-LEFT, deliberately UNSTAGED:**
- 🧑 **`.claude/agents/qa-reviewer.md`** — modified in the tree, **left dirty. Jonathan has still not
  ruled** (`TASK-1035`).
- The other-lane test files (`SiegeFogVolumeTest.cpp`, `SiegeBrightSunTest.cpp`,
  `SiegeFogRetentionWiringTest.cpp`) were **absent** from the dirty set. Neither presence nor absence
  is a finding; recorded for the record.

⛔ No `--amend`, no revert, no `stash`/`reset`/`clean`, no `--no-verify`, **NO PUSH**.

---

## 8. ⚠️ THE LIVE RESIDUAL, CARRIED FORWARD TO `TASK-1075` — **READ THIS BEFORE RE-AUDITING ANYTHING**

**WARN-1, upheld and handed on:** the vendor `BP_FogArea` implements `ReceiveTick`, and this
read-back is **same-frame**. A **Tick-driven re-clobber on a later frame would still read clean.**
QA established that `BeginPlay` runs *inside* `SpawnActor` — i.e. **before** the correction — so the
`BeginPlay` window **is** covered. It is **specifically the Tick window that is open.**

⇒ 🚨 **IF `TASK-1075`'S PIXEL MEASUREMENT SHOWS NO CHANGE while the log prints a clean
`ACHIEVED (640, 360, 260)`, THE VENDOR `ReceiveTick` IS THE FIRST HYPOTHESIS — NOT THIS DIFF.**
Second hypothesis is the froxel grid. Do **not** spend a cycle re-auditing code that is correct;
the mismatch `Error`'s own "what to check" text at `FogVolume.cpp:672-673` already names it.

**Also carried:** ⚠️ **WARN-2 overrides the author's §8.5 pre-routing** — `ENABLE_NAN_DIAGNOSTIC` is
**0** in this Development build, so the pre-routed remedy *"if test 6 fails, delete the NaN rows"* is
predicated on a hazard that cannot fire. It did not arise (test 6 is green, and red-on-demand under
mutation C), but the standing instruction is: **if test 6 ever reds, route it — do NOT delete its
NaN rows.**

---

## 9. ⛔ WHAT THIS COMMIT DOES **NOT** PROVE — IN MY OWN VOICE

**Nothing here says Jonathan will see fog.** I compiled it, I made it red on demand three times, and
I ran 496 tests green. **Not one of those 496 tests spawns an actor.** *"The engine applies the
scale at runtime"* remains, at this instant, **unobserved by anybody** — by the artist, by the
author, by QA, and by me.

I added exactly three things to the stack: that it **compiles**, that its guards **really go red**,
and that the suite **really is 496/0**. That is the rung I reach and no higher.

⇒ 🧑 **RUNG 4 IS `TASK-1075` (art-director), AND THAT ROW — NOT THIS COMMIT — CLOSES THIS LANE.**
A pixel measurement, zero-control first, in bursts, at his vantage `(-21580, -44, 201)` pitch `−15`,
against a noise floor **re-measured this time**. Three correct verdicts have already stacked into
three failed playtests in exactly this lane. The only thing that stops a fourth is somebody looking
at the screen.

**`TASK-1075` measures against the commit hash in §7.**
