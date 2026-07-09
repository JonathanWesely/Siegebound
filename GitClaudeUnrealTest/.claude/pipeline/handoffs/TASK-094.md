# TASK-094 — Projectile terrain/tree collision (C++) — programmer handoff

**Date:** 2026-07-08
**Status set by orchestrator; work is ready-for-qa from my side.**
**Files only — NOT compiled (build-master owns compiles; rides M5's TASK-103 batch per the Fab amendment).**

## Files touched (complete list)

- `Source/GitClaudeUnrealTest/Siegebound/Projectile.h`
  - Class comment: new "Terrain law (M4.5)" bullet; softened the "can never collide with anything" claim to the still-true physics statement.
  - New private const helper declaration: `bool FindEnvironmentImpact(const FVector& TraceStart, const FVector& TraceEnd, FHitResult& OutHit) const;`
- `Source/GitClaudeUnrealTest/Siegebound/Projectile.cpp`
  - Two new includes: `CollisionQueryParams.h`, `Engine/HitResult.h` (IWYU hygiene; both already reachable via Engine/World.h / Actor.h).
  - `Tick`: after the (unchanged) target reach test and direction computation, the tick's full travel segment `MyLocation → ProposedLocation` is checked via `FindEnvironmentImpact` BEFORE `SetActorLocation`. Tagged hit ⇒ VeryVerbose log + one-line impact-puff reuse + `Destroy()`. No tagged hit ⇒ movement exactly as before (`bSweep` still false).
  - New `FindEnvironmentImpact` implementation (bottom of file).

No other files touched. `DamageTypes.h/.cpp`, `HandleImpact` (castle 50% scaling, AoE branch, same-team gate), `InitProjectile`, homing/`GetDistanceToTarget` — all byte-identical.

## Detection mechanism (spec choice 1 — FLAGGED)

`UWorld::LineTraceMultiByObjectType` over the full per-tick travel segment, object types **WorldStatic + WorldDynamic**, `bTraceComplex = false`, self ignored; then an O(hit-result) min-scan keeping the NEAREST hit whose actor has tag `"Terrain"` or `"Obstacle"` (exact FNames, file-local statics).

Why this shape:

- **Segment coverage = no tunneling.** The trace spans every unit the projectile would travel this tick (25–50 u at 1500 u/s @ 60/30 fps), so a 50–70 u trunk hull can never be stepped over, at any frame rate.
- **Multi OBJECT-type trace, not a channel trace:** object-type multi traces return EVERY matching primitive along the segment instead of stopping at the first blocker — an untagged wall standing just in front of a tagged hill cannot mask the hill. Channel traces would have needed a re-trace-and-ignore loop.
- **`bTraceComplex = false` is load-bearing twice:** tree/rock conforms carry footprint-only simple hulls (canopy has NO simple collision ⇒ shots through the canopy PASS, ruling 6), while walkable terrain is Use-Complex-Collision-As-Simple, which answers simple queries with its tri-mesh anyway (ruling 3). Complex tracing would have made canopies block arrows — wrong.
- **Pawn object types are not queried** — units/hero never appear in the hit list; §3.0 friendly safety is untouched.
- **Cost:** one line trace + a tag scan over its hits, per projectile per tick. No world iteration, no `TActorIterator`.

## Behavior summary

- In flight, on crossing anything tagged `Terrain` (walkable ground + hills) or `Obstacle` (trees + rocks): `Destroy()` — zero damage, no AoE, no attribution, no VFX-gated damage path. Impact puff + VeryVerbose log (see flagged decision 2).
- **Order (deliberate):** the target reach test runs FIRST each tick (shipped code, untouched). A projectile that reaches its target this tick impacts it exactly as shipped even if the target hugs a hill flank; the environment check only governs the movement segment when the target was NOT reached.
- **Homing vs hills:** when a target moves behind a hill, the projectile chases the current location and legitimately dies on the hillside — that IS the physical high-ground value (ruling 2). Same for the lost-target flight to `LastKnownAimPoint`: it can now end early on terrain (previously it always reached the point; both endings are harmless no-damage despawns).
- **Deliberately unchanged:** walls, buildings, castles and units are untagged ⇒ the tag filter never matches them ⇒ archer-behind-own-wall fly-through, homing, target-overlap damage, §3.0 castle 50% projectile scaling, AoE (Bomb Tower), despawn-on-target — all behavior-equivalent. Until TASK-095 sets the tags in L_Arena, NOTHING observable changes (the legacy ArenaGround slab is untagged).
- **Ruling-2 WATCH restated (spec item 3):** target ACQUISITION stays range-only everywhere — no LOS checks were added to towers or units. A tower may waste shots into a hillside at a target behind it; if Jonathan's playtest reads that as broken, LOS-at-acquisition becomes a follow-up task. Nothing here prevents or prejudges that.

## Flagged decisions for QA

1. **Detection mechanism:** multi object-type segment trace (WorldStatic + WorldDynamic) with tag filter, as above. WorldDynamic is included defensively (a conform placed with Movable mobility keeps object type WorldStatic by default, but a donor with a customized profile might not); it costs nothing when nothing matches.
2. **VFX choice:** I took the spec's one-line-reuse option — `CachedImpactEffect` (NS_Damage donor) spawns at the tagged hit point, PLUS the VeryVerbose log. Rationale: ruling 2's value ("hills and tree trunks BLOCK projectiles") must be readable in play; a silently vanishing arrow reads as a bug. This deviates from the shipped "puff only when damage landed" telegraph gate (TASK-016/020 pattern) — deliberate and confined to the terrain-death path; the damage-path gate itself is untouched. If QA rules the §3.8 telegraph semantics must stay pure (puff ⇒ damage), deleting the 4-line `if (CachedImpactEffect)` block makes it a silent despawn with the log — no other change needed.
3. **Trace ordering trust:** I do an explicit nearest-hit min-scan over `Hit.Time` instead of assuming the engine returns distance-sorted hits. Belt-and-braces; only affects which tagged actor's ImpactPoint hosts the puff.
4. **`bTraceComplex = false`** (see mechanism) — QA should confirm this matches the art contract: TASK-092 must strip canopy/full-mesh collision (footprint hulls only) and TASK-091 must set Use-Complex-Collision-As-Simple on walkable terrain, else canopies won't block (correct per law) but a terrain mesh WITHOUT complex-as-simple would only collide via whatever simple hulls it has (hulls cannot carry hills — the law already mandates complex-as-simple for exactly this reason).
5. **Start-penetrating hits are honored:** a segment that STARTS inside a tagged primitive (e.g., a previous tick ended flush against a trunk) reports a distance-0 hit and dies there — correct, and part of the no-tunneling guarantee. Side effect: a shooter standing ON a hill firing steeply downhill can clip the crown edge — physical high ground working as ruled, but worth an eye at the M4.5 playtest.

## Assets/tags referenced

- Actor tags `Terrain` and `Obstacle` — exact strings, character-for-character per CONVENTIONS "Arena terrain & environment (M4.5)"; set on level instances by TASK-095. No asset paths referenced (the VFX reuse is the already-cached `ImpactEffect` soft ref, unchanged).

## What QA should scrutinize

- Tag strings vs CONVENTIONS (must be `Terrain` / `Obstacle`, no pluralization/casing drift).
- That the target reach test and everything in `HandleImpact` is untouched (diff should show Tick's movement block + new helper only).
- Shadow scan (C4457/58/59): new locals are `ProposedLocation`, `EnvironmentHit`, `World`, `ObjectParams`, `QueryParams`, `Hits`, `NearestTaggedHit`, `Hit`, `HitActor` + the two static FNames — none collide with member names.
- `SCENE_QUERY_STAT` + 4-arg `FCollisionQueryParams` ctor usage (canonical engine pattern) and the const-correctness of the helper (`LineTraceMultiByObjectType` is const on UWorld).
