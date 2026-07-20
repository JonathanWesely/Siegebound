# Handoff — TASK-213 — SK fleet root-fix reimport: 8× same-path SK_<Unit> + Message-Log clean verify (art-director)

**Date:** 2026-07-18 · **Status:** done — all 8 units reimported at unchanged paths, missing-bones cause structurally removed, full acceptance met. Skeleton NEVER dirtied (see "bone count" below — the no-growth outcome is the success mode). Jonathan's session untouched beyond the coordinated wave.

## How the wave actually ran (lane deviation, recorded for the pipeline)

- `SkeletalMeshTools.import_file` REFUSES same-path overwrite ("SK_Pikeman at /Game/Characters already exists") — same non-mutating refusal as the StaticMesh precedent (TASK-031/085/086). The MCP server exposes NO reimport tool and no console exec; the commandlet lane needs the editor closed (forbidden — Jonathan ACTIVE).
- Resolution: I pre-selected all 8 SK assets in the Content Browser via MCP (`SetContentBrowserPath` + `SelectAssets`), and **Jonathan performed ONE right-click → Reimport on the selection** (coordinated via the 🎨 Art thread; he confirmed in Claude Code). Reimport re-read each asset's stored source (`../RawAssets/Characters/<Unit>.fbx` — the TASK-212-fixed files; stored MD5s were pre-fix, so all 8 re-read new bytes). Everything downstream (verification, saves, scans) was machine-side MCP/shell.
- **Standing debt unchanged:** same-path SK/SM reimport still has no fully-autonomous in-editor lane; it costs one human Reimport click (bulk selection makes it one click per WAVE, not per asset) or an editor-bounce commandlet window. The console-exec MCP tool remains the M7 ask that would close this.
- ProgrammaticToolset law learned: a failing `execute_tool` call aborts the whole script UNCATCHABLY (try/except never sees it) — failure-probes must be single direct `call_tool` invocations.

## Per-unit results (all 8 PASS)

| SK_<Unit> | Reimported | Slots post | Materials post (unchanged from pre) | Verts LOD0 pre→post | Skeleton binding | uasset name-table post-save |
|---|---|---|---|---|---|---|
| SK_Pikeman | yes | [TeamRegion, PikemanPBR] | MI_TeamColor_Blue + MI_Pikeman_PBR | 17789 = | SK_Footman_Skeleton | Pikeman_Rig 0 hits, Footman_Rig 3 |
| SK_Cleric | yes | [TeamRegion, ClericPBR] | MI_TeamColor_Blue + MI_Cleric_PBR | 17008 = | SK_Footman_Skeleton | Cleric_Rig 0, Footman_Rig 3 |
| SK_Longbowman | yes | [TeamRegion, LongbowmanPBR] | MI_TeamColor_Blue + MI_Longbowman_PBR | 17557 = | SK_Footman_Skeleton | Longbowman_Rig 0, Footman_Rig 3 |
| SK_MilitiaMob | yes | [TeamRegion, MilitiaMobPBR] | MI_TeamColor_Blue + MI_MilitiaMob_PBR | 16978 = | SK_Footman_Skeleton | MilitiaMob_Rig 0, Footman_Rig 3 |
| SK_Miner | yes | [TeamRegion, MinerPBR] | MI_TeamColor_Blue + MI_Miner_PBR | 19382 = | SK_Footman_Skeleton | Miner_Rig 0, Footman_Rig 3 |
| SK_Sapper | yes | [TeamRegion, SapperPBR] | MI_TeamColor_Blue + MI_Sapper_PBR | 19468 = | SK_Footman_Skeleton | Sapper_Rig 0, Footman_Rig 3 |
| SK_Cavalry | yes | [TeamRegion, CavalryPBR] | MI_TeamColor_Blue + MI_Cavalry_PBR | 17496 = | SK_Footman_Skeleton | Cavalry_Rig 0, Footman_Rig 3 |
| SK_Knight | yes | [TeamRegion, KnightPBR] | MI_TeamColor_Blue + MI_Knight_PBR | 18456 = | SK_Footman_Skeleton | Knight_Rig 0, Footman_Rig 3 |

- Material slots were NOT reset by the reimport (the M7 slot-reset precedent did not recur — no set_material fixes needed). Sections LOD0 = 2 everywhere. No unit has a physics asset (pre AND post — pre-existing state, not a regression).
- Pre-wave disk truth (my scan, matching TASK-211): every SK uasset had `<Unit>_Rig` (2 hits) and ZERO `Footman_Rig`. Post-save: `<Unit>_Rig` purged from all 8, `Footman_Rig` present ×3. The mesh roots now match the skeleton root — `MergeAllBonesToBoneTree` can never fail on these again.
- API caveat for future tasks: `SkeletalMeshTools.get_bone_names`/`get_bone_parent` mirror the bound SKELETON's hierarchy, NOT the mesh's own RefSkeleton (probe: `Bone "Cavalry_Rig" not found on SK_Cavalry` while the uasset name table contained it). Mesh-root verification must be the on-disk name-table scan.

## Skeleton bone count before/after: 22 → 22, by design

TASK-211 observed the skeleton stuck at 22 (`Footman_Rig → root → pelvis → …`) with merges FAILING. The spec's "BoneTree should dirty/grow" expectation does not apply to the fix lane that shipped: TASK-212 renamed every unit's root to the skeleton's EXISTING root, so each reimported mesh's 22 bones are name-identical to the skeleton's 22 — the merge succeeds trivially with nothing to add. SK_Footman_Skeleton never dirtied and was therefore NOT saved (only-what-dirties law; file mtime still 2026-07-16 02:59). No-growth + no-dirty + no-warning = the correct success signature.

## Saved assets (exactly 8 — nothing else)

`/Game/Characters/SK_{Pikeman,Cleric,Longbowman,MilitiaMob,Miner,Sapper,Cavalry,Knight}` via explicit-path `save_assets` (no save-all at any point). Git status shows exactly these 8 modified uassets. NOT saved/dirtied: SK_Footman_Skeleton, SK_Footman, ABP_Footman, all live A_ clips, AB_Test scratch, L_Arena — final `is_dirty` sweep all false. Pre-existing untracked scratch (AB_Test/, IK_*, RTG_*) untouched.

## Post-verify (all PASS)

- **ABP_Footman:** TargetSkeleton = `/Game/Characters/SK_Footman_Skeleton` (registry tag, pre AND post), generated class `ABP_Footman_C` alive, ZERO blueprint/anim-compile errors in the full session log, not dirty. No a7a77f6 rebind needed — the trap (skeleton modified then discarded) never armed because the skeleton was never modified.
- **Animate spot-check (QA's end-gate for the TASK-212 vertex/weight blind spot):** `A_Knight_Walk` renders a correctly mid-stride, skinned, textured character; `A_Cavalry_Attack` renders the raised-arm attack pose — no T-pose, no candy-wrapping, no weight explosion (captures: scratchpad `A_Knight_Walk.png`, `A_Cavalry_Attack.png`).
- **Textured-render check:** SK_Knight (sword/shield, blue TeamRegion crest) + SK_Cavalry (mounted rider, team accents) thumbnails render fully textured (`SK_Knight_post.png`, `SK_Cavalry_post.png`).
- **Message Log / load-all-8:** explicit `load_asset` of all 8 post-reimport fired ZERO new missing-bones entries; the whole-session log contains only the 2 pre-wave baseline lines (20:38:40 Cavalry / 20:38:43 Knight, early-session frames [251]/[388] — Jonathan's earlier load, documented in TASK-211). Those two lines remain visible in THIS session's Message Log as historical residue and clear on the next editor launch; the cause is structurally gone (roots match; nothing to merge). **Jonathan's next fresh session is the cosmetic final confirmation — expect zero LoadErrors.**

## For build-master (rides TASK-210)

- Commit scope from this task: the 8 `Content/Characters/SK_<Unit>.uasset` (paired with TASK-212's 8 rewritten `Content/RawAssets/Characters/*.fbx` + `Tools/ArtPipeline/rig_character.py` + `Tools/ArtPipeline/fix_rig_root.py`). Skeleton/ABP/anims deliberately untouched — do not expect them in the diff.
- Rollback (unused): sha-verified original FBXs at `Tools/ArtPipeline/Cache/RigRootFix/backup/` + `git checkout --` the 8 uassets (mind the editor's streaming locks — restore in an editor-bounce window).
- TASK-205 fleet anim waves are now unblocked on a clean-rooted fleet, per the TASK-211 sequencing ruling.
