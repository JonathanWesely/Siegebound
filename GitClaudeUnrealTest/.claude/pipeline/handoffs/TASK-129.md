# TASK-129 — Strip the [TASK125DIAG] diagnostic from WBP_DeckCardTile RefreshCell

**Agent:** art-director
**Date:** 2026-07-10
**Editor/MCP:** up. Editor-only Blueprint node removal — no C++, no Git.
**Status:** ready-for-integration. `/Game/UI/WBP_DeckCardTile` diagnostic stripped, saved (`is_dirty=false`). Only `WBP_DeckCardTile.uasset` touched.

## What I removed
The temporary `[TASK125DIAG]` `PrintString` node in `RefreshCell` — the one that logged `"[TASK125DIAG] count=<N> <DisplayName>"` on every refresh (left live in TASK-125 for Jonathan's manual +/− WATCH, now satisfied: *"the deck builder fixes are fine"*). Removing it also dropped the now-unused `GetCardDisplayName` bind that had fed both the print and the name text; the name text now calls `GetCardDisplayName` directly (a pure getter — identical result).

## Readback confirming the production path survives (post-strip `RefreshCell`)
```
(fn RefreshCell ()
  (bind _cardid (Variables|Default|GetCardID))
  (bind _ownerbuilder (Variables|Default|GetOwnerBuilder))
  (bind _returnvalue (Siegebound|Deck|GetCountOf _ownerbuilder _cardid))
  (Utilities|IsValid _ownerbuilder
    (:"Is Valid"
      (Widget|SetText(Text) (Variables|Default|GetCopyCountText) (ToText (ToString _returnvalue)))     ; above-card count
      (Widget|SetText(Text) (Variables|Default|GetNameText)  (ToText (GetCardDisplayName ...)))          ; name
      (Widget|SetText(Text) (Variables|Default|GetCostText)  (ToText "Cost " + (GetCardCost ...)))       ; cost
      (Widget|SetIsEnabled  (Variables|Default|GetAddBtn) (< _returnvalue (GetCardMaxCopies ...))))))    ; +/- cap-grey
```
- **Count TextBlock** (`CopyCountText`) still driven by `GetCountOf` (plain in-deck count). ✓
- **Name / Cost** still driven by `GetCardDisplayName` / `GetCardCost`. ✓
- **`+` enable-state (cap-grey)** still `SetIsEnabled(AddBtn, GetCountOf < GetCardMaxCopies)`. ✓
- **No `PrintString` / `[TASK125DIAG]` node remains.** ✓

Per the task, a graph readback confirming the production path is sufficient for a diagnostic-strip — no full PIE run needed. (`write_graph_dsl` recompiled the function as part of the edit; the tile compiled clean and saved.)

## Constraints honored
No Git. No C++. Did not touch `WBP_UnitHealthBar` / `WBP_CastleHealthBar` / `WBP_MainMenu` or any health-bar asset (TASK-130/131/132 territory). Did not terminate the editor. Saved ONLY `WBP_DeckCardTile.uasset`. build-master (TASK-126) can now commit the clean tile.

## Files
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_DeckCardTile.uasset` (saved, diagnostic-free)
