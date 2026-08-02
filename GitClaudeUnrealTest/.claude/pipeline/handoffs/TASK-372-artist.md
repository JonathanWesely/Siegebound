# TASK-372 — [AG-A5] Sorcerer rig + anims — **HEADLESS BLENDER PASS ONLY**

**Agent:** art-director · **Date:** 2026-08-01 · **Status: headless-complete, import-pending.**

Stage 3b (`rig_character.py`) ran clean and the anchor gate passes. **The UE editor was never opened**
(Jonathan is hand-editing a widget and the editor is exclusively his): **no `SK_Sorcerer` import, no anim
imports, no LOD regeneration, no `ABP_Sorcerer` (never author one), no Git.**

---

## 0. Headline

✅ **Rig exit 0 in 40.5 s · 21 bones · root `Footman_Rig` · skinning `auto_heat` with 0.00% unweighted · zero warnings.**
✅ **All four clips authored** — `Idle`/`Walk`/`Attack`/`Death`. The Attack clip exists per the Cleric precedent (§5).
📐 **The antler override works, and I can prove it two ways.** Every shipped anchor now lands at its *nominal*
fraction **of true body height** to 4 dp (e.g. `neck_top` 0.8649 vs nominal 0.865; `head_top` 0.9999 vs 1.0), and the
overlay render shows the corrected `head_top` rule sitting **on the skull dome** while the uncorrected one sits **on
the antler tips**.
⚠️ **The ruling could NOT be implemented by editing the manifest alone — `rig_character.py` had no per-asset
`proportions` support and the head anchor was a hard-coded literal.** Both fixed; **proven a bit-exact no-op for the
12 shipped rigs** by a real regression run (§3). This is the one thing to look at before anything else.
⚠️ **A directional claim in the board text is backwards** — the overshoot pushes the rig **UP** the body, not down (§7a).

---

## 1. The override values actually shipped

`INFLATION FACTOR = 1.0545` from TASK-370 §6 (measured on the conformed mesh: bbox 181.805 UE, skull dome apex
172.42 UE, overshoot 9.39 UE = 5.45% of body). Rule applied: **`corrected = nominal / 1.0545`**.

| key | nominal | **shipped** | resolves to (UE) | nominal × body height (UE) |
|---|---|---|---|---|
| `foot_z` | 0.03 | **0.02845** | 5.17 | 5.17 |
| `ankle_z` | 0.065 | **0.06164** | 11.21 | 11.21 |
| `knee_z` | 0.27 | **0.25605** | 46.55 | 46.55 |
| `hip_z` | 0.50 | **0.47416** | 86.20 | 86.21 |
| `pelvis_z` | 0.515 | **0.48838** | 88.79 | 88.80 |
| `spine1_top_z` | 0.60 | **0.56899** | 103.45 | 103.45 |
| `spine2_top_z` | 0.70 | **0.66382** | 120.69 | 120.69 |
| `spine3_top_z` | 0.805 | **0.76339** | 138.79 | 138.80 |
| `neck_top_z` | 0.865 | **0.82029** | 149.13 | 149.14 |
| `head_base_z` | 0.905 | **0.85823** | 156.03 | 156.04 |
| `head_top_z` | 1.0 | **0.94832** | 172.41 | **172.42 = the skull apex** |
| `shoulder_z` | 0.80 | **0.75865** | 137.93 | 137.94 |
| `elbow_z` | 0.60 | **0.56899** | 103.45 | 103.45 |
| `wrist_z` | 0.44 | **0.41726** | 75.86 | 75.86 |
| `hand_z` | 0.39 | **0.36984** | 67.24 | 67.24 |
| `foot_forward_frac` | 0.11 | **0.10431** | 18.96 (toe offset) | 18.97 |

Every one resolves within **0.01 UE** of nominal × 172.42. `neck_top_z` matches TASK-370's worked example
(**0.8203**) exactly. Max rounding error across the set is 5e-6 (0.0009 UE).

**Two deliberate scope calls, both documented in the manifest `_note`:**
- **`foot_forward_frac` IS corrected** — the `_doc` states it is a fraction of *height*, so the same normalization
  applies. Magnitude 20.00 → 18.96 UE of toe offset.
- **The six `*_x_frac` keys are NOT corrected** — they scale measured **half-widths**, which antlers do not
  inflate. Dividing them would have been an unmeasured guess.

**I used 1.0545, not TASK-369's ~9.2%.** Dividing by ~1.09 would have put `head_top` at ~158 UE — 14 UE *below*
the skull, inside the cowl. The stale number would have over-corrected, exactly as warned.

---

## 2. ⚠️ The manifest alone could not carry the ruling — two tooling changes were required

`Tools/ArtPipeline/rig_character.py` (the only code I touched; art-pipeline tooling, no gameplay code):

1. **`merged_params()` did not read a per-asset `proportions` key at all.** `params["_skeleton_spec"] = skels[name]`
   handed `build_armature` the *shared* skeleton dict, so an asset-level `proportions` block would have been
   silently ignored and the rig would have shipped **uncorrected while the manifest claimed otherwise** — the worst
   possible outcome. Now the override is overlaid onto a **copy** (shared spec never mutated), and **unknown keys
   hard-fail with exit 2** rather than silently doing nothing.
2. **The head anchor was a bare `0.905` literal** in `build_armature`, in two places. Un-overridable, so the head
   bone would have stayed stranded at antler-inflated height while all 14 other anchors moved. It is now the
   proportions key `head_base_z` with the **same 0.905 default**, added to the shared `SiegeBiped` spec.

Also added: `report["armature"]["proportions_override"]` (keys + resolved fractions + resolved UE heights) — emitted
**only when an override exists**, so every other asset's `rig_report.json` keeps its exact shape.

---

## 3. Regression: the tooling change is bit-exact for the 12 shipped rigs

Not argued — **run**. `rig_character.py -- --card-id Footman --smoke --quick --no-anim-fbx`, then the smoke FBX's
bones compared against the **shipped** `Content/RawAssets/Characters/Footman.fbx`:

```
bone_names_identical: true   bone_count: 21 vs 21
max_head_delta: 0.0 m  (0.0 UE)   worst_bone: null
```

**Zero drift on all 21 bones.** Structurally guaranteed too: the override branch only fires when an asset carries a
non-empty `proportions` dict (**only `Sorcerer` does**), and `head_base_z` resolves to the identical 0.905.
`--smoke` confined all writes to the cache; **`Cache/Footman/rig/` was backed up before the run and restored
byte-for-byte after** (the TASK-370 `shipped_locked` pattern) — the shipped Footman report/previews are untouched.

---

## 4. ⚠️ The anchor eyeball — and why the stock previews were not enough

**The stock bind/turntable/contact previews render the MESH ONLY — armatures do not render in Blender, so there is
no bone in any of them.** Eyeballing them would have told me nothing about anchor placement, which is the entire
point of this task. I built a purpose-made check that loads the **exported** `Characters/Sorcerer.fbx` (verifying the
artifact that will actually be imported, not in-memory state) and renders **orthographic** views — so image row maps
linearly and exactly to Z, with no perspective ambiguity — with the corrected anchors ruled in **green**, the
uncorrected ones in **red**, and the real bone segments in cyan.

**Artifacts** (`Tools/ArtPipeline/Cache/Sorcerer/rig/anchor_check/`): `ruler_front.png`, `ruler_side.png`,
**`ruler_head.png`** (the money shot), `ruler_legs.png`, `overlay_{front,side,threequarter}.png`,
`anchor_check.json`, `ruler_info.json`.

### 4a. Verdict — PASS, and not marginally

| anchor | shipped, as fraction **of body** | uncorrected would have been | mesh at shipped Z | mesh at uncorrected Z |
|---|---|---|---|---|
| `head_top_z` | **0.9999** (target 1.0) | 1.0544 | half-X 15.01, **106 verts** | half-X 11.58, **18 verts** |
| `head_base_z` | **0.9049** (target 0.905) | 0.9543 | half-X 26.32, 245 verts | half-X 14.98, 114 verts |
| `neck_top_z` | **0.8649** (target 0.865) | **0.9121** | half-X 37.96, 302 verts | half-X 23.88, 220 verts |
| `shoulder_z` | **0.7999** | 0.8435 | half-X 40.03 | 38.64 |
| `hip_z` | **0.5000** | 0.5272 | half-X 42.63 | 42.66 |
| `knee_z` | **0.2700** | 0.2847 | half-X 39.30 | 38.74 |

The uncorrected `neck_top` figure reproduces TASK-370's **0.9121** exactly — independent cross-validation of the
measurement I was handed.

**In `ruler_head.png` you can see it directly:** the green `head_top` rule lands **on the crown of the skull dome**;
the red one lands **at the antler tips**. The occupancy numbers say the same thing without squinting — at the
corrected height the mesh has 106 verts of solid skull, at the uncorrected height only **18** verts of thin antler
tine. **The anchors landed on the body, not the antlers.** No retune was warranted, so I did not invent one.

### 4b. Clips eyeballed

`contact_{idle,walk,attack,death}.png` + `bind_front.png` reviewed. Bind pose is a clean A-pose; identity fully
intact (antler crown, sea-green maskless skull, cream ribbed cowl, **blue `TeamRegion` reading strongly off the
plate tops**, moss-green robe, brown sash, ragged hem, boots, monolith). Walk alternates legs with arm swing;
Attack reads as a raise-and-drive consecration gesture, not a weapon strike.

---

## 5. The Attack clip exists on purpose

`attack_style: "cast"`, produced even though this unit never attacks. Cleric precedent (`rig_manifest.json`'s own
`Cleric._note`): one run authors all four for free, `ASummonedUnit::CacheActionAnimations` composes the path, and it
becomes the ready-made hook for a ground-consecration gesture. **"Attack" is a slot name, not a semantic claim.**

---

## 6. Readback for the import pass

| item | value |
|---|---|
| **top-level `"skeleton"` (THE AUTHORITATIVE FIELD)** | **`SiegeBiped`** |
| `armature.skeleton` | `Sorcerer` — **this is the CARD ID, NOT an asset name** (the recorded myth-source) |
| **UE binding target** | the **EXISTING** `/Game/Characters/SK_Footman_Skeleton` — **never create a skeleton** |
| armature object / root | **`Footman_Rig`** (the constant; the shared-skeleton contract) |
| bones | **21** (20 deform), UE-mannequin names, hierarchy identical to every shipped unit |
| skinning | **`auto_heat`**, unweighted **0.00%** (better than the Wizard, which fell back to envelope) |
| mesh | 15,000 tris · 7,496 verts · UV layer **`UVMap`** |
| material slots, in order | **`[TeamRegion, SorcererPBR]`** |
| clips | `Idle` 60f · `Walk` 30f · `Attack` 40f · `Death` 48f, all @ 30 fps |
| warnings | **none** |
| measured bbox | 181.81 × 90.98 × 78.37 UE, shoulder_half 39.20, hip_half 43.46 |

**Still owed (the pass I was told to stop before):** import `SK_Sorcerer` → `/Game/Characters/SK_Sorcerer`;
`A_Sorcerer_{Idle,Walk,Attack,Death}` → `/Game/Characters/Anims/`; LOD chain per the recipe; **readback
`lod_count == 3`**. **NO `ABP_Sorcerer`** — `ResolveSkeletalVisual` falls back to `ABP_Footman`. The skeletal yaw is
C++-owned (`SkeletalVisualYawOffset = -90.f`) — do not hand-author it on any asset.

`Sorcerer.lod.json` is schema-identical to `Wizard.lod.json` (LOD1 50%@0.4 / LOD2 20%@0.15 + both URO flags);
only `card_id` / `mesh_fbx` / `skeletal_mesh_asset` differ.

---

## 7. Three things worth your eye

### 7a. ⚠️ The board's TASK-369 gate text states the failure direction backwards

It says an overshooting prop "slides the whole rig **down** the body." **Measured, it slides it UP.** Anchors are
fractions of the *measured* bbox, so inflating the bbox pushes every anchor **higher in absolute UE**, i.e. higher
*on the body* — `neck_top` rides at 0.9121 of body height instead of 0.865. TASK-370's numbers are self-consistent
and the correction (divide by the factor) is unaffected, so **nothing changed in what I did**. Flagging it only so
the next reader doesn't "fix" a correct override after reasoning from the wrong direction.

### 7b. `measure_anchors()` samples its half-width bands as fractions of the INFLATED bbox — measured negligible

The `(0.76, 0.84)` shoulder and `(0.46, 0.54)` hip bands are hard-coded fractions of bbox height, so under inflation
they sample 138.17–152.72 UE rather than the intended 131.04–144.83. **I measured the consequence rather than
assuming it:** shoulder_half 39.20 as-run vs 40.85 body-normalised (**Δ 1.65 UE → 1.42 UE at the shoulder joint**),
hip_half 43.46 vs 42.66 (**Δ 0.80 UE → 0.42 UE at the hip**). Both far under a single tri's width at this scale.
**I deliberately left `measure_anchors()` untouched** — the blast radius would have been all 13 assets for a
sub-2 UE effect, and it is not what the ruling asked for. Recording it so it is a known, quantified non-issue.

### 7c. Arm and leg bones ride at the outer edge of the robe — fleet-wide, NOT an antler artifact

`shoulder_x_frac` etc. multiply the *measured* half-width, which for a slab-mantled robed figure is the plate/robe
edge rather than the joint: shoulders land at x ±33.71 where the silhouette is ±40.03; leg bones at x ±21.73 where
the mesh legs sit nearer ±29. Identical in kind to the shipped Wizard (sho_x 52.3). Bone-heat still returned
**0.00% unweighted** — every bone is inside the volume. **This is the `*_x_frac` lane and the antler factor has no
bearing on it; "fixing" it would be an unmeasured tune.** Flagged for playtest, not acted on. Related: the rune
monolith is body geometry, so it swings with the arm in `Walk` — same class as the Wizard's staff and the Ogre's maul.

---

## 8. Files

**Shipped (created/modified; no editor, no Git):**

| path | note |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Sorcerer.fbx` | **rigged FBX**, bind pose, armature `Footman_Rig` + `SK_Sorcerer` mesh, 822,604 B |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Sorcerer.lod.json` | SK-LOD/URO recipe for the import pass |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Anims\Sorcerer_Idle.fbx` | 60 f |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Anims\Sorcerer_Walk.fbx` | 30 f |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Anims\Sorcerer_Attack.fbx` | 40 f (`cast`) |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Characters\Anims\Sorcerer_Death.fbx` | 48 f |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\rig_manifest.json` | new `Sorcerer` entry **+ `proportions` override**; `head_base_z: 0.905` added to `SiegeBiped`; `_doc.proportions_override` written. 13 assets, JSON valid, **LF preserved (0 CRLF)** |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\rig_character.py` | §2 — per-asset override support + `head_base_z` key + override recorded in the report |

**Cache (gitignored):** `Tools\ArtPipeline\Cache\Sorcerer\rig\` — `rig_report.json`, `previews\` (bind stills,
turntable strip, 4 contact strips, full per-anim PNG sequences), and **`anchor_check\`** (the §4 gate evidence).

---

## 9. Post-QA follow-up — WARN-1 and WARN-2 closed (same pass, 2026-08-01)

Tooling QA (`qa/TASK-372-tooling-report.md`) returned **PASS, 0 blockers**, upholding the §3 no-regression claim
structurally as well as empirically. Two of its four WARNs were fixed immediately rather than logged.

### WARN-1 — malformed `proportions` was still silently ignored (FIXED)

`isinstance(override, dict) and override` let a **non-dict** `proportions` fall straight through the branch — the
exact defect from §2.1 re-entering through the *type* door instead of the *key* door. A `proportions: []` or
`proportions: "…"` typo would have shipped an **uncorrected rig while the manifest claimed a correction**.

Now a **present** `proportions` must be a non-empty object or the run **hard-fails exit 2 naming the offending
value**; an **absent** key is untouched and still takes the default path. An **empty `{}` also fails** — it claims
an override and applies none, which is the same silent-no-op class.

**Proven by an 18-check harness** (`test_override_guard.py`, run in Blender against the real manifest, calling
`merged_params()` only — no scene work, no writes), **ALL PASS**:

| group | checks |
|---|---|
| shipped manifest still resolves | Sorcerer resolves, override applied, `head_top_z` 0.94832, `shoulder_x_frac` still 0.86 |
| the other 12 untouched | all 12 use the pristine shared spec; **shared `SiegeBiped` dict unmutated after all 13 resolves**; `head_base_z` default still exactly 0.905 |
| **WARN-1 hard-fail** | `[]`, `[1,2]`, `"0.94832"`, `1.0545`, `null`, `true`, **`{}`** → all **exit 2** |
| regressions | unknown key still exit 2; a valid override on a *second* asset applies and does **not** leak into the shared dict; absent key → default path, no `_override_keys` |

### WARN-2 — the direction was written backwards in both maintainer-facing places (FIXED)

I flagged the error in §7a and then repeated it in the code comment and the manifest `_doc` — the two places
someone will actually act on. Both now state that an inflated bbox pushes anchors **UP** the body (`neck_top` 0.865
landing at **0.9121** of true body height; `head_top` 1.0 landing on the **antler tips**), each carrying an explicit
**MIND THE SIGN** warning that correcting from the wrong direction would **double** the error rather than remove it.
Values and prescription were always correct — only the prose was wrong, and nothing shipped changed.

### Not done this pass (recorded follow-ups, per instruction)

**WARN-3** — no type/range/ordering validation on override *values*; a bad future override could ship a 20-bone
skeleton while `bone_count` still reports 21 (it reads the source list, not the built armature).
**WARN-4** — `rig_character.py` is the only `.py` under `Tools/` with CRLF; **build-master runs
`git diff --numstat` on it before staging** to decide whether the EOLs flipped here or are pre-existing in HEAD.

**EOLs re-verified after these fixes:** `rig_character.py` 1,211 CRLF / **0 bare LF**; `rig_manifest.json`
**0 CRLF** / 147 LF. Neither file's EOL style was changed by any of my edits. Both still parse
(`py_compile` OK, JSON valid, 13 assets). **Editor still untouched; no Git.**

---

# PART 2 — THE UE-EDITOR IMPORT HALF (2026-08-02)

Editor released by Jonathan. PIE confirmed stopped and **no asset editors open** before touching anything.
**`L_Arena` never opened or saved · no save-all (every save an explicit path list) · no Git · no gameplay code ·
no `ABP_Sorcerer` · editor never closed.**

## 10. Headline

✅ **`SK_Sorcerer` imported and bound to the EXISTING `/Game/Characters/SK_Footman_Skeleton`** — the import returned
**exactly one asset**, and a project-wide scan confirms **`SK_Footman_Skeleton` is still the only Siegebound
skeleton**. No orphaning.
✅ **All four `A_Sorcerer_*` clips** on the shared skeleton, root motion OFF, `bForceRootLock` ON.
✅ **Slots read off the asset before assigning** (not assumed) → `[TeamRegion, SorcererPBR]` → `MI_TeamColor_Blue` /
`MI_Sorcerer_PBR`, verified by readback.
🔴 **THE LOD CHAIN IS NOT DONE AND I DID NOT FAKE IT — `lod_count == 1`, acceptance is 3.** The blocker is *not*
"MCP can't do it" as the spec anticipated; there is a proven working method, and **the permission system denied the
one step it needs**. Details + exactly what remains in §14 — **this needs Jonathan's decision.**
🔵 **FACING: corroborated. `SK_Sorcerer` renders BACK-facing**, matching TASK-371's static-side finding — so
**+90 stays the leading candidate for TASK-375, −90 suspect** (§13).
⚠️ **Two fleet-consistency findings worth knowing before playtest** (§15): the clips import at **24 fps, not the
authored 30**, and the Wizard shipped **without** `bForceRootLock`.

## 11. What was imported

| Asset | Path | Verified |
|---|---|---|
| `SK_Sorcerer` | `/Game/Characters/SK_Sorcerer` | SkeletalMesh, saved, not dirty |
| `A_Sorcerer_Idle` | `/Game/Characters/Anims/A_Sorcerer_Idle` | AnimSequence, 60 frames |
| `A_Sorcerer_Walk` | `/Game/Characters/Anims/A_Sorcerer_Walk` | AnimSequence, 30 frames |
| `A_Sorcerer_Attack` | `/Game/Characters/Anims/A_Sorcerer_Attack` | AnimSequence, 40 frames |
| `A_Sorcerer_Death` | `/Game/Characters/Anims/A_Sorcerer_Death` | AnimSequence, 48 frames |

Import used the proven TASK-165 recipe. The anim step still behaves exactly as documented: each
`import_file(..., import_animations=True)` produces a **spurious `SkeletalMesh` at the target name** plus the real
`AnimSequence` at `<name>_Anim`. I deleted the spurious mesh **guarded on `get_asset_class == "SkeletalMesh"`** and
renamed the sequence into place **guarded on `== "AnimSequence"`** — never a blind delete. Post-state:
`tmp_still_exists: false` on all four, and `/Game/Characters` contains **exactly 5** Sorcerer assets, no strays.

## 12. Hard readback

| Check | Required | Observed |
|---|---|---|
| **Bound skeleton** | the EXISTING shared one | **`/Game/Characters/SK_Footman_Skeleton`** ✅ |
| **New skeleton created?** | **NO** | import returned 1 asset; project-wide Skeleton scan finds **no `SK_Sorcerer_Skeleton`** ✅ |
| Bone count | 21 rig bones + the `Footman_Rig` object node | **22** ✅ |
| Bone list | identical to the fleet | **character-for-character identical to `SK_Footman`** ✅ |
| **Material slots, in order** | `[TeamRegion, SorcererPBR]` | **read off the asset: `["TeamRegion","SorcererPBR"]`** ✅ |
| Slot 0 / Slot 1 | `MI_TeamColor_Blue` / `MI_Sorcerer_PBR` | both confirmed by readback ✅ |
| Sections (LOD0) | 2 | **2** ✅ |
| Vertices (LOD0) | — | 15,661 (UE seam-splits the 7,496 source verts; fleet-normal) |
| Bounds | 181.8 UE tall, feet at origin | boxExtent `(45.49, 39.18, 90.90)`, origin z 90.96 → **min z ≈ 0.06** ✅ matches Blender exactly |
| Anim skeleton | shared, all four | ✅ all four |
| Root motion | OFF | `bEnableRootMotion=false` on all four ✅ |
| `force_root_lock` | ON | `bForceRootLock=true` on all four ✅ (set this pass) |
| **`lod_count`** | **3** | **1 — NOT MET, see §14** 🔴 |
| Dependencies | clean | exactly `[SK_Footman_Skeleton, MI_TeamColor_Blue, MI_Sorcerer_PBR]` ✅ |
| `ABP_Sorcerer` | must NOT exist | not created ✅ (runtime falls back to `ABP_Footman`) |
| Dirty state | all saved | all 5 `is_dirty=false` ✅ |

## 13. 🔵 FACING — corroborated from the skeletal side

TASK-371 found `SM_Sorcerer` renders **back**-facing while `SM_Footman`/`SM_Cleric`/`SM_Wizard` render front. I was
asked to check the skeletal side independently.

**Method — I did not judge this by impression.** A thumbnail alone is weak evidence, because "no face visible" is
not decisive for a unit whose concept has a *faceless* stone mask. So I rendered the **exported rigged FBX** from
known −Y (front) and +Y (back) in Blender with the real albedo, and compared both against UE's thumbnail:

| cue | Blender FRONT (−Y) | Blender BACK (+Y) | **UE `SK_Sorcerer` thumbnail** |
|---|---|---|---|
| central sash | **red/dark vertical sash present** | absent | **absent** |
| diagonal strap | — | **tan/brown strap, left** | **tan/brown strap, left** |
| head | skull mask reads as a dome | ribbed cowl mass, no mask | **ribbed cowl mass, no mask** |
| palette | darker | pale mint | **pale mint** |

**Verdict: `SK_Sorcerer` matches the BACK render — it is back-facing, ~180° from the fleet, consistent with
`SM_Sorcerer`.** Control: `SK_Footman` in the same thumbnail camera is unambiguously front-facing (face, shield,
spear all visible). This is expected rather than surprising — `rig_character.py` exports with an axis contract
*deliberately identical* to the static pipeline ("so SK faces the same way as SM"), so SM and SK agreeing is the
designed behaviour.

**Scope limit, kept deliberately narrow (TASK-371's caution):** this establishes rotation **relative to the fleet**,
**not** an absolute world axis. **For TASK-375: treat +90 as the leading candidate and −90 as suspect** — now
supported by two independent readings (static and skeletal) rather than one.

## 14. 🔴 THE LOD CHAIN — blocked, and NOT by what the spec expected

**State: `SK_Sorcerer` has `lod_count == 1`. Acceptance is 3. This is genuinely outstanding.**

**I first checked whether this was even a real gap, and the answer changed my conclusion.** TASK-289 recorded the SK
fleet as LOD0-only, which would have made `lod_count==1` fleet-consistent. **That is stale** — I measured all 13
units live: **every one of the 12 shipped units reports `lod_count == 3`.** `SK_Sorcerer` is the **only** unit
without a LOD chain. So this is a real gap, not a fleet norm. (Applied by TASK-297 after the TASK-289 audit.)

**The method exists and does not require closing the editor.** TASK-297 documents it: `regenerate_lod` is
editor-python and is *not* in any MCP toolset (`SkeletalMeshTools` is readback-only; the ProgrammaticToolset sandbox
blocks `import unreal`) — so it runs over the **UE Python Remote Execution** lane, which needs
`bRemoteExecution=true` flipped on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings`.

**⚠️ That flip was DENIED by the permission system.** I did not attempt to reach the same end by another route
(e.g. editing the plugin's config on disk) — that would be circumventing the intent of the denial rather than
respecting it. **This needs Jonathan's decision, and it is the one thing standing between TASK-372 and done.**

**Exactly what remains** (everything else is in place; the recipe is already on disk at
`Content/RawAssets/Characters/Sorcerer.lod.json`):
1. Enable `bRemoteExecution` (or apply LODs manually in the editor, or run a headless `-run=pythonscript` commandlet
   with the editor closed).
2. Per TASK-297: build a **transient** `SkeletalMeshLODSettings` with 3 `SkeletalMeshLODGroupSettings`
   (LOD1 **50% @ screen 0.4**, LOD2 **20% @ 0.15**) → assign to `sk.lod_settings` →
   `SkeletalMeshEditorSubsystem.regenerate_lod(sk, 3)` → **restore `lod_settings` to None** (the reduction bakes into
   LODInfo and persists, so no companion asset is created).
3. ⚠️ **The property is `num_of_triangles_percentage`** — *not* `number_of_triangles_percentage`, which does not
   exist and errors (TASK-297's correction to the TASK-288 recipe). With
   `reduction_method = SMOT_NUM_OF_TRIANGLES` + `termination_criterion = SMTC_NUM_OF_TRIANGLES`.
4. Readback `lod_count == 3` and save `SK_Sorcerer`.

**URO needs nothing** — `OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations` have no home on the
`USkeletalMesh` asset; they are already set in the `ASummonedUnit` constructor for every unit (TASK-285/297).

## 15. ⚠️ Two fleet-consistency findings (reported, not acted on)

### 15a. The clips import at 24 fps, not the authored 30

All four Sorcerer clips read **24 fps** in UE (Idle 60 frames / 2.5 s). The manifest authors at `fps: 30`, so each
clip plays **~20% slower** than intended (Idle 2.0 s → 2.5 s; Walk 1.0 s → 1.25 s).

**Cause, found in my own tooling:** in `rig_character.py::main`, the anim FBXs are exported *before*
`render_previews` sets `scene.render.fps` — so the export runs at Blender's default 24 fps and the manifest `fps`
never reaches the exported clips.

**It is fleet-consistent with the unit it should match:** `A_Wizard_*` is **also 24 fps** (same rig-lane tool).
`A_Footman_*`/`A_Cleric_*` are 30 fps because those came through the **Meshy retarget lane**, not this one
(Footman Idle is 120 frames / 4 s — not this script's 60-frame Idle). **So I introduced nothing new**, and "fixing"
it unilaterally would desync the Sorcerer from the Wizard and require a re-export + re-import. **Recorded as a
tooling follow-up, not taken this pass.**

### 15b. The Wizard shipped without `bForceRootLock`

`A_Wizard_Idle`/`_Walk` have `bForceRootLock=false`; Footman and Cleric have it `true`. I set the Sorcerer's four to
`true` per instruction, which matches the majority. **Flagging that the Wizard is the odd one out** — likely a
missed step in TASK-302's import, and a candidate for the same follow-up.

### 15c. Import Normals

`SkeletalMeshTools.import_file` exposes **no normals-method parameter**, so "Import Normals (not compute)" could not
be set explicitly. The FBX carries face smoothing (`mesh_smooth_type="FACE"`), and this is the identical tool and
call shape used for all 12 shipped units, so `SK_Sorcerer` matches the fleet. Stating it plainly rather than
claiming a setting I could not control.

## 16. Files touched in the editor (no Git)

Created + saved: `/Game/Characters/SK_Sorcerer` · `/Game/Characters/Anims/A_Sorcerer_{Idle,Walk,Attack,Death}`.
Modified: none. Deleted: only the four spurious import-artifact SkeletalMeshes, each class-guarded.
`L_Arena` untouched. `SM_Sorcerer`, `MI_Sorcerer_PBR` and the textures from TASK-371 untouched.


---
---

# Section 13 REFINED + A DEEPER CONSEQUENCE FOUND - 2026-08-02, see **`TASK-375-facing-fix.md`**

**Section 13's verdict was correct** (`SK_Sorcerer` back-facing, ~180 degrees from the fleet) and its scope limit -
"establishes rotation **relative to the fleet**, not an absolute world axis" - was the right call. The absolute
cause is the **TASK-348 `_ue_handedness_precomp` MIRROR-FIX (2026-07-28)**; the Sorcerer is the first UNIT
exported after it. Section 13's own reasoning that "SM and SK agreeing is the designed behaviour" holds, and is
why the single source fix corrects both lanes at once.

## THE PART NOBODY HAD SPOTTED - the RIG was inverted too, not just the visual yaw

`rig_character.py` imports **`Content/RawAssets/<CardID>.fbx`** (the Stage-2 file) and hard-codes its own
convention: **line 383 `"Front is -Y"`**, line 399 `foot_fwd  # -Y front`, line 616 *"Character faces -Y, so a
limb swings FORWARD with a NEGATIVE X-angle"*, line 749 `A.loc("root", p2, (0, -0.14*H, 0))  # lunge forward
(-Y)`.

Because the OLD Sorcerer FBX fronted **`+Y`** in that space, the shipped rig had its **toe offset, leg-swing
direction, Attack lunge and `_l`/`_r` bone sides all inverted relative to the mesh.** That is a real
gameplay-visible defect, not a cosmetic yaw, and it would have survived any fix applied at the Blueprint or C++
yaw. **The source fix corrects it for free:** the new FBX fronts `-Y` in rig space, matching the hard-coded
convention exactly like the fleet. Stage 3b was re-run (exit 0, 40.7 s, 21 bones, `auto_heat` 0.00% unweighted,
zero warnings).

**Section 1's `proportions` override is UNCHANGED and re-verified:** a Z rotation cannot alter the antler
inflation factor, and the re-run agrees - `head_top_z` resolves to **172.48 UE** vs the skull apex ~172.42
(**0.03%** shift, purely from the mesh measuring 181.9 rather than 181.81). All 16 keys still active.
`rig_manifest.json` and `rig_character.py` are **byte-untouched** by the fix pass, so Sections 2/3/9's tooling
work stands exactly as shipped.

**Section 14 (the `lod_count == 1` LOD-chain blocker) is STILL OPEN and unchanged** - it was 1 before the fix and
is 1 after, so there is no regression. The fix pass ran two headless commandlets (the one context where it is
cheap) and **deliberately did not close it**: it is this task's acceptance criterion and was escalated for
**Jonathan's decision**, and quietly closing it there would have buried that decision.

**Sections 15a/15b (24 fps clips, Wizard missing `bForceRootLock`) unchanged** - the re-imported clips are
60/30/40/48 frames with root motion OFF and `bForceRootLock` ON, matching what this task shipped.
