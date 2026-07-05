# TASK-041 — WBP_CardHand visual hand UI + HUD stat texts — HANDOFF (COMPLETE via MCP)

- author: gameplay-programmer
- date: 2026-07-05 (MCP-UMG retry; editor UP PID 19464, healthy the whole task)
- status: **COMPLETE / ready-for-qa.** BOTH the WBP_HUD stat texts AND the full WBP_CardHand 6-slot
  interactive hand shipped via MCP. Footman button removed. Both blueprints compile clean, saved
  (is_dirty=false), PIE-boot-verified in L_Arena with ZERO runtime errors. Editor left UP, PIE stopped.

## Headline
The M2/TASK-033 deferral is CLOSED. Unlike the TASK-033 attempt, MCP-driven UMG was rock-solid on this
fresh boot (~140 MCP calls, survived end-to-end, transport verified alive at the end). The full visual hand
tree WAS authored via MCP — not by a designer — using **runtime widget construction** (ConstructObjectFromClass
+ AddChild/AddChildToOverlay) exactly like the proven TASK-050/064 pattern, scaled up to a 6-slot loop.

## MCP invocation note (for whoever runs MCP next)
`mcp__unreal-mcp__call_tool` dispatch shape is `{tool_name:<short>, toolset_name:<registry id>, arguments:{...}}`.
The arguments key is **`arguments`** (NOT `parameters`), and a toolset tool needs BOTH `tool_name` and
`toolset_name` (e.g. toolset_name `editor_toolset.toolsets.blueprint.BlueprintTools`, tool_name `write_graph_dsl`).

---

## PART 1 — WBP_HUD stat texts (ADDITIVE; gold Construct PROTECTED)

Followed TASK-050/064 exactly: bulk logic in NEW function graphs (safe fresh-graph `write_graph_dsl`) + a single
granular `create_node` append onto the EventGraph Tick do-once. **ZERO `write_graph_dsl` on the EventGraph** — the
M1 gold Construct's `GetDataTableRowDT_Cards` is the known-lossy node, so it was never round-tripped. Readback
confirms the Construct is **byte-intact** (gold text create, SetGoldText, UpdateGoldDisplay, AssignOnGoldChanged,
AssignOnClicked, overlay attach MakeMargin 24/12 — all identical).

**3 new member vars** (TextBlock refs): `GoldRateText`, `MinerCountText`, `OvertimeText`.

**5 new function graphs:**
- `UpdateGoldRateDisplay(NewRate:int)` — IsValid-guarded; SetText `"+<N>/s"` (BuildString(Integer) "" "+" N "/s").
- `UpdateMinerCountDisplay(AliveCount:int)` — IsValid-guarded; SetText `"<N>/6"`. The **"6" is a static constant**
  (documented mechanic constant — `MaxActiveMiners`; there is no `GetMaxActiveMiners` BP getter, same justification
  class as the discard "1"). If QA wants it data-driven, needs a new C++ getter (no compile this task).
- `UpdateOvertimeDisplay(bActive:bool)` — IsValid-guarded; SetText "OVERTIME"; visibility HitTestInvisible if active
  else Collapsed.
- `ShowOvertime()` — no-param wrapper → `UpdateOvertimeDisplay(true)` (the `OnOvertimeStarted` delegate is no-param).
- `SetupStatTexts()` — constructs the 3 TextBlocks + a VerticalBox, attaches top-RIGHT of the root Overlay
  (HAlign_Right/VAlign_Top, MakeMargin L0/T44/R24/B0 — sits BELOW the TASK-050 Rally indicator at T12/R24),
  **SEEDS** all three from getters (`GetGoldRate`, `GetAliveMinerCount`, `IsOvertimeActive`), THEN binds the 3
  delegates. Overtime seeded Collapsed at construct.

**Bindings wired (seed-then-bind, CONVENTIONS honored):** inside `SetupStatTexts`, after seeding:
- `Siegebound|Gold|BindEventtoOnGoldRateChanged` (on the cast SiegePlayerState) → CreateEvent → `UpdateGoldRateDisplay`
- `Siegebound|Miners|BindEventtoOnMinerCountChanged` (SiegePlayerState) → CreateEvent → `UpdateMinerCountDisplay`
- `Siegebound|Match|BindEventtoOnOvertimeStarted` (cast SiegeGameState via GetGameState) → CreateEvent → `ShowOvertime`
- CreateEvent delegates assigned via `set_create_event_function` AFTER connecting each OutputDelegate to the bind's
  Delegate pin (the ordering requirement TASK-064 flagged). PlayerState cast uses `Class|Controller|GetPlayerState`
  (NOT `Game|GetPlayerState`, which is the index overload and fails to connect). GameState cast nested inside the
  PlayerState cast's `:then` (the "casts-with-continuations terminate the flow" rule — TASK-064 nesting trap).

**EventGraph Tick do-once EXTENDED** (granular append, one node): `CallFunction|SetupStatTexts` wired onto the free
`then` of the existing `BindEventtoOnHeroUpgradesChanged` node (the previous chain tail). Rally + upgrade chains untouched.

## PART 2 — WBP_CardHand 6-slot interactive hand (full tree AUTHORED via MCP)

WBP_CardHand's EventGraph has NO lossy nodes, so it was safe to author wholesale via `write_graph_dsl`.

**8 new member vars:** arrays `SlotNameTexts`/`SlotCostTexts`/`SlotBoxes`(VerticalBox)/`SlotPlayButtons`/`SlotDiscardButtons`
(Button); singles `PreviewNameText`, `PreviewCostText`, `RefusalText`.

**Function `BuildHandTree()`** (runtime tree construction, `for i (range 6)` loop): per slot builds a VerticalBox
containing [NameText(16pt), CostText(14pt), PlayButton→"Play" label, DiscardButton→"1" label], appends each widget
to the matching array, adds the slot VBox to a HorizontalBox `_handbox`. Then builds the preview VBox
(static "Next:" label + PreviewNameText + PreviewCostText) and the RefusalText (collapsed). Attaches: hand-box
bottom-CENTER (VAlign_Bottom, MakeMargin B24), preview bottom-RIGHT (R24/B24), refusal center. CastFailed logs
"WBP_CardHand: root Overlay not found; hand not attached". **No costs/names typed into UMG** — all runtime text
comes from the BIEs. The "Play" / "1" labels are static mechanic strings (the §3.6/§7 flat 1-gold discard).

**3 BIE overrides implemented directly in the EventGraph** (ubergraph — required for the latent refusal delay):
- `EventOnHandSlotUpdated(SlotIndex, CardID, DisplayName, Cost, bAffordable)` — index SlotBoxes[SlotIndex],
  IsValid-guard; **empty CardID ⇒ SetVisibility(box, Collapsed)** (hide the face — keyed off empty CardID, NOT
  bAffordable, per the header contract); else SelfHitTestInvisible + SetText name (DisplayName) + SetText cost
  (`"<Cost>g"`) + **SetIsEnabled(PlayButton, bAffordable)** (the §3.5 grey/disable when unaffordable).
- `EventOnNextCardUpdated(DisplayName, Cost)` — empty DisplayName ⇒ collapse both preview texts; else show
  DisplayName + `"<Cost>g"`.
- `EventOnCardRefusedMessage(Reason)` — SetText(RefusalText, Reason) + show, then `RetriggerableDelay 2.0` →
  SetVisibility Collapsed (the §3.0 ~2 s show-then-fade; retriggerable so rapid refusals re-arm the timer).

**Construct** rewritten to: `BuildHandTree()` → `SetVisibility(self, SelfHitTestInvisible)` → 12 button binds →
`CastToSiegePlayerController → InitForController(self, pc)` (the TASK-033/029 data-path entry, preserved).

**Play/discard button click wiring (12 buttons → RequestPlaySlot/RequestDiscardSlot):** Each slot's play button
OnClicked → `RequestPlaySlot(i)`, each discard button OnClicked → `RequestDiscardSlot(i)` (0-based). 12 custom
events `OnPlaySlot0..5` / `OnDiscardSlot0..5` carry the literal-index RequestSlot calls; each button's
`AssignOnClicked` node's Delegate pin is granularly connected to the matching event's OutputDelegate. VERIFIED via
node readback: AssignDelegate_14→OnPlaySlot0 … AssignDelegate_25→OnDiscardSlot5, self pins ← the SlotPlayButtons/
SlotDiscardButtons array-get. (See "MCP quirk" below for why this needed a granular pass.)

## WARN-4 hit-test posture (final)
Root `WBP_CardHand` = **SelfHitTestInvisible** (set in its own Construct AND the WBP_HUD Tick spawn was changed
from `Collapsed`→`SelfHitTestInvisible` so the hand actually shows). Slot VerticalBoxes = SelfHitTestInvisible;
the interactive children (buttons) are default **Visible** ⇒ cards clickable under Alt-cursor / placement mode,
gameplay LMB is NOT swallowed by the root. Empty slots collapse (no phantom faces).

## Footman button — REMOVED (collapsed, not deleted — and why)
The M1 Footman card button is the donor `Btn_Jump`. It **cannot be deleted** via MCP: (a) there is no widget-tree
delete tool, and (b) it is **load-bearing** for the PROTECTED gold Construct — the gold text's overlay attach and
the Rally/upgrade/stat-text attaches all navigate `GetParent(GetParent(Btn_Jump))` to reach the root Overlay, and
a temp "Footman (2)" label is AddChild'd to it. So removal = a granular Tick append `SetVisibility(Btn_Jump,
Collapsed)` at the END of the do-once (after all attach logic + SetupStatTexts have run). A Collapsed button is
invisible + non-hit-testing (effectively removed for the player) while remaining in the hierarchy as the layout
anchor. Its `OnClicked_Event`→EnterPlacementMode("Footman") binding still exists but can never fire (collapsed) —
harmless dead code. **No UI gap:** the 6-slot hand (with Footman as a card) + hotkeys 1–6 cover play. Zero change
to the gold counter (byte-intact, confirmed by readback + PIE).

## Preservation (confirmed by full EventGraph readback + PIE)
- M1 gold counter Construct: **byte-intact** (never round-tripped).
- M3 Rally indicator (TASK-050): intact.
- M4 upgrade row (TASK-064): intact.
- WBP_CardHand InitForController data path (TASK-033/029): preserved (BuildHandTree runs BEFORE InitForController
  so widgets exist when the C++ seed fires the BIEs).

## Verification
- Both blueprints `compile_blueprint` clean (null). Both saved, is_dirty=false on disk.
- Structural readback of every graph after writes: Construct byte-intact; Tick tail = SetupStatTexts →
  SetVisibility(Btn_Jump,Collapsed); WBP_CardHand button binds mapped correctly.
- **PIE boot (real PIE, L_Arena, 5 s warmup): ZERO runtime errors** — no "Accessed None", no "Blueprint Runtime
  Error", and crucially NONE of my CastFailed PrintStrings ("root Overlay not found", "not bound", "hand left
  unbound", "hand not attached"). ⇒ every cast succeeded (overlay found, PlayerState + GameState + controller cast
  OK) and all delegates bound. LogSiegeBot shows a healthy match; StopPIE clean.
- **Interactive play (click a card, watch a slot grey when gold drops, discard→redraw, refusal fade) is
  Jonathan's** — MCP cannot inject card plays or clicks. The bindings + seed + structure are delivered and
  boot-verified.

## MCP quirk documented (the one non-obvious thing) — NOT a gap, already worked around
`Button|Event|AssignOnClicked(btn, AddEvent|Custom|<myName>)` **ignores `<myName>`** and auto-creates an empty
event `OnClicked_Event_N` bound to the button, leaving any same-named standalone `(event Custom|<myName> body)`
orphaned. So the first full EventGraph write left the 12 buttons bound to EMPTY handlers while my 12
`OnPlaySlot/OnDiscardSlot` body-events sat unbound. FIXED deterministically: read the node graph, mapped each
button's AssignOnClicked node to its slot, and `connect_pins`'d each body-event's OutputDelegate onto the button's
Delegate pin (replacing the empty auto-event). Compile clean, verified. **Nothing here is left for a human.**

## Cosmetic cruft (harmless, unreachable — do NOT let it confuse QA; M7 sweep)
- WBP_CardHand: 13 empty auto-created `OnClicked_Event_3..15` custom events (the AssignOnClicked side-effects,
  now disconnected after the rewire) + inherited donor dead code (`OnGoldChanged_Event*`, `UpdateGoldDisplay`,
  the donor `OnClicked_Event` Footman stub). All unreachable — nothing binds them. MCP has no delete-event tool
  worth the compiles; left for the designer sweep.
- WBP_HUD: empty numbered stub events `OnGoldChanged_Event_*/OnClicked_Event_*/OnRallyStateChanged_Event_*` from
  node churn (same class TASK-050/064 documented). Empty bodies, harmless.
- Stale log warnings from mid-task (do NOT chase): three `"Update*Display"/"ShowOvertime" is not a compatible
  function` at ~05.40 — the set_create_event_function calls I made BEFORE connecting the delegate pins; superseded
  by the successful post-connect assignments (final compile + PIE are clean).

## Nothing left for a human designer
The full spec shipped via MCP: 6 slots (name+cost, grey/disable on unaffordable), play button → RequestPlaySlot,
discard button "1" → RequestDiscardSlot, next-card preview, ~2 s refusal message, all 3 BIEs rendered, WARN-4
posture, HUD gold-rate/miner/overtime seeded+bound, Footman button removed. Optional M7 visual polish (card art,
fonts/colors, animated refusal fade, real pip glyphs) is out of scope. If QA wants the runtime-constructed slots
re-authored as editable DESIGNER widgets (so Jonathan can restyle them in the UMG designer), that is a separate
art/polish task — the C++ BIE contract + array-index model would carry over unchanged.

## Files touched
- `/Game/UI/WBP_HUD.uasset` — modified + saved (stat texts, hand-visible, Footman collapsed; gold Construct intact).
- `/Game/UI/WBP_CardHand.uasset` — modified + saved (full 6-slot interactive hand + 3 BIE render events).
- `.claude/pipeline/handoffs/TASK-041.md` — this file.
- NOT touched: C++ (shipped aafd968), Git, TASKBOARD.md, WBP_VictoryScreen, L_Arena (loaded/PIE'd only).
- Auto-staged `.uasset` changes left for build-master to commit (separate from TASK-070's L_Arena.umap).
