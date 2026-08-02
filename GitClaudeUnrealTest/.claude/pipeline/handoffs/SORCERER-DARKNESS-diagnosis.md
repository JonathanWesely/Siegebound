# SORCERER "VERY DARK" — DIAGNOSIS (measurement-led, no fix applied)

**Agent:** art-director · **Date:** 2026-08-02 · **Status: DIAGNOSIS ONLY — nothing changed.**

**Discipline honoured:** PIE stopped · `L_Arena` never opened, never saved · `MI_Sorcerer_PBR` never touched
(read-only queries) · no Git command of any kind · no gameplay code · editor left open · no asset saved, no asset
created, no actor placed. Every write went to the scratchpad or to `Saved/ArtDiag/` (gitignored).

---

## 0. HEADLINE — the hypothesis in the brief is REFUTED, and so is the complaint's obvious reading

🎯 **The Sorcerer is not dark. In every unit-asset domain it is one of the BRIGHTEST things in the fleet** —
including, decisively, **the rendered mesh, where it ranks 13/13 (brightest, z = +2.49)**.

🎯 **The one place it IS the fleet's darkest, by a wide margin, is the CARD ART BACKDROP — specifically the half
of the card that `accept.py` never samples.** Its bottom-corner backdrop measures **0.0286 vs a fleet mean of
0.1771 — 6.19× darker, z = −2.29, rank 1/13.**

🎯 **It is the only card in the roster whose backdrop gets DARKER toward the bottom.** Bottom/top luma ratio
**0.46**; all twelve other unit cards run **1.36 – 6.76 (mean 1.98)**. The fleet lights its floor; the Sorcerer's
floor falls away to near-black.

⚠️ **The gate could not have caught it.** Every backdrop criterion in `accept.py` is *relative* (ΔE2000 separation
≥ 10, halo lift ratio > 1.3×) and its key sampler `key_of()` reads **only the two TOP 64×64 corners**. There is no
absolute luminance floor anywhere in the card gate.

💰 **Recommended fix: re-render the card only. ZERO Meshy credits, ZERO HF spend.** Do not re-gen, do not re-bake
the texture set, do not touch the ORM or the material.

---

## 1. Ranked diagnosis with numbers

| # | Factor | Measurement | Fleet position | Verdict |
|---|---|---|---|---|
| **1** | **Card backdrop, LOWER half** | bottom-corner key luma **0.0286** (fleet mean 0.1771) | **rank 1/13, z = −2.29, 6.19× darker** | 🔴 **DOMINANT CAUSE** |
| **2** | **Card backdrop, whole field** | non-figure luma **0.0597** (fleet mean 0.1563) | **rank 1/13, z = −1.99, 2.6× darker** | 🔴 same defect, wider lens |
| 3 | Card backdrop gradient direction | bottom/top **0.46** (fleet 1.36–6.76, mean 1.98) | **only card in the roster that inverts** | 🔴 the structural tell |
| 4 | Simultaneous-contrast amplifier | card FIGURE luma **0.2752** (fleet mean 0.1909) | rank 12/13 — 2nd brightest | 🟠 amplifies (see §4) |
| 5 | Card backdrop, TOP key (what the gate reads) | **0.0628** (fleet mean 0.1052) | rank 4/13, z = −0.96 | 🟡 mildly dark — **gate saw only this** |
| 6 | ORM ambient occlusion | AO mean **0.4854** | rank 2/13 darkest | ⚪ **RULED OUT** — §3 |
| 7 | `_D` base colour texture | UV-norm **0.4403** | rank 12/13 — 2nd brightest, 74% over floor | ⚪ **RULED OUT** |
| 8 | `MI_Sorcerer_PBR` wiring | 3/3 params, correct parent | **identical to all 20 fleet MIs** | ⚪ **RULED OUT** |
| 9 | Concept art | mean linear Y **0.1599** | rank 9/13, **above** fleet mean, z = +0.39 | ⚪ **RULED OUT** |
| 10 | `TeamRegion` slot 0 | shared `MI_TeamColor_Blue`, 3.12% of area | fleet-wide shared asset | ⚪ **RULED OUT** |
| 11 | Stale/failed texture import | D ratio 1.05, ORM 1.39 | **both uniform across the fleet** | ⚪ **RULED OUT** |
| 12 | Roughness / metallic | rough 0.6150 (rank 9), metal **0.0041** (rank 1) | 4 units rougher; lowest metal in fleet | ⚪ **RULED OUT** |

---

## 2. The rendered evidence — this is the number that corresponds to the complaint

`CaptureAssetImage` renders every asset through the **identical UE thumbnail light rig** (256×256, verified
byte-identical backdrop across all 13 captures: corners `(24,24,24)`/`(15,15,14)`). Foreground = pixels above the
measured backdrop ceiling (border max 25 → threshold 31).

### Static meshes `/Game/Meshes/SM_*`

| asset | fg mean luma | fg L* |
|---|---|---|
| Wizard | 0.1838 | 42.28 |
| Knight | 0.1882 | 41.77 |
| Archer | 0.1964 | 44.78 |
| … | … | … |
| Cleric | 0.3776 | 60.16 |
| **Sorcerer** | **0.4166** | **65.16** |

**Sorcerer rank 13/13 — the BRIGHTEST unit in the fleet. z = +2.49 vs the other twelve** (fleet mean 0.2729).

### Skeletal meshes `/Game/Characters/SK_*` — the asset that actually spawns in play

| asset | fg mean luma |
|---|---|
| Ogre | 0.1500 |
| Archer | 0.1687 |
| … | … |
| Longbowman | 0.3402 |
| **Sorcerer** | **0.3717** |
| Cleric | 0.4581 |

**Sorcerer rank 12/13 — second brightest, z = +1.23.**

> There is no rendering configuration in which this asset is dark. Both the static and the skeletal path put it at
> the top of the fleet under identical lighting.

---

## 3. Why the ORM hypothesis fails despite the AO being low

The brief called ORM "the most likely divergence between metrics pass and looks dark". It is not, and the fleet
data disproves it directly:

| unit | AO mean | rendered fg luma | rank |
|---|---|---|---|
| **Cleric** | **0.4692** (darkest AO in fleet) | **0.3776** | 12/13 — 2nd brightest |
| **Sorcerer** | **0.4854** (2nd darkest AO) | **0.4166** | 13/13 — brightest |
| Pikeman | 0.6710 (brightest AO) | 0.2798 | 7/13 |

**The two units with the darkest AO maps render as the two brightest units.** AO is anti-correlated with the
rendered result here, because `apply_albedo_delight` *divides the AO out of `_D` before the PNG is written*
(`linear + (linear/ao − linear) * strength`, floor 0.25) and the engine then multiplies it back into the indirect
term only. The round trip is self-cancelling and fleet-uniform. Metallic is **0.0041 — the lowest in the fleet**,
so there is no stray-metal darkening either.

The TASK-342 γ0.55 chroma-collapse mechanism the brief asked me to check for is **also absent**: chroma retention
0.9292 and rendered fg chroma 10.53 vs fleet mean 11.71 (z = −0.28) — ordinary, not collapsed. The Sorcerer runs
the locked fleet profile `1.0 / 0.25 / 0.55 / 1.2`; Archer and Pikeman were moved off it to γ1.0 + high gain, but
the Sorcerer shows none of the symptoms that motivated that move.

---

## 4. The actual defect, in full

`Content/RawAssets/CardArt/Sorcerer.png` → `/Game/UI/CardArt/T_CardArt_Sorcerer`.

| region | Sorcerer | fleet mean (12 other unit cards) | ratio | z | rank |
|---|---|---|---|---|---|
| **bottom-corner key** | **0.0286** | 0.1771 | **6.19× darker** | **−2.29** | **1/13** |
| lower band (below 62% height) | 0.0928 | 0.1770 | 1.91× darker | −1.48 | 2/13 |
| whole non-figure backdrop | 0.0597 | 0.1563 | 2.62× darker | −1.99 | 1/13 |
| top-corner key (**the gate's sample**) | 0.0628 | 0.1052 | 1.67× darker | −0.96 | 4/13 |
| **figure** | **0.2752** | 0.1909 | **1.44× BRIGHTER** | **+1.56** | **12/13** |
| whole card | 0.1419 | 0.1699 | 1.20× darker | −0.55 | 4/13 |

### 4a. The gradient inversion — the single cleanest tell

| card | bottom/top backdrop ratio |
|---|---|
| **Sorcerer** | **0.46** ← darkens downward |
| Footman | 1.36 |
| Longbowman | 1.39 |
| MilitiaMob / Miner | 1.45 |
| Cleric | 1.46 |
| Pikeman | 1.48 |
| Archer | 1.54 |
| Cavalry | 1.57 |
| Knight | 1.59 |
| Ogre | 1.76 |
| Sapper | 1.97 |
| Wizard | 6.76 |

**Twelve of thirteen brighten downward. The Sorcerer is the sole inversion.** Visually confirmed: Longbowman sits
on a bright olive field, Wizard on a lit purple field, Cleric on a lit teal field; the Sorcerer's figure stands in
a spotlight halo above a floor that falls to near-black.

### 4b. Why it reads *worse* than the raw number

The figure is the **2nd brightest of the roster (0.2752)** sitting on the **darkest backdrop of the roster
(0.0597)** — a figure/backdrop contrast ratio of **4.61×** against a fleet mean of **1.24×**. Simultaneous contrast
makes the dark field read darker than its measured value, which is why "very dark" is a fair description of a card
whose *whole-frame* luma is only 1.20× below fleet mean. The eye is reporting the **contrast**, not the mean.

### 4c. Root cause of the defect

TASK-375 §6a records that TASK-373's original render script was destroyed and the card was **rebuilt from recorded
parameters, in Cycles rather than EEVEE, with "self-solving framing and backdrop/light solves"**. Those solves were
driven by `accept.py`'s criteria. Since the only backdrop criteria are the **top-corner** ΔE≥10 uniqueness test and
the halo *ratio*, the solve had every incentive to darken the floor (which raises the halo ratio and is invisible
to the key sample) and none to keep it lit. The card passed all four gates while inverting the fleet's lighting
convention.

### 4d. ⚠️ Systemic — this will recur and get worse

`accept.py`'s uniqueness gate demands **ΔE2000 ≥ 10 from all 29 shipped keys**. As the roster fills, the
unoccupied volume in colour space is increasingly in the **dark** corner — the shipped key distribution already
runs `val` 0.225 → 0.655 with a mean of 0.394, and the newest cards cluster low. **The gate structurally pushes
each new card darker than the last.** Left unamended, the next unit inherits this.

---

## 5. Priced fixes, with expected before/after

| option | cost | effect | recommend |
|---|---|---|---|
| **A. Card re-render, floor/backdrop lift only** | **0 Meshy credits · 0 HF · ~1 headless Blender run** | fixes the dominant cause at source | ✅ **THIS ONE** |
| B. Card re-render + top-key lift to val ≈ 0.48 | same | also lifts the mildly-dark top key | ✅ optional, fold into A |
| C. `_D` texture-set-only re-bake (TASK-239/342 precedent) | 0 credits | **would make the 2nd-brightest texture brighter still** and risks the anti-bleach guard | ❌ **no defect to fix** |
| D. ORM correction | 0 credits | AO is anti-correlated with the rendered result (§3) | ❌ |
| E. Material-parameter change | 0 credits | **`M_AssetPBR` exposes only `BaseColor`/`Normal`/`ORM` — there is no brightness scalar to change** | ❌ impossible |
| F. Meshy `image3d` re-gen | **30 credits** | the mesh is already the brightest in the fleet | ❌ **do not spend** |

### 5a. Expected numbers for option A/B

Hue **168.0°** and saturation **0.633** held (identity preserved — the jade/teal read Jonathan approved); only
value raised, plus the floor relit to the fleet's downward-brightening gradient.

| target `val` | key rgb8 | key luma | ΔE2000 to nearest shipped | nearest |
|---|---|---|---|---|
| 0.310 (**shipped**) | (29, 79, 69) | 0.0628 | 8.67 | Pickpocket |
| 0.45 | (40, 115, 100) | 0.1357 | 8.38 | Archer |
| **0.48 (recommended)** | **≈ (43, 124, 107)** | **≈ 0.156** | **≈ 10** | Archer |
| 0.50 | (44, 128, 111) | 0.1700 | **10.73** | Archer |
| 0.55 | (49, 140, 122) | 0.2088 | 12.63 | Cleric |

**`val` 0.48–0.50 lands the key on the unit-card backdrop mean (0.1563) while ΔE2000 *improves* from 8.67 to
≈10.7 — the uniqueness gate gets stronger, not weaker.**

Predicted post-fix state:

| metric | now | after | fleet mean |
|---|---|---|---|
| bottom-corner key luma | 0.0286 | **≈ 0.15 – 0.18** | 0.1771 |
| bottom/top ratio | 0.46 | **≈ 1.4 – 1.7** | 1.98 |
| whole-backdrop luma | 0.0597 | **≈ 0.14 – 0.16** | 0.1563 |
| whole-card luma | 0.1419 | **≈ 0.19 – 0.22** | 0.1699 |
| figure luma | 0.2752 | **unchanged 0.2752** | 0.1909 |
| ΔE2000 uniqueness | 8.67 | **≈ 10.7** | gate ≥ 10 |

**No bleaching is involved** — the figure is untouched and the concept gate is not in play. This lifts a backdrop
to the fleet's own convention.

### 5b. Gate hardening (manager/tooling call — NOT taken by me)

Three amendments would close the blind spot permanently:
1. **Sample the key from all FOUR corners, not the top two** — the defect lives entirely in the two the gate skips.
2. **Add an ABSOLUTE backdrop luminance floor** (suggest ≥ 0.08 linear, ~fleet min) alongside the relative ΔE test.
3. **Add a gradient-direction check** — assert bottom/top ≥ 1.0. This one criterion alone would have failed the
   shipped card at 0.46 while passing all thirteen others.

---

## 6. Method / honesty notes

- **Harness re-validated before any number was trusted:** my albedo path reproduces the recorded anchors exactly —
  Sorcerer UV-norm **0.4403** (matches the brief's table to 4 dp) and Footman **0.2536** (the floor is literally the
  Footman's own value). ORM read as **linear** (no sRGB decode), per `measure_fidelity.py::orm_stats`.
- **`Cache/_task342/measure_fidelity.py` was left byte-untouched.** Its functions were *copied* into scratchpad
  scripts, never edited in place, so its fleet-anchor validation stays meaningful.
- **Concept masking used the TASK-370 correction** (`derived_alpha` at `neighbor_tol = 0.010`, not the shipped
  0.03) and was cross-checked against the independent corner mask. Sorcerer is the **most mask-stable unit in the
  fleet** — derived fg 0.3377 vs corner 0.3383, a 0.06% difference — so its concept number does not depend on the
  mask choice. Ogre/Footman/Archer concept rows are mask-unreliable and are flagged rather than leaned on.
- **The ORM "stale import" flag is a false positive and I checked before reporting it:** engine-thumbnail/disk
  ratios are Sorcerer 1.39, Footman 1.39, Cleric 1.37, Longbowman 1.35 — uniform, i.e. the thumbnail renderer
  display-encoding a linear texture, not a bad import. D ratios are 1.05 across all four.
- ⚠️ **One thing I could NOT test:** the unit under `L_Arena`'s actual runtime lighting. Measuring it requires
  placing actors, which dirties the level, and the brief said change nothing. I ruled out every *asset-side* cause
  instead, including two independent rendered captures under a controlled rig. **If Jonathan's complaint turns out
  to be about the unit in the arena rather than the card in hand, the next step is a lighting-side look, not an
  asset-side one** — because the asset measures brightest in the fleet on every axis.
- The two key samplers disagree on rank (4/13 top-corner vs 1/13 four-corner) and I have reported **both** rather
  than picking the flattering one. The disagreement is itself the finding: it localises the defect to the bottom.

---

## 7. Files

**Written (none in `/Game`, none tracked):**
- `.claude/pipeline/handoffs/SORCERER-DARKNESS-diagnosis.md` — this note
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\ArtDiag\*.txt` — 31 base64 captures
  (gitignored `Saved/`); safe to delete
- scratchpad: `fleet_audit.py` · `render_and_card.py` · `card_decompose.py` · `decode_sk.py` · `price_fix.py` ·
  `gate_metric.py` · `top_vs_bottom.py` · `measure_concept_darkness.py` · `thumbs/*.png`

**Read only, never modified:** `Content/RawAssets/Textures/*/T_*_{D,N,ORM}.png` · `Content/RawAssets/Concepts/*.png` ·
`Content/RawAssets/CardArt/*.png` · `Tools/ArtPipeline/refine_trellis_glb.py` · `Cache/_task342/measure_fidelity.py` ·
all `/Game` assets (MCP read-only queries only).

**The fix, when authorised, touches exactly two files:** `Content/RawAssets/CardArt/Sorcerer.png` and
`Content/UI/CardArt/T_CardArt_Sorcerer.uasset`. Nothing in `/Game/Meshes`, `/Game/Textures`, `/Game/Materials`,
`/Game/Characters` needs to change.

---
---

# PART 2 — ARENA-SIDE INVESTIGATION (round 2)

**Agent:** art-director · **Date:** 2026-08-02 · **Status: INCOMPLETE — STOOD DOWN MID-INVESTIGATION**
(orchestrator recall: Jonathan needs exclusive editor use for the TASK-377 PIE ship gate)

**Discipline honoured:** `L_Arena` loaded and **NEVER saved** (dirty in memory only — must not reach disk) ·
all 4 temp actors removed, **sweep empty, actor count back to the 118 baseline exactly** · PIE/Simulate stopped ·
editor left OPEN · `MI_Sorcerer_PBR` never touched · no Git command · no gameplay code · no asset created or saved.

> ⚠️ **READ THIS FIRST: I did NOT obtain the per-unit arena luminance numbers this round was commissioned to get.**
> A render artifact blocked the measurement (§P2-4). Everything below is either directly read engine state or
> derived geometry. **No luminance number in this Part is mine — I am not reporting any, because the only pixels I
> could capture were the artifact's, not the shading's.** The candidate ranking below is therefore *provisional*
> and rests on ruling-out-by-mechanism, not on photometry.

---

## P2-0. HEADLINE — what round 2 actually settled

🎯 **Candidate #2 (fog) is refuted by camera geometry, not by pixels.** `BP_HeroCharacter`'s `CameraBoom.TargetArmLength`
is **400** with FOV **90** — a close third-person camera, **not an RTS camera**. `ExponentialHeightFog_0.StartDistance`
is **10,000**. At gameplay viewing distance (~400–3,000 units) **the fog contributes exactly zero to a unit.**

🎯 **Candidate #1 (auto-exposure / eye adaptation) is refuted by settings.** `PP_Arena_Global` pins
`AutoExposureMinBrightness = AutoExposureMaxBrightness = 1.0` — **adaptation is locked off.**

🎯 **Both #1 and #2 are also structurally incapable of the complaint.** Fog is a function of *pixel depth* and exposure
is a function of *whole-scene* average. Two units standing side by side at equal distance receive **identical** fog and
**identical** exposure. Neither can darken one unit *relative to its neighbours* — and "the Sorcerer looks dark" is a
relative claim. This does not make them worthless to test, but it does demote them.

🎯 **The brief's fog numbers were stale.** Actual: density **0.012** (not 0.008), max opacity **0.92** (not 0.85).

🎯 **NEW, unrelated to the Sorcerer, and possibly more serious: `M_AssetPBR.bUsedWithSkeletalMesh = false` is
confirmed still false, and the output log contains live proof of consequence** (§P2-3). This is TASK-325's flagged
issue with receipts.

---

## P2-1. `L_Arena` lighting rig — exact values, read from the live level

### DirectionalLight_0 (`LightComponent0`)

| property | value |
|---|---|
| Intensity | **11 lux** |
| Light colour / temperature | white (1,1,1) · `bUseTemperature` true · **5400 K** |
| Rotation | **pitch −38° · yaw 145°** |
| Mobility | Movable · CastShadows true · CastDynamicShadows true |
| DynamicShadowDistanceMovableLight | 20,000 |
| LightSourceAngle | 1.5 · ShadowBias 0.5 · ContactShadowLength **0** |
| IndirectLightingIntensity | 1.0 · VolumetricScatteringIntensity 1.0 |
| LightShaftOcclusion | enabled · OcclusionMaskDarkness 0.05 |

### SkyLight_0

| property | value |
|---|---|
| Intensity | **1.0** · `SLS_CapturedScene` · `bRealTimeCapture` **true** |
| Light colour | (0.800, 0.871, 1.000) — cool/blue |
| Occlusion | MinOcclusion **0** · OcclusionMaxDistance 1000 · OcclusionTint **black** · Exponent 1 |
| Other | CastShadows true · Indirect 1.0 · Contrast 0 · `bLowerHemisphereIsBlack` false |

### ExponentialHeightFog_0 (`HeightFogComponent0`)

| property | value | brief said |
|---|---|---|
| FogDensity | **0.012** | 0.008 ❌ |
| FogMaxOpacity | **0.92** | 0.85 ❌ |
| StartDistance | **10,000** | 10,000 ✅ |
| FogHeightFalloff | 0.20 | — |
| **FogInscatteringLuminance** | **(0, 0, 0) — BLACK** | — |
| DirectionalInscatteringLuminance | (0, 0, 0) · exponent 4 · start 10,000 | — |
| Volumetric fog | **DISABLED** | — |
| Actor Z | **−6,850** (fog origin far below the ground plane at z=0) | — |

### PostProcessVolume_0 — label **`PP_Arena_Global`**

`bEnabled` true · **`bUnbound` true** (applies everywhere) · Priority 1 · BlendWeight 1 · BlendRadius 100.

**Exactly 10 overrides are enabled. This is the complete list:**

| override | value | note |
|---|---|---|
| ColorSaturation | **0.90** | global −10% saturation |
| ColorContrast | **1.05** | +5% contrast — pivots about mid-grey, so it **darkens everything below the pivot** |
| BloomIntensity | 0.60 | |
| BloomThreshold | 0.85 | |
| **AutoExposureMinBrightness** | **1.0** | |
| **AutoExposureMaxBrightness** | **1.0** | **min == max ⇒ eye adaptation is LOCKED OFF** |
| AutoExposureBias | +0.40 | fixed EV compensation |
| VignetteIntensity | **0.40** | **darkens frame edges — a position-dependent confound for any capture** |
| ReflectionMethod | Lumen | |
| DynamicGlobalIlluminationMethod | Lumen | |

Method is `AEM_Histogram`, `AutoExposureApplyPhysicalCameraExposure` true — but with min == max the histogram result
is clamped to a constant, so exposure is effectively fixed.

---

## P2-2. Camera geometry — the finding that kills the fog hypothesis

Read from `BP_HeroCharacter`'s CDO subobjects:

| property | value |
|---|---|
| `CameraBoom.TargetArmLength` | **400** |
| `CameraBoom.SocketOffset` / `TargetOffset` | (0,0,0) / (0,0,0) |
| `CameraBoom.bUsePawnControlRotation` | true · `bDoCollisionTest` true |
| `FollowCamera.FieldOfView` | **90** |

**This is a close over-the-shoulder third-person camera.** The brief's premise of "RTS camera distance in a ±28,000-unit
arena" does not hold for how the player actually views a unit. A unit engaged near the hero sits roughly **400–3,000
units** from the camera; fog does not begin until **10,000**.

**⇒ At the distance Jonathan actually looks at a unit, `ExponentialHeightFog_0` contributes nothing at all.**

Fog would only apply to units more than 100 m away — i.e. distant specks, not the unit he is complaining about.
(Corroborating visual: in every capture I took, `Castle_Red` at ~26,000 units read as a clearly legible, only mildly
hazed silhouette — the fog is far weaker in practice than density 0.012 / opacity 0.92 suggests, because the
0.2 height-falloff with the fog origin 6,850 units *below* the ground plane puts the playfield in very thin fog.)

### P2-2a. Sun-vs-player geometry — derived, and worth a look next round

Light forward vector from pitch −38° / yaw 145° = **(−0.646, +0.452, −0.616)**.
Blue's `PlayerStart` is at **x = −23,800**; Blue's castle **x = −25,000**, Red's **x = +25,000** — so the player
advances toward **+X** and therefore looks toward **+X**. The sun sits toward **+X / −Y / up**.

**⇒ The player is looking roughly into the sun, and the face of a unit turned toward the player is its *shadowed*
side.** This is true of the whole fleet, not just the Sorcerer — so on its own it explains "units read dark",
not "the Sorcerer reads dark". It becomes a *differential* explanation only if the Sorcerer's silhouette
(slab mantle, hood, antler crown) self-shadows its own camera-facing surfaces more than the fleet's simpler
shapes do. **That is exactly the test I did not get to run.**

---

## P2-3. `bUsedWithSkeletalMesh` — CONFIRMED FALSE, with log receipts

`/Game/Materials/M_AssetPBR`:

| property | value |
|---|---|
| **`bUsedWithSkeletalMesh`** | **false** ⚠️ |
| BlendMode | `BLEND_Opaque` |
| ShadingModel | `MSM_DefaultLit` |
| TwoSided | false · MaterialDomain `MD_Surface` |
| bUsedWithStaticLighting / InstancedStaticMeshes / Nanite | false / false / false |

The live output log carries the consequence:

```
LogSkeletalMesh: Warning: Material with missing usage flag was applied to skeletal mesh
                 /Game/Characters/SK_Sorcerer.SK_Sorcerer
```

— repeated, and the identical warning for **SK_Archer, SK_Pikeman, SK_Ogre, SK_Cavalry**
(timestamps 17:50, i.e. emitted by round 1's own `CaptureAssetImage` calls on the SK meshes).

**Two consequences, and I want to be precise about which is established and which is not:**

1. ⚠️ **It puts round 1's SK_* table in doubt.** Those warnings fire at exactly the moment round 1 captured SK
   thumbnails. In-editor UE auto-sets the usage flag and recompiles on first use, so some of those captures may
   have rendered a *fallback* material rather than `MI_<Unit>_PBR`. **I did not verify which.** The round 1 SM_*
   table is unaffected — static meshes do not need this flag.
2. ⚠️ **In a cooked/packaged build the editor's auto-fix does not run**, so the SK path — which is what actually
   spawns in play (`ASummonedUnit::ResolveSkeletalVisual` hides the static `VisualMesh` and shows `SkeletalVisualMesh`)
   — would fall back to the default material. **I did not confirm the packaged behaviour.**

This is a real, reproducible, zero-credit-to-fix defect independent of the Sorcerer question. It is TASK-325.

---

## P2-4. ⛔ WHY THERE ARE NO LUMINANCE NUMBERS — the blocking artifact

I placed `BP_Unit_{Footman,Wizard,Cleric,Sorcerer}` in a row at x = 0 (arena centre, flat ground, z-top = 0),
facing the camera, and captured them in one frame at two framings (camera 1,500 units back, then 900 units back —
the latter chosen to match the real 400-unit boom plus melee spacing).

**In every editor capture the four units rendered as a translucent, vertically-streaked, flat sage-green stipple
with no texture detail.** The rest of the level rendered correctly in the same frame, and — decisively — **the units
cast correct, solid, opaque shadows onto the ground.** So this is a per-primitive stipple/dither on the newly placed
actors, not a shading result.

**Measuring luma off those pixels would have been measuring the artifact.** I did not do it, and I am not reporting
any number derived from them.

### What I ruled out as the artifact's cause

| hypothesis | test | result |
|---|---|---|
| Editor selection highlight | `SelectActors([])`, re-captured | ❌ unchanged |
| `CaptureViewport` being an offline path with no temporal history | `CaptureEditorImage` — the **live** viewport as Jonathan sees it | ❌ **same ghosting**, so not capture-path-specific |
| LOD distance / low-LOD | moved camera to 900 units (LOD0 range) | ❌ unchanged |
| Translucent material | `M_AssetPBR` + both MIs read | ❌ `BLEND_Opaque`, no base-property overrides |
| Wrong material assigned | slots read per mesh | ❌ correct: `[TeamRegion, <Unit>PBR]` → `MI_TeamColor_Blue` + `MI_<Unit>_PBR` |
| Asset corruption | `CaptureAssetImage /Game/Meshes/SM_Sorcerer` re-rendered this session | ❌ **clean and bright** — pale mint/jade robe, white-blue pauldrons, brown sash |

### Still untested as the cause (for round 3)

- **Velocity/temporal smear** — I `set_actor_transform`'d the actors immediately before each capture; a large
  per-actor velocity delta can smear TSR/TAA into exactly this streaked semi-transparency. **This is my leading
  suspicion** and the cheapest to test: place, then let the viewport settle, then capture without touching them.
- Texture streaming not resident for freshly placed actors (would explain flat, detail-free colour).
- `DitheredLODTransition` on the master material (I read the MI override struct, not `M_AssetPBR`'s own value).
  Note `SM_Wizard` has **1 LOD** while Footman/Cleric/Sorcerer have **4** — the Wizard ghosted too, which argues
  against LOD dithering, but not conclusively.

### The Simulate attempt

Simulate (SIE) **did** render solidly — no ghosting — confirming the renderer and assets are fine. But it was not
usable for a controlled measurement: `BP_BattlefieldScatter` spawns dense foliage at BeginPlay and **the four units
immediately marched out of frame**. I stopped it rather than chase moving targets.
For round 3: `CaptureViewport`'s `annotations.labeledActors` returns per-actor name + `screenPosition` +
`distanceCm`, which is the clean way to locate moving units and sample a patch at each — capture annotated to
locate, capture unannotated for pixels.

---

## P2-5. Provisional ranking — mechanism-based, NOT photometric

| # | candidate | status after round 2 | basis |
|---|---|---|---|
| — | Auto-exposure / eye adaptation | 🟢 **RULED OUT** | Min == Max == 1.0 locks adaptation; and it is scene-global, so it cannot darken one unit *relative to its neighbours* |
| — | ExponentialHeightFog | 🟢 **RULED OUT at gameplay distance** | 400-unit boom vs 10,000-unit fog start; and fog is depth-dependent, identical for units at equal distance |
| 1 | **Self-shadowing / silhouette under the backlit sun** | 🟡 **UNMEASURED — now the top candidate** | Player looks toward +X into a sun at +X/−Y (§P2-2a); Sorcerer's mantle/hood/antlers are the fleet's most self-occluding silhouette. Only this class of cause can act on one unit and not its neighbour |
| 2 | **Roughness under a single directional key** | 🟡 **UNMEASURED** | Still the best reconciliation of "brightest under the thumbnail's soft rig" vs "dark under one hard key". Round 1 has Sorcerer roughness 0.6150 (rank 9/13) — *not* an outlier, which weakens it, but it was never tested under a directional key |
| 3 | **Value/contrast composition** (unit vs the arena's grass) | 🟡 **UNMEASURED** | The arena floor is bright yellow-green; a cool jade/mint figure against it is a hue-and-value adjacency problem, not a luminance one. Fixes here are rim light / brighter trim, not a re-bake |
| 4 | SK material-usage fallback | 🟠 **CONFIRMED FLAG, EFFECT UNMEASURED** | §P2-3 — real defect, but affects 5 named units incl. Sorcerer; needs an in-play check to see if it changes the render |
| — | `ColorContrast 1.05` + `VignetteIntensity 0.40` | 🟡 noted | Global, but both darken — contrast crushes below-pivot values, vignette darkens frame edges. Global, so again not differential; relevant only to "the arena reads dark overall" |

**No fix is priced yet.** Pricing requires the photometry I did not get. I am not going to invent before/after
numbers for a fix whose cause is still unproven — and I want to flag that the strongest remaining candidates
(#1 and #3) are both **zero-credit** by nature: #1 is a light-angle or per-unit-rotation matter, #3 is a rim-light /
trim-value matter. **Nothing measured so far argues for a re-gen or even a texture re-bake.**

---

## P2-6. Exactly what round 3 should do

1. Place the 4 units, **then leave them untouched** for a settle interval before capturing (tests the velocity-smear
   hypothesis first, since it gates everything else).
2. If still ghosted, measure via Simulate using `annotations.labeledActors` to locate units.
3. Capture Sorcerer + controls **in one frame**, then **swap the Sorcerer between the outermost and innermost slot**
   and re-capture — this cancels the `VignetteIntensity 0.4` position bias, which is large enough to fake a result.
4. Rotate the Sorcerer through 4 yaws at a fixed position to quantify **self-shadowing sensitivity** — the single
   most diagnostic test available, and the one that separates candidate #1 from the rest.
5. Second capture at >10,000 units to quantify the fog term for completeness.
6. Compare against grass background luma to score candidate #3 as a contrast problem.

## P2-7. Files

**Written:** this note only.
**Scratchpad (safe to delete):** `…/scratchpad/decode.py` · `…/scratchpad/shots/*.png`
**Read-only:** `L_Arena` actors/components · `M_AssetPBR` · `MI_Sorcerer_PBR` · `MI_TeamColor_Blue` ·
`SM_{Footman,Wizard,Cleric,Sorcerer}` · `BP_HeroCharacter` CDO · `Source/…/SummonedUnit.cpp` ·
`Source/…/GitClaudeUnrealTestCharacter.cpp` · Unreal output log.
**Nothing saved. `L_Arena` is dirty in memory and MUST NOT be saved.**

---

# PART 2 (RESUMED) — THE ARENA RENDER IS SUBSTITUTING THE MATERIAL

**Agent:** art-director · **Date:** 2026-08-02 (after TASK-377 ship gate) · **Status: VERDICT REACHED, with one named gap.**

**Discipline honoured:** `L_Arena` **NEVER saved** · all 5 temp actors removed, **sweep empty, count back to 118 = baseline** ·
PIE/Simulate stopped · editor left OPEN · no asset saved or created (build-master was running Git in parallel — I saved nothing) ·
`MI_Sorcerer_PBR` untouched · no Git · no gameplay code.

---

## P2-8. 🎯 VERDICT — the premise of the whole investigation is wrong

**The Sorcerer is not dark because of its albedo, roughness, ORM, fog, or exposure. In `L_Arena` the unit meshes
do not render their own material at all — they render a SUBSTITUTED neutral material.**

The measurement that proves it. Four units placed in one frame, identical lighting, segmented by frame-difference
against a background plate, editor-mode `L_Arena`:

| unit | true colour identity | **arena render hue** | sat | val |
|---|---|---|---|---|
| Footman | brown leather / steel | **82.6°** | 0.266 | 0.513 |
| Wizard | purple robe | **83.0°** | 0.254 | 0.557 |
| Cleric | teal / white | **82.2°** | 0.251 | 0.563 |
| Sorcerer | mint / jade | **83.2°** | 0.263 | 0.510 |
| — | — | — | — | — |
| **Sorcerer, thumbnail rig (ground truth)** | mint / jade | **135.2°** | **0.088** | 0.647 |

**Four units with four completely different colour schemes render within 1.0° of hue of each other.** That is not
lighting. That is one material being drawn four times. Hue 82–83° at sat ≈0.26 is a neutral surface picking up
yellow-green Lumen bounce from the arena grass (grass measured hue ≈83°, sat 0.474 in the same frame).

The Sorcerer's real albedo is **hue 135.2°, saturation 0.088** — a desaturated jade. The arena render is hue 83°,
saturation 0.263. Wrong hue by ~52°, wrong saturation by 3×.

### P2-8a. It is not the Blueprint
I placed a **raw `StaticMeshActor` of `/Game/Meshes/SM_Sorcerer`** (no `BP_Unit_`, no construction script, no
`SiegeMeshJuiceComponent`) beside the Blueprint units. **It rendered identically flat.** So the substitution is not
caused by `ASummonedUnit`, the juice component, or the team-colour override — it happens to any actor using these
meshes in this level.

### P2-8b. It is not the asset, the material wiring, or the textures
All verified read-only this session, all correct:
- `SM_{Footman,Wizard,Cleric,Sorcerer}` slots are exactly `[TeamRegion, <Unit>PBR]` → `MI_TeamColor_Blue` + `MI_<Unit>_PBR`. Nanite off, ~15,000 tris.
- `MI_Sorcerer_PBR` binds all three params correctly: `BaseColor`→`T_Sorcerer_D`, `Normal`→`T_Sorcerer_N`, `ORM`→`T_Sorcerer_ORM`. Parent `M_AssetPBR`. No base-property overrides. `BLEND_Opaque`, `MSM_DefaultLit`.
- Textures: `virtualTextureStreaming` **false**, `neverStream` false, `lodGroup` `TEXTUREGROUP_World`, `lodBias` 0, sRGB true on `_D`, false on `_ORM` — all per CONVENTIONS.
- `CaptureAssetImage /Game/Meshes/SM_Sorcerer` **re-rendered perfectly this session** — pale mint robe, white-blue pauldrons, brown sash, all texture detail present.

**The asset is fine. Round 1's conclusion that the Sorcerer is one of the brightest units in the fleet stands, and
is not contradicted by anything I measured.**

### P2-8c. The confirmed mechanism — `bUsedWithSkeletalMesh`

| master material | `bUsedWithSkeletalMesh` | `bAutomaticallySetUsageInEditor` |
|---|---|---|
| `M_AssetPBR` | **false** ⚠️ | true |
| `M_TeamColor` | **false** ⚠️ | true |

**Both** master materials used by every unit lack the skeletal-mesh usage flag. When a material lacking the flag is
applied to a skeletal mesh, Unreal **substitutes the default material**. `bAutomaticallySetUsageInEditor = true`
means the editor silently patches the flag in memory on first use and recompiles — so it *appears* to self-heal in
the editor, **but a cooked build has no auto-set and renders the default material.**

And this is not theoretical — it fires in **Jonathan's real PIE ship-gate run** (log timestamps 18:28, during TASK-377):
```
LogSkeletalMesh: Warning: Material with missing usage flag was applied to skeletal mesh /Game/Characters/SK_Archer
                                                            ... SK_Sapper / SK_Knight / SK_Footman / SK_Pikeman
```
and earlier for `SK_Sorcerer`, `SK_Ogre`, `SK_Cavalry`. This matters because **the skeletal path is the one that
actually spawns in play** — `ASummonedUnit::ResolveSkeletalVisual` hides the static `VisualMesh` and shows
`SkeletalVisualMesh` with `SK_<CardID>`.

---

## P2-9. ⚠️ THE GAP I AM NOT PAPERING OVER

**`bUsedWithSkeletalMesh` does not explain the static-mesh case.** Static meshes do not need that flag, yet my raw
`StaticMeshActor` of `SM_Sorcerer` was substituted too. So either there are **two** causes, or there is one deeper
cause I have not identified. I could not close this before running out of room.

What this means for confidence:
- **High confidence:** the arena render substitutes the material (four-hue-collapse is unambiguous), and the asset itself is innocent.
- **High confidence:** `bUsedWithSkeletalMesh=false` on both masters is real, is a shipping defect, and fires in the live gate.
- **NOT established:** that the flag is *the* cause of what Jonathan sees. The static-mesh substitution is unexplained and could indicate a shared root cause (e.g. an uncompiled shader permutation for this level's feature set — both masters were substituted simultaneously, and every non-unit asset in the same frame rendered correctly).

**I also could not measure the original candidate list.** Because the units never rendered their own material in the
arena, the following remain **UNMEASURED** and must not be treated as ruled out on photometric grounds:
roughness under a directional key (candidate 1), self-shadowing (4), and value-contrast against grass (6). Any luma
number from an arena capture this session — including an apparent "Sorcerer 0.2061 vs Footman 0.2131" — reflects
geometry and silhouette only, **not the units' materials**, and I am explicitly withdrawing it as evidence.

Ruled out on mechanism in the earlier half of PART 2 and unaffected by this: **fog** (400-unit camera boom vs
10,000-unit fog start) and **auto-exposure** (`Min == Max == 1.0`).

---

## P2-10. Ranked verdict

| rank | candidate | status | evidence |
|---|---|---|---|
| **1** | **Material substitution in the arena render** | 🔴 **CONFIRMED — dominant** | Four units collapse to hue 82–83° ±1°; true Sorcerer albedo is 135.2°. Reproduces with a raw StaticMeshActor |
| **2** | **`bUsedWithSkeletalMesh=false` on `M_AssetPBR` + `M_TeamColor`** | 🔴 **CONFIRMED defect; causal link partial** | Both flags false; warning fires in the live PIE gate for 8 SK meshes. Explains the skeletal path, **not** the static one (§P2-9) |
| 3 | Roughness under a directional key | ⚪ **UNMEASURED** | Blocked — the material never rendered. Round 1's roughness 0.6150 (rank 9/13) is not an outlier |
| 4 | Self-shadowing / silhouette | ⚪ **UNMEASURED** | Blocked. Geometry-only luma put Sorcerer 3.3% under Footman — within noise, and not material evidence |
| 5 | Value contrast vs grass | ⚪ **UNMEASURED** | Blocked |
| — | ExponentialHeightFog | 🟢 RULED OUT | §P2-2 |
| — | Auto-exposure / eye adaptation | 🟢 RULED OUT | §P2-1 |
| — | Albedo / ORM / concept / card art | 🟢 RULED OUT (round 1) | unchanged |

---

## P2-11. Priced recommendation

| option | cost | what it does | recommend |
|---|---|---|---|
| **A. Tick `bUsedWithSkeletalMesh` on `M_AssetPBR` and `M_TeamColor`, save, let shaders recompile** | **0 credits · 0 HF · one checkbox each** | Removes the confirmed substitution on the skeletal path — the path that spawns in play — and stops the packaged build shipping default-material units | ✅ **DO THIS FIRST** |
| **B. Re-test the arena render after A** | 0 credits · one capture round | Confirms whether A also clears the static-mesh case, or whether a second cause remains (§P2-9) | ✅ **REQUIRED to close** |
| C. Then, and only then, measure roughness / self-shadowing / grass contrast | 0 credits | The original candidate list, now on a render that is actually showing the material | ✅ after B |
| D. `_D` texture-set re-bake | 0 credits | **No defect to fix** — albedo verified correct and bright, twice | ❌ |
| E. Meshy `image3d` re-gen | **30 credits** | Nothing wrong with the mesh | ❌ **do not spend** |

**Total cost of the recommended path: ZERO credits.** Option A is two checkboxes.

⚠️ **A is an asset edit and I did not make it** — diagnose-only was the standing instruction, and the build-master
was running Git in parallel. It needs its own task, and it dirties `M_AssetPBR` + `M_TeamColor` (shader recompile).

**Honest framing for Jonathan:** his instinct that something is wrong in the arena is correct, but it is not the
Sorcerer and not its art. The art is good — it measured as one of the brightest, best-formed assets in the fleet
twice over. What is broken is the arena *render path*, and it is hitting **every unit**, not just this one. The
Sorcerer is simply the unit whose true colour is furthest from the neutral substitute, so it is the one that reads
most obviously wrong.

## P2-12. Files
**Written:** this note. **Scratchpad (safe to delete):** `decode.py`, `analyze.py`, `shots/*.png`.
**Read-only:** `L_Arena` actors/components · `M_AssetPBR` · `M_TeamColor` · `MI_Sorcerer_PBR` · `MI_TeamColor_Blue` ·
`SM_{Footman,Wizard,Cleric,Sorcerer}` · `T_Sorcerer_{D,N,ORM}` · `T_Footman_D` · `BP_HeroCharacter` CDO · output log.
**Nothing saved. `L_Arena` is dirty in memory and MUST NOT be saved.**

---
---

# PART 3 — THE FIX, AND WHAT THE RE-MEASUREMENT ACTUALLY SHOWED

**Agent:** art-director · **Date:** 2026-08-02 · **Status: FIX APPLIED + SAVED. VERDICT REACHED ON BOTH GAPS.**
**Board task:** _(to be assigned by manager — attach this section as that task's artist handoff; it also EXECUTES the
asset-side half of the long-carried **TASK-325**)_

**Discipline honoured:** PIE never started · `L_Arena` loaded, **NEVER saved** (dirty in memory only, verified still
dirty at exit) · all 8 temp actors removed, **sweep empty, actor count back to 118 = baseline exactly** ·
`MI_Sorcerer_PBR` never touched (verified `is_dirty == false`) · **no Git command of any kind** · no gameplay code ·
**`save_assets` was never called with an empty list** — only the two explicit master paths · editor left OPEN ·
zero Meshy credits, zero HF spend, no re-gen, no re-bake, no new asset.

---

## P3-0. 🎯 HEADLINE — there were TWO independent defects, and PART 2 fused them into one wrong verdict

🎯 **1. The flag fix is real, and it fixed exactly one thing — decisively.** `SK_Sorcerer` rendered **4.13× too dark**
in `L_Arena`. After setting `bUsedWithSkeletalMesh = true`, it brightened from linear luma **0.0235 → 0.0971** and now
matches its own static twin `SM_Sorcerer` (0.0972) **to 0.1%**. Every one of the other seven measurements moved by
≤6% — i.e. frame noise. **Jonathan's complaint was correct, it was the Sorcerer, and it is now fixed.**

🎯 **2. §P2-8's headline verdict — "the arena render substitutes the material" — is REFUTED.** The four-unit
hue-collapse to 82–83° was **not** material substitution. It was the **`CaptureZone_0` decal** painting over every
unit standing on the capture point. PART 2's units were placed "in a row at x = 0 (arena centre)" — dead centre of a
decal whose projection box is **±840 in X and Y**. §P2-9's unexplained static-mesh case is the same thing.

🎯 **The two defects are independent, and I proved it both ways.** After the flag fix, moving the same eight units
back inside the decal box **reproduces the collapse exactly** (six units within **7.2°** of hue, albedos erased).
The flag fix does not touch it, and never could have.

💰 **Cost: two booleans. Zero credits, zero HF, no art re-authored.**

---

## P3-1. The fix, as applied

| master | `bUsedWithSkeletalMesh` before | after | saved | `is_dirty` after save |
|---|---|---|---|---|
| `/Game/Materials/M_AssetPBR` | **false** | **true** | ✅ `M_AssetPBR.uasset` rewritten 12:06 | **false** |
| `/Game/Materials/M_TeamColor` | **false** | **true** | ✅ `M_TeamColor.uasset` rewritten 12:06 | **false** |

Both recompiled via `MaterialTools.recompile` before saving; neither raised a shader-compile error.

### P3-1a. ⚠️ TASK-325 was scoped to ONE master. It needed TWO.

TASK-325's spec names only `M_AssetPBR`. The log proves `M_TeamColor` needed it too —
`LogMaterial: Warning: Material /Game/Materials/Instances/MI_TeamColor_Red missing usage flag SkeletalMesh!`
Fixing only `M_AssetPBR` would have left every unit's `TeamRegion` slot still falling back in a cooked build.

### P3-1b. The other usage flags — checked, and deliberately NOT set

I read **all 27** usage flags on both masters. Every one is `false` except `bUsedWithStaticMesh` (already `true`).
I did **not** set any of the others, and that is a considered decision, not an omission:

- I censused **all 353** "missing usage flag" entries in the session log and extracted which usages the engine is
  actually *asking* for. For these two masters the answer is **`SkeletalMesh` and nothing else.**
- The engine's own tooltip on `bAutomaticallySetUsageInEditor` warns that *"adding usage flags accidentally can add
  unwanted shader permutations."* Each speculative flag costs compile time and package size on two shipped, shared
  masters. Setting flags nothing requests would be a real cost for an imagined benefit.
- `bUsedWithNanite` stays `false` — Nanite OFF is CONVENTIONS law for these assets.

### P3-1c. ⚠️ NEW, unrelated defect found by the same census — NOT mine to fix

```
LogMaterial: Warning: Material /Game/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Trunk_Mobile
                     missing usage flag InstancedStaticMeshes! Default Material will be used in game.
             ... and M_Pack1_Leaf_Mobile, same flag
```
Same class of defect, different flag, different materials — the arena's tree pack would ship **default-material
foliage**. These are not the masters I was sent to fix and they are in a third-party pack folder; **this needs its own
task.** I did not touch them. (Both are the `_Mobile` variants — worth confirming they are actually used before
spending anything.)

### P3-1d. The warnings have stopped — verified, not assumed

Before: 353 entries, latest `19:05:55`. I then ran six fresh full-scene captures and re-counted: **still 359, zero new
entries** (the six between 353 and 359 were emitted during the settle loop *immediately* after the save, before the new
shader map went live). The `LogSkeletalMesh` warning no longer fires.

---

## P3-2. The re-measurement — before/after, identical conditions

**Method.** Eight raw actors — `StaticMeshActor(SM_<Unit>)` and `SkeletalMeshActor(SK_<Unit>)` **interleaved in pairs**
so each unit's two render paths sit side by side under identical light. Row at `x = 2000` (clear of the decal box),
`y = ±140…±980`, yaw 180, flat ground `z = 0`. Camera `(200, 0, 150)` yaw 0, FOV 90, 2764×828 — **the gameplay-facing
direction (+X), i.e. the backlit case**. Background plate captured by lifting the same eight actors to `z = 20000`;
figure pixels = plate-difference > 14/255. Every frame settled with 10–16 successive captures; consecutive-frame
stability verified at **mean |Δ| = 0.26/255** before any number was taken.

**Only the two booleans changed between the two rows below. Same actors, same transforms, same camera, same plate.**

| unit | luma BEFORE | luma AFTER | ratio | L\* before → after | verdict |
|---|---|---|---|---|---|
| SM_Footman | 0.0250 | 0.0242 | 0.97× | 17.9 → 17.6 | unchanged |
| SK_Footman | 0.0281 | 0.0271 | 0.96× | 19.3 → 18.8 | unchanged |
| SM_Wizard | 0.0244 | 0.0239 | 0.98× | 17.7 → 17.4 | unchanged |
| SK_Wizard | 0.0243 | 0.0239 | 0.98× | 17.6 → 17.4 | unchanged |
| SM_Cleric | 0.0888 | 0.0860 | 0.97× | 35.7 → 35.2 | unchanged |
| SK_Cleric | 0.0907 | 0.0857 | 0.94× | 36.1 → 35.1 | unchanged |
| SM_Sorcerer | 0.1002 | 0.0972 | 0.97× | 37.9 → 37.3 | unchanged |
| **SK_Sorcerer** | **0.0235** | **0.0971** | **4.13×** | **17.2 → 37.3** | 🔴 **FIXED** |

### P3-2a. The clincher: static/skeletal agreement

Every other unit's static and skeletal renders already agreed. The Sorcerer's did not — and now does.

| unit | SM luma | SK luma | SK/SM before | SK/SM **after** |
|---|---|---|---|---|
| Footman | 0.0242 | 0.0271 | 1.12 | 1.12 |
| Wizard | 0.0239 | 0.0239 | 1.00 | 1.00 |
| Cleric | 0.0860 | 0.0857 | 1.02 | 1.00 |
| **Sorcerer** | **0.0972** | **0.0971** | **0.23** ⚠️ | **1.00** ✅ |

**Visually confirmed** (`Saved/ArtDiag/p3_beforeafter_sheet.png`): before, `SK_Sorcerer` is a near-black silhouette
with only the blue team helmet legible; after, it is the pale mint/jade robe, cream underlayer, brown staff and antler
crown — **the same read as `SM_Sorcerer` beside it.** No other tile changes between the two rows.

### P3-2b. Honesty note on the hue numbers

Hue separation is now **37° → 163°** across the eight (vs the 1.0° collapse in §P2-8), which satisfies the stated pass
criterion. **But I will not lean on those hue values.** Four of the eight units sit at linear luma ≈ 0.024 — the
backlit shadow side — and a hue angle computed on near-black pixels is numerically unstable. My hue is also a
chroma-weighted circular mean over segmented pixels, which is **not** the same estimator PART 2 used, so 163.3° here is
not comparable to 82.6° there. **The load-bearing evidence is the luminance table, the static/skeletal agreement, and
the visual sheet** — three independent lines that agree. The hue spread corroborates; it does not carry the verdict.

---

## P3-3. §P2-9 CLOSED — the static case was never a material problem

**The mechanism.** `CaptureZone_0` (class `/Script/GitClaudeUnrealTest.CaptureZone`) owns a `DecalComponent`
`ZoneDecal` at world origin, `relativeRotation` pitch **−90** (projecting straight down), material
`/Game/Materials/M_CaptureZone`:

| property | value |
|---|---|
| `zoneHalfExtent` | **(840, 840)** |
| `decalProjectionDepth` | **1024** |
| `DecalSize` | (1024, 840, 840) ⇒ world box **X ±840 · Y ±840 · Z ±1024** |
| `M_AssetPBR.materialDecalResponse` | **`MDR_ColorNormalRoughness`** — units accept the decal over base colour, normal AND roughness |
| `M_TeamColor.materialDecalResponse` | **`MDR_ColorNormalRoughness`** — same |

**The evidence, in the order I got it.**

1. First capture: eight units at `x = 0`, `y = ±140…±980`. **Exactly two rendered solid** — the two at `|y| = 980`.
   The six at `|y| ≤ 700` rendered as pale, vertically-streaked, semi-transparent sage-green. **8/8 correlation with
   the ±840 box, no exceptions.** Vertical streaking down a standing figure is the textbook signature of a
   **top-down projected decal** smearing across near-vertical surfaces.
2. **Not distance, not screen position, not vignette:** re-captured at camera x = −1200 and x = −3600. At −3600 all
   eight cluster in the middle of the frame at near-identical depth — **the same two render solid.** Actor-bound, not
   view-bound.
3. **Not the mesh type:** the solid pair is one static (`SM_Footman`) and one skeletal (`SK_Sorcerer`); the ghost six
   include both types. **Not the material instance:** `MI_Footman_PBR` appears in one solid and one ghost;
   `MI_Sorcerer_PBR` likewise.
4. **Positive control (post-fix):** moved the same eight back to `x = 0`. The collapse **returns**, with the flag now
   correct:

| unit | \|y\| | inside ±840 box | torso mean RGB | hue |
|---|---|---|---|---|
| SM_Footman | 980 | **no** | (59, 62, 50) | 78.1° |
| SK_Footman | 700 | yes | (131, 139, 103) | 74.0° |
| SM_Wizard | 420 | yes | (131, 144, 107) | 81.2° |
| SK_Wizard | 140 | yes | (129, 143, 104) | 81.1° |
| SM_Cleric | 140 | yes | (138, 150, 107) | 77.0° |
| SK_Cleric | 420 | yes | (133, 145, 104) | 77.9° |
| SM_Sorcerer | 700 | yes | (130, 143, 104) | 79.1° |
| SK_Sorcerer | 980 | **no** | (84, 91, 72) | 82.4° |

**The six inside the box agree to within 7.2° of hue and ~8 RGB levels — brown, navy, white and mint all erased to one
colour — while the two outside differ from them and from each other.** That is §P2-8's finding, reproduced on demand,
switched on and off by position alone, with the usage flag already fixed.

**⇒ PART 2's four "units" were standing on the capture point. The measurement was of a decal.** §P2-8's withdrawal of
its own luma numbers was the right call; §P2-8's *conclusion* now goes with them.

### P3-3a. Is the decal itself a defect?

**It is a genuine readability problem, and it is fleet-wide** — any unit fighting on the capture point loses its team
and identity colour. But it is a **design/tuning** question (decal opacity, or narrowing `MDR_ColorNormalRoughness` to
`MDR_Color`, or `bReceivesDecals = false` on unit components), not an asset defect, and **it is not the Sorcerer.**
I have **not** changed it — it is outside this task and touching it would dirty `L_Arena` or a shipped master on a
guess. **Recommend it be taskified separately** with Jonathan's eye on the intended look.

---

## P3-4. The original candidate list, finally measured on a render that shows the material

All four yaws captured out of the decal box, post-fix, same rig. Grass reference: linear luma **0.3032**.

| unit | yaw 180 (faces camera) | yaw 0 | yaw 90 | yaw 270 | **swing (max/min)** | **× darker than grass** |
|---|---|---|---|---|---|---|
| SM_Footman | 0.0242 | 0.0243 | 0.0386 | 0.0357 | 1.60 | **12.5×** |
| SK_Footman | 0.0271 | 0.0256 | 0.0343 | 0.0311 | 1.34 | 11.2× |
| SM_Wizard | 0.0239 | 0.0261 | 0.0222 | 0.0242 | **1.18** | **12.7×** |
| SK_Wizard | 0.0239 | 0.0317 | 0.0291 | 0.0339 | 1.42 | **12.7×** |
| SM_Cleric | 0.0860 | 0.1170 | 0.0835 | 0.1321 | 1.58 | 3.5× |
| SK_Cleric | 0.0857 | 0.1509 | 0.0860 | 0.1405 | 1.76 | 3.5× |
| SM_Sorcerer | 0.0972 | 0.0688 | 0.0539 | 0.0683 | 1.80 | **3.1×** |
| **SK_Sorcerer** | **0.0971** | 0.0704 | 0.0501 | 0.0728 | **1.94** | **3.1×** |

**Candidate 4 — self-shadowing / silhouette: REAL BUT MODEST, and it does not indict the Sorcerer.** The Sorcerer is
indeed the most orientation-sensitive unit measured (**1.94×** skeletal, 1.80× static) — its mantle/hood/antler
silhouette does self-occlude more than the Wizard's simple cone (1.18×). But the effect is a factor of ~2 across a
full 360°, against the **4.13×** the usage flag was costing. It is a second-order term.

**The backlit premise is REFUTED for this unit.** §P2-2a predicted the camera-facing side would be the shadowed,
darkest one. For the Sorcerer, yaw 180 — facing the camera — is its **brightest** orientation (0.0971), and yaw 90 is
its darkest (0.0501). The prediction holds for the Cleric (yaw 0/270 brightest) but not the Sorcerer.

**Candidate 6 — value contrast vs grass: the Sorcerer is the BEST in the group, not the worst.** At **3.1×** below
grass it and the Cleric (3.5×) read clearly. **The Wizard (12.7×) and Footman (12.5×) are four times worse** — those
two are genuinely hard to read against a bright yellow-green field. If there is a remaining readability job in the
arena, **it is the Wizard and the Footman, not the Sorcerer.**

**Candidate 3 — roughness under a single directional key: STILL NOT ISOLATED, and I am not claiming it.** The yaw
sweep captures the *combined* response to key-light angle (diffuse orientation + specular + self-shadow) and I report
it as such. I did **not** vary roughness independently, so I cannot separate its contribution from geometry. Round 1's
figure — Sorcerer roughness 0.6150, rank 9/13, not an outlier — remains the only direct evidence, and it argues
against roughness mattering here.

---

## P3-5. Ranked verdict after PART 3

| rank | candidate | status | evidence |
|---|---|---|---|
| **1** | **`bUsedWithSkeletalMesh=false` on `M_AssetPBR` + `M_TeamColor`** | 🟢 **CONFIRMED CAUSE — FIXED** | `SK_Sorcerer` 4.13× brighter; now matches `SM_Sorcerer` to 0.1%; 7 controls moved ≤6%; engine warnings stopped |
| **2** | **`CaptureZone_0` decal over units on the capture point** | 🟠 **CONFIRMED, NOT FIXED — needs its own task** | ±840 box, 8/8 correlation, reproduced post-fix on demand (§P3-3) |
| 3 | Unit-vs-grass value contrast | 🟡 **MEASURED — real, but Wizard/Footman (≈12.7×), not Sorcerer (3.1×)** | §P3-4 |
| 4 | Self-shadowing / silhouette | 🟡 **MEASURED — real, modest (1.94× over 360°)** | §P3-4 |
| 5 | Roughness under a directional key | ⚪ **STILL UNISOLATED** | not separable from the yaw sweep; round 1 rank 9/13 argues against |
| — | Material substitution of the STATIC path (§P2-8a/P2-9) | 🔴 **REFUTED** | was the decal; `bUsedWithStaticMesh` was already `true` all along |
| — | "Four units render one material" (§P2-8) | 🔴 **REFUTED** | was the decal (§P3-3) |
| — | Fog · auto-exposure | 🟢 RULED OUT (PART 2, unchanged) | §P2-1, §P2-2 |
| — | Albedo · ORM · concept · MI wiring | 🟢 RULED OUT (round 1, unchanged) | PART 1 |
| — | Card-art backdrop (PART 1's separate finding) | 🟠 **STILL OPEN — untouched by this task** | PART 1 §4 |

**Round 1 refuted its own hypothesis. Round 2 refuted its own. Round 3 refuted round 2's — and found the real one.**

---

## P3-6. What build-master must do (no Git was run by me)

1. **Commit exactly two binary assets** — `Content/Materials/M_AssetPBR.uasset` and
   `Content/Materials/M_TeamColor.uasset` (both already saved to disk, `is_dirty == false`). This closes the asset-side
   of **TASK-325**, whose spec must be read as **two** masters, not one (§P3-1a).
2. **Do NOT stage** `MI_Sorcerer_PBR` (untracked and deliberately uncommitted — verified untouched and not dirty),
   and **do NOT stage `L_Arena.umap`** — it is dirty in memory from my temp actors and **must never reach disk**. At any
   editor-close prompt, **DECLINE `L_Arena.umap`**.
3. `git diff --stat` should show the two `.uasset` files and nothing foreign. **No push.**
4. Integration notes: the change adds **one shader permutation** per master. No material parameter, slot, mesh, texture
   or pivot changed — nothing downstream needs re-wiring. All **21** `MI_*_PBR` instances and both `MI_TeamColor_*`
   instances inherit it automatically; none were left dirty.

---

## P3-7. What is still unmeasured / still open

- **Roughness as an isolated variable** (§P3-4) — never separated; low priority, evidence argues it is not a factor.
- **A cooked/packaged build was never verified.** The whole cooked-build argument rests on the engine's own
  `"Default Material will be used in game."` warning, which has now stopped firing. I did not cook.
- **The `CaptureZone` decal** (§P3-3a) — confirmed, deliberately unfixed, needs a task + Jonathan's eye.
- **`Tree_Pack_1` `InstancedStaticMeshes` flag** (§P3-1c) — confirmed, deliberately unfixed, needs a task.
- **PART 1's card-art backdrop defect** — untouched by this task and still open.
- **Why only the Sorcerer lost the race.** All 12 SK meshes logged the warning, yet at capture time only `SK_Sorcerer`
  was visibly substituted while `SK_Footman`/`SK_Wizard`/`SK_Cleric` rendered correctly. The substitution is evidently
  **racy** — the usage check fails on the concurrent path and cannot self-heal there, so which meshes get the default
  material varies per session. **This means the pre-fix symptom was never stable, and "only the Sorcerer looks dark"
  was luck, not a property of the asset.** Fixing the flag removes the race entirely, so this is now moot — but it is
  the honest reason the earlier rounds found a moving target.

## P3-8. Files

**Changed (2, both saved, neither committed):** `Content/Materials/M_AssetPBR.uasset` ·
`Content/Materials/M_TeamColor.uasset` — one boolean each.
**Written:** this section only.
**Evidence (gitignored `Saved/ArtDiag/`, safe to delete):** `p3_beforeafter_sheet.png` (the money shot) ·
`p3_out_before_c.txt` / `p3_out_after_a.txt` (before/after frames) · `p3_plate_out.txt` (background plate) ·
`p3_decalctrl_after.txt` (decal positive control) · `p3_yaw{0,90,270}.txt` · `p3_out_before_sheet.png` ·
`p3_cam{1200,3600}_sheet.png` · `crop_sorc.png` · `crop_left3.png`.
**Scratchpad:** `seg.py` (segmentation/photometry).
**Read only, never modified:** `L_Arena` actors/components · `CaptureZone_0` + `ZoneDecal` · `MI_Sorcerer_PBR` ·
`MI_TeamColor_{Blue,Red}` · `SM_/SK_{Footman,Wizard,Cleric,Sorcerer}` slot tables · output log.
**`L_Arena` is dirty in memory and MUST NOT be saved. Editor left OPEN. No Git command was run.**
