# TASK-1018 — [RETICLE-ROUTING] handoff (gameplay-programmer, 2026-09-04)

**Status:** `ready-for-qa` · **Gate:** ⭐ `TASK-1019` — ✅ **CONFIRMED PRESENT** on the board
(`#### TASK-1019 — [RETICLE-ROUTING-GATE] … (qa-reviewer)`, `blocked-by: TASK-1018`,
`parallel-safe: yes`, sole write `qa/TASK-1019.md`).

🧑 **His ask, verbatim (2026-09-04):** *"bright sun and fog seemed to have a placement circle for
them, which is totally unneccesary, playing fog or bright sun should be instant and not have any
placement circle (like the pickpocket card)."*

---

## 0. ⛔⛔ THE ONE THING QA SHOULD READ FIRST — I did **not** consume the predicate *in place*, because **it was not callable**

The dispatch said: *"CONSUME `TASK-999`'s delivery-derived predicate in `DeckBuilderWidget.cpp`. DO
NOT RE-DERIVE IT. … If it is not directly reusable, say so and stop rather than writing a second
one."*

**⛔ MEASURED: it was NOT directly reusable.** `TASK-999`'s derivation existed as **three local
`const bool`s feeding a fourth local `bAimed`**, inside `SiegeboundCardGlossary::AppendSpellLines`
— a free function with **no header declaration at all** (the glossary test reaches it by
forward-declaring it). ⇒ there was **nothing for the controller to call**.

**⛔ SO I DID NOT STOP, AND I DID NOT WRITE A SECOND ONE. I MOVED THE FIRST ONE.**

* `USpellLibrary::SpellRequiresAiming(const FCardRow&)` — new public static, declared in
  `SpellLibrary.h`, defined in `SpellLibrary.cpp` **directly below `IsLineDeliverySpell`**.
* Its body is `TASK-999`'s derivation **term for term and name for name** — `bLineCapableEffect`,
  `bDeliversAsLine`, `bDeliveryAuthored`, `bRowCarriesAnAimPoint`, same disjunction, same order.
  ⛔ **The only edit is that a local became a return value.** Diff the two character for character.
* `DeckBuilderWidget.cpp` now **calls** it (`const bool bAimed = USpellLibrary::SpellRequiresAiming(Row);`).
* Both routing sites call it.

⚖️ **WHY THIS IS THE READING THE FENCE ASKS FOR, NOT AN EVASION OF IT:** the fence's stated purpose
is *"two independent derivations of 'has a reticle' diverge, and the divergence presents as two bugs
instead of one"*, and `TASK-1019`'s check (1) is *"is there EXACTLY ONE derivation in the tree?"*.
⛔ **Stopping would have left the defect shipped. Copying would have created the second derivation.
Moving is the only outcome that satisfies both sentences** — the tree holds **one** derivation, now
with **two** consumers. **⛔ It is a MOVE, not a rewrite; if QA reads it as a second derivation, the
diff is the disproof.**

⛔ **WHY `USpellLibrary` AND NOT THE WIDGET:** the routing must not depend on a widget translation
unit, and `SpellLibrary` is already the **one delivery brain** (`GetEffectiveDelivery` and
`IsLineDeliverySpell` sit immediately above it). **Both consumers already `#include` it** —
`SiegePlayerController.cpp` at its existing `Siegebound/SpellLibrary.h` line, `DeckBuilderWidget.cpp`
because it already called `GetEffectiveDelivery`. ⛔ **No new include was added anywhere.**

---

## 1. ⛔ ALL THREE SITES REPAIRED, IN ONE DIFF — including the comment

Located **by symbol**, never by line (`SC-§38`; the predicate had already moved `:1037` → `:1090`
in a single day, byte-identical).

| # | Site | Was | Is |
|---|---|---|---|
| ➊ | `ASiegePlayerController::PlayHandSlot`, `case ECardType::Spell:` | `if (Row->SpellEffect == ESpellEffect::GoldSteal)` | `if (!USpellLibrary::SpellRequiresAiming(*Row))` |
| ➋ | `ASiegePlayerController::EnterTargetingMode`, head (before the hero-dead gate) | the **same** comparison, a second time | the **same** call, once |
| ➌ | `ASiegePlayerController::ResolveSpellInstant`, comment | *"only GoldSteal reaches this instant path"* | rewritten to the derived truth |

**⛔ ➌ IS PART OF THE FIX, NOT A COURTESY.** Repairing ➊ and ➋ and leaving ➌ would ship a confident
sentence explaining behaviour that no longer exists — ⚖️ the stale-prose class (`SC-§77`) arriving
inside the fix for a different defect, which this batch has already seen once.

**⚠️ AND ➌ IS ASSERTED WITH A COMMENT-AWARE SCANNER, WHICH MATTERS:** the house
`CountOccurrencesInCode` **skips comment lines by design** — it is **structurally blind** to a lie
that lives in prose and would have reported a serene zero over it. `SiegeSpellRoutingTest.cpp`
therefore carries a second, deliberately comment-*reading* counter used for **exactly that one row**.
⛔ **Consequence a reviewer must not undo:** the replacement comment **paraphrases** the old sentence
rather than quoting it, and says so in place — quoting it verbatim would re-fail the gate that
guards it.

### ⛔ Three MORE stale-prose sites the diff repaired, found by census rather than by instruction
My change is what made these false, so leaving them would be the same defect ➌ names:
* `SiegePlayerController.h` — `ResolveSpellInstant`'s doc said *"(M5 ruling 7 — GoldSteal/Pickpocket)"*.
* `SiegePlayerController.h` — `EnterTargetingMode`'s doc said *"a GoldSteal card never targets"*.
* `SiegePlayerController.cpp` — `ResolveSpellInstant`'s head comment and its `INDEX_NONE` note both
  named `GoldSteal` as the only arrival.
* `Tests/SiegeBrightSunTest.cpp` — two assertion **messages** said the targeting path is *"where
  `Fog` and `BrightSun` resolve today"* and *"a refused Fog/BrightSun"*. ⛔ **PROSE ONLY: both
  assertions, their counts and their ordering claims are BYTE-UNCHANGED.** That file's gate
  (`qa/TASK-986.md`) has **RETURNED**, so this is not a mid-review mutation — same direction the
  project ruled for `TASK-991` growing into `SiegeFogRefusalTest.cpp`.

### ⛔ Census after the change
`SpellEffect == ESpellEffect::GoldSteal` — **0 executable occurrences in the whole shipping tree.**
Two mentions survive in `SiegePlayerController.cpp` and both are **comment lines** quoting the guard
that must not return; the code-line counter skips them, and `SiegeSpellRoutingTest` TEST 3 pins the
tree-wide zero.

---

## 2. ⛔⛔ THE TRAP, AND WHY THE DIFF IS NOT `== GroundCircle`

⛔ `GetEffectiveDelivery`'s `Auto` arm returns **`GroundCircle`** for `GoldSteal`, `FogCover` **and**
`FogClear`. A guard written `== ESpellDelivery::GroundCircle` would print a reticle for **three**
cards — ⛔ **strictly worse than the blacklist it replaced, which at least got `Pickpocket` right.**
⭐ The structural reason is recorded in the new header doc: **`ESpellDelivery` has no value meaning
*"no aim at all"*.**

⛔ **TEST 1 IS SHAPED SO THAT BOTH WRONG REPAIRS GO RED ON ONE ASSERTION:** its `Auto` + zero-radius
row demands `false` for every non-line effect. `== GroundCircle` answers `true` there for **five**
effects; `!= GoldSteal` answers `true` there for **four**.

---

## 3. ⚖️ THE RIDER — everything the old path did, re-proven on the new one

### (i) Both fog refusal gates still fire — verified at source, then pinned
Both gate on `Row->SpellEffect` and `return` **before** the `switch (Row->CardType)`, which is itself
before the routing branch I changed. ⛔ **I edited nothing inside either gate.** New ordering
assertions (`SiegeSpellRoutingTest` TEST 5) measure `CardRefused_BrightSunActive` and
`CardRefused_BrightSunWouldShorten` as occurring **before** the new routing symbol — a claim that
**could not have been written before this diff** and that dies if anyone moves the routing above the
gates. ⛔ Paired with the negative control that the switch still **sits between** them, so a build
whose gates ate the function fails.

### (ii) Both refund sites remain reachable — and they are different call sites
* **Instant path** (`ResolveSpellInstant`): `AddGold(Row.Cost)` — **now the fog cards' fizzle site.**
  It is a **live** path for them, not theoretical: `ResolveSpell`'s two fog arms genuinely return
  `false` (no `AFogVolume` spawnable · `RaiseFog` refused under a live window, `J-F19` ·
  `ApplyBrightSun` refused a shortening cast, `J-F18`).
* **Targeting path** (`TryConfirmSpellTarget`): `AddGold(TargetingCost)` — **unchanged**, and it
  still carries every aimed spell (`Fireball`, `FrostNova`, `Lightning`, `BattleCry`). ⛔ A re-route
  that quietly emptied the lane it left would pass every other row in the file.
* Plus the second half of his net-zero ruling as a **separate** assertion: `ConfirmInstantDraw` sits
  **after** the refund-and-return, so a refused instant play cannot eat the card.

### ⛔ NO REFUND BUG WAS CHASED
`VID-006` proved the resolver already returns **true** for both cards. ⛔ I changed **nothing** in
`ResolveSpell`, `AFogVolume`, or any refusal. `NS_Spell_Fog` / `NS_Spell_BrightSun` are ⭐ `TASK-1024`'s
and need no code from me — the spawn call is already `SpawnSpellVFX(World, CardID, TargetPoint)`,
null-safe and log-once, and the instant path reaches it exactly as the targeted one did.

### ✅ The dead-hero consequence, confirmed rather than changed (board item 2b)
The hero-dead gate lives **after** site ➋'s early return, so an unaimed spell never reaches it.
⇒ **`BrightSun` is playable with a dead hero and yields the BASE prevention window** — `AFogVolume`'s
deliberate degradation governing, exactly as ruled. ⛔ Confirmed at source; **not** changed; recorded
in `EnterTargetingMode`'s header doc so the next reader does not "fix" it.

---

## 4. ⛔ THE SCHEDULED RED — re-pointed, never deleted

`Tests/SiegeFogRefusalTest.cpp` test 4, whose own comment instructed this:

| | needle | value |
|---|---|---|
| ⛔ **OLD** | `if (Row->SpellEffect == ESpellEffect::GoldSteal)` | **1** |
| ✅ **NEW** | `USpellLibrary::SpellRequiresAiming(*Row)` | **1** |

⭐ **THE CLAIM IS UNCHANGED — that is why it moves instead of dying:** *"the spell routing decision is
made in exactly ONE place inside this entry."* Only the **spelling** of that one place changed. ⛔ The
count stays **1, not 2**, and that is load-bearing: `EnterTargetingMode` holds the other call, but it
is a different function and therefore outside `PlayBody`.
⛔ **AND I ADDED THE PAIRED BAN ROW** (`+1` assertion): a re-point alone passes if somebody keeps
**both** — the derived call for the fog cards and the old list beside it for something else.

---

## 5. ⛔ THE INHERITED EDGE — `qa/TASK-1013.md` WARN-1, fixed in one line and pinned

**The defect:** an **authored `HeroLine` cell on a non-line-capable effect** printed the
**ground-circle** sentence. The selector carried an extra *"…and the effect is AoEDamage/Freeze"*
conjunct, so ⚖️ **the composer contradicted the cell in precisely the case whose whole rationale is
"agree with the data"**.

**The fix, one line:** the aiming-sentence selector is now `ResolvedDelivery == HeroLine`.
⛔ **I did NOT re-derive the predicate to do it** — item (2)'s fence held; this is the *"which
sentence"* selector, not the *"whether"* one.

⭐ **AND IT NOW AGREES WITH THE GAME, NOT ONLY WITH THE CELL:** that expression **is**
`USpellLibrary::IsLineDeliverySpell`'s question — the predicate `ASiegePlayerController`'s targeting
aim pass itself gates on. ⇒ the sentence he reads and the confirm behaviour he gets cannot disagree
for any row.

**⚠️ A SECOND, PROVABLE COLLAPSE CAME WITH IT — declared because it looks like a behaviour change and
is not.** `bLineCapableEffect` was then dead in the composer, because `bDeliversAsLine`'s only other
readers are `case ESpellEffect::AoEDamage:` and `case ESpellEffect::Freeze:` — **inside which
`bLineCapableEffect` is true by construction**, the switch having already established the effect.
⇒ the conjunction was a **tautology at both use sites**; dropping it changes no output for any row,
and it leaves the effect-mirror in **one** place instead of two.

**Pinned** in `Tests/SiegeCardGlossaryTest.cpp` beside its paired case (`+2` assertions, no new
case): the hero-line sentence must be **present** and the ground-circle sentence **absent** — the
second row is the half that actually goes red on the old behaviour.

---

## 6. Files touched

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.h` | `SpellRequiresAiming` declaration + the doc carrying the `== GroundCircle` trap and the `SC-§75`(B) rationale |
| `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp` | the derivation, **MOVED** verbatim from `DeckBuilderWidget.cpp` |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | consumes the predicate · the WARN-1 one-line fix · the tautology collapse. ⛔ **No player-facing string changed** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | sites ➊ ➋ ➌ + three more stale-prose repairs the change itself caused |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | two doc blocks that named `GoldSteal` as the only instant/no-target effect |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSpellRoutingTest.cpp` | ⭐ **NEW** — 5 cases |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` | the pin **re-pointed** + the paired ban row |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardGlossaryTest.cpp` | the WARN-1 pin (+2 assertions in the existing TEST 4) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBrightSunTest.cpp` | ⛔ **PROSE ONLY** — two assertion messages; counts and ordering byte-unchanged |

⛔ **Untouched, deliberately:** `SpellLibrary.cpp`'s resolver arms · `FogVolume.*` · both refusal
gates · `cards.csv` · `DT_Cards.uasset` · `Content/` · `Tests/SiegeSpellDeliveryTest.cpp`
(⭐ `TASK-1017`'s, not written) · `Tests/SiegeSpellVFXRosterTest.cpp` (⭐ `TASK-1025`'s).
⚠️ **File-name collision check done before writing:** `TASK-1017` claims `SiegeSpellDeliveryTest.cpp`
and `TASK-1025` claims `SiegeSpellVFXRosterTest.cpp` — mine is `SiegeSpellRoutingTest.cpp`, colliding
with neither.

---

## 7. Suite **DELTA** (⛔ never executed — declared)

**+5 test cases, +1 file.** Plus **+1** assertion in `SiegeFogRefusalTest`'s existing test 4 and
**+2** in `SiegeCardGlossaryTest`'s existing TEST 4 (**no new registered cases** in either).
⛔ **DELTA only; no absolute is published** (`TL-§5c`). Last executed by anyone: `475 / 0` at `1a457df`.

| case | claim |
|---|---|
| `…EveryDeliveryValueGetsTheRightRoutingAnswer` | quantified over `StaticEnum<ESpellDelivery>()` × `StaticEnum<ESpellEffect>()` — ⛔ never by naming today's cards (`SC-§65`) |
| `…TheNoReticleSpellShapesTakeTheInstantPath` | `FOG-§10.1` in **behaviour**, with four paired positive controls |
| `…TheRoutingConsumesThePredicateAndHoldsNoBlacklist` | one call per site · tree-wide blacklist zero · ➌'s sentence gone (comment-aware) |
| `…ThereIsExactlyOneDerivationOfHasAReticle` | `TASK-1019`'s check (1), as an executable census |
| `…BothRefusalGatesAndBothRefundSitesSurviveTheReRoute` | the rider |

### ⛔ RED-PROOF (`SC-§37`) — including the one that is honestly **weak**
* **TEST 1** goes RED against `== GroundCircle` (5 effects) and against `!= GoldSteal` (4 effects).
  ⚠️ **DECLARED HONESTLY: it does NOT go red against `TASK-999`'s own derivation, because this
  predicate IS that derivation, moved.** TEST 1 guards the **content**; **TEST 3** guards the
  **consumption**, and TEST 3 *is* red against the pre-fix controller (the call appeared **zero**
  times in both functions; the banned comparison appeared **twice**).
* **TEST 2**'s three `TestFalse` rows are paired with four `TestTrue` controls in the same case — a
  predicate returning `false` unconditionally would delete targeting mode from the game and still
  pass the negatives alone.
* **TEST 4**'s census self-checks that the scan is alive (`SC-§40`), so a dead instrument cannot read
  as a clean zero; every ordering probe in **TEST 5** fails on a missing marker rather than reporting
  "nothing after it" (`SC-§38`).
* **Glossary WARN-1 pin:** the *absence* row is the discriminating half — the old behaviour printed
  the ground-circle sentence and fails it.

---

## 8. ⛔ SEAMS AND JUDGEMENT CALLS — read these before passing

1. **The move vs. the fence** (§0). ⛔ The single most important thing to grade. If QA rules the
   predicate should have stayed in `DeckBuilderWidget.cpp` with a header declaration instead, the fix
   is a **relocation**, not a reshape — the body and all three call sites are unchanged.
2. **`bLineCapableEffect` still names two effects.** ⛔ It is **not** the blacklist this row killed,
   for `TASK-999`'s own reason: it **mirrors `ResolveSpell`'s branch set** and **fails closed**. It
   now exists in **one** file instead of two.
3. **`bDeliversAsLine` is deliberately NOT pinned by TEST 4's one-derivation census**, and the test
   says why in place: the composer legitimately keeps a local of that name to choose *which* wording,
   which is a different question from *whether* there is an aim.
4. **`SpellRequiresAiming` has no `SpellEffect == None` early-out**, deliberately. The glossary's own
   `None` early-return (a *"not a spell row"* guard) is untouched and still runs first; adding a
   second `None` rule inside the predicate would have changed routing for a malformed Spell row
   rather than merely moving code. A `None`-effect Spell row behaves **exactly as before**: it
   refuses net-zero at the resolver.
5. **Not compiled, not run, no engine, no MCP, no Git mutation** (read-only `status`/`diff` only).
   Nothing in this row has been executed by anyone.

---

# ⭐⭐ LOOP 2 — `qa/TASK-1019.md` FAIL (2 BLOCKERS) CLOSED (gameplay-programmer, 2026-09-04)

> ⛔ **APPENDED, NOT A REWRITE.** Everything above is the LOOP 1 record and still stands.
> ⭐⭐ **THE GATE UPHELD THE GAMEPLAY FIX — INCLUDING THE MOVE-VS-STOP DECISION — AND I RE-OPENED NONE OF IT.**
> ⛔ **EXACTLY ONE FILE CHANGED THIS LOOP:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSpellRoutingTest.cpp`.

## L2.0 — ⛔ What did NOT move, stated first because it is the load-bearing claim

⛔ **Byte-untouched this loop, verified by `git status`:** `SpellLibrary.h` · `SpellLibrary.cpp` ·
`SiegePlayerController.h` · `SiegePlayerController.cpp` · `DeckBuilderWidget.cpp` ·
`Tests/SiegeFogRefusalTest.cpp` · `Tests/SiegeCardGlossaryTest.cpp` · `Tests/SiegeBrightSunTest.cpp`.
They remain `M` from LOOP 1 with **no LOOP 2 edit**. ⛔ **No source, no assertion, no count, no needle
and no prose outside the one test file moved** — which is exactly the fence the gate set.

## L2.1 — ⛔ BLOCKER-1: the file-scope `using namespace` — **DELETED**

**Confirmed at source before fixing, with QA's own probe:** `grep -rn "^using namespace" Source/`
returned **exactly one hit — this file's line 399**, and `Tests/SiegeUnitNoticeRangeTest.cpp` really
does declare `LoadProjectFile` (`:108`), `CountOccurrencesInCode` (`:131`) and `ExtractFunctionBody`
(`:182`) with identical signatures inside its own **named** fixture (`:88`), calling them
**unqualified** under function-scope directives at `:216 :345 :501 :710 :890`. ⇒ the leak really
would have made **a file this row never touched** ambiguous in a shared unity blob.

**Fix:** the directive is **deleted**. In its place sits a declared-decision comment block explaining
why there is no directive here **in either form** — file scope *or* the house `RunTest`-body idiom —
so the next reader restores neither.

✅ **EVIDENCE (re-run after the edit):** `grep -rn "^using namespace" Source/` now returns
**0 hits tree-wide.** The only surviving `using namespace` text in the file is inside that comment
(`//`-prefixed), which the probe cannot match and no counter in the tree pins.

## L2.2 — ⛔ BLOCKER-2: it **does** survive the B-1 fix — closed **twice over**

**Confirmed at source:** `Tests/SiegeCardGlossaryTest.cpp:77` opens a genuine **anonymous** namespace
holding `GatherDeclaredSpellEffects` (`:157`) and `MinimumDeclaredSpellEffects` (`:190`). Anonymous-
namespace members sit at **global scope unconditionally**, so QA is right that removing my directive
does **not** clear them, and right that a function-scope directive would not either.

⛔ **I verified QA's preferred one-edit repair actually resolves B-2 rather than assuming it:**
a **qualified** lookup (`SiegeSpellRoutingFixture::Name`) is looked up *only* in the nominated
namespace — global scope is never consulted — so qualification does close it on its own. I then did
**both** anyway:

1. ⭐ **All 42 fixture references are now fully qualified** with `SiegeSpellRoutingFixture::`.
   Per-symbol post-edit census: **unqualified = 0 for all 18 symbols**, and the qualified counts match
   the pre-edit counts symbol-for-symbol (2·2·1·1·1·4·1·2·6·1·5·3·1·1·1·9·1·1) ⇒ **no reference was
   missed and none was invented.**
2. ⭐ **The two colliding names are renamed** to QA's own suggested spellings —
   `GatherDeclaredSpellEffects` → **`GatherRoutingSpellEffects`**, `MinimumDeclaredSpellEffects` →
   **`MinimumRoutingSpellEffects`** — which restores the tree-uniqueness rule `qa/TASK-1013.md`
   applied when these symbols were introduced, and which QA's BLOCKER-2 prose flagged this row for
   breaking (*"This row made `GatherDeclaredSpellEffects` a 2-file name"*).

✅ **EVIDENCE:** both names are back to **1-file** tree-wide — every remaining hit is in
`SiegeCardGlossaryTest.cpp`. The only mention in my file is inside the explanatory comment, in
backticks, where no probe can reach it.

⚠️ **Deliberately NOT renamed:** `GatherDeclaredDeliveries` / `MinimumDeclaredDeliveries`. They are
already unique tree-wide (censused), and renaming a name that never collided is churn. **That
asymmetry is declared in place** at both definitions so nobody "restores" it.

⛔ **Intra-namespace calls are deliberately left UNQUALIFIED** (e.g. `CountOccurrencesInCode(Text,
Needle)` inside `CountAcrossShippingSource`). Unqualified lookup from inside a namespace finds the
enclosing namespace's own member and **stops** — global scope is never reached — so those are immune
by construction, and qualifying them would be noise.

## L2.3 — ⛔ How I verified without a compile (the gate's three checks)

| QA's check | Result |
|---|---|
| File matches the siblings' placement convention | ✅ The tree now holds **0** file-scope directives (was 1 — mine). This file goes **stricter** than the 16 siblings (full qualification, not a function-scope `using`) because the sibling idiom fixes B-1 but **not** B-2 — and that deviation is **declared in place**. |
| Re-census the two names tree-wide | ✅ **1 file each** (`SiegeCardGlossaryTest.cpp`). **No unqualified collision remains at global scope.** |
| `SiegeUnitNoticeRangeTest.cpp`'s three functions no longer ambiguously reachable | ✅ My fixture's members can reach global scope **only** via a using-directive, and this file now has **none**. The leak path is gone at the source. |

⭐⭐ **AND THE PROOF THAT NOTHING ELSE IN THE FILE MOVED — the check I would want if I were the gate.**
I normalized both revisions (mechanically undoing the qualification and the rename) and diffed them.
**The only non-comment delta in the entire 841-line file is the deleted directive**, and an
executable-line diff (comments and blanks stripped) returns a count of **1**. ⇒ ⛔ **every assertion,
every needle literal, every expected count, every message and every fixture shape is byte-identical
to the revision the gate read and passed.**

Structural sanity also re-checked: **5** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros / **5** `RunTest`
bodies (unchanged), `#if`/`#endif WITH_DEV_AUTOMATION_TESTS` intact, brace counts **identical to the
pre-fix file** (56/57 — the odd `}` is the body-end marker string inside `ExtractFunctionBody`,
pre-existing and balanced), and no double qualification anywhere (**0**).

## L2.4 — Files touched this loop

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSpellRoutingTest.cpp` | ⛔ **THE ONLY ONE.** Deleted the file-scope using-directive (replaced by a declared-decision comment block); qualified all 42 fixture references; renamed the 2 colliding fixture symbols; added 2 locality riders at their definitions. |

⛔ **Nothing else.** No `.h`, no shipping `.cpp`, no other test, no data, no asset, no convention file.

## L2.5 — ⚠️ FOR THE COMMIT HOST: the file is **still NEW and UNTRACKED**

⛔ **`Tests/SiegeSpellRoutingTest.cpp` remains untracked** — re-confirmed this loop with
`git status --porcelain` (`??`) and `git ls-files --error-unmatch` (*"did not match any file(s) known
to git"*). ⛔ **It must be staged by explicit pathspec, or it commits as a SILENTLY ABSENT gate — the
suite goes green because the file is not there.**

⚠️ **Note the repo root is one level ABOVE the project directory** (`git rev-parse --show-toplevel`
= `C:/GitProjects/GitHub/GitClaudeUnrealTesting`), so the **repo-relative** pathspec is:

```
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeSpellRoutingTest.cpp
```

The other eight files in the row are pre-existing and already show as `M`.

## L2.6 — Suite **DELTA** (⛔ unchanged by this loop, ⛔ still never executed)

⛔ **`+5` registered cases in `+1` file**, plus `+1` assertion in `SiegeFogRefusalTest`'s existing
test 4 and `+2` in `SiegeCardGlossaryTest`'s existing TEST 4 (**no new registered cases** in either).
⛔ **LOOP 2 adds ZERO cases and ZERO assertions** — it is a compile-lookup repair only.
⛔ **DECLARED, NEVER EXECUTED.** ⛔ **No tree absolute is asserted here.**
⚠️ Last executed by anyone, **relayed not measured**: **`475 / 0` at `1a457df`**.

## L2.7 — Disposition of the non-blocking findings (⛔ none actioned, and why)

⛔ The dispatch fenced this loop to **the two blockers**, and QA graded the rest **WARN/NIT**. I read
every one and left them **deliberately**, rather than missing them:

- **WARN-1** (TEST 4 pins bare tokens; `SC-§41` cl. 1 deviation undeclared) — ⭐ **the cheapest real
  follow-up, and it is comment-only**: QA's own suggested fix is to declare the deviation in place as
  the file already does for the `bDeliversAsLine` exclusion. ⛔ Left out of scope this loop; **worth
  boarding.** QA re-derived all three counts as correct today, so nothing is red.
- **WARN-2** (the message says *"exactly ONE shipping translation unit"* while
  `CountAcrossShippingSource` returns a **tree-wide sum**) — a **real** imprecision; the fix changes
  **assertion text**, which is precisely what the gate told me not to move on the return trip. ⛔ Left
  for the gate to direct. **Worth boarding.**
- **WARN-3** (the LOOP 1 handoff prose drops a modal verb: *"both fog arms genuinely return `false`"*)
  — ✅ **QA is right and I accept the correction.** The site is **reachable, not always taken**. The
  test message beside it already says **CAN**. ⛔ I did **not** rewrite the original section (the
  dispatch forbids it); this paragraph is the correction, and RULING 2 in `qa/TASK-1019.md` records
  the resolution. ⛔ **No code defect, no regression** — casting `Fog` does not now refund where it
  previously resolved.
- **NIT-1** (paraphrase-sensitive needle) and **NIT-3** (claim duplicated with
  `Tests/SiegeBrightSunTest.cpp`; a cross-reference comment in each is the suggested fix) — ⛔ both
  left; boardable.
- **NIT-2** (`CardRow.h`'s *"reticle-placed"* doc) — ⛔ **explicitly out of this row's scope by QA's
  own ruling**; belongs to `TASK-1017`.
- **NIT-4** (the instant lane anchors VFX at the **hero's feet**, not the cursor) — ✅ correct and
  intended; ⛔ **`TASK-1024`'s VFX author must design for a hero-anchored spawn.** Relaying it.

## L2.8 — ⛔ Fences honoured

⛔ **No compile** (it is another row's gate — the first compile is still the first real test of these
two hazards) · ⛔ no engine · ⛔ no MCP · ⛔ **no mutating Git** (read-only `status` / `rev-parse` /
`ls-files` only) · ⛔ board edit confined to **`TASK-1018`'s own `status:` line**, re-located by
heading immediately before writing.
