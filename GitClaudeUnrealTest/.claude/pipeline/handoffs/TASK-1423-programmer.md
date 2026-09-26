# TASK-1423 — [MENU-NAV-DECKBUILDER-RELAYER] — programmer handoff

**Marker:** `TASK-1423-MENU-NAV-DECKBUILDER-RELAYER`
**Status set:** `ready-for-qa` → routes to `TASK-1424` (QA) → 5a → 5b = `TASK-1427` → commit `TASK-1428`
**Files written:** `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` · `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` · this file · this row's `status:`
**Files NOT written (fence (5) honoured):** no `.uasset`, no `SiegeMenuInputSubsystem.{cpp,h}`, no other screen, no `CONVENTIONS.md`, no git, no compile, no PIE, no MCP.
**Diff:** `+392 / −2` (.cpp) · `+192 / −0` (.h). **Both deletions declared in §9.**

---

## 0. THE ONE QUESTION THE DISPATCH ASKED FIRST: *entered-and-wrong, or never entered?*

⭐ **NEVER ENTERED. Proven from the call graph, no runtime required, and it is the fact the whole row turns on.**

```
inject_input_action
  → UEnhancedInputLocalPlayerSubsystem::InjectInputForAction
  → the PC's UEnhancedInputComponent action bindings          ← the ONLY consumers
```

There is **no edge** from that component into `UUserWidget::NativeOnKeyDown` or `NativeOnPreviewKeyDown`. Slate key routing begins at `FSlateApplication::ProcessKeyDownEvent` over `SlateUser->GetFocusPath()` (`SlateApplication.cpp:5015-5017`) — a path an injected **action** never enters. Before this row the deck builder's only doors were those two Slate overrides. ⇒ `HandleCardGridKey` was **never called** on the injected lane, on any run, healthy or broken.

⇒ `TASK-1399` §2 row 2's `-1 → -1 → -1` and `TASK-1306`'s `-1` are **both** "the handler never ran", not "the handler ran and computed `-1`". **The logic was never wrong; only the entry point was missing.** That is why this row moves the entry point and rewrites no navigator.

---

## 1. THE TWO (now THREE) ENTRY POINTS, SHOWN CALLING **ONE** IMPLEMENTATION

`HandleCardGridKey` **was** the one decision function behind two Slate doors. It now sits behind three, and **no key table was copied**.

| # | Door | Lane | Reaches the body via |
|---|---|---|---|
| 1 | `NativeOnPreviewKeyDown` (Slate tunnel, root→leaf) | 🧑 his real keys | `HandleCardGridKey(InKeyEvent)` — **call site byte-identical** |
| 2 | `NativeOnKeyDown` (Slate bubble, leaf→root) | 🧑 his real keys | `HandleCardGridKey(InKeyEvent)` — **call site byte-identical** |
| 3 | `RouteMenuNavKey` ← 6 × `IA_Menu*` Enhanced Input bindings | 🤖 the rig / gamepad | `HandleCardGridKey(Key)` |

The whole of the refactor:

```cpp
// the ONE implementation — signature changed, BODY BYTE-IDENTICAL (:1664)
FReply UDeckBuilderWidget::HandleCardGridKey(const FKey& Key) { /* ...unchanged... */ }

// the Slate adapter, so doors 1 and 2 are not edited at their call sites (:1823)
FReply UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent& InKeyEvent)
{
    return HandleCardGridKey(InKeyEvent.GetKey());
}
```

The third door is six one-line functions, each handing the body **a key the shipped table already names**:

| Action | Key handed to the one implementation | What the shipped table already does with it |
|---|---|---|
| `IA_MenuUp` | `EKeys::Up` | grid live ⇒ `MoveCardFocus(Up)` |
| `IA_MenuDown` | `EKeys::Down` | grid live ⇒ `MoveCardFocus(Down)`; **not live ⇒ ENTER the grid at index 0** |
| `IA_MenuLeft` | `EKeys::Left` | grid live ⇒ `MoveCardFocus(Left)` |
| `IA_MenuRight` | `EKeys::Right` | grid live ⇒ `MoveCardFocus(Right)` |
| `IA_MenuAccept` | `EKeys::Enter` | grid live ⇒ `AcceptFocusedCard()` ⇒ `AddCopy` |
| `IA_MenuBack` | `EKeys::Gamepad_FaceButton_Right` | grid live ⇒ `ExitCardGridFocus()` (the grid→bar crossing) |

⛔ **No key gains a meaning it did not have. No rule is re-stated. No second table exists to diverge.** The FKey the relay hands over is always one already on the `:1639` / `:1663` / `:1727` lines.

**Bound with `ETriggerEvent::Started`** — byte-identical to the event `USiegeMenuInputSubsystem` binds the same six assets on (`SiegeMenuInputSubsystem.cpp:180-199`), so one press is one step on both lanes.

**Asset names are not re-typed:** the six paths are read from `USiegeMenuInputSubsystem::Menu*ActionPath` (public statics), so the widget and the subsystem cannot bind two different assets that share a name.

### 1a. ⭐ THE SLATE PATH SURVIVED — and here is why it had to

`BP_MenuGameMode` sets `FInputModeUIOnly`, whose `ApplyInitialFocus` calls `GameViewportClient.SetIgnoreInput(true)` ⇒ **a real keypress never reaches Enhanced Input on `L_MainMenu`.** So:

- door 3 (IA) serves **only** the rig / injection;
- doors 1 and 2 (Slate) serve **only** 🧑 his hands.

They are **disjoint, not redundant.** Deleting either would ship a screen that works for exactly one of us. Doors 1 and 2 are unchanged in behaviour *and unchanged at their call sites* — `git diff` shows zero modified lines inside `NativeOnPreviewKeyDown` / `NativeOnKeyDown`.

---

## 2. THE GRID ↔ DECK BAR CROSSING RULE (both regions named, neither dropped)

| Crossing | Gesture | Lane 1+2 (Slate / 🧑 his keys) | Lane 3 (`IA_Menu*` / 🤖) |
|---|---|---|---|
| **Bar → Grid** | `Down` with the grid not live | ✅ shipped (`HandleCardGridKey` `:1751`) | ✅ **new** — `IA_MenuDown` |
| **Grid → Bar** | the exit gesture ⇒ `ExitCardGridFocus()` ⇒ focus lands on the deck-bar entry's own `SlotButton` (`ResolveGridExitFocusTarget` `:1351`) | ✅ shipped (Escape / `Gamepad_FaceButton_Right` / virtual Back) | ✅ **new** — `IA_MenuBack` |
| **within the bar** (entry → entry, Enter → `SelectDeckForEdit`) | `Left`/`Right`/`Enter` on a `SlotButton` | ✅ shipped — **Slate's own navigation**, untouched (DECK-§3) | ⛔ **NOT REACHABLE — see §5 escalation E2** |
| **within the grid** | 2-D ring, `Up`/`Down` step by the live row width | ✅ shipped | ✅ **new** |

⛔ **I did NOT build bar traversal.** It would mean a *third* navigator inside this widget, intercepting the `Left`/`Right`/`Enter` that Slate already delivers on a hand-confirmed path (DECK-§3's bar contract, which the shipped comments pin as untouched). The gap is subsystem-side and is escalated, not papered over.

---

## 3. THE `TASK-1306` CRITERION, RESTATED AS **THIS ROW'S** ACCEPTANCE

> ⛔ **`FocusedCardIndex != -1` after `inject_input_action IA_MenuDown`.**
> It is a **direct re-run of a recorded failure** (`TASK-1306`: `-1` while the builder's root demonstrably held Slate focus; `TASK-1399` §2 row 2: `-1 → -1 → -1` across two injected Downs).
> A `-1` on the post-5a binary is a **`VERIFY-FAILED`, not an `UNOBSERVABLE`** — the instrument is demonstrably alive.

**The mechanism that now makes it pass:** `IA_MenuDown` → `UDeckBuilderWidget::HandleMenuNavDown` → `RouteMenuNavKey(EKeys::Down)` → `HandleCardGridKey(Down)` → grid not live → `SetFocusedCardIndex(0)` → `FocusedCardIndex = 0`.

🚨 **AND THE HONEST HALF, BECAUSE THE TWO LIMBS CAN DISAGREE HERE:** `SetFocusedCardIndex` needs only **`DT_Cards`** (it clamps against `GetCollectionCardIDs().Num()`), while the *visible ring* needs **`CardTileClass`** to resolve. ⇒ if `/Game/UI/WBP_DeckCardTile.WBP_DeckCardTile_C` fails to resolve, **`FocusedCardIndex` becomes 0 with no ring on screen** — the 🤖 state limb passes and the 🧑 pixel limb fails. Discriminator: the once-per-widget Warning `UDeckBuilderWidget: card-tile class '%s' did not resolve — keyboard card focus keeps working but draws no outline.` (`:1028`). **If that line is present, a green state limb is not a green screen.**

---

## 4. ⛔ THE (4) DECLARATION — MADE **EXPLICITLY, BOTH WAYS**

### 4a. THE UNREGISTER **IS** REACHABLE FROM C++ ON THE BP EXIT PATH — no escalation needed, no BP edit authored

`WBP_DeckBuilder`'s Exit is BP-opaque (no `RemoveFromParent` for the builder anywhere in `Source/`). It does **not need to be C++-visible, only C++-reached**, and it is — traced in the 5.8 source, not assumed:

```
BP `RemoveFromParent(self)`
  → the viewport slot drops its reference to this widget's SObjectWidget
  → ~SObjectWidget (SObjectWidget.cpp:52) → SObjectWidget::ResetWidget (:55)
  → WidgetObject->NativeDestruct()                                  (:63)
```

`NativeDestruct`'s **first two statements** are now `UnregisterAsMenuNavTarget(); UnbindMenuNavActions();` — LIFO against `NativeConstruct`'s tail, **beside** the existing `bFocusReadbackPending = false` teardown, never instead of it (`TASK-1417`'s measured law).

⚠️ **The one honest weakness, named:** that chain is driven by a **shared-pointer release**, so it is reachable but **not guaranteed synchronous** with `RemoveFromParent` — anything still holding a `TSharedPtr` to the `SObjectWidget` defers it. There is therefore a window in which the builder is out of the viewport and still on the nav stack.
⭐ **That window is exactly what `TASK-1406`'s net covers**, which is why this is a note and not a defect: `GetRegisteredNavTarget()` re-validates `IsInViewport() && IsVisible()` on **every** read and skips a removed screen ⇒ `IsNavTargetActionable()` falls back to `IsMenuUncovered()` ⇒ `TASK-1400`'s 0.2 s poll re-arms the main menu's top option. 🧑 **His complaint ("exit the deck builder … the outline is no longer there") holds even in the deferred case.**

### 4b. THE **CLOSE** IS **NOT** REACHABLE, AND I AM NOT GUESSING IT — ESCALATED

This widget deliberately **does NOT implement `ISiegeMenuNavCloseTarget`.** Closing the builder is `CreateWidget(WBP_MainMenu_C) → AddToViewport → RemoveFromParent(self)` in a Blueprint graph (`TASK-1399` §5.3). A C++ `RemoveFromParent()` from a close handler would be **guessing a teardown** — the exact thing `HandleMenuBack`'s own comment forbids — and could leave the player on `L_MainMenu` with **no main menu** or with **two**. ⇒ `IA_MenuBack` logs `implements no ISiegeMenuNavCloseTarget — Back is INERT for it` at the subsystem, which is the **boarded, designed state**; `TASK-1454` wires the real close. **No Blueprint node was authored by this code row.**

⇒ ⛔ **The two-press Back semantics 🧑 he already has are preserved exactly: once leaves the GRID; the second press arrives with the grid unfocused and falls through.**

---

## 5. 🚨 ESCALATIONS — declared, not silently scope-grabbed (`SC-§50`)

### E1 — ⛔ REGISTRATION TURNS ON A **SECOND NAVIGATOR** OVER A SCREEN THAT DRIVES ITSELF. The clean fix is subsystem-side.

I **did** register (the row's (1)). ⚠️ **But registering here does not mean what it means on Settings, and QA must check this paragraph, not the call.**

On Settings the subsystem's ring **is** the screen's navigation. Here it **cannot be**, for three independent reasons, all confirmed at source:
1. `GetMenuFocusStops()` walks `Target->WidgetTree->ForEachWidget`, whose descent is a `UPanelWidget` cast (`UWidgetTree::ForWidgetAndChildren`) ⇒ a nested `UUserWidget` is **visited but never descended into**. The ten `UDeckSlotEntryWidget` bar entries and the N `WBP_DeckCardTile` tiles are nested `UUserWidget`s ⇒ their inner `UButton`s are unreachable. **((4b)(i) confirmed.)**
2. `IsNavFocusStop()` admits four classes; a tile is made focusable **as a `UUserWidget`** (`FocusCardTile` `:1089`) ⇒ **tree descent alone would still admit nothing.** **((4b)(ii) confirmed — I did not "add descent and declare victory".)**
3. The ring here is **two-dimensional** (`ResolveGridColumns` / `StepCardFocusIndex`); a linear wrap over a flat stop list cannot express it at all.

**Why I registered anyway** — for the two things it *does* buy: **(a)** `LogNavTargetRetarget` prints **this screen's focus-stop count**, which is the binding instrument of `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` (`TASKBOARD.md:6059`); without a registration there is **no count to falsify §6 against, on any lane**. **(b)** it is the seam `TASK-1454` needs.

**The cost, declared:** `USiegeMenuInputSubsystem` binds its `IA_Menu*` handlers at `OnWorldBeginPlay` — **before** this widget's `NativeConstruct` — so its delegate is **earlier** in the component's array and its `MoveFocus(+1)` runs **first on the same press**. Once registered, that walker is live over this tree and can place the ring on one of the builder's own `UButton`s, after which `IsCardGridFocusLive()` reads false and the grid would re-arm at index 0 **every press instead of moving.**

**What I did about it (inside the fence):** `RouteMenuNavKey` carries **one precondition repair** — if the **model** says the grid is armed and **Slate** disagrees, re-assert the tile's focus with `FocusCardTile(FocusedCardIndex)` **before** dispatching. ⛔ It is **not a key rule**: it is the identical re-assert `AcceptFocusedCard` and `RemoveFocusedCard` already make for the WBP-rebuilt-the-grid case — the house idiom, already shipped, applied at a new moment. It cannot ARM the grid (`FocusedCardIndex != INDEX_NONE` gates it), and it is **deliberately not added to the Slate doors**, whose behaviour must stay byte-identical.

⇒ 🚨 **THE MANAGER ROW I AM ASKING FOR:** the subsystem needs a way for a screen to say *"I own my own navigation — register me for Back and for the stop-count line, but do NOT walk my tree."* A `bScreenDrivesOwnNavigation` on `RegisterMenuNavTarget`, or an `ISiegeMenuNavDriver` the handlers check before `MoveFocus`. **That is subsystem-side and fence (5) forbids me to write it.** Until it exists, the composition converges on the grid every press by repair rather than by construction.

### E2 — ⛔ THE DECK BAR IS UNREACHABLE FROM THE INJECTED LANE, and for the same root cause.
The bar's `SlotButton`s live inside nested `UUserWidget`s ⇒ invisible to `GetMenuFocusStops` ⇒ `IA_MenuLeft/Right/Accept` find no stop and log "no semantics". Slate's own navigation walks the bar fine for 🧑 him. Fixing it means either the subsystem descent+vocabulary change (E1's file) or a third navigator in this widget (rejected — see §2). **Not fixed here. Named here.**

---

## 6. ⛔⛔ BINDING PRE-5b DECLARATION — THE EXPECTED STOP LISTS

*(`EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24`, `TASKBOARD.md:6059` — stated BEFORE 5b so `TASK-1427` can FALSIFY it. **Two stop sets exist on this screen and conflating them is the trap.**)*

### 6A — THE SUBSYSTEM'S STOP SET (what `LogNavTargetRetarget` will print on registration)

Walker: `UWidgetTree::ForEachWidget` over `WBP_DeckBuilder`'s tree, depth-first pre-order, admitting `UButton` / `UCheckBox` / `USlider` / `UEditableTextBox` that are members of **that tree's own panel hierarchy**.

| Expected stop | Class | Construction route | In the list? |
|---|---|---|---|
| `Btn_Jump` (under `Overlay_19 → SizeBox_0`) | `UButton` | **asset-authored** (third-person template; the `EVENTGRAPH-CENSUS-GAP` ruling names `WBP_DeckBuilder` as one of the five carrying it) | ✅ **YES, iff visible + enabled** |
| `UI_Thumbstick` × 2 | `UUserWidget` | asset-authored (template) | ❌ not an admitted class |
| `DeckBar` | `UHorizontalBox` | asset-authored (`BindWidgetOptional`) | ❌ not an admitted class |
| `SlotButton` × 10 | `UButton` | **C++** (`DeckSlotEntryWidget::ConstructEntryTree`) | ❌ **NO — inside a nested `UUserWidget`; the walker does not descend** |
| `UDeckSlotEntryWidget` × 10 | `UUserWidget` | **C++** (`DeckBuilderWidget.cpp:607`) | ❌ visited, not admitted |
| `WBP_DeckCardTile` × 34 | `UUserWidget` | **EventGraph** (`CreateWidget` → `AddChildToWrapBox`) | ❌ visited, not admitted |
| `WBP_DeckBuilder`'s own authored buttons (Exit / Play / Save / ±) | `UButton` | **asset-authored** | ⚠️ **count = `A`, UNKNOWN from `Source/`** — see below |

⇒ **PREDICTED COUNT: `1 + A`**, where `A ≥ 0` is the number of `UButton`s `WBP_DeckBuilder` authors directly in its own tree.
⛔ **`A` IS HONESTLY UNKNOWN AND I AM SAYING SO RATHER THAN GUESSING A NUMBER.** The only authored member C++ binds is `DeckBar`; the asset is BP-opaque and **this row holds no asset-read lane** (no MCP, no editor). **THE RULE, WHICH IS WHAT THE RULING ASKS FOR:** *"every `UButton`/`UCheckBox`/`USlider`/`UEditableTextBox` in `WBP_DeckBuilder`'s own panel hierarchy that is visible and enabled, PLUS zero from every nested `UUserWidget`."*

**FALSIFIERS — any one of these is a finding:**
- **F1** — a `SlotButton`, a `WBP_DeckCardTile` or any tile-internal name appears among the stops ⇒ **(4b)(i) is wrong about descent** ⇒ escalate to `TASK-1465`.
- **F2** — the count is **0** ⇒ `Btn_Jump` is collapsed/disabled on this screen **and** `A == 0`. Not a failure of this row (the grid does not use this set) but a real reading about the template overlay.
- **F3** — the count is **≥ 10** ⇒ descent happened ⇒ same as F1.
- ⚠️ `LogNavTargetRetarget` prints a **count, not names**. The name-level checks need `ui_snapshot`; **the count alone cannot separate F1 from a large `A`.**

### 6B — THE CARD-GRID RING (this row's actual deliverable)

⛔ **Not a bare integer — the RULE:** the index space is `GetCollectionCardIDs()` = **`DT_Cards::GetRowNames()`**, i.e. **every row in the card table, in CSV/import order**. It is **NOT deck-dependent** — `GetCollectionCardIDs` reads the *table*, never the deck, so the count is identical for `deck1`…`deck10` and for an empty slot.

- **Measured against the committed table:** `Docs/Data/cards.csv` = **34 data rows** (`Footman, Archer, Knight, Miner, ArrowTower, Wall, MilitiaMob, Pikeman, Sapper, Cavalry, Longbowman, Cleric, Ogre, BombTower, BallistaTower, Barracks, DeepMine, Masons, SharpenedBlade, PlateArmor, SwiftBoots, WarBanner, Fireball, FrostNova, Lightning, BattleCry, Pickpocket, CrystalTower, Wizard, Sorcerer, WatchTower, Witch, Fog, BrightSun`). ⇒ **N = 34 today**, and the criterion is `0 ≤ FocusedCardIndex ≤ N−1`.
- **Order:** DT_Cards row-map order, which is the order the WBP fills the WrapBox in — so grid order and index order coincide **by construction**, not by luck.
- **Construction route of every stop in this set:** **EventGraph** (`CreateWidget(WBP_DeckCardTile)` → `AddChildToWrapBox`), recognised in C++ by `CollectCardTiles` matching `CardTileClass`.

**UNHAPPY PATHS AND THEIR DISCRIMINATORS:**

| Unhappy path | Count | Runtime criterion | Discriminator (the line that tells them apart) |
|---|---|---|---|
| `DT_Cards` unresolvable | **0** | `FocusedCardIndex` stays **`-1`** | `UDeckBuilderWidget: card table '…' not found — the collection shows empty and AddCopy refuses (logged once).` (`:2383`) |
| `CardTileClass` unresolvable | 34 (model) / **0 tiles** | `FocusedCardIndex` = **0 ✅** but **NO RING** | `card-tile class '…' did not resolve — keyboard card focus keeps working but draws no outline.` (`:1028`) |
| the relay never armed | n/a | `-1` | **absence** of `UDeckBuilderWidget::BindMenuNavActions: N IA_Menu* action(s) bound (Started) on '…'` — one `Log`-level line per builder open |
| an `IA_` asset missing | n/a | that key inert only | the same line's `; ABSENT, so THOSE keys are inert on this lane: <names>` clause |
| the action arrived but declined | `-1` | `-1` | `RouteMenuNavKey: IA_MenuDown -> 'Down' (FocusedCardIndex=…, grid live=…)` — **`Verbose`**, so `Log LogGitClaudeUnrealTest Verbose` must be typed FIRST or it prints nothing |

🚨 **OWED TO THE VERIFIER: type `Log LogGitClaudeUnrealTest Verbose` before the sitting**, or the per-press instrument prints nothing **and an empty log reads as a zero** (this project has paid for that before). The **arm** line is `Log` level and needs nothing.

---

## 7. ⚖️ THE `TASK-1418` INHERITANCE — HOW MY STOP SET BEHAVES FOR A **HIDDEN** TILE

*Inherited, not re-litigated: `IsNavFocusStop` reads a widget's **own** Slate visibility and never its ancestors' ⇒ a collapsed parent does not hide its children from the subsystem's ring.*

**Said out loud, for set 6B, which is the one that matters here:**

⛔ **`CollectCardTiles` / `SiegeDeckCardFocus::GatherTiles` apply NO visibility filter and NO enabled filter at all.** A `Collapsed` or `Hidden` tile is still gathered, still counted in grid order, and still handed to `SetUserFocus`. If it has no live focus path, `FocusCardTile` falls through to the deferred `GetSlateOperations().SetUserFocus` and **returns `true` anyway** ⇒ `FocusedCardIndex` would advance onto an invisible card and **the ring would vanish while the model insists it is there.**

⛔ **AND THE INDEX SPACE IS THE TABLE, NOT THE LIVE TILE LIST** — `GetCollectionCardIDs()` is `DT_Cards` rows. So if the WBP ever renders **fewer** tiles than rows (a filter, a search box, a paged grid), the ring's index space is **larger** than the visible grid and the extra indices are silently unfocusable.

⭐ **WHY I DID NOT FIX IT:** it has **no live trigger today.** The shipped builder has no filter/search UI and the WBP builds exactly one tile per `GetCollectionCardIDs()` entry ⇒ tiles rendered = 34 = rows, none hidden. Adding a visibility filter would change the shipped, hand-confirmed navigator's arithmetic (`StepCardFocusIndex` is asserted by the suite against a flat count) for a case that cannot occur. **It is latent. It is named. The day a filter is added to `WBP_DeckCardTile`'s grid, this is the defect that ships with it.**

## 7a. ⭐ THE `TASK-1417` SCROLL WARNING — **ALREADY DISCHARGED IN SHIPPED CODE**, inherited and re-confirmed

The dispatch warns that `EScrollWhenFocusChanges::NoScroll` is the engine default and must be configured **pre-`Super::RebuildWidget()`** plus a `NativeOnFocusChanging` hook. ⛔ **Neither is needed here, and neither was added.** `FocusCardTile` (`:1100-1107`) already walks to the nearest `UScrollBox` ancestor and calls `ScrollWidgetIntoView(Tile, /*AnimateScroll*/ true)` **explicitly**, with a shipped comment naming `ScrollBox.cpp:31` and `SScrollBox.cpp:1443-1452` as the reason. ⇒ a tile below the fold scrolls into view on **every** lane, including the new one (it goes through the same `FocusCardTile`). **No `SetScrollWhenFocusChanges`, no WBP-owned configuration rewritten.**

---

## 8. ⚖️ `ESCAPE` — UNTOUCHED, AND THE GRANT NOT WIDENED BY ONE KEY

- `EKeys::Escape` appears in **exactly one place** in this file, unchanged: the `:1727` exit line inside `HandleCardGridKey`'s `bGridFocused` block, under 🧑 his `TASK-1300` "Scoped — Escape may exit the card grid" ruling. **Zero lines of that block were edited.**
- ⛔ **The third door does NOT map `IA_MenuBack` to `Escape`.** It maps it to `EKeys::Gamepad_FaceButton_Right` — a key the shipped exit line **already** names. `IA_MenuBack` is authored on `Backspace` + `Gamepad_FaceButton_Right` (`TASK-1408`), and this row widens that by **zero** keys.
- `AS-§6 A-2` remains **unrelaxed** for the assistant console and `ASiegePlayerController`'s in-match cancel routes. **This row absorbs `Escape` nowhere new.**

---

## 9. ⛔ THE TWO DELETED LINES, DECLARED (house shape is `+N/−0`; this is `+392/−2` and `+192/−0`)

`git diff -U0 | grep '^-'` returns exactly:

```
-FReply UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent& InKeyEvent)
-	const FKey Key = InKeyEvent.GetKey();
```

1. **the signature line** — replaced in place by `HandleCardGridKey(const FKey& Key)`, and the deleted text **re-appears verbatim 159 lines later** as the one-line Slate adapter. Nothing was removed; one function became two.
2. **the extraction line** — replaced **in place** by a single comment line, so the key table's internal addresses (which DECK-§9 cites by number) suffer **zero net shift within `HandleCardGridKey`**. The extraction itself moved up one frame, into the adapter.

⛔ **Neither deletion removes a behaviour, a key, a guard or a rule.** The alternative — a second key table for the IA lane — is what `TASK-1424` (1) correctly calls an automatic FAIL.

⚠️ **Line-number drift, declared:** inserting `BindMenuNavActions()` / `RegisterAsMenuNavTarget()` into `NativeConstruct`'s tail shifts everything below it, so the key-table block as a whole moved from `:1616` to `:1664`. Its **internal** offsets are unchanged.

---

## 10. FOR 🧑 JONATHAN — cl. 3(b)

> **"Open the Deck Builder and use only the arrow keys — does the outline move around the card grid, and can you get to the deck bar at the bottom and back?"**
>
> *(⛔ A highlight that appears ⛔ only while the mouse is ⛔ HOVERING a control is a ⛔ NO. ⛔ An outline already sitting where you ⛔ left it is a ⛔ NO — ⛔ that is Slate ⛔ restoring old focus, ⛔ not our code. ⛔ If you pressed ⛔ any key or ⛔ moved the mouse before looking, the answer ⛔ does not count — ⛔ just say so and we'll redo it. ⛔ Roughly what time, so the log line can be found.)*

⚠️ **For his leg specifically:** the grid→bar exit gesture on his keyboard is **`Escape`** (his own `TASK-1300` grant) or gamepad **B**; getting back into the grid is **`Down`**. Moving *along* the bar is Slate's own `Left`/`Right`.

---

## 11. WHAT QA SHOULD SCRUTINISE (ranked)

1. **§5 E1 — the double-navigator.** Is registering defensible given the repair, or should this screen not register at all? I argue registering, for the stop-count instrument the census ruling makes binding. **This is the one judgement call in the row and it is the manager's to overturn.**
2. **The repair in `RouteMenuNavKey`** — is it a *precondition repair* or a *second behaviour*? I claim the former (it is `FocusCardTile(FocusedCardIndex)`, the identical call `AcceptFocusedCard`/`RemoveFocusedCard` already make) and that it is correctly absent from the Slate doors.
3. **Register-BEFORE-`AcquireBuilderFocus`.** `RegisterMenuNavTarget` ends in `FocusFirstNavStop()`, which cannot see the builder root (a `UUserWidget` is not a stop) and could move focus. Ordering it before `AcquireBuilderFocus` makes the end-of-construct focus state **byte-identical to what shipped**. **Falsifier: `TASK-1307`'s POST-FLUSH read-back must still print `IDENTITY=MATCH`.** A `NO-MATCH` on this screen after this row is this ordering being wrong and nothing else.
4. **`UnbindMenuNavActions` removes BY HANDLE, never `ClearActionBindings()`** — the component is **shared** with `USiegeMenuInputSubsystem`, which bound the same six assets on it. A blanket clear would kill main-menu navigation for the session and would look like a fix on the way past.
5. **`ETriggerEvent::Started`** on all six — same as the subsystem, so one press is one step on both lanes.
6. **Double-bind on re-open.** `BindMenuNavActions()` calls `UnbindMenuNavActions()` first, because a `UUserWidget` survives `RemoveFromParent`/`AddToViewport` and `NativeConstruct` runs again on the same instance.
7. **§6A's `A`** — I refused to invent a number for the WBP's own authored buttons. Check that refusal is honest rather than lazy: `Source/` binds exactly one authored member (`DeckBar`), and I hold no asset-read lane on this row.

---

## Not examined / limitations

- **L1 — NO COMPILE, NO PIE, NO MCP, NO GIT.** Not this role's; `build-master` owns 5a. Every engine claim here is traced to 5.8 source lines, not to a run.
- **L2 — `A` (the WBP's own authored `UButton` count) is UNMEASURED.** `WBP_DeckBuilder.uasset` is BP-opaque and this row holds no asset-read lane. §6A states the **rule** and the falsifiers instead of a number.
- **L3 — the third door is NOT covered by the automation suite.** `SiegeDeckSlotsTest.cpp` `NewObject`s a builder and **never runs `NativeConstruct`** (asserted there by design, to keep the auto-save funnel refused), so nothing binds and nothing routes. The routing claim is measurable at **runtime only** — the same property the header already declares for doors 1 and 2. ⛔ I did not widen anything for a test.
- **L4 — `Remove` (Delete / `Gamepad_FaceButton_Left`) is Slate-only.** `TASK-1408` authored no `IA_` action for it; inventing a seventh binding would be an unreviewed asset dependency. Named in the arm log line.
- **L5 — deck-bar traversal is Slate-only** on the injected lane (§5 E2).
- **L6 — `IA_MenuBack` does NOT close the builder** (§4b). Inert by design until `TASK-1454`.
- **L7 — the hidden/filtered-tile case is LATENT and UNFIXED** (§7). No live trigger today.
- **L8 — the `NativeDestruct` chain is reachable but not provably SYNCHRONOUS** with `RemoveFromParent` (§4a). Covered by `TASK-1406`'s re-validation net; the window is real and is named.
- **L9 — I did not verify that `Btn_Jump` is visible on this screen.** §6A's `1 +` term assumes the template overlay is not collapsed on desktop; F2 is the falsifier.
- **L10 — the two lanes' interleaving on one press is ARGUED FROM BINDING ORDER, not measured.** `OnWorldBeginPlay` precedes `NativeConstruct`, so the subsystem's delegate is earlier in the array; Enhanced Input fires bindings in array order. If that ever stopped holding, the repair would run *before* the steal instead of after and the grid would lose a press. **Discriminator:** the subsystem's `MoveFocus(+1): focus moved …` line and `RouteMenuNavKey`'s line appearing for the same press, in that order.

---

# QA LOOP 1 of 3 — 5a COMPILE FAILURE, FIXED 2026-09-25

Marker `TASK-1423-LOOP1-C4458-RENAMED-2026-09-25`. Appended, not a rewrite: everything above this line is the
`ready-for-qa` submission of 2026-09-24 and still stands unamended.

## L1.1 — What failed

`Result: Failed (OtherCompilationError)` at `UnrealBuildTool/Log.txt:211`. Two diagnostics, both mine, both the
same added line:

```
DeckBuilderWidget.cpp(1901,27): error C4458: declaration of 'InputComponent' hides class member
DeckBuilderWidget.cpp(2001,27): error C4458: declaration of 'InputComponent' hides class member
```

The hidden member is `UUserWidget::InputComponent` (`UserWidget.h:1764`). The build runs `-WarningsAsErrors`
(`Log.txt:69`), so C4458 is a hard error. 22 of 25 actions ran; the link never ran.

This was **not** a QA miss. `qa/TASK-1424.md` gave a clean PASS and was right about every API call it checked —
the collided name is not in this repository at all, so no text-level review of the diff and no census over
`Source/` could have found it. Its 5 WARN and 2 NIT are **not** reopened and nothing below acts on them.

## L1.2 — The fix

Rename only. `InputComponent` -> **`EnhancedInputComp`**, at both declaration sites and their four uses:

| Line | Function | Was | Now |
|---|---|---|---|
| 1901 | `BindMenuNavActions` | `UEnhancedInputComponent* InputComponent = ...` | `UEnhancedInputComponent* EnhancedInputComp = ...` |
| 1902 | `BindMenuNavActions` | `if (InputComponent == nullptr)` | `if (EnhancedInputComp == nullptr)` |
| 1959 | `BindMenuNavActions` | `InputComponent->BindAction(...)` | `EnhancedInputComp->BindAction(...)` |
| 2001 | `UnbindMenuNavActions` | `UEnhancedInputComponent* InputComponent = ...` | `UEnhancedInputComponent* EnhancedInputComp = ...` |
| 2002 | `UnbindMenuNavActions` | `if (InputComponent != nullptr)` | `if (EnhancedInputComp != nullptr)` |
| 2012 | `UnbindMenuNavActions` | `InputComponent->RemoveBindingByHandle(Handle)` | `EnhancedInputComp->RemoveBindingByHandle(Handle)` |

The name carries the **type**, not the role, which is what makes it collision-proof here: `UUserWidget`'s member
is named for the role.

**`PC->InputComponent` is deliberately untouched at 1901 and 2001.** That is `APlayerController::InputComponent`,
the *source* of the `Cast`, and renaming it would be wrong. A reviewer checking the rename by eye should expect to
still see the token `InputComponent` on both declaration lines — once, on the right-hand side.

Nothing else in either function changed: no comment edits, no reordering, no log-string edits (the strings say
"UEnhancedInputComponent", the *type*, which is still correct).

## L1.3 — The base-class shadowing sweep, and HOW it was done

The lesson from the bounce is that the shadowing surface is every member of `UUserWidget` and its bases, so a
project grep is a false negative by construction. **No project grep was used to establish this section.**

`UDeckBuilderWidget : public UUserWidget` **directly** (`DeckBuilderWidget.h:53`), so the complete inheritance
chain is `UUserWidget -> UWidget -> UVisual -> UObject -> UObjectBaseUtility -> UObjectBase`. All six headers were
read out of `C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/`:

- `UMG/Public/Blueprint/UserWidget.h`
- `UMG/Public/Components/Widget.h`
- `UMG/Public/Components/Visual.h`
- `CoreUObject/Public/UObject/Object.h`
- `CoreUObject/Public/UObject/UObjectBaseUtility.h`
- `CoreUObject/Public/UObject/UObjectBase.h`

The subject set is every identifier this row *introduces* into a scope where shadowing is possible — locals,
parameters, loop variables, the local struct's members, and the one new class member — recovered mechanically from
`git diff 40c824b` rather than from memory: `PC` · `InputComponent` · `Bindings` · `Binding` · `Action` ·
`BoundNames` · `MissingNames` · `MissingClause` · `Handle` · `Key` · `ActionName` · `bGridWasLive` · `MenuInput` ·
`World` · `InKeyEvent` · `MenuNavBindingHandles` · `AssetPath` · `Handler`.

**Two independent methods, run against those six headers:**

1. **Forward** — a declaration-shaped regex per candidate name (covering `T* Name;`, `TObjectPtr<class T> Name;`,
   `T Name = ...`, arrays, and `uint8 bName : 1` bitfields).
2. **Inverted** — extract *all* member-like declarations from the six headers into a list (**242 names**), then
   intersect with the 18 subject names. This one does not depend on my per-name regex being right.

**Both returned exactly one hit: `InputComponent`, `UserWidget.h:1764`** — the site already fixed.

=> **There is no site 3.** That conclusion does not rest on the short 22-of-25 build; it rests on the header
enumeration, which does not care which files the compiler reached.

`EnhancedInputComp` was checked against the same 242-name list (absent) and against the whole project (absent),
so the replacement cannot collide either.

## L1.4 — Why the name looked safe, stated so it is not re-introduced

`SiegeMenuInputSubsystem.cpp:121` holds the **byte-identical** local `UEnhancedInputComponent* InputComponent =
Cast<UEnhancedInputComponent>(PC->InputComponent);` and compiles clean. `USiegeMenuInputSubsystem` is a
`UWorldSubsystem` and has no `InputComponent` member. The idiom was copied from a site where it is correct into a
base class where it is fatal — the collision is a property of the *destination class*, not of the line.
⚠️ **That file is read-only to me this loop (`TASK-1469` owns it) and was not edited.** It does not need editing:
the name is correct there.

## L1.5 — Corrected diff shape

Re-measured rather than repeated:

| File | vs `40c824b` | vs the pre-fix working tree |
|---|---|---|
| `DeckBuilderWidget.cpp` | **`+392 / −2`** (unchanged) | **−6 / +6** (6 lines rewritten in place) |
| `DeckBuilderWidget.h` | **`+192 / −0`** (unchanged) | untouched this loop |

The baseline numbers are **unchanged, and that is the right answer, not a stale one**: all six renamed lines were
themselves *added* lines of this row, so rewriting them changes added content without moving either count. The
`−N/+N` shape applies against the pre-fix working tree. Total file length is unchanged at 3110 lines.

## L1.6 — Fences honoured

Written: `DeckBuilderWidget.cpp` (6 lines) · this row's `status:` · this appended section. Nothing else.
Not touched: `SiegeMenuInputSubsystem.{cpp,h}` · `AccountMenuWidget.*` · `SessionMenuWidget.*` ·
`SiegeGraphicsMenuWidget.*` · `SettingsMenuWidget.*` · `SiegePlayerController.cpp` · `CONVENTIONS.md` · any
`.uasset`. No compile, no PIE, no MCP call, no editor lifecycle action (editor left up, PID 32196), no git,
no commit, nothing staged. E1 and E2 left exactly as submitted — the manager owns both.

## L1.7 — What QA should scrutinise on THIS loop

1. That `PC->InputComponent` survived at 1901 and 2001 (it must).
2. That the six sites are the *complete* set — `grep -nE "(^|[^>a-zA-Z_])InputComponent"` over the .cpp now
   returns **zero** bare identifiers.
3. Whether the sweep's subject set missed an introduced name. It was recovered from the diff mechanically, but a
   reviewer re-deriving it independently is the honest check.
4. **Unchanged from the original submission:** the runtime behaviour of this row is untested by this loop. A
   rename cannot change it, but it also cannot demonstrate it — 5b still owes the reading, and it still needs
   `Log LogGitClaudeUnrealTest Verbose` typed first or the per-press instrument prints nothing.
