# TASK-451 — the comparator sweep (gameplay-programmer)

**Status:** ready-for-qa (reviews under TASK-446)
**Date:** 2026-08-03
⛔ **NOT COMPILED. NOT RUN.** TASK-447 is the one compile gate, and nothing in this repo has been built
this batch — so **no test in these four files has ever executed**, before or after this change. Every
claim below is derived from engine source and from the shipped code, not from a green run. Say so
downstream rather than reporting these as passing tests.

---

## 1. THE MECHANISM — re-derived from engine source, not taken from the board

I did not accept the board's or the orchestrator's statement of the defect. Re-derived at the artifact:

| Claim | Engine `file:line` | Verbatim |
|---|---|---|
| The `FString` overload is a thin forward | `Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h:1997-2000` | `bool TestEqual(const TCHAR* What, const FString& Actual, const FString& Expected) { return TestEqual(What, *Actual, *Expected); }` |
| …to the `TCHAR*` overload, which is **case-INSENSITIVE** | `Engine/Source/Runtime/Core/Private/Misc/AutomationTest.cpp:2163` | `bool bAreEqual = (Actual && Expected) ? (FCString::Stricmp(Actual, Expected) == 0) : (Actual == Expected);` |
| `TestEqualSensitive` is the byte-exact twin | `AutomationTest.cpp:2295` | `bool bAreEqual = (Actual && Expected) ? (FCString::Strcmp(Actual, Expected) == 0) : (Actual == Expected);` |
| …and its `FString` overload forwards identically | `AutomationTest.h:2016-2019` | `return TestEqualSensitive(What, *Actual, *Expected);` |

**`Stricmp` vs `Strcmp` — one letter, and it is the whole defect.** ✅ **The board's mechanism is
CONFIRMED for `FString`.** The sweep is not built on a mis-stated mechanism.

### 1a. Where the stated mechanism is NARROWER than it sounds (scope was NOT widened for it)
`FCString::Stricmp` ignores **case and nothing else** — it does not fold whitespace, punctuation,
ordering or Unicode normalisation. ⇒ A byte-identity claim written with `TestEqual` is vacuous
**specifically and only for a regression whose entire diff is alphabetic case.** Any other byte
difference still fails the test. The honest framing of this sweep is therefore *"these assertions
cannot see a casing-only regression"* — **not** *"these assertions check nothing."*

### 1b. Where it is BROADER than CONVENTIONS §13 currently says — ⚠️ **§13 NEEDS ONE AMENDMENT**
§13's bullet *"the defect is confined to `FString` overloads"* is **not accurate.** Four more
`TestEqual` overloads are case-insensitive:

- `FStringView` — `Actual.Compare(Expected, ESearchCase::IgnoreCase)` (`AutomationTest.cpp:2184-2193`)
- `FUtf8StringView` — same (`:2173-2182`)
- **`FText`** — `Actual.EqualToCaseIgnored(Expected)` (`:2195-2204`)
- **`FName`** — `FName::IsEqual(Other)`, whose default is `ENameCase::IgnoreCase` (`NameTypes.h:806`)

⛔ **And the gap that matters for future test authors: `TestEqualSensitive` has NO `FText` and NO
`FName` overload** (`AutomationTest.h:2014-2020` — only `TCHAR*`, `FStringView`, `FString`,
`FUtf8StringView`). A case-sensitive `FName`/`FText` claim **cannot be expressed directly**; you must
call `.ToString()` and compare as `FString`. That is exactly what all four of these files already do,
so **no site in scope was affected** — but it should go in §13 before someone writes
`TestEqual(TEXT("…"), SomeFName, OtherFName)` and believes it is checking case.

**Manager owes §13 that amendment. I did not edit CONVENTIONS (not my file).**

---

## 2. THE COUNT — and a correction to the board

⚠️ **The board says "117 `TestEqual`/`TestEqualSensitive` sites across 4 test files". The real number
is 113.** The 4-site gap is entirely in `SiegeAssistantZoneATest.cpp`: the board's 17 counts **four
prose mentions of the identifiers inside a comment block** (`:353,354,356,357`), not call sites. Actual
call sites there: 13. Counted with `grep -o`, per file, not eyeballed.

| File | Sites | → `TestEqualSensitive` | Left `TestEqual` (integer) | Already Sensitive |
|---|---|---|---|---|
| `SiegeAssistantGrammarTest.cpp` | 53 | **26** | 27 | 0 |
| `SiegeAssistantGuardTest.cpp` | 33 | **29** | 4 | 0 |
| `SiegeAssistantZoneATest.cpp` | 13 | **0** | 8 | 5 |
| `SiegeSettingsTest.cpp` | 14 | **3** | 11 | 0 |
| **Total** | **113** | **58** | **50** | **5** |

✅ **`SiegeAssistantZoneATest.cpp` (TASK-423) was ALREADY FULLY COMPLIANT and I changed nothing in it.**
All 5 of its string comparisons already used `TestEqualSensitive`, with a comment at `:353-362`
deriving the same mechanism independently. Its 8 `TestEqual` calls are all `.Len()` / byte-count
integers. **The board's scope implied work there; there was none.** Its author got this right first.

**Conversion method:** explicit line-number list, first `TestEqual(` on each listed line swapped in
place. **No line was added or removed in any of the three converted files** — so every line number cited
by the live QA reports on TASK-441 / TASK-423 / TASK-436 still resolves. No file was reformatted,
renamed, restructured or re-encoded (verified NO-BOM / LF preserved).

### Deliberately LEFT as `TestEqual` — the reasoning, not just the count
- **All 50 integer / `.Num()` / `static_cast<int32>` sites.** Unaffected by the defect; churning them
  was explicitly out of scope.
- **`SiegeSettingsTest.cpp:132` `TestNotEqual` — LEFT ON PURPOSE, and converting it would be a
  REGRESSION.** The claim is *"the automation scratch slot is NOT the shipped settings slot"*, i.e. the
  hermeticity guarantee. The slot name becomes a **save FILENAME**, and Windows filenames are
  case-insensitive — so two names differing only in case would be **the same file on disk**. The
  case-INSENSITIVE `TestNotEqual` is the comparator that actually discharges that claim; the sensitive
  one would call two aliases of one file "different" and report SAFE. **§13's tell applied in reverse:
  here the insensitive comparator is the stronger one.**
- **`SiegeAssistantZoneATest.cpp:597` `TestNotEqualSensitive`** — same reverse-polarity note (an
  insensitive "these differ" claim would be stricter). **Not touched:** the two lanes differ by the
  whole synonym block, far more than case, so both comparators return the same verdict; and it is
  TASK-423's authored choice in a `ready-for-qa` file whose review cites line numbers. Recorded, not churned.

### Two conversions that CANNOT bite, declared rather than hidden
`SiegeAssistantGrammarTest.cpp:1021` and `:1083` compare `Error` against `FString()` — **empty**. The
only string equal-ignoring-case to `""` is `""`, so these were never vacuous. I converted them anyway so
the file carries one checkable invariant — *every `FString` comparison in these files is `*Sensitive`* —
rather than a mixed idiom a future editor would have to re-derive. **Counted in the 58; worth zero of
the safety.** Flagging so QA does not score them as fixes.

---

## 3. ⛔ GUARD #2 — PROOF THAT A CONVERSION ACTUALLY BITES

Target: **`SiegeAssistantGrammarTest.cpp:464`** — *"Symbol casing does not change the emitted grammar"*,
the flagship site named by §13.

**The guarantee under test** is `SiegeAssistantGrammar.cpp:355` — `const FString Canonical =
Symbol.ToString().ToLower();`. **The regression the test exists to catch is that `.ToLower()` being
deleted.** Inputs are the file's own fixtures: `MidMatchKinds()` (`:59`, lowercase) → `First`, and
`MixedCaseKinds` (`:460` = `{"Footman","ARCHER","SoRcErEr","cleric"}`) → `FromMixedCase`.

Emission reconstructed from the shipped emitters (`GbnfTerminal` `:15-29`, `GbnfJsonString` `:38-52`,
`JoinAlternatives` `:67-69`, kind rule `:499-508`):

```
                 .ToLower() PRESENT (shipped)                    .ToLower() DELETED (the regression)
First         :  kind ::= "\"footman\"" | "\"archer\"" | …       kind ::= "\"footman\"" | "\"archer\"" | …
FromMixedCase :  kind ::= "\"footman\"" | "\"archer\"" | …       kind ::= "\"Footman\"" | "\"ARCHER\"" | …
                 ── identical ──                                 ── differ ONLY in case ──
TestEqual          (Stricmp) →  PASS                             PASS   ⛔ REPORTS SAFE ON THE REGRESSION
TestEqualSensitive (Strcmp)  →  PASS                             FAIL   ✅ CAUGHT
```

⇒ **Pre-fix assertion PASSES, post-fix assertion FAILS, on a concrete input.** Guard #2 discharged.

**Two things this also establishes, and both matter:**
1. ✅ **On the CURRENT, CORRECT code both comparators pass** — the conversion introduces no new failure.
   The sweep does not turn TASK-447's first-ever run red by itself.
2. ⛔ **`:464` was the single worst site in the repo**: `Build` lower-casing every symbol is *the only
   reason* the mixed-case and lowercase fixtures agree, so **casing is the entire content of that
   assertion** — and it was asserted with the one comparator that cannot see casing. The test would have
   gone green forever on the exact defect it was written for.

**Fidelity note:** the arithmetic above is a faithful port of the shipped emitters run outside the
engine, not a UE run — I cannot compile (TASK-447 is the gate). What is *not* inferred is the
mechanism: `Stricmp`/`Strcmp` are quoted verbatim from engine source in §1. `CanonicalizeSymbols`
restricts output to `[a-z0-9_]` (`:379-392`), so ASCII case-folding is an exact model of `Stricmp` here.

---

## 4. THE TWO ADDED ITEMS (scope amendment received mid-task)

### WARN-437-1 — `SettingsMenuWidget.cpp` ✅ one line, as scoped
Added `Initialize();` as the first statement of `USettingsMenuWidget::RebuildWidget()` (+ a comment).
**Engine claims re-derived myself rather than taken from qa/TASK-439.md — all four hold:**
- `WidgetTree` is allocated **inside** `Initialize()` — `UserWidget.cpp:159-162`
- `Super::RebuildWidget()` self-heals **after** our code would already have bailed — `UserWidget.cpp:1197-1200`
- `Initialize()` is **public** — `UserWidget.h:297`, under `public:` at `:285`
- `Initialize()` is **idempotent** — `UserWidget.cpp:135-137`, guards on `!bInitialized && !HasAnyFlags(RF_ClassDefaultObject)`

Hardening, not a live bug: `CreateWidgetInstance` initialises before anything can take the widget.
**Nothing else in that file was touched.**

### WARN-436-2 — `SiegeSettingsTest.cpp` ⚠️ **THIS ONE WAS NOT ONE LINE — REPORTING IT, AS INSTRUCTED**
The defect is real and is now fixed, but **a true one-line fix does not exist**, and the reason is
mechanical: the only API that can move the reader off `true` is `SetAssistantConfirmEnabled`, which
**also persists to disk** (`SiegeSettingsSubsystem.cpp:47`, `bPersistToDisk = true`) into the *same*
scratch slot. Perturbing the reader in place would therefore overwrite the very value the writer just
stored, and the test would fail for a new reason. **The fix has to reorder:** construct `SecondReader`
→ drive it to `false` → *then* have `Writer` store `true` → then load.

Net change: **+1 statement, +1 pre-condition assertion, 1 existing statement moved down**, all inside
`FSiegeSettingsSaveLoadRoundTripTest`. No other test touched. ⚠️ **Flagged rather than absorbed
silently — if the orchestrator wanted strictly one line, this item needs a ruling, not my judgement.**
The final assertion now reads *"save(true) → load() returns true in a SEPARATE store that was sitting
at false"*, which only a real round-trip can satisfy.

---

## 5. FOR QA — what to scrutinise, and what I could not check

1. ⛔ **NOT COMPILED, NEVER RUN.** `TestEqualSensitive` overload resolution on every converted site is
   **unverified by a compiler.** All 58 pass `(const TCHAR* | FString What, const FString&, const FString&)`
   — the `*FString::Printf(...)` message forms resolve to the `TCHAR*` overload — but **UHT/UBT at
   TASK-447 is the first parser to see this.** A failure there is expected-normal, not a QA miss.
2. ⚠️ **THE ONE RESIDUAL RISK IN THIS CHANGE, AND IT IS THE ONLY WAY THE SWEEP COULD TURN TASK-447 RED.**
   The 12 converted sites comparing `FName::ToString()` against a lowercase literal (Grammar
   `:786,788,805,807,809,847,861`; Guard `:228,273,339,352,367,381,383,416,547`; Settings `:126,415`)
   depend on **`WITH_CASE_PRESERVING_NAME`**, which is `#define`d to `WITH_EDITORONLY_DATA`
   (`NameTypes.h:32-33`) = **1 in the editor build these `EditorContext` tests run in** ⇒ `ToString()`
   returns the casing each `FName` was constructed with, and every construction site in these tests
   feeds a lowercase literal. **If it were 0**, `ToString()` would return the *first* casing registered
   process-wide, and a `"Footman"` registered by unrelated code could fail these where `TestEqual`
   passed. **I judge this contained** (these tests are editor-only), **but it is the item to watch on
   the first run, and it is not something I can prove without executing.**
3. Verify my §1b reading independently if you disagree — the `FText`/`FName` gap is the part of §13 I am
   asserting is wrong, and I would rather be corrected than have it enter the law unchallenged.
4. Confirm the two "cannot bite" conversions (§2) and the two deliberate `TestNotEqual` non-conversions
   are the calls you would have made. **I would rather they were argued than assumed.**
5. `SiegeAssistantZoneATest.cpp` is **untouched** — a zero-diff file in this task's scope is the
   expected result, not an omission.

## 6. Files touched
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` — 26 comparator conversions
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGuardTest.cpp` — 29 comparator conversions
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSettingsTest.cpp` — 3 comparator conversions + WARN-436-2
- `Source/GitClaudeUnrealTest/Siegebound/SettingsMenuWidget.cpp` — WARN-437-1, one line + comment
- **NOT touched:** `SiegeAssistantZoneATest.cpp` (already compliant), `CONVENTIONS.md` (§13 amendment is manager's)

**M8 DECLARATION DUTY: adds no replicated property, no new replicated class, no new relevancy tier.**
True by construction — this task changed test assertions and one editor-only widget-initialisation call.
No `UPROPERTY` was added, removed or re-specified anywhere.

⛔ No compile · no Git · no editor · no MCP · no PIE · no `.uasset` · no `.ini`.
