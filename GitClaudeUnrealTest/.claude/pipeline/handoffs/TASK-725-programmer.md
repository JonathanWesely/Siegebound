# TASK-725 — [TOWER-1] NAV FEASIBILITY SPIKE — ruling

**Verdict: ✅ GO — WITH ONE REFUTATION THAT CHANGES TASK-727'S SPEC.**

A ranged unit **can** path up onto a 1,200 uu platform on a runtime-spawned straight ramp under this
project's real navmesh settings. **But the ruled 40° slope is REFUTED** — at the shipped
`CellSize 32` / `CellHeight 20`, Recast's ledge filter destroys a 40° ramp. **The ramp must be ≤ 32°,
and the spec is 30°** (run ≈ 2,078 uu, not ≈ 1,430 uu).

Everything below is measured at `Config/DefaultEngine.ini`, `Engine/Config/BaseEngine.ini` and the
UE 5.8 engine source. **No editor was opened and none was needed** — see §6 for how the serialized-
level trap was neutralised by arithmetic instead of a PIE session.

---

## 1. The five config numbers — verified at source

The manager's spec asserts the four agent figures from a **comment** at `DefaultEngine.ini:287-290`.
A comment is not a value, so I read the actual defaults. The project sets **no** `SupportedAgents`
and **no** `[/Script/NavigationSystem.NavigationSystemV1]` section at all, so the engine defaults are
genuinely live.

| # | Figure | Claimed | Measured | Source | Verdict |
|---|---|---|---|---|---|
| 1 | `AgentMaxSlope` | 44° | **44.0** | `BaseEngine.ini:3062` | ✅ CONFIRMED |
| 2 | `AgentMaxStepHeight` | 35 uu | **35.0** | `BaseEngine.ini:3061` + project `:340` | ✅ CONFIRMED |
| 3 | `AgentRadius` | 34 | **34.0** | `BaseEngine.ini:3058` | ✅ CONFIRMED |
| 4 | `AgentHeight` | 144 | **144.0** | `BaseEngine.ini:3059` | ✅ CONFIRMED |
| 5 | `CellSize` / `CellHeight` | 32 / 20 | **32.0 / 20.0** | `DefaultEngine.ini:340` | ✅ CONFIRMED |

Also confirmed: `RuntimeGeneration=Dynamic` (`:281`), `TileSizeUU=2000` (`:339`),
`bDoFullyAsyncNavDataGathering=False` (`:343`), `MaxSimultaneousTileGenerationJobsCount=8` (`:344`).

⚠️ **All five numbers are right. The manager's arithmetic is also right. The CONCLUSION drawn from
them is wrong, because the binding constraint is a sixth number nobody computed** — see §2.

---

## 2. ⭐⭐ THE HEADLINE FINDING — 44° IS NOT THE SLOPE CEILING. **32.0° IS.**

The 44° `AgentMaxSlope` only decides which *triangles* get marked walkable
(`rcMarkWalkableTriangles`, `RecastNavMeshGenerator.cpp:2854`). It is **not** what kills a ramp here.
`rcFilterLedgeSpans` runs **unconditionally** afterwards (`RecastNavMeshGenerator.cpp:3183` — not
behind `bPerformVoxelFiltering`, which gates a different UE-only filter), in mode
`RC_SLOPE_FILTER_RECAST` (`RecastNavMesh.cpp:544`, CDO default, no project override).

Its second clause, `RecastFilter.cpp` (`rcFilterLedgeSpansImp`):

```cpp
else if (neighborSlopeFilterMode == RC_SLOPE_FILTER_RECAST && (asmax - asmin) > walkableClimb)
{
    s->data.area = RC_NULL_AREA;
}
```

`asmax - asmin` is the **sum of the uphill and downhill voxel steps** at a cell. So a ramp cell
survives only when `d_up + d_down <= walkableClimb`.

- `walkableClimb = FMath::CeilToInt(AgentMaxStepHeight / CellHeight)` = `ceil(35/20)` = **2 voxels**
  (`RecastNavMeshGenerator.cpp:5280`, and identically `:5305`).
- Rise per cell at slope θ = `CellSize * tan(θ)` = `32 * tan(θ)` uu, i.e. `32*tan(θ)/20` voxels.
- Span tops are `ceil()`-quantised, so if rise-per-cell exceeds **1 voxel**, some cells get a step of
  2 next to a step of 1 ⇒ `d_up + d_down = 3 > 2` ⇒ **the cell is nulled.**

⇒ **Ramp survives iff `32 * tan(θ) <= 20`, i.e. θ ≤ atan(0.625) = 32.005°.**

| Slope | Rise/cell | Voxels | Ledge clause B | Result |
|---|---|---|---|---|
| **40°** (ruled) | 26.85 uu | **1.343** | steps of 1 and 2 ⇒ sums of **3 > 2** | ⛔ **SHREDDED — 2 of every 3 cells nulled** |
| 32° | 19.996 uu | 0.9998 | exactly at the boundary | ⚠️ knife-edge, refused |
| **30° (SPEC)** | 18.48 uu | **0.924** | all steps ≤ 1 ⇒ sums ≤ **2** | ✅ **GENERATES** |
| 28° | 17.01 uu | 0.851 | sums ≤ 2 | ✅ generates, more margin |

**Why this has never bitten this project before:** at *engine-default* cells (19/10),
`walkableClimb = ceil(35/10) = 4` and the ceiling is atan(2*10/19) = 46.5°, so the 44° slope cap
binds and 40° is fine. **TASK-217's coarsening to 32/20 for the 10× arena silently dropped the real
slope ceiling from ~46° to 32°.** That is precisely the class of thing this spike existed to catch.

### ⭐ The shipped hills independently confirm 32°
`CONVENTIONS.md:167` — the climbable-geometry law is **every hill face ≤ 30°**, and the three
shipped meshes measure **27.54° / 27.18° / 27.48°** (`handoffs/TASK-139-artist.md:28-30`). Units
demonstrably climb these. **This project has zero shipped evidence of a walkable surface steeper
than 30°, and the arithmetic says >32° breaks.** The hill law picked ≤30° for a *stated* reason
(44.76° `WalkableFloorAngle` / 44° `AgentMaxSlope`) that turns out to be the wrong reason for the
right number. 30° is the angle this project already knows how to build and has hit three times.

⚠️ The engine even ships a validator for this at `RecastNavMeshGenerator.cpp:5309-5332`. For 44° it
computes `RequiredClimbVx = ceil(32*tan(44°)/20) = 2` vs `WalkableClimbVx = 2` and stays silent —
it checks only the *worst-case* slope, so it does not warn about the mid-range failure at 40°.

---

## 3. GO / NO-GO on the straight ramp — **GO**, and the geometry for TASK-727

Every number below is a build target, not a range to interpret.

| Property | **Value** | Derivation |
|---|---|---|
| **Ramp slope** | **30.0°** | ≤32.005° ledge ceiling (§2), 7.6% margin; matches the shipped hill law |
| **Platform height (rise)** | **1,200 uu** | `T-5`, `PlatformHeightUU` |
| **Ramp horizontal run** | **2,078 uu** | `1200 / tan(30°)` = 2078.46 |
| **Ramp sloped face length** | **2,400 uu** | `1200 / sin(30°)` |
| **Ramp deck width** | **300 uu** | 128 uu lost to ledge+erosion ⇒ 172 uu (5.4 cells) of surviving corridor |
| **Vertical clearance over walking surface** | **≥ 200 uu** | 160 nav + 20 quantisation, and the real capsule is 176 tall |
| **Platform size** | **600 × 600 uu** | leaves 472×472 walkable after erosion — 6+ bodies |
| **Ramp→platform junction** | **FLUSH, co-planar, no lip** | any step > 40 uu (2 voxels) severs the connection |

### Width — why 300, not 200
`walkableRadius = ceil(34/32) = 2` voxels (`RecastNavMeshGenerator.cpp:5260`), and
`rcErodeWalkableArea` nulls cells with chamfer `dist < radius*2 = 4` (`RecastArea.cpp:177-179`).
The ramp's outermost cell column is nulled first by ledge clause A (`minh < -walkableClimb` — the
drop off the open side), then erosion takes one more column. **2 cells = 64 uu lost per side,
128 uu total.**

- W = 200 ⇒ **72 uu** corridor (2.25 cells) — survives, but one cell of grid-phase noise takes it
  to ~1 cell. The manager's ≥200 is a **valid floor, refined — not refuted**.
- W = 300 ⇒ **172 uu** corridor (5.4 cells). ✅ **This is the spec.**
- Hard floor for any corridor at all: **160 uu**.

⚠️ **A parapet must be built OUTSIDE the 300 uu deck, never carved out of it** — a solid rail seeds
erosion from its own face and would cost another 2 cells per side. Open sides are fine and preferred.

### Clearance — 200 is right, for a reason the spec did not state
`walkableHeight = ceil(144/20) = 8` voxels = 160 uu (`:5279`). But **`ASummonedUnit`'s real capsule
is 176 uu tall** (engine `ACharacter` default 34 r / 88 half-height — the project never calls
`InitCapsuleSize`; `SiegeSpawnConstants.h:9` names 88 as the fallback). **The nav agent height of
144 is SHORTER than the actual unit**, so Recast would happily generate nav under a 165 uu ceiling
that a unit physically cannot walk through. 200 uu clears the nav requirement *and* the real body.

### Solid wedge, not a floating plank
`ABuilding` does **not** set `bFillCollisionUnderneathForNavmesh` (contrast
`BattlefieldScatter.cpp:982`, which does for hills). If the ramp is a thin floating deck, Recast
generates ground spans underneath it, and where the deck's underside is within 160 uu of the ground
`rcFilterWalkableLowHeightSpans` (`:3188`) nulls the ground — punching a **ring of missing ground
nav around the ramp's low end**. **Build the ramp as a solid wedge/embankment resting on the
ground.** That removes the ground span entirely and the artifact with it.

---

## 4. The two refused shapes — conclusions CONFIRMED, stated mechanisms CORRECTED

**⛔ STAIRS — REFUSED (conclusion stands, reason restated).**
The claim *"a 35 cm riser will not generate nav"* is **not accurate**. `walkableClimb` is 2 voxels
= **40 uu**, and `rcFilterLowHangingWalkableObstacles` (`:3178`) is exactly the pass that re-marks a
riser's top as walkable — a 35 uu riser most likely *would* generate. Stairs are refused on **cost
and fragility**: risers must stay strictly under `AgentMaxStepHeight = 35`, so 1,200 uu of rise
needs **35+ steps**, each with a tread deep enough to survive 64 uu/side erosion (≈130 uu), giving a
run of **≈4,550 uu** — 2.2× the ramp, for a far more expensive mesh. And at `CellHeight 20` a 34 uu
riser is 1.7 voxels, so quantisation makes each step a coin-flip. **Refused. Do not build stairs.**

**⛔ SPIRAL — REFUSED (conclusion stands, reason corrected).**
The span-merge concern is overstated: Recast heightfields are multi-span by design, and turns only
need the same ≥200 uu clearance the ramp already needs. The **real** disqualifier is geometric: on a
helix the **inner edge is steeper than the centreline**. To fit a 300 uu deck the outer radius must
be ≥ ~350 uu, and at 30° centreline the inner edge exceeds the 32° ceiling and gets shredded by the
same clause B — invisibly, on the inside of the curve only. **Refused for this ship.**

---

## 5. ⭐ "Does a runtime-spawned building generate walkable nav, or only carve?"

**It generates. Carving and walking are the same mechanism with different slopes.**

There is **no code path anywhere in the generator that distinguishes a building from a hill.** Both
register in the same nav octree and go through the same rasteriser:

- `ABuilding::VisualMesh` — `SetCollisionProfileName(BlockAll)` (`Building.cpp:47`) +
  `SetCanEverAffectNavigation(true)` (`Building.cpp:55`)
- scatter hill HISM — `QueryOnly`, blocks `ECC_Pawn` + both team channels,
  `SetCanEverAffectNavigation(true)` (`BattlefieldScatter.cpp:970-983`)

A wall "carves" **because its faces are 90°**, which exceeds `AgentMaxSlope 44` ⇒ `RC_NULL_AREA` ⇒
obstacle. A 30° face is under 44° ⇒ `RC_WALKABLE_AREA` ⇒ walkable surface. Same pass, same rules.
The shipped hills are the existence proof that runtime-spawned geometry carries walkable nav; the
only setup difference is `bFillCollisionUnderneathForNavmesh`, handled in §3.

**⇒ A building-spawned ramp takes the same route. This is the claim the whole no-rig path rested on,
and it holds.**

---

## 6. The serialized-`L_Arena` trap — NEUTRALISED WITHOUT THE EDITOR

The ini warns at `:291-294` / `:324-331` that the placed `RecastNavMesh` actor carries its own
copies. I checked whether the engine reconciles them: it does, at `RecastNavMesh.cpp:963-1015`
(forces `CellSize`/`CellHeight`/`AgentMaxStepHeight`/`AgentMaxSlope` back to the CDO) — **but that
block is gated on `IsVoxelCacheEnabled()`, which returns `DefOb->bUseVoxelCache`, and this project
never enables it.** So the safety net does **not** run and the serialized values **do** win.

⇒ **I cannot determine from files alone which cell size the shipped level runs.** Rather than force
the editor open, I made the question irrelevant by checking the spec against **both** possibilities:

| | ini values (32/20) | stale engine defaults (19/10) |
|---|---|---|
| `walkableClimb` | 2 vx (40 uu) | 4 vx (40 uu) |
| **30° ramp** | rise 0.924 vx ⇒ sums ≤ 2 ✅ | rise 1.097 vx ⇒ sums ≤ 3, cap 4 ✅ |
| **40° ramp** | rise 1.343 vx ⇒ sums to 3 > 2 ⛔ | rise 1.594 vx ⇒ sums ≤ 4 ✅ |
| 300 uu width | 172 uu corridor ✅ | 224 uu corridor ✅ |
| 200 uu clearance | needs 160 ✅ | needs 150 ✅ |

**30° / 300 / 200 / 600×600 generates under BOTH configurations. 40° is a coin-flip on an
unresolved question.** That asymmetry is on its own sufficient reason to refuse 40°, and it is why
this ruling needs no PIE session.

---

## 7. What can still fail at runtime, with correct geometry

1. ⚠️ **Nav rebuild latency on spawn.** `RuntimeGeneration=Dynamic` rebuilds only dirtied tiles. The
   tower footprint (~2,700 × 300 uu) at `TileSizeUU=2000` touches **~2–4 tiles**, not the 326 the
   scatter dirties — so this is nothing like the measured ~216 s scatter drain (`:310-316`). But it
   is **not instant**: for a short window after placement the ramp carries no nav and a unit ordered
   up will path to the old surface. Naturally hidden in play; no fix needed, but 731 should watch it.
2. ⚠️ **Units standing where the tower spawns.** `BlockAll` geometry materialises around them; UE
   does not push characters out. **Pre-existing behaviour for every building**, but amplified here
   because the footprint is ~10× a wall's. Worth a placement sweep in 726 — flagged, not specced.
3. ⚠️ **`bCanWalkOffLedges` is engine-default `true`** (never set anywhere in Siegebound) and
   `bUseRVOAvoidance` is `false`, so units physically jostle. **Units can be shoved off the ramp and
   off the 1,200 uu platform.** They survive (§8), but expect it to look untidy. A parapet is the
   art fix — outside the 300 uu deck, per §3.
4. ⚠️ **Pathing back DOWN is free.** Recast polys are undirected; if the surface generates, it
   generates for both directions. Ledge clause A is a generation-time test, not a travel-direction
   test. ✅ No extra work.
5. ⚠️ **The junction is the fragile seam.** A lip > 40 uu at ramp→platform, or a UCX hull that
   swallows the ramp into a solid box, produces a tower where every property readback is correct and
   nothing can climb it. This is the castle-floor defect class (35→61 hulls), and it is TASK-727's
   single biggest risk. **The ramp deck must be its own collision surface.**

---

## 8. ⭐ `T-4` — occupants fall and survive: **FREE. No work needed.**

- `ASummonedUnit : public ACharacter` (`SummonedUnit.h:117-118`) with a real
  `UCharacterMovementComponent`; `GravityScale` untouched.
- **There is no fall damage anywhere in the project.** `Landed(`, `OnLanded`, `NotifyHit`,
  `FallDamage`, `LandingVelocity`, `MOVE_Falling` — **zero hits** across all of
  `Source/GitClaudeUnrealTest/Siegebound`. Every hit in the repo is in untouched Epic template
  variants that are not ancestors of `ASummonedUnit`.
- On `Destroy()` the mesh unregisters from the octree, the floor vanishes, CMC drops to
  `MOVE_Falling`, the unit lands and resumes `MOVE_Walking`. **Zero damage, no death.**
- The stuck watchdog does not interfere during the fall: `Evaluate` re-anchors whenever speed
  exceeds `MinSpeedSq = 2500` (50 uu/s), and a falling unit is far above that.

⚠️ **One caveat worth 726's attention, and it is not a blocker.** Units land wherever gravity puts
them. If a unit lands somewhere with no navmesh poly under its feet, **nothing in this project
recovers it**: the `NAV-§` ladder never teleports and never nav-projects — its rungs are
sidestep → widen/repath → `EnterIdle()` (`SummonedUnit.cpp:3065-3175`), and
`SiegeStuckStatics.h:237-241` states the sidestep point is deliberately *not* nav-projected. So the
claim *"the `NAV-§` stuck watchdog already covers a bad landing"* (TASK-726 spec §5) is **too
strong** — the ladder covers a unit that is stuck *on* the navmesh, not one stranded *off* it.

In practice the landing zone is the ground directly under a tower that was standing on that same
ground, which is navmesh-covered, so this is a low-likelihood edge. **T-4 needs no code.** But 726
should not cite the watchdog as the guarantee, because it is not one.

---

## Files touched

- **This handoff only.** ⛔ No code, no mesh, no `.uasset`, no editor, no MCP, no compile, no Git.

## Read (read-only)
`Config/DefaultEngine.ini:276-344` · `Engine/Config/BaseEngine.ini:3038-3066` ·
`Engine/.../NavMesh/RecastNavMesh.{h,cpp}` · `Engine/.../NavMesh/RecastNavMeshGenerator.cpp` ·
`Engine/.../Recast/RecastFilter.cpp` · `Engine/.../Recast/RecastArea.cpp` ·
`Siegebound/Building.{h,cpp}` · `Siegebound/BattlefieldScatter.cpp` · `Siegebound/SummonedUnit.{h,cpp}` ·
`Siegebound/SiegeStuckStatics.h` · `Siegebound/SiegeSpawnConstants.h` · `CONVENTIONS.md:156-179` ·
`handoffs/TASK-139-artist.md`

## What QA should scrutinise

1. **§2 is the whole ruling — audit the arithmetic, not the prose.** `ceil(35/20) = 2`;
   `32*tan(40°) = 26.85 uu = 1.343 vx`; ceil-quantised steps of 1 and 2 summing to 3 > 2. If that
   chain is wrong, 40° is back and TASK-727's spec changes again.
2. **Confirm `LedgeSlopeFilterMode` really is `Recast`** (`RecastNavMesh.cpp:544`, no project ini
   override). Under `UseStepHeightFromAgentMaxSlope` the cap becomes `maxStepFor2CellsVx = 4` and 40°
   would pass. ⛔ **I deliberately did NOT recommend flipping that mode** — it is a global generation
   change that would re-bake every hill, the castle floor and the whole arena, to buy a steeper
   ramp nobody asked for. Naming it as the rejected alternative, per `SC-§15`.
3. **The width derivation** (§3): 2 cells lost per side = 128 uu. Check the erode threshold reading
   (`RecastArea.cpp:177-179`, `thr = radius*2`) — I claim exactly one column beyond the ledge column.
4. **§8's caveat** contradicts TASK-726's spec §5 on the watchdog. That is a deliberate, cited
   disagreement, not an oversight.
