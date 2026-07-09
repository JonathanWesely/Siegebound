# TASK-118 handoff — WBP_DeckBuilder screen (art / editor)

**Status:** ready-for-integration (art skips QA → build-master integration check at TASK-120)
**Scope:** editor/MCP only. Editor UP (PID 4948, MCP :8000); `IsPIERunning=false` verified before AND after all mutation — **no PIE started** (TASK-120 owns PIE verification). No gameplay code, no Git, no TASKBOARD.md edits.

## Assets delivered (both saved, is_dirty=false, compile clean)
1. **`/Game/UI/WBP_DeckBuilder`** — the deck-builder screen.
   - **Parent class: `UDeckBuilderWidget`** (`/Script/GitClaudeUnrealTest.DeckBuilderWidget`) — **readback-confirmed via `get_parent` after reparent AND after all edits AND after save** (this is the TASK-111 crux: a silently-failed reparent = a dead screen; it took).
   - Donor: duplicated from **`/Game/UI/WBP_MainMenu`** (the L_MainMenu full-screen menu — closest to the deck-builder's role as a full-screen overlay on L_MainMenu, cleanest round-trippable Construct, plain-UserWidget parent). Its menu Construct was fully replaced.
2. **`/Game/UI/WBP_DeckCardTile`** — NEW helper sub-widget (one card cell), duplicated from `WBP_MainMenu`, parent stays `UUserWidget`. **This is a required dependency of WBP_DeckBuilder — build-master must commit BOTH at TASK-120.**

## Architecture (why a sub-widget)
The 28-card grid needs each `+`/`−` click routed to `AddCopy(CardID)`/`RemoveCopy(CardID)` for the RIGHT card. A raw UMG `Button.OnClicked` carries no sender, so per-index events would explode to 56 hand-unrolled handlers. Instead each grid cell is a **`WBP_DeckCardTile`** instance that holds its own `OwnerBuilder` (the `UDeckBuilderWidget`) + `CardID`; the cell's buttons call `OwnerBuilder.AddCopy/RemoveCopy(CardID)` **directly** (pure "call the C++ API" — the WBP never touches DT_Cards). This scales the grid with one runtime loop.

Both trees are built by **runtime widget construction** (`ConstructObjectFromClass`/`CreateWidget` + `AddChild*` in the graph, refs stored in member vars) — the proven TASK-041 technique; there is no MCP widget-tree authoring tool. Root panel reached via the donor's `CastToOverlay(GetParent(GetParent(Btn_Jump)))`.

## WBP_DeckCardTile — element inventory
Members: `OwnerBuilder`(DeckBuilderWidget), `CardID`(name), `NameText`/`CostText`/`CountText`(TextBlock), `AddBtn`/`RemoveBtn`(Button).
- **EventConstruct**: collapses donor cruft (Btn_Jump + 2 thumbsticks), builds Border→VBox[NameText, CostText, HBox[RemoveBtn "−", CountText, AddBtn "+"]], stores refs, binds the two buttons, then calls `RefreshCell`.
- **`OnAddPressed`** → `OwnerBuilder.AddCopy(CardID)` · **`OnRemovePressed`** → `OwnerBuilder.RemoveCopy(CardID)` (both IsValid-guarded on OwnerBuilder). Button bindings readback-verified after the granular AssignOnClicked-rewire fix (see "MCP quirk").
- **`SetupCell(Owner, InCardID)`** (fn): stores Owner+CardID, calls RefreshCell.
- **`RefreshCell()`** (fn, null-guarded): `NameText`=`GetCardDisplayName`, `CostText`="Cost "+`GetCardCost`, `CountText`=`GetCountOf`+"/"+`GetCardMaxCopies`, and **`SetIsEnabled(AddBtn, GetCountOf < GetCardMaxCopies)`** (this is the "+"-greys-at-MaxCopies gate).

## WBP_DeckBuilder — element inventory + which C++ API each drives
Members: `CardTiles`(array of WBP_DeckCardTile), `TotalText`/`AvgText`/`SavedNamesText`(TextBlock), `NameInput`(EditableText), `PlayBtn`(Button).

**EventConstruct** builds root VBox → title "Deck Builder" → **WrapBox grid**: `for cid in GetCollectionCardIDs()` → CreateWidget(WBP_DeckCardTile_C) → cast → AddChildToWrapBox → `SetupCell(self, cid)` → add to `CardTiles`. Then the bottom controls, then seeds: `LoadDefaultDeck()` → `RefreshAll()` → `RefreshSavedNames()`.

| Element | C++ API it drives |
|---|---|
| 28-card browser grid (one WBP_DeckCardTile per card) | `GetCollectionCardIDs` (build) → per tile `GetCardDisplayName`/`GetCardCost`/`GetCountOf`/`GetCardMaxCopies`; `+`→`AddCopy`, `−`→`RemoveCopy` |
| `TotalText` "Deck: x/50" | `GetTotalCount` |
| `AvgText` "Avg cost: N" (§8 guide) | `GetAverageCost` |
| `SavedNamesText` "Saved: …" | `GetSavedDeckNames` (JoinStringArray) |
| `NameInput` (name field) + **Save** btn | `SaveDeckAs(NameInput text)` then re-reads `GetSavedDeckNames` |
| **Load** btn | `LoadDeck(NameInput text)` |
| **Reset to Default** btn | `LoadDefaultDeck` |
| **`PlayBtn`** "Play With This Deck" | enabled ONLY at `IsCurrentDeckLegal` (set in `RefreshAll`); OnClick = `SaveDeckAs("Active")` → `SetActiveDeck("Active")` → `ASiegeGameMode::StartMatch` (the documented persist-then-activate order; PlayBtn is legal-gated so the saved "Active" deck is always the 50-card legal one) |
| **Back** btn | `RemoveFromParent(self)` + CreateWidget `WBP_MainMenu` + AddToViewport |

**Seed-then-bind:** `RefreshAll()` (guarded fn) sets Total/Avg/Play-enabled + loops `CardTiles.RefreshCell()`. The two C++ BIEs are **overridden** (readback: `OnDeckModelChanged` + `OnDeckSlotCountChanged` both `bIsImplemented:true` with the C++ doc-comments) and each calls `RefreshAll()` — so every add/remove/load/save re-reads the getters. Construct seeds first, BIEs keep it live.

## Robustness notes
- **Tile data population is double-safe against Construct-timing**: each tile calls its own `RefreshCell` at the end of its Construct AND the parent calls `RefreshAll`+`SetupCell` — all null-guarded — so counts/names populate whether the child Construct fires synchronously on AddChild or is deferred.
- **AssignOnClicked MCP quirk (TASK-041) recurred and was fixed**: `Button|Event|AssignOnClicked` auto-creates an empty `OnClicked_Event_N` and binds the button to THAT, orphaning the named handler. Fixed deterministically for all 7 buttons (2 on the tile, 5 on the builder) by `break_pins` + `connect_pins` rewiring each button's AssignDelegate.Delegate pin to the real handler's OutputDelegate; **readback confirms** every button now bound to its real handler.

## Honest flags for TASK-120 (things I could NOT verify without PIE, or deliberately simplified)
1. **No runtime verification** — per the constraint I did not start PIE. Structure/compiles/bindings are readback-verified; **the actual on-screen render + interaction is unverified.** TASK-120 must PIE-check: grid shows 28 tiles with names/costs/counts; `+`/`−` change counts; `+` greys at MaxCopies; `TotalText` tracks x/50; `AvgText` updates; PlayBtn enabled only at 50; Save/Load/Reset work; Back returns to menu.
2. **Saved-deck "list" is a READOUT + name-field Load, not clickable per-row buttons.** `SavedNamesText` shows "Saved: A, B, C" (from `GetSavedDeckNames`); the player types a name in `NameInput` and clicks **Load** (`LoadDeck`). This uses both required APIs and is fully functional, but is not a click-each-row list. Flag for M7 polish if clickable rows are wanted (would reuse the same OwnerBuilder-payload sub-widget pattern).
3. **Layout is functional, not visually polished** — plain VerticalBox + WrapBox, minimal padding, no ScrollBox. 28 tiles in a WrapBox on a full-screen menu: **if they overflow the screen, a ScrollBox wrap is needed** (M7 / quick polish). TASK-120 please eyeball the fit and note it.
4. **`NameInput` is an `EditableText`** (not EditableTextBox) — only `Widget|GetText(EditableText)` was available via MCP; functionally identical, just no box border. No hint text set.
5. **Input mode**: I did not touch input mode; the screen assumes the menu's UIOnly + visible-cursor posture (BP_MenuGameMode). TASK-119 opens it from the menu under that same posture.
6. **Harmless cruft** (do not let it confuse review; M7 sweep): both graphs retain orphaned donor events (`OnClicked_Event`/`_0`/`_4`, touch/thumbstick events) and the empty auto-created `OnClicked_Event_8..12` (tile: `_8`/`_9`) left over from the AssignOnClicked quirk. All unbound/unreachable.

## For TASK-119 (enable + wire the main-menu Deck Builder button)
- Target exists: `/Game/UI/WBP_DeckBuilder` (class `WBP_DeckBuilder_C`). Open it via `CreateWidget(WBP_DeckBuilder_C)` + `AddToViewport`, then `RemoveFromParent` the main menu (standard overlay nav on L_MainMenu). WBP_DeckBuilder's **Back** button already performs the reverse.

## Source / paths
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_DeckBuilder.uasset`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_DeckCardTile.uasset`
- Donor (unmodified): `/Game/UI/WBP_MainMenu`
- No Blender/RawAssets output (pure UMG editor assets).
