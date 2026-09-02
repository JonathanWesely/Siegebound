# QA Report — TASK-779

**Gate:** ONE gate over SEVEN diffs — TASK-776 · 777 · 778 · 784 · 785 · 786 · 787
**Inputs read:** `handoffs/TASK-775|776|777|778|783|784|785|786|787` + the shipped source of every file they name
**Law:** `CONTACT-§0..§13` (incl. the five mid-wave amendments `§7a` `§7b` `§8` `§10.1/10.2` `§11.1` `§12` `§13`) · `TOWER-§8.3`/`§8.4(A)(B)`/`§8.5`/`§8.5a` · `SC-§36`/`§36.1` · `SC-§37` · `SHIP-§9c` · `AS-§6 A-2` · `MARK-§1`
**Method:** read at source. ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git, ⛔ no TASKBOARD write (the dispatch fenced the board write; the status flip is returned to the orchestrator).

## Verdict: **PASS**

**BLOCKERS 0 · WARN 6 · NIT 4 · deviations ruled 9**

---

## ⭐⭐ THE HIGHEST-RANKING CHECK — BOTH SEAMS ARE PRESENT, TOGETHER, AND THE SLOT PROVABLY RELEASES

**✅ CONFIRMED AT SOURCE, ⛔ not on the handoff's word.**

**SEAM 1 — START.** `ILadderClimber` is 4 methods, closed (`LadderClimber.h:159` `AbortLadderClimb` · `:162` `IsClimbing` · `:183` `BeginLadderClimb` · `:210` `GetOnLadderClimbEnded`). Neither implementer's declaration changed (`SummonedUnit.h:760`, `HeroCharacter.cpp:1585` — both already `bool (const FVector&, const FVector&)`).

**SEAM 2 — COMPLETION, END TO END.** Traced, statement by statement:

| link | site | verified |
|---|---|---|
| the tower binds **before** it starts, both paths | `ClimbableTower.cpp:529-531` (link) · `:855-857` (contact) — `ActiveClimber = Climber` **first**, then `GetOnLadderClimbEnded().AddUniqueDynamic`, then `BeginLadderClimb` | ✅ |
| the hero **owns** the instance | `HeroCharacter.h:763-764` `UPROPERTY(BlueprintAssignable) FSiegeLadderClimbEnded OnLadderClimbEnded;` + accessor `:741`, returning a **reference** | ✅ |
| **one** delegate TYPE serves both pawns | `LadderClimber.h:59` — type name and signature unchanged; `SummonedUnit.h:797` and `HeroCharacter.h:764` are the same type | ✅ |
| the hero broadcasts **last** | `HeroCharacter.cpp:1799`, the final statement of `EndLadderClimb` | ✅ |
| …**after the latch** | latch `:1721` (`FSiegeLadderClimbStatics::End`, the only `return;` in the function) `<` broadcast `:1799` | ✅ |
| …**after the restore** | `SetDefaultMovementMode()` `:1742` `<` broadcast `:1799` | ✅ |
| the tower hears it and clears | `HandleLadderClimbEnded` `:553` → `ReleaseClimber` `:598` → unbind `:618` through the same accessor → `ActiveClimber.Reset()` `:638` | ✅ |
| `H-10` still reaches a hero | `EndPlay` `:236-243` — `Cast<ILadderClimber>` → `AbortLadderClimb()` → `ReleaseClimber` (belt) | ✅ |

**Exact-once counts, grepped in the shipped file — all THREE are ONE:**
`OnLadderClimbEnded.Broadcast(` → **1** (`:1799`) · `SetDefaultMovementMode()` → **1** (`:1742`) · `SetMovementMode(MOVE_Flying)` → **1** (`:1678`).

**THE SLOT-RELEASE TEST GENUINELY DISCRIMINATES.** `Siegebound.ClimbableTower.TheSlotIsReleasedForANonUnitClimberSoTheLadderIsNotBricked` (`SiegeClimbableTowerTest.cpp:1963`):
- (b) ① admitted → ② `LadderBusy` → ③ admitted again — and it is **not** a tautology: ③'s claim is *"the gate carries no state across calls"*, so it goes red the day anyone gives `EvaluateLadderEntry` a cache, a cooldown or a "has been refused" flag.
- (c) ⭐ **`SignatureFunction` POINTER IDENTITY** between `AHeroCharacter::OnLadderClimbEnded` and `ASummonedUnit::OnLadderClimbEnded` (`:2029`), with a self-check that the shared signature is non-null (`:2031`). **A hero-only lookalike fails here where a parameter-by-parameter compare would pass.** This is the row that makes ③ mean something in the world.
- (d) the hero's `OnLadderClimbEnded.Broadcast(` occurs **exactly once** in code.
- (e) the belt is present **and the eager clear survives it**: `ActiveClimber.Reset()` counted at **exactly 2** (`ReleaseClimber` + `EndPlay`) — the row that goes red if a future task ships the belt *instead of* the seam.

**⛔ ZERO concrete climber classes on the climb path** — grepped `ClimbableTower.{h,cpp}`: every `Cast<ASummonedUnit>` / `CastChecked<ASummonedUnit>` / `Cast<AHeroCharacter>` occurrence is on a **comment-only line** (`:27` `:52` `:57` `:232` `:607` `:837`); `#include "Siegebound/SummonedUnit.h"` is **gone**; `HeroCharacter.h` was never included. Including the hidden-one-entry case: there is **no** `Cast<ASummonedUnit>` guarding a unit-only bind — the unbind at `:616` is `Cast<ILadderClimber>`.

⇒ **A start-only widening did NOT ship. The ladder is not bricked.**

---

## Findings

### BLOCKERS — none

### WARN

- **[WARN] `Tests/SiegeHeroLadderClimbTest.cpp:57-61` — stale file-header claim, `CONTACT-§10.1`'s exact defect class, in a file TASK-787 edited.** It still reads *"the tower's entry gate admits `ASummonedUnit` ONLY (`ClimbableTower.cpp:650-656`), so the hero's poll is REFUSED at runtime today with the verdict `NotAnAdmittedClimber`."* TASK-787 falsified every clause of that sentence and the cite is stale twice over (live: `:285-287` / `:812` / `:821`). The same stale claim rides an assertion **message** at `:1319` (*"the refusal that blocks this feature today"*). Comment-only, no assertion depends on it, so it is not a blocker — but D-1 was ruled IN at TASK-777 for precisely this shape (*a pin that outlives its pinning is worse than no pin*). **Suggested fix: repair both blocks (never delete), same treatment 787 gave `HeroCharacter.cpp:1777-1799` and `LadderClimber.h:97-105`. Boardable as prose debt.**

- **[WARN] The brick test proves the release *composition*, not the live *cycle* — and that is the maximum a headless suite can reach here, so TASK-780's PIE row may not be waived.** Rows (b)①②③ drive the pure static `EvaluateLadderEntry(..., bBusy)`; they cannot show `bLadderOccupied` actually returning to `false` on a spawned tower. The test says so in-source (`:1999-2001`, `:1956-1961`) and routes it to TASK-780. **Ruled acceptable.** Consequence carried forward: the *only* instrument that can see a leaked slot is TASK-780's `hero climbs → ends → climbs again → a unit climbs after` row.

- **[WARN] TASK-787 §3's re-entrancy window — RULED ACCEPTABLE, and the invariant it rests on is recorded here because it is nowhere else.** Between `ActiveClimber = Climber` and a successful `BeginLadderClimb`, `IsLadderSlotOccupied()` reads **free** (the belt requires `held && HeldApi->IsClimbing()`, and `IsClimbing()` is `LadderClimb.bActive`, set inside `FSiegeLadderClimbStatics::Begin`). I traced it independently rather than accepting the analysis: the window is entirely inside one synchronous call; `AddUniqueDynamic` invokes nothing; the **hero's** `Begin` path is world-check → `IsRecalling()` → reflection read → capsule read → `Begin` → `SetMovementMode` → `SetTimer`, none of which re-enter a tower; the **unit's** `Begin` calls `AIController::StopMovement()`, whose `FinishUsingCustomLink` path reaches `UNavLinkCustomComponent::OnLinkMoveFinished` — and this tower binds **only** `SetMoveReachedLink` (`ClimbableTower.cpp:441`), never a finished delegate. ⇒ **unreachable today.** The unwritten invariant is: **nothing inside any `ILadderClimber::BeginLadderClimb` may re-enter `AClimbableTower`.** A future climber that raised an event from inside `Begin` opens it. ⛔ The conservative alternative (treat held-but-not-climbing as busy) would need the second flag `CONTACT-§12.6` forbids, so the shipped shape is right. **Suggested: one sentence on `IsLadderSlotOccupied`'s doc naming that invariant.**

- **[WARN] `SiegeLadderClimbTest.cpp:243-248` — the `WantsActorTick` arity `static_assert` catches the *repair*, ⛔ not the *mistake*.** It makes widening the predicate a compile error naming `CONTACT-§11.5`/`SC-§33` (✅ verified, and `RefreshActorTickEnabled` at `SummonedUnit.cpp:3649-3662` is byte-restored to `WantsActorTick(bLungeActive, LadderClimb.bActive)` with the refusal recorded in-comment). But a poll added to `ASummonedUnit::Tick` **without** widening it still compiles, still reviews clean, and still never runs. It is the strongest headless proxy available (with 15(e) and 14(e)'s diff read) — **not a defect**, but it means TASK-780's *"order a UNIT to the ladder and confirm it climbs"* row is the only direct instrument. ⭐ The **hero's** equivalent is genuinely always-on and I checked it independently, not by analogy: `PollLadderContact` is called unconditionally from `Tick` (`HeroCharacter.cpp:208-209`), the hero never writes its own tick flag, and the only runtime `SetActorTickEnabled` in `Source/` is a unit writing its own (`SummonedUnit.cpp:3662`). Its dwell accrues per-frame `DeltaSeconds` ⇒ the model is EXACT, not probabilistic. (`RefreshNearbyClimbableTowers` is rate-limited; **the ask is not** — correct.)

- **[WARN] TASK-785's `bLadderLineFromFallback` proposal is proposed-not-implemented and needs boarding.** A licence-aware fallback (the hero's `§8.5a` window declining an *unmeasured* line) reaches into `HeroCharacter.{h,cpp}` and is a `TOWER-§8.4(A)` vs `§8.5a` law question. ⛔ Not a defect in this batch; ⛔ do not let it evaporate into a closed handoff.

- **[WARN] `K-6.1`'s declared residual stands: buffed Cavalry does not close.** Battle Cry (+25%) ⇒ 750 uu/s ⇒ 14/16 phases at R=350. Full reliability needs `2 × 0.25 × 750` = **375**, which would push the grab window past **±270**. Declared by TASK-786, not slipped in. 🧑 Jonathan's row.

### NIT

- **[NIT] `handoffs/TASK-787-programmer.md` §5 — its own `CONTACT-§10.1` cite table has drifted for `HeroCharacter.h`.** Measured live: `WalkSpeed`/`SprintSpeed` **`:1057`/`:1061`** (table says 1046/1050) · `LadderClimb`/`LadderClimbWatchdogTimerHandle` **`:1306`/`:1313`** (table says 1295/1302) · the `LadderClimber.h` include **`:17`** (§1 says 13). ✅ The `SummonedUnit.h` half of the table is **exact** — I re-grepped `:187` base list, `:760` `BeginLadderClimb`, `:797` `OnLadderClimbEnded`, `:810` accessor, `:1132` `LadderClimbSpeedUU`, all confirmed. Use the live values above.
- **[NIT] The socket-Warning cite has moved and TASK-780 must not grep by line.** TASK-786 says `ClimbableTower.cpp:391`; the board's item (3a)(iv) says `:331-342`. **Live: `:411`.** It is the **only** `Warning`/`Error` in the file. Grep the **string**, not the line — see the TASK-780 section.
- **[NIT] `Δ = (300, 0.0002, 1200)` in TASK-783's FBX readback** — 2×10⁻⁴ uu of float noise. The code's pinned literals are exact (`ClimbableTower.cpp:101-102`). Note only; it does not perturb length, lean or the window.
- **[NIT] The dispatch expected the passer-by fixture at `100 → 250`; the shipped value is `100 → 300`.** See D-786-a — the deviation is ruled **IN** and is the better number.

---

## Deviations ruled

| # | Deviation | Ruling |
|---|---|---|
| **D-787-a** | `ASummonedUnit` gained `GetOnLadderClimbEnded()` (`SummonedUnit.h:810`), outside the board's literal *"the delegate MOVE only"* fence | ⭐ **IN.** The **language forces it**: a pure virtual on an implemented interface makes the `UCLASS` abstract. ✅ Verified `SummonedUnit.cpp` was **not** opened — the accessor exists only in the header, and `GetOnLadderClimbEnded` appears **zero** times in that `.cpp`; its `Broadcast(this, bReachedTop)` compiles untouched through the `CONTACT-§4.4` implicit conversion. The `.cpp`'s `M` in `git status` belongs to 776/784. ⚖️ The fence was under-specified, ⛔ not the placement wrong — `CONTACT-§11.1`'s shape, second application. |
| **D-787-b** | the belt applied to **both** entry paths via one private helper, where board item (4) named only the contact path's `:730` | ⭐ **IN.** Item (3)'s *"two paths, one rule"* and `CONTACT-§12.5`'s closing clause both point that way, and two copies of the expression is exactly how the two paths come to disagree about when a ladder is free. ✅ `IsLadderSlotOccupied()` is **private** (`ClimbableTower.h:776`, after `private:` at `:687`), **unreflected** (no `UFUNCTION`), and therefore invisible to `SiegeClimbableTowerTest.cpp:1197`'s `Timer`/`Tick`/`Poll`/`Interval` reflection walk — which reads `FProperty`/`UFunction` only. ⛔ It is **not** a tick/timer/poll. Both call sites (`:474`, `:822`) land in this task ⇒ `SC-§36.1` clause 4 satisfied. |
| **D-787-c** | the new re-entrancy window, **explicitly submitted for ruling** rather than hidden | ⭐ **ACCEPTABLE — see WARN 3.** Independently traced unreachable. ⚖️ Declaring it was correct; the alternative shape is forbidden by `§12.6`. |
| **D-786-a** | passer-by fixture `100 → **300**`, not the 250 the dispatch expected | ⭐ **IN, and it is the stronger number.** At R=350, `d=250` refuses by **0.015 s** — a coin flip dressed as an assertion; `d=300` refuses by **0.326 s** while the pawn is inside the disc for **1.202 s (≈4 dwells)** ⇒ the row now isolates the **cone** as the sole refuser. At the old `d=100` the pawn banks 0.895 s and **is** abducted, so leaving it would have shipped a genuine failure — ⛔ the fixture moved because the geometry moved, ⛔ not to green a red row. |
| **D-786-b** | `PinnedContactRadiusUU` fixed by moving the **fixture**, ⛔ not by splitting the constant | ⭐ **IN.** Two radius constants is two numbers that drift, and the abduction row would then prove safety at a radius the game does not ship. `FarFromFoot` `−300 → **−600**` (`:1443`) — the old value is no longer outside the disc at all, and at R=300 it sat **exactly on the rim** against an inclusive `<=`. Row (g) **re-sited, ⛔ not relaxed** (`:1594-1596`): velocity turned `Outward` so the two hypotheses diverge in **both** outputs (Z ⇒ `Climb` + ascent; XY ⇒ `NotHeadingIn` + descent). ⭐ Relaxing to `NotHeadingIn` would have proved **nothing** — the broken XY resolution yields it too. |
| **D-784-a** | the `ConsumeStuckDeltaSeconds()` hoist in `UpdateState` | ✅ **IN — behaviourally neutral, verified at `SummonedUnit.cpp:1453-1454`.** It is a **consuming** read (it latches `LastStuckTickTimeSeconds`), so a second call would hand the contact poll ~0 and the dwell would never advance. `TickStuckWatchdog` receives the identical value. ONE clock read, ONE value, TWO consumers. |
| **D-784-b** | `ForEachObjectOfClass` — a new idiom in this file (zero prior uses) | ✅ **IN.** Collect-first/act-second honoured (`UObjectHash.h:243`'s no-mutation contract + `FHashTableLock`); the lambda does **only** `Cast` / `IsValid` / `IsActorBeingDestroyed` / `GetWorld()==World` / `Add` — ⛔ no gameplay call inside the lock; `TInlineAllocator<4>`. The **world filter is mandatory, not padding** — the class hash spans every loaded world. |
| **D-776-a** | `ESiegeLadderExit` stayed in `SummonedUnit.h` | ✅ **IN, ruled against `CONTACT-§3.1` and ⛔ not against tidiness:** no lifted function takes a reason, its enumerators are unit-driver vocabulary, and publishing an eight-value exit enum on the shared header invites exactly the *"map the hero's ten across the unit's eight"* the law forbids. ⭐ And the law was honoured downstream — the hero authored its **own** `ESiegeHeroLadderExit`. |
| **D-776-b** | two moved comment clauses left deliberately stale + annotated in the new file-header | ✅ **IN.** A preserved-and-annotated original stays diffable character-for-character, which is what item (2) reads the region for; `CONTACT-§2` calls a *changed* comment a lost ruling. |

*(TASK-777's D-1/D-2/D-3/D-4 were already manager-adjudicated; I re-checked D-2's one load-bearing property and it holds — `SiegeLadderClimbStatics.h:10` still says "⛔ No .generated.h … deliberately", the file includes **only** `CoreMinimal.h`, and the appended block is a plain struct of statics + a plain enum. `LadderClimber.h` correctly carries the `UINTERFACE` and the delegate instead.)*

---

## ⚠️ THE TWO INVERTED ASSERTIONS — RULED **HONEST**, ⛔ NOT A REGRESSION AND ⛔ NOT A WEAKENING

`SiegeHeroLadderClimbTest.cpp` test 20 row (d) (`:1344-1347`) used to **require** `PollLadderContact` to contain `NotAnAdmittedClimber` and `bWarnedLadderStartSeamClosed` — it asserted TASK-778's refusal was *reported* rather than swallowed, and **it was correct**. Both rows now assert the **absence**.

**The causation is real and I verified it at source rather than reading it off the handoff:**
1. The tower's identity term is genuinely widened — `ClimbableTower.cpp:812`/`:821` (contact) and `:461`/`:478` (link) derive `bClimberIsAdmittedClimber` from `Cast<ILadderClimber>`, and `AHeroCharacter` implements `ILadderClimber` as a **compile-time base** (`HeroCharacter.h:398`). ⇒ a hero **cannot** produce that verdict.
2. The branch and the flag are genuinely gone — `bWarnedLadderStartSeamClosed` is **zero** occurrences in `HeroCharacter.{h,cpp}`; the removal is explained in-source at the same call site (`:2161-2177`).
3. The verdict itself **stays in the enum** (`ClimbableTower.h:363`) — an `ACharacter` that implements nothing is still refused, **by identity**, before team and before occupancy. The precedence `IDENTITY → TEAM → OCCUPANCY` is intact (`:285` / `:294` / `:307`).
4. The rows are **as specific as they were** and carry a **positive control** (`:1354-1355`): the code-only scanner must still find `ELadderContactVerdict::Climb` exactly once in the same body ⇒ neither absence can pass vacuously.

⚖️ **`SC-§36` inverted is the right reading: a warning that cannot fire is indistinguishable from one that works.** ⛔ Filing this as a weakened test would have the causation backwards.

**`CountOccurrencesInCode` (`SiegeClimbableTowerTest.cpp:490`) — RULED IN, and it is the right instrument.** This codebase deliberately writes the **refused shapes** into its comments (that is what `CONTACT-§12.4` is *for*), so a naive text scan would force `ClimbableTower.cpp` and `HeroCharacter.cpp` to choose between *explaining what they refuse* and *passing their own tests* — and the explanation would lose. ✅ The implementation is sound: `//`, `/*`, `*/`, `* `, and bare `*` continuations are skipped, and `*` is **qualified** rather than bare so a `*GetNameSafe(Foo)` argument line is still scanned. Its declared limitation (a comment **trailing** a code line is still counted) is stated in-source and does not bite: I checked every refused-form occurrence in `ClimbableTower.cpp` and **all six are whole-line comments**. Every probe carries a positive control (`:1886-1888` requires ≥2 `Cast<ILadderClimber>` before reporting four absences; it finds 5).

---

## The docket, row by row

**(1) `CONTACT-§12` — the pawn asks, the tower decides.** ✅ No pawn self-starts. The hero's poll (`HeroCharacter.cpp:2086-2179`) calls only `Tower->TryBeginContactClimb(this, DeltaSeconds)` (`:2147`) and accepts the verdict in silence; test 20(c) (`:1319`) asserts `PollLadderContact` never contains `BeginLadderClimb(`. The unit's (`SummonedUnit.cpp:3665-…`) makes **one** decision — which tower is nearest — and holds ⛔ not one authored float. `CanTeamAscend` keeps exactly **one** route (`EvaluateLadderEntry:294`), reached by **both** entry paths through `EvaluateContactEntry:758`; ⛔ no hero exemption (an enemy hero is refused by that same line). ⭐ The tower names **zero** concrete climber classes and `#include "Siegebound/SummonedUnit.h"` is removed.

**(2) The tick trap and its guard.** ✅ 784's poll rides the shipped 0.25 s `StateTimerHandle` — `UpdateState` `SummonedUnit.cpp:1472`, below the `bDead || !bStatsLoaded || bAIFrozen || bSpellFrozen || IsClimbing()` fence at `:1427` and above the follow hoist / profile dispatch / sidestep-lease early-out. The `return` after a successful ask (`:1472-1475`) is load-bearing and present. `RefreshActorTickEnabled` (`:3649-3662`) is **byte-restored**. The `static_assert` (`SiegeLadderClimbTest.cpp:243`) makes the "obvious repair" a compile error naming `CONTACT-§11.5`/`SC-§33` — see WARN 4 for its exact reach. ⭐ Hero side verified **independently**, not by analogy (WARN 4).

**(3) Ten exits through one teardown.** ✅ All ten present and all route through `EndLadderClimb` (`HeroCharacter.cpp:1716`): **H-1** arrival `:1847-1851` · **H-2** abort `:1705-1713` · **H-3** input release `:1819-1825` · **H-4** death `:827` (**before** `DisableMovement()` at `:842`, so a corpse rests at `MOVE_None`) · ⭐ **H-5 `UnPossessed`** `:2181-2189` — **before `Super`**, while the controller and movement component are still coherent · **H-6** respawn backstop `:873` (before the respawn's own walking-mode write) · ⭐ **H-7** recall teleport `:1438` — **BEFORE `OnHeroRecallArrived.Broadcast` at `:1443`**, which is the call that moves the actor · **H-8** match end `:1815` · **H-9** `EndPlay` `:1534` (before `Super`) · **H-10** tower `EndPlay` → `ILadderClimber::AbortLadderClimb()`. `SetDefaultMovementMode()` ×1, `MOVE_Flying` ×1, one `return;` in the teardown. ⭐ **The hero's own watchdog exists** and is not the unit's shape: `LadderClimbWatchdogTimerHandle` armed at `:1695` for the climb's duration only, cleared at `:1728` on **every** exit, serviced by `OnLadderClimbWatchdog` `:1974`; it is an `FTimerManager` entry **on the world**, so TASK-760's tick-flag hazard class cannot exist here. ⛔ Not harmonised onto the unit's shape.

**(4) `K-6.1` = 350 — RE-DERIVED INDEPENDENTLY, ⛔ not transcribed.** Hero speeds confirmed at source: `WalkSpeed = 500.f` (`HeroCharacter.h:1057`) · `SprintSpeed = 750.f` (`:1061`) · `MoveSpeedBonus = 0.25f` (`:1169`) ⇒ **500 / 625 / 750 / 937.5**. Floor = `0.35 × 937.5` = **328.125**; `350 / 328.125 = 1.0667` ⇒ **+6.67 %**. `R/v` at 350 = 0.700 / 0.560 / 0.467 / **0.373** — all ≥ 0.35; at 300 the fourth is **0.320 ⇒ fails at every offset**. ⭐ **20 fps floor re-derived**: `floor(W/f)` vs `ceil(0.35/f)` — 60 fps 22 vs 21 ✅ · 30 fps 11 vs 11 ✅ · 20 fps 7 vs 7 ✅ · **15 fps 5 vs 6 ⛔ FAILS**. Shipped value verified `ClimbableTower.h:653` = `350.f`; ⭐ **the other two `K-5` numbers did NOT move** — `LadderContactIntentCos = 0.5f` (`:669`) and `LadderContactDwellSeconds = 0.35f` (`:685`).
⚠️ **THE DECLARED COST IS RULED CORRECT AND THE `0.4·R` MODEL IS REFUTED.** I solved the shipped geometry myself — a pawn at perpendicular offset `d` is in-cone for `(√(R²−d²) − d/√3)/v`, and the `d/√3` term is exactly `IntentCos = 0.5`'s 60° half-cone (`|x| ≥ 0.5r ⇒ 0.75x² ≥ 0.25d² ⇒ |x| ≥ d/√3`). Setting that to 0.35 s at v=300 gives **±57.8 (R150) → ±202.1 (R300) → ±247.2 (R350)** — reproduced to the digit. **Super-linear at every step** (3.5× for the first doubling, then +22.3 % for a +16.7 % rise) because the dwell only ever eats a **fixed 105 uu** of a growing disc ⇒ ⛔ **any `0.4·R` model understates it at every size** (it would predict 60/120/140). A *linear* scale from ±204 would have guessed ±238 and understated by ~9 uu.
⭐ **The dwell "free lever" is free for UNITS ONLY, and the agent that proposed it retracted it** — a 0.50 s dwell needs `R ≥ 468.75` for sprint+Boots and `≥ 375` even for plain sprint; real hero headroom is `0.35 → 350/937.5 = **0.373 s**`. Retraction accepted.

**(5) The pin that does double duty.** ✅ Ruled — see D-786-a / D-786-b. `PinnedContactRadiusUU` (`:314`) is typed **from the law** with its label and derivation, ⛔ never read back off the class (`:127` of 786's handoff, verified at source).

**(6) Tangent-disc / Z-resolution at 350.** ✅ Endpoints 1,200 apart in Z, **300 in XY** ⇒ at 350 a **400 uu lens**, each centre **50 uu inside** the other's disc. ⭐ **The Z rule survives BY CONSTRUCTION — read, not assumed:** `FSiegeLadderContactStatics::IsAtTopEndpoint` (`SiegeLadderClimbStatics.cpp:207-224`) is `FMath::Abs(P.Z − Top.Z) < FMath::Abs(P.Z − Foot.Z)` and takes ⛔ **no radius term at all**. ⇒ ⛔ **a radius change can NEVER break it; a `PlatformHeightUU` change can** (it is a midplane test at 600 uu; a ground pawn stands at 88 ⇒ 512 uu margin). ✅ 787 item (6)'s prose repair is **comment-only** — the one executable line (`:223`) is byte-identical, in both `.cpp:209-222` and `.h`.

**(7) TASK-785 — the fallback.** ✅ `LadderFootDefaultRelative(-460,0,0)` / `LadderTopDefaultRelative(-160,0,1200)` (`ClimbableTower.cpp:101-102`), with the derivation moved with them (96 uu foot / 76 uu top, TASK-783's **measurements**). ⭐ The old pair genuinely reconstructed the standoff-**voiding** line (hero 51.62 against a required 56) degrade-open with one Warning and every readback correct. **BOTH RULINGS PRESERVED AND ⛔ NOT RE-ADJUDICATED:** ① the fallback stays **degrade-OPEN** — a refusing fallback creates `TOWER-§8.4(A)`'s forbidden unreachable deck and would punish the **units** (clearance 69.32, never in question) for a licence risk that was the **hero's**; ② ⛔⛔ **the socket Warning's severity is NOT raised** — verified at source: `ClimbableTower.cpp:411` is the **only** `Warning`/`Error` in the file, verbatim, and TASK-786 made **zero** edits to that `.cpp`. ⭐ My own item (7) reads the **absence** of that string as TASK-780's PIE confirmation, so it is intact. ✅ Test pins repaired, ⛔ not weakened and ⛔ not synced off the class; the **derived** pins (1236.9 / 76.0) untouched — `Δ` is translation-invariant. ✅ I confirmed rather than took on trust that **no CONTACT scenario row changed meaning**: every scenario point is expressed *relative* to the two pins (`PinnedLadderFootRelative + FVector(-100,…)`, `.X - 700.f`, `PinnedLadderTopRelative + FVector(100,…)`, `FVector(PinnedLadderTopRelative.X, 0, hh)`), and the predicate is a function of distances and bearings only.

**(8) TASK-776 — the lift.** ✅ `+1 −292` / `+1 −175` / `+1 −0`, every `+1` an `#include`; the moved region is contiguous **from the top of the file** to the attribution banner at `SiegeLadderClimbStatics.cpp:195-205`, so item (2)'s byte-diff is still possible. All six safety properties **re-verified in their new home, ⛔ not accepted on the handoff's word**: eight exits through one latched teardown · the deck-breach window (**Z-resolved** elevated end `.cpp:82` `State.bDeckIsAtEnd = (End.Z >= Start.Z)` ⛔ never argument order · `≤ 3× half-height` converted by the line's **own slope** `.cpp:87-91`, ⛔ no hardcoded `sin(76°)` · continuous drive ⛔ not a teleport · velocity zeroed each breach frame · snap only on a **real** arrival · half-height **read** from the capsule with `SiegeSpawn::DefaultCapsuleHalfHeight` as the null-capsule fallback only · 78 % still swept, `ShouldSweep` measuring from the **elevated** end `.cpp:127`) · the symmetric capsule-centre lift `.cpp:64-70` · TASK-760's self-heal (`SummonedUnit.cpp:1405` re-assert on the independent `StateTimerHandle` poll) · the transient disarm at three guard points with ⛔ **no `CanEverAttack()` override** · `CanBegin`'s **symmetric `SizeSquared()`** `.cpp:43` with its verbatim comment ⇒ ⛔ **no Z-ordering**, and the fields are still `Start`/`End` (⛔ not `Foot`/`Top`). ✅ Suite delta **ZERO** for its own diff.

**(9) 787's three flagged items.** ✅ All three ruled — D-787-a, D-787-b, D-787-c above.

**(10) The two inversions + `CountOccurrencesInCode`.** ✅ Ruled — see the section above.

**(11) Can the new/moved assertions actually FAIL?** ✅ Audited every new and moved row. Positive controls / self-checks present on: tower 14 (`:1886`, ≥2 `Cast<ILadderClimber>`, finds 5) · tower 15(a) (`:1987` self-check the widening **added** a class rather than swapping one) and 15(c) (`:2030` the shared signature is non-null — comparing two nulls would "pass") · hero 24(c) (`:1662-1663` both index anchors are real; an `INDEX_NONE` of −1 would make every "comes after" trivially true) · hero 20(d) (`:1354` the code-only scanner still finds the verdict the poll **does** act on) · tower 11(f) (`:1544` the two walks must **disagree**) · 784's 15(a)/16(b)/17(a) (two-sided probes and edge self-checks). ⛔ **Pins are typed from the law, ⛔ never read back off the class** — verified for `PinnedContactRadiusUU` (350), `PinnedLadderFoot/TopRelative` (−460/−160), and `SiegeLadderClimbTest.cpp:1089` (`ClimberParam->PropertyClass == ACharacter::StaticClass()` — a strict equality, ⛔ **not** weakened to a null check, so item (5a)'s hazard is clear). Compile-time pins added: `ILadderClimber::GetOnLadderClimbEnded` and `AHeroCharacter::GetOnLadderClimbEnded` must return `FSiegeLadderClimbEnded&` — ⭐ a by-value accessor would compile at the call site, bind the tower to a **temporary** and silently never release the slot; that is the brick wearing a different hat, and it is now a compile error.

**(12) THE SUITE — see below.**

---

## Conformance rows checked and clear

- ✅ **`CONTACT-§4.4`'s ruled delegate shape, character-for-character:** `LadderClimber.h:59` — `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop)`. Type name **`FSiegeLadderClimbEnded`** (⛔ not `FOnLadderClimbEnded` — the refused rename stays refused) · first param **`ACharacter*`** (⛔ not `APawn*`) · `ActiveClimber` is `TWeakObjectPtr<ACharacter>` (`ClimbableTower.h:840`) · ⛔ **no second parallel hero-only slot** · `ILadderClimber` lives in its **own** header · ⛔ **no `Cast<ASummonedUnit>`/`Cast<AHeroCharacter>` branch pair**.
- ✅ **A `DECLARE_DYNAMIC_MULTICAST_DELEGATE` did NOT land in `SiegeLadderClimbStatics.h`** — that pair still says *"⛔ No .generated.h … deliberately"* at `:10` and includes only `CoreMinimal.h`. The automatic blocker did not fire.
- ✅ **The `class ACharacter;` forward declaration is sufficient and has precedent in this very module** — a dynamic delegate's pointer parameter needs no complete type, and `HeroCharacter.h:44`/`:260` already declare delegates over a forward-declared `AHeroCharacter*`. The `LadderClimber.generated.h` include is still last (`:7`).
- ✅ **The delegate's move does NOT break Blueprint bindings.** A global `DECLARE_DYNAMIC` delegate's signature function is outered to the **module package** (`/Script/GitClaudeUnrealTest`), so moving its declaration between headers **inside the same module** leaves the object path unchanged. TASK-777's measured BP risk (the *signature widening*) is unchanged by 787 — carried to TASK-780's message-log read anyway.
- ✅ **`AS-§6 A-2`:** `Escape` appears **once** in the whole batch, in a comment stating it is untouchable (`HeroCharacter.cpp:1292`); **zero** occurrences in `ClimbableTower.cpp`. ⛔ No `Escape` handler of any kind. **Not an automatic fail.**
- ✅ **`RECALL-§`'s shipped cancel routes still fire.** The one insertion is `H-7`'s `AbortLadderClimb()` at `:1438`, guarded by the climb latch (a no-op on a non-climbing hero) and correctly ordered **before** the teleport broadcast. All seven recall exits intact; `EndRecall`'s own latch is still its first line.
- ✅ **Scope creep (`CONTACT-§6`):** ⛔ no garrison order, ⛔ no `tower` place symbol, ⛔ no new intent, ⛔ no bot rule. Zone A's `5658` freeze is **untouched** — the batch's ten files do not include `SiegeAssistantSnapshot.{h,cpp}`, `WarMapWidget.{h,cpp}` or `SiegeAssistantZoneATest.cpp`.
- ✅ **`SC-§36.1` clause 4 — every `public` function added in this batch has a shipped caller:** `TryBeginContactClimb` → **two** (`SummonedUnit.cpp:3665`/`ClimbableTower` via `HeroCharacter.cpp:2147`) · `FSiegeLadderContactStatics::{WantsToClimb, Disarm, IsAtTopEndpoint}` → `ClimbableTower.cpp:734`/`:589`/`:794` · `ILadderClimber::{AbortLadderClimb, IsClimbing, BeginLadderClimb, GetOnLadderClimbEnded}` → `:240`/`:676`/`:539`+`:870`/`:531`+`:857`+`:618` · `AHeroCharacter::{BeginLadderClimb, AbortLadderClimb, IsClimbing, GetOnLadderClimbEnded}` → reached through the interface. 787 added **no** new public function to `AClimbableTower` (`IsLadderSlotOccupied` is private). ⭐ `GetLadderLink()` (`ClimbableTower.h:461`) is **pre-batch** — it is consumed by the pre-777 link test at `SiegeClimbableTowerTest.cpp:966`, so 784 did **not** widen its narrow grant.
- ✅ **`TOWER-§8.5a`'s VOID IS DISCHARGED — the licence is real.** `handoffs/TASK-783-artist.md` reports **all THREE** `§8.3` numbers, measured over the whole line against every hull, deck slab excluded: climb line **1236.931688 / 75.963757°** (`Δ` unchanged) · standoff **`dist(spine, geometry) = 103.32015 uu`** at hull 00, `t = 246.43` vs the `≥ 98.0` gate ⇒ **clears by +5.32** (hero 61.320, unit 69.320) · rung plane **mid −22.0 / near −12.0 / far −32.0, stiles ±66/±86 — UNMOVED** ⇒ ⛔ `A_SiegeBiped_Climb` needs **no** re-export and `TOWER-§8.3`'s mesh↔clip binding does **not** fire. ⇒ this gate **can** pass, and the hero's `§8.5a` window is licensed.
- ✅ **`Δ` is still `(300, 0, 1200)`** and TASK-778's numbers did **not** move: length 1236.93169 · lean 76.0° · lift 96.0 · breach ceiling 288 uu of Z ⇒ 296.863 uu of line = **24.00 %** · watchdog budget 3.5341 s. The cheap decisive cross-check passes.
- ✅ **TASK-778's handoff names the ruled option — `A` — explicitly** (line 5), and names no refused one.
- ✅ **The flag mapping (`CONTACT-§3.2`) is a MAPPING, ⛔ not a typed `false`:** `bAIFrozen ⇒ IsMatchOver()` (a real mapping — `ASiegeGameMode::FreezeWorldAtMatchEnd` is what sets `bAIFrozen` on units) and `bSpellFrozen ⇒ false` as a **DECLARED GAP with its grep evidence in-source** (`HeroCharacter.cpp:1647-1656`: Frost applies only to `ASummonedUnit`/`ABuilding`; zero Freeze/Frozen/Stun in `HeroCharacter.{h,cpp}`). ⇒ acceptable under item (6). ⭐ The **fifth** term (`CONTACT-§3.4` bullet 1 — no climb start during a recall channel) is enforced twice: at the poll (`:2124`) and as a refusal inside `BeginLadderClimb` (`:1611`).
- ✅ **`AMinerUnit` can never contact-climb** (`MinerUnit.cpp:62` seals `StateCheckInterval = 0`) — declared, asserted as a *discriminating* test, and the structural seal was ⛔ not re-opened. Correct: this project has refused that twice.
- ✅ GC-safety: `NearbyClimbableTowers` is `TArray<TWeakObjectPtr<AClimbableTower>>` (`HeroCharacter.h:1336`), pruned on use; `ActiveClimber`/`ActiveClimberPathComp`/`LadderContacts[].Climber` all weak. ⛔ No raw `UObject*` member added anywhere in the batch.

---

## ⛔ THE ONE SUITE TOTAL FOR TASK-780 — **279**

**Computed two independent ways; they agree exactly. ⛔ Neither is a raw tree count taken on its own.**

**(A) Baseline + declared deltas.** Baseline **248** (`handoffs/TASK-776-programmer.md` §5, the batch's own on-disk measurement at the lift):

| task | declared | verified |
|---|---|---|
| 776 | **0** | ✅ `SiegeLadderClimbTest.cpp` 14 → 14, the only edit an `#include` |
| 777 | **+2** | ✅ tower file 10 → 12 |
| 778 | **+23** | ✅ NEW `SiegeHeroLadderClimbTest.cpp` |
| 784 | **+3** | ✅ ladder file 14 → 17 (+2 `static_assert`s, compile-time, **no** suite count) |
| 785 | **0** | ✅ an assertion **repaired**, ⛔ none added or deleted |
| 786 | **0** | ✅ pins and fixtures **moved**, ⛔ never added; row (g) still holds its 4 assertions |
| 787 | **+3** | ✅ +2 tower, +1 hero (+5 compile-time pins, **no** suite count) |

`248 + 0 + 2 + 23 + 3 + 0 + 0 + 3` = **279**.

**(B) Static census of `IMPLEMENT_*_AUTOMATION_TEST` across `Source/`** = **279** across 21 files. `279 − 31` = **248**, which reproduces the declared baseline exactly. ⇒ ⛔ no task's "zero delta" hid a deleted assertion, and the seven declarations **sum**.

**Per-group expectations, so a mismatch localises rather than just failing:**
`Siegebound.LadderClimb` = **17** · `Siegebound.ClimbableTower` = **14** · `Siegebound.HeroLadderClimb` = **24** (new group).

⚠️ 279 is the macro census. TASK-731's build-master handoff established that macro census == executed count for this project (declared 171 == actual 171 on a live run), so **assert 279**; ⛔ asserting 248, 276 or 277 would read a correct suite as broken.

---

## Notes for build-master — what TASK-780 must verify

**⛔ 1. THE MESH AND ITS THREE TEXTURES IMPORT TOGETHER OR NEITHER IMPORTS (`CONTACT-§13`) — this is the item most likely to be dropped.**
TASK-783 measured that `uv_atlas()` packing is **object-space**, so the 10 uu translation repacked **2508 / 2508** loop UVs (max delta 0.973), and a same-mesh determinism control proved it **causal** (`0.0000000000`), ⛔ not packer noise. ⇒ `T_WatchTower_{D,N,ORM}` were **re-baked** and **MUST be reimported with `SM_WatchTower`**. ⛔ **Shipping the mesh against the stale atlas makes the tower sample GARBAGE with every readback correct and not one warning.** The struck *"no texture reimport"* text in item (3b) is **wrong**; read the repaired text. Same-path overwrite, references preserved (`Tools/reimport_meshes.py`); `MI_WatchTower_PBR` is **not** opened.

**2. The PIE rows this gate cannot see (`SC-§35` — ⛔ not waivable on a clean compile):**
- ⭐⭐ **`hero climbs → END that climb → climb AGAIN → then order a UNIT onto the same ladder and confirm it climbs.`** A second refusal means the occupancy slot **leaked** ⇒ ⛔ **STOP.** This is the only instrument that can see it (WARN 2) — the suite proves the composition, not the live cycle.
- ⭐ **A UNIT walks to the foot and climbs.** The only direct test that 784's poll actually *runs* (WARN 4). A unit that stands there is a ⛔ STOP.
- ⭐ **SPRINT at the ladder (with Swift Boots if obtainable) — 937.5 uu/s is the speed `300` could never admit at any offset.** ⚠️ If PIE is below **20 fps**, say so rather than reporting a failure as a defect (measured: holds to 20, fails at 15).
- **Walk PAST at an angle and confirm no abduction.** ⚠️ The half-window is **±247.2 uu at v = 300**, ⛔ not ±125 — twice as load-bearing as the old text claimed.
- Release input mid-climb (**drops**, ⛔ never hangs) · descend and confirm ⛔ no instant re-ascent (`K-C`) · die mid-climb (the ghost hand-off, `H-5`) · destroy the tower under yourself (`H-10`).

**3. `Q2(b)` and the socket Warning — use the STRING, ⛔ not a line number.**
The Warning is **`ClimbableTower.cpp:411`** (NIT 2 — the board's `:331-342` and 786's `:391` are both stale). Grep the log for **`ladder line taken from the TOWER-§8.3 pinned literals`**. ⛔ **Its ABSENCE = the sockets resolved**, and with the mesh reimported that now confirms the **new** `−460`/`−160` coordinates resolved. Its **presence** = the fallback fired and the named socket is missing. Rows (i)–(iv) (`RecastNavMesh` `RuntimeGeneration = Dynamic`; a nav poly at `LadderFoot`'s world XY; a nav poly on the deck) are unchanged and any failure is a STOP.

**4. Read back and report:** sockets **`LadderFoot (−460,0,0)` / `LadderTop (−160,0,1200)`** · **8** convex hulls, `bIsGenerated == false` ×8, `box_count 0` · Nanite **off** · slots `[TeamRegion, WatchTowerPBR]` · ⛔ **ZERO hulls over the ladder** (collision there would break the shipped `CONTACT-§4` predicate — ⛔ STOP). ⚠️ TASK-783 owes `bIsGenerated ×8` and Nanite-off from the engine side (its pre-flight was a real MCP request that failed, ⛔ never a port check).

**5. The compile.** ⛔ **PARSE THE LOG FOR `Result: Failed` — ⛔ never trust `$LASTEXITCODE`.** A failure is appended to **this file** and routed back. **Expected-clean, ⛔ not expected-red:** both armed tripwires were reconciled inside the batch — `SiegeClimbableTowerTest.cpp`'s radius pin (150 → 350, label `K-5` → `K-6`) by TASK-786, and the `−450/−150` pins by TASK-785. ⇒ ⛔ **there should be NO red row; one would mean a reconciliation was lost.**

**6. The message log:** watch for a Blueprint binding error on `OnLadderClimbEnded` (TASK-777 raised the `BlueprintAssignable` signature-widening risk and largely discharged it; 787's *move* adds no new risk — same module package, same object path — but the log is free).

**7. Suite:** assert **279**. Per-group: `LadderClimb` 17 · `ClimbableTower` 14 · `HeroLadderClimb` 24. A red suite is a STOP.

**8. Carry to the manager (⛔ not TASK-780's to fix):** WARN 1 (the stale `SiegeHeroLadderClimbTest.cpp:57-61` header + the `:1319` message) · WARN 3's invariant on `IsLadderSlotOccupied` · WARN 5 (`bLadderLineFromFallback`) · WARN 6 (buffed Cavalry at 375/±270) · NIT 1 (787's `HeroCharacter.h` cite table: live `:1057`/`:1061`/`:1306`/`:1313`/`:17`).

---

**Board action owed (⛔ this gate was fenced from writing it):** `TASK-779 → qa-passed`; `TASK-780` unblocked.

---

## ⛔ BUILD-MASTER APPEND — TASK-780 PHASE 1: COMPILE **FAILED** (2026-09-02)

**Parsed from the log — ⛔ not from the exit code:**

```
Result: Failed (OtherCompilationError)
```

Raw process exit code was **`6`**. ⚠️ Recorded for the standing law: the exit code did **not** lie this time, but the verdict above is the one taken from the log, as required. Total execution time **22.21 s** (⛔ **not** a ~2 s death, and ⛔ **no `0x800711C7`** ⇒ this is **NOT** Smart App Control). ⛔ **No Live Coding lock** — the editor was measured **DOWN** (`tasklist` → no `UnrealEditor.exe`) before the build started. ⇒ **This is a real code error and DOES count as a QA loop.**

**Scale:** 19 of 22 actions ran; **2 errors, 0 warnings.**

### The two diagnostics — ⭐ ONE root cause, BOTH in test files

**(1) `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp(2030,3)`**
```
error C2672: 'FAutomationTestBase::TestNotNull': no matching overloaded function found
    TestNotNull(TEXT("(c) SELF-CHECK: the shared signature function exists - comparing two nulls would 'pass' while proving nothing"),
        HeroClimbEnded->SignatureFunction);
note: could be 'bool FAutomationTestBase::TestNotNull(const FString &,const ValueType *)'
note: ...could not deduce template argument for 'const ValueType *' from 'const TObjectPtr<UFunction>'
note: or       'bool FAutomationTestBase::TestNotNull(const TCHAR *,const ValueType *)'
note: ...could not deduce template argument for 'const ValueType *' from 'const TObjectPtr<UFunction>'
```

**(2) `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroLadderClimbTest.cpp(436,3)`** — identical error, identical notes:
```
error C2672: 'FAutomationTestBase::TestNotNull': no matching overloaded function found
    TestNotNull(TEXT("(e) SELF-CHECK: the delegate carries a signature function at all - an identity compare between two nulls would pass while proving nothing"),
        HeroClimbEnded->SignatureFunction);
```

### Mechanism (build-master diagnosis — ⛔ no fix authored, this is the programmer's call)

In UE 5.8 `FDelegateProperty::SignatureFunction` is a **`TObjectPtr<UFunction>`**, and both `TestNotNull` overloads are **templates** deducing `const ValueType*`. Template argument deduction ⛔ **does not** run `TObjectPtr`'s implicit conversion-to-raw-pointer, so neither overload matches. ⭐ The failure is **specific to the deduced-template call** — the *sibling* lines on the very same objects all compile clean, which is why only these two rows fell over:
- `HeroClimbEnded->SignatureFunction == UnitClimbEnded->SignatureFunction` — `TObjectPtr` comparison, fine.
- `if (HeroClimbEnded->SignatureFunction)` — explicit `operator bool`, fine.
- `HeroClimbEnded->SignatureFunction->NumParms` — `operator->`, fine.

⇒ Both sites are the **new SELF-CHECK rows this batch added** (the guard against "two nulls compare equal and pass while proving nothing"). The *intent* of both rows is sound and QA's reasoning for adding them stands; only the call idiom fails to compile. A non-deduced raw pointer at the call site resolves it — **exact form is the programmer's to choose and QA's to re-review.**

### ⭐ What this failure does NOT implicate — narrowing for the return trip

- ⛔ **The `UINTERFACE` is CLEAR.** UHT emitted **zero** diagnostics; reflection code generation completed and every `Module.GitClaudeUnrealTest.*.gen.cpp` unit compiled. The flagged UHT risk did **not** materialise.
- ⛔ **All gameplay code compiled clean:** `ClimbableTower.cpp` [1/22], `HeroCharacter.cpp` [4/22], **`SiegeLadderClimbStatics.cpp` [12/22]** (the new pair), `SummonedUnit.cpp` [19/22]. The new `LadderClimber.h` and the widened interface produced **no** errors.
- ⛔ **`SiegeLadderClimbTest.cpp` compiled clean** [14/22] — the `LadderClimb` group is unaffected.
- ⇒ The blast radius is **two lines in two test files.** ⛔ Nothing in the shipped runtime path is implicated.

### Suite: ⛔ NOT RUN — unreachable

**Declared 279 · actual = N/A.** The link never produced a binary, so there is no build to test. ⛔ The previously-built binary was **deliberately not** run: it contains the **old** code and a green result from it would be a **false pass**. Group riders `LadderClimb 17` · `ClimbableTower 14` · `HeroLadderClimb 24` are **all unverified.** ⚠️ Note `SiegeHeroLadderClimbTest.cpp` — the file carrying the `HeroLadderClimb 24` rider — is one of the two that failed to compile, so that group had **no chance** to register.

⛔ The two deliberately-INVERTED assertions (`SiegeHeroLadderClimbTest` test 20(d)) remain **unobserved** — ⛔ do not read this failure as evidence about them either way.

### Status

`TASK-779` / `TASK-780` → **`qa-failed`**, routed back to `gameplay-programmer`. **Counts as a QA loop.**
⛔ Nothing staged, ⛔ nothing committed, ⛔ no push, ⛔ editor not opened (it stays Jonathan's to launch). Phase 2 (asset reimport + PIE + commit) is **NOT** started and remains fully owed.
Full build log retained at `C:/Users/wesel/AppData/Local/Temp/claude/C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest/10fcb540-8a89-457a-9798-070dabfaf278/scratchpad/task780_build.log`.

**Measured git state at gate time (⛔ branched on what was read, not on the briefing):** `HEAD = cf45f13c4c4949642a9524168360caa1feefaa0d`, main **3 ahead / 0 behind** `origin/main` — matches `0990caa` · `4a03e03` · `cf45f13`. ⛔ No mid-session commit or push by Jonathan this time.
