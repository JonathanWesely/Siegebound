# TASK-791 — health bars hide when the world is in the way — PROGRAMMER HANDOFF

**Status:** `ready-for-qa` → gate is **TASK-801** (one report over TASK-790 + TASK-791).
**Law:** `VIS-§2`, ruling `VIS-R1`; `SHIP-§9c`. **Compile is TASK-802's** — nothing here was built, no editor, no MCP, no Git.

---

## 1. THE SCREEN-SPACE FINDING — VERIFIED AT SOURCE, AND IT HOLDS

**Confirmed:** `CombatantHealthBarComponent.cpp:30` is `SetWidgetSpace(EWidgetSpace::Screen);`. Read directly, not taken on the spec's word.

I then went further than the spec asked, because the whole fix hinges on a mechanism nobody had checked — **does `SetVisibility` actually hide a Screen-space widget?** It does, and the reason is one engine line:

> `UWidgetComponent::UpdateWidgetOnScreen()` — UE 5.8 `WidgetComponent.cpp:1317`
> `if (TargetPlayer && PlayerController && IsVisible() && !(GetOwner()->IsHidden()))` → `AddWidgetToScreen(...)`, **else** `RemoveWidgetFromScreen()`

⭐ **That is the entire mechanism by which this cull works**, and it is why the fix is expressed as visibility rather than as anything to do with depth. `UpdateWidgetOnScreen()` is called from `UpdateWidget()`, which runs inside `TickComponent` — so a visibility change lands on the next frame's tick.

**Two engine facts I checked that a reviewer should NOT have to re-derive:**

1. ⚠️ **`SWorldWidgetScreenLayer`'s own visibility check is a red herring.** Its `!IsWidgetVisible()` collapse (`SWorldWidgetScreenLayer.cpp:141`) is gated behind `GSlateWorldWidgetIgnoreNotVisibleWidgets`, which **defaults to `false`** — and `UWidgetComponent::IsWidgetVisible()` only consults the component's own `IsVisible()` **in `World` space** (`WidgetComponent.cpp:1077`); in Screen space it reads the inner `UUserWidget`'s visibility instead. ⇒ **that path is not what hides our bar.** The `UpdateWidgetOnScreen` path above is.
2. ⛔ **`SetTickMode(ETickMode::Enabled)` is now doubly load-bearing.** Under `Automatic`/`Disabled` the engine turns a component's tick **off** once its widget is invisible (`WidgetComponent.cpp:1264`) — which would strand a culled bar hidden forever with no tick left to re-poll and un-hide it. I added a `⛔ do not "optimise" this` comment at the constructor.

**Conclusion:** the spec is right — this is a rendering mode behaving as documented, not a broken setting. `EWidgetSpace::World` was **not** touched.

---

## 2. THE INSTRUMENT, AND WHAT IT COSTS

**Chosen: a polled line trace, camera → the bar's world anchor.** (`VIS-R1`'s ruled instrument.)

| | |
|---|---|
| **Query** | `UWorld::LineTraceTestByChannel` — the **test** form: boolean answer, no `FHitResult`, no impact point, nothing allocated |
| **Channel** | `ECC_Visibility` (pinned) |
| **From** | the **owning player's** camera via `UWidgetComponent::GetOwnerPlayer()` → `PlayerController` → `PlayerCameraManager` |
| **To** | `GetComponentLocation()` — the owner's root + `BarHeightZ`, i.e. the exact point the screen layer projects |
| **Ignores** | the owner actor (ctor arg); `bFindInitialOverlaps = false` |

### Per-frame cost per unit — stated plainly, as demanded

- **Cull off:** **one bool test.** Nothing else. No accumulate, no trace, no `SetVisibility`.
- **Owner has the bar hidden** (dead, or a miner's `bShowHealthBar = false`): **two bool tests, forever.** ⭐ Opted-out units cost **zero traces for their whole life**.
- **Live, opted-in, non-poll frame:** **one float add + one compare** (`ShouldPollOcclusion`). ⛔ **No trace.**
- **Poll frame** (default every `0.15 s` ⇒ ≈6.7 Hz): one camera-manager fetch + **one line trace**.

**Roster arithmetic at 60 fps, 20 units:** `20 × 6.67 ≈ 133 traces/second ≈ 2.2 traces per frame across the entire roster`. A per-frame design would be `20 traces/frame = 1200/second` — **9× more**, and it is the cost `VIS-§2` forbids by name.

**The price I am paying for that, stated rather than hidden:** up to **one period of staleness**. A bar can persist ≤150 ms after the world closes in front of it, and stay hidden ≤150 ms after it clears. Plus a **≤1-frame** latency from the tick ordering. Both are documented on the members.

### Why not the alternatives

- **`WasRecentlyRendered` / `LastRenderTime`:** rejected. It is updated by **any** view — including shadow-depth and scene-capture passes — is latent by one to several frames, and answers "was this primitive drawn?" rather than "is the world between the camera and the **bar**". The bar sits `BarHeightZ` above a mesh that may itself be visible while the bar's anchor is not.
- **Distance / frustum test:** rejected outright — it cannot see a wall, and a wall is the entire defect.

### Two design calls a reviewer should weigh

1. ⚠️ **`bFindInitialOverlaps = false`, and this one interacts with TASK-790.** A trace that **starts inside** a blocking hull reports an initial-overlap hit. **TASK-790 is repairing exactly that state** — the spring arm collapsing into the watchtower for ~2 s at the ladder approach (VID-004 01:12.0–01:13.5). Without this flag, every unit on screen would read as occluded during any such moment and **the whole roster's bars would blink off together**. Suppressing initial overlaps makes a buried camera fail **open**. ⭐ **This is the finding I most want QA to challenge**, because it is a judgement call, not a law.
2. **Reset-to-zero, not `-= Period`.** After a 1.0 s hitch a subtracting accumulator still holds ~0.85 s — six more periods — and would fire on **six consecutive frames**, per pawn, on the frames immediately after a hitch. Dropping the arrears is the right trade for a cull whose skipped answers are stale anyway. Test 6 is the only thing that tells the two implementations apart.

---

## 3. THE COMPOSITION — AND THE REGRESSION IT PREVENTS

⭐ **The cull introduced a SECOND writer of this component's visibility, and that is the real hazard in this task — not the trace.**

The obvious wrong shape (`poll, then SetVisibility(!bOccluded)`) would make **every dead unit's bar pop back on ~150 ms after death, all over the field** — a far louder defect than bars drawing through stonework.

So visibility is now composed, never overwritten:

```
ComputeDesiredBarVisibility(bOwnerWantsBarShown, bCullEnabled, bOccluded)
    = bOwnerWantsBarShown && !(bCullEnabled && bOccluded)
```

- `ShowBarIfEnabled()` / `HideBar()` no longer call `SetVisibility` — they **latch the owner's intent** (`bBarShownByOwner`) and delegate to `ApplyBarVisibility()`, the **single writer**.
- ⭐ **The owner's intent is the outer AND.** The cull can only ever **subtract**. There is no input by which a clear trace shows a bar the owner hid.
- **`bOccludeHealthBarWhenBlocked = false` collapses this to `bOwnerWantsBarShown`** — bit-for-bit the two lines it replaced. That is the pinned regression guard, and it is asserted over the whole input space.

**Fail-open everywhere:** `UpdateHealthBarOcclusion()` **clears** occlusion first, so every early-out (no world, no widget, no owner, no local player, no camera manager) resolves to **visible**. A missing camera must never blank every health bar on the field.

---

## 4. ⚖️ THE RULING ON THE CASTLE BAR AND THE DAMAGE NUMBERS — **NAMED AS SEPARATE TASKS, NOT FIXED HERE**

I was fenced out of both, and I did not touch them. But the dispatch is right that a fix helping one of three leaves the symptom on screen, so here is the ruling with the structural fact that decides it:

⭐ **Neither sibling is a `UCombatantHealthBarComponent`.** Both create the **base** `UWidgetComponent`:

- `Castle.cpp:287` — `HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(...)`
- `DamageNumberActor.cpp:31` — `NumberWidget = CreateDefaultSubobject<UWidgetComponent>(...)`

⇒ **the cull cannot leak to them and cannot be inherited by them.** Extending it is not a one-line widening; it needs either a component-class swap or a second implementation. That is real design content, which is exactly why they are tasks and not a fence I quietly widened.

**My recommendation to the manager, per row:**

| site | verdict | reasoning |
|---|---|---|
| **`ACastle::HPBarWidget`** | ⚠️ **OWED — worth a task, LOW priority** | Its anchor sits at **z = 9450**, ~1.17× above an 8083-tall mesh, i.e. above essentially all playable geometry — so it is rarely occluded in practice and **has never been reported**. But "rarely" is not "never": a hero on the watchtower deck with the castle behind the tower body is the exact geometry that produced V3. ⛔ It is **not** free — it needs the class swap, and `ApplyTeamVisuals`/`HandleDestroyed`/`ResetCastle` all touch that component's lifecycle. **Recommend: a task, boarded, not urgent.** |
| **`ADamageNumberActor::NumberWidget`** | ⛔ **RECOMMEND CLOSING WITHOUT A FIX** | A damage number lives well under a second, rises and self-destructs, and is spawned **at the moment of a hit the player just caused**. ⭐ A cull here would mostly *remove* feedback the player is actively looking for, and a 0.15 s poll on a ~0.8 s actor is a coin flip rather than a rule. ⚠️ And it ticks per frame already (`PrimaryActorTick.bCanEverTick = true`) with a `LiveCount` cap — the natural implementation *would* be per-frame, which is the cost `VIS-§2` refuses. **If the manager disagrees, it needs a different instrument (spawn-time single trace), not this one.** |

⛔ **I did not board these myself** — naming them is my job, boarding them is the manager's.

---

## 5. TESTS — `Siegebound.HealthBarOcclusion.*` — AND WHAT EACH WOULD CATCH

**New file:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHealthBarOcclusionTest.cpp`

⭐⭐ **The stated hazard was that a visibility cull's tests pass because nothing was visible either way.** Three explicit anti-vacuous clauses are the answer, flagged in the file header so a reviewer reads them first:

| # | test | what it catches — i.e. how it FAILS |
|---|---|---|
| 1 | `ABlockedBarHidesAndAClearBarShowsAndTheAnswersDiffer` | A blocked bar hides; a clear one shows. ⭐ **(c) asserts the two answers DIFFER** — red the moment the implementation stops reading `bOccluded` (a cull still costing a trace but no longer changing anything). |
| 2 | `TheCullOnlySubtractsSoAnOwnerHiddenBarStaysHidden` | The dead-unit resurrection regression. Exhaustive over all four `(cull × occluded)` inputs. Fails against an OR, an argument-order swap, or any future term that forgets the owner is outermost. |
| 3 | `DisablingTheCullReproducesThePreTask791BehaviourExactly` | The pinned regression guard, over the whole input space. ⭐ **(b) is the anti-vacuous half:** (a) alone passes perfectly for a cull that does nothing in *either* state, so (b) asserts enabling the cull changes at least one outcome — that the switch has two sides. |
| 4 | `ThePollFiresOnAFractionOfFramesAndOnMoreThanZero` | The perf clause. 600 frames @60 fps: fires ≈66, not 600. ⭐ **(b) asserts fires > 0** — "never traces" satisfies "doesn't trace per frame" perfectly while leaving the feature inert and tests 1–3 green. **(d)** asserts the fires are **spaced** ≈9 frames apart, so a front-loaded burst can't hide behind a healthy total. |
| 5 | `AZeroOrNegativeIntervalCannotDegradeIntoAPerFrameTrace` | `HealthBarOcclusionIntervalSeconds` is `EditDefaultsOnly` and "0 = as fast as possible" is a natural misreading. Asserts the **documented contract** (≤30 polls/s ⇒ ≤1 fire per 2 frames), ⛔ **not** the floor constant transcribed from the `.cpp`. Plus: it still polls — a floor that clamped to infinity would also pass. |
| 6 | `ASecondLongHitchDoesNotBankACatchUpBurstOfTraces` | The `-= Period` catch-up. A 1.0 s hitch fires **once**, then the following ~8 frames fire **zero** times. A subtracting accumulator fires on every one of them. |
| 7 | `TheShippedDefaultsAreTheOnesTheLawPinned` | CDO reflection on all three pinned tunables + `IsHealthBarOccluded()` resting **false** (fail-open). Each lookup `AddError`s by name if `FindFProperty` returns null, so a rename cannot silently make the test vacuous. |
| 8 | `TheWidgetSpaceIsStillScreenBecauseWorldSpaceWasRefused` | ⭐⭐ **Defends the ruling, not a behaviour.** The next person to see a bar through a wall (the cull is a *poll* — a bar can persist one period) has every reason to reach for `EWidgetSpace::World`. `VIS-R1` refused it; documents don't go red, this does, and it names the ruling in its failure text. |

**Two things the tests deliberately do NOT cover, stated so nobody mistakes green for done** (`SC-§32`, and both are written into the file header):

- ⛔ **The trace itself.** `UpdateHealthBarOcclusion()` needs a world, a local player and a camera manager. There is no headless path, and I did **not** invent an injectable camera purely so a test could reach it. **Its instruments are QA's diff read and Jonathan's pixels** — stand on the watchtower deck with units on the ground below and confirm the four floating bars from **VID-004 @ 02:08.0** are gone.
- ⛔ **That `SetVisibility` hides a screen-space bar.** That is an engine guarantee, verified by reading UE 5.8 source (§1 above). A test could only restate it.

---

## 6. SUITE DELTA

| | |
|---|---|
| Baseline | **279** |
| **TASK-791 (mine)** | **+8** ⇒ **287** |
| ⚠️ Also on disk | `SiegeHeroCameraTest.cpp` **+10** — **TASK-790's**, landed in parallel, ⛔ not mine |
| **Total TASK-802 should expect** | **297** |

Counting command: `grep -c IMPLEMENT_SIMPLE_AUTOMATION_TEST Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`

---

## 7. FILES TOUCHED

- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` — 3 pinned `UPROPERTY`s, `IsHealthBarOccluded()`, two pure statics, `TickComponent` override, 2 helpers, 3 private runtime fields, class-comment section.
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp` — the cull; `ShowBarIfEnabled`/`HideBar` rerouted through `ApplyBarVisibility()`; constructor comments; `MinHealthBarOcclusionPeriodSeconds` in the existing anonymous namespace.
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHealthBarOcclusionTest.cpp` — **new**, 8 tests.

⛔ **Not touched:** `Castle.cpp` · `DamageNumberActor.cpp` · `HeroCharacter.{h,cpp}` (TASK-790 live) · `ClimbableTower.*` · `SummonedUnit.*` · `SiegeLadderClimbStatics.*` · `CONVENTIONS.md`.

**Assets referenced:** `/Game/UI/WBP_CombatantHealthBar` (pre-existing default, unchanged).

---

## 8. ⭐ WHAT QA SHOULD SCRUTINISE

1. ⭐ **`bFindInitialOverlaps = false`** — the TASK-790 interaction in §2. Judgement call, argued not assumed.
2. **The two new public statics.** They are the **shipping implementation** (`TickComponent`/`ApplyBarVisibility` are their callers), not accessors added to make a test possible — the `HeightAdvantageMultiplier`/`HIGH-§3` precedent. Plain C++ statics, deliberately **not** `UFUNCTION`s.
3. **`bBarShownByOwner` defaults `true`**, matching the constructor's deliberate "do NOT start hidden". ⚠️ `BeginPlay` can return early on an unresolved widget class **without** reaching `ShowBarIfEnabled()` — `false` here would silently change that path.
4. **`ECC_Visibility` is load-bearing, not decorative.** The engine's stock `Pawn` and `CharacterMesh` profiles both set Visibility to `ECR_Ignore` (`BaseEngine.ini:3110/3112`), so the trace passes **through** units and stops on world geometry. Retargeting it to `ECC_Camera`/`ECC_Pawn` changes the **question** — on `ECC_Pawn` every bar would flicker whenever a friendly crossed the camera line.
5. **Known false-hide, accepted:** a unit behind a low parapet with a visible torso but an occluded anchor gets its bar hidden. The trace ends at the **bar**, which is the thing being culled.
6. `FBoolProperty` is read via `GetPropertyValue_InContainer`, **not** a `ContainerPtrToValuePtr<bool>` cast — bitfield safety; a reflection test that lies is worse than none.

---
---

# TASK-791 — QA-LOOP 1 · REPAIR OF `qa/TASK-801.md` (FAIL, 2 blockers)

**Date:** 2026-09-02 · **Status:** `qa-failed` → `ready-for-qa`
**Fence this pass:** `CombatantHealthBarComponent.{h,cpp}` · `Tests/SiegeHealthBarOcclusionTest.cpp` · **`SummonedUnit.cpp:4252` ONLY** (granted).
⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ `HeroCharacter.{h,cpp}` untouched (verified: 0 markers of mine) · ⛔ `Castle.cpp` / `DamageNumberActor.cpp` / `ClimbableTower.*` / `SiegeLadderClimbStatics.*` untouched · ✅ `LADDER_OUTWARD_SHIFT = 10.0` **intact and still PINNED** (`build_watchtower.py:137`).

⚖️ **QA was right on both counts, and it was right about WHY I could not see them: both blockers live in the call graph, and both of my honest `SC-§32` "not covered" declarations were pointing straight at them.** I have fixed both, and I have made as much of both testable headlessly as is honest — with the residue named plainly at the end.

---

## ⛔ B-1 — THE UNLATCHED SECOND WRITER

### The line, as repaired

`Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — the death-anim hold branch of `HandleDeath()`:

```cpp
        if (HPBarWidget)
        {
            // ⛔ HideBar(), ⛔ NEVER SetVisibility() — qa/TASK-801 B-1, and this is the ONE line of this
            // file TASK-791 owns. [...] a raw SetVisibility(false) leaves the latch reading "shown", so
            // the next poll ≈150 ms later recomputes visible and puts a 0-HP bar back over the corpse
            // for the whole death-anim hold.
            HPBarWidget->HideBar();
        }
```

⛔ **One call line changed, plus a guard comment. Nothing else in that file** — the diff is 1 deletion / 8 insertions and `git diff` confirms it touches only this block.

### ⭐ THE CENSUS QA ASKED FOR — IS THE SHAPE ANYWHERE ELSE ON MY PATHS?

`CreateDefaultSubobject<UCombatantHealthBarComponent>` occurs in **exactly three files**, and that is the complete owner set:

| owner | raw visibility write? | verdict |
|---|---|---|
| `ASummonedUnit` (`SummonedUnit.cpp:174`) — and `AMinerUnit` through it | ⛔ **YES — `:4252`** | ✅ **FIXED.** The only one. |
| `ABuilding` (`Building.cpp:62`) — and `ATower`/`ABarracks`/`ADeepMine` through it | ✅ **NONE.** `HPBarWidget` appears only at `:62`/`:63` (create + attach). | ✅ **And it cannot acquire the shape by accident:** `ABuilding::HandleDestroyed()` calls `Destroy()` (`Building.cpp:370`), so the actor is gone in the same frame — there is ⛔ no corpse window in which a poll could run. |
| `AHeroCharacter` (`HeroCharacter.cpp:133`) | ✅ **NONE** — `HideBar()` at `:869`, `ShowBarIfEnabled()` at `:920`. | ✅ **Already correct, and it is the proof the latch design was sound**: the hero is *hidden, not destroyed* on death, so he has a real corpse window and would have shown the identical bug had he written raw. |

⚠️ **`ACastle::HPBarWidget` DOES carry the same shape** — `Castle.cpp:1251` (`SetVisibility(!bNowDestroyed, true)`) and `:1447` (`SetVisibility(true, true)`). ⛔ **I did not touch them** (separate boarded task, and its component is still a base `UWidgetComponent` so it is legal today). ⭐ **But the boarded task MUST carry this**: the class swap it proposes would inherit `B-1`'s exact shape at two sites at once, and `ACastle`'s bar has *three* lifecycle touchpoints (`ApplyTeamVisuals` / `HandleDestroyed` / `ResetCastle`). That is now written into the component's own class comment as a standing law, and into the test-10 fixture comment beside the owner census, so it fails loudly rather than being rediscovered.

⭐ **Also fixed by the same edit, per QA's cost note:** dead units now latch `bBarShownByOwner = false`, hit `TickComponent`'s early-out, and spend **zero traces** through the death hold — the cost claim in §2 is true again for that population.

---

## ⛔ B-2 — THE INERT INSTRUMENT · ⭐ THE MECHANISM I CHOSE, AND WHY THE OTHERS LOSE

### First: QA's engine finding, re-verified independently line by line

I did not take it on the report's word. `bDiscardInitialOverlaps` has **exactly one consumer in the entire engine** (`CollisionQueryFilterCallback.cpp:217`), it sits behind `if (!bIsSweep) { return ECollisionQueryHitType::Block; }` at `:211-214`, and `SceneQuery.cpp:522` constructs the callback with `Traits::GeometryQuery == ESweepOrRay::Sweep`. ⇒ **on a raycast the flag is never read. My line 303 was a no-op and my 8-line comment promised protection that did not exist.** ⛔ Fully conceded.

### ⭐ THE CHOICE: **detect the burial on the HIT** — `LineTraceSingleByChannel` + discard a start-penetrating / zero-distance hit

```cpp
FHitResult OcclusionHit;
const bool bTraceBlocked = World->LineTraceSingleByChannel(
    OcclusionHit, CameraLocation, BarAnchorLocation, HealthBarOcclusionChannel, QueryParams);

bHealthBarOccluded = ComputeOcclusionFromTraceResult(
    bTraceBlocked, OcclusionHit.bStartPenetrating, OcclusionHit.Distance);
```

```cpp
bool UCombatantHealthBarComponent::ComputeOcclusionFromTraceResult(bool bTraceBlocked, bool bTraceStartedInsideGeometry, float HitDistanceUU)
{
    return bTraceBlocked && !bTraceStartedInsideGeometry && HitDistanceUU > 0.f;
}
```

**⭐ The chain that makes this work is verified end-to-end in UE 5.8 source, and it is the part QA could not have assumed:** `bStartPenetrating` **is** set on the raycast path.

1. `ChaosInterfaceWrapperCore.h:117` — `HadInitialOverlap(const FLocationHit& Hit) { return Hit.Distance <= 0.f; }` — ⭐ **a pure distance test with nothing sweep-specific about it.**
2. `CollisionConversions.cpp:319` — `const bool bInitialOverlap = HadInitialOverlap(Hit);`
3. `:321` — `if (bInitialOverlap && Geom)` — for a **raycast `Geom == nullptr`**, so the sweep-only branch is skipped and it falls through.
4. `:366` — `OutResult.bStartPenetrating = bInitialOverlap;` and `:371` — `OutResult.Distance = GetDistance(Hit);`
5. `ConvertTraceResults<FHitRaycast>` is **explicitly instantiated** for both the `FHitResult&` and `TArray` forms, and is reached from `SceneQuery.cpp:571`.

⇒ a raycast from a buried camera returns `bBlockingHit == true`, `bStartPenetrating == true`, `Distance == 0`. **The burial is a first-class, detectable fact on the raycast path.**

⭐ **And the rule is the engine's own:** clauses 1-2 are character-for-character `FHitResult::IsValidBlockingHit()` (`HitResult.h:236-239` — `bBlockingHit && !bStartPenetrating`). The distance clause is a deliberate **second witness** read from the raw hit rather than the conversion layer; on today's path they cannot disagree (step 1 *defines* the flag as `Distance <= 0`), and that is exactly why it is cheap insurance for a bool that decides whether the whole roster's HUD survives.

### ⚖️ WHY THE OTHER THREE LOSE

| option | ruling |
|---|---|
| **Ignore the tower actor in the query params** | ⛔ **LOSES HARDEST — it deletes the fix.** The tower is not incidental scenery here; it *is* V3. VID-004 @ 02:08.0 is four bars floating over the watchtower's **empty deck**, drawn through the tower's own stonework. Ignoring `AClimbableTower` would make exactly those four bars visible again. It also needs `ClimbableTower.h` in this component (a fence I do not hold) and hard-codes one actor class into a general cull. |
| **Start the trace outside the near geometry** | ⛔ **Loses on three counts.** (a) It needs a magic offset, and the only non-arbitrary value is TASK-790's 150 uu tunable — coupling my file to a number in a file I am fenced out of, which is precisely the cross-fence dependency `VIS-§1` warns about. (b) It is not even correct: at 150 uu of burial the shifted start can land *still inside* the hull or *through* the far wall depending on approach angle. (c) It silently blinds the cull to every genuine occluder within the offset of the camera. |
| **Reverse the ray (bar → camera), keeping the cheap TEST form** | ⛔ **Relocates the wrong answer instead of fixing it.** The start would be outside the hull, so no initial overlap — but the ray still crosses the tower shell between the bar and the buried camera and returns a blocking hit at a *positive* distance. The bar reads occluded exactly as before. ⭐ Worth stating because it is the tempting cheap fix. |
| **A sweep** | ⛔ **Refused, and I could ⛔ NOT show it affordable** — which by the dispatch's own terms means it loses. A sweep needs a geometry, computes MTD, and pays per-shape sweep math against every candidate; `GetHitFlags()`/`GetQueryFlags()` (`SceneQuery.cpp:364-400`) give the sweep path strictly more work than either ray form. It also **changes the question**: a swept sphere of radius *r* asks "is anything within *r* of the line", which multiplies false hides at wall edges. ⚠️ And it would fix `B-2` only *incidentally*, via a flag that a future edit could flip back. |

### ⭐ WHY MINE IS BETTER THAN THE FLAG IT REPLACES, NOT JUST DIFFERENT

- **General, not tower-specific.** It knows nothing about `AClimbableTower`, TASK-790, or 150 uu. Any camera inside any blocking geometry — a future interior, a debug fly-cam, a castle — fails open for free.
- **Query-kind agnostic.** A start-penetrating hit reports `Distance 0` whether the query is ever a ray or a sweep, so the fail-open survives a change of instrument. ⛔ The flag does not: it is *silently* inert on one and live on the other, which is the entire reason this shipped.
- **It joins the existing ledger.** The burial is now the fifth entry on the fail-open side, next to no-world / no-widget / no-owner / no-camera — the same rule, stated once: *"we could not tell" must never blank the HUD.*

### ⚠️ THE COST, STATED RATHER THAN GLOSSED — AND THE ACCEPTED RESIDUAL

**Cost.** `LineTraceTestByChannel` carries `EQueryFlags::AnyHit` + `EHitFlags::None` (`SceneQuery.cpp:368`, `:394`), stops at the **first** blocking hit and skips `ConvertTraceResults` entirely (`:569`). `LineTraceSingleByChannel` drops `AnyHit` — the traversal must resolve the **nearest** hit — and pays one hit conversion. ⭐ **It is still ONE raycast, still ZERO heap allocation** (one stack `FHitResult`), and still **≈2.2 queries/frame across a 20-unit roster** at the 0.15 s period. The per-frame cost of the cull on a non-poll frame is unchanged.

**Residual, declared:** a single-hit query stops at the closest hit, so while the camera is buried we discard that hit and **do not look past it** — a genuine wall beyond the tower would not be seen for that ≈2 s window. That *is* the fail-open, deliberately, and it is bounded by the burial window. ⛔ The precise alternative (`LineTraceMultiByChannel`, skip zero-distance hits, take the next blocker) allocates a `TArray<FHitResult>` per poll and forfeits the early-out — the cost `VIS-§2` rations by name. **Recorded as the upgrade path if Jonathan's pixels ever show the fail-open is too permissive.**

---

## ✅ THE WARN/NIT ITEMS THAT LIVE INSIDE MY FENCE — ALL REPAIRED

- **`W-4` (the over-stated `SetTickMode` claim) — FIXED, and re-verified at source myself.** `SetTickMode(Enabled)` is load-bearing against `ETickMode::Disabled`, which kills the tick outright (`WidgetComponent.cpp:1275-1279`). ⚠️ QA is right that `:1264` is a different mechanism: it keys on `IsWidgetVisible()`, and `IsWidgetVisible()` (`:1074-1090`) consults the **component's** `IsVisible()` **only** when `Space == EWidgetSpace::World` (`:1077`); in Screen space it reads the inner `UUserWidget`. ⭐ **And `bPropagateToChildren` propagates to child *scene components*, not into the widget** — so the cull genuinely never touches what `:1264` reads. Both the constructor comment and the `TickComponent` doc now say exactly this.
- **`N-1` (`Cond ? TObjectPtr<T> : nullptr`) — FIXED, but ⛔ NOT with `ToRawPtr`.** I removed the construct instead of restating it: two early-outs replace both mixed-type ternaries, so there is no conditional expression to resolve at all. ⭐ **Deliberate on a no-compile task** — adopting a new symbol I cannot compile-check to satisfy a NIT QA itself marked "not a compile risk" is the wrong trade; the early-out form uses only the plain copy-initialisation conversion this file already relies on, and reads like every other fail-open above it.
- **`N-2` (the out-of-line static call) — FIXED.** The per-frame figure is now stated as *one call around one add and one compare, ~20 calls/frame for the roster*, in both the call-site comment and the `ShouldPollOcclusion` doc.
- **`W-3` (the wrong `SpringArmComponent.cpp:84-86` citation) — ⛔ NOT MINE TO FIX.** All three sites are TASK-790's (`handoffs/TASK-790-programmer.md`, `HeroCharacter.h:1139`, `SiegeHeroCameraTest.cpp:28`) and every one is behind a fence I do not hold. ⭐ **Flagged for the orchestrator: the correct lines are `:27` (`bDoCollisionTest`), `:34` (`ProbeSize`), `:35` (`ProbeChannel`) — I re-verified them.** A shipped comment with a wrong citation in a wave whose whole method is `file:line` is worth one edit whenever 790's fence next opens.
- **`N-3` / `N-4` — TASK-790's files, out of fence.** `N-5` is addressed above and written into the component's class comment.

---

## ⭐ WHAT I MADE HEADLESSLY TESTABLE — AND WHAT I COULD NOT

QA's counter-observation was the sharpest thing in the report: *"anti-vacuity was applied rigorously to what the suites COVER, and both blockers live in what they DON'T."* Four new rows, `+4`:

| # | test | catches |
|---|---|---|
| **9** | `ACameraInsideGeometryFailsOpenInsteadOfBlankingEveryBar` | ⭐ **`B-2` as a rule.** (a) buried ⇒ not occluded · (b) **anti-vacuous:** a real wall at 900 uu *does* occlude (a stuck-false rule passes (a) perfectly and switches the whole cull off) · (c) exhaustive over the unblocked column · (d) **each veto clause is independently load-bearing** — delete either and one row goes red · (e) ⭐⭐ **end-to-end across both pure seams:** feed the buried-camera answer into `ComputeDesiredBarVisibility` for a live unit and assert **VISIBLE** — the roster-wide-blackout symptom, named. |
| **10** | `EveryOwnerHidesTheBarThroughHideBarAndNeverThroughSetVisibility` | ⭐ **`B-1`, the assertion QA asked for by name.** Source fence over the complete C++ owner set with a **positive control per file** (`HPBarWidget` must appear, or the probe reports "would have passed VACUOUSLY"), plus the **anti-vacuous total**: the latch route must actually be *used* somewhere (measured: **3** calls), because "nobody writes SetVisibility" is satisfied perfectly by a codebase where nobody touches the bar. |
| **11** | `TheOwnerIntentLatchIsWrittenOnlyByShowBarIfEnabledAndHideBar` | ⭐ **The other half of `B-1`** — the second assertion QA asked for. `bBarShownByOwner =` exactly **2×** file-wide, **1× in each entry-point body** (so (a) cannot stay green with the writes relocated into the poll), each body writes `SetVisibility` **0×**, and the **positive control** that `SetVisibility(` appears exactly **1×** in the whole component — the single writer exists and is alone. |
| **12** | `TheTraceIsStillTheSingleHitFormBecauseABooleanCannotSeeABuriedCamera` | ⭐ **Defends the `B-2` ruling** (the test-8 idiom). `LineTraceSingleByChannel` ×1 · `LineTraceTestByChannel` ×**0** · `bFindInitialOverlaps` ×**0** on code lines · and (d) the **anti-vacuous** clause: the hit must actually be *consulted* (`bStartPenetrating` read, routed through `ComputeOcclusionFromTraceResult`) — otherwise the code pays for the expensive query and throws the answer away, which is `B-2` in a costlier costume. |

⭐⭐ **Every count in tests 10-12 was MEASURED ON DISK before shipping, not predicted** — I re-implemented `CountOccurrencesInCode`'s comment-skip rule exactly and ran it against the real files. Measured: owners `HPBarWidget->SetVisibility` = **0/0/0**, latch calls = **3**; component `bBarShownByOwner =` = **2**, `SetVisibility(` = **1**, bodies **1/0** and **1/0**; `LineTraceSingleByChannel` **1**, `LineTraceTestByChannel` **0**, `bFindInitialOverlaps` **0**, `bStartPenetrating` **1**, `ComputeOcclusionFromTraceResult` **2**. **All 12 rows should run green on today's tree.**

⚠️ **The comment-skip is load-bearing here and I want QA to check it:** the symbols these probes forbid are named repeatedly in the shipped comments that explain *why* they are forbidden. A naive count would read those warnings as violations and report a permanent, unfixable red.

### ⛔ WHAT I STILL COULD NOT MAKE TESTABLE — SAID AS PLAINLY AS LAST TIME

- ⛔ **That a real camera inside a real tower produces a real zero-distance hit.** Tests 9 and 12 prove *the rule* and *that the query can still feed it*; they do **not** prove the physics. That chain rests on reading UE 5.8 source (the five steps above) and is owed a **PIE row**. ⛔ There is no headless path to a `UWorld` + `PlayerCameraManager` + physics scene, and I again did **not** invent an injectable camera to manufacture one.
- ⛔ **The runtime tick and the visibility WRITE.** Still uncovered — `W-5` stands. Tests 10 and 11 fence the *shape* of the write; nothing asserts that a poll on a live component actually flips a live widget.
- ⛔ **A source probe catches a shape, not an alias.** A raw write through a local variable (`auto* Bar = HPBarWidget; Bar->SetVisibility(false);`) slips past test 10. It is here because the alternative is nothing.
- ⇒ ⛔ **TASK-802 must still not read 301-green as "V2 and V3 are fixed."** QA's five PIE rows all stand, and rows 3 and 4 are now the **direct** verification of these two repairs.

---

## SUITE DELTA — ⭐ **301**

| | |
|---|---|
| Baseline (`qa/TASK-779.md`) | **279** |
| TASK-790 (not mine, on disk) | **+10** |
| **TASK-791 — mine, was `+8`, now `+12`** | **+4 this pass** |
| **⛔ TASK-802 must expect** | **301** |

**Reconciled two ways on disk**, both agreeing: `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` across `Siegebound/Tests/*.cpp` = **301**; the wider `^IMPLEMENT_(COMPLEX|CUSTOM|SIMPLE)_*_AUTOMATION_TEST` sweep over all of `Source/` = **301** (⇒ still no complex/custom macros anywhere). `301 − 10 − 12 = 279` = the baseline **exactly**.

---

## FILES TOUCHED THIS PASS

- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.h` — the third pure static + its doc; `UpdateHealthBarOcclusion` doc; `W-4`/`N-2` corrections; two new class-comment laws (the owner latch law, the buried-camera fail-open).
- `Source/GitClaudeUnrealTest/Siegebound/CombatantHealthBarComponent.cpp` — `#include "Engine/HitResult.h"`; the trace swap + `ComputeOcclusionFromTraceResult`; `N-1` early-outs; `W-4`/`N-2` comment corrections.
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — ⭐ **`:4252` only**, `SetVisibility(false)` → `HideBar()` + guard comment.
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHealthBarOcclusionTest.cpp` — 4 new rows (8 → **12**), source-probe fixture, header contract updated.

⛔ **Not touched:** `HeroCharacter.{h,cpp}` (verified 0 markers of mine) · `Castle.cpp` · `DamageNumberActor.cpp` · `ClimbableTower.*` · `SiegeLadderClimbStatics.*` · `GitClaudeUnrealTestCharacter.{h,cpp}` · `build_watchtower.py` (`LADDER_OUTWARD_SHIFT` intact) · `CONVENTIONS.md`.

**Assets referenced:** `/Game/UI/WBP_CombatantHealthBar` (pre-existing default, unchanged).

