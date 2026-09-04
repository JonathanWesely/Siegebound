# LADDER-REGRESSION — diagnosis (gameplay-programmer, DIAGNOSE-ONLY)

**Date:** 2026-09-02 · **Mode:** read-only (git read, source read, arithmetic reproduction). ⛔ No source changed, ⛔ no compile, ⛔ no editor/MCP, ⛔ no Git write, ⛔ no board row touched.
**Subject:** `Siegebound.LadderClimb.ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine` — 1 of 376, red under `TASK-824`'s build.

---

## ⭐ VERDICT IN ONE LINE

**It is not a regression. The test has been red since the line was written, and `TASK-824`'s build is the first time it has ever been executed.** The defect is in the **test's probe construction** — a `float`→`double` round-trip that places the probe **4.706 × 10⁻⁵ uu short of the top** — and ⛔ **not** in `Advance`, ⛔ not in the two dirty callers, ⛔ not in anything `TASK-824` touched. **There is no gameplay consequence: the worst real-world cost is one frame (16.7 ms), and the state is unreachable in play anyway.**

---

## 1. ⭐⭐ WAS IT RED AT HEAD TOO? — **YES. It was born red, and it had never run.**

This is the fork everything hangs off, and the prior "clean suite" claim does **not** survive being checked.

`git log -S` puts the birth of the failing line in exactly one commit:

```
bf0cd9e  TASK-805: "climbs off-centre" and "gets grabbed walking past" were ONE defect - suite 306
```

⇒ the row was **introduced by `bf0cd9e` itself** (`SiegeLadderClimbTest` went 17 → 22; this is one of the +5). So "did it pass before?" collapses to "did `bf0cd9e`'s suite actually run?"

### ⛔ THE "306/306" IS A **STATIC CENSUS**, NOT A PASS COUNT — measured, not assumed

`handoffs/TASK-805-buildmaster.md` §3 is titled "✅ THE SUITE — `306`". Every probe in it counts **macros on disk**:

| what §3 actually measured | value |
|---|---|
| `^IMPLEMENT_` across `Siegebound/Tests/*.cpp` | 306 |
| `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` | 306 |
| anchors table columns | **"declared" vs "on disk"** |

**Contrast `handoffs/TASK-802-buildmaster.md` (`8910c17`) — the commit immediately before — which genuinely ran it:**

| evidence | TASK-802 (`8910c17`) | TASK-805 (`bf0cd9e`) |
|---|---|---|
| `-ExecCmds="Automation RunTests Siegebound;Quit"` | ✅ recorded at §3 | ⛔ **absent** |
| `-nullrhi -unattended` | ✅ | ⛔ **absent** |
| `Result={Success}` count | ✅ **301** | ⛔ **absent** |
| `Result={Fail}` count | ✅ **0** | ⛔ **absent** |
| per-suite "executed" column | ✅ | ⛔ **absent — "on disk" only** |
| any log reference | build **and** suite | **build only** (its one `.log` cite is the `Result:` line at build-log line 42) |

A grep for `Result={` · `Test Completed` · `RunTests` · `ExecCmds` across the whole TASK-805 handoff returns **0**.

> ⇒ **`bf0cd9e` compiled the suite and never executed it.** The `306` in the commit message, in the handoff and in the session memory is *"306 test macros exist"*, reconciled declared-vs-on-disk — and the second `306` in "306/306" was **never measured**. `301/301` (TASK-802) was the last genuinely executed number, and it predates this test.

⭐ **This is the whole explanation of the paradox.** An unchanged test testing unchanged code "started" failing because it never ran before. Nothing changed. `TASK-824`'s build is simply the first suite execution since `8910c17`.

---

## 2. ⭐ IS THE TEST WRONG OR THE CODE WRONG? — **THE TEST. The code is correct.**

### The probe is misconstructed (`SiegeLadderClimbTest.cpp:2212`)

```cpp
const FVector LevelButOffLine = PointOffLine(State, static_cast<double>(State.LengthUU), 200.0);
```

`State.LengthUU` is a **`float`** (`SiegeLadderClimbStatics.cpp:71` — `static_cast<float>((State.End - State.Start).Size())`), while `Start`, `End` and the direction are all **`double`** (`FVector` is double-precision in UE5). Round-tripping the length through `float` loses the last ~5×10⁻⁵ uu, and it loses it in the **fatal direction**:

| quantity | value |
|---|---|
| true length `(End − Start).Size()` (double) | `1236.9316876852981` |
| `State.LengthUU` (float32) | `1236.931640625` |
| **error** | **`+4.706029812950874e-05` — the float is SHORT** |

### That error IS the arrival dot, exactly

The cross axis is `Up × Direction` normalized = **exactly `(0, 1, 0)`** for this geometry, so the 200 uu lateral offset contributes **exactly zero** to the dot. The algebra collapses to one term:

```
ToTop = End − Probe = Direction·(L_true − L_float) + (0, −200, 0)
dot(ToTop, Direction) = L_true − L_float = +4.706e−05
```

Reproduced numerically against the shipped constants (`LadderFoot(-450,0,0)`, `LadderTop(-150,0,1200)`, hero half-height 96 ⇒ `Start(-450,0,96)`, `End(-150,0,1296)`):

```
dot(ToTop, Direction) = +4.706029828496031e-05     ⇒ bPassedTheTop  (dot <= 0)    = FALSE
ToTop.SizeSquared     = 40000.0  vs 16² = 256      ⇒ bWithinTolerance             = FALSE
⇒ Advance returns TRUE  (still climbing)   ⇒ TestFalse at :2223 FAILS
⇒ bReachedTop = FALSE                      ⇒ TestTrue  at :2224 FAILS
```

**Both reported assertions, exactly, deterministically.** ⛔ Nothing stochastic, nothing environmental, nothing to do with the build.

### The author reasoned in exact arithmetic and built the probe in mixed precision

The comment at `:2208` states the intent: *"dot(ToTop, Line) == 0 ⇒ `<= 0` ⇒ ARRIVED (today's meaning)"*. That is true **only** if the probe sits at the true double length. It never did. The probe is not "level with the top" — it is **4.7×10⁻⁵ uu below it**, and `Advance` correctly answers "not yet".

⭐ **The 200 uu offset is not the cause — it is the reason the error is visible.** It only removes the second arrival path: with `ArrivalToleranceUU = 16`, any offset under 16 uu would have made `bWithinTolerance` true and masked the mistake. The row sits on a knife edge (`<= 0` at a value of `+5e-5`) and picked the one lateral distance that exposes it.

### ⛔ `Advance` is correct and should not be touched

Arrival on a **plane** (`dot(ToTop, Direction) <= 0`) rather than a sphere is the right design — it is what makes a low-frame-rate step that overshoots the top still register, which is the documented reason both tests are ORed (`:151`–`:157`). Changing `<= 0` to an epsilon-tolerant compare to make this row green would be **fixing correct code to satisfy a broken probe**.

---

## 3. ⭐ THE GAMEPLAY CONSEQUENCE (Jonathan's terms) — **NONE. The ladder is fine, ascending and descending.**

**Can a real pawn reach the failing state?** Effectively no, and it would not matter if it did.

**(a) It cannot get there.** `SteerDirection`'s cross-track term runs for the first **940.07 uu** of the hero's 1,236.93 uu climb (the deck-breach window is the last **296.86 uu** = `3 × 96 × L/1200`). Test 20 — which **passes** — asserts an off-line entry converges to sub-uu before that window opens. A pawn arriving at the top 200 uu off the line is not a state the shipped composition produces.

**(b) If it somehow got there, it arrives on the next frame.** Simulated at the shipped 300 uu/s, 60 Hz, driving line-only (the deck-breach window's steer):

```
frame 1: dot = +0.00005  -> not arrived
frame 2: dot = -4.99995  -> ARRIVED
```

⇒ worst case **one extra frame, 16.7 ms**. ⛔ Not stuck at the top, ⛔ not falling, ⛔ not climbing forever, ⛔ no watchdog trip (budget is 4× the climb's own duration).

**(c) It does not bear on the descent.** `Advance`'s arrival test is the **same plane test at `End`** whichever way the line was armed — a descent is `Begin(TopSocket → FootSocket)` and the arrival plane simply sits at the foot. `bDeckIsAtEnd` (`:82`) and `ShouldSweep` (`:127`) already resolve the deck by **Z, not argument order**, so the descent's non-swept window is at `Start` as intended. **Nothing in this finding is descent-specific, and nothing here is a reason to expect the untested descent to misbehave.** ⚠️ The descent remains genuinely untested in PIE — that debt is unchanged by this diagnosis, neither worsened nor discharged.

> ⭐ **For Jonathan: the ladder he playtested is not broken. This is a test that measured itself wrong, on a line of arithmetic, and had never been run to find out.**

---

## 4. THE TWO DIRTY CALLERS — **exonerated, measured**

`HeroCharacter.cpp` and `SummonedUnit.cpp` are dirty, but for `TASK-828` (`WITCH-§1`), not for anything on the ladder:

- Both diffs replace an inline `GetAllActorsWithInterface` + `Cast<ITeamAgent>` + team compare with `FSiegeCombatStatics::GatherHostileAgents` — the acquisition funnel. `HeroCharacter.cpp` = site 8 of 9 (+23/−?), `SummonedUnit.cpp` = sites 1 and 2 of 9.
- A grep of both diffs for `advance|climb|ladder|steer|direction` on changed lines returns **zero hits**. Neither `TickLadderClimb` nor any `Advance` call site appears in either diff.
- ⇒ they cannot have moved this row, and neither can `TASK-824` (3 lines in `SiegePlacementTest.cpp`, a TU defining nothing this test calls). The build-master's read is confirmed.

**Why the neighbouring 24-test `SiegeHeroLadderClimb` suite passes:** it lives in a different file with a different fixture — `SiegeHeroLadderClimbTest.cpp` contains **0 occurrences of `PointOffLine`** and never constructs this probe. It passes because it does not contain the defective line, ⛔ **not** because anything differs about the hero. The failure is one line in one file, not a suite-level condition.

---

## 5. RECOMMENDATION (⛔ NOT APPLIED — fenced; `SummonedUnit.{h,cpp}` / `SiegeCombatStatics.{h,cpp}` belong to `TASK-829`)

The fix is **one line, in the test, at `SiegeLadderClimbTest.cpp:2212`**. It touches no shipped source and no file `TASK-829` owns.

**Recommended — true double length plus a 1 uu margin past the top:**

```cpp
// `State.LengthUU` is a float32 of a double length: the round-trip lands 4.7e-5 uu SHORT of
// End, which is a POSITIVE arrival dot and reads as "not arrived". Re-derive in double, and
// place the probe a clear 1 uu PAST the top so the row tests a SIGN, not a last-bit tie.
const double TrueLengthUU = (State.End - State.Start).Size();
const FVector LevelButOffLine = PointOffLine(State, TrueLengthUU + 1.0, 200.0);
```

Verified numerically:

| variant | `dot(ToTop, Direction)` | arrives? | `(c)` self-check at `:2216` (needs > 1.0) |
|---|---|---|---|
| shipped — `(double)State.LengthUU` | **`+4.706e-05`** | ⛔ **no — today's failure** | 200.000 ✅ |
| option A — true double `.Size()` | `+0.000000e+00` | ✅ yes | 200.000 ✅ |
| ⭐ option B — `+ 1.0 uu` margin | **`-9.999529e-01`** | ✅ **yes, unambiguously** | 200.002 ✅ |

⚠️ **Option A alone is not recommended.** It happens to land on exactly `+0.0` for *this* geometry and passes via `<= 0`, but it is still a last-bit tie: any retune of the sockets or of a capsule half-height could flip it to `+1e-13` and the row would go red again for the same reason, having "already been fixed once". **Option B tests the sign the row actually cares about**, with margin.

⭐ **The test's intent is fully preserved either way** — against the LINE the dot is negative ⇒ arrived; against the STEER it is `+200` ⇒ not arrived. The `(c)` self-check at `:2216` still holds at 200.002, so the row remains a real tripwire for a re-pointed `Advance` and ⛔ does not become a tautology.

**Naming note for whoever applies it:** with the margin the local is 1 uu past the top, so `LevelButOffLine` is marginally a misnomer — worth a comment or a rename, ⛔ not worth weakening the margin.

### Latent siblings — one to watch, none currently red

`static_cast<double>(State.LengthUU)` appears at `:2261`, `:2308`, `:2374`, `:2422`, `:2449`, `:2517`. **`:2212` is the only one that feeds an exact sign test** — the rest are subtractions or bounds where a 5×10⁻⁵ error is swamped. `:2517` (`AlongProbesUU`) passes today; it is the only other one where a future exactness assumption could bite.

---

## 6. ⛔ WHAT THIS COSTS THE PIPELINE — the finding behind the finding

The suite was **not executed for two commits** (`bf0cd9e`, and by inheritance every board row built on it), while three separate records — the commit message, the handoff, and the session memory — read `306/306` as a pass. **A static census of `IMPLEMENT_` macros cannot fail a test**, and it was reported in a table headed "THE SUITE" directly beneath a `Result: Succeeded` build line, which is how it came to be read as one.

⭐ Suggested law, for the manager to phrase: **a suite number is only a pass count when it is backed by a `Result={Success}` / `Result={Fail}` pair from an actual `Automation RunTests` invocation.** A census reconciles *what exists*; only an execution reconciles *what works*. When only the census was taken, the number should be written `306 declared` — ⛔ never `306/306`.

---

## 7. Scope fence — held

⛔ No source file edited (not a line). ⛔ No compile. ⛔ No editor, no MCP. ⛔ No Git write — only `git log -S` / `git show` / `git diff` / `git status`. ⛔ No TASKBOARD row created or edited. The one file written is this report. `SummonedUnit.cpp` was **read only**, and left untouched for `TASK-829`.
