# Handoff — TASK-176 (Lumen key+GI + post stack) & TASK-177 (stylized gradient skybox + skylight)

**Author:** art-director · **Date:** 2026-07-16 · **Map:** `/Game/Maps/L_Arena` (saved)
**Status:** ready-for-integration → build-master (compile-free; scene + 1 material asset only)

The M7 §6 scene-lighting / post-process / skybox pass. Additive scene/lighting only — NO gameplay
logic, NO collision/navmesh/transform changes.

---

## Assets created

| Asset | /Game/ path | Source |
|---|---|---|
| `M_Skybox` (Material) | `/Game/Materials/M_Skybox` | authored from scratch via MCP MaterialTools (no raw file — pure node graph) |

`SkyDome_Gradient` uses the engine mesh `/Engine/BasicShapes/Sphere` (soft-ref, not duplicated).
No new textures/FBX. Nothing written to `Content/RawAssets/` (mesh batch lane untouched).

## New actors in L_Arena (outliner folder `M7_SceneLighting`)

- **SkyDome_Gradient** — StaticMeshActor, `/Engine/BasicShapes/Sphere`, loc (0,0,0), scale 900 (~45000 u radius),
  material slot 0 = `M_Skybox`. CastShadow OFF, bAffectDynamicIndirectLighting OFF, bAffectDistanceFieldLighting OFF,
  **collision profile NoCollision**, bCanEverAffectNavigation FALSE.
- **TeamFill_Cool_Blue** — RectLight @ (-4000, 0, 1600), pitch -90 (down). Color linear (0.30,0.50,1.0),
  20000→38000 lm, SourceWidth 4000 / Height 7000, AttenuationRadius 6500, **CastShadows OFF**.
- **TeamFill_Warm_Red** — RectLight @ (+4000, 0, 1600), pitch -90. Color linear (1.0,0.45,0.15), 38000 lm,
  same geometry/atten, **CastShadows OFF**.
- **PP_Arena_Global** — PostProcessVolume @ (0,0,500), **bUnbound = true**, Priority 1.0 (see PPV settings below).

## Existing actors re-tuned (NOT created)

- **DirectionalLight_0** (key) — rotation pitch **-38** / yaw **145** / roll 0 (low warm rake across the field);
  Intensity **11**, Temperature **5400K** (bUseTemperature), LightSourceAngle **1.5** (softer shadows). Movable, shadows on.
- **SkyLight_0** — LightColor cool (0.80,0.87,1.0), Intensity **1.0**, SLS_CapturedScene + bRealTimeCapture (feeds Lumen GI).
- **VolumetricCloud_0** — component `bVisible = false` (HIDDEN — removes the default-HDRI cloud band so the dome reads stylized;
  actor NOT deleted, trivially reversible).
- **SkyAtmosphere_0** — left as-is (occluded by the dome from player view; still feeds the cool real-time SkyLight capture).

## PP_Arena_Global settings (all bOverride_* set true)

- Bloom: Intensity 0.6, Threshold 0.85 (subtle; gives bright emissives/VFX a warm halo)
- Vignette: Intensity 0.4 (subtle)
- Color grade (Ori rule): ColorSaturation (0.9,0.9,0.9,1.0) = mild GLOBAL environment desaturation; ColorContrast (1.05,1.05,1.05,1.0)
- Exposure: **manual-locked** — AutoExposureMinBrightness = MaxBrightness = 1.0, AutoExposureBias 0.4 (deterministic stylized grade, no auto-exposure wash)
- Lumen pinned: DynamicGlobalIlluminationMethod = Lumen, ReflectionMethod = Lumen (also confirmed active project-wide via cvars = 1)

## Warm-vs-cool team framing (§6)

Blue half = X ≤ 0 (cool), Red half = X ≥ 0 (warm) per the arena contract. Warm key sun + cool SkyLight give the
global warm-key/cool-shadow cinematic contrast; the two RectLights paint the per-half split and blend to a neutral
centerline. Center field stays desaturated green (characters pop against it = Ori rule).

## M_Skybox graph (for reference)

Unlit + TwoSided. WorldPosition → ComponentMask(Z) → Multiply(×0.0000222 = 1/45000) → Saturate → LinearInterpolate
(A = warm horizon (0.70,0.38,0.17), B = cool zenith (0.09,0.15,0.32), Alpha = t) → Emissive. Colors are Constant3Vectors
(easy to re-tune in-editor). Reads cool-neutral top → warm horizon per the §6 palette.

---

## FLAGS / notes for integration & downstream

1. **No LUT asset** (TASK-176 named `T_ColorGrade_LUT` "if authored"). The MCP toolset has no LUT texture import/assign
   path, so the color grade is approximated with the PPV color-grading controls (mild global desat + contrast + filmic-locked
   exposure). This satisfies the §6 "slightly desaturated environment" intent. A hand-authored LUT is a future in-editor polish pass.
2. **Gold node** (`GoldNode_Blue/Red`, `M_GoldGlow`) is present + glowing (brightest object, casts a warm light pool). At the
   manual-locked exposure its emissive core clips toward white (ACES tonemapper highlight desaturation). **I did NOT touch
   M_GoldGlow or the GoldNode actors** — this is the authored material's high emissive, not a regression. At normal
   gameplay/cinematic distance it reads as a warm glowing node; only the extreme close-up (05_AFTER_goldnode.png) exaggerates the
   white core. If richer yellow is wanted, that's a `M_GoldGlow` tweak (cross-map, out of this lane).
3. **Perf note for TASK-183:** +2 dynamic RectLights (shadow-casting OFF, so cheap) + Lumen GI. The arena's large white perimeter
   walls bounce a lot of GI (flat-ish fill is partly inherent to that blockout). Nothing egregious; watch Lumen cost at Jonathan's
   1440p/RTX3060/60-unit playtest. Manual-locked exposure means no auto-exposure cost.
4. **Collision/nav safety:** SkyDome uses NoCollision profile (empty response array) + nav disabled + its shell is at radius
   ~45000 (far outside the ±8000 play area) → provably cannot affect gameplay collision or navmesh. RectLights/PPV are
   non-colliding by nature. build-master PIE check should confirm pathing/placement unchanged.
5. **Gameplay actors verified UNCHANGED** (before == after): Castle_0 (-8000,0,0), Castle_1 (8000,0,0, yaw180),
   GoldNode_0 (-7200,0,0), GoldNode_1 (7200,0,0). No PlayerStart/nav-bounds/spawn logic touched.
6. **TASK-178 (arena set dressing) NOT done in this pass** — remains `backlog`. It's a separate, larger job requiring a
   build-master PIE collision-regression confirmation; out of scope for the lighting/post/skybox mandate.

## Screenshots (before/after — for Jonathan's review + M7 before/after slice)

Absolute paths under `C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Saved/Screenshots/M7_SceneLighting/`:
- `01_BEFORE_arena_side.png`  — high side view, default flat blue sky + washed lighting
- `02_BEFORE_arena_hero.png`  — high 3/4 corner, same
- `03_AFTER_arena_side.png`   — same camera as 01, new lighting/sky/grade
- `04_AFTER_arena_hero.png`   — same camera as 02 (money shot: gradient sky + warm/cool framing)
- `05_AFTER_goldnode.png`     — Red-castle + gold-node close-up (warm framing; gold-node glow note #2)

Camera poses (reproducible): side = loc (0,-14000,6000) rot (-21,90,0); hero = loc (-13000,-13000,8000) rot (-23.5,45,0).
