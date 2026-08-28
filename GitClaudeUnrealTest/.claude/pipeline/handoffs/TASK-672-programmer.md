# TASK-672 — [DB3-4] THE GRAPH SURGERY — programmer handoff (2026-08-28)

Status: **ready-for-qa** (TASK-673 reviews 670+671+672 in one report; TASK-674 owns the compile slot — QUIET-MODULE honored, **zero C++ compile run**; the WBP-internal compile ×2 below is the TL-§4 blueprint compile, sanctioned by the spec's own step 7). Board flip proxied by the orchestrator (dispatch fence barred board writes — the TASK-671 precedent, declared not skipped).

Executed against the AMENDED spec (manager ruling 2026-08-27 on the TASK-669 record). Editor **PID 9072** (667's binary) throughout; MCP lane = scratchpad `mcp_client.py`/`u.py` + new `t672_ops.py`; design-time lane = UE Python remote execution (667's `bRemoteExecution=True` cargo — first productive use). ⛔ No Source/ edit, no git, no TASKBOARD edit, no PIE/SIE/console/`M`, no viewport moves, no L_Arena or foreign-WBP save. `IsPIERunning=false` at start and finish.

## 0. INSTRUMENT DECLARATION (669 §5.4 law)

**Node-level reads ONLY**: `find_nodes` + `get_node_infos` (batched, full pin detail). `read_graph_dsl` was **never called** — proven lossy by 669. Every op below carries a before/after `get_node_infos` read; the raw JSONs live in the session scratchpad (`t672_g*_before/after.json`, `t672_eg_nodes_pre/post.json`, `t672_refreshall_pre/post.json`) and the compact per-node lines are pasted here. Compact line format: `id | type | lits[literal pins] | src[data inputs] | prev=exec-in | out[exec/data outs]`.

## 1. THE LIVE RE-VERIFY (RELAYED-DIAGNOSIS — before ANY cut)

Full fresh `get_node_infos` dump of all EventGraph nodes vs 669's `t669_eg_nodes.json`:

- **Live = 218 nodes, 669 record = 219. The ONE delta: `K2Node_CustomEvent_23` (an `OnClicked_Event_18` stub — zero connected pins, one of 669 §5.3's "~17 empty OnClicked_Event_N stubs") is absent live.** Disk was untouched between the tasks (mtime Jul 26 stood; git clean), so this is in-memory node-reconstruction variance between 669's editor instance (PID 28772) and PID 9072's fresh load of the same bytes. Not a target, exec-dead residue, no bearing on any op.
- **All 218 common nodes: signature-identical** (type + every pin name/type/value/connection) to 669's record — including every target node. Each op group below additionally re-read its targets immediately before cutting and asserted identity (e.g. 87 == `Siegebound|Deck|LoadDefaultDeck` self←Self_1; 109/110 `Name=Active` literals) — asserts pasted in §3–§8.
- `RefreshAll` graph freshly dumped (23 nodes) — matches 669's DSL rendering; the enable-gate = graph-local `K2Node_CallFunction_9` (`Widget|SetIsEnabled`, self←`VariableGet_2 GetPlayBtn`, bInIsEnabled←`CallFunction_8 IsCurrentDeckLegal`). ⚠️ Note for QA: **node IDs are graph-scoped** — RefreshAll's `Self_0`/`MacroInstance_0`/`CallFunction_9` are NOT the EventGraph's chain-A nodes of the same names.

## 2. GROUP 1 — THE NO-CLIP FIX (DECK-§6 rider) ✅

New node **`K2Node_CallFunction_91`** = `Layout|VerticalBoxSlot|SetSize`, created via `create_node`; wired self ← `CallFunction_50.ReturnValue` (the MainVBox←GridRow VerticalBoxSlot — the measured unused pin), `InSize = (SizeRule=Fill,Value=1.000000)` (set + read back), exec-inserted 50→91→MacroInstance_0.

BEFORE:
```
K2Node_CallFunction_50 | Panel|AddChildToVerticalBox | src[self<-GCO_7 Content<-VariableGet_15] | prev=CF_139 | out[then->K2Node_MacroInstance_0]   (ReturnValue UNUSED)
```
AFTER (post-save re-read):
```
K2Node_CallFunction_50 | ... | out[then->K2Node_CallFunction_91 ReturnValue->K2Node_CallFunction_91]
K2Node_CallFunction_91 | Layout|VerticalBoxSlot|SetSize | lits[InSize=(Value=1.000000,SizeRule=Fill)] | src[self<-K2Node_CallFunction_50] | prev=K2Node_CallFunction_50 | out[then->K2Node_MacroInstance_0]
K2Node_MacroInstance_0 | Utilities|Array|ForEachLoop | prev=K2Node_CallFunction_91 | (LoopBody/Completed unchanged)
```
(The InSize string reads `(SizeRule=Fill,Value=1.000000)` pre-save and `(Value=1.000000,SizeRule=Fill)` after — serializer member ordering, same struct value. Declared.)

## 3. GROUP 2 — THE SEED CUT (DECK-§4b) ✅

Identity re-verified then cut: **`K2Node_CallFunction_87` (`Siegebound|Deck|LoadDefaultDeck`, self←Self_1, prev=AssignDelegate_7, next=88) DELETED**; rewire `AssignDelegate_7.then → CallFunction_88 (RefreshAll)`. ⛔ No code-side sibling exists or was added (670's NativeConstruct ordering untouched — zero Source edits this task).

BEFORE: `AssignDelegate_7 out[then->CF_87]` · `CF_87 | Siegebound|Deck|LoadDefaultDeck | prev=AssignDelegate_7 | out[then->CF_88]`
AFTER: `AssignDelegate_7 out[then->K2Node_CallFunction_88]` · `CF_88 | RefreshAll | prev=K2Node_AssignDelegate_7` · **node 87 ABSENT** (find_nodes sweep, §9).

## 4. GROUP 3 — THE PLAY REWRITE (DECK-§4c, amended D8) ✅

1. Identity re-verified (109 = `SaveDeckAs Name=Active`, 110 = `SetActiveDeck Name=Active`, 111 = `StartMatch`) then **109 + 110 DELETED**.
2. `CustomEvent_17 (OnPlayClicked).then → CallFunction_111 (StartMatch)` connected DIRECT.
3. Label literal: `CallFunction_79.InString` `"Play With This Deck"` → **`"Play"`** (set + read back; CF_79 feeds CF_80 SetText on GCO_22, the PlayBtn label — the pin located per the node record).
4. Enable-gate REMOVED from `RefreshAll` — **exact nodes deleted: `K2Node_CallFunction_9` (SetIsEnabled) + `K2Node_CallFunction_8` (IsCurrentDeckLegal) + `K2Node_VariableGet_2` (GetPlayBtn)** (graph-local IDs); rewire `CallFunction_7 (AvgText SetText).then → MacroInstance_1 (ForEach tiles)`. RefreshAll 23 → 20 nodes; TotalText "n/50" feedback untouched.
5. ⛔ **No node calls the new C++ symbols** — post-op sweep over every node signature for `SetActiveDeckBySlot`/`GetEditingDeckIndex`/`SelectDeckForEdit`/`GetActiveDeckIndex`: **zero hits** (§9).

AFTER: `CustomEvent_17 out[OutputDelegate->AssignDelegate_6 then->K2Node_CallFunction_111]` · `CF_111 | StartMatch | prev=K2Node_CustomEvent_17` · `CF_79 lits[InString=Play]` · RA: `CF_7 out[then->K2Node_MacroInstance_1]`.

## 5. GROUP 4 — THE DECKBAR CONTAINER (route (i), DECK-§7 rider) ✅

**Design-time half (remote-exec lane — the TL-§4 children lane):** `DeckBar` (`/Script/UMG.HorizontalBox`) created with `unreal.new_object(outer=WidgetTree, name='DeckBar')` and added via `Overlay_19.add_child_to_overlay` → `OverlaySlot_1`, **HAlign_Fill / VAlign_Top**, empty (0 children — 671's C++ populates; zero code change needed, the design's point). Root child set after: `SizeBox_0, Thumbstick_Move, Thumbstick_Aim, DeckBar` (4 children). Post-save readback: slot `OverlaySlot_1` HA=`H_ALIGN_FILL` VA=`V_ALIGN_TOP` persisted. Root identity held (Overlay_19, slot=None). No duplicate+reparent anywhere; no other widget created; `/Game/UI/WBP_DeckSlotEntry` untouched/reserved.

**The ONE graph op:** new **`K2Node_CallFunction_92`** = `Layout|OverlaySlot|SetPadding`, self ← `CallFunction_43.ReturnValue` (MainVBox's OverlaySlot — the 44/45 chain), `InPadding = (Left=0,Top=64,Right=0,Bottom=0)` (read back), exec-inserted 45→92→GCO_8. Top=64 = 671's declared entry height 48 + 2×8 bar padding (DECK-§7 rider defaults).

AFTER: `CF_45 out[then->K2Node_CallFunction_92]` · `CF_92 | Layout|OverlaySlot|SetPadding | lits[InPadding=(...Top=64.000000...)] | src[self<-K2Node_CallFunction_43] | out[then->K2Node_GenericCreateObject_8]`.

## 6. GROUP 5 — EXIT (DECK-§4a; D6 substance = the binding) ✅

1. **Relabel:** `CallFunction_83.InString` `"Back"` → **`"Exit"`** (set + read back; feeds CF_84 SetText on GCO_24, the Back label).
2. **Reposition:** `CallFunction_86` (AddChildToVerticalBox MainVBox←GCO_23) **DELETED**; new nodes **`K2Node_CallFunction_93`** `Widget|AddChildToOverlay` (self←`DynamicCast_1.AsOverlay` — the same live root reference chain B itself uses; Content←`GCO_23.ReturnValue`) → **`_94`** `SetHorizontalAlignment(HAlign_Right)` → **`_95`** `SetVerticalAlignment(VAlign_Bottom)` → **`_96`** `SetPadding(Left=0,Top=0,Right=24,Bottom=24)`, all self←93.ReturnValue; exec chain `CF_85 → 93 → 94 → 95 → 96 → AssignDelegate_7`. This is the boarded "own OverlaySlot HAlign_Right/VAlign_Bottom under Overlay_19, padding ~24" — padding put on the right/bottom edges only (the anchored corner), declared as the minimal equivalent.
3. **Binding UNCHANGED:** `AssignDelegate_7 | AssignOnClicked | src[self<-K2Node_GenericCreateObject_23 Delegate<-K2Node_CustomEvent_19]` — before and after reads identical on both data pins; `CustomEvent_19 (OnBackClicked) → CreateWidget_2 (WBP_MainMenu)` chain untouched.

## 7. GROUP 6 — THE SAVE-UI CUT (DECK-§4, the 669 §4 list verbatim) ✅

All 35 nodes re-read and identity-matched against the record immediately before deletion, then deleted:

| Family | Nodes deleted (EventGraph) |
|---|---|
| SavedNamesText | GCO_12 · CF_63 (SetFontSize 14) · VariableSet_2 · CF_64 (add) · CF_89 (RefreshSavedNames call, Construct tail) |
| NameInput | GCO_13 · VariableSet_3 · CF_65 (add) · VariableGet_10 · VariableGet_11 · CF_101/105 (GetText) · CF_102/106 (ToString) |
| Save | GCO_15 (Btn) · GCO_16 (label) · CF_67 (ToText "Save") · CF_68 (SetText) · CF_69 (AddChild) · CF_70 (AddChildToHorizontalBox) · AssignDelegate_2 · CustomEvent_4 (OnSaveClicked) · MacroInstance_4 (IsValid) · CF_103 (SaveDeckAs) · CF_104 (RefreshSavedNames call) |
| Load | GCO_17 · GCO_18 · CF_71 (ToText "Load") · CF_72 · CF_73 · CF_74 · AssignDelegate_4 · CustomEvent_13 (OnLoadClicked) · MacroInstance_5 · CF_107 (LoadDeck) |

**Exec rewires (each before → after):**
- `CF_62.then`: →GCO_12 ⇒ **→GCO_14** (the SavedNamesText+NameInput block collapses; GCO_14→CF_66 was already direct)
- `CF_66.then`: →GCO_15 ⇒ **→GCO_19** (Save+Load families collapse; Reset family GCO_19/20 + CF_76/77/78 + AssignDelegate_5 KEPT byte-intact)
- `CF_88.then`: →CF_89 ⇒ **→AssignDelegate_10** (Construct tail closes on the details-close bind, unchanged)

**Variables removed:** `NameInput`, `SavedNamesText` (list_variables before/after pasted below). **Function graph removed:** `RefreshSavedNames` — after a live sweep of ALL 7 graphs for surviving callers found only the graph's own FunctionEntry (call sites were exactly 89 + 104, both already deleted — matches the record).

```
vars BEFORE: ['In String','CardTiles','TotalText','AvgText','SavedNamesText','PlayBtn','NameInput','GridScroll','DetailsPanel',...,'GridRow']
vars AFTER : ['In String','CardTiles','TotalText','AvgText','PlayBtn','GridScroll','DetailsPanel',...,'GridRow']
graphs AFTER: BuildSandboxButton · RefreshAll · BuildDetailsPanel · RefreshDetailsPanel · SplitGrid · EventGraph  (RefreshSavedNames GONE)
```

**KEPT untouched (verified in the post dump):** Reset family (GCO_19/20, CF_75/76/77/78, AssignDelegate_5, CustomEvent_15→CF_108 LoadDefaultDeck) · TotalText/AvgText families · details family · tile family · `In String` variable (pre-existing, not mine) · chain A (§8).

## 8. GROUP 7 — THE CHAIN FENCE ✅ (machine-verified, not just eyeballed)

Full pre/post signature diff over the whole EventGraph:

- **Deleted = EXACTLY the enumerated set** (39: the 35 above + 86 + 87 + 109 + 110) — asserted set-equal, `True`.
- **Added = EXACTLY {91, 92, 93, 94, 95, 96}** — asserted set-equal, `True`.
- **Changed = 24 nodes, every one accounted for:** the 18 expected rewire/relabel endpoints + 6 connection-list consequences (`CF_43` gained the 92 feed; `GCO_7`/`Self_1` lost deleted consumers; `Self_6`/`Self_7`/`Self_9` dropped to ZERO connections — see deviation D1).
- **Chain A: byte-identical.** Anchors individually verified identical pre/post: `Self_0`, `DynamicCast_0`, `GCO_0`, `GCO_6`, `CF_11`, `CF_35`, `CF_59`, `CustomEvent_1/_3/_9`, `AssignDelegate_0/_1/_3` — all `True`; and no chain-A id appears in the deleted/added/changed sets. 155 of 179 surviving common nodes fully untouched; the 24 changed are all chain-B op endpoints.

**Node arithmetic:** EventGraph 218 → 185 (−39 +6) → **182** (−3 orphaned Selfs, D1). RefreshAll 23 → **20**.

## 9. POST-CONDITION SWEEPS (the 673 criteria, pre-answered)

Over the complete post-op node dump (every pin of every node):

- Node **87 ABSENT** · nodes **109/110 ABSENT** · `OnPlayClicked → StartMatch` DIRECT ✅
- Literal `"Active"`: **0 occurrences** anywhere in the graph ✅
- Composed `"deck"+N` / any `deckN` literal: **0 occurrences** ✅
- References to `SetActiveDeckBySlot`/`GetEditingDeckIndex`/`SelectDeckForEdit`/`GetActiveDeckIndex`: **0 nodes** (the compile-order wall honored) ✅
- PlayBtn label literal = `"Play"`; Back label literal = `"Exit"`; binding `AssignDelegate_7 → OnBackClicked (CustomEvent_19)` data pins byte-identical ✅
- DeckBar = design-time `UHorizontalBox` named exactly `DeckBar`, child of `Overlay_19`, OverlaySlot Top/Fill, empty; MainVBox top padding 64 via ONE graph op on CF_43's slot ✅
- No `write_graph_dsl`, no duplicate+reparent, no new `.uasset`, `/Game/UI/WBP_DeckSlotEntry` reserved-unused ✅

## 10. COMPILE ×2 + THE SAVE (spec step 7) ✅

- **Compile 1** (`compile_blueprint`, 06:48:11): the predicted GUID self-heal fired **exactly once** — `Ensure condition failed: WidgetBP->WidgetVariableNameToGuidMap.Contains(Widget->GetFName())` (WidgetBlueprintCompiler; the new `DeckBar` receiving its GUID map entry). Handled ensure, by design.
- **Compile 2** (06:48:13): **silent** — no ensure, zero `LogBlueprint` warnings, zero errors. Two-compile rule satisfied.
- **The save:** dirty set read immediately before save = **exactly `['/Game/UI/WBP_DeckBuilder']`** (669's in-memory dirt on `WBP_MainMenu`/`WBP_DeckCardTile` did not survive 667's editor bounce — nothing to decline; no save prompt ever appeared). `EditorAssetLibrary.save_asset('/Game/UI/WBP_DeckBuilder')` → `True`. Dirty set after = **`[]`**.

**Disk ledger (sha256, post-save):**

| File | Hash | Verdict |
|---|---|---|
| `Content/UI/WBP_DeckBuilder.uasset` | **`2e88002ec3f702da695cc7a08f69b8f4c66f9809fe7ea7bb50401e1f6457e13f`** (757,995 bytes; was `118abfa4941404f8a9d2bddf9bf4bcbafa858779a20ee7cd7d2ca6ae5aa0bb51` / 857,141) | the legitimate cargo — **674's §25b input** |
| `Content/Maps/L_Arena.umap` | `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` | **== the ROT-§2 ledger, entry AND exit — never saved** |
| `Content/UI/WBP_MainMenu.uasset` | `03cc0a…4ae3` | byte-identical to pre-op |
| `Content/UI/WBP_DeckCardTile.uasset` | `062528…a7bd` | byte-identical to pre-op |

`git status --porcelain Content/` = exactly one line: ` M Content/UI/WBP_DeckBuilder.uasset`.

## 11. DEVIATIONS / INTERPRETATIONS (SC-§15 — declared, not silent)

1. **D1 — three orphaned pure `Self` nodes deleted beyond the record's verbatim list:** `Self_6` (fed only CF_103), `Self_7` (only CF_107), `Self_9` (only CF_109/110) dropped to zero connections after their families were cut; each was read, confirmed 0 connections, then deleted. Reasoning: a family's self-reference node is part of the family body; leaving NEW orphans would add residue to a graph already carrying legacy stubs. `Self_1` (many live consumers) untouched.
2. **D2 — Exit padding = `(0,0,24,24)`** (right+bottom only): the spec's "padding ~24" applied to the anchored corner's edges; uniform 24 would offset nothing else on a Right/Bottom-aligned slot anyway. One-line retune if Jonathan's eye disagrees at 675.
3. **D3 — the Exit reposition is 4 nodes** (AddChildToOverlay + 3 slot setters), the boarded op's literal shape ("own OverlaySlot … or the minimal equivalent, declared"). The overlay reference reuses chain B's own `DynamicCast_1.AsOverlay` — no new root lookup authored.
4. **D4 — `InSize`/struct literal serialization order** varies between set-time and post-save readback (`(SizeRule=Fill,Value=1.000000)` ⇄ `(Value=1.000000,SizeRule=Fill)`); same value, cosmetic.
5. **D5 — the G4 padding op sits AFTER CF_45** (44→45→92), reading the spec's "the 44/45 chain" as the insertion locus; any position after 43 is semantically identical (same slot object, Construct-time).
6. **D6 — `CustomEvent_23` live-absence** (§1): recorded, not repaired — it was exec-dead residue; nothing in the amended spec touches it.
7. **D7 — remote-exec lane used for the design-time half and the save** (the dispatch explicitly opened it): MCP has no widget-tree authoring or asset-save surface. `root_widget`/`Slot.Parent` are reflection-walled to Python exactly as 669 found via MCP; identity was proven through `find_object` + child enumeration instead — no wall was worked around by writing.

## 12. WHAT ONLY PIXELS CAN VERIFY (TL-§4 — no claim made here; 674's lane)

1. `GridScroll` actually scrolling / the full bottom tile row reachable at 1280×720 · 1600×900 · 1920×1080 (the AFTER set; before-record = Jonathan's screenshot + 669 §3).
2. The DeckBar row visibly on top, 10 entries, no overlap with MainVBox content (the 64px padding vs 671's 48+8 arithmetic — if entries render taller, the padding is a one-pin retune).
3. Exit rendering bottom-right, clickable, above/clear of the thumbstick residue (all three template widgets are Collapsed at Construct — nodes 38/39/40 untouched — so no collision is expected, but that is a render claim).
4. Play entering a match with the ACTIVE deck (the log line names it — 674), builder-open ×2 byte-stability of the active slot (the seed-cut proof), the orange outline following right-click (675, hands).
5. `BindWidgetOptional` resolving `DeckBar` at widget construction (674's live open; the CDO carries no bind by design).

## 13. EDITOR STATE AT FINISH

PID 9072 UP · MCP green · `IsPIERunning=false` · dirty packages **0** · `L_Arena` never loaded/saved/hashed-different (== ledger) · no console, no `M`, no pawn input, no viewport move — the 571+552 latch UNSPENT by this task. Instruments left reusable in the session scratchpad: `t672_ops.py` (granular node-op helpers) · `t672_deckbar.py`/`t672_deckbar_verify.py`/`t672_slot_post.py`/`t672_save.py` (remote-exec) · all before/after JSONs + `t672_eg_post_compact.txt`.
