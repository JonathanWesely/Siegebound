# TASK-1083 — [TREE-ROSTER] — build-master handoff (ESCALATION, nothing written)

**Marker:** `TASK-1083-TREE-ROSTER` · **Date:** 2026-09-06 · **Status:** **blocked-with-question** — Jonathan ruled **Option D** (§7); D is **not executable as worded** because the node it names does not exist (§8). Roster FINAL = unchanged (§9). §0–§6 below are the original escalation, kept as history.
**THIS ROW CHANGED NOTHING.** No `set_properties`, no save of any kind, no `.uasset` write, no commit, no push, no sheet re-issue. Verified at the end (§6).

Instruments: Unreal MCP (editor PID 20564, opened fresh 08:24, MCP `127.0.0.1:8000` answering; the art-director's TASK-1093 is live in the same process) · `ObjectTools.get_properties` on the live DataAsset · `StaticMeshTools.get_bounds/get_lod_count/get_triangle_count/get_lod_thresholds` on all 24 candidates · `AssetTools.get_dependencies/exists/get_referencers` · `sha256sum` · `nvidia-smi`.

---

## 0. WHY THIS IS AN ESCALATION AND NOT A WRITE — cl. (1)

The board row's cl. (1) says stop if the praised light-yellow trees came from the **same `Trees` layer**. `TASK-1081` §2 measured they did **not** — they are 15 hand-placed `StaticMeshActor`s outside the playfield — **but the orchestrator broadened the condition to "same layer OR same mesh" at 1081's dispatch, and on that rule it fires:** the background ring uses `SM-Mobile_Tree_{1,4,5,7,8,11,12}`, 7 of the 12 entries in the live `Layers[Trees].Meshes`. 1081 imaged dark **and** gold canopies of the same mesh in one frame (`TASK-1081-lane-vantage.png`), differing only by bearing-to-sun and distance, and wrote *"His ruling is required."*

⇒ *"Swap the battlefield ones for lighter ones"* has **nothing lighter to swap to** — every rosterable tree in both folders shares the one gold `T_Leaf_Pack1` albedo (`TASK-1082` §③: CTI 28.1–30.8, a 9 % spread). Any roster I wrote would be a **silent pick** among options that differ on framerate, VRAM and an *unproven* tone hypothesis — cl. (2) / `FIELD-§3` forbid that. So: numbers, options, and wait.

**One mechanical fact that bounds every option below (1081 §2, re-confirmed here):** a `DA_BattlefieldScatter` edit **cannot reach** the 15 praised actors — they reference `SM-Mobile_Tree_*` directly and `overrideMaterials` is empty on all 15. The roster is the **only** lever that changes the battlefield trees *without* touching the background ones. A **lighting** change or a **vendor-material** change touches **both** populations.

---

## 1. THE CURRENT `Layers[Trees]` — READ BACK FROM THE ENGINE (PID 20564), IN FULL

Second independent read (different editor process from 1081's PID 17008); **byte-for-byte the same values 1081 reported.**

| field | live value |
|---|---|
| `LayerName` | `Trees` |
| `Meshes` (12) | `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1` … `SM-Mobile_Tree_12`, in numeric order 1→12 (idx 0→11) |
| `InstanceCount` | **340** |
| `ScaleRange` | **(0.8, 1.2)** |
| `bRandomYaw` | true |
| `bBlocking` | **true** |
| `RegionBias` | **EdgeBias** |
| `MinSpacing` | **300** |
| `ZOffset` | **0** |
| `FootprintRadius` | **150** (explicit override — the only layer with one) |
| `CollisionProxyMesh` | **`/Engine/BasicShapes/Cylinder`** |
| `CollisionProxyScale` | **(1.4, 1.4, 17)** |
| `CollisionProxyZOffset` | **850** |
| `bAllowOnHills` | true · `MaxPlacementSlopeDeg` 35 |
| `OverrideMaterial` | None |
| `CullStartDistance` / `CullEndDistance` | **24000 / 32000** |
| `bCastShadows` | **true** |
| Globals | `SymmetryMode` **Rotational180** · `ArenaHalfExtent` (26000, 12000) |

Read-back `Meshes` array verbatim (engine JSON, `refPath` wrapper stripped):
```
[0]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1.SM-Mobile_Tree_1
[1]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_2.SM-Mobile_Tree_2
[2]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_3.SM-Mobile_Tree_3
[3]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_4.SM-Mobile_Tree_4
[4]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_5.SM-Mobile_Tree_5
[5]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_6.SM-Mobile_Tree_6
[6]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_7.SM-Mobile_Tree_7
[7]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_8.SM-Mobile_Tree_8
[8]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_9.SM-Mobile_Tree_9
[9]  /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_10.SM-Mobile_Tree_10
[10] /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_11.SM-Mobile_Tree_11
[11] /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_12.SM-Mobile_Tree_12
```
⇒ **`TASK-1082`'s sheet indices 12–23 are the live array's 0–11.** Its indices 0–11 (`SM_Highpoly_Tree_1..12`) are rostered **nowhere** (`get_referencers(SM_Highpoly_Tree_1)` = only the vendor showcase map `/Game/Tree_Pack_1/Maps/Highpoly_Tree_1`). The sheet is correctly marked PROVISIONAL and is **not** re-issued by this row — there is no final array yet.

---

## 2. THE ONE CANDIDATE CHANGE THAT EXISTS — `SM_Highpoly_Tree_1..12` — PRICED FROM THE ENGINE

### 2a. `ZOffset` — derived, not eyeballed: **0, deliberately (base-pivot meshes)**
`get_bounds` on all 24. **Highpoly: `min.z = 0.0` on 12/12.** Mobile: `min.z ∈ {0.0, −0.1, −0.5}` (12/12 within 0.5 uu of 0). Both sets are base-pivot; the current `ZOffset 0` is right for Mobile and would be right for Highpoly. Nothing floats, nothing sinks.

### 2b. The collision-proxy triple — **transfers unchanged, with numbers**
Heights: Highpoly **1381.8–2088.7 uu**, Mobile **1385.2–2094.2 uu** (per-tree Δ ≤ 5.5 uu). XY extents agree within ≤ 47 uu on every axis of every pair — **the same 12 silhouettes** (matches 1082's IoU 0.81 diagonal). The engine `Cylinder` is 100 uu tall / 100 uu wide, pivot centred: `(1.4, 1.4, 17)` ⇒ a **70 uu-radius, 1700 uu-tall** trunk proxy; `850` = half its height ⇒ base at ground. Identical trunks ⇒ **`Cylinder` / `(1.4,1.4,17)` / `850` must be carried across verbatim** under any option that writes.

### 2c. `FootprintRadius` — **recommend PRESERVE 150, and here is why the "prefer 0" clause does not bind**
`FIELD-§2`/cl. (3) prefer auto (0) *"for a mixed roster"*. This roster is **not** mixed in the sense that matters: the Highpoly silhouettes are the Mobile silhouettes (2b). And on a **proxy layer** `FootprintRadius` is a **trunk clearance**, not a canopy one — the canopy is `NoCollision` by the proxy contract. Auto-derive (`BattlefieldScatter.cpp:670-679`: XY half-diagonal × rolled scale) would give **476–1161 uu** per instance (half-diagonals 594.5 `Tree_1` … 967.8 `Tree_8`, × 0.8–1.2) instead of 150, and that value feeds the field-edge clamp (`:683`), keep-clear/corridor inflation (`:691`) **and** the radius-aware `MinSpacing` (`:717`: reject if closer than 300 + r₁ + r₂). Centre-to-centre minimum would go from **600 uu to ~1250–2600 uu**; the corridor band would repel trees out to ~1.5–2.2 k uu from the lane centre. ⇒ **a materially sparser field**, on top of 1081 §5's open "field may already read sparse" anomaly. Flipping it is a **design change** he has not asked for; 150 stays unless he rules otherwise.

### 2d. MB — the VRAM delta of a Highpoly swap
| component | Mobile (live) | Highpoly | Δ |
|---|---:|---:|---:|
| meshes (12, engine `EstTotalCompressedSize`, 1082) | 1.49 | 2.31 | **+0.82** |
| `T_Leaf_Pack1` 2048² DXT5 | 5.33 | 5.33 | 0 |
| `T_Trunk_Pack1` 1414² B8G8R8A8 (uncompressed) | 7.63–10.17 | same | 0 |
| `T_Leaf_Pack1_normal` 2048² BC5 | — | 5.33 | **+5.33** |
| `T_Trunk_Pack1_normal` 1414² B8G8R8A8 (uncompressed) | — | 7.63–10.17 | **+7.63–10.17** |
| **total Δ** | | | **≈ +14 to +21 MB** (1082's formula → 1081's engine reading; 1081's own headline is *"≈ +21 MB"*) |

Against 1081's converged PIE headroom of **394–493 MiB free** on the 8151 MiB card (91.9 % occupied at peak, banner already firing at baseline): the swap spends **3–5 % of the headroom**. It fits. Now (editor up, no PIE, no Fab browser): `nvidia-smi` 2357 MiB used / 5535 free — a different machine state from 1081's 6455 idle, which is exactly 1081 §3's point that the banner measures machine contention, not trees.

### 2e. The render cost the MB does not show — **1 LOD vs 5, across 340 instances, in numbers**
`get_lod_thresholds`: Highpoly = `[2]` (LOD0 only — no fall-off, ever, until the 24–32 k uu cull). Mobile = 5 LODs, screen-size thresholds `[2, 0.079–0.119, 0.026, ~0.002, ~0.0015]`.
LOD0 triangles, sum of 12: **Highpoly 89,695 (avg 7,475)** vs **Mobile 27,148 (avg 2,262)**; Mobile's chain halves per step to ~140 at LOD4.
⇒ Tree-layer worst case (all 340 at LOD0): **2.54 M tris vs 0.77 M — 3.3×**; across the cull band Mobile drops to LOD1–2 while Highpoly cannot, so the *typical* gap is larger (≈ 4–9× depending on FOV; derivation: UE screen-size ≈ 2·ScreenMultiple·radius/distance with radius ≈ 1.1 k uu). Not a VRAM number — a per-frame draw cost. **`TASK-1085` measures it; nobody should quote it as free.** Masked-foliage overdraw (the usual foliage cost) is the same for both sets — same silhouettes.

### 2f. The dangling refs — **CONFIRMED from the engine's asset registry** (1081 §4 / 1082 verified a third way)
`get_dependencies`:
```
M_Pack1_Leaf_Mobile   -> /Game/Tree_Pack_1/Textures/T_Leaf_Pack1, /Game/Tree_Pack_16/Textures/dgd
M_Pack1_Trunk_Mobile  -> /Game/Tree_Pack_1/Textures/T_Trunk_Pack1, /Game/Tree_Pack_16/Textures/fg
M_Pack1_Leaf          -> T_Leaf_Pack1, T_Leaf_Pack1_normal
M_Pack1_Trunk         -> T_Trunk_Pack1, T_Trunk_Pack1_normal
```
`exists()`: `/Game/Tree_Pack_16` **false** · `…/dgd` **false** · `…/fg` **false** · both real normals **true**.
⚠️ Stated precisely: this session's engine log has **no** `Tree_Pack_16` line — the Mobile materials have not been loaded in PID 20564 yet (no level/scatter loaded by anyone), so the `LogScript: Asset does not exist` warning 1081 quoted has not had a chance to fire. The registry read above is the confirmation; the log will repeat it the first time `L_Arena` loads.

---

## 3. 🧑 THE OPTIONS, WITH NUMBERS — his call (`FIELD-§3` cl. 3)

| | A — no roster change | B — Highpoly swap (12) | C — both families (24) | D — repoint the 2 Mobile refs (vendor edit) |
|---|---|---|---|---|
| what changes on the field | nothing from this lane; *"too dark"* moves to a **lighting/atmosphere row** (dusk sun bearing, SkyLight, height fog, exposure) | battlefield trees get **real normal maps** + no dangling refs; same 12 silhouettes | B, plus the 12 Mobile duplicates alongside | Mobile materials gain the normals **in place** |
| tone effect | none from assets | **hypothesis, unproven** (1081 §4/§6b) — he may see no difference | half of B's effect (half the instances stay Mobile) | same hypothesis as B |
| variety | none available (12 designs is the pack) | none — same 12 designs | **none** — duplicates (IoU 0.81) | none |
| VRAM Δ | **0** | **+14…+21 MB** of ~394 MiB | **+15…+22 MB** | **+13…+15.5 MB** (normals resident) |
| draw cost | 0 | **3.3× tree tris worst case, no LOD fall-off** | ~2.2× (half the fleet at 1 LOD) | **0** — keeps 5 LODs |
| touches the praised background trees? | a lighting row **lights them too** (`FIELD-§4` hazard) | **NO** — DA cannot reach them | NO | **YES** — they use the same two materials (`overrideMaterials` empty ×15) |
| fences | clean | `DA_BattlefieldScatter` only (my WRITES) | same | ⛔ **vendor asset — forbidden without his fresh ruling** (`FIELD-§3`) |
| reversibility | — | 12 array entries; sheet 12–23 already documents the way back | 12 removals | material edit revert |
| sheet | 1082's stays provisional; a final sheet of 0–11 = Mobile must be issued | 1082's indices **0–11 already match**; re-issue without 12–23 | 1082's 0–23 already match; re-issue with the banner changed | as A |

**What he loses either way, in one line each:**
- **A:** the only asset-side lever on tone is left unused, and the fix moves to lighting — which also re-lights the trees he said he likes.
- **B:** ~5 % of a headroom that is already overdrawn plus a 3.3× tree-triangle load with no LOD fall-off, for a tone change nobody has proven he will see.
- **C:** B's costs for half of B's effect and zero variety — 1082's *"do not roster both"* stands; listed for completeness.
- **D:** needs a vendor-edit licence and changes the background trees' materials too; if he licenses it, it is B's normal maps at A's draw cost.

**If he wants genuine variety** (a second light species), neither folder can supply it — that is a `FAB-###` request (1082 §Notes), not a roster edit.

### Ready-to-execute values for B (so his "yes" is a 10-minute row)
`Layers[0].Meshes` = `/Game/Tree_Pack_1/Meches/Highpoly_Tree_1/SM_Highpoly_Tree_{1..12}` in numeric order (idx 0 → `_1` … idx 11 → `_12`, = 1082's sheet 0–11 unchanged) · `ZOffset 0` (derived §2a) · `FootprintRadius 150` (preserved §2c) · `CollisionProxyMesh Cylinder` / `(1.4,1.4,17)` / `850` (§2b) · `InstanceCount 340` · `ScaleRange (0.8,1.2)` · `MinSpacing 300` · `bBlocking true` · `RegionBias EdgeBias` · `Cull 24000/32000` · `bCastShadows true` · `bAllowOnHills true` · `SymmetryMode` untouched. Then `save_assets(["/Game/Data/DA_BattlefieldScatter"])` by explicit path, re-read from the engine, quote the array, re-issue the sheet from `Saved/TreeSurvey/sm_highpoly_tree_*.txt` (1082's base64 captures, still on disk, 52 files).

---

## 4. FOR `TASK-1084` / `TASK-1085` REGARDLESS OF THE RULING
- The DataAsset is **unmodified and clean** (`is_dirty=false`, on-disk sha256 `0f23576f…dc64`, last commit `ea2a70f`). `TASK-1084` may not start until this row **saves and closes** (`FIELD-§6`) — if his ruling is **A**, this row closes with no write and 1084 is dispatchable immediately.
- `TASK-1085`'s after-reading must hold machine state constant (1081 §3): today's idle reads span 2357 MiB (now) vs 6455 MiB (1081's morning) on the same content.
- The 1414² uncompressed trunk pair (~12 MiB recoverable) is still a vendor-texture change needing his licence.

## 5. FOR THE MANAGER
Three things already boarded by 1081 stand. New from this row: **(i)** `FootprintRadius` on a proxy layer is a trunk clearance — `FIELD-§2`'s *"prefer auto for a mixed roster"* would sparsify the field by 2–4× centre spacing if applied here (§2c); worth a clause so a later row does not apply it by reflex. **(ii)** Option D is the cheapest normal-map route but is a vendor edit that also re-materials the praised trees — it needs his explicit word on both counts.

## 6. FENCES — ALL HELD (verified after the last engine call)
- ⛔ `save_assets` **never called** in any form; `set_properties` never called; no save prompt seen.
- ✅ `Content/Maps/L_Arena.umap` sha256 **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** before **and** after (1081's value) · `is_dirty(L_Arena)=false`.
- ✅ `Content/Data/DA_BattlefieldScatter.uasset` sha256 **`0f23576f432aed1905b16f4896eb612a2cd9faad47d17e2b369663ffa886dc64`** before and after · `is_dirty=false`.
- ✅ `git status --short Content/Maps/ Content/Data/` = **empty** before and after. Nothing staged. No `checkout`/`restore`/`stash`/`reset`/`clean`, no commit, no push.
- ✅ Vendor packs read-only — queried only. The art-director's `/Game/Characters`, `/Game/Textures`, `/Game/Materials/Instances` — untouched, unlisted, unsaved.
- ✅ Editor **left as found**: PID 20564, MCP up, no PIE started, no level loaded by me.
- Written by this row: **this file only.** Board: my `- status:` line only, via `Edit`.

---
---

# RULING + EXECUTION (2026-09-06, ~09:00 PDT onward)

## 7. THE AUTHORIZATION RECORD — verbatim, as relayed by the orchestrator

Jonathan, in the orchestrator terminal (the terminal is authorization; Slack never is):
> **"I approve you to go with your recommendation."**

The recommendation he approved, verbatim from the orchestrator's message to him:
> **"Revised recommendation: D first (it fixes a genuine pack defect at its source; needs your explicit OK because vendor packs are read-only by law), then your eye; A (re-light the dusk) stays your design call."**

⇒ **D is authorized INCLUDING the vendor-content edit; A is deferred (not boarded); B and C are rejected. NO roster change.**

**Where the authorization is recorded (cite, not restate):**
- **`TASKBOARD.md` line 240, the `- ruling:` line of this row** — scope verbatim: *"⛔ EXACTLY TWO texture references: … `M_Pack1_Leaf_Mobile.uasset` **normal slot** `/Game/Tree_Pack_16/Textures/dgd` → `…/T_Leaf_Pack1_normal`, and `…/M_Pack1_Trunk_Mobile.uasset` **normal slot** `…/fg` → `…/T_Trunk_Pack1_normal` … ⛔ Nothing else under `Content/Tree_Pack_1/**` is touched — **not a parameter, not a node**, not a re-save of any sibling asset."* Acceptance shape: same-vantage BEFORE/AFTER tone as a number + engine read-back that `/Game/Tree_Pack_16` appears in NO dependency list; *"no measurable change"* is a valid outcome.
- **`CONVENTIONS.md` `FIELD-§3`, line 10348 — the dated RECORDED EXCEPTION** (*"TWO texture references in TWO materials, ⛔ defect-repair ONLY … ⛔ Nothing else under the pack is licensed by this entry — not a parameter, **not a node**, not a texture"*), quoting the same two sentences of his.

⚠️ **Both records presuppose a "normal slot" that §8 shows does not exist, and both explicitly exclude "a node."** That is why §10's D1 — the only variant that gives the Mobile materials normals — is **outside the authorization as recorded** and needs a fresh word, while D2 (re-save of exactly those two files, no other change) arguably stays inside it. Neither is mine to pick.
The orchestrator's execution spec for D: *repoint* the `TextureSample` in `M_Pack1_Leaf_Mobile` that references the missing `/Game/Tree_Pack_16/Textures/dgd` to `T_Leaf_Pack1_normal`, and the one in `M_Pack1_Trunk_Mobile` referencing `…/fg` to `T_Trunk_Pack1_normal`; **first read the graph and record what each dangling node is wired to and its sampler type; if the node is NOT wired to `MP_Normal`, do not rewire on my own judgement — report the exact graph state, set blocked-with-question, stop.**

## 8. 🚨 THE GRAPH STATE — THE NODE TO REPOINT DOES NOT EXIST

Read from the live engine (PID 20564) with `MaterialTools.get_expressions` + `get_property_input` + `ObjectTools.get_properties` on every node, for all four pack materials:

| material | expression nodes (ALL of them) | `MP_Normal` | `MP_BaseColor` | other outputs |
|---|---|---|---|---|
| **`M_Pack1_Leaf_Mobile`** | `TextureSample_0` = `T_Leaf_Pack1`, `SAMPLERTYPE_Color` · `Constant_0` = 0 · `Constant_1` = 1 | **← None (unconnected)** | `TextureSample_0.RGB` | OpacityMask ← `TextureSample_0.A` · Roughness ← `Constant_1` · Specular/Metallic ← `Constant_0` |
| **`M_Pack1_Trunk_Mobile`** | `TextureSample_0` = `T_Trunk_Pack1`, `SAMPLERTYPE_Color` · `Constant_0` = 0 · `Constant_1` = 1 | **← None (unconnected)** | `TextureSample_0.RGB` | Roughness ← `Constant_1` · Specular/Metallic ← `Constant_0` |
| `M_Pack1_Leaf` (Highpoly) | `TextureSample_0` = `T_Leaf_Pack1`, Color · **`TextureSample_1` = `T_Leaf_Pack1_normal`, `SAMPLERTYPE_Normal`** | **← `TextureSample_1.RGB`** | `TextureSample_0.RGB` | OpacityMask ← `.A` · Specular ← `.B` |
| `M_Pack1_Trunk` (Highpoly) | `TextureSample_0` = `T_Trunk_Pack1`, Color · **`TextureSample_1` = `T_Trunk_Pack1_normal`, `SAMPLERTYPE_Normal`** | **← `TextureSample_1.RGB`** | `TextureSample_0.RGB` | Roughness ← `.G` · Specular ← `.B` |

**⇒ Neither Mobile material contains ANY node that references `/Game/Tree_Pack_16/…`, and neither has a normal-map node at all.** The registry's dangling dependency is a **package-level stale import**, not a graph node.

**On-disk corroboration** (name-table scan of the `.uasset` files, read-only): the Mobile packages carry the names `/Game/Tree_Pack_16/Textures/dgd` + `dgd.dgd` (resp. `fg`/`fg.fg`) — an object-import entry — but **no `SAMPLERTYPE_Normal`, no `Normal`, no `*_normal` texture name** anywhere; the Highpoly package carries all three (`SAMPLERTYPE_Normal`, `Normal`, `T_Leaf_Pack1_normal`). The Mobile materials **never serialized a normal sampler.** How the pack author left a dead import behind (a deleted node's residue, a copy from their `Tree_Pack_16` working project) is not measurable from here and I am not guessing.

**Two consequences that change what D means:**
1. **The "broken refs" cannot be the cause of the darkness.** The Mobile shaders compile with **no normal input** (vertex normals only); a dead import the loader warns about and drops changes nothing in the shader. The warning is real; the *rendering* defect it was suspected of causing is not there.
2. **"Repoint" has nothing to act on.** Giving the Mobile materials normal maps means **ADDING** a `TextureSample` node (`SamplerType = Normal`) and **connecting** it to `MP_Normal` — a two-step graph addition per material, exactly mirroring the vendor's own Highpoly graphs. That is a rewire, which the spec reserved. **Stopped.**

## 9. THE ROSTER — FINAL, UNCHANGED (cl. 5 / cl. 6 discharged under the ruling)

`Layers[Trees].Meshes` as read back from the engine (§1, verbatim) **is the shipped array**; no DataAsset write was made (`is_dirty=false`, sha `0f23576f…dc64`). `ZOffset 0` and `FootprintRadius 150` are **deliberate** (§2a, §2c). The collision-proxy triple is untouched.

**Index → path (the only tree layer table anyone needs):**

| idx | asset path | CTI (1082) | LOD0 tris | LODs | MB |
|---:|---|---:|---:|---:|---:|
| 0 | `/Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1` | 24.23 | 2,458 | 5 | 0.134 |
| 1 | `…/SM-Mobile_Tree_2` | 30.80 | 2,025 | 5 | 0.107 |
| 2 | `…/SM-Mobile_Tree_3` | 28.36 | 2,115 | 5 | 0.118 |
| 3 | `…/SM-Mobile_Tree_4` | 27.94 | 2,137 | 5 | 0.113 |
| 4 | `…/SM-Mobile_Tree_5` | 27.57 | 2,283 | 5 | 0.119 |
| 5 | `…/SM-Mobile_Tree_6` | 27.51 | 2,422 | 5 | 0.133 |
| 6 | `…/SM-Mobile_Tree_7` | 29.85 | 2,249 | 5 | 0.114 |
| 7 | `…/SM-Mobile_Tree_8` | 28.92 | 2,213 | 5 | 0.115 |
| 8 | `…/SM-Mobile_Tree_9` | 28.83 | 2,360 | 5 | 0.131 |
| 9 | `…/SM-Mobile_Tree_10` | 27.30 | 2,202 | 5 | 0.129 |
| 10 | `…/SM-Mobile_Tree_11` | 26.91 | 2,291 | 5 | 0.135 |
| 11 | `…/SM-Mobile_Tree_12` | 28.19 | 2,393 | 5 | 0.138 |

Rule: **index i ⇔ `SM-Mobile_Tree_{i+1}`**. 🧑 *"drop 7"* = remove index 7 = `SM-Mobile_Tree_8`, one array removal.

**Sheet re-issued:** `.claude/pipeline/playtest-evidence/2026-09-06/RosterSheet_Trees.png` (1470×900, sha256 `49d1631d…`) — 12 tiles, 0–11, from 1082's live thumbnail captures (`Saved/TreeSurvey/sm-mobile_tree_N.txt`), each captioned index + asset name + CTI + LOD0 tris + LODs + MB, the path rule and proxy triple printed on the sheet, and the provisional sheet's 12–23 explicitly declared non-existent. Every printed index matches the read-back array. Generator: session scratchpad `sheet_final.py` (tiling is by construction `idx i ← sm-mobile_tree_{i+1}.txt`).

## 10. 🧑 THE QUESTION — which D is D? (numbers; nothing written)

| variant | what it is | look | VRAM | draw | touches the 15 praised background trees | fences |
|---|---|---|---|---|---|---|
| **D1 — ADD the normals** (what D *meant*) | per material: add 1 `TextureSample` (`T_Leaf_Pack1_normal` / `T_Trunk_Pack1_normal`, `SAMPLERTYPE_Normal`) + connect `RGB → MP_Normal`; recompile; save the 2 materials by path. Mirrors the vendor's Highpoly graphs node-for-node — no design of mine. | the original tone hypothesis, **still unproven** | **+13…+15.5 MB** (two normals resident) | **0** (5 LODs untouched) | **YES** — same two materials, `overrideMaterials` empty ×15; at 20–47 k uu normal detail is sub-pixel, so the expected change there is small, but it is a change to what he praised | vendor edit — **covered by his D authorization** if the orchestrator reads "repoint" as "give them the normals" |
| **D2 — re-save only** | open + save both materials unchanged; the rebuilt import map should drop `Tree_Pack_16/*` (verify by `get_dependencies` read-back) | **none** | 0 | 0 | no (materials render identically) | vendor write, pure hygiene — silences the load warning, nothing else |
| **D0 — nothing** | record §8 and close | none | 0 | 0 | no | — |

**Honest framing for his eye:** the "genuine pack defect at its source" is a **dead import**, not a mis-pointed normal. Fixing the defect (D2) does not touch the look; changing the look (D1) is an addition the pack never had. **Only D1 tests the tone hypothesis;** if he wants the test, it is D1, and *"no measurable change"* remains a valid outcome that hands A back to him.

**Build-master recommendation, labelled as such:** D1. It is what the ruling was reaching for, it copies the vendor's own graph, and it is the only variant that produces evidence. Before/after evidence plan if D1 is ruled: same-vantage burst captures at the 1081 lane vantage — **camera `(-24000, 0, 1400)`, pitch −4, yaw 0** (recovered from 1081's session transcript: its first `captureTransform`, PIE `PlayMode_InViewPort`) — plus the mid-field `(-6000, -2000, 1600)` pitch −6 yaw 75; canopy tone on a fixed ROI by 1082's CTI method (mean Rec.709 linear luminance of sRGB-linearised foreground pixels ×100); promoted under the **pinned names** (byte-for-byte, per the manager's TASK-1083 WRITES / TASK-1085 STAGES / TASK-1086 INPUTS): `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1083-D-BEFORE.png`, `…/TASK-1083-D-AFTER.png`, plus the unpinned `…/TASK-1083-D-SIDE-BY-SIDE.png` (1085 stages it on this handoff's word). **None of the three exists yet** — no capture was taken because D did not execute; nothing to rename. The ruling's read-back clause (`/Game/Tree_Pack_16` in NO dependency list) is satisfied by D1 or D2 and verified by `get_dependencies` after the save. Also the thumbnail-level CTI on `SM-Mobile_Tree_2`/`_8` before vs after (deterministic renderer, Δ=0.0 repeatability per 1082) as the controlled instrument.

## 11. FOR `TASK-1084` / `TASK-1085` (unchanged by this section)
- `TASK-1084`'s blocker is **discharged**: this row wrote **nothing** to `DA_BattlefieldScatter` under any D variant. It may start on the orchestrator's word.
- `TASK-1085` (host): pathspec must include `.claude/pipeline/playtest-evidence/2026-09-06/RosterSheet_Trees.png` + this handoff; **if D1 executes**, also `TASK-1083-D-BEFORE.png`, `TASK-1083-D-AFTER.png`, `TASK-1083-D-SIDE-BY-SIDE.png` in the same folder; **if D1/D2 executes**, also `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset` + `M_Pack1_Trunk_Mobile.uasset`, and the commit message must name the **vendor edit** and **Jonathan's authorization** (§7 verbatim).

## 12. FENCES — RE-VERIFIED AFTER THE LAST ENGINE CALL
- ⛔ `save_assets` never called; `set_properties` never called; no `connect_*`/`add_expression`/`recompile` called; no vendor write of any kind.
- ✅ `L_Arena.umap` sha `1f78419d…5622` unchanged · `DA_BattlefieldScatter.uasset` sha `0f23576f…dc64` unchanged · both Mobile material `.uasset` files **byte-identical to HEAD, verified by LFS oid** (the memory law: oid-vs-sha256, never size or the pointer's hash): working-tree sha256 `4197c79b…901ca` (`Leaf_Mobile`, 123,720 B) and `30427f3f…5909` (`Trunk_Mobile`) equal the oids inside the HEAD LFS pointers exactly; `git status` clean on both. These are the BEFORE baselines for whoever executes D.
- ✅ `git status --short Content/` = only the art-director's TASK-1093 paths (listed, not touched).
- Editor left as found: PID 20564, MCP up, `L_Arena` is the loaded level (already was), PIE never started by me.
- Written by this section: this file, the re-issued sheet (a WRITES-list item), my board status line.

---
---

# §13. D1 EXECUTION (2026-09-06, ~09:45–09:45 PDT ruling → 09:25–09:42 execution window)

## 13.1 The authorization record — verbatim

🧑 Jonathan, orchestrator terminal, 2026-09-06 ~09:45 PDT (the terminal is authorization; Slack never is):
> **"ok, I will go with your recommendation, D1"**

The scope he approved, as the orchestrator put it to him and relayed to me:
> **"add ONE `TextureSample` per material pointing at the pack's own existing normal texture (`T_Leaf_Pack1_normal` for `M_Pack1_Leaf_Mobile`, `T_Trunk_Pack1_normal` for `M_Pack1_Trunk_Mobile`), `SAMPLERTYPE_Normal`, wired `RGB → MP_Normal` — node-for-node the vendor's own Highpoly graph; +13–15.5 MB, zero draw cost; the 15 background trees re-material as a known, accepted consequence. D2's cleanup (the dead `Tree_Pack_16` imports dropping on save) comes free in the same save — verify it by read-back, don't do anything extra for it."**

Recorded at: **`TASKBOARD.md` line 240, this row's `- ruling:` line**, and **`CONVENTIONS.md` `FIELD-§3` line 10348 (the dated recorded exception)** — both being amended by the manager from *"not a node"* to this D1 scope concurrently with this execution; cite by location.

## 13.2 What was done — exactly the scope, nothing else

Per material, via `MaterialTools`: `add_expression(MaterialExpressionTextureSample)` → `set_properties(Texture, SamplerType=SAMPLERTYPE_Normal)` → `connect_to_output(RGB → MP_Normal)` → `recompile` (raised nothing ⇒ shaders compiled). No parameter, constant, or other node touched; no other output re-wired.

**Node-level read-back from the live asset AFTER the edit, BEFORE the save (engine query, not my edit list):**

| material | nodes (all) | `MP_Normal` | unchanged outputs (pre == post) |
|---|---|---|---|
| `M_Pack1_Leaf_Mobile` | `TextureSample_0` = `T_Leaf_Pack1` / Color · `Constant_0` = 0 · `Constant_1` = 1 · **`TextureSample_1` = `/Game/Tree_Pack_1/Textures/T_Leaf_Pack1_normal` / `SAMPLERTYPE_Normal`** | **← `TextureSample_1.RGB`** (was None) | BaseColor ← `TS_0.RGB` · OpacityMask ← `TS_0.A` · Roughness ← `Constant_1` · Specular/Metallic ← `Constant_0` |
| `M_Pack1_Trunk_Mobile` | `TextureSample_0` = `T_Trunk_Pack1` / Color · `Constant_0` = 0 · `Constant_1` = 1 · **`TextureSample_1` = `/Game/Tree_Pack_1/Textures/T_Trunk_Pack1_normal` / `SAMPLERTYPE_Normal`** | **← `TextureSample_1.RGB`** (was None) | BaseColor ← `TS_0.RGB` · OpacityMask None · Roughness ← `Constant_1` · Specular/Metallic ← `Constant_0` |

Pre-edit node count 3 → post-edit 4, both. This is the Highpoly graph's normal branch, node-for-node.

## 13.3 Save + dependency read-back (`SC-§94` cl. A)

`save_assets(["/Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile", ".../M_Pack1_Trunk_Mobile"])` — **by explicit path, exactly two; `save_assets([])` never called.** Returned true.
- `is_dirty`: both **true → false** across the save. `DA_BattlefieldScatter`, `L_Arena`, `M_Pack1_Leaf`, `M_Pack1_Trunk` all **false** before and after (nothing else dirtied).
- `get_dependencies` after save, re-read again at close-out:
  - `M_Pack1_Leaf_Mobile` → `[/Game/Tree_Pack_1/Textures/T_Leaf_Pack1, /Game/Tree_Pack_1/Textures/T_Leaf_Pack1_normal]`
  - `M_Pack1_Trunk_Mobile` → `[/Game/Tree_Pack_1/Textures/T_Trunk_Pack1, /Game/Tree_Pack_1/Textures/T_Trunk_Pack1_normal]`
  - **`/Game/Tree_Pack_16/*` appears in NO dependency list** (D2's cleanup came free in the save, as predicted); `exists("/Game/Tree_Pack_16")` = false.
- **On disk:** `git status --short Content/Tree_Pack_1/` = **exactly two lines** (`M` on both files). New sha256 `58957692…7283` (Leaf, 20,939 B) and `977b4b13…13d2` (Trunk, 23,457 B) vs the HEAD LFS oids `4197c79b…` (123,720 B) / `30427f3f…` (165,602 B).
  ⚠️ **The files shrank ~6× — explained, benign:** the vendor packages were **UE 4.26 saves** (`++UE4+Release-4.26` in the old name table) carrying an embedded PNG thumbnail (1 PNG signature in the old file, 0 in the new) and the 4.26 `MaterialCachedExpressionData` / **`ReferencedTextures`** block — which is where the dead `Tree_Pack_16/dgd` reference lived (old-only names: `ReferencedTextures`, `/Game/Tree_Pack_16/Textures/dgd`). The new files are UE 5.8 saves with no embedded thumbnail and 5.8's cached-data layout. Graph content is verified by the engine read-back above, not by size.

## 13.4 Evidence — same vantage, burst captures, tone as a number

**Vantage:** 1081's lane vantage, camera **`(-24000, 0, 1400)`, pitch −4, yaw 0**, PIE `PlayMode_InViewPort`, FOV 90, frame **2764×828** (identical to `TASK-1081-lane-vantage.png`). Read back from every capture: camera == requested.
**PIE discipline (shared editor):** the engine log's `LogPlayLevel` timeline — 09:19:27–09:25:33 another agent's session · **09:26:40–09:28:08 mine (BEFORE burst)** · 09:29:47 the TASK-1084 build-master's session, polled every 30 s (`true` ×8 across two windows) and **never stopped by me** · it ended 09:41:29 · **09:41:49–09:42:01 mine (AFTER burst)**, stopped by me, `IsPIERunning` read back false. Each of my sessions lasted only warm-up + the six-frame burst.
**Convergence (`SC-§88`):** six frames 1.5 s apart per burst; consecutive-frame mean |Δ| fell monotonically BEFORE 1.07 → 0.73 → 0.45 → 0.38 → 0.32 /255 and AFTER 1.08 → 0.62 → 0.44 → 0.39 → 0.34 /255; frame 5 of each burst is the reported one.
**Instrument:** canopy masks **fixed from the BEFORE frame** (HSV hue 40–110°, S ≥ 0.15, V ≥ 0.05 — foliage-coloured pixels; sky/wall/orange horizon excluded by hue) inside three rectangles above the hill line + the whole tree-line band; metric = **mean Rec.709 linear luminance of sRGB-linearised masked pixels ×100** (1082's CTI units), same pixels before and after. **Noise floor** = the same metric between consecutive converged frames of one burst.

| ROI (full-res rect) | masked px | CTI BEFORE | CTI AFTER | Δ | rel. | noise floor (4→5) | Δ / noise |
|---|---:|---:|---:|---:|---:|---:|---:|
| L dark cluster `(330,0,1000,185)` | 56,992 | 3.662 | 3.675 | **+0.013** | +0.35 % | −0.029 / −0.030 | 0.4× |
| C castle pair `(1410,0,1680,185)` | 16,303 | 4.404 | 4.523 | **+0.119** | +2.7 % | −0.044 / −0.018 | 2.7× |
| R gold pair `(1725,0,2070,185)` | 24,725 | 4.746 | 4.782 | **+0.037** | +0.8 % | −0.044 / −0.034 | 0.8× |
| **ALL tree line `(0,0,2764,185)`** | 111,339 | **4.160** | **4.242** | **+0.082** | **+2.0 %** | −0.034 / −0.030 | 2.4× |

Mean sRGB of the tree line: (58.3, 55.4, 35.7) → (59.6, 55.8, 35.8). **The canopies at this vantage sit at CTI ≈ 4 — near-black — before and after.**

**The change map (proof the edit is live in the render, and that nothing else moved):** pixels changed by > 4/255 — **70,258 in the tree-line band**, 47,531 in rows 200–400 (the ring trees' trunks and lower canopies and the near-right tree), then 7,354 → 645 per 100-row band down the field; **open ground `(1000,500,1800,800)`: 0 of 240,000 changed**; the two big rocks in identical positions (mean |Δ| 1.1–1.4/255 = temporal AA). ⇒ the scatter layout was the same in both sessions (no per-match confound at this vantage), and the only objects that changed are the ones carrying the two edited materials. ⚠️ Stated plainly (1081 §5 stands): **no battlefield scatter tree is imaged at this vantage** — every canopy in frame is a hand-placed ring tree on the same two materials; the physics of the change is identical, but his eye on the field trees is `TASK-1086`'s.

**Controlled instrument (deterministic thumbnail rig, no lighting/atmosphere):** `CaptureAssetImage` on three rostered meshes, canopy mask fixed from BEFORE, retaken ×2 after the edit (retakes byte-identical for `Tree_8`/`Tree_12`, and identical to each other for `Tree_2`). Cross-session control: 1082's 03:23 renders of `Tree_8`/`Tree_12` match my 09:25 BEFORE renders to 0.50/255 mean with only 46/54 pixels differing by > 2, so 1082's `Tree_2` render is a valid BEFORE baseline (my own 09:25 `Tree_2` capture came back on a different rig — grey ground plane, cast shadow — and is **excluded as invalid**, montage kept in scratch).

| mesh | CTI BEFORE | CTI AFTER | Δ |
|---|---:|---:|---:|
| `SM-Mobile_Tree_2` (1082 baseline) | 27.88 | 27.03 | **−0.85** (−3.0 %) |
| `SM-Mobile_Tree_8` | 27.13 | 25.97 | **−1.16** (−4.3 %) |
| `SM-Mobile_Tree_12` | 28.65 | 27.59 | **−1.06** (−3.7 %) |

Under a uniform rig light a perturbed normal turns some facets away from the light: slightly darker, as expected. Under the dusk sun: +2 %. **Both instruments agree in magnitude — a few percent — and disagree in sign; neither is a change a person sees.**

### ⇒ VERDICT: **D1 is real, live, and correct — and it does not move the tone.**
*"No measurable change"* is the outcome the ruling declared valid. The battlefield trees read near-black at CTI ≈ 4 because of the dusk **lighting**, exactly 1081 §2's conclusion; the material now has the normal maps the pack meant it to have, the dead imports are gone, and **Option A (re-light the dusk) goes back to 🧑 him with these numbers at `TASK-1086`.**

**Evidence files (pinned names, byte-for-byte):**
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1083-D-BEFORE.png` — sha256 `dbb668fc…`, 2764×828, BEFORE burst frame 5
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1083-D-AFTER.png` — sha256 `10287237…`, AFTER burst frame 5
- `.claude/pipeline/playtest-evidence/2026-09-06/TASK-1083-D-SIDE-BY-SIDE.png` — sha256 `c13a935c…`, before / after / |Δ|×8 with the ROI boxes and the numbers printed
- `.claude/pipeline/playtest-evidence/2026-09-06/RosterSheet_Trees.png` — sha256 `49d1631d…` (§9)
Scripts (session scratchpad): `tone_thumb.py`, `pie_tone.py`, `sheet_final.py`; raw base64 captures in `Saved/TreeSurvey/d1_*` (gitignored).

## 13.5 For `TASK-1085` (host) and `TASK-1086` (his eye)
- **Pathspec:** `Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset` + `M_Pack1_Trunk_Mobile.uasset` (the vendor edit, LFS) · the four evidence PNGs above · this handoff. `git status --short Content/Tree_Pack_1/` must read exactly those two lines at staging time.
- **Commit message must name the vendor edit and quote his authorization:** *"ok, I will go with your recommendation, D1"* (and the D1 scope sentence of 13.1).
- `DA_BattlefieldScatter.uasset` is **not** in this row's pathspec — sha `0f23576f…dc64` untouched by me; the TASK-1084 build-master owns any change to it.
- `TASK-1086`: judge `TASK-1083-D-BEFORE.png` vs `-AFTER.png`; the number says +2 %; the decision left open is **A** (dusk lighting), his design call.

## 13.6 Fences — verified after the last engine call
- ✅ `L_Arena.umap` sha **`1f78419d…5622`** before and after (never saved; `is_dirty` false). ✅ `DA_BattlefieldScatter.uasset` sha **`0f23576f…dc64`** untouched by me (`is_dirty` false at close-out).
- ✅ `git status --short Content/Tree_Pack_1/` = **exactly two lines.** `Content/` otherwise shows only the art-director's TASK-1093 paths (`SK_MainCharacter`, `MI_MainCharacter_PBR`, `T_MainCharacter_*`, `RawAssets/*`) — listed, not touched, not staged.
- ⛔ `save_assets([])` never called; no save prompt; no commit; no push; no `checkout`/`restore`/`stash`/`reset`/`clean`.
- ✅ PIE: only my two short sessions started/stopped by me; the other agent's session never stopped.
- ✅ Editor left up: PID 20564, MCP answering, `L_Arena` loaded, PIE off, nothing of mine dirty.
- ⚠️ Instrument note for whoever scripts long MCP calls in a shared editor: other agents' error text can displace a long script's return value (happened twice); write the result to a file in `Saved/` inside the script and read it back from disk.
