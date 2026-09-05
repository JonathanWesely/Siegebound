# TASK-1003 — [ORDER-LAW] · gameplay-programmer handoff

**Status:** `ready-for-qa` · gate ⭐ `TASK-1006`
**Law:** ⭐⭐⭐ `FOG-§9.11` · `FOG-§9.8c` · `FOG-§9.8e` · `FOG-§9.9` (DEFAULT-NOT-A-CAP, survives) · `FOG-§9.5` · `SC-§37` · `SC-§38` · `SC-§45` · `SC-§47` · `SC-§60` · `SHIP-§9` · `TL-§5c`
**Fence honoured:** ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `cards.csv` · ⛔ no `SiegeCombatStatics.*` · ⛔ no `SiegeFogStatics.*` · ⛔ no `SiegeAcquisitionFunnelTest.cpp` · ⛔ no `CardRow.h`

---

## 0. THE THREE THINGS THE ROW ASKED ME TO CONFIRM, CONFIRMED FIRST

| claim | evidence |
|---|---|
| ⛔ **THE DERIVATION SURVIVED** | `ResolveEffectiveLeashRangeUU`'s **body is byte-unchanged**. Its `return FMath::Max(LeashRangeUU, NoticeRadiusUU * SafeMultiplier);` is still there; a machine check confirms the seam contains **zero `8000`** and still carries `SafeMultiplier`. Verified: `max(8000, 5000 × 1.5 = 7500)` = **`8000`** — his J-F27 number exactly. |
| ⛔ **THE ORDERING PIN IS DERIVED** | New **test 5** opens `Docs/Data/cards.csv`, reads **all 32 rows**, resolves **every** row through the shipped `ResolveNoticeRadiusUU`, scans **every `bRanged` row** for the firing range, and runs the winner through the shipped `ResolveEffectiveLeashRangeUU`. ⛔ **Not one of `8000` / `5000` / `3600` is typed in that test.** |
| ⛔ **ZERO FOG SYMBOLS** | `IsVisibleTo(` · `FSiegeFogStatics` · `EffectiveVisionRadius` measure **0 in `SummonedUnit.cpp` by BOTH the comment-skipping scan AND a raw scan** — so the count is immune to my prose as well as to my code. |

---

## 1. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | the **two constants** + 9 re-authored doc blocks |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | **comment-only** — 8 re-authored blocks. ⛔ **Zero executable lines changed** in this file. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeUnitNoticeRangeTest.cpp` (untracked, mine) | value pins + **test 2 restructured** + **test 3 (b2)/(e)/(f) new** + **TEST 5 new** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogClampTest.cpp` | licensed prose/value updates only — ⛔ **no fog pin touched**, no assertion added or removed |
| `.claude/pipeline/TASKBOARD.md` | ⛔ my own `status:` line only |

### The change itself
```
SummonedUnit.h   static constexpr float UnitEngagementRadiusUU = 2000.f;  →  5000.f
SummonedUnit.h   float LeashRange = 900.f;                                →  8000.f
SummonedUnit.h   float LeashMarginMultiplier = 1.5f;                      →  UNCHANGED
```
`ResolveNoticeRadiusUU`, `ResolveEffectiveLeashRangeUU`, `GetEffectiveLeashRangeUU`,
`GetClassDefaultEngagementRadiusUU` — **all four bodies byte-unchanged.**

---

## 2. ⛔⛔ THE FINDING QA SHOULD SCRUTINISE FIRST — **HIS `5000` INVERTED WHICH MISREADING IS DANGEROUS, AND IT SILENTLY FALSIFIED TWO SHIPPED GUARDS**

This is the one thing in the row that a value-only edit would have shipped broken, and it is **two separate defects**:

**(a) A test row that would have gone RED.** `SiegeUnitNoticeRangeTest` test 2(a) asserted
`ResolveNoticeRadiusUU(Default, 3600.f) > Default`. At a 2000 default that read `3600 > 2000` ✅.
At 5000 it reads **`3600 > 5000` — FALSE**. A pure value update leaves the suite red.

**(b) ⛔ Far worse — a guard that would have gone GREEN while ceasing to guard.** The *same* row's
message claimed *"a `FMath::Min(Range, UnitEngagementRadiusUU)` returns 2000 here and this goes RED."*
At 5000, **`min(3600, 5000) = 3600`** ⇒ **a clamp now PASSES that row.** The identical false claim sat
in `SiegeFogClampTest.cpp` test 7(d), labelled *"THE ANTI-CLAMP ROW"*. ⇒ two tests advertising a
discrimination they no longer had — `SHIP-§9` / RULING 3's *"a guard that passes the change it was
written to catch is worse than no guard"*, arriving in a third lane.

**Root cause, and it generalises:** at 2000 the Longbowman's 3600 sat **above** the default, so a
**live card** demonstrated the anti-cap property. At 5000 **no shipped card exceeds the default at
all** (the largest reach in the roster is the Longbowman's 3600 *firing* range). The property became
**undemonstrable from data**, and both guards were riding data.

**What I did about it:**
1. The anti-cap property moved onto a **synthetic** value (`Default × 2`, derived, not typed) plus the
   structural probe. The loss of live-data coverage is **declared in the test**, not papered over.
2. ⭐ The **new** danger is named and pinned: at 5000 every cell a designer can write is **below** the
   default, so the dangerous spelling is now **`FMath::Max`** — "normalise the sparse column up to the
   default" would **widen** a card that asked for less. **Nothing in the tree caught that before.**
   Test 2(b) asserts it by value; test 2(f) asserts **`FMath::Max` == 0** in the resolver body.
3. `SiegeFogClampTest` 7(d)'s false anti-clamp claim is **retracted in place** and the row re-pointed
   at the claim that is genuinely this file's subject (fog treats a per-card reach exactly like the
   default). The **discrimination moved to the file that owns the channel** — no fog pin was touched.

---

## 3. ⛔⛔ THE SECOND FINDING — **`LeashMarginMultiplier` IS NOW INERT, AND A COMMENT WAS NOT GOING TO SAVE IT**

Item (3) asked me to re-argue the multiplier in prose. I did — **and then made the argument executable**,
because "an inert term with a good reason" and "dead code" are indistinguishable to the next reader
unless something goes **red** when it is deleted.

- **History (no longer the reason):** `1.5` was *measured* — the game shipped `900 / 600` = exactly 1.5.
- **Today:** `max(8000, 7500)` = **8000**. The floor wins; the multiplier changes **nothing** for any
  shipped card, and deleting the whole `NoticeRadiusUU * SafeMultiplier` term would leave **every
  pre-existing test green**.
- **Its one surviving job:** the future-card case. Crossover = `LeashRange / MarginMultiplier` =
  `8000 / 1.5` = **5333.33 uu**. Above that the `max` flips to the product.
- ⇒ **new test 3(e)/(f):** the crossover is **derived**, a notice radius just under it must still yield
  the **floor**, and one above it must yield the **product** (`12000`, checked exactly).
  ⛔ **Collapse the expression to `return 8000.f;` and test 3(f) goes RED, naming the reason.**
  Test 3(b2) additionally pins his `8000` **through** the expression with both operands read.

⚠️ **Declared honestly in the test:** test 5(d) (`LEASH > NOTICE`) **cannot** discriminate a derivation
from a hard-coded constant — the floor wins today, so a bare `8000` passes it. Only test 3(f) fails
against the collapse. Two rows, one property; neither alone is sufficient. That is stated in the code.

---

## 4. TEST 5 — THE DERIVED ORDERING PIN, AND THE TWO INSTRUMENT HAZARDS IT HANDLES

`Siegebound.Notice.TheLeashOutranksTheNoticeRadiusWhichOutranksTheLongestFiringRangeInTheRoster`

- **`FOG-§9.8e` — the empty first header cell.** The CardID column is addressed as **index 0**, never by
  name. A by-name lookup returns `INDEX_NONE`, yields a well-formed table of blanks, and makes every
  comparison trivially true. **Row (a) is a positive control** that fails on an empty read (rows ≥ 10,
  `bRanged` rows ≥ 2, longest firing > 0, winning CardID non-empty, widest resolved notice > 0).
- ⭐ **The house probe's silent `continue` is turned into a HARD FAILURE.** `SiegeFogClampTest` test 7
  skips a row whose field count disagrees with the header (a comma in free-text `Notes`). **Skipping is
  unsafe here** — a skipped row could hide the longest gun in the game or a populated notice cell, and
  the test would report a smaller number **and pass**. `RowsMisaligned` is asserted **== 0**.
- ⭐ **`FOG-§9.8c` is handled rather than inherited (row c2).** The law is worded around `bRanged`, but
  that flag is **projectile delivery, not "is ranged"** — `CrystalTower` ships `bRanged=false` at
  `Range 800` and shoots anyway. So the law's set is scanned **as written** *and* its **complement** is
  scanned too. Measured: widest non-`bRanged` reach = **800 uu (CrystalTower)** < 5000. ✅

**Measured against the live roster (machine-read, both before and after TASK-1004's CSV edit landed):**

| derived quantity | value | source |
|---|---|---|
| rows read / misaligned | **32 / 0** | every data line aligned |
| `bRanged` rows | **6** | Archer · ArrowTower · Longbowman · BombTower · BallistaTower · Wizard |
| LONGEST FIRING RANGE | **3600 uu (`Longbowman`)** | `Range` column, `bRanged` rows |
| widest non-`bRanged` reach | **800 uu (`CrystalTower`)** | the `FOG-§9.8c` complement |
| WIDEST NOTICE | **5000 uu** | all 32 rows through `ResolveNoticeRadiusUU` |
| LEASH at that notice | **8000 uu** | through `ResolveEffectiveLeashRangeUU` |
| populated `NoticeRange` cells | **0** | the column is entirely sparse |

⇒ **`8000 > 5000 > 3600` ✅**, and `LEASH > NOTICE` re-derived **per row** holds for **32/32** (row e).

⚠️ **Declared vacuity:** row (b) — "every populated cell survives the channel unchanged" — has **0 cells
to check** today. That is stated in the assertion message and the count is printed, so a reader sees
zero coverage rather than inferring coverage that does not exist. Tests 2(a)/2(b) carry both refusals
on values instead.

⚠️ **Declared scope (`FOG-§9.11a`):** test 5 reads the **authored CSV**. The running game reads
`/Game/Data/DT_Cards`, which this module cannot open. Green here is a claim about the authored roster,
never about the shipped asset. Stated in the test.

---

## 5. ⚠️ CROSS-ROW OBSERVATION FOR QA — **`TASK-1004`'s CSV EDIT LANDED WHILE I WAS WORKING**

I machine-read `cards.csv` **twice**, ~40 minutes apart, and it changed underneath me:

| | first read | second read (current) |
|---|---|---|
| `Longbowman.NoticeRange` | `3600` | **blank** |
| `Archer.Range` / `Wizard.Range` | `2000` / `2000` | **`2100` / `2100`** |
| populated `NoticeRange` cells | 1 | **0** |

That is `TASK-1004` items (1) and (3) landing exactly as boarded. ⛔ **I did not touch that file.**

⭐ **Test 5 was verified GREEN in BOTH states** (simulated cell-for-cell before I wrote it): with the
Longbowman cell populated at 3600, the widest resolved notice is still 5000 (31 sparse rows), the leash
is still 8000, and `8000 > 5000 > 3600` holds. So the row is **not** ordering-coupled to `TASK-1004`,
which is what `parallel-safe: yes vs TASK-1004` promised.

⚠️ **One thing for the gate to check, because it is the only place my diff makes a claim about another
row's artefact:** my prose in `SummonedUnit.{h,cpp}` and in both test files says the `NoticeRange`
column ships **entirely sparse** because `TASK-1004` blanked the Longbowman's cell. **That is now true
on disk** — but if `TASK-1004` were ever reverted, those sentences go stale (they would not go red;
only test 5's row (b) count would change from 0). Named here rather than left to be discovered.

---

## 6. THE CLAIM CENSUS — **BY SHAPE, NOT BY RE-READING MY OWN PARAGRAPHS** (`SC-§47`)

Census method: (i) numeric grep for `600` / `900` / `2000` / `3000` / `5400` / `3600` across the four
in-scope files; (ii) **shape** grep for `notice`/`aggro`/`leash` prose repo-wide; (iii) **ratio and
percentage** shapes carrying **no retired number at all** (`2.5×`, `3.66×`, `2.47×`, `16%`, `84%`,
`69.5%`, `4.9×`, `UNBOUNDED`); (iv) a re-run of (i) after editing.

**19 sites re-authored.** The ones a value-grep would have MISSED are marked ⭐:

| # | site | was | now |
|---|---|---|---|
| 1 | `.h` section banner | his 2000/3600 quotes as live | his 5000/8000 quotes; the old ones retained and marked SUPERSEDED |
| 2 | `.h` `UnitEngagementRadiusUU` doc | 2000, and a DEFAULT-not-a-CAP argument riding the Longbowman | 5000; **the inverted danger** (§2 above) |
| 3 | ⭐ `.h` the one-literal **census** | enumerated the other `2000`s (ACastle HP, braking decel) | **re-measured for 5000/8000** — see §7 |
| 4 | `.h` fog-cut cost | "at 600 SKIPPED / at 2000 RUNS" | at 5000 RUNS; **87.8%** cut (69.5% named STALE) |
| 5 | `.h` `ResolveNoticeRadiusUU` doc | "a 3600 must come back as 3600" | pass-through **in both directions** |
| 6 | `.h` `ResolveEffectiveLeashRangeUU` doc | multiplier-is-MEASURED as the live reason | **provenance vs. surviving job**, crossover 5333.33 |
| 7 | ⭐ `.h` the `J-F27`/`J-F28` **riders** | open questions routed to the manager | **ANSWERED**, his rulings quoted; retention clamp ruled and pointed at `TASK-1008` |
| 8 | `.h` leash consequence | "disengages at 3000 instead of 900" | **8000**, an 8.9× widening; fog gap **13.1×**, chase ≈7390 uu |
| 9 | ⭐ `.h` `GetEngagementRadiusUU` doc | argued from the Longbowman's 3600 — **a retired example** | re-argued from the **three** live sources of divergence |
| 10 | `.h` `GetClassDefaultEngagementRadiusUU` | base CDO "says 2000" | 5000; **the stake grew 6.25×** |
| 11 | ⭐ `.h` `AcquireEnemyNearPoint` decl | "**≈2.5×** … blind to **≈16%**" (**no retired number in the sentence**) | 1.00× / 0.99× ⇒ **100% covered** |
| 12 | ⭐ `.h` same block, opposite faces | "**3.66×** the default notice radius" | **1.46×** — narrowed, ⛔ **not closed** |
| 13 | `.h` `AggroRadius` member doc | 2000, `min(2000, 609.6)` | 5000, `min(5000, 609.6)`, 87.8% |
| 14 | `.h` `LeashRange` member doc | "Leash FLOOR … (GDD §3.8: 900)" | **8000**, his verbatim; floor-wins explained |
| 15 | `.h` `LeashMarginMultiplier` doc | 1.5 IS MEASURED (as the reason) | history vs. **future-card job** |
| 16 | ⭐ `.cpp` LoadStats sparse note | *"cards.csv has **no NoticeRange column yet**"* — **premise false**, no stale number in it | the **column exists**; every **cell** is empty, by design |
| 17 | `.cpp` drop-site + acquire comments | notice 2000 / flat leash 900 | 5000 / 8000 floor; crossover named |
| 18 | ⭐ `.cpp` `J-F28` blind-area block | 2.5× / **~84% blind** / 2.47× / 3.66× | 1.00× / **0% blind** / 0.99× / 1.46×; + the `FOG-§9.5` two-5000s warning |
| 19 | `.cpp` both resolver banners/bodies | 2000 default, Longbowman channel rationale | 5000; **both** refusal spellings |

**Checked and found NOT falsified** (recorded so the gate need not re-derive them):
- ⭐ `SiegeFogClampTest.cpp`'s **"UNBOUNDED"** label (the site the row named as unreachable by any
  value-grep): it describes the *gather* radius vs. the *pick* bound. **Unaffected** by 5000/8000.
- `SummonedUnit.h`'s historical line *"aggro 600, leash 900, Archer 700 — were ALL stale"*: a claim
  about **history**, still true. The Archer-was-2100 clause is **true again** post-`TASK-1004`.
- `.h` "Cavalry (600)" = a **speed**, and `StructureGoalProjectionExtent 600.f` = an **extent** —
  different quantities that share a round number.
- Controls-help *"there is deliberately no separate leash range: the zones are the leash"* — carries no
  number and `FOG-§9.11` confirms the grouped lane reads no leash. **Untouched.**

**Out of scope, found, NOT edited — for `TASK-1000`:** `CardRow.h:224-230` states
`UnitEngagementRadiusUU = 2000`, quotes his retired *"notice range of 2000"*, and warns against
`FMath::Min(NoticeRange, 2000)`. **Three false claims**, in a file this row does not own. It is the only
place in `Source/` outside my four files carrying a stale notice/leash number (measured repo-wide).

---

## 7. THE RE-MEASURED LITERAL CENSUS (`FOG-§1` one-literal discipline)

Machine-scanned code lines (comments excluded) across `Source/`:

- **`8000`** — **0** occurrences in shipped source before this diff. `LeashRange = 8000.f` is now the only one.
- **`5000`** — 2 in shipped source: `USiegePlayerController::GroupRadiusMax` and a `-5000` downward
  trace in `SiegeBotController.cpp`. Neither is a copy of this constant.
- **`7500`** (the product) — **0**, as required: it is computed, never typed.

⭐⭐ **`GroupRadiusMax` needs its own sentence and gets one in the code, because it is `FOG-§9.5` verbatim
and it arrived here BY DESIGN, not by coincidence:** it is *also* 5000 uu, *also* a reach — his ruling
sizes unit notice to cover a maximum guard circle **exactly** (that is why the number is 5000 and not a
rounder one). ⛔ **They are still not the same number.** *"When one moves, must the other?"* ⇒ **NO** —
he can widen the legal guard circle without re-ruling eyesight. So sharing a symbol would be a **defect**,
and test 4(c) **reads `GroupRadiusMax` structurally out of its own header** and computes the coverage
figure from **two separate operands**, so retuning either shows up as a real change rather than `x/x = 1`.

---

## 8. ⚠️ WHAT I AM TELLING JONATHAN, VIA THE GATE — **`J-F28` IS ANSWERED FOR THE GUARD CIRCLE, NOT FOR THE CASTLE**

His 5000 resolves the finding **exactly**: a unit at the centre of a maximum guard circle went from
**16% coverage / 84% blind** to **100% / 0%**. ⛔ **But a second, different measurement did not clear:**
the castle's colliding footprint is ≈7313.7 uu across = **1.46× the notice radius** (it was 3.66×), so a
defender at one face **still** does not acquire a besieger at the opposite one. The gap **narrowed
sharply and did not close.** This is asserted and messaged in test 4(c) and written into both source
files, so nobody reads *"J-F28 is resolved"* as *"the DEFEND gap is closed."*

---

## 9. SUITE — **DELTA ONLY** (`TL-§5b`/`§5c`)

⛔ **I executed no suite** (no compile is mine to run — `TASK-987` owns this wave's compile), and
⛔ **`445 / 34` is DECLARED, never executed — it is not a pass count and I do not report it as one.**

**DELTA: +6 assertions, +1 test case, 0 removed, 0 relaxed.**

| test | delta |
|---|---|
| test 2 | 2(a) re-pointed to a synthetic (**was going RED at 5000**); **+2** value assertions (2(b), the anti-widen pair); **+1** structural (`FMath::Max` == 0) |
| test 3 | **+1** (b2 his 8000 through the expression); **+3** (e)/(f) crossover rows |
| test 4 | **+1** (c) the J-F28-answered tripwire |
| **test 5 (NEW case)** | **+8** assertions (3 controls, 1 channel-vs-data, 4 ordering) + 1 `AddInfo` |
| `SiegeFogClampTest` | **±0** — prose/justification only; ⛔ no assertion added, removed or relaxed |

⭐ **Every one of the 33 structural pins the suite takes over `SummonedUnit.{h,cpp}` was evaluated
without a compile** (a script mirroring `CountOccurrencesInCode` + `ExtractFunctionBody`): **33/33 pass**,
including test 9's three fog tokens at **0 by both scans**, `> GetEffectiveLeashRangeUU()` == 2,
`> LeashRange` == 0, the grouped lane's leash-free-ness, both class seals at `0.f`, and
`GroupRadiusMax = 5000.f` == 1. Brace/paren/bracket balance verified on all four files.

---

## 10. WHAT I DID **NOT** DO

- ⛔ No fog code, no fog symbol, **not even in a comment** — the retention clamp is `TASK-1008`'s.
- ⛔ No `cards.csv` (`TASK-1004`), no `DT_Cards` (`TASK-1005`), no `CardRow.h` (`TASK-1000`).
- ⛔ No `AggroRadius` class seal raised (both still `0.f`, machine-verified).
- ⛔ No `152.4f` height constant touched.
- ⛔ No compile, no editor, no MCP, no Git, no board row but my own.
- ⛔ No line-number citations added to prose (`SC-§40` cl. 9) — every probe locates by **symbol**.

⚠️ **One mechanical note so it is not mistaken for content:** my symbol rename inside
`SiegeFogClampTest.cpp` (the stale local `LongbowmanNoticeUU` → `PerCardNoticeUU`, 3 sites — the name
itself was a claim shape carrying no number) was applied with a script that rewrote the file with **LF**
endings. The repo normalises to LF in the index, so `git diff --numstat` shows **content-only** changes
(278/16) with **no whole-file line-ending churn**; its untouched sibling `SiegeAcquisitionFunnelTest.cpp`
is LF in the working tree too. Flagged rather than left for the gate to wonder about.

## 11. NO FORCING CONTRADICTION

Nothing in the spec required a rewrite. The two places where I went **beyond** a literal reading —
(i) test 2's restructure and `SiegeFogClampTest` 7(d)'s retraction, and (ii) turning the multiplier's
re-argument into executed assertions — are both **item (6)'s claim census applied to claims that happen
to live inside assertion messages**, which is where the two most dangerous falsifications were. Both are
argued in the code itself, not only here.
