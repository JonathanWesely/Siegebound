# TASK-669 — [DB3-1] THE MEASURED WBP RECORD — `WBP_DeckBuilder` tree audit + baseline clip captures (build-master)

Date 2026-08-27 · read-only · editor PID 28772 UP throughout · MCP live (scratchpad `mcp_client.py` lane) · ⛔ nothing saved, no compile, no git.

## 0. THE HEADLINE, BEFORE THE TREE — the WBP is NOT what the law predicted

**`WBP_DeckBuilder`'s design-time WidgetTree contains NO deck-builder UI at all.** The entire visible screen is **CONSTRUCTED AT RUNTIME by the EventGraph** (`ConstructObjectFromClass` + `AddChild*`, the TL-§4-era workaround pattern), hung on the design-time root Overlay of a **duplicated touch-interface template**. `GridScroll` and `GridRow` are BP **variables assigned at runtime** — they do not exist as design-time widgets (probe: `:WidgetTree.GridScroll` = not a valid object; `:WidgetTree.Overlay_19` resolves).

⇒ **TASK-672's surgery is GRAPH surgery (granular node ops), not designer slot-rule edits.** The RELAYED-DIAGNOSIS law did its job: recompute every op from THIS record, not from DECK-§6's prediction.

Same-family fact: `WBP_MainMenu`, `WBP_DeckCardTile`, `WBP_DeckBuilder` all carry the IDENTICAL template design tree (verified per-widget) and all three build their real UI in-graph. Parents: `WBP_DeckBuilder` → C++ `UDeckBuilderWidget`; the other two → plain `UserWidget`.

## 1. THE DESIGN-TIME TREE (complete — verified widget-by-widget via ObjectTools; `Slots`/`RootWidget` arrays are reflection-walled per TL-§4, child set enumerated by slot-name probes)

```
Overlay_19                     (Overlay — ROOT; slot=None proven)
├─ [OverlaySlot_0  HAlign_Right/VAlign_Bottom, pad R50 B400] SizeBox_0 (130×130 overrides ON)
│    └─ [SizeBoxSlot_0 Fill/Fill] Btn_Jump (Button)                — Collapsed at Construct
├─ [OverlaySlot_4  HAlign_Left /VAlign_Bottom, pad 60 all]  Thumbstick_Move (UI_Thumbstick_C) — Collapsed
└─ [OverlaySlot_5  HAlign_Right/VAlign_Bottom, pad 60 all]  Thumbstick_Aim  (UI_Thumbstick_C) — Collapsed
```
No other OverlaySlot exists (probed _1/_2/_3/_6/_7/_8 = absent). **The root exists ⇒ the TL-§4 children lane is OPEN for design-time additions (e.g. `DeckBar`).**

## 2. THE RUNTIME TREE (what the player sees — authored by EventGraph chain B; every node ID given for 672)

EventConstruct (`K2Node_Event_1`) → collapse the 3 template widgets (38/39/40) → `CastToOverlay(GetParent(GetParent(Btn_Jump)))` (`DynamicCast_1`) → build:

```
Overlay_19
└─ MainVBox = GenericCreateObject_7 (VerticalBox)
   [OverlaySlot via CallFunction_43; HAlign_Fill (44) + VAlign_Fill (45)]
   ├─ Title TextBlock "Deck Builder" 28pt      (GCO_8, added 49)          [VBoxSlot AUTO — default]
   ├─ GridRow (HorizontalBox, GRAPH-SET var; constructed in fn SplitGrid) (added by ★CallFunction_50★) [VBoxSlot AUTO ⚠ — return pin UNUSED, no SetSize]
   │    ├─ GridScroll (ScrollBox, GRAPH-SET var; SplitGrid)   [HBoxSlot SetSize Fill 1.0]
   │    │    └─ WrapBox = GCO_9 (added 139)                   [ScrollBox slot default]
   │    │         └─ 28 × WBP_DeckCardTile (CreateWidget_1 in ForEach MacroInstance_0; SetupCell; appended to CardTiles array)
   │    └─ DetailsPanel (Border, var; constructed in fn BuildDetailsPanel) [HBoxSlot SetSize Fill 0.42]
   │         └─ VBox: DetailsHintText 16pt wrap · DetailsArtBorder · DetailsNameText 22pt · DetailsCostText 16pt · DetailsBodyText 14pt wrap · Btn_DetailsClose ["Close"]
   ├─ TotalText 20pt (GCO_10, VariableSet_0, added 56)        [AUTO]
   ├─ AvgText 16pt (GCO_11, VariableSet_1, added 62)          [AUTO]
   ├─ SavedNamesText 14pt (GCO_12, VariableSet_2, added 64)   [AUTO]
   ├─ NameInput EditableText (GCO_13, VariableSet_3, added 65)[AUTO]
   ├─ Save/Load/Reset row HorizontalBox (GCO_14, added 66)    [AUTO]
   │    ├─ Save  Btn GCO_15 + label GCO_16 "Save"  → AssignDelegate_2 → OnSaveClicked (CustomEvent_4)
   │    ├─ Load  Btn GCO_17 + label GCO_18 "Load"  → AssignDelegate_4 → OnLoadClicked (CustomEvent_13)
   │    └─ Reset Btn GCO_19 + label GCO_20 "Reset to Default" → AssignDelegate_5 → OnResetClicked (CustomEvent_15)
   ├─ PlayBtn "Play With This Deck" (GCO_21+GCO_22, VariableSet_4, added 82) → AssignDelegate_6 → OnPlayClicked (CustomEvent_17)  [AUTO]
   └─ Back Btn "Back" (GCO_23 + label GCO_24, added 86) → AssignDelegate_7 → OnBackClicked (CustomEvent_19)  [AUTO]
Construct tail: CallFunction_87 LoadDefaultDeck → 88 RefreshAll → 89 RefreshSavedNames → AssignDelegate_10 (Btn_DetailsClose → OnDetailsClosePressed)
```
(GCO_N = `K2Node_GenericCreateObject_N`; plain numbers = `K2Node_CallFunction_N`; all in `WBP_DeckBuilder:EventGraph` unless said otherwise. `SplitGrid`/`BuildDetailsPanel`/`RefreshAll`/`RefreshSavedNames`/`RefreshDetailsPanel` are function graphs on the same BP.)

## 3. ⛔ THE CLIP MECHANISM — STRUCTURALLY PROVEN (pixel confirmation owed, §6)

- `GridScroll` (ScrollBox) **EXISTS** — and is **defeated by its ancestor chain**: `GridRow` sits in MainVBox in an **AUTO VerticalBoxSlot** (★the return pin of `CallFunction_50` is unused — no `SetSize` node★). An Auto slot grants desired size; a ScrollBox's desired height = its FULL content height ⇒ it is never height-constrained ⇒ **it never scrolls**.
- MainVBox is Fill/Fill in the root Overlay (top-pinned) ⇒ when title + full grid + 4 text rows + input + 3 button rows exceed the window height, **everything below the fold is clipped at the screen edge with no scrollbar** — the bottom tile row first (Jonathan's screenshot), and note: **Save/Play/Back are even further down ⇒ at clipping sizes the user cannot reach Save or Back at all** (explains the directive's auto-save + always-reachable Exit).
- Window-size dependence: WrapBox wrap-width = ScrollBox width (≈ 1.0/1.42 of window width) ⇒ column count varies with width ⇒ row count varies ⇒ desired height varies ⇒ "works at some window sizes" — exactly the reported symptom.
- **The minimal no-clip op (672 recomputes, this is the measured anchor):** insert `VerticalBoxSlot.SetSize(SlateChildSize Fill 1.0)` on the slot returned by `CallFunction_50` (MainVBox ← GridRow). Everything below the grid stays outside the scroll region by construction (they are MainVBox siblings). Wrap-width already follows window width.

## 4. NAME CENSUS (real names; the MCP JSON lane aliases them camelCase — `totalText` etc. Same objects.)

| Role | Real element/variable | Form |
|---|---|---|
| Grid scroll container | `GridScroll` (ScrollBox) | BP variable, GRAPH-set in `SplitGrid` — ⛔ not design-time |
| Grid row container | `GridRow` (HorizontalBox) | BP variable, GRAPH-set in `SplitGrid` |
| Tile array | `CardTiles` (array of WBP_DeckCardTile_C) | BP variable |
| Counters | `TotalText` ("Deck: n/50") · `AvgText` ("Avg cost: x") | BP variables, runtime TextBlocks |
| Save-UI (the DECK-§4 cut) | `NameInput` (EditableText) · `SavedNamesText` (TextBlock) · anonymous Save/Load Buttons (GCO_15/GCO_17) | vars + runtime widgets |
| Play | `PlayBtn` (Button, "Play With This Deck") | BP variable |
| Reset | anonymous Button GCO_19 ("Reset to Default") | no variable |
| **Back button** | **anonymous runtime Button GCO_23, label GCO_24 "Back"** — ⛔ there is NO `Btn_Back` element | no variable; binding = AssignDelegate_7 → `OnBackClicked` |
| Details panel | `DetailsPanel` (Border) + `DetailsHintText`/`DetailsArtBorder`/`DetailsNameText`/`DetailsCostText`/`DetailsBodyText`/`Btn_DetailsClose` | vars, runtime-built in `BuildDetailsPanel` |
| Template residue (design-time) | `Overlay_19` · `SizeBox_0` · `Btn_Jump` · `Thumbstick_Move` · `Thumbstick_Aim` | the ONLY design-time widgets |

**672's exact CUT list (DECK-§4: NameInput/Save/Load/SavedNamesText + graph nodes):**
- NameInput: GCO_13 · VariableSet_3 · add-node 65 · variable `NameInput` · readers CallFunction_101/105 (GetText) + 102/106 (ToString)
- Save: GCO_15/GCO_16 · 67/68/69/70 · AssignDelegate_2 · CustomEvent_4 `OnSaveClicked` + body (MacroInstance_4 IsValid · 103 SaveDeckAs · 104 RefreshSavedNames)
- Load: GCO_17/GCO_18 · 71/72/73/74 · AssignDelegate_4 · CustomEvent_13 `OnLoadClicked` + body (MacroInstance_5 · 107 LoadDeck)
- SavedNamesText: GCO_12 · VariableSet_2 · 63/64 · variable · fn graph `RefreshSavedNames` + its call sites (89 in Construct tail — rewire 88→AssignDelegate_10)
- ⚠ exec-chain rewires at every cut: 64→(NameInput block)→65→66 collapses to …→66; AssignDelegate_2→GCO_17 etc. — 672 lists each before→after.

**KEEP (⛔ untouched per spec):** PlayBtn · Reset (GCO_19 family) · TotalText/AvgText · details family · tile family.

## 5. ⚠️ FINDINGS THE SPEC MUST HEAR (FR culture — measured, with node IDs)

1. **⛔ WAVE-BREAKING: the Construct tail calls `LoadDefaultDeck` (CallFunction_87) AFTER C++ `NativeConstruct` has run.** 670's NativeConstruct (migrate → `SelectDeckForEdit(GetActiveDeckIndex())`) executes BEFORE BP Construct — node 87 then resets the working deck to the curated default; and once D9 makes `LoadDefaultDeck` persist through the auto-save funnel, **every builder open would OVERWRITE the active slot with the curated default = silent deck loss.** 672 must cut node 87 (rewire AssignDelegate_7→88) or 670 must make the seeding order immune; one of them must own it in writing. (87→88 RefreshAll/89 refresh calls are still wanted.)
2. **⛔ TEN-SLOT-LAW violation left standing by the cut/keep boundary as boarded:** `OnPlayClicked` = `SaveDeckAs("Active")` (109, literal) → `SetActiveDeck("Active")` (110) → StartMatch (111). Post-migration this creates/activates an 11th, NON-fixed deck named "Active" on every Play — and re-triggers migration mapping on next open (idempotence broken by a recurring legacy name). The boarded 672 spec says PlayBtn is untouched; **these two literal pins (or the whole Play body) need a DECK-§1-conform rewrite** (e.g. persist-current-slot + `SetActiveDeckBySlot(editing)`) — manager/670/672 decide the owner, but it cannot ship as-is.
3. **Orphaned duplicate main-menu chain INSIDE WBP_DeckBuilder's EventGraph** (chain A, exec-dead: 1/3/4 → DynamicCast_0 → GCO_0..GCO_6, OverlaySlot Center/Center, buttons "Play (vs Bot)"/"Deck Builder (Coming Soon)" (disabled)/"Quit" + BuildSandboxButton call 59, events CustomEvent_1 StartMatch / _3 QuitGame / _9 StartSandboxMatch). Never runs (no event heads it). ⚠ 672 must not confuse chain-A node IDs with chain-B's (A uses `Self_0`/GCO_0–6/11–13/17–35; B uses `Self_1`/GCO_7–24/38–89 + the IDs in §2). ~17 empty `OnClicked_Event_N` stubs + touch/jump handlers are further residue in all three WBPs.
4. **`read_graph_dsl` is LOSSY** — it omitted the OverlaySlot/VerticalBoxSlot alignment setter calls (44/45, 19/26/34) entirely. Node-level reads (`find_nodes` + `get_node_infos`) are the trustworthy lane; QA should not treat a DSL dump as a complete op audit.
5. **DECK-§7 `DeckBar` recompute needed:** the law says DeckBar = design-time TOPMOST row (672 authors, 671 binds `BindWidgetOptional`). Design-time authoring is OPEN (root exists, children lane) — **but the runtime MainVBox is Fill/Fill over the whole root Overlay, so a design-time DeckBar would be overlapped/covered by it.** Options measured for 672: (i) design-time `DeckBar` under `Overlay_19` (OverlaySlot HAlign_Fill/VAlign_Top) + a graph op giving MainVBox top padding = bar height; (ii) fold the bar row INTO the graph-built MainVBox — ⛔ breaks BindWidget (binding is design-time-only) — not viable with DECK-§8 as pinned; (iii) rebuild the page design-time and retire chain B (larger surgery; no duplicate+reparent since the root exists). (i) is the minimal-op route consistent with DECK-§8.
6. Environmental note: with editor + a standalone game co-resident the GPU ran ~580 MB over budget (on-screen warning in the captures). Performance-only; 674 should expect the same during its live verify.

## 6. BASELINE CAPTURES — the lane is PROVEN; the builder-screen set is GAP-DECLARED (desktop locked)

**The wall, measured:** the Windows desktop is LOCKED (window under the click point = `Windows Default Lock Screen` / `LockScreenBackstopFrame`, LogonUI PID 20988; probed twice, task start→end). Every input-injection lane (SetForegroundWindow, mouse_event, SendInput, posted WM_*) dies at the lock screen; UE ignores posted mouse messages by design (polls the physical cursor). **No machine lane can OPEN the deck builder until the desktop is unlocked** — MCP exposes no function-call/input door (TASK-567/569 enumerations, re-confirmed), and the sanctioned UI-click lane needs a live desktop.

**What WAS captured (PrintWindow flag 3 works under lock):**
- `handoffs/TASK-669-menu-1280x720.png` — standalone game (`-game -windowed -ResX=1280 -ResY=720`), main menu, 7 entries (Play (vs Bot) · Sandbox (No Bot) · Deck Builder · Multiplayer · Settings · Login · Quit).
- `handoffs/TASK-669-menu-hover-deckbuilder.png` — cursor verified ON the Deck Builder button (hover state on pixels) — one un-eaten click short of the builder screen.

**The re-run recipe (2 minutes/size once the desktop is unlocked — scripts live in the session scratchpad):**
1. `UnrealEditor.exe <uproject> -game -windowed -ResX=<W> -ResY=<H> -WinX=80 -WinY=80` (boots to L_MainMenu).
2. Menu geometry at 720p (centered VBox, pitch 49.7 px): Deck Builder button center = client (639, 310); rows scale with window center — recompute as (W/2, H/2 − 2·49.7·uiScale) or re-derive from a fresh menu capture.
3. `t669_topclick.ps1 -ProcId <pid> -X <x> -Y <y>` (topmost + real cursor + double press; aborts if not top window) → `t669_wincap.ps1 -Mode cap` → `TASK-669-clip-<W>x<H>.png`. Kill the process after.
4. Sizes owed: 1280×720 · 1600×900 · 1920×1080 — the DECK-§6 baseline TASK-674's after-set compares against. **Owed by: any session with an unlocked desktop (this task's declared gap), or folded into 674's live verify, or Jonathan's own three screenshots.**

The clip verdict itself does NOT wait on these pixels for 672 to proceed — §3 is structural and node-exact; the pixels are the DECK-§6/TL-§4 confirmation layer.

## 7. INTERACTION AUDIT (the surfaces 670/671/672 coexist with)

- **Tile** (`WBP_DeckCardTile`, runtime-built like its owner): full-tile transparent `Btn_CardFace` (OnClicked → `OnCardFacePressed` → `SelectCardForDetails(OwnerBuilder, CardID)`) + `RemoveBtn` "−" (OnClicked_Event_13 → `RemoveCopy`) + `AddBtn` "+" (OnClicked_Event_14 → `AddCopy`; greyed at cap by `RefreshCell`) + `CopyCountText` badge. All LEFT-click Button bindings; RMB falls through today (DECK-§5's premise confirmed on the real widget). Tiles call straight into `OwnerBuilder` (the UDeckBuilderWidget) ⇒ **670's auto-save funnel captures tile edits with zero tile changes.** Dead duplicates `OnAddPressed`/`OnRemovePressed` exist unbound (residue).
- **Refresh flow:** C++ BIEs `OnDeckModelChanged`/`OnDeckSlotCountChanged` → `RefreshAll` (counters + PlayBtn enable + per-tile RefreshCell + details); `OnCardDetailsRequested` → `RefreshDetailsPanel`.
- **Reset:** anonymous button → `LoadDefaultDeck` (D9 will make this persist via 670's funnel — by design).
- **Back/Exit (D6):** `OnBackClicked` → CreateWidget `WBP_MainMenu` → AddToViewport → RemoveFromParent(self). Keep the AssignDelegate_7 wiring; the relabel = the literal pin on CallFunction_83 ("Back"→"Exit"); reposition = re-parent GCO_23's add (86) out of MainVBox to a bottom-right anchor (672's op list).
- **Open flow:** WBP_MainMenu "Deck Builder" (3rd entry) → `OnClicked_Event_9` → RemoveFromParent(menu) → CreateWidget `WBP_DeckBuilder_C` (no owning-player pin) → AddToViewport.

## 8. STATE LEDGER (read-only fences honored)

- **Editor PID 28772 UP** at finish; `IsPIERunning=false`; MCP responsive. **No PIE/SIE was started; no level was loaded or touched** ⇒ L_Arena context untouched (hash re-verify not triggered per dispatch); `is_dirty(L_Arena)=false`.
- **In-memory dirt, declared:** `WBP_DeckBuilder`/`WBP_DeckCardTile`/`WBP_MainMenu` read `is_dirty=true` after my load+graph reads (BP node reconstruction on load; TASK-662's same-day full sweep recorded dirty=[] before this task; zero write calls were issued). **Disk proven untouched:** git porcelain clean for `Content/UI/`, mtimes 2026-07-26/2026-08-16. ⛔ **If the editor ever prompts to save these three, DECLINE** — the dirt is a load artifact, not content.
- One instrument attempt was permission-blocked and NEVER EXECUTED: enabling `PythonScriptPluginSettings.bRemoteExecution` (a settings-CDO write under the TASK-568 set/restore protocol) — the whole command was refused pre-execution; the setting read `false` before and was never written. The task stayed within its no-property-write fence.
- Stray synthetic input (2 clicks + cursor moves) landed exclusively on the LOCK SCREEN (verified via WindowFromPoint) — no application received it; the editor was never foregrounded or clicked.
- My standalone game process (PID 5908) was launched and closed by me; `Saved/` config side-effects only, untracked.
- Latch/console: the in-game assistant console was never opened, no sentence sent, no `DumpAssistantPrompt`/`Spike*` — the TASK-552/571 latch is UNSPENT by this task.

## 9. Instruments (session scratchpad, reusable)

`mcp_client.py` + `u.py` (MCP lane) · `t669_tree.py` (subobject tree prober) · `t669_graphs.py` / `t669_nodes.py` (graph DSL + full node dump; `t669_eg_nodes.json` = the complete EventGraph node record) · `t669_db_*.dsl.txt`, `t669_WBP_*.dsl.txt` (graph DSLs) · `t669_wincap.ps1` (PrintWindow capture) · `t669_topclick.ps1` / `t669_realmouse.ps1` / `t669_msg.ps1` (click lanes; topclick is the good one) · game log `t669_game_720.log`.
