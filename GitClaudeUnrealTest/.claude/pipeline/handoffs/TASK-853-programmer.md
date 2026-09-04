# TASK-853 — the ladder probe fix (gameplay-programmer)

**Date:** 2026-09-02 · **Status out:** `ready-for-qa` (gate = `TASK-854`)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp` — **SOLE**.
**Assets referenced:** ⛔ **NONE.**
⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `CONVENTIONS.md` edit · ⛔ no board edit but my own `status:` line.

---

## 0. ⭐ SUITE DELTA: **EXACTLY ZERO — stated explicitly** (`TL-§5b`)

**`+0`. ⛔ No test added, ⛔ removed or ⛔ renamed.** Measured, not asserted:

```
IMPLEMENT_ in Tests/SiegeLadderClimbTest.cpp   HEAD = 22   working tree = 22   DELTA = 0
```

⛔ **And per `TL-§5c` I am ⛔ NOT writing a pass count, because I did ⛔ not execute the suite.** A fresh census taken at the moment of writing, with its scope and file count (`TL-§5`/`TL-§5b`):

> **`378 declared across 29 files in Siegebound/Tests/*.cpp`.**

⚠️ **The `376` in my dispatch is already ⛔ stale by `+2`** — exactly the drift `TL-§5b` predicts with four lanes live in one tree. ⛔ It is ⛔ historical context, ⛔ not a target, and I have ⛔ reconciled ⛔ nothing to it. ⛔ My own contribution to that number is **`0`**.

---

## 1. ⭐ WHAT CHANGED — one substantive line, plus a comment, plus a rename

**The site (located by symbol, `SC-§38`):** the `PointOffLine(...)` probe in block **(c) READER 1** of `FSiegeLadderCrossTrackFenceTest`. (Now `:2225`; it was `:2212` before my comment shifted it — ⛔ the line number is a hint, ⛔ never a coordinate.)

```cpp
- const FVector LevelButOffLine  = PointOffLine(State, static_cast<double>(State.LengthUU), 200.0);
+ const FVector JustPastTopOffLine = PointOffLine(State, (State.End - State.Start).Size() + 1.0, 200.0);
```

- **Length re-derived in `double`** from `(State.End - State.Start).Size()` — ⛔ the `static_cast<double>(State.LengthUU)` at this site is **gone**.
- **`+ 1.0` uu margin** past the top, present **literally**, exactly as specified.
- **Naming rider (item 4) — I chose the RENAME, and I did both:** `LevelButOffLine` → **`JustPastTopOffLine`** (4 occurrences, all inside the one block), **and** a comment block explaining the float trap, the margin, and why the 200 uu offset is there. ⭐ I renamed rather than only commenting because `LevelButOffLine` asserts a falsehood about the value — and *believing the probe was level with the top* is the precise misconception that authored this bug. ⛔ The margin was never a candidate for weakening.

### ⛔ What I did NOT touch
- ⛔ **`SiegeLadderClimbStatics.{h,cpp}` — ZERO diff, verified by `git diff --numstat` (no output at all).** `Advance`'s `dot(ToTop, Direction) <= 0` arrival plane is **byte-identical**. ⛔ No epsilon, ⛔ no `KINDA_SMALL_NUMBER`, ⛔ no re-ordered OR, ⛔ not even a comment.
- ⛔ **Every assertion is byte-identical** — ⛔ none weakened, deleted, renamed or re-ordered; the two ORed arrival paths are untouched. **The `TEXT(...)` message strings are ⛔ unchanged**, deliberately: item (3) fences assertions, and a message string is part of one. ⚠️ **Declared for the gate:** the `(c) READER 1` message still reads *"a pawn level with the top"* while the probe now sits 1 uu past it. I judged an accurate **block comment** directly above (which I did write) the correct place to carry that, rather than editing assertion text under a "do not touch assertions" fence. ⭐ **If QA prefers the string updated, say so and I will — it is a one-word edit, ⛔ not a disagreement.**
- ⛔ `SummonedUnit.{h,cpp}` and `SiegeCombatStatics.{h,cpp}` (**`TASK-829`'s, live**) · ⛔ `HeroCharacter.{h,cpp}` (`TASK-828`'s) · ⛔ `Tests/SiegeHeroLadderClimbTest.cpp` · ⛔ `ClimbableTower.*`. **All verified ZERO DIFF** where they were mine to check; the three that are dirty were dirty on arrival and I ⛔ did not read-and-tidy them.

---

## 2. ⭐ I RE-DERIVED THE DIAGNOSIS RATHER THAN TRUSTING IT — and it holds

⚠️ **The whole reason this row exists is that a number was propagated without being executed.** So I reproduced the arithmetic independently (double + a real IEEE-754 float32 round-trip, UE's `GetSafeNormal`/`Clamp` semantics modelled) instead of quoting the table. **Confirmed to the digit:**

| quantity | diagnosis | **my reproduction** | |
|---|---|---|---|
| true `(End − Start).Size()` | `1236.9316876852981` | `1236.9316876852981` | ✅ |
| `State.LengthUU` (float32) | `1236.931640625` | `1236.931640625` | ✅ |
| error (float is **SHORT**) | `4.706029812950874e-05` | `4.706029812950874e-05` | ✅ |
| cross axis | exactly `(0,1,0)` | `(0.0, 1.0, 0.0)`, `dot(Cross,Dir) = 0.0` | ✅ |

**The four probe variants, `dot(ToTop, Direction)`:**

| probe | dot | passedTop | withinTol | **arrives?** |
|---|---|---|---|---|
| ⛔ shipped — `(double)State.LengthUU` | **`+4.706030e-05`** | ⛔ false | ⛔ false | ⛔ **NO — today's failure, both assertions, deterministically** |
| option A — true `.Size()` | `+0.000000e+00` | ✅ true | ⛔ false | ⚠️ yes, **on a last-bit tie** |
| ⭐ **B — true `.Size()` + 1.0** *(applied)* | **`-1.000000e+00`** | ✅ true | ⛔ false | ✅ **yes, unambiguously** |

⇒ the rejection of option A is **confirmed on my own numbers, not adopted on trust**: it arrives *only* through the `=` in `<=`. ⛔ Not mine to re-open, and I did not — I checked it and agree.

### ⚠️ ONE CORRECTION TO THE DIAGNOSIS — small, and worth recording precisely because of what this task is about

The diagnosis's option-B cell reads **`-9.999529e-01`**, and my dispatch repeated it as *"about `-0.99995`"*. **The recommended code it sits beside actually yields `-1.000000e+00`.** The difference is exactly the `4.706e-05` float error ⇒ **`-9.999529e-01` is what `L_float + 1.0` gives, ⛔ not what `(End - Start).Size() + 1.0` gives.** The prose and the snippet disagreed; **the snippet was right** and is what I implemented.

⛔ **Immaterial to every verdict** — both are ~4 orders of magnitude clear of the knife edge and identical in sign. ⭐ **Recorded anyway**: this is a fifth-decimal figure that travelled from a report into a dispatch prompt unchecked, which is the same shape (at harmless scale) as the `306/306` that travelled into three records unexecuted. ✅ **The applied value is `-1.000000e+00`.**

---

## 3. ⭐⭐ THE ROW IS ⛔ STILL A TRIPWIRE — the check I was told to make my own judgement on

**I was asked to stop and escalate if the margin made the assertion a tautology. ⛔ It does not — measured, three ways.** I re-ran block (c) against each way it is supposed to be able to fail:

| the defect the row exists to catch | dot under the margin | **row (c) still goes RED?** |
|---|---|---|
| `Advance` re-pointed at `SteerDirection` | `dot(ToTop, Steer) = +200.0025` ⇒ not arrived | ✅ **YES** |
| the arrival **PLANE** deleted, sphere kept | `\|ToTop\| = 200.0025` vs `ArrivalToleranceUU` 16 ⇒ not arrived | ✅ **YES** |
| `SteerDirection` collapsed into `ClimbDirection` | self-check dot `= -1.000000`, ⛔ not `> 1.0` | ✅ **YES — the self-check fails** |

**Item (3)'s required number, stated as a number: the `(c)` self-check at the `> 1.0` assertion reads `200.002500`** (it was `200.000000` before). ⛔ It did not become vacuous; it moved by 0.0025.

⭐ **AND THE MARGIN MAKES THE ROW STRICTLY STRONGER, which is more than tie-avoidance:** under option A the dot is `+0.0`, so arrival is carried entirely by the `=` branch of `<=`. Under the margin it arrives on a genuine **negative** — so the row now proves the plane fires on the *passed-the-top* side, ⛔ which is the low-frame-rate overshoot case the plane exists for. **Option A would have tested the boundary; option B tests the behaviour.**

---

## 4. ⛔ THE SIX SIBLING SITES — REPORTED, ⛔ CHANGED: **NONE** (item 5)

**All six verified present and unmodified in the working tree.** ⚠️ **Line numbers below are post-edit** (my comment shifted everything after `:2225` by +13); the diagnosis's `:2261/:2308/:2374/:2422/:2449/:2517` are the same six sites pre-edit.

| site | expression | verdict |
|---|---|---|
| `:2274` | `static_cast<double>(State.LengthUU - State.DeckBreachUU)` | ⛔ **leave** — window bound, compared against a `+50 uu` offset; 5e-05 swamped |
| `:2321` | `static_cast<double>(State.LengthUU - State.DeckBreachUU)` | ⛔ **leave** — a stretch length, same reasoning |
| `:2387` | `static_cast<double>(UnitState.LengthUU - UnitState.DeckBreachUU)` | ⛔ **leave** — same, on `UnitState` |
| `:2435` | `static_cast<double>(State.LengthUU) * (Index / SampleCount)` | ⛔⛔ **LEAVE — and it is ⛔ LOAD-BEARING, see below** |
| `:2462` | `static_cast<double>(State.LengthUU)` as a march target | ⛔ **leave** — swamped by a 5.83 uu step |
| `:2530` | `static_cast<double>(State.LengthUU)` in `AlongProbesUU[]` | ⛔⛔ **LEAVE — load-bearing for coverage, see below** |

⚠️ **Note for the record:** three of the six are `static_cast<double>(A - B)` — the subtraction happens in **float** and is *then* widened. The diagnosis described all six as `static_cast<double>(State.LengthUU)`; the line numbers are right, the shape differs at those three. Still harmless, for the stated reason.

### ⭐⭐ AND A FINDING THAT ⛔ INVERTS THE ADVICE AT TWO OF THEM — I measured this, it is not a guess

**A "consistency" sweep of the siblings would have turned a ⛔ CURRENTLY-GREEN row RED.** `SteerDirection` clamps its aim at `State.LengthUU` — **the float**. At `:2435`'s top sample the probe is built from that same float, so it lands **exactly** on the clamp, `Aim - Probe` is exactly zero, and the function takes its `Steer.IsZero()` fallback and returns the line. Rebuild that probe from the true `double` length and the probe sits `4.7e-05` **past** the clamp ⇒ the aim now points **backwards**:

```
:2435 top sample   AS SHIPPED (float)  Steer = ( 0.242535625, 0,  0.9701425)  Equals(Line,1e-5) = TRUE   -> row (a) PASSES
:2435 top sample   IF SWEPT   (double) Steer = (-0.242535625, 0, -0.9701425)  Equals(Line,1e-5) = FALSE  -> row (a) GOES RED
```

⛔ **Exactly reversed.** And `:2530` would silently stop exercising the `IsZero()` fallback branch it currently covers by accident (it keeps passing, so ⛔ nothing would have told anyone).

⭐ **THE RULE THAT SEPARATES MY SITE FROM THOSE TWO, and it is the one to carry forward: ⛔ derive a probe in the precision of the thing it will be COMPARED AGAINST.** `:2225` is measured against `State.End` and `ClimbDirection` — **doubles** ⇒ derive in double. `:2435`/`:2530` are measured against `SteerDirection`'s clamp at `State.LengthUU` — **a float** ⇒ they must stay float-derived. ⛔ **"Fix the float casts" is the wrong generalisation of this bug**, and the sweep item (5) forbade would have proved it the expensive way.

---

## 5. ⚠️ WHAT THIS FIX DOES ⛔ NOT DO — stated so it cannot be misread later

- ⛔ **It fixes a TEST. There was ⛔ never a gameplay defect here.** ⭐ **The ladder Jonathan playtested is fine.** The failing row never described a reachable state; if reached it clears in **one frame (16.7 ms)**.
- ⛔⛔ **IT DOES ⛔ NOT TOUCH THE UNTESTED-DESCENT DEBT.** Arrival is the **same plane test** whichever way the line is armed, so ⛔ nothing in this diagnosis or this fix is descent-specific. ⛔ **Jonathan's descent debt is ⛔ UNCHANGED — ⛔ neither worsened nor discharged — and ⛔ nothing in this handoff may be cited to close it.** It still owes a real PIE run.
- ⛔ **It proves nothing until something compiles.** ⛔ I ran no build (four lanes, one UBT module — the commit-gate build adopts this as `TASK-855`). ⭐ **This fix is verifiable ⛔ only by EXECUTION**, and the acceptance evidence is `TASK-855`'s `Result={Success}` on the named row — ⛔ **not my arithmetic, and ⛔ not QA's reading.** ⛔ Until then it is *unverified and expected-green*, ⛔ never "fixed".

---

## 6. 🔍 WHAT QA SHOULD SCRUTINISE

1. ⭐⭐ **`SiegeLadderClimbStatics.{h,cpp}` ZERO diff** — I claim it; ⛔ re-verify it independently. `<= 0` must be intact.
2. **The margin is literally `+ 1.0`** and the length comes from `(State.End - State.Start).Size()` — ⛔ no `static_cast<double>(State.LengthUU)` survives at that site.
3. **§3's tautology check** — ⛔ the number that matters is `200.002500`, and ⛔ that the row can still go red three ways. ⛔ Disbelieve my table if you like; it reproduces in ~40 lines of arithmetic.
4. ⭐ **§4's inverted finding** — if you think the siblings *should* have been swept, ⛔ read the `:2435` measurement first.
5. **The declared assertion-string judgement in §1** (*"level with the top"* left standing) — ⛔ my call under item (3), ⛔ flagged rather than hidden; overrule it freely.
6. **Suite delta `0`**, and that I have written **`declared`**, ⛔ never `N/N` (`TL-§5c`).
