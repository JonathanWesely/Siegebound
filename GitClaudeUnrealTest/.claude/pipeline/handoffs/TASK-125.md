# TASK-125 — WBP_DeckCardTile: physical card art (same in-game images) + above-card in-deck copy count

**Agent:** art-director
**Date:** 2026-07-10
**Editor/MCP:** up (PID 34584). Editor-mutating; serialized after TASK-123 per overnight-auth §4.
**Status:** ready-for-integration. `/Game/UI/WBP_DeckCardTile` re-skinned, compiled clean, saved (`is_dirty=false`). NO C++, NO Git. Did not touch `WBP_CardHand`, `WBP_DeckBuilder`, `WBP_UnitHealthBar`, `WBP_CastleHealthBar`, or `WBP_MainMenu`.

## What I built (WBP_DeckCardTile only — runtime-constructed tree, per TASK-118 technique)
The tile is built at runtime in EventConstruct. New layout (root donor Overlay):
1. **Card art** — a `CardArtBorder` (new Border var) added first (fill, `SelfHitTestInvisible` so it never eats +/− clicks). Its brush is set ONCE per cell in `SetupCell` via `Appearance|SetBrushFromTexture(CardArtBorder, GetCardArtTexture(Owner, CardID))` — the SAME `T_CardArt_<CardID>` textures the in-match hand uses (`UDeckBuilderWidget::GetCardArtTexture`, DeckBuilderWidget.h:127). Null art → transparent brush = text-only fallback (spec).
2. **Above-card copy count** — a new `CopyCountText` TextBlock (font 22) inside a **top-right dark plate** (Border, brush color black @ 0.65α) — the prominent "how many in this build" number. Driven by `GetCountOf(CardID)` (the IN-PROGRESS `WorkingDeck`, DeckBuilderWidget.cpp:134-137). The old bottom `CountText` is dropped from the layout; **`/max` is gone** (the `+` cap-grey still signals the cap).
3. **Name + Cost** — in a **bottom dark caption plate** (Border, brush color black @ 0.55α, VAlign_Bottom/HAlign_Fill) → VBox[NameText 12, CostText 10, HBox[`−`,`+`]]. Driven by `GetCardDisplayName`/`GetCardCost`.
4. **+/− buttons + cap-grey KEPT** — `RemoveCopy`/`AddCopy` wiring intact; `RefreshCell` still does `SetIsEnabled(AddBtn, GetCountOf < GetCardMaxCopies)`.

Legibility guard (the manager's (c), the health-bar failure mode in miniature): the count sits on a **black 0.65 plate** and name/cost on a **black 0.55 strip** — dark backing plates so text cannot be swallowed by arbitrary bright card art (rather than relying on the art). White is the TextBlock default color.

## Node class of every BIE override the refresh path relies on — GENUINE OVERRIDES (anti-custom-event guard)
The tile's on-screen count updates via C++→BP: `AddCopy`/`RemoveCopy` → `OnDeckModelChanged`/`OnDeckSlotCountChanged` (BIE overrides **on WBP_DeckBuilder**) → `RefreshAll` → each tile's `RefreshCell`. I verified the two builder BIEs are real overrides, not the DSL-indistinguishable custom-event trap:
- `OnDeckModelChanged` = object class **`K2Node_Event`**, `type_id = AddEvent|Siegebound|Deck|EventOnDeckModelChanged`, its `then` → `RefreshAll`.
- `OnDeckSlotCountChanged` = object class **`K2Node_Event`**, `type_id = AddEvent|Siegebound|Deck|EventOnDeckSlotCountChanged` (CardID/Count params), `then` → `RefreshAll`.
Neither is `K2Node_CustomEvent`. So the C++ WILL drive `RefreshAll → RefreshCell`.

## Runtime execution proof — what I could and could NOT run
- **`[TASK125DIAG]` added inside `RefreshCell`:** `PrintString "[TASK125DIAG] count=<N> <DisplayName>"` — fires on every refresh, i.e. on every +/−. **This is left LIVE** (mirror of the sanctioned `[TASK122DIAG]` pattern) so Jonathan's manual +/− click produces a log proving the count updates. **TASK-126 must strip it before commit** (mirror TASK-128).
- **I could NOT machine-execute the +/− click.** The deck builder only constructs from the main-menu button (needs a click; locked desktop = no SendInput), there is no MCP UFUNCTION/exec tool to call `AddCopy` or open the builder, and `WorkingDeck` is a protected transient (not settable). So I could not fire `RefreshCell` at runtime myself. The genuine-override node-class proof above is the strongest static guarantee that the path fires; the live log is for Jonathan.

## HUMAN WATCH owed to Jonathan (never inferred from the tree — screen-space Slate is uncapturable, desktop locked)
1. **Count VISIBLE + updates on +/−:** open Deck Builder, click +/− on a tile, confirm the top-right number changes; the `[TASK125DIAG]` log will corroborate each click.
2. **Legibility over art:** confirm the count, name, and cost read clearly on top of the card art (dark plates are the guard; verify they suffice).
3. **Art renders:** confirm each tile shows its `T_CardArt_<CardID>` image like the hand.
TASK-126 carries these (build-master is also headless on the locked desktop — same limits).

## Honest flags / follow-ups
- **Card sizing:** the art Border fills the CONTENT-sized overlay (the tile has no fixed card size — that lives in WBP_DeckBuilder's WrapBox slot, which I was told not to touch). So the card is compact, not a large card face. Sizing polish is a follow-up (M7 / a WBP_DeckBuilder tile-slot size tweak).
- **Graph cruft (carry-forward of TASK-118 flag #6):** the AssignOnClicked MCP quirk recurred when the runtime construct re-created the buttons — the buttons ended up bound to auto-created empty events `OnClicked_Event_13`(remove)/`_14`(add). I fixed it by authoring the `RemoveCopy`/`AddCopy` bodies INTO those bound events (verified: both bodies present + wired, single instances, no duplicates). Consequently `OnAddPressed`/`OnRemovePressed` are now orphaned duplicate handlers (harmless, unbound), alongside the pre-existing donor cruft (`OnClicked_Event_*`, StickInput, Btn_Jump). Benign "No execute/then pin" compile warnings come from these unreachable nodes. M7 cleanup; does not affect the live path.
- **CardArtImage var:** an unused leftover Image var from a first approach (SetBrushFromTexture resolved to the Border overload, so I used a Border). Harmless; remove in an M7 sweep.

## Files
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\UI\WBP_DeckCardTile.uasset` (saved, compiled clean)
- Resolvers (read-only, unchanged): `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.h`
