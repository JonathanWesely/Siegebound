Verdict: PASS — 0 BLOCKER · 2 WARN · 4 NIT
subject: TASK-1286
delta-vs: qa/TASK-1290-report.md
reviewer: qa-reviewer · date: 2026-09-17 · gate marker `TASK-1299-DECK-KEYBOARD-REGATE` · host `TASK-1291`

> ⚠️ **WHAT THIS VERDICT IS, AND WHAT IT IS NOT.** I hold no `Bash` and no Unreal MCP write lane. Nothing below is a
> compile result, a hash, or an executed test count — `TASK-1291` re-measures all three (`SC-§71b`). What IS measured
> by me this session: the three changed C++ files as they sit on disk (census run by me, never read off the handoff),
> the UE 5.8 engine headers at every line the delta newly depends on, and — through the **read-only**
> `unreal_inspector` against the already-running editor — `WBP_DeckBuilder`'s event list, as an independent
> corroboration of (G1)'s Blueprint lane. ⛔ I started, stopped and recompiled nothing, and mutated no asset, graph or
> setting.

---

## ⛔ (0) SCOPE — WHAT THIS ROW DID NOT RUN, AND WHY

`qa/TASK-1290-report.md` was read **whole, first**. **My subject is the DELTA between the diff `TASK-1290` passed and
the files on disk now** — amendment 1 (Remove B→X, the new B/`Virtual_Back` grid exit, the (C) back-precedence guard,
WARN-3 folded, the second test) and amendment 2 (`Escape` **dropped** under `AS-§6 A-2`).

**Checks I deliberately did NOT run, and why:** `TASK-1290`'s **(1)** route-(0)-A decision · **(2)** the `+`/`−` entry
point · **(4)** the focus visual (`DECK-§5` (e)) · **(5)** the FIRST test's architecture · **(6)** the 14 UE symbols it
already read at the headers · **(7)** `.uasset` discipline. **They passed, `TASK-1299`'s spec (0) forbids re-running
them, and a delta gate that silently re-opens a passed decision is a rescope — which is the manager's, not QA's
(`SC-§100`).** ⇒ ⚠️ **No future reader may read this PASS as a second full review of the feature.** Nothing I re-read
changed my mind about any of the above; the `## Note to the manager` section at the end is consequently short and
carries **no reversal**.

**Not re-opened, and named so the absence is not read as an oversight:** route (0)-A, the `DECK-§6`
`ScrollWidgetIntoView` fix, `FocusCardTile`'s deliberate non-refactor into a shared helper (spec block (H)), and
`TASK-1290`'s NIT-2…NIT-6 (deferred by the manager — recorded, not boarded; handoff §A8.6).

---

## Verdict at a glance

| check | result |
|---|---|
| (1) the key census **is** the table — run by me, not read off the handoff | **PASS** — **code 19 / comment 5**, `EKeys::Escape` **0 in code**, `EKeys::Tab` **0**, zero letters, zero digits |
| (2) the exit clause's four state conditions, one entry point | **PASS with WARN-2** — (i) asserted in the suite, (iv) named + measured; (iv) has one declared degenerate path |
| (3) the back-precedence ruling (C) | **PASS** — `Handled` lives inside `if (bGridFocused)` **and** `if (ExitCardGridFocus())`; (G1) measured, and I corroborated its Blueprint lane on the live asset |
| (4) WARN-3 folded | **PASS** — `return false` at `:1191`; the retired justification is named **as retired**, not restated |
| (5) exactly ONE new test, STATE-asserting, exit assertion = (i) ⛔ not (iii) | **PASS** — ⭐ the vacuous-assertion trap is **avoided**, and written into the test's own header |
| (6) the fences, re-asserted | **PASS** (the `KBD-§`/hunk halves ACCEPTED-AS-DECLARED — no `Bash`) |
| (7) UE 5.8 API — only what the DELTA newly calls | **PASS** — 6 symbols read at the line; 0 deprecated, 0 removed, 0 signature mismatches |
| (8) `git status` | **CORROBORATED** by the session-start porcelain snapshot — exactly the named files, nothing else |
| (9) the two prose corrections the amendment ordered | **PASS** — WARN-5 corrected in place, NIT-1 "three" ⇒ six ⇒ **seven** |
| (10) the three `A-2` flag sites | **PASS** — none claims the row implements an `Escape` absorb; the citation is retained at all three |

---

## (1) THE KEY CENSUS — CODE **19**, COMMENT **5**, AND THE TWO NUMBERS ARE DIFFERENT THINGS

**My own census** (`EKeys::[A-Za-z_0-9]+` across all three changed files, whole-directory, unlimited):

**CODE — 19 literals, each exactly once, and nothing else:**

| line | literals |
|---|---|
| `DeckBuilderWidget.cpp:1206` | `Left` · `Gamepad_DPad_Left` · `Gamepad_LeftStick_Left` |
| `:1210` | `Right` · `Gamepad_DPad_Right` · `Gamepad_LeftStick_Right` |
| `:1214` | `Up` · `Gamepad_DPad_Up` · `Gamepad_LeftStick_Up` |
| `:1218` | `Down` · `Gamepad_DPad_Down` · `Gamepad_LeftStick_Down` |
| `:1462` | `Enter` · `Virtual_Accept` · `Gamepad_FaceButton_Bottom` → `AcceptFocusedCard` |
| `:1484` | `Delete` · ⭐ **`Gamepad_FaceButton_Left`** → `RemoveFocusedCard` |
| `:1521` | ⭐ **`Gamepad_FaceButton_Right`** · ⭐ **`Virtual_Back`** → `ExitCardGridFocus` |

⇒ **17 (`TASK-1290`'s count) + `Gamepad_FaceButton_Left` + `Virtual_Back` = 19.** ✅ **Matches the expected set exactly.
No unaccounted literal.** ⛔ Zero letters · ⛔ zero digits ⇒ `KBD-§4`'s table (all 26 letters; digits/punctuation
excluded by design) is never entered. ⛔ `EKeys::Tab` = **0** in all three files (every `Tab` hit — `.h:227/257/259/285`,
`.cpp:1205/1553` — is comment prose saying it is deliberately *not* bound).

**COMMENT — `EKeys::Escape` = 5, and its presence is CORRECT:** `.cpp:1508` · `:1512` · `:1517` and `.h:266` · `:270`.
⛔ **These are the `AS-§6 A-2` register citation, ordered by amendment 2, and removing them would be the defect** — they
are the pointer that caught this collision before a compile, and `DeckSlotEntryWidget.h:60`, the class next door in this
very feature, already carries the same pointer. The handoff's §A2 table declares exactly this split (3 + 2) and my count
agrees line-for-line.

### ⭐⛔ THE `AS-§6 A-2` BLOCKER CONDITION — **0 SITES, VERIFIED INDEPENDENTLY**

⛔ **`EKeys::Escape` reachable from an `FReply::Handled()` = ZERO.** `EKeys::Escape` appears in **no executable line** of
this class. Escape's actual fate is the tail at `DeckBuilderWidget.cpp:1558`, `return Super::NativeOnKeyDown(...)` —
and I read what that means at the engine: `UUserWidget::NativeOnKeyDown` is
`return OnKeyDown(InGeometry, InKeyEvent).NativeReply;` (`UserWidget.cpp:2505-2508`), and `WBP_DeckBuilder` implements
**no** `OnKeyDown` (measured — see (3)), so the reply is a default-constructed `FEventReply` = **Unhandled**.
⇒ **`Escape` falls through this class in every state, on every path.** `A-2` is not breached, only cited.

### Fences on the two changed bindings — measured on the bare tokens, not just on `EKeys::`

- ⭐ **The old B→`RemoveCopy` binding is DELETED, not shadowed and not kept "as well."** `Gamepad_FaceButton_Right`
  appears **once in code**, `:1521`, **inside the exit handler**. The Remove line is now
  `if (Key == EKeys::Delete || Key == EKeys::Gamepad_FaceButton_Left)` (`:1484`). Its four other occurrences
  (`.h:235`, `.h:241`, `.cpp:1469`, `.cpp:1482`) are all comment prose. ✅
- ⭐ `Gamepad_FaceButton_Left` appears **once in code**, `:1484`, wired to `RemoveFocusedCard()` → **`RemoveCopy`**
  (`:1327`) — the same `UFUNCTION` the `−` button calls. ✅
- ⭐ `Virtual_Back` is present and is bound **alongside** the concrete face button at `:1521`, mirroring the
  `Virtual_Accept` pair at `:1462`. ✅ (See NIT-3 on what that pair actually means on Windows.)

---

## (2) THE EXIT CLAUSE — ONE ENTRY POINT, FOUR CONDITIONS · PASS (WARN-2)

**⭐ ONE ENTRY POINT, confirmed at both ends — there is no "test variant".** `UDeckBuilderWidget::ExitCardGridFocus()`
(`.h:368`, `.cpp:1383`): the key handler calls it at `.cpp:1523`; the new test calls **the same function** three times
(`SiegeDeckSlotsTest.cpp:1465`, `:1477`, `:1488`). This is the `AddCopy`/`RemoveCopy` principle applied to the third
gesture, and it is the check I would have failed the row on. ✅

| condition | verdict | how |
|---|---|---|
| **(i)** `GetFocusedCardIndex() == INDEX_NONE` | ✅ | `FocusedCardIndex = INDEX_NONE` is written **first** (`.cpp:1399`), before any Slate work, and ⛔ written **directly** rather than through `SetFocusedCardIndex` — correct, because that mutator no-ops on `INDEX_NONE` and would leave Slate focus parked on the tile. **This is the suite's assertion.** |
| **(ii)** no card tile holds Slate focus | ✅ mechanism sound, closes at the verify leg | moving Slate focus off the tile is what makes it true; `HasAnyUserFocus()` (`Widget.h:668`) / `HasFocusedDescendants()` (`Widget.h:672`) both read live |
| **(iii)** `IsCardGridFocusLive()` false | ✅ follows from (i) | that predicate's first line returns false on `INDEX_NONE` (`.cpp:1161-1164`). ⛔ Correctly **NOT** the suite's assertion — see (5) |
| **(iv)** focus lands on a **named** target | ✅ on every live path (**WARN-2** for one declared degenerate path) | `ResolveGridExitFocusTarget()` (`.h:755`, `.cpp:1332`): the `UDeckSlotEntryWidget` for `EditingDeckIndex` → else the first non-null bar entry → else `GetRootWidget()` |

**THE REACHABILITY MEASUREMENT, RE-VERIFIED BY ME RATHER THAN ACCEPTED:** `DeckBarEntries` really is this class's own
member — `UPROPERTY(Transient) TArray<TObjectPtr<UDeckSlotEntryWidget>>` at **`DeckBuilderWidget.h:688`** (GC-safe),
cleared and repopulated in `NativeConstruct` at **`.cpp:602-603`/`:639`** under a `DeckBar != nullptr` guard. ⇒ the bar
needs **no tree walk and no knowledge of the WBP's layout**. (The handoff cites `.h:610` / `.cpp:598-606` — stale by
~78 lines; see NIT-1. The **substance is true**, which is what matters.)

**The `SlotButton` walk is correctly bounded and correctly motivated.** `SiegeDeckCardFocus::FindFirstButton`
(`.cpp:962-992`) is null-guarded, depth-capped at the same `MaxTreeDepth = 32` the tile walk uses, checks `UButton`
before descending, and reaches through a nested `UUserWidget` via `GetRootWidget()`. It exists because `SlotButton` is
`protected` on a `DECK-§5`-pinned class this row may not edit — ⛔ and `git status` confirms `DeckSlotEntryWidget.*` is
**not** in the diff. Landing on `SlotButton` rather than the entry root is the right call for a reason I checked: the
root would need `SetIsFocusable(true)` on a `DECK-§5` widget and would park focus where `SButton::OnKeyDown` never
looks, **silently killing `Enter` → `SelectDeckForEdit` on the deck bar**. The exit must not cost a shipped gesture.

**Const-correctness of the new walk, verified at the engine header (it is the one thing here that could have failed to
compile):** `Entry` is a `const UDeckSlotEntryWidget*` and `UUserWidget::GetRootWidget()` is
`UMG_API UWidget* GetRootWidget() const` (`UserWidget.h:1420`) — **const method, non-const return** — so
`FindFirstButton(UWidget*, int32)` takes it with no `const_cast`. ✅ The comment at `.cpp:1369-1370` says exactly this.

**Null-safety of the new path, walked end to end:** `DeckBarEntries.IsValidIndex(INDEX_NONE)` is false (no negative
index read) · `Entry->GetRootWidget()` is null-safe · `Target->GetCachedWidget()` is guarded by an explicit
`Target != nullptr` **statement** (not a ternary — the comment at `:1415-1417` correctly avoids an implicit-conversion
ambiguity) · `GetOwningPlayer()` is `PlayerContext.IsValid() ? … : nullptr` (`UserWidget.cpp:1487-1490`), so it is
safe on a `NewObject`'d widget · `FSlateApplication::Get()` is reached **only** past an explicit
`FSlateApplication::IsInitialized()` guard (`.cpp:1401`, engine decl `SlateApplication.h:308`). ⛔ **No new crash path.**

---

## (3) THE BACK-PRECEDENCE RULING (C) · PASS

**The enforcement point is four lines, and it is right:** `Handled` is returned **only** inside `if (bGridFocused)`
(`.cpp:1460`) **and** inside `if (ExitCardGridFocus())` (`.cpp:1523-1526`). `ExitCardGridFocus()` returns `false` when
`FocusedCardIndex == INDEX_NONE` (`.cpp:1389-1392`). ⇒ ⛔ **there is no unconditional `Handled` on
`Gamepad_FaceButton_Right` or `Virtual_Back`, and no `Handled` on a path that changed no state** — which is precisely
the defect the amendment exists to remove.

**Where a refused exit goes:** out of the `bGridFocused` block → `NavigationFromKey(Gamepad_FaceButton_Right)` returns
`EUINavigation::Invalid` (`.cpp:1201-1223` — the face buttons are not in that table) → `Super::NativeOnKeyDown` →
**Unhandled**. Falling through to `Super` rather than a bare `FReply::Unhandled()` is the **better** shape: it preserves
the WBP's own key door, which currently does nothing but might one day.

**⭐ `IsCardGridFocusLive()` IS NOW THE BACK-PRECEDENCE GUARD, AND THAT IS THE POINT OF (4).** It was the
Accept/Remove gate; after this amendment a wrong `true` no longer merely means "Delete removes a card with no outline
on screen" — it means **the grid eats the player's universal Back in exactly the state where there is nothing on
screen to go back from**. That is why the fold below is a blocker-grade check and not a tidy-up.

**(G1) is measured, not claimed — and I corroborated its Blueprint lane myself rather than trusting a binary `grep`.**
A read-only `get_asset_meta` on `/Game/UI/WBP_DeckBuilder` returns **33 events and NOT ONE `OnKeyDown` or
`OnPreviewKeyDown`** — `Construct`, `OnBackClicked`, `OnPlayClicked`, `OnResetClicked`, `OnDeckModelChanged`,
`OnDeckSlotCountChanged`, the `OnClicked_Event_*` family, the touch-template residue. Positive control (`SC-§39`): the
same read **did** return `Construct` and `OnBackClicked`, so the instrument can see events; a zero from it is a real
zero. This independently confirms (G1)'s conclusion that **leaving the deck builder today is a mouse click on "Exit",
never a key**, and that `Super::NativeOnKeyDown` genuinely resolves to Unhandled here. ⇒ ⛔ **No WARN is owed under
check (3)'s `SC-§101` clause** — the handoff makes the fall-through claim *and* backs it.

**🧑 THE PLAYER'S SECOND BACK PRESS, IN ONE LINE: it does NOTHING, and that is the measured, correct outcome — the
first B leaves the GRID, the second B finds an unfocused grid, is refused, falls through, and meets nothing on this
screen that handles Back.** ⛔ Not a defect of this row. (`TASK-1291`'s verifier: acceptance (11)'s failure case is the
opposite one — a **first** Back that closes the whole builder.)

**One honest note kept from §A8.1, because it reads like dead code and is not:** `bGridFocused == true` already implies
`FocusedCardIndex != INDEX_NONE`, so `if (ExitCardGridFocus())` can never take its false branch *from that call site*.
It is an invariant's enforcement point, and the suite's (a)/(d) assertions are what make it non-vacuous.

---

## (4) WARN-3 FOLDED · PASS

`DeckBuilderWidget.cpp:1172-1191` — the live-Slate/no-tile fallback now `return false`. ✅ **Folded.**

⛔ **No WARN on the comment:** it does not restate the retired justification, it **names it as retired** —
*"This used to `return true` on the reasoning 'no live grid to corroborate against — an offline widget'. That reasoning
does not hold: Slate IS initialised here, because the check immediately above already claimed the offline lane."* —
and then states the new stake (the Back-precedence guard). That is the correct shape for a load-bearing predicate, and
the first fallback (`:1166-1169`, `!FSlateApplication::IsInitialized()` ⇒ `true`) still owns the offline lane cleanly.

**Blast radius of the fold, re-walked:** `IsCardGridFocusLive()` is reachable **only** from `NativeOnKeyDown`, which no
test drives (it is `protected`). ⇒ ⛔ the fold cannot move the suite. Live, it makes the feature **inert** instead of
acting invisibly when `CardTileClass` fails to resolve — strictly safer.

---

## (5) THE SUITE DELTA · PASS — ⭐ THE VACUOUS ASSERTION IS AVOIDED

**Exactly ONE new test.** `Siegebound.Deck.ExitingCardGridClearsTheFocusedIndex` (`FSiegeDeckExitCardGridTest`,
`SiegeDeckSlotsTest.cpp:1421-1424`, `EditorContext | EngineFilter`). The file now holds **17**
`IMPLEMENT_SIMPLE_AUTOMATION_TEST`, of which **two** are `TASK-1286`'s (`:1238` and `:1421`) ⇒ **baseline +2**,
consistent with the declaration. ⛔ The executed count is `TASK-1291`'s to measure (`SC-§71b`).

### 🚨 THE SUBTLEST CHECK ON THIS ROW — AND IT PASSES

⛔ **The exit assertion is (i), NOT (iii).** Verified at the lines:

- **(b) THE POSITIVE CONTROL, present:** `TestEqual("(b) POSITIVE CONTROL: tile K is the focused tile BEFORE the exit",
  Builder->GetFocusedCardIndex(), TileK)` (`:1472-1473`), with `TileK = Collection.Num() / 2` — a **middle** tile, so a
  bug that silently focuses 0 cannot pass by coincidence.
- **(c) the exit:** `GetFocusedCardIndex() == INDEX_NONE` and `GetFocusedCardID().IsNone()` (`:1479-1482`).
- ⛔ **`IsCardGridFocusLive()` is asserted NOWHERE in either test** — I grepped the file for it: the only occurrences
  are in the test's own header prose (`:1393-1400`) **explaining why it must not be asserted**. Post-fold that
  predicate is false in this lane **by construction** (Slate initialised under `EditorContext`, no `WBP_DeckCardTile`
  resolves for a `NewObject`'d builder), so such an assertion could not fail and would not be evidence (`SC-§39`).
  ⭐ **The trap was not merely avoided — it was written into the test's header so the next author cannot walk into it
  while "strengthening" the test.** That is the right artefact, and it is worth saying out loud.

**The no-write guarantee, RE-WALKED for the new test (⚠️ `DECK-§4` auto-save is live and this is 🧑 his real deck):**
`ExitCardGridFocus` touches `FocusedCardIndex`, `ResolveGridExitFocusTarget`, `GetCachedWidget`, `GetOwningPlayer` and
`FSlateApplication` — ⛔ it reaches **neither `AddCopy` nor `RemoveCopy`**, so it cannot enter `PersistWorkingDeck` and
cannot reach `SaveDeckAs` / `SaveGameToSlot`. The test's other calls are `SetFocusedCardIndex` / `MoveCardFocus`
(focus only), `GetTotalCount` / `GetCountOf` / `GetFocusedCard*` (reads) and `DoesSaveGameExist` (a read). The belt and
braces are all present: `NewObject`'d builder, `NativeConstruct` never runs, `GetEditingDeckIndex() == INDEX_NONE`
asserted as a **premise** (`:1439-1440`), `FDeckScratchGuard` in scope, scratch slot asserted absent (`:1503-1504`).
⇒ **no path in this test can write a save.**

**The `Occurrences 2` pin is still exact, and for the right reason.** The new test adds **zero** `PersistWorkingDeck`
refusals (it never mutates), and the sibling's `AddExpectedMessagePlain(… 2)` lives in its own `RunTest` scope,
untouched. ⛔ No off-by-one re-pin was needed and none was made — so a RED here would be a real regression, not
bookkeeping.

**STATE, never call counts (`SC-§104`)** — every assertion reads `GetFocusedCardIndex` / `GetFocusedCardID` /
`GetTotalCount` / `GetCountOf` / `DoesSaveGameExist`. Two further things I checked rather than assumed: **(f)** compares
against `TotalBefore` / `CountOfKBefore` captured at `:1456-1457`, i.e. **before anything ran** — ⛔ not against a
second read of itself (the programmer declared having written that trap and fixed it; the fix is on disk); and **(e)**'s
`GetFocusedCardIndex() == 0` after `MoveCardFocus(Down)` follows from `StepCardFocusIndex`'s arming branch
(`.cpp:1238-1251`: `CurrentIndex < 0` + cardinal + `Down` ⇒ `0`), so the re-entry assertion is sound and the exit
provably does not brick the grid.

---

## (6) THE FENCES, RE-ASSERTED

| fence | result |
|---|---|
| `EKeys::Tab` | ✅ **0** in code across all three files (comment prose only, explaining it is deliberately unbound) |
| letters / digits | ✅ **0** — all 19 literals are named keys |
| the `+` / `−` mouse-button bodies | ✅ diff **0** — they are BP graph nodes inside `WBP_DeckCardTile.uasset`, and `git status` carries **no `Content/` entry at all**, so they cannot have changed |
| Accept / Remove still the SAME entry points | ✅ `AcceptFocusedCard` → `AddCopy(CardID)` (`.cpp:1308`) · `RemoveFocusedCard` → `RemoveCopy(CardID)` (`.cpp:1327`) — the only call sites, declarations untouched at `:793` / `:838`. ⛔ Nothing re-implemented |
| no `IA_` / `IMC_` / `EnhancedInput` / `BindAction` **code** | ✅ **0** — every `IA_` hit (`.h:210/211/214/219`, `.cpp:879`) is comment prose explaining why there is none |
| `DECK-§3`'s right-click gets no keyboard twin | ✅ `EKeys::RightMouseButton` appears only in `DeckSlotEntryWidget.{h,cpp}`, which are **not in the diff** |
| `IMC_Hero` / `IMC_Default` / `IMC_MouseLook` in `git status` | ✅ absent |
| `.uasset` in the diff | ✅ **zero** — route (0)-A stands and is not re-opened |
| `KBD-§` remap tables diff 0 lines | ⚠️ **ACCEPTED-AS-DECLARED** (`SC-§71b`) — I hold no `Bash`; handoff §3 declares `git diff -- CONVENTIONS.md \| grep -c 'KBD-§'` = **0**. `TASK-1291` re-measures |

---

## (7) UE 5.8 API — ONLY WHAT THE DELTA NEWLY CALLS, READ AT THE LINE

| symbol | where I read it | verdict |
|---|---|---|
| `EKeys::Gamepad_FaceButton_Left` | `InputCoreTypes.h:518` — `static INPUTCORE_API const FKey` | ✅ live, not deprecated |
| `EKeys::Virtual_Back` | `InputCoreTypes.h:747` — same form | ✅ live, not deprecated |
| `FPlatformInput::GetGamepadBackKey()` | `InputCore/Public/GenericPlatform/GenericPlatformInput.h:32-35` → `return EKeys::Gamepad_FaceButton_Right;` (and `:27-30` → `Gamepad_FaceButton_Bottom` for Accept) | ✅ **re-quoted here on purpose — this is the row's own justification and it now ships in the report beside the fix** |
| `UUserWidget::GetRootWidget()` | `UserWidget.h:1420` — `UMG_API UWidget* GetRootWidget() const` | ✅ const method, non-const return ⇒ the new walk compiles from a `const` entry pointer |
| `UWidget::HasFocusedDescendants()` / `HasAnyUserFocus()` | `Widget.h:672` / `:668` — `UMG_API bool … const`, both `UFUNCTION` | ✅ exact |
| `UWidget::GetCachedWidget()` | `Widget.h:857` — `UMG_API TSharedPtr<SWidget> GetCachedWidget() const` | ✅ exact |
| `FSlateApplication::SetUserFocus(uint32, const TSharedPtr<SWidget>&, EFocusCause)` | `SlateApplication.h:678` | ✅ signature matches the call |
| `FSlateApplication::IsInitialized()` | `SlateApplication.h:308` — `static bool` | ✅ |
| `UUserWidget::NativeOnKeyDown` default body | `UserWidget.cpp:2505-2508` | ✅ delegates to the BP event; unimplemented ⇒ **Unhandled** |
| `UUserWidget::GetOwningPlayer()` | `UserWidget.cpp:1487-1490` | ✅ null-safe on an unconstructed widget |

**⛔ 0 deprecated · 0 removed · 0 signature mismatches.** The 14 symbols `TASK-1290` already read were **not** re-read.

**Reflection / GC on the delta:** `ExitCardGridFocus()` is `UFUNCTION(BlueprintCallable)` returning `bool`
(`.h:367-368`) — UHT-legal. `ResolveGridExitFocusTarget()` is a private, non-reflected `const` method returning a raw
`class UWidget*` consumed immediately in the same frame — no storage, no GC exposure. `DeckBarEntries` was already
`UPROPERTY(Transient)` + `TObjectPtr` (`.h:687-688`). `Components/Button.h` (`.cpp:5`) is the amendment's **only** new
include, and `UMG` is already in `PublicDependencyModuleNames` ⇒ ⛔ `Build.cs` diff 0. Includes are alphabetical,
commented, **seven** total for this feature (`.cpp:5,8,9,12,14,15,25`), matching the corrected §A2/NIT-1 count.

---

## (8) `git status` — CORROBORATED, NOT MERELY ACCEPTED

The session-start porcelain snapshot available to me reads exactly:
`M CONVENTIONS.md` · `M TASKBOARD.md` · `M DeckBuilderWidget.cpp` · `M DeckBuilderWidget.h` ·
`M Tests/SiegeDeckSlotsTest.cpp` · `?? handoffs/TASK-1286-programmer.md` · `?? qa/TASK-1290-report.md`.

⇒ **exactly what check (8) expects, and nothing else.** ⛔ No `.uasset`, no `Content/`, no `Saved/SaveGames/`, no
`GitClaudeUnrealTest.uproject`, no `Config/SiegeCloudDev.ini`, no `settings.local.json`, no `IMC_*`. `CONVENTIONS.md` /
`TASKBOARD.md` are the manager's named `SC-§121` / `AS-§6 A-2` / TASK-1286/1291/1299/1300 dirt. ⚠️ This is a snapshot
taken at my session start, **not** a live `git status` I ran (no `Bash`); `TASK-1291` re-measures at 5a.

---

## Findings

### WARN-1 — `Tests/SiegeDeckSlotsTest.cpp:1390` and `:1485` — two comment sentences assert the DROPPED `Escape` binding as live fact, in the ONE file of this feature that carries no `AS-§6 A-2` pointer
⚖️ **ROUTED TO ME FOR A RULING. My ruling: FIX IT — a two-site, comment-only edit — ⛔ not a recorded NIT.** Reasons:

1. **Both sentences are now false, and one of them sits directly above the assertion it misdescribes.** `:1390`
   ("*B stops deleting cards and starts meaning Back, **alongside Escape***") and `:1485` ("*B/Escape once leaves the
   grid, again leaves the builder*") — `:1485` is the comment on assertion **(d)**, the nested-Back case.
2. **This file has no `A-2` citation at all.** The `.h`, the `.cpp` and the handoff each carry the register pointer;
   the test file carries only the false claim. A reader who opens only this file to extend the test meets
   *"Escape leaves the grid"* with nothing to contradict it — and the cheapest way to "complete" that test is to bind
   `Escape`, which is **an automatic QA FAIL** under a closed Jonathan ruling. That is check (10)'s own harm model
   (`SC-§119`'s family: a stale comment on a load-bearing decision is how the next reader re-derives the wrong rule),
   and it applies here with more force, not less, because the misdescription is in a *test*.
3. ⛔ **Why it is nevertheless NOT a blocker, and why the programmer was RIGHT to leave it:** zero code effect, zero
   effect on what the suite asserts, and its brief **fenced** that file. It **declared** the residual instead of
   editing a fenced file on its own authority (`SC-§101`) — the same instinct that produced amendment 2 in the first
   place. Penalising that would teach exactly the wrong lesson.
4. **The route, since ⛔ QA may not edit source and ⛔ build-master may not write code:** the manager amends the fence
   line to permit these two comment lines, and the `gameplay-programmer` makes the edit on its next touch of the row.
   Suggested text — `:1390` "…starts meaning Back" (delete "alongside Escape"); `:1485` "B once leaves the grid,
   again leaves the builder — ⛔ Escape is NOT bound (AS-§6 A-2)". If the manager would rather not re-open a passed
   file at all, **downgrading this to a recorded NIT is a defensible call that is his to make** — but then the
   `A-2` pointer should be added to the file's header in the same breath, because the sentence that survives is false.

### WARN-2 — `DeckBuilderWidget.cpp:1414-1433` — exit condition (D)(iv) holds on every LIVE path, but there is one declared path where focus is not moved at all and the function still reports `true`
If `ResolveGridExitFocusTarget()` yields nothing with a cached Slate widget, or there is no `ULocalPlayer`, the code
clears the index, logs `Verbose`, and returns **`true`** ⇒ `FReply::Handled()`.
- ⛔ **This is NOT "focus left nowhere"** (the board's blocker condition): Slate focus stays where it was, on the card
  tile; it is simply not *moved*. And the state change is real — the model index went K → `INDEX_NONE` and the grid
  stops consuming keys — so the (C) ruling is not violated either.
- **Unreachable on a live menu, and I checked why:** `NativeConstruct` populates `DeckBarEntries` with
  `CreateWidget`'d entries that are added to `DeckBar`, so on any screen the player can actually see, an entry with a
  cached `SlotButton` exists.
- **The alternative is worse, which is why I am not asking for a change:** returning `false` there would leave the
  player *in* the grid with his Back press falling through to nothing (G1: nothing on this screen handles Back).
- ⇒ **Recorded as a disclosure, not a change request.** The residual state it produces (a tile still wearing the dashed
  outline while the model reads `INDEX_NONE`) is `TASK-1290`'s NIT-6 + the handoff's own `[AUDIT]` §8.10, both
  already passed and ⛔ not re-litigated here.

### NIT-1 — handoff §A4 — the `DeckBarEntries` line citations are stale by ~78 lines
§A4 cites `DeckBuilderWidget.h:610` and `.cpp:598-606`; measured now at **`.h:688`** and **`.cpp:602-603` / `:639`**.
The drift is caused by the amendment's own prose additions to the header. ⛔ The **claim** is true and I verified it
independently; only the line numbers moved. Named so a future reader does not conclude the measurement was invented.

### NIT-2 — a raw `EKeys::` census over the three files returns **26**, not 24 — the handoff's table omits two `Virtual_Back` comment hits
19 code + **7** comment: the declared 5 × `EKeys::Escape`, **plus `EKeys::Virtual_Back` at `.cpp:1470` and `.h:242`**
(both inside the B-is-Back justification prose). §A2's table tracks only `Escape` in its comment column. Harmless, but
a mechanical census that expects "19 + 5 = 24" will come up two over on a **correct** diff — which is the exact class
of confusion §A2 exists to prevent.

### NIT-3 — `DeckBuilderWidget.cpp:1521` — on Windows the two comparisons are the SAME key, and that is fine
`InputCoreTypes.cpp:424` assigns `Virtual_Back = FPlatformInput::GetGamepadBackKey()`, which I re-read returns
`EKeys::Gamepad_FaceButton_Right` (`GenericPlatformInput.h:32-35`). So `Key == Gamepad_FaceButton_Right ||
Key == Virtual_Back` is redundant **on this platform** and correct on one that remaps its Back button. ⛔ Not a change
request: it is character-for-character the shape `:1462` already uses for Accept (`Virtual_Accept` beside
`Gamepad_FaceButton_Bottom`), which `TASK-1290` passed. Recorded only so nobody "simplifies" one of the two away.

### NIT-4 — `DeckBuilderWidget.cpp:962-992` — `FindFirstButton` returns the FIRST `UButton` in panel-child order
If a future `UDeckSlotEntryWidget` tree ever grows a button **before** `SlotButton`, the exit focus lands on that one
instead. Latent and declared (§A8.3); same shape as `TASK-1290`'s NIT-4. The mitigation today is measured: the entry
tree is code-authored as `OutlineBorder` → `SlotButton` → `SlotLabelText`, and this row may not edit that class.

---

## ⚖️ THE SECOND ROUTED RULING — the two extra comment-only prose fixes the programmer made and DECLARED

**Sites:** `DeckBuilderWidget.cpp`'s `IsCardGridFocusLive` WARN-3 block ("the grid EATS ~~Escape /~~ gamepad B") and
`DeckBuilderWidget.h:345-347`'s `ExitCardGridFocus` doc comment ("the gesture behind gamepad B / `Virtual_Back`.
⛔ NOT Escape").

⭐ **RULING: IN SCOPE, CORRECT, AND THE STRONGER STATEMENT IS THAT NOT MAKING THEM WOULD HAVE BEEN THE DEFECT.**

1. **Both sites are inside the very functions this amendment rewrites.** The WARN-3 block **is** amendment work — the
   fold is check (4)'s subject and the manager ordered the comment rewritten "to say WHY rather than what". The
   `ExitCardGridFocus` doc comment documents the function the amendment's key change binds. Neither is a drive-by edit
   of untouched code.
2. **Both asserted the dropped binding as live fact.** Leaving them would have put the row in direct violation of its
   own check (10) principle one screen away from the three sites check (10) names.
3. ⛔ **Zero code effect** — I verified no declaration, signature, specifier or executable line changed at either site;
   they are comment text inside already-modified functions.
4. **They were declared, not quiet** (`SC-§101`), which is the whole difference between this and a scope violation.

⇒ ✅ **No finding is recorded against them.** ⚠️ Note the asymmetry with WARN-1 and that it is *principled*, not
inconsistent: these two sites are in files and functions the row is editing anyway; the test-file residual is behind an
explicit fence the programmer had no authority to lift.

---

## (10) THE THREE `A-2` FLAG SITES · PASS

| site | reads | verdict |
|---|---|---|
| `DeckBuilderWidget.h:262-286` | "THE EXIT IS GAMEPAD-ONLY — Escape was DROPPED under `AS-§6 A-2`, and that is the current, settled state of this class, not an omission… ⛔ There is no `FReply::Handled()` on `EKeys::Escape` anywhere in this class, and none may be added." Names the collision as **flagged, not overruled**; names `TASK-1300` as 🧑 **his open scope question**; labels its own (G1) evidence "⛔ evidence for HIS ruling, not a ruling" | ✅ **citation retained** (`CONVENTIONS ~:789`, `DeckSlotEntryWidget.h:60`, `:7519`, `:8316`, `:8595`, `:2679` cl. 13) |
| `DeckBuilderWidget.cpp:1504-1520` | the same, quoting `A-2`'s operative sentence, plus the one-line re-add recipe should he scope `A-2` out | ✅ citation retained |
| handoff §A7 | retitled **"RULED AND CLOSED — `AS-§6 A-2` WINS"**; "This section was a **flag**; it is now a **record**"; `A-2` quoted verbatim | ✅ citation retained |

⛔ **No site still claims the row implements an `Escape` absorb.** §A7's surviving *"implemented as the board specced
it and flagged"* is a **historical statement about instance 1's conduct**, not a claim about the shipping code — and it
is correctly framed as the positive exhibit for `SC-§121` cl. 5. ⇒ **no WARN under check (10).**

---

## Note to the manager (⛔ out of scope — nothing here is a reversal, `SC-§100`)

1. ⛔ **Nothing `TASK-1290` passed was reversed, and nothing I re-read made me want to.** This section exists because
   the row requires it, and it is deliberately empty of verdicts.
2. **WARN-1 needs YOUR fence decision, not mine.** QA may not edit source, and the test file is fenced by the
   programmer's brief. Either amend the fence for two comment lines, or rule it a recorded NIT — but in the second
   case please pair it with an `A-2` pointer in that file's header, because the sentence that survives is false.
3. 🙋 **`TASK-1300` is untouched by this gate and blocks nothing.** Whether `AS-§6 A-2` scopes to a main-menu deck
   builder is 🧑 **his**. This PASS is issued reading `A-2` at its **widest** — nothing absorbs `Escape`, anywhere —
   exactly as the row instructs, and the code complies at that width. If he later scopes `A-2` out of the menus, the
   re-add is `Key == EKeys::Escape ||` on `DeckBuilderWidget.cpp:1521` and **nothing else**; I verified the claim that
   makes that true — `EKeys::Escape` is **0** in `SiegeDeckSlotsTest.cpp` and both tests drive `ExitCardGridFocus()`
   directly, so no test would need touching.
4. ⭐ **Worth a law, or at least a line in the register:** this row is the first time a flagged collision reached a
   gate with the flag **still readable in the shipping code**. The `A-2` citation surviving at three sites is the
   reason the next author cannot re-derive the wrong rule — and it is also why the census had to be split into code and
   comments. ⛔ Any future "cleanup" grep that deletes `EKeys::Escape` from these comments is deleting the control.

---

## Notes for build-master (`TASK-1291`) and the `playtest-verifier`

1. **ACCEPTED-AS-DECLARED, re-measure at 5a (`SC-§71b`):** the compile result · the suite total (**expect baseline +2**,
   not +1 — the amendment adds `Siegebound.Deck.ExitingCardGridClearsTheFocusedIndex`) · `git status --porcelain` ·
   `git diff -- CONVENTIONS.md | grep -c 'KBD-§'` = 0 · the `.cpp`/`.h` hunk geometry. I hold no `Bash`.
2. ⚠️ **A green suite remains NECESSARY AND NOT SUFFICIENT.** Nothing in the suite drives `NativeOnKeyDown` (it is
   `protected`), so ⛔ **nothing proves that B reaches the exit, that X reaches `RemoveCopy`, or that `Escape` falls
   through** — the key table, `NavigationFromKey` and the `IsCardGridFocusLive()` gate close only on the verify leg
   and on 🧑 his hands.
3. ⛔⛔ **DO NOT PRESS `Escape` TO TEST THIS ROW.** `Escape` is not bound (`AS-§6 A-2`), and in PIE an unconsumed
   `Escape` is the editor's **Stop** shortcut — spending one ends the session for nothing, and a session that ends is
   not a measurement.
4. **What IS keyboard-observable — the whole of the keyboard acceptance:** `Down` → `Enter` (**n+1**, the positive
   control) → `Delete` (**n**). The observable reads **`"Deck: n/50"`**, not `"n/50"`. ⛔ Net zero mutations on his real
   deck; say so explicitly. Use `simulate_key_press` — there is no `IA_` for this feature **by design**, so
   `inject_input_action` will not exercise it and "no `IA_Deck*` mapping exists" is describing the design.
5. ⭐ **The exit (B) and the new Remove (X) are GAMEPAD-ONLY and are therefore `UNOBSERVABLE` unless
   `Gamepad_FaceButton_Right` / `Virtual_Back` / `Gamepad_FaceButton_Left` can be injected as real Slate key events.**
   One line, ⛔ zero attempts spent, ⛔ never scored as a pass (`VER-§5`). **This code read is what closes them.**
6. **So an expected behaviour is not filed as a bug:** the **second** B press does **nothing** — the builder stays
   open, measured (G1) and corroborated by me on the live `WBP_DeckBuilder` event list. The failure case is the
   opposite one: a **first** B that closes the whole builder ⇒ the precedence defect ⇒ `fail`.
7. 🙋 **Still owed to Jonathan's eye, and unchanged by this amendment:** `DECK-§5` (e) — does the dashed white
   50 %-alpha focus rectangle actually *read* over a card's art? A still of a focused tile is the cheapest evidence.
8. **M8:** the delta adds no replicated property, no new replicated class, no relevancy tier and no RPC.
