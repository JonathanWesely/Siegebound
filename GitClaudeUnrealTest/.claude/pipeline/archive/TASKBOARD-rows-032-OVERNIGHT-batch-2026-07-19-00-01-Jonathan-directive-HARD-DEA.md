<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-221 — SPIKE: in-editor IKRetargetBatchOperation export — does FK limb rotation survive? (art, editor) [P1, 25 min]
- assignee: art-director
- status: done 2026-07-19 01:15 — **SPIKE VERDICT: FAIL.** In-editor `IKRetargetBatchOperation.run_batch_retarget` (via UE python remote-execution INSIDE the live editor, on Src_Walk per dispatch) produces the IDENTICAL root-only output as the headless commandlet: retargeted foot_l/foot_r amplitude **7.07 uu** vs source foot **56.97 uu** (pelvis 7.16 vs 7.1 transfers fine; hands 7.2 = root-only too). Repair attempts all no-effect: per-op `run_op_initial_setup`, `assign_ik_rig_to_all_ops` (both rigs), `auto_map_chains(EXACT, force)` global AND per-op. Config verified correct at EVERY layer incl. new checks: FK op source-chain map LeftLeg←LeftLeg…(9/9), chain start/end bones resolve on both real skeletons, retarget roots Hips/pelvis valid. Conclusion: the batch-op FK transfer is broken PROCESS-INDEPENDENTLY in UE 5.8 — NOT a commandlet-environment defect. Anim lane BLOCKED on Jonathan's manual UI check/Export (note: the UI Export lane itself is still UNTESTED — TASK-203 §5's one-click preview check will reveal whether even that works). Evidence asset `/Game/Characters/Anims/AB_Test/Src_Walk_LiveSpike` (saved). Handoff: handoffs/TASK-221-222.md **[2026-07-19 day update: UI Export lane now TESTED — ALSO FAIL. Jonathan's live "Export Selected Animations" produced root-only clips (Walk foot 7.07 uu, identical to batch); output log shows the UI button calls the same Duplicate-and-Retarget batch op. Preview processor confirmed GOOD by eyewitness. Spike conclusion upgraded: the defect is in the one shared exporter, not any invocation environment. See TASK-204 FINDING 2.]**
- blocked-by: TASK-218 (editor frees)
- parallel-safe: no (editor)
- spec: >
    Via editor-python INSIDE the live editor (NOT the proven-broken commandlet path): run IKRetargetBatchOperation/export on ONE
    clip (Footman Attack, RTG_MeshyBiped_to_SiegeBiped) to a scratch path; verify FK limb rotation SURVIVES (visual + bone-curve
    check vs the TASK-203 in-editor preview). PASS ⇒ unblocks 222/224. FAIL ⇒ anim lane blocked on Jonathan's manual click —
    record verbatim on TASK-204/205, post 🚨, pivot clock. Post in 🎨 Art.
- names: >
    Scratch `/Game/Characters/Anims/AB_Test/`. Law: M7.5 anim same-path law (NOT yet — scratch only), Ruling A.

#### TASK-225 — Recorded-dark albedo fix: per-asset albedo_delight rebake (A, headless NOW) + same-path reimport (B, editor) (art) [P2]
- assignee: art-director
- status: done 2026-07-19 01:21 — part-A rebakes + part-B editor same-path reimport BOTH COMPLETE. 5/5 D textures live in-engine via in-place AssetImportTask over the TASK-221 remote-exec lane (asset identity/refs preserved; sRGB+TC_DEFAULT preserved; AssetRegistry FileMD5 == on-disk PNG MD5 on all 5). N/ORM skipped (bit-identical), FBX skipped (D-only contract). In-engine before/after: units + statics visibly lift (identical cam/light/exposure) — strip at Tools/ArtPipeline/Cache/_TASK225_report/AB_TASK225B_inengine_dark_vs_delight.png. Saved ONLY the 5 textures; L_Arena reloaded clean (zero dirty readback); no material/shader errors, MapCheck 0/0. Cavalry residual-darkness note stands (part-A §residual). handoffs/TASK-225.md §Part B
- blocked-by: part A none; part B TASK-218 (editor; slot after TASK-222, editor-bounce authorized overnight)
- parallel-safe: part A yes / part B no
- spec: >
    Ruling B middle lane. PART A (headless, NOW): Stage-2 rebake with tuned per-asset `albedo_delight` (TASK-195-proven values)
    for the recorded-dark assets — Ogre + Knight first, then the other TASK-172-recorded dark units as time allows; outputs
    overwrite Content/RawAssets/<CardID> textures (backups verified first). PART B (editor): same-path reimport via the proven
    commandlet (bounce authorized; editor RUNNING at end), MCP readback slots/Nanite/refs. NO fleet retexture — TASK-200 stays
    open. Before/after renders for morning. Post in 🎨 Art.
- names: >
    `Tools/ArtPipeline/refine_trellis_glb.py` (run only), `Content/RawAssets/Textures/<CardID>/*`, same-path `/Game/Meshes/
    SM_<CardID>` + `T_<CardID>_*`. Law: "Textured mesh law"; Ruling B.

#### TASK-226 — Environment life: emissive pulse on OUR glow materials + lawful-path WPO sway check (art, editor) [P3, 15 min]
- assignee: art-director
- status: done 2026-07-19 01:26 — sine emissive pulse LIVE on M_GoldGlow + M_CrystalGlow (Time→Sine 0.1 Hz→±12%→Multiply spliced ahead of EmissiveColor; original chains intact; desc-tagged "TASK226 pulse mult"; no MI changes needed). Verified: recompiles clean, no shader errors, timed captures of GoldNode_Blue rank exactly per predicted sine (0.889/0.924/0.942/1.116 → 174.6/175.1/175.3/177.2 glow means), CrystalTower crystal-top pixels breathe, glow integrity intact. Evidence: Tools/ArtPipeline/Cache/_TASK226_report/AB_TASK226_pulse_min_vs_max.png. Saved ONLY the 2 materials; L_Arena reloaded clean (zero dirty). **WPO sway SKIPPED per Ruling C — no lawful path (Fab donors read-only, DA branch-owned); recorded in handoff.** handoffs/TASK-226.md
- blocked-by: TASK-218 (editor; slots after the anim loop)
- parallel-safe: no (editor)
- spec: >
    Ruling C. (1) Subtle sine emissive pulse (slow, ±10–15%) on `M_GoldGlow` + `MI_CrystalGlow`/M_CrystalGlow (OUR assets).
    (2) WPO wind sway ONLY if a lawful path exists (materials already under /Game; NO Fab-donor edits, NO DA repoints) — else
    SKIP + record. No collision/nav/perf-risk changes; no L_Arena edits. Post in 🎨 Art.
- names: >
    `/Game/Materials/M_GoldGlow`, `M_CrystalGlow` + `MI_CrystalGlow`. Law: Ruling C; "Fab quarantine"; M7.6 branch-ownership.

#### TASK-228 — Overnight close: commit all completed lanes + editor safe-state by 02:30 (build) [HARD 02:15]
- assignee: build-master
- status: done (2026-07-19 — overnight batch committed on main via git-worktree lane [primary tree stayed on m7.6-arena10x, editor never disturbed]; scope = TASK-223 40 Meshy FBXs + TASK-225 A/B lifted albedo set [5 raw D PNGs + 5 FBX + 5 T_*_D.uasset + manifest] + TASK-226 pulse materials + overnight handoffs + board/CONVENTIONS docs snapshot; M7.5 pending pile [SKs, rig FBXs, tooling py, TASK-203 Footman Meshy FBXs, IK_/RTG_ uassets] deliberately left for TASK-210; editor-SCC strays Src_Walk_LiveSpike/RTG unstaged; not pushed; commit hash in 🔧 Build & Git)
- blocked-by: pencils-down 02:10 (whatever of 222/224/225/226/227 completed)
- parallel-safe: no (Git + editor)
- spec: >
    Commit completed lanes on MAIN with explicit pathspecs (anim assets; reimported textures/meshes; material edits; Meshy raw
    FBX + provenance) — M7.6 branch UNTOUCHED, L_Arena/DA untouched, no push, verify git state first. Editor left RUNNING and
    saved (graceful only). Flip board statuses; evidence paths + done/held/blocked splits into the handoff for the orchestrator's
    morning 📢 report. Post hash(es) in 🔧 Build & Git.
- names: >
    Commits on `main` only. Law: CLAUDE.md hard gates; overnight grant (graceful, editor running at end).

#### TASK-230 — Footman live wiring via the Blender lane (revives the TASK-222 spec) (art, editor)
- assignee: art-director
- status: **done** (2026-07-19 — 4 Meshy clips LIVE at unchanged `/Game/Characters/Anims/A_Footman_*` paths via same-path reimport-over; UE amplitude gate PASS Walk feet 61.59/52.79 ≥40; PIE-verified full-body attack + stride walk + believable death + idle sway, handedness correct/not mirrored, zero slide (root-lock); Attack RateScale 1.8, Death 1.5 (fits the 2.0 s destroy hold); backups saved at `Backup_Procedural/`; AB_Test Src_*1 + LiveSpike evidence deleted; handoffs/TASK-230.md — captures for Jonathan in Tools/ArtPipeline/Cache/Footman/retarget/ue_previews/, hero shot pie_41.png)
- blocked-by: TASK-229 (qa-passed + amplitude gate proven), editor+MCP up
- parallel-safe: no (editor)
- spec: >
    Revive TASK-222 with the sanctioned lane: run TASK-229's tool on Footman's 4 clips → import the retargeted FBXs onto
    SK_Footman_Skeleton → BACKUPS of the live clips first → root-motion OFF, RateScale tuned to the existing cadence →
    same-path OVERWRITE `/Game/Characters/Anims/A_Footman_{Idle,Walk,Attack,Death}` (preserves TASK-189 triggers + ABP law;
    verify skeleton binding after import — the a7a77f6 lesson). PIE via SummonTestUnit: attack moves LEGS+ARMS (Jonathan's
    named example), walk/idle clean, death holds, miner promptness intact. Before/after captures for Jonathan. ACCEPTANCE:
    Footman animates with the Meshy clips live at unchanged paths; amplitude visibly correct in PIE. Post in 🎨 Art.
- names: >
    Same-path `/Game/Characters/Anims/A_Footman_{Idle,Walk,Attack,Death}`; SK_Footman_Skeleton; ABP_Footman verify. Law:
    CONVENTIONS M7.5 animation clause (same-path), sanctioned-export-path clause; Ruling A.

#### TASK-231 — Fleet loop via the Blender lane: 8 remaining units, Cavalry preview-gated (revises TASK-224) (art, editor)
- assignee: art-director
- status: **done** (2026-07-19, RECOVERY RUN — prior agent's stall reconstructed from evidence: its 5-unit wire was COMPLETE and law-compliant, nothing half-wired, no rollback. LIVE on Meshy clips at unchanged paths: Knight, Pikeman, Cleric, Longbowman, Miner (UE Walk gates 52.2/56.6, 55.8/45.1, 57.0/59.4, 59.4/41.3, 51.9/47.0 ≥40; rm=off rl=on; Attack rates 1.25/2.0/1.6/2.5/2.0 — Knight+Pikeman cadence-exact; Death 1.5 fleet-wide → 1.98s ≤ 2.0 hold; PIE-verified: Longbowman draws left-hand-bow correctly + death collapse, Cleric staff idle, Miner chops at GoldNode, Knight/Pikeman confirmed from live captures). HELD: Cavalry (gate FAIL 37.9/36.2 + preview shows horse deformed by rider-bone Walk — NEVER wire, needs quadruped source), Sapper (gate FAIL 5.4/11.8 — hunched rig breaks aim transfer; preview confirms lump-shuffle), MilitiaMob (borderline FAIL 39.4/34.2, short-unit stride; height-scaled floor ≈34 would pass — manager adjudication flagged). QA WARN-1 asymmetry assert applied: flips only on near-equal pairs, adjudicated non-mirror via PIE handedness. T231_Stage scratch deleted, zero dirty packages, ABP intact. handoffs/TASK-231.md; captures in Cache/<Unit>/retarget/ue_previews/)
- blocked-by: TASK-230 (Footman proven live)
- parallel-safe: no (editor; the Blender/tool half is headless and may pre-run)
- spec: >
    Run the tool across the remaining 8 rigged units (Knight, Cavalry, Pikeman, MilitiaMob, Sapper, Cleric, Longbowman,
    Miner — 36 FBXs on disk) and wire per unit exactly as TASK-230 (backups, root-motion off, RateScale, same-path A_<Unit>_*
    overwrite, binding verify, PIE spot-check). CONSERVATIVE per-unit acceptance (Ruling A): any unit that reads badly is
    HELD BACK with the finding recorded. CAVALRY is PREVIEW-GATED — the rider-fit hold-back stands: retarget + preview
    captures only, NO live overwrite without an explicit pass. Record the done/held split; TASK-205 closes at the reconcile
    that follows this task. Commit rides the next build-master window. ACCEPTANCE: each accepted unit animates with Meshy
    clips at unchanged paths; holds documented; Cavalry gated. Post per-unit one-liners in 🎨 Art.
- names: >
    Same-path `/Game/Characters/Anims/A_<Unit>_{Idle,Walk,Attack,Death}` per accepted unit; Cavalry = captures only until
    passed. Law: CONVENTIONS M7.5 animation clause, sanctioned-export-path clause; Rulings A + conservative rollout.

#### TASK-232 — MilitiaMob wire under the height-normalized floor (art, editor) [~15 min]
- assignee: art-director
- status: **done** (2026-07-19 — art DONE: LIVE at unchanged paths, saved not-dirty; gate PASS 39.43/34.25 vs 34.06, PIE verdict PASS all 4 clips; handoffs/TASK-232.md. INTEGRATED: assets committed on main in `f609887` via TASK-235 (e) clause — fleet closes 7/9)
- blocked-by: none (ruling 1 is the authorization; editor+MCP up)
- parallel-safe: no (editor)
- spec: >
    Wire MilitiaMob exactly per the proven TASK-230/231 mechanics: backup /Game/Characters/Anims/Backup_Procedural/A_MilitiaMob_*,
    same-path reimport-over A_MilitiaMob_{Idle,Walk,Attack,Death} onto SK_Footman_Skeleton, enable_root_motion=False +
    force_root_lock=True, RateScale to cadence, in-editor amplitude re-gate at the HEIGHT-NORMALIZED floor (≈34 uu @ 1.49 m),
    PIE visual verdict (short-stride plausible = pass; conservative law). ACCEPTANCE: MilitiaMob animates full-body at unchanged
    paths, saved not-dirty; captures for Jonathan. Commit rides TASK-235. Post in 🎨 Art.
- names: >
    Same-path `/Game/Characters/Anims/A_MilitiaMob_{Idle,Walk,Attack,Death}` + Backup_Procedural. Law: CONVENTIONS
    sanctioned-path clause (height-normalized floor), M7.5 animation clause.

#### TASK-234 — BACKLOG: Sapper re-rig + retarget retry (~17 Meshy credits) (art)
- assignee: art-director
- status: **done** (2026-07-21 evening -- WIRED + PIE VISUAL PASS. Wire ran post-relaunch (editor PID 41012, after the wedge-kill of PID 24104 on Jonathan's explicit instruction): t234_backup [4 procedural clips -> Backup_Procedural/, saved] -> t234_wire [same-path reimport of 4 MeshyRetargeted FBXs onto SK_Footman_Skeleton; UE-side WALK GATE PASS 42.94/39.81 vs the 38.4 height-normalized floor; root-lock on, root-motion off; RateScale Idle/Walk 1.0, Attack 2.0 -> 1.50s eff, Death 1.5 -> 1.98s eff <= 2.0s destroy hold; ABP target skeleton intact; all 4 SAVED]. Cadence adjudication: cards.csv Sapper Cadence=1.0 bSuicide=true -- at rate 2.0 the slam-impact frames (raw ~2.0s) land exactly on the 1.0s explosion tick; kept 2.0. MANDATORY PIE verdict (borderline foot_r +1.4): PASS -- upright alternating stride across 3+ frames (no skew/skating); full suicide sequence captured in a 0.05-dilation duel (wind-up crouch -> lunge -> explosion at 1.0s -> collapse -> prone -> destroy ~2.0s post-contact, verified numerically t=158.7->160.7); captures Tools/ArtPipeline/Cache/Sapper/retarget/ue_previews/live_Sapper_00..44.png (key: 00-02 walk, 13/20/24 slam, 30/38 collapse). Gameplay quirk recorded (programmer-domain, not an anim defect): Sapper unit-kills resolve within one cadence tick and the exploder survives unit contacts, marching on. NOT committed (build-master lane). Details: handoffs/TASK-234.md sec 7c) (re-rig attempt 1 recap: bomb-carve fixed the Meshy limb fit; WALK GATE 42.94/39.81 vs 38.4, was 5.36/11.81; Attack = Charged_Ground_Slam 127; 17 credits) <- was: headless-done-pending-wire
- blocked-by: none (credits ample)
- parallel-safe: yes (headless Meshy + Blender; wire step editor-serial)
- spec: >
    Retry the Sapper hold (hunched rig, Hips z-frac 0.308, breaks the aim-transfer rest-pose premise; Attack preset was pure
    root travel): Meshy re-rig with a corrected UPRIGHT pose and/or a different attack preset (~17 credits), re-run
    retarget_meshy_to_siegebiped.py, amplitude + PIE gates, wire per the proven mechanics only on pass. ACCEPTANCE: Sapper
    full-body at unchanged paths or hold re-affirmed with findings. Post in 🎨 Art.
- names: >
    `Content/RawAssets/Characters/Meshy/Sapper/` (new rig FBXs) → `MeshyRetargeted/Sapper/` → same-path `/Game/Characters/
    Anims/A_Sapper_*` on pass. Law: sanctioned-path clause.

#### TASK-235 — CONSOLIDATED M7.5 commit: anim batch + the retired TASK-210 pile (build) [AUTHORIZED]
- assignee: build-master
- status: **done** (2026-07-19 — commit `f609887` on main via the TASK-228 worktree route, 139 files: 28 live A_<Unit>_* + 28 Backup_Procedural + 36 MeshyRetargeted FBXs + retarget tool [anim batch, TASK-229/230/231/232] + the retired TASK-210 pile [8 SK_, 8 rig FBXs, rig/reimport/meshy tooling, guard-secrets, IK_/RTG_, TASK-203 Footman FBXs] + 12 handoffs + 3 QA reports + board/CONVENTIONS snapshot. All binaries LFS-pointer-verified; secret scan clean; leakage scan clean [zero branch-owned files]; primary tree byte-identical before/after [branch 42011de untouched, editor undisturbed]; index reconciled [28 SCC-auto-staged backups unstaged pre-commit]. AB_Test evidence + .fbm extractions + __pycache__ deliberately excluded [no keep marking]. NOT pushed. handoffs/TASK-235.md)
- blocked-by: TASK-232 (fold if landed; else commit without it and note)
- parallel-safe: no (Git; worktree pattern — checkout is on m7.6-arena10x with the editor LIVE: commit to MAIN via the proven
  TASK-228 worktree route, never touch the branch checkout or the live editor)
- spec: >
    ONE consolidated M7.5-lane commit to `main` (VERIFY git state first — Jonathan self-commits; explicit pathspecs; LFS-aware;
    NO push). SCOPE: (a) handoffs/TASK-231.md §7 — 20 modified A_<Unit>_* + 20 Backup_Procedural + 32 MeshyRetargeted raw FBXs;
    (b) TASK-230's Footman A_* + backups + its raw FBXs; (c) TASK-229 tool retarget_meshy_to_siegebiped.py + qa/TASK-229-qa.md;
    (d) the RETIRED TASK-210 pile: 8 SK_<Unit> uassets, rig FBXs, reimport_meshes.py + meshy_generate.py (+ their QA reports),
    guard-secrets.sh, IK_/RTG_ assets, Footman Meshy FBXs; (e) TASK-232's MilitiaMob assets if landed; (f) the TASK-230/231
    handoffs + board/CONVENTIONS deltas per repo convention. Leakage scan (no L_Arena, no DA, no M7.6-branch files). Flip board
    lines; mark TASK-210 retired-superseded. ACCEPTANCE: clean consolidated commit(s), hash posted, tree risk cleared. Post in
    🔧 Build & Git.
- names: >
    Commit on `main` via worktree. Law: CLAUDE.md hard gates (QA-passed only, no push), M7.6 branch-ownership law.

#### TASK-236 — Spell mechanics: Fireball + FrostNova → hero-origin line delivery (C++)
- assignee: gameplay-programmer
- status: done (2026-07-21 — integrated + committed `c2b2f59` at TASK-240; compile clean on the mixed tree. QA trail: qa/TASK-236-237-qa.md PASS 0 blockers; WARN-1 bot far-cluster whiff = playtest WATCH; WARN-2 CSV column append = manager follow-up; 3 nits)
- blocked-by: none
- parallel-safe: yes (SpellLibrary.{h,cpp} + delivery actor/sweep + SiegePlayerController aim pass; TASK-237 is data-only — no file overlap)
- spec: >
    Per CONVENTIONS "Spell delivery overhaul": Fireball + FrostNova fire FROM the hero toward the cursor/reticle point — a
    projectile travel or capsule/line sweep (programmer's call; reusing the M2 AProjectile lane is acceptable) over a SHORT
    forward distance, hitbox IN THE AIR along the line; effects apply to what the line hits (Fireball damage law incl. castle
    50% + no friendly fire; FrostNova freeze law) — magnitudes/costs UNCHANGED. Targeting mode KEPT: reticle sets aim, LMB
    confirm fires, deduct-at-confirm + resolver-false-refund laws intact. `ResolveSpell` pinned signature PRESERVED —
    TargetPoint becomes the AIM-POINT for these two (flag every call site). BOT: line spells originate from its castle toward
    its chosen target point (flagged design default); LogSiegeBot lines intact. Lightning/BattleCry/Pickpocket/ChainZap paths
    byte-untouched. VFX: spawn the SAME NS_Spell_<CardID> soft paths at the new delivery points (muzzle/travel/impact seams
    for TASK-238, null-safe). Line length + width = UPROPERTY tunables (`// GDD §4` comments), flagged playtest numbers.
    ACCEPTANCE: both spells visibly fire from the hero and hit along a line; refund on no-hit resolve rule documented; bot
    still casts; all other spells regression-clean. QA implied (shadow + include scans; flag the M5 law deltas). COMPILE on
    the mixed tree at TASK-240 (flagged). Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.{h,cpp}` + `SiegePlayerController.{h,cpp}` (aim pass) + optional
    delivery actor. Law: CONVENTIONS "Spell delivery overhaul (2026-07-21)", "Spells & Set III (M5)" (unchanged laws).

#### TASK-237 — Lightning: bigger radius as DATA + honest visual/hitbox scale verify (data)
- assignee: gameplay-programmer
- status: done (2026-07-21 — integrated + committed `c2b2f59` at TASK-240; DT_Cards live at 700 (readback verified). QA trail: qa/TASK-236-237-qa.md PASS clean; stale-comment sweep stays deferred until 700 survives playtest)
- blocked-by: none
- parallel-safe: yes (cards.csv only — no overlap with TASK-236's files)
- spec: >
    cards.csv: Lightning AoERadius 400 → **700** (flagged playtest number — manager pick, Jonathan tunes at playtest). VERIFY
    (read-only trace, no code edits): the reticle decal AND the selection hitbox both scale from the AoERadius data (the
    honest-scaling law) — if either is hardcoded, route the finding back as a TASK-236-owner code item, do NOT patch here.
    Document the strike-height expectation for TASK-239 (VFX-side; a spawn-height seam only if TASK-239 requests one). Other
    cells frozen (git-diff confinement at TASK-240). ACCEPTANCE: single-cell diff; scaling verdict recorded. QA implied.
    DT_Cards reimport rides TASK-240. Post in ⚙️ Dev & QA.
- names: >
    `Docs/Data/cards.csv` (Lightning row, AoERadius cell only). Law: CONVENTIONS "Spell delivery overhaul" (honest scaling),
    "Data-driven card stats".

#### TASK-238 — VFX v2: NS_Spell_Fireball + NS_Spell_FrostNova as projectile/line effects (art, editor)
- assignee: art-director
- status: done (2026-07-21 — integrated + committed `c2b2f59` at TASK-240 (both uassets, LFS-verified). Authoring trail: handoffs/TASK-238.md; Fireball ← NS_Fire_Magic_Projectile3, FrostNova ← NS_Ice_Magic_FrontSpike; PIE-verified at gameplay camera; captures Tools/ArtPipeline/Cache/TASK-238/. FLAG stands: Fireball visual front ~1600 uu/s vs sweep 3000 — TravelDuration is the code-side sync lever if Jonathan wants exact sync)
- blocked-by: TASK-236 (delivery seams defined — muzzle/travel/impact), editor+MCP up
- parallel-safe: no (editor Niagara — serialize)
- spec: >
    Re-author IN PLACE at `/Game/VFX/NS_Spell_Fireball` + `NS_Spell_FrostNova` (composed-path law — no renames) as DETAILED
    projectile/line effects from the packs: Fireball ← `Content/Fire_Magic/` (bolt/trail + explosion impact), FrostNova ←
    `Content/Ice_Magic/` (ice bolt/shard trail + freeze burst along the line). Must read as SHOOTING OUT from the hero and
    covering the line (§6 one-frame readability); scale to TASK-236's line length/width tunables. Donors READ-ONLY. PIE check
    with the new delivery. ACCEPTANCE: both spells read as detailed hero-fired bolts with line coverage; paths unchanged;
    Jonathan captures for morning/next playtest. Post in 🎨 Art.
- names: >
    In place `/Game/VFX/NS_Spell_Fireball`, `NS_Spell_FrostNova`. Donors `Content/Fire_Magic/`, `Content/Ice_Magic/`
    (READ-ONLY). Law: CONVENTIONS "Spell delivery overhaul", "Spell VFX element re-skin (M7)", "Template-donor rule".

#### TASK-240 — Spell overhaul integration: mixed-tree compile + DT_Cards reimport + PIE spell suite + worktree commit (build)
- assignee: build-master
- status: done (2026-07-21 — commit `c2b2f59` on main via worktree, 17 files. Mixed-tree compile SUCCEEDED 0 warn/0 err (SpellLineSweep + SpellLibrary + SiegePlayerController + SiegeBotController). DT_Cards: Lightning aoERadius=700 live via DataTableTools set_rows + saved (28 rows; SpellDelivery column serialized Auto — the QA WARN-2 benign path; a transient "Missing RowStruct while saving" log line was disproven by disk verify: 37.4KB uasset carries all row/column names, +2KB vs main = the new enum column). cards.csv git-diff confinement = single Lightning cell confirmed. PIE ×3: two full matches to match-end (winner Red, Defeat + Play Again UI up, LogSiegePlayerController match-ended ×2) + fresh deck-select re-init ×3; bot Rule 4 castle-front waves marching; MapCheck 0/0. LIMIT: player resolver-path casts NOT machine-exercised (input-injection lane denied by the session permission classifier; bot never drew rule-3a in the AFK matches) — TASK-238's PIE spawn evidence + QA's resolver verification + compile stand in; hands-on cast check = Jonathan's playtest with the WATCH list (line reach 900, bot far-cluster whiff QA WARN-1, slope overfly, radius-700 feel). Stale 400-comment sweep deferred per QA ruling until 700 survives playtest. TASK-239 (Lightning VFX) still pending — old strike visuals live, WIP assets uncommitted. handoffs/TASK-240-241.md)
- blocked-by: TASK-236 (qa-passed), TASK-237 (qa-passed), TASK-238, TASK-239
- parallel-safe: no (single editor + Git)
- spec: >
    (1) COMPILE on the mixed tree (checkout = m7.6-arena10x carrying qa-passed branch code — acceptable, FLAGGED; editor-bounce
    per learnings, coordinate with Jonathan if active). (2) DT_Cards reimport for the Lightning radius + git-diff confinement
    check (TASK-237 single cell). (3) PIE SPELL SUITE: Fireball + FrostNova fire from the hero with line hits (units along the
    line die/freeze; ground-circle behavior gone), Lightning strikes the bigger circle from high with honest radius, BattleCry/
    Pickpocket/ChainZap regression-clean, bot casts (castle-origin lines, rules 3a/3b logs), refund paths, Play Again clears
    all spell state. (4) COMMIT to MAIN via the TASK-235 worktree route (explicit pathspecs, no push, no branch files, no
    L_Arena/DA). ACCEPTANCE: suite green, hash posted, WATCH list (line coverage balance, radius 700, bot-origin feel) recorded
    for Jonathan's playtest. Post in 🔧 Build & Git.
- names: >
    Compile per CLAUDE.md; commit on `main` via worktree. Law: CLAUDE.md hard gates, M7.6 branch-ownership, "Spell delivery
    overhaul".

#### TASK-241 — Retexture-fleet integration: per-wave verify + worktree commit (build)
- assignee: build-master
- status: done (2026-07-21 — BOTH waves in one window (commit `b9a756d` on main via worktree, 160 files; also in 🔧 Build & Git + handoffs/TASK-240-241.md). 19/19 structural readback green: same paths, slots [TeamRegion→MI_TeamColor_Blue, <CardID>PBR→MI_<CardID>_PBR], Nanite OFF, units 4 hulls / buildings 1 manifest box, lods=4 @ LargeProp all (TASK-220 line applied for free, zero LOD_STEP_FAILED), refs held (BP_Unit_*/BP_Building_*/L_Arena). CrystalTower slot-1 MI_CrystalGlow preserved + emissive visually confirmed glowing over the refreshed mesh; GoldNode all-slots M_GoldGlow variant. Wall+DeepMine FIRST TextURED import over the blockout SM_ paths (box-UCX branch, refs survived) → **16/16 card roster textured; TASK-173 lineage closed 8/8**. Color-fidelity evidence = TASK-201 triptychs (Cache/_TASK201_report/) + in-world captures Cache/TASK-202/. PIE sanity rode TASK-240's suite (units marching, matches clean). NOT pushed)
- blocked-by: TASK-202 (fleet reimports landed per wave)
- parallel-safe: no (Git; may run per wave — units commit first, buildings+Wall/DeepMine after)
- spec: >
    Fresh integration window for the TASK-200-ruled fleet (TASK-210 retired): per wave — structural readback (same paths,
    slots, Nanite off, collision, GoldNode variant if touched), COLOR-FIDELITY evidence attached (per-asset render vs
    `Content/RawAssets/Concepts/<CardID>.png`, the new law), brief PIE sanity, then COMMIT to MAIN via the worktree route
    (LFS-aware, explicit pathspecs, no push, no branch files). Wall+DeepMine wave additionally closes TASK-173 to 8/8 —
    reconcile that board line. ACCEPTANCE: per-wave hash + evidence posted; 16/16 roster recorded when wave 2 lands. Post in
    🔧 Build & Git.
- names: >
    Commits on `main` via worktree. Law: CLAUDE.md hard gates, "Textured mesh law", "Color-fidelity acceptance bar", M7.6
    branch-ownership.

---

#### TASK-242 — Archer + Ogre: rig_character.py skinning + Meshy clips + retarget (art, HEADLESS) [dispatch NOW]
- assignee: art-director
- status: **done** (reconciled 2026-07-22 by TASK-243 from on-disk evidence — the 242 agent died at wrap-up but its work was complete: FBXs + handoff + gates all verified; TASK-243 consumed the outputs. Prior status note kept: 2026-07-21 evening — BOTH units complete, all gates PASS on rig attempt 1 each. SiegeBiped rigs (Footman_Rig root, 21 bones, two-slot carried) at Content/RawAssets/Characters/{Archer,Ogre}.fbx; 4 Meshy clips each (Archer Archery_Shot_1 224; Ogre Heavy_Hammer_Swing 128 — browsed two-hand smash, stills-verified); retarget WALK GATE PASS at height-normalized floors Archer 41.1 (62.07/54.18) + Ogre 65.8 (84.17/81.17), not borderline. 34 cr (17/unit), balance 2866. Evidence + wire notes in handoffs/TASK-242.md. NO editor/MCP/Git touched — TASK-243 owns the SK first-import.)
- blocked-by: none
- parallel-safe: yes
- spec: >
    (1) SKIN: run rig_character.py on the game-ready SM_Archer + SM_Ogre meshes → SiegeBiped-hierarchy rigged FBX per unit
    (Footman_Rig armature root, 21 bones, two-slot materials carried). (2) CLIPS: Meshy auto-rig each unit + 4 presets —
    Archer: Idle / Walk / **Archery_Shot_1 (preset 224, the proven Longbowman pick)** / Death; Ogre: Idle / Walk / a heavy
    TWO-HAND SMASH preset / Death (~17–23 cr per unit; budget 2 rig attempts each — WATCH prop-confusion on the Archer's bow
    and the Ogre's huge/nonstandard proportions; the Sapper rest-pose lesson applies: inspect Hips z-frac/pose BEFORE
    spending clip credits). (3) RETARGET via retarget_meshy_to_siegebiped.py with HEIGHT-NORMALIZED floors (heights from
    pipeline_manifest: Archer ≈ standard ⇒ ~40 uu; Ogre is TALL ⇒ floor ABOVE 40, compute 40 × height/1.75). ACCEPTANCE:
    per unit — rigged SiegeBiped FBX + 4 retargeted clips passing the height-normalized amplitude gate in Blender; Meshy
    provenance recorded; escalate (don't patch tools) on rig failure. Post in 🎨 Art.
- names: >
    `Content/RawAssets/Characters/{Archer,Ogre}.fbx` (rigged, raw-asset rule) + `Content/RawAssets/Characters/Meshy/
    {Archer,Ogre}/` + `MeshyRetargeted/{Archer,Ogre}/`. Tools RUN-only: rig_character.py, meshy_generate.py,
    retarget_meshy_to_siegebiped.py. Law: "Skeletal rig & animation workstream (M7)" (SiegeBiped, Footman_Rig root),
    sanctioned-export-path clause (height-normalized floor).

#### TASK-243 — SK_Archer + SK_Ogre FIRST import: shared-skeleton bind + anim wire + runtime light-up (art, editor)
- assignee: art-director
- status: **done** (INTEGRATED + COMMITTED `b90157e` by TASK-244, 2026-07-22 — structural readback re-verified at the commit window: shared-skeleton bind, two-slot materials, LOD0, all 8 anim paths, SM_ fallbacks, everything saved not-dirty. Prior note: DONE 2026-07-22 late-evening, no holds — SK_Archer + SK_Ogre live at /Game/Characters/ bound to the shared SK_Footman_Skeleton (clean bind, skeleton not dirtied), two-slot law, Nanite off, fleet-mirror (LOD0, no PhysAsset); 8 first-authored A_* clips wired (root-lock law; UE re-gate PASS Archer 62.07/54.18 vs 41.1, Ogre 84.17/81.17 vs 65.8; rates Archer Attack 2.5 [release beat 0.87s < 1.2 cadence, probe-verified], Ogre Attack 1.2 [1.53s ≈ 1.5 tick], Deaths 1.5 → 1.98s ≤ 2.0 hold). PIE verdicts ALL PASS: Archer WALKS (Jonathan's finding closed), left-hand bow draw + projectiles, death collapse + prompt destroy, red recolor; Ogre marches past enemy units per Siege, smashes tower+castle, heavy death collapse; ABP intact, SM_ fallbacks intact, Message Log clean for both. Captures in Cache/{Archer,Ogre}/retarget/ue_previews/live_*_t243_*.png. Handoff: handoffs/TASK-243.md. Commit rides TASK-244; note MI_{Archer,Ogre}_PBR left dirty (pre-existing, see handoff §4).)
- blocked-by: TASK-242 (rigged FBX + gated clips), editor+MCP up (queue behind TASK-174 and any live-playtest priority)
- parallel-safe: no (editor)
- spec: >
    FIRST-import lane (differs from the prior wire tasks — these units have NO SK yet; TASK-160-era precedent): import each
    rigged FBX as a NEW `/Game/Characters/SK_<CardID>` BOUND TO THE SHARED `SK_Footman_Skeleton` (Footman_Rig root law — the
    bind must succeed with NO missing-bones warning); slots carried from the SM_ MIs: [0] TeamRegion → MI_TeamColor_Blue,
    [1] <CardID>PBR → MI_<CardID>_PBR (two-slot SK law; Nanite off, ≤15k tris). Import the 4 clips per unit at
    `/Game/Characters/Anims/A_<CardID>_{Idle,Walk,Attack,Death}` (FIRST authoring — no backups exist; root-motion OFF +
    force_root_lock + RateScale to cadence per the proven wire mechanics; in-editor amplitude re-gate at the height-
    normalized floors). VERIFY the runtime path LIGHTS UP for both: ASummonedUnit auto-resolves `/Game/Characters/
    SK_<CardID>` at BeginPlay (composed-soft-path law) + ABP sharing on the shared skeleton (NO ABP edits via MCP — the
    AnimBP minefield; a7a77f6 rebind is the recovery). PIE visual verdicts: ARCHER WALKS (Jonathan's finding closed),
    bow-shot reads, Ogre walks + heavy smash, deaths hold, placement ghost + static SM_ fallback intact at their paths.
    ACCEPTANCE: both units fully animated in PIE at law paths, saved not-dirty; captures for Jonathan. Commit rides
    TASK-244. Post in 🎨 Art.
- names: >
    NEW `/Game/Characters/SK_Archer`, `SK_Ogre` (bound to SK_Footman_Skeleton) + `/Game/Characters/Anims/A_{Archer,Ogre}_*`.
    Law: "Skeletal rig & animation workstream (M7)" (SkeletalVisualMesh swap contract, two-slot SK law), sanctioned-path
    clause, M7.5 animation clause.

#### TASK-244 — Next build-master window: Archer/Ogre SK batch + tonight's owed items (build)
- assignee: build-master
- status: **done** (2026-07-22 — single consolidated commit **`b90157e`** on `main` (parent `59a994a`) via the worktree route (C:/GitProjects/wt244, core.longpaths, torn down; primary tree byte-verified untouched on m7.6-arena10x @ 42011de, editor live). 63 files: TASK-243 SK batch (SK_Archer/SK_Ogre + 8 A_* clips), TASK-242 raw provenance (rigged FBXs, procedural anim exports, Meshy 10 + MeshyRetargeted 8, rig_manifest.json), TASK-234 owed Sapper wire (4 live A_Sapper_* + 4 Backup_Procedural + 9 Meshy/MeshyRetargeted FBXs updated to the new rig — supersede f609887 content), MI_Castle_PBR streaming-rebuild save, TASKBOARD snapshot + handoffs 234/239/240-241/242/243. Gates: 56/56 binaries LFS-pointer-verified, secret-scan clean, leakage-scan clean (no branch files/AB_Test/.fbm/pycache/Cache/GDD-pdf), staged set == 63-path scope exactly. ADJUDICATION: MI_Archer_PBR + MI_Ogre_PBR EXCLUDED — byte-identical to main on disk (editor dirty-flag is in-memory only, never saved; per TASK-243 §4, Jonathan's save toast decides). cards.csv verified already carried by c2b2f59; CONVENTIONS.md already identical to main; qa/229+236-237 and the TASK-201/221-222 appends verified already committed. Owed-items ledger CLEARED. 14 editor-SCC auto-staged index entries unstaged post-commit (TASK-235 §4 precedent). NOT pushed. Handoff: handoffs/TASK-244.md.)
- blocked-by: TASK-243 (+ fold whatever owed items are verified by then: Sapper anim wire output [TASK-234], MI_Castle_PBR; the GDD as-built pass is NO LONGER in this fold — approved + committed separately as `59a994a`, 2026-07-21)
- parallel-safe: no (Git; worktree route — checkout on m7.6-arena10x, editor live)
- spec: >
    Consolidated next-window commit to MAIN via the worktree route (verify git state — Jonathan self-commits; explicit
    pathspecs; LFS-aware; NO push; no branch files, no L_Arena/DA): SK_Archer/SK_Ogre + their A_* clips + rigged/Meshy raw
    FBXs (TASK-242/243) + the owed Sapper anims (TASK-234 output) + MI_Castle_PBR + handoffs/QA reports per convention.
    Structural readback before commit (SK bind, slots, anim paths, SM_ fallback intact). Flip board lines. ACCEPTANCE:
    clean commit(s) + hash posted; owed-items ledger cleared or carried with reasons. Post in 🔧 Build & Git.
- names: >
    Commits on `main` via worktree. Law: CLAUDE.md hard gates, M7.6 branch-ownership.

---

#### TASK-245 — Lightning stock-node rework + staged capture/swap slice (art, editor)
- assignee: art-director
- status: done (2026-07-22 — stock-node rework SHIPPED + canonical swap + PIE-verified. Fresh-build approach: broken custom-HLSL material deleted (referencer-checked), rebuilt stock-nodes-only (~120 expressions, compiles in seconds) over a generated 1024² branching-bolt atlas `T_Spell_LightningStrike_E`; all look values are MI parameters (`MI_Spell_LightningStrike`). KEY PIVOT: sprite-VF TexCoord is garbage in the VS (bisect-proven) — UV-based WPO geometry rebuild only works on MESH renderers, so the system base moved to the Ice FrontSpike mesh-emitter donor with `/Engine/BasicShapes/Plane` overridden per particle; `NS_Spell_Lightning_NEW` (sprite lane) deleted after the pivot. Canonical `/Game/VFX/NS_Spell_Lightning` live (placeholder deleted at 0 refs, rename-in, compiled); material renamed to canonical `M_Spell_LightningStrike`. Look verdict PASS vs directive: tall 3400-uu branching bolts, honest 700-uu dashed ring (overhead witness-cube verified; exact radius, center wanders ≤~100uu early from baked donor cone drift — module lane closed, accepted+recorded), core flash. PIE: live L_Arena match, resolver-signature cast rendered (PIE245_f10). LIMIT: bot rule-3b UNOBSERVABLE — neither curated bot deck contains Lightning (manager flag: 3b is dead code in curated play). Strays for build-master: `Content/VFX/M_T245_WpoTest.uasset` (delete after editor close), dormant `M/MI_Spell_LightningFlash` (KEEP — referenced by disabled sprite renderers). Full ledger: handoffs/TASK-239.md FINAL SESSION. Captures: Tools/ArtPipeline/Cache/TASK-239/AFTER5_* + PIE245_f10.png. ASSET SET COMMITTED to main at the 2026-07-22 final overnight data+VFX sweep (Jonathan's blanket overnight approval closed the manager-flagged commit window; hash in 🔧 Build & Git). Branch-side residue owed at the merge gate: NS_Spell_Lightning_NEW deletion + old M_Spell_LightningStrike branch blob supersede — both branch-tracked via 02eda0f, recorded in the merge-gate notes)
- blocked-by: none (editor is CLOSED — this task RELAUNCHES it; launching is agent-lawful, Jonathan is present — coordinate)
- parallel-safe: no (editor)
- spec: >
    Re-author `M_Spell_LightningStrike`'s look with STOCK nodes/parameters ONLY (the CONVENTIONS Custom-HLSL law) hitting the
    recorded design intent from handoffs/TASK-239.md: tall ~3,400-uu BRANCHING sky strike, honest 700-uu ground ring (visual =
    hitbox, honest-scaling law), core flash. `NS_Spell_Lightning_NEW` + the staged capture/swap scripts SURVIVE and are
    reusable — do not rebuild them. Then run the staged slice: captures → look verdict (Jonathan present — his eyeball) →
    CANONICAL SWAP into `/Game/VFX/NS_Spell_Lightning` (composed-path law, no rename) → PIE verify incl. bot rule-3b cast
    (LogSiegeBot). ACCEPTANCE: stock-node-only material (no Custom HLSL), strike reads tall/branching/detailed over the
    honest 700 ring, canonical path live, PIE + bot verified, saved not-dirty. Post in 🎨 Art.
- names: >
    `M_Spell_LightningStrike` (stock nodes), `NS_Spell_Lightning_NEW` (WIP donor) → canonical `/Game/VFX/NS_Spell_Lightning`.
    Law: CONVENTIONS Custom-HLSL law, "Spell delivery overhaul" (honest scaling), composed-path law.

#### TASK-246 — Branch junk-strip commit on m7.6-arena10x (build, git — direct window, editor closed)
- assignee: build-master
- status: **done** (2026-07-22 — branch-tip commit **`ec7a271`** on `m7.6-arena10x` (parent 02eda0f), direct window verified [branch checked out, no UnrealEditor.exe], no lock errors. 20 files: DELETED AB_Test/ 11 uassets + 5 Footman *.fbm/texture_0.png extraction dirs (source FBXs kept) + 2 __pycache__ .pyc; UNTRACKED Docs/GDD-Submission-v3.pdf (--cached; verified still on disk + now shows `!!` ignored); .gitignore guards added to the project-level GitClaudeUnrealTest/.gitignore (`__pycache__/`, `*.fbm/`, `Docs/GDD-Submission-*.pdf`). KEPT M_Spell_LightningStrike + NS_Spell_Lightning_NEW (tracked + on disk for TASK-245). Merge-tree recheck: main↔branch conflict set unchanged = TASKBOARD.md only. Board/CONVENTIONS working-tree dirt NOT staged. NOT pushed — push-pending, Jonathan's call. Handoff: handoffs/TASK-246.md.)
- blocked-by: none
- parallel-safe: no (Git; serialize with TASK-247)
- spec: >
    ON THE BRANCH TIP (tree is checked out on m7.6-arena10x, editor closed — direct commit, no worktree): strip the
    02eda0f-swept debris — (a) DELETE (git + disk) `Content/Characters/Anims/AB_Test/` (11 uassets — scratch slated for
    deletion per TASK-221-222); (b) DELETE the 5 `Meshy/Footman/*.fbm`/texture_0.png extraction dirs; (c) DELETE the 2
    `__pycache__` .pyc; (d) `git rm --cached Docs/GDD-Submission-v3.pdf` — **KEEP ON DISK** (Jonathan's homework file).
    Add .gitignore entries: `__pycache__/`, `*.fbm/`, `Docs/GDD-Submission-*.pdf`. **KEEP** the two TASK-239 WIP VFX assets
    (M_Spell_LightningStrike + NS_Spell_Lightning_NEW — legitimate WIP, TASK-245 consumes them). ONE commit on the branch.
    NOTE: Jonathan PUBLISHED the branch → this commit is PUSH-PENDING; HE decides whether to push — DO NOT push. ACCEPTANCE:
    clean branch-tip commit, debris gone, homework file on disk, WIP kept, hash posted. Post in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` tip + `.gitignore`. Law: CLAUDE.md hard gates (no push), M7.6 branch-ownership.

#### TASK-247 — TASKBOARD.md mojibake repair (scripted, quiet-window) (build)
- assignee: build-master (manager ruling: needs shell for a scripted round-trip repair + verification — not hand-edits)
- status: **done** (2026-07-22 — scripted two-pass sloppy-cp1252→utf-8 repair: 2,335 lines fixed (2,333 whole-line round-trip + 2 mixed-line run-scoped, regions 7–485 + 634–6126), zero residual signatures outside the INTENTIONAL spec examples on lines 606–607 (excluded by design — never "fix" those); encoding-only diff proven (line-set equal to script log, ASCII projection identical per line, inverse re-corruption proof); anchors verified live (manager's W1-PREP insert + QA's TASK-248 flip both matched repaired anchors); CONVENTIONS.md scanned CLEAN both passes, untouched; snapshots + tool in scratchpad task247/, pre-repair sha 9177524a…, installed sha 5fbfd1ff…; NOT committed per ruling — rides the next MAIN docs window with the board churn. handoffs/TASK-247.md → COMMITTED to main at the 2026-07-22 pre-W1 bounce docs window)
- blocked-by: TASK-246 (serialize build-master git work; repair AFTER the junk-strip so the commit lanes stay clean)
- parallel-safe: no (single-writer on the board file)
- spec: >
    Repair the flagged UTF-8 double-encode corruption in older TASKBOARD.md sections (em-dashes as "â€"-class sequences,
    "Ã—", "ðŸ" emoji wrecks — introduced by a recent non-UTF-8 board write). SCRIPTED pass only (ftfy-style / cp1252→utf-8
    round-trip on affected sequences), NEVER a blind mass re-encode: verify byte-for-byte that ONLY mojibake sequences
    change, then verify anchors/emoji/dashes render correctly (spot-check the known-corrupt TASK-241 status + M7.6 heading
    tail) and that agent-used anchor strings match again. Check CONVENTIONS.md for the same signature while in there (repair
    if found). Commit rides the next docs window on MAIN. ACCEPTANCE: zero mojibake signatures remain; content-identical
    otherwise (diff = encoding only); verification noted. Post in 🔧 Build & Git.
- names: >
    `.claude/pipeline/TASKBOARD.md` (+ CONVENTIONS.md check). Law: files-are-the-contract (board integrity).

#### TASK-248 — Spell-lane follow-ups: SpellDelivery header cell + stale-comment sweep (files)
- assignee: gameplay-programmer
- status: **done** (2026-07-22 — committed to main at the pre-W1 bounce docs window, hash in handoffs/PRE-W1-BOUNCE.md + 🔧 Build & Git. QA trail: qa/TASK-248-qa.md PASS 0 blockers; diff exact, skip ruling confirmed correct, WARN-2 formally closed; 1 WARN = CONVENTIONS :207 registry fallback text wrong-for-tomorrow (unset⇒Auto not GroundCircle) → manager one-liner owed; merge-gate checklist gains the .h:247 one-number swap)
- blocked-by: none
- parallel-safe: yes (cards.csv header + code comments; no logic)
- spec: >
    (1) Formally append the `SpellDelivery` column to the cards.csv HEADER (QA WARN-2 closure) — registry law added first
    (CONVENTIONS "Spell delivery overhaul" → SpellDelivery column entry, 2026-07-22): value strings pinned by SpellLibrary.h,
    unset ⇒ legacy ground-circle; record the exact value set in the handoff. (2) Sweep the 4 stale "400" comment lines
    (TASK-240 checklist item — the 700 radius survived Jonathan's playtests without complaint). NO logic changes — comments +
    header only; git-diff confinement is the acceptance evidence. QA-LIGHT per manager norm: orchestrator-proxied diff review
    acceptable (no unreviewed LOGIC ever; this is header+comments). Post in ⚙️ Dev & QA.
- names: >
    `Docs/Data/cards.csv` (header), the 4 stale-comment sites (SpellLibrary/PlayerController lineage per TASK-240's
    checklist). Law: CONVENTIONS "Data-driven card stats", "Spell delivery overhaul" (SpellDelivery entry).

#### TASK-252 — D-SPELL-VISIBILITY deck tweak: FrostNova into the default deck + Lightning into one bot deck (data, TONIGHT)
- assignee: gameplay-programmer
- status: **done** (2026-07-22 final overnight window, build-master — COMMITTED to main at the data+VFX sweep (hash in 🔧 Build & Git; verified pre-commit: cards.csv delta vs main tip = exactly the 2 DeckCount cells, SiegeBotController.cpp delta = exactly the 3 constructor hunks, CSV line-ending normalization settled by git — stored delta content-sane). Bot half live since the pre-W1 bounce compile; player half live via the in-place DT_Cards reimport (same-object, sum 50 verified — TASK-252 handoff DT-append). ← was: qa-passed (2026-07-22 — orchestrator proxy diff review PASS: content delta = exactly 2 DeckCount cells + 3 constructor hunks; whole-file CSV diff appearance = line-ending normalization artifact, git settles at commit — noted for the final window. NOTE: the bounce's 647d7aa may have committed cards.csv pre-252-cells — final window verifies + sweeps the residue. Prior detail:) ← was: ready-for-qa (2026-07-22 — QA-LIGHT, orchestrator-proxied diff review per spec. DONE: FrostNova DeckCount 0→1 in cards.csv, donor Footman 12→11 (largest/cheapest/most-redundant stack; Fireball-donor rejected as anti-purpose) — sum 50, caps hold, TASK-248 SpellDelivery column preserved byte-exact; Lightning ×2 into BotDecks[1] "Bot Defensive Economy", donor Wall 10→8 (single-donor, no DPS/economy loss, identity intact) — sum 50, Lightning 2/2 cap exact, avg 6.86→7.02, closes the M6 open item. LANE RULING: SiegeBotController.cpp byte-identical main b90157e ↔ branch ec7a271 (blob e48be2c both tips; only the .h diverges, untouched) → edit lane-neutral, TASK-248 precedent. DEPENDENCIES: bot half live at the pre-W1 bounce compile (rides TASK-250's owed compile — no separate tonight window); player half live at the DT_Cards reimport — NOT done, owed to the art pair's editor window / final overnight commit slot. IsDeckLegal traced pass ×2, LogSiegeBot deck names unchanged. Donor reasoning prominent in handoffs/TASK-252.md for Jonathan's morning review)
- blocked-by: none
- parallel-safe: yes (cards.csv + SiegeBotController deck arrays — disjoint from the art pair's editor/material/DA files)
- spec: >
    Jonathan's ruling implements BOTH halves. (1) FROSTNOVA: bump DeckCount 0 → 1–2 in cards.csv; KEEP THE 50-SUM LAW — pick
    the LEAST balance-sensitive donor card to decrement, each count ≤ MaxCopies, and RECORD the donor choice + reasoning in
    the handoff for Jonathan's review. (2) LIGHTNING → ONE bot curated deck: LOCATE the M6-era bot decks first (M6 decision 4
    says `TArray<FDeckList> BotDecks` constructor defaults on ASiegeBotController, EditDefaultsOnly — verify on disk, data
    vs C++ vs per-BP tune); add Lightning ×1–2 to ONE deck — DEFAULT: ×2 into "Defensive Economy" per the recorded M6 QA
    recommendation (this closes that old open checkpoint item) — keeping THAT deck legal at its total (decrement inside it,
    record the donor). Verify: both deck sums re-add to 50, caps hold, `UDeckLibrary::IsDeckLegal` would pass both; grep
    LogSiegeBot deck-pick still names both decks. DT_CARDS REIMPORT dependency: fold the reimport into tonight's REMAINING
    editor window (the art pair holds the editor next — coordinate the slot; no PIE between reimport and the window's end
    checks). The final overnight commit carries the data (explicit pathspecs). QA-LIGHT (data + at most one array edit; no
    logic — orchestrator-proxied diff review acceptable, no unreviewed logic ever). ACCEPTANCE: FrostNova reachable in the
    default deck, Lightning in one bot deck (rule 3b live in curated play), sums/caps verified, donors recorded, reimport
    scheduled in-window. Post in ⚙️ Dev & QA.
- names: >
    `Docs/Data/cards.csv` (FrostNova DeckCount + donor decrement) + `Source/GitClaudeUnrealTest/Siegebound/
    SiegeBotController.cpp` (BotDecks array — locate first). Law: CONVENTIONS "Data-driven card stats" (50-sum, MaxCopies,
    single-source), "Deck-builder & saved decks (M6)" (IsDeckLegal single validator), M6 decision 4 (bot decks).

---

