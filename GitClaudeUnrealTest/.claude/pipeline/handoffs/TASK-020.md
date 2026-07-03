# TASK-020 Handoff — Footman procedural attack lunge + impact VFX (C++)

- author: gameplay-programmer
- date: 2026-07-03
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files touched

1. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — 3 new UPROPERTYs, `Tick` override, lunge API (`StartAttackLunge`/`StopAttackLunge`/`UpdateLunge`), `GetDistanceToTarget` out-param overload, private lunge/cache state, `UNiagaraSystem` forward decl, class doc updated.
2. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — constructor (tick flags + `AttackImpactEffect` soft default), BeginPlay (rest-pose cache + one-time effect resolve), Attack-exit and death restore points, `PerformAttack` feedback block, new function bodies, distance-helper refactor. New includes: `NiagaraFunctionLibrary.h`, `NiagaraSystem.h`.

**NOT touched (verified/deliberate):** `GitClaudeUnrealTest.Build.cs` — `"Niagara"` is already in `PublicDependencyModuleNames` (TASK-016 owns that edit; verified present before starting). No HeroCharacter, no Castle, no template files, no TASKBOARD edit.

## Assets referenced

- `/Game/Variant_Combat/VFX/NS_Damage.NS_Damage` — READ-ONLY donor, soft-referenced from C++ only, never edited (CONVENTIONS template-donor rule).

## New UPROPERTYs (all `EditAnywhere, Category = "Combat|Feedback"`, spec-exact)

| Property | Type | Default |
|----------|------|---------|
| `AttackLungeDistance` | `float` | `40.f` |
| `AttackLungeDuration` | `float` | `0.3f` |
| `AttackImpactEffect` | `TSoftObjectPtr<UNiagaraSystem>` | `/Game/Variant_Combat/VFX/NS_Damage.NS_Damage` (set in constructor) |

## How each spec point is satisfied

1. **One cycle per cadence hit** — `StartAttackLunge()` is called from `PerformAttack` immediately after `ApplyDamage`. Hits are always >= Cadence apart (`LastAttackTime` gate in `EnterAttack`), and the cycle is clamped to 0.8 × Cadence, so a cycle always completes before the next hit; a defensive mid-cycle restart snaps to Base first and cannot drift.
2. **Local +X, not the mesh's rotation** — the offset is applied to VisualMesh's RELATIVE location, which lives in the parent CAPSULE's axes; `Base + (Offset, 0, 0)` therefore moves along actor forward regardless of the mesh's own -90° import-fix yaw (handoffs/TASK-014.md). `FaceTarget` runs before every hit, so forward points at the target for either team/facing.
3. **Sine-eased out-and-back** — `Offset = AttackLungeDistance * sin(π * elapsed / duration)`: 0 at cycle start, peak at half cycle, 0 at cycle end (see flagged decision 3).
4. **Duration clamp** — `CycleDuration = min(AttackLungeDuration, 0.8f * AttackCadence)`, computed at cycle start from the table-bound Cadence. `AttackCadence` is already floored at `MinAttackCadence` (0.05) from TASK-004, so the clamp alone can never produce a zero-length cycle.
5. **Zero drift** — the BP-authored rest pose is cached ONCE in BeginPlay (post-construction, so BP defaults/construction script — including TASK-010's down-offset — are captured). The mesh's relative location is only ever written as `Base + f(elapsed)` or exactly `Base`. Exact-Base restore paths: cycle end (`UpdateLunge`), leaving Attack (`EnterAdvance` transition branch and `EnterIdle`), death (`HandleDeath`). `StopAttackLunge` is idempotent.
6. **Impact VFX, resolve-once** — `AttackImpactEffect.LoadSynchronous()` exactly once in BeginPlay into `UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> CachedAttackImpactEffect` (hard ref = GC-safe for the unit's lifetime). Per-attack path touches only the cached pointer — zero per-attack loads. Missing asset: one Warning log, no VFX, no crash; cleared-in-BP (`IsNull`) is a silent opt-out.
7. **Contact point** — `GetDistanceToTarget` gained a 3-arg overload returning the closest point on the target's collision (`ActorGetDistanceToCollision`, ECC_Pawn); the existing range re-check in `PerformAttack` now also yields the impact point for free. Fallback when the target has no usable collision: `GetActorLocation()` (spec fallback), same value used for the distance.
8. **Damage numbers/timing unchanged** — `ApplyDamage` call has identical arguments; the only change is capturing its return value. Early-outs, `FaceTarget`, and the `LastAttackTime` stamp are byte-identical. Stats still bind exclusively from DT_Cards; nothing stat-like added to C++.
9. **Null-safety** — lunge no-ops without VisualMesh or an uncached rest pose; VFX no-ops without the cached system; every path compiles and runs with no mesh, no table, and no Niagara asset present.

## Flagged decisions for QA (please rule on each)

1. **`PrimaryActorTick.bCanEverTick` flipped to `true` (with `bStartWithTickEnabled = false`).** TASK-004 mandated the state machine "never per-tick" — that invariant is intact: all gameplay logic remains timer-driven, and `Tick` contains ONLY `UpdateLunge`. Tick is enabled exclusively inside `StartAttackLunge` and disabled on every stop path plus a defensive self-disable in `UpdateLunge`; a unit not mid-lunge ticks zero times. A smooth 0.3 s visual cannot run on the 0.25 s state timer, and a 60 Hz FTimer would be a worse tick. Please confirm this reading of the TASK-004 constraint.
2. **Lunge fires on every EXECUTED cadence hit; the puff only when `ApplyDamage > 0`.** "Deals its cadence hit" = reaches `ApplyDamage` (target alive + in range). The lunge is the swing, so it plays even if the receiver zeroes the damage (e.g. castle destroyed the same tick — next state check stands the unit down anyway); the puff is the impact, so it follows the hero's TASK-016 flagged-decision-1 return-value reading. Alternative (gate both on > 0) would make the unit visibly freeze for the one hit where the receiver zeroes it.
3. **Ease curve: `sin(π·α)`** — zero offset at both ends, peak at half cycle; velocity is highest at the endpoints (punchy jab, eases at contact). The alternative "smoother" curve `0.5·(1−cos(2π·α))` has zero endpoint velocity but reads mushy for an attack. Both are legitimately "sine-eased"; I chose the jab. One-line swap if QA prefers the other.
4. **UPROPERTY flags are spec-exact: no `BlueprintReadOnly`, no `ClampMin` meta** — mirrors accepted TASK-016 flagged decision 4 (spec character-for-character over house style; other floats in this file carry `ClampMin`). Degenerate values are guarded in code instead: duration <= 0 or |distance| ~ 0 skips the cycle; a NEGATIVE `AttackLungeDistance` is allowed and recoils backward (still zero-drift) — flagging in case QA wants it treated as disabled.
5. **Resolve strategy = one `LoadSynchronous` at BeginPlay**, not async streaming. The spec bans a PER-ATTACK sync-load hitch; a single spawn-time resolve matches the existing `CardTableAsset` pattern, and NS_Damage is in practice already resident (BP_HeroCharacter hard-references it after TASK-017), making the call a lookup rather than a disk load. Async would add StreamableManager machinery for an asset that is already loaded.
6. **Damage lands at lunge START, not at the visual apex.** The trigger is post-hoc (after `ApplyDamage`) because the spec forbids touching damage timing; syncing "contact" to the apex would require moving the hit half a cycle, which is exactly what I'm not allowed to do. Blockout-acceptable; M7's skeletal rig owns real contact timing.
7. **`GetDistanceToTarget` refactored to a forwarding 2-arg + new 3-arg out-param overload.** All pre-existing call sites (`UpdateState`, `AcquireTarget`, `FindNearestEnemyCastle`, tie-break) are textually unchanged and behave identically; only `PerformAttack` uses the new overload.

## What QA should scrutinize

- **`PerformAttack` diff** against the "damage numbers/timing unchanged" requirement — highest-risk surface. Only changes: range check moved onto the out-param overload (same math), return value captured, `StartAttackLunge()` + guarded spawn inserted between `ApplyDamage` and the untouched `LastAttackTime` stamp.
- **Tick lifecycle** — no path leaves tick enabled with no active cycle (stop paths + the `UpdateLunge` self-disable).
- **Zero-drift argument** — verify every write to `VisualMesh` relative location is `Base` or `Base + f`, and every Attack-exit path hits a restore (`EnterAdvance` Attack branch, `EnterIdle`, `HandleDeath`, cycle end).
- **Include/plugin correctness** — `NiagaraFunctionLibrary.h` / `NiagaraSystem.h` with `"Niagara"` already in Build.cs (same pair TASK-016 used in HeroCharacter.cpp).

## For integration / downstream

- No editor work needed: defaults activate by themselves once compiled (soft default points at the donor; BP_Unit_Footman needs no new assignments). Designers can tune `AttackLungeDistance`/`AttackLungeDuration`/`AttackImpactEffect` under Details > Combat > Feedback on the BP.
- PIE check per acceptance: footman vs Red castle lunges once per 1.0 s with a puff at the wall contact point; rest pose pixel-identical after 50+ hits; Blue-vs-Red facing both correct; castle HP still falls 12 per 1.0 s.
