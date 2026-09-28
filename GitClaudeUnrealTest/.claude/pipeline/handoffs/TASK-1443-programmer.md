# TASK-1443 — ONKEYDOWN-INSTRUMENT-CONTROL — programmer handoff

- **Agent:** gameplay-programmer, 2026-09-27, the middle passenger of the batched read-only session TASK-1439 → TASK-1443 → TASK-1445 on PID 3108.
- **(4) branch: (a). The override IS listed.** `get_asset_meta` lists an overridden UMG `On Key Down` as a Function (`OnKeyDown [Inputs(FGeometry MyGeometry = (),FKeyEvent InKeyEvent = ()), Outputs(FEventReply ReturnValue = ())]`). This was shown on a donor that an independent instrument proves carries the override, and replicated on a second donor.
- **Donor (full path):** `/Landmass/Landscape/BlueprintBrushes/Widgets/LandmassViewport_Widget.LandmassViewport_Widget` (engine plugin `Landmass`, file `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\Landmass\Content\Landscape\BlueprintBrushes\Widgets\LandmassViewport_Widget.uasset`, sha256 `45767469493b7210f80bb4563756b00a9fcc13019ba5e18c66639f1f6b99a8d8`). Replication donor: `/PCG/Utilities/Assemblies/ActorTagger/Widgets/Widget_Tagging.Widget_Tagging`.
- **No Blueprint was authored** (clause (3) never triggered, because a donor existed). No asset written, no code, no compile, no PIE, no git.

## 0. State (`SC-§138`)

- Editor: PID **3108**, the GUI editor by command line (re-identified at session start, the same instant as TASK-1439's §0). `is_pie_active` false at session start. PIE never started.
- **`BROKEN_BP_LOADED`: True at the start of this row, True at its end.** It was loaded by TASK-1439 earlier in the same session, on purpose. This row loaded **nothing** from `sA_ArcheryVfxPack`. Pack residency at the end of the row: `['/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement']`, the same single package TASK-1439 left.
- Dirty: `DIRTY_CONTENT=[]` / `DIRTY_MAPS=[]` at the start and end of this row.
- **What this row loaded:** the two donors and their dependency trees, all engine-plugin content: about 150 packages under `/Landmass/...` (the Landmass donor drags in its brushes, materials and structs) and 9 under `/PCG/Utilities/Assemblies/ActorTagger/...`. In-memory Blueprints went from 47 to 77. **The in-memory `BS_ERROR` set is still exactly one** (`BP_Basic_Movement`), so none of the donor loads added a new PIE-modal trigger. Every loaded package clears on the `TASK-1538` relaunch.

## 1. The donor search (clause (2), narrowed by (C): the ten screens were not re-read)

**Candidate finder (loads nothing):** a byte scan of package name tables. An overridden `On Key Down` is a function graph named `OnKeyDown`, so the FName `OnKeyDown` sits in plain ASCII in the uncooked `.uasset` name table. `grep -l -a -F -e OnKeyDown -e OnPreviewKeyDown`:

| Scope | Files scanned | Hits |
|---|---|---|
| `Engine/Content` + `Engine/Plugins/**` (`.uasset`) | 16,375 | **6**, all `OnKeyDown`, none `OnPreviewKeyDown` |
| Project `Content/` (incl. `sA_ArcheryVfxPack`, the template `Variant_*`/`ThirdPerson` folders and the ten screens) + project `Plugins/` (`.uasset`+`.umap`) | 4,416 | **0** |

The 6 engine hits: `ImpostorBaker/.../ImpostorViewport_Widget` (plugin **not mounted**) · `Landmass/.../LandmassViewport_Widget` · `Landmass/.../EUW_Landscape_Erosion` · `PCG/.../ActorTagger/ActorTagger` · `PCG/.../ActorTagger/Widgets/Widget_Tagging` · `VirtualProduction/PerformanceCaptureWorkflow/.../EW_Viewport` (plugin **not mounted**). Mount check via `AssetRegistry.get_assets_by_package_name` (no load): the four mounted hits are all `EditorUtilityWidgetBlueprint`, parent `/Script/Blutility.EditorUtilityWidget`. `EditorUtilityWidget` derives from `UUserWidget`, so its `On Key Down` is the same `UUserWidget::OnKeyDown` override a menu screen would carry.

## 2. The control, verbatim

### 2.1 The instrument under test: `get_asset_meta` (parts `Functions`, `Events`) on the donor, VERBATIM

```
Blueprint Name: LandmassViewport_Widget
Path: /Landmass/Landscape/BlueprintBrushes/Widgets/LandmassViewport_Widget.LandmassViewport_Widget

Parent Class: EditorUtilityWidget

Functions:
- OnMouseWheel [Inputs(FGeometry MyGeometry = (),FPointerEvent& MouseEvent), Outputs(FEventReply ReturnValue = ())]
- OnMouseButtonDown [Inputs(FGeometry MyGeometry = (),FPointerEvent& MouseEvent), Outputs(FEventReply ReturnValue = ())]
- OnMouseButtonUp [Inputs(FGeometry MyGeometry = (),FPointerEvent& MouseEvent), Outputs(FEventReply ReturnValue = ())]
- OnKeyUp [Inputs(FGeometry MyGeometry = (),FKeyEvent InKeyEvent = ()), Outputs(FEventReply ReturnValue = ())]
- OnKeyDown [Inputs(FGeometry MyGeometry = (),FKeyEvent InKeyEvent = ()), Outputs(FEventReply ReturnValue = ())]
- OnMouseMove [Inputs(FGeometry MyGeometry = (),FPointerEvent& MouseEvent), Outputs(FEventReply ReturnValue = ())]

Events:
- Construct [No Inputs]
- Tick [Inputs(FGeometry MyGeometry = (),float InDeltaTime = 0.000000)]
- OnMouseLeave [Inputs(FPointerEvent& MouseEvent)]
- Set Camera LookAt [Inputs(FVector FocusPoint = (X=0.000000, Y=0.000000, Z=0.000000),double Distance = 0.000000,FVector ViewDirection = (X=0.000000, Y=0.000000, Z=0.000000))]
```

**The override is visible**, and it renders under **Functions**, not Events, exactly as clause (1) predicted (it returns `FEventReply`).

### 2.2 Ground truth from an independent instrument, with negative controls

The donor file has to be proven to carry the override by something other than `get_asset_meta`, or the control proves nothing:

- **`unreal.BlueprintEditorLibrary.find_graph(bp, name)`** (read-only, on the loaded `EditorUtilityWidgetBlueprint`):
  - `OnKeyDown` → `/Landmass/.../LandmassViewport_Widget.LandmassViewport_Widget:OnKeyDown` ✅ **exists**
  - `OnKeyUp` → exists ✅
  - `OnPreviewKeyDown` → `None` · `OnKeyChar` → `None` · `OnFocusReceived` → `None` · `NoSuchGraph_1443` → `None`
- **`get_asset_graph` strand `OnKeyDown`**: a real authored function graph, `K2Node_FunctionEntry_0` `Function: OnKeyDown [...]` → `GetKey` → `EqualEqual_KeyKey` (`B`=`L`, `B`=`LeftAlt`) → sets `L Key Pressed` / `Alt Pressed` → `MakeStruct` Event Reply → `FunctionResult`. So it is the donor author's own key handler, not a stub.
- **Byte scan of the donor file:** `OnKeyDown` ×2, `OnKeyUp` ×1, `OnPreviewKeyDown` ×0, `OnKeyChar` ×0.
- Status of the donor: `BS_UP_TO_DATE`.

⇒ **All three instruments agree in both directions.** The handlers the donor authors (`OnKeyDown`, `OnKeyUp`) are listed by `get_asset_meta`, found by `find_graph` and present in bytes. The inherited-but-not-authored handlers (`OnPreviewKeyDown`, `OnKeyChar`, `OnFocusReceived`) are absent from all three. That second direction matters: it shows `get_asset_meta` is **not** echoing the parent class's overridable functions (otherwise it would list `OnPreviewKeyDown` here too). So an absent handler in its output is a real absence.

### 2.3 Replication on a second, independent donor

`get_asset_meta` on `/PCG/Utilities/Assemblies/ActorTagger/Widgets/Widget_Tagging.Widget_Tagging` (different plugin, different author, 17 functions): its Functions list contains, verbatim, `- OnKeyDown [Inputs(FGeometry MyGeometry = (),FKeyEvent InKeyEvent = ()), Outputs(FEventReply ReturnValue = ())]` and `- OnKeyUp [...]`, among `AssetBrowserTagAdd`, `OnMouseButtonDown`, `MultiClickTagging` and the others. (Not independently ground-truthed with `find_graph`. The Landmass donor is the control; this one only shows the listing is not specific to one asset.)

## 3. (4)(a): what it frees, by ID

**The upgrade: STRONG → AIRTIGHT** for every "no `On Key Down` override" absence that was read with `get_asset_meta` `Functions`:

1. **The BP premise block** (`TASKBOARD.md`, marker `BP-PREMISE-SETTLED-2026-09-24`, with its "THE CONTROL LIMIT" paragraph that says "STRONG, NOT AIRTIGHT … THE DIRECT CONTROL IS BOARDED AS `TASK-1443`", and `BP-CENSUS-COMPLETE-2026-09-24`). Its stated condition, "until it fires", is met.
2. **`TASK-1405` cl. 11, Leg 1** ("The inference is strong, not airtight … **Shut by:** `TASK-1443` returning (4)(a)"). It returned (4)(a).
3. **`TASK-1442`** (closed). Its (6) caveat "SURVIVES THE CLOSURE UNRELAXED: until `TASK-1443` fires, every one of these absences is 'STRONG, NOT AIRTIGHT'" can now be relaxed.
4. **`qa/TASK-1399-verify.md` §2.2 + §5.1**: the nine Blueprint assets behind the ten screens (`WBP_MainMenu`, `WBP_VictoryScreen`, `WBP_DeckCardTile`, `WBP_HUD`, `WBP_CardHand`, `WBP_SessionMenu`, `WBP_WarMap`, `WBP_DeckBuilder`, `BP_MenuGameMode`), "ZERO key-handler overrides". Its line 139-141 caveat ("no Blueprint in this project has an `On Key Down` override to serve as a direct positive control") is now answered by an out-of-project donor. **That report is another agent's landed report and I did not edit it (`SC-§101`).** The relaxation belongs on the board, which is the manager's to write.

**A second, independent leg for the same absences** (new, and it costs no editor session): the project-wide byte scan in §1 found **0 of 4,416** project `.uasset`/`.umap` files containing the FName `OnKeyDown` **or** `OnPreviewKeyDown`, and likewise 0 for `OnKeyUp`, `OnKeyChar`, `OnAnalogValueChanged`, `OnFocusReceived`. The scan is controlled three ways: on the donor file (`OnKeyDown` ×2 where the override is proven), on a project file (`Content/UI/WBP_HUD.uasset`: package magic `c1 83 2a 9e`, `Construct` ×3, `Tick` ×16, so project name tables are readable), and against Git LFS stubs (0 pointer files among the scanned project packages). ⇒ Two different instruments, a graph read and a byte read, now agree that **no Blueprint in the project overrides `On Key Down`**.

**What it does NOT free (a different caveat that only looks the same; named so nobody over-reads (a)):**
- `TASK-1398` §4 / the Aura **index** blindness: that is the index's instrument, not `get_asset_meta`. (a) proves the graph reader, not the index.
- `TASK-1419` L8 ("lane B is STRONG, NOT AIRTIGHT — no agent lane delivers a real key into Slate"), and `handoffs/TASK-1419-programmer.md:313` ("fallthrough is strong, not airtight until he presses Down"): both are about **input delivery**, not graph enumeration. Unchanged.
- C++ key handling: `NativeOnKeyDown` and Slate handlers in C++ (e.g. `WBP_DeckBuilder`'s keys, per `TASK-1398`) are outside every BP-graph instrument by construction. (a) says nothing about them.

## Not examined / limitations

- **The donor is an `EditorUtilityWidgetBlueprint`, the ten screens are `WidgetBlueprint`s.** Both are `UWidgetBlueprint` and the override is the same `UUserWidget::OnKeyDown` function graph, but no plain-`WidgetBlueprint` donor exists in the reachable content (0 project hits; all 4 mounted engine hits are EUWs). The residual risk is that `get_asset_meta` enumerates functions differently for the two asset subclasses. I rate it low (the tool's function enumeration is class-agnostic: it lists `Construction Script` for `BP_Basic_Movement` and custom functions for `Widget_Tagging` the same way), but it is **not measured**. The byte-scan leg is subclass-agnostic and closes most of it.
- **Per-name coverage:** the direct positive control exists for `OnKeyDown` (and `OnKeyUp`). **No donor was found for `OnPreviewKeyDown`, `OnKeyChar`, `OnAnalogValueChanged` or `OnFocusReceived`** (0 engine hits for `OnPreviewKeyDown`; the others were not scanned in engine content). For those four names the claim rests on mechanism (same function-graph storage, same listing) plus the negative-control agreement in §2.2, not on a per-name positive control.
- The two unmounted candidates (`ImpostorBaker`, `PerformanceCaptureWorkflow`) were not read. Two mounted candidates (`EUW_Landscape_Erosion`, `ActorTagger`) were not read, because they were not needed.
- The byte scan's scope is uncooked editor packages. It says nothing about cooked or IoStore content.
- The ten screens were **not** re-read (clause (C)).

## Routing

No QA row (zero code, zero asset). **GATE: WAIVED** (the row's declaration). **HOST:** `TASK-1540` if this lands before G derives, else `TASK-1550` (the row's `names:` line). Returns to the **manager → `TASK-1405` cl. 11 weighing** (`TASK-1544`). Files touched: this handoff, and `TASKBOARD.md` (TASK-1443's `status:` line only).
