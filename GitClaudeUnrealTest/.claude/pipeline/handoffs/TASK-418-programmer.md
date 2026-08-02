# TASK-418 — [LLM-A5] `AAncientGround::FindNearestAncientGround` — handoff

**Agent:** gameplay-programmer
**Status:** ready-for-qa (gate = TASK-419)
**Date:** 2026-08-02

## M8 DECLARATION DUTY (verbatim, as required)

> **adds no replicated property, no new replicated class, no new relevancy tier.**

`AAncientGround`'s **NET RELEVANCY TIER C — NOT REPLICATED** declaration is untouched. `bReplicates` is still
never set, the added member is a **static free-function-shaped query** with no state at all, and it neither
reads nor writes anything that crosses the wire.

## Pre-flight (spec step 0)

`git status --porcelain Source/GitClaudeUnrealTest/Siegebound/AncientGround.{h,cpp}` → **empty (clean)** before
any edit. No parked work merged. Note the board/CONVENTIONS both spell the path without the `Siegebound/`
folder in one place and with it in another — the real path is
`Source/GitClaudeUnrealTest/Siegebound/AncientGround.{h,cpp}` and that is what was checked and edited.

## What changed

Two files, **72 insertions, 0 deletions, 0 modifications** (`git diff --stat`):

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/AncientGround.h` | +28 — doc comment + the one public static declaration, inserted between `GetZoneHalfExtent()` and the `OnConstruction` override |
| `Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp` | +44 — the definition, inserted between `IsPointInZone` and `ApplyBoostTick` |

Signature, character-for-character against the CONVENTIONS §9 pinned registry (`CONVENTIONS.md:699`):

```cpp
static AAncientGround* FindNearestAncientGround(UWorld* World, const FVector& From);
```

Verified mechanically — piping the registry line and the header line through `sort -u` collapses them to a
**single** line, and `cat -A` shows no trailing whitespace.

## Donor matched: `AGoldNode::FindBestMineFor`

Primary donor as specced. Line-for-line correspondence:

| Element | Donor (`GoldNode.cpp:187-236`) | This implementation (`AncientGround.cpp:153-195`) |
|---|---|---|
| Declaration site | `GoldNode.h:153`, plain public static, **no `UFUNCTION`** | `AncientGround.h:119`, plain public static, **no `UFUNCTION`** |
| Null-`World` guard | `GoldNode.cpp:189-192` → `nullptr` | `AncientGround.cpp:155-158` → `nullptr` |
| Best/BestDistSq seed | `GoldNode.cpp:194-197`, `TNumericLimits<float>::Max()` | `AncientGround.cpp:160-161`, identical |
| Iteration | `TActorIterator<AGoldNode> It(World)` (`:199`) | `TActorIterator<AAncientGround> It(World)` (`:167`) |
| Validity skip | `!IsValid(Mine)` (`:202`) | `!IsValid(Ground)` (`:170`) |
| Metric | `static_cast<float>(FVector::DistSquared2D(...))` (`:211`) | identical (`:183`) |
| Tiebreak | strict `<`, first-found wins, stable iteration (`:216`) | identical (`:184`) |
| Empty answer | `nullptr` = normal endgame answer, never a crash (`:235`) | `nullptr` = no grounds, never a crash (`:194`) |

`ACastle::FindNearestCastleForTeam` (`Castle.h:190` / `Castle.cpp:655-691`) was read as the second precedent
and agrees with all of the above; the shape here is closer to Castle's (single tier) than to GoldNode's
two-tier body, which is the only structural difference — see deviations.

### ⚠️ DONOR PROVENANCE — one correction to the dispatch, QA please note

The dispatch described both finders as "the two shipped precedents". Verified against `HEAD`:

- **`AGoldNode::FindBestMineFor` IS shipped** — present at `HEAD`, at the identical `GoldNode.cpp:187`. My
  line references to it are stable and reviewable against the committed tree.
- **`ACastle::FindNearestCastleForTeam` is NOT at `HEAD`** — `git show HEAD:...Castle.cpp | grep` returns
  nothing. It is the **FOLLOW batch's parked, uncommitted TASK-398 work** sitting dirty in the working tree
  (its own doc comment says TASK-398). All `Castle.h` / `Castle.cpp` line numbers above are therefore
  **working-tree references only** and will drift if that batch changes.

This does not weaken the implementation: I matched **`FindBestMineFor`**, the committed donor, so this task
creates **no dependency on the FOLLOW batch landing**. Castle was used only as corroboration that a
single-tier reduction of the idiom is the accepted house shape. Nothing in `Castle.*` or `GoldNode.*` was
edited — they were opened read-only as donors.

## Deviations from the donor — all three deliberate, please scrutinize

1. **The tier-2 branch is gone (single tier).** `FindBestMineFor` keeps a second candidate (`BestWait`) for
   enemy-occupied mines and returns `BestMinable ? BestMinable : BestWait`. An ancient ground has **no claim
   state and no owner** — it is team-neutral by the shipped FRIENDLY-ONLY boost law — so there is nothing to
   fall back to and only one `Best`. This makes the body structurally identical to
   `ACastle::FindNearestCastleForTeam`, which is why both were named as donors.
2. **No team parameter and no team filter** (spec step 2, and I put the reasoning in the doc comment so a
   later task cannot "fix" it): near/far is resolved by the **caller's `From`**, per CONVENTIONS §8
   (`CONVENTIONS.md:608`) — own-castle location ⇒ `ancient_ground_near`, enemy-castle location ⇒
   `ancient_ground_far`. Under the 180°-rotational-symmetry law the two grounds are rotational twins, so the
   two calls return the two distinct grounds exactly, with zero extra state.
3. **No `UFUNCTION(BlueprintPure)`** — spec step 4 makes exposure optional. Skipped on purpose, for two
   reasons: (a) **both** donors are plain statics with no `UFUNCTION`, and "follow the idiom exactly" was the
   instruction; (b) a raw `UWorld*` first parameter on a Blueprint-exposed static wants
   `meta = (WorldContext = "World")` to be usable from a graph, and adding that meta **would** change the
   pinned line — so exposing it cleanly and keeping §9 character-for-character are in tension. The only
   consumer in this batch (`USiegeAssistantSnapshot::ResolvePlace`, TASK-416) is C++. Adding exposure later
   is a one-line, zero-risk change if anyone wants it.

## Invariants held (spec step 3 / QA-419 check 9)

- **`grep -c HasAuthority AncientGround.cpp` → `0`.** The pushed-authority law is intact; the new function
  reads no authority at all.
- **Zero deletions and zero modified lines** in both files (`git diff -U0 | grep '^-'` returns nothing beyond
  the `---` file headers). The boost tick, `InitAncientGround`, `IsPointInZone`, the decal/footprint code, the
  paired `ZoneHalfExtent` tunable and the Tier-C declaration are **byte-identical**.
- **No new include** in either file — `EngineUtils.h` (`TActorIterator`) and `Engine/World.h` were already
  included at `AncientGround.cpp:8` and `:7`. `UWorld` needs no forward declaration in the header, exactly as
  in `GoldNode.h` / `Castle.h`, which declare the same `UWorld*` parameter and rely on `GameFramework/Actor.h`.
- **Line endings:** both files are 100% CRLF after the edit (246/246 and 306/306 lines), matching the
  untouched `GoldNode.cpp` / `Castle.cpp`. No mixed-ending contamination.
- Compile traps checked: no literal `*/` inside the doc comment, no `FString::Printf`, no shadowing (`Ground`,
  `BestGround`, `DistSq`, `BestDistSq` collide with no member of `AAncientGround`, and the function is static
  so no member is in scope anyway).

## What QA should scrutinize

1. **The signature against `CONVENTIONS.md:699`** — that is the whole of check (1) for this task.
2. **The `Ground->GetActorLocation()` call is the ACTOR ORIGIN, not the zone box.** `FindNearestAncientGround`
   answers "which ground's *center* is nearest", not "am I inside one" — `IsPointInZone` remains the
   containment test and is untouched. A caller wanting "the ground I am standing on" must use `IsPointInZone`,
   not this. Flagging it because the two are easy to confuse; no current caller does.
3. **The 2D metric is a genuine choice, not an accident.** `IsPointInZone` ignores Z, and both donors use
   squared 2D — so a ground on a rise cannot lose the "nearest" test to a flatter one. Consistency with
   `IsPointInZone` was the deciding argument.
4. **Deviation 3 (no Blueprint exposure)** is the one judgement call where a reviewer could reasonably want
   the opposite. It is reversible in one line and changes nothing else.
5. There is **no caller yet** — TASK-416 is the first consumer and lands in the same compile unit at
   TASK-420. This task adds a leaf function only.

## Not done, per the hard limits

No compile, no Git, no editor, no MCP, no `Content/` change, `L_Arena` never opened. `SiegePlayerController.*`,
`SummonedUnit.*`, `MinerUnit.*` (FOLLOW batch) and `GoldNode.*` / `Castle.*` (read-only donors) were **not**
modified by me.

⚠️ **Reading `git status` correctly for this task:** the working tree is NOT clean — the FOLLOW batch's parked
work leaves `SiegePlayerController.*`, `SummonedUnit.*`, `MinerUnit.*`, `Castle.*`, `GoldNode.*`,
`UnitCommand.h`, `SiegeCheatManager.cpp`, `DeckBuilderWidget.cpp` and several `Content/` assets dirty, and they
were **already dirty before I started**. That is expected (the FOLLOW batch is mid-gate) and none of it is
mine. My contribution is exactly the two `AncientGround` files plus this handoff and the board status line.
The spec's pre-flight is scoped to `AncientGround.{h,cpp}` and **those two were clean**, which is the check
that mattered. A reviewer diffing the whole tree will see foreign changes — do not attribute them here.
