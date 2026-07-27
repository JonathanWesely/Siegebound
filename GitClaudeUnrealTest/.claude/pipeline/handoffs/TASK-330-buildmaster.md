# TASK-330 — [CASTLE-int] Same-path SM_Castle import + LOD chain + hard LOD readback + Simulate verify — build-master handoff

**Date:** 2026-07-27 · **Branch:** main · **Editor:** live session (MCP + TASK-221 remote-exec python lane; `bRemoteExecution` flipped in-memory, reverts on restart)
**Verdict: PASS — imported, hard-gated, Simulate-verified, committed.**
**⚠️ THE CASTLE IS NOT SHIPPED UNTIL TASK-331 LANDS.** `SM_Castle_Crumble01/02/03` are still byte-copies of the OLD castle and `M_CastleCrumble` samples the `T_Castle_*` set this task just overwrote — the 75/50/25 % damage states are scrambled until re-derived (manager ruling 4; CONVENTIONS CRUMBLE-DERIVATION LAW). TASK-331 runs in this same editor session, own commit.

---

## Discipline gates honored
- **Simulate STOPPED before every import/save** — `IsPIERunning` read back `false` before the texture and mesh imports (lane-knowledge 8). Verification ran in **Simulate**, never PIE-in-viewport.
- **Same-path overwrite ONLY** — `AssetImportTask(replace_existing=True, automated=True)` over the existing objects. Never delete+recreate. Referencers before == after == `["/Game/Maps/L_Arena"]` (object identity preserved).
- **`L_Arena` NEVER saved** — it is dirty in-editor (pre-existing residue) and was left dirty; only the SM_/T_ Castle assets were saved, each explicitly via `save_loaded_asset`.
- **Texture-skip trap honored** — the three textures were imported EXPLICITLY as their own step (the `reimport_meshes.py` `_ensure_textures_and_mi` early-return would have silently kept the old dark set). MI was NOT deleted/recreated.
- Castles located **by class** (`GameplayStatics.get_all_actors_of_class` on `/Script/GitClaudeUnrealTest.Castle`), never by label.

## What was imported (sources = TASK-329 outputs, already committed in `7bedf58`)

| Asset | Source | Result |
|---|---|---|
| `/Game/Textures/T_Castle_D` | `Content/RawAssets/Textures/Castle/T_Castle_D.png` | 2048², **sRGB ON**, `TC_DEFAULT`, saved; import-source readback = the new PNG |
| `/Game/Textures/T_Castle_N` | `…/T_Castle_N.png` | 2048², **LINEAR**, `TC_NORMALMAP`, saved |
| `/Game/Textures/T_Castle_ORM` | `…/T_Castle_ORM.png` | 2048², **LINEAR**, `TC_MASKS`, saved |
| `/Game/Meshes/SM_Castle` | `Content/RawAssets/Castle.fbx` (843,644 B) | same-path reimport; Nanite OFF; `lod_group=None` + explicit castle chain; 11 manifest box hulls; slots `[0 TeamRegion → MI_TeamColor_Blue, 1 CastlePBR → MI_Castle_PBR]`; saved |
| `/Game/Materials/Instances/MI_Castle_PBR` | — | **untouched by design**: it references the texture OBJECTS by path, which were overwritten in place. `git status` confirms zero diff — nothing to rewire, nothing committed |

Import ran INSIDE the live editor (remote-exec lane), so the slot→material assignment persisted directly — the headless-commandlet slot-reset limitation did not apply. No editor bounce needed.

## HARD LOD GATE (manager ruling 6 — the SM_MilitiaMob lesson) — **PASS**

Readback via MCP StaticMeshTools (independent of the import script):

| | Before (shipped) | After (rebuilt) | Expected (TASK-329) |
|---|---|---|---|
| `lod_count` | **1** | **3** | 3 |
| LOD0 tris / verts | 40,000 / 52,775 | **20,000 / 24,333** | ~19,995 / — |
| LOD1 tris / verts | — | **10,000 / 14,534** | ~10k |
| LOD2 tris / verts | — | **5,000 / 7,745** | ~5k |
| LOD screen thresholds | — | **[1.0, 0.4, 0.15]** | 1.0/0.4/0.15 |
| Bounds (X×Y×Z) | 813.13 × 819.13 × 897.21 | **814.52 × 820.56 × 894.87** | ±10 % of ~814×819×898 — deltas +0.17 %/+0.17 %/−0.26 % |
| min-Z (ground-centre) | 0.445 | **0.561** | 0.561 |
| Nanite | off | **off** | off |
| Collision | — | **11 box hulls, 0 convex** | manifest `ucx.boxes` (11) |
| `lod_group` | — | **"None"** (explicit chain, NOT LargeProp) | None |

- **No 0-triangle LOD.** LOD1/LOD2 = 10,000/5,000 tris.
- **No unweld signature:** LOD0 verts 24,333 ≠ tris×3 (60,000). (24,333 render verts vs the FBX's 9,792 welded modeling verts is normal UV/normal splitting, ratio 1.22 — nowhere near the 3.0 unweld fingerprint.)
- LOD0 readback is 20,000 vs the FBX's 19,995 (+5 tris, +0.025 % — import triangulation noise, not a defect).
- The MilitiaMob wedge did NOT reproduce on this asset.

## Simulate verify (a)–(e) — all PASS (observations, not conclusions)

Simulate started via MCP (`bSimulate=true`), warmup 8 s; stopped cleanly after verification.

- **(a) Colour:** both castles render warm cream/sandstone under the real arena sun, clearly BRIGHTER than the grass they stand on (the audit's "darker than the grass" is gone). After-shots re-captured in the audit's three-quarter framing (scatter trees now populate the arena, so the exact empty-field background of the before shot no longer exists):
  - `handoffs/TASK-330-verify-Castle-in-arena-after.png` (Blue, camera castle-relative (−2350, −2350, +560), yaw 45, pitch −7)
  - `handoffs/TASK-330-verify-CastleRed-in-arena-after.png` (Red, from the south-east — the south-west vantage at Red is occluded by scatter)
  - Before = `handoffs/TASK-309-audit-Castle-in-arena.png`.
- **(b) Team colour on BOTH:** Castle_0 (label `Castle_Blue`, Team BLUE) slot0 = `MI_TeamColor_Blue`; Castle_1 (label `Castle_Red`, Team RED) slot0 = `MI_TeamColor_Red` (ApplyTeamVisuals live); slot1 = `MI_Castle_PBR` on both. Blue roofs/spires on Blue, red on Red, visible in both captures. TeamRegion band lands on cones/roofs/wall-walk tops per the TASK-329 selector.
- **(c) Bounds/footprint/grounding:** asset bounds within 0.3 % of shipped (table above); actors at the anchors (−25000,0,0)/(+25000,0,0); mesh min-Z 0.561 uu at actor Z=0 — castles sit ON the ground, contact shadows correct in both captures. **Observation:** both castle ACTORS read yaw 0.0 (the board's "Castle_Red carries yaw 180" did not reproduce; located by class so nothing depended on it).
- **(d) Plinth dead-zone:** `CastlePlinthClearance` reads **420.0** on the live `SiegeBotController_0`. Behavioral sample from this Simulate: every bot/game placement sits OUTSIDE both 420-uu keep-out boxes — `BP_Building_DeepMine_C_0` at (24200, −800): |dx|=800 from Castle_Red; `BP_Unit_Ogre_C_0` at (−24775, 441): |dy|=441 > 420 from Castle_Blue. 0 violations. (The player-side `ASiegePlayerController` does not exist in Simulate — no local siege player — so its identical constant could not be read live; it is the same C++ default, untouched by this task.)
- **(e) Log clean:** `Ensure condition failed|Accessed None|Fatal error|LogOutputDevice: Error` over the whole session → **0 hits**. The only Castle-matching "Error" line is my own earlier failed inline-python attempt (tooling, pre-import, benign).

## Commit
- Files: `Content/Meshes/SM_Castle.uasset`, `Content/Textures/T_Castle_{D,N,ORM}.uasset`, this handoff, the 2 after-shots. Raw sources (`Castle.fbx`, 3 PNGs, `pipeline_manifest.json`) and the TASK-329 handoff were **already committed in `7bedf58`** ("mesh color fixes") — not duplicated here. `MI_Castle_PBR.uasset` is byte-unchanged (see above).
- `git diff --stat` clean of anything foreign. NO push. No `reset --hard`/`clean -fd` at any point.

## Notes / residue for the manager
1. **UCX west/back drift observed as shipped, not fixed** (CONVENTIONS UCX-DRIFT LAW: accepted, parked as TASK-336; integration only observes). Not re-measured in-engine this pass — the manifest boxes were applied verbatim, so the TASK-329 measurements stand.
2. The board's "Castle_Red carries yaw 180" claim did not reproduce (both actors yaw 0.0) — worth a one-line board correction if anyone relies on it.
3. Editor left running, L_Arena left dirty-unsaved, remote-exec flag still on (in-memory only).
