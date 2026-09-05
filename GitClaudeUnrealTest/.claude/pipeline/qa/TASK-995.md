# QA Report — TASK-995 · [LONGBOW-GATE]

**Verdict: PASS**
**BLOCKERS: 0** · WARN: 6 · NIT: 4
**Under gate:** `TASK-993` (the `NoticeRange` channel in `Docs/Data/cards.csv` + its `DT_Cards` half) · `TASK-997` (items **(5a)** + **(5c)** only — items (1)–(5) cancelled unspent)
**Cited, NOT re-derived:** `qa/TASK-1006.md` (both data halves already passed: CSV field-by-field, `DT_Cards` instrument calibrated by hand) · `qa/TASK-981.md` (the fog-curve diff in these two files is `TASK-981`'s, ruled ACCEPTED, and is **not a finding here**).
**Law:** `FOG-§9.11` · `FOG-§9.11a` · `FOG-§9.9` · `SC-§29` · `SC-§37` · `SC-§38` · `SC-§38a` · `SC-§39` · `SC-§45` · `SC-§60` · `SC-§62` · `SC-§63` · `SC-§65` · `TL-§5c`

---

## 0. ⛔ INSTRUMENT DISCLOSURE, ABOVE THE FINDINGS (`SC-§39`)

| instrument | used for | controlled how |
|---|---|---|
| `Grep` → `Read` (`SC-§38a`) | every character-exact claim below | every quoted line was `Read`, never judged from a `Grep` rendering |
| needle-narrowing census | the `609.6f` / `304.8f` code-literal law | `609.6f\|304.8f` returns **2** hits while `609.6\|304.8` returns **22** in the same file ⇒ the needle discriminates, it is not a dead instrument |
| CSV field-position read | every card value quoted | header field 8 = `Range`, field 32 = `NoticeRange`, field 1 empty (`FOG-§9.8e`); controls hold both ways — `Footman 120`, `BallistaTower 1400`, `Wizard.AoERadius 250` all match, and the row alternation returned exactly the 9 rows named and nothing else |
| **line-anchor differential** (new, see §2) | *"prose-only, one insertion"* | four independent anchors measured by `qa/TASK-981.md` **before** this diff, re-located today |

⛔ **NO shell this session.** `Bash` is **absent from my toolset**, not merely fenced. Consequences, stated rather than buried:

- ⛔ **Board item (3) explicitly instructs *"verify with a `git diff --stat` over `Source/`"*. I could not run it.** I substituted the §2 line-anchor differential plus a full cell-for-cell re-derivation. **Byte-identity is a Git claim; I record the absence rather than assert it** (`SC-§40`, same posture as `qa/TASK-981.md` §3).
- ⛔ No compile, no engine, no MCP, no hash, no Git. **Nothing in this batch has ever executed** (`SC-§37`).
- ⛔ **No `Edit` tool.** The board flip is returned as a `status:` line for the manager to proxy (§9), exactly as on `TASK-996` / `TASK-1006`.

⚠️ **`TASK-998` is running now on `FogVolume.{h,cpp}` / `SpellLibrary.cpp` / `CardRow.h` / `SiegeCombatStatics.cpp`.** I read none of those as settled and touched nothing on its row.

---

## 1. ⛔⛔ THE SUBJECT MOVED — AND I GRADED THE SURVIVOR, NOT THE CORPSE

`TASK-993` was authored before 🧑 his `5000` ruling. Three of its cells were **deliberately reversed** by later rows. Measured by me on disk at my own instant:

| cell | `TASK-993` wrote | **on disk today** | who reversed it |
|---|---|---|---|
| `Archer.Range` | `2000` | ✅ **`2100`** | `TASK-1004` (CSV) + `TASK-1005` (`DT_Cards`) |
| `Wizard.Range` | `2000` | ✅ **`2100`** | same |
| `Longbowman.NoticeRange` | `3600` | ✅ **BLANK** | `TASK-1004` |
| `Longbowman.Range` | untouched | ✅ **`3600`** | — untouched throughout |

⇒ ⛔ **`Archer.Range == 2100` is CORRECT ON DISK AND IS NOT A DEFECT.** Board items **(4)**, **(4a)(i)** and **(4b)** instruct this gate to *require* `2000` / `-69.5%`. **Obeying them literally would have filed a false BLOCKER against a correct tree** — see **W-3**.

### What `TASK-993` contributed **that survives**, and it is the whole of what I graded

1. ⭐ **The `NoticeRange` channel exists in the data.** `Docs/Data/cards.csv` header field **32** is `NoticeRange`; **32/32 cells blank** = the sparse sentinel. ✅
2. ⭐ **The 33-line floor is satisfied.** Header 32 fields; every data row 32 fields — I counted `Footman` (line 2) and `CrystalTower` (line 29) by hand against the header, and `qa/TASK-1006.md` derived all 32. ✅ `TASK-993` §2's *"the minimum correct diff is 33 lines, not one"* was **right**, and the literal one-cell reading would have turned `SiegeAssistantSelectionTest` red 31 times.
3. ⭐ **The firing side is a zero diff.** `Longbowman.Range` is `3600` in the CSV **and** in `DT_Cards` (`TASK-1005` read-back, adjudicated by `qa/TASK-1006.md` item 4 half B), and the `-83.1%` documentation of it was correctly left alone.
4. ⭐ **The CSV/asset discipline.** `sha256`-not-size (`SC-§68`), explicit one-asset save, read-back with a discriminating positive control. That method is what let `TASK-1005` revert cleanly — the discipline outlived the values.

### ⛔ Board item (1) — the BLOCKER-grade one — **PASSES, and more strongly than it was written**

The item asks whether a `min(…, 2000)` survives anywhere on the notice path. **It does not, and the ceiling it feared is gone from the code entirely:**

- `ASummonedUnit::UnitEngagementRadiusUU = 5000.f` — `SummonedUnit.h:895`. The class default is **2.5× the retired `2000`**.
- `ASummonedUnit::ResolveNoticeRadiusUU` (`SummonedUnit.cpp:4455-4469`) — seal first (`!(ClassDefaultRadiusUU > 0.f)` ⇒ return it, NaN-safe), then a **pass-through in both directions**: ⛔ zero `FMath::Min`, ⛔ zero `Clamp`.
- The read site takes **this instance's class**, never the base CDO — `GetClassDefaultEngagementRadiusUU()` at `SummonedUnit.cpp:4526-4538` reads `MyClass->GetDefaultObject<ASummonedUnit>()->AggroRadius`. ⇒ ⛔ **board item (5)'s `Building.cpp:302` defect is NOT repeated.**
- The binding is `AggroRadius = ResolveNoticeRadiusUU(GetClassDefaultEngagementRadiusUU(), Row->NoticeRange);` (`:1310`) — the row never overwrites the member directly.
- The **only** universal ceiling is the fog one, applied as a `min` at one chokepoint: `FSiegeFogStatics::EffectiveVisionRadius` (`SiegeFogStatics.cpp:86-95`) returns the caller's float **untouched** with fog off and degrades to *no fog* — never to *no vision* — on a broken tuning.

⇒ ⛔ **THE ONE LINE THE BOARD ASKS FOR:** **the Longbowman notices at `5000` uu with fog off — 2.5× the `2000` this item was written to protect, through a resolver that provably cannot clamp — while its firing `Range` is UNTOUCHED at `3600` in both data halves; under fog it still takes the shared `min` to `609.6`.**

⛔ **Board item (4)'s deferred clause (*"Longbowman NOTICE `== 3600` in `DT_Cards`"*) is now DEAD, not deferred.** `TASK-1001` was re-scoped to **verification only** (*"DO NOT WRITE `noticeRange`. ANYWHERE. FOR ANY ROW."*) because the sparse sentinel **is** the correct state. Nothing is owed to the ship host on this clause. ⇒ see **W-1**, the one artefact that still says otherwise.

---

## 2. ⛔⛔ `TASK-997` — **ZERO NUMERIC CELLS MOVED. VERIFIED DIRECTLY, BY THREE INDEPENDENT LINES.**

This is the row's whole safety property: its original item (2) would have written `-69.5%` beside a restored `2100` under a header promising *"nothing here is estimated."*

### (a) ⭐⭐ THE LINE-ANCHOR DIFFERENTIAL — the strongest evidence available without a shell

`qa/TASK-981.md` **W-2** recorded four coordinates in `SiegeFogStatics.h`, measured **after** `TASK-981`'s rewrite and **before** `TASK-997` touched the file. Re-located today by content:

| anchor | `qa/TASK-981.md` | **today** | Δ |
|---|---|---|---|
| *"2% is a LETHAL SHOT"* prose | `:67` | `:67` | **0** |
| the *"LETHAL"* prose | `:340` | `:361` | **+21** |
| **the Longbowman table row** | `:176` | **`:197`** | **+21** |
| the 50,000 uu arena line | `:187` | **`:208`** | **+21** |
| the *"LETHAL"* prose | `:471` | `:492` | **+21** |

The inserted prose block is `:174-:194` = **exactly 21 lines**. ⇒ ⛔ **every anchor above the insertion moved `0`; every anchor below it moved `+21`, including one on each side of the table.** A constant shift across anchors that *straddle* the table means **no line was added or removed inside it**. ⛔ One insertion, at one point, and nothing else changed a line count anywhere in that file.

Same test on the other file: `qa/TASK-981.md` recorded `Tests/SiegeFogTest.cpp` `:232`, `:654`, `:691-692`. **All three resolve to the same lines today** ⇒ the (5a) edit was **line-count-neutral**, consistent with *"one token."*

⚠️ Residual, declared: this method cannot see an **in-place, same-line-count** edit. That residual is closed by (b).

### (b) ⛔ THE TABLE, RE-DERIVED CELL FOR CELL AGAINST `Docs/Data/cards.csv` — board item (4b)

`:172` promises *"measured from `Docs/Data/cards.csv` — nothing here is estimated."* I re-derived every row rather than reading the handoff's claim:

| `SiegeFogStatics.h` | shipped `Range` | cards.csv | `1 − 609.6/R` | printed | |
|---|---|---|---|---|---|
| `:197` Longbowman | 3600 | **3600** (line 12) | 0.83067 | `-83.1%` | ✅ |
| `:198` Archer | 2100 | **2100** (line 3) | 0.70971 | `-71.0%` | ✅ |
| `:199` Wizard | 2100 | **2100** (line 30) | 0.70971 | `-71.0%` | ✅ |
| `:200` BallistaTower | 1400 | **1400**, `MinRange 300` | 0.56457 | `-56.5%` + annulus | ✅ |
| `:203` ArrowTower | 900 | **900** | 0.32267 | `-32.3%` | ✅ |
| `:204` BombTower / Crystal | 800 | **800 / 800** | 0.238 | `-23.8%` | ✅ |
| `:205` Cleric | 400 | **400** | — inside | ✅ UNAFFECTED | ✅ |
| `:206` every melee | 120 | **120** (Footman, Knight) | — inside | ✅ UNAFFECTED | ✅ |

⇒ ⛔ **The provenance promise HOLDS, cell for cell, on all eight rows.** The `609.6` ceiling column header is intact. **The `SC-§63` fence names its protected rows and every one of them is correct: the Longbowman row, every tower row, Cleric, melee.**

### (c) THE MARKER CENSUS

`TASK-997` appears **exactly once in all of `Source/`** — `SiegeFogStatics.h:174`, the banner it self-stamped. Not conclusive alone (an edit need not stamp), but it agrees with (a) and (b).

### ⛔ (5a) — VERIFIED BY CENSUS, **NOT** BY HUNTING FOR THE DELETION

⛔ **I did not look for a removed line, and a reviewer who does will wrongly call this unshipped** — the literal lived in `TASK-981`'s still-uncommitted block, so it never existed in `HEAD`.

- Located by **symbol**: `Tests/SiegeFogTest.cpp:956` reads `NegativeCeilingCurve.FogVisionCeilingUU = -DerivedCeilingUU;` ✅
- **Census:** `609.6f` / `304.8f` return **2 hits, both prose** (`:107`, `:129`) and **zero code lines**. The file's own law (`:30-38`) and its subject header (`SiegeFogStatics.h:167-168`) are satisfied again. ✅
- **In scope, not a new dependency:** `DerivedCeilingUU` is `SiegeFogTestFixture`'s `CeilingFeet * CentimetresPerFoot` (`:114`) and the same block already reads it at `:927`. ✅
- ⛔ **ZERO behaviour change — confirmed at source, not accepted on the author's word.** The guard is `if (!(Ceiling > 0.f)) { return 0.f; }` (`SiegeFogStatics.cpp:45-48`): **sign-only**. The author's declared 1-ULP magnitude caveat is therefore correct *and* inert here. ✅ Flagging it rather than shrugging was right — it **would** matter at a site comparing magnitudes.

### ⛔ (5c) — THE DISAMBIGUATION IS CORRECT, AND `J-F31` IS **NOT** REPEATED

`SiegeFogStatics.h:174-194`, prose only, between *"nothing here is estimated:"* and the table:

- ✅ **The table's percentages are labelled `FIRING-RANGE` cuts**, explicitly *"NOT A NOTICE CUT, NOT A CHASE CUT"*, and the ✅ UNAFFECTED rows are scoped *"in firing range only"* — which is the single most misreadable cell in the block now that melee notice is cut too.
- ✅ **`87.8%` is attributed to NOTICE**, with its derivation inline: *"fog costs EVERY unit 87.8% of its notice radius (`1 − 609.6/5000`)"*. ⛔ **I re-derived it: `1 − 609.6/5000 = 0.87808` ⇒ `87.8%`.** ✅
- ✅ **`5000` is true on disk**: `SummonedUnit.h:895` `static constexpr float UnitEngagementRadiusUU = 5000.f;`, and the banner correctly calls it *"a DEFAULT, never a cap"*.
- ✅ The derived **comparison** *"steeper than every firing cut in this table, the Longbowman's included"* introduces no number and is true: `87.8 > 83.1`.
- ✅ *"it was AMBIGUOUS, not wrong"* is stated **in the comment itself**, so a future reader does not read the banner as an erratum.

⇒ ⛔ **The exact conflation `J-F31` has produced three times in one day is closed at the point of reading, with both true numbers named and separated.**

### ⛔ SUITE-NEUTRALITY OF A PROSE EDIT — checked, not assumed

This project pins prose with executable censuses, so a comment-only edit is not automatically free. I verified the mechanism myself:

- `CountOccurrencesInCode` (`SiegeFogClampTest.cpp:110-150`) skips a trimmed line starting `//`, `* `, `*/`, `/*`, or equal to `*`. **Every inserted line is a `*` doc-comment continuation** ⇒ invisible to it.
- The **three** probes that load `SiegeFogStatics.h` by path are the only ones tree-wide: the `AsymmetryTokens` loop (`:824-853`, code-only, and none of its seven tokens appears in the new prose); `ClampMin = "304.8"` (`:970`, code-only, needle absent from the new prose); and the raw `FogHeader.Contains(TEXT("THE VISUAL'S CURVE ONLY"))` (`:1408`) — **that string is untouched and still present at `:351`.** ✅
- Every `CountAcrossShippingSource` needle is a **call shape** (`FSiegeVisionQuery::Seeing`, `FSiegeFogStatics::EffectiveVisionRadius(`, `ReadFogState(`, `FogDensityAt(`, `EffectiveVisionRadius(`). **None appears in the new prose.** ✅
- Nothing counts `UnitEngagementRadiusUU` tree-wide; the one census (`SiegeUnitNoticeRangeTest.cpp:841`) is scoped to `SummonedUnit.h` with a code-line needle. ✅

⇒ ⛔ **The declared `0/0/0` delta is `0` by mechanism, and I confirmed the mechanism rather than the claim.**

---

## 3. ⚖️⛔⛔ THE RULING YOU ASKED FOR — **THE PROHIBITION WAS SCOPED TO THE TABLE'S CELLS. THE AUTHOR READ IT CORRECTLY. NOT A FINDING.**

The dispatch said *"do NOT change, **add**, or recompute a single number"*; `TASK-997`'s `spec:` **(5c)** requires the header to state *"NOTICE is a SEPARATE gate at `5000` cutting `87.8%`"*. **The board row is the contract.** Ruled, four ways:

1. ⛔ **The absolute reading makes the row's only surviving new item unsatisfiable.** (5c) names both figures as required content. `SC-§62` cl. (i) is precisely this test — an instruction whose obedience makes a boarded spec item impossible does not bind.
2. ⛔ **The row scopes its own FAIL condition to cells, twice.** `spec:` (5c): *"DO NOT ALTER A SINGLE FIGURE IN THE TABLE. **A DIFF TO ANY NUMERIC CELL** ⇒ FAIL."* `names:`: *"THE `:175-185` TABLE **HEADER PROSE ONLY** … **ZERO NUMERIC CELLS**. `:177`/`:178` ARE OFF LIMITS."* Both are cell-scoped; neither forbids prose.
3. ⛔ **Nothing was recomputed, so even the strict reading is satisfied on its own terms.** Both figures were **quoted**: `5000` from `FOG-§9.11`'s table and the shipped constant; `87.8%` from `FOG-§9.9`'s corrected row, corroborated at `SummonedUnit.h:890` and `SummonedUnit.cpp:1803-1823`. ⛔ **I re-derived both independently anyway and both are true** (§2).
4. ⛔ **The strict reading produces the WORSE artefact.** A header saying *"the notice cut is a different number"* without saying **which** number sends the reader straight back into the lane that produced `J-F31` three times today. The instruction and the defect it was meant to prevent point in opposite directions.

⇒ ✅ **NO fix required. NO finding raised. And the author is graded UP, not down**: declaring the contradiction with both readings, the exact one-line fix, and the provenance of both figures is exactly what `SC-§62` asks for. ⛔ **The contradiction was introduced by the dispatch, not by the diff, and the record should say so.**

---

## 4. BOARD ITEMS — DISPOSITION

| item | verdict |
|---|---|
| **(1)** Longbowman notice can exceed `2000`; firing still `3600` | ✅ **PASS** — notice resolves to `5000`; no `Min`/`Clamp` on the path; `Range` untouched (§1) |
| **(2)** `SiegeFogTest.cpp:232` hand-types the Longbowman range | ⚠️ **WARN-4** — correct today; not failed, not fixed here, as instructed |
| **(3)** the `3600` sites NOT touched | ✅ **PASS by content + line anchors** — ⚠️ the named `git diff --stat` was unavailable (**W-5**) |
| **(4)/(4a)(i)** `Archer`/`Wizard` `Range == 2000` in both halves | ⛔ **INVERTED BY LATER ROWS — `2100` is correct.** Graded per dispatch; see **W-3**. Both halves already passed by `qa/TASK-1006.md` |
| **(4a)(ii)** their `NoticeRange` cells still BLANK | ✅ **PASS** — blank on both, and **32/32** blank roster-wide; zero hand-typed values |
| **(4)** deferred `Longbowman NOTICE == 3600` in `DT_Cards` | ⛔ **DEAD, not deferred** — `TASK-1001` re-scoped to *"DO NOT WRITE `noticeRange`"*; the sentinel **is** the correct state (§1) |
| **(4b)** header table agrees with `cards.csv` cell for cell | ✅ **PASS** — all 8 rows re-derived (§2b) |
| **(4c)** `SC-§60`, did the test NAME move with its number | ✅ **PASS** — `:694` *"lose 71.0%"* / `:695` `0.710f` against `ArcherRange 2100` ⇒ `0.70971`. `:691` *"83.1%"* / `:692` `0.831f` ⇒ `0.83067`. **Neither was renumbered; neither name lies.** `Read`, not `Grep`-judged (`SC-§38a`) |
| **(5)** read site reads the instance's class | ✅ **PASS** — `SummonedUnit.cpp:4526-4538` |
| **(6)** non-regression, every other unit bit-identical | ✅ **PASS** — 9 rows read by me (`Knight` 120 · `Miner` 0 · `ArrowTower` 900 · `Cleric` 400 · `BombTower` 800 · `BallistaTower` 1400 · `CrystalTower` 800 · `Sorcerer` 0 · `WatchTower` 0), all matching the table and the fixture; `sum(DeckCount) == 50` per `qa/TASK-1006.md` |
| **(7)** the castle-to-castle question | ⛔ closed before dispatch; not raised |
| **(8)** `SC-§39` instruments declared above findings | ✅ §0 |

**Suite: DECLARED, `0 / 0 / 0` (`TASK-997`) and `Δ 0 / 0` (`TASK-993`, zero `Source/` files).** ⛔ **Stated as a DELTA and nothing more — no row in this batch owns a compile, and nothing in it has ever been executed** (`SC-§37`, `TL-§5c`). My structural check (§2) agrees with the declaration: no assertion was added, removed, renamed or relaxed.

---

## Findings

### ⛔ BLOCKERS — **0**

*(Neither diff contains a defect. `TASK-997` shipped exactly two edits and moved not one numeric cell; `TASK-993`'s surviving contribution — the channel, the sentinel, the 33-line floor, the untouched firing side — is correct on disk.)*

### WARN

- **[WARN] W-1 — `handoffs/TASK-993-programmer.md` §9.6 — A NOW-FALSE IMPERATIVE ADDRESSED AT THE BUILD-MASTER.** It reads, in bold: *"REQUIRED, AND IT MUST BE BOARDED ON OR AFTER `TASK-987`: one post-compile `set_rows` — `{"Longbowman": {"noticeRange": 3600}}` — followed by a read-back and a save."* ⛔ **Executing that today would write a value 🧑 he retired**, make `DT_Cards` disagree with 32 blank CSV cells (`FOG-§9.11a`), and leave the Longbowman noticing **less** than every other unit (`3600 < 5000`) — inverting `J-F20`, the exact reasoning `TASK-1004` used to blank the cell. It is also the one class of defect this project has measured as invisible: the tests read the CSV or call the resolver, **neither reads `DT_Cards`**. ⇒ **WARN and not a BLOCKER for one reason only: the row that owns the action is already fenced** — `TASK-1001` item (1) reads *"DO NOT WRITE `noticeRange`. ANYWHERE. FOR ANY ROW."*, and the board beats a handoff. ⇒ **Fix: strike §9.6 in place with a dated superseding note pointing at `TASK-1004`/`1005`/`1001` + `FOG-§9.11`.** ⛔ **Do NOT re-author the document** — see W-2.
- **[WARN] W-2 — same handoff, §1 and §9.2 — four superseded cell claims. SUPERSEDE, DO NOT RE-AUTHOR.** Now false on disk: *"`MOVED Archer Range '2100' -> '2000'`"*, the same for `Wizard`, *"`NoticeRange AUTHORED : [('Longbowman', '3600')]`"*, and §9.2's `DT_Cards` after-column of `2000`/`2000`. All four were reversed **deliberately** by `TASK-1004`/`1005` and re-verified by `qa/TASK-1006.md`. ⇒ **A dated banner at the top is sufficient and re-authoring would be a net loss** (`TL-§5c` cl. 4): the document's *method* is what later rows have already built on — the `405`-and-`Properties not found in schema` measurement, the `sha256`-not-size discipline, the 33-line-floor derivation, the BP-CDO byte probe, and §6(D)'s fence collision that produced `TASK-1000`. **None of that is falsified by the revert.**
- **[WARN] W-3 — `TASKBOARD.md` `TASK-995` items (4), (4a)(i), (4b) — INSTRUCTIONS THAT WOULD HAVE PRODUCED A FALSE BLOCKER.** They order this reviewer to require `Archer`/`Wizard` `Range == 2000` and `SiegeFogStatics.h` `:177`/`:178` at `-69.5%`, and (4a)(i) calls `2100` *"a BLOCKER"*. ⛔ **`2100` is correct on disk on both halves.** A reviewer obeying the row literally fails a correct tree and sends `TASK-997` back to re-introduce the precise false measurement its own item (1) exists to prevent. Same defect class as `qa/TASK-1006.md` **W-2** against `TASK-1000` item (1) — **second instance in one day, same cause: `J-F29` text left standing after `J-F29` was reversed.** ⇒ **Fix (manager, board-side, zero code): strike or invert those three clauses on `TASK-995` before the row is ever cited again.**
- **[WARN] W-4 — `Tests/SiegeFogTest.cpp:231-238` — the hand-typed roster block** (board item (2), inherited from the cancelled `TASK-994`, **widened**). `:232` `LongbowmanRange = 3600.f` is the line the board names, but the same decoupling covers **all seven** constants (`3600 / 2100 / 1400 / 900 / 800 / 400 / 120`), and they sit under `:231`'s provenance promise *"read from `Docs/Data/cards.csv`"*. ✅ **All seven are correct today — I checked every one against the CSV.** ⛔ They would **stay green through any future change to the data**, which is exactly the shape `TASK-1003` had to repair elsewhere in this batch. ⇒ **Not failed here and NOT to be fixed here, per the row.** Board it against the next row that owns this file (the pattern to copy is `SiegeUnitNoticeRangeTest` test 5, which scans the CSV instead of quoting it).
- **[WARN] W-5 — THE GIT CLAIM IS UNMADE, NOT MADE.** Board item (3) names `git diff --stat` as the instrument; I have no shell (§0). I substituted the line-anchor differential and a cell-for-cell re-derivation, which are **stronger on correctness** and **weaker on "was it touched"**. ⛔ **I do not assert byte-identity of the fog-cut table; I assert that every cell in it is arithmetically correct against today's CSV and that no line was inserted or removed inside it.** ⇒ **Fix: `TASK-987` reads the staged diff for these two files and confirms `SiegeFogStatics.h` shows one 21-line comment insertion at `:174` and `Tests/SiegeFogTest.cpp` shows one changed token.**
- **[WARN] W-6 — NOTHING HAS EXECUTED.** Both suite deltas are **DECLARED**. `TASK-987` owns the first executed run for this entire batch; `qa/TASK-1006.md` W-3 already binds it to reconcile against `+16 / +1`, not the declared `+6`. Nothing in `TASK-993`/`TASK-997` adds to that reconciliation (`0 / 0`).

### NIT

- **[NIT] N-1 — coordinate rot in this gate's own row.** `TASK-995` cites `SiegeFogStatics.h:176`/`:177`/`:178` and `SiegeFogTest.cpp:654`/`:690-692`/`:694`. Live: **`:197`/`:198`/`:199`** (all +21 after the (5c) insertion) and `:654`/**`:691-692`**/`:694`. The **symbols** are correct (`SC-§38`); the numbers were already rotting when the row was written.
- **[NIT] N-2 — `SiegeFogStatics.h:185` says *"fog costs EVERY unit 87.8%"*.** Exactly true **while the column is fully sparse** (32/32 today). A future card with a populated `NoticeRange` gets a different cut (`6000` ⇒ `89.8%`). The banner two lines above already says *"the SAME value for every shipped card (a DEFAULT, never a cap)"*, so the scope is stated — but *"every shipped card"* in that sentence too would close it against the exact re-reading this lane keeps having to retract. One word.
- **[NIT] N-3 — `Tests/SiegeFogTest.cpp:129` asserts `20.f * 30.48f` and `609.6f` are *"different bit patterns."*** ⛔ **Unverified — I cannot execute, and a hand-derivation in binary32 suggests they may round to the same value.** ⛔ **Pre-existing, NOT this row's, and it has zero bearing on (5a)**, whose guard is sign-based. Recorded so nobody later cites it as reviewer-confirmed; the tolerance it justifies (`1e-4`) is safe either way. ⛔ **Do not "fix" it without executing.**
- **[NIT] N-4 — the fog-cut table lists only firing cards.** `Miner`, `Sorcerer` and `WatchTower` carry `Range 0` and are absent. Correct (no firing range to cut) and pre-existing, but the block never says the roster is filtered — worth half a line the next time the header is touched, now that it advertises which quantity it measures.

---

## Notes for build-master (`TASK-987`)

1. ✅ **Both rows under this gate are clear to ship. Zero blockers. Nothing here blocks the commit.**
2. ⛔⛔ **DO NOT WRITE `noticeRange` INTO `DT_Cards` — ignore `handoffs/TASK-993-programmer.md` §9.6 (W-1).** It is boldfaced, specific, addressed at you, and **wrong**. The correct end state is the **default state**: `noticeRange = 0.0` on all 32 rows, agreeing with 32 blank CSV cells. `TASK-1001` item (1) is the binding instruction. ⛔ *A cancelled sweep is more dangerous than an open one — its text is specific, confident and wrong.*
3. ⛔ **`Content/Data/DT_Cards.uasset` is correct on disk but UNCOMMITTED** (`qa/TASK-1006.md` note 6). **Stage it or the revert never ships.** Verify by **oid-vs-`sha256`, never by size** — this asset has now lied by size in **both** directions.
4. **Expect exactly two `Source/` changes from `TASK-997`:** a 21-line comment insertion at `SiegeFogStatics.h:174` (the table below it byte-identical) and one token at `Tests/SiegeFogTest.cpp:956`. **Confirm both in the staged diff (W-5)** — the second renders as a **lone `+` with no matching `-`**, because the literal lived in `TASK-981`'s uncommitted block. ⛔ **A missing deletion is NOT a missing edit.**
5. **`TASK-993` contributes zero `Source/` files** — one new CSV column, 32 blank cells, uniform 32-field rows. If the compile reports a short-row failure in `SiegeAssistantSelectionTest`, it is **not** this row.
6. **Suite: declared `0 / 0` from both rows.** The batch reconciliation is `qa/TASK-1006.md`'s `+16 / +1`, unchanged by this gate.
7. ⛔ **Still blocking your commit, from the other gate, not this one:** `TASK-1000` must land first (`CardRow.h`'s `NoticeRange` doc block — `qa/TASK-1006.md` W-1), and `CardRow.h:234`/`:252` still name `TASK-993`'s retired cell. **Order: `998 → 1000 → 987`.**

---

## Board update — ⛔ I HAVE NO `Edit` TOOL. PROXY THIS LINE.

> `- status:` ✅✅ **qa-passed 2026-09-04 — ⛔ 0 BLOCKERS · ⛔ 6 WARN · ⛔ 4 NIT.** Report ⛔ **`.claude/pipeline/qa/TASK-995.md`**. ⛔ **`TASK-997` moved ⛔ ZERO numeric cells — verified three ways (line-anchor differential: one 21-line insertion at `:174`, `0`/`+21` across four `qa/TASK-981.md` anchors straddling the table · all 8 table rows re-derived cell-for-cell against `cards.csv` · `TASK-997` marker = 1 site tree-wide). ⛔ `87.8%` (`1 − 609.6/5000`) is ⛔ CORRECTLY attributed to NOTICE and the table is ⛔ CORRECTLY labelled FIRING ⇒ ⛔ `J-F31` NOT repeated.** ⭐ **(5a) verified by CENSUS (`609.6f`/`304.8f` = 2 prose hits, 0 code lines), ⛔ never by hunting the deletion.** ⛔ **THE LONGBOWMAN NOTICES AT `5000` uu FOG-OFF (2.5× the retired `2000`, resolver provably unclamped) AND ITS `Range` IS ⛔ UNTOUCHED AT `3600` ON BOTH HALVES.** ⚖️ **PROHIBITION-SCOPE RULED: cell-scoped, ⛔ the author read it correctly, ⛔ NOT a finding — the contradiction was the dispatch's, ⛔ not the diff's.** ⚠️ **W-1 ⇒ `handoffs/TASK-993-programmer.md` §9.6's *"REQUIRED post-compile `set_rows noticeRange 3600`"* is ⛔ NOW FALSE and is ⛔ addressed at the build-master — ⛔ already fenced by `TASK-1001`(1); ⛔ SUPERSEDE the handoff, ⛔ do NOT re-author it.** ⚠️ **W-3 ⇒ this row's OWN items (4)/(4a)(i)/(4b) demand `2000`/`-69.5%` and would have produced a ⛔ FALSE BLOCKER — ⛔ manager to strike.** ⛔ **Suite DECLARED `0/0`; ⛔ nothing in this batch has executed.** ⇒ **ready-for-integration on `TASK-987`.**

---

**Verdict: PASS · 0 blockers · 6 WARN · 4 NIT. ⛔ Nothing in this gate blocks the commit.**
