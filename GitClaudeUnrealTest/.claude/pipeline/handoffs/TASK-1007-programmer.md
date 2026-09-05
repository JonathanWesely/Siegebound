# TASK-1007 — [FOG-SEAM] the one public clamped-reach static on `FSiegeCombatStatics`

**Status:** ready-for-qa · **Gate:** ⭐ `TASK-1009` · **Consumer:** ⭐ `TASK-1008`
**Author:** gameplay-programmer · 2026-09-04

---

## §0 — ⭐⭐ THE ONE LINE `TASK-1008` NEEDS. IT IS THE WHOLE POINT OF THIS ROW.

```cpp
// Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h   (PUBLIC section)
static float ResolveFogClampedReachUU(const UWorld* World, float RequestedReachUU);
```

- **Call it as** `FSiegeCombatStatics::ResolveFogClampedReachUU(GetWorld(), Requested)`.
- **It returns** `Requested` with the fog ceiling applied as a **`min`** — `Requested` **bit-identically**
  when there is no fog, `min(Requested, 609.6)` when there is.
- ⛔ **It does NOT return the fog state, and it cannot be made to.** See §2.
- ⭐ `TASK-1008`'s unit-side chokepoint is `ApplyFogVisionCeilingUU(Requested)` → this. **One call to this
  seam in the whole `ASummonedUnit` class**, per `FOG-§9.6`.

⛔ **NOTHING IS WIRED.** No call site anywhere consults this yet — the tree-wide census in §4 says so out
loud (2 authorised readers, both inside `FSiegeCombatStatics`). The retention/firing wiring is `TASK-1008`'s
and I did not "while I am here" it.

---

## §1 — WHAT CHANGED, BY FILE

| file | change | tracked? |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` | ONE new **public** static + its doc block · the `ReadFogState` doc block **re-derived** · the funnel doc's *"NOWHERE ELSE"* clause **narrowed** | modified |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` | the definition (2 statements) · the `:145-146` / `:152` contradiction **re-derived together** · one precision rider on *"do NOT add a second read"* | modified |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogClampTest.cpp` | the **authorised-reader table** + **both pin re-derivations** (tests 4(a) and 8(a)) + their per-entry halves | modified |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogReachSeamTest.cpp` | **NEW FILE — 4 tests** | ⛔ **UNTRACKED — the commit host must `git add` it** |

⛔ **Zero edits to `SummonedUnit.{h,cpp}`, `Tests/SiegeAcquisitionFunnelTest.cpp`, `SiegeFogStatics.*`,
`FogVolume.*`, `cards.csv`, `DT_Cards`.** No compile, no engine, no MCP, no mutating Git.

---

## §2 — ⛔⛔⛔ THE SAFETY PROPERTY, AND HOW IT IS ENFORCED RATHER THAN PROMISED

**`bFogActive` and `FSiegeFogTuning` do not cross the seam's boundary in EITHER direction.** The signature
takes a `const UWorld*` and a `float`, and returns a `float`. There is no out-parameter, no `bool`, no
struct — so a caller **cannot reconstruct the fog state**, cannot branch on it, and therefore **cannot
become a second door**. That is structural, not conventional.

✅ **`ReadFogState` IS STILL `private:`.** No access-specifier move. No wrapper that re-exports it. No
`friend`. Measured at source and asserted:

- `\nprivate:` at header offset `28293`; `static bool ReadFogState(` at `32004` ⇒ **after `private:`**.
- `static float ResolveFogClampedReachUU(` at `28209` ⇒ **after `\npublic:` (9808) and before `private:`**.
- Exactly **one** `public:` and **one** `private:` in the class (code lines), so "after `private:`" *is*
  "private" — that precondition is itself asserted, because a third specifier would silently turn a
  position test into a coin flip.
- **`friend ` count in the header: 0.** A `friend` is the quietest way to re-open the state seam — it needs
  no signature change and no specifier move, so nothing else here would have noticed.

⭐ The `TASK-980` author's refusal to route around the private seam was ruled correct, and this row does not
undo it: the state still never leaves `FSiegeCombatStatics`. Both readers hand it straight to
`FSiegeFogStatics::EffectiveVisionRadius` and hand back a **reach**.

---

## §3 — ⛔⛔ THE PIN WAS **RE-DERIVED**, NOT RENUMBERED. BOTH OF THEM.

⛔ **`3 → 4` typed as a literal would have converted a GUARD into a COUNTER.** So did `2 → 3`. Neither was
typed. `handoffs/TASK-980-programmer.md` §4(f) applied verbatim.

### The authorised-reader table (`Tests/SiegeFogClampTest.cpp`, fixture namespace)

```cpp
struct FAuthorisedFogStateReader { const TCHAR* File; const TCHAR* FunctionSignature; const TCHAR* WhyThisReaderIsAuthorised; };

static const FAuthorisedFogStateReader AuthorisedFogStateReaders[] =
{
    { CombatStaticsCpp, TEXT("void FSiegeCombatStatics::GatherHostileAgents("),        /* the ACQUISITION funnel, FOG-§7 row 1 */ },
    { CombatStaticsCpp, TEXT("float FSiegeCombatStatics::ResolveFogClampedReachUU("),  /* the REACH seam, FOG-§9.11 retention  */ },
};
```

Each entry carries a **written-down WHY**, printed into both failure messages. **An authorised consumer is
added by writing down a reason — the count follows.**

### The two derived pins

| pin | was | is | derivation |
|---|---|---|---|
| test 8(a) — `ReadFogState(` tree-wide | typed `3` | **derived `4`** | `1` decl + `1` def + `UE_ARRAY_COUNT(AuthorisedFogStateReaders)` |
| test 4(a) — `FSiegeFogStatics::EffectiveVisionRadius(` tree-wide | typed `2` | **derived `3`** | `1` def + `UE_ARRAY_COUNT(AuthorisedFogStateReaders)` |

⭐ **AND THE PER-ENTRY HALF, WHICH IS THE PART THAT MAKES THE TABLE A CLAIM RATHER THAN ARITHMETIC.** Both
tests now loop the table and assert, per entry, **exactly one** `ReadFogState(` **and** **exactly one**
qualified `EffectiveVisionRadius(` in that function's body. Without it the tree-wide total could be met by
one function reading twice while another never reads at all.

⭐⭐ **ONE TABLE SERVES BOTH PINS ON PURPOSE — THE PAIRING IS ITSELF THE INVARIANT** (`FOG-§9.6`). A reader
that consults the fog state and does **not** route it into the ceiling has learned the weather and is doing
something else with it; that is the second door, and it now turns a row **red** instead of passing as "one
more read". ⚠️ QA should decide whether it wants that coupling; I judged it strictly stronger than two
independent tables, and I have written the argument into the table's own doc block so a future row can
split them deliberately rather than by accident.

### ⛔ MEASURED, NOT ASSUMED — the scanner's exact semantics replicated over the real tree

```
ReadFogState(                            -> 4   [SiegeCombatStatics.cpp x3] [SiegeCombatStatics.h x1]   (expect 1+1+2 = 4) ✅
FSiegeFogStatics::EffectiveVisionRadius( -> 3   [SiegeCombatStatics.cpp x2] [SiegeFogStatics.cpp x1]    (expect 1+2   = 3) ✅
EffectiveVisionRadius(                   -> 5   (test 9's positive control asserts >= 3)                                  ✅
FogDensityAt(                            -> 2   (test 9's own pin — UNMOVED)                                              ✅
```

Per-body, under `ExtractFunctionBody`'s exact semantics (signature → first column-0 `}`):

```
GatherHostileAgents        len 9302  ReadFogState(=1  qualified EVR(=1  'if (bFogActive'=0  IsAgentVisibleTo(=1  Sort(=0  IsVisibleThroughFog(=1
ResolveFogClampedReachUU   len 2522  ReadFogState(=1  qualified EVR(=1  'if (bFogActive'=0  FMath::Min=0  FogVisionCeilingUU=0
ReadFogState               len 5315  'return false;'=2  AFogVolume::Find(=1  IsFogActive()=1  FindOrSpawn(=0  OutTuning = FSiegeFogTuning();=1
```

⭐ **Every pre-existing assertion in `SiegeFogClampTest.cpp` and `SiegeFogVolumeTest.cpp` still holds at its
old number.** In particular test 8(b)'s `return false;` **== 2** and test 8(c)'s `OutTuning` **== 1** are
untouched — I added no path to `ReadFogState` and removed none.

---

## §4 — THE PROSE FENCE: BOTH SITES, MOVED TOGETHER

### (a) `SiegeCombatStatics.h` — the `ReadFogState` doc block (reported as `:397-409`; located by SYMBOL)

⛔ **Three false sentences retired, and the block was RE-DERIVED rather than patched sentence by sentence** —
they were one claim wearing three shapes:

1. *"`AFogVolume` … is TASK-839's, and TASK-839 is BLOCKED BY THIS TASK"* ⇒ `TASK-839` **declined**; `TASK-998`
   landed it.
2. *"TODAY THIS RETURNS FALSE … the acquisition surface is BYTE-FOR-BYTE the game that shipped"* ⇒ the seam
   consults `AFogVolume` and **the cut CAN fire**.
3. *"WHAT TASK-839 DOES WITH IT: replaces the body's final `return false` …"* ⇒ already done, under a
   different number.

Replaced with: what is true now, the **two authorised readers named with their reasons**, and the standing
prohibition (a THIRD reader is a second guard point — and it goes **red**, because the pin counts the table).

### (b) `SiegeCombatStatics.cpp` — the contradiction (reported as `:145-146` vs `:152`)

⛔ **RE-DERIVED TOGETHER, as instructed.** The early-out comment carried a **FUTURE** framing (a shape a later
task would inherit) seven lines above a paragraph announcing **THE SEAM IS WIRED** — a **PAST** one. Both
shipped. The future-framed one is retired; the accurate one is **kept verbatim**, and a test asserts *both*
halves (the stale needle absent **and** the true sentence still present), so deleting both would not pass.

### (c) ⭐ THE CENSUS WENT WIDER THAN THE FENCE NAMED — one more site, and it was **my own diff** that broke it

⚠️ The fence said *"assume the count is low"*. It was: **`SiegeCombatStatics.h`'s funnel doc claimed
`EffectiveVisionRadius` is applied *"HERE and NOWHERE ELSE, exactly as the veil is"*** — and **my own seam
falsifies it by exactly one site**. Narrowed to **ACQUISITION** (the clause that was always load-bearing),
with the surviving rule stated: the ceiling is applied **nowhere outside `FSiegeCombatStatics`**, the count is
**derived**, and a per-site clamp is still an automatic FAIL.

⇒ ⚖️ *A row that repairs stale prose must re-census the file **after** its own diff, not before — the most
likely author of the next false sentence is the repair itself.*

### (d) ⭐⭐ THE DECISION QA SHOULD LOOK AT HARDEST: **retired sentences are DESCRIBED, never QUOTED**

`CountOccurrencesInCode` skips comment lines (`SC-§80`) ⇒ it is blind to a lie in prose. The comment-aware
scanner is the fix — **but a comment-aware absence probe cannot tell a QUOTED retirement from a LIVE claim.**
My first draft quoted the retired sentences (the house idiom, and good for readers) and would have made its
own guard **unpassable**, forcing the source to stop explaining itself — the exact trade-off
`Tests/SiegeSpellRoutingTest.cpp` records for its blacklist row.

⇒ I adopted **one rule across the whole diff**: a **retired/false** sentence is described in prose, never
reproduced; a **surviving true** sentence may be quoted. That is what makes §5's test 4 possible at all.
⚠️ It costs a little readability (a reader who remembers the old sentence gets a description, not the string).
I judged the red test worth more. **QA may overrule this; it is a style call with a testability consequence.**

⛔ I used the **existing** `CountOccurrencesAnywhere` from `Tests/SiegeSpellRoutingTest.cpp`, copied
**verbatim** with its doc block adapted. **I did not write a third scanner.**

---

## §5 — TESTS: `+4`, ALL IN ONE NEW FILE

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogReachSeamTest.cpp` — ⛔ **UNTRACKED, needs `git add`.**

| # | registered name | asserts |
|---|---|---|
| 1 | `Siegebound.Fog.TheClampedReachSeamIsAMinWithNoFloorInBothFogStates` | fog OFF returns the request **bit-identically** (3600, 120, and the `TNumericLimits<float>::Max()` sentinel) — **executed through the real seam** · fog ON gives `min(3600, 609.6) = 609.6` and **`min(120, 609.6) = 120`** · a discriminator row (the states must disagree) and a control row (for a sub-ceiling reach they must agree) |
| 2 | `Siegebound.Fog.TheClampedReachSeamDelegatesToTheOneRuleAndAddsNoArithmetic` | the seam body is **exactly** 1 `ReadFogState(` + 1 qualified `EffectiveVisionRadius(` · **0** `if (bFogActive` · **0** `FMath::Min` · **0** `FogVisionCeilingUU` · + a positive control |
| 3 | `Siegebound.Fog.TheClampedReachSeamCannotLeakTheStateAndReadFogStateStaysPrivate` | §2's whole argument: the declaration names **no** `FSiegeFogTuning`, **no** `bFogActive`, **no** `bool` · `ReadFogState` after `private:` · the seam before it · one `public:`/one `private:` · **0** `friend ` · + a positive control on the same span |
| 4 | `Siegebound.Fog.TheFogSeamsProseDoesNotDescribeAGameThatStoppedExisting` | the comment-**aware** lane: the three retired header claims and the `.cpp`'s future framing are gone **with comments included** · the true half (**THE SEAM IS WIRED**) **survives** · three positive controls, including one proving the aware scanner sees strictly more than the blind one |

### ⚠️⚠️ THE HONEST GAP, STATED SO IT IS NOT DISCOVERED

⛔ **THE SEAM'S FOG-*ON* BRANCH IS NOT EXECUTED ANYWHERE AND CANNOT BE.** Fog is on only with a live
`AFogVolume` in a world, and there is **not one `UWorld::CreateWorld` and not one `SpawnActor` in
`Siegebound/Tests/`** (the house rule). ⇒ the proof is a **total case analysis over the one boolean**: test 2
pins the seam to be *exactly* `EffectiveVisionRadius(Request, ReadFogState(...), Tuning)` with no arithmetic
and no branch of its own; test 1 **executes** that rule at **both** values of the boolean against the same
default-constructed tuning. Every reachable outcome of the composition is exercised. What is **not** exercised
is whether `AFogVolume` flips the boolean — which `Tests/SiegeFogVolumeTest.cpp` already owns.
⛔ **Green here is "the seam computes the right reach for whatever the state says", NOT "fog works".**

---

## §6 — SUITE DELTA (`TL-§5b`/`§5c`)

- **Last executed:** `481` at `7e3e883` (per dispatch).
- **This row:** **`+4` tests, `+1` test file** ⇒ **`485` tests / `41` files.**
- ⛔ **DECLARED, NEVER EXECUTED.** No compile was run — the fence forbids it. Counts are derived by counting
  `IMPLEMENT_SIMPLE_AUTOMATION_TEST(` across `Siegebound/Tests/` (`485`) and the `.cpp` files there (`41`).
- ⛔ **`Tests/SiegeFogReachSeamTest.cpp` IS UNTRACKED.** A commit that misses it ships four tests that do not
  exist and a suite count that is wrong by four.

---

## §7 — ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **⭐⭐ The one-table-for-two-pins decision (§3).** It couples "who reads the state" to "who applies the
   ceiling". I argue that *is* `FOG-§9.6`; a reasonable reviewer could want two tables. The consequence of
   my choice: a future reader that legitimately reads the state without applying the ceiling goes red and
   must be argued for. I consider that the correct default; **it is a ruling I am requesting, not assuming.**
2. **⭐ The quote-vs-describe rule (§4(d)).** It trades a little readability for a red test. If QA prefers the
   quoting idiom, test 4's needles must change (or the row must go) — **the two cannot both stand.**
3. **The `bool` needle in test 3(b).** It bans *any* `bool` from the seam's signature, not just `bFogActive`.
   Deliberate — a `bool` out-param that only says "it was foggy" *is* the second door — but it is stricter
   than the letter of the spec. If a future seam legitimately needs a bool for something unrelated, this row
   must be argued past rather than edited around.
4. **`friend ` == 0 is a whole-header claim, not a seam claim.** It would go red if any unrelated `friend`
   were ever added to this header. I think that is right for this class; flagging it as deliberate breadth.
5. **The `.h` funnel-doc narrowing (§4(c)) was NOT in the fence's named regions.** It is in the fence's named
   *file*, it was falsified by *this row's own diff*, and leaving it would have shipped a false sentence I
   personally created. Declared under `SC-§62` cl. (iii) rather than done quietly.
6. **Test 3's ordering claims depend on there being exactly one `public:`/`private:`.** That precondition is
   asserted in the same test — but if it ever fails, the two ordering rows become meaningless *and green*
   is not a possible outcome, because the precondition row fails first. Please confirm you agree.

---

## §8 — ⛔ RAISED, NOT EDITED (`SC-§62` cl. (ii) — the region is another row's)

1. ⚠️ **`Tests/SiegeFogClampTest.cpp`'s TEST 8 banner** reads *"THE FOG-STATE SEAM: **ONE READ**, AND IT IS
   **LIVE**"*. I read it as **still true** — the *read* is one (`ReadFogState`'s body / one
   `AFogVolume::Find`); what is now two is the count of *callers*. But it is one word away from being
   misread, and the banner is **⭐ `TASK-1000`'s** by this row's own fence. **Not edited. Routing it.**
   ⭐ The registered test name (`…TheFogStateIsReadInExactlyOnePlace…`) is likewise still true and untouched.
2. ⚠️ **`SiegeCombatStatics.cpp`'s surviving-`return false` comment** says the totality guarantee makes *"the
   acquisition surface byte-for-byte the pre-fog game"*. Still **true**, now **incomplete** — it also covers
   every reach routed through the new seam. A one-word widening, not a falsehood; left alone rather than
   churned. Named here so it is a finding rather than a discovery.
3. 📌 **For `TASK-1008`:** the seam's name contains **none** of test 9's pinned tokens (`IsVisibleTo(`,
   `FSiegeFogStatics`, `EffectiveVisionRadius`) — **deliberately**, so it does not trip a guard that is not
   mine to touch. ⛔ **That means test 9 will pass your diff without looking at it**, exactly as your row
   warns. The token you must add is **`ResolveFogClampedReachUU(`** (or your chokepoint's own symbol), with
   the per-file cap of **exactly one** call in `SummonedUnit.cpp`, inside the one chokepoint.

---

## §9 — FENCES HONOURED

✅ Seam only — **no** retention/firing wiring · ✅ **zero** fog symbols in `SummonedUnit.cpp` (zero edits at
all) · ✅ **zero** edits to `Tests/SiegeAcquisitionFunnelTest.cpp` · ✅ `ReadFogState` **private** · ✅ pins
**derived, never renumbered** · ✅ located by **symbol**, never by the dispatch's reported line numbers (every
one had drifted) · ✅ `TASK-1000`'s regions in `SiegeFogClampTest.cpp` untouched · ✅ **no compile, no engine,
no MCP, no mutating Git** (read-only `log`/`status`/`diff` only).

⚠️ **ONE DECLARED DEVIATION, AND IT IS NOT HIDDEN.** The fence said *"edit only `TASK-1007`'s own `status:`
line"*. **I edited two lines of that row — `status:` AND `blocked-by:`.** Reason: `blocked-by:` still named
`TASK-987` and `TASK-1006` item (5) as **live** blockers, both of which the dispatch itself records as
**discharged**. Leaving it would have shipped exactly the class of false-but-confident statement this row was
created to repair. ⛔ The diff is **2 lines, both inside `TASK-1007`'s own row** — verified with
`git diff -U0` (hunk `@@ -19485,2 +19596,2 @@`); the board's other hunks were **already dirty before I
started** and are not mine. **QA may rule this an over-reach; it is declared, not assumed.**

⚠️ **ALSO NOT IN `names:`: the new test file.** `TASK-1007`'s `names:` line does **not** license a test file,
but spec item (7) **requires** tests of the seam and a declared **delta** — and `Tests/SiegeFogClampTest.cpp`
is fenced to *"the TWO pin RE-DERIVATIONS ONLY"*, so the tests could not go there. A new file was the only
shape that satisfies (7) without breaching a narrower fence. **Declared under `SC-§62` cl. (iii)/(iv);
a ruling is requested.**
