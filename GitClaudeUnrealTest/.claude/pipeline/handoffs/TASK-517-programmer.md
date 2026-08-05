# TASK-517 — ROSTER VISIBILITY — `MaxRosterKinds` 8 → 13 **and** `other_kinds:` prints NAMES

**Agent:** gameplay-programmer · **Date:** 2026-08-04 · **Status:** `ready-for-qa` · **QA gate:** TASK-525
**M8 DECLARATION DUTY (verbatim, as required):** *"adds no replicated property, no new replicated class, no new relevancy tier."*

---

## 1. What changed — two halves, both landed

**Files touched (the whole change, nothing else):**
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp`

⛔ `BuildZoneA` was **not touched** — TASK-521 owns every Zone A byte and is unblocked. ⛔ No grammar, no parser, no executor, no console widget, no `Plugins/SiegeLlama/**`, no test file, no CSV. ⛔ No compile, no Git, no editor/MCP/PIE.

### Half 1 — Jonathan's ruling, implemented exactly

`static constexpr int32 MaxRosterKinds = 8;` ⇒ **`= 13`**. Still a `static constexpr int32`; 13, not 12, not "all kinds dynamically".

Its doc comment was **rewritten**, because the old one (*"eight covers a realistic mid-match board"*) was false in exactly the way that caused the defect. The new comment records **whose ruling it is and when**, the **887-vs-893 / ~6-char headroom**, that **a fourteenth commandable kind forces a re-tune**, the ⛔ ban on buying room from `SnapshotTrimBudgetChars`, and the fact that the cap is only half the fix.

### Half 2 — the root-cause fix, and it is the durable one

`USiegeAssistantSnapshot::AppendRosterBlock`'s collapse line:

```
- Out.Appendf(TEXT("other_kinds: %d kinds, %d units\n"), CollapsedKinds, CollapsedUnits);
+ Out.Appendf(TEXT("other_kinds: %s (%d units)\n"), *CollapsedNames, CollapsedUnits);
```

Emitted format, matching the pin character-for-character:

```
other_kinds: none                          <- UNCHANGED, and still ALWAYS emitted
other_kinds: sorcerer (1 units)            <- one kind collapsed
other_kinds: wizard, sorcerer (3 units)    <- two
other_kinds: longbowman, cleric, ogre, wizard, sorcerer (9 units)
```

Canonical symbols, `", "`-separated, in the **same fixed `DT_Cards` row order** the rows above use, then the aggregate unit count. The names are accumulated **in the same pass that tallies the units** — one walk of the collapsed tail — so the line can never disagree with the rows above it about *what* was hidden or *in what order*.

The fixed-key law is untouched: the key is always emitted, and `none` is byte-identical to what shipped.

### Also updated, because they became untrue

| where | was | now |
|---|---|---|
| `.h` `SnapshotTrimBudgetChars` comment (was `:279-285`) | *"At the shipped MaxRosterKinds = 8 … RAISE MaxRosterKinds TOWARD 13 AND IT BITES IMMEDIATELY (887 against 893)"* — a warning about a future edit | the **shipped operating point**, with the head/roster/tail arithmetic spelled out |
| `.cpp` collapse `Warning` string | *"The grammar still admits all %d kinds, so the model can name a unit the prompt never showed it."* | *"…and `other_kinds:` NAMES every collapsed one, so the model can still SEE each symbol — what a collapse costs is the per-kind COUNTS, which degrades an order into a clarification rather than a refusal."* |
| `.cpp` comment above the collapse test | same now-false claim in prose | narrowed, with the reason the log is **still** worth firing |
| `.h` `FSiegeAssistantRosterEntry::Orderable` comment (`:79-80`) | *"…and the t0 tripwire cannot move"* | annotated: the claim **about that field** still holds, but the **shipped-lane** t0 tripwire is retired (670 → 887) |

---

## 2. ⭐ The property worth stating: this CONVERGES the two lanes

The spike fixture (`SiegeLlamaSpike.cpp`) **always printed all 13 kinds plus a hardcoded `other_kinds: none`**; the shipped builder printed 8 plus a computed collapse line. At cap 13 the shipped builder emits 13 rows and **computes** `none` — **byte-identical** to the fixture's hardcoded line.

I verified this by reproducing **both** builders' formats from source and re-deriving the recorded figure: the shipped builder on a t0-shaped board measures **670 chars before** this change and **887 after** — and 887 is precisely the number QA measured on the fixture. **t0's sealed bytes were not touched, the harness was not touched, and no fixture was shrunk.** The lane that moved is the one that was wrong about the game. (Head 108 / roster 621 / tail 158 reproduce 887 exactly, which is what makes the arithmetic below a measurement of the artifact rather than an estimate.)

---

## 3. The char-budget arithmetic — from the artifact, not from memory

Board: the 13-kind t0 shape, default 61-char `order:` line (`send 10 footmen with a sorcerer to the nearest ancient ground`), `pending: none`.

```
Head  ([FORCES] + all 7 places)                       108
Roster block, 13 kinds printed, other_kinds: none     621   <-- THE NUMBER ASKED FOR
Tail  (stances, hero, pending, [ORDER], order)        158
                                                    -----
Zone C                                                887

Budget  = SnapshotTrimBudgetChars - ZoneBCharReserve = 1085 - 192 = 893
RosterBudget = 893 - 108 - 158                       = 627
Roster block                                          621
                                                    -----
HEADROOM                                                6
```

- **Exact char length of a 13-kind roster block: 621.** Zone C total: **887**. Zone B+C against the trim budget: **1079 of 1085**.
- **Headroom for a default-length `order:` line: 6 chars.**
- ⛔ **Extra `order:` characters that re-collapse the first kind (the Sorcerer): 7.** That is the unflattering number and it is the one Jonathan's fix actually turns on — at **7** extra typed characters `KindsToPrint` drops to 12 and the line becomes `other_kinds: sorcerer (1 units)`. **The Sorcerer's *name* survives that; before this change its name did not.**

Supporting figures, same method:
- Old format vs new, at the same `KindsToPrint`: **+36 chars** with 5 kinds collapsed, **+2** with one. Every char of that is bought by a roster row that is no longer printed (a row costs `36 + len(symbol) + digits` = **43–49** on today's kinds; the same symbol on the collapse line costs `len(symbol) + 2`).
- Shrink-loop monotonicity **proved, not assumed**: the symbol cancels between the two, so each step shrinks the block by ≥ ~29 chars regardless of naming. Measured ladder: `621 → 588 → 551 → … → 182` at one kind printed. `KindsToPrint <= 0` remains the unconditional floor.
- Worst case (**both** player lines at `MaxUtteranceBytes = 240`, four-digit stances, all 7 places): head 108, tail 583, `RosterBudget` **202** — the block prints **1 kind** at **182 chars** and names the other twelve. **The zero-row floor is unreachable today** (a zero-row block would be 152 chars, up from 47 under the old format — an overshoot that is bigger but still never reached).

⚠️ **Qualifier on all of the above:** counted on **t0's single-digit tallies**. A board with three-digit counts adds ~2 chars per row (~26 total) and eats the headroom on its own. That is the same risk as the 14th kind, arriving sooner.

---

## 4. ⚠️ NAMED RISK — the 6-character headroom (for TASK-528)

| | |
|---|---|
| **Risk** | 887 of 893. **~6 chars.** A 14th commandable kind, three-digit unit counts, a longer `pending:` line, or **7** extra typed `order:` chars re-collapses the tail. |
| **Owner-accepted** | ✅ Jonathan was told the number and accepted it, including *"a future unit kind forces a re-tune."* ⛔ Not re-escalated here. |
| **Why survivable** | ⭐ `other_kinds:` now prints NAMES. The failure mode drops from *"the model refuses a unit that is alive"* to *"the model does not know how many of them there are."* A clarification, not a refusal. |
| **The one lever** | `ZoneBCharReserve = 192` over-charges by ~124 chars (recorded readings 68 / 71). Lowering it can only WIDEN the roster. ⛔ **TASK-528 owns it and it is MEASURE-FIRST: it must be set from a PRINTED `zoneB_chars` off a live `FIRST LIVE CAPTURE` line from THIS builder — never derived, never from the spike's `AppendZoneB`.** I did not touch it. |
| ⛔ **Banned** | Raising `SnapshotTrimBudgetChars`. Its own header forbids it; `AS-§19`'s three-role table is why. I did not touch it. |

---

## 5. Tests I expect to move — **and the headline is an absence**

⭐ **NO shipped automation test asserts Zone B/C character counts or roster shape. Not one.** I grepped every `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in `Source/` and `Plugins/`: `BuildZoneB`, `BuildZoneC` and `AppendRosterBlock` appear in **zero** test files.

> ⛔ **That absence is the finding, and TASK-523 should read it as one: the roster block — the single thing the model reads unit names out of — has no coverage at all, which is exactly why this defect shipped and why nobody caught it. It was not that a test was wrong; there was no test.**

**Verdict for TASK-523: TASK-517 moves NO existing test.** In detail:

| thing | file | moves? |
|---|---|---|
| `Siegebound.Assistant.ZoneA.TwoLaneByteEquality` | `Tests/SiegeAssistantZoneATest.cpp` | ❌ **Not by me.** Zone A only; I did not touch `BuildZoneA`. It moves for **TASK-521**. |
| `Siegebound.Assistant.ZoneA.MeasuredCharCount` (5116) | same | ❌ **Not by me**, same reason. |
| `Siegebound.Assistant.Guard.*` (8 tests) | `Tests/SiegeAssistantGuardTest.cpp` | ❌ They build `FSiegeAssistantRosterEntry` arrays by hand and call `ValidateCommandAgainstSnapshot`. They never render a zone. |
| `Siegebound.Assistant.Grammar.*` (12 tests) | `Tests/SiegeAssistantGrammarTest.cpp` | ❌ Grammar is built from `GetUnitKinds()`, which is **never** truncated — unchanged by the cap. |
| `Siegebound.Keyboard.*` / `Siegebound.Settings.*` | — | ❌ Unrelated. |

**Non-test assertions and diagnostics that carry numbers which move — reported so TASK-523/525 can route them, ⛔ I did not edit them (not my files):**

1. **`Plugins/SiegeLlama/.../SiegeLlamaSpike.cpp:5785`** — runtime tripwire `if (!bUseT1 && (ZoneB.Len() != 68 || ZoneC.Len() != 887) …)`. ✅ **STAYS GREEN and must NOT be "aligned".** It measures the **spike's own** `AppendZoneB`/`AppendZoneC`, which I did not touch. ⛔ Anyone who reads the retirement of the *shipped-lane* 887 tripwire as licence to edit this line has crossed the lane boundary `FT-§16` protects.
2. **`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp:3005`** — the `FIRST LIVE CAPTURE` reference line says *"the SHIPPED builder was DERIVED at B+C=738."* **That figure is now stale** (shipped B+C on a 13-kind board ≈ 68 + 887). It is a diagnostic, not an assertion, so nothing fails — but an operator taking the owed `zoneB_chars` reading will compare against a dead number.
3. **`Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp:670`** — ⛔ **this text becomes FALSE, and it is the sharpest of the three.** It prints *"EXPECTED ROSTER DIVERGENCE ON A 13-KIND BOARD … this lane prints MaxRosterKinds=8 rows plus a COMPUTED 'other_kinds: <N> kinds, <M> units'; the harness prints 13 rows plus a HARDCODED 'other_kinds: none'. … do NOT 'align' anything."* **After this change both lanes print 13 rows and `other_kinds: none`.** A tool that announces a divergence that no longer exists will send its next reader hunting a defect that is not there. Needs a one-paragraph rewrite by whoever owns `SiegeCheatManager.cpp`.

Also worth TASK-523's attention when it writes `CollapseNamesTheHiddenKinds`: **forcing a collapse on a 13-kind board takes exactly 7 extra `order:` characters** past the 61-char default (or any `pending:` text at all). That is a cheap, deterministic way to drive the case.

---

## 6. Spec items answered explicitly

- **(4) "does the collapse log still say something true at the new cap?"** — **Checked by reading, and one string needed fixing.** The **cause** strings are fine: at cap 13, `CapKinds == UnitKinds.Num()` on every live board (DT_Cards has exactly 13 commandable kinds), so `bBudgetBite` is **always true** and the only reachable cause is `"the CHARACTER BUDGET, below the MaxRosterKinds cap"` — which is true, and is itself the proof that the cap raise alone would not have closed the defect. I **kept** the now-unreachable `"the MaxRosterKinds cap"` branch (a 14th kind makes it reachable the day it is added) and said so in a comment rather than deleting it. What was **not** true any more was the `Warning`'s claim that *"the model can name a unit the prompt never showed it"* — that is the defect I just fixed, so the sentence was rewritten rather than left to reassure a future reader about a hazard that had changed shape.
- **(3) the elastic trimmer** — untouched. `RosterBudget`, the shrink loop, `SnapshotTrimBudgetChars`, `ZoneBCharReserve`: all byte-identical. Only comments were added around them.
- **(6) `BuildZoneA`** — untouched, verified by diff.
- **Zone C bytes changed**, as authorised: `MaxRosterKinds` only, under the D2 resolution superseding §12c for this constant.

---

## 7. What QA should scrutinise

1. **The collapse-line format against the pin**, character for character: `other_kinds: sorcerer, cleric (5 units)` and `other_kinds: none`. Separator is `", "`; the aggregate is in parentheses; the key is always emitted.
2. **`(1 units)`** — deliberate and declared, not an oversight. The format is pinned as `(<N> units)`; a pluralisation branch would spend chars out of a 6-char headroom and make the line's bytes board-dependent. **If TASK-525 wants pluralisation it is a ruling, not a fix.**
3. **Order agreement** — the collapse line walks `UnitKinds[PrintCount .. Num)`, the same array and the same fixed card-row order the printed rows use. There is no second sort and no second source.
4. **`KindTotals.IsValidIndex` guard** — kept exactly as it was on the units tally. The name walk uses `UnitKinds[KindIndex]` directly, which is safe by the loop bound.
5. **`CollapsedNames.Reserve(...)`** — `FMath::Max(0, UnitKinds.Num() - PrintCount) * 14` is a heuristic reserve, not a bound; over/under-shooting only costs a realloc.
6. **The arithmetic in section 3** — re-derivable from the two builders' `Appendf` formats plus `SpikeRoster`'s 13 rows. I reproduced both lanes to get it; **887 falling out of the shipped builder's own format is the check that the model is right.**
7. **Whether TASK-521 should now say, in Zone A, that `other_kinds:` names ARE `[FORCES]` members** — TASK-521's item (4) asks exactly this and my output is its input. ✅ My read: the refusal rule's hazard is **largely** closed because the symbol is now visible, but Zone A currently frames `[FORCES]` as the roster rows; a reader-model could plausibly treat the `other_kinds:` line as *not* part of the roster. **That is TASK-521's call, not mine — but it should be made deliberately and stated either way.**

---

## 8. Anything in the spec I think is wrong

Nothing is wrong. Three refinements, all made and declared above rather than silently:

- The spec's *"~46 chars"* for a roster row is a fair average; the true figure is **43–49** (`36 + len(symbol) + digits`). Direction and conclusion unchanged — the names are still strictly cheaper than the rows they replace.
- The spec asks for *"the exact char length of a 13-kind roster block"* — **621**; the **887** in the law is Zone C **including head and tail**. Both are reported so the two numbers are not confused for each other.
- **The named-collapse format makes the zero-row floor case bigger** (152 vs 47 chars), which the law does not mention. It is **unreachable** today with a ~20-char margin, it is proved in a comment at the loop, and it is reported here rather than left for someone to find at 2 a.m.
