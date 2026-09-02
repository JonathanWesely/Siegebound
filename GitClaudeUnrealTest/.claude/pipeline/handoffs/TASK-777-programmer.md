# TASK-777 — THE CONTACT TRIGGER + THE WIDENED OCCUPANCY SLOT (gameplay-programmer)

**Status:** `ready-for-qa` · **Law:** `CONTACT-§4` (`§4.1`/`§4.2`/`§4.3`/`§4.4`) · `CONTACT-§8` · `CONTACT-§9` · `CONTACT-§10.1` · `TOWER-§8.4(B)` (2nd amendment) · `TOWER-§8.6`/`§8.7`/`§10 L-1` · `WR-§5` · `HIGH-§1` · `SC-§36` · `SHIP-§9c`
**⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git, ⛔ no `CONVENTIONS.md` edit, ⛔ no mesh, ⛔ no anim.**

---

## ⛔⛔ READ THIS FIRST — ONE DECLARED GAP THAT IS **NOT** IN MY FENCE AND BLOCKS THE FEATURE FROM RUNNING

**The contact trigger has ⛔ NO CALL SITE, and no task in this batch owns one.** This is `SC-§36`'s exact defect class (*"a correct mechanism that is never actually driven"*), raised the way `SC-§36` clause 3 requires — **on the board, not only in a source comment.**

- `CONTACT-§4.1` puts the radius and the test on the owning actor and says **"the pawn only ASKS"** (`WR-§5`). ⭐ **I measured the idiom rather than assuming it: `ACommanderNpc` ships `bCanEverTick = false` (`CommanderNpc.cpp:105`) with its own comment at `:102` reading *"the proximity gate is POLLED BY THE CALLER … through `IsPlayerInRange`"*, and the caller is `SiegePlayerController.cpp:4970`.** ⇒ the tower stays passive and the **pawn** polls.
- ⛔ **The unit's poll would live in `ASummonedUnit`, and my grant on that file is the delegate line + one base-list entry + its include, with "⛔ No body changes" written into it.** TASK-778 (hero) is blocked on `K-1` and its spec never mentions calling the tower either. ⇒ **nobody was told to write the one line.**
- ⛔⛔ **I did ⛔ NOT reach for it** (item (7): *"If you find a defect OUTSIDE this file, report it — ⛔ do NOT reach for it"*), **and I did ⛔ NOT give the tower a tick or a timer to route around it.** That would have been a double violation: `WR-§5`'s measured idiom, **and a shipped test that would go RED by name** — `Tests/SiegeClimbableTowerTest.cpp` test 10(c) errors on any reflected `AClimbableTower` member containing `Timer`/`Tick`/`Poll`/`Interval`, citing `TOWER-§8` (4).

**⭐ THE WHOLE FIX IS ONE LINE, and the surface is built and tested to receive it:**

```cpp
// ASummonedUnit::Tick (⛔ NOT the 0.25 s StateTimerHandle poll — see the cadence note below)
Tower->TryBeginContactClimb(this, DeltaSeconds);
```

- ⚠️ **It must ride `Tick`, ⛔ not the shipped 0.25 s `StateTimerHandle`:** a 0.35 s dwell sampled at 0.25 s is **1–2 samples**, so the dwell would quantise to 0.25/0.50 s and `K-C`'s latch (which needs to observe the pawn leaving the cone) would miss most departures.
- ⚠️ **It also needs a tower to ask.** `ASummonedUnit` holds ⛔ zero executable references to `AClimbableTower` today (`CONTACT-§1` `C-3` clause 1, confirmed). Finding the tower is the wiring task's other half; ⛔ I did not design it, because designing another task's file from here is how two tasks ship two mechanisms.
- 🙋 **FOR THE MANAGER:** this wants a numbered item in a real task (a unit-side sibling of TASK-778, or a granted item added to TASK-777's successor). **⛔ Without it, TASK-780's PIE rows and Jonathan's TASK-781 sitting rows ① ③ ④ cannot pass** — the mechanism is correct, tested, and never driven.

---

## 1 · THE TRIGGER'S SHAPE — proximity + cone + dwell, with the numbers and why

**⛔ It is ⛔ NOT literally "contact", and that is measured:** the ladder has **ZERO collision** (TASK-737 — no hull anywhere over `LadderFoot`, nearest surface 116 uu). A pawn walking at it passes **through** and is stopped ~300 uu later by the tower body ⇒ ⛔ an overlap-on-blocking-hit trigger would ⛔ **never** fire, ⛔ not by tuning, ⛔ structurally.

| Term | Rule | The number, and what it buys |
|---|---|---|
| **PROXIMITY** | 2D (XY) distance to the **nearer endpoint** ≤ `LadderContactRadiusUU` | **150 uu** (`K-5`, declared invented). ⛔ 2D because the endpoint is a **surface** point and the pawn's origin is its capsule **centre**; a 3D test would need `§8.5a` clause 6's half-height lift just to be satisfiable, and would then be wrong for any pawn with a different capsule. |
| **INTENT** | horizontal **movement direction** · horizontal **pawn→endpoint** direction ≥ `LadderContactIntentCos` | **0.5 = a 60° half-cone** (`K-5`). ⭐ Reads **direction**, ⛔ never a key — the only shape that behaves identically for a player-driven and an AI-driven pawn (and `RECALL-§2`/`KBD-§` forbid a hardcoded key anyway). |
| **DWELL** | both above hold **continuously, at one endpoint** | **0.35 s** (`K-5`). ⭐⭐ **The load-bearing term, and it is load-bearing for the UNIT half.** |

**⭐ THE ARITHMETIC THAT SHOWS THE THREE COOPERATE — written down because "it felt right" is not reviewable.** A pawn crossing the disc in a straight line at speed `v`, perpendicular offset `d`: with the cone measured **pawn → endpoint**, the endpoint is inside a 60° half-cone only while the pawn is still `d / tan 60° = d/1.732` short of closest approach ⇒ it can accumulate `(sqrt(R² − d²) − d/1.732) / v` seconds.

| `d` | in-cone time at `R = 150`, `v = 300` | vs the 0.35 s dwell |
|---|---|---|
| 0 (walking straight at it) | **0.500 s** | ✅ ADMITTED — that *is* the intent |
| 50 | 0.375 s | ✅ admitted (marginal) |
| 75 | 0.289 s | ⛔ refused |
| 100 | **0.180 s** | ⛔ REFUSED — marching past is not walking into it |

⇒ the trigger admits a pawn aimed within **~60 uu** of the ladder and refuses everything marching past outside that. **Both the `d = 0` and `d = 100` rows are asserted, with the pawn actually walking** (test 11(f)).

**⚠️⚠️ AND THIS IS WHERE I DIVERGED FROM MY OWN FIRST INSTINCT AND CHECKED IT WITH ARITHMETIC — worth QA's eye.** I initially thought the cone should be measured against the **climb line's own horizontal direction** (constant, never degenerate, never flips when the pawn overshoots the socket). **⛔ That would have been WRONG and would have destroyed the dwell's whole purpose:** a constant direction is satisfied for the **entire chord**, `2·sqrt(R² − d²)/v` = **0.745 s at `d = 100`** ⇒ every pawn crossing the disc eastward gets abducted. ⭐ **The bearing MUST swing as the pawn passes; that swing IS the discrimination.** The law's literal term is right and it ships unchanged.

**⭐ BOTH ENDPOINTS ARM, RESOLVED BY Z** (`K-C`, mirroring `§8.5a` clause 1) ⇒ walking into the ladder **on the deck descends**. ⛔ **Z and ⛔ never 2D**, and that is arithmetic: the endpoints are **1,200 uu apart in Z but only 300 uu apart in XY**, so at a 150 uu radius the two XY discs are **tangent** and a pawn on the ground can fall inside the TOP's disc. A tie reads as the FOOT — the same tie-break `HandleLadderLinkReached` already applies.

**⛔⛔ `K-C`'s RE-ARM LATCH SHIPS AND IS MANDATORY.** A pawn finishing a **descent** stands at the foot, inside the radius, still supplying the input that brought it there ⇒ ⛔ instant re-climb, yo-yo. The latch is armed in **`HandleLadderClimbEnded`** — ⭐ deliberately, because that is the **one** place every climb ends, so a nav-link ascent and a contact ascent are protected by the same line. It resolves the endpoint from **where the pawn is** (the same Z rule that admitted it), so it does ⛔ **not** read `bReachedTop` and the shipped *"`bReachedTop` drives NOTHING here"* property survives intact. It clears on **either** re-arm condition (left the radius **or** out of the cone).

**⭐ ONE DERIVED CONSTANT THAT IS ⛔ NOT A FOURTH `K-5` TUNABLE:** `FSiegeLadderContactStatics::MinContactSpeedUU = 1.f`. Below 1 uu/s a pawn covers **< 0.35 uu across the whole dwell** — a hundredth of its own capsule radius. It keeps braking residue and depenetration nudges out of the intent reading. The `MinClimbLineUU` idiom: a math guard, ⛔ not a policy. **Declared here so QA does not read it as a smuggled feel number.**

**⛔ NO PER-FRAME LOGGING.** `TryBeginContactClimb` runs out of a pawn's tick; only the two **events** log (an admission, and a `BeginLadderClimb` that declined).

---

## 2 · THE WIDENED DELEGATE + BOTH GRANT-SITE EDITS

**Implemented exactly as ruled — ⛔ not re-proposed, ⛔ not re-derived:**

```cpp
// SummonedUnit.h:114 (was :100 — see the cite table in §6)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSiegeLadderClimbEnded, ACharacter*, Climber, bool, bReachedTop);
```

- ✅ Type name **`FSiegeLadderClimbEnded` UNCHANGED** (⛔ `FOnLadderClimbEnded` was refused and is ⛔ not present anywhere).
- ✅ Param name `Unit` → `Climber` only (names are ⛔ not part of a function's type — this file's own `FromWorld`/`ToWorld` precedent).
- ✅ `ActiveClimber` → `TWeakObjectPtr<ACharacter>` (`ClimbableTower.h:707`) · `ReleaseClimber(ACharacter*)` (`:650`) · `HandleLadderClimbEnded(ACharacter*, bool)` (`:647`).
- ✅ **`SummonedUnit.cpp:3781`'s `OnLadderClimbEnded.Broadcast(this, bReachedTop)` is UNTOUCHED** and compiles through the implicit conversion. ⭐ Zero edits in the broadcast path, exactly as ruled.
- ⭐ **Free property, banked:** `ASiegeGhostPawn : APawn` is not an `ACharacter` ⇒ **"a ghost can never climb" is now true BY TYPE.**

**⭐ THE CAPABILITY SEAM — NEW `Source/GitClaudeUnrealTest/Siegebound/LadderClimber.h`.** `UINTERFACE(MinimalAPI, NotBlueprintable)` `ULadderClimber` / `ILadderClimber`, surface **`AbortLadderClimb()` + `IsClimbing() const` and ⛔ nothing else.** The shipped `TeamId.h` / `HealthBarProvider.h` pattern.
- ⛔ **It is NOT in `SiegeLadderClimbStatics.h`** — that header's `:10` says *"⛔ No .generated.h: nothing in this pair is reflected, deliberately"*, and a `UINTERFACE` would force one in.
- ⛔ **Methods are plain virtuals, ⛔ not `UFUNCTION`s** (the `IHealthBarProvider` half of the precedent, ⛔ not the `ITeamAgent` half): both implementers already declare these names as their own `UFUNCTION`s, and a reflected interface method would have to agree specifier-for-specifier or fail in UHT for a reason no reader would connect to that file. ⭐ Nothing here needs Blueprint.
- ✅ Consumed at **`ClimbableTower.cpp:202-210`** — `EndPlay` aborts through `Cast<ILadderClimber>`, ⛔ never through a class list.

### The narrow, named cross-fence edits — ⛔⛔ ATTRIBUTED TO **TASK-777**, ⛔ NOT TASK-776

⭐ **TASK-776's "suite delta EXACTLY ZERO" applies to ITS OWN diff and is already discharged; it stays true.** `TASK-779` item (5a) already carries this attribution — this section is the programmer's half of it.

| Site | What changed | Why |
|---|---|---|
| `SummonedUnit.h:114` (was `:100`) | the delegate declaration — first param `ASummonedUnit*` → `ACharacter*`, `Unit` → `Climber` | the ruled widening |
| `SummonedUnit.h:85-113` | the declaration's **own doc comment**, extended with the amendment | ⚠️ **DECLARED DEVIATION — see §5 (D-1).** Leaving *"SIGNATURE PINNED CHARACTER-FOR-CHARACTER"* beside a changed signature is the `CONTACT-§10.1` defect class |
| `SummonedUnit.h:10` | `#include "Siegebound/LadderClimber.h"` | `ILadderClimber` is a **base** of the UCLASS ⇒ complete type required |
| `SummonedUnit.h:196` | ONE base-list entry: `, public ILadderClimber` | ⛔ no body changes anywhere in `SummonedUnit.{h,cpp}` |
| `Tests/SiegeLadderClimbTest.cpp:~907-908` | `PropertyClass == ASummonedUnit::StaticClass()` → **`ACharacter::StaticClass()`** | ⭐⭐ **That test WENT RED on the widening, which is the evidence the pinning mechanism works.** ⛔ It was **retargeted**, ⛔ NOT weakened to a null check — **an `APawn*` (the shape ruled against) and an `ASummonedUnit*` (what shipped) BOTH still fail that line** |
| `Tests/SiegeLadderClimbTest.cpp:894` | message text → `(ACharacter* Climber, bool bReachedTop)` | keeps the failure message honest |
| `Tests/SiegeLadderClimbTest.cpp:~85` | **comment only** — the refuted *"the project never calls `InitCapsuleSize`"* | ⛔ It is FALSE: `GitClaudeUnrealTestCharacter.cpp:18` calls `InitCapsuleSize(42.f, 96.0f)` on the hero's own base. **34 is the NAV AGENT radius** (`DefaultEngine.ini:290`), ⛔ not a capsule radius — two different 34s. ⛔ **What that test ASSERTS is unchanged**, because 88 is still the right scenario for the unit these tests are about |

⭐ **`SiegeLadderClimbTest.cpp` keeps all 14 tests. ⛔ Not one renamed, deleted, re-ordered or added.**

---

## 3 · HOW I AVOIDED A NINTH EXIT (and preserved everything I was told to preserve)

**⭐ The contact path creates ⛔ NO new way for a climb to end. It has exactly one job — deciding WHO starts and BETWEEN WHICH TWO POINTS — and it hands the traversal to the same shipped state machine the nav link does.** Concretely:

1. **Every exit still routes through `ASummonedUnit`'s own exactly-once latch.** The tower still sets ⛔ no movement mode, still interpolates nothing, still teleports nothing. `TryBeginContactClimb`'s only mutation of a climber is `BeginLadderClimb(From, To)` — the identical call `HandleLadderLinkReached` makes.
2. **The `BeginLadderClimb`-refused branch is byte-for-byte the link path's:** *"returns false and changes NOTHING"* ⇒ no climb started ⇒ ⛔ no movement mode to restore ⇒ undo our own binding via `ReleaseClimber` and stop. ⛔ No new teardown was written.
3. **⛔ I refused to ship a claimed-but-unstarted slot.** A climber the tower cannot start (a future `AHeroCharacter`) is **refused** with the shipped `NotASummonedUnit` verdict and ⛔ **nothing is claimed**. A slot claimed for a climber that then failed to start would **brick the ladder for the rest of the match** — a hazard shipped for a later task to trip over. ⭐ The refusal is a **loud, compile-visible seam** instead.
4. **`ActiveClimberPathComp` is `Reset()` on a contact entry, ⛔ not set.** A contact entry is ⛔ not a path-following handshake — nothing handed the pawn to the link. `ReleaseClimber`'s `ResumeAgentPathFollowing` then re-derives a component and calls `FinishUsingCustomLink`, which **no-ops unless that component is actually holding THIS link** (`PathFollowingComponent.cpp:1481-1494`, the shipped comment). ⇒ harmless for a contact climber, correct for one that walked in off a path.
5. **⭐ ONE STEERING AUTHORITY, verified at the source rather than assumed.** A contact climb can start while a `MoveTo` is still in flight — the link path never could. `BeginLadderClimb`'s **step (4)** already calls `AIController::StopMovement()` (*"Path following must let go before we drive the capsule … the NAV-§3 no-double-driver law"*). ⇒ ⛔ no double driver, and ⛔ no new code was needed to get it.

### ⛔ PRESERVED, EXACTLY AS INSTRUCTED

| Thing | State |
|---|---|
| the **ninth exit** you closed — `bLadderOccupied = ActiveClimber.IsValid()`, **no identity comparison** | ✅ **UNTOUCHED** (`ClimbableTower.cpp:419`). ⛔ I did not add `&& IsClimbing()` or any identity term, on either path |
| `EvaluateLadderEntry`'s **verdicts and precedence** | ✅ **BYTE-IDENTICAL** — signature, enum, comments, `if` order. ⭐ The contact path **calls it**, ⛔ it does not re-implement it. There is ⛔ no second entry gate |
| sockets via **`GetSocketTransform(…, RTS_Actor)`** | ✅ **UNTOUCHED** (`ResolveLadderSocketRelative`). The contact path never reads a socket at all — it reads the **link's** armed endpoints, the same source `HandleLadderLinkReached` reads. ⭐ Three consumers, one source |
| `CanTeamAscend` as the **live rule** | ✅ **SECOND CALLER, ⛔ never an exception.** `Climb` is returned from **exactly one place**, only after `EvaluateLadderEntry` returned `Climb`. ⛔ No hero exemption. Asserted (test 12(a), both team directions) |
| the optional **fail-open** pathfinding layer | ✅ **UNTOUCHED** (`ClimbableTower.cpp:274-307`) |
| **the nav link** | ✅ **KEPT, ⛔ not deleted, disabled or bypassed.** The link plans a route; contact starts a body. ⛔ They cannot double-fire, and it is **asserted, ⛔ not prose**: test 12(b) (one gate, one occupancy slot) + test 12(c) (`CanBegin` refuses an already-climbing pawn — the second, independent belt) |
| this class **never ticks, arms no timer** | ✅ **SURVIVED THE DWELL**, which was the hard one. `WR-§5`, measured |

---

## 4 · TESTS — what each would catch · **SUITE DELTA: +2**

**⭐ TWO new tests in `Tests/SiegeClimbableTowerTest.cpp` (11 and 12). ⛔ No test added, removed or renamed anywhere else. Every assertion below can FAIL** (`SHIP-§9c`), and each carries a self-check that fires if the instrument has gone blind.

### Test 11 — `Siegebound.ClimbableTower.ContactNeedsProximityIntentAndDwellAndRefusesAPasserBy`

| Row | Catches |
|---|---|
| self-check ×2 | the predicate returning one value for everything; the "admitting" case not actually admitting ⇒ every refusal below refusing nothing |
| (a) inside the radius, moving **away** | a cone term deleted or inverted ⇒ pawns abducted while walking the other way. Also asserts the dwell clock is held at **zero**, so it cannot bank credit |
| (b) toward it for **0.30 s** ⇒ `Dwelling`, then **+0.10 s** ⇒ `Climb` | a missing or mis-compared dwell. The two halves differ **only** in elapsed time |
| (c) 0.30 s + **one frame away** + 0.30 s ⇒ still `Dwelling` | a dwell that survives interruption ⇒ credit accumulated by loitering |
| (d) standing still · creeping at 0.5 uu/s | the `MinContactSpeedUU` floor removed ⇒ braking residue or a depenetration nudge read as intent |
| (e) 300 uu out, walking straight in | the radius term removed ⇒ intent reaching across the map |
| (f) ⭐⭐ **the pawn actually WALKS**: `d = 0` admitted · `d = 100` refused · **and the two must disagree** | **the abduction defect `CONTACT-§4.1` names.** Both expectations are **derived** (0.500 s vs 0.180 s of in-cone time against a 0.35 s dwell), ⛔ not transcribed. Fails if the dwell is removed, the radius widened, or the cone changed to the constant-direction shape I rejected |
| (g) descend from the deck · `bAscending == false` · **a ground pawn under the deck socket resolves to the FOOT and is `TooFar`** | ⭐ **an XY endpoint resolution instead of a Z one** — that pawn would be **0 uu from the top** and would climb. Also catches a driver handed the line backwards |
| (h) `Disarm` then press in ⇒ `Disarmed`, dwell stays 0 | ⛔ **the yo-yo**: instant re-ascent after a descent |
| (i) latch clears on **leaving the radius** AND on **steering out of the cone** | the opposite defect — a pawn permanently unable to use a ladder, which nothing logs |

### Test 12 — `Siegebound.ClimbableTower.ContactEntryStillRefusesEnemiesAndASecondClimber`

| Row | Catches |
|---|---|
| self-check ×2 | a composed gate that is a constant |
| (a) ⛔ **an ENEMY satisfying all three terms** ⇒ `WrongTeam`, both team directions | ⚠️⚠️ **the silent back door around `T-3`** — a Jonathan ruling overturned from inside a task about something else, which ⛔ no reviewer would have a reason to look for |
| (b) own team, all terms, ladder **occupied** ⇒ `LadderBusy` | `L-1` broken by the new path; ⭐ also half of `K-E`'s no-double-fire claim |
| (c) `CanBegin` refuses an already-active state (+ admits an inactive one on the same line) | the **second, independent belt** behind no-double-fire |
| (d) a non-`ASummonedUnit` at a free own-team ladder ⇒ `NotASummonedUnit` | identity precedence lost; ⭐ this is the verdict `AHeroCharacter` will land on |
| (e) out-of-range **enemy** at a **busy** ladder ⇒ `TooFar` | the evaluation **order** (contact terms first) silently flipped |
| (f) ⭐⭐ **latch armed, then re-armed entirely while the ladder is `LadderBusy`, then climbs** | ⛔⛔ **the exact defect the ordering prevents.** Under the "tidier" gate-first order the pawn is short-circuited on `LadderBusy` every frame, never observed leaving the cone, and carries the latch **for the rest of the match** |
| (g) the three tunables resolve as `EditDefaultsOnly` floats at `K-5`'s 150 / 0.5 / 0.35 · **and `ASummonedUnit` declares no `LadderContact*` member** | a retune drifting away from the law silently (expectations typed **from the law**, ⛔ never read back off the class); and a **second copy** of the tuning on the pawn, which `WR-§5` exists to prevent |

### Also strengthened (⛔ not a new test — test 10, my own file)

**Test 10(b) asserted only the handler's ARITY (2), which is unchanged before and after the amendment ⇒ it would have slept straight through it.** It now also asserts the **first parameter TYPE is `ACharacter`**, with a self-check that the parameter walk actually reached a parameter. ⛔ Without that row the handler could drift back to `ASummonedUnit*` (or to the ruled-against `APawn*`) with the test still green while `AddUniqueDynamic` silently refused to bind — leaving `ActiveClimber` permanently occupied after the first climb of the match. **Comments at the old `:1061`/`:1067` refreshed to `(ACharacter*, bool)`.**

---

## 5 · DECLARED DEVIATIONS

| # | Deviation | Reasoning |
|---|---|---|
| **D-1** | I edited **`SummonedUnit.h:85-113`** — the widened declaration's **own doc comment** — beyond the bare grant line | The comment read *"⛔ SIGNATURE PINNED CHARACTER-FOR-CHARACTER IN TOWER-§8.4(B)"* **directly above a signature I was ordered to change**. Leaving it is the `CONTACT-§10.1` defect class (a record that looks authoritative and is false), and `CONTACT-§4.4` says the amendment *"lands as a ⚖️ LAW AMENDMENT … ⛔ never silently"*. ⛔ **No code, no `UPROPERTY`, no body, no other declaration touched.** Flagged for an explicit ruling |
| **D-2** | `FSiegeLadderContactStatics` went into **`SiegeLadderClimbStatics.{h,cpp}`** (TASK-776's pair), which my dispatch's compressed fence did not list | The **board** names that location three times — item (1) (*"in TASK-776's new pair"*), item (9)'s fence, and `CONTACT-§8`'s file map (*"in the **same** new pair"*). The dispatch summary is lossy elsewhere too (it says "two narrow grants" and lists three, and omits the `ILadderClimber` base-list grant the board explicitly gives). ⇒ **I followed the board.** ⭐ **Mitigation for TASK-779 item (2)'s byte-diff:** the addition is **appended below a loud attribution banner** in both files, so TASK-776's moved region stays **contiguous from the top** and diffs character-for-character. ⛔ Nothing above the banner changed, and ⛔ nothing reflected was added, so that pair's deliberate no-`.generated.h` property is intact |
| **D-3** | `MinContactSpeedUU = 1.f` is a **fourth** number in the contact code | ⛔ **Not a fourth `K-5` tunable** — a derived degeneracy floor (private `constexpr`, ⛔ not `EditDefaultsOnly`, ⛔ not tunable). Derivation in §1 |
| **D-4** | The `ELadderEntryVerdict::NotASummonedUnit` verdict name is now **narrow** for the contact path | ⛔ **I did NOT rename it** — the brief says preserve `EvaluateLadderEntry`'s verdicts. Today the name is **exact** (`ASummonedUnit` is the only climber class). ⚠️ When TASK-778 widens the identity term, that verdict's **name** will need to widen with it — **boarded here rather than left to be discovered** |
| **D-5** | Two `Cast<ASummonedUnit>` remain in `ClimbableTower.cpp` (the `AddUniqueDynamic`/`RemoveDynamic` pair) | ⛔ **Not the refused class-list branch pair.** It is the only way to reach `OnLadderClimbEnded`, which is an `ASummonedUnit` member and nothing else's; `CONTACT-§4.4` closed `ILadderClimber` at Abort + IsClimbing **deliberately**. The completion lane is per-class by the same `CONTACT-§2` ruling that keeps the driver per-class |
| **D-6** | `SummonedUnit.h:19`'s comment (`class ASummonedUnit; // … named by FSiegeLadderClimbEnded below`) is now **stale** and I left it | ⛔ Outside my grant ("⛔ Nothing else in `SummonedUnit.{h,cpp}`"). **Declared per `CONTACT-§10.1` so the manager can board a one-line comment fix** rather than have it rot |

---

## 6 · ⚠️ CITES I INVALIDATED (`CONTACT-§10.1`'s duty — re-grepped, ⛔ never arithmetic)

| Cite as written in law / on the board | ⭐ NEW value |
|---|---|
| `SummonedUnit.h:100` — the delegate | **`SummonedUnit.h:114`** |
| `SummonedUnit.h:792` — the `UPROPERTY` | **`:806`** |
| `SummonedUnit.h:182` — the class declaration | **`:196`** |
| `SummonedUnit.h` `BeginLadderClimb` `:755` · `AbortLadderClimb` `:767` · `IsClimbing` `:779` | **`:769` · `:781` · `:793`** |
| `ClimbableTower.h:487` — `ActiveClimber` | **`ClimbableTower.h:707`** |
| `ClimbableTower.h:448` / `:451` — handler / release | **`:647` · `:650`** |
| `ClimbableTower.cpp:168` — `EndPlay`'s abort | **`ClimbableTower.cpp:202`** |
| `ClimbableTower.cpp:189-199` — `CanTeamAscend` | **`:226-236`** |
| `ClimbableTower.cpp:237-270` — `ShouldLinkAllowPathfinding` (fails open) | **`:274-307`** |
| `ClimbableTower.cpp:331-342` — ⭐ **the socket Warning TASK-780 item (3a)(iv) greps for** | **`:368-379`** |
| `ClimbableTower.cpp:366` — `HandleLadderLinkReached` | **`:403`** |
| `ClimbableTower.cpp:382` — the busy test | **`:419`** |
| `ClimbableTower.cpp:221-223` — the interpenetration note | **`:258-260`** |
| `Tests/SiegeClimbableTowerTest.cpp:1061`/`:1067` | **`:1074`/`:1080` region** (test 10(b), now also asserting the param type) |
| `Tests/SiegeLadderClimbTest.cpp:907-908` | **`:920-921` region** |

⛔ **`SummonedUnit.cpp` was ⛔ NOT edited — every `SummonedUnit.cpp` cite is unchanged**, including `:3781`'s broadcast and `:608-615`'s `EndPlay` belt.

---

## 7 · FILES TOUCHED

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/LadderClimber.h` | ⭐ **NEW** — `ULadderClimber` / `ILadderClimber`, 2 methods, ⛔ nothing else (85 lines) |
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` | widened slot + handler + release · 3 `EditDefaultsOnly` tunables · `ELadderContactVerdict` · `EvaluateContactEntry` (pure) · `TryBeginContactClimb` · the contact table + 2 private helpers |
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp` | `EndPlay` aborts via `ILadderClimber` · widened handler/release · the `K-C` latch in `HandleLadderClimbEnded` · the contact implementation (`:611-815`) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.h/.cpp` | ⭐ **APPENDED below an attribution banner** — `FSiegeLadderContactState`, `ESiegeLadderContactVerdict`, `FSiegeLadderContactStatics` (`IsAtTopEndpoint` / `WantsToClimb` / `Disarm`). ⛔ Nothing above the banner changed |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | ⚠️ **GRANT** — delegate line + its doc comment (D-1) · one include · one base-list entry. ⛔ No body changes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` | tests **11** and **12** (+2) · test 10(b) strengthened · fixture helpers · 2 includes |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp` | ⚠️ **GRANT** — `:907-908` retargeted to `ACharacter` · `:894` message · `:85` comment. **Still 14 tests** |

**Assets referenced:** ⛔ **NONE.** `CONTACT-§0`'s happiest fence holds — ⛔ no mesh, ⛔ no texture, ⛔ no socket, ⛔ no clip, ⛔ no material. `SM_WatchTower`'s two socket **names** are read through the shipped, unchanged `ResolveLadderSocketRelative`.

**⛔ M8 (`CONTACT-§9`), declared and ⛔ not copied from another batch's boilerplate:** ⛔ **no replicated property and ⛔ no RPC is authored.** ⚠️ A climb *is* authority-relevant movement state on a player-possessed pawn, so the shape is **declared in a header comment and ⛔ not built** (`ACC-§8`'s reserved-not-authored discipline). ⭐ Both new state types (`FSiegeLadderContactState`, `LadderContacts`) are **deliberately unreflected**, so that declaration is a **structural** guarantee rather than a promise that survives only until the next refactor.

---

## 7a · ⭐ SELF-REVIEW BEFORE HANDOFF — ⛔ I CANNOT COMPILE, SO THE READ WAS THE INSTRUMENT

I ran an **independent, adversarial compile-risk and logic read** over all nine touched files (fresh from disk, ⛔ not from memory), including a brace/comment-state balance pass. **It found two things and both are FIXED:**

1. ⛔ **A REAL COMPILE ERROR:** `FindOrAddContactState(const ACharacter*)` **stores** its argument into a `TWeakObjectPtr<ACharacter>`, and `const ACharacter*` is ⛔ not convertible to `ACharacter*` ⇒ no viable `operator=`. ✅ **Fixed — the parameter is now non-`const`, with the reason written at the declaration.** `ForgetContact` keeps its `const` because it only compares.
2. ⚠️ **AN OVERSTATED ASSERTION MESSAGE** at test 12(f): it claimed the re-arm frames "were refused as `LadderBusy`". They actually return `Disarmed` then `NotHeadingIn` — **because the contact terms short-circuit before the entry gate, which is the very property under test.** ✅ **Re-worded to say what is observed and why it would go red under the gate-first ordering.** ⛔ The assertion itself was already valid and is unchanged.

**Independently re-derived and confirmed:** every arithmetic expectation in tests 11(b)/(c)/(f)/(g) (including the float margins at 60 Hz — 0.30000002 vs 0.34999999, and the `d = 0` / `d = 100` step-by-step walks), the UHT interface shape against `TeamId.h`/`HealthBarProvider.h`, the implicit override of `ILadderClimber`'s pure virtuals by `ASummonedUnit`'s existing non-`virtual` `UFUNCTION`s, the UE 5.8 API spellings (`AddDefaulted_GetRef`, `RemoveAtSwap(int32)`, `DistSquared2D`/`GetSafeNormal2D`/`Size2D`), and **both name scans** (test 4 and test 10(c)) against every new member. **Comment-block balance is `depth = 0`, `stray_close = 0` in all nine files.**

⚠️ **AND IT CONFIRMED THE GAP AT THE TOP OF THIS FILE BY GREP RATHER THAN BY ASSUMPTION: `TryBeginContactClimb` has ⛔ ZERO callers anywhere in `Source/`.**

---

## 8 · ⚠️ WHAT QA SHOULD SCRUTINISE HARDEST

1. **⛔ THE DECLARED GAP AT THE TOP OF THIS FILE.** The mechanism is correct and has **no caller**. `SC-§36` says the obligation goes on the **board**. ⛔ Please do not pass this batch as "the contact climb ships" without that one line being boarded.
2. **⭐ The evaluation ORDER** (contact terms → entry gate) and its stated reason. It looks backwards. Test 12(f) is the row that proves it is not, and it is the row to read first.
3. **`D-1` and `D-2`** — both are me choosing the board/law over a narrower reading of my dispatch. Rule on them explicitly.
4. **`CONTACT-§4.3`'s back door.** `CanTeamAscend` is reached only through `EvaluateLadderEntry`, from **one** return path. Test 12(a) covers both team directions.
5. **✅ THE BLUEPRINT RISK — RAISED, THEN ⭐ MEASURED AND LARGELY DISCHARGED (⛔ not left as a worry).** `OnLadderClimbEnded` is `BlueprintAssignable`, so **a Blueprint binding it would break at LOAD on the widened signature** — invisible to the compiler and to the headless suite, i.e. exactly `SC-§35`'s class (1,806 BP runtime errors that compiled clean and cleared two QA gates).
   - **`Source/`:** ⛔ **zero** binders other than the tower's own (`AddUniqueDynamic`/`RemoveDynamic`, both widened together).
   - **`Content/` (grepped the `.uasset` name tables):** ⛔ **zero** hits for `OnLadderClimbEnded`, `BeginLadderClimb`, `AbortLadderClimb` or `IsClimbing`.
   - ⭐ **WITH ITS OWN SELF-CHECK, because a grep that finds nothing proves nothing until you show it can find something:** the same grep resolves `BeginPlay` in `BP_BattlefieldScatter` / `BP_CommanderNpc` / `BP_HeroCharacter`, so plain FName strings **are** reachable this way.
   - ⚠️ **THE LIMIT, STATED RATHER THAN GLOSSED:** an asset-name-table grep is ⛔ not an editor load, and `SummonedUnit.h:781` claims *"`IsClimbing` is read by `ABP_Footman`'s climb state (TASK-739)"* — which this grep does **not** corroborate. Either that wiring reads a cached variable rather than the function, or the claim is stale. ⛔ **Neither `IsClimbing`'s signature nor `ABP_Footman` was touched by me**, so it is not a risk this diff creates — but ⇒ **TASK-780's PIE row should still include a MESSAGE-LOG READ**, which costs nothing on a session it is already running.
6. **`ILadderClimber` overriding without the `virtual` keyword.** `ASummonedUnit::AbortLadderClimb()` / `IsClimbing() const` are declared without `virtual` and are `UFUNCTION`s; a matching signature overrides regardless, so ⛔ neither shipped declaration had to change. Worth a second pair of eyes on the UHT interaction, since I cannot compile.
7. **`LadderContacts` bounding.** It only grows for pawns **inside** the radius, is pruned of dead handles on every lookup, and drops a row the moment a pawn leaves. Confirm there is no path that adds a row without a matching drop.
