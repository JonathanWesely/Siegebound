# QA Report — TASK-1290 (gate over TASK-1286)
Verdict: PASS — 0 BLOCKER · 5 WARN · 6 NIT
subject: TASK-1286
reviewer: qa-reviewer · date: 2026-09-17 · gate marker `TASK-1290-DECK-KEYBOARD-GATE` · host `TASK-1291`

> ⚠️ **WHAT THIS VERDICT IS AND IS NOT.** I hold no `Bash` and no Unreal MCP write lane. Nothing here is a compile
> result, a hash, or an executed test count — `TASK-1291` re-measures all three. What IS measured, by me, this
> session: the three changed C++ files as they sit on disk, the UE 5.8 engine headers/sources at every line the
> handoff cites, and — through the **read-only** `unreal_inspector` against the running editor (PID as launched by
> others; I started, stopped and recompiled nothing, and mutated no asset, graph or setting) — the `WBP_DeckCardTile` /
> `WBP_DeckBuilder` CDOs and graphs. Each finding below says which of the two it is.

---

## Verdict at a glance

| check | result |
|---|---|
| (1) deliverable (0) — measurement quoted, route named/justified | **PASS** — route **A**, and the route-B refusal rests on a line I read myself (`GameViewportClient.cpp:767`). Ruling on the disclosed unmeasured half: **§1 below — it satisfies the check.** |
| (2) same entry point as `+` / `−`; button bodies diff 0 | **PASS — MEASURED on the live graphs**, not accepted on the handoff's word |
| (3) key fences (no letter/digit, `KBD-§` diff 0, `Tab`, no second meaning, no `IMC_`) | **PASS with WARN-1/WARN-2** — the prescribed `Gamepad_FaceButton_Right` **is** Slate's Back gesture; that is the row's own default, not a substitution |
| (4) focus visual (`DECK-§5` (e)) | **WARN-4** — "it paints" is measured; "it reads over card art" is correctly **not** claimed ⇒ WARN, not a pass |
| (5) the one test, on a non-real slot, STATE-asserting | **PASS** — the no-write guarantee verified END-TO-END IN THE CALL GRAPH; the `SetSlotNameForAutomationTests` absence is real (I re-ran the census) |
| (6) UE 5.8 API names at the headers | **PASS** — 14 symbols read at the line; 0 deprecated, 0 removed, 0 signature mismatches |
| (7) `.uasset` discipline (`SC-§68`) | **PASS** — **zero** `.uasset` in the diff ⇒ nothing to hash |
| (8) `git status` = only the named files | **ACCEPTED-AS-DECLARED** (`SC-§71b`) — handoff §3 matches the orchestrator's independent porcelain; `TASK-1291` re-measures |

---

## (1) Deliverable (0) — THE MEASUREMENT AND THE ROUTE · PASS

**What the handoff claims, and what I re-measured myself (read-only inspector, live editor):**

| claim (handoff §1) | my independent read | agrees? |
|---|---|---|
| `UButton` CDO is focusable ⇒ the tile's three runtime-built buttons ARE focusable ⇒ `TASK-1274` hypothesis (a) **refuted** | `unreal.Button` CDO **`IsFocusable = True`** | ✅ |
| the tile ROOT is not a focus stop | `WBP_DeckCardTile_C` CDO **`bIsFocusable = False`** | ✅ |
| `AddBtn` / `RemoveBtn` / `Btn_CardFace` are **BP variables built at runtime**, not design-time widgets | the tile's own graph: `GenericCreateObject Class=Button` → `Add Btn` / `Remove Btn` variables, labels `"+"` / `"-"` | ✅ |
| grid order == `GetCollectionCardIDs()` order; tiles in a `WrapBox` inside a `ScrollBox` | `WBP_DeckBuilder::Construct` — `GetCollectionCardIDs` → For Each → `CreateWidget(WBP_DeckCardTile_C)` → `AddChildToWrapBox` → `SetupCell(Owner, InCardID)` → `Array_Add(Card Tiles)`; `SplitGrid` creates the `ScrollBox` `Grid Scroll` and `Construct` does `AddChild(Grid Scroll, WrapBox)` | ✅ **no filter, no re-sort** |
| `SObjectWidget::SupportsKeyboardFocus` asks the `UUserWidget` **live** ⇒ `SetIsFocusable(true)` works on a built tile | `SObjectWidget.cpp:175-182` verbatim; `UserWidget.cpp:2411-2414` `return bIsFocusable;`; `:2421-2425` setter + `Invalidate(Paint)` | ✅ |
| `UserWidget.h:1030`'s "not modifiable at runtime" is about the deprecated **property**, not the setter | `:1030` is `UE_DEPRECATED(5.2, "Direct access to bIsFocusable…")` on the property; `IsFocusable()` `:1102` and `SetIsFocusable()` `:1104` are public, `UMG_API`, **not deprecated** | ✅ |
| route B is inert in his hands (`FInputModeUIOnly` ⇒ Enhanced Input never sees the key) | `GameViewportClient.cpp:767` — `if (IgnoreInput()) { return ViewportConsole ? … : false; }` | ✅ |

**⚖️ RULING ON THE DISCLOSED UNMEASURED HALF — *"does Slate directional navigation from the deck bar reach a tile?"*
The disclosed partial SATISFIES check (1) for this route-A diff. Put on the record, because a future reader will hit
this gap.** Three reasons, each tested rather than taken:

1. **The row's BLOCKER condition is the opposite case.** It reads *"a route-B diff with no measurement showing Slate
   cannot reach the grid"* — i.e. it exists to stop the **expensive** route being prescribed away unmeasured
   (`SC-§101`). This is a route-A diff that went **stricter than the row allowed** (no `IA_`, no new `.uasset`, not
   even the permitted `WBP_DeckCardTile.uasset` flag edit). There is no cheaper route the missing measurement could
   have revealed.
2. **The argument that the answer cannot change the route holds on three independent limbs**, and I checked each:
   (i) **wrap** — deliverable (1) demands "wrap at the ends"; Slate's default rule is `Escape`, wrap requires an
   explicit `EUINavigationRule::Wrap`, and the tiles are created at CDO defaults with a null `Navigation` object
   (measured: no `Navigation`/`Focus*` override anywhere in the three `.uasset`s, and the tiles are graph-created), so
   default navigation does **not** ring-wrap; (ii) **the focus unit** — deliverable (1) demands "the tile as the focus
   unit", and my own `UButton` CDO read (`IsFocusable = True`) is precisely what makes default navigation stop
   **three times per tile**; (iii) **Remove** — deliverable (2) demands a Remove gesture, and Slate's default
   `FNavigationConfig` has only `Accept` and `Back` (`NavigationConfig.cpp:32-38`); there is no remove action to reach.
   Any answer to the unmeasured question leaves all three unmet.
3. **The question is moot after the diff, and by construction.** The diff does not bypass Slate navigation — it *uses*
   it (`FSlateApplication::SetUserFocus(…, EFocusCause::Navigation)`), and it makes the tile a focus stop itself. So
   the post-diff answer to "can focus reach a tile" is *yes, deliberately*; the pre-diff answer is of historical
   interest only. What genuinely remains unproven at runtime is a **different** sentence — "does a real key event
   reach `UDeckBuilderWidget::NativeOnKeyDown` on that screen" — and that is the verify leg's job (see *Notes*).

**No WARN is owed here either:** the row's WARN condition is "a measurement asserted without the property values read
back". The values were read back, and I read them back a second time.

---

## (2) THE ENTRY POINT · PASS — measured, not accepted

The row's whole safety argument, closed on the live graphs rather than on the handoff's sentence:

- `+` button → `OnAddPressed` → `IsValid(Owner Builder)` → **`AddCopy(Card ID)`**  (WBP_DeckCardTile graph)
- `−` button → `OnRemovePressed` → `IsValid(Owner Builder)` → **`RemoveCopy(Card ID)`**
- keyboard: `AcceptFocusedCard()` → **`AddCopy(CardID)`** — `DeckBuilderWidget.cpp:1246`
- keyboard: `RemoveFocusedCard()` → **`RemoveCopy(CardID)`** — `DeckBuilderWidget.cpp:1265`

⇒ the **same two `UFUNCTION`s**, so the unknown-row refusal, `OnDeckSlotCountChanged`, `OnDeckModelChanged` and the
`DECK-§4` auto-save funnel run unchanged. ⛔ Nothing is re-implemented, no "keyboard variant" exists.

**The `+` / `−` bodies are diff-0**: they are Blueprint graph nodes inside `WBP_DeckCardTile.uasset`, and `git status`
carries **no `Content/` entry at all**, so they cannot have changed. The C++ `AddCopy` (`:792-835`) and `RemoveCopy`
(`:837-868`) bodies read as the `DECK-§4`/`UNCAP-§4` originals with no TASK-1286 content, and the handoff's hunk
geometry is internally consistent with the file on disk (6 new include lines in 4 hunks + 9 constructor lines = the
15-line shift that turns old `855` into new `870`, the insertion point being the line after `RemoveCopy`'s closing
brace). The hunk headers themselves are ACCEPTED-AS-DECLARED (no `Bash`).

---

## (3) KEY FENCES · PASS (with WARN-1 / WARN-2)

**My own census** (`EKeys::` across all three changed files, not the handoff's list) — **17 literals, all in
`DeckBuilderWidget.cpp`**, identical to the handoff's:

`Left`·`Right`·`Up`·`Down` (`:1144`,`:1148`,`:1152`,`:1156`) · `Gamepad_DPad_{Left,Right,Up,Down}` ·
`Gamepad_LeftStick_{Left,Right,Up,Down}` · `Enter`·`Virtual_Accept`·`Gamepad_FaceButton_Bottom` (`:1279`) ·
`Delete`·`Gamepad_FaceButton_Right` (`:1287`).

- ⛔ **zero letters, zero digits** ⇒ `KBD-§4`'s table (all 26 letters; digits/punctuation/`Enter`/`Escape` excluded
  *by design*, `CONVENTIONS:2519-2522`) is never entered. The pre-image had **no** key handling in this class, so the
  file census **is** the diff census.
- ⛔ **`Delete` / `Gamepad_FaceButton_Right` stand unchanged** — the spec default, and 🧑 named no alternative
  (`SC-§97`). **No substitution occurred.**
- ⛔ `Tab` absent · ⛔ no `IA_`/`IMC_`/`EnhancedInput`/`BindAction` **code** (the only `IA_` strings in the diff are
  comments explaining why there is none) · ⛔ `DECK-§3`'s right-click gets no keyboard twin.
- **What each bound key meant before** (the fence's disclosure half):

| key | previous meaning on this screen | after |
|---|---|---|
| `Enter` / `Virtual_Accept` / `Gamepad_FaceButton_Bottom` | Slate `Accept` — activates a focused **button** | unchanged everywhere; on a focused **tile root** (not a button) Accept did nothing before, now = `AddCopy`. `SButton::OnKeyDown` handles Accept and never bubbles, so `Enter` on a deck-bar slot still reaches `SelectDeckForEdit` — the `IsCardGridFocusLive()` gate makes this belt-and-braces |
| `Delete` | nothing | `RemoveCopy`, grid-focused only |
| arrows / D-pad / left stick | Slate directional navigation | consumed **only while the grid is focused**; outside the grid only `Down` is consumed (the entry affordance) |
| **`Gamepad_FaceButton_Right`** | **Slate `Back`** (see WARN-1) | `RemoveCopy`, grid-focused only |

---

## Findings

### WARN-1 — `DeckBuilderWidget.cpp:1287` — the handoff's *"⛔ No existing key gains a second meaning"* is **not accurate** for the gamepad remove key
Measured: `EKeys::Virtual_Back = FPlatformInput::GetGamepadBackKey()` (`InputCoreTypes.cpp:424`) and
`GetGamepadBackKey() { return EKeys::Gamepad_FaceButton_Right; }` (`GenericPlatformInput.h:32-35`); Slate's default
`FNavigationConfig` maps that virtual key to `EUINavigationAction::Back` (`NavigationConfig.cpp:38`). So on Windows
**B *is* the Back/Cancel gesture**, and while a tile holds focus this diff consumes it (`FReply::Handled()`) for a
**destructive, immediately auto-saved** `RemoveCopy` — Back never fires.
**This is NOT a defect in the diff and NOT a blocker:** `Gamepad_FaceButton_Right` is the row's prescribed default and
🧑 declined to name an alternative, so `SC-§97` forbids any agent substituting it. The finding is against the
**sentence**, which a future reader would otherwise trust. Fix = amend the sentence (and see WARN-2 for the substance).

### WARN-2 — `DeckBuilderWidget.cpp:1270-1319` — a **gamepad** user who enters the card grid cannot leave it, and their natural "back" press deletes a card
Once armed, all four directions are consumed and `StepCardFocusIndex` wraps (the row *ordered* "wrap at the ends"), so
there is no directional exit; `IsCardGridFocusLive()` only goes false when something else takes focus.
- **Keyboard has an unintended but real escape:** `Tab` is in neither table, so `NativeOnKeyDown` falls through to
  `Super` → Slate's `Next` navigation moves focus off the tile, after which every key behaves as it did yesterday.
- **Gamepad has none** — and its one plausible "get me out" button is bound to Remove (WARN-1).
Not a blocker (it is the spec's own wrap requirement, and the programmer named it in handoff §8.4 rather than hiding
it). **Suggested follow-up row** (⛔ not a silent edit here, and ⛔ not this row's to make): `Escape` and/or the gamepad
Back **when the grid is armed** clears the card focus and returns focus to the deck bar. `Escape` is not a letter or a
digit and does not enter `KBD-§4`.

### WARN-3 — `DeckBuilderWidget.cpp:1126-1130` — `IsCardGridFocusLive()`'s second fallback returns **true** on a live-Slate/no-tile state, and its stated justification does not hold
The comment justifies `return true` as "no live grid to corroborate against (an offline widget)". But the **first**
fallback (`!FSlateApplication::IsInitialized()`, `:1121-1124`) already owns the offline lane, and — decisively —
`IsCardGridFocusLive()` is reachable **only** from `NativeOnKeyDown`, which the new automation test never drives (the
programmer says so himself in `[AUDIT] §9`: it is `protected` and unreachable without a subclass). So the second
fallback is not needed by anything that runs today, and what it buys instead is: if `CardTileClass` ever fails to
resolve in a live menu (one `Warning`, `:961-968`), `Delete` still removes a card **with no outline on screen** and the
`DECK-§4` funnel saves it. The programmer asked for this judgement explicitly (§8.2) — I disagree with it.
**One-line remedy, if the manager wants it closed:** when `FSlateApplication::IsInitialized()` and no tile resolves,
`return false`. It cannot affect the suite (see above). Bounded today: reaching the state needs
`/Game/UI/WBP_DeckCardTile` to stop resolving.

### WARN-4 — `DECK-§5` (e), the focus visual — "it PAINTS" is measured; "it READS over card art" is **not claimed**, so this is a disclosure, not a pass
The handoff names **which** visual and cites it: Slate's dashed `FocusRectangle` = `FAppStyle` `"Old/DashedBorder"`,
white @ 50 % alpha, drawn around the **whole tile**. I verified the mechanism at the lines —
`SWidget.cpp:1744-1751` (`#if PLATFORM_UI_NEEDS_FOCUS_OUTLINES` → `if (bCanSupportFocus && SupportsKeyboardFocus())`
→ `ShowUserFocus` → `GetFocusBrush()`), `SWidget.cpp:1012-1014` (the brush name). It also states plainly that whether
it *reads* over that card's art is a pixel judgement it did not make and left to 🧑 his eye (§7.4).
Per this gate's check (4) that is **WARN-shaped, not a pass** — and `DECK-§5` (e)'s own words are that
render/interaction correctness *"closes on PIXELS/Jonathan, never tree/property readback"*. ⇒ **the verify leg must
capture a still of a focused tile, and Jonathan's eye is the acceptance.** (Scope note: `DECK-§5`'s five conditions are
written for `UDeckSlotEntryWidget`; the row imports (e)'s pixel-closure principle, which is the right import.)

### WARN-5 — handoff §1a — *"`WBP_DeckCardTile_C:WidgetTree` holds exactly ONE subobject (`SizeBox_0`)"* is contradicted by a second read
A `get_asset_meta` WidgetTree read of that asset shows `Overlay_19 > SizeBox_0 > Btn_Jump` plus two `UI_Thumbstick_C` —
inherited touch-template residue, exactly the shape `WBP_DeckBuilder` carries (its own `Construct` casts
`Btn_Jump->GetParent()->GetParent()` to `Overlay` and collapses all three; the tile's `Construct` does the same and
carries the same `"…: root Overlay not found"` `PrintString`). So the enumeration behind that sentence was incomplete.
**The conclusion the sentence supports is unaffected and independently confirmed** (the three card buttons ARE
runtime-created BP variables at `UButton` CDO defaults; the tile ROOT is `bIsFocusable = False`), and **no line of the
diff depends on the subobject count** — `CollectCardTiles` deliberately walks the *live* panel hierarchy and stops at a
tile. Recorded so the sentence is not quoted forward as measured fact.

### NIT-1 — handoff §4 — "Three includes added" then lists six
The file has **six** TASK-1286 includes (`DeckBuilderWidget.cpp:7,8,11,13,14,24`), all alphabetical, all commented,
all in modules already in `PublicDependencyModuleNames` (`Slate` at `Build.cs:22`, `SlateCore` immediately after its
own ⚠️ comment block; `UMG`/`InputCore`/`CoreUObject` above). Prose slip only — and the six-lines-in-four-hunks count
is what makes the handoff's hunk arithmetic add up.

### NIT-2 — `DeckBuilderWidget.cpp:1059-1069` — the `TASK-1274` focus idiom is reused **almost** verbatim, and the two deltas are worth knowing
Against `SiegeMenuInputSubsystem.cpp:395-404` and `UWidget::SetUserFocus` (`Widget.cpp:738-754`), `FocusCardTile`
**omits `DelayedSlateOperations.CancelFocusRequest()` on the immediate-success path**. A deferred focus request left
pending by an earlier caller can therefore still apply at end of frame and pull focus off the tile. It also returns
`true` after queueing the deferred fallback where the precedent returns `false` — harmless, because every caller
ignores the return. `FSlateApplication::GetUserIndexForController(int32)` returns a plain `int32` here
(`SlateApplication.h:1822`; the `TOptional` overload is the 2-arg one), and `SetUserFocus` `:678` is bounds-safe via
`GetUser`, so an unmapped controller degrades to the deferred path rather than indexing out of range.

### NIT-3 — per-keystroke cost, named so nobody re-derives it
One arrow press walks the live panel tree up to **three** times (`IsCardGridFocusLive` → `FindTileForCard`,
`ResolveGridColumns`, then `SetFocusedCardIndex` → `FocusCardTile` → `FindTileForCard`), calls
`GetCollectionCardIDs()` ~4×, and calls `CardTileClass.LoadSynchronous()` once per walk. Over ~28 tiles in a menu this
is free, and `IsCardGridFocusLive` short-circuits on `INDEX_NONE` before any walk, so nothing runs while the grid is
not armed. `LoadSynchronous` after first load resolves through `FSoftObjectPath::ResolveObject` (a lookup, not a load).
Cheap future win: collect the tiles once per key event and pass the array down.

### NIT-4 — `DeckBuilderWidget.cpp:1072-1112` vs `:1163-1205` — column count comes from the TILE array, the index ring from the CARD-ID array
Safe today and I measured why: the `Construct` loop is 1:1 over `GetCollectionCardIDs()` with no filter and no re-sort,
so the arrays have the same length and order, and every index is taken modulo `CardCount` regardless. A future WBP-side
filter or sort of the grid would desync the two (`FindTileForCard`'s CardID match would keep the *outline* honest while
the *stepping* followed the model). Latent coupling, not a defect.

### NIT-5 — `Tests/SiegeDeckSlotsTest.cpp:1334` — the test now synchronously loads `/Game/UI/WBP_DeckCardTile`
`SetFocusedCardIndex` → `FocusCardTile` → `CollectCardTiles` → `CardTileClass.LoadSynchronous()` runs inside a lane the
file describes as "zero widget tree". It succeeds in `EditorContext`; if that asset is ever renamed the test emits one
unexpected `Warning`, which is noise rather than a failure (`FAutomationTestBase::bElevateLogWarningsToErrors = false`,
`AutomationTest.cpp:181`).

### NIT-6 — `DeckBuilderWidget.cpp:1023-1026` — `SetIsFocusable(true)` is never reverted
Every visited tile stays a Slate focus stop for the widget's lifetime, so `Tab` and mouse clicks can subsequently land
focus on a tile root. Cosmetic, bounded, and reverting it would fight the feature.

---

## (5) The test · PASS — and the departure from the file's own header is sound

**The `SetSlotNameForAutomationTests` absence is real — I re-ran the census rather than accepting `[AUDIT]`:** the
symbol exists on exactly two classes, `USiegeAccountSubsystem` (`SiegeAccountSubsystem.h:253`) and
`USiegeSettingsSubsystem` (`SiegeSettingsSubsystem.h:230`), and is called only from the account / cloud / settings /
graphics tests. `UDeckBuilderWidget` and `USiegeDeckSaveGame` have no such seam, and `SiegeDeckSlotsTest.cpp` has never
called it. ⇒ the row's **letter** is unsatisfiable without adding a seam the file's header calls a `DECK-§8` deviation;
declaring that (`SC-§101`) rather than inventing one was correct, and the row's **intent** — ⛔ never the real save — is
met by a stronger mechanism.

**I verified the no-write guarantee END TO END IN THE CALL GRAPH, not at the entry point** (⚠️ `DECK-§4` auto-save is
live and this is 🧑 his real deck):

1. `AcceptFocusedCard` (`:1235`) → `AddCopy` → on success `OnDeckSlotCountChanged` + `OnDeckModelChanged` (both
   `BlueprintImplementableEvent`, `DeckBuilderWidget.h:498/507` — **no-ops** on a `NewObject<UDeckBuilderWidget>`,
   which is the C++ class with no Blueprint graph) → `PersistWorkingDeck` (`:741`).
2. `PersistWorkingDeck`'s guard is `if (EditingDeckIndex < 0 || >= NumFixedDeckSlots) { UE_LOG(Warning …); return; }`
   (`:746-754`) — it returns **before** `SaveDeckAs` (`:756`), so `ResolveDeckSlotName` / `UGameplayStatics::
   SaveGameToSlot` are never reached and the `ACC-§4` seam is never resolved.
3. `EditingDeckIndex = INDEX_NONE` **at its declaration** (`DeckBuilderWidget.h:578`) and `NativeConstruct` — the only
   writer — never runs in the test. Asserted as a precondition at `SiegeDeckSlotsTest.cpp:1311`.
4. `RemoveFocusedCard` → `RemoveCopy` → the identical funnel.
5. The focus path writes nothing: `FocusCardTile` reads `GetCollectionCardIDs`, loads a widget **class**, and exits at
   `GetRootWidget() == nullptr` (`UserWidget.cpp:1356-1364`, null-safe) / `Tile == nullptr`.
6. The only other disk touch in the file is `FDeckScratchGuard`'s scratch slot, asserted absent at `:1382`.
⇒ **no path in this test can reach `SaveGameToSlot`.**

**The pin is a real tripwire, verified at the engine:** `AddExpectedMessagePlain(…, Occurrences 2)` +
`AutomationTest.cpp:1826` — `ExpectedNumberOfOccurrences > 0 && != ActualNumberOfOccurrences` ⇒ **test error**. So 0, 1
or 3+ refusals all fail. A future edit that lets the funnel through turns the test **RED** instead of writing his deck.
Exactly two successful mutations occur ((a) and (b)); the (c) no-op pair returns on `CardID.IsNone()` before any
mutation, so it adds no third.

**STATE, not call counts** (`SC-§104`): every assertion reads `GetCountOf` / `GetTotalCount` / `GetFocusedCardIndex` /
`GetFocusedCardID` / `DoesSaveGameExist`. Tile **K** is `Count/2` — a middle tile, so a bug that silently focuses index
0 cannot pass by coincidence. **Exactly one** new automation test
(`Siegebound.Deck.KeyboardFocusedTileAcceptAddsOneCopyAndRemoveTakesItBack`, `EditorContext | EngineFilter`).

---

## (6) UE 5.8 API — read at the header, not from memory

`UUserWidget::IsFocusable` / `SetIsFocusable` `UserWidget.h:1102/1104` (public, `UMG_API`, **not** deprecated — only
the `bIsFocusable` *property* at `:1030` is) · `NativeOnKeyDown` `UserWidget.h:1609` (signature matches exactly) ·
`GetRootWidget` `UserWidget.cpp:1356-1364` (null-safe) · `UWidget::HasAnyUserFocus` `:668` / `HasFocusedDescendants`
`:672` / `GetParent` `:769` / `GetCachedGeometry` `:788` / `GetCachedWidget` `:857` · `UPanelWidget::GetChildrenCount`
`PanelWidget.h:28` and `GetChildAt` `:36` (**both `const`**, which is what lets the walk run from a `const` method) ·
`UScrollBox::ScrollWidgetIntoView(UWidget*, bool, …)` `ScrollBox.h:321`, null-guarded `ScrollBox.cpp:248-256` ·
`FSlateApplication::SetUserFocus(uint32, const TSharedPtr<SWidget>&, EFocusCause)` `SlateApplication.h:678` ·
`GetUserIndexForController(int32)` → `int32` `:1822` · `ULocalPlayer::GetControllerId()` `LocalPlayer.h:522`
(**legacy-but-not-deprecated**, no `UE_DEPRECATED`) and `GetSlateOperations()` `:325` ·
`EUINavigation` is `UENUM(BlueprintType)` `SlateEnums.h:97-115` incl. `Invalid` (so the `UFUNCTION` parameter is
UHT-legal, and `Types/SlateEnums.h` in the header is the include that makes it so) · `EKeys::Virtual_Accept`
`InputCoreTypes.h:745`, `Gamepad_FaceButton_Right` `:517`, `Gamepad_DPad_Up` `:524`, `Gamepad_LeftStick_Up` `:530` ·
`TSoftClassPtr::LoadSynchronous() const` → `UClass*` `SoftObjectPtr.h:1008` (**`const`**, and it returns `nullptr`
rather than a bad cast when the class is not a `UUserWidget` child), `ToString()` `:972`, `explicit
TSoftClassPtr(const FSoftObjectPath&)` `:827` · `FGeometry::GetAbsolutePosition` `Geometry.h:540` /
`GetAbsoluteSize` `:548` (`FDeprecateVector2DResult`, so reading `.Y` as a `float` is correct and unambiguous).
**0 deprecated or removed APIs. 0 signature mismatches. Reflection and GC are clean** — `FocusedCardIndex` is
`UPROPERTY(Transient)` (an `int32`), the tile arrays are function-local raw pointers into a live tree, and
`bWarnedMissingTileClass` is a POD spam-guard.

## The `DECK-§6` save found en route · CONFIRMED REAL, and correctly fixed

- **The defect is real, measured twice:** `UScrollBox` sets `ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)`
  in its constructor (`ScrollBox.cpp:31`), and the live CDO reads `ScrollWhenFocusChanges = NO_SCROLL`. Without an
  explicit call, a focused tile below the fold wears an outline nobody can see — the present-but-unusable class
  `DECK-§6` exists to forbid (*"at ANY window size, every card tile is REACHABLE"*, `CONVENTIONS:6810`).
- **The fix reaches a real ScrollBox:** measured on the graphs, `SplitGrid` creates `Grid Scroll` (a `ScrollBox`) and
  `Construct` does `AddChild(Grid Scroll, WrapBox)`, so the ancestor walk `Tile → WrapBox → Grid Scroll` terminates on
  it. (`DECK-§6`'s own 2026-08-27 amendment already records `GridScroll`/`GridRow` as runtime variables set in
  `SplitGrid` — this matches.)
- **Blast radius: none.** `ScrollWidgetIntoView` only forwards a one-shot request to the Slate widget
  (`ScrollBox.cpp:248-262`, null-tolerant by design) — it mutates no property of the WBP-owned widget.
  `SetScrollWhenFocusChanges` would have rewritten that widget's configuration for every other focus change; declining
  it was the right call.

---

## Notes for build-master (`TASK-1291`) and the `playtest-verifier`

1. **ACCEPTED-AS-DECLARED, re-measure at 5a:** `git status --porcelain` (handoff §3 — 3 C++ files, no `Content/`, no
   `.uasset`, no `Saved/SaveGames/`, `CONVENTIONS.md`/`TASKBOARD.md` being the manager's `SC-§119`/R17 dirt for host
   `TASK-1293`); `git diff -- CONVENTIONS.md | grep -c 'KBD-§'` = 0; the `.cpp` hunk headers; the compile result; the
   suite count (**expect +1**). I have no `Bash` (`SC-§71b`).
2. ⚠️ **A green suite is NECESSARY AND NOT SUFFICIENT — the programmer says so and he is right.** Nothing in the suite
   drives `NativeOnKeyDown` (it is `protected`), so **nothing proves `Enter` reaches `AddCopy` or `Delete` reaches
   `RemoveCopy`**, and nothing proves a real key event reaches this widget's key door at all. The whole key table,
   `NavigationFromKey` and the `IsCardGridFocusLive()` gate close **only** on the verify leg.
3. **Drive REAL keys.** There is no `IA_` for this feature by design — `inject_input_action` will not exercise it, and
   a report saying "no `IA_Deck*` mapping exists" is describing the design. Use `simulate_key_press`: `Down`, then
   `Enter`, then `Delete`.
4. **The observable reads `"Deck: n/50"`, not `"n/50"`** — measured in `RefreshAll` (`Concat "Deck: " + n + "/50"` →
   `TotalText`). Read it before and after; net zero mutations on his real deck, and say so.
5. **Measured for you, so an expected behaviour is not filed as a bug:** `OnDeckModelChanged → RefreshAll` iterates the
   existing `Card Tiles` array calling `RefreshCell` — it does **not** rebuild the grid. So the focus outline **should
   survive** an Accept. If it vanishes after one `Enter`, that is a NEW defect, not the design.
6. 🙋 **For Jonathan, three things this gate cannot settle:** (a) **`DECK-§5` (e)** — does the dashed white 50 %-alpha
   rectangle actually *read* over card art? A still of a focused tile is the cheapest evidence and his eye is the
   acceptance; (b) **WARN-1/WARN-2** — on a gamepad, **B removes a card** (it is Slate's Back button) and there is no
   way back out of the grid; on a keyboard `Tab` gets you out. Both follow from his own key choice and the row's
   "wrap at the ends", so they are his call, and a follow-up row (`Escape` leaves the grid) is the cheap remedy;
   (c) `Down` pressed anywhere else on the builder screen now jumps into the card grid — measured layout is
   Title → grid → totals → "Reset to Default" → "Play", with "Exit" floating bottom-right, so the old
   Reset→Play `Down` step is superseded (Up still walks back).
7. **Nothing in the diff is replicated** — no new replicated property/class/relevancy tier/RPC (M8 clean).
