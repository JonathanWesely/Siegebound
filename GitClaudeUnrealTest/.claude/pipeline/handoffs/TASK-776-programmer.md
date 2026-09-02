# TASK-776 — [CONTACT-1] THE STATICS LIFT — handoff (gameplay-programmer)

**Status:** `ready-for-qa` · **Law:** `CONTACT-§2` / `§8` · `TOWER-§8.4(B)` / `§8.5` / `§8.5a` · `SHIP-§9c`
**Compile:** ⛔ NOT run (ONE compile, at TASK-780). **Editor / MCP / Git:** ⛔ untouched.

---

## ⭐⭐ THE HEADLINE — THE DIFF IS A RELOCATION PLUS THREE INCLUDE LINES, AND THE NUMBERS SAY SO

```
+1  -292   Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h
+1  -175   Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp
+1    -0   Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp
        NEW  Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.h    (351 lines)
        NEW  Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.cpp  (190 lines)
```

**⭐ Every one of the three `+1`s is an `#include`.** There is ⛔ not a single other added or modified
line in any shipped file. Three files touched, two files created — ⛔ nothing else in `Source/`.

### ⛔ THE BYTE-IDENTITY PROOF, RUN RATHER THAN ASSERTED

The move was performed by **extracting the exact line ranges** out of the shipped files and
re-assembling them, ⛔ never by re-typing. Verified afterwards with `diff`, all three IDENTICAL:

| moved range | from | to | `diff` |
|---|---|---|---|
| the ladder-climb banner | `SummonedUnit.h:48-74` | `SiegeLadderClimbStatics.h:61-87` | ✅ IDENTICAL |
| `FSiegeLadderClimbState` + `FSiegeLadderClimbStatics` (incl. the blank between) | `SummonedUnit.h:117-379` | `SiegeLadderClimbStatics.h:89-351` | ✅ IDENTICAL |
| the `.cpp` banner + all **seven** definitions | `SummonedUnit.cpp:129-302` | `SiegeLadderClimbStatics.cpp:17-190` | ✅ IDENTICAL |

⚠️ **Line endings:** both new files are **CRLF**, matching `SummonedUnit.{h,cpp}`. An intermediate
`sed` pass silently stripped `\r` from the two edited files; it was caught and re-normalised, and
`git diff --numstat` above is the proof it left no whole-file churn behind.

---

## 1 · WHAT MOVED — ENUMERATED FROM THE SOURCE, ⛔ NOT FROM THE SPEC'S LIST

`FSiegeLadderClimbState` — **9 fields**, all defaults unchanged:
`bActive` · `Start` · `End` · `LengthUU` · `ElapsedSeconds` · `TimeoutSeconds` · `DeckBreachUU` ·
`bDeckIsAtEnd` · `CapsuleHalfHeightUU`.

`FSiegeLadderClimbStatics` — **6 constants + 9 functions** (the spec's list named 10 items; the
actual surface is 15, and `MinClimbLineUU` / `ArrivalToleranceUU` / `TimeoutScale` /
`MinTimeoutSeconds` / `DeckBreachCapsuleHalfHeights` were **not** on it):

| member | new location | signature / value — ⛔ unchanged |
|---|---|---|
| `MinClimbLineUU` | `.h:177` | `static constexpr float = 1.f` |
| `ArrivalToleranceUU` | `.h:188` | `static constexpr float = 16.f` |
| `TimeoutScale` | `.h:196` | `static constexpr float = 4.f` |
| `MinTimeoutSeconds` | `.h:199` | `static constexpr float = 1.f` |
| `MinClimbSpeedUU` | `.h:202` | `static constexpr float = 1.f` |
| `DeckBreachCapsuleHalfHeights` | `.h:240` | `static constexpr float = 3.f` |
| `IsAttackAllowed` | `.h:255` (inline) | `static bool (bool bCanEverAttack, bool bClimbing)` |
| `WantsActorTick` | `.h:274` (inline) | `static bool (bool bLungeActive, bool bClimbActive)` |
| `CanBegin` | `.h:283` / `.cpp:24` | `static bool (const FSiegeLadderClimbState&, bool bDead, bool bAIFrozen, bool bSpellFrozen, const FVector& FromWorld, const FVector& ToWorld)` |
| `Begin` | `.h:298` / `.cpp:46` | `static bool (FSiegeLadderClimbState&, bool bDead, bool bAIFrozen, bool bSpellFrozen, const FVector& FromWorld, const FVector& ToWorld, float ClimbSpeedUU, float CapsuleHalfHeightUU)` |
| `ClimbDirection` | `.h:302` / `.cpp:102` | `static FVector (const FSiegeLadderClimbState&)` |
| `ShouldSweep` | `.h:322` / `.cpp:117` | `static bool (const FSiegeLadderClimbState&, const FVector& CurrentWorld)` |
| `ArrivalTarget` | `.h:329` / `.cpp:110` | `static FVector (const FSiegeLadderClimbState&)` |
| `Advance` | `.h:339` / `.cpp:133` | `static bool (FSiegeLadderClimbState&, const FVector& CurrentWorld, float DeltaSeconds, bool& bOutReachedTop, bool& bOutTimedOut)` |
| `End` | `.h:350` / `.cpp:175` | `static bool (FSiegeLadderClimbState&)` |

## 2 · ⛔ WHAT DID **NOT** MOVE, AND WHY EACH STAYED — please scrutinise these three rulings

| stayed in `SummonedUnit.h` | why |
|---|---|
| ⛔ **`ESiegeLadderExit`** (`SummonedUnit.h:48-87`) | **(a)** ⛔ **No function in the lifted module takes a reason** — the teardown is REASON-AGNOSTIC by its own pinned comment, so the enum is not part of the rules module. **(b)** Its enumerators are **`ASummonedUnit` driver vocabulary** (`AbortLadderClimb` · `FreezeAI` · `ApplyFreeze` · `ASummonedUnit::EndPlay`) and its only consumers are `ASummonedUnit::EndLadderClimb` + the eight-exits test. **(c)** ⭐⭐ **`CONTACT-§3.1` requires the hero's TEN exits to be enumerated FRESH and ⛔ never mapped across the unit's eight** — publishing an eight-value enum on the shared header is an invitation to do exactly what the law forbids. |
| `FSiegeLadderClimbEnded` delegate | ⛔ **Signature PINNED character-for-character in `TOWER-§8.4(B)`** and it names `ASummonedUnit*`. Moving it would drag `class ASummonedUnit;` into the pure header. ⚠️ `CONTACT-§4.4`/`§8` already schedule its widening as an explicit **LAW AMENDMENT** owned by TASK-777 — ⛔ not by this move. |
| the whole `ASummonedUnit` driver | `CONTACT-§2` (3): behaviourally untouched. `LadderClimb` stays a by-value member (`SummonedUnit.h`), ⛔ no component, ⛔ no base class, ⛔ no registry, ⛔ no subsystem, ⛔ no singleton. |

## 3 · FILES RE-POINTED — FOUND BY GREP, ⛔ NOT ASSUMED

Grepped every `FSiegeLadderClimb*` reference in `Source/` and every file that includes
`SummonedUnit.h` (27 files).

- ✅ **`Tests/SiegeLadderClimbTest.cpp`** — the **one include line** (line 10). ⛔ Nothing else.
- ✅ **`SummonedUnit.cpp`** — explicit include added (the `SiegeStuckStatics.h:43` IWYU precedent in
  this very file: *"also reached via SummonedUnit.h … explicit per IWYU"*).
- ✅ **`SummonedUnit.h`** — include added (`LadderClimb` is a by-value member ⇒ complete type).
  Placed **before** `SummonedUnit.generated.h`, which stays last.
- ⛔ **`Tests/SiegeStuckStaticsTest.cpp`** — the spec's named candidate: **REFUTED.** Zero
  `FSiegeLadderClimb*` references. ⛔ Not touched.
- ⛔ **`HeroCharacter.h:85`** and **`Tests/SiegeRecallTest.cpp:59`** name `FSiegeLadderClimbStatics`
  **inside prose comments only** — no code dependency, and both are outside this task's fence.
  ⛔ Not touched (`HeroCharacter.{h,cpp}` is TASK-778's, even for an include).
- ⛔ **`ClimbableTower.{h,cpp}`**, `Castle.h`, `MinerUnit.h`, `SorcererUnit.h` + 20 others include
  `SummonedUnit.h` for the **actor**, and any that need the types get them transitively.
  ⛔ Not touched (`ClimbableTower` is TASK-777's).

## 4 · ⛔⛔ THE SIX SAFETY PROPERTIES — RE-VERIFIED AFTER THE MOVE, ⛔ NOT ASSUMED FREE

Every line below was re-grepped in the **post-move** tree; the line numbers are the new ones.

### (1) THE EIGHT EXITS THROUGH ONE IDEMPOTENT TEARDOWN — ✅ all 8 present, all restore the mode

| exit | site (post-move) |
|---|---|
| 1 Arrival | `SummonedUnit.cpp:3812` (with Timeout, the declared ninth) |
| 2 Abort | `SummonedUnit.cpp:3732` (`AbortLadderClimb`) |
| 3 NewOrder | `SummonedUnit.cpp:2097` |
| 4 Death | `SummonedUnit.cpp:4023` |
| 5 MatchEndFreeze | `SummonedUnit.cpp:674` |
| 6 SpellFreeze | `SummonedUnit.cpp:949` |
| 7 EndPlay | `SummonedUnit.cpp:615` |
| 8 Tower dies mid-climb | `ClimbableTower.cpp:170` → the same public `AbortLadderClimb` |

All eight funnel into **`ASummonedUnit::EndLadderClimb` (`SummonedUnit.cpp:3735`)**, whose FIRST
statement is the latch `FSiegeLadderClimbStatics::End` (**`SiegeLadderClimbStatics.cpp:175`**,
whole-struct reset at `:188`), and whose restore block
(`SummonedUnit.cpp:3755-3761`: `StopMovementImmediately` → exact `MaxFlySpeed` restore →
`SetDefaultMovementMode`) is unchanged. ⭐ `MOVE_Flying` ignores gravity; ⛔ no exit lost its restore.

### (2) THE DECK-BREACH WINDOW — ✅ all seven sub-properties intact

- **Z-resolved elevated end** (⭐ what makes DESCENT work): `SiegeLadderClimbStatics.cpp:82`
  — `State.bDeckIsAtEnd = (State.End.Z >= State.Start.Z);` ⛔ never argument order.
- **≤ 3 × capsule half-height, converted by the line's OWN slope**: `.cpp:87-91`
  — `RiseUU` → `BreachZUU = DeckBreachCapsuleHalfHeights * HalfHeight` → `× (LengthUU / RiseUU)`,
  clamped by `FMath::Min(…, State.LengthUU)`. ⛔ No hardcoded `sin(76°)`.
- **Continuous drive, ⛔ NOT a teleport**: `SummonedUnit.cpp:3857-3859` — one step of
  `LadderClimbSpeedUU × DeltaSeconds` along the same direction, same rate as the swept stretch.
- **Velocity zeroed each breach frame**: `SummonedUnit.cpp:3852-3855` — `StopMovementImmediately()`
  immediately before the non-swept step, so `PhysFlying` cannot fight it.
- **Snap only on a REAL arrival**: `SummonedUnit.cpp:3802-3804` — `if (bReachedTop)` guards the
  `SetActorLocation(ArrivalTarget(...))`; a timed-out climb drops from where it actually is.
- **Half-height READ from the capsule, ⛔ never a literal**: `SummonedUnit.cpp:3650-3652` —
  `GetCapsuleComponent()->GetScaledCapsuleHalfHeight()`, with `SiegeSpawn::DefaultCapsuleHalfHeight`
  only as the null-capsule fallback.
- **78% of the line still swept**: `ShouldSweep` (`.cpp:117-130`) returns **true** everywhere
  outside the window and measures distance from the **elevated** end, ⛔ not from `End`.

### (3) THE CAPSULE-CENTRE LIFT — ✅ intact (`SiegeLadderClimbStatics.cpp:64-70`)

`const FVector Lift(0.f, 0.f, HalfHeight);` applied to **both** ends, `CapsuleHalfHeightUU` cached
for the arrival snap. ⭐ Sockets are *surface* points; without this the unit arrives feet 88 uu below
the deck, buried in the slab. The lift is symmetric, so `LengthUU`, direction and watchdog budget
are unchanged by it — as the comment says, verbatim, in its new home.

### (4) TASK-760's SELF-HEAL — ✅ intact, and its declared non-coverage restated

- The re-assert: **`SummonedUnit.cpp:1405`** — `if (LadderClimb.bActive) { RefreshActorTickEnabled(); }`
  inside `UpdateState`, which rides **`StateTimerHandle`'s independent 0.25 s poll** (an
  `FTimerManager` entry on the WORLD, which knows nothing about `PrimaryActorTick`).
- The decision half: **`WantsActorTick`** (`SiegeLadderClimbStatics.h:274`), called from the ONE
  writer `RefreshActorTickEnabled` (`SummonedUnit.cpp:3621`). ⛔ Still never a bare
  `SetActorTickEnabled(true)`.
- ⚠️ **DECLARED NON-COVERAGE, CARRIED FORWARD UNCHANGED:** `AMinerUnit` seals
  `StateCheckInterval = 0.f` (**`MinerUnit.cpp:62`**) ⇒ **a miner has no poll and therefore no
  self-heal.** ⛔ This move did not change that and ⛔ did not silently "fix" it.

### (5) THE TRANSIENT DISARM — ✅ three guard points, and ⛔ still NOT via `CanEverAttack()`

`FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing())` at exactly **three**
sites, unchanged: **`SummonedUnit.cpp:1907`** (`EnterAttack` stands down) ·
**`:2675`** (`UpdateStateGrouped` acquires nothing) · **`:2962`** (`PerformAttack` refuses).
⛔ **No `CanEverAttack()` override was added anywhere.** The three that exist are untouched and are
the `const` class-identity seals: `SummonedUnit.h:567` (base `true`), `MinerUnit.h:354` (`false`),
`SorcererUnit.h:78` (`false`). ⭐ That separation is what keeps a Sorcerer's permanent inability
distinguishable from a Footman's 3.5-second climb.

### (6) THE PINNED SIGNATURE + ⛔ NO Z-ORDERING — ✅ intact

`bool ASummonedUnit::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)`
(`SummonedUnit.h:755` / `SummonedUnit.cpp:3632`), consumed at `ClimbableTower.cpp:441`.
`CanBegin`'s final term is still the **symmetric** `SizeSquared()` test
(**`SiegeLadderClimbStatics.cpp:43`**) with its verbatim comment: *"⛔ SYMMETRIC IN THE TWO POINTS —
a squared distance cannot express an ordering … the link is BothWays and a descent passes them the
other way round."* ⛔ **No ordering was enforced or implied**, and the field names stayed
`Start`/`End` (⛔ not `Foot`/`Top`) for precisely that reason (`TOWER-§8.4(B)`).

## 5 · ✅ SUITE DELTA: **EXACTLY ZERO — the total stays 248**

- `Tests/SiegeLadderClimbTest.cpp`: **`IMPLEMENT_SIMPLE_AUTOMATION_TEST` count = 14**, before and
  after. `git diff --numstat` on that file is **`1 0`** — one added line, **zero** removed, **zero**
  modified. ⛔ Not one test renamed, deleted, re-ordered or altered.
- ⛔ No test file was created, and no other test file was touched.

## 6 · ⚠️ FOR TASK-778 — HOW A **NON-UNIT** CALLER SATISFIES `bAIFrozen` / `bSpellFrozen`

⭐⭐ **These two parameters are UNIT VOCABULARY (`CONTACT-§3.2`). They are MAPPED, ⛔ never passed
through — and ⛔ NEVER typed `false` to make the call compile.**

`false` is **not** a neutral value there. It is an **assertion** that no match-end freeze and no
spell freeze can exist for this pawn. A pawn that starts a `MOVE_Flying` climb during either one is
the exact hang this whole feature is shaped around, and the mistake would be invisible in review
because the code compiles and reads correctly.

**The contract the header publishes:** `CanBegin`/`Begin` take **four caller-supplied truth terms**
— `bDead`, `bAIFrozen`, `bSpellFrozen`, plus the already-climbing test read from the caller's own
`FSiegeLadderClimbState`. Each is *"is this pawn currently forbidden to start scripted movement,
for this reason?"*. A non-unit caller must **find its own equivalent at file:line and name it in its
handoff**, or — if it genuinely has none — say so explicitly with the grep that proves it, so the
absence is a recorded finding rather than a typed literal. ⛔ **This task did NOT map them for the
hero and must not be read as having done so** — that is TASK-778's work and it is gated on TASK-775.

⚠️ A note TASK-778 is owed and this task cannot answer: `CONTACT-§3.4` also forbids starting a
climb while a **recall channel** runs. That is a **fifth** term with no parameter on this API. It
is satisfiable **caller-side** (refuse before calling `Begin`) with ⛔ no signature change — and
⛔ **I did not add a parameter for it**, because widening a pinned signature is not a pure move.

## 7 · ⚖️ DECLARED DEVIATIONS — three, all additive, all prose

1. ⭐ **The banner comment moved too** (`SummonedUnit.h:48-74` → the new header). It is the rules
   module's rationale and its last sentence reads *"why **End() below** is an exactly-once latch"* —
   leaving it behind would have broken that reference. Carried **verbatim**.
2. ⭐ **NEW prose was added, and ONLY as new prose:** a file-header block on
   `SiegeLadderClimbStatics.h` (provenance, the `CONTACT-§2` ruling, the `ESiegeLadderExit`
   decision, the frozen-terms warning for TASK-778, the `CONTACT-§9` M8 pointer) and an
   include-rationale comment on `SiegeLadderClimbStatics.cpp`. ⛔ **Not one moved line was edited to
   make room for it.**
3. ⚠️ **TWO MOVED COMMENT CLAUSES ARE NOW STALE, AND I DELIBERATELY DID ⛔ NOT EDIT THEM** — they
   are recorded as superseded in the new header's file-header block instead, so the moved text stays
   diffable character-for-character against the shipped version:
   - `FSiegeLadderClimbStatics`' doc says *"no new file (TASK-738 owns SummonedUnit.{h,cpp} only) —
     it shares this header with its consumer"*. TASK-738's fence was real; `CONTACT-§2` has since
     granted the file.
   - the `.cpp` banner says *"everything it DOES lives on the actor below"*. The split it describes
     is unchanged; only the word *"below"* moved.
   ⚖️ **If QA rules the other way, editing those two clauses is a one-line change each — but I judged
   a preserved-and-annotated original safer than a "tidied" one, because `CONTACT-§2` calls a
   changed comment a lost ruling.**

## 8 · 🚩 THINGS FOR QA TO SCRUTINISE HARDEST

1. ⭐⭐ **The `ESiegeLadderExit` stay-put ruling (§2 above).** It is the one judgement call in the
   task. If QA rules it must move, the mechanical cost is small — but please rule against
   `CONTACT-§3.1`'s *"enumerate fresh, never map across"*, not against tidiness.
2. **The two stale comment clauses (§7.3).** A deliberate non-edit, ⛔ not an oversight.
3. **The new pair's include hygiene.** `SiegeLadderClimbStatics.h` includes **only** `CoreMinimal.h`
   and has ⛔ **no `.generated.h`** (nothing in it is reflected — deliberate, so it cannot be
   replicated by accident). `SiegeLadderClimbStatics.cpp` includes **only its own header**.
   `UE_KINDA_SMALL_NUMBER`, `FMath` and `FVector` all arrive through `CoreMinimal`.
   ⚠️ **An engine include appearing in this pair later means the rules stopped being pure and the
   14 headless tests are lost.**
4. **UHT ordering:** the new include sits **before** `SummonedUnit.generated.h`, which is still the
   last include in `SummonedUnit.h`.
5. ⚠️ **⛔ NOT COMPILED** (`TASK-780` owns the one compile). ⛔ No editor, ⛔ no MCP, ⛔ no Git,
   ⛔ no `CONVENTIONS.md` edit. ⛔ `ClimbableTower.{h,cpp}` and `HeroCharacter.{h,cpp}` were not
   touched — ⛔ not even for an include.
