# TASK-778 — THE HERO CLIMBS: the driver, the TEN exits, the watchdog it did not have — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Status:** `ready-for-qa`
**Suite delta: +23** (a new file, `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroLadderClimbTest.cpp`; 23 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, verified by count).
**Ruled option built: `A` (Jonathan's `K-1`).** `TOWER-§8.5a`'s deck-breach window ships **exactly as written, all eight clauses, unamended** — because option `A` repairs the deficit in the *geometry* (TASK-783's translation) and changes nothing in this task's mechanism. ⛔ No ignore-and-sweep (`B`), ⛔ no capsule change (`C`), ⛔ no short climb (`D`), ⛔ no sixth option.

---

## ⛔⛔ READ THIS FIRST — THE FEATURE IS COMPLETE AND **CANNOT FIRE**, FOR A REASON OUTSIDE MY FENCE

`AClimbableTower::TryBeginContactClimb` refuses the hero on **identity** and always will until someone widens it:

- `ClimbableTower.cpp:267` — `EvaluateLadderEntry` returns `NotAnAdmittedClimber` when `bClimberIsSummonedUnit` is false.
- `ClimbableTower.cpp:708` — the caller passes `Cast<ASummonedUnit>(Climber) != nullptr`, which is **false for `AHeroCharacter`**.
- `ClimbableTower.cpp:726` — the start is `CastChecked<ASummonedUnit>(Climber)` → `Unit->BeginLadderClimb(...)`.

⇒ **My poll runs every frame, satisfies all three contact terms, and receives `NotAnAdmittedClimber`.** It logs once (latched) and does nothing else.

**⛔ I did NOT work around it, and each alternative is refused for a stated reason:**

| Workaround | Why refused |
|---|---|
| Hero calls its own `BeginLadderClimb` on that verdict | ⛔ Skips `CanTeamAscend` — a **silent back door around Jonathan's `T-3`** (`CONTACT-§4.3` names exactly this) · ⛔ skips the occupancy slot (`TOWER-§10 L-1`: two capsules on one line, and depenetration shoves one **off it, in mid-air**) · ⛔⛔ leaves `ActiveClimber` unset, so `AClimbableTower::EndPlay` cannot reach the hero ⇒ **exit `H-10` would not exist and the player hangs forever when the tower dies.** |
| Widen the identity term myself | ⛔ Fenced: item (11) grants me *"item (0c)'s ONE narrow, named rename in `ClimbableTower.{h,cpp}` and ⛔ NOTHING ELSE in that file."* ⛔ And it is **law-pinned**, not merely fenced — see below. |

**⚠️ WHY THE WIDENING IS A REAL DESIGN CHANGE AND NOT A ONE-LINER — three seams, two of them law-pinned:**

1. **The identity term** — `EvaluateLadderEntry`'s `bClimberIsSummonedUnit` argument would become "is an admitted climber". *(Cheap.)*
2. **The START seam** — the tower can only start a climb through `ASummonedUnit::BeginLadderClimb`. `ILadderClimber` deliberately carries **no `Begin`** (`CONTACT-§4.4`: *"STARTING a climb is the pawn's business"*), and `CONTACT-§8` **pins that surface at two methods**. ⇒ widening needs a **`CONVENTIONS.md` amendment**, which is manager-only and explicitly outside my fence.
3. **The COMPLETION seam** — the tower learns a climb ended only via `ASummonedUnit::OnLadderClimbEnded` (`ClimbableTower.cpp:743`). A hero has no such delegate (`CONTACT-§8` gives this class exactly two new members and a delegate is not one of them). ⇒ **without it, a hero that claimed the slot never releases it and the ladder is BRICKED for the rest of the match** — which is the precise hazard TASK-777 refused to ship when it declined to pre-widen.

**📌 MY RECOMMENDATION FOR THE BOARD:** one task owning **(2) + (3) together**, landing as a `TOWER-§8.4(B)` / `CONTACT-§8` amendment (either add `BeginLadderClimb` + a completion delegate accessor to `ILadderClimber`, or give the tower a `RegisterClimber`/`NotifyClimbEnded` pair the pawn calls). ⛔ Splitting them ships the bricked ladder. **Everything on the hero's side is already built against it: the moment the tower can start an `ILadderClimber`, this feature works with zero further edits here.**

---

## 0. ⚠️ A SECOND REACHABILITY FINDING — `K-6`'s 300 uu FIXES THREE OF THE HERO'S FOUR SPEEDS, NOT FOUR

`LadderContactRadiusUU = 300.f` has landed (TASK-784). ⭐ **For the hero the naive model is EXACT rather than probabilistic**, and that is a real difference from TASK-784's unit analysis: the unit polls on a 0.25 s timer, so its dwell is credited in 0.25 s lumps and admission is a *sampling* question. **My poll runs in `Tick`, so dwell accrues continuously with `DeltaSeconds`** ⇒ the requirement is simply `window ≥ dwell`, with no luck in it.

Dead-on approach, `window = R / v`, dwell `0.35 s`, `R = 300`:

| Hero speed | Source | window | vs 0.35 s dwell | max lateral offset that still admits |
|---|---|---|---|---|
| walk **500** | `HeroCharacter.h:999` | **0.600 s** | ✅ | ±145 uu |
| walk + Swift Boots **625** | `× (1 + MoveSpeedBonus 0.25)` | **0.480 s** | ✅ | ±105 uu |
| sprint **750** | `HeroCharacter.h:1003` | **0.400 s** | ✅ (0.05 s slack) | ±56 uu |
| ⛔ **sprint + Swift Boots 937.5** | `GetEffectiveSprintSpeed()` | ⛔ **0.320 s** | ⛔ **FAILS** | ⛔ **none, at any offset** |

⚠️ **At 150 uu (pre-`K-6`) every hero speed failed** — 0.300 s walking, 0.200 s sprinting — exactly as the correction said.
⭐ **The smallest radius that admits the hero's full shipped speed range is `937.5 × 0.35 = 328.1 uu`; 350 would give a 6.6 % margin.** ⛔ **I did not touch the radius** (`CONTACT-§8`: it lives on the tower; `K-5` makes the feel Jonathan's). Test 23 measures all four rows and emits a permanent `AddWarning` naming any unreachable one, so this stays visible on every suite run instead of living only in this file.
⚠️ *Behavioural note, so nobody reads the failing row as a hang:* a Swift-Boots sprint simply runs **through** the ladder (it has zero collision) and stops at the tower body ~300 uu on; walking back works. It reads as *"sometimes it doesn't grab"* — the failure class TASK-784 warns is worse than a clean never.

---

## 1. THE TEN EXITS — every one, where it lives, how the mode is restored, and its test

**All ten route through ONE teardown, `AHeroCharacter::EndLadderClimb` (`HeroCharacter.cpp:1716`), and the restore lives there and nowhere else:**
`FSiegeLadderClimbStatics::End(LadderClimb)` (the exactly-once latch, consumed **first**) → `ClearTimer(LadderClimbWatchdogTimerHandle)` (`:1728`) → `StopMovementImmediately()` → `MaxFlySpeed = LadderClimbSavedMaxFlySpeed` (exact restore) → **`SetDefaultMovementMode()` (`:1742`)** → whole-state reset of the four transient members.
⭐ **Measured and asserted: `SetDefaultMovementMode()` appears EXACTLY ONCE in the file, `SetMovementMode(MOVE_Flying)` EXACTLY ONCE, `FSiegeLadderClimbStatics::End(` EXACTLY ONCE** (test 13). ⇒ "every exit restores the mode exactly once" is a property of **one branch**, not a promise repeated ten times.

| # | Exit | Call site | How the mode is restored | Test |
|---|---|---|---|---|
| **H-1** | Arrival | `TickLadderClimb` `:1821` (`Arrival`) | teardown; **arrival snap first**, guarded by `bReachedTop` (`§8.5a` cl. 5) | 3 — `ExitH1_ArrivalEndsTheClimbAndOnlyArrivalMaySnap` |
| **H-2** | `AbortLadderClimb()` | `:1705` → `:1713` (`Abort`) | teardown | 4 — `ExitH2_AbortLadderClimbReachesTheTeardown` |
| **H-3** | ⭐ Player releases / steers away | `TickLadderClimb` `:1795` (`InputReleased`) | teardown | 5 — `ExitH3_ReleasingTheInputEndsTheClimb` |
| **H-4** | Death | `HandleDeath` `:827`, beside `EndRecall(...)` `:814` | teardown, then `DisableMovement()` `:842` overwrites it ⇒ corpse rests at `MOVE_None` | 6 — `ExitH4_DeathAbortsTheClimbBeforeMovementIsDisabled` |
| **H-5** | ⭐⭐ `UnPossessed()` — the ghost hand-off | `:2171`, before `Super` | teardown | 7 — `ExitH5_UnPossessedIsAnIndependentBelt` |
| **H-6** | Respawn (`ResetHero`) | `:873`, before `SetMovementMode(MOVE_Walking)` `:896` | teardown, then the respawn's own deliberate `MOVE_Walking` | 8 — `ExitH6_RespawnAbortsBeforeItWritesWalking` |
| **H-7** | Recall teleport | `EndRecall` `:1438`, **before** `OnHeroRecallArrived.Broadcast` | teardown ⇒ the body arrives home walking | 9 — `ExitH7_TheRecallTeleportEndsTheClimbFirst` |
| **H-8** | Match end | `TickLadderClimb` `:1785` (`MatchEnd`), checked **first** | teardown | 10 — `ExitH8_MatchEndIsTheHerosFrozenTermAndItIsMapped` |
| **H-9** | `EndPlay` | `:1534`, before `Super::EndPlay` | teardown (timer manager + movement component still valid) | 11 — `ExitH9_EndPlayAbortsBeforeSuper` |
| **H-10** | ⭐ Tower dies mid-climb | `AClimbableTower::EndPlay` → `Cast<ILadderClimber>` → `AbortLadderClimb()` | teardown, via the interface this class now implements | 12 — `ExitH10_TheTowerDyingReachesTheHeroThroughTheInterface` (asserts **both** halves, across both files) |
| *(+)* | Watchdog | `OnLadderClimbWatchdog` `:1975` | teardown + the one `Warning` this feature logs; ⛔ **never** the arrival snap | 21 — `TheWatchdogIsArmedOnlyForTheClimbAndClearedByEveryExit` |

**⭐ H-5 is an INDEPENDENT belt, not a duplicate of H-4** (the `SummonedUnit.cpp:608-615` idiom, second application): a possession change for *any* reason — debug possess, seamless travel, a game-mode restart — ends a climb, and enumerating the reasons is how you miss one. Idempotent with H-4 by the latch.

---

## 2. THE WATCHDOG — armed for ~3.5 s, and the hazard class that cannot reach it

- **Armed** in `BeginLadderClimb` (`:1695`), 0.25 s looping, **only** for a climb. **Cleared** by the one teardown (`:1728`) and self-clearing as a belt (`:1954`).
- **Deadline** = `World->GetTimeSeconds() + LadderClimb.TimeoutSeconds` — the climb's **own** budget on the **world** clock, so a Tick that stops cannot postpone it. ⛔ Not a second budget and not a second number.
- **Two things it catches:** an expired budget (`Watchdog`, drops the hero **where it is** — `§8.5a` clause 5, never the deck it failed to reach) and a **dead hero still holding a climb** (reported as `Death`, the H-4 belt).
- ✅ **TASK-760's hazard class cannot exist here, and I am saying so instead of porting a fix I do not need:** that hazard is *"the watchdog rides the very driver it watches"* — an actor tick-flag write killing the poll. **An `FTimerManager` entry lives on the world and cannot be killed by an actor tick-flag write.** ⛔ I did not harmonise this onto the unit's tick-flag shape.
- ⚠️ **Why the hero needed its own at all (measured):** the only two shipped `SetTimer` calls are `RallyCooldownTimerHandle` (`:678`, one-shot) and `WarBannerAuraTimerHandle` (`:1054`, armed only while the upgrade is owned and paused by death). ⇒ a driver copied from the unit's would have shipped with **no watchdog at all and looked identical in review**.

---

## 3. THE FOUR RE-DERIVATIONS — as numbers, from **this** capsule

Hero capsule read at the CDO in test 14: **r 42.0 / hh 96.0** (`GitClaudeUnrealTestCharacter.cpp:18`; `BP_HeroCharacter` overrides nothing).

| # | Quantity | Hero | Unit (for contrast — ⛔ never inherited) |
|---|---|---|---|
| 1 | **Endpoint lift** = `GetScaledCapsuleHalfHeight()` | **96.0 uu** | 88 |
| 2 | **Deck-breach ceiling** = `3 × hh` in Z → line by the line's own slope | **288 uu of Z ⇒ 296.863 uu of line = 24.00 %** of 1236.93 | 264 ⇒ 272.126 ⇒ 22.00 % |
| 3 | **Watchdog budget** = `4 × len / rate` | **14.136 s** against a **3.534 s** ascent | same line, same rate |
| 4 | **≥56 uu standoff** | `93.6242 − 42 =` **51.624** ⇒ ⛔ short by **4.376** *before* TASK-783 | 59.624 ✅ |

- ⛔ **Not one of these is a literal in the shipped code.** The half-height is read from the capsule (`:1660`-region); everything else is derived inside the statics. Test 17 scans the shipped climb region and fails on `88.f`, `96.f`, `150.f`, `0.35f`, `0.5f`, `350.f` appearing in it.
- ⭐ **Test 14(c) is the one that keeps it honest:** it drives a *different* half-height through the same code and requires the answer to move. A future literal turns the suite red and names the reason.
- ⭐ **Re-derivation 4 is discharged by TASK-783, not by me.** Its handoff reports **δ = 10.0 uu** (not the ~4.6 first estimated). Reconstructing against `§8.5a`'s own worst-case point: `√((90.5788+10)² + 23.6849²) − 42 = 61.33 uu` on the `§8.3` gate line and **63.45 uu** on the clause-6-lifted driven line ⇒ **both ≥ 56, with margin** — where the original 4.6 would have cleared the gate line by only **0.08 uu**. ⚠️ **The licence is TASK-779's check against TASK-783's measurement, not my assertion.**
- ⛔⛔ **Nothing in my code hardcodes a socket, an endpoint, or the size of that translation** — both world points arrive as parameters read at runtime from the link. The comment that would have quoted "~4.6" was deliberately rewritten to quote **no figure at all** (`CONTACT-§10.1`, applied to a comment).

---

## 4. `CanBegin`'s FROZEN TERMS — mapped at file:line, or refuted with evidence

Call site: `HeroCharacter.cpp:1660`-region.

| Argument | Hero value | Evidence |
|---|---|---|
| `bDead` | `bDead` | the class's own death latch |
| `bAIFrozen` | ⭐ **`IsMatchOver()`** (`HeroCharacter.cpp:1459`) | **A real mapping, not a stand-in:** `ASiegeGameMode::FreezeWorldAtMatchEnd` (`SiegeGameMode.cpp:616`) is what sets `bAIFrozen` on units (`:632`); the hero's shipped equivalent of that same event is this predicate — **the one the recall channel's own match-end exit already reads**. Also enforced continuously as exit H-8. |
| `bSpellFrozen` | ⛔ **`false`, with evidence that NOTHING corresponds** | Frost is the only freeze in the game and applies `ApplyFreeze` to `ASummonedUnit` and `ABuilding` **only**: `SpellLibrary.cpp:304-320` ends with the shipped comment *"every other ITeamAgent (castle, hero) is excluded by ruling 5 — no branch on purpose"*, and `SpellLineSweep.cpp:240-249` has the same two branches. Grepped: `HeroCharacter.{h,cpp}` contain **zero** occurrences of Freeze/Frozen/Stun. ⚠️ **Declared gap:** the day a spell can freeze the hero, this argument acquires that state. |
| *(5th, hero-only)* | ⭐ **`IsRecalling()` refuses before `Begin`** | `CONTACT-§3.4` bullet 1, owed by me and provided by nothing else. The doc comment says the contract has **five** refusal reasons, not the pinned four, so no caller learns the wrong rule. |

---

## 5. THE CALL SITE — and the proof I duplicated no tunable

- **`AHeroCharacter::PollLadderContact` (`:2053`), reached from `Tick` (`:209`)**, asks `Tower->TryBeginContactClimb(this, DeltaSeconds)` (`:2114`). One entry point; ⛔ no re-implemented predicate, dwell, latch or Z resolution; ⛔ never a second path around `CanTeamAscend`.
- **Driver before poll** in `Tick` (`:208` then `:209`), so a climb ending this frame cannot be re-entered on the same frame.
- **Tower discovery:** `RefreshNearbyClimbableTowers` (`:2025`) rebuilds a weak list with `TActorIterator<AClimbableTower>` at a 1 s cadence; the per-frame loop only walks that list and prunes dead handles. ⛔ **No distance pre-filter — deliberately**: filtering needs a radius, the radius is the tower's, and the tower's own cheap 2D term drops a passer-by for one squared-distance compare.
- **Proof of no duplication (test 17, reflection + source):** `AHeroCharacter` declares **no** `LadderContactRadiusUU` / `LadderContactIntentCos` / `LadderContactDwellSeconds` / `LadderClimbSpeedUU`; **positive controls both ways** — the probe *does* find `RecallChannelSeconds` on the hero, and it *does* find all three contact names on `AClimbableTower` and the rate on `ASummonedUnit`, so the four absences are findings rather than a broken lookup. The hero also keeps **no `FSiegeLadderContactState`** (test 20(e)).
- ⭐ **`H-3`'s sustain needs no cone tunable:** it is a **sign test** (`dot(line horizontal, steer horizontal) > 0`) plus a **frame-adjacency** freshness test — ⛔ no number at all, so nothing can drift from the tower's cone. Entering stays deliberate (60° cone × 0.35 s, the tower's); staying only requires not having turned away.

### ⭐ WHERE THE POLL LIVES — measured, because the unit side has the opposite answer

`ASummonedUnit` ships `bCanEverTick = true` **with `bStartWithTickEnabled = false`** (`SummonedUnit.cpp:140-141`) and its only tick-flag writer is `WantsActorTick(bLungeActive, LadderClimb.bActive)` (`:3662`) ⇒ **its tick is OFF in exactly the state a climb starts from**, which is why TASK-784 moved its poll onto `StateTimerHandle`. **The hero is the opposite, with evidence:**
1. `PrimaryActorTick.bCanEverTick = true` (`HeroCharacter.cpp:93`) and **no** `bStartWithTickEnabled = false`;
2. grepped `Source/`: the **only** runtime `SetActorTickEnabled` call in the entire project is `SummonedUnit.cpp:3662`, a unit writing its **own** flag ⇒ nothing anywhere can disable a hero's tick (death hides it, disables input and collision and stops movement — never the tick);
3. shipped behaviour: the recall channel's movement-cancel is serviced from this same `Tick` unconditionally, and Jonathan has played it.
⇒ `Tick` is correct for both the driver and the poll. ⭐ **Test 22 pins it:** the hero writes its tick flag **zero** times, with the same probe aimed at `ASummonedUnit` as a positive control. The day someone "optimises" a hidden hero's tick off, the suite goes red naming the reason.

---

## 6. RECALL × CLIMB — a resolved interaction, recorded as one

- **Start:** refused while channelling (§4 above).
- **Teleport:** exit `H-7` (`:1438`), **before** the broadcast that moves the actor.
- **The third direction is not re-investigated:** the recall cancel reads **position** — `HasLeftAnchor` (`HeroCharacter.cpp:1233`, called `:1360`), a full 3D `DistSquared` vs `RecallMoveCancelToleranceUU = 25.f` (`HeroCharacter.h:1159`), ticked unconditionally ⇒ a scripted `MOVE_Flying` climb **cancels a running recall within ~0.07 s at 350 uu/s**. ⚖️ The two features do not disagree about "movement": a position check is the most inclusive of the three candidates.
- ⚠️ **`H-7` kept anyway** — it becomes near-unreachable (a completion and a climb-start on the same frame, ahead of the cancel), not impossible. *An exit you cannot reach is free; an exit you removed is a hang.*

---

## 7. THE DISARM (`K-4`) — one term at the one shipped guard point

`DoMeleeAttack`'s existing early-out becomes `bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState) || IsClimbing()`. The `TOWER-§9.2` / `RECALL-§ R-5` idiom, **third** application: ⛔ no new guard point, ⛔ no new suppression mechanism, ⛔ no timer. The disarm ends on the exact frame the climb does, and a refused swing still does not consume the cooldown.
⚠️ **FLAGGED AS A PROCEEDING DEFAULT, NOT HIS RULING:** `TOWER-§9` was ruled about **units**. Extending it to the hero is `CONTACT-§7` `K-4`'s default. The *attackable* half gets **no code** — a climber is an ordinary live hero at an ordinary world location, and nothing here narrows anyone's acquisition.

---

## 8. ⚠️ THE BOARD-GRANTED RENAME (item 0c, TASK-777's `D-4`) — **ATTRIBUTE TO TASK-778**

`ELadderEntryVerdict::NotASummonedUnit` → **`NotAnAdmittedClimber`**, and the mirrored `ELadderContactVerdict` enumerator with it (they are one rule expressed twice; `EvaluateContactEntry` maps one onto the other at `ClimbableTower.cpp:673`). The name now states the **rule** — *this gate admits only climber types it can drive* — instead of the **implementation** of today's admitted set.

**Every use updated, because a half-done rename does not compile** (and TASK-780 has one compile):

| File | Sites | Note |
|---|---|---|
| `ClimbableTower.h` | 2 enumerators + 1 doc mention | the ⭐ granted file |
| `ClimbableTower.cpp` | `:267`, `:673` | " |
| `Tests/SiegeClimbableTowerTest.cpp` | 2 executable comparisons + 4 message strings | ⚠️ **TASK-777's test file.** Covered by the grant's own words *"rename the enumerator **and every use**"*; item (11)'s *"nothing else"* is scoped to `ClimbableTower.{h,cpp}` ("in that file"). ⛔ **Nothing but the identifier changed here** — no assertion was added, removed, weakened or retargeted. |
| `HeroCharacter.cpp`, `Tests/SiegeHeroLadderClimbTest.cpp` | mine | — |

⛔ **NOT touched, declared instead:** `SummonedUnit.cpp:3766` carries the old name in a **comment** listing the verdicts. It is TASK-784's live file, a comment-only staleness with **zero** compile impact, and editing a file another agent is actively writing is a write race. **Manager: hand it to 784 or to the compile task as a one-word comment fix.**
⛔ **No second verdict was added beside the renamed one**, and the rule, the precedence (identity → team → occupancy) and every behaviour are unchanged.

---

## 9. TEST LIST — **suite delta +23**, all in `Tests/SiegeHeroLadderClimbTest.cpp`

| # | Name (`Siegebound.HeroLadderClimb.…`) | What it would catch |
|---|---|---|
| 1 | `TheHeroImplementsTheClimberSeamAndTheClimbApi` | the interface or an API member going missing; **positive control** on the reflection walk |
| 2 | `TheExitReasonListIsClosedAndEnumeratedFresh` | an 11th exit added without a test; a unit enumerator (`NewOrder`, `SpellFreeze`) mapped across |
| 3–12 | `ExitH1…ExitH10` (one per exit) | **the whole feature** — an exit that stops reaching the teardown, plus the four ordering claims (abort before `DisableMovement`, before `Super::UnPossessed`, before `MOVE_Walking`, before the recall teleport, before `Super::EndPlay`) |
| 13 | `TheDriverConsumesTheShippedStaticsAndHasExactlyOneTeardown` | a second restore path; a second `End()`; a rule re-implemented instead of consumed |
| 14 | `TheEndpointLiftIsReadFromTheHerosOwnCapsule` | a literal half-height (asserted by **changing** the capsule); a capsule resize that would re-void `§8.5a` |
| 15 | `TheDeckBreachWindowIsReDerivedFromTheHerosHalfHeight` | the unit's 22 % inherited; the window not Z-resolved; the foot/midpoint losing their sweep |
| 16 | `TheBudgetComesFromTheOneShippedRateTheHeroReadsRatherThanOwns` | ⭐ **the guard on my runtime reflection read** — a rename of `LadderClimbSpeedUU` fails here loudly instead of silently refusing every climb; plus a 0-rate immortal climb |
| 17 | `TheHeroOwnsNoneOfTheTowersContactTunables` | a copied tunable, by reflection **and** by literal-scan, with controls both ways |
| 18 | `TheClimbDisarmsMeleeAtTheOneShippedGuardPoint` | a fourth guard point; a suppression mechanism; the term drifting out of the composed early-out |
| 19 | `TheClimbAddsNoKeyAndNoEscapeHandler` | `AS-§6` A-2; a key/binding/prompt; an animation reference |
| 20 | `ThePollAsksTheTowerAndNeverSelfStarts` | ⭐ **`SC-§36.1` clause 4** — the call site existing *and being reached from `Tick`*; a self-start around the team gate; a hero-side contact state |
| 21 | `TheWatchdogIsArmedOnlyForTheClimbAndClearedByEveryExit` | a watchdog armed permanently, never armed, or never cleared; a silent timeout |
| 22 | `TheHerosTickIsUnconditionalSoAPollInItActuallyRuns` | ⭐ a future `SetActorTickEnabled(false)` on the hero, which would kill both driver and poll |
| 23 | `TheShippedContactRadiusAdmitsTheHerosOwnSpeeds` | ⭐ the `K-6` reachability arithmetic for all four hero speeds (warns, does not fail — the radius is another task's file); asserts the hero owns no workaround copy |

⭐ **On instruments:** tests 3–13 and 17–22 read the shipped source between named function boundaries, because a world-less `AHeroCharacter` cannot be driven through this feature (`SetDefaultMovementMode` → `GetPhysicsVolume` dereferences `GetWorld()` unconditionally and would **crash** the suite). That is the `SiegeRecallTest.cpp` test-12 instrument, second application, and the extractor **fails loudly** when it cannot find or size a body — an exit test can never pass by finding nothing.

---

## 10. CITES I INVALIDATED (`CONTACT-§10.1`'s duty — re-grepped, ⛔ never arithmetic)

`HeroCharacter.cpp` grew 1,438 → **2,176** lines. Every live cite in the law and on the board:

| Cite, as written | New value |
|---|---|
| `HeroCharacter.cpp:61` — `bCanEverTick` | **`:93`** |
| `:163-165` — `TickRecall` from Tick | **`:195-198`** (call at `:197`) |
| `:623` — Rally cooldown `SetTimer` | **`:678`** |
| `:759` — `EndRecall` inside `HandleDeath` | **`:814`** |
| `:774` — `DisableMovement()` | **`:842`** |
| `:818` — `ResetHero`'s `SetMovementMode(MOVE_Walking)` | **`:896`** |
| `:976` — War Banner `SetTimer` | **`:1054`** |
| `:1155-1168` — `HasLeftAnchor` definition | **`:1233-1246`** |
| `:1282` — `HasLeftAnchor` call in `TickRecall` | **`:1360`** |
| `HeroCharacter.h:736` / `:740` — Walk/Sprint speed | **`:999` / `:1003`** |
| `HeroCharacter.h:896` — `RecallMoveCancelToleranceUU` | **`:1159`** |
| `ClimbableTower.h` — `ELadderEntryVerdict::NotASummonedUnit` | **renamed** → `NotAnAdmittedClimber` (§8) |

---

## 11. FILES TOUCHED

| File | Scope |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` | SOLE — 2 includes, `ESiegeHeroLadderExit`, `ILadderClimber` base, the climb API + M8 declaration, 8 internals, 9 private members |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | SOLE — the climb region (sentinel-delimited), the 5 exit call sites, the melee guard term, the Tick wiring, 4 includes, 2 file-scope constants |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroLadderClimbTest.cpp` | **NEW**, SOLE — 23 tests |
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.{h,cpp}` | ⚠️ **NARROW GRANT (0c): the rename only** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` | ⚠️ **the same rename's use sites only** (§8) |

⛔ **Not touched:** the mesh · `SummonedUnit.{h,cpp}` · `SiegeLadderClimbStatics.{h,cpp}` · `LadderClimber.h` · `SiegeGameMode.{h,cpp}` · `SiegeGhostPawn.{h,cpp}` · `CONVENTIONS.md` · any animation asset. ⛔ No compile, no editor, no MCP, no Git.

**⛔ M8 (`CONTACT-§9`):** declared in a header comment, **not built** — `ServerBeginLadderClimb` / `ServerAbortLadderClimb` (Server, Reliable, WithValidation), replicated `bLadderClimbing` + endpoints, **no new relevancy tier** (rides Tier B), and the named P1 gap (`GetAuthGameMode()` is null on a client — the identical gap the recall channel already carries, to be moved onto replicated match state in one pass rather than acquiring a second answer). ⛔ No RPC and no replicated property authored. `FSiegeLadderClimbState` stays unreflected, so nothing in it can be replicated by accident.

---

## 11a. ⭐ SELF-REVIEW BEFORE HANDOFF — I CANNOT COMPILE, SO I **SIMULATED THE SUITE**

Every source-scanning assertion in the new file was executed against the shipped bytes before I called this done — the same extraction the tests perform (signature → first column-0 `}`), the same needles, the same ordering comparisons: **47 checks, all green.** It found **two real defects, both in my own test design, and both would have gone red at TASK-780 for a reason that was not a code defect**:

1. **The `ResetHero` ordering trap.** Test 8 asserts the abort precedes the respawn's `SetMovementMode(MOVE_Walking)` — but my own *comment* above the abort quoted that call verbatim, so the first match was the prose and the ordering read backwards. **Fixed in the comment** (the code order was correct all along). ⚖️ *A source-scanning probe makes comments load-bearing; that is the cost of the instrument and it is worth naming for whoever writes the next one.*
2. **The H-10 `Cast<AHeroCharacter>` needle.** `AClimbableTower::EndPlay`'s comment *names the refused shape in prose*, so a bare-identifier search failed against **correct** code. **Fixed by making the needle code-shaped** (`Cast<AHeroCharacter>(` — a cast expression always carries its parenthesis; a sentence about one does not). ⛔ The alternative — deleting the assertion — would have removed a real `CONTACT-§4.4` guard.

⚠️ **What this instrument still cannot tell me, stated rather than implied:** whether the module compiles (UHT on the new `UENUM` + the `ILadderClimber` base + the `DoMove` override), and whether any of the ten exits fires at runtime. The first is TASK-780's compile; the second is its PIE session — and per the blocker above, **the hero half cannot be observed in PIE until the tower's start seam admits an `ILadderClimber`.** ⭐ The `EndLadderClimb` Verbose line names *which* exit fired, so when it can be observed, it will be observable.

---

## 12. ⚠️ WHAT QA SHOULD SCRUTINISE HARDEST

1. **The declared blocker (top of this file).** Is my refusal to self-start correct, and is the three-seam widening the right shape to board? ⛔ Do not read "the hero climbs" as delivered at runtime — it is delivered *up to the tower's gate*.
2. **The rename's cross-file reach** (§8). If the grant is read more narrowly than I read it, the fix is a revert of an identifier — nothing behavioural moved.
3. **`bSpellFrozen = false`.** I claim nothing corresponds and cite three files. If a hero-freezing path exists that I missed, that argument becomes wrong and the mapping must change.
4. **`H-3`'s frame-adjacency test** (`GFrameCounter > LadderClimbSteerFrame + 1`). It is my answer to an undefined tick order between the player controller and the pawn. If the project would rather have a seconds-based grace, that is a design call — but note it would introduce a number where there currently is none.
5. **`DoMove` suppression.** I override a template virtual and deliberately do **not** call `Super` while climbing. Verify that costs nothing outside a climb (it is a single bool test) and that no other input path can still feed the movement component during one.
6. **The reflection read of the rate** (`TryResolveLadderClimbSpeedUU`). It reaches into another class's CDO by property name because `CONTACT-§8` forbids a hero copy and the shipped property is `protected`. Test 16 is its guard. ⭐ **Law finding for the manager:** `CONTACT-§8` says the rate is *"read from the TOWER's shipped value"* — **the tower does not have one**; `LadderClimbSpeedUU` lives at `SummonedUnit.h:1128` and appears **zero** times in `ClimbableTower.{h,cpp}`. The intent is honoured; the cite needs repair, and the clean landing is to move the property onto `AClimbableTower` where the law already believes it lives.
7. **Suite arithmetic for TASK-780:** +23 from this task; `SiegeClimbableTowerTest.cpp`'s macro count is **unchanged** by my rename.
