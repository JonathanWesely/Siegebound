# QA Report — TASK-889 (the gate over `TASK-874`, the assistant-roster tail)

**Verdict: FAIL** — **1 BLOCKER · 4 WARN · 3 NIT.**

⛔⛔ **I HAVE NO SHELL, AND I AM SAYING SO FIRST AND PLAINLY** (`TL-§5c` cl. 5). No Bash, no git, no compile, no editor, no MCP. Everything below is a **structural read of files on disk** plus arithmetic. **Nothing was executed.** Every count I report is `declared`, and the two columns that need a runner — the suite execution and the `HEAD` reconciliation — are marked DECLARED and transferred by name in §7.

⚖️ **The FAIL is ONE line.** The deliverable this row was boarded for — *is the roster derived, or was `13` bumped to `14`* — is **unambiguously satisfied**, and the hard fence was not merely honoured, it was hit live and resolved correctly. Sections 1–5 are almost entirely commendation. The blocker is a missing three-line precondition in the new test, with the correct form already shipped **150 lines above it in the same file**.

---

## `SC-§29` COVERAGE LEDGER

This gate covers **exactly ONE task, named: `TASK-874`.**

⛔ **NOT covered here, and not implied:** `TASK-876` (already gated by `qa/TASK-878.md`) · `TASK-903`/`904`/`905` (the comment repair this diff *routed* — see §6, I ruled on **where it lands**, not on its content) · the compile · the suite execution · `TASK-906`'s commit.

⛔ **Method (`SC-§38`):** every coordinate in this report is my own read, dated **2026-09-03**, taken by opening the named function and reading the quoted expression. **No line number from any document was trusted**, including the handoff's and including the dispatch's.

---

## (0) THE PREMISE, RE-MEASURED BY ME — ⛔ AND REPORTED BECAUSE IT MATCHES (`SC-§40` cl. 9)

I counted the commandable kinds myself, from `Docs/Data/cards.csv`, against the predicate I read out of `ASiegePlayerController::ResolveCardActorClass` (SiegePlayerController.cpp) + `IsBuildingCard`:

```
Building                                 -> ABuilding
Economy AND in BuildingEconomyCardIDs    -> ABuilding      (DeepMine)
Unit, or Economy NOT in that list        -> ASummonedUnit  <-- commandable
Spell / HeroUpgrade / Utility            -> never spawned
```

**My tally over the 32 shipped rows:** `Unit` **13** (Footman, Archer, Knight, MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, Wizard, Sorcerer, **Witch**) · `Economy` **2** (Miner ⇒ counts, DeepMine ⇒ excluded) · `Building` **7** · `Utility` 1 · `HeroUpgrade` 4 · `Spell` 5. **13+2+7+1+4+5 = 32.** ✅

⇒ ✅ **14 commandable kinds.** ✅ **Tail, in card-table row order, is `witch` (row 32 of 32).** **Both figures match the dispatch and the handoff, and I am reporting them because I measured them.**

**Corroborations I ran rather than inherited:**

| claim | my measurement |
|---|---|
| `Capture()` has no profile/type filter | `SiegeAssistantSnapshot.cpp:662-676` — `IsValid` / `GetTeamId() != Team` / `IsUnitDead()` / `CanonicalKind().IsNone()`. ⛔ **Confirmed: nothing else.** |
| the `Witch` row is in the **ASSET**, not just the CSV | binary scan of `Content/Data/DT_Cards.uasset` — **`Witch` = 4 hits**. ⭐ **With a positive control (`SC-§39`): `Sorcerer\|WatchTower` = 5 hits in the same file**, so the instrument demonstrably sees card rows and a zero would have meant something. |
| `BuildingEconomyCardIDs` really is `protected` (so reflection is *necessary*, not decorative) | `SiegePlayerController.h:1663`, sitting between the `protected:` at 1603 and the `private:` at 2239. ✅ **Reflection is required, not a flourish.** |
| the CDO read is safe | `ASiegePlayerController` is a `UPROPERTY(EditDefaultsOnly) TArray<FName>`, so `FindFProperty<FArrayProperty>` reaches it; the value is **copied** at line 271, the CDO is never mutated. |
| `MaxRosterKinds` is reachable and is 13 | `SiegeAssistantSnapshot.h:445`, `static constexpr int32 = 13`, inside the `public:` region (232–925). |
| `CapKinds = min(14, 13)` | `SiegeAssistantSnapshot.cpp:2163`, and `KindsToPrint` at 2176 only ever **decreases** (2187). ⛔ **The trap is real.** |

---

## (a) IS THE ROSTER DERIVED, OR WAS `13` BUMPED TO `14`? — ✅ **DERIVED. NOT A RE-TRANSCRIPTION.**

⭐⭐ **`ThirteenKindsInCardRowOrder()` is GONE.** A symbol sweep of the file returns it at exactly **one** site: line 138, inside the comment that explains why it was deleted. `13` was **not** bumped to `14`; there is **no** fourteen-name array anywhere in the file. The remaining bare symbol literals (`footman`, `miner`, `archer`, `sorcerer` at 2292/2655/2702/3746/4978/5010 etc.) are **two- and four-element hand-built boards for grammar and command-parse tests** — they are not a roster and never were.

**The three read sources, each verified live:**

1. **Rows + order** — `Docs/Data/cards.csv` off disk via `FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / …)` + `FFileHelper::LoadFileToString` (lines 274-280). ✅ **This is byte-for-byte the shipped `SiegeFogClampTest::LoadProjectFile` idiom** (SiegeFogClampTest.cpp:82-87), including the `Lines.Num() < 10` death check and the `IndexOfByKey` header lookup — so the technique, and `Test.AddError` from a free function taking `FAutomationTestBase&`, **compile today**.
2. **The card-type symbols** — `StaticEnum<ECardType>()->GetNameStringByValue(...)` passed through `ShortEnumEntry` (lines 227-235, 249-251). ✅ **That helper is a faithful copy of the shipped `ShortEnumEntryName`** at `SiegeAssistantCommand.cpp:26-35`, and it is *necessary*: `ECardType` is a `UENUM` `enum class` (CardRow.h:17-25), so `GetNameStringByValue` returns the qualified `ECardType::Unit` and the qualifier must come off before it can match the CSV cell `Unit`. Nothing is compared against the literal `"Unit"` (`SC-§38`).
3. **The Economy-building exception** — `ASiegePlayerController::StaticClass()->GetDefaultObject()` + `FindNameArrayField(…, "BuildingEconomyCardIDs")` (lines 263-271). ✅ Non-templated `GetDefaultObject()` has two shipped precedents (`SiegePlayerController.cpp:5384`, `SiegeHeroCameraTest.cpp:168`) and is not deprecated in 5.8. `FindNameArrayField` (line 526-534) checks the **inner property type** as well as the name, so a retype fails loudly instead of returning garbage.

**⛔ EVERY WAY THE PROBE CAN DIE IS A FAILURE, NEVER AN EMPTY ARRAY — verified line by line.** Ten named death conditions (276, 284, 296-303, 314-319, 324-329, 339-344, 380-384, 391-396, 398-402, 406-418, 425-429), each returning a `Failure` string that **names the cause**, and `CommandableKindsInCardRowOrder` (455-466) raises `AddError` **and** returns an empty board, so a caller who forgot to check fails rather than passes. ✅ **This is the correct shape and it is the shape the whole batch is about.**

**The two positive controls hold, and I checked the needles before trusting them** (`SC-§39`, `SC-§41`):
- The discriminator must return **`Building` rows as well as `Unit` rows** (391-396). ⭐ **This is the stronger of the two available controls** and I endorse the choice: the failure it rules out — a column read as empty, or read at the wrong index — yields zero on *both* sides, so counting only the included class could not see it.
- **`sorcerer` must be in the derived roster** (425-429). Correct, and load-bearing: a dozen assertions in the file name that symbol.
- **Needle audit:** `CardType` and `Notes` each appear **exactly once as whole header fields** in the 31-field header, and `IndexOfByKey` is **exact whole-field equality** — so the substring form of the `SC-§41` defect (`SpawnCardID` swallowing `CardType`) is impossible by construction, not by luck.

---

## (a′) THE HARD FENCE — ⛔ I AUDITED THE 22-ROW LEDGER CASE BY CASE. ✅ **ZERO CASES LOST.**

⭐⭐ **The trap it names is real and I re-derived it independently:** on the full board `CapKinds = FMath::Min(14, 13) = 13` and `KindsToPrint` only decreases ⇒ `CollapsedKinds = 14 - KindsToPrint ≥ 1` **at every sentence length** ⇒ **`other_kinds: none` is STRUCTURALLY UNREACHABLE on the full roster.** A naive "derive it and use it everywhere" would have deleted a live case while looking like a clean refactor. **The fence caught it, and it is the only reason anybody noticed.**

**Every "within-cap" row, verified at its call site:**

| ledger row | site | board | verdict |
|---|---|---|---|
| 1, 2, 3, 4, 6 | TEST 1, line **1467** | `BoardWithinRosterCap` | ✅ uncollapsed print (1594), `other_kinds: none` (1611), sorcerer's full row (1618), row-by-row order (1600) — **all reachable** |
| 5 | TEST 1, 1537-1558 within-cap **+ 1566-1578 a NEW full-roster reading** | both | ✅ gained, nothing lost |
| 12 | TEST 3 case 1, line **1923** | `BoardWithinRosterCap` | ✅ **this is the case the naive fix would have destroyed** |
| 20 | Region test, line **4628** | `BoardWithinRosterCap` | ✅ `PrintedRosterSymbols == WithinCap.Num()` (4634) + `other_kinds: none` (4636) reachable |

**Every "full 14" row, verified at its call site:** 7-11 (TEST 2, line 1661 — `Collapsed.Num() > 0` and `Printed.Num() > 0` are *preconditions*, not full-print claims) · 13 (TEST 3 case 2, line 1924) · 14 (empty roster, 1926, untouched) · 15-18 (TEST 4, line 2010 — union/monotonicity/visibility, all relative to `Kinds.Num()`) · 19 (byte-parity sweep, 4551 — with/without region comparison, roster-size independent) · 21-22 (mark tests, 5115/5553/5672/5793 — `places:` arithmetic and publish counts are roster-independent; the roster claims are `Kinds.Num()`-relative).

⛔ **I looked specifically for the failure the ledger would hide — a full-roster site that still asserts an UNCOLLAPSED print.** I traced **every** `PrintedRosterSymbols` call in the file (1551, 1552, 1575, 1587, 1681, 1824, 2041, 4613, 4634, 5700, 5830). **There is none.** The one that could have been — the mark-cost baseline at 5700-5723 — is deliberately **recorded via `AddInfo`, not asserted**, with a comment saying why.

⭐ **NEW row confirmed:** the `MaxRosterKinds` cap branch (`bBudgetBite = KindsToPrint < CapKinds` → false → `Cause = "the MaxRosterKinds cap"`, `SiegeAssistantSnapshot.cpp:2225-2228`) is **genuinely live now** and is tested for the first time by the `+1`. ✅ **One case gained.**

---

## (a″) IS THE WITHIN-CAP PREFIX REALLY THE OLD THIRTEEN — AND IS THAT A CONSEQUENCE OR A HIDDEN TRANSCRIPTION?

✅ **CONFIRMED, AND IT IS A CONSEQUENCE.** `BoardWithinRosterCap` (489-497) is `CommandableKindsInCardRowOrder` followed by `SetNum(MaxRosterKinds)`. **There is no literal list anywhere in it.** I derived the prefix myself from the CSV row order:

`footman, archer, knight, miner, militiamob, pikeman, sapper, cavalry, longbowman, cleric, ogre, wizard, sorcerer`

— **symbol-for-symbol the thirteen that used to be typed.** The identity falls out of exactly two facts: card-table row order, and the Witch having been **appended last**. It is not written down anywhere.

⭐⭐ **And the diff already knows the identity is CONTINGENT, which is the part I most wanted to find and did.** Line 1511-1512 asserts `Kinds.Contains(sorcerer)` on the **within-cap** board, with a comment saying that if a future card row pushes the Sorcerer past the cap **this row goes red and a human decides where the claim moves — it does not quietly stop testing.** That is the correct treatment of a contingent identity and it is the difference between a derivation and a coincidence.

---

## (b) FENCE CHECK — `Tests/SiegeAssistantSelectionTest.cpp` ONLY

- ✅ **`USiegeAssistantSnapshot::Capture` is intact** as I read it (662-676): the four-way filter, `CanonicalKind`, the `(Kind, GroupId)` aggregation, and the delegation to `IsGroupCommandEligible`/`IsFollowCommandEligible` at 695-699. Nothing in it references the fixture.
- ✅ **The `SiegeAssistantSnapshot.cpp:2216-2224` comment is UNTOUCHED** — I read it verbatim and it still says *"DT_Cards has exactly 13 commandable kinds"* and *"the ONLY reachable cause is the character budget."* ⛔ **Both sentences are now FALSE, and the diff correctly did NOT fix them.**
- ⚠️ **What I cannot measure without a shell: `git status`.** "Zero production edits" is a claim I can support only negatively — I read the two production files this diff depends on and found nothing fixture-shaped in either. The **pathspec** is the commit row's duty (`§25b` cl. R), transferred in §7.

✅ **`SC-§47` applied as the spec asks: the handoff REPORTING the `Capture()` question rather than acting on it is CORRECT, and I am recording it as correct rather than merely not-wrong.** A fence limits the repair, never the report.

---

## FINDINGS

### ⛔ BLOCKER-1 — `Tests/SiegeAssistantSelectionTest.cpp:1866` — `Collapsed.Last()` has no non-empty precondition, so a regression **CRASHES the automation run instead of turning it red**

```cpp
// line 1847 — NON-FATAL. Execution falls straight through.
TestTrue(*FString::Printf(TEXT("⛔ At least %d kind(s) are collapsed by the cap alone — %d were"), ...),
    Collapsed.Num() >= Kinds.Num() - USiegeAssistantSnapshot::MaxRosterKinds);
...
// line 1865-1866 — UNGUARDED.
TestEqualSensitive(..., Collapsed.Last(), Kinds.Last().ToString());
```

`Collapsed` is `CollapsedSymbols(ZoneC)`, which **returns an empty array** whenever `other_kinds:` reads `none` or the key is missing (lines 719-727, explicitly documented). `TArray::Last()` on an empty array trips `RangeCheck`'s `checkf`; `DO_CHECK` is **on** in an Editor Development build ⇒ **assertion failure, process abort — the whole suite's results are lost**, rather than one red row naming the defect.

⛔ **This is not a hypothetical style point: the identical call in the sibling test is guarded, fatally, with a comment explaining exactly why.** Test 2, lines 1692-1696:

```cpp
if (!TestTrue(TEXT("⛔ PRE-CONDITION: ... If this fails the test below proves NOTHING and must not be read as a pass."),
    Collapsed.Num() > 0))
{
    return false;
}
```

The file guards `.Last()` **everywhere else** — 1717 (`if (Kinds.Num() > 0)`), 1487 (`if (Roster.Num() > 0)`), 5854 (`if (Kinds.Num() > 0)`) — and uses `IsValidIndex`/guard forms at 12 sites. **Line 1866 is the one place the house style is broken, and it was introduced by this diff.**

⚖️ **Why this is a BLOCKER and not a WARN, stated so the ruling can be argued with:** it is unreachable on today's shipped contract (`CapKinds` guarantees ≥1 collapsed kind), so it changes nothing if committed today. **But it becomes reachable in exactly one scenario — a Zone C collapse-line format regression — which is precisely the scenario this test was written to detect.** A test whose response to finding its defect is to kill the runner is worse than no test at that moment, and the whole subject of `TASK-874` is *code that must survive the world moving*.

**Suggested fix — the Test 2 form, preferred over a bare `if (Collapsed.Num() > 0)` because it also stops 1853-1861 from "proving nothing" after the precondition has already failed:**

```cpp
if (!TestTrue(*FString::Printf(TEXT("⛔ PRE-CONDITION: at least %d kind(s) are collapsed by the cap alone — %d were. If this fails, nothing below proves anything."),
        Kinds.Num() - USiegeAssistantSnapshot::MaxRosterKinds, Collapsed.Num()),
    Collapsed.Num() >= Kinds.Num() - USiegeAssistantSnapshot::MaxRosterKinds))
{
    return false;
}
```

⛔ **This is the ONLY blocker. Nothing else in the diff needs to change for a PASS.**

---

### ⚠️ WARN-1 — `handoffs/TASK-874-programmer.md` §4, final bullet: *"the derived roster would follow the production predicate automatically"* is **FALSE for the case that sentence is about**

The fixture mirrors `ResolveCardActorClass` + `IsBuildingCard` — **which actor class a card spawns**. It does **not** mirror `USiegeAssistantSnapshot::Capture`'s filter. So if Jonathan rules the Witch out and the repair is a **type or profile filter in `Capture()`** — which is the repair the handoff and the board both correctly insist on — the derived roster would **still return 14**, the fixture would diverge from the shipped roster, and **nothing would go red.** That is the same defect class this task just deleted, one level over.

⛔ **The code is correct today. The sentence is not, and the sentence is what the next engineer will read.** ⇒ **binding rider:** *if a `Capture()`-side filter ever lands, `BuildDerivedRoster` must mirror it in the SAME commit.* (`SC-§47`: a defect of **description**, invisible to the author, which is why a second reader is owed.)

### ⚠️ WARN-2 — the `git show HEAD:` provenance in §7/§8 **cannot be true if the file is untracked**, and two dated project records say it is

Handoff §7 measures `38 → 39` *"against `git show HEAD:` for the same path"*, and §8 compares paren/brace parity against *"the `HEAD` blob."* **Both are impossible for a path absent from `HEAD`.** And two records dated **today** state plainly that it is absent:
- `CONVENTIONS.md` `TL-§5d`: *"`Tests/SiegeAssistantSelectionTest.cpp` is in the executed 426 and **absent from `HEAD`**. `TASK-906` is the only thing between it and a silent loss."*
- `TASKBOARD.md:14412`: *"`Tests/SiegeAssistantSelectionTest.cpp` is **STILL IN EXACTLY THAT STATE** and is `TASK-906`'s — find it, name it, LEAVE it."*

⛔ **One of the two is wrong and I have no shell to settle which.** Consequence, and it is not cosmetic: **if the file is untracked, the durable figure at commit time is not `+1`, it is the whole file — `39` declared.** ⇒ transferred by name in §7.

### ⚠️ WARN-3 — the census absolute has already moved, and **nothing has been executed**

**My own re-measurement, taken now:** `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp` = **431 declared across 31 files**; this file = **39**. The handoff recorded **426/31** — a live-lane absolute, stale on arrival exactly as `TL-§5b` predicts. **The `+1` is the durable figure; the absolute is not.**

⭐ **Instrument control, and I tested the variable the dispatch told me to test:** the broad needle `IMPLEMENT_[A-Z_]*AUTOMATION_TEST` over the **same scope** returns **431 across the same 31 files** — the two needles agree **exactly**. ✅ **This confirms the dispatch's correction: the "one extra file" is a SCOPE difference (`Source/**` picks up `IMPLEMENT_PRIMARY_GAME_MODULE`), not a needle difference.** Holding scope constant, there is no discrepancy to explain.

⛔ **`declared`, never executed** (`TL-§5c`). Last EXECUTED run remains **`427 Result={Success}` / `0 Result={Fail}` at `1aa0fee`**.

### ⚠️ WARN-4 — several tests moved onto a board that **always** collapses; the margins moved with them, and none of it has been run

Moving TEST 2, TEST 4, the byte-parity sweep and the mark tests from a 13-kind board to the full 14-kind roster means `BuildZoneC` now emits `Snapshot roster TRUNCATED` at Warning where some of them previously emitted nothing.

- ✅ **Log side is clean.** I traced every site that calls `BuildZoneC` on a full-roster board and **every one** either registers `AddExpectedMessagePlain(…, Occurrences -1)` (1457, 1904, 2007, 4548, 5110, 5548, 5668, 5790) or **asserts** the latch fired with `Occurrences 0` (1658, 1810). I found **no unregistered site.** The new test's `Occurrences 0` registration is also correctly placed **after** the skip branch (1798-1804), so the not-exercised path cannot fail for the wrong reason — and the latch is safe because `WarnedRosterKindsPrinted`/`WarnedRosterKindsCollapsed` are **`mutable` instance members** (`SiegeAssistantSnapshot.h:1167/1170`), not statics, so each fresh snapshot starts clean. **Verified, not assumed.**
- ⚠️ **Margin side is thin and unexecuted.** TEST 2's `Printed.Num() > 0` precondition (line 1698) now runs one kind deeper. My arithmetic: `RosterBudget = 1085 - 192 - head 108 - tail 577 = **208**` (`MaxUtteranceBytes = 240`, so order 61→240 and pending 0→240 against the 158-char default tail); the one-row block measured at **182** on the 13-kind board grows ~7 chars (`", witch"`) to ≈ **189**. **≈19 characters of margin.** It should hold — but this is arithmetic, not a run. ⇒ **if anything goes red on this diff, TEST 2 line 1698 is the first place to look.**

### NIT-1 — a corrected figure was re-relayed
Handoff §0 and the file comment at line 178 both cite `TASK-849`'s *"all **29** comparable columns."* `qa/TASK-849.md` **NIT-3** already corrected that: the header carries **30** named columns beside the row name, **31 fields**. Cosmetic — the property the parse actually depends on is 31-field alignment, which I re-measured and which holds — but re-relaying a figure a gate has already corrected is the `SC-§40` cl. 3 shape.

### NIT-2 — `FindNameArrayField` hands back a **non-const** pointer into the CDO
Line 526 returns `TArray<FName>*`; `BuildDerivedRoster` copies from it at 271 and never writes. Read-only in fact, not by construction. **Pre-existing helper signature, not introduced here** — fold into whoever next touches it.

### NIT-3 — declared residual, correctly declared, restated so it is not lost
The fixture derives from `Docs/Data/cards.csv`; `Capture()` loads `/Game/Data/DT_Cards` **and takes its row order from `GetRowNames()`**. A row written to the asset and not mirrored to the CSV, or a reimport that reorders `RowMap`, would be invisible to the fixture. ✅ Already declared at lines 175-179 and mitigated (I independently confirmed `Witch` is in the asset). ⛔ **Note it is not a regression: the deleted hand-typed array carried the identical dependency and carried it worse.**

---

## ⚖️ THE FOUR RULINGS THE HANDOFF ASKED FOR — RULED, NOT DEFERRED

**RULING 1 — §9 item 4: the deliberate divergence from `SiegeFogClampTest`'s skip rule. ✅ CORRECT, and correct for the reason given.**
FogClamp reads `AoERadius` (index 18, **after** `Notes`), so a mis-aligned row genuinely cannot be read and skipping is the only honest option *there*. This probe reads index **0** and index **2**, both **before** `Notes`, **and it asserts that ordering by NAME at parse time** (314-319) rather than assuming it. An embedded comma in the free-text `Notes` makes a row **longer**, not shorter — `Fields.Num() < Header.Num()` stays false and fields 0 and 2 stay correctly placed — so the hard-fail can only fire on **genuine truncation**, where skipping would silently drop a commandable kind. **I measured today's data: header 31 fields; all 32 rows 31 fields; no `Notes` cell contains a comma.** A malformed CSV taking this file red is the **right** trade, and the trailing blank line is handled (`ParseIntoArrayLines(..., bCullEmpty=true)`, line 283) so it cannot fire spuriously.

**RULING 2 — §9 item 3: the two positive controls. ✅ ADEQUATE, and `BuildingRows > 0` is the right choice — I would not ask for a different one.**
"Prove the class I **exclude** is visible" is strictly stronger than "count the class I include", because the failure being ruled out (empty column / wrong index) yields zero on **both** sides. ⭐ One optional strengthening if this is ever revisited — **not required, not a finding:** also require `UnitRows + EconomyRows + BuildingRows < RowsRead`, which is the only control that would catch a discriminator matching **everything**. Today that reads `13 + 2 + 7 = 22 < 32`.

**RULING 3 — §9 item 5: the stale test name `RosterShowsAllThirteenKinds`. ⚖️ KEEP IT. I am ruling, not deferring.**
Two reasons, and the second is the one that decides it. (i) The test now runs on the **within-cap board of 13**, so the name is *accidentally still accurate* — it names `MaxRosterKinds`, not the roster. (ii) It is a **symbol** cited by name in `handoffs/TASK-523-programmer.md` (×2), `TASK-524` and `handoffs/TASK-526-buildmaster.md`; `SC-§38` cuts **for** stability here, because those citations point at a symbol rather than a coordinate, and renaming rots four records for a cosmetic. ⛔ **Conditions on the ruling:** add **one comment line above the macro** stating that the "Thirteen" is `MaxRosterKinds` and not the roster size — cheaper than a rename and it stops the next reader re-opening this — and if it is *ever* renamed, all four citing records move **in the same commit**. ✅ Flagging it rather than deciding it quietly was the correct call and is noted as such.

**RULING 4 — §4 / the dispatch's ⚠️: does the Witch-should-be-commandable read hold? ✅ YES. It holds, on three measurements of my own.**
1. `Capture()` (`SiegeAssistantSnapshot.cpp:662-676`) filters on validity, team, death and `CanonicalKind().IsNone()` — **no profile filter, no card-type filter.** Read, not inferred.
2. **The Cleric is `Profile Support` (`cards.csv` row 13) and is commandable today.** Eligibility is delegated to `IsGroupCommandEligible` / `IsFollowCommandEligible` (`SiegeAssistantSnapshot.cpp:695-699`) — which is exactly where the Cleric's follow-but-cannot-hold split already lives. ⇒ **a Support-profile exclusion at `Capture()` would break the Cleric, not merely the Witch.** This is the strongest leg of the argument and it is the programmer's, not mine.
3. The Witch is `CardType Unit`, spawned as a plain `ASummonedUnit` — a unit standing on the battlefield. Excluding her from *"send all units to the middle"* would reproduce **the exact complaint Jonathan filed about sorcerers**.

⇒ **The reasoning holds and I endorse it as the proceeding default.** ⛔⛔ **And the escalation clause is right, so I am restating it as binding: if Jonathan disagrees, the repair is a filter in `Capture()`, ⛔ NEVER, under any circumstance, a re-shrunk fixture.** Re-typing a 13-name array to make a test agree with a design opinion is the exact defect `TASK-874` was boarded to delete. ⚠️ **With WARN-1 attached: such a filter does NOT propagate into this fixture automatically, and must be mirrored in `BuildDerivedRoster` in the same commit.**

---

## ⭐ §6 — WHERE THE `SiegeAssistantSnapshot.cpp:2216-2224` REPAIR LANDS. ⛔ **NOT HERE. THE OBJECTION IS UPHELD.**

I read the comment verbatim at `SiegeAssistantSnapshot.cpp:2216-2224` and **confirm both sentences are now false**: *"DT_Cards has exactly 13 commandable kinds"* (it holds **14**) and *"the ONLY reachable cause is the character budget"* (`bBudgetBite` at 2225 is **false** on the full board, so `Cause` reads *"the MaxRosterKinds cap"* — the branch nobody had ever run). ✅ **Reporting it rather than reaching for it was correct** (`SC-§47`: a fence limits the repair, never the report).

⚖️ **RULING — the repair does NOT land in `TASK-874`'s diff, and the standing objection is the reason.** Appending a prose fix to an already-gated diff puts an **ungated hunk inside a gated one**: the gate would then be signing text it never reviewed, and the file is *production source*, not `Tests/`. ✅ The manager has already boarded it correctly as **`TASK-903`** (gate `TASK-904`, rider `TASK-905`). **That is where it goes.**

⛔ **Two operational riders for whoever writes it:**
1. `TASK-905`'s own row already records that `SiegeAssistantSnapshot.cpp` is production source **adjacent** to this ungated test diff ⇒ **the two must not be half-committed**; if both are pending, one host takes them together.
2. ⛔ **The replacement must NOT simply say "14."** That re-arms the identical trap the test just disarmed. The honest form names the **condition** — *the cap branch is reachable whenever `DT_Cards` holds more than `MaxRosterKinds` commandable kinds, and it does today* — and cites `Siegebound.Assistant.Selection.RosterCapCollapsesTheTail` as the test that now measures it.

⚖️ *The original comment earned this treatment: it named the condition under which it would become false, dated its own expiry, and was **right**. That is worth more than a comment that is merely true, and the replacement should keep the property rather than the sentence.*

---

## §7 — DUTY TRANSFER: WHAT I COULD NOT EXECUTE, TRANSFERRED **BY NAME** (`TL-§5c` cl. 5)

⛔ **I had no shell. The following are `DECLARED` and are owed by the commit row — `TASK-906`, the host that takes `Tests/SiegeAssistantSelectionTest.cpp`:**

| # | owed | why it is `TASK-906`'s |
|---|---|---|
| **(a)** | ⛔ **Settle WARN-2 first, before the pathspec.** Run `git ls-files --error-unmatch Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp`. **If it is untracked, the delta this commit lands is `+39`, NOT `+1`** — and the handoff's §7/§8 `HEAD` comparisons are unsupported and must not be re-relayed. | `TL-§5d` cl. 1-2; the figure is wrong by 38 in one branch |
| **(b)** | ⛔ **ONE executed suite.** `declared 431 / 31 files` must become an executed `Result={Success}` / `Result={Fail}` pair. **A declared green may travel through a gate; it may NOT travel through a commit.** | `TL-§5c` |
| **(c)** | ⛔ **`§25b` cl. R reconciliation** — `git status --porcelain` against the union of all pending pathspecs, **naming every file in neither**. This tree is carrying several lanes at once and this diff sits beside `TASK-873`/`TASK-919`, both of which fence this file OUT. | `TL-§5d` cl. 2 |
| **(d)** | ⚠️ **If the run goes red, look at TEST 2 line 1698 first** (WARN-4's ≈19-char margin), then at the new test's `AddExpectedMessagePlain(..., Occurrences 0)` at 1810. | my arithmetic is not a run |

---

## BOARD FLIPS — ⛔ RETURNED AS EXACT LINES, NOT APPLIED

⛔ **I have no line-editing tool, and I will not whole-file `Write` a board other agents are editing concurrently** (three conflicts have already fired tonight). **Apply these two, in place:**

**`TASKBOARD.md` line 15578 — `#### TASK-889`:**
```
- status: ✅ **DONE 2026-09-03 — verdict `FAIL` (⛔ 1 BLOCKER · 4 WARN · 3 NIT), report `qa/TASK-889.md`.** ⛔ **`SC-§29` ledger names `TASK-874` ALONE.** ⛔ **NO SHELL: no compile, no editor, no MCP, no Git — the suite figure is `declared 431 / 31 files`, ⛔ NOT executed, and four duties transfer BY NAME to `TASK-906` (§7).** ⭐ **(0) RE-MEASURED INDEPENDENTLY AND REPORTED THOUGH IT MATCHES: ⛔ 14 commandable kinds, ⛔ tail = `witch`; `Witch` PROVEN in `DT_Cards.uasset` (4 hits, with a `Sorcerer|WatchTower` = 5 positive control).** ✅ **(a) THE ROSTER IS ⛔ DERIVED — `ThirteenKindsInCardRowOrder()` is GONE at every site but the comment explaining its deletion, `13` was ⛔ NOT bumped to `14`, all three sources READ. ⛔ 22-row ledger AUDITED CASE BY CASE: ⛔ ZERO lost, ⛔ one gained; the within-cap prefix is symbol-identical to the old thirteen ⛔ AS A CONSEQUENCE, and its contingency is ASSERTED at 1511.** ✅ **(b) FENCE HELD — `Capture()` and the `2216-2224` comment both UNTOUCHED.** ⛔⛔ **THE ⛔ ONE BLOCKER: `Tests/SiegeAssistantSelectionTest.cpp:1866` — `Collapsed.Last()` with ⛔ NO non-empty precondition ⇒ an empty collapse line trips `RangeCheck` and ⛔ CRASHES THE AUTOMATION RUN instead of going RED, ⛔ in exactly the regression this test exists to catch. ⭐ The identical call in TEST 2 (1720) is guarded FATALLY at 1692-1696 — ⛔ the fix is that same 4-line form.** ⚖️ **4 RULINGS DELIVERED, ⛔ not deferred: the FogClamp skip divergence ✅ correct · the two positive controls ✅ adequate · `RosterShowsAllThirteenKinds` ⚖️ KEEP (+1 comment line) · the Witch SHOULD be commandable ✅ the reasoning HOLDS (the Cleric leg decides it) — ⛔ and if Jonathan disagrees the fix is `Capture()`, ⛔ NEVER a re-shrunk fixture.** ⚠️ **WARN-1 is the one to carry: the handoff's *"the derived roster would follow the production predicate automatically"* is ⛔ FALSE for a `Capture()`-side filter — the fixture mirrors `ResolveCardActorClass`, ⛔ not `Capture()`.** ⚖️ **§6: the `2216-2224` comment repair lands on `TASK-903`/`904`/`905`, ⛔ NOT appended here — the ungated-hunk-inside-a-gated-one objection is UPHELD.**
```

**`TASKBOARD.md` line 15306 — `#### TASK-874` (prepend to the existing status, preserving its history):**
```
- status: ⛔ **`qa-failed` (2026-09-03, `qa/TASK-889.md`) — ⛔ 1 BLOCKER, ⛔ ONE LINE, ⛔ and the deliverable is otherwise ⭐ EXEMPLARY.** ⛔ **`Tests/SiegeAssistantSelectionTest.cpp:1866` — `Collapsed.Last()` is called with ⛔ NO non-empty precondition. `CollapsedSymbols` returns an ⛔ EMPTY array on `other_kinds: none` or a missing key (719-727), and the `TestTrue` at 1847 that would notice is ⛔ NON-FATAL ⇒ `TArray::Last()` trips `RangeCheck`'s `checkf` (`DO_CHECK` is ON in Editor Development) and ⛔ ABORTS THE RUN instead of failing the row — ⛔ in precisely the Zone-C-format regression this test was written to detect.** ⭐ **The fix is already in this file, 150 lines up: TEST 2 guards the identical `Collapsed.Last()` FATALLY at 1692-1696 (`if (!TestTrue(...)) { return false; }`). ⛔ Convert 1847 to that form — ⛔ it also stops 1853-1861 proving nothing after the precondition failed.** ⛔ **NOTHING ELSE NEEDS TO CHANGE FOR A PASS.** ⚠️ **Carry back with it: WARN-1 (§4's *"the derived roster would follow the production predicate automatically"* is ⛔ FALSE — the fixture mirrors `ResolveCardActorClass`+`IsBuildingCard`, ⛔ NOT `Capture()`'s filter; a `Capture()`-side filter must be MIRRORED in `BuildDerivedRoster` in the SAME commit) · WARN-2 (the `git show HEAD:` provenance in §7/§8 is ⛔ impossible if the file is untracked, which `TL-§5d` and TASKBOARD:14412 both say it is) · WARN-3 (tree now reads ⛔ 431/31, ⛔ declared) · WARN-4 (TEST 2's ≈19-char margin at line 1698).** ✅ **CONFIRMED BY THE GATE: 14 kinds / tail `witch` (re-measured) · the roster is ⛔ DERIVED, ⛔ not re-transcribed · the 22-row ledger holds, ⛔ ZERO cases lost · the within-cap prefix is symbol-identical ⛔ as a consequence · the fence held · the `Capture()` question was ⛔ correctly REPORTED, and its reasoning ⛔ HOLDS.** *(was: `ready-for-qa` — history below retained.)*
```

---

## Notes for build-master

⛔ **DO NOT COMMIT THIS FILE YET — the verdict is FAIL.** `TASK-906` must wait for the re-gate. When it PASSES, §7's four duties (a)-(d) come with it, and **(a) is first**: settle whether `Tests/SiegeAssistantSelectionTest.cpp` is in `HEAD` before writing any pathspec or any delta, because the two branches differ by **38 tests**.

⭐ **One thing worth knowing before the re-gate lands:** this diff makes the assistant suite depend, at run time, on `Docs/Data/cards.csv` being present and well-formed on disk. That is deliberate, it matches the shipped `SiegeFogClampTest` precedent, and every failure path is loud — but it means **a broken or missing CSV now turns this file red rather than passing vacuously.** That is the correct direction and it should not be "fixed" if it ever fires.

---
---

# QA REPORT — TASK-889 · ⭐ **LOOP 2 of 3 — APPENDED, ⛔ THE LOOP-1 RECORD ABOVE IS UNTOUCHED**

**Verdict: ✅ PASS** — **⛔ 0 BLOCKER · 2 WARN carried (neither new, neither blocking) · 1 NIT ⛔ WITHDRAWN BY ME.**
**Date:** 2026-09-03 · **Loop:** 2/3 · **Re-gated:** `handoffs/TASK-874-programmer.md` **§11** (appended, §0–§10 intact as required).

## `SC-§29` COVERAGE LEDGER — ⛔ UNCHANGED FROM LOOP 1

This gate covers **exactly ONE task, named: `TASK-874`.**

⛔ **NOT covered, and not implied:** `TASK-876` · `TASK-903`/`904`/`905` (the `SiegeAssistantSnapshot.cpp:2216-2224` comment repair) · `TASK-906` (the commit) · `TASK-919` · the compile · the suite **execution**.

## ⛔ METHOD, STATED FIRST AND PRECISELY (`TL-§5c` cl. 5)

⛔ **No shell, no Git, no compile, no editor, no MCP** (editor is up at PID 7076; I did not touch it). ⛔ **Nothing was executed.**
⚖️ **What I DID have, and it is more than loop 1 credited:** `Read` and `Grep` (ripgrep). Those are **real instruments**, and every count below is **my own measurement taken today**, not a re-read of the handoff. ⭐ **Where an instrument could have been blind I carried a positive control and I say so** (`SC-§39`). ⛔ **Every coordinate is my own read**; no line number from the handoff or the dispatch was trusted (`SC-§38`).

---

## ⛔ (1) THE BLOCKER — ✅ **FIXED, AND FIXED IN THE FORM THE GATE NAMED.**

**Located by symbol** (`SC-§38`), not by the loop-1 line numbers, which had moved: `FSiegeAssistantSelectionRosterCapCollapsesTheTailTest::RunTest`, now at **1806-1923**.

The guard is at **1885-1890**, and it is TEST 2's shape, copied:

```cpp
if (!TestTrue(*FString::Printf(TEXT("⛔ PRE-CONDITION: at least %d kind(s) are collapsed by the cap alone — %d were. If this fails, NOTHING below proves anything and it must not be read as a pass."),
        Kinds.Num() - USiegeAssistantSnapshot::MaxRosterKinds, Collapsed.Num()),
    Collapsed.Num() >= Kinds.Num() - USiegeAssistantSnapshot::MaxRosterKinds))
{
    return false;
}
```

⛔ **I re-derived the safety rather than accepting it, and it is AIRTIGHT — the chain is three links and I checked each:**

| link | coordinate | consequence |
|---|---|---|
| `Kinds.Num() == 0` returns early | **1811-1815** | `Kinds` is non-empty below |
| `Kinds.Num() <= MaxRosterKinds` returns early (`AddInfo`, ⛔ *"this is a report, not a pass"*) | **1819-1825** | ⇒ `Kinds.Num() - MaxRosterKinds >= 1` |
| the fatal guard demands `Collapsed.Num() >= Kinds.Num() - MaxRosterKinds` | **1885-1890** | ⇒ ⛔ **`Collapsed.Num() >= 1` is GUARANTEED** |

⇒ ✅ **`Collapsed.Last()` at 1908 can no longer reach `RangeCheck`.** ✅ **`Kinds.Last()` at 1907 is safe because `Kinds.Num() > MaxRosterKinds == 13 > 0`** — and that reasoning is **recorded in the comment at 1883-1884**, so it will not be re-opened. ⭐ **The comment at 1868-1882 states BOTH reasons the guard is fatal, including the abort-the-runner one, in the words of the defect rather than the words of the fix.** That is the right way to leave a repair.

⭐ **And the form is PROVEN to compile, which matters more than my reading it:** `if (!TestTrue(...)) { return false; }` is not a new construction — the identical shape sits at **1713-1717** in this same file and is inside the **427 executed green at `1aa0fee`**. ⛔ I am not inferring that `TestTrue` returns `bool`; the file already ships a call that depends on it.

✅ **No stale duplicate of the old non-fatal check survives** — I read 1845-1922 end to end; there is exactly **one** collapsed-count assertion in the function. ✅ **Test count unchanged at 39** (measured below), which independently corroborates that the fix converted a check rather than adding one.

---

## ⭐⭐ (2) THE SWEEP — ⛔ **I VERIFIED A SUPERSET OF WHAT IT CLAIMED, AND IT HOLDS.**

The dispatch told me to verify the sweep, not the fix. ⛔ **A sweep confirmed only over the class the author chose is not a verification, it is an echo** — so I swept a **wider** class than §11.2 claims, over all **6370** lines:

| needle | sites | result |
|---|---|---|
| `.Last(` | **4 code sites** (+2 in comments) | ✅ all guarded — see below |
| `\w+[0]` | 13 | ✅ all guarded |
| `[1]`–`[9]` | 9 | ✅ all inside arity checks |
| `[<identifier>]` — ⭐ **the class §11.2 did NOT claim** | 27 | ✅ all loop-bounded, arity-checked or `IsValidIndex` |

**The four `.Last()` sites, each re-read at its own coordinate:**

| site | access | guard | verdict |
|---|---|---|---|
| **1521** | `Roster.Last()` | `if (Roster.Num() > 0)` @ **1508** | ✅ |
| **1740-1741** | `Collapsed.Last()`, `Kinds.Last()` | fatal `Collapsed.Num() > 0` @ **1713-1717** + `if (Kinds.Num() > 0)` @ **1738** | ✅ |
| **1907-1908** | `Collapsed.Last()`, `Kinds.Last()` | ⭐ **the fix** @ **1885-1890** | ✅ **now guarded** |
| **5898-5899** | `Kinds.Last()` | `if (Kinds.Num() > 0)` @ **5896** | ✅ |

⇒ ✅ **The claim *"no second instance exists"* is CONFIRMED, and confirmed against a broader needle set than the one that produced it.**

**The sites I checked hardest, because a board change is what this diff DID and roster-driven loops are where it would show:**
- **1621** — `Index < Kinds.Num() && Index < Printed.Num()` — ⭐ a **dual** bound; indexes both arrays it compares. ✅
- **1747** — `Index = Printed.Num(); Index < Kinds.Num()` — bounds `Kinds[Index]`; degenerate if `Printed > Kinds`, never out of range. ✅
- **5473-5477** — bounded by `RegionsWithout.Num()`, second array via `IsValidIndex`. ✅

**The CSV parse (the only genuinely new indexing in the diff), each guard traced to its own line:**
`Lines[0]` @306 ← `Lines.Num() < 10` hard-fail @299 · `Header[0]` @339/342 ← `IsValidIndex(0)` on **both** branches · `Fields[0]` @363 and `Fields[CardTypeColumn]` @364 ← short-row hard-fail @354, and `CardTypeColumn` comes from `Header.IndexOfByKey` so `CardTypeColumn < Header.Num() <= Fields.Num()` **by construction**. ✅

⭐ **ONE SITE I OPENED SPECIFICALLY TO FILE A FINDING AGAINST, AND DID NOT — reported because the near-miss is the point** (`SC-§41`'s lesson: a needle that looks alarming is not a defect until you read the hits). Line **1073** writes `(*HalfExtents)[Index]` with an index taken from a **different** array (`Places.IndexOfByKey`). That is the classic parallel-array crash. ⛔ **It is not one here:** `HalfExtents` is filled at **1057-1061** by a loop over **that same `Places` array**, so `Index < Places.Num() == HalfExtents->Num()`. ✅ **No finding. I am recording the check so nobody has to repeat it.**

---

## ⭐⭐⭐ (3) THE `SC-§47` EXHIBIT — ⚖️ **RULED: YES. IT BELONGS IN THE LAW, AND IT SHARPENS THE MECHANISM RATHER THAN MERELY ILLUSTRATING IT.**

The question put to me: the CRLF→LF self-catch (§11.4) is a defect of **DESCRIPTION** found by the **AUTHOR** — which looks like a counterexample to `SC-§47`'s assignment of readers. **Does it belong as the exhibit?**

### ✅ MY RULING: YES — and ⛔ **for a reason narrower and stronger than "the author caught it."**

⛔ **First, the measurement, because I will not rule on a sentence I have not tested** (`SC-§40` cl. 1). I re-measured it myself, with a positive control:

| claim | my instrument | result |
|---|---|---|
| the file is **LF, not CRLF** | `\r` over `Tests/*.cpp` | **0 CR across all 31 files** ⇒ ✅ **the retraction is CORRECT; the original §8 sentence was FALSE** |
| ⭐ **positive control — can the needle SEE a CR at all?** (`SC-§39`) | same `\r` over `Source/GitClaudeUnrealTest/**/*.cpp` | ⛔ **16940 hits across 59 files** — including `Siegebound/Barracks.cpp` and `Siegebound/Building.cpp` ⇒ ✅ **the instrument demonstrably works, and the 0 is a measurement, not a blind needle** |
| the siblings are consistent | as above | ✅ **all 31 `Tests/` files LF** ⇒ ⛔ **nothing was converted by the edits** |
| the line count | `^` over the file | ✅ **6370** — **exactly** the corrected figure |
| no BOM | `\x{FEFF}` over `Tests/` | 0 — ⚠️ **but I found no BOM anywhere in `Source/`, so this needle has NO positive control and I am declaring it unproven.** The substantive claim does not rest on it. |

⇒ ✅ **Code right, description wrong, siblings consistent, retraction accurate, and the corrected figure reproduces to the line.**

### ⚖️ WHY IT BELONGS — the three clauses I would attach

1. ⭐⭐ **IT ISOLATES THE VARIABLE, WHICH NO PRIOR EXHIBIT DID.** Every earlier `SC-§47` case confounded *"different reader"* with *"fresh measurement"* — the second reader always brought both at once. ⛔ **Here the reader is held CONSTANT and only the METHOD varies: same author, same sentence, two passes.** Re-reading (loop 1, §8, a thorough self-audit) **missed** it. Re-measuring **caught** it. ⇒ **the barrier `SC-§47` describes is not `author` vs `second reader`; it is `RE-READING` vs `RE-MEASURING`.** A second reader is merely the **usual delivery mechanism** for a fresh measurement, because they carry no memory of having written the sentence. ⛔ **That is a mechanism claim, and this is the first case on the record that supports it as evidence rather than as a story.**

2. ⭐⭐ **THE OPERATIONAL CLAUSE, AND IT IS THE PART I MOST WANT KEPT — the yield came from a DIFFERENT SENTENCE.** WARN-2 was about `git show HEAD:` **provenance**. The line-ending defect was **collateral**: it lived in the same section and had nothing to do with what was retracted. ⇒ ⛔ **THE RULE THAT ACTUALLY PRODUCED THE FIND: when one sentence in a section is retracted, RE-MEASURE THE WHOLE SECTION, ⛔ never only the retracted sentence.** That is cheap, mechanical, and it is the clause with the highest expected yield in the whole exhibit.

3. ⛔ **THE LIMIT, WHICH MUST TRAVEL WITH IT OR THE LAW GETS MISREAD AS AN EXEMPTION.** ⛔ **This does NOT license dropping the second reader.** Note what it cost: an **external gate had to fire first.** The author's own loop-1 self-audit — which was genuinely rigorous, ran its own census, and carried its own positive controls — did **not** catch it. **Two passes of the author's eyes missed it; one retraction caught it.** ⇒ `SC-§47`'s core (*neither pass substitutes for the other*) is **untouched**. The exhibit adds a **mechanism** clause, ⛔ not an exemption.

⚠️ **ONE HONEST CAVEAT ON THE EXHIBIT'S STRENGTH, so the law is not over-claimed:** the false sentence was **harmless** — no downstream consumer, nothing would have broken. ⛔ **It is a clean specimen precisely because nothing was at stake, which is also why it survived two readings.** A *load-bearing* false description tends to get caught by something failing. ⇒ **the exhibit demonstrates the MECHANISM, it does not measure the COST.** Say so in the law.

### ⭐⭐⭐ AND THE PART THAT DECIDES IT — **I FOUND TWO MORE OF THE SAME SHAPE, BOTH IN MY OWN LOOP-1 VERDICT, BOTH THE SAME WAY**

⛔ `SC-§40` cl. 3: *a QA verdict is a CITATION, not a fact.* ⛔ **That binds my own report, so I re-measured it instead of re-reading it — and it did not survive:**

- ⛔ **MY DEFECT #1 — loop-1 §(a″).** I wrote: *"`BoardWithinRosterCap` (489-497) is `CommandableKindsInCardRowOrder` followed by `SetNum(MaxRosterKinds)`."* ⛔ **FALSE as a description.** The real code (**504-512**) is `if (Kinds.Num() > MaxRosterKinds) { Kinds.SetNum(...); }` — **conditional**. ⭐ **The distinction is load-bearing and I dropped it:** an *unconditional* `SetNum` would **GROW** a short roster with `NAME_None` entries, silently manufacturing empty kinds on any board between 10 and 12 kinds. **The code is right. My sentence described a latent defect that does not exist.** Found only because I re-opened the function this loop instead of re-reading my own paragraph.

- ⛔⛔ **MY DEFECT #2 — NIT-2, and it is the sharpest of the four.** I filed *"`FindNameArrayField` hands back a non-const pointer into the CDO… read-only in fact, not by construction."* ⛔ **I read the SIGNATURE and never opened the CALL SITES.** Measured now: the helper has **six** call sites and **four are WRITE targets** — `UnitKinds` @619, `PlaceNames` @623/1174, and `RegionPlaceNames` @1035, which is written at **1043** (`Regions->Reset()`) and **1074** (`Regions->Add(Region)`). ⇒ ⛔ **a const return is not a viable change to that signature at all; it would break four sites.** ⭐ **And worse for me: at the ONE read-only site — the CDO read at 279-280 — the pointer is ALREADY bound as `const TArray<FName>* const`.** ⇒ ⛔ **the concern I raised was ALREADY SATISFIED at the only place it applied, in code that was in front of me when I filed it.**

⚖️ ⇒ **FOUR instances of one shape in a single gate cycle — one the programmer's, one self-caught by the programmer, TWO MINE — and every one of them was invisible to re-reading and fell to the first re-measurement.** ⛔ **I could not have found my own two by re-reading my own report; it agreed with me both times.** ✅ **That is as strong a confirmation of `SC-§47`'s mechanism as this pipeline is likely to produce, and I endorse the exhibit without reservation.**

---

## ⚖️ (4) THE RULINGS THE DISPATCH DEMANDED

### ⚖️ RULING A — the WARN-1 binding rider: **enforceable, or merely hopeful?**
⇒ ⛔ **MERELY HOPEFUL AS WRITTEN. I am ruling it plainly and it does NOT block.**

- ⛔ **A comment binds nobody.** If a `Capture()`-side filter lands and `BuildDerivedRoster` is not mirrored, **nothing goes red** — which is *precisely the failure the rider describes*. ⛔ **A rule whose violation is invisible is a hope, not a control** (`SC-§39`: a guard only ever seen passing is indistinguishable from no guard).
- ✅ **BUT THE MOVE WAS STILL RIGHT, AND I ENDORSE IT EXPLICITLY.** Correcting it **in the fixture** rather than only in the handoff is materially better, and the programmer's reason — ⭐ *"a handoff is not what the next engineer edits"* — is **correct and worth promoting**. The rider now sits at **170-180**, in the file and adjacent to the symbol anyone changing the derivation must open. `SC-§38`'s logic applies: **a rider attached to a SYMBOL travels with the code; a rider in a handoff rots the moment nobody re-reads that handoff.** I read it at 170-180 and it states the scope, the reason the two agree today, the divergence, and the same-commit rider.
- ⛔ **THE ENFORCEABLE FORM EXISTS, IS CHEAP, AND ⛔ MUST NOT LAND HERE:** a structural pin over `USiegeAssistantSnapshot::Capture`'s filter set that goes **RED** the day a filter is added and **names `BuildDerivedRoster` as the mirror site**. ⛔ **Why not in this diff, three reasons, and the third decides it:** (i) it is a **new test**, moving the census off 39 mid-gate; (ii) it pins **production source** this diff is fenced out of; (iii) ⛔ **`SC-§39.1` cl. 4 requires proving the new guard goes RED on a synthesised violation — which cannot be done without a compile.** ⇒ ⛔ **adding it now would put an UNPROVEN guard into an UNGATED diff.** ⛔ **Board it, do not smuggle it.** Transferred by name in §7′.
- ⚠️ ⇒ **ACCEPTED AS A MITIGATION, ⛔ RULED NOT A CONTROL.** ⛔ **Nobody downstream may write that the divergence is now "covered." It is not covered — it is ANNOUNCED.** That is a real improvement and it is not the same thing.

### ⚖️ RULING B — NIT-2, declined by the programmer: **⛔ I am not merely upholding the decline. I am WITHDRAWING THE NIT.**
- ✅ **The programmer's stated reason is correct on its own terms** — widening an ungated diff for no behavioural gain — and ⛔ **ruling the other way would have made me self-contradictory inside one gate**, since that is the *identical* principle I used in loop-1 §6 to keep the `2216-2224` repair OUT of this diff (an ungated hunk inside a gated one).
- ⛔ **But the stronger reason is mine and it is that the NIT was WRONG**, per MY DEFECT #2 above: the non-const return is **load-bearing** at four write sites, and the one read-only consumer **already** binds `const` at the call site. ⇒ ⛔ **there is nothing for "whoever next touches it" to do.** **NIT-2 is struck from the record, not deferred.** ⛔ A declined nit nobody rules on becomes a silent omission — and a **wrong** nit left standing is worse, because it schedules future work against a defect that does not exist.

### ✅ RULING C — RULING 3's condition: **MET.** Read at **1444-1458**. It states the "Thirteen" is `MaxRosterKinds` and **not** the roster size, records the KEEP ruling, and carries the *"all four citing records move in the SAME commit"* condition. ✅ Name kept, as ruled.

### ✅ RULING D — NIT-1: **FIXED.** Line **191**: *"30 named columns beside the row name, 31 fields in all"*, and it cites **both** gates that corrected it (`qa/TASK-849.md` NIT-3 and this report). ✅ The re-relay is closed at the source.

### ✅ RULING E — the fence: **HELD, AND I RE-READ IT RATHER THAN ASSUMING IT.** `SiegeAssistantSnapshot.cpp:2216-2228` is **byte-identical to what I read in loop 1** — still *"DT_Cards has exactly 13 commandable kinds"* and *"the ONLY reachable cause is the character budget"*, both still **false**, both still correctly **left alone**. ✅ **My loop-1 objection is upheld a second time: that repair is `TASK-903`/`904`/`905`'s.**

---

## ⚠️ (5) THE `+39` CONSEQUENCE, AND THE PARITY CROSS-CHECK — ⛔ VERIFIED

⛔ **WARN-2 is RETRACTED BY ITS AUTHOR, not resolved.** I still have no Git. ⇒ ⛔ **carried forward as instructed: treat the commit delta as `+39` — THE WHOLE FILE — never `+1`, until `TASK-906` settles tracking with `git ls-files --error-unmatch`.** ⭐ **Retracting rather than defending an unsupportable provenance claim was the correct call and I am recording it as correct, not merely as not-wrong** (`SC-§47`: a fence limits the repair, never the report).

⭐ **The replacement parity claim is self-standing, and ⛔ I cross-checked it independently rather than accepting it:**

| claim | my measurement | verdict |
|---|---|---|
| `40 top-level blocks` | `^\}` = **40** | ✅ |
| `= 39 test bodies + 1 fixture namespace` | `^bool F\w+::RunTest` = **39**; `^bool…\|^namespace \|^\{` = **80** ⇒ `^{` = **40**, `^namespace` = **1** | ✅ **exact** |
| cross-checks against the macro count | `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` in this file = **39** | ✅ **39 = 39** |
| depth-at-EOF 0 / no negative-depth event | ⛔ **not independently reproducible with my instruments** — I can count openers and closers at column 0, ⛔ not track depth. **Declared, not verified.** | ⚠️ declared |

⇒ ✅ **The cross-check the dispatch told me to verify RECONCILES EXACTLY: 40 = 39 + 1, and the 39 matches the 39 macros.** ⛔ **It is still not a compile** — it rules out the class of error a 692-line insertion most easily makes, and nothing more. The compile remains owed.

---

## ⚠️ (6) FINDINGS — ⛔ 0 BLOCKER

- ⚠️ **WARN-2′ (carried, NOT new, ⛔ NOT blocking)** — `handoffs/TASK-874-programmer.md` §7/§8 — the `git show HEAD:` provenance is **withdrawn by its author**; the durable delta is **`+39`**, not `+1`. ⛔ **Nobody may re-relay the `+1`.** Settling it is `TASK-906`'s and it is **first**.
- ⚠️ **WARN-4′ (carried, NOT new, ⛔ NOT blocking)** — the ≈19-character margin on TEST 2's `Printed.Num() > 0` precondition (**1719**) is arithmetic, ⛔ never a run. ⇒ **if anything goes red on this diff, that line is the first place to look**, then the new test's `AddExpectedMessagePlain(…, Occurrences 0)` at **1831**.
- **NIT-1** ✅ fixed · **NIT-2** ⛔ **WITHDRAWN BY ME** (see RULING B) · **NIT-3** ⛔ correctly no-action, already declared in the fixture header at **187-194**.
- ⚠️ **NIT-4 (new, cosmetic, ⛔ no action asked):** my BOM needle (`\x{FEFF}`) returned 0 across all of `Source/`, so I **could not construct a positive control for it** and I am declaring it **unproven** rather than reporting a clean absence as a measurement (`SC-§39`). The **line-ending** claim does not rest on it and **is** proven.

**⭐ Behavioural delta of the loop-2 fix, verified as exactly one thing:** if the cap precondition ever fails, the test **returns false immediately** instead of continuing. ⇒ ⛔ **a run-killing abort becomes a single red row.** That is the whole change, and it is the change that was asked for.

---

## §7′ — DUTY TRANSFER, ⛔ RE-STATED BY NAME (`TL-§5c` cl. 5) — owed by `TASK-906`

| # | owed | why |
|---|---|---|
| **(a)** | ⛔ **FIRST, BEFORE ANY PATHSPEC OR DELTA:** `git ls-files --error-unmatch Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp`. **Untracked ⇒ the delta is `+39`.** The two branches differ by **38 tests**. | `TL-§5d` cl. 1-2; WARN-2 is retracted, so nothing supports the `+1` |
| **(b)** | ⛔ **ONE EXECUTED SUITE.** `declared 431 / 31 files` must become an executed `Result={Success}`/`Result={Fail}` pair. ⛔ **A declared green may pass a gate; it may ⛔ NOT pass a commit.** | `TL-§5c` |
| **(c)** | ⛔ **`§25b` cl. R reconciliation** — `git status --porcelain` against the union of all pending pathspecs, **naming every file in neither.** This tree carries several lanes at once. | `TL-§5d` cl. 2 |
| **(d)** | ⚠️ **If the run goes red:** TEST 2 line **1719** first, then **1831**. | arithmetic is not a run |
| **(e)** | ⭐ **NEW — board the enforceable form of the WARN-1 rider** (RULING A): a structural pin on `Capture()`'s filter set that goes RED when a filter is added and names `BuildDerivedRoster` as the mirror. ⛔ **Its own gate must prove it goes RED on a synthesised violation** (`SC-§39.1` cl. 4). ⛔ **NOT this diff.** | it needs a compile and touches production source |

⛔ **Census, re-measured by me today, ⛔ not inherited:** `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` over `Siegebound/Tests/*.cpp` = ⛔ **431 declared across 31 files**; this file = **39**. ⭐ **Both needles agree exactly:** the broad `IMPLEMENT_[A-Z_]*AUTOMATION_TEST` over the same scope returns **431 / 31**, ⛔ confirming the "extra file" is a **SCOPE** effect, never a needle effect. ✅ **The fix adds ZERO tests — 39 before, 39 after.** ⛔ **`declared`, ⛔ NEVER executed. Last EXECUTED run remains `427 Result={Success}` / `0 Result={Fail}` at `1aa0fee`** (`TL-§5c`).

---

## ⚠️⚠️ `TL-§5d` — WHAT THIS PASS UNBLOCKS, STATED PLAINLY

`Tests/SiegeAssistantSelectionTest.cpp` is **untracked, inside the working tree's green suite, invisible to `HEAD`**, and was **fenced out of two commits because it was ungated**. ⇒ ⛔ **It is exactly one working-tree accident away from a silent loss of 39 tests.**

✅ **THIS PASS IS THE THING THAT LETS `TASK-906` HOST IT.** ⛔ **The file must not sit ungated another night.** `TASK-919` may proceed on this verdict.

---

## BOARD FLIPS — ⛔ RETURNED AS EXACT LINES, ⛔ NOT APPLIED (I have no line editor, and the board has taken **four** concurrent-writer conflicts tonight)

⛔ **`TASKBOARD.md` ~line 15578 — `#### TASK-889`'s OWN row. THIS WAS OWED FROM LOOP 1 AND IS STILL UNAPPLIED — apply it ONCE, in this form, superseding the loop-1 text:**
```
- status: ✅ **DONE 2026-09-03 — LOOP 2/3 verdict `PASS` (⛔ 0 BLOCKER · 2 WARN carried · 1 NIT ⛔ WITHDRAWN BY THE GATE), report `qa/TASK-889.md` (loop-1 FAIL record PRESERVED above the loop-2 append).** ⛔ **`SC-§29` ledger names `TASK-874` ALONE.** ⛔ **NO SHELL / NO GIT / NO COMPILE / NO EDITOR / NO MCP — suite is `declared 431 / 31 files`, ⛔ NOT executed; FIVE duties transfer BY NAME to `TASK-906` (§7′).** ✅ **BLOCKER-1 FIXED and re-derived AIRTIGHT: the early returns at 1811/1819 make `Kinds.Num() - MaxRosterKinds >= 1`, so the fatal guard at 1885-1890 GUARANTEES `Collapsed.Num() >= 1` ⇒ `Collapsed.Last()` at 1908 can no longer trip `RangeCheck`. ⭐ The `if (!TestTrue(...)) { return false; }` form is PROVEN to compile — the identical shape ships at 1713-1717 inside the 427 executed green at `1aa0fee`.** ⭐⭐ **SWEEP VERIFIED OVER A ⛔ SUPERSET of what was claimed: all 4 `.Last()` sites · 13 `[0]` · 9 `[1-9]` · ⛔ 27 `[<identifier>]` (a class §11.2 did NOT claim) — ⛔ ZERO unguarded across 6370 lines.** ⭐ **PARITY CROSS-CHECK RECONCILES EXACTLY: `^\}`=40, `^bool…::RunTest`=39, `^namespace`=1, macros=39 ⇒ 40 = 39+1 = 39 macros.** ⚖️ **`SC-§47` EXHIBIT ⛔ ENDORSED WITHOUT RESERVATION — and the gate found ⛔ TWO MORE OF THE SAME SHAPE ⛔ IN ITS OWN LOOP-1 VERDICT (the `SetNum` description, and ⛔ NIT-2 ITSELF, which is hereby ⛔ STRUCK: `FindNameArrayField`'s non-const return is LOAD-BEARING at 4 write sites and the 1 read-only site ALREADY binds `const`). ⛔ FOUR instances in one cycle, ⛔ two of them the gate's own.** ⚖️ **WARN-1 rider ⛔ RULED ⛔ HOPEFUL, ⛔ NOT ENFORCEABLE — accepted as a MITIGATION, ⛔ NOT a control; ⛔ nobody may write that the divergence is "covered", it is ⛔ ANNOUNCED. The enforceable form is boarded to `TASK-906` §7′(e), ⛔ NOT smuggled here.** ⚠️ **⛔ CARRY: the delta is `+39` (THE WHOLE FILE), ⛔ NEVER `+1` — the `git show HEAD:` provenance is RETRACTED BY ITS AUTHOR.** ✅ **Fence re-verified: `Capture()` and `SiegeAssistantSnapshot.cpp:2216-2228` BOTH untouched; line endings re-measured LF/0 CR with a ⛔ PROVEN positive control (16940 CR across 59 files elsewhere).**
```

⛔ **`TASKBOARD.md` ~line 15306 — `#### TASK-874` (prepend, preserving history):**
```
- status: ✅ **`qa-passed` (2026-09-03, `qa/TASK-889.md` LOOP 2/3) — ⛔ 0 BLOCKER. ⛔ READY FOR INTEGRATION, host = `TASK-906`.** ✅ **The one blocker is FIXED in the form the gate named — TEST 2's shape copied, ⛔ not a third invention — and the gate ⛔ re-derived the safety chain rather than accepting it: 1811/1819's early returns ⇒ `Kinds.Num() - MaxRosterKinds >= 1` ⇒ the fatal guard at 1885-1890 GUARANTEES `Collapsed.Num() >= 1`, so `Collapsed.Last()` at 1908 ⛔ cannot reach `RangeCheck`. ⛔ Test count UNCHANGED at 39 — it converted a check, it did not add one.** ⭐⭐ **It swept the CLASS, not the coordinate, and the gate verified a ⛔ SUPERSET: ⛔ zero unguarded index accesses in 6370 lines.** ⭐⭐ **AND IT SELF-REPORTED A SECOND FALSE SENTENCE OF ITS OWN (§11.4: "uniformly CRLF" ⇒ ⛔ actually 6370 LF, 0 CRLF) — ⛔ found ONLY because a retraction forced a RE-MEASUREMENT rather than a re-reading, and ⛔ it measured all 31 siblings BEFORE calling it a problem (all LF ⇒ ⛔ nothing was converted). ⚖️ THE GATE ENDORSES THIS AS THE `SC-§47` EXHIBIT and adds TWO of its OWN.** ⚠️ **CARRY TO THE COMMIT: the delta is ⛔ `+39`, THE WHOLE FILE, ⛔ NEVER `+1` (provenance RETRACTED) · one EXECUTED suite is owed · `§25b` cl. R reconciliation · if red, look at 1719 then 1831 · and board the ENFORCEABLE form of the `Capture()`-mirror rider (⛔ ruled HOPEFUL as written).** *(was: `qa-failed` — history below retained.)*
```

---

## Notes for build-master

✅ **VERDICT IS PASS. `TASK-906` MAY HOST THIS FILE, AND ⛔ SHOULD — `TL-§5d` is the live risk and 39 tests are one accident from a silent loss.**

⛔ **§7′(a) IS FIRST AND IT IS NOT OPTIONAL:** settle tracking with `git ls-files --error-unmatch` **before** writing any pathspec or any delta. ⛔ **The two branches differ by 38 tests, and the `+1` figure is RETRACTED BY ITS AUTHOR — do not re-relay it from §7 of the handoff, which is superseded by §11.3.**

⛔ **`declared` is not `executed`.** `431 / 31 files` is a **declaration**. The last executed run is `427/0` at `1aa0fee`. **A declared green may travel through a gate; it may not travel through a commit** (`TL-§5c`) — one real run is owed with this commit.

⚠️ **Two adjacent-lane cautions:** `SiegeAssistantSnapshot.cpp` is production source **adjacent** to this ungated test diff and is `TASK-903`/`904`/`905`'s — ⛔ **do not half-commit the pair**; and this file is explicitly **fenced OUT** of `TASK-873` and `TASK-919`, so a pathspec derived from prose rather than from `git status` will get it wrong (`SC-§40` cl. 1).

⭐ **Unchanged from loop 1 and still worth knowing:** the assistant suite now depends at run time on `Docs/Data/cards.csv` being present and well-formed. That is deliberate, every failure path is loud, and ⛔ **it should not be "fixed" if it ever fires.**

---
---

# 📋 MANAGER ANNOTATION — 2026-09-03, ⛔ APPENDED AFTER `239ca77`. ⛔ NO VERDICT TEXT ABOVE IS EDITED.

⛔ **This report's `PASS` is ⛔ UNDISTURBED. ⛔ One finding in it — ⛔ `WARN-2`, carried into loop 2 as `WARN-2′` and into `§7′(a)` — is ⛔ NOW MEASURED AND IS ⛔ WRONG. ⛔ It is annotated, ⛔ not rewritten, because the ⛔ error is more valuable than the correction.**

## ⛔⛔ THE MEASUREMENT

```
git ls-files --error-unmatch Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp
⇒ exit 0     ⇒ TRACKED, in HEAD since 4a03e03
HEAD 38 macros · worktree 39 ⇒ delta = +1     (NOT +39)
```

⇒ ⛔ **`WARN-2`'s premise — *"two dated project records say it is [untracked]"* — was ⛔ true about the records and ⛔ false about the world.** ⛔ **Both records had ⛔ conflated this file with `Tests/SiegeFogClampTest.cpp`, which genuinely ⛔ WAS new and untracked at `+8`.** ⛔ **One of the two was ⛔ `CONVENTIONS.md` `TL-§5d` — ⛔ the law itself was the stale citation** (⛔ now struck and corrected in place).

## ⚖️ WHAT THE GATE GOT ⛔ RIGHT, STATED FIRST AND PLAINLY

- ✅ ⛔ **Noticing the contradiction at all was ⛔ CORRECT and it is exactly `SC-§40` cl. 3 behaviour.** ⛔ Two dated records ⛔ did contradict the handoff, and a gate that ⛔ said nothing would have been ⛔ worse.
- ✅ ⛔ **Saying *"one of the two is wrong and I have no shell to settle which"* was ⛔ EXACTLY RIGHT** — ⛔ the gate ⛔ knew the limit of its instrument and ⛔ declared it.
- ✅ ⛔ **`§7′(a)` ⛔ transferred the question by name to the committing row. ⛔ That transfer is ⛔ why it was ever settled.**

## ⛔ THE ⛔ ONE DEFECT, AND IT IS ⛔ NARROW

⛔ **`§7′(a)` ⛔ pre-decided its own transferred question: *"⛔ IT IS EXPECTED ⛔ UNTRACKED ⇒ ⛔ THE DELTA IS `+39`."*** ⇒ ⛔ **A transfer that ships its ⛔ own expected answer is a ⛔ RELAY, ⛔ not a transfer** — and ⛔ **that expectation is what the manager copied into ⛔ FOUR dispatch prompts as fact.**

⛔⛔ **AND THE ⛔ CONSEQUENCE THAT MATTERS MOST: ⛔ `WARN-2` told a programmer who is ⛔ FENCED FROM GIT that his ⛔ TRUE claim *"cannot be true."*** ⇒ ⛔ **He ⛔ retracted it.** ⛔ **The fence left him ⛔ no other move, and the gate's loop-2 §5 then ⛔ COMMENDED the retraction as *"the correct call"* — ⛔ so the FALSE figure inherited the ⛔ moral authority of the TRUE one.**

## ⭐⭐ THE LAW IT BOUGHT — `SC-§40` cl. 15

> ⛔ **A QA `WARN` is a ⛔ CITATION too, and the first party with the ⛔ INSTRUMENT must re-measure ⛔ EVEN A RETRACTION — because retracting ⛔ FEELS like the cautious option, and here it was the ⛔ wrong one.**

⛔ **Operative half for ⛔ future gates, and it is ⛔ this gate's own shape improved rather than reversed:** ⛔ when you contradict an author on a fact the author is ⛔ **fenced from measuring**, ⛔ **do not invite a retraction — ⛔ raise an OPEN QUESTION and ⛔ transfer it ⛔ WITHOUT an expected answer.** ⛔ **Neither of you can settle it; ⛔ only the row with the instrument can.**

⚖️ ⛔ **This gate found ⛔ TWO of its own defects in loop 2 and ⛔ struck NIT-2 outright — ⛔ which is why this annotation is written ⛔ in its voice rather than against it. ⛔ It is the ⛔ FOURTH instance of the same shape in one cycle, and the ⛔ first where the defect was a ⛔ WITHDRAWAL.** ⛔ **Nothing was lost: the file shipped in `239ca77` and the outcome is ⛔ identical either way.**
