# TASK-808 handoff — `WBP_CardHand` LIVE TREE MEASUREMENT (art-director) — ⛔ READ-ONLY, ZERO EDITS

- **Author:** art-director · **Date:** 2026-09-03
- **Editor session:** PID 30748, started `02:38:52` (log `Saved/Logs/GitClaudeUnrealTest.log`), `IsPIERunning = false` throughout, no editor bounce, no modal, `WBP_CardHand` **never opened in an asset editor** (measured only through MCP object/graph queries).
- **Writes performed: ZERO.** No `save_assets` on any path. No widget edit, no duplicate, no reparent, no compile, no C++, no Git, `L_Arena` untouched (`is_dirty = false`, verified). No TASKBOARD edit — orchestrator flips the board.
- **Method:** `BlueprintTools.list_graphs / list_variables / read_graph_dsl / find_node_types`, `ObjectTools.get_properties / list_properties / get_class`, `AssetTools.is_dirty`, `EditorAppToolset.IsPIERunning / CaptureEditorImage`, plus a direct read-only parse of `Content/UI/WBP_CardHand.uasset` on disk.

---

## ⛔⛔ TWO FINDINGS THAT CHANGE `TASK-809`. READ THESE BEFORE THE TREE.

### 🚨 FINDING A — `GetSlotKeyLabel` IS **NOT IN THE RUNNING EDITOR**. `TASK-809` CANNOT BIND IT TODAY.

`TASK-807` wrote the symbol to **source only**. The editor is running a module built **before** it. Measured three independent ways:

| Evidence | Value |
|---|---|
| `find_node_types(filter "KeyLabel")` on a live `WBP_CardHand` graph | **`[]` — empty** |
| `find_node_types(filter "Siegebound\|UI\|")`, same graph | `RequestPlaySlot`, `RequestDiscardSlot`, `InitforController`, `GetNextCardArtTexture`, `GetCardArtTexture`, `InitforCastle`, `InitforCombatant`, `Variables\|Siegebound\|UI\|GetObservedController` — ⛔ **`GetSlotKeyLabel` absent from its own category** |
| `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` mtime | **2026-09-02 18:45:10** |
| `Source/.../CardHandWidget.h` mtime (TASK-807's edit) | **2026-09-02 19:35:30** — ⛔ **50 min AFTER the DLL** |
| `CardHandWidget.cpp` mtime | 2026-09-02 19:34:09 |

⇒ ⛔ **The node cannot be created; the palette does not contain it.** The Lane A graph `807 → 809 → 811` is **unsatisfiable as ordered**: 809 must bind a symbol that only exists after a compile, and `QUIET-MODULE` puts Lane A's **only** compile in `TASK-811`, *after* 809. **A compile must be inserted before 809, or 809 must be re-sequenced after 811.** This is the orchestrator's to rule — flagged, not worked around.

### 🚨 FINDING B — ⛔ `CARDBAR-§4` IS **WRONG ABOUT THIS WIDGET**. THE CARD BAR **IS** GRAPH-CONSTRUCTED.

`CARDBAR-§4` refutes the "graph-constructed" claim by citing `handoffs/TASK-355-artist.md:71-79`'s `WBP_CardHand -> ['HorizontalBox','Overlay','SizeBox','VerticalBox']`. **That table was a package NAME-TABLE read, and a name-table entry does not distinguish a design-time instance from a class reference.** Those four names are in the package because `BuildHandTree`'s `ConstructObjectFromClass` nodes reference those **classes**.

Measured on the asset:

- ⛔ **`HorizontalBox_0` and `VerticalBox_0` do not resolve** in the WidgetTree (`is not valid Object`). **No design-time HorizontalBox, VerticalBox, TextBlock or Image instance exists.**
- ✅ **The entire six-slot bar — every VerticalBox, TextBlock, Play button, discard button, art Image and the wrapper Overlay — is built at RUNTIME** by the function graph `BuildHandTree`, read verbatim below.
- ✅ The design-time tree is **five nodes**, all donor template leftovers from the `WBP_HUD` duplication. **Not one of them is a card.**

⇒ ⚖️ **The `CARDBAR-§4` *conclusion* that survives is the right one — ⛔ never duplicate+reparent this widget — but its stated evidence is false and its "designer-authored" claim must be struck.** `TASK-033`'s own record already said so (`TASK-041` authored the tree via MCP runtime construction, explicitly: *"the full visual hand tree WAS authored via MCP — not by a designer — using runtime widget construction"*). ⭐ **Every 809 edit is a GRAPH edit, ⛔ not a designer-tree edit.**

---

## 1. THE FULL WIDGET TREE

### 1a. DESIGN-TIME tree — complete, 5 nodes (all read back individually)

```
Overlay_19            Overlay          ROOT (Slot: None)                       Visibility: SelfHitTestInvisible
├─[OverlaySlot_0]  SizeBox_0     SizeBox    HAlign_Center / VAlign_Bottom / pad B40
│                                          WidthOverride 240, HeightOverride 80 (both bOverride_ = true)
│                                          Visibility: SelfHitTestInvisible
│   └─[SizeBoxSlot_0] Btn_Jump   Button    Visibility: VISIBLE      ⚠️ donor — LOAD-BEARING (see §2b)
├─[OverlaySlot_4]  Thumbstick_Move  /Game/Input/Touch/UI_Thumbstick.UI_Thumbstick_C   Collapsed
└─[OverlaySlot_5]  Thumbstick_Aim   /Game/Input/Touch/UI_Thumbstick.UI_Thumbstick_C   Collapsed
```

Marked-as-variable design-time widgets = exactly three: `Btn_Jump`, `Thumbstick_Move`, `Thumbstick_Aim` (confirmed by `find_node_types "Variables|WBP_CardHand|"` → `GetThumbstick_Move`, `GetThumbstick_Aim`, `GetBtn_Jump`, **and nothing else**). `Overlay_19` / `SizeBox_0` exist but are not variables. `OverlaySlot_1/2/3` do not exist.

**Root visibility — the board's explicit question.** `TASK-033` shipped `Collapsed`; the flip to `SelfHitTestInvisible` **HAS HAPPENED**. Live now, both layers: the generated-class CDO reads `visibility: SelfHitTestInvisible` (`bIsFocusable: false`, `clipping: Inherit`, `cursor: Default`), **and** `EventConstruct` re-sets it at runtime — `(Widget|SetVisibility _self "SelfHitTestInvisible")`.

### 1b. RUNTIME tree — one card slot, and the bar container (from `BuildHandTree`, verbatim)

```
_handbox : HorizontalBox                       ← ONE, built before the loop
│   attached: AddChildToOverlay(Overlay_19) → HAlign_Center / VAlign_Bottom / MakeMargin(0,0,0,24)
│
└─ ×6  FaceOverlay : Overlay                   ← WrapCardFace() return; THIS is what enters the HorizontalBox
       Visibility: SelfHitTestInvisible
       ├─ z0  ArtImage : Image                 → Img_CardArt[i]   Visibility: Collapsed at build;
       │                                          UpdateSlotArt sets HitTestInvisible on a non-null resolve
       │                                          overlay slot HAlign_Fill / VAlign_Fill  (fills the WHOLE face)
       └─ z1  slotVBox : VerticalBox            → SlotBoxes[i]     overlay slot Fill / Fill
              Visibility: SelfHitTestInvisible (CDO default AND set explicitly on every non-empty push)
              ├─ [0] NameText : TextBlock  font 16   → SlotNameTexts[i]      Visibility: Visible (CDO, never overridden)
              ├─ [1] CostText : TextBlock  font 14   → SlotCostTexts[i]      Visibility: Visible (CDO, never overridden)
              ├─ [2] PlayButton : Button             → SlotPlayButtons[i]    Visible / ClickMethod DownAndUp
              │        └─ TextBlock, text literal "Play"
              └─ [3] DiscardButton : Button          → SlotDiscardButtons[i] Visible / ClickMethod DownAndUp
                       └─ TextBlock, text literal "1"
```

Preview + refusal (same `BuildHandTree`, after the loop): a VerticalBox of `TextBlock "Next:"` (12) + `PreviewNameText` (14) + `PreviewCostText` (12), also `WrapCardFace`-wrapped (`Img_NextCardArt`), attached HAlign_Right / VAlign_Bottom / margin R24 B24; `RefusalText` (20, Collapsed) attached centre/centre. Tail: `ApplyCardTextShadows()`.

**Graphs (7):** `EventGraph`, `BuildHandTree`, `WrapCardFace`, `UpdateSlotArt`, `UpdateNextCardArt`, `ApplyCardTextShadows`, `UpdateGoldDisplay` (donor dead code). ⛔ **No `Tick`, no `PreConstruct`.**

**Member variables (13):** `SlotNameTexts`, `SlotCostTexts`, `SlotBoxes`, `SlotPlayButtons`, `SlotDiscardButtons` (arrays) · `PreviewNameText`, `PreviewCostText`, `RefusalText`, `Img_NextCardArt` (singles) · `Img_CardArt` (array) · `CardCost`, `GoldText`, `SiegePS` (donor dead code).

---

## 2. THE PLAY BUTTON — where it lives, and what its removal leaves

**Location:** `slotVBox` child **index 2**, constructed in `BuildHandTree`'s loop, stored in `SlotPlayButtons[i]`, label = a child `TextBlock` with the literal `"Play"`.

**⭐ LAYOUT-COLLAPSE RISK: LOW — and this is MEASURED, not assumed.** ⛔ **There is NO `SizeBox` and NO fixed-size widget anywhere in the runtime bar.** The only `SizeBox` in the asset is design-time `SizeBox_0` (240×80), and it wraps **`Btn_Jump`, not a card**. Every card container auto-sizes: `VerticalBox` → content, `Overlay` → largest child, `HorizontalBoxSlot` → Auto. Removing child [2] simply shortens the column. ✅ **No parent is sized around the absent child.**

**⚠️ THE REAL RISK IS DANGLING NODES, NOT LAYOUT.** Three live consumers read `SlotPlayButtons` and would index an emptied array:
1. `EventConstruct` — 6× `AssignOnClicked(Array|Get(SlotPlayButtons, i), OnPlaySlotI)`.
2. Custom events `OnPlaySlot0..5` → `RequestPlaySlot(self, i)` (12 nodes).
3. ⭐ `EventOnHandSlotUpdated` — `(Widget|SetIsEnabled (Array|Get SlotPlayButtons SlotIndex) bAffordable)`. **This is the §3.5 grey-out's button half.** Leaving it against an empty array = `Accessed None` **on every slot push, i.e. every gold tick.**

✅ **The affordability grey-out SURVIVES the removal**: `UpdateSlotArt` independently tints the **art** via `SelectColor(white, (0.35,0.35,0.35,1), bAffordable)`. Only the button-disable is lost.

---

## 3. ⛔⛔ THE `"1"` DISCARD BUTTON — CONFIRMED ON THE ASSET, AND EVERY REFERENCE ITS REMOVAL STRANDS

### 3a. `CARDBAR-§0` is ✅ **CONFIRMED, not refuted.** First eye on the widget agrees with the record.

Measured in `BuildHandTree`: a `Button` whose only child is a `TextBlock` with the literal text **`"1"`**, bound in `EventConstruct` to `OnDiscardSlot{i}` → **`Siegebound|UI|RequestDiscardSlot(self, i)`**. ⛔ It is a **button**, ⛔ not a label, and it is the only route to `RequestDiscardSlot` in the widget. ⛔ **No disagreement with the record to report.**

### 3b. ⛔ EVERY dangling reference — the complete blast radius

**INSIDE the WBP (809 must remove these TOGETHER or the bar breaks at Construct):**

| # | Where | What |
|---|---|---|
| a | `BuildHandTree` loop | `ConstructObjectFromClass Button` + `ConstructObjectFromClass TextBlock` + `SetText "1"` + 2× `AddChild` + `Array|Add(SlotDiscardButtons)` |
| b | `EventConstruct` | **6×** `AssignOnClicked(Array\|Get(SlotDiscardButtons, i), OnDiscardSlotI)` + 6 `Array\|Get` + 6 variable getters |
| c | EventGraph | **6** custom events `OnDiscardSlot0..5`, each → `RequestDiscardSlot(self, i)` (12 nodes) |
| d | Variables | member `SlotDiscardButtons` (Button array) becomes unreferenced |

⚠️⚠️ **THE ORDERING TRAP: if (a) is removed but (b) is left, `Array|Get` on an EMPTY array returns null and `AssignOnClicked` fires on a null Button — `Accessed None` ×6 at every Construct.** (a)+(b)+(c) are one atomic edit.

✅ **Confirmed CLEAN elsewhere:** `SlotDiscardButtons` appears in **no** other graph — not `EventOnHandSlotUpdated`, `UpdateSlotArt`, `UpdateNextCardArt`, `ApplyCardTextShadows`, `WrapCardFace` or `UpdateGoldDisplay`. All 7 graphs read. The blast radius is exactly (a)–(d).

**OUTSIDE the WBP — ⛔ these dangle and are ⛔ NOT the art-director's to fix:**

| # | Reference | Effect of removal |
|---|---|---|
| e | `UCardHandWidget::RequestDiscardSlot` (`CardHandWidget.h:111`, `.cpp:108-118`) | Becomes uncalled from BP. No compile error — **dead BlueprintCallable surface.** ⭐ The right-click "discard all" would need it again. ⛔ Do not delete (C++ is out of art scope). |
| f | `ASiegePlayerController::DiscardHandSlot` (`.h:536`, `.cpp:1035-1140`) | ⛔ **Unreachable by any human input.** `DiscardCost` (`.h:1284`) still charged by it. |
| g | ⛔⛔ `SiegeControlsHelpWidget.cpp:480-492`, row **`Cards.Discard`** | ⚠️⚠️ **THE SHIPPED TAB HELP SCREEN GOES FALSE.** Summary: *"Click a hand card's discard button to bin it and draw a replacement — it costs gold."*; `ESiegeInputLane::PointerOnly`; `Row.bPointerOnly = true`; Detail: *"Reached only from the HUD: UCardHandWidget::RequestDiscardSlot → ASiegePlayerController::DiscardHandSlot."* **Every one of those sentences becomes a lie.** ⛔ `CARDBAR-§0` ruled this explicitly — *a wrong control in a help screen is worse than no help screen*, and any change to this button is a `HELP-§2` change that lands in the **SAME** task. ⭐ **Under the keep-and-relabel ruling no `HELP-§` edit was owed. Under Jonathan's REMOVE ruling, one IS owed.** |
| h | `Tests/SiegeControlsHelpTest.cpp:227` | Asserts a `Cards.Discard` row **exists** in the registry. ⛔ Deleting the row **FAILS the suite**. If right-click replaces the button, the row must be **REWRITTEN** (new lane/text), ⛔ never removed. |

⇒ ⭐ **(g)+(h) are the actual cost of the removal, and neither is inside my lane.** They need a `gameplay-programmer` task boarded alongside 809.

---

## 4. WHERE THE NON-INTERACTIVE KEY CHIP GOES

**⛔ It cannot go inside `slotVBox`.** The art `Image` fills the **entire** `FaceOverlay` (`HAlign_Fill`/`VAlign_Fill`), and `slotVBox` is the overlay's z1 child — so anything added to `slotVBox` renders **ON the card art**, which is "on the card", ⛔ not *"above the card"* (his words).

**✅ The host that satisfies "above the card":** a **new per-slot `VerticalBox`** wrapping the existing face —

```
slotColumn : VerticalBox            ← NEW; this is what enters _handbox instead of FaceOverlay
├─ [0] KeyChip : TextBlock          ← NEW. Visibility HitTestInvisible (non-interactive, his ask)
└─ [1] FaceOverlay : Overlay        ← EXISTING WrapCardFace return, unchanged
```

Built inside `BuildHandTree`'s loop; `Widget|Panel|AddChild(_handbox, slotColumn)` replaces the current `AddChild(_handbox, _faceoverlay)` — a **one-line** change in the loop body.

**Sizing it inherits:** `HorizontalBoxSlot` = **Auto** (size-to-content) — nothing is fixed-size, so the chip's width participates in the column width with no override to fight. ⭐ **"Make the space a bit smaller"** is achievable purely by the chip's font size (the existing scale is Name 16 / Cost 14 / preview 12 — **12 matches the smallest shipped size**) plus slot padding. ⛔ No `SizeBox` needs to be introduced, and introducing one would create the very fixed-size collapse hazard that does not exist today.

⚠️ `SlotBoxes[i]` must keep pointing at **`slotVBox`**, not the new `slotColumn` — `EventOnHandSlotUpdated` collapses `SlotBoxes[SlotIndex]` to hide an empty slot. If the chip should also vanish on an empty slot, it needs its own explicit collapse (or `SlotBoxes` must be re-pointed at `slotColumn` — **a semantic change, so it is 809's call to make deliberately, not by accident**).

---

## 5. THE SHIPPED BINDING IDIOM — ⭐ follow it, do not invent one

⛔ **There is NO UMG property binding anywhere in this widget.** Zero. The idiom is **event-driven push → explicit imperative set**, with extra data **PULLED** by a `BlueprintCallable` C++ getter called *inside* the handler:

1. C++ fires the BIE `OnHandSlotUpdated(SlotIndex, CardID, DisplayName, Cost, bAffordable)`.
2. `EventOnHandSlotUpdated` does `Array|Get(SlotBoxes, SlotIndex)` → **`IsValid` guard** → work.
3. Anything not carried by the BIE is pulled inside that guard. **The shipped exemplar is `UpdateSlotArt(SlotIndex, CardID, bAffordable)` calling `GetCardArtTexture(CardID)`** — `CARDBAR-§3`'s named PULL precedent (`TASK-079` ruling 3), with `IsValid(texture)` ⇒ show, `null` ⇒ `SetVisibility(Collapsed)`.

⇒ ⭐ **`GetSlotKeyLabel` follows it exactly — a new function graph `UpdateSlotKeyChip(SlotIndex)` called from inside `EventOnHandSlotUpdated`'s `IsValid` guard (beside the existing `UpdateSlotArt` call), doing: `GetSlotKeyLabel(SlotIndex)` → `IsEmpty?` → **empty ⇒ `SetVisibility(KeyChip, Collapsed)`** (degrade-open, `CARDBAR-§3`) : **`SetText(KeyChip, label)` + `SetVisibility(KeyChip, HitTestInvisible)`**.** Node-for-node the `UpdateSlotArt` shape. ⛔ No new BIE param, ⛔ no property binding.

⚠️ Slots re-push on **every gold tick**, so this runs constantly — which is exactly why `CARDBAR-§3` mandates the once-per-slot warning guard on the C++ side.

---

## 6. ⭐ PER-SLOT vs TEMPLATED — **HYBRID. The answer is not one number.**

| Part | Shape | Edit cost |
|---|---|---|
| **Construction** (`BuildHandTree`) | ⭐ **ONE templated loop body**, `(for _ (range 6) …)` | ✅ **ONE change** |
| **Bindings** (`EventConstruct`) | ⛔ **SIX literal, unrolled** `AssignOnClicked` calls with hardcoded indices 0..5 | ⛔ **SIX changes** |
| **Handlers** | ⛔ **SIX literal custom events** `OnDiscardSlot0..5` / `OnPlaySlot0..5` | ⛔ **SIX × 2 changes** |
| **Per-slot update** (`EventOnHandSlotUpdated`) | ⭐ **ONE** index-driven handler | ✅ **ONE change** |

⇒ Adding the key chip = **1 edit**. Removing the Play button = **1 + 6 + 6**. Removing the discard button = **1 + 6 + 6**.

⚠️ **`TASK-041`'s recorded MCP quirk applies to any re-binding:** `Button|Event|AssignOnClicked(btn, AddEvent|Custom|<name>)` **ignores the name**, auto-creates an empty `OnClicked_Event_N`, and leaves the intended body-event orphaned — it must be fixed by granularly `connect_pins`-ing the body event's `OutputDelegate` onto the button's Delegate pin. **18 inert `OnClicked_Event_0..17` stubs are already in the graph** from exactly this (measured — they are unbound dead code, ⛔ not a defect, ⛔ do not let them confuse QA).

---

## 7. ⛔⛔ RIGHT-CLICK VERDICT — NOTHING CONSUMES IT, AND THE CARD FACE IS **NOT** CLICKABLE WITHOUT THE BUTTONS

**(i) Does any node consume right-click? ⛔ NO.** All 7 graphs read: there is **no** `OnMouseButtonDown`, **no** `OnPreviewMouseButtonDown`, **no** mouse-event override, **no** `Handled` return anywhere. The only input surface in the widget is `UButton::OnClicked` — and `UButton`'s CDO reads `ClickMethod: DownAndUp`, `IsFocusable: true`; `SButton` responds to the **LEFT** mouse button only. ⛔ **RMB fires nothing today, on any node.**

**(ii) Is the card even hit-testable for it? — per-node, measured:**

| Node | Visibility | Hit-tests? |
|---|---|---|
| `WBP_CardHand` self / `Overlay_19` | `SelfHitTestInvisible` (CDO **and** set in EventConstruct) | self ❌, children ✅ |
| `FaceOverlay` | `SelfHitTestInvisible` (`WrapCardFace`) | self ❌, children ✅ |
| `ArtImage` | `Collapsed` → `HitTestInvisible` on resolve | ❌ **never** |
| `slotVBox` | `SelfHitTestInvisible` (CDO + explicit set) | self ❌, children ✅ |
| `NameText`, `CostText` | ⭐ **`Visible`** — `UTextBlock` CDO default, **never overridden anywhere** | ✅ **yes** |
| `PlayButton`, `DiscardButton` | `Visible` | ✅ yes |

⇒ ⛔⛔ **SCOPE FINDING — `TASK-809` §(1) told me to STOP and report exactly this: `CARDBAR-§1`'s claim that *"the CARD FACE itself can carry the click"* is ⛔ REFUTED AS BUILT.** There is no clickable face. The face is an Overlay + VerticalBox + Image + two TextBlocks; the Overlay/VBox are `SelfHitTestInvisible`, the Image is `HitTestInvisible`, and while the two TextBlocks *do* hit-test (`Visible`), a `TextBlock` **handles no mouse event** — the click falls through unhandled. ⛔ **Remove both buttons and the card bar is mouse-dead**, LMB and RMB alike.

⇒ Making right-click work therefore needs a node that **handles** mouse input over the card — and ⛔ **I am fenced from designing it, so I report only**: neither of the two obvious routes is free — a plain `UButton` wrapper still will not fire on RMB (`OnClicked` is LMB-only), and a `UUserWidget` with `OnMouseButtonDown` overridden is a **new WidgetBlueprint + C++**, i.e. ⛔ outside 809's "edit in place, no new WBP" fence. ⭐ **This needs a manager ruling before 809 can promise right-click.**

---

## 8. ⚠️ THE LANDMINE FOR `TASK-809` — `Btn_Jump` IS LOAD-BEARING

`BuildHandTree` reaches the root Overlay by walking the **donor** button:

```
(bind _asoverlay (Utilities|Casting|CastToOverlay
                   (Widget|GetParent (Widget|GetParent (Variables|WBP_CardHand|GetBtn_Jump))))
  (:then  … attach _handbox, the preview face, the refusal text …)
  (:CastFailed (Development|PrintString "WBP_CardHand: root Overlay not found; hand not attached")))
```

Verified by read-back: `Btn_Jump.Slot` → `SizeBox_0.SizeBoxSlot_0`; `SizeBox_0.Slot` → `Overlay_19.OverlaySlot_0`; `Overlay_19.Slot` → **`None`** (root). So `GetParent(GetParent(Btn_Jump))` **is** `Overlay_19`.

⇒ ⛔⛔ **DELETING OR REPARENTING `Btn_Jump` MAKES THE ENTIRE CARD BAR VANISH** (cast fails ⇒ nothing is attached ⇒ silent empty hand + one PrintString). ⛔ Do not touch it.

⚠️ **AND A SEPARATE OBSERVATION FOR THE RECORD:** `Btn_Jump` in **this** widget reads `Visibility: Visible` — ⛔ it is **not** collapsed. `TASK-041`'s `SetVisibility(Btn_Jump, Collapsed)` lives in **`WBP_HUD`'s** Tick and targets **`WBP_HUD`'s own** instance variable; it cannot reach the card-hand instance. Nothing in any of `WBP_CardHand`'s 7 graphs collapses it, and its donor handler `OnClicked_Event` (→ `EnterPlacementMode "Footman"`) is present but **unbound** (the Construct that bound it was rewritten by TASK-041). Its `SizeBox_0` sits **HAlign_Center / VAlign_Bottom / pad B40, 240×80** — i.e. **bottom-centre, overlapping the hand bar's own bottom-centre B24 attach**. It renders *behind* the cards (design-time child added before the runtime `AddChildToOverlay`), but it is `Visible` and hit-testable. ⭐ **Flagged for Jonathan's pixel eye at `TASK-811`, ⛔ not fixed here — and ⛔ collapsing it is safe for the attach (`GetParent` works on a collapsed widget) whereas deleting it is fatal.**

---

## 9. ⚠️ PACKAGE DIRTY STATE — DECLARED, AND THE SAVE IS ⛔ DECLINED

`AssetTools.is_dirty("/Game/UI/WBP_CardHand")` → **`true`**. The editor status bar reads **"1 Unsaved"**, and it is this package (`L_Arena` → `false`, `WBP_HUD` → `false`, both verified).

**⛔ Nothing reached disk, and that is measured:**
- `Content/UI/WBP_CardHand.uasset` mtime = **2026-07-07 17:52:28**, size 821274, sha256 `5c3ce72eaa80b5da3c90e44f425653b6b75b6a3b5559b53297a906d1419e5078`.
- `git status --short Content/UI/` → **empty (clean)**.
- Every MCP call I made was a read (`list_*`, `read_graph_dsl`, `get_*`, `find_node_types`, `is_dirty`, `IsPIERunning`, `CaptureEditorImage`). ⛔ **`save_assets` was never called, on any path.**

**Probable cause (labelled as inference, ⛔ not measured):** the editor started `02:38:52` from the **18:45:10** DLL, and my reads were the session's first load of this Blueprint. `TASK-807` changed the native parent `UCardHandWidget` **after** that DLL was built, so a compile-on-load against the stale-vs-source parent is the likely dirtier. No `recompile`/`Compiling Blueprint` line appears in the log to confirm it. ⛔ **This is exactly the case `TASK-808` §(3) anticipated: report it and decline the save. Declined.**

⚠️ **For build-master at `TASK-811` (`§25b`):** the working tree is currently **clean** for this file. If a later step saves it, the editor's SCC **auto-stages by itself** — verify **oid-vs-worktree-sha256**, ⛔ never a size check.

---

## 10. ⛔ PIXELS — STATED PLAINLY, NOT CLAIMED

`TASK-808` §(4) asks for a screenshot of the live card bar **if the tooling allows**. ⛔ **It does not, read-only.** The bar only exists at **runtime** (§1b — it is built by `BuildHandTree` on `EventConstruct`), so seeing it requires **PIE**, and PIE is a state change on an editor whose lifecycle the orchestrator owns. ⛔ **I did not start PIE.**

✅ What I did capture, read-only: `CaptureEditorImage` — editor healthy, **no modal dialog**, `L_Arena` loaded, `IsPIERunning = false`, no asset-editor window open, status bar "1 Unsaved" (§9). Saved to `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\editor.png`.

⛔⛔ **Everything in this handoff is a property/graph readback, and `AS-§6` A(e) / `HELP-§3` are explicit that a readback is ⛔ NOT a pixel. The live bar remains unobserved. `TASK-811` is still the real gate.**

---

## 11. WHAT `TASK-809` MUST HAVE RULED BEFORE IT STARTS

1. 🚨 **The compile ordering** (Finding A) — `GetSlotKeyLabel` is not bindable until the module is rebuilt. **Blocking.**
2. 🚨 **Right-click** (§7) — the card face is mouse-dead without the buttons; RMB has no handler and `UButton` cannot provide one. Needs a ruling, ⛔ not an invention.
3. 🚨 **The help-screen debt** (§3b g/h) — removing the discard button owes a `HELP-§2` rewrite **and** a test change, both in `gameplay-programmer`'s lane, boarded **with** 809.
4. ✅ Non-blocking: the `SlotBoxes` re-point decision (§4) and the `Btn_Jump` visibility observation (§8).

## Files
- `.claude/pipeline/handoffs/TASK-808-artist.md` — this file, **the only thing written.**
- ⛔ Not touched: `/Game/UI/WBP_CardHand` (read-only), `/Game/UI/WBP_HUD`, `L_Arena`, all C++, TASKBOARD.md, Git.
