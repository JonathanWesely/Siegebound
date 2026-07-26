# TASK-255 — Scatter mines pass: mirrored placement + hill parity + clearance + traversability (handoff)

**Author:** gameplay-programmer · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x`
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h`, `BattlefieldScatter.h`, `BattlefieldScatter.cpp` — nothing else (file-scope law held).
**NOT compiled** (TASK-258 batch compile, by spec). Builds ON TASK-250's two-pass + hill-surface machinery (landed + compiled at `f49d8b1`); consumes TASK-253's `InitMine` API. This lands the fix for TASK-253 known-breakage #2 (the `Node->GetTeam()` caller in RebuildKeepClearZones is deleted with its block).

## What shipped (by spec section)

1. **ScatterConfig `Scatter|Mines` block** — `MineCountPerSide=3`, `MineMinSpacing=3000`, `MineClearanceRadius=600`, `MineGoldReserve=300`, `MineEdgeMargin=600`, `MineMaxSlopeDeg=30` (ClampMax 89), `MineClass` (`TSubclassOf<AGoldNode>`, null ⇒ `AGoldNode`). **`GoldNodeKeepClearRadius` REMOVED** (serialized DA value drops silently on load; TASK-257 populates the new block post-compile). No collision with the pending Phase-1 cull-field additions (those live on `FScatterLayer`; this block is on `USiegeScatterConfig`).
2. **RebuildKeepClearZones** — the ENTIRE gold-node block deleted: live `TActorIterator<AGoldNode>` sweep AND both hardcoded ±24,200 fallback discs (the phantom-disc trap — a comment at the deletion site records why). Keep-clears = castles + PlayerStart only. Docs updated everywhere they said "nodes" (class doc, method docs, member docs, config class doc).
3. **`PlaceMines(Seed)`** — called in GenerateScatter after the pass-2 loop, before `StartNavSettlePoll` (injected twin hills + clearance culls are inside the nav rebuild the validation waits on). Dedicated `FRandomStream(Seed ^ 0x4D494E45)` ("MINE" in ASCII) — the layer stream gains zero draws, existing seeds stable (seed-order law).
4. **`FindHillSurfaceAt`** — extends the TASK-250 trace to also surface `Hit.Item` (instance index) + the hit component; `ResolveHillAwareGroundZ` is now a thin delegating wrapper (layer-pass behavior byte-identical — same trace, same slope gate, identity outs discarded).
5. **`RemoveBlockingInstancesInDisc(Center, Radius)`** — disc sibling of `CullCorridorBlockers`: nav-relevant comps only (grass auto-excluded by the same `CanEverAffectNavigation` guard), visual+proxy lockstep via the existing `FindVisualForProxy` pairing, **hill-surface comps EXEMPT** (hills never deleted — the one guard the corridor cull deliberately does NOT have, pre-existing corridor semantics kept).
6. **Hill parity** (either-side-has ⇒ both-have): mirrored same-component `AddInstance` clone (−X, Y, yaw+180, Z/scale preserved — flat-slab floor makes the mirrored Z exact), footprint clearance-delete (un-bury), re-trace + slope gate, reject-if-no-fit (clone rolled back on reject — see deviations).
7. **Spawn** — tracked `SpawnedMines` (UPROPERTY Transient) pair per index, `AlwaysSpawn` collision handling (a nudged twin would break the equal-distance fairness proof), `InitMine(MineGoldReserve)` on both. Primary yaw 0 / twin yaw 180.
8. **Fallback** — ≤ 2×`MaxPlacementAttemptsPerInstance` (= 48 default) attempts, then the deterministic fallback slot with an **Error** log.
9. **`ClearScatter`** — destroys `SpawnedMines` + resets the array (Play-Again lifecycle; `AGoldNode::EndPlay` handles its own timer, skips miner notify per TASK-253).
10. **`ValidateTraversability` extension** — after the castle check passes: `FindPathToLocationSynchronously(BlueAnchor → each mine)`; any failure ⇒ widening per-mine disc cull (`MineClearanceRadius + attempt × CorridorWidenStep`) inside the SAME attempts machinery (shared `ReachabilityAttempt` counter, `MaxReachabilityAttempts` cap, nav-settle re-poll loop). **`RegroundMines()` after EVERY defensive cull** (both branches — the corridor cull can delete a supporting hill). CONFIRMED log keeps its grep prefix `Traversability CONFIRMED` and now appends the mine-path count. Distinct final-attempt Error texts for castle-fail vs mine-fail.

## THE DRAW SEQUENCE (determinism review mode — this is the audit table)

All draws come from `MineStream` (`FRandomStream(Seed ^ 0x4D494E45)`), consumed in this exact order and NOWHERE else:

| # | When | Call | Range (defaults, extents 26,000×12,000) | Purpose |
|---|------|------|------|---------|
| 1 | per attempt | `MineStream.FRandRange(MinAbsX, MaxAbsX)` | [1,500, 25,400] | \|X\|, negated → Blue-half X |
| 2 | per attempt | `MineStream.FRandRange(-MaxAbsY, MaxAbsY)` | [−11,400, +11,400] | Y |

- Per mine index i: attempts consume exactly 2 draws each, loop exits on first accept ⇒ total draws = 2 × Σ attempts-consumed. Deterministic given (seed, config, level actors).
- **Zero draws** in: spacing test, keep-clear tests, `GroundZAt`, `FindHillSurfaceAt`, parity injection/rollback, `RemoveBlockingInstancesInDisc`, the fallback slot, spawn, `RegroundMines`, `ValidateTraversability`. Every rejection path is draw-free BY CONSTRUCTION (the two draws happen first, all tests after), so no internal control flow can desync the sequence.
- Rejection-test order per attempt (cheap→expensive, all post-draw): spacing vs prior primaries → keep-clear discs at P → at P′ → hill resolve P → hill resolve P′ → parity (inject/re-trace).
- `MineCountPerSide<=0` or a degenerate draw band (warn-logged) consume zero draws — still deterministic.
- Constants derived, not drawn: `MinAbsX = max(MineClearanceRadius, MineMinSpacing/2)` = 1,500; `MaxAbsX/Y = HalfX/Y − MineEdgeMargin`.
- The reproducibility line (grep `MinesPass`): `MinesPass seed=%d mineStream=%d pairsPlanned=%d minesSpawned=%d reserve=%d: [i] P=(x,y,z) M=(−x,y,z) hill=yes|no inj=P|M|none fb=yes|no culls=%d …` — same seed ⇒ byte-identical line (the TASK-258 ×2 criterion).

**Why "spacing vs primaries only" is complete:** both primaries live on the Blue half with |X| ≥ max(ClearR, Spacing/2), so (a) own twin: dist(P,P′)=2|X| ≥ Spacing and the pair's clearance discs never overlap across X=0 (2|X| ≥ 2·ClearR); (b) cross-pair vs another pair's mirror: the X's add — dist(Pi,Pj′) ≥ |Xi|+|Xj| ≥ Spacing. Holds for ANY config where either term dominates. Documented at the band-setup comment in code.

## Fallback-slot coordinates (chosen + rationale)

Formula (zero draws, per mine index i, count N): `X = −clamp(HalfX·0.5, MinAbsX, MaxAbsX)`, `Y = clamp((i − (N−1)/2) × max(Spacing, 2·ClearR), ±MaxAbsY)`.
**Defaults ⇒ X = −13,000; Y ∈ {−3,000, 0, +3,000} for i ∈ {0,1,2}** (mirrors at +13,000, same Y). Mid-half — clear of Blue castle (−25,000, r 1,500+600) and PlayerStart (−23,800, r 800+600) by construction; slot-to-slot spacing exactly = MineMinSpacing; i=1 sits in the corridor (ALLOWED by ruling). Slots are NOT re-tested vs spacing/keep-clear (nothing left to try — the Error log flags the match). Slope gate opens to 90° at the fallback (must seat; a steep face means the mine sits ON the slope, never buried, because FindHillSurfaceAt still returns the top-surface Z).

## Deviations from spec (each justified — QA adjudicate)

1. **Parity order swap: re-trace BEFORE footprint-delete** (spec: delete → re-trace). Behavior-equivalent on accept: `FindHillSurfaceAt` reads ONLY hill-surface comps and the footprint delete touches only non-hill blockers (and `GroundZAt` ignores scatter blockers by the channel law) — deleting first cannot change the trace. Tracing first makes a REJECT side-effect-free (no blockers deleted for a candidate that never ships). Commented in code.
2. **Clone rolled back on reject** (spec silent). Without rollback, every rejected parity attempt strands an orphan mineless hill (up to 47/mine). `RemoveInstance(CloneIdx)` — the clone is the last-added instance, removal is index-safe.
3. **Re-trace "missed the clone" (`ReComp == nullptr`) also rejects** (spec names only the slope no-fit). yaw+180 mirrors the hill mesh's LOCAL Y (a true reflection needs negative scale — flips HISM normals; the house mirror law at ScatterLayer:382 is yaw+180), so an edge-of-hill primary can mirror OFF the clone's surface; grounding the twin at floor beside an injected hill would be a parity lie.
4. **"|X| ≥ max(600, spacing/2)" — the 600 read as `MineClearanceRadius`**, not `MineEdgeMargin` (both are 600 by default; the dispatch is ambiguous). Rationale: this floor exists to keep a pair's own clearance discs from overlapping across the centerline (2|X| ≥ 2·ClearR); the edge margin governs only the outer band. Board/CONVENTIONS' "|X| ≥ 1,500" reproduced exactly at defaults.
5. **No yaw draw for the mine actors** (primary 0°, twin 180° fixed). The specced draw list is X,Y only — a cosmetic yaw roll would shift every subsequent draw and pollute the audit table.
6. **`IsInKeepClear` refactored** to delegate its disc loop to the new `IsInKeepClearDiscs` (mines need discs-without-corridor). Layer-pass behavior byte-identical.
7. **Center-in-disc semantics** for `RemoveBlockingInstancesInDisc` (instance ORIGIN inside the disc) — matches its sibling `CullCorridorBlockers`' center-in-band. A huge-footprint blocker centered outside r=600 can still lean in visually; the mine itself is NoCollision so nothing functional blocks, and the reachability cull widens if nav disagrees.
8. **Mine reachability checked only after the castle path confirms** (spec doesn't order them). On an unpathable field every mine query would false-fail against the same break; the corridor cull is the prerequisite repair. Shared-counter consequence: castle-fail rounds consume attempts before mine checks run — accepted, it IS "the existing attempts machinery".
9. **Fallback pair-resolve failure residue:** if even the 90°-gate resolve fails at the fallback slot (defensive: unreadable source instance / clone miss), the pair grounds at floor Z — parity could theoretically be one-sided there. Purely defensive path, Error already logged.
10. **CONFIRMED log line extended** with the mine-path count (grep prefix `Traversability CONFIRMED` preserved for TASK-258's checklist).

## QA pointers (determinism review + the usual scans)

- **Determinism:** audit the two `FRandRange` calls at BattlefieldScatter.cpp (search `draw 1:`) — they are the only `MineStream` consumers; verify no other call sites.
- **`Hit.Item` as instance index:** `LineTraceComponent` on an (H)ISM reports the per-instance body index in `FHitResult::Item`, which is `GetInstanceTransform`-compatible — the parity clone source. Worth a 5.8 API double-check.
- **Lockstep + exemption:** `RemoveBlockingInstancesInDisc` mirrors `CullCorridorBlockers` exactly plus the `HillSurfaceComponents.Contains` exempt; the corridor cull deliberately keeps NO hill exempt (pre-existing behavior — that is why `RegroundMines` exists).
- **Hill-exemption scope note:** `HillSurfaceComponents` = pass-1 real-geometry blocker layers not opted onto hills (TASK-250 rule). In the shipped DA (TASK-251) rocks/trees are hill-opted, so only HILLS are exempt. A DA that leaves a blocker layer `bAllowOnHills=false` would make it clearance-immune — DA config smell, not a code path.
- **Shadow scan:** no member shadowing; `World` locals follow the house pattern; lambda params/outs are distinct from members.
- **Includes:** no new .cpp includes needed (GoldNode.h/EngineUtils/Engine/World.h already present); ScatterConfig.h adds `Templates/SubclassOf.h` + `class AGoldNode;` fwd; BattlefieldScatter.h adds `class AGoldNode;` fwd (TObjectPtr member).
- **Known cross-file state:** tree will NOT compile until TASK-254/256 land (MinerUnit/SiegeBotController still call the removed `AGoldNode` surface — expected, TASK-258 batch).
- **Grass under injected clones:** stays (harmlessly inside the mound) — grass-untouched law; grass also doesn't re-scatter onto clones (mines run after pass 2). Cosmetic, accepted.
