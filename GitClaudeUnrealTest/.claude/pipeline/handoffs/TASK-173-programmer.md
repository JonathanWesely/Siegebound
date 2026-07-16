# TASK-172/173 — Handoff (gameplay-programmer): M7 building + GoldNode + U2-unit reimport wave

**Date:** 2026-07-16
**Author:** gameplay-programmer
**Scope:** Apply the proven TASK-172 same-path SM reimport automation to the remaining M7 wave —
4 U2 units + 5 buildings + GoldNode = **10 meshes reimported this pass**, plus verify the 4 U1
units still good (14 total covered). Extends `Tools/reimport_meshes.py` with a per-CardID collision
mode (unit convex hulls vs. building/GoldNode authored box hulls) and the GoldNode single-slot
emissive variant.

## Outcome: 14/14 CLEAN. All reimported meshes are the refined textured versions at their unchanged paths.

| Group | CardIDs | Result |
|---|---|---|
| U1 units (verify-only) | Knight, Cavalry, Pikeman, MilitiaMob | unchanged, verified 15000 tris + correct slots/MIs |
| U2 units (reimported) | Sapper, Cleric, Longbowman, Miner | 15000 tris (↑ from 452–1984), 2-slot PBR, ≤4 hulls |
| Buildings (reimported) | ArrowTower, BallistaTower, Barracks, BombTower, CrystalTower | 20000 tris (↑ from 312–1212), 2-slot PBR, 1 box hull |
| GoldNode (reimported) | GoldNode | 12000 tris (↑ from 222), single `GoldNodePBR`→M_GoldGlow, 1 box hull |

Not in this wave: **Wall, DeepMine** (TASK-173's other 2 buildings) — NOT refine-ready (no baked
D/N/ORM textures on disk). Reimport them via the same automation once their textured FBX land.

## The automation (two steps, editor-bounce authorized)

1. **`Tools/reimport_meshes.py`** (headless `-run=pythonscript` commandlet, EXTENDED). Batch from
   `Tools/reimport_cards.txt` sidecar; category + collision + box hulls read from
   `Tools/ArtPipeline/pipeline_manifest.json`. Per CardID:
   - **Textures + MI** (units + non-GoldNode buildings, idempotent — skipped if the MI exists):
     import `Content/RawAssets/Textures/<CardID>/T_<CardID>_{D,N,ORM}.png` →
     `/Game/Textures/T_<CardID>_{D,N,ORM}` with correct settings (**D** sRGB on / **N** `TC_Normalmap`
     sRGB off / **ORM** `TC_Masks` sRGB off — the linear ORM law), then create
     `/Game/Materials/Instances/MI_<CardID>_PBR` from `/Game/Materials/M_AssetPBR` with `BaseColor`/
     `Normal`/`ORM` wired (`MaterialEditingLibrary`). *Imagery is PRE-BAKED by the art pipeline — this
     step only imports+wires already-baked PNGs; it authors no art.*
   - **In-place reimport** of `Content/RawAssets/<CardID>.fbx` OVER `/Game/Meshes/SM_<CardID>`
     (`AssetImportTask(replace_existing=True, automated=True)` → object identity + refs preserved).
   - **Nanite OFF.**
   - **Collision by category (the TASK-173 extension):**
     - `unit` → ≤4 convex decomposition hulls (unchanged TASK-037 recipe).
     - `building` / `goldnode` → explicit **box hull(s)** authored from `manifest[cid].ucx.boxes`
       (`FKBoxElem` on `body_setup.agg_geom`), wall-footprint-exact, ground-center space. NO convex
       decomposition on buildings (a single auto-hull on a hollow tower re-creates the M1 plinth
       interior placement dead-zone). 1 box each this wave.
   - Save.
2. **`Tools/reimport_finalize_materials_mcp.py`** (ProgrammaticToolset body, run over MCP in the
   RELAUNCHED editor — inlined into the finalize call this run). Assigns the mesh slot→material
   POINTERS (the commandlet's post-build reset defeats these headless), re-asserts Nanite-off,
   re-asserts ≤4 hulls on UNITS only (buildings/GoldNode collision left as the commandlet's boxes),
   saves, and reads back full verification. Slot assignment is category-aware:
   - 2-slot unit/building: slot0→`MI_TeamColor_Blue`, slot1→`MI_<CardID>_PBR`.
   - GoldNode: every slot→`M_GoldGlow` (NO TeamColor).
   - CrystalTower: slot0→`MI_TeamColor_Blue`, slot1→`MI_CrystalTower_PBR`, slot2→`M_CrystalGlow` *if*
     a 3rd slot exists (it did not — see flag).

**Sequence:** MCP save-all → graceful `CloseMainWindow()` (NOT a force kill) → wait clean exit →
headless commandlet on the unlocked project → relaunch interactive editor → confirm MCP (port 8000)
→ MCP material finalize + verify → asset-thumbnail screenshots.

## Verification (MCP readback, all 14) — PASS

- **Tri count ↑ from blockout** for every reimported mesh (units→15000, buildings→20000,
  GoldNode→12000; the refine bake caps). Proof they are the refined textured meshes, not blockouts.
- **Slots / materials** exactly per contract (table above); GoldNode is the single-slot emissive
  variant `GoldNodePBR`→`M_GoldGlow` (no TeamRegion).
- **Nanite OFF** on all 14.
- **Refs intact** (object path never changed): units ← `BP_Unit_<CardID>`, buildings ←
  `BP_Building_<CardID>`, GoldNode ← `L_Arena` (level placement). Ghost/`cards.csv` string refs
  resolve unchanged.
- **Collision:** units ≤4 convex hulls; buildings + GoldNode 1 authored box hull each (commandlet
  `body_setup.agg_geom` readback: `boxes=1`). No MCP box-count readback exists — count is from the
  commandlet's own post-save readback (QA scrutiny item 3).
- **Screenshots** (`Saved/Screenshots/M7_ReimportWave/SM_{ArrowTower,Sapper,GoldNode,CrystalTower}.png`):
  textured stone tower with the blue TeamRegion roof; textured Sapper (bomb + blue shoulder accents);
  warm-yellow glowing GoldNode; CrystalTower with blue PBR crystal (NOT glowing — see flag).
- **dirty=false** (saved). `.uasset`/`T_`/`MI_` are LFS-tracked; **build-master owns the commit.**

## FLAG (non-blocking) — CrystalTower emissive not preserved

The CrystalTower blockout carried a dedicated `M_CrystalGlow` emissive slot, but the **refined FBX
authored only 2 slots** `[TeamRegion, CrystalTowerPBR]` (refine collapsed to the standard building
contract). `M_AssetPBR` has **no emissive parameter** and there is **no `T_CrystalTower_E`** baked, so
the crystal glow cannot be wired into `MI_CrystalTower_PBR`. Per the time-box rule the **textured mesh
shipped** (crystal reads blue via PBR albedo) and the glow is flagged for an **art-director emissive
pass**: bake `T_CrystalTower_E` + an emissive-capable master, OR author a dedicated `M_CrystalGlow`
slot into the FBX. This matches `pipeline_manifest.json`'s own `_emissive_note` ("flagged, NOT
resolved"). Screenshot `SM_CrystalTower.png` shows the non-glowing crystal for confirmation.

## Scope note (normally art-director)

The texture-import + MI-creation step is art-director's lane in TASK-172; the orchestrator explicitly
delegated it here ("import their T_*/MI_*_PBR first if not present, same as TASK-172's texture+MI
step"). It is mechanical (fixed D/N/ORM→`BaseColor`/`Normal`/`ORM` wiring, no visual-style choice; the
imagery is pre-baked). GoldNode deliberately gets **no** PBR MI/textures — it keeps `M_GoldGlow` per
the spec (single-slot emissive, no TeamColor), so `T_GoldNode_*` PNGs exist on disk but are NOT
imported (would be orphan/unused and would drop the glow).

## Files touched / created (all absolute)

- **MODIFIED** `C:\GitProjects\...\Tools\reimport_meshes.py` — extended: texture+MI step, per-CardID
  collision mode (unit hulls vs. manifest box hulls), GoldNode single-slot variant, manifest-driven
  categorization. Also fixed a `MaterialInstanceConstantFactoryNew.initial_parent` non-reflected-
  property bug (parent now set via `MaterialEditingLibrary.set_material_instance_parent`).
- **NEW** `C:\GitProjects\...\Tools\reimport_finalize_materials_mcp.py` — superset finalize (2-slot /
  GoldNode single-slot / CrystalTower emissive), replaces the TASK-172 2-slot-only finalize for mixed
  batches.
- **NEW** `C:\GitProjects\...\Tools\reimport_cards.txt` — sidecar batch list (the 10 this wave).
- **MODIFIED (assets, on disk — build-master commits):** `Content/Meshes/SM_{Sapper,Cleric,Longbowman,
  Miner,ArrowTower,BallistaTower,Barracks,BombTower,CrystalTower,GoldNode}.uasset`;
  `Content/Textures/T_{Sapper,Cleric,Longbowman,Miner,ArrowTower,BallistaTower,Barracks,BombTower,
  CrystalTower}_{D,N,ORM}.uasset` (27 textures); `Content/Materials/Instances/MI_{…9…}_PBR.uasset`.
- **NEW (evidence):** `Saved/Screenshots/M7_ReimportWave/SM_{ArrowTower,Sapper,GoldNode,CrystalTower}.png`.

## What QA should scrutinize (code review of the two scripts)

1. **Object-preservation** (same as TASK-172): reimport-in-place, not delete+recreate. Evidence:
   `imported_object_paths == ['/Game/Meshes/SM_<CardID>.SM_<CardID>']` + `get_referencers` unchanged.
2. **Building box-collision authoring** (`_apply_box_collision`): `FKBoxElem` on
   `body_setup.agg_geom.box_elems`, cleared convex/sphere/sphyl first, `CTF_USE_DEFAULT`. Boxes come
   from `manifest[cid].ucx.boxes` (center+full-size, ground-center). Confirm this is the right
   footprint collision for placement/navmesh (solid full-height footprint box per building — the
   manifest's own "acceptable start"; buildings are route-around obstacles). No MCP box readback, so
   the count=1 comes from the commandlet's `agg_geom` readback.
3. **Texture sRGB/compression** correctness: D sRGB-on/`TC_Default`, N `TC_Normalmap` sRGB-off, ORM
   `TC_Masks` sRGB-off (the linear-ORM law). Set post-import via `set_editor_property`.
4. **GoldNode single-slot handling** — kept `M_GoldGlow` on every slot, no TeamColor; refined FBX gave
   exactly 1 slot `GoldNodePBR` (so no fallback needed). Confirm this honors the emissive-variant law.
5. **Two-step split** unchanged from TASK-172 (commandlet does MCP-impossible reimport + assets;
   MCP finalizes the flaky mesh slot→material pointers). Zero manual Content-Browser clicks.
6. **Editor bounce** — graceful `CloseMainWindow()` after save-all (no data loss); relaunch; MCP
   reconfirmed on port 8000 (all post-relaunch MCP calls succeeded).

## Not done (out of scope / handed off)

- **Wall, DeepMine** — reimport pending their refine-ready textured FBX + baked textures (art-director).
- **CrystalTower emissive** — flagged for an art-director emissive pass (above).
- **Git** — build-master commits the `SM_`/`T_`/`MI_` assets (I never touch Git).
- **Team-region tuning** on any reimported mesh is the art-director eyeball-gate call, not this task.
