# TASK-581 — [WR-27] ⭐ THE TESTABILITY SEAM — `ComposeAppendedInput` extracted and tested

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Law:** `WR-§6` (separator-composition clause, ruling `W4-R1`) · `SC-§32` · `SC-§33` · `SC-§15` · `SC-§18c` · `AS-§12g` · `AS-§6` RULING A-2 (Escape, CLOSED)
**Gate:** `qa/TASK-565.md` criterion (20) · **Compile + suite run:** TASK-566 · **Commit:** TASK-570
⛔ **No compile, no editor, no MCP, no PIE, no Git WRITE, no model, no inference.** The editor was up and was not touched.
⛔ **NO TOKEN FIGURE IS QUOTED, DERIVED OR REASONED FROM ANYWHERE IN THIS DOCUMENT** (`AS-§12g` / `WR-§6`). Chars/bytes only.

---

## 0. ONE-PARAGRAPH SUMMARY

TASK-561's inline whitespace composition is now a public plain `static` on `USiegeAssistantConsoleWidget`, pinned character-for-character to the spec, and `AppendToInput`'s call site is one read plus one call. **The move is behaviour-free: the body was relocated verbatim and the ONLY textual change is the two parameter names** — §2 puts the before and after side by side so that is evidence and not an assertion. One new test, `Siegebound.WarMap.ComposeAppendedInputWhitespaceRule`, asserts all six required cases plus an always-exactly-one-trailing-space invariant over six input shapes, **every string claim through `TestEqualSensitive`**, **on the shipped function and ⛔ never on a replica**. ⭐ **The suite total is `111`** (110 + 1). ⛔ **`Tests/SiegeAssistantZoneATest.cpp` is byte-untouched** and the four-refusal ladder is untouched. **Three comment corrections beyond the pure move are declared as departures in §7 — each one fixes a claim my own edit made FALSE.**

---

## 1. FILES TOUCHED — exactly the three I own

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h` | declaration + doc block added after `AppendToInput`; ⚠️ one existing doc line CORRECTED (§7 D-1) |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` | definition added above `AppendToInput`; the composition block REPLACED by one call |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp` | **EXTENDED** — test 23 appended; ⚠️ two header-comment claims CORRECTED (§7 D-2, D-3) |

⛔ **NOT touched, and this is the batch's headline evidence:** `Tests/SiegeAssistantZoneATest.cpp` · `SiegeAssistantComponent.{h,cpp}` · `SiegeAssistantSnapshot.{h,cpp}` · `SiegeAssistantVocabulary.{h,cpp}` · `SiegeAssistantGrammar.{h,cpp}` · `SiegeAssistantCommand.h` · `SiegePlayerController.{h,cpp}` (TASK-563) · `WarMapWidget.{h,cpp}` (TASK-579) · every file in TASK-578's list · any `.csv` · 🔒 `assistant_eval_holdout2.csv` · any `Content/` asset · `Build.cs`.

**READ-ONLY git, and it is labelled as such — ⛔ no write, no stage, no commit, no branch.** Prior tasks in this batch discharged the same proof obligation the same way (TASK-561 §1, TASK-564 §1); if the gate reads "no Git" as forbidding even a read, say so and the two commands below come out.

```
$ git status --porcelain -- Source/GitClaudeUnrealTest/Siegebound/Tests/
?? GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp
```

⇒ ⛔ **`Tests/SiegeAssistantZoneATest.cpp` does not appear: not modified, not staged, not renamed.** The `Tests/` tree still carries exactly ONE pending change — TASK-564's untracked file, which I extended in place. **I added no second Zone A assertion, in that file or in mine.**

```
$ git diff --stat -- .../SiegeAssistantConsoleWidget.cpp .../SiegeAssistantConsoleWidget.h
 .../Siegebound/SiegeAssistantConsoleWidget.cpp     | 173 +++++++++++++++++++
 .../Siegebound/SiegeAssistantConsoleWidget.h       | 186 +++++++++++++++++++++
 2 files changed, 359 insertions(+)
```

⚠️ **READ THAT FIGURE CORRECTLY — `359 insertions, 0 deletions` is a COMBINED total against `HEAD`, ⛔ not my diff.** TASK-561's `+255 / −0` is still uncommitted in the same two files, so `HEAD` contains **no `AppendToInput` at all**; my edit deletes lines TASK-561 added, which never existed in `HEAD` and therefore cannot show as deletions. **My own net contribution is ≈104 lines across the two files, the large majority of it the doc block and the extraction comment.** ⛔ **I am stating this rather than letting `0 deletions` read as "he deleted nothing" — I did delete the inline block, and §2 shows exactly what.**

---

## 2. ⛔⛔ THE PROOF OBLIGATION — BEFORE AND AFTER, SIDE BY SIDE (spec item (7))

⚖️ **An assertion that "it is just a move" is not evidence; the two blocks next to each other are.**

### BEFORE — `SiegeAssistantConsoleWidget.cpp`, inline inside `AppendToInput` (symbol `AppendToInput`; as-of-authoring lines 1219-1243)

```cpp
	const FString Existing = InputBox->GetText().ToString();

	const bool bNeedsLeadingSpace =
		!Existing.IsEmpty() && !FChar::IsWhitespace(Existing[Existing.Len() - 1]);

	FString Composed = Existing;
	if (bNeedsLeadingSpace)
	{
		Composed.AppendChar(TEXT(' '));
	}
	Composed.Append(Symbol);

	Composed.AppendChar(TEXT(' '));
```

### AFTER — (a) the pure static, symbol `USiegeAssistantConsoleWidget::ComposeAppendedInput`

```cpp
FString USiegeAssistantConsoleWidget::ComposeAppendedInput(const FString& ExistingText, const FString& TrimmedSymbol)
{
	const bool bNeedsLeadingSpace =
		!ExistingText.IsEmpty() && !FChar::IsWhitespace(ExistingText[ExistingText.Len() - 1]);

	FString Composed = ExistingText;
	if (bNeedsLeadingSpace)
	{
		Composed.AppendChar(TEXT(' '));
	}
	Composed.Append(TrimmedSymbol);

	Composed.AppendChar(TEXT(' '));

	return Composed;
}
```

### AFTER — (b) the call site, inside `AppendToInput`

```cpp
	const FString Existing = InputBox->GetText().ToString();
	const FString Composed = ComposeAppendedInput(Existing, Symbol);
```

### ⛔ THE CLAIM, STATED PLAINLY: **THE PRODUCED STRING IS IDENTICAL FOR EVERY INPUT.**

**The token-by-token difference between the two blocks is EXACTLY two identifier renames and one added `return`:**

| before | after | is this observable? |
|---|---|---|
| `Existing` (a local, initialised from `InputBox->GetText().ToString()`) | `ExistingText` (a `const FString&` parameter, bound to that same value at the one call site) | ⛔ **No.** Same value, same type family, read-only in both. |
| `Symbol` (a local, trimmed and proven non-empty above) | `TrimmedSymbol` (a `const FString&` parameter, bound to that same `Symbol`) | ⛔ **No.** |
| falls through to `InputBox->SetText(...)` | `return Composed;` → assigned to `const FString Composed` at the call site → `InputBox->SetText(...)` | ⛔ **No.** One copy/move of the same bytes. |

⛔ **NOT ONE OPERATOR, OPERAND, BRANCH OR ORDER OF EVALUATION MOVED.** Specifically, and these are the three the spec pinned because each has a defending comment at the source:

1. ✅ **The separator still decides on the LAST CHARACTER via `FChar::IsWhitespace`** — ⛔ still not `EndsWith(" ")`, so a tab still counts. Test 23 case (d) asserts the tab, the newline and the carriage return.
2. ✅ **`!ExistingText.IsEmpty()` is still FIRST in the `&&`** — ⛔ the short-circuit is still what makes `ExistingText[ExistingText.Len() - 1]` safe. Test 23 case (a) is the input that would fault if it were ever reordered.
3. ✅ **There is still ALWAYS exactly one trailing space** — the unconditional `AppendChar(TEXT(' '))` is unchanged and unconditional. Test 23's invariant loop asserts it over six input shapes at once.

⛔ **NOTHING WAS "TIDIED" ON THE WAY THROUGH.** No `MoveTemp`, no `Reserve`, no early-out, no `TrimEnd`, no `FStringView`, no ternary. The temptation was there and it is refused on record: **a refactor that changes the composed string is strictly worse than no refactor at all.**

### ⛔ WHAT STAYED BEHIND, AND IT IS THE OTHER HALF OF "MOVE NOTHING ELSE"

**The four-refusal ladder is INLINE, UNTOUCHED, IN ORDER** — verified line by line after the edit:

| # | refusal | verbosity | still there? |
|---|---|---|---|
| 1 | empty / whitespace-only insert (post-`TrimStartAndEndInline`) | `Warning` | ✅ |
| 2 | `!bConsoleEnabled` | `Log` | ✅ |
| 3 | `!bConsoleOpen` | `Log` | ✅ |
| 4 | `InputBox == nullptr`, behind the one-shot `bWarnedNoInputBoxForAppend` | `Warning` | ✅ |

⛔ **`bWarnedNoInputBox` is STILL NOT REUSED.** ✅ **TASK-564's `Siegebound.WarMap.AppendToInputRefusesAndNeverOpensTheConsole` already tests this ladder green and I did not disturb it.** ✅ **`SetText` THEN `FocusInputBox`, in that order, unchanged.** ✅ **The `UE_LOG` still reports `Symbol.Len()` and `Composed.Len()` — LENGTHS, ⛔ never the player's text.**

⛔ **THE APPEND-AT-END / CARET DECISION WAS NOT TOUCHED** (spec item (5)). I moved WHERE the composition lives, ⛔ not WHAT it composes and ⛔ not where the result lands. `qa/TASK-565.md` criterion (10)(b) still owns that verdict on TASK-561's three engine legs, and its evidence is exactly where it was.

---

## 3. ⛔ THE PINNED SIGNATURE — checked against the pin character-for-character

**Pinned:** `static FString USiegeAssistantConsoleWidget::ComposeAppendedInput(const FString& ExistingText, const FString& TrimmedSymbol);`

**Shipped** (`SiegeAssistantConsoleWidget.h`, symbol `ComposeAppendedInput`):

```cpp
	static FString ComposeAppendedInput(const FString& ExistingText, const FString& TrimmedSymbol);
```

| requirement | status | how it was checked |
|---|---|---|
| `public` | ✅ | `public:` at `:292`, `protected:` at `:640`; the declaration is at `:601` — **between them.** Checked by grepping the access specifiers, ⛔ not by eye. |
| plain `static`, ⛔ NOT a `UFUNCTION` | ✅ | `grep -B3 "static FString ComposeAppendedInput" … \| grep -c "UFUNCTION"` → **0**. The three lines above it are the doc block's close. |
| exactly TWO parameters | ✅ | both `const FString&`, both required. |
| ⛔ NONE defaulted — ⚠️ **the `bool bAddTrailingSpace = true` trap** | ✅ **NOT ADDED, and the header says in terms why a third parameter is forbidden on this signature.** | §4's default-argument sweep. |
| the `static` keyword is NOT repeated at the definition | ✅ | C++ forbids it out-of-class; the definition reads `FString USiegeAssistantConsoleWidget::ComposeAppendedInput(...)`. **This is the ONLY respect in which the definition's text differs from the pin, and it is a language rule, not a choice.** |
| precondition DOCUMENTED, ⛔ not defended in code | ✅ | header doc: *"TrimmedSymbol IS ALREADY TRIMMED AND NON-EMPTY"*, with the reason — a second guard would be `§19`'s duplicated authority. **The function contains ZERO guards.** |

**Same-module reachability, checked rather than assumed:** the class is `GITCLAUDEUNREALTEST_API` (`:288`) and `Tests/` is a subfolder of the same game module, so a public plain static is callable from the test with no export concern. **Shipped precedent in this exact file: `CreateAndAddToViewport` is also a plain static and is called from `SiegePlayerController.cpp`.**

---

## 4. ⛔ `SC-§33` — THE TRAILING-DEFAULT LAW, DISCHARGED WITH PASTED COMMANDS AND RAW COUNTS

`SC-§33`'s SCOPE clause: *"this binds a DEFAULTED parameter added to a function that ALREADY HAS CALL SITES… it does not bind a brand-new function."* **Both legs are proven mechanically below.**

### (a) ZERO DEFAULT ARGUMENTS WERE ADDED — the file's default-argument set is unchanged

```
CMD:  grep -nE "^\s+(TSubclassOf|int32|float|bool|const |FString|FText|APlayerController)[^;]*=\s*[^;]*[,)]" \
        Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.h

RAW HITS: 2
  637:		TSubclassOf<USiegeAssistantConsoleWidget> ConsoleClass = nullptr,
  638:		int32 ZOrder = 5);
```

⇒ **The same two defaults, on the same one function (`CreateAndAddToViewport`), with the same two values, before and after.** TASK-561 ran this identical expression and got the identical two hits at `:578` / `:579`; the numbers moved only because I inserted 59 lines above them (`SC-§18c` — **I located by symbol; line numbers are reported as evidence, ⛔ never used as anchors**).

**`CreateAndAddToViewport`'s call sites are unchanged from TASK-561's enumeration and I re-state the one that matters:** `SiegePlayerController.cpp` calls it with **both defaults omitted**, which is the shipped v1 state — (ii) **deliberately left at the default**, and (iii) **not mine to edit** in any case: that file is TASK-563's. **Nothing about that signature moved in this task.**

### (b) THE NEW FUNCTION HAD ZERO EXISTING CALL SITES — the `SC-§33` exemption, proven

```
CMD:  grep -rn "ComposeAppendedInput" Source/
RAW HITS: 30
```

| hits | where | classification |
|---|---|---|
| **1** | `SiegeAssistantConsoleWidget.h:601` | the **DECLARATION** — new, mine |
| **2** | `SiegeAssistantConsoleWidget.h:478`, `:480` | doc prose naming it — mine |
| **1** | `SiegeAssistantConsoleWidget.cpp:1180` | the **DEFINITION** — new, mine |
| **1** | `SiegeAssistantConsoleWidget.cpp:1285` | ⭐ **THE ONE AND ONLY SHIPPED CALL SITE** — `AppendToInput`, passing **BOTH** arguments explicitly, (i) **updated / authored by this task** |
| **3** | `SiegeAssistantConsoleWidget.cpp:1166`, `:1278`, `:1279` | comment prose naming it / naming the test — mine |
| **22** | `Tests/SiegeWarMapTest.cpp` (`:25`, `:88`, `:1712`, `:1729`, `:1730`, `:1733`, and 16 call expressions inside test 23) | **TEST calls + prose** — (i) **updated / authored by this task.** ⛔ Test calls are not shipped call sites and none can omit an argument: **there is no default to omit.** |

⇒ ⛔ **The function was born with ZERO pre-existing call sites** — exactly the `FSiegeAssistantRegionStatics::IsPointInRegion` precedent `SC-§33` names as owing nothing here. ✅ **A sweep that finds nothing to classify is a RESULT and is reported as one** (`SC-§22`'s closing rule).

### (c) `AppendToInput`'s OWN SIGNATURE DID NOT MOVE — swept anyway, because I edited its body

```
CMD:  grep -rn "AppendToInput" Source/ | wc -l
RAW HITS: 27
```

| hits | file | classification |
|---|---|---|
| 6 | `SiegeAssistantConsoleWidget.h` | declaration + doc prose — **signature byte-identical**, one param, no default |
| 3 | `SiegeAssistantConsoleWidget.cpp` | definition + comment prose — **body edited, signature untouched** |
| 4 | `SiegePlayerController.cpp` | ⭐ **THE ONE REAL SHIPPED CALLER** (`:4938` `Console->AppendToInput(PlaceSymbol.ToString())`) plus 3 comment lines — (iii) **OUTSIDE MY OWNERSHIP, owner TASK-563.** ⛔ **NO ACTION IS OWED: it passes the one required argument, there is no default to drop, and the signature it compiles against is unchanged.** |
| 1 | `SiegePlayerController.h` | comment naming the pinned contract — (iii) **TASK-563.** Still true, verbatim: *"AppendToInput NEVER opens the console and NEVER submits"*. |
| 13 | `Tests/SiegeWarMapTest.cpp` | test 22's calls + prose — mine, unchanged in behaviour |

⇒ ⛔ **Not one caller of anything needs an edit as a consequence of this task.**

---

## 5. ⭐ THE TESTS — `Siegebound.WarMap.ComposeAppendedInputWhitespaceRule`

**⛔ EXTENDED `Tests/SiegeWarMapTest.cpp`. ⛔ NO NEW FILE** (the RULING 4 precedent — a second war-map test file is a finding). `#if WITH_DEV_AUTOMATION_TESTS`, `EditorContext | EngineFilter`, class `FSiegeWarMapComposeAppendedInputTest`.

⭐ **THE POINT OF THE WHOLE TASK, VISIBLE IN THE TEST'S FIRST LINE: there is no `NewObject`, no `TStrongObjectPtr`, no widget, no Slate, no world and no CDO in it.** Test 22 needs a widget object to assert the refusals; **test 23 needs nothing at all** and calls `USiegeAssistantConsoleWidget::ComposeAppendedInput(...)` directly. **That difference IS the seam `WR-§6` now requires.**

⛔ **IT ASSERTS THE SHIPPED FUNCTION, ⛔ NEVER A REPLICA.** Every one of the 16 call expressions is a call into `SiegeAssistantConsoleWidget.cpp`. **TASK-564 was right to refuse the replica and this task is what makes the real assertion possible.**

| spec case | what is asserted | ⭐ the mutation it kills |
|---|---|---|
| **(a)** empty box | `"" + "mid"` ⇒ **`"mid "`**; and `"" + "ancient_ground_near"` ⇒ `"ancient_ground_near "` | ⛔ **a LEADING space on an empty box**, and it is also the input that **faults** if `!IsEmpty()` is ever moved out of first position in the `&&` — the out-of-bounds read, not a wrong answer |
| **(b)** ends in a non-space | `"send 10 footmen to" + "ancient_ground_near"` ⇒ **`"send 10 footmen to ancient_ground_near "`** — ⭐ TASK-561's own worked example, verbatim — **plus a separate negative assertion that the result does NOT contain `toancient_ground_near`** | ⛔ **the missing separator.** The parse fails and the entire war-room feature reads as broken while every individual piece looks correct |
| **(c)** already ends in a space | `"send 10 footmen to " + …` ⇒ the **same** string as (b); **plus** an assertion that the result contains ⛔ no `"  "` anywhere | ⛔ **the double space** |
| **(d)** ⭐ ends in a **TAB** | `"send 10 footmen to\t" + "mid"` ⇒ **`"send 10 footmen to\tmid "`** — ⛔ no separator. **Plus `\n` and `\r`**, so the claim is about WHITESPACE and not about one lucky character | ⭐⭐ **`EndsWith(" ")` substituted for `FChar::IsWhitespace`.** This is the leg a naive rewrite breaks **silently** — no other assertion in the file would notice |
| **(e)** ⭐ idempotence | **the second call is fed the FIRST call's OUTPUT**, which is what two map clicks actually do ⇒ **`"mid hero "`**, ⛔ never `"mid  hero"`. **A third click too** (`"mid hero own_castle "`) | ⛔ **rule 2 weakened to "a trailing space only when needed"** — which breaks rule 1 one click later. ⚠️ **Feeding it a hand-written `"mid "` would assert the rule against a string the TEST invented; feeding it the function's own output does not** |
| **(f)** interior text byte-preserved | `"  Send  10 FOOTMEN   to"` (leading whitespace + interior double space + mixed case) survives **byte for byte**; **plus the structural form** — the existing text is a byte-exact **PREFIX** of the result | ⛔ **any global whitespace normalisation, re-casing or head trim** (`§31`) |
| **invariant** | over **SIX** input shapes (empty · non-space · space · tab · whitespace-only · messy): result non-empty · **last char IS a space** · **the char before it is NOT a space** · **the symbol is present case-sensitively** | ⛔ *"always"* is the word rule 2 uses and no per-case assertion can say it. **The symbol-presence leg is what stops a function that merely returned `Existing + " "` from passing every other claim** |
| **opacity** | an unknown symbol (`NOT_A_PLACE_SYMBOL`) composes identically | ⛔ **a vocabulary branch creeping into this function** — a second, drifting copy of a vocabulary this file does not own |

⛔ **`TestEqualSensitive` ON EVERY STRING CLAIM — zero `TestEqual`-on-`FString` in the added code.** ⭐ **AND THE SAME TRAP HAS A SECOND MOUTH I AVOIDED DELIBERATELY: `FString::operator==` and `FString::StartsWith` are ALSO case-insensitive by default**, so the (f) prefix claim is made with **`Left()` + `TestEqualSensitive`**, ⛔ never with `==` or `StartsWith`. Both `Contains` calls pass **`ESearchCase::CaseSensitive` explicitly**.

### ⚠️ INCLUDES — CHECKED, ⛔ NOT ASSUMED (spec item (6)'s explicit instruction)

⛔ **NO INCLUDE WAS ADDED. NOT ONE.** Proven, not hoped:

- **`Siegebound/SiegeAssistantConsoleWidget.h`** — ✅ **already at `Tests/SiegeWarMapTest.cpp:13`**, because test 22 already calls `AppendToInput`. (The spec guessed it very likely did; **I confirmed it rather than relying on the guess.**)
- **`ESearchCase`** — lives in `Misc/CString.h`, reached transitively through `Misc/AutomationTest.h:11` → `Containers/UnrealString.h`. ⭐ **PROVEN BY PRECEDENT IN THE SAME DIRECTORY, not by inspection alone: `Tests/SiegeAssistantGrammarTest.cpp` uses `ESearchCase::CaseSensitive` at `:95`, `:107`, `:127`, `:230` while including ONLY `Misc/AutomationTest.h`, `Algo/Reverse.h` and three project headers.**
- **`TArray`** — `Containers/Array.h` already at `:5`.

### ⚠️ COMPILE RISK, STATED HONESTLY (`SC-§32`) — nothing here was compiled

**Every engine overload the new test leans on was read at the installed UE 5.8 headers on this machine, ⛔ not remembered:**

- `TestEqualSensitive(const TCHAR* What, const FString& Actual, const FString& Expected)` — `AutomationTest.h:2016`. ⚠️ **The `FStringView` sibling at `:2015` is also viable for `FString` arguments; `:2016` wins on exact match.** Same form the shipped tests at `:996` / `:1102` already use.
- `TestTrue(const TCHAR*, bool)` `:2603` · `TestFalse(const TCHAR*, bool)` `:2367`.
- `FString::Left(int32) const&` `UnrealString.h.inl:950` · `Right(int32) const&` `:990` · `Contains(const ElementType*, ESearchCase::Type, …)` `:1177`.
- ⭐ **ONE DELIBERATE REWRITE FOR COMPILE SAFETY, and I am naming it because it is the kind of thing a reviewer should see reasoned rather than guessed: I first wrote the "not a second space" claim as `TestNotEqual(…, Result[Len-2], TEXT(' '))`.** `TestNotEqual`'s template (`:2412`) would win — **but `TCHAR` converts implicitly to `float`, so the non-template `TestNotEqual(const TCHAR*, float, float, float)` at `:2427` is a VIABLE candidate.** The template wins on conversion rank, so it would have compiled — **but this file is compiled exactly once (TASK-566) and a bool has no candidate set to reason about at all.** ⇒ **It is now `TestFalse(…, Result[Len-2] == TEXT(' '))`.** The comparison itself is shipped idiom (`SiegeAssistantGrammar.cpp`, `SiegeAssistantComponent.cpp:124-125`). **The rewrite is recorded in a comment at the assertion.**
- **A compile error in the new code is MINE and I expect it back, ⛔ not TASK-566's to fix.**

---

## 6. ⭐ THE NEW SUITE TOTAL — `111` (spec item (8): the exact number, ⛔ not a delta)

**Counted mechanically across all eight test files** (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` / `IMPLEMENT_COMPLEX_AUTOMATION_TEST` / `IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST`):

| file | tests |
|---|---|
| `SiegeAssistantGrammarTest.cpp` | 12 |
| `SiegeAssistantGuardTest.cpp` | 9 |
| `SiegeAssistantSelectionTest.cpp` | 28 |
| `SiegeAssistantZoneATest.cpp` | **5** ⛔ **unchanged — byte-untouched** |
| `SiegeKeyboardLayoutTest.cpp` | 7 |
| `SiegeSettingsTest.cpp` | 7 |
| `SiegeStuckStaticsTest.cpp` | 20 |
| `SiegeWarMapTest.cpp` | **23** (was 22) |
| ⭐ **SUITE TOTAL** | ⭐ **111** |

✅ **This reconciles exactly with the batch's running figure:** the pre-batch baseline of **88** (TASK-551's recorded `88/88`) + TASK-564's **22** = **110**, + this task's **1** = **111**. ⚠️ **TASK-566: a total that is not `111` means a test failed to register, and that is itself a finding.** ⛔ **No other in-flight task adds a test — TASK-578 is comment-only and TASK-579's `names:` block lists `Tests/` as NOT touched, both verified on the board.**

---

## 7. ⚠️ DECLARED DEPARTURES (`SC-§15`) — three comment corrections beyond the pure move

⚖️ **Each one fixes a claim that MY OWN EDIT made false. Leaving a stale claim standing to keep a diff minimal is how a document rots (`SC-§34`'s lesson), and every one of these is a sentence a reader would act on.** ⛔ **All three are comment-only; zero emitted bytes.** **If the gate rules any of them out of scope, say so and I will revert that one — none is load-bearing.**

**D-1 — `SiegeAssistantConsoleWidget.h`, `AppendToInput`'s doc block.** It read *"THE WHITESPACE RULE — A DECISION, NOT A DETAIL, **AND TASK-564 TESTS EACH CASE BY NAME**"*. ⛔ **That was FALSE the day it shipped — TASK-564 could not, and proved it.** ⇒ **Corrected, with the original wording QUOTED rather than deleted** (the `WR-§2b` / `AS-§21.2` discipline: a corrected instruction whose original is erased teaches nothing), and the three rules re-pointed at `ComposeAppendedInput` as their owner. ⭐ **This sentence is the strongest single piece of evidence for `WR-§6`'s new law and it belongs in the file, not only in a handoff.**

**D-2 — `Tests/SiegeWarMapTest.cpp` header, the "WHAT THESE TESTS DO NOT COVER" list.** Its `THE WHITESPACE RULE` bullet described the rule as unreachable. ⛔ **That bullet is now false and it sits under a heading that says "STATED SO NOBODY MISTAKES GREEN FOR DONE" — a false entry there is worse than none.** ⇒ **Rewritten to record that the gap is CLOSED by TASK-581, ⛔ with TASK-564's original text quoted in full**, because the history is the lesson and the next reader should be able to see the mechanism that made it unassertable.

**D-3 — `Tests/SiegeWarMapTest.cpp` header, the subject line.** Added TASK-581's subject alongside TASK-564's, so the file's own header names what it now covers.

### ⛔ NOT A DEPARTURE, AND CHECKED RATHER THAN ASSUMED

⛔ **The extraction WAS behaviour-free**, so spec item (11)'s stop-and-report clause did not fire. ✅ **The composition was already a clean pure function of `(Existing, Symbol)` — it read no member, wrote no member, called nothing on the widget and touched no clock, no RNG and no world.** ⚖️ **That is why this task is cheap: TASK-561 wrote a pure fragment in an impure place, and the fix was to move it, ⛔ not to redesign it.**

---

## 8. ⛔ THE AIRLOCK, AS-§6, AND THE OTHER STANDING CHECKS

| check | result |
|---|---|
| ⭐ `Tests/SiegeAssistantZoneATest.cpp` byte-untouched | ✅ **Absent from `git status --porcelain -- Tests/`** (§1). **`ShippedZoneAChars = 5658` and `Siegebound.Assistant.ZoneA.MeasuredCharCount` are exactly where TASK-564 found them, and I added no second assertion.** |
| ⭐ structural reason the freeze is unaffected | ✅ `ZoneA.MeasuredCharCount`'s input surface is **`SiegeAssistantSnapshot.{h,cpp}` + `SiegeAssistantVocabulary.{h,cpp}`**. ⛔ **`SiegeAssistantConsoleWidget.{h,cpp}` is not in it** (`WR-§6` says so in law) ⇒ **the byte-freeze proof is structurally untouched by this edit, ⛔ not merely "probably fine".** |
| ⛔ prompt characters spent | ✅ **ZERO.** No Zone builder, no `TEXT()` prompt payload, no few-shot, no rule line, no `.csv` was opened. **This is a refactor of a UI string helper plus a test.** |
| ⛔ any token figure quoted or derived | ✅ **ZERO**, in the code and in this document (`AS-§12g`). Chars/bytes only. |
| ⛔ `AS-§6` RULING A-2 — `Escape` | ✅ `git diff -U0` on both console-widget files, `grep -c "^+.*Escape\|^+.*FReply\|^+.*Handled\|^+.*EKeys::"` → **0**. ⛔ **No preview handler edited, no Enhanced Input action added, no `FReply::Handled` introduced, and the token `Escape` appears zero times in anything I added.** **The four shipped close routes are unchanged and un-extended.** |
| ⛔ no proximity / NPC / range condition entered the file | ✅ `WR-§5` RULING 5 — *"the console still works anywhere"*. **The new static cannot reach a world, so it could not gate on one if it tried.** |
| ⛔ no length cap, no vocabulary branch added | ✅ `MaxUtteranceBytes` remains the single authority; the static moves an **opaque** string and test 23 asserts that it does. |
| 🔒 sealed holdout · `ReportFirstCapture`'s latch · the spike file | ✅ **untouched and unspent.** No inference, no `Capture()`, no eval. |

---

## 9. 📌 M8 DECLARATION (`WR-§8` — ⛔ NOT the last three batches' boilerplate)

**This task adds NO replicated property, NO new replicated class, NO new relevancy tier and NO RPC.** ⭐ **`ComposeAppendedInput` is the least networked thing this batch has produced: a `static` with no `this`, no `UWorld`, no authority concern and no gameplay state — it takes two strings and returns a third.** `USiegeAssistantConsoleWidget` remains client-local by construction and nothing about that moved. ⚠️ **The batch's two RPCs (`ServerRequestEnemyReveal` / `ClientReceiveEnemyReveal`) are TASK-563's, on the controller, and are ⛔ unreachable from this file.**

---

## 10. ⚠️ WHAT QA SHOULD SCRUTINISE (ranked)

1. ⭐⭐ **§2, the before/after.** This is the deliverable. **Read the two blocks against each other and confirm the ONLY differences are the two parameter names and the added `return`.** ⛔ **If you find any third difference, the behaviour-free claim is wrong and this comes back to me** — and per spec item (11), a refactor that changes the composed string is strictly worse than no refactor.
2. ⭐ **The three pinned details, at the source, ⛔ not from my table:** `FChar::IsWhitespace` on the last character (⛔ not `EndsWith`) · `!ExistingText.IsEmpty()` FIRST in the `&&` · the unconditional trailing `AppendChar(TEXT(' '))`.
3. ⭐ **The refusal ladder.** Re-read `AppendToInput` top to bottom. **Four refusals, in order, verbosities `Warning`/`Log`/`Log`/`Warning`, `bWarnedNoInputBoxForAppend` still separate from `bWarnedNoInputBox`.** ⛔ **If anything moved into or out of that ladder, fail it** — TASK-564's test 22 is already green on it.
4. ⛔ **`SC-§33`, and the gate owes its OWN re-run and its OWN pasted count** (that second leg is what makes the law enforceable). Expected: **2 default-argument hits, identical values, unchanged** · **30 `ComposeAppendedInput` hits, exactly ONE of them a shipped call site, passing both arguments** · **27 `AppendToInput` hits, signature unmoved.**
5. ⚠️ **§7's three declared departures.** They are comment-only and each fixes a claim my edit falsified. **If you read any as out of scope, say so and I will revert it.**
6. ⚠️ **The `TestNotEqual` → `TestFalse` rewrite in §5.** Confirm you agree that avoiding a viable-but-losing `float` overload is worth one line of comment, given TASK-566 is the batch's single compile.
7. ⚠️ **The read-only git usage in §1.** Two read commands, no writes. **If the gate reads the dispatch's "no Git" as absolute, the airlock evidence has to come from somewhere else and I would like the ruling on record** — it is the same route TASK-561 and TASK-564 used in this batch.
8. ⚠️ **`SC-§32`, honestly: nothing here was compiled or run.** Test 23 has never been observed to pass **or fail**. **The mutation I would most want watched is (d): change `FChar::IsWhitespace(…)` to `ExistingText.EndsWith(TEXT(" "))` and confirm ONLY the tab/newline/CR assertions go red.** That is the leg a naive rewrite breaks silently, and it is the cheapest proof this test is load-bearing rather than merely green.

---

## 11. ⚠️ NOT MINE, FLAGGED NOT FIXED

- ⛔ **The caret-vs-append verdict** stays with `qa/TASK-565.md` criterion (10)(b). **I did not touch the decision, the code that implements it, or the header block that declares it** — only the whitespace-rule block above it (§7 D-1).
- ⚠️ **TASK-564's other open items are untouched and still open:** the arena-extent pin refusal (its §5), the eighth-file judgement call (its §2), and the short-balance PIE row it flagged — **which the manager has already boarded as TASK-569 (q)**.
- 📌 **`WR-§9` row 12** (no place markers on a first open) is TASK-579's status line and TASK-580's real repair. **Nothing here touches it.**
