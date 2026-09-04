# TASK-815 — [ST-3] THE PLACEMENT WHEEL. The THIRD and FINAL `MARK-§4` consumer.

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Status → `ready-for-qa`** · gate **TASK-816**
**Law:** `STACK-§4` (the wheel is consumer 3; the collision is refuted at source) · `STACK-§6` (scaled bounds, ⛔ never local) · `MARK-§4` **as amended** · `HIGH-§1` (every tunable carries its consequence) · `HELP-§2` mechanism 3 (§7 below is `TASK-823`'s citation table) · `SC-§33` · `SC-§37`
**Compile / editor / MCP / Git:** ⛔ none run. `TASK-824` owns the wave's one compile.

---

## 0. FILES TOUCHED — three, exactly the fence

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | +3 public pure statics · +3 `EditDefaultsOnly` tunables · +1 private method · +2 private members |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | 1 line in `PlayerTick`'s placement branch · 1 block in `EnterPlacementMode` · 2 lines in `ExitPlacementMode` · 1 block at the head of `UpdatePlacementGhost` · 1 shipped line widened in `TryConfirmPlacement` · `ApplyPlacementFootprintWheel()` · the 3 statics |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` | **+5 tests (24–28)** in a new `SiegePlacementWheelFixture` namespace · +1 include |

⛔ **Not touched:** `SiegeControlsHelpWidget.{h,cpp}` (**`TASK-821` is in it right now; `TASK-823` owns my rows**) · `Building.{h,cpp}` / `ClimbableTower.*` (**read only** — I call `CanScaleFootprint()`, I author nothing) · any `TOWER-§8.*` symbol (⛔ no socket, ⛔ no climb line, ⛔ no rung plane, ⛔ no standoff, ⛔ no `A_SiegeBiped_Climb`, ⛔ no `SM_WatchTower`) · `HIGH-§` · `cards.csv` · `M_Ghost` · every WBP · `CardHandWidget.*`. Airlock: ⛔ no `Capture()`/`EnsureSnapshot()`, Zone A untouched, 🔒 the 552 latch unspent, ⛔ no token figure.

### ⛔ `TASK-819`'s BLOCK IS BYTE-IDENTICAL — the one thing my dispatch asked me to guarantee

`IA_DiscardAll` · `OnDiscardAllPressed()` · `DiscardEntireHand()` · `DiscardAllCost` are **untouched, unmoved and unrenumbered**. `DiscardAllCost = 20;` still sits where `819` left it (`SiegePlayerController.h:1496`), so **`TASK-844`'s one-line substitution to `SiegeCardEconomy::DiscardAllGold` lands on exactly the line `CARDBAR-§14` expects.** My header additions are all *above* it (the public statics, ~`:901`–`:952`) or *far below* it (tunables ~`:2087`, method `:2244`, members `:3006`), so `844`'s anchor text is unchanged.

---

## 1. ⭐⭐ SPEC (5) — THE HARD PREREQUISITE, **VERIFIED AT SOURCE BEFORE I WROTE A LINE**

> ⛔ *"if the shipped footprint reads LOCAL bounds, ⛔ STOP and report."*

✅ **IT READS THE SCALED BOUNDS. I did not have to stop.** `SiegePlayerController.cpp:5348` (inside `TryGetPlacementFootprintRadius`), quoted:

```cpp
const FBoxSphereBounds ScaledBounds = GhostMesh->CalcBounds(GhostMesh->GetComponentTransform());
DerivedRadius = PlacementFootprintRadiusFromBounds(ScaledBounds.BoxExtent);
```

`CalcBounds(GetComponentTransform())` applies the component's **live world transform**, so `GetScale3D()` is already in the answer. ⛔ It is **not** `UStaticMesh::GetBounds()`. `TASK-735` wrote its own comment predicting this exact day (`:5333-5345`) and the board recorded the measurement at `TASKBOARD.md:14081`. ⇒ ⭐ **I set the ghost's scale and the validated footprint follows with ⛔ zero further edits — a ×1.5 building can ⛔ never be validated at ×1.0.** Test 25(c) asserts that property through the **real** `FBoxSphereBounds::TransformBy` path rather than describing it.

---

## 2. THE SHAPE — `ApplyGroupPickWheel` COPIED, ⛔ NOT REINVENTED

```
PlayerTick :785   ApplyPlacementFootprintWheel();      ← the ONLY call site in the codebase
                    ├ inert unless bPendingCardCanScaleFootprint      (STACK-§2 exclusion)
                    ├ polled WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)   → NotchDelta
                    ├ inert on NotchDelta == 0  (no notch, or BOTH fired and cancelled)
                    ├ StepPlacementFootprintScale(...)  → clamped [Min, Max]
                    └ inert if the value did not change (pinned at a clamp)
                         ⇒ writes PlacementFootprintScale and ⛔ NOTHING ELSE
PlayerTick :788   UpdatePlacementGhost();
                    ├ :2605  GhostActor->SetActorScale3D(MakePlacementFootprintScale3D(PlacementFootprintScale))
                    └ :2668  TryGetPlacementFootprintRadius(...)      ← reads the SCALED bounds
TryConfirmPlacement :2362  FTransform(Rot, Loc, MakePlacementFootprintScale3D(PlacementFootprintScale))
```

**Every element of `ApplyGroupPickWheel`'s shape is reproduced:** polled, ⛔ no new `InputAction`, `+=`/`-=` so both directions in a frame cancel, one notch = one step, clamped, an early-out when the value is pinned at a clamp, and the tail that applies the result. ⛔ **No new mechanism was invented.**

### ✅ SPEC (2) — I DID **NOT** ADD A GUARD, AND I DID **NOT** FIND A REASON TO

`PlayerTick`'s four cursor modes are mutually exclusive **structurally**, not by discipline: the group-pick branch `return`s, targeting `return`s, the war map `return`s, and placement is reached only past `if (!bInPlacementMode) { return; }`. ⇒ **consumers 1 and 3 can never poll in the same frame.** ⛔ **No guard added for a state that cannot exist**, and I have ⛔ **no finding** to report against that claim. Test 27(c) turns it into an assertion: the two polls sit on **opposite sides** of the placement gate, measured on code lines.

---

## 3. THE THREE PURE SEAMS (`SC-§33` — ⛔ no world, ⛔ no member state, ⛔ no parameter defaulted)

| seam | `.h` | `.cpp` | what it decides |
|---|---|---|---|
| `static float StepPlacementFootprintScale(float Current, int32 NotchDelta, float Step, float MinScale, float MaxScale)` | `:901` | `:5236` | how far one notch moves, and where it stops |
| `static FVector MakePlacementFootprintScale3D(float FootprintScale)` | `:924` | `:5273` | what a scale factor means as a 3D scale — ⭐ **the ONE expression, called by BOTH consumers** |
| `static bool CanCardActorScaleFootprint(const UClass* CardActorClass)` | `:952` | `:5294` | which cards may be wheeled at all |

They sit beside `TASK-735`'s four and `TASK-813`'s three, in the same block, for the same stated reason.

### ⭐ `MakePlacementFootprintScale3D` is what makes spec (4) STRUCTURAL rather than disciplinary

> ⛔ *"THE GHOST AND THE SPAWNED BUILDING MUST AGREE."*

They agree because they are **the same member run through the same function**. There is deliberately ⛔ **no second place** where an `FVector` is built out of `PlacementFootprintScale` — test 28 measures exactly three occurrences of `MakePlacementFootprintScale3D(` on code lines (its definition and its two consumers) and zero `FVector(PlacementFootprintScale` anywhere. ⚖️ Two call sites building "the same" vector independently is precisely how that agreement rots six months from now.

---

## 4. ⭐⭐ THE FINDING THAT MAKES `J-4` FREE — measured in `TASK-812`'s file, ⛔ not assumed

> `J-4`, his own words: *"keeping the same width and length."*

**`ABuilding` roots `VisualMesh`** (`Building.h:43-51`, `Building.h:262-264`). ⇒ the spawn transform's scale **lands as `VisualMesh`'s relative scale**, which is exactly the vector `ApplyStackUpgrade` then reads:

```
Building.cpp:439-441   FVector Scale = VisualMesh->GetRelativeScale3D();
                       Scale.Z = AuthoredHeightScaleZ * StackHeightMultiplier(StackUpgradeCount);
                       VisualMesh->SetRelativeScale3D(Scale);
```

⇒ **the wheel writes X/Y, the upgrade rewrites Z alone, and neither knows about the other.** A building placed at ×1.4 and upgraded four times ends at `(1.4, 1.4, 5 × authored)`. ⛔ **Zero coordination, ⛔ zero new API, ⛔ zero risk of the two fighting over one component.** Test 28(f) pins it from inside my fence, read-only: `ApplyStackUpgrade` writes `Scale.Z` once and `Scale.X`/`Scale.Y` **zero** times.

📌 One consequence worth stating: `AuthoredHeightScaleZ` is captured lazily from `GetRelativeScale3D().Z`, and my scale vector's **Z is always exactly 1** — so the baseline the height series is defined against is unchanged by the wheel, at any wheel setting.

---

## 5. ⚠️⚠️ TWO ORDERING FACTS, BOTH LOAD-BEARING AND BOTH INVISIBLE TO ANY BEHAVIOURAL TEST

**(1) The wheel is polled BEFORE `UpdatePlacementGhost`, ⛔ not after.** `UpdatePlacementGhost` reads the ghost's scaled bounds for the footprint gates. Polled after it, this frame's click would be validated against **last** frame's size — a green the confirm could refuse, which is the one thing this placement path was built never to do. (It is also the exact order the pick branch uses: `ApplyGroupPickWheel()` then `UpdateGroupPickReticle()`.)

**(2) The ghost is SIZED at the head of `UpdatePlacementGhost` (`:2605`), ⛔ not beside `SetActorLocation` at its foot.** The footprint read at `:2668` is below it. Written the other way round the gates would validate a ×1.5 building against a ×1.0 footprint — silently, in exactly the class of defect `TOWER-§7` exists to close.

⭐ **Both are asserted, because neither can fail a behavioural test:** `CheckPrecedes` rows in tests 27(d) and 28(e).

### ⭐ AND THAT IS WHY THE WHEEL DOES **NOT** TOUCH THE GHOST

My first draft applied `SetActorScale3D` inside the wheel, on the frame a notch landed. **I moved it and the reason is a real defect, not tidiness:** a scale applied only *when a notch lands* is an **event**, so the ghost and the member would silently disagree on the **first frame of every session** the day anyone retunes `PlacementFootprintMin` off 1.0. Applying it unconditionally in `UpdatePlacementGhost` makes the agreement an **invariant**. ⇒ ⭐ **one writer for the number (the wheel), one writer for the transform (`UpdatePlacementGhost`)**; tests 28(d) assert the wheel contains ⛔ zero `SetActorScale3D` and ⛔ zero `GhostActor`.

---

## 6. ⛔⛔ SPEC (6) — THE EXCLUSION IS THE **SAME** PREDICATE, ⛔ NOT A SECOND CHECK

`bPendingCardCanScaleFootprint` is resolved **once per placement session** (`EnterPlacementMode`, `.cpp:1773`):

```cpp
bPendingCardCanScaleFootprint =
    bPendingIsBuilding && CanCardActorScaleFootprint(ResolveCardActorClass(CardID, Row->CardType));
```

- ⛔ **There is no `CardID` string compare on this path and no class-name compare either.** `CanCardActorScaleFootprint` asks the card class's **CDO** for `ABuilding::CanScaleFootprint()` — the virtual `TASK-812` put on the base and `AClimbableTower` overrides `false` (`ClimbableTower.h:503`). ⇒ the WatchTower is excluded from the **wheel** by the same one rule that excludes it from the **upgrade**, and the next climbable building inherits both.
- ⭐ **WHY THE CDO, AND WHY THAT IS SOUND RATHER THAN CONVENIENT:** the wheel runs while the player is still choosing a spot, so there is **no instance to ask**. Both implementations are `const` and read ⛔ no instance state, so the class answers for every instance it will make. ⚠️ **That is the one assumption the seam rests on, so test 26(f) asserts it** — the CDO's answer is compared against a **live** `AClimbableTower`'s, asked through an `ABuilding*` (which doubles as a virtual-dispatch check: a shadowed non-virtual override passes every other row and fails that one).
- ⭐ **A UNIT card is inert for free.** `ASummonedUnit` is not an `ABuilding`, so the same `Cast` that asks the question answers it — ⛔ no separate "is this a building" rule was invented for the wheel (test 26(d)).
- ⛔ **`Cast<ABuilding>(GetDefaultObject())`, ⛔ never `GetDefaultObject<ABuilding>()`** — the templated form `checkSlow`s that the class *is* a `T`, and a unit card's class reaching it must answer **false**, not trip a check.

⚖️ **Inert, ⛔ not refused.** The wheel is a continuous adjustment, not a click: a HUD line here would fire on every notch of an idle scroll. The WatchTower's refusal already has a voice — `TASK-813`'s RED ghost plus *"That building cannot be stacked"* on the click.

---

## 7. ⚠️ SPEC (7) — `HELP-§2` MECHANISM 3: **THE `file:line` FACTS FOR `TASK-823`.** ⛔ I WROTE NO HELP ROW.

> ⛔ I did **not** touch `SiegeControlsHelpWidget.{h,cpp}` — `TASK-821` is editing it in parallel and `TASK-823` owns the tower/wheel rows in a different commit. **Everything `TASK-823` needs, traceable to a line I actually wrote:**

| what the row must say | read it here |
|---|---|
| **The gesture** — mouse wheel up/down, while a **building card's placement ghost is up** (⛔ never on the war map, ⛔ never during a group-order pick) | `SiegePlayerController.cpp:785` (the call, inside `PlayerTick`'s placement branch) · `:2816` (the poll) |
| **Which branch it lives in** — the **PLACEMENT** branch, reached only past `if (!bInPlacementMode) { return; }` at `.cpp:757`; the group-pick poll is at `:683` and `return`s at `:696` | `SiegePlayerController.cpp:667-788` |
| **What it changes** — the building's **width and length (X/Y)**. ⛔ **Never its height** — that is the stack upgrade's axis | `SiegePlayerController.cpp:5273` (`MakePlacementFootprintScale3D`, Z is always 1) |
| **The range** — ⛔ **NAME the tunables, ⛔ never type the numbers** (`HELP-§2`'s no-restated-number rule): the floor is **`PlacementFootprintMin`** and the ceiling is **`PlacementFootprintMax`** | `SiegePlayerController.h:2106` · `SiegePlayerController.h:2122` |
| **⛔ NO SHRINKING** — the floor is the *authored* footprint, and it is a **ruling** (`J-3`), not a rounding | `SiegePlayerController.h:2090-2105` (the tunable's own comment) |
| **One notch = one step**, named **`PlacementFootprintWheelStep`** ⛔ (never typed) | `SiegePlayerController.h:2087` · `.cpp:5236` |
| **⛔ The Watch Tower cannot be resized** — ⭐ **and the row must give the player's reason (*it is the one you climb*), ⛔ never `TOWER-§8.5a`'s machinery** | `SiegePlayerController.cpp:5294` (`CanCardActorScaleFootprint`) · `ClimbableTower.h:503` |
| **Unit and spell cards ignore the wheel entirely** | `SiegePlayerController.cpp:1773` (the `bPendingIsBuilding` short-circuit) |
| **The size you see is the size you get** — the ghost and the spawned building are one value | `.cpp:2605` (ghost) · `.cpp:2362` (spawn) |
| **⭐ THE DISAMBIGUATION `STACK-§4` DEMANDS** — this is wheel meaning **3 of 3**: (1) group-pick **circles**, world uu, `ApplyGroupPickWheel` `.cpp:3583`; (2) map **marks**, widget space, `UWarMapWidget::NativeOnMouseWheel`; (3) **this**, a **scale factor** on a placement ghost. ⛔ All three are the same physical gesture in different modes, so ⭐ **each row must say WHEN it applies** | `CONVENTIONS` `MARK-§4` (amended) + the three sites |

---

## 8. ⚖️ THE JUDGMENT CALLS — ⛔ none blocked, all proceeding defaults

| # | question | ⚖️ ruled | why |
|---|---|---|---|
| **W-1** | the step value | ✅ **`0.1` — five notches from floor to ceiling** | `STACK-§4`: *"small enough that the max is reachable in a few notches."* Five is a few; at `0.05` it is ten notches and several sizes are visually indistinguishable. ⭐ **Measured, not asserted:** the walk lands on `1.5f` **bit-exactly** in 5 notches. 🧑 `EditDefaultsOnly` — one word retunes it |
| **W-2** | inert or refused for a non-scalable card? | ✅ **INERT** | §6 above. A refusal per notch would spam the HUD on an idle scroll, and the exclusion already speaks once at the click |
| **W-3** | accumulate the float, or count notches? | ✅ **ACCUMULATE + CLAMP — the template's shape** | `ApplyGroupPickWheel` accumulates `GroupPickRadius` identically. ⭐ It is also the only shape where **one notch down at the ceiling shrinks immediately**; a notch counter needs its own clamp or the wheel feels dead for as many notches as the player over-scrolled. ⚠️ The price is declared: ~1e-7 drift across many notches — **both endpoints stay exact** because the clamp pins them |
| **W-4** | reset in one place or two? | ✅ **BOTH `EnterPlacementMode` and `ExitPlacementMode`** | `TASK-813`'s handoff item 6 asked for exactly this, and it is now the pattern in both functions |
| **W-5** | seed the scale from a literal `1.0` or from the tunable? | ✅ **from `PlacementFootprintMin`** | `HIGH-§1` — a second copy of the floor would silently survive a retune. ⛔ The member's `= 1.f` initialiser is the **identity for a controller that has never placed anything**, not a copy of the tunable, and its comment says so |

---

## 9. 🔍 WHAT QA SHOULD SCRUTINISE HARDEST

1. **⭐⭐ `EnterPlacementMode` NOW CALLS `ResolveCardActorClass` — and that is a real, declared behaviour change.** It is a `LoadSynchronous`, run **once per placement session** and **only for building cards** (`bPendingIsBuilding` short-circuits it). ⚠️ **The one visible consequence, stated rather than discovered:** for a building card whose BP class is **missing**, `ResolveCardActorClass`'s existing `Warning` now fires at **entry** as well as at confirm — one extra log line, in an already-broken case, and ⛔ no behaviour change (placement still enters, the confirm still refuses and exits). **Rule on it.** The alternatives I rejected: rebuilding the soft-class path myself (a second source of truth for `/Game/Blueprints/Buildings/BP_Building_<CardID>` — automatic-fail territory) or a per-frame CDO resolve (a class load per tick).
2. **⭐ I WIDENED ONE SHIPPED LINE** — `TryConfirmPlacement`'s `const FTransform SpawnTransform(...)` gained a third argument (`.cpp:2360-2362`). It is the ⛔ only shipped statement I changed; ⛔ nothing was reordered, re-indented or deleted anywhere in either file. Spec (4) cannot be satisfied without it.
3. **⚠️ A BP CONSTRUCTION SCRIPT COULD OVERRIDE THE SPAWN SCALE.** `FinishSpawning(SpawnTransform)` applies the transform and *then* runs the construction script; a `BP_Building_*` that set its root's relative scale would win. ⛔ **I did not check the `.uasset`s** — that is outside a code fence and it is the same exposure the shipped *location* has had since `TASK-027`. **Declared as a risk for the PIE eye, ⛔ not as a defect.**
4. **⭐ THE FIVE FALLBACK BRANCHES IN `StepPlacementFootprintScale` ARE UNREACHABLE FROM SHIPPED STATE** (`ClampMin` guards the editor field and the member is only ever written through the clamped stepper). They exist because a hand-edited `.uasset` or a bad merge bypasses `ClampMin`, and because this number multiplies a real collision and navmesh footprint. Test 24(g) covers all five, each paired with the 24(h) control proving the seam can still move.
5. **⛔ THE POLL ITSELF IS NEVER EXERCISED HEADLESSLY** (`SC-§32`). `WasInputKeyJustPressed` needs a live `UPlayerInput`; `SetActorScale3D` needs a spawned ghost; `SpawnActorDeferred` needs a world. ⇒ tests 24–26 assert the **arithmetic and the predicate** the poll calls, and tests 27–28 assert the shipped body really has the shape that calls them. **Neither half is worth anything alone, which is why neither is omitted.** The real instruments are your diff read and Jonathan's playtest.
6. **⚠️ `PlacementFootprintScale` IS ⛔ NOT RE-CLAMPED AT CONFIRM.** It cannot be out of range (one writer, always clamped) and `MakePlacementFootprintScale3D` sanitises non-finite/non-positive input at both consumers. A second clamp would be a second copy of the range. **Rule on it if you disagree.**
7. **⭐ `MARK-§4` IS NOW AT ITS DECLARED CEILING OF THREE CONSUMERS.** Anyone adding a fourth must amend that line again. Test 27(a) makes the third one countable: exactly two occurrences of `ApplyPlacementFootprintWheel()` on code lines — its definition and its one call.

### ⭐ HOW I KNOW THE SOURCE PROBES ARE GREEN WITHOUT COMPILING (the `TASK-819` device)

I re-implemented `CountOccurrencesInCode`, `CodeLinesOnly`, `CheckPrecedes` and `ExtractControllerFunctionBody` outside the engine and ran **all 40 source-probe assertions** of tests 27 and 28 against the real files: **40/40 pass**, including both ordering chains and the three `GroupRadius*` self-checks. ⛔ **This is NOT a substitute for the suite run** — it cannot compile, and it says nothing about tests 24–26.
⭐ **I also simulated the wheel arithmetic at `float` precision** (`numpy.float32`, the shipped code's exact expression): the fixture walk lands on `2.0` in 6 notches and the **shipped** tunables land on `1.5f` **bit-exactly in 5 notches** — so test 24(c)/(i)'s zero-tolerance equalities are safe.
⚠️ **`819`'s finding 8 applies to me too and I built around it:** `CountOccurrencesInCode` skips whole comment lines but ⛔ **not trailing comments**. ⛔ No forbidden token appears at the end of a code line in anything I wrote.

---

## 10. TESTS — `SC-§37`. **MY DELTA: `+5`.** ⛔ I DO NOT OWN AN ABSOLUTE.

⚠️⚠️ **THE TREE MOVED UNDER ME WHILE I WORKED, exactly as `TASK-819` warned.** Measured with `grep -rh "^IMPLEMENT_.*_AUTOMATION_TEST(" | wc -l`:

- **`370` across 29 files** when I started (2026-09-02 20:48);
- **`376` across 29 files** at hand-off — but `SiegePlacementTest.cpp` went **23 → 28**, so **only 5 of that +6 are mine.** ⭐ **A parallel lane added the sixth and I have no idea which; ⛔ do not attribute it to me.**

> ⇒ **FOR `TASK-824`: RECONCILE, ⛔ DO NOT ADD.** Take a **fresh** `^IMPLEMENT_` census at compile time and reconcile it against each task's declared **delta**. ⛔ **Trust no task's absolute, mine included — `376` was already at risk of being stale when I wrote it.**

**Frame check (⛔ "check first"):** `Tests/SiegePlacementTest.cpp` **exists and is the placement frame** — its own charter says *"`TASK-813` AND `TASK-815` EXTEND IT — ⛔ THEY DO ⛔ NOT ADD A SECOND ONE."* ⭐ **EXTENDED.** The wheel sizes the placement ghost and the building the confirm spawns, so unlike `819`'s discard-all these are ⛔ **not** lodgers. Fourth fixture namespace: `SiegePlacementWheelFixture`.

| # | test | ⭐ what it would CATCH |
|---|---|---|
| 24 | `ThePlacementFootprintWheelStepsOneNotchAndClampsAtBothEnds` | one notch = **two** steps · a fixed increment that ignores the tunable · ⛔ **shrinking below the floor** (`J-3`) · a ceiling reached *near* but not *exactly* · a wheel **dead at the top** (the notch-counter bug) · a **multiplicative** series · five misconfiguration paths · ⭐ a shipped range that needs 40 notches, or that the shipped step can never reach |
| 25 | `ThePlacementFootprintScaleTouchesWidthAndLengthOnlyAndNeverHeight` | ⛔ **a Z the wheel had no right to write** · an **oblong** scale whose footprint depends on the ghost's facing · a mirrored/NaN/zero scale reaching an `FTransform` · ⭐⭐ **a footprint that does NOT track the wheel** — asserted through the real `TransformBy` path, with an **X-only negative control** that must fail to grow it |
| 26 | `ThePlacementWheelIsInertForAnyCardClassThatRefusesFootprintScaling` | ⛔⛔ **a wheel-able WatchTower** (the `TOWER-§8.5a` void) · an **over-broad** exclusion that also caught the Arrow/Bomb/Ballista/Crystal family · a **shadowed non-virtual** override · a null class crashing or defaulting to *true* · ⭐ a CDO that **disagrees with a live instance**, which is the one assumption the seam rests on |
| 27 | `ThePlacementWheelPollLivesOnlyInThePlacementBranchAndReusesNoGroupPickTunable` | ⛔ a **second** call site (`MARK-§4`'s fourth consumer, added quietly) · a poll in the **wrong branch** · a new `InputAction` · ⭐⭐ the wheel polled **after** `UpdatePlacementGhost` (the one-frame lie) · ⛔⛔ **`GroupRadiusWheelStep/Min/Max` reused** — world-space uu that would grow a building to ×5000 · a `CardID` name compare |
| 28 | `TheGhostScaleAndTheSpawnedBuildingScaleAreTheSameValueByConstruction` | ⛔ **a ghost that lies about what it previews** — a second expression, a hand-built `FVector`, a third consumer · the wheel writing the ghost transform behind `UpdatePlacementGhost`'s back · ⭐⭐ **the ghost sized AFTER its footprint is measured** · `ApplyStackUpgrade` growing to write X or Y and breaking `J-4` |

**Every "inert"/"zero" claim is paired with its control:** 24(h) proves the identical call **moves** with a valid step · 26(a)/(c) prove the predicate says **yes** to the family it must not catch · 25(d) proves an asymmetric scale **fails** to grow the footprint · every source-probe zero is paired with a self-check proving the scanner finds that token where it really is (`GroupRadius*` ×3, `FVector(`, `WatchTower` in prose), and every extracted body is proven to be the right function before anything is counted in it.

---

## 11. 📌 M8 DECLARATION (`STACK-§7`) — explicit, ⛔ not boilerplate

Everything this task adds is **client-local, pre-gate presentation and input**: the poll, `PlacementFootprintScale`, `bPendingCardCanScaleFootprint`, the ghost's scale. ⛔ **No new replicated property, ⛔ no new RPC, ⛔ no new relevancy tier, ⛔ no new replicated class.** The three tunables are `EditDefaultsOnly` class defaults, ⛔ not state.

⭐ **The one thing that crosses to the server is the spawn transform**, and it crosses through the **already-shipped** authority path: `TryConfirmPlacement` → `SpawnActorDeferred` → `FinishSpawning`, unchanged in kind. `StackUpgradeCount` remains ⛔ **SERVER-SET** and untouched by me — I never author it, and the wheel never reads it.

---

## 12. ⚠️ DECLARED, ⛔ NOT FIXED (out of fence — ⛔ named, ⛔ not boarded)

- **⛔ NO HUD READOUT OF THE CURRENT SCALE.** The player sees the ghost change size and nothing else — no number, no bar. Spec asks for none and `HELP-§` is `TASK-823`'s. 🧑 **If Jonathan wants "×1.3" on screen, that is a new ask**, and it would want the `RefuseCardPlay` HUD channel or a new one. There is a `Verbose` log (`.cpp:2871`) for the playtest, and ⚠️ **it prints nothing without `Log LogGitClaudeUnrealTest Verbose`.**
- **⛔ THE UPGRADE DOES NOT RE-WHEEL.** `J-4` is his own sentence: an upgrade inherits the placed building's X/Y verbatim and the wheel sizes what you **place**, never what you **grow**. `TASK-813` said the same. ⛔ Nothing to coordinate.
- **`TASK-735`'s three declared misleading survivors** (the point-slope gate under a wide ghost, the point spawn-box test, the other building's footprint as a point) are **unchanged and still theirs** — ⚠️ **but the wheel makes the third one bite harder**: two ×1.5 buildings are still separated by one radius, not two. ⛔ Not mine, ⛔ not fixed, named here because the wheel is what widens the gap.
- **`J-11`'s UV stretch now has an X/Y sibling.** A ×1.5 footprint stretches the texture horizontally exactly as a ×5 Z stretches it vertically. ⛔ Shipped as-is, same ruling, ⛔ no task.
- **`OnCard1Pressed`'s empty-slot Footman fallback** (`TASK-807` item 5) is still declared-not-fixed and is still not mine.
