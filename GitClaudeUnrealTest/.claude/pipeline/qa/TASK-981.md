# QA Report — TASK-981
Verdict: **PASS** — 0 BLOCKERS · 4 WARN · 5 NIT

Subject: `SiegeFogStatics.h` · `SiegeFogStatics.cpp` · `Tests/SiegeFogTest.cpp` · `Tests/SiegeFogClampTest.cpp`
Input: `handoffs/TASK-981-programmer.md` · board rows TASK-981 / TASK-986 · `CONVENTIONS.md` FOG-§9.2 / §9.4
Coverage (`SC-§29`): this report covers **TASK-981 and that task alone.** It is not TASK-986.

## 0. Instrument declaration (`SC-§39`, and `SC-§38a` honoured)

- Every character-exact claim below is quoted from **`Read`**, never from `Grep` content. `Grep` was used to LOCATE only (counts + file/line), and every located line was re-read before being quoted or judged.
- Arithmetic was re-derived independently from `σ = −ln(0.02)/609.6`; I did **not** confirm the author's numbers, I recomputed them and then compared.
- UE 5.8 API existence checked by reading the engine header directly.
- ⛔ **Not done, by fence:** no compile, no engine/editor/MCP, no Git. ⇒ **byte-for-byte identity of `EffectiveVisionRadius` / `IsVisibleThroughFog` is NOT independently verified** (that is a `git diff` claim and Git is fenced). What I *can* attest, by reading, is stated in §3.
- ⛔ **Tool limitation, recorded not worked around:** the `Edit` tool was disabled for this session. See §7 — **I could not flip the board status and I have not pretended to.**

---

## 1. ⛔⛔ THE FENCE BREACH — RULED, EXPLICITLY

> **RULING: the breach is ACCEPTED. Writing `Tests/SiegeFogTest.cpp` outside the row's `names:` was FORCED, correctly declared, and is NOT a finding against the author. The `names:` list is the defective artefact, not the diff.**

Reasoning, in the order it decides the question:

1. **Obeying the fence to the letter produces a tree that does not compile.** Spec item (5) orders `FogDensityExponent` REMOVED. I verified the symbol's surviving census myself: **5 textual occurrences, all on comment lines, 0 code references** (`SiegeFogStatics.h:41`, `:55`, `:252` — doc-comment `*` lines; `Tests/SiegeFogTest.cpp:296`, `:968` — `//` lines). Before the edit that symbol was referenced from code in `Tests/SiegeFogTest.cpp`. Removing the member and leaving the references is a hard compile error, and `TASK-987` carries **one compile for this entire wave** — the break would have been eaten by every other task riding that commit.
2. **Every alternative shape is worse.** Leaving the tunable dormant is banned in the same spec sentence and by `SC-§40` cl. 2. Shipping a non-compiling tree defeats the pipeline's one hard gate. There is no reading of item (5) that leaves the file alone.
3. **The file was genuinely unowned.** I re-ran the board census rather than accepting the author's: `TASK-910`'s `names:` lists `Tests/SiegeFogClampTest.cpp` and `SiegeFogStatics.h`, ⛔ not this file. `TASK-838` / `847` / `908` are closed. **No live row claims `Tests/SiegeFogTest.cpp` as a subject.** Confirmed.
4. **It is also the RIGHT file, not merely the forced one.** `SiegeFogClampTest.cpp`'s own header (`:20-25`) states it *"deliberately does NOT re-assert TASK-837's arithmetic"*. The file the row named therefore contains **almost none** of the rows Jonathan's ruling invalidates. A diff confined to `names:` would have left nine false assertions live and green.

**Precedent I am setting deliberately, so silence does not set a worse one.** A `names:` fence may be exceeded ONLY when all four hold: **(i)** obeying it makes a spec item literally unsatisfiable or produces a non-compiling tree; **(ii)** the extra file is unowned by any live row at the moment of the edit; **(iii)** the breach is declared in the handoff *before* QA reads the diff, with the measurement that forces it; **(iv)** the author asks for an explicit ruling instead of assuming one. All four hold here. **A breach missing any of the four is a BLOCKER.**

⇒ **Manager action:** add `Tests/SiegeFogTest.cpp` to `TASK-981`'s `names:` and to `TASK-986`'s subject list. Retroactive, bookkeeping only. ⚠️ **And see WARN-2 — the breach has a downstream consequence neither the author nor the dispatch caught.**

---

## 2. ⛔ THE NUMBERS — RE-DERIVED, NOT CONFIRMED

σ = −ln(0.02) / 609.6 = 3.9120230054 / 609.6 = **0.00641736 per uu** (header says `0.0064174` — correct rounding).

| d (uu) | ft | my independent value | ships as | law (FOG-§9.2) | verdict |
|---|---|---|---|---|---|
| 0 | 0 | exactly `0` | `0` exact | — | ✅ |
| 120 (melee) | ~4 | **`0.5370255`** | `> 0.5` asserted | — | ✅ |
| 152.4 | 5 | **`0.62393968`** | `1 − √√0.02` | 62.4 % | ✅ |
| 304.8 | 10 | **`0.85857864`** | `1 − √0.02` | 85.86 % | ✅ |
| 609.6 | 20 | **`0.98`** | `1 − 0.02` | 98.00 % | ✅ |
| 1219.2 | 40 | **`0.9996`** | `< 1.0` asserted | — | ✅ |

- ⭐ **`0.6239397` is CONFIRMED.** `1 − 0.02^(1/4) = 1 − 0.37606032 = 0.62393968`. The board's old `0.6238` missed it by **`1.397e-4`**, against `Tolerance = 1e-4` in `SiegeFogTest.cpp:133` — i.e. it **would have turned a correct build red**, and the obvious remedy (loosening the tolerance) is the one `qa/TASK-847.md` NIT-3 forbids. The author's catch is real and it is the most valuable single line in the handoff.
- ✅ **`0.6238` survives NOWHERE as a live prescription.** I swept `.claude/` for `0.6238` / `62.38`: the only surviving instances are inside explicit *"this row read 0.6238 and was CORRECTED"* notes on TASK-981 (`:18133`), TASK-841 (`:14635`) and TASK-986 (`:18283`), plus the handoff's own §2. All three rows now prescribe `0.6239397` / `62.39 %`. **Manager's correction is complete. No finding.**
- ⭐ **The `~53.7 %` melee figure is CONFIRMED** — `1 − exp(−0.00641736 × 120) = 0.5370255`. This is the one number Jonathan will see with his own eyes, and it is right. ⚠️ **But see the qualifier in §6 — he will not see it *yet*.**
- Concavity ratio **7.0711×** confirmed analytically: first-half gain / second-half gain = `(1−√T)/(√T−T) = 1/√T = 1/0.141421 = 7.0711`. Independent of tuning — a genuinely structural claim.

---

## 3. `SC-§60` — EACH RED CONTROL ADJUDICATED FOR VACUITY, INDIVIDUALLY

The dispatch's core question: **can each control actually go red against the shipped curve?** I worked each one by hand.

| control | site | what it kills | can it go RED? |
|---|---|---|---|
| 3 absolute anchors `0.9800 / 0.8586 / 0.6239` | `SiegeFogTest.cpp:467-474` | wrong σ, `t²`, linear, smoothstep, `sqrt(d/c)`, base-10 log, sign error | ✅ **YES** — absolute values, 200×–8600× tolerance separations |
| ceiling `< 0.99` | `:478` | σ derived from the **onset** (reads `0.9996`) | ✅ **YES** — 0.0196 above the bound |
| retired `t²` at the onset | `:484` | a tree still carrying `t²` (both read 0 ⇒ diff 0) | ✅ **YES** — separation `0.8586` vs a `0.5` bar |
| retired `t²` at the quarter | `:487` | same | ✅ **YES** — `0.6239` vs `0.5` |
| retired `t²` at the ceiling | `:492` | the retired **hard cut** on the picture | ✅ **YES** — `0.02` vs `1e-2` bar (200× tolerance) |
| `AtQuarter > 0.5` | `:498` | linear-in-distance (`0.25`) | ✅ **YES** |
| `AtHalf > 0.80` | `:501` | `sqrt(d/ceiling)` (`0.707`) | ✅ **YES** |
| **concavity, reversed** | `:512` | `t²` **asserts the opposite**; linear/smoothstep split 50/50 | ✅ **YES** — under `t²` the row reads `0 > 1` |
| concavity ≥ 2× | `:515` | same, harder | ✅ **YES** |
| descending equal steps | `:524`, `:527` | `t²` (ascending), linear (equal) | ✅ **YES** — `0.624 > 0.235 > 0.088`; under `t²` step1 = step2 = 0 |
| ⭐ `PositiveInsideTheOldOnset >= 20` | `:598` | **any curve that still has a clear bubble** | ✅ **YES** — 50 under Beer-Lambert, **0** under `t²` |
| far end `< 1.0` at 1200 uu | `:613` | a hard cut | ✅ **YES** |
| test 10(a) T→0.5 | `:1031`, `:1037` | ⛔ **a hardcoded `Sigma = 0.0064174f`** | ✅ **YES** — hardcoded σ reads `0.98`, row demands `0.50` |
| test 10(b) ceiling ×2 | `:1047`, `:1050` | same | ✅ **YES** — hardcoded σ reads `0.9996` / `0.98` |
| test 10(c) tightest legal ceiling | `:1060` | same | ✅ **YES** — hardcoded σ reads `0.8586` |
| test 10(d) positive control | `:1074` | a comparison that could never differ | ✅ **YES** — it is a real positive control |
| `FogDensityAt(` == 2 tree-wide | `ClampTest.cpp:1301` | a gameplay caller | ✅ **YES** — and I **re-measured it myself: exactly 2** (`SiegeFogStatics.h` decl + `.cpp` defn) |
| ⭐ its positive control `EffectiveVisionRadius(` ≥ 3 | `ClampTest.cpp:1319` | **a dead scanner reading as a clean pin** | ✅ **EXISTS AND IS ALIVE** — I re-measured ≥ 4 real hits; if the scanner died it returns −1 + `AddError` and this row goes red first |
| per-file zeros + `float` controls | `ClampTest.cpp:1338-1346` | a file that failed to load reading as clean | ✅ **YES** — 5 files, each with its own live control |
| funnel zero + its extraction control | `ClampTest.cpp:1358-1371` | an empty/mis-extracted body | ✅ **YES** |

**The `SC-§60` cl. 3 hazard is real and it was handled correctly.** The retired test 3 named its red controls as linear (0.50), smoothstep (0.50) and **Beer-Lambert (~0.63)** — the curve that now ships. Re-signing that row would have certified the winning curve as *wrong*. Nothing was inherited: the discriminator set is rebuilt, the concavity row asserts the **opposite direction** of what shipped, and the sweep's counter was **replaced** because the old `OpaqueSamples > 0` row would itself have gone RED against a correct implementation (Beer-Lambert reaches exactly 1 only past ~16,201 uu; the sweep stops at 1200). I verified that claim: at 1200 uu the curve reads `0.999548`, so the old row was genuinely unsatisfiable. **This is a correct re-derivation, not a re-signature.**

**Is the `Sqrt`-vs-`Loge`/`Exp` independence genuine, or a clever transcription?** Genuine. The expectation constructors (`:200-213`) call `FMath::Sqrt`; the subject (`SiegeFogStatics.cpp:77,83`) calls `FMath::Loge` + `FMath::Exp` — different libm entry points, different code paths. The only shared input is the ratio `0.02`, and the fixture writes it as **its own constant re-derived from "98 %"** (`:180`) rather than reading `Tuning.FogTransmittanceAtCeiling`. Crucially the expectations are pinned to **specific distances** (ceiling, ceiling/2, ceiling/4), so they encode the σ-derivation claim itself — a wrong σ misses them. ⚠️ I did **not** reproduce the author's *"agree to `0.000e+00` in float32"* (that needs execution, which is fenced). It does not matter: float32 error at these magnitudes is ~1e-7 against a `1e-4` tolerance, so the rows pass with ≥1000× headroom **whether or not** the agreement is bit-exact. The conclusion is robust to the unverified claim.

**Is the banned `√` RELATION test absent?** ✅ **YES, absent.** No row anywhere asserts `f(d/2) == √f(d)` or any function of one shipped sample against another shipped sample via `√`. Every `√` in the suite is inside an expectation *constructor*, producing an **absolute number**. Test 10(a)'s `1.f − FMath::Sqrt(0.5f)` is likewise absolute, under a *different* σ. **The `√` is the derivation path; the claim is always the value.** Spec item (3) honoured.

**The author's own worry — is test 3(b)'s ceiling row duplicated by test 2(d)?** No, and there are **three** independent witnesses to "the picture is not a hard cut at the ceiling", not one: `3(a)` (`AtCeiling == 0.98 ± 1e-4`, which a hard cut misses by 200× tolerance — this is the *strongest* of the three, and the author under-credited it), `3(b)` (`|AtCeiling − 1.0| > 0.01`) and `2(d)` (`< 1.0`), plus test 4's far-end row at 1200 uu. Different predicates, different distances, different files' worth of reasoning. **Not the same row twice.**

**The new divide-by-zero — I tried to reach the division.** I cannot. `FMath::Loge(Transmittance) / Ceiling` at `SiegeFogStatics.cpp:77` is reached only after: non-finite rejection (`:33`), `!(Ceiling > 0.f)` rejection (`:45`), and `!(T > 0.f) || !(T < 1.f)` rejection (`:55`). At the division, `Ceiling` is finite and strictly positive and `T ∈ (0,1)` strictly ⇒ `Loge` is finite and strictly negative ⇒ σ is finite and strictly positive. **No reachable divide-by-zero, no reachable NaN.** The guard order is correct and the guard is load-bearing, exactly as declared. (One residual: NIT-2.)

**Pins.** I re-measured what is measurable by reading: `FogDensityAt(` tree-wide shipping = **2** ✅ (new pin, correct); `FSiegeFogStatics::EffectiveVisionRadius(` = **2** ✅ (h:445 is unqualified and correctly excluded); `Ceiling <= 0.f` inside the extracted `EffectiveVisionRadius` body = **1** ✅ (`.cpp:114`); `Ceiling < 0.f` = **0** ✅ (`"Ceiling <= 0.f"` does not contain `"Ceiling < 0.f"` as a substring — checked character by character); `ClampMin = "304.8"` in the header = **1** ✅ (`.h:246`). ⭐ **The new `FogDensityAt` was placed BEFORE `EffectiveVisionRadius` in the .cpp, which keeps `ExtractFunctionBody`'s first-`\n}` scan clean.** That was not luck-proof and it happens to be right. **Zero pins moved — confirmed.**

**`FOG-§7b` both halves intact.** Half (a): `.h:246` `meta = (ClampMin = "304.8")` — present, unchanged. Half (b): `.cpp:114` `Ceiling <= 0.f` — present, unchanged, still `<=`. ✅ Neither touched.

**`EffectiveVisionRadius` / `IsVisibleThroughFog`.** Read in full. Shapes are exactly what the law and the surviving tests describe: `min`, not `clamp`; unconditional inert path; `<=` predicate delegating to the same function. ⚠️ Byte-for-byte identity is a Git claim I cannot make (fenced) — **I record the absence rather than assert it** (`SC-§40`). What I attest is **behavioural and structural identity by reading**, which is what the mechanic's safety depends on.

---

## 4. Findings

- **[WARN-1]** `Tests/SiegeFogTest.cpp:956` — `NegativeCeilingCurve.FogVisionCeilingUU = -609.6f;` introduces a **code literal of a pinned distance** into a file whose own header (`:30-38`) states *"NEITHER `304.8` NOR `609.6` APPEARS ANYWHERE IN THIS FILE AS A CODE LITERAL (only in prose…)"* and *"there is exactly ONE `609.6` and ONE `304.8` in the codebase, and both are in FSiegeFogTuning"*, and whose subject header (`SiegeFogStatics.h:167-168`) repeats *"There is exactly ONE `609.6` in the codebase and it is this line."* — **The line sits inside the (c2) block this diff created**, so the diff falsified its own file's stated discipline. No test goes red (`CountAcrossShippingSource` excludes `/Tests/`, and nothing pins `609.6` tree-wide), and the value is arbitrary — any negative works. ⛔ **Not the same paragraph as `qa/TASK-908.md` WARN-3** (that is the ONSET's paragraph and is `TASK-910`'s); **the ceiling's paragraph is unowned**, so this is a genuinely new, unrouted contradiction. — **Fix: `= -DerivedCeilingUU;`** (the fixture constant is already in scope at `:114`). One token.

- **[WARN-2] ⛔ CROSS-LANE — must reach the manager BEFORE `TASK-995` is dispatched.** The fence breach is correct (§1) but it collides with two **live negative fences** nobody flagged. `TASK-995`'s `names:` reads verbatim: *"⛔ **MUST BE UNTOUCHED: `SiegeFogStatics.h` · `Tests/SiegeFogTest.cpp` · `Range`/`AttackRange`**"*, and `TASK-994`'s cancellation note (`TASKBOARD.md:18425`) extends the same prohibition to `TASK-993` and `TASK-979`. **`TASK-993` ships on the same host, `TASK-987`.** ⇒ `TASK-995`'s reviewer will open a tree in which both named files are heavily modified and, with no note, will file a **false BLOCKER against a correct `TASK-993` diff** — precisely the expensive error `SC-§59` cl. 5 names. — **Fix (manager, board-side, zero code):** annotate `TASK-995` (and `TASK-979`) that the fog-curve diff in those two files is **`TASK-981`'s, gated by `TASK-986`**, and that `TASK-995` must attribute **by content, not by dirtiness**. ✅ **Mitigating measurement I took so the note can be precise: every coordinate those rows cite is still exact** — `Tests/SiegeFogTest.cpp:232` (`LongbowmanRange = 3600.f`), `:654`, `:691-692` (the `83.1 %` rows), `SiegeFogStatics.h:176` (the table row), `:187` (the 50,000 uu arena), `:67` and `:471` (the "lethal shot" prose) all resolve to the cited lines unchanged. Only `:340` drifted to `:340-341`. **The Longbowman lane's line numbers survived the rewrite intact.**

- **[WARN-3] ⛔ CROSS-LANE — must reach the manager BEFORE `TASK-980` is dispatched.** `Tests/SiegeFogClampTest.cpp:699-714` pins `FSiegeFogStatics::EffectiveVisionRadius(` at **exactly 2** across shipping source, and its own failure text reads *"⛔ A THIRD hit is a per-site clamp … an automatic QA FAIL. ⛔ Do NOT fix a red here by clamping at a new site."* — **`TASK-980` spec item (1) prescribes adding exactly such a call**: `float ASummonedUnit::GetEffectiveFiringRangeUU() const` returning `FSiegeFogStatics::EffectiveVisionRadius(...)` in `SummonedUnit.cpp` (`TASKBOARD.md:18072`). ⇒ **as boarded, `TASK-980` turns that test red and the test's own message forbids the fix.** This pin is pre-existing (`TASK-838`/`867`) and was **correctly not moved** by TASK-981 — the author even declined to duplicate it, using `>= 3` for its new positive control (`ClampTest.cpp:1313-1323`), which is the right call and I endorse it. But the collision is live and cheap only until `TASK-980` is dispatched. — **Fix (manager):** amend `TASK-980` to bump the pin `2 → 3` **deliberately, in the same diff**, with the firing-vs-acquisition distinction written into the test's message; add the expectation to `TASK-985`'s gate so its reviewer does not read a deliberate bump as the forbidden per-site clamp.

- **[WARN-4]** `Tests/SiegeFogTest.cpp:439` — the C++ identifier is still `FSiegeFogFalloffAcceleratesTest` while the row it implements now asserts the falloff **decelerates** (`:512`, `:524`) and its reported automation name was changed to `…DeceleratesAndIsNotTheRetiredQuadratic` (`:440`). The author's `SC-§60` cl. 2 reasoning ("keep the `IMPLEMENT_` identity") is sound but incomplete: the *reported* identity already changed, so retaining the stale class name buys nothing and leaves a symbol that names the losing side of the ruling. A future `grep Accelerates` lands on the test that proves the opposite. — **Fix: rename the class** (e.g. `FSiegeFogFalloffIsBeerLambertTest`; the symbol is file-local, I checked — zero external references), **or** add one line saying why the stale name is retained. Non-blocking.

- **[NIT-1]** `Tests/SiegeFogTest.cpp:958` (negative ceiling) and `:975-976` (transmittance `1.f` and `2.f`) **cannot go red against a missing guard** — the trailing `FMath::Clamp(…, 0.f, 1.f)` at `.cpp:83` absorbs the negative density to `0` anyway. They correctly assert the *contract*, but they are not discriminators. The real red controls in that block are the **zero ceiling** (`:952` — unguarded, σ = +inf ⇒ returns 1.0, a whiteout) and **transmittance `0.f` / `-0.5f`** (`:973-974` — unguarded, whiteout / NaN). Worth one sentence in the block so a future reader does not over-credit the decorative rows.

- **[NIT-2]** `SiegeFogStatics.h:367` / `.cpp:29` prose — *"every degenerate input fails toward **CLEAR**, never toward a whiteout"* has exactly one exception I could construct: a **positive but sub-`1.2e-38` ceiling** overflows σ to `+inf` ⇒ `Exp(-inf)` = 0 ⇒ returns **`1.0`, a whiteout**. Unreachable in practice (`ClampMin = "304.8"`, and nothing instantiates `FSiegeFogTuning` outside a default construct today — see NIT-3). Recorded because the totality claim is stated absolutely. Cheapest honest fix is one word in the comment, not a code change.

- **[NIT-3]** ⭐ **A UE-correctness hazard that could have bitten and does not:** retiring a `UPROPERTY` from a `USTRUCT` orphans any serialized instance and normally wants a `CoreRedirects` entry. I checked — **`FSiegeFogTuning` appears in `SiegeCombatStatics.h` only as a forward declaration (`:19`) and an out-parameter (`:416`)**, and nowhere as a `UPROPERTY` on any `UCLASS`. ⇒ **no asset can be carrying a serialized `FogDensityExponent`, so no redirector is needed.** Declared so nobody adds one, and so `TASK-839`'s `AFogVolume` author knows this window closes the moment the struct becomes a member.

- **[NIT-4]** `meta = (ClampMin = "0.001", ClampMax = "0.99")` on `FogTransmittanceAtCeiling` (`.h:303`) are the **author's numbers, not Jonathan's** — correctly declared in handoff §6(6) rather than hidden. Accepted: they are dimensionless slider stops applying `FOG-§7b`'s ruling class to the adjacent input, the `.cpp` guard is total regardless, and both stops sit far from any plausible tuning. Recorded so they reach the manager as a *declared choice* rather than becoming law by silence.

- **[NIT-5]** `SiegeCombatStatics.cpp:123` — confirmed **still standing**, quoted from `Read`: `// exponent 2), which is also what AFogVolume's instance will default to.` (the sentence begins `:122`, *"A default-constructed tuning IS the shipped tuning (609.6 / 304.8 /"*). It names a tunable that no longer exists. ✅ **It HAS an owner:** `TASK-980` item (1a) (`TASKBOARD.md:18073`) already carries it as a comment-only rider. Correctly fenced out of TASK-981 and correctly routed. **No action for this task.** ⚠️ But `TASK-980` is blocked behind `TASK-978` + `TASK-979`, so the stale comment will sit in the tree across at least one commit — acceptable, and now recorded twice.

---

## 5. Notes for build-master (TASK-987)

1. ⛔ **DO NOT reconcile against `441 / 33`.** That count was accurate when the author measured it, but a parallel lane has landed since. **Live re-measurement, taken by me with the `TL-§5b` scoped pattern (`^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp`): `445` across `34` files.** The delta reconciles exactly: `439` executed at `84eec02` **+ 2** (TASK-981: `SiegeFogTest` 9→10, `SiegeFogClampTest` 8→9 — I verified both) **+ 4** (`SiegeUnitNoticeRangeTest.cpp`, a new file from the TASK-979 lane) = **445**. ⭐ **TASK-981's own delta of +2 is CONFIRMED and is not the discrepancy.**
2. ⛔ **`TL-§5c`: `445` is a DECLARED count. Nothing has been compiled or executed this wave.** The author wrote `441 declared` and never `441/441` — **correct, and I have not written an executed number either.** The first execution of the rewritten curve suite is owed by you.
3. ⚠️ `TL-§5b`'s bare-pattern trap is live on this tree: the bare `^IMPLEMENT_` form over `Source/**/*.cpp` gives a different pair. **Use the scoped form.**
4. **UE 5.8 API — verified at source, not assumed:** `FMath::Exp(float)` at `Engine/Source/Runtime/Core/Public/GenericPlatform/GenericPlatformMath.h:487` and `FMath::Loge(float)` at `:495`. Both `[[nodiscard]]`, both used for their return value. The `Exp(volatile float)` variant at `:485` is inside `#if PLATFORM_WINDOWS && PLATFORM_CPU_ARM_FAMILY` — **inactive on x64, so there is no overload ambiguity.** Nothing in this diff is deprecated or removed in 5.8.
5. **All 19 automation test name strings in the two fog files are unique tree-wide** — checked. No duplicate-registration hazard from the two new tests.
6. **Nothing in this diff is reachable at runtime.** `FogDensityAt` has **zero callers** (pin = 2 = declaration + definition), and `ReadFogState` still returns `false` until `TASK-839`. This commit changes no pixel and no engagement in the shipped game.
7. ⚠️ **WARN-2 is yours as much as the manager's:** if `TASK-993` rides this same commit, say in the handoff which files belong to which task, or `TASK-995` will mis-attribute `SiegeFogStatics.h` and `Tests/SiegeFogTest.cpp`.

## 6. For Jonathan — the consequence, sanity-checked

**The melee band is now ~53.7 % obscured *visually* where the old curve rendered it perfectly clear. The figure is CORRECT — I recomputed it: `1 − exp(−0.00641736 × 120) = 0.5370`.** The **mechanic is untouched**: `EffectiveVisionRadius` is a `min`, `120 < 609.6`, and the suite asserts the 120-uu reach comes back bit-identically under fog (`SiegeFogTest.cpp:374`). Every melee unit still fights at exactly the range it always did.

⚠️ **Two honest qualifiers he should have with the number:**
1. **He will not see it yet.** `FogDensityAt` has zero callers and `TASK-841`'s visual is still blocked by `TASK-858` (`L_Arena`'s `bEnableVolumetricFog = false`). This is a *future* consequence, correctly computed, **not yet observable on pixels.**
2. **This is arithmetic, not a choice anyone made after his ruling.** 98 % at 20 ft forces 86 % at 10 ft forces ~54 % at 4 ft — one σ, one curve. It is the same square-root relation he was shown and accepted. If ~54 % haze at sword range is not what he pictured, the knob is `FogTransmittanceAtCeiling` and it is `EditDefaultsOnly` — one number, no recompile of anything but defaults.

---

## 7. ⛔ THE BOARD FLIP I COULD NOT MAKE — RECORDED, NOT FAKED (`SC-§40`)

**`TASK-981`'s board status is still `ready-for-qa`. I did not change it, and this report is the authoritative verdict until someone does.**

⛔ **Why:** the `Edit` tool is **disabled for this session** (confirmed by attempting it). The only remaining write path is `Write`, which would require reproducing all ~18,500 lines of `TASKBOARD.md` verbatim — with three parallel lanes live on the same file, that is a board write race with a far worse expected cost than a missing status word. **I declined it deliberately.**

⇒ **Orchestrator / manager, the flip to make on the `TASK-981` row:**

> `- status:` ✅✅ **qa-passed 2026-09-04** — `qa/TASK-981.md` (⛔ **0 BLOCKERS** · 4 WARN · 5 NIT) → ready-for-integration on **`TASK-987`**; feature gate remains **`TASK-986`**.
> 🔍 **QA RULED THE FENCE BREACH ACCEPTED, NOT A FINDING** — writing `Tests/SiegeFogTest.cpp` was FORCED (item (5) breaks its code refs ⇒ non-compiling tree ⇒ `TASK-987`'s one compile eaten) and the file was UNOWNED (census re-run independently). ⛔ **The `names:` list is the defective artefact.** Four-part precedent in `qa/TASK-981.md` §1. 📌 **MANAGER: add `Tests/SiegeFogTest.cpp` to this row's `names:` and to `TASK-986`'s subjects.**
> ⭐ **QA re-derived the numbers rather than confirming them:** `609.6` ⇒ `0.98` · `304.8` ⇒ `0.85857864` · `152.4` ⇒ `0.62393968` · melee `120` ⇒ `0.5370255`. `0.6238` survives nowhere as a live prescription. `FOG-§7b` both halves intact · `√`-relation ABSENT · `FogDensityExponent` 0 code refs · `FogDensityAt(` = 2 with a LIVE positive control · **ZERO pins moved.**
> ⚠️⚠️ **TWO CROSS-LANE WARNs, unanticipated, both cheap only until dispatch:** **W-2** `TASK-995` names these two files *"MUST BE UNTOUCHED"* and `TASK-993` ships on the SAME host ⇒ false-BLOCKER risk; **W-3** `SiegeFogClampTest.cpp:699-714` pins `FSiegeFogStatics::EffectiveVisionRadius(` at **2** and forbids a third, while `TASK-980` item (1) prescribes adding exactly a third ⇒ **`TASK-980` as boarded turns that test red and the test's own message forbids the fix.**

Slack mirror posted to ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`). **This file remains the authoritative verdict; Slack is the mirror.**
