# TASK-728 (IMPORT HALF) — `SM_WatchTower` + textures + `MI_WatchTower_PBR` land in `/Game/` — art-director handoff

**Status: the Stage-3 IMPORT half of TASK-728 is DONE and MEASURED. The BLUEPRINT half is NOT
done and was NOT attempted — see §6. ⛔ The board task is NOT `ready-for-integration`.**

Executed against `handoffs/TASK-727-artist.md` §7's recipe verbatim, ⛔ not re-derived.

---

## 1. ⛔⭐ THE STOP GATE — PASSED, MEASURED, NOT ASSUMED

> **Required: `convex_hull_count == 14` AND `box_count == 0`.**

```
convex_hull_count        14      <- REQUIRED 14   ✅
box_count                 0      <- REQUIRED  0   ✅
sphere_count              0
sphyl_count               0
levelSet_count            0
any_engine_generated   false     <- ⭐ THE DECISIVE FIELD
generated_true_count      0
vert_counts            [6, 8,8,8,8,8,8,8,8,8,8,8,8,8]
```

⭐ **`bIsGenerated == false` on all 14 hulls is the load-bearing result, not the count.** UE sets that
flag to `true` for any shape *"created by the engine and was not imported."* **Zero hulls carry it**
⇒ **no solver ran**; all 14 are the hand-authored UCX hulls read out of the FBX. A convex
decomposition would have produced generated hulls in an arbitrary count — the failure mode TASK-727
§3 and the dispatch both warned about **did not occur**, and this is measured rather than inferred
from the count alone.

The `[6, 8×13]` vertex signature independently corroborates it: TASK-727 §2's manifest specifies hull
`_00` as a **6-vertex triangular prism** and all thirteen others as 8-vertex boxes. That exact
signature survived. A solver would not reproduce it.

### 1a. The two load-bearing hulls, re-measured IN-ENGINE from the imported hull vertices

| Property | TASK-727 authored | **IN-ENGINE after import** | Verdict |
|---|---|---|---|
| Ramp slope (hull `_00` top face) | 30.0000° | **29.999999602267813°** | ✅ 4.0e-7° deviation |
| Ramp run | 2,078.4610 uu | **2,078.4609375 uu** | ✅ |
| Ramp rise | 1,200 uu | **1,199.9999625282435 uu** | ✅ |
| Cap (`_06`) deck z | 1,200 uu | **1,200.0001220703125 uu** | ✅ |
| Cap terminates at x | 380 (= ramp start) | **380** exactly | ✅ flush by construction |
| **Junction lip ramp→platform** | 0.0001 uu | **0.0001220703125 uu** (budget 40) | ✅ 327,000× inside budget |

⭐ **The ramp plane is intact: 29.9999996° is 2.005° inside the 32.005° Recast ledge-filter ceiling**,
so the surface the whole task exists to protect is walkable. The residual is float32 storage noise
(the authored worst case was 0.0005 uu), ⛔ **not a solver tolerance** — a fitted hull would deviate
by orders of magnitude more, and `bIsGenerated` would have said `true`.

⚠️ **Method note, stated so it is not silently trusted:** my first cap-identification pass selected
the hull with `elemBox.max.z` nearest 1200 and **re-picked the ramp** (both top out at ~1200),
reporting a meaningless `lip = 0` measured against itself. Corrected by selecting the 8-vertex hull
that **terminates at x = 380**; the 0.000122 uu figure above is the corrected measurement. The wrong
number never left this file.

---

## 2. Assets created — exact `/Game/` paths

⭐ All five were **absent before this pass** (verified on disk first), so every one is a **CREATE**;
the never-delete-and-recreate law had nothing to protect and ⛔ nothing was deleted.

| Asset | Path | Source |
|---|---|---|
| Static mesh | **`/Game/Meshes/SM_WatchTower`** | `Content/RawAssets/WatchTower.fbx` |
| Base color | **`/Game/Textures/T_WatchTower_D`** | `Content/RawAssets/Textures/WatchTower/T_WatchTower_D.png` |
| Normal | **`/Game/Textures/T_WatchTower_N`** | `…/T_WatchTower_N.png` |
| ORM | **`/Game/Textures/T_WatchTower_ORM`** | `…/T_WatchTower_ORM.png` |
| Material instance | **`/Game/Materials/Instances/MI_WatchTower_PBR`** | parent `/Game/Materials/M_AssetPBR` |

All five saved explicitly by path. ⛔ **`L_Arena` was never opened, touched or saved** — the save call
passed an explicit five-asset list, ⛔ never the empty-list "save all dirty" form that would have
swept the level in.

---

## 3. ⚠️⭐ TEXTURE SETTINGS — the ORM imported WRONG and was corrected

⛔ **The importer did NOT honour the ORM's linear intent.** Read back immediately after import:

```
T_WatchTower_ORM  AS IMPORTED:  SRGB=true,  TC_Default,  TEXTUREGROUP_World   <- ⛔ WRONG
T_WatchTower_ORM  AS SET NOW:   SRGB=false, TC_Masks,    TEXTUREGROUP_World   <- ✅ CORRECT
```

⭐ **This is exactly why the dispatch said to read back rather than assume.** An sRGB-decoded ORM
silently skews AO/roughness on every surface — it looks plausible in the viewport and is wrong in the
lighting. Corrected explicitly and re-read to confirm the write landed.

The target values are ⛔ not invented — they were **copied from the shipped house pattern**
(`T_ArrowTower_{D,N,ORM}`), which all three now match byte-for-byte:

| Texture | SRGB | CompressionSettings | LODGroup | vs house pattern |
|---|---|---|---|---|
| `T_WatchTower_D` | **true** | `TC_Default` | `TEXTUREGROUP_World` | ✅ as imported, no change needed |
| `T_WatchTower_N` | **false** | `TC_Normalmap` | `TEXTUREGROUP_WorldNormalMap` | ✅ auto-detected by `_N` suffix |
| `T_WatchTower_ORM` | **false** | `TC_Masks` | `TEXTUREGROUP_World` | ✅ **after explicit correction** |

---

## 4. Mesh properties — every TASK-727 §5 number confirmed in-engine

| Property | §5 expected | **IN-ENGINE** | Verdict |
|---|---|---|---|
| Material slots (order) | `[TeamRegion, WatchTowerPBR]` | **`["TeamRegion","WatchTowerPBR"]`** | ✅ exact |
| Slot 0 material | `MI_TeamColor_Blue` | `/Game/Materials/Instances/MI_TeamColor_Blue` | ✅ |
| Slot 1 material | `MI_WatchTower_PBR` | `/Game/Materials/Instances/MI_WatchTower_PBR` | ✅ |
| Triangles | 464 | **464** | ✅ |
| Nanite | OFF | **false** | ✅ (import default; ⛔ no write needed) |
| LOD count | 1 | **1** | ✅ see §5 |
| Bounds | X[-600, 2458.461] Y[±470] Z[0, 1420] | **min (-600, -470.0002, -7.66e-05) · max (2458.4609, 470.0000, 1420)** | ✅ |

`MI_WatchTower_PBR` parameter bindings, read back individually (parent exposes exactly
`BaseColor` / `ORM` / `Normal`):

```
BaseColor -> /Game/Textures/T_WatchTower_D     ✅
Normal    -> /Game/Textures/T_WatchTower_N     ✅
ORM       -> /Game/Textures/T_WatchTower_ORM   ✅
```

**Vertex count reads 930, not §5's 310 — this is NOT a deviation.** 310 is the Blender vertex count;
UE splits vertices at UV and hard-normal seams, and with the FACE smoothing the FBX was exported
under, 464 tris × 3 = 1,392 corners deduplicating to 930 is the expected result.

---

## 5. The two EXPECTED non-failures, and what actually happened

- **`LOD_STEP_FAILED`** — ⛔ **did not occur, because no LOD step was run.** `TL-§3` / §5 make the LOD
  chain pointless at 464 tris, and §5's *"the readback already matches, so nothing was written"* is a
  complete result. `get_lod_count` returns **1** (LOD 0 only), which is the wanted end state. ⛔ I did
  not call `generate_lods` merely to produce the expected token.
- **`WARN: no manifest boxes for WatchTower`** — ⛔ **did not occur, because `reimport_meshes.py` was
  not run.** That WARN is emitted by `_apply_box_collision()` at `reimport_meshes.py:305-307`. This
  was a first-time CREATE via the MCP importer, so the bulk-reimport lane was never entered
  (⛔ dispatch fence: no bulk reimport sweep this pass). **The trap TASK-727 §3 closed remains closed
  and remains necessary** for any future sweep — ⛔ nobody should "fix" `ucx.boxes: []`.

---

## 6. ⛔⭐ WHAT WAS DELIBERATELY NOT DONE — and why the board must NOT flip to ready-for-integration

- ⛔ **`BP_Building_WatchTower` NOT created.** It must derive from **`AClimbableTower`**, which is
  TASK-726's new C++ that **has not compiled** — the editor does not know the class exists. ⭐ **A
  Blueprint parented to a missing class is the silent-corruption class this project has already been
  bitten by** (see the duplicate+reparent widget precedent). Creating it "and making it work" would
  be worse than waiting. **This is a separate pass after the compile lands.**
- ⛔ **`T_CardArt_WatchTower` NOT created** (board spec item 3). No `Content/RawAssets/CardArt/WatchTower.png`
  exists, and this pass's dispatch scoped the deliverable to the five assets in §2. The card-art lane
  is untouched — ⛔ nothing was written to `Content/RawAssets/CardArt/` or `/Game/UI/CardArt/`.
- ⛔ **`DT_Cards` NOT reimported.** ⛔ No C++, ⛔ no compile, ⛔ no Git, ⛔ nothing in `Tools/Packaging/`,
  ⛔ TASK-716/700 untouched, ⛔ no Fab browsing.
- ⛔ **The editor was NOT closed, restarted or otherwise disturbed** — Jonathan is at the machine and
  the editor is his. ⛔ No forced save, ⛔ no forced quit, ⛔ no `L_Arena` save.

⇒ **TASK-728 remains open.** Board status set to `in-progress`, ⛔ **not `ready-for-integration`** —
TASK-731 is `blocked-by` *"TASK-728 (the art chain complete)"*, and flipping this task would falsely
signal to build-master that the Blueprint and card art exist.

---

## 7. Integration notes (unchanged from TASK-727 §5, re-confirmed against the imported asset)

- ⭐ **PIVOT: origin is the centre of the tower shaft at ground level**, ⛔ not the bbox centre.
  `min_z ≈ 0` (measured −7.66e-05). **The ramp runs along local +X** — ⭐ **this answers TASK-726's one
  unverified input: `AscentGateHalfExtentXY`'s assumption that the ramp runs along local X is
  CORRECT, no axis swap needed** (ramp hull spans x 380 → 2458.46 at constant y ±230).
- **Scale 1.0.** Bounds **3,058.461 × 940 × 1,420 uu** — ⚠️ ~12× a wall in X. The 200 uu
  building-clearance check and the placement ghost use the whole bounds, so placement will feel very
  different from other buildings; that is geometry, ⛔ not a bug.
- **Slot 0 `TeamRegion`** takes `MI_TeamColor_<Team>` from `Building.cpp:80` unchanged at runtime;
  `MI_TeamColor_Blue` is design-time only.

---

## 8. ⚠️ One declared method departure, for the next gate to scrutinise

**I imported via Unreal MCP `StaticMeshTools.import_file`, ⛔ not via `reimport_meshes.py`'s
`FbxImportUI` lane.** MCP's importer ⛔ **does not expose `auto_generate_collision`** (nor
`generate_lightmap_u_vs` / vertex-colour options) — `TASK-566-buildmaster.md:439` flags exactly this,
and it is why the commandlet lane exists.

**Why this is safe here, measured rather than argued:**
- `auto_generate_collision`'s only effect is to synthesise collision **when none was imported**. The
  readback proves collision **was** imported: 14 hulls, **`bIsGenerated == false` on every one**, and
  `box_count == 0`. Had auto-generation fired, it would have produced engine-generated shapes; none
  exist. ⇒ **the collision outcome is identical to what the commandlet lane would have produced.**
- Lightmap settings land on the house pattern anyway — `LightMapCoordinateIndex = 1`,
  `LightMapResolution = 64`, **identical to `SM_ArrowTower` and `SM_BallistaTower`** (checked).
- ⭐ The commandlet lane exists chiefly to **overwrite an existing** `SM_<CardID>` in place
  (`reimport_meshes.py:6-9` — MCP `import_file` cannot). **This was a first-time CREATE**, so that
  constraint did not apply and no references existed to preserve.

⚠️ **This does mean a future `reimport_meshes.py` sweep is still the lane for any re-import of this
mesh** — and TASK-727 §3's manifest entry is what keeps that sweep from destroying the 14 hulls.

---

## 9. What the next gate should scrutinise

1. **§1's `any_engine_generated == false` is the deliverable**, more than the count of 14. If a later
   pass reports 14 hulls but `bIsGenerated == true`, a solver ran and the ramp is decoration.
2. **§3's ORM correction was a real defect caught by readback** — any future texture import in this
   project should read back sRGB, not trust the importer.
3. **§6: the Blueprint is owed** and is blocked on TASK-726's compile, ⛔ not on art.
4. **§8's importer departure** — confirm the ruling that the measured-identical collision outcome
   makes the MCP lane acceptable for a CREATE, or direct a re-import through the commandlet.
