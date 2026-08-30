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

---
---

# TASK-728 (COMPLETION PASS) — the BLUEPRINT verified + `T_CardArt_WatchTower` imported

**Status: TASK-728 is COMPLETE. Both owed assets exist AND read back correct ⇒ board flips to
`ready-for-integration`.**

⚠️ **Context: the previous agent died mid-task on a server error.** Its blueprint half was on disk
but **had never been read back or reported**. Everything below was **re-measured from the live
editor**, ⛔ nothing was assumed correct because a file existed. ⭐ **The pre-existing blueprint was
found CORRECT on every gate and was therefore NOT re-created** — ⛔ no delete, ⛔ no rebuild, ⛔ no
overwrite. Editor PID 5584 left running and undisturbed throughout.

---

## A. ⭐⭐ THE BLUEPRINT AS FOUND — verified BEFORE any change (nothing needed changing)

| Gate | Required | **READ BACK FROM THE LIVE EDITOR** | Verdict |
|---|---|---|---|
| Parent class | `AClimbableTower` | **`/Script/GitClaudeUnrealTest.ClimbableTower`** | ✅ ⛔ not `ABuilding`, ⛔ not `ATower` |
| `PlatformHeightUU` | present, default `1200` | **`1200`** | ✅ the parent binding is REAL, ⛔ not nominal |
| Static mesh | `/Game/Meshes/SM_WatchTower` | **`/Game/Meshes/SM_WatchTower.SM_WatchTower`** | ✅ |
| `CardID` | `WatchTower` | **`"WatchTower"`** | ✅ |
| BP-added variables | none | **`[]`** (empty) | ✅ ⛔ no re-authored state |
| Graph logic | none | 3 empty stub events, ⛔ zero statements | ✅ see A.2 |

⭐ **The `PlatformHeightUU` check is the load-bearing one and it passed for the right reason.** The
CDO does not merely carry the number — it exposes the whole `AClimbableTower` member set
(`platformHeightUU`, `ascentGateVolume`, `ascentGateFloorUU` = **300**, `ascentGateHeadroomUU` =
**400**, `ascentGateHalfExtentXY` = **(1500, 500)**), each with TASK-726's authored doc comment
attached. **A silent fallback to `ABuilding` would have produced none of these** — that is the
failure this check exists to catch, and it did not occur.

### A.1 ⛔⛔ THE PATH STRING — compared CHARACTER FOR CHARACTER against the shipped resolvers

Both call sites compose the path identically (`SiegePlayerController.cpp:3842`,
`SiegeBotController.cpp:1570`):

```cpp
FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C"), *CardName, *CardName)
```

`CardName = CardID.ToString()`; `cards.csv:32`'s row name is **`WatchTower`** ⇒ the runtime asks for:

```
/Game/Blueprints/Buildings/BP_Building_WatchTower.BP_Building_WatchTower_C
```

⭐ **Proven by resolution, ⛔ not by eyeballing.** `ObjectTools.search_subclasses(base = ABuilding,
filter = "WatchTower")` returned **exactly one** result:

```
/Game/Blueprints/Buildings/BP_Building_WatchTower.BP_Building_WatchTower_C     <- byte-identical ✅
```

⭐ **That single call also discharges the OTHER half of both call sites' check** — they each gate on
`ActorClass->IsChildOf(RequiredBase)` with `RequiredBase = ABuilding::StaticClass()` for a building
card. The class was **found by a search rooted at `ABuilding`**, so `IsChildOf(ABuilding)` is proven
true by construction, ⛔ not asserted. `cards.csv:32` types the row **`Building`** ⇒ `IsBuildingCard`
routes to the buildings branch. **The card will resolve and spawn.**

### A.2 ⚠️ The three "implemented" events — investigated, and they are the HOUSE SHAPE

`list_events` reports `ReceiveBeginPlay`, `ReceiveTick` and `ReceiveActorBeginOverlap` as
`bIsImplemented: true`, which looks exactly like the re-authored gameplay the dispatch warned about.
⛔ **It is not.** The graph reads:

```
(event EventBeginPlay)
(event Collision|EventActorBeginOverlap (OtherActor))
(event EventTick (DeltaSeconds))
```

— three **bare event nodes with zero connected statements**. ⭐ **I did not judge this from the
WatchTower asset alone; I read the two shipped siblings and got a byte-identical string from each:**
`BP_Building_ArrowTower:EventGraph` and `BP_Building_Barracks:EventGraph` both return **exactly the
same three lines.** These are UE's default placeholder nodes present in every Actor Blueprint in this
project. ⇒ **⛔ Nothing was added and ⛔ nothing was removed.** `UserConstructionScript` is likewise
empty (`(fn ConstructionScript ())`).

⇒ **The C++ still owns team gating, the no-fire property and the platform height, exactly as the
fence requires.**

### A.3 Wiring shape vs the shipped siblings — matched, ⛔ not invented

| | **BP_Building_WatchTower** | `BP_Building_ArrowTower` (shipped) |
|---|---|---|
| `VisualMesh.staticMesh` | `SM_WatchTower` | `SM_ArrowTower` |
| `overrideMaterials` | **`[MI_TeamColor_Blue]`** (slot 0 only) | **`[MI_TeamColor_Blue]`** (slot 0 only) |
| relative location / scale | `(0,0,0)` / `(1,1,1)` | `(0,0,0)` / `(1,1,1)` |
| mobility | `Movable` | `Movable` |
| root component | `VisualMesh` | `VisualMesh` |
| BP variables | `[]` | `[]` |
| **dependencies** | `[/Script/GitClaudeUnrealTest, MI_TeamColor_Blue, SM_WatchTower]` | `[/Script/GitClaudeUnrealTest, MI_TeamColor_Blue, SM_ArrowTower]` |

⭐ **The dependency sets are structurally identical — same length, same shape, only the mesh differs.
There is no stray reference and no extra component.** Slot 1 is deliberately NOT overridden, so it
falls through to the mesh asset's own material: `SM_WatchTower` slots read back
**`["TeamRegion", "WatchTowerPBR"]`** with `WatchTowerPBR` → **`MI_WatchTower_PBR`**. That is the
ArrowTower pattern exactly, and it is what lets `Building.cpp:80` swap slot 0 to
`MI_TeamColor_<Team>` at runtime.

**`is_dirty` on the blueprint = `false`** ⇒ the in-memory object I measured **is** the 37,876-byte
`.uasset` on disk. ⛔ No save of the blueprint was needed or performed.

---

## B. ⭐ THE STOP GATE RE-DERIVED FROM RAW GEOMETRY — ⛔ not trusted from the import-half section

The dispatch required the ramp collision to be verified, so I re-read `SM_WatchTower`'s
`BodySetup_0.aggGeom` directly and counted the elements myself:

```
convexElems              14      <- REQUIRED 14   ✅
boxElems                  0      <- REQUIRED  0   ✅
sphereElems / sphylElems / taperedCapsuleElems / levelSetElems / skinnedLevelSetElems / mLLevelSetElems / skinnedTriangleMeshElems   ALL 0   ✅
bIsGenerated             false on ALL 14          <- ⭐ THE DECISIVE FIELD
vertex signature         [6, 8×13]                <- TASK-727 §2's manifest, intact
```

⭐ **This independently reproduces §1's result from a fresh measurement.** Hull `_00` is the ramp
prism, and its six vertices give the slope directly:

```
(380, ±230, ~0) · (380, ±230, 1200) · (2458.4609375, ±230, ~0)
run  = 2458.4609375 − 380 = 2078.4609375 uu
rise = 1200 uu   ⇒   atan(1200 / 2078.4609375) = 30.0000000° (to ~1e-7°)
```

✅ **2.005° inside the 32.005° Recast ledge-filter ceiling — the walkable surface survived import.**
✅ **The ramp runs along local +X** (constant y ±230), re-confirming TASK-726's one unverified input:
**`AscentGateHalfExtentXY` needs no axis swap.**

### B.1 ⚠️ NEW MEASURED FINDING for TASK-731 — the gate does not span the ramp's outer half

⛔ **Not a defect I may fix** (`AscentGateHalfExtentXY` is C++/`EditAnywhere` gameplay tuning, and
this pass is fenced from gameplay) — **raised with numbers so integration can rule on it.**

`ClimbableTower.cpp:106-107` centres the gate box on the actor origin:
`SetRelativeLocation(FVector(0, 0, GateCentreZ))` with `SetBoxExtent(X = 1500, …)` ⇒ the gate spans
**x ∈ [−1500, +1500]**. But the mesh is **not centred on its pivot** — the pivot is the tower shaft,
and the structure spans **x ∈ [−600, +2458.46]**. ⇒ **the ramp's outer 958 uu (x 1500 → 2458.46)
lies OUTSIDE the gate.**

```
enemy enters the ramp at its low end (x ≈ 2458, z ≈ 0) — outside the gate, unobstructed
first blocked at x = 1500, where the ramp deck sits at
    (2458.4609 − 1500) × tan(30°) = 553.4 uu
```

⇒ an enemy climbs to **≈553 uu (46% of the 1,200-uu platform) before being turned back**, where
TASK-726's header predicts ~300 uu. ⭐ **T-3 still HOLDS — no enemy reaches the platform** (the gate
fully covers the platform, x ∈ [−560, +380]), and it still **fails OPEN, ⛔ never into a stuck
unit**: the enemy is blocked while standing on walkable navmesh and can walk back down.
⚠️ **The one consequence worth a ruling:** an enemy parked on the ramp's outer half gains ~553 uu of
real height ⇒ a `HIGH-§` damage bonus of roughly **×1.36**. Whether that is acceptable flavour or
wants an extent/offset correction is **TASK-731's call, ⛔ not mine.**

---

## C. `T_CardArt_WatchTower` — imported, corrected, verified

**The CSV is the contract and I read it rather than trusting the dispatch.** `Docs/Data/cards.csv:32`
(`CardArt` column) holds:

```
/Game/UI/CardArt/T_CardArt_WatchTower.T_CardArt_WatchTower
```

The imported asset's refPath is **byte-identical** to that cell. ✅

### C.1 ⚠️⭐ A SECOND IMPORTER DEFECT CAUGHT BY READ-BACK — same class as §3's ORM

⛔ **The importer did not honour the UI intent.** Read back immediately after import:

```
T_CardArt_WatchTower  AS IMPORTED:  LODGroup = TEXTUREGROUP_World   <- ⛔ WRONG
T_CardArt_WatchTower  AS SET NOW:   LODGroup = TEXTUREGROUP_UI      <- ✅ CORRECT
```

⭐ **The import call returned success both times.** This is the second independent confirmation of
§9.2's lesson: in this project a texture import's `LODGroup`/`SRGB` **must** be read back, never
assumed. Corrected explicitly and re-read to confirm the write landed.

### C.2 Settings vs the shipped sibling — ⛔ matched, not assumed

Target values were **copied from `T_CardArt_ArrowTower`** and cross-checked against
`T_CardArt_Barracks` (both agree). All nine properties now match:

| Property | `T_CardArt_ArrowTower` | **`T_CardArt_WatchTower`** |
|---|---|---|
| Dimensions | 512 × 512 | **512 × 512** ✅ |
| `SRGB` | `true` | **`true`** ✅ (as imported) |
| `CompressionSettings` | `TC_Default` | **`TC_Default`** ✅ |
| `LODGroup` | `TEXTUREGROUP_UI` | **`TEXTUREGROUP_UI`** ✅ **after correction** |
| `MipGenSettings` | `TMGS_FromTextureGroup` | **`TMGS_FromTextureGroup`** ✅ |
| `NeverStream` / `Filter` | `false` / `TF_Default` | **`false` / `TF_Default`** ✅ |
| `AddressX` / `AddressY` | `TA_Wrap` / `TA_Wrap` | **`TA_Wrap` / `TA_Wrap`** ✅ |

Source PNGs also match in format: **512×512, 8-bit, colour-type 2 (RGB, no alpha)** — identical to
`ArrowTower.png` and `Barracks.png`.

Saved by **explicit single-path list**, ⛔ never the empty-list "save all dirty" form that would have
swept in the level. `is_dirty` after save = **`false`**; the `.uasset` is on disk (320,159 B).

### C.3 ⚠️ Expected: the texture has ZERO referencers, and that is CORRECT

`get_referencers(T_CardArt_WatchTower)` = **`[]`**, while the sibling returns **`[/Game/Data/DT_Cards]`**.
⛔ **Not a defect** — `DT_Cards` has not yet been reimported from `cards.csv`, and that reimport is
**build-master's next step and is explicitly fenced out of this pass.** The texture path is correct
and will bind the moment the row lands. ⭐ **Until then the card face falls back to text-only** — so
if the card art looks missing at playtest, **the cause is the pending `DT_Cards` reimport, ⛔ not
this asset.**

---

## D. ⚖️ MY JUDGEMENT OF THE GENERATED IMAGE — shippable, with one honest reservation

I looked at `Content/RawAssets/CardArt/WatchTower.png` **before** importing it, and compared it
against three shipped siblings (`ArrowTower`, `BallistaTower`, `Barracks`).

**What it gets right:** it is a genuine render of **the actual shipped asset** — crenellated stone
tower, doorway, and the ramp — at the family's 512×512 on the family's soft studio-gradient backdrop,
single subject, same framing language. ⛔ **It is not a placeholder and not a stock image.** The
blue-violet backdrop is **within family variance** (backdrops already range tan → teal-blue → brown;
`BallistaTower` is a near-identical cool blue).

**⚠️ The reservation, stated rather than hidden — it is a COMPOSITION issue, not a quality one:**
the camera sits on the ramp's **blind outer side**, so the ramp reads as a blank sloping **wall** and
the **walking surface is never visible.** ⭐ The card's entire identity is *"climbable — your units
ascend to a raised platform"*, and that is the one thing the picture does not show. A second, minor
point: it carries **tiled brick/stone PBR detail** where every sibling is flat-shaded pastel, so it
sits slightly outside the family's material language.

⚖️ **Ruling: SHIP IT, flagged — ⛔ do not block TASK-728 on this.** It is honest, correct and
recognisable, and a text-only card face (the alternative) is strictly worse. **Recommended follow-up
if Jonathan wants the card to communicate the mechanic:** a re-render from a raised camera on the
**+X / deck side**, showing the ramp surface climbing to the platform. That is a 1-hour re-render
against the same mesh, ⛔ no re-model, and it is **not boarded here.**

---

## E. Assets — exact paths and sources

| Asset | `/Game/` path | Source on disk | This pass |
|---|---|---|---|
| Blueprint | `/Game/Blueprints/Buildings/BP_Building_WatchTower` | — | **VERIFIED as found, ⛔ unmodified** |
| Card art | `/Game/UI/CardArt/T_CardArt_WatchTower` | `Content/RawAssets/CardArt/WatchTower.png` | **CREATED + LODGroup corrected** |

⛔ Nothing else in `/Game/` was written. The five §2 assets were re-read but **⛔ not modified**.

## F. Fences — all held

⛔ No `DT_Cards` reimport · ⛔ **editor NOT closed or restarted** (PID 5584 alive) · ⛔ no `L_Arena`
or any `.umap` saved (the one save call passed a single explicit texture path) · ⛔ no C++ · ⛔ no
compile · ⛔ no Git · ⛔ no bulk reimport sweep (`reimport_meshes.py` never invoked — its
`_category_of()` CardID trap stays untouched and stays dangerous) · ⛔ `Tools/Packaging/` and
TASK-716/700 untouched · ⛔ `cards.csv` read only, never edited · ⛔ nothing written to
`Content/RawAssets/CardArt/` (the PNG already existed) · ⛔ no Fab browsing.

## G. What the next gate should scrutinise

1. ⭐ **B.1 — the gate's X half-extent vs the ramp's asymmetric +X reach.** The one open ruling.
2. **C.3 — the card art binds only after `DT_Cards` is reimported.** Build-master's step.
3. **D — the card art hides the ramp deck.** Jonathan's aesthetic call, ⛔ not a blocker.
4. ⚠️ **The importer has now silently mis-set a texture setting TWICE in this task** (ORM sRGB, card
   art LODGroup). **Any future texture import in this project must read back, ⛔ never trust the
   success return.**
