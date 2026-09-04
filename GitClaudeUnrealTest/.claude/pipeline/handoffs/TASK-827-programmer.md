# TASK-827 — `SiegeInvisibilityStatics` — programmer handoff

**Status:** ready-for-qa
**Law:** `WITCH-§3` + `WITCH-§6` (cited, not restated)
**Date:** 2026-09-02

---

## ⛔⛔ READ THIS FIRST — A LANDED FILE IS NOT A LANDED FEATURE

**THIS TASK CHANGES ZERO BEHAVIOUR.** Nothing in the shipped game consults
`FSiegeInvisibilityStatics`. No unit can be veiled, no witch exists, no acquisition is
suppressed, no material changes. Three new files land; **zero existing files were touched.**

- TASK-829 owns `bool bIsInvisible` on `ASummonedUnit`, the `BreakInvisibility(...)` call sites,
  and the consult inside `GatherHostileAgents`.
- TASK-830 owns the witch's 3-second cast.

If a playtest is run after this lands and invisibility "doesn't work", **that is correct.**

---

## Files created (3 — all new, ⛔ zero existing files edited)

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeInvisibilityStatics.h` | `ESiegeVeilBreakReason` + `FSiegeInvisibilityStatics`, the measured break-site ledger and the inverse ledger |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeInvisibilityStatics.cpp` | the five bodies; includes **only** its own header (the `SiegeLadderClimbStatics.cpp` discipline) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | 8 headless tests |

**No `Build.cs` change** — same module, no new dependency (`CoreMinimal` + `Siegebound/TeamId.h`,
both already linked everywhere). **No `.generated.h`, no `UFUNCTION`, no `UENUM`, no reflection.**

**Fences honoured:** ⛔ no `SiegeCombatStatics.*` · ⛔ none of the nine acquisition sites ·
⛔ no `SummonedUnit.*` · ⛔ no `SiegePlayerController.*` · ⛔ no art · ⛔ no compile ·
⛔ no editor / MCP / Git. (TASK-828's six files and TASK-819's controller are untouched.)

---

## ⭐⭐ THE MEASURED LIST — WHAT BREAKS THE VEIL, WITH `file:line`

Read out of the shipped code, **not** paraphrased from his sentence. Six enumerators,
`WITCH-§3`'s closed set, each mapped to its real site.

### `Attack`
| site | `file:line` |
|---|---|
| `ASummonedUnit::PerformAttack` — melee damage | `SummonedUnit.cpp:3050` |
| ...armed in `EnterAttack`, synchronous first hit / cadence timer | `SummonedUnit.cpp:2753` / `:2756` |
| `ASummonedUnit::PerformAttack` — ranged (`FireProjectileAt`, def `:3516`) | `SummonedUnit.cpp:3031` |
| ...projectile actually spawned / armed | `SummonedUnit.cpp:3545` / `:3554` |
| ⭐⭐ **`ASummonedUnit::ApplyDetonation` — the SAPPER's suicide blast** (def `:4180`, compose `:4202`) | **`SummonedUnit.cpp:4209`** |
| ...entered from `UpdateStateSiege` contact / from `HandleDeath` | `SummonedUnit.cpp:2418` / `:4227` |
| `ATower::FireProjectileAt` (spawn `:337`) | `Tower.cpp:307` |
| `ATower::FireChainZapAt` (damage `:469`) | `Tower.cpp:359` |
| `AHeroCharacter::DoMeleeAttack` (damage `:611`) | `HeroCharacter.cpp:463` |

> ⭐⭐ **THE SAPPER IS THE VERB HIS SENTENCE DOES NOT CONTAIN, AND IT IS THE HEADLINE FINDING.**
> `ApplyDetonation` is **not reached through `PerformAttack`** — it has its own two entries. A
> guard placed only in `PerformAttack` would let a veiled Sapper blow a building open **while
> still invisible**, and that is exactly the report "invisibility is broken". **TASK-829 must
> hook `SummonedUnit.cpp:4209` separately.**

### `Heal`
| site | `file:line` |
|---|---|
| `ASummonedUnit::PerformHeal` — `HealTarget->ApplyHealing(...)` | `SummonedUnit.cpp:2662` |
| timer armed in `StartHealing` (`:2627`), from `UpdateStateSupport` (`:2429`) | `SummonedUnit.cpp:2619` |

> ⚠️ **DIRECTION IS LOAD-BEARING.** The **healer** breaks. The **patient** does **not** —
> `ASummonedUnit::ApplyHealing` (`SummonedUnit.cpp:2665`, HP mutation `:2675`) is the receiver
> side. Also: **a FOLLOWING Cleric still heals** (`UpdateStateFollow` shares
> `UpdateSupportHealTargeting`), and the heal runs on its own `HealTimerHandle` at 0.1 s
> **independent of `State`**.

### `Mine`
| site | `file:line` |
|---|---|
| ⭐ `AMinerUnit::UpdateMining` — `Node->TryRegisterArrivedMiner(this)` (**the arrival/claim**) | **`MinerUnit.cpp:514`** |
| `AMinerUnit::UpdateMining` — `OwnerState->AddMinerIncome()`, once per tenure | `MinerUnit.cpp:541` |
| node side: `AGoldNode::TryRegisterArrivedMiner` / claim | `GoldNode.cpp:124` / `:150` |
| node side: `AGoldNode::HandleDrainTick` (reserve drain) | `GoldNode.cpp:317` |
| player side: `ASiegePlayerState::HandleGoldTick` → `SetGold` | `SiegePlayerState.cpp:197` → `:238` |

> ⚠️⚠️ **TRAP: THE GOLD IS NEVER GRANTED ON THE MINER'S CALL STACK.** A hook on "where gold
> appears" would break the veil of **every miner the player owns, including ones still walking.**
> **Break at the arrival (`MinerUnit.cpp:514`).**

### `Empower`
| site | `file:line` |
|---|---|
| `AAncientGround::ApplyBoostTick` — `Unit->AddPermanentDamageStacks(Grant)` | `AncientGround.cpp:270` |
| the sorcerer is only **counted** here (`SorcererUnit.h:81` returns true) | `AncientGround.cpp:233` |

> ⚠️⚠️ **THE DIRECTION IS INVERTED FROM WHERE THE CODE IS, AND THIS ONE WILL BITE.**
> The **sorcerer** is the actor — but **the sorcerer executes no code at all.** It is passive:
> the *ground's* tick reads `IsAncientGroundEmpowerer()` and does the granting. A naive "break at
> the `AddPermanentDamageStacks` call" would un-veil **exactly the wrong actors** (the
> *recipients*, who are being acted upon and must stay veiled). **TASK-829 must break on the
> counted sorcerers at `AncientGround.cpp:233`, not in the `Occupants` loop.**

### `Cast`
**No shipped site exists yet** — this is TASK-830's landing pad.

> ⚠️ **REFUTED, MEASURED:** the shipped card spells (Fireball / FrostNova / Lightning / BattleCry
> / Pickpocket) are cast by a **controller**, never by a pawn. No unit, hero or pawn calls
> `USpellLibrary::ResolveSpell` (def `SpellLibrary.cpp:572`); every caller is
> `SiegePlayerController.cpp:3128` / `:4380` or `SiegeBotController.cpp:777` / `:827`. The
> HeroLine delivery uses the hero only as a geometric **origin** (`SpellLibrary.cpp:598`/`609`).
> ⇒ **the shipped spells are not break sites.**

### `Death`
| site | `file:line` |
|---|---|
| `ASummonedUnit::HandleDeath` | `SummonedUnit.cpp:4213` |
| `AHeroCharacter::HandleDeath` | `HeroCharacter.cpp:831` |

---

## ⭐ THE INVERSE — WHAT MUST **NOT** BREAK IT (each stated explicitly)

**Locomotion & posture:** walk/advance (`SummonedUnit.cpp:2760`, `:2892`) · idle/stand (`:2947`,
`MinerUnit.cpp:845`) · **ladder climb** (`SummonedUnit.cpp:3792`/`:3975`, hero
`HeroCharacter.cpp:1737`) — measured as locomotion: `MOVE_Flying` + `AddMovementInput`, and it
*disarms attacking* at all three guard points, so a unit that cannot attack while doing it cannot
be "acting" · stuck recovery: sidestep `:3265`, widen-and-repath `:3288-3290`, abandon `:3314`,
`MinerUnit.cpp:999` · **Cavalry charge wind-up** `:3081` (running, not hitting — the multiplier is
consumed later at `:3389`) · hero sprint `HeroCharacter.cpp:446`/`:457` · facing a target `:3501`.

⭐⭐ **ACQUIRING A TARGET** (`SummonedUnit.cpp:1654`, `:2199`) — **the subtle one.** A veiled unit
may walk up to an enemy, pick it, close to range and raise its weapon and **stay veiled**.
Acquisition is *seeing*, not *acting*. Breaking here would un-veil a full approach early.

**Being acted upon (his rule breaks on ACTING, not on being acted upon):**
taking damage (`SummonedUnit.cpp:4086`, `HeroCharacter.cpp:721`) — ⚠️ it **does** interrupt an
in-progress witch *cast* (`WITCH-§4`, TASK-830), a **different rule on a different subject; do not
merge them** · **being healed by someone else** (`SummonedUnit.cpp:2665`) · being buffed
(`ApplyMoveSpeedBuff` `:744`, `SetAuraDamageBonus` `:810`, `ApplyCombatBuff` `:1042`,
`AddPermanentDamageStacks` `:881`) · being frozen (`:916`, `:661`) · **being caught in an AoE**
(`WITCH-§2`/`J-W2`) · move blocked (`:3326`).

**Being commanded (`WITCH-§2` lane 4 — an unorderable unit is a BUG):**
being ordered (`AssignCommandGroup` `:2111`, `ClearCommandGroup` `:2145`,
`TryAutoEnrollInFollowGroup` `:1318`, stance read `:1594-1612`) · being selected (controller
state; the unit does nothing) · being spawned (`InitUnit` `:622`).

**Swept for and confirmed absent from the shipped game:** taunt · revive/resurrect · unit-side
repair (Masons is a *player* spell on the *castle*, `Castle.cpp:1218`/`:1414`) ·
shield/block/parry/dodge (the "Shield Wall" at `SummonedUnit.cpp:1582` is a **stance**) · item
pickup · flag planting · unit-deployed ladders (`AClimbableTower` is pre-placed) · ram/batter as a
distinct verb (Siege-typed *melee*, routes through `:3050`) · unit-side summoning · unit-side
building placement · generic ability cooldowns.

---

## ⚖️ FLAGGED — THINGS I AM NOT CERTAIN BELONG (ruled, but escalate if you disagree)

1. **⚖️ CAPTURING A ZONE — ruled DOES NOT BREAK.** `ACaptureZone::EvaluateCapture`
   (`CaptureZone.cpp:136`) is a **zone-side poll** reading only `GetActorLocation()` of every unit
   (`:150-169`) and hero (`:171-190`). There is no unit-side call, no channel, no progress bar —
   capture is **presence**, indistinguishable from standing still. Breaking here would un-veil
   every unit that merely *walks through* a zone, which is the main route across the field.
   ⚠️ **ACCEPTED SIDE EFFECT:** a veiled unit **still captures**, so a zone flipping with no
   visible cause is a real (deliberate) information leak. **This is a design question for
   Jonathan, not a code fix.**
2. **⚖️ THE HERO'S RECALL CHANNEL** (`HeroCharacter.cpp:1457`/`:1493`/`:1525`) — a real 3-part
   channel with its own attack-disarm (`:1407`, consumed `:490`). Ruled **out of scope**: it is
   **hero-only** and `WITCH-§4` targets "nearest friendly **UNIT**", so it is unreachable today.
   ⚠️ **It does not fit any of the six names.** If TASK-830 ever rules the hero veilable, recall
   needs a ruling and that is a **manager amendment to `WITCH-§3`, never a quiet 7th enumerator.**
3. **⚖️ HERO Rally (`HeroCharacter.cpp:706`) / War Banner pulse (`:1259`)** — these *are* "powering
   up something", mapped to `Empower`, but **dormant** unless the hero becomes veilable.
4. **⚖️ TOWERS** (`Tower.cpp:307`/`:359`) — listed under `Attack` for completeness; towers are not
   veilable today.
5. **⚖️ `AHeroCharacter::ApplyUpgrade` (`:953`)** — excluded per `WITCH-§3` ("the hero's
   `HeroUpgrade` cards are the hero's, not a unit's"). Stated so the exclusion is visible.
6. **⚖️ `ABarracks::SpawnUnit` (`Barracks.cpp:80`) / `ADeepMine::TryRegisterIncome`
   (`DeepMine.cpp:60`)** — buildings, not units. Excluded.

---

## The state shape TASK-829 consumes

```cpp
enum class ESiegeVeilBreakReason : uint8 { Attack, Heal, Mine, Empower, Cast, Death };

struct FSiegeInvisibilityStatics
{
    static bool IsVisibleTo(ETeamId ViewerTeam, ETeamId TargetTeam, bool bTargetIsInvisible);
    static bool ApplyVeil(bool& bIsInvisible);                                  // ONLY write-true
    static bool ApplyBreak(bool& bIsInvisible, ESiegeVeilBreakReason Reason);   // ONLY write-false
    static constexpr int32 VeilBreakReasonCount = 6;
    static const TCHAR* ToString(ESiegeVeilBreakReason Reason);
    static const TCHAR* UnrecognisedReasonToken();
};
```

- **`bool&`, not a struct.** `WITCH-§6` says `bIsInvisible` is **one** source of truth living on
  `ASummonedUnit`; a `FSiegeVeilState` wrapper would be a second shape. TASK-829 passes its own
  member by reference.
- **`ApplyBreak` returns true exactly once** — on the true→false edge. That is what lets
  `BreakInvisibility` fire its one-shot side effects (material swap, log) **without a
  "was visible" cache**, which `WITCH-§6` bans.
- **`IsVisibleTo` checks same-team FIRST and unconditionally.** TASK-829 calls it **inside
  `GatherHostileAgents` only** (`WITCH-§1`), and must **exempt `ApplyRadialDamage`** (`J-W2`).
- **Plain enum, not a `UENUM`** — the `ESiegeLadderExit` / `ESiegeStuckAction` discipline
  (`SummonedUnit.h:84`). It is never a `UPROPERTY`, and an unreflected type cannot be replicated
  by accident — which matters here because `WITCH-§6`'s M8 declaration records that a naive
  replicated broadcast of the flag **leaks the veiled unit's position to the enemy client's
  renderer**. If TASK-829 needs it reflected, that is a deliberate amendment with the leak
  re-argued, not a convenience.

## How "permanently" is enforced (so it cannot drift into a cooldown)

1. **⭐ NO FUNCTION IN THE FILE TAKES A TIME PARAMETER.** No `DeltaSeconds`, no `Duration`, no
   `Seconds`, no clock, no handle, and no state that could hold one. **A cooldown is
   *unrepresentable* in this API** — drifting into one requires changing a signature, which is a
   review event rather than a quiet edit.
2. **No inverse exists.** There is deliberately no `RestoreVeil` / `RefreshVeil` / `TickVeil`. The
   only write-true site is `ApplyVeil` (TASK-830's cast completion); the only write-false site is
   `ApplyBreak`. **A grep for veil writes finds exactly two functions, both in this file.**
3. **`ApplyBreak` is monotone and idempotent** — once false, every later call is a no-op returning
   false, regardless of `Reason`.
4. **`Reason` is a LABEL, not a condition** — deliberately unused by the transition
   (`(void)Reason;`). Branching on it would re-open the closed set through the back door.
5. **The type system is the predicate.** An action that does not break has **no enumerator**, so
   it *cannot be passed*. There is deliberately no `bool ActionBreaksVeil(...)` classifier: a
   classifier can be called with the wrong argument; a missing enumerator cannot compile.

---

## Test list — `Tests/SiegeInvisibilityTest.cpp` (8 tests)

| # | test | what fails it |
|---|---|---|
| 1 | `Siegebound.Invisibility.VisibilityPredicateTruthTable` | all 2×2×2 inputs vs a **hand-written literal** table (not the formula re-derived); plus an anti-degeneracy guard pinning 6 VISIBLE / 2 HIDDEN so a flattened table fails |
| 2 | `Siegebound.Invisibility.FriendlyVeilNeverSuppressed` | the `WITCH-§2` lane-4 bug guard, both teams, **plus a cross-team discriminator** so a `return true` cannot satisfy it |
| 3 | `Siegebound.Invisibility.VeilGrantIsIdempotentAndReportsTheEdge` | double-grant must be a measurable no-op that never un-veils |
| 4 | `Siegebound.Invisibility.BreakIsExactlyOnceAndNeverSelfRestores` | edge fires once; 6 repeat breaks with **different reasons** never restore and never re-fire |
| 5 | `Siegebound.Invisibility.EveryBreakReasonClearsTheVeil` | each of the 6, on a **fresh** subject, and the predicate is re-checked so a flag flip that doesn't change visibility fails |
| 6 | `Siegebound.Invisibility.BreakOnAnUnveiledUnitIsANoOp` | there is no "un-break" |
| 7 | `Siegebound.Invisibility.BreakReasonCensusIsClosed` | ⭐ **the absence gate** — see below |
| 8 | `Siegebound.Invisibility.AFreshWitchCastReVeilsAfterABreak` | his parenthetical, veil→break→re-veil→break, predicate checked at each stage |

### ⭐⭐ The control discipline (TASK-827 asked for this explicitly)

**A veil-break assertion passes trivially if the unit was never veiled.** Every break test here
does **both**:
1. **asserts the veil is actually ON before breaking** — tests 4 and 8 `AddError` + `return false`
   if `ApplyVeil` did not set the flag, so a broken grant fails *loudly* instead of turning six
   `TestFalse` calls green; test 5 asserts the precondition per-iteration and `continue`s rather
   than emitting a trivially-passing assertion;
2. **carries a CONTROL VEIL** — a second bool veiled at the same moment, **never** passed to
   `ApplyBreak`, asserted **still true** at the end (tests 4, 5, 8). An implementation that
   clobbers every veil, or one where `ApplyVeil` silently no-ops, **cannot pass.**

### ⭐⭐ The absence gate (test 7) — how "no `Walk`" survives a future edit

`VeilBreakReasonCount == 6`; every index `0..5` maps to its expected name **in declaration
order** (so a *reorder*, which would silently re-label every log line TASK-829 writes, also
fails); **index 6 — one past the end — must still return the unrecognised token.** `ToString` has
**no `default:` label**, so adding a 7th enumerator *even with a matching case arm* turns index 6
into a real name and **the test goes red**. Finally, the six shipped names are asserted **not** to
be `Walk`, `TakeDamage` or `Order`. ⇒ **absence is enforced, not merely commented.**

---

## ⭐ SUITE DELTA — AS A COUNT

**+8 tests.** (8 new `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, one per test, each at line start
so a grep census counts them correctly.)

⚠️ **The absolute census is a moving target — the wave is live.** Measured in
`Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`:
- **338** at the start of this task (26 files);
- **361** at the end (27 files) — of which **8 are mine**, so **353** came from the other 26.
- ⇒ concurrent tasks (TASK-824/811/814/819) added **+15** while this task ran.
**+8 is the number that is mine.** Do not read 338→361 as this task's delta.

---

## What QA should scrutinise

1. **⭐ THE ENUMERATION ITSELF — this is the real deliverable.** Is the six-name mapping right, and
   did I miss a verb? Re-check `SummonedUnit.cpp:4209` (Sapper) and `AncientGround.cpp:233`
   (sorcerer counted, not calling) in particular — those two are the ones a wiring task gets wrong.
2. **⚖️ The three judgment calls above** (capture-by-presence, hero recall, hero Rally/War Banner).
   The capture ruling has an accepted information leak; if QA disagrees, it goes to the manager,
   **not** into a 7th enumerator.
3. **`IsVisibleTo` argument order** — `(Viewer, Target, bTargetIsInvisible)`. Two `ETeamId`s in a
   row is a swap hazard at the call site. The truth table is asymmetric in rows 4 and 6, so a
   swapped call **would** be caught by TASK-829's tests, but flag it if you want a stronger shape.
4. **`ApplyBreak`'s unused `Reason` (`(void)Reason;`)** — deliberate, documented. Confirm you agree
   it is a label and not dead surface.
5. **Plain enum vs `UENUM`** — I chose plain per the `ESiegeLadderExit` precedent and the M8
   leak declaration. If TASK-829 needs Blueprint exposure, this is the one decision to revisit.
6. **Compile risk (I could not compile — `QUIET-MODULE`).** I verified every assertion signature
   against `UE_5.8/.../Misc/AutomationTest.h`: `TestTrue`/`TestFalse` (`:2603`/`:2367`, both
   `TCHAR*` and `FString` forms), `TestEqual(TCHAR*, int32, int32)` (`:1985`),
   `TestEqual(TCHAR*, const FString&, const FString&)` (`:1997`), the templated
   `TestEqual(FString, ValueType)` (`:2197`) and `TestNotEqual(FString, ValueType)` (`:2422`).
   **`TestEqual` on a pair of bools was deliberately avoided** (bool converts to
   `int32`/`int64`/`SIZE_T`/`float`/`double`, all overloaded) — test 1 uses `TestTrue`/`TestFalse`
   instead. Encoding checked: all three files are **UTF-8 with no BOM**, byte-identical in their
   first three bytes to the shipped emoji-carrying headers.
7. **One-class-per-header** — the enum + struct share the header under the shipped "pure data
   types may share a header when they form one concept" exception (`TeamId.h` precedent;
   CONVENTIONS records it for `SiegeStuckStatics.h` and `SiegeKeyboardLayoutStatics.h`).
   `WITCH-§6` names this one file as the home of both. **Not a violation.**

## Notes for TASK-829 (traps found while measuring)

- **`ESummonedUnitState` is only `{Idle, Advance, Attack}`** (`SummonedUnit.h:45-53`). **Heal and
  mine run on their own private timers while `State == Advance`.** Gating the veil on
  `State != Attack` **will leak.**
- **A climbing unit's `UpdateState` early-returns** (`SummonedUnit.cpp:1427`), so a climber runs no
  decision loop at all and the climb steers the movement component directly.
- **`AGoldNode` deliberately does not implement `ITeamAgent`**, so mining nodes are never caught by
  `ApplyRadialDamage` — noted because TASK-829 touches that function's neighbourhood.
