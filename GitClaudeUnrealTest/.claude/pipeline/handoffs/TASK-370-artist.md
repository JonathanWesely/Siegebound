# TASK-370 — [AG-A3] Sorcerer Meshy image-to-3D + Blender refine + the mandatory pre-import gate

**Agent:** art-director · **Date:** 2026-08-01 · **Status: ready-for-integration — GATE PASSED ON EVERY CRITERION.**

Stage 0 housekeeping → Stage 1 (Meshy, 30 credits) → Stage 2 (headless Blender) → the mandatory pre-import gate.
**No UE import (TASK-371), no rigging (TASK-372), no editor, no Git.**

---

## 0. Headline

✅ **Every gate passes, and none is marginal.** UV-normalised albedo **0.4377** (floor 0.2536) at luma retention
**0.9983** (band 0.85–1.25, target ≈1.0) — the floor and the concept gate did **not** conflict, so there was nothing
to escalate and nothing to bleach.
✅ **15,000 tris · `UVMap` · feet-center origin · slots exactly `[TeamRegion, SorcererPBR]`** — all asserted on the
exported FBX, not inferred from the manifest.
✅ **The two-island `TeamRegion` is real:** 168 faces left / 185 right, mirror ratio **0.9798**, **zero faces within
|x|<5**, **zero faces anywhere near the head**.
📐 **THE NUMBER TASK-372 NEEDS: antler `INFLATION FACTOR = 1.0545`** (§6). The concept-stage estimate overstated it —
`neck_top_z 0.865` lands at **0.9121** of true body height, not the predicted ≈0.94.
⚠️ **Two things in the brief were factually wrong and I have measured both rather than argued them** — §7.

---

## 1. Housekeeping — the concept is archived (done first, as instructed)

`Tools/ArtPipeline/Inbox/Sorcerer.png` → `Content/RawAssets/Concepts/Sorcerer.png`, **byte-identical**:

| | |
|---|---|
| sha256 | `bc268ccb7449a98f38658fcb2b428cd28bd8df8b245dab22729e303bb257a0ea` (identical both sides) |
| bytes | 612,298 |
| provenance | roll 5 / **seed 71022**, the accepted TASK-369 candidate |

Every retention metric below measures against this file. Meshy's own `state.json` independently records the same
sha256 for its input, so the mesh provably derives from the archived concept.

---

## 2. TLS — ran bare, and bare was enough

`uv run meshy_generate.py --check` (FREE, always first) **PASSED on the BARE invocation.**
**`api.meshy.ai` did NOT need the CA-bundle triplet** — unlike `router.huggingface.co`, which still does. Norton is
not intercepting this host. Verification was never disabled and no fallback was required.

```
/openapi/v1/balance      - OK (credits remaining: 2416)
/openapi/v1/retexture    - OK      /openapi/v1/image-to-3d - OK
--check PASSED: key valid, API reachable, documented endpoints present.
```

`MESHY_TOKEN` resolved from process env, redacted by the tool, never on argv, never logged. **No 402 at any point.**

---

## 3. Stage 1 — Meshy image-to-3D (the credit spend)

| | |
|---|---|
| task id | `019fc042-8cc4-7d1c-9b8f-28feb63f8f16` |
| params | `latest` / `triangle` / `target_polycount 300000` / remesh + texture + **PBR on** — **byte-identical to the Wizard precedent** |
| **credits** | **30 consumed — balance 2416 → 2386** |
| wall clock | 3 m 48 s |
| output | `Cache/Sorcerer/meshy_raw.glb`, 24,244,868 B, sha256 `a6f74bca6dcf1ef9f1d305f01f58f513b25e76c03f84d331acad941944d2dbaa` |

---

## 4. Stage 2 — headless Blender refine (10.8 s, free, re-runnable)

```
blender.exe --background --python refine_trellis_glb.py -- --card-id Sorcerer --input Cache/Sorcerer/meshy_raw.glb
```
(`--input` is REQUIRED — the script defaults to `trellis_raw.glb`; the Meshy donor is `meshy_raw.glb`.)

| stage | result |
|---|---|
| IMPORT | 221,777 verts / 302,762 tris |
| CLEANUP | → 151,211 verts, **0 islands removed, 0 holes filled** |
| CONFORM | **91.4 × 78.7 × 182.0 UE**, `dims_within_tolerance` **True** on the FIRST run, feet-center |
| REMESH | voxel 1.5 → 71,540 tris → decimated **15,000** (budget 15,000) |
| BAKE | 1024², D/N/ORM (+ R/AO/M debug), 8 spp / 64 spp AO |
| DELIGHT | locked fleet 1.0 / 0.25 / 0.55 / 1.2 — mean linear **0.0646 → 0.3139**, p99 0.9177, 5.9% in shoulder |
| SLOTS | **`[TeamRegion, SorcererPBR]`** — 311 report-faces / **3.08% of area** (cap 35%) |
| EXPORT | `Content/RawAssets/Sorcerer.fbx`, object **`SM_Sorcerer`** |

**No re-run was warranted** — the first bake landed mid-band on every metric (§5). The retuned `target_dims_ue`
would produce a byte-identical result (under `fit_mode: height` only Z is enforced, and Z is unchanged at 182.0).

---

## 5. ⚠️ THE MANDATORY PRE-IMPORT GATE — nothing was imported unseen

**Measurement harness re-validated FIRST** (standing method law): 6/6 albedo anchors reproduce to **4 dp**
(Footman 0.1639, Knight 0.1639, Cleric 0.3614, Archer 0.2919, Pikeman 0.2107, Footman UV-norm 0.2536) and the Ogre
retention anchors to **0.04%**. Only then were any numbers trusted.

### 5a. A real METHOD defect, found and corrected before it could corrupt a number

**The shipped TASK-342 `derived_alpha_mask` default `neighbor_tol=0.03` LEAKS THROUGH the Sorcerer's left granite
shoulder plate** (light grey against a light grey-green backdrop) and flood-fills it as *background* — deleting a
large light region of the figure from the retention **denominator**.

| tol | left plate coverage | right plate | worst backdrop | verdict |
|---|---|---|---|---|
| **0.030 (shipped default)** | **0.100** | 1.000 | 0.0000 | **REJECT — eats the plate** |
| 0.020 / 0.015 / **0.010** | 1.000 | 1.000 | 0.0000 | OK |
| 0.006 | 1.000 | 1.000 | 0.0142 | REJECT — backdrop leaks in |
| legacy corner d>0.06 | 0.999 | 1.000 | 0.0000 | OK |

**Used: `derived_alpha` at `neighbor_tol=0.010`** — the tightest clean value, and its foreground fraction
**0.3377** agrees with the *independent* legacy corner-distance mask **0.3383** to **0.06%** (cross-method
validation). Mask eyeballed (`mask_Sorcerer_USED.png`).
**Quantified bias had this gone unnoticed: the defective mask read the concept 0.9242× — 7.6% too DARK — inflating
every luma-retention number by ~8%.** `Cache/_task342/measure_fidelity.py` was left **byte-untouched** so its
fleet-anchor validation stays meaningful; the re-implementation lives in the scratchpad.

### 5b. The numbers

| gate | value | bar | verdict |
|---|---|---|---|
| **UV-normalised mean linear albedo** | **0.4377** | ≥ 0.2536 | ✅ **PASS** (73% headroom) |
| raw mean linear albedo (reported, never gating) | 0.3139 | — | UV coverage 0.717 (units band 59–73%) |
| **luma retention vs the ALPHA-MASKED concept** | **0.9983** | 0.85–1.25, target ≈1.0 | ✅ **PASS — 0.17% off target** |
| — per view | front 1.1185 · back 0.9976 · 3⁄4 0.8788 | | all three individually in band |
| **anti-bleach guard** (>1.25× AND >0.60 UV-norm) | 0.9983 / 0.4377 | | ✅ **NOT raised** |
| chroma retention (secondary, non-gating) | **0.8596** | ref floor 0.60, oversat 1.35 | ✅ strong (Cleric ref 0.28×) |
| hue shift, dominant concept cluster | **0.75°** | ≤20° (rework gate) | ✅ |
| hue shift, 2nd cluster | 23.1° | | ⚠️ reported honestly — see §7c |
| tris | **15,000** | ≤ 15,000 | ✅ |
| UV layer | **`UVMap`**, `uv_layer_ok: true` | required | ✅ |
| origin | min Z **0.059** UE; X −45.69..45.29, Y −39.21..39.16 | feet-**center** | ✅ centred on both axes |
| **material slots, in order** | **`[TeamRegion, SorcererPBR]`** | exact | ✅ 353 / 14,647 tris |
| ORM | AO mean 0.4773, metallic mean 0.0035 | | ✅ non-metal, sane AO |

> The `texture_basis` luma retention is 3.04× — expected and **not** a gate: an albedo map is unlit by definition,
> so it reads far brighter than a shaded illustration. The fleet gate is the **preview basis** (flat-lit render vs
> concept), which is how the Castle/Ogre anchors were recorded. That is the 0.9983 above.

### 5c. The eyeball — every preview against `Concepts/Sorcerer.png`

All five previews reviewed (front, back, three-quarter, top, Cycles beauty). **Every identity element survived
the round trip:** antler crown · carved stone mask with **no face** · **two** boxy granite shoulder plates with flat
up-facing tops · cream scarf/cowl · moss-green robe with the ragged hem · brown sash · **both hands down at the
planted rune-monolith** · standing with legs visible · grass/rock base.

- **Gate A monolith half, re-checked post-refine as the spec requires:** the monolith top sits at **≈88.5 UE**
  against a shoulder line of **157.9 UE** — barely past waist height, **far** below the shoulders. Comfortable PASS.
- **Gate B (not-a-Wizard), re-checked on the 3D result:** no hood, no beard, no face, no fireball, cool jade/teal
  key vs the Wizard's warm fire, and a wholly different silhouette. Passes decisively.
- **Honest note on first impressions:** the flat-lit Workbench previews *look* pale next to the concept, and my
  first read was "bleached". **That read was wrong** — flat-lit albedo carries no shading, so it cannot look like a
  shaded illustration. The Cycles beauty render (which does shade) sits visibly on the concept's palette, and the
  metric agrees at 0.9983. **I did not tune on the wrong impression.**

### 5d. TeamRegion — asserted on the exported FBX, not on intent

| check | result |
|---|---|
| **two symmetric islands** | left **168** faces `x −38.50..−9.37` · right **185** faces `x 8.63..40.09` |
| mirror symmetry | face balance **0.9081**, mean-\|x\| ratio **0.9798** |
| **centre gap clean** | **0 faces within \|x\|<5**; nearest face \|x\|=**8.63** — neck, scarf knot and jaw all spared |
| genuinely up-facing | mean normal Z **0.7999**; **97.73%** of faces at dot ≥ 0.55 |
| **no head paint** | region z span **127.58..157.87**, topping out **~15 UE BELOW the skull apex (172.42)** — zero contact with mask, dome or antlers (**the Archer bare-HEAD lesson holds**) |
| area | **3.08%** — inside the fleet band (Cleric 2.2 · Ogre 2.1 · Wizard 4.1) |

The top-down preview confirms the two plate tops dominate the RTS silhouette — the best team-colour read on the
roster. **No `helm_dome`** (a stone mask is not a helmet), `max_fraction` 0.35, selectors kept exactly as authored —
**no gate tune was needed.**

---

## 6. 📐 THE ANTLER INFLATION FACTOR — TASK-372's input, MEASURED not guessed

Jonathan's ruling: **KEEP THE ANTLERS**, accept the height overshoot, compensate with a per-asset `proportions`
override in `rig_manifest.json`. This task owed the measurement that override divides by.

**Method.** A vertex Z-slice clustering pass was tried and **rejected** — at 15k tris / 7,496 verts a thin slice
holds 3–50 verts and "largest cluster diameter" is vertex-sampling noise (it returned a wild 21.78%). A silhouette
width rule was also rejected: the tines converge over the centre, and a dome tapers at its own apex, so any width
threshold fires early or late (it returned 7.76%). **What is authoritative is a COLOUR segmentation** — the skull is
pale sea-green, the antlers are tan — restricted to the head column band, on the orthographic previews, which are
self-calibrating (top row ≡ bbox zmax, bottom row ≡ zmin). Front and back agree to **0.3%**, and the result was
eyeballed against a Z-ruled zoom of the head.

| quantity | value |
|---|---|
| total mesh height (bbox Z 0.059 → 181.864) | **181.805 UE** |
| skull dome apex | **172.42 UE** (front 172.19 · back 172.76) |
| **true body height** (feet → skull apex) | **172.42 UE** |
| antler overshoot | **9.39 UE = 5.45% of body** |
| **INFLATION FACTOR** | **1.0545** |
| skull top as a z-fraction of total mesh height | **0.9484** |
| `neck_top_z` 0.865 **uncorrected** lands at | **0.9121** of true body height |
| **corrected `neck_top_z`** | **0.8203** |
| **general rule for TASK-372** | **`corrected_z = nominal_z / 1.0545`** — apply to EVERY `*_z` anchor |

> ### ⚠️ The concept-stage estimate OVERSTATED the problem — this is the correction, and it matters
> TASK-369 recorded **~9.2%** inflation and predicted `neck_top_z 0.865` would land at **≈0.94**.
> **The conformed mesh measures 5.45%, landing at 0.9121.** (My own independent re-measurement of the *concept*
> under the corrected mask gives 4.43%, so the 9.2% figure looks like a skull-top definition difference, not a
> generation change.) Meshy pulled the tines more outward and less vertical than the 2D concept implied.
> **The override is still required — just smaller. TASK-372 must divide by 1.0545, NOT by ~1.09.**
> This is exactly why the ruling asked for a mesh measurement rather than a guess.

---

## 7. ⚠️ Three things worth your eye

### 7a. "Start with the locked fleet delight profile (omit the block)" — the two halves contradict, and I have measured which one to keep

**Omitting the block does NOT give the locked fleet profile.** `refine_trellis_glb.py`'s in-script
`ALBEDO_DELIGHT_DEFAULTS` is the **older CONSERVATIVE profile `{0.6, 0.35, 0.85, 1.0}`** — that is what every
block-less asset actually ran (it is why the Wizard's shipped report records exactly those four values and baked to
0.0968). CONVENTIONS calls the locked profile "the fleet-wide DEFAULT", but **the script does not implement it as
the default** — the only way to start an asset on it is to pin it.

I followed the **named intent** (the locked profile) over the parenthetical mechanism, and then ran the
counterfactual in the `--smoke` sandbox to check I was right:

| start point | profile | raw | **UV-norm** | floor 0.2536 |
|---|---|---|---|---|
| **omit the block** (as literally written) | 0.6 / 0.35 / 0.85 / 1.0 | 0.1291 | **0.1801** | ❌ **FAIL — 29% under** |
| **locked fleet profile, pinned** (shipped) | 1.0 / 0.25 / 0.55 / 1.2 | 0.3139 | **0.4377** | ✅ **PASS** |

**Taking the instruction literally would have failed the albedo floor.** The pinned values are the locked fleet
numbers *verbatim* — **this is not a per-asset tune** and no `albedo_delight` override was needed. Recorded in the
manifest `_note` and here, per the override law. The smoke artifacts are kept as evidence at
`Cache/Sorcerer/smoke/`; the shipped previews/report were backed up to `Cache/Sorcerer/shipped_locked/` before the
smoke run and restored after (smoke mode cannot touch `Content/`, but it does write cache previews).

**Suggested follow-up (manager/tooling, not mine to take):** either make `ALBEDO_DELIGHT_DEFAULTS` the locked
profile, or amend the CONVENTIONS wording — today the doc and the script disagree, and that gap is a live trap.

### 7b. The FBX path in the spec points at the rig lane's file

The board's `names` block and CONVENTIONS:281 say `Content/RawAssets/Characters/Sorcerer.fbx`. **The Stage-2 static
FBX is hardcoded to `Content/RawAssets/<CardID>.fbx`** in `refine_trellis_glb.py`, and that is where all 24 shipped
assets live. `Content/RawAssets/Characters/<CardID>.fbx` is the **RIGGED** FBX written by `rig_character.py`
(TASK-372) — both exist for every rigged unit (e.g. `Wizard.fbx` in both places, plus `Wizard.lod.json`).
**Not a defect and I did not "fix" it by moving the file** (that would break the fleet convention and the reimport
tooling). Recording it so TASK-371/372 are not surprised.

### 7c. The second hue cluster shifted 23.1°

The dominant concept colour cluster tracks almost perfectly (**0.75°**), but the second — the darkest cluster,
Lab L≈24 — shifts **23.1°** and loses chroma (9.4 → 5.63). That is the de-light stage lifting deep shadow greens,
which is what it is for. **The ≤20° criterion binds concept-fidelity REWORK tasks, not ordinary builds**, and every
gating metric passes comfortably, so this is **reported, not escalated**. Flagging it only so that if the Sorcerer
ever reads flat in the shadows at playtest, this number is the first place to look.

---

## 8. Files

**Created / modified (all outside the editor; NO Git operations performed):**

| path | note |
|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Concepts\Sorcerer.png` | accepted concept, byte-identical archive (NEW) |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Sorcerer.fbx` | **Stage-2 static FBX**, object `SM_Sorcerer`, 15,000 tris, slots `[TeamRegion, SorcererPBR]` |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Sorcerer\T_Sorcerer_D.png` | 1024², sRGB source → `T_Sorcerer_D` |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Sorcerer\T_Sorcerer_N.png` | 1024² normal |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\Textures\Sorcerer\T_Sorcerer_ORM.png` | 1024², **LINEAR — sRGB OFF at import** |
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\pipeline_manifest.json` | new `Sorcerer` entry (22 assets, JSON valid, CRLF preserved, 0 bare LF) |

**Cache (gitignored):** `Tools\ArtPipeline\Cache\Sorcerer\` — `meshy_raw.glb`, `state.json`, `refine_report.json`,
`previews\` (5), `bake_debug\`, `shipped_locked\` (pre-smoke backup), `smoke\` (§7a counterfactual evidence).

---

## 9. What TASK-371 (UE import) needs from me

- `SM_Sorcerer` ← `Content/RawAssets/Sorcerer.fbx` → **`/Game/Meshes/SM_Sorcerer`**. Fresh asset, no same-path
  overwrite trap. **Nanite OFF**, `lod_group='LargeProp'`, ≤4 generated hulls, `UVMap`, feet-center.
- Textures **imported EXPLICITLY** (the texture-skip trap): `T_Sorcerer_D` **sRGB ON**, `T_Sorcerer_N` normal,
  `T_Sorcerer_ORM` **LINEAR / sRGB OFF** → `/Game/Textures/`. **Run the sampler-type sweep after import.**
- `MI_Sorcerer_PBR` at `/Game/Materials/Instances/` from `/Game/Materials/M_AssetPBR`, params `BaseColor` /
  `Normal` / `ORM`.
- **Slot order is LAW and already correct in the FBX:** slot 0 `TeamRegion` ← `MI_TeamColor_Blue` (design-time
  placeholder; the BeginPlay recolor overwrites at runtime), slot 1 `SorcererPBR` ← `MI_Sorcerer_PBR`.
- Retention/chroma numbers to quote back: **UV-norm 0.4377 · luma 0.9983 · chroma 0.8596** (§5b).

## 10. What TASK-372 (rig) needs from me

- **`INFLATION FACTOR = 1.0545`** → per-asset `proportions` override: **`corrected_z = nominal_z / 1.0545`**,
  e.g. `neck_top_z` 0.865 → **0.8203**. **Do NOT use the ~9.2% concept figure** (§6).
- True body height **172.42 UE** of a **181.805 UE** total mesh; skull apex at z-fraction **0.9484**.
- Bind to the **EXISTING shared `SK_Footman_Skeleton`** — never create a skeleton. Same two-slot contract.
- The rigged FBX goes to `Content/RawAssets/Characters/Sorcerer.fbx` — a **different file** from the Stage-2
  static FBX in §8 (§7b).


---
---

# RE-RUN 2026-08-02 - see **`TASK-375-facing-fix.md`** (Stage 2 re-run, zero Meshy credits)

**Nothing in this note was wrong, and Section 5's gate work is what made the re-run trustworthy.** Stage 2 was
re-run from the cached `Cache/Sorcerer/meshy_raw.glb` - **11.7 s, ZERO Meshy credits, no Stage-1 regeneration** -
with one manifest change: **`pre_rotate_z_deg: 0.0 -> 180.0`**, to correct a 180-degree facing error that only
manifests *after export* (root cause: the TASK-348 `_ue_handedness_precomp` MIRROR-FIX; the Sorcerer is the first
UNIT exported after it). **This note's conformed-space previews were always correct** - `preview_front.png` showed
the FRONT, matching the Footman's and Cleric's cached previews.

**Section 5a's method defect and its correction were reused verbatim and paid off again:** the mask ran at
`neighbor_tol = 0.010`, and the concept foreground fraction reproduces **0.3377 exactly** - an independent control
that the concept side is unchanged. `Cache/_task342/measure_fidelity.py` was again left **byte-untouched**
(sha256 `68cd0258...` before and after).

**Section 7a's finding was decisive and was honoured:** `albedo_delight` stayed **PINNED** to the locked fleet
values `1.0 / 0.25 / 0.55 / 1.2`. Omitting the block would have fallen back to the conservative in-script default
and failed the floor at 0.1801, exactly as measured here.

| gate | this note | **post-fix re-run** | bar |
|---|---|---|---|
| UV-normalised mean linear albedo | 0.4377 | **0.4403** | >= 0.2536 PASS |
| luma retention vs alpha-masked concept | 0.9983 | **0.9811** | 0.85-1.25 PASS |
| chroma retention | 0.8596 | **0.9292** | >= 0.60 PASS |
| dominant-cluster hue shift | 0.75 deg | **2.55 deg** | <= 20 deg PASS |
| tris / UV / origin / slots | 15,000 / `UVMap` / feet-centre / `[TeamRegion, SorcererPBR]` | **unchanged** | PASS |
| TeamRegion | 3.08%, 2 islands, 0 centre faces, no head paint | **3.12%, 2 islands, 0 centre faces, no head paint** | PASS |
| report warnings | `[]` | **`[]`** | PASS |

**A clean cross-validation that this is the same asset merely rotated:** the front/back per-view retention figures
*swapped roles* - old `front` 1.1185 vs new `back` **1.1211** (0.2% apart), old `back` 0.9976 vs new `front`
**1.0041** (0.7% apart). The three-quarter view legitimately captures the other diagonal now, which accounts for
the whole 0.9983 -> 0.9811 mean shift.

**In conformed/preview space the asset now fronts `+Y`**, so `preview_back.png` shows the character's FRONT and
`preview_front.png` shows its back. **That inversion is the compensation, not a defect - do not reset
`pre_rotate_z_deg` to 0.** The manifest `_pre_rotate_source` note records the full derivation.

**Section 6's antler INFLATION FACTOR 1.0545 is UNCHANGED** - a Z rotation cannot alter it, and the rig re-run
agrees (`head_top_z` -> 172.48 UE vs skull apex ~172.42, a 0.03% shift from the height measuring 181.9 vs
181.81). **Section 7b (static vs rigged FBX paths) and Section 7c (2nd hue cluster 23.1 deg -> 22.17 deg) are
unchanged.**
