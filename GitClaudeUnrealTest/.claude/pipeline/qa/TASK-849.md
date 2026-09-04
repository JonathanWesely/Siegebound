# QA Report — TASK-849 (LANE W FEATURE GATE)

**Verdict: PASS** — all four subjects. **0 BLOCKERS · 4 WARN · 3 NIT.**

| task | verdict | blockers |
|---|---|---|
| **TASK-829** — the veil state + the break call | ✅ **PASS** | 0 |
| **TASK-830** — the 3 s interruptible cast + circle targeting **+ item (8)** | ✅ **PASS** | 0 |
| **TASK-831** — the `Witch` card row | ✅ **PASS** | 0 |
| **TASK-851** — the bot's threat read | ✅ **PASS** | 0 |

## `SC-§29` COVERAGE LEDGER

This gate covers **exactly four tasks, named**: **`TASK-829` · `TASK-830` (both sittings, §1–§21) · `TASK-831` · `TASK-851`**.

⭐ **THE `blocked-by` VALVE IS RESOLVED AND THE LEDGER SAYS WHICH WAY: `TASK-851` FOLDED IN.** It was `ready-for-qa` at gate time, so it did **not** drop out and does **not** take a gate of its own.

⛔ **NOT covered by this gate:** `TASK-827` (→ `qa/TASK-847.md`) · `TASK-828` (→ `qa/TASK-848.md`) · `TASK-837`/`838`/`839`/`840` (→ `TASK-850`) · `TASK-860`/`861`/`867`/`868` (not gated here — but see N-1, where I measured `868`'s repair as landed) · the compile · art (`832`/`833`/`834` → `TASK-835`).

⛔ **Method (`SC-§38`): every location in this report was found by opening the named FUNCTION and reading the quoted EXPRESSION. Not one line number from any document was trusted.** All coordinates below are my own reads, dated 2026-09-02, and are annotations.

---

## ⚖️ (a0) — THE SCOPE RULING ON `FindLightningTowerTarget`

### ✅ **UPHELD. `WITCH-§8`'s corrected table STANDS. The law edit does NOT revert.**

I verified the **reasoning**, not the manager's conclusion (`SC-§40` cl. 3). The call graph, traced end to end at source:

```
ASiegeBotController::EvaluateDecisions  rule 3b
  → FindLightningTowerTarget(Chosen.Row->AoERadius, …)        ← the COUNT  (SiegeBotController.cpp)
  → USpellLibrary::ResolveSpell(World, CardID, Row, BotTeam, TargetPoint)
      → switch (Row.SpellEffect) case ESpellEffect::TopTargetsDamage
        → ResolveTopTargetsDamage(...)
          → FSiegeCombatStatics::GatherHostileAgents(World, CasterTeam, HostileAgents)   ← the RESOLVE
```

Three independent facts, each measured:

1. **`Lightning`'s `SpellEffect` cell is `TopTargetsDamage`** — read out of `Docs/Data/cards.csv` row 26, not inferred from the card's name.
2. **`ResolveTopTargetsDamage` gathers with the DEFAULT policy** — `GatherHostileAgents(World, CasterTeam, HostileAgents)`, no fourth argument.
3. **The default is `ESiegeVeilPolicy::SuppressVeiled`** — `SiegeCombatStatics.h`, the declaration's default argument.

⇒ **the premise is TRUE: Lightning's resolver is already veil-suppressed.** An unsuppressed count feeding a suppressed resolver is the two halves of one spell disagreeing about who exists — the bot pays for a bolt aimed at units it cannot damage. It is the **same *"choosing is seeing"* category as the Fireball scan**, not a new one, and the same precedent this gate's sibling upheld for `GatherFriendlyAgents`. **In scope, correctly suppressed, and the row belongs in the law.**

### ⛔ (a0a) — THE TWO DELIBERATE NON-CHANGES: BOTH ARE COMMENTS, NEITHER IS CODE

- **The HERO arm** (`FindNearestEnemyIntruderOnBotHalf`, `TActorIterator<AHeroCharacter>`) — **comment only**, naming `J-W10`, followed by the untouched loop. ✅ No consult. The comment also states *why* a consult there could never evaluate false (the predicate class-filters before it asks the rule), which is the strongest form of the omission.
- ⛔ **`IsBotHalfPointClear` — THE TRAP ON THE PAGE. CONFIRMED LEFT ALONE.** Its `TActorIterator<ASummonedUnit>` loop carries **no** `IsAgentVisibleTo` call. I verified this three ways: (a) a whole-file symbol sweep returns `FSiegeCombatStatics::IsAgentVisibleTo(` at **exactly three** code sites — `FindNearestEnemyIntruderOnBotHalf`, `FindFireballClusterTarget`, `FindLightningTowerTarget` — and none inside the clearance function; (b) the function carries a three-reason named comment (presence ≠ perception · the loop is **not team-filtered** so a consult would suppress only enemies and make a symmetric physics rule asymmetric · it would re-open `TASK-265`'s pile-up as an exploit); (c) **the shipped suite actively pins it at ZERO** — `SiegeInvisibilityTest.cpp` extracts the clearance body and asserts `FSiegeCombatStatics::IsAgentVisibleTo(` == 0 in it. ⭐ **The trap is not merely avoided; it is guarded.**

### ⛔ (a0b) — I RE-MEASURED THE SITE COUNT BY SYMBOL MYSELF

**My census of `SiegeBotController.cpp`, taken independently:**

| # | symbol | iterator | category | shipped |
|---|---|---|---|---|
| 1 | `FindNearestEnemyIntruderOnBotHalf` — units arm | `ASummonedUnit` | threat read | ✅ consult |
| 2 | same function — hero arm | `AHeroCharacter` | threat read | 💬 comment, no code |
| 3 | `FindFireballClusterTarget` | `ASummonedUnit` | aiming | ✅ consult |
| 4 | `FindLightningTowerTarget` | `ASummonedUnit` | aiming | ✅ consult |
| 5 | `IsBotHalfPointClear` | `ASummonedUnit`, both teams | physical occupancy | 💬 comment, no code |

**⇒ FIVE unit-perception sites across FOUR functions. It MATCHES, and I am reporting it because a silent match is indistinguishable from a skipped check** (`SC-§40` cl. 9). Non-unit iterators in the file (`ATower` ×1, `ACastle` ×2, `ACaptureZone` ×2, `ABuilding` ×1) are untouched and none of those classes is veilable.

⛔ **`ApplyRadialDamage` in the bot file: 0 on code lines** (1 raw — a single occurrence inside the `FindFireballClusterTarget` comment explaining that the blast still lands). Hidden ≠ invulnerable, held. ⛔ **Zero fog terms** — `FSiegeFogStatics` / `EffectiveVisionRadius` / `Fog` return no hits in the file, so `J-F4` is intact and `TASK-838`'s fence is not breached by `851`.

---

## ⚖️ (z1) — THE NAMED ITEM OF `TASK-830`: RULED, IN WRITING

### ✅ **NEITHER (a) NOR (b). ITEM (8) IS *BUILT*. The premise of (z1) is stale, and I measured it rather than relaying it.**

`TASK-867`'s reading of `GetCastProgressPercent` / `IsCastInProgress` at **0 tree-wide** was correct *at the time it was taken*. A **second sitting** of `TASK-830` (handoff §11–§21, appended 2026-09-02) landed the item. Re-derived by me, at source:

- `Source/.../HealthBarProvider.h` carries **both defaulted virtuals, character-for-character with `WITCH-§9.3`'s pin**:
  `virtual float GetCastProgressPercent() const { return 0.f; }` · `virtual bool IsCastInProgress() const { return false; }`
- `ASummonedUnit` is the **only** class overriding them (declared `override` in its `IHealthBarProvider` block; bodies in the .cpp).
- `ABuilding` / `AHeroCharacter` grow **no** override ⇒ `WITCH-§9.6`'s pixel-identical requirement is met structurally.

⇒ **`TASK-860`'s premise gate (0a) is GREEN and should NOT fire.** ⛔ **`TASK-860` must still verify it itself — its row tells it to, and this ruling is a citation, not a measurement it may inherit** (`SC-§40` cl. 3).

⚖️ **Process note, on the record:** a task reporting `ready-for-qa` with a named item unbuilt was a real defect, and the read-only sweep that caught it (`TASK-867`) was aimed at something else entirely. The item was then built *before* this gate opened, so there is nothing to return — but the QA-loop event that (z1) contemplated **did not have to happen only because two downstream tasks refused to proceed on a red premise.** That refusal is the behaviour to keep.

---

## ⚖️ (z2) — THE LOAD-BEARING SEAL BEHIND TWO ZERO-VALUED CELLS

⛔ **NAMED AS LOAD-BEARING, AND ALL THREE PARTS EXIST. Verified by symbol, each one read:**

**`Damage 0` + `Support` is safe because of ONE guard:**
- `ASummonedUnit::UpdateSupportHealTargeting()` opens with `if (IsVeilCaster()) { StopHealing(); SupportHealTarget = nullptr; return nullptr; }` — **above** the Cleric's four extracted statements. ✅ Without it a `Damage`-0 Support witch arms the heal timer and `PerformHeal` calls `BreakInvisibility(Heal)` every 0.1 s to deliver zero HP. **`TASK-830` found this itself and it is a genuine defect fix, not a tidy-up.**

**`Cadence 0` would be 20 attacks/second but for the THREE-PART SEAL, and I name all three:**
- `AttackCadence = FMath::Max(Row->Cadence, MinAttackCadence)` with `constexpr float MinAttackCadence = 0.05f` ⇒ a `Cadence`-0 row **does** bind a 0.05 s cadence. The hazard is real, not hypothetical — the shipped source names it in `EnterAttack`'s own comment (*"a Cadence-0 Sorcerer row would arm a 0.05 s looping attack timer — 20 hits/s"*).
- **SEAL 1 — `ASummonedUnit::UpdateStateGrouped`:** `if (!FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing()))` ⇒ target forced null, falls to tier-3 station-keeping. **Acquires nothing.**
- **SEAL 2 — `ASummonedUnit::EnterAttack`:** the same `IsAttackAllowed(CanEverAttack(), …)` gate ⇒ **the attack timer is never armed.**
- **SEAL 3 — `ASummonedUnit::PerformAttack`:** `if (bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || !FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing()))` ⇒ **refuses even if a timer somehow fired.**
- All three key on `CanEverAttack()`, whose base body is now `return !IsVeilCaster();` ⇒ **false for the Witch and byte-unchanged for every other shipped unit.**

⇒ ✅ **The coupling is ACCEPTABLE and the seal is real. But see `WARN-2`: it is not asserted by a test.**

---

## ⚖️ (z3) — THE `Range 400` MECHANISM: THE DISPATCH WAS WRONG, THE AGENT WAS RIGHT

⛔ **RULED FOR `TASK-831`, ON MY OWN MEASUREMENT.** The dispatch's framing (*"it is what a witch with no position circle actually uses"*) is **refuted**:

- `ResolveWitchPositionCircle` writes `OutRadius = (AttackRange > 0.f) ? AttackRange : WitchVeilRadiusFallbackUU;` and `WitchVeilRadiusFallbackUU = 400.f` ⇒ **`Range 0` yields the SAME 400 uu radius through the backstop.** The cell is *not* load-bearing for the radius value.
- ✅ **THE REAL MECHANISM, CONFIRMED AT SOURCE:** `EnterAdvance` reaches `AI->MoveToActor(Goal, FMath::Max(AttackRange * 0.8f, 40.f), /*bStopOnOverlap=*/ true)`. **`0.8 × 400 = 320 uu`, and `320 < 400` ⇒ she halts INSIDE her own veil circle, so the cast she walked over to start is still valid when she arrives.** With `Range = 0` this is `max(0, 40) = 40 uu` — she would push into the subject's capsule. **There is no fallback on this path.**
- ✅ **The second reason is also real and is the GDD §3.0 one:** at 400 the **data** drives the knob and the constant is a genuine backstop; at 0 the column would be a lie and the number unreachable from `cards.csv`.

⇒ **Right cell, right value, wrong mechanism in the dispatch. The agent opened the file, found it, and said so rather than passing it over — that is `SC-§40` cl. 5 working.**

---

## Findings

### WARN-1 — `SiegeBotController.cpp` (`FindLightningTowerTarget` comment) · `CONVENTIONS.md` `WITCH-§8` · `TASKBOARD.md` `TASK-849`/`TASK-851` · `handoffs/TASK-851-programmer.md` §3
**⛔ *"the bot spends **40 gold** aiming a bolt at units the resolver cannot damage"* — LIGHTNING COSTS **24**.**
Measured: `Docs/Data/cards.csv` row `Lightning` → `Cost` column = **`24`**, corroborated independently by the bot's own deck table (`Entry(TEXT("Lightning"), 2), // 24 x2 = 48`). The figure **40** appears in four places, **including now in shipped source** as a comment inside the very function this gate ruled on.
⚖️ **The ruling is UNAFFECTED** — the defect is *"the count and the resolver disagree about who exists"*, and its severity does not rest on the price. But this is `SC-§38` cl. 8 exactly: *a figure quoted in prose is a dated annotation; the expression it was computed from is the key.* The expression here is the `Cost` cell, and nobody read it.
**Fix:** correct `40` → `24` in `WITCH-§8`'s row-4 note and in the source comment (comment-only, no behaviour). ⚠️ **Not a blocker and not `TASK-851`'s fault** — it inherited the number from the law it was sent to check, and it caught the *structural* error in that same law.

### WARN-2 — `Tests/SiegeInvisibilityTest.cpp` (absent assertion)
**⛔ THE THREE-PART ATTACK SEAL THAT MAKES `Cadence 0` SAFE IS NOT ASSERTED BY ANY TEST.**
Measured absence, with the symbols named and a positive control (`SC-§40` cl. 11): I searched all 30 files under `Siegebound/Tests/` for `CanEverAttack`, `IsVeilCaster` and `MinAttackCadence`. `IsVeilCaster()` **is** pinned — but only in `UpdateSupportHealTargeting` (heal) and `CanTakeZoneOrders` (orders). **No row anywhere pins `CanEverAttack()`'s base body as `!IsVeilCaster()`, nor asserts any of the three `IsAttackAllowed(CanEverAttack(), …)` guard points against a veil caster.** (Positive control: the same sweep found `CanEverAttack` in `SiegeLadderClimbTest` and `SiegeRecallTest`, so the scan was not blind.)
⚠️ **The exposure, stated concretely:** the two cells that arm the hazard (`Damage 0`, `Cadence 0`) live in a **CSV row**, and the guard that defuses them lives in a **C++ predicate body** — nowhere near each other, with nothing linking them. A future edit narrowing `IsVeilCaster()` or `CanEverAttack()` would silently arm a **20-attacks-per-second healer** and **no test in the tree would go red**.
**Named follow-on:** board a row adding two assertions to `SiegeInvisibilityTest.cpp` — (a) `CanEverAttack()`'s base body contains `IsVeilCaster()` exactly once, (b) `AttackCadence = FMath::Max(Row->Cadence, MinAttackCadence)` is still the binding expression — each with the `Damage 0`/`Cadence 0` coupling written into its failure message so the next reader finds the CSV from the C++.

### WARN-3 — `Tests/SiegeAssistantSelectionTest.cpp` `ThirteenKindsInCardRowOrder()` (data-driven staleness; **manager's to board, NOT `TASK-831`'s to patch**)
**⛔ THE WITCH ROW MOVES THE COMMANDABLE-KIND COUNT FROM 13 TO 14, AND THE CAP IS PINNED AT 13.**
Verified independently: `USiegeAssistantSnapshot::Capture` iterates `TActorIterator<ASummonedUnit>` and filters on **validity, team and death only — no `Profile` filter, no `CardType` filter** — then takes `CanonicalKind(Unit->GetCardID())`, a total lower-casing of any non-`None` CardID. **The Witch is an `ASummonedUnit` with a bound CardID ⇒ she enters the roster.** The transcription is a static 13-entry array ending `sorcerer`, and `MaxRosterKinds` is asserted at **13**.
⇒ **Three statements in that array's own comment are now false in the shipped data**, and the array's stated purpose — guarding the case of Jonathan's shrink-loop defect — now guards a tail that the data may have moved.
⚠️ **HONEST LIMIT ON MY OWN CLAIM:** I confirmed the array, the cap and the absence of a profile filter. **I did NOT measure the runtime tally ordering**, so *"`witch` is the new tail"* is `TASK-831`'s measurement and **not mine** — a boarded task must re-derive it. ✅ **No test goes red** (static transcription).
⚖️ **`TASK-831` was right on all three counts:** it is outside its names list, it belongs to a different shipped defect of Jonathan's, and its own comment warns that reshaping the array *"would quietly convert the collapse tests into tests of a case that cannot happen"*. **Reporting and not patching was correct.**

### WARN-4 — `GetDamageBoostPercent`: the reported 10-vs-6 discrepancy is REAL, and **both figures are unusable**
`TASK-830` §18 flagged that `TASK-867` records **10** (raw `grep -rn`) where the house helper reads **6**. **Concur that they disagree; the disagreement reproduces.** But my own re-derivation over `Source/GitClaudeUnrealTest/` gives **15 raw / 9 skip-aware** — matching *neither* published figure, because **neither figure named its SCOPE** (tree vs `Siegebound/` vs shipping-source-excluding-`Tests/`), and because `TASK-830`'s own addendum added three more occurrences after `TASK-867` measured.
⇒ ⭐ **`TASK-830`'s general form is right but one term short.** The durable rule is: **a count is meaningful only together with the INSTRUMENT *and* the SCOPE that produced it.** That is `SC-§38` cl. 8 married to `TL-§5`, and I recommend the amendment be written that way rather than as instrument-only.
⚠️ **Live consequence:** a future task pinning **10** with `CountOccurrencesInCode` would be red on arrival; one pinning **6** at tree scope would be red today.

### NIT-1 — `SummonedUnit.cpp` `ASummonedUnit::IsCastingVeil()` (comment is inaccurate)
Its comment claims the body is *"Safe pre-BeginPlay and on a torn-down world"*. **That is true of a spawned actor and FALSE of a class default object** — `AActor::GetWorld()` returns null for a CDO by construction, and `GetWorldTimerManager()` is a bare `GetWorld()->GetTimerManager()`.
⚖️ **RULED — and this is one of the four `TASK-830` put to me by name:** ✅ **the workaround was CORRECT.** Placing the guard **above** the call in `ResolveCastClockOwner` rather than inside the pinned body is right on the merits and not merely on fence grounds — a guard inside `IsCastingVeil()` would make every caller pay for a case only one caller can reach, and would **hide the CDO question at the one place it matters.** ⛔ **No CDO path reaches `IsCastingVeil()` today** (its callers are `UpdateWitchCast`, `CancelWitchCast` and `ResolveCastClockOwner`, and the last already guards) ⇒ **no crash risk.** The sentence should still be corrected — a **comment-only** change that cannot disturb the `IsTimerActive(WitchCastTimerHandle)` == 1 pin, since that pin counts a code line.

### NIT-2 — `HealthBarProvider.h` class comment still reads *"Pure-virtual C++ interface"*
Inexact **before** this batch (the boost pair defaulted it in `TASK-362`); the cast pair adds the second exception. ✅ **Correct to leave alone** — it is a shipped comment outside the two named virtuals. Fold into whoever next edits that header.

### NIT-3 — `handoffs/TASK-831` §1 says *"all 29 comparable columns"* where the header carries **30** named columns beside the row name
Cosmetic bookkeeping in a handoff only; the CSV row is **field-count-aligned with the header (31 fields each)**, which is the property that actually matters — see N-3.

---

## Notes for build-master (PASS)

**N-1 — ⭐ THE ONE KNOWN RED ROW IS ALREADY REPAIRED IN THE TREE, AND I RE-DERIVED IT GREEN.**
`SiegeAcquisitionFunnelTest`'s `Siegebound.Acquisition.VeilAndFogSuppressionLiveOnlyInTheFunnel` (test 9) went red on **correct** code when `TASK-829` landed the veil's two write-doors on `ASummonedUnit`. **`TASK-868`'s repair is present in the shipped file** — the flat five-zeros row is split into **DECISION tokens** (`IsVisibleTo(`, `FSiegeFogStatics`, `EffectiveVisionRadius` — zero in all five call-site files) and **STATE tokens** (`bIsInvisible`, `FSiegeInvisibilityStatics` — zero in the four non-owner files, owner exempted **by name** and replaced by an exact enumerated write-door census). I re-derived every row:
- `IsVisibleTo(` in `SummonedUnit.cpp` = **0** ⭐ (`IsAgentVisibleTo(` does **not** contain the substring `IsVisibleTo(` — the two characters before `VisibleTo(` are `nt`, not `Is`; I checked this rather than assumed it)
- `FSiegeFogStatics` / `EffectiveVisionRadius` = **0** in all five
- `bIsInvisible` / `FSiegeInvisibilityStatics` = **0** in `Tower.cpp`, `HeroCharacter.cpp`, `SpellLineSweep.cpp`, `SiegeCheatManager.cpp` — the only hits in `Tower.cpp` and `HeroCharacter.cpp` are **`//` comment lines**, which the house helper skips
- owner census: `FSiegeInvisibilityStatics::ApplyVeil(` = **1** · `ApplyBreak(` = **1** · `(bIsInvisible` = **2** · inlined assignments = **0**
⇒ **I do not expect this row red at compile.** ⛔ It is **not** a subject of this gate and I have failed nobody for it; if it *is* red on the runner, the cause is newer than my read.

**N-2 — THE SITE LEDGER, MEASURED, FOR THE COMMIT MESSAGE.**
Eight `BreakInvisibility(ESiegeVeilBreakReason::…)` calls tree-wide: `PerformAttack` melee · `PerformAttack` ranged · **`ApplyDetonation` (the Sapper trap — inside the function BOTH `Detonate()` and `HandleDeath()` share, so a third entry inherits it)** · `PerformHeal` · `HandleDeath` · `CompleteWitchCast` (`Cast`, on **herself**) — all in `SummonedUnit.cpp` — plus **`AAncientGround::ApplyBoostTick`'s `if (Unit->IsAncientGroundEmpowerer())` arm (the COUNTED sorcerer, which `continue`s before `Occupants`)** and **`AMinerUnit::UpdateMining`'s `if (Node->TryRegisterArrivedMiner(this))` success arm (the ARRIVAL claim)**. ⭐ **All three `WITCH-§3a` traps land on the right side, and the two wrong sites carry named refusal comments** (the recipients loop; the gold payout).

**N-3 — DATA, RE-MEASURED INDEPENDENTLY.**
`Docs/Data/cards.csv` `DeckCount` across the 32 rows sums to **exactly 50** (Footman 9 · Archer 8 · Knight 3 · Miner 3 · ArrowTower 3 · Wall 4 · MilitiaMob 3 · Pikeman 3 · Cavalry 3 · Longbowman 2 · Cleric 2 · Ogre 2 · Fireball 2 · FrostNova 1 · Sorcerer 2; all others **0**, **Witch 0**). The `Witch` row is **31 fields, header-aligned**, and its `Notes` cell carries **no comma** — which is what keeps `SiegeFogClampTest`'s runtime CSV parser from silently skipping it. `CardID` is **`Witch`**, character-for-character against `WitchCardID`, on which `IsVeilCaster()` and therefore the whole feature hangs.

**N-4 — COMPILE RISKS I READ BUT CANNOT EXECUTE (⛔ no compile in my lane).** All resolve clean on reading: `SummonedUnit.h` does **not** include `SiegeCombatStatics.h` ⇒ **no header cycle** with `SiegeCombatStatics.cpp`'s new `#include "Siegebound/SummonedUnit.h"` · `Cast<ASummonedUnit>(const AActor*)` yields `const ASummonedUnit*` via the const-propagating overload · `TArray::RemoveAll` is **order-preserving** (`RemoveAllSwap` is the one that is not) — ✅ `TASK-829` §9.1's highest-consequence claim is **correct**, and it matters because all eight acquisition sites tie-break by strict improvement · `ResolveCastClockOwner`, `IsWitchVeilCandidate` and `ClearWitchCastChannel` reach **private** members of *another instance of the same class* (legal — access is per-class) · `AActor::GetWorldTimerManager()` is `const`, so binding its return to `const FTimerManager&` from a `const` accessor compiles.

**N-5 — RESIDUALS TO CARRY INTO THE COMMIT, DECLARED BY THE AGENTS AND ENDORSED BY ME.** A **lone veiled sorcerer un-veils itself** with nobody to boost (the only reading `WITCH-§3a` permits; reversible on one word, a `WITCH-§3` amendment) · a **veiled unit still captures a zone** (`J-W9`, accepted) · **the render half is absent** (`MI_Unit_Invisible`, `TASK-832`) — a veiled unit looks **completely normal to its owner** · **the witch keeps walking while channelling** · `BP_Unit_Witch` and `T_CardArt_Witch` do not exist yet (`TASK-835` / `TASK-834`; the card face degrades to text-only, never a crash).
⛔⛔ **AND THE ONE THAT BOUNDS EVERYTHING ABOVE: NOTHING IN THIS BATCH IS PLAYTESTED.** The house rule bans `SpawnActor`/`CreateWorld` under `Siegebound/Tests/`, so every wiring claim here is pinned **structurally**. **A green suite is not *"invisibility works"*** — that needs a PIE pass with a Witch on the field: play her, watch a friendly go invisible after ~3 s, **shoot the witch mid-cast and confirm NOTHING is veiled**, then confirm the veiled unit is ignored by enemy units, towers **and the bot**, and that it is **still attackable** by a player who knows where it is.

**N-6 — SUITE CENSUS (`TL-§5c`).**
**410 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` declarations across 30 files** — measured by me, scoped to `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`.
⛔ **`declared`. ⛔ NOT a pass count. I did not compile and I did not run the suite.**
**Reconciliation balances against the declared deltas:** `TASK-851` `+3` (`SiegeInvisibilityTest` 19→22) → `TASK-830` sitting 1 `+8` (22→30) + `SiegeFogClampTest.cpp` `+8` as a new file from `TASK-838`'s lane → `TASK-830` sitting 2 `+2` (30→32) → **`SiegeInvisibilityTest.cpp` reads 32 on disk, and 410/30 is the arithmetic.** ✅ **`TASK-831` correctly declared `0`.** No undeclared test file landed.

---

## Rulings on the decisions each task put to me by name

**`TASK-829`** — `ESiegeVeilPolicy` as a **defaulted enum parameter** rather than a second public gather: ✅ **upheld**, and the default direction is the safe one (a forgotten argument **suppresses**). · `IsAgentVisibleTo(ETeamId, const AActor*)`: ✅ **upheld and stronger than asked** — the one-team/one-actor signature makes the argument swap **untypeable**, which closes `qa/TASK-847.md` WARN-2 **structurally**; the symmetric form survives at exactly one call site, on **one line**, carrying its named-argument comments. ⭐ **The one-line layout is load-bearing, not style** — the house helper skips lines whose trimmed form starts with `/*`, so the idiomatic one-argument-per-line wrapping would make the guard **invisible to the very test enforcing it**. ⛔ **Do not re-wrap that call.** · `GrantInvisibility()`'s zero callers: **moot — it now has exactly one**, `CompleteWitchCast`, and shipping the door early was correct (without it the suppression branch would have been unreachable code and a reviewer would have flagged *that* instead). · Two breaks in `PerformAttack` rather than one above the branch: ✅ upheld — a reviewer verifying **by symbol** must find a break beside each delivery mode. · Break **before** the act, **after** every early-out: ✅ upheld — a receiver's `TakeDamage` can re-acquire on the same call stack.

**`TASK-830`** — §3b **the enemy-team argument**: ✅ **UPHELD, AND I VERIFIED THE PREMISE AT SOURCE RATHER THAN TAKING IT.** `FSiegeInvisibilityStatics::IsVisibleTo` opens `if (ViewerTeam == TargetTeam) { return true; }` — **first and unconditional**. ⇒ `IsAgentVisibleTo(Team, Candidate)` is **true for every friendly, veiled or not**, and asking with her own team would **filter nothing while compiling and reading sensibly**. Asking through the enemy is the only phrasing with a veil in its answer, and it is the card's own sentence. ⭐ **Pinned by the suite** (`IsAgentVisibleTo(EnemyTeam,` == 1 and `IsAgentVisibleTo(Team,` == 0 inside `IsWitchVeilCandidate`) — the subtle ruling is guarded, not just documented. · §3c **the Follow-group trap**: ✅ upheld — the guard is `Type == Follow || PositionRadius <= 0.f`, and I confirmed `CanFollowHero()` returns true for `Support`, so a just-played Witch really does carry a Follow group and a `Group != nullptr` test alone would have centred her circle on the **world origin, silently, forever**. ⭐ The OR is safe **regardless** of the spawn-default premise. · §5b `CanEverAttack()` base body: ✅ upheld — CDO reads `CardID == NAME_None` ⇒ `IsVeilCaster()` false ⇒ base stays `true`, which is what `SiegeLadderClimbTest`'s CDO row pins. · §5c `CanTakeZoneOrders()` widening: ✅ upheld — **required by the feature**, since the R/F stage-3 confirm is the only writer of `PositionCenter`/`PositionRadius`; the Cleric's FOLLOW-ONLY ruling is untouched because the heal body it protected is not reshaped and the Witch does not run it. · §5d **T/E mapping**: ✅ accepted as a **declared** design call, movement-only, reversible inside one function — flagged to the manager, **not** a finding. · §5e the 0-HP heal defect: ✅ upheld, guard placement correct (one function, two callers, and a following witch is the common case). · §9.8 / §14b **the back-pointer agreement check**: ✅ upheld — and the shipped line is **stronger than §14b claims**, because it re-checks the caster's **live timer** as well as the forward pointer. · §14c two independent accessors sharing one resolver: ✅ upheld — `IsCastInProgress()` must **not** be `GetCastProgressPercent() > 0.f`, or the bar hides for the first poll of every cast. · §14d **the CDO guard**: ✅ upheld as **load-bearing** — see NIT-1. · §13 **the `WitchCastSeconds` equality pin**: ⭐ **I re-derived it myself rather than taking his word.** `SiegeInvisibilityTest` asserts `CountOccurrencesInCode(BeginBody, "WitchCastSeconds") == CountOccurrencesInCode(WholeFile, "WitchCastSeconds")` — i.e. **every read must live inside `BeginWitchCast`**. The obvious `Elapsed / WitchCastSeconds` turns it **red**. **`TASK-830` navigated it WITHOUT loosening it**: the denominator is the live timer's `GetTimerRate`, and the suite now pins `WitchCastSeconds` at **0** inside `GetCastProgressPercent`'s body so the coupling is discoverable from the file that must respect it. ⭐ **The second reason is the better one and stands alone:** `BeginWitchCast` arms with `FMath::Max(WitchCastSeconds, 0.05f)`, so for a row tuned below the floor **the tunable is the wrong denominator** and the clamp would hide the disagreement. · §16 **scoping the stored-tell ban to `SummonedUnit.h`**: ✅ **UPHELD, AND SAID NOW AS ASKED.** A **local** in `TASK-860`'s component that reads the surface is **not** a second source of truth; a **member on the actor that owns the cast** is. A header can only declare members, so the scoped ban catches exactly the defect. ⛔ **A tree-wide ban would go red the moment `TASK-860` consumes the surface — in a test naming neither `860` nor its file — and the "fix" would be to loosen it.** ⭐ Its existence control is `> 0` rather than a pinned integer, for the same reason. · §17.1 **no delegate, poll-shaped**: ✅ upheld — item (8) named exactly two getters and `WITCH-§9.3`'s contract is float/bool only; widening it would have been a law change, and a continuously-progressing cast wants a poll, not a per-frame broadcast.

**`TASK-831`** — `HP 70` / `MaxCopies 2`: ✅ accepted as declared calls (`MaxCopies` is provably inert for a `Unit` under `UNCAP-§2`; `0` would have been the wrong signal). · `Speed 350`: ✅ good — it reuses a shipped speed, so `SiegeLadderClimbTest`'s deduplicated speed set is unchanged and only a failure-message string goes cosmetically stale. · `T_CardArt_Witch` written as a full object path to a nonexistent asset: ✅ upheld — that is the shipped `TSoftObjectPtr` mechanism, degrading to a logged text-only card face, **and the agent proved the path survived the round trip rather than assuming a setter would keep it.** · `is_dirty` is blind: ✅ **concur, and the agent RETRACTED its own earlier reasoning that had leaned on it** — that retraction is worth more than the finding. Already law as `SC-§39.1`'s fifth blind instrument; the sha256-change proof is the right replacement and is why the `DT_Cards` save is believable.

**`TASK-851`** — the `names:` deviation: ✅ **the programmer was RIGHT to deviate and right to say so.** Literal obedience to the stale `FSiegeInvisibilityStatics::IsVisibleTo` instruction would have made the tree-wide count **5** against a pin of **2** and turned `AVeiledEnemyIsDroppedByTheOneAcquisitionFunnel` **RED as a direct consequence of following the spec.** · Placement of the consult **inside** the existing `IsValid && !IsUnitDead && GetTeamId == EnemyTeam` chain: ✅ upheld — short-circuited behind the team filter, so it never runs on friendlies or the dead. · The early-outs seeing the **filtered** count: ✅ correct and intended — an all-veiled field costs **less**, and the bot must not cluster on units it cannot see. · Scan 1's consult sitting **before** `IsOnOwnHalf`: ✅ behaviour-neutral, both are pure filters. · **The instrument hazard it met and reported**: ⭐ the `#include`'s **trailing `//`** comment naming the symbol sits on a **code line**, so a bare-token needle reads **4** where the truth is **3**. It pinned the **call shape** (`Symbol(`) instead. ✅ **Exactly what `SC-§39`'s mirror clause requires — and the clause exists because this task measured it.**

---

## ⛔ THE AUTOMATIC FAILS — ALL CHECKED, NONE PRESENT

✅ **No inlined `bIsInvisible = …` anywhere** — measured with the comparison count subtracted, both spellings, tree-wide: **0**. The flag is **private, unreflected, and written by exactly two functions**, both of which do nothing but hand it to `FSiegeInvisibilityStatics`. ✅ **No second copy, no mirrored bool, no *"was visible"* cache** — `ApplyBreak`'s true-once edge is what makes one unnecessary. ✅ **No timer, cooldown, `RestoreVeil`, `RefreshVeil` or `TickVeil`** — and ⭐ **no signature grew a time parameter**: the seven functions in the feature take teams, actors, a reason and a `bool&`; the one float in the feature (`WitchCastSeconds`) is a **cast** duration consumed by a single **one-shot** `SetTimer`, and the header says so in writing. ✅ **`ESiegeVeilBreakReason` is still SIX** — the `VeilBreakReasonCount` tripwire and the census row are intact. ✅ **No gating on `ESummonedUnitState`** — the cast rides the existing 0.25 s poll from **one line above every dispatch**, and the veil is never consulted from the state machine. ✅ **`ApplyRadialDamage` opts out EXPLICITLY** via `ESiegeVeilPolicy::IncludeVeiled` — **the only such call site in shipping code** — with a comment saying **why** (*a blast is not an act of seeing; it is the 50-gold card's only counter*). ✅ **The predicate is consulted inside `GatherHostileAgents` ONLY** and is re-implemented nowhere. ✅ **The friendly lane is never suppressed** — `GatherFriendlyAgents` takes **no policy parameter at all**, so it cannot be given one by accident, and the predicate's same-team early return is the second, independent reason. ✅ **The HEALER breaks, the PATIENT does not.** ✅ **Acquisition does NOT break** — no `BreakInvisibility` on either acquisition path. ✅ **Taking damage does NOT break the veil but DOES interrupt the CAST** — `TakeDamage` carries **two cancel calls and ZERO `BreakInvisibility` calls**, placed after the friendly-fire refusal and the `ActualDamage <= 0` early-out and before `HandleDeath`. ⭐ **The two rules are not merged.** ✅ **The circle is `FSiegeUnitGroup::PositionCenter`/`PositionRadius`**, never `AttackCenter`/`AttackRadius`, never `MARK-§`'s `circle_1..9`, with the ungrouped `Range` fallback present. ✅ **"One at a time" = one CONCURRENT cast** — `UpdateWitchCast` tests the live timer **before** the acquire and returns, so a second cast is **unrepresentable**; two sequential veils leave **both** units invisible, and the completion path breaks **no other unit's** veil. ✅ **Interrupt ⇒ no veil, no partial state, no cost** — `BeginWitchCast` does **nothing observable**, which is how that is guaranteed rather than remembered. ✅ **The blast still lands on a veiled cluster the bot aimed elsewhere** — `ApplyRadialDamage` untouched by `851`. ✅ **`TL-§5b`/`§5c`: declared deltas only; no spec absolute carried; no pass count written.**

---

**Gate closed 2026-09-02. Four subjects, four PASSes, zero blockers.**
⚖️ *Worth recording: of the four things this gate was told to check hardest, three were already correct and guarded by a test the implementer wrote against its own naive form — and the fourth (`FindLightningTowerTarget`) existed only because one agent refused to trust a count that was written into law.*
