# QA Report — TASK-189

**Task:** Attack/death anim trigger for rigged units (code-only, follow-up to TASK-165/162)
**Files reviewed:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}`, `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h`
**Handoff:** `.claude/pipeline/handoffs/TASK-189-programmer.md`
**Nature:** correctness/behavioral review (build-master already confirmed a clean compile — no recompile needed)

Verdict: **PASS**  (0 BLOCKER / 2 WARN / 3 NIT)

## Findings

### Blockers
None.

### Scrutiny results (the 5 risk areas)

**1. Locomotion restore reliability — SOUND.**
Every path out of single-node attack mode restores the locomotion ABP:
- The one-shot `AttackAnimRestoreTimerHandle` is re-armed on *every* attack clip (`PlaySkeletalAttackAnim`, cpp:344-345), so even a unit stuck in Attack state with an unreachable target (PerformAttack early-returns out of range at cpp:1549 without re-arming) still restores when the last-armed timer fires.
- `RestoreLocomotionAnim` is also called on every Attack exit: `EnterAdvance` (cpp:1462, gated on `State==Attack`), `EnterIdle` (cpp:1520), and `FreezeAI` (cpp:463). `UpdateState`/`UpdateStateSiege` only ever leave Attack through `EnterAdvance`/`EnterIdle`/`EnterAttack`/`Detonate→HandleDeath` — no unguarded transition exists. Support (Cleric) never enters Attack, so it never plays an attack clip.
- **Engine-verified the load-bearing claim:** `USkeletalMeshComponent::SetAnimInstanceClass` (UE 5.8, SkeletalMeshComponent.cpp:3700) re-initializes when `(NewClass != AnimClass || !bWasUsingBlueprintMode)`. Coming out of single-node mode `bWasUsingBlueprintMode` is false, so re-init is forced *even when the locomotion class is unchanged* — the restore genuinely works. The redundant same-class-already-in-BP-mode call cheaply no-ops, so the unconditional `EnterIdle` restore and range-edge churn are safe.
- `LocomotionAnimClass` (and the cached clips) are set in `ResolveSkeletalVisual`/`CacheActionAnimations` (cpp:281,284) inside `LoadStatsAndStart` **before** `bStatsLoaded=true` and the first `UpdateState` (cpp:967-971). `PerformAttack` is gated on `bStatsLoaded` (cpp:1532). Caching-before-first-use is guaranteed.
- The no-ABP pathological rig is handled: `PlaySkeletalAttackAnim` is gated on `LocomotionAnimClass` (cpp:331), so a rig with no ABP never enters single-node mode (can't cleanly return) — keeps its ref pose. Correct.

**2. Repeated/continuous attacks — SOUND.** A faster next attack re-`SetTimer`s the *same* one-shot handle (replaces, never stacks/leaks) and `PlayAnimation` restarts the clip. Continuous swings while attacking; ABP returns only once attacks actually stop. No double-timer.

**3. Death-hold correctness — SOUND.** The §6 gold-burst + final `OnHPChanged(0)` broadcast fire in `HandleDeath` *before* the defer (cpp:1989-1995), so death still reads on both the hold and immediate-destroy paths. `bDead`/collision-off/bar-hidden all set before the deferred `Destroy` (cpp:2027-2037). I traced every live-unit iterator that a 2s-lingering corpse could reach — all filter the dead: `AcquireTarget`/`IsTargetAlive` (cpp:1067), `FindNearestDamagedFriendly`/`FindNearestFriendlyCombatUnit` (cpp:1279,1329), bot army/Fireball/Lightning scans (SiegeBotController.cpp:761,827,902), hero Rally (HeroCharacter.cpp:399), match-end `FreezeAI` (early-returns on `bDead`), and the PlayAgain sweep (`Destroy`→`EndPlay` clears the defer timer). `TakeDamage` early-returns on `bDead` (cpp:1851) so a corpse cannot re-trigger death or be double-killed. **No gold-on-kill/bounty system exists** (grep: no `OnKill`/`Bounty`/`KillReward`), so no unit double-counts gold. The **miner** override genuinely protects §3.3: `AMinerUnit::ShouldHoldDeathAnim()==false` short-circuits `PlaySkeletalDeathAnim` entirely (`HandleDeath` cpp:2015), so the miner is destroyed immediately and its `EndPlay` income/cap-slot bookkeeping stays prompt. No other unit holds a resource on death (Sapper detonation runs before the defer with its `bDetonated` single-blast guard; buildings/towers are a separate class untouched by this task).

**4. Interactions — SOUND.** Death during an attack: `HandleDeath` clears the restore timer (cpp:1999) and `PlaySkeletalDeathAnim` re-clears it and overrides the in-flight attack clip with the death clip (cpp:376-377) — the death pose wins, no double-trigger. Swarm (MilitiaMob) units are independent actors with no shared anim state. A unit that dies before its restore timer fires: `HandleDeath` + `EndPlay` clear `AttackAnimRestoreTimerHandle` and `DeathDestroyTimerHandle`, so no dangling timer on a destroyed actor.

**5. Null-safety / lifetime — SOUND.** Every trigger no-ops on missing skeletal runtime / clip / (attack) ABP. Both timers cleared in `EndPlay` (cpp:399-400). `CachedAttackAnim`/`CachedDeathAnim` are `UPROPERTY(Transient) TObjectPtr` and `LocomotionAnimClass` is `UPROPERTY(Transient) TSubclassOf` — all GC-alive. `TSoftObjectPtr` locals in `CacheActionAnimations` are brace-init'd `{ FSoftObjectPath(...) }` (cpp:316,320 — vexing-parse guard). Complete-type includes present: `Animation/AnimSequence.h` (GetPlayLength + the `UAnimSequence*`→`UAnimationAsset*` upcast in `PlayAnimation`), `Animation/AnimInstance.h`, `Components/SkeletalMeshComponent.h`, `GameFramework/CharacterMovementComponent.h`. No shadowing of inherited reflected members.

### API validity (UE 5.8)
- `USkeletalMeshComponent::PlayAnimation(UAnimationAsset*, bool)` — NOT deprecated (SkeletalMeshComponent.h:1258). `meta=(UnsafeDuringActorConstruction)` — only ever called at runtime (PerformAttack/HandleDeath), never construction. OK.
- `SetAnimInstanceClass(UClass*)` — NOT deprecated (SkeletalMeshComponent.h:1096); the deprecated forms are `K2_SetAnimInstanceClass` / the 5.5-deprecated overload, neither used. OK.
- `UAnimSequenceBase::GetPlayLength()` — current accessor (AnimSequenceBase.h:86). OK.
- `SetAnimInstanceClass` refuses (warns, no-ops) if called during anim evaluation — all callers are timer/state-tick callbacks, never anim notifies. OK.

### Header/cpp consistency & UE correctness
`ShouldHoldDeathAnim()` base `virtual ... {return true;}` (h:475) + `AMinerUnit` `override {return false;}` (MinerUnit.h:119) — const-correct, plain C++ virtual (no reflection needed). All new helpers declared and defined. `DeathAnimMaxHoldSeconds` `UPROPERTY(EditAnywhere, ClampMin=0)`. `MinAttackCadence` is a file-scope anonymous-namespace `constexpr` (cpp:72) visible to `PlaySkeletalAttackAnim`. OK.

## Non-blocking findings

- [WARN] SummonedUnit.cpp (`EnterAttack`/`EnterAdvance` at the range edge) — Attack↔Advance flapping drives ABP↔single-node churn; each *actual* switch runs `ClearAnimScriptInstance` + `InitAnim`. Bounded by the 0.25s state cadence and only on real mode changes (not per-frame), so no perf hazard. Programmer-flagged, acceptable blockout-tier. No action required.
- [WARN] SummonedUnit.cpp `ApplyFreeze` (cpp:626) — a FrostNova-frozen rigged unit mid-swing does not hold its swing pose: its already-armed restore timer still fires during the freeze and swaps to the (velocity-0) idle locomotion pose. Cosmetically fine (thematically even reasonable) and self-corrects on unfreeze via the re-armed state loop; FrostNova visuals are out of TASK-189 scope. No action required.
- [NIT] Death pose held ≤2s off-world on a KillZ fall routed through `HandleDeath` — cosmetic, programmer-flagged.
- [NIT] Repeated-clip restart for a fast attacker whose clip length > cadence restarts the clip from frame 0 each cadence (never completes) — intended "continuous swings," reads fine at blockout tier.
- [NIT] Hero attack/death anim remains a flagged fast follow-up (asset wiring on `BP_HeroCharacter`, not code) — correctly out of scope; the hero already has a working montage-based trigger that the single-node approach would regress.

## Notes for build-master (if PASS)
- Binaries already built (build-master PASS on the compile). This is a behavioral clear only — **no recompile required**; proceed to assemble/commit.
- No asset or scene changes in this task. The triggers are null-safe against un-imported clips, so a commit is safe even if any `A_<CardID>_{Attack,Death}` clip is missing for a given card (that card simply plays no swing/death anim — never a crash).
- Runtime PIE spot-checks worth a human glance at Jonathan's next playtest (not commit gates): a rigged unit visibly swings on the attack tick and returns to walk/idle after; dies into its held death pose ~2s then vanishes; a miner still vanishes immediately on death (income/cap-slot released promptly); static/non-rigged units keep the TASK-020 lunge + immediate destroy unchanged.
