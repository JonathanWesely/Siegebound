# TASK-099 Handoff — Freeze + combat-buff APIs on units & buildings (C++)

- author: gameplay-programmer
- date: 2026-07-08
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)
- milestone: M5 (spell system + Set III), file wave 1
- law: M5 rulings 5, 6, 14; CONVENTIONS "Spells & Set III (M5)"

## Files touched (2 pairs, 4 files — NOTHING else)

1. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — `ApplyFreeze` / `IsFrozen` / `ApplyCombatBuff` public API + 2 BattleCry mechanic UPROPERTYs + 2 magnitude getters; 5 private helpers; 7 private members; doc updates on `ApplyMoveSpeedBuff` / `FreezeAI`.
2. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — bodies; composition refactor inside `ApplyMoveSpeedBuff`/`EndMoveSpeedBuff`; additive lines in `EndPlay`, `FreezeAI`, `UpdateState`, `PerformAttack`, `PerformHeal`, `EnterAttack`, `StartAttackLunge`.
3. `Source/GitClaudeUnrealTest/Siegebound/Building.h` — `ApplyFreeze` / `IsFrozen` public API; `EndPlay` override decl; private `EndSpellFreeze` + `bSpellFrozen` + `SpellFreezeTimerHandle`.
4. `Source/GitClaudeUnrealTest/Siegebound/Building.cpp` — bodies; new `TimerManager.h` include.

**NOT touched (deliberate, ruling 14):** `Tower.h/.cpp` (TASK-101 owns the fire-gate), `SpellLibrary.*` (TASK-098), `SiegePlayerController.*` (TASK-093/100), `MinerUnit.*`, `Castle.*` (castle gets NO freeze API — and ACastle is not an ABuilding, so nothing leaks), `SiegeGameMode.*`. No TASKBOARD edit, no compile, no Git.

## API surface (exact signatures — TASK-098/101 consume these character-for-character)

### ASummonedUnit (SummonedUnit.h)
```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
virtual void ApplyFreeze(float Seconds);

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
bool IsFrozen() const;                        // spell freeze ONLY; match-end stays IsAIFrozen()

UFUNCTION(BlueprintCallable, Category = "Siegebound|Unit")
void ApplyCombatBuff(float MoveSpeedMult, float AttackSpeedMult, float Seconds);

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
float GetBattleCryMoveSpeedMultiplier() const;   // 1 + BattleCryMoveSpeedBonus  = 1.25
UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
float GetBattleCryAttackSpeedMultiplier() const; // 1 + BattleCryAttackSpeedBonus = 1.5
```

### ABuilding (Building.h)
```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Building")
virtual void ApplyFreeze(float Seconds);

UFUNCTION(BlueprintPure, Category = "Siegebound|Building")
bool IsFrozen() const;                        // TASK-101 gates ALL tower firing on !IsFrozen()
```

### BattleCry call shape for TASK-098 (magnitudes live with the API owner — ruling 6 / Rally precedent)
```cpp
Unit->ApplyCombatBuff(Unit->GetBattleCryMoveSpeedMultiplier(),
                      Unit->GetBattleCryAttackSpeedMultiplier(),
                      Row.EffectDuration);
```
Mechanic UPROPERTYs (ASummonedUnit, Category `Siegebound|Keywords`, both `// GDD §4`):
`BattleCryAttackSpeedBonus = 0.5f`, `BattleCryMoveSpeedBonus = 0.25f`.

## Mechanisms

### Unit spell freeze — separate resumable state, match-end precedence (ruling 5)
- **Separate state:** new `bSpellFrozen` + `SpellFreezeTimerHandle`, fully independent of the permanent match-end `bAIFrozen` latch. `IsFrozen()` reports ONLY the spell state.
- **Pause** (the FreezeAI body minus permanence): clears state + attack timers, cancels any in-flight lunge to the exact cached rest pose, stops Support healing and drops the heal target, `StopMovement`, **and `DisableMovement()` (MOVE_None) on the movement component** — this last one is what makes the pause hold against AMinerUnit's own arrival poll, which re-issues walks on a timer I cannot touch (MinerUnit.* is outside my file set). Parks Idle, clears CurrentTarget/CurrentMoveGoal.
- **Refresh-not-stack:** expiry timer re-armed at `max(GetTimerRemaining, Seconds)` — never additive, never trimmed shorter.
- **Match-end precedence, triple-guarded:**
  1. `ApplyFreeze` no-ops on `bAIFrozen` (also on dead/unbound/non-positive Seconds).
  2. `FreezeAI` now WIPES the spell state: clears `SpellFreezeTimerHandle`, resets `bSpellFrozen` — no expiry can fire post-match.
  3. `EndSpellFreeze` refuses to resume when `bDead || bAIFrozen || !bStatsLoaded` even if somehow invoked.
- **Resume:** `SetDefaultMovementMode()` (exact inverse of DisableMovement) + re-arm the state loop. Deliberately NO synchronous `UpdateState()` (unlike LoadStatsAndStart): (a) preserves AMinerUnit's seal #1 — its `StateCheckInterval` is 0, so this SetTimer CLEARS instead of scheduling, and no stray castle-bound Advance is ever issued to a resumed miner (its own poll re-issues the gold-node walk); (b) no damage is ever applied synchronously from inside the expiry callback. Combat units reacquire within ≤ 0.25 s.
- **Defense-in-depth:** `bSpellFrozen` added to the `UpdateState` / `PerformAttack` / `PerformHeal` gates (the existing bAIFrozen pattern).

### Building spell freeze — STATE ONLY (ruling 14)
- `ApplyFreeze` latches `bSpellFrozen` + arms the expiry at `max(remaining, Seconds)`; `EndSpellFreeze` drops the latch. Nothing else — TASK-101's tower fire path re-checks `IsFrozen()` every shot, so there is nothing to resume.
- Match-end precedence building-side: `FreezeWorldAtMatchEnd`'s `ClearAllTimersForObject` sweep on towers (SiegeGameMode.cpp:281) also clears this expiry → a spell-frozen tower stays `IsFrozen()` until Play Again destroys it (an expiry can never "resume" a silenced tower). On non-tower buildings a post-match expiry only flips the state flag — resumes nothing by construction.
- New `ABuilding::EndPlay` override clears the expiry timer (belt-and-braces); verified ATower/ABarracks/ADeepMine EndPlay all chain `Super::EndPlay`.

### Combat buff — composes with Rally, restores exactly (ruling 6)
- **Attack speed:** effective cadence = `AttackCadence ÷ CombatBuffAttackSpeedMult`, floored at `MinAttackCadence`, via new `GetEffectiveAttackCadence()`. Consumed at BOTH cadence sites: `EnterAttack` (timer rate + FirstDelay cooldown math) and `StartAttackLunge` (0.8× clamp, so the lunge still completes before a hastened next hit). `RearmAttackTimerAtEffectiveCadence()` re-rates a LIVE attack loop on buff apply AND expiry, honoring the `LastAttackTime` cooldown — a mid-Attack unit speeds up immediately and an expiry never leaves a fast loop running. The re-arm never fires synchronously (0.01 s floor) so a buff API call can never re-enter combat code mid-resolve. With no buff active the effective cadence IS the row cadence — M1..M4 units byte-unchanged.
- **Move speed / Rally stacking:** refactored to ONE walk-speed writer, `RefreshComposedMoveSpeed()`: `MaxWalkSpeed = SharedBase × RallyMult × CombatMoveMult`, or EXACTLY the shared base when neither is active. The resting base (`MoveSpeedBuffBaseSpeed`, reused) is captured only while NEITHER speed buff is active — so no ordering of Rally/BattleCry applies, refreshes, or expiries can drift it (TASK-020 discipline). Rally's own magnitude/refresh/restore behavior is unchanged (its multiplier now lives in `MoveSpeedBuffMultiplier` instead of being written inline).
- **Self-refresh non-stacking:** both multipliers written DIRECTLY from the args (never compounded); the single one-shot `CombatBuffTimerHandle` is re-armed. **Stacks with War Banner** trivially: the aura is a damage-output multiplier composed in `ComputeOutputDamage` — different axis, untouched.
- **Guards:** no-op on `bDead || bAIFrozen || !bStatsLoaded`; `Seconds <= 0` = end-now; non-positive multipliers sanitize to exactly 1.
- **Cleanup:** `FreezeAI` calls `EndCombatBuff()` (zero residual at match end — its attack-timer re-arm is a no-op because FreezeAI already cleared that timer); `EndPlay` clears `CombatBuffTimerHandle` + `SpellFreezeTimerHandle` alongside the existing clears. Play Again destroys all units/buildings → EndPlay path covers reset (matches the TASK-042/055 precedent).

## Flagged decisions for QA (please rule)

1. **No frozen visual tint.** Spec allowed "minimal tint" as optional. Implementing one would require either a new material asset or a material-param contract on MI_TeamColor — neither is in my names block, and choosing it is an art call. The NS_Spell_FrostNova burst (TASK-098's VFX contract) is the read. If a tint is wanted, it needs a small art+code follow-up task.
2. **Freeze disables the movement component (MOVE_None), not just StopMovement.** Reason: AMinerUnit's arrival poll (`EnsureWalkingToNode`, MinerUnit.cpp:254) re-issues MoveToActor on its own timer and only gates on `IsAIFrozen()`; MinerUnit.* is outside my file set. MOVE_None makes the pause hold at the component no drive can bypass. Restore is `SetDefaultMovementMode()` (→ walking). Side effect: gravity is off for the freeze window — units are ground-standing so this is inert.
3. **Resume has NO synchronous UpdateState** (≤ 0.25 s reacquire latency) — load-bearing for the miner seal, see mechanism notes. Deviation from the LoadStatsAndStart start pattern; deliberate.
4. **A spell-frozen miner that already ARRIVED keeps its income during the freeze** (income is arrival-latched on the player state; freeze pauses movement/AI/attack per ruling 5, and GDD is silent on income). Also its `UpdateMining` poll keeps ticking (harmless — it cannot move the pawn); an en-route frozen miner can even latch arrival if the freeze caught it inside the ring. Flagging rather than gating: gating would need MinerUnit.* .
5. **Post-freeze attack timing:** the cooldown runs on wall-clock, so a unit frozen 4 s with a 1 s cadence attacks on its first post-resume Attack entry (cooldown long since elapsed). "Freeze pauses cadence" = no attacks DURING the freeze; it does not re-phase the cooldown after. Same rule the existing target-swap cadence gate uses.
6. **Buffs are accepted while spell-frozen** (`ApplyCombatBuff`/`ApplyMoveSpeedBuff`/`SetAuraDamageBonus` gate on match-end freeze only). Cross-team reality: your unit can be enemy-frozen and friendly-buffed simultaneously; the buff's window burns in wall-clock either way. Speed writes while MOVE_None are inert until resume.
7. **`ApplyCombatBuff` (and `ApplyFreeze`) additionally gate on `!bStatsLoaded`** — a pre-bind buff would capture a pre-row walk speed as the episode base (a latent TASK-042 edge I did NOT change on the Rally path, which stays behavior-identical; flagging the asymmetry).
8. **BattleCry magnitude placement:** mechanic UPROPERTYs on ASummonedUnit (`Siegebound|Keywords`, next to Charge/Slayer, `// GDD §4`) + two public multiplier getters, since USpellLibrary cannot read protected members. Rally precedent kept (magnitudes with the API owner; resolver passes them as args). TASK-098 must use the call shape above — NOT hardcode 1.5/1.25.
9. **Stale comment upstream (not mine to touch):** SiegeGameMode.cpp:274 says "the fire loop is the ONLY timer a tower ever arms" — a spell-frozen tower now also carries the freeze-expiry timer. The match-end sweep is `ClearAllTimersForObject`, so the freeze changes nothing about its correctness (both timers die), but the comment's claim is now stale. Recommend a one-line comment touch-up on whichever task next owns SiegeGameMode.cpp.
10. **`ApplyMoveSpeedBuff`/`EndMoveSpeedBuff` internals refactored** (QA-passed TASK-042 code): capture condition widened to "neither speed buff active", write routed through `RefreshComposedMoveSpeed`. External behavior with only Rally in play is value-identical (verified by inspection: capture condition equivalent when no combat buff exists; write is Base × Mult; restore exact base). This is the minimal change that makes ruling 6's "stacks with Rally" true without drift.

## C4458 shadow scan (self-check)
New params: `Seconds`, `MoveSpeedMult`, `AttackSpeedMult` — no members with those names (members are `CombatBuffMoveSpeedMult`/`CombatBuffAttackSpeedMult`). New locals: `RemainingFreeze`, `FreezeSeconds`, `ComposedSpeed`, `EffectiveCadence`, `Now`, `FirstDelay`, `Movement` — none shadows a member/UPROPERTY in ASummonedUnit, ABuilding, or their parents (`Movement`/`Now`/`FirstDelay` already used safely as locals elsewhere in SummonedUnit.cpp). New members (`bSpellFrozen`, `SpellFreezeTimerHandle`, `MoveSpeedBuffMultiplier`, `bCombatBuffActive`, `CombatBuffMoveSpeedMult`, `CombatBuffAttackSpeedMult`, `CombatBuffTimerHandle`, `BattleCryAttackSpeedBonus`, `BattleCryMoveSpeedBonus`) collide with nothing in any subclass (grepped AMinerUnit/ATower/ABarracks/ADeepMine).

## What QA should scrutinize
- **Precedence proof:** the three guards under "Unit spell freeze"; plus building-side reasoning (tower timers all die at match end; non-tower expiry is state-only). Ruling 5's sentence is satisfied at every path I could find — please hunt for one I missed.
- **Zero-drift proof for the composed speed:** base captured only when NO speed buff active; every write is `Base × RallyMult × CombatMult` or exactly `Base`; both end paths reset their multiplier to literal 1 before recomposing.
- **Cadence restore:** `EndCombatBuff` → `RearmAttackTimerAtEffectiveCadence` re-rates a live loop back to the row cadence; `GetEffectiveAttackCadence` returns the plain row value when inactive (no float residue).
- **M1..M4 non-regression:** with no combat buff and no spell freeze, `EnterAttack`/`StartAttackLunge` compute the identical values as before (effective cadence == AttackCadence), the Rally path is value-identical per flagged decision 10, and all new gates add a `bSpellFrozen` that is always false outside M5 spells.
- **Include correctness:** Building.cpp gained `TimerManager.h`; SummonedUnit.cpp already had `TimerManager.h` + `CharacterMovementComponent.h`; `FTimerHandle` in Building.h rides GameFramework/Actor.h (AActor itself carries FTimerHandle members).

## For downstream
- **TASK-098 (resolver):** freeze = `Unit->ApplyFreeze(Row.EffectDuration)` / `Building->ApplyFreeze(Row.EffectDuration)` on ENEMY actors only, hero + castle excluded caller-side (hero has no API; ACastle is not an ABuilding). BattleCry = the call shape above on FRIENDLY units.
- **TASK-101 (tower gate):** `IsFrozen()` is live on ABuilding now; gate BOTH fire paths on `!IsFrozen()`. State-only means the tower's own cadence timer keeps ticking through a freeze — the gate must sit in the fire callback (shots suppressed while frozen resume with the loop's phase; if the design wants the whole loop paused instead, that is TASK-101 scope against this same API).
- **TASK-103 (compile):** no Build.cs change (no new modules).
