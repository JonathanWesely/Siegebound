# QA Report — TASK-229 (Blender-side retarget tool: retarget_meshy_to_siegebiped.py)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-19. File: `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py` (new).
Reviewed against the board spec (anim-export resolution lane → TASK-229), CONVENTIONS "UE-5.8
retarget-export defect + SANCTIONED export path" (≥40 uu floor codified), handoffs/TASK-229.md,
handoffs/TASK-221-222.md §1/§2/§7 (defect evidence + sampling-method baseline), the
`rig_character.py` SiegeBiped authority (bone table + `export_anim_fbx`), AND the live machine
evidence (`Cache/Footman/retarget/retarget_report.json` + the 4 artifact FBXs on disk).
Board status NOT edited per dispatch.

Blockers: 0 · Warnings: 1 (non-blocking, adjudicated) · Nits: 4

---

## 1. Retarget correctness — VERIFIED (symbolic trace of the full transfer chain)

### Helper-bone construction + COPY_ROTATION semantics
Worked the math end-to-end: helper rest world orientation = `A @ tgt_rest` (:387-394), helper
rigidly parented to the source bone ⇒ helper world rotation at time t =
`src_delta(t) @ A @ tgt_rest`. COPY_ROTATION (WORLD→WORLD, REPLACE, influence 1 — :417-424)
replaces the target bone's world rotation with exactly that. Net:
- **`aim` (19 anatomical bones):** A = `tgt_rest_dir.rotation_difference(src_rest_dir)`
  (:389-390) ⇒ at source rest the target aligns to the source's rest direction, and thereafter the
  target bone TRACKS the source bone's absolute world direction — per-unit proportion differences
  drop out. Exactly the documented semantics; the "A is small" premise (both rigs fitted to the
  same mesh, quirk 5's 0.9255/0.9175 m rest heights) holds, so minimal-rotation twist transport is
  safe and the antiparallel-ambiguity edge of `rotation_difference` is unreachable.
- **`delta` (Hips→pelvis):** A = identity (:392) ⇒ pelvis keeps its own rest orientation and
  receives the source's world-space rotation delta — correct for a root bone whose geometric
  direction is arbitrary (probe: sideways). Matches the doc.
- Helper authored in ARMATURE space via `src_obj_rot⁻¹ @ helper_world_q` (:394, :408) — correct
  object-space conversion; the source's 0.01 uniform object scale (quirk 5) cannot perturb a
  rotation decompose. Helpers are non-deform, on the SOURCE armature only — they cannot leak into
  the ARMATURE-only export of the TARGET (artifact bone count 21 confirms).

### 9-chain map incl. the spine reversal
`CHAIN_MAP` (:134-156) encodes IK_MeshyBiped's 9 chains + RetargetRoot exactly as documented:
Spine02→spine_01 / Spine01→spine_02 / Spine→spine_03 (Meshy Spine02 = lowest — the TASK-203
quirk, TASK-223-confirmed fleet-wide), L→L / R→R throughout, unmapped source
(ToeBase×2/head_end/headfront) and unmapped target (`root`) match the handoff table. Verified
against the rig_character.py 21-bone authority: all 20 mapped targets + `root` = 21, names exact.

### Root/pelvis translation split
`write_bake` (:469-536): Hips world offset from rest × hip-ratio (target pelvis rest-world Z /
source Hips rest-world Z — 1.0087 for Footman, guarded against ~0, :481-487); split
horizontal→`root` / vertical→`pelvis` in WORLD space, each converted to armature space via the
rotation-only inverse (:502-503) — "horizontal" stays a world concept, correct. Root's pose basis
= `rest⁻¹ @ Translation @ rest` with identity rotation (:509-510) — root motion PRESERVED on the
root bone, rotation held at rest; pelvis gets the vertical bob added to its FK position (:520-521)
so walk bob + death collapse survive a UE root-lock. Matches the documented design and the
recorded Attack/Death root travel (~132/77 uu ≈ the known ~1.25 m).

### Bake reconstruction + depsgraph soundness
- **Order verified:** capture (:441-457, evaluated depsgraph, constraints ACTIVE, world matrices
  via `obj.matrix_world @ pose.bone.matrix`) happens BEFORE `strip_constraints` (:489) — no
  constraint survives into the keyframes or the export.
- **FK write-back is self-consistent:** the parent-first walk uses the standard Blender pose
  accumulation identity (`k = parent_pose @ parent_rest⁻¹ @ child_rest`, basis = `k⁻¹ @ m`,
  :505-533); because each mapped bone's captured WORLD rotation is written absolutely, every
  bone's final rotation is independent of its parent's new orientation — wrong-space accumulation
  errors are structurally excluded. Non-root/pelvis bones key rotation only; their basis
  translation is exactly zero by construction (pos taken from k). `clear_pose_transforms` before
  the bake (:495) zeroes residual locations; scale is never keyed (`LocRotScale(..., None)`) —
  **"no skew" is a structural guarantee, not an observation**.
- **Hemisphere continuity:** per-bone `prev_q.dot(q) < 0 → negate` (:525-527) on normalized quats
  — sound (affects interpolation only; pose per key is identical either hemisphere).

### The "amplitude proves existence, not correctness" adjudication
Correct observation, and the tool does NOT carry an automated mirror/axis check — see WARN-1.
Adjudicated ACCEPTABLE for this gate because (a) the gate's design target is the specific UE-5.8
root-only defect class (~7 uu), unreachable at a 40-floor; (b) semantic/visual correctness is
EXPLICITLY assigned to TASK-230's Jonathan eyeball gate (before/after captures, PIE) by the board
spec and lane ruling; (c) the recorded numbers carry soft corroboration a gross mirror/axis error
would break: the source's L/R foot asymmetry (56.97 > 50.34) is preserved in order through baked
and artifact (61.59 > 52.79), all five sampled bones are differentiated and plausible per clip,
and clip lengths match the UE evidence to the millisecond.

## 2. The gate — VERIFIED against the live report JSON

- **Method identity:** `sample_amplitudes` (:540-564) = 9 uniform samples, world bone position,
  per-axis max−min, max over axes, ×100 uu — the TASK-203/221 spike method verbatim (§2 of the
  evidence handoff); fractional-time `frame_set(int, subframe)` mirrors time-based UE sampling.
- **Cross-validation CONFIRMED from `retarget_report.json`:** Walk source row = Hips 7.1 /
  LeftFoot **56.97** / RightFoot **50.34** — reproduces the QA baseline (handoffs/TASK-221-222
  §1) EXACTLY, proving the Blender sampler and the UE AnimPose sampler agree on identical source
  data. `verify_method` string recorded in the report.
- **Gate application:** both feet ≥ floor on BOTH the baked in-scene action AND the re-imported
  artifact (:682-699); report `walk_gate` = "PASS — baked 61.79/52.96, artifact 61.59/52.79 ≥ 40.0";
  gate failure → exit 1 (:780-781). **False pass unreachable for the defect class:** a root-only
  export shows feet ≈ pelvis ≈ 7 uu (Walk horizontal root travel is only 8.5 uu) — an order of
  magnitude under the floor at both stages; a constraint/slot regression would show near-rest
  amplitudes at the BAKED stage and also fail. The artifact stage specifically re-imports the
  exported file — the exact failure surface of the UE defect — so an export-side FK drop cannot
  hide.
- The 4 artifact FBXs exist at the documented output path; artifact bone count 21 + armature name
  re-check are in-code (:659-664).

## 3. House contract — VERIFIED

- **Exit codes 0/1/2** — the Blender-side house style (rig_character/fix_rig_root lineage):
  usage/missing-input/`--check`-fail → 2; stage/gate failures + unhandled exceptions → 1
  (`--python-exit-code 1` invocation belt-and-braces); success/`--check` OK → 0.
- **OutputGuard confinement:** allowed roots exactly `Cache/<Unit>/retarget/**` +
  `MeshyRetargeted/<Unit>/**`, CardArt banned (:174-190); every write (debug blend, artifact FBX,
  report JSON) is funneled through `guard.check` — verified at all call sites. The importer-side
  `.fbm` extraction beside INPUT FBXs is honestly documented as outside the tool's own writes
  (quirk 4) — an import side-effect inside RawAssets, folders pre-existing; acceptable.
- **Export block mirrors `export_anim_fbx` VERBATIM** — diffed param-for-param against
  rig_character.py:771-783: identical list (apply_unit_scale, FBX_SCALE_NONE, −Z/Y axes, FACE,
  no leaf bones, bake_anim step 1.0 / simplify 0.0, no NLA/all-actions, deform-only False, bone
  axes Y/X, STRIP) with `object_types={'ARMATURE'}` as the single, documented deviation.
- **fps pinned to 30 BEFORE the clip import** (:620-623) with the re-pin after the 24-fps rig
  import and a post-import drift warning (:626-628) — the ordering the fps law requires.
- **bpy + stdlib only:** argparse/json/math/sys/time/traceback/datetime/pathlib + bpy/mathutils.
- **Fleet-ready:** every path/name composes from `--card-id`; `SHARED_SKELETON_ROOT="Footman_Rig"`
  is the TASK-212 fleet-wide shared-root LAW (all 9 unit rigs root there), not a Footman hardcode
  — the rename-warning path even self-heals deviations; 21-bone + mapped-bone validation is
  per-run; Meshy bone names are TASK-223-confirmed identical across units (recorded at the map).
  `--verify-only`/`--clips` support the TASK-231 loop.
- **Cavalry saddle-line hip-ratio warning:** recorded (handoff quirk 7, z-frac 0.688, preview-gated
  at TASK-231) ✓.

## 4. The 7 quirk pointers — each verified real + correctly stated for TASK-230

1. **Frame re-base 1..N → 2..N+1 on re-import** — real Blender FBX time-mapping behavior; span and
   duration preserved (report frames + seconds match to the ms); UE imports by time. Cosmetic —
   correctly stated.
2. **Idle hand amplification (109 vs 69 uu)** — FK rotation transfer sweeps a world arc ∝ chain
   length (our ~0.94 m arm chain vs Meshy ~0.71 m, plus clavicle contribution/pivot offsets);
   qualitatively correct, judged at the TASK-230 eyeball as stated.
3. **Attack foot_l 37.6 vs source 59.6** — plant-foot + proportion/root-scale interaction; the
   clip is decisively not root-only (foot_r 210.45, every limb ≠ pelvis); the no-skew claim is
   structurally guaranteed (no scale keys, pure normalized quats) — verified in code.
4. **`.fbm` side-folders** — real importer behavior, input-side, pre-existing, inside RawAssets;
   tool writes nothing there — correct.
5. **Source armature at 0.01 object scale** — uniform scale drops out of the world-space math and
   rotation decomposes; hip ratio computed from world heights — correct.
6. **fps law / call ordering** — verified in code (:620-623); the "do not reorder" warning is
   exactly right.
7. **Cavalry saddle-line Hips** — consistent with the TASK-223 §4 hold-back; translation-scaling
   caveat correctly scoped; preview-gated regardless.

## Findings

- [WARN — adjudicated non-blocking] No automated mirror/axis sanity check: the ≥40 amplitude floor
  cannot catch a left/right-mirrored or axis-flipped-but-moving retarget. Mitigated by the
  explicit L→L/R→R chain map, the preserved source L/R asymmetry in the recorded artifact numbers,
  and — decisively — the TASK-230 eyeball gate that owns visual correctness by design. OPTIONAL
  hardening for the fleet loop (TASK-231): assert the artifact's foot_l/foot_r asymmetry ordering
  matches the source's per clip (one comparison, catches a silent mirror before an editor session
  is spent).
- [NIT] `parse_args` (:204-207) converts argparse's `--help` SystemExit(0) into exit 2 — help
  exits with the usage-error code. Cosmetic.
- [NIT] A `--clips` subset excluding Walk skips the gate and exits 0 (warning + "SKIPPED" verdict
  recorded in the report) — deliberate subset semantics, but TASK-231's fleet runs must include
  Walk for the gate to bind (it does by default).
- [NIT] `load_target_rig` warns on non-identity loc/scale but not on object ROTATION — rotation is
  fully compensated by the world-space math, and the genuinely hazardous case (non-uniform scale)
  IS warned; purely a diagnostics gap.
- [NIT] The shared `warnings` list can repeat the rig-rename warning once per clip. Cosmetic.

## Notes for downstream

- **TASK-230:** the UE half of the gate (import onto SK_Footman_Skeleton, ≥40 uu after import,
  bone-name match, no skew, backups + root-motion OFF + RateScale per the TASK-222 spec) is now
  cleanly reachable; quirk 1 (frame re-base) and the root-motion-on-`root` design (UE decides
  root-lock) are the two import-side facts to remember.
- **TASK-231:** run `--check` per unit first; keep Walk in every gated run; Cavalry preview-gated
  (quirk 7). The optional mirror-asymmetry assert (WARN-1) would be a cheap pre-editor filter for
  the 8-unit loop.
- **build-master:** commit scope = the tool + `Content/RawAssets/Characters/MeshyRetargeted/`
  (raw-asset rule); `Cache/**` stays gitignored.
