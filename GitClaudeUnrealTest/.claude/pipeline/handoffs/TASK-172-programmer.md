# TASK-172 — Handoff (gameplay-programmer): automated same-path SM reimport

**Date:** 2026-07-16
**Author:** gameplay-programmer
**Scope:** Solve the MCP automation blocker for the M7 TRELLIS mesh swaps — automate the
same-path FBX reimport OVER `/Game/Meshes/SM_<CardID>` so it preserves the asset object
(and thus all soft + hard refs). Proven on the 4 ready units; reusable for the other 12.

## Outcome: AUTOMATED (approach 2 = headless commandlet). Manual Reimport click NO LONGER needed.

- **Approach 1 (MCP ProgrammaticToolset `unreal.*`) — CONFIRMED IMPOSSIBLE.** The toolset is a
  hard sandbox: its `execute_tool_script` can only call registered MCP tools via `execute_tool()`
  and import only `{re, json, math, datetime, copy, time}`. There is NO `import unreal`, so
  `AssetImportTask(replace_existing=True)` / `ReimportManager` are unreachable through MCP. No
  registered toolset exposes a reimport-in-place (only `StaticMeshTools.import_file`, which is the
  tool that fails "already exists"). Verified via `get_execution_environment`.
- **Approach 2 (headless `-run=pythonscript` commandlet) — WORKS.** Editor-bounce (authorized):
  MCP save-all → graceful `CloseMainWindow()` (NOT a force `/F` kill) → wait for clean exit →
  run the commandlet against the closed/unlocked project → relaunch editor → confirm MCP (port 8000).

## What the automation does (two steps)

1. **`Tools/reimport_meshes.py`** (headless commandlet). Per CardID: `unreal.AssetImportTask` with
   `replace_existing=True, automated=True` reimports `Content/RawAssets/<CardID>.fbx` OVER
   `/Game/Meshes/SM_<CardID>` **in place** (object identity preserved → refs intact), Nanite OFF,
   ≤4 convex hulls (via `EditorStaticMeshLibrary.set_convex_decomposition_collisions`), SAVE.
   Command (interactive editor MUST be closed):
   ```
   "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" ^
     "C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" ^
     -run=pythonscript ^
     -script="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/reimport_meshes.py" ^
     -unattended -nosplash -nopause -stdout -FullStdOutLogOutput
   ```
   Batch selection: edit `DEFAULT_CARD_IDS` in the script, or drop `Tools/reimport_cards.txt`
   (one CardID per line). Skips any CardID whose FBX or existing `SM_` is missing (never creates).
2. **`Tools/reimport_apply_materials_mcp.py`** (ProgrammaticToolset body, run over MCP in the
   RELAUNCHED editor). Assigns `slot[0] TeamRegion→MI_TeamColor_Blue`, `slot[1] <CardID>PBR→
   MI_<CardID>_PBR`, re-asserts Nanite-off, guarantees ≤4 hulls, saves, and reads back verification.

**Why two steps (KNOWN commandlet limitation, documented in both files):** in a headless commandlet
the post-reimport mesh BUILD resets empty material slots to `WorldGridMaterial` AFTER the script's
`static_materials` write (reordering the assignment before/after collision does NOT fix it), so the
MI-per-slot assignment does not persist from the commandlet. The geometry reimport + the two NAMED
slots + Nanite-off + hulls + save DO persist (those are the MCP-impossible parts). MI-per-slot is
finalized reliably over MCP (`StaticMeshTools.set_material`). No manual clicks in either step.

## Verification (MCP readback, all 4 units) — PASS

| Unit | tris (was blockout) | slots | slot0 | slot1 | Nanite | referencer (hard ref) | saved |
|---|---|---|---|---|---|---|---|
| Knight | 15000 (1244) | TeamRegion, KnightPBR | MI_TeamColor_Blue | MI_Knight_PBR | off | BP_Unit_Knight | yes |
| Cavalry | 15000 (804) | TeamRegion, CavalryPBR | MI_TeamColor_Blue | MI_Cavalry_PBR | off | BP_Unit_Cavalry | yes |
| Pikeman | 15000 (532) | TeamRegion, PikemanPBR | MI_TeamColor_Blue | MI_Pikeman_PBR | off | BP_Unit_Pikeman | yes |
| MilitiaMob | 15000 (452) | TeamRegion, MilitiaMobPBR | MI_TeamColor_Blue | MI_MilitiaMob_PBR | off | BP_Unit_MilitiaMob | yes |

- **Hard ref survived:** `get_referencers(/Game/Meshes/SM_<CardID>)` still returns
  `/Game/Blueprints/Units/BP_Unit_<CardID>` after the reimport — the in-place reimport preserved the
  UObject at its unchanged path. Because the object PATH never changed, the string-resolved
  placement-ghost (`/Game/Meshes/SM_<CardID>`) and cards.csv references also resolve unchanged.
- **Screenshot:** `CaptureAssetImage(/Game/Meshes/SM_Knight)` shows the detailed textured knight
  (sword + studded shield + armour) with the blue TeamRegion recolor — not the blockout.
- **dirty = false** on all 4 (saved to disk; `.uasset` are LFS-tracked — git shows them modified;
  build-master owns the commit).

## Files touched / created (all absolute)

- **NEW** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\reimport_meshes.py`
  (headless commandlet, step 1).
- **NEW** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\reimport_apply_materials_mcp.py`
  (MCP ProgrammaticToolset body, step 2).
- **MODIFIED (assets, on disk):** `Content/Meshes/SM_Knight.uasset`, `SM_Cavalry.uasset`,
  `SM_Pikeman.uasset`, `SM_MilitiaMob.uasset` — reimported + materials + collision + saved.
- **Board:** TASK-172 status updated to `in-progress` (4/8 mesh overwrite done + automated).

## Assets referenced

- `Content/RawAssets/{Knight,Cavalry,Pikeman,MilitiaMob}.fbx` (READ only — the TASK-169 U2 batch
  generating the other units in `RawAssets/` was NOT touched).
- `/Game/Materials/Instances/MI_TeamColor_Blue`, `/Game/Materials/Instances/MI_<CardID>_PBR`
  (art-director TASK-172 — pre-existing, verified present).

## What QA should scrutinize

1. **Object-preservation claim.** The core requirement was reimport-in-place (NOT delete+recreate).
   Evidence: `imported_object_paths == ['/Game/Meshes/SM_<CardID>.SM_<CardID>']` (unchanged path) and
   `get_referencers` still lists `BP_Unit_<CardID>` post-reimport. Confirm this is sufficient proof
   that BP hard refs + string soft refs are intact (I did not open each BP_Unit to eyeball the mesh
   pointer — the referencer edge in the asset registry is the proof).
2. **Two-step split.** Material assignment is a SEPARATE MCP step, not in the commandlet. For the
   remaining 12, the flow is commandlet → relaunch → MCP material batch. Confirm this is acceptable
   as "automated" (zero manual Content-Browser clicks) vs. a single-command ideal.
3. **Collision.** ≤4 hulls generated by both the commandlet and re-asserted by MCP
   `generate_convex_collisions(hull_count=4)`. There is no MCP hull-count readback; the count=4 came
   from the commandlet's `body_setup.agg_geom.convex_elems` readback + the idempotent MCP regen.
   Units get their real in-game look from the later `SK_` rig batch, so these hulls mainly matter for
   ghost previews; buildings (TASK-173) will need the tighter UCX authoring per that task.
4. **Editor bounce.** Graceful `CloseMainWindow()` after a save-all (no `/F` force kill, no data loss);
   editor relaunched; MCP confirmed back on port 8000 (all post-relaunch MCP calls succeeded).

## Remaining 12 units/buildings — automatable, no manual clicks

- **U2 units** (Sapper, Cleric, Longbowman, Miner): await TASK-169 FBX generation, then run the same
  two-step automation.
- **Buildings** (TASK-173): the reimport step is identical; NOTE buildings need explicit
  `UCX_SM_<CardID>` collision (not auto convex hulls) per TASK-173 — `reimport_meshes.py` would need
  a per-batch collision toggle (or skip collision and let the artist author UCX). Flagged, not built.
