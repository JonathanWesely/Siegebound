# TASK-837 — `SiegeFogStatics`: the two pinned constants + the falloff — handoff

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Law:** `FOG-§1`, `FOG-§6` (cited, not restated)

## ⛔⛔ ZERO BEHAVIOUR CHANGE — READ THIS FIRST

**Nothing in the shipped game consults these files.** Three NEW files landed; not one existing
file was opened for writing. Until **TASK-838** routes the clamp through
`FSiegeCombatStatics::GatherHostileAgents`, fog does not exist at runtime and **not one unit's
range changes by a single unit**. ⛔ A landed file is not a landed feature.

## Files

| file | state |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.h` | ⭐ NEW |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.cpp` | ⭐ NEW |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogTest.cpp` | ⭐ NEW |

⛔ **Fences honoured:** no `SiegeCombatStatics.*` (TASK-828 LIVE), no `SiegePlayerController.*`
(TASK-819 LIVE), no `SummonedUnit.*`, no `SpellLibrary.*`, no art, no `BP_FogArea` (TASK-836), no
`Build.cs` change (same module, Core only). ⛔ No compile (`QUIET-MODULE`, three queued), no
editor, no MCP, no Git.

## ⭐ THE TWO CONSTANTS, AS WRITTEN

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "0"))
float FogVisionOnsetUU = 304.8f;   // FOG-§1: 10 ft × 30.48 cm/ft

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Fog", meta = (ClampMin = "0"))
float FogVisionCeilingUU = 609.6f; // FOG-§1: 20 ft × 30.48 cm/ft
```

Each carries the full arithmetic (`1 ft = 30.48 cm exact ⇒ 10 ft = 304.8 uu`, `20 ft = 609.6 uu`),
the 30× feet-as-units warning, and the ceiling additionally carries **all eight rows of `FOG-§2`'s
consequence table** plus the 1.22%-of-the-arena scale — so the next person to retune it sees
−83.1% on the Longbowman *before* they touch it (spec item 4).

⛔ **VERIFIED BY GREP: `304.8` and `609.6` each appear exactly ONCE in `Source/`, and both are
these two lines.** Neither `300`, `600`, `10` nor `20` appears as a fog distance anywhere. The
test file derives every expectation from `Feet × 30.48` and deliberately contains **no literal of
either number** — a test transcribed from its subject cannot disagree with a wrong subject
(`SHIP-§9c`, the `SiegeHighGroundTest` precedent).

### ⚠️ ONE DECLARED DEVIATION, NOT SMUGGLED

`FOG-§6` says *"plain C++ statics"* while `FOG-§1` says the constants are `EditDefaultsOnly` —
and `EditDefaultsOnly` requires reflection, which a plain `class F...` cannot carry. **The project
already solved this exact tension once:** `FSiegeStuckTuning` (`SiegeStuckStatics.h:135`) is a
reflected tuning struct sharing a header with a plain static library, and `NAV-§7` explicitly
instructs QA not to flag it. I reused that precedent rather than inventing a pattern. The statics
class itself stays plain and world-free, so the headless-testability half of `FOG-§6` is intact.

### ⚠️ ONE ADDITION OVER THE SPEC, DECLARED

`FogDensityExponent = 2.f` — a **third** `EditDefaultsOnly` tunable. It is a **shape, not a
distance**, so it cannot carry the 30× trap and does not touch `FOG-§1`'s no-second-literal law.
It exists because TASK-837 asked me to *rule* on linear-vs-curved, and a ruling that ships as a
hardcoded `2` is one somebody must recompile to disagree with. Shipped as a tunable, *"make it
linear"* is a one-word retune to `1.0` — the identical argument `J-F2` made for the two distances.
⛔ If QA judges a third constant out of scope, it deletes cleanly (hardcode `2.f` in the ramp);
say so and I will.

## ⭐ THE FALLOFF SHAPE — QUADRATIC EASE-IN (`t²`), AND WHY

`density(d)` on `[0,1]`: `0` at/inside the onset → `t^2` where `t = (d−onset)/(ceiling−onset)` →
`1` at/beyond the ceiling.

**Both halves of his sentence point at an ease-in, and they point away from linear:**

- *"a **light** amount of fog **starting** at about 10 feet"* — an ease-in leaves the onset with
  zero slope, so the fog fades in from nothing. One uu past the onset: **quadratic reads
  0.0000108, linear reads 0.0033 — 300× more.** Linear puts a visible **edge** exactly where he
  asked for a light haze.
- *"it gets **thicker and thicker**"* — a doubled comparative is an *acceleration*, not a constant
  rate. The quadratic gains **0.25 density over the first half of the band and 0.75 over the
  second**, and is steepest at the ceiling, where his sentence slams shut.

⛔ **Not smoothstep** — it eases in *and out*, flattening as it approaches the ceiling, which is
the one place his sentence is absolute. ⛔ **Not Beer-Lambert** — the physical curve is *concave*
(fastest first, decelerating), i.e. the exact opposite of "thicker and thicker", and it is
asymptotic so it can never reach the state he named. (His rule is already non-physical: real
homogeneous fog has no clear bubble, and he asked for one.)

## ⚖️ THE FOUR RULINGS (proceeding defaults; ⛔ none blocked)

| # | question | ruling | reason |
|---|---|---|---|
| **1** | does the clamp apply to **both teams**? | ✅ **YES — symmetric**, and it is enforced **structurally** | ⛔ **Not one function takes a team, a viewer, a controller or an actor. An asymmetric fog is *unrepresentable* — you cannot write the favouritism because there is no parameter to branch on.** His words, `J-F3`. ⭐ Note the deliberate contrast: TASK-827's invisibility predicate *does* take viewer + target team, because that feature *is* asymmetric. The two signatures disagree on purpose. |
| **2** | **linear or curved**? | ✅ **CURVED — quadratic ease-in**, exponent shipped as a tunable | see above: the ease-in is what makes the onset *light* and the ramp *accelerating*. Linear satisfies neither half of his sentence. |
| **3** | ceiling a **hard cut** or an asymptote? | ✅ **HARD CUT** | (a) his sentence is absolute — *"really cannot see **anything** beyond 20 feet"*; an asymptote reading 0.97 leaves 3% visibility, and at Longbowman range 3% is still a lethal shot; (b) the acquisition clamp is binary by nature, so an asymptotic *picture* over a hard *mechanic* would disagree at exactly the boundary the player judges the card by; (c) a hard cut is assertable **exactly** — an asymptote can only be asserted to a tolerance and can never be shown to have reached the state he named. |
| **4** | ⭐ **acquisition only, or a shot already in flight?** | ✅ **ACQUISITION ONLY — an arrow already in the air still lands** | His sentence covers only *"will not be able to **fire** beyond this range"* — the decision to fire. ⭐ There is a **shipped precedent directly on point in this same batch**: `WITCH-§2` rules already-locked projectiles UNAFFECTED because `Projectile.cpp:352` is target-locked at FIRE time. Fog adopts it for the same reason, and it is also the physical answer. ⛔ **Declared residual:** a volley fired the instant before the fog lands still connects. Exposure is tiny — because firing is already clamped, an in-flight arrow can only ever be travelling ~609.6 uu plus target drift. |

**A fifth boundary ruling QA should look at:** at `d == 609.6` **exactly**, `FogDensityAt` returns
`1.0` (opaque) while `IsVisibleThroughFog` returns **true** (still in range). Deliberate. The
project's shipped acquisition idiom is **inclusive** — measured at `SummonedUnit.cpp:1644`,
`:1785`, `:1851`, `:1956`, `:2409`, all `GetDistanceToTarget(...) <= AttackRange`. ⇒ **the clamp
substitutes the *operand* of that comparison, never its *shape*.** An exclusive fog predicate
would make the fog boundary the only exclusive range boundary in the game. They are two different
consumers asking different questions at one float of measure zero.

## ⭐⭐ THE SEAM TASK-838 CONSUMES

```cpp
static float EffectiveVisionRadius(float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning);
static bool  IsVisibleThroughFog(float DistanceUU, float RequestedRadiusUU, bool bFogActive, const FSiegeFogTuning& Tuning);
static float FogDensityAt(float DistanceUU, const FSiegeFogTuning& Tuning);   // TASK-841's curve
```

`EffectiveVisionRadius` returns `min(Requested, Ceiling)` under fog and — critically —
**`RequestedRadiusUU` bit-identically when `bFogActive == false`**: no clamp, no sanitising, not
one ulp of drift. ⭐ That is what makes it safe for TASK-838 to call **unconditionally** at the
funnel: with fog off the game is byte-for-byte the game that shipped, which is the property that
keeps any fog regression *attributable to fog*.

### ⛔⛔ TWO MEASURED FINDINGS TASK-838 MUST NOT INHERIT BY ACCIDENT

`FOG-§6` says the clamp is applied *inside* `GatherHostileAgents`. ⚠️ **That funnel is about to
become the chokepoint for consumers that are NOT acts of seeing**, and a blind clamp there is
wrong:

1. ⛔⛔ **`ASpellLineSweep::LineRange = 900.f` (`SpellLineSweep.h:103`) ALREADY EXCEEDS 609.6.**
   A blind clamp inside the funnel would **cut the hero-line spell by 32.3% under fog** — a live
   behaviour change on a surface Jonathan's sentence never mentions. ⛔ **This is not a
   hypothetical; it is true today.**
2. ⚠️ **Every AoE radius in the game is *smaller* than the ceiling** — Sapper 250, BombTower 250,
   Wizard 250, Fireball 300, FrostNova 350, BattleCry 400 (measured from `Docs/Data/cards.csv`).
   So a blind clamp is inert on them ⛔ **by coincidence of today's data, not by design** — one
   700-radius spell added later silently becomes fog-dependent.

⇒ **Recommendation:** `GatherHostileAgents` should take the caller's radius and apply
`EffectiveVisionRadius` at **one** place inside itself for **vision/acquisition** queries only,
with blast/sweep consumers passing their radius unclamped. That keeps `WITCH-§1`'s
one-chokepoint law *and* keeps a blast out of it. ⚖️ `WITCH-§2` already ruled the same distinction
for the other card — *"a blast is not an act of seeing"*. **The function name is the guard:
`EffectiveVisionRadius`. A blast radius is not a vision radius, so a wrong call site reads wrong.**

### ⚠️ A THIRD, SMALLER FINDING FOR TASK-828/838

The board pins the signature as `GatherHostileAgents(const UWorld*, **ESiegeTeam** ViewerTeam, ...)`,
but **the shipped enum is `ETeamId`** (`TeamId.h:14`); `ESiegeTeam` does not exist in `Source/`.
⛔ Not my file to fix — flagging so TASK-828 does not ship a name that has to be renamed later.
My API sidesteps it entirely by taking no team at all.

## ⛔ TOTALITY (what QA should try to break)

Every function is total — no input divides by zero, returns NaN, or asserts. TASK-838 puts this
inside the funnel **every attack in the game routes through**, so a NaN escaping there would take
out combat globally. Degenerate cases **fail toward NO FOG** (never toward *no vision*):

- non-finite distance/onset/ceiling → density `0` (clear); a **broken tuning must never blind the
  army**, so `EffectiveVisionRadius` returns the request **unchanged** on a NaN or negative
  ceiling. ⚠️ A bare `min()` against a negative ceiling would clamp every unit in the game to a
  negative range and stop all combat.
- `ceiling <= onset` (zero-width **or** inverted band) → the continuous limit: a **hard step** at
  the ceiling. This branch is *why* the division cannot divide by zero — it is the guard, not
  defence in depth.
- exponent `0` / negative / NaN → falls back to **linear**, ⛔ never to `Pow(t, 0) == 1`, which
  would paint a wall of fog at the onset.
- `IsVisibleThroughFog` is the **one** place that fails the other way: a NaN *distance* returns
  `false`, because a garbage distance is a broken *target* and acquiring it would push the NaN
  into a move order.

## Tests — `Tests/SiegeFogTest.cpp`

**⭐ SUITE DELTA: +9 TESTS** (census by grep of `IMPLEMENT_SIMPLE_AUTOMATION_TEST` across
`Siegebound/Tests/`, taken before and after).

⚠️ **The absolute total moved for a second reason and the arithmetic is reconciled here rather
than left to look like a miscount:** my baseline was **344**; the tree now reads **361**. The gap
is **TASK-827's `SiegeInvisibilityTest.cpp` (+8)**, which landed in parallel during this task.
⇒ **344 + 8 (TASK-827) + 9 (TASK-837) = 361 ✓.** ⛔ My contribution is **+9 and only +9** —
`Tests/SiegeFogTest.cpp` is the only test file I created or touched.

| # | test | what makes it able to FAIL |
|---|---|---|
| 1 | `TheConversionIsThirtyPointFourEightPerFootAndNotFeetAsUnits` | derives `10×30.48` / `20×30.48`; explicitly refuses `20`, `10`, `600`, `609`, `300`; asserts ceiling `== 2 ×` onset (catches a **swapped pair**, the edit most likely to survive review) |
| 2 | `DensityIsExactlyZeroAtTheOnsetAndExactlyOneAtAndBeyondTheCeiling` | ⭐ **the boundary + BEYOND rows.** `Exact` 1.0 **at** the ceiling ⇒ **every asymptote fails**. Five beyond-ceiling distances (610, 900, 2100, 3600, 50000) ⇒ an unclamped ramp reading 10.8 at 3600 fails. Also asserts one-uu-past-onset is `< 0.001` — **linear reads 0.0033 and fails** |
| 3 | ⭐⭐ `TheFalloffAcceleratesBetweenTheOnsetAndTheCeilingAndIsNotLinear` | **THE DISCRIMINATING TEST.** Midpoint must read **0.25**; ⛔ **linear reads 0.50 and fails by 2,500× the tolerance — so does smoothstep; Beer-Lambert reads ~0.63 and fails the other way.** Plus quarter (0.0625 vs linear 0.25) and three-quarter (0.5625 vs 0.75); plus the second-derivative claim (2nd half gains > 2× the 1st — **linear splits it 0.5/0.5 and fails flat**); plus three equal distance-steps gaining progressively more (**a concave curve reverses them**); plus exponent `1.0` really yielding linear, which proves the rows above measure the *exponent* and not the boundary code |
| 4 | `DensityIsMonotonicNonDecreasingAndStaysInsideZeroToOne…` | 201-sample sweep 0→1200 uu. ⛔ **Carries anti-triviality counters and asserts ≥20 samples land STRICTLY between 0 and 1** — the exact trap TASK-837 named: a sweep that never left the onset now **fails the sweep test itself** |
| 5 | `EffectiveVisionRadiusIsBitIdenticalToTheRequestWhenFogIsInactive` | `Exact` tolerance on all nine shipped ranges ⇒ **any unconditional clamp fails** |
| 6 | `TheCeilingClampsEveryLongRangeCardAndLeavesTheShortOnesUntouched` | `FOG-§2`'s table as assertions: −83.1% / −71.0% / −56.5% / −32.3% ⇒ **a 600 ceiling reads −83.3% and fails**. Cleric 400 and melee 120 asserted **unchanged** ⇒ a blanket clamp fails. Ballista `MinRange 300 < ceiling` tripwire ⇒ a retune below 300 that silently disables the card fails |
| 7 | `TheClampOnlyEverShortensARangeAndNeverLengthensOne` | ⛔ **the invented-floor test.** 101-sample sweep; fails if any range is *lengthened* or exceeds the ceiling. Also carries anti-triviality counters proving **both** branches were exercised |
| 8 | `TheVisibilityPredicateMatchesTheShippedInclusiveRangeComparison` | the card in one pair of rows (Longbowman sees 3000 unfogged, cannot fogged). ⛔ Asserts it **can still shoot at 500** ⇒ a predicate returning false everywhere fails. ⛔ Asserts melee does **not gain reach** at 500 ⇒ an implementation ignoring the caller's own range fails |
| 9 | `DegenerateInputsNeverReturnNaNAndNeverBlindTheArmy` | NaN/Inf built from **IEEE bit patterns**, not `sqrt(-1)` — ⛔ a fast-math compiler could fold the obvious spelling away and turn this into a test of two ordinary floats |

## ⛔ What these tests do NOT cover (stated so nobody mistakes green for done)

- ⛔ **Nothing consults this at runtime yet** — they prove the *arithmetic*, not the feature.
- ⛔ **Symmetry is not asserted, deliberately** — it is enforced by the *type system* (no team
  parameter exists). A runtime "both teams get the same answer" row would be trivially true and
  would report SAFE forever; the absent parameter is the stronger instrument.
- ⛔ The **visual** (TASK-841), the **duration/refresh** (TASK-839), the **card row** (TASK-840).
- ⛔ **No compile has been run** (`QUIET-MODULE`, three queued). UHT risk is minimised by mirroring
  two proven in-repo patterns: `FSiegeStuckTuning` (reflected tuning struct + plain statics in one
  header) and `FCardRow` (`EditAnywhere, BlueprintReadOnly` on USTRUCT members).
- 🔒 No inference, no `Capture()`, no latch spend, no token figure (`AS-§12g`).

## ⚖️ For Jonathan, unchanged and unsoftened

`609.6 uu` ships **exactly**, knowing it is **−83.1% on the Longbowman** and **1.22% of the way to
the enemy castle**. ⛔ It was not adjusted, and ⛔ no floor was invented to soften it. `FOG-§2` is
disclosure, not a counter-proposal (`J-F2`). Both constants are `EditDefaultsOnly`, so his next
sentence retunes them with no code change.
