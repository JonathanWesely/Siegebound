# QA Report — TASK-099 — Freeze + combat-buff APIs on ASummonedUnit and ABuilding
Verdict: **PASS**

- reviewer: qa-reviewer, 2026-07-08 (pre-compile review; UE 5.8 API check included)
- files reviewed in full: SummonedUnit.h/.cpp, Building.h/.cpp
- cross-reads: MinerUnit.cpp, SpellLibrary.cpp (TASK-098), Tower.cpp/.h (TASK-101), Barracks.cpp/.h, DeepMine.cpp/.h, SiegeGameMode.cpp (FreezeWorldAtMatchEnd + PlayAgain), handoffs/TASK-042.md, CONVENTIONS "Spells & Set III (M5)", M5 rulings 5/6/14
- blockers: 0 · warnings: 2 · nits: 3

## Findings

- [WARN] SpellLibrary.cpp:23-38, 363, 367-370 (TASK-098's file, NOT 099's) — **BattleCry magnitude seam, ruled here**: TASK-098 hardcodes `BattleCryMoveSpeedMultiplier = 1.25f` / `BattleCryAttackSpeedMultiplier = 1.5f` as namespace constants and passes them at line 363 instead of calling TASK-099's getters. TASK-099's side is the spec-compliant one — M5 ruling 6 and CONVENTIONS M5 pin the magnitudes as "mechanic-rule UPROPERTYs (Rally precedent, GDD §4 comments)", which a static UBlueprintFunctionLibrary cannot carry; the UPROPERTYs + public getters on ASummonedUnit (SummonedUnit.h:379-384, 237-243) are exactly that. **Reconciliation (route to gameplay-programmer as a TASK-098 fix BEFORE TASK-103 compiles):** inside the ResolveAllyBuff loop replace line 363 with `Unit->ApplyCombatBuff(Unit->GetBattleCryMoveSpeedMultiplier(), Unit->GetBattleCryAttackSpeedMultiplier(), Row.EffectDuration);` (per-unit getters are semantically correct — the magnitudes are EditAnywhere and tunable per class); delete the two namespace constants at lines 37-38 (and their doc block's constants claim); adjust the log line at 367-370 to print the per-unit multipliers (or drop the magnitudes from the log). Note: the current code COMPILES fine — this is a law/tuning break (designer edits to the UPROPERTYs would be silently ignored), not a compile break, hence WARN on 098's ledger rather than a 099 blocker.
- [WARN] Building.cpp:76-113 — no building-side match-end cast latch: ABuilding has no bAIFrozen equivalent, so an `ApplyFreeze` call arriving AFTER `FreezeWorldAtMatchEnd`'s tower sweep would arm a fresh expiry timer on a silenced tower. Consequence today is inert (expiry only drops the latch; the fire loop is already dead; Play Again destroys the building), and no caller currently exists post-match (end screen is UIOnly, bot frozen). Carry-forward, not a 099 defect: TASK-100's targeting mode must cancel/refuse confirms at match end (its QA should verify), and TASK-109's PIE pass should confirm no post-match resolve path exists.
- [NIT] SiegeGameMode.cpp:273-274 — stale comment (programmer's flagged decision 9, confirmed): "the fire loop is the ONLY timer a tower ever arms" is no longer true (spell-freeze expiry). Correctness unaffected — the sweep is `ClearAllTimersForObject` (line 281), which kills both. One-line comment touch-up owed by the next task that owns SiegeGameMode.cpp.
- [NIT] Building.cpp:279 — pre-existing TODO `"TODO(Spell 50% — M5): spell damage types vs buildings"` is now WRONG per M5 ruling 3: buildings take FULL spell damage (the 50% is `ACastle::TakeDamage` ONLY — Lightning must kill an Arrow Tower). No code change is pending here, so the TODO misleads; delete it on the next task that owns Building.cpp (TASK-099 wasn't asked to and correctly left TakeDamage untouched).
- [NIT] SummonedUnit.cpp:267-312 — Rally's `ApplyMoveSpeedBuff` still lacks the `!bStatsLoaded` gate that `ApplyCombatBuff`/`ApplyFreeze` carry (programmer's flagged decision 7). Pre-existing latent TASK-042 edge, deliberately unchanged to keep the refactor value-identical — correct call. Add the gate on the next task that owns the Rally path.

## Rulings on the 10 flagged decisions

1. **No frozen visual tint — ACCEPTED.** Spec made it optional; a tint needs a material contract not in the names block (art call). NS_Spell_FrostNova (TASK-098's VFX contract) is the read. If design wants a persistent 4 s frozen read, it is a small art+code follow-up task (M7 candidate).
2. **MOVE_None (DisableMovement) instead of StopMovement-only — ACCEPTED (load-bearing).** Verified against MinerUnit.cpp: `UpdateMining` (poll gated only on `IsUnitDead()||IsAIFrozen()`) re-issues `MoveToActor` via `EnsureWalkingToNode` (lines 254, 294) on its own timer, and MinerUnit.* is outside the file set — the component-level disable is the only in-scope lever that holds. `SetDefaultMovementMode()` is the exact inverse (→ walking). Side effect (gravity off for the window; a unit frozen mid-fall would hang, then resume falling on restore) is inert for ground-standing units — accepted. The frozen miner's poll re-issuing dead MoveToActor requests every 0.25 s for ≤4 s is trivially cheap.
3. **No synchronous UpdateState on resume — ACCEPTED (load-bearing twice over).** (a) Miner seal #1 preserved: `StateCheckInterval = 0` (MinerUnit.cpp:41) makes EndSpellFreeze's `SetTimer` CLEAR instead of schedule — no stray castle-bound Advance ever reaches a resumed miner; its own poll re-issues the gold-node walk. (b) A synchronous UpdateState could reach EnterAttack's immediate-`PerformAttack` branch, i.e. damage applied synchronously from inside a timer callback — correctly avoided. ≤0.25 s reacquire latency is the state machine's native granularity.
4. **Frozen arrived miner keeps income; en-route miner can latch arrival inside the ring — ACCEPTED.** GDD is silent; income is arrival-latched on the player state; gating would require MinerUnit.*/SiegePlayerState edits outside the file set. Carry-forward: if playtest wants freeze to pause miner income, that is a new task on MinerUnit.* — do not hack it into this API.
5. **Post-freeze attack cooldown runs on wall-clock (no re-phase) — ACCEPTED.** "Freeze pauses cadence" is satisfied as no-attacks-DURING (timer cleared + `bSpellFrozen` gates in UpdateState/PerformAttack/PerformHeal); the post-resume first hit passes through EnterAttack's existing `LastAttackTime` gate — identical to the shipped target-swap rule. Consistent.
6. **Buffs accepted while spell-frozen — ACCEPTED.** Freeze and buff come from opposite casters; windows burn wall-clock. Verified safe: speed writes are inert under MOVE_None until resume, and `RearmAttackTimerAtEffectiveCadence` no-ops while frozen (attack timer cleared by ApplyFreeze), so a buff can never start an attack loop mid-freeze; a buff expiring mid-freeze likewise re-arms nothing.
7. **`!bStatsLoaded` gate on ApplyCombatBuff/ApplyFreeze, Rally left asymmetric — ACCEPTED.** The new APIs are stricter (correct); the Rally path stays behavior-identical per the refactor mandate. NIT-3 records the follow-up.
8. **BattleCry magnitudes as UPROPERTYs + public getters on ASummonedUnit — ACCEPTED, and ruled the authoritative side of the 098 seam** (see WARN-1). Placement matches ruling 6, CONVENTIONS M5 mechanic-rules law, and the Charge/Slayer siblings (`Siegebound|Keywords`, `// GDD §4`, defaults 0.5/0.25 ⇒ ×1.5/×1.25 — GDD-correct).
9. **Stale SiegeGameMode.cpp:274 comment — ACCEPTED as flagged**; NIT-1 records the carry-forward. Verified the sweep's correctness claim independently: `ClearAllTimersForObject` (line 281) kills the fire loop AND the new freeze expiry, so a match-end-silenced tower stays `IsFrozen()` until Play Again destroys it — precedence intact.
10. **Rally internals refactored value-identically — PASS, verified by inspection.** Capture condition `!bMoveSpeedBuffActive && !bCombatBuffActive` reduces to the old `!bMoveSpeedBuffActive` whenever no combat buff exists (always, pre-M5); every write through `RefreshComposedMoveSpeed` is `Base × RallyMult` (≡ old inline write) or exactly `Base`; both end paths reset their multiplier to literal 1 before recomposing. With only Rally in play the observable sequence is bit-identical to TASK-042.

## Match-end precedence audit (ruling 5 — highest scrutiny)

Triple guard verified: (1) `ApplyFreeze` no-ops on `bAIFrozen|bDead|!bStatsLoaded|Seconds<=0` (SummonedUnit.cpp:393); (2) `FreezeAI` wipes `SpellFreezeTimerHandle` + `bSpellFrozen` (lines 243-244) AND ends the combat buff (line 236, after the attack-timer clear at 217-218 so its re-arm is a no-op); (3) `EndSpellFreeze` refuses resume on `bDead|bAIFrozen|!bStatsLoaded` (lines 462-465). Orderings hunted, no hole found:

- spell-freeze → match-end: FreezeAI wipes state+timer; Rally/aura/combat buffs all ended with exact restores. No expiry fires post-match. Unit stays MOVE_None permanently — inert (parked Idle until destroyed).
- match-end → spell-freeze: refused at the unit's ApplyFreeze gate. Building-side has no latch (WARN-2, caller-prevented, consequence inert).
- Play Again mid-freeze/mid-buff: PlayAgain destroys all units/buildings → `EndPlay` clears `SpellFreezeTimerHandle` + `CombatBuffTimerHandle` (SummonedUnit.cpp:163-164) and Building's expiry (Building.cpp:71). Clean.
- death mid-freeze: TakeDamage still lands on frozen units; HandleDeath → Destroy → EndPlay clears everything. Clean.
- refresh: `max(remaining, Seconds)` on both classes (SummonedUnit.cpp:402-407, Building.cpp:92-100) — never additive, never trimmed. Correct.
- towers: sweep clears fire loop + freeze expiry; frozen tower stays `IsFrozen()` till Play Again — expiry can never "resume" a silenced tower.
- non-tower buildings (Wall/Barracks/DeepMine): their expiry is NOT swept at match end — it fires once post-match, drops the latch, and resumes NOTHING by construction (Building.cpp:103-113 does exactly two things: clear + flag). One-shot, self-clearing — no leak, no resume; satisfies ruling 5's intent. Accepted.
- `IsFrozen()` semantics consistent both sides: spell state only; match-end stays `IsAIFrozen()` (unit) / silenced-tower state (building). Matches the handoff's pinned API character-for-character; ATower::ScanAndFire's `IsFrozen()` call (Tower.cpp:168) lines up with `bool IsFrozen() const`.

## Combat-buff audit (ruling 6)

- **Zero-drift proof holds:** the ONE walk-speed writer (`RefreshComposedMoveSpeed`) writes `SharedBase × RallyMult × CombatMoveMult` or exactly `SharedBase`; base captured only while NEITHER speed buff is active (both apply sites); both end paths reset to literal 1 then recompose. Traced Rally→BattleCry→Rally-end→BattleCry-end and every permutation incl. refreshes: restore is exact, no cycle drifts.
- **Stacking:** Rally × BattleCry compose on walk speed; War Banner is a separate damage-output axis (`ComputeOutputDamage`, SummonedUnit.cpp:1401) — untouched. Ruling 6 satisfied.
- **Self-refresh non-stacking:** multipliers written directly from args (lines 523-524, sanitized to 1 on non-positive), single one-shot handle re-armed.
- **Cadence:** `GetEffectiveAttackCadence` returns the PLAIN row value when inactive (no float residue — M1..M4 byte-identical); both cadence sites consume it (EnterAttack line 1177, StartAttackLunge 0.8× clamp line 1503, so a hastened lunge still completes). `RearmAttackTimerAtEffectiveCadence` re-rates a live loop on apply AND expiry, honors `LastAttackTime`, floors FirstDelay at 0.01 s — never synchronous, mid-swing edge handled both directions (verified arithmetic: buff at T+0.3 on 1.0 cadence ×1.5 → next hit at T+0.667; expiry at T+0.3 → next hit at T+1.0).

## Shadow scan (C4457/58/59) — clean

New params (`Seconds`, `MoveSpeedMult`, `AttackSpeedMult`) and locals (`RemainingFreeze`, `FreezeSeconds`, `ComposedSpeed`, `EffectiveCadence`, `Now`, `FirstDelay`, `Movement`, `World`) shadow no member/UPROPERTY in ASummonedUnit, ABuilding, or parents. New members (`bSpellFrozen`, `SpellFreezeTimerHandle`, `MoveSpeedBuffMultiplier`, `bCombatBuffActive`, `CombatBuffMoveSpeedMult`, `CombatBuffAttackSpeedMult`, `CombatBuffTimerHandle`, `BattleCryAttackSpeedBonus`, `BattleCryMoveSpeedBonus`) collide with nothing in MinerUnit.h, Tower.h, Barracks.h, DeepMine.h (grepped independently). No deprecated UE 5.8 APIs used (`DisableMovement`/`SetDefaultMovementMode`/`GetTimerRemaining`/`IsTimerActive` all current).

## Scope & convention checks

- Tower.cpp/SpellLibrary.*/SiegePlayerController.*/MinerUnit.*/Castle.*/SiegeGameMode.* untouched by 099 — ruling 14 honored (verified via grep: `ApplyFreeze`/`IsFrozen` exist only in the 4 owned files + the two consumer tasks' files). Castle and hero carry NO freeze API. ✔
- Building freeze is STATE ONLY — ApplyFreeze/EndSpellFreeze touch nothing but the latch + timer; TakeDamage/nav/collision paths byte-unchanged. ✔
- New `ABuilding::EndPlay` — ATower (Tower.cpp:40-45), ABarracks (Barracks.cpp:24-33), ADeepMine (DeepMine.cpp:34-58) all chain `Super::EndPlay`, verified. ✔
- Includes: Building.cpp gained `TimerManager.h` (line 15); `FTimerHandle` in Building.h rides Actor.h; SummonedUnit.cpp already had TimerManager.h + CharacterMovementComponent.h. ✔
- Names/signatures match the TASK-098/101 pinned externals and the names block character-for-character. ✔

## Notes for build-master

- No Build.cs change (no new modules).
- Compile order: SpellLibrary.cpp and Tower.cpp reference these APIs — all inside the TASK-103 batch as planned.
- **Gate:** the WARN-1 seam fix (TASK-098 constants → getters) must land and re-pass QA BEFORE TASK-103 compiles/commits; it is a 3-line SpellLibrary.cpp change with no 099-side edit.

## Carry-forwards (orchestrator routing)

1. TASK-098 fix per WARN-1 (before TASK-103).
2. TASK-100 QA: verify targeting mode cancels/refuses at match end (WARN-2 caller-side latch).
3. Next owner of SiegeGameMode.cpp: NIT-1 comment touch-up. Next owner of Building.cpp: NIT-2 stale TODO deletion. Next owner of the Rally path: NIT-3 `!bStatsLoaded` gate.
4. Design option (not required): frozen visual tint (decision 1) and freeze-pauses-miner-income (decision 4) if playtest asks.
