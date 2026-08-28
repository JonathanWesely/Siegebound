# TASK-671 — [DB3-3] THE DECK BAR — programmer handoff (2026-08-27)

Status: **ready-for-qa** (TASK-673 reviews the wave; TASK-674 owns the compile slot after 667 — QUIET-MODULE honored, **zero compile run**). Board flip proxied by the orchestrator (the dispatch fence barred board writes from this task; declared, not skipped silently).

## 1. Diff summary (files touched — exactly the TASK-671 names block, nothing else)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/DeckSlotEntryWidget.h` | **NEW** — the DECK-§8 class; every pinned line character-for-character (delegate decl, three setters, two gesture delegates, `RebuildWidget`/`NativeOnMouseButtonDown` overrides, the three BindWidgetOptional children, `SlotIndex = INDEX_NONE`). Plus the declared implementation additions (§6.1). |
| `Source/GitClaudeUnrealTest/Siegebound/DeckSlotEntryWidget.cpp` | **NEW** — tree build (root-first, DECK-§5b), gesture handling, the two state writers, `DeckActiveOutlineColor` ONE definition. |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h` | ADDS the pinned `DeckBar` BindWidgetOptional member + private `RefreshDeckBarStates()` (both registry lines verbatim incl. trailing comments) + private `DeckBarEntries` array + `class UDeckSlotEntryWidget;` fwd decl. 670's comments updated where they said "TASK-671 appends here". Nothing 670 landed was moved or reworded semantically. |
| `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` | Bar build appended at the END of `NativeConstruct` (after 670's model init); `RefreshDeckBarStates()` implemented; ONE refresh call appended to `SelectDeckForEdit` (after its broadcast) and ONE to `SetActiveDeck`'s success path (after its broadcast). Two includes + one include added. **No other function body changed.** |

**Fence proof (session-scoped):** `git status` shows my four files; also dirty are `Castle.{h,cpp}` / `SiegeGameMode.{h,cpp}` / `SiegeBotController.{h,cpp}` (TASK-664/665 in-flight), `SiegeDeckSaveGame.{h,cpp}` + `Tests/SiegeDeckSlotsTest.cpp` (TASK-670 in-flight, untouched by me), pipeline docs, and a pre-existing `Content/Maps/L_Arena.umap` working-tree line that predates this task (editor-session dirt — for 674's reconcile ledger, NOT part of any deck-wave diff). **Zero diff on `SiegePlayerController.{h,cpp}`, `SiegeDeckSaveGame.{h,cpp}`, all ROT files; no `.uasset` authored anywhere** (`/Game/UI/WBP_DeckSlotEntry` stays reserved-unused; no editor/MCP touched).

## 2. The tree + state table (DECK-§3/§5 as implemented)

Tree (pinned): `OutlineBorder` (`UBorder`, root — set BEFORE `Super::RebuildWidget()`) > `SlotButton` (`UButton`) > `SlotLabelText` (`UTextBlock`). Label font 20, button content padding 10/6/10/6, border padding = 3px (the outline ring).

| State | Visual channel | Only writer | Value |
|---|---|---|---|
| ACTIVE ("my match deck") | `OutlineBorder` brush (the 3px rim) | `SetOutlineActive` | `DeckActiveOutlineColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)` (pinned, ONE definition `DeckSlotEntryWidget.cpp:20`) vs fully transparent `(0,0,0,0)` |
| EDITING ("the grid edits this") | `SlotButton` background fill | `SetEditingHighlight` | steel-blue tint `(0.55, 0.70, 0.95, 1)` vs stock white |
| ACTIVE + EDITING | both channels at once | — | orange rim AND blue fill — different channels, never a second outline, never conflated |
| neither | — | — | transparent rim, white fill |

State changes never resize the entry (the rim exists in both states; only its color changes) — the bar cannot shift a pixel on right-click.

`RefreshDeckBarStates()` is the ONE redraw: one `GetActiveDeckIndex()` read per refresh (never per entry), `EditingDeckIndex` read directly; silent no-op when the bar was never built. Call sites: the bar build (NativeConstruct, once), `SelectDeckForEdit` (editing moved), `SetActiveDeck` success path (active moved — this one site covers BOTH `SetActiveDeckBySlot`'s right-click lane AND the shipped D8 "Play with this deck" activation, so the orange follows both, and covers TASK-674 driving the UFUNCTIONs directly with no gesture).

## 3. The right-click path trace (DECK-§5)

RMB press anywhere on an entry → `SButton::OnMouseButtonDown` answers the left/touch gesture only ⇒ returns Unhandled for RMB (this is WHY a Button `OnClicked` right-click is a dead gesture) → the press bubbles up the hit-test widget path: `STextBlock` (unhandled) → `SButton` (unhandled for RMB) → `SBorder` (no bound handler ⇒ unhandled) → the entry's `SObjectWidget` → **`UDeckSlotEntryWidget::NativeOnMouseButtonDown`** → `InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton` → `OnRightClicked.ExecuteIfBound(SlotIndex)` → **`UDeckBuilderWidget::SetActiveDeckBySlot(SlotIndex)`** (BindUObject, made in the bar build) → range check → `SetActiveDeck(MakeFixedDeckName(SlotIndex))` — 670's EXISTING strict path unchanged: deck-exists check, canonical-name store into `ActiveDeckName`, ACC-§4 call-time seam (`ResolveDeckSlotName`), `SaveGameToSlot` persists, `OnDeckModelChanged` — → `RefreshDeckBarStates()` (my appended line) → the orange rim moves → back in the entry, `return FReply::Handled()`.

Every non-RMB press returns `Super::NativeOnMouseButtonDown(...)` so LMB still reaches `SlotButton` → `OnClicked` → `HandleSlotButtonClicked` → `OnLeftClicked(SlotIndex)` → `SelectDeckForEdit` (D7). **No key handling exists anywhere in either file — Escape stays permanently unabsorbed (AS-§6 A-2).**

## 4. DECK-§8 conformance table (character-for-character)

| Registry line | Where | Status |
|---|---|---|
| `DECLARE_DELEGATE_OneParam(FOnDeckSlotGesture, int32 /*SlotIndex*/);` | DeckSlotEntryWidget.h | verbatim |
| `void SetSlotIndexAndLabel(int32 InSlotIndex, const FString& Label);` | DeckSlotEntryWidget.h | verbatim |
| `void SetOutlineActive(bool bActive);` + trailing comment | DeckSlotEntryWidget.h | verbatim incl. `// DeckActiveOutlineColor vs transparent — the ONLY outline writer` |
| `void SetEditingHighlight(bool bEditing);` + trailing comment | DeckSlotEntryWidget.h | verbatim incl. `// SlotButton fill tint ONLY (DECK-§3)` |
| `FOnDeckSlotGesture OnLeftClicked;` / `OnRightClicked;` + comments | DeckSlotEntryWidget.h | verbatim |
| `virtual TSharedRef<SWidget> RebuildWidget() override;  // root-first, then Super (TASK-444 order)` | DeckSlotEntryWidget.h | verbatim |
| `virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;` | DeckSlotEntryWidget.h | verbatim |
| the three `UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class ...>` children | DeckSlotEntryWidget.h | verbatim incl. registry column spacing |
| `int32 SlotIndex = INDEX_NONE;` (private) | DeckSlotEntryWidget.h | verbatim |
| `UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UHorizontalBox> DeckBar;   // authored in WBP by TASK-672; null-safe` | DeckBuilderWidget.h | verbatim |
| `void RefreshDeckBarStates(); // outline = active, fill tint = editing` (private) | DeckBuilderWidget.h | verbatim |
| `DeckActiveOutlineColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)`, one definition in the .cpp | DeckSlotEntryWidget.cpp | verbatim (DECK-§7 pin; grep §7 proves single definition) |
| 670's landed surface | — | untouched: no signature moved; `SelectDeckForEdit`/`SetActiveDeck` gained one appended refresh line each AFTER their existing broadcasts (see §6.3) |

## 5. What only pixels can verify (TL-§4 / DECK-§5e — no claim made here)

1. The orange rim actually RENDERS orange (brush-tint behavior of a default UBorder brush), and the transparent rim renders as nothing.
2. The editing tint is visually distinct from the orange and legible against the button's hover/pressed styles.
3. The physical RMB gesture reaching `NativeOnMouseButtonDown` live (the bubbling argument in §3 is engine-source reasoning, not an observation; ruling 4 — gestures close on Jonathan at 675, state mechanics via direct calls at 674).
4. Ten labeled entries visible, in order, at 1280×720 / 1600×900 / 1920×1080 (674's capture gate); label font 20 legibility at 1280 wide.
5. The bar sitting at the very top — that is TASK-672's container placement, not mine; my code fills whatever `DeckBar` binds.

## 6. Deviations / interpretations (SC-§15 — declared, not silent)

1. **Unpinned members added to `UDeckSlotEntryWidget`** (the registry pins the surface, the implementation needs a lifecycle): `NativeConstruct`/`NativeDestruct` overrides (bind/unbind `OnClicked` — children only exist after `RebuildWidget`, the AccountMenu lifecycle rule), protected `UFUNCTION() HandleSlotButtonClicked()` (a dynamic delegate REQUIRES a UFUNCTION thunk — the registry's own `// from SlotButton->OnClicked` comment names this lane), private `ConstructEntryTree()` (the ACC-§5-shape build helper every code-authored widget here has), and four file-local .cpp constants (inactive rim, editing tint, neutral tint, 3px thickness).
2. **Unpinned private on `UDeckBuilderWidget`:** `DeckBarEntries` (`UPROPERTY(Transient)` array) — `RefreshDeckBarStates` must reach the ten entries without re-creating them; also GC-anchors the created widgets.
3. **`SetActiveDeck` gained one appended line** (`RefreshDeckBarStates()` after `OnDeckModelChanged()` on the success path). 670 declared this function byte-compatible; my append changes no existing behavior, mirrors 670's own funnel-append pattern on the mutators, and is REQUIRED so the outline follows D8's "Play with this deck" and 674's direct `SetActiveDeckBySlot` calls — a gesture-handler-only refresh would leave the bar stale on every non-gesture activation. QA to ratify.
4. **The editing tint value is mine** (law pins only the orange): steel blue `(0.55, 0.70, 0.95)`, chosen maximally far from orange; pixels rule at 674/675 and it is a one-line retune.
5. **The bar build is inlined in `NativeConstruct`** rather than a new private helper — the registry pins `RefreshDeckBarStates` as the only added private routine; I did not invent a second unpinned one for a once-per-construct loop.
6. **Entry slots are `FSlateChildSize(ESlateSizeRule::Fill)` each** (+2px padding, Fill/Fill alignment): ten equal shares of any window width, so the bar itself can never clip an entry (DECK-§6 spirit). Layout choice, pixels rule.
7. **`RefreshDeckBarStates` reads the save once per refresh** via `GetActiveDeckIndex()` (a disk read, 670's shape) — one extra read per mutation, same cost class as the mutation's own save; no caching added (no cached slot/state anywhere, ACC-§4 posture).

## 7. QA greps (run 2026-08-27; re-run to verify)

- One-composer law (DECK-§1): `grep -rn 'TEXT("deck' Source/ --include=*.cpp --include=*.h | grep -v Tests/` ⇒ still ONLY `SiegeDeckSaveGame.cpp:25`. My two new files contain "deck1"/"deck10" in COMMENTS only — the entry receives labels via `SetSlotIndexAndLabel(MakeFixedDeckName(i))` and never composes or hand-types a name.
- Orange pin (DECK-§7): `grep -rn DeckActiveOutlineColor Source/` ⇒ one definition (`DeckSlotEntryWidget.cpp:20`), one use site (`SetOutlineActive`), plus comments.
- No new save path (DECK-§4): `grep -n SaveGameToSlot` on `DeckSlotEntryWidget.cpp` ⇒ zero; my `DeckBuilderWidget.cpp` diff adds no save/load call — every mutation still funnels through 670's `PersistWorkingDeck`/`SetActiveDeck`.
- No key handling / Escape (AS-§6 A-2): `grep -n 'OnKeyDown\|EKeys::Escape'` on both new files ⇒ zero.
- Trailing-default law (SC-§33): nothing owed — no defaulted parameter added to any function, new or existing.

## 8. Compile-risk sites for QA (⛔ no compile was run — QUIET-MODULE, 674 owns the slot)

1. `UButton::SetBackgroundColor(FLinearColor)` and `UTextBlock::SetFontSize(20.f)` (float literal) — both cloned from the compiled `AccountMenuWidget.cpp` idiom (`SetFontSize(28.f)` at :334, brush/color setters throughout).
2. `CreateWidget<UDeckSlotEntryWidget>(this)` with a `UUserWidget*` owner — the templated owner overload in `Blueprint/UserWidget.h`.
3. `Entry->OnLeftClicked.BindUObject(this, &UDeckBuilderWidget::SelectDeckForEdit)` — plain-delegate binding straight onto a UFUNCTION member with the matching `void(int32)` signature (same for `SetActiveDeckBySlot`).
4. `UHorizontalBoxSlot::SetSize(FSlateChildSize(ESlateSizeRule::Fill))` — types come via `Components/HorizontalBoxSlot.h`.
5. `DECLARE_DELEGATE_OneParam` above a `UCLASS` in a `.generated.h` header — the shipped non-dynamic-delegate pattern (FSiegeCloudResult precedent).
6. `Initialize()` call in `RebuildWidget` — public + idempotent, the WARN-437-1 hardening cloned verbatim from AccountMenuWidget.

## 9. M8 DECLARATION (verbatim, also in the new header)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All of it is client-local menu UI and widget-state logic.

## 10. QA scrutiny list (TASK-673)

1. **The 670 cross-task finding (672's Event-Construct `LoadDefaultDeck` cut): NOTHING in my diff depends on that cut landing.** The bar is built at the END of `NativeConstruct`, after Super fired the WBP graph — the legacy seed runs before any entry exists, touches no bar state, and is still overwritten by 670's `SelectDeckForEdit(GetActiveDeckIndex())`. If 672 does NOT cut the node, the only residue remains 670's declared benign funnel Warning per open. The Super-first ordering defense (670 §4) is preserved untouched.
2. §6.3 — the one-line append inside `SetActiveDeck` (existing API) — please rule explicitly.
3. The §3 bubbling argument (RMB reaches `NativeOnMouseButtonDown` through an unhandled SButton press) — engine-source reasoning; 674's direct-call verify + 675's hands close it.
4. The DeckBar-null lane: one Warning, `DeckBarEntries` stays empty, every refresh no-ops — 672 landing later must produce a working bar on next builder open with ZERO code change (the parallel-lanes contract).
5. Idempotence across viewport re-adds: `ClearChildren()` + `DeckBarEntries.Reset()` per `NativeConstruct` — no duplicate entries, no dangling bindings (entry delegates are per-entry and die with the entry; the entry's own `AddUniqueDynamic`/`RemoveDynamic` pair is symmetric).
6. `SetSlotIndexAndLabel` performs no fixed-name validation by design — the ONE caller passes the ONE composer's output; validating here would need a second knowledge site (DECK-§1).

---

## 11. LOOP 1 FIX — 2026-08-28 — [BLOCKER 674-1] blank deck-bar labels (qa/TASK-673.md §8)

**Files touched THIS loop: `DeckSlotEntryWidget.h` + `DeckSlotEntryWidget.cpp` ONLY.** `DeckBuilderWidget.{h,cpp}`, `SiegeDeckSaveGame.{h,cpp}`, `Tests/SiegeDeckSlotsTest.cpp` byte-untouched (mtime proof: the pair stamps 2026-08-28 01:17, the other five hold their 8/27 wave stamps). The optional caller reorder was NOT taken — order-independence makes it unnecessary, and it keeps the QA-passed caller byte-identical.

### Root cause (QA-confirmed, restated)

`DeckBuilderWidget.cpp:350` stamps `SetSlotIndexAndLabel` on a `CreateWidget`-fresh entry; the entry's children are built lazily in `RebuildWidget()`, which runs at `TakeWidget()` **inside** `:360`'s `AddChildToHorizontalBox`. At stamp time `SlotLabelText == nullptr`, and the old null-guard silently dropped the `SetText` — nothing ever re-applied it. All ten labels rendered `''`.

### Before → after

**`SetSlotIndexAndLabel` (.cpp, was :200-208)** — BEFORE: `SlotIndex = InSlotIndex;` then `if (SlotLabelText != nullptr) { SlotLabelText->SetText(FText::FromString(Label)); }` (the silent skip). AFTER: stores BOTH members unconditionally (`SlotIndex = InSlotIndex; StoredSlotLabel = Label;`) then calls `ApplyStoredLabel()`.

**`RebuildWidget` (.cpp, was :40-55)** — BEFORE: `Initialize(); ConstructEntryTree(); return Super::RebuildWidget();`. AFTER: one call inserted between `ConstructEntryTree()` and the `return` — `ApplyStoredLabel();` — so a stamp that landed pre-construction is applied the moment the tree exists (this also covers the DECK-§5(c) asset-authored escape hatch, where `SlotLabelText` arrives via BindWidgetOptional instead of construction).

**NEW private `ApplyStoredLabel()` (.h + .cpp)** — the one write site: `if (SlotIndex != INDEX_NONE && SlotLabelText != nullptr) { SlotLabelText->SetText(FText::FromString(StoredSlotLabel)); }`. The `SlotIndex != INDEX_NONE` fence means an entry that was never stamped writes nothing — an asset-authored design-time label can never be blanked by the default-empty member.

**NEW private member `FString StoredSlotLabel` (.h)** + the public doc comment on `SetSlotIndexAndLabel` updated from "Null-safe on the label child" to the order-independence contract.

### The order-independence argument

The stamp and the tree construction can now land in EITHER order and converge on the same rendered state: (a) stamp-then-tree (the shipping order) — the store survives in `StoredSlotLabel`, `RebuildWidget` applies it post-construction; (b) tree-then-stamp — `SetSlotIndexAndLabel`'s own `ApplyStoredLabel()` writes immediately since the child is alive. `ApplyStoredLabel` is idempotent (pure last-write of the stored member), so any future re-parent/re-`TakeWidget` rebuild re-applies rather than blanks — the robustness reason QA's option (a) beats the caller reorder (b), which would only pin today's call order.

### DECK-§1 one-composer law — HELD

No second label-writing path exists: `StoredSlotLabel` is the SINGLE source, written only by `SetSlotIndexAndLabel` (whose one caller passes `MakeFixedDeckName(SlotIndex)`), and `ApplyStoredLabel` is the SINGLE `SetText` site (the old inline `SetText` was replaced, not duplicated — grep: `SetText` appears exactly once in the .cpp). This class still composes no name; "deck1"/"deck10" remain comment-only in both files.

### Untouched by this loop (the QA-passed surfaces)

`SetOutlineActive` / `SetEditingHighlight` (the two-channel state rendering), `NativeOnMouseButtonDown` (the RMB path), `NativeConstruct`/`NativeDestruct`, `ConstructEntryTree`, the file-local color constants, all delegates — zero bytes changed in any of them. New unpinned privates this loop (SC-§15 declared, the §6.1 lane): `ApplyStoredLabel()` + `StoredSlotLabel`.

### For 674 loop 2

⛔ No compile was run (compile-trap law — 674 owns the slot). Re-verify: all ten labels render `deck1`..`deck10` in order at 1280 wide; spot-check the already-passed machine lane per qa/TASK-673.md §8's "spot-check only" note; re-shoot the AFTER capture set with labels.
