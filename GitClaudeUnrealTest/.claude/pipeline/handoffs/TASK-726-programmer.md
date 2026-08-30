# TASK-726 — [TOWER-2] `AClimbableTower` — delivered

**Built to TASK-725's GO-WITH-REFUTATION ruling, not to the board's original GO shape.** Two of the
board's own statements are contradicted below and both are flagged, not quietly worked around.

## Files touched (3 — all NEW, ⛔ nothing existing modified)

- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h`
- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp`

⛔ `Tower.{h,cpp}` · ⛔ `Building.{h,cpp}` · ⛔ `SummonedUnit.{h,cpp}` (TASK-724's, live) ·
⛔ `SiegeNavAreas.{h,cpp}` · ⛔ `cards.csv` · ⛔ `WarMapWidget.cpp` · ⛔ `Tools/Packaging/` ·
⛔ `.Build.cs` — **all untouched.** ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.
**Airlock clean:** ⛔ no `Capture()` / `EnsureSnapshot()`, Zone A unread, 🔒 the 552 latch unspent,
⛔ no token figure anywhere (`AS-§12g`). **M8:** ⛔ no new replicated property, ⛔ no RPC, ⛔ no
class-tier change — it sits at `ABuilding`'s tier on its shipped team/HP replication.

## The shape

`AClimbableTower : public ABuilding` — a **direct** child, a **sibling** of `ATower` (`TOWER-§5`).

| Member | Kind | What it is |
|---|---|---|
| `PlatformHeightUU = 1200.f` | `EditDefaultsOnly` | 🧑 T-5. Comment carries the `HIGH-§` consequence (**×1.787 / +78.7%** flat, **×2.44** on a tall hill) **and** that it is `SM_WatchTower`'s contract — 725's 2,078-uu run is arithmetic from it |
| `AscentGateVolume` | `UBoxComponent` | the team gate, physical lane — `ACastle::GateBlockerVolume` verbatim |
| `AscentGateFloorUU = 300.f` | `EditAnywhere` | gate bottom; 300 − 176 (capsule top) = **124 uu** of clearance over ground traffic |
| `AscentGateHeadroomUU = 400.f` | `EditAnywhere` | gate top = `PlatformHeightUU + this` |
| `AscentGateHalfExtentXY = (1500, 500)` | `EditAnywhere` | ⚠️ **the one unverified number — see finding 1** |
| `AscentBlockedChannel(ETeamId)` | `static`, pure | ⭐ **the single source of truth** — the gate's one `ECR_Block` and the test's prediction both come from here, so they cannot drift |
| `CanTeamAscend(ETeamId, ETeamId)` | `static`, pure | T-3 as a testable predicate. ⛔ No capacity term, ⛔ no unit-type term — because there are none in the rule |
| `GetPlatformHeightUU()` | `BlueprintPure` | lets the test read the shipped default off the CDO |

⛔ **No `OnStatsLoaded` override** — the class has no fire path to arm, belt-and-braces over the
row's `Cadence 0`. ⛔ No tick, ⛔ no timer, ⛔ no target acquisition, ⛔ no projectile.
⛔ **No `#include` of `SummonedUnit.h` or `Tower.h` in either shipped file**, and the `.cpp` says so
in a comment at the include list.

### The gate, and why its SHAPE differs from the castle's — **DECLARED DEVIATION (`SC-§15`)**

The **mechanism** is the shipped one, unmodified: `ECC_SiegeTeamBlue`/`ECC_SiegeTeamRed` from
`SiegeNavAreas.h`, object type = own channel, Ignore-all base, one `ECR_Block` on the enemy channel,
`QueryAndPhysics` last. ⛔ No new channel, ⛔ no new area class, ⛔ no bespoke filter.

The **shape** is an **elevation shell**, not a doorway box, and that is deliberate: `SM_WatchTower`
does not exist yet (TASK-727), so nobody can say where the ramp mouth sits in mesh-local space — a
doorway box would be a guess. The shell spans the footprint horizontally and runs from **300 uu** up
past the platform, so a ground-walking enemy (capsule top **176 uu**) passes **under it untouched**,
while an enemy that starts up the ramp is stopped the moment it gains real height — **wherever the
ramp turns out to be.** At 30° it climbs ≈520 uu of run first, which reads as "turned back on the
ramp". ⭐ **It fails safe both ways:** too small ⇒ an enemy climbs (a missed rule, ⛔ never a stuck
unit); too large ⇒ it only forbids enemy bodies from an air column over ground the tower's own solid
body occupies.

### ⛔⛔ THE CASTLE'S SECOND LANE IS REFUSED — **DECLARED DEVIATION (`SC-§15`)**

`TOWER-§4` cites the castle's team gating, which has **two** lanes. I shipped the physical lane and
**refused the `UNavModifierComponent` / `UNavArea_*CastleInterior` / `UNavFilter_Team*` lane**, on a
checkable mechanism:

1. ⚠️ **It would make the tower UNATTACKABLE.** The excluded area covers the whole ~2,700-uu
   footprint, so an enemy melee unit's path would **end at the footprint boundary** — up to ~1,300 uu
   from the tower body, against a melee reach of ~150 uu. **A destructible 250-HP building enemy
   melee can never reach is a worse bug than the one the lane prevents.**
2. ⭐ **The pile-up it exists to prevent cannot occur here.** The castle needs it because the enemy
   castle **is a path goal**, so paths genuinely route inside. This platform is a **dead end** — its
   only nav connection is back down the ramp — so no route through it is ever shorter and Recast
   never chooses it. The asymmetry is structural, not a judgement call.
3. ⚠️ It would punch a team-excluded hole in the battlefield around a **runtime-placed** building,
   stranding any enemy already standing there — manufacturing the `NAV-§` failure this design avoids.

⇒ The physical lane alone fully delivers T-3, and the navmesh (and therefore enemy pathing **to** the
tower) stays exactly as `ABuilding` already ships it. Both refusals are written into the header at
`ConfigureAscentGate`'s doc, ⛔ not only here.

## The four runtime risks TASK-725 named

| # | Risk | Verdict |
|---|---|---|
| 1 | **Nav rebuild latency on spawn** (~2–4 tiles, not instant) | ⚠️ **DECLARED, ⛔ not fixed.** No code can make `RuntimeGeneration=Dynamic` instant, and the window is naturally hidden (the ramp is unwalkable for a beat after placement). ⛔ Adding a spawn-time nav-force would be a new mechanism for an invisible symptom. **Watch item for TASK-731/732.** |
| 2 | **Units standing where the tower spawns** (pre-existing, amplified ~10×) | ⚠️ **DECLARED, ⛔ not fixed.** This is every building's shipped behaviour; `UCharacterMovementComponent` already depenetrates a capsule that geometry materialises around. A placement sweep would be the **first code in this project that moves a unit it does not own** — exactly what `NAV-§` refuses — and would need a direction choice nobody has ruled. ⭐ **The real lever is the PLACEMENT rule, not this class:** the footprint is ~10× a wall's and `SiegePlayerController`'s validation was never sized for it. **Raised for the manager, ⛔ not boarded here (out of fence).** |
| 3 | **`bCanWalkOffLedges` is engine-default `true`** | ✅ **RULED: LEAVE IT `true`. See below.** |
| 4 | **Pathing back down is free** | ✅ Confirmed, no work. Recast polys are undirected; ledge clause A is a generation-time test, not a travel-direction test. Cited in the header. |

### ⚖️ Ruling on `bCanWalkOffLedges` — **leave engine-default `true`, and it is DESIRABLE**

1. ⛔ **It is not this class's property to set.** It lives on each unit's
   `UCharacterMovementComponent`; the tower holds no unit reference and acquiring one to flip a
   movement flag would be the coupling `TOWER-§` exists to avoid.
2. ⛔ **The only place to set it is `SummonedUnit` — fenced out, and it is GLOBAL.** It would change
   behaviour on every hill, every castle rampart and every ledge in the arena, to fix an aesthetic on
   one building.
3. ⭐ **It is the pressure-release valve.** `bUseRVOAvoidance` is `false`, so units jostle. A platform
   units could not be shoved off, with no capacity counter and no eviction, is a **trap**. Being
   walk-off-able is what guarantees no unit is ever permanently stranded up there — and `T-4` already
   proves falling is free.
4. The art fix, if he wants one, is a **parapet built OUTSIDE the 300-uu deck** (TASK-725 §3 —
   carved out of it, a rail seeds erosion and costs 2 more cells per side).

### Off-mesh recovery path — **NOT needed, and I am NOT citing the watchdog**

⭐ **TASK-725's correction is respected: the board's spec §5 claim that "the `NAV-§` stuck watchdog
already covers a bad landing" is FALSE and I have not relied on it.** The ladder never teleports and
never nav-projects; its terminal rung is `EnterIdle()` (`SiegeStuckStatics.h:237-241`), so it covers
stuck-**on**-navmesh, ⛔ not stranded-**off**-it. The header says so explicitly.

**No recovery path ships, and here is the honest residual rather than a claim of coverage:** a tower
dies, occupants fall, and they land inside a footprint whose navmesh has **not yet regenerated** —
for that window they are off-mesh and nothing in this project deterministically recovers them. It is
low-likelihood (the landing zone is ground that was navmesh before the tower and becomes navmesh
again within the dynamic-rebuild window, and `UNavigationSystemV1` projects a move request's start
point to the nearest poly), and **the fix would be worse than the bug**: a nav-projecting teleport is
precisely the mechanism `NAV-§` deliberately refuses. ⇒ **Declared as a residual for TASK-732, ⛔ not
silently assumed away.**

## Tests — `Tests/SiegeClimbableTowerTest.cpp`, **5 new** ⇒ suite **165 → 170**

*(165 = the batch's 156 baseline + TASK-724's 9, counted in the working tree.)*
New file under the "extend, never a parallel new frame unless none fits" law — **declared "none
fits"**: no shipped test file covers `ABuilding`, `ATower` or any structure. Same frame as the suite
(`EditorContext | EngineFilter`, `Siegebound.*`). ⛔ Zero world, ⛔ zero PIE, ⛔ zero asset loads.

⭐ **Every test carries a SELF-CHECK, because yesterday's QA loop was an assertion whose two sides
were equal by construction.** What each would catch:

| # | Test | What it catches | Its self-check (what stops it passing vacuously) |
|---|---|---|---|
| 1 | `IsADirectABuildingChildAndNeverAnATowerSubclass` | a reparent to `ATower` (⇒ inherited auto-fire loop); an intermediate class inserted; the reverse reparent | asserts **`ATower` IS an `ABuilding`** first — otherwise "not a child of `ATower`" would also pass if `ATower` were an unrelated class |
| 2 | `CarriesNoneOfATowersFireLoopStateAndNeverTicks` | any of `ATower`'s 7 `Attack*` fire-loop properties appearing on this class (`FindPropertyByName` walks supers, so it also proves `ABuilding` has none); a tick sneaking in | asserts **each probe name still resolves on `ATower`** — a rename would otherwise blind all 7 probes and pass silently (`SHIP-§9`) |
| 3 | `RefusesEnemyClimbersAndAdmitsEveryOwnTeamUnit` | a one-way gate (only one team tested); the gate blocking the **own** channel; a capacity limit appearing | asserts the two channels are **distinct** (collapsed aliases would lock everyone out yet stay self-consistent) **and** pins which channel each team blocks (catches "consistent but backwards") |
| 4 | `DeclaresNoCapacityCounterNoRangedFilterAndNoOccupantBookkeeping` | a member named for capacity / occupancy / a ranged-only filter / a fall special case | ⚠️ **the load-bearing one** — asserts the reflection walk found ≥4 members **including `PlatformHeightUU` and `AscentGateVolume` by name.** A scan over an empty list passes any banned-token check, permanently and vacuously |
| 5 | `PlatformAndAnEqualHillYieldTheIdenticalDamageMultiplier` | ⭐ **the parity ask** — plus a `PlatformHeightUU` retune away from the 1,200 uu whose consequence is quoted to Jonathan; a `HIGH-§` retune; **and any tower-named member appearing on `ASummonedUnit`** | three: (i) the multiplier **rises** with height before it is used to compare heights (a constant function would satisfy parity perfectly); (ii) a hill exactly one step lower yields exactly `-0.10`, so the equality is not degenerate; (iii) the `ASummonedUnit` walk found `HeightBonusStepUU` |

⭐ **On test 5's parity specifically — how it avoids the `X == X` trap.** The two heights come from
**genuinely different sources**: the left from `GetDefault<AClimbableTower>()->GetPlatformHeightUU()`
(the class under test), the right from a literal `1200.f` typed as `HIGH-§5`'s table row and
**deliberately not derived from the left**. The expected multiplier `1.78740157` is pinned as a
**literal**, ⛔ not recomputed from the tuning, so re-deriving the formula in the test could never
make the claim agree with itself. The `HIGH-§` tuning is **read off the CDO** (not transcribed), with
a hard error if the properties are gone — so a retune fails here loudly instead of quietly changing
what the file proves.

## ⚠️ Findings QA and the orchestrator should act on

1. ⚠️⚠️ **`AscentGateHalfExtentXY` assumes the ramp runs along the mesh's LOCAL X.** `SM_WatchTower`
   does not exist yet, so the axis is unverifiable today. **If the ramp runs along local Y, swap the
   two numbers.** ⇒ **TASK-728 must verify the axis against the real mesh** (it already verifies ramp
   collision at its step (4) — this rides there), and TASK-731 confirms at integration. Getting it
   wrong **fails open** (an enemy climbs the far end), ⛔ never into a stuck unit. `EditAnywhere` for
   exactly this fix; the header flags it in bold.
2. ⚠️ **The board's spec §5 is wrong about the `NAV-§` watchdog** (see above). TASK-725 §8 caught it;
   this task did not build on it. **The law text `TOWER-§4`'s "occupants when it dies" cell carries
   the same false claim** — the manager should correct `CONVENTIONS.md`, ⛔ which I did not touch.
3. ⚠️ **Placement validation is sized for a wall, not a ~2,700-uu tower** (risk 2 above). Out of this
   task's fence — **raised for the manager**, not boarded here.
4. 📌 **This test file rides TASK-724's seam** (`ASummonedUnit::HeightAdvantageMultiplier`,
   `HeightBonusStepUU`, `HeightBonusPerStep`). Verified present in the working tree at
   `SummonedUnit.h:644/923/936` and defined at `SummonedUnit.cpp:3213`. **If TASK-724 is re-worked
   before the one compile, this file must be re-checked** — the property reads fail loudly by design,
   but the static call is a compile-time coupling. **⛔ A test-only include; the two shipped files
   have zero coupling.**

## What QA should scrutinise

1. ⭐ **The two `SC-§15` deviations** — the elevation-shell gate shape and the refused nav lane.
   Argument 1 for the nav refusal (melee can never reach a 250-HP building) is the one to audit; if
   it is wrong, the nav lane comes back.
2. **The 300-uu gate floor arithmetic.** Capsule half-height 88 ⇒ top 176 ⇒ 124 uu clearance. If the
   capsule is not 88 (725 §3 says the project never calls `InitCapsuleSize`; `SiegeSpawnConstants.h:9`
   names 88), the clearance changes and a too-low floor is the way to manufacture stuck units.
3. **Test 4's and test 5(iii)'s self-checks are the file's whole integrity.** Confirm they actually
   fail if the iterator walks nothing.
4. **`AscentBlockedChannel` as single source of truth** — confirm `ConfigureAscentGate`'s one
   `ECR_Block` and `CanTeamAscend` genuinely both route through it, with no second expression of the
   rule anywhere.
5. **Confirm the include lists**: no `SummonedUnit.h` / `Tower.h` in the two shipped files.
