# TASK-1471 — [MENU-NAV-SELF-DRIVING-SCREEN-FLAG] — programmer handoff

marker `TASK-1471-MENU-NAV-SELF-DRIVING-SCREEN-FLAG`
Row B. Written 2026-09-25 by the gameplay-programmer. Gate: `TASK-1472`.
⛔ NO compile · NO PIE · NO MCP mutation · NO editor lifecycle action · NO git · NO `.uasset` · NO `CONVENTIONS.md`.
⛔ The automation suite is **NOT CITED** (`TASK-1476` passed `TASK-1475` but did not lift the prohibition: 5a has not succeeded on a build containing that file and no run has listed the three new tests as EXECUTED and green).

---

## 0. ⚠️ ONE FENCE DEVIATION, DECLARED FIRST AND NOT BURIED

The dispatch said **"WRITE ONLY `SiegeMenuInputSubsystem.{cpp,h}`"** and listed the six verified screen files as READ-ONLY. **The board row says something different**, in two places:

- spec cl. **(5)**: *"THE DECK BUILDER IS THE ONLY INTENDED CONSUMER TODAY. If setting the flag requires an edit to `DeckBuilderWidget.{cpp,h}`, that is ONE extra file and it is **GRANTED** — but NOTHING ELSE in it moves (no deck rule, no slot count, no auto-save, no right-click path)."*
- `names:`: *"WRITES: `SiegeMenuInputSubsystem.{cpp,h}` · **`DeckBuilderWidget.{cpp,h}`** · `handoffs/TASK-1471-programmer.md` · `TASKBOARD.md`"*

**I took the row's grant, and I took less of it than it offers.** What I wrote:

- `DeckBuilderWidget.cpp` — **ONE statement** changed (`RegisterMenuNavTarget(this)` → `RegisterSelfDrivingMenuNavTarget(this)`) plus its comment. Nothing else in the file moves.
- `DeckBuilderWidget.h` — **NOT TOUCHED AT ALL**, although the row grants it.

**Why the deviation was unavoidable, stated as an argument rather than a preference.** Spec cl. (3) requires the flag to be *"PER-SCREEN, set by the **SCREEN**, read by the SUBSYSTEM."* A flag the screen sets requires the screen to say something, and `UDeckBuilderWidget` is C++. The three alternatives were each worse:

1. **Infer it** (subsystem detects that the screen binds the `IA_Menu*` actions itself) — not a flag, and `TASK-1472`'s stated question is literally *"whether it is really a flag"*. An inference cannot be set, cleared or reviewed.
2. **Hard-code the class name** in the subsystem — violates cl. (3) outright and makes the policy unusable by the next screen.
3. **Ship the flag with no caller** — the runtime criterion would fail, the measured hazard would stand unclosed, and this project has a name for that shape (`SC-§36.1`: *the better the SURFACE, the more complete the ILLUSION* — a built and tested trigger shipped with ZERO callers and nothing failed).

🙋 **If QA or the orchestrator rules the dispatch's narrower fence binding, the revert is one statement** at `DeckBuilderWidget.cpp` (the `MenuInput->RegisterSelfDrivingMenuNavTarget(this);` line inside `UDeckBuilderWidget::RegisterAsMenuNavTarget`), and everything else in this row stands as a subsystem-only change with no consumer.

⚠️ **One documentation debt created by declining the `.h` half of the grant, named rather than hidden.** `DeckBuilderWidget.h`, in `RegisterAsMenuNavTarget`'s declaration comment, says *"registration turns the subsystem's generic ring ON over a screen that drives itself. RouteMenuNavKey's repair above is what makes that composition converge on the grid every press"*. After this row that sentence is **stale** — registration no longer turns the generic ring on for this screen. The sentence beside it (*"the clean fix is subsystem-side and is ESCALATED in the handoff rather than taken silently"*) remains **accurate as history**: it describes what `TASK-1423` did, and this row is that escalation landing. I left both byte-identical rather than widen the footprint. ⇒ **one-line doc amendment owed to a future row** (or grant me the `.h` on a bounce and I will make it in the same breath).

---

## 1. ⛔ THE (2) ANSWER FIRST, AS THE ROW ORDERS IT: **CLOSES. NOT NARROWS.**

The hazard: `qa/TASK-1424.md` WARN-1, promoted to a live PIE reading by `qa/TASK-1427-verify.md` — registration puts `IA_MenuAccept` within reach of the builder's own *"Reset to Default"* button, **which wipes 🧑 Jonathan's deck and auto-saves the wipe**, and every `IA_MenuDown` was measured moving the ring onto that button *within the press*.

**It is closed, and the closure is a property of the control flow, not of where the ring is standing.**

> `USiegeMenuInputSubsystem` contains exactly **two** statements that can activate a control on the active nav target:
> `SetCheckBoxChecked(CheckBox, ...)` and `Focused->OnClicked.Broadcast()` — and inside `StepFocusedStop`, `Member->OnClicked.Broadcast()` for a stepper pair.
> **All of them are below a `return`** that is taken whenever `GetSelfDrivingActiveTarget()` is non-null.

⇒ with the flag set there is **no path through `HandleMenuAccept` or `StepFocusedStop` that presses ANY button on the declared screen, from ANY focus state, cold or not** — not just no path from a tile-focused state. And the *transit* is gone too, because `MoveFocus` returns before it reads a single stop.

**Acceptance stated as a PROPERTY, never an index** (⭐ *a gate written in indices has an expiry date nobody printed on it*):

> **P1.** While the active nav target has declared self-driving, `USiegeMenuInputSubsystem` performs **no `SetUserFocus` request on any widget in that target's tree** and **broadcasts no `OnClicked` and no `OnCheckStateChanged` on any widget in that target's tree**, for any `IA_Menu*` action, from any focus state.
> **P2.** While the same is true, `GetActiveNavTarget()` still returns that screen and `GetMenuFocusStops()` still returns its full stop list, so `LogNavTargetRetarget` still prints its focus-stop count.
> **P3.** With `SelfDrivingScreens` empty (every build in which no screen declares), every code path is the one `TASK-1406` / `TASK-1409` / `TASK-1469` / `TASK-1429` shipped.

⛔ **Anchoring.** The hazardous stop is anchored by **object path**, as the verifier recorded it: `WBP_DeckBuilder_C_0 → Overlay_19/VerticalBox_0/HorizontalBox_1/Button_1` = *"Reset to Default"*. ⛔ No claim in this handoff rests on a bare widget name (a 17/17 census found `Btn_Jump` in nine design-time trees and `BackButton` in three classes). Where I could not anchor, I write **UNANCHORED** — see §7.

---

## 2. ⛔ THE (3) STOPS / KEEPS SPLIT

### STOPS — exactly four things, and every one is "the generic ring touching this screen's controls"

| # | Site | What stops |
|---|------|-----------|
| 1 | `MoveFocus(±1)` | Up / Down move **no** focus on the declared screen's tree. ⛔ **This is the measured transit.** |
| 2 | `HandleMenuAccept()` | **No** `OnClicked.Broadcast()`, **no** check-box toggle. ⛔ **This is the *"Reset to Default"* reach.** |
| 3 | `StepFocusedStop(±1)` | Left / Right mutate **none** of its sliders, check boxes or stepper pairs. |
| 4 | `FocusFirstNavStop()` | Registration places **no** ring on its stop 0 — and neither does `UnregisterMenuNavTarget`'s hand-back, because the gate is **inside** the function, not at its two call sites. |

Each refusal emits **one** `Log` line naming the screen and its class. ⛔ **No existing log string is reworded** — `TASK-1395` / `TASK-1402` read four of them as grep discriminators, and all four are byte-identical.

### KEEPS — and this half is the point: the screen stays **ENUMERABLE**

- **It stays registered.** `NavTargetStack` is untouched by the flag, so `GetActiveNavTarget()` still names it and registrations still nest (a panel opened *on top of* a declared screen is an ordinary undeclared target and the ring drives it normally).
- **`GetMenuFocusStops()` and `IsNavFocusStop()` are NOT TOUCHED** ⇒ `LogNavTargetRetarget` still prints **this screen's focus-stop count**. That is the binding instrument of `EVENTGRAPH-CENSUS-GAP-RULED-2026-09-24` and the only way the deck builder gets a focus-stop-count line on any lane. ⛔ **I did not "fix" this by un-registering the screen** — the dispatch forbade it and it would have deleted the measurement instead of the defect.
- **`IsNavTargetActionable()` unchanged** — the registration still ANSWERS the coverage question for whatever is underneath.
- **`IA_MenuBack` unchanged.** It drives nothing and activates nothing: it *asks* the active target to close itself through `ISiegeMenuNavCloseTarget`, which is already the screen's own decision. Gating it would have suppressed the one diagnostic line `TASK-1454` needs. The deck builder implements no close target, so Back stays INERT for it and logs exactly the sentence it logged before.
- 🚨 **The in-match arm is unchanged, and that is load-bearing rather than incidental.** `IsInMatchScreenOpen()` is still `GetRegisteredNavTarget() != nullptr`; a declared screen is still registered ⇒ it still arms and disarms `IMC_MainMenu` on its two edges. **A self-driving screen listens to the SAME six `IA_Menu*` assets on the SAME input component**, so a flag that suppressed the arm would have killed that screen's own keys off the menu map.
- **Row A's three limbs are untouched**: the two-pass focus read in `GetFocusedNavStop()`, `HasVisibleSlateAncestry`, and the stepper-`Next` refusal in `IsNavFocusStop`. I re-opened none of them.
- **`InMatchMenuMappingContextPriority` is untouched** (⚖️ `0 → 2` is 🧑 Jonathan's unresolved call; I neither pre-empted nor "tidied" it).
- **`TASK-1474`'s widget-tree descent is not implemented.**

---

## 3. ⛔ THE (4) FIVE-SCREENS-UNAFFECTED STATEMENT — AND THE SIX-SCREEN STOP TABLE

**Default is OFF and the default is the whole regression contract.** `SelfDrivingScreens` is empty until a screen calls the new entry point; `GetSelfDrivingActiveTarget()` returns `nullptr` on an empty array; every gate is `if (nullptr) → fall through`.

⭐ **Measured, not asserted:** a census of the new symbol across `Source/` finds `RegisterSelfDrivingMenuNavTarget` called from **exactly one** site — `DeckBuilderWidget.cpp`, inside `UDeckBuilderWidget::RegisterAsMenuNavTarget`. Settings, Graphics, Login (`UAccountMenuWidget`), Session and Controls Help all still call the unchanged `RegisterMenuNavTarget`.

### The stop table — per-binary, attributed

| Screen | Baseline (verified) | After Row B | Attribution |
|---|---|---|---|
| Main menu (`WBP_MainMenu`) | **7** | **7** | unchanged — never registers a declaration; it is the DEFAULT target and `GetSelfDrivingActiveTarget()` reads `GetRegisteredNavTarget()`, so it can never be gated by somebody else's flag |
| Settings | **3** | **3** | unchanged — calls `RegisterMenuNavTarget` |
| Graphics | **19** (**21** with a confirm pending) | **19** / **21** | unchanged — calls `RegisterMenuNavTarget` |
| Login (`UAccountMenuWidget`, per mode) | **3 / 5 / 4 / 5 / 2–3** | **3 / 5 / 4 / 5 / 2–3** | unchanged — calls `RegisterMenuNavTarget`; its `ApplyMode` re-registration path is untouched |
| **Deck builder** | **4** | **4** | ⭐ **unchanged, and that is the deliverable.** The flag changes what the subsystem DOES with the stops, never which widgets ARE stops |
| Session | **4** | **4** | unchanged — calls `RegisterMenuNavTarget` |

**Why every count is identical, in one sentence:** the count is produced by `GetMenuFocusStops()` → `IsNavFocusStop()` → `HasVisibleSlateAncestry()`, and **this row edits none of those three functions**. ⇒ if a count moves after this row, it is not this row.

⚠️ **Controls Help (`USiegeControlsHelpWidget`)** is a seventh registered screen with no baseline count in this table; it is being written right now by `TASK-1432` and I touched neither of its files. It calls the plain `RegisterMenuNavTarget`, so the flag does not reach it.

---

## 4. ⛔ WHAT THE VERIFIED DECK-BUILDER READINGS DO AFTER THIS CHANGE — INCLUDING THE ONE THAT MOVES

`TASK-1423` is `qa-passed` **and runtime-`VERIFIED`**. The row required me to say which of its readings my flag could plausibly break and why it does not. Taken one at a time:

**R1 — `FocusedCardIndex` −1 → 0 on one injected `IA_MenuDown`. ✅ HOLDS.**
Could the flag break it? The plausible mechanism is that `TASK-1423`'s repair inside `RouteMenuNavKey` is somehow *load-bearing* for the arm. It is not: the repair branch is `if (FocusedCardIndex != INDEX_NONE && !bGridWasLive)`, so at `FocusedCardIndex == INDEX_NONE` it is **not entered at all** — the arm is `HandleCardGridKey`'s decision alone, and that function is untouched. What changes is that the competing focus setter is gone, so the arm now happens on a tree nothing else moved. ⛔ The repair is **not removed** (it is not mine to remove, and it still covers any other focus stealer); it simply stops being exercised on the common path.

**R2 — the 2-D ring stepping `0 → Right → 1 → Down → 14 → Up → 1` (row width 13). ✅ HOLDS.**
Could the flag break it? The plausible mechanism is that the subsystem's refusal also refuses the *screen's* handler. It cannot: those are **two separate bindings on the same `UInputAction`**, added to the same `UEnhancedInputComponent` by two different objects. `DeclineIfActiveTargetSelfDriving` returns from `USiegeMenuInputSubsystem`'s delegate only; `UDeckBuilderWidget::HandleMenuNavRight` / `…Down` / `…Up` → `RouteMenuNavKey` → `HandleCardGridKey` is a different delegate in the same array and still runs. ⭐ Every step in R2 is produced by the builder's own handler, and the change makes it **more** reliable, because `bGridWasLive` is now true on entry instead of having to be repaired.

**R3 — the grid→bar crossing via `IA_MenuBack` returning the index to `-1` and focusing `…/DeckBar/DeckSlotEntryWidget_0/OutlineBorder/SlotButton` ("deck1"). ✅ HOLDS.**
Could the flag break it? Two plausible mechanisms, both refuted. (a) *The flag gates Back.* It does not — `HandleMenuBack` is byte-identical and still logs the "implements no `ISiegeMenuNavCloseTarget` — Back is INERT for it" line for this screen. (b) *The ring re-takes the focus the builder just placed on `SlotButton`.* It cannot, for two independent reasons: the ring no longer moves on this screen at all, and `SlotButton` lives inside `DeckSlotEntryWidget_0`, a **nested `UUserWidget`**, whose own `WidgetTree` `UWidgetTree::ForEachWidget` never descends into — so it was never a subsystem focus stop in the first place.

**R4 (not one of the three, but it is a reading and it MOVES — declared, not discovered).**
`qa/TASK-1427-verify.md` recorded exactly one `StepFocusedStop(+1) declined: no focus stop holds focus` line in the deck builder (an `IA_MenuRight` with a card tile focused) and called it *correct*. After this row **that press logs the new self-driving line instead**. ⛔ The behaviour is the same in both cases (the subsystem does nothing), the builder's own Right step is unaffected (R2), and no existing string was reworded — a new string appears and an old one stops appearing **on this screen only**.

**R5 — the end-of-construct focus state and `TASK-1307`'s `IDENTITY=MATCH` post-flush read-back. ✅ HOLDS, and this is the subtle one.**
Gate 4 removes the `FocusFirstNavStop()` that `RegisterMenuNavTarget` performs. On the deck builder that call was **already being overwritten**: `UDeckBuilderWidget::NativeConstruct` runs `BindMenuNavActions(); RegisterAsMenuNavTarget(); AcquireBuilderFocus();` in that order, and its own comment states the ordering is load-bearing precisely because `AcquireBuilderFocus()` immediately re-takes focus onto the widget itself with `EFocusCause::SetDirectly`, naming `IDENTITY=MATCH` as the falsifier. ⇒ the state that read-back samples is the one `AcquireBuilderFocus` leaves, and `AcquireBuilderFocus` is untouched. The subtraction is a focus request that was discarded a line later.

**R6 — the registration/count line. ✅ HOLDS, deliberately.** `LogNavTargetRetarget` still runs inside `RegisterMenuNavTarget`, which `RegisterSelfDrivingMenuNavTarget` **calls rather than duplicates**, so the deck builder's `4 focus stop(s)` line still prints.

---

## 5. THE DESIGN, AND WHY IT IS A SEPARATE ENTRY POINT RATHER THAN A PARAMETER

`RegisterSelfDrivingMenuNavTarget(UUserWidget* Screen)` records the mark and then **calls** `RegisterMenuNavTarget(Screen)`. Two mechanical reasons, neither aesthetic:

1. `RegisterMenuNavTarget` is a `UFUNCTION` that WidgetBlueprint graphs call. **Widening its signature moves every existing node's pin set** in assets this row may not touch.
2. The declaration **must** be recorded *before* `RegisterMenuNavTarget`'s closing `FocusFirstNavStop()` runs. A records-then-delegates call guarantees that **by construction**; a "set the flag afterwards" setter would make it an ordering rule nobody enforces, and getting it wrong would paint the focus rectangle on the screen's stop 0 exactly once per open.

**Storage** is `TArray<TWeakObjectPtr<UUserWidget>> SelfDrivingScreens`, declared exactly like its twin `NavTargetStack` (`UPROPERTY(Transient)`, weak). ⛔ **A set of marks beside the stack, not a field inside it** — the same argument the header already makes for `ISiegeMenuNavCloseTarget` over a stored delegate: a per-entry field means turning `NavTargetStack` into an array of a new `USTRUCT`, i.e. rewriting `TASK-1406`'s shipped stack, its weak-pointer contract and all five of its lambdas to carry one bool.

**Lifetime**: the mark is dropped in `UnregisterMenuNavTarget`, and nowhere else, so the flag's lifetime is exactly the registration's and there is no second un-declare call for a teardown to forget. ⚠️ Deliberately **not** cleared by the plain `RegisterMenuNavTarget`: `UAccountMenuWidget::ApplyMode` re-registers on every mode change, and a re-registration is a re-assertion of ownership, not a change of navigation model. 🔍 **QA should scrutinise this choice** — the consequence is that a screen which declared, then re-registered through the plain entry point without unregistering, stays declared. No screen does that today (the deck builder registers once in `NativeConstruct`).

**Predicate**: `GetSelfDrivingActiveTarget()` reads `GetRegisteredNavTarget()`, ⛔ **never `GetActiveNavTarget()`**. So the DEFAULT target can never be gated by somebody else's declaration, and a declared screen stops gating the instant it stops being the top LIVE registration. ⭐ It also makes the gate placement provably harmless: when `GetSelfDrivingActiveTarget()` is non-null, `GetRegisteredNavTarget()` is non-null, so `IsNavTargetActionable()` is necessarily **true** — the new gate can therefore sit **before** that one (where the spec requires it: ahead of the move) **without being able to change which branch any press takes**.

---

## 6. ⛔ DELETIONS — DECLARED, AND ONE CORRECTION TO THE DISPATCH'S NUMBERS

**My own deletions against the committed baseline: ZERO**, and the precise form of that claim matters:

- In **both subsystem files**, every one of my thirteen edits is a **strict insertion** — each `old_string` is a contiguous substring of its `new_string`, so not one shipped line changed. Line counts corroborate it: `.h` 947 → 1114 (**+167**), `.cpp` 1885 → 2064 (**+179**).
- In **`DeckBuilderWidget.cpp`** my edit is a **substitution, not an insertion**: `MenuInput->RegisterMenuNavTarget(this);` → `MenuInput->RegisterSelfDrivingMenuNavTarget(this);`, plus 21 comment lines. ⛔ It shows as `−0` in the table below **only because that statement is not in `40c824b` at all** — `UDeckBuilderWidget::RegisterAsMenuNavTarget` is `TASK-1423`'s, still uncommitted. ⇒ I did substitute exactly one line; it is a line this epic wrote days ago, and the two deletions the file does carry are `TASK-1423`'s own `HandleCardGridKey` signature change.

| File | Cumulative vs `40c824b` **before** this row | **after** this row | mine |
|---|---|---|---|
| `SiegeMenuInputSubsystem.h` | `+681 / −4` | `+848 / −4` | **+167 / −0** |
| `SiegeMenuInputSubsystem.cpp` | `+1522 / −140` (see below) | `+1701 / −140` | **+179 / −0** |
| `DeckBuilderWidget.cpp` | `+393 / −2` | `+415 / −2` | **+22 / −0** |

⚠️ **CORRECTION, MEASURED: the dispatch's cumulative `.cpp` figure of `+1400 / −18` does not match the file.** The header's `+681 / −4` matched **exactly** (848 − 167 = 681, and −4 is unchanged), which is what makes the `.cpp` mismatch worth reporting rather than hand-waving: before my edits the `.cpp` stood at **`+1522 / −140`**, not `+1400 / −18`. I did not cause it and I did not correct it.

⛔ **And the −140 is accounted for, not left as a number.** A line-by-line check of all 140 deleted lines against the current file finds **126 of them still present elsewhere in the file** (moves caused by the earlier queue rows) and **14 genuinely gone**. All 14 belong to `TASK-1406`'s documented refactor and pre-date this row:

- `MoveFocus` switching from `TArray<UButton*> Buttons` to `TArray<UWidget*> Stops` (8 lines: the `Buttons.Num()`, `Buttons.IndexOfByKey`, `Buttons.IsValidIndex`, `FocusButton(Buttons[Next])` forms and their comment);
- `HandleMenuAccept`'s `UButton* Focused = GetFocusedMenuButton();` becoming the `GetFocusedNavStop()` + `Cast` pair (1 line);
- `FocusButton`'s body moving into `FocusWidget` with `SlateButton` renamed `SlateWidget` (5 lines).

⇒ **nothing removed in this row's diff pre-dates this epic**, and nothing was removed by me at all.

---

## 7. ⛔ THE COMPILE TRAP — `error C4458` SWEEP

The trap: a local shadowing an **engine base-class** member, hard under `-WarningsAsErrors`, where a `Source/` grep is a false negative by construction because the collided name is not in this repository.

**Swept against the real base chain, read at the engine source, not assumed.** `USiegeMenuInputSubsystem` → `UWorldSubsystem` → `USubsystem` → `UObject` → `UObjectBaseUtility` → `UObjectBase` (verified: `UWorldSubsystem : public USubsystem` at `Engine/Source/Runtime/Engine/Public/Subsystems/WorldSubsystem.h:16`, `USubsystem : public UObject` at `Subsystem.h` — **no interface and no secondary base** on either).

Every data member on that chain:

| Class | Data members |
|---|---|
| `UWorldSubsystem` | `bHasCalledPostInitialize`, `bHasCalledBeginPlay` (protected bitfields) |
| `USubsystem` | `InternalOwningSubsystem` (private) |
| `UObject` / `UObjectBaseUtility` | *(none)* |
| `UObjectBase` | `ObjectFlags`, `InternalIndex`, `ClassPrivate`, `NamePrivate`, `OuterPrivate` |

⚠️ Two `FName Name;` hits in the engine headers were checked and **discarded**: `UObjectBase.h:411` is inside the nested `struct FNameAndObjectHashIndex` and `Object.h:851` is inside the nested `FAssetRegistryTag`. Neither is a member of any class in the chain.

**Every new identifier this row introduces**, against that list:

- members: `SelfDrivingScreens` — no collision.
- functions: `RegisterSelfDrivingMenuNavTarget`, `GetSelfDrivingActiveTarget`, `DeclineIfActiveTargetSelfDriving` — no collision.
- locals / parameters: `Registered`, `bDeclared`, `SelfDriver`, `Entry`, `Screen`, `Site` — no collision. (`Registered`, `Entry` and `Screen` are names the shipped, compiled body of this file already uses in the same scope shapes.)

Other compile-surface notes for QA:
- `DeclineIfActiveTargetSelfDriving(const FString& Site)` is called twice with `FString::Printf(...)` and twice with a `TEXT("…")` literal (implicit `FString` temporary). Both bind to the `const FString&`.
- The new `UE_LOG` literal contains an em dash. That is **already established in this file's compiled code** — `HandleMenuBack`'s shipped `"… — Back is INERT for it …"` line carries one and is `built`.
- The only new reflection surface is **one `UFUNCTION`** (`RegisterSelfDrivingMenuNavTarget`) and **one `UPROPERTY`** (`SelfDrivingScreens`); `.generated.h` moves by exactly those two.
- ⛔ No new include is needed in either file (`DeckBuilderWidget.cpp` already includes `Siegebound/SiegeMenuInputSubsystem.h`).

---

## 8. FILES TOUCHED

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.h` | class-comment section "THE SELF-DRIVING SCREEN FLAG"; `RegisterSelfDrivingMenuNavTarget` declaration + doc; three added lines on `UnregisterMenuNavTarget`'s doc; `GetSelfDrivingActiveTarget` / `DeclineIfActiveTargetSelfDriving` declarations + docs; `SelfDrivingScreens` member + doc. **+167 / −0** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeMenuInputSubsystem.cpp` | the two helper bodies; `RegisterSelfDrivingMenuNavTarget`; the mark-drop in `UnregisterMenuNavTarget`; four gates (`MoveFocus`, `HandleMenuAccept`, `StepFocusedStop`, `FocusFirstNavStop`); one hygiene line in `Deinitialize`. **+179 / −0** |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | **one statement** in `UDeckBuilderWidget::RegisterAsMenuNavTarget` + its comment. **+22 / −0** |
| `.claude/pipeline/handoffs/TASK-1471-programmer.md` | this file |
| `.claude/pipeline/TASKBOARD.md` | ⛔ **this row's `status:` only** |

⛔ **NOT touched:** `SiegeMenuInputTest.cpp` · `SiegePlayerController.cpp` · `SettingsMenuWidget.{cpp,h}` · `SiegeGraphicsMenuWidget.{cpp,h}` · `AccountMenuWidget.{cpp,h}` · `SessionMenuWidget.{cpp,h}` · `SiegeControlsHelpWidget.{cpp,h}` (being written by `TASK-1432`) · `SiegeAssistantConsoleWidget.{cpp,h}` (under review by `TASK-1435`) · `DeckBuilderWidget.h` · any `.uasset` · `CONVENTIONS.md` · any other row's board line.

---

## 9. RUNTIME CRITERION — what a sitting should read

> *with the flag set, the deck builder's own ring still moves AND `IA_MenuAccept` cannot land on "Reset to Default" from a tile-focused state.*

Rides `TASK-1436` as an additional subject. What the log should show while the deck builder is the active target:

- **present**: `menu nav target registered -> 'WBP_DeckBuilder_C_0' (registered screen), 4 focus stop(s), N screen(s) registered.` — the count line must still be there (P2).
- **present, once per `IA_Menu*` press**: `MoveFocus(+1) declined: the active nav target 'WBP_DeckBuilder_C_0' (DeckBuilderWidget_C) drives its OWN navigation (TASK-1471) …`
- ⛔ **absent**: any `MoveFocus(…): focus moved …` line naming a widget in the builder's tree — in particular the measured `… ('Button_1')` transit.
- ⛔ **absent**: any `IA_MenuAccept -> OnClicked.Broadcast()` line naming a builder button.
- **unchanged**: `IA_MenuBack: active target 'WBP_DeckBuilder_C_0' (…) implements no ISiegeMenuNavCloseTarget — Back is INERT for it …`
- **unchanged**: `FocusedCardIndex` −1 → 0 on one injected `IA_MenuDown`, read by live reflection.

🚨 **THE ABORT CONDITION FROM `TASK-1427` (R1) STILL BINDS on any sitting of this row**: if the `CardTileClass`-unresolvable Warning fires, **STOP** — *"Reset to Default"* wipes 🧑 his deck and auto-saves it. Record `UNOBSERVABLE`, escalate, press nothing. 🧑 His real deck is `deck1`, reading `Deck: 51/50`.

**cl. 3(b) first sentence, for his sitting, verbatim:**
> 🧑 In the Deck Builder, move the outline onto a card and press ⛔ Enter — ⛔ does it pick the card, or does something else happen?
> *(and the parenthesis verbatim)*

---

## 10. ## Not examined / limitations

1. ⛔ **Not compiled, not run, no PIE, no MCP mutation.** Every claim here is source-level or cited from a report. The editor was left exactly as found (PID 18236 up, no PIE).
2. ⛔ **The automation suite is not cited** and no test was added or edited (`SiegeMenuInputTest.cpp` untouched, per the row's fence). The prohibition is unlifted.
3. **The six-screen count table is argued, not re-measured.** The argument is that `GetMenuFocusStops` / `IsNavFocusStop` / `HasVisibleSlateAncestry` are unedited, so no count can move. I did not re-run a census, and the baseline numbers are `TASK-1469`/`TASK-1427`'s, per-binary.
4. **Controls Help has no baseline count in the table** and is being written concurrently by `TASK-1432`; I state only that it does not call the new entry point.
5. **The main menu can in principle be declared self-driving** (`WBP_MainMenu` could call the new `BlueprintCallable` from its graph). Nothing does, and `ApplyInitialFocus()` is deliberately **not** gated — it is `IsMenuUncovered()`-gated and serves the default target. If a future row ever declares the main menu, `ApplyInitialFocus` would still place the ring and that would be an inconsistency worth a line then; it is unreachable today.
6. **The "re-register plain after declaring" case is decided, not measured** (§5). No screen does it today.
7. **UNANCHORED:** the deck builder's stops 0, 2 and 3 are not anchored anywhere I could read — only stop 1 (`Overlay_19/VerticalBox_0/HorizontalBox_1/Button_1`, *"Reset to Default"*) and the excluded `Overlay_19/SizeBox_0/Btn_Jump` carry object paths in `qa/TASK-1427-verify.md`. No claim in this handoff depends on the identity of stops 0, 2 or 3 — only on the count (4) being unchanged.
8. **`FString` allocation on a refused press.** Gates 1 and 3 build a `FString::Printf` site string per press so the log line can carry the direction (this file's doctrine: *a line that cannot tell Up from Down is not an instrument*). It is one small allocation on a press that does nothing else, on a screen that has declared it wants nothing done. Named rather than hidden.
9. ⚖️ **The fence deviation in §0 is a declared decision, not an adjudication.** I picked the board's explicit grant over the dispatch's narrower paraphrase and made the smallest edit that grant allows; reverting it is one statement.
