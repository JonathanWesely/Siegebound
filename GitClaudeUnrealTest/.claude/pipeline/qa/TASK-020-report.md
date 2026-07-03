# QA Report — TASK-020 — Footman procedural attack lunge + impact VFX (C++)

**Verdict: PASS** (0 blockers, 0 majors, 1 warning, 2 nits)

- reviewer: qa-reviewer
- date: 2026-07-03
- reviewed: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` against the TASK-020 spec + names block, M1 manager decisions, CONVENTIONS.md, handoffs/TASK-020.md, handoffs/TASK-004.md (state-machine invariants), handoffs/TASK-014.md (mesh -90° yaw), qa/TASK-004-report.md (M1 baseline), qa/TASK-016-report.md (precedent rulings), GitClaudeUnrealTest.Build.cs, HeroCharacter.cpp (include/usage consistency)

## Findings

### Blockers
None.

### Majors
None.

### Warnings
- [WARN-1] SummonedUnit.cpp:90-94 + 114-120 — **Re-BeginPlay after `EndPlay(RemovedFromWorld)` mid-lunge would permanently pollute the cached rest pose.** BeginPlay re-caches `VisualMeshBaseRelativeLocation` unconditionally; EndPlay clears the timers but does NOT call `StopAttackLunge()`. If this actor's level ever streams out mid-cycle and back in, the mesh is frozen at `Base + offset`, BeginPlay recaptures that as the new Base, and the drift becomes permanent. **Unreachable in M1** (L_Arena is a single persistent level, no streaming; units are short-lived and die via `HandleDeath`, which restores), so not a blocker — the acceptance-relevant paths are all clean. Fix on next touch, either line works: guard the cache with `if (!bVisualMeshBaseCached)` or add `StopAttackLunge()` to EndPlay. (Note the same stream-out scenario already breaks the TASK-004 state machine — timers cleared, never restarted, `bStatsLoaded` early-out — so this is a latent-class issue, not a TASK-020 regression.)

### Nits
- [NIT-1] SummonedUnit.cpp:451-456 — `EnterIdle`'s `State == Idle` early return skips the `StopAttackLunge()` at :461. Currently unreachable with an active lunge (proof: `bLungeActive` can only become true inside `PerformAttack`, which only executes while `State == Attack` — timer set exclusively in `EnterAttack` after `State = Attack`, cleared on every Attack exit; and every Attack→Idle transition takes the non-early-return path). Hoisting `StopAttackLunge()` above the early return would make the restore invariant local instead of depending on that global argument. Do NOT respin for this.
- [NIT-2] SummonedUnit.cpp:47-48, 577-580 — `bCanEverTick` is now `true`, so a future BP child can visually wire Event Tick — but `bStartWithTickEnabled = false` plus `UpdateLunge`'s defensive `SetActorTickEnabled(false)` (and every `StopAttackLunge`) will silently keep/turn it off. Correct for this task's contract; recorded as a footgun note for M2+ unit-type work so nobody debugs a "dead" BP Tick.

## Flagged-decision rulings (handoffs/TASK-020.md — all seven ruled)

1. **`bCanEverTick = true` with `bStartWithTickEnabled = false`, tick alive only during a lunge cycle — ACCEPTED.** The TASK-004 invariant ("state machine never per-tick") reads on gameplay logic, and that is intact: `Tick` (cpp:528-534) contains ONLY `Super::Tick` + `UpdateLunge`; every state/target/damage decision remains timer-driven and untouched. Verified the full tick lifecycle: enabled in exactly one place (`StartAttackLunge`:559, always paired with `bLungeActive = true`), disabled on cycle end (:586 via `StopAttackLunge`:572), on every Attack exit (:404, :461), on death (:685), plus the defensive self-disable for a stray tick (:577-579). A unit not mid-lunge ticks zero times; a 60 Hz FTimer would indeed be a worse tick. Confirmed reading of the TASK-004 constraint.
2. **Lunge on every EXECUTED cadence hit; puff only when `ApplyDamage > 0` — PASS.** Consistent with qa/TASK-016-report.md ruling 1 and its WARN-1 receiver contract (both current receivers verified there to return 0 for every ignored hit — the forward-looking M2 CONVENTIONS note is already on record; no need to duplicate it). The swing-vs-impact split is the right semantic: gating the lunge on the return value would visibly freeze the unit for the one hit a receiver zeroes, and `StartAttackLunge` touches no damage state either way.
3. **Ease curve `sin(π·α)` — ACCEPTED.** The spec says "sine-eased"; this qualifies, and it analytically guarantees exact-zero offset at both endpoints (drift-proof independent of the restore paths, which also exist). The punchy-jab-vs-mushy call is a legitimate blockout aesthetic choice; designers can retune in M7. No change.
4. **Spec-exact UPROPERTY flags (no `BlueprintReadOnly`, no `ClampMin`); negative `AttackLungeDistance` = backward recoil — ACCEPTED, including the negative case as-is.** Mirrors the already-accepted TASK-016 ruling 4 (spec character-for-character over house style). Degenerate guards verified in code: `CycleDuration <= UE_KINDA_SMALL_NUMBER || IsNearlyZero(AttackLungeDistance)` skips the cycle (:547-551). A negative distance stays zero-drift — the sine offset is sign-symmetric and every restore path writes exactly `Base` regardless of sign — and the property doc (:171-176) documents the recoil behavior. Do not add "treat negative as disabled" logic without a spec change.
5. **Single `LoadSynchronous` at BeginPlay — PASS.** The spec bans a PER-ATTACK sync-load hitch, which this satisfies exactly; it matches the accepted `CardTableAsset` pattern, and NS_Damage will already be resident via BP_HeroCharacter's hard ref (TASK-016 ruling 6, TASK-017). The cache is `UPROPERTY(Transient) TObjectPtr` (h:327-328) — GC-safe hard ref for the unit's lifetime. Failed-load warning (:103-108) and `IsNull` silent opt-out (:100) verified. Async StreamableManager machinery would be over-engineering here.
6. **Damage lands at lunge START, not the visual apex — ACCEPTED.** Forced by the task's own hard constraint: syncing contact to the apex would move the hit by half a cycle, i.e., exactly the damage-timing change the spec forbids. Blockout-acceptable; M7's skeletal rig owns real contact timing.
7. **`GetDistanceToTarget` refactor (forwarding 2-arg + new 3-arg out-param overload) — PASS.** All five pre-existing call sites verified textually unchanged and still on the 2-arg form (leash :227, attack-range gate :254, acquisition :297, tie-break :333, castle scan :353); the 2-arg forwards to the 3-arg (:728-732) with identical math — same `ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint)`, same `< 0` origin-distance fallback (:748-756). Only `PerformAttack` (:485) uses the new overload. Distinct arities, no overload ambiguity.

## Priority 1 — damage timing/numbers audit (highest-risk surface)

`PerformAttack` (cpp:468-511) against the M1 baseline (qa/TASK-004-report.md "State machine" + "Damage attribution" sections, handoffs/TASK-004.md §State machine):

- Early-outs unchanged: `bDead || !bStatsLoaded` (:470-473); `!IsTargetAlive(Target)` (:476-479); range gate `> AttackRange` (:485-488) — same closest-point-on-collision math, now merely also yielding the contact point through the out-param.
- `FaceTarget(Target)` before the hit — unchanged position (:490).
- `ApplyDamage(Target, AttackDamage, GetController(), this, UDamageType::StaticClass())` (:495) — argument-for-argument identical to the M1 call QA-004 verified (castle attribution contract intact via both resolution steps); the ONLY change is capturing the return value into a const local, which has no behavioral effect.
- `LastAttackTime` stamp (:507-510): same unconditional stamp at the end of an executed hit; the two inserted statements (`StartAttackLunge()`, guarded `SpawnSystemAtLocation`) sit between ApplyDamage and the stamp and are synchronous, same-frame, and touch no damage/cadence state. Cadence gating in `EnterAttack` (:378-396) — `FirstDelay = Cadence − (Now − LastAttackTime)`, immediate hit + looping timer — is untouched.
- Feedback is provably additive: `StartAttackLunge` writes only visual/tick members; `SpawnSystemAtLocation` spawns a fire-and-forget Niagara component; neither can re-enter the state machine, kill the unit, or alter any stat.
- §3.0: no new stat-like values — `AttackLungeDistance`/`AttackLungeDuration` are visual tuning, not card stats; grep confirms Footman's 80/12/120/1.0/400 still appear only in comments; stats still bind exclusively in `LoadStatsAndStart` from `/Game/Data/DT_Cards`.

Caveat on method: QA has no Git access, so "byte-identical" was verified structurally against the TASK-004 QA report and handoff (which describe every M1 hunk), not via literal diff. Build-master: a `git diff` of SummonedUnit.h/.cpp should show ONLY the hunks listed in handoffs/TASK-020.md §Files touched — flag anything else back to QA.

## Priority 2 — zero-drift proof

- Base cached ONCE at BeginPlay (:90-94), post-construction — BP defaults and construction scripts (TASK-010's down-offset) have run for placed, plain-spawned, and deferred-spawned actors alike.
- Every write to VisualMesh relative location is one of exactly three: `Base` (StartAttackLunge snap :555, StopAttackLunge restore :568), or `Base + (Distance·sin(π·α), 0, 0)` (:596-597). No read-modify-write anywhere — drift is structurally impossible, not just numerically unlikely.
- Restore coverage: cycle end (`UpdateLunge` :584-588, overshoot-safe — checked BEFORE writing an offset, so a hitch frame restores instead of overshooting); leaving Attack → Advance (:404); leaving Attack → Idle (:461); death (:685, before `Destroy()`); `StopAttackLunge` idempotent and null-safe (skips the write if the mesh is gone, still clears state).
- Mid-cycle restart (defensive only — cycle ≤ 0.8×Cadence < Cadence-spaced hits, so it cannot occur in practice): snaps to Base first (:555), resets elapsed — no accumulation.
- Cadence edge cases: Cadence ≤ 0 in CSV → floored to 0.05 (M1 code, :188) → cycle = min(0.3, 0.04) = 0.04 s, still < hit spacing; huge Cadence → cycle = AttackLungeDuration; duration ≤ 0 or ~0 distance → guarded skip, no state touched, no tick enabled.
- `bLungeActive ⇒ State == Attack` invariant verified (see NIT-1 proof), so no lunge can survive into Advance/Idle unrestored.
- Sole exception is the exotic stream-out path — WARN-1, unreachable in M1.

## Priority 3/4 — tick lifecycle & lunge direction

- Tick: covered in ruling 1 — constructor flags exact (:47-48), single enable point always paired with an active cycle, disable on every stop path plus the stray-tick self-disable. No path leaves tick running without `bLungeActive`.
- Direction: offset applied to the RELATIVE location of VisualMesh, whose parent is the capsule — the `(Offset, 0, 0)` vector lives in capsule-local axes, i.e., actor forward, and is mathematically independent of the component's own -90° import-fix yaw (handoffs/TASK-014.md). `FaceTarget` runs in `EnterAttack` (every state check) and in `PerformAttack` immediately before the lunge starts, so +X points at the target for either team and any approach angle; a mid-cycle actor rotation rotates the offset with the unit (correct). Impact point = closest point on target collision to the unit, actor-location fallback (:748-756) — spec-exact.

## Priority 6 — compile risk (UE 5.8, nothing compiled yet)

- Build.cs: `"Niagara"` present in `PublicDependencyModuleNames` (Build.cs:22, landed by TASK-016 as owned); this file correctly NOT touched by TASK-020.
- Includes match TASK-016's accepted pair exactly: `NiagaraFunctionLibrary.h` + `NiagaraSystem.h` (cpp:19-20 vs HeroCharacter.cpp:18-19) — plugin-standard flat paths, valid in 5.8.
- `UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CachedAttackImpactEffect, ImpactPoint)` — matches the current static signature (world context + system + location, defaults for the rest); `TObjectPtr` decays to the raw pointer param. Same 3-arg usage as HeroCharacter.cpp:241.
- Header: `UNiagaraSystem` forward decl (h:13) is sufficient for both `TSoftObjectPtr` UPROPERTY (h:195) and `TObjectPtr` cache (h:328); `UObject/SoftObjectPtr.h` included; `Tick(float DeltaSeconds)` override signature matches AActor.
- `UE_KINDA_SMALL_NUMBER` / `UE_PI` are the current UE5-prefixed core constants; `SetActorTickEnabled`, `SetRelativeLocation`, `GetRelativeLocation` all current. No deprecated API found.
- Constructor soft-path defaults via `FSoftObjectPath` (:74-79) — same accepted pattern as `CardTableAsset`; donor path `/Game/Variant_Combat/VFX/NS_Damage.NS_Damage` is character-exact to the names block and referenced-only (template-donor rule respected).
- Conventions: property names `AttackLungeDistance` / `AttackLungeDuration` / `AttackImpactEffect` and Category `"Combat|Feedback"` are character-for-character the spec's names block; `TObjectPtr` per CONVENTIONS; all new state is either UPROPERTY (UObject refs) or POD (FVector/bool/float — no GC concern).

## Notes for build-master

1. **Compile TASK-016 + TASK-018 + TASK-020 as one batch** — TASK-020's Niagara includes only link because TASK-016's Build.cs edit is in the working tree, and UBT globs all of Source/ anyway (see qa/TASK-016-report.md note 3 on in-progress files). All three are now qa-passed, so a single editor-bounce compile covers them.
2. Git-diff sanity check per the audit caveat above: SummonedUnit.h/.cpp changes must match handoffs/TASK-020.md §Files touched exactly; Build.cs must show only TASK-016's Niagara line.
3. No editor assembly needed for this task: the donor soft default self-activates post-compile; BP_Unit_Footman needs no new assignments. PIE spot-check per acceptance: footman vs Red castle lunges once per 1.0 s with a puff at the wall, castle still falls 12 HP per hit, rest pose exact after sustained attacking, both facings correct.
4. WARN-1 and the nits require no action this cycle; fold WARN-1's one-line guard into the next SummonedUnit.cpp touch (same convention as the QA-002 collision-profile WARN).
