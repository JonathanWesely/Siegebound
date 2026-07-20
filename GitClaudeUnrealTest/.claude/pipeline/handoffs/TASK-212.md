# Handoff — TASK-212 — Rig-FBX armature-root fix: rig_character.py hardening + batch rename (gameplay-programmer)

**Date:** 2026-07-18 · **Status:** ready-for-qa · All 8 FBXs rewritten in place, machine-verified; rig_character.py future exports emit the constant root. No editor, no SK_ uassets, no Content/*.uasset, no Git, no meshy_generate.py.

## Root cause recap (handoffs/TASK-211.md — do not re-derive)

Each unit rig FBX exported its Blender armature OBJECT as `<Unit>_Rig`; UE's FBX importer converts that node into an extra ROOT bone, so every SK_<Unit> roots at `<Unit>_Rig` != the shared skeleton's root `Footman_Rig` → `MergeAllBonesToBoneTree` silently fails on every load (the recurring missing-bones Message Log warnings).

## Deliverable 1 — `Tools/ArtPipeline/rig_character.py` hardening

- New module constant `SHARED_SKELETON_ROOT = "Footman_Rig"` (with the TASK-211/212 root-cause comment) next to the axis constants.
- `build_armature()` now creates the armature OBJECT as `bpy.data.objects.new(SHARED_SKELETON_ROOT, arm_data)` — constant for EVERY unit, was `f"{card_id}_Rig"`. That object name is the only line that changed behavior.
- Armature DATA block stays per-unit `f"{card_id}_Armature"` (never exported as a node UE reads).
- Header docstring step 3 documents the law. Everything else in the rig contract untouched: 21-bone SiegeBiped hierarchy (`root → pelvis → …`), two-slot `[TeamRegion, <CardID>PBR]` materials, `UVMap` layer, per-anim exports, axis contract, OutputGuard.

## Deliverable 2 — `Tools/ArtPipeline/fix_rig_root.py` (NEW) + the 8 in-place rewrites

Headless Blender 5.1.2 (`--background --factory-startup --python-exit-code 1`), bpy + stdlib only. Per unit: sha256 + backup → factory-empty scene → import → full snapshot (every bone's name/parent/deform/connect/head/tail + mesh verts/polys/tris/slots/UVs/vgroups/world matrices) → rename armature OBJECT → export to a temp path with the EXACT `export_skeletal_fbx` exporter block (axis_forward=-Z, axis_up=Y, apply_unit_scale, FBX_SCALE_NONE, FACE smoothing, no leaf bones, bake_anim=False, bone axes Y/X, path_mode=STRIP) → re-import temp + assert snapshot identity + byte scan → only then `os.replace` onto the Content path → final in-place byte scan. A failing unit's original is never overwritten. Fix mode is idempotent (rerun proved: all `ALREADY_OK`, zero writes).

Also renamed per file: the armature DATA block `<Unit>_Rig` → `<Unit>_Armature` (the importer had stamped the node name onto the data block; renaming purges the last stale `<Unit>_Rig` string from the file — it is not a node UE reads, and it matches rig_character.py's data naming).

## Per-FBX verification table (all under `Content\RawAssets\Characters\`)

| FBX | Armature object | Bones | Root bone | Mesh (verts/tris) | Slots | sha256 orig → new (size B) | Verdict |
|---|---|---|---|---|---|---|---|
| Footman.fbx | `Footman_Rig` (pre-existing) | 21 | root | untouched | untouched | 272156d5484e (800844) — UNTOUCHED | PASS (read-only check) |
| Pikeman.fbx | `Pikeman_Rig` → `Footman_Rig` | 21=21 | root | SK_Pikeman 7242/14996 = | [TeamRegion, PikemanPBR] = | 0dfc638ce8d9 (949756) → 3b219279e5b4 (950444) | PASS |
| Cleric.fbx | `Cleric_Rig` → `Footman_Rig` | 21=21 | root | SK_Cleric 7120/14998 = | [TeamRegion, ClericPBR] = | 370f69940b89 (864428) → 586f9e2a3d15 (865516) | PASS |
| Longbowman.fbx | `Longbowman_Rig` → `Footman_Rig` | 21=21 | root | SK_Longbowman 7362/14996 = | [TeamRegion, LongbowmanPBR] = | a5924e0b5340 (1048380) → 1b22f0a4968d (1048940) | PASS |
| MilitiaMob.fbx | `MilitiaMob_Rig` → `Footman_Rig` | 21=21 | root | SK_MilitiaMob 7184/14998 = | [TeamRegion, MilitiaMobPBR] = | b895b8ec7106 (862844) → 246a18cb48d7 (863484) | PASS |
| Miner.fbx | `Miner_Rig` → `Footman_Rig` | 21=21 | root | SK_Miner 7020/14993 = | [TeamRegion, MinerPBR] = | 0473eb86c85d (882620) → 0df3134a5939 (882764) | PASS |
| Sapper.fbx | `Sapper_Rig` → `Footman_Rig` | 21=21 | root | SK_Sapper 7138/14994 = | [TeamRegion, SapperPBR] = | 4feb035d2a09 (928188) → a715c82405c9 (928796) | PASS |
| Cavalry.fbx | `Cavalry_Rig` → `Footman_Rig` | 21=21 | root | SK_Cavalry 7410/14996 = | [TeamRegion, CavalryPBR] = | ba03cdfb2663 (950268) → 9544607454cc (950876) | PASS |
| Knight.fbx | `Knight_Rig` → `Footman_Rig` | 21=21 | root | SK_Knight 7256/14995 = | [TeamRegion, KnightPBR] = | ff6268fa1028 (875164) → 94209ae92d38 (875484) | PASS |

"=" means pre/post identical on re-import. Every file also passed the TASK-211-style byte scan: `Footman_Rig` present, `<Unit>_Rig` absent. Bone rest poses round-tripped IDENTICALLY to 6 decimal places; armature + mesh world matrices identity pre and post. UV layer `UVMap` and vertex-group sets identical everywhere.

## Backups + machine evidence (Cache/ is gitignored — backups never ride a commit)

- Backups (byte-identical originals, sha-verified at copy time): `Tools\ArtPipeline\Cache\RigRootFix\backup\{Pikeman,Cleric,Longbowman,MilitiaMob,Miner,Sapper,Cavalry,Knight}.fbx`
- Fix-mode report (current state, post-fix rerun = ALREADY_OK across the board): `Tools\ArtPipeline\Cache\RigRootFix\fix_report.json`
- Read-only verification report (all 9 PASS on the live files): `Tools\ArtPipeline\Cache\RigRootFix\verify_report.json`
- Rollback path if TASK-213 ever needs it: copy the backup over the Content path (or re-run rig_character.py per unit — it now emits the constant root anyway).

## Flagged decision QA should scrutinize

**`use_connect` inference flips (non-fatal by design):** on re-import verification, 4 bones per unit (`pelvis`, `spine_02`, `spine_03`, `head`) report `use_connect` True → False. This flag is NOT serialized in FBX — it is Blender's import-side inference (child head exactly coinciding with parent tail), which trips on sub-1e-6 float drift. The head/tail transforms themselves are compared FATALLY and round-tripped identically (6 dp); byte-marker scans of original vs re-export show identical structure counts. UE builds its ref skeleton from bone node transforms and never sees a connect flag, so this cannot affect TASK-213's reimport, skinning, or anims. `compare_snapshots()` documents this and records the flips as `notes` in the report rows.

## For TASK-213 (art-director, editor reimport)

- Same-path reimport the 8 `SK_<Unit>` from these FBXs against `/Game/Characters/SK_Footman_Skeleton`; mesh roots now match the skeleton root, so no bone merge is needed and the missing-bones warning cannot re-fire.
- Anim FBXs (`Characters/Anims/<Unit>_*.fbx`) were NOT touched — the `<Unit>_Rig` wrapper never animated (identity transform, per TASK-211), and by-name retargeting drives the 21 real bones. If any anim reimport ever complains about the root, the same script pattern extends to Anims/, but nothing today requires it.
- Rig lane law going forward: any future `rig_character.py` export (new units, DeepMine retry, etc.) emits `Footman_Rig` automatically.

## Files touched

- `Tools/ArtPipeline/rig_character.py` (hardened — constant armature root)
- `Tools/ArtPipeline/fix_rig_root.py` (new — batch fix + verify tool)
- `Content/RawAssets/Characters/{Pikeman,Cleric,Longbowman,MilitiaMob,Miner,Sapper,Cavalry,Knight}.fbx` (in-place rewrite, armature root only)
- NOT touched: Footman.fbx, all Anims/*.fbx, all Content/**/*.uasset, editor, Git.
