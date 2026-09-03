# TASK-803 — [MERGE-1] gameplay-programmer: THE CROSS-TRACK TERM (`CONTACT-§14`)

**Status: `ready-for-qa`. ⛔ NOT compiled (TASK-805 owns the compile), ⛔ no editor, ⛔ no MCP, ⛔ no Git.**

**Variant: A (closed-loop look-ahead steering). ⛔ A2 REFUSED, with a reason — §2 below.**
**Suite: `301 → 306` (+5). Census: `^IMPLEMENT_` at column 0 across `Tests/*.cpp` = **306**, one macro form only (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`).**

---

## 0. THE DIFF, IN FULL — 5 files, `+1016 / −4`

| file | + | − | what |
|---|---|---|---|
| `SiegeLadderClimbStatics.h` | 95 | **0** | `SteerLookAheadUU` + `SteerDirection` declaration + the fence banner |
| `SiegeLadderClimbStatics.cpp` | 57 | **0** | `SteerDirection` definition, in its own attributed banner section |
| `HeroCharacter.cpp` | 71 | **2** | the swept swap · entry-offset log · arrival-pop log |
| `SummonedUnit.cpp` | 61 | **2** | the same three |
| `Tests/SiegeLadderClimbTest.cpp` | 732 | **0** | fixture + tests 19–23 |

### ⛔⛔ THE FOUR DELETED LINES, NAMED — because the dispatch asked me to stay additive and I did not, exactly four times

```
-  AddMovementInput(Direction, 1.f, /*bForce=*/ true);                                     ×2
-  SetActorLocation(FSiegeLadderClimbStatics::ArrivalTarget(LadderClimb), /*bSweep=*/false); ×2
```

- The two `AddMovementInput` lines are **the swap the spec mandates** (spec item 1). A swap cannot be an insertion.
- The two `SetActorLocation` lines are **semantically identical replacements**: `ArrivalTarget` is a pure function whose result is now hoisted into a `const FVector ArrivalWorld` so the arrival-pop log can report the delta, then passed to the same `SetActorLocation(..., /*bSweep=*/ false)`. ⭐ **One call to a pure function instead of two. ⛔ Zero behaviour change.** If QA wants that reverted to a strict insertion, the log line would have to call `ArrivalTarget` a second time — the same value, computed twice. I judged the hoist cleaner and declare it here rather than hiding it in a `+1016` diff.

**Everything else is an insertion.** ⛔ Nothing outside my five fenced files is touched — `git status` shows exactly those five. ⛔ `Tools/ArtPipeline/build_watchtower.py:137` `LADDER_OUTWARD_SHIFT = 10.0` is **verified still 10.0 and untouched** (`CONTACT-§14.4` — the revert this clause exists to prevent did not happen here).

---

## 1. `SteerDirection`, AS WRITTEN

`SiegeLadderClimbStatics.h:397` (declaration) · `SiegeLadderClimbStatics.cpp:207` (definition)

```cpp
static constexpr float SteerLookAheadUU = 150.f;
static FVector SteerDirection(const FSiegeLadderClimbState& State, const FVector& CurrentWorld);
```

```cpp
FVector FSiegeLadderClimbStatics::SteerDirection(const FSiegeLadderClimbState& State, const FVector& CurrentWorld)
{
    const FVector Along = ClimbDirection(State);          // ⛔ READ, never redefined

    if (Along.IsZero() || CurrentWorld.ContainsNaN())
    {
        return Along;                                     // identical refusal, identical value
    }

    const double AlongUU = FVector::DotProduct(CurrentWorld - State.Start, Along);

    const double AimUU = FMath::Clamp(AlongUU + static_cast<double>(SteerLookAheadUU),
        0.0, static_cast<double>(State.LengthUU));
    const FVector Aim = State.Start + Along * AimUU;

    const FVector Steer = (Aim - CurrentWorld).GetSafeNormal();

    return Steer.IsZero() ? Along : Steer;
}
```

**The two swapped call sites — and there are EXACTLY two:**

| driver | line | before | after |
|---|---|---|---|
| `HeroCharacter.cpp` | **2088** | `AddMovementInput(Direction, 1.f, true)` | `AddMovementInput(FSiegeLadderClimbStatics::SteerDirection(LadderClimb, Here), 1.f, true)` |
| `SummonedUnit.cpp` | **4052** | `AddMovementInput(Direction, 1.f, true)` | `AddMovementInput(FSiegeLadderClimbStatics::SteerDirection(LadderClimb, Here), 1.f, true)` |

*(The spec cited `:1867` / `:3993`. `SummonedUnit`'s number still holds pre-edit; `HeroCharacter`'s had already moved to `:2019` because TASK-790 inserted a camera-arm floor above it. Both are the same statement — the ONE swept `AddMovementInput` inside `if (ShouldSweep(...))` in `TickLadderClimb`.)*

### ⛔ THE `SteerLookAheadUU = 150` DERIVATION — bounded at BOTH ends, ⛔ not felt

Steering law: aim `L` uu ahead on the line, pawn `e` uu off it ⇒ cross component `−e/sqrt(L²+e²)` ⇒
`de/ds = −e/sqrt(L²+e²)`, closed form `s = F(e₀) − F(e)`, `F(u) = sqrt(L²+u²) − L·ln((L+sqrt(L²+u²))/u)`.
Linear while `e ≫ L`, exponential with constant `L` once `e ≪ L`.

- **CEILING — set by the fence, not by taste.** The deck-breach stretch is driven along `ClimbDirection` (may not change), so convergence has only the SWEPT stretch: hero `1236.93 − 296.86 = 940.07 uu` (binding — the unit's is 964.80). From `CONTACT-§14.2`'s worst admitted `±277.8 uu`: `L = 150` ⇒ **1.03 uu** left ⇒ **23× inside** the hero's 24.0 uu margin, **15× inside** `ArrivalToleranceUU`. `L ≈ 179` is the largest value still meeting a 10× margin; **150 sits 16 % under it.**
- **FLOOR — a real bound.** `L` must stay far above `ArrivalToleranceUU` (16). At `L = 16`, `e = 247`, the cross component is **0.998** ⇒ the pawn crabs sideways with no rise. **Test 20(d) asserts this floor** (`along component ≥ 0.40` at the worst entry; shipped value 0.475; the row fails for any `L ≤ ~121`).
- **⚠️ THE 1.03 IS A CONSERVATIVE BOUND, DECLARED.** It spends 940.07 uu of *path*; covering 940.07 uu *along the line* costs ~1,041 uu of path (converging costs ~8 % extra path). The suite measures the real residual at **~0.5 uu**. The bound errs toward MORE residual — the only safe direction.
- **⚠️ AND THE HONEST CAVEAT, IN THE HEADER TOO:** the arithmetic assumes the driver realises the commanded unit vector exactly. It does not — `AddMovementInput` goes through acceleration and `MaxFlySpeed`, so real convergence LAGS this bound. ⇒ these are the *steering law's* numbers, ⛔ not a promise about the movement component. **That is precisely what the instrumentation exists to measure in PIE.**

---

## 2. ⭐ VARIANT A, ⛔ NOT A2 — AND WHY, SINCE THE SPEC PREFERRED A2

The spec permitted A2 ("carry the entry cross-track error in `FSiegeLadderClimbState` and lerp it to zero") **"if you judge it safe"**. **I judge it NOT safe, for two reasons, and I would rather be argued out of this than have it pass silently:**

1. **⛔⛔ A2 IS OPEN-LOOP AND WOULD BECOME A SECOND SOURCE OF TRUTH ABOUT WHERE THE PAWN IS.** A seeded offset decayed by dead reckoning is only correct while nothing else moves the capsule. But the swept branch is *swept* — depenetration, a `MaxFlySpeed` clamp, a blocked sweep, or `Internal_AddMovementInput` dropping a frame all move the pawn by an amount the seeded offset never learns about. It would then steer to a phantom. ⭐ Variant A re-reads `GetActorLocation()` every frame, so it is **self-correcting by construction** — and it fails in exactly the direction the shipped watchdog already covers.
2. **A2 CANNOT BE SEEDED WITHOUT WIDENING THE PINNED API.** `Begin`'s parameters are the two SOCKETS, ⛔ not the pawn's location — `Begin(State, bDead, bAIFrozen, bSpellFrozen, FromWorld, ToWorld, ClimbSpeedUU, CapsuleHalfHeightUU)`. Seeding the entry error therefore needs either a signature change (which the existing suite pins at compile time, and which `TOWER-§8.4(B)` pins as law) or a **second** new function called from **two more** call sites — i.e. one function + four call sites, against the spec's "ONE additive pure function + TWO one-line driver edits".

**⭐ AND A2's ADVERTISED PRIZE IS DELIVERED ANYWAY.** A2 was offered because it makes `CONTACT-§14.3`'s arrival pop "disappear for free". **It disappears under A too**, for a better reason: the pop is not special-cased away, it is simply *not there*, because the error is ~0.5 uu by the time the pawn arrives. **Test 23 measures it: `277.8 uu → 0.5 uu`, a >500× reduction on the identical scenario.** See §4.

---

## 3. ⛔⛔ PROOF `ClimbDirection` IS BYTE-IDENTICAL AT ITS THREE OTHER READERS

**A. The function itself is untouched.** `git diff` reports **0 deleted lines** in `SiegeLadderClimbStatics.{h,cpp}`. `ClimbDirection`'s declaration (`.h:302`) and definition (`.cpp:102-108`) are character-for-character the shipped text. The new function is **appended between TASK-776's moved region and TASK-777's contact region**, under its own attribution banner, so both existing regions stay contiguous and diffable from the top of the file.

**B. Full call-site census** (`grep ClimbDirection|SteerDirection` over `Siegebound/*.{h,cpp}`):

| # | site | role | status |
|---|---|---|---|
| 1 | `SiegeLadderClimbStatics.cpp:149` | **READER 1** — `Advance`'s arrival dot test | ⛔ **UNCHANGED** |
| 2 | `HeroCharacter.cpp:2064` → used at `:2119` | **READER 2a** — the hero's deck-breach step | ⛔ **UNCHANGED** (`const FVector Direction` is still `ClimbDirection`, still `Here + Direction * StepUU`) |
| 3 | `SummonedUnit.cpp:4024` → used at `:4078` | **READER 2b** — the unit's deck-breach step | ⛔ **UNCHANGED** |
| 4 | `HeroCharacter.cpp:2148` | **READER 3** — `IsLadderClimbInputHeld`'s sustain sign test | ⛔ **UNCHANGED** |
| 5 | `HeroCharacter.cpp:1837` | ⭐ **A FOURTH READER THE SPEC DID NOT NAME** — the hold-to-climb *seed* in `BeginLadderClimb` | ⛔ **UNCHANGED** — and it *must* be: the seed exists to survive one frame until real input arrives, and a position-dependent vector there would be meaningless |
| 6 | `SiegeLadderClimbStatics.cpp:211` | **NEW** — read (⛔ not redefined) inside `SteerDirection` | added |
| 7 | `HeroCharacter.cpp:1879` · `SummonedUnit.cpp:3902` | **NEW** — log-only, inside the entry instrumentation | added |

`SteerDirection` is called at **exactly two** sites: `HeroCharacter.cpp:2088` and `SummonedUnit.cpp:4052`. ⛔ Nowhere else.

**C. The suite asserts it, and this is the first thing to check (test 19):**
- **19(a)** `ClimbDirection(State) == (End − Start).GetSafeNormal()` — an **exact** `FVector::operator==`, ⛔ not a tolerance, so a re-pointing that happened to be *close* still fails.
- **19(b1)** it returns the identical vector at 4 wildly different pawn positions — paired with **19(b2)**, a SELF-CHECK that `SteerDirection` **disagrees by >5° at exactly the 3 off-line probes**. ⛔ Without (b2), (b1) would pass on a `SteerDirection` that did nothing.
- **19(c) READER 1, and this is the row that would flip.** A pawn level with the top but 200 uu off-line: against the LINE `dot(ToTop, Dir) == 0 ⇒ ARRIVED`; against the STEER the dot is `+200 ⇒ NOT arrived`. The test asserts arrival **and** self-checks that the two answers genuinely disagree at that probe. **A re-pointed `Advance` goes red here.**
- **19(d) READER 3.** Ascent and descent horizontal bearings dot to `< −0.99` — the constant-bearing property the sign test is built on, which a position-dependent vector would not have.
- **19(e) READER 2 IS A DECLARED DIFF READ, ⛔ NOT COVERED.** Neither driver can be instantiated headlessly (the teardown dereferences `GetWorld()` unconditionally — this test file's own opening block measures it). **I am not claiming coverage I do not have.** What the row *does* assert is that the two directions are **distinguishable at the exact position that step runs at**, so QA's diff read is a real check rather than a rename hunt. **⇒ QA must eyeball `HeroCharacter.cpp:2119` and `SummonedUnit.cpp:4078` and confirm both still read `Direction`.**
- **Two `static_assert`s** pin both signatures at compile time with messages naming `CONTACT-§14.5`. Giving `ClimbDirection` a position parameter — the single most likely way to breach the fence — becomes a **compile error in this module**, not a silent semantic change.

---

## 4. HOW THE ARRIVAL POP WAS HANDLED

**⛔ Not by touching the snap.** `ArrivalTarget` still returns `State.End` and the snap is still `bSweep = false` — both correct and both required (`TOWER-§8.5a` clause 5: the arrival snap is what puts the pawn's feet ON the deck rather than wherever the frame's step landed).

**⭐ The pop is nulled UPSTREAM.** The pop is `|End − Here|` on the cross-track axis, and the cross-track error is ~0.5 uu by the time `Advance` reports arrival. **Test 23** simulates the full shipped driver composition (`ShouldSweep` picks the branch — ⛔ nothing re-implemented) from the worst admitted entry offset and measures:

| | cross-track pop at the unswept snap |
|---|---|
| ⛔ pre-TASK-803 (`ClimbDirection` on both branches) | **277.8 uu, sideways, in one frame** |
| ⭐ with the cross-track term | **~0.5 uu** |

Test 23 also asserts the **total horizontal** displacement against a **derived** bound (`ArrivalToleranceUU × cos 76° = 3.88 uu` of legitimate along-track remainder — which a dead-centre climb also pays and which is ⛔ not the defect — plus 2 uu), and that the two figures disagree by **>50×**.

**⚠️ Declared residual:** the pop is not *zero*, and it cannot be — the snap always closes up to `ArrivalToleranceUU` of along-track remainder. What is gone is the **lateral** component, which is the whole of `CONTACT-§14.3`.

---

## 5. THE INSTRUMENTATION (what TASK-805 runs)

⛔ **All `Verbose` on `LogGitClaudeUnrealTest`. ⛔ Never Warning, ⛔ never on-screen, ⛔ never shipping-visible.** Every local in every block is read by its log line and by **nothing else** — deleting all four blocks changes **no behaviour**.

**(a) ENTRY CROSS-TRACK OFFSET** — `HeroCharacter.cpp:1877-1885` · `SummonedUnit.cpp:3900-3908`, inside **both** `BeginLadderClimb`s.

```
'<pawn>': ladder climb ENTRY OFFSET — cross-track +XX.XX uu (signed, across the ladder's clear
opening: 0 = dead centre, the stiles are at ±66.0, this capsule's side margin is 24.0/32.0).
Entry <vec>, line start <vec>.
```

- **⛔ ONE sample, and the board's original row is WITHDRAWN rather than shipped short.** "Y at 3–4 points up the line" returns the **identical number four times** — `CONTACT-§14.1` proves `Y(top) == Y(entry)` exactly, five ways. **4 samples, 1 bit.** The other end of the signal is the arrival-pop line.
- **⚠️ MEASURED AS A SIGNED PERPENDICULAR AGAINST THE CLIMB LINE, ⛔ NOT AS A RAW WORLD `Y` — and this is a deliberate divergence from the spec's wording.** The castles ship **rotated 90°**, so world `Y` is not tower-local `Y` in general; the line's own horizontal normal (`Cross(Up, ClimbDirection)`) is correct in every frame and for any future ladder. **For the shipped watchtower the two coincide exactly**, which is what `§14.1` asserts — so TASK-805 reads the number `§14.2` predicts either way. The raw entry vector and line start are logged alongside, so nothing is lost.

**(b) ARRIVAL POP DELTA** — `HeroCharacter.cpp:2048-2051` · `SummonedUnit.cpp:4007-4010`, at the two unswept snaps, **measured BEFORE the snap happens**.

```
'<pawn>': ladder climb ARRIVAL POP — horizontal X.XXX uu (this is the residual CROSS-TRACK error),
|ΔY(world)| X.XXX uu, total X.XXX uu. From <vec> to <vec>, UNSWEPT.
```

Both the spec's literal `|ΔY(world)|` **and** the frame-independent horizontal magnitude are reported. **Reading it: under ~1 uu = converged; tens or hundreds = `CONTACT-§14.2`'s band arriving intact.**

**(c) ⭐ THE FREE CONTACT-vs-LINK DISCRIMINATOR — ⛔ ZERO new lines, and it is BETTER than the one the dispatch described.**
`AClimbableTower::TryBeginContactClimb` logs `"CONTACT climb started"` at `ClimbableTower.cpp:884-886` **immediately after `BeginLadderClimb` returns true**; the ordered nav-link path logs **nothing** on start. Since my `ENTRY OFFSET` line is emitted *inside* `BeginLadderClimb`, the ordering is deterministic:

> **`ENTRY OFFSET` followed by `CONTACT climb started` ⇒ the CONTACT path.**
> **`ENTRY OFFSET` with no such follow-up ⇒ the ordered LINK path.**

⭐ This is stronger than "a `climb ended` with no preceding start line", because it discriminates **at the start**, per-climb, without waiting for the end. ⛔ **No log was added to `ClimbableTower.*`** (fenced) and none is needed.

---

## 6. THE TESTS — 5 new, and what each would catch

**Suite `301 → 306` (+5).** All in `Tests/SiegeLadderClimbTest.cpp`, appended under a TASK-803 banner. ⛔ **Tests 1–18 are untouched: 0 deleted lines in the file.**

| # | test | ⭐ what it would catch |
|---|---|---|
| **19** | `ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine` | **THE FENCE.** Any re-pointing of `ClimbDirection` (exact-bits row); a `SteerDirection` that ignores its position argument (b2); a re-pointed `Advance` (the arrival row FLIPS); a re-pointed sustain test (the ascent/descent bearing row); a `Begin` whose lift stopped being equal at both ends. Plus **2 `static_assert`s** that make a signature change a compile error. |
| **20** | `APawnAdmittedOffLineConvergesOntoItBeforeTheDeckBreachWindowOpens` | **THE FIX.** A steer that does not converge; one that oscillates (monotonicity row); one that converges too slowly to finish inside the swept stretch; one with too short a look-ahead (the **crab floor**, `along ≥ 0.40`); one that stalls or drives backwards. Runs the **hero** (binding case) and the **unit** (different lift, different window). |
| **21** | `APawnAlreadyOnTheLineClimbsExactlyAsItDidBeforeWithNoWobbleIntroduced` | **THE REGRESSION.** Any new lateral behaviour on a centred climb — sampled at **26 points** from foot to top so an end-clamp bug that only bites near the deck is caught. Paired with (b), which asserts the steer **does** bend at one side-margin off-line, so (a) cannot pass on a no-op function. |
| **22** | `TheCrossTrackSteerRefusesTheDegenerateLineExactlyAsClimbDirectionDoes` | **THE DOOR.** A widened `CanBegin`; a NaN or non-unit steer reaching `AddMovementInput` (a **56-probe grid** including behind the start, far past the top, ±5000 uu off-line); an aim point on the line's **backward extension** (would drive a low-admitted pawn *down*); a NaN pawn position propagating through `GetSafeNormal` (whose size test is `NaN < tol` = **false**). |
| **23** | `TheUnsweptArrivalSnapNoLongerTeleportsThePawnSidewaysByItsWholeEntryOffset` | **`CONTACT-§14.3`.** The lateral teleport returning; convergence bought with the **watchdog's budget** (extra path measured at ~8 %, bounded at 15 %, against a 300 % allowance); a fix that costs the climb its arrival (`bReachedTop` / `!bTimedOut` rows). |

### ⭐ EVERY ASSERTION CAN FAIL — the anti-triviality discipline, since six stopped discriminating this week

The specific triviality here is named and guarded: **a steering fix's test passes for free if the pawn was never off-line.**

1. **Explicit entry guards.** Test 20(a) asserts the pawn **starts** `277.8 uu` off-line — `>10× the hero's margin` — and that the figure is exactly `CONTACT-§14.2`'s, ⛔ not one invented in the test.
2. **A NEGATIVE CONTROL on every convergence claim.** Every scenario is run twice: once on `SteerDirection`, once on `ClimbDirection` **exactly as the drivers shipped before this task**. The control must come out **`277.8 uu`, unchanged** — i.e. the defect, reproduced on the real pre-fix code path. If the control ever converges, it has stopped being a control and every row it backs is void.
3. **Paired self-checks where a row could be vacuous.** 19(b2) (the two functions differ), 19(c) self-check (the two candidate directions genuinely disagree at that probe), 21(b) (the steer bends at one margin), 22's `IsNaN` guards (borrowed from `SiegeStuckStaticsTest.cpp:1350`), 20(e) (the unit's window really is smaller than the hero's), 23's `>50×` disagreement row.
4. **⛔ No march can pass by not running.** `SimulateClimb` returns `bHitStepCap`, and every caller asserts it is **false** — a stalled steer FAILS rather than silently truncating to a short, converged-looking march. The cap is derived from the shipped `TimeoutScale`.
5. **⛔ Nothing re-implements a shipped decision.** The branch choice is `ShouldSweep`; the arrival is `Advance`; the rate is read off the `ASummonedUnit` CDO (`LadderClimbSpeedUU`) so a retune re-derives every step length rather than quietly meaning something else; the step is `rate ÷ 60`.

---

## 7. ⚠️ WHAT QA SHOULD SCRUTINISE — my own list, hardest first

1. **⛔⛔ THE FENCE, AT READER 2 — the ONE reader my suite cannot cover.** Eyeball `HeroCharacter.cpp:2119` and `SummonedUnit.cpp:4078`: both must still read `SetActorLocation(Here + Direction * StepUU, /*bSweep=*/ false)` with `Direction` still `ClimbDirection`. Test 19(e) states this gap explicitly rather than papering over it.
2. **The four deleted lines** (§0). Two are the mandated swap; two are the `ArrivalTarget` hoist. Judge the hoist.
3. **⚠️ My divergence from the spec's wording on the entry log: signed cross-track against the line, ⛔ not raw world `Y`** (§5a). I believe rotated castles make raw world `Y` wrong in general and coincident here, but this is a judgement QA should confirm — the raw vectors are logged too, so nothing is lost either way.
4. **`SteerLookAheadUU = 150`.** Both bounds are in the header with the closed-form; the ceiling (~179) and floor (~121) are recomputable. **The `1.03 uu` figure is a declared conservative bound, not the expected value** (~0.5 uu measured).
5. **The convergence bound assumes the driver realises the commanded vector exactly.** It does not (acceleration, `MaxFlySpeed`, sweeps). **Declared in the header, in the handoff, and it is exactly why the instrumentation is shipped in the same diff.** ⛔ A green 306 does **not** prove a pawn converges in PIE — that is `TASK-805`'s pixel/log verdict (`SC-§32`, `AS-§6 A(e)`).
6. **Behaviour change I did NOT make and want on the record:** a pawn is now *steered* laterally by up to 0.88 of its speed on the first frames of a badly-off-line entry. It is **swept**, and the ladder has zero collision hulls, so it moves through open air — but this is a real, visible change to how an off-centre entry looks, and it is the intended one.
7. **⛔ NOT touched, each verified:** `LadderContactRadiusUU` / `IntentCos` / `DwellSeconds` (Jonathan's, `TASK-806`) · the sockets · the mesh · `build_watchtower.py` (`LADDER_OUTWARD_SHIFT` **still 10.0**) · `ClimbableTower.*` · `CONVENTIONS.md` · any `Content/` asset. ⛔ No compile, no editor, no MCP, no Git.
