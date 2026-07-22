# QA Report — TASK-212 (Rig-FBX armature-root fix: rig_character.py hardening + fix_rig_root.py batch rename)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-18. Scope per dispatch: (1) `Tools/ArtPipeline/rig_character.py`
hardening, (2) new `Tools/ArtPipeline/fix_rig_root.py`, (3) adjudicate the flagged `use_connect`
finding (load-bearing for TASK-213), (4) 8-unit list completeness. Reviewed against the board spec
(M7.5 → TASK-212), handoffs/TASK-211.md + TASK-212.md, CONVENTIONS "Skeletal rig & animation
workstream (M7)", and the LIVE machine evidence (`Cache/RigRootFix/fix_report.json` +
`verify_report.json`). Board status NOT edited per dispatch (orchestrator flips on this verdict).

Blockers: 0 · Warnings: 2 (both doc/robustness — non-blocking) · Nits: 3 · 1 observation

---

## Scope 1 — rig_character.py hardening: VERIFIED

- `SHARED_SKELETON_ROOT = "Footman_Rig"` (line 100) beside the axis constants, with the full
  TASK-211/212 root-cause comment. `build_armature()` line 350:
  `bpy.data.objects.new(SHARED_SKELETON_ROOT, arm_data)` — constant for every unit. Header
  docstring step 3 documents the law (lines 29–33).
- **No other drift:** a `_Rig` grep across the file hits only the constant + comments — there are
  ZERO name-based object lookups (export selects by object reference,
  `_select_for_export(mesh, arm_obj)`), so the rename cannot dangle anything. The 21-bone SiegeBiped
  build is intact (7 axial + 2×7 limb = 21; `root → pelvis → …`; deform flags; roll 0), two-slot
  material preservation, `UVMap` verbatim, per-anim export block, axis contract
  (`axis_forward='-Z'`, `axis_up='Y'`, FBX_SCALE_NONE, FACE, no leaf bones, bone axes Y/X,
  path_mode STRIP), and the OutputGuard (Cache/<CardID>/rig + Characters/ + CardArt lane block) are
  all unchanged.
- **Armature DATA naming (`<CardID>_Armature`, line 348) inert for UE — CONFIRMED, and in fact
  EMPIRICALLY:** Blender's FBX exporter does not serialize the armature data-block name at all —
  the live reports show every shipped FBX re-imports with `armature_data == "Footman_Rig"` (the
  importer stamps the NODE name onto both object and data). The only skeletal-relevant string in
  the file is the armature OBJECT node — exactly what UE converts to the root bone. The data name
  cannot reach UE because it is not in the file.
- [NIT] `report["armature"]["name"]` in rig_report.json now records `Footman_Rig` for every unit
  (was `<Unit>_Rig`); grep across Tools/ shows no script consumes it — informational only.

## Scope 2 — fix_rig_root.py: VERIFIED

- **bpy + stdlib ONLY:** imports are argparse/hashlib/json/os/shutil/sys/time/traceback/
  datetime/pathlib + bpy (lines 58–74). No numpy, no third-party, no `requests` — no network at
  all. Headless-safe: `read_factory_settings(use_empty=True)` fresh scene per import, object-mode
  guard, no UI-context ops.
- **Ordering is genuinely safe — the original is never at risk before verification** (lines
  343–396): backup copy2 → backup sha256 verified against source sha (mismatch aborts BEFORE any
  further write) → in-memory rename → export to `Cache/RigRootFix/out/<Unit>.fbx` (temp, guarded)
  → fresh scene → re-import TEMP → contract checks + full snapshot compare + byte scan on the temp
  file → ONLY on zero problems `os.replace(tmp, dst)` (same volume — atomic) → final in-place byte
  scan. A failing unit returns FAIL with the Content file untouched (line 380–384). The
  post-replace scan is defense-in-depth (temp already passed the identical scan) with the
  sha-verified backup as the rollback.
- **Idempotency sound:** `already_ok` short-circuit (armature object == `Footman_Rig`) → byte-scan
  + `ALREADY_OK`, zero writes, unless `--force`; unexpected armature names are refused outright
  (lines 329–334). LIVE PROOF: the current `fix_report.json` is the rerun — ALREADY_OK across the
  board, and `verify_report.json` shows all 9 units PASS with empty problems on the in-place files.
- **Exporter block diffed param-for-param against `rig_character.export_skeletal_fbx`
  (fix lines 273–283 vs rig lines 745–755): IDENTICAL** — use_selection, object_types
  {ARMATURE, MESH}, apply_unit_scale, FBX_SCALE_NONE, -Z/Y axes, FACE smoothing,
  use_mesh_modifiers=False, add_leaf_bones=False, bake_anim=False, use_armature_deform_only=False,
  bone axes Y/X, path_mode STRIP. See the observation below re the one intentional non-param
  difference (reset_pose).
- **Backup integrity:** sha-verified at copy time (lines 347–349); `Tools/ArtPipeline/Cache/*` is
  confirmed gitignored (.gitignore:27) → backups/reports never ride a commit, matching the handoff.
- **Confinement:** `guard_write` allows only `Cache/RigRootFix/**` + `Content/RawAssets/
  Characters/**`, CardArt lane blocked; every write path (backup, temp, report, dst) is guarded.
- **Exit-code discipline:** 0 all-pass / 1 any-FAIL (fail() after verdicts) / 2 usage-input
  (missing FBX, Footman in `--units`); ALREADY_OK correctly counts as success. `--verify-only`
  writes nothing but its Cache report. Footman is structurally unwritable: rejected from `--units`
  (exit 2) and `check_footman()` is read-only (its sha in the handoff table is unchanged).

## Scope 3 — `use_connect` adjudication: CLAIM CONFIRMED (safe for TASK-213)

The handoff's reasoning is sound on all three legs:

1. **Not serialized in FBX — TRUE.** FBX bones are LimbNode Model nodes carrying transforms; the
   format has no bone-connect attribute. Blender's `use_connect` is an EDIT-time constraint that
   the FBX importer *infers geometrically* (child head coinciding with parent tail within a tight
   epsilon). It is also not consulted by the exporter for transforms — it cannot round-trip
   because it never enters the file.
2. **The flip pattern matches the inference-noise explanation.** The 4 flipped bones
   (pelvis/spine_02/spine_03/head) sit exactly at chain joints `build_armature` authors with
   coincident parent-tail/child-head coordinates — the knife edge where a sub-1e-6 float drift
   flips an exact-coincidence test while the head/tail values round-trip identically at the
   recorded 6 dp. The transforms are compared FATALLY (`compare_snapshots` lines 242–244,
   POS_TOL 1e-4 m) — a connect flip with moved geometry would FAIL the unit. A True→False flip
   with identical transforms changes zero geometry (the importer only sets True when head already
   equals parent tail).
3. **UE never sees it.** UE's FBX skeletal import builds the reference skeleton from the limb-node
   hierarchy + node transforms (+ bind-pose clusters); UE bone data has no connect concept.
   Therefore the flips cannot affect TASK-213's 8× same-path reimport, skinning, or anims. The
   `notes`-not-`problems` classification (lines 238–241) is the correct severity.

## Scope 4 — 8-unit list completeness: CONFIRMED against disk truth

- `Content/RawAssets/Characters/` contains EXACTLY 9 FBXs: Footman +
  {Pikeman, Cleric, Longbowman, MilitiaMob, Miner, Sapper, Cavalry, Knight} = `DEFAULT_UNITS`
  exactly. Nothing else exists to fix.
- `Content/Characters/` SK set: SK_Footman (+ _Skeleton + _PhysicsAsset) + the same 8 SK_<Unit>.
  **No SK_Archer and no Archer.fbx — Archer was never rigged, so its absence from the list is
  CORRECT** (nothing to re-root, nothing TASK-213 will reimport). No Ogre rig assets exist either
  (no own-skeleton unit is exposed to this tool today).
- Footman: read-only check PASS in both live reports; file untouched (handoff sha row consistent).
- Handoff per-FBX table cross-checks against the live `verify_report.json`: all 9 verdicts PASS,
  `sha256_before` values in verify match the handoff's post-fix shas, armature object
  `Footman_Rig` everywhere, 21 bones, root bone `root`.

## Findings

- [WARN] fix_rig_root.py:175–207 — snapshot verification blind spots: vertex POSITIONS,
  skin-weight VALUES (only vertex-group names), and bone ROLL are not compared across the round
  trip (head/tail do not encode roll). Mitigated by: the pass is a pure import→rename→export with
  no geometry-touching operator, the exporter mirror is exact, rest transforms + world matrices are
  compared fatally, and TASK-213's editor-side gate (same-path reimport + ABP compile + roster
  animate spot-check) is the true end-verification. Non-blocking; TASK-213 must keep its animate
  spot-check (already in its spec).
- [WARN] handoffs/TASK-212.md Deliverable-2 — data-block claim imprecise: the `<Unit>_Rig` →
  `<Unit>_Armature` data rename does NOT survive into the shipped FBX. Blender's exporter never
  serializes the armature data-block name; both live reports show every file re-imports with
  `armature_data == "Footman_Rig"` (importer stamps the node name on both object and data). The
  PURGE goal is still fully achieved — byte scans confirm `b'<Unit>_Rig'` absent from every file —
  and this actually strengthens the inertness claim (the string is not in the file at all). Doc
  accuracy only: future Blender inspections should expect data name `Footman_Rig`, NOT
  `<Unit>_Armature`.
- [NIT] fix/verify reports are overwritten per run — the FIRST fix run's evidence
  (`renamed_data_block` rows, `use_connect` notes) was replaced by the idempotency rerun's
  ALREADY_OK rows; only the handoff table preserves it. `verify_report.json` (all 9 PASS on the
  live files) is the current-state evidence and suffices for TASK-213. Suggest run-stamped report
  filenames if the tool is reused.
- [NIT] fix_rig_root.py:120 — no allowlist on `--units`: any `<Unit>.fbx` under Characters/ whose
  armature is `<Unit>_Rig` would be re-rooted. Safe today (exactly the 9 known files exist;
  unknown units exit 2 on missing FBX), but a FUTURE own-skeleton unit (an Ogre-class rig with its
  own `SK_<Unit>_Skeleton`) dropped into Characters/ would be one `--units` typo away from
  mis-rooting. A one-line allowlist assert or a docstring warning would harden.
- [NIT] fix_rig_root.py:355 — in `--force` mode on an already-fixed file, `renamed_object` records
  `"<Unit>_Rig -> Footman_Rig"` even though the name was already `Footman_Rig`. Cosmetic report
  string.
- [Observation, no action] The mirror exporter intentionally omits `reset_pose()` (rig_character
  calls it before export). Harmless here: a bind-pose FBX with `bake_anim=False` imports with
  pose == rest by construction, and any deviation would surface in the fatal rest-transform
  compare. Recording so a future copy of this block into an ANIM-carrying context re-adds it.

## Notes for build-master / orchestrator

- TASK-213 is CLEAR from QA's side: the 8 FBXs on disk are machine-verified (`Footman_Rig` root,
  21-bone `root → pelvis → …`, two-slot slots, UVMap, world-identity matrices) and the
  `use_connect` finding is confirmed cosmetic. Rollback path: sha-verified originals at
  `Tools/ArtPipeline/Cache/RigRootFix/backup/` (gitignored — copy back over the Content path).
- Commit scope when it rides its flow: `rig_character.py` + `fix_rig_root.py` + the 8 rewritten
  `Content/RawAssets/Characters/*.fbx` (raw-asset rule: FBX is checked in). Backups/reports stay
  out via the Cache gitignore. Anims/*.fbx and all .uasset untouched, as the handoff states.
