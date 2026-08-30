# TASK-721 — MEASURE THE GRASS — the ramp's two greens + the 5-overlay legibility table

**Agent:** art-director · **Date:** 2026-08-30 · **Status:** ready-for-integration (input to TASK-722)
**Law:** `WM-§8b` (measured, never picked) · `WM-§8c` (legibility is an acceptance criterion) · `WM-§1` · `AS-§6 A(e)`

⛔ **Read-only on `Content/`.** No `.uasset` written, no material edited, no texture authored, no code touched, no Git, editor never opened.

---

## 0. THE TWO NUMBERS (the deliverable)

> ### ⚠️⚠️ COLOUR SPACE — READ THIS BEFORE TYPING THE LITERALS
> These constants are written into an **`SRGB = true` texture as raw bytes** (`WarMapWidget.cpp:1062` sets `Texture->SRGB = true`; `:1085` writes `RoundToInt(Luma * 255.f)`). **The bytes of an sRGB texture ARE sRGB-encoded values.** ⇒ the two constants must hold **DISPLAY-ENCODED (sRGB) values**, exactly as the shipped scalar `ElevationFloorLuma = 0.10f` already does.
> ⛔ **Typing the LINEAR column into `:1084-1087` would render the ramp far too dark** (the light end would drop from byte 255 to byte 160 on green, and the dark end from 26 to 3) — visibly wrong, and it would review as correct because the type is named `FLinearColor`.

```cpp
// DISPLAY-ENCODED (sRGB) values in an FLinearColor container — the space the shipped
// ElevationFloorLuma 0.10f already lives in, because these become bytes of an SRGB=true texture.
// Measured from /Game/Materials/Instances/MI_BattlefieldGround — see TASK-721 handoff §1.
static const FLinearColor ElevationGrassDark  = FLinearColor(0.0626f, 0.1000f, 0.0624f, 1.00f);
static const FLinearColor ElevationGrassLight = FLinearColor(0.6257f, 1.0000f, 0.6243f, 1.00f);
```

| | R | G | B | as bytes | space |
|---|---|---|---|---|---|
| **`ElevationGrassDark`** ⭐ **USE THIS** | **0.0626** | **0.1000** | **0.0624** | (16, 26, 16) | **display / sRGB-encoded** |
| **`ElevationGrassLight`** ⭐ **USE THIS** | **0.6257** | **1.0000** | **0.6243** | (160, 255, 159) | **display / sRGB-encoded** |
| `ElevationGrassDark` — true linear | 0.00516 | 0.01002 | 0.00515 | — | linear (⛔ do NOT type at `:1084`) |
| `ElevationGrassLight` — true linear | 0.34934 | 1.00000 | 0.34770 | — | linear (⛔ do NOT type at `:1084`) |

**Conversion used** (IEC 61966-2-1, both directions, applied per channel):
`linear = c/12.92 if c ≤ 0.04045 else ((c+0.055)/1.055)^2.4` · `display = 12.92c if c ≤ 0.0031308 else 1.055·c^(1/2.4) − 0.055`

**Why these two are ONE colour at two luminances (`WM-§8b`'s "hue is the grass's; only value moves"):**
both ends are the identical display-space chromaticity **C = (0.6257, 1.0000, 0.6243)** scaled by the shipped luminance endpoints **0.10** and **1.00**. Since `Lerp(C·0.10, C·1.00, B) = C · Lerp(0.10, 1.00, B)`, **hue and saturation are constant at every point on the ramp by construction**, and the luminance curve is **byte-identical to the shipped gray ramp**. ⭐ Only the colour changed; the relief's shape did not (`WM-§8a`).

⚠️ **Interpolate in DISPLAY space** (the same `FMath::Lerp` + `*255` arithmetic as today), ⛔ not in linear. A linear-space lerp would put the ramp's midpoint at display **0.73** instead of **0.55** — a visible change to how relief is distributed, which `WM-§8a` says must not happen.

---

## 1. SAMPLE SOURCE AND METHOD

### 1a. The asset chain, read at source

| Asset | On disk | What it contributed |
|---|---|---|
| `/Game/Materials/Instances/MI_BattlefieldGround` | `Content/Materials/Instances/MI_BattlefieldGround.uasset` | **the live tint values** (this MI is what `ArenaGround` renders) |
| `/Game/Materials/M_BattlefieldGround` | `Content/Materials/M_BattlefieldGround.uasset` | the recipe + the texture bindings |
| `/Game/Tree_Pack_1/Textures/Ground/T_Ground_Grass_C` | `Content/Tree_Pack_1/Textures/Ground/T_Ground_Grass_C.uasset` | **the base-colour texels** |
| `/Game/Materials/M_HillGrass` | `Content/Materials/M_HillGrass.uasset` | the required cross-check |

**The shipped recipe** (`M_BattlefieldGround`, confirmed by name-table read — it references **exactly two** textures, `T_Ground_Grass_C` and `T_Ground_Grass_N`, and exposes `GrassBaseColor` / `GrassNormal` / `MacroMaskTex` / `GrassTiling` / `MacroTiling` / `TintLush` / `TintDry` / `GroundRoughness`):

```
BaseColor = TriplanarSample(T_Ground_Grass_C) × Lerp(TintLush, TintDry, MacroMask.R)
```

**The MI's live parameter values, read out of the `.uasset` binary** (float32 scan of `VectorParameterValues`), **not taken on trust from the TASK-249 note:**

| Parameter | MI_BattlefieldGround (live) | master default (overridden) |
|---|---|---|
| `TintLush` | **(0.55, 1.15, 0.38)** @ byte 14317 | (0.90, 1.28, 0.55) |
| `TintDry` | **(0.95, 1.05, 0.48)** @ byte 14631 | (1.55, 1.35, 0.72) |

These reproduce the TASK-249 handoff's recorded values **exactly**, so two independent sources agree.

### 1b. Which texels, and how they were averaged

⚠️ **Stated plainly because it is the one soft spot in this measurement:** the 2048×2048 source-art PNG **is** embedded in `T_Ground_Grass_C.uasset` (IHDR verified: `2048x2048 depth=8 colortype=6` at byte offset 94019) but its byte stream is **package-compressed (Oodle-framed)** — chunk CRCs fail on roughly half the `IDAT` chunks, with 5-, 11- and 23-byte insertions between chunk boundaries, so it cannot be inflated off disk without the engine.

**What I sampled instead:** the **256×256 RGBA thumbnail UE stores in the same package** (offset 1611, decodes cleanly, **alpha = 255 on every texel** so nothing was composited over a checker). This is UE's own downsample of the same texels.

- **Region:** the **entire 256×256 image**, all 65,536 texels, no crop, no weighting.
- **Method:** each texel decoded **sRGB → linear** first, **then** averaged. ⚖️ Averaging the bytes and converting afterwards is the common error; here it would have given (0.4400, 0.5299, 0.5397) display instead of the correct (0.4438, 0.5322, 0.5432) — a small gap only because this texture is low-contrast, but the correct order was used regardless.

**`T_Ground_Grass_C` area-average = LINEAR (0.16564, 0.24507, 0.25627)** — note it is a desaturated grey-green ground, very slightly **blue**-dominant on its own. The green comes from the tint, which is exactly why `WM-§8b` pins the MI and not the raw texture.

**Robustness of the substitute source** (so the thumbnail choice is not a hidden assumption):

| Perturbation of the texture mean | resulting chromaticity C |
|---|---|
| baseline | (0.6257, 1.0000, 0.6243) |
| all channels **+15%** | (0.6332, 1.0000, 0.6289) |
| all channels **−15%** | (0.6176, 1.0000, 0.6192) |
| worst single-channel ±15% | (0.5732 … 0.6748, 1.0000, 0.5771 … 0.6727) |
| four independent image quadrants | R 0.6075 … 0.6479 |

⇒ **a uniform ±15% sampling error moves the delivered chromaticity by under 0.01**, because C is a *ratio* and `TintLush`/`TintDry` dominate the hue. The four quadrants agree to ~0.04, so the texture is spatially uniform enough that an area-average is meaningful. 🙋 **If Jonathan wants it re-confirmed exactly, one editor call on the live texture would do it** — but on these numbers it cannot change the ramp materially, so it did not justify asking him to open the editor.

### 1c. The macro-mask term, and why it does not need pinning

`MacroMaskTex` is a texture *parameter* and the master references **no third texture**, so it defaults to one of the two grass maps. Rather than guess, the result is **bracketed across the whole possible range** — and the chromaticity barely moves:

| macro-mask R | effective BaseColor (linear) | chromaticity R : G : B |
|---|---|---|
| 0.0 (all `TintLush`) | (0.09110, 0.28183, 0.09738) | 0.323 : 1.000 : 0.346 |
| **0.166 — measured mean ⭐ used** | **(0.10208, 0.27777, 0.10163)** | **0.367 : 1.000 : 0.366** |
| 0.5 (a normal-map default would land here) | (0.12423, 0.26957, 0.11019) | 0.461 : 1.000 : 0.409 |
| 1.0 (all `TintDry`) | (0.15736, 0.25732, 0.12301) | 0.612 : 1.000 : 0.478 |

Green stays dominant across the entire bracket; only the red end warms. The delivered value uses the measured mean.

### ⭐ RESULT — the shipped grass albedo

**LINEAR (0.10208, 0.27777, 0.10163)** · **display (0.3527, 0.5637, 0.3519)** — a mid moss green.

### 1d. CROSS-CHECK vs `/Game/Materials/M_HillGrass` — ✅ NO DISAGREEMENT

Float scan of `Content/Materials/M_HillGrass.uasset` returns `TintLush` **(0.55, 1.15, 0.38)** @ 39399 and `TintDry` **(0.95, 1.05, 0.48)** @ 39727 — **byte-identical to the MI**, and TASK-249 documents it samples **the same `T_Ground_Grass_C`** with the same macro-tint recipe. Its third vector, `RockTint (0.72, 0.62, 0.50)`, applies only to faces steeper than ~37°, which the ≤30° climbable-hill law excludes from walkable ground.

⇒ **field and hills are the same measured green.** ⭐ The ramp's high end sits on hills and its low end on the field, and there is **no seam to report** — the two ends of the ramp match the two terrains they actually cover.

---

## 2. ⚠️ THE COLOUR-SPACE SPLIT INSIDE `WarMapWidget.cpp` — a finding TASK-722 and QA both need

**The elevation ramp and the overlay tints reach the screen through two different transfers, and the file's `FLinearColor` constants do not all mean the same thing:**

| Path | Mechanism (read at source) | What the numbers mean |
|---|---|---|
| **Elevation ramp** | `Texture->SRGB = true` (`:1062`) + byte `= Luma × 255` (`:1085`) | **display / sRGB-encoded** |
| **Every overlay tint** | Slate `PackVertexColor()` → `InLinearColor.ToFColor(bSRGBVertexColor)`; `bSRGBVertexColor = !IsVertexColorInLinearSpace()`, and `FSlateRHIRenderingPolicy::IsVertexColorInLinearSpace()` returns **`false`** ⇒ `ToFColor(true)` ⇒ **sRGB encode applied at draw** | **true linear** |

*(engine source: `Engine/Source/Runtime/SlateCore/Public/Rendering/ElementBatcher.h:282-287`, `Engine/Source/Runtime/SlateRHIRenderer/Private/SlateRHIRenderingPolicy.h:37`)*

⚠️ **Consequence for the table below:** an overlay's on-screen appearance is `srgb_encode(tint)`, so `AncientGroundIconTint (0.62, 0.93, 0.66)` linear actually paints as display **(0.81, 0.97, 0.83)** — a **near-white** mint, far paler than the literal reads. All contrast arithmetic below places both families in the same physical-light space before comparing.

🚩 **FLAGGED for QA, not fixed here:** after TASK-722 lands, `WarMapWidget.cpp` will hold `FLinearColor` constants in **two different spaces** a few hundred lines apart. ⛔ Not my file to change — but the two new constants should carry a comment naming their space, or the next reader will "correct" them into linear and darken the map.

---

## 3. ⚠️ THE LEGIBILITY TABLE — all five overlay families, both ramp ends

**Method:** WCAG 2.1 relative luminance `L = 0.2126R + 0.7152G + 0.0722B` on **linear** values; contrast `(L₁+0.05)/(L₂+0.05)`. **Gate = 3.0:1**, WCAG 2.1 SC 1.4.11 (non-text graphical objects). ΔE is CIE76 in L\*a\*b\* (D65) and carries the **hue** dimension that a luminance ratio cannot see. ⛔ **No claim is made here that the map "reads well"** (`AS-§6 A(e)`) — this is arithmetic; the eye is Jonathan's.

Ramp end luminance: **old** dark 0.0100 / light 1.0000 (17.49:1 total) · **new** dark 0.0086 / light **0.8146** (14.74:1 total).

| # | Overlay family | Colour (source) | old dark → light | **new dark → light** | ΔE new dark / light | Verdict |
|---|---|---|---|---|---|---|
| 1 | **White POI glyphs** (`WM-§1`, white-on-transparent bound) | `(1.00,1.00,1.00)` | 17.49 → **1.00** | 17.91 → **1.21** | 92.5 / **60.1** | ⭐ **IMPROVES, as predicted.** Today it is a literal **1.00:1 — white on white, mathematically invisible** at the top end. The green top end is strictly better and adds ΔE 60 of hue separation. ⚠️ Still under 3:1 on *luminance* alone at the extreme top; it now reads by **hue**, not by value. **No repair — this row gets better, not worse.** |
| 2 | **Blue ally dots** `GetDefaultBlueBarColor()` `(0.05,0.30,1.00)` (`:1448`) | — | 5.79 → 3.02 | 5.92 → **2.49** | 85.6 / **116.6** | ⚠️ **PASS on hue, marginal on luminance.** Dips from 3.02 to 2.49 at the light end — just under the gate. But **ΔE 116.6 is the largest separation in the table**: saturated blue on light green cannot be confused. ⛔ **No repair proposed**; if Jonathan ever wants more presence the `AllyDotRadius` tunable already exists (`WM-§3`, TASK-685). Enemy red is the same story (3.11 → 2.56, ΔE 113.5). |
| 3 | **Castle team tints** (same two accessors, `W4-R5`) | blue / red as above | 5.79 / 5.63 → 3.02 / 3.11 | 5.92 / 5.76 → **2.49 / 2.56** | 85.6 / 116.6 · 96.8 / 113.5 | ⚠️ **Identical arithmetic to row 2 — PASS on hue.** Castles are also the largest glyphs on the map, so the marginal luminance dip matters least here. **No repair.** |
| 4 | **Map text labels** `MarkerLabelColor (0.97,0.94,0.86)` (`:131`) | drawn at `:1602` **with NO outline** | 16.50 → **1.06** | 16.89 → **1.15** | 90.1 / **56.5** | 🚩 **PRE-EXISTING FAILURE, marginally improved, ⛔ NOT caused by this change.** Near-white text already sits at **1.06:1** on today's white top end. Green nudges it to 1.15:1. ⚠️ The *marker glyph* beside it is safe — it gets `MarkerOutlineColor` at `:1577` (12.22:1 vs the new light end) — but **`MakeText` at `:1602` passes no outline**. **Smallest repair, if Jonathan wants it: give the label the outline the marker already has** (an `FSlateFontInfo` outline setting) — ⛔ out of this batch's scope, flagged only. |
| 5 | ⚠️⚠️ **`AncientGroundIconTint (0.62,0.93,0.66)`** (`:167`), drawn at `:1514` **with NO outline** | "pale verdant" | 14.90 → 1.17 | 15.26 → **1.03** | 87.2 / **36.7** | ⛔⛔ **CONFIRMED — THIS IS THE COLLISION.** **1.03:1** at the light end, and **ΔE 36.7 — by far the lowest in the table**. It is the only overlay that collides on **luminance AND hue at once**: a pale green icon on a light green background. The manager's read was right, and it is now measured. ⇒ **repair below.** |

**Supporting row (measured, not one of the five):** `MineIconTint (1.00,0.72,0.18)` gold — 1.33 → **1.09** at the light end, ΔE **49.6**. 🚩 Also already failing today; gold-on-green keeps real hue separation and the law rates it low-risk. **Flagged, no repair proposed** — it is a pre-existing top-end issue and re-tinting a second icon in this batch is scope creep.

### ⭐ THE STRUCTURAL FINDING BEHIND ROWS 1, 4 AND 5

The ramp spans **14.74:1** end to end. For a **flat, un-outlined** overlay to clear 3:1 against **both** ends, its luminance must satisfy

`3(L_dark + 0.05) − 0.05 ≤ L ≤ (L_light + 0.05)/3 − 0.05` ⇒ **L ∈ [0.1259, 0.2382]**

⚠️ **The same window against today's gray ramp is [0.1301, 0.3000] — so this constraint is PRE-EXISTING and the green ramp barely narrows it.** ⛔ **The green ramp is not what broke these overlays; a 17:1 background range did, and it shipped that way.** The best worst-case any flat colour can achieve against the new ramp is **√14.74 = 3.84:1**.

⇒ **Two shipped overlays (`MarkerColor`, and the marker family generally) already solve this the right way — with an outline.** The three rows that fail are precisely the three drawn **without** one.

---

## 4. ⭐ THE RULED REPAIR — re-tint the ONE icon, ⛔ never the ramp

⛔ **The ramp is NOT pulled off the grass.** Jonathan asked for grass-matched green; the icon yields. This is `WM-§1`'s own named-constant lever and it is a **one-line edit at `:167`**, which TASK-722 already owns by file ownership (`W691-4`).

```cpp
// TASK-721: re-tinted off "pale verdant" — measured 1.03:1 / dE 36.7 against the new green
// ramp's light end (it collided on luminance AND hue at once). Deep emerald keeps the verdant
// identity, clears 3:1 at BOTH ramp ends, and stays far from team blue AND team red.
// ⚠️ TRUE LINEAR — Slate sRGB-encodes tints at draw (ToFColor(true)); NOT the ramp's space.
static const FLinearColor AncientGroundIconTint = FLinearColor(0.04f, 0.22f, 0.09f, 1.00f);
```

**`AncientGroundIconTint = FLinearColor(0.04f, 0.22f, 0.09f, 1.00f)`** — **true linear** (this one *is* a Slate tint, so linear is correct here — see §2). Paints as display (0.221, 0.506, 0.332).

| Candidate (linear) | lum | vs dark | vs light | ΔE light | ΔE blue | ΔE red | ΔE gold | verdict |
|---|---|---|---|---|---|---|---|---|
| **SHIPPED** pale verdant (0.62,0.93,0.66) | 0.845 | 15.26 | **1.03** | **36.7** | 84.6 | 94.9 | 46.3 | ⛔ **FAIL** |
| ⭐ **deep emerald (0.04, 0.22, 0.09)** | 0.172 | **3.79** | **3.89** | **49.8** | **89.8** | **100.9** | **64.0** | ✅ **PASS — RECOMMENDED** |
| deep teal (0.05, 0.26, 0.30) | 0.218 | 4.57 | 3.22 | 67.8 | 57.3 | 102.5 | 77.9 | ✅ pass — alternate, more hue separation from the ramp, but closer to team blue |
| magenta (0.55, 0.12, 0.45) | 0.235 | 4.86 | 3.03 | **121.1** | 53.2 | 75.6 | 100.3 | ✅ pass — maximum separation, but abandons the verdant identity |
| amethyst (0.42, 0.16, 0.78) | 0.260 | 5.29 | **2.79** | 132.2 | **37.9** | 100.3 | 118.8 | ⛔ **FAIL** — misses the gate *and* is nearest team blue |

**Why deep emerald is the recommendation, on the numbers, not on taste:**
- **3.79:1 / 3.89:1** — essentially the **best worst-case any flat colour can achieve** (the 3.84:1 ceiling). Its luminance 0.1723 sits near the centre of the legal window [0.1259, 0.2382], so it degrades gracefully at both ends rather than hugging one.
- **Largest distance from BOTH team colours** of any passing candidate (ΔE **89.8** from blue, **100.9** from red) — which is the property `:167`'s own comment says the tint exists to have.
- **ΔE 49.8 / 50.6 against the two ramp ends** — balanced, unlike the shipped tint's lopsided 87.2 / **36.7**. It is also ΔE **47.5** from the colour it replaces, so the change is visible rather than cosmetic.
- ⭐ **Smallest possible repair:** it keeps the icon **verdant**, exactly as designed. Only its **value** moved, down into the legible window. Same change the ramp itself makes: hue kept, luminance moved.

🙋 **Jonathan's call, not mine:** teal and magenta both pass and separate from the green background more strongly; emerald separates from the *team colours* more strongly and preserves the shipped design intent. ⛔ I am not permitted to declare which "reads well" (`AS-§6 A(e)`) — the arithmetic is above, the eye is his.

---

## 5. Notes for TASK-722

1. ⭐ **Type the DISPLAY column from §0.** The values become bytes of an `SRGB = true` texture. ⛔ Not the linear column.
2. ⭐ **Keep the lerp in display space** — the same `FMath::Lerp(...)` + `× 255` arithmetic as `:1084-1085`, done per channel. This reproduces the shipped luminance curve exactly (`WM-§8a`); a linear-space lerp would move the ramp's midpoint 0.55 → 0.73.
3. ⚠️ **`FColor` is little-endian B,G,R,A** (the file says so at `:1096`). The dark end is nearly symmetric (16, 26, 16) so a transposition would hide there; **the light end (160, 255, 159) is where a swapped R/B shows** — R and B differ by only 1 byte, so ⛔ **do not use the ramp itself as the channel-order check**. Verify against the comment, not the pixels.
4. **`ElevationFloorLuma = 0.10f` (`:198`)** is now folded into `ElevationGrassDark` (it is literally the 0.10 the chromaticity was scaled by). Retire it per spec item 2 so no dead scalar implies a gray ramp.
5. **`AncientGroundIconTint` at `:167` → `(0.04f, 0.22f, 0.09f, 1.00f)`** — ⚠️ **true linear**, ⛔ a different space from the two ramp constants in the same file. Comment both.
6. **Property tests hold on these numbers:** G > R and G > B at **both** ends (0.1000 > 0.0626 > 0.0624 dark; 1.0000 > 0.6257 > 0.6243 light) ✅ · monotonic in luminance ✅ · brightness 0 → dark constant, 1 → light constant ✅.
7. ⛔ **Nothing in this handoff touches gameplay.** No `HIGH-§` seam, no `GetActorLocation`, no export off the display path (`WM-§8d`).

## 6. Evidence

- **Preview render:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\Cache\WarMapGreen\TASK-721_green_ramp_legibility.png` — the new ramp with all eight overlay families dotted across it, today's gray ramp above for comparison, and swatches for the grass albedo / both ramp ends / the ancient-ground tint before and after. (Gitignored Cache, the TASK-249 pattern. ⛔ Not evidence that anything "reads well" — a visual aid for Jonathan's eye.)
- No file under `Content/` was created, modified or deleted.
