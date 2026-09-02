# QA Report — TASK-801 (GATE-1, ONE gate over TASK-790 + TASK-791)

## ⭐ FINAL VERDICT (after qa-loop 1): **PASS** — 0 BLOCKERS · 2 open WARNS (declared + accepted) · 1 deferred NIT
**Suite total for `TASK-802`: 301** — ⛔ **not 297**; recomputed on disk after loop 1 (`§L-5`).
**Flips for the orchestrator to route:** `TASK-790` → **`qa-passed`** · `TASK-791` → **`qa-passed`**
⇒ **The loop-1 re-review is `§L`, at the bottom.** Everything above it is the **ROUND-0 record, kept verbatim** — the two blockers are marked `RESOLVED`, ⛔ not deleted, because the reasoning is what the fixes rest on and what the PIE rows still test.

---

## ROUND 0 — verdict as filed: **FAIL** — 2 BLOCKERS · 5 WARNS · 5 NITS *(superseded by `§L`)*

**Reviewed:** `HeroCharacter.{h,cpp}` · `CombatantHealthBarComponent.{h,cpp}` · `Tests/SiegeHeroCameraTest.cpp` (new) · `Tests/SiegeHealthBarOcclusionTest.cpp` (new)
**Law:** `VIS-§2`/`§3` (`VIS-R1`/`VIS-R2`) · `VIS-§1` · `SC-§35` · `SHIP-§9c` · baseline `qa/TASK-779.md`
**Fences held by this gate:** ⛔ no edits · ⛔ no compile · ⛔ no engine/MCP · ⛔ no Git · ⛔ no TASKBOARD write (per dispatch; status flips are returned to the orchestrator, ⛔ not written here).

⚖️ **BOTH DIFFS ARE INDIVIDUALLY WELL-BUILT AND BOTH BLOCKERS ARE THINGS NEITHER AUTHOR COULD SEE ALONE.** `TASK-790`'s refutation is correct, its scope ruling is correct, and its "nothing to unwind" property is genuinely established. `TASK-791`'s engine research is correct to the line and its composition rule is correct *as a function*. ⛔ **Both blockers live in the ⛔ CALL GRAPH, ⛔ not in the leaf code** — which is `VIS-§1` restated: *a grep of the leaf class is not evidence about inherited behaviour*, and neither is a proof about a pure function evidence about its callers.

| | round 0 | after loop 1 |
|---|---|---|
| `TASK-790` | ⛔ qa-failed — 1 shared blocker (`B-2`, the half it CAUSES) · 2 WARN · 3 NIT | ✅ **qa-passed** |
| `TASK-791` | ⛔ qa-failed — 2 blockers (`B-1` sole, `B-2` shared) · 3 WARN · 2 NIT | ✅ **qa-passed** |
| Automatic-fail gates | ✅ ALL CLEAR (§0) | ✅ still clear |
| Suite | 297 | **301** |

---

## §0 — THE AUTOMATIC-FAIL GATES: ALL CLEAR

| Gate | Result | Evidence |
|---|---|---|
| `GitClaudeUnrealTestCharacter.{h,cpp}` UNTOUCHED (`VIS-R2`) | ✅ **CLEAR** | The ctor still reads `CameraBoom = CreateDefaultSubobject<...>` / `SetupAttachment(RootComponent)` / `TargetArmLength = 400.0f` / `bUsePawnControlRotation = true` and **nothing else** (`GitClaudeUnrealTestCharacter.cpp:39-42`). `bDoCollisionTest`/`ProbeSize`/`ProbeChannel` appear **0×** in it. Both files are OLDER by mtime than all four edited files. Test 7 makes the gate executable **with a positive control**. |
| `Castle.cpp` UNTOUCHED | ✅ **CLEAR** | `Castle.cpp:285` still `CreateDefaultSubobject<UWidgetComponent>`; no `TASK-791` marker anywhere in the file; older by mtime. |
| `DamageNumberActor.cpp` UNTOUCHED | ✅ **CLEAR** | `DamageNumberActor.cpp:29-31` still base `UWidgetComponent` + `SetWidgetSpace(EWidgetSpace::Screen)`; no marker; older by mtime. |
| Still `EWidgetSpace::Screen` (no silent World switch) | ✅ **CLEAR** | `CombatantHealthBarComponent.cpp:55` unchanged, and **test 8 asserts it on the CDO and names `VIS-R1` in its failure text**. ⭐ The World-space option was ARGUED and REFUSED in the handoff, ⛔ not taken — the correct behaviour. |
| Marker census | ✅ | `TASK-790`/`TASK-791`/`bOccludeHealthBar`/`OcclusionChannel` appear in **exactly 6 files**: the 4 declared sources + the 2 new tests. ⛔ Nothing leaked. |
| `LADDER_OUTWARD_SHIFT` never opened (`CONTACT-§14.4`) | ✅ **INTACT** | `Tools/ArtPipeline/build_watchtower.py:137` still `LADDER_OUTWARD_SHIFT = 10.0`, still marked PINNED-by-banner; sockets at `:146-147` are `(-450.0 − SHIFT, 0.0, 0.0)` / `(-150.0 − SHIFT, 0.0, 1200.0)` ⇒ the 10 uu move is on the **X standoff axis** and **exactly 0.000 uu on Y**. ⛔ No revert. |

*(Method note, since this gate has ⛔ no Git: "untouched" is established by (a) marker greps, (b) file mtime ordering, (c) reading the cited lines. All three agree.)*

---

## FINDINGS

### ✅ B-1 [BLOCKER — **RESOLVED at loop 1, see `§L-1`**] `SummonedUnit.cpp:4252` — an unlatched **SECOND WRITER**: every dying unit's health bar comes **BACK** ~150 ms after death

**This is the exact regression `TASK-791`'s §3 says it prevented.** The composition is correct; one of the three shipped owners never feeds it.

- `ASummonedUnit::HPBarWidget` **is** a `UCombatantHealthBarComponent` (`SummonedUnit.cpp:174`) — the 20+ roster this whole feature exists for.
- Its death path calls `HPBarWidget->SetVisibility(false);` **raw** (`SummonedUnit.cpp:4252`) — ⛔ **not** `HideBar()`. `ASummonedUnit` calls `HideBar()` **nowhere** (grep: the only callers are `HeroCharacter.cpp:869`/`:920`).
- ⇒ `bBarShownByOwner` stays **`true`**, so `TickComponent`'s `if (!bBarShownByOwner) return;` **never fires**, the poll keeps running, and on the next fire `ApplyBarVisibility()` — the declared *single writer* — computes `true && !(true && bOccluded)` and calls **`SetVisibility(true)`** on a corpse.
- **It is the ordinary path, not an edge case:** `ShouldHoldDeathAnim()` is `true` for everything except `AMinerUnit` (`MinerUnit.h:438`), `A_<CardID>_Death` clips exist for **every live unit row**, and the hold is up to `DeathAnimMaxHoldSeconds = 2 s` (`SummonedUnit.h:1169`).
- ⇒ **a 0-HP bar pops back over every corpse on the field for up to ~1.85 s.** Before `TASK-791`, `SetVisibility(false)` was terminal — this component had **no** tick-driven visibility writer. ⭐ **The diff created the writer that undoes it.**
- ⛔ **Why the suite was green:** test 2 proves `ComputeDesiredBarVisibility` only subtracts — and that is **true**. The defect was a caller that never set the input.

### ✅ B-2 [BLOCKER — **RESOLVED at loop 1, see `§L-2`**] `CombatantHealthBarComponent.cpp:303` — `bFindInitialOverlaps = false` is **SWEEP-ONLY**; the declared buried-camera fail-open **does not exist** — ⭐ **and `TASK-790` makes the buried camera a DESIGNED, recurring state**

**(a) The flag is never read on a line trace — engine, exact:**
- `CollisionQueryFilterCallback.h:59` — `bDiscardInitialOverlaps = !InQueryParams.bFindInitialOverlaps;`
- `CollisionQueryFilterCallback.cpp:211-220` — `PostFilterImp` opens with `if (!bIsSweep) { return ECollisionQueryHitType::Block; }` and only **then** reaches `else if (bIsOverlap && bDiscardInitialOverlaps)`.
- `SceneQuery.cpp:522` — `FCollisionQueryFilterCallback QueryCallback(Params, Traits::GeometryQuery == ESweepOrRay::Sweep);` ⇒ **`bIsSweep == false` for a raycast.**

**(b) …and the hit it was meant to suppress is really produced:** `Chaos::FConvex::RaycastFast` (`Convex.cpp:50-101`) — start inside the hull ⇒ every plane's signed distance negative ⇒ `EntryTime` stays `0.` ⇒ falls through to `OutTime = 0; OutPosition = StartPoint; return true;`.

**(c) The interaction:** `TASK-790`'s floor places the camera up to `MinCameraArmLengthUU = 150 uu` **inside** the tower at every ladder approach, for ≈2 s, **by design** ⇒ every unit's ray starts inside the same hull ⇒ **the whole roster's bars blink off together**, at the exact moment and place of the defect the pair was fixing. ⛔ Neither diff is wrong alone; together they were.

### ⚠️ W-1 [WARN — **OPEN, accepted, PIE row owed**] `HeroCharacter.cpp:1172-1175` — the push is continuous in the arm but **discontinuous in blocker identity**
`ComputeCameraPushOutLocalX` is continuous in `FixedArm` (at `FixedArm == Floor` it returns exactly 0 — test 1(e)), so there is ⛔ no pop as the arm crosses the floor. ✅ Good design. But `PushOutX` jumps between `−Floor` and `0` **in one frame** whenever the reproduced sweep's blocker identity changes, or the sweep simply **misses**. Up to **150 uu of instantaneous camera translation**. Not a correctness bug; a feel-pass row. 🧑 **`TASK-802` PIE: turn on the spot at the ladder foot and watch for a snap.**

### ⚠️ W-2 [WARN — **OPEN, accepted, no fix wanted**] `HeroCharacter.cpp:1143-1148` — the declared one-frame latency is **mixed-frame**, not uniformly stale
The `TG_PostPhysics` (`SpringArmComponent.cpp:22` ✅ verified) vs `TG_PrePhysics` latency is correctly identified and **declared**. But `ArmOrigin` comes from **this** frame's actor tick while `GetUnfixedCameraPosition()`/`GetSocketLocation()` are **last** frame's post-physics values ⇒ `NaturalArm`/`FixedArm` are distances between points sampled one frame apart (~10 uu at 600 uu/s). Harmless for the *decision*; it means the "lands EXACTLY on the floor" property proved for the **pure function** does not hold exactly at **runtime**, and on a fast turn the identity sweep can return a different blocker or none ⇒ intermittently inert. **Declare it; ⛔ do not fix it** — the alternative is a `USpringArmComponent` subclass, which `VIS-R2` fences out. ⚖️ **The trade is correctly judged.**

### ⚠️ W-3 [WARN — **DEFERRED to TASK-790's next reopen; ruled CORRECT at `§L-4`**] Wrong `file:line` citation, shipped **three times**: `SpringArmComponent.cpp:84-86`
The **fact** is right; the **line numbers** are wrong. The engine defaults are at `SpringArmComponent.cpp:27` (`bDoCollisionTest = true`), `:34` (`ProbeSize = 12.0f`), `:35` (`ProbeChannel = ECC_Camera`). Lines 84-86 are the `bInheritRoll` block. Cited wrongly in `handoffs/TASK-790-programmer.md`, `HeroCharacter.h:1139`, and `SiegeHeroCameraTest.cpp:28`. ⭐ **Every other engine citation in both handoffs is EXACT** — that hit rate is why this one is a WARN and not a shrug.

### ✅ W-4 [WARN — **REPAIRED at loop 1**] `CombatantHealthBarComponent.cpp:63-66` / `.h:179-182` — the `SetTickMode(Enabled)` claim is right for `Disabled`, **over-stated for `Automatic`**
`:1264` keys on **`IsWidgetVisible()`**, which (`:1074-1090`) consults the **component's** `IsVisible()` **only in World space** (`:1077`); in **Screen** space it reads the **inner `UUserWidget`'s** visibility — which this cull never touches. ⇒ under `Automatic` the cull alone would **not** trip `:1264`. The tick mode is still load-bearing (`:1275-1279` under `Disabled`); the comment claimed more than the engine gives.

### ⚠️ W-5 [WARN — **OPEN and PERMANENT; carried into the final verdict**] Both suites are **structurally blind** to the two things that broke
⭐ Both handoffs say so honestly (`SC-§32`), and that honesty is why the blockers were findable. **`TASK-791` has no assertion that touches the trace or the visibility WRITE**; **`TASK-790` has none that touches the runtime tick.** ⇒ every green tick in this gate's scope is about arithmetic, configuration, structure and fences. ⛔ **`TASK-802` must not read a green suite as "V2 and V3 are fixed."**

### NITS (round 0)
- **N-1** *(✅ repaired at loop 1)* `CombatantHealthBarComponent.cpp:277-278` — `Cond ? TObjectPtr<T> : nullptr`. It **compiles** (the templated `operator U*` in `ObjectPtr.h:726-728` is `explicit` + deprecated, so all built-in `?:` candidates route through the single implicit `operator T*` at `:722` and the exact-match candidate wins). ⛔ Not a compile risk, but a construct that leans on `TObjectPtr`'s conversion operators staying as they are.
- **N-2** *(✅ repaired at loop 1)* `ShouldPollOcclusion` is an out-of-line static, so the stated per-frame cost is a real **call** per unit per frame, not literally "one add and one compare".
- **N-3** `HeroCharacter.cpp:1183` — `SetRelativeLocation` written **unconditionally** every frame. **Deliberate** (it is what makes "one write, one exit" true; test 9 asserts it) and negligible. ⛔ Not a change request.
- **N-4** `HeroCameraBaseRelativeLocation` is captured once at `BeginPlay`. ✅ Correct today — nothing else in `Source/` writes the follow camera's relative location. A silent constraint if that ever changes.
- **N-5** *(✅ addressed at loop 1 — better than asked)* `TASK-791` §4's castle recommendation should carry `B-1` forward: `ACastle::HPBarWidget` also writes visibility raw (`Castle.cpp:1251`, `:1447`), so **a future class swap would inherit `B-1`'s exact shape**.

---

## RULINGS DEMANDED BY THE GATE

### §1 ⭐⭐ THE REFUTATION OF THE BOARD'S OWN SUGGESTED FIX — **VERIFIED AT SOURCE, UPHELD**
`SpringArmComponent.cpp:194` is character-for-character `FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());`, constructed as a **local inside `UpdateDesiredArmLocation`**, rebuilt every frame. ⛔ No member ignore array, ⛔ no accessor, ⛔ no setter, ⛔ no hook. The sole extension point is `BlendLocations` (`SpringArmComponent.h:177`, `ENGINE_API virtual`), which requires a subclass; the boom is a `CreateDefaultSubobject` in a base ctor taking ⛔ no `FObjectInitializer` (`GitClaudeUnrealTestCharacter.cpp:39`), so `SetDefaultSubobjectClass` cannot be threaded without editing the file `VIS-R2` fences out. ⇒ **spec §4's suggested fix is ⛔ NOT IMPLEMENTABLE.** ⭐ **It was ARGUED, ⛔ not silently substituted — write-discipline rule 1 working exactly as written.**

### §2 ⭐ THE SCOPE FINDING — **UPHELD**
`AHeroCharacter` carried **zero** camera code before this task. The service is called from `Tick` at `HeroCharacter.cpp:243`, **outside every climb guard**, and test 8 makes the ruling executable **with a control**. VID-004's 01:12.0–01:13.5 is the **APPROACH**; the mount is at 01:17.0 ⇒ ⛔ **a climb-scoped fix would not have covered the frames Jonathan captured.**

### §3 THE MECHANISM — **VERIFIED**
`:197` sweeps every frame; `:201` → `:227-231` `BlendLocations` returns `bHitSomething ? TraceHitLocation : DesiredArmLocation` — ⛔ **no floor, no blend, no damping**. `ArmOrigin = GetComponentLocation() + TargetOffset` (`:130`) is reproduced **verbatim** at `HeroCharacter.cpp:1143`. ✅ **`bDoCollisionTest`/`ProbeSize`/`ProbeChannel` occur 0× in all of `Source/` outside the three TASK-790 files.**

### §4 ⭐ NO STATE TO UNWIND — **UPHELD, and the strongest thing in the 790 diff**
One local from `0.f`, **one** `SetRelativeLocation`, **one** `return;` (test 9, with a control). `EndLadderClimb` (`:1868-1952`) contains **zero** `Camera`/`Boom` tokens on code lines (test 10, with a control). ⇒ ⛔ **nothing ever STARTS ignoring the tower, so nothing can be left ignoring it.**
✅ **Preserved invariants, re-measured:** `SetDefaultMovementMode()` ×**1** (`:1894`) · `SetMovementMode(MOVE_Flying)` ×**1** (`:1830`) · `FSiegeLadderClimbStatics::End(` ×**1** (`:1873`) · `LADDER_OUTWARD_SHIFT` ⛔ never opened.

### §5 ⚠️ THE DECLARED TRADE — **SHAPE SOUND, NEAR-CLIP BOUNDED. ⛔ The number is Jonathan's.**
1. ✅ **Bounded** — `FMath::Min(NaturalArmUU, MinArmLengthUU)` (`cpp:1097`): never more than 150 uu, never past open ground (test 2, with a control).
2. ✅ **Narrow** — only `IsA(AClimbableTower)` (`cpp:1172`). Every other blocker keeps the engine's answer byte-for-byte.
3. ✅ **Continuous at engagement** — exactly 0 at the boundary.
4. ✅ **Reversible two ways with ⛔ no build**, both asserted inert.
5. ⚖️ *"A near-clip beats a blind screen"* is a judgement of the right kind: the failure it accepts is **legible**, and it is flagged for the feel pass in the code.
⛔ Round 0: sound in isolation, **unsound in combination** until `B-2` was repaired. ✅ **`§L-2` clears that.**
**One-frame `TG_PostPhysics`/`TG_PrePhysics` latency: ACCEPTED** (bounded, declared, alternative fenced out) — recorded as `W-2`.

### §7 ⭐⭐ DOES `SetVisibility` HIDE A SCREEN-SPACE WIDGET? — **YES. VERIFIED, LINE-EXACT.**
`UpdateWidgetOnScreen()` (`WidgetComponent.cpp:1308`), gated on `Space == EWidgetSpace::Screen` (`:1310`); `:1317` is character-for-character as cited, **else** → `RemoveWidgetFromScreen()` (`:1329`). Reached every tick via `TickComponent` → `UpdateWidget()` (`:1255`) → `UpdateWidgetOnScreen()` (`:1936`) with ⛔ no intervening early-out under `TickMode::Enabled`. ⭐ **He checked the one thing the whole fix hangs on that nobody had checked, and he was right.**
- ✅ **Red herring CONFIRMED:** `GSlateWorldWidgetIgnoreNotVisibleWidgets` is `static bool … = false;` (`SWorldWidgetScreenLayer.cpp:23`), used at `:141`; `IsWidgetVisible()` consults component visibility **only in World space** (`:1077`).
- ✅ **`SetTickMode(Enabled)` load-bearing CONFIRMED** — with the `W-4` precision correction.

### §8 ⭐ OWNER INTENT AS THE OUTER AND — **VERIFIED IN THE FUNCTION; round 0 falsified in the call graph, ✅ closed at `§L-1`**
`ComputeDesiredBarVisibility = bOwnerWantsBarShown && !(bCullEnabled && bOccluded)` (`cpp:255`) — ✅ can only **subtract**; ⛔ no input shows a bar the owner hid (test 2, exhaustive). ✅ `false` collapses **bit-for-bit**, and `TickComponent` early-outs before accumulate/trace/write, with a one-shot self-heal. ✅ `ApplyBarVisibility()` is deliberately unconditional and always propagates to children.

### §9 COST — **VERIFIED AS STATED**
20 × (1 / 0.15 s) ÷ 60 fps = **2.22 queries/frame** ✅. Per unit per non-poll frame: two bool tests + one call wrapping one add and one compare, ⛔ no trace ✅. Owner-hidden bars: **zero traces for life** ✅.
- ✅ **`ECC_Visibility` IS load-bearing, verified in the ⛔ project, not just the engine:** `BaseEngine.ini:3110` (`Pawn`) and `:3112` (`CharacterMesh`) both carry `(Channel="Visibility",Response=ECR_Ignore)`. ⭐ **And the thing that could have invalidated it here:** this project **re-types** pawn capsules to team object channels (`HeroCharacter.cpp:167`) — **object type only, ⛔ not the response matrix** (`:160`) — so `Visibility = Ignore` survives.
- ✅ **Rejection of `WasRecentlyRendered` — UPHELD** (updated by any view incl. shadow/capture passes; answers the wrong question).
- ✅ **Rejection of distance/frustum — UPHELD** (cannot see a wall).
- ✅ **Reset-to-zero instead of `-= Period` — UPHELD**; test 6 genuinely discriminates.

### §10 ⭐⭐ `bFindInitialOverlaps` AND THE 790↔791 INTERACTION — round 0: judgement CORRECT, instrument INERT → `B-2`. ✅ **Closed at `§L-2`.**
⚖️ **This is the finding that justifies a single gate over two diffs.** Apart, each diff is sound. Together, `790` manufactures the precise world-state that `791` relied on a **sweep-only flag** to survive. ⛔ Neither author could have seen it from inside their own fence.

### §11 THE SIBLINGS — **BOTH RECOMMENDATIONS ADOPTED**
| site | ruling |
|---|---|
| `ACastle::HPBarWidget` (`Castle.cpp:285-289`) | ✅ **ADOPTED — board it, LOW priority.** Base `UWidgetComponent`, anchor `z = 9450`, never reported. ⛔ Not free — class swap + `ApplyTeamVisuals`/`HandleDestroyed`/`ResetCastle` lifecycle. ⭐ Must carry `N-5`: `:1251`/`:1447` write visibility raw, so the swap would inherit `B-1`. ✅ **Loop 1 wrote that trap into the test fixture — better than a comment, because it goes red.** |
| `ADamageNumberActor::NumberWidget` (`DamageNumberActor.cpp:29-31`) | ✅ **ADOPTED — CLOSE WITHOUT A FIX.** Base `UWidgetComponent`, Screen space, ticks per frame (`:27`), sub-second life. A 0.15 s poll on a ~0.8 s actor is a **coin flip, not a rule**; the natural implementation is per-frame, the cost `VIS-§2` refuses by name; and culling feedback the player just **caused** is a net loss. If a manager disagrees it needs a **spawn-time single trace**. |
✅ Both are base `UWidgetComponent`s ⇒ the cull ⛔ cannot leak or be inherited. Neither was touched.

### §12 ⛔ ANTI-VACUITY — **ALL SIX REQUIRED CLAUSES PRESENT; all 18 round-0 rows can genuinely fail**
| required clause | where | can it fail? |
|---|---|---|
| 791: occluded vs clear must **DIFFER** | test 1(c) | ✅ |
| 791: the toggle has **two SIDES** | test 3(b) | ✅ |
| 791: the poll fires **> 0** | test 4(b) + 5(c) | ✅ |
| 790: control vs a **stuck-zero** function | test 3(d) | ✅ |
| 790: CDO equality **PLUS** an independent source probe | test 4(b)+(c)+(d) | ✅ |
| 790: **positive control** on the `VIS-R2` string fence | test 7(b)+(c) | ✅ |
**Every control's target verified to exist on disk**, and all four timing rows re-derived against float behaviour (4(c) lands 60–66 inside [56,70]; 4(d) gap 9–10 vs ≥8; 5(b) exactly 300 vs ≤301 — `2×(1/60)` is **exactly** `1/30` in binary FP, so the boundary is deterministic, ⛔ not flaky; 6(b) 8 quiet frames = 0.133 s < 0.15 s ⇒ exactly 0).
⚠️ ⛔ **The counter-observation:** anti-vacuity was applied rigorously to everything the suites **cover**, and both blockers lived in what they **do not** (`W-5`). ⭐ *Anti-vacuity makes a green tick mean something; it does not make the set of ticks complete.*

### §13 THE SUITE (round 0) — **297** · superseded by **301** at `§L-5`
297 across 23 files; the wider macro grep over all of `Source/` returned the same 297 ⇒ ⛔ no complex/custom macros exist. 297 − 10 − 8 = **279** = the `qa/TASK-779.md` baseline exactly; anchors `17`/`14`/`24` all hold.

---
---

# §L — QA-LOOP 1 RE-REVIEW · **VERDICT: PASS**

**Scope, as dispatched:** the two blockers, the new rows, the changed deltas, and `W-3`/`W-4`/`N-1`/`N-2`. ⛔ **Nothing previously approved was re-opened** — the 150 uu shape, the cost model, `ECC_Visibility`, both sibling rulings and `TASK-790`'s diff stand as ruled.

## §L-1 — B-1 ✅ **RESOLVED**, and the census is **independently confirmed**

**The fix, at the granted line.** `SummonedUnit.cpp:4259` is now `HPBarWidget->HideBar();`, carrying a six-line comment that names *why* the two calls were equivalent before `TASK-791` and are not now. ⭐ **That comment is the deliverable, not the line** — it is what stops the next person "simplifying" it back.

**I re-ran the census rather than accepting it.** `CreateDefaultSubobject<UCombatantHealthBarComponent>` occurs in **exactly three files**: `SummonedUnit.cpp:174` · `Building.cpp:62` · `HeroCharacter.cpp:133`. ✅ Complete.

| owner | writes visibility? | corpse window? | verdict |
|---|---|---|---|
| `ASummonedUnit` | ✅ now `HideBar()` | yes (≤2 s death-anim hold) | ✅ **was the bug, fixed** |
| `ABuilding` | ⛔ **never — the only `HPBarWidget` lines in the file are `:62` create and `:63` attach** | ⛔ **none — `HandleDestroyed()` broadcasts 0 HP at `:363` then `Destroy()` at `:370`** | ✅ **cannot acquire the shape even by accident** |
| `AHeroCharacter` | ✅ already `HideBar()`/`ShowBarIfEnabled()` | **yes — he is HIDDEN, NOT DESTROYED on death** | ⭐⭐ **the proof the latch design was sound** |

⭐⭐ **The hero row is the elegant part and it is genuinely load-bearing, not decoration:** he has a **real** corpse window, so had he written raw he would have shown the identical bug. ⇒ the latch was never a theory that happened to survive — it was **already carrying a live case**, and `ASummonedUnit` was the one owner that had opted out of it. ✅ **Verified: `Building.cpp:370` is `Destroy();`.**

**And it is now asserted, twice, from both sides:**
- **Test 10** (`EveryOwnerHidesTheBarThroughHideBarAndNeverThroughSetVisibility`) — walks all three owner files, asserts `HPBarWidget->SetVisibility` and `->SetHiddenInGame` are **0×** on code lines, with a **per-file positive control** (`HPBarWidget` must appear at all, else `AddError` — so a renamed member or moved file **fails** instead of scanning nothing), plus the **anti-vacuous clause** that the latch route is *live* (`TotalLatchCalls > 0`; I counted 3 on disk: one `HideBar()` in each of `SummonedUnit`/`HeroCharacter`, one `ShowBarIfEnabled()` in `HeroCharacter`). ⭐ **It is also honest about its own reach** — it catches the shape, ⛔ not every alias — which is the correct thing for a source probe to say about itself.
- **Test 11** (`TheOwnerIntentLatchIsWrittenOnlyByShowBarIfEnabledAndHideBar`) — the other half: `bBarShownByOwner =` appears **exactly 2×** in the whole `.cpp`, and each write is located **inside** its entry point by name, with `(c)` asserting `SetVisibility(` appears **exactly once** in the component. ✅ I verified all three counts on disk (2 · 1+1 · 1) — comment lines are correctly skipped by `CountOccurrencesInCode`, and `ApplyBarVisibility`'s `bBarShownByOwner,` argument correctly does **not** match the `bBarShownByOwner =` needle.

✅ **`ACastle` is deliberately ABSENT from the owner list and says so** (`SiegeHealthBarOcclusionTest.cpp:286-289`), naming `Castle.cpp:1251/:1447` as the trap a future class swap would inherit — ⭐ **and noting the probe would start failing the moment the swap lands, which is exactly when someone should be reading it.** That is a better answer than `N-5` asked for.

## §L-2 — B-2 ✅ **RESOLVED**, and the load-bearing engine fact is **independently confirmed at four line-exact points**

⭐⭐ **My blocker rested on the opposite assumption, so I re-derived this from scratch rather than accepting it. The claim holds.**

| link in the chain | verified |
|---|---|
| `HadInitialOverlap` is a **pure distance test with nothing sweep-specific in it** | ✅ `ChaosInterfaceWrapperCore.h:117-120` — `inline bool HadInitialOverlap(const FLocationHit& Hit) { return Hit.Distance <= 0.f; }` (`FLocationHit` is the base of **both** ray and sweep hits) |
| the sweep-only branch is **skipped for a ray** | ✅ `CollisionConversions.cpp:319` `bInitialOverlap = HadInitialOverlap(Hit)`, `:321` `if (bInitialOverlap && Geom)` — **`Geom` is null for a raycast** |
| it is copied into `bStartPenetrating` **unconditionally, on the shared path** | ✅ `CollisionConversions.cpp:366` `OutResult.bStartPenetrating = bInitialOverlap;` |
| the raycast path really runs that conversion | ✅ `CollisionConversions.cpp:552-553` — **`ConvertTraceResults<FHitRaycast>` is explicitly instantiated**, both the array and the single-`FHitResult` forms |
| the first two clauses are the **engine's own rule** | ✅ `HitResult.h:236-239` — `IsValidBlockingHit() { return bBlockingHit && !bStartPenetrating; }`, character-for-character |

⇒ ✅ **`bStartPenetrating` IS set on the raycast path.** The mechanism is real, and `ComputeOcclusionFromTraceResult(bTraceBlocked, bStartPenetrating, Distance) = bTraceBlocked && !bStartPenetrating && HitDistanceUU > 0.f` closes the chain that `B-2` opened.

⭐ **AND I CHECKED THE ONE THING THAT COULD HAVE MADE THE NEW RULE SILENTLY INERT, because it is the same class of trap as `B-2` itself:** the third clause reads `Distance`, and if the engine did **not** populate `Distance` on a ray-single, *every* hit would read `0` and the cull would never hide anything — a totally dead feature with a green suite. ✅ **It is populated:** `SceneQuery.cpp:372-374` gives a non-test **ray** `EHitFlags::Position | Normal | Distance | MTD | FaceIndex`, `CollisionConversions.cpp:313` even `checkSlow`s the flag, and `:371` copies it. ⇒ the second witness is genuinely readable, ⛔ not a constant zero.
⚖️ **And note the direction of its failure mode is the safe one:** if that ever changed, the cull goes **inert** (bars stay visible), ⛔ never "HUD blanks". That matches the component's declared fail-open philosophy, so the second witness cannot become a new roster-wide defect.

**Ruling on the four refuted alternatives — all four refutations UPHELD:**
1. ⛔ **Ignoring the tower DELETES the fix** — ⭐ **and the refutation uses the footage correctly: VID-004 @ 02:08 *is* four bars drawn through that tower's stonework.** Adding the tower to the ignore list makes exactly those bars come back. ✅ **Decisive, and it is the strongest of the four.**
2. ⛔ **Offsetting the trace start** needs a magic number whose only non-arbitrary value lives in a fenced file, and at 150 uu of burial can land still-inside or through the far wall depending on angle. ✅ Correct — and an angle-dependent constant is exactly the kind of tunable that goes stale silently.
3. ⛔ **Reversing the ray relocates the wrong answer** rather than fixing it: a ray ending inside the hull still hits its near face and still reports occluded. ✅ Correct.
4. ⛔ **A sweep it could not show affordable** ⇒ **it loses by the dispatch's own terms** (`VIS-§2` rations this query by name). ✅ Correct — and refusing to spend a budget it could not justify is the right instinct.
⭐ **And the shipped choice is QUERY-KIND AGNOSTIC, which the flag was not.** ⚠️ That asymmetry — silently inert on one query kind, live on the other — **is precisely what made `B-2` a shipped defect rather than a design choice**, so choosing an instrument that cannot have that failure mode is the right lesson drawn, not just the right fix.

**The cost of Single over Test is stated, not glossed, and it is accurate.** ✅ `SceneQuery.cpp:366-368` (Test ⇒ `EHitFlags::None`) and `:392-394` (ray Test ⇒ `PreFilter | AnyHit`; ray Single ⇒ `PreFilter` only) confirm both halves: Test skips the conversion and stops at the first blocking hit; Single must resolve the **nearest** and pays one stack `FHitResult`. Still **one raycast, zero heap allocation, ≈2.2 queries/frame**. ✅ The trade is correctly priced and correctly taken — the boolean form **cannot** answer the question this cull now has to ask.

**Guarded by two new rows:**
- **Test 9** — the rule itself: `(a)` buried ⇒ not occluded · `(b)` ⭐⭐ **anti-vacuous**: a genuine wall at 900 uu **must still occlude** (a blanket fail-open would pass `(a)` perfectly and put VID-004's bars back with a green suite) · `(c)` exhaustive over the unblocked column · `(d)` **each veto clause independently load-bearing** — two rows that go red if *either* clause is deleted · `(e)` ⭐⭐ the **end-to-end** statement across both pure seams: a live unit's bar is **still visible while the camera is buried**, which names the roster-wide blackout by name.
- **Test 12** — defends the *ruling*: `LineTraceSingleByChannel` exactly 1×, `LineTraceTestByChannel` **0×**, `bFindInitialOverlaps` **0×** on code lines (⭐ it stays in the comments deliberately, so nobody re-adds it as protection — and the probe correctly skips comment lines), plus `(d)` that the hit is **actually consulted** (`bStartPenetrating` read, answer routed through `ComputeOcclusionFromTraceResult`). ⭐ **That last pair is the one that matters:** `(a)`–`(c)` are all satisfied by code that runs the expensive query and throws the hit away — *"the exact defect in a more expensive costume"*, as the file itself puts it. All behind a positive control.

## §L-3 — W-4 · N-1 · N-2 ✅ all repaired in-fence
- **W-4** — `.h:230-236` now cites `WidgetComponent.cpp:1275-1279` for the `Disabled` auto-disable and **carries the correction explicitly**: `:1264` keys on `IsWidgetVisible()`, which in Screen space reads the **inner** widget, so `Automatic` alone would not trip it. ✅ Accurate now, and the "do not optimise this tick mode" instruction survives intact — which was the point of the comment.
- **N-1** — both mixed-type ternaries are gone, replaced by **early-outs** (`cpp:296-306`) that read like every other fail-open in the function. ⭐ **And the right call on the alternative:** it declined to adopt `ToRawPtr` — an idiom it could not compile-check under this gate's no-compile fence — in favour of **removing the construct** rather than restating it. ✅ Removing beats restating.
- **N-2** — `cpp:216-220` now names the out-of-line call honestly instead of claiming two instructions. ⭐ *"the honest figure is the one that stays true when someone re-measures it."*

## §L-4 — ⚖️ THE `W-3` DEFERRAL: **CORRECT, and I would have failed the alternative**
All three sites (`handoffs/TASK-790-programmer.md`, `HeroCharacter.h:1139`, `SiegeHeroCameraTest.cpp:28`) sit inside **`TASK-790`'s SOLE fence**, which `TASK-791` does not hold. ⇒ ⛔ **crossing that fence to correct a comment would have been a fence violation traded for a NIT-class defect — strictly the worse deal**, and write-discipline rule 1 says an assignee who believes another task's file is wrong **says so**, it does not self-serve the edit. ⭐ **Flagging and stopping is the behaviour this pipeline is built to reward**, and it is the same instinct that produced the `SpringArmComponent.cpp:194` refutation in the first place.
✅ **The correct lines are recorded here for whoever reopens 790:** `SpringArmComponent.cpp:27` (`bDoCollisionTest = true`) · `:34` (`ProbeSize = 12.0f`) · `:35` (`ProbeChannel = ECC_Camera`). ⛔ **It does not block `TASK-802`** — a wrong line number in a comment cannot break a build, a test or a frame.

## §L-5 — THE SUITE: **301** (computed independently, ⛔ not accepted)
`^IMPLEMENT_SIMPLE_AUTOMATION_TEST` at column 0 across `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp` = **301 across 23 files**.
- `SiegeHealthBarOcclusionTest.cpp` = **12** (was 8; +4 = tests 9, 10, 11, 12) ✅
- `SiegeHeroCameraTest.cpp` = **10** (unchanged) ✅
- **301 − 10 − 12 = 279** = the `qa/TASK-779.md` baseline **exactly** ✅
- Anchors intact: `SiegeLadderClimbTest` **17** · `SiegeClimbableTowerTest` **14** · `SiegeHeroLadderClimbTest` **24** ✅
⇒ ⛔ **`TASK-802` must assert 301. Any other number — including the superseded 297 — is a STOP.**

## §L-6 — ⚠️⚠️ THE LIMIT THIS GATE CARRIES INTO ITS OWN PASS

⭐ **`TASK-791` volunteered this and it is right to, so it goes in the verdict in its own terms:**

> ⛔ **That a real camera in a real tower produces a real zero-distance hit is NOT testable headlessly.** It rests on engine-source reasoning — mine and the programmer's, independently derived and agreeing at five line-exact points — and it is **owed a PIE row**.

⇒ **`W-5` STANDS, UNCHANGED AND PERMANENT: the runtime tick and the visibility write remain uncovered by all 301 tests.** The 22 rows across these two files prove the *arithmetic*, the *configuration path*, the *structure*, the *fences* and the *rules*. They do ⛔ **not** prove that a camera behaves, that a bar hides, or that a corpse stays quiet.

⛔⛔ **`TASK-802` MUST NOT READ A GREEN 301 AS "V2 AND V3 ARE FIXED."** A green suite here means *"nothing we can check headlessly is wrong."* The pixels are the verdict (`SC-§35`, `AS-§6 A(e)`).

---

## NOTES FOR BUILD-MASTER (`TASK-802`)

- **Suite total = 301.** ⛔ A red suite, or any other total, is a **STOP**.
- ⛔⛔ **PARSE THE BUILD LOG FOR `Result: Failed` — ⛔ never trust `$LASTEXITCODE`** (standing Build.bat law).
- ⚠️ First compile for **both** diffs — same UBT module, one compile, this is it. Nothing in either diff has ever been built.
- ⭐ **THE FIVE PIE ROWS THIS GATE OWES YOU. ⛔ Nothing in the 301 observes any of them:**
  1. **V2 pixels** — walk the hero to the ladder foot and **watch**. Is the world visible through the ≈2 s approach? Is the 150 uu near-clip into stone acceptable, or does Jonathan want the number moved? 🧑 **His call, ⛔ not ours.**
  2. **`W-1`** — turn on the spot at the ladder foot; look for a **snap** as the traced blocker flips tower→ground.
  3. ⭐⭐ **`B-2`, the decisive one (`§L-6`)** — with units on the field, walk in until the camera pushes into the tower, and watch **every other unit's health bar**. ✅ **They must stay up.** If the roster's bars vanish together, the zero-distance fail-open is not working in-engine and the engine-source reasoning was wrong.
  4. **`B-1`** — kill one rigged unit in view and watch its bar for ~2 s. ✅ **It must stay gone.** A 0-HP bar reappearing over the corpse means the latch is still being bypassed somewhere the source probe cannot see.
  5. **V3 pixels** — stand on the watchtower deck with units on the ground below and confirm the four floating bars from **VID-004 @ 02:08.0** are gone.
- ⚖️ Rows 3 and 4 are **regression checks on repairs this gate forced**; rows 1, 2 and 5 are the original acceptance. ⭐ **Report all five by name in the `TASK-802` handoff, including the ones that pass.**
