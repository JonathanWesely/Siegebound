# QA Report — TASK-372 (TOOLING CODE LANE ONLY)
Verdict: **PASS** — 0 BLOCKERS · 4 WARN · 4 NIT

**Scope reviewed:** `Tools/ArtPipeline/rig_character.py` (per-asset `proportions` support + `head_base_z` key
+ override recorded in the report) and `Tools/ArtPipeline/rig_manifest.json` (`SiegeBiped.head_base_z`,
the `Sorcerer` entry + its `proportions` block, `_doc.proportions_override`).
**Out of scope:** the Sorcerer art itself, the un-started UE-editor import half of TASK-372, and the
`.fbx`/`.lod.json` artifacts (art-integration lane, not code).

**No shell/git access in this role** — everything below is from reading source, manifest, gate artifacts,
and the implementer's scratchpad evidence. Where a claim could only be settled with `git`, I say so and
hand build-master an exact command (see §Notes).

---

## Verdict on the four claims I was asked to break

### 1. "Bit-exact no-op for the 12 shipped rigs" — **UPHELD**, and it is stronger than the evidence claims

I verified this **structurally** (which outranks the empirical run) and then **corroborated the run**.

Structural proof, path by path:

* `merged_params()` (`rig_character.py:222-232`) — the new block is gated on
  `override = assets[card_id].get("proportions")` / `if isinstance(override, dict) and override:`.
  I read every asset entry in `rig_manifest.json`: **only `Sorcerer` carries a `proportions` key**
  (Footman, Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman, Miner, Archer, Ogre, Wizard
  carry only `input_fbx`/`skeleton`/`weapon_side`/`attack_style`/`_note`). For all 12 the branch is skipped,
  `skel_spec` remains the *same object* `skels[skel_name]`, and `params["_skeleton_spec"]` is what it was
  before the change. Identical path, not merely equivalent.
* Non-mutation is correct: `base_props = dict(...)` is taken **before** `skel_spec = dict(skel_spec)`, and
  `skel_spec["proportions"] = base_props` writes to the copy. The shared `skels["SiegeBiped"]` dict is never
  written. So even in a hypothetical multi-asset run the override cannot leak.
* `head_base_z` (`rig_character.py:385`) — `float(P.get("head_base_z", 0.905))` with
  `SiegeBiped.proportions.head_base_z = 0.905` in the manifest. `json.load` and the Python literal `0.905`
  both go through correctly-rounded decimal→double conversion, so they are **the same IEEE-754 double**.
  Not "close" — identical bit pattern. The old literal and the new lookup are indistinguishable.
* **Both** former literals were replaced with the same variable (`:395` neck_01 tail, `:396` head head), so
  they cannot drift apart. Replacing only one would have silently changed the neck/head joint relationship.
* Nothing else reads `spec["proportions"]`: `author_animations()`, `skin()`, `render_previews()` and the
  export stages never touch it (grep of `proportions` in the file returns only :211-232, :364, :381-385,
  :443-449). So the **anim FBXs are also unaffected** — the smoke run used `--no-anim-fbx`, so the empirical
  run did not cover clips; this structural argument does, and it holds because clip authoring is pure
  pose-space rotation on bone rest transforms that are themselves bit-identical.
* Report shape: `proportions_override` is emitted only under `if spec.get("_override_keys")`
  (`:442`), so every other asset's `rig_report.json` keeps its exact key set. Confirmed against the shipped
  `Cache/Footman/rig/rig_report.json` — no such key, and its `armature` block is unchanged in shape.
* Adding `head_base_z` to the shared `SiegeBiped.proportions` is inert for the other 12: nothing iterates
  `P`, every read is by explicit key name. And `rig_manifest.json` has exactly **one** reader in the repo —
  `rig_character.py` (grep across `Tools/**/*.py`), so no sibling tool is perturbed by the new key.

Corroborating the claimed run (the handoff quotes numbers but persists no artifact — see NIT-3):

* The comparison script is real: `<scratchpad>/cmp_footman.py`. Method is sound — imports the **shipped**
  `Content/RawAssets/Characters/Footman.fbx` and the post-change smoke FBX, and compares
  `matrix_world @ bone.head_local` componentwise across all bones, plus a name-set equality check.
* Its `B` path, `Cache/Footman/rig/smoke/Footman.fbx`, matches exactly what `--smoke` produces
  (`rig_character.py:1117-1119`), so the script really did read the post-change output.
* `--smoke` cannot touch the shipped FBX: `OutputGuard.__init__` only appends `CHARACTERS_RAW` to `allowed`
  **when not smoke** (`:151-156`), and every write goes through `guard.check` (`:820, :839, :909, :1025-1078,
  :1167, :1172`). The shipped Footman.fbx was structurally unreachable by that run.
* The backup/restore claim checks out: `<scratchpad>/footman_rig_backup/` exists with the full
  `rig_report.json` + `previews/` tree, the live `Cache/Footman/rig/rig_report.json` is still the shipped
  2026-07-26T22:01:44 non-smoke report (`"smoke": false`, `elapsed 43.7`), and `Cache/Footman/rig/smoke/`
  no longer exists. Consistent with "backed up, ran, restored".
* Precision floor worth knowing: `cmp_footman.py` rounds to 6 dp in metres, so "0.0" means < 5e-5 UE, not
  provably exact-zero. The structural argument above supplies the exactness the rounding cannot.

**Conclusion: no regression risk to the 12 shipped rigs. This does not block the batch commit.**

### 2. "The default really is unchanged" — **UPHELD**
`head_base_z` defaults to `0.905` in two independent places (manifest key and the `P.get(...)` fallback), and
an asset with no `proportions` block takes a byte-identical path (§1). Belt-and-braces defaulting is a plus
here: deleting the manifest key would still not change behaviour.

### 3. "The hard-fail is real and correctly scoped" — **UPHELD, with one hole (WARN-1)**
`unknown = sorted(k for k in override if k not in base_props)` → `fail(..., code=2)` (`:225-228`) validates
against the *chosen skeleton's* key set, so a valid key passes and a typo dies loudly with the offending
key names **and** the valid-key list in the message. Exit 2 is the documented code for a manifest error
(module docstring `:62` — "0 = OK, 1 = stage failure, 2 = usage/manifest/input error"), so the contract is
consistent. The hole is the *type* guard, not the *key* guard — see WARN-1.

### 4. "The Sorcerer override values" — **UPHELD, all 16 recomputed independently**
I recomputed every value as `nominal / 1.0545` and compared to the shipped manifest:

`foot_z` .0284495→**.02845** · `ankle_z` .0616406→**.06164** · `knee_z` .2560455→**.25605** ·
`hip_z` .4741584→**.47416** · `pelvis_z` .4883832→**.48838** · `spine1_top_z` .5689900→**.56899** ·
`spine2_top_z` .6638217→**.66382** · `spine3_top_z` .7633950→**.76339** · `neck_top_z` .8202940→**.82029** ·
`head_base_z` .8582267→**.85823** · `head_top_z` .9483167→**.94832** · `shoulder_z` .7586534→**.75865** ·
`elbow_z` .5689900→**.56899** · `wrist_z` .4172594→**.41726** · `hand_z` .3698435→**.36984** ·
`foot_forward_frac` .1043148→**.10431**.

All 16 correct to 5 dp (worst residual ~5e-6 ≈ 0.0009 UE). 15 `*_z` keys + `foot_forward_frac`. The factor
used is the **measured 1.0545**, not TASK-369's superseded ~9.2%; `neck_top_z` reproduces TASK-370's worked
example 0.8203 exactly.

**The `*_x_frac` exclusion is correct, and I confirmed it in code rather than accepting the argument.**
In `build_armature`, `hip_x/foot_x/clav_x/sho_x/elb_x/wri_x` multiply `hh`/`sh` — the **measured half-widths**
from `measure_anchors()` — never `H` (`:373-378`). `foot_fwd = P["foot_forward_frac"] * H` is height-scaled
(`:379`) and so **is** correctly included in the override. The scope split is exactly right: divide what
multiplies `H`, leave what multiplies `sh`/`hh`. Including the `*_x_frac` keys would have narrowed the rig
by 5.45% for no measured reason.

**Directional-sign check (the thing I was warned not to "correct"):** `z(frac) = base + frac*H` with `H` =
inflated bbox height ⇒ for fixed `frac`, a taller bbox puts the anchor **higher in absolute Z**, i.e. higher
**on the body**. Uncorrected `neck_top` = 0.865 × 181.81 = 157.27 UE = 0.9121 of the 172.42 body — the neck
would ride **up** into the skull. **The rig slides UP; the handoff's §7a correction is right and the shipped
override sign (divide) is right.** I did not touch it. (It is, however, still written backwards in two new
places — WARN-2.)

### Anchor-gate method (asked: does it actually support the PASS?) — **YES, with one honest caveat**
* The previews-are-useless point is correct: Blender does not render armature bones, so no shipped preview
  could have shown anchor placement. Building a bespoke check was the right call, not a workaround.
* `anchor_ruler.py` really is orthographic (`cd.type = "ORTHO"`, ortho_scale-driven `ue_per_px_v` recorded in
  `ruler_info.json`), so the "row maps linearly to Z" claim holds for the ruler images. I opened
  `ruler_head.png`: the green rule sits on the crown of the skull/hood dome and the red rule sits at the
  antler-tine tips. It reads exactly as described. (`anchor_check.py`'s three `overlay_*.png` are
  *perspective*, lens 60 — they are illustrative only; the handoff's "orthographic" claim is true of the
  ruler set, which is the set the verdict rests on.)
* The check loads the **exported** `Characters/Sorcerer.fbx`, i.e. the artifact UE will import, and dumps
  real bone heads (`mw_arm @ b.head_local`). All **21 bones are present in the export** with monotonic Z —
  independent proof the override neither dropped nor inverted a bone.
* **Caveat, stated plainly:** `frac_body` reduces algebraically to `frac_bbox × (181.805/172.42)`, so the
  "0.9999 vs target 1.0" table is *arithmetically implied* by the override being `nominal/1.0545`. It proves
  the value made it correctly through manifest → `merged_params` → `build_armature` → FBX (genuinely
  valuable: it is the end-to-end plumbing proof), but it does **not** re-derive the 172.42 body height —
  that is inherited from TASK-370. What *is* independent is the geometry: 106 verts of skull within ±3 UE of
  the corrected `head_top` vs 18 verts of thin tine (y-span 1.81 UE) at the uncorrected one, and 170 verts in
  the 165–175 skull band vs 51 in the 175–182 antler band. That is real corroboration of the inherited
  measurement, and it is enough. Verdict supported.
* Bone tails in `anchor_check.json` look "wrong" (`head` tail 162.93, `root` tail 88.79) — that is Blender's
  FBX importer reconstructing tails from children/parent length, **not** a rig defect. The heads are
  authoritative and correct. Corollary: `head_top_z` only sets the head bone's tail, which the FBX round-trip
  discards, so that key is cosmetic in UE. Harmless, but do not chase it later.

---

## Findings

- **[WARN-1]** `rig_character.py:223` — `if isinstance(override, dict) and override:` **silently ignores a
  malformed `proportions` value.** `"proportions": [...]`, `"proportions": "0.9"`, or a stray null all skip
  the branch with no message, and the rig ships uncorrected while the manifest claims otherwise — which is
  precisely the defect class this change exists to eliminate, re-entering by the type door instead of the key
  door. Fix: `if override is not None:` → `if not isinstance(override, dict): fail(f"asset '{card_id}'
  'proportions' must be an object, got {type(override).__name__}", code=2)` and treat `{}` as an explicit
  no-op (or fail it too). Not triggered today — Sorcerer's block is a well-formed dict — so it does not block.
- **[WARN-2]** `rig_character.py:216-218` **and** `rig_manifest.json` `_doc.proportions_override` — **the new
  documentation states the failure direction backwards.** Both say an overshooting crown makes anchors land
  "too LOW" and "slides the whole rig down the body". Measured and re-derived above: it slides the rig **UP**
  (`neck_top` rides at 0.9121 of body height instead of 0.865). The handoff §7a flags this error in the
  *board* text but then repeats it verbatim in the two places a future maintainer will actually read. The
  prescription (`corrected = nominal / factor`) is correct and the shipped values are correct, so there is no
  behavioural impact — but a maintainer reasoning from the stated direction could "fix" a correct override.
  Fix: replace "too LOW … slides the whole rig down" with "too HIGH on the body — the anchor is a fraction of
  an inflated bbox, so it lands higher in absolute Z than the intended fraction of the true body (Sorcerer:
  `neck_top` at 0.9121 of body instead of 0.865)". The divide stays exactly as-is.
- **[WARN-3]** `rig_character.py:222-232` — **override values are never validated for type, range, or
  ordering.** The new escape hatch makes hand-authored floats load-bearing for bone placement, and a slipped
  decimal (`8.2029`) or a swapped pair (`head_base_z` > `head_top_z`) produces a zero-length or inverted bone.
  Blender **removes zero-length bones when leaving edit mode**, which would silently ship a 20-bone skeleton —
  and that is the documented `MergeAllBonesToBoneTree` failure mode called out at `:99-104`. Compounding it,
  `report["armature"]["bone_count"]` is `len(bones)` (the *source list*, `:436`), so the report would still
  say 21. Cheap fix: after `bpy.ops.object.mode_set(mode="OBJECT")`, assert
  `len(arm_data.bones) == len(bones)` and fail otherwise; optionally range-check overrides to `0 < v <= 1.2`
  and assert the centreline `*_z` keys are non-decreasing. The Sorcerer is fine — the export carries all 21
  bones with monotonic Z (verified in `anchor_check.json`) — so this is a guard for the *next* override.
- **[WARN-4]** `Tools/ArtPipeline/rig_character.py` (whole file) — **it is the only `.py` under `Tools/` with
  CRLF line endings.** All six sibling pipeline scripts (`trellis_generate.py`, `concept_generate.py`,
  `meshy_generate.py`, `fix_rig_root.py`, `retarget_meshy_to_siegebiped.py`, `refine_trellis_glb.py`) are LF,
  and `rig_manifest.json` is LF (0 CRLF — that half of the handoff's claim is verified). There is **no
  `.gitattributes`** in the repo, so nothing normalises this on commit. I cannot tell without `git` whether
  the CRLF is pre-existing (likely, if an earlier edit flipped it) or was introduced here. **Zero runtime
  risk** — Python and Blender read CRLF fine, and the script demonstrably ran to exit 0. The risk is a
  1191-line whole-file diff that would bury this change and destroy blame on a shipped pipeline script.
  **Build-master must resolve this before committing — see Notes.**
- **[NIT-1]** `rig_character.py:199-203` — the generic merge also leaves the raw asset block at
  `params["proportions"]` (a *partial*, unmerged dict for Sorcerer; absent for everyone else). Nothing reads
  it today, so it is inert, but it is a trap for the next author who reaches for `params["proportions"]`
  instead of `params["_skeleton_spec"]["proportions"]`. Suggest `params.pop("proportions", None)` after
  `_skeleton_spec` is built, or a one-line comment.
- **[NIT-2]** `rig_character.py:436` — `bone_count` reported from the input list rather than the built
  armature (see WARN-3). Suggest `len(arm_obj.data.bones)`.
- **[NIT-3]** Evidence durability — the §3 regression numbers exist only as prose in the handoff. The smoke
  FBX was (correctly) cleaned up in the restore, so the comparison is no longer reproducible from artifacts;
  only `<scratchpad>/cmp_footman.py` survives, and a scratchpad is session-scoped and disposable. For a claim
  this load-bearing, the next run should dump the comparison JSON into
  `Cache/<CardID>/rig/` next to the other gate evidence. Does not affect the verdict — the structural proof
  in §1 stands on its own.
- **[NIT-4]** `anchor_check.py`'s `overlay_*.png` are perspective (lens 60) while `ruler_*.png` are ortho; the
  handoff describes the whole set as orthographic. The verdict rests on the ruler set and the numeric
  occupancy data, so nothing is wrong — just do not cite `overlay_*.png` for a measurement.

## Checked and clean (no finding)

- **Secrets:** the script reads no environment variables, has no tokens, no `os.environ`/`getenv`, and takes
  nothing sensitive on argv (`--card-id`, `--manifest`, `--input`, four flags). Nothing to leak. ✔
- **Network:** no `requests`/`urllib`/socket use anywhere — this stage is fully local, so the timeout rule is
  not applicable. ✔
- **Write confinement:** `OutputGuard` is unchanged and every write still routes through `guard.check`;
  smoke mode is confined to `Cache/<CardID>/rig/`, non-smoke adds only `Content/RawAssets/Characters`. The new
  code adds **no writes at all**. No path can escape into another chain's territory. ✔
- **Exception handling:** no bare `except:`. The two broad handlers (`:524` skinning fallback, `:918` LOD
  sidecar) both log the exception `repr` and continue by design; the top-level handler prints the traceback
  and exits non-zero (`:1184-1191`). Nothing is swallowed silently. ✔
- **Headless-bpy:** the change introduces **no `bpy` calls whatsoever** — it is pure dict manipulation before
  any Blender work. No UI-context `bpy.ops`, no `view_layer`/window assumptions added. Re-runs stay
  idempotent. ✔
- **Exit-code contract:** new `fail(..., code=2)` matches the documented "2 = usage/manifest/input error". ✔
- **Conventions:** shared-skeleton law intact — armature object is still the `SHARED_SKELETON_ROOT`
  `Footman_Rig` constant, bone names/hierarchy untouched, 21 bones. Asset paths in the manifest match the
  spec (`Content/RawAssets/Sorcerer.fbx` in, `Characters/Sorcerer.fbx` + `Anims/Sorcerer_<Action>.fbx` out).
  `rig_manifest.json` is valid JSON, 13 assets, LF. ✔

---

## Notes for build-master

1. **Run this before staging, it is one command:**
   `git diff --numstat -- Tools/ArtPipeline/rig_character.py`
   * If it reports roughly **1191 insertions / 1191 deletions** (or `git diff --stat` shows the whole file),
     the line endings flipped to CRLF **in this change** — renormalise the file to LF and re-diff so the
     commit shows only the ~30 real lines. Do not commit a whole-file EOL churn on a shipped pipeline script.
   * If it reports a **small diff** (~30-40 lines across `merged_params`, `build_armature`, and the report
     block), the CRLF is pre-existing in HEAD and WARN-4 downgrades to a NIT — commit as-is and leave the
     EOL cleanup to a separate housekeeping change.
2. **Commit only the two files this report covers** from the tooling lane:
   `Tools/ArtPipeline/rig_character.py` and `Tools/ArtPipeline/rig_manifest.json`.
   `Tools/ArtPipeline/Cache/**` is gitignored gate evidence — do **not** stage `Cache/Sorcerer/rig/anchor_check/`.
3. **Nothing here needs a compile.** This is headless Blender tooling with no UE C++ or Blueprint surface, so
   the UE build gate does not apply to these two files. Do not spend an editor bounce on them.
4. **The rest of TASK-372 is NOT qa-passed by this report.** This verdict covers the *tooling code lane* only.
   The UE-editor half (`SK_Sorcerer` import, the four `A_Sorcerer_*` clips, LOD chain + `lod_count == 3`
   readback) is still not started, and the `.fbx`/`.lod.json` art artifacts are an art-integration check, not
   a code review. Do not let a PASS here be read as "TASK-372 done".
5. WARN-1/2/3 and the NITs are **follow-up quality**, not commit blockers. WARN-2 (the backwards direction in
   the code comment and the manifest `_doc`) is the one worth folding into the next touch of this file — it is
   a two-sentence text change and it is the sentence a future maintainer will act on.

## Board status (could not apply — no Edit tool in this role)

I hold Read/Grep/Glob/Write only; updating `TASKBOARD.md` would mean rewriting a 2600-line hub file in full,
which is exactly the write-race hazard the pipeline has been bitten by before. **Orchestrator/manager: please
append this clause to the TASK-372 status line** (do not overwrite the existing text — the task is still
half-done):

> 🔍 **TOOLING CODE LANE qa-passed** (2026-08-01, `qa/TASK-372-tooling-report.md`) — 0 blockers, 4 WARN, 4 NIT.
> Bit-exact-no-op-for-the-12-shipped-rigs claim **UPHELD** (verified structurally *and* against the surviving
> `cmp_footman.py` + Footman cache backup); `head_base_z` default `0.905` confirmed identical; unknown-key
> exit-2 hard-fail confirmed real and correctly scoped; all 16 Sorcerer override values independently
> recomputed at `nominal / 1.0545`; the `*_x_frac` exclusion confirmed **in code** (they multiply measured
> half-widths `sh`/`hh`, never `H`). Anchor-gate method accepted. Open WARNs: malformed (non-dict)
> `proportions` still silently ignored; the new code comment + manifest `_doc` state the inflation direction
> **backwards** (it slides the rig UP, not down); no range/ordering validation on override values; and
> `rig_character.py` is the only `.py` under `Tools/` with CRLF — build-master must run
> `git diff --numstat` on it before staging. **The editor half of TASK-372 remains un-reviewed and un-done.**
