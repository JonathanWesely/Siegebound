# TASK-1475 — [NAV-STOP-SUITE-COVERAGE-GAP] programmer handoff

**Marker:** `TASK-1475-NAV-STOP-SUITE-COVERAGE-GAP`
**Gate:** `TASK-1476` (qa-reviewer) — its lead question is *"CAN THIS TEST EVER FAIL?"*
**Status set:** `ready-for-qa`
**Law cited:** `SC-§39` · `SC-§50` · `SC-§95` · `SC-§97` · `SC-§101` · `SC-§104` · `SC-§138` · `SHIP-§9`

## Files written (exactly the three the `names:` line grants)

- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp` — **the only code file touched.** 202 → 872 lines.
- `.claude/pipeline/handoffs/TASK-1475-programmer.md` — this file.
- `.claude/pipeline/TASKBOARD.md` — **this row's `status:` line only.**

⛔ `SiegeMenuInputSubsystem.{cpp,h}` was **READ ONLY**. ⛔ No production code, no asset, no `CONVENTIONS.md`, no git, no compile, no PIE, no MCP call, no editor lifecycle action. The editor (PID 18236) and `TASK-1421`'s PIE session were not touched.

---

## §0 — 🚨 THE ANSWER TO THE GATE'S LEAD QUESTION, FIRST, BECAUSE IT IS THE ONE THAT MATTERS

### **NOT MEASURED.**

I did **not** witness the red. Those are the row's words and they are the honest verdict: this row's `names:` line grants `Read` / `Grep` / `Edit` + Slack and **explicitly forbids a compile**, so there is no path from here to a running binary, let alone to a pre-`TASK-1469` one. ⛔ **Nothing below should be read as a substitute for that, and I am not offering it as one.**

What I did instead, and what it is worth:

**(a) EVERY NEW ASSERTION IS PAIRED WITH A CONTROLLED NEGATIVE ON THE SAME WIDGETS.** Not "the test would fail if the code were wrong" — a *second arm* of the same fixture whose expected value is different, so the two arms cannot both be satisfied by a predicate that has stopped discriminating:

| Limb | Positive arm | Negative arm on the SAME tree | What a dead predicate does |
|---|---|---|---|
| 2 (ancestor visibility) | border `Visible` ⇒ `BackButton \| KeepSettingsButton \| RevertSettingsButton` | border `Collapsed` ⇒ `BackButton` | both arms read the 3-name string ⇒ **arm B red** |
| 3 (stepper collapse) | named pair ⇒ `Prev` only; glyph pair ⇒ `<` only | Settings footer (2 buttons, 1 parent) ⇒ **BOTH** | limb 3 removed ⇒ **stepper assertion red**; limb 3 widened to a structure-only rule ⇒ **footer assertion red** |
| fence (c) vocabulary | 4 classes admitted | `UTextBlock`, opted-out button, disabled button, unrealized button, `nullptr` refused | a class filter that stopped filtering ⇒ **red** |

**(b) NO NEW TEST CAN PASS VACUOUSLY.** Every fixture's headline assertion is a **positive** ordered-name string. If the synthetic tree failed to realize — the single largest technical risk in this diff, see §5 — `UWidget::IsVisible()` would be false for every widget, the collected list would be **empty**, and the headline assertion would go **red with an empty actual**. There is no silent-green failure mode. This was designed in, not noticed after.

**(c) THE EXACT THREE ONE-LINE MUTATIONS THAT MUST TURN IT RED.** ⛔ **None of these were executed.** They are handed over so the gate, or the next build, can buy the red cheaply:

| # | Mutation in `SiegeMenuInputSubsystem.cpp` | Expected red |
|---|---|---|
| M1 | delete the `if (!HasVisibleSlateAncestry(Widget)) return false;` block (`:791-793`) | `NavStop.CollapsedAncestorIsNotAStop` — "border COLLAPSED" reads the 3-name string |
| M2 | delete the `if (StepperNext != nullptr && StepperNext == Button) return false;` block (`:845-848`) | `NavStop.StepperRowIsOneStop` — headline string gains `ScreenResolutionNextButton` and `GlyphStepRightButton` |
| M3 | change `IsNavFocusStop`'s `UCheckBox` branch to `return false;` | `NavStop.FocusStopVocabulary` — headline string loses `VocabCheckBox` |

**(d) WHAT I WILL NOT CLAIM.** A paired control is *evidence the assertion discriminates against a fixture change*; it is **not** evidence it discriminates against a **production** change, because no production change was made and no binary was run. That distinction is the whole reason `MEASURED` and `UNOBSERVABLE` are different words in this pipeline, and I am claiming **neither** — I am writing `NOT MEASURED`.

---

## §1 — (1) THE SIX-SCREEN, TWO-COLUMN DIVERGENCE TABLE

**Column A — `GetMenuButtons()`** (`SiegeMenuInputSubsystem.cpp:389-407`). Walks `FindMainMenuWidget()` **unconditionally**; never consults `GetActiveNavTarget()`. Admits `Cast<UButton>` only. Tests only the widget's **own** `GetIsFocusable() / GetIsEnabled() / IsVisible()`.

**Column B — the `IsNavFocusStop()` walker**, collected by `GetMenuFocusStops()` (`:875-905`). Walks `GetActiveNavTarget()`. Four admitted classes (`UButton`, `UCheckBox`, `USlider`, `UEditableTextBox`), **plus** limb 2 (no Collapsed/Hidden Slate ancestor), **plus** limb 3 (a recognised stepper row yields its `Prev` member only).

| # | Screen | **A: `GetMenuButtons()`** | **B: `IsNavFocusStop()` walker** | Agree? |
|---|---|---|---|---|
| 1 | **Main menu** `WBP_MainMenu` | **7** — the menu's own buttons | **7** — ⭐ the *same seven widgets in the same order* | ✅ **THE ONLY ONE** |
| 2 | **Settings** `USettingsMenuWidget` | **7 / 0** — the *main menu's* buttons | **3** (2 unhappy) — `ConfirmToggleCheckBox` · `GraphicsButton` · `BackButton` | ⛔ no |
| 3 | **Graphics** `USiegeGraphicsMenuWidget` | **7 / 0** — the *main menu's* buttons | **19** (was 24: −2 limb 2, −3 limb 3); ⚠️ **21** while a video-mode confirm is pending | ⛔ no |
| 4 | **Login** `UAccountMenuWidget` | **7 / 0** — the *main menu's* buttons | **3 / 5 / 4 / 5 / 2–3** by mode (Chooser / CreateForm / LoginForm / CloudLinkForm / LoggedIn) | ⛔ no |
| 5 | **Deck builder** `WBP_DeckBuilder` | **7 / 0** — the *main menu's* buttons | **3** details closed / **4** details open (limb 2 on `Btn_DetailsClose`) | ⛔ no |
| 6 | **Session** `WBP_SessionMenu` | **7 / 0** — the *main menu's* buttons | **4** — `AddressTextBox` at index 3 | ⛔ no |

### 🚨 The "7 / 0" is the headline, not a footnote

On **every screen but the main menu**, column A returns the **main menu's** seven buttons whenever `WBP_MainMenu` is still a top-level widget, and **zero** when it is not. ⛔ **Neither branch depends on the screen the player is looking at.** The divergence is therefore not "the counts differ" — it is that **column A is not a measurement of five of these six screens at all.** A suite reading through it was, on those five screens, asserting a property of a widget nobody was looking at.

### The two mechanisms behind the numbers, stated separately

- **BY CLASS (predates limbs 2 and 3).** The control a player reaches *first* on Settings is a `UCheckBox`; on Session it is a `UEditableTextBox`; on Graphics **eleven** of the stops are `USlider`s. Column A cannot see any of them. ⇒ Even before `TASK-1469`, a green `GetMenuButtons()` suite said nothing about those screens.
- **BY LIMB (`TASK-1469`).** −2 ancestor-visibility on `KeepSettingsButton`/`RevertSettingsButton`; −3 stepper-`Next` on `ScreenResolutionNextButton`/`WindowModeNextButton`/`FrameRateLimitNextButton`. Column A has neither rule.

### Provenance, verb-labelled (`SC-§97`)

- **Column B, all six rows — CITED, not recomputed** (the row says cite, and I cited): `handoffs/TASK-1469-programmer.md` §2, gated by `qa/TASK-1470.md` with all six rows independently confirmed.
- **Main-menu `7`, column B — MEASURED at runtime**: `qa/TASK-1413-verify.md` §5, walked by name in both directions, `of 7` on every line, ring closed at `6 -> 0`.
- **Graphics `21`-with-a-pending-confirm — CITED, unverified**: `handoffs/TASK-1469-programmer.md` §2 note. ⛔ **No Graphics count is asserted anywhere in the test** — see §3.
- **Column A's "7 / 0" — DERIVED here from the call graph** (`GetMenuButtons` → `FindMainMenuWidget` → `GetAllWidgetsOfClass(TopLevelOnly)`, no visibility test, no nav-target test). **Not measured.**

---

## §2 — (3) THE THREE ASSERTIONS THE ROW NAMES, AND WHERE EACH ONE LIVES

All three assert **STATE** — an ordered `A | B | C` string of widget names or labels compared to an explicit literal — never a count against a count (`SC-§104`). A count that matches for the wrong reason is exactly what this row exists to stop, and a red that prints only `3 != 1` cannot be diagnosed.

### (3a) A control inside a COLLAPSED ancestor is NOT collected — limb 2
`Siegebound.MenuInput.NavStop.CollapsedAncestorIsNotAStop`. Fixture shaped and **named** after the real Graphics confirm bar (`VideoModeConfirmBorder` → `KeepSettingsButton` / `RevertSettingsButton`, with a `BackButton` outside it as the control).
- border `Visible` ⇒ `BackButton | KeepSettingsButton | RevertSettingsButton`
- border `Collapsed` ⇒ `BackButton`
- **the premise asserted, not assumed:** `KeepSettingsButton->IsVisible()` is **still true** under the collapsed ancestor. If that ever reads false, limb 2 is unreachable and arm B would be passing for an unrelated reason.
- **the divergence asserted:** the `GetMenuButtons()`-shaped predicate still admits all three.

### (3b) A stepper row yields ONE stop, not two — limb 3
`Siegebound.MenuInput.NavStop.StepperRowIsOneStop`. Three two-button rows in one tree:
- `ScreenResolutionPrevButton` / `ScreenResolutionNextButton` → the **name** discriminator
- `GlyphStepLeftButton` (`<`) / `GlyphStepRightButton` (`>`) → the **glyph** fallback discriminator
- `GraphicsButton` / `BackButton` → 🚨 **the controlled negative**: `USettingsMenuWidget`'s own footer shape, which must keep **both**
- headline: `ScreenResolutionPrevButton | GlyphStepLeftButton | GraphicsButton | BackButton`
- **the divergence asserted:** the `GetMenuButtons()`-shaped predicate admits all **six**.

### (3c) 🚨 THE REGRESSION THAT MATTERS MOST — the main menu is STILL 7
In the existing PIE test's step 1, on the live `WBP_MainMenu`, through the **real** subsystem calls. ⛔ **This replaces a `Buttons.Num() >= 3` tally** — which was true of a menu that had lost four entries, and was the only thing between this suite and a silently gutted ring. Five assertions:
1. labels in tree order == `Play (vs Bot) | Sandbox (No Bot) | Deck Builder | Multiplayer | Settings | Login | Quit`
2. `GetMenuButtons()` == 7
3. `GetMenuFocusStops()` == 7
4. the two lists name the same widgets in the same order
5. **element-by-element pointer identity** between them (a name match is not an identity match) — plus `IsNavFocusStop()` true for each of the seven, printed by name.

**Why (3c) is sound rather than optimistic, since it is the one literal a wrong reading would make red on a correct build:** `TASK-1413`'s seven stops were measured on the **pre-1469** walker (own-visibility + four classes) and all seven are `UButton`s ⇒ `GetMenuButtons()`, which is that rule minus the three extra classes, returns exactly them. `WBP_MainMenu`'s design-time `Btn_Jump` (`qa/TASK-1470.md` WARN-5) was in **neither** set, so it fails a clause both rules share. `TASK-1469`'s limbs can only **remove**, and its §3 shows neither can remove any of the seven.

### Beyond the three (declared as extra, not smuggled in)
`Siegebound.MenuInput.NavStop.FocusStopVocabulary` — fence (c): the four admitted classes are stops; `UTextBlock` is not; an authored `IsFocusable == false` is honoured; a disabled button is not a stop; an **unrealized** button is not a stop (`TASK-1469` (2a)); `IsNavFocusStop(nullptr)` is false. This is the **by-class** half of the divergence and the table above shows it is the half that reaches the most screens, so I judged it inside the row's intent rather than scope creep. **If the gate disagrees, deleting this one test costs nothing and breaks nothing.**

---

## §3 — WHY NO GRAPHICS COUNT IS BAKED ANYWHERE

The brief's warning is correct and I obeyed it: *a gate written in indices has an expiry date nobody printed on it.* Graphics is **19** stops with `BackButton` at 18, and **21** with `BackButton` at 20 the instant a stepper press un-collapses the confirm bar. ⇒ **The MECHANISM is stable; the COUNT is not.** So:

- the limb-2 and limb-3 mechanisms are asserted on fixtures **shaped and named after** the real Graphics controls, where the collapse state is a property *the test sets*, and
- the only **real screen** asserted end-to-end is the main menu, whose seven stops are MEASURED at runtime with the wrap.

⛔ `19`, `21`, `24` and `18` appear in this file **only in comments, as cited provenance.** No assertion depends on any of them.

---

## §4 — (4) THE NO-REFACTOR STATEMENT

⛔ **`GetMenuButtons()` was NOT touched, NOT deprecated, NOT renamed and NOT unified with `IsNavFocusStop()`.** Three reasons, in the order they bind:

1. **The divergence is the subject of the measurement.** Erasing it destroys what this row was boarded to measure.
2. **It is a live predicate**, not dead weight: `TASK-1400`'s `ApplyInitialFocus` guard reads through it, the existing PIE test reads through it, and `TASK-1469`'s §3 non-regression argument is stated in its terms.
3. `SC-§50` — a refactor riding a coverage fix makes both unreviewable.

**Do I think it should eventually go?** ⭐ **My opinion, offered as opinion (`SC-§101`): no — not as a deletion, but it should stop being reachable as "the menu's controls".** Its real remaining job is narrow and legitimate: *the `UButton`s of the main menu specifically*. Its **name** is what misleads, because it reads as a general accessor while being hard-wired to one screen. If a future row wants this tidied, the cheap and safe move is a **rename** (e.g. `GetMainMenuButtons()`) with no behaviour change — ⛔ **and that is its own row, with its own caller census, which this row did not perform and must not be read as having performed.**

---

## §5 — WHAT QA SHOULD SCRUTINISE (ranked by how much it would cost to be wrong)

1. 🚨 **THE LARGEST RISK IN THE DIFF: will a synthetic `UWidgetTree` realize outside a `UUserWidget` and a viewport?** The fixtures call `RootWidget->TakeWidget()` on a tree built with `UWidgetTree::ConstructWidget` under `GetTransientPackage()`. I traced `UVerticalBox::RebuildWidget` → `UVerticalBoxSlot::BuildSlot` → `Content->TakeWidget()`, and `UBorder::RebuildWidget` → `UBorderSlot::BuildSlot` → `SBorder::SetContent`, so descendants realize and get Slate parents. ⛔ **I could not run it.** Mitigation already in the code: every headline assertion is a **positive** list, so a realization failure is a **loud red with an empty actual**, never a silent green.
2. ⚠️ **`UWidget::MyWidget` is a WEAK pointer.** Discarding the `TSharedRef<SWidget>` from `TakeWidget()` would destroy the Slate tree on the spot and make every `IsVisible()` false. `FSiegeNavStopFixture::SlateRoot` holds it for the life of the assertions. **If a reviewer sees that member as redundant, it is not — it is load-bearing.**
3. ⚠️ **The `PRAGMA_DISABLE_DEPRECATION_WARNINGS` block in `BuildVocabularyFixture`.** It is required, not stylistic: `UButton` has **no** `SetIsFocusable`, and `InitIsFocusable()` is **`protected`** on both `UButton` (`Button.h:206`, inside the `protected:` block opened at `:187`) and `UCheckBox` (`CheckBox.h:155`) — measured in the 5.8 headers, and it is the mistake this diff made first and corrected. The remaining route is the deprecated public field, which is **exactly** what this project's own shipped opt-out does at `SiegeControlsHelpWidget.cpp:176-178` — same field, same pragmas.
4. ⚠️ **The `GetMenuButtons()` replica (`CollectLegacyMenuButtonStyleAdmissions`) is a DOCUMENTED CONSTANT, not a read of production code.** It transcribes `:403`. It therefore **cannot** notice a future edit to `GetMenuButtons()` itself. Its job is only to show the two notions give different answers on the same tree. Declared here so nobody credits it with more.
5. ⚠️ **`(3c)`'s seven literal labels are the most brittle thing in the diff by design.** They are MEASURED (`qa/TASK-1413-verify.md` §5) and they are the point: a walker change that shortens that ring must go red.
6. **C4458 sweep, done against the BASE CHAIN and not against the project** (the false-negative-by-construction mistake that cost `TASK-1423` a build). New locals inside member `RunTest` bodies: `Fixture`, `ConfirmBorder`, `KeepButton`, `Stops`, `LegacyAdmissions`, `bElementwiseIdentical`, `StopIndex`, `UnrealizedButton`, `VocabCheckBox`, `ButtonsAsWidgets`. `FAutomationTestBase`'s data members, enumerated from `Misc/AutomationTest.h`: `bSuppressLogWarnings`, `bSuppressLogErrors`, `bElevateLogWarningsToErrors`, `SuppressedLogCategories`, `bComplexTask`, `bRunOnSeparateThread`, `bSuppressLogs`, `TestName`, `TestParameterContext`, `ExecutionInfo`, `ExpectedMessages`, `ActionCS`. **No collision.** All fixture construction lives in **namespace-scope free functions**, where base-class shadowing is impossible by construction.
7. **Unity-build name uniqueness, greped across `Source/`:** `FSiegeMenuInputNavStopCollapsedAncestorTest`, `FSiegeMenuInputNavStopStepperRowTest`, `FSiegeMenuInputNavStopVocabularyTest`, `FSiegeNavStopFixture`, the three new `Siegebound.MenuInput.NavStop.*` test-name strings, and every new helper name — **0 occurrences elsewhere.** `namespace SiegeMenuInputTestUtils` exists in this file only.
8. **New includes** (all UMG/CoreUObject/SlateCore, all confirmed to exist at those paths in 5.8): `Blueprint/WidgetTree.h`, `Components/Border.h`, `Components/CheckBox.h`, `Components/EditableTextBox.h`, `Components/HorizontalBox.h`, `Components/PanelWidget.h`, `Components/Slider.h`, `Components/TextBlock.h`, `Components/VerticalBox.h`, `UObject/StrongObjectPtr.h`, `Widgets/SWidget.h`. The module already links `UMG`, `Slate` and `SlateCore` (`qa/TASK-1470.md` "Notes for build-master"), so **no `Build.cs` change is needed.**
9. **No UHT-visible delta**: no new `UCLASS`/`USTRUCT`/`UPROPERTY`/`UFUNCTION`, no new header. Ordinary recompile; `WITH_DEV_AUTOMATION_TESTS` only.
10. **The three new tests need no map and no PIE** — they are synchronous and world-free, and they guard the game thread explicitly (`IsInGameThread()`) because they build Slate widgets. Only the pre-existing test drives `L_MainMenu`.

---

## §6 — 🚨 A NOTE FOR `TASK-1474` (deck-bar descent, queue position 4), WHOSE GRANT LANDS ON THIS FILE

Your grant is **expected VALUES only** — no new test, no deleted test, no changed assertion shape — with a duty to restate every moved value one by one. This file was written to make that easy and to make the alternative impossible:

- Every expected value is an **explicit literal** in the assertion, with the **screen** and the **limb** named in the assertion's own message text. None is derived from the code under test, and none is computed.
- The values you may need to move, and nowhere else:
  - `NavStop.CollapsedAncestorIsNotAStop` — two ordered-name strings (border VISIBLE, border COLLAPSED) + one `GetMenuButtons()`-replica string.
  - `NavStop.StepperRowIsOneStop` — one ordered-name string + four per-widget `TestTrue`/`TestFalse` + two footer `TestTrue` + one replica string.
  - `NavStop.FocusStopVocabulary` — one ordered-name string + four per-widget refusals + one replica string.
  - the PIE test's step 1 — the seven-label string, the two `7`s, the same-order string, the pointer-identity loop.
- ⛔ **If your change makes a whole fixture the wrong shape rather than the wrong value, that is a `names:`-line question for the manager, not a value edit.** Say so rather than reshaping a fixture under a values-only grant.

---

## Not examined / limitations

- ⛔ **`NOT MEASURED`** — the red was not witnessed. No compile, no run, no pre-`TASK-1469` binary. §0(c) hands over the three one-line mutations that should produce it; **none was executed.**
- ⛔ **Nothing in this diff has been compiled.** Every API was verified by reading the UE 5.8 headers and sources on disk (access specifiers, defaults, signatures and include paths are cited inline where they were load-bearing), but reading is not building.
- ⛔ **No runtime acceptance criterion and therefore no 5b** — a unit test has none. Per the row's routing that is **not** an `UNOBSERVABLE`; it is the absence of a criterion.
- ⚠️ **The synthetic fixtures reproduce the real screens' SHAPES and NAMES, not the real screens.** They are not evidence about `USiegeGraphicsMenuWidget`'s live tree; they are evidence about the predicate that walks it. The only live screen this file asserts is the main menu.
- ⚠️ **`CollectNavStops` duplicates `GetMenuFocusStops()`'s traversal** (same `UWidgetTree::ForEachWidget`, same predicate) because the real function is bound to `GetActiveNavTarget()`. The **predicate** — the thing `TASK-1469` changed — is the production one; the **collection loop** is the test's. A change to `GetMenuFocusStops`'s traversal alone would not be caught by the three synthetic tests, though it would be caught by the main-menu pointer-identity assertion.
- ⚠️ **Column A's "7 / 0" per-screen reading is DERIVED, not measured.** Whether `WBP_MainMenu` is still a top-level widget behind each sub-screen was not verified per screen; the claim that does not depend on it — *column A never consults the active nav target* — is read directly off `:389-407`.
- ⚠️ **Login's 2–3 range and the Settings unhappy path (2) are carried as cited ranges and are not asserted.**
- ⚠️ **No caller census of `GetMenuButtons()` was performed.** §4's rename opinion is explicitly *not* backed by one and must not be actioned on this row's authority.
- ⚠️ **`qa/TASK-1470.md` WARN-2's widened limitation** (a stepper row loses *all* stops if `Prev` is excluded by **any** clause, not just collapse) is **not** covered by a test here. It is not live today — `USiegeGraphicsMenuWidget::SetControlsEnabled` moves both members together — but it is the likeliest future trip, and a fixture for it would be four lines. Recording it as an unclaimed gap rather than quietly widening my own scope.
