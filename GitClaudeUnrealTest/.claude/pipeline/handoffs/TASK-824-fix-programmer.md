# TASK-824 — BUILD-FAILURE FIX (gameplay-programmer)

**Date:** 2026-09-03
**Trigger:** routing rule 6 — `TASK-824`'s compile returned `Result: Failed (OtherCompilationError)` with **15 errors**. Counts as a QA loop.
**Build log:** `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\TASK-824-build.log`
**File touched:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Tests\SiegePlacementTest.cpp` — **the only file changed. 3 lines.**

Both defects are **non-semantic**: a comment terminator and a format-string message. **No test's meaning, assertion value, expected value or tolerance changed.** If QA finds otherwise, the diagnosis is wrong and I want to hear it.

---

## Defect 1 — comment terminator (`TASK-735`'s diff) — 14 of 15 errors

`:139` carried the prose `TCHAR*/FStringView/…` **inside a `/** … */` block**. The `*/` in `TCHAR*/` **closed the comment four lines early**, so `:140`–`:142` parsed as code and the `DiffersFrom()` helper at `:143` was never defined.

The 14 errors, all one root cause:

| Line | Error | Cause |
|---|---|---|
| 139,67 | `C2059` syntax error: `'/'` | comment closed; `/FStringView/…` is now code |
| 140,17 | `C2059` syntax error: `')'` | ditto |
| 140,20 | `C3873` `U+21d2` not allowed as first char of identifier | the `⇒` in prose |
| 142,3 | `C4138` `'*/'` found outside of comment | the **real** closer, now orphaned |
| 144,2 | `C2143` missing `';'` before `'{'` | `DiffersFrom`'s body |
| 144,2 | `C2447` `'{'`: missing function header | ditto |
| 276, 278, 326, 454, 463, 2460, 2545, 2658 | `C3861` `'DiffersFrom'` not found ×8 | the helper never got defined |

**Before (`:139`–`:140`):**
```
 *  `Misc/AutomationTest.h:2004-2012` — only TCHAR*/FStringView/FString/FUtf8StringView/
 *  FText/FName). ⇒ every "these two numbers must DIFFER" claim goes through this, so the
```

**After:**
```
 *  `Misc/AutomationTest.h:2004-2012` — only TCHAR*, FStringView, FString, FUtf8StringView,
 *  FText, FName). ⇒ every "these two numbers must DIFFER" claim goes through this, so the
```

**Why commas and not one space.** The minimum edit is inserting a space (`TCHAR* /`). I chose comma separators instead because they remove the `*`-adjacent-`/` sequence **structurally** — `TCHAR* /` is one whitespace slip away from reintroducing the identical defect, and this comment is exactly the kind that gets reflowed. The meaning is preserved verbatim: it still enumerates the six string overloads `TestNotEqual` has in UE 5.8. Line widths after the edit are **94** and **93**, inside the file's existing 94-column maximum (`:133`), so nothing reflows.

**`DiffersFrom`'s body, its signature, and all 8 call sites are byte-identical.** I verified every call site has `using namespace SiegePlacementTestFixture;` in scope within its enclosing `RunTest`, so the terminator fix alone clears all 8 `C3861`s — there is no second scope problem hiding behind them:

| Call | Enclosing test | `using` in scope via |
|---|---|---|
| 276, 278 | `FSiegePlacementFootprintRadiusTracksTheMeshAndIsNeverAConstantTest` | `:259` |
| 326 | `FSiegePlacementFootprintUsesScaledBoundsNeverLocalBoundsTest` | `:302` |
| 454, 463 | `FSiegePlacementBuildingClearanceComposesAsMaxNotSumTest` | `:438` |
| 2460, 2545 | `FSiegePlacementWheelStepsOneNotchAndClampsBothEndsTest` | `:2449` |
| 2658 | `FSiegePlacementWheelScalesXAndYOnlyTest` | `:2596` |

⚠️ The same prose appears in `handoffs/TASK-735-programmer.md` (~`:132`). **Left untouched** — it is a document, not a translation unit, and per the dispatch it is harmless there.

---

## Defect 2 — format string (`TASK-815`'s diff) — 1 error, independent

`:2609,29` — `C7595: call to immediate function is not a constant expression`. `FString::Printf` was passed `Sample` but the literal held **no `%` specifier**. UE 5.8's format-string sanitiser (`TCheckedFormatStringPrivate`) is `consteval`, so this is a **hard error, not a warning**.

**Before:**
```cpp
TestEqual(FString::Printf(TEXT("(a) …and Y carries the SAME factor (⛔ never an oblong the ghost's facing could change)"), Sample),
    static_cast<float>(Vector.Y), Sample, Exact);
```
**After:**
```cpp
TestEqual(FString::Printf(TEXT("(a) …and Y carries the SAME factor at scale %.2f (⛔ never an oblong the ghost's facing could change)"), Sample),
    static_cast<float>(Vector.Y), Sample, Exact);
```

### Why add the specifier rather than drop the argument — the reasoning QA asked for

Both fixes compile. They are **not** equivalent, and the siblings decide it:

- `:2606` — `"(a) ⛔ Z is exactly 1 at scale %.2f — …"`, passes `Sample`.
- `:2608` — `"(a) X carries the factor at scale %.2f"`, passes `Sample`.
- `:2609` — passes `Sample`, specifier missing.

All three sit inside `for (const float Sample : { 1.f, 1.25f, 1.5f, 3.f })`. **Both siblings report which scale they were at; the Z and X assertions can name their failing iteration and the Y one was plainly written to do the same** — the argument is still there, only the specifier was lost. Dropping `Sample` would compile but make all four Y-iterations emit an **identical** message, so a failure at ×3 would be indistinguishable from one at ×1.25 in a loop whose entire point is that the factor varies. That is a silent loss of diagnostic power in exactly the assertion most likely to catch an oblong-footprint bug.

I placed the specifier to mirror `:2608`'s wording (`at scale %.2f`) so the three messages read as one family.

**No outcome change:** `TestEqual`'s first parameter is the `What` **message only**. Actual (`Vector.Y`), expected (`Sample`) and tolerance (`Exact`) are untouched, so the test passes and fails on precisely the same conditions as before.

---

## Third problems: none found

I swept the whole file rather than trusting the log's line numbers:

1. **Comment terminators** — scanned every doc-block continuation line for a `*/` with trailing content. **Line 139 was the only one.** The other `*/` hits in the file are well-formed inline parameter comments (`/*bIsBuilding=*/ true`, `/*Gold=*/ 0`) and two string literals `TEXT("*/")` at `:963`/`:1675`, all legitimate.
2. **`Printf` parity** — checked every `Printf(TEXT(…))` in the file for args-without-specifier **and** specifier-without-args. **Zero mismatches remain.** Corroborated by the log itself: compilation reached `:2658` after erroring at `:2609`, so the whole TU was parsed and any second `C7595` would have been reported.
3. **`DiffersFrom`** — defined exactly once (`:143`), 8 call sites, matching the 8 `C3861`s one-for-one.

## Scope fence — held

- **Only** `Tests/SiegePlacementTest.cpp`. No other file in `Source/` touched.
- **3 lines changed** (139, 140, 2609). No refactor, no tidying, no test "improvements".
- **Neither edit is in `TASK-819`'s test region** — `:139` is in the `SiegePlacementTestFixture` namespace header, `:2609` is in `FSiegePlacementWheelScalesXAndYOnlyTest` (`TASK-815`'s). The two in-flight reviews (`TASK-822`, `TASK-848`) read regions this diff does not enter.
- **No compile, no editor, no MCP, no Git.** The file is currently untracked (`??`), which is why `git diff` shows empty — that is expected, not a missing edit.

## Compile prediction — a prediction, not a fact

**I did not build this.** Based on the log's 15 diagnostics mapping one-for-one onto two root causes that are both now removed, and on the three sweeps above finding no further instance of either class, **I expect this file to compile clean.** Confidence is high for the 14 `C3861`/syntax errors (single mechanical cause, fix verified structurally, scope confirmed at all 8 sites) and high for the `C7595` (one specifier, one argument, now matched).

**The residual risk I cannot rule out without a build:** errors the compiler never reached because it stopped emitting after these. The log shows it parsed through `:2658`, which covers the whole file, so this risk is low — but `build-master` re-running the build is the only thing that converts this from prediction to fact.

## For QA / build-master

- Re-run the `TASK-824` compile. Parse the log for `Result: Failed` — **never trust `$LASTEXITCODE`** (`Build.bat` returns 0 on failure).
- Suite total should be **unchanged** by this fix: zero tests added, zero removed, zero renamed.
- If `C3861 'DiffersFrom'` reappears at any subset of the 8 sites, that means a scope problem I mis-read — the table above is the thing to check.
