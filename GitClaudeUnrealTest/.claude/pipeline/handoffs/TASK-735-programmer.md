# TASK-735 — FOOTPRINT-AWARE BUILDING PLACEMENT — programmer handoff

**Assignee:** gameplay-programmer · **Status → `ready-for-qa`** · delivered 2026-09-02
**Law:** `TOWER-§7` · `TOWER-§6` row **T-6** · `STACK-§6` (the scaled-bounds requirement) · `SC-§15` · `SC-§33` · `SHIP-§9c` · `NAV-§` · `HIGH-§1`

> ⛔ **No compile · no editor · no MCP · no Git.** TASK-814 owns the compile; TASK-816 owns the gate.

---

## 1. FILES TOUCHED — the complete list, and it is three

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | +243 / −3 |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | +255 / −5 |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` | **NEW**, 10 tests |

⛔ **Nothing else.** `AClimbableTower` · `SM_WatchTower` · ladder geometry · `Tools/ArtPipeline/build_watchtower.py` · every WidgetBlueprint · `Building.{h,cpp}` · `CardHandWidget.{h,cpp}` — **not opened for edit.**

✅ **`LADDER_OUTWARD_SHIFT = 10.0` VERIFIED INTACT** at `Tools/ArtPipeline/build_watchtower.py:137`, and `git status` on that file is **empty** (unmodified). `LADDER_FOOT` / `LADDER_TOP` still derive from it at `:146-147`.

⚠️ **`git diff --stat` on `Source/` also lists `Building.{h,cpp}`, `CardHandWidget.{h,cpp}` and `ClimbableTower.h` — those are NOT mine.** They belong to the parallel Lane-A / TASK-812 work running in the same wave. My diff is exactly the two `SiegePlayerController` files plus one new test file.

---

## 2. ⭐ HOW THE FOOTPRINT RADIUS IS DERIVED FROM THE MESH — and the proof no literal survives

**The single read, `SiegePlayerController.cpp:4568`:**

```cpp
const FBoxSphereBounds ScaledBounds = GhostMesh->CalcBounds(GhostMesh->GetComponentTransform());
DerivedRadius = PlacementFootprintRadiusFromBounds(ScaledBounds.BoxExtent);
```

- **Which bounds call, and why (the disclosure `STACK-§6` promoted to a requirement):**
  **`UStaticMeshComponent::CalcBounds(GetComponentTransform())` — the ⭐ SCALED bounds, ⛔ never `UStaticMesh::GetBounds()`'s LOCAL ones.** `CalcBounds` applies the component's live world transform, so `GetScale3D()` is already inside the answer. ⇒ the day **TASK-815** wheels the ghost to ×1.5, this reads ×1.5 with **zero further edits**, and a 1.5× building can never be validated at 1.0×. At the shipped scale of 1 the two calls agree, so this is **not speculative work today**.
- **Radius = `max(|X|, |Y|)`** — the larger horizontal half-extent, 2D only. Z is discarded (a tower is *tall*; height must not leak into a planar refusal).
- **⚠️ ROTATION IS ALSO IN `CalcBounds`, DECLARED:** the ghost's only rotation is `GhostYawOffset = -90°`, an exact quarter turn that swaps X and Y and leaves `max(|X|,|Y|)` invariant — and `UpdatePlacementGhost` moves the ghost without ever re-rotating it (its own shipped comment says so). ⇒ **exact today.** An off-axis yaw would inflate the world AABB, which **over-refuses and can never under-refuse**. Follow-on, **not fixed here**.

**⛔⛔ PROOF NO LITERAL SURVIVES — three independent instruments:**

1. **Grep.** `grep -n "2700\|1350\|375\.\|750\.\|600\.f"` over both changed files returns **exactly two hits, both inside PROSE comments** explaining *why* a literal is forbidden (`SiegePlayerController.h:587`, `.cpp:4474`). **Zero code literals.**
2. **The only numbers in the new code are `0.f` sanitisation guards and the one `EditDefaultsOnly` tunable (`UnitPlacementClearance = 0.f`).** No size, no extent, no radius is written down anywhere.
3. **Test 2 is the executable proof** (`SiegePlacementTest.cpp`): three different meshes must produce three different answers, each equal to its own input, and the three must be mutually distinct. **Any constant — 2700, 750, 375, or the shipped clearance — fails at least two of those five assertions.** Test 2(c) additionally pins *both* the pre-redesign (~1,350 half-extent) and post-redesign (~375) tower sizes reporting themselves through the same unedited code — the `TOWER-§7` dividend, made a regression claim.

**Degrade path (house null-safety law):** missing ghost actor / missing static mesh / degenerate-or-NaN bounds ⇒ `TryGetPlacementFootprintRadius` returns **false** with `OutRadius = 0` and **ONE `Warning` per placement session** (`bWarnedNoFootprintBounds`, cleared in `EnterPlacementMode` — per *card*, because the missing piece is that card's ghost). The caller then **skips the unit gate entirely** and composes the building clearance with `0`, which is `max(200, 0) = 200` — **the shipped rule byte-for-byte**. It never bricks placement and never refuses on an absent asset.

---

## 3. THE NEW REJECTION REASON, AND HOW IT READS TO THE PLAYER

**`EPlacementInvalidReason::Units`** — exactly one new value, appended to `{ None, Point, Slope, Obstacle, Clearance }`, matching the family's shape (a noun for the offending thing). ⛔ **`Clearance` is NOT reused**, and the header says why: `Clearance` names *another building* and tells the player so; reusing it for a unit overlap would be a lie the player cannot act on.

**Player-facing:** red ghost while hovering, and on the confirm click —

> **"Your units are in the way"**

through the shipped `RefuseCardPlay` vocabulary, alongside *"Too steep"* / *"Too close to obstacles"* / *"Too close to another building"*. **No gold moves** (the refusal returns before any spend, unchanged), the player **stays in placement mode** so a different point can succeed, and the message names the thing to move — which matters because **the fix is the player's**: ⛔ **placement REFUSES and NEVER moves a unit** (`NAV-§`, spec (3)). 🧑 **T-6 is Jonathan's row and one word overrules the whole refusal.**

**Re-gated at confirm exactly as every other building rule is** — and structurally, not by a second code path: `UpdatePlacementGhost` recomputes `bPlacementValid` + `PlacementInvalidReason` every frame, and `TryConfirmPlacement` reads only those two members. The new rule inherits that mechanism with no additional work.

**The one new tunable** — `UnitPlacementClearance`, `EditDefaultsOnly`, `ClampMin = 0`, **ships at `0.f`**. The 0 is a decision: the two terms it is added to (the mesh footprint + the unit's own scaled capsule radius) already express *"the building would materialise through this unit's body"* exactly, so 0 refuses real overlap **and nothing more** — the smallest new refusal that closes the finding. **Its consequence is written beside it** (`HIGH-§1`): every uu costs placement room around *every* unit at once, so in a busy spawn box a large building can become genuinely unplaceable until the army is ordered elsewhere.

---

## 4. ⭐ EVIDENCE THAT EXISTING SMALL BUILDINGS ARE NON-REGRESSIVE

Four independent arguments, all structural rather than promissory:

1. **⭐⭐ THE COMPOSITION IS A `max`, NOT A SUM.** `EffectiveBuildingClearance(Base, Footprint) = max(Base, Footprint)`. For **every** footprint radius ≤ `BuildingClearance`, the function returns `BuildingClearance` **identically — not approximately**. Every shipped small building therefore cannot change behaviour. A sum would have pushed all existing content apart by its own size; the max cannot. **Test 4 sweeps 0 / 0.01× / 0.25× / 0.5× / 0.75× / 0.99× / 1.0× of the shipped clearance and demands exact equality with tolerance `0`.**
2. **⛔ THE SHIPPED `200.f` IS UNTOUCHED**, as are `ObstaclePlacementClearance = 150.f` and `MaxPlacementSlopeDegrees = 20.f`. **Test 10 pins all three off the CDO by reflection** — the footprint work cannot quietly retune the rules it composes with.
3. **⭐ THE NEW GATE IS EVALUATED LAST.** Every pre-existing rule (spawn box → navmesh → slope → obstacle → building clearance) is checked *before* the unit rule, so under first-failing-rule-wins **no placement that refuses today can change WHICH message it shows.** Cheapest-first ordering is preserved too (one trace, then three iterations).
4. **⛔ UNIT AND MINER CARDS ARE UNTOUCHED, AT THE READ.** `bPendingIsBuilding` short-circuits `TryGetPlacementFootprintRadius` itself, so a non-building card pays no bounds transform and emits no warning — exactly as it is exempt from the slope and obstacle gates (M4.5 ruling 7).

⚠️ **THE ONE HONEST CAVEAT, STATED RATHER THAN BURIED: the unit gate itself is genuinely NEW for every building card, small ones included** — spec (2) mandates that, and it *is* the fix. What "non-regressive by construction" buys is that the refusal region **scales with the mesh**: a wall refuses only within its own small footprint (physically-correct overlap), never within a tower-sized radius. Nothing that was legal *and not overlapping a unit* has become illegal.

---

## 5. ⛔ FENCED OUT AND DECLARED, NOT SILENTLY SKIPPED (spec (5))

**Not re-sampled across the footprint — still POINT tests, by instruction:** ground hit / spawn box / captured zone · navmesh projection (`NavProjectionExtent`) · `MaxPlacementSlopeDegrees` · `ObstaclePlacementClearance`. `TOWER-§7` records that general form as a **follow-on finding**.

**⭐ SPEC (5) INVITED ME TO SAY WHERE THE NARROW FIX IS ACTIVELY MISLEADING. Three places — reported, ⛔ not fixed:**

- **(a) THE OTHER BUILDING'S FOOTPRINT IS STILL A POINT.** `HasBuildingClearance` composes **the ghost's** radius only, so two large buildings are separated by **one** radius, not two. A large structure can still sit closer to an *existing* large structure than either one's own size. Fixing it means reading bounds off every live `ABuilding` — a different, wider change.
- **(b) THE SLOPE GATE IS THE MOST MISLEADING SURVIVOR.** The ghost is now *known* to be wide, and the slope is still measured by a **single** straight-down trace at the cursor. A structure whose centre sits on a flat crown can overhang a 40° flank and pass. Now that the footprint radius exists as a value, four corner traces would be a small change — **and it is not this task's.**
- **(c) THE SPAWN-BOX TEST IS ALSO A POINT.** A large building placed at the very edge of the spawn box legally extends outside it. Same shape as (b), same reason for not touching it.

**Also deliberately excluded (spec (2)):** ⛔ enemy units (a different problem with a different answer) · ⛔ `AHeroCharacter` · ⛔ `ASiegeGhostPawn` · ⛔ `ACommanderNpc` — **all three verified NOT to derive from `ASummonedUnit`**, so `TActorIterator<ASummonedUnit>` genuinely excludes them. `AMinerUnit` and `ASorcererUnit` **do** derive from it and are therefore covered.

**Airlock / M8:** ⛔ no `Capture()` / `EnsureSnapshot()` · Zone A untouched · 🔒 the 552 latch unspent · ⛔ no token figure (`AS-§12g`) · ⛔ **no replicated property, no RPC, no class-tier change** — placement validation remains client-side pre-gate + server confirm, unchanged.

---

## 6. ⚠️ WHAT `TASK-813` AND `TASK-815` MUST KNOW ABOUT MY DIFF

They serialize behind me on the same two files. **Everything they need, in one place:**

1. **⛔ I DID NOT REFACTOR THE `TryConfirmPlacement` SWITCH.** I added one `case EPlacementInvalidReason::Units:` in place (`SiegePlayerController.cpp:1985-1996`), directly after the `Clearance` case and before `default:`. **You add yours the same way** — a `default:` is present, so no `-Wswitch` exhaustiveness pressure. I considered extracting a `reason → FText` mapper and **rejected it deliberately**: three tasks editing one switch in series are better served by a small additive diff than by a refactor that reflows lines you are about to touch.
2. **⭐ `EPlacementInvalidReason` STAYS `private`** (`SiegePlayerController.h:1719-1726`). `Units` is appended **last**. **Append yours after `Units`** — do not insert in the middle.
3. **⭐⭐ `TASK-815`'s VERIFICATION ITEM (5) IS SATISFIED: THE FOOTPRINT READS *SCALED* BOUNDS.** The call is `GhostMesh->CalcBounds(GhostMesh->GetComponentTransform())` at `SiegePlayerController.cpp:4568`, and `SiegePlacementTest.cpp` test 3(b) asserts a ×1.5 transform yields exactly 1.5× the local radius and **is not** the local radius. ⇒ **you do not need to change the bounds read at all.** Set the ghost component's scale and the footprint follows.
4. **⛔ `HasBuildingClearance` CHANGED SIGNATURE** — now `HasBuildingClearance(const FVector& Point, float FootprintRadius) const`. **Exactly ONE call site** (`:2255`); **no parameter is defaulted** (`SC-§33`), and the call-site grep is pasted in §8 below. `HasUnitClearance` and `TryGetPlacementFootprintRadius` are likewise undefaulted.
5. **New private state:** `bWarnedNoFootprintBounds`, reset in `EnterPlacementMode` beside `bPendingIsBuilding` (`SiegePlayerController.cpp:1546`). New `UPROPERTY`: `UnitPlacementClearance`, in the `Siegebound|Placement` block after `ObstaclePlacementClearance`.
6. **New public statics** (plain C++, ⛔ not `UFUNCTION`s), declared as one block after `ExitPlacementMode()`: `PlacementFootprintRadiusFromBounds` · `EffectiveBuildingClearance` · `IsInsidePlacementFootprint` · `UnitFootprintRefusalText`.
7. **⭐ THE TEST FRAME IS `Tests/SiegePlacementTest.cpp` AND IT IS NAMED FOR THE MECHANIC, NOT FOR THIS TASK — ⛔ EXTEND IT, DO NOT ADD A SECOND PLACEMENT TEST FILE.**
8. **The validity chain now has SIX gates** (`UpdatePlacementGhost`, `:2239-2270`). The footprint is read **once per frame** into `FootprintRadius` / `bFootprintKnown` before the chain — reuse those locals rather than reading bounds again.

---

## 7. TEST LIST + ⭐ SUITE DELTA

**New file `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` — 10 `IMPLEMENT_SIMPLE_AUTOMATION_TEST`s.**

**"None fits", per spec (7) — checked first.** No placement or controller test file existed. The six files that `#include` `SiegePlayerController.h` (`SiegeAssistantSelectionTest` · `SiegeControlsHelpTest` · `SiegeDeckSlotsTest` · `SiegeRecallTest` · `SiegeRespawnLifecycleTest` · `SiegeWarMapTest`) are each another feature's frame. ⛔ Not a duplicate frame.

| # | Claim |
|---|---|
| 1 | Radius is the larger horizontal half-extent; **Z is ignored**; sign-blind; zero/NaN → 0 |
| 2 | ⭐ **No literal survives** — three meshes, three distinct answers; both the pre- and post-redesign tower sizes report themselves |
| 3 | ⭐⭐ **Scaled, never local** — ×1.5 yields 1.5× and **is not** the local radius; ×0.5 shrinks; the shipped −90° yaw is invariant; an off-axis yaw only ever grows it |
| 4 | ⭐ **Non-regression** — 7 small footprints all return the shipped clearance with tolerance `0`; **+ the control that a large one moves the answer**; degrade path (0 / negative / NaN) is exact |
| 5 | It is a **max, not a sum**, above and below the clearance; never falls below the shipped floor |
| 6 | ⭐⭐ **The refusal discriminates** — large footprint refuses, **small footprint ADMITS the same unit at the same distance**, large footprint **ADMITS** a distant unit; gate is live at the shipped `0` pad; planar |
| 7 | Body radius and pad **each independently** flip the answer; boundary is exclusive (`<`); the three terms compose |
| 8 | ⛔ **Degrade open** — zero/negative/NaN footprint contains nobody, **+ the control that a real overlap still refuses** |
| 9 | ⭐ The new refusal's message is **its own**, differs from all four shipped messages, and names units |
| 10 | The three shipped tunables are **unchanged** off the CDO; the one new tunable exists and ships at `0` |

**⛔⛔ EVERY ASSERTION CAN FAIL — and the trap you named is guarded explicitly.** *"A placement-refusal test passes trivially if the footprint never overlapped anything."* ⇒ **every refusal claim is paired with a control that must be ADMITTED**, holding the fixture fixed and moving only the term under test: 6(b) same unit + small footprint · 6(c) same footprint + distant unit · 7(a) same geometry + zero body · 8(c) the inverse control that a real overlap still refuses. **A "refuses everything" implementation fails 6(b), 6(c), 7(a), 8(a); a "refuses nothing" implementation fails 6(a), 7(b), 7(c), 8(c). Neither can pass this file.** The shipped numbers are read off the CDO by **reflection** and the footprints are expressed as **multiples of the shipped clearance**, so the tests state a *relationship* and keep meaning through any T-6 retune.

**⚠️ TWO ENGINE-API CORRECTIONS MADE WHILE WRITING, WORTH THE GATE'S EYE:**
- **`FAutomationTestBase::TestNotEqual` has NO numeric overload in UE 5.8** (verified at `Misc/AutomationTest.h:2004-2012` — TCHAR*/FStringView/FString/FUtf8StringView/FText/FName only). Every *"these must differ"* float claim goes through a fixture helper `DiffersFrom()` instead. A float `TestNotEqual` would not have compiled.
- **NaN is built from its bit pattern** (`0x7FC00000` via `FMemory::Memcpy`), adopting the `SiegeStuckStaticsTest.cpp:90-98` house idiom, ⛔ not `FMath::Sqrt(-1.f)` (foldable). **Each NaN test asserts `FMath::IsNaN` on the fixture first**, so a build that optimised the value away reports itself instead of passing silently.

### ⭐⭐ SUITE DELTA — READ THE ARITHMETIC, THE BASELINE MOVED UNDER ME

- **My delta: `+10`.** Against the dispatched baseline of **306**, TASK-735 alone gives **316**.
- ⚠️ **BUT THE WORKING TREE NOW MEASURES `331`, AND THAT IS NOT MY DOING.** Two test files from parallel wave tasks landed while I worked, both still untracked: **`SiegeBuildingStackTest.cpp` (+10, TASK-812)** and **`SiegeCardHandKeyLabelTest.cpp` (+5, the CARDBAR lane)**.
- **The arithmetic closes exactly: `306 + 10 (mine) + 10 (812) + 5 (cardbar) = 331`** — re-measured with `grep -rh "^IMPLEMENT_.*_AUTOMATION_TEST(" | wc -l` across 26 files.
- ⇒ 📌 **THE INTEGRATION GATE MUST ASSERT THE WAVE TOTAL, ⛔ NOT `316`.** As of this handoff that is **331**, and **TASK-813 / TASK-815 will raise it further**. Whoever declares the number last owns it.

---

## 8. SC-§33 CALL-SITE GREP (pasted, for the gate to re-run)

```
$ grep -rn "HasBuildingClearance\|HasUnitClearance\|TryGetPlacementFootprintRadius" Source/ --include=*.cpp --include=*.h
Siegebound/SiegePlayerController.cpp:2237:  const bool bFootprintKnown = bPendingIsBuilding && TryGetPlacementFootprintRadius(FootprintRadius);
Siegebound/SiegePlayerController.cpp:2255:  if (bValid && bPendingIsBuilding && !HasBuildingClearance(PlacementLocation, FootprintRadius))
Siegebound/SiegePlayerController.cpp:2260:  if (bValid && bPendingIsBuilding && bFootprintKnown && !HasUnitClearance(PlacementLocation, FootprintRadius))
Siegebound/SiegePlayerController.cpp:4342:  bool ASiegePlayerController::HasBuildingClearance(const FVector& Point, float FootprintRadius) const
Siegebound/SiegePlayerController.cpp:4543:  bool ASiegePlayerController::TryGetPlacementFootprintRadius(float& OutRadius)
Siegebound/SiegePlayerController.cpp:4597:  bool ASiegePlayerController::HasUnitClearance(const FVector& Point, float FootprintRadius) const
Siegebound/SiegePlayerController.h:2263:  bool HasBuildingClearance(const FVector& Point, float FootprintRadius) const;
Siegebound/SiegePlayerController.h:2310:  bool HasUnitClearance(const FVector& Point, float FootprintRadius) const;
Siegebound/SiegePlayerController.h:2341:  bool TryGetPlacementFootprintRadius(float& OutRadius);
```

**`HasBuildingClearance` has exactly ONE call site and it passes the new argument explicitly. No parameter anywhere in this diff is defaulted.**

---

## 9. WHAT QA SHOULD SCRUTINISE HARDEST

1. **The bounds call.** `CalcBounds(GetComponentTransform())` vs `GetStaticMesh()->GetBounds()` — `STACK-§6` makes this the load-bearing line (`SiegePlayerController.cpp:4568`). Confirm the *scaled* one shipped.
2. **`max` vs `sum` in `EffectiveBuildingClearance`.** The entire non-regression argument rests on it. A `+` there silently changes every shipped building's spacing.
3. **The degrade asymmetry is deliberate** — building clearance takes radius `0` (⇒ shipped 200), the unit gate is *skipped* via `bFootprintKnown`. Verify a missing ghost mesh produces **today's exact behaviour**, not a radius-0 unit gate.
4. **Gate ORDER.** The `Units` check must stay **last** in `UpdatePlacementGhost` — that is what preserves first-failing-rule-wins for every pre-existing refusal message.
5. **`bWarnedNoFootprintBounds` is reset in `EnterPlacementMode` only.** Confirm no path can enter placement with a stale latch (and that the warning cannot spam a per-frame gate).
6. **Team resolution in `HasUnitClearance`** — no `ASiegePlayerState` ⇒ degrade **open**, never "guess Blue".
7. **My three declared misleading survivors in §5(a)/(b)/(c)** — please rule whether (b), the single-trace slope gate under a now-known-wide ghost, should be boarded as a follow-on task.
8. **The suite number in §7.** The baseline moved under me; the gate must not assert `316`.
