# TASK-727 — [TOWER-3] `SM_WatchTower` + the ramp geometry — art-director handoff

**Status: mesh + collision + textures AUTHORED and MEASURED. Stage-3 IMPORT is BLOCKED on the
Unreal MCP endpoint and is NOT done — see §7. Nothing was faked and nothing was forced.**

Built to **TASK-725's refuted-and-corrected budget (30°)**, ⛔ not to the board's original 40°.

---

## 1. As-built vs the specified table — every row MEASURED, not asserted

Measured by raycasting the **exported** collision hull soup and the **exported** render mesh
(`mathutils.bvhtree`), not read back from the constants that produced them.

| Property | Required | **AS BUILT** | Verdict |
|---|---|---|---|
| Ramp slope | 30.0° | **30.0000°** (collision) / **30.0000°** (render) | ✅ EXACT |
| Rise per Recast cell | ≤ 1 voxel | **0.9238 vx** (18.4752 uu / 20) | ✅ ledge clause B passes |
| Ledge-filter ceiling | 32.005° | 30.0000° — **2.005° of margin** | ✅ |
| Run | 2,078 uu | **2,078.4610 uu** | ✅ EXACT |
| Sloped face | 2,400 uu | **2,400.0000 uu** | ✅ EXACT |
| Deck width | 300 uu (⛔ not 200) | **300.0 uu** clear between kerbs ⇒ **172 uu** corridor after the 128 uu ledge+erosion cost | ✅ |
| Clearance | ≥ 200 uu | **UNBOUNDED** — open sky over every walkable surface, on **both** the collision and the render mesh (upward raycast, 40 m reach, zero hits over ~13k samples) | ✅ EXCEEDS |
| Platform | 600 × 600 @ 1,200 uu rise | **780 × 780 clear @ 1,200.0 uu** ⇒ **652 × 652** after erosion | ✅ EXCEEDS (+30% per axis) |
| Form | SOLID WEDGE, ⛔ not a floating plank | **Solid triangular prism resting on the ground**, z = 0 underside, full 460 uu width | ✅ |
| Ramp→platform junction | FLUSH (lip > 40 uu severs) | **lip = 0.0001 uu** collision, **0.0001 uu** render | ✅ 400,000× inside budget |

⭐ **ZERO DEVIATIONS.** Nothing drifted; in particular **the slope is not merely under 32° — it is
exactly 30.0000° on both surfaces**, so the silent-failure mode the dispatch warned about did not
occur and could not have occurred (the ramp plane is a single unbroken quad, not a tessellation).

**Two deliberate over-builds, stated so they are not read as drift:**
- Platform **780×780**, not 600×600. The parapet is built OUTSIDE the clear deck per TASK-725 §3, and
  the tower's cap is 940×940, so 780 clear falls out of the massing for free. More capacity, same law.
- Clearance is **unbounded, not 200**. There is deliberately **no roof and no overhead geometry of any
  kind** — every tower box terminates at or below the deck plane. This makes the ≥200 uu requirement
  structural rather than something a future edit could erode. ⚠️ **Anything added over the deck later
  must clear 200 uu, and the binding number is the unit's real 176 uu capsule, not the 144 nav agent.**

---

## 2. ⭐ How the collision was authored, and the worst-case hull deviation

**Authored as 14 hand-placed exact convex primitives. There is NO convex decomposition anywhere in
the chain — not in Blender, not at import.** This is the answer to the dispatch's central warning.

**Worst-case deviation of the authored collision from the analytic walkable surface:**

| Surface | max &#124;deviation&#124; | mean &#124;deviation&#124; | samples | ray misses |
|---|---|---|---|---|
| **Ramp deck** (400 × 21 grid over the full 2,078 uu run) | **0.000512 uu** | 0.000126 uu | 8,400 | **0** |
| **Platform deck** (60 × 60 grid over the clear 780×780) | **0.000286 uu** | 0.000043 uu | 3,600 | **0** |

**0.0005 uu is float32 storage noise — it is not a modelling tolerance.** The deviation is zero *by
construction* and the measurement confirms the construction survived the export:

- **`UCX_SM_WatchTower_00`** (the ramp) is a **6-vertex triangular prism whose +Z face IS the 30°
  plane** — the same four corner coordinates as the render mesh's deck quad. Not a hull *fitted to*
  the ramp; the ramp's own surface.
- **`UCX_SM_WatchTower_06`** (the cap) has its **+Z face at z = 1200**, and it terminates at
  **x = 380** — exactly where hull 00 begins at z = 1200. That is why the junction lip is 0.
- **Zero ray misses** is the load-bearing half of the table: it proves there is no hole in the
  collision anywhere on either walking surface.

**The hull manifest (14):**

| # | Node | Role | Verts |
|---|---|---|---|
| 00 | `UCX_SM_WatchTower_00` | **ramp wedge — carries the walkable 30° plane** | 6 |
| 01 / 02 | `_01` / `_02` | kerb rail +Y / −Y (slanted prisms, OUTSIDE the 300 uu deck) | 8 |
| 03 | `_03` | plinth | 8 |
| 04 | `_04` | shaft | 8 |
| 05 | `_05` | corbel ring | 8 |
| 06 | `_06` | **cap — carries the platform deck at z = 1200** | 8 |
| 07–11 | `_07`…`_11` | parapet W / N / S / E-N / E-S | 8 ea |
| 12 / 13 | `_12` / `_13` | buttress +Y / −Y | 8 |

**Why UCX and not the manifest's `ucx.boxes`** — a **declared departure (`SC-§15`)**, recorded in the
manifest itself:
- **A 30° ramp cannot be expressed as an axis-aligned box.** `_apply_box_collision()` *clears*
  `agg_geom` and rewrites it, so supplying boxes here would **destroy the ramp surface** — the exact
  "hull swallows the ramp, every property readback still reads correct" defect the spec names.
- `TL-§2`'s own **W6-R3 correction licenses this**: UCX hulls **do** survive UE 5.8 import when the
  node names bind. **Verified in the produced file:** `WatchTower.fbx` carries **exactly one render
  node `SM_WatchTower`** plus `UCX_SM_WatchTower_00..13` matching it byte-for-byte, and its
  Model/Geometry name pattern is **identical to the known-good `Castle.fbx`** (whose 25 hulls bound).
  The WarTable failure mode (two render nodes, `WarTable` + `SM_WarTable`) is absent.
- Decorative-only, deliberately uncollided: merlons, string course, doorway slab, kerb posts.
  **None of them lies on or above a walkable surface** — the clearance measurement above was run
  against the render mesh too and still returned unbounded.

⛔ **`TL-§2` acceptance readback at import: expect `convex_hull_count == 14`, `box_count == 0`.
ZERO hulls is a STOP — this mesh is NOT collisionless on purpose.**

### 2b. ⭐ Round-trip probe — the contract verified against the FILE, not against memory

Re-imported `WatchTower.fbx` into a clean headless Blender and re-measured from the imported result:

```
render_nodes                 ["SM_WatchTower"]          <- EXACTLY ONE; the TL-2 binding contract
ucx_count / first / last     14 / _00 / _13
tris / uv_layers / slots     464 / ["UVMap"] / ["TeamRegion","WatchTowerPBR"]
bounds_uu                    [-600, -470, -0.0] .. [2458.461, 470, 1420]
roundtrip_slope_deg          30.0000
roundtrip_platform_deck_z    1200.0002 uu
roundtrip_junction_lip       0.00000 uu
```

⇒ **the slope, the flush junction, the single-render-node rule, the UV layer name and the slot order
all survive serialisation.** This is the check that would have caught a silent export-time failure,
and it passed.

---

## 3. ⛔⭐ THE TRAP I FOUND AND CLOSED — it would have auto-decomposed the ramp behind a green run

`Tools/reimport_meshes.py::_category_of()` **defaults an unknown CardID to `"unit"`**, and the unit
branch runs `remove_collisions()` + `set_convex_decomposition_collisions()`. With no manifest entry,
**any future sweep that touched `WatchTower` would have deleted all 14 authored hulls and let an
automatic convex decomposition decide whether the ramp is walkable** — and reported DONE.

Closed by adding the `WatchTower` entry to `Tools/ArtPipeline/pipeline_manifest.json`
(**purely additive, +19 lines, CRLF preserved, ASCII-only, no `TL-§1` cp1252 trapdoor bytes**):
`"category": "building"` (routes away from the unit branch) + `"ucx": {"boxes": []}` (⇒
`_apply_box_collision()` appends a WARN and **leaves the imported UCX collision**, which is the wanted
behaviour) + the reasoning, the acceptance count, and the measured deviations, so nobody later
"repairs" the empty box list.

**Simulated against the shipped functions, not assumed:**
```
WatchTower   category=building  boxes=[]   -> _apply_box_collision -> WARN + LEAVE imported collision
CONTROL (no manifest entry)  category=unit -> AUTO-DECOMPOSITION decides whether the ramp is walkable
```

---

## 4. Files produced

**Source (checked into Git alongside the future .uasset):**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\WatchTower.fbx`
  — 61,020 bytes, sha256 `913c2723a96bcbcc…`, 15 nodes (1 render + 14 UCX)
- `C:\...\Content\RawAssets\Textures\WatchTower\T_WatchTower_D.png` — 2048², sRGB
- `C:\...\Content\RawAssets\Textures\WatchTower\T_WatchTower_N.png` — 2048², normal
- `C:\...\Content\RawAssets\Textures\WatchTower\T_WatchTower_ORM.png` — 2048², LINEAR, R=AO G=rough **B=0**

**Tooling (`Tools/**/*.py` is CODE ⇒ the tooling QA gate applies before commit):**
- `C:\...\Tools\ArtPipeline\build_watchtower.py` — the procedural authoring script; every dimension
  is a named constant traceable to TASK-725 §3. Reproduce headless:
  `blender.exe --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/build_watchtower.py`
- `C:\...\Tools\ArtPipeline\pipeline_manifest.json` — +19 lines, additive (see §3)

**Evidence (gitignored cache):**
- `C:\...\Tools\ArtPipeline\Cache\WatchTower\watchtower_report.json` — every number in this handoff
- `C:\...\Tools\ArtPipeline\Cache\WatchTower\previews\watchtower_{hero,side_elevation,top,collision_hulls}.png`
  — ⭐ **all four eyeballed before export; `collision_hulls.png` shows the 14 hulls forming one
  continuous walkable channel with no hull intruding above the ramp plane.**

⚠️ **No `Content/RawAssets/Concepts/WatchTower.png` and no `Tools/ArtPipeline/Inbox/WatchTower.png`.**
Deliberate and declared: this is **not** a TRELLIS asset. The defining property is arithmetic, and a
generative mesh cannot hold 30.000°. **The build script is the source**, which is the same
prop-authoring lane as `build_warroom_props.py` / `build_entry_dressing_props.py`. Zero TRELLIS/Meshy
spend, zero HF calls, `HF_TOKEN` never read.

---

## 5. Integration notes (for TASK-728 / build-master)

- ⭐ **PIVOT — read this before placing anything.** The origin is at the **centre of the tower shaft
  at ground level**, ⛔ **not** the bounding-box centre. `min_z = 0.0` exactly. **The ramp extends
  along +X**, so the actor's local +X is "down the ramp". The player places the *tower*; the ramp is
  its appendage. Bounds: X `[-600, +2458.461]`, Y `[±470]`, Z `[0, 1420]` ⇒
  **3058.461 × 940 × 1420** (X min is −600, not −560 — the buttresses are the westmost geometry).
  ⚠️ **The 200 uu building-clearance check and the placement ghost both use the whole bounds, which
  are 3,058 uu long — ~12× a wall. Expect placement to feel very different from other buildings; that
  is geometry, not a bug.** (TASK-725 §7 item 2 flagged the same thing for spawn-overlap.)
- **Scale 1.0.** Authored in uu through a single `m()` conversion; export axis contract is the shipped
  one verbatim (`axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale`, FACE smoothing, triangulated,
  `path_mode='STRIP'`) plus the TASK-348 UE handedness pre-comp (mirror_Y + winding flip). The mesh is
  **exactly Y-symmetric (worst mismatch 0.0 uu, 0 misses)**, so the mirror is a proven geometric no-op.
- **Material slots, in order — the two-slot contract:** `[TeamRegion, WatchTowerPBR]`.
  Slot 0 `TeamRegion` takes `MI_TeamColor_<Team>` from `Building.cpp:80` unchanged — **measured area
  fraction 0.0615** (cap 0.4). Selector = merlons + kerb posts + parapet coping tops, chosen for the
  **top-down RTS read: team colour marches UP THE RAMP on the posts and rings the platform**.
  Slot 1 `WatchTowerPBR` takes `MI_WatchTower_PBR`.
- **Mesh:** 464 tris (budget 20,000) · 310 verts · UV layer named exactly `UVMap` · **Nanite OFF**.
  At 464 tris an LOD chain is pointless; a `LOD_STEP_FAILED` token here is expected and non-blocking
  (`TL-§3`), and "the readback already matches, so nothing was written" is a complete result.

---

## 6. Texture gates

| Gate | Value | Verdict |
|---|---|---|
| Albedo floor (UV-norm covered mean linear) | **0.3788** vs floor 0.2536 | ✅ PASS |
| Anti-bleach operative guard (`retention > 1.25 AND uv_norm > 0.60`) | uv_norm 0.3788 | ✅ **NOT tripped** |
| ORM metal max | 0.0 | ✅ house B=0 pattern |
| ORM AO covered mean | 0.5494 | — |
| Intent retention | **1.4013** vs warn band [0.85, 1.25] | ⚠️ **above band — declared, see below** |

⚠️ **The retention exceedance is the pinned delight profile, not a palette error, and the report says
so in a computed field rather than in prose.** The LOCKED profile divides linear albedo by
`max(AO, 0.25)` *before* the gamma, so a mesh with this much self-occlusion lifts itself:
`(1/0.5494)^0.55 = ` **1.3902**. Dividing that out leaves **1.008** — the authored palette is within
0.8% of its declared intent. The band is warn-only; the operative guard is untripped; the previews
read as mid-tone stone/timber, not bleached. `intent_retention_ao_divide_component` and
`intent_retention_residual_vs_palette` are now emitted by the script so this cannot be re-litigated
from prose alone.

**Two defects found by eyeballing run 1 and fixed in run 2** (recorded because both will recur):
1. **Z-fighting black rectangle.** The string course's `+X` face landed **exactly on the shaft's `+X`
   face plane with the SAME normal**. Standing rule now in the script: *a decoration is PROUD on every
   axis where it touches, or it stops short.* Coincident faces with **opposite** normals are
   backface-culled and are fine — which is why every stacked box in the massing is safe.
2. **Brick degenerating to vertical stripes.** Blender's Brick node lays rows along **Y**, so raw
   object coords make every vertical wall a set of vertical streaks. Fixed by remapping to `(x+y, z)`
   so rows advance with **height** on any vertical face. Horizontal faces degenerate under that map,
   which is why the **platform deck got its own `art_paving` family** in true `(x, y)` — the right
   answer anyway, since it is a walking surface and now reads as flagstone.

---

## 7. ⛔ THE EDITOR — import genuinely needs it, so I stopped rather than forced it

**Measured, not assumed:**
- `UnrealEditor` **process IS running** (PID 372, started 11:45).
- **`http://127.0.0.1:8000/mcp` is NOT reachable** — and `Get-NetTCPConnection -State Listen` shows
  **no listener on 8000 / 8080 / 9876 / 30010**. The editor is up; **its MCP plugin server is not.**
- (The Blender MCP bridge on 9876 is also down — irrelevant here, since heavy authoring must run
  headless anyway under the 30 s socket cap, and it did.)

⇒ **Stage-3 IMPORT is NOT done. These paths DO NOT EXIST yet:**
`/Game/Meshes/SM_WatchTower` · `/Game/Textures/T_WatchTower_{D,N,ORM}` ·
`/Game/Materials/Instances/MI_WatchTower_PBR`. Verified absent on disk.

**The exact recipe, so the import is mechanical the moment the endpoint is up** (this is a **CREATE**,
not a same-path overwrite — the asset has never existed, so the never-delete-and-recreate law has
nothing to protect yet):
1. Textures → `/Game/Textures/`: `T_WatchTower_D` **sRGB ON** · `T_WatchTower_N` normal-map ·
   `T_WatchTower_ORM` **LINEAR, sRGB OFF**.
2. `MI_WatchTower_PBR` in `/Game/Materials/Instances/`, parent **`/Game/Materials/M_AssetPBR`**,
   params `BaseColor` / `Normal` / `ORM` (match `MI_ArrowTower_PBR`).
3. FBX → `/Game/Meshes/SM_WatchTower` with **`auto_generate_collision = False`** and
   **`import_materials = False`**; slots exactly `[TeamRegion, WatchTowerPBR]`
   (`MI_TeamColor_Blue` design-time on slot 0, `MI_WatchTower_PBR` on slot 1); **Nanite OFF**.
4. ⛔ **READ BACK AND REPORT `convex_hull_count` — it must be `14`, with `box_count == 0`.
   Zero is a STOP.** Then re-run the §1 slope/lip check in-engine if cheap.

⛔ I did **not** attempt to start, bounce, or otherwise touch the editor — Jonathan is likely at the
machine and the editor's state is his call. ⛔ No `L_Arena` save. ⛔ `SK_Footman_Skeleton`, `ABP_Footman`
and every existing `BP_Building_*` untouched.

---

## 8. Fences honoured

⛔ No `BP_Building_WatchTower` (TASK-728's) · ⛔ no C++ (TASK-726's) · ⛔ no `cards.csv` (TASK-723's,
already landed) · ⛔ no compile · ⛔ no Git · ⛔ nothing in `Tools/Packaging/` · ⛔ no TASK-716/700
surface touched · ⛔ no `Content/RawAssets/CardArt/` or `/Game/UI/CardArt/` write (card-art lane
isolation) · ⛔ no Fab/marketplace browsing.

## 9. What the next gate should scrutinise

1. **§2's deviation table is the deliverable.** If `convex_hull_count` reads anything but **14** at
   import, the UCX binding failed and everything else in this handoff is moot.
2. **§3's manifest entry is safety, not bookkeeping.** Anyone who "fixes" `ucx.boxes: []` by adding
   boxes destroys the ramp. The entry says so at the point of edit.
3. **The 780×780 platform and unbounded clearance EXCEED spec** — confirm that is read as headroom,
   not as a spec miss.
4. **Retention 1.4013 is above the warn band** and is declared with its derivation (§6). It is not
   hidden and the operative anti-bleach guard is untripped.
