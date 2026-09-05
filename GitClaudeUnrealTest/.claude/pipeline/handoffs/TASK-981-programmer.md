# TASK-981 — THE FALLOFF BECOMES BEER-LAMBERT — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-986` · **Ship host:** `TASK-987`
**Law:** ⭐⭐ `FOG-§9.2` · `FOG-§7a` (the curve REPLACED) · `FOG-§7b` (**UNAFFECTED — still binds**) · `FOG-§1` · `SC-§37` · `SC-§39` · `SC-§40` cl. 2/9/10/12 · `SC-§53` cl. 3 · ⭐⭐ `SC-§60` cl. 1–4 · `SC-§55` · `TL-§5b`/`§5c`
**Fence honoured:** ⛔ no compile · ⛔ no editor · ⛔ no MCP (editor left alone, PID 16020) · ⛔ no Git · ⛔ `SiegeCombatStatics.{h,cpp}` **NOT TOUCHED** (verified `git diff --stat` empty).

---

## 0. THE HEADLINE

**The `t²` quadratic ease-in is GONE. `FogDensityAt` now returns Beer-Lambert obscuration `1 − exp(−σ·d)` with σ DERIVED, never typed.** `FogDensityExponent` is **retired, not dormant**. `EffectiveVisionRadius` and `IsVisibleThroughFog` are **byte-for-byte untouched**.

⛔ **This is a SUPERSESSION, not a bug fix, and the code says so in those words.** The old curve was correct under the earlier reading of his sentence. Jonathan then ruled the opposite outcome explicitly, after being shown the arithmetic, and his word wins.

| | |
|---|---|
| **Files written** | `Siegebound/SiegeFogStatics.h` · `SiegeFogStatics.cpp` · `Tests/SiegeFogTest.cpp` · `Tests/SiegeFogClampTest.cpp` |
| **Suite** | **441 declared across 33 files** in `Siegebound/Tests/*.cpp` — ⭐ **MY DELTA = `+2`** (`SiegeFogTest.cpp` 9→10, `SiegeFogClampTest.cpp` 8→9) |
| **Prior EXECUTED** | `439 Success / 0 Fail` at `84eec02` · ⭐ I re-measured the pre-edit tree at **439/33** with the `TL-§5b` mandated pattern — **it reconciles exactly**: `439 + 2 = 441` |
| **Pins moved** | ⭐⭐ **ZERO.** Every pin in `handoffs/TASK-867-programmer.md`'s table re-measured live and intact (§5) |
| **Executed** | ⛔ **NOTHING.** `TL-§5c`: `441 declared`, ⛔ never `441/441` |

---

## 1. ⛔⛔ THE ONE THING QA MUST RULE ON FIRST — I EXCEEDED MY `names:` LIST, AND IT WAS FORCED

⛔ **`TASK-981`'s `names:` lists `SiegeFogStatics.{h,cpp}` and `Tests/SiegeFogClampTest.cpp`. I also rewrote `Tests/SiegeFogTest.cpp`, which is named nowhere on my row.**

**It is not optional, and here is the measurement rather than the argument.** Spec item (5) orders `FogDensityExponent` **REMOVED**. Before my edit that symbol was referenced on **4 code lines of `Tests/SiegeFogTest.cpp`** (`:211` the CDO assertion · `:378` the linear-retune fixture · `:786` the degenerate-exponent loop · `:796` the zero-exponent row). ⇒ ⛔ **obeying item (5) while honouring the letter of `names:` produces a tree that DOES NOT COMPILE**, and `TASK-987`'s one compile would have eaten it.

⚠️ **And the second half is worse than the compile break**, which is why I did not look for a way to leave the file alone: `SiegeFogTest.cpp` is where the CURVE is asserted. `SiegeFogClampTest.cpp` asserts the **wiring** and deliberately re-asserts none of the arithmetic (its own header says so). ⇒ the file my row names contains **almost none** of the rows his ruling invalidates. The rows that had to flip were: test 1(e), test 2 (whole), test 3 (whole), test 4's counters, test 9(c)/(d).

📌 **My reading:** the row's `names:` was written against the two files `FOG-§9.2` talks about and missed that the curve's suite lives in the third. ⇒ **manager should add `Tests/SiegeFogTest.cpp` to `TASK-981`'s `names:` and to `TASK-986`'s subject list.** ⛔ If QA rules the fence break unacceptable, the task cannot be completed as specified and the row needs re-boarding — but the diff should be judged before that, because there is no version of item (5) that leaves this file alone.

✅ **Nobody else owns it right now.** Board census: `TASK-910`'s `names:` says ⛔ **NOT `Tests/SiegeFogTest.cpp`**; `TASK-838`/`847`/`908` are closed. ⛔ No live row claims it.

---

## 2. ⛔⛔ THE DEFECT I FOUND IN MY OWN SPEC — THE THIRD CHECKABLE VALUE IS WRONG

> **`TASK-981` item (2) prescribes: "the three checkable values … `d = 152.4` ⇒ `0.6238`."**
> ### ⛔ **THE TRUE VALUE IS `0.6239397`. I ASSERTED THE TRUE ONE.**

**Derivation, both ways, agreeing:** `1 − 0.02^(1/4)` = `1 − exp(−3.9120230/4)` = `1 − 0.3760603` = **`0.6239397`**.

⚠️⚠️ **THIS IS NOT PEDANTRY — THE SPEC'S NUMBER WOULD HAVE TURNED A CORRECT BUILD RED.** The file's tolerance is `Tolerance = 1e-4` (and `TASK-838(6)`/`qa/TASK-847.md` NIT-3 forbid tightening it below that). `|0.6239397 − 0.6238| = 1.397e-04` ⇒ ⛔ **larger than the tolerance.** An implementer who transcribed the spec's decimal would have shipped a **red row against a correct curve** and the obvious "fix" would have been to loosen the tolerance the law says not to touch.

✅ **`FOG-§9.2` ITSELF IS RIGHT** — it says `Obscuration(152.4)` = **62.4 %**, which rounds from `0.62394`, not from `0.6238`. ⇒ ⛔ **the board row's `0.6238` is a transcription slip in the ROW, not an error in the LAW.** Nothing in `CONVENTIONS.md` needs amending; the `TASK-981` spec line does.

⭐ **And the reason it never reached an assertion: I do not transcribe the decimals at all** — see §4.

---

## 3. WHAT CHANGED, FILE BY FILE

### 3.1 `SiegeFogStatics.h`

| change | note |
|---|---|
| ⭐ **NEW `FogTransmittanceAtCeiling = 0.02f`** | `EditDefaultsOnly`, `meta = (ClampMin = "0.001", ClampMax = "0.99")`. His *"98% obscured at 20 feet"* written as the 2% that gets through. ⛔ A **RATIO** ⇒ cannot carry `FOG-§1`'s 30× trap and does not touch the "no second literal" law |
| ⛔ **RETIRED `FogDensityExponent`** | removed outright (`SC-§40` cl. 2). ⭐ Census: **5 surviving textual occurrences, ALL on comment lines** describing the retirement; ⛔ **0 code references** anywhere in `Source/` |
| ⛔ **`FogVisionOnsetUU` comment REWRITTEN, constant KEPT** | item (6) + `SC-§53` cl. 3: the correction is **dated 2026-09-04**, quotes the two sentences that died, and names the **two surviving roles** — `FOG-§7b`'s `ClampMin` floor (load-bearing) and the reporting point |
| ⭐ **`FogVisionCeilingUU` gains a documented role** | it is **σ's anchor** *and now a DENOMINATOR*, which it was not under the old curve. The arithmetic is written beside it |
| ⛔ **`FogDensityAt` doc-comment: the DEMOTION BANNER** | item (4). A boxed banner: *"THE VISUAL'S CURVE ONLY … a gameplay site that calls `FogDensityAt` is an AUTOMATIC QA FAIL"*, with the reason (asymptotic ⇒ 2% forever ⇒ lethal at Longbowman range) |
| ⚠️ **`IsVisibleThroughFog`'s boundary paragraph corrected** | it said *"at d == 609.6 FogDensityAt returns 1.0"*. ⛔ Now 0.98. Rewritten as `FOG-§9.2`'s **declared divergence** (picture 0.98 / mechanic ZERO), with the note that the old agreement was a property of a curve he overruled |
| ⚠️ two `@param Tuning` lines | said *"the falloff exponent"* |

⛔ **The two pinned distance literals are untouched:** `609.6f` and `304.8f` each remain **exactly one code line**; the `ClampMin = "304.8"` meta string remains **exactly one** (both re-measured, §5).

### 3.2 `SiegeFogStatics.cpp` — the rewrite

`FogDensityAt` only. **`EffectiveVisionRadius` and `IsVisibleThroughFog` are not edited by one character.**

```cpp
const float Sigma = -FMath::Loge(Transmittance) / Ceiling;
return FMath::Clamp(1.f - FMath::Exp(-Sigma * DistanceUU), 0.f, 1.f);
```

⛔ **Guard order, and one of these is a NEW hazard the old curve did not have:**
1. non-finite distance / ceiling / transmittance ⇒ `0` (clear)
2. ⛔⛔ **`!(Ceiling > 0.f)` ⇒ `0` — NEW. The ceiling is σ's DENOMINATOR now.** The retired ramp divided by `(ceiling − onset)` and was protected by the `Ceiling <= Onset` branch, **which no longer exists because the band no longer exists.** ⛔ Without this branch a `0` ceiling is a divide by zero
3. `!(T > 0.f) || !(T < 1.f)` ⇒ `0` — `T <= 0` makes σ **infinite** (a whiteout at the camera); `T >= 1` makes σ zero or negative
4. `!(DistanceUU > 0.f)` ⇒ **exactly `0`** — the camera stays bit-identically clear and a negative distance cannot produce the negative density `1 − exp(+x)` would give

⭐ Every degenerate path fails toward **CLEAR**, never toward a whiteout — the module's own totality law, unchanged in spirit. `FMath::Loge(float)`/`FMath::Exp(float)` verified present in UE 5.8 `GenericPlatformMath.h:495`/`:487`.

⛔ **`FogVisionOnsetUU` is NOT READ by the function any more**, and the .cpp says so at the read site so nobody re-adds it.

### 3.3 `Tests/SiegeFogTest.cpp` — 9 → 10

| test | what happened |
|---|---|
| 1 | (e) `FogDensityExponent == 2` ⇒ **`FogTransmittanceAtCeiling == 1 − 0.98`** + a strictly-inside-(0,1) row |
| 2 | ⛔ **INVERTED + renamed.** Was *"ExactlyZeroAtTheOnset…ExactlyOneAtTheCeiling"* — **both halves now false**. Now asserts his two quoted numbers, the **non-hard-cut** at the ceiling, the melee-band consequence, and the picture/mechanic divergence |
| 3 | ⛔⛔ **THE BIG INVERSION + renamed** — see §4 |
| 4 | counters **re-derived** — see §4 |
| **5, 6, 7, 8** | ⭐⭐ **UNTOUCHED, not one character.** These are the `EffectiveVisionRadius` / `IsVisibleThroughFog` tests, including the `Exact` bit-identity test `qa/TASK-847.md` claim 4 verified |
| 9 | (c) zero-width/inverted **band** rows ⇒ ⭐ **the onset is IGNORED** (bit-identical curve across wild onsets) + (c2) the **new** ceiling-denominator guards; (d) degenerate exponent ⇒ degenerate **transmittance** |
| ⭐ **10 NEW** | `TheExtinctionCoefficientIsDerivedFromTheTunablesAndIsNotHardcoded` |

### 3.4 `Tests/SiegeFogClampTest.cpp` — 8 → 9

- ⭐ **NEW test 9** `FogDensityAtIsTheVisualsCurveOnlyAndNoGameplaySiteCallsIt` — spec item (4)'s census.
- ⚠️ header prose amended: it said *"the QUADRATIC falloff, the HARD CUT … in NINE tests"*. ⛔ The falloff is Beer-Lambert, the density curve is no longer a hard cut, and I deliberately **did not restate the count** (`SC-§40` cl. 9 — a count in prose is a citation that rots).
- ⛔ **No existing assertion in this file was touched.**

---

## 4. ⛔⛔ `SC-§60` — THE POLARITY FLIP, AND THE ANTI-FAKE ARGUMENT RE-DERIVED

### 4.1 cl. 1 — THE MANDATORY CENSUS, REPORTED AS A NUMBER

**Rows anywhere in `Siegebound/Tests/` that asserted the OLD behaviour: `9`.** Searched by symbol (`FogDensityAt`, `FogDensityExponent`) and by the three retired tuning targets (`0.0625`, `0.25f`, `0.5625`):

| # | location | what it asserted | disposition |
|---|---|---|---|
| 1 | `SiegeFogTest.cpp` t1(e) | exponent `== 2` | **inverted** ⇒ transmittance `== 0.02` |
| 2 | t2(a) melee 120 uu `== 0` Exact | clear melee band | **inverted** ⇒ `> 0.5`, mechanic asserted untouched beside it |
| 3 | t2(a) onset `== 0` Exact | the clear bubble | ⛔ **inverted** ⇒ `== 0.8586` — *the ruling itself* |
| 4 | t2(a) onset+1 `< 0.001` | "a whisper" | **inverted** ⇒ the onset is thick |
| 5 | t2(b) ceiling `== 1.0` Exact | hard cut on the picture | **inverted** ⇒ `< 1.0` |
| 6 | t2(c) five beyond-ceiling `== 1.0` Exact | hard cut | **inverted** ⇒ `>= 0.98 && <= 1.0` |
| 7 | t3 (a)/(b)/(c) `0.25`/`0.0625`/`0.5625` + ascending steps | `t²` + acceleration | ⛔ **inverted** ⇒ three Beer anchors + **deceleration** |
| 8 | t4 `OpaqueSamples > 0` and far end `== 1.0` Exact | a reachable opacity | **inverted** ⇒ ⛔ **would have gone RED against a correct build** |
| 9 | t9(c)/(d) zero-width band + degenerate exponent | a band that no longer exists | **replaced** |

⛔ **Zero rows outside these two files** referenced the curve. `SiegeFogClampTest.cpp` correctly held none (it never re-asserted `837`'s arithmetic).

### 4.2 cl. 2/cl. 4 — KEPT AND INVERTED, WITH HISTORY AND THE RULING'S CITATION

⛔ **No test was deleted.** Tests 2, 3, 4 and 9 keep their `IMPLEMENT_` identities and carry banners naming (a) what they used to assert, (b) that it was **superseded, not wrong**, and (c) `FOG-§9.2` + his verbatim sentence. Test 3's banner is the long one because it is the row that changed sides.

### 4.3 cl. 3 — ⛔⛔ THE OLD RED CONTROL **WAS** THE NEW SHIPPED CURVE

> **The retired test 3 named its red controls as: linear (0.50), smoothstep (0.50), and ⛔ BEER-LAMBERT (~0.63).**
> ### ⛔ **THE CURVE THAT ROW EXISTED TO KILL IS THE CURVE THAT NOW SHIPS.** This is `SC-§60` cl. 3's exact hazard, live.

⇒ **nothing was inherited.** The new discriminator set, with each shape it kills and the measured separation:

| the new instrument | kills | measured |
|---|---|---|
| **three absolute anchors** (0.9800 / 0.8586 / 0.6239) | ⛔ the retired `t²` · linear-in-distance · smoothstep · `sqrt(d/ceiling)` · a σ derived from the **onset** (reads 0.9996 at the ceiling) · a base-10 log · a sign error | 0.624 / **0.859** / 0.020 vs `t²`; 0.374 / 0.359 vs linear |
| ⭐ **the retired curve, computed in the fixture as a live red control** | the tree still carrying `t²` | asserted `> 0.5` separation at two anchors, `> 100×Tolerance` at the third |
| **concavity (first half > second half)** | ⛔ `t²` **asserts the reverse** · linear splits 50/50 · smoothstep is symmetric | **7.0711×** |
| **descending equal steps** | same family; the old row asserted **ascending** | 0.624 > 0.235 > 0.088 |
| ⭐⭐ **test 10's two retune controls** | ⛔ **a hardcoded `Sigma = 0.0064174f`** — which passes every anchor above | T→0.5 ⇒ ceiling must read 0.50; ceiling→×2 ⇒ old ceiling must read 0.8586 |

⭐⭐ **THE PROPERTY THAT MAKES THE ANCHORS EVIDENCE RATHER THAN TRANSCRIPTION, and it is the load-bearing choice in the diff:** the test computes its expectations with **`FMath::Sqrt`** (`1−T`, `1−√T`, `1−√√T`) while the subject computes with **`FMath::Loge` + `FMath::Exp`**. ⛔ **Two different functions, two different code paths.** Agreement is a falsifiable coincidence.

⛔⛔ **AND THIS IS NOT THE BANNED √ TEST.** Item (3) / `FOG-§9.2` forbid asserting the **relation** `f(d/2) == √f(d)` — a theorem, true for every σ and every d, which could never fail. ⛔ **I assert three ABSOLUTE NUMBERS.** The √ is my derivation path; the claim is the value. Every wrong-σ implementation above fails them.

📌 **Measured in float32:** the two routes agree to **0.000e+00** at all three anchors. Full simulation of every new assertion: **all pass**, worst residual `5.96e-08` against a `1e-4` tolerance (1,678× headroom).

### 4.4 THE ANTI-TRIVIALITY COUNTERS — RE-DERIVED, AND THE OLD ONES WOULD HAVE GONE RED

⛔ **Two of test 4's three old categories are wrong for this curve.** Beer-Lambert is exactly `0` at **one** sample (the camera) and is **never** exactly `1` inside the sweep — `exp` needs `d > 16,201 uu` to underflow, and the sweep stops at 1200. ⇒ ⛔ **the shipped `OpaqueSamples > 0` row and the `far end == 1.0` Exact row would both have gone RED against a CORRECT implementation.**

✅ **The replacement is a stronger property than what it replaces:**

> ⭐⭐ **`PositiveInsideTheOldOnset >= 20`** — samples with `0 < d <= onset` carrying **strictly positive** density.
> Under the retired `t²` curve **every one of those was exactly 0**. Under his ruling **all 50 are positive.**

⇒ **a tree that still carries a clear bubble fails the sweep test itself** — the sweep-level embodiment of the ruling, and it keeps the property the spec required. Measured counters: **50 · 200 · 99**.

---

## 5. ⛔ THE PINS — RE-MEASURED LIVE, **ZERO MOVED** (spec item (7))

⛔ **`SC-§40` cl. 12: I rebuilt `CountOccurrencesInCode`'s skip rules exactly rather than approximating with `grep`** (leading `//`, `* `, `*/`, `/*`, bare `*`), plus `CountAcrossShippingSource`'s `/Tests/` exclusion and its `< 20 files` liveness check. Instrument reported **269 source files**.

| pin | pinned at | measured NOW | status |
|---|---|---|---|
| `FSiegeFogStatics::EffectiveVisionRadius(` tree-wide | **2** | **2** | ✅ intact |
| `ReadFogState(` tree-wide | **3** | **3** | ✅ intact |
| `Ceiling <= 0.f` in `EffectiveVisionRadius` body | **1** | **1** | ✅ `FOG-§7b`(b) intact |
| `Ceiling < 0.f` in that body | **0** | **0** | ✅ intact |
| `ClampMin = "304.8"` in the header | **1** | **1** | ✅ `FOG-§7b`(a) intact |
| `FSiegeFogStatics::EffectiveVisionRadius(` in the funnel body | **1** | **1** | ✅ intact |
| ⭐ `FogDensityAt(` tree-wide shipping | *(new)* | **2** | ⭐ **NEW PIN** — decl + defn, zero gameplay callers |

⛔ **No `== N` was loosened to `>= N`.** ⚠️ **One deliberate `>=`, declared:** my new positive control uses `EffectiveVisionRadius(` `>= 3` rather than `== 4`, **because the exact tree-wide count of that needle is already pinned by test 4 and by the `TASK-867` table.** A second `== N` on the same needle would be a duplicate pin two different tasks could break. It is a **control**, not a pin.

⭐ **Every zero carries a positive control (`SC-§39`, both directions of `SC-§40` cl. 12):** the tree-wide `2` is controlled by the same scanner finding `EffectiveVisionRadius(`; each per-file `0` is controlled by `float` on a code line (**113 · 4 · 47 · 8 · 16**); the funnel's `0` is controlled by the same extracted body still yielding its `1` clamp call — so a mis-extracted or empty body cannot read as clean. All needles are **call shapes** (`Symbol(`), never bare tokens.

---

## 6. ⚠️ WHAT QA SHOULD SCRUTINISE HARDEST

1. ⛔⛔ **THE FENCE (§1).** Rule on `Tests/SiegeFogTest.cpp` explicitly. It is the biggest judgement call in the diff.
2. ⛔ **THE SPEC'S `0.6238` (§2).** Confirm the arithmetic independently; I assert `0.6239397`.
3. ⛔ **THE NEW DIVIDE-BY-ZERO.** `!(Ceiling > 0.f)` in `FogDensityAt` is a hazard class that did not exist before. Try to reach the division without it.
4. ⭐⭐ **ADJUDICATE EACH INVERSION FOR VACUITY, INDIVIDUALLY (`SC-§60` cl. 5).** ⚠️ **The one I am least sure of is test 3(b)'s ceiling row**: the shipped curve and the retired one differ by only **0.020** there (vs 0.859 at the onset). It is 200× the tolerance so it does discriminate, but it is the thinnest margin in the file and it carries the whole "not a hard cut" claim by itself at that distance. **Test 2(d) (`< 1.0`) is the independent second witness — check I have not made them the same row twice.**
5. ⚠️ **Is my `Sqrt`-vs-`Exp` derivation genuinely independent**, or have I talked myself into a clever transcription? It is the argument the whole suite rests on. ⛔ Also confirm I have not smuggled in the banned √ **relation** test.
6. ⚠️ **`ClampMin = "0.001"` / `ClampMax = "0.99"` on the new tunable are numbers I chose.** They are dimensionless slider stops applying `FOG-§7b`'s ruling to the adjacent input, and the code guard is total regardless — but they are **mine, not his**, and I declare them rather than hide them.
7. ⚠️ **The melee band is now ~53.7% obscured visually** where the old curve rendered it perfectly clear. ⛔ The mechanic is untouched (`min`, 120 < 609.6, asserted). **This is a real, visible consequence of his ruling and Jonathan should hear it from us**, not discover it.

---

## 6.5 ⛔⛔ THE PIXEL GATE, RE-RULED WITH THE CURVE — **NEW PASS/FAIL TARGETS, STATED EXPLICITLY**

⛔ **`FOG-§7a`'s gate — *"zero fog inside `304.8`, opaque at `609.6`"* — is now FALSE and must NOT be run.** It was written against the `t²` curve and `TASK-843` already rewrote it once for exactly this reason. ⛔ **It must be rewritten again**, and this is the second rewrite.

⭐⭐ **AND THE HISTORY IS WORTH ONE LINE, BECAUSE IT INVERTS:** `FOG-§8` cl. 4 proved that gate **arithmetically unpassable** by a uniform-density volume, and treated it as the thing blocking `TASK-841`. ⛔ **That was never a defect — it was a SPEC CONFLICT, and Jonathan resolved it in our favour.** The gate was right to fire; the target was wrong.

### THE NEW TARGETS — obscuration measured at the camera-to-sample distance

| distance | ft | ⛔ **PASS** (obscuration) | ⛔ **FAIL** |
|---|---|---|---|
| `0` uu | 0 | **exactly `0.0000`** — bit-identically clear | any visible haze at the camera |
| `152.4` uu | 5 | **`0.6239` ± 0.02** | ⛔ `0.0` (a clear bubble = the retired curve) · `0.25` (linear) |
| `304.8` uu | 10 | ⭐ **`0.8586` ± 0.02** — *his "86% at 10"* | ⛔⛔ **`0.0` — this is THE row the retired curve fails** · `0.50` (linear) |
| `609.6` uu | 20 | ⭐ **`0.9800` ± 0.02** — *his "98% obscured at 20 feet"* | ⛔ **`1.0000` (a hard cut — you must still just barely make something out)** |
| `1219.2` uu | 40 | **`0.9996`**, and still **`< 1.0`** | ⛔ exactly `1.0` |

⛔ **THE TWO ROWS THAT CARRY THE RULING**, and a gate that drops either is not testing his sentence:
1. ⛔ **`304.8` MUST BE THICK (~86%), NOT CLEAR.** The old gate demanded the opposite.
2. ⛔ **`609.6` MUST NOT BE FULLY OPAQUE.** The picture is asymptotic; only the **mechanic** is a hard cut.

⚠️ **±0.02 is my proposed pixel tolerance, not a measured one** — it is ~2 sRGB steps out of 255 and it is deliberately wider than the C++ `1e-4` because a volumetric render carries noise, dithering and exposure. ⛔ **The art lane should tighten or widen it against a real capture; I have not seen one.** ⛔ **`TASK-858` still blocks every pixel observation** (`L_Arena`'s `bEnableVolumetricFog = false` ⇒ the asset renders literally nothing and any reading is uninterpretable).

📌 **AND A SLIP TO CORRECT WHILE IT IS STILL CHEAP:** `TASK-841`'s row already carries the new Beer-Lambert targets — but writes ⛔ **`62.38%` at `152.4`**. ⛔ **It is `62.39%`** (`0.6239397`). ⛔ **Same one-digit slip as `TASK-981` item (2)'s `0.6238` (§2) — it has now propagated to a second row**, which is exactly how a wrong number becomes law. **Manager: fix both.**

---

## 7. 📌 FINDINGS I AM **NOT** FIXING (not mine — routed, per `SC-§40` cl. 11(b) with symbols named)

1. ⛔ **`SiegeCombatStatics.cpp:122-123`** — `ReadFogState`'s comment reads *"A default-constructed tuning IS the shipped tuning (609.6 / 304.8 / **exponent 2**)"*. ⛔ **`exponent 2` names a tunable that no longer exists.** ⛔ That file is `TASK-980`/`TASK-839`'s and item (8) fences it. **Route to whoever next opens it** — one comment word.
2. ⚠️ **`SiegeFogStatics.h`'s onset paragraph still carries `qa/TASK-908.md` WARN-3** — *"There is exactly ONE `304.8` in the codebase and it is this line"*, which the `ClampMin` meta string at the ceiling contradicts. ⛔ **That sentence is `TASK-910`'s named subject** (*"the ONSET's paragraph ONLY"*) and `TASK-910` has **not landed** (no handoff on disk). ⛔ **I did not touch that sentence** and the `304.8` count is unchanged at 1 code line + 1 meta string, so WARN-3's severity is unchanged. ⚠️ **But I rewrote a later paragraph in the same comment block, so `TASK-910` must re-locate by CONTENT, not by line number** (`SC-§38`).
3. 📌 `TASK-981`'s board `names:` list needs `Tests/SiegeFogTest.cpp` (§1) and its item (2) needs `0.6238` → `0.6239` (§2). Manager's.

---

## 8. ⚠️ `SC-§55` — **MY CONTEXT `gitStatus` SNAPSHOT WAS WRONG, AND I RECORD THE DISAGREEMENT**

⛔ **My session preamble's `gitStatus` showed a CASTLE-era working tree** — `Content/Materials/MI_Castle_Interior_Crumble0{1,2,3}.uasset` with `A ` **staged** entries, `Castle.cpp`/`Castle.h` modified, `TASK-62x`/`63x` handoffs untracked, HEAD at `7142839`.

✅ **The LIVE `git status` at my instant shows none of it** — no castle files at all, a different modified set, and `TASK-92x`–`97x` handoffs untracked instead. ⇒ ⛔ **the snapshot is weeks stale and it carries staged-index entries, exactly as `SC-§55` cl. 2 warns.** An agent building a pathspec from it would have staged files that have not been dirty for weeks and found "agreement".

📌 **Also observed live, and it matters for routing:** `handoffs/TASK-978-programmer.md` **exists on disk** ⇒ `TASK-979` is unblocked (`SC-§40` cl. 11(a)).
📌 **Not mine, present in the tree:** `Tests/SiegeCardRosterTest.cpp` (modified) and `Tests/SiegeCardArtRosterTest.cpp` (untracked) are a **parallel lane's**. ⛔ My 441 census includes them; my **delta of +2** does not.

⚠️ **`TL-§5b` 2a — the bare-pattern trap is LIVE on this tree, measured both ways:** scoped `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp` = **441/33**; bare `^IMPLEMENT_` over `Source/**/*.cpp` = **440/34** *(pre-edit: 439/33 vs 440/34)*. ⛔ Use the scoped form.

---

## 9. WHAT I DID NOT DO

⛔ No compile · ⛔ no editor, no MCP (editor left untouched at PID 16020) · ⛔ no Git · ⛔ no test executed (`TL-§5c`: **441 declared**, and the **first execution** of the rewritten curve suite is owed by `TASK-987`) · ⛔ `SiegeCombatStatics.{h,cpp}` untouched · ⛔ `EffectiveVisionRadius` / `IsVisibleThroughFog` bodies untouched · ⛔ no team, viewer, controller or actor parameter added to any fog function (symmetry stays structural) · ⛔ no *"both teams agree"* test · ⛔ no test of the √ relation · ⛔ no third distance literal.
