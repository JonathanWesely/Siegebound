# TASK-941 — THE CLIMB RE-MEASUREMENT AT `SetRelativeScale3D(1, 1, n)`, `n ∈ {1..5}`

**Author:** gameplay-programmer · **Date:** 2026-09-03 · **Mode:** ⛔ DIAGNOSE-ONLY.
⛔ Zero `Source/**` writes · ⛔ zero test edits · ⛔ no compile · ⛔ no editor / MCP (the editor stayed
`TASK-946`'s) · ⛔ no Git writes · ⛔ no `.uasset` opened. The only file written is this one.

---

## ⭐⭐ THE ANSWER, IN ONE LINE

> ### **THE LICENSED CEILING IS `n = 2`.**
> **`AClimbableTower::MaxStackHeightMultiplier = 2`.** ⭐ **×2 does not merely survive — the STANDOFF
> *improves* by +9.06 uu and the non-swept window *halves*.** ⛔ **×3, ×4 and ×5 fail, and they fail on a
> term nobody had named: the deck slab scales with the mesh while the capsule does not, so the
> deck-breach window stops being able to cross the slab.** ⛔ **The break-even is `n ≤ 2.2`, and it is
> exact: `40n ≤ CapsuleHalfHeight`.**

**`STACK-§8` cl. 4 outcome measured = row 2, *"some hold (e.g. ×2 only)"*** — quoted from the clause, not
re-scoped (`SC-§49`). ⇒ `CanStackHeight() → true` on `AClimbableTower` with a **per-class ceiling of 2**.
⛔ **The RULING is the manager's. The NUMBER is `2`.**

---

## 0. ⛔⛔ THE `n = 1` POSITIVE CONTROL — **BLOCKING, AND IT PASSES** (`SC-§39`, spec item (3))

⛔ I did not report a single scaled figure before this reconciled. **7 controls, 6 exact, 1 declared miss
on a non-binding hull.** The instrument is a closed-form spine→convex-solid sweep over the *whole* line,
built from `Tools/ArtPipeline/build_watchtower.py`'s `HULL_SPEC` + massing constants — **the same source
the shipped FBX is generated from**, not a re-typed copy.

| # | control | published | **mine at `n = 1`** | |
|---|---|---|---|---|
| C1 | climb-line length | `1236.93169` | **`1236.93169`** | ✅ exact |
| C2 | lean | `75.96376°` (law's rounded **76.0°**) | **`75.96376°`** | ✅ exact |
| C3 | whole-line worst spine→hull, `t` and hull | `103.32015` @ `t = 246.43`, `_00` (TASK-783) | **`103.32018` @ `t = 246.42`, `00 plinth`** | ✅ **Δ 0.00003** |
| C4 | the manager's single-feature reconstruction at the **`t = 245` grid sample** | `103.330` | **`103.3300`** | ✅ exact |
| C5 | 2nd-worst hull, hero clearance | `80.72` (`_06` fill wedge) | **`84.60`** (`06`, spine `126.6036`) | ⛔ **MISS, +3.88 — see §6** |
| C6 | worst render dressing, hero | `88.0` (`course_lo_W…`) | **`88.00`** (spine **exactly `130.0` = `BAY_HALF_Y`**) | ✅ exact |
| C7 | rung plane depth / pitch | `−22.0` / `40.0` | **`22.00000` / `40.00000`**, slab half-thk **`10.00000`** ⇒ near `−12.000` far `−32.000` | ✅ exact |
| C8 | `§8.5a` cl. 2 window, unit | `264 uu Z ⇒ 272.1 of line = 22.0%` | **`264.0 Z ⇒ 272.12 of line = 22.00%`** | ✅ exact |
| C9 | `§8.5a` cl. 2 window, hero | `3 × 96 of Z ⇒ 296.86 of line` | **`296.86`** | ✅ exact |
| C10 | hull-06 vertex count (model validation) | `[8,8,8,8,8,8,**6**,8]` (TASK-783 §2) | my wedge is a **6-vertex triangular prism** | ✅ shape confirmed |

⭐ **C3 + C4 together are the strong control**: they reproduce *both* the artist's continuous minimum
*and* the manager's grid-sample reconstruction, from one instrument, at the same contact feature.

---

## 1. ⭐ THE TABLE — `TOWER-§8.3`'s THREE NUMBERS, PER `n`

`VisualMesh->SetRelativeScale3D(1, 1, n)`. ⭐ **`VisualMesh` IS the root component** (`Building.cpp:37-38`,
`SetRootComponent(VisualMesh)`) ⇒ this sets the **ACTOR** transform's Z scale. Sockets scale with it;
`LadderFoot.Z = 0` so **the foot does not move at any `n`**.

| `n` | **(a) CLIMB LINE** `Δ` / length / lean | **(b) STANDOFF** `dist(spine, geom)` — ⛔ **BLOCKING** | unit clr / hero clr | **(c) RUNG DEPTH** ⭐ | **(c) RUNG PITCH** ⚠️ | window margin (unit) | verdict |
|---|---|---|---|---|---|---|---|
| **1** | `(300, 0, 1200)` · `1236.93169` · `75.96376°` | **`103.32018`** ✅ ≥ 98.0 | `69.320` / `61.320` ✅ | **`22.000`** | `40.00` | **`+48.00` Z** | ✅ ships today |
| **2** | `(300, 0, 2400)` · `2418.67732` · `82.87498°` | ⭐ **`112.37547`** ✅ **+9.06 vs `n=1`** | `78.376` / `70.376` ✅ | **`22.502`** (+2.3 %) | `78.22` | **`+8.00` Z** | ⭐ **PASS** |
| **3** | `(300, 0, 3600)` · `3612.47837` · `85.23636°` | `115.10104` ✅ ≥ 98.0 | `81.101` / `73.101` ✅ | `22.599` (+2.7 %) | `116.82` | ⛔ **`−32.00` Z** | ⛔ **FAIL** |
| **4** | `(300, 0, 4800)` · `4809.36586` · `86.42367°` | `116.39788` ✅ ≥ 98.0 | `82.398` / `74.398` ✅ | `22.633` (+2.9 %) | `155.53` | ⛔ **`−72.00` Z** | ⛔ **FAIL** |
| **5** | `(300, 0, 6000)` · `6007.49532` · `87.13759°` | `117.15365` ✅ ≥ 98.0 | `83.154` / `75.154` ✅ | `22.649` (+3.0 %) | `194.27` | ⛔ **`−112.00` Z** | ⛔ **FAIL** |

⛔ **READ THE TABLE CAREFULLY: `n = 3..5` FAIL, AND THEY DO ⛔ NOT FAIL ON THE STANDOFF.** The standoff
column is **green at every `n`**. The thing that kills them is the last column, and it is a term neither
`STACK-§2` nor `TOWER-§8.5a` had named.

---

## 2. ⛔⛔ THE TWO HAZARDS, **SEPARATED** (`STACK-§8` cl. 4's mandatory split)

### 2(i) ⛔ THE STANDOFF — **BLOCKING** — ⭐ **IT IMPROVES. MONOTONICALLY. AT EVERY `n`.**

**⛔ `STACK-§2`'s *"a non-uniform scale preserves nothing"* is MEASURED FALSE for the standoff.** I did not
inherit it and I do not cite it (`SC-§49`); the manager flagged it as analytic and asked for a number.
Here is the number, and the direction it moved in:

```
n :        1          2          3          4          5        -> limit
spine:  103.320    112.375    115.101    116.398    117.154   ->  120.0
```

⭐ **WHY, IN ONE SENTENCE, SO IT IS CHECKABLE RATHER THAN ASSERTED:** the binding contact is the plinth's
**top-west edge** (`UCX_SM_WatchTower_00`, X/Y ±300, Z `[0, 160n]`) — **and the plinth top and the climb
line's Z scale *together*, so the line's X where it passes the plinth's top height is `n`-INVARIANT at
`X = −420`** (fraction `s = 160n / 1200n = 2/15`, always). ⇒ the horizontal gap to the face at `X = −300`
is a fixed **120 uu**, and the only `n`-dependent term is the capsule spine's 54 uu drop, whose *vertical*
contribution **shrinks** as the line steepens. ⇒ the distance rises from 103.32 toward 120.

**Per-hull, whole-line minima (deck slab `_07` EXCLUDED exactly as `TOWER-§8.3`'s box requires):**

| hull | `n=1` | `n=2` | `n=3` | `n=4` | `n=5` |
|---|---|---|---|---|---|
| **`00` plinth — ⛔ BINDING at every `n`** | **103.320** | **112.375** | **115.101** | **116.398** | **117.154** |
| `01`/`02` keep walls ±Y | 196.000 | 196.000 | 196.000 | 196.000 | 196.000 |
| `03` keep wall +X | 356.000 | 356.948 | 360.251 | 361.919 | 362.847 |
| `04`/`05` west bay piers | 130.000 | 130.000 | 130.000 | 130.000 | 130.000 |
| `06` interior fill wedge | 126.604 | 136.190 | 139.018 | 140.351 | 141.124 |
| *(render dressing, ⛔ no collision)* | 130.000 | 130.000 | 130.000 | 130.000 | 130.000 |

⭐ `04`/`05` and the west dressing are pinned at **exactly `130.0` = `BAY_HALF_Y`** at every `n` — the west
bay is gapped in **Y**, and **a Z scale cannot touch Y**. They never become binding, because the plinth
asymptotes to 120 < 130.

**Also measured, because `TOWER-§8.5a` cl. 6 drives a *lifted* path, not the `§8.3` line:**

| line | `n=1` | `n=2` | `n=3` | `n=4` | `n=5` |
|---|---|---|---|---|---|
| `§8.3` line (the gate's own, **the conservative one**) | 103.320 | 112.375 | 115.101 | 116.398 | 117.154 |
| cl. 6 driven path, unit (`+88` Z) | 124.663 | 123.291 | 122.409 | 121.887 | 121.548 |
| cl. 6 driven path, hero (`+96` Z) | 126.604 | 124.283 | 123.073 | 122.386 | 121.948 |

⇒ ⭐ **the `§8.3` line is the worst of the three at every `n`, so the gate's own number is the acceptance,
and it clears `≥ 98.0` with margin `+5.32 → +19.15`.**

⚠️ **ONE DECLARED DIVERGENCE OF INTERPRETATION FROM `TASK-783`, stated rather than reconciled away:** its
*"clause-6 LIFTED hero driven line"* row uses a **`+8` Z** lift (hero-minus-unit) and lands on `105.2604`.
**I reproduce that number exactly at `+8`** (`105.261`) — so we agree arithmetically. But cl. 6 lifts
**both endpoints by the pawn's FULL `GetScaledCapsuleHalfHeight()`**, so the physically driven paths are
`+88` and `+96`, which is what my table reports. ⛔ **The disagreement is about which line is "the lifted
line", ⛔ not about any measurement, and it cannot change the verdict: all three lines clear the gate at
all five `n`.**

⇒ ⭐⭐ **`TOWER-§8.5a`'s licence is NOT voided by a Z stack at any `n ∈ {1..5}`. At `n = 2` it is
STRENGTHENED.**

### 2(ii) ⚠️ THE RUNG PLANE — **DEPTH SURVIVES · PITCH STRETCHES · REPORTED SEPARATELY**

⭐ **THE `−22.0 uu` DEPTH — THE THING THE HANDS GRIP — SURVIVES.** ⛔ Not asserted: I took the *authored*
rung centres (`climb_point(t) − W_AXIS · LADDER_OFF`, `LADDER_OFF = 22.0`), applied `diag(1,1,n)`, and
re-measured the perpendicular offset **against the NEW climb line**.

| `n` | mid-plane depth | vs `−22.0` | slab half-thickness | near / far face (law: `−12` / `−32`) |
|---|---|---|---|---|
| 1 | **22.000** | — | 10.000 | `−12.000` / `−32.000` |
| 2 | **22.502** | **+2.28 %** | 10.228 | `−12.274` / `−32.730` |
| 3 | 22.599 | +2.72 % | 10.272 | `−12.327` / `−32.871` |
| 4 | 22.633 | +2.88 % | 10.288 | `−12.345` / `−32.921` |
| 5 | 22.649 | +2.95 % | 10.295 | `−12.354` / `−32.944` |

Closed form (matches the numeric sweep to 5 dp): `depth(n) = 22 · n / sqrt(Ux² + n²Uz²)`, bounded above by
`22 / Uz = 22.677`. ⇒ ⭐ **the depth can never move more than +3.08 %, at ANY `n`, and the slab's near/far
faces move with it, so the climber stays outboard of the stiles exactly as authored.** ⛔ **`§8.3`'s
mesh↔clip binding does NOT fire on depth, and `A_SiegeBiped_Climb` needs NO re-export on depth grounds.**

⚠️ **THE PITCH STRETCHES, AND IT IS COSMETIC** (`STACK-§8` cl. 4(ii)): `pitch(n) = 40 · sqrt(Ux² + n²Uz²)`
⇒ `40.00 → 78.22 → 116.82 → 155.53 → 194.27`. The rung **count is unchanged (31)** and the pitch as a
**fraction of the line is invariant** — the ladder is stretched, not re-rungged. At `n = 2` the hands
cycle at `350 / 78.22 = 4.47 Hz` against `8.75 Hz` of authored rung passage ⇒ **the hands grip between
rungs about half the time.** ⛔ **This does NOT void anything**: the depth is preserved, so the hands still
land in the ladder slab. ⛔ **Reporting this as a blocker would refuse a shippable feature** — it is a
visual-fidelity note for 🧑 Jonathan's eye, not a gate.

---

## 3. ⛔⛔⭐⭐ THE THING THAT ACTUALLY SETS THE CEILING — **AND NO EXISTING LAW CLAUSE NAMES IT**

> ### **THE DECK SLAB SCALES WITH THE MESH. THE CAPSULE DOES NOT. THE DECK-BREACH WINDOW IS SIZED IN CAPSULES.**

`FSiegeLadderClimbStatics::Begin` (`SiegeLadderClimbStatics.cpp`) sizes the non-swept window as
`BreachZUU = DeckBreachCapsuleHalfHeights (3) × HalfHeight` — **264 uu of Z for the unit, 288 for the
hero — at EVERY `n`**, because it is a property of the *pawn*. Meanwhile `UCX_SM_WatchTower_07`'s slab is
`Z [1160, 1200]` **× n** ⇒ **40 n uu thick.**

`DeckBreachCapsuleHalfHeights`' own doc comment already declared the exposure and nobody connected it to
scaling: *"an allowance for how far the slab hangs BELOW its own surface. ⚠️ This file cannot read the
mesh, so it is an ASSUMPTION — and it is **2.2× the ~40 uu the mesh actually ships**."*
⇒ ⛔⛔ **THAT `2.2×` OF HEADROOM IS EXACTLY WHAT A Z STACK SPENDS, AND IT IS SPENT LINEARLY IN `n`.**

**THE ARITHMETIC, WRITTEN OUT** (ascent; `HH` = the climber's capsule half-height):

```
window OPENS at capsule-centre Z   =  (1200n + HH) − 3·HH   =  1200n − 2·HH
sweep JAMS   at capsule-centre Z   =  1160n − HH            (capsule TOP meets the slab underside)

need OPEN <= JAM :   1200n − 2·HH  <=  1160n − HH   <=>   40n <= HH   <=>   n <= HH / 40
                     unit HH = 88  ->  n <= 2.2          hero HH = 96  ->  n <= 2.4
```

⇒ ⛔ **THE BINDING CASE IS THE UNIT (`n ≤ 2.2`), NOT THE HERO.** ⭐ *That inverts `TOWER-§8.5a`'s existing
intuition, where the hero's fatter capsule was the hazard — here the hero's TALLER capsule buys it more
window, so the AI unit is the one that fails first.*

| `n` | slab | window (unit) | opens @ `Zc` | jams @ `Zc` | **margin** | outcome |
|---|---|---|---|---|---|---|
| 1 | 40 | 264 Z / 272.12 line / **22.00 %** | 1024 | 1072 | **+48.00** | ✅ |
| **2** | **80** | 264 Z / 266.05 line / **11.00 %** | 2224 | 2232 | ⭐ **+8.00** | ✅ **PASS** |
| 3 | 120 | 264 Z / 264.92 line / 7.33 % | 3424 | 3392 | ⛔ **−32.00** | ⛔ stall |
| 4 | 160 | 264 Z / 264.52 line / 5.50 % | 4624 | 4552 | ⛔ **−72.00** | ⛔ stall |
| 5 | 200 | 264 Z / 264.33 line / 4.40 % | 5824 | 5712 | ⛔ **−112.00** | ⛔ stall |

**⛔ WHAT `n ≥ 3` ACTUALLY LOOKS LIKE IN PLAY:** the swept capsule's top meets the slab underside **32 uu
of Z BEFORE `ShouldSweep` would turn sweeping off**. It cannot advance, the window never opens, and the
unit hangs in `MOVE_Flying` against the deck's underside until the watchdog (`4 × length / 350` = 41.3 s
at `n=3`) drops it from ~3,392 uu. ⭐ **This is `TOWER-§8.5a` clause 7's LOUD failure — the watchdog warns
and drops rather than hanging forever — which is the one piece of good news: it is not silent.** ⛔ But it
is still a **tower whose deck cannot be reached**, i.e. exactly the `STACK-§8` cl. 2 outcome the ban
exists to prevent.

**⭐ WHY `n = 2`'s `+8 uu` MARGIN IS ROBUST AND NOT KNIFE-EDGE — reasoned from the shipped predicate:**
`ShouldSweep` turns sweeping **off at 2224**, which the ascending capsule reaches **before** 2232, so the
jam point is never reached with sweeping on. A frame long enough to step past both in one swept move
(`> 8.06 uu` of line ⇒ **below ~43 fps**) is simply *blocked at 2232* by the sweep — and 2232 ≥ 2224, so
the very next frame's `ShouldSweep` returns false and the non-swept drive resumes. ⇒ **worst case is a
one-frame hitch, ⛔ not a stall.** ⚠️ **Declared: this is an argument from the shipped `ShouldSweep`
expression, ⛔ not an observed PIE result** — see §7.

⚠️ **AND THE LAW GAP, NAMED FOR THE MANAGER:** `TOWER-§8.5a`'s voiding condition is **two-sided** — (i) a
`SM_WatchTower` re-author, (ii) a new pawn class. ⛔ **A RUNTIME TRANSFORM IS A THIRD TRIGGER AND NEITHER
SIDE COVERS IT.** The mesh is untouched and the pawn set is untouched, yet the licence's *inputs* move.
⇒ if `TASK-942` ships, `§8.5a` owes a **side (iii)**.

---

## 4. ⛔⛔ SPEC ITEM (5) — **THE `BeginPlay` SOCKET RULING, IN WRITING**

> ### ⭐⭐ **RULING: THE HAZARD AS STATED IS ⛔ REFUTED. THE CLIMB LINE IS ⛔ NOT CACHED IN WORLD SPACE, AND IT TRACKS A RUNTIME SCALE ⛔ LIVE, ON BOTH ENTRY PATHS.**

⛔ Answered **by symbol, from the source** (`SC-§38`), in five measured steps:

1. **`ConfigureLadderLink()` is called from exactly ONE site** — `AClimbableTower::BeginPlay`
   (`ClimbableTower.cpp:202`). **`SetLinkData` is called from exactly ONE site** —
   `ConfigureLadderLink` (`:428`). *Census: `grep -rn "ConfigureLadderLink\|SetLinkData" Source/` →
   2 non-comment hits + 1 declaration + 1 test reference.* ⇒ **the spec's premise is TRUE.**
2. ⭐ **But what is stored is ⛔ NOT the world line.** `ResolveLadderSocketRelative` returns
   `Mesh->GetSocketTransform(SocketName, RTS_Actor).GetLocation()` (`:375`) — **actor space**, which
   *divides the actor transform (scale included) straight back out*. `VisualMesh` **is** the root
   (`Building.cpp:37-38`), so `RTS_Actor` yields the raw authored mesh-local socket. ⇒
   `LinkRelativeStart/End = (−460,0,0) / (−160,0,1200)`, **scale-free by construction** — and it would be
   scale-free even if `BeginPlay` ran on an already-scaled tower.
3. ⭐⭐ **The world points are recomputed on EVERY read, from the LIVE owner transform.** UE 5.8,
   `NavLinkCustomComponent.cpp:518-525`:
   ```cpp
   FVector UNavLinkCustomComponent::GetStartPoint() const
   { return GetOwner()->GetTransform().TransformPosition(LinkRelativeStart); }
   ```
   `FTransform::TransformPosition` applies **Scale3D** before rotation ⇒ at scale `(1,1,n)` the end point
   is `ActorLoc + Rot · (−160, 0, 1200n)`. **The line stretches with the tower, with zero code changes.**
4. **Every consumer reads it live. Census of `GetStartPoint`/`GetEndPoint` in `ClimbableTower.cpp` — 4
   sites, ⛔ ZERO caches:** `:504-505` (the nav-link entry path), `:781-782` (the contact/hero entry
   path), `:592` (K-C's re-arm latch). ⭐ **Negative control:** the only `FVector` members on
   `AClimbableTower` are the two `static const` degrade-open fallbacks
   (`LadderFootDefaultRelative` / `LadderTopDefaultRelative`) — **there is no `CachedFoot`, no
   `CachedTop`, no cached line of any kind.**
5. **The fallback path is safe too:** the literals are stored as *relative* endpoints, so they get the
   same live owner transform applied ⇒ a tower with missing sockets scales identically.

⇒ ⛔ **A `n`-scaled tower does ⛔ NOT strand a climber at the old deck height.** ⭐ **Because the endpoints
are stored RELATIVE and resolved LIVE, the runtime scale is handled by construction — `TOWER-§8.4(A)`'s
`RTS_Actor` decision, made for the spawn-squash, pays for this for free.**

### ⚠️⚠️ BUT — THE ADJACENT HAZARD I FOUND WHILE RULING ON THAT ONE, AND IT IS **NOT** REFUTED

**`UNavLinkCustomComponent` is a `UActorComponent`, ⛔ NOT a `USceneComponent`** (the class's own comment
at `ClimbableTower.cpp:184-188` says so). ⇒ when the **root scene component** is rescaled,
`USceneComponent::PropagateTransformUpdate` calls `UpdateNavigationData()` **for that scene component
only** (`SceneComponent.cpp:1038-1040` → `UNavigationSystemV1::UpdateComponentInNavOctree`). **Nothing
re-gathers the LINK component's octree element**, whose baked off-mesh connection was captured via
`GetNavigationData` → `ProcessNavLinkAndAppend(&Data.Modifiers, GetOwner(), NavLinks)` using the owner
transform **at gather time**. `UpdateNavigationBounds()` / `RefreshNavigationModifiers()` fire only from
`SetLinkData` (`NavLinkCustomComponent.cpp:330-332`) and register/enable paths.

⇒ ⚠️ **PREDICTED, ⛔ NOT MEASURED: after a runtime stack upgrade the tower's *collision* navmesh
regenerates at the new height, while the *registered off-mesh connection* may remain at the old
`Z = 1200`. The link's endpoint would then sit ~1,200 uu below the new deck poly and Recast could drop
the connection ⇒ ⛔ AI units stop being handed a path to the ladder, while the HERO's contact climb keeps
working (it reads `GetStartPoint()` live and never consults the navmesh).**

⭐ **Falsifiable in one line and cheap to close:** re-calling `ConfigureLadderLink()` after a successful
`ApplyStackUpgrade` re-reads the same scale-free relatives and calls `SetLinkData` → `UpdateNavigationBounds`
+ `RefreshNavigationModifiers`. ⛔ **I am reporting this, ⛔ not prescribing it** — it is `TASK-942`'s
call, and it must not be shipped without the observation in §7 row 1.

---

## 5. ⛔ TWO MORE COUPLINGS I MEASURED WHILE I WAS IN THE FILES — **riders for `TASK-942`, neither a blocker**

**(a) ⚠️ `USiegeMeshJuiceComponent` can silently UN-STACK a tower — LATENT TODAY, ⛔ NOT LIVE.**
`SetTargetMesh` snapshots `BaseScale = InMesh->GetRelativeScale3D()` (`SiegeMeshJuiceComponent.cpp:24`)
and the squash's terminal branch writes **`TargetMesh->SetRelativeScale3D(BaseScale)` VERBATIM** (`:95`).
⭐ **Measured safe as shipped:** `SetTargetMesh` + `PlaySpawnSquash` are called **once**, at
`ABuilding::BeginPlay` (`Building.cpp:91-94`); the scale channel runs **only while `bSquashActive`**, set
**only** by `PlaySpawnSquash`; and `AClimbableTower : public ABuilding` (`ClimbableTower.h:219`) —
⛔ **not `ATower`** — so `PlayRecoil` (`Tower.cpp:192`, the only other trigger, and a *location* channel
anyway) can never reach it. ⇒ ⛔ **no defect today.** ⚠️ **The trap: a stack upgrade is exactly the moment
someone would want a squash.** Any future `PlaySpawnSquash()` on a stacked building **resets the mesh to
the ×1 baseline and un-stacks the tower, height and climb line together, with every readback correct.**

**(b) ⚠️ `AClimbableTower::PlatformHeightUU = 1200.f` becomes a LIE at `n > 1`.** It is a hardcoded member
(`ClimbableTower.h:595`) that does not track the mesh. **Census of `GetPlatformHeightUU`: 1 declaration +
3 references, ⛔ ALL THREE IN `Tests/SiegeClimbableTowerTest.cpp` (`:827`, `:1054`, `:1327`), ⛔ ZERO
non-test consumers** ⇒ **no gameplay reads it, so nothing is wrong today** and the tests read the **CDO**,
which a runtime instance scale cannot touch. ⚠️ It is a NIT with teeth only if a future `HIGH-§` consumer
binds to it.

---

## 6. ⛔ WHAT I COULD **NOT** MEASURE — declared, with a named owner (spec item (6))

`STACK-§8` cl. 4 row 4 says a declared, reasoned *"I cannot compute this"* is complete and passing. **Four
items, none of which moves the ceiling:**

| # | what | why I cannot | who can | does it move `n = 2`? |
|---|---|---|---|---|
| **1** | ⛔⛔ **`BP_Building_WatchTower`'s authored `VisualMesh` relative Z scale.** My whole table assumes it is **`1.0`** — i.e. `AuthoredHeightScaleZ == 1.0`, so the world rise is `1200 n`. | ⛔ **The row forbids me opening a `.uasset`.** | **build-master (MCP readback)** or **art-director** | ⚠️ **YES if it is not 1.0** — every Z figure scales by it. ⭐ **Corroborated but ⛔ not measured:** `PlatformHeightUU = 1200.f`, `LadderTopDefaultRelative.Z = 1200.f` and `build_watchtower.py`'s `RISE = 1200.0` all agree on 1200, which is consistent with an authored 1.0 — **that is a CITATION, not a measurement** (`SC-§40` cl. 1). ⛔ **`TASK-942` must not ship until this is read back.** |
| **2** | ⛔ **`_06` fill wedge, `n=1` hero clearance:** TASK-783 says `80.72`; I get **`84.60`** (spine `126.604`). ⛔ I cannot explain the `+3.88`. My model is corroborated by the **6-vertex** count TASK-783 itself published for `_06`, and by hull `_00` agreeing to 5 dp with the same instrument. | ⛔ I measure the **authoring script**; TASK-783 measured the **exported FBX** by BVH. | **art-director** (re-run the `_06` probe) | ⛔ **NO.** `_06` is **never binding** — it clears the plinth by **23.3 → 24.0 uu** at every `n`. Even taking TASK-783's `122.72`, it stays 19.4 uu above the binding hull. |
| **3** | ⚠️ **The nav-link re-registration in §4** — whether Recast actually drops or keeps the off-mesh connection after a runtime rescale. | ⛔ Requires PIE + a live navmesh; ⛔ the editor is `TASK-946`'s and I was fenced from MCP. | **build-master (PIE)** | ⛔ **Not the ceiling** — it is a *shippability* condition on `TASK-942`, and the hero path is unaffected either way. |
| **4** | ⚠️ **The `n=2` `+8 uu` window margin under a real frame budget.** §3's self-recovery argument is derived from `ShouldSweep`'s expression, ⛔ not observed. | ⛔ Headless; no PIE. | **build-master (PIE)** | ⛔ **No** — the argument shows the failure degrades to a one-frame hitch, and `§8.5a` cl. 7's watchdog covers the residual **loudly**. |

⛔ **Nothing above is a silence and nothing above is an assumed number.**

---

## 7. ⭐ THE LICENCE — and the three observations that should ride with it

**`STACK-§8` cl. 4 outcome = row 2 (*"some hold"*). ⛔ CEILING = `2`.** ⇒ `AClimbableTower` sets
`MaxStackHeightMultiplier = 2` in its constructor. Under the shipped series
`min(1 + UpgradeCount, MaxMultiplier)` that is **exactly one upgrade: ×1 → ×2, then the cap holds** — and
`J-6`'s cap behaviour already paints the ghost **BLUE** at the ceiling and surfaces the HUD note, so the
partial is graceful with zero new UI.

⭐ **What `n = 2` actually buys, measured, not hoped:** a **2,400 uu** tower whose standoff is **9.06 uu
BETTER** than the one shipping today, whose non-swept stretch is **halved** (22.00 % → 11.00 %, i.e. more
of the line is swept, which is strictly safer against `§8.5`'s named harms), whose foot and deck navmesh
margins are **bit-identical** (both are X/Y facts and a Z scale cannot touch them: foot clears the eroded
carve by 96 uu, top sits 76 uu inside the deck poly, **at every `n`**), and whose watchdog scales with the
line (14.14 s → 27.64 s against a 6.91 s ascent) because it was **derived, never a literal**.

**⛔ THE THREE THINGS THAT SHOULD RIDE ON `TASK-942`, in priority order:**
1. ⛔ **Read back `BP_Building_WatchTower`'s authored `VisualMesh` Z scale before shipping** (§6 row 1).
2. ⚠️ **Decide the nav-link re-arm** (§4) — the AI ascent path may need `ConfigureLadderLink()` re-called
   after `ApplyStackUpgrade`; the hero path does not.
3. ⚠️ **`§8.5a` owes a side (iii)** to its voiding condition: *a runtime transform on the tower.*
4. ⚠️ **Note the juice-component trap** (§5a) beside `ApplyStackUpgrade` so a future "upgrade squash"
   cannot silently un-stack the tower.

⛔ **The ruling is the manager's. The number is `2`, and it is measured.**

---

## Files read (⛔ none modified)

- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.{h,cpp}` · `Building.{h,cpp}` ·
  `SiegeLadderClimbStatics.{h,cpp}` · `SiegeMeshJuiceComponent.cpp` · `Tower.cpp` (base-class check) ·
  `Tests/SiegeClimbableTowerTest.cpp` (reference census only)
- `Tools/ArtPipeline/build_watchtower.py` — **the geometry source: `HULL_SPEC`, the massing constants,
  `fill_faces`, `decoration_boxes`, the socket literals**
- `.claude/pipeline/CONVENTIONS.md` — `STACK-§8` · `STACK-§2` · `STACK-§5` · `STACK-§7` · `STACK-§9` ·
  `TOWER-§8.3` · `TOWER-§8.4` · `TOWER-§8.5` · `TOWER-§8.5a` · `SC-§38`/`§39`/`§40`/`§49`
- `.claude/pipeline/handoffs/STACK-BUGS-diagnosis.md` · `TASK-783-artist.md` · `TASK-737-artist.md`
- `.claude/pipeline/footage/VID-005-tower-stack-refused-and-witch-card-actor-unavailable.md`
- UE 5.8 engine source (read-only): `NavigationSystem/Private/NavLinkCustomComponent.cpp` ·
  `NavigationSystem/Private/NavigationSystem.cpp` · `Engine/Private/Components/SceneComponent.cpp`
- Instrument: `<scratchpad>/task941_measure.py` (⛔ scratch, ⛔ not in the repo) — closed-form
  spine→convex-solid sweep, coarse scan + 4 bracketed refinements per solid.
