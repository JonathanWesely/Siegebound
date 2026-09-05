# TASK-839 — handoff (gameplay-programmer) — **THE SAFE HALF ONLY**

**Date:** 2026-09-04 · **Status set to:** `ready-for-qa (SAFE HALF ONLY — actor half still blocked)`
**Gate:** `TASK-850` (rescoped: `839` + `840`) · **Ship host:** `TASK-987` (owns the wave's single compile)
**Compile / editor / MCP / Git:** ⛔ none. `QUIET-MODULE` in force. **Nothing here has been built.**

---

## 0. ⛔⛔ READ THIS FIRST — WHERE I DREW THE LINE, AND WHY IT IS **NOT** WHERE THE BOARD DREW IT

My dispatch cleared **only the timer half** and added one binding constraint the board row does not contain:

> *"the `AFogVolume` actor half is blocked on **absence** (the class does not exist yet), so **do not create it and do not stub it**."*

**That constraint is load-bearing, and honouring it moves the line.** The board's own five "premise-independent" items (row line: *"the 'fog is active until T' SCALAR state · `FogDurationSeconds` · the refresh-not-stack rule · the new `ESpellEffect` value · the match-reset clear"*) are **not** all premise-independent once `AFogVolume` is forbidden — **four of the five have no home without it.** Measured, item by item:

| row item | needs `AFogVolume`? | ⇒ landed? |
|---|---|---|
| **(1)** the *"fog is active until T"* scalar | ⛔ **YES** — the row's own item (1) says *"Ship `AFogVolume` … the authoritative state"*, and `FOG-§10.1` pins *"the SAME ONE authoritative fog-state object"* | ⛔ **NO** |
| **(2)** `FogDurationSeconds = 300.f`, `EditDefaultsOnly` | ⛔ **YES** — `EditDefaultsOnly` **requires reflection ⇒ a `UCLASS`**, and the only `UCLASS` this row owns is `AFogVolume` | ⛔ **NO** |
| **(3)** refresh-not-stack (`J-F16`) | ⛔ **YES** — it mutates the scalar in (1) | ⛔ **NO** |
| **(4)** ⭐ **ONE new `ESpellEffect` value** | ✅ **NO** | ✅ **LANDED** |
| **(5)** match-reset clears fog | ⛔ **YES** — it zeroes the scalar in (1) | ⛔ **NO** |
| **(0b)** invert `ReadFogState`'s final `return false` | ⛔ **YES**, **and blocked a second, independent way** — see §4 | ⛔ **NO** |

⇒ ✅ **THE LINE I DREW: I landed exactly the items that are buildable with `AFogVolume` absent, plus two adjacent honesty fixes inside my own fence. I declined every item whose home is `AFogVolume` rather than inventing a different home for the state.** Putting the scalar on `ASiegeGameState`/`ASiegeGameMode` or in a new subsystem would have contradicted `FOG-§6`'s M8 clause, `FOG-§10.1`, and `TASK-982` item (1) simultaneously, and would have been exactly the *"guessing where the line falls"* my dispatch forbids.

### ⛔⛔ THE ROW COLLISION THIS PRODUCES — **IT IS THE MANAGER'S TO RULE, NOT MINE**

**`TASK-982` DOES NOT BECOME DISPATCHABLE.** Its status line reads *"NOT dispatchable until `TASK-839`'s **TIMER HALF** has landed (**the fog-state object must exist to add a second scalar to**)"* and its item (1) is *"`AFogVolume` gains a **SECOND SCALAR**"*. **There is no first scalar to add a second to.** The board and my dispatch disagree about whether "the timer half" includes creating `AFogVolume`:

- **The board** says it does — its VALVE, `TASK-982`'s blocker, and item (1) all require it.
- **My dispatch** says it does not — *"do not create it and do not stub it."*

⇒ ⛔ **Seven rows stay blocked** (`982` → `983`/`989`/`991` → `986`/`990`/`992`), **exactly as they were before this dispatch.** ✅ **The one row this DOES unblock is `TASK-840`**, which the VALVE itself says *"needs **only the enum value**"* — and that it now has.

---

## 1. ✅ WHAT LANDED — THREE FILES, ALL INSIDE `names:`

### (A) `Source/GitClaudeUnrealTest/Siegebound/CardRow.h` — row item **(4)**, the one item that is genuinely premise-independent

- ⭐ **New value: `ESpellEffect::FogCover`**, **appended after `GoldSteal`**.
- ⛔ **THE NAME IS MINE AND NOTHING PINNED IT.** I searched `CONVENTIONS.md`, `TASKBOARD.md`, `qa/`, `handoffs/` and `cards.csv`: **no proposed spelling exists anywhere.** It follows the shipped family's noun-compound shape (`AllyBuff`, `GoldSteal`), and it is the string **`TASK-840` must write into its `SpellEffect` cell character-for-character.** 🧑 **If the manager wants a different spelling, it is a two-site rename (this enum + the `SpellLibrary.cpp` arm) and no data exists yet to migrate — say so before `TASK-840` writes the CSV cell, not after.**
- ⭐ **RECOMMENDED SIBLING, not implemented (it is `TASK-982` item (9)): `FogClear`** for `BrightSun`. Recorded only so the two are not overloaded onto one value (`FOG-§10.1` forbids that explicitly).

#### ⛔⛔ A SERIALISATION FINDING I DOCUMENTED IN PASSING — **AND IT IS A REAL ONE, NOT TIDYING**

`ESpellEffect` is a reflected `uint8` `UENUM` held as a `UPROPERTY` on `FCardRow`, **and `FCardRow` is the row type of the saved asset `/Game/Data/DT_Cards`.** ⇒ **inserting a value in the MIDDLE renumbers every later value and silently re-reads every saved cell as the next effect along.** No compile error, no log, no red test — the DataTable just starts resolving the wrong spells. **I appended, and I wrote the append-only rule into the enum's doc comment** so the next person adding `FogClear` does not insert it next to `FogCover` for tidiness. ⚠️ **This is a live hazard for `TASK-982` item (9) specifically.**

### (B) `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp` — the resolve seam, **refusing out loud**

One new arm in `USpellLibrary::ResolveSpell`'s dispatch switch: `case ESpellEffect::FogCover:` logs a `Warning` naming the absent state object and `return false;` (the caller then refunds, per the shipped contract).

⛔⛔ **WHY THIS IS NOT OPTIONAL, AND IT IS THE ROW'S OWN FAILURE PATH (2):** the row warns that the dangerous outcome is *"a green suite and a 50-gold fog card that renders and clamps nobody."* **Without an explicit arm, `FogCover` falls into `case ESpellEffect::None: default:`, whose message is `"row '%s' carries no resolvable SpellEffect"` — which is now FALSE**: the row *does* carry one; it simply is not implemented. A false log in a project that treats comments as law is a real defect (`SC-§65`). The explicit arm makes the absence **audible at the exact moment a player could be misled by it.**

⚠️ **QA — THIS IS THE ONE JUDGEMENT CALL IN THE DIFF.** It is in-fence (`SpellLibrary.cpp` is named on both the fence line and `names:`), it has a boarded caller (`TASK-840` ⇒ `SC-§40` cl. 2 satisfied), and it is **one contiguous hunk that reverts cleanly** if you rule it over-reach. **The arm is written to be INVERTED by the actor half, not deleted** — same idiom as test 8's row.

### (C) `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` — the stale *"exponent 2"* comment, **comment-only**

`:123` read `A default-constructed tuning IS the shipped tuning (609.6 / 304.8 / **exponent 2**)`. **The third term names a curve that no longer exists** — `TASK-981` replaced the `t^n` falloff with Beer-Lambert and retired `FogDensityExponent` to zero references (`FOG-§9.2`).

- ⭐ **I DELETED THE ENUMERATION RATHER THAN RE-TYPING IT WITH `FogTransmittanceAtCeiling`.** A transcribed copy of a tuning struct is precisely what went stale here once; `FOG-§9.4` already pins every one of those values in exactly one place.
- ⭐ I also marked the surviving `AFogVolume` mention **as an explicit forward reference with its measurement (0 declarations in `Source/`)**, so it cannot be read as describing the build.

#### ⚖️ WHY I FIXED IT RATHER THAN REPORTING IT UNOWNED (my dispatch offered both)
My safe half does not otherwise touch this file, so under the narrow reading it is "outside". **Under the substantive reading it is inside, and I took that one:** the claim is about the **DEFAULT TUNING**, not about the volume — it is premise-independent in exactly the way the row's safe list means. `SiegeCombatStatics.cpp` is **first on this row's `names:` line**, `TASK-981`'s out-of-scope hands it to `TASK-839` **by name**, and `TASK-980` — the only other claimant — **does not list it on its `names:` line and is not dispatched** (blocked on `978` + `979`). ⇒ **no live row holds it.** It is one hunk and reverts cleanly.

✅ **`SC-§65` CENSUS, claim-shape not number:** I swept every `exponent` / `FogDensityExponent` occurrence in shipping source. **`SiegeCombatStatics.cpp:123` was the ONLY stale one.** The three `SiegeFogStatics.h` mentions (`:41`, `:55`, `:252`) are `TASK-981`'s own *retirement* notes and are **correct as written — I did not touch them** (that file is `TASK-981`/`997`/`995`-fenced anyway).

---

## 2. ⛔⛔ THE `CoreRedirects` WINDOW — **IT IS STILL OPEN. THE ANSWER IS "NO".**

> ⛔ **`FSiegeFogTuning` did NOT become a member of any serialised object in this diff.**

**Measured, not assumed** — every `FSiegeFogTuning` reference tree-wide is a function parameter, a local, or a test fixture. **No `UCLASS` holds one as a `UPROPERTY`.** `FSiegeFogTuning` is itself a `USTRUCT(BlueprintType)` with `UPROPERTY(EditDefaultsOnly)` members (`SiegeFogStatics.h:107`), but **nothing instantiates it into a saved asset.**

⇒ ✅ **`qa/TASK-981.md` NIT-3's note remains CURRENT and is NOT yet dated: retiring a field on `FSiegeFogTuning` today still orphans NO serialized data and still needs NO `CoreRedirects`.**

⛔⛔ **AND THE WARNING TRANSFERS INTACT TO WHOEVER SHIPS THE ACTOR HALF:** the moment `AFogVolume` (or anything else serialised) holds an `FSiegeFogTuning` `UPROPERTY`, **that window shuts**, and retiring or renaming any field on it **does** need `CoreRedirects` or it silently drops a designer's saved value on load. **That row must date the note. This one could not, because it did not close the door.** *(`SC-§40` — an absence is a measurement, so it is stated rather than left silent.)*

⚠️ **A SECOND, SEPARATE SERIALISATION OBLIGATION DID ARRIVE TODAY, AND IT IS NOT THE ONE ANYONE WAS WATCHING:** `ESpellEffect` — see §1(A). It was *already* a live contract (`DT_Cards` saves it); this row is simply the first to add to it since. **Documented in-place.**

---

## 3. ✅ PINS — **NONE MOVED.** MEASURED WITH A COMMENT-SKIPPING INSTRUMENT, NOT A RAW GREP

`SC-§39` — I mirrored `SiegeFogClampFixture::CountOccurrencesInCode` exactly (skips lines whose trimmed form starts with `//`, `* `, `*/`, `/*`, or is bare `*`) and `ExtractFunctionBody` (signature → first column-0 `}`), then re-measured **against the working tree, after my edits**:

| pin | where | expected | ⇒ measured | verdict |
|---|---|---|---|---|
| `return false;` inside `ReadFogState` | test 8 (b) — the **DECISION** pin | `2` | **2** | ✅ ⛔ **NOT INVERTED — see §4** |
| `OutTuning = FSiegeFogTuning();` inside `ReadFogState` | test 8 (c) | `1` | **1** | ✅ |
| `ReadFogState(` **tree-wide, shipping** | test 8 (a) | `3` | **3** (`.cpp` ×2, `.h` ×1) | ✅ |
| `FSiegeFogStatics::EffectiveVisionRadius(` **tree-wide** | the `TASK-851` structural twin | `2` | **2** | ✅ |
| `ReadFogState(` inside the funnel body | test 8 (a2) | `1` | **1** | ✅ |
| `FogDensityAt(` inside the funnel body | test 9 (c) | `0` | **0** | ✅ |
| `EffectiveVisionRadius(` inside the funnel body | test 9 (c) control | `1` | **1** | ✅ |
| `ReadFogState` body length | test 8 self-check | `> 200` | **2366** | ✅ |
| `SpellLibrary.cpp`: `GatherHostileAgents(` / `GatherFriendlyAgents(` | funnel site 6 | `2` / `1` | **2 / 1** | ✅ |
| `SpellLibrary.cpp`: `FSiegeVisionQuery::SeeingFrom(` · `GatherTeamAgents(` · `FogDensityAt(` | fog + funnel | `0` each | **0 / 0 / 0** | ✅ |
| `ResolveAllyBuff(` precedes `ResolveGoldSteal(` | the funnel test's **slice boundary** | true | **true** | ✅ my arm is in `ResolveSpell`, outside the anon namespace |

⭐ **Why the comment edits are pin-safe rather than merely "probably fine":** every line I wrote in `ReadFogState` begins `//` when trimmed, so the house counter skips them entirely, and none contains `return false;`, `OutTuning = FSiegeFogTuning();` or `ReadFogState(`. **`FogDensityExponent` appears in my new comment and that is also invisible to the counter** — verified, not assumed.

⚠️ **`Cast<ASummonedUnit>(Candidate)` reads 3 in `SpellLibrary.cpp` and that is CORRECT, not a finding.** In `SiegeAcquisitionFunnelTest.cpp:691` that string is the **`LiveControlToken`** (the thing that must be *present*), not the scanned needle — the `0` beside it belongs to a different column. **Recording it because the table reads like the opposite at a glance and the next person will make the same misread I did.**

---

## 4. ⛔⛔ THE PIN I DID **NOT** INVERT — **AND IT IS BLOCKED TWO INDEPENDENT WAYS**

Item (0b) instructs: *invert `return false; == 2`, never delete it.* **I did not, and I could not:**

1. ⛔ **The inversion needs the state object.** `ReadFogState`'s job after inversion is *"iterate `AFogVolume`, take its active flag and its `FSiegeFogTuning`"*. With no `AFogVolume` there is nothing to iterate. An inversion without it would be a **lie in the opposite direction** — the seam would claim to consult a source that does not exist.
2. ⛔⛔ **THE FILE IS HELD BY A LIVE ROW.** `Tests/SiegeFogClampTest.cpp` carries **`TASK-979`'s live QA-loop-2 repair — a 278-line uncommitted diff, confirmed at my own instant.** The row's own **2026-09-04** fence (the newest text on it) says: *"It must ⛔ NOT touch … `Tests/SiegeFogClampTest.cpp` (`TASK-979`'s LIVE REPAIR)."*

⛔⛔ **AND THIS IS AN INTERNAL CONTRADICTION ON THE ROW ITSELF, which the manager should resolve rather than the next agent re-deriving it:**

- **2026-09-02** (line 14608): *"the timer dispatch **MUST include** `SiegeCombatStatics.cpp` + `Tests/SiegeFogClampTest.cpp` in its fence and **MUST invert test 8's pin**."*
- **2026-09-04** (line 14587, newer): *"It must **NOT** touch … `Tests/SiegeFogClampTest.cpp`."*

⇒ I honoured the **newer** text and the live-row evidence. **`SC-§62` clause (ii) fails outright here** — the file is owned by a dispatched row — so proceeding under a declared breach was not available to me either.

✅ **THE GOOD NEWS, STATED SO NOBODY PANICS: LEAVING IT AT 2 IS CURRENTLY *CORRECT*, NOT MERELY SAFE.** Test 8's assertion says *"the seam is not wired to a fog source yet."* **That is true, and my diff keeps it true.** The suite's honest row and the shipped behaviour still agree. ⛔ **The row goes WRONG the instant the actor half lands and does not invert it** — and that is the failure the row exists to catch.

---

## 5. ⛔ SEAMS LEFT FOR THE ACTOR HALF — **NAMED, SO NOBODY RE-DERIVES THEM**

| # | seam | where | what the actor half does to it |
|---|---|---|---|
| **S1** | `case ESpellEffect::FogCover:`'s **refusal arm** | `SpellLibrary.cpp`, in `ResolveSpell` | ⛔ **INVERT, do not delete** ⇒ stamp the expiry on the state object, **refresh-not-stack** (`J-F16`), `return true` |
| **S2** | `ReadFogState`'s **final `return false;`** | `SiegeCombatStatics.cpp` | the **ONE line** it replaces (row item (0b)) — **and it must invert test 8's `== 2` pin in the same diff** |
| **S3** | `ReadFogState`'s **forward reference** to `AFogVolume` | `SiegeCombatStatics.cpp`, my new comment | rewrite from *"will default to"* to *"does default to"* once the class exists |
| **S4** | `ESpellEffect::FogCover`'s *"the effect is not live yet"* paragraph | `CardRow.h` | ⛔ **must be struck when S1 inverts** — otherwise a shipped mechanic carries a comment saying it does not work (`SC-§65`) |
| **S5** | ⛔ **`FogDurationSeconds = 300.f`** | ⛔ **nowhere — NOT SHIPPED** | ⭐ **`TASK-982` item (8) says *"If `TASK-839` shipped `30.f`, this row corrects it."* ⛔ IT SHIPPED NEITHER. There is no constant to correct — there is one to CREATE.** ⚠️ **`TASK-840` is about to write `EffectDuration = 300` into `cards.csv` with NO code constant for it to match.** |
| **S6** | the two accessors `TASK-982` owes `TASK-989`/`991` | ⛔ nowhere | unaffected by this row either way — flagged only so `TASK-982` does not assume a base class exists to hang them on |

### ⚠️⚠️ S5 IS THE ONE THAT CAN BITE QUIETLY
`FOG-§9.4` pins `FogDurationSeconds = 300.f`, and `TASK-840` item (1) requires the CSV `EffectDuration` to **match it**. **With no constant in code, `300` will exist in exactly one place — a CSV cell — and the "must match" contract will have nothing to match against.** That is a cross-file agreement with **only one party present.** 🧑 **Manager: either dispatch the actor half, or explicitly license `TASK-840` to ship `300` unmatched and record that the pin is owed.**

---

## 6. 📌 SUITE COUNT — `TL-§5c`

> ⭐ **445 tests across 34 files — DECLARED, ⛔ NOT EXECUTED.** **Delta from this row: `0`.**

✅ **Reconciles exactly with the other two lanes' `445 / 34`.** Counted as `IMPLEMENT_*_AUTOMATION_TEST` at column 0 across `Siegebound/Tests/*.cpp`. **Nothing in this batch has been compiled or run; `TASK-987` owns the wave's single compile and the first execution.**

### ⛔ WHY I ADDED **ZERO** TESTS, AND IT IS A FENCE DECISION RATHER THAN AN OMISSION
**`TASK-839`'s spec contains no test item** — items (0)–(5) and (0b), none require one. Every natural home is held:

- `Tests/SiegeFogClampTest.cpp` ⇒ ⛔ `TASK-979` live · `Tests/SiegeFogTest.cpp` ⇒ ⛔ `TASK-981` + `TASK-995`'s coordinate fence · `Tests/SiegeCardRosterTest.cpp` ⇒ ⛔ **dirty, 306-line live diff, owned by the `TASK-946`…`949`/`964` roster lane** (measured from its diff, not guessed).
- A **new** test file would exceed `names:`. **`SC-§62` clause (i) fails** — no spec item becomes unsatisfiable and no build breaks without it ⇒ the breach would be unforced, which is a BLOCKER by the law's own terms. **So I did not take it.**

⛔ **DECLARED OWED, with the assertions written out so the actor half or `TASK-850` can just take them:**
1. `static_cast<uint8>(ESpellEffect::GoldSteal) == 5` **and** `static_cast<uint8>(ESpellEffect::FogCover) == 6` — pins the append-only contract §1(A) documents but does **not** enforce.
2. `case ESpellEffect::FogCover:` appears **exactly once** in `SpellLibrary.cpp` — one resolve arm, never two.
3. ⛔ The honest row, in test 8's idiom: **that arm still refuses** ⇒ **INVERT when S1 inverts.**

---

## 7. ⚠️ FINDINGS THE BOARD **AND** MY DISPATCH BOTH MISSED

1. ⛔⛔ **`DeckBuilderWidget.cpp`'s card glossary has no `FogCover` arm** (`:1342`'s switch falls to `default: break;`). ⇒ **the `Fog` card will render in the deck builder with NO effect line at all** — a blank where every other spell describes itself. **Compile-safe** (the switch has a `default:`), **but it is a visible content gap the moment `TASK-840` lands the row.** ⛔ **The file is in NO row's fence and NO `names:` line — I did not touch it. It needs a row.** *(The same gap will hit `BrightSun` via `TASK-983`, so one row can fix both.)*
2. ✅ **All three `ESpellEffect` switches carry `default:`** (`SpellLibrary.cpp:635`, `SpellLineSweep.cpp:256`, `DeckBuilderWidget.cpp:1395`) ⇒ **adding an enum value cannot produce a `-Wswitch` error.** Checked deliberately **because I cannot compile** and that is the one class of break a new enum value causes.
3. ✅ **No test anywhere references `ESpellEffect`** — zero hits across all 34 test files ⇒ the new value moves no test-side count. *(This is also why finding 1 is invisible to the suite.)*
4. ⛔ **The row's fence and its `names:` line disagree about `Tests/SiegeFogClampTest.cpp`** — see §4. **Manager's to reconcile.**
5. ⚠️ **`TASK-995`'s coordinate fence does NOT cover `SiegeCombatStatics.cpp`** (it names only `SiegeFogStatics.h` and `Tests/SiegeFogTest.cpp`). ⇒ my §1(C) comment edit **is not a `TASK-995` fence breach.** Stated because my dispatch flagged that fence at me.
6. ⚠️ **`CardRow.h` was already dirty at my instant with `TASK-979`'s `FCardRow::NoticeRange` hunk** — re-confirmed per the row's own `SC-§55` instruction. **It is not mine, I did not touch it, and it will appear in this file's diff.** ⛔ **Reviewers: `git diff CardRow.h` shows TWO authors' work.**
7. ⚠️ **`ESpellEffect::FogCover`'s name is unpinned by any law** — see §1(A). If the manager wants a different one, **now is free; after `TASK-840` writes the CSV cell it is a data migration.**

---

## 8. ⛔ EXPLICITLY **NOT** DONE
⛔ `AFogVolume` — **not created, not stubbed** (per dispatch) · ⛔ the fog-active scalar · ⛔ `FogDurationSeconds` · ⛔ refresh-not-stack · ⛔ the match-reset clear · ⛔ `ReadFogState`'s inversion · ⛔ test 8's pin · ⛔ the `Static`-mobility runtime spawn (**still UNPROVEN and still OWED** — `VIS-§4`; **no PIE, no pixels, and `TASK-858` still gates any pixel read**) · ⛔ `BrightSun` in any form (`TASK-982`/`983`/`989`/`991`) · ⛔ `cards.csv` (`TASK-993`) · ⛔ `SummonedUnit.*` (`TASK-979`) · ⛔ `SiegeFogStatics.*` (`TASK-981`) · ⛔ any compile, editor, MCP or Git.

---

## 9. 🔍 WHAT QA SHOULD SCRUTINISE HARDEST
1. ⛔⛔ **§0's line-drawing.** Is *"do not create `AFogVolume`"* + *"land the timer half"* satisfiable at all? **I concluded it is only ~1/5 satisfiable and said so instead of inventing a state home.** If you disagree, the disagreement is with the dispatch, not the diff.
2. ⛔ **§1(B), the `SpellLibrary.cpp` refusal arm** — the only judgement call. In-fence, boarded caller, one revertible hunk.
3. ⛔ **§1(C), the comment fix** — I fixed rather than reported. Reasoning is in §1(C); overrule it in one hunk if you read *"falls inside your safe half"* narrowly.
4. ✅ **§3's pin table** — re-measure it. **I mirrored the house counter rather than grepping** (`SC-§38a`), and the `Cast<ASummonedUnit>(Candidate)` note is there because that table misreads easily.
5. ⛔ **§5's S5 and §7's finding 1** — **two live gaps that will surface in `TASK-840`, not here.**
