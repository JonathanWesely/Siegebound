# TASK-830 — THE WITCH'S 3-SECOND INTERRUPTIBLE CAST + POSITION-CIRCLE TARGETING — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-849` · **Law:** `WITCH-§0`/`§2`/`§3`/`§3a`/`§4`/`§6`/`§7`, `SC-§38`, `HIGH-§1`, `TL-§5c`
**Blocker consumed:** `TASK-829` (the veil state + break call — `GrantInvisibility()` shipped with zero callers as my landing pad)
**Compile / editor / MCP / Git:** ⛔ none touched. The editor was live under an art task for this whole task.
**Fence honoured:** ⛔ `SiegeCombatStatics.{h,cpp}` and `SiegeFogStatics.*` **read only** (`TASK-838` was writing `SiegeCombatStatics.cpp` while I worked — its mtime moved mid-task) · ⛔ `SiegeBotController.*` untouched (`TASK-851`, which has since landed its three `IsAgentVisibleTo` calls) · ⛔ no card row, mesh, material or card art.

---

## 1. WHAT SHIPPED, IN ONE PARAGRAPH

A witch is now a real, commandable unit that acquires the nearest still-visible friendly inside her position circle, channels for `WitchCastSeconds = 3.0`, and on completion calls **`Subject->GrantInvisibility()`** — the **only** caller of that door in the project, which is what turns `TASK-829`'s suppression branch from unreachable code into the feature. The cast is driven by **one line** in `UpdateState`, placed above every dispatch (the `TickStuckWatchdog` idiom), so it runs identically under Follow, Hold, Ambush, a latched stance and no command at all, with **zero new timers on the poll side**. Damage to **either** actor interrupts it; the subject dying, being veiled by someone else, leaving the circle, or the witch being re-ordered cancels it; and an interrupt leaves **no veil, no partial state, no cost**. Eight new automation tests pin the shape, including the one row that is easy to get backwards (the veil term is asked with the **enemy's** team) and the trap I found while wiring (a witch would have un-veiled herself every 0.1 s by "healing" for 0 HP).

---

## 2. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | `IsVeilCaster()` (public, virtual, beside `IsAncientGroundEmpowerer()`); `CanEverAttack()` base body now `!IsVeilCaster()`; 2 `EditDefaultsOnly` tunables; 11 private declarations; 3 private members |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | the `WitchCardID` file-scope constant; 11 function bodies; 8 wiring edits (`UpdateState` ×3, `UpdateSupportHealTargeting`, `CanTakeZoneOrders`, `TakeDamage`, `HandleDeath`, `AssignCommandGroup`, `FreezeAI`, `ApplyFreeze`, `EndPlay`) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | **+8 tests (22 → 30)** and the **two rows `TASK-829` instructed by name** — see §7 |

⛔ **Not touched:** `SiegeInvisibilityStatics.*` (`TASK-827`'s, gated PASS, consumed unchanged) · `SiegeCombatStatics.*` · `SiegeFogStatics.*` · `SiegeBotController.*` · `MinerUnit.*` · `AncientGround.*` · `Tower.*` · `HeroCharacter.*` · `UnitCommand.h` · `CardRow.h` · `SiegePlayerController.*` · `Docs/Data/cards.csv` · any asset.

---

## 3. ⭐⭐ THE FOUR THINGS HIS SENTENCE PINS — MY ANSWER AND WHY

### 3a. "nearest" — measured **when the cast BEGINS**, and the target is then **held**

**Chosen: at BEGIN.** A subject that walks away mid-cast **cancels the cast**; it does **not** cause a silent re-pick.

The distinguishing case decides it. `WITCH-§4` states, in his own ruling, that *"a cast also cancels if the target dies, **leaves the circle**, or the witch is ordered away."* ⛔ **If "nearest" were re-evaluated at completion, that clause would be unreachable code** — a subject leaving the circle would simply be replaced by whoever is nearest now, and the cast would always succeed on somebody. The cancel rule only means anything if the cast is *bound* to one subject for its whole duration. An interruptible 3-second channel that silently swaps targets is also not a channel; it is an instant spell with a delay.

⇒ `BeginWitchCast` latches `WitchCastTarget`; `UpdateWitchCast` **re-validates that same subject** every 0.25 s poll and `CompleteWitchCast` re-validates it once more against the **live** circle before granting. Ranking is from the **witch**, not the circle's centre (his *"walk up to **nearby** units"* — ranking from the centre would send her past a unit at her elbow).

### 3b. ⭐⭐⭐ "visible" — read through the shipped rule, and it is asked with the **ENEMY'S** team

**This is the row I would put in front of QA first, because the naive form compiles, reads sensibly, and filters nothing.**

`FSiegeInvisibilityStatics::IsVisibleTo` checks **same-team first and unconditionally** (`WITCH-§2` lane 4 — an invisible unit its own player cannot see is a BUG). ⇒ **measured consequence:** `FSiegeCombatStatics::IsAgentVisibleTo(Team, Candidate)` — the obvious call, with the witch's own team — is **`true` for every friendly, veiled or not**. A witch written that way would burn three seconds re-veiling somebody already invisible, on every poll, forever.

The only phrasing with a veil in its answer is the enemy's:

```cpp
const ETeamId EnemyTeam = (Team == ETeamId::Blue) ? ETeamId::Red : ETeamId::Blue;
if (!FSiegeCombatStatics::IsAgentVisibleTo(EnemyTeam, Candidate)) { return false; }
```

⭐ And it is not a trick — it *is* the card's sentence: **she veils the units the enemy can still see.** `ETeamId` has exactly two values (`TeamId.h`), so the mapping is total.

⛔ **Deliberately NOT `Candidate->IsInvisible()`.** That would be a second expression of a rule `WITCH-§1` exists to hold at one, and it would not inherit the day "visible" grows a term. **I added a *consumer* of `IsAgentVisibleTo`, never a second guard point** — the suite still asserts the symmetric predicate is named exactly twice tree-wide (test 27), and I measured that after `TASK-851`'s three bot calls landed.

### 3c. "its position circle" — `FSiegeUnitGroup::PositionCenter` / `PositionRadius`, with the `J-W5` fallback beneath it

`ResolveWitchPositionCircle` writes the `J-W5` fallback **first** (self-centred, row `Range` via `AttackRange`, `WitchVeilRadiusFallbackUU` only as the `Range`-0 backstop), then upgrades to the group's **POSITION** zone. ⛔ Never `AttackCenter`/`AttackRadius` (that is the tier-1 engage trigger and a witch never engages); ⛔ never `MARK-§`'s `circle_1..9`. No new radius concept was invented.

⭐⭐ **THE TRAP, AND IT IS THE COMMON CASE RATHER THAN AN EDGE ONE — QA please check this row hardest:** `FSiegeUnitGroup` is **reused unchanged** for Follow, and a Follow group carries `PositionRadius == 0` with `PositionCenter == ZeroVector` (`UnitCommand.h` says so at the struct). **Follow is the SPAWN DEFAULT for every follow-eligible Blue unit, and the witch is follow-eligible (Support).** ⇒ the overwhelmingly common witch **has a group and has no circle**, and a resolver that tested only `Group != nullptr` would centre her circle on the **world origin** — she would never veil anybody, silently, forever, with nothing in any log. Both terms are guarded (`Type == Follow` **and** `PositionRadius <= 0.f`); test 26 pins the Follow term specifically.

### 3d. "one at a time" — one **concurrent cast**, and the latch is the live timer

`UpdateWitchCast` tests `IsCastingVeil()` **before** the acquire and returns, so a second cast is **unrepresentable** rather than guarded. `IsCastingVeil()` reads `GetWorldTimerManager().IsTimerActive(WitchCastTimerHandle)` — ⛔ **no second bool**: one left true by an early return seals the witch forever; one left false lets her double-cast. Test 25 asserts the **ordering** (latch index < acquire index, on code lines only).

⛔ `J-W7`'s other half is honoured structurally: a witch who veils A then B leaves **both** invisible. The completion path never touches a previously veiled unit, and test 23 asserts `Subject->BreakInvisibility` appears **zero** times there.

---

## 4. THE CAST, AS A STATE MACHINE (what QA is reviewing)

| seam | what it does |
|---|---|
| `UpdateWitchCast()` | **one call** from `UpdateState`, above every dispatch. Validate-or-acquire. No-op after one compare for every non-witch. |
| `BeginWitchCast(Subject)` | latches both ends of the link, arms the **one-shot** timer. ⛔ **Nothing observable happens** — that is how "no partial state" is *guaranteed* rather than remembered. |
| `CompleteWitchCast()` | re-validate → `ClearWitchCastChannel()` → `Subject->GrantInvisibility()` → `BreakInvisibility(ESiegeVeilBreakReason::Cast)` **on herself** |
| `CancelWitchCast(Reason)` | idempotent teardown + a Verbose line naming **which** rule killed it |
| `InterruptIncomingWitchCast(Reason)` | the **subject** half — his sentence names both actors |
| `ClearWitchCastChannel()` | the **one** teardown, so neither end of the link can be freed by one path and leaked by the other |

**Interrupt / cancel sites, all of them:**

| site | call(s) | why |
|---|---|---|
| `TakeDamage` | `CancelWitchCast` **+** `InterruptIncomingWitchCast` | his *"if the witch **OR** unit that is turning invisible are attacked"* — two different objects, no single call covers both. Placed **after** the friendly-fire refusal and the `ActualDamage <= 0` early-out, **before** `HandleDeath`. |
| `HandleDeath` | both | subject dies ⇒ cancel (`WITCH-§4`); witch dies ⇒ abandon |
| `AssignCommandGroup` | `CancelWitchCast` | *"the witch is ordered away"* — at the **press**, not on a poll, because the order moves the circle out from under the subject. ⛔ Not `ClearCommandGroup` (that is also the null-group self-heal). |
| `FreezeAI` | both | ⛔ **without it this is the ONE timer in that function that would survive match end and fire onto the end screen** — it is neither `StateTimerHandle` nor `AttackTimerHandle` |
| `ApplyFreeze` | `CancelWitchCast` only | a frost-frozen witch's cast **dies, it does not pause** (a resumed cast would have channelled through the freeze). ⛔ One end only: a frozen *subject* is still a legal subject — being frozen is being **acted upon**, which `WITCH-§4` does not list among its cancels. |
| `EndPlay` | both | releases a surviving subject's back-pointer now rather than on the next poll |

⛔ **There is not one `BreakInvisibility` call at any of those sites.** `WITCH-§3` states the two rules on one row and warns in writing not to merge them; test 24 asserts `TakeDamage`'s body carries **zero**.

---

## 5. THE DESIGN DECISIONS THAT ARE MINE (⚠️ QA: rule on these)

### 5a. ⚖️ `ECardProfile::Support`, and identity by **CardID** rather than a subclass
**Profile:** `Support`, per the spec — she never attacks, she is follow-eligible for free, and the Cleric's "walk at the friendly you are working on" body is her body with one noun changed. ⛔ No fourth profile.

**Identity:** `virtual bool IsVeilCaster() const { return CardID == WitchCardID; }`. Every other mechanic identity in this class hangs on a C++ subclass (`ASorcererUnit`, `AMinerUnit`) — **the witch has none**: `WITCH-§6`'s file map and this task's names list are both `SummonedUnit.{h,cpp}` and neither contains an `AWitchUnit`, so a new class file is outside this task. Identity therefore resolves off the **one name that already selects her Blueprint** (`BP_Unit_<CardID>`), which is exactly the key `ASiegePlayerController::MinerCardID` uses for a shipped gameplay branch and that `AMinerUnit`'s constructor writes for its own class. ⭐ It is `virtual` so a future `AWitchUnit` overrides it with `return true;` and **nothing here changes**. ⛔ Not a CSV column — the mechanic-rules-aren't-card-stats law.

### 5b. ⚠️ `CanEverAttack()`'s base body changed to `return !IsVeilCaster();`
This is the one edit that touches a shipped predicate's body rather than adding beside it, so it is called out.

- **Why:** it is what makes a **zone-ordered** witch behave with zero new code — `UpdateStateGrouped`'s guard 2 forces her target null and she falls through to tier-3 station-keeping inside her position circle. That is the **shipped sorcerer path**, i.e. behaviour Jonathan has already played.
- **Why it is safe:** `IsVeilCaster()` is false for every shipped unit, so every existing body is byte-unchanged. The **CDO** reads `CardID == NAME_None` ⇒ **false** ⇒ `ASummonedUnit`'s default stays `true`, which is what `SiegeLadderClimbTest.cpp:891` pins. `ASorcererUnit`/`AMinerUnit` override to `false` regardless.

### 5c. ⚠️ `CanTakeZoneOrders()` widened by one OR-term — **required by the feature, not a convenience**
`return Profile == ECardProfile::Standard || IsVeilCaster();`

⛔ The R/F stage-3 confirm is the **only** thing in the game that ever writes `PositionCenter`/`PositionRadius`. **Without this term the position circle could never be non-zero for a witch, `WITCH-§4`'s central ruling would be dead code, and every cast would silently take the `J-W5` fallback forever.** The manager's FOLLOW-ONLY ruling for the Cleric stands exactly as written — it was protecting `UpdateStateSupport`'s heal body from being reshaped, and the witch does not run that body (§5e). Test 30 asserts both the new term and that the `Standard` term survives.

### 5d. ⚖️ The command surface — **all five, and how each one lands**
| command | how she takes it |
|---|---|
| **select** | ordinary unit, nothing needed |
| **Follow (C)** | free — `CanFollowHero()` already covers `Support`; it is her spawn default |
| **Hold (R)** / **Ambush (F)** | §5c + the shipped `UpdateStateGrouped` tier-3 sealed path |
| **Attack (T)** / **Defend (E)** | ⚖️ **read inside her own body, not by widening the shared stance gate.** That gate is `Profile == Standard` and a Support unit has never entered it; widening it would hand a 0-damage unit the whole acquire-and-march machine. Instead `UpdateStateWitch` step (3) reads the latched stance itself and uses it for the **last** goal rung only: Defend ⇒ own castle, Attack ⇒ enemy castle — the same two goals `UpdateStateStandardCommanded` uses, via the same two shipped finders. A witch with a subject or an escort keeps doing her job under every stance; the stance decides where a **lone** witch walks. |

⚠️ **Declared:** this is my mapping, not his words. The stances are movement-only for her because she cannot fight. Reversible inside one function.

### 5e. ⭐⭐ A DEFECT I FOUND WHILE WIRING, AND IT IS NOT A SMALL ONE
**The witch is `ECardProfile::Support`, the Cleric's heal RATE is the row `Damage`, and her row `Damage` is 0.** Without a guard she arms the heal timer beside any damaged friendly and `PerformHeal` calls **`BreakInvisibility(Heal)` every 0.1 s to deliver zero HP**. ⇒ a veiled witch un-veils herself instantly, for an act with **no observable effect**, and the report reads *"the witch cannot stay invisible"* with nothing in the heal code wrong.

Guarded at `UpdateSupportHealTargeting`'s **top** — that function has exactly two callers (`UpdateStateSupport` and the FOLLOW body) and **a following witch is the common case**, so one guard covers both roads. `StopHealing()` + drop the target rather than a bare `return`, so no earlier-armed timer survives. The Cleric's four extracted statements are byte-unchanged and the guard is false for it. Test 29 pins the guard, its **ordering** above `FindNearestDamagedFriendly()`, and — as its own stake — that `PerformHeal` really does carry the break.

### 5f. `TActorIterator<ASummonedUnit>` rather than `GatherFriendlyAgents`
Spec (1) says the targeting is the **Cleric's shipped shape**, and `FindNearestDamagedFriendly` is that shape. The friendly gather returns `AActor*` over every `ITeamAgent` (castles, buildings, the hero) and would cost a `Cast` per candidate to get back where this iterator starts. ⚠️ Nothing is being smuggled around `WITCH-§1`'s funnel: this is the **friendly** lane, which `WITCH-§2` rules is *never* veil-suppressed, and the veil term in this search is the explicit `IsAgentVisibleTo` call.

### 5g. The `WitchVeilRadiusFallbackUU` reading
`J-W5` says the ungrouped fallback is *"the card's own `Range` column"*, and `WITCH-§6` names `WitchVeilRadiusFallbackUU` as a tunable. I read those as **row first, constant as the backstop**: `(AttackRange > 0.f) ? AttackRange : WitchVeilRadiusFallbackUU`. Default `400.f` = the Cleric's shipped ring, and the number `TASK-831` should write into the `Witch` row's `Range` cell. ⚠️ Without the constant a `Range`-0 row yields radius 0 ⇒ zero candidates ⇒ a 50-gold card that silently does nothing.

---

## 6. ⚠️⚠️ DECLARED RESIDUALS — said out loud rather than left for QA to find

1. **The lone veiled sorcerer un-veils itself** (`TASK-829` §5.1). ⛔ Untouched, as instructed.
2. **A veiled unit still captures a zone** (`J-W9`). Untouched.
3. **There is no cast range separate from the circle.** A player who draws a large Hold circle lets the witch veil across it without walking to the subject. That is exactly what his sentence says (the circle **is** the leash), and her body walks at the subject anyway — but it is a behaviour a big circle makes visible. Reversible by adding a `min(circle, AttackRange)` clamp if he dislikes it; I did **not** add an undeclared clamp.
4. **She keeps walking while channelling.** His sentence gives the cast an *animation*, not a stop. There is no movement seal, so she closes on her subject during the three seconds. If the animation needs her planted, that is a movement seal plus the art lane's clip — flagged, not absorbed.
5. **A climbing witch's cast is not re-validated.** `UpdateState` early-outs on `IsClimbing()`, so the 0.25 s validation pass is skipped for the climb; the one-shot timer still fires and `CompleteWitchCast` re-validates once against the live circle. Worst case is up to one cast's worth of stale validation on a ladder.
6. **The render half is absent** (`WITCH-§5`'s `MI_Unit_Invisible`, `TASK-832`). A veiled unit is invisible to enemy acquisition and to the bot, and **looks completely normal** to its owner. ⛔ Green tests are not "the witch works".
7. **No test drives a witch over a populated world.** The house rule forbids `SpawnActor`/`CreateWorld` in `Siegebound/Tests/`, and a cast is a timer on an actor in a world. The suite pins the **shape**; the behaviour needs a PIE pass with a `Witch` card, which cannot exist before `TASK-831`/`833`/`834`.
8. **`WitchCardID` is a magic name.** If `TASK-831` writes the row as anything but `Witch`, character-for-character, the card spawns a unit that never casts — with nothing in any log. `WITCH-§6` pins it; I am naming the coupling anyway.

---

## 7. THE TEST FILE — MY DELTA AND THE TWO ROWS `TASK-829` ASKED FOR BY NAME

`Tests/SiegeInvisibilityTest.cpp` is outside my names list. I edited it for the same reason `TASK-829` edited `SiegeAcquisitionFunnelTest.cpp`: **the rows instruct it in their own comments.**

- **The census `Sites[]` gains a `Cast` row** (`SummonedUnitCpp`, expected **1**).
- **The tree-wide break total: `7` → `8`.**
- **The `Cast`-is-uncalled row: INVERTED, not deleted** — its own text said *"⚠️ When 830 lands this becomes 1 and this row is updated **DELIBERATELY**."* It now asserts exactly 1, **and** that it lives in `CompleteWitchCast`, **and** that `BeginWitchCast` carries neither a break nor a grant (the interrupted-cast-pays-nothing rule).

**+8 new tests (22 → 30):**

| test | fails when |
|---|---|
| `TheCompletedWitchCastIsTheOnlyWayAUnitBecomesInvisible` | the grant door has 0 callers (the feature is inert again) or 2 (a second way to become invisible); or the completion path breaks another unit's veil (`J-W7`'s wrong reading) |
| `DamageInterruptsTheCastForBothActorsAndBreaksNoVeil` | ⭐ only **one** of the two interrupt calls is present — the naive reading that ships a card where shooting the *subject* does nothing; or a "for symmetry" `BreakInvisibility` appears in `TakeDamage` |
| `OneCastAtATimeAndTheSubjectIsRevalidatedEveryPoll` | the latch is tested **after** the acquire; the cancel becomes a retarget; a new order stops cancelling, or starts breaking a veil |
| `ThePositionCircleIsTheGroupPositionZoneWithTheRangeFallback` | ⭐⭐ the **Follow** term is dropped (circle centred on the world origin); the ATTACK zone is used; the `J-W5` fallback is lost |
| `TheWitchTargetsTheNearestUnitTheEnemyCanStillSee` | ⭐⭐⭐ the veil term is asked with **her own** team (filters nothing); or re-expressed as an inline `IsInvisible()`; or `!= this` is dropped (a witch who can start a cast and never finish one); or a third `IsVisibleTo` guard point appears |
| `TheCastHasADurationAndTheVeilStillDoesNot` | the cast clock is read outside `BeginWitchCast`; the timer loops; it is armed at a second site; the latch stops reading the live timer; either exit skips the shared teardown |
| `TheWitchNeverEntersTheHealLaneThatWouldUnVeilHer` | §5e's defect returns, or the guard sinks below the shipped four statements |
| `TheWitchIsZoneOrderableAndCastsUnderEveryCommand` | §5c's term is dropped (`WITCH-§4` becomes dead code); the driver moves below a dispatch (dead for four of five commands); her body moves above the zone dispatch |

⭐ **The instrument hazard `TASK-829` measured was honoured, not assumed away.** `CountOccurrencesInCode` skips any line whose trimmed form starts with `/*`. My `SetTimer`'s `/*bLoop=*/ false` sits on a line that **also carries code** (the `FMath::Max` argument), and test 28 asserts `/*bLoop=*/` is visible (**1**) *before* asserting `/*bLoop=*/ false` — a positive control for the exact needle, so a zero can never be mistaken for a clean result. Every whole-tree scan carries a control in the same pass, and both ordering probes run on `CodeLinesOnly` text so prose naming a token cannot fool them.

**I verified every expected count against the shipped source before writing it** — extracted each function body with the test's own algorithm and counted with the test's own comment-skipping rule. All 30+ expectations match. One (`ClearWitchCastChannel()` tree-wide) came back **3** rather than the 4 I first wrote; rather than change the number I **changed the shape** to "both exits use it" (asserted per-body), which cannot rot when a third exit is added for a good reason.

---

## 8. SUITE DELTA (`TL-§5c` — a **declared** delta, ⛔ never a pass count I did not execute)

- **My delta: `+8` tests, all inside `Tests/SiegeInvisibilityTest.cpp` (22 → 30). No new test file.**
- **Tree census, measured with `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Siegebound/Tests/*.cpp`:** **392 declared across 29 files** at my start → **408 declared across 30 files** at my end.
- ⚠️ **`+16` moved, and only `+8` is mine.** The other `+8` is a **new file that landed during my task**: `Tests/SiegeFogClampTest.cpp` (8 tests, `TASK-838`'s lane, which was writing `SiegeCombatStatics.cpp` while I worked). ⛔ **Trust my `+8`; verify the rest against that file, not against me.**
- ⛔ **Declared, not executed** — no compile, no editor, no suite run in my lane.

---

## 9. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **§3b — the enemy-team argument.** This is the whole "visible" ruling and the naive form is invisible to review. Confirm you agree that `IsAgentVisibleTo(Team, Candidate)` is unconditionally true for a friendly (`FSiegeInvisibilityStatics::IsVisibleTo` checks same-team first) and that asking through the enemy is therefore the only phrasing that filters.
2. **§3c — the Follow-group trap.** Confirm both terms are needed and that a just-played witch really does carry a Follow group (`TryAutoEnrollInFollowGroup` in `BeginPlay`, gated on `IsFollowCommandEligible()` ⇒ `CanFollowHero()` ⇒ `Profile == Support` is true for her).
3. **§5b — `CanEverAttack()`'s base body.** The one shipped predicate whose body I changed. Confirm the CDO reasoning (`CardID = NAME_None` at `SummonedUnit.h`) holds and that `SiegeLadderClimbTest`'s CDO row stays green.
4. **§5c — `CanTakeZoneOrders()`.** Confirm the widening does not re-open the manager's Cleric FOLLOW-ONLY ruling (the heal body it protected is not reshaped; the witch does not run it).
5. **§5d — T/E.** The one place I mapped behaviour his sentence does not spell out. If the manager wants a different mapping it is one `if` inside `UpdateStateWitch`.
6. **§5e — the 0-HP heal defect.** Please confirm the guard's placement is right (top of `UpdateSupportHealTargeting`, above the four extracted statements) rather than at the two call sites.
7. **Compile risks I cannot test** (⛔ no compile in my lane): `IsVeilCaster()` is called from `CanEverAttack()`'s inline body **before** its own declaration (legal — complete-class context, but worth a second pair of eyes); `IsWitchVeilCandidate`/`ClearWitchCastChannel`/`InterruptIncomingWitchCast` reach **private** members and functions of *another instance* of the same class (legal — access is per-class, not per-object); `Candidate` is `const ASummonedUnit*` and is passed to `IsAgentVisibleTo(ETeamId, const AActor*)` by implicit upcast; the `UpdateStateWitch` ternary uses `static_cast<AActor*>(FindOwnCastle())` to unify `ACastle*` with `FindNearestEnemyCastle()`'s `AActor*`.
8. **`ClearWitchCastChannel`'s "only if it still points at us" test.** A second witch that has since begun her own cast on the same subject owns the back-pointer; clearing it unconditionally would silently hand her subject to a third witch mid-channel. If you disagree with that reading, it is the highest-consequence line in the teardown.
9. **My edits to `Tests/SiegeInvisibilityTest.cpp`** (§7) — confirm the inversion is the one `TASK-829` asked for by name and that the census total `7 → 8` is the only absolute I moved.

## 10. FOR `TASK-835` (integration)

Nothing in this diff can work without **`TASK-831`'s row**: `CardID` **`Witch`** character-for-character (§6.8), `Profile` **`Support`**, `Damage` **0**, `Range` = **400** to match `WitchVeilRadiusFallbackUU`. The unit Blueprint is `/Game/Blueprints/Units/BP_Unit_Witch` parented to **`ASummonedUnit`** (there is no witch C++ class, deliberately — §5a). The PIE check that actually proves the feature: play a Witch, watch a friendly go invisible after ~3 s, shoot the witch mid-cast and confirm **nothing** is veiled, then confirm the veiled unit is ignored by enemy units, towers and the bot — and that it is still **attackable** by a player who knows where it is.

---

# ⭐ ADDENDUM — ITEM (8) — THE CAST'S READ-ONLY SURFACE (appended 2026-09-02, second sitting)

**Why this addendum exists:** `TASK-830` reported `ready-for-qa` with **item (8) unbuilt**. Two independent
measurements caught it — `TASK-867`'s stale-symbol sweep (`GetCastProgressPercent` / `IsCastInProgress` =
**0 tree-wide**) and `TASK-861` from the consumer side (`HealthBarProvider.h` had neither; the widget exposes
only `SetTeamColor` / `SetDamageBoost` / `OnHPChanged`). **Two tasks were blocked and both correctly refused to
proceed:** `TASK-860`'s premise gate (0a) was red, and `TASK-861` refused to author the Blueprint event rather
than produce a `K2Node_CustomEvent`. ⛔ **Nothing in §1–§10 above was redone or reopened.** This is one named
item, built on top of the shipped cast.

**Status:** `ready-for-qa` · **Gate:** `TASK-849` · **Law:** `WITCH-§9.1`/`§9.2`/`§9.3`/`§9.6`, `WITCH-§4`,
`SC-§38`, `SC-§39`, `SC-§40` cl. 1/4/9, `HIGH-§1`, `TL-§5c`
**Compile / editor / MCP / Git:** ⛔ **none touched.** The editor was live under `TASK-861`/`TASK-831` throughout.
**Fence held:** ⛔ no widget code, ⛔ no `UWidgetComponent`, ⛔ no UMG, ⛔ no `DrawSize`, ⛔ no Blueprint event ·
⛔ `CombatantHealthBar*.{h,cpp}` **untouched** (`TASK-860`'s, and it carries the `SetVisibility(` == 1 pin) ·
⛔ `SiegeCombatStatics.*` / `SiegeFogStatics.*` / `SiegeBotController.*` **untouched** · ⛔ no card row, no asset.

---

## 11. WHAT SHIPPED, IN ONE PARAGRAPH

`IHealthBarProvider` gains the **two DEFAULTED virtuals the law pins character-for-character** —
`float GetCastProgressPercent() const { return 0.f; }` and `bool IsCastInProgress() const { return false; }` —
so `ABuilding`, `AHeroCharacter` and every non-witch provider need **zero changes** and render pixel-identically
to today (`WITCH-§9.6` calls that a requirement, not an expectation). `ASummonedUnit` is the **only** class in
the tree that overrides them, and it answers for **both ends of one cast**: the witch reads her own live timer,
and her subject **pulls** the same timer through its own `IncomingWitchCaster` back-pointer. ⛔ The witch never
pushes a value onto the subject's widget (`WITCH-§9.3` forbids it by name) — the pull is what makes the
subject's bar vanish on the **same frame** the witch dies, which the interrupt rule makes the common case.
Both accessors go through **one** private resolver so a bar's *gate* and its *fill* can never disagree. Two new
tests, and **one count-pin trap found on my own path and avoided** (§13).

## 12. FILES TOUCHED (this addendum only)

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/HealthBarProvider.h` | the **two DEFAULTED virtuals**, in a new `//~ Begin CAST PROGRESS` block beside the boost pair they copy. ⛔ Nothing else in the file changed |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | 2 public `override` declarations in the existing `IHealthBarProvider` block; 1 private declaration (`ResolveCastClockOwner`) in the cast block |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | 3 function bodies + a block header comment, inserted **between** `CompleteWitchCast()` and `UpdateStateWitch()`. ⛔ **No existing line was edited** — the whole diff is an insertion |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | **+2 tests (30 → 32)** + 5 includes |

## 13. ⛔⛔ THE COUNT-PIN TRAP ON MY PATH — MEASURED, AVOIDED, AND IT IS A **FOURTH** CLASS-2 FINDING FOR `TASK-867`'s LIST

**`SiegeInvisibilityTest` test 28 (`…TheCastHasADurationAndTheVeilStillDoesNot`) pins the tunable by EQUALITY,
not by an absolute:**

```cpp
const int32 FileSeconds  = CountOccurrencesInCode(Text, TEXT("WitchCastSeconds"));      // whole SummonedUnit.cpp
const int32 BeginSeconds = CountOccurrencesInCode(BeginBody, TEXT("WitchCastSeconds"));
TestEqual(..., BeginSeconds, FileSeconds);
```

⇒ ⛔ **every read of `WitchCastSeconds` on a code line in `SummonedUnit.cpp` must be inside `BeginWitchCast`.**
**Measured before my edit: 2 / 2. Measured after: 2 / 2.**

**The obvious implementation of a progress percent is `Elapsed / WitchCastSeconds`.** It compiles, it reads
sensibly, and it turns that test **RED** — in a row named for the cast's **DURATION**, which names neither this
surface nor this task (`SC-§40` cl. 4 on top of the count pin, exactly `TASK-867`'s F-2 shape).

⭐ **And the pin is RIGHT rather than merely in the way** — its own comment refuses *"a countdown displayed and
then acted on"*, which is precisely how a CAST duration turns into a VEIL duration. ⛔ **Loosening it was never
on the table.** The denominator is the **live timer's own `GetTimerRate`**, and that is better on a second,
independent ground: `BeginWitchCast` arms with `FMath::Max(WitchCastSeconds, 0.05f)`, so for a row tuned below
the floor the tunable is the **wrong** denominator — the fill would run past 100 and the clamp would *hide* the
disagreement. **Two independent reasons, one answer.** Test 32 pins `WitchCastSeconds` at **0** inside
`GetCastProgressPercent`'s body and says why, so the coupling is discoverable from the file that must respect it.

## 14. ⭐⭐ THE FOUR DESIGN DECISIONS (⚠️ QA: rule on these)

### 14a. ⭐⭐ THE SUBJECT **PULLS** — one clock, two readers, and no bar can be stranded
`WITCH-§9.3` says the target answers *about itself* and forbids the witch pushing a value. Implemented as
`ResolveCastClockOwner()`: the witch end returns `this`; the subject end reaches the caster through its **own**
`IncomingWitchCaster` and reads **the caster's** handle. ⇒ there is **one** `FTimerHandle` in the feature and
**two readers of it**, so the two bars are the same event by construction rather than by synchronisation.
⭐ Both links are **weak**, so a witch destroyed mid-cast takes her subject's bar down on the same frame with
**no teardown having to run** — the stranded-bar failure `WITCH-§9.3` names is unreachable, not merely handled.

### 14b. ⛔ THE SUBJECT RE-CHECKS THAT THE CASTER STILL POINTS **BACK** AT IT — the highest-consequence line
`Caster->WitchCastTarget.Get() != this` is not padding. `ClearWitchCastChannel` (§9.8 above) deliberately
releases a subject's back-pointer **only when it still names the clearing witch**, so a subject re-targeted by a
**second** witch legitimately keeps a link the first no longer owns. Reading the clock off that caster would
paint a bar for a cast **no longer aimed here**. ⚠️ **QA: this is the reading I would put in front of you second,
after 14d — it is the mirror of the teardown decision §9.8 already asked you to rule on.**

### 14c. ⛔ THE TWO ACCESSORS ARE INDEPENDENT QUESTIONS SHARING **ONE** RESOLVER
⛔ `IsCastInProgress()` is deliberately **not** `GetCastProgressPercent() > 0.f`. A cast that has just begun
reads **0%**, so that phrasing hides the bar for the first poll of every cast — the exact moment `WITCH-§9.1`
requirement 1 (*"a cast is RUNNING, on THIS one"*) exists to serve. But deriving them **separately** is the
other failure: a bar whose **gate** and whose **fill** disagree. ⇒ one private resolver, two thin readers.

### 14d. ⛔⛔ THE CLASS-DEFAULT-OBJECT GUARD, AND IT IS **LOAD-BEARING**, NOT DEFENSIVE PADDING
`AActor::GetWorld()` returns `nullptr` for a CDO **by construction** (`RF_ClassDefaultObject` early-out,
verified in `Engine/Private/Actor.cpp`), and `AActor::GetWorldTimerManager()` is a bare
`GetWorld()->GetTimerManager()`. ⇒ **an unguarded accessor null-derefs on any world-less caller — and this
surface is on an INTERFACE, so world-less callers are reachable from anywhere.** `SiegeLadderClimbTest` already
records this hazard in its own words. Without the guard, test 31's non-witch rows **CRASH the runner instead of
failing it**. ⚠️ **I did NOT add the guard to `IsCastingVeil()`** — that body is pinned
(`IsTimerActive(WitchCastTimerHandle)` == 1) and is outside this addendum's fence; the guard sits **above** the
call instead. ⚖️ **Flagged: `IsCastingVeil()`'s own comment claims it is "safe pre-BeginPlay and on a torn-down
world". That is true of a spawned actor and FALSE of a CDO.** I left the body alone and worked around it; if QA
wants that comment corrected it is a one-line docs change and it is **not** mine this sitting.

## 15. ⭐ THE TWO TESTS — THE TWO THE SPEC NAMES, AND WHAT EACH ONE FAILS ON

| test | fails when |
|---|---|
| 31 `…EveryNonWitchAnswersTheCastSurfaceThroughTheDefaultedVirtual` | either virtual is made **pure** (every provider in the game breaks) · a **second** class overrides them (a second actor type grows a cast row) · `Building.h`/`HeroCharacter.h` grow an override (`WITCH-§9.6`'s pixel-identical requirement) · ⛔ **or the CDO guard is removed, in which case it CRASHES rather than fails** |
| 32 `…AnInterruptedCastNeverReachesFullAtTheCastSurface` | the fill is derived from the **tunable** instead of the live rate (§13) · the clamp or its 0..100 ceiling is dropped · the **subject end** is dropped (the tell collapses to the one-ended cheap default `WITCH-§9.2` refuses by name) · a **stored** percent/bool member appears on the unit · any cast-lifecycle function starts **writing** the tell · the teardown stops clearing the timer |

⭐⭐ **BOTH TESTS RUN REAL C++ ON CLASS DEFAULT OBJECTS, not only source text** — the first rows in this file
that do. That is possible *only* because the accessors are `const` and world-guarded, and it is worth having:
`ABuilding`, `AHeroCharacter` and the `ASummonedUnit` CDO are **called** and asserted to answer `false` / `0.f`.

⭐ **The positive control that makes those three zeros mean something** (`SC-§39`): on the **same** CDOs, the
shipped defaulted pair this surface copies **does** discriminate — `ASummonedUnit`'s override of
`GetDamageBoostChangedDelegate()` returns a real delegate while `ABuilding` takes the interface default
(`nullptr`). ⇒ virtual dispatch is live and *"defaulted"* genuinely means *"answered without an override"*.
⛔ Without it, *"everything answered false/0"* is indistinguishable from *"dispatch reached nothing"*.

## 16. ⛔⛔ A TRAP I ALMOST SHIPPED **AT `TASK-860`**, AND CAUGHT BEFORE WRITING THE HANDOFF

My first draft of test 32 banned `bIsCasting` / `bCastInProgress` / `CastProgressPercent =` **tree-wide**.
⛔ **`TASK-860`'s component will legitimately write `const bool bIsCasting = Provider->IsCastInProgress();` and
`const float CastPercent = Provider->GetCastProgressPercent();` as ORDINARY LOCALS** ⇒ that sweep would have
gone **RED because a downstream caller EXISTED**, in a test naming neither `TASK-860` nor its file — i.e. I would
have manufactured the exact defect class this task was dispatched to avoid, and aimed it at the next task.

✅ **Fixed by scoping to `SummonedUnit.h`.** ⚖️ *A **local** that reads the surface is not a second source of
truth; a **member** on the actor that owns the cast is.* A header can only declare members, so the scoped ban
catches exactly the defect and nothing else. It ships with a positive control (`FTimerHandle
WitchCastTimerHandle;` == 1 — the one piece of state the cast is allowed to keep).
⭐ **And the control for the surface's own existence is `> 0`, not an absolute, ON PURPOSE — `TASK-860` is about
to add callers and a pinned integer would go red for the crime of being consumed.**

## 17. ⚠️ DECLARED RESIDUALS

1. ⛔ **THE SURFACE IS POLL-SHAPED — there is no `FOnCastProgressChanged` delegate, DELIBERATELY.** Spec item (8)
   names exactly two getters and says *"READ-ONLY … no widget code"*, and `WITCH-§9.3`'s pinned contract carries
   **float and bool only**. ⛔ I did **not** widen it to be helpful. ⚠️ **`TASK-860` must therefore POLL**, which
   is also correct on the merits: a cast progresses *continuously*, so a push model would need a per-frame
   broadcast — strictly worse than one read on the bar's own update. **Flagged so nobody reads the missing
   delegate as an oversight.**
2. ⛔ **There is no colour channel and I did not add one.** `TASK-861` designed against float/bool and ruled the
   cast bar team-neutral amber at design time *because* the contract has no R/G/B. ⛔ Widening it would have
   invalidated a shipped art ruling.
3. ⚠️ **Nothing here is proof the bar works.** These are getters plus structural/CDO tests. The two-ended read
   at gameplay distance is `TASK-861` §9's pixel question and it is **Jonathan's eye**, not a green suite.
4. ⚠️ **`HealthBarProvider.h`'s class comment still says *"Pure-virtual C++ interface"*.** That sentence was
   already inexact before me (the boost pair defaulted it in `TASK-362`); my block adds the second exception.
   ⛔ **Left alone deliberately** — it is a shipped comment outside the two named virtuals, and rewriting it is a
   wider edit than this item authorises. Named so QA can rule it either way.
5. ⚠️ **A cast running while the witch is on a ladder.** `UpdateState` early-outs on `IsClimbing()` (§6.5 above),
   so validation is skipped — but the **tell is unaffected**, because it reads the timer rather than the poll.
   ⭐ That is a small dividend of deriving rather than storing.
6. ⚠️ **`ResolveCastClockOwner` is a fourth function in the witch-cast block.** `SiegeInvisibilityTest`'s
   one-flag-one-door row carries prose about *"the actor-side API is exactly three functions: read, grant,
   break"* — ⛔ **that row's assertions are three specific `== 1` needles on the VEIL doors, none of which this
   touches** (re-measured: all three still 1). A cast-progress reader is not a veil function and takes no time
   parameter. **Declared rather than left for QA to trip over.**

## 18. ⭐ `SC-§40` cl. 9 — COUNTS RE-MEASURED BY SYMBOL AND REPORTED **EVEN WHERE THEY MATCH**

*A silent match is indistinguishable from a skipped check.* Instrument: a faithful replica of the house
`CountOccurrencesInCode` (same skip rule: trimmed line starting `//`, `* `, `*/`, `/*`, or equal to `*`).

**⭐ THE INSTRUMENT WAS PROVEN IN BOTH FAILURE DIRECTIONS BEFORE ANY NUMBER BELOW WAS TRUSTED (`SC-§39`):**
· **over-count edge** — `SiegeBotController.cpp`: bare `IsAgentVisibleTo` = **4**, call-shaped
`FSiegeCombatStatics::IsAgentVisibleTo(` = **3**, reproducing `TASK-851`'s measured 4-vs-3 exactly; also
`ApplyRadialDamage` = **0** on code lines vs **1** raw.
· **under-count edge** — `SiegeInvisibilityStatics.h`, needle `/*`: raw **16**, skip-aware **0**, reproducing
`TASK-867`'s measured 16 eaten hits.

| count | source | before | **after my diff** | verdict |
|---|---|---|---|---|
| `GetCastProgressPercent` tree-wide | `TASK-867` §5 / `TASK-861` §2 = **0** | **0** ✅ confirmed | **3** | the absence is closed |
| `IsCastInProgress` tree-wide | same = **0** | **0** ✅ confirmed | **3** | " |
| `->GrantInvisibility()` tree-wide | pinned **1** | **1** | **1** | ✅ untouched |
| `BreakInvisibility(ESiegeVeilBreakReason::` tree-wide | pinned **8** | **8** | **8** | ✅ untouched |
| `WitchCastSeconds` in `SummonedUnit.cpp` / in `BeginWitchCast` | pinned **equal** | **2 / 2** | **2 / 2** | ✅ §13 |
| `SetTimer(WitchCastTimerHandle` | pinned **1** | **1** | **1** | ✅ |
| `IsTimerActive(WitchCastTimerHandle)` in `IsCastingVeil` | pinned **1** | **1** | **1** | ✅ |
| the **13** banned veil names tree-wide | pinned **0** each | **0** ×13 | **0** ×13 | ✅ |
| `FSiegeInvisibilityStatics::IsVisibleTo(` tree-wide | pinned **2** | **2** | **2** | ✅ |
| `SetVisibility(` in `CombatantHealthBarComponent.cpp` | `TASK-867` F-2 = **1** | **1** (raw 3) | **1** | ✅ **untouched — still `TASK-860`'s trap** |
| `return false;` in `FSiegeCombatStatics::ReadFogState` | `TASK-867` F-1 = **2** | **2** | **2** | ✅ **untouched — still `TASK-839`'s trap** |
| `FSiegeFogStatics::EffectiveVisionRadius(` tree-wide | pinned **2** | **2** | **2** | ✅ |
| `ReadFogState(` tree-wide | pinned **3** | **3** | **3** | ✅ |

⚠️⚠️ **ONE INSTRUMENT DISCREPANCY, REPORTED THOUGH NOBODY ASKED (`SC-§40` cl. 9):** `TASK-867` records the
positive control `GetDamageBoostPercent` at **10**, measured with `grep -rn`. **The house helper reads 6**
(4 of the 10 are comment lines). ⛔ **Both are correct for their instrument, and neither is wrong** — but a
future task that pins **10** *using `CountOccurrencesInCode`* would be red on arrival. ⭐ The general form:
**a count is only meaningful together with the instrument that produced it.**

## 19. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST (this addendum)

1. **§14d — the CDO guard.** Confirm `AActor::GetWorld()` really is null for a CDO and that placing the guard
   *above* `IsCastingVeil()` (rather than inside its pinned body) is the right call. This is the row where a
   mistake is a **crash**, not a failure.
2. **§14b — the back-pointer agreement check.** The mirror of §9.8. If you disagreed with §9.8's teardown
   reading, you must disagree with this line too — they are one decision seen from two ends.
3. **§13 — the `WitchCastSeconds` pin.** Please re-derive the equality assertion in test 28 yourself rather than
   taking my word (`SC-§40` cl. 3): it is the reason the denominator is `GetTimerRate` and not the tunable.
4. **§16 — the scoping of the stored-tell ban.** Confirm you agree a **local** in `TASK-860`'s component is not a
   second source of truth, and that scoping to `SummonedUnit.h` is the right line. If you think the ban should
   be tree-wide, say so **now** — after `TASK-860` lands it becomes a red suite and a wrong "fix".
5. **§17.1 — the missing delegate.** Confirm poll-shaped is what item (8) asked for, and that adding a delegate
   would have been widening the pinned contract rather than completing it.
6. **Compile risks I cannot test** (⛔ no compile in my lane):
   `Cast<IHealthBarProvider>(const ABuilding*)` returns `const IHealthBarProvider*` via
   `TCopyQualifiersFromTo_T` (verified in `Templates/Casts.h`) — the const-propagating overload is what test
   31's `const` provider pointers rely on · `ResolveCastClockOwner` reaches **private** members
   (`WitchCastTarget`, `WitchCastTimerHandle`, `IsCastingVeil`) of **another instance** of the same class (legal
   — access is per-class) · `Caster->WitchCastTarget.Get() != this` compares `ASummonedUnit*` with
   `const ASummonedUnit*` (composite pointer type, legal) · `const FTimerManager&` bound to
   `GetWorldTimerManager()`'s non-const `FTimerManager&` return · `TimerManager.h` and `Engine/World.h` were
   already included in `SummonedUnit.cpp`; the test file gained `Building.h`, `HealthBarProvider.h`,
   `HeroCharacter.h`, `SummonedUnit.h`, `UObject/UObjectGlobals.h`.

## 20. SUITE DELTA (`TL-§5c` — a **DECLARED** delta, ⛔ never a pass count I did not execute)

- **My delta this sitting: `+2` tests, both in `Tests/SiegeInvisibilityTest.cpp` (30 → 32). No new test file.**
- **Tree census, `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Siegebound/Tests/*.cpp`:**
  **408 declared across 30 files** at my start → **410 declared across 30 files** at my end.
- ⭐ **`+2` moved and all `+2` is mine** — no other file changed count during this sitting.
- ⛔ **DECLARED, ⛔ NOT EXECUTED** — no compile, no editor, no suite run in my lane.
- ⚠️ **The tree already carried ONE known red row before I started** (`SiegeAcquisitionFunnelTest` test 9, from
  `TASK-829`'s veil write-doors). ⛔ **It is not mine and I did not add a second** — every pin adjacent to my
  diff is re-measured unchanged in §18.

## 21. FOR `TASK-860` AND `TASK-861` — WHAT IS NOW UNBLOCKED, AND WHAT TO READ FIRST

- ✅ **`TASK-860`'s premise gate (0a) is now GREEN:** both virtuals exist in `HealthBarProvider.h`, both are
  **DEFAULTED** (not pure), and `ASummonedUnit` overrides both. Verify it yourself — the row tells you to.
- ⛔ **The contract is still float/bool ONLY.** ⛔ No delegate, ⛔ no colour, ⛔ no enum (§17.1/§17.2).
- ⛔ **`CastPercent` is `0..100`.** The widget divides by 100, exactly as `TASK-861` §7 already designed.
- ⛔⛔ **`SetVisibility(` is pinned at EXACTLY 1 in `CombatantHealthBarComponent.cpp` — re-measured **1** today
  (`TASK-867` F-2).** The collapse is the **widget's** job. ⛔ Do not add a second call and ⛔ do not loosen it.
- ⭐ **Poll both accessors together on the bar's own update.** They share one resolver, so they cannot disagree —
  but reading one this frame and the other next frame would reintroduce the disagreement by hand.
- ⭐ **Both the witch AND her subject answer `true` for the same cast. That is the feature** (`WITCH-§9.2`), so
  seeing two bars appear at once is correct behaviour, not a duplicate.
