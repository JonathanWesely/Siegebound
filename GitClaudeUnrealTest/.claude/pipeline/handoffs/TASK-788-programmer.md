# TASK-788 — gameplay-programmer handoff

**Two lines, two files. ⛔ Nothing else.**
**Source:** `qa/TASK-779.md` (build-master append, 2026-09-02 — `Result: Failed (OtherCompilationError)`, raw exit code `6`).

---

## 1. WHAT WAS BROKEN — and it is exactly one root cause, twice

```
SiegeClimbableTowerTest.cpp(2030,3)  error C2672: 'FAutomationTestBase::TestNotNull': no matching overloaded function found
SiegeHeroLadderClimbTest.cpp(436,3)  error C2672: same
note: could not deduce template argument for 'const ValueType *' from 'const TObjectPtr<UFunction>'
```

`FMulticastDelegateProperty::SignatureFunction` is a **`TObjectPtr<UFunction>`**. Both `TestNotNull`
overloads are **templates deducing `const ValueType*`**, and ⛔ **template argument deduction does not
run `TObjectPtr`'s implicit conversion-to-raw-pointer** (a user-defined conversion is never considered
while deducing). ⇒ neither overload matches.

⭐ **Why only these two rows fell over, confirmed at source:** every sibling use of the same field is a
**non-deduced** context and compiles clean —
`==` comparison (tower `:2029`, hero `:434`), `if (…)` via `operator bool` (hero `:446`), and
`->NumParms` via `operator->` (hero `:449`). ⛔ I did **not** touch any of them.

---

## 2. THE IDIOM I MATCHED — ⭐ a sibling test in this repo already asserts non-null on this **very** field

**`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp:1061-1062`:**

```cpp
const UFunction* const Signature = ClimbEndedProperty->SignatureFunction;
if (!TestNotNull(TEXT("(b) SELF-CHECK: the delegate carries a signature function"), Signature))
```

It converts via a **typed local** — copy-initialisation of a raw pointer, which *is* a non-deduced
context, so the implicit conversion runs. ⭐ **That file compiled CLEAN in the very build that failed
(`[14/22]`)**, so the idiom is proven against this exact compiler and engine version, not merely plausible.

**⛔ I measured the alternatives before choosing rather than assuming — both would have been inventions:**

| candidate | occurrences in `Source/` | verdict |
|---|---|---|
| typed local + implicit conversion | **1**, on this exact field (`SiegeLadderClimbTest.cpp:1061`) | ⭐ **the repo idiom — matched** |
| `ToRawPtr(...)` | **0** repo-wide | ⛔ inventing one |
| `.Get()` **on a `TObjectPtr`** | **0** — all 152 `.Get()` hits are other smart pointers (`TWeakObjectPtr`/`TSharedPtr`); `ClimbableTower.h:41` is a *comment* quoting engine code (`Query.Owner.Get()`) | ⛔ inventing one |

⚠️ **DECLARED TENSION, so QA can overrule cheaply.** The dispatch said *"do not add a local just to
satisfy the compiler **if a direct conversion reads clearly**"* — but it ranked *"check whether a sibling
test already asserts non-null on a `TObjectPtr` and match it"* **first**, with the smallest explicit
conversion as the fallback *"if none exists."* A sibling **does** exist, and by this repo's own measured
vocabulary **no** direct conversion reads clearly (both candidates have zero precedent). ⇒ I took the
sibling. **If QA prefers the one-token form, the swap is `SharedSignature`/`HeroSignature` →
`HeroClimbEnded->SignatureFunction.Get()` and deletes the local — a trivial, self-contained change.**

---

## 3. THE TWO LINES — before / after

### (A) `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` (was `:2030`, now `:2036-2038`)

**BEFORE**
```cpp
		TestNotNull(TEXT("(c) SELF-CHECK: the shared signature function exists — comparing two nulls would 'pass' while proving nothing"),
			HeroClimbEnded->SignatureFunction);
```

**AFTER**
```cpp
		const UFunction* const SharedSignature = HeroClimbEnded->SignatureFunction;
		TestNotNull(TEXT("(c) SELF-CHECK: the shared signature function exists — comparing two nulls would 'pass' while proving nothing"),
			SharedSignature);
```

### (B) `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroLadderClimbTest.cpp` (was `:436`, now `:442-444`)

**BEFORE**
```cpp
		TestNotNull(TEXT("(e) SELF-CHECK: the delegate carries a signature function at all — an identity compare between two nulls would pass while proving nothing"),
			HeroClimbEnded->SignatureFunction);
```

**AFTER**
```cpp
		const UFunction* const HeroSignature = HeroClimbEnded->SignatureFunction;
		TestNotNull(TEXT("(e) SELF-CHECK: the delegate carries a signature function at all — an identity compare between two nulls would pass while proving nothing"),
			HeroSignature);
```

⭐ Each site also gained a **comment block** naming the mechanism (`TObjectPtr` + deduction), citing
`SiegeLadderClimbTest.cpp:1061`, and stating that the row is unchanged — so the next reader does not
"tidy" the local away and re-break the build.

---

## 4. ⭐⭐ THE SELF-CHECKS ARE INTACT — the assertion message is **byte-identical** and it still fails on a null

⛔ **Neither row was deleted, weakened to a bare `if`, nor dropped in favour of the `==` row alone.**

- Both `TEXT(...)` messages are **character-for-character** what they were. ⛔ Nothing was re-worded.
- The **subject** is unchanged: still `HeroClimbEnded->SignatureFunction`, reached through the same
  property, on the same object, inside the same `if (HeroClimbEnded && UnitClimbEnded)` guard.
- **It still fails on a null.** `TestNotNull(const TCHAR*, const ValueType*)` reports a failure when the
  pointer is null. `TObjectPtr<UFunction>`'s conversion yields the underlying raw pointer verbatim —
  a null `TObjectPtr` converts to `nullptr`, and the local is `nullptr`, and the row goes **red**.
  ⇒ the guard against *"two nulls would compare equal and the assertion would pass while proving
  nothing"* is **exactly as strong as QA ruled it**. The only thing that changed is where the
  `TObjectPtr → UFunction*` conversion happens: on the initialiser instead of inside a deduced argument.
- ⛔ **No type was changed.** `SignatureFunction` is still a `TObjectPtr<UFunction>`; the locals are new
  `const UFunction* const` reads of it, not a redeclaration of anything.

---

## 5. ⛔ THE SUITE STAYS **279** — declared explicitly

**⛔ Assertions added: 0. ⛔ Assertions removed: 0. ⛔ Test macros added: 0. ⛔ Test macros removed: 0.**

This is guaranteed by construction: each edit's replaced text contained **exactly one** `TestNotNull(`
and its replacement contains **exactly one** `TestNotNull(`. I repaired the call idiom of two existing
assertions; I did not touch the assertion inventory.

**Measured after the fix, ⛔ not asserted from memory:**

```
macro census, IMPLEMENT_*_AUTOMATION_TEST across Source/  =  279
"Siegebound.LadderClimb.       = 17
"Siegebound.ClimbableTower.    = 14
"Siegebound.HeroLadderClimb.   = 24
```

⇒ **279**, with all three group riders exactly as `qa/TASK-779.md` computed them. ⛔ Unchanged by this task.

---

## 6. ⚠️ WHAT QA / TASK-780 SHOULD SCRUTINISE

1. ⭐ **The idiom choice (§2).** The one genuine judgement call in this task, and it is declared with its
   measurements and its escape hatch. ⛔ Not hidden.
2. ⭐ **The two locals are used by the repaired row only** — the `==` / `if` / `->NumParms` rows still read
   `HeroClimbEnded->SignatureFunction` directly. **That is deliberate**: the fence said two lines and
   nothing else, and those rows **compile clean**. Folding them onto the local would be a tidy that
   touches working code inside a repair, which is how a two-line fix becomes a four-line regression.
   ⚠️ It does read slightly asymmetrically — ⛔ flagged rather than silently "fixed."
3. ⛔ **There is no third latent instance of this error class, and that is MEASURED, not hoped:** MSVC
   enumerates every C2672 in a translation unit, and the build log reported **exactly 2 errors, 0
   warnings** across 19 of 22 actions. Both are repaired. ⇒ any *new* error at TASK-780 would be a
   different defect, ⛔ not a leftover of this one.
4. ⚠️ **279 has never actually executed for this batch** — the link never produced a binary. The two
   deliberately-**INVERTED** assertions (`SiegeHeroLadderClimbTest` test 20(d)) remain **unobserved**.
   ⭐ They are expected **GREEN**. ⛔ **If either is red after this fix, that is a REAL REGRESSION, ⛔ not
   the expected inversion** — do not wave it through as "the inverted rows."
5. ✅ **⛔ No shipped runtime file was opened.** `ClimbableTower.{h,cpp}`, `HeroCharacter.{h,cpp}`,
   `SiegeLadderClimbStatics.{h,cpp}`, `SummonedUnit.{h,cpp}`, `LadderClimber.h` — all untouched by
   TASK-788, exactly as the build-master's narrowing said they should be.

---

## 7. FILES TOUCHED — the complete list

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` | one assertion's call idiom repaired (`:2036-2038`) + its explanatory comment |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroLadderClimbTest.cpp` | one assertion's call idiom repaired (`:442-444`) + its explanatory comment |

**Assets referenced:** ⛔ **none** — this task references no `/Game/` asset and no mesh, texture or material.

⛔ No compile (TASK-780 owns it) · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `CONVENTIONS.md` edit.
⚠️ **TASK-788 was ⛔ dispatched, ⛔ not boarded — self-boarded retroactively by its assignee under
TASKBOARD rule 6**, the same treatment TASK-785 and TASK-786 received in this batch.

**Status:** `ready-for-qa`.
