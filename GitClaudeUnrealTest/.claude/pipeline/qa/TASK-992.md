# QA Report — TASK-992
**Gate over:** `TASK-991` ALONE (`SC-§29` ledger below) · **Subject handoff:** `handoffs/TASK-991-programmer.md`
**Cited, ⛔ NOT re-reviewed:** `qa/TASK-990.md` (PASS · 0 BLOCKERS · 4 WARN · 4 NIT) — `TASK-989`'s half is **closed** and I do not re-open it.
**Verdict: PASS** — ⛔ **0 BLOCKERS** · 4 WARN · 5 NIT
**Date:** 2026-09-04 · **Reviewer:** qa-reviewer

---

## §0 — `SC-§39`: MY INSTRUMENTS, DECLARED ABOVE THE FINDINGS, CONTROLLED BOTH WAYS

**Had:** `Read` · `Grep` · `Glob` · scoped `Edit` (this report + `TASK-992`'s own `status:` line).
**Did NOT have:** ⛔ `Bash` (`SC-§78`) ⇒ ⛔ **zero Git**, ⛔ zero compile, ⛔ zero suite execution, ⛔ zero engine/MCP.

⇒ ⛔⛔ **EVERY `HEAD`-vs-tree, tracked-vs-untracked and "byte-untouched" claim in the handoff is ACCEPTED AS DECLARED, ⛔ NEVER MEASURED — and that explicitly includes the staging warning in §7 below, which is the single most consequential claim I am forwarding without measuring it.** I read the **working tree**; I cannot distinguish "this row wrote it" from "it was already there". Every finding is a claim about the **shipped bytes as they now stand**, which is what a compiler will see.

**Instrument controlled in BOTH directions (a zero from a dead scanner is the failure mode):**
- **Positive control:** the needle `FMath::Max` returns **7** hits in `SiegePlayerController.cpp` (`:4063, :4501, :4993, :5002, :5308, :5321, :5520`) ⇒ the scanner finds that exact token in that exact file when it is there. **The zero I report inside `PlayHandSlot` is therefore a finding, not a blind scan.**
- **Positive control 2:** `GetBrightSunWindowSeconds` returns hits in **three** files (`FogVolume.h`, `FogVolume.cpp`, `SiegePlayerController.cpp`) ⇒ the seam is reachable by needle.
- **Negative control:** the four-tunable needle set (`BrightSunBaseDurationSeconds` · `BrightSunBonusSecondsPerStep` · `BrightSunHeightStepUU` · `ArenaGroundReferenceZUU`) returns **hits in `FogVolume.{h,cpp}`** and **`0 total occurrences` in `SiegePlayerController.cpp`** ⇒ the anti-duplication zero is measured, not assumed.
- ⚠️ **My `Grep` cannot skip comments; the shipped `CountOccurrencesInCode` can.** Wherever the two could disagree I **read the hit** and classified it by hand (`SC-§38a`). Every such case is named inline below.

---

## §1 — ⛔⛔⛔ THE RULING'S ONLY VISIBLE INSTRUMENT: THE `FMath::Max` CENSUS — **RE-DERIVED MYSELF, AT ZERO**

🧑 `J-F18` is **longer RESETS, shorter REFUSES** — ⛔ a **branch**, not a policy. ⛔ `max(remaining, new)` and *"reset if longer"* return the **identical remaining time in the longer case**, so **no duration test in existence can tell them apart**; they diverge only in the shorter case, where `max` silently keeps the timer **while billing the gold and eating the card**. ⇒ **the census IS the guard, and a weakened census ships the ruling with no guard at all while the suite stays green through its deletion.**

| # | claim | verdict | evidence (line numbers mine, from the tree) |
|---|---|---|---|
| 1 | `PlayHandSlot`'s extent | ✅ **ESTABLISHED FIRST** | `void ASiegePlayerController::PlayHandSlot(int32 Slot)` at **`:892`**, column-0 `}` at **`:1205`** (read, not inferred) |
| 2 | ⛔⛔ `FMath::Max` **at ZERO across the whole entry** | ✅ **CONFIRMED — 0 hits in `892-1205`** | the file's 7 occurrences are **all** at `:4063` or beyond ⇒ **not one inside the entry.** Positive control above proves the needle lives. |
| 3 | the assertion that holds it | ✅ **PRESENT AND CORRECTLY SCOPED** | `Tests/SiegeFogRefusalTest.cpp:833-835` — `CountOccurrencesInCode(PlayBody, TEXT("FMath::Max")), 0` over the **whole `PlayHandSlot` body**, ⛔ not the narrow guard region. **This is the right scope: a `max` written anywhere in the entry is caught.** |
| 4 | the **state-object half** is a different assertion, not a duplicate | ✅ **CONFIRMED** | `Tests/SiegeBrightSunTest.cpp:659-664` pins `FMath::Max == 0` inside **`AFogVolume::ApplyBrightSun`**'s extracted body (⭐ `TASK-982`, gate `TASK-986` **returned PASS**). ⛔ **Different function, different introduction site** — a `max` could be written at either end and only one of the two rows would see it. **Not one duplicated assertion.** |

⚖️ **RULING: the census is present, at the correct scope, at zero, on BOTH sides of the seam, and I re-derived the entry-side zero independently of the author's word.** ⛔ The ruling ships **with** its guard.

---

## §2 — ⛔⛔ THE ROW'S DEFINING REQUIREMENT: THE EFFECT IS **REUSED**, ⛔ NEVER REIMPLEMENTED — **CENSUS RE-DERIVED, ALL SEVEN NEEDLES**

This is the first message in the game that must **compute a full card effect purely to explain why it refuses to apply it.** ⇒ the computation **must be shared with the apply path**, or the message and the effect diverge the first time either is retuned. I re-derived the whole census rather than accepting it.

| needle | claimed | **my count in `SiegePlayerController.cpp`** | note |
|---|---|---|---|
| `GetBrightSunWindowSeconds(` | **1** | ✅ **1 in code** (`:1119`) | `:1081` is a comment, `:33` matched a *different* needle in my sweep — both read and classified by hand |
| `FMath::FloorToFloat` | **0** | ✅ **0** | the `floor` at the heart of the formula |
| `FMath::FloorToInt` | **0** | ✅ **0** | no integer spelling either |
| `AFogVolume::BrightSunWindowSeconds(` (the **pure static**) | **0** | ✅ **0** | the controller asks the **actor**, which samples the hero — never the raw formula |
| `BrightSunBaseDurationSeconds` | **0** | ✅ **0** | `Grep` reports **`0 total occurrences`** for all four tunables as one pattern |
| `BrightSunBonusSecondsPerStep` / `BrightSunHeightStepUU` / `ArenaGroundReferenceZUU` | **0** | ✅ **0** | ⇒ **not one hand-typed `120`/`60`/`1524`/datum in this file** |
| `ApplyBrightSun` / `FogPreventedUntilTimeSeconds` | **0** / **0** | ✅ **0 in code** (`:1070` is a comment, read and classified) | ⛔ **nothing in the entry writes an expiry** |

**And I verified the seam itself rather than taking its contract on trust** (`FogVolume.h:251-270`, `FogVolume.cpp:254-292`):

- `float AFogVolume::GetBrightSunWindowSeconds(ETeamId CasterTeam) **const**` — declared **before** the `protected:` at `:295` ⇒ ⛔ **genuinely public**, ⛔ genuinely `const`.
- **Side-effect-free, read at source:** a `GetWorld()` null-check, a `TActorIterator<AHeroCharacter>` with a team + `!IsDead()` filter, one `GetActorLocation().Z` read, then a `return` of the pure static. ⛔ **No assignment to any member, no spawn, no broadcast.**
- ⛔ **IT RE-SAMPLES HEIGHT ON EVERY CALL** — the iterator runs per call ⇒ **`Y` changes as he climbs.** This is what makes the message worth showing.
- ⭐ **The formula exists exactly once in the project** (`FogVolume.cpp:294-324`) and both consumers reach it through the same accessor.

⚖️ **RULING: `Y` is the effect, computed and thrown away — ⛔ NOT a second implementation of it.** The card is never cast to find out whether to cast it. ⭐ **This is the difference between reusing the effect and reimplementing it, and the diff is on the correct side of it.**

---

## §3 — ⚖️ THE TWO DECLARED CALLS — **MY RULING ON EACH**

### ⚖️ 1. **NO EXTRA `RemainingWindowSeconds > 0.f` PRE-GATE** ⇒ ✅ **UPHELD — but the justification offered is NARROWER than claimed, and I am saying so rather than laundering it.**

**First, the identity itself — verified at source, not accepted:**

| | entry (`SiegePlayerController.cpp:1118-1121`) | state object (`FogVolume.cpp:140-153`) |
|---|---|---|
| operand A | `GetBrightSunWindowSeconds(CasterTeam)` | `GetBrightSunWindowSeconds(CasterTeam)` |
| operand B | `GetFogPreventionSecondsRemaining()` | `GetFogPreventionSecondsRemaining()` |
| shape | `if (WouldBeWindowSeconds < RemainingWindowSeconds)` | `if (NewWindowSeconds < RemainingSeconds)` |

⇒ ✅ **Same two accessors, same operand order (new < remaining), same strict `<`.** Only the **local names** differ — and that difference is deliberate and load-bearing: it is why `TASK-989`'s exact-string local-binding pins stayed at **1** instead of needing a re-point (see §5).

**Now the part I will not wave through.** ⛔ **`qa/TASK-990.md`'s reasoning RESEMBLES this one; it does NOT fully transfer, and the distinction matters:**

- `qa/TASK-990.md` §1 established that `GetFogPreventionSecondsRemaining() > 0.f` and `IsFogPrevented()` are *"the SAME predicate over the SAME two scalars"* ⇒ substituting one read for two is free of behaviour change **and free of the window, because there is only one time.** That is a **TEMPORAL** argument, made **inside one function**, and it is airtight there.
- This row's claim is **SPATIAL**: two textually identical predicates in **two different functions**, evaluated at **two different instants**. Textual identity buys agreement **given identical inputs**; it buys **nothing at all** about agreement **in time** — and the author's own §4 proves it: under today's targeting-mode routing the click and the confirm are seconds apart, the guard can pass and `ApplyBrightSun` can then refuse, and the player gets the generic *"Spell fizzled"*.

⇒ ⚖️ **The decision is RIGHT; the reason offered is only half of why.** The load-bearing reason is the one the author states second and understates: **any extra term in the entry predicate is a pure divergence source.** I checked what the pre-gate would actually change, and the answer decides it:

- With the shipped tuning `Y ≥ BrightSunBaseDurationSeconds` **always** (`FogVolume.cpp:302-323` — the bonus term is `≥ 0` by the `FMath::Max(0.f, …)` clamp), and `X` is clamped at `0` (`:251`). ⇒ `Y < X` is **unreachable** when no window is up ⇒ **the card falls through and plays normally, with or without the pre-gate.**
- The **only** region where the pre-gate changes anything is degenerate tuning (a non-positive base). There, **without** it both halves refuse **together**; **with** it the entry would wave through a cast the state object then refuses — i.e. **the pre-gate's sole effect is to CREATE the disagreement it looks like it prevents.**
- ⛔ **It is therefore not a missing safety check.** A `> 0.f` pre-gate here would be a **performance** micro-win (see NIT-3), never a correctness one, and it would be paid for in exactly the failure mode this design exists to exclude.

⚖️ **UPHELD. This is the shape I would have required.** ⚠️ See **WARN-1** — the identity the whole ruling rests on is **not pinned by any executable assertion**, and that is the real finding here.

### ⚖️ 2. **EQUAL RESETS** (strict `<`) ⇒ ✅ **THE READING IS RIGHT, AND ⛔ IT DOES NOT NEED HIS WORD TO SHIP.**

🧑 His sentence: *"UNLESS that new time would be **LESS** than the current time."* **Strict `<` is that sentence, read literally.** A reviewer preferring `<=` would be **inventing a rule he did not write**, which is the larger error. ⇒ ⛔ **Ruled correct, and it ships as-is.**

**Saying it plainly, as the row demanded rather than leaving it implied:**

1. ⛔ **It does NOT need Jonathan's word.** It **is** his word. The author correctly labelled it a *declared default* because the *equality case itself* was never addressed — but the default lands on the literal reading, which is the only defensible place for it to land.
2. ⭐ **It is a statement of intent, ⛔ not a behaviour — and here is the arithmetic behind that.** `X` is a continuous countdown (`expiry − now`, `FogVolume.cpp:250`); `Y` sits on a **coarse 60-second lattice** (`120`, `180`, `240`, …). Exact float equality between a free-running clock difference and a lattice point is **measure-zero — effectively unreachable.** ⇒ **no player will ever observe this branch.**
3. ⚠️ **But it is not purely cosmetic, and that is worth one line for his eye.** *If* equality ever did occur, "reset" costs the player **60 gold and the card for zero benefit**, while "refuse" keeps both. ⛔ **That is precisely the arithmetic-right / economics-wrong shape `J-F18` itself was created to fix** — which is why the row was right to declare it rather than bury it.
4. ⚠️⚠️ **THE ONE THING HIS EYE SHOULD ACTUALLY KNOW: flipping it is a TWO-CHARACTER change in TWO FILES, and they must move TOGETHER** (`SiegePlayerController.cpp:1121` **and** `FogVolume.cpp:153`). Moving one alone re-creates the entry/state-object disagreement the no-pre-gate decision exists to prevent. ⇒ ⛔ **Checkpoint line, ⛔ never a QA loop, ⛔ never a blocker** — and see **WARN-1**, because today only *one* of those two sites is guarded by a test.

---

## §4 — THE BOARD'S NUMBERED "ALSO VERIFY" ITEMS, EACH RE-DERIVED

**(1) ⛔⛔ THE PREDICATE PINNED AT 1 IN-REGION, AND THE ROUTING SWITCH **STILL FOLLOWS** THE MESSAGE.** ✅ **BOTH CONFIRMED — and the fall-through is real.**
- `Tests/SiegeFogRefusalTest.cpp:822-825` — `if (WouldBeWindowSeconds < RemainingWindowSeconds)` at **1** inside `ExtractSunOnSunRegion`'s output (`:229-241`: everything between the `FogClear` effect gate and the `CardRefused_BrightSunWouldShorten` message). ⛔ **An unconditional refusal deletes that substring and dies here, having passed every other row in the file.**
- `:847-852` — `SwitchIndex > MessageIndex`, both `!= INDEX_NONE`. ⛔ **The `switch (Row->CardType)` still follows.**
- ⭐ **I confirmed the fall-through at source, not just at the pin:** the guard's `return` is at **`:1135`, INSIDE the `if`**; on a **longer or equal** window nothing returns, execution reaches `PlaySound2D(this, CardPlaySoundPath)` at **`:1142`** and the routing `switch` at **`:1144`**. ⇒ ⛔ **A longer window casts normally. The refusal did NOT swallow the reset case.**
- ⭐ **And the region probe fails LOUDLY** (`:233-237`, `AddError` + `return false`) — a degraded probe returning an empty string would have reported every *"must contain"* row red and every *"must not contain"* row **green**, i.e. passed the ban rows while testing nothing (`SC-§38`). ⛔ **It cannot do that.**
- ⚖️ **On the board's item (1) demand that the negative control *"assert the expiry MOVED"*:** the expiry write is `AFogVolume::ApplyBrightSun`'s, and `TASK-991` is **forbidden** from re-implementing it (its own item (1)); no test in this project can execute it (**there is not one `SpawnActor` anywhere in `Siegebound/Tests/`**). ⇒ **the demand is met in the only form available, and it is met on both sides:** the entry proves it **SKIPS** and writes **no** expiry (`ApplyBrightSun` 0, `FogPreventedUntilTimeSeconds` 0), and `Tests/SiegeBrightSunTest.cpp:607-615` proves the state object stamps the prevention deadline **exactly once**, `=` and never `+=`. ⛔ **The control is PRESENT. Not a blocker.** The unexecuted half is **NIT-4**, recorded so the ledger stays honest.

**(2) ⛔ `WholeSecondsText` REUSED, ⛔ NOT COPIED — and the message left UN-GENERALISED.** ✅ **CONFIRMED BY TREE-WIDE CENSUS.** One tree-wide grep for `WholeSecondsText` across `Siegebound/`:
- **1 declaration** — `SiegePlayerController.h:945` (`static FText WholeSecondsText(float Seconds);`)
- **1 definition** — `SiegePlayerController.cpp:5494`
- **3 calls in `PlayHandSlot`** — `:1055` (the sibling's one value) + `:1133` + `:1134` (**mine, twice**)
- ⛔ **Zero other definitions anywhere in the module.** A second formatter would have been an automatic fail; there is not one.
- ⭐ **Un-generalised, proven structurally rather than asserted:** **both** wordings still live **inline at their own call sites with their own `NSLOCTEXT` keys** — `CardRefused_BrightSunActive` at `:1054` (one value) and `CardRefused_BrightSunWouldShorten` at `:1132` (two). ⛔ **No optional-second-value builder exists**, because there is no builder at all. Pinned at `:1145-1166`.
- ⭐ **The formatter itself is untouched and still card-blind** (`:1176-1181`: `CardRefused_` 0, `BrightSun` 0 inside its own extracted body) — which is exactly what let this row consume it without inheriting the sibling's wording.

**(3) ⛔ NOTHING CACHED — both values read at click time.** ✅ **CONFIRMED, AND I HAND-CLASSIFIED THE HEADER HITS.**
`SiegePlayerController.h` returns only **two** hits for the needle set `PreventionSeconds|FogPrevent|Remaining|WouldBeWindow|BrightSunWindow`: **`:514`** and **`:527`**. ⛔ **I read both: they are `*`-prefixed lines inside the `PlayHandSlot` doc block** (the shipped `CountOccurrencesInCode` skips them; my raw grep cannot, so I classified by eye — `SC-§38a`). ⇒ ⛔ **ZERO stored remainders, ZERO stored windows, zero `static`, zero `mutable`, zero member fields.** Pinned at `:417-424` and `:998-1003`. And ⛔ **nothing writes an expiry** — `ApplyBrightSun` and `FogPreventedUntilTimeSeconds` both **0 in code** in the whole controller (§2).

**(4) ⭐⭐ THREE PINS RE-POINTED, ⛔ NONE DELETED — GRADED AS CONDUCT.** ✅ **ALL THREE VERIFIED IN PLACE, EACH WITH ITS OLD VALUE AND ITS REASON.**

| # | site | pin | old ⇒ new | my check |
|---|---|---|---|---|
| 1 | test 2, `:383-394` | `GetFogPreventionSecondsRemaining()` in `PlayBody` | **1 ⇒ 2** | ✅ my own census: `:1044` + `:1118` = **2** in `892-1205`. The comment states the old value, the new value, and that the claim is *"ONE LIVE READ PER REFUSAL, and no second, older read"* — ⛔ **never "one read in the file"**, which was only that claim's arithmetic with one refusal on the board. |
| 2 | test 2, `:395-397` | same needle, file-wide | **1 ⇒ 2** | ✅ same two hits file-wide — **no third, older read anywhere** |
| 3 | test 5, `:652-657` | `RefuseCardPlay(CardID, FText::Format(` in `PlayBody` | **1 ⇒ 2** | ✅ `:1053` + `:1131` = **2**. Claim: *"every prevention refusal rides the shipped formatted-refusal idiom"* — two refusals, both ride it. |

⚖️ **RULING — and the row asked me to grade this as conduct, so I do, explicitly:**
⭐⭐ **This is `qa/TASK-990.md` WARN-2's re-point-never-delete discipline applied by a PROGRAMMER, UNPROMPTED, to a file whose gate had already passed — i.e. at the exact moment the incentive runs the other way.** A row that had simply deleted the three inconvenient pins would have shipped a **greener** suite and nothing would have failed.
⭐ **And it did not weaken while re-pointing, which is the part that separates discipline from convenience.** It kept **exact** counts (never relaxing to a `>=`), and it gave its locals **different names** (`RemainingWindowSeconds` vs the sibling's `PreventionSecondsRemaining`) **specifically so the sibling's exact-string local-binding pins at `:402-407` stayed at `1` and did not need touching at all.** ⇒ **only what the diff genuinely changed moved.** ⛔ **I checked each re-point preserves the claim rather than weakening it. All three do.**
⭐ **`TASK-989`'s three deliberately-unwritten pins all held**, exactly as predicted (`:608-615`): the `FogClear` gate is now pinned at **1 in test 7** — an assertion about *this* refusal rather than a stale zero left in test 4 — and the `ESpellEffect::` total is still, correctly, unpinned (`TASK-1018`'s).

**(5) ⛔ THE `GoldSteal` PREDICATE — BYTE-IDENTICAL, UNTOUCHED, AND STILL SCHEDULED TO GO RED.** ✅ **ALL THREE CONFIRMED.**
`if (Row->SpellEffect == ESpellEffect::GoldSteal)` occurs **once** inside `PlayHandSlot`, now at **`:1169`** (`:3244` is a different function). ⚠️ **It has MOVED again — `:1037` ⇒ `:1090` ⇒ `:1169`, pushed down by two insertions — but the TEXT is byte-identical and the count is 1.** The line number changed; the code did not (`SC-§77`: the author wrote **no** line numbers into any source file — every reference in the diff is by key or symbol, which I confirmed). Test 4's pin at `:604-606` is intact, **and so is its re-point instruction to `TASK-1018` at `:600-603`.** ⛔ **It remains the pin scheduled to go red there.** See **WARN-2**.

**(6) ⛔ SUITE Δ `+4`, DECLARED, NOTHING EXECUTED, NO ABSOLUTE.** ✅ **RE-COUNTED AT SOURCE.** `Tests/SiegeFogRefusalTest.cpp` holds **10** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros and **10** distinct `"Siegebound.FogRefusal.…"` names; `qa/TASK-990.md` counted **6** for `TASK-989` ⇒ **Δ = +4**, in tests 7-10. All four class names (`FSiegeSunOnSunConditionalTest`, `FSiegeSunOnSunLiveWindowTest`, `FSiegeSunOnSunNetZeroTest`, `FSiegeSunOnSunFormatterReuseTest`) return **exactly one file** on a module-wide grep ⇒ **unique, no ODR collision.** ⛔ **No tree absolute is claimed anywhere in the handoff, and I executed nothing** (§0).

---

## §5 — THE THREE PROPERTIES, AND THE ARGUMENT ORDER

**⛔ ZERO GOLD · ⛔ CARD NOT CONSUMED · ⛔ BOTH LIVE VALUES — asserted BY NAME, never one standing for another.** ✅
The author measured these on the **prefix before the refusal** rather than on the whole body, and the reason is correct and worth recording: **this guard sits LOWER in the function than its `FogCover` sibling**, so a `PlayHandSlot`-wide census that satisfied `TASK-989` could in principle have been satisfied while this refusal sat *past* a spend. `:1043-1069` measures `SpendGold` 0 · `AddGold` 0 · `ConfirmPlayFromHand` 0 · `ConfirmInstantDraw` 0 · `DiscardFromHand` 0 · `CardPlaySoundPath` 0 · `CanAfford` **1**, all in `BeforeSunRefusal`.
**I re-derived the ordering at source:** the guard occupies `:1105-1138`, the refusal `return` is at `:1135`, `CanAfford` is at **`:1000`** (above — an unaffordable card still says *"Not enough gold"*, §3.5 precedence preserved), the accept stinger `PlaySound2D(this, CardPlaySoundPath)` at **`:1142`** (below — **no accept sound contradicts the toast**), and the routing `switch` at **`:1144`**. ⇒ ⛔ **Nothing on the path can move gold and nothing can consume a card. Both properties hold BY CONSTRUCTION, not by refund.**

**⭐⭐ THE ARGUMENT ORDER — the one defect a reader cannot see, and the pin really does die on a swap.** ✅ **CHECKED, as the author asked.**
`{0}` = `X` = current, `{1}` = `Y` = new. Swapping them still compiles, still refuses, still shows two live values — and reads as an **increase** while refusing the card. The two pins at `:1089-1094` identify each slot by its **trailing punctuation**: `WholeSecondsText(RemainingWindowSeconds),` (the argument followed by a **comma**) and `WholeSecondsText(WouldBeWindowSeconds)));` (the one that **closes the call**). ⇒ ⛔ **A swap makes both counts 0 and the row goes red in TWO places.** ✅ Confirmed against the shipped `:1133-1134`. The sentence itself is pinned verbatim at `:1095-1097`.

**⭐ THE CASTER-TEAM DERIVATION — I checked it copied the RIGHT idiom.** ✅
`Cast<AHeroCharacter>(GetPawn())` with an `IsValid` guard falling back to `ETeamId::Blue` (`:1112-1113`). ⛔ **It must match what the cast would use, or `Y` is measured off the wrong hero and the message describes an effect the player would not get.** `AFogVolume::GetBrightSunWindowSeconds` resolves the hero by `TActorIterator` + **team filter** + `!IsDead()` (`FogVolume.cpp:271-284`) ⇒ the team is the *only* input the entry supplies, and it supplies the shipped one. **Correct.** Compile surface checked: `HeroCharacter.h` is included at `SiegePlayerController.cpp:34`, and `virtual ETeamId GetTeamId() const override` (`HeroCharacter.h:432`) is `const` ⇒ callable on `const AHeroCharacter* const`. ✅

---

## §6 — COMPILE-RISK READ (no compiler available — a source read, not a build)

Nothing here is a UE 5.8 deprecation or a reflection error.
- `FText::Format` with **two** ordered arguments, `NSLOCTEXT`, `Cast<>`, `IsValid`, `TActorIterator`, `EAutomationTestFlags::EditorContext | EngineFilter` — **all current**, and the flag form matches the module's ~38 sibling test files.
- **Const-correctness:** `const AFogVolume* const FogState` calls two `const` accessors ✅ · `const AHeroCharacter* const CasterHero` calls a `const` `GetTeamId()` ✅.
- **Reflection:** `WholeSecondsText` is a plain `static` — ⛔ **no `UFUNCTION` needed**, matching its shipped refusal-text siblings. It stores no pointer ⇒ **no GC surface**, no `TObjectPtr` question. `FogState`/`CasterHero` are raw **locals**, never members ⇒ ⛔ **no `UPROPERTY` obligation** (this is exactly why the header census at zero also closes the GC question).
- **Null safety, both new pointers:** `AFogVolume::Find(GetWorld())` is checked by the `if`-init itself; `Cast<AHeroCharacter>(GetPawn())` is `IsValid`-guarded with a defined fallback ⇒ ⛔ **a null pawn cannot crash and cannot silently pick the wrong team** — it picks `Blue` and the accessor degrades to the base window.
- **Includes:** the controller `#include`s `Siegebound/FogVolume.h` (`:33`) and `Siegebound/HeroCharacter.h` (`:34`); the test file `#include`s both `Siegebound/FogVolume.h` (`:11`, for the pure static it calls directly) and `Siegebound/SiegePlayerController.h` (`:12`). ⛔ **Every symbol named is reachable.**
- **Probe signatures:** `ExtractFunctionBody` is fed `PlayHandSlotSignature`, `RefuseCardPlaySignature` and `WholeSecondsTextSignature` — all three matched the shipped source exactly when I checked them, which matters because a stale signature there **fails** the probe rather than skipping it (correct direction, but only if the string is right **today**). **It is.**
- ⚠️ **The likeliest first-compile error in this batch is not in this diff:** `SiegePlayerController.cpp` and `Tests/SiegeFogRefusalTest.cpp` both `#include "Siegebound/FogVolume.h"`, which is **untracked new code** from `TASK-982`. **If `FogVolume.h` does not reach the compiler, both files fail on a missing header — that is a STAGING failure, not a defect in `TASK-991`.** See §7.

---

## §7 — ⚠️⚠️ FOR THE COMMIT HOST (`TASK-987`) — RESTATED, AND EXPLICITLY UNMEASURED

⛔⛔ **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` IS UNTRACKED (`??`) — ACCEPTED AS DECLARED, ⛔ NEVER MEASURED** (`SC-§78`: I have no `Bash` and therefore no Git; §0). ⛔ **It must be staged by EXPLICIT PATHSPEC, or `TASK-991`'s FOUR tests *and* `TASK-989`'s SIX commit as SILENTLY ABSENT** — a working tree reporting **+10 green** over a `HEAD` that never received the file, forever, with nothing failing anywhere. ⭐ **This project has already paid for that exact defect once** (`Tests/SiegeCardHandKeyLabelTest.cpp`, `CONVENTIONS` §4882).
⛔ **The same applies to `Siegebound/FogVolume.{h,cpp}` and `Tests/SiegeBrightSunTest.cpp`** — both declared untracked, and §6 shows this row's two files **will not compile without `FogVolume.h`**.
⇒ ⛔ **Stage by pathspec, then read `git status` back for any `??` under `Siegebound/` before committing.** `SiegePlayerController.{h,cpp}` are declared tracked (`M`).

---

## Findings

### BLOCKERS — **none (0).**

### WARN

- **[WARN-1]** `Tests/SiegeBrightSunTest.cpp` (and `Tests/SiegeFogRefusalTest.cpp:839-841`) — ⛔⛔ **THE IDENTITY THIS WHOLE DESIGN RESTS ON IS NOT PINNED BY ANY EXECUTABLE ASSERTION, ON EITHER SIDE.** The no-pre-gate decision is justified by *"the predicate is bit-identical to `ApplyBrightSun`'s"* — which I **verified true at source** (§3) — but a **tree-wide grep across `Siegebound/Tests/` for `NewWindowSeconds` / `< RemainingSeconds` returns ZERO hits.** ⛔ **Nothing anywhere pins the state object's strict `<`.** Test 7 bans the non-strict spelling **only inside the entry's region**. ⇒ **A slip or a retune to `<=` in `AFogVolume::ApplyBrightSun` ALONE goes completely undetected, and produces exactly the *"the toast said yes but the card fizzled"* divergence the absent pre-gate exists to prevent.** ⭐ It is also the concrete cost of the §3(2) equal-resets flip being a two-site change. ⚖️ **NOT a blocker and NOT this row's to fix:** the host file is `TASK-982`'s deliverable, its gate `TASK-986` has **returned**, and a cross-file pin from `SiegeFogRefusalTest.cpp` was available but is a new claim, not a re-point. *Suggested fix: manager boards a one-assertion row — pin `CountOccurrencesInCode(ApplyBody, TEXT("if (NewWindowSeconds < RemainingSeconds)")) == 1` beside the existing `FMath::Max == 0` at `SiegeBrightSunTest.cpp:659-664`, so the two halves of the predicate are guarded symmetrically.*
- **[WARN-2]** `Tests/SiegeFogRefusalTest.cpp:604-606` — ⚠️ **`qa/TASK-990.md` WARN-2 IS STILL OPEN, AND I RE-RAISE IT ONCE.** That report asked the manager to carry *"re-point `SiegeFogRefusalTest.cpp` test 4's routing-predicate census"* into `TASK-1018`'s spec. ⛔ **A grep of `TASKBOARD.md` for `SiegeFogRefusalTest` returns NO hit** ⇒ the obligation lives **only** in an in-file comment (`:600-603`), and **an in-file comment is not a board obligation.** The pin remains **scheduled to go red on a CORRECT successor diff.** ⛔ `TASK-991` handled its side perfectly — it met the pin, confirmed it byte-identical, and left it alone. *Suggested fix: manager adds one clause to `TASK-1018`'s spec so the next author meets an EXPLAINED red rather than a mysterious one, and re-points rather than deletes.*
- **[WARN-3]** `Tests/SiegeFogRefusalTest.cpp` — **UNTRACKED (`??`), ⛔ ACCEPTED AS DECLARED, ⛔ NEVER MEASURED** (`SC-§78`). Full statement in **§7**; it is the single largest commit-time hazard in this row and it is **not visible from anything I can run.** *Suggested fix: stage by explicit pathspec, plus `FogVolume.{h,cpp}` and `Tests/SiegeBrightSunTest.cpp`; read `git status` back before committing.*
- **[WARN-4]** **REPORTED, ⛔ NOT A DEFECT IN THIS ROW (🧑 `J-F25`, closed — the board told me to report, not fail).** The shipped sentence is ~**85 characters** and carries **TWO** numbers the player must read, on a HUD channel whose show-then-hide lifetime `VID-005` measured at **≈1.8 s** and `CardHandWidget`'s own comment calls *"~2 s"* — two independent sources agreeing. ⭐ **This is strictly sharper than the sibling's case that `qa/TASK-990.md` WARN-4 raised: twice the length, twice the numbers, same 1.8 seconds.** The author measured it, reported it, and correctly did **not** action it — the timer lives in `WBP_CardHand`, is not assertable from C++, and retuning it is a game-wide UI change. ⇒ 🧑 **Jonathan's call, at a checkpoint, never a task in this lane.** ⚖️ If he wants it shorter, *"Bright Sun would shorten fog prevention: 143s → 83s"* is a one-line change to one `NSLOCTEXT`.

### NIT

- **[NIT-1]** `handoffs/TASK-991-programmer.md` §7(2) — **a stale premise, declared in good faith.** It states *"`TASK-986` is still outstanding, so the file that was a review subject for `TASK-989` is still one for me."* ⛔ **`qa/TASK-986.md` EXISTS ON DISK and reads `Verdict: PASS — 0 BLOCKERS · 4 WARN · 4 NIT`** ⇒ that gate **has returned**, and `Tests/SiegeBrightSunTest.cpp` was in fact editable by the author's own rule. ⭐ **Nothing unwinds:** the outcome (leaving the file byte-untouched) is the **conservative** direction, and the row's *other* directional claim — that `qa/TASK-990.md` had returned, permitting the edit to `SiegeFogRefusalTest.cpp` — I verified **true**. **Recorded only so the reasoning stays accurate**, and because it is why **WARN-1**'s missing pin had no owner.
- **[NIT-2] Checked and cleared, recorded so nobody "fixes" it into a divergence:** the two halves read the operands in **opposite order** — the entry reads `X` then `Y` (`:1118-1119`), `ApplyBrightSun` reads `Y` then `X` (`FogVolume.cpp:140, 152`). ⛔ **This is NOT a difference:** both accessors are `const` and side-effect-free, and neither advances the world clock, so the pair of values is identical either way. The entry's order is a **readability** choice matching the sentence (*"from X to Y"*), stated as such. **No change wanted.**
- **[NIT-3]** `SiegePlayerController.cpp:1119` — **the honest cost of the absent pre-gate, stated rather than hidden.** Every `BrightSun` click now runs `GetBrightSunWindowSeconds` — a `TActorIterator<AHeroCharacter>` — **even when no prevention window is up**, i.e. in the overwhelmingly common case. ⛔ **Once per CLICK, never per tick**, over a hero population of 2, with no `FindObject`/`LoadObject` on the path. ⚖️ **Negligible, declared by the author, and UPHELD** — §3 shows the pre-gate that would skip it is the one that would create a disagreement. ⛔ **Caching either value to "save" it is precisely the defect this row exists to prevent.**
- **[NIT-4]** `Tests/SiegeFogRefusalTest.cpp` — **the limits of a source-text instrument, recorded so the ledger stays honest.** (a) The end-to-end claim — *stand on a tower, click, walk down, click again, see two different `Y` values* — is **not executed**: there is **not one `SpawnActor` anywhere in `Siegebound/Tests/`**, a gap `TASK-982` and `TASK-989` both declared for the same reason. The executed half (two heights ⇒ `240 s` vs `120 s` ⇒ two different sentences, `:908-919`) proves the window varies with height; the structural half proves the call site asks the **live** accessor and stores nothing. **That is the strongest form available without a world.** (b) A source-text pin cannot catch an unconditional `return` inserted **after** a retained conditional — the conditional pin plus the switch-follows control cover every shape a real diff takes, but not an adversarial one. **Declared by the author at `:426-428`, `:1005-1008` and `:221-227`; no action.**
- **[NIT-5] `qa/TASK-990.md` NIT-1's prediction has now come true, and I closed it out rather than leaving it hanging.** That report noted `FMath::RoundToInt` on an arbitrarily large input overflows `int32`, and that *"the formatter is public and `TASK-991` will feed it a COMPUTED duration"* — which is exactly what `:1134` now does, into a window `J-F14` ruled **UNCAPPED**. ⛔ **Still unreachable:** overflow needs `Y > 2.1×10⁹` s ⇒ a hero at ~`5×10¹⁰` uu, and the pure static's `FMath::IsFinite` guard (`FogVolume.cpp:302`) catches the degenerate inputs. **Recorded as closed, not as a required change.**

---

## §8 — REGRESSION SWEEP: DOES THIS DIFF TURN ANYTHING ELSE RED?

This is where an uncompiled wave hides its cost. **Result: no existing pinned assertion is disturbed, and the three that needed moving were moved rather than discovered.**

- **The 79-line insertion moves probes only inside `SiegeFogRefusalTest.cpp`.** `qa/TASK-990.md` §5 established that a tree-wide grep for `PlayHandSlot` across `Siegebound/Tests/` hits **only that file**; this row added no new extractor of `PlayHandSlot` elsewhere.
- **The three pins that DID have to move were re-pointed in place** (§4 item 4) — ⭐ **found by the author's own 51-measurement sweep of the shipped bytes, not by this gate.** That is the correct direction and it is worth saying: the gate met an already-consistent file.
- **`Tests/SiegeBrightSunTest.cpp`'s censuses cannot see this diff.** They are scoped to `ApplyBrightSun`'s / `RaiseFog`'s / `ResolveSpell`'s extracted bodies inside `FogVolume.cpp` and `SpellLibrary.cpp` — **neither file was opened by this row**, and `FogVolume.cpp:775`'s *"exactly two prevention-deadline write sites"* file-wide pin is over `FogVolume.cpp`, which is unchanged.
- **Test 4's `GoldSteal` pin stays green** — the predicate moved by line number only (§4 item 5).
- **`SiegeFogClampTest.cpp` / `SiegeAcquisitionFunnelTest.cpp` / `SiegeInvisibilityTest.cpp` tree-wide censuses** pin needles (`FSiegeVisionQuery::Seeing`, `EffectiveVisionRadius(`, `ReadFogState(`, `FogDensityAt(`, `GetAllActorsWithInterface(`, `bIsInvisible`) that **none of this diff's new tokens match** ⇒ **zero added hits.**
- **`SiegePlacementTest.cpp`'s `CardRefused_CantAfford == 1`** targets `DiscardEntireHand`'s body — untouched; and `CanAfford` at `:1000` is unmoved relative to the guard.

## §9 — `SC-§29` COVERAGE LEDGER

| covered by me | ⛔ not covered, and by whom |
|---|---|
| ✅ `TASK-991` — `SiegePlayerController.{h,cpp}` + tests 7-10 of `Tests/SiegeFogRefusalTest.cpp` | ⛔ `TASK-989`'s own refusal + tests 1-6 ⇒ **`qa/TASK-990.md` (PASS, cited, ⛔ NOT re-reviewed)** |
| ✅ the `FMath::Max` census on the **entry** side, re-derived at zero | ⛔ the **state-object** half (`ApplyBrightSun`, the accessors, the formula) ⇒ ⭐ `TASK-982` / `qa/TASK-986.md` (**returned PASS**) — I **read** them to verify this row's predicate rests on solid ground; I did **not** re-grade them |
| ✅ the no-pre-gate and equal-resets calls, ruled explicitly | ⛔ the toast lifetime + `J-F24`'s wording ⇒ 🧑 **Jonathan** (flagged, not findings) |
| ✅ the +4 delta, counted at source; the three re-points, each re-derived | ⛔ any suite **absolute**, any `HEAD`-vs-tree or tracked-vs-untracked fact ⇒ `TASK-987`'s build-master (**I have no Git — §0, §7**) |
| ✅ the declared targeting-mode residual, graded below | ⛔ the reticle-routing defect itself ⇒ ⭐ `TASK-1018` (consuming ⭐ `TASK-999`'s delivery-derived predicate) |

**⚖️ On the declared residual (handoff §4):** ✅ **GRADED A DECLARED RESIDUAL, ⛔ NOT A DEFECT — and I checked that the transfer from `qa/TASK-990.md` §3(2) is REAL here rather than merely convenient.** It is the same window, in the opposite direction: today's blacklist-of-one routing sends `BrightSun` into targeting mode, so the player can click (guard passes), descend, and confirm — `ApplyBrightSun` then re-reads both values, refuses, and he gets the generic *"Spell fizzled"*. ⛔ **His two default properties still hold in that window** — `TryConfirmSpellTarget` refunds in full on the false branch and its `ConfirmPlayFromHand` sits **after** the refusal's early `return`, both measured by `TASK-982` and re-confirmed at source by `qa/TASK-990.md` §3(2). **Only the wording is generic.** ⭐ **The window closes BY CONSTRUCTION when `TASK-1018` lands**, and a second call site built for it now would be **dead the day that row ships** — the exact dead surface `SC-§40` cl. 2 bans. ⛔ **The author's refusal to build it is correct. No row needed.**

---

## Notes for build-master

- ⛔⛔ **STAGE `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` BY EXPLICIT PATHSPEC** (WARN-3 / §7) — and **`Siegebound/FogVolume.{h,cpp}`** + **`Tests/SiegeBrightSunTest.cpp`**, all declared untracked. **Read `git status` back for `??` under `Siegebound/` before you commit.** ⛔ **I could not measure any of this — it is forwarded exactly as declared.**
- ⛔ **This row has NEVER been compiled or executed by anyone, and neither has anything else in this batch.** Declared delta **+4** (re-counted by me at source: 10 macros, 10 unique registered names, 6 of them `TASK-989`'s). ⛔ **Publish no absolute** — the tree is contaminated by concurrent lanes (`TL-§5c`).
- ⚠️ **The likeliest first-compile error is a STAGING failure, not a code defect** (§6): both of this row's files `#include "Siegebound/FogVolume.h"`, which is untracked new code from `TASK-982`. A missing-header failure there is **not** a `TASK-991` defect — do not route it back as one.
- ⛔ **Serialisation still stands and this row was the middle link:** `TASK-989` → **`TASK-991` (done)** → `TASK-1018`. `SiegePlayerController.cpp` is now **free** on this chain.
- 🧑 **Three things for Jonathan at the checkpoint, none blocking:** the **≈1.8 s toast** vs an ~85-char two-number sentence (WARN-4) · the **display floor** inherited from `qa/TASK-990.md` WARN-1 · the **equal-resets** boundary (§3(2)) — ⛔ **a one-line note, not a question that holds anything up.**

## ⛔ ONE LINE ON THE COMMIT

**⛔ NOTHING BLOCKS THE COMMIT.** 0 blockers; the refusal is **provably CONDITIONAL** (the strict comparison pinned at 1 inside its own region, `FMath::Max` re-derived by me at **zero** across the whole entry, and the routing switch confirmed to **still follow** the message so a **longer** window falls through and casts); the effect is **reused, never reimplemented**; the only commit-time hazard is **staging the untracked files by pathspec** (WARN-3), which I could not measure.

---

**`TASK-991`'s status line (⛔ NOT flipped by me, as instructed): `.claude/pipeline/TASKBOARD.md:18583` — currently `status: ✅ **`ready-for-qa` 2026-09-04** — gate **`TASK-992`** …`.** ⇒ it is now **`qa-passed` / `ready-for-integration`** on this verdict, for whoever owns that row.
