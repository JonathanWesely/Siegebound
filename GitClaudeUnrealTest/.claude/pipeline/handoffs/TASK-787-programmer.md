# TASK-787 — the two-seam widening: the tower learns to ADMIT, START and RELEASE any `ILadderClimber`

**Assignee:** gameplay-programmer · **Status set to:** `ready-for-qa` · **Law:** `CONTACT-§12` (all) · `§4.3`/`§4.4`/`§7b`/`§8`/`§10.1` · `TOWER-§8.4(B)`/`§10 L-1` · `SC-§36`/`§36.1` · `SHIP-§9c`
**⛔ No compile, no editor, no MCP, no Git, no `CONVENTIONS.md` edit.** One compile is TASK-780's.

---

## 0. THE HEADLINE, IN ONE PARAGRAPH

Both seams shipped **together**. `ILadderClimber` went **2 → 4** methods (`BeginLadderClimb`, `GetOnLadderClimbEnded` added; the surface is CLOSED again at four), `FSiegeLadderClimbEnded` **moved** `SummonedUnit.h` → `LadderClimber.h` with **type name and signature unchanged**, `AHeroCharacter` gained the delegate **instance + accessor + ONE broadcast** inside its existing teardown, and `AClimbableTower` now holds **zero concrete climber classes** on the climb path — identity, start, bind, unbind and the occupancy belt all read the interface. The hero's now-unreachable `NotAnAdmittedClimber` branch and its one-shot latch are **removed**. Suite delta **+3 tests**; **2 existing assertions were INVERTED** (repaired, never weakened) and are called out in §6 so TASK-779 cannot mis-file them.

---

## 1. SEAM 1 — THE WIDENED INTERFACE, AS WRITTEN (`LadderClimber.h`)

```cpp
class ACharacter;   // pointer parameter only — the Castle.h → `class ACastle;` precedent

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);

class ILadderClimber
{
    GENERATED_BODY()
public:
    virtual void AbortLadderClimb() = 0;
    virtual bool IsClimbing() const = 0;
    virtual bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld) = 0;   // ⭐ NEW
    virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() = 0;                           // ⭐ NEW
};
```

- **Zero signature churn, verified before relying on it:** both implementers already shipped `bool BeginLadderClimb(const FVector&, const FVector&)` character-for-character (`ASummonedUnit` and `AHeroCharacter`), so **neither declaration changed** — a matching signature overrides without repeating `virtual`.
- **No new include anywhere:** `SummonedUnit.h:10` and `HeroCharacter.h:13` already include `LadderClimber.h`.
- **`LadderClimber.h:41-44`'s now-false paragraph is REPAIRED, not deleted** (`CONTACT-§10.1`): it claimed `CONTACT-§2` made starting "the pawn's business". `§2` refused a shared **driver**, never a shared start **entry**, and the tower has always been the caller. The repaired text says exactly that and keeps the old wording quoted.
- **Still non-`UFUNCTION`** — the `IHealthBarProvider` half of the precedent, unchanged.

### ⭐ The one thing the board's fence did not anticipate, declared plainly
**`ASummonedUnit` had to gain the accessor too** — `SummonedUnit.h` now carries
`virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() override { return OnLadderClimbEnded; }`.
This is **required by the language**, not a scope choice: a pure virtual on an implemented interface makes the class abstract, and a `UCLASS` cannot be abstract by accident. The board's `SummonedUnit.h` fence says *"the delegate MOVE only"*; the accessor is the move's unavoidable other half. **`SummonedUnit.cpp` was NOT opened** (its `Broadcast(this, bReachedTop)` compiles untouched). Flagging it rather than letting QA discover it.

### ✅ And the shape is the shipped project idiom, measured — the strongest pre-compile evidence available
`HealthBarProvider.h` declares `FOnCombatantHPChanged` beside `IHealthBarProvider`, the interface exposes `virtual FOnCombatantHPChanged& GetHPChangedDelegate() = 0;` (`HealthBarProvider.h:66`), and each implementer owns a `UPROPERTY` instance returned by a one-line inline override — `Building.h:75`/`:78` and **`HeroCharacter.h:437`/`:440`, i.e. on the very class this task adds the second one to.** Delegate-in-the-interface's-header + accessor + per-implementer instance already compiles in this module.

---

## 2. SEAM 2 — THE COMPLETION PATH, END TO END (this is the brick)

**The failure it prevents:** the tower learns a climb ended through **exactly one channel**. A hero admitted to `ActiveClimber` that never broadcasts is never released ⇒ every later climber, **hero or unit**, gets `LadderBusy` for the rest of the match.

The path, in order:

1. **`AClimbableTower` binds BEFORE it starts** (both entry paths, ordering pinned):
   `ActiveClimber = Climber;` → `ClimberApi->GetOnLadderClimbEnded().AddUniqueDynamic(this, &AClimbableTower::HandleLadderClimbEnded);` → `ClimberApi->BeginLadderClimb(FromWorld, ToWorld)`.
2. **The hero's teardown broadcasts, LAST** — `HeroCharacter.cpp:1799`, the final statement of `EndLadderClimb`:
   `OnLadderClimbEnded.Broadcast(this, bReachedTop);`
   after `FSiegeLadderClimbStatics::End(LadderClimb)` consumed the exactly-once latch (`:1721`) and after `SetDefaultMovementMode()` restored the mode (`:1742`). **Measured offsets inside the function body: latch 367 < restore 1544 < broadcast 5480 of 5528 chars.**
3. **Exactly-once is INHERITED, not re-latched.** Ten exits, one teardown, one signal — `OnLadderClimbEnded.Broadcast(` appears **once in the whole file**, `SetDefaultMovementMode()` **once**, and the teardown has **exactly one `return;`** (the latch). No second flag was added.
4. **`HandleLadderClimbEnded` → `ReleaseClimber`** unbinds through the same accessor and clears the slot. `ReleaseClimber` still survives a climber with no live binding — **no "was it bound?" flag** (`CONTACT-§12.6`).

**`HeroCharacter.cpp:1777-1782`'s *"NO COMPLETION DELEGATE IS BROADCAST"* comment is REPAIRED, quoted, and credited** — it was correct when written and it is the declaration that bought this task.

---

## 3. THE CALLER — `ClimbableTower.{h,cpp}`

| What | Before | Now (live line) |
|---|---|---|
| identity term, contact path | `Cast<ASummonedUnit>(Climber) != nullptr` | `ClimberApi != nullptr` from `Cast<ILadderClimber>` — `:812` / `:821` |
| start, contact path | `CastChecked<ASummonedUnit>` + `Unit->BeginLadderClimb` | `ClimberApi->BeginLadderClimb(...)` — `:868` (**the same pointer the identity term used; no re-cast**) |
| bind, contact path | `Unit->OnLadderClimbEnded.AddUniqueDynamic` | `ClimberApi->GetOnLadderClimbEnded().AddUniqueDynamic` — `:857` |
| identity + team, **link path** | `Cast<ASummonedUnit>(AgentActor)` + `Unit->GetTeamId()` | `Cast<ACharacter>` → `Cast<ILadderClimber>` + `Cast<ITeamAgent>` — `:460`-`:466`, `:478` (**the contact path's own seam, now used by both**) |
| bind / start, link path | `Unit->…AddUniqueDynamic` / `Unit->BeginLadderClimb` | `:531` / `:539` |
| unbind | `if (ASummonedUnit* Unit = Cast<ASummonedUnit>(Climber))` | `if (ILadderClimber* ClimberApi = Cast<ILadderClimber>(Climber))` — `:618` |
| occupancy argument | `ActiveClimber.IsValid()` | `IsLadderSlotOccupied()` — `:474` and `:822` |
| identity parameter name | `bClimberIsSummonedUnit` | `bClimberIsAdmittedClimber` (both `EvaluateLadderEntry` and `EvaluateContactEntry`) |
| include | `#include "Siegebound/SummonedUnit.h"` | **removed** — and the file's include banner says why, rather than the include vanishing quietly |

**Preserved exactly:** precedence **IDENTITY → TEAM → OCCUPANCY** · `CanTeamAscend` as the live rule with its second caller · every verdict name and value (`NotAnAdmittedClimber` needed no rename) · bind-before-the-call with `ActiveClimber` set **first** · the refusal path's `ResumeAgentPathFollowing` · the fail-open pathfinding layer · sockets via `GetSocketTransform(..., RTS_Actor)` · the degrade-open socket fallback and **its Warning's severity, untouched** · no tick, no timer, no poll, no second slot.

### ⚖️ One declared deviation from the board's letter, in the spirit of `CONTACT-§11.1`
Board item (4) applies the belt to *"the occupancy argument at `:730`"* — the **contact** path. **I applied it to both paths** through one shared private helper, because item (3)'s *"two paths, one rule"* and `CONTACT-§12.5`'s closing clause (divergent release semantics are the thing being refused) both point that way, and two copies of the expression is exactly how the two paths come to disagree. `bool AClimbableTower::IsLadderSlotOccupied() const` — private, unreflected, **not** a tick/timer/poll (invisible to the `SiegeClimbableTowerTest.cpp` no-poll walk, which reads `FProperty`/`UFunction` only), and both its call sites land in this task (`SC-§36.1` clause 4).

### 🔎 A residual I considered and am declaring rather than hiding — **QA should rule on it**
The belt reads `slot held && held climber IsClimbing()`. Between `ActiveClimber = Climber` and a **successful** `Begin`, `IsClimbing()` is briefly false, so the slot reads **free** for the duration of that one call — where the old eager-only term read **busy**. I traced re-entrancy: the only entry points are the link's `FOnMoveReachedLink` and a pawn's own poll, and neither can run inside `BeginLadderClimb` (single-threaded; `Begin` arms a timer, stops movement and sets a movement mode — none of which re-enter this class). The window is therefore unreachable, **but it is a genuinely new window and I would rather it be reviewed than assumed.** The conservative alternative — treating "held but not yet climbing" as busy — would need a second flag, which `CONTACT-§12.6` forbids.

---

## 4. THE HERO'S DEAD BRANCH (item 5) — REMOVED, AND WHY THAT IS NOT A LOSS

`HeroCharacter.cpp`'s `NotAnAdmittedClimber` branch and `bWarnedLadderStartSeamClosed` are gone (the flag from the header too). A hero **cannot fail** `Cast<ILadderClimber>` — the interface is a compile-time base — so the branch is unreachable and its Warning could never fire. `SC-§36` inverted: a warning that cannot fire is indistinguishable from one that works. The reasoning is preserved in-source as a repaired comment at the same call site, and the **verdict itself stays in the enum** (an `ACharacter` implementing nothing is still refused, by identity).

---

## 5. `CONTACT-§10.1` — THE CITE SWEEP, ⛔ RE-GREPPED, ⛔ NEVER ARITHMETIC

The pre-edit lines were recovered by reconstructing the file (reversing this task's four edits) and then re-grepping every symbol in the shipped file — the two methods agree.

### `SummonedUnit.h` — the delegate move removed 24 lines and the fwd-decl edit added 5
| Old cite | What it named | New location |
|---|---|---|
| `:100` | *"SIGNATURE PINNED CHARACTER-FOR-CHARACTER IN TOWER-§8.4(B)"* (cited by `CONTACT-§8` **and** `TOWER-§8.4(B)`) | **`LadderClimber.h:37`** (moved file) |
| `:115` | `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, …)` | **`LadderClimber.h:57`** (moved file) |
| `:85-113` | the delegate's doc comment (cited by `CONTACT-§11.2`) | **`LadderClimber.h:16-56`** (moved file) |
| `:197` | the `ASummonedUnit` UCLASS base list (`… public ILadderClimber`) | **`:187`** |
| `:755` | tail of `BeginLadderClimb`'s doc (*"Descent has its own test."*) | **`:745`** |
| `:770` | `bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld);` | **`:760`** |
| `:781` | `void AbortLadderClimb();` | **`:772`** |
| `:793`/`:794` | `bool IsClimbing() const;` | **`:783`/`:784`** |
| `:801-804` | the broadcast-ordering doc (*"BROADCAST LAST…"*) | **`:791-794`** |
| `:806`/`:807` | `UPROPERTY(BlueprintAssignable …)` / `FSiegeLadderClimbEnded OnLadderClimbEnded;` | **`:796`/`:797`** |
| `:1129` | `float LadderClimbSpeedUU = 350.f;` | **`:1132`** (the accessor block below `:797` shifts this one **down**) |
| `:21` | `class ASummonedUnit;` (self-forward-declaration) | **REMOVED** — its stated reason was the delegate that left |
| `:10` | the `LadderClimber.h` include | **unchanged at `:10`** (comment extended) |

⚠️ Note the sign flip at `:1129`: everything above the accessor moved **up** ~10, everything below it moved **down** 3. A blanket shift would have been wrong in both directions — which is the rule's own point.

### `HeroCharacter.h` / `.cpp` (this task added ~55 lines to the header)
| Old cite | What | New |
|---|---|---|
| `HeroCharacter.h:999` / `:1003` | `WalkSpeed = 500.f` / `SprintSpeed = 750.f` (cited by **`CONTACT-§7b`**, itself a repair of the older `:736`/`:740`) | **`:1046`** / **`:1050`** |
| `HeroCharacter.h:1248` / `:1255` | `LadderClimb` / `LadderClimbWatchdogTimerHandle` (cited by `CONTACT-§8`) | **`:1295`** / **`:1302`** |
| `HeroCharacter.cpp:1582` / `:1585` | `IsClimbing` / `BeginLadderClimb` (cited by `CONTACT-§12.2`, `§12.5`) | **`:1578`** / **`:1585`** (unchanged) |
| `HeroCharacter.cpp:1716` / `:1721` | `EndLadderClimb` / the `End` latch | **unchanged** |
| `HeroCharacter.cpp:1777-1782` | the *"no completion delegate"* declaration | **`:1777-1799`**, repaired, ending in the broadcast |
| `HeroCharacter.cpp:2137-2174` | the dead `NotAnAdmittedClimber` branch | **GONE**; `PollLadderContact` now starts at **`:2086`** |
| `HeroCharacter.cpp:2008` | *"the clean landing is to move the property onto `AClimbableTower`"* | **repaired in place** — `CONTACT-§12.4` ruled that shape **out**; the comment now records the ruling, and the Warning string at **`:2048`** no longer suggests it |

### `ClimbableTower.cpp` (cited by `CONTACT-§12`'s opening box)
`:267` verdict → **`:287`** · `:729` identity → **`:821`** (contact) and **`:478`** (link) · `:747` `CastChecked` → **gone**, the start is **`:868`** · `:764` bind → **`:857`** · `:568` unbind → **`:618`** · `:775` start → **`:868`** · link path `:492`/`:499`/`:505` → **`:531`/`:539`/`:545`** · `:730` occupancy → **`:822`** · `EndPlay`'s interface abort `:225-227` → **`:236-243`** (the `Cast<ILadderClimber>` at `:238`, the abort at `:240`, the belt-and-braces `ReleaseClimber` at `:242`).

### `SiegeLadderClimbStatics`
`IsAtTopEndpoint` `.cpp:213` → **`:223`** (its body); the function opens at **`.cpp:207`**. Declaration `.h:479` → **`:492`**. Both prose blocks repaired (§7 below).

---

## 6. TESTS — SUITE DELTA **+3**, AND THE TWO ASSERTIONS THAT WENT RED BY DESIGN

### Added (+3)
1. **`Siegebound.ClimbableTower.TheClimbPathNamesNoConcreteClimberClass`** — zero `Cast<ASummonedUnit>` / `CastChecked<ASummonedUnit>` / `Cast<AHeroCharacter>` / `CastChecked<AHeroCharacter>` **in code**; no `SummonedUnit.h`/`HeroCharacter.h` include; exactly **2** interface starts, **2** binds, **1** unbind, **0** direct `->OnLadderClimbEnded.` reaches. Positive control: the scanner must find ≥2 `Cast<ILadderClimber>` first (it finds 5).
2. **`Siegebound.ClimbableTower.TheSlotIsReleasedForANonUnitClimberSoTheLadderIsNotBricked`** — **the brick test.** ① admitted → ② `LadderBusy` → ③ admitted again; `AHeroCharacter` really does implement `ILadderClimber` (reflection); **the hero's and the unit's `OnLadderClimbEnded` are the SAME delegate type** (`SignatureFunction` pointer identity — this is what forbids `CONTACT-§12.4`'s refused hero-only clone); the hero broadcasts exactly once; and the belt is present **without** having replaced the eager clear (`ActiveClimber.Reset()` still appears exactly twice: `ReleaseClimber` + `EndPlay`).
3. **`Siegebound.HeroLadderClimb.TheTeardownBroadcastsTheOneCompletionSignalLastAndOnlyOnce`** — the broadcast exists, is in the teardown, passes `(this, bReachedTop)`, is **one** in the function and **one** in the file, comes **after** the latch and **after** the restore (index comparisons, not `Contains` pairs — both anchors self-checked), and the teardown has exactly **one** `return;`.

Plus **compile-time pins** in `SiegeHeroLadderClimbTest.cpp` (the `SiegeLadderClimbTest.cpp` member-function-pointer idiom, second application): all four `ILadderClimber` methods and `AHeroCharacter::GetOnLadderClimbEnded`'s **reference** return. A by-value accessor would compile at the call site, bind the tower to a temporary and silently never release the slot — that is the brick wearing a different hat, and it is now a compile error naming `CONTACT-§12.3`.

### ⚠️ INVERTED — repaired, never weakened. **Read the causation before filing it as a weakened test.**
`SiegeHeroLadderClimbTest.cpp` test 20 row (d) required `PollLadderContact` to **contain** `NotAnAdmittedClimber` and `bWarnedLadderStartSeamClosed` — it asserted TASK-778's refusal was *reported* rather than swallowed, and it was **correct**. `CONTACT-§12` closed that blocker, so the branch is unreachable code carrying a Warning that cannot fire. **Both rows now assert the absence**, and a **positive control** requires the scanner to still find the verdict the poll *does* act on. The claim is as specific as it was and goes red the moment a dead identity branch reappears.

### ⭐ A probe change that is worth a QA line of its own
Both suites gained **`CountOccurrencesInCode`** (comment-only lines skipped). This codebase **deliberately writes the refused shapes into its comments** (`CONTACT-§12.4` exists so nobody re-proposes them); a text scanner that counted comments would have forced every one of those files to choose between *explaining what it refuses* and *passing its own test* — and the explanation would have lost. The prose is the guard, so the instrument got smarter instead. Declared limitation, in-source: a comment **trailing** a code line is still scanned; every probe here asks about a whole-line construct or a statement.

### Unchanged and re-checked green (predicted, not run)
Tower test 8(e) and 13(d) keep their identity assertions verbatim — **only their prose moved** from *"not an `ASummonedUnit`"* to *"not an admitted climber"*; both still prove identity outranks team and occupancy. `SiegeLadderClimbTest.cpp` is **untouched**: its `ASummonedUnit` member-function-pointer `static_assert`s, TASK-784's `WantsActorTick` pin, and its `OnLadderClimbEnded` reflection rows all read a property that stayed on the unit.

---

## 7. TASK-786's PROSE DEBT (item 6) — REPAIRED TO THE SHIPPED GEOMETRY

`SiegeLadderClimbStatics.cpp:210` and `.h:472` claimed the endpoint discs are **tangent at a 150 uu radius**. False. At `CONTACT-§7b`'s **K-6.1 = 350** they **overlap in a 2R − d = 400 uu lens**, and each endpoint's XY centre sits **R − d = 50 uu inside** the other's disc. The `.h` text also still named the **pre-TASK-785 sockets** (`foot −450`, `top −150`); they are `−460`/`−160` now, and because that move was a **pure translation** the separation is still exactly **300**, so every number survives it. Both blocks now also carry TASK-786's finding: **`IsAtTopEndpoint` takes no radius term at all**, so no radius retune can break it — a `PlatformHeightUU` change is the one that could. ✅ **Comment-only: zero executable change in that pair.** ⚠️ Stated as what it is rather than as a diff: that pair is still **untracked** (TASK-776 created it and the batch is uncommitted), so `git diff` cannot show it — both edits replaced text that was entirely inside a `//` block and a `/** */` block, and `IsAtTopEndpoint`'s one executable line (`.cpp:223`) is byte-identical. QA can confirm by eye in ~15 lines.

---

## 8. FILES TOUCHED (six + two test files, and nothing else)

- `Source/GitClaudeUnrealTest/Siegebound/LadderClimber.h` — 2 → 4 methods, `FSiegeLadderClimbEnded`'s new home, `class ACharacter;`, the repaired `§2` paragraph
- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` — parameter rename ×2, `IsLadderSlotOccupied()`, the fwd-decl removal, prose repairs
- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp` — both entry paths, bind/unbind, the belt, the include removal
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h` — accessor + `OnLadderClimbEnded` + M8 line; `bWarnedLadderStartSeamClosed` removed
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` — the broadcast, the repaired declaration, the dead branch out, the `§12.4` ruling recorded
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — the delegate **move** + the accessor it forces (see §1) + the stale self-fwd-decl
- `Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.{h,cpp}` — **comment-only**
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp`, `Tests/SiegeHeroLadderClimbTest.cpp`

**⛔ `SummonedUnit.cpp` was NOT opened.** It shows as modified in `git status` from TASK-776/784 — attribute it there, not here.

---

## 9. WHAT QA SHOULD SCRUTINISE FIRST

1. **The two seams shipped together** — one grep: zero concrete climber casts in `ClimbableTower.{h,cpp}` on the climb path, **including a single one guarding a unit-only bind**. Test 14 arms it.
2. **The belt window** in §3 (held-but-not-yet-climbing). My analysis says unreachable; I want it ruled on, not assumed.
3. **The `ASummonedUnit` accessor** in §1 — required by C++, outside the board's literal fence, `SummonedUnit.cpp` untouched.
4. **The inverted rows** in §6 — causation runs from the law to the test, not the other way.
5. **`H-10`** (`CONTACT-§12.6`): `EndPlay` still reaches the climber through `ILadderClimber` and now gets a broadcast back from the hero, so the slot clears on the tower's own death too.
6. **Not verified here, and TASK-780 owns it:** that any of this compiles, and the live sequence — walk the hero in, climb, **end the climb, climb again**, then order a **unit** onto the same ladder. A second refusal means the slot leaked.
