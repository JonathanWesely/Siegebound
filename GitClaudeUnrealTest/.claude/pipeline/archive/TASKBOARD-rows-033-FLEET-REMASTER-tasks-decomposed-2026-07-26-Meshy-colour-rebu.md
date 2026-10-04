<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-306 — [REM-float-diag] DIAGNOSE the recurring floating-units bug — systemic root cause + fix spec (gameplay-programmer)
- assignee: gameplay-programmer
- status: **done** (2026-07-26 — root cause: TASK-259 v1 copied the STATIC VisualMesh's BP-authored Z onto SkeletalVisualMesh, merely RELOCATING the per-BP hand-authored-offset trap; `BP_Unit_Wizard`'s static `VisualMesh.Z` was never offset ⇒ 0 propagated ⇒ float ≈ capsule half-height. Fix spec = derive Z from capsule half-height + SK bounds. `handoffs/DIAG-floating-units.md`.)
- blocked-by: none · parallel-safe: yes
- spec: > Diagnose WHY the TASK-259 floating-units fix RECURRED on the Wizard. Establish the SYSTEMIC root cause (capsule-half-height-relative offset rather than a per-class constant) and output the fix SPEC into `handoffs/DIAG-floating-units.md`. No code edit, no editor, no Git. Post in ⚙️ Dev & QA; hand off to TASK-307.
- names: > `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (`ResolveSkeletalVisual`, read-only) · `handoffs/DIAG-floating-units.md`.

#### TASK-307 — [REM-float-fix] Systemic feet-grounding: capsule-relative SkeletalVisualMesh Z-offset (gameplay-programmer, C++ file-only)
- assignee: gameplay-programmer
- status: **done** (2026-07-26 — `ResolveSkeletalVisual()` now derives `L.Z = -ScaledCapsuleHalfHeight - (SK bounds min-Z × RelativeScale3D.Z)`; NO-OP for the already-grounded fleet. Shipped in `0d717c0`.)
- blocked-by: TASK-306 · parallel-safe: yes
- spec: > Replace the fixed `VisualMeshBaseRelativeLocation` Z-offset with a SYSTEMIC capsule-relative one. MUST be a NO-OP for correctly-grounded units and close the recurrence permanently. File-only — no compile/editor/Git; write `handoffs/TASK-307-programmer.md`.
- names: > `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (`ResolveSkeletalVisual`).

#### TASK-307-QA — [REM-float-fix QA] Review TASK-307
- assignee: qa-reviewer
- status: **done — qa-passed** (`qa/TASK-307.md`)
- blocked-by: TASK-307 · parallel-safe: no
- spec: > Pre-compile review: offset is genuinely systemic; genuine NO-OP for already-grounded units; static-fallback path untouched and null-safe; shadow + complete-type-include laws.
- names: Report `qa/TASK-307.md`.

#### TASK-308 — [REM-float-int] Compile the float-fix + verify grounding + commit (build-master)
- assignee: build-master
- status: **done** (2026-07-26 — compiled, grounding verified, committed `0d717c0` on main, NO push. UNBLOCKED all 11 per-unit verifies.)
- blocked-by: TASK-307-QA · parallel-safe: no
- spec: > Compile TASK-307 (code hard-gate), spot-check that a currently-grounded unit stays grounded AND the Wizard's feet now meet the ground, Message Log clean, commit on main with explicit pathspecs, NO push.
- names: > Commit on main, no push. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}`.

#### TASK-309 — [REM-audit] Building/tower colour audit — flag the washed-out ones (art-director)
- assignee: art-director
- status: **done — 2026-07-27, JONATHAN RULED AND THE FINDINGS ARE DECOMPOSED.** ← was: ready-for-integration — AUDIT COMPLETE 2026-07-26 SESSION-4 (audit only, nothing rebuilt, ZERO Meshy credits). Handoff: `handoffs/TASK-309-artist.md`; decisive evidence `handoffs/TASK-309-audit-Castle-in-arena.png` + `-concept-vs-shipped-{A,B,C}.png`. **VERDICT: Jonathan's "the buildings look fine" HOLDS for 8 of 9 — Castle refutes it, severely.**
    **➡️ DISPOSITION (2026-07-27, all three of Jonathan's rulings — full specs in "## BUILDING-AUDIT rulings … TASK-329..333"):**
    (1) **Castle → APPROVED REBUILD**, BUILDING pipeline variant, with an explicit independent RE-VERIFY of the Castle's state before any
    Meshy spend (Jonathan: don't take the audit on trust) → **TASK-329 → TASK-330 → TASK-331** (331 re-derives the crumble meshes — a trap
    this audit did not surface: `SM_Castle_Crumble0N` are byte-copies of the OLD `SM_Castle` and `M_CastleCrumble` samples the same
    `T_Castle_*`, so new UVs scramble the 75/50/25 % states).
    (2) **GoldNode glow → APPROVED DIAL-BACK**, one material lever → **TASK-332 → TASK-333**. The audit's "needs Jonathan's eye first" gate
    is SATISFIED by his ruling. Note the lever is NOT the obvious one — see manager ruling 9.
    (3) **The other EIGHT buildings are CLOSED** — no tasks, do not re-open without new evidence.
    **The two waste-preventing findings and the Wizard incidental are recorded in CONVENTIONS and in the TASK-329..333 block header
    (TeamRegion-not-washout · GoldNode+CrystalTower textures orphaned · `T_Wizard_D` 0.0968 informational-only).** Nothing further is owed
    by this task.

    | # | Building | Verdict | mean linear albedo (raw / UV-normalised) | reason |
    |---|---|---|---|---|
    | 1 | **Castle** | **🚩 REBUILD** | **0.0078 / 0.0653** | Never de-lit at all; renders near-black under real `L_Arena` sun vs a warm sandstone concept |
    | 2 | ArrowTower | OK | 0.0408 / 0.1996 | Grey stone faithful; retention 0.31× at the accepted-unit floor |
    | 3 | Wall | OK | 0.0810 / 0.2451 | **Best of set** — 1.02× chroma retention (zero loss), albedo at baseline |
    | 4 | BombTower | OK | 0.0523 / 0.2314 | Lowest absolute chroma but its concept is too; retention 0.51×, 3rd best |
    | 5 | BallistaTower | OK | 0.0314 / 0.1531 | Warm timber reads best in engine; low albedo is genuine dark wood |
    | 6 | Barracks | OK | 0.0576 / 0.2724 | Above baseline; timber + gold banner read |
    | 7 | DeepMine | OK (watch) | 0.0368 / 0.1596 | Faithful dark rock; retention 0.35× = exactly Knight's accepted value |
    | 8 | CrystalTower | OK | n/a — textures UNUSED | Ships on `MI_CrystalGlow`; cyan glow reads (emissive `0.05/0.6/1.0` @ 12) |
    | 9 | GoldNode | OK (separate flag) | n/a — textures UNUSED | Ships on `M_GoldGlow`; warm-yellow emissive reads but OVER-reads |

    **CASTLE — the one rebuild (→ manager to decompose, BUILDING pipeline variant):** albedo **21× below** the 0.164 Footman baseline (3.9× on the fair UV-normalised number) — **the lowest of every asset in the project, units included, before or after the remaster.** Its `refine_report.json` predates TASK-193 and has **NO `albedo_delight` block at all — the de-light stage never ran on it.** Chroma retention 0.20× is below the worst unit already signed off (Cleric 0.28×). Under real arena lighting it is a black mass darker than the grass. Structural debt on the same asset: `lod_count == 1` (all others 4) and 40k tris.
    **TWO FINDINGS THAT CHANGE HOW THE DATA READS:** (1) **the bleached roofs on ArrowTower/Barracks are the TeamRegion slot painted `MI_TeamColor_Blue`, NOT wash-out** — do NOT commission a rebuild to "restore the red roof"; (2) **CrystalTower and GoldNode never use their baked textures** (orphaned `T_*` sets; both ship on hand-authored glow materials with no BaseColor/Normal/ORM) — a texture rebuild for either would change NOTHING on screen.
    **SEPARATE FLAG (one scalar, not a rebuild) → Jonathan's eye:** GoldNode's `GlowIntensity` blows **86.3% of the mesh past luma 0.85** — a featureless cream blob losing the rock/gold/orange-crack separation; CrystalTower's comparable glow is 6.5% blown and still reads.
    **INCIDENTAL:** `T_Wizard_D` is still on the old conservative delight profile at **0.0968** — the only unit below the 0.164 baseline (Jonathan approved that look at the time; informational).
    **METHOD CONFOUND (carry forward):** building UV coverage is 12–33% vs units' 59–73%, so raw all-pixel albedo is NOT comparable across the two groups — hence the UV-normalised column. The measurement script reproduces the pipeline's own `mean_linear_after` to 4 decimals on every unit, so the metric is validated, not assumed.
- blocked-by: none
- parallel-safe: yes (eyeball inspection of committed assets vs concepts; read-only)
- spec: >
    Audit EVERY building/tower/economy mesh for the same washed-out / colourless defect the fleet units have. For each of
    `ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode, Castle`, eyeball the current in-editor mesh
    (thumbnail/preview render, read-only — no mutation) against its approved concept `Content/RawAssets/Concepts/<Building>.png` and the
    §6 colour bar (GoldNode: does the warm-yellow emissive still read; Castle: the big one). Jonathan thinks the buildings look fine —
    CONFIRM or REFUTE per mesh. Output a FLAGGED LIST (per building: OK / REBUILD, with a one-line reason) into
    `handoffs/TASK-309-artist.md`; post the verdict in 🎨 Art and route the list to the manager. Do NOT rebuild anything here — the
    manager decomposes per-building rebuild tasks (BUILDING pipeline variant) ONLY for the flagged-bad ones.
- names: >
    Read-only over `/Game/Meshes/SM_{ArrowTower,Wall,BombTower,BallistaTower,Barracks,DeepMine,CrystalTower,GoldNode,Castle}` vs
    `Content/RawAssets/Concepts/<Building>.png`. Report `handoffs/TASK-309-artist.md`. Law: CONVENTIONS "Fleet Meshy remaster"
    (audit clause) + "Textured mesh law" (BUILDING path + GoldNode emissive variant) for the follow-up rebuilds.

#### TASK-310 — [REM-close] Batch closeout: confirm 11 commits + assemble the before/after gallery (build-master)
- assignee: build-master
- status: ✅ **done — COMMITTED `bb6df70` (2026-07-26): `TASK-310: re-capture the in-engine fleet verify shots and commit them`.** 📋 **MANAGER FLIP 2026-09-07, ⛔ 43 DAYS LATE, ⛔ ON GIT EVIDENCE** — `handoffs/BOARD-STALENESS-audit.md` §2 (build-master's git-resolved sweep; ⛔ the manager holds ⛔ NO `Bash`, `SC-§71b`). ⛔ **LOGGED EXCEPTION TO WRITE-DISCIPLINE RULE 1** — `status:` is the ⛔ assignee's field; the ⛔ commit host never flipped it, so this row asserted a falsehood for ⛔ 43 days. ⚠️⚠️ **THIS IS THE ⛔ WORST-SHAPED OF THE THIRTEEN: it did ⛔ not merely read *"unfinished"*, it read `backlog` ⛔ PLUS AN EXPLICIT ⛔ *"dispatchable"* — ⛔ a stale row that ⛔ ADVERTISES ITSELF FOR RE-DISPATCH.** ⛔ **⛔ Re-running it would have re-captured and re-committed 11 screenshots for nothing.** ⛔⛔ **A FLIP IS ⛔ NOT A GO — and here the ⛔ FLIP IS SPECIFICALLY A ⛔ STOP: ⛔ do ⛔ NOT dispatch this row.** Marker `BOARD-STALE-FLIP-2026-09-07`. ← was: ~~backlog (all 11 `-verify` blockers now CLEARED — dispatchable)~~
- blocked-by: TASK-311-verify … TASK-321-verify (**all done**)
- parallel-safe: no (editor + Git read; EDITOR-GATED)
- spec: >
    Batch integration checkpoint (the chain-closing build-master task). Confirm all 11 fleet units committed on main as SEPARATE commits
    (one per unit — so any single unit is independently revertible), `git log --oneline` shows them, working tree clean of foreign
    changes, main NOT pushed. Assemble a BEFORE/AFTER gallery for Jonathan: for each unit, the "after" screenshot (from its `-verify`)
    beside its `Content/RawAssets/Concepts/<Unit>.png` (and the old washed-out look if recoverable), so the colour improvement is
    reviewable at a glance; drop the gallery paths + the commit hashes into `handoffs/TASK-310.md`. Post the batch summary + commit-hash
    list in 🔧 Build & Git and flag the manager for Jonathan's review. No push (Jonathan's push after review).
    **AMENDED 2026-07-26 (manager) — CARRY THE THREE COLOUR CALLS TO JONATHAN IN THE GALLERY.** The gallery is the decision point for
    every non-blocking colour judgement returned by build-master; put each one in front of Jonathan explicitly as a yes/no, with the
    lever named, so he answers once instead of the team guessing:
      (a) **Cleric — brightest of the batch** (mean linear albedo 0.361 vs the 0.164 Footman pilot baseline), reads HIGH-KEY under sun.
          Manager ruling: FAITHFUL to its pale-cream concept — no action. Lever if Jonathan disagrees: `gamma 0.65 / gain 1.0` re-bake.
      (b) **Knight — steel darker than the concept's mid-grey.** Manager ruling: CORRECT — a deliberate consequence of a 32.5%-metal ORM,
          verified as genuine metalness with speculars, not a wash-out. Lever if Jonathan disagrees: roughness/metallic **in the bake,
          NEVER albedo** (brightening the albedo of a metal is the wrong fix and would reintroduce the wash-out).
      (c) **Ogre — weakest colour read of the batch, pale under sun; team region 1.33% vs the 2.1% that shipped in TASK-194.**
          Manager ruling: JONATHAN-EYEBALL ITEM, no work pre-authorised — pre-specced and PARKED as **TASK-324**, which is dispatchable
          the moment he says go. Lever = wider team-region z-band + a Stage-2-only re-bake from the CACHED GLB, **NO Meshy credits**.
          **[ANSWERED 2026-07-27 — Jonathan ruled directly in Claude Code without waiting for the gallery: the Ogre is "still way too
          dark" and "the mesh may need to be remade entirely using meshy". That IS the verdict on this item and it EXCEEDS TASK-324's
          no-credits fence ⇒ TASK-324 SUPERSEDED; full Meshy remake chain = TASK-340 → TASK-341 (see "## OGRE-REMAKE" section).]**
    Record his answer per item in `handoffs/TASK-310.md`; a "change it" on (a) or (b) comes back to the manager for a new task.
- names: >
    Read/commit-audit on main, no push. Gallery + hashes in `handoffs/TASK-310.md`. Law: CONVENTIONS "Fleet Meshy remaster"
    (per-unit commit + before/after-gallery clause).

---

#### TASK-324 — [REM-ogre-team] SUPERSEDED: Ogre team-region widen + colour lift (Stage-2-only, NO Meshy credits) (art-director)
- assignee: art-director
- status: **closed — SUPERSEDED 2026-07-27 by TASK-340/341 (manager adjudication of Jonathan's direct directive).** Jonathan's verdict on colour call (c) arrived in Claude Code, not at the gallery, and goes FURTHER than this task's scope: *"they are still way too dark … the mesh may need to be remade entirely using meshy"* — a FULL fresh image-to-3D remake with Meshy credits APPROVED, which obsoletes this task's central constraint (cached-GLB-only, NO credits). **Both of this task's levers are FOLDED into TASK-340's acceptance floor, not dropped:** team-region coverage back to ≈2.1% on deliberate geography (lever 1, verbatim) and the colour lift (lever 2, subsumed by the stronger UV-norm ≥ 0.2536 + concept-retention gates). Nothing else here is owed; do not dispatch. ← was: parked — NOT DISPATCHABLE, requires Jonathan's explicit go at the TASK-310 gallery review.
- blocked-by: n/a (superseded — was: Jonathan's verdict on TASK-310 colour call (c))
- parallel-safe: yes (the Stage-2 re-bake + re-rig are headless; only the UE import is editor-gated)
- **MANAGER RULING — why this is PARKED and not dispatched.** Three reasons, recorded so nobody re-litigates it: (1) it is a purely
  COSMETIC, SUBJECTIVE call on a unit that PASSED its gate and is committed (`42d2ab2`) — "weakest of the batch" is still a pass, and the
  batch's whole review model is per-unit commits so Jonathan can judge each one himself; (2) it is NOT the cheap re-bake it first looks
  like — the team region is a FACE-selection → material-slot assignment, so widening the z-band changes MESH data, which means Stage-2
  re-export **plus** a re-rig **plus** a same-path SM+SK reimport, LOD regen, verify and commit: effectively a fresh 3-stage per-unit chain
  (free of Meshy credits, but a full editor cycle); (3) pre-empting Jonathan's eyeball on a subjective colour call is exactly the kind of
  guessing the gallery exists to stop. It is specced in full HERE so that the moment he says go it is zero-latency — the manager expands it
  into the standard `TASK-324-model` / `-rig` / `-verify` chain against the PER-UNIT TEMPLATE above.
- spec: >
    ONLY on Jonathan's explicit go. Two levers on the Ogre, both cheap and both **REUSING THE CACHED `Cache/Ogre/meshy_raw.glb`** —
    **NO Meshy generation, NO credits, NO re-concept**: (1) **team region** — widen the team-region selector z-band so coverage returns to
    roughly the **2.1%** that shipped in TASK-194 (current rebuild is **1.33%**, the weakest read of the pair); the band must still land on
    deliberate armour/cloth geography, not stripe the silhouette (the TASK-086 striping defect is the failure mode to avoid);
    (2) **colour** — the Ogre bakes at mean linear albedo **0.1416**, the lowest of the eleven and pale under the `L_Arena` sun; if
    Jonathan wants it lifted, temper the locked fleet profile toward more gain (the Footman-validated baseline is 0.164; do NOT exceed the
    p99/clamp headroom — the Ogre currently clamps only 0.22%, so there IS room) and re-bake `T_Ogre_{D,N,ORM}` **same-path**.
    Then the standard chain: re-rig onto the SHARED `SK_Footman_Skeleton` (ruling 3 — there is NO bespoke Ogre skeleton), same-path
    reimport of `SM_Ogre` / `SK_Ogre` / `T_Ogre_*` / `MI_Ogre_PBR`, regenerate LODs, PRESERVE all four `A_Ogre_*` sequences and the ABP
    (do NOT reimport them), then build-master verify + a single commit on main, NO push.
    ACCEPTANCE: team coverage ≈2.1% on deliberate geography, colour reads at or above the 0.164 baseline under sun, anims still bind and
    play, LOD readback correct (SK `lod_count==3`; SM per-LOD triangle counts per lane-knowledge 9), feet grounded, Message Log clean.
    If Jonathan says "leave it", CLOSE this task as `wont-do` and record his call — that is a legitimate, expected outcome.
- names: >
    Same-path: `/Game/Meshes/SM_Ogre`, `/Game/Characters/SK_Ogre`, `/Game/Textures/T_Ogre_{D,N,ORM}`,
    `/Game/Materials/Instances/MI_Ogre_PBR`, raw `Content/RawAssets/Characters/Ogre.fbx`. Skeleton **`SK_Footman_Skeleton`** (shared).
    PRESERVED: `/Game/Characters/Anims/A_Ogre_{Idle,Walk,Attack,Death}` + the ABP. Cached input `Cache/Ogre/meshy_raw.glb` (NO new Meshy
    generation). Law: CONVENTIONS "Fleet Meshy remaster" (same-path overwrite, preserve-anims, shared-skeleton binding law as corrected
    2026-07-26), "Textured mesh law" (two-slot TeamRegion).

