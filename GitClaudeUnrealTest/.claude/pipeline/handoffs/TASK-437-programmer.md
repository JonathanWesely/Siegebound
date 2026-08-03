# TASK-437 — [SET-2] `USettingsMenuWidget` — the settings screen, CODE-AUTHORED TREE

**Agent:** gameplay-programmer · **Date:** 2026-08-03 · **Status on exit:** `ready-for-qa` (QA gate = TASK-439)

**M8 DECLARATION (verbatim):** adds no replicated property, no new replicated class, no new relevancy tier.
*Why it is true here rather than merely asserted:* this widget only reads and writes `USiegeSettingsSubsystem`, whose single value is **client-local by construction** — it governs a LOCAL review step (whether the player is shown the parsed order before it executes) and never an authoritative outcome. Nothing in these two files is sent to, or read by, the server.

---

## 1. The ruling this widget is built under — and the one it is NOT

✅ **Cited: CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)" §3** — *"THE SETTINGS WIDGET'S TREE IS CODE-AUTHORED. THIS IS A **SECOND, SEPARATE** NAMED EXCEPTION AND IT DOES NOT INHERIT FROM §6 RULING A."* Recorded on the board as **SETTINGS+CONFIRM manager ruling 8**.

⛔ **NOT cited, and deliberately so: "In-match LLM command assistant" §6 ruling A.** That ruling is scoped to `USiegeAssistantConsoleWidget` **only** and says in as many words that citing it for another widget is a misuse. §3 restates the same point from the other side. The two rulings are argued separately on their own facts and neither inherits from the other; a third widget may cite neither. The class comment in `SettingsMenuWidget.h` states this in full so the next reader cannot get it wrong from the code alone.

⛔ **No `.uasset` was authored, duplicated, or reparented.** Nothing was opened in the editor, no MCP call was made, no PIE was run. `/Game/UI/WBP_SettingsMenu` remains **RESERVED and unauthored** (§3 condition (c)) — it is the zero-C++-change fallback and the escape hatch below is what makes it free.

### The five §3 conditions, and where each is satisfied

| Condition | Where |
|---|---|
| **(a) SCOPE** — this class only | Stated in the header class comment; nothing else in this batch was touched. |
| **(b) ESCAPE HATCH BUILT IN** | All 8 children are `UPROPERTY(meta=(BindWidgetOptional))` with the exact pinned names. `ConstructSettingsTree()` returns immediately if `WidgetTree->RootWidget != nullptr` (an asset-authored tree wins WHOLE), and every individual child is constructed **only when that member is still null**. |
| **(c) ASSET NAME RESERVED** | Not authored. Referenced only in comments. |
| **(d) CONTRACT UNCHANGED** | Cloned from the shipped `USessionMenuWidget`: `BindWidgetOptional` members, `BlueprintCallable` wrappers, BIE with **`const FString&` + `bool` only**. No enum, no struct, in any BIE parameter. |
| **(e) VERIFICATION IS A HUMAN PIXEL CHECK** | **I have verified nothing on screen.** See §6 below — that section is TASK-448's checklist. |

---

## 2. Files touched

**New, and the only two files this task wrote:**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SettingsMenuWidget.h` (264 lines)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SettingsMenuWidget.cpp` (532 lines)

**Not touched, by law:** `SiegeSettingsSubsystem.{h,cpp}` / `SiegeSettingsSaveGame.{h,cpp}` (TASK-436's), `GitClaudeUnrealTest.Build.cs` (TASK-443's — **no edit was needed**: `UMG`, `Slate` and `SlateCore` are already public dependencies), any `Content/` asset, any `.ini`, Git. **Nothing was compiled** — the single compile gate is TASK-447 under the quiet-module law.

---

## 3. Structure — what is built, and what it exposes

### The tree (code-authored, in `RebuildWidget()`)

```
BackdropBorder  (UBorder)          <- WidgetTree->RootWidget; ESlateVisibility::Visible
└── RootPanel   (UVerticalBox)
    ├── TitleText              (UTextBlock)  "Settings", font 36, centered
    ├── ConfirmToggleCheckBox  (UCheckBox)   <- content = ConfirmToggleLabelText
    │     └── ConfirmToggleLabelText (UTextBlock) "Confirm AI orders before they execute", font 24
    ├── ConfirmToggleHintText  (UTextBlock)  the hint, font 18, auto-wrap, dimmed
    └── BackButton             (UButton)     <- content = BackLabelText
          └── BackLabelText    (UTextBlock)  "Back", font 28
```

All 8 pinned member names are present character-for-character per §3 / the board's `names:` line.

**One structural decision worth QA's eye:** `ConfirmToggleLabelText` is the **check box's content**, not a sibling in a horizontal box. Reasons: it keeps the tree to the pinned members only (no unpinned layout container is introduced), it makes the words part of the click target, and disabling the check box greys the label with it — which is the "row renders disabled" requirement for free. A future `WBP_SettingsMenu` is free to arrange them any other way; binding is by name, not by position.

`BackButton` content padding is `FMargin(24,12,24,12)` and `BackLabelText` is font 28 — **lifted from the shipped `WBP_MainMenu` button idiom this panel sits on top of**, not invented.

### Public surface (CONVENTIONS §8 pin, character-for-character)

```cpp
UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings") void BackPressed();
UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings") void ConfirmTogglePressed(bool bChecked);
UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Settings") void OnSettingsValueChanged(const FString& SettingName, bool bValue);
```

Plus one non-`UFUNCTION` constant: `static const TCHAR* USettingsMenuWidget::ConfirmSettingName == TEXT("bAssistantConfirmBeforeExecute")` — see §5 for why it exists.

### Player-facing text (game-authored, all of it in one namespace at the top of the .cpp)

- label: `"Confirm AI orders before they execute"` — spec item (3), exact
- hint: `"Shows the parsed order and its target circles for review. Recommended — the assistant can pick the wrong place or the wrong number."` — spec item (3), exact, **em dash preserved**
- title `"Settings"`, back `"Back"`
- one string I authored (there was no pinned text for the failure case): `"Settings are unavailable right now, so AI orders will always be shown for review."` — see §4.

---

## 4. The correctness points, stated so QA can check them by reading

**(a) ⚠️ `BackdropBorder` IS HIT-TEST VISIBLE — the blocker criterion.**
`BackdropBorder->SetVisibility(ESlateVisibility::Visible)`, with a comment saying it is correctness and not styling and naming the trap: TASK-355's `WBP_SessionMenu` backdrop was deliberately `HIT_TEST_INVISIBLE` because it sat over nothing clickable; copying that here ships a live click-through into Play / Sandbox / Deck Builder / **Quit** while the panel looks modal. The owning `UUserWidget` is left at UMG's default `SelfHitTestInvisible`, which means *"I do not hit-test, my children do"* — the border is the child doing the absorbing. **Slate hit-tests on visibility and geometry, not on painted pixels**, so the click blocking does not depend on the brush drawing anything; the dim brush colour is appearance only. **The click-through itself is a runtime behaviour I cannot verify without PIE — it is item 2 of the pixel checklist.**

**(b) SEED-THEN-BIND, in that order.** `SeedAndBind()` removes the check-box delegate, reads `IsAssistantConfirmEnabled()`, pushes it to the row, *then* adds the delegate. A bind-only control would show every player "off" on a setting whose shipped default is `true` — the qa/TASK-005 major-2 lesson. Verified from engine source that `UCheckBox::SetIsChecked` does **not** broadcast `OnCheckStateChanged` (only `SlateOnCheckStateChangedCallback`, i.e. real user interaction, does — `UMG/Private/Components/CheckBox.cpp:188` vs `:267`), so there is no feedback loop; the remove/re-add is belt-and-braces in case that ever changes.

**(c) BINDING HAPPENS IN `NativeConstruct()`, NOT `NativeOnInitialized()` — and this is mechanical, not stylistic.** Engine order (`UMG/Private/UserWidget.cpp`) is `Initialize()` → `NativeOnInitialized()` → `RebuildWidget()` → `OnWidgetRebuilt()` → `NativeConstruct()`. The code-authored children **do not exist** during `NativeOnInitialized()`. Binding there would silently bind nothing on the code-authored path while working perfectly on a future WBP path — a difference that would only ever surface as a dead check box nobody could reproduce. One binding site covers both paths; `AddUniqueDynamic` keeps repeated construct cycles single-bound; `NativeDestruct()` unbinds symmetrically.

**(d) `BackPressed()` calls `RemoveFromParent()` and nothing else** (plus one log line). It does **not** re-create or re-open the main menu. `RemoveFromParent()` is the current API — `RemoveFromViewport` is `UE_DEPRECATED(5.1)` and is not used.

**(e) NULL-SAFE THROUGHOUT, and the failure state is honest.** `ResolveSettingsSubsystem()` is null-safe at every hop (`GetWorld()` → `GetGameInstance()` → `GetSubsystem<>`). When it returns null, `ShowRowUnavailable()` disables the check box, replaces the hint with the unavailable string, and logs **once per widget instance** (a `bLoggedSubsystemUnavailable` latch — this path is reachable from the seed, from a click and from a broadcast, and log spam on a dead control is how a log stops being read). It is reached from all three. **`BackButton` is bound unconditionally and last, so the panel is never a trap you cannot leave even when the settings row is dead.**
The disabled check box is shown **CHECKED**, on purpose: TASK-443 spec item (4) fails safe by treating an unresolvable subsystem as confirm = **ON** (*"failing safe means MORE review, never less"*), so a checked-and-disabled row shows what the game will actually do. The unavailable string says exactly that. ⚠️ **This is a cross-task consistency claim about a file that does not exist yet — QA should confirm it against TASK-443 when that lands.**

**(f) NO DISK READ ON ANY PATH HERE.** Only the in-memory getter `IsAssistantConfirmEnabled()` is called; `SetAssistantConfirmEnabled()` owns the write *and* the save. No `LoadGameFromSlot`, no `SaveGameToSlot`, no `.ini` anywhere in these files.

**(g) THE BIE DOES NOT FIRE ON A NO-OP.** `ApplyConfirmValueToRow` fires `OnSettingsValueChanged` only when the pushed value is new for this instance (the first seed always counts as new, so a future WBP never opens stale). `ShowRowUnavailable()` clears the "already pushed" latch, because the forced checked state is not an observed setting value.

---

## 5. Things QA should scrutinise — I am flagging these rather than burying them

1. ⚠️ **SPEC WORDING vs. ENGINE MECHANICS — the one place I did not follow the spec's literal text, called out on purpose.**
   Spec item (1) says *"construct each child ONLY IF that member is still null **after `Super::RebuildWidget()`**"*. I construct the tree **BEFORE** `Super::RebuildWidget()`.
   The **semantic** half — only ever construct a child that is still null — is honoured exactly. The **ordering** half is not followable: `UUserWidget::RebuildWidget()` reads `WidgetTree->RootWidget` *as it stands at the moment it is called* and returns an `SSpacer` when it is null (verified in `UMG/Private/UserWidget.cpp`, UE 5.8), so constructing afterwards and returning Super's result yields a **silently EMPTY widget that still passes every property readback** — exactly the defect class this project's UMG verification law exists for. This is not my inference: it is the **headline finding TASK-411's probe recorded as a deliberate rehearsal of the code-authored-tree idiom** (`SiegeAssistantInputProbe.cpp:396-407`), and I follow that precedent. Separately, "still null" is fully determined before `Super` runs anyway, because `BindWidgetOptional` members are resolved in `UUserWidget::Initialize()`, which has already completed. **If QA reads the spec literally, this will look like a deviation. It is a deliberate one, and reversing it would break the widget.**
2. ⚠️ **`SiegeAssistantInputProbe.{h,cpp}` is the precedent I followed, and TASK-444 DELETES it.** I did not `#include` it, reference it, or depend on it in any way — only the lesson travelled. Nothing in my files breaks when it is deleted.
3. ⚠️ **Cross-task assumptions I could not verify because TASK-436 does not exist yet** (this file **will not compile alone and is not expected to** — the TASK-416/417 precedent; ⛔ do not open a QA loop over it):
   - `#include "SiegeSettingsSubsystem.h"` — a same-directory include. If TASK-436 places the file anywhere other than `Source/GitClaudeUnrealTest/Siegebound/`, this include breaks. Every other game class lives there and CONVENTIONS §2 says "game", so this is the expected location.
   - `LogSiegeSettings` is used but not declared by me — CONVENTIONS §2 says it is declared/defined in `SiegeSettingsSubsystem.h/.cpp`, and §8 pins `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeSettings, Log, All);`. I use `Log`, `Warning`, `Error` and one `Verbose` line.
   - `OnSettingsChanged` is bound with `AddUniqueDynamic` against the pinned `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSiegeSettingsChanged, FName, SettingName)`; my handler is `void HandleSettingsChanged(FName SettingName)`.
4. ⚠️ **I do NOT filter on the broadcast's `SettingName`, deliberately.** The token `USiegeSettingsSubsystem::OnSettingsChanged` broadcasts is **TASK-436's to choose and is not pinned anywhere**, so comparing against a guessed string would be an unpinned cross-task assumption that fails silently. Instead the handler re-reads the live value on **any** broadcast (one in-memory read) and the no-op suppression in (g) stops a broadcast for some future *other* setting from producing a spurious event. The BIE reports `ConfirmSettingName` — the **pinned CONVENTIONS §2/§8 field name** `bAssistantConfirmBeforeExecute` — which is a name I own rather than one I guessed.
5. **Every UMG API used was checked against the installed UE 5.8 headers**, not from memory: `UBorder::SetBrushColor/SetPadding/SetHorizontal|VerticalAlignment/SetContent`, `UCheckBox::SetIsChecked/SetContent/OnCheckStateChanged`, `UButton::SetContent`→`UButtonSlot`, `UVerticalBox::AddChildToVerticalBox`→`UVerticalBoxSlot`, `UTextBlock::SetText/SetFontSize/SetAutoWrapText/SetColorAndOpacity(FSlateColor)`, `UWidget::SetVisibility/SetIsEnabled/RemoveFromParent`, `UWidgetTree::ConstructWidget`. **None is deprecated.** ⚠️ Note `UCheckBox`'s content slot is a plain `UPanelSlot` (`UContentWidget::GetSlotClass()`), *not* a padding/alignment slot — so nothing is cast off `SetContent` there; only `UButton`'s slot is cast (`UButtonSlot`).
6. **Non-ASCII in a `TEXT()` literal:** the hint keeps the spec's em dash. This is an established, compiling pattern in this exact module (dozens of `TEXT("… — …")` literals across `Barracks.cpp`, `BattlefieldScatter.cpp`, `AncientGround.cpp`, all BOM-less and green), so it is precedent, not a new risk.
7. **Styling is minimal and is not mine to decide.** Font sizes, the 0.75-alpha black plate, paddings and the dimmed hint colour are functional placeholders chosen to be legible and to match the shipped main-menu idiom. `RootPanel` has **no panel background of its own** — the centred column sits directly on the dim plate. If the art direction wants a framed card, that is a `WBP_SettingsMenu` (zero C++ change) or an art task, not an edit here.

**What I did NOT run:** no compile, no UBT, no UHT, no editor, no MCP, no PIE, no Git. **No parser has seen these files.** Reviewing by reading is legitimate and incomplete, and this is the incomplete half stated plainly.

---

## 6. ⚠️ WHAT TASK-438 NEEDS TO WIRE THE BUTTON

TASK-438 (art-director, inside TASK-447's session, **after** the compile) adds `Btn_Settings` to `/Game/UI/WBP_MainMenu`:

- **Pick the C++ class `USettingsMenuWidget` in the `CreateWidget` node's class field.** There is **no `WBP_SettingsMenu` asset and there must not be one.** If the class does not appear in the picker, **the compile did not land — stop and report; do not author a WidgetBlueprint to work around it.**
- `CreateWidget(USettingsMenuWidget)` → **`AddToViewport(ZOrder 10)`**. ⛔ **Do NOT `RemoveFromParent` the main menu** — the panel sits on top and `BackPressed()` removes only itself. Removing the menu would leave the player on a black screen after Back.
- **Nothing else is required.** No bindings, no event graph work, no properties to set on the created widget: the tree, the seed, the delegates and the Back behaviour are all self-contained. `BackPressed()` / `ConfirmTogglePressed(bool)` are `BlueprintCallable` if ever needed, and `OnSettingsValueChanged(FString, bool)` is available as a BIE for a future WBP — none of them need wiring for the screen to work.
- Order: **Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Settings → Quit**, spliced before the Quit block.

---

## 7. ⚠️ PIXEL CHECKLIST FOR JONATHAN (TASK-448) — UI RENDERING CANNOT BE SELF-VERIFIED

**I have not seen this screen. There is no `.uasset` to read back, and on this project an MCP/property readback has repeatedly passed on visually-broken UMG — TASK-355 read back six controls 6/6 correct while they were stacked in a 165×48 px box in the corner. A binding readback cannot see geometry. Nothing below is reported as verified; every line is a thing to LOOK at.**

1. **It appears at all.** Main menu → Settings shows a dim full-screen plate with a centred column: title "Settings", a check box with its label, the hint sentence, a "Back" button. ⛔ **An empty or invisible panel is the specific failure mode the ordering fix in §5.1 exists to prevent — if the screen is blank, that is the first suspect.**
2. ⛔ **THE CLICK-THROUGH TEST — this is the blocker criterion and only a human can close it.** With the panel open, click **directly over where a main-menu button sits** (Play / Sandbox / Deck Builder / Multiplayer / **Quit**), on the dim area outside the panel column. **Nothing must happen.** If the game starts, or worse **quits**, `BackdropBorder` is not absorbing and it is a blocker.
3. **The check box reflects the real setting on open** — first ever launch should show it **CHECKED** (default `true`). Toggle it off, press Back, re-open Settings: it must still read **off**. Quit the game entirely, relaunch, open Settings: it must **still** read off (that is TASK-436's save round-trip seen through this screen).
4. **The label is clickable**, not just the box — clicking the words toggles it.
5. **Back dismisses only the panel** and leaves the main menu exactly as it was — same buttons, still working, no flicker, no reload.
6. **The hint sentence is fully readable** — it is long and auto-wraps; check it is not clipped, overlapping, or running off the panel at your resolution.
7. **Legibility/layout sanity:** title, row and button are inside the screen, not stacked in a corner, not overlapping. ⚠️ This is the exact failure TASK-355 shipped past a green readback.

---

## 8. Status

- TASK-437 → **`ready-for-qa`** on the board.
- QA gate: **TASK-439** (covers TASK-436 + TASK-437).
- Compile: **TASK-447 only**, once the module is quiesced. This file does not compile until TASK-436 lands, and that is expected.
