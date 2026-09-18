# TASK-1286 — [DECK-BUILDER-KEYBOARD-CARDS] programmer handoff (2026-09-17)

Status flipped: `ready-for-qa`. **Gate: `TASK-1290`** (`qa-reviewer`). **Verify: `playtest-verifier`, acceptance (2), ⛔ BINDING.** **Host: `TASK-1291`** (5a compile → `built`, ⛔ no commit; then the verify leg; then the 5c commit).

> ⚠️ **PROVENANCE — READ FIRST, THE TRAIL IS HONEST BY CONSTRUCTION.** The code and §§1–9 below were written by a **previous `gameplay-programmer` instance that lost power mid-task** and retains no memory of the work. A **second instance recovered the task and did NOT rewrite a line of it** — it re-read the full diff of all three files and **independently re-derived every load-bearing claim from primary sources** (the UE 5.8 headers and engine `.cpp`s on disk, the module `Build.cs`, and the surrounding code), then flipped the board and posted to Slack. **So: §§1–9 are AUTHORED by instance 1; the attestation that they are accurate is instance 2's, and is an AUDIT, not authorship.** What the audit re-measured and CONFIRMED independently, each at the line: `UserWidget.h:1102/1104` (`IsFocusable`/`SetIsFocusable` both live, public, `UMG_API`) · `SObjectWidget.cpp:175-182` (the `SupportsKeyboardFocus` → `NativeSupportsKeyboardFocus` delegation, i.e. §1c's deciding claim) · `Widget.h:668/672/769/788` · `PanelWidget.h:28/36` (both `const`) · `ScrollBox.h:321` · `Geometry.h:540/548` · `SoftObjectPtr.h:1008` (`LoadSynchronous() const`, so the `const` walk compiles) · `LocalPlayer.h:325` · `InputCoreTypes.h:745` (`Virtual_Accept`) · `Slate` **and** `SlateCore` both already `PublicDependencyModuleNames` in `GitClaudeUnrealTest.Build.cs`, so `FSlateApplication` links (`DeckBuilderWidget.cpp` is not even the module's first user) · `EditingDeckIndex = INDEX_NONE` **at its declaration** (`DeckBuilderWidget.h:578`) and `PersistWorkingDeck`'s guard is `EditingDeckIndex < 0 || >= NumFixedDeckSlots` ⇒ **the test's no-write guarantee is mechanically real, confirmed at both ends** · the `.cpp` hunk headers are `@@ -6,0 @@` / `-8,0` / `-9,0` / `-18,0` (includes), `-523,0` (constructor), `-855,0 +870,451` (**a pure insertion AFTER `RemoveCopy`'s closing brace**) ⇒ `AddCopy` (`:792`) and `RemoveCopy` (`:837`) bodies are **diff 0, proven by hunk geometry, not by eye**. The audit found **no defect requiring a code change**; the three findings it did raise are additive documentation, marked **[AUDIT]** below.

⛔ Not compiled, ⛔ no Live Coding, ⛔ nothing staged, ⛔ no Git, ⛔ no PIE. The GUI editor already running (PID 8972, `UnrealEditor.exe` with only the `.uproject` on its command line — **no `-game`**, so not one of his sessions under `SC-§118`) was used **read-only** and left running; I neither started nor closed nor restarted it.

---

## 1. Deliverable (0) — THE MEASUREMENT, THEN THE ROUTE (`SC-§101`)

All of it read live off the running editor with `execute_unreal_python_readonly`, plus an on-disk string census and the UE 5.8 engine source. Dirty census afterwards: `get_dirty_content_packages() = []`, `get_dirty_map_packages() = []`.

### 1a. "Are `WBP_DeckCardTile`'s three buttons `Is Focusable`?"

| what | measured |
|---|---|
| `WBP_DeckCardTile_C:WidgetTree` subobjects | ⛔ **CORRECTED 2026-09-17 (QA WARN-5, `qa/TASK-1290-report.md`) — the original sentence here said "exactly ONE: `SizeBox_0`" and that enumeration was INCOMPLETE.** A `get_asset_meta` WidgetTree read shows **`Overlay_19 > SizeBox_0 > Btn_Jump`, plus two `UI_Thumbstick_C`** — inherited touch-template residue, the same shape `WBP_DeckBuilder` carries (its `Construct` casts `Btn_Jump->GetParent()->GetParent()` to `Overlay` and collapses all three; the tile's `Construct` does the same and carries the same `"…: root Overlay not found"` `PrintString`). ⚠️ **The CONCLUSION this row supports is unaffected and independently confirmed, and it STANDS**: `AddBtn` / `RemoveBtn` / `Btn_CardFace` / `NameText` / `CostText` / `CountText` / `CopyCountText` / `CardArtImage` / `CardArtBorder` are **BP variables built at runtime**, not design-time widgets (`find_object` on each returns `None`), so they exist at `UButton`'s CDO defaults. ⛔ **No line of the diff depends on the subobject count** — `CollectCardTiles` walks the LIVE panel hierarchy and stops at a tile. Corrected in place rather than deleted so an incompletely-enumerated sentence is never quoted forward as measured fact (`SC-§119` cl. 8). |
| how they exist, then | built by the tile's OWN Construct graph at runtime (`AddChildToOverlay`, `K2Node_DynamicCast_AsOverlay`, and the runtime log string `"WBP_DeckCardTile: root Overlay not found"` all present in the package) ⇒ they are created at **`UButton`'s CDO defaults** |
| `UButton` CDO `is_focusable` | **`True`** (live read of the CDO) |
| `bIsFocusable` / `Focus*` / `Navigation` property names in `WBP_DeckCardTile.uasset`, `WBP_DeckBuilder.uasset`, `WBP_MainMenu.uasset` | **NONE in any of the three** — so nothing, at design time or in a graph, unchecks it |
| ⇒ **the answer** | **The three buttons ARE focusable.** Hypothesis (a) on `TASK-1274`'s status ("the tile's three buttons have `Is Focusable` unchecked") is **REFUTED** |
| the tile ROOT `UUserWidget` | `is_focusable = **False**` (its own CDO) — **this** is why the tile is not a focus stop today |
| grid contract | `WBP_DeckBuilder`'s graph: `GetCollectionCardIDs` → loop → `K2Node_CreateWidget(/Game/UI/WBP_DeckCardTile)` → `AddChildToWrapBox` → `Array_Add` into a `CardTiles` BP array ⇒ **grid order == `GetCollectionCardIDs()` order**, and the tiles live in a `WrapBox` inside `GridScroll` (`ScrollBox`) |

### 1b. "Does Slate directional navigation from the deck bar reach a tile at all?"

**⚠️ NOT MEASURED, and I am naming it rather than asserting it.** Deciding it needs PIE, and PIE is `VER-§3`/verifier territory on an editor Jonathan is using. What I measured instead is that **the answer does not change the route**, because neither branch of the row's (0)-A/(0)-B pair can deliver deliverable (1)+(2) on Slate's default navigation alone:

- Slate's `Escape`-rule navigation goes through `FHittestGrid::FindNextFocusableWidget`, which **does not wrap** (deliverable (1) says "wrap at the ends") and would stop on **three focus stops per tile** (`Btn_CardFace`, `AddBtn`, `RemoveBtn`), not "the tile as the focus unit".
- Slate has **no Remove gesture at all** — `SButton` answers only the Accept key — so a pure-Slate route cannot deliver `−` under any measurement.

So the row's route question resolves on a different measured fact, below.

### 1c. ⭐ THE DECIDING MEASUREMENT (engine source, read at the line, not from memory)

```
SObjectWidget::SupportsKeyboardFocus()            UMG/Private/Slate/SObjectWidget.cpp:175-182
    → WidgetObject->NativeSupportsKeyboardFocus()
UUserWidget::NativeSupportsKeyboardFocus()        UMG/Private/UserWidget.cpp:2411-2414
    → return bIsFocusable;                         ← read LIVE, on EVERY call
UUserWidget::SetIsFocusable(bool)                 UMG/Private/UserWidget.cpp:2421-2425
    → bIsFocusable = In; Invalidate(Paint);
```

⇒ **`SetIsFocusable(true)` DOES take effect on an already-constructed tile.** The Slate side never caches the flag — it asks the `UUserWidget` every time it needs to know.

⛔ **`UserWidget.h:1030`'s deprecation note — *"this property is only set at construction and is not modifiable at runtime"* — is TRUE OF `UButton` (which bakes `bIsFocusable` into `SButton` at `RebuildWidget`) and FALSE OF `UUserWidget`.** QA: this is the single claim the whole diff rests on; the three line references above are the proof, and the comment at `FocusCardTile` says "do not fix this into an asset edit" for exactly that reason.

And the outline really paints on a focused tile root:

| fact | source |
|---|---|
| `SWidget::Paint` draws `GetFocusBrush()` when `bCanSupportFocus && SupportsKeyboardFocus()` and the user shows focus | `SlateCore/Private/Widgets/SWidget.cpp:1746-1751` |
| `bCanSupportFocus` defaults **true** | `SWidget.cpp:217` (`: bCanSupportFocus(true)`) |
| `SObjectWidget::Construct` never clears it (it only calls `SetCanTick(false)`) | `UMG/Private/Slate/SObjectWidget.cpp:17-27` |
| `PLATFORM_UI_NEEDS_FOCUS_OUTLINES` = **1** on Windows | `Core/Public/HAL/Platform.h:539-540` (Android/iOS set 0) |
| the brush = `FAppStyle` `"FocusRectangle"` = `"Old/DashedBorder"`, white @ 50 % alpha | `SWidget.cpp:1012-1014`, `CoreStyle.cpp:305` (quoted in `handoffs/TASK-1274-programmer.md` §4, re-checked) |

### 1d. ⇒ ROUTE CHOSEN: **(0)-A, SLATE-NATIVE — and stricter than the row allowed**

⛔ No `IA_DeckAdd` / `IA_DeckRemove`. ⛔ No new `.uasset`. ⛔ **Not even the `WBP_DeckCardTile.uasset` focusable-flag edit the `names:` list permitted** — the tile is made focusable in C++ at focus time. **The whole diff is three C++ files.**

**Why NOT (0)-B, on a measured fact and not on taste:** `handoffs/TASK-1274-programmer.md` §7 measured that `BP_MenuGameMode` applies `FInputModeUIOnly` at boot, whose `ApplyInputMode` calls `UGameViewportClient::SetIgnoreInput(true)`, and `UGameViewportClient::InputKey` returns early on `IgnoreInput()` (`GameViewportClient.cpp:767`). **On `L_MainMenu` — which hosts this builder — a REAL key press never reaches Enhanced Input at all.** An `IA_DeckAdd` would have been reachable only through the verifier's `inject_input_action`: green on the verify row and **inert in Jonathan's hands**, i.e. the opposite of what he asked for. Slate's key route (`SObjectWidget::OnKeyDown`, `SObjectWidget.cpp:231-239`) is untouched by `SetIgnoreInput`, and it is the path his own arrow keys already ran on this very screen (his words, quoted on `TASK-1274`'s status).

---

## 2. The key table (deliverable (2))

| meaning | keyboard | gamepad | goes to |
|---|---|---|---|
| move focus across the tiles (grid order, **ring-wrap both ends**) | `Left` `Right` `Up` `Down` | `Gamepad_DPad_{Left,Right,Up,Down}` · `Gamepad_LeftStick_{Left,Right,Up,Down}` | `MoveCardFocus` → `StepCardFocusIndex` |
| enter the grid from the deck bar | `Down` | `Gamepad_DPad_Down` · `Gamepad_LeftStick_Down` | `SetFocusedCardIndex(0)` |
| **Accept** = what left-clicking `+` does | `Enter` · `Virtual_Accept` | `Gamepad_FaceButton_Bottom` | `AcceptFocusedCard` → **`AddCopy(CardID)`** |
| **Remove** = what left-clicking `−` does | `Delete` | `Gamepad_FaceButton_Right` | `RemoveFocusedCard` → **`RemoveCopy(CardID)`** |

🧑 `Delete` is the spec default: he was asked to name an alternative and named none (*"Yes to TASK 1286."*, 2026-09-17).

**Fences, each machine-checked against the diff (`git diff -- Source/ | grep -o 'EKeys::[A-Za-z_0-9]*' | sort -u`) — 17 literals, listed in full:**
`Delete` · `Down` · `Enter` · `Gamepad_DPad_{Down,Left,Right,Up}` · `Gamepad_FaceButton_Bottom` · `Gamepad_FaceButton_Right` · `Gamepad_LeftStick_{Down,Left,Right,Up}` · `Left` · `Right` · `Up` · `Virtual_Accept`.

- ⛔ **Zero letters, zero digits** ⇒ `KBD-§4`'s remap table is never entered (it tables all 26 letters and *deliberately* excludes digits), and `USiegeKeyboardLayoutSubsystem` has nothing to translate. `git diff -- CONVENTIONS.md | grep -c 'KBD-§'` = **0**.
- ⛔ `Tab` absent (Slate's own).
- ⛔ **No existing key gains a second meaning:** Accept and Remove fire **only** while a card tile actually holds Slate focus (`IsCardGridFocusLive()`), so `Enter` on a deck-bar slot still reaches `SButton::OnKeyDown` → `SelectDeckForEdit` exactly as today, and `Delete` anywhere else is untouched.
- ⛔ `DECK-§3`'s right-click = "make active" gets **no keyboard twin** in this row.
- ⛔ The `+` / `−` button bodies are not touched; `AddCopy` / `RemoveCopy` diff = 0 (they are *called*, never edited), so `OnDeckSlotCountChanged` and the `DECK-§4` auto-save funnel run unchanged.

---

## 3. Files (`git status --porcelain`, quoted; git root is one level up — `SC-§102`)

```
 M GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md                                   ← ⛔ NOT MINE (see below)
 M GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                                     ← the manager's concurrent edits + my ONE status-line flip
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp       (+465)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h         (+222)
 M GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp (+180)
```

Plus this handoff. **No `.uasset` anywhere.** `Content/` is clean; `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` / `Content/Input/` / `WBP_DeckCardTile.uasset` / `WBP_DeckBuilder.uasset` / `DeckSlotEntryWidget.*` / `DeckLibrary.*` / `Saved/SaveGames/` all untouched.

**`CONVENTIONS.md` is the manager's, not mine — verified, not assumed:** its diff is `SC-§119` + the R17 Aura-index amendment; `grep -c 'KBD-§'` = **0** and `grep -c '1286\|FocusedCard\|StepCardFocus'` = **0**. Acceptance (3) is met on both halves.

---

## 4. What the diff actually does

### `DeckBuilderWidget.h` (+222, all in `UDeckBuilderWidget`)
Public: `GetFocusedCardIndex` · `GetFocusedCardID` · `SetFocusedCardIndex` · `MoveCardFocus(EUINavigation)` · `AcceptFocusedCard` · `RemoveFocusedCard` (all `UFUNCTION`), plus the **pure static** `StepCardFocusIndex(Current, Count, Columns, Direction)` (⛔ no `UFUNCTION` — a static cannot be `BlueprintCallable` outside a function library).
Protected: `CardTileClass` (`TSoftClassPtr<UUserWidget>`, `EditDefaultsOnly`, defaulted in the ctor to `/Game/UI/WBP_DeckCardTile.WBP_DeckCardTile_C` — the `Castle.cpp:348` `HPBarWidgetClass` idiom character-for-character) and the `NativeOnKeyDown` override.
Private: `FocusedCardIndex` (`UPROPERTY(Transient)`, `INDEX_NONE` — the `EditingDeckIndex` shape) · `bWarnedMissingTileClass` (`mutable bool`, the `bWarnedMissingTable` spam-guard shape) · `CollectCardTiles` · `FindTileForCard` · `FocusCardTile` · `ResolveGridColumns` · `IsCardGridFocusLive` · `NavigationFromKey`.
Two includes added: `InputCoreTypes.h` (`FKey`) and `Types/SlateEnums.h` (`EUINavigation`, a `UENUM(BlueprintType)` UHT must see).

### `DeckBuilderWidget.cpp` (+465)
- `NativeOnKeyDown`: one `IsCardGridFocusLive()` read, then Accept/Remove (grid-focused only), then the direction table; `Down` from outside the grid arms at tile 0; **everything else returns `Super::NativeOnKeyDown`**.
- `FocusCardTile`: `SetIsFocusable(true)` → **`ScrollWidgetIntoView`** on the nearest `UScrollBox` ancestor → `FSlateApplication::SetUserFocus(..., EFocusCause::Navigation)` with the `LocalPlayer->GetSlateOperations()` deferred fallback (the `USiegeMenuInputSubsystem::FocusButton` idiom, `SiegeMenuInputSubsystem.cpp:374-404`, reused verbatim).
- `CollectCardTiles`: walks the **live panel hierarchy** from `GetRootWidget()` — deliberately not `this->WidgetTree`, because the tiles are graph-created at runtime and are children of a panel without being entries in any design-time tree. Depth-guarded at 32; stops *at* a tile and never descends into one.
- `FindTileForCard`: matches on the tile's **own** `CardID` BP variable, read by reflection (`FindPropertyByName` + `CastField<FNameProperty>` / `FStrProperty` — the `HeroCharacter.cpp:48` precedent), falling back to grid position only when no tile exposes a readable `CardID`. ⛔ Deliberately not an index zip: a zip could light one tile while `Accept` added a different card, which is the confusable-signal class `DECK-§3` forbids.
- `ResolveGridColumns`: row width from the painted tiles' absolute Y, floored at 1. Unpainted / `-nullrhi` ⇒ 1 ⇒ Up/Down degrade to Left/Right. ⛔ A guessed column count here would be an `SC-§101` claim.
- ⛔ **CORRECTED 2026-09-17 (QA NIT-1): "Three includes added" was wrong — the list beside it has always had SIX, and the six-in-four-hunks count is what makes this handoff's hunk arithmetic add up.** **Six** includes added by the first cut (`Components/PanelWidget.h`, `Components/ScrollBox.h`, `Framework/Application/SlateApplication.h`, `Engine/LocalPlayer.h`, `GameFramework/PlayerController.h`, `UObject/UnrealType.h`), **now SEVEN** — the 2026-09-17 amendment adds `Components/Button.h` (`UButton`, for the deck-bar exit target). ⇒ the `.cpp`'s include hunks are now **5**, totalling **7 lines**. Every module they live in (`UMG` / `Slate` / `SlateCore` / `InputCore` / `CoreUObject`) is already in `PublicDependencyModuleNames`; ⛔ `Build.cs` diff = 0.

### ⭐ `DECK-§6`, found while writing this and worth QA's eye
`UScrollBox`'s `ScrollWhenFocusChanges` **defaults to `NoScroll`** (`UMG/Private/Components/ScrollBox.cpp:31`), so `SScrollBox::OnFocusChanging` (`SScrollBox.cpp:1443-1452`) would **not** bring a focused tile into view by itself — a tile below the fold would have worn an outline nobody could see, which is precisely `DECK-§6`'s present-but-unusable defect class. `FocusCardTile` therefore calls `ScrollWidgetIntoView` explicitly. ⛔ Deliberately **not** `SetScrollWhenFocusChanges`, which would rewrite a WBP-owned widget's configuration for every other focus change too.

---

## 5. The test — `Siegebound.Deck.KeyboardFocusedTileAcceptAddsOneCopyAndRemoveTakesItBack`

In the **existing** deck test file `Tests/SiegeDeckSlotsTest.cpp` (+180), `EditorContext | EngineFilter`, suite **+1**. What it asserts is STATE (`SC-§104`), never a call count:

- **(0) the pure ring** — 17 assertions on `StepCardFocusIndex` with no world, no widget tree, no Slate: arming from each of the four cardinals, ±1 on Left/Right, ±`Columns` on Up/Down, wrap off both ends and both rows, the clamp on a nonsense column count, the degradation to 1-D when the width is unmeasurable, and that `Next`/`Previous`/`Invalid` neither move nor arm.
- **(a)** focus **tile K** (deliberately a *middle* tile, `Count/2` — a bug that silently focuses index 0 cannot pass by coincidence) → `AcceptFocusedCard()` ⇒ `GetCountOf(CardK)` **+1** and `GetTotalCount()` **+1**, focus unmoved.
- **(b)** `RemoveFocusedCard()` ⇒ **both back** to their exact pre-values, focus unmoved.
- **(c)** with nothing focused, Accept and Remove are silent no-ops (no count, no total moves).
- **(d)** an out-of-range or negative index **clears** the focus rather than clamping to a neighbour.
- **(e)** `MoveCardFocus` arms on `Down`, walks on `Right`, and wraps to the last tile on `Left` off the first — driven through the public mutator, so the widget's own wiring is covered, not just the static.
- **(f)** the scratch slot does not exist afterwards.

### ⚠️⚠️ QA — READ THIS, IT IS THE ONE PLACE I DEPART FROM THE FILE'S OWN HEADER
`SiegeDeckSlotsTest.cpp`'s header says driving `UDeckBuilderWidget`'s mutators *"would resolve the deck slot through the REAL seam and could write the player's actual guest slot, which no test may do"*. **That warning is about a CONSTRUCTED builder.** This test's widget is `NewObject`'d and **`NativeConstruct` never runs**, so `EditingDeckIndex` stays `INDEX_NONE`, and the ONE auto-save funnel refuses on exactly that index (`PersistWorkingDeck`: *"no editing slot selected yet — mutation NOT auto-saved"*, `DeckBuilderWidget.cpp`) **before anything reaches `SaveDeckAs`** — so the `ACC-§4` seam is never resolved and no slot, real or profile or guest, is ever opened for writing.

That is **mechanical, not conventional**, and it is **asserted three ways rather than assumed**:
1. `TestEqual(... GetEditingDeckIndex(), INDEX_NONE)` as a stated precondition;
2. `AddExpectedMessagePlain(TEXT("PersistWorkingDeck: no editing slot selected yet"), ELogVerbosity::Warning, Contains, /*Occurrences*/ 2)` — **pinned to exactly 2**, one per successful mutation. `Occurrences > 0` fails the test on 0 **or** on 3+ (`AutomationTest.cpp:1808-1865`), so a future edit that lets the funnel through turns this test **RED** instead of quietly writing Jonathan's deck. The (c) no-op pair must not add a third, which is itself part of the assertion;
3. `FDeckScratchGuard` + an explicit `DoesSaveGameExist(ScratchDeckSlotName) == false` at the end.

The one asset the test touches is the shipped `/Game/Data/DT_Cards`, **read** through the widget's own soft pointer — the `SiegeCardRosterTest.cpp:378` / `SiegeCardArtRosterTest.cpp:431` precedent for this lane. Zero network, zero PIE, zero widget tree.

#### ⚠️ **[AUDIT] The spec's `SetSlotNameForAutomationTests` DOES NOT EXIST for the deck lane — measured, and QA must not read its absence as a skipped instruction.**
The row's **Test:** clause says the test must run *"on a test slot (`SetSlotNameForAutomationTests`, ⛔ never the real save)"*. A census of the whole module (`grep -rn "SetSlotNameForAutomationTests" Source/GitClaudeUnrealTest/`) returns that symbol on **exactly two classes, neither of them the deck path**:

| class | declaration |
|---|---|
| `USiegeAccountSubsystem` | `SiegeAccountSubsystem.h:253` (impl `.cpp:409`) |
| `USiegeSettingsSubsystem` | `SiegeSettingsSubsystem.h:230` (impl `.cpp:140`) |

`UDeckBuilderWidget` has **no such seam**, `USiegeDeckSaveGame` has none, and **no test in `SiegeDeckSlotsTest.cpp` has ever called it** (0 hits in the file, before this diff or after). This is not an oversight — the file's own header states the reason as settled law: *"the widget has **no slot-override seam to add without deviating from `DECK-§8`**"* (`DECK-§8` = the pinned cross-task signature registry). **So the spec named a seam that the subject class does not have and may not be given without breaking a different law.** ⇒ the row's *intent* (⛔ never the real save) is honoured by the **stronger** mechanism above — the save funnel is closed at `EditingDeckIndex == INDEX_NONE`, which needs no new API, no `DECK-§8` deviation, and is asserted three ways — while the row's *letter* is unsatisfiable as written. **`SC-§101`: this is declared, not quietly substituted.** If QA rules the letter binding, the remedy is a `DECK-§8` amendment adding the seam, which is a manager/board decision and a different row — ⛔ not something this diff should have done on its own authority.

---

## 6. Acceptance evidence

| # | criterion | evidence |
|---|---|---|
| (1) | the `WBP_DeckCardTile` measurement quoted + the route named | §1 above, in full, with the object names, the CDO reads, the string census and the five engine line references |
| (2) | RUNTIME, verifier-observable | **not mine** — recipe in §7 below |
| (3) | `KBD-§` tables diff 0; no letter/digit key anywhere in the diff | `git diff -- CONVENTIONS.md \| grep -c 'KBD-§'` = **0**; the 17-literal `EKeys::` census in §2, machine-generated from the diff, contains no letter, no digit and no `Tab` |
| (4) | compile `Result: Succeeded`, suite **+1** green | **build-master's**, on its own host row. ⛔ I did not compile |
| (5) | `git status` = only the named files; any `.uasset` named with its hash | §3 — **no `.uasset` at all**, so `SC-§68` has nothing to name |

---

## 7. For the `playtest-verifier` (acceptance (2)) — the recipe, and one honest caveat

1. `TASK-1274`'s recipe opens the builder from `L_MainMenu` PIE (`inject_input_action IA_MenuDown ×2`, then `IA_MenuAccept`; `ui_snapshot` shows `DeckBar` with 10 `deck1..deck10` children).
2. ⚠️ **There is no `IA_` for this feature by design** (§1d). Drive it the way a player does — real key events through Slate: `simulate_key_press` with `Down`, then `Enter`, then `Delete`. `inject_input_action` will **not** exercise this row, and a report that says "no `IA_Deck*` mapping exists" is describing the design, not a defect.
3. Read `TotalText` ("n/50") in `ui_snapshot` **before** and after: Accept ⇒ `n+1`; Remove ⇒ `n` again. **Net zero mutations** — say so explicitly, per the row.
4. 🙋 **The pixel question `DECK-§5`(e) reserves for a human eye, which I cannot answer and did not:** *is the dashed white 50 %-alpha focus rectangle around a whole card tile actually legible over that card's art?* The mechanism is measured (§1c) — that it PAINTS is not in doubt — but whether it READS is a pixel judgement. A still of a focused tile is the cheapest evidence; Jonathan's eye is the acceptance.

## 8. What QA should scrutinise

1. **The one claim everything rests on** — §1c's `SObjectWidget` → `NativeSupportsKeyboardFocus` → `bIsFocusable` chain. Read the three lines. If that chain is wrong, the feature has no visible focus state and the route should have been the asset-flag edit the `names:` list permitted.
2. **`IsCardGridFocusLive()`'s two "return true" fallbacks** (no Slate; no live tile). They are what makes the test drivable, and they mean that in a hypothetical state where Slate is up but the tile lookup fails, `Delete` would act on the model with no outline on screen. I judged that safe (the model index is only ever non-`INDEX_NONE` after a deliberate arm) — say so if you disagree.
3. **`Down` from outside the grid is now consumed by this widget.** On the deck bar that takes nothing away (horizontal row; Slate uses Left/Right there). On the Play / Exit buttons at the bottom it *does* newly mean "jump to card 0". Named deliberately; it is the entry affordance.
4. **There is no keyboard way OUT of the grid** once armed — the row says "wrap at the ends", so Up from the top row wraps to the bottom rather than escaping upward. A mouse click leaves (and `IsCardGridFocusLive` then goes false). If Jonathan wants `Escape`/`Up`-at-the-top to return to the deck bar, that is a follow-up row, not a silent addition here.
5. **`SelectCardForDetails` is NOT touched** — clicking a card face does not sync the keyboard cursor to that card. Deliberate (the row fences the existing bodies); the consequence is that after a mouse click the next `Down` re-arms at tile 0. Flag it if you read the row otherwise.
6. **`NativeOnKeyDown` cost:** every keystroke calls `IsCardGridFocusLive()`, which short-circuits on `FocusedCardIndex == INDEX_NONE` **before** any tree walk — so the walk only happens while the grid is genuinely focused, over ~28 tiles, in a menu. Named so nobody has to re-derive it.
7. **UE 5.8 names checked at the headers, not from memory:** `UUserWidget::SetIsFocusable` / `IsFocusable` (`UserWidget.h:1102-1104`) · `NativeOnKeyDown` (`UserWidget.h:1609`) · `GetRootWidget` const + null-safe (`UserWidget.cpp:1356-1364`) · `UWidget::HasAnyUserFocus` / `HasFocusedDescendants` / `GetCachedWidget` / `GetCachedGeometry` / `GetParent` (`Widget.h:668/672/857/788/769`) · `UPanelWidget::GetChildrenCount` / `GetChildAt` (`PanelWidget.h:28/36`) · `UScrollBox::ScrollWidgetIntoView` (`ScrollBox.h:321`, null-guarded at `ScrollBox.cpp:248-256`) · `EUINavigation` `UENUM(BlueprintType)` incl. `Invalid` (`SlateEnums.h:97-116`) · every `EKeys::` literal (`InputCoreTypes.h:364/375-381/516-533/745`) · `TestEqual(const TCHAR*, const FName, const FName)` and `AddExpectedMessagePlain` (`AutomationTest.h:2003/1796`) · `TSoftClassPtr::LoadSynchronous() const` (`SoftObjectPtr.h:1008`).
8. **Two deliberate non-conversions I would defend rather than "tidy":** `ResolveGridColumns` reads `.Y` straight off the `FDeprecateVector2DResult` instead of constructing an `FVector2D` (that type derives from `FVector2f` *and* offers `operator FVector2d`, so an explicit construction is an ambiguity waiting to happen); and `UE_KINDA_SMALL_NUMBER`, not the deprecated bare macro, matching `FogVolume.cpp:895` and friends.

9. ⚠️ **[AUDIT] The suite does NOT cover the key table itself — acceptance (2) is the only thing that will.** The automation test drives the *mutators* (`SetFocusedCardIndex` / `MoveCardFocus` / `AcceptFocusedCard` / `RemoveFocusedCard`) and the pure ring, but it never drives **`NativeOnKeyDown`**, which is `protected` and unreachable from a test without a subclass. ⇒ **nothing in the suite proves that `Enter` reaches `AddCopy` or that `Delete` reaches `RemoveCopy`** — the key→action wiring, `NavigationFromKey`, and the `IsCardGridFocusLive()` gate are **all** closed only by the `playtest-verifier` leg on `TASK-1291`. A green suite here is therefore **necessary and NOT sufficient**; ⛔ do not read "suite +1 green" as evidence the feature works. Named so the verify leg is not treated as a formality.
10. ⚠️ **[AUDIT] Clearing the focus leaves a residual Slate outline (cosmetic, bounded, not fixed on purpose).** `SetFocusedCardIndex(CardIndex)` calls `FocusCardTile` **only** on the `!= INDEX_NONE` branch, so clearing the model (an out-of-range index, or an empty collection) does **not** take Slate focus off the previously focused tile, nor undo its `SetIsFocusable(true)`. The tile keeps the dashed rectangle while `GetFocusedCardIndex()` reads `INDEX_NONE`, and `IsCardGridFocusLive()` then returns **false**, so the arrow keys fall through to Slate's default navigation *from a tile that still holds focus*. **Reachability is the mitigation:** `StepCardFocusIndex` returns `INDEX_NONE` only when `CardCount <= 0`, so on a populated grid the sole route in is a deliberate `SetFocusedCardIndex(-1)`-style Blueprint call, which nothing in the WBP makes. I judged it a **NIT not worth a speculative fix** in an uncompiled diff; if QA wants it closed, the one-line remedy is to clear Slate focus on the `INDEX_NONE` branch — ⛔ but that is a behaviour change and should be a QA finding, not a silent edit.

M8: adds no replicated property, no new replicated class, no new relevancy tier, no RPC. The focus index is `Transient` and purely cosmetic-per-client.

## 9. What I would have checked had I been allowed to compile (I was not — §0)
- UHT on `MoveCardFocus(EUINavigation)`: that `Types/SlateEnums.h` in the header is enough for the generated code (it is a `UENUM(BlueprintType)` in `SlateCore`, which is a public dependency).
- That the `SiegeDeckCardFocus` namespace's `static` free functions draw no `-Wunused-function` (both are used).
- That the new test's `AddExpectedMessagePlain` count of 2 is exact in the suite lane — if it reports "occurred N times", **N is the new fact to bring back**; ⛔ do not loosen the pin from the build seat, it is load-bearing for the no-write guarantee.

---
---

# ⭐⭐ AMENDMENT — 2026-09-17, ON 🧑 HIS RULING (the `TASK-1299` re-gate's entire subject)

> **Everything above describes the diff `TASK-1290` PASSED. It is not withdrawn.** This section is the **delta only**.
> Scope note for the re-gate: `TASK-1299` reviews **this section**; spec block (H) does not re-open route (0)-A, the
> `+`/`−` entry points, the focus visual, the `DECK-§6` `ScrollWidgetIntoView` fix, or the first test's architecture.
>
> ⛔ **Nothing compiled, ⛔ no Live Coding, ⛔ no Git, ⛔ nothing staged, ⛔ no PIE.** The GUI editor was re-censused by
> command line before any engine call (`SC-§118`): **one** `UnrealEditor.exe`, **PID 8972**, command line
> `"…/UnrealEditor.exe" "…/GitClaudeUnrealTest.uproject"` — plain `.uproject`, ⛔ **no `-game`**. Used **read-only**
> (`unreal_inspector`: one `get_asset_graph` on `WBP_DeckBuilder`). ⛔ Not closed, not quit, not restarted.

> ## ⭐⭐ AMENDMENT 2 — 2026-09-17, THE `AS-§6 A-2` RULING. **`Escape` IS DROPPED; THE GAMEPAD HALF SHIPS INTACT.**
>
> **Read this before any other line of this section: every sentence below that binds `Escape` to the grid exit is
> SUPERSEDED.** §A7 (the flag) is rewritten in place; §A1, §A2, §A8·5 and §A9 are corrected at the affected cells.
>
> **What happened.** The previous instance FLAGGED the `AS-§6 A-2` collision instead of quietly deviating; the
> orchestrator read `A-2` at source and ruled the conservative way while 🧑 is out: **`Escape` is dropped.** `A-2`
> names `Escape` and nothing else, so his headline ruling — *"B stops being destructive and starts meaning Back"* —
> ships in full. ⚠️ Whether `A-2` binds a MAIN-MENU deck builder is a genuine **scope** question and it is **HIS**
> (`CONVENTIONS:2679` cl. 13 — a reviewer's agreement cannot authorize it); it is boarded as **`TASK-1300`** and
> **blocks nothing**. Until he answers, the prohibition is read at its **widest: nothing absorbs `Escape`, anywhere.**
>
> **The delta, in full — one line of code.** `DeckBuilderWidget.cpp` (the exit condition): `Key == EKeys::Escape ||`
> deleted. Nothing else in the diff is re-opened — `ExitCardGridFocus()`, the (C) back-precedence guard, WARN-3's
> fold, the four exit-clause state conditions, both tests, the exit target and route (0)-A are all untouched.
> ⛔ **Escape was dropped BEFORE THIS EVER COMPILED** — no `FReply::Handled()` on `EKeys::Escape` has ever existed in
> a compiled binary of this class, and `A-2` is therefore not breached, only cited.
>
> **⛔ The `A-2` citation STAYS at all three prose sites** (`.h` key-table block · `.cpp` exit block · §A7 below). It
> is the register pointer that caught this; deleting it would delete the only thing that worked.
>
> **Tests: ⛔ zero change, and this is MEASURED, not assumed** — `EKeys::Escape` appears **0** times in
> `Tests/SiegeDeckSlotsTest.cpp` (prose-only mentions at `:1390` and `:1485`); the suite drives `ExitCardGridFocus()`
> directly. Suite stays **baseline +2**.

## A1. What changed, in one table

| action | keyboard | gamepad | status |
|---|---|---|---|
| move focus | `Left`/`Right`/`Up`/`Down` | `Gamepad_DPad_*` · `Gamepad_LeftStick_*` | ⛔ unchanged (ring-wrap; `Down` still enters the grid) |
| Accept → `AddCopy` | `Enter` | **A** `Gamepad_FaceButton_Bottom` + `Virtual_Accept` | ⛔ unchanged |
| Remove → `RemoveCopy` | `Delete` (unchanged) | ⭐ **X** `Gamepad_FaceButton_Left` | ⭐ **CHANGED** — the old **B** binding is **DELETED** |
| ⭐ **Exit the grid** (NEW) | ⛔ **none** — `Escape` DROPPED (amendment 2, `AS-§6 A-2`) | **B** `Gamepad_FaceButton_Right` + `Virtual_Back` | ⭐ **NEW behaviour, gamepad-only** |
| exit (inherited) | `Tab` | — | ⛔ **documented, never implemented** — 0 `EKeys::Tab` |

## A2. ⭐ THE FINAL KEY CENSUS — **19** code literals (⛔ was 20; amendment 2 dropped `Escape`)

Machine census across all three changed files. **17 (QA's count) + `Gamepad_FaceButton_Left` + `Virtual_Back` = 19.**
Every literal exactly once:

`Left`·`Right`·`Up`·`Down` · `Gamepad_DPad_{Left,Right,Up,Down}` · `Gamepad_LeftStick_{Left,Right,Up,Down}` ·
`Enter`·`Virtual_Accept`·`Gamepad_FaceButton_Bottom` · `Delete`·**`Gamepad_FaceButton_Left`** ·
**`Gamepad_FaceButton_Right`**·**`Virtual_Back`**

⚠️⚠️ **`EKeys::Escape` STILL APPEARS — in COMMENT PROSE ONLY, and that is ORDERED, not residue.** It is the `AS-§6`
`A-2` register citation at two of the three prose sites, kept deliberately (amendment 2). **A raw grep therefore
returns a non-zero `Escape` count on a CORRECT diff.** Count code and comments separately, as this handoff does:

**Re-measured 2026-09-17 after the WARN-1 comment fix. ⚠️ Two columns were corrected — see the note under the table.**

| file | `EKeys::Escape` in **code** | `EKeys::Escape` in **comments** | `EKeys::Virtual_Back` in **comments** | lines containing bare `Escape` |
|---|---|---|---|---|
| `DeckBuilderWidget.cpp` | **0** | **3** (`:1508` + `:1512` the `A-2` citation & compliance line, `:1517` the re-add recipe) | **1** (`:1470`, the B-is-Back justification) | 9 |
| `DeckBuilderWidget.h` | **0** | **2** (`:266` + `:270`, the key-table block's `A-2` citation) | **1** (`:242`, same justification) | 11 |
| `Tests/SiegeDeckSlotsTest.cpp` | **0** | **0** | **0** | 4 (was 2 — the WARN-1 fix, below) |
| **total** | ⭐ **code-0** | **comment-5** | **comment-2** | 24 |

⚠️ **NIT-2 CORRECTED — the raw census is `26`, not `24`.** `19` code literals **+ 7** comment hits (the declared
`5 × EKeys::Escape` **plus `2 × EKeys::Virtual_Back`**, at `.cpp:1470` and `.h:242`, both inside the B-is-Back
justification prose). The original table tracked only `Escape` in its comment column, so a mechanical census
expecting "19 + 5 = 24" came up **two over on a CORRECT diff** — the exact class of confusion §A2 exists to prevent.
Measured now, per file: `.cpp` **23** · `.h` **3** · `Tests/SiegeDeckSlotsTest.cpp` **0** ⇒ **26**. ⚠️ Also corrected
in passing: `.cpp`'s bare-`Escape` line count is **9**, not the 10 first written (re-measured with `grep -c`).

⇒ ⛔ **`EKeys::Escape` reachable from any `FReply::Handled()` = 0 sites.** The whole-project count is unchanged from
before this feature existed. Verified with the same instrument that prints the 19 above (`EKeys::` tokens on
comment-stripped lines: 19 distinct, each exactly once, `Escape` absent).

### ✅ **WARN-1 — CLOSED 2026-09-17. The two stale test-file sentences are rewritten (comment-only).**

**The residual I declared rather than quietly fixed** (`Tests/SiegeDeckSlotsTest.cpp` asserting the DROPPED binding as
live fact) **was routed back to me as a prescribed remedy, with the fence lifted by the orchestrator** — the fence was
the brief's, not the board's, and the row's `names:` WRITE list contains this file. QA recorded the original refusal
to edit a fenced file on my own authority as **correct conduct**; this pass has the authority the first one lacked.

**The two sentences, before → after:**

| site | ⛔ was (FALSE since the Escape drop) | ✅ now |
|---|---|---|
| `:1389-1390` | "gamepad B stops deleting cards and starts meaning Back, **alongside Escape**" | "gamepad B stops deleting cards and starts meaning Back" + a new ⛔⛔ **GAMEPAD-ONLY** block: `Gamepad_FaceButton_Right` / `Virtual_Back` and NOTHING ELSE; the Escape key was **DROPPED under `AS-§6 A-2`** and is permanently unabsorbed project-wide |
| `:1485` | "**B/Escape** once leaves the grid, again leaves the builder" | "**gamepad B** once leaves the grid, **B** again leaves the builder … ⛔ **B ONLY** (`Gamepad_FaceButton_Right` / `Virtual_Back`) — the Escape key is NOT bound to either leg and must not be, per `AS-§6 A-2`" |

⭐ **WHY THIS WAS A FIX AND NOT A NIT — QA's reasoning, recorded so it is not re-litigated:** this was **the one file
of the feature carrying no `AS-§6 A-2` pointer**. A future author extending the exit test met *"Escape leaves the
grid"* with **nothing in the file to contradict it**, and the cheapest way to "complete" what that comment describes
is to **bind the Escape key** — which is an **automatic QA FAIL** under a CLOSED Jonathan ruling. The stale prose was
therefore not cosmetic: it was a live instruction toward a forbidden edit. ⇒ **the `A-2` register now reaches this
file too** (2 pointers, at the test's doc comment and beside `:1485`). ⚠️ A-2's **scope** is his own open question at
**`TASK-1300`** — the pointers say so explicitly; this is ⛔ not a re-litigation and it blocks nothing.

⛔ **THE FENCE THAT STILL HELD, AND HOW I PROVED IT: comments only, ZERO executable lines changed.**
- ⛔ Untouched: every assertion in both tests, `ExitCardGridFocus()`, the precedence guard, WARN-3's fold, and
  **both `DeckBuilderWidget.{h,cpp}`** (neither file was opened for edit this pass).
- **How it was verified — not by eye.** A comment-stripping state machine (string- and char-literal aware, so a `//`
  inside a `TEXT("…")` cannot be mistaken for a comment) was run over the pre-edit baseline and the edited file, and
  the whitespace-normalised, comment-free line lists compared: **920 executable lines both sides, byte-identical,
  `difflib` empty.** Corroborated by `diff -u`: exactly **2 hunks**, both landing wholly inside comment blocks.
- ⛔ **`EKeys::Escape` is still 0/0 in this file — the census QA certified was NOT moved.** The new prose deliberately
  says *"the Escape key"* and never the `EKeys::` token; measured after the edit, this file contains **0** `EKeys::`
  tokens of any kind (so the 26-total above is unchanged by this fix). Bare-word `Escape` lines went 2 → 4, all four
  comment-only, all four now stating the **correct** rule.
- ⛔ Nothing compiled, ⛔ no Live Coding, ⛔ no Git, ⛔ nothing staged, ⛔ no PIE, ⛔ no board edit (`TASK-1286` stays
  `qa-passed` — a QA-prescribed remedy is not a status change), ⛔ the GUI editor (PID 8972) was not touched.
- 🔎 **For the host's delta check (`TASK-1291`):** the whole of this pass is the 2 comment hunks in
  `Tests/SiegeDeckSlotsTest.cpp` plus this handoff. Any executable-line delta against what QA reviewed is ⛔ NOT mine.

⛔ **Prose I DID correct beyond the three named flag sites, both in files already in the diff and both comment-only:**
`DeckBuilderWidget.cpp` (the `IsCardGridFocusLive` WARN-3 block) — "the grid EATS ~~Escape /~~ gamepad B"; and
`DeckBuilderWidget.h` (the `ExitCardGridFocus` doc comment) — "the gesture behind gamepad B / `Virtual_Back`. ⛔ NOT
Escape". Both asserted the dropped binding as live fact; leaving them is exactly how the next reader re-derives the
wrong rule. ⛔ No declaration, no signature and no behaviour changed at either.

| fence | measured |
|---|---|
| `EKeys::Tab` | **0** in all three files (⚠️ I wrote the token into a comment mid-pass and removed it — a comment would have tripped a mechanical census) |
| letters / digits | **0** — a grep for `EKeys::` followed by any single letter A–Z or any digit-word returns nothing |
| `KBD-§` in the `CONVENTIONS.md` diff | **0** |
| `IA_` / `IMC_` / `EnhancedInput` / `BindAction` **code** | **0** (comment-only mentions remain, explaining why there is none) |
| `.uasset` / `Content/` / `Saved/SaveGames/` in `git status` | **0** |
| `+` / `−` button bodies | diff **0** — they are BP graph nodes in `WBP_DeckCardTile.uasset`; `git status` carries no `Content/` entry |
| `AddCopy` / `RemoveCopy` / `PersistWorkingDeck` bodies | diff **0** — no hunk lands inside them; the `.cpp` diff is **pure insertion, zero deleted lines** |

⚠️ **READ THIS BEFORE HUNTING FOR A DELETED `B` BINDING:** the whole feature is **uncommitted**, so `git diff` vs
`HEAD` shows the amendment as insertion and the pre-amendment lines never existed in history. **The delta is visible
on disk, not in a diff.** Verify the deletion the way (G3) does — by census, below.

## A3. ⭐ THE THREE MEASUREMENT DUTIES (spec block (G))

### (G1) What handles `Escape` / Back on the deck-builder screen **today**? ⇒ **NOTHING.**

Four independent reads, each with a firing positive control (`SC-§39` — a zero from an unproven reader is not a zero):

| lane | what I read | result | positive control |
|---|---|---|---|
| C++ | a grep for `virtual FReply NativeOn(Preview)?Key` over `Source/**.h` | the **only** `NativeOnKeyDown` override in the project is `UDeckBuilderWidget.h:561` — this feature's own. (`USiegeAssistantConsoleWidget.h:723` overrides `NativeOnPreviewKeyDown`, a different screen.) | the grep found both overrides |
| Blueprint (the three menu WBPs) | `grep -ac` for `OnKeyDown` / `OnPreviewKeyDown` / `Escape` / `Virtual_Back` / `Gamepad_FaceButton_Right` in `WBP_DeckBuilder.uasset`, `WBP_DeckCardTile.uasset`, `WBP_MainMenu.uasset` | **0 for every name, in all three** | `Construct` = **2** in each ⇒ the name-table reader works |
| `BP_MenuGameMode` | same census | `OnKeyDown` **0** · `Escape` **0** · `Gamepad` **0** | `SetInputMode` 1 · `InputMode` 1 · `UIOnly` 1 · `WidgetToFocus` 1 · `BeginPlay` 4 ⇒ reader works |
| Slate engine side | a grep for `EUINavigationAction::Back` across `Slate` + `SlateCore` + `UMG` | exactly **3** hits: the two rules that CREATE it (`NavigationConfig.cpp:37-38`) and **one** consumer — `SComboBox.h:497` (closes an open combo box) | the grep found all three |

**⇒ The named answer the spec asked for: leaving the deck builder today is a MOUSE CLICK on an "Exit" button, never a
key.** Measured on the live graph (`get_asset_graph /Game/UI/WBP_DeckBuilder`, strand `OnBackClicked`):
`Construct` builds a `Button` (`K2Node_GenericCreateObject_23`) labelled `"Exit"` (`Conv_StringToText "Exit"` →
`SetText`), bound via `AssignDelegate` (`K2Node_AssignDelegate_7`) to `Custom Event: OnBackClicked`, whose body is
`CreateWidget(WBP_MainMenu_C)` → `IsValid` → `AddToViewport` → `RemoveFromParent(self)`. That delegate is `OnClicked`
— reachable by mouse, or by Slate `Accept` while the button holds focus. ⛔ **It is bound to no key.**

⇒ 🙋 **FOR THE VERIFIER, SO AN EXPECTED BEHAVIOUR IS NOT FILED AS A BUG: the SECOND B press does NOTHING, and that is
BY DESIGN, not a defect of this row.** There is nothing on this screen for a fall-through Back to reach.
Acceptance (11)'s "⚠️ if the FIRST Back closes the deck builder ⇒ fail" is the case that would indicate the
precedence defect; a second press that leaves the builder open is the correct, measured outcome.
⛔⛔ **AMENDMENT 2 — `Escape` IS NO LONGER BOUND AT ALL (`AS-§6 A-2`), so every "`Escape` / B" pairing in this
subsection now reads "B" only.** ⚠️ And do not press `Escape` to test this row: in the editor an unconsumed `Escape`
is the PIE **Stop** shortcut, and since the grid no longer consumes it, even the FIRST press may end the session.

### (G2) Does **X** (`Gamepad_FaceButton_Left`) carry any existing meaning? ⇒ **NO.**

- **At the line, Slate's complete default table — `NavigationConfig.cpp:19-38`, read in full, not summarised.**
  `KeyEventRules` (direction) = `Left`/`Right`/`Up`/`Down` + `Gamepad_DPad_*` **only**. `KeyActionRules` (action) =
  `Enter`, `SpaceBar`, `Virtual_Gamepad_Accept.GetVirtualKey()` → `Accept`; `Escape`,
  `Virtual_Gamepad_Back.GetVirtualKey()` → `Back`. ⇒ **Slate's defaults have exactly two actions, and
  `Gamepad_FaceButton_Left` appears NOWHERE in the table.**
- **Project census, with the firing positive control the spec demanded:**

| lane | `Gamepad_FaceButton_Left` | positive control (`Gamepad_FaceButton_Right`, same instrument, same roots) |
|---|---|---|
| `Source/` `Config/` `Tools/` `Docs/` (text) | **0** | **2 hits** — both in this feature's own files ⇒ reader alive |
| `Content/**` (`grep -a`, binary) | **0 files** | **1 file** — `Content/Variant_Platforming/Input/IMC_Platforming.uasset` ⇒ binary reader alive |

- **Corroborating the B-is-Back chain myself rather than taking QA's word:** `InputCoreTypes.cpp:423-424` —
  `Virtual_Accept = FPlatformInput::GetGamepadAcceptKey()`, `Virtual_Back = FPlatformInput::GetGamepadBackKey()`; and
  `InputCore/Public/GenericPlatform/GenericPlatformInput.h:32-35` — `GetGamepadBackKey()` returns
  `EKeys::Gamepad_FaceButton_Right` (⚠️ the path is under **`InputCore`**, not `Core`, as the QA report's shorthand
  implies). `:27-30` likewise gives `GetGamepadAcceptKey()` → `Gamepad_FaceButton_Bottom`.

### (G3) Is the old **B → `RemoveCopy`** binding GONE, not shadowed? ⇒ **YES.**

`Gamepad_FaceButton_Right` appears **5 times** across the three files: **4 in comments** (the key table at `.h:235`,
the provenance of the change at `.h:241` and `.cpp:1469`, and the (G2) census note at `.cpp:1482`) and **exactly ONE
in code** — `DeckBuilderWidget.cpp:1521`, inside the exit handler (⚠️ line numbers re-measured after amendment 2's
prose rewrite; the code hit was `:1514` before it and `:1507` in the pre-amendment-2 handoff text). The Remove line now reads
`if (Key == EKeys::Delete || Key == EKeys::Gamepad_FaceButton_Left)` (`:1477`). ⛔ Not duplicated, ⛔ not kept "as well".

## A4. The exit clause (spec (D)) — the four conditions, and **which target focus lands on**

**One entry point, shared:** `UDeckBuilderWidget::ExitCardGridFocus()` (`.h:352`, `.cpp:1383`). The key handler calls
it once (`.cpp:1516`); the suite calls the **same** function three times. ⛔ There is no "test variant" — the
`AddCopy`/`RemoveCopy` principle applied to the third gesture.

| condition | how it is met |
|---|---|
| **(i)** `GetFocusedCardIndex() == INDEX_NONE` | `FocusedCardIndex = INDEX_NONE` is written **first**, before any Slate work, so a re-entrant focus change already sees a dead grid. ⛔ Written directly, not via `SetFocusedCardIndex` (that mutator no-ops on `INDEX_NONE` and would leave Slate focus sitting on the tile). **This is the suite's assertion.** |
| **(ii)** no card tile holds Slate focus | moving the Slate focus off the tile is what makes it true; a tile that no longer holds focus reports false from both `HasAnyUserFocus()` and `HasFocusedDescendants()`. **Closes at the verify leg.** |
| **(iii)** `IsCardGridFocusLive()` false | follows from (i) — that predicate's first line returns false on `INDEX_NONE`. ⛔ **Deliberately NOT the suite's assertion** — see A5. |
| **(iv)** focus **lands somewhere named** | `ResolveGridExitFocusTarget()` (`.h:739`, `.cpp:1332`) — preference order below. |

### ⭐ THE TARGET, NAMED: **the deck-bar entry's own `SlotButton`** — and the measurement that made it reachable

1. **Preferred — the `UDeckSlotEntryWidget` for `EditingDeckIndex`, else the first bar entry.** **Reachability,
   quoted:** `UDeckBuilderWidget` holds its own member `TArray<TObjectPtr<UDeckSlotEntryWidget>> DeckBarEntries`
   (**`DeckBuilderWidget.h:688`**), populated in `NativeConstruct` (**`.cpp:601-607` → `:639`**:
   `DeckBar->ClearChildren()` → `DeckBarEntries.Reset()/Reserve()` → `CreateWidget<UDeckSlotEntryWidget>(this)`
   ×`NumFixedDeckSlots` → `DeckBarEntries.Add(Entry)`). ⇒ **the bar needs NO tree walk and no knowledge
   of the WBP's layout — it is already a member of this class.**
   > ⚠️ **CITATIONS CORRECTED 2026-09-17 (`TASK-1299` NIT-1).** This handoff first wrote `.h:610` / `.cpp:598-606`,
   > stale by ~78 lines — drift caused by the amendment's own prose additions to the header, ⛔ not by an invented
   > measurement. QA verified the **claim** independently and only the line numbers moved; I re-measured all four
   > sites myself before writing them here (`grep -n DeckBarEntries` on both files). ⛔ No code changed for this.
2. **Within the entry, the focus lands on its inner `UButton` (`SlotButton`), found by a walk.**
   - **Why a walk:** `SlotButton` is **`protected`** (`DeckSlotEntryWidget.h:120`) and the entry's tree is
     **code-authored** — `ConstructEntryTree` builds `OutlineBorder` (`UBorder`, and `WidgetTree->RootWidget`,
     `.cpp:87/109`) → `SlotButton` (`UButton`, `.cpp:114`) → `SlotLabelText`, with `OutlineBorder->SetContent(SlotButton)`
     (`.cpp:125`). `UBorder` is a `UContentWidget`, i.e. a `UPanelWidget`, so one `GetChildAt` hop reaches the button.
     ⛔ **This touches NOTHING in `DeckSlotEntryWidget.*`** — that file's gesture contract is `DECK-§5`-pinned and this
     row's `names:` fence forbids editing it. **`git status` confirms it is not in the diff.**
   - **⛔ Why NOT the entry ROOT** (the obvious alternative, rejected on a measured cost): focusing the root would need
     `SetIsFocusable(true)` on a `DECK-§5` widget — adding a focus stop that class never had — **and** would park the
     focus on a widget `SButton::OnKeyDown` never sees, **silently killing `Enter` → `SelectDeckForEdit` on the deck
     bar.** The exit must not cost a shipped gesture.
   - ⇒ **Landing on `SlotButton` restores the focus to precisely where Slate's own default navigation already stops**
     (QA: "the deck-bar focus he saw is Slate's own default navigation over the ten `SlotButton`s"). After an exit,
     `Left`/`Right` still walk the bar and `Enter` on a slot still reaches `SelectDeckForEdit`, **byte-identically**.
3. **Declared fallback — `GetRootWidget()`**, the builder's own root. ⛔ Non-null whenever we arrived from a live grid
   (a tile resolved, so a root exists) ⇒ **"focus left nowhere" is unreachable by construction, not merely unlikely.**

Focus is applied with the `TASK-1274` idiom used by `FocusCardTile`: `FSlateApplication::SetUserFocus(UserIndex,
SlateTarget, EFocusCause::Navigation)` — the **only** cause that paints a focus rectangle — with the
`LocalPlayer->GetSlateOperations()` deferred fallback on refusal. ⛔ **`FocusCardTile` is deliberately NOT refactored
into a shared helper**: it is code `TASK-1290` passed and spec block (H) does not re-open it. The ~10 duplicated lines
are the cheaper of the two risks, and they are named here rather than hidden.

## A5. The test (spec (10)) — ⛔ **the assertion is (i), NOT (iii)**

`Siegebound.Deck.ExitingCardGridClearsTheFocusedIndex` (`EditorContext | EngineFilter`),
`Tests/SiegeDeckSlotsTest.cpp`. **Suite is baseline +2 for this row.**

⛔ **THE VACUOUS-TEST TRAP, AVOIDED AND WRITTEN INTO THE TEST'S OWN HEADER (`SC-§39`):** once WARN-3 is folded,
`IsCardGridFocusLive()` returns **false in this lane BY CONSTRUCTION** — Slate IS initialised under `EditorContext`,
and no `WBP_DeckCardTile` resolves for a `NewObject`'d builder, which is exactly the live-Slate/no-tile branch just
changed to `return false`. An assertion *"`IsCardGridFocusLive()` is false after exit"* **cannot fail and is not
evidence.** ⇒ the exit assertion is **(i)**, with its positive control:

- **(b) POSITIVE CONTROL** — `GetFocusedCardIndex() == TileK` **before** the exit, so the getter is shown able to
  report a non-`INDEX_NONE` value at all. `TileK = Count/2`, a **middle** tile, so a bug that silently focuses 0
  cannot pass by coincidence.
- **(c)** `ExitCardGridFocus()` returns **true**, then `GetFocusedCardIndex() == INDEX_NONE` and `GetFocusedCardID()`
  is `None`.
- **(a)** the (C) ruling's other half, asserted **first**: with nothing focused the exit returns **false** ⇒ the key is
  **not consumed**.
- **(d)** a **second** exit returns **false** — the nested-Back sequence, in the suite.
- **(e)** `Down` **re-arms** on tile 0 afterwards ⇒ ⛔ the exit does not brick the grid.
- **(f)** the `x/50` total and card K's count are compared against values captured **before** anything ran ⇒ the exit
  mutates no deck. (⚠️ I first wrote this as a `TestEqual` of `GetTotalCount()` against a second read of itself — the
  very `SC-§39` trap this section is about — and fixed it. Flagging it because it is the shape QA should grep for.)

**⛔ NO EXPECTED-MESSAGE PIN IS ADDED, AND THAT IS MEASURED, NOT ASSUMED:** `ExitCardGridFocus()` reaches neither
`AddCopy` nor `RemoveCopy`, so it produces **ZERO** `PersistWorkingDeck` refusals. The sibling test's `Occurrences 2`
pin lives in its own `RunTest` scope and is **untouched** — no off-by-one re-pin was needed, and none was made.
Same no-write architecture: `NewObject`'d builder, `NativeConstruct` never runs, `EditingDeckIndex` stays
`INDEX_NONE` (asserted), `FDeckScratchGuard` in place, scratch slot asserted absent.

## A6. WARN-3 folded (spec (9))

`DeckBuilderWidget.cpp` `IsCardGridFocusLive()`, the live-Slate/no-tile fallback: `return true` → **`return false`**,
**and the comment rewritten to say WHY rather than what** — the old text justified `true` as "an offline widget",
which the *first* fallback already owns. The new comment names the stake the amendment created: this predicate is now
the **back-precedence guard**, so a wrong `true` means the grid **eats `Escape` / B in exactly the state where there
is nothing on screen to go back from.** QA measured it cannot affect the suite (reachable only from
`NativeOnKeyDown`, which the suite never drives) — that still holds.

## A7. ⛔⛔ **RULED AND CLOSED — `AS-§6 A-2` WINS. `Escape` IS DROPPED; THE EXIT IS GAMEPAD-ONLY.**

> **⭐ STATUS (amendment 2, 2026-09-17).** This section was a **flag**; it is now a **record**. The previous instance
> raised the `A-2` collision instead of quietly deviating — the orchestrator read `A-2` at source, agreed, and ruled
> the conservative way while 🧑 is out. **`Escape` was deleted from the exit condition before this ever compiled.**
> The gamepad half is **untouched** — `A-2` names `Escape` only.
>
> ⚠️ **The one thing still open, and it is HIS, not ours:** whether `A-2`, written for the in-match cancel routes,
> also binds a **main-menu deck builder**. That is a genuine *scope* question and a reviewer's agreement cannot
> authorize it (`CONVENTIONS:2679` cl. 13). Boarded as **`TASK-1300`** for 🧑; ⛔ **it blocks nothing** — the code
> already complies. Until he answers, `A-2` is read at its **widest: nothing absorbs `Escape`, anywhere.**
>
> **⛔ The `A-2` citation is KEPT in the code at both sites** (`.h` key-table block, `.cpp` exit block) and here. It
> is the register pointer that caught this before a compile; deleting it would delete the only thing that worked.
>
> **If he later scopes `A-2` out of the menus, the re-add is one line** — `Key == EKeys::Escape ||` back on the exit
> condition — and ⛔ **nothing else**: the tests drive `ExitCardGridFocus()` directly (`EKeys::Escape` = **0** in
> `SiegeDeckSlotsTest.cpp`, measured).

**The collision that was flagged, quoted from source** — kept verbatim so the next reader meets the register entry,
not a summary of it. `CONVENTIONS.md` ~line 789, `AS-§6` **RULING A-2**, Jonathan's closed ruling of 2026-08-04:

> ⛔ **Any future task that absorbs `Escape` — `NativeOnKeyDown`, `NativeOnPreviewKeyDown`, an Enhanced Input action,
> a Slate `FReply::Handled()` on `EKeys::Escape`, or a viewport-level intercept — is overturning a Jonathan ruling and
> is an automatic QA FAIL.**

It is cited as **project-wide** in four later places, including the class next door in this very feature:
`DeckSlotEntryWidget.h:60` ("Escape stays permanently unabsorbed, **project-wide** (AS-§6 A-2)") · `CONVENTIONS:7519`
(controls-help overlay) · `:8316` (recall channel) · `:8595` (climb). `CONVENTIONS:2679` names this precedent as the
canonical case that **a reviewer agreeing with a change does not authorize it.**

**`TASK-1286`'s 2026-09-17 amendment (1) ordered exactly that `Handled()` on `Escape` and did not cite `AS-§6 A-2`
anywhere** — which is why the collision was read as *not seen* rather than overruled. **Amendment 2 confirmed that
reading and dropped `Escape`.**

**What instance 1 did, and why it was right.** It implemented **as the board specced it** and **flagged** — `SC-§97`
forbids an agent substituting 🧑 his key choice, so between "follow the row and flag" and "quietly deviate", only the
first was its to do. ⛔ It did not claim the conflict was resolved. That is now a positive exhibit in law
(`SC-§121` cl. 5): **raise the collision, do not silently resolve it.**

**The measured evidence — now the material for `TASK-1300`'s scope question, ⛔ NOT a ruling and never was.** The harm
`A-2` protects is *degrading a shipped `Escape` route*, and on **this** screen there is none to degrade:

- (G1) above: `Escape` is handled by **nothing** on the deck-builder screen today — no C++ handler, no BP `OnKeyDown`
  in any of the three menu WBPs, nothing in `BP_MenuGameMode`, and Slate's `Back` action has one engine consumer
  (`SComboBox`) that is not on this screen.
- `A-2`'s own named protected routes — **placement, spell targeting, group-pick** — live in `ASiegePlayerController`
  (`SiegePlayerController.cpp:803`, `:835`, `:873`, `:893`) and are each gated behind in-match state
  (`GroupPickStage != None`, `bInTargetingMode`, placement mode). ⛔ **None can be live on `L_MainMenu`.**
- The (C) ruling makes the absorption the narrowest possible: `Handled` **iff** a card tile really holds focus **and**
  the exit changed state. In every other state `Escape` falls through untouched.

**⛔ THE REVERSAL — APPLIED, 2026-09-17 (amendment 2). It was one line, and the gamepad half is untouched** (`A-2`
names `Escape` only; `Virtual_Back` / `Gamepad_FaceButton_Right` are not in it):
1. ✅ `DeckBuilderWidget.cpp` (exit condition, was `:1514`) — `Key == EKeys::Escape || ` **DELETED**. The line now
   reads `if (Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Virtual_Back)`.
2. ✅ `SiegeDeckSlotsTest.cpp` — **no change, and this is measured**: the tests drive `ExitCardGridFocus()` directly;
   `EKeys::Escape` = **0** occurrences in the file (the bare word appears as prose at `:1390`, `:1485`).
3. ✅ Prose rewritten at all three flag sites (`.h` key-table block · `.cpp` exit block · this section), **`A-2`
   citation RETAINED at each**, so no future reader meets the gamepad-only exit without meeting the ruling.

⇒ code census **20 → 19** (§A2), and **the keyboard has no exit key out of the grid** (⚠️ `Tab`'s inherited Slate
`Next` remains, per (A), and the mouse "Exit" button is unchanged — so the screen is no worse off than before this
feature existed). ⛔ Do not file the missing keyboard exit as a defect: it is the ruling.

## A8. What `TASK-1299` should scrutinise

1. **The (C) ruling's enforcement point** — `NativeOnKeyDown` `.cpp:1521-1528` (re-measured after amendment 2). `Handled` is returned **only** inside
   `if (ExitCardGridFocus())`. ⚠️ Note that `bGridFocused == true` already implies `FocusedCardIndex != INDEX_NONE`,
   so that `if` can never take its false branch **from this call site** — it is an invariant's enforcement point, not
   dead code, and it is what makes the suite's (a)/(d) assertions meaningful.
2. **The exit target chain** — is `GetRootWidget()` genuinely non-null on every path that reaches it? My argument is
   "we only got here because a tile resolved", which is true of the live path but is an **inference**, not a
   measurement, for a hypothetical caller. The null case degrades to "index cleared, focus not moved" + a `Verbose`
   log, which is safe but is **(iv)-incomplete**; I chose that over refusing the exit.
3. **`FindFirstButton`'s recursion** — bounded by the same `MaxTreeDepth = 32` the tile walk uses, and it returns the
   **first** `UButton` in panel-child order. If a future entry tree grows a button **before** `SlotButton`, focus
   would land on that one instead. Latent, same shape as NIT-4.
4. **The seven includes / five include hunks** (A2, NIT-1 correction) — `Components/Button.h` is the amendment's only
   new one.
5. ✅ **A7 — the `AS-§6 A-2` collision is RULED AND APPLIED (amendment 2): `Escape` is dropped, the exit is
   gamepad-only.** ⛔ **The gate should now BLOCK on the opposite condition**: any `EKeys::Escape` reachable from an
   `FReply::Handled()` is a BLOCKER. ⚠️ Count **code** and **comments** separately (§A2) — the `A-2` citation
   deliberately keeps `EKeys::Escape` in comment prose at two sites, so a raw grep is non-zero on a correct diff.
   The remaining scope question ("does `A-2` bind a main-menu screen?") is 🧑 his, at **`TASK-1300`**, and ⛔ blocks
   neither this gate nor `TASK-1291`.
6. ⛔ **NIT-2…NIT-6 are DEFERRED by the manager — recorded, not boarded.** I did **not** take NIT-2's
   `CancelFocusRequest()`: it was declined on purpose as a behaviour change QA did not ask for, in the one path this
   amendment already rewrites. ⛔ Please do not read its absence as an oversight.

## A9. Verifier notes added by this amendment (acceptance (11))

- ⛔⛔ **SUPERSEDED BY AMENDMENT 2 — DO NOT PRESS `Escape` AND DO NOT SCORE IT.** `Escape` is **not bound** (`A-2`);
  a press does nothing here and, in PIE, an unconsumed `Escape` is the editor's **Stop** shortcut, so spending one
  ends the session for nothing. The retired sequence read: `Down` → Accept → Remove → ~~`Escape`~~ → Accept → `Down`
  → Accept → Remove.
- ⭐ **The exit itself is now GAMEPAD-ONLY, and is therefore `UNOBSERVABLE` unless `Gamepad_FaceButton_Right` /
  `Virtual_Back` can be injected as a real Slate key event.** One line, ⛔ zero attempts spent, ⛔ never read as a
  pass (`VER-§5`). `TASK-1299`'s code read is what closes it.
- ⭐ **What IS still keyboard-observable, and is the whole of the keyboard acceptance:** `Down` → Accept (**n+1**, the
  control) → Remove (**n**). Final read **n**; ⛔ net zero on his real deck. That proves entry, Accept and Remove.
- 📌 **The gamepad half (X = Remove, B = Exit) is likewise `UNOBSERVABLE` without face-button injection** — same one
  line, same ⛔ zero attempts, same ⛔ never a pass.
- The observable is `"Deck: n/50"`, not `"n/50"` (QA measured it in `RefreshAll`).

## A10. ⚠️⚠️ **INCIDENT, DECLARED BY ME BECAUSE IT IS MINE — I TRUNCATED `TASKBOARD.md` TO 0 BYTES AND RECOVERED IT IN FULL.**

⛔ **Nothing was lost, nothing was committed, and no manager content is missing — but this is declared, not buried,
because the recovery is only worth the checkable account attached to it (`SC-§119`).**

**What happened.** To flip one `status:` line I used a Python `io.open(path, "w")` write. The truncating handle
emptied the file, and the write then died on a `UnicodeEncodeError` (a surrogate pair in *my own* string literal, not
in the file's data) — so the file was left at **0 bytes**. ⚠️ **This is the exact failure mode the user's memory index
already records against a previous orchestrator**, which is why it is worth a law, not just a fix.

**How it was recovered — the account, each step checkable:**

| step | what | measured |
|---|---|---|
| 1 | Establish the baseline | `git show HEAD:…/TASKBOARD.md` ⇒ **36 237 lines**. The board's last commit is **`aed6922`, 2026-09-17T22:12:46Z**; the git index held the same blob (⇒ nothing newer was staged). ⚠️ The pathspec is rooted **one level up** (`SC-§102`). |
| 2 | Find every change made after that commit | Scanned this session's **subagent transcripts** for `Edit`/`Write` tool calls whose `file_path` is `TASKBOARD.md` and whose timestamp is later than the commit ⇒ **45** calls, of which **44 had a success result** and **1 had been rejected by the tool** (`String to replace not found`). |
| 3 | Rule out any other writer | Every non-`Edit` touch of the file after the commit was re-read from the transcripts: **6 shell commands, all read-only** (`grep`/`sed`/`awk`/`git diff`), plus my own. ⇒ the Edit list is complete. |
| 4 | Replay | Applied the **44 successful edits in timestamp order** onto the baseline. ⭐ **Every single `old_string` matched EXACTLY ONCE** — 0 not-found, 0 ambiguous. The rejected edit was ⛔ not replayed. ⇒ 36 237 → **36 302 lines**. |
| 5 | Verify against evidence taken BEFORE the truncation | Line **3113** and lines **3248-3256** were captured in my own tool output minutes earlier; the restored file matches both **byte for byte**, `#### TASK-1286` is at **3247** and its `status:` at **3249** exactly as before. Markers present: `TASK-1300` ×13, `TASK-1299` ×32, `SC-§121` ×8. |
| 6 | Verify against git | `git status --porcelain` = the **same 5 `M` + 2 `??`** as at session start; board diff vs `HEAD` = **81 insertions / 16 deletions** — the manager's amendment-2 edits and nothing else. |
| 7 | Install it | ⛔ **NOT** with another truncating write — `Copy-Item`, then the `status:` flip through the `Edit` tool. |

**⇒ The only thing the incident changed is that `TASKBOARD.md`'s line endings are now uniformly `LF`.** `git diff` is
unaffected (`core.autocrlf=true` normalises), and the file had no `CRLF` pairs before either.

🙋 **The law I'd ask the manager to write, because it would have prevented both occurrences:** *never write a
UTF-8 project file through a truncating handle.* `Edit` (which cannot truncate on failure), or write a temp file and
`Copy-Item`. A truncating open is a **destructive act that happens BEFORE the operation can fail**, so a crash
mid-write is not a no-op — it is a delete. ⚠️ And the corollary that actually saved this file: **the session's own
subagent transcripts are a complete, ordered, verifiable edit log** — recovery is replay, not reconstruction, and it
is checkable precisely because every `old_string` must still match exactly once.
