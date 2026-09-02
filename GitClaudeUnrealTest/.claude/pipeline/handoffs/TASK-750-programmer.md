# TASK-750 — [GHOST-2] DEATH → GHOST → 180 s → RESPAWN — programmer handoff

**status:** ready-for-qa · **agent:** gameplay-programmer · **date:** 2026-09-01
**law:** `GHOST-§0`/`§1`/`§2`/`§3`/`§4`/`§5`/`§6` · `HELP-§5` · `RECALL-§1`/`§4` · `MARK-§ M-4` · `HIGH-§1` · `SC-§13`/`§32`/`§35` · `SHIP-§9c`

---

## 0. ⚠️⚠️ THE SCALE OF THIS CHANGE, MEASURED — READ THIS FIRST

| | |
|---|---|
| shipped `HeroRespawnDelay` before this task | **5.0 s** (`SiegeGameMode.h:48`, comment `// GDD §3.1: exactly 5`) |
| GDD §3.1's sentence | *"back within 5-6 s"* |
| **shipped now** | **180.0 s** |
| **factor** | **36×** |

⛔ **It was NOT softened, and no agent may soften it.** It is Jonathan's explicit design call (`GHOST-§0`), shipped as **ONE `EditDefaultsOnly` value** so TASK-752's raze-time measurement can move it in the editor with no recompile. A test asserts that **no second float on either class I own holds 180**, so the retune is provably one edit.

⚠️ **GDD §3.1 IS NOW FALSE and `Docs/GDD.md` WAS NOT EDITED** (⛔ no agent edits it). The `// GDD §3.1` comment beside the value was **rewritten, not left lying** — it now records whose ruling it is, the number, the 36× factor, and the consequence. **FOR-JONATHAN row: the GDD sentence needs his correction.**

---

## 1. THE LIFECYCLE STATE MACHINE

```
        ┌──────────────── ALIVE ─────────────────┐
        │  PC possesses AHeroCharacter           │
        └───────────────┬────────────────────────┘
                        │ AHeroCharacter::HandleDeath()
                        │   ├─ EndRecall(InterruptedByDeath)   ← TASK-748's, exit 5 / G-6
                        │   ├─ hide + no collision + DisableInput
                        │   └─ OnHeroDied.Broadcast
                        │        ├─(1st)→ ASiegePlayerController::HandleHeroDied
                        │        │        ExitPlacementMode / ExitTargetingMode / CancelGroupPick
                        │        └─(2nd)→ ASiegeGameMode::HandleHeroDied
                        ▼
        ShouldEnterGhostState(bMatchEnded, bHasController)   ← THE ONE PREDICATE
                 │ false                        │ true
                 ▼                              ▼
     nothing scheduled,          SetTimer(HeroRespawnTimers[PC], 180 s)   ← armed FIRST
     no ghost, hero stays        SpawnAndPossessGhost(PC, DeadHero)
     down (match-end rule,         ├─ SpawnActor<ASiegeGhostPawn>(AlwaysSpawn) at the death transform
      GHOST-§2, inherited)         ├─ Ghost->InitializeGhost(Hero->GetTeamId())   ← 749 API ①
                                   ├─ PC->Possess(Ghost)   ← engine unpossesses the hero; it stays in the world
                                   ├─ ActiveGhosts.Add(PC, {Ghost, Hero})
                                   └─ PC->HandleGhostPossessionChanged()  ← ClearUICursorHold + ApplyCursorInputState
                                            ▼
        ┌──────────────── GHOSTED (≤ 180 s) ─────────────────┐
        │  orders ✅ · AI commander ✅ · war map ✅            │
        │  cards ⛔ · attack ⛔ · be attacked ⛔ · capture ⛔   │
        └───────────────┬────────────────────────────────────┘
                        │ EXIT 1: timer fires   EXIT 2: match end   EXIT 3: PlayAgain   EXIT 4: EndPlay
                        ▼
        RetireGhostFor(PC, Reason)
          ├─ Hero = ResolveHeroToRestore(PC->GetPawn(), TrackedHero)   ← returns the SAME hero actor
          ├─ PC->Possess(Hero)          ← ⛔ POSSESS FIRST
          ├─ Ghost->RetireGhost()       ← ⛔ DESTROY SECOND (749 API ③, idempotent)
          └─ PC->HandleGhostPossessionChanged()
                        │
                        ▼ (exit 1 only)
        RestoreHeroAtStart(PC)  ← ⛔ UNCHANGED, byte-for-byte
          teleport to GetHeroStartTransform(PC, Hero->GetTeamId()) → ResetHero() → SetControlRotation
```

**A ghost exists if and only if a respawn is pending.** Both halves are gated by the same pure predicate and `ActiveGhosts` is keyed and torn down at exactly the four sites `HeroRespawnTimers` is — so "a permanent ghost" and "180 s with no pawn at all" are unrepresentable, not merely unlikely.

---

## 2. THE SINGLE TUNABLE

**`ASiegeGameMode::HeroRespawnDelay`** — `UPROPERTY(EditDefaultsOnly, Category="Siegebound|Hero", ClampMin=0)`, `= 180.0f`.

⭐ **The ghost has no lifetime of its own.** It is retired by the timer's own callback, so there is no second duration to keep in step. `ASiegeGhostPawn` owns **no timer at all** (749 states this), and test 2 walks every declared float on `ASiegeGameMode` + `ASiegePlayerController` and asserts **exactly one** holds 180 and that it is named `HeroRespawnDelay`.

---

## 3. HOW DEATH-MID-RECALL RESOLVES — and it needed ZERO code from me

**TASK-748 already lands the abort.** Measured at source: `AHeroCharacter::HandleDeath()` calls `EndRecall(ESiegeRecallExit::InterruptedByDeath)` — `RECALL-§4` exit 5, `GHOST-§ G-6` — as its **first statement**, before `OnHeroDied.Broadcast`. ⇒ **the channel is already cleared before my death path runs at all.** `EndRecall`'s `bChannelling` latch makes it idempotent with exit 4 (lethal damage reaches both in one stack).

**What I owe, and what I verified:**
- ⛔ **No recall STATE exists on either class I own** — asserted by reflection over declared *properties* (functions excluded deliberately; see below). The game mode owns exactly two per-controller maps, both cleared at the same four sites.
- ⭐ **The ghost cannot inherit or continue a channel**: it is not an `AHeroCharacter`, declares nothing recall-related, and the recall binding lives on the hero pawn's input component, which the engine destroys on unpossess. Asserted at the type.
- ⛔ **I add no second recall teardown.** A second clear would be a second source of truth for a rule that already has one.

### 🚩 SCOPE ADDITION — DECLARED, NOT SMUGGLED: I bound `OnHeroRecallArrived`

`HeroCharacter.cpp:1332` ships this warning, verbatim: *"The teleport-home owner must bind this delegate **(TASK-750)**."* **Nothing bound it.** ⇒ a COMPLETED 10 s recall channel **moved and healed nothing** — TASK-748's headline feature shipped inert with one warning per hero.

I bound it: `ASiegeGameMode::HandleHeroRecallArrived`, bound in `SetPlayerDefaults` beside `OnHeroDied` (same site, same `AddUniqueDynamic` idiom, same class of seam).

- ⭐ **It teleports through `GetHeroStartTransform` — the SAME resolver the 180 s respawn uses.** "Back at the castle" is now decided in exactly one place, so a completed recall and a respawn can never arrive at different homes.
- ⛔ **It calls NOTHING else, and specifically never `ResetHero()`** — the delegate's own contract names that as the trap: on a *live* hero `ResetHero` re-applies every cumulative upgrade modifier and re-arms a running War Banner aura. The heal is the hero's own, applied by `EndRecall` the instant this returns.
- **QA:** if you rule this out of scope for TASK-750, it is **one bind + one handler** to remove and I will remove it. My reasoning for shipping it: the seam is inside my sole-owned fence, it names this task by number, and leaving it would ship a whole feature dead.

---

## 4. HOW I AVOIDED DOUBLE-APPLYING UPGRADES — three distinct places

`ResetHero()` is a **death-path** function: it re-applies cumulative upgrade mods onto a freshly restored base, re-arms the War Banner aura, resets Rally, restores input, re-broadcasts the loadout. Correct on the death path — catastrophic anywhere else.

1. **⭐ ONE hero actor lives across the whole death.** `ResolveHeroToRestore(PossessedPawn, TrackedHero)` returns the *tracked* hero when a ghost is possessed.
   ⚠️ **Without it:** `RestoreHeroAtStart`'s `Cast<AHeroCharacter>(Player->GetPawn())` returns null (the pawn is a ghost) → the defensive `RestartPlayer` branch fires → **a SECOND hero is spawned**, the first is orphaned in the world *still carrying every upgrade stack*, and `ResetHero()` re-applies those stacks onto the corpse while the player drives a blank pawn. Test 4(b) proves the previous implementation returned null here before asserting the new one does not.
2. **⛔ `PlayAgain()` retires ghosts at step 1b — BEFORE step 5.** Step 5 reaches the hero through `IterPC->GetPawn()`, so a Play Again pressed while a player is ghosted would **skip `ResetUpgrades()` entirely** and the very next `ResetHero()` would re-apply the OLD stacks onto the "reset" hero — **upgrade stacks surviving a full match reset**. This is a live bug the ghost would have introduced; the ordering is the fix and it is commented as load-bearing at both the code and the `PlayAgain` doc-comment.
3. **⛔ The recall-arrival handler never calls `ResetHero()`** (§3 above).

`RestoreHeroAtStart` itself is **byte-identical** — ⛔ the teleport and the heal are not re-implemented anywhere (`GHOST-§2`).

---

## 5. THE CURSOR / INPUT HAND-OFF (`GHOST-§4` — the highest-risk line in the batch)

- ⛔ **There is no `SetInputMode` and no `bShowMouseCursor` write in `SiegeGameMode.{h,cpp}`. Grep it.** The game mode's only posture action is calling `ASiegePlayerController::HandleGhostPossessionChanged()`, which does `ClearUICursorHold()` then `ApplyCursorInputState()` — **the one owner** (`HELP-§5`).
- ⛔ **The ghost adds NO term to the cursor-owner ladder and re-orders nothing.** It is a free-look pawn with the hero's own movement and vision (`G-1`), so it wants the posture the hero wanted. Placement · Alt-held `IA_UICursor` · end screen · targeting · group-pick · war map · assistant console · help overlay keep their exact shipped precedence.
- ⭐ **Ordering, measured rather than assumed:** `AGameModeBase::FinishRestartPlayer` calls `Possess()` (→ the controller's `OnPossess` binds *its* `HandleHeroDied`) **before** `SetPlayerDefaults()` (→ the game mode binds *its* handler). Delegates fire in binding order ⇒ **the controller's handler runs FIRST** and has already cancelled placement / targeting / group-pick — each through its own `ApplyCursorInputState()` — before the ghost is spawned. **The ghost never possesses under a live cursor mode.**
- **`ClearUICursorHold()` on the swap** mirrors the shipped `HandleMatchEnd` idiom for the identical reason: a swallowed *release* would leave `bUICursorHeld` latched and the counter-based `SetIgnoreLookInput` unbalanced — **a ghost that cannot look around for 180 seconds**. Measured caveat written at the code: the hold's binding is on the *controller's* own input component and the key reaches it through IMC_Hero, which is only ever added and never removed, so the release very probably does still arrive — ⛔ *"very probably"* is what `GHOST-§4` refuses to stake three minutes on. Cost of the belt: one re-press of Left Alt.
- ⛔ **`Escape` is untouched.** No key, action, `NativeOnKeyDown` or viewport intercept was added anywhere in this task (`AS-§6` A-2).

### ✅ THE RESPAWN-SIDE INPUT-CONTEXT CHECK THE ORCHESTRATOR ASKED FOR — the reverse trap does NOT exist

749 found that `IMC_Hero` is added by the **HERO PAWN**, not the controller, so nothing re-adds it when possession leaves the hero. I checked the mirror image on the respawn hand-off:

1. `APawn::NotifyControllerChanged()` fires on **possession as well as unpossess** ⇒ `AHeroCharacter::NotifyControllerChanged` **re-adds IMC_Hero itself** (with its `KBD-§5`/`§6` positional-layout resolve) the instant `PC->Possess(Hero)` lands. The hero re-arms itself, by the same mechanism the ghost mirrors in the other direction.
2. ⭐ **Stronger: there is no window to be caught in.** `ASiegeGhostPawn` adds the **same context** (`IMC_Hero`) at the **same priority** (`GhostMappingContextPriority == 1 == HeroMappingContextPriority`), and **there is not one `RemoveMappingContext` call in the entire module** (grepped). ⇒ the input composition is **invariant across the whole death → ghost → respawn cycle**, and `AddMappingContext` collapses the duplicate into one TMap entry.
3. ⛔ **I therefore add no context and remove none.** Doing so would be a second owner of a composition that already has exactly one per pawn.
4. **Test 9 makes this falsifiable**: each pawn declares its own mapping context, the controller declares none, the game mode declares none — so the day someone "centralises" the context onto the controller (which looks like an obvious tidy-up) the suite goes red and names the reason.

⚠️ **This is a structural argument and it is NOT a substitute for `GHOST-§4`'s PIE obligation.** See §9.

---

## 6. ✅ `ClearMarks()` — TASK-744's cross-task line, CONFIRMED IN

`ASiegeGameMode::PlayAgain()` **step 6b** (after step 6's controller walk), per `MARK-§ M-4`:

```cpp
for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
{
    const APlayerController* const IterPC = It->Get();
    if (!IterPC) { continue; }
    if (ULocalPlayer* LocalPlayer = IterPC->GetLocalPlayer())
    {
        if (USiegeMapMarkSubsystem* MarkSubsystem = LocalPlayer->GetSubsystem<USiegeMapMarkSubsystem>())
        {
            MarkSubsystem->ClearMarks();
        }
    }
}
```

- Call shape taken **verbatim from `handoffs/TASK-744-programmer.md` §3** (the ordinary local-player route; ⛔ no custom getter, ⛔ nothing improvised).
- Null-safe at every step: a remote client's server-side PC has no `ULocalPlayer` and is skipped — correct, because marks are per-player (`M-2`) and each machine clears its own.
- Documented at the code and in the `PlayAgain` doc-comment step list with **why it has to live here**: the subsystem is a `ULocalPlayerSubsystem` and **outlives an in-place `PlayAgain`**, so without it last match's numbered circles are still painted on next match's map and still nameable to the AI commander.
- New includes: `Engine/LocalPlayer.h`, `Siegebound/SiegeMapMarkSubsystem.h`.

---

## 7. FILES TOUCHED

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.h` | `HeroRespawnDelay` **5 → 180** + comment rewritten · `GhostPawnClassAsset` · `ShouldEnterGhostState` / `ResolveHeroToRestore` (public pure statics) · `ResolveGhostPawnClass` / `SpawnAndPossessGhost` / `RetireGhostFor` / `HandleHeroRecallArrived` · `FSiegeGhostState` · `ActiveGhosts` · `ResolvedGhostPawnClass` · `bWarnedGhostClassMissing` · class comment: the lifecycle + **the M8 tier declaration** · `PlayAgain` doc-comment steps 1b + 6b |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` | the four new functions · ghost spawn/possess in `HandleHeroDied` · retire at the 4 timer sites (`HandleHeroRespawnTimer`, `OnCastleDestroyedHandler`, `PlayAgain` 1b, `EndPlay`) · `PlayAgain` **6b `ClearMarks`** · `OnHeroRecallArrived` bind + handler · respawn-side input-context reasoning at the possess site |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | `IsGhostPossessed()` · `CanPlayCardsWhilePossessing()` (pure static) · `HandleGhostPossessionChanged()` · `GetFollowAnchor` doc rider (**G-8**) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | the three functions · ghost card-play gate in **`PlayHandSlot`** *and* **`EnterPlacementMode`** · `GetFollowAnchor` **G-8** comment |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp` | **NEW**, 9 tests |

**Assets referenced:** none new. `GhostPawnClassAsset` ships **UNSET** — no task in this batch produces a ghost blueprint, so the raw C++ `ASiegeGhostPawn` is the shipped ghost and pointing at a nonexistent asset would warn every match forever. **Declared deviation** from "warn once and fall back": the resolver distinguishes **UNSET** (expected → `Log` once) from **AUTHORED-BUT-UNRESOLVABLE** (mis-config → `Warning` once). Both fall back; neither crashes; neither can produce a dead 180 seconds.

**⛔ Files NOT touched:** `HeroCharacter.{h,cpp}` · `SiegeGhostPawn.{h,cpp}` · 744's four files · `WarMapWidget.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SummonedUnit.{h,cpp}` · `Tower.{h,cpp}` · `ClimbableTower.{h,cpp}` · `Docs/GDD.md`. ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.

---

## 8. TESTS — 9 new, `Siegebound.RespawnLifecycle.*`, and what each would CATCH

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeRespawnLifecycleTest.cpp` — headless (CDOs + reflection + the pure statics), ⛔ no world, ⛔ no `SpawnActor`, ⛔ no asset load, matching every other file in that directory.

| # | test | ⛔ the wrong implementation it kills |
|---|---|---|
| 1 | `HeroRespawnDelayIsExactlyOneHundredEightySeconds` | the value silently reverted to 5; a non-`EditDefaultsOnly` (uneditable, or instance-editable) tunable. **Self-checks:** a different property with a different value (300) reads through the same call; a `Transient` property proves the flag reader can say NO |
| 2 | `TheThreeMinutesIsASingleTunableWithNoSecondCopy` | ⭐ a SECOND 180 anywhere on either class — a ghost lifetime, a death-screen duration, a UI countdown — which would make the retune two edits. **Self-checks:** the walk found ≥4 floats; the value comparison matches something else (300) |
| 3 | `GhostStateIsEnteredExactlyWhenARespawnIsScheduled` | ⭐ **a ghost spawned after match end** (a second match-end rule); a ghost with no controller; a predicate that hard-returns a constant (asserted explicitly) |
| 4 | `RespawnRestoresTheSameHeroActorAndNeverASecondOne` | ⭐⭐ the naive `Cast<AHeroCharacter>(GetPawn())` resolve — **proven to return null for a ghost before the row is asserted**, so the row cannot pass vacuously; a resolve that hands back the GHOST; a stale tracked hero winning over the possessed one |
| 5 | `TheGhostCannotPlayCardsAndEveryOtherPawnIsUnchanged` | the ghost playing cards (`G-5`); a gate that broke the LIVING hero or the no-pawn case; ⭐ **an `IsA<AHeroCharacter>` implementation** — killed by requiring a plain `APawn` to be allowed |
| 6 | `OrdersCommanderAndMapAreBoundOnTheControllerNotThePawn` | ⭐ any of `G-3`'s powers migrating onto the hero pawn, which would silently strip it from the ghost with a clean compile. **Self-check:** `SprintAction` must MISS on the controller and HIT on the hero |
| 7 | `NoRecallStateLivesOnTheGameModeControllerOrGhost` | recall state leaking into the lifecycle classes; a ghost that could carry a channel. **Self-check:** the property scan must FIND `Respawn` on the mode and `Recall` on the hero before its misses count. Row (d) asserts the mode DOES own the recall **destination** handler |
| 8 | `GhostPawnClassAssetIsTypedToTheGhostAndShipsUnset` | a `TSoftClassPtr<APawn>` that could spawn any pawn — or a hero — as the ghost; a default path to a nonexistent asset. **Self-check:** the same reader sees `HeroPawnClassAsset` as SET with a different MetaClass |
| 9 | `EachPawnOwnsItsOwnMappingContextAndTheControllerOwnsNone` | ⭐⭐ the `GHOST-§4` disaster: "centralising" IMC ownership onto the controller, or either pawn losing its own — the input-dead arena, caught at compile-time-adjacent |

**Every assertion can fail**, and every instrument carries a self-check that fires *before* the claim if the instrument has gone blind (`SHIP-§9c`).

### Suite total for TASK-754 — the arithmetic, ⛔ not a frozen number
- **HEAD baseline: 171** (744's measured figure, matches the board).
- **This task's delta: `+9`.**
- ⚠️ The working tree read **218** while I authored (744's +9 and 749's +8 had landed; other files are dirty). ⛔ **TASK-754 must not take 218 or 227 from this note.** **TASK-753 reconciles `171 + Σ(declared deltas)`; my declared delta is exactly `+9`.**

---

## 9. 🚩 FINDINGS, FLAGS AND DEVIATIONS QA SHOULD SCRUTINISE

1. ⛔⛔ **THE PIE ROW IS UNDISCHARGED AND MAY NEVER BE WAIVED ON A CLEAN COMPILE.** `GHOST-§4` / `SC-§35` ruling 5 owes **ONE PIE session with a MESSAGE-LOG READ** because this batch spawns a new actor class into the world. My dispatch explicitly withheld editor/MCP access (⛔ no compile, editor, MCP or Git), so **I could not run it and I did not fake it.** ⭐ **It must be scheduled before this ships.** What only PIE can settle: that the player actually has working input on the ghost, that the camera hands off both ways, and that the ghost walks the ground rather than falling through it.
2. 🚩 **SCOPE ADDITION — `OnHeroRecallArrived` bound (§3).** Deliberate, argued, and trivially revertible. Ruling requested.
3. ⭐ **`GetFollowAnchor()` now returns THE GHOST while ghosted — `G-8`, and it cost zero lines.** This IS a behaviour change from shipped (a dead hero returned null → followers HOLD POSITION; now they follow the ghost) and it is the ruled behaviour: *"resolving `hero` to a hidden corpse would silently walk the player's army to where he died ⇒ `rally` and `follow` keep working and the ghost is the anchor."* Written into the function so a later "tidy-up" that casts to `AHeroCharacter` first is told it is overturning a Jonathan-level ruling. **Honest limit:** `rally` still refuses while ghosted, correctly — `ExecuteRallyOrder` casts the anchor to `AHeroCharacter` because `Rally()` is a hero ability. That file is 746's and is untouched.
4. 🚩 **FOR JONATHAN — `G-5` (cards while ghosted).** Shipped as the ruled default: **NO**. It lives in **one function**, `CanPlayCardsWhilePossessing`, and flipping it is a single `return true` with no other edit. ⚠️ Worth his eye beside `GHOST-§0`: no attacking + no cards + 180 s = a spectator with a chat window for three minutes.
5. ⚠️ **THE GHOST SILENTLY UNDOES A SHIPPED REFUSAL, AND THAT IS WHY THE GATE EXISTS.** `EnterPlacementMode`'s shipped guard is `Cast<AHeroCharacter>(GetPawn())` → `IsDead()` → *"Hero is down"*. **The instant a ghost is possessed that cast returns null and the guard stops firing.** Both entries (`PlayHandSlot` and `EnterPlacementMode`) are gated, with the **same refusal string**, because it is the same fact.
6. 🚩 **NOT BUILT, DECLARED:** the paid enemy-reveal survey (`SiegePlayerController.cpp`) iterates `AHeroCharacter` and skips `IsDead()`, so **the ghost is not dotted on the war map**. `G-4` ("the enemy should be able to see this ghost") is satisfied by the ghost being a visible world actor — 749's job — not by the reveal purchase. Adding it is a design ruling nobody gave me. **Flagged, not built.**
7. 🚩 **NOT BUILT, DECLARED:** discarding a card for 1 gold is still allowed while ghosted. `G-3`'s list is closed and discard is not *playing* a card; it reshapes the hand for after the respawn and moves no world state. **One line to close if ruled otherwise.**
8. ⚠️ **BEHAVIOUR NOTE for review:** retiring the ghost at **match end** re-possesses the still-dead hero, which makes the engine push the hero's input component back onto the stack — where before it stayed popped until `ResetHero`. Practically inert (`HandleMatchEnd` immediately applies UI-only input; the hero is `bDead` with movement disabled), and it restores the exact pre-ghost state of *"the hero stays down; `PlayAgain()` revives it"*. Called out rather than left for QA to find.
9. ⚠️ **`ActiveGhosts` and `HeroRespawnTimers` must stay in lockstep.** Any future edit that clears one without the other breaks the "ghost iff pending respawn" invariant. The four call sites are deliberately adjacent in every function.
10. ✅ **M8 declared (`GHOST-§6`), ⛔ not copied from another batch:** `ASiegeGhostPawn` is replicated-relevant (player-possessed, `G-4` enemy-visible) — 749 declares **Tier B**, arena-scaled, matching the hero for the stated reason. **⛔ No RPC and no replicated property are authored here**; `ActiveGhosts` is the per-controller server-side shape, the same shape `HeroRespawnTimers` already documents (`ACC-§8`).
11. ⚠️ **Compile order.** `SiegeGameMode.cpp` and `SiegePlayerController.cpp` now include `SiegeGhostPawn.h` and `SiegeMapMarkSubsystem.h`. Both **have landed** (749, 744), so the earlier "will not compile until 749 lands" state is **resolved** — but this task was still authored without a compile, per the dispatch.
