# TASK-1521 — [SET-ACTIVE-REPEAT-FILTER] — gameplay-programmer handoff

- **Status set:** `ready-for-qa` → gate `TASK-1522` (QA) → 5a `TASK-1523` → 5b `TASK-1524` → host `TASK-1525` (commit D).
- **Date:** 2026-09-26. **No compile, no PIE, no git write, no `.uasset`** (row (5)). Git was used read-only (`git show HEAD:` / `git diff`) for the measurements below.
- **Authority:** 🧑 his answer to `qa/TASK-1509.md` W1, relayed by the orchestrator: *"Yes, fire once"*. A held `Home` / pad Y sets the active deck ONCE per physical press. His answer covered the set-active pair only.
- **Blockers at start (re-measured):** `TASK-1514` is committed (`0a5b8a7` is HEAD). `git diff --stat HEAD -- GitClaudeUnrealTest/Source` was empty before my first edit, so no `TASK-1480` edits were sitting in `DeckBuilderWidget.{h,cpp}` / `SiegeDeckSlotsTest.cpp`. This row holds those three files until host `TASK-1525` commits.

---

## 1. What changed

**Files touched (all on the row's WRITES list):**
- `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp`: the adapter's body and comment block, plus one new helper definition.
- `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h`: the helper's declaration (needed, because the test calls it) and two doc comments that would otherwise have become false.
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeDeckSlotsTest.cpp`: one include and one new test, appended.

**Diff shape (`git diff -U0`, removed lines counted by grep, not from `--numstat`):**
- 10 removed lines, all comments: 6 in the `.cpp` adapter comment block, 4 in the `.h` adapter doc.
- 0 removed lines in the test file.
- The test file stays LF-only (CR = 0, as at HEAD). The `.h` / `.cpp` stay CRLF (CR count == LF count: 1388/1388 and 3354/3354).

**New symbol (plain C++; no `UFUNCTION`, no `UPROPERTY`, no data member):**

| Symbol | Access | Role |
|---|---|---|
| `static bool IsHeldDeckBarActivationRepeat(const FKeyEvent& InKeyEvent, int32 FocusedBarSlotIndex)` | public (next to `ResolveDeckBarActivationSlot`) | The pure half of the adapter's one divergence: the testable decision. |

**Assets referenced:** none. The row touches no asset path.

---

## 2. The new branch, quoted

**(a) The adapter, `UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent& InKeyEvent)`** (`.cpp` ~:1861). Before this row it was one line, `return HandleCardGridKey(InKeyEvent.GetKey());`. It is now:
```cpp
FReply UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent& InKeyEvent)
{
	const int32 FocusedBarSlot = (InKeyEvent.IsRepeat() && IsDeckBarActivationKey(InKeyEvent.GetKey()))
		? FindFocusedDeckBarSlot()
		: INDEX_NONE;
	if (IsHeldDeckBarActivationRepeat(InKeyEvent, FocusedBarSlot))
	{
		return FReply::Handled();
	}

	return HandleCardGridKey(InKeyEvent.GetKey());
}
```
- The last line is the adapter's original line, character for character. Every event the filter does not claim runs exactly what ran before.
- Focus is read only for a repeat of the pair. So `FindFocusedDeckBarSlot` keeps its header contract: *"called ONLY for Home / pad Y (HandleCardGridKey) and Left / Right (door 3) — never on every key."* A first press and every other key's event pay nothing new.
- The comment block above the function now names the one divergence. It replaces the sentence *"There is NO behaviour here to diverge from the FKey body"*, as the row's (1) required.

**(b) The decision, `UDeckBuilderWidget::IsHeldDeckBarActivationRepeat`** (`.cpp` ~:2226, placed after `ResolveDeckBarActivationSlot`, below every line `DECK-§9` cites):
```cpp
	return InKeyEvent.IsRepeat()
		&& ResolveDeckBarActivationSlot(InKeyEvent.GetKey(), FocusedBarSlotIndex) != INDEX_NONE;
```
- The claim half is the (3) row's own test, read through the same resolver. The (3) row is `const int32 BarSlot = ResolveDeckBarActivationSlot(Key, FindFocusedDeckBarSlot());` then `if (BarSlot != INDEX_NONE)`.
- The pair is still spelled once. `IsDeckBarActivationKey` is read inside the resolver, and the adapter's pre-check calls that same function. No second key table exists (row (1)).

**(c) Door 3: nothing needed, as the row expected.** `IA_MenuSecondary` is bound `ETriggerEvent::Started` in `BindMenuNavActions`, which is unchanged. `HandleMenuNavSecondary` → `RouteMenuNavKey(EKeys::Home, …)` → `HandleCardGridKey(Key)` enters the FKey body directly, never through the adapter, and carries no `FKeyEvent`, so it never carries a repeat. The relay's synthesized Left/Right events are built with `/*bIsRepeat=*/ false` and are not the pair, so the filter never touches them.

---

## 3. Why the claim set is exactly the first press's, with no new fall-through and no new absorb

Let **S** be the state *"`IsRepeat()` is true, the key is `Home` or `Gamepad_FaceButton_Top`, and `FindFocusedDeckBarSlot()` returns a slot in [0, 10)"*.

- **Outside S**, the adapter runs its original line. The reply and the effect are byte-for-byte today's, for every first press and for every repeat. That includes a pair repeat with no bar slot focused: the body declines it at `(3)`, as it declines the first press there.
- **Inside S**, the adapter returns `FReply::Handled()` and calls nothing. For the same key in the same state, the FKey body (unchanged, see §4) also returns `Handled`, for three reasons:
  1. `(1)` is gated on `IsCardGridFocusLive()`. That is false while a bar slot holds focus: a single user's focus path cannot hold a card tile and a deck-bar `SlotButton` at once, because they sit in different subtrees. Even if it ran, none of its keys is `Home` or `Gamepad_FaceButton_Top`. On Win64, `Virtual_Gamepad_Accept`/`Back` resolve to FaceButton_Bottom/Right.
  2. `(2)`: `NavigationFromKey` has no entry for either key, so it returns `EUINavigation::Invalid`.
  3. `(3)` claims, because its gate `BarSlot != INDEX_NONE` is exactly S's focus condition.

  So in S the first press and the repeat both reply `Handled`. The only difference is that the first press calls `SetActiveDeckBySlot(BarSlot)` and the repeat calls nothing. The set of claimed states is identical, and a claimed repeat stops at the preview door exactly where the first press stops. It falls through to nothing the first press would not have reached.
- **User-index caveat (unchanged, `qa/TASK-1509.md` N2):** the filter reads the same any-user `FindFocusedDeckBarSlot` that the `(3)` row reads, so the two stay identical under any user mapping.

**What a repeat in S no longer does (the row's property):** no `SetActiveDeckBySlot`, therefore no `SetActiveDeck`. That means no `SaveGameToSlot`, no `OnDeckModelChanged`, no `RefreshDeckBarStates` (outline redraw), and on an illegal deck no refusal `Warning` and no `OnDeckActivationRefused`.

**Engine premise, checked at source (UE 5.8):** both real auto-repeat sources reach these doors with the bit set.
- Keyboard: `FSlateApplication::OnKeyDown(…, const bool IsRepeat)` builds `FKeyEvent KeyEvent(Key, …, GetUserIndexForKeyboard(), IsRepeat, CharacterCode, KeyCode);`.
- Pad: `FSlateApplication::OnControllerButtonPressed(…, bool IsRepeat)` builds `FKeyEvent KeyEvent(Key, …, InputDeviceId, IsRepeat, 0, 0, UserIndex);`. XInput calls it with `true` for held buttons (`XInputInterface.cpp`, `InitialButtonRepeatDelay = 0.2f` / `ButtonRepeatDelay = 0.1f`).

---

## 4. Row (2): unchanged, item by item

Method: each function body was extracted from `git show HEAD:` and from the working tree (brace-matched, CRLF normalised) and compared. Every hash below is sha256 before my first edit and after my last.

| Item | Result |
|---|---|
| Every other key's repeat behaviour, ⛔ `Enter` → `AddCopy` especially | ✅ **By argument and by test.** A non-pair key fails the adapter's pre-check, so the original line runs unchanged. The helper returns false for every non-pair key, repeat or not, at every focus probe (test (0)). `HandleCardGridKey(const FKey&)`, `AcceptFocusedCard` and `AddCopy` are IDENTICAL. |
| Right-click path | ✅ `DeckSlotEntryWidget.h` `d0245179…5cfc`, `.cpp` `bd145fac…d077`: unchanged. The `NativeConstruct` body (where `OnRightClicked` binds to `SetActiveDeckBySlot`) is IDENTICAL. |
| `SetActiveDeckBySlot` body | ✅ IDENTICAL. `SetActiveDeck`, `TryActivateSavedDeck` and `SelectDeckForEdit` are also IDENTICAL. |
| The two Slate doors' call sites `HandleCardGridKey(InKeyEvent)` | ✅ `NativeOnPreviewKeyDown` / `NativeOnKeyDown` bodies are IDENTICAL, with 2 code call sites before and after. Their definitions moved +29 lines (`:1845`→`:1874`, `:1867`→`:1896`) because the adapter above them grew (see §7). |
| Door 3 | ✅ `HandleMenuNavSecondary`, `RouteMenuNavKey`, `BindMenuNavActions`, `UnbindMenuNavActions` and `RelayDeckBarNavigationKeyToSlate` are IDENTICAL. |
| `IsDeckLegal` | ✅ `DeckLibrary.h` `56cee095…0efc`, `.cpp` `1748908e…51e3`: unchanged. |
| Every `DECK-§9` row | ✅ `.cpp` lines 1..1833 are IDENTICAL to HEAD (measured `True`). The first hunk starts after the FKey body's closing brace, so every cited site and address is unmoved: Accept, Remove, Exit, move dispatch, enter-grid, `NavigationFromKey`, and the `(3)` set-active block (`if (IsDeckBarActivationKey(Key))` → `SetActiveDeckBySlot(BarSlot);`). `HandleCardGridKey(const FKey&)` and `NavigationFromKey` are IDENTICAL. |
| `Tab` absent | ✅ `EKeys::Tab`: `.cpp` 0 → 0, `.h` 0 → 0. |
| `Escape` scope | ✅ `EKeys::Escape`: `.cpp` 5 → 5, `.h` 2 → 2. The exit line is inside the identical prefix. |
| No letter or digit | ✅ The added production lines name no `EKeys` at all. The only new key references are the calls `IsDeckBarActivationKey(…)` / `InKeyEvent.GetKey()`. `EKeys::Home` stays 3 → 3 and `EKeys::Gamepad_FaceButton_Top` 1 → 1. The test names `EKeys::H` / `EKeys::One` only as negative probes that must NOT be filtered. |
| `IsDeckBarActivationKey` is the pair's one spelling | ✅ Its body is IDENTICAL. The new code calls it and never restates the pair. |
| `SiegeMenuInputSubsystem.{h,cpp}` | ✅ **zero bytes**: `.h` `e4ad7ecf…4c77`, `.cpp` `9cb571d1…942d`, unchanged. |
| Also checked, not on the list | `ResolveDeckBarActivationSlot`, `FindFocusedDeckBarSlot`, `IsCardGridFocusLive`, `ExitCardGridFocus`: IDENTICAL. `WasInputKeyJustPressed(` call sites 0 → 0. |

---

## 5. Tests added (`Tests/SiegeDeckSlotsTest.cpp`)

**`Siegebound.Deck.HeldSetActiveKeyFiresOncePerPress`** (`FSiegeDeckHeldSetActiveKeyFiresOncePerPressTest`), EditorContext | EngineFilter. The existing `Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes` is untouched: 0 removed lines in the file.

Every assertion reads state (`SC-§104`): the filter's answer, a per-event outcome, `ActiveDeckName` in memory, or `ActiveDeckName` on the scratch slot. Nothing counts calls. The 30 in the hold loops is W1's rate (~30 repeats per second) used to size the hold; no assertion counts anything.

- **SELF-CHECK:** `MakeSlateKeyEvent(Home, true).IsRepeat() == true`, `(Home, false).IsRepeat() == false`, and `GetKey() == Home`. A misplaced constructor argument would otherwise make the rest vacuous. The event is built the way `FSlateApplication::OnKeyDown` builds one, with a `uint32` user index so the non-device-id constructor is an exact match.
- **(0) THE DECISION, on real `FKeyEvent`s:**
  - **PREMISE:** for `Home` and `Y` over the focus domain {`INDEX_NONE`, −2, 0..9, 10}, the `(3)` row claims the first press iff a bar slot is focused.
  - **CLAIM SET:** over that same domain, a REPEAT is filtered iff the first press is claimed by `(3)`. The claim set is equal, never wider and never narrower. This fails a filter that ignores focus, and one that filters nothing.
  - A **FIRST** press is never filtered, at any focus. This fails a filter that ignores `IsRepeat()`.
  - **17 other keys** are never filtered, as a repeat or as a first press, at focus {`INDEX_NONE`, 0, 3, 9}. `Enter` comes first in the list. The others are SpaceBar, Left/Right/Up/Down, Escape, Tab, Delete, BackSpace, End, pad A/B/X, `Gamepad_DPad_Down`, `H` and `One`. This fails a filter that claims any repeat.
- **(1) THE EFFECT**, on a migrated in-memory ten-slot save with a transient table (the TASK-1270 idiom). deck1 and deck4 are legal (50 Footman each), deck5 is empty, and deck1 is active. Persists go to **`SiegeDecks_AutomationScratch` only**, asserted ≠ `USiegeDeckSaveGame::SlotName`, with `FDeckScratchGuard` deleting it on the way in and out. The helper `SendSetActiveKeyEvent` composes the shipping route's halves in order: the filter, then the resolver, then `TryActivateSavedDeck`, then a scratch persist on success only.
  - **(a) Hold `Home` on legal deck4.** The press gives `Activated`; memory and scratch both read `"deck4"`. Then an **intervening** mid-hold right-click on deck1 (its in-memory step plus a scratch persist): memory and scratch both read `"deck1"`. Then 30 repeats: each one is `ClaimedCalledNothing`. After the hold, memory still reads `"deck1"`, **scratch still reads `"deck1"`**, and no deck's content moved. ⭐ **This is the discriminating state.** An unfiltered route would re-set deck4 and re-write the file on the first repeat. A content-only check cannot tell the two apart, because the repeated write is identical (W1: no data lost).
  - **(b) A NEW press after the release** gives `Activated`; memory and scratch read `"deck4"` again. The filter fires once per press, not once ever.
  - **(c) Hold pad Y on empty deck5.** The press gives `Refused`, with IsDeckLegal's `"exactly 50"` reason: the hold's one refusal. Each of 30 repeats is `ClaimedCalledNothing` with an **empty** refusal, so no `Warning` and no `OnDeckActivationRefused`. `ActiveDeckName` stays `"deck4"` in memory and on scratch, and the save deep-equals the post-(b) state.
  - **(d) Hold `Home` with no bar slot focused.** The press gives `NotClaimed`, and each of 30 repeats is also `NotClaimed` (no new absorb). The save and scratch are unchanged.
  - **(e) Pad Y's first press on legal deck1** gives `Activated`; memory and scratch read `"deck1"` (the Y positive control).

**Mutations these tests should fail on (reasoned, not executed; nothing compiled):**
- No filter: (0) CLAIM SET fails on slots 0..9, and (1a) reads `"deck4"`.
- `IsRepeat()` ignored: the (0) first-press rows fail, and (1a)'s press is not `Activated`.
- Focus ignored: (0) CLAIM SET fails at `INDEX_NONE`/−2/10, and (1d)'s repeats are claimed.
- Key ignored: the (0) other-key rows fail.

**What carries what this lane can't host (declared, not silent):**

| Limb | Why the unit lane can't | Carried by |
|---|---|---|
| A real Slate auto-repeat through `NativeOnPreviewKeyDown` with a bar slot focused | A `NewObject`'d builder has no bar, so `FindFocusedDeckBarSlot()` is `INDEX_NONE` by construction (`SC-§39`: no such assertion made). No rig here generates a Slate key-repeat (`VER-§8` cl. 1). | `TASK-1524` A3, 🧑 his hand check (hold Home on deck5: one refusal line, not a run) |
| The adapter's wiring (that it calls the filter and returns `Handled`) | Same: the adapter is private and needs Slate focus to reach the branch. The effect helper composes the halves and does not call the adapter (the `qa/TASK-1509.md` N5 shape). | `TASK-1522`'s read of the nine added lines |
| The first press through door 3 still working, and the refusal once per press | Runtime | `TASK-1524` A1/A2 |
| `SetActiveDeck`'s persist to the real seam-resolved slot | It would write his real guest save (`qa/TASK-1509.md` Point 4) | `TASK-1524` A1's disk read |

---

## 6. Exec-symbol set / `UFUNCTION`

**No `UFUNCTION` was added. Both directions are empty.** `UFUNCTION(` in `DeckBuilderWidget.h` is 35 → 35, `UPROPERTY(` 8 → 8, `GENERATED_` 1 → 1. No reflected function was removed (0 removed code lines). The new member is a plain `static` function with an `FKeyEvent` parameter, which UHT ignores. No data member was added, so the object layout is unchanged.

⇒ **5a (`TASK-1523`):** `DeckBuilderWidget.generated.h`'s exec-symbol set should be equal to the pre-change set. **The header changed** (a new member function declaration), so UHT re-runs. **Full `Build.bat` with the GUI editor closed; ⛔ never Live Coding.** Suite delta +1 by name: `Siegebound.Deck.HeldSetActiveKeyFiresOncePerPress`. `Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes` must stay green. The only disk touch is the scratch slot.

---

## 7. For QA (`TASK-1522`) to scrutinise

1. **The claim-equality argument (§3)** leans on three things. Please hold me to each of them:
   - `IsCardGridFocusLive()` and a focused bar slot are mutually exclusive for one user.
   - `NavigationFromKey` has no entry for either key.
   - The FKey body is IDENTICAL. Even without the first point, the `(1)` keys don't include the pair on Win64, and every exit before the tail is `return FReply::Handled();`.
2. **The adapter's pre-check `InKeyEvent.IsRepeat() && IsDeckBarActivationKey(…)` repeats half of the helper's test.** It exists so focus is read only for a repeat of the pair (the `FindFocusedDeckBarSlot` header contract). It is a call to the one spelling, not a second table. Walks of the ≤10 bar entries per event: a claimed pair repeat does 1 (the adapter); a pair first press does 1 (the body's `(3)`, as before); an unclaimed pair repeat (no bar focus) does 2 (the adapter, then `(3)` again); every other key does 0.
3. **BLOCKER criterion (1)** (no writer reached): the claimed branch is `return FReply::Handled();` with nothing before it but the two reads (`FindFocusedDeckBarSlot` is `const`; the helper is `static` and pure).
4. **Address drift (N7's kin, for the manager, not a violation):** the adapter grew +29 lines, so the two Slate door definitions moved `:1845`→`:1874` and `:1867`→`:1896`. `DECK-§9` cl. 10's "Addresses" bullet cites the old pair. Every `DECK-§9` table site sits above the hunk and is unmoved. Cite by function (cl. 1). `DECK-§9` cl. 10's W1 bullet (*"recorded as OPEN"*) is the manager's to update when this ships (lane K2 header).
5. **`qa/TASK-1509.md` N6** (the two stale "six" `IA_Menu*` counts in the header) is **not** touched here. It is boarded on `TASK-1480` (h), and folding it in would widen this diff past the row's `.h` allowance.

---

## Not examined / limitations

- **Nothing compiled, nothing run.** The tests' outcomes are reasoned, and so are the mutation claims in §5. 5a is the first execution.
- **A real held key was not observed.** That the OS / XInput repeat reaches the preview door with `IsRepeat()` true is read from engine source (§3), not measured. `TASK-1524` A3 (his hands) is the only instrument for it.
- **Platform scope of §3's "none of `(1)`'s keys is the pair":** checked for Win64's `FPlatformInput::GetGamepadAcceptKey`/`GetGamepadBackKey` (FaceButton_Bottom/Right). The claim-set equality does not depend on it (every `(1)` exit is `Handled`), but "no other effect is suppressed" does, on a hypothetical platform whose Back key is FaceButton_Top.
- **Repeat semantics of other input stacks** (Steam Input, GameInput, RawInput) were not traced beyond seeing that each calls `OnControllerButtonPressed(…, true)` for repeats. They reach the same `FKeyEvent` constructor.
- **The multi-user gap (`qa/TASK-1509.md` N2)** is inherited unchanged. The filter reads the same any-user focus test as the `(3)` row, so the two agree, but neither is per-user.
- **`CONVENTIONS.md`** (`DECK-§9` cl. 10's W1 bullet and door addresses) is the manager's and is untouched (row (5)).
