# TASK-098 handoff — USpellLibrary resolver + USiegeDamageType_Spell + castle 50% spell scaling

**Author:** gameplay-programmer · **Date:** 2026-07-08 · **Status:** ready-for-qa (orchestrator flips the board)

## Files touched

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.h` | NEW — USpellLibrary (UBlueprintFunctionLibrary), pinned `ResolveSpell` entry, full refusal-semantics contract in the class doc |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp` | NEW — dispatch on `Row.SpellEffect` + five effect resolvers + VFX contract, all in an anonymous namespace |
| `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.h` | NEW `USiegeDamageType_Spell` UCLASS; registry comment updated (Spell = 50% castle-only, mirrors Projectile precedent) |
| `Source/GitClaudeUnrealTest/Siegebound/DamageTypes.cpp` | comment-only update (registry consumers) |
| `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` | `ACastle::TakeDamage` gains the `USiegeDamageType_Spell` (and subclasses) = ×0.5 branch — disjoint `else if` next to Siege ×2 / Projectile ×0.5; old M5 TODO comment replaced. ABuilding.cpp deliberately NOT touched (ruling 3). |

## Per-spell resolution summary (dispatch on `ESpellEffect`)

- **AoEDamage (Fireball):** `FSiegeCombatStatics::ApplyRadialDamage(World, nullptr, CasterTeam, TargetPoint, Row.AoERadius, Row.Damage, USiegeDamageType_Spell)` — the TASK-055 helper REUSED verbatim: enemy-only Team filter, closest-point radius, per-receiver TakeDamage routing. Castle overlap therefore lands at 50% via the new Castle.cpp branch (§3.11 acceptance: 50, not 100).
- **Freeze (FrostNova):** every enemy `ASummonedUnit` (not `IsUnitDead`) and `ABuilding` (not `IsBuildingDestroyed`) within closest-point `AoERadius` of TargetPoint gets `ApplyFreeze(Row.EffectDuration)` (TASK-099 pinned API). Castle + hero excluded by type (neither is a unit/building — ruling 5). Refresh/precedence semantics live in TASK-099.
- **TopTargetsDamage (Lightning):** ruling-4 selection — candidates = live enemy `ASummonedUnit` / `AHeroCharacter` / `ABuilding` (castle `IsA<ACastle>` explicitly excluded; unknown ITeamAgent types never selected) within `AoERadius`; stable-sorted by **CURRENT HP desc, tie → reticle distance asc** (deterministic); first `MaxTargets` each take `Row.Damage` via `UGameplayStatics::ApplyDamage` tagged Spell. Buildings take it FULL (50% is castle-only) — Lightning 200 kills ArrowTower 150.
- **AllyBuff (BattleCry):** every FRIENDLY live `ASummonedUnit` in radius gets `ApplyCombatBuff(1.25f /*move*/, 1.5f /*attack*/, Row.EffectDuration)` (TASK-099 pinned signature/order: MoveSpeedMult, AttackSpeedMult, Seconds). Hero never buffed (not a unit).
- **GoldSteal (Pickpocket):** instant global (ruling 7 — TargetPoint is VFX-anchor only). Victim = the other team's `ASiegePlayerState` via `ASiegeGameState::GetPlayerStateForTeam` (TASK-043). **Exact API composition (spec item): `Steal = FMath::Min(Row.GoldSteal, Victim->GetGold())`; if Steal > 0 → `Victim->SpendGold(Steal)` (checked) then `Caster->AddGold(Steal)`.** Both mutations ride the private SetGold choke point (clamp + OnGoldChanged) — no raw gold writes anywhere.
- **VFX contract (ruling 11):** every SUCCESSFUL resolve spawns `/Game/VFX/NS_Spell_<CardID>` at TargetPoint — soft path composed from CardID, `LoadSynchronous`, missing system = log-once-per-CardID + resolve anyway.

## Refusal semantics (bool contract — caller refunds on false)

`false` (nothing changed, caller refunds): null World; `CardID.IsNone()`; `SpellEffect None`/unknown; malformed row for the effect (non-positive Damage/AoERadius/EffectDuration/MaxTargets/GoldSteal as applicable); GoldSteal with no `ASiegeGameState` or a missing caster/victim player state (**Sandbox mode: Pickpocket refuses → full refund**, since no Red economy exists).
`true` (gold stays spent): every well-formed cast, **including zero-target whiffs** — Fireball on empty ground, FrostNova/BattleCry catching nobody, Lightning with no candidates, Pickpocket on a 0-gold victim (stole everything they had: nothing). Consistent with the §3.5 "reticle anywhere / resolves at the point" law.

## FLAGGED DECISIONS (QA please rule on each)

1. **BattleCry magnitudes = named constants in SpellLibrary.cpp** (`BattleCryMoveSpeedMultiplier = 1.25f`, `BattleCryAttackSpeedMultiplier = 1.5f`, `// GDD §4` comments). Ruling 6 says "mechanic-rule UPROPERTYs … live with the API owner", but (a) the pinned resolver entry `ApplyCombatBuff(MoveSpeedMult, AttackSpeedMult, Seconds)` makes the CALLER supply the values (the Rally/War Banner caller-owns-magnitude precedent), (b) a static UBlueprintFunctionLibrary cannot carry instance UPROPERTYs, and (c) my names block pins ONLY ApplyFreeze/IsFrozen/ApplyCombatBuff as usable externals — reading an unpinned accessor/UPROPERTY off ASummonedUnit would be name-guessing against TASK-099's in-flight "flag exact placement". **Reconciliation:** when TASK-099's placement lands, swapping the two constants for the owned values is a two-line change at TASK-103.
2. **Gold-steal composition uses `AddGold`** — SiegePlayerState.h's `AddGold` doc says "Sole caller: the dev/test Sandbox starting-gold grant". Pickpocket is now a second legitimate caller through the same choke point. The stale comment lives in SiegePlayerState.h (NOT my file set) — flag for its next owner; behavior is correct per the SetGold choke-point law.
3. **Caster at the 999 MaxGold cap:** the victim loses the full Steal amount but the caster's `AddGold` clamps at MaxGold (overflow evaporates). This is the choke-point clamp law working as designed; the ruling only pins `min(GoldSteal, victim's gold)`. Alternative (pre-check caster headroom) is unspecced — not implemented.
4. **Whiff = success rule** (above): GDD/rulings are silent on zero-target casts; I aligned everything with the implicit Fireball-on-empty-ground behavior of the radial helper. If the manager wants refuse-on-whiff for any spell, it is a per-resolver one-liner.
5. **Closest-point distance mirror:** Freeze/Lightning/AllyBuff need the ApplyRadialDamage distance convention, but `SiegeCombatStatics.h` is outside my assigned file set — a file-local helper in SpellLibrary.cpp mirrors it (documented against qa/TASK-026 NIT-4; consolidate on the wave that owns all mirrors).
6. **Castle.h stale TODO:** `Castle.h` line ~80 still carries `TODO(Spell 50% — M5)` in the TakeDamage doc comment. My names block pins **Castle.cpp only**, so I did not touch the header. One-line comment removal for a future Castle.h owner (or QA pre-approval).
7. **Miners are units:** `AMinerUnit` IS-A `ASummonedUnit`, so FrostNova freezes enemy miners and BattleCry buffs friendly miners (move term useful, attack term inert). GDD says "units" — reading them as included.
8. **FrostNova freezes ALL ABuilding subclasses** (Barracks, Deep Mine, walls included) via the state-only `ABuilding::ApplyFreeze`. In M5 only the tower fire-gate consumes `IsFrozen()` (ruling 14 / TASK-101), so a frozen Barracks keeps spawning and a frozen Deep Mine keeps paying until some future task gates them. Matches the ruling text ("enemy units + buildings in radius"); flagging the visible consequence.
9. **Null instigator/causer on all spell damage** (radial helper AND Lightning strikes): the resolver has no controller; the enemy-only Team filter is the friendly-fire authority (the documented ApplyRadialDamage design). Receivers that resolve no team apply the damage — safe because friendlies are filtered out before ApplyDamage is ever called.
10. **VFX log-once guard is a process-lifetime function-local `static TSet<FName>`** (a static library has no instance to own it — the cached-static precedent from ApplyTeamMaterial). Worst case: the warning re-fires only after an editor restart, never per-cast spam.
11. **GoldSteal VFX anchors at the caller's TargetPoint** — for the instant spell the caller (TASK-100/102) chooses the anchor; recommend the caster's castle or hero location, but ZeroVector is harmless.
12. **Pinned externals consumed, not yet compiled:** `ASummonedUnit::ApplyFreeze/ApplyCombatBuff` and `ABuilding::ApplyFreeze` do not exist until TASK-099 lands — by design (parallel-safe note on my task block); the batch compile is TASK-103. Signatures used character-for-character from the CONVENTIONS/names pins.

## What QA should scrutinize

- Castle.cpp branch: Spell must be a sibling `else if` (disjoint types), castle ONLY — verify ABuilding.cpp untouched.
- Lightning: current-HP (not MaxHP) metric; castle exclusion; deterministic tie-break; no double-hit possible (each candidate appears once in the array).
- Shadow scan: USpellLibrary has no instance members; the local `FStrikeCandidate` fields (`TargetActor`/`TargetCurrentHP`/`ReticleDistance`) shadow nothing (C4458-clean by construction).
- Gold flow: both mutations choke-pointed; `SpendGold` result checked before `AddGold`; no path moves gold on a refusal.

## Fix loop 1 (2026-07-08)

Per `qa/TASK-098-report.md` (FAIL: 1 blocker + pre-approved WARN-2.1 + optional NIT-1). Files touched this loop: `SpellLibrary.cpp`, `SpellLibrary.h`, `Castle.h` (one comment line, pre-approved). Nothing else — SummonedUnit.*, SiegePlayerController.*, SiegeBotController.*, Building.*, Tower.* all untouched.

### BLOCKER — BattleCry magnitudes now sourced from the unit (ASummonedUnit is the lawful owner, M5 ruling 6)

- **SpellLibrary.cpp — anonymous-namespace constants DELETED** (were lines 25-38: the doc block + `constexpr float BattleCryMoveSpeedMultiplier = 1.25f` / `BattleCryAttackSpeedMultiplier = 1.5f`). The namespace now opens directly with `TeamToString`. Grep-verified: zero references to the old constant names remain anywhere in `Source/`.
- **Call site (now SpellLibrary.cpp:355)** replaced with the exact composition pinned at SummonedUnit.h:226-230, read PER-UNIT as the buff is applied:
  `Unit->ApplyCombatBuff(Unit->GetBattleCryMoveSpeedMultiplier(), Unit->GetBattleCryAttackSpeedMultiplier(), Row.EffectDuration);`
  Accessor names/semantics verified character-for-character against the landed SummonedUnit.h:237-243 (`1.f + BattleCry*Bonus`, BlueprintPure). Editor tweaks to the `BattleCryMoveSpeedBonus`/`BattleCryAttackSpeedBonus` UPROPERTYs are now live — the dead-tunable trap is gone; single source of truth.
- **Summary log (now SpellLibrary.cpp:359-362)** — took the report's "count + duration" option since the multipliers are per-unit: `buffed %d friendly unit(s) (each unit's own BattleCry multipliers) for %.1fs within %.0f of (%s)`. 5 format specifiers / 5 args, verified.
- **ResolveAllyBuff doc comment** updated to state per-unit magnitude ownership (ruling 6) and the pinned composition.
- **SpellLibrary.h class-doc AllyBuff bullet (lines 38-43)** — the "see the .cpp constants" reference replaced with the accessor-based composition + "magnitudes are the unit's own BattleCry* mechanic UPROPERTYs".

### Pre-approved WARN-2.1 — Castle.h:80

Stale `TODO(Spell 50% — M5)` line UPDATED in place (QA pre-approval said "remove/update this comment line") to the accurate doc: `USiegeDamageType_Spell (and subclasses) = 50% (spells vs the castle, GDD §3.0/§3.11 — TASK-098).` — one comment line touched, zero code effect, and it also fixes the report's note that the TakeDamage doc list omitted Spell.

### Optional NIT-1 — taken: `UFUNCTION(BlueprintCallable)` dropped from ResolveSpell

Condition verified before acting: grep of all `ResolveSpell` consumers shows ONLY C++ static calls (SiegeBotController.cpp:487/:534 landed; SiegePlayerController.h:124/:638/:843 docs pin the same `USpellLibrary::ResolveSpell` C++ call for TASK-100) and no Blueprint consumer exists. Removing the specifier leaves the C++ declaration character-identical, so the pinned signature is untouched. A doc sentence records the decision ("C++-only entry, deliberately NOT BlueprintCallable — qa/TASK-098 NIT"). Note for build-master: SpellLibrary.generated.h shrinks (BP thunk gone) — expected, still batch-compiles at TASK-103.

### Deliberately left (carry-forwards, not this loop's files)

- WARN-2.2 `Building.cpp:279` TODO — report explicitly says do NOT touch (keeps ABuilding.cpp byte-identical per ruling 3).
- WARN-2.3 `SiegePlayerState.h:128-130` "Sole caller" doc — next owner.
- WARN-1 closest-point mirror consolidation — next wave that owns SiegeCombatStatics (qa/TASK-026 NIT-4 debt).
- NIT-2 (per-cast LoadSynchronous) and NIT-3 (exact-float tie predicate) — report records both as acceptable/intended, no action requested.

Shadow scan of changed regions (C4457/58/59): CLEAN — no new locals introduced; deleting the namespace-scope constants only removed collision surface. No compile run (build-master owns it, TASK-103 batch). Status: back to ready-for-qa.
