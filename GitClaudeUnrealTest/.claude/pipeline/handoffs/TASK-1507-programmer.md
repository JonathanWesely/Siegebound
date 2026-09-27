# TASK-1507 — [DECK-KEYBOARD-SET-ACTIVE] — gameplay-programmer handoff

- **Status set:** `ready-for-qa` → next `TASK-1509` (QA) → 5a `TASK-1510` → 5b `TASK-1511` → host `TASK-1514`.
- **Date:** 2026-09-26. **No compile, no PIE, no git, no `.uasset`** (row (7)).
- **Blocker release:** `CONVENTIONS.md` carries `DECK-3-KEYBOARD-SET-ACTIVE-RULED` (grep: exactly 1 hit), with 🧑 his words verbatim: *"yes, add a keyboard route to set the active deck"*, key **`Home` + gamepad Y**. So the row's "proposed pins" are law and are used character-for-character:
  `EKeys::Home` · `EKeys::Gamepad_FaceButton_Top` · `/Game/Input/Actions/IA_MenuSecondary` · `UDeckBuilderWidget::MenuSecondaryActionPath` = `TEXT("/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary")` · `UDeckBuilderWidget::HandleMenuNavSecondary()`.

---

## 1. (3) FIRST — CONFIRMED at source: an injected `IA_MenuLeft`/`IA_MenuRight` on a deck-bar slot moved nothing

Cited by quoted text, not line address (`SC-§126` cl. 12). Files are the HEAD state; none of these lines was edited.

1. **The widget's door 3 declines them.** In `UDeckBuilderWidget::HandleCardGridKey(const FKey& Key)`, with the grid unfocused the directional block claims one key only:
   > `if (Direction == EUINavigation::Down && GetCollectionCardIDs().Num() > 0)`

   Left/Right fall to the function's tail:
   > `return FReply::Unhandled();`

   and `UDeckBuilderWidget::RouteMenuNavKey` discards the reply on purpose:
   > `// ⭐ THE ONE IMPLEMENTATION. The FReply is DISCARDED on purpose` … `HandleCardGridKey(Key);`

   Its precondition repair doesn't run on the bar either. It is gated on `if (FocusedCardIndex != INDEX_NONE && !bGridWasLive)`, and `ExitCardGridFocus` (the `IA_MenuBack` landing) sets `FocusedCardIndex = INDEX_NONE;` before it moves focus to the bar.
2. **The subsystem's ring declines them.** `USiegeMenuInputSubsystem::HandleMenuLeft()` / `HandleMenuRight()` are `StepFocusedStop(-1);` / `StepFocusedStop(+1);`. `StepFocusedStop` returns at:
   > `if (DeclineIfActiveTargetSelfDriving(FString::Printf(TEXT("StepFocusedStop(%+d)"), Direction)))`

   because this screen registers with:
   > `MenuInput->RegisterSelfDrivingMenuNavTarget(this);` (`UDeckBuilderWidget::RegisterAsMenuNavTarget`, TASK-1471).
3. **Nothing else listens.** `inject_input_action` reaches the PC's `UEnhancedInputComponent` bindings only. It has no edge into Slate's `ProcessKeyDownEvent` (the TASK-1423 call-graph finding, header comment "THE DIAGNOSIS FIRST").

**Result:** an agent could land on a slot (`IA_MenuDown` into the grid, then `IA_MenuBack` out; `ExitCardGridFocus` lands on the editing slot's `SlotButton`) but could not walk the bar. **The manager's source-read is CONFIRMED.**

---

## 2. What changed

**Files touched (all three are on the row's WRITES list, and every change is an addition):**
- `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h`
- `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp`

`git diff -U0 -- Source/ | grep -c '^-[^-]'` = **0**. No line was deleted or edited, only added. (This was measured by grepping the diff for `-` lines, not taken from `--numstat`.)

**New symbols (all plain C++; no `UFUNCTION`, no `UPROPERTY`):**

| Symbol | Access | Role |
|---|---|---|
| `static int32 ResolveDeckBarActivationSlot(const FKey&, int32 FocusedBarSlotIndex)` | public | The pure half of the key's decision (the testable half). |
| `static const TCHAR* MenuSecondaryActionPath` | public | The pinned asset path. |
| `static bool IsDeckBarActivationKey(const FKey&)` | private | The one spelling of the key pair `Home` · `Gamepad_FaceButton_Top`. |
| `int32 FindFocusedDeckBarSlot() const` | private | Which bar slot holds Slate focus, or `INDEX_NONE`. |
| `void HandleMenuNavSecondary()` | private | `IA_MenuSecondary` → `RouteMenuNavKey(EKeys::Home, …)`. |
| `bool RelayDeckBarNavigationKeyToSlate(const FKey&, const TCHAR*)` | private | Door 3's bar walk (§4). |

**Assets referenced (by path only; `TASK-1508` authors them in parallel):**
- `/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary` (through the constant)
- its `Home` + `Gamepad_FaceButton_Top` mappings in `/Game/Input/IMC_MainMenu`

This row loads nothing else. If the asset is absent, the per-action degrade-open makes **only** this key inert, and the one `BindMenuNavActions` summary line names it: `ABSENT, so THOSE keys are inert on this lane: IA_MenuSecondary`.

---

## 3. The new branches, quoted

**(a) The decision, in `HandleCardGridKey(const FKey& Key)`.** Appended after the directional block and before the `// Everything else is NOT CLAIMED` tail:
```cpp
	if (IsDeckBarActivationKey(Key))
	{
		const int32 BarSlot = ResolveDeckBarActivationSlot(Key, FindFocusedDeckBarSlot());
		if (BarSlot != INDEX_NONE)
		{
			SetActiveDeckBySlot(BarSlot);
			return FReply::Handled();
		}
	}
```
- `SetActiveDeckBySlot` is the function right-click calls: `Entry->OnRightClicked.BindUObject(this, &UDeckBuilderWidget::SetActiveDeckBySlot);` in `NativeConstruct`, unchanged.
- The key is `Handled` even when the model refuses (an illegal deck). That matches right-click, whose `NativeOnMouseButtonDown` does `OnRightClicked.ExecuteIfBound(SlotIndex); return FReply::Handled();` whatever the model then decides.
- **Doors that reach it:**
  - Slate tunnel: `NativeOnPreviewKeyDown` → `HandleCardGridKey(InKeyEvent)` → the `FKey` overload.
  - Slate bubble: `NativeOnKeyDown`, same path.
  - Door 3: `IA_MenuSecondary` → `HandleMenuNavSecondary` → `RouteMenuNavKey(EKeys::Home, TEXT("IA_MenuSecondary"))` → `HandleCardGridKey(Key)`.
- There is no second key table and no poll (`WasInputKeyJustPressed`: 0 before, 0 after).

**(b) The seventh binding, in `BindMenuNavActions`.** One more row of the same table:
```cpp
		{ UDeckBuilderWidget::MenuSecondaryActionPath,    TEXT("IA_MenuSecondary"), &UDeckBuilderWidget::HandleMenuNavSecondary },
```
It gets the same `LoadObject` degrade-open and the same `ETriggerEvent::Started`. Its handle goes into `MenuNavBindingHandles`, which `UnbindMenuNavActions` drains, so the seventh handle is removed by construction. `UnbindMenuNavActions` is byte-identical.

**(c) Door 3's bar walk, at the end of `RouteMenuNavKey`.** Placed after the precondition repair and before the shipped call:
```cpp
	if ((Key == EKeys::Left || Key == EKeys::Right) && FindFocusedDeckBarSlot() != INDEX_NONE)
	{
		if (RelayDeckBarNavigationKeyToSlate(Key, ActionName))
		{
			return;
		}
	}

	HandleCardGridKey(Key);
```

---

## 4. (3)'s mechanism, and why real-key behaviour on the bar is unchanged for every slot, both ends included

**Mechanism: door 3 hands Left/Right to Slate's own key route. It does not re-derive where a Left "should" land.**
- `RelayDeckBarNavigationKeyToSlate` builds an `FKeyEvent` the way `FSlateApplication::OnKeyDown` builds one for a physical key: live modifier state, not a repeat.
- It calls `FSlateApplication::ProcessKeyDownEvent`, the function a physical key reaches. From `FSlateApplication::OnKeyDown`, verbatim: `return ProcessKeyDownEvent( KeyEvent );`.
- It then calls `ProcessKeyUpEvent`.
- Engine precedent for doing this from game code: `UWidgetInteractionComponent::PressKey`, which is `FSlateApplication::Get().ProcessKeyDownEvent(KeyEvent)`, paired with `ReleaseKey` → `ProcessKeyUpEvent`.

**What that key-down then does:**
1. It tunnels root→leaf. The builder's `NativeOnPreviewKeyDown` → `HandleCardGridKey` still declines Left/Right on a slot (unchanged).
2. It bubbles from the focused `SlotButton`. `SButton::OnKeyDown` is not Accept, so it goes `SBorder::OnKeyDown` → `SWidget::OnKeyDown`, which returns `FReply::Handled().SetNavigation(Direction, Genesis)`.
3. `FSlateApplication::ProcessReply` runs the same `OnNavigation` leaf→root walk and `AttemptNavigation` → hittest grid that a physical arrow runs.

So the landing slot is Slate's own answer for all ten slots, both ends included, by construction. There is no geometry argument to be wrong, and no custom `Navigation` metadata can be bypassed, because this is the path that honours it. (`FSlateApplication::NavigateFromWidget` was considered and rejected: it skips the per-widget `OnNavigation` walk and uses a different boundary-selection loop, so it would be a near-copy of the real path, not the path itself.)

**Feedback guard.** The relay runs only if `GetNavigationDirectionFromKey(KeyDownEvent) != EUINavigation::Invalid`. Under that condition, the enabled, focused `SlotButton`'s `SWidget::OnKeyDown` (`if (bCanSupportFocus && SupportsKeyboardFocus())`) returns `Handled`. The key-down therefore never reaches `SViewport` → `UGameViewportClient::InputKey` → Enhanced Input, where it could re-fire `IA_MenuLeft`.
- Stated precondition: the button is enabled. `DeckSlotEntryWidget.cpp` has 0 `SetIsEnabled` / `SetIsFocusable` calls (grep).
- The project installs no custom `FNavigationConfig` (0 `SetNavigationConfig` calls in `Source/`).
- The default config maps `EKeys::Left` / `EKeys::Right` (`KeyEventRules.Emplace(EKeys::Left, EUINavigation::Left);`).
- On `L_MainMenu` the chain is closed anyway (`FInputModeUIOnly` → `SetIgnoreInput`, TASK-1274 §7).

**Why real keys are unchanged, for every slot including both ends:**
- The Slate doors (`NativeOnPreviewKeyDown`, `NativeOnKeyDown`, `HandleCardGridKey(const FKeyEvent&)`) are byte-identical (§5).
- `HandleCardGridKey(const FKey&)` with the TASK-1507 block removed equals the HEAD body exactly (measured: `True`).
- The only new code in it is gated on `IsDeckBarActivationKey(Key)`, which is false for `Left`/`Right`.
- So for `Key ∈ {Left, Right}` the function's control flow and return value are identical in every state. Both Slate passes return what they returned before, and Slate's navigation runs unchanged from every slot, 0 through 9.
- The relay is reachable only from `RouteMenuNavKey` (door 3). A physical key never enters it.

**Outcome instrument for 5b.** One `Log`-level line per door-3 bar step:
> `UDeckBuilderWidget::RelayDeckBarNavigationKeyToSlate: IA_MenuRight -> 'Right' handed to Slate's own key route on deck-bar slot 0 ('deck1'); key-down handled=true; the focused bar slot is now 1 ('deck2').`

At an end of the bar, "before == after" is Slate's answer. It is reported, not corrected. Deck names come from `USiegeDeckSaveGame::MakeFixedDeckName` (DECK-§1's one composer), never a hand-built `"deck%d"`.

---

## 5. (5) Unchanged, byte-for-byte — checked item by item

Measured by extracting each function body from `git show HEAD:` and from the working tree and comparing them (CRLF normalised):

| Item | Result |
|---|---|
| Right-click path `UDeckSlotEntryWidget::NativeOnMouseButtonDown` | ✅ `DeckSlotEntryWidget.cpp` sha256 `bd145fac…d077` and `.h` `d0245179…fcb0efc` are unchanged from the pre-edit baseline. The file is not in the diff. |
| `SetActiveDeckBySlot` body | ✅ IDENTICAL |
| `SelectDeckForEdit` | ✅ IDENTICAL |
| `SetActiveDeck` (the one persist and the TASK-1270 gate) | ✅ IDENTICAL. `TryActivateSavedDeck` is also IDENTICAL. |
| Every `DECK-§9` row | ✅ the table's lines are text-identical and at the same addresses as HEAD: Accept `:1687`, Remove `:1711`, Exit `:1775`, move dispatch `:1790`, enter-grid `:1799`, `NavigationFromKey` `:1273`. Only the two door definitions moved, `NativeOnPreviewKeyDown` `:1828→:1845` and `NativeOnKeyDown` `:1850→:1867` (+17, the appended block). DECK-§9 cl. 9 says to cite the function. |
| `Tab` absent | ✅ `EKeys::Tab` 0 → 0 |
| `Escape` scope | ✅ `EKeys::Escape` 5 → 5 total, 1 → 1 in code. The exit line is untouched. |
| Slate doors' call sites `HandleCardGridKey(InKeyEvent)` | ✅ 3 → 3. `NativeOnPreviewKeyDown` / `NativeOnKeyDown` / the `FKeyEvent` adapter are IDENTICAL. |
| `IsDeckLegal` | ✅ `DeckLibrary.{h,cpp}` are not in the diff. |
| No letter or digit key added | ✅ added production lines contain no single-letter or digit `EKeys` (scripted census = `[]`). The test file names `EKeys::H` and `EKeys::One` only as negative probes that must resolve to `INDEX_NONE`. |
| Only `SetActiveDeckBySlot` writes `ActiveDeckName` | ✅ assignments in code are 1 → 1 (`Save.ActiveDeckName = OutCanonicalName;` in `TryActivateSavedDeck`, reached only via `SetActiveDeck`). The key's only effect is the `SetActiveDeckBySlot(BarSlot)` call. |
| `SiegeMenuInputSubsystem.{h,cpp}` | ✅ zero bytes. sha256 `e4ad7ecf…4c77` / `9cb571d1…942d` are unchanged from the pre-edit baseline. |
| Also unchanged (not on the list, checked anyway) | `NativeConstruct`, `ExitCardGridFocus`, `UnbindMenuNavActions`, `RegisterAsMenuNavTarget`: IDENTICAL. |

---

## 6. Tests added (`SiegeDeckSlotsTest.cpp`)

**`Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes`** (`FSiegeDeckKeyboardSetActiveOnFocusedBarSlotTest`). EditorContext | EngineFilter.

Every row asserts state (`SC-§104`):
- **(0) PIN:** `FString(UDeckBuilderWidget::MenuSecondaryActionPath)` equals `"/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary"`, case-sensitive.
- **(0) Decision, positive:** for every slot 0..9, `ResolveDeckBarActivationSlot(Home, s) == s` and `(Gamepad_FaceButton_Top, s) == s`. Both ends are included.
- **(0) Decision, not claimed:**
  - `Home` / `Y` with `INDEX_NONE` → `INDEX_NONE`.
  - `Home` on slot 10 → `INDEX_NONE`, and on slot −2 → `INDEX_NONE`. Neither is clamped onto a neighbouring slot.
  - 17 other keys pressed on a focused slot 3 all → `INDEX_NONE`: Enter, SpaceBar, Left, Right, Up, Down, Escape, Tab, Delete, BackSpace, End, A, B, X, RightMouseButton, a letter (`H`), a digit (`One`). This fails a resolver that ignores the key.
- **(1) Effect**, on a migrated in-memory ten-slot save with a transient table (the TASK-1270 idiom). deck4 = 50 Footman (legal), deck5 empty, active = deck1. The helper `PressSetActiveKey` composes the same two halves the shipping route composes, in the same order: resolver, then `TryActivateSavedDeck(MakeFixedDeckName(slot))`.
  - **(a) Key with no slot focused:** `Home` and `Y` resolve to `INDEX_NONE` and activate nothing. `ActiveDeckName == "deck1"`, and the whole save is deep-equal to its pre-image.
  - **(b) Key on an empty (illegal) slot:** resolves to 4 (claimed). The activation is refused, and the refusal contains IsDeckLegal's `"exactly 50"`. `ActiveDeckName` is still `"deck1"`, the rim index is still 0, and the save is deep-equal.
  - **(c) Positive control, key on the legal slot:** resolves to 3 and activates. `ActiveDeckName == "deck4"` and the rim index is 3. The save is no longer its pre-image, and it is deep-equal to the pre-image with only `ActiveDeckName` changed.
  - **(d) Persists:** `SaveGameToSlot(scratch)` then `LoadGameFromSlot(scratch)`. The read-back `ActiveDeckName == "deck4"`, and every deck is deep-equal.
  - **(e)** Pad `Y` on the empty slot after the success is refused, and `ActiveDeckName` stays `"deck4"`.
  - **(f)** The key with no slot focused after the success leaves the post-(c) state intact.
- **Hermeticity:** `TestNotEqual(scratch, USiegeDeckSaveGame::SlotName)`. `FDeckScratchGuard` deletes the scratch slot on the way in and out. No widget, world or PIE.

**⚠️ ROW PREMISE CORRECTED (`SC-§101`).** The row says to test "on a test slot (`SetSlotNameForAutomationTests`)". **That seam does not reach the deck lane.**
- `void SetSlotNameForAutomationTests` exists only on `USiegeAccountSubsystem` and `USiegeSettingsSubsystem` (grep over `Source/`).
- `SlotNameOverride` is read only by the account slot's resolver (`return SlotNameOverride.IsEmpty() ? USiegeAccountSaveGame::SlotName : SlotNameOverride;`).
- `USiegeAccountSubsystem::GetDeckSlotName()` never reads it.
- `UDeckBuilderWidget`'s `ResolveDeckSlotName(GetGameInstance())` returns the bare guest constant for a world-free widget. That is 🧑 **his real guest slot**.

So no test drives `SetActiveDeckBySlot` or `SetActiveDeck` on a widget: it would write his real save. This is the file header's standing reason. I did not add a deck-slot seam, because it would have meant editing `SetActiveDeck`'s body, which is outside this row's fence.

**What carries the limbs this lane can't host (declared, not silent):**

| Limb | Why this lane can't | Carried by |
|---|---|---|
| Slate focus on a real bar slot | A `NewObject`'d builder never runs `NativeConstruct`, so it has no bar and `FindFocusedDeckBarSlot()` is `INDEX_NONE` by construction. An assertion of that could not fail (`SC-§39`), so none is made. | `TASK-1511` A1 (focused slot confirmed by `ui_snapshot`, then `IA_MenuSecondary`) and A2 (card tile focused, cold open). |
| `SetActiveDeck`'s own `SaveGameToSlot` on the real seam-resolved slot | It would write his real save (above). | A1's on-disk read. |
| Door 3's bar walk | Runtime-only. | A1's `IA_MenuRight` ×N with `ui_snapshot` after each step, plus the relay's Log line (§4). |
| His real `Home` key | Not rig-verifiable (`VER-§8` cl. 1). | A5, his hand check. |

---

## 7. Exec-symbol set

**No `UFUNCTION` was added. Both directions are empty:**
- Added reflected functions: none. `UFUNCTION(` count in `DeckBuilderWidget.h` is 35 before and 35 after. The only `UFUNCTION`/`UPROPERTY`/`GENERATED` match in the header diff is a comment line (`// ⛔ no UFUNCTION`).
- Removed reflected functions: none (0 removed lines).

⇒ 5a (`TASK-1510`) should find `DeckBuilderWidget.generated.h`'s exec-symbol set equal to the pre-change set. The new `static const TCHAR*` member has no `UPROPERTY`, which is the same shape as the six constants in `USiegeMenuInputSubsystem`.

Because the diff adds new member functions to a header, 5a needs a full compile. Live Coding is not enough (rule 5a / the row).

---

## 8. For QA (`TASK-1509`) to scrutinise

1. **The relay synthesizes a Slate key event from an Enhanced Input callback.** This is the one novel shape in the diff. My argument that it is safe is §4: the engine precedent, the navigation-key guard, and the enabled-button precondition. Please check the guard's reasoning against `SWidget::OnKeyDown` and `FSlateApplication::ProcessKeyDownEvent`'s bubble lambda (it skips a disabled widget).
2. **The relay sits after the precondition repair.** In a stale state (the model has a card armed, a mouse click put Slate focus on a bar slot), the repair re-focuses the tile first and door-3 Left/Right then walk the grid, not the bar. A real arrow would walk the bar. That divergence is pre-existing in kind (the repair already overrides door 3 in that state for Down, Up and Enter). An inject-only rig can't reach the state, since it needs a mouse click. I chose not to reorder or edit TASK-1423's repair.
3. **`Home` via door 3 goes through the same repair**, with the same stale-state caveat.
4. **Array index == slot index in `FindFocusedDeckBarSlot`** is the class's existing convention (see Not examined).
5. **One `Warning` path in the relay:** a non-navigation key. It cannot fire with the only two callers, `EKeys::Left` / `EKeys::Right` under the default config, so it guards a future config change.

---

## Not examined / limitations

- **Nothing compiled, nothing run.** Every runtime claim (the relay's landing, the key through each Slate door, the seventh binding resolving) is unverified until 5a/5b.
- **User index.** The relay addresses `GetUserIndexForController(LocalPlayer->GetControllerId())`, the user the builder's own focus calls address. A physical keyboard's event carries `GetUserIndexForKeyboard()`. For a single local player I expect both to be the default user, but I did not measure it. A1's per-step `ui_snapshot` will show whether the synthesized key walks the same user's focus.
- **Input preprocessors.** `ProcessKeyDownEvent` runs `InputPreProcessors.HandleKeyDownEvent` first. Whatever the editor/PIE (or Aura) registers there sees the synthesized arrow exactly as it would a physical one. I did not enumerate which preprocessors are live in the verifier's session.
- **Degenerate bar build.** If `CreateWidget<UDeckSlotEntryWidget>` ever fails for slot N, `NativeConstruct`'s `continue` shifts later entries down one array position. `FindFocusedDeckBarSlot` would then return a shifted slot, while right-click (bound to the entry's own `SlotIndex`) would stay correct. `RefreshDeckBarStates` and `ResolveGridExitFocusTarget` already share this assumption. I kept the one convention and did not fix it here, because the entry's `SlotIndex` is private to a class this row may not edit.
- **D8 / "Play with this deck".** Not touched. The dispatch cites the `TASK-671` comment in `SetActiveDeck` ("this one site covers SetActiveDeckBySlot's right-click lane AND the shipped D8 'Play with this deck' activation"). However, `DECK-§4(c)` records that `OnPlayClicked`'s `SaveDeckAs`/`SetActiveDeck` nodes were CUT, so Play now starts the match with the active deck and no longer activates. That comment's D8 clause may describe a path the WBP no longer has. I did not open `WBP_DeckBuilder` (no `.uasset` reads in this lane). Either way, the key goes through `SetActiveDeckBySlot`, the right-click function, and does not touch D8.
- **Enter / Up / Down on a bar slot via door 3** are unchanged and still move nothing on the bar. The row asked only for Left/Right. A real Enter on a slot still reaches `SelectDeckForEdit` through `SButton`, untouched.
- **Gamepad D-pad through door 3.** `IA_MenuLeft`/`Right` carry whatever keys `IMC_MainMenu` maps. The relay always hands Slate `EKeys::Left`/`Right` (the relay's existing table keys), so a pad press via door 3 walks with keyboard genesis. Controller-genesis-specific navigation behaviour (`GSlateEnableGamepadEditorNavigation`) is therefore not exercised on this lane.
- **`TASK-1508`'s asset** (`IA_MenuSecondary` + the two IMC rows) is referenced, not verified. Its absence degrades only this key, and the bind line names it.
- **`DECK-§9` table text** in `CONVENTIONS.md` does not yet list the new row. That is the manager's edit (`CONVENTIONS.md` is forbidden to this row). The two door addresses it cites moved +17 (§5).
</content>
</invoke>
