# TASK-860 — THE TWO-ENDED CAST BAR, THE C++ HALF — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-862` · **Law:** `WITCH-§9` (`§9.1`, `§9.3`, `§9.6`) · `HIGH-§1` · `SHIP-§9` · `SC-§40` cl. 9
**Sitting:** 2026-09-03, **RESUMED** after the previous agent was killed mid-task by a network outage.

---

## 0. ⛔⛔ READ THIS FIRST — WHAT WAS ALREADY ON DISK, AND HOW I ESTABLISHED IT

The dispatch told me the source was *probably* complete and told me **not to trust that**. I did not.

**Measured before writing a line** (`SC-§40` cl. 3 — verify the premise, don't inherit it):

| check | result |
|---|---|
| brace balance, `CombatantHealthBarComponent.{h,cpp}` + `CombatantHealthBarWidget.h` | 53/53 · 2/2 · 1/1 — **no truncated function** |
| every declared cast seam has exactly one definition | ✅ 5 statics + 3 members, `h=1 / cpp=2` each (`PushCastProgress` cpp=3: decl-site + 2 callers) |
| file tails | all three end on a complete function |
| **item (0a) premise** | ✅ **GREEN** — see §1 |
| **every live count-pin adjacent to the diff** | ✅ **all hold** — see §6 |

⇒ **The source half was COMPLETE and CORRECT. Only the test file was missing.** The killed agent's last words
(*"All pins hold. Now the test file"*) were accurate, and I re-derived both halves of that claim rather than
taking them. **I changed ⛔ ZERO lines of shipping source this sitting.** The whole of my diff is one new file.

---

## 1. ITEM (0a) — THE PREMISE GATE, VERIFIED AT SOURCE

⭐ **GREEN. The STOP guard did NOT need to fire.** `TASK-849` adjudicated `TASK-830`'s missing item (8) and it
landed. Read directly in `HealthBarProvider.h:141` / `:158`:

```cpp
virtual float GetCastProgressPercent() const { return 0.f; }
virtual bool IsCastInProgress() const { return false; }
```

Both **DEFAULTED**, neither pure ⇒ `ABuilding`, `AHeroCharacter` and every non-witch unit need zero changes
(`WITCH-§9.6`). ⛔ I did not touch that file — it is `TASK-830`'s.

I also read the **implementation** rather than the handoff's description of it (`ASummonedUnit.cpp:3293-3388`),
because the tests had to be honest about what the surface actually does:
- `ResolveCastClockOwner()` answers for **both ends** — the witch from `IsVeilCaster() && IsCastingVeil()`, the
  subject by **pulling** through its weak `IncomingWitchCaster`, with a **back-pointer agreement re-check**
  (`Caster->WitchCastTarget.Get() != this` ⇒ null) so a subject re-targeted by a second witch does not read a
  clock the first witch no longer owns.
- The percent is `100.f * Elapsed / Rate` off the **live timer's own `GetTimerRate`** — ⛔ never the
  `WitchCastSeconds` tunable. **That is why `SiegeInvisibilityTest` test 28's equality pin survives my work:
  I add no caller inside `SummonedUnit.cpp` at all.**

---

## 2. FILES TOUCHED

| file | this sitting |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCastBarTest.cpp` | ⭐ **NEW — the only file I wrote.** 1 606 lines, **15 tests** |
| `CombatantHealthBarComponent.{h,cpp}` | ⛔ **UNCHANGED this sitting** (complete on arrival; audited, not edited) |
| `CombatantHealthBarWidget.h` | ⛔ **UNCHANGED this sitting** (complete on arrival) |
| `HealthBarProvider.h` · `SummonedUnit.{h,cpp}` · any UMG asset | ⛔ **NOT TOUCHED — outside the fence** |

⛔ No compile, no editor, no MCP, no Git — the editor is up (PID 22940) and the art task is resuming on it.

---

## 3. ⛔ WHAT IS AND IS NOT VISIBLE UNTIL `TASK-861` PHASE B LANDS — SAID PLAINLY

**Nothing is visible. Not one amber pixel.** And that is the correct state, not a defect:

- `CastBarRoot` / `CastBarFill` **do not exist yet**. `TASK-861` is blocked on an irreducible **~90-second
  🧑 Jonathan step** — MCP cannot instantiate a widget into a `WidgetTree` (re-measured live by `861`: no UMG
  toolset, `WidgetTree` `list_properties` returns the empty string, `Slots` absent from `BarStack`).
- `OnCastProgressChanged` is a `BlueprintImplementableEvent` with **no implementation**, which in UE is a
  **silent no-op**. ⇒ my half calls into nothing and **cannot** change a single pixel on any actor.

⭐ **INERT AND HARMLESS BY CONSTRUCTION, and it is stronger than "probably fine":**
- `ShouldPushCastRow(false, false) == false` ⇒ every building, the hero and every non-witch unit make
  **zero Blueprint calls for their entire life**. Their per-frame cost is one float add + one compare.
- `ApplyCastRowGeometry` self-gates on the size already being right ⇒ on a non-casting actor it **never writes
  `DrawSize` or `Pivot` at all**, so the geometry is bit-identical to the pre-`TASK-860` component.
- The only actors that reach any new code are a witch mid-cast and her subject — and today they call an
  unimplemented BIE.

⇒ **This is safe to compile and ship ahead of the widget.** When phase B + C land, the data is already flowing.

---

## 4. THE SIX SPEC ITEMS — HOW EACH LANDED

| item | where | note |
|---|---|---|
| **(1)** extend, never duplicate | the existing `UCombatantHealthBarComponent` / `UCombatantHealthBarWidget` / `WBP_CombatantHealthBar` | ⛔ **no new widget class, no second `UWidgetComponent`, no new WBP** |
| **(2)** one atomic BIE | `CombatantHealthBarWidget.h:136` | `void OnCastProgressChanged(float CastPercent, bool bCasting);` — **character-for-character** as `WITCH-§9.3` pins it; float/bool only; range in the comment (`HIGH-§1`) |
| **(3)** driven by the **owner's own** provider | `UpdateCastProgress()` reads `Cast<IHealthBarProvider>(GetOwner())` | the component names **`ASummonedUnit` 0 times**, `IncomingWitchCaster` 0, `WitchCastTarget` 0 — asserted (test 11) |
| **(4)/(4a)** DrawSize grows **at cast time** | `ApplyCastRowGeometry` | ⛔ **not the constructor.** See §5 |
| **(4b)** the `Pivot` rider | `ComputeCastPivotY` | **decided explicitly**, see §5 |
| **(5)** pixel-identical when not casting | the seed + the self-gate | see §3 and test 5/6 |
| **(6)** tests | `SiegeCastBarTest.cpp`, 15 tests | see §7 |

---

## 5. ⭐⭐ THE PIXEL BUDGET AND THE PIVOT — THE TWO NUMBERS QA SHOULD CHECK MY ARITHMETIC ON

### 5a. The budget (item 4) — before/after, per segment

`TASK-861` measured the live slots; `CastBarRoot`'s slot **mirrors `BoostOutline`'s** (`Fill 1.0`, pad-bottom
`1`, index 0):

| state | `DrawSize` | layout | `CastBarRoot` | `BoostOutline` | `Bar` |
|---|---|---|---|---|---|
| **not casting** | **(90, 22)** | root **Collapsed** ⇒ slot **SKIPPED** ⇒ (22 − 1) split Fill **1:2** | — | **7.00** | **14.00** |
| **casting** | **(90, 30)** | (30 − 1 − 1) split Fill **1:1:2** | **7.00** | **7.00** | **14.00** |

⭐ **`BoostOutline` and `Bar` keep their EXACT current heights in BOTH states** — the health bar does not shrink
to make room. That is **stronger than item (4) asked for**.
`CastBarFill` = 7.00 − (2 × 1.5 padding) = **4.00 px**, identical to `BoostBar`'s 4.00 px.

### 5b. ⛔ WHY IT IS NOT THE CONSTRUCTOR — item (4a), and I built to the amendment

`BoostOutline` and `Bar` are **`Fill` slots** and absorb every spare pixel ⇒ a constructor bump to `(90,30)`
renders `Bar` at **19.33 px instead of 14.00** on **every building, the hero and all 20+ units**, casting or
not. `SetDrawSize`/`SetPivot` are therefore written **only in `ApplyCastRowGeometry`**, which is **idempotent
and self-gating**: it early-outs when the size is already right, so the pair lands on the cast's **two edges**
and on none of the ~59 polls between them.

⭐ **Test 5 catches a regression here by CALLING THE CDO** (`GetDefault<>()->GetDrawSize()` / `GetPivot()`), not
by reading source text — a runtime read of the shipped default, which goes red the instant anyone "simplifies"
the geometry back into the constructor.

### 5c. ⚠️ THE PIVOT — DECIDED EXPLICITLY, AND I CHOSE **NEITHER** OF THE TWO OBVIOUS OPTIONS

`TASK-861`'s rider is real: the component **never sets `Pivot`**, so it is the engine default `(0.5, 0.5)` and a
+8 px grow about a centred pivot goes **4 px up AND 4 px down — into the unit's head**.

- ⛔ **Refused: `Pivot=(0.5,1.0)` in the constructor.** It is the obvious fix and it is **worse** — it would move
  **every bar in the game up 11 px, permanently**, which is exactly the fleet-wide regression `WITCH-§9.6`
  forbids. (Test 4d records the size of what was refused.)
- ⛔ **Refused: compensating `BarHeightZ`.** It is an `EditDefaultsOnly` a BP may legitimately override, and it
  is in **world units**, not pixels — the correction would be camera-distance dependent and therefore wrong at
  every distance but one.
- ✅ **Chosen: recompute the pivot WITH the size, at cast time**, to hold the **bottom edge exactly still**:

```
ComputeCastPivotY(Base, BasePivotY, Grown) = 1 − ((1 − BasePivotY) × Base) / Grown
     22, 0.5, 30  ⇒  1 − 11/30  =  0.633333
```

**Mechanism:** in the screen-space path the engine feeds `Pivot` to the canvas slot as its **ALIGNMENT**
(`SWorldWidgetScreenLayer.cpp` — `CanvasSlot->SetAlignment(ComponentPivot)`, re-read every frame alongside
`GetDrawSize`), and an alignment `A` offsets a box of height `H` by `−A·H` ⇒ the bottom edge sits `(1−A)·H`
below the anchor. Holding that product constant is the whole trick.

⇒ **bottom offset 11.00 px in both states; the top edge rises by exactly 8.00 px.** The growth is **entirely
upward, into the empty sky above the unit** — the only direction with room. Asserted in test 4(a)/(b).

⚠️ `ComputeCastBarHeightPixels` **rounds to a whole pixel** rather than leaving it to the engine, and that is
load-bearing: `SetDrawSize` **truncates** into an `FIntPoint`, so an un-rounded 30.4 would be laid out as 30
while the pivot compensated for 30.4 — a half-pixel disagreement between the two halves of **one** geometry
change, i.e. a bar that creeps.

---

## 6. ⛔ THE COUNT-PINS — RE-MEASURED BY SYMBOL AND REPORTED **EVEN WHERE THEY MATCH** (`SC-§40` cl. 9)

*A silent match is indistinguishable from a skipped check.* Instrument: a faithful replica of the house
`CountOccurrencesInCode` (skip a trimmed line starting `//`, `*`, `/*`). **Proven in both directions before any
number below was trusted** (`SC-§39`): `SetVisibility(` in the component reads **3 raw / 1 skip-aware**, and the
2 eaten hits are the shipped comments that explain why it is forbidden.

| pin | owner | value | verdict |
|---|---|---|---|
| **`SetVisibility(` in `CombatantHealthBarComponent.cpp` == 1** | `SiegeHealthBarOcclusionTest` test 11(c) | **1** (raw 3) | ✅ **UNTOUCHED — the trap avoided** |
| `bBarShownByOwner =` == 2 | same, test 11(a) | **2** | ✅ |
| `LineTraceSingleByChannel` == 1 · `LineTraceTestByChannel` == 0 · `bFindInitialOverlaps` == 0 | same, test 12 | **1 · 0 · 0** | ✅ |
| `bStartPenetrating` > 0 · `ComputeOcclusionFromTraceResult` > 0 · `UpdateHealthBarOcclusion` > 0 | same, test 12 | **1 · 2 · 2** | ✅ |
| `virtual float GetCastProgressPercent() const override;` tree-wide == 1 | `SiegeInvisibilityTest` test 31 | **1** | ✅ my component **calls**, never overrides |
| `IsCastInProgress` tree-wide control | same, test 32 | **`> 0`, not an absolute** | ✅ **`TASK-830` §16 wrote it that way ON PURPOSE, anticipating my consumption** |
| `GetCastProgressPercent`+`IsCastInProgress` in `Building.h`/`HeroCharacter.h` == 0 | same, test 31 | **0** | ✅ not my files |
| `WitchCastSeconds` in `SummonedUnit.cpp` / in `BeginWitchCast` — **equality** | same, test 28 | **2 / 2** | ✅ **I add no caller in that file** |
| `return false;` in `FSiegeCombatStatics::ReadFogState` == 2 | `SiegeFogClampTest` | untouched | ✅ not my files |
| `FSiegeFogStatics::EffectiveVisionRadius(` tree-wide == 2 | same | untouched | ✅ not my files |

⭐ **AND THE ONE THAT MATTERS FOR MY NEW FILE:** `CountAcrossShippingSource` (`SiegeFogClampTest:198`) skips any
path containing `/Tests/` (`IsAutomationTestFile`). ⇒ **`SiegeCastBarTest.cpp` is invisible to every tree-wide
sweep** and cannot perturb a shipping-source count. Verified by reading the helper, not assumed.

### 6a. ⭐ THE STORED-TELL BAN — I DID **NOT** WIDEN IT, AND I FOUND WHY THE OBVIOUS NEEDLE WOULD HAVE BEEN RED

`TASK-849` ruled the `bIsCasting`/`bCastInProgress` ban is correctly **scoped to `SummonedUnit.h`**. I honoured
that and wrote the mirror rule for **my own header only** (test 13).

⚠️ **AND THE OBVIOUS NEEDLE IS A TRAP I MEASURED BEFORE USING IT:** banning the bare word **`CastPercent`** in
`CombatantHealthBarComponent.h` would have been **RED ON ARRIVAL** — it occurs **3× on code lines** as a
**substring of the shipped parameter names** `ProviderCastPercent` and `RawCastPercent`. ⇒ the ban is written as
a **member shape** instead: `Percent;` == 0 and `Percent =` == 0 (a member declaration ends in `;` and a member
initialiser contains ` = `; a parameter does neither), with a positive control that the same needle shape
matches real members here (`Accumulator = 0.f;` == 2). **This is `SC-§39` in its substring form rather than its
comment form, and I have not seen it recorded before.**

---

## 7. THE TEST FILE — 15 TESTS, AND WHAT EACH FAILS ON

| # | test | goes red when |
|---|---|---|
| 1 | `ThePushGateFiresOnTheFallingEdge…` | the gate becomes `if (bCasting)` · either argument stops being read |
| 2 | `NotCastingIsExactlyZero…ClampedToTheShipped0To100Scale` | a stale percent survives the gate · the clamp or its ceiling goes · **the `IsFinite` branch is deleted** (a NaN passes a clamp — every comparison against NaN is false) · the scale is rescaled to 0..1 |
| 3 | `TheGrownHeightIsWholePixels…` | the whole-pixel round goes (pivot/size disagree ⇒ a creeping bar) · a negative row height **shrinks** the bar |
| 4 | `TheGrownBarHoldsItsBottomEdgeStill…` | the health row moves when a cast starts · the growth stops being entirely upward · the pivot stops changing at all |
| **5** | ⭐⭐ `TheConstructorDefaultDrawSizeAndPivotAreUntouched…` | **the fleet-wide regression: a constructor `DrawSize` bump or a constructor `Pivot`.** A **runtime CDO read**, not a source probe |
| 6 | `TheGeometryIsDerivedFromTheCapturedBase…` | the grown height is read off the **live** size (bar creeps 8 px per cast) · the restore targets the C++ constant instead of the captured base (deletes a BP override) · the self-gate goes · a cast symbol appears in the constructor |
| **7** | ⭐⭐ `ABrokenCastAndACompletedCastProduceDifferentEventSequences` | **the deliverable.** The two endings become indistinguishable · the row is not brought down exactly once · a terminal event carries a non-zero fill · the fill goes non-monotonic |
| **8** | ⭐⭐ `DroppingTheFallingEdgeStrandsTheRowOpenForever…` | **`SHIP-§9`** — replays the *same two casts* through the tempting `if (bCasting)` gate and asserts the suite would catch it |
| 9 | `TheCastRowHasItsOwnFasterPeriod…` | the period is retuned to the cull's 0.15 s (completed peaks at 95.0% ⇒ **4.4 px short — confusable with an interrupt at 95%**) |
| 10 | `ThePollGateFires…FloorsAtThirtyHertz…NeverBursts` | the poll never fires · a BP lowers it below 30 Hz · catch-up returns (a burst on ~19 consecutive frames after a hitch) |
| 11 | `TheComponentReadsItsOwnOwnersProvider…` | the bar layer learns what a witch is (⇒ someone is **pushing**) · a second clock appears in the component |
| 12 | `ExactlyOnePlaceDecidesWhatTheWidgetIsTold…` | a second push site · geometry stops preceding the value · **the C++ learns the name `CastBarRoot`/`CastBarFill`** · a `SetVisibility` appears on any cast path |
| 13 | `TheComponentRemembersWhetherTheRowIsOpenAndNeverWhatItSaid` | a cached percent member appears · the row latch is renamed or duplicated |
| 14 | `TheCastPollIsNotGatedByTheOcclusionCullsThreeEarlyOuts` | the poll slides under `bOccludeHealthBarWhenBlocked` (a stonework flag kills the tell) · under `!bBarShownByOwner` (**a unit that dies mid-cast respawns still painting it**) · under the cull's own gate · above `Super::TickComponent` |
| 15 | `TheOneEventIsDeclaredExactlyAsTheLawPinsIt…` | **the pinned signature drifts by one character** ⇒ `TASK-861`'s override binds nothing and the tell vanishes with a green build · an enum/colour parameter appears |

⭐⭐ **Tests 7 and 8 are the pair `SHIP-§9` asks for.** Test 7 proves the shipped composition tells BROKEN from
COMPLETED. Test 8 proves that claim is **discriminating**, by replaying the identical two casts through the
wrong gate and asserting the terminal event disappears. A replay harness (`MakeCastRun` / `ReplayCastRun`)
composes the **shipped statics** exactly as `UpdateCastProgress` → `PushCastProgress` compose them.

**The measured numbers** (3 s cast, 0.05 s poll):

| | events | peak | row-down events | last event |
|---|---|---|---|---|
| **COMPLETED** | 60 | **98.333 %** | 1 | `(0.0, false)` |
| **BROKEN @ 1.25 s** | 25 | **40.000 %** | 1 | `(0.0, false)` |
| COMPLETED, **buggy gate** | 59 | 98.333 % | **0** | `(98.33, true)` ⛔ frozen forever |
| BROKEN, **buggy gate** | 24 | 40.000 % | **0** | `(40.00, true)` ⛔ frozen forever |

⇒ a **58.3-point gap** separates the two endings, and the wrong gate loses the terminal event in both.

---

## 8. ⚠️ DECLARED RESIDUALS — said out loud rather than left for QA to find

1. ⛔⛔ **NOTHING HERE IS EVIDENCE THE BAR APPEARS.** See §3. Whether **4.00 px of amber** reads at gameplay
   distance, whether centre-out reads as different in motion, and whether the two-ended pair reads as **one
   event** are 🧑 **Jonathan's eye** — `TASK-861` §9 asked those and correctly refused to answer them.
   `TASK-131` is this project's record of structural readback **passing on a visually broken widget**.
2. ⚠️ **A COMPLETED CAST PEAKS AT 98.33 %, NOT 100 %,** because the timer's callback clears the handle — the last
   observable value is one poll before the end. That is a **1.47 px shortfall on an 88 px fill** and is invisible.
   At the cull's 0.15 s it would be **4.4 px** and *visible*, which is why the cast row has its own period.
3. ⚠️ **THE ≤1-POLL RESIDUAL.** An interrupt inside the final 50 ms of a 3 s cast (1.7 % of the window) still
   paints as "nearly full". Disambiguated by the other half of the signal — a completed cast also turns the
   target translucent (`MI_Unit_Invisible`); a broken one changes nothing.
4. ⚠️ **`ShouldPollCastProgress` FORWARDS to `ShouldPollOcclusion` rather than restating it.** The gate is generic
   by construction and its 30 Hz floor is a guarantee a BP cannot lower; two copies of a guarantee drift.
   ⚠️ **Renaming the shared gate to suit both callers would be tidier and is deliberately NOT done** — the name is
   bound by `SiegeHealthBarOcclusionTest`, which is not this task's file. **Flagged rather than done quietly.**
5. ⚠️ **`ApplyCastRowGeometry`'s self-gate keys on `DrawSize` alone.** If a BP changed `Pivot` at runtime *while*
   the size was already correct, the pivot would not be restored until the next real edge. Judged correct: the
   alternative is writing both every poll, and no shipped path mutates `Pivot` at runtime.
6. ⚠️ **`PushCastProgress` writes the latch BEFORE the null-widget early-out.** Deliberate — if a widget appears
   later, the next real edge still pushes. The alternative (latch after) would make a bar created mid-cast miss
   its own falling edge.
7. ⚠️ **A degenerate `CastBarRowHeightPixels == 0`** makes casting and not-casting geometry identical, so the
   geometry never changes. Harmless (the event still fires); recorded because it is reachable from a BP.

---

## 9. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **§5c — the pivot ruling.** Re-derive `1 − ((1−0.5)×22)/30 = 0.6333` yourself and confirm the canvas-alignment
   mechanism. This is the row where a mistake is **a health bar sitting in the unit's head for 3 seconds**, and
   the two alternatives I refused are both more obvious than the one I chose.
2. **§6a — the substring trap.** Confirm you agree that banning `CastPercent` in my header would have been red on
   arrival, and that the member-shape needle catches the real defect. If you think the ban should be the bare
   word, say so **now** — after this lands it is a red suite and a wrong "fix".
3. **Test 8's modelled bug.** Confirm the `bUseShippedGate == false` branch reads as a **model of a wrong
   implementation** and not as dead production surface. It exists only inside the test fixture.
4. **Item (0b) compliance.** Confirm I added **zero** `SetVisibility(` calls and that the collapse is left
   entirely to `TASK-861`. The C++ names neither `CastBarRoot` nor `CastBarFill` anywhere (measured 0/0).
5. **Compile risks I cannot test** (⛔ no compile in my lane):
   - ⭐ **`FVector2D` is `TVector2<double>` under LWC**, so `GetDrawSize().Y` is a **double**. Handing that to
     `TestEqual(const TCHAR*, float, float, float)` beside float literals makes the float and double overloads
     **AMBIGUOUS — a hard compile error, not a warning.** ⇒ **I found this in my own first draft and fixed it**:
     test 5 narrows through explicit `static_cast<float>` locals. **Please re-check I got them all** (measured:
     0 remaining un-cast uses).
   - The **shipped** `ComputeCastBarHeightPixels(CastBarBaseDrawSize.Y, …)` passes a double to a float parameter —
     an implicit narrowing, legal, and **proven in this project**: `BattlefieldScatter.cpp:670/678` assigns a
     `double` `Size()` result to a `float FootprintR` and compiles today. ⇒ **not a finding**, declared so nobody
     re-raises it.
   - `FMath::RoundToFloat` is `FloorToFloat(F + 0.5f)` (read at `GenericPlatformMath.h:332`) — test 3's 30.4→30
     and 30.6→31 hold under either that or `roundf`; I deliberately avoided the 30.5 half-way case.
   - `GetDrawSize() const` / `GetPivot() const` are both **const** (`WidgetComponent.h:247`/`:309`), which is what
     lets test 5 call them on the CDO.
   - `FCastSample{ bool, float }` / `FCastEvent{ float, bool }` are **aggregate-initialised** — legal in C++20 for
     a class with default member initialisers.
   - My fixture namespace is `SiegeCastBarTestFixture`; its `CountOccurrencesInCode` / `LoadProjectSource` /
     `ExtractFunctionBody` are replicas inside **my own namespace** ⇒ no ODR clash with the occlusion test's.
6. ⚠️ **A defect I found in my OWN test and fixed** (declared because it is instructive): my first draft asserted
   *"the three frames after a 1 s hitch fire zero times"*. **Three frames at 1/60 s is exactly 0.0500000 s**,
   which **reaches** the 0.05 s period and fires legitimately ⇒ that row would have been **red on arrival, for
   the crime of the gate working**. Corrected to **two** frames plus a 20-frame burst ceiling, and both were
   re-verified against a modelled catch-up implementation to confirm they still discriminate (shipped: 0 and 7;
   catch-up: 2 and 20).

---

## 10. SUITE DELTA (`TL-§5c` — a **DECLARED** delta, ⛔ never a pass count I did not execute)

- **My delta: `+15` tests in one NEW file, `Tests/SiegeCastBarTest.cpp`.**
- **Tree census**, `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Siegebound/Tests/*.cpp`:
  **410 declared across 30 files** at my start → **425 declared across 31 files** at my end.
- ⭐ **All `+15` are mine**; no other file changed count during this sitting.
- ⛔ **DECLARED, ⛔ NOT EXECUTED** — no compile, no editor, no suite run in my lane.
- ⚠️ **The tree already carried TWO known red rows before I started** (`SiegeAcquisitionFunnelTest` tests 1 and 9,
  owned by `TASK-868` and `TASK-882`). ⛔ **Neither is mine and I did not add a third** — every pin adjacent to my
  work is re-measured unchanged in §6.

---

## 11. FOR `TASK-861` (phase C) AND `TASK-862` (the gate)

- ✅ **The C++ half is complete and the event now EXISTS to bind.** `TASK-861`'s blocker (2) is cleared: after
  this compiles, `list_events` will show `OnCastProgressChanged` as a bindable parent event.
- ⛔ **Blocker (1) is untouched and is still 🧑 Jonathan's ~90-second widget-tree step.** Nothing I can do reaches it.
- 📌 **The signature is pinned and test 15 asserts it character-for-character:**
  `void OnCastProgressChanged(float CastPercent, bool bCasting);` — ⛔ **`CastPercent` is `0..100`; the widget
  divides by 100**, exactly as the HP row divides `CurrentHP` by `MaxHP`.
- ⛔ **The collapse is yours.** `bCasting == false` ⇒ `CastBarRoot` **Collapsed**, never merely hidden and never
  merely `RenderOpacity 0`. The C++ names neither element and never will.
- ⚠️ **Author it as a TRUE override (`bOverrideFunction = true`).** A `K2Node_CustomEvent` of the same name is
  DSL-indistinguishable, reports `bIsImplemented = true`, and **never fires from C++** — the defect that hid the
  health-bar bug five times (`TASK-131`). **Assert the node's object CLASS, never `bIsImplemented`.**
- ⭐ **Your `Fill 1.5` fallback still works with zero code change:** if 4.00 px of amber does not read, re-slot
  `CastBarRoot` and set `CastBarRowHeightPixels` to 10 — the pivot arithmetic follows it automatically, because
  the grown height and the pivot are both derived from that one number.
