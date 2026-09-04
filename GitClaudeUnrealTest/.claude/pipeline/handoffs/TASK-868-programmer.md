# TASK-868 — REPAIRING THE RED ROW IN `SiegeAcquisitionFunnelTest` TEST 9 — programmer handoff

**Status → `ready-for-qa` (gate `TASK-869`).**
**Files touched: EXACTLY ONE — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAcquisitionFunnelTest.cpp`.**
⛔ Zero production edits · ⛔ zero other test files · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git.

---

## 0. THE HEADLINE

✅ **Test 9's guard is corrected and it is now STRICTER than the row it replaces, not looser.** The pin stayed **EXACT** (`TestEqual`, never `>=`), every admitted site is **named and justified**, and the guard is proven to go **RED** on seven distinct violations — including **a write-door whose name does not exist yet**, which the original row could not have caught either.

⛔⛔ **AND A SECOND, PRE-EXISTING RED ROW IN THE SAME FILE THAT NOBODY HAS BOARDED: `TEST 1` IS ALSO FAILING, FOR A STRUCTURALLY IDENTICAL REASON.** It is **outside my fence**, so per spec (5) I have **REPORTED IT AND NOT REPAIRED IT** — see **§8**. ⚠️ **`TASK-868` alone does NOT turn this file green. `TL-§5c` will still see a nonzero `Result={Fail}`, and both commits will still be blocked, unless test 1 is boarded too.** That is the single most important sentence in this handoff.

---

## 1. ⛔ THE PREMISE, MEASURED BEFORE ANYTHING WAS CHANGED (spec item 0, `SC-§40` cl. 3)

⚠️ **HOW I "RAN THE ROW" — stated plainly, because the honest answer is not "I executed the suite".** I am fenced out of compiling, so I could not invoke `Automation RunTests`. Instead I wrote a **faithful line-for-line replica** of `SiegeAcquisitionFunnelFixture::CountOccurrencesInCode` (same skip rule — trimmed form starting `//`, `* `, `*/`, `/*`, or equal to `*`; same case-sensitive scan; same advance-by-needle-length) and evaluated **every assertion in the row** against the live tree. ⛔ **This is a replica, not an execution — the only proof that counts is the commit build's `Result={Fail} 0`** (`TL-§5c`). It is the same instrument `TASK-867` used and it reproduced that task's published figures exactly, which is my control that the replica is faithful.

**MEASURED — the row was genuinely RED, so the two commits were not being held on a phantom:**

| file | `bIsInvisible` | `FSiegeInvisibilityStatics` | `IsVisibleTo(` | `FSiegeFogStatics` | `EffectiveVisionRadius` |
|---|---|---|---|---|---|
| **`SummonedUnit.cpp`** | ⛔ **3** | ⛔ **4** | ✅ 0 | ✅ 0 | ✅ 0 |
| `Tower.cpp` / `HeroCharacter.cpp` / `SpellLineSweep.cpp` / `SiegeCheatManager.cpp` | ✅ 0 | ✅ 0 | ✅ 0 | ✅ 0 | ✅ 0 |

⇒ **2 failing assertions**, both on `SummonedUnit.cpp`, both pinned at 0. **`TASK-838`'s relayed measurement is CONFIRMED exactly** — and confirming it is reported here even though it matched, per `SC-§40` cl. 3.

---

## 2. ⭐⭐ THE ROOT CAUSE — AND IT IS MORE INTERESTING THAN "THE NUMBER WAS STALE"

The original row asserted **five tokens read zero in five files**, and its own comment promised this was *"THE PERMANENT LAW… ⛔ including for TASK-829 and TASK-838, which must NOT relax this row."*

⛔ **That promise was unkeepable the moment `TASK-829` landed.** `WITCH-§6` **requires** the veil's two write-doors to live on `ASummonedUnit` — so two of the five tokens became **legitimately non-zero in exactly one of the five files**, and the row went red **on correct code**.

⭐ **THE DISTINCTION THE ROW BLURRED, WHICH IS THE WHOLE FIX:** it bundled two different claims under one list.
- **A DECISION claim** — *"does this site decide whether a unit is visible?"* — the real law (`WITCH-§1`), permanently true, and **still true today**: `IsVisibleTo(` reads **0** in all five files.
- **A STATE claim** — *"does this site name the flag at all?"* — a **PROXY** that only held while the veil had **no implementation**.

⇒ ⚖️ **A guard whose subject is a PROXY expires when the thing it proxies for gets built — and it expires by going RED ON THE FEATURE LANDING, which is precisely the moment everyone is most tempted to loosen it.** That is why spec item (3) was right to forbid the range.

---

## 3. ✅ THE FIX — WHAT THE PIN IS NOW, AND WHY EACH ADMITTED SITE IS LEGITIMATE (spec 3a)

Test 9 is now four blocks. **Every pin is `TestEqual` on an exact integer. There is no `>=`, no range, no tolerance, and no deleted assertion.**

**BLOCK A — DECISION tokens, ⛔ ZERO in all five files. Needles UNCHANGED.**
`IsVisibleTo(` · `FSiegeFogStatics` · `EffectiveVisionRadius`. This is the half carrying the permanent law and I did not touch it. (I deliberately left the two bare fog needles bare: for a **zero-pin** the bare token is the *wider* net, so "improving" them to call shapes would have **weakened** a green row for no benefit — and `TASK-867` flagged `EffectiveVisionRadius(` as a pin family to leave alone.)

**BLOCK B — STATE tokens, ⛔ ZERO in the FOUR NON-OWNER files.**
`bIsInvisible` · `FSiegeInvisibilityStatics`, asserted zero in `Tower.cpp`, `HeroCharacter.cpp`, `SpellLineSweep.cpp`, `SiegeCheatManager.cpp`. `SummonedUnit.cpp` is exempted **BY NAME**, with the reason in the code. A mirrored flag elsewhere is a second source of truth and still an automatic fail.

**BLOCK C — the instrument control (§4).**

**BLOCK D — ⭐⭐⭐ THE CLOSED WRITE-DOOR CENSUS on `SummonedUnit.cpp` — this is what replaces the exemption, and it is STRICTER than the zero it replaces.**

| # | pin | value | why this site is legitimate |
|---|---|---|---|
| 1 | `FSiegeInvisibilityStatics::ApplyVeil(` | **== 1** | The **ONE write-true door**, inside `ASummonedUnit::GrantInvisibility`. `WITCH-§6` puts it on the class that owns the flag. |
| 2 | `FSiegeInvisibilityStatics::ApplyBreak(` | **== 1** | The **ONE write-false door**, inside `ASummonedUnit::BreakInvisibility`. Its true-exactly-once edge is why `WITCH-§6` can ban a "was visible" cache. |
| 3 | `(bIsInvisible` | **== 2** | ⭐ **THE CLOSURE PIN.** Both sites are doors 1 and 2. This is the row that catches **a write-door whose name does not exist yet**. |
| 4 | inlined assignment, derived: `bIsInvisible =` − `bIsInvisible ==`, plus the no-space twin | **== 0** | `WITCH-§6` bans an inlined `bIsInvisible = false` **by name**. Pin 3 cannot see this shape — a bare statement has no parenthesis. |
| 5 | prose-immunity of needles 1–3 | skip-on **==** skip-off | Proves each pinned number reads **structure**, not documentation. |

**⭐ THE THIRD ADMITTED SITE, NAMED AND DELIBERATELY *NOT* PINNED — `FSiegeInvisibilityStatics::ToString(Reason)`** (the `Verbose` log line in `BreakInvisibility`). It is a **pure diagnostic that touches no state**. ⛔ Pinning it would make **adding a log line a suite failure** — and a pin that fires on a log line is exactly how a guard gets loosened by the next person in a hurry. **Measured: 1 today; deliberately unbounded.**

⭐⭐ **WHY PIN 3 IS THE LOAD-BEARING ONE.** Pins 1 and 2 only know **today's two static names**. A third entry point invented next month (`ApplyDecay`, `ApplyRefresh`, …) taking `bool&` would sail past both. Every write-door must hand the flag to something **by reference**, so counting parenthesised mentions of the flag bounds the door set **without knowing the doors' names**. Mutation **M3** below proves exactly this: pins 1 and 2 stay green, pin 3 goes red.

---

## 4. ⛔ INSTRUMENT DISCIPLINE — BOTH DIRECTIONS, AND WHICH CONTROL EXERCISED WHICH (`SC-§39`)

⛔ **The over-count direction is LIVE IN THIS TEST'S OWN SUBJECT, and it is the reason the old numbers must never be re-pinned.** `SummonedUnit.cpp`'s `#include "Siegebound/SiegeInvisibilityStatics.h"` carries a **trailing `//` comment naming both state tokens**. It sits on a **code line**, survives every skip rule, and is counted.

| needle on `SummonedUnit.cpp` | skip-aware (what the suite sees) | raw (skip off) | truth |
|---|---|---|---|
| `bIsInvisible` (bare) | **3** | 7 | **2** real code sites |
| `FSiegeInvisibilityStatics` (bare) | **4** | 5 | **3** real calls |
| `(bIsInvisible` | **2** | 2 | **2** ✅ immune |
| `…::ApplyVeil(` | **1** | 1 | **1** ✅ immune |
| `…::ApplyBreak(` | **1** | 1 | **1** ✅ immune |

⇒ ⛔ **Pinning `bIsInvisible == 3` would have pinned THE WORDING OF A COMMENT** — in a file `TASK-830` is editing **right now** (mtime 23:04, ten minutes before I measured). It reads **one too high** in the over-count direction *and* has **4 real hits eaten** in the under-count direction. **It is wrong in both directions at once.** That is why every needle I pinned carries an open paren or an assignment operator.

**THE SHIPPED CONTROLS, and which direction each exercises — both are EXECUTED ASSERTIONS, not handoff prose:**

I added `CountOccurrencesIncludingComments` to the fixture (the same scan with the skip rule off — a **control**, not a better scanner; documented **test 9 only**), and a **synthetic, in-memory probe**:

| control | direction | assertion |
|---|---|---|
| probe, bare needle | ⭐ **OVER-COUNT** | reads **2** where the truth is **1** — the trailing `//` manufactures a hit |
| probe, call-shaped needle | (truth) | reads **1** — the paren tells a CALL from PROSE |
| probe, call-shaped, skip off | ⭐ **UNDER-COUNT** | reads **2** — the block-comment line hides one |
| live file, needles 1–3 | **immunity** | skip-on == skip-off ⇒ pinned numbers are instrument-independent |

⭐ **The probe is SYNTHETIC on purpose.** A control read off the live tree goes red when somebody reflows a comment — a false alarm on a row that blocks commits. Its symbol `FSiegeVeilProbeControlSymbol` was **MEASURED ABSENT** before use (`SC-§40` cl. 10): **0 hits across 266 source files**, with `FSiegeInvisibilityStatics` at **75 hits** as the positive control that the absence scan was not itself blind. ⛔ I did not assume it absent — `TASK-838`'s near-miss with the "synthetic 700" that turned out to be Lightning's live `AoERadius` is exactly the trap.

---

## 5. ⭐⭐ THE RED-PROOF — "DOES IT STILL GO RED?" ANSWERED BY MUTATION, NOT BY ASSERTION

I mutated `SummonedUnit.cpp` **in memory only** (⛔ the file on disk was never written) and re-evaluated all 38 assertions of the rewritten row against each mutant.

| mutation | required | failures | result |
|---|---|---|---|
| **M1** a SECOND `ApplyVeil(` (duplicate write-true door) | RED | 2 | ✅ pins 1 + 3 fire |
| **M2** a SECOND `ApplyBreak(` at a new site | RED | 2 | ✅ pins 2 + 3 fire |
| **M3** ⭐ a **THIRD, UNKNOWN** static door (`ApplyDecay(bIsInvisible)`) | RED | 1 | ✅ **only pin 3 fires — the case name-pins cannot catch** |
| **M4** inlined `bIsInvisible = false;` | RED | 1 | ✅ pin 4 fires |
| **M5** inlined `bIsInvisible=false;` (no spaces) | RED | 1 | ✅ pin 4 fires |
| **M6** a suppression **DECISION** at the call site (`IsVisibleTo(`) | RED | 1 | ✅ Block A still fires |
| **M7** a pinned needle written **inside a comment** | RED | 2 | ✅ immunity rows fire |
| **M8** an extra `ToString(` diagnostic | GREEN | 0 | ✅ stays green, as designed |
| **M9** a parenthesised `==` read | *(declared over-catch)* | 1 | ⚠️ fires — **see §6** |
| **M10** the `#include`'s trailing comment reflowed away | GREEN | 0 | ✅ **robust to the edit `TASK-830` is most likely to make** |
| **M11** every comment mention of the flag stripped | GREEN | 0 | ✅ after the §6 removal |

**Live tree: 38 assertions, 0 failures.** ⇒ ⛔ **The row is not merely green — it is green for reasons I can name, and red for seven distinct reasons I can demonstrate.** A test that can no longer fail would be worse than the red one I started with; this one fails on strictly more than it used to.

---

## 6. ⚠️ TWO THINGS I CHANGED MY MIND ABOUT, RECORDED BECAUSE QA SHOULD RULE ON THEM

**(a) ⛔ I REMOVED A ROW FROM MY OWN FIRST DRAFT.** It asserted, on the live file, that the bare token reads strictly more than the structural needle (7 / 3 / 2) — a vivid demonstration that the bare token is the wrong instrument. **Mutation M11 caught it going red when comments were stripped.** Its truth depended on how many comment lines happen to quote `WITCH-§6` ⇒ **a row that goes red for a reason that is not in the diff**, which is exactly what `SC-§39`'s closing warning says tempts the next person to loosen the guard — on a test that blocks commits, in a file under active edit. **Nothing was lost**: the synthetic probe proves both directions deterministically and can never false-fire, and the immunity rows prove every *pinned* number is comment-independent. The demonstration now lives in a comment; only the invariant is asserted.

**(b) ⚠️ THE CLOSURE PIN HAS A DELIBERATE OVER-CATCH, DECLARED RATHER THAN HIDDEN.** `(bIsInvisible` also fires on a **parenthesised READ** — e.g. `if (bIsInvisible == false)` (M9). ⛔ **I kept it, and here is the trade:** pin 3 is the *only* thing that catches M3 (an unknown future door), which is the highest-value guarantee in the block. The cost is that a raw read in the owner file goes red — and a raw read there is genuinely off-pattern, because the class already exposes `IsInvisible()` and `WITCH-§6` routes every rule through the statics. The failure message says all of this and tells the reader the two legitimate responses. ⚖️ **I judged an over-catch that fails LOUD preferable to an under-catch that fails SILENT — but this is a judgement call and QA should overrule it if it disagrees.** (Note the *assignment* pin does **not** have this problem: the `==` count is subtracted, exactly so an honest comparison cannot masquerade as a violation.)

---

## 7. ✅ CROSS-PIN CHECK — DOES MY CHANGE DISTURB ANY OTHER PINNED COUNT? (the `TASK-867` warning)

I enumerated every source-scanning assertion in the tree and checked its **scope**. ⭐ **The decisive fact: every tree-wide pin except one lane skips `/Tests/` via `IsAutomationTestFile`, and my file lives under `/Tests/` — so my new text is invisible to them by construction.**

| pin | scope | measured | pinned | disturbed? |
|---|---|---|---|---|
| qualified interface-enum needle, **whole tree incl. Tests/** | tree | 1 | 1 | ✅ no |
| bare interface-enum needle, shipping | shipping | 1 | 1 | ✅ no |
| `UTeamAgent::StaticClass()`, shipping | shipping | 1 | 1 | ✅ no |
| `FSiegeInvisibilityStatics::IsVisibleTo(` — *the twin that nearly took `TASK-851` down* | shipping | 2 | 2 | ✅ no |
| `FSiegeFogStatics::EffectiveVisionRadius(` | shipping | 2 | 2 | ✅ no |
| `ReadFogState(` | shipping | 3 | 3 | ✅ no |
| `return false;` inside `ReadFogState` body | `SiegeCombatStatics.cpp` | 2 | 2 | ✅ no |
| tree-wide `bIsInvisible` allowed-file census (`SiegeInvisibilityTest.cpp`) | shipping | 13 | asserted `> 0` only | ✅ no |
| ⛔ **bare interface-enum needle, `Tests/` ONLY** | **Tests/** | **3** | **1** | ⛔ **ALREADY RED — see §8** |

⚠️ **The one literal I had to avoid** was the bare interface-enumeration token, because test 1's Tests/-lane exemption is the sole pin that reads my file. **I wrote it nowhere.** My additions contribute **0** to it — verified by counting.

---

## 8. ⛔⛔⛔ THE FINDING THAT MATTERS MOST — **TEST 1 IS ALSO RED, IT IS PRE-EXISTING, AND IT IS OUTSIDE MY FENCE**

⛔ **`FSiegeAcquisitionFunnelGrepGateTest` (test 1) has exactly ONE failing assertion**, and it will block the same two commits.

**Replicated, all eight of its assertions:**

| assertion | got | want | |
|---|---|---|---|
| composed-needle self-check | ok | ok | ✅ |
| **A** qualified needle, whole tree | 1 | 1 | ✅ |
| **B** bare needle, shipping | 1 | 1 | ✅ |
| ⛔ **C** bare needle, **`Tests/` named exemption** | **3** | **1** | ⛔ **FAIL** |
| **D** `SiegeGhostPawn.h` names it (self-check) | true | true | ✅ |
| **E** comment-skip control == 0 | 0 | 0 | ✅ |
| **F/G** `UTeamAgent::StaticClass()` lanes | 1 / 1 | 1 / 1 | ✅ |

**THE TWO EXTRA HITS, both inside `SiegeAcquisitionFunnelTest.cpp` itself, both PRE-EXISTING and NOT MINE:**
1. **Test 1's own registered automation-test name string** — `"Siegebound.Acquisition.GetAllActorsWithInterfaceAppearsExactlyOnceInSource"`. I read this verbatim at the file's original line 196 **before making any edit**.
2. **A `TEXT(...)` failure message in a later test** quoting the token in prose about `SpellLibrary`'s one call feeding three loops.

⭐⭐ **THE IRONY, AND IT IS THE SAME DEFECT CLASS AS TEST 9:** the assertion's own message asserts the false premise in prose — *"⭐ **THIS FILE contributes ZERO by construction** (its needles are composed at runtime), which is what lets the gate scan itself."* ⛔ **The runtime composition protects the NEEDLE. It does not protect the test's own NAME, which spells the token out in full on a code line.** The guard's premise about its own file stopped being true; **the code it guards is correct.**

✅ **THE GUARANTEE ITSELF IS INTACT — this is bookkeeping, not a second enumeration.** Lanes **A** (qualified, whole tree) and **B** (bare, shipping) both read **1**. There is **no** second interface enumeration anywhere. The failure is purely in the automation-lane exemption count.

⚠️ **AND IT LOOKS BORN-RED.** Hit (1) is test 1's *own name*, present since `TASK-828` wrote the file ⇒ the exemption has been ≥ 2 against a pin of 1 from the moment it shipped. ⇒ ⭐ **this is `TL-§5c`'s own case study repeating: a test that has never been executed cannot be known to be red, and the suite has not run since `8910c17`.**

⛔ **I DID NOT FIX IT.** Spec (5) fences me to *"⛔ Do NOT touch any other test in the file"* and says a real defect found elsewhere is to be **reported, not repaired**. ✅ **The smallest correct fix, for whoever boards it:** count the Tests/-lane exemption **per named file** (`SiegeGhostPawnTest.cpp` == 1, `SiegeAcquisitionFunnelTest.cpp` == 2 — its own name plus its own prose), or exclude the gate's own file from lane C and say so — ⛔ **never by relaxing lane A or lane B, which are the ones that actually hold `WITCH-§1`.**

---

## 9. CENSUS — `TL-§5c`

- ⭐ **MY DELTA: `0`**, exactly as the spec required. `SiegeAcquisitionFunnelTest.cpp` declared **9** tests before my diff and declares **9** after. I fixed an existing test; I added none and removed none.
- **Tree total: `410 declared across 30 files`.** ⚠️ The dispatch cited **408 across 30** — **the tree moved by +2 under me while I worked**, from another lane, not from my diff. **Reconciled as a delta, never as an absolute** (`TL-§5b`).
- ⛔ **`declared` — NOT a pass count. I executed nothing.** The 38-assertion figure above comes from a **replica** of the house scanner, not from `Automation RunTests`. ⛔ **My fix is proven only by the commit build's executed `Result={Fail}` pair** (`TL-§5c`), and per §8 that pair will **not** read 0 until test 1 is also boarded.
- File grew 1097 → 1389 lines (+292, overwhelmingly comment).

---

## 10. ⛔ WHAT QA SHOULD SCRUTINISE (gate `TASK-869`)

1. ⭐⭐ **§8 FIRST — confirm test 1 independently and route it.** If this gate passes `TASK-868` and the commit build then fails on test 1, the loop cost is two blocked commit builds. **Please verify my count rather than take my word** (`SC-§40`: my measurement is a citation, not a fact).
2. ⛔ **Is the pin still EXACT?** Every one is `TestEqual` on an integer. There is no `>=`, no range, no tolerance, no deleted assertion. Block A is byte-identical in its needles.
3. ⭐ **§6(b) — the declared over-catch on `(bIsInvisible`.** This is the one judgement call in the diff. Rule on it explicitly.
4. ⛔ **The `ToString(` non-pin (§3).** I admitted a third site and deliberately left it unbounded. If you think a diagnostic should be pinned, say so — but note M8.
5. ⛔ **The new fixture helper `CountOccurrencesIncludingComments`.** Purely additive; no existing symbol changed; used by test 9 only; documented as a control, not a replacement. Confirm no other test's count moved.
6. ⚠️ **The compile hazard I self-caught:** my first draft put a literal block-comment opener inside a `/** */` docstring. It is now spelled out in prose. ⛔ **I cannot compile — please eyeball the new block for anything similar**, especially the `InstrumentProbe` string literal, which legitimately contains comment digraphs **inside a string** (safe) rather than in a comment.
7. ⭐ **`SummonedUnit.cpp` is moving under all of us** (mtime 23:04 during my task, `TASK-830` item 8). My pins are proven invariant under the six `BreakInvisibility(…)` verb sites and the one `GrantInvisibility()` caller **already in the tree** — none of them moved pins 1–4. M10 proves robustness to the likeliest comment edit.

---

## 11. FENCE COMPLIANCE

✅ One file, and only what the guard needed · ✅ `SummonedUnit.{h,cpp}` **never written** (read-only; all mutation was in memory) · ✅ no `SiegeCombatStatics`, `SiegeFogStatics`, `SiegeBotController` · ✅ no other test in the file altered (test 1 **reported**, not repaired) · ✅ no compile, no editor, no MCP, no Git · ✅ suite delta 0 · ✅ no pass count claimed.
