# TASK-263 handoff — M_CaptureZone owner-tint decal material

Status: **ready-for-integration**
Agent: art-director
Branch: m7.6-arena10x
Date: 2026-07-23

## Asset created
- **`/Game/Materials/M_CaptureZone`** (Material) — saved to disk at `Content/Materials/M_CaptureZone.uasset`.
- NEW asset (not an edit of the doomed `M_CenterlineStripe`). No new textures — stock/engine nodes only (Custom-HLSL ban honored).

## Material settings (matches the M_SpellReticle / M_CenterlineStripe decal recipe)
- MaterialDomain = **MD_DeferredDecal**
- BlendMode = **BLEND_Translucent**
- ShadingModel = **MSM_DefaultLit**
- Outputs used: **Emissive** + **Opacity** only. BaseColor is intentionally unconnected (emissive-only glow, like M_SpellReticle) — the decal tints/glows over the ground and never writes an opaque albedo slab, so units standing inside stay readable.

## The runtime parameter — how the C++ side drives it
- Parameter name (exact): **`ZoneColor`**
- Type: **Vector parameter** (FLinearColor / VectorParameter node)
- Default value: neutral gray **(0.5, 0.5, 0.5)** — the pre-capture / Neutral look.
- Drive it from `ACaptureZone` (TASK-260) via a Dynamic Material Instance on the DecalComponent:
  - `UMaterialInstanceDynamic* MID = DecalComp->CreateDynamicMaterialInstance();`
  - `MID->SetVectorParameterValue(FName("ZoneColor"), Color);`
  - Owner → color mapping per CONVENTIONS "W1-PREP additions 3":
    - **Neutral** = gray `(0.5, 0.5, 0.5)` (or just leave the default / re-apply it on reset)
    - **Blue** = linear `(0.05, 0.30, 1.00)` (matches MI_TeamColor_Blue)
    - **Red**  = linear `(1.00, 0.10, 0.05)` (matches MI_TeamColor_Red)
- The material is null-safe by contract on the C++ side: if the soft-load of `/Game/Materials/M_CaptureZone` fails, no MID is created, no visual shows, and the capture mechanic still runs (log once) — nothing here forces a hard dependency.

## Visual design (what it looks like)
- Reads as a **square owner-colored region marker**: a bright colored **border ring** around the zone edge + a **very faint interior tint** — so the player can see both "this square is capturable" and "who currently holds it," without obscuring gameplay in the middle where units fight/stand.
- Shaping is UV-space (TexCoord → Chebyshev distance-from-center → SmoothStep border band), so it auto-fits whatever XY size the DecalComponent is scaled to (the 840×840 half-extent zone box). No hardcoded world size.
- Intensity is deliberately low/subtle:
  - Opacity = lerp(**0.08** interior fill → **0.45** border) — tunable in the graph (LinearInterpolate_0 constA/constB).
  - Emissive = `ZoneColor` × lerp(**0.9** interior → **1.8** border glow) — tunable (LinearInterpolate_1 constA/constB); border SmoothStep band = 0.80→0.97 (SmoothStep_0 constMin/constMax).
  - If Jonathan wants it stronger/weaker at playtest, those five constants are the knobs; no re-wire needed.

## Integration notes for build-master (TASK-260 / TASK-264)
- The `ACaptureZone` `DecalComponent` should project **straight down** onto the ground (pitch its rotation so the decal faces −Z) and be scaled to the zone footprint (~840 half-extent in XY; give it enough projection depth in the facing axis to reach the terrain). The square border aligns to the decal box, centered on the actor origin at (0,0,0).
- Decal `SortOrder` should sit above the ground but is a scene-placement detail (integrator's call) — the material itself carries no sort assumptions.

## Acceptance (all met, readback-verified via Unreal MCP)
- Material compiles clean (recompile raised no error).
- `ZoneColor` VectorParameter present and drivable (readback: parameterName="ZoneColor", default (0.5,0.5,0.5,1)).
- Domain/blend/shading confirmed MD_DeferredDecal / BLEND_Translucent / MSM_DefaultLit.
- Emissive ← ZoneColor×glow, Opacity ← fill/border lerp confirmed wired to the material outputs.
- Asset saved to disk (`Content/Materials/M_CaptureZone.uasset`).
