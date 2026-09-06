# TASK-1104 — DEATH-CAM-SHIP — build-master handoff

**Commit `eeb29c4`** (parent `9daa641`) · main **6 AHEAD** of `origin/main`, ⛔ **NOT pushed**
**Marker:** `TASK-1104-DEATH-CAM-SHIP` · **Subject:** `TASK-1102` · **Gate:** `TASK-1103` PASS-WITH-WARNINGS (0 BLOCKER · 3 WARN · 5 NIT)
**Law:** `§25c` · `SC-§68` · `SC-§83` · `SC-§87` · `SC-§94` · `TL-§5e` · the `Build.bat` exit-code law

⚠️ **This file was born OUTSIDE its own commit** (`TL-§5e` cl. 7). `eeb29c4` was sealed before it
existed, so it is untracked at the instant of writing and needs a later doc host. Saying so is the
clause; it is not an oversight.

---

## 1. THE COMPILE — editor closed under the standing grant

| | |
|---|---|
| editor before | **PID 20564** (up since 08:24), killed by name, **nothing saved** — `L_Arena` never touched, every save prompt moot because the process was terminated |
| Smart App Control | `VerifiedAndReputablePolicyState = 0` ⇒ **not enforced**, no `0x800711C7` risk |
| editor after | **PID 21076**, relaunched detached with the `.uproject`; **MCP `127.0.0.1:8000` answering** (`list_toolsets` returned the full 19-toolset registry) |

```
Result: Succeeded
```

**Parsed from the log, never from `$LASTEXITCODE`** — which read `0` on every run including the four
mutated ones, exactly as the law says it would. Zero warnings, zero errors, 16.5 s.

The first build reported `Invalidating makefile for GitClaudeUnrealTestEditor (source file added)` and
compiled **all five subject translation units**, which is the proof the new pair actually entered the
build rather than being silently skipped:

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: HeroCharacter.cpp,
  SiegeDeathCameraStatics.cpp, SiegeGameMode.cpp, SiegeRespawnLifecycleTest.cpp
[1/11] Compile [x64] SiegeDeathCameraStatics.cpp
[2/11] Compile [x64] SiegeRespawnLifecycleTest.cpp
[4/11] Compile [x64] SiegeGameMode.cpp
[7/11] Compile [x64] HeroCharacter.cpp
[10/11] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
```

A final rebuild on the **restored** bytes (`[1/4] Compile [x64] SiegeGameMode.cpp`) also returned
`Result: Succeeded`, so the DLL the last suite ran against is the shipped fix, not a mutant.

---

## 2. THE SUITE — executed, bounded, and the inherited number was wrong

```
UnrealEditor-Cmd.exe "<uproject>" -ExecCmds="Automation RunTests Siegebound;Quit"
  -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=<scratchpad>/suite-final.log
```

Bounds **ARMED**: 900 s overall / 240 s boot / 180 s stall. Nothing filtered, skipped or excluded.

### `498 / 0` · verdict **`EXITED`** at **39.7 s** — no bound fired

| field | value (verbatim) |
|---|---|
| `VERDICT` | `EXITED` |
| `TEST_STARTED` | **498** |
| `TEST_COMPLETE` | **498** |
| `SUCCESS` | **498** |
| `FAIL` | **0** |
| `OTHER_RESULT` | 0 |
| **`N / M`** | **`498 / 0`** |
| `TESTCOMPLETE` | `**** TEST COMPLETE. EXIT CODE: 0 ****` |
| `PROC_EXITCODE` | 0 |

### 🚨 FINDING — the `306` baseline and the `308` prediction are a **stale static census**, not a pass count

The row, the handoff §7 and the gate's §6 item 4 all carry **306 inherited → 308 predicted**. That
number is **190 tests out of date** and it is not a pass count at all.
`handoffs/LADDER-REGRESSION-diagnosis.md` already established this: `306` originated at `TASK-805` as a
count of `^IMPLEMENT_` **macros on disk**, was written down as `306/306`, and was never an execution.

Measured here, both ways:

| instrument | value |
|---|---|
| `^IMPLEMENT_` macros across `Siegebound/Tests/*.cpp` (static census) | **498** |
| `Result={Success}` from an actual `Automation RunTests` (execution) | **498** |
| last genuinely **executed** baseline — `TASK-1074` at `c6bb376` | **496 / 0** |

⇒ `496 + 2 = 498`. **The arithmetic in the handoff was right; the base it was added to was wrong.**
The census and the execution agree today, which is the reconciliation `SC-§94` wants — but they agree
by coincidence of a healthy suite, and the two must never again be quoted as one number.

### Gate item 6 — both new tests **registered** and green

```
Test Completed. Result={Success} Name={ResetHeroReturnsControlRotationRollToZero}
  Path={Siegebound.RespawnLifecycle.ResetHeroReturnsControlRotationRollToZero}
Test Completed. Result={Success} Name={TheDeathPathHandsTheControllerAYawOnlyRotation}
  Path={Siegebound.RespawnLifecycle.TheDeathPathHandsTheControllerAYawOnlyRotation}
```

Both also appear in the enumerated test list at log lines 2733/2735, so they registered rather than
merely being absent-and-uncounted. `+2` confirmed; a `497` would have meant one failed to register.

### Gate item 8 — the cross-file regression, proven rather than derived

`Siegebound.HeroLadderClimb.ExitH6_RespawnAbortsBeforeItWritesWalking` reads the **same** `ResetHero()`
body this diff edits. The gate derived (E9) that it stays green because the new block lands after both
indices it asserts on. **Executed:** `Result={Success}`. ⭐ The gate's derivation was correct and is now
an observation.

---

## 3. THE FOUR MUTATIONS (`SC-§83`) — all four WITNESSED RED

`TASK-1102` declared **NO WITNESSED RED** because its own row forbade it to compile. All four are now
executed. Each was compiled (`Result: Succeeded` every time) and run through the full bounded suite.

**Method (`SC-§68`):** a `sha256` of all five subject files taken **before** any mutation, byte-exact
scratch copies alongside it, mutation applied by a helper that **refuses unless the target string
occurs exactly once**, and restore by copying the scratch file back — ⛔ **never `git restore`**, the
diff was uncommitted.

| # | edit | expected (gate's re-derivation) | **ACTUAL** | match |
|---|---|---|---|---|
| **M1** | `LevelViewRoll` → `return CurrentViewRotation;` | test 11 RED, 2 of 3 `CheckRotator` rows, printing `roll 89.9000` **and** `roll -89.9000` | test 11 RED, **`497 / 1`**, rows `(b1)` printing `roll 89.9000` and `(b2)`-negative printing `roll -89.9000` | ✅ **exact** |
| **M2** | `MakeDeathViewRotation` → `return DeadPawnRotation;` | test 10 RED, **exactly 2** rows — `(a1)` and `(a3)`-negative; `(a2)` and `(a3)`-upright GREEN | test 10 RED, **`497 / 1`**, exactly 2 rows: `(a1)` and `(a3)`-negative | ✅ **exact** |
| **M3** | delete the `if (AController* const OwningControllerForCamera = …)` block, keep the comment | test 11 RED on the **two `(b3)` census rows only**, behaviour rows green | test 11 RED, **`497 / 1`**, exactly the two `(b3)` rows (`test:1089`, `test:1096`) | ✅ **exact** |
| **M4** ⭐ *(gate-added)* | `SiegeGameMode.cpp:868` → `const FRotator GhostRotation = DeadHero->GetActorRotation();` | test 10 RED on its **census half only** — `(a4)`; `(a1)`/`(a2)`/`(a3)`/`(a5)` green | test 10 RED, **`497 / 1`**, exactly one row: `(a4)` at `test:1002` | ✅ **exact** |

**Verbatim reds** (timestamps and the repeated path stripped):

```
M1  Expected '(b1) the MEASURED rolled camera (roll 89.9) is levelled, and the player's look
    direction is untouched — got (pitch -17.5000, yaw 133.2500, roll 89.9000),
    expected (pitch -17.5000, yaw 133.2500, roll 0.0000)' to be true.   [test:873]
    Expected '(b2) a NEGATIVE roll is levelled too — got (pitch -17.5000, yaw 133.2500,
    roll -89.9000), expected (pitch -17.5000, yaw 133.2500, roll 0.0000)' to be true. [test:873]

M2  Expected '(a1) a corpse rolled by the MEASURED 89.9° yields a yaw-only view rotation —
    got (pitch -17.5000, yaw 133.2500, roll 89.9000), expected (pitch 0.0000, yaw 133.2500,
    roll 0.0000)' to be true.                                            [test:873]
    Expected '(a3) a NEGATIVE roll is removed too — got (pitch 0.0000, yaw 133.2500,
    roll -89.9000), expected (pitch 0.0000, yaw 133.2500, roll 0.0000)' to be true. [test:873]

M3  Expected '(b3) 🚨 `ResetHero()` LEVELS THE CAMERA — it resolves a controller and writes back
    FSiegeDeathCameraStatics::LevelViewRoll of its current control rotation. …' to be true. [test:1089]
    Expected '(b3) …and it WRITES the levelled value back through that controller — computing it
    and dropping it would be the SC-§36.1 zero-caller failure in miniature' to be true. [test:1096]

M4  Expected '(a4) ⛔ GhostRotation is PRODUCED BY the rule — the dead pawn's rotation reaches
    nothing until it has been through MakeDeathViewRotation' to be true.  [test:1002]
```

### ⭐ The gate's WARN-3 was right and the handoff was wrong — confirmed by execution, not by argument

- **M1:** the handoff said the two rows print "each `roll 89.9000`". They do **not** — the second prints
  **`roll -89.9000`**. The gate caught this by hand; the log agrees with the gate.
- **M2:** the handoff said "the (a1) and (a3) rows fail". There are **two** `(a3)` rows and only the
  negative one failed; `(a3)`-upright and `(a2)` stayed **green**, because the mutation is an identity
  on a yaw-only input. Exactly the gate's correction.
- **M4** did not exist in the handoff at all. It is the **only** mutation that reddens half **(a)**'s
  call site: M1–M3 would all have passed on a build where `SiegeGameMode.cpp:868` had never been
  rewired, leaving half (a) a landed *file* rather than a landed *fix*. ⭐ Worth keeping: three
  carefully-chosen mutations still left the primary fix's call site unproven, and the gap was found by
  a role that could not run any of them.

### Restore proof (`SC-§68` — hash, never size)

Every file byte-identical to its pre-mutation state, verified after the last restore:

```
1391a87db84882136cf4f3e564af3aa04df6bf9b4c2b5d9b51619a13fec9bcda  SiegeDeathCameraStatics.h
acb223136be3f8205ef72ef3dbd679accf5417174865a8765b734b76d8c6ba32  SiegeDeathCameraStatics.cpp
3f60416e73e37e4539b66f36326b7f178b2621da9f15d81fc8e2a478cf433d09  SiegeGameMode.cpp
55ce13f38ab3b472c802e5ca58b602c92866bf15ef494474692c28fa14a61bfd  HeroCharacter.cpp
bb0dce26b294daf4f83d2b56cf226dd2248fd12947833cded7d6095782bc9ed3  Tests/SiegeRespawnLifecycleTest.cpp
```

`diff` of the BEFORE and AFTER hash manifests: **identical, all five**. The mutant hashes were
`08b9927e…` (M1), `0fcd992c…` (M2), `a78af2df…` (M3), `43970e46…` (M4) — recorded so a future reader
can tell a restore happened rather than trusting that it did.

---

## 4. THE COMMIT — explicit pathspec, verified after (`§25c`)

Index was **empty** before I started and **empty** after the commit — no editor-provider stray this
time (the editor was closed for the whole window, which is why). The four new files needed an explicit
`git add -- <4 paths>` first, because `git commit -- <paths>` reaches only tracked files; ⛔ no `add -A`,
no `add .`, no `commit -a`.

```
git commit -F <msg> -- \
  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.h \
  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeDeathCameraStatics.cpp \
  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp \
  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp \
  GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp \
  GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1102-programmer.md \
  GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1103.md \
  GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md \
  GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md
```

`git show --stat HEAD`, pasted:

```
 .../.claude/pipeline/CONVENTIONS.md                |  23 ++
 GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md  | 195 +++++++++++-
 .../pipeline/handoffs/TASK-1102-programmer.md      | 329 +++++++++++++++++++++
 .../.claude/pipeline/qa/TASK-1103.md               | 161 ++++++++++
 .../Siegebound/HeroCharacter.cpp                   |  33 +++
 .../Siegebound/SiegeDeathCameraStatics.cpp         |  33 +++
 .../Siegebound/SiegeDeathCameraStatics.h           | 130 ++++++++
 .../Siegebound/SiegeGameMode.cpp                   |  36 ++-
 .../Siegebound/Tests/SiegeRespawnLifecycleTest.cpp | 275 +++++++++++++++++
 9 files changed, 1204 insertions(+), 11 deletions(-)
```

**Nine paths asked for, nine paths in the commit, zero strays.** No `Content/**`, no `Tools/**`, no
`L_Arena.umap`. ⛔ **Not pushed.**

### Gate item 7 — the tree half of scope, which QA could not check

Confirmed from `git status --porcelain` and `git diff --stat` at my instant:

- `TASK-1102` modified **exactly** the five `Source/**` files its handoff §10 named (`HeroCharacter.cpp`
  +33, `SiegeGameMode.cpp` +36/−1, `Tests/SiegeRespawnLifecycleTest.cpp` +275, plus the two new files)
  and its own handoff. Nothing else.
- **Zero `Content/**` paths moved.** The author's claim holds in the tree, not merely in the text.
- The foreign dirt he declared is **untouched by him** and, where it is not mine either, left alone.

---

## 5. WHAT STAYED DIRTY, AND WHOSE

| path | owner | why it is not in `eeb29c4` |
|---|---|---|
| `Content/RawAssets/MainCharacter.fbx` | ⭐ `TASK-1101` | the manager pinned its **deletion** to that row **by name**; I do not touch it |
| `handoffs/TASK-1085-buildmaster.md` · `TASK-1094-buildmaster.md` · `TASK-1098-buildmaster.md` | other hosts | other lanes' records, explicitly fenced out of my pathspec |
| `handoffs/TASK-1099-artist.md` · `TREE-DARK-diagnosis.md` · `TREE-DARK-diagnosis-2.md` | tree-LOD lane | ditto |
| `playtest-evidence/2026-09-06/TASK-1099-{FORCELOD-AUTO,FORCELOD0,SIDE-BY-SIDE}-lane.png` | tree-LOD lane | ditto |
| `.claude/pipeline/TASKBOARD.md` | **mine, deliberately** | the two `status:` flips (this row + `TASK-1102`) had to be written **after** the commit, because both quote the hash. Two lines, dirty, for the next host — the house `done (commit <hash>)` convention makes this unavoidable |
| `handoffs/TASK-1104-buildmaster.md` (this file) | **mine** | `TL-§5e` cl. 7 — born outside its own commit |

I committed `CONVENTIONS.md` deliberately (+23) even though its content is largely the **tree lane's**
`FIELD-§7` and the `§25c` INSTANCE 2 record: the precedent is explicit — `e12907c` and `e37c899` both
carried the records pair, and `e12907c`'s own message calls it *"a records backstop for a day of
boarding that existed nowhere in git"*. Law that exists in no commit is the orphan the board fears more
than a lane-boundary smudge. **But see FOLLOW-UP (ii) — it does not land cleanly.**

---

## 6. FOLLOW-UPS FOR THE MANAGER (report, do not solve)

1. ⛔ **The unowned upstream — the gate's ruling 5 and §7, and it is the real defect behind this one.**
   Nothing in `Source/**` writes roll onto the hero capsule, and `RotationRate = (0, 500, 0)`
   (`GitClaudeUnrealTestCharacter.cpp:27`) means `PhysicsRotation` corrects **yaw only**, so any roll
   that lands is held forever. This ship makes the death path structurally incapable of *transporting*
   that roll; it explains nothing about where the ~90° comes from. Suspects are outside `Source/` — a
   Blueprint, the physics asset, an anim, or an engine path. The gate's probe recipe: set
   `Log LogGitClaudeUnrealTest Verbose` **FIRST** (an empty log reads as a zero offset — a false pass),
   then log `Hero->GetActorRotation().Roll` at `HeroCharacter.cpp:864` and `SiegeGameMode.cpp:780`.
   Fold in WARN-1's `RetireGhostFor:961` match-end residual rather than boarding it alone.
2. 🚨 **`CONVENTIONS.md`'s new `FIELD-§7` — shipped in `eeb29c4` — opens with an instruction to read
   `handoffs/TREE-DARK-diagnosis.md`, which is STILL UNTRACKED** and fenced out of every pathspec I was
   given. A law section in `HEAD` that orders the reader to a file `HEAD` does not contain is
   `TL-§5e`'s orphan shape pointed the other way. Either the tree lane's doc host commits those two
   diagnosis files, or the citation needs rewording. **Not mine to fix; not silently absorbable.**
3. ⭐ **`TL-§6` is still undischarged.** The bounded suite runner is *still* not a tracked tool — I
   re-typed it into the session scratchpad to satisfy `SC-§87`, and re-typing a guard from memory is
   precisely the signal that law names. It also cost real time here: my first two attempts silently ran
   **zero tests** because PowerShell split the `-ExecCmds` value on its spaces, and the engine cheerfully
   logged `Cmd: Automation`, `Ready to start automation`, and then sat idle until the boot bound fired.
   ⚠️ **A suite runner with a quoting bug reports `0 started / 0 failed`, which is indistinguishable from
   a green run to anything that only reads the FAIL count.** A tracked runner would have had this bug
   fixed once, in `bf0cd9e`'s era, instead of every session.
4. `TASK-1103`'s own NIT-1 (the `:867` vs `:868` off-by-one in the handoff and the `TASK-1102` board
   row) is now moot in code — `:868` is what shipped and what M4 targeted — but the board row still
   says `:833`/`:862`. Cosmetic; left as written history per the flip instruction.

---

## 7. WHAT THIS SHIP DOES **NOT** MEAN (`SC-§94`)

It compiles, the suite executes at `498 / 0`, and four mutations prove the tests can tell a landed fix
from a landed file. **None of that is a death in a real match.** No PIE session was run, no frame was
looked at, and neither new test spawns a world, a pawn or a controller — they exercise two pure
functions and read two source files. Half **(b)** in particular is **latent**: `ResetHero()`'s only
shipped caller already levels roll eleven lines later, so M3's red proves the *call site exists*, not
that any player benefits from it today.

The claim I can defend is exactly the gate's: *the death path is now structurally incapable of
transporting roll from the corpse into the control rotation or into the ghost's spawn rotation, and
`ResetHero()` now zeroes control-rotation roll while preserving pitch and yaw.* 🧑 **Only Jonathan
dying once and watching the ghost phase closes the loop.**
