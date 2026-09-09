# TASK-1151 — [FOGLOOK-DIAGNOSE] — art-director handoff

**DIAGNOSE-ONLY. ZERO WRITES executed.** One editor session, PID 14188, MCP `http://127.0.0.1:8000/mcp` answered.
Date 2026-09-08 · blocker `VID-007` read in full before touching the editor.

## Fence report (verify me first)

| Fence | Result |
|---|---|
| `L_Arena.umap` SHA-256 **BEFORE** | `1F78419D888940739C949A60B29EE43451C464915F7AB37942B921FE0AF15622` |
| `L_Arena.umap` SHA-256 **AFTER** | `1F78419D888940739C949A60B29EE43451C464915F7AB37942B921FE0AF15622` — **IDENTICAL**, mtime still `2026-09-05 00:42:58` |
| `git status --porcelain Content/` | **empty** — zero `.uasset`/`.umap` touched |
| `set_properties` / `save_actor` / `save_assets` / `recompile` | **never called** (`TASK-239` respected) |
| `BP_FogArea` asset editor | **never opened** (`OpenEditorForAsset` never called on anything) |
| PIE/Simulate | `IsPIERunning` = **false** before, and I started none |
| Viewport camera | unchanged — every capture used `captureTransform`, which does not move the viewport. Before **and** after: `(-20607.816, -0, 98.15)` / `(0, 180, 0)` |
| Editor state at exit | **UP, PID 14188**, level `/Game/Maps/L_Arena`, no play session, nothing dirtied by me |

Writes actually made: this file, one promoted PNG, and one `- status:` line on the board. Nothing else.

---

## Instruments used, and what each can and cannot see

1. **Property read-back via Unreal MCP** on the live editor (`ObjectTools.get_properties`, `MaterialTools.get_expressions`, `EditorAppToolset.SearchCVars`). Sees authored values. ⛔ **Cannot** tell me what renders — `FIELD-§7` / `TASK-1109`.
2. **Rendered pixels, game side** — the six `VID-007` promoted PNGs at `.claude/pipeline/playtest-evidence/2026-09-08/`, re-sampled by me with PIL/numpy for **per-channel RGB** (the analyst reported luma only). These are the actual frames Jonathan judged, at Epic with `r.VolumetricFog:1`.
3. **Rendered pixels, editor side** — `EditorAppToolset.CaptureViewport` at the hero vantage `VID-007` derived. ⭐ The editor world contains the `ExponentialHeightFog` but **not** `BP_SiegeFog` (that actor is runtime-spawned by `RefreshFogVisual()`), so this capture is a **clean isolation of the engine fog with the vendor fog subtracted**.
   ⚠️ Editor render is at `GridPixelSize 16 / GridSizeZ 64`; the recording ran `8 / 128`. Those are *quality* knobs, not density — but declare it.
4. ✅ **POSITIVE CONTROL (`SC-§39`), passed:** in one editor frame the same sampler reads mid-band grass at **B/G = 0.618** and the sky band at **B/G = 0.919** — a 49 % separation in the blue/green ratio inside a single image. The instrument demonstrably resolves hue *before* I use it to say where hue comes from.
5. ⛔ **What my instrument cannot see:** I never rendered `BP_SiegeFog` myself (spawning it would dirty `L_Arena`, which `GFX-§11` forbids). Every claim about the vendor fog's *rendered* behaviour comes from Jonathan's own recording, not from a render I commissioned. Nor did I measure a frame-time cost.

---

# 1. ASK (B) — WHERE THE 4× SWING COMES FROM

## ⭐ The measurement that reframes it: the far field never changes; only the near field does

Per-channel RGB, re-sampled by me from the `VID-007` frames. "far-field" = scene rows 25–40 %, "near ground" = rows 80–95 %.

| frame | far-field RGB | near-ground RGB |
|---|---|---|
| control t=3.1 (**no fog card yet**) | (106.6, 116.7, 74.6) — **green** | (97.3, 110.5, 83.9) — green |
| pole B t=43.0 (near-clear) | (185.5, 176.0, 158.1) | (103.2, 110.9, 65.1) — **green, ground visible** |
| step-before t=47.6 (clear) | (187.0, 177.6, 159.6) | (114.7, 118.7, 73.7) — green |
| **reference t=27.0 (his target)** | (190.6, 181.1, 162.2) | (146.7, 141.1, 109.3) — half-washed |
| step-after t=47.8 (opaque) | (192.7, 183.1, 164.3) | (165.5, 155.8, 130.4) — washed |
| pole A t=20.0 (whiteout) | (191.7, 182.2, 163.3) | (168.1, 158.1, 134.3) — washed |

🚨 **The far-field wash is the SAME in all five fogged frames — R spans 185.5…192.7, a 3.9 % range across a swing `VID-007` measured at 4× in distance and 7× in contrast.** The fog is fully saturated at the far field in *every* frame, including the two he called "not even there". **100 % of the swing lives in how close to the camera the fog reaches that same opacity.** ⇒ this is not "sometimes there is fog and sometimes there isn't"; it is **one fog whose extinction coefficient at the camera swings**, exactly as `FOG-§12.3` insists ("ONE distribution, not two bugs").

## The contributors, ranked, each with its instrument

| # | contributor | authored value | vendor default | side | verdict |
|---|---|---|---|---|---|
| 1 | `BP_SiegeFog` → `general Data.density` | **5** | `MF_Fog:Density` = **1** | **ASSET** | ⭐⭐ **5× the pack's own default.** Read-back. Sets mean opacity; at 5 the far field saturates everywhere, which is exactly what the table above shows. |
| 2 | `MF_Fog:Base Noise Intensity` | **1** (not exposed on the BP struct) | 1 | **ASSET** (vendor material ⇒ reachable via an MI or the child BP only) | ⭐⭐ **Full-amplitude multiplicative density noise.** A 0…2× modulation of a mean that is already 5× ⇒ a ≥4× swing in visible distance is the *arithmetically expected* result, not a surprise. |
| 3 | `BP_SiegeFog` → `noise Data.sharpness` | **0.35** | `Base Noise Sharpness` = **0.15** | **ASSET** | ⭐⭐ **We more than DOUBLED the blob-edge hardness.** This is what turns a gradient into a wall. Read-back. |
| 4 | `BP_SiegeFog` → `general Data.wind Speed` + `wInd World Space` | **1**, **true** | `Wind Speed` = **0.5**, `Wind Direction` = (1,0,0), `Global Wind` = 0 | **ASSET** | ⭐⭐⭐ **We DOUBLED the pan speed, and it pans in WORLD space.** `MF_Fog` contains **2 `MaterialExpressionTime` nodes** (census, measured). ⇒ **the density field moves through the world**, so density at a standing camera changes with **time**. This is the only contributor that explains `VID-007`'s **≤ 0.2 s / ≤ 150 uu step at identical `14 FPS · 70.4 ms` and identical `Gold: 989`.** |
| 5 | `BP_SiegeFog` → `noise Data.scale` | **2.5** | `Base Noise Size` = **3** | **ASSET** | Sets blob size. ⚠️ The scalar is unitless in the graph; I did **not** convert it to a cell size in uu. |
| 6 | `MF_Fog:Near Camera Fade Distnace` (vendor's typo) | **200** | 200 | **ASSET** (MI param) | ⭐ The pack's *only* near-camera protection ends at **200 uu**, and `MF_Fog` reads `CameraPositionWS` to apply it. Jonathan's hero sits at **400 uu** on the boom ⇒ **outside the fade.** This is why pole A can extinguish his own character, and it is the seat of `VID-007`'s "floor on near-camera clarity" ask. |
| 7 | World→Local sampling at the forced `(640, 360, 260)` scale | `MF_Fog:TransformPosition_0` = `TRANSFORMPOSSOURCE_World` → `TRANSFORMPOSSOURCE_Local`, and it **is** in a `TextureSample` coordinate chain | — | **ASSET** | ⚠️⚠️ **CANDIDATE, NOT ESTABLISHED.** The node exists and would stretch whatever it feeds by **640×/360×/260× — anisotropically, 2.46:1 in X:Z**. ⛔ **I did not establish whether it feeds the Base Noise or the Shape mask.** `VID-007`'s C1 is therefore **neither confirmed nor refuted**; see *decisive next measurement* below. |
| 8 | `VT_Noises` virtual texture (the `Base Noise Texture` default; `BP_SiegeFog` sets its own override to `None`, so the material default is what samples) | — | — | **ASSET/ENGINE** | ⚠️ **CANDIDATE.** `VID-007` crop-verified `2798.546 MB over budget` in every frame. VT tile streaming under a 2.8 GB overdraft evicts in **steps**. Not measured by me. |
| 9 | froxel grid — `VolumetricFogDistance` | **6000 uu** (re-measured live at my own instant, `SC-§91`) | — | **ENGINE** | ⛔⛔ **REFUTED as an explanation of his poles.** See §3. |
| 10 | volumetric-fog temporal reprojection — `HistoryWeight 0.9`, `TemporalReprojection 1`, `HistoryMissSupersampleCount 4`, `Jitter 1` | live cvars | — | **ENGINE** | ⛔ **REFUTED, by a different route than framerate.** These knobs govern the **engine** volumetric fog only — and §3 measures that system's visible contribution at ≈ 0. A temporal artefact in a system you cannot see cannot produce a 4× swing. (`VID-007` had already refuted it on framerate; this is an independent second refutation.) |

### ⛔ Eliminated outright (measured, so nobody re-opens them)

- **Shape mask** — `BP_SiegeFog.shape Data.bUseShape = false`; `Shape Texture` default is `/Engine/EngineResources/WhiteSquareTexture` ⇒ a constant 1. **No spatial variation.**
- **Distortion / curl** — `Distortion Intensity = 0` on **both** the BP struct **and** the `MF_Fog` default. `VT_Curl_Low` is sampled but multiplied by zero.
- **Self-shadows, light shafts, distance field** — `BP_SiegeFog.mode = "Base"`, and `MF_Fog`'s static switches `bUseShadows` / `bUseLightShafts` / `bUseDistanceField` all default **false**. All three structs (`S_FogAreaShadows`, `S_FogAreaShafts`, `S_FogAreaField`) are **inert**. Do not tune them.
- **Lighting** — already refuted on pixels by `VID-007`; I add that the sun is a single `Movable` `DirectionalLight` at pitch −38° with `castShadows` on, unchanged all session.

## CAUSE — ASSET-SIDE ✅ (ask B)

**Established.** The whole visible fog — every pixel of the wash in all five fogged frames — is produced by `BP_SiegeFog`'s vendor raymarch material, and the swing is produced by **its own authored density parameters**, three of which we ourselves pushed past the vendor's defaults:

> `general Data.density` **5** (vendor 1) · `noise Data.sharpness` **0.35** (vendor 0.15) · `general Data.wind Speed` **1** (vendor 0.5, panning in **world** space)
> — on top of `Base Noise Intensity = 1`, a full-amplitude multiplicative modulation, and with the pack's only near-camera protection (`Near Camera Fade Distnace = 200`) ending **inside** the 400 uu boom.

⇒ ⭐ **`TASK-1152` (FOGUNIFORM-ART) FIRES.**

## CAUSE — ENGINE-SIDE ⛔ (ask B) — **absent, and measured absent**

There is **no** engine-side cause for the density variation. The `ExponentialHeightFog` + froxel volumetric fog are on and configured, and their **rendered** contribution at the player's vantage is ≈ 0 (§3, on pixels). Nothing in `FOG-§12.2`'s seam can move a fog the player cannot see.

⇒ ⛔ **`TASK-1153` (FOGUNIFORM-CODE) DOES NOT FIRE.** Per its cl. (0) it should close with: `unnecessary — cause is asset-side, see TASK-1152`, quoting this section. ⇒ its gate **`TASK-1156` closes with it.**

## 🚨 The decisive next measurement, named (`FOG-§12.3` cl. 0 — I will not adopt what I did not measure)

I have **not** separated contributors 3+4 (noise amplitude/sharpness — a *spatial* field) from contributor 4's **pan** (a *temporal* field), and `VID-007` is explicit that "the fog is patchy in space" is not established. The one measurement that separates them, and it is cheap:

> **Park a camera at ONE fixed world position with the fog up and sample the same pixel band every 0.25 s for 20 s.**
> · Contrast swings while the camera never moves ⇒ **temporal** ⇒ the fix is `wind Speed` (and/or `Global Wind`).
> · Contrast holds flat ⇒ **spatial** ⇒ the fix is `Base Noise Intensity` / `sharpness` / `scale`.
> · Both ⇒ both, and `TASK-1152` must say which share it spent its budget on.

`TASK-1152` should run this **before** it changes a value, not after. My ranking above is ordered by strength of evidence, not by an assumption about which of the two wins.

---

# 2. ASK (C) — WHERE THE YELLOW COMES FROM: **ALL** CONTRIBUTORS, WITH SHARES

## The colour, measured on the actual frames he judged

The purest sample of the fog itself is the far-field band, where the wash is opaque and no terrain shows through. Across all five fogged frames it is **sRGB ≈ (190, 181, 162)** — **R/G = 1.05, B/G = 0.895**; saturation `(max−min)/max` = **14.7 %**; hue ≈ **40°**, i.e. a **15 %-saturated yellow-beige**. Against the fog-free control in the same clip, **(101, 108, 78)**, R/G **0.930**, B/G **0.722** — green-dominant grass. So the wash is a real warm overlay, not the terrain showing through.

That number is the subject of his sentence. It is also *why* he said "more of a grey": at 15 % saturation the hue is unmistakable but shallow.

## The full contributor table — every source, its share, and who can reach it

| # | candidate source | authored value (read live) | share of the HUE | code-reachable? | verdict |
|---|---|---|---|---|---|
| C-1 | `BP_SiegeFog` → `general Data.base Color` | **(1, 1, 1, 1)** — pure white | **0 % injected · 100 % of the CONTROL** | n/a — art, `Content/Blueprints/BP_SiegeFog.uasset`, ours (`FOG-§6`) | ⭐⭐ **It holds no hue, but the fog's colour is `BaseColor × light`, so this is the one multiplicative knob that can cancel the tint without touching a map actor, the vendor pack, or the sun.** |
| C-2 | `BP_SiegeFog` → `general Data.emissive Color` | **(0.05, 0.05, 0.05, 1)** — neutral grey | **0 %** | n/a — art | Adds a flat lift, no hue. A second, weaker lever. |
| C-3 | vendor material `M_FogArea` + `MF_Fog` colour params | `Base Color` **(1,1,1)** · `Emissive Color` **(0,0,0)** · `Shadows Color` **(0.02,0.02,0.02)** · `Distance Field Color` **(0.25,0.25,0.25)** · the 3 `Constant3Vector`s are **(1,0,0)/(0,0,1)/(1,0,0)** = channel masks, not colours | **0 %** | n/a — vendor, **READ-ONLY** (`FOG-§6`) | ⭐ **Census complete: there is not one non-neutral colour anywhere in the vendor material.** The pack injects zero hue. Nothing to fix here, and nothing may be edited here anyway. |
| C-4 | `L_Arena`'s **`ExponentialHeightFog_0`** | `fogInscatteringLuminance` **(0,0,0)** · `directionalInscatteringLuminance` **(0,0,0)** · `volumetricFogAlbedo` **(1,1,1)** · `volumetricFogEmissive` **(0,0,0,0)** · `fogDensity` **0.012** · `startDistance` **10000** · `volumetricFogDistance` **6000** · `bEnableVolumetricFog` **true** | 🚨 **≈ 0 %** | would need code (`GFX-§11`) — **but it does not need anything** | ⛔⛔ **NOT A CONTRIBUTOR, and this is settled on PIXELS, not on the read-back.** Every colour field on it is black or white, **and** the editor capture (this actor live, `BP_SiegeFog` absent) shows the enemy castle **crisp at ~50,000 uu** with no wash at any depth. See the promoted PNG. |
| C-5 | **`DirectionalLight_0`** — `bUseTemperature` **true**, `temperature` **5400 K**, `lightColor` (1,1,1), intensity 11, `bAtmosphereSunLight` true, pitch −38° | 5400 K → linear sRGB **(1.000, 0.841, 0.730)**, **R/G 1.189, B/G 0.868** (Planck × CIE 1931, computed) | ⭐⭐ **dominant — ~100 % of the blue deficit** | map actor, not the height fog ⇒ **outside both 1154 and 1155** | 🚨 **THIS IS THE YELLOW.** Against D65 the same computation gives R/G 1.059 / B/G **1.051**. Switching the sun to 6500 K would neutralise the fog — **and would also recolour the entire battlefield, which he did not ask for.** ⇒ diagnosed, **not** the lever. |
| C-6 | **`SkyLight_0`** — `SLS_CapturedScene`, `bRealTimeCapture` true, `lightColor` (0.800, 0.871, 1.000), intensity 1 | its *tint* is blue (B/G 1.148) but the **sky it captures is warm** — measured sky band **(97.5, 87.2, 80.1)**, R/G 1.119, B/G 0.919 | **minor, and warm on net** | map actor | ⚠️ Reading `lightColor` alone would say "the skylight cools the fog". The captured sky says otherwise. Recorded so nobody makes that mistake. |
| C-7 | **`SkyAtmosphere_0`** with `bAtmosphereSunLight = true` | sun at 38° elevation ⇒ modest transmittance warming, on top of C-5 | small, same direction as C-5 | map actor | Reinforces C-5. Not separable from it without a render I did not commission. |
| C-8 | **`PostProcessVolume_0`** (unbound, priority 1, weight 1) | `bOverride_WhiteTemp` **false** · `sceneColorTint` (1,1,1) · `colorGain`/`colorOffset`/`colorGamma` all neutral · `colorSaturation` **0.9** · `colorContrast` 1.05 · `autoExposureMin = Max = 1` (**fixed exposure**) · `autoExposureBias` 0.4 | **0 % injected** — and saturation 0.9 *reduces* what is there | `Config`/code | ⛔ Cleanly eliminated. Grading is hue-neutral. |

### 🚨 The interaction `FOG-§12.4` warned about — answered

> *"if MORE THAN ONE of the three contributes, changing only one yields a PARTIAL colour shift that reads as 'still a bit yellow'."*

**It does not happen here, and here is why, stated so the fixer can rely on it:** the hue is **single-sourced** (the 5400 K sun, C-5, reinforced by C-6/C-7), and it reaches the player through **exactly one carrier** — `BP_SiegeFog`'s white `base Color`. There is no second fog painting a second yellow. ⇒ **one change on `BP_SiegeFog.general Data.base Color` moves 100 % of it.** No partial shift, no round trip.

The corollary the fixer must not miss: **because `base Color` is a multiplier and not the hue's origin, "set it to grey (0.5,0.5,0.5)" does nothing but darken.** To neutralise, it must be tinted *against* the light — a cool bias roughly reciprocal to (1.000, 0.841, 0.730), i.e. of the shape **(0.88, 1.00, 1.13) normalised**, then judged on pixels and handed to Jonathan's eye (`TASK-1159`). `FOG-§12.4` cl. 3's warning against a dead neutral is exactly right and applies doubly here.

## CAUSE — ASSET-SIDE ✅ (ask C)

**Named contributor: `BP_SiegeFog` → `general Data.base Color`, currently `(1, 1, 1, 1)`** (with `general Data.emissive Color`, currently `(0.05, 0.05, 0.05, 1)`, as a secondary lever). Both live on `/Game/Blueprints/BP_SiegeFog` — **ours**, freely editable, outside `Content/FogArea/**`.

⇒ ⭐ **`TASK-1154` (FOGGREY-ART) FIRES**, for that contributor.

## CAUSE — ENGINE-SIDE ⛔ (ask C) — **absent, measured absent**

The `ExponentialHeightFog` actor is **not** a colour contributor: every colour field on it is pure black or pure white, and — decisively, on pixels — it renders no wash at all at the player's vantage (§3). `TASK-1155`'s trigger is mechanical and it is **not** met.

⇒ ⛔ **`TASK-1155` (FOGGREY-CODE) DOES NOT FIRE.** Per its cl. (0): `unnecessary — colour is asset-side, see TASK-1154`. ⇒ its gate **`TASK-1157` closes with it.**

⚠️ **This is NOT a "mixed cause".** I am saying so explicitly, as cl. (5) requires: for ask (C) the cause is **asset-side only**, and `TASK-1154` runs alone.

---

# 3. THE FROXEL GRID AT THIS ARENA'S SCALE

## What the grid actually covers

| quantity | value | source |
|---|---|---|
| `ExponentialHeightFog_0.volumetricFogDistance` | **6000 uu** | re-measured live this session (`SC-§91`) — confirms the `6000` transcribed at `FogVolume.h:613` as `FogVisualHorizontalMarginUU` is **still accurate**; the falsifier that comment names has not fired |
| `r.VolumetricFog` | **1** | live cvar |
| `r.VolumetricFog.GridPixelSize` / `GridSizeZ` | **16 / 64** *(this editor session)* — the recording ran **8 / 128** at Epic | live cvars vs `VID-007`'s log |
| `r.VolumetricFog.DepthDistributionScale` | **32** (exponential slice distribution) | live cvar |
| `r.VolumetricFog.HistoryWeight` / `TemporalReprojection` / `HistoryMissSupersampleCount` / `Jitter` | **0.9 / 1 / 4 / 1** | live cvars |
| arena half-extent, derived from the achieved fog scale | **26,000 × 12,000 uu** ⇒ field ≈ **52,000 × 24,000 uu** | `(640,360,260) × 50 uu` minus `FogVisualHorizontalMarginUU`; Z checks exactly: `260×50 = 13,000 = (20,000+6,000)/2` and centre `Z = 7,000`, matching the logged `ACHIEVED Z=7000` |
| `BP_SiegeFog` box, world size | **64,000 × 36,000 × 26,000 uu**, centred on the arena at **Z = 7,000** ⇒ spans **Z −6,000 → +20,000** | same derivation |
| player camera | **~176 uu** above ground, i.e. **6,176 uu above the box's floor and 19,824 uu below its lid** | `VID-007`'s validated pinhole model |

**So: the froxel grid reaches 6,000 uu — 11.5 % of the field's 52,000 uu length, and 24 % of the fog box's 26,000 uu height.** On paper that is a dramatic mismatch, and it is exactly the shape of argument that `FOG-§12.3` cl. (1a) warned would repeat the LOD mistake with a different noun.

## 🚨 And it does not matter, because the system it bounds is invisible

`FOG-§12.3` cl. (1a) asked one question — *does `6000` uu against a 50,000-uu field explain his two poles, or does it not?* — and forbade "it is consistent with". **Measured answer: it does not.**

**Evidence — `.claude/pipeline/playtest-evidence/2026-09-08/TASK-1151-editor-heightfog-only-castle-crisp-at-50000uu.png`.** Editor world, hero vantage `(-24192, 0, 900)` pitch −6°, `ExponentialHeightFog_0` live with volumetric fog on, `BP_SiegeFog` absent by construction. The enemy castle at the far end of the field — roughly **50,000 uu** from the camera — resolves individual red roof cones, crenellations, the gate arch and the two gold units flanking it; the boulder scatter, the mown-stripe pattern in the grass and sharp directional cast shadows are all legible. Sampled: mid-band **(131.3, 134.0, 82.7)**, B/G **0.618** — grass green, **no wash of any kind, at any depth**.

The reason is on the read-back and is unsurprising once seen: `fogDensity` **0.012** with `startDistance` **10,000 uu** means the non-volumetric height fog contributes nothing inside 100 m, and the volumetric term at that density is below the noise floor. `VID-007`'s own in-clip control (t = 3.1 s, before the Fog card, contrast 25.5, featureless 2.9 %) says the same thing from the game side.

⇒ **Three consequences, and they are the load-bearing part of this section:**
1. `VolumetricFogDistance = 6000` **cannot** explain the poles. Do not spend `TASK-1153` on it.
2. Every temporal-reprojection cvar (`HistoryWeight 0.9`, `HistoryMissSupersampleCount 4`) governs **only** this invisible system ⇒ `VID-007`'s C3 is refuted a second time, independently of framerate.
3. ⭐ **`BP_SiegeFog` is a translucent raymarched mesh, not a froxel participant.** Its density is computed in `MF_Fog`'s own ray march (5 `WorldPosition` nodes, 1 `CameraPositionWS`, 2 `Time`, one volume-texture sample). **It is not in the grid at all.** Widening the grid would cost frame time — on a machine `VID-007` crop-verified at **2798.546 MB over VRAM budget** — and change nothing he can see.

⚠️ **One real seam, recorded but not his complaint:** `r.SupportExpFogMatchesVolumetricFog = 0` (project config) plus `startDistance = 10,000` leaves a band from **6,000 → 10,000 uu** served by neither the froxel fog nor the height fog. Far outside the 243–970 uu range in question. Noted so it is not rediscovered as a cause.

---

# 4. CONDITIONAL ROWS — FIRE / DO NOT FIRE

| row | fires? | the line that triggers it |
|---|---|---|
| ⭐ **`TASK-1152`** [FOGUNIFORM-ART] | ✅ **FIRE** | §1 **`CAUSE — ASSET-SIDE ✅ (ask B)`**. Target = `VID-007`'s **≈ 650 ± 50 uu**. Levers in evidence order: `general Data.density` **5→?** (vendor 1) · `noise Data.sharpness` **0.35→?** (vendor 0.15) · `general Data.wind Speed` **1→?** (vendor 0.5) · `Base Noise Intensity` **1** and `Near Camera Fade Distnace` **200→~450** via an MI. 🚨 **Run the fixed-camera time series in §1 FIRST** — it decides whether you are fixing a spatial field or a temporal one, and spending the budget on the wrong one looks like success in a screenshot. ⚠️ `FOG-§12.3`'s *"roughly"*: leave visible variation; the swing to remove is the near-field extinction distance, not the texture. |
| ⭐ **`TASK-1153`** [FOGUNIFORM-CODE] | ⛔ **DO NOT FIRE** | §1 **`CAUSE — ENGINE-SIDE ⛔ (ask B) — absent, measured absent`**. Close with `unnecessary — cause is asset-side, see TASK-1152`. |
| ⭐ **`TASK-1154`** [FOGGREY-ART] | ✅ **FIRE** | §2 **`CAUSE — ASSET-SIDE ✅ (ask C)`** — named contributor `BP_SiegeFog → general Data.base Color`, currently `(1,1,1,1)`. Before RGB is on record: **(190, 181, 162)**, R/G 1.05, B/G 0.895. ⚠️ Tint **against** the 5400 K light (shape ≈ `(0.88, 1.00, 1.13)` normalised) — a flat grey only darkens. ⚠️ Sequencing per `TASK-1152`'s `parallel-safe`: **uniformity first, colour second**, one editor session, two handoffs. |
| ⭐ **`TASK-1155`** [FOGGREY-CODE] | ⛔ **DO NOT FIRE** | §2 **`CAUSE — ENGINE-SIDE ⛔ (ask C) — absent, measured absent`**. Close with `unnecessary — colour is asset-side, see TASK-1154`. **Explicitly NOT a mixed cause.** |
| `TASK-1156` (gate over 1153) | ⛔ closes with its subject | its own status clause |
| `TASK-1157` (gate over 1155) | ⛔ closes with its subject | its own status clause |
| `TASK-1158` (ship host) | ✅ hosts | this file + `.claude/pipeline/playtest-evidence/2026-09-08/TASK-1151-editor-heightfog-only-castle-crisp-at-50000uu.png`, plus `VID-007` and its six PNGs (`FR-§6`) |

---

# 5. WHAT I DID NOT MEASURE — say it plainly (`SC-§94`)

1. ⛔ **I never rendered `BP_SiegeFog`.** Spawning it in the editor world dirties `L_Arena`, and `GFX-§11` forbids that. Every claim about the vendor fog's *rendered* behaviour rests on Jonathan's own recording.
2. ⛔ **Spatial vs temporal is NOT separated.** Both mechanisms are present and authored above vendor defaults; I did not measure which dominates. Decisive test named in §1.
3. ⛔ **The World→Local `TransformPosition` was not traced to its consumer.** `VID-007`'s C1 stays a **candidate**. Trace `MF_Fog`'s `Base Noise Texture` sample's Coordinates chain and report whether `TransformPosition_0` is in it. (My graph-walk script hit a schema error on unwired pins; a one-node trace in the material editor settles it in a minute.)
4. ⛔ **`noise Data.scale = 2.5` was not converted to a cell size in uu.** Without it, "the blobs are arena-scale" is a picture, not a number.
5. ⛔ **VT/VRAM (C4) not measured.** `Base Noise Texture` defaults to the **virtual** `VT_Noises`, and the session ran 2798.546 MB over budget. Still live, still unmeasured, still owned by the VRAM lane.
6. ⛔ **No frame-time measurement of any kind.**
7. ⚠️ **Editor render quality ≠ recording quality** (`GridPixelSize 16/64` vs `8/128`). Quality, not density — but declared.
8. ⚠️ `MI_FogArea_Box`'s `basePropertyOverrides` mirror reads `blendMode = BLEND_Additive` with `bOverride_BlendMode = false`, i.e. inherited from `M_FogArea`. **An additive fog can only brighten, never darken** — which matches the measurement exactly (luma rises 102 → 176 while contrast falls; whiteout, never blackout). Consistent, but read from a mirror field, so: **corroboration, not a finding.**

---

## Pinned reads (`FOG-§12.5` names, character-for-character)

- `/Game/Blueprints/BP_SiegeFog` — read via `Default__BP_SiegeFog_C`. **Ours.** Not opened in an editor, not saved.
- `Content/FogArea/Materials/M_FogArea` (4 nodes) + `Functions/MF_Fog` (149 nodes) + `Functions/MF_Shapes` (16) + `Base/MI_FogArea_Box` — **READ-ONLY, unchanged.**
- `Content/FogArea/Blueprints/BP_FogArea` — ⛔ **never opened.**
- `L_Arena` actors read (never selected, never saved): `ExponentialHeightFog_0`, `DirectionalLight_0`, `SkyLight_0`, `SkyAtmosphere_0`, `PostProcessVolume_0`, `PlayerStart_0`.

**Law honoured:** `FOG-§12.3` · `FOG-§12.4` · `FOG-§12.5` · `FOG-§12.6` · `FOG-§6` · `GFX-§11` · `GFX-§12` · `FIELD-§7` · `SC-§39` · `SC-§88a` · `SC-§91` · `SC-§94`.
