# TASK-836 — `BP_FogArea` measured in-editor (READ-ONLY)

**Agent:** art-director · **Date:** 2026-09-03 · **Editor:** UP, MCP live (verified with a real
`GetCameraTransform` before any read — returned live camera `(-20607.8, -0, 98.15)`, PIE false).
**Level open:** `/Game/Maps/L_Arena` — **never loaded, never saved, never modified.**

**⛔ ZERO WRITES ATTESTATION.** `git status --porcelain Content/FogArea/` still reports the single
untracked line `?? Content/FogArea/` (Jonathan's import, uncommitted). Every `.uasset` under
`Content/FogArea/` still carries mtime `2026-09-02 20:04` — his import time, not mine. No asset was
created, edited, duplicated, reparented, recompiled or saved; no actor was placed in any level.

---

## 0. THE HEADLINE — answer to `J-F1`, and it is a **measurement**, not a preference

> ### `BP_FogArea` is **PLACE-BASED**. Its density has **ZERO dependence on the viewer**.
> ### And the reason is not an omission — the vendor **wrote** a camera-relative term and then **left it disconnected.**

The material contains exactly one camera-relative node in the entire graph:

```
SphereMask_0 :  A <- CameraPositionWS_0
                B <- WorldPosition_7.XYZ
                Radius <- ScalarParameter_22  =  "Near Camera Fade Distnace"  (default 200, vendor typo)
                Hardness <- (unwired)         ;  node HardnessPercent = 0
OneMinus_2   :  <- SphereMask_0
Multiply_16  :  A <- Multiply_3 (the density chain),  B <- OneMinus_2
```

`Multiply_16` **feeds nothing.** I built the full producer/consumer map of all 149 expressions in
`MF_Fog` and ran an orphan pass: the only unconsumed nodes are `Multiply_16` and
`NamedRerouteDeclaration_0` (the latter a false positive — named reroutes bind by name, and "Wind" is
consumed by `NamedRerouteUsage_1/_4`). The three function outputs are wired as:

```
FunctionOutput_1 "Color"      <- StaticSwitchParameter_5   (bUseDistanceField)
FunctionOutput_2 "Emissive"   <- Multiply_2
FunctionOutput_3 "Extinction" <- Multiply_6 = (Multiply_0 × "Density") × FunctionInput "Mask"
```

Nothing on that chain touches the camera. Traced to the leaves, **Extinction** is:

| term | space | source |
|---|---|---|
| `Density` scalar | — | `ScalarParameter_2`, default 1 |
| bounds mask (`Mask`) | **volume-LOCAL** | `MF_Shapes`: `TransformPosition(WorldPosition → local)` → `SphereMask`/box mask vs. `Local Bounds Size` |
| base noise | **absolute WORLD** | `Divide_8 = WorldPosition_10.XYZ / NoiseScale` → `VT_Noises` |
| distortion | **absolute WORLD** | `Divide_14 = WorldPosition_4.XYZ / (NoiseScale × Distortion Scale)` → `VT_Curl_Low` |
| shape / distance-field / light-shaft gates | local / world | `bUseShape`, `bUseDistanceField`, `bUseLightShafts` |

⇒ **Jonathan's sentence and his asset are describing two different features, exactly as `FOG-§3`
suspected — and this task confirms it from the live graph rather than the name table.** He asked for a
bubble that travels with the eye. The asset paints fog into a *place* and lets you walk into it.

**What each would look like in play, plainly:**

- **Place-based (what the asset does today):** a visible bank of fog sitting somewhere on the field.
  You see it from outside as a discrete cloud with an edge. You walk in; inside, visibility drops.
  You walk out the far side; it clears. From the commander's overhead view it reads as a grey patch on
  the map. Enemies outside it are perfectly visible to you from outside it.
- **Viewer-relative (what he described):** no cloud, no edge, nowhere to stand outside it. Everyone on
  the field carries their own 20-ft bubble. The map looks uniformly socked in from every camera, and
  there is nothing to *see* — the fog is only ever a wall at arm's length.

⚠️ **These are not tunings of one another.** No parameter on this asset converts one into the other.

---

## 1. What it actually is — and `FOG-§3` needs one correction

`FOG-§3` called the material a *"raymarched translucent material"* and reasoned about a
*"translucent pass"*. **That is wrong, and the correction matters more than the error.**

| property | measured value |
|---|---|
| Blueprint parent | `/Script/Engine.Actor` ✅ (as `FOG-§3` had it) |
| **`M_FogArea` MaterialDomain** | ⭐ **`MD_Volume`** — *not* a surface/translucent material |
| BlendMode / lighting | `BLEND_Additive`, `TLM_VolumetricNonDirectional`, `MSM_DefaultLit` |
| outputs used | `MP_BaseColor` (Albedo), `MP_EmissiveColor`, **`MP_SubsurfaceColor` = the Extinction pin** |
| mesh component | `Mesh_GEN_VARIABLE`, `/Engine/BasicShapes/Sphere`, **default scale `(20, 20, 5)`** ⇒ a **2000 × 2000 × 500 uu** ellipsoid |
| mobility | ⛔ **`Static`** ✅ |
| collision | ⛔ **`NoCollision`** profile, `bGenerateOverlapEvents = false` ✅ |
| tick | ⛔ **`bCanEverTick = false`, `bStartWithTickEnabled = false`** |
| replication | `bReplicates = false`, `InitialLifeSpan = 0` |
| all 7 `MI_FogArea_*` | descend from `M_FogArea` (Box variants via `MI_FogArea_Box`) — **one shader family** |

**Why the correction matters:** a `MD_Volume` material is **not drawn by the mesh at all.** The mesh is
only a *bounds volume* that injects albedo/emissive/extinction into Unreal's **Volumetric Fog froxel
grid**. Everything `FOG-§3` inferred about translucency cost and sort-order artefacts is off-target;
the real constraints are the froxel grid's, and they are different and mostly *better*.

**Pixel corroboration (in-fence, zero writes):** `CaptureAssetImage` on `M_FogArea` renders **empty** —
just the alpha checkerboard, nothing drawn. Control: `CaptureAssetImage` on the project's own
`/Game/Materials/M_AssetPBR` renders a fully-shaded sphere. Same renderer, same call, one frame apart.
The blank is not a broken capture — **it is what a volume material looks like with no volumetric-fog
host present**, which is precisely the condition described in §2.
Saved: `…\scratchpad\M_FogArea_thumb.png` and `…\scratchpad\M_AssetPBR_thumb.png`.

### The three inert stubs
`EventGraph` contains `EventBeginPlay`, `EventActorBeginOverlap` and `EventTick` — **all three with no
connected logic.** `EventTick` can never fire (`bCanEverTick = false`) and `EventActorBeginOverlap` can
never fire (`NoCollision` + overlap events off). `FOG-§3`'s *"ZERO GAMEPLAY HOOKS"* is confirmed, and
now with the reason: the vendor left the hooks *drawn but dead*. **Do not read those stubs as an
existing "player entered the fog" signal.** Every gameplay effect in his paragraph is new C++.

---

## 2. ⛔⛔ THE BLOCKER NOBODY HAS COSTED — `bEnableVolumetricFog = false`

> ### Measured on `L_Arena.PersistentLevel.ExponentialHeightFog_0.HeightFogComponent0`:
> ### **`bEnableVolumetricFog = false`.**
> ### ⇒ **Dropped into `L_Arena` today, `BP_FogArea` renders literally nothing.** Not faint. Nothing.

A `MD_Volume` material has no other rendering path. `r.VolumetricFog = 1` (the feature is allowed
project-wide) but the **level's own host is switched off**, so no froxel grid is built and there is
nothing for the volume to inject into.

**The good news, also measured — the rig is otherwise already volumetric-ready:**

| thing | measured | verdict |
|---|---|---|
| `r.VolumetricFog` | `1` | ✅ enabled |
| `DirectionalLight_0` | `Movable`, `VolumetricScatteringIntensity = 1`, `bCastVolumetricShadow = true` | ✅ will light the fog |
| `SkyLight_0` | `Movable`, `VolumetricScatteringIntensity = 1` | ✅ (a *Static* skylight would not have worked) |
| `volumetricFogDistance` | `6000` uu | ✅ 609.6 sits comfortably inside |
| `volumetricFogStartDistance` | `0` | — |
| grid | `GridPixelSize 16`, `GridSizeZ 64`, `DepthDistributionScale 32` | — |

⚠️ **But it is a one-checkbox change with a level-wide blast radius.** `bEnableVolumetricFog` is a
property on `L_Arena`'s height fog — a level under a **never-save law with a pinned hash** — and
switching it on changes **every pixel of the level, permanently, fog card or no fog card**
(`fogDensity = 0.012`, `fogMaxOpacity = 0.92`, `startDistance = 10000`). That squarely threatens the
`TASK-620..622` Lighting-Wave-1 pixel gate that already passed. 🧑 **This needs Jonathan's ruling and a
re-run of the lighting gate — it is not a detail an integration task should absorb silently.**

---

## 3. Exposed parameters — full inventory, measured (name · default · range)

`Density` has no clamp above 0; sliders reading `SliderMin = SliderMax = 0` mean **no slider range was
authored** (free entry), not a range of zero.

**Blueprint-level (the Details panel Jonathan sees):**

| property | type | default | range |
|---|---|---|---|
| `bBoxShape` | bool | **false** (⇒ Sphere) | — |
| `bTraceSurface` | bool | false | one-shot: traces down 2500 uu, snaps to ground, unticks itself |
| `MaxDrawDistance` | float | **25000** | see §5 |
| `Material Mode` | enum | `Dynamic` | `Dynamic` \| `Static` |
| `Mode` | enum | `Base` | `Base` \| `Shadows` \| `DistanceField` \| `LightShafts` |
| `General Data.density` | float | **1.0** | min 0, no max |
| `General Data.base Color` | LinearColor | (1,1,1,1) | — |
| `General Data.emissive Color` | LinearColor | (0.05,0.05,0.05,1) | — |
| `General Data.mask Margin` | float | **3.0** | min 0 |
| `General Data.wind Speed` / `wInd World Space` | float / bool | 1.0 / false | — |
| `Noise Data.scale` | float | **2.5** | — |
| `Noise Data.sharpness` | float | 0.35 | **−1 … 1** |
| `Noise Data.channel` | enum | R | R\|G\|B\|A |
| `Noise Data.noise Texture` | VolumeTexture | None (master default `VT_Noises`) | — |
| `Noise Data.distortion Intensity` | float | 0 | **−1 … 1** |
| `Noise Data.distortion Scale` / `Speed` | float | 4.0 / 0.5 | — |
| `Shape Data.bUseShape` | bool | **false** | — |
| `Shape Data.rotation` | float | 0 | **0 … 1** |
| `Shape Data.scale` | float | 1 | **0 … 1.25** |
| `SelfShadows Data.offset` | float | 0.02 | **0 … 0.25** |
| `SelfShadows Data.intensity` | float | 3 | **0 … 50** |
| `Distance Field Data.{colorDistance, offset, distance}` | float | 150 / 60 / 250 | — |
| `Shafts Data.{scale, sharpness, intensity, speed}` | float | 450 / 0 / 1 / 0.5 | sharpness **−1 … 1** |

**Material parameters (`M_FogArea` + `MF_Fog` + `MF_Shapes`), grouped as authored:**

- **Base** — `Density` 1 · `Base Color` (1,1,1) · `Emissive Color` (0,0,0) · `Wind Direction` (1,0,0) ·
  `Wind Speed` 0.5 · `Global Wind` 0 · ⭐ **`Near Camera Fade Distnace` 200 — DISCONNECTED**
- **Base|Noise** — `Base Noise Texture` `VT_Noises` · `Base Noise Size` 3 · `Base Noise Intensity` 1 ·
  `Base Noise Sharpness` 0.15 · `Base Noise Channel` (1,0,0,0)
- **Base|Distortion** — `bDistrotion` **true** (sic) · `Distortion Texture` `VT_Curl_Low` ·
  `Distortion Intensity` 0 (**−1…1**) · `Distortion Scale` 2 · `Distortion Speed` 0.25
- **Shape** — `bUseShape` true · `Shape Texture` `WhiteSquareTexture` · `Shape Rotation` 0 ·
  `Shape Scale` 1 (**0…1.25**) · `ShapeChannel` (1,0,0,0) · `Mask Margin` 3
- **Shadows** — `bUseShadows` false · `Shadows Color` (0.02³) · `Shadows Intensity` 7 · `Shadows Offset` 0.05
- **DistandField** (sic) — `bUseDistanceField` false · `Distance Field Color` (0.25³) ·
  `Disance Field Gradient` 50 · `Disance Field Color Gradient` 50 · `Disance Field Offset` 0
- **LightShafts** — `bUseLightShafts` false · `LightShafts Size` 400 · `LightShafts Speed` 0.02 ·
  `LightShafts Sharpness` −0.1 · `LightShafts Intensity` 1
- **M_FogArea** — `bBoxMask` false

⭐ **There is no parameter anywhere in this asset that expresses a distance-from-viewer onset or
ceiling.** The only candidate is the disconnected one.

---

## 4. Runtime spawn / destroy — the fog card is a timed spell, so this is load-bearing

| question | measured | verdict |
|---|---|---|
| Is all setup in the Construction Script? | ✅ yes — `UserConstructionScript` picks Cube/Sphere, calls `SetMaxDrawDistance`, `CreateMID`, then pushes all six struct setters | ✅ **`SpawnActor` runs the CS, so a spawned instance configures itself** |
| Any BeginPlay dependency? | none — the stub is empty | ✅ |
| Can density be animated at runtime? | ✅ `SetGeneralData` writes `Density`, `Base Color`, `Emissive Color`, `Wind Direction`, `Global Wind`, `Wind Speed`, `Mask Margin` straight onto the MID | ✅ **fade-in / fade-out is available** |
| Can it drive its own fade? | ⛔ **no** — `bCanEverTick = false` and every event stub is empty | ⚠️ **the timer must live outside this actor** (C++ or a child BP) |
| Destroy? | plain `AActor` | ✅ `Destroy()` is fine |
| ⛔ **Mobility** | ⛔ **`Static`** | ⚠️ **the risk — see below** |
| ⚠️ Spawn hitch | `CreateMID` calls **`LoadAsset_Blocking`** on the chosen `MI_` soft reference | ⚠️ **synchronous load on the frame the card is played** unless pre-warmed |

⚠️ **`Static` mobility on a runtime-spawned actor is the one item I could not settle by reading.** It
should render, but it cannot be moved after spawn, and I could not confirm whether 5.8 logs a mobility
ensure on the spawn path — **testing that requires PIE and a placed actor, both outside this task's
fence.** The safe integration move is a **duplicate** (never an edit of the vendor asset,
`FOG-§6`) with `Mobility = Movable`, plus pre-loading the `MI_` to kill the blocking load.

---

## 5. Scale to arena size, and what `MaxDrawDistance` actually does

**`MaxDrawDistance` is not a fog parameter.** The CS calls `UPrimitiveComponent::SetMaxDrawDistance` on
the mesh — a **per-primitive cull distance**. Past it the primitive stops being rendered *entirely*;
it is a hard pop, not a falloff. Default `25000`.

**Scaling one volume to 52,000 × 24,000 uu — measured reasoning:**

- ✅ **The raymarch does *not* break down, because there is no per-volume raymarch.** Cost lives in the
  froxel grid, which is **view-resolution-sized, not volume-sized** (`GridPixelSize 16`, `GridSizeZ 64`).
  A huge volume is close to free. This is the single biggest practical advantage of it being `MD_Volume`,
  and it reverses the concern the task spec was written around.
- ✅ **Noise does not stretch.** `Base Noise` and `Distortion` are sampled from **absolute world
  position** (`WorldPosition / NoiseScale`), not local UVs — so grain stays constant at any volume size.
- ⚠️ **The bounds mask *does* scale.** `MF_Shapes` derives both radius and edge falloff from
  `Local Bounds Size` × `Mask Margin`, so at arena scale the soft edge becomes proportionally enormous.
  `Mask Margin` would need retuning.
- ⛔ **`volumetricFogDistance = 6000` is the real ceiling, not the volume size.** No volumetric fog is
  computed beyond 6000 uu from the camera regardless of how big the volume is. Fine for a 609.6 uu
  vision band; it does mean **you can never see fog "rolling across the map" at distance**, and it is a
  live interaction with `J-F5` (the overhead/war-map view).
- ⚠️ **`MaxDrawDistance = 25000` becomes a trap at arena scale.** UE culls by distance from the view
  origin to the primitive's *bounds origin* — so a volume centred mid-arena is ~26,000 uu from a camera
  at either castle (castles at ±25,000) and would **cull out**. *Inferred from the culling rule, not
  measured* — I could not place an actor to observe it. At the 2000-uu default size it is inert
  (25,000 ≫ 6,000).
- ⚠️ **Memory, measured from the build log:** `VT_Noises` is a 128³ BGRA8 volume texture,
  **~81 MB estimated**; `VT_Curl_Low` 32³, ~2.2 MB. Both built cleanly.

---

## 6. ⛔ Incompatibility with `FogVisionOnsetUU = 304.8` / `FogVisionCeilingUU = 609.6`

Verified the shipped constants are still `SiegeFogStatics.h:110` `304.8f` and `:153` `609.6f`, with
`FogDensityExponent = 2.f` at `:189`. (Noted in passing: `FogVisionCeilingUU` still carries
`meta = (ClampMin = "0")` at `:152` — `FOG-§7b`'s raise to `"304.8"` has **not** landed yet, correctly,
since it ships with `TASK-838`.)

> ### ⛔ **A uniform-density volume CANNOT be clear at 304.8 and opaque at 609.6. This is arithmetic, not tuning.**

With uniform extinction σ, transmittance is `T(d) = exp(−σd)`, so for *any* σ whatsoever:

```
T(304.8)  =  T(609.6) ^ (304.8 / 609.6)  =  √T(609.6)
```

The onset is locked to the square root of the ceiling. **No parameter on this asset can change that.**

| tune it so 609.6 reads… | required σ /uu | ⇒ forced transmittance at 304.8 | ⇒ obscuration at the ONSET | shipped C++ says |
|---|---|---|---|---|
| 98% obscured (T = 0.02) | 0.006417 | 0.1414 | ⛔ **85.9%** | **0.0%** |
| 95% obscured (T = 0.05) | 0.004914 | 0.2236 | ⛔ **77.6%** | **0.0%** |
| 99% obscured (T = 0.01) | 0.007554 | 0.1000 | ⛔ **90.0%** | **0.0%** |

⇒ **At the exact distance Jonathan asked for "a light amount of fog", this asset is forced to deliver
78–90% obscuration.** And the curve shape is Beer-Lambert — **concave, asymptotic** — which is the exact
shape `FOG-§7a` already ruled out twice, in writing, as *"the exact opposite of 'thicker and thicker'"*
and *"asymptotic, so it can never reach the state he named."*

⚠️ **So the vendor asset's own falloff cannot be made to agree with the shipped `t²` curve. Per the
task brief, that is a finding, not a detail.** The integration pixel gate (*zero fog inside 304.8,
opaque at 609.6*) **cannot be passed by this asset in its shipped wiring.**

---

## 7. ⭐ The one path that reconciles them — and it is the vendor's own node

`Multiply_16` is not junk. It is the missing feature, unplugged. `1 − SphereMask(CameraPositionWS,
WorldPosition, Radius, Hardness)` is **structurally exactly the normalized band** `FOG-§7a` defines:
zero at the near edge, rising to one at `Radius`, clamped. Set `Radius = FogVisionCeilingUU = 609.6`
and `Hardness = Onset/Ceiling = 0.5`, insert a `Power(·, FogDensityExponent)`, and you have
`FogDensityAt` **reproduced in the material** — camera-relative, quadratic ease-in, hard cut at the
ceiling, all three constants driven from the same pinned numbers.

⚠️ **The exact algebra of UE's `SphereMask` Hardness convention is *inferred, not measured*** — I could
not run a numeric probe without writing. **The implementing task must verify it numerically before
relying on the mapping.**

⛔ **This requires a DUPLICATE** of `M_FogArea`/`MF_Fog` into `/Game/Materials/` — `Content/FogArea/` is
a read-only vendor pack (`FOG-§6`) and must not be edited in place.

⚠️ **And it still does not make the volume viewer-relative in the way he described.** A camera-relative
*density* inside a *bounded* volume still only exists where the volume is. Genuinely map-wide,
follows-the-eye fog wants either (a) the volume parented to the view, or (b) `L_Arena`'s own
`ExponentialHeightFog` volumetric parameters — which already expose `volumetricFogStartDistance` and
`volumetricFogNearFadeInDistance` and are *natively* camera-relative. **Option (b) is worth Jonathan's
attention: it may deliver his sentence with no new asset at all.**

---

## 8. ⛔ WHAT I COULD NOT DETERMINE — stated plainly, because two tasks build on this

1. ⛔ **No in-level pixel test of the fog was run.** The board asked for pixels; the fence forbade
   placing an actor and `L_Arena` is under a never-save law. It would also have been **uninterpretable**
   — with `bEnableVolumetricFog = false` the correct result is a blank frame either way. *The headline
   question was instead settled structurally, which is stronger evidence than a screenshot: a photo of a
   uniform volume cannot distinguish "uniform density" from "camera falloff" without controlled camera
   moves, whereas a disconnected wire is dispositive.*
2. ⛔ **The vendor demo map `Content/FogArea/Maps/Overview.umap` was NOT opened.** It is the one place
   the asset is set up to look right. Loading it would unload `L_Arena` and risk a modal save prompt.
   **Recommend a dedicated task with an explicit grant to open it** — that is where the real look lives.
3. ⛔ **Runtime spawn was not executed** (needs PIE). `Static` mobility behaviour on spawn is
   *unverified*.
4. ⛔ **`MaxDrawDistance` bounds-origin culling at arena scale is inferred**, not observed.
5. ⛔ **`SphereMask` Hardness algebra is inferred** (§7).
6. ⛔ **Cost was not profiled.** No `stat gpu`, no froxel timings.
7. ⛔ **Exposure interaction not measured numerically** — I read the PPV is `bUnbound`, priority 1, but
   did not dump its exposure settings.

## 9. ⚠️ EDITOR STATE — for whoever touches it next

- The editor **autosaved `BP_FogArea`** to `Saved/Autosaves/Game/FogArea/Blueprints/BP_FogArea_Auto3.uasset`
  at `05:03:02` — i.e. **the package is DIRTY in memory.** Loading a `++UE5+Release-5.5` asset into 5.8
  silently upgrades and dirties it. The **on-disk vendor file is untouched.**
  ⛔ **DECLINE any save prompt for `BP_FogArea` or `L_Arena`.**
- ✅ **Answering board item (2):** the 5.5 asset **loads, compiles and builds cleanly in 5.8.** All five
  textures built (`VT_Noises` 128³, `VT_Curl_Low` 32³, plus 2D sources). **Zero upgrade warnings, zero
  shader compile errors** — I searched the session log for `older/newer version`, `package version`,
  `shader compile error`, `failed to compile` and found **nothing** for this pack. The only FogArea log
  lines are my own read-tool property misses.

## 10. Paths

- Vendor pack (⛔ READ-ONLY): `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\FogArea\`
- `/Game/FogArea/Blueprints/BP_FogArea` · `/Game/FogArea/Materials/M_FogArea` ·
  `/Game/FogArea/Materials/Functions/MF_Fog` · `MF_Shapes` · 7 × `/Game/FogArea/Materials/Base/MI_FogArea_*`
- Vendor demo map (unopened): `/Game/FogArea/Maps/Overview`
- Pixel evidence: `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\M_FogArea_thumb.png` (empty) and `M_AssetPBR_thumb.png` (control, renders)
- Shipped constants cross-checked: `Source\GitClaudeUnrealTest\Siegebound\SiegeFogStatics.h:110, :153, :189`

## 11. For `TASK-841` (visual) and `TASK-839` (fog volume) — the three things that change your spec

1. ⛔ **`bEnableVolumetricFog = false` on `L_Arena` blocks both of you.** Nothing renders until it is
   on, and turning it on is a level-wide look change needing Jonathan + a lighting-gate re-run.
2. ⛔ **You cannot hit the `t²` curve with this asset as wired.** Uniform density forces 78–90%
   obscuration at the 304.8 onset. Either duplicate + reconnect the camera term (§7), or use
   `ExponentialHeightFog`'s native camera-relative volumetric parameters.
3. ⚠️ **Duplicate, never edit.** `Content/FogArea/` is a vendor pack; `Static` mobility and the
   `LoadAsset_Blocking` in `CreateMID` both want fixing **in the duplicate**.
