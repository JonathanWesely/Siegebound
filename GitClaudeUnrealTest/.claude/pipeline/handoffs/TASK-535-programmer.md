# TASK-535 — `BattlefieldScatter`: 3 telemetry call sites + HONEST traversability + the determinism fix

**Agent:** gameplay-programmer · **Status:** `ready-for-qa` · **QA gate: TASK-537** (`qa/TASK-537.md`)
**Law:** CONVENTIONS `NAV-§4` · `NAV-§5` · `NAV-§7` · `NAV-§8` · `NAV-§9` · the TRAVERSABILITY GUARANTEE ("Battlefield & procedural terrain (M6.5)") · the determinism law (`TASKBOARD.md:11044`) · the seed-order law.

> ⚖️ **M8 DECLARATION, VERBATIM: adds no replicated property, no new replicated class, no new relevancy tier.**
> Structural reason: everything added is server-side validation state on an actor that already replicates only its
> seed pair (`ChosenSeed` + `GenerationIndex`, untouched). The client path (`OnRep_GenerationIndex` →
> `RunScatterPasses(..., false)`) returns before every line I added past the passes — it never validates, never culls,
> and never binds the delegate. No `UPROPERTY(Replicated)` and no `DOREPLIFETIME` entry was added or moved.

---

## 1. FILES TOUCHED — the file pair I own, and nothing else

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` | +136 / −4 — class-doc law block, 4 new private methods, 6 new members, 1 new `UPROPERTY`, 2 forward decls |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` | +303 / −8 — 2 includes, 3 telemetry call sites, the settled/provisional labelling, the settled-only cull gate, 4 new function bodies |

⛔ **NOTHING ELSE MOVED.** No `SiegeNavDiagnostics` edit (TASK-529 owns it — I only `#include` and call it), no `SummonedUnit`,
no `SiegeNavAreas`, no `.ini`, no tests, no `Build.cs` (`NavigationSystem` is already a public dep), **no `.umap` write and no
`L_Arena` save** (`NAV-§5`), no compile, no Git, no editor/MCP/PIE.

---

## 2. PART 1 — THE THREE TELEMETRY CALL SITES (behaviour-free)

All three are one-liners against TASK-529's shipped library (`FSiegeNavDiagnostics`, signatures verified
character-for-character against the `NAV-§8` pin — `static void LogNavConfigOnce(const UWorld*)` and
`static void LogNavBuildSnapshot(const UWorld*, const TCHAR*)`; both take `const UWorld*` and are null-World safe).

| # | site | code |
|---|---|---|
| 1 | `BeginPlay`, **before** the authority branch | `FSiegeNavDiagnostics::LogNavConfigOnce(TelemetryWorld);` + `LogNavBuildSnapshot(TelemetryWorld, TEXT("pre-scatter"));` |
| 2 | `GenerateScatter`, **after** `RunScatterPasses(Seed, true)` | `LogNavBuildSnapshot(World, TEXT("post-scatter"));` |
| 3 | `ValidateTraversability`, immediately after the World null-check | `LogNavBuildSnapshot(World, TEXT("at-confirmation"));` |

- ⭐ Site 1's `gatherOnGameThread=` token is the batch's acceptance test for TASK-530 and the trigger for TASK-540.
- Site 1 is **deliberately NOT authority-gated** (the spec says "BeginPlay"): the config line is a pure read, and a
  client whose serialized nav actor disagrees with the server's is exactly what this line exists to expose. Site 2 is
  authority-only for free (it is inside `GenerateScatter`, which refuses on a non-authority copy at its head), and site 3
  is authority-only because the validation itself is (M8 doc D9).
- Site 2 sits in `GenerateScatter`, **not** in `RunScatterPasses` — per spec, and because `RunScatterPasses` is also the
  client regen path.
- **Spam check:** site 1 fires once per world (the library latches per-`FObjectKey`), site 2 once per generate, site 3 once
  per validation pass (bounded by `MaxReachabilityAttempts` = 5 plus at most one definitive pass). No tick, no poll.

---

## 3. PART 2 — HONEST LABELLING

`ValidateTraversability` now reads the live queue before it makes any claim:

```cpp
const int32 RemainingTileTasks = FMath::Max(NavSys->GetNumRemainingBuildTasks(), 0);
const bool  bNavStillBuilding  = UNavigationSystemV1::IsNavigationBeingBuilt(World);
const bool  bNavSettled        = (RemainingTileTasks == 0) && !bNavStillBuilding;
```

### 🚩 DECLARED STRENGTHENING (not a silent deviation) — the `&& !IsNavigationBeingBuilt` conjunct

⚠️ **I READ THE ENGINE AND THE SPEC'S STATED MECHANISM IS NOT WHAT UE 5.8 DOES.** `NAV-§1` cause 3 and the task spec
both say `IsNavigationBeingBuilt` (`:1953`) "reads idle ~211 s before the queue drains", implying
`GetNumRemainingBuildTasks()` can be **> 0** while the poll reads idle. **That state is impossible:**

- `GetNumRemainingBuildTasks()` = Σ over `NavDataSet` of `RunningDirtyTiles.Num() + PendingDirtyTiles.Num() + TileGeneratorSync.IsValid()` (`RecastNavMeshGenerator.h:794`).
- `IsNavigationBeingBuilt` = `HasDirtyAreasQueued() || IsNavigationBuildInProgress()`, and `IsNavigationBuildInProgress` is `RunningDirtyTiles.Num() || PendingDirtyTiles.Num() || TileGeneratorSync.IsValid()` over the **same** set (`NavigationSystem.cpp:5549`, `:4889`, `RecastNavMeshGenerator.cpp:7764`).
- ⇒ **`IsNavigationBeingBuilt() == false` PROVES `GetNumRemainingBuildTasks() == 0`.** The pinned predicate alone can therefore *never* fire on the poll's idle-exit path — it would have been a no-op there.

The conjunct exists so the label means what it says in the *other* direction: **dirty areas that have not yet become tile
tasks read `remaining=0` while carving is still owed.** Folding that in can only make `CONFIRMED` **harder** to print,
never easier, so it strictly serves `NAV-§4` ("a pre-settle query may NEVER print CONFIRMED") and strictly tightens the
cull gate. The pinned `GetNumRemainingBuildTasks()` is still the primary read and still the number in the log line.
**If QA rules the conjunct out of scope, deleting the `&& !bNavStillBuilding` term is a one-token revert** — but read §7
first, because the conjunct is doing real work.

### The exact lines emitted (QA greps these)

**SETTLED SUCCESS** — `LogSiegeTerrain, Log`:
```
[BattlefieldScatter '%s'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + %d mine path(s) exist (after %d cull(s))%s.
```
**PRE-SETTLE SUCCESS** — `LogSiegeTerrain, Warning`:
```
[BattlefieldScatter '%s'] Traversability PROVISIONAL (%d tile task(s) pending — PRE-SETTLE query; %s) — a Blue→Red castle path + %d mine path(s) were found, but the navmesh is NOT settled, so this is NOT the CONFIRMED guarantee; the definitive re-check runs once on OnNavigationGenerationFinished%s.
```
**PRE-SETTLE FAILURE (cull suppressed)** — `LogSiegeTerrain, Warning`:
```
[BattlefieldScatter '%s'] Traversability PROVISIONAL (%d tile task(s) pending — PRE-SETTLE query; %s) — castle lane %s, %d mine(s) unreachable; defensive cull SUPPRESSED (bCullOnProvisionalFailure=false, the determinism law): a cull decided against a partially-built navmesh is decided by wall-clock timing. Deferring the repair to the definitive post-settle check (OnNavigationGenerationFinished)%s.
```
**GENERATION FINISHED (the event)** — `LogSiegeTerrain, Log`:
```
[BattlefieldScatter '%s'] Nav generation FINISHED ('%s') — running the DEFINITIVE post-settle traversability check (one deferred re-check; NAV-§4).
```
Trailing `%s` on the verdict lines is `VerdictSource` = `" [definitive: OnNavigationGenerationFinished]"` on the
event-driven pass, `""` otherwise. The `; %s` inside the parenthesis is `UnsettledReason` =
`"tile tasks still queued"` or `"dirty areas queued but not yet submitted as tile tasks"` — so a
`PROVISIONAL (0 tile task(s) pending …)` line can never read as a contradiction.

⚠️ **PINNED-STRING CONFLICT I RESOLVED IN FAVOUR OF CONVENTIONS + the board's `names:` block.** `NAV-§4` and
`TASKBOARD.md` `names:` pin `CONFIRMED (nav settled: 0 pending)` / `PROVISIONAL (N tile task(s) pending — PRE-SETTLE query)`;
the dispatch prompt worded them `CONFIRMED (nav settled: 0 tile task(s) pending)` / `PROVISIONAL (N tile task(s) **still**
pending — …)`. **Both pinned CONVENTIONS strings appear verbatim in my lines**, and I added `0 tile task(s) pending` /
the reason clause so the prompt's phrasing is also present in substance. A grep for either CONVENTIONS literal hits.

The two terminal `Error` lines also carry the honesty rule: the mine-force-clear line previously asserted
"castle lane itself is CONFIRMED" unconditionally; it now prints `CONFIRMED (nav settled: 0 pending)` **or**
`PROVISIONAL (PRE-SETTLE query)` per the same `bNavSettled`.

---

## 4. PART 3 — THE DETERMINISM FIX (settled-only cull) AND HOW IT IS ENFORCED

**One gate, immediately above the cull machinery, and the cull code itself is untouched:**

```cpp
if (!bNavSettled && !bCullOnProvisionalFailure)
{
    UE_LOG(... "defensive cull SUPPRESSED ..." ...);
    return;      // ⛔ no cull, ReachabilityAttempt NOT consumed, no re-poll armed
}
++ReachabilityAttempt;   // ← everything below this line is unchanged
```

- `bCullOnProvisionalFailure` — `UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Terrain|Traversability")`, **default `false`**,
  documented as the diagnostic-A/B escape hatch that re-opens the hole (⛔ never a shipping default).
- **The gate is the only way in.** `CullCorridorBlockers` and `RemoveBlockingInstancesInDisc` have exactly one caller each on
  this path (`ValidateTraversability`), and the `PlaceMines` / `PlaceAncientGrounds` clearance calls are *seed-deterministic
  placement*, not nav-driven culls — they sit far above this function and were not touched.
- `ReachabilityAttempt` is **not** consumed by a suppressed pass: it counts *culls*, and no cull happened. That keeps the
  widening ladder's band sequence a function of how many culls ran, not of how many times a timer fired.
- **No re-poll is armed on suppression** — re-polling a navmesh whose queue we just measured as non-empty is exactly the
  216 s stall the spec forbids. The event bind is what brings us back.

**Why the guarantee survives (also written into the header):** post-settle the reachability answer is STABLE, so a cull
driven by it is a pure function of the geometry — hence of the seed. **The cull is not removed; it is MOVED to the only
moment at which it is both TRUE and DETERMINISTIC.** Layer (1) — the reserved corridor — never depended on nav state at
all and is byte-unchanged, so the geometric half of the NON-NEGOTIABLE guarantee is untouched.

### Delegate lifecycle (bind → fire → discharge → unbind)

| step | where | detail |
|---|---|---|
| **arm** | `RunScatterPasses`, authority tail (beside `ReachabilityAttempt = 0`) | resets `bDefinitiveCheckPending/Done/bInDefinitiveCheck`, clears `DefinitiveCheckTimerHandle`, calls `BindNavGenerationFinished()` |
| **bind** | `BindNavGenerationFinished()` | latched by `bNavGenerationFinishedBound`; **authority-only**; null-World and null-NavSys safe (one `Verbose` line, no warning spam); `AddUniqueDynamic` on `UNavigationSystemV1::OnNavigationGenerationFinishedDelegate` (`NavigationSystem.h:444`, `FOnNavDataGenericEvent`, `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(..., ANavigationData*, NavData)`); stores `TWeakObjectPtr<UNavigationSystemV1> BoundNavSystem` |
| **fire** | `UFUNCTION() OnNavGenerationFinished(ANavigationData*)` | early-out on `bDefinitiveCheckDone \|\| bDefinitiveCheckPending` (the engine broadcasts **once per `ANavigationData`**), on non-authority and on null World; then sets the pending latch, logs once, and arms a **one-shot** 0.001 s timer |
| **run** | `RunDefinitiveTraversabilityCheck()` | clears the pending latch, re-checks the done latch, sets `bInDefinitiveCheck`, calls the **same** `ValidateTraversability` (no second implementation of the guarantee), clears the flag |
| **discharge** | inside `ValidateTraversability` | settled + reachable **on the definitive pass** ⇒ `bDefinitiveCheckDone = true` + `UnbindNavGenerationFinished()`. Both `MaxReachabilityAttempts`-exhausted terminal branches do the same. |
| **teardown** | `EndPlay` | clears **both** timer handles and calls `UnbindNavGenerationFinished()` unconditionally |

- **`RemoveDynamic` goes through the weak ptr**, so a nav system torn down before `EndPlay` is simply forgotten rather than
  unbound through a stale pointer. `UnbindNavGenerationFinished()` is idempotent and safe to call when never bound.
- ⚠️ **Deferred by one timer tick ON PURPOSE.** The broadcast originates *inside* the Recast generator's tick
  (`RecastNavMeshGenerator.cpp:7631` → `ARecastNavMesh::OnNavMeshGenerationFinished` → `NavigationSystem.cpp:4915`). Culling
  inline would delete HISM instances and re-dirty nav areas from **inside the nav system's own generator tick**. One tick of
  latency removes that whole class of re-entrancy against a check that used to be wrong by ~211 s. It is a **one-shot that
  never re-arms itself** — not a poll.
- **A settle-poll `CONFIRMED` deliberately does NOT discharge the bind.** A queue that reads empty at +5 s can be a lull
  between waves of dirty areas; the point of the bind is to get one verdict against a navmesh **the engine itself calls
  finished**. Normal match ⇒ exactly **one** extra reachability check (1 castle + N mine sync path queries, the same cost the
  existing check already pays), then unbind. Once unbound, mid-match nav churn (buildings) cannot re-trigger anything.

---

## 5. ⛔ PLACEMENT / SEEDING IS BYTE-UNCHANGED — the explicit statement

**No `FRandomStream` construction, draw, draw ORDER, placement rule, density, layer count, keep-clear radius, spacing,
corridor width, mirror/180°-rotation step, mine stream (`Seed XOR 0x4D494E45`) or ancient-ground stream
(`Seed XOR 0x41474E44`) was read, moved, added or removed.** `ScatterLayer`, `IsInKeepClear`, `IsInKeepClearDiscs`,
`RebuildKeepClearZones`, `GroundZAt`, `ResolveHillAwareGroundZ`, `FindHillSurfaceAt`, `PlaceMines`, `PlaceAncientGrounds`,
`RegroundMines`, `CullCorridorBlockers`, `RemoveBlockingInstancesInDisc`, `ResolveCastleLocation` and `ClearScatter` are
**not modified by this diff at all** (verify: `git diff` touches none of their bodies).

**Proof shape for one fixed seed** (QA can check by reading, per criterion 6 — nothing here needs a run): the whole
seed-deterministic body is `RunScatterPasses`, and my only edit inside it is a block of **latch assignments + a timer clear
+ `BindNavGenerationFinished()` placed AFTER the two layer passes, after `PlaceMines`, after `PlaceAncientGrounds` and after
the `!bAuthoritativeGenerate` early-return** — i.e. after the last draw of the generate, on the authority path only. The two
telemetry calls in `BeginPlay` execute **before** `GenerateScatter` and draw nothing; the `post-scatter` call executes after
it. ⇒ For a fixed `OverrideSeed`, the `GenerateScatter seed=… mirror=… layers=… corridorHalfY=…` line, the `MinesPass` line
and the `AncientGroundsPass` line are byte-identical before and after this task, and so is every instance transform.
**The only permitted difference is WHICH CULL RUNS AND WHEN** — exactly as `NAV-§4` requires.

---

## 6. 🚩 FLAGS FOR QA / THE MANAGER — the things I decided rather than buried

1. **🚩 THE ACCEPTED VISIBLE LATE CULL (recorded, NOT designed around).** A post-settle cull can now delete an instance
   **mid-match** (~27 s at 8× tile concurrency, ~216 s if TASK-540 reverts Stage 1) where it used to happen invisibly at
   +5 s. It fires **only** when the field is genuinely walled off — the HARD-FAILURE case — and a visible pop is strictly
   better than an unwinnable match. ⛔ I did **not** restore the provisional cull to avoid it. It is on Jonathan's TASK-539 list.
2. **🚩 THE SPEC'S STATED MECHANISM FOR CAUSE 3 IS WRONG IN UE 5.8 SOURCE** (§3 above): `IsNavigationBeingBuilt() == false`
   *proves* `GetNumRemainingBuildTasks() == 0`, so the pinned predicate alone could never have fired on the poll's idle path.
   **The conclusion still stands** — I verified it in the shipped logs, see §7 — but the *route* to the false `CONFIRMED` is
   the `MaxNavSettleWait` cap, not a false idle. `NAV-§1` cause 3's second sentence and `NAV-§12`'s "which of the two
   returned false is UNKNOWN" limitation should be corrected by the manager at the checkpoint; I did not edit CONVENTIONS.
3. **🚩 DECLARED STRENGTHENING**: the `&& !IsNavigationBeingBuilt(World)` conjunct in `bNavSettled` (§3). One-token revert
   if QA rules it out of scope.
4. **🚩 A SECOND `FTimerHandle` EXISTS** (`DefinitiveCheckTimerHandle`). `NAV-§3`'s ZERO-NEW-TIMERS law binds the **stuck
   watchdog** (TASK-531/532/533/534), not this actor, which is already timer-driven. It is a **one-shot deferral**, never
   re-armed by itself, cleared in `EndPlay` and on every re-scatter, and it is deliberately **not** shared with
   `TraversabilityTimerHandle` (the two can legitimately be in flight together; one handle would silently cancel the other).
5. **🚩 RESIDUAL, HONEST**: `bNavSettled` proves *the queue is empty right now*, which is not identical to *the navmesh will
   never change again*. If dirty areas arrive in waves, an early wave's drain can produce a settled-but-not-final verdict.
   The design's answer is that every verdict — including that one — is taken against a **settled** navmesh, so no cull is
   ever decided by a half-carved one; and a cull immediately re-dirties nav, which makes the *next* pass provisional and
   therefore cull-free until the re-carve settles. That self-protection is what stops two triggers double-culling the same
   window. A strictly-final verdict would require waiting for a quiet period, i.e. polling — which the spec forbids.
6. **Not flagged, just noted:** `Traversability CONFIRMED` (the old bare string QA has grepped since M6.5) **no longer
   appears**; every occurrence now carries a `(nav settled: …)` or `PROVISIONAL` qualifier. Any downstream grep for the bare
   string needs updating — this is the intended, spec-mandated break.

---

## 7. WHAT I VERIFIED FIRST-HAND (and what I could not)

**Verified against installed UE 5.8 source** (`C:\Program Files\Epic Games\UE_5.8\Engine\Source`):
`NavigationSystem.h:441-444` (the delegate is a `UPROPERTY(BlueprintAssignable)` `FOnNavDataGenericEvent`, public) ·
`NavigationSystem.h:1106` `GetNumRemainingBuildTasks() const` is **public** · `NavigationSystem.cpp:4913-4946` ·
`NavigationSystem.cpp:5549-5560` · `NavigationSystem.cpp:4889-4909` · `RecastNavMeshGenerator.h:794` ·
`RecastNavMeshGenerator.cpp:7614-7631` (the broadcast fires on the `bHasTasksAtStart && !bHasTasksAtEnd` transition —
i.e. genuine queue drain) · `RecastNavMesh.cpp:3237-3240`.

**Verified against the shipped logs** (`Saved/Logs/`):
- `GitClaudeUnrealTest-backup-2026.08.04-02.55.13.log` — **four matches, every one of them**:
  `Navigation still building after 10.0 s (MaxNavSettleWait cap) — proceeding with the reachability validation anyway.`
  immediately followed by `Traversability CONFIRMED`. ⇒ **This is the real false-CONFIRMED route, and my PROVISIONAL branch
  fires on exactly it.** (lines 2583-2584, 2769-2770, 3040-3041, 3241-3242.)
- `GitClaudeUnrealTest.log` — the session the plan cites: scatter `00.44.39:632`, `CONFIRMED` `00.44.44:659` (**+5.03 s**),
  and **no cap warning**, i.e. the poll genuinely read idle ⇒ the tile-task queue really was empty at that instant.

**I could NOT verify** (nothing was compiled and nothing was run — TASK-538 owns the only compile): that the emitted lines
render as written; that `activeTiles=`/`poolCap=` behave as TASK-529 documents; whether the +5 s idle in the cited session
is a genuine settle or a between-waves lull — **the `at-confirmation` snapshot's `dirtyAreas=`/`hasDirty=` tokens are what
will answer that on the first PIE run**, and that is precisely why site 3 exists.

---

## 8. WHAT QA SHOULD SCRUTINISE

1. **The cull gate placement** — that no path reaches `++ReachabilityAttempt` without passing `bNavSettled ||
   bCullOnProvisionalFailure`. One `if`, one `return`, directly above it.
2. **`bCullOnProvisionalFailure` really defaults `false`** and is `EditDefaultsOnly` (header, tunables block).
3. **The delegate is bound once and unbound on `EndPlay`** — plus the three other unbind sites (settled+confirmed on the
   definitive pass, and both terminal `Error` branches). `AddUniqueDynamic` / `RemoveDynamic` / weak-ptr teardown.
4. **`UFUNCTION()` on `OnNavGenerationFinished`** and the signature matching `FOnNavDataGenericEvent`
   (`ANavigationData*`) — a dynamic delegate cannot bind a non-`UFUNCTION`.
5. **Complete-type include law** — `#include "NavigationData.h"` added because `GetNameSafe(NavData)` upcasts through a
   complete type; `#include "Siegebound/SiegeNavDiagnostics.h"` for the three calls. No `Build.cs` change.
6. **No shadowing** — the new `UWorld* const World` in `RunScatterPasses` is the only `World` in that scope;
   `TelemetryWorld` in `BeginPlay` is named to avoid any collision; no inherited reflected member is shadowed.
7. **The seed-order law** (criterion 6, "check by reading, not by accepting the claim") — §5 names every function the diff
   does *not* touch.
8. **Flag 2 in §6** — the CONVENTIONS text that my engine read contradicts.
