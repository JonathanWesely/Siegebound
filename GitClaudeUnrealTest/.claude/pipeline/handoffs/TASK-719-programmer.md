# TASK-719 — the one unsound assertion in `SiegeControlsHelpTest.cpp`, repaired

**Agent:** gameplay-programmer · **Date:** 2026-08-30 · **Status:** ready-for-qa
**File touched — ONE, and nothing else:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp`
(test 4, `Siegebound.ControlsHelp.RawLanesAreIdentity`, the Lane-C **F-1 reversal** block, formerly `:627-634`)

⛔ Not touched: `SiegeControlsHelpWidget.{h,cpp}` · the registry data · every other test · `MakeDvorakTranslation()` — **the shared fixture map is byte-identical** · `Tools/Packaging/` · no compile, no editor, no MCP, no Git.

---

## 1. THE DIAGNOSIS — VERIFIED BEFORE IT WAS FIXED, AT THREE LEVELS

Build-master's reading is correct, and it is correct about the **shipped** map and not merely the fixture:

| # | Evidence | What it proves |
|---|---|---|
| 1 | `SiegeControlsHelpTest.cpp:98` — `Translation.Add(EKeys::Z, EKeys::Semicolon)`, and ⛔ **no `Semicolon` entry exists** | in the fixture, `Semicolon` is a fixed point |
| 2 | `SiegeKeyboardLayoutStatics.cpp:55-64` — the source table is **`{EKeys::A…EKeys::Z}`, 26 rows, letters only** | in the **shipped** subsystem `Semicolon` can never *be* a source ⇒ the fixed point is real, not a fixture artefact |
| 3 | `SiegeControlsHelpWidget.cpp:1110-1117` (Lane B) — *"the translation table holds A..Z and nothing else … GetPositionalKey on a mouse button / Escape / the wheel is a provable identity"*, contract at `SiegeKeyboardLayoutSubsystem.h:238-240` (absent key returns **unchanged**) | the shipped code **relies** on exactly this identity |

⇒ `GetPositionalKey(Semicolon) == Semicolon`, so the old assertion reduced to **`Semicolon != Semicolon`** — it could not pass however correct the code was, and (the worse half) **at that position a double translation is indistinguishable from a single one**, so it could not have caught the defect even in principle. Unsound fixture, ⛔ not a hole in the feature.

## 2. BEFORE / AFTER

**BEFORE** (`:630-633`, both claims inside one `if`):
```cpp
TestTrue(TEXT("⭐ Clearing bLiteralKeyLabel makes the row DERIVE - the F-1 reversal really is one flag"),
    DerivedResolved[0] == ZTranslated);
TestTrue(TEXT("...and it is EXACTLY ONE translation, never two"),
    DerivedResolved[0] != Dvorak.Layout->GetPositionalKey(ZTranslated));   // ⛔ Semicolon != Semicolon
```

**AFTER** — the `Z` derive claim is **kept verbatim** (it is sound and green: it is what proves the F-1 reversal is one flag *on the accept key's own row*). Only the unmeasurable claim moves, onto the **same Lane-C derive path** at a key whose image has a distinct onward hop:
```cpp
if (DerivedResolved.Num() == 1)
{
    TestTrue(TEXT("⭐ Clearing bLiteralKeyLabel makes the row DERIVE - the F-1 reversal really is one flag"),
        DerivedResolved[0] == ZTranslated);          // ⭐ unchanged, still on Z
}

// …comment recording WHY the claim cannot be made at the Z position…
const FSiegeControlsHelpAction DerivedHopRow =
    MakeRow(ESiegeInputLane::RawLetter, { EKeys::F }, /*bLiteral=*/false, false);
const TArray<FKey> DerivedHopResolved =
    FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(DerivedHopRow, TArray<FKey>(), Dvorak.Layout.Get());

const FKey OneHop  = Dvorak.Layout->GetPositionalKey(EKeys::F);      // U
const FKey TwoHops = Dvorak.Layout->GetPositionalKey(OneHop);        // G

// FIXTURE SELF-CHECK — the claim is not vacuous the way the Z one was:
TestTrue(…"the derived key's image has a DISTINCT onward hop…(%s -> %s -> %s)"…, TwoHops != OneHop);

TestEqual(TEXT("The derived raw-letter row labels exactly one key"), DerivedHopResolved.Num(), 1);
if (DerivedHopResolved.Num() == 1)
{
    TestTrue(…"...and it is EXACTLY ONE translation, never two (got %s; one hop is %s, two hops would be %s)"…,
        DerivedHopResolved[0] == OneHop && DerivedHopResolved[0] != TwoHops);
}
```

## 3. ⭐ WHY THE NEW ASSERTION CAN ACTUALLY FAIL — THE DEFECT, NAMED CONCRETELY

The row is `ESiegeInputLane::RawLetter`, `bLiteralKeyLabel = false`, reference key **`F`** ⇒ it takes `SiegeControlsHelpWidget.cpp:1129-1137`, the one-`GetPositionalKey`-per-key derive loop. Under the injected map (`SiegeControlsHelpTest.cpp:88-89`, both entries pre-existing and placed there for exactly this purpose):

* **correct — one translation:** `F → U`. `DerivedHopResolved[0] == OneHop(U)` ✅ and `!= TwoHops(G)` ✅ ⇒ **PASS**.
* **defective — two translations:** `F → U → G`. First conjunct `G == U` **false** ⇒ **FAIL**, and the message prints `got G; one hop is U, two hops would be G`, naming the defect on sight.

That is the *literal* defect the shipped code documents as the thing to prevent — `SiegeControlsHelpWidget.cpp:1150-1152`: *"on US-Dvorak `F` -> `U` -> `G`, teaching the wrong key to the one player this feature exists for"* — and it is test 2's proven idiom (`MappedLaneIsNeverDoubleTranslated`, green), reused here rather than re-invented.

⛔ Not weakened, ⛔ not deleted, ⛔ not `#if 0`, ⛔ no expected value edited to make it green: the claim is **strictly stronger** than the old text, which asserted only `!=` the double image; the new one asserts `== the single image AND != the double image`, so it also fails on a *third* wrong answer that merely happens not to equal the double image. The fixture self-check above it guarantees `TwoHops != OneHop`, so the claim can never quietly go vacuous the way its predecessor did.

## 4. THE 156 TOTAL — UNCHANGED

`IMPLEMENT_SIMPLE_AUTOMATION_TEST` occurrences in `SiegeControlsHelpTest.cpp`: **13, before and after** (⛔ none added, ⛔ none removed — measured with `grep -c`). The edit lives entirely **inside** `FSiegeControlsHelpRawLanesTest::RunTest`, which adds assertions to an existing test rather than a test to the suite. ⇒ **the suite total stays 156**, and the expected result is **156 / 156 pass** (the one FAIL was this assertion and nothing else was red).

## 5. DECLARED DEVIATIONS (`SC-§15`)

* **D1 — the claim MOVED off `Z` instead of `Z`'s image being given an onward hop.** The prompt allowed either. Extending `MakeDvorakTranslation()` with a `Semicolon → …` entry was **rejected**: the shipped table's domain is the 26 letters (`SiegeKeyboardLayoutStatics.cpp:57-63`), so a non-letter *source* entry would be a fixture that models a map the shipped code can never produce — and Lane B's identity assertions in this same test rest on that domain. ⛔ The fixture stays byte-identical.
* **D2 — the `Z` derive assertion was KEPT, not replaced.** The F-1 reversal's own claim ("clearing the one flag makes the accept row derive") is sound and green at the `Z` position; only the *double-translation* claim was unmeasurable there. Splitting them keeps the F-1 tie to the actual accept key.
* **D3 — +3 assertions in test 4** (a fixture self-check, an arity guard, the repaired claim) replacing 1. No new test; see §4. The arity guard exists so the indexed read cannot run off an empty array.
* **D4 — a ~10-line comment** records why the claim cannot live at the `Z` position, with the three citations from §1. Deliberate: without it the next reader "simplifies" this straight back into the fixed point.

## 6. WHAT QA SHOULD SCRUTINISE (`SC-§27`, diff-scoped)

1. That `MakeDvorakTranslation()` really is untouched, and no other test's behaviour can move.
2. That `F`/`U` were **already** in the fixture (`:88-89`) with a comment saying they exist for this purpose — nothing was added to make the new claim true.
3. That the new claim is not vacuous: the fixture self-check `TwoHops != OneHop` fires first.
4. That `Describe`/`MakeRow`/`ResolveRowDisplayKeys` are used exactly as the surrounding blocks use them (`using namespace SiegeControlsHelpTestUtils;` is already in scope at `:570`).
5. ⛔ **Not compiled** — a re-run of TASK-709 owns the compile + suite gate. Syntax is the file's own existing idiom; no new include, no new symbol, no new type.

**M8 declaration (verbatim):** adds no replicated property, no new replicated class, no new relevancy tier, no RPC. Test file; no shipped surface at all.
