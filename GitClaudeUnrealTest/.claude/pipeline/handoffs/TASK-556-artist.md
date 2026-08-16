# TASK-556 — [WR-2] `SM_Torch` + `SM_WarTable`, two procedural props — art-director handoff

**Status:** ✅ **AUTHORED + EXPORTED + EYEBALL-GATED. HEADLESS THROUGHOUT.**
**Date:** 2026-08-15 · **Branch:** `main`

⛔ **NO Unreal Editor, NO Unreal MCP, NO PIE, NO compile, NO Git.** ⛔ **NO Meshy / TRELLIS call, NO credits, NO concept render, NO texture bake.**
⛔ **`Tools/ArtPipeline/pipeline_manifest.json` NOT TOUCHED** — TASK-555 owns it in the same wave, and these two props are deliberately **manifest-free** (see §6). Verified: the only manifest diff on disk is TASK-555's `assets.Castle` edit.
⛔ **No existing mesh / texture / material / `.cpp` / `L_Arena` touched.**

---

## 1. Deliverables

| Path | Bytes | What |
|---|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Torch.fbx` | 27,612 | FBX node **`SM_Torch`**, slots `[TorchBody, TorchFlame]` |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\WarTable.fbx` | 30,460 | FBX nodes **`SM_WarTable`** (slot `[WarTable]`) + **`UCX_SM_WarTable_00`** |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\build_warroom_props.py` | — | The generator. Deterministic, re-runnable, ~6 s, zero credits |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\Cache\WarRoomProps\props_report.json` | — | Every number below, machine-readable |
| `...\Cache\WarRoomProps\previews\*.png` | — | 16 renders: orthos, vertex-colour passes, **scale-vs-human** shots |

**Reproduce:**
```
"C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" --background --factory-startup \
    --python-exit-code 1 --python Tools/ArtPipeline/build_warroom_props.py
```
Blender 5.1.2 (`ec6e62d40fa9`). Exit 0, no warnings, `props_report.json` `"warnings": []`.

---

## 2. ⛔ MEASURED READBACKS — every figure below is read back off the mesh, none is asserted

### `SM_Torch`

| thing | measured | law / budget |
|---|---|---|
| bounds (local, uu) | min `[0.00, −25.00, −32.00]` → max `[75.00, 25.00, 104.00]` | — |
| **dims (uu)** | **`75 × 50 × 136`** | — |
| **tris / verts** | **324 / 172** | ⛔ budget **≤600** — **46 % spent** |
| flame tris (slot 1) | 80 of the 324 | — |
| **UV layer** | **`UVMap`** (exactly one) | `UVMap` law (TASK-038) |
| **material slots, in order** | **`0 = TorchBody`, `1 = TorchFlame`** | `WR-§4` |
| **origin — min X** | **`0.0000`** ⇒ the origin plane **IS the wall-mount face** | ⛔ `WR-§4` / TASK-562 |
| origin — Z | `−32 … +104`; **Z 0 = the vertical centre of the backplate** | a wall point, not a floor point |
| **flame bbox (uu)** | min `[35.00, −14.27, 46.00]` → max `[65.00, 14.27, 104.00]` | — |
| ⭐ **`flame_centre_uu`** | ⭐ **`(50.0, 0.0, 75.0)`** | ⭐ **TASK-558 §7 has a named landing site for this — see §5** |
| vertex colour | `Col`, `CORNER`, **values exactly `{0.0, 1.0}`**; 60 of 696 loops wood (`R=0`) | iron/wood mask, §3 |
| Y-symmetry | **172/172 verts match after mirror_Y, 0 unmatched** | handedness trap cannot bite |
| Nanite | **OFF** (import setting — named in §5) | `WR-§4` |

### `SM_WarTable`

| thing | measured | law / budget |
|---|---|---|
| bounds (local, uu) | min `[−190.00, −125.00, 0.00]` → max `[190.00, 125.00, 110.00]` | — |
| **dims (uu)** | **`380 × 250 × 110`** | — |
| **tris / verts** | **496 / 272** | ⛔ budget **≤800** — **62 % spent** |
| **UV layer** | **`UVMap`** (exactly one) | `UVMap` law |
| **material slot** | **`0 = WarTable`** (exactly one) | ⛔ spec (2) |
| **origin — min Z** | **`0.0000`** ⇒ the origin plane **IS the floor-contact plane** | ⛔ spec (2); **TASK-559 §"→ TASK-556" depends on this and it is MET** |
| parchment surface height | **95 uu**; rim top **110 uu** | §4 |
| half-diagonal (XY) | **227.43 uu** | vs `InteractRadius` 400 — see §5 |
| vertex colour | `Col`, `CORNER`, **values exactly `{0.0, 1.0}`**; 24 of 1080 loops parchment (`R=0`) | wood/parchment mask, §3 |
| Y-symmetry | **272/272 verts match after mirror_Y, 0 unmatched** | handedness trap cannot bite |
| **`UCX_SM_WarTable_00`** | 1 hull, 12 tris / 8 verts, `[−190,−125,0] → [190,125,110]` | §6 |
| Nanite | **OFF** (import setting) | `WR-§5` prop class |

### ⭐ ROUND-TRIP PROBE — what the EDITOR gets, not what the script believes it wrote

Each FBX was re-imported into a fresh headless scene and multiplied by `diag(1,−1,1)` to **emulate UE's right→left-handed import negation** (the offline-probe recipe from `refine_trellis_glb.py::export_fbx`'s own docstring):

| | dims (uu) | tris | UV | slots | vertex colour |
|---|---|---|---|---|---|
| `SM_Torch` | `75 × 50 × 136` ✅ | 324 ✅ | `UVMap` ✅ | `[TorchBody, TorchFlame]` ✅ | `Col`, reds `{0.0, 1.0}`, 108/972 loops wood ✅ |
| `SM_WarTable` | `380 × 250 × 110` ✅ | 496 ✅ | `UVMap` ✅ | `[WarTable]` ✅ | `Col`, reds `{0.0, 1.0}`, 36/1488 loops parchment ✅ |
| `UCX_SM_WarTable_00` | `380 × 250 × 110` ✅ | 12 ✅ | — | — | — |

(The `.001` suffixes on material names in the raw JSON are an artefact of the probe re-importing into a session that already held those datablocks. **The FBX bytes themselves carry the bare names** — verified by scanning the binaries: `Torch.fbx` → `SM_Torch`, `TorchBody`, `TorchFlame`; `WarTable.fbx` → `SM_WarTable`, `UCX_SM_WarTable_00`, `WarTable`.)

### Axis / space contract
Blockout-identical (TASK-014/037/038) plus the TASK-348 MIRROR-FIX pre-compensation: `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True`, `apply_scale_options='FBX_SCALE_NONE'`, `mesh_smooth_type='FACE'`, `use_triangles=True`, `path_mode='STRIP'`, plus `colors_type='SRGB'` on both (the vertex-colour stream). ⭐ **Both props are authored Y-SYMMETRIC and measured to be so, so the mirror is a geometric no-op** — the HANDEDNESS/MIRROR TRAP is structurally unable to fire on either asset, and no `pre_rotate_z_deg` lever is needed.

---

## 3. ⚠️ SCALE — how the numbers were chosen (`SC-§34` / `WR-§1`)

⛔ **Nothing here was multiplied by 3, and nothing here is keyed to the hall.** The reference body is the **180-uu roster human** — `pipeline_manifest.json` `assets.Footman.target_dims_ue` Z = **180.0**, and `SiegeSpawn::DefaultCapsuleHalfHeight` = **88** ⇒ a 176-uu capsule. **The hall (`2910 × 720`, 1560 clear) was used only as a legibility CHECK, never as the driver.**

| prop | measured | vs the 180-uu human | vs the hall | reasoning |
|---|---|---|---|---|
| `SM_Torch` | 136 tall, 75 out from the wall | **0.756 ×** human height | **8.7 %** of the 1560 clear height; projects **10.4 %** across the 720 hall width | A "great torch" — the upper end of what a person can carry and light. Real-world scale (~70 uu) is the matchstick the spec warned about; a hall-keyed torch (~400) would read as a bonfire on a pole and would be the `SC-§34` defect. |
| `SM_WarTable` **height** | surface **95**, rim top **110** | **0.53 ×** / **0.61 ×** human height | — | ⛔ **BODY-KEYED and therefore UNSCALED.** You lean over this table. Real tables are 0.42–0.50 × body height; 0.53 is a deliberate half-step up for a heavy oak war table, nothing more. |
| `SM_WarTable` **footprint** | **380 × 250** | 2.6 × the Footman's 147-uu width | **13.1 %** of the hall length, **34.7 %** of its width; leaves **235 uu** walking clearance on each side in Y | ⭐ **The ONE deliberately room-keyed number, and it is declared as one.** A big table is a big table — a footprint is not a scaled human. This is the dimension that makes the prop read across a 2910-uu hall. |

⭐ **The split is the whole point and is stated so a reviewer can check it: HEIGHTS are body-keyed (unscaled), the FOOTPRINT is room-keyed (generous).** Applying a uniform ×3 to either prop would have been the exact defect `SC-§34` names.

**Eyeball gate — PASS, all 16 previews viewed.** `TorchScale_scale_side.png` / `TorchScale_scale_threequarter.png` and `WarTableScale_scale_front.png` render each prop **beside a 180-uu human proxy** (the torch also against a wall slab), so the scale claim above is visually verifiable and not just arithmetic. The dead-on `WarTableScale_scale_front.png` is the honest one — screen-vertical is exactly Z there, and the rim lands at the human's hip.

---

## 4. What the props actually are

**`SM_Torch`** — wall backplate → socket boss → angled shaft → faceted fire bowl → smooth teardrop flame. Flat-shaded stylised low-poly except the flame, which is smooth. Local `+X` = **into the room**, so an anchor's rotation is a `UStaticMeshComponent`'s natural forward.

**`SM_WarTable`** — 4 tapered legs, two stretchers, a slab, a **raised rim** on all four edges, and an inset **parchment sheet** recessed 13 uu below the rim top. The parchment is its own geometric island (no shared verts with the slab), so the vertex-colour boundary is crisp by construction rather than by luck.

---

## 5. ⛔ MATERIAL RECIPES — stock nodes only (Custom-HLSL BAN respected; ZERO texture samplers)

`WR-§4`'s **PROP-CLASS EXCEPTION** applies to all three: no `T_` set, no bake, and the albedo-floor / anti-bleach / luma-retention gates are **NOT APPLICABLE**. ⛔ **NOT the two-slot `TeamRegion` contract — these props are NEUTRAL** (the M4.5 precedent).

⭐ **NOTE FOR TASK-566's SAMPLER-TYPE SWEEP: these three materials contain NO `TextureSample` node at all, so that sweep is vacuous here by construction.** Say so and move on — do not go hunting.

### The vertex-colour mask, and its polarity law
Both meshes carry one `BYTE_COLOR` / `CORNER` attribute named **`Col`** whose red channel is **exactly `0.0` or `1.0`** (measured; no intermediate values, so sRGB byte encoding is lossless at both endpoints).

> ⭐ **RULE, identical on both props: `R = 1.0` ⇒ the PRIMARY material, `R = 0.0` ⇒ the ACCENT.**
> ⛔ **The polarity is deliberate.** UE returns **white** when a vertex-colour stream is absent — so a mask that fails to import degrades to **all-iron** on the torch and **all-wood** on the table, both of which are correct-looking objects. The inverse polarity would have produced a wooden bowl holding fire and an all-parchment table.

⚠️⛔ **THEREFORE, THE ONE IMPORT FLAG THAT MATTERS: `Vertex Color Import Option = Replace`.** Set to *Ignore* the props still import and still look fine — they just silently lose the accent material. That is the failure mode to watch for.

### `M_Torch` → `/Game/Materials/M_Torch` (slot 0)
Surface · Default Lit · Opaque · Two-Sided **off**.
```
VertexColor ──(R)──┬── Lerp(A = WoodColor,      B = IronColor,      Alpha) → Base Color
                   ├── Lerp(A = WoodRoughness,  B = IronRoughness,  Alpha) → Roughness
                   └── Lerp(A = 0.0,            B = IronMetallic,   Alpha) → Metallic
```
| parameter | type | default (linear) |
|---|---|---|
| `IronColor` | Vector | `(0.055, 0.052, 0.050)` — the value the previews were gated on |
| `WoodColor` | Vector | `(0.085, 0.050, 0.026)` ⚠️ **recipe-only — see §8 flag F3** |
| `IronRoughness` | Scalar | `0.55` |
| `WoodRoughness` | Scalar | `0.78` |
| `IronMetallic` | Scalar | `0.85` |

### `M_TorchFlame` → `/Game/Materials/M_TorchFlame` (slot 1)
**Unlit** · Opaque · Two-Sided **off**. (Unlit is the cheap, correct shading model for a pure emitter; Default Lit + Emissive also works and is the fallback if anything objects.)
```
Multiply(A = FlameColor, B = FlameEmissiveIntensity) → Emissive Color
```
| parameter | type | default |
|---|---|---|
| `FlameColor` | Vector | ⭐ **`(1.00, 0.72, 0.42)` — deliberately the SAME triple `WR-§4` pins for `TorchLightColor`, and TASK-558 shipped it verbatim at `Torch.h`.** ⛔ No new colour invented; the flame and the light it implies agree by construction. |
| **`FlameEmissiveIntensity`** | Scalar | ⚠️🚩 **`8.0` — THE TUNABLE, see §8 flag F1** |

⛔ **NO flicker node.** The spec says the emissive strength is *the only visual lever this pass* and `WR-§4` defers flame VFX to a Niagara follow-up. A `Sine(Time)` pulse is one node and is **RECORDED, NOT TAKEN** (§8 F2).

### `M_WarTable` → `/Game/Materials/M_WarTable` (the single slot)
Surface · Default Lit · Opaque.
```
VertexColor ──(R)──┬── Lerp(A = ParchmentColor,     B = WoodColor,      Alpha) → Base Color
                   └── Lerp(A = ParchmentRoughness, B = WoodRoughness,  Alpha) → Roughness
Metallic = 0 (constant)
```
| parameter | type | default (linear) |
|---|---|---|
| `WoodColor` | Vector | `(0.115, 0.070, 0.040)` — the value the previews were gated on |
| `ParchmentColor` | Vector | `(0.520, 0.430, 0.290)` ⚠️ **recipe-only — §8 F3** |
| `WoodRoughness` | Scalar | `0.75` |
| `ParchmentRoughness` | Scalar | `0.92` |

⭐ **Every value above is a PARAMETER, not a constant — so an MI (or Jonathan's playtest note) re-tunes any of them with zero shader recompilation.** That is the same property that keeps D3 cheap (§7).

---

## 6. ⛔ COLLISION — read this before importing

| prop | authored | why |
|---|---|---|
| `SM_Torch` | ⛔ **NONE. No UCX in the FBX.** | It is a decoration bolted high on a wall. TASK-558 already ships `TorchMesh` with **`NoCollision` profile + `SetCollisionEnabled(NoCollision)` + `SetCanEverAffectNavigation(false)`** — ✅ **our two decisions agree; nothing is owed.** ⚠️ Import with **auto-collision OFF** so nothing is generated: a mesh with no simple hull whose component *did* have collision enabled would silently fall back to **per-poly complex** collision. |
| `SM_WarTable` | ✅ **ONE box hull, `UCX_SM_WarTable_00`, `380 × 250 × 110`** (the full furniture volume, floor → rim top) | See the ruling below. |

> ⭐⛔ **WHY THE TABLE CARRIES A HULL EVEN THOUGH TASK-559 DISABLES COLLISION ON IT — this is deliberate, not a contradiction.**
> TASK-559 set `WarTableMesh` to `NoCollision` + no nav effect and **flagged the consequence to Jonathan** (*"THE PLAYER CAN WALK THROUGH THE WAR TABLE"*, proposed as `WR-§9` outcome 8), noting that making it solid is *"one line"*. ⇒ **At runtime today my hull is inert and costs nothing.** But if Jonathan says *"make it solid"*, that one line is only actually cheap **because a clean single-box simple hull already exists** — without it, a blocking profile would fall back to per-poly complex collision on a 496-tri mesh. **The hull is what makes the flagged decision reversible for free.**

⚠️ **FBX-COLLISION-GAP, scoped correctly so nobody misreads it:** that law says *the raw FBX is never trusted for collision* — and it is about **`SM_Castle`**, whose authority is `pipeline_manifest.json`'s `ucx.boxes`. ⛔ **These two props have NO manifest entry by design** (a manifest edit would have collided with TASK-555 in the same wave, and neither prop is a Meshy/TRELLIS asset), so **for `SM_WarTable` the FBX IS the collision authority.** ⇒ import with **Auto Generate Collision = OFF** and read back that exactly **1 simple hull** landed.

📌 **These are FIRST imports, not same-path overwrites** — I verified myself: `Content/Meshes/` (37 assets) and `Content/Materials/` (16 `.uasset`) contain **zero** matches for `torch` or `table`. ⇒ ⛔ **The same-path-overwrite law and the `_ensure_textures_and_mi()` TEXTURE-SKIP TRAP do not apply to either prop.**

---

## 7. 🚩 D3 (torch light cost) — ⛔ NOT DECIDED HERE. What I assumed, and what keeps it cheap.

**Assumed:** nothing about mobility. **Neither prop contains a light, baked lighting, or any light-dependent detail**, and the flame's brightness is a material *parameter*, so the mesh is indifferent to whether Jonathan later rules Movable, Stationary or Static.

**What I did to keep the decision cheap — one recommendation, and it is free:**

> ⭐ **Import BOTH props with `Generate Lightmap UVs = ON` (destination UV channel 1).**
> A Static/Stationary light needs its receivers to carry a lightmap UV. Generating it now costs **nothing at runtime** under the Movable default and means a later flip needs **no re-export and no art task**. Omitting it now means a flip needs an art re-run.

**The honest bound on that, stated so the flag is not oversold:** the torches light the **castle walls**, not mainly themselves. So the real art-side cost of a Static/Stationary ruling is `SM_Castle`'s lightmap UVs (TASK-555/566's asset, ⛔ **not mine**) **plus** a lighting build **plus** an `L_Arena` save — and that last one is precisely what `WR-§3` / D1 forbids without a fresh ruling from Jonathan. ⇒ **My two props add nothing to that cost. I am not arguing for either mobility.**

⛔ **No frame-rate claim is made here** — `WR-§4` is explicit that no FPS baseline exists and that Jonathan's eye is the instrument.

---

## 8. 🚩 FLAGGED FOR JONATHAN — recorded, not decided

| # | item | default that ships | cost to change |
|---|---|---|---|
| **F1** | ⭐ **`FlameEmissiveIntensity = 8.0`** — the flame's brightness, and per the spec **the only visual lever this pass**. Sensible tuning band **3 – 25**: below ~3 the flame reads as painted-on at gameplay camera distance; above ~25 it blooms into a white blob. ⚠️ **I have never seen it in-engine — no editor this task** (see §9). | `8.0` | **An MI scalar.** No recompile, no re-export, no art task. |
| **F2** | **Flame flicker.** A `Sine(Time)` multiply on the emissive is one stock node and would sell the fire far better than a static glow. ⛔ **NOT taken** — `WR-§4` defers flame VFX and the spec named emissive strength as the only lever this pass. | static emissive | one node, or the Niagara `NS_TorchFlame` follow-up `WR-§4` already boards |
| **F3** | ⚠️ **Two recipe colours were never eyeballed:** `M_Torch`'s `WoodColor` and `M_WarTable`'s `ParchmentColor`. The previews render the meshes with a single preview material each, so **what I gated is the SILHOUETTE and the MASK SHAPE, not those two tints.** Declared rather than implied. | as tabled in §5 | vector parameters |
| **F4** | ⭐⛔ **A MEASURED GEOMETRY/TUNABLE TENSION I CAN NOW PRICE FOR TASK-568/569 — TASK-559 explicitly asked for this once the real footprint existed.** `CommanderWarTableForwardOffset = 200` puts the table centre 200 uu ahead of the NPC; my table's half-X is **190**, so its **near edge sits 10 uu from the NPC's origin** and the avatar's body would visually clip into the table's near rim. **Raising it to ~240** clears the avatar (≈16 uu outside a 34-uu capsule) but pushes the table's **far** edge to 430 uu — **30 uu outside `InteractRadius` 400**. ⛔ **Both numbers are BP/feel tunables that are not mine, so I am handing over the arithmetic, not a decision.** ⭐ **A third lever, and it is mine and free: a shorter table.** Re-running `build_warroom_props.py` with the X half-extent at 150 (`SM_WarTable` → `300 × 250`) takes ~6 s, costs zero credits, and dissolves the tension entirely — say the word. | offset `200`, table `380 × 250` | one BP field, **or** a 6-second art re-run |
| **F5** | **Walk-through table** — already TASK-559's flag; my `UCX_SM_WarTable_00` is what makes reversing it a one-liner (§6). | walk-through | one collision profile |

---

## 9. ⛔ WHAT I DID NOT DO, AND THE BOUND ON EVERY CLAIM ABOVE

1. ⛔ **Nothing has been imported. No editor, no MCP, no PIE, no compile, no Git.** ⇒ **Every visual claim is a Blender Workbench render, not an engine render.** Emissive brightness, PBR response and the vertex-colour mask **have never been observed in UE**. The mask's *data* is measured (§2); its *appearance* is not.
2. ⛔ **`pipeline_manifest.json` untouched** — deliberate, and the reason is in §6.
3. ⚠️ **MEASURED FINDING, worth knowing for any future prop task: `bpy.ops.uv.smart_project` is NOT reproducible on this Blender build.** Two runs over bit-identical vertex data produced different UVs and a different loop order (torch: **verts identical, UV max delta 0.923**), so the exported FBX changed size run to run. I replaced it with **`uv.cube_project` + `pack_islands(rotate=False, shape_method='AABB')`**, which is deterministic. UV *layout* is cosmetically irrelevant to these props (no textures); **reproducibility is not.**
4. ⚠️ **Residual, benign, and reported rather than hidden:** the FBX is still not byte-identical run to run because the export-time triangulation picks different diagonals on the lathe's symmetric quads, which reorders loops. **Proven benign by measurement:** vertex positions are bit-identical, and the UV set is identical as a multiset (**340 unique coords, same bbox, within 1e-5**) — only element ORDER differs. Tri count, bounds, slots and UV-layer name are invariant across every run.
5. ⛔ **No `Content/` `.uasset`, no `.cpp`, no `L_Arena`, no `Content/RawAssets/CardArt/`, no `/Game/UI/CardArt/`** (lane isolation held).

---

## 10. UE IMPORT RECIPE — turnkey for **TASK-566**

Serialized, exclusive editor session. **First imports — no delete+recreate question arises.**

**(a) `SM_Torch`** — `Content/RawAssets/Torch.fbx` → **`/Game/Meshes/SM_Torch`**
- **Nanite OFF.** Combine Meshes OFF. Import Normals (not Compute). Generate Lightmap UVs **ON** (§7).
- ⚠️⛔ **Vertex Color Import Option = `Replace`** — *Ignore* silently deletes the iron/wood mask (§5).
- **Auto Generate Collision OFF.** No collision is authored and none is wanted (§6).
- Material slots, exactly in this order: **0 `TorchBody` → `M_Torch`**, **1 `TorchFlame` → `M_TorchFlame`**. The FBX slot names are already `[TorchBody, TorchFlame]` (measured), so they map 1:1.
- **Read back:** 324 tris, bounds `75 × 50 × 136`, UV0 `UVMap`, 2 slots, vertex colours present.

**(b) `SM_WarTable`** — `Content/RawAssets/WarTable.fbx` → **`/Game/Meshes/SM_WarTable`**
- **Nanite OFF.** Combine Meshes OFF. Import Normals. Generate Lightmap UVs **ON**.
- ⚠️⛔ **Vertex Color Import Option = `Replace`** — the wood/parchment mask (§5).
- ⛔ **Auto Generate Collision OFF** so the authored **`UCX_SM_WarTable_00`** is used verbatim; **read back exactly 1 simple hull, `380 × 250 × 110`** (§6).
- Single material slot **`WarTable` → `M_WarTable`**.
- **Read back:** 496 tris, bounds `380 × 250 × 110`, min-Z 0, UV0 `UVMap`, 1 slot, vertex colours present.

**(c) The three materials** — build per §5 in `/Game/Materials/`. **Stock nodes only, Custom-HLSL BAN.** Zero texture samplers ⇒ the sampler-type sweep is vacuous here.

**(d) ⚠️ Do not conflate these with the castle.** The `_ensure_textures_and_mi()` TEXTURE-SKIP TRAP is a **same-path remaster** hazard; both props are first imports with no textures at all. ⛔ Nothing to chase.

---

## 11. → Handoffs to the tasks downstream

- **→ TASK-566 (import + compile):** §10 is the whole recipe. Two FBXs, three materials. ⛔ Nanite OFF on both, `Vertex Color = Replace` on both, auto-collision OFF on both.
- **→ TASK-562 (`ACastle` furnishing — `TorchAnchors` / `CommanderNpcAnchor`):** ⭐ **`SM_Torch`'s origin plane IS the wall-mount face (min X measured `0.0000`), `+X` points into the room, and Z 0 is the vertical centre of the backplate.** ⇒ an anchor is literally *a point on a wall, rotated to face into the room* — no offset fudge, no per-anchor Z correction. The torch occupies `X ∈ [0, 75]`, so it protrudes 75 uu into a hall that is 720 uu wide.
- **→ TASK-558 (`ATorch`) / TASK-569 (BP tuning):** ⭐ **`TorchLightRelativeOffset` = `(50.0, 0.0, 75.0)`** — TASK-558 §7 named this exact landing site and asked for the published `flame_centre_uu`. **Here it is, measured.** For the record, TASK-558's zero-offset fallback derives `(37.5, 0, 104)` from the mesh bbox (XY centre, box top), which lands the light **12.5 uu closer to the wall and 29 uu above the flame's centre** — i.e. at the flame's apex. Both are defensible; the published value puts the light in the middle of the fire.
- **→ TASK-568 (`BP_CommanderNpc`, UI content) / TASK-569:** ⛔ **read §8 F4** — the measured table footprint vs `CommanderWarTableForwardOffset 200` vs `InteractRadius 400`. Also: `SM_WarTable`'s floor-contact origin (min-Z `0.0000`) is confirmed, so TASK-559's *"both sit on the floor at relative Z = 0 with no per-asset Z fudge"* holds exactly.
- **→ TASK-565 (QA gate):** every number in §2 is re-derivable by running `build_warroom_props.py` and reading `Cache/WarRoomProps/props_report.json`; the previews are on disk for an independent eyeball.
- ⛔ **NOT mine and named so:** TASK-567 (crumble trio re-derivation) and TASK-555's castle FBX/manifest are the other art lane entirely; nothing in this task touches them.
