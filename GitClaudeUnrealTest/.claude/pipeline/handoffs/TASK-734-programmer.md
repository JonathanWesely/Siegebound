# TASK-734 — [LADDER-4] THE LADDER: nav link + traversal entry + team gate + shell REMOVAL

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **Date:** 2026-09-01
**Law:** `TOWER-§8.1`/`§8.3`/`§8.4`/`§8.5`/`§8.6`/`§8.7` · `TOWER-§9` · `TOWER-§10` L-1/L-5 · `TOWER-§4a`/`§4b`/`§5` · `HIGH-§3` · `NAV-§` · `SHIP-§9c`

## Files touched (SOLE owner of all three)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` | Rewritten: `UClimbableTowerLadderLink` (new) + the ladder surface; the ascent shell REMOVED |
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp` | Rewritten: link arming, entry gate, completion lane, `EndPlay` abort; `ConfigureAscentGate` DELETED |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` | 5 tests kept (2 amended), **5 new** ⇒ 10 in this file |

⛔ Nothing else. `SummonedUnit.{h,cpp}` untouched · `Tower.{h,cpp}`/`ABuilding` untouched (`TOWER-§5`) · `Build.cs` untouched (`AIModule` + `NavigationSystem` were **already** `PublicDependencyModuleNames`) · no `Content/`, no editor, no MCP, no Git, no compile.

---

## 1. THE SMART-LINK SHAPE

`UClimbableTowerLadderLink : public UNavLinkCustomComponent`, one CDO subobject named `LadderLink`.

- **Smart, ⛔ never simple** — `TOWER-§8.1` M-3 re-verified at source: `SetMoveSegment` calls `StartUsingCustomLink` only when `PathPt0.CustomNavLinkId != FNavLinkId::Invalid` (`PathFollowingComponent.cpp:959-963`). A simple link path-plans beautifully and moves nobody.
- **`ENavLinkDirection::BothWays`**, set explicitly in the component constructor even though it is also the engine default — "we inherited the right value" is not auditable, and test 6(b) pins it.
- ⚠️ **It is a `UActorComponent`, ⛔ not a `USceneComponent`** (`UNavRelevantComponent : UActorComponent`). No `SetupAttachment`, no relative transform. Endpoints are OWNER-relative and the owner transform places them (`NavLinkCustomComponent.cpp:518-526`).
- Registration is the stock path: `bAttachToOwnersRoot` is `true` by default and `ABuilding`'s root `VisualMesh` is nav-relevant, so the link's modifier attaches to the building's own octree entry and leaves with it on destroy.
- **⛔ No tick and ⛔ no timer were added.** Entry is pushed (`FOnMoveReachedLink`), completion is pushed (`OnLadderClimbEnded`). Test 2 pins `bCanEverTick == false`; test 10 pins that no `Timer`/`Tick`/`Poll`/`Interval` member crept in as a substitute.

## 2. HOW THE SOCKETS ARE CONSUMED

`ConfigureLadderLink()` at `BeginPlay` (after `Super::`, so `Team` is authoritative — the same ordering the removed gate used).

- `VisualMesh->GetSocketTransform(Name, **RTS_Actor**).GetLocation()` for `LadderFoot` / `LadderTop`.
- ⭐ **`RTS_Actor` is load-bearing, not cosmetic, and it defuses a live hazard in this very actor.** `ABuilding`'s spawn squash writes `SetRelativeScale3D` on `VisualMesh` — **which is the root component**, so the *actor's* transform scale wobbles by up to **±30 %** for `0.15 s` after every spawn (`SiegeMeshJuiceComponent.cpp:102-106`, `SquashAmplitude 0.30`, settling to exactly `BaseScale`). `RTS_Actor` divides that transform straight back out (`StaticMeshComponent.cpp:1402-1407`), so what is **stored** is the authored socket coordinate and the squash can never be baked into the link. `RTS_World` would have.
- **Degrade OPEN, three ways, one warning**: no mesh · no socket · a line shorter than one nav-agent diameter (68 uu = 2 × `AgentRadius` 34) ⇒ fall back to the `TOWER-§8.3` literals `(-450,0,0)` / `(-150,0,1200)`, exactly **one** `Warning` naming which socket was missing, and the tower stays fully functional. ⛔ A degenerate line does **not** disable the link — that would make the deck an unreachable island, the one outcome `TOWER-§8.4(A)` forbids.
- At climb time the two **world** points are read back off the link (`GetStartPoint()`/`GetEndPoint()`), ⛔ never re-derived from the sockets — so the traversal and the navmesh cannot disagree about where the ladder is.
- Socket names live as `AClimbableTower::LadderFootSocketName` / `LadderTopSocketName` (`const FName` statics, the shipped `USiegeSettingsSubsystem::SettingName_*` pattern). Test 7(b) pins the literal strings and their distinctness.

## 3. THE SHELL REMOVAL + PREDICATE WIRING

**Removed:** `AscentGateVolume`, `AscentGateFloorUU`, `AscentGateHeadroomUU`, `AscentGateHalfExtentXY`, `ConfigureAscentGate()`, and the `Components/BoxComponent.h` include. ⛔ The known-open pivot-vs-mesh (`:106-107`) tuning item was **not fixed — it dissolved** (`TOWER-§8.2`).

**Kept and now LIVE:** `AscentBlockedChannel` (the vocabulary) and `CanTeamAscend` (the rule). `CanTeamAscend` had shipped with **no caller**; it now has two:

1. **The shipped gate** — the entry predicate at the one discrete entry point, via a new pure static `EvaluateLadderEntry(TowerTeam, ClimberTeam, bClimberIsSummonedUnit, bLadderOccupied)` returning `Climb` / `NotASummonedUnit` / `WrongTeam` / `LadderBusy`. Precedence is **identity → team → occupancy** and is part of the contract (test 8(d)/(e)); the whole gate is a pure function so it has a real headless truth table, leaving `HandleLadderLinkReached` as plumbing.
2. **`TOWER-§8.6`'s optional second layer — SHIPPED, because I measured the Querier** (details in §6 below). `IsLinkPathfindingAllowed` refuses an enemy a *path* through the ladder; it **fails OPEN** on every unresolved case.

⚠️ **The refusal path is load-bearing and measured, ⛔ not padding.** `UNavLinkCustomComponent::OnLinkMoveStarted` returns `true` — "this link is driving the agent now" — merely because the delegate is **bound** (`NavLinkCustomComponent.cpp:198-208`); it never asks whether we accepted. A refusal that just returned would leave the unit in custom-link limbo forever — a manufactured stuck unit created by the gate meant to protect the tower. **Every** non-`Climb` verdict therefore resumes path following immediately (`FinishUsingCustomLink`, the `ANavLinkProxy::ResumePathFollowing` idiom, `NavLinkProxy.cpp:344-363`).

## 4. HOW I AVOID CREATING A NINTH EXIT

**The structural rule: `ClimbableTower.cpp` never writes a movement mode, anywhere.** It cannot forget to restore one. Exactly **one** line in the file ends a climb, and it ends it through the unit's own idempotent API:

| Case | What this file does |
|---|---|
| **Tower destroyed mid-climb** (`L-5`) | `EndPlay` → `ActiveClimber->AbortLadderClimb()` (738's exit 8) → its broadcast re-enters my handler → unbind + resume + clear. A second `ReleaseClimber` follows as belt; it is idempotent. ⚠️ This was **free** under the ramp (the floor vanished and CM dropped to `MOVE_Falling`) and is **not** free now. |
| **Link disabled mid-climb** | ⛔ Cannot happen — **there is no runtime `SetEnabled()` call in this file at all.** I deliberately did not add one. |
| **Unit killed mid-traversal** | 738's `HandleDeath`/`EndPlay` exits fire; my handler only cleans up. `ActiveClimber` is a `TWeakObjectPtr`, so a unit destroyed without any completion path evaporates on its own — the defensive occupant clear `TOWER-§8` (6) asks for. |
| **Entry refused** (team / busy / non-unit / `BeginLadderClimb == false`) | ⭐ **No climb was started**, so there is no mode to restore. Agent handed back to path following the same frame. |

⭐ **A hazard I found in my own first draft and fixed — QA should confirm the fix, because it is exactly a ninth exit.** My occupancy term was originally `ActiveClimber.IsValid() && ActiveClimber.Get() != Unit`. Path following re-enters a link on every re-path (`SetMoveSegment` → `StartUsingCustomLink`, which force-finishes the previous link, `PathFollowingComponent.cpp:1454-1463`) — **including for the unit already on it.** Under that reading the re-entry would be *admitted*, `BeginLadderClimb` would refuse it as *already climbing*, and my refusal handler would then **unbind and clear a climber still in the air** — after which nothing would abort it when the tower died. ⇒ the term is now **`ActiveClimber.IsValid()`**, with ⛔ no identity comparison. Header + `EvaluateLadderEntry` + test 8(c) all carry the reasoning.

**Bind-before-call ordering** is also deliberate: `ActiveClimber` and the `AddUniqueDynamic` binding are set **before** `BeginLadderClimb`, because a completion can broadcast synchronously inside that call. Binding afterwards would miss it and leave a stale handle that bricks the ladder for the match.

## 5. `BothWays` IN BOTH DIRECTIONS — the one interpretive note on the pinned API ⚠️ **READ THIS, QA**

`DestPoint` (= `SegmentEnd`, `PathFollowingComponent.cpp:963`) tells me the travel direction; the nearer endpoint is the destination (margin: 1,236.9 uu). For a **descent** I call `BeginLadderClimb(TopWorld, FootWorld)`.

⚠️ **The pinned parameter *names* (`LadderFootWorld`, `LadderTopWorld`) read ascent-only. Its *semantics* are From → To.** I did **not** change the signature; I **verified 738's landed implementation** before relying on this:

- `FSiegeLadderClimbStatics::Begin` stores `State.Foot = arg1`, `State.Top = arg2` with ⛔ no Z-ordering and ⛔ no "must ascend" check (`SummonedUnit.cpp:155-178`).
- `ClimbDirection` = `(Top - Foot).GetSafeNormal()` (`:180-186`); `Advance` tests arrival against `State.Top` (`:203-219`).
- `CanBegin`'s only geometric refusal is `MinClimbLineUU = 1.f` (`:135-152`).

⇒ **A descent works correctly today.** 🧑 **Suggested law amendment, ⛔ not a divergence and ⛔ nothing to change now:** rename the two parameters to `FromWorld`/`ToWorld` in `TOWER-§8.4(B)`, since `BothWays` is *required* by `§8.7` and the current names invite a future implementer to add a Z-ordering "fix" that would silently break descent.

## 6. THE PATHFINDING LAYER — MEASURED, so it ships (`TOWER-§8.6` allowed it "only if you MEASURE the querier")

**`Querier` is the pathing pawn's `AController`.** Four cited sites, each re-derivable:

1. `AIController.cpp:868` — `OutQuery = FPathFindingQuery(*this, …)`; the query owner is the **controller**. Every unit move in this project goes through `AAIController::MoveToActor`/`MoveToLocation` (`SummonedUnit.cpp:2652`/`:2679`, `MinerUnit.cpp:921`/`:986`), so it is always our `ASiegeUnitAIController`.
2. `RecastNavMesh.cpp:3885` — `FindPath` passes `Query.Owner.Get()` down.
3. `PImplRecastNavMesh.cpp:1258` / `:442` — that owner becomes `FRecastSpeciaLinkFilter::SearchOwner`, cached by `initialize()`; `DetourNavMeshQuery.cpp:311-316` proves `dtNavMeshQuery::init` always calls `initialize()`, so the cache is never accidentally stale-null.
4. `PImplRecastNavMesh.cpp:439` — `isLinkAllowed` → `IsLinkPathfindingAllowed(CachedOwnerOb)`.

⚠️ **And it fails OPEN by construction** (null querier · not a controller · no pawn · pawn is not an `ITeamAgent` ⇒ `true`). The worst case of a wrong measurement is exactly the behaviour of not shipping the layer, ⛔ never an own-team unit that cannot path to its own tower. It does pointer reads and one enum compare only — pathfinding can run off the game thread; `GateTeam` is written once at `BeginPlay`, before any link exists to traverse.

⭐ **Why it is worth having:** the deck is a dead end, so Recast never routes *through* it — **except** when an enemy's target is a unit standing on the deck, which is a routine `MoveToActor` and exactly the `T-3` case. Without this layer that enemy walks to the ladder foot and is bounced by the entry predicate every re-path; with it, it simply gets no path and behaves as for any unreachable target.

## 7. TEST LIST — `Tests/SiegeClimbableTowerTest.cpp`, 10 tests

| # | Test | New? |
|---|---|---|
| 1 | Sibling of `ATower`, direct `ABuilding` child | kept |
| 2 | No fire-loop state, never ticks | kept (comment re-pointed) |
| 3 | `CanTeamAscend` refuses enemies / admits any own-team unit | kept (now the LIVE rule) |
| 4 | No capacity counter / ranged filter / occupant bookkeeping | **amended** |
| 5 | `HIGH-§3` parity + the damage seam stays sealed | **amended** |
| 6 | ⭐ The link is a **smart**, **`BothWays`** connection on the pinned line | **NEW** |
| 7 | ⭐ The four shell members are **GONE**; socket names pinned | **NEW** |
| 8 | ⭐ `EvaluateLadderEntry`'s full truth table **including precedence** | **NEW** |
| 9 | ⭐ The pathfinding layer **fails open** on every unresolved querier | **NEW** |
| 10 | ⭐ The completion **UFUNCTION** exists with 2 params; no polling substitute | **NEW** |

**Every assertion can fail, and each block carries a self-check that fails if the instrument goes blind:**
- test 6 seeds `GetLinkData`'s three out-params with sentinels and requires all three to be overwritten — otherwise the whole test could be reading its own scratch memory;
- test 6(d) **re-derives** the law's other two figures (length **1,236.9 uu**, lean **76.0°**) from the code's own two points, so a plausible typo shows up as a wrong length rather than as a coordinate nobody eyeballs;
- test 6(e) cross-checks the link's top Z against `PlatformHeightUU` — two independently declared numbers that must agree, so a height retune landing without the mesh fails here;
- test 7 does two **positive** lookups before four null assertions (a dead lookup would otherwise pass all four);
- test 8 proves the verdict is not a constant before believing any row, and 8(f) proves the entry gate *delegates* to `CanTeamAscend` rather than owning a second copy of T-3;
- test 9 anchors on the rule being discriminating, so "fails open" reads as an exception rather than the only behaviour;
- test 10 anchors on a UFUNCTION that predates the ladder.

### ⚠️ ASSERTIONS THAT DIED — declared, ⛔ not deleted in silence (`SC-§27` R-6)
1. **DIED:** test 4's self-check `DeclaredMemberNames.Contains("AscentGateVolume")`. The member was removed by law; a self-check requiring a deleted member is a guaranteed red. **REPLACED** by the same self-check on `LadderLink`, and the *stronger* half is **promoted into test 7**, which asserts all four removed members are gone by name — something the old file could not say at all.
2. **NARROWED:** test 5(d)'s `TowerAwarenessTokens` no longer bans `"Climb"`. ⛔ **Not a weakening of `HIGH-§3`** — `TOWER-§8.4(B)` *declares* the movement coupling, and `ASummonedUnit` now legitimately declares `BeginLadderClimb`/`AbortLadderClimb`/`IsClimbing`/`OnLadderClimbEnded`. A scan banning "Climb" would have failed on correct, specified code. `Tower`/`Platform`/`Occupan`/`Ascen` are untouched, and the removal is made honest by a **new self-check that requires `IsClimbing` to be FOUND** — the walk is proven live on the very token that left the list. *(Verified against the landed 738: the only `Tower`/`Platform`/`Occupan`/`Ascen` strings in `SummonedUnit.h` are `AClimbableTower`, `ATower` and `bIsOnATower`, all in comments; ⛔ no declared member.)*

### SUITE TOTAL FOR TASK-742: **187**
`171` baseline **+ 11** (TASK-738's `SiegeLadderClimbTest.cpp`) **+ 5** (this task) = **187**. Counted repo-wide over `IMPLEMENT_*_AUTOMATION_TEST`. ⚠️ Both programmer deltas are already on disk, so this is a measured number, ⛔ not a projection.

## 8. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐ **The occupancy term (`§4` above).** It is the one place I changed my own design to close a hanging-unit path. Confirm `bLadderOccupied == ActiveClimber.IsValid()` with **no** identity comparison, and that `ReleaseClimber` can therefore only ever be reached for a unit this tower registered.
2. **The descent call `BeginLadderClimb(TopWorld, FootWorld)`** (`§5`). I verified 738's implementation is From→To, but the parameter *names* say otherwise. If QA disagrees with my reading, this is the finding.
3. **`RTS_Actor` vs `RTS_World`** (`§2`). If this is wrong the link is silently mis-placed for the 0.15 s spawn squash window and, worse, the *stored* relative points would be scaled.
4. **Refusal always resumes.** Confirm every `return` after a non-`Climb` verdict is preceded by `ResumeAgentPathFollowing`, and that `EndPlay`'s double `ReleaseClimber` is genuinely idempotent.
5. **The `HIGH-§3` fence.** `SummonedUnit.h` is now included in `ClimbableTower.cpp` — confirm it is used **only** for `BeginLadderClimb` / `AbortLadderClimb` / `OnLadderClimbEnded` / `GetTeamId`, and that nothing calls `HeightAdvantageMultiplier` or reports elevation.
6. **`UClimbableTowerLadderLink` deriving from a `MinimalAPI` engine class.** I checked the generated header: `NavLinkCustomComponent.generated.h:41` exports the constructor as `NAVIGATIONSYSTEM_API` (⛔ not `NO_API`), so the subclass links. Worth a second pair of eyes since I cannot compile.

## 9. ⚠️ FOR TASK-742 (build-master) — A CONTENT ITEM I AM FENCED OUT OF

**`Content/Blueprints/Buildings/BP_Building_WatchTower.uasset` still carries `AscentGateVolume` in its FName table** (offset `808`, adjacent to `AttachParent`/`bAllowDeletion` — the inherited-component record). Once this code lands, that inherited component no longer exists on the native class.

- Expected: UE drops the orphaned node with a `LogBlueprint` warning on load and marks the BP dirty. It should **not** break the Blueprint.
- ⇒ **The BP needs a load + resave in the editor step**, and the load log should be checked for anything worse than a dropped-node warning. ⛔ I touched no `Content/`.
- ✅ The new `LadderLink` needs **no** BP work — it is a native CDO subobject and appears automatically.

## 10. DECLARED DEVIATIONS / RESIDUALS

1. **`TOWER-§8.6`'s optional layer was SHIPPED.** The law permits it only on a measurement; §6 above is that measurement, with four line-cited sites and a fail-open construction. If QA rejects the measurement, deleting `UClimbableTowerLadderLink::IsLinkPathfindingAllowed` (4 lines) reverts to the law's minimum with ⛔ no other change.
2. **A new `UCLASS` (`UClimbableTowerLadderLink`) lives in `ClimbableTower.h`.** Inside my SOLE-owned file; ⛔ not a replicated class, ⛔ not a new relevancy tier, ⛔ not a class-tier change.
3. **`EndPlay` aborting an in-flight climb is ⛔ NOT covered by a headless test** and is declared as such in the test file's "does not cover" block. It needs a world, a spawned tower, a spawned unit and a live climb; fabricating world-less possessed actors to fake it is more likely to crash the suite than to catch the bug. Test 10 asserts the completion lane `EndPlay` drives (the 2-param UFUNCTION) instead, and 738 ships an **independent belt** for the same case (`SummonedUnit.cpp:725-733`, exit 7 covering exit 8).
4. **`TOWER-§4a`'s residual stands, unchanged and unsolved by design:** a unit that falls when the tower dies can land where the navmesh has not yet regenerated, with ⛔ no recovery lane. ⛔ No nav-projecting teleport was added.
5. **The 0.15 s spawn-squash window**: the *stored* link data is squash-proof (`RTS_Actor`), but `GetStartPoint()`/`GetEndPoint()` return world points through the live actor transform, so for ≤0.15 s after a spawn the link's *world* bounds wobble by up to 30 %. A climb cannot start in that window (a unit has to path there first) and the nav rebuild outlasts it. Accepted; ⛔ no timer added to dodge it, since a timer would cost the no-tick property.
6. **`MinimumLadderLineUU = 68 uu`** (2 × `AgentRadius`) is *my* guard on the **link geometry** — deliberately stricter than, and independent of, 738's `MinClimbLineUU = 1.f` guard on the **climb math**. Different resources, different questions.
7. **⛔ Not delivered here, by design:** `TOWER-§9.3`'s exposure-table restatement is **TASK-738's** handoff deliverable (its spec item 6), not this task's.
8. Airlock: ⛔ no `Capture()`/`EnsureSnapshot()`, ⛔ no token figure, Zone A untouched, 🔒 latch untouched. M8: ⛔ no replicated property, ⛔ no RPC, ⛔ no class-tier change.
