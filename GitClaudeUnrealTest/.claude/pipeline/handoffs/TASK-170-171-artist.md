# Handoff — TASK-170 (Wall) / TASK-171 (DeepMine) — art-director

**Date:** 2026-07-17
**Author:** art-director
**Goal:** Finish the LAST 2 TRELLIS.2 meshes — **DeepMine** and **Wall** — to complete the 16/16 textured-mesh roster (headless only; no editor).

## Outcome: EXPECTED QUOTA PAUSE — both blocked at Stage 1 (no failure)

Neither mesh could complete. Both hit the **TRELLIS.2 ZeroGPU PRO quota** at the `/image_to_3d`
GPU reservation (exit code 3). This is an expected pause per exit-code discipline, **not** a
pipeline or code failure. The `--check` (tokenless Space reachability + endpoint schema) PASSED
first, so the Space is healthy and there is no API drift — it is purely quota.

| Asset | Task | Stage 1 run (UTC) | Exit | Space message (verbatim) |
|-------|------|-------------------|------|--------------------------|
| Wall     | TASK-170 | 2026-07-17T07:16:54Z | 3 | "You have exceeded your Pro ZeroGPU quota (120s requested vs. 145s left). Try again in **3:24:44**." |
| DeepMine | TASK-171 | 2026-07-17T07:17:38Z | 3 | "You have exceeded your Pro ZeroGPU quota (120s requested vs. 142s left). Try again in **3:23:36**." |

**Shared-account-pool reset: ~10:41 UTC on 2026-07-17** (≈03:41 PDT). Both runs converge on the
same reset instant, confirming one shared ZeroGPU pool. DeepMine's earlier 06:39 UTC block gave the
identical ~10:41 UTC reset. NOTE: the "Xs requested vs. Ys left" wording reads as if headroom
remains, but the Space returns a hard `AppError` with a definitive reset time — surfaced verbatim,
not re-interpreted.

## State on disk (nothing damaged, resume-idempotent)

- `Tools/ArtPipeline/Cache/Wall/state_failed.json` — refreshed, status quota-blocked.
- `Tools/ArtPipeline/Cache/DeepMine/state_failed.json` — refreshed, status quota-blocked.
- Raw GLBs `Cache/<CardID>/trellis_raw.glb` — **absent** for both (Stage 1 never produced them).
- Concepts intact: `Inbox/Wall.png` (sha256 6d6a9a97…), `Inbox/DeepMine.png` (sha256 4ead7cbe…).
- **Blockout FBX UNTOUCHED** — `Content/RawAssets/Wall.fbx` (Jul 4) and `DeepMine.fbx` (Jul 5) are
  the ORIGINAL blockout meshes; Stage 2 never fired (no raw GLB to refine), so nothing was overwritten.
- No textures written; no `Content/RawAssets/Textures/{Wall,DeepMine}/` created; no editor touched;
  no Git touched.

## Resume procedure (after ~10:41 UTC 2026-07-17) — from `Tools/ArtPipeline/`

Both are `category: building`, `fit_mode: box`, `tri_budget 20000`, `bake_resolution 2048`,
ground-center, two-slot `[TeamRegion, <CardID>PBR]`. Manifest entries present and complete.

1. **Stage 1 (GENERATE):** `uv run trellis_generate.py Wall` and `uv run trellis_generate.py DeepMine`
   (BARE — *.hf.space is Norton-excluded). Idempotent: rerun overwrites the failed state and writes
   `Cache/<CardID>/trellis_raw.glb` on success. If quota is still short, exit 3 recurs — wait the next window.
2. **Pre-import gate:** read `Cache/<CardID>/refine_report.json` (tris ≤ 20000, bounds ≈ target dims
   — Wall 400×100×250, DeepMine 300×305×300 UE; UV layer `UVMap`; D/N/ORM PNG inventory) AND eyeball
   the Cache preview renders. Copy the accepted concept to `Content/RawAssets/Concepts/<CardID>.png`.
3. **Stage 2 (REFINE, HEADLESS):**
   `"<blender.exe>" --background --factory-startup --python-exit-code 1 --python refine_trellis_glb.py -- --card-id Wall`
   (and `--card-id DeepMine`) → `Content/RawAssets/<CardID>.fbx` + `Textures/<CardID>/*.png`. Verify
   2-slot + tri budget + box-fit from `refine_report.json`.
4. **Stage 3 (IMPORT, editor — separate step, NOT this task):** these are `category:building`, so they
   reimport via the proven **box-UCX branch** of the reimport commandlet (Tools/reimport_meshes.py),
   OVERWRITING `/Game/Meshes/SM_Wall` and `/Game/Meshes/SM_DeepMine` in place — that closes TASK-173
   from 6/8 to 8/8 (16/16 roster).

## Board / downstream

- TASK-170 status → `in-progress — quota-blocked` (3/4 done; Wall pending; reset ~10:41 UTC).
- TASK-171 status → `in-progress — quota-blocked` (3/4 done; DeepMine pending; reset ~10:41 UTC).
- TASK-173 (import) stays `in-progress (6/8)` — dated note added; unblocks to 8/8 once these two refine.
- No Git. No editor. HF_TOKEN never read/logged/passed on argv (lives only in env).

## Note on task numbering
The dispatch named "TASK-169/173". On the board, **TASK-169 is the already-done+integrated UNIT
wave** (Sapper/Cleric/Longbowman/Miner). Wall's Stage-1+2 tracker is **TASK-170** and DeepMine's is
**TASK-171**; the import is **TASK-173**. I edited 170/171/173 (the tasks that actually track these
two meshes) and left the completed 169 record intact. Flag for orchestrator reconciliation.

---

## ADDENDUM 2026-07-18 — Stage-1 rerun (M7.5 carry-in, decision 3): NEW BLOCKER — Space-side malfunction (NOT quota)

**Author:** art-director. **Scope run:** Stage 1 ONLY (Stage-2 refine gated on TASK-193 albedo de-light per the M7.5 board annotation; no scripts or manifest touched).

The ZeroGPU quota window was open (last block 07-17 07:18 UTC; these reruns 07-18 ~19:05 UTC). Both
assets FAILED with **exit 1** — a Space-side `AppError: RuntimeError` from the GPU endpoints. This is
a DIFFERENT failure mode from the 07-17 quota pause:

| Asset | Run (UTC) | Failing endpoint | Detail |
|---|---|---|---|
| Wall | 19:05:51–19:11:46 | `/extract_glb` (3/3 attempts) | Generation SUCCEEDED (`/image_to_3d` 229.6s) but extraction errored. Identical failure on an earlier 05:20–05:34 UTC run found on resume — persistent, not transient. |
| DeepMine | 19:12:08–19:21:28 | `/image_to_3d` (3/3 attempts) | Generation itself errored; never reached extract. |

**Diagnosis:** NOT quota (no exit-3 quota message anywhere), NOT API drift (`--check` PASSED
19:23 UTC — Space reachable, all three endpoints present, params unchanged), NOT our pipeline (zero
script changes; same params as the 14 successful meshes). The `microsoft/TRELLIS.2` Space's GPU
endpoints are currently throwing server-side RuntimeError. Note: ~2 × ~230 s of PRO GPU time was
consumed by Wall's successful-but-unextractable generations.

**State on disk:** `Cache/Wall/state_failed.json` + `Cache/DeepMine/state_failed.json` refreshed
(status `failed`); NO `trellis_raw.glb` for either; concepts intact (same sha256s as above); blockout
FBX untouched; no editor, no Git; HF_TOKEN env-only, never logged.

**Paths forward (orchestrator/Jonathan pick one):**
1. **Retry Stage 1 at a later window** — idempotent as before; the Space may be fixed upstream.
2. **README manual-browser fallback** — Jonathan generates in the Space UI and drops the GLB at
   `Tools/ArtPipeline/Cache/<CardID>/trellis_raw.glb` (~500k decimation / 2048 texture). Stage 2
   resumes from a hand-delivered GLB — and is gated on TASK-193 anyway, so no time is lost waiting.
3. **TASK-200 re-route** — Jonathan's A/B ruling may close these two via Meshy image-to-3D or the
   FAB buildings pack instead (recorded on the board carry-in).
