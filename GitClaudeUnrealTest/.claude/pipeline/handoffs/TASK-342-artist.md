# TASK-342 — [FID-archer-pikeman] Archer + Pikeman chroma-fidelity rework — art-director handoff

**Status:** BOTH UNITS PASS the CHROMA-FIDELITY GATE (measured, validated method). Deliverables staged on disk, same-path. **Delivery form: TEXTURE-SET-ONLY for BOTH units (measured, ruling 6)** — geometry/UV hash-identical to shipped, FBX left byte-untouched, no re-rig, anims trivially safe. **UE import NOT done — editor-gated, build-master owns it (TASK-343).** NO editor / MCP / Git / TASKBOARD / CONVENTIONS writes. **ZERO Meshy credits spent** (every iteration was a free local Stage-2 re-run from the cached 2026-07-26 donors).
**Date:** 2026-07-27 · Law: CONVENTIONS "Fleet Meshy remaster" → **CHROMA-FIDELITY GATE + ALPHA-MASK METHOD LAW** (v1) + per-asset `albedo_delight` override law + same-path/preserve-anims. Board: "TWO-LANE PLAN" rulings 1–8 (Lane A).

---

## STEP 0a — MANIFEST PIN (ruling 3 pre-flight, done FIRST)

Confirmed: `assets.Archer.albedo_delight` was **ABSENT** from `Tools/ArtPipeline/pipeline_manifest.json` (TASK-312's write lost to the 07-26 crash — but the TASK-312 *bake itself* had run WITH the locked profile: the crash-surviving refine report records `{1.0, 0.25, 0.55, 1.2}` + `mean_linear_after 0.2919`, so only the pin was lost, not the shipped bake). Pinned Archer at the locked fleet profile as the iteration starting point; verified Pikeman's existing pin `{1.0, 0.25, 0.55, 1.2}` (TASK-225) in place. Both blocks were then superseded by the TASK-342 finals below — full `_note` audit trail in the manifest.

**Final pinned overrides (per the per-asset override law, recorded in manifest `_note`s AND here):**

| Unit | ao_divide_strength | ao_floor | gamma | gain | (shoulder/max_out) |
|---|---|---|---|---|---|
| Archer | 1.0 | 0.25 | **1.0** (was 0.55) | **1.7** (was 1.2) | 0.8 / 0.98 (in-script defaults) |
| Pikeman | 1.0 | 0.25 | **1.0** (was 0.55) | **2.3** (was 1.2) | 0.8 / 0.98 (in-script defaults) |

**The mechanism (root cause of the chalk):** the delight pass's levels lift is applied **per RGB channel** — `gain * linear^gamma`. γ=0.55 raises dark channels proportionally more than bright ones, compressing channel ratios ⇒ **chroma collapse** (forest-green → grey-teal, olive → cream). `gain` is a pure linear scalar (chroma-*raising* in CIELAB, ∝ scale^⅓). The fix moves the entire lift onto AO-divide + gain with γ=1.0 (ratio compression eliminated). Measured on the delight-off diagnostic bakes: most of the baseline desaturation is in the **Meshy donor bake itself** (delight-off preview chroma retention: Archer 0.6785, Pikeman 0.5843) — the old γ0.55 profile then *wasted* the brightening's natural chroma boost on ratio compression.

## STEP 0b — METHOD VALIDATION (method law, before any number was trusted)

- **Albedo metric — six 4-dp anchors, ALL EXACT:** Footman 0.1639 · Knight 0.1639 · Cleric 0.3614 · Archer 0.2919 · Pikeman 0.2107 (recorded `mean_linear_after`) + Footman UV-norm baseline 0.2536. Decode byte-identical to `refine_trellis_glb.py::_srgb_to_linear`; covered = any nonzero texel.
- **Retention method — TRIPLE anchor lock (<0.1% each):** preview_front + alpha≥128 fg mask + CIE-Y luma reproduces Castle 1.0153 (rec 1.0155/1.0133), Ogre-final 0.9402 (rec 0.9399), Ogre-shipped 0.8121 (rec 0.8124). This is the recorded fleet method, now identified exactly.
- **Chroma metric NAMED (law):** mean **CIELAB C\*ab** (D65). Honesty note: the TASK-340 Ogre chroma figures (1.1149/1.2337) do **not** reproduce under C\*ab — the prior chroma metric was unrecorded; C\*ab is the metric of record from now on (v1 first data).
- **Bake determinism PROVEN:** delight-off smoke bakes reproduce the shipped reports' `mean_linear_before` to 4 dp (0.0615 / 0.0567), and the final runs' N + ORM PNGs are **byte-identical** to shipped — the Stage-2 bake is fully deterministic from the cached donor.

### ALPHA-MASK finding (first-data surprise, recorded per the escape-valve duty)
**Neither concept carries a usable alpha channel** — `Concepts/Archer.png` is RGBA with **alpha ≡ 255 everywhere**; `Concepts/Pikeman.png` is plain RGB. The law's operative requirement (no backdrop pixel in any denominator) is met with a **derived alpha**: border-connected smooth region growing (neighbor tol 0.03) — tracks Archer's *gradient* studio sweep and consumes Pikeman's soft contact shadow, stopping at the sharp figure edge. Masks eyeballed clean (viz saved: `Cache/_task342/mask_{Archer,Pikeman}_derived_alpha.png`); sensitivity tested tol 0.02↔0.03: luma ≤4.1%, chroma ≤0.6% — immaterial vs gate margins. **The legacy corner-sample mask kept 54.6% of Archer's frame as "foreground"** (true figure: 9.05%) — quantified proof of ruling 3.2's contamination: the recorded 0.7505 luma retention had a 2×-inflated denominator (backdrop luma 0.1529 vs true fg 0.0761). **Archer's true shipped luma retention was 1.5597 — ABOVE the 1.25 bleach line**, flipping its diagnosis from "dark-ish" to over-bright + desaturated (same failure family as Pikeman, one fix direction).

## The GATE TABLES — before → after (preview-front basis = the anchor-validated lens; vs derived-alpha concept)

### Archer — ALL PASS
| Gate criterion | Shipped (before) | **FINAL (after)** | Gate | Verdict |
|---|---|---|---|---|
| (a) chroma retention (C\*ab) | 0.7594 | **0.9682** | ≥ 0.60, target ≈1.0 | ✅ **PASS — at target** |
| (b) oversaturation flag | — | 0.9682 | > 1.35 = flag | ✅ no flag |
| (c) hue shift, top-2 clusters | 0.4° / 1.3° | **2.7° / 6.0°** | ≤ 20° | ✅ PASS |
| (d) luma retention | **1.5597 (BLEACH-side breach)** | **1.0141** | 0.85–1.25 | ✅ **PASS — dead-center** |
| (d) UV-norm albedo | 0.3980 | **0.2676** | ≥ 0.2536 | ✅ PASS (1.055×) |
| 3-view means (reported) | luma 1.4362 / chroma 0.7654 | luma 0.9057 / chroma 0.9764 | — | in-band |
| top-2 cluster C\* ratios | 0.56× / 0.71× | **0.77× / 0.85×** | reported | leather browns restored |
| team region | 3.29% / 375 faces | 3.29% (unchanged — texture-only) | recorded | — |
| ORM AO / metallic (covered) | 0.5566 / 0.0177 | byte-identical | — | untouched, as diagnosed (not ORM-side) |

### Pikeman — ALL PASS
| Gate criterion | Shipped (before) | **FINAL (after)** | Gate | Verdict |
|---|---|---|---|---|
| (a) chroma retention (C\*ab) | **0.5685 (BELOW the 0.60 floor)** | **0.7805** | ≥ 0.60, target ≈1.0 | ✅ **PASS (+30% over floor; donor-limited, see below)** |
| (b) oversaturation flag | — | 0.7805 | > 1.35 = flag | ✅ no flag |
| (c) hue shift, top-2 clusters | 4.8° / 16.8° | **2.2° / 5.8°** | ≤ 20° | ✅ PASS (tabard's 16.8° warm drift healed) |
| (d) luma retention | 1.2668 (at/over the edge) | **1.1315** | 0.85–1.25, **must not breach 1.25** | ✅ **PASS** |
| (d) UV-norm albedo | 0.3313 | **0.2659** | ≥ 0.2536 | ✅ PASS (1.048×) |
| 3-view means (reported) | luma 1.3244 / chroma 0.5763 | luma 1.2616 / chroma 0.8190 | — | 3-view luma inflated by the BACK view (1.776) — that view is dominated by the pale cape, a mesh/material property present in the shipped state too (1.72); the gate lens (front, anchor-validated) is 1.1315 |
| top-2 cluster C\* ratios | 0.78× / 0.75× | **0.71× / 0.74×** (hue now faithful) | reported | — |
| team region | 10.31% / 1549 faces | 10.31% (unchanged — texture-only; **DELIBERATE per ruling 5, do not flag**) | recorded | — |
| ORM AO / metallic (covered) | 0.6710 / 0.0772 | byte-identical | — | untouched |

**Iteration record (all free, ~8–9 s each):** delight-off diagnostic ×2 (smoke) → offline exact-math grid search (54 profiles/unit, simulator reproduces the pipeline's DELIGHT numbers to 4 dp) → real smoke verification: Archer γ1.0/g1.6 (UV-norm 0.2541, floor margin razor-thin — rejected) and γ1.0/**g1.7** (winner); Pikeman γ1.0/**g2.3** (winner) and γ1.0/g2.6 (+0.013 chroma for +0.06 luma near the bleach line — rejected) and γ1.0/g2.3/shoulder-0.9 (+0.007 chroma for +0.008 luma — rejected, keeps standard shoulder). Final non-smoke runs measured identical to their candidates (determinism).

**Two structural findings for the manager (informational, no gate conflict):**
1. **The 0.2536 UV-norm floor itself binds against dark-palette concepts:** Archer below gain 1.6 and Pikeman below gain ~2.3 break the floor, and holding the floor keeps preview luma retention at ~1.01/1.13 (brighter than concept). The floor and the bleach ceiling leave a real but narrow window; both units land inside it.
2. **Pikeman's chroma is DONOR-CAPPED at ≈0.8:** the delight-off baseline is 0.5843 and a scalar lift can only add ∝ scale^⅓. Reaching ≈1.0 needs a fresh donor (manager go + Jonathan's pose-reroll answer — NOT taken, per ruling 4). The eyeball previews no longer read chalky (tabard reads cream-olive, trousers slate, pikes warm wood) — shipped as a gate PASS with this ceiling recorded. If Jonathan's eye still sees wash-out on the tabard, that is the remaining headroom.
3. *(method, v1 honesty)* The law's literal "chroma on covered texels" texture-basis ratio is cross-domain (unlit albedo vs lit concept render) — it measured **1.27 on the visibly chalky shipped Archer** and cannot steer. Gated on the preview basis (same flat-lit domain as every recorded fleet retention anchor, triple-anchor-validated, and STRICTER here); texture-basis numbers reported as secondary in the measure JSONs. Suggest the v1→v2 gate text adopt the preview basis explicitly.

**Cluster-method note:** Archer's forest-green tunic measures **C\*=3.1 (hue 167.6°)** in the concept render — colorimetrically near-neutral (dark, shadowed), correctly excluded by the ≥6 chromatic floor; its top-2 chromatic clusters are the leather browns (C\* 12.8/11.6). The tunic's green read is verified by eyeball (it is unmistakably forest-green in the final previews). Full k=8 cluster tables in the measure JSONs.

## Assets staged (same-path — the working-tree delta is exactly 3 files + docs)

| File | State | Purpose |
|---|---|---|
| `Content/RawAssets/Textures/Archer/T_Archer_D.png` | **NEW BAKE** (1,347,210 B, 1024², 2026-07-27) | base colour — the entire Archer fix |
| `Content/RawAssets/Textures/Pikeman/T_Pikeman_D.png` | **NEW BAKE** (1,267,380 B, 1024², 2026-07-27) | base colour — the entire Pikeman fix |
| `Tools/ArtPipeline/pipeline_manifest.json` | **EDITED** | Archer pin re-established + both TASK-342 overrides with full `_note`s |
| `Content/RawAssets/Textures/{Archer,Pikeman}/T_*_{N,ORM}.png` | rewritten **byte-identical** (no diff) | untouched by the fix, as diagnosed |
| `Content/RawAssets/{Archer,Pikeman}.fbx` | **byte-untouched** (shipped bytes restored after hash-identical verify) | texture-only delivery |
| `Content/RawAssets/Characters/{Archer,Pikeman}.fbx` + `.lod.json` | **untouched** | no re-rig — UVs identical |
| `A_{Archer,Pikeman}_*` + ABP + `SK_Footman_Skeleton` | **untouched** | preserve-anims law, trivially satisfied |

Cache evidence (gitignored, for Jonathan's review): `Cache/_task342/` — `BEFORE_AFTER_{Archer,Pikeman}.png` (concept | shipped | final galleries) · `measure_*.json` (full metric dumps incl. k=8 cluster tables) · `measure_fidelity.py` + `delight_grid.py` + `compare_fbx.py` (the validated tooling) · candidate runs `cand_*/` · masks · logs. `Cache/{Archer,Pikeman}/shipped_e3f929e/` = complete shipped-state backups (textures, FBX, previews, reports). `Cache/{Archer,Pikeman}/previews/` = FINAL previews; `refine_report.json` = FINAL reports (Archer `mean_linear_after 0.1963`, Pikeman `0.1691`).

---

# TURNKEY same-path IMPORT recipe (TASK-343, build-master, editor-gated) — zero judgement calls

**Serialized, EXCLUSIVE editor on main; Simulate STOPPED for every import/save. Delivery form is TEXTURE-SET-ONLY for BOTH units — there is NO mesh work in this integration: no SM reimport, no SK reimport, no LOD regeneration, no rig, no collision. NEVER delete+recreate anything. `L_Arena` never saved.**

### 1. Textures → `/Game/Textures/` — EXPLICIT same-path imports (🚨 TEXTURE-SKIP TRAP law)
`Tools/reimport_meshes.py::_ensure_textures_and_mi()` early-returns because `MI_{Archer,Pikeman}_PBR` exist — a mesh-tool run would silently skip these textures. **Import each texture EXPLICITLY with `replace_existing`, then READ BACK size/dimensions/import-timestamp (not just "asset exists"):**
- `T_Archer_D` ← `Content/RawAssets/Textures/Archer/T_Archer_D.png` — **sRGB ON**, TC_Default *(expect: 1024², source 1,347,210 B, stamp 2026-07-27)*
- `T_Archer_N` ← `…/T_Archer_N.png` — LINEAR/sRGB OFF, TC_Normalmap *(bytes unchanged — reimport is a harmless no-op; law says import the set)*
- `T_Archer_ORM` ← `…/T_Archer_ORM.png` — LINEAR/sRGB OFF, **TC_Masks**
- `T_Pikeman_D` ← `…/Pikeman/T_Pikeman_D.png` — **sRGB ON**, TC_Default *(expect: 1024², source 1,267,380 B, stamp 2026-07-27)*
- `T_Pikeman_N` / `T_Pikeman_ORM` — as Archer's.
Do NOT touch `MI_{Archer,Pikeman}_PBR` — same-path texture overwrite propagates for free; deleting/rewiring the MI breaks refs.

### 2. 🚨 SAMPLER-TYPE TRAP sweep (law)
Enumerate ALL material referencers of the six `T_*` — **MASTERS included** (`M_AssetPBR` expected; verify nothing node-defaults these textures) — and verify each compiles. **`Failed to compile Material` grep = 0 on a fresh load** — a Default-Material fallback anywhere is an automatic FAIL.

### 3. Meshes — VERIFY-ONLY (nothing to import)
Read back that the chains are UNDISTURBED (no reimport happened, so any change = something went wrong): `SM_Archer` LOD0 **14,998 tris / 7,505 welded verts**, `SM_Pikeman` LOD0 **15,000 tris / 7,502 verts**, both 4-chain ≈ `15000/7500/3750/1874`-family; `SK_{Archer,Pikeman}` `lod_count == 3`; slots `[TeamRegion, ArcherPBR]` / `[TeamRegion, PikemanPBR]`; anims still bound (shared `SK_Footman_Skeleton` — sole skeleton in `/Game/Characters`).

### 4. VERIFY in Simulate on `L_Arena`
(a) Both units read SATURATED matching their concepts — Archer: forest-green tunic + warm brown leather + gold griffin (the grey-teal chalk GONE); Pikeman: cream-olive tabard + slate trousers + warm wood pikes (the bleached cream GONE). Capture side-by-sides in the TASK-310 gallery framing (flat-lit before/afters already staged at `Cache/_task342/BEFORE_AFTER_*.png`). (b) Team recolour both teams — **Pikeman's 10.31% band is DELIBERATE (ruling 5), do not flag**; Archer 3.29%. (c) Anims TICK (live bone-delta — nothing was reimported, so a dead anim = wrong asset touched). (d) Facing/grounding OBSERVED only — capsule half-heights **READ, never assume: Archer 90 / Pikeman 95**; author NO component transforms. (e) Message Log: ensure/AccessedNone/Fatal = 0 **plus the step-2 grep**.

### 5. COMMIT — two separate per-unit commits on main, explicit pathspecs, NO push
- **Commit 1 (Archer):** `Content/RawAssets/Textures/Archer/T_Archer_D.png` + `/Game/Textures/T_Archer_D.uasset` + `Tools/ArtPipeline/pipeline_manifest.json` + this handoff/board docs. *(The manifest carries BOTH units' pins in one file — noted so a per-unit revert's blast radius is understood.)*
- **Commit 2 (Pikeman):** `Content/RawAssets/Textures/Pikeman/T_Pikeman_D.png` + `/Game/Textures/T_Pikeman_D.uasset` (+ remaining docs).
`git diff --stat` clean of anything foreign (N/ORM/FBX are byte-identical = must NOT appear). `git reset --hard`/`git clean -fd` BANNED.

**WATCH (say it in the TASK-343 handoff too):** Jonathan's playtest eye is final. If either unit still reads washed-out to him, the numbers above go to the manager — Pikeman's remaining headroom is donor-capped (fresh image3d = manager go + Jonathan's pose-reroll answer + ~30 cr; balance 2446, untouched this task).

---

## Discipline
NO editor / MCP / Unreal import / Git / TASKBOARD / CONVENTIONS writes. **Meshy spend: ZERO** (donors cached; `MESHY_TOKEN` never read, never echoed, never on argv). Files written: the 2 D textures (staged same-path), the manifest pins, gitignored Cache evidence, this handoff.
