# TASK-1560 — gameplay-programmer handoff (HELP-PROSE-FOLLOWTHROUGH)

- **Row:** `TASKBOARD.md` `#### TASK-1560 ` (marker `TASK-1560-HELP-PROSE-FOLLOWTHROUGH`).
- **Status set:** `ready-for-qa`. Routing per the row: gate `TASK-1561`, then 5a + commit `TASK-1562`. No runtime criterion, so no 5b.
- **Summary:** 6 of 6 stale sites fixed, all of them comments except the one label literal. The guard is in: `TEXT("::")` and `TEXT("()")` were appended to `ForbiddenInPlayerProse`. **The census found 0 hits for both**, so there is no STOP. No new test was added, so `N` stays 566. The mutation arm `TASK-1562` owes is named in §4.
- **Spec (2) approval:** the orchestrator relayed Jonathan's answer on the `::`/`()` guard on 2026-09-28, "Yes, keep it (Recommended)". The manager records that on the row.

## 0. Start state and declared tooling (`SC-§71a`)

- **Blockers met:** G (`133012a`, TASK-1540) and H (`8ea87e4`) are committed. HEAD was `f1091b8`, and the tree was clean for these three files.
- **Start bytes equal G's committed bytes, and HEAD's too.** Each file's sha256 on disk equals `git show 133012a:<path> | sha256sum` and `git show HEAD:<path> | sha256sum` (table in §5).
- **Read-only git, declared, all with `--no-optional-locks`:**
  - `status --short` and `log --oneline -3`
  - `diff --stat 133012a HEAD -- <3 files>`, which was empty
  - `show 133012a:./<path>` and `show HEAD:./<path>`, piped to `sha256sum` and to a scratch copy
  - `diff -U0 133012a -- <3 files>` (§5)
  - No mutating git.
- **Byte tools:** `sha256sum`, `wc`, `tr`, `od`, `sed`, `grep` and `awk`.
- **Python scripts in my scratchpad.** They read the repo and write only to the scratchpad (`…\35e683bf-936e-4e1a-93dd-6b0127c2c1b8\scratchpad\`):
  - `census1560.py`: a lexer plus the `::`/`()` census.
  - `mkctl1560.py`: builds two mutated scratch copies for the census's negative controls.
  - `beside1560.py`: checks that each code name sits in a comment beside its string, and prints composed `Detail` text.
  - `fence1560.py`: the fence proof, comparing G's skeleton with the new one.
- **Edits:** `Edit` tool only.
- **Not done, as the row forbids:** no compile, no editor, no PIE and no asset. `CONVENTIONS.md` is untouched.
- **Line endings:** all three files are still LF only (0 `\r` bytes) with no BOM.

## 1. The six sites, old beside new (spec (1))

These are cited by text; line numbers are as of G and move. Every site is a comment except site 6, which is the label literal the row names.

| # | File, site (found by text) | OLD | NEW |
|---|---|---|---|
| 1 | `SiegeControlsHelpWidget.cpp`, file-level TASK-707 transfer block, the "⛔ NO TUNABLE'S VALUE IS RE-TYPED" paragraph | `704 §4 deliberately NAMES tunables instead of restating numbers (its U-5 / D-6, the M7.7 lesson) and the names are carried through unchanged.` | `704 §4 deliberately NAMES tunables instead of restating numbers (its U-5 / D-6, the M7.7 lesson). TASK-707 carried those names into the prose; since TASK-1541 (2026-09-27) the prose describes each tunable in plain words ("a set step", "the rally radius") and its code name sits in the comment beside the string.` The rest of the paragraph (the 30 gold sentence) is unchanged. |
| 2 | Same block, its opening paragraph under "TASK-707: THE `Detail` COLUMN…" | `Every detail string below is handoffs/TASK-704-programmer.md §4's prose for that row. 704 cited EVERY factual sentence at a file:line it had personally read; ⛔ nothing here was re-authored, re-derived or invented, and where I disagreed …` | `TASK-707 filled each original row's detail string from handoffs/TASK-704-programmer.md §4's prose for that row. Not every string below is still 704's words: TASK-823 wrote its three appended rows at source, and later rows rewrote others, among them TASK-821 (Cards.Discard, in place), TASK-870 (Interface.WarMap) and TASK-1541 (2026-09-27: the code names the prose printed, put into plain words). 704 cited EVERY factual sentence at a file:line it had personally read; ⛔ TASK-707's transfer re-authored, re-derived and invented nothing, and where I disagreed …` The tail is unchanged. |
| 3 | `SiegeControlsHelpWidget.h`, the `Detail` field's doc comment, "⛔ NO TUNABLE'S VALUE IS RE-TYPED HERE" | `TASK-704 §4 deliberately NAMES tunables (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) instead of restating numbers, and this task keeps that (704 U-5; the M7.7 "in 400"/AoERadius-700 lesson).` | `TASK-704 §4 deliberately NAMES tunables (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) instead of restating numbers (704 U-5; the M7.7 "in 400"/AoERadius-700 lesson). TASK-707 kept those names in the prose; since TASK-1541 (2026-09-27) the prose describes each one in plain words ("a set step", "once per melee cooldown", "a fixed reveal fee") and the code name sits in the C++ comment beside its string, still with no number typed.` The 30 gold sentence is unchanged. |
| 4 | `Tests/SiegeControlsHelpTest.cpp`, `FSiegeControlsHelpDiscardAllLayoutTest`, the comment above the `{ActionId}` token assertion | `⚠️ A Body.Contains("D") here would be VACUOUS: the prose also names DiscardAllCost, so that letter is present whatever the implementation does.` | `… would be VACUOUS: the prose carries a capital D of its own ("Dumping a single dead card"), so that letter is present whatever the implementation does.` The conclusion is unchanged. The rest of the comment was rewrapped (+1 line) with its words unchanged. |
| 5 | Same test, section header (e) | `// ── (e) ⛔ THE FEE IS NAMED, ⛔ NEVER TYPED (`HELP-§2`'s M7.7 rule) ──…` | `// ── (e) ⛔ THE FEE IS PUT IN WORDS, ⛔ NEVER TYPED (`HELP-§2`'s M7.7 rule) ──…` The dash run was shortened by 7 so the line keeps its width. |
| 6 | Same file, `FSiegeControlsHelpTowerRowsTest`, the one-liner loop's assertion label (from `qa/TASK-1546.md` §9 W1) | `TEXT("⛔ Row '%s' one-liner types NO number - the tunables are NAMED")` | `TEXT("⛔ Row '%s' one-liner types NO number - the tunables are described in plain words")` The `%s` and the literal's place in the `Printf`/`TestFalse` call are unchanged. |

**Truth of the new words, checked at source (`SC-§101`).**

- **Every quoted phrase exists in the composed prose.** I concatenated each row's `TEXT()` literals after lexing:
  - "a set step" appears in `PickMode.Resize` and `Cards.PlacementResize`.
  - "the rally radius" appears in `Hero.Rally`.
  - "once per melee cooldown" appears in `Hero.Attack`.
  - "a fixed reveal fee" appears in `Interface.WarMapReveal`.
  - "Dumping a single dead card" appears in `Cards.Discard`. It spans the `"…flat. Dumping a "` / `"single dead card costs…"` literal join, so it is contiguous only in the composed text, which is what `Body` holds.
- **"the code name sits in the comment beside the string" (sites 1 and 3):**
  - `beside1560.py` checked all 62 code names from `TASK-1541`'s census table, across the 19 rows it changed. Each one is present in a `//` comment inside its own row's block, and absent from that row's composed `Detail`. **0 failures.** (The table has 63 hits; the borderline bare argument list `(true, 0)` is not a name, so it was not checked.)
  - Site 3's three examples each sit in their row's comment: `GroupRadiusWheelStep` in the `PickMode.Resize` block, `MeleeCooldown` in `Hero.Attack` and `EnemyRevealCost` in `Interface.WarMapReveal`.
- **Site 2's history:**
  - `TASK-821` shows in the `Cards.Discard` block ("R-09. ⭐⭐ REWRITTEN IN PLACE BY TASK-821").
  - `TASK-870` shows in the `Interface.WarMap` block ("⛔⛔ TASK-870 — THE REPAIR…").
  - `TASK-823`'s three rows are named by the block above ("Rows 25-27 are TASK-823's own prose").
  - "among them" is deliberate. I did not audit every task that ever touched a `Detail` string: `TASK-1432`, `TASK-1433` and `TASK-568` also appear in the registry's comments, so no complete list is claimed.
  - "each original row" is also deliberate. The old sentence said "every detail string", and that was false for `TASK-823`'s rows, which `TASK-707` never filled.
- **Site 6 holds for the rows its loop walks** (`Cards.StackUpgrade`, `Cards.PlacementResize`, `Interface.MapMarks`). Their pages describe the tunables in plain words: "a set maximum multiple" and "a set factor"; "a set step" and "a set floor and a set ceiling"; "a limited number".

## 2. The guard (spec (2))

**Census first, measured at my instant on G's bytes** (`census1560.py` on `SiegeControlsHelpWidget.cpp`, sha256 `a6bf281f…`):

| Scope | Count | Contains `::` | Contains `()` |
|---|---|---|---|
| Every string literal in code state in the file (registry, chrome, `UE_LOG`, object names; no raw strings exist) | 610 | 0 | 0 |
| Composed `Detail` per row: all `TEXT()` literals of each `Row.Detail = FText::FromString(FString(…))` concatenated and unescaped, which is what `ComposeDetailForDisplay` hands the test | 27 rows, 27263 chars | 0 | 0 |
| Literal joins inside a `Detail` (the last char of one literal plus the first of the next) | every join | 0 | 0 |
| `AddRow` literals (id, display name, one-liner) | 27 calls | 0 | 0 |
| `SiegeControlsHelpText` chrome constants (title, hints, Close, Back, related header, chips, TODO string, category headers) | 23 | 0 | 0 |

- **Negative controls (the census can see a hit).** `mkctl1560.py` made two mutated scratch copies; the repo file was never mutated.
  - **Control 1:** "fire() A::B" injected into the `Hero.Attack` literal `"stacks. No friendly fire.\n\n"`. Seen: (a) = 1 literal, and (b) = 2 hits (`::`, `()`) on `Hero.Attack`.
  - **Control 2:** a `::` split across two literals (`"…melee cooldown:"` + `": "`). Seen: (b) = 1 and (c) = 1 join. (a) = 0, as expected, since no single literal holds it.
- **Result: 0 = `TASK-1541`'s 0.** The census has no hit, so there is no STOP.
- **Only the registry's `Detail` is read by the guard.** No other file writes it: `grep` finds `.Detail =` outside the widget only in the test file's own synthetic or empty fixture rows, which `ForbiddenInPlayerProse`'s loop never walks, because it walks `GetActions()`.
- **The edit:** `TEXT("**"), TEXT("`"), TEXT("§")` → `TEXT("**"), TEXT("`"), TEXT("§"), TEXT("::"), TEXT("()")`.
- **N stays 566, and no new test was needed.**
  - `ForbiddenInPlayerProse` is a `const TCHAR*[]` read only by the range-`for` in `EveryRowHasAuthoredDetail`.
  - Two more entries give two more `TestFalse` calls per row, inside the existing test.
  - Nothing sizes the array: there is no `UE_ARRAY_COUNT` and no `static_assert`.
  - No `IMPLEMENT_SIMPLE_AUTOMATION_TEST` was added.

## 3. The fence

`fence1560.py` compared G's bytes with the new bytes. It removed comments, masked literals and collapsed whitespace.

| File | Code skeleton | Literals | Comments |
|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | **identical** | 610 = 610, 0 changed | 2062 → 2068 `//` lines (+6: site 2 went from 4 to 8 lines, site 1 from 2 to 4) |
| `SiegeControlsHelpWidget.h` | **identical** | 47 = 47, 0 changed | 189 = 189 comments (the doc block is one `/** */`); +255 comment bytes |
| `Tests/SiegeControlsHelpTest.cpp` | differs **only** in the `ForbiddenInPlayerProse` initializer (8 → 10 `TEXT()` entries) | 406 → 408: **insert** `"::"`, `"()"`; **replace** exactly one, the site-6 label | 504 → 505 (+1 from site 4's rewrap) |

- **No player-facing literal changed:** the widget's 610 literals are byte-identical.
- **No other executable byte changed.**
- **No other file was touched**, apart from this handoff and my row's `status:` line.

## 4. The mutation arm `TASK-1562` owes (spec (3))

- **String:** `Hero.Jump`'s `Detail` literal `TEXT("default handling (which would delete your hero outright) and goes down the same ")` (find it by that text).
- **Inject this exact fragment at the start of the literal's bytes:** `AHeroCharacter::FellOutOfWorld() ` (with one trailing space). The mutated literal is then:
  `TEXT("AHeroCharacter::FellOutOfWorld() default handling (which would delete your hero outright) and goes down the same ")`
  - This re-creates the pre-`TASK-1541` defect class on the very row it came from.
  - One fragment carries both needles, and the test emits one labelled `TestFalse` per list entry, so each new entry is seen red under its own label.
- **Expected when compiled and run:**
  - Exactly **one test fails**: `Siegebound.ControlsHelp.EveryRowHasAuthoredDetail`.
  - It fails with exactly **two** errors:
    - `Row 'Hero.Jump' detail carries no developer-only fragment '::' (T1/T2: …)`
    - `Row 'Hero.Jump' detail carries no developer-only fragment '()' (T1/T2: …)`
  - The other 565 tests stay green, for these reasons:
    - `Hero.Jump` has no `{ActionId}` token, so the (c) layout-identity check and the token scans still hold.
    - The fragment adds no digit and no `{`.
    - The page grows, so the length check holds.
    - The pairwise distinct-page checks still differ.
    - No pin reads `Hero.Jump` prose.
- **Revert:** the file returns to after-sha256 `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990`, then the suite runs 566/566.
- **Reading the result:** a red that shows only one of the two labels means that list entry is dead.

## 5. Hashes and the diff against G

| File | Before = G `133012a` = HEAD (sha256, bytes) | After (sha256, bytes, lines) |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` | `a6bf281fd83d4db65461fe8a831e93b632faa1aca9af80ef6785b6cfbc235d8b` (278984) | `6f511582cee9929893daf85d1b51ff98389ac70e62b8f3ab84308db400ddc990` (279507, 4503) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h` | `cc16d2caade4dab2a4676724ba9a2596b1f455d115560afe35f259941012d174` (80750) | `249277ed46601d16bb8f717ead2d76e7486efb76f60983662e5f6968917d2720` (81005, 1400) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp` | `525b5886779d1e346162f3d1e04e226ef69cfffab6ed6714705341385fc591d7` (150481) | `33fd21e74d9378a4b8b28a39d79fd2b72b7e37bf1b8d031988dc3138f2a58c5f` (150550, 2625) |

`git --no-optional-locks diff -U0 133012a -- <the 3 files>` (read-only, declared; the CRLF warnings git printed are omitted):

```diff
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp
@@ -307,3 +307,7 @@ namespace
-//  Every detail string below is handoffs/TASK-704-programmer.md §4's prose for that row. 704
-//  cited EVERY factual sentence at a file:line it had personally read; ⛔ nothing here was
-//  re-authored, re-derived or invented, and where I disagreed with a sentence I FLAGGED it in
+//  TASK-707 filled each original row's detail string from handoffs/TASK-704-programmer.md
+//  §4's prose for that row. Not every string below is still 704's words: TASK-823 wrote its
+//  three appended rows at source, and later rows rewrote others, among them TASK-821
+//  (Cards.Discard, in place), TASK-870 (Interface.WarMap) and TASK-1541 (2026-09-27: the code
+//  names the prose printed, put into plain words). 704 cited EVERY factual sentence at a
+//  file:line it had personally read; ⛔ TASK-707's transfer re-authored, re-derived and
+//  invented nothing, and where I disagreed with a sentence I FLAGGED it in
@@ -337 +341,3 @@ namespace
-//     numbers (its U-5 / D-6, the M7.7 lesson) and the names are carried through unchanged.
+//     numbers (its U-5 / D-6, the M7.7 lesson). TASK-707 carried those names into the prose;
+//     since TASK-1541 (2026-09-27) the prose describes each tunable in plain words ("a set
+//     step", "the rally radius") and its code name sits in the comment beside the string.
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h
@@ -163,2 +163,5 @@ struct FSiegeControlsHelpAction
-	 *  (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) instead of restating numbers,
-	 *  and this task keeps that (704 U-5; the M7.7 "in 400"/AoERadius-700 lesson). The ONE
+	 *  (GroupRadiusWheelStep, MeleeCooldown, EnemyRevealCost …) instead of restating numbers
+	 *  (704 U-5; the M7.7 "in 400"/AoERadius-700 lesson). TASK-707 kept those names in the
+	 *  prose; since TASK-1541 (2026-09-27) the prose describes each one in plain words ("a set
+	 *  step", "once per melee cooldown", "a fixed reveal fee") and the code name sits in the
+	 *  C++ comment beside its string, still with no number typed. The ONE
--- a/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
+++ b/GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp
@@ -1114 +1114 @@ bool FSiegeControlsHelpAuthoredDetailTest::RunTest(const FString& Parameters)
-		TEXT("**"), TEXT("`"), TEXT("§")
+		TEXT("**"), TEXT("`"), TEXT("§"), TEXT("::"), TEXT("()")
@@ -1802,3 +1802,4 @@ bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
-	// would be VACUOUS: the prose also names DiscardAllCost, so that letter is present whatever
-	// the implementation does. The claim that can actually fail is that the RAW prose names the
-	// key as a {ActionId} token — i.e. that no letter was typed into the sentence at all.
+	// would be VACUOUS: the prose carries a capital D of its own ("Dumping a single dead card"),
+	// so that letter is present whatever the implementation does. The claim that can actually
+	// fail is that the RAW prose names the key as a {ActionId} token — i.e. that no letter was
+	// typed into the sentence at all.
@@ -1809 +1810 @@ bool FSiegeControlsHelpDiscardAllLayoutTest::RunTest(const FString& Parameters)
-	// ── (e) ⛔ THE FEE IS NAMED, ⛔ NEVER TYPED (`HELP-§2`'s M7.7 rule) ──────────────────
+	// ── (e) ⛔ THE FEE IS PUT IN WORDS, ⛔ NEVER TYPED (`HELP-§2`'s M7.7 rule) ───────────
@@ -2058 +2059 @@ bool FSiegeControlsHelpTowerRowsTest::RunTest(const FString& Parameters)
-		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' one-liner types NO number - the tunables are NAMED"), Expected.ActionId),
+		TestFalse(*FString::Printf(TEXT("⛔ Row '%s' one-liner types NO number - the tunables are described in plain words"), Expected.ActionId),
```

## 6. Seen outside my fence, NOT edited (`SC-§53`), for the manager to board or rule

1. **`Tests/SiegeControlsHelpTest.cpp`: the comment above `ForbiddenInPlayerProse`** ("Every one of these belongs in a C++ comment beside the string (TASK-704 §4's citations, rule T1) or is markdown … (rule T2)").
   - My two entries are neither T1 nor T2. They are the T5 class ("a C++ fragment … moves into the comment beside the string"; T5 is the widget's rule list).
   - The first half of that comment still holds for them; its parenthetical list is now incomplete.
   - The same goes for the docstring's "(rules T1 and T2)" above the test, and for the executable label at the `TestFalse` in the loop, "(T1/T2: citations and markup live in comments)". When `::` or `()` goes red, its message will say T1/T2.
   - All three are outside the row's `names:` fence, and the label is an executable literal that (4) forbids me. **A one-row follow-up could add "and T5 code names".**
2. **`SiegeControlsHelpWidget.h`, the same `Detail` doc block, first sentence:** "FILLED BY TASK-707 from handoffs/TASK-704-programmer.md §4 — ⛔ transferred, ⛔ never re-authored."
   - This is the same staleness as site 2 (TASK-823's rows; 821 / 870 / 1541 rewrites).
   - It is not one of the six named sites, and "Change no other claim" held me off it.
3. **Candidates, NOT measured:**
   - The `.h`'s `OneLine` field doc, "(TASK-704 §4, verbatim)". Rows 25-27 are TASK-823's one-liners.
   - The `.cpp` header line "THE ACTION REGISTRY — handoffs/TASK-704-programmer.md §4, in category order."
   - The test label "…neither does its detail page - the fee is read from DiscardAllCost, never typed". The game does read the fee from that property, so I judge this label true as written. It names a code symbol in a test label, which is not player prose.

## What QA (`TASK-1561`) should scrutinize

1. **Same claims, new words (`SC-§101`):**
   - Site 2's reframing: "each original row", "among them", and "TASK-707's transfer re-authored … nothing" in place of "nothing here was re-authored". The old wording was false today, and the new wording scopes the claim to the transfer act.
   - Site 6: "described in plain words", in place of "NAMED".
2. **The fence proof (§3):** the widget's literals are byte-identical, and the test has exactly three literal ops (2 inserts, 1 replace).
3. **The census's controls (§2):** that the census could see a hit, including one formed across a literal join.
4. **The arm (§4):** that exactly the two labelled errors are expected, and nothing else goes red.

## Not examined / limitations

- **Nothing was compiled or run.**
  - "N stays 566" is argued from the code shape (no new `IMPLEMENT_…`); it was not run.
  - "The guard is green on today's prose" rests on the census; the suite never ran it. `TASK-1562`'s 5a is where both are first measured.
- **The census covers what the guard reads**, the registry's `Detail` text (plus every other literal in the widget file). It does not cover:
  - key-chip text the engine composes from `FKey::GetDisplayName`, which the guard never sees, because the test reads raw `Detail` with tokens unresolved;
  - player-facing strings in any other widget.
- **The lexer handles** `//`, `/* */`, strings with escapes and char literals, and it stops on any raw string (the file has none). Its literal count (610) matches `TASK-1541`'s independent count.
- **The truth of the sentences around my six sites was not re-examined**, for example the "ONE number … 30 gold" sentence. Neither were the numeric `file:line` citations in those blocks. `TASK-1480` (a)'s file-level declaration already marks those citations ⛔ UNVERIFIED.
- **Site 2's history list is not exhaustive**, and says so ("among them").
- **Line numbers in this handoff are as of G or of my after-bytes, and they move.** Every site is cited by its text.
