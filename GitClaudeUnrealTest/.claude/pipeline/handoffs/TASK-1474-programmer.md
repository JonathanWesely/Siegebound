# TASK-1474 — [DECKBAR-DESCENT] programmer handoff

**Row:** `TASK-1474` · marker `TASK-1474-DECKBAR-DESCENT` · gate `TASK-1477`
**Status flipped to:** `ready-for-qa`
**Date:** 2026-09-25
**Files written (four, and no others):**

- `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeMenuInputTest.cpp` — **comment only** (see §7)
- `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` — **the one stale sentence only** (see §8)

⛔ No compile, no PIE, no MCP, no `.uasset`, no git, no `CONVENTIONS.md`, no other row's line.
⛔ `SiegeControlsHelpWidget.{cpp,h}` was **read** and is **cited by text** throughout; not touched.

---

## §0 — 🚨 READ THIS FIRST: THE ROW'S HEADLINE CLAIM IS HALF FALSE, AND ROW B IS WHAT FALSIFIED IT

The row says the descent makes "the deck bar's ten `SlotButton`s **agent-reachable**". Measured:

> ⛔ **`UDeckBuilderWidget` declares itself SELF-DRIVING** — `DeckBuilderWidget.cpp:2155` calls
> `RegisterSelfDrivingMenuNavTarget(this)`, shipped by `TASK-1471` (Row B), which is **this row's own
> unblocking gate**. `MoveFocus` returns at `DeclineIfActiveTargetSelfDriving` **before it ever calls
> `GetMenuFocusStops`** on that screen.

⇒ **What this row actually delivers on the deck builder is the COUNT, not the RING.** The ten
`SlotButton`s are now *collected* — so `LogNavTargetRetarget`'s `%d focus stop(s)` finally describes
the whole screen — and they are **not** walked by Up/Down, because nothing is walked there any more.

**This is not a reason to close the row, and I have not treated it as one** (`SC-§100`: a re-scope is a
board edit). It is recorded because:

1. it is the second time this wave a conclusion outlived the premise that produced it;
2. the deck bar is **already keyboard-reachable by the screen's own driver** —
   `UDeckBuilderWidget`'s grid-exit lands focus on an entry's own `SlotButton` via `FindFirstButton`
   (`DeckBuilderWidget.cpp:1017-1059`, used at `:1406-1432`). Nothing was unreachable; the *instrument*
   was blind;
3. ⚖️ **for the manager:** if 🧑 his sentence "all 10 screens fully navigatible by an agent" means the
   generic ring must walk the deck bar, this row does **not** get there and no walker change can — the
   blocker is the self-driving declaration, which Row B took deliberately to stop the ring transiting
   "Reset to Default" (a deck-wiping, auto-saving button). That is a **policy** question, not a
   traversal one, and it is **not mine to decide**.

---

## §1 — 🚨 THE NINE-TREE CENSUS (`TASK-1477` (7)) — ANSWERED EXPLICITLY, AND THE STOPPER IS NAMED

**The question:** `Btn_Jump` sits at `Overlay_19/SizeBox_0/Btn_Jump` in nine design-time trees, and
`WBP_DeckCardTile` is one of them ⇒ **34 + 1 `Btn_Jump`s are in the deck builder's rendered tree at
once**. Does this diff admit 34 `Btn_Jump`s into the ring?

### ⛔ ANSWER: **NO — ZERO OF THEM.** And the stopper is an EXPLICIT CLASS-SHAPED EXCLUSION I WROTE, not a hoped-for asset state.

`USiegeMenuInputSubsystem::IsCodeAuthoredSubWidget` — the walker descends into a nested `UUserWidget`'s
own `WidgetTree` **if and only if that widget's class carries `CLASS_Native`**, i.e. was declared with
`UCLASS()` in `Source/`. A `UWidgetBlueprintGeneratedClass` never carries it.

- `WBP_DeckCardTile_C` → Blueprint → **not entered** → its `Btn_Jump` **and** its `Btn_CardFace` **and**
  its two nested `UI_Thumbstick_C` are all unreachable. ×34 tiles ⇒ **0 admitted.**
- `UI_Thumbstick_C` (2 in `WBP_MainMenu`, 2 in `WBP_DeckBuilder`, 2 in every other template-derived WBP)
  → Blueprint → **not entered.**
- `UDeckSlotEntryWidget` → native → **entered** → its `SlotButton` admitted. ×10.

### ⛔ WHY I REFUSED THE STOPPERS THE GATE OFFERED AS CANDIDATES

| Candidate stopper | Verdict | Why |
|---|---|---|
| **Visibility** (`IsVisible()`) | ⛔ **REFUSED** | It works for `WBP_DeckBuilder`'s and `WBP_MainMenu`'s *own* `Btn_Jump` (collapsed in their `Construct` graphs, `qa/TASK-1424.md` WARN-2) — but that is a fact about **content**, invisible in a C++ diff, and **nobody has measured whether `WBP_DeckCardTile`'s `Construct` collapses its copy.** I could not measure it under this row's fences, and a stopper I cannot read is not a stopper. |
| **Slate ancestry** (`HasVisibleSlateAncestry`) | ⛔ **REFUSED** | Same objection, plus it is the exact assumption `qa/TASK-1433.md` just failed a row over: it does **not** model `SWidgetSwitcher`. A tile is visible, so its interior has visible ancestry. |
| **A name / path exclusion** (`Btn_Jump`, `Overlay_19/SizeBox_0/...`) | ⛔ **REFUSED** | A gate written in names or indices has an expiry date nobody printed on it. `Btn_Jump` is also not the only debris — `Btn_CardFace` and two thumbsticks ride the same tile. |
| **"Is the sub-widget itself focusable?"** | ⛔ **REFUSED — AND IT WOULD HAVE BEEN A DISASTER** | It looks right (tiles are focusable, entries are not) and it is **false at the only moment that matters**: `UDeckBuilderWidget::FocusCardTile` sets `SetIsFocusable(true)` **lazily, on the tile being focused** (`DeckBuilderWidget.cpp:1135-1138`). Before the player ever touches the grid, all 34 tiles read `false` ⇒ all 34 would be descended. I tried this rule and discarded it on that measurement. |
| **Class authorship (`CLASS_Native`)** | ✅ **TAKEN** | Decidable from the object alone; no asset read; no name list; and it is the **only** line that separates the deck bar from the card grid *by construction* rather than by coincidence. |

### ⛔ THE ARGUMENT FOR AUTHORSHIP, AS A PROPERTY RATHER THAN A PREFERENCE

A C++-authored subtree's stop set is a function in `Source/`: it moves only in a diff QA reads, in the
same review as the walker. A Blueprint-authored subtree's stop set moves when someone drags a button in
UMG, with no C++ diff at all — and this project has an artist working the same screens in parallel.
⇒ **the ring can only ever grow through code review.**

Engine backing, read at source: `ObjectMacros.h:223` documents `CLASS_Native` as "Class is a native
class"; `CLASS_RecompilerClear` (`ObjectMacros.h:296`) **clears** it on every recompiled Blueprint class;
`BlueprintGeneratedClass.cpp:132` finds a BP's C++ base by walking up until the flag appears.

### ⛔ AND THE OTHER TWO TREES THE GATE NAMED

- **`WBP_HUD`** — carries a `Btn_Jump` (`IsFocusable = False`, measured and recorded in this file's own
  header at the "(6) THE HUD AND THE FPS COUNTER" section). **Triple-fenced:** it never registers, so no
  walk ever enters its tree at all; its `Btn_Jump` is non-focusable; and it is a Blueprint, so even if it
  were nested under a registered screen the boundary would refuse it.
- **`WBP_CardHand`** — nested inside `WBP_HUD` (measured in the `.uasset` import table). Same three
  fences, plus its `Btn_Jump` is `Collapsed`.
- `WBP_VictoryScreen`, `UI_TouchSimple`, `UI_TouchInterface_*` — none registers; none is nested under a
  registered screen.

---

## §2 — 🚨 THE EXCLUSION EVERY DOCUMENT IN THIS CHAIN CITED WAS THE WRONG ONE (and the "before" column was re-measured, not inherited)

`qa/TASK-1433.md`'s correction is **right**, and I re-derived it independently at engine source:

- `class UUserWidget : public UWidget, public INamedSlotInterface` (`UserWidget.h:280`)
  ⇒ `ForWidgetAndChildren`'s **named-slot limb IS entered** on every nested user widget
  (`WidgetTree.cpp:249-263`). *"A `UUserWidget` is not a `UPanelWidget`"* explains nothing.

**So why were the old numbers right? I measured it rather than inheriting either account:**

- `UUserWidget::GetSlotNames` returns the generated class's `InstanceNamedSlots`
  (`UserWidget.cpp:1264-1282`), and `GetContentForSlot` returns `NamedSlotBindings[].Content` — i.e.
  content the **outer** tree injected into the sub-widget, **never** the sub-widget's own interior.
- **This project authors ZERO `UNamedSlot`s.** Measured two ways:
  - all **17/17** widget `.uasset`s in `Content/` contain the token `NamedSlot` exactly twice, and in
    every case the match is the class property `AvailableNamedSlots` — there is **no** bare `NamedSlot`,
    no `InstanceNamedSlots` and no `NamedSlotBindings` entry in any of them;
  - `Source/` constructs none (the only textual hit is a comment in `SiegeControlsHelpWidget.cpp`).

⇒ ✅ **The "before" column stands, for a reason nobody had written down.** The received mechanism was
wrong; the numbers it "explained" were right by a different route. Both corrections are now written into
`SiegeMenuInputSubsystem.cpp` at the walker and into `SiegeMenuInputSubsystem.h` at
`RegisterSelfDrivingMenuNavTarget`, `SC-§120` strike-never-delete.

---

## §3 — 🚨 THE DELTA'D COUNT TABLE, PER SCREEN, WITH THE ADMITTING MECHANISM NAMED

**Baseline = `TASK-1469`'s AFTER column** (`handoffs/TASK-1469-programmer.md` §2), **not** pre-Row-A
numbers. Controls help's 1 comes from `TASK-1432`.

| Screen | Nested `UUserWidget`s in its tree (measured) | Before (TASK-1469 after) | **After (this row)** | Δ | Admitting / refusing mechanism |
|---|---|---|---|---|---|
| **Main menu** (`WBP_MainMenu`) | 2 × `UI_Thumbstick_C` | **7** | **7** | **0** | ⛔ REFUSED — Blueprint class, boundary declines. Its own `Btn_Jump` is a **direct child**, already walked before this row and already excluded. **The runtime-VERIFIED 7 does not move, by construction.** |
| **Settings** (`USettingsMenuWidget`) | **none** | **3** (2 unhappy) | **3** (2 unhappy) | **0** | Nothing to descend into — no `CreateWidget`/`ConstructWidget` of any `UUserWidget` subclass in that file. |
| **Graphics** (`USiegeGraphicsMenuWidget`) | **none** | **19** (21 confirm-pending) | **19** (21 confirm-pending) | **0** | `USiegeFrameRateCounterWidget` is a **viewport sibling** (`Counter->AddToViewport(ZOrder)`), not a child of this tree. |
| **Login** (`UAccountMenuWidget`) — Chooser / CreateForm / LoginForm / CloudLinkForm / LoggedIn | **none** | **3 / 5 / 4 / 5 / 2–3** | **3 / 5 / 4 / 5 / 2–3** | **0** | Nothing to descend into. |
| **Deck builder** (`WBP_DeckBuilder`) | 10 × `UDeckSlotEntryWidget` (**native**) · 34 × `WBP_DeckCardTile_C` · 2 × `UI_Thumbstick_C` | **3** details closed / **4** details open | 🚨 **13** closed / **14** open | **+10** | ✅ **ADMITTED:** the entry's `OutlineBorder (UBorder) > SlotButton (UButton) > SlotLabelText` tree is entered because `UDeckSlotEntryWidget` is native; `SlotButton` is the only admitted-class widget in it ⇒ exactly +1 per entry, ×10. ⛔ **REFUSED:** all 36 Blueprint sub-widgets. |
| **Session** (`WBP_SessionMenu`) | **none** (measured — its `.uasset` references no other widget class at all) | **4** | **4** | **0** | Nothing to descend into. |
| **Controls help** (`USiegeControlsHelpWidget`) | N × `USiegeControlsHelpRowWidget` (**native**) · 1 × `USiegeControlsDetailWidget` (**native**) | **1** | **1** | **0** | ⭐ **ENTERED AND STILL ZERO — this is the controlled negative, see §4.** |

**Order property:** the descent is appended **at the point the parent walk reaches the sub-widget**, so
the result is a strict **superset** of the old sequence with **nothing permuted**. On the deck builder the
ten `SlotButton`s appear consecutively, in `DeckBar` slot order, where `DeckBar` already sat.

**Anchoring** (`MoveFocus` prints a bare `GetName()`): the ten new stops are `SlotButton` × 10 — a name
that exists in exactly one class (`UDeckSlotEntryWidget`, code-authored, `DeckSlotEntryWidget.cpp:120`)
and is therefore **unambiguous by class**, but **NOT unique by instance**: ten siblings share it.
⇒ ⛔ **`UNANCHORED` by name at the instance level.** A verifier must distinguish them by **ring index**
or by the entry's `SlotLabelText` (`deck1`..`deck10`), never by the logged name. Every other row of the
table moves by zero, so no other anchor is at risk.

---

## §4 — ⛔ THE CONTROL, AND IT DISCRIMINATED (not an assertion)

The gate asked me to **prove** the stopper with a control. Three arms, two static and one runtime:

**Arm 1 — the positive (descent happened).** Deck builder 3/4 → 13/14. If the boundary had refused
native classes too, this arm reads 3/4 and the row has done nothing.

**Arm 2 — ⭐ THE CONTROLLED NEGATIVE, AND IT IS THE ONE THAT MATTERS: controls help.**
Its rows **ARE** native, so the walker **DOES** now enter them — and the count **still does not move.**
Measured reason: every `UButton` in both branches is authored non-focusable —
`USiegeControlsHelpRowWidget::RowButton` and `USiegeControlsDetailWidget::BackButton` both pass through
that file's own `ApplyButtonNotFocusable` helper (which sets `Button->IsFocusable = false`), and that
file's `CloseButton` comment records **in its own voice** that the other four de-focus sites were
deliberately **not** flipped, `RowButton` and `BackButton` among them, *"same structural refusal, same
measurement"*. `IsNavFocusStop`'s `GetIsFocusable()` clause refuses them.
⇒ **This arm separates "did the descent reach it" from "was it admitted".** A rule that merely said
"descent adds stops" would have predicted +N here and been wrong.

**Arm 3 — the refused side.** Main menu 7 → 7 with two native-looking-but-Blueprint thumbsticks sitting
right there. If the boundary leaked, this arm moves — and it is a **runtime-`VERIFIED`** number, so it
is the loudest possible falsifier.

**⛔ THE RUNTIME CONTROL IS ALREADY INSTRUMENTED AND NEEDS NO NEW CODE.**
`LogNavTargetRetarget` prints `%d focus stop(s)` at `UE_LOG(LogSiegeMenuInput, Log, …)`, and
`LogSiegeMenuInput` is `DECLARE_LOG_CATEGORY_EXTERN(…, Log, All)` ⇒ **it prints in Development with no
console verb**, so `TASK-1436` can read it without a `Verbose` incantation and an **absent** line is a
zero and a defect signature, not an `UNOBSERVABLE`. On the deck builder that one number discriminates
all three outcomes:

- prints **3 / 4** ⇒ the descent did not run — **defect**
- prints **13 / 14** ⇒ descent ran and the boundary held — **pass**
- prints **47+ / 48+** ⇒ the boundary leaked and the tiles were entered — **defect, and the exact one
  the gate named**

I deliberately added **no new log line**: `MoveFocus`'s format string is quoted verbatim by
`TASK-1421`/`TASK-1427` acceptance criteria and must not move.

---

## §5 — ⛔ (3) THE CARD GRID IS STILL UNFIXED BY THIS ROW. SAYING SO PLAINLY.

The 34 `WBP_DeckCardTile` tiles are **not** stops after this change and **cannot become** stops through
descent. Two independent reasons, both of which hold:

1. a tile is a `UUserWidget`, which is **not one of `IsNavFocusStop`'s four admitted classes**
   (`UButton` / `UCheckBox` / `USlider` / `UEditableTextBox`) — `qa/TASK-1424.md`, confirmed;
2. `WBP_DeckCardTile_C` is a Blueprint class, so this row's boundary **refuses to enter it at all**.

⇒ `TASK-1423` (4b)(ii)'s *"adds descent and declares victory"* failure is **not** committed here. The
grid is navigated by the deck builder's own 2-D driver, exactly as before, and this row changes nothing
about it.

---

## §6 — ⚠️ THE HAZARD THIS ROW MULTIPLIES, CENSUSED AND NOT FIXED (coordinator's duty (2))

`SiegeMenuInputSubsystem.cpp`'s own `HasVisibleSlateAncestry` banner has always declared that it does
**not** model `SWidgetSwitcher` — a stop in an **inactive** switcher slot is still admitted and still
refuses focus, and such a stop **swallows the ring**.

**Census, per screen, of anything this row newly admits into an admitted-but-unfocusable position:**

- **Deck builder (+10):** `DeckBar` is a `UHorizontalBox`; no switcher, no inactive branch, all ten
  entries always arranged. ⛔ **No exposure.** (And the ring does not walk this screen at all — §0.)
- **Controls help (+0):** ⚠️ **the switcher is here, and this row puts candidates inside it.**
  `ViewSwitcher` holds the row list in one slot and `DetailView` in the other. The descent now enters
  **both** branches. ✅ **Not live today** because every button in both is `IsFocusable = false`
  (§4 arm 2) ⇒ refused before the switcher question is asked.
  🚨 **TRIP-WIRE, and it is one row away:** the moment anyone flips `RowButton` or `BackButton`
  focusable — which is exactly what the controls-help screen needs to become navigable — the **inactive**
  branch's buttons become admitted-but-unfocusable stops and **swallow the ring**. I have written that
  warning into `HasVisibleSlateAncestry`'s banner, where the next editor will hit it, together with the
  strike of its now-expiring *"that overlay never registers"* clause (`TASK-1432` registers it).
  ⛔ **Modelling the switcher is not this row's scope and was not attempted.**
- **All other screens (+0):** nothing newly admitted ⇒ no new exposure.

**⭐ AND THE CORRECTION TO THE CLAIM `TASK-1432` MADE ABOUT THIS ROW.** That row called this descent
*"the last thing standing between this screen and 🧑 his standing instruction."* ⛔ **It is necessary and
it is NOT sufficient.** Descent is required (without it, flipping `RowButton` is inert — that file says
so itself); flipping `RowButton` focusable is *also* required and is **in `SiegeControlsHelpWidget.cpp`,
which I may not touch**; and doing so opens the switcher hazard above. **Controls help is 1 → 1 on this
row.** Nobody should read this handoff's table as having navigated that screen.

---

## §7 — 🚨 THE SUITE FILE: EVERY MOVED COUNT, RESTATED ONE BY ONE

### ⛔ COUNTS MOVED: **ZERO.** Here is the one-by-one, so the zero is a measurement and not a silence.

| Assertion in `SiegeMenuInputTest.cpp` | Expected value | Moved? |
|---|---|---|
| `MAIN MENU / GetMenuButtons() — exactly 7 buttons` | **7** | ⛔ **NO.** `GetMenuButtons()` is not touched by this row at all. |
| `MAIN MENU / IsNavFocusStop() walker — exactly 7 focus stops` | **7** | ⛔ **NO.** `WBP_MainMenu`'s two nested user widgets are `UI_Thumbstick_C` — Blueprint ⇒ the boundary refuses them. **7 → 7.** |
| `MAIN MENU / GetMenuButtons() — the seven button labels, in tree order` | the 7 labels | ⛔ **NO.** Nothing added, nothing permuted. |
| `MAIN MENU — element-by-element POINTER identity between GetMenuButtons() and the stop walker` | identity | ⛔ **NO.** Both arrays unchanged ⇒ identity preserved. |
| `GRAPHICS SHAPE / DIVERGENCE — … still admits all three` | 3 | ⛔ **NO.** Synthetic fixture, no nested `UUserWidget`. |
| `STEPPER SHAPES / DIVERGENCE — … admits all six buttons` | 6 | ⛔ **NO.** Same. |
| `VOCABULARY / DIVERGENCE — admits ONLY the plain UButton` | 1 | ⛔ **NO.** Same. |
| every `IsNavFocusStop(<fixture widget>)` true/false assertion | unchanged | ⛔ **NO.** `IsNavFocusStop` itself is **byte-unchanged** by this row. |

**Why zero, measured:** `qa/TASK-1476.md` found that **no fixture in this file contains a nested
`UUserWidget`** — I re-confirmed it (every fixture tree is `UVerticalBox`/`UHorizontalBox`/`UBorder`
panels plus leaf controls). And the PIE main-menu block, which *does* have two nested user widgets, is
held at 7 by the boundary. ⇒ **the descent cannot change any expected value in this file.**

⚠️ **The coordinator's warning that this row "reddens the PIE main-menu block" was written against a
naive `ForEachWidgetAndDescendants` swap.** Under the **bounded** descent it does not — and that is a
deliberate design consequence, not luck: **I did not want to have to argue a runtime-`VERIFIED` number
downward.**

### ⛔ THE REPLICA COMMENT — I TOOK **OPTION 2**: RECORD THE DIVERGENCE, DELIBERATELY AND DATED.

The comment claiming `CollectNavStops` is *"byte-for-byte `GetMenuFocusStops()`'s body"* is falsified by
this row. I **did not** port the recursion into the test's copy. The comment now strikes the false clause
(`SC-§120`), names exactly what diverged, and states the measurement that makes the choice defensible:
**no fixture contains a nested `UUserWidget`, so porting the recursion would execute ZERO times and
change ZERO results** — an inert edit to a test is review surface bought with nothing. It also states the
honest consequence in the file itself: ⛔ **the descent is UNCOVERED by this suite**, its evidence is the
runtime count, and if anyone ever adds a fixture with a nested `UUserWidget` this copy becomes **wrong**
rather than merely narrower and the recursion must be ported in the same diff.

⛔ **Scope held:** no new test, no deleted test, no changed assertion shape, no changed expected value.
**The only edit to that file is the one comment block.**

---

## §8 — THE TWO ROUTED DOC DEBTS, BOTH DISCHARGED

1. **`DeckBuilderWidget.h`** — *"registration turns the subsystem's generic ring ON over a screen that
   drives itself"*, stale since Row B. **Struck, not deleted** (`SC-§120`), and replaced with the fact:
   the escalation **landed** — `TASK-1471` shipped the subsystem-side fix and this class now calls
   `RegisterSelfDrivingMenuNavTarget`, so the generic ring is **not** turned on over this screen and
   `RouteMenuNavKey`'s repair is kept as a **net**, not as the mechanism. It also records that
   benefit (a) got sharper: the retarget count now includes the deck bar's ten `SlotButton`s.

2. **`SiegeMenuInputSubsystem.cpp`, `HandleMenuAccept`'s TASK-1471 gate banner** — *"the ONLY two
   statements"*. **False as written; struck and corrected.** There are **seven** control-activating
   statements, enumerated **by statement, never by line number** (a map written in indices goes stale on
   the next insertion with nothing to announce it):
   1. `SetCheckBoxChecked(CheckBox, !CheckBox->IsChecked(), …)` — `HandleMenuAccept`, below that return
   2. `Focused->OnClicked.Broadcast()` — `HandleMenuAccept`, below that return
   3. `SetCheckBoxChecked(CheckBox, Direction > 0, …)` — `StepFocusedStop`
   4. `Member->OnClicked.Broadcast()` — `StepFocusedStop`
   5. `Slider->SetValue(NewValue)` — `StepSlider`
   6. `Slider->OnControllerCaptureEnd.Broadcast()` — `StepSlider`
   7. `Slider->OnMouseCaptureEnd.Broadcast()` — `StepSlider`
   ✅ **The verdict "CLOSES" survives intact and on a wider footing than the old sentence claimed:**
   (3)–(7) are all behind `StepFocusedStop`'s own `DeclineIfActiveTargetSelfDriving` gate (its first
   statement after the unconditional entry instrument, before any other branch), and `StepSlider` is
   private with exactly **one** caller, downstream of that gate. **What was wrong was the map, not the
   conclusion** — and the map is what the next editor reads before deciding where a new activation may go.

**A third correction I took because I hold the file and my own change bears on it:** the
`HasVisibleSlateAncestry` banner's *"…and that overlay never registers, so this is a latent hazard for a
FUTURE screen, not a live one"* — struck, with the switcher trip-wire of §6 written in its place.

---

## §9 — WHAT CHANGED IN CODE (by content, never by a diff integer)

⚠️ Deletion counts are declared **by content**: `git diff --numstat`'s deletion integer is unstable
across Myers / `--patience` / `--histogram`, so no integer is quoted here.

**`SiegeMenuInputSubsystem.h`**
- **Added** `class UWidgetTree;` forward declaration.
- **Added** three private statics: `IsCodeAuthoredSubWidget(const UUserWidget*)`,
  `CollectNavStopsFromTree(const UWidgetTree*, TArray<UWidget*>&, int32)`, and
  `static constexpr int32 MaxNavDescentDepth = 8`.
- **Extended** `GetMenuFocusStops`'s declaration comment (descent + what it does and does not buy).
- **Struck and corrected** the clause *"are not even reached by `UWidgetTree::ForEachWidget`"* in
  `RegisterSelfDrivingMenuNavTarget`'s comment.
- ⛔ **No statement deleted. No `UPROPERTY`, no `UFUNCTION`, no member variable, no reflection surface**
  ⇒ **no UHT-visible delta; ordinary recompile.**

**`SiegeMenuInputSubsystem.cpp`**
- **Added** the definitions of `IsCodeAuthoredSubWidget` and `CollectNavStopsFromTree`.
- **Replaced one statement:** `GetMenuFocusStops`'s `Target->WidgetTree->ForEachWidget([&OutStops](…){…});`
  → `CollectNavStopsFromTree(Target->WidgetTree, OutStops, /*Depth*/ 0);`. **That lambda body is the only
  code deleted on this row, and it is preserved verbatim inside `CollectNavStopsFromTree`.**
- **Added** the named-slot correction and the self-driving caveat at the walker.
- **Struck and corrected** the two comment blocks of §8.
- ⛔ `IsNavFocusStop`, `HasVisibleSlateAncestry`'s body, `GetMenuButtons`, `MoveFocus`'s logic and every
  log format string are **byte-unchanged**.
- **No new `#include`** — `Blueprint/UserWidget.h` and `Blueprint/WidgetTree.h` were already present, and
  `UClass::HasAnyClassFlags` / `CLASS_Native` are complete via `CoreMinimal.h` (the file already calls
  `GetClass()->GetName()`).

**`Tests/SiegeMenuInputTest.cpp`** — one comment block (§7). No code, no values.
**`DeckBuilderWidget.h`** — one comment block (§8.1). No code.

**⛔ C4458 SWEEP (hard under `-WarningsAsErrors`), against the inheritance chain, not the project:**
new identifiers are `Tree`, `OutStops`, `Depth`, `SubWidget`, `SubWidgetClass`, `MaxNavDescentDepth`.
Swept against `USiegeMenuInputSubsystem`'s own members and **`UWorldSubsystem` → `USubsystem` → `UObject`
→ `UObjectBase`** (single base each, no interface, no secondary base — as verified twice this wave).
`UObjectBase`'s data members are `ObjectFlags`, `InternalIndex`, `ClassPrivate`, `NamePrivate`,
`OuterPrivate`; `UObject`, `USubsystem` and `UWorldSubsystem` add none. ⇒ **zero collisions.**

---

## §10 — WHAT QA SHOULD SCRUTINISE HARDEST

1. **§0.** Is "the count moves, the ring does not" an acceptable delivery of a row whose title says
   "reachable"? I say it must be **recorded and escalated**, not silently redefined — but the judgement
   is the manager's.
2. **The boundary predicate.** Is `CLASS_Native` the right line, or is it a proxy I have dressed as a
   property? My argument is in §1; the alternative I discarded, and why, is in the same table.
3. **The controls-help trip-wire (§6).** It is one row away and it is written into the subsystem rather
   than into that file, because that file is `TASK-1433`'s. Confirm that is the right place for it.
4. **The suite choice (§7).** Option 2 over option 1. If QA prefers the ported replica, it is a
   mechanical change — but it executes zero times.
5. ⛔ **Whether `SubWidget->WidgetTree` can be non-null on an *unrealized* nested native widget** and
   whether that costs a pointless walk. I believe it can, and I believe the cost is bounded and belongs
   to `TASK-1456` (perf), not here. **I did not add a visibility pre-check before descending** —
   deliberately, `SC-§50`: every child is already tested individually by `IsNavFocusStop`, so a
   pre-check would be a second rule buying only speed.

---

## Not examined / limitations

- ⛔ **No runtime reading of any kind.** No PIE, no editor, no MCP. Every number in §3 is **DERIVED**
  from construction source and from `.uasset` string-table measurement, delta'd onto `TASK-1469`'s
  table. **The deck builder's 13/14 has never been observed** — `TASK-1436` owns that.
- ⛔ **I did not measure whether `WBP_DeckCardTile`'s `Construct` collapses its `Btn_Jump`.** I could not
  under this row's fences — and the whole design exists so the answer **does not matter**. If someone
  later measures it, it neither validates nor invalidates the boundary.
- ⛔ **`UI_Thumbstick`'s interior was not enumerated by class.** Its `.uasset` names only `Thumbstick`
  and `Thumb`, which look like images, but I did not confirm no `UButton` hides there. **Irrelevant
  under this design** — it is a Blueprint and is never entered — but it means I **cannot** tell you what
  a naive `ForEachWidgetAndDescendants` swap would have cost the main menu, only that it would have been
  nonzero risk against a `VERIFIED` number.
- ⛔ **The controls-help row count `N` is unknown to me.** I state its delta as **0**, which is
  independent of `N`; I do not state `N`.
- ⛔ **`DeckBuilderWidget.h:1126-1128`** still says the tiles *"are not even reached by it
  (`UWidgetTree::ForEachWidget` descends through `UPanelWidget` only)"*. Its **conclusion is still true**
  after this row (the tiles remain unreached), but its **parenthetical mechanism is the wrong one** (§2).
  ⛔ **I did not touch it** — my grant on that file is the one stale sentence of §8.1 only.
  **Owed: a one-line correction, for the manager to board.** Suggested replacement for the parenthetical:
  *"(they are Blueprint sub-widgets, which `USiegeMenuInputSubsystem::IsCodeAuthoredSubWidget` refuses to
  enter — the walker's descent boundary, TASK-1474)"*.
- ⛔ **I did not cite the automation suite as evidence of anything.** It is not run, and `TASK-1476`
  PASSED without lifting that prohibition.
- ⚠️ **NOTICED, NOT FIXED — four PRE-EXISTING line-number citations into a file that is being edited
  right now.** `SiegeMenuInputSubsystem.h` and `.cpp` each cite `SiegeControlsHelpWidget.cpp:177`, and
  `Tests/SiegeMenuInputTest.cpp` cites `:176-178` twice. **None of the four is mine** — they predate this
  row (`TASK-1406` / `TASK-1469` / `TASK-1475`) — but `TASK-1432`/`TASK-1433` are editing that file
  tonight, so all four are about to point at the wrong lines. **I added zero new line citations into it**
  and cite it by text everywhere. ⛔ Correcting the four is **not** in my grant (two are in a file I hold,
  two are in a file I hold under a values-only grant) and is offered to the manager as a trivial row.
  The symbol to anchor on is that file's `ApplyButtonNotFocusable` helper.
- ⛔ **No claim about in-match behaviour.** `WBP_HUD` / `WBP_CardHand` are argued in §1 from the fact that
  they never register; the in-match arm and
  ⚖️ `InMatchMenuMappingContextPriority` were not touched or read for a decision.
