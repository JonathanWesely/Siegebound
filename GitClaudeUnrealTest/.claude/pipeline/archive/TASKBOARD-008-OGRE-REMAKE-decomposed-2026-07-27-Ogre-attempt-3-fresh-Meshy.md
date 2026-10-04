<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## OGRE-REMAKE (decomposed 2026-07-27) — Ogre attempt #3: fresh Meshy image-to-3D, credits approved (TASK-340..341)

**Directive (Jonathan, verbatim, 2026-07-27, in Claude Code):** *"the ogre are now correctly facing forward, but they are still way too dark, did you do the meshy rework for them? If not, then please do so, the concept art for them looks good, but the mesh may need to be remade entirely using meshy."*

**The honest history (why this is attempt #3 and why the floor below is the point):** the Ogre HAS already been Meshy-rebuilt — TASK-316, commit `42d2ab2`, full same-path SM/SK/MI/tex overwrite through the LOCKED fleet brightness profile — and STILL shipped as the batch's weakest: raw albedo **0.1416** (lowest of eleven; UV-normalised ≈**0.21**, BELOW the amended 0.2536 floor), team region **1.33%** vs TASK-194's shipped 2.1%. Before that, the TRELLIS-era Ogre was the recorded ACCEPTED-DARK asset (TASK-150). Jonathan has now confirmed with his own eye that the remaster result is still too dark in play. **Re-running the same profile a third time and hoping is not a plan — attempt #3 passes a numeric floor the shipped asset FAILS, or it does not ship.**

### Manager rulings (binding for TASK-340..341)

1. **TASK-324 is SUPERSEDED — closed, both levers folded into TASK-340, nothing dangling.** Jonathan's directive IS his verdict on the TASK-310 colour call (c) and exceeds 324's scope (fresh generation + credits vs 324's cached-GLB/no-credits fence). Lever 1 (team region → ≈2.1% on deliberate geography) carries into TASK-340's acceptance verbatim; lever 2 (colour lift) is subsumed by the stronger floors below. Recorded on the TASK-324 block and the TASK-310 (c) item.
2. **The CONCEPT IS APPROVED AS-IS** (Jonathan: "the concept art for them looks good") — `Content/RawAssets/Concepts/Ogre.png` feeds Meshy unchanged. NO concept regeneration, NO style drift, and the concept is the ANCHOR of the acceptance (retention gates below).
3. **Fresh Meshy spend is APPROVED** — image-to-3D ≈30 credits of the ~2506 balance. Quota exhaustion ⇒ exit 3 ⇒ 🚨 Blockers + PAUSE, never fake, never silently substitute the cached GLB (that was TASK-324's lane and it is superseded).
4. **MEASURE-FIRST, NON-BLOCKING (diagnose-first culture, scoped to minutes).** TASK-340 opens with a measurement of the SHIPPED `T_Ogre_*` — where the darkness actually lives (bake albedo vs ORM/AO/metallic vs team region) — because this is attempt #2-going-on-#3 and the numbers must steer Stage 2. It does NOT gate the generation: Jonathan has ruled the remake happens regardless. There is no stop-on-contradiction here (unlike TASK-329) — a surprising measurement changes HOW Stage 2 is tuned, not WHETHER the remake runs.
5. **PER-ASSET `albedo_delight` OVERRIDE PRE-AUTHORIZED (new law, CONVENTIONS "Fleet Meshy remaster").** The locked fleet profile `{ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` is the DEFAULT, not a ceiling — it is now PROVEN insufficient for this palette (it produced the 0.1416 Ogre). The art-director tunes the Ogre's OWN manifest `albedo_delight` block as needed to pass the floor — per-asset, recorded, never a silent global change — inside the anti-bleach guard.
6. **THE ACCEPTANCE FLOOR (what makes a third dark Ogre impossible — measure, don't eyeball):**
   - (a) **UV-normalised mean linear albedo ≥ 0.2536** (the amended fleet floor, binding on rebuilds, units and buildings alike; the SHIPPED Ogre FAILS it at ≈0.21, so a same-as-before bake CANNOT pass). Raw is reported, never a fail reason.
   - (b) **Luma retention vs `Concepts/Ogre.png` in 0.85–1.25×** (target ≈1.0) — not darker than the approved concept, not bleached; >1.25× retention AND >0.60 UV-norm = BLEACH FLAG (temper `gamma 0.65 / gain 1.0`, don't ship on numbers).
   - (c) **If (a) and (b) genuinely conflict** (an intrinsically dark approved palette making both unreachable), **the CONCEPT gate (b) governs — STOP and flag the manager with the numbers**; do not ship either extreme.
   - (d) Chroma retention vs the concept reported (Cleric 0.28× reference; secondary/non-gating per standing law — but the Ogre was the "weakest colour read", so quote it prominently).
   - (e) **Team region 1.7–2.5% coverage (target ≈2.1%)** on deliberate armour/cloth geography, no silhouette striping (the TASK-086 failure mode). ← the folded TASK-324 lever.
7. **FACING AND GROUNDING ARE CLOSED — fence.** Jonathan's own directive confirms facing is fixed (TASK-326..328/334, `aad4b08`/`390aff3`). Neither task authors ANY component transform: yaw (−90) and Z are C++-derived (CONVENTIONS "Unit mesh facing" + the TASK-307 grounding); the Ogre capsule half-height is **145** — never assume 90 in any measurement. TASK-341 merely OBSERVES correct facing/grounding in Simulate.
8. **Everything that made the remaster batch safe carries over unchanged:** same-path overwrite (never delete+recreate), shared `SK_Footman_Skeleton` + all four `A_Ogre_*` + ABP PRESERVED (a new skeleton asset appearing is a HARD FAIL — ruling 3 of the batch, corrected 2026-07-26), both LOD chains reapplied post-import with FULL per-LOD readback (lane-knowledge 9), the SAMPLER-TYPE TRAP compile sweep after any texture reimport, Simulate-stopped imports (lane-knowledge 8), ONE commit on main, NO push.
9. **No QA-reviewer task is owed** (no code); the gate is TASK-341's integration check. **WATCH: Jonathan's eye at his next playtest is the final authority** — if attempt #3 STILL reads dark to him, the numbers come back to the manager for adjudication, not a fourth silent re-run.

**Dispatch map:** **TASK-340 is dispatchable NOW and is FULLY HEADLESS** — it runs in parallel with the entire editor lane (TASK-332/339/337/338/333). **TASK-341 (blocked-by 340) joins the exclusive-editor queue** — serialize with every other editor task and Jonathan's PIE; exact interleaving vs 339/337/338/333 is the orchestrator's call.

#### TASK-340 — [OGRE-remake] Measure the shipped darkness → fresh Meshy image-to-3D from the APPROVED concept → Stage-2 vs the floor → re-rig + turnkey recipe (art-director, NO editor)
- assignee: art-director
- status: **done** (2026-07-27 — integrated at TASK-341's commit, build-master. Art record: ALL FLOORS PASS: UV-norm albedo 0.2420→**0.2983** [≥0.2536, 1.18×] · luma retention 0.8124→**0.9399** [∈0.85–1.25] · chroma 1.2337 · team region 1.33%→**2.36%**/317 faces on pauldron/shoulder geography · bleach NO · eyeball PASS vs concept. Fresh image3d 30 cr [balance 2446], per-asset override `gamma 0.53 / gain 1.26` + z-band 0.62–0.86 recorded in manifest. Rig: `Footman_Rig` 21 bones shared-skeleton, 0.00% unweighted, `A_Ogre_*` byte-untouched. Turnkey recipe in `handoffs/TASK-340-artist.md`)
- blocked-by: none
- parallel-safe: **yes** (runs concurrently with any editor task)
- spec: >
    **STEP 0 — MEASURE THE SHIPPED STATE (minutes; steers Stage 2, does NOT gate the remake — ruling 4).** Validate the measurement
    script against recorded values first (method law: Footman 0.1639 / Knight 0.1639 / Cleric 0.3614), then quote in the handoff:
    (a) shipped `T_Ogre_D` mean linear albedo RAW (recorded 0.1416 at TASK-316 — confirm) + **UV-NORMALISED** + UV coverage
    (UV-norm expected ≈0.21 vs the 0.2536 floor — confirm the shipped asset FAILS the floor, that is the "before" evidence);
    (b) p99 + clamp headroom (0.22% recorded); (c) luma + chroma retention vs `Content/RawAssets/Concepts/Ogre.png` AND the concept's
    OWN mean luma/palette (is the approved palette intrinsically dark, or did the bake lose light? — this decides how hard ruling 5's
    override must push and whether ruling 6(c) is in play); (d) the ORM channels — AO mean + metallic mean (an over-dark AO or spurious
    metallic darkens the unit in-engine INDEPENDENT of albedo; if the darkness is materially ORM-side, the new bake must fix THAT, not
    just D — say so explicitly); (e) team-region coverage (1.33% recorded).
    **STEP 1 — FRESH MESHY GENERATION (≈30 credits, APPROVED — ruling 3):** `Tools/ArtPipeline/meshy_generate.py --mode image3d` from
    `Content/RawAssets/Concepts/Ogre.png` UNCHANGED (ruling 2 — concept approved as-is). `MESHY_TOKEN` per TASK-197; exit 3 ⇒
    🚨 Blockers + PAUSE.
    **STEP 2 — Stage-2 refine, UNIT path (the TASK-316 recipe):** cleanup → conform → remesh **≤15k tris** → UV `UVMap` → bake
    **D/N/ORM 1024²** → two-slot TeamRegion split (**coverage per ruling 6(e): 1.7–2.5%, target ≈2.1%, deliberate geography, no
    striping**) → ≤4 hulls → FBX; **feet-centre origin, Nanite OFF**. START from the locked fleet profile; **apply the PRE-AUTHORIZED
    per-asset `albedo_delight` override (ruling 5) as needed** — record the exact values in `pipeline_manifest.json` (the Ogre's own
    block) + the handoff. **GATE ON RULING 6 IN FULL: (a) UV-norm ≥ 0.2536 · (b) concept luma retention 0.85–1.25× · (c) conflict ⇒
    concept governs + STOP and flag manager · (d) chroma reported · (e) team coverage.** A bake that fails (a) or (b) does NOT go into
    the handoff as deliverable — iterate the override (Stage-2 re-runs are free) or flag.
    **STEP 3 — RE-RIG headless (the Stage-B recipe):** `Tools/ArtPipeline/rig_character.py` onto the SHARED **`SK_Footman_Skeleton`**
    (there is NO bespoke Ogre skeleton — batch ruling 3 as corrected 2026-07-26; read the rig report's TOP-LEVEL `skeleton` field ONLY,
    never `armature.skeleton`). Ogre's tall/nonstandard proportions carry the known height handling (floor 40 × height/1.75). The
    existing `A_Ogre_{Idle,Walk,Attack,Death}` + ABP are PRESERVED — nothing here touches them. Deliver the rigged FBX.
    **STEP 4 — TURNKEY IMPORT RECIPE** in `handoffs/TASK-340-artist.md` so TASK-341 runs with zero judgement calls: exact source →
    same-path destinations, sRGB flags (`_D` ON, `_N`/`_ORM` LINEAR), slot order `[0 TeamRegion, 1 OgrePBR]`, both LOD chains + the
    expected per-LOD counts to read back, the STEP-0 vs new-bake measurement table, and the manifest override values used.
    **NO editor, NO MCP, NO Unreal import, NO Git** (TASK-341 owns those). Post the numbers + a flat-lit preview in 🎨 Art.
- names: >
    Same-path targets (art produces SOURCES): raw `Content/RawAssets/Characters/Ogre.fbx` (overwrite) → `/Game/Meshes/SM_Ogre`,
    `/Game/Characters/SK_Ogre`, `/Game/Textures/T_Ogre_{D,N,ORM}`, `/Game/Materials/Instances/MI_Ogre_PBR` (master `M_AssetPBR`).
    Skeleton **`SK_Footman_Skeleton`** (shared, sole). PRESERVED: `/Game/Characters/Anims/A_Ogre_{Idle,Walk,Attack,Death}` + ABP.
    Concept `Content/RawAssets/Concepts/Ogre.png` (approved as-is). Cache `Tools/ArtPipeline/Cache/Ogre/`. Manifest
    `Tools/ArtPipeline/pipeline_manifest.json` (per-asset `albedo_delight` override — NEW law). Report `handoffs/TASK-340-artist.md`.
    Law: CONVENTIONS "Fleet Meshy remaster" (same-path + shared-skeleton + preserve-anims + **PER-ASSET `albedo_delight` OVERRIDE**,
    NEW 2026-07-27), "Meshy second engine (M7.5)", "Castle remaster" → the amended ALBEDO ACCEPTANCE FLOOR (UV-norm arm).
#### TASK-341 — [OGRE-remake-int] Same-path SM+SK import + LOD reapply + sampler-law compile sweep + Simulate verify vs the concept + ONE commit (build-master)
- assignee: build-master
- status: **done** (2026-07-27 — build-master; ONE commit on main, NO push. Same-path SM/SK/T_ overwrite: SM 4-LOD **15000/7500/3750/1874** tris, LOD0 17202 verts [no unweld, no 0-tri LOD — MilitiaMob wedge did NOT reproduce]; SK bound `SK_Footman_Skeleton`, census unchanged [NO stray], `lod_count=3` 17395/10719/5836; textures 1024² explicit-imported [skip-trap honored], D sRGB ON / N+ORM LINEAR; sampler sweep: sole referencer `MI_Ogre_PBR`, `Failed to compile Material` = 0; MI byte-untouched. Simulate: yaw −90 / Z −145 C++-derived [capsule READ 145], float 2.15 cm, Blue+Red recolor readback-proven [red band visually subtle in hunched idle — flagged], anims tick [URO confirmed], game-world log clean [2 AccessedNone = pre-existing ABP_Footman TryGetPawnOwner in a transient PREVIEW world — residue]. `L_Arena` never saved. Mid-task editor collision with Jonathan's PIE recorded + texture saves re-verified in a clean session — `handoffs/TASK-341-buildmaster.md`. **WATCH ruling 9 stands: Jonathan's eye is final; if still dark → manager, no fourth silent re-run**)
- blocked-by: TASK-340
- parallel-safe: no (EXCLUSIVE editor + Git; EDITOR-GATED — serialize with TASK-332/339/337/338/333 and never during Jonathan's PIE)
- spec: >
    Exclusive editor session on **main**, **Simulate STOPPED before every import/save** (lane-knowledge 8). Execute the TASK-340
    turnkey recipe:
    **(1) SAME-PATH overwrite, never delete+recreate:** `/Game/Meshes/SM_Ogre` + `/Game/Characters/SK_Ogre` +
    `/Game/Textures/T_Ogre_{D,N,ORM}` + `/Game/Materials/Instances/MI_Ogre_PBR` + raw FBX. `_D` sRGB ON, `_N`/`_ORM` LINEAR, Nanite
    OFF, slots `[0 TeamRegion → MI_TeamColor_<Team>, 1 OgrePBR → MI_Ogre_PBR]`. **SKELETON GATE:** `SK_Ogre` binds the EXISTING
    `SK_Footman_Skeleton`; it must remain the SOLE skeleton in `/Game/Characters` — a new/stray skeleton asset is a HARD FAILURE
    (restore, do not commit). All four `A_Ogre_*` + the ABP untouched and still bound.
    **(2) LOD chains reapplied with FULL readback (lane-knowledge 9):** SK via the TASK-297 recipe (LOD1 50%@0.4 / LOD2 20%@0.15 +
    `OnlyTickPoseWhenRendered` + `bEnableUpdateRateOptimizations`; readback `lod_count == 3` + per-LOD verts) and the SM chain per the
    fleet law — read back **LOD0 welded vert count AND per-LOD triangle counts**; a 0-triangle LOD or `verts == tris × 3` unweld is a
    HARD FAILURE: restore the last-known-good (`42d2ab2`) state, do NOT commit, report (the `SM_MilitiaMob` trap, TASK-322, is still
    open — this is its exact signature).
    **(3) SAMPLER-TYPE TRAP sweep (law):** after the texture reimport, enumerate ALL material referencers of `T_Ogre_*` — MASTERS
    included, not just MIs — and verify each still COMPILES; grep `Failed to compile Material` = 0 hits on a fresh load.
    **(4) FACING/GROUNDING FENCE (ruling 7):** author NO component transform — yaw and Z are C++-owned; capsule half-height is **145**
    (read `GetScaledCapsuleHalfHeight()`, never assume 90). OBSERVE in Simulate: the live Ogre faces its travel direction and feet
    ground ~2.1–2.4 cm, fleet-standard.
    **(5) VERIFY in Simulate on `L_Arena`** (never PIE-in-viewport): (a) the Ogre reads VIVID and matches `Concepts/Ogre.png` — capture
    a side-by-side AND the same framing as the TASK-310 gallery shot so before(0.1416-era)/after is direct; (b) team recolour reads on
    a RED bot Ogre and the team region is visibly present (the 1.33% read was near-invisible); (c) the PRESERVED anims TICK (live
    bone-delta sample, not a screenshot); (d) Message Log clean (ensure / AccessedNone / Fatal = 0) AND the (3) grep clean.
    **(6) ONE COMMIT** on main with explicit pathspecs (`SM_/SK_/MI_/T_Ogre*` uassets + raw FBX + `pipeline_manifest.json` +
    board/CONVENTIONS/handoff), `git diff --stat` shows nothing foreign, **NO push**. `L_Arena` NEVER saved; **`git reset --hard` /
    `git clean -fd` BANNED**.
    **WATCH (ruling 9, say it in the handoff):** Jonathan's playtest eye is final — if attempt #3 still reads dark, route the numbers
    to the manager; no fourth silent re-run. Post readbacks + before/after + commit hash in 🔧 Build & Git.
- names: >
    Same-path: `/Game/Meshes/SM_Ogre` · `/Game/Characters/SK_Ogre` · `/Game/Textures/T_Ogre_{D,N,ORM}` ·
    `/Game/Materials/Instances/MI_Ogre_PBR` · raw `Content/RawAssets/Characters/Ogre.fbx`. Skeleton `SK_Footman_Skeleton` (sole).
    PRESERVED: `A_Ogre_*` + ABP. READ-ONLY: `BP_Unit_Ogre`, `DT_Cards`, `L_Arena`. Commit on main, no push. Law: CONVENTIONS "Fleet
    Meshy remaster" (same-path + SAMPLER-TYPE TRAP + per-asset override), "Unit mesh facing" (fence), M7.6 SK-unit LOD law, the hard
    gate (integration check before commit).

---

