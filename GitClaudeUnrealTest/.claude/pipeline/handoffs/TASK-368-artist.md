# TASK-368 — [AG-A1] MCP: brushes/tints on `BoostBar` + `BoostOutline`, `SetDamageBoost` as a VERIFIED `K2Node_Event`

**Agent:** art-director
**Date:** 2026-08-02
**Editor/MCP:** up (PID 23372). PIE confirmed stopped (`IsPIERunning = false`) before and throughout.
**Asset touched:** `/Game/UI/WBP_CombatantHealthBar` — **and nothing else.** No Git, no C++, no Build.bat, no save-all, `L_Arena` never opened or saved.
**Status:** ready-for-integration.

> ⚠️ **RENDERING IS UNVERIFIED. See §7.** Everything below is *structural* readback from the live editor. I did not and cannot see a
> pixel of this widget. **TASK-377 (Jonathan's PIE gate) is the only thing that can close the visual question.**

---

## 1. PRE-FLIGHT — the tree Jonathan built (TASK-367), machine-verified

| Check | Method | Result |
|---|---|---|
| Asset exists | `AssetTools.exists` | `true` |
| Parent class | `BlueprintTools.get_parent` | `/Script/GitClaudeUnrealTest.CombatantHealthBarWidget` ✓ |
| Was saved / clean on arrival | `AssetTools.is_dirty` | `false` ✓ |
| `BarStack` class | `ObjectTools.get_class` | `/Script/UMG.VerticalBox` ✓ |
| `BoostOutline` class | `ObjectTools.get_class` | `/Script/UMG.Border` ✓ |
| `BoostBar` class | `ObjectTools.get_class` | `/Script/UMG.ProgressBar` ✓ |
| `Bar` class | `ObjectTools.get_class` | `/Script/UMG.ProgressBar` ✓ |

**Hierarchy proven by slot parentage** (each widget's `slot` refPath names its owning panel — this is a hard structural read, not an assumption):

- `BoostBar.slot` = `…:WidgetTree.**BoostOutline**.BorderSlot_0` → **BoostBar is a child of BoostOutline** ✓
- `BoostOutline.slot` = `…:WidgetTree.**BarStack**.VerticalBoxSlot_4` → **BoostOutline is a child of BarStack** ✓
- `Bar.slot` = `…:WidgetTree.**BarStack**.VerticalBoxSlot_5` → **Bar is a child of BarStack** ✓

⇒ `BarStack` (VerticalBox) ▸ `BoostOutline` (Border) ▸ `BoostBar` (ProgressBar), plus `Bar` (ProgressBar) as BarStack's other child. **Matches the contract.**

**Is Variable — BOTH new widgets CONFIRMED.** `bIsVariable` is not a readable UPROPERTY over MCP (`get_properties` refuses it), so I used the
authoritative test instead: `find_node_types(type_id_filter = "Variables|WBP_CombatantHealthBar|")` returned

```
["Variables|WBP_CombatantHealthBar|GetBoostOutline",
 "Variables|WBP_CombatantHealthBar|GetBoostBar",
 "Variables|WBP_CombatantHealthBar|GetBar"]
```

A widget only publishes a `Get<Name>` node type if **Is Variable** is ticked. Both new widgets publish one, and both getters were then
successfully instantiated and connected in the graph (§4) — the strongest possible confirmation.

**`SetDamageBoost` is live as an INHERITED overridable event.** `list_events` shows it with `bIsImplemented: false` and the full C++ doc
comment from `CombatantHealthBarWidget.h` — i.e. TASK-366's compile landed and the BIE is visible to the WBP. This is what guarantees
`add_event` produces a genuine override rather than a custom event.

### ✅ 1.1 ONE DEVIATION FROM TASK-367 — FOUND, FLAGGED, AND (ON AUTHORIZATION) FIXED

**As first found**, TASK-367 step 8 had not landed: `Bar`'s slot was `Fill 1.0`, not `Fill 2.0`. I flagged it rather than patching it,
because `Bar` was declared off-limits and I was told not to silently work around a tree that differs from spec.

**The orchestrator then explicitly AUTHORIZED the fix** (2026-08-02), with the reasoning recorded here so the off-limits rule is not
muddied later: **a `VerticalBoxSlot` is a layout property of the CONTAINER, not a property of `Bar` itself.** The prohibition protects
`Bar`'s brushes, colours, fill style and `Percent` — the fields TASK-131 burned nine attempts on. None of those were touched.

**The single call made:**
```
ObjectTools.set_properties
  instance = /Game/UI/WBP_CombatantHealthBar.WBP_CombatantHealthBar:WidgetTree.BarStack.VerticalBoxSlot_5
  values   = {"size":{"value":2.0,"sizeRule":"Fill"}}
```

**Final slot state, read back after compile + save:**

| Slot | Spec (TASK-367) | Final readback | Verdict |
|---|---|---|---|
| `BoostOutline` → `VerticalBoxSlot_4` | Fill 1.0, HAlign/VAlign Fill, Pad Bottom 1 | `{value:1, sizeRule:Fill}`, `HAlign_Fill`/`VAlign_Fill`, padding `{0,0,0,1}` | ✅ correct (untouched) |
| `Bar` → `VerticalBoxSlot_5` | **Fill 2.0**, HAlign/VAlign Fill | **`{value:2, sizeRule:Fill}`**, `HAlign_Fill`/`VAlign_Fill`, padding `{0,0,0,0}` | ✅ **FIXED** |

The 22 px `DrawSize` now splits **~7 px boost / ~15 px health** as designed, instead of the 11/11 it would have shipped with.

**`Bar` itself re-verified UNCHANGED after the slot edit** — a third full property capture (`widgetStyle` incl. both brushes, `percent 1`,
`barFillStyle Scale`, `barFillType LeftToRight`, `fillColorAndOpacity {0,0.5,1,1}`, `visibility Visible`, `renderOpacity 1`) is byte-identical
to the pre-work baseline. Recompiled and re-saved; **`is_dirty = false`.**

### 1.2 Honest limit — child ORDER is not machine-readable
`UPanelWidget::Slots` is not exposed over MCP (`get_properties` refuses it), so I **cannot** prove that `BoostOutline` sits at index **0**
(above) and `Bar` at index **1** (below) rather than the reverse. Parentage is proven; vertical ORDER is not. The slot object numbering
(`VerticalBoxSlot_4` for BoostOutline vs `_5` for Bar) is *suggestive* of the correct order but is creation order, not child order, so I am
not counting it as proof. **Add "boost row is ABOVE the health bar" to the TASK-377 eyeball checklist.**

---

## 2. `BoostBar` (ProgressBar) — every property set, with post-save readback

Set via one `ObjectTools.set_properties`; **partial struct writes were confirmed non-destructive** (sibling fields such as `marqueeImage`
survived byte-identical).

| Property | Value set | Readback after compile+save |
|---|---|---|
| `widgetStyle.fillImage.resourceObject` | `/Engine/EngineResources/WhiteSquareTexture` | `{refPath: /Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture}` ✓ |
| `widgetStyle.fillImage.drawAs` | `Image` | `Image` ✓ |
| `widgetStyle.fillImage.tintColor` | white `(1,1,1,1)` `UseColor_Specified` | `{1,1,1,1}` ✓ **identity multiply** |
| `widgetStyle.fillImage.imageType` | `FullColor` | `FullColor` ✓ |
| `widgetStyle.fillImage.imageSize` / `margin` | `{32,32}` / `{0,0,0,0}` | matches ✓ (mirrors the proven `Bar` fill) |
| `widgetStyle.backgroundImage.tintColor` | **`(0.22, 0.22, 0.24, 0.85)`** | `{0.2199999988, 0.2199999988, 0.2399999946, 0.8500000238}` ✓ |
| `widgetStyle.backgroundImage.drawAs` | `RoundedBox` | `RoundedBox` ✓ |
| `widgetStyle.backgroundImage.resourceObject` | (left `None`) | `None` ✓ — solid colour track, **no material** |
| `barFillStyle` | `Scale` | `Scale` ✓ (was `Mask`) |
| `barFillType` | `LeftToRight` | `LeftToRight` ✓ |
| `percent` | `0` | `0` ✓ |
| `visibility` | `HitTestInvisible` | `HitTestInvisible` ✓ |
| `renderOpacity` | **`0`** (set LAST, §6) | `0` ✓ |

**NO material anywhere on this widget.** Trap 2 (TASK-131's real root cause — a material fill brush swallowing the runtime tint) is closed by
construction: plain texture + `DrawAs = Image` + white tint, exactly the setup that made `Bar` work.

## 3. `BoostOutline` (Border) — every property set

| Property | Value set | Readback after compile+save |
|---|---|---|
| `background.resourceObject` | `/Engine/EngineResources/WhiteSquareTexture` | `{refPath: …WhiteSquareTexture.WhiteSquareTexture}` ✓ |
| `background.drawAs` | `Image` | `Image` ✓ (was `Image` with a null resource / `NoImage`) |
| `background.tintColor` | white `(1,1,1,1)` | `{1,1,1,1}` ✓ **identity** |
| `background.imageType` | `FullColor` | `FullColor` ✓ (was `NoImage`) |
| `background.imageSize` / `margin` | `{32,32}` / `{0,0,0,0}` | matches ✓ |
| `brushColor` | white `(1,1,1,1)` | `{1,1,1,1}` ✓ |
| `visibility` | `HitTestInvisible` | `HitTestInvisible` ✓ |
| `renderOpacity` | **`0`** (set LAST, §6) | `0` ✓ |
| `padding` | *(untouched — Jonathan's)* | `{1.5, 1.5, 1.5, 1.5}` ✓ |

**Why both tints must be white:** `UBorder::SetBrushColor` drives `BrushColor` → `SBorder::SetBorderBackgroundColor`, which **multiplies**
against the background brush's own tint. White design-time tint = identity, so the C++-pushed band colour lands undimmed. Same logic on
`BoostBar`: `SetFillColorAndOpacity` multiplies against `fillImage.tintColor`.

---

## 4. Event graph — `SetDamageBoost`

### 4.1 ⚠️ THE CLASS ASSERTION (trap 1 — the defect that hid the health-bar bug five times)

Authored with `BlueprintTools.add_event(event_name = "SetDamageBoost")` → node `…:EventGraph.**K2Node_Event_3**`.

**`ObjectTools.get_class` on that node returns, verbatim:**

```
/Script/BlueprintGraph.K2Node_Event
```

**NOT `K2Node_CustomEvent`.** Re-asserted a second time *after* the compile — same result. Two further independent corroborations:

- `get_node_infos` → `type_id = "AddEvent|Siegebound|UI|EventSetDamageBoost"` — the `AddEvent|` prefix is the override form, and it matches
  the pattern of the two BIEs TASK-131 verified (`AddEvent|Siegebound|UI|EventOnHPChanged` / `…EventSetTeamColor`).
- The engine's own error string when probing `bOverrideFunction` echoed the class independently:
  `GetObjectProperties on '…EventGraph.K2Node_Event_3' **(K2Node_Event)**`.

**I did not use `bIsImplemented` as evidence anywhere** — per TASK-131 round 1 that readback lies.

**Signature verified pin-by-pin** — exactly 5 float outputs, in the contract order, no extras:
`FillFraction` (idx 2) · `R` (3) · `G` (4) · `B` (5) · `RowOpacity` (6), all `Float (single-precision)`.

### 4.2 The wiring — final `read_graph_dsl` (post-compile, post-save)

```
(event Siegebound|UI|EventSetDamageBoost (FillFraction R G B RowOpacity)
  (bind _linearcolor (Utilities|Struct|MakeLinearColor R G B 1.0))
  (bind _boostbar    (Variables|WBP_CombatantHealthBar|GetBoostBar))
  (bind _boostoutline (Variables|WBP_CombatantHealthBar|GetBoostOutline))
  (Progress|SetPercent              _boostbar     FillFraction)
  (Progress|SetFillColorAndOpacity  _boostbar     _linearcolor)
  (Appearance|SetBrushColor         _boostoutline _linearcolor)
  (Widget|SetRenderOpacity          _boostoutline RowOpacity)
  (Widget|SetRenderOpacity          _boostbar     RowOpacity))
```

One linear exec chain in the exact spec order. **ONE** `MakeLinearColor` (`_linearcolor`), genuinely fanned to **both** consumers — the DSL's
`bind` proves a single shared node, not two copies. **ZERO conditionals** — no Branch, no compare, no divide, no select, no enum. The widget
is the dumb pipe the C++ contract requires.

**Trap 4 — the alpha literal:** `get_pin_value` on `MakeLinearColor.A` (input idx 3) returns **`"1.0"`**. Set explicitly and read back, not assumed.

`SetBrushColor` resolved to the **Border** overload, verified before wiring: its `self` pin type is `Border Object Reference`
(`get_node_type_pins` on `Appearance|SetBrushColor`) and it is fed by `GetBoostOutline`, whose output pin type is `Border Object Reference`.

### 4.3 ⚠️ ORPHAN CHECK (trap 3) — clean

I deliberately **did NOT use `write_graph_dsl`**. It would have required re-emitting the whole graph including the two working events, which is
exactly the operation TASK-131 recorded as silently orphaning nodes. Instead: `create_node` + `connect_pins` + `set_pin_value`, purely additive,
touching nothing that already existed.

`find_nodes(title = "")` on the EventGraph:

| | Count | Detail |
|---|---|---|
| **Before** | **14** | `K2Node_Event_0/_1/_2/_5/_6`, `PromotableOperator_4/_5`, `CallFunction_19/_20/_21`, `IfThenElse_2`, `VariableGet_6/_7`, `MakeStruct_4` |
| **After** | **23** | all 14 above **present with identical refPaths**, + the 9 I created |
| **Delta** | **+9** | `K2Node_Event_3` (SetDamageBoost) · `VariableGet_1` (GetBoostBar) · `VariableGet_2` (GetBoostOutline) · `MakeStruct_2` (MakeLinearColor) · `CallFunction_6` (SetPercent) · `_7` (SetFillColorAndOpacity) · `_8` (SetBrushColor) · `_9` (SetRenderOpacity→Outline) · `_10` (SetRenderOpacity→BoostBar) |

**9 created, 9 present, 0 orphans, 0 duplicates, 0 strays.** Every created node is reachable from the event.

*(Note: `get_node_type_pins` reports pins against transient probe nodes — I re-ran `find_nodes` afterwards and confirmed it leaves **no**
residue in the graph.)*

### 4.4 `OnHPChanged` / `SetTeamColor` — BYTE-UNTOUCHED
Their DSL text after my work is **character-for-character identical** to the read I took before touching anything, and all 14 of their nodes
kept their original refPaths (no re-creation):

```
(event Siegebound|UI|EventOnHPChanged (CurrentHP MaxHP)
  (if (> MaxHP 0.0)
    (Progress|SetPercent (Variables|WBP_CombatantHealthBar|GetBar) (/ CurrentHP MaxHP))))

(event Siegebound|UI|EventSetTeamColor (R G B)
  (Progress|SetFillColorAndOpacity (Variables|WBP_CombatantHealthBar|GetBar) (Utilities|Struct|MakeLinearColor R G B 1.0)))
```

---

## 5. `Bar` — PROVABLY UNTOUCHED

I captured `Bar`'s full state **before** any edit and re-read it **after** compile+save. Identical on every field:

| Property | Before | After |
|---|---|---|
| `widgetStyle.backgroundImage.tintColor` | `{0.03, 0.03, 0.03, 0.7}` RoundedBox | **same** |
| `widgetStyle.fillImage` | `WhiteSquareTexture` / `Image` / white / `{32,32}` | **same** |
| `widgetStyle.marqueeImage`, `enableFillAnimation` | engine defaults | **same** |
| `barFillStyle` / `barFillType` | `Scale` / `LeftToRight` | **same** |
| `percent` | `1` | **same** |
| `fillColorAndOpacity` | `{0, 0.5, 1, 1}` | **same** |
| `visibility` | `Visible` | **same** |
| `renderOpacity` | `1` | **same** |

Its near-black track was **not** touched — only `BoostBar` got the new medium-grey `(0.22,0.22,0.24,0.85)`, which is the whole point (band-4
black would compute ~1.3:1 on the near-black health track). Note `Bar.visibility` is `Visible`, **not** `HitTestInvisible` — that is its
pre-existing shipped state and I left it exactly as found rather than "improving" it.

---

## 6. Order of operations, compile, save

`RenderOpacity = 0` was applied **LAST**, after every other property and the entire graph were set and verified, exactly as instructed — so the
widgets stayed visible in the Designer during the work and the defensive default is what ships. A unit that never receives a
`SetDamageBoost` push renders **health-bar-only**, identical to today.

- `compile_blueprint` — ran twice (after wiring, and again after the opacity change). **No errors.**
  `LogBlueprint` for this asset contains only `Compiling Blueprint '/Game/UI/WBP_CombatantHealthBar…'` lines — zero error/warning entries.
  The only `Warning:` lines in the log naming this asset are my own read-only `GetObjectProperties` probes (`bIsVariable`, `Slots`,
  `RootWidget`, `bOverrideFunction`) — introspection noise, not asset state.
- `save_assets(["/Game/UI/WBP_CombatantHealthBar"])` → `true`. **Single asset only — never save-all.**
- **`is_dirty` = `false`** ✓

---

## 7. ⚠️ RENDERING IS UNVERIFIED — TASK-377 IS THE ONLY GATE

**I am not claiming this bar renders correctly, because I have no way to check.** `CaptureAssetImage` refuses WidgetBlueprints
("Asset type does not support image capture") and screen-space Slate on a `UWidgetComponent` is uncapturable headless — the standing limit from
TASK-131, unchanged. Everything in this document is **structural readback from the live editor**, and TASK-131 is a 267-line record of MCP
readback passing on a widget that was visually broken. Structure passing is necessary, not sufficient.

**What Jonathan should specifically look for at TASK-377:**
1. The boost row is **ABOVE** the health bar, not below (§1.2 — order is not machine-verifiable).
2. Row proportions — now expect **~7 px boost / ~15 px health** (§1.1 `Fill 2.0` was authorized and applied).
3. The boost fill shows the **band colour**, not grey/white (that failure mode = the tint is being swallowed, TASK-131's signature).
4. The fill **drains/grows with the value** (`BarFillStyle = Scale` behaviour).
5. The outline ring is the **same colour** as the fill (they share one `MakeLinearColor`).
6. An **unboosted** unit shows health-bar-only — no ghost strip (`RenderOpacity = 0` default).
7. The health bar's **head offset is constant** between boosted and unboosted units (the whole reason hiding is opacity, not visibility).

---

## 8. For integration

- **Asset:** `/Game/UI/WBP_CombatantHealthBar` (`Content/UI/WBP_CombatantHealthBar.uasset`) — compiled, saved, `is_dirty = false`.
- **No new source files.** Zero `Content/RawAssets/` output — this is an editor-only UMG task, no Blender, no FBX, no textures.
  The only external asset referenced is the engine's `/Engine/EngineResources/WhiteSquareTexture` (already a dependency via `Bar`).
- **The BIE contract is met exactly:** `SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity)` — 5 floats, that
  order, genuine `K2Node_Event` override. `UCombatantHealthBarComponent::PushDamageBoost` can drive it as written.
- **Nothing was wired into any actor or Blueprint** — no integration performed, per role.
- **§1.1 slot deviation is CLOSED** — authorized and applied; `Bar`'s slot is `Fill 2.0`, `Bar` itself still byte-identical.
- **Remaining open item: none for this task.** The only outstanding uncertainty is child ORDER (§1.2), which is not machine-readable and
  belongs on the TASK-377 eyeball checklist.
