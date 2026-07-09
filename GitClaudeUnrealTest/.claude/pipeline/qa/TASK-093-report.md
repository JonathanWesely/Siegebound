# QA Report — TASK-093
Verdict: **PASS**

- Task: Placement v3 — building slope limit + obstacle clearance + ghost on sloped ground (C++)
- Reviewer: qa-reviewer, 2026-07-08 (pre-compile review; ships via TASK-103 batch compile)
- Files reviewed: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`, `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
- Blockers: 0 · Warnings: 1 · Nits: 3

## Findings

- [WARN] SiegePlayerController.cpp:1582-1607 — The slope trace (ECC_Visibility, straight down) can hit any transient actor whose mesh blocks Visibility if one occupies the candidate point (e.g. a walking unit whose VisualMesh carries a blocking profile), reading a body-surface normal and flashing a frame-transient "Too steep" red ghost. Direction of failure is SAFE (fail-closed, self-heals next frame once the actor moves), and the navmesh-projection gate running FIRST (cpp:1047) filters most such hits (extent Z=50), so this is not a blocker — but it is the one physical uncertainty in the chain. Suggested handling: no code change now; add a PIE check at TASK-096/103 integration ("place a building where a friendly unit stands — refusal must be transient and recover").
- [NIT] SiegePlayerController.cpp:1565-1569 vs 1612-1616 — Null-world asymmetry: `IsGroundSlopePlaceable` fails CLOSED on null world, `HasObstacleClearance` (and shipped `HasBuildingClearance`) fail OPEN. Both are unreachable in live play (PlayerTick implies a world); slope's fail-closed matches its spec. Acceptable as-is; documenting so the asymmetry is deliberate on record.
- [NIT] Handoff cites "Projectile.cpp's IWYU precedent" for `SCENE_QUERY_STAT`/`CollisionQueryParams.h` — Projectile.cpp:406 is itself uncompiled TASK-094 code, so no *compiled* in-repo precedent exists. The include is nonetheless correct: `SCENE_QUERY_STAT` is defined in the engine's `CollisionQueryParams.h`, and the include sits in correct alphabetical order (cpp:6). Informational only; both files compile-gate together in TASK-103.
- [NIT] SiegePlayerController.cpp:1591 — `!SlopeHit.bBlockingHit` after `LineTraceSingleByChannel` is redundant (the function's return IS bBlockingHit). Harmless belt-and-braces; keep or drop at leisure.

## UE 5.8 API check

No deprecated/removed APIs. `TActorIterator`, `LineTraceSingleByChannel`, `FCollisionQueryParams(SCENE_QUERY_STAT(...), bTraceComplex)`, `FVector::DistSquared2D`, `FMath::Acos/RadiansToDegrees/Clamp`, `ActorHasTag`, `NSLOCTEXT` — all current. Double-precision math is handled correctly (`ImpactNormal.Z` is double; clamp literals `-1.0, 1.0` deduce double; `SlopeDegrees` double vs float member compares by promotion — no narrowing).

## Shadow scan (C4457/58/59) — MANDATORY — CLEAN

New locals `TraceStart`, `TraceEnd`, `SlopeQueryParams`, `SlopeHit`, `SlopeDegrees` (IsGroundSlopePlaceable), `ObstacleTagName`, `ObstacleClearanceSq`, `Candidate` (HasObstacleClearance): none collide with any member/UPROPERTY in ASiegePlayerController (header verified end-to-end). Param `Point` matches the sibling-helper convention and shadows nothing. New UPROPERTY names `MaxPlacementSlopeDegrees`/`ObstaclePlacementClearance` are unique repo-wide (grep verified — only this class + comments). Existing locals in the touched functions (`Hit`, `bGroundHit`, `bValid`) unchanged and clean.

## Spec compliance (verified by inspection)

- **UPROPERTYs** (h:492-506): `MaxPlacementSlopeDegrees = 20.f` (ClampMin 0, ClampMax 90), `ObstaclePlacementClearance = 150.f` (ClampMin 0), both `EditDefaultsOnly, Category = "Siegebound|Placement"` with `// GDD §5 (M4.5)` comments. Names character-exact per the Fab amendment.
- **HUD strings character-exact:** `"Too steep"` (cpp:821, key `CardRefused_TooSteep`), `"Too close to obstacles"` (cpp:828, key `CardRefused_ObstacleClearance`). Tag FName `"Obstacle"` character-exact, static-const (cpp:1628).
- **Building-only gating:** both new gates behind `bPendingIsBuilding` (cpp:1049, 1054) — sourced from `IsBuildingCard` so Deep-Mine-style Economy buildings are covered and units/miners are exempt (navmesh-valid point suffices, unchanged).
- **Net-zero law:** both refusals resolve in `UpdatePlacementGhost` (runs in PlayerTick BEFORE the LMB confirm poll — cpp:212 vs 218) and fire from the `!bPlacementValid` branch at the TOP of `TryConfirmPlacement` (cpp:813-846), before `SpendGold`/spawn/`ConfirmPlayFromHand` can execute. Refusal stays in mode (correct — a different point can succeed).
- **Slope math:** angle = `acos(clamp(ImpactNormal.Z))`, refuse strictly `>` threshold (`<=` passes at cpp:1607); sideways/downward normals read >= 90° and refuse naturally. ±500 Z window brackets max terrain height 250. Trace miss / no world = fail-closed with Verbose log (correct level — per-frame path would spam).
- **Obstacle clearance:** 2D `DistSquared2D < clearance²` refuses; mirrors shipped `HasBuildingClearance` math shape character-for-character.
- **Refusal path signatures UNCHANGED:** `FOnCardPlayRefused`/`FOnCardRefused` byte-identical; new reasons ride the existing `RefuseCardPlay` → FString path. New `EPlacementInvalidReason` enumerators (`Slope`, `Obstacle`) are private, non-UENUM — zero serialization/BP surface.
- **UNCHANGED list verified:** unit/miner placement path, castle-roof refusal (`IsPointOnNavmesh` + extent 50, cpp:1506-1531), enemy-half (`PlacementMaxX`, cpp:1044), 200 `BuildingClearance` (cpp:1533-1560), miner cap entry gate (cpp:604-611) + confirm re-gate (cpp:863-871), card-leaves-hand-at-CONFIRM (cpp:993-1001), seven-exit-path melee-suppression law (`ExitPlacementMode` releases before early-out, cpp:651-660). No cards.csv/DT_Cards change.

## Rulings on the 8 flagged decisions

1. **Slope-trace channel = ECC_Visibility — ACCEPTED.** Channel coherence with `TraceCursorToGround` (cpp:1177) is the strongest correctness argument available: the surface that positioned the ghost IS the surface whose slope is measured, eliminating channel-mismatch drift by construction. `bTraceComplex=false` matches the cursor trace, and Use-Complex-As-Simple terrain answers with per-triangle normals either way. Spec offered the choice; documented as required.
2. **Slope measured at `PlacementLocation`, not the nav-projected point — ACCEPTED.** The building spawns at `PlacementLocation` (SpawnTransform, cpp:905); measuring where the actor will actually stand is the physically correct reading, `IsPointOnNavmesh` discards its projection today (shipped behavior — reusing it would be new plumbing), and the navmesh is a tessellated approximation whose surface can float off true geometry. Divergence bounded by NavProjectionExtent (50). The spec phrase is fairly read as "the candidate that has passed navmesh projection" — which the validation order enforces (gate 2 before gate 4).
3. **No obstacle caching — ACCEPTED.** Per-frame `TActorIterator<AActor>` over a few hundred actors, placement-mode-only, matching the shipped `HasBuildingClearance`/`IsPointInsideCastlePlinth` pattern. At N≈20 obstacles a cache buys nothing and adds staleness risk. Per-tick cost ruling: ACCEPTABLE — recomputing validity per tick during placement is the established, correct pattern here; revisit only if obstacle counts grow order-of-magnitude (M7 instancing note already covers that world).
4. **Boundary semantics (20.0° passes; exactly 150.0 passes) — ACCEPTED.** Spec wording "steeper than 20°" is a strict `>` — `<=` passing at exactly 20.0° is the correct reading. "Within 150" at exactly 150.0 is a measure-zero float boundary; mirroring the shipped `BuildingClearance` `<` comparison character-for-character makes the two clearance rules behave identically, which outweighs any inclusive reading. No change requested.
5. **ClampMax="90" — ACCEPTED.** Editor-safety metadata only; no behavior change at the default; an upward-facing normal can never exceed 90° from +Z, so values above 90 were dead range. Sensible unrequested hardening, correctly flagged rather than smuggled.
6. **Refusal priority slope → obstacle → building-clearance — ACCEPTED.** Spec pinned no order; first-fail-wins is recorded deterministically in `PlacementInvalidReason` and documented at the site (cpp:1023-1042). Ground-property-first, then clearances in age order is a defensible, stable convention.
7. **Names-block "OnCardRefusedMessage" — ACCEPTED, no penalty; convention ruling.** The board's name is NOT phantom: `UCardHandWidget::OnCardRefusedMessage` (CardHandWidget.h:142) is the BlueprintImplementableEvent at the TERMINUS of the refusal path (controller `OnCardRefused` → widget handler → BIE, 1:1). Its `const FString&` signature is untouched — the new reasons ride the existing FString path, so the names-block contract "signature UNCHANGED" is SATISFIED end-to-end. The board's phrasing is accurate as a path terminus but sloppy as a controller-member reference; the programmer's flag was correct diligence and is not scored against the task. **Carry-forward to manager:** when a names block pins a contract on ASiegePlayerController, name the controller delegates (`OnCardPlayRefused`/`OnCardRefused`) explicitly; reserve `OnCardRefusedMessage` for widget-side contracts.
8. **Ghost-on-slope as verification + comments, no new code — ACCEPTED.** Verified: `UpdatePlacementGhost` moves the ghost with `SetActorLocation(PlacementLocation)` only (cpp:1084) where `PlacementLocation` IS the cursor trace's `ImpactPoint` (surface Z on crowns/flanks); rotation is set exactly once at spawn, yaw-only (`GhostYawOffset`, cpp:1110) and never touched after; the law-encoding comment at the update site (cpp:1075-1083) fences future edits. Spec item 4 was already satisfied by shipped behavior — encoding it as law is the right deliverable.

## TASK-100 trap scan (same-file successor — heightened scrutiny)

No traps found. Specifically:
- `TraceCursorToGround` is cleanly reusable for the spell reticle: the enemy-half restriction lives in `UpdatePlacementGhost` (cpp:1044), NOT in the trace helper, so a targeting mode tracing anywhere-on-map inherits nothing it must undo — and the trace-to-surface behavior already satisfies the M5 "reticle projects onto uneven terrain, never assume Z=0" law (CONVENTIONS Spells block).
- `PlayerTick` early-outs on `!bInPlacementMode` (cpp:196) — a parallel targeting-mode branch slots in without touching the placement chain.
- `EPlacementInvalidReason` is private and non-serialized; `EnterPlacementMode`/`ExitPlacementMode` reset it symmetrically (cpp:615/664) — state hygiene is clean for a second mode living alongside.
- The refusal switch (cpp:815-844) has a safe `default:` arm — new reasons added later cannot fall through silently.

## Carry-forwards

1. **TASK-095:** every tree/rock instance MUST carry actor tag `Obstacle` (character-exact) or the clearance gate passes vacuously (which is correct on today's flat arena — zero tagged actors — but silently wrong after the Fab drop if tagging is missed).
2. **TASK-096/103 PIE checks:** (a) hill flank refuses "Too steep", crown (≤10°) accepts; (b) exactly-flat floor never refuses; (c) WARN-1 — place near/under a standing friendly unit and confirm any refusal is frame-transient.
3. **Manager:** names-block wording fix per ruling 7 (controller-delegate names vs widget BIE name).
4. **TASK-100:** proceed on this file — single-writer hold can lift on this verdict (orchestrator owns the flip).

## Notes for build-master (PASS)

- Ships in the TASK-103 batch compile together with TASK-094 (both consume `SCENE_QUERY_STAT`/`CollisionQueryParams.h` — first compiled use in this project; if it errors, both files are implicated, route back per QA-loop law).
- No content, no DT_Cards, no BP signature changes — code-only; existing WBP_HUD refusal binding displays the new strings with zero widget edits.
- Expect zero behavior change in PIE on the current flat arena except: nothing (no `Obstacle` tags exist yet; floor slope is 0°). Terrain behavior becomes observable only after TASK-091/095 land.
