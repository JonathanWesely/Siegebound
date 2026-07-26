# TASK-292 handoff — LWC render-precision ensure + InverseFast NaN (gameplay-programmer diagnosis)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-24 · Diagnosis only — NO code, NO config, NO compile, NO Git.
**Outcome:** ROOT CAUSE CONFIRMED with engine-source + live-editor evidence. BOTH build-master hypotheses ("world tile-size
setting" and "degenerate vista transform") are **REFUTED**. The proper fix is a small L_Arena component-property change =
**build-master's lane**. Status → `ready-for-integration`.

---

## 1. Symptom reproduced (not theorized)
`Saved/Logs/GitClaudeUnrealTest.log`, PIE first-frame on L_Arena (frame 417, timestamp 20.13.58–59):
- **L2645 / L2674** — `EnsureFailed: OriginX <= OriginMax && OriginY <= OriginMax && OriginZ <= OriginMax`
  `[DoubleFloat.cpp:19]` + `Found precision loss while converting matrix to GPU format … view transform is invalid, or the
  PreViewTranslation/ViewOrigin was not set up correctly.` — fires ONCE (ensures self-suppress).
- **L2704 / L2708 / L2735** — `TMatrix<T>::InverseFast(), trying to invert a non-invertible matrix, this results in NaNs!`
  `[Matrix.h:468]` — fires ONCE as an ensure, then repeats every frame as a plain `LogUnrealMath: Error` (L2763, L2850,
  L2938-42, … through L6773).
- **Both share the same render-thread callstack** (`UnrealEditor-Renderer.dll` → `UnrealEditor-RenderCore.dll`, the view
  uniform-buffer / matrix-to-GPU conversion). The two top frames differ by ~0x147 = two call sites in ONE function: it both
  DF-encodes a matrix origin (→ OriginX ensure) and inverts a matrix (→ InverseFast).

## 2. Engine-source evidence — the ensure is NOT a magnitude/tile-size overflow
`C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Core\Private\Math\DoubleFloat.cpp:5-25`:
```cpp
constexpr float UE_DF_MIN_PRECISION = 1.0f/(1 << 2);                 // = 0.25
constexpr float UE_DF_FLOAT_MAX_VALUE = ((float)(1<<23) * UE_DF_MIN_PRECISION - 1.0f);  // = 2,097,151
FMatrix CheckMatrixPrecision(const FMatrix& Matrix){
    const double OriginMax = UE_DF_FLOAT_MAX_VALUE;                  // 2,097,151 uu ≈ 21 km
    const FVector Origin = Matrix.GetOrigin();
    ...ensureMsgf(Abs(Origin.X) <= OriginMax && ... , TEXT("Found precision loss..."));
}
```
- **`OriginMax = 2,097,151 uu ≈ 21 km.`** The entire L_Arena scene sits within ±37,000 uu (0.37 km — vista actor bounds max) —
  **56× under the limit.** So ±34,000 does NOT trip this; the task's "340 m is small for LWC" instinct is exactly right.
- `NaN <= 2097151` evaluates to **`false`** → the only way the ensure fires at these magnitudes is a **NaN/Inf** origin.
- `Matrix.h:465-469` — `ErrorEnsure` (the `Matrix.h:468` ensure) is called by `InverseFast()` when the matrix is
  non-invertible; it returns NaNs. **That NaN is what trips the DoubleFloat ensure.** ⇒ the two errors are ONE bug: a
  non-invertible matrix in the render view/scene path produces a NaN that then fails the (unrelated-to-magnitude) DF check.
- **⇒ No `r.LWC.*` / world-tile-size / world-bounds `DefaultEngine.ini` setting can fix this.** `OriginMax` is a hardcoded
  `constexpr` engine constant, not a cvar; and the values are 56× under it anyway — the problem is a NaN, not tile size.
  Applying an LWC ini setting would be a speculative no-op (exactly the guessed-fix trap the task warns against). **NOT DONE.**

## 3. Live-editor evidence — the vista actor transforms are NOT degenerate
Loaded L_Arena in the running editor (read-only; nothing edited/saved) and read all 16 vista actors
(`StaticMeshActor_7..22`) via `get_actor_transform` / `get_actor_bounds`:
- Scales are **UNIFORM** 8×/12×/13×/14× (e.g. SMA_7 = scale(13,13,13) @ (−34000,0,0); SMA_8 = scale(14,14,14) @ (−32000,9000,0)).
  A uniform scale has determinant 8³…14³ (2744 max) → **fully invertible.** No zero component, no non-uniform skew, no NaN.
- World bounds are clean (e.g. SMA_7: x∈[−36603,−31480], z∈[−830,5721]). No Inf/NaN. Farthest geometry ≈ ±37,000 uu.
- ⇒ **There is no "degenerate vista transform" to correct.** The build-master's second hypothesis is refuted at the actor level.

## 4. Live-editor evidence — the ACTUAL gap: DF + Lumen-GI participation left ON
`get_properties` on the vista `StaticMeshComponent0` (checked SMA_7 = SM_Vista_01 @13× and SMA_9 = SM_Vista_04 @14× — consistent):
```
CastShadow                    = false   ✓ (TASK-291 flag set)
bVisibleInRayTracing          = false   ✓
bCanEverAffectNavigation      = false   ✓
bAffectDistanceFieldLighting  = true    ✗  <-- LEFT ON
bAffectDynamicIndirectLighting= true    ✗  <-- LEFT ON
```
Supporting facts:
- `is_nanite_enabled(SM_Vista_01)` = **false** → the vistas are NON-Nanite (art-director used TASK-290's scaled-donor
  fallback, not dedicated Nanite cliffs). Local bounds ≈ 320×270×504 → at 13–14× ≈ **4000×3500×6500 uu** world meshes.
- `Config/DefaultEngine.ini`: `r.GenerateMeshDistanceFields=True`, `r.DynamicGlobalIlluminationMethod=1` (**Lumen**),
  `r.DistanceFields.DefaultVoxelDensity=0.2`.
- `DirectionalLight_0.LightComponent0`: Movable, `DynamicShadowDistanceMovableLight=20000`,
  **`DistanceFieldShadowDistance=30000`** → the light samples mesh distance fields out to 30 km.
- ⇒ Every vista mesh generates a mesh distance field and, with the two flags ON, is inserted into the **distance-field scene
  and Lumen's GI scene** (mesh cards / surface cache). These are huge, heavily-scaled, distant backdrop meshes — precisely the
  geometry whose per-mesh DF/Lumen-card capture/transform matrix is prone to going singular → `InverseFast` NaN → OriginX ensure.
  This is the DELTA the vista ring added (build-master observed the InverseFast is NEW-since-vista); the pre-existing OriginX
  ensure from TASK-287's 10× density fill is the same NaN mechanism from the scatter's DF/Lumen participation (see §6).

## 5. THE FIX (build-master — L_Arena, no code, no compile)
On the **16 vista actors** `StaticMeshActor_7 … StaticMeshActor_22` (refPath
`/Game/Maps/L_Arena.L_Arena:PersistentLevel.StaticMeshActor_N.StaticMeshComponent0`), set:
```
bAffectDistanceFieldLighting   = false
bAffectDynamicIndirectLighting = false
```
Recommended (same rationale — all pure visual dressing, no reason to be in DF/Lumen): apply the same two flags to the **8 POI
props** and **2 gold-node props** (the other TASK-291 StaticMeshActors). Via MCP: `ObjectTools.set_properties` with
`{"bAffectDistanceFieldLighting": false, "bAffectDynamicIndirectLighting": false}` on each component (they render at 20:09,
20:10 backups already carry these actors). Then save L_Arena.

Why this is the PROPER fix (not a dressing hack): distant backdrop/skybox geometry should never participate in distance-field
shadows or dynamic GI — it's the standard treatment and it removes the vista from BOTH candidate singular-matrix paths (DF
object matrix AND Lumen card-capture view). The vista already has `CastShadow=false`, so it contributes nothing to lighting
anyway; there is no meaningful visual change expected. This ADDS two flags to the CONVENTIONS Nanite-vista amendment
(which set shadow/RT/nav/collision but omitted DF+GI) — manager may want to record that.

## 6. What build-master must VERIFY after an editor RESTART
1. **Clean Message Log at PIE first-frame on L_Arena:** NO `OriginX <= OriginMax` ensure (`DoubleFloat.cpp:19`), NO
   `InverseFast … non-invertible → NaN` (`Matrix.h:468`), and no per-frame `LogUnrealMath` NaN spam. Restart is required —
   these fire during render init and ensures self-suppress within a session.
2. **No regression:** traversability (Blue→Red + the 6 mine paths, 0 culls — TASK-291's check) UNCHANGED; the vista ring still
   renders as the far backdrop (pixel-check — expect no visible change since CastShadow was already false); no new perf hit
   (removing meshes from DF/Lumen only reduces cost).
3. **Residual-NaN fallback (documented so build-master isn't stuck):** the OriginX ensure predates the vista (TASK-287), so if
   it (or any InverseFast) STILL fires after the vista flags are off, the same NaN mechanism is coming from another DF/Lumen
   participant — most likely the **scatter layers'** DF at 10× coords. Next lever: (a) pull the vista ring inside
   `DistanceFieldShadowDistance` (≤~28,000, matching the nav bound) and/or cap max vista scale; (b) drop
   `bAffectDistanceFieldLighting` on the scatter HISM layers (a DA/scatter change — hand back to gameplay-programmer if needed).
   Do NOT apply any `r.LWC.*`/world-tile ini setting — §2 proves it cannot help.

## 7. Editor state note
I loaded `/Game/Maps/L_Arena` (it was resting on `L_MainMenu`) to read the transforms — **read-only, nothing edited or
saved.** No PIE was launched by me. Build-master will relaunch/verify/commit as usual; no cleanup owed from this task.

## 8. Files
- Diagnosis evidence (read-only): `Saved/Logs/GitClaudeUnrealTest.log`, `Config/DefaultEngine.ini`, UE_5.8 engine source
  `DoubleFloat.cpp` + `Matrix.h`, live L_Arena actor/component/mesh properties.
- Changed by this task: `.claude/pipeline/TASKBOARD.md` (TASK-292 entry), `.claude/pipeline/handoffs/TASK-292.md` (this file).
- NO source, NO config, NO L_Arena, NO Git touched by gameplay-programmer.
