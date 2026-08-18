# TASK-616 — Lane D: Interior Lighting Audit (file-only) — gameplay-programmer handoff

Date: 2026-08-17 · Status requested: ready-for-qa · Scope fence honoured: NO code/config edits, NO compile, NO editor/MCP, NO git, NO asset writes. The only writes this task made are this handoff. L_Arena was never opened, never touched (read-only binary string scan only).

Jonathan's ask (verbatim): *"fix the lighting inside the castle a little better"* — bounded by G3: **dials, not a relight**. This handoff is the measured audit + a COSTED dial set as INPUT to the manager's repair spec (CF-R1: ⛔ not authorization).

---

## 1. THE MEASURED TABLE — what ships today (every value re-verified at the artifact)

| Quantity | Value | Where verified | Live truth chain (W8-R3) |
|---|---|---|---|
| `TorchIntensity` | **12.0 — in CANDELAS** | `Torch.h:176-177`; units asserted at `Torch.cpp:125` `SetIntensityUnits(ELightUnits::Candelas)` | BP_Torch carries **NO serialized delta** on this field (name-table scan, §3) ⇒ C++ default 12 cd is live |
| `TorchAttenuationRadius` | **1,200 uu** | `Torch.h:191-192`; applied `Torch.cpp:131` | NO BP delta ⇒ 1,200 live |
| `TorchLightColor` | **(1.00, 0.72, 0.42)** warm, sRGB round-trip | `Torch.h:208-209`; applied `Torch.cpp:137` | NO BP delta ⇒ live |
| The 4th EditDefaultsOnly field | **`TorchLightRelativeOffset`**, C++ default ZeroVector = derive-from-mesh-bounds | `Torch.h:235-236` (declared addition per the `Torch.h:88-96` class-doc commentary the spec cites) | **THE ONE FIELD BP_Torch OVERRIDES** (§3) — the TASK-556 `flame_centre_uu` seam landed exactly as designed |
| Units note (load-bearing) | The `Torch.h:154-177` commentary: a candela number in a unitless field would render ~625× too dim. The component is explicitly told Candelas, and project-wide `r.DefaultFeature.LightUnits=1` (candelas) agrees (`DefaultEngine.ini:97`). The units lane is CORRECT — dimness is not a units bug. | | |
| Shadows | **CastShadows = false, asserted on every apply** (`Torch.cpp:145`, WR-§4 law) ⇒ torch light passes THROUGH masonry both ways | | |
| Mount height | `TorchWallMountZ` = 174 + 0.5×1560 = **954** mesh-local (`Castle.cpp:102`); light pool centre **780 above the interior floor** (`Castle.cpp:96-100`) | | |
| Floor-pool radius | √(1200² − 780²) ≈ **911.9 uu** (my recomputation matches the `Castle.cpp:99` ≈912 figure) | | |
| Anchors | **6** (`Castle.cpp:262-289`): hall north wall (−1435, 990), (−465, 990), (+505, 990); hall south wall (−1435, 270); annex east wall (+1680, 780); corridor west wall (−732, −315) — all at Z 954 | | |
| Cap law | `MaxTorchesPerCastle = 6` (`Castle.h:535`); spawn takes `min(TorchAnchors.Num(), max(cap,0))` (`Castle.cpp:561`) — the flagged 7th corridor anchor (+768, −315, 954) yaw 180 is "one array entry plus one cap bump" (`Castle.cpp:285-288`) | | |
| Castle instance overrides | **NONE** — `MaxTorchesPerCastle` / `TorchAnchors` / `TorchClassAsset` do not appear in L_Arena.umap's name table, and **no BP_Castle exists** (`Content/Blueprints/` listing) ⇒ the two level-placed castles run pure C++ defaults for furnishing | | |
| What else lights the interior | **Nothing.** The interior ships with exactly the 6 shadowless torch point lights. Sun/sky/fog/cloud exist in L_Arena (§4) but the shell occludes them except through openings; `r.AllowStaticLighting=False` (`DefaultEngine.ini:69`) ⇒ no bake anywhere; Lumen GI is on (`r.DynamicGlobalIlluminationMethod=1`) so the torches DO get bounce — of a 12-cd source | | |

## 2. THE COVERAGE MATH — geometric coverage is NOT the hall's problem; magnitude is

Union of the 6 floor pools (radius 911.9 uu discs at the anchor XYs) against the three interior floor regions (mesh-local, from `Castle.cpp:104-124`), numeric integration at 10-uu step:

| Region | Area | Inside ≥1 pool (attenuation cut-off) | Inside half-brightness radius (≤598 uu of a pool centre) |
|---|---|---|---|
| hall_main (2910×720) | 2,094,900 uu² | **100.0%** | 83.2% |
| hall_east annex (840×420) | 352,800 uu² | **100.0%** | 94.6% |
| gate_corridor (1500×1410) | 2,115,000 uu² | **58.8% — uncovered fraction 41.2%** | 26.5% |

- The **corridor's east/south portion is the one true coverage hole** — exactly the case the `Castle.cpp:285-288` comment pre-flagged ("the FIRST thing to add if the passage reads dark").
- Hall + annex are 100% covered geometrically. Their darkness is **delivered illuminance**, not coverage:

**Illuminance physics (single torch, light 7.8 m above the floor, Lambertian floor, E = I·cosθ/d²):**

| Floor radius from nadir | E (lux) |
|---|---|
| 0 uu (directly under a torch) | **0.197** |
| 300 uu | 0.160 |
| 598 uu (half-brightness edge) | 0.099 |
| 912 uu (pool edge, before the inverse-square window zeroes it) | 0.054 |

Best case with pool overlap: **≈0.3–0.4 lux**. 0.2–0.4 lux is full-moon level. For scale: 1 cd ≈ one candle — the hall is lit by six 12-candle sconces from 7.8 m up, in a 29-m room. The `Torch.h:159-174` derivation anchored 12 cd to "engine-default point-light brightness at the attenuation edge" — an engine-default point light is a garden lamp (8 cd); the derivation is internally correct and the arithmetic checks, but the anchor itself is orders of magnitude below "torch-lit hall." A real wall torch reads as 80–150 cd. **This is the black floor, quantified.** (Target math: 3 lux at nadir needs ≈183 cd; 60 cd ⇒ 0.99 lux; 100 cd ⇒ 1.64 lux; overlap roughly doubles those.)

**Wall hotspots:** at 0.5 m / 1 m from a torch light the wall receives ≈48 / 12 lux — a **240:1 to 900:1 in-frame ratio** against the 0.05–0.2 lux floor. No single exposure displays that range; something must clip, and the pixels show the floor lost.

## 3. BP_Torch DELTAS (W8-R3 name-table method, file-side)

`Content/Blueprints/BP_Torch.uasset` **EXISTS** (24,333 bytes, modified 2026-08-15 — so `ACastle::ResolveTorchClass`'s BP-first soft path `Castle.cpp:127/:242` should now resolve it; whether the SPAWNED class is actually `BP_Torch_C` is a TASK-617 read, item 3 below). Binary name-table scan:

- `TorchLightRelativeOffset` — **PRESENT** ⇒ serialized override. This is the TASK-556 measured `flame_centre_uu` landing in its designed seam. ⛔ Not a dial; do not touch it.
- `TorchIntensity`, `TorchAttenuationRadius`, `TorchLightColor`, `TorchMeshAsset` — **ABSENT** ⇒ no serialized delta ⇒ **the C++ defaults in §1 are the live values.** (A tagged-property name only enters a uasset's name table when the property serializes as a delta — the same method W8-R3 pinned for DA_BattlefieldScatter.)

Consequence for the dial set: BP_Torch is a **clean, existing, no-compile tuning surface** for intensity/attenuation/colour.

## 4. WHAT ELSE TOUCHES THE INTERIOR PIXEL — the exposure adjudication

**Config (`Config/DefaultEngine.ini:86-91`, verbatim UE5 template-stock block):**
```
r.DefaultFeature.AutoExposure=False
r.DefaultFeature.AutoExposure.Method=0
r.DefaultFeature.AutoExposure.Bias=1.000000
r.DefaultFeature.AutoExposure.ExtendDefaultLuminanceRange=True
r.DefaultFeature.LocalExposure.HighlightContrastScale=0.800000
r.DefaultFeature.LocalExposure.ShadowContrastScale=0.800000
```
Source/ grep: **zero** code-side PostProcess/exposure/skylight/directional-light references — no camera post-process overrides exist in C++.

**L_Arena.umap binary name-table scan (read-only; the map was never opened):** the map contains a **PostProcessVolume** with serialized `bUnbound` + `bOverride_AutoExposureMinBrightness` + `bOverride_AutoExposureMaxBrightness` + `bOverride_AutoExposureBias` (each present; `bOverride_AutoExposureMethod` ABSENT), plus DirectionalLight (actor "LightSource", with a serialized `Intensity` override — value unreadable file-side), SkyLight, SkyAtmosphere, ExponentialHeightFog, VolumetricCloud. `LocalExposure*` names are **ABSENT** from the map ⇒ the two config LocalExposure keys are NOT overridden by any volume.

**Adjudication (on what is file-readable):**
- Exposure in L_Arena is governed by the **map PPV's serialized Min/Max/Bias**, not by the config defaults (a volume override beats a `r.DefaultFeature.*` default; and the config default here is AE-off anyway). The numeric values are binary-encoded and NOT readable by this audit — but this exact cvar block + unbound-PPV-with-Min/Max-override is the UE5 third-person template's stock **fixed-exposure** setup (template ships Min = Max = 1.0 EV100, Bias 1.0). **Expected state: exposure is FIXED, not auto** — the spec's "AE dragged up until walls blow" hypothesis is therefore PROBABLY INVERTED: nothing adapts; the interior floor at 0.2 lux sits ~7 stops under a fixed exposure tuned to the sunlit exterior (the entrance PNG's exterior IS well-exposed, which corroborates fixed) and clips to black, while the bright faces clip white from the other side. ⚠️ PROBABLY — the serialized values are exactly what TASK-617 must read (item 1). If they turn out non-equal (real AE window), the mechanism flips to the spec's original read; either way the DIAL is the same lever (§5-D4).
- **The blown white faces — two candidates, not settled on paper:** (a) sun through openings/carves painting near-white-albedo masonry (neutral-pale, large uniform slabs — matches the shot's geometry; the anti-bleach-era pale albedo is a multiplier here), vs (b) the six warm torch hotspots (48+ lux at <0.5 m, warm-tinted, localized around Z 954 — matches the shot's height band). Discriminating live reads: TASK-617 items 4–5.
- **Cross-lane confound (recorded, not resolved — the board already flags the converse):** the interior PNG's "floor" shows GRASS and a chest-height seam. If lane B's hull-free-column prediction holds, part of the near-black ground pixel is **arena ground at z≈0 seen from below the visual floor plane** — 954 uu under the torch lights (nadir E = 0.13 lux) and sun-occluded by the floor slab above it. The lighting read and the sink read confound each other in BOTH directions; the dial set below is correct regardless, but pixel acceptance of any lighting repair should wait for, or be judged beside, the lane-B repair.

## 5. THE COSTED DIAL SET (G3: dials, not a relight; CF-R1: input to the manager's spec, ⛔ not authorization; ⛔ L_Arena is never an option, CF-R4)

| # | What changes | Where (file + symbol) | Cost | Expected effect | Risk / does NOT touch |
|---|---|---|---|---|---|
| **D1** | `TorchIntensity` 12 → **60–120 cd** | **BP_Torch class default** (asset exists, field currently delta-free; C++ fallback `Torch.h:177` would be a compile) | One editor/MCP session + BP_Torch asset save. **No compile. No map touch.** | Floor nadir 0.20 → 0.99–1.97 lux; ×~2 in overlap zones. The single biggest floor lever. | Wall hotspots scale identically (48 → 240–480 lux at 0.5 m) — pair with D2/D3 and pixel-check; shadowless leak through walls brightens exterior bands near mounts. Does not touch L_Arena, code, or the TASK-556 offset seam. |
| **D2** | `TorchAttenuationRadius` 1200 → **1800–2000** | BP_Torch class default (same surface) | Same session as D1. No compile. | Floor pool 912 → 1546–1842 uu; corridor coverage from its one torch roughly doubles; inter-pool valleys fill; gentler falloff = less in-frame contrast. | Bigger light volumes ×12 (shadowless, MegaLights off — cheap); more through-wall spill. |
| **D3** | LocalExposure contrast scales 0.8 → **0.6–0.7** (Shadow first, Highlight optional) | `Config/DefaultEngine.ini:90-91` | **Config-only, next boot. No compile, no map, no code.** PPV-proof: L_Arena serializes NO LocalExposure override. | Local tonemap lifts shadow regions and compresses highlights IN THE SAME FRAME — attacks "black floor beside blown walls" directly without adding a single lumen. The cheapest dial in the set. | Global (exterior look shifts mildly); overdone reads flat/HDR-photo. |
| **D4** | Exposure Min/Max/Bias re-clamp | The lock lives on **L_Arena's PPV** ⇒ three lanes: **(a)** Jonathan edits the PPV in-editor + saves — CF-R4 escalation, HIS hands only, evidence attached; **(b)** code-side: `PostProcessSettings` override on the hero camera component (new C++, e.g. Min EV100 −1..0, Max ~current fixed EV, method histogram + speed clamps) — compile (G4-gated), zero map touch, the ATorch runtime-spawn precedent; **(c)** runtime-spawned unbound PPV at higher priority — compile, whole-scene. ⛔ The config lane is DEAD for this dial: `r.DefaultFeature.AutoExposure.*` cannot beat the volume's serialized overrides, and no cvar unlocks Min/Max. | (a) free but his; (b)/(c) one compile | Lets the view adapt indoors: floor rises toward mid-tones, sun-lit faces clamp at Max instead of blowing. | Exposure pumping at the threshold walk-through (bound with Min/Max span + adaptation speeds); changes the exterior look Jonathan has already accepted — clamp Max at the current fixed EV to protect it. ⛔ Requires TASK-617's item-1 read FIRST (you cannot clamp around numbers you haven't read). |
| **D5** | The **7th anchor** (+768, −315, 954) yaw 180 + `MaxTorchesPerCastle` 6 → 7 | `Castle.cpp` one `TorchAnchors.Add` + `Castle.h:535` cap default. ⚠️ The `Castle.cpp:287-288` "no recompile" note presumes a Blueprint default surface — **no BP_Castle exists**, so as shipped this is a C++ edit + **compile** (G4-gated: waits for Jonathan's standalone to close). | One compile | Corridor in-pool coverage 58.8% → ~95%+ (facing pair spans the 1650-uu passage) — kills the only genuine coverage hole. | Minimal; the cap law is honoured by its own designed "deliberate second edit." |
| **D6** | Low ambient fill: one dim large-radius shadowless point light per castle at hall centre (e.g. 8 cd @ 3500 uu), spawned like the torches | New C++ (or a torch-anchor abuse — not recommended, it would mount a visible torch mesh mid-air) | One compile | Raises the floor everywhere at once, no hotspots. | Flattens the torch-pool look; D1+D2 approximate it cheaper. ⛔ NOT via the map SkyLight (edit = map save). Rank last. |

**Recommended package for the manager (costed, not prescribed):** Wave 1 = **D3 + D1 + D2** — zero compile, zero L_Arena touch, one BP_Torch session + one config line pair; then the TASK-617-pattern pixel re-check. Wave 2 (only if the floor still reads black or the corridor stays dark) = **D4(b) + D5**, one compile between them, gated on G4 + the 617 item-1 exposure baseline.

## 6. TASK-617 LIVE-ADJUDICATION CHECKLIST (lane D) — everything this audit could not read file-side

1. **The PPV numbers (the single most load-bearing read):** `AutoExposureMinBrightness`, `AutoExposureMaxBrightness`, `AutoExposureBias`, `bUnbound`, effective `AutoExposureMethod` on L_Arena's PostProcessVolume. Min == Max ⇒ fixed exposure confirmed (my expected state); Min < Max ⇒ real AE window, spec's original hypothesis revives. Read-only property inspect — no save.
2. **Sun/sky baselines:** the "LightSource" DirectionalLight's serialized `Intensity` (an override EXISTS — value unknown) + units; SkyLight intensity + which components carry the two serialized `Mobility` overrides.
3. **The spawned torch truth:** per castle — spawned-class name (expect `BP_Torch_C`, confirming the BP default surface is live for D1/D2), count (expect 6), live `Intensity`/`IntensityUnits`/`AttenuationRadius`/colour on each `TorchLight` (expect 12 cd/1200/warm), and `TorchLight` world Z (expect ≈954 + the BP offset override). Deltas from §1 are findings.
4. **Interior HDR pixel reads at floor level:** scene luminance at the black floor and at the blown wall slabs (`show VisualizeHDR` / debug eyedropper in SIE) — quantifies the in-frame ratio my §2 predicts at 240:1–900:1.
5. **Blown-face discrimination:** hue at the blown slabs (warm ≈ torch (1.0, 0.72, 0.42) vs neutral ≈ sun) + do they coincide with the six mount XYs; if SIE permits, a transient DirectionalLight visibility toggle (no save, no dirty-flag risk — if it would dirty the level, skip and use hue only).
6. **Exposure histogram in SIE:** where the metered EV sits vs the Min/Max clamp — the direct confirm/refute of the fixed-vs-adaptive verdict.
7. **Lumen sanity:** confirm Lumen GI active in the SIE viewport and whether 12-cd torch bounce registers at all (a Lumen-off viewport would overstate the darkness).
8. **Cross-lane (with lane B's grid):** one pixel + trace at the interior evidence column — is the visible dark ground the interior floor's top face or arena ground/underside through a hull-free column (my §4 confound note).

## 7. FILES READ (all read-only) / TOUCHED

- Read: `Source/GitClaudeUnrealTest/Siegebound/Torch.h`, `Torch.cpp` (full), `Castle.cpp:40-330` + `:489-624` excerpts, `Castle.h:524-535`, `Config/DefaultEngine.ini` (full), the two evidence PNGs, TASKBOARD spec. Binary name-table scans (grep -a, no open/parse/write): `Content/Maps/L_Arena.umap`, `Content/Blueprints/BP_Torch.uasset`.
- Written: **this handoff only.**
- Not done, per fence: no code/config edit, no compile, no editor/MCP (down — nothing faked; every live number is enumerated in §6 instead), no git, no TASKBOARD edit, no console/`M`/DumpAssistantPrompt, no L_Arena open or save.

**QA should scrutinize:** (i) the §2 integration constants (region rectangles transcribed from `Castle.cpp:104-124` — an error there shifts the corridor coverage figure); (ii) the §4 template-stock inference — it is an EXPECTATION about serialized values, deliberately labelled, never claimed as a read; (iii) the D5 compile-cost correction against the in-code "no recompile" comment (the comment presumes a BP surface that does not exist).
