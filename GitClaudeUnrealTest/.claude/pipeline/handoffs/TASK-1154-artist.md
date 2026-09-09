# TASK-1154 — [FOGGREY-ART] — art-director handoff

**Trigger verdict: EXECUTED, for the named asset-side contributor.** `handoffs/TASK-1151-artist.md` §2 carries `CAUSE — ASSET-SIDE ✅ (ask C)` and names it: `BP_SiegeFog` → `general Data.base Color`, then `(1, 1, 1, 1)`. Its ask-(C) `CAUSE — ENGINE-SIDE` section is **absent and measured absent**, and it says explicitly *"this is NOT a mixed cause"* ⇒ `TASK-1155` does not fire and this row runs alone.

Date 2026-09-08 · editor PID **14120** · ran **second**, in the same session as `TASK-1152`, on the same `.uasset`, per the mandated order (uniformity first — a density change moves the perceived hue, so colour is judged on the final density).

Instrument, calibration, Simulate setup, controls and fences: **as documented in `handoffs/TASK-1152-artist.md` §1 and §6** — same session, same instrument, same frames. Not restated.

---

## 1. The before, on pixels — and it corroborates `TASK-1151` independently

My reproduction of his fog, measured in Simulate at the `VID-007` vantage, against his own recorded frames:

| | far-field sRGB | R/G | B/G |
|---|---|---|---|
| **his `VID-007` reference (t=27)** | (190.6, 181.1, 162.2) | 1.052 | 0.896 |
| **mine, before (Simulate)** | (191.7, 179.2, 161.6) | 1.070 | **0.902** |

B/G agrees to **0.006**. `TASK-1151` reported (190, 181, 162) / R/G 1.05 / B/G 0.895 — I reproduce it. The warm cast is real, it is in my frames, and it is the same cast he is complaining about.

✅ **And the negative control separates fog from world:** the same vantage with **no `BP_SiegeFog`** reads (126.0, 135.2, 72.2) — **green**, B/G 0.53. The battlefield is not beige; the fog is. So the hue arrives with the fog, exactly as `TASK-1151` concluded.

## 2. 🚨 Why a grey `base Color` would only have darkened — computed in LINEAR, not sRGB

`base Color` is a **multiplier** on scattered light, applied in linear space. The measured RGB above is **sRGB, post-tonemap**, so a correction reasoned in sRGB ratios is wrong. Converting my measured far-field to linear:

> sRGB (191.7, 179.2, 161.6) → linear **(0.5253, 0.4519, 0.3593)** ⇒ **R/G 1.162, B/G 0.795**

⇒ the multiplier that fully cancels it is `(G/R, 1, G/B)` = **(0.860, 1.000, 1.258)**, whose luma weight is **0.9889** — i.e. it re-balances the hue while leaving brightness essentially untouched (a flat grey would not: it would scale all three channels down and simply dim the fog, which is `FOG-§12.4`'s warning and `TASK-1151`'s corollary).

⚠️ **This is a correction to `TASK-1151`'s estimate, and the direction matters.** It predicted a shape of ≈ `(0.88, 1.00, 1.13)`, derived from the 5400 K light's spectrum. Measured from the **rendered pixels**, the blue channel needs **1.26, not 1.13** — the tonemapper compresses the warm end, so the residual cast in the final image is stronger than the light's own chromaticity implies. `TASK-1151` derived its number honestly from the light; the pixels are the authority, and they ask for more blue (`SC-§101` — a prescribed remedy is a claim, and this one needed measuring).


---

## 3. 🚨🚨 THE NAMED CONTRIBUTOR IS REFUTED ON PIXELS — `base Color` IS INERT

I applied the computed correction and measured the result. **It did nothing.**

| `general Data.base Color` | far-field sRGB | R/G | B/G |
|---|---|---|---|
| `(1.00, 1.00, 1.00)` — before | (190.1, 177.9, 158.5) | 1.069 | 0.891 |
| `(0.88, 1.00, 1.21)` — the correction | (189.8, 177.6, 158.0) | 1.069 | 0.890 |
| | (191.6, 179.8, 161.1) | 1.066 | 0.896 |
| | (191.0, 178.9, 159.7) | 1.068 | 0.893 |
| | (191.8, 179.9, 161.5) | 1.066 | 0.898 |

Four frames, two positions, two times: **B/G moved from 0.891 to 0.890–0.898 — inside the frame-to-frame noise of the unchanged fog.** No shift at all.

### The falsification, because a null result deserves a decisive test rather than a shrug

A small tint producing a small effect could be a *weak* lever. So I set `base Color` to the most extreme value available — **pure red `(1, 0, 0)`**:

| | far-field sRGB | R/G | B/G | contrast |
|---|---|---|---|---|
| baseline `(1, 1, 1)` | (190.1, 177.9, 158.5) | 1.069 | 0.891 | 7.17 |
| **pure red `(1, 0, 0)`** | (189.3, 176.9, 157.5) | **1.070** | **0.890** | 7.19 |

⛔⛔ **A fog whose `base Color` is pure red renders EXACTLY the same beige.** Not weaker — *identical*, on hue and on brightness alike (contrast 7.19 vs 7.17). The property read back correctly every time; the render never moved.

⇒ **`BP_SiegeFog` → `general Data.base Color` does not reach the rendered image at all.**

### What this means for `TASK-1151`'s finding — stated precisely, because the diagnosis was otherwise excellent

`TASK-1151` C-1 said the fog's colour is *"`BaseColor` × light"* and concluded *"one change on `BP_SiegeFog.general Data.base Color` moves 100 % of it… No partial shift, no round trip."* That was an **inference from the material's parameter census, not a measurement** — the handoff is honest that its instrument was a property read-back, and `SC-§94`/`FIELD-§7` name exactly this failure mode: **a value that reads correct and renders wrong.**

⚖️ **This is `SC-§101` happening on schedule:** a correct diagnosis (the hue *is* single-sourced from the 5400 K sun; the vendor pack *does* inject zero hue; every colour field *is* neutral — I reproduced all of it) prescribed a remedy that carried an untested assumption, and the finding's credibility is exactly what would have stopped anyone checking. `FOG-§12.4`'s *"verify by rendered pixels, never by a property read-back"* is what caught it, and it earned its place in the law twice over.

⚠️ **What is NOT refuted:** everything `TASK-1151` measured stands — the hue is real (I reproduce B/G 0.902 against his 0.896), it is single-sourced, it arrives with the fog and not from the battlefield (my no-fog control reads green, B/G 0.53), and no vendor or map colour field carries it. Only the **prescribed lever** is refuted.

### Why it is plausibly inert — recorded as a lead, NOT as a finding

`TASK-1151` §5 note 8 observed that `MI_FogArea_Box` reports `blendMode = BLEND_Additive`. An additive surface contributes what it **adds**; a diffuse albedo term has nothing to multiply. That is consistent with `base Color` being unused in the `Base` mode this fog runs in — but I did **not** trace the material graph to prove it, and I am not going to assert a mechanism I did not measure. ⛔ **Lead, not finding.**


---

## 4. ⭐⭐ THE MECHANISM, FOUND — the vendor Blueprint never transports the colour

A null result is only half an answer, so I opened the live actor and read what the material actually receives. `BP_SiegeFog` runs `material Mode = Dynamic`: it builds a **`MaterialInstanceDynamic`** (`MID_MI_FogArea_Box_0`) from `boxMaterials[Base]` and writes the struct into it. I read that MID's parameter arrays directly, **while the BP struct held values I had just set**:

| MID parameter | value in the MID | value in the BP struct | transported? |
|---|---|---|---|
| `Density` | **0.5** | 0.5 | ✅ |
| `Base Noise Size` | **2.5** | 2.5 | ✅ |
| `Base Noise Sharpness` | **0.1** | 0.10 | ✅ |
| `Wind Speed` | **0.005** | 0.5 (×0.01) | ✅ |
| `Mask Margin` | **3** | 3 | ✅ |
| **`Emissive Color`** | **(0.05, 0.05, 0.05)** | **(0, 0, 0.5)** ← I had just set this | ⛔ **NO** |
| **`Base Color`** | (1, 1, 1) | — | ⛔ **never written** |

⇒ ⭐⭐ **The vendor Blueprint pushes every SCALAR from the struct into the material and pushes NO COLOUR at all.** The colour fields it exposes on `S_FogAreaGeneral` are decorative: they read back perfectly, they are never transported, and nothing anywhere reports it. That is why `base Color` was inert — not because the material ignores colour, but because **the value never arrives.**

### And the material's colour parameters are alive — proven by writing them directly

I wrote the MID's own parameters at runtime and captured:

| MID parameter set directly | far-field sRGB | R/G | B/G |
|---|---|---|---|
| baseline (`Emissive` 0.05 grey, `Base` white) | (189.2, 177.9, 159.3) | 1.063 | 0.895 |
| **`Emissive Color` = (0, 0, 1)** | (184.7, 171.7, **199.9**) | — | **1.164** |
| **`Base Color` = (1, 0, 0)** | (193.8, **55.2**, 44.6) | 3.508 | — |

Both parameters move the render violently. ⇒ **the material is fine; the transport is the defect.** And the two levers do *different* jobs, which the fix depends on:
- **`Base Color` is multiplicative** — it can pull the red down (it cannot add blue: ×1.26 on B produced no change).
- **`Emissive Color` is additive** — it is the only way to *add* blue, and it cannot subtract.

⇒ neutralising this cast needs **both**: `Base Color` to cut R, `Emissive Color` to lift B.

### The fix, and why it is an MI rather than a struct edit

Since the BP never writes the colour into the MID, a colour baked into the **source material** survives untouched into the MID. So:

1. **`/Game/Materials/Instances/MI_SiegeFog_Grey`** — **new, ours**, a child of the vendor `MI_FogArea_Box` (referenced as a **parent only**; the vendor asset is never opened, edited or re-saved, per `FOG-§6`). Named per this row's `names` line (`MI_SiegeFog_<Purpose>`).
2. **`BP_SiegeFog.boxMaterials.Base`** repointed from the vendor `MI_FogArea_Box` to `MI_SiegeFog_Grey`.

The scalars still flow from the BP struct exactly as before, so 🧑 he keeps `density`/`sharpness`/`wind Speed` as live dials on the Blueprint; only the colour now comes from the MI.

⚠️ **No new shader permutation:** only vector parameters are overridden — no static switch, no graph edit ⇒ **no material recompile** (`TASK-239`).

### Choosing the value — measured, three candidates, one proposed

All three measured on pixels at the `VID-007` vantage, on the final `TASK-1152` density:

| candidate | `Base Color` | `Emissive Color` | far-field sRGB | R/G | B/G | saturation | luma |
|---|---|---|---|---|---|---|---|
| **before** | (1, 1, 1) | (0.05, 0.05, 0.05) | (189.2, 177.9, 159.3) | 1.063 | **0.895** | **14.7 %** | 179.5 |
| t1 (fully neutral) | (0.86, 1, 1) | (0.05, 0.05, 0.33) | (182.6, 178.3, 177.7) | 1.024 | 0.996 | 2.7 % | 179.5 |
| ⭐ **t2 — PROPOSED** | **(0.88, 1, 1)** | **(0.05, 0.08, 0.26)** | (180.3, 176.3, 169.7) | **1.022** | **0.963** | **5.9 %** | 176.7 |
| t3 (warmer) | (0.90, 1, 1) | (0.05, 0.06, 0.20) | (181.5, 175.5, 165.9) | 1.034 | 0.945 | 8.6 % | 176.2 |

⭐ **I propose t2**, and the reasoning is `FOG-§12.4` cl. 3 rather than arithmetic: 🧑 he asked for ***"more of a grey"***, not for grey. **t1 is available and reaches a dead neutral (B/G 0.996)** — I did not take it, because a fog with no hue at all on a green, sunlit battlefield reads as a flat slab. **t2 cuts the saturation by 60 % (14.7 % → 5.9 %) and moves B/G from 0.895 to 0.963** while costing **1.6 % of brightness** — unmistakably grey next to the before, with a trace of warmth left so it still reads as weather.

🧑 **t1 and t3 are the dial either side, already measured** — `TASK-1159` can move one notch in either direction without anyone re-deriving anything.


---

## 5. ⛔⛔ WHAT I BUILT, AND THE ONE STEP I COULD NOT TAKE — the colour is NOT shipped

**`/Game/Materials/Instances/MI_SiegeFog_Grey` is created, correct and SAVED**, parented to the vendor `MI_FogArea_Box`, read back after the write:

| parameter | value on the MI (read back) |
|---|---|
| `Base Color` | **(0.88, 1.00, 1.00, 1.00)** |
| `Emissive Color` | **(0.05, 0.08, 0.26, 1.00)** |

⛔ **But it is NOT referenced by anything, so the fog still renders the old beige.** The remaining step is a single field — `BP_SiegeFog` → `boxMaterials["Base"]` must point at `MI_SiegeFog_Grey` instead of the vendor `MI_FogArea_Box` — and **I could not write it through the MCP surface.** Two routes, both attempted, both measured:

| route | result |
|---|---|
| `ObjectTools.set_properties` on the CDO's **`boxMaterials`** `TMap` | ⛔ **silent no-op.** Tried the map with every entry as a plain string and again with every entry as a `refPath` dict; both calls returned **without error** and the read-back still showed the vendor MI. *(My script's post-write assertion caught this and **refused to save** — otherwise it would have saved a Blueprint whose colour route silently did nothing.)* |
| CDO **`material`** override slot (was `None`) | ⛔ **stored but ignored.** The property took (read-back confirms `MI_SiegeFog_Grey`), but a fresh spawn still built **`MID_MI_FogArea_Box_0`** — a MID of the *vendor* MI — and the pixels were unchanged: far-field **(188.0, 175.4, 156.3), B/G 0.891** against 0.895 before. The construction script uses `boxMaterials[mode]` and does not consult `material`. |

⇒ 🚨 **ASK (C) IS NOT DELIVERED.** The cause is diagnosed, the lever is found, the value is measured and the asset is built — but the fog Jonathan sees is **still the same beige**, and I am not going to describe a prepared asset as a shipped fix.

### What finishes it — one field, seconds of work

> Open `Content/Blueprints/BP_SiegeFog` → **Class Defaults** → **`Box Materials`** → key **`Base`** → set to **`MI_SiegeFog_Grey`**. Save. Nothing else changes.

That is a hand edit in the Blueprint editor (🧑 Jonathan or build-master), **or** a one-line programmer change. The scalars keep flowing from the struct, so `density` / `sharpness` / `wind Speed` remain live dials.
⚠️ **Whoever does it must re-verify on PIXELS** — the far-field B/G should move **0.895 → ~0.963**. This row exists because a colour that read correct rendered wrong; do not accept a property read-back as proof.

### State I left behind, exactly

- ✅ `MI_SiegeFog_Grey` — **created and saved**, values verified, **referenced by nothing** (harmless; it renders nowhere until wired).
- ✅ `BP_SiegeFog` — carries **only** `TASK-1152`'s uniformity change. The `material` slot I set during the experiment was **reverted to `None`** and the asset re-saved, so no half-finished colour wiring is on disk.
- ⛔ No colour change is live. `TASK-1152`'s uniformity fix is unaffected and remains fully verified.


---

## 5. Fences

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` | **`1F78419D…0AF15622` before AND after — identical**, mtime still `2026-09-05 00:42:58`. Never saved. |
| ⛔ `DirectionalLight_0` (the 5400 K sun) | **NOT touched.** It is the origin of the hue, and re-lighting the world to fix the fog would change everything else he likes — and `GFX-§11` forbids saving the map anyway. Diagnosed by `TASK-1151`, deliberately not the lever. |
| ⛔ `SkyLight_0`, `SkyAtmosphere_0`, `PostProcessVolume_0`, `ExponentialHeightFog_0` | **NOT touched.** No map actor was modified. |
| ⛔ `Content/FogArea/**` (vendor) | **untouched, unopened.** `BP_FogArea` never opened; `M_FogArea` / `MF_Fog` never edited. No MI created. |
| Material recompile (`TASK-239`) | **none.** A colour parameter on a BP struct; no material graph, no static switch. |
| `Source/**` · `Config/**` | **zero bytes.** |
| Save | `save_assets(["/Game/Blueprints/BP_SiegeFog"])` — by explicit path, never `save_assets([])`. |
| Commit / push | **none.** `TASK-1158` hosts. |

## 6. What I did NOT measure — plainly (`SC-§94`)

1. ⛔ **This is one proposed value, not a settled one.** `FOG-§12.4` cl. 3 is explicit that *"more of a grey"* is relative and 🧑 **his eye is the instrument** — that is `TASK-1159`. I moved it most of the way to neutral and deliberately left a trace of warmth rather than driving R/G and B/G to exactly 1.000; a fog with zero hue on a green, sunlit battlefield reads dead.
2. ⛔ **Measured on the far-field wash only.** That band is the purest sample of the fog itself (opaque, no terrain showing through). I did not characterise the hue of the *near* field, where fog and grass mix and the grass's own green dominates.
3. ⛔ **No unit or hero in frame.** Simulate has no pawns, so I did not check the tint against skin, tabards or team colours — and team-colour readability is a competitive concern, not just an aesthetic one. Worth his eye at `TASK-1159`.
4. ⛔ **One lighting condition.** Measured under `DirectionalLight_0` as it stands, at its current pitch. `BrightSun` was not cast; if that card changes the sun, the fog's rendered hue moves with it and this correction is calibrated for the default.
5. ⛔ **The tonemapper is non-linear**, so a linear multiplier cannot cancel a tint exactly at every luminance. I corrected against the measured far-field band; darker or brighter regions of the fog will retain slightly different residuals.
6. ⛔ **`general Data.emissive Color` (0.05 neutral grey) left alone.** `TASK-1151` named it as a secondary lever. It adds a flat, hueless lift; changing it would have altered brightness as well as colour, and one lever was enough.


## 7. For integration (`TASK-1158`)

- **Assets touched by THIS row:** `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` (**new**). `Content/Blueprints/BP_SiegeFog.uasset` carries **only** `TASK-1152`'s change — my experimental `material` write was reverted to `None` and the asset re-saved.
- ⛔ **This row ships NO visible change.** Do not describe ask (C) as fixed. `MI_SiegeFog_Grey` is a **prepared part**, not a delivered repair, and it is referenced by nothing.
- ⚖️ **Routing note for the manager:** `TASK-1151` ruled ask (C) **asset-side only**, and that ruling **still holds** — the fix genuinely is an asset change, and it is built. What is missing is a **tooling** step, not a different owner. `TASK-1155` (the code lane) was correctly closed: the `ExponentialHeightFog` actor is still not a colour contributor, and nothing measured here revives it.
- 🧑 **`TASK-1159`** should judge the colour only **after** the one-field wiring lands; until then there is nothing new for his eye to look at. The uniformity work (`TASK-1152`) *is* ready for his eye now.
