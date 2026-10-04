<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## Post-M7.6-merge main-lane follow-ups (decomposed 2026-07-25) — TASK-297 + the deck-builder resume

**Context:** M7.6 is MERGED to main LOCALLY (merge `4c680bb`, NOT pushed) and M7.5's retextured fleet is on main. Two follow-ups that were BLOCKED by that merge are now unblocked and develop on **main**: (1) the deferred unit SK-LODs (TASK-297, new), and (2) the parked deck-builder card-details chain (TASK-268..272, resumed on main — see the "## M7.7 tasks" section below, statuses refreshed 2026-07-25). **Both are EDITOR-MUTATING and need EXCLUSIVE editor access (no concurrent PIE) — they must NOT run while Jonathan is playtesting, and they SERIALIZE with each other (one editor session at a time). Dispatch order is the orchestrator's call; if they would fight the same editor session, run TASK-297 first, then the deck-builder chain (or vice-versa) — never concurrently.** No push in either lane (Jonathan's push).

#### TASK-297 — [P4-residual] Unit SK-LOD apply on MAIN: regenerate LOD chains + URO on the 11 fleet SK meshes (build, editor-python remote-exec lane)
- assignee: build-master
- status: **done (2026-07-25 — build-master).** All 11 fleet SK meshes LOD0-only→**3-LOD** via `SkeletalMeshEditorSubsystem.regenerate_lod` over the PythonScriptPlugin remote-exec lane (`bRemoteExecution` flipped via MCP per TASK-221 spike): LOD1 50%@0.4 / LOD2 20%@0.15 (`num_of_triangles_percentage` 0.5/0.2 + `SMOT_NUM_OF_TRIANGLES`; **NIT: the real UE 5.8 property is `num_of_triangles_percentage`, NOT the recipe's assumed `number_of_triangles_percentage`**; transient `SkeletalMeshLODSettings` baked in, then `lod_settings` restored to None ⇒ no companion asset). Readback: all 11 `lod_count=3` (was 1), per-LOD vert reduction confirmed (e.g. Footman 14142/9240/5283, Ogre 21899/12257/6638). **URO** (`OnlyTickPoseWhenRendered`+`bEnableUpdateRateOptimizations`) has NO USkeletalMesh-asset home (probed absent) — already set at RUNTIME by merged TASK-285 C++ (`SummonedUnit.cpp:155-156`), verified live in PIE. PIE on L_Arena: Traversability CONFIRMED 0 culls, bot spawned+marched 8 distinct fleet types incl. Ogre, Message Log clean (ensure/AccessedNone/Fatal/InverseFast/non-invertible/real-NaN = 0). Saved the 11 `SK_.uasset`; committed on main (11 SK_ + board + `handoffs/TASK-297.md`), NO push. ← was: backlog — **dispatchable when Jonathan frees the editor (editor-mutating; no concurrent PIE; serialize with the deck-builder chain).** Both TASK-289 deferral blockers are now CLEARED: (A) M7.5's retexture is MERGED onto main (`4c680bb`) so applying LODs on the 11 SK fleet meshes on main no longer collides with M7.5's active retexture territory; (B) the TASK-288 recipe tooling exists and is committed (`rig_character.py` emits `<CardID>.lod.json`; committed via TASK-289b `1df47b6`).
- blocked-by: none (TASK-289 deferral cleared — M7.5 retexture merged to main + TASK-288/289b recipe tooling committed). Serialize with TASK-269..272 (shared exclusive editor).
- parallel-safe: no (single editor + Git; needs EXCLUSIVE editor access, no concurrent PIE)
- spec: >
    On **main**, in the editor. Regenerate the LOD chains that TASK-289 DEFERRED for the 11 fleet skeletal meshes — all 11
    read LOD0-only per the TASK-289 audit: `SK_Footman`, `SK_Archer`, `SK_Knight`, `SK_Miner`, `SK_Cleric`, `SK_Ogre`,
    `SK_Sapper`, `SK_Pikeman`, `SK_Cavalry`, `SK_MilitiaMob`, `SK_Longbowman`. Apply the CONVENTIONS SK-unit LOD law EXACTLY:
    **LOD1 50% @ screen 0.4 / LOD2 20% @ 0.15**, plus set `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered` +
    `bEnableUpdateRateOptimizations = true` (the URO flags) on the SkeletalMesh's component setup. Use the TASK-288 recipe
    values (`<CardID>.lod.json`) as the numeric source of truth. **⚠ MECHANISM (the wrinkle — read before starting):**
    `regenerate_lod` is EDITOR-PYTHON and is NOT reachable via the standard MCP toolsets — `SkeletalMeshTools` has NO LOD-gen
    tool and the ProgrammaticToolset can't run the `unreal.` API. The known-working path is the **UE editor-python via the
    PythonScriptPlugin remote-exec lane** (enabled by the TASK-221 spike; the same raw-HTTP-MCP editor-python lane the recent
    TASK-292c/293/294 tasks drove). Apply through the `SkeletalMeshEditorSubsystem` LOD API (or the `regenerate_lod`
    editor-python helper referenced in CONVENTIONS) with the recipe values. **NIT carried from TASK-288 QA:** map the recipe's
    `percent_triangles` → the SK reduction property `number_of_triangles_percentage`. VERIFY, per mesh: (1) LOD chain present
    — readback LOD count > 1 on all 11; (2) URO flags set (`OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations=true`);
    (3) a PIE on `L_Arena` confirms units still ANIMATE + behave (full A→B march, hero + summoned units move/attack, Play
    Again ×1–3, Message Log clean). COMMIT on **main** with explicit pathspecs (the 11 `SK_` `.uasset` only — verify
    `git diff --stat` shows nothing else; if the deck-builder trio or any other asset appears, STOP and report), **NO push**.
    Post LOD-count before/after + the URO readback + the PIE result + hash in 🔧 Build & Git.
- names: >
    On **main**. Meshes: `SK_Footman`, `SK_Archer`, `SK_Knight`, `SK_Miner`, `SK_Cleric`, `SK_Ogre`, `SK_Sapper`,
    `SK_Pikeman`, `SK_Cavalry`, `SK_MilitiaMob`, `SK_Longbowman` (11 fleet SK meshes). Recipes: `<CardID>.lod.json`
    (TASK-288, `Tools/ArtPipeline/rig_character.py`). Lane: `SkeletalMeshEditorSubsystem` LOD API / `regenerate_lod`
    editor-python via the PythonScriptPlugin remote-exec lane (TASK-221 spike; TASK-292c/293/294 precedent). Law: CONVENTIONS
    "Arena 10× scale-up & LOD/perf (M7.6)" SK-unit LOD + URO law (LOD1 50%@0.4 / LOD2 20%@0.15).

