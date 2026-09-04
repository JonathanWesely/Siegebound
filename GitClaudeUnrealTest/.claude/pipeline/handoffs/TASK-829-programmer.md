# TASK-829 — THE VEIL STATE + THE BREAK CALL — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-849` · **Law:** `WITCH-§1`/`§2`/`§3`/`§3a`/`§6`/`§7`/`§8`, `SC-§38`
**Blockers consumed:** `TASK-827` (the pure statics, gate `TASK-847` PASS) · `TASK-828` (the funnel, gate `TASK-848` PASS)
**Compile / editor / MCP / Git:** ⛔ none touched — build-master's lane. The editor was live under an art task for this whole task.

---

## 1. WHAT SHIPPED, IN ONE PARAGRAPH

`ASummonedUnit` now carries `bool bIsInvisible` (private, unreflected) with exactly three public seams — `IsInvisible()`, `GrantInvisibility()`, `BreakInvisibility(ESiegeVeilBreakReason)` — and the last two do nothing but hand the flag to `TASK-827`'s statics. Seven break calls are wired at the **measured** sites, three of which are the `WITCH-§3a` traps. Suppression is consulted in **exactly one place** — inside `FSiegeCombatStatics::GatherHostileAgents` — so all eight acquisition sites honour the veil without one line of their own, and the AoE lane opts out **explicitly** through a new named policy. Eleven new automation tests assert the **wiring** (not `TASK-827`'s arithmetic), and three of them are built specifically to fail the obvious-wrong implementation.

---

## 2. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | `bIsInvisible` (private, **not** a `UPROPERTY`) + `IsInvisible()` / `GrantInvisibility()` / `BreakInvisibility(...)` in the existing `public:` block; `#include "Siegebound/SiegeInvisibilityStatics.h"` |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | the two door bodies; **5** break calls (melee, ranged, Sapper blast, heal, death); include |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` | new `enum class ESiegeVeilPolicy`; `GatherHostileAgents` gains a **defaulted** 4th param; new `IsAgentVisibleTo(ETeamId, const AActor*)` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` | `IsAgentVisibleTo` body (the ONE predicate call); the veil filter inside `GatherHostileAgents`; `ApplyRadialDamage` passes `IncludeVeiled`; 2 includes |
| `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` | `Mine` break in the `TryRegisterArrivedMiner(this)` arm; include |
| `Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp` | `Empower` break in the `IsAncientGroundEmpowerer()` arm + a **named refusal comment** on the recipients loop; include |
| `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp` | ⛔ **comment only** — the dormant-site note |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | ⛔ **comment only** — two dormant-site notes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeInvisibilityTest.cpp` | **+11 tests** (8 → 19) + a source-probe fixture |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAcquisitionFunnelTest.cpp` | **2 needles updated + 1 row inverted** — see §7, this is the one file outside my names list and it asked for it by name |

⛔ **Not touched, deliberately:** `SiegeBotController.{h,cpp}` (`TASK-851`) · `SiegeFogStatics.*` (`TASK-838`) · `SiegeInvisibilityStatics.{h,cpp}` (`TASK-827`'s, consumed unchanged) · `TeamId.h` · `SiegeGhostPawn.*` · `Projectile.cpp` · any witch actor / card row.

---

## 3. ⭐ THE THREE TRAPS — LOCATED BY SYMBOL (`SC-§38`), AND ALL THREE CITATIONS HAD DRIFTED AGAIN

I did not use a single line number from any document. Measured at source, current as of this task:

| trap | ✅ where the break actually landed — **the symbol is the key** | doc's number | at my read | **now, post-edit** |
|---|---|---|---|---|
| **Sapper** | the `FSiegeCombatStatics::ApplyRadialDamage(...)` call inside **`ASummonedUnit::ApplyDetonation`** | `:4212` | `:4212` ✅ | **`:4312`** |
| **Empower** | the `if (Unit->IsAncientGroundEmpowerer())` arm in **`AAncientGround::ApplyBoostTick`** | `:233` | `:233` ✅ | **`:234`** |
| **Mine** | the `if (Node->TryRegisterArrivedMiner(this))` arm in **`AMinerUnit::UpdateMining`** | `:514` | `:514` ✅ | **`:515`** |

📌 **Stated precisely, because an inflated claim is worse than none: all three citations were still ACCURATE when I read them** — `TASK-847`'s re-measurement held, and `SC-§38` did not have to save me from a stale number this time. ⭐ **What it did do is make the method right anyway: I located every one by opening the named function and finding the quoted expression, so a drift would have been a non-event.** ⚠️ **And the numbers above are already stale for the next reader — my own edits moved all three** (`+100` / `+1` / `+1`). ⛔ `TASK-849` must verify by **symbol**; a gate that checks these numbers will report my correct wiring as a defect.

**The three "obvious answers" I did not take, each with its consequence:**
1. A guard only at `PerformAttack` — `ApplyDetonation` is reached from `Detonate()` (contact) and `HandleDeath()` (shot down en route), **never** through `PerformAttack`. I put the break inside `ApplyDetonation` itself rather than at either entry, so a **third** entry added later inherits it.
2. A break at `Unit->AddPermanentDamageStacks(Grant)` — that loop is the **recipients**. They are being acted upon and stay veiled. The sorcerer executes no code; it is only *counted*.
3. A break where gold appears (`ASiegePlayerState::HandleGoldTick` / `AGoldNode::HandleDrainTick`) — neither runs on a miner's stack; it would un-veil every miner the player owns, walkers included.

---

## 4. THE DESIGN DECISIONS THAT ARE MINE, AND WHY (⚠️ QA: these are the rulings to scrutinise)

### 4a. `ESiegeVeilPolicy` — a new type, not in `WITCH-§6`'s naming table
`ApplyRadialDamage` gets its candidate set from `GatherHostileAgents`, so putting the consult in the gatherer would have made **every blast miss veiled units** — the exact opposite of `J-W2`. The blast needs a different answer from the same function.

**Why a defaulted enum parameter and not a second named function:** a second public gather (`GatherBlastTargets`, or anything containing the substring `GatherHostileAgents`) breaks `TASK-828`'s header gate, and — decisively — **a non-defaulted parameter would have forced me to edit `SpellLibrary.cpp`, `SpellLineSweep.cpp` and `SiegeCheatManager.cpp`, none of which are in my names list.** The default is also the **safe** direction: a tenth acquisition site written by someone who has never read `WITCH-§` honours the veil *by omission*. Only the one lane that must see through it says so, out loud, at its call site — and the suite asserts that lane is **exactly one** tree-wide.

### 4b. `IsAgentVisibleTo(ETeamId ViewerTeam, const AActor* Candidate)` — and it closes `qa/TASK-847.md` WARN-2 structurally
QA measured that `IsVisibleTo(Viewer, Target, bInvisible)` is **exactly symmetric** under exchange of its two `ETeamId` arguments, so a swapped call is undetectable by any test writable today. The spec asked for named-argument comments as the cheap guard. **I shipped those *and* something stronger:** this wrapper takes **one team and one actor**, so for every *caller* the swap is **untypeable** — different types. The symmetric form now exists at exactly **one** call site in the project, inside this function, carrying `/*ViewerTeam=*/` `/*TargetTeam=*/` `/*bTargetIsInvisible=*/`.
⭐ It is also the **clean seam `TASK-851` was promised**: the bot's three `TActorIterator` scans cannot route through a gather, so the only way they can honour the *same* rule is to call the *same* function. `851` should call `FSiegeCombatStatics::IsAgentVisibleTo(BotTeam, Unit)` at sites 1 and 3, and leave site 2 alone with a `J-W10` comment.

### 4c. ⚠️ `GrantInvisibility()` ships with **zero callers** — flagged, not hidden
`TASK-830` owns the cast. But without a write-true door `bIsInvisible` could never become true, which would make the suppression branch inside `GatherHostileAgents` **unreachable code** — a reviewer would (correctly) flag that as the defect instead. Same precedent as `ESiegeVeilBreakReason::Cast`, which `TASK-827` shipped with no site by exactly this argument and passed with 0 blockers. ⚖️ **If QA rules otherwise, deleting it is a 3-line revert** (header decl, body, one test row) — but `TASK-830` re-adds it immediately.

### 4d. `Cast<ASummonedUnit>` inside `SiegeCombatStatics.cpp` rather than a method on `ITeamAgent`
`WITCH-§6` forbids a mirrored veil bool on any other class and `J-W10` rules the hero not veilable, so **`ASummonedUnit` is the complete set of veilable actors** — `true` for a non-unit is the whole truth, not a permissive fallback. An `ITeamAgent` method would have meant editing `TeamId.h`, which is **outside my names list**. The cost is one extra `Cast` per hostile candidate on acquisition polls; the code says so, and says that a *second* veilable class belongs on the interface rather than as a second `Cast` here.

### 4e. Two break calls in `PerformAttack`, not one before the branch
One call placed above `if (bRangedAttack)` would be exactly equivalent at runtime. I shipped two, each adjacent to its act, because `WITCH-§3a`'s measured ledger names the two delivery modes as **two sites** and a reviewer verifying **by symbol** must find a break beside each. They are mutually exclusive at runtime.

### 4f. Break placement: **before** the act, in every case
`ApplyDamage` → the receiver's `TakeDamage` can re-acquire inside the same call stack, and it must see a unit that has already revealed itself. Every break also sits **after** every early-out, so a Cleric whose target topped off, or an attacker whose target drifted out of range, returns **without** un-veiling. The break is the *act*, never the intention.

### 4g. `Death` is placed **after** the `bSuicide ApplyDetonation`
A veiled Sapper therefore logs `Attack` (its blast is what revealed it) and the later `Death` call is an idempotent no-op. Truthful attribution; `ApplyBreak` is monotone so the flag outcome is identical either way.

### 4h. Towers and the hero get **comments only**
`WITCH-§3` lists `ATower::FireProjectileAt` / `FireChainZapAt` / `AHeroCharacter::DoMeleeAttack` / `HandleDeath` under `Attack`/`Death` *for completeness*; `WITCH-§7` + `J-W10` rule both classes **not veilable**, and `WITCH-§6` forbids the mirrored flag that would be required. A break there would be **dead code contradicting a live ruling**. Both files carry a named note (the `WITCH-§8` site-2 precedent), and both classes' **acquisition** is suppressed for free through the funnel.

---

## 5. ⚠️⚠️ DECLARED RESIDUALS — say these out loud rather than let QA find them

1. **A lone veiled sorcerer un-veils itself with nobody to boost.** `ApplyBoostTick` counts empowerers before it knows whether `Occupants` is non-empty, so a veiled Sorcerer standing on an Ancient Ground breaks its veil on the first tick even if no unit is there to receive stacks. ⭐ I believe this is the correct reading (channelling the ground *is* the act) and it is the only one available: `WITCH-§3a` names **that arm** as the site, and the alternative — break only when a grant lands — is the recipients trap wearing a disguise. ⛔ Reversible on one word, and it is a `WITCH-§3` amendment, not an edit here.
2. **A veiled unit still captures a zone** (`J-W9`, already accepted). A zone can flip with no visible cause. Asserted in the suite as a *decision*, with the reason.
3. **The bot still sees veiled units** until `TASK-851` lands. Between this task and that one, the card does very little against the only shipped opponent. `WITCH-§8` rules this in my favour but boards it separately — I stayed inside the fence.
4. **The material/render half is absent.** `WITCH-§5`'s `MI_Unit_Invisible` swap belongs on `BreakInvisibility`'s true edge; the code says so and does **not** ship a mesh or material call. A veiled unit is currently invisible to enemy acquisition and looks **completely normal** to its owner.
5. **No test drives the veil over a populated world.** The house rule forbids `SpawnActor`/`CreateWorld` in `Siegebound/Tests/`, so the wiring is pinned structurally. ⛔ **Green here is not "invisibility works"** — that needs a PIE pass with a witch on the field, which cannot exist before `TASK-830`.

---

## 6. ⭐ A MEASURED FINDING WORTH A CONVENTIONS LINE (QA / manager: this is a real one)

**The house `CountOccurrencesInCode` helper skips any line whose trimmed form starts with `/*` — so a named-argument comment written in the natural one-argument-per-line layout is INVISIBLE to the very gate that enforces it.**

I hit this while writing the guard `TASK-829(3b)` asked for. The idiomatic layout

```
    return FSiegeInvisibilityStatics::IsVisibleTo(
        /*ViewerTeam=*/ ViewerTeam,
        ...
```

made all three assertions read **zero**, and the obvious "fix" is to change the expectation to zero — which deletes the guard while leaving a green test that claims to enforce it. The shipped call is therefore written on **one line**, and `SiegeCombatStatics.cpp` says why beside it so nobody re-wraps it.

⚖️ Same family as `qa/TASK-848.md` N-2 and `SC-§38`: **an instrument's blind spot is indistinguishable from a clean result.** Suggested law: *a structural guard expressed as a comment must live on a line that also carries code, or the scanner cannot see it.*

---

## 7. ⚠️ THE ONE FILE I TOUCHED OUTSIDE MY NAMES LIST, AND WHY IT IS NOT SCOPE CREEP

`Tests/SiegeAcquisitionFunnelTest.cpp`, three edits:
- **`InClassHostileNeedle`** `GatherHostileAgents(World, Team, HostileAgents)` → `…HostileAgents, ` — the in-class call now names its policy. Still cannot match the definition (`(const UWorld* World,`), which was the needle's whole purpose.
- **Test 7's gather row** — same needle, now the full expression including `ESiegeVeilPolicy::IncludeVeiled`.
- **Test 7's `bIsInvisible == 0` row — INVERTED, not deleted, exactly as `TASK-828` instructed in that row's own comment** (*"When 829 lands, this row is the one it revisits BY NAME — do not delete it, invert it"*). It now asserts `IncludeVeiled` appears once and `SuppressVeiled` zero — **and the original `bIsInvisible == 0` row survives**, now saying something still worth saying: the exemption is a named policy, never an inline flag read.

⛔ **`GatherTeamAgentsFiltered`'s signature is byte-identical** — deliberately, so `TASK-828`'s reverse-control probe (which pins that signature character-for-character, and its `Radius`/`Distance`/`IsA<`/`IsDead`/`Sort(`/`Out.Add`/`Out.Reset` ban list) stays green untouched. That is why the veil filter lives in `GatherHostileAgents` and not in the private helper.

📌 **`TASK-848`'s valve note is now closed:** its *"zero behaviour change"* window was only checkable **before** this task. It landed first, gate PASS, 0 blockers. The window is now shut and that is the expected order, not a loss.

---

## 8. SUITE DELTA — MEASURED, WITH A DISCREPANCY I AM NOT CLAIMING

- **My delta: `+11` tests, all inside `SiegeInvisibilityTest.cpp` (8 → 19). No new test file; the file count stays 29.**
- ⚠️ **The tree total moved by more than my delta and I did not cause the rest.** I measured **376 / 29 files** at task start; it now reads **389 / 29**. `Tests/SiegeControlsHelpTest.cpp` and `Tests/SiegeLadderClimbTest.cpp` were rewritten by another lane **during** my task (mtimes `21:54:22` / `21:54:32`, both `M` in git, `+3` `IMPLEMENT_` lines between them). ⛔ **Trust my `+11`, verify the rest against those two files, not against me** (`TL-§5b` — the delta is the only trustworthy figure).

**The eleven, and what each one fails on:**

| test | fails when |
|---|---|
| `TheVeilHasOneSourceOfTruthAndOneWriteDoor` | a mirrored `bIsInvisible` appears on any other class, or any file inlines `bIsInvisible = …` |
| `EveryShippedActInTheClosedSetIsWired` | any of the 7 break calls is dropped, or an 8th appears without a ruling |
| `TheSapperBreaksInsideApplyDetonationNotOnlyInPerformAttack` | ⭐ **the break is only at `PerformAttack`** — the trap |
| `TheEmpowerBreakLandsOnTheSorcererNotTheRecipients` | ⭐ **the break is at `AddPermanentDamageStacks`** — the trap (ordering claim, on code lines only) |
| `TheMineBreakLandsOnArrivalNotOnTheGoldPayout` | ⭐ **the break moves to the payout, or above the claim** — the trap |
| `TheHealerBreaksAndThePatientStaysVeiled` | the direction is inverted |
| `WalkingClimbingAndBeingActedUponDoNotBreakTheVeil` | a "for safety" break is added at acquire / take-damage / climb / order / capture / tower / hero |
| `AVeiledEnemyIsDroppedByTheOneAcquisitionFunnel` | the consult is missing, duplicated, sorted, or the named-argument guard is lost |
| `BlastLaneIsExemptFromTheVeil` | the `IncludeVeiled` argument is deleted — **which breaks no other test in the tree** |
| `TheFriendlyLaneCannotBeVeilSuppressed` | the friendly gather grows a policy or a consult |
| `PermanentlyIsEnforcedByShapeNotByComment` | a restore/refresh/tick/duration/timer name or a "was visible" cache appears anywhere |

Every trap test asserts a **pair** — present at the right site **and** absent at the wrong one. Only the second half fails a naive wiring (`TASK-829(6a)`). Every whole-tree scan carries a **positive control** in the same pass, so a broken scanner cannot report "safe".

---

## 9. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **`Out.RemoveAll(...)` in `GatherHostileAgents` must be order-preserving** — every acquisition site tie-breaks by *strict* improvement, so a reordering filter would silently re-pick targets at eight sites **without changing a single result set**. `TArray::RemoveAll` preserves relative order; if you disagree with that reading, it is the highest-consequence line in the diff.
2. **The `IncludeVeiled` default direction.** Confirm a forgotten argument yields *suppression*, not exposure. That is the whole safety argument for the default.
3. **`GrantInvisibility()`'s zero callers** (§4c) — rule it, please, rather than leaving it for `TASK-830` to inherit as an open question.
4. **The lone-sorcerer residual** (§5.1) — I believe it is correct and it is the only reading `WITCH-§3a` permits, but it is a real behaviour a player could notice.
5. **My edits to `SiegeAcquisitionFunnelTest.cpp`** (§7) — confirm the inversion is the one `TASK-828` asked for and that `GatherTeamAgentsFiltered`'s signature is untouched.
6. **Compile risks I cannot test** (⛔ no compile in my lane): `SiegeCombatStatics.cpp` now includes `SummonedUnit.h` (no header cycle — `SummonedUnit.h` does not include `SiegeCombatStatics.h`); the `Cast<ASummonedUnit>` on a `const AActor*` relies on UE's const `Cast` overload; the `RemoveAll` lambda takes `const AActor*` against a `TArray<AActor*>`.
7. **`ESiegeVeilPolicy` is a name `CONVENTIONS` does not contain.** If the manager wants it in `WITCH-§6`'s table, that is a docs edit — I did not write to `CONVENTIONS.md`.

## 10. FOR `TASK-851` (blocked on me — here is the seam)

Call **`FSiegeCombatStatics::IsAgentVisibleTo(BotTeam, Candidate)`** inside `FindNearestEnemyIntruderOnBotHalf`'s **unit** arm and inside `FindFireballClusterTarget`. ⛔ Do **not** read `IsInvisible()` inline and do **not** re-express the rule — the suite asserts the symmetric predicate has exactly one caller tree-wide, and a second one turns `AVeiledEnemyIsDroppedByTheOneAcquisitionFunnel` red on purpose. Leave the **hero arm** alone with a `J-W10` comment. The blast still lands: `ApplyRadialDamage` is exempt, so a Fireball the bot aimed at something else still catches a veiled cluster.
