# QA Report — TASK-908 (F-GATE-A: the fog-clamp gate)

**Gate task:** TASK-908 · **Reviewer:** qa-reviewer · **Date:** 2026-09-03
**File named for the GATE task** (`SC-§29`; `qa/TASK-847.md` / `qa/TASK-848.md` precedent) — ⛔ never for a subject task.

---

## ⛔⛔ WHAT WAS **NOT** RUN, SAID ABOVE THE VERDICT AND NOT IN A FOOTNOTE (`TL-§5c` cl. 5)

- ⛔ **I did not compile.** ⛔ **I did not run one test.** ⛔ No editor (it is UP, PID 23636 — untouched), no MCP, no Git, no edit to any source file.
- ⛔ **`TASK-838` was NEVER COMPILED and its 8 new tests were NEVER EXECUTED.** Its handoff's `408 declared across 30 files` is **its author's census**, not a pass — and mine below is a census too.
- ⇒ ⛔ **THE DUTY IS TRANSFERRED BY NAME TO `TASK-835`** (the Lane W commit-gate build). It owes the **first execution** of `Tests/SiegeFogClampTest.cpp`:
  ```
  -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended
  ```
  ⛔ Report **both** `Result={Success}` and `Result={Fail}` verbatim; an **absent** `Result` line is an unreadable instrument, not a green suite (`SC-§39`).

---

## ⭐⭐⭐ THE INERTNESS VERDICT — ONE SENTENCE, AT THE TOP, BECAUSE IT IS WHAT THE NEXT COMMIT READS

> ## ✅ **`TASK-838`'s FOG CLAMP IS STRUCTURALLY INERT AT RUNTIME: with `ReadFogState` returning `false` on every path, `EffectiveVisionRadius` returns each site's requested radius bit-identically, the funnel's `EffectiveRadiusUU < RequestedRadiusUU` guard is therefore FALSE at every one of the five vision sites, and `Out.RemoveAll(...)` CANNOT EXECUTE — so the acquisition surface committed by Lane W is byte-for-byte the game that shipped.**

⛔ **Verified by reading both function bodies end to end (`SC-§43` cl. 3 / `SC-§40` cl. 1), NOT by reading the pins that count them.** The handoff's own sentence *"fog does nothing at runtime"* was treated as the claim under test, never as its evidence.

**Verdict: ✅ PASS — 0 BLOCKERS · 4 WARN · 4 NIT**

---

## ⭐ `SC-§29` COVERAGE LEDGER — this gate covers the tasks it NAMES

| task | subject | verdict | evidence |
|---|---|---|---|
| **TASK-838** | `SiegeCombatStatics.{h,cpp}` (the `FSiegeVisionQuery` 5th param + `ReadFogState` + the funnel cut) · `SiegeFogStatics.{h,cpp}` (`FOG-§7b` **both halves only**) · `Tests/SiegeFogClampTest.cpp` (NEW, +8) · `Tests/SiegeFogTest.cpp` (comment-only) · the 5 vision call sites | ✅ **PASS — 0 blockers** | this file |

⛔ **NOT covered by this gate:** `TASK-837` (→ `qa/TASK-847.md`, PASSED) · `TASK-828` (→ `qa/TASK-848.md`, PASSED) · `TASK-829` (→ `TASK-849`) · `TASK-839`/`TASK-840` (→ `TASK-850`, **not yet written — not reviewed and not failed for their absence**) · `TASK-851`'s `SiegeBotController.{h,cpp}` diff · all of Lane W · the fog **visual** and every pixel row (`TASK-843`/`858`/`J-F11`) · the compile.

---

## 1. ⛔⛔⛔ ITEM (1) — THE HEADLINE ROW. BOTH LEGS, AS MEASUREMENTS

### 1(a) `FSiegeCombatStatics::ReadFogState` returns `false` on **EVERY PATH** — ✅ **TRUE, READ WHOLE**

`SiegeCombatStatics.cpp:119-145`. The **entire** body, enumerated statement by statement:

| line | statement | can it return true? |
|---|---|---|
| `:124` | `OutTuning = FSiegeFogTuning();` | not a return — and it is **unconditional and first**, so no early-out hands back an uninitialised band ✅ |
| `:128-131` | `if (!World) { return false; }` | ⛔ **false** |
| `:144` | `return false;` (the seam, the only line `TASK-839` replaces) | ⛔ **false** |

⇒ **there are exactly TWO return statements in the function and both are the literal `false`. There is no third exit, no ternary, no output parameter that carries a state, and no `bool` local.** The function is `private` (`h:380` `private:` → `h:416`), it has **one** call site tree-wide, and `ReadFogState(` counts **3** across shipping source (header decl, definition, the one call) — I re-measured that myself rather than taking test 8's pin.

📌 **The pin is corroboration only, exactly as the spec requires:** test 8 asserts `return false;` == 2 inside the extracted body. A count of occurrences is **not** a proof of reachability — the proof above is the enumeration of exits.

### 1(b) `FSiegeFogStatics::EffectiveVisionRadius` is **BIT-IDENTICAL** with fog off, and `838` calls it **UNCONDITIONALLY** — ✅ **TRUE, RE-VERIFIED ON THE CONSUMING SIDE**

- **Producer side (re-verified, though it is `847`'s):** `SiegeFogStatics.cpp:73-76` is the **first** statement of the function — `if (!bFogActive) { return RequestedRadiusUU; }` — and it returns **before `Tuning.FogVisionCeilingUU` is ever read** (`:78`). No `min`, no clamp, no sanitising, no ulp of drift.
- **Consumer side (this gate's job):** `SiegeCombatStatics.cpp:195-196` calls it **unqualified by any fog-state branch**:
  ```cpp
  const float EffectiveRadiusUU = FSiegeFogStatics::EffectiveVisionRadius(
      Vision->RequestedRadiusUU, bFogActive, FogTuning);
  ```
  ⛔ **There is no hand-written `if (bFogActive)` anywhere in the funnel.** Measured: `if (bFogActive` occurs **0** times on any code line of `SiegeCombatStatics.cpp` (the two textual hits at `:189`/`:198` are `//` comment lines and are skipped by the house scanner **and** by my own read). ✅ Gate row honoured.

### 1(c) ⭐⭐ THE COMPOSITION — WHY (a) + (b) MAKES THE DIFF INERT AND NOT MERELY QUIET

With `bFogActive == false` for **every** call:

1. `EffectiveRadiusUU` is the **same float** as `Vision->RequestedRadiusUU` (returned by value, unmodified).
2. The guard at `:207` is `if (EffectiveRadiusUU < Vision->RequestedRadiusUU)` ⇒ `R < R` ⇒ **false**.
3. ⇒ `Out.RemoveAll(...)` at `:216-244` **never runs**; not one `ActorGetDistanceToCollision` is issued; not one element of `Out` can be removed.
4. Everything after the cut — the `VeilPolicy` early-out (`:256`) and the veil `RemoveAll` (`:279-282`) — is **byte-identical** to the pre-`838` file.

**Degenerate inputs also stay inert, which is the part a state check would have got wrong:**
- `RequestedRadiusUU == NaN` ⇒ `NaN < NaN` is **false** by IEEE ⇒ no-op ✅ (correct direction).
- `RequestedRadiusUU == TNumericLimits<float>::Max()` (the two **unbounded** sites) ⇒ `Max < Max` is **false** ⇒ no-op ✅.
- On x64/SSE there is no x87 excess-precision hazard; and even under excess precision `R < R` remains false.

⇒ ✅ **`SC-§43` cl. 3's price is PAID by the first of its two readers. `TASK-835` item (0c) is the second.**

### 1(d) AND THE REST OF THE DIFF, SO "INERT" COVERS THE **WHOLE** DIFF AND NOT ONLY THE CLAMP

| change | runtime effect today |
|---|---|
| `GatherHostileAgents` gains a **defaulted** 5th param | ⛔ none — every unchanged caller (blast, 2 spell gathers, cheat lane) binds `nullptr` and takes the `if (Vision)` false branch |
| the 5 vision sites now spell `ESiegeVeilPolicy::SuppressVeiled` out loud | ⛔ none — it **is** the default they already bound |
| `MyLocation` hoisted at 3 sites (`SummonedUnit.cpp:1743`, `:2329`, `Tower.cpp:224`) | ⛔ none — `GetActorLocation()` is a pure read and `GatherHostileAgents` moves no actor. **No shadowing:** each function has exactly one `MyLocation` (measured: `Tower.cpp` 224/235/265; `HeroCharacter.cpp` 535 was already there for `PlayWorldSound`) |
| `ReadFogState` called once per gather at 5 sites | one default `FSiegeFogTuning` construct + one call + one float compare — **unobservable** |
| `FOG-§7b`(a) `ClampMin "0"` → `"304.8"` | ⛔ none at runtime — editor spinner metadata only; the shipped default `609.6f` is **untouched** |
| `FOG-§7b`(b) `Ceiling < 0.f` → `<= 0.f` | ⛔ none today (`bFogActive` is never true); when fog lands it converts a **combat outage** into a no-op |
| `Tests/SiegeFogTest.cpp` | comment/message text only — the predicates are byte-identical (see NIT-4) |

⚠️ **Null-safety of the cut, checked because it is the one new loop over actors:** `GatherTeamAgentsFiltered` (`:55-75`) applies `IsValid(Candidate)` before `Out.Add`, so the fog lambda cannot dereference a null or pending-kill actor. The lambda captures `ViewOrigin`/`FogTuning` by reference to locals that outlive `RemoveAll`, and `Vision` is dereferenced **only** inside `if (Vision)`. ✅ No dangling, no null deref.

---

## 2. THE SHAPE — VERIFIED RATHER THAN ACCEPTED

| # | claim | verdict | measured |
|---|---|---|---|
| 1 | **The clamp lives in exactly ONE place, inside `GatherHostileAgents`** | ✅ **TRUE** | The **leaf-grep gate reads 2**, as the spec asked me to confirm: `FSiegeFogStatics::EffectiveVisionRadius(` occurs on code lines in shipping source at `SiegeFogStatics.cpp:67` (its **definition**) and `SiegeCombatStatics.cpp:195` (the **one call**). ⛔ Zero in `SummonedUnit.cpp`, `Tower.cpp`, `HeroCharacter.cpp`, `SpellLineSweep.cpp`, `SiegeCheatManager.cpp`. (`SiegeFogStatics.cpp:120`'s intra-class call is **unqualified** and is correctly outside this population — a per-site clamp in another TU **cannot** be written unqualified.) |
| 2 | **The no-op decision is on the OUTPUT, never on `bFogActive`** | ✅ **TRUE, and I concur it is strictly stronger** | `:207` tests the seam's return against the request. It is correct for **every** member of `FOG-§7a`'s totality list — NaN ceiling, negative ceiling, **zero** ceiling, `Ceiling <= Onset` — all of which return the request unchanged and therefore skip the loop. ⛔ A state check would have **entered** the loop with a broken band. **I do not ask for it to be deleted.** |
| 3 | **No branch inspects the caller** | ✅ **TRUE** | `switch (` occurs **0** times on any code line of `SiegeCombatStatics.cpp` (the single textual hit at `:161` is the comment that forbids it). No caller enum, no exempt-site list, no `CallerKind`. The `TASK-838(1b)` automatic-FAIL shape is **absent**. |
| 4 | **The exemption is STRUCTURAL, not conventional** | ✅ **TRUE — and I checked this rather than accepted it** | Two independent legs: **(i)** the parameter is `const FSiegeVisionQuery*` defaulted to `nullptr`, so a lane that constructs no query supplies **no radius the funnel could clamp with** — the absence is not a policy, it is the absence of an operand; **(ii)** `838` rejected the default-constructed "unbounded" query precisely because unbounded is a *legitimate* vision query (2 sites use it) that clamps to the ceiling under fog — making it the default would have silently clamped the blast, both spell gathers, the friendly buff and the cheat lane. **Absence had to mean "not an act of seeing", which is a different thing from "seeing without limit". That reasoning is correct and it is the load-bearing one.** |
| 5 | **The friendly lane cannot be fogged** | ✅ **TRUE at the DECLARATION** | `h:313` — `GatherFriendlyAgents(const UWorld*, ETeamId, TArray<AActor*>&)`. **No policy parameter, no vision parameter.** A lane with no parameter cannot be given one by accident (`FOG-§7` row 5). |
| 6 | **The per-candidate metric is never stricter than any site's own** | ✅ **TRUE, and the fallback is byte-identical** | The funnel (`:229-236`) is `ActorGetDistanceToCollision(ViewOrigin, ECC_Pawn, ClosestPoint)` with `Distance < 0.f` → `FVector::Dist(ViewOrigin, Candidate->GetActorLocation())`. **`HeroCharacter.cpp:613-619` is character-for-character the same construct**, including the fallback to `Target->GetActorLocation()` (⛔ **not** to the untouched `ClosestPoint`, which is the trap in that idiom — the hero avoids it and so does the funnel). Closest-point ≤ origin-to-origin ⇒ a tower measuring origin-to-origin keeps its own gate untouched. ✅ |
| 7 | **Order preservation** | ✅ **TRUE** | Two independent `RemoveAll` passes, both order-preserving; `Sort(` == **0** in the funnel body. Every caller tie-breaks by strict improvement, so enumeration order still decides ties at all eight sites. |
| 8 | **`FOG-§7b` shipped BOTH halves** | ✅ **TRUE, and the reasoning survives intact** | **(a)** `SiegeFogStatics.h:181` — `meta = (ClampMin = "304.8")`, the **onset's own pinned value**, ⛔ not a new invented number. **(b)** `SiegeFogStatics.cpp:95` — `!FMath::IsFinite(Ceiling) \|\| Ceiling <= 0.f \|\| !FMath::IsFinite(RequestedRadiusUU)`. ⛔ The strict `< 0.f` is **gone** from the body. ⭐ **Half (a) alone would have been the fix that looks complete and holds nothing** — `ClampMin` constrains the editor spinner and nothing else, and an `.ini`, a Blueprint default or a line of C++ can still write `0`; `FogVisionCeilingUU = 0` would give `min(Range, 0) == 0` for **every** acquisition and stop all combat under fog. **(b) is the half that protects the game and it shipped.** ✅ |
| 9 | **Symmetry is structural; `838` added nothing** | ✅ **TRUE — measured in both files** | `ETeamId` / `ITeamAgent` / `AActor` / `AController` / `APlayerController` / `UWorld` / `ViewerTeam` occur on **ZERO code lines** of `SiegeFogStatics.h` and `SiegeFogStatics.cpp`. The only textual hits are `h:7`, `h:223`, `cpp:6-7` — all comment lines saying the concepts are absent. ⭐ **And the one new type is clean too:** the `FSiegeVisionQuery` slice (`h:111-156`) carries **no** `ETeamId` and **no** `AActor` — it is a **point and a radius**. An asymmetric fog remains **unrepresentable**, and no *"both teams agree"* test was written (`FOG-§7a` forbids it). ✅ |
| 10 | **`SiegeBotController.{h,cpp}` untouched BY THIS TASK** | ✅ **TRUE — authorship checked, not assumed** | `FSiegeFogStatics` / `EffectiveVisionRadius` / `FSiegeVisionQuery` / `ReadFogState` / `FogVision*` all return **ZERO** occurrences in `SiegeBotController.cpp`. Its only funnel touch is `#include … // TASK-851 (WITCH-§8): FSiegeCombatStatics::IsAgentVisibleTo` — ⛔ **that is `TASK-851`'s invisibility diff, not a `838` fence break.** |
| 11 | **`838` did not re-assert `837`'s arithmetic** | ✅ **TRUE** | `SiegeFogClampTest.cpp` carries no falloff curve, no `FogDensityAt` sweep, no `FOG-§2` table. Its only direct `EffectiveVisionRadius` calls are **counterfactuals** (what a clamp *would* cost) and the `FOG-§7b` zero/negative rows, which are `838`'s own subject. |
| 12 | **`WM-§` / `MARK-§` untouched** | ✅ **TRUE** | No war-map or marker symbol appears in any file of this diff. |

---

## 3. ⭐⭐ ITEM (3) — `838`'s REFUTATION OF THE LAW: **CONFIRMED AT SOURCE, AND IT IS OWED AN AMENDMENT**

I re-measured `Docs/Data/cards.csv` myself rather than confirming the handoff's arithmetic.

- **Header field index of `AoERadius` = 18** (the header's leading empty field is index 0). ✅
- **`Lightning` (`cards.csv:26`): `AoERadius = 700`, `MaxTargets = 3`, resolver `TopTargetsDamage`.** ✅ **CONFIRMED.** 700 − 609.6 = **90.4 uu above the ceiling, today, in shipped data.**
- **The complete non-zero `AoERadius` column, read row by row:** Sapper 250 · BombTower 250 · Fireball 300 · FrostNova 350 · **Lightning 700** · BattleCry 400 · Wizard 250. **Exactly ONE exceeds 609.6.** ⇒ `FOG-§7`'s / `TASK-838(5b)`'s claim that *every* AoE radius is under the ceiling is **FALSE**, and its six-card enumeration missed precisely the one that matters.

**The consequences `838` drew — all confirmed:**

| claim | my arithmetic |
|---|---|
| line sweep would be cut **−32.3%** | `ASpellLineSweep::LineRange = 900.f` (`SpellLineSweep.h:103`, verified at source). (900 − 609.6)/900 = **32.27%** ✅ |
| Lightning's 3-target selection would be cut **−12.9%** | (700 − 609.6)/700 = **12.91%** ✅ |
| ⇒ a blind clamp would have been **TWO** live nerfs, not one | ✅ **CONFIRMED — `J-F9` covers a CLASS (`FOG-§7` row 3), not one spell.** Both hand over nothing; both are exempt for the same structural reason. |
| the spec's proposed *"synthetic 700"* was a **SHIPPED NUMBER** | ✅ **CONFIRMED — `SC-§37` one level up from itself.** A test written to the letter of `TASK-838(5b)` would have been drawn from live data. |

**And the substitute radius is DERIVED, not transcribed** (`SC-§40` cl. 10) — checked, because the spec said a transcribed constant is a WARN even with the right reasoning:
```cpp
static float SyntheticRadiusAboveCeilingUU()  { return ShippedTuning().FogVisionCeilingUU * 2.f; }   // = 1219.2
```
✅ **Derived from the tunable**, so a ceiling retune can never make it stop discriminating — **and** test 3 additionally proves it collides with nothing shipped (`bSyntheticValueCollides` must be **false** against the whole AoE column). ⛔ **Not a WARN.**
📌 The two transcribed constants that *do* exist (`ShippedLineSweepRangeUU = 900.f`, `ShippedLightningReticleUU = 700.f`) are each **pinned against their source in the same test** (test 2 re-reads `LineRange = 900.f` from `SpellLineSweep.h`; test 3 re-reads the largest AoE from `cards.csv`), which is the correct treatment — they are premises that go red when they rot, not silent copies.

⇒ ⚖️ **MANAGER AMENDMENT OWED TO `FOG-§7` (and to `TASK-838(5b)`'s text), ⛔ NOT A CODE FIX.** The exemption is *more* load-bearing than the law claimed, not less: rows 2 and 3 are **structural**, so the day an AoE crosses the ceiling that card does **not** silently become fog-dependent. That day was already yesterday.

---

## 4. ITEM (4) — THE RESIDUALS ARE **DECLARED**, IN PLAIN LANGUAGE

An undeclared residual is its own finding. All are present and none is softened:

| residual | where | plain enough for Jonathan? |
|---|---|---|
| 🧑 **`J-F9`** — *"under fog the hero's line spell still reaches its full 900 uu, so he can hit something at 900 uu that he literally cannot see (his eyes reach 609.6)"* | handoff **§4**, verbatim, in its own block quote; repeated at `SiegeCombatStatics.cpp:173-175` and `h:239-245` | ✅ **YES — he can overrule it from that sentence alone.** And the reversal cost is stated: **one line** at `SpellLineSweep.cpp`'s gather; `SiegeFogClampTest` test 2 has the row to invert (it asserts the sweep hands over **0** vision queries). ⛔ **The recorded proceeding default was BUILT, not improvised** — I checked the code matches the ruling rather than a programmer's preference. |
| 🧑 **`J-F4`** — the bot's **units** go blind, its **card-play planning** stays omniscient through three `TActorIterator` scans | handoff **§7.4** | ✅ named as **deliberate**, with the file (`SiegeBotController.cpp`) and the owning task (`TASK-851`, invisibility only) |
| **Row 2 is STRUCTURAL, not merely currently-inert** | handoff **§5.3**, **§3**; test 3(b) | ✅ and it is now *proven* by the Lightning finding — the "inert by coincidence of current data" reasoning was **never even true** |
| ⭐ **`AggroRadius` is the GDD §3.8 profile constant 600 — already inside the ceiling ⇒ unit acquisition is essentially untouched; the card bites at the TOWERS** | handoff **§7.2**, with the numbers (ArrowTower 900 −32.3%, Bomb/Crystal 800 −23.8%, Ballista 1400 −56.5% with `MinRange 300` intact ⇒ a **300–609.6 annulus**) | ✅ **YES**, and it correctly separates the **gather** from the `<= AttackRange` **firing** gate, flagging that a "Longbowman should stop firing past 609.6" expectation would be a **new task**, not this one |
| the chain zap's candidate pool narrows under fog | handoff **§7.3** | ✅ declared as a **cost**, with the bounce rule untouched |
| an arrow already in the air still lands | handoff **§7.5** | ✅ inherited from `837` ruling 4, unchanged |
| **fog does not exist at runtime yet — a landed clamp is not a landed feature** | handoff **§7.1**, `h:397-403`, `cpp:133-144`, test 8's message | ✅ **stated four times, unsoftened** |
| no test drives the clamp over a populated world — **green here is not "fog works"** | handoff **§7.6**, file header `:53-56` | ✅ and it names `TASK-850(3a)`'s PIE pass as where live behaviour is judged |

---

## 5. THE NEW SUITE — READ, NOT COUNTED

I read all eight tests. They assert **wiring**, not `837`'s arithmetic, and each can genuinely go red:

- **1** — per-file vision-query census **2/2/1** plus **zeros** for the four unclamped lanes, each with its own positive control; then the same needle **tree-wide** at 5, so a sixth lane in a file nobody listed is caught. Re-measured by me: `SummonedUnit.cpp:1758`, `:2341` · `Tower.cpp:235`, `:420` · `HeroCharacter.cpp:581` = **5, and 5 tree-wide.** ✅
- **2** — re-reads `LineRange = 900.f` **at source**; asserts the counterfactual (−32.3%) so the exemption is load-bearing rather than decorative. ✅
- **3** — derived synthetic radius + the no-collision proof + the `cards.csv` tripwire (`RowsAboveCeiling == 1`, `LargestAoE == 700` = `Lightning`) + Lightning's own −12.9%. ⭐ **The csv probe is not pinned to a row count** (`RowsRead >= 10`), so the parallel `Witch` row landing did **not** make it stale — see WARN-3. ✅
- **4** — the leaf-grep gate at **2**; the call inside the **funnel body**; `if (bFogActive` == 0; veil consult still 1; `Sort(` == 0; the friendly lane's declaration pinned. ✅
- **5** — seven asymmetry tokens == 0 across **both** fog files with a per-file positive control, plus the same over the `FSiegeVisionQuery` slice. ⛔ Correctly **not** a *"both teams agree"* test. ✅
- **6** — zero **and** negative ceilings return the request at `Exact`; ⭐ **and the PREDICATE is exercised too** (`IsVisibleThroughFog(3000, 3600, true, ZeroCeiling)` and a melee row), because *"returns 3600"* and *"can still shoot"* are different claims. Source form `Ceiling <= 0.f` == 1 / `Ceiling < 0.f` == 0. The `ClampMin` string is proved to **track** `FogVisionOnsetUU` (the pair `ClampAtOnset == 1` + `Atof("304.8") == onset` does establish tracking — an onset retune turns the second row red). ✅
- **7** — the six radii the five sites actually hand over, at `Exact`, **including the unbounded sentinel**; then the **no-op property** itself, which is the assertion form of §1(c). ✅
- **8** — `ReadFogState` == 3 tree-wide; called **once per gather, not per candidate**; the honest `return false` count labelled as the row `839` must **INVERT, not delete**; `OutTuning` assigned unconditionally. ✅

**Instrument discipline (`SC-§39` / `SC-§41`) — checked in both directions:**
- ✅ Every needle in the new file carries an **open paren** or is a full statement/whole-line construct — `FSiegeCombatStatics::GatherHostileAgents(`, `FSiegeVisionQuery::SeeingFrom(`, `Ceiling <= 0.f`, `OutTuning = FSiegeFogTuning();`. ⛔ No bare token is pinned where a substring or a comment could move it.
- ✅ **The `/*`-skip blind spot is respected by the SUBJECT, not only by the probe:** `SiegeCombatStatics.cpp:110-116` explicitly keeps the named-argument `IsVisibleTo` call **on one line** because a one-argument-per-line layout would hide `/*ViewerTeam=*/` from the very test that enforces it. That is the `SC-§39` amendment applied prospectively.
- ✅ **Substring hazard controlled:** `SeeingFrom(` is provably **not** a substring of `SeeingFromUnbounded(` (the next character differs), and the file says so at `:344-345`. I verified the claim.
- ✅ Every absence row has a **positive control** (`CountAcrossShippingSource` returns `-1` and errors if the recursive scan finds < 20 files; each lane/file row asserts a live token first). A dead instrument **fails** rather than reading as a clean zero.

---

## 6. CENSUS — `TL-§5b` / `TL-§5c`, **DECLARED**, ⛔ NEVER A PASS COUNT

**Scoped pattern:** `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`.

> ### **426 declared across 31 files** (working tree, my instant, 2026-09-03) — ⛔ **DECLARED. I EXECUTED NOTHING.**

- ⭐ **`838`'s OWN DELTA IS `+8` AND I VERIFIED IT EXACTLY:** `Tests/SiegeFogClampTest.cpp` = **8** declarations, all `IMPLEMENT_SIMPLE_AUTOMATION_TEST`, all `EditorContext | EngineFilter` (the idiom in all 31 shipped test files).
- ⛔ **The handoff's `408 declared across 30 files` cannot be reconciled and should not be** — it is a measurement of a tree that has since moved under `TASK-851`/`853` and at least one new test file. `TL-§5b`: **reconcile deltas, never absolutes.** The board's `408/30` on `838`'s row is likewise stale; do not gate on it.
- ⚠️⭐ **THE PATTERN TRAP IS LIVE ON THIS TREE — I MEASURED IT MYSELF, BOTH WAYS** (`TL-§5b` cl. 2a):
  | pattern | result |
  |---|---|
  | ⛔ bare `^IMPLEMENT_` over `Source/**/*.cpp` | **427 across 32 files** — the extra is `GitClaudeUnrealTest.cpp`'s `IMPLEMENT_PRIMARY_GAME_MODULE` |
  | ✅ `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` scoped to `Siegebound/Tests/*.cpp` | **426 across 31 files** |
  ⇒ `TASK-814` measured this trap **absent** one day ago; **that measurement was not a repeal.** The absence of a trap is a property of a tree.
- ⛔⛔ **`TL-§5d` WARNING FOR THE COMMIT, AND IT IS A NUMERIC COINCIDENCE THAT WILL MISLEAD SOMEBODY:** the last **executed** figure on record is `426 / 0` at `e9df584`, and today's **declared** working-tree census is also **426**. ⛔ **THE SAME NUMBER IS NOT THE SAME SET.** `Tests/SiegeFogClampTest.cpp` is **new and untracked** — it is inside today's 426 and **invisible to `HEAD`** until `TASK-835` takes it. ⇒ **do NOT reconcile `838`'s landing by observing "still 426".** Reconcile by naming the file and its 8 rows.

---

## Findings

- **[WARN-1] `handoffs/TASK-838-programmer.md` §8 — the "PRE-EXISTING RED ROW" IT ROUTES TO `TASK-849`/`TASK-850` DOES NOT EXIST ON TODAY'S TREE. The row was already repaired by `TASK-868`.**
  `SiegeAcquisitionFunnelTest.cpp` test 9 no longer asserts one flat list of five zeros. It was **split** (`:1124-1161`) into **DECISION tokens** — `IsVisibleTo(`, `FSiegeFogStatics`, `EffectiveVisionRadius` — zero in **all five** call-site files, and **STATE tokens** — `bIsInvisible`, `FSiegeInvisibilityStatics` — zero in the **four non-owner** files, with `SummonedUnit.cpp` exempt **BY NAME** and replaced by a stricter enumerated write-door census (`:1293+`). ⇒ `838`'s measured `bIsInvisible` ×3 / `FSiegeInvisibilityStatics` ×4 in `SummonedUnit.cpp` (which I reproduced exactly: `:40`'s trailing include comment + `:2859` + `:2868`, and `:40` + `:2859`/`:2868`/`:2884`) **are no longer assertion subjects.** `IsVisibleTo(` is **0** there ✅ and both fog tokens are **0** in all five files ✅.
  ⇒ **The row is GREEN. Nothing is owed to `TASK-849`/`TASK-850`.** ⭐ `838`'s *suggested fix* — split into GUARD and STATE tokens with the owner exempt by name — is, to the character, **what `TASK-868` already shipped**; the analysis was right and the tree had simply moved under it (`TL-§5d`, applied to a test file instead of a suite total).
  **Suggested fix:** strike §8's routing so no build-master or gate chases a phantom red. ⛔ No code change.
- **[WARN-2] `Tests/SiegeFogClampTest.cpp:475-477` (test 3's banner) — THE NEW FILE'S OWN COMMENT REPEATS BOTH OF THE ERRORS THE FILE EXISTS TO CORRECT.** It reads *"THE BLAST EXEMPTION, ASSERTED AT A **SYNTHETIC** 700 … because every shipped radius is already under the ceiling."* ⛔ **Both halves are false and the file proves them false 200 lines later:** the synthetic value is `2 × ceiling` = **1219.2** (700 is the *shipped* `Lightning` value — the whole point of §5), and **one shipped radius is above the ceiling**. The same stale `700` recurs in the assertion **message** at `:502` (*"A 700-uu blast WOULD be cut…"*) while the value under test is `SyntheticRadius` = 1219.2.
  ⛔ **The assertions are CORRECT** — this is prose only, and the file's own top-of-file header (`:32-46`) gets it right, saying **"almost"** and then explaining the correction. But `838`'s own handoff argues that *"a wrong number in a comment is a real defect in this project"* and repaired exactly this species one file over. Same standard applies here.
  **Suggested fix (test file, one commit, no assertion change):** banner → *"ASSERTED AT A **DERIVED** RADIUS (2× the ceiling), because ALMOST every shipped AoE radius is under the ceiling — see the `Lightning 700` correction below"*; `:502` message → `%.1f`-format the synthetic value instead of the literal `700-uu`.
- **[WARN-3] `SiegeFogStatics.h:96` vs `:181` — THE ONSET'S OWN PARAGRAPH NOW CONTRADICTS THE CEILING'S.** `:96` still states *"There is exactly ONE `304.8` in the codebase and it is this line."* `FOG-§7b`(a) put a **second** code-line occurrence at `:181` (`meta = (ClampMin = "304.8")`). The **ceiling's** comment declares it properly (`:175-179`: meta string, not a float literal, UHT cannot reference a C++ constant, unavoidable, asserted by test 6) — the **onset's** comment was not updated to point at it. In a codebase whose `FOG-§1` "no second literal" grep is a real gate, the two paragraphs now disagree about a fact the gate depends on.
  ⛔ **Not a code defect and not a `FOG-§1` violation:** the value is a `ClampMin` **string**, the divergence is asserted red-able by test 6, and the declaration exists — it is simply in the wrong paragraph for the reader who greps.
  **Suggested fix:** one cross-reference line at `:96` — *"⛔ …and ONE meta string, `ClampMin = "304.8"` on the CEILING at `:181`, declared there and pinned by `SiegeFogClampTest` test 6."*
- **[WARN-4] Two census figures in the handoff are stale, and one is stale in a way that matters at commit time.** (i) §9's `408 declared across 30 files` vs today's **426 across 31** — reconcile the **delta** (`+8`, verified) and **not** the absolute. (ii) §5's *"31 rows parsed"* of `cards.csv` — today the file holds **32** data rows (all 32 align with the 31-field header; the `Witch` row landed in parallel). ⛔ **This does NOT make test 3 stale**, because the probe asserts `RowsRead >= 10` rather than a pinned count — the right call, and worth saying out loud since a pinned `31` there would be red **right now**.
  **Suggested fix:** none in code. For `TASK-835`: assert `838`'s **`+8`**, and see §6's `TL-§5d` warning about the `426 == 426` coincidence.
- **[NIT-1] `Docs/Data/cards.csv:26` — `Lightning`'s `Notes` column says *"…in 400 castle excluded"* while its `AoERadius` is **700**.** Every other AoE card's prose agrees with its data (Sapper 250/250, BombTower 250/250, Fireball 300/300, FrostNova 350/350, BattleCry 400/400). ⭐ **This is almost certainly HOW `FOG-§7`'s six-card enumeration missed Lightning** — whoever wrote the law read the human-readable column, which is wrong by 300 uu. Route to the manager **with** the `FOG-§7` amendment: either the data or the note is wrong, and a designer reading the deck will believe the note. ⛔ Out of `838`'s fence; ⛔ do not "fix" it inside a fog task — a radius change is a balance change.
- **[NIT-2] `SiegeCombatStatics.cpp:243` — the ceiling is derived `1 + N` times per fogged gather, not once.** `IsVisibleThroughFog(Distance, RequestedRadiusUU, …)` re-invokes `EffectiveVisionRadius` internally for **every candidate**, while `EffectiveRadiusUU` (computed once at `:195`) is used only by the no-op guard. Same inputs ⇒ same answer, so it is **correct**, and it is **free today** (the loop cannot run) and near-free under fog (a branch and a `Min`). ⛔ **Recorded only so nobody later "optimises" it** by writing `Distance > EffectiveRadiusUU` at `:243` — that would put a second, silently divergent copy of the inclusive `<=` boundary rule in the tree, which `:238-242` explains at length and which is the correct call. **No change requested.**
- **[NIT-3] `SiegeCombatStatics.h:128` — `FSiegeVisionQuery::SeeingFrom(Center, BlastRadius)` remains TYPEABLE.** The type cannot distinguish a blast radius from a vision radius; the guard is the **constructor's name** (which would read wrong on the page), the header's ban at `:119`, and test 1's per-file census pinning `SiegeCombatStatics.cpp` (the blast's file) at **0** vision queries. That is a naming + test guard, not a type guard — which is a weaker class than the `if (Vision)` exemption itself. ⛔ Declared in the header, correctly. **No change requested** (a distinct `struct FVisionRadius` wrapper would buy little against three converging guards).
- **[NIT-4] `Tests/SiegeFogTest.cpp:263` — "comment-only" is true of the PREDICATE, not literally of the line.** The `TASK-847` NIT-2 correction also rewrote the assertion's **message string** (`"~3× more"` → `"~305× more"`). The predicate `FogDensityAt(Onset + 1.f, Tuning) < 0.001f` is byte-identical, as are all nine tests' assertions; `:97-105`'s ULP correction is a pure comment block. ⛔ Zero behaviour change, and both corrections are **substantively right** (I recomputed: ratio = band width = **304.8**; ULP at 609.6 = **6.10e-05**, so `1e-4` clears by 1.6× and the "do not tighten" warning is warranted). Recorded only so the build-master's diff review is not surprised by a changed line inside a `TestTrue(`.

---

## Notes for build-master (`TASK-835`)

1. ✅ **`TASK-838` PASSES. `TASK-835` may count `.claude/pipeline/qa/TASK-908.md` as satisfied for `TASK-838` — and for that task ALONE** (`SC-§29`). ⛔ `TASK-829` still needs `TASK-849`; `TASK-839`/`TASK-840` still need `TASK-850`. ⛔ Nothing here is a compile result.
2. ⛔⛔ **ITEM (0c) IS STILL OWED AND THIS REPORT DOES NOT DISCHARGE IT** (`SC-§43` cl. 3 requires **two** independent readers; I am the first). **The four-line recipe, so your re-verification is cheap and identical to mine:**
   - `SiegeCombatStatics.cpp` `ReadFogState` (`:119-145`) — **enumerate the exits, do not count them**: two `return` statements, both `false`; `OutTuning` assigned first and unconditionally.
   - `SiegeFogStatics.cpp:73-76` — `if (!bFogActive) { return RequestedRadiusUU; }` is the **first** statement, **before** the tuning is read.
   - `SiegeCombatStatics.cpp:207` — `if (EffectiveRadiusUU < Vision->RequestedRadiusUU)` ⇒ `R < R` ⇒ false ⇒ `RemoveAll` unreachable.
   - `grep "if (bFogActive"` in the funnel ⇒ **0** on code lines. If any of the four moved, ⛔ **STOP and escalate — do not commit.**
3. ⚠️ **COMPILE-SHAPE CHECKS I DID MAKE**, so the build is not surprised (⛔ none of this is a compile):
   - `SiegeCombatStatics.h` adds `#include "Math/NumericLimits.h"` (needed — `TNumericLimits<float>::Max()` is a default **member initialiser** in a header) ✅ and a plain `struct FSiegeFogTuning;` forward declaration ✅ (legal for a reference parameter; the header stays free of the fog header and free of `.generated.h`, so **no UHT surface is added by this diff**).
   - `SiegeCombatStatics.cpp` gains `#include "Siegebound/SiegeFogStatics.h"` (`:15`) — required for the complete `FSiegeFogTuning` at `:186` ✅. `ECC_Pawn` is covered by the existing `Engine/EngineTypes.h`; `ActorGetDistanceToCollision` by `GameFramework/Actor.h` ✅.
   - All five call-site TUs already `#include "Siegebound/SiegeCombatStatics.h"` (`SummonedUnit.cpp:37`, `Tower.cpp:14`, `HeroCharacter.cpp:36`) ✅ — `FSiegeVisionQuery` needs no new include anywhere.
   - `Out.RemoveAll(lambda)` with `(const AActor* Candidate)` matches the shipped veil lambda's signature at `:279` ✅.
   - ⚠️ **`SiegeFogStatics.generated.h` is still the batch's only new reflected header** — an **editor bounce**, not Live Coding, remains the safe compile path (carried from `qa/TASK-847.md` note 2).
4. 📌 **`SC-§43` cl. 4 — THE PATHSPEC CHECK, ALREADY RESOLVED IN YOUR FAVOUR AND WORTH RE-CONFIRMING:** `SiegeCombatStatics.cpp` calls `FSiegeFogStatics::EffectiveVisionRadius` **and** `FSiegeFogStatics::IsVisibleThroughFog` ⇒ `SiegeFogStatics.{h,cpp}` **must** be in the same commit (they already are, per the law's own worked example). ⭐ **Add `Tests/SiegeFogClampTest.cpp` too** (`TL-§5d` cl. 3 / `SC-§43` cl. 5 — a test file always travels with the code it covers); it is **new and untracked**, so if it is omitted the 8 rows are invisible to `HEAD` and **nothing will ever go red to say so**.
5. ⛔ **CENSUS: assert `838`'s DELTA `+8` BY FILE NAME, never an absolute** (`TL-§5b`), and **read §6's `TL-§5d` coincidence warning before you write a number** — today's declared working-tree total is **426/31**, which is numerically identical to the last executed `426 / 0` at `e9df584` and is **a different set**.
6. ⚖️ **Two items route to the MANAGER, not to you:** the **`FOG-§7` amendment** (the *"every AoE radius is under the ceiling"* sentence is measured **FALSE** — `Lightning` 700; §3 above carries the full measurement) and **NIT-1** (`cards.csv`'s `Lightning` Notes say 400 while the data says 700 — the likely origin of the wrong law).
7. 🧑 **ONE SENTENCE FOR JONATHAN, VERBATIM, BECAUSE HE CAN OVERRULE IT AND SHOULD SEE IT BEFORE IT SHIPS:** *"Under fog the hero's line spell still reaches its full 900 uu, so he can hit something at 900 uu that he literally cannot see."* ⭐ It now covers **two** cards, not one — `Lightning`'s 700-uu reticle sits above the ceiling too. Flipping either is **one line** at that spell's gather, and `SiegeFogClampTest` tests 2 and 3 carry the rows to invert.

**Reviewed, not edited.** ⛔ No code, compile, engine, MCP or Git action was taken by this gate.
