# TASK-101 handoff — Crystal Tower chain attack + tower freeze-gate (gameplay-programmer)

**Status:** file work complete, ready-for-qa. FILE WORK ONLY — no compile attempted (batch compile is TASK-103 per the M5 dispatch shape).

## Files touched (complete list)

- `Source/GitClaudeUnrealTest/Siegebound/Tower.h`
- `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp`

Nothing else. Ruling 14 honored in both directions: TASK-099 stayed out of Tower.cpp; this task stayed out of SummonedUnit/Building/SpellLibrary.

## What changed

1. **`ATower()` constructor (new):** sets the `ChainZapEffect` soft path default to `/Game/VFX/NS_ChainZap.NS_ChainZap` (TASK-101 names block; art TASK-108 authors the asset in parallel).
2. **`OnStatsLoaded`:** binds NEW row cells `AttackChainTargets = Row.ChainTargets` / `AttackChainFalloff = Row.ChainFalloff` alongside the existing stat binds; two new data-smell warnings (negative falloff; chain+AoERadius both authored — chain wins, splash ignored); chain rows (and only chain rows) resolve + hard-cache the NS_ChainZap soft ptr once, after the cadence guard (a mute tower loads no VFX), warn-once on load failure.
3. **`ScanAndFire`:** freeze gate first — `if (IsFrozen()) return;` — the SINGLE choke point every tower shot passes through, so projectile AND chain paths are both gated (ruling 14). Then the existing acquire; then `AttackChainTargets > 0 ? FireChainZapAt : FireProjectileAt`.
4. **`IsAcquirableEnemy` (new private helper):** the tower targeting gate (IsValid + §3.7 positive class gate units/hero + per-type liveness + enemy-team-only) factored out of `AcquireTarget`'s loop, checks byte-for-byte identical, so the chain bounce search shares it instead of hand-mirroring (qa/TASK-026 NIT-4 discipline).
5. **`FireChainZapAt` (new):** instant chain zap, no projectile actor (ruling 9). Two phases:
   - *Select:* one fire-time snapshot of live enemies (same gather as AcquireTarget). Chain = primary (the normal nearest-in-Range acquisition, passed in) + up to ChainTargets−1 bounces; each bounce = the **nearest not-yet-hit enemy within `ChainBounceRadius` (350) of the PREVIOUS target** (target-to-target, never tower-to-target; origin-to-origin metric matching the primary acquisition). No candidate in reach ⇒ chain ends short, never a re-search from the tower. Selected targets leave the candidate pool ⇒ no double-hit per zap.
   - *Apply:* in bounce order, hit n (0-indexed) takes `Dmg − n×ChainFalloff` floored at 0 (CrystalTower row ⇒ 15/10/5), via `UGameplayStatics::ApplyDamage(..., USiegeDamageType_Projectile)`; NS_ChainZap spawned at each hit actor's location (null-safe).
6. **Header:** class doc extended (chain + freeze-gate paragraphs); new UPROPERTYs `ChainBounceRadius` (EditAnywhere float = 350, `// GDD §4 Chain, manager-defined (M5 ruling 9)` — mechanic rule, NOT a CSV column), `ChainZapEffect` (soft), `CachedChainZapEffect` (Transient hard cache), `AttackChainTargets`/`AttackChainFalloff` (VisibleInstanceOnly Transient row binds).

## Existing towers unchanged (acceptance item 4)

Arrow/Bomb/Ballista rows author ChainTargets 0 ⇒ chain branch never taken, chain VFX never loaded; the `AcquireTarget` refactor is a pure extraction (gate checks relocated verbatim into `IsAcquirableEnemy`); the freeze gate reads constant-false on a never-frozen tower. AoERadius/MinRange paths untouched.

## Compile-order dependencies (for TASK-103 / build-master)

- `Row.ChainTargets` / `Row.ChainFalloff` — TASK-097. **Already landed** in CardRow.h (verified on disk at handoff time).
- `ABuilding::IsFrozen()` — TASK-099 pinned API. **NOT yet on disk** at handoff time (099 runs parallel). Tower.cpp will not compile until 099 lands; ruling 14 explicitly prescribes coding against the pinned API. Fold both into the TASK-103 batch.

## Flagged decisions (QA: please rule on each)

1. **Bounce selection = NEAREST unhit enemy to the previous target.** Ruling 9 gives the radius but not which enemy wins when several qualify; nearest-first is the deterministic house pattern (AcquireTarget precedent). Ties by iteration order (strict `<`) — practically never observed with float distances.
2. **Bounce class gate = the tower's §3.7 gate (units + hero ONLY; castles/buildings never), shared via `IsAcquirableEnemy`.** GDD's Chain keyword says "nearby targets"; I read the chain as the tower's attack, and towers target units/hero (§3.7). Consequence: a chain can never zap an enemy building/castle. Also consistent with ruling 4's anti-sniping intent.
3. **Two-phase instant resolution:** the whole chain is selected from one fire-time snapshot, then damage applies in bounce order. A mid-chain cascade death (hit 1 kills a Sapper whose suicide blast kills the would-be hit 2) does NOT re-shape the chain — the receiver's dead-gate zeroes that hit (and phase 2 IsValid-guards torn-down actors). Alternative (interleave damage and search) rejected: "INSTANT-hit" reads as one atomic zap.
4. **Bounces may step outside the tower's 800 Range.** Only the primary is range-gated; bounces are gated solely by ChainBounceRadius from the previous target (ruling 9's wording). A 3-chain can reach ~800+2×350 from the tower in the worst case.
5. **NS_ChainZap spawns at EVERY chain hit regardless of damage outcome** (unlike the projectile puff's damage-landed gate): with no projectile to watch, the zap is the attack's only §6 read. Includes floored-to-0 hits (unreachable with the CrystalTower row's 15/5×3).
6. **0-damage floored bounces still count as chain hits** (occupy a slot, get VFX, take no damage — ApplyDamage skipped when HitDamage ≤ 0). Only reachable with data no current row authors (falloff ≥ Dmg and ≥4 ChainTargets).
7. **Attribution upgrade on the chain path:** DamageCauser = the TOWER itself (an ITeamAgent) ⇒ receivers resolve the attacking team via their step-2 chain, so the receiver-side same-team gate ACTIVELY backstops the chain (stronger than the projectile path's unattributable-and-apply; EventInstigator stays the tower's usual null). Deliberate: honest attribution was free here since no projectile intermediary exists.
8. **Freeze-gate semantics = gate-skip, not timer-pause:** the cadence timer keeps looping while frozen; a shot scheduled during the freeze is skipped (tower doesn't even scan) and firing resumes on the first cadence tick after expiry. No partial-cadence credit/debt. This is the literal reading of the spec's "ALL tower firing gates on !IsFrozen()".
9. **Match-end precedence (ruling 5) needs nothing tower-side:** match end silences towers by `ClearAllTimersForObject` (SiegeGameMode::FreezeWorldAtMatchEnd, qa/TASK-027 WARN-1) — the fire loop is GONE, so a spell-freeze expiry can never resume a match-end-silenced tower regardless of freeze state. TASK-101 preserves the two invariants that mechanism relies on: the fire loop is still the ONLY timer a tower ever arms, and OnStatsLoaded is still single-fire. (SiegeGameMode.cpp's comment calling Tower.cpp a "frozen qa-passed contract" is now historical — noting rather than editing; that file is outside my task's file set.)
10. **Shadow-scan (C4457/C4458/C4459) self-check done:** new locals are `PrimaryTarget, TeamAgents, Candidates, ChainHits, BounceRadiusSq, PreviousLocation, NextTarget, NextDistSq, NextIndex, CandidateIndex, HitIndex, HitActor, HitDamage` — none collide with inherited reflected members (no `Team`, `Owner`, `Instigator`, `CardID`, `Target` member exists on this chain, etc.).

## Assets referenced (exact paths, art builds in parallel)

- `/Game/VFX/NS_ChainZap` (object path `/Game/VFX/NS_ChainZap.NS_ChainZap`) — TASK-108. Missing = one warning per chain tower, zaps deal full damage with no visual.

## What QA should scrutinize

- Falloff math vs the 15/10/5 acceptance (phase-2 loop, `HitIndex` 0-based).
- The `AcquireTarget` → `IsAcquirableEnemy` extraction for any behavioral drift.
- The freeze gate's position (after the destroyed guard, before acquisition) and that no second fire entry point bypasses `ScanAndFire`.
- Flagged decisions 2 and 4 (bounce class gate; bounces escaping tower Range) — the two most consequential spec interpretations.
