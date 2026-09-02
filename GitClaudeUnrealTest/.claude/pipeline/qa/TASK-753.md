# QA Report — TASK-753 — [GATE-1] THE ONE GATE over MARKS + RECALL + GHOST

**Verdict: PASS** — **0 BLOCKERS** · **14 WARN** · **5 NIT**
**Scope (`SC-§27`, diff-scoped):** TASK-**744 · 745 · 746 · 747 · 748 · 749 · 750 · 756 · 757 · 758**.
⛔ `qa/TASK-741.md` (the ladder wave's gate) was **NOT re-opened**. TASK-751 is `backlog` and is **not in this gate**.
**Law cited, ⛔ not restated:** `MARK-§0..§6` · `RECALL-§1..§6` · `GHOST-§0..§6` (incl. `§1`'s self-correction and `§4`'s possession clause) · `SC-§36` · `SC-§37` · `SC-§13`/`§15`/`§21`/`§27`/`§32`/`§35` · `AS-§6 A-2` · `AS-§12f`/`§12g` · `HELP-§5` · `HIGH-§1` · `SHIP-§9c` · `WM-§8c`/`§8d`/`§8e` · `WR-§6`/`§9` · `KBD-§2a`/`§4`/`§5`/`§6` · `TOWER-§8`/`§9.2`.

⛔⛔ **THIS PASS IS A PASS ON THE DIFFS AND IT IS ⛔ NOT A PASS ON RUNTIME.** `GHOST-§4` / `SC-§35` ruling 5's **PIE row is UNDISCHARGED** and is carried to TASK-754 as a **commit gate that may ⛔ NEVER be waived on a clean compile, and ⛔ never discharged by reasoning about the code** (§9 below). ⚖️ *`SC-§35` was bought by 1,806 Blueprint runtime errors that compiled clean and cleared two QA gates: a batch in which no task ran PIE has observed NOTHING about runtime, however green its gates.*

---

## 1. ⭐⭐ THE ZONE-A FREEZE — the single most important check. ✅ HELD.

| | |
|---|---|
| `ShippedZoneAChars` | **5658**, `SiegeAssistantZoneATest.cpp:370` — **unmodified**, still the 2026-08-05 named/dated baseline |
| `ZoneA.MeasuredCharCount` | **NOT touched** (TASK-746 D8: it extended `ZoneA.StaticPrefixContract` instead, which is the genuinely new claim — *"5658 **with nine marks published**"*) |
| `BuildZoneA` | **not edited**; it reads ⛔ no member state, and `PlaceNames` is member state ⇒ the freeze is **structural**, not disciplinary |
| `GetRegionPlaceNames()` | **`M-6` held** — `AppendMarkPlaces` does not take `RegionPlaceNames` as a parameter, so it is **not expressible** for it to write one (`SiegeAssistantSnapshot.cpp:860-864`) |
| `MARK-§1`'s costed 42/56-char fallback | ⛔ **NOT taken**; a test goes red if it ever is |
| `ZoneBCharReserve` / `SnapshotTrimBudgetChars` | **read, never written** |
| `AS-§12g` / `AS-§12f` | ⛔ no token figure and ⛔ no accuracy figure anywhere in the batch — every quantity in TASK-746's handoff is **characters**, and its provenance is labelled (99/108 independently re-counted; 621/158 are the file's own documented figures, declared as a derivation) |
| 🔒 `assistant_eval_holdout2.csv` | **untouched, uncited, unopened** |

✅ **Zone A moved zero bytes and the feature needed none.** `MARK-§1`'s five source readings were re-verified by TASK-746 at source and the load-bearing one (`SiegeAssistantSnapshot.cpp:1109`, `WHERE = a place symbol from places in [FORCES]`) is now **pinned as a failable assertion**.

---

## 2. ⛔⛔ THE GHOST IS UNTARGETABLE **STRUCTURALLY** — verified, and verified **NOT UNDERMINED**.

| check | result |
|---|---|
| `class GITCLAUDEUNREALTEST_API ASiegeGhostPawn : public ACharacter` | ⛔ **no `ITeamAgent`**, ⛔ **no `IHealthBarProvider`** — `SiegeGhostPawn.h:247` |
| suppression flag / damage-rejection branch | ⛔ **ZERO.** Grepped the whole class pair: no `TakeDamage` override, no `bInvulnerable`, no `SetCanBeDamaged`, no `Immune`, no targeting filter. The only hits are **comments forbidding them** |
| all **eight** acquisition sites | enumerate via `GetAllActorsWithInterface(UTeamAgent::StaticClass())` — TASK-749 swept **six beyond the two the law cited**, incl. ⭐⭐ `FSiegeCombatStatics::ApplyRadialDamage` (**every AoE** — the lane that catches bystanders who were never "acquired") |
| the two **non-enumeration** projectile lanes | `Projectile.cpp:352` is target-locked at fire time; `:434 FindTerrainHit` traces `WorldStatic`/`WorldDynamic` **object types only** and then requires a `Terrain`/`Obstacle` tag ⇒ the ghost is excluded **twice** |
| ⛔ **capsule OBJECT TYPE** | **`ECC_Pawn`**, `SiegeGhostPawn.cpp:66` — with the reason written at the line. ✅ **TASK-758 did not touch one line of the constructor** (verified: `InitCapsuleSize` → `SetCollisionObjectType(ECC_Pawn)` → all-ignore → `WorldStatic` Block → `Pawn` Ignore → `QueryAndPhysics` are intact). ⇒ ⛔ **no projectile shield for a dead player** |
| `SummonedUnit.*` · `Tower.*` · `ClimbableTower.*` | ⛔ **NOT written by any task in this batch** — none of this batch's symbols (`GhostPawn`, `SiegeMapMark`, `RecallState`, `IsRecalling`, `OnHeroRecallArrived`) appears in any of the three. ⇒ ⛔ no design violation **and** ⛔ no write-collision with the live ladder wave |
| the test | `FSiegeGhostPawnNotATeamAgentTest` asserts **at the type**, in **two independent lanes** — reflection (`ImplementsInterface`, the exact predicate the enumerations filter on) **and** cast (`Cast<ITeamAgent>`, what every acquisition performs after gathering) — with hero/unit/tower self-checks returning TRUE. ⇒ ⛔ **cannot pass vacuously**, and it goes red the day someone adds the interface "for consistency" |

### ⚖️ RULING — TASK-749's declared base-class deviation (`ACharacter`, not the law's literal `APawn`): **ACCEPT.**
`ACharacter` **IS-A** `APawn`, so every statement `GHOST-§1` and `GHOST-§4` make about the type stays **literally true** (including *"is an `APawn`, so `TryGetPawnOwner()` resolves"*). The law's **binding content** — a pawn that does not implement `ITeamAgent` — is honoured exactly, and **test 4 machine-checks the `APawn` claim at the type**, so the deviation can never quietly become a violation. `UCharacterMovementComponent` (gravity, ground-walking, step-up, slopes) **requires** an `ACharacter` owner; the alternatives were weighed and are worse (`UFloatingPawnMovement` **flies**, breaking `G-1`; a hand-rolled gravity re-implements an engine component for a worse result). It also supplies the `USkeletalMeshComponent` `GHOST-§4` anticipates and the capsule `G-2` configures. ⇒ **No amendment owed; the deviation is recorded here as ruled.**

---

## 3. ⭐ THE POSSESSION TRAP (`GHOST-§4`) — the invariant genuinely holds. ✅ VERIFIED INDEPENDENTLY.

**The premise, re-measured:** `IMC_Hero` is added by the **PAWN**, ⛔ not the controller — `AHeroCharacter::NotifyControllerChanged` (`HeroCharacter.cpp:294`, `Subsystem->AddMappingContext(ContextToApply, HeroMappingContextPriority)`), whose own comment records that `ASiegePlayerController` *"does not add contexts the way the template controllers do."* ⇒ a naive hand-off leaves the player controlling **nothing for 180 s**.

**The three legs, each checked at source:**

1. **The ghost re-adds it.** `ASiegeGhostPawn::NotifyControllerChanged` mirrors the hero's guard chain (`GhostMappingContext` → `APlayerController` → `ULocalPlayer` → `UEnhancedInputLocalPlayerSubsystem`) with the `KBD-§5`/`§6` positional resolve **in the innermost scope** — correct, because `NotifyControllerChanged` also runs **on the server for a remote client's pawn**, and hoisting it would probe an OS keyboard layout for a machine that is not there (`SiegeGhostPawn.cpp:428-460`).
2. ⭐ **SAME CONTEXT, SAME PRIORITY — measured, not asserted.** `GhostMappingContextPriority = 1` (`SiegeGhostPawn.cpp:37`) **==** `HeroMappingContextPriority = 1` (`HeroCharacter.cpp:42`).
3. ⭐⭐ **AND THERE IS NO WINDOW TO BE CAUGHT IN — I grepped the entire `Source/` tree myself: `RemoveMappingContext` appears exactly TWICE, and BOTH are comments** (`SiegeGameMode.cpp:904`, `SiegePlayerController.cpp:1374`). **Zero call sites. Zero `ClearAllMappings`.** ⇒ the input composition is **invariant across the whole death → ghost → respawn cycle**, `AddMappingContext` collapses the duplicate into one entry, and the respawn side genuinely needs nothing.

⇒ ✅ **"Add on both, remove on neither" is safe, and TASK-750 was right to add no context and remove none.** Test 9 (`EachPawnOwnsItsOwnMappingContextAndTheControllerOwnsNone`) makes it falsifiable: the day someone "centralises" IMC ownership onto the controller — which looks like an obvious tidy-up — the suite goes red **and names the reason**.

⚠️ **This is a STRUCTURAL argument. It is evidence, ⛔ not an observation** — see §9.

---

## 4. ⛔ THE TWO RECALL TRAPS (`RECALL-§1`) — both asserted, ⛔ not merely absent. ✅ HELD.

**Trap one — `ResetHero()` is ⛔ NOT on any recall path.** Three independent guarantees:
- **Structural:** the arrival is a **two-field `USTRUCT`** `FSiegeRecallArrival { HealTargetHP, bTeleportHome }` (`HeroCharacter.h:130-149`). The death-path restore's five extra effects (cumulative upgrade re-apply · War Banner aura re-arm · Rally-cooldown reset · input restore · loadout re-broadcast) are **not expressible in it.** **T11** enumerates the struct by reflection and fails on a third field.
- **Textual:** **T12(b)** — 0 occurrences of the token inside the recall region, positive control ≥2 elsewhere in the same file (measured 0-in-region / 7-in-file).
- **At the binder:** `ASiegeGameMode::HandleHeroRecallArrived` (`SiegeGameMode.cpp:470-507`) performs `GetHeroStartTransform` → `SetActorLocationAndRotation(..., TeleportPhysics)` → `SetControlRotation` → one `Log`. **Nothing else. ⛔ No `ResetHero()`, ⛔ no possession change, ⛔ no input change, ⛔ no HP write.** Read line by line.

**Trap two — the heal reads `GetEffectiveMaxHP()`, ⛔ never `MaxHP`.** Single call site, `EndRecall`: `BuildArrival(Exit, bDestinationOwnerBound, GetEffectiveMaxHP())` (`:1311`). **T12(c)** asserts on the shipped text that `count("MaxHP") == count("EffectiveMaxHP") + count("GetMaxHP")` — i.e. **every** mention of the base field's name is part of an effective read (measured 4 == 3 + 1). **T8** proves the arithmetic with both numbers **re-derived from the CDO by reflection**, and asserts the bonus is non-zero so the claim cannot pass on a hero where the two readings agree.

**⭐ ATOMICITY — CONFIRMED AS THE RIGHT RULING.** `ExitGrantsArrival(Exit, bDestinationOwnerBound)` returns true for **exactly one** of seven exits **and only with a destination owner bound** (`:1186-1197`). ⇒ **nothing bound ⇒ no teleport ⇒ NO HEAL.** A wiring gap can therefore never become a **free full refill from anywhere on the map**. ⚖️ *A feature that is inert until integrated is a visible bug; one that half-fires into an exploit is an invisible one.* ✅ **Ruling upheld.**

**THE SEVEN EXITS — each walked, each clears exactly once.**

| # | exit | site | verified |
|---|---|---|---|
| 1 | Completion | `TickRecall` → `IsComplete` (`:1289`) | ✅ checked **last**, so a completion cannot beat a match end or a walk-away on the same frame |
| 2 | Re-press `B` | `HandleRecallInput` (`:1220`) / `CancelRecall` (`:1259`) | ✅ state **replaced**, never amended ⇒ `R-4` from zero |
| 3 | Movement | `TickRecall` → `HasLeftAnchor` (`:1284`) | ✅ full 3-D distance, negative tolerance clamped |
| 4 | Damage that landed | `TakeDamage` (`:683-686`) | ✅ **after** the shipped `ActualDamage <= 0` early return, **before** `HandleDeath()` — `R-1` in one named testable place |
| 5 | Death (`G-6`) | `HandleDeath` (`:759`) | ✅ **first statement**, before `OnHeroDied.Broadcast` ⇒ the ghost hand-off inherits a hero with no channel |
| 6 | Match end | `TickRecall` → `IsMatchOver` (`:1276`) | ✅ checked **first** |
| 7 | `EndPlay` | override (`:1433`), before `Super` | ✅ while the tell is still valid |

⭐ **"Exactly once" is STRUCTURAL, not a promise:** every exit routes through `EndRecall`, whose **first line** refuses a second run for one channel (`:1301-1304`). Lethal damage genuinely reaches exits 4 and 5 in one call stack; the second lands on the latch.

**`R-5` disarm** rides the **existing** guard with one state term — `if (bDead || bMeleeSuppressed || FSiegeRecallStatics::IsAttackDisarmed(RecallState))` (`:401`). ⭐ A **function of STATE**, so the same hero answers *disarmed* now and *armed* ten seconds later — ⛔ never a `const` class-identity seal, which structurally cannot say that. ⛔ No fourth guard point, ⛔ no parallel suppression mechanism. ⚠️ **Movement is not restricted anywhere on this path** — moving *cancels*, which is a different rule, and the two are not conflated.

### ⚖️ RULING — TASK-748's **test 12, the SOURCE SCAN**: **ACCEPT.**
⭐ **It is `SC-§37`'s principle applied to a PROHIBITION, and that is exactly the case the law was written for:** a headless suite has **no other way to watch a call that must never happen**, and this gate's own item (3) demands both traps *asserted*, not merely absent. The instrument is sound on every axis I checked:
- **Self-checking, and it fails toward ERROR:** a missing file, an unreadable file, or ≠1 of either sentinel ⇒ `AddError` + `return false` (`SiegeRecallTest.cpp:128-143`, `:859-878`) — ⛔ it can never degrade into a quiet pass.
- **Positive controls on every claim** (`ResetHero` ≥2 elsewhere; `GetEffectiveMaxHP` ≥1; `GetPositionalContext` for the `KBD` pair) ⇒ ⛔ it cannot pass on a typo or a renamed token.
- **Region-bounded**, so it makes a claim about the recall path and not about the file.

⚠️ **Two conditions recorded with the acceptance (W-7):** (a) it reads from `FPaths::ProjectDir()`, so it is **dev-tree bound** — in any environment without `Source/` on disk it **FAILS rather than skips**, which is the correct failure direction for a guard but means the suite is no longer runnable from a packaged tree (out of scope here; `PKG-§` is explicitly not TASK-754's business); (b) the **sentinel comments are now load-bearing source text** and the region must not acquire the tokens `ResetHero` or a bare `MaxHP` **even in prose** — which the sentinel block itself says. **Tests 1–11 stand independently if this instrument is ever withdrawn.**

---

## 5. ⛔⛔ `Escape` IS NOT CONSUMED — ANYWHERE. ✅ HELD.

Grepped `HeroCharacter.{h,cpp}` · `SiegeGhostPawn.{h,cpp}` · `SiegeGameMode.{h,cpp}` · `SiegeMapMark*.{h,cpp}` · `WarMapWidget.{h,cpp}` for `EKeys::Escape` · `NativeOnKeyDown` · `NativeOnPreviewKeyDown` · `FReply`:
- **`EKeys::Escape`: 0 hits. `NativeOnKeyDown`: 0. `NativeOnPreviewKeyDown`: 0.**
- The **only** `FReply::Handled()` returns in the batch are the seven in `WarMapWidget.cpp`, and every one is on a **mouse** path (`NativeOnMouseButtonDown` / `NativeOnMouseWheel`) — ⛔ no key path, ⛔ no viewport intercept.
- The channel adds **no key handler of any kind**; cancel is `B` or movement, and that is the complete list. **T12(d)** asserts zero occurrences of `NativeOnKeyDown` / `NativeOnPreviewKeyDown` / `FReply` / `SetInputMode` / `bShowMouseCursor` across **both** hero files.
- ⭐ **`IMC_Hero`'s `Escape` row survived the append**: TASK-747's pre-mutation key list carries `Escape` at index 10, the first **26 keys are identical in array order** after the append, and the append landed at index **26**.

✅ **`AS-§6` A-2 is intact.** The shipped cancel routes (placement, spell targeting, group-pick) keep firing byte-identically while a channel runs.

---

## 6. ⛔ THE CURSOR / POSSESSION LADDER (`HELP-§5`, `GHOST-§4`). ✅ HELD.

- ⛔ **Zero `SetInputMode` and zero `bShowMouseCursor` writes in `SiegeGameMode.{h,cpp}` and in `SiegeGhostPawn.{h,cpp}`.** The game mode's only posture action is calling `ASiegePlayerController::HandleGhostPossessionChanged()` (`:859`, `:934`).
- `HandleGhostPossessionChanged()` (`SiegePlayerController.cpp:1362-1392`) does exactly two things: `ClearUICursorHold()` then **`ApplyCursorInputState()` — the ONE owner.** ⛔ Nothing else.
- The only `SetInputMode`/`bShowMouseCursor` outside `ApplyCursorInputState` in the controller is the **pre-existing `HandleMatchEnd` end-screen posture** (`:1731-1739`), which `HELP-§5` already names as the shipped exception. ⛔ **Not touched by this batch.**
- ⭐ **The ghost adds ⛔ no term to the owner ladder and re-orders nothing.** Placement · Alt-held `IA_UICursor` · end screen · targeting · group-pick · war map · assistant console · help overlay keep their exact shipped precedence.
- ⭐ **Ordering measured rather than assumed:** `FinishRestartPlayer` calls `Possess()` (binding the **controller's** `HandleHeroDied`) **before** `SetPlayerDefaults()` (binding the **mode's**). Delegates fire in binding order ⇒ the controller's handler runs first and has already cancelled placement/targeting/group-pick — each through its own `ApplyCursorInputState()` — before the ghost is spawned. ⇒ **the ghost never possesses under a live cursor mode.**
- `ClearUICursorHold()` on the swap mirrors the shipped `HandleMatchEnd` idiom for the identical reason (a swallowed *release* would latch `bUICursorHeld` and unbalance the counter-based `SetIgnoreLookInput` ⇒ **a ghost that cannot look around for 180 s**). The programmer's own caveat is honest — the release *very probably* still arrives — and ⭐ *"very probably"* is exactly what `GHOST-§4` refuses to stake three minutes on. Cost of the belt: one re-press of Left Alt. `ClearUICursorHold` is itself guarded, so the counter cannot unbalance. ✅ **Correct call.**
- ⛔ **POSSESS FIRST, DESTROY SECOND** in `RetireGhostFor` (`:911-926`), with `RetireGhost()`'s own possessed-warning checking it from the other side.

---

## 7. `SHIP-§9c` ON EVERY NEW TEST — can each claim actually FAIL? ✅ YES.

Spot-audited the two claims the gate names as most likely to be written un-failably, plus the two new instruments:

| claim | why it can go red |
|---|---|
| ⭐ **`M-1` no-renumber** (`SiegeMapMarkTest.cpp` #3) | after deleting 2 of {1,2,3} the survivors must be **{1, 3}**, ⛔ not {1,2} — **and each survivor must still denote the ground it was drawn on.** ⚖️ A renumbering store **passes a numbers-only check** and fails the **pairing** check; the pairing is what makes it real. `FindMark(2)` must answer nothing. Backed at the implementation by three mechanisms — **there is no counter to renumber from** (`FindLowestFreeNumber` derives it), `RemoveAt` shifts indices and touches no `Number`, and there is **no compaction pass anywhere in the file** |
| ⭐ **ghost is not an `ITeamAgent`** (`SiegeGhostPawnTest.cpp` #1) | asserted **at the type** in **two lanes** with hero/unit/tower positive self-checks ⇒ ⛔ cannot pass vacuously |
| `SymbolIsCircleUnderscoreNumber` | uses **`TestEqualSensitive`** — correct, and `SC-§13` is why: `TestEqual` on two `FString`s compares **case-insensitively**, so a byte claim made with it is vacuous |
| `MarkRingColorIsLegibleAtBothRampEnds` (#34) | ⭐⭐ **`SC-§37`'s founding instance, and it does measure both ends.** Ramp ends read **through the shipped `BrightnessToRampColor` seam**, not transcribed; reproduces `WM-§8c`'s recorded 14.74:1 span and 3.84:1 flat-colour ceiling; the constant lands at **3.81:1 dark / 3.85:1 light**, within ~1 % of the ceiling on **both** ends. ⇒ an sRGB "correction" raises its luminance to ~0.60 and collapses the light end to ~1.3:1 ⇒ **the invisible colour-space error becomes a RED TEST.** ✅ `WM-§8e` is now defended by a test instead of a comment. ⚠️ The honest limit is stated in-file: contrast + channel distance are **necessary, ⛔ not sufficient**, this is ⛔ not a colour-blind simulation, and **Jonathan's eye is the acceptance** |
| `MarkPublicationRefusesBadInput` (746) | its two refusal logs are `AddExpectedMessagePlain(..., Occurrences 1)` ⇒ ⭐ **they MUST fire** — a refusal that stopped logging is a silent failure and this test goes red for it |
| the four collapse tests (746) | `Occurrences -1` on the shipped roster-collapse Warning — correct: an unexpected Warning fails a UE automation test, and these four exercise the collapse deliberately. **The split between `-1` and `1` is right** |
| ⭐ **758's +2** | see §11 |

⚠️ **Named honestly by the authors and confirmed by me — what the suite CANNOT see (`SC-§32`):** `USiegeMapMarkSubsystem` is a `ULocalPlayerSubsystem`, so a `NewObject<UWarMapWidget>` can never reach one ⇒ **no test in 745 adds, deletes or resizes a real mark, and none pretends to.** The live `NativeOnMouseButtonDown` / `NativeOnMouseWheel` dispatch needs a realized Slate widget. 745's test 35's `M-4` leg **declares itself weak in its own comment** (headlessly the count is 0 either way) — a tripwire, ⛔ not proof. ⇒ **all of it lands on the PIE row and Jonathan's eye.** ✅ Correctly disclosed rather than oversold.

---

## 7a. ⭐⭐ TASK-750's THREE SCOPE ADDITIONS — **VERIFIED AS WIRINGS**, ⛔ not fence breaches (board item 7a). ⛔ NOT re-litigated.

### (a) `OnHeroRecallArrived` — ⭐ THE BEST FIND OF THE BATCH, and `SC-§36`'s founding instance.
`HeroCharacter.cpp:1333` shipped the warning *"The teleport-home owner must bind this delegate **(TASK-750)**"* and **nobody had honoured it** ⇒ ⛔⛔ **a completed 10-second recall would have teleported and healed NOTHING**, and it would have failed **SILENTLY**, because the `IsBound()` guard degrades correctly into doing nothing. ⭐ *The better the guard, the quieter the omission.*

| the three hardest checks | result |
|---|---|
| ⛔ **NO fence breach** | the bind is at `SiegeGameMode.cpp:466`, handler `:470` — **TASK-750's own sole-owned file.** ⛔ Zero collision with TASK-748's `HeroCharacter.{h,cpp}` |
| ⭐⭐ **it must ⛔ NOT call `ResetHero()`** | ✅ **IT DOES NOT.** Read line by line, `:470-507`. Teleport + control rotation + one log. ⛔ Nothing else. ⇒ ⛔ no double-applied upgrade stacks, ⛔ no re-armed War Banner aura on a live hero |
| ⭐ **it must route through `GetHeroStartTransform`** | ✅ `:493` — **the same resolver the 180 s respawn uses at `:1012`.** ⇒ "back at the castle" is decided in **exactly one place**, and `handoffs/TASK-569-buildmaster.md` row (n) — *"hero spawns OUTSIDE the keep"* — cannot be re-introduced by a second hand-typed destination |
| the binding **EXISTS** (`SC-§36` ¶4) | ✅ `UFUNCTION()` at `SiegeGameMode.h:402-403` (required for `AddUniqueDynamic`), bound in `SetPlayerDefaults` — **the same site and the same `AddUniqueDynamic` idiom as `OnHeroDied`**, which runs for **every** pawn the mode hands a player, including a restart fallback. Repeated restarts stay idempotent |
| null-safety | `IsValid(RecallingHero)` guard first; `GetHeroStartTransform`'s only use of the controller is `FindPlayerStart(Player)`, which is engine-null-safe (see **N-1**) |

### (b) `USiegeMapMarkSubsystem::ClearMarks()` called from `PlayAgain()` — ✅ **A CALL ONLY.**
`SiegeGameMode.cpp:1500-1514`, **step 6b**, using the ordinary local-player route verbatim from TASK-744's handoff (⛔ no custom getter). Null-safe at every hop; a remote client's server-side PC has no `ULocalPlayer` and is skipped — **correct**, because marks are per-player (`M-2`) and each machine clears its own. ✅ **TASK-744's four files are unmodified** (its API is included, ⛔ not written; the pinned registry landed character-for-character and 745/746 both re-verified it at the source). ⇒ `MARK-§ M-4` is now **wired**, and last match's numbered circles can no longer survive into the next match still nameable to the commander.

### (c) The respawn-side input-context check — ✅ **the reverse trap genuinely does not exist.** See §3; I re-measured all three legs myself rather than accepting the report.
⚠️ **And TASK-750 labelled its own argument as not a substitute for the PIE row. That line is HELD — see §9.**

---

## 8. ⭐ THE CIRCLES (744 + 745 + 746) — every named property verified.

| property | result |
|---|---|
| **hole-preserving numbering, ⛔ never renumber** | ✅ **three mechanisms, and the third survives a tidy-up:** ⛔ **no counter exists to renumber from** (`FindLowestFreeNumber` scans `FirstMarkNumber..MaxMapMarks` for the first free number, `SiegeMapMarkSubsystem.cpp:134-155`) · `RemoveMark` removes **by number** via `IndexOfByPredicate` + `RemoveAt` and writes nothing else, with ⛔ no compaction pass in the file · ⭐ **the AIRLOCK REASON is written at the allocator**: a symbol already composed and sitting **unsent** in the player's input box would otherwise silently start denoting **different ground** — an order already given, quietly redirected |
| cap **9** | ✅ `MaxMapMarks = 9`, `EditDefaultsOnly` (`SiegeMapMarkSubsystem.h:159-160`), with all three of `M-5`'s reasons written beside it. ⭐ **ONE refusal gate** — the cap **is** `FindLowestFreeNumber() == INDEX_NONE` and nothing else; a second `Num() >= Max` check could disagree with it, and two rules for one question is how a cap ships off-by-one. Refusal returns `false` and ⛔ **leaves `OutMark` untouched** |
| symbol **`circle_N`**, ⛔ not a bare digit | ✅ `inline FString FSiegeMapMark::MakeSymbol` (`SiegeMapMark.h:116-124`) — `circle_%d` for `N >= FirstMarkNumber`, ⛔ **the EMPTY string below it, never `circle_0`.** Zone A ships `COUNT = 1 to 30`, so a bare digit would collide with a count token. The upper bound is deliberately **not** duplicated here (the cap lives on the tunable; a second copy would drift) |
| ⭐ **`ZoneA.MeasuredCharCount` still 5658** | ✅ §1 |
| ⚠️ **Zone-C overrun at the FIRST mark** | ✅ **measured and DECLARED, ⛔ not absorbed:** head `108 + 10M` vs a `627 − 10M` roster budget against a 621-char 13-kind block ⇒ collapse at **M ≥ 1**. At the cap of 9: head 198, roster budget 537, **90 chars of roster detail gone.** ⭐ **Collapse proved GRACEFUL:** ⛔ no key dropped · the player's sentence **byte-identical** · **printed rows ∪ `other_kinds:` = all 13 kinds** (a collapse costs **COUNTS**, never **EXISTENCE**) · `GetUnitKinds()` untrimmed so the grammar still admits every kind · **all 9 marks survive** · the shipped Warning still fires. ⛔ `ZoneBCharReserve` **NOT** taken |
| ⚖️ **overrun REPORTED (`AddInfo`), ⛔ not asserted** (746 D7) | ✅ **CORRECT, and I endorse it.** The file's own recorded law says so verbatim: pinning it would make **TASK-528's funded repair FAIL THIS FILE FOR SUCCEEDING.** ⚖️ *A gate that punishes the repair for working is a defect, not rigour.* Only the **10-chars-per-mark invariant** is asserted, which the lever cannot move |
| ⭐ **745's contrast test measures BOTH ramp ends** | ✅ §7 — `SC-§37`'s founding instance |
| ⛔ **markers outrank marks in hit AND paint order** | ✅ **verified at the source, and the two orders are the same order.** Hit: markers tested first (`WarMapWidget.cpp:2652`), the mark lane runs **only** on `HitIndex == INDEX_NONE` (`:2654`). Paint: `Elevation(+1) < MarkRing(+2) < PoiIcon(+3) < Ally(+4) < Enemy(+5) < MarkNumber(+6) < Marker(+7) < Label(+8)` — every shipped layer moved up in order. ⇒ ⛔ **a big circle over `own_castle` can never make that marker permanently unclickable**, and what you see and what you click cannot disagree |
| the wheel separation | ✅ **confirmed at the source, ⛔ not assumed.** Controller = **POLL** (`WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)`, `ApplyGroupPickWheel`, gated to `GroupPickStage != None`). Map = **SLATE EVENT** on a focused widget (`NativeOnMouseWheel`). ⛔ Not one line of the controller's path touched; when the map is closed the widget is `Collapsed` and never receives the event. **Exactly two consumers; ⛔ no third.** The widget's tunables are separately named and **widget-space** — the controller's 100/200/5000 uu appear **nowhere** in the file |
| `NativePaint` (⛔ not `NativeOnPaint`) | ✅ `WarMapWidget.cpp:2269` — the UE 5.8 virtual, spelled correctly. ⛔ `NativeOnPaint`: 0 hits |
| the airlock | ✅ **the map's entire outbound surface is still `FOnWarMapPlacePicked(FName)`**, and the only value that can ride it for a mark is `MakeMarkPickSymbol(N)`. `OnPlacePicked.Broadcast` → the **shipped, untouched** `HandleWarMapPlacePicked` → `AppendToInput` → `ComposeAppendedInput`, and **the player presses Enter himself.** ⛔ No `Capture()`, ⛔ no `EnsureSnapshot()`, ⛔ the mark lane never reads the snapshot at all. ⭐ **`WM-§8d`'s elevation firewall is honoured structurally** — the world XY comes from the projection pair's exact inverse and an `FVector2D` **has no Z**, so the 1,000-uu-clamped display buffer this same class owns is **unreachable** from a mark |
| `M-6` region fence | ✅ §1. Test counts `"circle_1"` occurring **EXACTLY ONCE** in the grammar — ⭐ a `Contains` would pass either way; **a count** is what makes *"not also a zone"* failable |
| `M-2` structural | ✅ `ULocalPlayer::GetSubsystemFromController<USiegeMapMarkSubsystem>(OwningController)` returns **null for a controller with no `ULocalPlayer`** — i.e. for the bot — so `Capture(World, ETeamId::Red)` **cannot** publish the human's private notation into the bot's prompt. ⛔ No code has to remember not to |
| GC / UHT safety | ✅ `FSiegeMapMark` holds ⛔ no `UObject` pointer, so `TArray<FSiegeMapMark> Marks` needs no `UPROPERTY`; the hazard of adding one is written at both ends. ⛔ **No `UFUNCTION` on the subsystem** — deliberate and correct: UHT would **reject** every reflected form (a plain struct cannot be a reflected parameter; `const TArray<...>&` is not a legal reflected return). ⚠️ A well-meaning "expose to Blueprint" edit does not fail review, it **fails UHT at TASK-754** — recorded in the header so nobody re-discovers it |
| `SanitizeRadius` NaN guard | ✅ ⭐ **not decorative.** `FMath::Clamp` is `Max(Min(X, Max), Min)` and every comparison against NaN is false ⇒ an unguarded implementation would clamp a NaN **UP TO THE MAXIMUM** — a circle that swallows the arena, then travelling to a Slate paint call **and** to a world-space resolution in the snapshot. The explicit `IsFinite` guard returns the **minimum**. On a mis-retune where Min > Max the minimum wins, which is the right failure direction |

### ⚖️ RULING — TASK-746's **D1** (the ghost identified by **possession**, ⛔ not `Cast<ASiegeGhostPawn>`): **ACCEPT**, with **W-8**.
The gate is `(DrivenPawn != nullptr) && (Cast<AHeroCharacter>(DrivenPawn) == nullptr)`, and it is reachable **only** through `ChooseHeroAnchorSource(bHeroExists, bHeroIsDead, bGhostAvailable)`, which returns `Ghost` **only when the hero exists AND `IsDead()`** (`SiegeAssistantSnapshot.cpp:979-1010`). In normal play the controller possesses the hero, so the branch is unreachable; in the shipped death flow the driven pawn **is** the ghost. Behaviour is **identical** to the tight form today, and the reasons given are sound (`ASiegeGhostPawn` is TASK-749's sole-owned class and is **not in TASK-746's `names:` block** — importing it would have been the fence breach this batch was decomposed to avoid). ⭐ **The `None` row is the one that matters and it is right: dead + no ghost ⇒ the symbol is NOT PUBLISHED — ⛔ never the corpse, ⛔ never a zero vector that reads as the map origin.** ⇒ **No change required for this gate.** **W-8** records the one-line tightening (`Cast<ASiegeGhostPawn>(DrivenPawn) != nullptr` + the include), now cheap because 749's class has landed, as a **manager-boarded follow-up, ⛔ not a blocker.**

**D2 / D3 — flagged, ⛔ not fixed: ✅ CORRECT RESTRAINT.** `G-8` ruled on **what the symbol resolves to**, ⛔ not on what Zone C's `hero:` key reports (`down`) and ⛔ not on `ReferenceLocation` (still the corpse, which drives `nearest_mine` and both castle resolves). Changing either would move Zone C's bytes / silently move `nearest_mine` while the player is dead — **neither is TASK-746's to re-rule.** ⛔ No prediction of any kind is attached (`AS-§12f`). ⇒ **W-13**, manager rows.

**745's D-1 / D-2 / D-3 — all three ACCEPTED.** D-1's `WR-§9` outcome-1 amendment **has already landed in `CONVENTIONS.md:3436-3440`** ⇒ ⛔ **no manager action is owed on it**, and the retired empty-click arm is a **reachable degrade path** (no local player ⇒ no subsystem; or a click in the letterbox), so the `W691-3` two-arm discriminator stays auditable rather than becoming debris. ⛔ **The RIGHT button still does nothing on empty map** — half the old rule stands verbatim, and right-click is handled **before** the shipped non-left absorb precisely to keep that byte-intact. D-2's one named px↔uu crossing is right on its own merits (a pixel radius would make `circle_1` denote **different ground** on a different monitor — `M-1`'s failure arriving through geometry). D-3's absorb-on-miss changes ⛔ **no state anywhere** and adds ⛔ no third consumer — it is the same one consumer declining to act, and the alternative would make the map's **wheel** behave differently from the map's **clicks** on the same pixels.

---

## 9. ⛔⛔ THE PIE ROW — UNDISCHARGED. CARRIED AS A **COMMIT GATE**, ⛔ NOT WAIVED.

**Stated explicitly, as this gate's item (9) requires: a PASS on the diffs is ⛔ NOT a PASS on runtime.**

`GHOST-§4` / `SC-§35` ruling 5 binds **by its own terms**, because this batch **spawns a new actor class into the world**. TASK-750's dispatch withheld editor/MCP access; it **could not run PIE and did not fake it** — the correct behaviour. TASK-756 could not meet its play-distance render for the same reason and **said so plainly rather than glossing it**.

⛔⛔ **IT MAY NOT BE WAIVED ON A CLEAN COMPILE, AND IT MAY NOT BE DISCHARGED BY REASONING ABOUT THE CODE — HOWEVER GOOD THE STRUCTURAL ARGUMENT.** §3's `RemoveMappingContext`-count invariant is the strongest structural argument in this batch and **TASK-750 labelled it as not a substitute itself.** ⚖️ *A structural argument that the input context survives the swap is evidence; it is ⛔ not an observation.* **That line is held here.**

**ONE PIE session with a MESSAGE-LOG READ, and it must observe — at minimum:**
1. the player **actually has working input on the ghost** (move, look, orders, war map, assistant console);
2. the camera hands off **both ways** (hero → ghost → hero);
3. the ghost **walks the ground** rather than falling through it;
4. ⭐ **folded in from TASK-756 §6:** look at the ghost **from the play camera at play distance** and confirm it does **not read as a living hero**;
5. ⭐ **folded in from TASK-757:** confirm `NS_RecallChannel` **actually renders** (see W-5);
6. the war map's live `NativeOnMouseButtonDown` / `NativeOnMouseWheel` dispatch, which no headless test can reach.

---

## 10. ⛔ THE ONE SUITE TOTAL — **COMPUTED HERE, ⛔ NOT TAKEN FROM ANY HANDOFF.**

⚠️ **`handoffs/TASK-742-buildmaster.md` DOES NOT EXIST — TASK-742 has not landed.** So the gate's instruction to read the baseline from it cannot be followed literally. I therefore computed the number from the **HEAD baseline + per-file measurement**, and reconciled it against the ladder wave's **own audited figures** in `qa/TASK-741.md` §8 + `handoffs/TASK-760-programmer.md`.

**Measured by me, now, on disk:** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` across `Source/GitClaudeUnrealTest/Siegebound/Tests/` = **248** across **20 files**. (All 248 are `SIMPLE`; there are ⛔ no `COMPLEX`/`CUSTOM` frames, so the figure is directly comparable to TASK-744's HEAD measurement.)

| lane | file | measured | delta |
|---|---|---|---|
| **HEAD baseline** | `git grep … HEAD` (TASK-744, matches the board) | — | **171** |
| 744 | `SiegeMapMarkTest.cpp` **(NEW)** | 9 | **+9** |
| 745 | `SiegeWarMapTest.cpp` 27 → **35** | 35 | **+8** |
| 746 | `SiegeAssistantSelectionTest.cpp` 28 → **38** | 38 | **+10** |
| 746 | `SiegeAssistantZoneATest.cpp` 5 → **5** (one test extended in place) | 5 | **+0** |
| 748 | `SiegeRecallTest.cpp` **(NEW)** | 12 | **+12** |
| 749 | `SiegeGhostPawnTest.cpp` **(NEW)** | — | **+8** |
| 758 | `SiegeGhostPawnTest.cpp` 8 → **10** | 10 | **+2** |
| 747 · 757 · 756 | asset-only, declared ZERO | — | **+0** |
| | | **BATCH TOTAL** | **+58** |

**Foreign, ⛔ NOT this gate's — the parallel ladder wave, reconciled so it cannot be misread:**

| | source | delta |
|---|---|---|
| ladder batch | `qa/TASK-741.md` §8 — audited: `SiegeLadderClimbTest.cpp` NEW 13 + `SiegeClimbableTowerTest.cpp` 5→10 | **+18** |
| TASK-760 | `handoffs/TASK-760-programmer.md` — declared +1, `SiegeLadderClimbTest.cpp` 13 → **14** (measured 14 ✅) | **+1** |
| | | **+19** |

### ⭐⭐ **171 + 58 + 19 = 248. On-disk measured = 248. ⇒ EXACT RECONCILIATION, ZERO RESIDUAL.**

> ### 🔧 **THE ONE NUMBER TASK-754 ASSERTS: `248`.**

⚠️ **THREE RIDERS TASK-754 MUST READ BEFORE IT STOPS ON ANYTHING:**
1. ⛔ **`qa/TASK-741.md` pinned `SiegeLadderClimbTest.cpp == 13` as a STOP condition for TASK-742. It now reads 14. That is TASK-760's DECLARED +1 — ⛔ NOT a regression.** Reconcile per file before stopping.
2. If **TASK-760 has not landed** in the tree that is compiled, the number is **247**. If TASK-742 or any other task lands further tests first, **subtract what it recorded and re-derive** — ⛔ do **not** re-attribute this batch's own files as foreign (`SiegeGhostPawnTest.cpp` and `SiegeMapMarkTest.cpp` are **THIS batch's** and were already on disk during the ladder wave, which is why the ladder's on-disk total read 206/247 against a batch figure of 189).
3. ⛔ **TASK-742 adds no tests** (compile + commit), so this batch's figure does not depend on it landing first — only the **compile order** does.

---

## Findings

### BLOCKERS — **NONE.**

### WARN

- **[WARN] W-1 — ⭐⭐ THE MOST CONSEQUENTIAL FINDING IN THIS REPORT. `SiegeGhostPawn.h:357-427` + `SiegeGameMode.h:479-480` + TASK-756 §5 — as this batch will ship at TASK-754, the ghost is INVISIBLE *and* IMMOBILE.**
  `GhostPawnClassAsset` ships **unset** ⇒ the raw C++ `ASiegeGhostPawn` is the spawned ghost; `BP_SiegeGhostPawn` **cannot exist until after TASK-754's compile** (measured by TASK-756: `search_subclasses(APawn,"Ghost") == []`, with a live anti-vacuity self-check on `"Hero"` proving the negative is real). ⇒ **all SEVEN designer slots are null**: `GhostMappingContext`, `MoveAction`, `LookAction`, `MouseLookAction`, `GhostMesh`, `GhostMaterial`, `GhostIdleAnimation` — none is defaulted in the constructor (verified line by line, `SiegeGhostPawn.cpp:40-172`).
  **Consequences, measured rather than feared:** the player **keeps** `G-3`'s powers (orders, war map, assistant console) because those are **controller-bound** and `IMC_Hero` is **never removed** (§3) — ✅ the 180-second input-dead catastrophe does **NOT** occur. But `MoveAction`/`LookAction`/`MouseLookAction` are bound on the **ghost's own** input component and are null ⇒ ⛔ **the ghost cannot move and cannot look**, and `GhostMesh` is null ⇒ ⛔ **the ghost is invisible, so `G-4` — Jonathan's explicit *"The enemy should be able to see this ghost as well"* — is MATERIALLY UNMET in the commit.**
  ⚖️ **Why this is a WARN and ⛔ not a BLOCKER:** the code is defensively correct, null-safe, **warns loudly at every unset slot rather than shipping silent invisibility**, and cannot crash; the gap is **declared** by three tasks; and the fix is an **editor** action that is **structurally impossible before the compile this gate exists to authorise.** Blocking here would deadlock the pipeline and change no code.
  ⇒ ⛔ **TASK-754 must record this in its handoff, and TASK-755's item ⑤ (*"fly the ghost"*) is ⛔ NOT PERFORMABLE until it is closed.**
  ⚠️⚠️ **AND A SPEC GAP THE MANAGER MUST CLOSE WHEN RE-BOARDING IT: the follow-up needs SEVEN slots, ⛔ not three.** TASK-756's board spec item (5) covers only `GhostMesh` / `GhostMaterial` / `GhostIdleAnimation`, and item (6) **explicitly declares `MoveAction`/`MouseLookAction` NOT ITS SCOPE** — while TASK-749's handoff requires `GhostMappingContext = /Game/Input/IMC_Hero` (⚠️ **a ghost-only context would silently drop every one of `G-3`'s powers**) plus the three input actions. **A re-board that copies item (5) verbatim ships a still-immobile ghost.**
- **[WARN] W-2 — the PIE row is undischarged.** §9. Carried to TASK-754 as a **non-waivable commit gate**.
- **[WARN] W-3 — ⚠️ `§25b` FIRED AGAIN: `Content/Input/IMC_Hero.uasset`'s INDEX ENTRY STILL HOLDS THE PRE-APPEND BLOB.** The editor's SCC auto-staged the two new assets by itself; `IMC_Hero` shows ` M` with the index at `9ba4aeb0…46ab` (14,099 B) against a worktree of `88f7f6e8…32fa1` (14,552 B). ⛔ **A commit as-found ships `B` UNBOUND while looking fully staged** — the exact failure TASK-709 caught last time. **Re-`git add` all three paths and verify each by oid-vs-worktree-sha256, ⛔ NEVER by byte size** (a size check "notices" the change while certifying the wrong blob, which is precisely why `§25b` bans it).
- **[WARN] W-4 — `HeroCharacter.h:899-900`: `RecallChannelEffect` is a HARD `TObjectPtr<UNiagaraSystem>` shipped UNSET ⇒ the `R-3` tell is INERT.** ⛔ It is **not** a composed soft path, so creating the asset at a conventional path does **not** wire it. The one edit: set `RecallChannelEffect = /Game/VFX/NS_RecallChannel` on the CDO of **`/Game/Blueprints/BP_HeroCharacter`** (the real pawn — `SiegeGameMode.cpp:47` resolves `BP_HeroCharacter_C`; the raw `AHeroCharacter` is only the meshless fallback), then compile + save that one Blueprint. Null-safe until then, ⛔ never a crash.
- **[WARN] W-5 — ⚠️ NEW ENGINE FINDING, RECORDED: A NIAGARA SYSTEM DUPLICATED VIA MCP / `EditorAssetLibrary` IS *SILENTLY INERT*.** It loads, reports the correct class, resolves the correct dependencies, spawns a component that reports **no error** — and **renders nothing**. Proven by a one-frame A/B (duplicate blank, donor burning); repaired by opening the duplicate once in the Niagara editor. ⚠️⚠️ **The `.uasset` bytes are UNCHANGED by the repair (sha `a082d797…` before and after) ⇒ the compiled data is DDC-side, not package-side** ⇒ ⛔ **TASK-754 must CONFIRM `NS_RecallChannel` actually renders in PIE on its own machine.** This is a **declared uncertainty, ⛔ not a claim that it is fine.** 📌 **Side finding for a separate check: `/Game/VFX/NS_CastleDebris` (TASK-157) was created by the same un-opened-duplicate idiom and may be inert on disk** — worth one PIE look at a castle crumble threshold. ⛔ Not this batch's to fix.
- **[WARN] W-6 — TASK-756's new master `M_HeroSpirit` + two UNWIRED proposal instances already STAGED in git.** See the ruling below. `Content/Materials/Instances/MI_HeroSpirit_Blue.uasset` and `…_Red.uasset` have **zero referencers** and are staged (`A `) by the editor's SCC, not by the artist. ⇒ **TASK-754 must either commit them knowingly or drop both files AND their index entries in one operation, per the manager's ruling.** ⛔ A commit as-found ships two dead assets.
- **[WARN] W-7 — TASK-748's T12 source-scan instrument is dev-tree bound and makes the sentinel comments load-bearing.** Accepted; conditions in §4.
- **[WARN] W-8 — TASK-746 D1's ghost gate is *"driven pawn that is not the hero"*.** Correct today and behaviourally identical; the one-line tightening is pre-written at the site and is now cheap. Manager-boarded follow-up. §8.
- **[WARN] W-9 — ⚠️ TASK-757 HAS NO BOARD ROW** (the artist's D5, and the board's own §22 *"a dispatch is ⛔ not a board entry"*). It was dispatched directly and is covered only inside TASK-747's row. ⇒ **TASK-754's `blocked-by` does not name it, yet `Content/VFX/NS_RecallChannel.uasset` belongs in this commit.** The manager should board it or formally fold it into 747's row and mark it `⛔ do-not-re-dispatch`. ⛔ I did not invent a row (board write is outside my fence).
- **[WARN] W-10 — `GhostPawnClassAsset`'s declared deviation from *"warn once and fall back"*: ACCEPTED.** The resolver distinguishes **UNSET** (expected → `Log` once) from **AUTHORED-BUT-UNRESOLVABLE** (mis-config → `Warning` once); both fall back to the raw C++ class, neither crashes, neither can produce a dead 180 seconds. ⭐ **The right call** — pointing at a nonexistent asset would warn every match forever, which is how a real warning gets trained out of a reader.
- **[WARN] W-11 — `SiegeLadderClimbTest.cpp` now reads 14 against `qa/TASK-741.md`'s pinned STOP value of 13.** TASK-760's declared +1. **TASK-742 must not read it as a ladder regression.** §10 rider 1.
- **[WARN] W-12 — `SiegeGameMode.cpp:911-940`: retiring a ghost at MATCH END when the tracked hero has been destroyed leaves the controller pawnless** until `PlayAgain()`. Very narrow (it needs the hero *actor* destroyed while ghosted), and practically inert because `HandleMatchEnd` has already applied UI-only input — but the retire path logs *"MISSING — the restore path will hand out a fresh pawn"* and **at match end no restore path follows.** Worth one line of thought at the next touch; ⛔ not worth a change now.
- **[WARN] W-13 — TASK-746 D2/D3 flagged-not-fixed** (`hero:` key prints `down` while `hero` resolves; `ReferenceLocation` stays the corpse while dead). ✅ Correct restraint — both are outside `G-8`'s wording. **Manager rows.**
- **[WARN] W-14 — TASK-749's two declared consequences, both ACCEPTED and both for TASK-755:** the ghost carries **no team collision channel** (correct — those are the *combatant body* channels and a ghost is not one), ⇒ **`ACastle::GateBlockerVolume` does not stop it** (`Castle.cpp:606-608` blocks only the enemy **team** channel). ⭐ This is ⛔ **not** the wall-pass `G-1` forbids — the castle's real walls are `WorldStatic` and **do** block it — and `G-3` makes the intrusion mechanically inert. **One line to change if Jonathan rules otherwise.**

### NIT

- **[NIT] N-1 — `SiegeGameMode.cpp:483-493`:** `HandleHeroRecallArrived` passes `RecallingHero->GetController()` straight to `GetHeroStartTransform`, which may be null. Engine-safe (`FindPlayerStart`/`ShouldSpawnAtStartSpot` null-check the controller) and **unreachable on the shipped path** — a recalling hero is always possessed. Recorded only so the next reader does not have to re-derive it.
- **[NIT] N-2 — `EditDefaultsOnly` on a `ULocalPlayerSubsystem` has no editor surface** (a subsystem has no asset to open), so `MaxMapMarks` is in practice retuned in C++. ✅ Correctly flagged by TASK-744 and correctly **not** "fixed" with an unrequested `config=` ini surface.
- **[NIT] N-3 — `MakeSymbol` does not enforce the upper bound.** Deliberate and right: the cap lives on the tunable Jonathan retunes, and a second copy here would drift. `MakeSymbol(10)` returns a well-formed string naming no mark, which the store answers with "no such mark".
- **[NIT] N-4 — designed outcome for the playtest sheet: you cannot place a circle *inside* an existing circle** — that click **names** the outer one. Delete it, or place elsewhere. ⛔ Not a defect; it falls out of the markers-and-marks precedence that keeps `own_castle` clickable.
- **[NIT] N-5 — TASK-745's mark-vs-order-zone colour row is stated as *"the material's"*, not claimed** — `M_SpellReticle` is a binary asset and the author held no editor. ✅ Honest; the other four distinguishing rows (hollow-vs-filled, the number, world-vs-map space, meaning) are structural and hold regardless.

---

## Rulings requested by the dispatch — the five, stated plainly

| # | item | ruling |
|---|---|---|
| 1 | **748's source-scan test (T12)** | ⭐ **ACCEPT.** `SC-§37`'s principle applied to a **prohibition** — the case the law exists for. Self-checking (a stale probe **errors**, never passes), every claim carries a positive control, region-bounded. Two conditions recorded (W-7). Tests 1–11 stand if it is ever withdrawn. |
| 2 | **749's base class `ACharacter` vs the law's literal `APawn`** | ⭐ **ACCEPT.** `ACharacter` IS-A `APawn`, so every statement the law makes stays literally true; the **binding content** (no `ITeamAgent`) is honoured exactly; **test 4 machine-checks the `APawn` claim at the type.** `UCharacterMovementComponent` requires it and both alternatives are worse. ⛔ No amendment owed. |
| 3 | **756's new master `M_HeroSpirit`** | ⭐ **NO QA OBJECTION — and the escape clause fired on MEASUREMENT, exactly as board item (2) provides for.** All five shipped translucent masters were measured: **four are `MD_DeferredDecal`** (structurally impossible on a mesh) and the fifth, `M_Ghost`, is **`bUsedWithSkeletalMesh = false`** ⇒ ⚠️ **it renders fine in-editor and falls back to the DEFAULT MATERIAL in a PACKAGED BUILD** — the silent-asset-defect class, and this project **ships packaged zips**. The artist **built the `M_Ghost`-parented version FIRST**, so the rejection is **evidence, ⛔ not an argument** (render `TASK-756-B`: flat, solid, reads ALIVE). ✅ The **pinned instance name did ⛔ NOT drift** (`/Game/Materials/MI_Ghost_Translucent` is the shipping asset), the **Custom-HLSL ban is honoured** (25 stock nodes, no Custom node), `bUsedWithSkeletalMesh = true`, and the artist correctly **refused** to flip the flag on TASK-012's shipped `M_Ghost`. ⭐ **Zero new geometry — the ghost wears the hero's own stock UE5 mannequin `SKM_Quinn_Simple`, so `G-1` is satisfied literally at zero cost.** ⚖️ **The SCOPE ruling remains the MANAGER's by the board's own words; QA's finding is that the measurement is sound and the alternative is strictly worse.** |
| 4 | **758's first-in-repo `NewObject<AActor>` in tests** | ⭐ **ACCEPT, and I record the boundary as precedent.** ⛔ **An ordering cannot be observed on a CDO** — a CDO never runs an initialisation sequence — so a transient instance is the **minimum instrument that can fail against the old code**, and a CDO-only assertion would be exactly the *"passes under both orderings"* test the fix exists to prevent. The mechanism is clean: `NewObject<ASiegeGhostPawn>(GetTransientPackageAsObject(), …, RF_Transient)` rooted by `TStrongObjectPtr`, ⛔ **no world, no `SpawnActor`, no PIE, no asset load, no CDO mutation**, and `GetTransientPackageAsObject()` is **already shipped precedent** in `SiegeAccountTest.cpp:142`. The directory's standing "no `CreateWorld`, no `SpawnActor`" claim **still holds** and the file's mechanism note was **amended rather than left stale**. ⇒ **PERMITTED where an ORDERING (not a default) is under test; `UWorld::CreateWorld` and `SpawnActor` remain banned.** |
| 5 | **746's D1 — ghost by possession, ⛔ not `Cast<ASiegeGhostPawn>`** | **ACCEPT** + **W-8**. §8. Gate reachable only when the hero exists **and** `IsDead()`; behaviour identical today; one-line tightening pre-written and now cheap. ⛔ Not a blocker. |

**And the one this gate was told to verify hardest — TASK-758's +2 CAN GENUINELY FAIL. ✅ CONFIRMED.**
⛔ **The trap was avoided exactly as the dispatch demanded:** asserting `GetGhostTeam() == Red` after `InitializeGhost(Red)` **would have PASSED against the defective code** — the *field* was always assigned correctly; what was wrong was **WHEN the work that reads it happened.** The tests instead measure **the team the team-dependent step ACTUALLY OBSERVED**, via `HasAppliedTeamAppearance(OutTeam)`:
- **row (c)** — *"the step observed `Red`"* — ⛔ **goes RED against the old ordering**, where the step ran in `BeginPlay` and observed `Blue` on every Red ghost;
- **row (b)** — `InitializeGhost` **drives** the step — ⛔ red against the old code, where it assigned a field and drove nothing;
- **self-checks:** the declaration default **is** Blue (asserted, because the whole discriminating power depends on Red ≠ default), and a Blue instance records Blue with the two instances **differing** ⇒ the record cannot be a hard-coded constant;
- `FSiegeGhostPawnTeamStepIsRerunnableTest` goes red the day someone adds an *"already applied"* early-out — which **reads like harmless idempotence and would break the deferred-spawn ordering**, since the mesh whose slots are stamped does not exist until `BeginPlay` has assigned it.
⭐ **And the fix itself is structural rather than a comment:** `ApplyGhostTeamAppearance()` is **gated on `bGhostTeamAssigned`** — ⛔ deliberately **not** `GhostTeam == Blue`, which cannot distinguish *"a Blue ghost"* from *"nobody told me yet"*; `ETeamId` has no unset value and **that ambiguity IS the defect** — and it is **called from BOTH `BeginPlay` and `InitializeGhost`**, so whichever runs last drives it and the work is correct under **either** ordering. ✅ TASK-749's four load-bearing properties re-verified after the change: no `ITeamAgent`, constructor **byte-untouched** (`ECC_Pawn` intact), the `IMC_Hero` re-add byte-identical, the three-call API signatures unchanged. ✅ `SC-§36` honoured at the tint seat: the comment **explicitly states it binds no task, schedules nothing and awaits nothing.**

---

## Notes for build-master (TASK-754) — ⛔ WHAT THIS GATE REQUIRES YOU TO VERIFY

**Order matters. (0) and (1) come before anything else.**

0. ⛔⛔ **THE PIE ROW IS A COMMIT GATE, ⛔ NOT A NICE-TO-HAVE, AND IT IS ⛔ NOT WAIVABLE ON A CLEAN COMPILE.** It is **undischarged** (§9). ⛔ **Do ⛔ not commit around it and ⛔ do not discharge it yourself by reasoning about the code** — §3's invariant is the strongest argument available and TASK-750 itself said it is not a substitute. If it cannot be run, ⛔ **SAY SO AND ESCALATE.** Run the six observations in §9.
1. ⛔⛔ **QUIET-MODULE FIRST.** `handoffs/TASK-742-buildmaster.md` **does not exist as of this report** ⇒ **confirm TASK-742 has landed and that ⛔ no other compile is live before you start.** Two waves were authored in parallel against ONE UBT module; a concurrent compile makes both waves' results unattributable.
2. **ONE COMPILE** via the CLAUDE.md `Build.bat` line. ⚠️⚠️ **PARSE THE LOG FOR `Result: Failed` — ⛔ NEVER trust `$LASTEXITCODE`.** ⭐ **Expected clean:** `SiegeMapMark.h`, `SiegeMapMarkSubsystem.{h,cpp}` and `SiegeGhostPawn.{h,cpp}` have **all landed**, so the designed "does not compile yet" cross-task state is **over**. Two compile-time eyes worth having: **(a)** `SpawnSystemAttached` / `NiagaraComponent.h` are **new to this module** (`Niagara` is already a public dependency); **(b)** `SiegeGhostPawn.cpp` includes `HeroCharacter.h` read-only for `GetEffectiveWalkSpeed()` — if TASK-748 had renamed it, that file is the second casualty (it did not).
3. ⛔ **THE SUITE: assert `248`.** §10, **including all three riders** — especially that `SiegeLadderClimbTest.cpp == 14` is **TASK-760's declared +1 and ⛔ not a ladder regression**, against `qa/TASK-741.md`'s pinned 13. ⛔ **A red suite is a STOP, ⛔ not a note.** ⛔ Compute your own; ⛔ a raw tree count is not this batch's figure.
4. ⛔⛔ **`§25b` — RE-`git add` ALL THREE ASSET PATHS AND VERIFY BY OID-vs-WORKTREE-SHA256, ⛔ NEVER BY SIZE (W-3).**
   `Content/Input/IMC_Hero.uasset` → `88f7f6e882e23f03fb154b63e2ce93cfab2531384bbaac0ac2a2be1c01332fa1` (⛔ the index still holds the **pre-append** blob `9ba4aeb0…46ab`) · `Content/Input/Actions/IA_Recall.uasset` → `4982c1585c13c2b751d51a14f5dfeb4b3f24f723abe343270206299780057a32` · `Content/VFX/NS_RecallChannel.uasset` → `a082d7978fdcb4c618e66db1ef3dfd77375941b3c0a7f0c303c45766fdd90cdc`. **A commit as-found ships `B` UNBOUND while looking fully staged.**
5. **Read back and REPORT the `IMC_Hero` state** (task spec item 4): **27 rows**, index 26 = `IA_Recall` / `B` / empty Triggers / empty Modifiers, first 26 keys identical in array order, `Escape` still present. ⛔ If absent, `B` is INERT by design and that is a **REPORTABLE state, ⛔ not a silent pass**.
6. **Confirm `NS_RecallChannel` RENDERS in PIE (W-5)** — the MCP-duplicate-is-silently-inert finding. If blank, the fix is one manual open of the asset in the Niagara editor. ⛔ Do not assume it is fine because the sha matches; **the sha is unchanged by the repair.**
7. ⭐ **RECORD W-1 IN YOUR HANDOFF, AND ⛔ DO NOT LET IT PASS AS DONE:** the commit ships an **invisible, immobile** ghost because `BP_SiegeGhostPawn` cannot exist before your compile. **TASK-755 item ⑤ is not performable until the follow-up lands, and the follow-up needs SEVEN slots, ⛔ not three.**
8. **W-4:** set `RecallChannelEffect = /Game/VFX/NS_RecallChannel` on the **`BP_HeroCharacter`** CDO, then compile + save that one Blueprint — or **report it as owed.** ⛔ Until then the `R-3` tell is inert.
9. **W-6:** `MI_HeroSpirit_Blue` / `_Red` are **staged with zero referencers.** Commit them knowingly **or** drop both files **and** their index entries in one operation, per the manager's ruling on `M_HeroSpirit`.
10. **ONE COMMIT on `main`, ⛔ NO PUSH.** ⚠️ **Check `git log` / `git status` FIRST — Jonathan sometimes commits work himself; if HEAD already carries it, ⛔ do not duplicate, ⛔ do not amend, ⛔ do not rebase.**
11. ⚠️⚠️ **`L_Arena` AND `ABP_Footman` ARE DIRTY IN MEMORY** (TASK-747's preview rig, and someone else's `ABP_Footman`). ⛔ **Do NOT save `L_Arena` and ⛔ do not let a "save all" run.** On disk it is pristine and hash-verified against the `ROT-§2` ledger `9ccd54ef…0e58` with its mtime unchanged; a restart or a discard returns it to the ledger state.
12. ⛔ **OUT OF SCOPE, ⛔ NOT TOUCHED:** TASK-716/700 (the packaged-zip arena adjudication) · the tower sitting · the ladder climb-rate ruling · TASK-735 · `Tools/Packaging/` · TASK-528 (`ZoneBCharReserve`).

## Notes for the manager

- **TASK-757 needs a board row** or a formal fold into TASK-747's, marked `⛔ do-not-re-dispatch` (W-9).
- **`M_HeroSpirit` needs your one-word scope ruling** — QA has no objection; the measurement is sound (ruling 3).
- ⚠️ **`BP_SiegeGhostPawn` must be RE-BOARDED after TASK-754's compile with SEVEN slots** (W-1), ⛔ not the three in TASK-756 item (5).
- **Rows owed to Jonathan, ⛔ none of them decided by any agent:** the **team-tint question** — ⭐ **`AHeroCharacter` has NO body team tint at all**; it conveys team **only** through its overhead health bar and damage numbers, and **`GHOST-§5` denies the ghost `IHealthBarProvider` while `GHOST-§1` denies it damage** ⇒ ⛔ **the ghost loses BOTH of the hero's team channels and blue and red ghosts are currently indistinguishable** (latent today with one human controller; **real at M8**, where `GhostTeam` is already reserved and both players ghost). **Recorded, ⛔ not decided.** · `G-5` (cards while ghosted — shipped as the ruled default NO, one `return true` to flip) · discarding for 1 gold is still allowed while ghosted (flagged, not built) · the ghost is **not dotted on the war map's paid enemy reveal** (`G-4` is satisfied by it being a visible world actor, which is 749's job) · **W-14** (castle gate blockers do not stop the ghost) · **W-13** (746 D2/D3) · **W-8** (746 D1 tightening) · ⚠️ **GDD §3.1's *"back within 5-6 s"* is now FALSE** — ⛔ no agent edits `Docs/GDD.md`; the comment beside `HeroRespawnDelay` was **rewritten in place** to record whose ruling it is, the 36× factor and the consequence. ✅ Correct.
- ⚠️ **`NS_CastleDebris` may be silently inert** (same un-opened-duplicate idiom, W-5). Worth one PIE look; ⛔ not this batch's.
- 📌 **FAB-008 raised** by TASK-757 — the project owns **no** effect built to last a channel (34 systems measured at exact age 10.0; **only three survived**).

---

*Gate closed 2026-09-01 by qa-reviewer. ⛔ No file was edited, no compile run, no engine or MCP touched, no Git command issued, no TASKBOARD spec written — status flip only.*

---

# ⛔⛔ COMPILE FAILED — APPENDED BY BUILD-MASTER (TASK-742 PHASE 1), 2026-09-01

**Status: `qa-failed`. This counts as a QA loop.** ⛔ No code fix was authored by build-master. ⛔ No commit, ⛔ no staging, ⛔ no editor, ⛔ no push. ⛔ The suite could ⛔ NOT be run — there is no binary.

## The `Result:` line, PARSED FROM THE LOG — ⛔ not from the exit code

```
Result: Failed (OtherCompilationError)
Total execution time: 61.18 seconds
```

⚠️⚠️ **THE EXIT CODE LIED AGAIN, EXACTLY AS THE LAW PREDICTS: the process returned `0`.** Anyone trusting `$LASTEXITCODE` here would have proceeded to stage and commit a module that does not build.

**Ruled out, so nobody re-derives it:**
- ⛔ **NOT Smart App Control** — `0x800711C7` occurs **0** times; the build ran **61 s** and did real work, ⛔ not a ~2 s death.
- ⛔ **NOT a Live Coding lock** — the only `mutex` hit in the whole log is the `-waitmutex` **argument echo** on line 2. Zero editor processes, zero UBT processes and port 8000 free were all verified **before** launch. ⛔ No editor reopened.
- ✅ **UHT PASSED.** All eleven `Module.GitClaudeUnrealTest.*.gen.cpp` TUs compiled clean ⇒ **`ASiegeGhostPawn`, `USiegeMapMarkSubsystem` and `UClimbableTowerLadderLink` all cleared reflection code generation.** The failures are ⛔ **plain C++**, ⛔ not reflection.
- ⛔ **NOT the designed cross-task state.** §11's predicted first-compile link errors naming `FSiegeMapMark` / `USiegeMapMarkSubsystem` / `ASiegeGhostPawn` **did not occur** — those files landed and compiled. ⇒ **This is new information.**

## The three diagnostics — 2 root causes, ⛔ BOTH IN THIS BATCH

```
SiegeGhostPawn.cpp(192,46):     error C2248: 'AHeroCharacter::GetEffectiveWalkSpeed': cannot access protected member declared in class 'AHeroCharacter'
SiegeGhostPawnTest.cpp(580,44): error C2248: 'AHeroCharacter::GetEffectiveWalkSpeed': cannot access protected member declared in class 'AHeroCharacter'
SiegeGameMode.cpp(483,15):      error C4458: declaration of 'Owner' hides class member
```
Tally: `C2248` ×2 · `C4458` ×1 · **0 warnings.**

### ⭐ ROOT CAUSE 1 (C2248 ×2) — `GetEffectiveWalkSpeed()` IS `protected` — **TASK-749's lane**

Verified at source, ⛔ not inferred: `HeroCharacter.h:544` opens `protected:` and **no later access specifier intervenes before `:626`**, where `GetEffectiveWalkSpeed()` is declared. `ASiegeGhostPawn` does ⛔ not derive from `AHeroCharacter`, so both call sites are external access to a protected member.

⚠️⚠️ **THIS GATE LOOKED DIRECTLY AT THIS CALL AND CHECKED THE WRONG PROPERTY.** §11(b) reads *"`SiegeGhostPawn.cpp` includes `HeroCharacter.h` read-only for `GetEffectiveWalkSpeed()` — if TASK-748 had renamed it, that file is the second casualty (it did not)."* ⭐ **The gate verified the NAME and never verified the ACCESS SPECIFIER.** The name was indeed never renamed; the member was **always** protected. ⇒ **Recommended standing amendment: a cross-class call must be checked for REACHABILITY, ⛔ not merely for spelling.**

⭐ **The asymmetry that made this easy to miss — recorded as diagnosis for the programmer; ⛔ build-master proposes no patch and rules nothing:** the HP twin `GetEffectiveMaxHP()` (`:620`) sits under the **same** `protected:` block, but TASK-748 only ever calls it **from inside `AHeroCharacter`** (legal), and a **public** wrapper `GetMaxHP()` already exists at `HeroCharacter.h:462`. **There is no equivalent public wrapper for walk speed.** ⚠️ `HeroCharacter.{h,cpp}` is **TASK-748's SOLE-owned file** while `SiegeGhostPawn.{h,cpp}` + its test are **TASK-749's** ⇒ **the remedy crosses a fence and needs a manager/QA ruling on which side moves.** ⛔ Build-master did not choose.

⚠️ **Do not lose what these call sites were FOR.** `SiegeGhostPawn.cpp:184-198` derives the ghost's speed from the hero's rather than duplicating the `500.f` literal, and `SiegeGhostPawnTest.cpp:574-580` test (d) is the **self-check on that derivation** — the assertion that goes red if the hero's speed API is ever removed, renamed or zeroed. **A "fix" that hard-codes the number would pass this compile while silently discarding the property both exist to hold.**

### ⭐ ROOT CAUSE 2 (C4458 ×1) — shadowed `AActor::Owner` — **TASK-750's lane**

`SiegeGameMode.cpp:483` declares `AController* Owner = RecallingHero->GetController();` inside `HandleHeroRecallArrived`. `ASiegeGameMode` inherits `TObjectPtr<AActor> Owner` from `AActor` (`Actor.h:839`); UE builds with **C4458 promoted to an error**. A local rename is sufficient — ⛔ **but that is the programmer's call and the file is TASK-750's SOLE-owned.**

⚠️ **This gate read this exact line and missed the shadow.** **N-1** analysed `SiegeGameMode.cpp:483-493` for the **null-safety** of `RecallingHero->GetController()` and pronounced it engine-safe and unreachable on the shipped path — correct on its own terms, but **the declaration it was quoting does not compile.** ⇒ The line was examined for semantics and never for legality.

⚠️ **Adjacent, ⛔ not asserted:** `Owner` is consumed at `:493` by `GetHeroStartTransform(Owner, ...)`. Whatever rename lands must carry to that call site — the seam **`SC-§36`** was just written to protect.

## ✅ WHAT PASSED — recorded so the WRONG lane is not looped

⭐⭐ **THE LADDER WAVE (`qa/TASK-741.md`: TASK-733b · 734 · 737 · 738 · 760) COMPILED 100 % CLEAN — ZERO diagnostics.** All four of its TUs built without a single error or warning: `ClimbableTower.cpp` `[2/32]`, `SiegeClimbableTowerTest.cpp` `[19/32]`, `SiegeLadderClimbTest.cpp` `[21/32]`, `SummonedUnit.cpp` `[29/32]`. **`UClimbableTowerLadderLink` cleared UHT**, closing §9 item 4 of `qa/TASK-741.md` as pre-cleared-and-confirmed. ⛔ **Do NOT route the ladder programmer back on this.**

Also clean in **this** batch: `SiegeMapMarkSubsystem.cpp`, `SiegeMapMarkTest.cpp`, `SiegeRecallTest.cpp`, `SiegeWarMapTest.cpp`, `SiegeAssistantSnapshot.cpp`, `SiegeAssistantSelectionTest.cpp`, `SiegeAssistantZoneATest.cpp`, `SiegeRespawnLifecycleTest.cpp`, `WarMapWidget.cpp`, `SiegePlayerController.cpp`, `HeroCharacter.cpp`. ⇒ §11(a)'s watch item is **discharged**: `SpawnSystemAttached` / `NiagaraComponent.h` compiled with no complaint.

## ⚠️ THE NEXT COMPILE IS ⛔ NOT GUARANTEED GREEN — 3 STEPS NEVER RAN

**29 of 32 steps executed. The remaining 3 — including the LINK — never ran**, because the compile aborted. ⇒ ⛔ **Link-time errors remain UNOBSERVED, and a second failure after these three are fixed would be NEW information — ⛔ not a regression, and ⛔ not a second loop against the same finding.**

## SUITE — ⛔ NOT RUN, and the static count reconciles EXACTLY at 248

The headless `Automation RunTests` lane needs a built binary, so **the suite could not execute.** The **static** count was still computed independently, per file, ⛔ never as a raw tree total:

**Measured on disk, line-anchored `^\s*IMPLEMENT_[A-Z_]*AUTOMATION_TEST`, 20 files = ⭐ 248.** (The unanchored count is also 248 ⇒ ⛔ no prose/string false positives. All 248 are `SIMPLE`; ⛔ zero `COMPLEX`/`CUSTOM`.)

`171 (HEAD) + 58 (this batch) + 19 (ladder +18 & TASK-760 +1) = 248` ✅ **exact, zero residual.**

**All three riders CHECKED:** ✅ `SiegeLadderClimbTest.cpp` = **14** (TASK-760's declared +1 against 741's pinned 13 — ⛔ **not** a regression) · ✅ `SiegeClimbableTowerTest.cpp` = **10** · ✅ `handoffs/TASK-742-buildmaster.md` confirmed **absent** and ⛔ not sought.

⚠️ **`248` is a count of DECLARATIONS, ⛔ not a green suite.** The executed figure and the pass/fail verdict remain **UNOBSERVED**, so TASK-754's *"a red suite is a STOP"* row is **UNDISCHARGED** — alongside the still-undischarged **PIE row**.

## ROUTING

| lane | verdict |
|---|---|
| **TASK-749** (`SiegeGhostPawn.cpp:192`, `SiegeGhostPawnTest.cpp:580`) | ⛔ **`qa-failed`** — C2248 ×2 |
| **TASK-750** (`SiegeGameMode.cpp:483`) | ⛔ **`qa-failed`** — C4458 |
| **TASK-748** (`HeroCharacter.{h,cpp}`) | compiled clean, ⛔ **but it owns the file any public-accessor remedy would touch** ⇒ **fence ruling required before the fix is written** |
| ladder wave (`qa/TASK-741.md`) | ✅ **clean — ⛔ do not loop** |

⛔ **Build-master authored no fix and ruled no fence.** Log retained at `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\t742\compile.log`.

*Appended 2026-09-01 by build-master. ⛔ No Git command issued, ⛔ no file staged, ⛔ no editor launched, ⛔ no MCP listener started, ⛔ no `Source/` file edited.*

---

# ✅ COMPILE GREEN · ⛔⛔ SUITE RED (3/248) — BUILD-MASTER, TASK-742 PHASE 1 RE-RUN (post TASK-761), 2026-09-01

**Status: `qa-failed` — ⛔ NOT for the compile, which is now clean, but for the SUITE.** ⛔ No commit, ⛔ no staging, ⛔ no push, ⛔ no editor, ⛔ no code fix authored.

## 1. COMPILE — ✅ `Result: Succeeded`, and ⭐ THE LINK IS NOW OBSERVED

```
[18/20] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[19/20] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[20/20] WriteMetadata GitClaudeUnrealTestEditor.target

Result: Succeeded
Total execution time: 20.98 seconds
```

**Zero errors, zero warnings — `grep -ic 'error|warning'` over the whole log returns `0`.** ⛔ Not SAC (`0x800711C7` ×0) · ⛔ not Live Coding (0 hits; machine verified quiesced first). **`UnrealEditor-GitClaudeUnrealTest.dll` relinked and verified on disk 39 s old at check time** ⇒ ⭐ **the previously UNOBSERVED link step ran and PASSED.** That caveat from the first run is discharged; ⛔ nothing about the link remains outstanding.

**TASK-761's four fixes verified at source by build-master, read-only:**
- `HeroCharacter.h:477` — `GetWalkSpeed()` is genuinely **public** (nearest preceding specifier is `:323 public:`; `GetEffectiveWalkSpeed()` remains protected under `:555`). `UFUNCTION(BlueprintPure)`, mirroring `GetMaxHP()` at `:462`. ✅ `HeroCharacter.cpp` untouched — `:775` and `:936` still call the protected form from **inside** the class, which is legal.
- `SiegeGhostPawn.cpp:197` and `Tests/SiegeGhostPawnTest.cpp:583` — both on the public accessor. ⛔ `500.f` **not** hard-coded; the derivation and its self-check both survive.
- `SiegeGameMode.cpp` — **all three** sites renamed to `RecallingController` (`:488` declaration, `:498` `GetHeroStartTransform`, `:504` `Cast<APlayerController>`). ⛔ Zero bare `Owner` locals remain; `SpawnParams.Owner` at `:814` is an unrelated struct field, correctly untouched.

⭐⭐ **BUILD-MASTER'S OWN MISS, RECORDED: my first-run diagnosis named only the `:493` consumer and ⛔ did NOT name `Cast<APlayerController>(Owner)`.** Left as `Owner` that third site would have **resolved to the inherited `AActor::Owner` — null on a game mode — and COMPILED CLEAN while silently skipping `SetControlRotation`.** The compiler would have gone quiet while the behaviour broke. **The all-or-nothing property is now written at `SiegeGameMode.cpp:483-487` in the shipped comment**, which is the right place for it.

## 2. ⛔⛔ SUITE — RED. **248 ran (the asserted number is EXACT), 3 FAILED.**

Headless lane, ⛔ no GUI editor, ⛔ nothing weakened, skipped or filtered:
`UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound" -TestExit="Automation Test Queue Empty" -unattended -nopause -nullrhi -nosplash -NoSound -ReportExportPath=… -abslog=…`

**Filter coverage proved before the run, ⛔ not assumed:** all 248 test names live under `Siegebound.` across 17 namespaces summing to exactly 248 ⇒ ⛔ zero omission, ⛔ zero leakage.

| JSON `index.json` | |
|---|---|
| `succeeded` | **233** |
| `succeededWithWarnings` | **12** |
| **`failed`** | ⛔ **3** |
| `notRun` / `inProcess` | **0 / 0** |
| `len(tests)` | ⭐ **248** |
| `totalDuration` | 5.05 s |

⭐ **THE COUNT ASSERTION HOLDS — 233 + 12 + 3 = 248, and 248 distinct test paths completed.** Static count also **248** (line-anchored and unanchored agree; all `SIMPLE`). **All three riders check out:** `SiegeLadderClimbTest.cpp` = **14** · `SiegeClimbableTowerTest.cpp` = **10** · `handoffs/TASK-742-buildmaster.md` absent. ⚖️ **But the number was never the gate — `a red suite is a STOP` is, and it fires.**

### The three failures — ⛔ ALL in `SiegeAssistantSelectionTest.cpp` (TASK-746's file), 6 assertions, ⭐ ONE root cause

```
Siegebound.Assistant.Selection.MarkIsNeverARegion
  :4666  Expected '"circle_1" appears EXACTLY ONCE in the grammar' to be 1, but it was 0.
Siegebound.Assistant.Selection.MarkSentencesParseEndToEnd
  :5363  Expected 'the grammar admits the intent "guard"'      to be true.
  :5365  Expected '…the intent "ambush"'                        to be true.
  :5367  Expected '…the destination "circle_1"'                 to be true.
  :5382  Expected '…and the destination "circle_2"'             to be true.
Siegebound.Assistant.Selection.MarkSymbolsReachThePlacesLine
  :4374  Expected 'the GBNF grammar admits "circle_1" as a where' to be true.
  :4381  Expected '…and "circle_2"'                              to be true.
```

⭐⭐ **`"guard"` AND `"ambush"` ARE PRE-EXISTING SHIPPED INTENTS WITH ⛔ NOTHING TO DO WITH MARKS.** Their failure is what identifies this as **⛔ not a marks bug**.

### ⭐ ROOT CAUSE — derived at source: the tests assume ONE quoting layer; the shipped builder applies TWO

`USiegeAssistantGrammar::Build` emits every symbol — intents, kinds **and** places alike — through `GbnfJsonString` (`SiegeAssistantGrammar.cpp:512`, `:527`, `:543`):

```cpp
FString GbnfJsonString(const FString& Value)   // SiegeAssistantGrammar.cpp:38-52
{
    /* …JSON-escape \ and " … */
    return GbnfTerminal(FString(TEXT("\"")) + JsonEscaped + TEXT("\""));
}
FString GbnfTerminal(const FString& Chars)     // :15-29 — escapes \ and ", then wraps in quotes
```

For `circle_1` the layers compose to the on-disk grammar text **`"\"circle_1\""`**. The tests search for **`"circle_1"`** (`TEXT("\"circle_1\"")` in C++ source = quote · `circle_1` · quote). In `"\"circle_1\""` every inner quote is **preceded by a backslash**, so that substring ⛔ **never occurs**. ⇒ **All six assertions are unsatisfiable against the shipped emitter**, and the same arithmetic explains `"guard"` and `"ambush"` identically.

⭐ **The double escaping is DELIBERATE and self-documented** at `SiegeAssistantGrammar.cpp:31-37`: *"Two escaping layers, applied in the right order… the nesting has to be correct for the defensive cases."*

⭐ **Corroborated by what PASSED, which is the decisive evidence:** in the *same* tests, `Printed.Contains("circle_1")` (Zone C `places:` line, `:4362-4364`), `FSiegeMapMark::MakeSymbol` equality (`:4345-4355`) and `Grammar.Len() > 0` (`:5360`) **all passed** ⇒ ⭐ **the marks DO reach the snapshot and Zone C, and the grammar IS produced.** Only the six quoted-substring probes fail. And **all 12 `SiegeAssistantGrammarTest.cpp` tests passed** — they assert via rule names (`intent ::= `, `SiegeAssistantGrammarTest.cpp:107`) or **unquoted** substrings (`:601`), ⛔ never the `"symbol"` form. **TASK-746's tests are the first in the repo to assert the quoted form.**

⚠️⚠️ **AND A VACUOUS PASS THIS GATE'S `SHIP-§9c` AUDIT DID NOT CATCH — worth more than the failures.** `MarkSymbolsReachThePlacesLine:4381` asserts **`TestFalse`** that `circle_4` — never drawn — is **absent** from the grammar. It **PASSED**, but under the real escaping it passes **for the wrong reason**: the unescaped form matches nothing, so ⛔ **it would pass even if `circle_4` WERE in the grammar.** ⭐ *The negative control that guards the "a mark he never drew is unsayable" property is currently inert.* ⇒ **Whatever remedy lands must re-arm it, or the safety claim is untested.**

### ⛔ WHAT BUILD-MASTER DOES ⛔ NOT RULE

⛔ **Whether the remedy is the TEST (match the shipped two-layer emission) or the CODE (emit a single layer) is ⛔ NOT build-master's call, and I make no recommendation.** The evidence above says the emitter is deliberate, documented and covered by 12 passing tests while the assertions are new — ⚖️ **but "the older thing is right" is an inference, ⛔ not a measurement**, and a grammar that llama.cpp must actually parse is a runtime question this headless suite cannot answer. **QA + the programmer own it.** ⛔ Build-master edited no `Source/` file.

⚠️ **Fence note:** `SiegeAssistantSelectionTest.cpp` is **TASK-746's**; `SiegeAssistantGrammar.cpp` is ⛔ **not named in TASK-746's `names:` block.** A code-side remedy therefore **crosses a fence** exactly as TASK-761's did, and needs the same explicit authorisation.

## 3. ✅ EVERYTHING ELSE IS GREEN — ⛔ do not re-loop these lanes

**245 of 248 pass.** ⭐ **Every test of both gated batches outside the three above is GREEN**, including all of: `Siegebound.Ghost.*` (10/10) · `Siegebound.Recall.*` (12/12) · `Siegebound.RespawnLifecycle.*` (9/9) · `Siegebound.MapMarks.*` (9/9) · `Siegebound.WarMap.*` (35/35) · `Siegebound.LadderClimb.*` (14/14) · `Siegebound.ClimbableTower.*` (10/10). ⇒ **TASK-749, TASK-750, TASK-748, TASK-744, TASK-745, TASK-758 and the whole ladder wave are compile-clean AND suite-green.** ⛔ **Only TASK-746 is red.**

The **12 `succeededWithWarnings`** are passes carrying expected-message warnings (10 × `ControlsHelp`, `Cloud.GuestSafeDefaults`, `Input.QwertyIsPassThrough`, `WarMap.AppendToInputRefusesAndNeverOpensTheConsole`) — ⛔ **not failures**, and consistent with the deliberate `AddExpectedMessagePlain` instruments this gate endorsed.

## 4. STILL UNDISCHARGED — ⛔ unchanged by a green compile

🚩 **The PIE row (`GHOST-§4` / `SC-§35` ruling 5) remains UNDISCHARGED.** A green compile and a 245/248 suite are ⛔ **not** an observation of runtime, and this gate's own §9 forbids waiving it on a clean compile.

*Appended 2026-09-01 by build-master. ⛔ No Git command issued, ⛔ no file staged, ⛔ no GUI editor launched, ⛔ no MCP listener started, ⛔ no `Source/` file edited, ⛔ no asset saved. `L_Arena.umap` re-verified `9ccd54ef…0e58` after the headless run. Artifacts: `…/scratchpad/t742/compile2.log`, `suite.log`, `report/index.json`.*

---

# ✅✅ GREEN — COMPILE + SUITE. BUILD-MASTER, TASK-742 PHASE 1 (3rd run, post TASK-762), 2026-09-01

**Both gates pass. ⛔ Still NO commit, ⛔ no staging, ⛔ no push, ⛔ no editor, ⛔ no code fix — phase 1 ends here by dispatch.**

## 1. COMPILE — ✅ `Result: Succeeded`

```
[1/4] Compile [x64] SiegeAssistantSelectionTest.cpp
[2/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[3/4] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[4/4] WriteMetadata GitClaudeUnrealTestEditor.target

Result: Succeeded
Total execution time: 5.74 seconds
```

**Zero errors, zero warnings** (`grep -ic 'error|warning'` = `0`). ⛔ Not SAC (`0x800711C7` ×0) · ⛔ not Live Coding (0 hits; machine verified quiesced before launch). **Link ran and passed.**

⭐ **THE BUILD INDEPENDENTLY CORROBORATES THE ONE-FILE SCOPE CLAIM, ⛔ rather than taking it on report:** UBT recompiled **exactly one translation unit** — `SiegeAssistantSelectionTest.cpp`. Had `SiegeAssistantGrammar.cpp` been touched it would necessarily have rebuilt too. ✅ Also confirmed directly: `git status` reports `SiegeAssistantGrammar.cpp` **CLEAN vs HEAD**. ⇒ **The emitter was not touched. The ruling was applied to the TEST.**

## 2. SUITE — ✅✅ **248 / 248, ZERO FAILURES**

| `report3/index.json` | |
|---|---|
| `succeeded` | 236 |
| `succeededWithWarnings` | 12 |
| **`failed`** | ✅ **0** |
| `notRun` / `inProcess` | 0 / 0 |
| `len(tests)` | ⭐ **248** |
| `totalDuration` | 4.68 s |

**Raw-log cross-check, independent of the JSON:** `Result={Success}` ×**248** · `Result={Fail}` ×**0** · `LogAutomationController: Error: Expected` ×**0** · **248 distinct test paths** completed. ⛔ Nothing weakened, skipped, filtered or narrowed.

**The three formerly-red tests, by name:** `MarkIsNeverARegion` ✅ **Success** · `MarkSentencesParseEndToEnd` ✅ **Success** · `MarkSymbolsReachThePlacesLine` ✅ **Success**.

**Every namespace green:** Account 7 · Assistant **64** · Castle 1 · ClimbableTower 10 · Cloud 9 · ControlsHelp 13 · Deck 12 · Ghost 10 · HighGround 9 · Input 7 · LadderClimb 14 · MapMarks 9 · Nav 20 · Recall 12 · RespawnLifecycle 9 · Settings 7 · WarMap 35 — **all `fail=0`**.

**Static count 248** (line-anchored and unanchored agree). **Riders:** `SiegeLadderClimbTest.cpp` **14** ✅ · `SiegeClimbableTowerTest.cpp` **10** ✅ · `SiegeAssistantSelectionTest.cpp` **38** (unchanged — assertions only, ⛔ no new frame) ✅ · `handoffs/TASK-742-buildmaster.md` **ABSENT** ✅. ✅ **Zone A verified by the suite itself** — all 5 `SiegeAssistantZoneATest.cpp` tests pass, and that file was never opened.

## 3. ⚠️ THE 6-vs-7 DISCREPANCY — **TASK-762 IS RIGHT; MY EARLIER FIGURE OF 6 WAS AN UNDERCOUNT**

Recounted from the preserved run-1 report (`report-run1-RED/index.json`) **before** anything was overwritten:

| test | error entries | reported lines |
|---|---|---|
| `MarkIsNeverARegion` | 1 | `4666` |
| `MarkSentencesParseEndToEnd` | 4 | `5363, 5365, 5367, 5382` |
| `MarkSymbolsReachThePlacesLine` | 2 | `4374, 4381` |
| | ⭐ **7** | |

**Raw run-1 log agrees independently: `LogAutomationController: Error: Expected` ×7.** ⇒ ⭐ **Seven assertions were red, ⛔ not six.** My earlier append and report said "6 assertions" — **that was my arithmetic slip on my own extracted data (1 + 4 + 2), ⛔ not a difference of definition**, and it is corrected here. **TASK-762 counted correctly and repaired all seven.** ⚠️ *The next reader should weight the earlier append's narrative accordingly; its root-cause analysis stands, its count did not.*

⚠️ **The two line-number lists differ only in ATTRIBUTION, ⛔ not in which sites:** TASK-762 cites the source lines of the `Grammar.Contains` calls (`:4373, :4375, :4663, :5362, :5364, :5366, :5368`); the runtime reports UE's macro `__LINE__` (`:4374, :4381, :4666, :5363, :5365, :5367, :5382`). **Same seven sites, both counts 7.**

## 4. ⭐ THE CORROBORATION IS REAL AND **STRONGER** THAN CITED — with one line-number correction

TASK-762 reports that the same file already asserts the correct escaped form at **`:3389`**. ⚠️ **The line number is off — current `:3389` is `FVector Centre = FVector::ZeroVector;`.** The assertion is at ⭐ **`:3594`**: `Grammar.Contains(TEXT("\\\"in\\\""), ESearchCase::CaseSensitive)`. *(The in-file comment at `:881` cites it without a line number, so this reads as a transcription slip, ⛔ not a fabricated citation.)*

⭐⭐ **And the corroboration is broader than one site — measured, 10 escaped-form usages in the file**, including **exact-equality** assertions on the full emitted terminal: `:3585-3586` pin `who` alternatives 4 and 5 to `"\"all\""` / `"\"none\""` character-for-character, plus `:2257` (`\"n\"`), `:3537`, `:3558-3559`, `:3571`, `:4114-4115`. ⇒ **The file's established convention was the escaped form throughout; the three mark tests were the sole outlier.** ✅ **This strengthens the ruling beyond what either the dispatch or my prior append claimed.**

## 5. ✅ THE DEAD NEGATIVE CONTROL IS ARMED — and its liveness is now ASSERTED, not assumed

Verified at source. The helper (`:903-972`) **measures** the wrapper off the shipped emitter's live output by walking the quoting alphabet (`IsGrammarQuotingChar`, `"` and `\` only) — ⛔ it does **not** mirror `GbnfJsonString`, and the file says why at `:893-900`: *"a mirror … would be a second copy free to drift from the first."*

Three properties I checked because each is a place this class of fix usually goes wrong:
- ⭐ **`IsWrapper()` (`:933-936`) is an anti-vacuity check asserted by EVERY caller** (`:4588`, `:4889`, `:5621`) — a zero-width wrapper fails loudly instead of sliding back to the bare substring search.
- ⭐⭐ **`IsIn()` (`:944-947`) is DELIBERATELY NOT gated on `bCalibrated`** — an uncalibrated spelling degrades to the **bare, broader** symbol, so a calibration failure makes an absence assertion **fire MORE readily, never less.** ⚖️ **Gating it would have re-created the silent pass.** This is the correct failure direction and it is documented as such.
- ⭐ **Calibrated on an INTENT, ⛔ not a place** (`:974-982`) — `intent` is the one generated rule `Build` emits **unconditionally** (`kind`/`where`/`zone` are all gated on the board having some), and its alternatives come from `ESiegeAssistantIntent` by reflection ⇒ the calibration symbol can neither be invented by the test nor go missing while the feature exists.

⭐ **The arming proof (`:4605-4613`) rebuilds the same board through the same shipped `Build` with `circle_4` appended and asserts the SAME needle DOES find it** ⇒ **the `circle_4` absence assertion at `:4599-4600` and `:5695-5696` can now genuinely fail.** It passed in this run. ⇒ **The guard protecting *"a mark Jonathan never drew is unsayable by the AI"* is live again**, and the old defect was proved by **measurement** rather than by argument.

## 6. 🚩 STILL UNDISCHARGED — ⛔ and a green suite does NOT touch it

🚩🚩 **THE PIE ROW (`GHOST-§4` / `SC-§35` ruling 5) REMAINS UNDISCHARGED.** ⛔ **248/248 green observes NOTHING about runtime.** It is a **commit gate**, ⛔ not waivable by a clean compile and ⛔ not dischargeable by argument. ⚖️ *This batch spawns a new actor class into the world; `SC-§35` was bought by 1,806 Blueprint runtime errors that compiled clean and cleared two QA gates.*

Also still open and ⛔ not touched by this run: **`W-1`** — the commit ships an **invisible, immobile** ghost until `BP_SiegeGhostPawn` exists (it could not exist before this compile) · the **`§25b` `IMC_Hero` index/worktree divergence** (index `9ba4aeb0…46ab` vs worktree `88f7f6e8…32fa1`) — ⛔ **verify by oid-vs-worktree-sha256, ⛔ NEVER by size** · the `NS_RecallChannel` **un-opened-duplicate inertness** risk, which needs a PIE eye.

*Appended 2026-09-01 by build-master. ⛔ No Git command issued, ⛔ no file staged, ⛔ no GUI editor launched, ⛔ no MCP listener started, ⛔ no `Source/` file edited, ⛔ no asset saved. `L_Arena.umap` re-verified `9ccd54ef…0e58` and CLEAN in git after the headless run. HEAD `f1c32d8`, 0/0 vs origin, index unchanged at the same 6 pre-staged assets. Artifacts: `…/scratchpad/t742/compile3.log`, `suite3.log`, `report3/index.json`, and the preserved red run at `report-run1-RED/`.*
