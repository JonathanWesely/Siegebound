# TASK-989 — [BS-REFUSE] the fog-during-prevention refusal message

**Author:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** `ready-for-qa` · **Gate:** `TASK-990` · **Ship host:** `TASK-987`
**Law:** `FOG-§10.6` · `FOG-§10.2` · `FOG-§10.3` · `SC-§37` · `SC-§40` cl. 2 · `SC-§53` cl. 3 · `SC-§65` · `SC-§77` · `TL-§5b`/`§5c`
**Rulings built to (⛔ not re-derived):** ✅ `J-F19` · `J-F24` · `J-F25` · `J-F26`
**Input consumed:** `handoffs/TASK-982-programmer.md` (its §5 pinned signature block)

---

## §0 — ⛔ READ FIRST: THE FOUR THINGS THE GATE MUST NOT TAKE ON TRUST

1. **⛔ THE VALUE IS LIVE, AND "live" is pinned in a form that can fail.** `GetFogPreventionSecondsRemaining()` occurs **exactly once** in `SiegePlayerController.cpp` (measured, not recalled), its result is bound to a **local**, and **that same local** is the message's format argument — both statements are pinned character-for-character. The controller header declares **no** stored remainder (`PreventionSeconds` = 0, `FogPrevent` = 0 on code lines). ⛔ A build that read the accessor and then formatted something else dies on the second pin.
2. **⛔ I DID NOT TOUCH THE `GoldSteal` ROUTING PREDICATE.** `if (Row->SpellEffect == ESpellEffect::GoldSteal)` is censused at **1** inside `PlayHandSlot` and that census is an assertion in the new suite, so the fence is checkable rather than promised. ⭐ `TASK-1018` owns that defect and **must** consume `TASK-999`'s delivery-derived predicate; §4 explains why my refusal does not depend on the routing at all, in either direction.
3. **⛔ SUITE DELTA ONLY: +6 registered automation tests**, all in the **new** `Tests/SiegeFogRefusalTest.cpp` (counted at source: 6 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, 6 `"Siegebound.FogRefusal.…"` names). **⛔ NO absolute is claimed. I ran no compile and no suite; nothing in this batch has been executed.** I added **no** rows to any existing test file, and **moved no pin anywhere**.
4. **⛔ `Tests/SiegeBrightSunTest.cpp` IS BYTE-UNTOUCHED, AND THAT IS A DECISION, NOT AN OVERSIGHT** — the row told me to look there first, and I did. §6 gives the reason in full: **its gate `TASK-986` has not returned**, so adding to it would have mutated a review subject mid-review.

---

## §1 — WHAT SHIPPED (3 files, 1 of them new)

### `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`

| # | change | what it is |
|---|---|---|
| 1 | `#include "Siegebound/FogVolume.h"` | ⛔ READ-ONLY use, stated in the include comment: the **read** door `Find` and one `const` accessor. This controller never spawns, mutates or resets a fog volume. |
| 2 | the guard in `PlayHandSlot`, between the affordability gate and the accept stinger | the whole refusal — **one** call site, **one** format string |
| 3 | `FText ASiegePlayerController::WholeSecondsText(float Seconds)` beside the five shipped refusal-text statics | ⭐ the **shared scalar formatter** (`TASK-991` consumes it) |

### `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- the `WholeSecondsText` declaration + its doc, immediately after `StackHeightCapNoticeText()` (same section, same `public:` block, same shape as its five siblings).
- **`PlayHandSlot`'s own doc gained the new refusal** (`SC-§65`): that doc enumerates the refusals this entry raises, and a shipped enumeration that silently omits a new member is the drift defect the section exists to stop.

### **NEW** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` — 6 tests (§5)

---

## §2 — THE SHIPPED STRING, AND WHY IT IS SPELLED THAT WAY

```cpp
NSLOCTEXT("Siegebound", "CardRefused_BrightSunActive", "Bright Sun is still up for {0}")
```
`{0}` is `WholeSecondsText(PreventionSecondsRemaining)` ⇒ the player reads **"Bright Sun is still up for 143 seconds"**.

- 📌 It is **his sentence**: *"a message telling them bright sun is still up for 'x' amount of seconds."*
- ✅ **`J-F24` — WHOLE SECONDS, ROUNDED, EVEN PAST 60.** `60` reads "60 seconds", `120` reads "120 seconds", `143` reads "143 seconds". ⛔ The suite asserts the message does **not** contain the substring `"minute"`, which is the one form he ruled out.
- ⛔ **The unit word lives INSIDE the formatter, not at the call site.** `J-F24` rules **units *and* rounding**, and the two must move together — that is what makes his retune the one word the row promised. It also lets the singular be handled once, and keeps `TASK-991`'s two-value sentence readable ("from 143 seconds to 83 seconds").
- ⭐ **The singular gets its own key** (`SecondsCount_One` ⇒ "1 second"). ⛔ **Deliberately NOT an ICU `|plural(…)` form**: this project ships no localisation and no plural-form string anywhere, so a hand-rolled plural pattern would be the codebase's only instance of an idiom that **fails at runtime rather than at compile time** — and I cannot compile or run.

### ⭐ THE ONE SUB-DEFAULT I ADDED, DECLARED RATHER THAN SLIPPED IN: THE DISPLAY FLOOR

`WholeSecondsText` clamps to **1** when its input is **strictly positive**. ⛔ **It is not a change to his rounding**: it can only fire in the `(0, 0.5)` band, and it never touches a value at or above half a second. The reason is that the *caller refuses the card* for exactly that band, so plain rounding would print **"Bright Sun is still up for 0 seconds"** while refusing the card for that very window — a message that contradicts its own refusal in one breath. A true zero still reads "0 seconds" (that path returns before the floor), so the floor can never invent time out of nothing. **⚖️ If the gate reads this as improvisation it is a two-line revert, and the test row that proves it is labelled.**

---

## §3 — ⛔ WHY THIS IS THE FIRST REFUSAL WHOSE VALUE CHANGES BETWEEN TWO CLICKS, AND HOW THAT IS HELD

`CardRefused_MaxStacks` — the pattern I was told to copy, **verified at source before copying** — formats a value that is *constant for the card* (its `DisplayName`). Mine changes every tick. The whole risk of this row is a value that is read once and reused.

```cpp
if (Row->SpellEffect == ESpellEffect::FogCover)
{
    if (const AFogVolume* const FogState = AFogVolume::Find(GetWorld()))
    {
        const float PreventionSecondsRemaining = FogState->GetFogPreventionSecondsRemaining();
        if (PreventionSecondsRemaining > 0.f)
        { … RefuseCardPlay(CardID, FText::Format(…, WholeSecondsText(PreventionSecondsRemaining))); return; }
    }
}
```

- ⭐⭐ **ONE READ, NOT TWO — and this is the design decision in the diff.** The refusal is gated on **the remainder itself**, not on `IsFogPrevented()`. So the number the player is shown is **bit-for-bit** the number the refusal was decided on: there is no window, however small, in which the predicate and the message could describe different instants. `TASK-982`'s accessor returns `0` whenever the machine is not `SHIELDED`, so `> 0.f` **is** the `SHIELDED` predicate — the two cannot disagree about which state the machine is in, by construction of that accessor.
- ⛔ **The READ door, never the WRITE door.** `AFogVolume::Find` (not `FindOrSpawn`): a refusal pre-check may not create the fog actor as a side effect of saying no. `AFogVolume::FindOrSpawn(` is censused at **0** in this file.
- ⛔ **Nothing is cached anywhere.** No member, no static, no `mutable`. Censused in the header, which is where a cached remainder would actually have to live.

---

## §4 — ⭐⭐ THE ONE JUDGEMENT CALL: I REFUSED AT THE **ENTRY**, NOT AT THE RESOLVER. QA SHOULD PRESS HERE FIRST.

The `Fog` card can be committed through two shipped resolvers (`ResolveSpellInstant`, `TryConfirmSpellTarget`), both of which **deduct then resolve** and refund on a resolver `false`. I did **not** put the message there. I put it at the card-play click, in `PlayHandSlot`, between the affordability gate and the accept stinger.

**Four reasons, in the order they decided it:**

1. ⭐⭐ **HIS TWO DEFAULT PROPERTIES BECOME STRUCTURAL INSTEAD OF TRANSACTIONAL.** Measured in `PlayHandSlot`'s body: `SpendGold` **0**, `AddGold` **0**, `ConfirmInstantDraw` **0**, `ConfirmPlayFromHand` **0**, `DiscardFromHand` **0**. Nothing is spent, so there is nothing to refund and no instant in which the player's gold is briefly lower; nothing is consumed, so the card is still in hand because it never left. `FOG-§10.6` property 1 and property 2 are asserted **separately** — a build that refunds but eats the card satisfies half his ruling, and a test written against either half alone passes it.
2. ⛔⛔ **IT IS ROUTING-AGNOSTIC, WHICH MATTERS BECAUSE THE ROUTING IS KNOWN-WRONG TODAY AND WILL CHANGE.** Today `Fog` wrongly enters **targeting** mode (`TASK-1018`); after that row lands it resolves **instantly**. A refusal in either resolver would have to be written twice or written to the wrong one. This gate fires identically before and after, and **`TASK-1018` needs no edit in my code** — which is the point of splitting the two rows in the first place.
3. ⛔ **THE PLAYER NEVER ENTERS A MODE HE CANNOT COMPLETE.** Under today's routing, refusing at the confirm would put the player into targeting mode with a reticle for a map-wide effect, let him aim it, and then refuse.
4. ⛔ **NO ACCEPT STINGER ON A REFUSED PLAY.** The guard sits **before** `PlaySound2D(this, CardPlaySoundPath)` — asserted — so the card-play sound does not contradict the toast. It sits **after** `CanAfford`, preserving §3.5's shipped precedence that the gold refusal outranks every type refusal.

### ⚠️ THE RESIDUAL THIS CREATES — DECLARED, MEASURED, AND CLOSED BY A BOARDED ROW

**Under today's (defective) routing there is a window my guard does not cover:** the player clicks `Fog` while the field is clear, enters targeting mode, and the opponent's `BrightSun` lands **before** he confirms. That confirm reaches `ResolveSpell` → `RaiseFog()` refuses (`TASK-982`'s `J-F19` guard) → the player gets the generic **"Spell fizzled"** with a **full refund and the card kept**.

- ⛔ **His two default properties still hold in that window** — it is only the *message* that is generic.
- ⭐ **The window closes by construction when `TASK-1018` lands**, because `Fog` will then resolve on the click with no targeting mode to sit inside.
- ⛔ **I did not build a second call site for it**, and that is deliberate: the row's spec says *"ONE call site + ONE format string"*, and a second site would be **dead the moment `TASK-1018` ships** — surface built to cover a defect that is already boarded to be deleted.
- ⚖️ **If the gate disagrees, the fix is one `if` in `TryConfirmSpellTarget` and it wants a ruling, not my preference.**

### ⛔ AND THE THING I DID **NOT** MAKE REDUNDANT

`TASK-982`'s guard **inside** `AFogVolume::RaiseFog()` is untouched and is **not** dead. It is the state object's own rule, and it still covers the **bot**, which reaches `USpellLibrary::ResolveSpell` without ever passing through `PlayHandSlot`. My guard is the *player-facing message*; that one is the *invariant*. ⛔ Removing either would be wrong.

---

## §5 — THE NEW TESTS (**+6**) — `Tests/SiegeFogRefusalTest.cpp`

| # | registered name | what it can catch |
|---|---|---|
| 1 | `…TheCountdownIsWholeRoundedSecondsEvenPastSixty` | ⭐ **executed by direct call.** His worked example re-derived (`2 × 60 + 23 = 143`) ⇒ "143 seconds"; the string must **not** contain `"minute"`; 60 and 120 stay seconds; **42.4 ⇒ 42 and 42.6 ⇒ 43** (the only pair that can tell round from floor *and* from ceil); the singular; 0 / negative / **bit-pattern NaN with an `IsFinite` self-check**; and the display floor at 0.2 s ⇒ "1 second" |
| 2 | `…TwoRefusalsASecondApartCannotShowTheSameNumber` | ⭐⭐⭐ **the one that decides the row**, in two halves that fail on different things — **executed:** the same window clicked twice one second apart yields "143 seconds" vs "142 seconds" and they must differ (a formatter that ignored its argument, or a static *"Bright Sun is still active"*, is red **here and only here**); **structural:** the accessor at **1** in the function and **1** in the file, the local binding and the format argument pinned verbatim, and **no stored remainder** in the header |
| 3 | `…ARefusedFogSpendsZeroGoldAndKeepsTheCardInHand` | property 1 and property 2 as **separate** assertions (`SpendGold`/`AddGold` at 0; `ConfirmPlayFromHand`/`ConfirmInstantDraw`/`DiscardFromHand` at 0) **plus the order that makes them true**: the refusal precedes the routing switch, precedes the accept stinger, and follows `CanAfford` |
| 4 | `…PreventionRefusesFogAloneAndOnlyWhileTheWindowIsUp` | ⭐ `J-F26` (one effect gate, one refusal, no CardID literal) + ⛔ **the negative control**: the region between the gate and the message must contain `PreventionSecondsRemaining > 0.f` and `AFogVolume::Find(GetWorld())`, and **the routing switch must still follow the guard** — without that last row, a build that ended the function at the refusal would pass every other test in this file while making `Fog` unplayable forever. Also `FindOrSpawn` at 0, and **the `:1037` fence as an executable census** |
| 5 | `…TheMessageRidesTheOneShippedRefusalSurface` | ✅ `J-F25` **measured**: the copied precedent still exists at source; the same `RefuseCardPlay` + `FText::Format` idiom; **no** new delegate, **no** `CreateWidget`; and the surface traced through all three hops (`RefuseCardPlay` → both delegates → `UCardHandWidget` 1:1 forward) |
| 6 | `…TheSecondsFormatterIsAScalarSeamWithALiveCaller` | `SC-§40` cl. 2 (defined **and** called), one declaration, and ⭐⭐ **the arity fence**: the formatter contains no `BrightSun` and no `CardRefused_`, so `TASK-991` can reuse the scalar without inheriting this row's wording; plus purity (no world, no fog state) |

**⛔ EVERY numeric pin in this file was MEASURED against the shipped bytes before it was typed** — with a script re-implementing `CountOccurrencesInCode`'s comment-skipping rule and `ExtractFunctionBody`'s signature/column-0-brace rule, run over the edited source. **38 source-text measurements backing those pins, all green.** ⛔ That is a measurement of the *source text*, ⛔ **not** an executed suite: nothing here has been compiled or run.

**⭐ TWO PINS I DELIBERATELY DID NOT WRITE, with the reason recorded in the file so nobody "completes the census":**
- the **total** `ESpellEffect::` count in `PlayHandSlot` — ⭐ `TASK-1018` changes it;
- `ESpellEffect::FogClear` at **0** — ⭐ `TASK-991` adds it.
⛔ Either pin would go **red on a correct successor diff**, and a pin that punishes a correct diff teaches the next author to weaken tests.

**⚠️ ONE PIN THAT *WILL* NEED MOVING, AND IT SAYS SO IN PLACE:** test 4's `GoldSteal`-predicate census is the executable form of *"this row did not touch `:1037`"*. Its comment instructs ⭐ `TASK-1018` to **re-point it, not delete it** — the claim is *"the routing is derived in exactly one place"*, and that claim outlives the spelling. This is the `SiegeFogVolumeTest.cpp` *"when `FogClear` is appended below, this row moves to it"* precedent, used on purpose.

**⚠️ THE DECLARED GAP, stated rather than discovered:** there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so the true end-to-end claim — click `Fog` twice a second apart during a live window and see two different toasts — is **not executed**. Half of it is (the formatter's injectivity across a second of elapsed time); the other half (the argument is a live read) is structural. `TASK-982` declared the identical gap for the same reason.

---

## §6 — ⛔ WHY I DID **NOT** ADD ROWS TO `Tests/SiegeBrightSunTest.cpp`

The row said *"CHECK FOR AN EXISTING FRAME FIRST, never a duplicate"* and named that file. I checked it, read it, and did not write in it:

1. ⛔⛔ **ITS GATE HAS NOT RETURNED.** `TASK-982` shipped it `ready-for-qa` under gate `TASK-986`, which is still `boarded`. Adding assertions to it now would mutate a **review subject mid-review** — the gate-integrity failure ruled on 2026-09-04, arriving through a *second author* rather than through a later edit. ⭐ `TASK-982` was permitted to edit `SiegeFogVolumeTest.cpp` for the **opposite** reason, and said so: that file's verdict had already been returned.
2. ⛔ **DIFFERENT SUBJECT.** That file is about `AFogVolume` — the state machine, the formula, the one-way door. This one is about `ASiegePlayerController` — the refusal, its wording, its ordering and its surface. **Not one assertion is duplicated between them.** This is not a second file testing the same thing, which is what "duplicate" means.
3. ⭐ **AND IT IS THE FRAME `TASK-991` SHOULD FIND.** The sun-on-sun refusal is this refusal's sibling — same function, same formatter, same surface — so `Siegebound.FogRefusal.*` is where it belongs, and this file existing is what stops it creating a third one.

⛔ `Tests/SiegeBrightSunTest.cpp`, `Tests/SiegeFogVolumeTest.cpp`, `Tests/SiegeFogTest.cpp`, `Tests/SiegeFogClampTest.cpp` and `Tests/SiegePlacementTest.cpp` are **byte-untouched**.

---

## §7 — ✅ `J-F25`: THE SURFACE, **MEASURED** — AND THE CONCERN I AM REPORTING AND NOT ACTING ON

**Measured, hop by hop, at source (not assumed):**

`RefuseCardPlay(CardID, Reason)` → `OnCardPlayRefused.Broadcast(CardID, Reason)` **(1)** + `BroadcastRefusal(Reason)` **(1)** → `OnCardRefused.Broadcast(Reason.ToString())` → `UCardHandWidget::HandleCardRefused` → `OnCardRefusedMessage(Reason)` **(1, a 1:1 pass-through)** → **`WBP_CardHand`**, which owns the show-then-hide.

⇒ ⛔ **No new path, no new broadcast, no new widget, no new toast.** My message rides exactly what *"Not enough gold"* rides.

⚠️⚠️ **THE CONCERN, RECORDED AND NOT ACTIONED:** that surface's lifetime is **≈1.8 s** (`VID-005`, measured on two independent bursts) and *"~2 s"* by `CardHandWidget.h`'s own doc — **two independent sources agreeing**. ⛔ **That is short for a number he is meant to READ**, and this is the first message in the game that carries one. ⛔ **I did not change it, and it is not assertable from C++ in any case** — the timer lives in a Blueprint asset. ⛔ **Changing it is a game-wide UI change and therefore Jonathan's call, not this lane's** (`J-F25`, closed). ⇒ **Re-raise at a checkpoint, never as a task.**

---

## §8 — ⛔ FINDINGS I AM **REPORTING, NOT FIXING** (all outside my fence)

1. **🚨 `SiegePlayerController.cpp`'s spell routing is a BLACKLIST OF ONE** — `GoldSteal` and nothing else — so `Fog` (and `BrightSun`, once `TASK-983` lands its row) enters **targeting mode** and the player aims a reticle at a map-wide effect, contradicting `FOG-§10.1`. ⛔ **NOT TOUCHED**, by explicit fence: it is ⭐ `TASK-1018`, which must consume ⭐ `TASK-999`'s **delivery-derived** predicate rather than re-derive one — two derivations of *"has a reticle"* diverge, and the divergence presents as two bugs instead of one. ⭐ `TASK-982` §8(1) reported the same finding independently; this is a **corroboration**, not a second sighting to board.
2. **⚠️ `CardRefused_SpellFizzled` is the message a `Fog` refused *at the confirm* still shows** (§4's residual). Generic, but net-zero and correct in gold and card. ⛔ Closes by construction with `TASK-1018`; ⛔ no row needed unless the gate disagrees with §4.
3. **`SpellLibrary.h`'s class-doc effect list still omits `FogCover` and `FogClear`** — inherited, reported by `TASK-982` §8(3) as well; ⛔ not my file, ⛔ not swept.
4. **⭐ FOR `TASK-991`, so it does not have to re-derive any of this:** the shared formatter is `ASiegePlayerController::WholeSecondsText(float)` — **public, static, pure, scalar-in/`FText`-out, and it already spells the unit word**. ⛔ Call it **twice** (once for `X`, once for `Y`). ⛔ Do **not** generalise my `CardRefused_BrightSunActive` message — it is inline at the call site precisely so there is no one-arity builder to be tempted by. ⛔ Its `X` is the same `GetFogPreventionSecondsRemaining()` read live at *its* click, and its `Y` is `TASK-982`'s `GetBrightSunWindowSeconds(CasterTeam)`. ⛔ **My row adds NO `FogClear` handling anywhere in this file — that whole arm is yours, and the file is clear of it (`ESpellEffect::FogClear` is at ZERO here today).**

---

## §9 — ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **⛔⛔ THE ENTRY-vs-RESOLVER PLACEMENT (§4) AND ITS DECLARED RESIDUAL.** This is the one architectural choice in the diff. Press on the in-targeting-mode window and decide whether the generic *"Spell fizzled"* there is acceptable until `TASK-1018`. I argue yes, and say why; ⚖️ it is a ruling, not a preference.
2. **⛔ THE DISPLAY FLOOR (§2).** A clamp to 1 second on a strictly positive input. I hold it is a display rule, not a change to his rounding, and it is confined to the `(0, 0.5)` band. If the gate reads it as improvisation it is a two-line revert.
3. **⛔ THE PREDICATE IS THE REMAINDER, NOT `IsFogPrevented()`.** Verify you agree that one read is stronger than two (§3). It rests on `TASK-982`'s documented contract that the accessor returns `0` exactly when the machine is not `SHIELDED` — ⛔ if `TASK-986` finds that contract broken, **this row's predicate breaks with it**, and that is the one way my diff can be wrong through no fault of its own.
4. **⛔ THE EFFECT GATE IS `ESpellEffect::FogCover`, NOT `CardID == "Fog"`.** Data-driven, and narrower than it looks: it refuses exactly the effect that raises fog. `J-F26` says *"`Fog` only"* and today `FogCover` **is** `Fog`. ⚖️ If a future card ever also raises fog it would also be refused — which I hold is correct rather than a scope widening, but it is the kind of thing that reads right and could be wrong.
5. **⛔ THE SINGULAR KEY.** Three `NSLOCTEXT` keys ship in the formatter (`SecondsCount_Zero` / `_One` / `_Many`) where an ICU plural form would use one. §2 gives the reason (an untestable runtime-parsed idiom, in a row that cannot compile or run). Overrule if you disagree; it is cheap now.
6. **⛔ THE NEW TEST FILE'S EXISTENCE** (§6) — the row said "check for an existing frame first". I did, and declined it on gate integrity. If you disagree, the fix is a file move after `TASK-986` returns, not a rewrite.

---

## §10 — SCOPE DISCIPLINE

**⛔ NOT DONE, by fence:** no compile · no editor · no MCP · no mutating Git (read-only `git diff`/`status` only, `SC-§71a`) · ⛔ **no edit to `AFogVolume` of any kind — both accessors were CALLED, never CHANGED** · ⛔ **no touch to the `GoldSteal` routing predicate (`TASK-1018`)** · no `J-F18` branch and no second-value message (`TASK-991`) · no toast lifetime or styling (**report only**) · no other refusal's wording · no card row (`TASK-983`) · no card art (`TASK-984`) · no visual (`TASK-841`) · no curve (`TASK-981`) · no firing/notice ranges (`979`/`980`) · no `SpellLibrary.{h,cpp}` · no `cards.csv` / `DT_Cards` · ⛔ no line numbers or row counts written into source (`SC-§77` — every reference in the diff is by **key**).

**Files changed (3):**
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp`

Plus `.claude/pipeline/TASKBOARD.md` — **only** TASK-989's own `status:` line, re-read immediately before the edit and grepped back out after (1 occurrence; `TASK-986`, `TASK-999`, `TASK-983`, `TASK-991` and `TASK-1018` rows untouched).

⚠️ **`Tests/SiegeFogRefusalTest.cpp` is UNTRACKED in git** (`??`, not `M`), as are `TASK-998`/`TASK-982`'s `FogVolume.{h,cpp}` and `Tests/SiegeBrightSunTest.cpp` / `Tests/SiegeFogVolumeTest.cpp`, which my file's probes depend on being present. **That is expected and is not a sign the work is missing** — they enter the repository for the first time with ⭐ `TASK-987`.
