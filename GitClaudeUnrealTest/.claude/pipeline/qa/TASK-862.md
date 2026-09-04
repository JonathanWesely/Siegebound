# QA Report — TASK-862 (the gate over `TASK-860` — the two-ended cast bar, the C++ half)

**Verdict: PASS** — **0 BLOCKER · 4 WARN · 5 NIT**
Gated: 2026-09-03 · Law: `WITCH-§9` · `SHIP-§9` · `SC-§29` · `SC-§37` · `SC-§38` · `SC-§39` · `SC-§40` · `SC-§41` · `TL-§5c` · `HIGH-§1`

---

## ⛔ `SC-§29` — THE COVERAGE LEDGER

**COVERED (this gate, and only this):** `TASK-860`'s diff.
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` / `.cpp` — the eight cast seams
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarWidget.h` — the one `BlueprintImplementableEvent`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCastBarTest.cpp` — **NEW**, 1 607 lines, **15** tests

**⛔ NOT COVERED, BY DESIGN — do not read this verdict as covering them:**
- `TASK-830`'s cast logic (`SummonedUnit.{h,cpp}`, `HealthBarProvider.h`) — **`qa/TASK-849.md`'s**. ⛔ Not re-gated. I read `HealthBarProvider.h` only to verify `860`'s own premise gate (0a), and I read `SummonedUnit.cpp`'s percent arithmetic **not at all** — the replay harness's model of it is taken from `849`'s upheld ruling, which is declared below as an **assumed** premise.
- `TASK-861`'s UMG asset — **ART**, gated by `TASK-835`'s integration check, not by me.
- `TASK-863` (the rig — deferred), all of Lane F.
- The **two pre-existing red rows** (`SiegeAcquisitionFunnelTest` tests 1 and 9, owned by `TASK-868`/`TASK-882`). Not this diff's, not gated here, and I confirmed `860` did not add a third.

**⛔ NOT EXECUTED (`TL-§5c`):** ⛔ no compile · ⛔ no editor · ⛔ no MCP (editor UP, PID 22940, art task running) · ⛔ no Git · ⛔ no suite run. **Every number below is STATIC and was re-derived by me on disk.** The suite delta is **DECLARED**, never a pass count.

---

## ⛔⛔ THE RESUMED AGENT'S CENTRAL CLAIM — VERIFIED INDEPENDENTLY, AND ⛔ NOT VIA ITS BRACE COUNT

The dispatch was explicit: the same outage left a sibling tool with a **present-tense docstring for two functions that did not exist**, and it compiled, *because prose is not a syntax error*. ⇒ **I did not accept balanced braces as evidence of completeness.** I opened and read **every one of the eight declared cast seams' bodies** and traced the call graph end to end.

| declared seam | header | definition | body read | behaviour implemented? |
|---|---|---|---|---|
| `ShouldPollCastProgress` | `.h:273` | `.cpp:519` | ✅ | forwards to `ShouldPollOcclusion` — real |
| `ShouldPushCastRow` | `.h:287` | `.cpp:528` | ✅ | `bCasting \|\| bRowAlreadyDriven` — the falling edge is really there |
| `SanitizeCastPercent` | `.h:304` | `.cpp:539` | ✅ | gate→0, `IsFinite` branch, `Clamp(0,100)` — all three present |
| `ComputeCastBarHeightPixels` | `.h:315` | `.cpp:564` | ✅ | `Max(row,0)` + `RoundToFloat` — both present |
| `ComputeCastPivotY` | `.h:340` | `.cpp:575` | ✅ | zero-guard + the bottom-hold formula — present and **arithmetically correct** |
| `UpdateCastProgress` | `.h:436` | `.cpp:594` | ✅ | reads `Cast<IHealthBarProvider>(GetOwner())`, both getters, gates, pushes |
| `PushCastProgress` | `.h:444` | `.cpp:617` | ✅ | latch → geometry → **`LiveBar->OnCastProgressChanged(...)`** → `RequestRedraw` |
| `ApplyCastRowGeometry` | `.h:452` | `.cpp:648` | ✅ | self-gate → `SetDrawSize` → `SetPivot` |

**⇒ 8 declared / 8 defined / 8 with real bodies. ⛔ No prose-only seam. And the CALL GRAPH closes:**
`TickComponent:225` → `ShouldPollCastProgress` → `:227 UpdateCastProgress` → `:614 PushCastProgress` → `:626 ApplyCastRowGeometry` **and** `:641 OnCastProgressChanged`. **⛔ There is no dead seam and no unreachable branch.** The seed path closes too: `BeginPlay:191-192` → `PushCastProgress(bSeedCasting, …)`.

**⇒ The claim "the source half was complete and correct on arrival" is CONFIRMED — on behaviour, not on braces.** Its corollary ("my entire diff is one new test file") I can corroborate but **not prove**: I have no Git in my lane and the session's `git status` snapshot is stale (it describes a castle batch from an older sitting). What I *can* say is that every shipped line I read is consistent with a component that gained a cast row and lost nothing — and every pre-`860` count-pin that touches this file still reads its pinned value (below).

---

## ⛔⛔ THE TWO REGRESSION TRAPS — BOTH HANDLED. ⛔ MEASURED, ⛔ NOT READ OFF A FUNCTION NAME.

### (4a) THE FLEET-WIDE ONE — ✅ **HELD, AND I MEASURED IT IN BOTH DIRECTIONS**

⛔ I did **not** verify this by reading the name `ApplyCastRowGeometry`. I swept the **whole `Source/` tree** for both writers:

| needle | tree-wide hits | verdict |
|---|---|---|
| `SetDrawSize` on `CombatantHealthBarComponent.cpp` | **`:57`** (constructor) · **`:669`** (`ApplyCastRowGeometry`) — code lines; `:570` is a `//` comment | ✅ |
| `SetPivot` on `CombatantHealthBarComponent.cpp` | **`:671` ONLY** (`ApplyCastRowGeometry`) | ✅ **the constructor never sets Pivot** |
| either, from **any other file** on this component | **0** (`Castle.cpp:288` and `DamageNumberActor.cpp:32` write their **own**, different widget components) | ✅ |
| the constructor's constant | `.cpp:28` `CombatantHealthBarDrawSize(90.f, 22.f)` | ✅ **UNCHANGED** |

**⇒ then I traced the two paths a NON-CASTING actor can take, because that is the claim:**
1. **`BeginPlay`** — `:106-107` captures `CastBarBaseDrawSize`/`CastBarBasePivot` from the **live** component; `:192` seeds `PushCastProgress(false, 0)` → `ApplyCastRowGeometry(false)` → `TargetDrawSize == CastBarBaseDrawSize == GetDrawSize()` ⇒ **`:658` early-outs. ⛔ ZERO writes.**
2. **Every tick thereafter** — `ShouldPushCastRow(false, false)` is `false` ⇒ `PushCastProgress` is **never called** ⇒ `ApplyCastRowGeometry` is **never reached**. **⛔ ZERO writes, for the actor's whole life.**

**⇒ ✅ On every building, the hero and all 20+ non-casting units, `DrawSize` and `Pivot` are BIT-IDENTICAL to the pre-`860` component.** The claim survives independent measurement.
⭐ **Belt-and-braces I add for the record:** `UWidgetComponent::SetDrawSize` is **itself** idempotent (`WidgetComponent.cpp:2245` — `if (NewDrawSize != DrawSize)`), so even a hypothetical redundant call could not dirty render state. The self-gate is a cost guarantee, not the correctness guarantee.
⭐ **And the 19.33 px is real, not rhetoric:** with `CastBarRoot` collapsed and `DrawSize (90,30)`, the remaining 29 px split `Fill 1:2` = **9.67 / 19.33**. I re-derived it from `TASK-861`'s measured slot values (`BoostOutline {value 1, Fill}` pad `{0,0,0,1}`; `Bar {value 2, Fill}` pad 0). **The trap was real and item (4a) was built to the amendment.**

### (4b) THE PIVOT RIDER — ✅ **CORRECT. I RE-DERIVED THE ARITHMETIC AND VERIFIED THE ENGINE MECHANISM AT SOURCE.**

Shipped, `.cpp:589-591`:
```cpp
const float BottomOffsetPixels = (1.f - BasePivotY) * BaseBarHeightPixels;
return 1.f - (BottomOffsetPixels / GrownBarHeightPixels);
```
**The mechanism, verified in the engine rather than taken from the handoff** — `SWorldWidgetScreenLayer.cpp:178-184`:
```cpp
FVector2D ComponentDrawSize = Entry.WidgetComponent->GetDrawSize();
FVector2D ComponentPivot    = Entry.WidgetComponent->GetPivot();
...
CanvasSlot->SetAlignment(ComponentPivot);
```
⇒ **`Pivot` IS the canvas alignment, and both are re-read EVERY FRAME** — which is also why `SetPivot` (`WidgetComponent.h:313`, a plain inline assignment with **no** `MarkRenderStateDirty`) is sufficient here. An alignment `A` spans a box of height `H` over `[anchor − A·H, anchor + (1−A)·H]`.

| | pivot | top edge (above anchor) | **bottom edge (below anchor)** |
|---|---|---|---|
| not casting | `0.50000` | 11.00 px | **11.00 px** |
| casting | `1 − 11/30 = 0.63333` | **19.00 px** | **11.00 px** |
| Δ | — | **+8.00 px UPWARD** | **0.00 — UNMOVED** |

**⇒ ✅ The +8 px is spent ENTIRELY upward, 0 px into the unit's head. The arithmetic and the claim both check out.**
✅ **Both refusals endorsed.** A constructor `Pivot=(0.5,1.0)` really does shift **every** bar up `(1−0.5)·22 = 11 px` permanently — the exact `WITCH-§9.6` regression. A `BarHeightZ` compensation really is in **world units** and therefore correct at exactly one camera distance. **Choosing neither obvious option was the right call, and it is the strongest single decision in this diff.**
✅ The `RoundToFloat` rider is load-bearing and correct: `SetDrawSize` truncates (`WidgetComponent.cpp:2243`), so an un-rounded height would disagree with its own pivot. ⭐ Note the base is captured from `GetDrawSize()`, which returns the **`FIntPoint`** `DrawSize` (`:2231-2234`) ⇒ always integral ⇒ `Equals` is exact and the bar **cannot** creep.

### (0b) THE COUNT-PIN TRAP — ✅ **UNTOUCHED, AND THE PIN WAS NOT LOOSENED**

| pin | owner | re-measured by me | verdict |
|---|---|---|---|
| `SetVisibility(` in `CombatantHealthBarComponent.cpp` **== 1** | `SiegeHealthBarOcclusionTest:925` | **1** code-line (**raw 3** — `:424`, `:682` are `//` comment lines) | ✅ |
| `bBarShownByOwner =` **== 2** | same, `:893` | **2** (`:683`, `:692`) | ✅ |
| `LineTraceSingleByChannel` == 1 · `LineTraceTestByChannel` == 0 · `bFindInitialOverlaps` == 0 | same, `:971-984` | **1 · 0 · 0** (`:365`'s `bFindInitialOverlaps` is a `//` line) | ✅ |
| `bStartPenetrating` / `ComputeOcclusionFromTraceResult` / `UpdateHealthBarOcclusion` **> 0** | same | all > 0 | ✅ |
| `HealthBarOcclusionIntervalSeconds` default `0.15` | same, `:661` | 0.15 | ✅ |
| `virtual float GetCastProgressPercent() const override;` tree-wide **== 1** | `SiegeInvisibilityTest:2923` | the component **calls**, never overrides; its header names **neither** getter (**0 hits**) | ✅ |
| `GetCastProgressPercent`+`IsCastInProgress` **== 0** in `Building.h`/`HeroCharacter.h` | same, `:2948-2976` | that sweep is scoped to those **two files only** — not this header | ✅ |
| `IsCastInProgress` tree-wide **`> 0`, not an absolute** | same, `:3102` | a control, not a pin — written that way on purpose | ✅ |

⭐ **AND THE ONE NOBODY LISTED, WHICH I WENT LOOKING FOR BECAUSE IT IS THE SHAPE OF THE TRAP:** `ShouldPollCastProgress` **adds a second call site** to `ShouldPollOcclusion`. I swept `SiegeHealthBarOcclusionTest.cpp` for a count-pin on `ShouldPollOcclusion(` and for **any** structural probe of `TickComponent` — **there is neither** (`:138`, `:575`, `:588` are direct *calls* from test bodies; the only `ExtractFunctionBody` targets are `ShowBarIfEnabled()` and `HideBar()`, both untouched and still extractable). ✅ **The forward cannot perturb it.**

✅ `CastBarRoot` / `CastBarFill` on code lines: **0 / 0**. The collapse is left entirely to `TASK-861`, as `849` ruled.
✅ **`TASK-849`'s scoped ban was NOT widened.** Its §16 ruling reads *"a tree-wide ban would go red the moment `TASK-860` consumes the surface — in a test naming neither `860` nor its file — and the 'fix' would be to loosen it."* Test 13's needles are scoped to `ComponentHeaderPath` alone. ✅ Correct.

---

## ⭐ THE THREE SELF-CAUGHT DEFECTS — ALL THREE FIXES VERIFIED

**1. The LWC `TestEqual` ambiguity (a hard compile error).** ✅ **FIXED, and I confirmed it is COMPLETE.** `FVector2D` is `UE::Math::TVector2<double>` ⇒ `.X`/`.Y` are doubles; beside float literals the `float`/`double` `TestEqual` overloads are genuinely ambiguous. Swept the whole test file for `.X`/`.Y`: **the only four component reads are `:669-672`, all four explicitly `static_cast<float>`**. ⛔ **Zero remaining un-cast uses.** Every downstream expression (`:677`, `:681`, `:690`, `:693`, `:709`, `:715`, `:717`) is float-on-float.

**2. The 3-frames-is-exactly-0.05 s test bug.** ✅ **FIXED, and the corrected numbers reproduce.** `3 × (1/60)` in float is `0.050000001` ⇒ **≥ 0.05 ⇒ it FIRES** — that row really would have been red on arrival for the crime of the gate working. Corrected to **two** frames: `2 × (1/60) = 0.033333336 < 0.05` ⇒ **0 fires** ✓. And the burst ceiling: from `0.033333336` the next 20 frames fire at frames 1, 4, 7, 10, 13, 16, 19 = **7**, which clears the `<= 8` ceiling and is exactly the declared figure. ✓ **The discrimination is real** — a `-= Period` implementation banks 0.95 s and fires on the very next frame and ~19 of the 20.

**3. ⭐ THE SUBSTRING COLLISION — REPRODUCED EXACTLY, AND IT EARNS A CLAUSE (ruling below).** `CastPercent` on code lines in `CombatantHealthBarComponent.h`: **3 hits on 2 lines** — `:304` (`SanitizeCastPercent` **and** `ProviderCastPercent`) and `:444` (`RawCastPercent`). ⛔ **Banning the bare word WOULD have been red on arrival.** The shipped member-shape ban measures clean and its positive control is live: `Percent;` = **0** · `Percent =` = **0** · `Accumulator = 0.f;` = **2** (`:640`, `:663`). ✅ **To the programmer's §9 question, answered NOW as asked: NO — do not make it the bare word. The member shape is correct.**

---

## ⭐⭐ `SHIP-§9` — TESTS 7 + 8. **THE DISCRIMINATION IS REAL.** I RE-DERIVED THE WHOLE REPLAY BY HAND.

`MakeCastRun` samples at `PollIndex × 0.05` for `PollIndex = 1…`, breaking at `>= EndSeconds`, then appends exactly one `(false, 0)`.

| run | live samples | peak | **events** | row-down | last event |
|---|---|---|---|---|---|
| **COMPLETED** (3.00 s) | idx 1…59 (2.95 s) | `100×2.95/3` = **98.333 %** | **60** | **1** | `(0.0, false)` |
| **BROKEN @ 1.25 s** | idx 1…24 (1.20 s) | `100×1.20/3` = **40.000 %** | **25** | **1** | `(0.0, false)` |
| COMPLETED, **buggy `if (bCasting)`** | — | 98.333 % | **59** | **0** | `(98.33, true)` ⛔ frozen |
| BROKEN, **buggy** | — | 40.000 % | **24** | **0** | `(40.00, true)` ⛔ frozen |

**Every figure reproduces**, including the float edges (`25 × 0.05f = 1.25000002 ≥ 1.25` ⇒ breaks; `60 × 0.05f = 3.00000004 ≥ 3.0` ⇒ breaks). **The 58.3-point gap is real and the `> 50.f` threshold is not tuned to squeak past it.**

✅ **Test 8 is a genuine `SHIP-§9` pair, not a restatement of test 7.** It replays the **identical two sample streams** through the modelled wrong gate and asserts three independent facts: row-down count **0** in both, both ending `bCasting == true`, and the shipped-minus-buggy event delta **exactly 1** — plus `(d)`, the positive half (shipped emits **2** terminal events across the two runs), so the row cannot pass by both implementations being broken the same way. ⛔ **Test 7 alone would have passed the buggy gate.** That is precisely what `SHIP-§9` exists to catch, and it is caught.

✅ **`§9` item 3, answered: the `bUseShippedGate == false` branch is a MODEL, not dead production surface.** It lives inside `SiegeCastBarTestFixture::ReplayCastRun`, inside `#if WITH_DEV_AUTOMATION_TESTS`, in a `Tests/` file, has **exactly two callers** (both in test 8), and is invisible to every tree-wide sweep — I verified the mechanism rather than assuming it: `SiegeInvisibilityTest.cpp:312-317` `IsAutomationTestFile` matches any path containing `/Tests/`, and `CountAcrossShippingSource:325` excludes them. ✅ **`SiegeCastBarTest.cpp` cannot perturb a shipping-source count.**

**⛔ No test passes trivially.** Every row that could has an explicit guard: `Completed.Num() < 2` (test 7), `BuggyCompleted.Num() == 0` (test 8), a null-CDO abort (tests 5 and 9), a null-`FFloatProperty` abort (5c, 9a), `GeometryBody.Len() > 200` (6), the four-symbol positive control (14), the `IsFinite(NaN)` self-check (2d — which would catch a fast-math-folded constant), and anti-vacuity pairs in 1, 2, 3, 4, 9 and 10.

---

## ⛔ THE ROW'S AUTOMATIC FAILS — ALL FIVE CHECKED, NONE PRESENT

| automatic fail | measured | verdict |
|---|---|---|
| a new worldspace widget / `UWidgetComponent` / `WBP_WitchCastBar` | tree-wide `CreateDefaultSubobject<UWidgetComponent>` = 4, **all pre-existing** (`Castle.cpp:285`, `DamageNumberActor.cpp:29`, 2 in `Variant_Combat/`); `WBP_WitchCastBar` appears **once**, in a comment forbidding it | ✅ **EXTENDED, not duplicated** |
| an **enum** in the BIE signature | `void OnCastProgressChanged(float CastPercent, bool bCasting);` — float/bool only; test 15(c) also bans `enum` and `FLinearColor` in the declaration slice | ✅ |
| `CastPercent` on a `0..1` range | `SanitizeCastPercent` clamps to `0..100`; test 2(b) pins that `0.5` is **NOT** rescaled to 50 | ✅ |
| the witch writing to another actor's widget/provider | `ASummonedUnit` / `IncomingWitchCaster` / `WitchCastTarget` = **0 / 0 / 0** on code lines; the poll reads `Cast<IHealthBarProvider>(GetOwner())` | ✅ |
| a non-defaulted / pure provider virtual | `HealthBarProvider.h:141` `{ return 0.f; }` · `:158` `{ return false; }` — **read at source, both DEFAULTED, neither pure** | ✅ item (0a) GREEN |

**Item (3), the pixel-identical guarantee:** asserted three independent ways — a **runtime CDO read** (test 5a/5b: `(90,22)` and `(0.5,0.5)`), a **structural constructor probe** (test 6e: `CastBar` ×0, `SetPivot(` ×0, with a live positive control), and the **call-graph trace** above. ⛔ The `Collapsed` half is `TASK-861`'s and is correctly *not* asserted here.

---

## Findings

- **[WARN]** `SiegeCastBarTest.cpp:577-625` (test 4) + `:715-717` (test 5d) — **`ComputeCastPivotY`'s ENTIRE test coverage sits at `BasePivotY = 0.5`, the one value where `BasePivotY` and `(1 − BasePivotY)` are numerically identical.** ⇒ swapping those two terms — the single most plausible edit to `.cpp:589` — **passes tests 3, 4, 5 and 6 in full.** Worse, tests 4(a) and 4(b) are **algebraic identities of the shipped formula** (`(1−A′)H′ ≡ (1−A)H` and `A′H′ − AH ≡ H′ − H` both reduce to tautologies once `A′` is substituted), so they hold for *any* implementation of that shape; 4(c) pins the number `1 − 11/30`, which the swapped form **also** produces at `A = 0.5`. ⛔ **The shipped code is CORRECT — I derived it independently — this is a coverage hole, not a defect.** But it is a hole on a supported path: the base pivot is **captured from the live component** (`.h:665-672`, `.cpp:107`) *precisely so a BP override survives a cast*, and at a base pivot of `1.0` the swapped form drops the bar **22 px**. **Fix — one row:** `TestEqual(…, ComputeCastPivotY(22.f, 1.f, 30.f), 1.f, Tolerance)` (a bottom-anchored base must stay bottom-anchored), or any non-`0.5` base.
- **[WARN]** `SiegeCastBarTest.cpp:1324` — pins the **BARE TOKEN** `OnCastProgressChanged` across the whole component `.cpp`, against **`SC-§41` cl. 1** and **`SC-§39`'s mirror clause** — both of which this file's own helper comment cites at `:301-303`. A trailing `//` on a code line naming the symbol (the exact measured case that bought the clause: `TASK-851`'s `#include` tail) **manufactures a false hit**, turning a green row red for a reason not in the diff, with the tempting fix being to loosen the pin. **Fix — one character:** `TEXT("OnCastProgressChanged(")`. It counts the same **1** today (`.cpp:641`) and is prose-immune tomorrow. The very next assertion (`:1330`) already uses the paren form.
- **[WARN]** `SiegeCastBarTest.cpp:1280` · `:1366` · `:1369` — the absence pins `ASummonedUnit`, `CastBarRoot`, `CastBarFill` are bare tokens over a file that **already contains two of them**: `CombatantHealthBarComponent.cpp:491` names `ASummonedUnit` and `:184` names `CastBarRoot`. They read **0** *only* because those comments begin their lines with `//`. ⇒ **re-wrapping either comment — a documentation-only edit — turns test 11(b) or 12(b) RED.** `qa/TASK-849.md` already had to ship the standing instruction *"⛔ Do not re-wrap that call"* for this exact coupling. **Fix:** pin `Cast<ASummonedUnit>` where a call shape exists, and/or carry a one-line rider on `.cpp:184` and `:491` recording that the line must start with `//` because `SiegeCastBarTest` counts the token. (⛔ Out of this task's fence to apply to the `.cpp`; recorded for the manager.)
- **[WARN]** `CombatantHealthBarComponent.cpp:645` — `RequestRedraw()` is **deprecated in UE 5.8**: `WidgetComponent.h:258` carries `meta = (DeprecatedFunction, DeprecationMessage = "Use RequestRenderUpdate instead")`. ⛔ **Not a compile error** — the `meta` deprecation binds Blueprint only, C++ callers compile silently — and this matches the file's two shipped precedents (`HandleOwnerHPChanged`, `PushDamageBoost`). I also measured it **inert on this path**: `WidgetComponent.cpp:1281` gates the render-target draw on `Space == EWidgetSpace::World`, and this component is `Screen`, so `RequestRedraw` costs one bool write. **Modern replacement: `RequestRenderUpdate()`.** ⛔ **Do NOT fix inside `TASK-860`** — the two existing sites are out of its fence and a one-of-three edit is worse than none. Boarded here so the eventual sweep is **one** edit across three sites rather than three separate discoveries.
- **[NIT]** `CombatantHealthBarComponent.h:277-278` — *"every building, the hero and every non-witch unit answers (false, false) for the entire match and therefore makes **ZERO Blueprint calls, ever**."* Measured: **exactly ONE** — the unconditional seed at `BeginPlay:192`, which is deliberate and required by the `qa/TASK-005` major-2 seed-then-bind law the same file cites twice. The **poll** makes zero. `SC-§40` cl. 9: a number in shipped prose reads as a measurement. Suggest *"zero Blueprint calls after the seed"*.
- **[NIT]** `SiegeCastBarTest.cpp:583-601` — tests 4(a)/(b) should say in their own comments that they are **properties of the shipped formula**, so a later reader does not over-trust them as independent witnesses. 4(c) and 4(f) are the rows carrying the discrimination. (Same root as WARN-1; separate because the comment fix is free and the coverage fix is not.)
- **[NIT]** `SiegeCastBarTest.cpp:212-240` — `ReplayCastRun` **calls** the shipped statics but **models** the composition (latch-first, geometry-before-value), because `PushCastProgress` is `protected` and needs a widget and the house rule bans world fixtures under `Siegebound/Tests/`. The modelled half is compensated structurally by test 12(a) (`bCastRowDriven = bCasting;` == 1 and the `StripCommentLines` ordering probe), which is the right instrument. Recorded so nobody reads tests 7/8 as an **execution** of the shipped push path — they are not, and the handoff says so.
- **[NIT]** `CombatantHealthBarComponent.cpp:601` — `UpdateCastProgress` performs an interface `Cast<>` on **every fire** (~20 Hz × the roster ≈ 400 interface casts/second across 20 units). Negligible, and the correct trade against caching an owner pointer. Recorded only because the class comment quantifies the *per-frame* cost to the instruction and is silent on the *per-fire* one; a later reader re-measuring will want the honest figure.
- **[NIT]** `SiegeCastBarTest.cpp:1148` — test 10(b) exercises the 30 Hz floor with `0.f` only. A **negative** interval is equally typeable in a BP and takes the identical `FMath::Max` branch, so coverage is adequate — noted **only** so nobody re-raises it as a gap.

---

## ⚖️ THE RULING ASKED FOR — DOES THE SUBSTRING FORM EARN A CLAUSE?

### ✅ **YES — and it belongs to `SC-§41`, ⛔ NOT to `SC-§39`, and it must say the thing `SC-§41` cl. 1 does ⛔ NOT say.**

**Why it is genuinely new.** `SC-§39`'s two recorded directions are a **leading `/*` that HIDES a real hit** and a **trailing `//` that MANUFACTURES a false one** — both are *comment-vs-code* confusions. `SC-§41` cl. 1 adds the **open-paren discriminator**, which separates a **CALL from PROSE**. ⛔ **Neither covers this.** `CastPercent` inside `ProviderCastPercent` is not a comment, not prose, and not a call — it is a **real identifier on a real code line**, and every clause in the book reads it as a legitimate hit **because it is one**. `SC-§41`'s discriminator sits on the wrong side of the token and buys nothing here.

**What actually fixed it, stated precisely** (the programmer's own reasoning, which I checked and endorse): the needle was re-pointed from the **token** to the **syntactic role**. A *member* is `Name;` or `Name =`; a *parameter* is `Name,` or `Name)`; a *call* is `Name(`. Banning the role rather than the name is what makes the ban immune to any identifier that merely **contains** the word.

**Proposed `SC-§41` clause 7 — a needle is anchored on the side where the collision lives; when the collision is another IDENTIFIER, pin the SYNTACTIC ROLE, never the name.**
1. ⛔ **Before shipping any absence needle, GREP IT AND READ THE HITS.** *"I picked a token nothing should match"* is not evidence (`SC-§40` cl. 10's shape, applied to needles). ⭐ **`TASK-860` did exactly this and it is the only reason the row is green instead of red on arrival: `CastPercent` reads 3 on code lines in its own header, as a substring of the shipped `ProviderCastPercent` / `RawCastPercent`.**
2. ⛔ **If the banned token is a legitimate SUBSTRING of a shipped identifier, ⛔ NO amount of trimming the token helps and ⛔ `Symbol(` does not either.** Re-point the needle at the **role**: `Name;` / `Name =` for a member, `Name(` for a call.
3. ⛔ **Carry a positive control OF THAT SAME ROLE, IN THAT SAME FILE** — here `Accumulator = 0.f;` == **2**, which proves the role-anchored needle can still see a real member. Without it the two zeros are indistinguishable from a needle that matches nothing.
4. ⚖️ *Why this is worth a clause rather than a note: the failure is **silent in the safe-looking direction at authoring time and loud in the wrong place at run time.** A bare-word ban here goes red in a test named for a **stored-tell ban**, naming neither the substring nor the parameter — and the obvious "fix" is to delete the ban.*

📌 **And the corroborating datum, offered for the record:** `SC-§41` itself was **cited as settled law three times before a word of it was written** (its own cl. 6). ⇒ **whoever lands this clause should re-grep `CONVENTIONS.md` for `SC-§41` before citing it downstream.**

---

## `TL-§5c` — SUITE CENSUS, **DECLARED**, ⛔ NOT EXECUTED

**Instrument controlled in BOTH directions, because two red-row scares tonight both turned on NEEDLE errors:**

| instrument | result |
|---|---|
| `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` (column-0 anchored), `Siegebound/Tests/*.cpp` | **425 across 31 files** |
| `IMPLEMENT_\w*AUTOMATION_TEST` (**unanchored**, catches indented declarations **and** the `COMPLEX` variant) | **425 across 31 files** — ⭐ **the two agree exactly**, so nothing is hiding behind indentation or a different macro |
| `SiegeCastBarTest.cpp` alone | **15**, with **15 distinct class names and 15 distinct test paths**, all under `Siegebound.CastBar.` |
| ⇒ tree without the new file | **410 across 30 files** |

⭐ **Reconciled against an INDEPENDENT prior measurement rather than against the handoff:** `qa/TASK-849.md` N-6 recorded **410 across 30 files** at its own sitting. **⇒ the delta is `+15`, all of it in one new file, and the baseline is corroborated by a gate that had no stake in this number.**
⛔ **`declared`. ⛔ NOT a pass count. I have no shell in this lane: I did not compile, did not launch the editor, and did not run the suite.** What stays **DECLARED and OWED to `TASK-835`/the build-master: that these 15 rows COMPILE and PASS**, and that the tree's total is 425 **passing** rather than 425 **declared**.
⚠️ `FSiegeCastBar*` appears in **exactly one file** (30 hits = 15 × the macro + 15 × `RunTest`) ⇒ no class-name collision. The fixture namespace is `SiegeCastBarTestFixture`, distinct from `SiegeHealthBarOcclusionTestFixture` (`SiegeHealthBarOcclusionTest.cpp:110`) ⇒ ⛔ **no ODR clash under a unity build**, which is the one place replicated helpers bite.

---

## Compile-by-inspection (⛔ nothing was built — this is a READING)

✅ **No deprecated or removed UE 5.8 API is introduced** beyond the inherited `RequestRedraw` (WARN-4). `FindFProperty<FFloatProperty>` is the correct UE5 replacement for `FindField<UFloatProperty>`; `FMath::RoundToFloat` / `Clamp` / `IsFinite` / `Max`, `ParseIntoArrayLines`, `FPaths::ProjectDir`, `FFileHelper::LoadFileToString` are all current.
✅ `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter` — identical to the 12 shipped uses in `SiegeHealthBarOcclusionTest.cpp`, which compiles today.
✅ Includes are sufficient for every symbol used (`HAL/UnrealMemory.h` for `FMemory::Memcpy`, `UObject/UnrealType.h` for `FFloatProperty`/`FindFProperty`, `UObject/UObjectGlobals.h` for `GetDefault`, `Components/WidgetComponent.h` for the CDO's accessors).
✅ `GetDrawSize() const` (`WidgetComponent.h:247`) and `GetPivot() const` (`:309`) are both `const` ⇒ callable on the `const*` from `GetDefault<>` — **checked, because the whole of test 5 rests on it.**
✅ `FCastSample{bool,float}` / `FCastEvent{float,bool}` are aggregates (no user constructors, no bases, no private members) ⇒ aggregate-init with default member initialisers is legal.
✅ All 24 `FString::Printf` sites: format specifier count matches argument count, `%f` receives floats (variadic-promoted), `%d` receives `int32`.
✅ `AddError(*FString::Printf(...))` implicitly constructs an `FString` — legal.
✅ Every array `.Last()` is behind a `Num()` guard. No unguarded dereference, no timer, no delegate, no new `UObject` pointer ⇒ **no GC exposure** (`bCastRowDriven`, `CastPollAccumulator`, `CastBarBaseDrawSize`, `CastBarBasePivot` are PODs holding no references — correctly **not** `UPROPERTY`, matching the occlusion trio's precedent).
✅ Reflection is correct where it is needed: both cast tunables are `UPROPERTY(EditDefaultsOnly, Category=…)` floats, which is what makes tests 5(c) and 9(a) able to read them without widening the shipped surface.
✅ Header/cpp consistency: 8 declarations, 8 definitions, 0 orphans, all three files end on a complete function.

⚠️ **And the engine question I would not sign this off without:** *can the cast poll ever stop running while the row is open?* — because that is the stranded-bar failure. `WidgetComponent.cpp:1264` auto-disables the component tick only when `TickMode != ETickMode::Enabled`, and `:1275` only when `TickMode == Disabled`. **The constructor sets `ETickMode::Enabled` (`:76`)** ⇒ **neither can fire.** ✅ The poll cannot be switched off by the engine, and `TASK-861` setting `CastBarRoot` to `Collapsed` touches a **child of the inner widget**, never the component's own visibility, so it cannot trip `:1264` either.

---

## ⚠️ DECLARED RESIDUALS — ⛔ ALL SEVEN OF THE PROGRAMMER'S ENDORSED, NONE PROMOTED

1. ⛔⛔ **NOTHING HERE IS EVIDENCE THE BAR APPEARS, AND THAT IS ⛔ NOT FILED AS A DEFECT.** `CastBarRoot`/`CastBarFill` do not exist; `OnCastProgressChanged` is a BIE with no implementation, which in UE is a **silent no-op**. ⇒ **this half is INERT AND HARMLESS BY CONSTRUCTION and is SAFE TO COMPILE AND SHIP AHEAD OF THE WIDGET.** Whether 4.00 px of amber reads at gameplay distance, whether centre-out reads as different in motion, and whether the two-ended pair reads as ONE event are 🧑 **Jonathan's eye** — `TASK-131` is this project's record of structural readback **passing on a visually broken widget**.
2. ⚠️ A completed cast peaks at **98.33 %**, not 100 % — a **1.47 px** shortfall on an 88 px fill. Test 9 proves the 0.15 s alternative costs **4.40 px**, which *is* visible and *is* confusable with an interrupt at 95 %. ✅ The own-period decision is correct and is **arithmetically justified**, not preferred.
3. ⚠️ The **≤1-poll residual** (an interrupt inside the final 50 ms of a 3 s cast = 1.7 % of the window) — endorsed as declared, disambiguated by the other half of the signal.
4. ✅ **`ShouldPollCastProgress` FORWARDS rather than restating — and NOT renaming the shared gate was CORRECT.** The name is bound by `SiegeHealthBarOcclusionTest` at three sites, which is not this task's file. **Flagging it rather than doing it quietly is the behaviour to keep.**
5. ✅ The self-gate keying on `DrawSize` alone — endorsed, and **weaker-consequence than declared**: `SetDrawSize` is internally idempotent, so the gate is a cost guarantee rather than a correctness one.
6. ✅ The latch written **before** the null-widget early-out (`.cpp:622` vs `:631`) — endorsed and verified; the alternative makes a bar created mid-cast miss its own falling edge.
7. ✅ A degenerate `CastBarRowHeightPixels == 0` — confirmed reachable from a BP and harmless (geometry never changes; the event still fires).

---

## Notes for build-master

1. ⛔ **`TL-§5c`: `425 declared / 31 files` is a DECLARATION, not a pass count.** ⛔ **This gate had no shell and executed NOTHING.** The build-master owes the **first execution** of these 15 rows. If any row is red on the runner, ⛔ **check WARN-1's coverage note and WARN-2/3's needle notes first** — those are the three places where a red would be an *instrument* fault rather than a code fault, and the tempting "fix" for each is to loosen a guard.
2. ⚠️ **Two red rows are EXPECTED and are NOT this diff's:** `SiegeAcquisitionFunnelTest` tests 1 and 9 (`TASK-868` / `TASK-882`). ⛔ Do not attribute them to `TASK-860` and do not let them block this commit.
3. ⭐ **The compile is the only thing that can still surprise us, and the one place to look is test 5**: `GetDrawSize()`/`GetPivot()` return `FVector2D` = `TVector2<double>` under LWC. All four component reads are explicitly `static_cast<float>` (`SiegeCastBarTest.cpp:669-672`) and I measured **zero** remaining un-cast uses — but if `TestEqual` reports an **ambiguous overload**, that is the family and that is the fix.
4. ⛔ **Nothing here is playtest evidence.** The commit message should say **"the cast bar's DATA path"**, never "the cast bar" — `TASK-861` phase B is still the irreducible ~90-second 🧑 Jonathan widget-tree step, and until it lands **not one amber pixel exists.**
5. ⚠️ **`WARN-4` (`RequestRedraw` deprecation) is boarded, NOT fixed, and must NOT be fixed in this commit** — the other two call sites are outside `TASK-860`'s fence and a partial sweep is worse than none.
6. ✅ **For the record, so the commit does not have to re-derive it:** `SetVisibility(` still reads **1** · `bBarShownByOwner =` **2** · `LineTraceSingleByChannel` **1** · `LineTraceTestByChannel` **0** · `bFindInitialOverlaps` **0** · `ASummonedUnit`/`IncomingWitchCaster`/`WitchCastTarget`/`CastBarRoot`/`CastBarFill` **0** each, all on code lines, all in `CombatantHealthBarComponent.cpp`. **No adjacent pin moved.**
7. ⚖️ **For the manager:** the `SC-§41` cl. 7 recommendation above is a **ruling, not a request** — it needs a manager edit to `CONVENTIONS.md` and should **not** be smuggled into this commit.
