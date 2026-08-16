# TASK-555 — [WR-1] THE 9× CASTLE — same-path re-derivation (art-director handoff)

**Status: COMPLETE — sources staged same-path, ready for TASK-566 (editor-gated import) and TASK-567 (crumble trio).**
Fully headless: **no Unreal editor, no MCP, no PIE, no compile, no Git, no board-adjacent edits beyond this file + the status line.**
**⛔ NO MESHY CREDITS SPENT. NO GENERATION, NO CONCEPT, NO RE-TOPOLOGY, NO BAKE.** Balance untouched; no network call of any kind was made.
Laws: CONVENTIONS **`WR-§0` · `WR-§1` · `WR-§2` · `SC-§34`** + "Castle 3× HOLLOW (2026-07-28)" (SHELL / GATE / UCX DOOR-GAP / FBX-COLLISION-GAP) + "Castle remaster" (CRUMBLE-DERIVATION, UCX-DRIFT, LOD).
Date: 2026-08-15 · Blender **5.1.2**, `--background --factory-startup`.

---

## THE ONE-LINE RESULT

The shipped castle mesh is **uniformly ×3.0** (`2437.9 × 2461.5 × 2694.2` → **`7313.6 × 7384.4 × 8082.6 uu`**, tris and verts **unchanged at 28,698 / 14,057**), and the **entry approach was RE-DERIVED, not scaled**: the naive ×3 chain `0→60→174` (**max step 114 uu**) is replaced by **six equal risers of 29.0 uu** — measured, not claimed.

---

## 1. DELIVERABLES

| artifact | path | state |
|---|---|---|
| the mesh | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Castle.fbx` | **OVERWRITTEN same-path**, 1,217,612 B, object `SM_Castle`, 25 embedded `UCX_SM_Castle_00..24` |
| the recipe | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\pipeline_manifest.json` | `assets.Castle`: `target_dims_ue`, `carve`, `ucx.boxes`, `voxel_size_ue`, `bake` + notes |
| the tool (NEW, reproducible) | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\rescale_refined_fbx.py` | Stage-2b: re-scale + manifest UCX regen + measured verify |
| the readbacks | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\Cache\Castle\rescale_report.json` | every number in this handoff |
| previews (eyeballed) | `...\Tools\ArtPipeline\Cache\Castle\previews_3x\` | 4 orthos + 4 approach/gate diagnostic frames |
| last-known-good | `...\Tools\ArtPipeline\Cache\Castle\Castle_pre_rescale.fbx` | the pre-scale 1× FBX, **verified byte-identical to `HEAD`** |

**Reproduce end-to-end:**
```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup \
    --python-exit-code 1 --python rescale_refined_fbx.py -- --asset Castle --factor 3.0
```

### ⛔ NOT TOUCHED (stated so nobody goes looking)
`Content/RawAssets/Textures/Castle/T_Castle_{D,N,ORM}.png` — **byte-unchanged, 2026-07-28 mtime intact** · `Content/RawAssets/Concepts/Castle.png` · any `.uasset` · `L_Arena` · any `.cpp`/`.h` · `Content/RawAssets/CardArt/` · the crumble trio · `refine_trellis_glb.py`.
⚠️ **The albedo-floor / anti-bleach / luma-retention gates are NOT re-opened and are NOT re-measured — there was no bake.** The texture set is the one that passed on 2026-07-28.

---

## 2. ⭐ THE NAMED DELIVERABLE — THE RE-DERIVED APPROACH (RULING 2, `WR-§1`)

### 2a. Why a naive ×3 was a hard failure, in the project's own numbers

| | shipped 1× | naive ×3 | **re-derived (shipped here)** |
|---|---|---|---|
| chain | `0 → 20 → 58` | `0 → 60 → 174` | **`0 → 29 → 58 → 87 → 116 → 145 → 174`** |
| **max riser** | 38 uu | **114 uu** | **29.0 uu** |
| `AHeroCharacter::HeroMaxStepHeight` = **50** | pass | ⛔ **FAIL** | pass (21 uu margin) |
| `AgentMaxStepHeight` = **35.0** (`Config/DefaultEngine.ini:330`) | ⚠️ **38 > 35** | ⛔ **FAIL** | pass (6 uu margin) |
| Recast effective climb = **40 uu** | pass | ⛔ **FAIL** | pass |
| unit `MaxStepHeight` (engine default 45) | pass | ⛔ **FAIL** | pass |

⭐ **A FINDING WORTH ITS OWN LINE: the SHIPPED 38-uu riser was already ABOVE the configured nominal `AgentMaxStepHeight` of 35.** It has worked only because of cell quantisation — `RecastNavMeshGenerator.cpp:5280` reads
`OutConfig.walkableClimb = FMath::CeilToInt(AgentMaxStepHeight / CellHeight)` = `CeilToInt(35 / 20)` = **2 cells = 40 uu effective**.
The shipped castle sat 2 uu under a rounding artefact. **29.0 uu clears the nominal value outright**, so the entry no longer depends on that rounding.

### 2b. ⛔ WHY TREADS AND NOT A RAMP — STRUCTURAL, NOT TASTE

`WR-§1` allows either. **A ramp is not expressible in this project's collision authority.** `Tools/reimport_meshes.py::_apply_box_collision` (the function that actually authors `SM_Castle`'s collision at import) builds `unreal.KBoxElem` and sets **`center` / `x` / `y` / `z` only — it never sets a rotation**:

```python
be = unreal.KBoxElem()
be.set_editor_property("center", unreal.Vector(float(c[0]), float(c[1]), float(c[2])))
be.set_editor_property("x", float(s[0])); ...
agg.set_editor_property("box_elems", box_elems)
```

An inclined hull would need a rotated box or a convex hull; the shipped path admits neither. ⇒ **option (a), treads.** (The *visual* approach under them **is** a ramp — see 2d.)

### 2c. The chain, MEASURED off the written FBX

| hull | top z | **riser** | y span | tread depth | x span |
|---|---|---|---|---|---|
| `approach_tread_01` | 29.0 | **29.0** | −3630.0 … −3447.0 | 183.0 | −960 … 990 |
| `approach_tread_02` | 58.0 | **29.0** | −3447.0 … −2876.5 | 570.5 | −960 … 990 |
| `approach_tread_03` | 87.0 | **29.0** | −2876.5 … −2344.5 | 532.0 | −960 … 990 |
| `approach_tread_04` | 116.0 | **29.0** | −2344.5 … −1796.5 | 548.0 | −882 … 918 |
| `approach_tread_05` | 145.0 | **29.0** | −1796.5 … −1170.0 | 626.5 | −882 … 918 |
| `floor_slab_hall` (interior floor) | 174.0 | **29.0** | −1170.0 … 1200.0 | 2370.0 | −2100 … 1860 |

- **MAX APPROACH STEP: `29.0 uu`** (6 risers, all equal). Law ≤40 ⇒ **PASS**, and ≥5 treads ⇒ **PASS (6)**.
- **MAX APPROACH WALKABLE FACE ANGLE: `0.000°`** from +Z. Not a claim: every walkable collision surface is the top face of an axis-aligned `KBoxElem`, so its normal is exactly +Z (2b). Tread depths 183–627 uu all exceed the agent diameter (2 × 34 = 68).
- Every tread's y-boundary is the **measured flush point** where the visual apron crosses that tread's top, so no tread top ever rises above the mesh.

### 2d. The VISUAL ramp under the treads — measured, and angle-preserved by design

Uniform scaling preserves angles, which is precisely why a **uniform** ×3 was chosen over a per-axis box-fit to `[7326, 7380, 8082]`:

| measure | value |
|---|---|
| least-squares fit over the exterior ramp (63 samples, y −3633.5 → −1773.5, z 36.58 → 146.12) | **3.072°** |
| endpoint slope | **3.370°** |
| steepest single 30-uu segment | **21.322°** |
| law | **≤ 30°** ⇒ **PASS** on every one of the three |
| `HeroWalkableFloorAngle` 50° / Recast `AgentMaxSlope` 44° | far clear |

### 2e. Does the collision hug the visual? — measured against the SHIPPED baseline

The failure mode of a stepped approach is units **floating** above the visual ramp or **sunk** into it. I measured both on the written 9× **and on the shipped 1×** (from the backup), so the comparison is like-for-like rather than asserted.

| | shipped 1× | shipped 1× **×3** (what a faithful scale would have shipped) | **delivered 9×** |
|---|---|---|---|
| **max FLOAT** (unit hovers) | 15.5 (`apron_step_hi`) | **46.5** | **17.46** ✅ **2.7× better** |
| **max SINK**, worst flank column | 137.4 (`apron_step_lo`) | **412.1** | **377** ✅ better |
| **median sink**, centre columns | ~15 | **~45** | **13–17** ✅ ~3× tighter |
| treads that ever float | — | — | **0 of 4** on `tread_01..04` (`float_cells: 0`) |

The only designed float is `approach_tread_05` (**17.46 uu**) across the gate passage, where the mesh's own floor dips to 128. A flat 174 passage floor there would have floated **46**.

---

## 3. FULL READBACK TABLE (every value re-imported from the written FBX)

| readback | measured | law / expectation | verdict |
|---|---|---|---|
| **bounds** | **7313.576 × 7384.367 × 8082.610 uu** | ±10 % of `7326 × 7380 × 8082` | ✅ **−0.17 % / +0.06 % / +0.01 %** |
| scale ratio vs shipped | **3.000000 / 3.000000 / 3.000000** | uniform ×3 | ✅ |
| **tris** | **28,698** (Δ **0**) | unchanged, ≤30k | ✅ |
| **welded verts** | **14,057** (Δ **0**) | unchanged | ✅ |
| unwelded loops | **86,094** (Δ 0) | (TASK-566 sees ~86k — this is the expected figure, not a defect) | ✅ |
| **min-Z / ground-centre** | **−0.166**, xy centre **(−0.358, −1.278)** | ground-centre | ✅ (1× was −0.055) |
| **UV layer** | **`UVMap`** (sole) | `UVMap` | ✅ |
| **material slots** | **`[TeamRegion, CastlePBR]`** | exact order | ✅ |
| face→slot split | **4,191 / 24,507** — *identical to the shipped file* | unchanged | ✅ |
| UV checksum | **identical to shipped**, TeamRegion area 0.1055 (= shipped 0.1055) | unchanged | ✅ |
| vertex checksum (÷3 vs shipped) | Δ **[−0.003, 0.002, 0.007] uu** total over 14,057 verts | float32 round-trip noise | ✅ |
| **hull count** | **25** (was 22) | ⚠️ *expected 22 — reported per spec* | see 4 |
| hulls vs manifest | **0 mismatches** (all 25 centre+size within 0.05 uu) | exact | ✅ |
| **gate clear opening (visual)** | **1470 uu** widest (= 490 × 3) | carve width 1800 | ✅ |
| **gate collision gap** | **1560 uu** (x −762 … +798) | = 520 × 3 | ✅ |
| **gate clear collision height** | **1356** over the 174 slab (**1385** over the 145 passage floor) | ≈1356 expected | ✅ |
| **interior floor z** | **174.0** (hall_main + hall_east floor median AND max = 174.0) | 58 × 3 | ✅ |
| **interior clear height** | **1560.0** (ceiling 1734 − floor 174) | 520 × 3 | ✅ |
| **max approach step** | **29.0 uu** | ≤40 | ✅ |
| **max approach face angle** | **0.000°** collision / **3.072°** visual | ≤30° | ✅ |
| **UCX drift** | see 5 — **all 13 ratios exactly 3.000** | scale-invariant | ✅ |
| **walkable interior hull-free** | ⚠️ **2 probe hits** — see 4b | hull-free | ⚠️ inherited |

---

## 4. THE TWO THINGS THAT ARE NOT A CLEAN "PASS" — BOTH MEASURED, NEITHER HIDDEN

### 4a. Hull count 22 → **25** (the spec said "expect 22, report if not")
`apron_step_lo` + `apron_step_hi` (**2**) are **retired** and replaced by `approach_tread_01..05` (**5**). Net **+3**. Every other hull is an exact ×3. This is the re-derivation itself, not drift.

### 4b. ⚠️ `keep_front_east` stands inside the `hall_main` carve footprint — **INHERITED, exactly ×3, NOT introduced here**
My probe found 2 interior points inside a hull. The arithmetic, both scales:

- 1×: `keep_front_east` x 246…330, y −20…120 ∩ `hall_main` carve x −640…485, y 90…330 ⇒ overlap **84 × 30 uu**
- 3×: x 738…990, y −60…360 ∩ x −1920…1455, y 270…990 ⇒ overlap **252 × 90 uu** = exactly ×3

At that point the mesh's own floor measures **108.0** (not 174), i.e. a natural pocket the carve never flattened. **It exists in the shipped 1× manifest identically** — TASK-348's "interior is hull-free" claim did not sample that corner. It removes **~1.1 %** of the hall floor area.
⇒ ⛔ **I did NOT "fix" it**: reshaping a shipped shell hull is outside "×3 except the approach", and the risk of cutting a hole in the hall floor is worse than a corner pier. **Named to TASK-566's PIE nav check.**

### 4c. ⚠️ THE CORRIDOR FLOAT — the one genuinely 3×-amplified cosmetic artefact
Measured along the through-route (gate → corridor → hall), visual floor vs collision floor:

| | shipped 1× | delivered 9× |
|---|---|---|
| visual floor, gate passage + corridor | **43** | **128** (= 43 × 3, exact) |
| collision floor over it | 58 | 174 |
| **float** | **15** over a ~497 uu run | **46** over a ~1490 uu run |
| halls | flush (58 = 58) | **flush (174 = 174)** ✅ |

**Root cause, stated honestly:** the carve stage only *subtracts*. Where the donor mesh's natural floor already sits below the carve plane, nothing fills the gap — so the collision floor is flat and the visual floor is not. A Stage-2 re-run would **not** fix this; it needs a "floor fill" cutter the pipeline does not have.
**Why I did not extend the 145 level through the corridor** (the obvious fix): it requires shrinking `floor_slab_hall`'s y-extent, and mis-cutting it leaves a **strip of hall with no collision floor at all** — units fall through. A cosmetic hover is a far cheaper defect than a hole. **Costed follow-up, not taken:** tread_05 extended to y +270 with `floor_slab_hall` starting there ⇒ corridor float 46 → 17. **Manager's/Jonathan's call.**

---

## 5. UCX-DRIFT (the reporting duty) — and a proof of scale-invariance

Hull outer face vs the **outermost** visual point along that face, measured at 3× **and at 1× with homothetic sampling**:

| hull | side | 1× delta | 1× × 3 | **9× delta** | ratio |
|---|---|---|---|---|---|
| `wall_front_west` | front(−Y) | 27.84 | 83.52 | **83.51** | 2.9996 |
| `wall_front_east` | front(−Y) | 2.88 | 8.64 | **8.63** | 2.9965 |
| `tower_front_west` | front(−Y) | 12.33 | 36.99 | **36.99** | 3.0000 |
| `tower_front_east` | front(−Y) | 11.53 | 34.59 | **34.58** | 2.9991 |
| `gatetower_west` | front(−Y) | −0.14 | −0.42 | **−0.42** | 3.0000 |
| `gatetower_east` | front(−Y) | −0.68 | −2.04 | **−2.05** | 3.0147 |
| `spine_west` | west(−X) | 137.89 | 413.67 | **413.68** | 3.0001 |
| `spine_east` | east(+X) | −137.81 | −413.43 | **−413.44** | 3.0001 |
| `keep_west` | west(−X) | 41.70 | 125.10 | **125.10** | 3.0000 |
| `keep_east` | east(+X) | −71.71 | −215.13 | **−215.14** | 3.0001 |
| `wall_back` | back(+Y) | −100.45 | −301.35 | **−301.34** | 2.9999 |
| `tower_back_west` | back(+Y) | −42.92 | −128.76 | **−128.77** | 3.0002 |
| `tower_back_east` | back(+Y) | −42.98 | −128.94 | **−128.95** | 3.0002 |

⇒ **Every drift is exactly ×3. No hull moved relative to the mesh.**
📌 These absolute values differ from TASK-348's published table because they use the **outermost**-visual-point rule while TASK-348 used the **narrowest**-visual-extent rule (the shipped authoring rule). Different metric, same geometry — the ×3 invariance is the load-bearing result.
⚠️ **Method note, recorded because it bit me:** the first version of this probe used a **fixed** 20-uu sample inset, which makes the 1× and 3× runs sample non-homothetic points; the ratios came out as 7.0 and 39.0 against a geometry that is exactly ×3 **by construction**. The probe now samples fractions of the face span. **A measurement artefact that looks like a defect is worth as much attention as the defect.**

---

## 6. ⚠️ TEXEL DENSITY — REPORTED, DELIBERATELY NOT FIXED (D2, `WR-§0`)

- Bake unchanged at **2048²**; the mesh is **3× linear** ⇒ **9.00× fewer texels per unit area**.
- Concretely: surface area went **1 → 9×** against a **constant** 2048² = 4,194,304 texel budget across UV coverage that did not change (TeamRegion 10.55 % / CastlePBR 89.45 %, identical face split).
- ⚖️ **RULED ACCEPTED for this pass.** ⛔ **The lever was NOT spent unasked:** a **4096²** re-bake is a *free* Stage-2 re-run (**no Meshy credits** — the donor `Cache/Castle/meshy_raw.glb` is on disk) that restores **4× of the 9×** lost density. It would, however, re-run remesh/decimate/bake, land a **different triangulation**, and **re-open the albedo/bleach/retention gates**. **Jonathan's call — flagged, not taken.**

---

## 7. THE RE-DERIVATION LEDGER (`SC-§34`) — MY ROWS ONLY

Rows I own outright, plus every row I touched or must name onward. (i) re-derived · (ii) deliberately unchanged · (iii) not mine, named to its owner.

| # | artifact | disposition | evidence |
|---|---|---|---|
| A | `Castle.fbx` bounds | **(i)** ×3.0 uniform → `7313.6 × 7384.4 × 8082.6` | §3 |
| B | tris / verts / UVs / slots | **(ii) UNCHANGED — a scale-up may not spend triangles** | Δ0 / Δ0 / identical checksum / identical split |
| C | `T_Castle_{D,N,ORM}.png` | **(ii) BYTE-UNCHANGED** — no bake ran | §1, §6 |
| D | 22-hull shell set | **(i)** ×3 exactly, drift ratios all 3.000 | §5 |
| E | `apron_step_lo` + `apron_step_hi` | **(i) RETIRED → `approach_tread_01..05`**, max riser 114 → **29.0** | §2 |
| F | approach **step LIMIT** (≤40) | **(ii) HUMAN-SCALE, NOT MULTIPLIED** — bodies did not grow | `WR-§1` |
| G | gate clear-opening floors ≥500 × ≥450 | **(ii) UNCHANGED — they describe BODIES**; delivered 1560 × 1356 simply exceeds them further | §3 |
| H | `carve` cutters | **(i)** ×3 (floor 58→174, gate 600→1800, corridor 500→1500, heights 470/520→1410/1560) — **recipe only; no carve ran** | manifest |
| I | `carve.segments` = 32 | **(ii) UNCHANGED — a tessellation COUNT, not a dimension** | manifest note |
| J | `voxel_size_ue` 6.0 → **18.0** | **(i)** re-run consistency: 7326/18 = 407 voxels across ≡ shipped 2442/6 = 407. **Did not execute** | manifest note |
| K | `bake.cage` 16→48, `ray` 48→144 | **(i)** re-run consistency; cage must scale with dims. **Did not execute** | manifest note |
| L | `team_region` selectors | **(ii) UNCHANGED — normalised 0..1 box, scale-invariant**; measured area fraction identical (0.1055) | §3 |
| M | `SM_Castle_Crumble01/02/03` | **(iii) NOT MINE → TASK-567.** ⛔ Until it lands the castle **shrinks to a third mid-match at 25 % damage** | §8 |
| N | `GateBlockerRelativeLocation` / `Extent` | **(iii) TASK-557 (landed)** — see §8, **one interaction I must name** | §8 |
| O | `SpawnBoxHalfExtent`, `CastleKeepClearRadius`, HP-bar Z, `InteriorNavModifier` | **(iii) TASK-557 / TASK-569** — verified landed at `(7380,7380)` / `4500`; not mine | §8 |
| P | `ACaptureZone::ZoneHalfExtent` `(840,840)` | **(ii) UNCHANGED** — descriptive origin, not a pairing law | `WR-§2` row 7 |
| Q | `SiegeNet::ArenaRelevancyDistance` | **(ii) UNCHANGED — the ARENA did not grow** | `WR-§2` row 13 |
| R | corridor collision-vs-visual float | **(ii) UNCHANGED, with the reason and the cost** | §4c |
| S | `keep_front_east` in the hall corner | **(ii) UNCHANGED — inherited, exactly ×3** | §4b |

---

## 8. ⛔ NOTES FOR THE TASKS DOWNSTREAM

### TASK-566 (same-path reimport, build-master) — read all five

1. **✅ THE TEXTURE-SKIP TRAP IS HARMLESS HERE, AND HERE IS WHY — DO NOT CHASE IT.**
`Tools/reimport_meshes.py::_ensure_textures_and_mi()` early-returns when `MI_Castle_PBR` exists, so a plain run reimports geometry and keeps the existing textures. **That is exactly the correct outcome this time**: TASK-555 deliberately changed **no texel**, and `T_Castle_{D,N,ORM}.png` are byte-identical to the ones already imported on 2026-07-28. The UVs are byte-identical too (checksum match, §3). ⇒ **The old textures on the new mesh ARE the right textures.** State that you confirmed it; **do not** add a texture-reimport step, and **do not** delete the MI.
2. **⛔ NEW HAZARD — ASCII-ONLY IN `pipeline_manifest.json`.** `Tools/reimport_meshes.py:162` reads the manifest with `open(MANIFEST_PATH, "r")` and **no `encoding=`**, so under UE's Windows Python it decodes as **cp1252**. A single byte outside cp1252 (any emoji) **crashes the reimport** — the very script that authors the collision boxes. My manifest edits are pure ASCII and I verified the file still decodes+parses under cp1252. **Keep it that way** (a note to that effect is now in `_dims_source`). Fixing the script to pass `encoding="utf-8"` is a real one-line improvement but it is **your file, not mine** — flagged, not taken.
3. **Hull count is 25, not 22** (§4a) — `_apply_box_collision` will write **25 `KBoxElem`s**. Expect `collision_boxes(25)`.
4. **LOD gate:** expect `lod_count == 3`, LOD0 **28,698 tris** in the FBX. ⚠️ The board says "≈28,702" — the 4-triangle difference is the pre-export vs post-export count in the 2026-07-28 refine report and is **pre-existing, not introduced here** (`verify3x.json` recorded 28,698 for the shipped file too). **Welded verts 14,057; unwelded loops 86,094 is EXPECTED, not the "≈86k ⇒ restore last-known-good" failure signature.**
5. **Restore path** if anything fails: `Tools/ArtPipeline/Cache/Castle/Castle_pre_rescale.fbx` is the pre-scale 1× file and is **verified byte-identical to `HEAD`** (`git status` reported no diff after restoring it). `git checkout -- Content/RawAssets/Castle.fbx` works equally.

### TASK-567 (crumble trio) — the reason it exists
`SM_Castle_Crumble01/02/03` are byte-copies of the **OLD** castle. Until 567 lands, the castle **visibly shrinks to a third of its size, on screen, mid-match, at 25 % damage.** The rebuilt source is the new `Content/RawAssets/Castle.fbx`; **both** material slots take `MI_Castle_Crumble0N` (slot 1 is never written at runtime). ⭐ **You may reuse my tool:** `rescale_refined_fbx.py` does a guarded uniform re-scale of any already-refined FBX + manifest UCX regen + a measured verify.

### TASK-557 / TASK-569 — ⚠️ ONE INTERACTION MY RE-DERIVATION CREATES, NAMED HERE
TASK-557 shipped `GateBlockerRelativeLocation (18, −1575, 852)` / `GateBlockerExtent (900, 405, 678)` ⇒ blocker spans z **174 … 1530**.
My re-derived **gate-passage collision floor is 145**, not 174 ⇒ **a 29-uu gap under the blocker.**
✅ **HARMLESS AS SHIPPED:** a 29-uu gap passes no capsule (agent radius 34, half-height 88), so the blocker still seals. **No change requested.**
📌 If a flush seal is ever wanted the arithmetic is **`RelLoc.z 837.5` / `Extent.z 692.5`** (bottom 145, top unchanged at 1530). Recorded so it is a decision, not a discovery.

### TASK-565 (the gate) — what to re-run independently
The whole readback set is reproducible from the command in §1; `rescale_report.json` holds every raw number including the full approach profile, the through-route floor table and the per-tread float/sink scan.

---

## 9. WHAT I DID NOT DO, ON PURPOSE

- ⛔ No Meshy call, no generation, no concept, no re-topology, no re-bake, no texture write.
- ⛔ No Unreal, no MCP, no PIE, no compile, no Git, no `L_Arena`, no `.uasset`, no `.cpp`.
- ⛔ No navmesh ruling — `WR-§3` puts that on TASK-566's PIE measurement, and nothing here depends on it.
- ⛔ Did not touch `Content/RawAssets/Torch.fbx` / `WarTable.fbx` / `build_warroom_props.py` (TASK-556's, running in parallel) — and my manifest edits were **surgical exact-string replacements**, not a rewrite, precisely so a concurrent `assets.*` addition could not be clobbered.
- ⛔ Did not "fix" §4b or §4c — both are inherited, both are measured, both are named to an owner.
