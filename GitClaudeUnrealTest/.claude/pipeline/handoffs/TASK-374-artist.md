# TASK-374 handoff — `M_AncientGround` jade rune-ring pulsing decal

Status: **ready-for-integration**
Agent: art-director
Branch: main
Date: 2026-08-01

## Asset created

| | |
|---|---|
| Asset | **`/Game/Materials/M_AncientGround`** (Material, NEW) |
| Composed path the C++ soft-load must use | **`/Game/Materials/M_AncientGround.M_AncientGround`** |
| On disk | `Content/Materials/M_AncientGround.uasset` (33,067 bytes, saved, `is_dirty == false`) |
| New textures | **none** — 100 % procedural, stock nodes only |
| Other assets touched | **none.** No save-all. `L_Arena` never opened or saved. PIE was verified STOPPED before authoring. |

## Material settings (readback-verified via Unreal MCP)

- `MaterialDomain` = **`MD_DeferredDecal`**
- `BlendMode` = **`BLEND_Translucent`**
- `ShadingModel` = **`MSM_DefaultLit`**
- Outputs connected: **`MP_EmissiveColor`** ← `Multiply_11`, **`MP_Opacity`** ← `Saturate_0`
- **`MP_BaseColor` = None (deliberately unconnected)**, as are `MP_Normal` / `MP_Metallic` / `MP_Roughness` — the `M_CaptureZone` / `M_SpellReticle` law, so units standing inside stay readable and the decal never writes an opaque albedo slab.
- 45 expressions. **`has_custom_node == false`** — the Custom-HLSL BAN is honored; every node is stock.

## Parameters (exact names + defaults, all readback-verified)

| Parameter | Type | Default | Group | What it does |
|---|---|---|---|---|
| **`GroundColor`** | **Vector** | **(0.10, 0.85, 0.55, 1.0)** jade | Color | The single tint param. Maximally far from Blue (0.05,0.30,1.00), Red (1.00,0.10,0.05) and the capture zone's neutral grey (0.5,0.5,0.5). |
| `RingCount` | Scalar | 2.5 | Rune Rings | Radial frequency → **5 rings** across the inscribed circle. |
| `RingSharpness` | Scalar | 6.0 | Rune Rings | Ring thinness (higher = thinner). |
| `SpokeCount` | Scalar | 6.0 | Rune Rings | Angular frequency → **12 rune ticks** per ring. |
| `SpokeSharpness` | Scalar | 3.0 | Rune Rings | Tick crispness. |
| `SpokeDepth` | Scalar | 0.55 | Rune Rings | How far the ticks dim the ring between them (0 = solid rings). |
| `PulseSpeed` | Scalar | 0.15 | Pulse | Hz. 0.15 ⇒ a **~6.7 s** breath. |
| `PulseAmount` | Scalar | 0.35 | Pulse | ±35 % amplitude on both opacity and glow. |
| `FillOpacity` | Scalar | 0.10 | Intensity | Faint wash over the whole square quad. |
| `RingOpacity` | Scalar | 0.55 | Intensity | Ring opacity. |
| `FillGlow` | Scalar | 0.60 | Intensity | Wash emissive multiplier. |
| `RingGlow` | Scalar | 2.40 | Intensity | Ring emissive multiplier. |

Every tunable is a parameter, so retuning at playtest is a **MID / material-instance edit with zero shader recompile** (the `M_CaptureZone` knob philosophy, extended — `M_CaptureZone` left five values as raw graph constants; here only the two disc-fade constants remain hard-coded, see below).

## Node list (the graph, in evaluation order)

**Shaping — radial, NOT Chebyshev:**
`TextureCoordinate_0` → `Subtract_0` (−0.5) → `Multiply_0` (×2) ⇒ centered UV in [−1, 1]
→ `Length_0` ⇒ **`r`** (0 at center, 1.0 at the edge midpoints, 1.414 at the corners)

**Rune rings:** `Multiply_1` (r × `RingCount`) → `Sine_0` (Period 1) → `Abs_0` → `Power_0` (Exp ← `RingSharpness`)
**Rune ticks:** `ComponentMask_0` (R) + `ComponentMask_1` (G) → `Arctangent2_0` (Y, X) → `Multiply_2` (×0.1591549431 = 1/2π) → `Multiply_3` (× `SpokeCount`) → `Sine_1` → `Abs_1` → `Power_1` (Exp ← `SpokeSharpness`)
`OneMinus_0` (1 − `SpokeDepth`) → `LinearInterpolate_0` (A = that, constB = 1.0, Alpha = tick) → `Multiply_4` (rings × tick factor)
**Disc confine:** `SmoothStep_0` (Value = r, **constMin 0.93, constMax 1.03**) → `OneMinus_1` ⇒ `discMask` → `Multiply_5` ⇒ **`ringMask`**

**Pulse:** `Time_0` → `Multiply_6` (× `PulseSpeed`) → `Sine_2` (Period 1) → `Multiply_7` (× `PulseAmount`) → `Add_0` (constB 1.0) ⇒ pulse ∈ [0.65, 1.35]

**Opacity:** `LinearInterpolate_1` (A `FillOpacity`, B `RingOpacity`, Alpha `ringMask`) → `Multiply_9` (× pulse) → `Saturate_0` → **`MP_Opacity`**
**Emissive:** `LinearInterpolate_2` (A `FillGlow`, B `RingGlow`, Alpha `ringMask`) → `Multiply_10` (× `GroundColor`) → `Multiply_11` (× pulse) → **`MP_EmissiveColor`**

The only remaining raw graph constants are `SmoothStep_0`'s **0.93 / 1.03** (where the rune circle fades out, in units of r). Everything else is a parameter.

## Why it cannot be mistaken for `M_CaptureZone` — captures attached

`.claude/pipeline/handoffs/TASK-374/M_AncientGround_shape_vs_capturezone.png` is a 4-panel comparison rendered by evaluating both node graphs offline at full precision (script alongside it, `preview_ancientground.py` — it is a 1:1 transcription of the shipped graph and is the retuning tool):

- **top-left** — `M_AncientGround` at pulse **peak**
- **top-right** — same at pulse **trough** (the breath is clearly visible)
- **bottom-left** — `M_CaptureZone` (blue, its square Chebyshev border band)
- **bottom-right** — **the centerline case: both overlapping.** A blue square frame and a jade dashed rune rosette. Nothing ambiguous about which is which.

`.claude/pipeline/handoffs/TASK-374/M_AncientGround_editor_thumbnail.png` is the live in-editor asset thumbnail — jade rune rings, i.e. the real compiled shader, not the grey default material.

Differentiation summary: `M_CaptureZone` = Chebyshev `max(|x|,|y|)` → **square border band**, static, blue/red/grey. `M_AncientGround` = Euclidean `length(uv)` → **5 concentric dashed rune rings + 12 angular ticks**, breathing on `Sine(Time)`, jade. Different distance metric, different topology, different color axis, different temporal behavior.

## ⚠️ THREE THINGS THE INTEGRATOR MUST DO — none of them can live on the material

### 1. `SortOrder = 10` is **NOT** a material property — it must be set on the `UDecalComponent`

The board spec assigned "SortOrder 10" to this task. **`UMaterial` in UE 5.8 exposes no SortOrder / SortPriority field** — I dumped the complete property list of `M_CaptureZone` (`ObjectTools.list_properties`) and there is no such property; decal sort order lives on **`UDecalComponent::SortOrder`**.

So this is **carried forward as a code-side requirement for `AAncientGround` (TASK-359)**:

```
AncientGroundDecal->SetSortOrder(10);   // or SortOrder = 10 in the constructor
```

Confirmed safe: a repo-wide grep for `SortOrder` across `Source/GitClaudeUnrealTest/Siegebound/` returns **zero hits**, so `ACaptureZone` leaves its decal at the default **0** and any positive value wins. `Source/GitClaudeUnrealTest/Siegebound/AncientGround.{h,cpp}` **does not exist yet** — TASK-359 has not landed — so nothing is broken today, but if TASK-359 ships without this line the two decals z-fight where they overlap near the centerline.

### 2. `DecalSize` and `FadeScreenSize` are also component-side (TASK-359)

Per CONVENTIONS and plan §6, on the decal component after the −90 pitch:
- `DecalSize = (1024, 840, 840)` — `.X` is the projection half-**depth**, `.Y`/`.Z` are the ground half-extents and must equal `ZoneHalfExtent`.
- **`FadeScreenSize = 0.001`** explicitly — the 0.01 default culls decals at this arena's zoom-out.

**Sanity-checked against that footprint** and the material is correct at it: shaping is entirely UV-space, so it auto-fits whatever XY the component is scaled to — no hardcoded world size, nothing to change if the extents move. At 840×840 half-extents the 5 rings land at radii ≈ **84, 252, 420, 588, 756 cm** from the actor origin (ring bands ≈ 50 cm thick), and the rune circle stops just short of the quad edge so no ring is clipped by the decal boundary.

### 3. The faint fill covers the **full square**, on purpose

The mechanic box is a **square** 840×840, but the rune rosette is a **circle** inscribed in it — a disc-only visual would leave ~21 % of the boosted area (the four corners) completely unmarked, which would be a lie about where the boost applies. So the `FillOpacity` wash is **not** disc-masked: it covers the entire decal quad, honestly marking the square footprint, while only the bright rings are confined to the inscribed circle. This is the one deliberate departure from a literal `M_CaptureZone` clone of the opacity chain, and it is why `Multiply_8` (the disc multiply on the opacity path) was removed after the first build.

## Null-safety contract (unchanged, worth restating because it fails *silently*)

`AAncientGround` (TASK-359) soft-loads this path null-safe: a missing material ⇒ log once, **no visual, and the boost mechanic still runs**. That means **a typo'd asset path produces a silently invisible feature, not an error** — the ancient ground would still empower units with nothing on screen to explain why. The one string that must match, character for character:

```
/Game/Materials/M_AncientGround.M_AncientGround
```

## Acceptance

- [x] `MD_DeferredDecal` · `BLEND_Translucent` · `MSM_DefaultLit` — readback-confirmed
- [x] Emissive + Opacity connected; **BaseColor unconnected** (`None`) — readback-confirmed
- [x] `GroundColor` VectorParameter present, default **(0.10, 0.85, 0.55, 1.0)** — readback-confirmed
- [x] 11 scalar params for every tunable (fill opacity, ring opacity, ring glow, pulse speed, pulse amount + shaping), grouped, slider-bounded
- [x] Shaped by **radial** rune rings + a `Sine(Time)` pulse — **not** a recolored square Chebyshev band; proven by the 4-panel capture including the overlap case
- [x] **Stock nodes only** — `has_custom_node == false` across all 45 expressions
- [x] **Compiles clean.** `MaterialTools.recompile` raised no error on three separate calls after wiring. `LogMaterial` holds exactly **one** `Failed to compile Material` line, at `22.45.58 UTC`, which is **the moment of creation, before a single node was wired** (an empty `MD_DeferredDecal` + `BLEND_Translucent` material with no Opacity connected always emits it). **No warning was emitted by any post-wiring recompile**, and the live in-editor thumbnail renders the authored jade rune rings rather than the grey default material — which a failed shader could not do.
- [x] Saved to disk; `is_dirty == false`; no other asset saved; `L_Arena` untouched
- [x] No Git, no C++ written
