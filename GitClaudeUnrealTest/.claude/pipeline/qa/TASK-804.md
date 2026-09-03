# QA Report — TASK-804

**Gate over `TASK-803`** (the merged lateral-offset / abduction fix). Law: `CONTACT-§14`, esp. `§14.4` / `§14.5` — **cited, ⛔ not restated.**

**Verdict: PASS** — **0 BLOCKER · 4 WARN · 5 NIT.**
**Suite total for `TASK-805`: `306`** (measured on disk, ⛔ not by adding the declared delta — §9 below).
**`ClimbDirection` finding, stated explicitly as the board demands: ⛔ UNCHANGED in body and in meaning at EVERY reader. The fence HOLDS.**

⚠️ A previous `TASK-804` attempt died on a server error before doing any work. No stub existed at `qa/TASK-804.md`; this report is the first and only one.

---

## 0. WHAT I DID, AND WHAT I DID NOT

**Read at source** (⛔ not from the handoff's summary, ⛔ not from a diff that "looks additive"): `SiegeLadderClimbStatics.{h,cpp}` in full · both `TickLadderClimb`s · both `BeginLadderClimb`s · `IsLadderClimbInputHeld` · `ClimbableTower.cpp`'s contact-log region and socket defaults · the 5 new tests in full · the 4 pre-existing **source-text** tests that read the two edited function bodies · `build_watchtower.py:128-145` · `Config/` · the whole `Tests/` macro census.

**Re-derived independently** (⛔ nothing taken on the handoff's word): the closed-form steering law and BOTH look-ahead bounds · the swept-stretch lengths from `DeckBreachCapsuleHalfHeights` · the convergence residual · the extra-path cost · the outcome of **every** new numeric assertion.

⛔ **Fences honoured: no edits · no compile · no engine/MCP · no Git · no `TASKBOARD.md` write** (the dispatch withdrew the board flip from me — the orchestrator owns it; see §11).

---

## 1. ⛔⛔ THE FENCE — MY OWN READER CENSUS, RUN INDEPENDENTLY. **THE ANSWER IS FOUR.**

`TASK-803` found a fourth reader the spec (and the manager) had called three. The dispatch asked whether there is a **fifth**. **There is not**, and I closed the census on **two independent axes** rather than one.

### Axis 1 — by NAME
`ClimbDirection|SteerDirection` over `Source/**` = **94 occurrences across 6 files**, of which 61 are `Tests/SiegeLadderClimbTest.cpp` and 1 is a string literal in `Tests/SiegeHeroLadderClimbTest.cpp:923`. **Only FOUR non-test files contain either name**: `HeroCharacter.cpp` (8 lines), `SiegeLadderClimbStatics.h` (11), `SiegeLadderClimbStatics.cpp` (8), `SummonedUnit.cpp` (5). ⭐ **`ClimbableTower.{h,cpp}` contains ZERO** — which is also the proof for the "zero `ClimbableTower` edits" claim in §7.

| # | site | role | verdict |
|---|---|---|---|
| **1** | `SiegeLadderClimbStatics.cpp:149` | **READER 1** — `Advance`'s arrival dot | ⛔ **UNCHANGED.** `const FVector Direction = ClimbDirection(State);` feeding `bPassedTheTop` at `:158` |
| **2a** | `HeroCharacter.cpp:2064` → used `:2119` | **READER 2a** — hero deck-breach step | ⛔ **UNCHANGED** (eyeballed — §6) |
| **2b** | `SummonedUnit.cpp:4024` → used `:4078` | **READER 2b** — unit deck-breach step | ⛔ **UNCHANGED** (eyeballed — §6) |
| **3** | `HeroCharacter.cpp:2148` | **READER 3** — `IsLadderClimbInputHeld` sustain sign test | ⛔ **UNCHANGED** |
| **4** | `HeroCharacter.cpp:1837` | ⭐ **READER 4** — the hold-to-climb SEED in `BeginLadderClimb` | ⛔ **UNCHANGED**, and it must be: a position-dependent vector there would be meaningless (it exists to survive one frame until real input arrives) |
| 5 | `SiegeLadderClimbStatics.cpp:211` | **NEW** — inside `SteerDirection` | ⭐ **READ, ⛔ never redefined** |
| 6–7 | `HeroCharacter.cpp:1879` · `SummonedUnit.cpp:3902` | **NEW** — log-only | additive |

### Axis 2 — by EXPRESSION (the axis a rename would survive)
A name census cannot catch a site that *recomputes* the direction inline. I searched for `State.End - State.Start` / `LadderClimb.(End|Start)` / `…GetSafeNormal` across `Siegebound/`. **Result: the only places the line vector is formed are `ClimbDirection`'s own body (`:107`) and `Begin`'s length (`:71`).** The only other hit is TASK-803's own log arithmetic (`EntryWorld - LadderClimb.Start`), which is a *position* difference, not a direction. ⇒ **no shadow reader exists.**

### ⇒ THE CENSUS IS **FOUR**. ⛔ NO FIFTH.

**And `ClimbDirection` itself is byte-identical.** `SiegeLadderClimbStatics.cpp:102-108` is the shipped text — `return (State.End - State.Start).GetSafeNormal();` under its shipped comment; the declaration at `.h:302` is untouched; the new function is appended **between** TASK-776's moved region and TASK-777's contact region under its own attribution banner, so both existing regions stay contiguous. ⛔ **No new `#include` in either file** — the purity bar that makes the whole suite headless survives (`SiegeLadderClimbStatics.cpp` still includes exactly one header).

**`SteerDirection` reads `ClimbDirection` and ⛔ never redefines it** — `const FVector Along = ClimbDirection(State);` at `:211`, used and never reassigned. ✅

**`SteerDirection` is called at EXACTLY TWO sites** — `HeroCharacter.cpp:2088`, `SummonedUnit.cpp:4052`. I confirmed by a project-wide `AddMovementInput` census: the only two in the whole `Siegebound/` climb path are those two, both now `SteerDirection`, both inside `if (ShouldSweep(...))`, both followed by `return`. The only other `AddMovementInput` in `Siegebound/` is `SiegeGhostPawn.cpp:550-551` (unrelated).

⭐ **A structural guarantee the handoff did not claim and which I want on the record:** both swept branches `return` immediately (`:2089` / `:4053`), so **the swept call and the deck-breach step can never both run in one frame.** The swap is provably confined to the swept branch by control flow, not merely by inspection.

---

## 2. ⚖️ RULING ON THE FOUR DELETIONS — **ALL FOUR ALLOWED, EACH FOR A DIFFERENT REASON**

The dispatch fence said *additive*; the diff has exactly 4 deletions. The programmer declared all four rather than hiding them in `+1016`. That declaration is itself correct behaviour. My rulings:

**(a) The 2 mandated call-site swaps** (`HeroCharacter.cpp:2088`, `SummonedUnit.cpp:4052`) — ⛔ **A SWAP CANNOT BE AN INSERTION.** `CONTACT-§14.5` specifies "**TWO one-line driver edits** (each driver's one movement call)" — an *edit*, not an addition. The "additive" in the fence governs **`ClimbDirection`'s meaning**, ⛔ not a literal line count. ✅ **WITHIN SPEC.**

**(b) The 2 `ArrivalTarget` hoists** — ⛔ **RULED A PURE REFACTOR, AT SOURCE, on four checks:**
1. `ArrivalTarget` is pure and unconditional — `return State.End;` (`SiegeLadderClimbStatics.cpp:110-115`), untouched.
2. `LadderClimb` is **not written** between the hoist and the snap. The only statements in between are two `const` locals and one `UE_LOG`. (`HeroCharacter.cpp:2045→2053`, `SummonedUnit.cpp:4004→4012`.)
3. The snap still passes `/*bSweep=*/ false` and still sits **inside `if (bReachedTop)`** — `TOWER-§8.5a` clause 5 intact.
4. ⭐ **The pre-existing source-text guard still holds:** `SiegeHeroLadderClimbTest.cpp:541-546` asserts `ArrivalTarget(` appears **after** `if (bReachedTop)` in the hero's `TickLadderClimb`. Post-hoist the first `ArrivalTarget(` in that body is line 2045, after the guard at 2022. ✅ **Green.**

⇒ **Behaviour change: ZERO.** And the alternative — reverting to a "strict insertion" — would call a pure function **twice for the same value** purely to satisfy a line-count reading of the fence. ⛔ **I refuse that trade.** The hoist is the better code and it is correctly declared.

**4 deletions · 4 ruled · 0 objections.**

---

## 3. ⭐ RULING ON THE VARIANT — **A UPHELD OVER A2, and the elegant consequence is CONFIRMED INDEPENDENTLY**

The spec *preferred* A2. The programmer shipped **A** and asked to be argued out of it. I decline to argue: **A is the right call, and one of its two reasons is verifiable rather than rhetorical.**

**(i) The API argument is DECISIVE and I checked it.** `Begin`'s signature (`SiegeLadderClimbStatics.cpp:46-48`) is `Begin(State, bDead, bAIFrozen, bSpellFrozen, FromWorld, ToWorld, ClimbSpeedUU, CapsuleHalfHeightUU)` — the two **SOCKETS**, ⛔ never the pawn's position. Seeding an entry cross-track error therefore requires either widening a signature the existing suite pins at compile time and `TOWER-§8.4(B)` pins as law, **or** a second new function called from **two more** sites — i.e. 1 function + 4 call sites against the spec's "ONE function + TWO one-line edits". **A2 costs 2× the spec's stated shape.**

**(ii) The open-loop argument is sound.** The branch A2 would run on is *swept*: `MaxFlySpeed` clamping, depenetration, a blocked sweep, or `Internal_AddMovementInput` dropping a frame under `IsMoveInputIgnored()` all move the capsule by an amount a dead-reckoned offset never learns. It would steer to a phantom — **with no compile error and no test failure**, which is the identical failure class `§14.5` exists to forbid. Variant A re-reads `GetActorLocation()` every frame and is self-correcting by construction.

**(iii) ⭐⭐ AND A2's PRIZE LANDS ANYWAY — I RE-DERIVED IT RATHER THAN TRUSTING TEST 23.**
The pop is nulled **upstream**, ⛔ not by touching the snap: `ArrivalTarget` still returns `State.End` (`:114`) and **both snaps still pass `/*bSweep=*/ false`** — verified at source.
- Deficit integral (path spent sideways): `∫₀^{e₀} (√(L²+e²) − L)/e de` at `e₀ = 277.8`, `L = 150` ≈ **99.3 uu**.
- ⇒ covering the hero's `940.07 uu` swept stretch costs **`1039.4 uu` of path** (the handoff says "~1,041" — agrees).
- ⇒ `F(e) = F(277.8) − 1039.4 = 238.212 − 1039.4 = −801.2` ⇒ **`e ≈ 0.53 uu`** at the deck-breach window.

**⇒ `277.8 → ~0.5 uu`, a >500× reduction, WITHOUT special-casing the unswept snap. `CONTACT-§14.3` disappears as a side effect of climbing the line properly. CONFIRMED.**

⚠️ **The declared residual is honest and I endorse it:** the pop is not *zero* and cannot be — the snap always closes up to `ArrivalToleranceUU` of **along-track** remainder (`16 × cos 76° = 3.88 uu` of horizontal), which a dead-centre climb also pays. What is gone is the **lateral** component, which is the whole of `§14.3`.

---

## 4. ⭐ RULING ON `SteerLookAheadUU = 150.f` — **BOTH BOUNDS RE-DERIVED FROM SCRATCH. BOTH REPRODUCE.**

The dispatch was right to insist: *a look-ahead outside the bounds is a real defect, not a taste call.* So I recomputed both rather than reading them.

**The law.** Aim `L` ahead on the line, pawn `e` off it ⇒ cross component `−e/√(L²+e²)`, along component `L/√(L²+e²)` ⇒ `de/ds = −e/√(L²+e²)`, closed form `s = F(e₀) − F(e)` with `F(u) = √(L²+u²) − L·ln((L+√(L²+u²))/u)`. ✅ **The header's integral is correct** (standard form of `∫√(a²+x²)/x dx`).

**The swept stretch, re-derived** from `DeckBreachCapsuleHalfHeights = 3` (`.h:240`), the hero's `InitCapsuleSize(42.f, 96.0f)` (`GitClaudeUnrealTestCharacter.cpp:18`, and the driver reads it live at `HeroCharacter.cpp:1782-1784`) and `Begin`'s slope conversion (`:87-91`):
`|(300,0,1200)| = 1236.93` · hero `3×96×(1236.93/1200) = 296.86` ⇒ **`940.07`** · unit `3×88×1.030775 = 272.13` ⇒ **`964.80`**. ✅ Hero binds. Matches.

| bound | handoff | **my independent value** | verdict |
|---|---|---|---|
| residual at `L = 150` | 1.03 uu | **1.0253 uu** ⇒ **23.4×** inside the 24.0 margin | ✅ reproduces |
| **CEILING** — largest `L` still meeting 10× (2.40 uu) | ~179 | **`L = 179` ⇒ 2.383 uu** — the 10× margin **to three figures** | ✅ **REAL, correctly placed** |
| along-component at the worst entry | 0.475 | **0.4751** | ✅ |
| **FLOOR** — `L` where along-component hits 0.40 | ~121 | **`L = 121.24`** | ✅ **REAL** — test 20(d) genuinely fails for any `L ≤ 121.2` |
| crab check at `L = 16`, `e = 247` | 0.998 | **0.99791** | ✅ |

⭐ **AND THE PLACEMENT IS BETTER THAN "INSIDE THE WINDOW":** `√(121.24 × 179.0) = 147.3`. **150 sits within 2 % of the geometric centre of its own admissible window.** ⇒ ⛔ **not a felt number, and ⛔ not a defect.** ✅ **RULED SOUND.**

⚠️ *The `0.40` crab criterion is itself a judgement* — but a declared one, and the header anchors it independently in arithmetic (`L` must stay far above `ArrivalToleranceUU = 16`; at `L = 16` the pawn spends 99.8 % of its speed sideways). Accepted.

---

## 5. ⭐ THE TESTS — **EVERY NEW ASSERTION CAN GENUINELY FAIL. I HAND-EVALUATED EVERY NUMERIC ROW.**

The dispatch named the exact risk twice: *six assertions stopped discriminating this week*, and *a steering fix's test passes trivially if the pawn was never off-line.* So I did not read the test names — I computed what each row evaluates to.

### The two things the dispatch demanded, both PRESENT
- **"the pawn really started off-line" guard on every convergence row:** `20(a)` asserts `EntryCross > 10 × HeroSideMargin` (277.8 > 240) **and** that the figure is exactly `§14.2`'s, ⛔ not invented in the test. `20(e)` and `23` build from the same two named constants.
- **a control that must come out `277.8 uu` UNCHANGED:** ✅ **PRESENT TWICE.** `ESimSteer::LineOnly` is `ClimbDirection` on both branches — character-for-character the pre-fix path, ⛔ not a strawman — and `20(b)` / `23(b)` assert the control finishes at **`277.8 uu` to 1e-2**. If the control ever converges it has stopped being a control and the rows it backs are void. ⭐ **This is the single most important anti-triviality device in the batch and it is correctly built.**

### Test 19's flip row — ⭐ **IT GENUINELY FLIPS. I VERIFIED THE ARITHMETIC.**
Probe = `End + 200·CrossAxis`, so `ToTop = −200·CrossAxis`:
- against the **LINE**: `dot(ToTop, Line) = 0` ⇒ `<= 0` ⇒ **ARRIVED** (today's meaning) ✅
- against the **STEER**: the clamp collapses the aim onto `End`, so `Steer = −CrossAxis` ⇒ `dot = +200` ⇒ **NOT arrived**
- and `|ToTop| = 200 > ArrivalToleranceUU = 16`, so **the tolerance test cannot rescue it.**
⇒ **a re-pointed `Advance` turns this row RED, and it is the only row in the project that would.** The self-check at `:2216` (`dot > 1.0`) proves the two candidates disagree at that probe. ✅

### Row-by-row, with what I computed
| row | what I evaluated | fails if |
|---|---|---|
| 19(a) | exact `FVector::operator==`, ⛔ no tolerance, plus a re-derivation from the two SOCKETS | any re-pointing, even a close one; an unequal lift at the two ends |
| 19(b2) | along-components at the 4 probes = **1.0 / 0.4751 / 0.5188 / 0.7809** ⇒ exactly **3** exceed 5° | a `SteerDirection` that ignores its position argument scores **0** ⇒ RED |
| 19(c) | above | a re-pointed `Advance` |
| 19(d) | ascent vs descent horizontals dot to exactly **−1** | a position-dependent line vector |
| 19(e) | probe at `breach-start + 50` off-line 60: `Dist(End) = √(246.86² + 60²) = 254.0 < 296.86` ⇒ `ShouldSweep` **false** ✅ | a zero/empty deck-breach window; a `SteerDirection` indistinguishable at that position |
| 20(c) | residual **≈ 0.53 uu** against a `< 2.40` bound; `WorstCrossTrackIncrease ≤ 1e-6` is a real max over every step | a non-converging, oscillating or too-slow steer |
| 20(d) | **0.4751 ≥ 0.40**; unit-magnitude to 1e-4; `MinAlongAdvance = 2.77 uu > 0` on the first step | any `L ≤ 121.2`; a crabbing, stalling or backwards steer |
| 20(e) | unit residual **≈ 0.40 uu** against `< 3.20`; `UnitDeckBreach 272.13 < HeroDeckBreach 296.86` ✅ | the same, on a genuinely different window |
| 21(a) | 26 samples incl. **Index 25 = `End` exactly**, where the clamp collapses and the `IsZero` fallback returns `Along` ⇒ agreement holds **through the end-clamp** | an end-clamp bug that only bites near the deck |
| 21(b) | `atan(24/150) = **9.09°** > 5°` | a `SteerDirection` that returns the line unconditionally ⇒ (a) would pass vacuously without this |
| 22(c) | 56 probes; `BadSteers = 0` (incl. the `Aim == Probe` degenerate at `Along = LengthUU, Cross = 0`, which exercises the `IsZero` fallback); `BackwardsAims = 0` (at `Along = −900, Cross = −5000` ⇒ `dot = 900/5080 = +0.177`) | a NaN/non-unit steer; a lost backward-extension clamp |
| 22(c) NaN | fixture NaN self-checked with `FMath::IsNaN` **and** `ContainsNaN` before use | a "NaN" that is not one ⇒ the row would pass for the wrong reason |
| 23(c) | `FixedPop ≈ 0.53 < 2.0`; total horizontal `√(3.878² + 0.53²) = 3.914 < 3.88 + 2.0`; disagreement `277.8/0.53 = 524× > 50×` | the lateral teleport returning; two spellings of one number |
| 23(d) | `(227 − 213) / 213 = **6.6 %**` against a **15 %** bound and a **300 %** watchdog allowance | convergence bought with the watchdog's budget |

⭐ **Every row lands with ≥ 2× margin but ≤ 10× — they discriminate without being brittle.** Tightest: `23(d)` at 6.6 % vs 15 %.

### The two `static_assert`s
✅ **Both name `CONTACT-§14.5` in their message text** (`:1963`, `:1971`). ✅ `<type_traits>` **is included** (`SiegeLadderClimbTest.cpp:5`), so `std::is_same_v` compiles. ⇒ **giving `ClimbDirection` a position parameter — the single most likely way to breach the fence — becomes a compile error in this module with the law quoted in the diagnostic**, rather than a silent semantic change. ⭐ This is the correct instrument for a failure mode no runtime row can see.

### Other anti-triviality checks I confirmed
- `bHitStepCap` is asserted **false** at every `SimulateClimb` call site ⇒ ⛔ **no march can pass by not running.** The cap derives from the shipped `TimeoutScale = 4.f`.
- Nothing re-implements a shipped decision: the branch is `ShouldSweep`, the arrival is `Advance`, the rate reads off the `ASummonedUnit` CDO (`LadderClimbSpeedUU = 350.f`, `SummonedUnit.h:1132`), the step is `rate ÷ 60`.
- ⭐ `SimulateClimb` stops at `LengthUU − DeckBreachUU` (**along-track**) while shipped `ShouldSweep` measures **3-D distance from the deck end** — so with any residual offset the real driver sweeps slightly *longer* than the test allows. **The test is conservative in the safe direction.**

### ⛔ AND THE OTHER FAILURE MODE I WENT LOOKING FOR: **a pre-existing test the diff breaks**
A green 306 is worthless if the diff quietly reds an old row. There are **four** source-text tests that extract the two edited function bodies. I read all of them:
`SiegeHeroLadderClimbTest.cpp:530-548` (arrival snap guarded by `bReachedTop`) ✅ · `:584-609` (H-3 + `DoMove` capture ordering) ✅ · `:769-796` (H-8 + `/*bAIFrozen=*/ IsMatchOver()` + `IsRecalling()`) ✅ · `:992-999` (`GetScaledCapsuleHalfHeight()`) ✅ · `:894-930` (`SetMovementMode(MOVE_Flying)` ×1, `SetDefaultMovementMode()` ×1, and **`(d)` requires `FSiegeLadderClimbStatics::ClimbDirection(` to still appear in the hero's source** — it does, at four sites) ✅ · `:1393-1441` (watchdog arming/clearing) ✅.
⛔ **No test counts `UE_LOG`, `AddMovementInput`, or the hero's line count.** The new scoped `{ }` instrumentation blocks are brace-balanced and the new comment text contains no `{`, `}` or `*/` that could confuse the brace-matching/comment-stripping helpers. ⇒ **No pre-existing row is disturbed.**

---

## 6. ⛔⛔ THE ONE UNCOVERABLE SITE — **I DID THE EYEBALL. HERE IS WHAT I SAW.**

`TASK-803` declared reader 2 not testable headlessly (both drivers' teardowns dereference `GetWorld()` unconditionally) and said so **in the test itself** — `19(e)`'s row text calls it *"a DECLARED DIFF READ (TASK-804), ⛔ not covered here"*. ⭐ **Declaring an uncovered mechanism instead of faking coverage is exactly right (`SC-§32`), and it is why this gate had something real to do.**

**`HeroCharacter.cpp:2064` and `:2117-2119`, verbatim:**
```cpp
const FVector Direction = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);
...
const float StepUU = FMath::Max(LadderClimbResolvedSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU)
    * FMath::Max(DeltaSeconds, 0.f);
SetActorLocation(Here + Direction * StepUU, /*bSweep=*/ false);
```

**`SummonedUnit.cpp:4024` and `:4076-4078`, verbatim:**
```cpp
const FVector Direction = FSiegeLadderClimbStatics::ClimbDirection(LadderClimb);
...
const float StepUU = FMath::Max(LadderClimbSpeedUU, FSiegeLadderClimbStatics::MinClimbSpeedUU)
    * FMath::Max(DeltaSeconds, 0.f);
SetActorLocation(Here + Direction * StepUU, /*bSweep=*/ false);
```

**What I saw, stated as findings rather than as a rename hunt:**
1. Both still read **`Direction`**, and `Direction` is still **`ClimbDirection`**. ✅
2. ⭐ **`Direction` is declared `const` at both sites and assigned exactly once in each function** ⇒ it **cannot** be re-pointed between its declaration and its use, even by accident. That is a stronger guarantee than "I looked at both lines".
3. ⭐ **The swept branch `return`s** (`:2089` / `:4053`) ⇒ the swap and the deck-breach step are **mutually exclusive by control flow**. No frame runs both.
4. `StopMovementImmediately()` still precedes the step at both sites (`:2112-2115` / `:4071-4074`) — the two-drivers-fight guard is intact.
5. Both snaps are still `/*bSweep=*/ false`. ✅

⇒ **READER 2 IS INTACT AT BOTH DRIVERS.** ⚠️ It remains **uncovered by all 306 tests** — see `W-4`.

---

## 7. THE INSTRUMENTATION — VERIFIED, AND ITS ONE DIVERGENCE RULED

✅ **All four blocks are `UE_LOG(LogGitClaudeUnrealTest, Verbose, …)`.** ⛔ No `Warning`, ⛔ no `AddOnScreenDebugMessage`, ⛔ no `GEngine`, ⛔ nothing shipping-visible.
✅ **Placement is correct and load-bearing:**
- Entry logs sit **after `Begin` succeeded** (so `LadderClimb.Start` is armed) and before `return true` ⇒ ⛔ a refused climb cannot log one. Both are inside a scoped `{ }` so no local leaks.
- Arrival-pop logs sit inside `if (bReachedTop)` and **before** `SetActorLocation` ⇒ they measure **the pop**, not its aftermath. ✅ Exactly as the dispatch required.
✅ **Cost:** 3 vector ops per climb *start*, 2 per climb *arrival*. ⛔ **Nothing per tick.** Deleting all four blocks changes no behaviour.
✅ **The cross-track measurement is arithmetically clean:** the lift is pure `+Z` and `CrossAxis = Cross(UpVector, Along)` has `Z == 0` by construction ⇒ the capsule lift contributes **exactly zero** to `dot(Entry − Start, CrossAxis)`. The number is the real signed perpendicular.

**⚖️ RULING on the divergence from the spec's literal `|ΔY(world)|`: ALLOWED, and the programmer is right.** The castles ship **rotated 90°**, so world `Y` is not tower-local `Y` in general; the line's own horizontal normal is correct in every frame and for any future ladder, and for the shipped watchtower the two coincide **exactly** — which is precisely what `§14.1` asserts. ⇒ TASK-805 reads `§14.2`'s predicted number either way. ⭐ **And nothing is lost: the raw `EntryWorld` and `LadderClimb.Start` vectors are logged alongside, so the literal world-`Y` reading is recoverable by subtraction.** The arrival-pop line reports **both** `|ΔY(world)|` and the frame-independent horizontal magnitude.

**⚖️ RULING on the free contact-vs-link discriminator: VERIFIED AT SOURCE, and it is genuinely free.** `AClimbableTower::TryBeginContactClimb` logs `"CONTACT climb started"` at Verbose (`ClimbableTower.cpp:884-886`) **immediately after** the `BeginLadderClimb` that returned true; the `ENTRY OFFSET` line is emitted **inside** `BeginLadderClimb` ⇒ strictly earlier in the same call stack, deterministically. The ordered nav-link path logs nothing on start. ⇒ `ENTRY OFFSET` **+** `CONTACT climb started` = contact; `ENTRY OFFSET` alone = link. ⛔ **ZERO `ClimbableTower.*` edits** — confirmed independently (that file contains zero occurrences of either function name, and the log it leans on is pre-existing). ⭐ It **is** better than the "a `climb ended` with no preceding start" version the dispatch described: it discriminates at the START, per climb, without waiting for the end.

**⚖️ RULING on the WITHDRAWAL of the board's "Y at 3–4 points up the line" row: SUSTAINED — but ⛔ NOT on the reason given.** See `W-2`. Short form: *"4 samples, 1 bit"* is true of the **pre-fix** driver and is **false once TASK-803 is live** — post-fix those four samples would trace the convergence curve, not repeat one number. The withdrawal survives anyway, because the replacement (entry offset **+** arrival pop, the two ENDS of that curve) answers the question TASK-805 actually asks, and the board's own pixel row `(3)(d)` covers *where* convergence completes. ⇒ **withdrawal upheld on the replacement's sufficiency, and TASK-805 is instructed accordingly.**

---

## 8. ⛔⛔ `LADDER_OUTWARD_SHIFT` — **STILL `10.0`. ⛔ NOT REVERTED.**

`Tools/ArtPipeline/build_watchtower.py:137` reads **`LADDER_OUTWARD_SHIFT = 10.0`**. ✅
Per `§14.4` this is **exactly `0.000 uu` for `Y` — inert, ⛔ NOT "insufficient"** — and **measured-good on the standoff axis** (hero clearance `51.61875 → 61.32015`, `+5.32015` over the required 56). ⛔ **A revert would have been a FAIL, not a cleanup. It did not happen.**

**Corroborated downstream, ⛔ not just at the constant:** the shipped socket defaults `AClimbableTower::LadderFootDefaultRelative(-460.f, 0.f, 0.f)` / `LadderTopDefaultRelative(-160.f, 0.f, 1200.f)` (`ClimbableTower.cpp:101-102`) are the **post-shift integers**, unchanged. ✅ ⇒ **zero socket changes.**

**Zero tunable changes**, read at the exact lines the board names: `ClimbableTower.h:653` `LadderContactRadiusUU = 350.f` · `:669` `LadderContactIntentCos = 0.5f` · `:685` `LadderContactDwellSeconds = 0.35f`. ✅ ⛔ **`TASK-806` is untouched and remains Jonathan's.**

**Zero `Content/` and zero other `Tools/` changes** are implied by the five-file diff, and nothing in those five files reaches an asset.

---

## 9. THE SUITE — **ONE NUMBER FOR `TASK-805`: `306`**

⛔ **Computed on disk, ⛔ never by adding the declared delta, ⛔ never a raw tree count.**

- `^IMPLEMENT_` at column 0 across `Source/**/Siegebound/Tests/*.cpp` = **306** across **23** files.
- `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` at column 0 = **306**. ⇒ ⭐ **ONE macro form only** — ⛔ no `IMPLEMENT_COMPLEX_*` (which registers N cases per macro and would break a count-by-macro).
- The only other `IMPLEMENT_` anywhere in `Source/` is `IMPLEMENT_PRIMARY_GAME_MODULE` (`GitClaudeUnrealTest.cpp:8`) — correctly outside the census.
- ⛔ No test file exists outside `Siegebound/Tests/` (glob-verified).

**Anchors, all five checked:**

| anchor | expected | on disk |
|---|---|---|
| `SiegeHealthBarOcclusionTest` | 12 | **12** ✅ |
| `SiegeHeroCameraTest` | 10 | **10** ✅ |
| `SiegeClimbableTowerTest` | 14 | **14** ✅ |
| `SiegeHeroLadderClimbTest` | 24 | **24** ✅ |
| `SiegeLadderClimbTest` | 17 (baseline) | **22** = 17 + TASK-803's **5** ✅ |

**Reconciled two ways: `301 + 5 = 306`, and an independent whole-tree census = `306`.** ✅ The declared delta is honest.
⛔ **`TASK-805` asserts `306/306`. A red suite is a STOP, ⛔ not a note.**

---

## Findings

- **[WARN] W-1 — `GitClaudeUnrealTest.h:8` / `Config/` — ⛔⛔ THE VERBOSE LINES ARE COMPILED IN BUT **RUNTIME-SUPPRESSED BY DEFAULT**, AND NOTHING IN THE REPO RAISES THEM.** `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)` — compile-time `All` ✅ so the lines survive the build, but the **default runtime verbosity is `Log`**, and a grep of `Config/` for `LogGitClaudeUnrealTest` / `[Core.Log]` returns **ZERO matches**. ⇒ **`TASK-805`'s entire PIE measurement will print NOTHING unless it first runs `Log LogGitClaudeUnrealTest Verbose` in the PIE console (or launches with `-LogCmds="LogGitClaudeUnrealTest Verbose"`).** ⛔ **This is not TASK-803's defect** — the spec mandated Verbose and Verbose is the right level — but it is an **unstated precondition that would silently void the whole instrument**, and an empty log would be misread as "no offset". ⇒ **Suggested fix: none in code. `TASK-805` must raise the category as step 1 and must confirm a known-good Verbose line (`ladder climb STARTED`) appears before trusting any absence.**

- **[WARN] W-2 — `handoffs/TASK-803-programmer.md` §5(a) / `HeroCharacter.cpp:1857-1863` · `SummonedUnit.cpp:3886-3889` — the withdrawal of the board's "Y at 3–4 points" row is CORRECT, but its stated reason EXPIRES WITH THE FIX.** *"`§14.1` proves `Y(top) == Y(entry)` exactly ⇒ 4 samples, 1 bit"* is true of the **pre-fix** driver — the proof's whole content is *that there was no cross-track term*. **With TASK-803 live the four samples would NOT be identical; they would trace the convergence curve**, which is arguably the most informative thing the row could have returned. ⇒ The withdrawal **survives on a different ground**: the shipped two-point instrument (entry offset + arrival pop) captures both ENDS of that curve and answers the verdict question ("did it converge by arrival?"), and the board's pixel row `(3)(d)` covers *where* it converged. **Suggested fix: none in code — but `TASK-805` must NOT treat the arrival-pop number as evidence about WHERE convergence completed; that is the pixel row's job, and it is now load-bearing rather than decorative.**

- **[WARN] W-3 — `SiegeLadderClimbStatics.h:363-369` — the convergence numbers are the STEERING LAW's, ⛔ not the movement component's, and the behaviour change is REAL and VISIBLE.** Declared by the programmer in three places and I endorse the declaration — but I want the consequence stated as a finding, not a caveat: the swept call goes through `AddMovementInput` → acceleration → `MaxFlySpeed`, and the commanded vector is now up to **0.88 lateral** on the first frames of a worst-case entry, so the **along-track climb rate drops by up to ~52.5 %** there and real convergence **lags** the derived bound. Watchdog headroom covers it (**+6.6 %** measured path cost against a **300 %** allowance). ⇒ ⛔ **A green 306 does NOT prove a pawn converges in PIE.** **Suggested fix: none — this is precisely why the instrumentation shipped in the same diff, and it is `TASK-805`'s verdict to return.**

- **[WARN] W-4 — `HeroCharacter.cpp:2119` · `SummonedUnit.cpp:4078` — READER 2 IS UNCOVERED BY ALL 306 TESTS, PERMANENTLY, AND THIS RIDES FORWARD.** Verified today by my own eyeball (§6) and structurally protected by `const` + the branch `return`, but **a diff read is not a test**: any future edit to either deck-breach step will change arrival/lift semantics with no compile error and no test failure. The `static_assert`s pin the *signature*, ⛔ not the *call site*. **Suggested fix: none available headlessly (neither driver instantiates without a `UWorld`). Record: every future ladder task must re-run this eyeball, and `19(e)` is the row that says so.**

- **[NIT] N-1 — `Tests/SiegeLadderClimbTest.cpp:78-79` — the test fixture's sockets are ONE REVISION STALE.** `LadderFoot(-450,0,0)` / `LadderTop(-150,0,1200)` against the shipped `ClimbableTower.cpp:101-102` `(-460,0,0)` / `(-160,0,1200)` (post-`TASK-783`). ⛔ **INERT for every assertion, old and new** — the shift is a pure `−X` translation, the delta stays `(300,0,1200)`, so length / direction / `DeckBreachUU` / every cross-track figure is bit-identical, and all five new tests build their points relative to `State.Start`. ⛔ **Pre-existing, ⛔ not TASK-803's, ⛔ and not worth a compile to fix.** Flagged only so nobody later "discovers" it as a defect.

- **[NIT] N-2 — `Tools/ArtPipeline/build_watchtower.py:133` — the comment says `(-6, 0, 0)` while line 137 ships `10.0`.** Pre-existing `TASK-783` cite-rot. ⛔⛔ **NOT a reason to touch the value — `§14.4` pins `10.0` and I confirmed it is `10.0`.** Recorded so a future reader does not "reconcile" the number to the comment and trigger exactly the revert `§14.4` exists to prevent.

- **[NIT] N-3 — `SiegeLadderClimbStatics.cpp:219` — an INFINITE (as opposed to NaN) `CurrentWorld` could still yield a NaN steer** via `GetSafeNormal`'s `inf × 0`. Unreachable in practice (actor locations are world-bounded; a NaN'd capsule is caught by the guard that IS there, for the documented reason that `GetSafeNormal`'s size test is `NaN < tol` = false). ⛔ **No change requested** — widening the guard would trade a real, documented protection for a theoretical one.

- **[NIT] N-4 — `HeroCharacter.cpp:1880` · `SummonedUnit.cpp:3903` — the entry log reports `+0.00` on a perfectly vertical line**, where `Cross(Up, Along)` collapses to zero. Log-only, harmless, and the shipped line is 76°.

- **[NIT] N-5 — `Tests/SiegeLadderClimbTest.cpp:2648-2650` — test 23(d) compares two marches that stop at DIFFERENT along-distances** (the control exits via `bPassedTheTop` at `1236.93`; the fixed climb exits via the 16 uu tolerance at `~1220.9`). The comparison therefore **charges the fix more steps for less line** — i.e. it is conservative and safe. Recorded only so the `6.6 %` is not later misread as a like-for-like path cost.

---

## Notes for build-master (`TASK-805`)

1. ⛔⛔ **RAISE THE LOG BEFORE ANYTHING ELSE (`W-1`).** `Log LogGitClaudeUnrealTest Verbose` in the PIE console, or `-LogCmds="LogGitClaudeUnrealTest Verbose"`. **Confirm a known-good Verbose line (`ladder climb STARTED — … uu of line`) appears before you trust the absence of an `ENTRY OFFSET` line.** ⛔ **An empty log is NOT a zero offset.** This single step is the difference between a measurement and a wasted PIE session.
2. **ONE COMPILE.** ⛔ Parse the log for `Result: Failed` — ⛔ **NEVER** `$LASTEXITCODE` (it returns 0 on a failed build). A failure is appended **to this report** and routed back, ⛔ never worked around.
3. **THE SUITE = `306`.** ⛔ A red suite is a STOP. If `SiegeLadderClimbTest` reports 22 and the total is 306, the census reconciles.
4. **PIE (a) — THE ENTRY DISTRIBUTION.** Read the signed cross-track per climb against **±24.0** (hero, `r 42`) and **±32.0** (unit, `r 34`). ⭐ **Walk in DELIBERATELY off-line — that is the point.** ~6 approaches at **different offsets AND different speeds**; the finding is a **DISTRIBUTION**, ⛔ not one number. Capture the raw `Entry` / `line start` vectors too.
5. **PIE (b) — THE ARRIVAL POP.** `CONTACT-§14.3`, ⛔ **never looked for before.** Read the `horizontal` figure: **under ~1 uu = converged; tens or hundreds = `§14.2`'s band arrived intact and the fix did not hold in PIE.** ⭐ **Report the number either way**, including when it is good.
6. ⭐⭐ **PIE (c) — THE ROW THIS GATE ADDS (`W-3`): PAIR EVERY ENTRY OFFSET WITH ITS OWN ARRIVAL POP, PER CLIMB.** The 306 tests prove the steering **law** converges; **only this pair proves the movement component does.** Expect the PIE residual to **lag** the simulated `~0.5 uu` — acceleration and `MaxFlySpeed` are not in the arithmetic. A residual of a few uu is a PASS; tens is a finding.
7. ✅ **PIE (d) — PIXELS.** The pawn must **visibly ride between the stiles by mid-climb**, ⭐ **and the climb must still ARRIVE** — a cross-track term that breaks arrival is worse than the defect. ⚠️ **Expect the declared new look:** a badly off-line entry now crabs laterally at up to **0.88 of its speed** for the first frames, through open air (the ladder carries zero collision hulls). That is the intended behaviour, ⛔ not a bug — but it is the first time it will ever have been seen.
8. **USE THE FREE DISCRIMINATOR (§7).** `ENTRY OFFSET` **followed by** `CONTACT climb started` = the **contact** path; `ENTRY OFFSET` with no follow-up = the ordered **LINK** path. ⭐ **Label every sample** — a distribution that silently mixes the two is two distributions.
9. ⛔ **OUT OF SCOPE: `TASK-798` and `TASK-806` are JONATHAN'S.** ⛔ You may **not** close either from a PIE session, however good the numbers are. `TASK-798` remains the **observation** half; `TASK-806` remains his one-word `A`/`B` ruling.
10. ⛔ **The pre-flight is a REAL MCP request, ⛔ never a port check (`SHIP-§9e`). If MCP is unreachable, SAY SO AND STOP — ⛔ do not fake a PIE result.** ⚠️ The editor was **DOWN** at the time of this gate.
11. **ONE COMMIT on `main`, ⛔ NO PUSH.** ⚠️ Check `git log` / `git status` **FIRST** — Jonathan sometimes commits and pushes himself; ⛔ if HEAD already carries it, do not duplicate, amend or rebase.
12. ⚠️ **Do not "tidy" `N-1` or `N-2` into this commit.** Both are pre-existing, both are inert, and `N-2` sits one line above the constant `§14.4` forbids reverting.
