# TASK-1084 — [GROUND-DETAIL] build-master handoff (2026-09-06)

Marker `TASK-1084-GROUND-DETAIL` · assignee build-master · editor PID 20564, MCP `127.0.0.1:8000` · law cited: `FIELD-§1` `FIELD-§2` `FIELD-§3` `FIELD-§6` `SC-§94` cl. A `SC-§88`.

**Status: ready-for-integration.** One knob moved on `Content/Data/DA_BattlefieldScatter.uasset` (`Layers[Grass].InstanceCount` 12,250 → 36,750; the field saturates at ~24.3k placed), saved by explicit path, read back from the engine after the save, `L_Arena` never saved. No commit (TASK-1085 hosts).

---

## 0. THE WHY-FINDING — with the live values, before any number moved (cl. 1)

**The layer was never invisible in the match. Two different things were true at once:**

### 0a. The instrument that produced every "bare field" frame was rendering the EDITOR world, not the match.

- `EditorAppToolset.CaptureViewport` with a `captureTransform` — the posed capture this pipeline has used for evidence (1081's lane vantage, 1083's D-BEFORE/AFTER plan, my own first BEFORE bursts) — **renders the level-editor viewport client's world.** With PIE-in-viewport running it still renders the editor world: no scatter (the scatter is spawned at match start and exists only in the PIE world), no hero pawn, no HUD, no runtime banners/mines.
- **Measured, not argued:** the same pose `(-15000, 0, 260)` pitch −10 captured (i) in PIE, seed 159322881, (ii) in PIE, seed 1309252609, (iii) with **no PIE at all** → mean |ΔRGB| 0.96 / 2.06 / 1.08 of 255 — the same band as burst noise (0.4–1.3). A capture aimed at `PlayerStart_0` from 5 m during a fresh match showed **no hero**. `CaptureEditorImage` (the real screen) during the same PIE showed the game camera behind the hero at the castle gate with the HUD (`Gold: 17`, `Rally Ready`, card bar) — a different picture from every posed frame. The null-transform capture reported the **editor** camera pose `(-20607.8, 0, 98.15) yaw 180`, not the game camera.
- Seven captures aimed by name at live instance positions read from the PIE scatter actor's HISMs (a grass tuft at 3 m, a plant at 3 m, a scatter tree, rock, hill and slab at 30 m) — **all bare**, while the same PIE session's census showed all 59 HISMs visible, `bHiddenInGame=false`, correct cull distances, 12,244 grass instances with sane transforms at Z=0 (floor traced at Z=0). Thumbnails (`CaptureAssetImage`) of `SM_Grass_1/9`, `SM_Plant_6/15` render perfectly — mesh + vendor material are fine.
- **Simulate-in-Editor (`StartPIE bSimulate=true, PlayMode_Simulate`) is the posed instrument that renders the match world** (the editor viewport client itself views the simulated world). `GenerateScatter` runs there (seeds 553927169 / 26739505 below). Every BEFORE/AFTER frame promoted by this task is a Simulate capture; the editor-world frames were NOT promoted. Same-pose editor-world vs Simulate differ by 20–46/255 — see `TASK-1084-INSTRUMENT-editorworld-vs-simulate-lane.png`.
- ⚠️ **Consequence for the lane (SC-§94 cl. A — the capture echoed the editor, not the result):** any PIE-posed `CaptureViewport` evidence of the *scatter* — including the dark-vs-gold canopy reading in `TASK-1081-lane-vantage.png` (which can only show the 15 hand-placed ring trees, not `Layers[Trees]`) and TASK-1083-D1's planned before/after of the scatter trees — is a picture of the editor world. TASK-1083-D1's capture must be re-taken in Simulate to be a measurement of the rostered trees. (D1's material edit itself is unaffected; only its evidence instrument is.)
- Not a defect of X1/X2/X3 in-match experiments either: writing `InstanceEndCullDistance`, `OverrideMaterials` or `bVisible` on live PIE HISMs through `ObjectTools.set_properties` **never reaches the render proxy** (control: the rock HISMs given `M_HillGrass` stayed rock-textured after a visibility toggle). Those experiments are void, not evidence. Recorded so nobody repeats them.

### 0b. In the match world (Simulate), what he actually sees — live DA values, read from the engine (identical to 1081 §1 and 1083 §1)

| layer | InstanceCount | meshes | ScaleRange | MinSpacing | bias | bBlocking | hills | CullStart/End | shadows |
|---|---|---|---|---|---|---|---|---|---|
| `Grass` (idx 5) | **12,250** | 10 · `SM_Grass_1,7,3,5,10,14,27,23,17,9` | 0.7–1.5 | 50 | WholeField | false | yes/35° | **6000 / 9000** | false |
| `Plants` (idx 6) | 2,000 | 5 · `SM_Plant_6,3,8,12,15` | 0.7–1.5 | 200 | WholeField | false | yes/35° | 8000 / 12000 | false |

Globals: `ArenaHalfExtent (26000, 12000)` → 124,800 m² · `CastleKeepClearRadius 4500` · `PlayerStartKeepClearRadius 800` · `CorridorHalfWidth 1000` · `SymmetryMode Rotational180`. Engine log (PIE seed 1309252609): `Layer 'Grass': placed 12230 instances (target 12250 …)`, `Layer 'Plants': placed 2000 (target 2000)`.

Mesh facts (engine `get_bounds`): grass XY half-extents 20–72 uu, heights 39–119 uu (SM_Grass_9 tallest), 5 LODs, LOD0 236–569 tris, LOD1+ 64–74; plants 15–55 uu tall, 5 LODs, LOD0 448–3,438 tris. `r.ViewDistanceScale` = 0.8 (scalability) → effective grass cull 4,800/7,200 uu. `foliage.DensityScale` 0.8 is opt-in and does not bind plain HISMs. No `CullDistanceVolume` in `L_Arena`. `M_Grass` OpacityMask = `Grass Mask.R × "Opacity Mask"(2)` vs clip 0.333 — no distance term (the `CameraPositionWS→Length→Saturate` chain only blends BaseColor, params `Distance offset 100` / `Maxdistance 2500`); `bUsedWithInstancedStaticMeshes=true`.

**Which of the row's candidates it is, with numbers:**
1. **Density (primary).** 12,230 tufts over 124,800 m² = **0.098 / m² — one tuft per ~10 m² (mean spacing ~3.2 m)** for meshes 0.4–1.4 m across. Where it renders it reads as polka-dots on the ground texture, ~10 % of ground pixels (mid-field ROI, below).
2. **The cull band (the header's hypothesis — real, secondary from his camera).** `CullEnd 9000 × 0.8 = 72 m`: beyond it the field is bare to the horizon in every frame; from the 14 m-high lane vantage that is most of the visible ground. From the real game camera (~2–3 m up, behind the hero — see the `CaptureEditorImage`) the far field compresses into a thin strip near the horizon, so the near 0–72 m band is most of his ground pixels — density dominates what he sees.
3. **The keep-clear discs (by design, untouched — TASK-623).** At his spawn `(-23800, 0, 100)` the castle disc (r 4500 + footprint) keeps the nearest tuft ≥ 35 m away; from the courtyard he sees the apron, then a 35–72 m grass strip, then bare. Correct per law; not "fixed".
4. `ScaleRange` 0.7–1.5 and `bCastShadows=false` — not the cause; tufts read clearly at 3–40 m in Simulate. Not moved.

## 1. THE ONE KNOB — and what the field did with it

- **`Layers[5] (Grass).InstanceCount: 12,250 → 36,750`** (3× target). Nothing else on any layer changed (engine diff of the full `Layers` array + globals before/after the write = exactly one entry; see §3).
- **Saturation finding:** the AFTER match placed **24,278 of 36,750 (66 %)** — `Layer 'Grass': placed 24278 instances (target 36750, sym=rot180, pairs 18375, twinSkipped=0, zMismatch=21 …)`. The radius-aware `MinSpacing` (50 uu + both auto footprints, 29–100 uu × scale) with 24 attempts per pair caps the field near ~24k grass. Result = **1.99× the shipped density (0.195 / m², one tuft per ~5 m²)**. If his eye wants more, the next knob is `MinSpacing` 50 → 25 or a `FootprintRadius` override — NOT a higher `InstanceCount`. If the manager prefers the DA to carry the achievable number, 24,500 is a one-line follow-up; I left 36,750 so the log states the cap every match.
- Why this knob and not the cull band: density is what he sees from his camera; extending the band would draw sub-pixel tufts at 100–300 m for GPU time with no VRAM benefit. Left as the second knob for TASK-1086's eye.

## 2. EXPECTED COST — stated before TASK-1085 re-measures (`FIELD-§3`)

- **VRAM: ≤ +3 MB.** +12,048 resident instances × ~128 B GPU-scene data (transform, prev-transform, bounds) ≈ +1.5 MB; CPU `PerInstanceSMData` + cluster tree ≈ +1.5 MB. No new mesh, texture or material — the 10 grass meshes were already resident. Against the ~394 MiB headroom this is inside the noise of 1081's whole-GPU instrument (sessions differ by hundreds of MB from other processes). **1085 should expect a PIE delta indistinguishable from zero; a change of tens of MB would be the machine, not this edit.**
- **Draw:** instances inside the 72 m effective cull disc double (~1,600 → ~3,200; ~half in a 90° frustum), at LOD1–4 (64–74 tris) beyond ~16 m → ≈ +0.1–0.2 M tris/frame, plus proportionally more masked two-sided overdraw near the camera. Expected GPU cost well under 1 ms on the RTX 5070.
- **Match start (measured from log timestamps):** Grass placement 75 ms (16:19:29.087→.162) → **492 ms** (16:47:55.586→56.078): +0.4 s once per match, spent on rejection attempts above the cap. Plants unchanged (2000/2000).
- GPU memory during my sessions, for the record only (Simulate ≠ 1081's PIE instrument): editor idle 2356 MiB used / 5536 free; PIE #1 (BEFORE) converged 7287 / 605 (5 identical samples); PIE #2 6873 / 1019. No in-Simulate reading survived (my sampler ran after the session ended). 1085 owns the AFTER PIE reading at 1081's vantage.

## 3. READ-BACK FROM THE ASSET AFTER THE SAVE (`SC-§94` cl. A) — engine JSON, verbatim

`save_assets(["/Game/Data/DA_BattlefieldScatter"])` → `true`; `is_dirty` → `false`. Re-read from the engine after the save:

```
Layers[5] = {"layerName":"Grass","meshes":[SM_Grass_1, SM_Grass_7, SM_Grass_3, SM_Grass_5, SM_Grass_10, SM_Grass_14, SM_Grass_27, SM_Grass_23, SM_Grass_17, SM_Grass_9] (all /Game/Realistic_Grass_and_plant/Meshes/),
  "instanceCount":36750,"scaleRange":{"x":0.7,"y":1.5},"bRandomYaw":true,"bBlocking":false,"regionBias":"WholeField","minSpacing":50,"zOffset":0,"footprintRadius":0,
  "collisionProxyMesh":"None","collisionProxyScale":{"x":1,"y":1,"z":1},"collisionProxyZOffset":0,"bAllowOnHills":true,"maxPlacementSlopeDeg":35,"overrideMaterial":"None",
  "cullStartDistance":6000,"cullEndDistance":9000,"bCastShadows":false}
Layers[6] = {"layerName":"Plants","meshes":[SM_Plant_6, SM_Plant_3, SM_Plant_8, SM_Plant_12, SM_Plant_15],
  "instanceCount":2000,"scaleRange":{"x":0.7,"y":1.5},"bRandomYaw":true,"bBlocking":false,"regionBias":"WholeField","minSpacing":200,"zOffset":0,"footprintRadius":0,
  "collisionProxyMesh":"None","collisionProxyScale":{"x":1,"y":1,"z":1},"collisionProxyZOffset":0,"bAllowOnHills":true,"maxPlacementSlopeDeg":35,"overrideMaterial":"None",
  "cullStartDistance":8000,"cullEndDistance":12000,"bCastShadows":false}
Globals: ArenaHalfExtent (26000,12000) · SymmetryMode Rotational180 · CastleKeepClearRadius 4500 · PlayerStartKeepClearRadius 800 · CorridorHalfWidth 1000
Layers[0] Trees: instanceCount 340, 12 meshes, [0] = /Game/Tree_Pack_1/Meches/Mobile_Tree_1/SM-Mobile_Tree_1.SM-Mobile_Tree_1  (untouched)
```
Field-by-field diff of the whole DA (7 layers + globals, `{refPath}`/string forms normalised) pre-write vs post-save: **`[[".Layers[5].instanceCount", 12250, 36750]]` — one entry.** `Plants` unchanged in every field. The whole-array writer was proven lossless first on a disposable `SiegeScatterConfig` probe (`/Game/Data/TMP_TASK1084_WriterProbe`, written, diffed empty, deleted, never saved) because `set_properties` rejects element paths (`Layers[5].InstanceCount`).

On disk: `Content/Data/DA_BattlefieldScatter.uasset` sha256 **`798c35797fe1241b52dbcc6a435bbcbbb90a8e77ccae4c943265729484541c19`** (9,098 B) — was `0f23576f…dc64` at start (unchanged by 1083, as expected under Option D). `Content/Maps/L_Arena.umap` sha256 `1f78419d…5622` unchanged; `git status --short Content/Maps/` empty before and after.

## 4. EVIDENCE FOR HIS EYE — Simulate, burst, same poses, promoted under `.claude/pipeline/playtest-evidence/2026-09-06/`

Poses: **lane** = 1081's `(-24000, 0, 1400)` pitch −4 yaw 0 · **hero** = 1 m in front of `PlayerStart_0 (-23800, 0, 100)`: `(-23700, 0, 260)` pitch −10 yaw 0 · **mid** = on the field, `(-15000, 0, 260)` pitch −10 yaw 0. FOV 90, 2764×828. BEFORE = Simulate seed 553927169 (Grass 12,230 placed); AFTER = Simulate seed 26739505 (Grass 24,278 placed). Bursts of 3/3/2 at 1.5 s; consecutive-frame mean |ΔRGB| ≤ 0.6/255 by the last pair (lane 2.27→0.60, hero 2.07→0.56; mid 3.17 between its two).

| file | what |
|---|---|
| `TASK-1084-BEFORE-lane.png` / `TASK-1084-AFTER-lane.png` | lane vantage, last frame of each burst |
| `TASK-1084-BEFORE-hero.png` / `TASK-1084-AFTER-hero.png` | hero's-eye at spawn |
| `TASK-1084-BEFORE-mid.png` / `TASK-1084-AFTER-mid.png` | on the field, 2.6 m eye height |
| `TASK-1084-SIDE-BY-SIDE-lane.png`, `-hero.png`, `-mid.png` | BEFORE over AFTER, ROI boxed in yellow |
| `TASK-1084-INSTRUMENT-editorworld-vs-simulate-lane.png` | §0a proof: same pose, PIE-posed capture (editor world) over Simulate capture |

**Coverage method:** pixel fraction inside a fixed ROI (original px) whose max-channel |Δ| vs the **zero-control** (the editor-world frame of the same pose: same lighting, same ground, no runtime actors) exceeds a threshold; plus "excess green over control" (G − max(R,B) > 20, minus the control's own fraction) as a tuft-specific cross-check. ROIs: lane `(900,500)-(1500,828)` (near-field strip between the banners, below the canopies), mid `(0,250)-(2764,828)` (everything below the tree line), hero `(415,200)-(2620,290)` (the field strip). Sanity: two editor-world frames score 0.00–0.01 %.

| pose | diff>15 BEFORE→AFTER | diff>40 BEFORE→AFTER | excess-green-over-control BEFORE→AFTER |
|---|---|---|---|
| mid | **13.7 % → 18.9 %** (1.38×) | 10.7 → 13.6 | 3.8 → 2.8 (darker AFTER tufts fall under the green threshold — see the eye) |
| lane | **17.8 % → 23.1 %** (1.30×) | 14.5 → 15.7 | **2.9 % → 9.1 %** (3.1×) |
| hero | 53.0 → 46.2 | 42.9 → 35.0 | 7.2 → 13.4 (1.9×) |

Caveats stated: BEFORE/AFTER are different seeds (`OverrideSeed` lives on the level actor; fixing it would dirty `L_Arena`), so every ROI also counts seed-moved trees, rocks, plants and banners — the hero strip is dominated by them and its diff numbers FELL while its green rose; the engine's own placed count (12,230 → 24,278, 1.99×) is the clean number and the pixels corroborate the direction. The eye (side-by-sides): mid-field goes from one tuft per ~3 m to a visibly continuous scatter at 5–30 m; the lane ROI roughly doubles its tufts; beyond ~72 m both are bare (the cull band, not moved).

## 5. FOR TASK-1085 (host)

- **Pathspec from this row:** `Content/Data/DA_BattlefieldScatter.uasset` (oid-vs-sha256 `798c3579…1c19`, never by size) + `.claude/pipeline/handoffs/TASK-1084-buildmaster.md` + the 10 PNGs in §4 (exact names above). Under Option D the DA carries **only this row's `Grass.InstanceCount` edit** — not "both layer edits".
- `git status --short Content/` at my finish: `M Content/Data/DA_BattlefieldScatter.uasset` (mine) · `M Content/Tree_Pack_1/Meches/Mobile_Tree_1/M_Pack1_Leaf_Mobile.uasset` + `M …/M_Pack1_Trunk_Mobile.uasset` (the second build-master's D1, saved during my run — **not mine**, exactly two lines under the pack) · untracked TASK-1093 art-director files under `Content/Characters`, `Content/Materials/Instances`, `Content/RawAssets`, `Content/Textures` (not mine). `Content/Maps/` clean.
- **Timing vs D1:** the D1 material edit landed between my PIE sessions (a third `GenerateScatter`, seed 779836673 at 16:26:40, was the other agent's PIE). My AFTER frames (16:47) therefore show the trees WITH the D1 normal maps; my BEFORE Simulate frames (16:42) were taken after D1 saved too — both evidence sets are post-D1 for the trees, so the tree tone is constant across my pair; do not read any tree difference between 1081's baseline and my frames as this row's.
- **The VRAM after-measurement is a PIE measurement at 1081's vantage** (SC-§88 discipline, PIE `PlayMode_InViewPort`, `nvidia-smi` 10×2 s converged); expect a delta within noise (§2). If 1085 also wants a *visual* re-check, it must use Simulate for any posed capture (§0a).
- No C++ changed ⇒ no compile. Rung 4 stays TASK-1086 (his eye): the pixels prove the field is denser, not that it reads as "a thin layer" to him.

## 6. FOR THE MANAGER (follow-ups, not executed)

1. **Instrument law candidate (SC-§94 family):** a PIE-posed `CaptureViewport` renders the editor world; posed match evidence must be taken in Simulate-in-Editor (`bSimulate=true`). TASK-1083-D1's tree before/after and 1081's dark-canopy attribution are affected (§0a). The real game camera cannot be moved by MCP (no input lane), so Simulate is the only posed match instrument available.
2. **Density cap:** the radius-aware spacing law saturates `Grass` at ~24.3k for these meshes; "more" means `MinSpacing`/`FootprintRadius`, "further" means the cull band — both are separate knobs for after his eye.
3. Optional hygiene: carry `24500` instead of `36750` in the DA to stop the 0.4 s of wasted rejection attempts per match start.

## 7. FENCES — verified after the last engine call

- Written: `DA_BattlefieldScatter` (one field, saved by explicit path), this handoff, the 10 evidence PNGs, my board status line. `save_assets([])` never called; no save-all prompt accepted; the disposable probe asset was deleted (exists → false) and never saved.
- Not touched: `Layers[Trees]` and every other layer/global (engine diff proves it), `Content/Realistic_Grass_and_plant/**`, `Content/Tree_Pack_1/**`, `Source/**`, any `.ini`, `L_Arena` (sha unchanged, never saved). No git add/commit/push.
- Editor left as found: PID 20564, MCP up, `L_Arena` loaded, PIE/Simulate stopped (`IsPIERunning=false`), viewport camera restored to `(-20607.8, 0, 98.15) yaw 180`. PIE was serialised with the second build-master per the coordinator (polled `IsPIERunning` before every start, never stopped a session I did not start, ≤ 60 s sessions). `Saved/GroundDetail/*.txt` (base64 captures, gitignored) left for reproduction.
