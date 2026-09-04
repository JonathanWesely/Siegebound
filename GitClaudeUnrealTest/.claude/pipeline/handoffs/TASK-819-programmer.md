# TASK-819 — the discard-all mechanic. ⭐ ONE route (`H`), ONE entry point, ONE charge.

**Agent:** gameplay-programmer · **Date:** 2026-09-03 · **Status:** `ready-for-qa` → gate **TASK-822**
**Law:** `CARDBAR-§6` (the pinned names, the ladder, the fee) · `CARDBAR-§7` (the mapped-vs-polled lane) · `CARDBAR-§10` (the economics flag) · `SC-§37` · `HIGH-§1`
**Compile / editor / MCP / Git:** ⛔ none run. Not mine.

---

## 0. ⚡ THE SCOPE CUT LANDED MID-TASK AND IT SHAPED THE WHOLE DELIVERABLE

Jonathan scrapped the right-click route while I was reading the law, before I had written a line
of it. His words, relayed verbatim by the orchestrator:

> *"we dont really need the entire card area to be right clickable to discard if we have a key for
> it, and having that feature will maybe interfere with a player trying to right click to stop a new
> spawn of a unit or something, so lets just scrap that right click feature to discard cards and keep
> it just to the 'H' key."*

⭐ **Consequence for review: there is EXACTLY ONE route and NO widget-facing surface anywhere.**

- ⛔ **Nothing was written and then removed.** The cut arrived before the pointer half existed, so
  there is no residue to hunt: `UCardHandWidget::NativeOnMouseButtonDown` and `RequestDiscardAll`
  were **never authored**. `CardHandWidget.{h,cpp}` is **byte-identical to what TASK-807 left**.
- ⛔ **The `FReply` rule is therefore MOOT for this task** — it governed an override that does not
  exist. `DECK-§5`'s contrast is moot with it. I have not written either into the code.
- ⛔ **`CARDBAR-§12` / TASK-808's hit-test finding / the one-node `Image` flip / TASK-825** are all
  moot as inputs to this task. I built against none of them.
- ⭐ **And the cut deleted a real hazard rather than merely simplifying:** the override was pinned to
  the **bar**, so it would have fired for anything that bubbled to it — including the bottom-right
  **next-card preview**, i.e. **20 gold for right-clicking a card that is not in your hand**
  (`CARDBAR-§12c` had boarded that as a pixel row). That class is gone, not mitigated.

**Test 23 is the regression guard that keeps it scrapped** — it measures, from inside my fence, that
`CardHandWidget.{h,cpp}` declares no mouse handler, holds no `RequestDiscardAll`, and never names
`DiscardEntireHand`. ⚠️ If that test ever goes red it is **not automatically a bug**: it means
somebody re-added the route, and the correct response is to confirm **he asked for it** and then
update the test deliberately. It is not a line to quietly delete.

---

## 1. FILES TOUCHED — three, exactly the fence

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | `DiscardEntireHand()` · `DiscardAllCost` · `DiscardAllAction` · `DiscardAllActionAsset` · `OnDiscardAllPressed()` · retirement notes on `DiscardHandSlot` + `DiscardCost` |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | the soft path · the resolve · the bind · `DiscardEntireHand()` · `OnDiscardAllPressed()` · one retirement line on `DiscardHandSlot` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` | **+6 tests** (18–23) in a new `SiegeDiscardAllFixture` namespace, and a charter note declaring them as lodgers |

⛔ **Not touched:** `CardHandWidget.{h,cpp}` · `DeckComponent.{h,cpp}` (**called, not changed** — no
batch-discard helper, no new API) · `SiegeControlsHelpWidget.*` (TASK-821's) · any `.uasset` ·
`IMC_Hero` · `SiegeBotController.*` (`CARDBAR-§10` row `J-20` is his).

---

## 2. THE SINGLE ENTRY POINT, AND HOW THE ONE ROUTE REACHES IT

```
IA_DiscardAll  (TASK-820: IMC_Hero 27 → 28, appended at the H position)
      │  Enhanced Input, ETriggerEvent::Started
      ▼
ASiegePlayerController::OnDiscardAllPressed()     ← private; carries ZERO logic
      │  one statement: DiscardEntireHand();
      ▼
ASiegePlayerController::DiscardEntireHand()       ← UFUNCTION(BlueprintCallable, "Siegebound|Cards")
```

`OnDiscardAllPressed` adds **not one line** to the ladder. Test 23(b) asserts that structurally: the
handler forwards once and contains **zero** occurrences of `HasAuthority`, `bInPlacementMode`,
`SpendGold` or `DiscardFromHand`.

**`DiscardEntireHand` stays `BlueprintCallable`** — it is the pinned signature (`CARDBAR-§6`) and it
matches its siblings `PlayHandSlot` / `DiscardHandSlot`. ⚠️ It is **not** a "dead entry point left in
case": it is the entry point, and it has exactly one live caller.

---

## 3. ⛔⛔ PROOF THE GOLD MOVES **ONCE** — three independent instruments

**(1) The code.** One statement charges, and it names the receiver:

```cpp
if (!SiegeState->SpendGold(DiscardAllCost))   // ← the ONLY charge in the function
```

then the loop is over the **deck**, never the per-slot entry point:

```cpp
for (const int32 Slot : OccupiedSlots)
{
    if (!DeckComponent->DiscardFromHand(Slot)) { /* loud Error tripwire */ continue; }
    ++DiscardedCount;
}
```

**(2) Test 21 — the source probe, with a self-checked scanner.** Over the extracted body of
`DiscardEntireHand`, on **code lines only** (the comment-skipping predicate is
`SiegePlacementUpgradeFixture::CountOccurrencesInCode`, reused verbatim):

| claim | value |
|---|---|
| `SiegeState->SpendGold(` | **exactly 1** |
| `DiscardHandSlot(` | **0** ← the automatic-FAIL bug |
| `DiscardCost` | **0** ← the wrong fee is not wired in |
| `DeckComponent->DiscardFromHand(` | **exactly 1** |
| `PlaySound2D(this, CardDiscardSoundPath)` | **exactly 1** ← one sound, not six |

plus the **order chain**, also on code lines:
`!HasAuthority()` → `bMatchEnded` → `bInPlacementMode` → `bInTargetingMode` → `!DeckComponent` →
`DiscardAllRefused_EmptyHand` → `SiegeState->SpendGold(` → `DeckComponent->DiscardFromHand(` →
`PlaySound2D(...)`.

⭐ **The tokens are chosen so log text cannot be miscounted as code.** Eight `UE_LOG` format strings
in the function say `"DiscardEntireHand() refused…"` and the tripwire string says
`"DiscardFromHand(%d) refused AFTER SpendGold(%d)"` — all on **code** lines. A bare `SpendGold(` or
`DiscardFromHand(` would have counted those. Naming the receiver (`SiegeState->` /
`DeckComponent->`) makes each count exact **and** says something extra: the charge goes to the player
state, the loop goes through the deck component.

⭐ **The scanner self-check is the sharp half** (the TASK-813 test-17 device): `DiscardCost` **is**
genuinely present in this body — in **prose**, warning against exactly the bug — so the zero on code
lines is the comment-skipper working, ⛔ not the token being absent. `DiscardAllCost` does **not**
contain the substring `DiscardCost` ("Discard" + "All" + "Cost"; the `C` never follows the `d`), so
the count is asked directly rather than by subtraction.

**(3) Test 19 — the behaviour, on the real model, with the loop bug built beside it.** A world-free
`ASiegePlayerState` + `UDeckComponent`, a **full hand of six distinct cards** asserted before
anything happens, then the shipped sequence. Assertions: gold moved by **exactly** `DiscardAllCost`;
six slots binned; discard pile +6; every slot non-empty again; **not one** of the six binned cards
still in hand. Then the **negative control**: the same fixture charged the loop-bug way
(`DiscardAllCost` + 6 × `DiscardCost`) and asserted to differ — ⛔ without which claim (a) would be
green under the bug too.

---

## 4. THE FOUR RULINGS (+ one I had to make)

| # | question | ruling | where it lives |
|---|---|---|---|
| 1 | fewer than 6 cards — still a flat 20? | ✅ **YES, flat.** One card costs the whole fee. `CARDBAR-§10` prices a hand **reset**, not a per-card cycle, and the per-card lever is gone at any price. | the single `SpendGold(DiscardAllCost)`; test 20(a) |
| 2 | cannot afford — refuse, and how does it read? | ✅ **Refuse, net-zero.** `SpendGold` refuses below the fee with no change and no broadcast, and the player sees the **shipped** `CardRefused_CantAfford` line **"Not enough gold"** — ⛔ **reused, not reworded**. A second wording of a shipped refusal is a UI regression wearing a feature's clothes. | test 20(c), boundary-exact |
| 3 | does the hand refill immediately? | ✅ **YES — and it is not a ruling, it is shipped behaviour.** `DiscardFromHand` → `MoveHandCardToDiscardAndRedraw` pushes to the discard pile **and redraws that slot in the same call** (§3.4). ⛔ No new `DeckComponent` API, ⛔ no batch broadcast. | test 19(c) |
| 4 | right-click while a placement ghost is up? | ⛔ **MOOT** — he scrapped right-click. Stated rather than answered. | §0 |
| 5 | *(mine)* the empty-hand refusal text | ⚠️ **A new line was genuinely needed** — the shipped `CardRefused_EmptySlot` says *"No card in that hand slot"*, which is about **one** slot and would be wrong here. New key `DiscardAllRefused_EmptyHand` → **"No cards to discard"**. ⛔ The only new string in the feature; every other refusal is reused. | test 21(f) asserts the three reused keys are reused, **and** that `DiscardHandSlot` still carries them |

---

## 5. ⚠️ FOR QA — the things worth your scrutiny, named rather than buried

1. **⭐ THE TEST FILE IS A DECLARED LODGER, AND I WANT A RULING ON IT.** The discard-all is not a
   placement mechanic, and `SiegePlacementTest.cpp`'s own charter says it *"is named for the
   mechanic"*. It lives there because my dispatch fenced me to `SiegePlayerController.{h,cpp}` +
   **that file**, and because the instrument my sharpest claims need — the self-checked
   comment-skipping source probe — is already built there (test 17) and would otherwise be cloned.
   I added a charter note saying exactly this. 🧑 **A future `SiegeCardDiscardTest.cpp` is tidier and
   re-homing costs nothing** — I raised it so it is a decision somebody makes, not a surprise.
2. **⚠️ `RequestDiscardSlot`'s retirement COMMENT is OWED and is NOT mine.** `CARDBAR-§6` retires
   three symbols with one comment line each. I wrote two (`DiscardHandSlot`, `DiscardCost` — both in
   my fence). The third lives in `CardHandWidget.h`, which my fence excludes. ⭐ **I closed the half I
   could from inside the fence:** test 23(e) asserts `UCardHandWidget::RequestDiscardSlot` still
   **exists**, so the "retired, ⛔ not deleted" half is enforced even though the prose is not written.
   The comment falls to whoever next edits that file.
3. **⚠️ THE BEHAVIOURAL TESTS DO NOT CALL `DiscardEntireHand`, AND THE FILE SAYS SO IN CAPITALS.** It
   needs a possessed controller with a PlayerState and a live `DeckComponent`, and its success path
   reaches `UGameplayStatics::PlaySound2D`, which resolves a world. Tests 19/20 drive **the same two
   shipped APIs it drives, in the same order**; **test 21 proves the shipped body really has that
   shape**. ⛔ Neither half is worth anything alone, which is why neither is omitted. Rule on whether
   that split is acceptable — it is my main judgment call.
4. **⚠️ THE WORLD-FREE SCRATCH ACTORS.** `ASiegePlayerState` and `UDeckComponent` are `NewObject`ed
   into the transient package (the `MakeScratchBuilding` precedent). `SpendGold` is authority-gated,
   so **test 19 asserts `HasAuthority()` as an explicit PRECONDITION** (cloned from
   `SiegeBuildingStackTest.cpp:834`) — if the engine ever changes the default actor role the suite
   says *why* it went red instead of reporting a phantom economy bug. The hand and draw pile are
   seeded through reflected `TArray<FName>` fields with the **inner type checked**, because a renamed
   `Hand` would otherwise leave the fixture with an empty hand — the exact trivial pass.
5. **⭐ THE FIXTURE STOCKS `HandSize + 1` SPARES ON PURPOSE.** With exactly `HandSize`, the last draw
   empties the pile, the §3.4 eager reshuffle folds the discard pile straight back in, and both
   "the discard pile grew" and "no binned card came back" would measure the reshuffle instead. ⚠️ That
   reshuffle is **correct shipped behaviour** (`CARDBAR-§6`'s physical-card rule) — the fixture simply
   must not sit on top of it. This is `CARDBAR-§6`'s "44 cards left after the deal" fact at fixture
   scale.
6. **⚠️ TWO ASSERTIONS I WROTE AND THEN KILLED, DECLARED BECAUSE THE BRIEF WARNED ABOUT EXACTLY THIS.**
   My first draft of test 20 asserted (i) `SpendForOne == SpendForFull` and (ii) `GetGold() ==
   GoldBefore` with nothing in between. **Both were tautologies** — (i) because both passes call
   `SpendGold` once by construction, (ii) because nothing ran between the two reads. Replaced with:
   (i) a **binned-count discriminator** proving the two passes did different work before the equal
   charge means anything, and (ii) a **bypass control** proving the balance *can* move, so "zero
   moved" is the guard working rather than a frozen fixture. Named here so the fix is auditable.
7. **⛔ THE OBSERVER-LOCKOUT BRANCH IS A DIFF READ, NOT A TEST.** `HasAuthority()` is true on a
   world-free actor by construction, so no headless fixture can reach it. It is the shipped branch,
   character-for-character from `DiscardHandSlot`. `SC-§32`: not observed ⇒ not known to function.
8. **⭐⭐ A FINDING WORTH CARRYING FORWARD — `CountOccurrencesInCode` DOES NOT STRIP *TRAILING*
   COMMENTS, ONLY WHOLE COMMENT LINES.** It skips a line that *starts* with `//`, ` * `, `/*` etc.,
   so a warning appended to the **end of a code line** is counted as code. ⚠️ **This bit me and I
   caught it before handing over:** my constructor line originally ended
   `// … ⛔ never a raw EKeys::H poll`, which made test 22(a) read `EKeys::H` **once** on a code line
   and go red — **the "never do X" warning tripping the very test that forbids X.** Reworded to
   `⛔ never a raw key poll; the trap is spelled out on DiscardAllActionAsset in the header`, and the
   prose self-check still holds because the full warning lives on a whole-line comment at
   `SiegePlayerController.cpp:643`. ⛔ **I did NOT "fix" the shared scanner** — it is TASK-813's and
   changing its comment predicate would silently change test 17's meaning. 🧑 **TASK-821 and
   TASK-823 will write probes of the same shape: this is the trap they will hit.**
9. **⭐ HOW I KNOW THE PROBES ARE GREEN WITHOUT COMPILING.** I re-implemented
   `CountOccurrencesInCode` + `CodeLinesOnly` + `ExtractControllerFunctionBody` outside the engine
   and ran **all 46 source-probe assertions** (tests 21/22/23) against the real files. **46/46 pass**,
   including the full ladder ORDER chain, whose measured code-line offsets are 57 → 306 → 495 → 837
   → 1186 → 1900 → 2294 → 2767 → 3216. ⛔ **This is NOT a substitute for the suite run** — it cannot
   compile, and it says nothing about tests 18/19/20, which need the engine. It is what caught
   finding 8.

---

## 6. `CARDBAR-§7` — THE INVERSE TRAP, CLOSED

The key is an **Enhanced Input mapped action**, resolved through the shipped null-safe
`ResolveInputAction` and bound on `ETriggerEvent::Started` — the `IA_CmdAmbush` / `IA_CmdFollow` /
`IA_AssistantConsole` / `IA_WarMap` / `IA_ControlsHelp` shape, character-for-character. A missing
asset leaves the key **inert**, never a crash, and that is the designed compile-time state.

- ⛔ **ZERO raw `EKeys::H` polls** and ⛔ **ZERO `GetPositionalKey` calls** in either controller file.
- ⭐ **Test 22 makes those zeroes mean something, both ways:** the scanner is proven able to find
  `EKeys::` on real code lines (the shipped RMB/Esc polls), **and** both forbidden tokens are proven
  genuinely **present in the files' prose** — so a zero on code lines is the comment-skipper working,
  not an absent token.
- ⭐ **And test 22(c) is the positive half without which deleting the feature entirely would pass
  (a) and (b) perfectly:** the action is resolved exactly once and bound exactly once.
- **Why it matters, restated in-code:** Jonathan plays US-Dvorak and means the **key position** —
  *"on Dvorak it would be 'D'"*. A mapped action inherits the subsystem's wholesale `IMC_Hero`
  retarget for free and lands on the position he means. A raw poll would fire on his physical **`J`**
  — invisible on every reviewer's machine and on every screenshot.
- ⛔ **Zero conditional layout logic. No `if (bIsDigit)`, no per-key special case, anywhere.**

---

## 7. SUITE DELTA — ⛔ A COUNT, NEVER AN ABSOLUTE

**MY DELTA: +6.** `Tests/SiegePlacementTest.cpp` **17 → 23** `IMPLEMENT_SIMPLE_AUTOMATION_TEST`
macros. ⛔ **No new test file. No second placement frame.** ⭐ **+6 is the only number I own and the
only one I stand behind.**

> ### ⚠️⚠️ **AND I WATCHED THE ABSOLUTE GO STALE ⛔ TWICE WHILE WRITING THIS PARAGRAPH — ⭐ THE "COUNT, NEVER AN ABSOLUTE" LAW EARNING ITSELF IN REAL TIME**
>
> - **before my edit:** `338` across **26** files (matches TASK-813's declared post-state)
> - **338 + 6 = 344** — the number I first wrote here, and it was **already wrong when I wrote it**
> - **first live re-census at hand-off:** `361` across **28** files
> - **second re-census, ~one minute later:** **`370` across `29` files**
>
> ⭐ **Cause, measured rather than guessed:** `Tests/SiegeFogTest.cpp` (**+9**) and
> `Tests/SiegeInvisibilityTest.cpp` (**+8**) landed from a task running in parallel with mine, and a
> **29th** file arrived between the two counts. ⛔ **None of it is mine and none of it is a defect** —
> it is simply what a shared census looks like during a multi-lane batch.

⚠️ **⇒ FOR TASK-811 / TASK-824: RECONCILE, ⛔ DO NOT ADD, and ⛔ do not trust `370` either — it was
stale within a minute of being measured.** Take a **fresh** `^IMPLEMENT_` census at compile time and
reconcile it against each task's declared **delta**. ⛔ **Trust no task's absolute, mine included.**

**The six, by name:**

| # | test | what fails it |
|---|---|---|
| 18 | `TheDiscardAllFeeIsItsOwnTunableAndNotThePerCardOne` | aliasing `DiscardCost`; a zero fee; a typo'd `IA_DiscardAll` path that would leave the key silently inert forever |
| 19 | `TheDiscardAllChargesItsFlatFeeExactlyOnceForAFullHand` | any charge ≠ the flat fee; a hand that did not refill; a binned card that came back; a fixture that was never full |
| 20 | `TheDiscardAllFeeIsFlatAcrossHandSizesAndAnEmptyHandSpendsNothing` | a fee that scales with hand size; gold moving on an empty hand; an unaffordable call that moves gold or cards; an off-by-one at the fee boundary |
| 21 | `TheDiscardEntireHandBodyChargesOnceAndLoopsTheDeckNotThePerSlotEntryPoint` | ⛔ **the automatic FAIL** — a second charge, a `DiscardHandSlot` loop, the wrong fee, a reordered ladder, six sounds, a reworded refusal |
| 22 | `TheDiscardAllKeyIsAMappedActionAndIsNeverRawPolledOrRetranslated` | a raw `EKeys::H` poll; a `GetPositionalKey` call; the action never bound at all |
| 23 | `TheDiscardAllHasExactlyOneRouteAndNoDeadPointerSurface` | a second call site; a guard duplicated into the handler; a re-added pointer surface; a deleted retired symbol |

---

## 8. WHAT THE DOWNSTREAM TASKS NEED FROM ME

**🎨 TASK-809 (the card-bar edit) — ⭐ I NEED NOTHING FROM YOU, AND YOU NEED NOTHING FROM ME.**
Your item (8) (the hit-test flip) and its **split valve** are **moot for the discard**: there is no
pointer route to make a surface for. ⛔ **Do not build the flip for my sake.** If it has independent
value, that is a separate call — but the 20-gold mis-click fence in your item (3b) is now
**unnecessary for this feature**, because nothing in the bar can spend gold on a press. ⛔ Add no
mouse handler, and `Btn_Jump` stays untouchable for the reasons that have nothing to do with me.
**TASK-825 should be CLOSED as not-needed** — its whole subject was scrapped.

**⚙️ TASK-821 (the controls menu, card half) — ⭐ WRITE THE `H`-ONLY VERSION. THE VALVE IS SETTLED.**
- Your item **(3b)** asked you to check `handoffs/TASK-809-artist.md` for the flip outcome. ⛔ **Do
  not.** The condition is no longer about the flip — **there is no right-click route at all.**
  ⇒ ⛔ **OMIT the right-click sentence entirely and document `H` alone.** A help screen teaching a
  control that does not exist is `HELP-§2`'s worse-than-nothing case.
- The lane flip stands: `Cards.Discard` **rewritten in place** (⛔ keep the id — `Cards.CursorHold`'s
  `RelatedActionIds` and `SiegeControlsHelpTest.cpp:227`'s `RequiredIds[]` both depend on it),
  `PointerOnly` → `MappedAction`, `Actions = { IA_DiscardAll }`, `bPointerOnly = false`,
  `QwertyReferenceKeys = { EKeys::H }` **as fallback/fixture only**.
- **Behaviour you can now cite as shipped source** (`HELP-§2` mechanism 3 — these are lines I wrote
  and you can read): the whole hand goes at once · the fee is **`DiscardAllCost`**, ⛔ **never typed
  as a number** · the refusal is **net-zero** · a replacement hand is drawn **immediately** · it is
  **refused while placing or targeting**, with those exact shipped sentences · an **empty hand is
  refused before any charge**.
- ⚠️ Your item **(4)** (disambiguating right-click on `Cards.Play` / `Cards.Cancel`) is **moot too** —
  right-click gained no fifth meaning. ⛔ Leave those pages alone.

**⚙️ TASK-815 (the placement wheel) — WHAT YOU MUST KNOW:**
- ⭐ **The controller file has moved under you again. Re-read; trust no line cite** — every number in
  the batch's specs for `SiegePlayerController.cpp` predates me. My additions sit at roughly
  `:235` (soft path), `:515` (resolve), `:652` (bind), `:1172` (`DiscardEntireHand`), `:1318`
  (`OnDiscardAllPressed`) — ⛔ **and those will move again if 821 lands first.**
- ✅ **We are disjoint by symbol.** I own the discard-all and its key lane; you own the wheel and the
  footprint. ⛔ **I edited NOT ONE shipped line** — every controller change is purely additive except
  two doc-comment expansions (`DiscardHandSlot`, `DiscardCost`), and **zero existing statements were
  changed, reordered or re-indented.** `PlayerTick` is **untouched**, so all four polled RMB
  consumers and the wheel branch are byte-identical.
- ⚠️ **`Tests/SiegePlacementTest.cpp` now has THREE fixture namespaces** (`SiegePlacementTestFixture`,
  `SiegePlacementUpgradeFixture`, `SiegeDiscardAllFixture`) and **23 tests**. ⛔ Do not add a second
  placement frame; ⛔ do not renumber mine. My helpers `CodeLinesOnly` / `CheckPrecedes` /
  `ExtractControllerFunctionBody` are reusable if you want ordering claims of your own.
- ⛔ **I added no member to reset in `Enter`/`ExitPlacementMode`.** `DiscardEntireHand` is stateless —
  it holds a function-local `TArray<int32>` and nothing else. TASK-813's reset-in-both pattern has
  **no counterpart here**, deliberately: there is no mode to leave.

**🔧 TASK-811 (the ship) —**
- ⭐ **The key is INERT until `IA_DiscardAll` exists**, by design (the `IA_Cmd*` precedent). TASK-820
  reports it landed (`IMC_Hero` 27 → 28). ⛔ **If your `IMC_Hero` readback shows the row absent, that
  is a REPORTABLE state, not a silent pass** — the code is correct and the key simply does nothing.
- ⛔ **Your pixel row (3b) for right-click is NOT APPLICABLE, and not because anything failed.**
  Report the right-click rows as **⛔ NOT BUILT — scrapped by Jonathan 2026-09-03**, ⛔ not as
  "not exercised" and ⛔ not as passed. ⭐ **The `H` row is the one that matters and it is required.**
- ⭐ **The `H` pixel row, as I would ask for it:** full hand → press `H` → **exactly 20 gold leaves**
  (⛔ not 26 — that number is the loop bug and it is what the eye must check) → **six new cards**.
  Then: **empty-ish hand → `H` → refusal line, gold unchanged**. Then: **`H` while a ghost is up →
  "Cannot discard while placing a card", gold unchanged**.

---

## 9. M8 DECLARATION (verbatim)

`DiscardEntireHand` runs **on the authority**, behind the shipped observer lockout — the same
`!HasAuthority()` → `BroadcastRefusal(GetObserverLockoutText())` branch `PlayHandSlot` and
`DiscardHandSlot` already carry. ⛔ **No new RPC. ⛔ No new replicated property. ⛔ No new replicated
class. ⛔ No new relevancy tier.** `DiscardAllCost` is an `EditDefaultsOnly` class default, not
replicated state; the gold movement is `ASiegePlayerState::SpendGold`'s existing owner-only path.
