# TASK-012 Handoff — Team-color + ghost materials (art-director)

Status: complete, all assets created in-editor via Unreal MCP, compiled with zero shader errors, and saved to disk.

## Assets created

| Asset | Path | Type |
|-------|------|------|
| Team master material | `/Game/Materials/M_TeamColor` | Material |
| Blue team instance | `/Game/Materials/Instances/MI_TeamColor_Blue` | MaterialInstanceConstant |
| Red team instance | `/Game/Materials/Instances/MI_TeamColor_Red` | MaterialInstanceConstant |
| Placement ghost material | `/Game/Materials/M_Ghost` | Material |

No Blender/ArtStaging source files — these are editor-native material assets with no external inputs.

## Parameter contract (verified character-exact in the editor)

- `M_TeamColor` exposes exactly one parameter: Vector **`TeamColor`** (default 0.5, 0.5, 0.5 neutral grey — instances override it).
- `M_Ghost` exposes: Vector **`GhostColor`** (default linear 0, 1, 0 — green) and Scalar **`GhostOpacity`** (default 0.35).
  - `GhostOpacity` is a bonus knob beyond the TASK-012 contract; TASK-007 code does NOT need to touch it. Setting only `GhostColor` on a dynamic material instance works exactly as specced.

## Verified instance values (read back from the saved assets)

- `MI_TeamColor_Blue`: TeamColor = linear (0.05, 0.30, 1.00, 1.0)
- `MI_TeamColor_Red`: TeamColor = linear (1.00, 0.10, 0.05, 1.0)

## M_TeamColor internals (for the M7 premium pass)

- BaseColor = Lerp(TeamColor * 0.45, TeamColor, VerticalGradient) — darker base to full-brightness top per GDD §6.
- VerticalGradient = B (Z) output of the engine function `/Engine/Functions/Engine_MaterialFunctions02/UVs/BoundingBoxBased_0-1_UVW`, i.e. normalized 0–1 across each mesh's own local bounds. Works at any mesh height (Footman ~180, Castle ~900) with no per-asset tuning.
- Default opaque/lit surface, engine-default roughness. M7 can rebuild the graph in place; only the `TeamColor` parameter name is contractual.

## M_Ghost internals

- BlendMode = Translucent, ShadingModel = Unlit, TwoSided = true (all verified on the saved asset).
- `GhostColor` drives Emissive; `GhostOpacity` (0.35) drives Opacity.

## Notes for downstream tasks

- **TASK-013 / TASK-014 (castle + footman meshes):** assign `/Game/Materials/Instances/MI_TeamColor_Blue` as the single material slot default. The gradient auto-fits each mesh's bounding box — origin at ground/feet-center as specced gives darkest at ground, brightest at top.
- **TASK-002 (ACastle):** the two instance paths referenced in code exist now, exactly as in the spec's names block.
- **TASK-007 (placement ghost):** create a dynamic material instance of `/Game/Materials/M_Ghost` and set vector parameter `"GhostColor"` — green (0,1,0) valid / red (1,0,0) invalid. Because the material is Unlit+Emissive it reads clearly in any lighting.
- Nothing else in /Game/ was touched; no levels were opened or modified.
