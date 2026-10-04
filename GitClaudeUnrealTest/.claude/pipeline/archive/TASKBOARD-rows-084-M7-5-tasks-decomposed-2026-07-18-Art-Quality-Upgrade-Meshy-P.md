<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-191 — CONVENTIONS: Meshy second-engine law + IK_/RTG_ prefixes + Track-D rejection record (manager)
- assignee: manager
- status: **done** (2026-07-18 — CONVENTIONS "Meshy second engine — Stage-1.5 retexture & image-to-3D alternative (M7.5)" section written + live; IK_/RTG_ prefix rows added; Track-D rejection + milestone ruling recorded in the Milestones entry. Must land before 192/193/198 — it does. This decomposition is the deliverable.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    Add the naming/contract law for the Meshy second engine BEFORE any task issues it: MESHY_API_KEY env-only law, meshy_generate.py
    tool contract + exit codes, Stage-1.5 cache paths (Cache/<CardID>/meshy_retex.glb, meshy_raw.glb), state.json/refine_report.json
    engine-provenance fields, manifest engine/albedo_delight keys, the Stage-2/3 INVARIANT, the A/B fleet-gate law, and the IK
    Retargeter naming (IK_/RTG_ prefixes; retargeted clips same-path-overwrite A_<CardID>_<Action>). Record the Track-D rejection.
- names: >
    CONVENTIONS.md "Meshy second engine — Stage-1.5 retexture & image-to-3D alternative (M7.5)" + prefix-table IK_/RTG_ rows.

#### TASK-194 — pipeline_manifest.json quality pass: 1536³ hero res + `_guess` team-region selector tuning (art, data-only)
- assignee: art-director
- status: **done** (2026-07-18 — hero 1536 pins on Castle/CrystalTower/ArrowTower/BombTower/BallistaTower; all 16 `_guess` selectors adjudicated: 14 verified vs shipped refine_reports+previews, 2 pending-generation (Wall/DeepMine); zero value changes needed; JSON valid; handoff handoffs/TASK-194.md; commit rides TASK-196)
- blocked-by: none
- parallel-safe: yes (single data file; no editor, no Blender; TASK-193 only READS the manifest)
- spec: >
    Track C-3/4, data-only. (1) Pin TRELLIS resolution 1536 for the HERO assets — Castle, CrystalTower, ArrowTower, BombTower,
    BallistaTower — in `Tools/ArtPipeline/pipeline_manifest.json` (applies to FUTURE re-gens only; this task re-runs NOTHING).
    (2) Clear the recorded `_guess` debt: audit every team_region selector still marked `_guess`, tune each against the SHIPPED
    Stage-2 preview renders/refine_reports (the accepted meshes are ground truth for where team accents actually landed), and
    either fix the selector or mark it verified-correct with a dated note. ACCEPTANCE: manifest is valid JSON; hero res pinned;
    zero remaining un-adjudicated `_guess` markers; a short evidence list (per asset: tuned vs verified) in the handoff. Post in
    🎨 Art.
- names: >
    `Tools/ArtPipeline/pipeline_manifest.json` (trellis resolution + team_region selectors; optional albedo_delight overrides
    where an asset needs non-default strength). Law: CONVENTIONS "Textured mesh law", "Meshy second engine (M7.5)".

#### TASK-195 — Track-C validation renders: FLUX-dev concept probe + albedo-lift rebake A/B (art)
- assignee: art-director
- status: **done** (2026-07-18 — both A/B image sets exist: concept pair Cache/Knight/AB_concept/AB_Knight_schnell_vs_fluxdev.png (FLUX-dev probe SUCCEEDED, no license gate; router TLS via proven SSL_CERT_FILE bundle) + delight pairs Cache/Ogre/AB_delight/ & Cache/Knight/AB_delight/ incl. 3-panel dark/defaults/tuned; VERDICT: lift works, zero clipping, but defaults too weak on recorded-dark donors — tuned values proven (aoDiv 1.0/floor .25/gamma .55/gain 1.2), ruling at TASK-200; shipped assets sha256-verified bit-identical, prompts file restored bit-identical; handoff handoffs/TASK-195.md)
- blocked-by: TASK-192, TASK-193 (scripts QA-passed + on disk), TASK-194 (manifest current)
- parallel-safe: yes (headless Bash + Blender; no editor, no import)
- spec: >
    Prove Track C before anything ships. (a) CONCEPT PROBE: generate ONE roster concept with FLUX.1-dev to a SCRATCH path (do NOT
    clobber the accepted Inbox PNG) and lay it beside the schnell original. (b) ALBEDO-LIFT REBAKE: Stage-2-ONLY rerun from the
    CACHED raw GLB of the recorded-dark accepted asset (Ogre; Knight too if its cache survives) with the TASK-193 lift active —
    quota-FREE (no Stage-1) — and assemble side-by-side refine_report renders: shipped-dark vs lifted. NO import, NO editor, NO
    commit. These renders are ALSO arm (B) of the TASK-199/200 Meshy A/B board. ACCEPTANCE: both comparisons exist as image pairs
    at recorded paths; the lift verdict (better / needs strength tuning) posted. Post renders + verdict in 🎨 Art.
- names: >
    Inputs `Tools/ArtPipeline/Cache/Ogre/` (cached trellis_raw.glb) + `Inbox/` (read-only). Outputs under Cache/<CardID>/ scratch
    + refine_report renders. Law: CONVENTIONS "Textured mesh law" (Stage 2), "Meshy second engine (M7.5)" (A/B gate).

#### TASK-196 — Track-C batch commit: tooling + manifest + validation evidence (build)
- assignee: build-master
- status: **done** (2026-07-18 — commit `a33aba6` on main, LOCAL-ONLY not pushed; 13 files via explicit pathspecs: 3 Tools/ArtPipeline files (192/193/194) + M7.5 pipeline docs deliberately included (TASKBOARD/CONVENTIONS/FAB-REQUESTS, handoffs 192–195+206+170-171 addendum, qa/TASK-192-193-qa.md PASS). Cache/Inbox evidence cited by path in the message, NOT committed; no Content/, __pycache__ left untracked. Wall/DeepMine NOT folded (Space-side malfunction, still blockouts). Note: origin/main was found at a16df32 — Jonathan pushed his M7 stack; a33aba6 is the only unpushed commit.)
- blocked-by: TASK-192, TASK-193 (qa-passed), TASK-194, TASK-195
- parallel-safe: no (Git)
- spec: >
    Commit Track C to `main` (NOT pushed): `concept_generate.py` + `refine_trellis_glb.py` (both with PASS QA reports on file) +
    `pipeline_manifest.json` + accepted validation-evidence paths in the message. VERIFY GIT STATE FIRST — HEAD `a16df32
    "polishingUp"` is a Jonathan self-commit; reconcile, never duplicate/amend. Explicit pathspecs, zero leakage (no Content/, no
    board files unless deliberately included). If the carried-in Wall+DeepMine (TASK-170/171/173) have landed by then, fold their
    LFS asset commit here OR leave it to TASK-210 — reconcile, don't double-commit. ACCEPTANCE: one clean local commit, hash
    posted; QA reports referenced. Post hash in 🔧 Build & Git.
- names: >
    Commit `Tools/ArtPipeline/concept_generate.py`, `Tools/ArtPipeline/refine_trellis_glb.py`, `Tools/ArtPipeline/pipeline_manifest.json`.
    Law: CLAUDE.md hard gates (PASS QA before commit; no push).

#### TASK-197 — MANUAL STEP 1: Meshy Pro account + MESHY_API_KEY env var (+ Norton exclusion if needed) (Jonathan — external gate)
- assignee: Jonathan (external gate — orchestrator surfaces in 🚨 Blockers + 📢; nothing in Track A proceeds without it)
- status: done (2026-07-18 — Jonathan in Claude Code: account created, premium sub active, key set as USER env var **`MESHY_TOKEN`** (HKCU; DEVIATION from specced MESHY_API_KEY — actual name is law, CONVENTIONS amendment routed to manager). No Norton exclusion yet — run bare first per playbook; TASK-198 first live call adjudicates)
- blocked-by: none (Jonathan-only; $20/mo Pro already approved by his own directive)
- parallel-safe: yes (external; no repo mutation)
- spec: >
    Jonathan: (1) create the Meshy account + subscribe PRO at meshy.ai; (2) generate an API key and set it as USER env var
    `MESHY_API_KEY` (HKCU — same as HF_TOKEN; note a Windows rollback wipes HKCU env, the uv/HF machine-config precedent);
    (3) ONLY IF the first tool run hits TLS/cert errors (Norton MITM), add exclusions for `meshy.ai` / `*.meshy.ai` — the same
    playbook as the proven huggingface.co exclusions. Say "done" in Slack or Claude Code. ACCEPTANCE: TASK-198's `--check` exits 0
    with the key present. The key value is NEVER pasted into chat, files, or logs.
- names: >
    Env var `MESHY_API_KEY` (HKCU). Norton exclusions `meshy.ai`/`*.meshy.ai` (conditional). Law: CONVENTIONS "Meshy second
    engine (M7.5)" secret law.

#### TASK-199 — Meshy A/B retexture run: Ogre (recorded-dark) through Stage-1.5 + Stage-2 → 3-arm board (art)
- assignee: art-director
- status: done (2026-07-18 — 4-col board at Cache/Ogre/AB_meshy/BOARD_Ogre_{front,threequarter,beauty_cycles,Dtexture}_dark_lift_tuned_meshy.png; arm C from EXISTING meshy_retex.glb, 0 credits; INVARIANT held [15000 tris, [TeamRegion, OgrePBR] 351f/2.1%, UVMap]; shipped assets sha-verified bit-identical; verdict C > B2 > B1 > A, fleet rec = Meshy retex + lift defaults, units-first 110 cr of 3273; handoffs/TASK-199.md → TASK-200 gate package complete)
- blocked-by: TASK-198 (tool QA-passed), TASK-195 (arm-B renders exist)
- parallel-safe: yes (headless Bash + Blender; no editor, no import)
- spec: >
    The fleet gate's evidence. Run `meshy_generate.py --mode retexture Ogre` (the recorded ACCEPTED-DARK asset, TASK-150) against
    its cached dense donor with `Inbox/Ogre.png` as style ref → Stage-2 rebake (WITH the TASK-193 lift, per manifest defaults) →
    refine_report renders. Assemble the 3-arm A/B board: (A) shipped dark (current Content), (B) Track-C albedo-lift only
    (TASK-195), (C) Meshy retexture + lift. Knight as an optional second sample if credits are comfortable. DO NOT import; DO NOT
    overwrite the shipped FBX/textures — Stage-2 output goes to a SCRATCH/AB path, not Content/RawAssets/ (this run is evidence,
    not production). Surface exit 3 (credit exhaustion) verbatim — expected pause, never fake. ACCEPTANCE: the 3-arm board exists
    as images at recorded paths; tris/slots/UVMap law verified identical across arms (the INVARIANT holds); posted for the gate.
    Post board + paths in 🎨 Art; the gate ask goes to 📢 Planning & Feedback.
- names: >
    Input `Cache/Ogre/trellis_raw.glb` + `Inbox/Ogre.png`. Outputs `Cache/Ogre/meshy_retex.glb` + scratch Stage-2 renders.
    Law: CONVENTIONS "Meshy second engine (M7.5)" (Stage 1.5 + A/B gate + INVARIANT).

#### TASK-201 — Fleet retexture/upgrade waves (Stage 1.5/1 + Stage 2) per the TASK-200 ruling (art)
- assignee: art-director
- status: done — UNITS WAVE 2026-07-21 (11/11 concept-true, 100 cr) + BUILDINGS WAVE 2 DONE 2026-07-21 (8/8 CONCEPT-TRUE, zero held: retexture ×6 ArrowTower/BombTower/BallistaTower/Barracks/CrystalTower/GoldNode @10 cr each + Wall/DeepMine via `--mode image3d` @**30 cr each — actual image3d cost, not ~10**; wave-2 = 120 cr, task total 220 cr, balance 3137→2917. All 8 RawAssets overwritten FBX+D/N/ORM (Wall/DeepMine = FIRST textured meshes over the blockout paths, full Stage-2 conform: box-fit exact, 20000/20000 tris, 2048², authored footprint UCX in-FBX); backups Cache/<CardID>/TASK201_shipped_backup/; triptychs ×8 Cache/_TASK201_report/. Wall selector fired clean 2.1% (manifest _pending→_tuned); DeepMine roof-band selector MISFIRED on the real mesh (18.8% bare-rock dome) → replaced with portal_beams 2.2% per the misfire laws (manifest _tuned, adjudication recorded). CrystalTower slot arrangement byte-parity with shipped — MI_CrystalGlow repoint undisturbed (TASK-202 must preserve it); GoldNode variant-law zero-TeamRegion parity, M_GoldGlow wiring import-side. Concepts/{Wall,DeepMine}.png committed per accepted-concept law. handoffs/TASK-201.md wave-2 section. ALL 19 assets ready for TASK-202)
- blocked-by: TASK-200 **[CLEARED 2026-07-21 — ruled GO; include list = FULL fleet: units wave 1, buildings wave 2, Wall+DeepMine via `--mode image3d` folded into wave 2 (closes 16/16). NEW acceptance bar per asset: color fidelity vs `Content/RawAssets/Concepts/<CardID>.png` (CONVENTIONS "Color-fidelity acceptance bar"). Dispatch NOW — headless, credit-paced]**
  **[WAVE 2 DISPATCH-READY (manager, 2026-07-21 afternoon window): RETEXTURE ×6 = ArrowTower, BombTower, BallistaTower, Barracks, CrystalTower, GoldNode (~90 cr total incl. the 2 image3d). FLAGS: CrystalTower — refresh T_CrystalTower_{D,N,ORM} SAME-PATH and DO NOT touch slot-1 MI_CrystalGlow wiring (the TASK-190 emissive rides those texture paths); GoldNode — single-slot emissive VARIANT law stands (no TeamRegion; keep the warm-yellow emissive). IMAGE3D ×2 = Wall + DeepMine: NEW meshes, NOT same-path retextures — full Stage-2 conform (box fit, building budgets, 2048², two-slot [TeamRegion, <CardID>PBR], UCX box per manifest) before any import; color-fidelity bar applies to all 8.]**
- parallel-safe: yes (headless; Meshy-credit + ZeroGPU quota-paced — orchestrator paces waves)
- spec: >
    Execute the approved fleet: per asset in the ruling's list, run the ruled engine (Stage-1.5 retexture on the cached donor /
    Meshy image-to-3D / TRELLIS 1536³ re-gen for heroes) → Stage-2 rebake (lift active) → production outputs OVERWRITING
    `Content/RawAssets/<CardID>.fbx` + `Textures/<CardID>/*` per the Textured-mesh law (two-slot [TeamRegion, <CardID>PBR],
    budgets, UVMap, feet/ground-center; GoldNode variant if in list). Wave in batches of ~4 (the M7 wave shape), previews per
    wave for a per-wave eyeball; surface exit 3 verbatim (credits/quota — pause, don't fake). Provenance in state.json per asset.
    ACCEPTANCE: per approved asset — FBX + D/N/ORM + refine_report + previews, law-conformant; report tris/bounds vs manifest.
    DO NOT import (TASK-202). Post per-wave in 🎨 Art.
- names: >
    Outputs `Content/RawAssets/<CardID>.fbx` + `.../Textures/<CardID>/*` + `Cache/<CardID>/*` per the ruled list. Law:
    CONVENTIONS "Textured mesh law" + "Meshy second engine (M7.5)" (INVARIANT).

#### TASK-202 — Fleet Stage-3 reimport: same-path SM_ overwrite of the retextured meshes (art, editor)
- assignee: art-director
- status: done (2026-07-21, executed by build-master inside the combined integration window per the manager's one-bounce authorization — headless commandlet Tools/reimport_meshes.py over the 19-asset sidecar: 19/19 OK, zero LOD_STEP_FAILED (all lods=4 @ LargeProp, castle-class not in wave); collision per manifest (units 4 hulls, buildings+Wall/DeepMine 1 UCX box); Nanite OFF; MI_Wall_PBR + MI_DeepMine_PBR created; commandlet texture-skip on the 17 existing-MI assets covered by a second headless pass (TASK-225 AssetImportTask pattern) — 18 assets × D/N/ORM refreshed (units 1024², buildings 2048², GoldNode variant excluded by law); slot→MI pointers finalized over MCP post-relaunch (reimport_finalize_materials_mcp.py law table, CrystalTower slot-1 → MI_CrystalGlow); summary JSONs in the TASK-240-241 handoff. Commit = TASK-241)
- blocked-by: TASK-201 (production FBX + per-wave eyeball), TASK-220 **[gate CLEARED — qa-passed + committed f609887; this wave applies LODs for free]**, editor+MCP up. Commit = TASK-241 (fresh integration window, TASK-210 retired)
  **[WAVE 2 PATH NOTE (manager, 2026-07-21): the 6 retextured buildings = same-path texture/mesh refresh as wave 1 (CrystalTower slot-1 MI_CrystalGlow preserved; GoldNode single-slot variant). Wall + DeepMine = FIRST TEXTURED import over the blockout SM_ via the proven box-UCX reimport branch (same-path overwrite of `/Game/Meshes/SM_Wall`/`SM_DeepMine`, authored-box collision from manifest ucx.boxes, refs must survive) — their landing closes the TASK-170/171/173 lineage to 8/8 (16/16 roster; TASK-241 reconciles those lines).]**
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    Import the approved fleet via the PROVEN automation (Tools/reimport_meshes.py commandlet + reimport_finalize_materials_mcp.py;
    editor-bounce authorized per the same-path-reimport memory — but Jonathan closes the editor himself when present): per asset,
    same-path OVERWRITE `/Game/Meshes/SM_<CardID>` + refresh `T_<CardID>_{D,N,ORM}` + `MI_<CardID>_PBR`; slots EXACTLY
    [TeamRegion → MI_TeamColor_Blue, <CardID>PBR → MI_<CardID>_PBR] (GoldNode single-slot variant); Nanite OFF; unit hulls /
    building box-UCX per category; refs MUST survive (BP_Unit/Building_<CardID>, cards.csv, placement ghost). MCP readback per
    asset. ACCEPTANCE: each approved SM_<CardID> is the retextured mesh at its unchanged path, readbacks reported; NOT committed
    (TASK-210). Post in 🎨 Art.
- names: >
    `/Game/Meshes/SM_<CardID>` (same-path) + `/Game/Textures/T_<CardID>_*` + `/Game/Materials/Instances/MI_<CardID>_PBR`.
    Tools: `Tools/reimport_meshes.py`, `Tools/reimport_finalize_materials_mcp.py`. Law: CONVENTIONS "Textured mesh law".

#### TASK-205 — Fleet animation retarget rollout per the TASK-204 ruling (art, editor)
- assignee: art-director
- status: **closed-by-reconcile** (2026-07-19 — scope executed incrementally by TASK-230 + TASK-231 under Ruling A via the sanctioned Blender lane [CONVENTIONS M7.5 export-defect clause]: 6/9 rigged units live on Meshy clips at unchanged paths, AB_Test evidence scrubbed at TASK-230. Remainder = 3 documented holds (Cavalry quadruped problem, Sapper re-rig, MilitiaMob floor adjudication) recorded in handoffs/TASK-231.md §2/§8 — new tasks if the manager rules them worth pursuing)
- blocked-by: TASK-204 (ruling — possibly cancelled-by-ruling) **[overnight batch note 2026-07-19: TASK-221/222/224 execute this task's scope incrementally under Ruling A — no duplicate ownership; TASK-205 closes or carries the remainder at morning reconcile]** **[TASK-221 spike FAIL 2026-07-19: batch export drops FK in-editor too (foot 7.07 vs 57 uu, identical to headless) — export lane needs Jonathan's manual UI Export or an engine fix; see TASK-204 finding + handoffs/TASK-221-222.md]** **[2026-07-19 day: Jonathan's manual UI Export ALSO produces root-only clips (same batch-op code path, log-proven) — NO working export lane in UE 5.8. Preview processor proven good. Fleet rollout now depends on an alternate retarget lane: Blender-side retarget + direct FBX anim import (agent-executable) is the leading candidate; see TASK-204 FINDING 2.]**
- parallel-safe: no (editor-mutating — single editor, serialize)
- spec: >
    Roll the approved clips across the ruled unit list: retarget via the TASK-203 RTG_ asset, then OVERWRITE `A_<CardID>_<Action>`
    AT THEIR EXISTING `/Game/Characters/Anims/` paths (same-path law — preserves the TASK-189 composed-soft-path Attack/Death
    triggers + the shared ABP locomotion wiring; the ABP-rebind lesson from commit 7ec9916/81230db applies: verify skeleton
    bindings after import). Per unit: PIE-verify via `SummonTestUnit` that it walks/idles/attacks/dies with the new clips; the
    miner death-promptness invariant (TASK-189) holds. Delete the AB_Test scratch folder at the end. ACCEPTANCE: every ruled unit
    animates with retargeted clips at unchanged asset paths; PIE spot-checks reported; scratch cleaned; NOT committed (TASK-210).
    Post in 🎨 Art.
- names: >
    Overwrite `/Game/Characters/Anims/A_<CardID>_{Idle,Walk,Attack,Death}` per ruling (same-path). `RTG_MeshyBiped_to_SiegeBiped`
    reused. Law: CONVENTIONS "Meshy second engine (M7.5)" animation clause + "Skeletal rig & animation workstream (M7)".

#### TASK-206 — FAB-005 + FAB-006: author the purchase-request entries with license notes (art, files)
- assignee: art-director
- status: **done** (2026-07-18 — FAB-005/006 authored at status `requested` in fab/FAB-REQUESTS.md; handoff handoffs/TASK-206.md; awaiting Jonathan's TASK-207 approve+purchase gate)
- blocked-by: none
- parallel-safe: yes (file-only — .claude/pipeline/fab/FAB-REQUESTS.md; no editor)
- spec: >
    Track B per the FAB-REQUESTS protocol (existing FAB-001..004 records UNTOUCHED). Author TWO entries at status `requested`:
    **FAB-005** — "Stylized RTS Buildings & Props Pack" (fab.com listing `a4b43ae5-e442-4d51-93f2-fea8d77e9f37`): roster mapping
    (Town Center/Fortress→Castle-tier, Barracks, Watch Tower→ArrowTower-tier, wall segments→Wall, construction meshes, props),
    intended drop `Content/Fab/StylizedRTSBuildings/`, license note (Fab standard license — record the exact license tier shown on
    the listing; agents cannot verify PRICE, FAB blocks bots — Jonathan verifies at checkout). **FAB-006** — ONE rigged stylized
    unit pack: evaluate the candidates (Toon RTS Units; TAB Medieval Knights — Epic-skeleton rigged, retargets cleanly in UE 5.8;
    Stylized Warrior packs), PICK one with a written justification (rig/skeleton type, roster coverage, style fit vs the §6 bar),
    intended drop `Content/Fab/<Pack>/`, same license discipline. ACCEPTANCE: both entries complete per protocol with license
    notes + drop paths + roster maps; awaiting Jonathan. Post in 🎨 Art + one-liner in 🚨 Blockers (purchase needed).
- names: >
    `.claude/pipeline/fab/FAB-REQUESTS.md` entries FAB-005, FAB-006 (status requested). Law: CONVENTIONS "Fab quarantine",
    fab/FAB-REQUESTS.md protocol.

#### TASK-211 — HOTFIX: clear Jonathan's 3 recurring editor Load Errors (stale Meshy-spike ref + unsaved skeleton bone merge) (art, editor)
- assignee: art-director
- status: **done (escalation resolved by manager ruling 2026-07-18)** ← was: escalated (2026-07-18 art-director: error 1 RESOLVED+verified, no Jonathan click needed; errors 2+3 STRUCTURAL — every unit mesh roots at `<Unit>_Rig` vs skeleton root `Footman_Rig` so the bone merge fails silently on every load and the spec'd load+save is a proven no-op; fleet-wide all 8 non-Footman units; fix lanes + evidence in handoffs/TASK-211.md — needs manager re-scope; ABP_Footman verified bound+clean, zero mutations made). **MANAGER RULING: FIX LANE (handoff option 1)** — Jonathan explicitly asked for these errors FIXED; accept-as-benign contradicts the user directive, and the fix is cheap, permanent, and hardens the rig pipeline against recurrence. → follow-ups **TASK-212 (tooling: armature-object rename in rig_character.py + batch-rename the 8 existing rig FBXs) → TASK-213 (editor: 8× same-path SK_<Unit> reimport + skeleton/ABP/Message-Log verify)**. NOT folded into TASK-205 (it gates on the TASK-204 eyeball — too slow for a live-editor annoyance, and it touches A_ clips, not SK meshes); TASK-213 sequences BEFORE any TASK-205 fleet wave so retargets land on a clean skeleton. TASK-209 FAB interaction noted: hardening + near-term fix stay valuable regardless of eventual pack replacements. This task itself made zero mutations; nothing to commit.
- blocked-by: none (MCP up; Jonathan ACTIVE in the editor — coordinate-safe ops ONLY, no editor close/restart, no save-all)
- parallel-safe: no (editor-mutating — single editor)
- spec: >
    Fix the 3 recurring Message Log Load Errors in Jonathan's live editor. DIAGNOSIS (done, orchestrator-verified — do not re-derive):
    (1) "/Game/A_Footman_Meshy_Idle → dependent /Game/Characters/Anims/AB_Test/SK_Footman_Meshy_Skeleton ... Skipped package" — the
    TASK-203 incident's root-level strays ARE deleted on disk (Content root clean; AB_Test/ holds the 11 intended scratch assets);
    the recurring load is a STALE REFERENCE in the live editor, most likely the spike-era anim-editor tab (the one that wedged the
    Walk delete, handoffs/TASK-203.md) being restored, or a lingering asset-registry/referencer entry. (2)+(3) "SK_Footman_Skeleton
    is missing bones that SK_Cavalry / SK_Knight needs" — PRE-EXISTING M7 debt, NOT spike damage: unit imports merged bones
    transiently but the shared skeleton was never re-saved, so it re-fires every load. WORK: (a) verify via asset-registry that
    NOTHING on disk references /Game/A_Footman_Meshy_Idle; close/clear the stale anim tab(s) programmatically if possible, else
    hand Jonathan the one-click instruction; (b) load SK_Cavalry + SK_Knight to force the bone merge, then SAVE SK_Footman_Skeleton
    plus ONLY the specific dirtied assets (NEVER save-all — the L_Arena law) and VERIFY ABP_Footman stays skeleton-bound + compiles
    (LogsToolset; the a7a77f6 rebind precedent is the recovery path if not) and the roster still animates. ACCEPTANCE: Message Log
    clean of all 3 errors on a fresh check; live assets otherwise untouched; every action coordinate-safe with Jonathan active.
    Post in 🎨 Art; NOT committed (rides TASK-210 or its own build-master flow — reconcile).
- names: >
    `/Game/Characters/Anims/AB_Test/*` (scratch, intact), `SK_Footman_Skeleton` + `ABP_Footman` (`/Game/Characters/`), SK_Cavalry/
    SK_Knight. No renames, no deletes outside verified strays. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)",
    "Meshy second engine (M7.5)" (scratch-path clause); editor-close-is-Jonathan's-choice memory.

#### TASK-213 — SK fleet root-fix reimport: 8× same-path SK_<Unit> + skeleton save-once + Message-Log clean verify (art, editor)
- assignee: art-director
- status: done (2026-07-18 art-director — all 8 SK_<Unit> reimported same-path from the TASK-212 FBXs [MCP import_file refuses overwrite → Jonathan's one bulk Reimport click on my pre-selected 8, coordinated in 🎨 Art]; disk truth: `<Unit>_Rig` purged / `Footman_Rig` ×3 in every uasset; skeleton 22→22 bones NEVER dirtied (roots now match — no merge needed; not saved, per only-what-dirties); slots [TeamRegion,<Unit>PBR] + MIs survived un-reset; verts identical; ABP bound + zero compile errors; A_Knight_Walk/A_Cavalry_Attack animate spot-check PASS; load-all-8 fired ZERO new missing-bones (only the 2 pre-wave 20:38 residue lines remain, clear on next session). Saved exactly the 8 SK uassets. handoffs/TASK-213.md; rides TASK-210)
- blocked-by: TASK-212 (renamed FBXs, QA-passed), editor+MCP up
- parallel-safe: no (editor-mutating — single editor; Jonathan is ACTIVE — coordinate the wave with him or run at a quiet moment, no close/restart without his say, NEVER save-all)
- spec: >
    Same-path REIMPORT of the 8 SK_<Unit> meshes from the TASK-212-renamed FBXs against the EXISTING shared skeleton
    `/Game/Characters/SK_Footman_Skeleton` (M7 rigged-import lane precedent, TASK-165): unchanged asset paths so ABP/BP/soft-ref
    wiring survives; slots keep the two-slot [TeamRegion, <UnitID>PBR] contract. Then VERIFY: loading each SK_<Unit> (incl.
    SK_Cavalry + SK_Knight, the two live offenders) fires NO missing-bones warning (mesh root `Footman_Rig` now matches the
    skeleton root); save SK_Footman_Skeleton ONCE IF it dirties plus ONLY the specific dirtied assets (only-what-dirties law);
    ABP_Footman stays skeleton-bound + compiles clean (a7a77f6 rebind is the recovery path); roster spot-check animates
    (A_<Unit>_Walk preview or SummonTestUnit on 2–3 units). ACCEPTANCE: fresh Message Log clean of the missing-bones errors with
    all 8 loaded; ABP bound + roster animates; nothing saved beyond what dirtied; NOT committed (rides TASK-210 — reconcile).
    Post in 🎨 Art.
- names: >
    Same-path `/Game/Characters/SK_{Pikeman,Cleric,Longbowman,MilitiaMob,Miner,Sapper,Cavalry,Knight}` from
    `Content/RawAssets/Characters/<Unit>.fbx`; `SK_Footman_Skeleton` + `ABP_Footman` verify. Law: CONVENTIONS "Skeletal rig &
    animation workstream (M7)" (shared skeleton, two-slot SK contract); editor-close-is-Jonathan's-choice memory.

---

