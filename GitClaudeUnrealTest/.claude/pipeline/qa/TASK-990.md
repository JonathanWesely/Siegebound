# QA Report — TASK-990
**Gate over:** `TASK-989` ALONE (`SC-§29` coverage ledger below) · **Subject handoff:** `handoffs/TASK-989-programmer.md`
**Verdict: PASS** — **0 BLOCKERS** · 4 WARN · 4 NIT
**Date:** 2026-09-04 · **Reviewer:** qa-reviewer

---

## §0 — `SC-§39`: MY INSTRUMENTS, DECLARED ABOVE THE FINDINGS, AND CONTROLLED BOTH WAYS

**Had:** `Read` · `Grep` · `Glob` · scoped `Edit` (this report + `TASK-990`'s own `status:` line).
**Did NOT have:** ⛔ `Bash` (`SC-§78`) ⇒ ⛔ **zero Git**, ⛔ zero compile, ⛔ zero suite execution, ⛔ zero engine/MCP.

**⇒ EVERY `HEAD`-vs-tree, tracked-vs-untracked and "byte-untouched" claim in the handoff is ⛔ ACCEPTED AS DECLARED, ⛔ NEVER MEASURED.** I read the **working tree**. I cannot distinguish "this row wrote it" from "it was already there" — so every finding below is a claim about the **shipped bytes as they now stand**, which is what a compile will see.

**Instrument controlled in BOTH directions (a zero from a dead scanner is the failure mode):**
- **Positive control:** the needle `GetFogPreventionSecondsRemaining` returns **1** hit in `SiegePlayerController.cpp` and **4** in `FogVolume.{h,cpp}` ⇒ the scanner finds the token when it is there.
- **Negative control:** the needle `FogClear` returns a **9-file** hit list that includes `FogVolume.{h,cpp}`, `SpellLibrary.cpp`, `CardRow.h` and **excludes `SiegePlayerController.cpp`** ⇒ the zero I report for `ESpellEffect::FogClear` in the controller is a **finding**, not a blind scan.

---

## §1 — ⛔⛔ THE CENTRAL CLAIM: **VERIFIED AT SOURCE, ALL FOUR PARTS**

The row's one blocker-grade question — *live at the click, or cached at cast time?* — is answered **LIVE**, and the form it is held in is stronger than "the author was careful".

| # | claim | verdict | evidence (line numbers are mine, from the tree) |
|---|---|---|---|
| 1 | the accessor is called **exactly once in the whole controller** | ✅ **CONFIRMED** | `SiegePlayerController.cpp:1044` is the **only** occurrence of `GetFogPreventionSecondsRemaining` in the file — file-wide grep, one hit |
| 2 | bound to a **local**, and **that same local** is the format argument | ✅ **CONFIRMED, character-for-character** | `:1044` `const float PreventionSecondsRemaining = FogState->GetFogPreventionSecondsRemaining();` → `:1055` `WholeSecondsText(PreventionSecondsRemaining)))`. No other value reaches the toast. |
| 3 | the header declares **no stored remainder** | ✅ **CONFIRMED** | `SiegePlayerController.h`: the *only* lines matching `PreventionSeconds` / `FogPrevent` / `Remaining` are **`:514`** (a `*`-prefixed doc line) and **`:934`** (the formatter declaration). **Zero member fields**, zero `static`, zero `mutable`. |
| 4 | ⛔⛔ the refusal is gated on **the REMAINDER ITSELF**, not on `IsFogPrevented()` | ✅ **CONFIRMED, and it is the strongest part of the diff** | `:1045` `if (PreventionSecondsRemaining > 0.f)`. `IsFogPrevented` appears in this file **only at `:1020`, inside a comment** — **zero code occurrences.** |

### ⭐⭐ AND I VERIFIED THE PROPERTY THE AUTHOR CLAIMS THAT BUYS — I did not take the accessor's contract on trust

The claim rests on *"the accessor returns 0 whenever the machine is not `SHIELDED`"*. I read it (`FogVolume.cpp:222-252`) rather than accepting `TASK-986`'s verdict as covering it:

- `IsFogPrevented()` ⇒ `World->GetTimeSeconds() < FogPreventedUntilTimeSeconds` (`:232`), `false` on a null world.
- `GetFogPreventionSecondsRemaining()` ⇒ `(FogPreventedUntilTimeSeconds - World->GetTimeSeconds() > 0.0) ? … : 0.f` (`:250-251`), `0.f` on a null world.

⇒ **`GetFogPreventionSecondsRemaining() > 0.f` and `IsFogPrevented()` are the SAME predicate over the SAME two scalars with the SAME strict comparison and the SAME null-world direction.** Substituting one read for two is therefore **free of behaviour change and free of the window** — there is no instant at which the predicate and the message can describe different times, because **there is only one time**.

> ⚖️ **RULING: one read is strictly stronger than two, and this is the shape I would have required.** A `if (IsFogPrevented()) { … Format(GetFogPreventionSecondsRemaining()) … }` build passes every other assertion in this file and re-samples the clock between deciding and printing. That build is what §1(4) makes **inexpressible**. ⛔ **There is no path in this diff that decides on one value and prints another.**

---

## §2 — THE BOARD'S NUMBERED ITEMS, EACH RE-DERIVED

**(1) `J-F24` — WHOLE SECONDS, ROUNDED, EVEN PAST 60.** ✅
`WholeSecondsText` (`:5415-5455`) emits `"{0} seconds"` / `"1 second"` / `"0 seconds"` — **no minutes branch, no `%02d`, no divmod by 60 anywhere in the function**. The unit word lives **inside** the formatter (so his retune really is one word) and the shipped string is `"Bright Sun is still up for {0}"` ⇒ **"Bright Sun is still up for 143 seconds"**. Test 1 re-derives `2 × 60 + 23 = 143` rather than typing it, asserts `60`/`120` stay seconds, and asserts the message does **not** contain `"minute"` — the one form he ruled out. `42.4 ⇒ 42` and `42.6 ⇒ 43` is the only pair that separates round from floor **and** from ceil; both are present.

**(2) `J-F26` — `Fog` ONLY.** ✅ **Verified twice, at two different layers:**
- **Code:** `ESpellEffect::FogCover` appears **once** in `PlayHandSlot` (`:1040`) and is the sole gate; `CardRefused_BrightSunActive` appears **once in the whole controller** (`:1054`). No CardID literal.
- **Data:** `Docs/Data/cards.csv` — `FogCover` appears in **exactly one row** (`Fog`, line 34). ⇒ today the effect gate **is** the card, so his sentence and the shipped gate cannot diverge.
⚖️ On the author's own §9(4) caveat (a *future* card that also raises fog would also be refused): **that is CORRECT, not a scope widening.** The refusal describes the mechanic ("new fog is prevented"), so gating it on the effect rather than the name is the narrower, truer reading.

**(3) `J-F25` — the SAME toast, NOT retuned.** ✅ **Surface re-measured hop by hop, not accepted from the handoff:**
`RefuseCardPlay(CardID, Reason)` (`:5854`) → `OnCardPlayRefused.Broadcast` **×1** (`:5858`) + `BroadcastRefusal(Reason)` **×1** (`:5861`) → `OnCardRefused.Broadcast(Reason.ToString())` (`:5866`) → `CardHandWidget.cpp:153` `OnCardRefusedMessage(Reason);` **×1, a 1:1 forward** → `WBP_CardHand`. **Zero new delegates, zero `CreateWidget`, zero new toast** inside `PlayHandSlot`. The copied precedent is real: `NSLOCTEXT("Siegebound", "CardRefused_MaxStacks", "{0} at max stacks")` ships at `:4793`. See **WARN-4** for the lifetime.

**(4) ⛔⛔ "NO GOLD, CARD KEPT" HOLDS BY CONSTRUCTION — CENSUS RE-DERIVED INDEPENDENTLY.** ✅
`PlayHandSlot` occupies `:892-1126` (`void ASiegePlayerController::PlayHandSlot(int32 Slot)` at `:892`, the column-0 `}` at `:1126`). File-wide grep with line numbers, first occurrence of each needle:

| needle | first occurrence in file | inside `892-1126`? |
|---|---|---|
| `SpendGold` | `:1210` (code), `:1026` (comment only) | **0** |
| `AddGold` | `:2612` | **0** |
| `ConfirmPlayFromHand` | `:2511`, `:1027` (comment only) | **0** |
| `ConfirmInstantDraw` | `:4648` | **0** |
| `DiscardFromHand` | `:1224` | **0** |

⇒ **Nothing in this entry can move gold and nothing in it can consume a card.** The refusal `return` is at `:1056`; the routing `switch (Row->CardType)` — the only way this function reaches any resolver — is at `:1065`. The guard is **above** it.
⭐ **That converts his two default properties from a behaviour into a structural guarantee: there is no refund to get wrong, and no instant in which the player's gold is briefly lower.** The two properties are asserted **separately** in test 3, which is the right shape — a build that refunds but eats the card satisfies half the ruling and passes a test written against either half alone.
Ordering extras I confirmed: the guard sits **after** `CanAfford` (`:1000`, preserving §3.5's precedence — an unaffordable `Fog` still says "Not enough gold") and **before** `PlaySound2D(this, CardPlaySoundPath)` (`:1063`, so no accept stinger contradicts the toast).

**(5) `:1037` NOT touched — the `GoldSteal` predicate censused at 1, and that census is an assertion.** ✅ **BOTH CONFIRMED.**
`if (Row->SpellEffect == ESpellEffect::GoldSteal)` appears **once** inside `PlayHandSlot`, now at **`:1090`** (`:1084` is a comment; `:3165` is a different function). ⚠️ **The predicate has MOVED from `:1037` to `:1090` — pushed down by the 52-line guard inserted above it — but its text is byte-identical and its count is 1.** The line number changed; the code did not. This is also why `qa/TASK-1012.md` WARN-1 cites `:1090`: the same statement, post-insertion.
The census **is** an executable assertion: `Tests/SiegeFogRefusalTest.cpp:564-566` pins that exact literal at **1**. See **WARN-2** for its scheduled expiry.

**(6) The shared formatter is reusable by `TASK-991` without copying.** ✅
`static FText WholeSecondsText(float Seconds);` is declared at `SiegePlayerController.h:934`, inside the `public:` block opened at `:207` (the next specifier is `protected:` at `:1613`) ⇒ **public + static + pure**. Verified pure at source: the body contains **no `GetWorld`, no `AFogVolume`, no member access** — it is a scalar in, an `FText` out. It also contains **no `BrightSun` and no `CardRefused_`** ⇒ **it is a formatter, not a message builder**, so `TASK-991`'s two-value sentence can call it twice without inheriting this row's wording. `SC-§40` cl. 2 is satisfied at every point: `WholeSecondsText(` occurs **twice** in the .cpp (the definition at `:5415` + the one live caller at `:1055`) — **defined AND called, never a seam waiting for one.** The dead-surface trap is also clear: **`ESpellEffect::FogClear` is at ZERO in this controller** (negative control in §0) ⇒ **no pre-built `J-F18` branch.**

**(7) Suite delta `+6`, all in the NEW `Tests/SiegeFogRefusalTest.cpp`, DECLARED never executed, no tree absolute.** ✅ **RE-COUNTED AT SOURCE:** 6 × `IMPLEMENT_SIMPLE_AUTOMATION_TEST` (`:219, 304, 403, 490, 582, 663`), 6 unique `"Siegebound.FogRefusal.…"` names, all six class names (`FSiegeFogRefusal*`) unique in the module. No absolute is claimed anywhere in the handoff. **I executed nothing** — no compile, no suite (§0).

---

## §3 — ⚖️ THE THREE DECLARED ITEMS: MY RULING ON EACH

### ⚖️ 1. THE ONE JUDGEMENT CALL — REFUSING AT THE **ENTRY** RATHER THAN AT A RESOLVER: ✅ **RIGHT, AND IT IS THE PLACEMENT I WOULD HAVE REQUIRED.**

The alternative was two resolver-side guards (`ResolveSpellInstant` + `TryConfirmSpellTarget`). I graded it on four axes and the entry wins on all four:

1. **Structural vs transactional.** At the entry, "no gold, card kept" is **the absence of any spend or consume** (§2(4) census). At a resolver it is a **compensating refund** — a spend, then a give-back, with a window between them and a second failure mode (`AddGold` refusing a non-positive grant, the `> 0` guard, the ordering of the early `return` relative to `ConfirmPlayFromHand`). His ruling is *"should not lose gold"*, and **never taking it is a stronger reading of that sentence than giving it back.**
2. **Routing-agnostic, and I verified the independence rather than assuming it** (see §4). The guard is at `:1040-1059`, strictly **above** the `switch` at `:1065` and therefore above the `GoldSteal` branch at `:1090`. It reads **no** `CardType`, **no** `SpellDelivery`, **no** routing predicate. ⇒ **`TASK-1018` can rewrite the entire routing arm and this guard fires identically, unedited.** A resolver-side guard would have had to be written into `TryConfirmSpellTarget` **today** and then moved (or duplicated) into `ResolveSpellInstant` **the moment `TASK-1018` lands** — one message, two call sites, one of them dead on arrival.
3. **The player never enters a mode he cannot complete.** Under today's routing a resolver-side refusal would put him into targeting mode, hand him a reticle for a **map-wide** effect, let him aim it, and *then* say no.
4. **It makes nothing redundant.** `TASK-982`'s guard inside `AFogVolume::RaiseFog()` is untouched and is **not** dead — the **bot** reaches `USpellLibrary::ResolveSpell` without ever passing through `PlayHandSlot`, and I confirmed the `FogCover` arm still refuses on `RaiseFog()` returning false (`SpellLibrary.cpp:690-697`). ⭐ **The layering is correct and deliberate: the state object keeps the INVARIANT, the controller owns the MESSAGE.** Removing either would be wrong, and this diff removes neither.

### ⚠️ 2. THE DECLARED RESIDUAL — *"Spell fizzled"* for a shield raised while already in targeting mode: ✅ **GRADED AS A DECLARED RESIDUAL, ⛔ NOT A DEFECT. NET-ZERO CONFIRMED AT SOURCE.**

I traced the window rather than accepting it: `TryConfirmSpellTarget` spends at `:3351`, `ResolveSpell` returns false (the `RaiseFog` refusal), the **full** refund fires at `:3377-3380` (`AddGold(TargetingCost)`), `RefuseCardPlay(…, CardRefused_SpellFizzled)` at `:3384`, then an **early `return` at `:3386`** — **before** `ConfirmPlayFromHand` at `:3406`.
⇒ ⛔ **Zero net gold and the card stays in hand.** Only the *wording* is generic, and the window closes **by construction** when `TASK-1018` makes `Fog` resolve on the click. The author's refusal to build a second call site for it is correct: that site would be **dead the day `TASK-1018` ships**, which is precisely the dead surface `SC-§40` cl. 2 bans. **No row needed. I do not disagree with §4.**

### ⭐⭐ 3. CONDUCT WORTH GRADING EXPLICITLY — THE NEW TEST FILE: ✅ **SAY SO IN THE VERDICT, AND I DO.**

The author was told *"check for an existing frame first, never a duplicate"* and was pointed at `Tests/SiegeBrightSunTest.cpp`. It read that file, and **declined to write in it because that file is `TASK-982`'s deliverable and its gate `TASK-986` had not returned at the time of writing** — adding assertions would have mutated **a review subject mid-review**.

⭐ **That is the gate-integrity discipline applied UNPROMPTED, by a programmer, to its own work, at a cost to itself** (a new file is more work than three appended rows, and it invites exactly the "why a second file?" challenge it then had to pre-answer). It is the same rule `TASK-982` invoked in the *opposite* direction when it edited `SiegeFogVolumeTest.cpp` — a file whose verdict had already been returned. **The rule was applied by its direction, not by its convenience. That is the mark of a rule someone actually holds.**

And the "duplicate" charge fails on the merits, which I checked rather than assumed: `SiegeBrightSunTest.cpp`'s net-zero test (`:743-841`) asserts over **`ResolveSpellInstant` and `TryConfirmSpellTarget`** — the **resolver** paths. This row's test 3 asserts over **`PlayHandSlot`** — the **entry**. **Different functions, different needles, not one duplicated assertion.** The two files are complements: `982` covers net-zero *by refund at the resolver*, `989` covers it *by construction at the entry*.

---

## §4 — ⚠️ INDEPENDENCE FROM `qa/TASK-1012.md` WARN-1 — **CONFIRMED, NOT ASSUMED**

WARN-1 there established that `BrightSun` will show a reticle because the no-reticle path is a hard-coded `GoldSteal` branch (`:1090`) rather than a `SpellDelivery` cell — `TASK-1018`'s territory. **I re-derived the independence here:**

- The refusal guard occupies `:1040-1059`; the `switch (Row->CardType)` is at `:1065`; the `GoldSteal` branch is at `:1090`. The guard is **entirely upstream** of all of it.
- The guard's inputs are exactly two: `Row->SpellEffect` and `AFogVolume`'s remainder. It never reads `CardType`, `SpellDelivery`, or the routing predicate.

⇒ ⛔ **Whatever `TASK-1018` does to `:1090` — replace it with `TASK-999`'s delivery-derived predicate, invert it, delete it — this refusal is unaffected, and needs no edit.** The **only** coupling in either direction is a test-side pin, which is **WARN-2**, not a code dependency.

---

## Findings

### BLOCKERS — **none (0).**

### WARN
- **[WARN-1]** `SiegePlayerController.cpp:5441` — **the display floor is a declared sub-default that goes slightly beyond `J-F24`'s "rounded".** `FMath::Max(1, FMath::RoundToInt(Seconds))` makes a strictly-positive remainder below half a second render **"1 second"** where plain rounding gives **"0 seconds"**. ⚖️ **I UPHOLD IT:** it can only fire in the `(0, 0.5)` band, a true zero returns before it (`:5431`), and without it the game would refuse the card *for* that window while announcing the window is over — a message contradicting its own refusal in one breath. It is declared, labelled, tested (`test 1`, the `0.2f ⇒ "1 second"` row) and is a **two-line revert**. ⛔ **Flagged for 🧑 Jonathan's eye as a one-word/one-line overrule, NOT for a QA loop.** *Suggested fix: none — carry it to the checkpoint alongside `J-F24`.*
- **[WARN-2]** `Tests/SiegeFogRefusalTest.cpp:564-566` — **a pin that is SCHEDULED to go red on a CORRECT successor diff.** It asserts the literal `if (Row->SpellEffect == ESpellEffect::GoldSteal)` at exactly 1 inside `PlayHandSlot`. `TASK-1018` must replace that predicate with `TASK-999`'s delivery-derived one ⇒ **this row turns red for a correct change.** ⭐ The author handled it the right way — the comment at `:560-563` instructs `TASK-1018` to **re-point it, not delete it**, because the claim (*"the routing is derived in exactly one place"*) outlives the spelling — and the author deliberately omitted two *other* pins (`ESpellEffect::` total, `FogClear` at 0) for exactly this reason, which is consistent, not arbitrary. ⛔ **But an in-file comment is not a board obligation.** *Suggested fix: manager to carry "re-point `SiegeFogRefusalTest.cpp` test 4's routing-predicate census" into `TASK-1018`'s spec, so the next author meets an explained red rather than a mysterious one.*
- **[WARN-3]** `Tests/SiegeFogRefusalTest.cpp` — **NEW and UNTRACKED (`??`, ACCEPTED AS DECLARED — I have no Git).** ⛔ **It must be staged by explicit pathspec at `TASK-987`, or this entire gate commits as silently absent** — a working tree reporting +6 green over a `HEAD` that never received the file, forever, with nothing failing. This project has already paid for that exact defect once (`Tests/SiegeCardHandKeyLabelTest.cpp`, `CONVENTIONS` §4882). The same applies to its dependencies `FogVolume.{h,cpp}` and `Tests/SiegeBrightSunTest.cpp`, also declared untracked. *Suggested fix: build-master stages by pathspec and reads back `git status` for `??` under `Siegebound/` before committing.*
- **[WARN-4]** **REPORTED, NOT A DEFECT IN THIS ROW (`J-F25`, and the board told me to report rather than fail):** the refusal surface's show-then-hide lifetime is **≈1.8 s** (`VID-005`, two independent bursts) and **"~2 s"** by `CardHandWidget.cpp:152`'s own comment — **two independent sources agreeing.** ⛔ **That is short for the first message in this game that carries a NUMBER the player is meant to READ.** The author measured it, reported it, and correctly did **not** action it: the timer lives in `WBP_CardHand` (a Blueprint asset, not assertable from C++) and retuning it is a **game-wide UI change**. ⇒ 🧑 **Jonathan's call, at a checkpoint, never as a task in this lane.**

### NIT
- **[NIT-1]** `SiegePlayerController.cpp:5441` — `FMath::RoundToInt` on a finite but arbitrarily large input overflows `int32`. Unreachable today (the accessor clamps; the window is bounded by `120 + 60 × floor(H/1524)`), but the formatter is **public** and `TASK-991` will feed it a **computed** duration. *Suggested fix (defensive only): clamp the rounded value, e.g. `FMath::Clamp(FMath::RoundToInt(FMath::Min(Seconds, 86400.f)), 1, …)`. Not required for this row.*
- **[NIT-2]** `SiegePlayerController.cpp:5433/5449/5453` — three `NSLOCTEXT` keys (`SecondsCount_Zero`/`_One`/`_Many`) where an ICU `|plural()` form would use one. ⚖️ **I UPHOLD the author's reasoning explicitly:** this project ships no localisation and no plural-form string anywhere, so a hand-rolled plural pattern would be the codebase's **only** idiom that fails at **runtime** rather than at **compile time** — written by an author who could neither compile nor run. Three legible keys are the correct call here.
- **[NIT-3]** `Tests/SiegeFogRefusalTest.cpp:313-336` — the "two clicks a second apart" half is executed over the **formatter**, not over a live click; the "the argument is a live read" half is **structural**. The author declared this openly (`:52-60`, `:386-388`) and the cause is real: **there is not one `SpawnActor` anywhere in `Siegebound/Tests/`**, so no test in this project can drive a card click. `TASK-982` declared the identical gap. ⛔ **Recorded so the ledger stays honest — the end-to-end claim remains unexecuted, and the two halves are the strongest form available without a world.**
- **[NIT-4] Checked and cleared, recorded so nobody "optimises" it:** `AFogVolume::Find` runs a `TActorIterator` (`FogVolume.cpp:35`). That is **once per `Fog`-card click**, ⛔ **not per tick** — negligible, and there is no `FindObject`/`LoadObject` anywhere on the path. ⛔ **Caching the pointer or the remainder to "save" it is precisely the defect this entire row exists to prevent.**

---

## §5 — REGRESSION SWEEP: DOES THIS DIFF TURN ANYTHING ELSE RED?

This is where an uncompiled wave hides its cost, so I swept it rather than assuming. **Result: NO existing pinned assertion is disturbed.**

- **No existing test extracts `PlayHandSlot`.** A tree-wide grep for `PlayHandSlot` across `Siegebound/Tests/` returns hits **only** in `SiegeFogRefusalTest.cpp`. The 52-line insertion moves no other probe.
- **The three tree-wide censuses were checked by needle, not by name.** `SiegeFogClampTest.cpp`'s `CountAcrossShippingSource` pins `FSiegeVisionQuery::Seeing`, `FSiegeFogStatics::EffectiveVisionRadius(`, `ReadFogState(`, `FogDensityAt(`, `EffectiveVisionRadius(`. `SiegeAcquisitionFunnelTest.cpp` pins `GetAllActorsWithInterface(`. `SiegeInvisibilityTest.cpp` pins `bIsInvisible`. ⇒ **None of them can see `AFogVolume::Find` or `GetFogPreventionSecondsRemaining`.** The new `#include` and the new call add **zero** hits to any of them.
- **Every existing `AFogVolume::Find(` / `FindOrSpawn(` pin is function-scoped**, to `ResolveSpell`'s body, `ReadFogState`'s body, or `FogVolume.cpp` — never controller-wide, never project-wide.
- **`SiegeBuildingStackTest.cpp`'s controller probes** target `ResolvePlacementUpgradeState` and `ConfirmStackUpgrade`; **`SiegePlacementTest.cpp:2221`'s `CardRefused_CantAfford == 1`** targets `DiscardEntireHand`'s body. All untouched.
- **`SiegeFogVolumeTest.cpp`'s `ResolveSpell` pins** (`FindOrSpawn` ×2, `RaiseFog()` ×1, `Find(` ×0) sit in `SpellLibrary.cpp`, which this row did not open.

## §6 — COMPILE-RISK READ (no compiler available — this is a source read, not a build)

Nothing here is a UE 5.8 deprecation or a reflection error: `FText::Format`, `NSLOCTEXT`, `FMath::IsFinite`, `FMath::RoundToInt`, `FText::AsNumber`, `TActorIterator`, `EAutomationTestFlags::EditorContext | EngineFilter` (the form used by **38 sibling test files**) are all current. `WholeSecondsText` is a plain `static` — **no `UFUNCTION` needed**, matching its five shipped refusal-text siblings, and it stores no pointer so there is no GC surface. `const AFogVolume* const` calling a `const` accessor is const-correct; `Find(const UWorld*)` accepts `GetWorld()` and is null-safe on both sides. The test file's includes cover every symbol it names, its fixture namespace (`SiegeFogRefusalFixture`) and six test class names are unique in the module, and its `ExtractFunctionBody` signatures (`PlayHandSlot`, `RefuseCardPlay(FName CardID, const FText& Reason)`, `WholeSecondsText(float Seconds)`) each match the shipped source **exactly and uniquely** — I checked all three, because a stale signature there fails the probe rather than skipping it, which is the correct direction but only if the string is right today. **It is.**

## §7 — `SC-§29` COVERAGE LEDGER

| covered | not covered, and by whom |
|---|---|
| ✅ `TASK-989` — `SiegePlayerController.{h,cpp}` + `Tests/SiegeFogRefusalTest.cpp` | ⛔ `TASK-982`'s state machine + accessors ⇒ `TASK-986` (**returned, qa-passed**) — I *read* the accessor to verify this row's predicate rests on solid ground, I did not re-grade it |
| ✅ the entry-vs-resolver placement, and its residual | ⛔ the reticle-routing defect ⇒ `TASK-1018` (+ `TASK-999`'s predicate) — see §4 |
| ✅ `J-F24` / `J-F25` / `J-F26` as **built**, and the live-value claim | ⛔ `J-F24`'s wording itself and the toast lifetime ⇒ 🧑 **Jonathan** (flagged, not findings) |
| ✅ the +6 delta, counted at source | ⛔ any suite **absolute**, any `HEAD`-vs-tree fact ⇒ `TASK-987`'s build-master (I have no Git — §0) |

---

## Notes for build-master

- ⛔ **STAGE `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` BY EXPLICIT PATHSPEC** (WARN-3). It is new and untracked; so are its dependencies `Siegebound/FogVolume.{h,cpp}` and `Tests/SiegeBrightSunTest.cpp`. Read `git status` back for `??` under `Siegebound/` before you commit — a gate that commits as absent is invisible forever.
- ⛔ **This row has NEVER been compiled or executed by anyone.** Declared suite delta **+6** (measured by me at source: 6 macros, 6 unique registered names). ⛔ **Publish no absolute** — the tree is contaminated by concurrent lanes (`TL-§5c`).
- ⛔ **Serialisation still stands:** `TASK-989` → `TASK-991` (consumes `WholeSecondsText`) → `TASK-1018`. `TASK-991` was held for this gate; it is now free on the file. `WholeSecondsText` is **public, static and pure** at `SiegePlayerController.h:934` — `TASK-991` calls it **twice** and must **not** generalise the `CardRefused_BrightSunActive` message (arity fence, asserted in test 6).
- ⛔ **First compile of this batch is where a real error will surface**, and the likeliest one is not in this diff but in the batch it rides: `SiegePlayerController.cpp` now `#include`s `Siegebound/FogVolume.h`, which is itself **untracked new code from `TASK-982`**. If `FogVolume.h` does not reach the compiler, this file fails on a missing header — **that would be a staging failure, not a defect in `TASK-989`.**
- 🧑 **Two things for Jonathan at the checkpoint, neither blocking:** the **display floor** (WARN-1) and the **≈1.8 s toast lifetime** vs a number he is meant to read (WARN-4).

## ⛔ ONE LINE ON THE COMMIT

**Nothing in this row blocks the commit** — 0 blockers, the value is provably live at the click, and the only commit-time hazard is **staging the untracked new test file by pathspec** (WARN-3).

---

**`TASK-989`'s status line (NOT flipped by me, as instructed): `.claude/pipeline/TASKBOARD.md:18523`.**
