<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-342 — [FID-archer-pikeman] Archer + Pikeman chroma-fidelity rework: pin the manifest → alpha-mask re-measure → Stage-2 iterate vs the CHROMA gate (art-director, NO editor, ZERO credits)
- assignee: art-director
- status: **done** (2026-07-27 night — INTEGRATED at TASK-343: both `_D` textures same-path imported [MD5-readback-proven], sampler sweep 0 hits, Simulate-verified on L_Arena, committed per-unit. ← was ready-for-integration 2026-07-27 late — BOTH UNITS PASS the chroma gate at ZERO credits: Archer chroma 0.7594→**0.9682** / luma 1.5597→**1.0141** [bleach breach healed]; Pikeman chroma 0.5685→**0.7805** [donor-capped ≈0.8, recorded] / luma 1.1315; hue shifts ≤6°; floors held. Overrides pinned: Archer γ1.0/g1.7, Pikeman γ1.0/g2.3 — root cause = γ0.55 per-channel compresses RGB ratios. **Delivery = TEXTURE-SET-ONLY, both units: only `T_<U>_D.png` changed; FBX/N/ORM byte-identical → no re-rig, no LOD churn.** Turnkey recipe `handoffs/TASK-342-artist.md`; galleries `Tools/ArtPipeline/Cache/_task342/` + committed copies `handoffs/TASK-342-BEFORE_AFTER_*.png`. 4 informational manager flags in the handoff incl. Pikeman's donor ceiling + a v2 gate-basis suggestion)
- blocked-by: none — **but SERIALIZE with TASK-340 (ruling 7: shared `pipeline_manifest.json` — never concurrent)** ✅ cleared (340 done)
- parallel-safe: yes vs everything EXCEPT TASK-340 (manifest write race)
- spec: >
    **STEP 0a — PIN THE MANIFEST (FIRST ACTION, blocking — ruling 3):** write `assets.Archer.albedo_delight = {ao_divide_strength 1.0,
    ao_floor 0.25, gamma 0.55, gain 1.2}` (the locked profile) into `Tools/ArtPipeline/pipeline_manifest.json` — TASK-312's pin was LOST
    post-crash and a bare Stage-2 re-run bakes the dark script defaults `{0.6, 0.35, 0.85, 1.0}`, regressing Archer DARKER. Verify
    Pikeman's existing pin while there. Quote both blocks in the handoff.
    **STEP 0b — MEASURE (alpha-masked, method validated to 4 dp vs recorded fleet values):** re-measure Archer luma retention with the
    concept ALPHA-MASKED (the ~0.75 corner-sample reading is backdrop-contaminated — ruling 3.2); then the first-ever chroma numbers for
    BOTH units: chroma retention (name the metric — CIELAB C*ab preferred), top-2 dominant-cluster hue shift, luma retention, UV-norm
    albedo. These are the "before" table and they steer the iteration.
    **STEP 1 — STAGE-2 ITERATE, 0 credits:** re-run Stage 2 from the CACHED fresh donors `Tools/ArtPipeline/Cache/{Archer,Pikeman}/
    meshy_raw.glb` (2026-07-26 batch; ~10 s/iteration), tuning for SATURATION/HUE fidelity vs the alpha-masked concept.
    **GATE (CONVENTIONS "CHROMA-FIDELITY GATE", in full):** chroma retention ≥ 0.60× (target ≈1.0×) · >1.35× = oversaturation FLAG ·
    hue shift ≤ 20° · luma retention 0.85–1.25× (**Pikeman MUST NOT breach 1.25×** — it starts at 1.13–1.24; Archer's band anchors on its
    RE-MEASURED value) · UV-norm ≥ 0.2536 maintained. A bake failing the gate is not a deliverable — iterate (free) or invoke the escape
    valve: **if the gate is unreachable, or a unit already measures ≥ 0.60× yet still reads chalky, STOP and route the numbers to the
    manager. Do NOT escalate to fresh image3d yourself** (ruling 4 — manager go required; Pikeman fresh-gen also needs Jonathan's
    pose-reroll answer BEFORE any spend; balance 2446).
    **STEP 2 — DELIVERY FORM (ruling 6):** verify whether the re-baked UV layout is IDENTICAL to shipped. Identical ⇒ deliver
    TEXTURE-SET-ONLY (`T_<Unit>_{D,N,ORM}` same-path sources). Shifted ⇒ full Stage-B re-rig onto the shared `SK_Footman_Skeleton`
    (anims byte-untouched) + SM+SK sources per the fleet template. State which, PER UNIT, in the turnkey recipe.
    **STEP 3 — TURNKEY RECIPE** in `handoffs/TASK-342-artist.md`: per-unit sources → same-path destinations, sRGB flags, the delivery
    form, expected readbacks (incl. LOD chains if meshes move), the before/after measurement table (all gate metrics), the manifest
    diffs. **NO editor, NO MCP, NO Git, NO Meshy generation.** Post before/after previews + the numbers in 🎨 Art.
- names: >
    Same-path targets: `/Game/Textures/T_{Archer,Pikeman}_{D,N,ORM}` (always) + `/Game/Meshes/SM_{Archer,Pikeman}` /
    `/Game/Characters/SK_{Archer,Pikeman}` / `/Game/Materials/Instances/MI_{Archer,Pikeman}_PBR` / raw
    `Content/RawAssets/Characters/{Archer,Pikeman}.fbx` (only if UVs shift). Concepts `Content/RawAssets/Concepts/{Archer,Pikeman}.png`
    (approved as-is; ALPHA-MASK for measurement). Donors `Tools/ArtPipeline/Cache/{Archer,Pikeman}/meshy_raw.glb`. Manifest
    `Tools/ArtPipeline/pipeline_manifest.json` (Archer pin = ruling 3). PRESERVED: `A_{Archer,Pikeman}_*` + ABP, `SK_Footman_Skeleton`.
    Report `handoffs/TASK-342-artist.md`. Law: CONVENTIONS "Fleet Meshy remaster" → **CHROMA-FIDELITY GATE + ALPHA-MASK METHOD LAW**
    (NEW 2026-07-27) + per-asset `albedo_delight` override + same-path/preserve-anims.
#### TASK-343 — [FID-int] Import + verify the fidelity rework in Simulate + per-unit commits (build-master)
- assignee: build-master
- status: **done** (2026-07-27 night — texture-only per the TASK-342 delivery form, `_D`-pair ONLY: same-path imports MD5-readback-proven [Archer `ea2e4b0a…` / Pikeman `269c7354…` = the new PNGs, sRGB ON, TC_Default], sampler sweep 0 hits, Simulate verify on L_Arena PASS [team recolor both teams, capsules READ 90/95, bone-delta anims tick, logs 0/0/0/0/0], TWO commits on main NO push: Archer **`3770bf6`** [+ manifest with BOTH pins + TASK-342 handoff], Pikeman = the commit carrying this line [+ TASK-343 handoff + galleries + 6 verify shots]. `L_Arena` never saved. WATCH: Jonathan's eye final — Archer deliberately darker than the old bleach; Pikeman donor-capped ≈0.8 chroma. `handoffs/TASK-343-buildmaster.md`)
- blocked-by: TASK-342
- parallel-safe: no (EXCLUSIVE editor + Git; EDITOR-GATED — serialize with the whole editor queue, never during Jonathan's PIE; may batch BOTH units in one session, SEPARATE commits)
- spec: >
    Exclusive editor on **main**, Simulate STOPPED for every import/save. Execute the TASK-342 turnkey recipe PER UNIT:
    **(1)** Same-path import per the recipe's delivery form — texture-set-only (EXPLICIT `T_<Unit>_{D,N,ORM}` imports + landed-readback:
    dimensions/size/timestamp, the TEXTURE-SKIP TRAP law) or the full SM+SK chain (then: shared-skeleton gate — `SK_Footman_Skeleton`
    sole, stray = HARD FAIL; LOD reapply + FULL readbacks — SM ≈15000/7500/3750/1874 + welded verts (unweld/0-tri = HARD FAIL, restore,
    don't commit), SK `lod_count==3` + URO; anims still bound).
    **(2) SAMPLER-TYPE TRAP sweep:** all material referencers of the touched `T_*` compile; `Failed to compile Material` grep = 0 on a
    fresh load.
    **(3) VERIFY in Simulate on `L_Arena`:** each unit reads SATURATED and matches its ALPHA-MASKED concept (side-by-side capture,
    gallery framing for direct before/after — the chalky grey-teal/bleached-cream read is GONE); team recolour intact both teams
    (Pikeman's 10.31% band is DELIBERATE — ruling 5, do not flag it); anims TICK (live bone-delta); facing/grounding OBSERVED only
    (capsules Archer 90 / Pikeman 95 — READ them); Message Log clean (ensure/AccessedNone/Fatal = 0 + the (2) grep).
    **(4) COMMIT PER UNIT — two separate commits** (per-unit revert model), explicit pathspecs per the delivery form + manifest +
    board/handoff; `git diff --stat` clean of anything foreign; **NO push**. `L_Arena` NEVER saved; `git reset --hard`/`git clean -fd`
    BANNED. **WATCH: Jonathan's eye final** — say so in the handoff. Post readbacks + before/afters + both hashes in 🔧 Build & Git.
- names: >
    Per-unit commit sets per the TASK-342 recipe (`T_`/`SM_`/`SK_`/`MI_` + raw FBX as applicable + `pipeline_manifest.json` + docs).
    READ-ONLY: `BP_Unit_{Archer,Pikeman}`, `DT_Cards`, `L_Arena`, `A_*` anims + ABP. Law: CONVENTIONS "Fleet Meshy remaster"
    (CHROMA-FIDELITY GATE · texture-skip trap · SAMPLER-TYPE TRAP · same-path), the hard gate.
#### TASK-344 — [CMD-3zone] Group orders: 3-stage HOLD flow + AMBUSH + wheel resize + spread + leashes — full C++ (gameplay-programmer, file-only)
- assignee: gameplay-programmer
- status: **done** (2026-07-27 night — INTEGRATED at TASK-346's feature commit: compile GREEN 16 s 0 warn, full PIE matrix PASS on L_Arena [flow/tiers/leashes/escalation/release/steal/all-dead-prune/cancels/teardowns/no-regression all observed, logs clean — `handoffs/TASK-346-buildmaster.md` row-by-row]. ← was qa-passed 2026-07-27 evening — `qa/TASK-344.md` PASS, 0 BLOCKER / 1 WARN / 3 NIT, all 11 flagged decisions ACCEPTED; WARN = manager doc-sync of the CONVENTIONS "unfrozen" wording [spell-frozen units ARE select-eligible] — STILL OPEN for the manager)
- parallel-safe: yes (sole owner of the five files this pass; disjoint from both art lanes)
- blocked-by: none
- spec: >
    **DESIGN AUTHORITY — implement, don't re-design:** the plan file `C:\Users\wesel\.claude\plans\groovy-bouncing-manatee.md`
    ("Lane B", verified file:line citations) + CONVENTIONS **"Group orders — 3-zone HOLD + AMBUSH"** (types, tunables, wheel-polling,
    behavior + byte-identical laws — all names are LAW). Files: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}`,
    `SummonedUnit.{h,cpp}`, `UnitCommand.h`. In brief (the law has the full detail):
    controller-owned `TArray<FSiegeUnitGroup> UnitGroups` + `ESiegeGroupCommandType {Hold, Ambush}` + private
    `EGroupPickStage {None, Select, Position, AttackZone}`; R (existing) and F (new, soft-ref `IA_CmdAmbush`, **inert-null-safe until
    the asset exists**) enter the 3-stage pick; POLLED wheel resize (`WasInputKeyJustPressed(EKeys::MouseScrollUp/Down)` in the pick
    branch of PlayerTick; step 100, clamp 200–5000; stage defaults 1200/700/1500); `SpawnGroupCircleDecal` reticle-recipe clone
    (identity spawn → absolute −90 pitch; `DecalSize=(500,R,R)`; `M_SpellReticle` null-safe); active circle follows the cursor, each
    confirm drops it + spawns the next; final confirm transfers the Position+Attack circles to the group as PERSISTENT markers;
    RMB/Esc = full-flow cancel at any stage; empty stage-1 selection = refuse-and-stay + prompt; sky-trace refusal as today; the ONE
    teardown `CancelGroupPick()` (melee-release-before-early-out) swapped into ALL 8 teardown sites; the OLD `HoldRadius`/`HoldLocation`
    + four `*HoldTarget` functions DELETED (`ESiegeUnitCommand` byte-identical — the Hold member stays declared). Unit side:
    `CommandGroupId` + `GroupStationOffset` (golden-angle sunflower, computed once at confirm, nav-projected), `UpdateStateGrouped`
    ABOVE the stance gate, priority ladder + anti-thrash stickiness (single monotone position→attack upgrade), HOLD both-zones leash vs
    AMBUSH chase-to-kill leash-exemption, null-group SELF-HEAL to the legacy stance gate, new public `IsGroupCommandEligible()`
    (Standard + Blue + alive + unfrozen), 150 uu arrival vs station, **TASK-275 kite-fix untouched**. `FOnCommandPromptChanged`
    (FString) with stage prompts, ALSO logged (feature ships without the BP bind). T/E clears all groups; death clears; Play-Again
    resets; ≤1 s all-dead prune.
    **MUST HOLD:** the CONVENTIONS byte-identical set (legacy post-gate body, AcquireTarget, Siege/Support/miner paths, bot/Red,
    ATTACK/DEFEND, placement + spell targeting, SetUnitCommand/HUD stance display, combat/economy/match-flow) — and the feature is
    FULLY FUNCTIONAL for HOLD via R with ZERO editor assets. **Compile traps:** no literal `*/` in doc comments; Printf formats
    literal/`constexpr` (TCheckedFormatString).
    File-only: no compile, no editor, no MCP, no Git. Write `handoffs/TASK-344-programmer.md` (per-file delta map, the 8 teardown-site
    swaps enumerated, the stickiness/leash truth table, the byte-identical argument). Post in ⚙️ Dev & QA; then ready-for-qa.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` · `SummonedUnit.{h,cpp}` · `UnitCommand.h`. New names per
    CONVENTIONS "Group orders" (ESiegeGroupCommandType · FSiegeUnitGroup · EGroupPickStage · UnitGroups · FindUnitGroup ·
    CancelGroupPick · SpawnGroupCircleDecal · CommandGroupId · GroupStationOffset · UpdateStateGrouped · IsGroupCommandEligible ·
    FOnCommandPromptChanged · the six tunables). Soft-ref `IA_CmdAmbush` (asset lands at TASK-345). Report
    `handoffs/TASK-344-programmer.md`.
#### TASK-345 — [CMD-editor] `IA_CmdAmbush` + F mapping + WBP_HUD additive prompt bind (art-director, editor)
- assignee: art-director
- status: **done** (2026-07-27 night — all three deliverables + the optional StageTint DONE inside TASK-346's session (`handoffs/TASK-345-artist.md`); PIE-verified by TASK-346: F opens the AMBUSH pick (IA+IMC live), prompts broadcast+logged, StageTint MID params read back white/green/red per stage; committed in TASK-346's ONE feature commit) ← was backlog
- blocked-by: TASK-344-QA (qa-passed) — **and the TASK-344 module must be COMPILED in the live editor: runs INSIDE TASK-346's session,
  immediately after 346's compile step** (ruling 11; TASK-330/331 shared-session precedent — own deliverable, no Git)
- parallel-safe: no (EDITOR-GATED; serialize with the whole editor queue)
- spec: >
    Three small editor deliverables, NOTHING else:
    **(1)** Create `/Game/Input/Actions/IA_CmdAmbush` (Digital/bool — the exact `IA_Cmd*` pattern).
    **(2)** Map it to **F** in `/Game/Input/IMC_Hero` — **FIRST confirm F is unmapped in-editor** (exploration says free, but IMC_Hero
    is binary); any existing F mapping ⇒ FLAG to the manager, never stomp (the E/R/T precedent).
    **(3)** `WBP_HUD`: bind `OnCommandPromptChanged` ADDITIVELY — a prompt text block that shows the pushed FString and, when EMPTY,
    falls back to the existing stance display. Do NOT touch the stance switch/pins (`ESiegeUnitCommand` byte-identity), do NOT
    duplicate+reparent anything (the corruption lesson — this is an edit to the EXISTING widget's graph only).
    **OPTIONAL, NON-BLOCKING (flag if done):** add a stage-tint colour param to `M_SpellReticle` (STOCK NODES ONLY — Custom-HLSL ban);
    the C++ hook is a silent no-op until the param exists, so skipping it costs nothing.
    Save the three assets (Simulate stopped); **no Git** (TASK-346 commits), no `L_Arena` save, no C++ edits. Readback-verify the F
    mapping + the bind; hand the editor back to build-master for the PIE matrix. Write `handoffs/TASK-345-artist.md`; post in 🎨 Art.
- names: >
    NEW `/Game/Input/Actions/IA_CmdAmbush` · edit `/Game/Input/IMC_Hero` (F mapping) · edit `/Game/UI/WBP_HUD` (additive
    `OnCommandPromptChanged` bind). Optional: `M_SpellReticle` colour param (stock nodes). Law: CONVENTIONS "Group orders" (input-asset
    clause), "Material & Niagara lane laws", the widget rules. Report `handoffs/TASK-345-artist.md`.
#### TASK-346 — [CMD-int] Compile + host TASK-345 + the full PIE matrix + ONE feature commit (build-master)
- assignee: build-master
- status: **done** (2026-07-28 — compile GREEN (16 s, 0 warn), TASK-345 hosted, FULL PIE matrix PASS (real PIE on L_Arena, bot playing; numeric readbacks + 5 verify shots; row-by-row in `handoffs/TASK-346-buildmaster.md` incl. the Esc-stops-PIE note, the stage-tint-subtlety feel item, and one non-reproduced T-idle WATCH), logs ensure/AccessedNone/Fatal/material-fail = 0; ONE feature commit on main (hash in 🔧 Build & Git), NO push, L_Arena never saved) ← was backlog
- blocked-by: TASK-344-QA (qa-passed); TASK-345 INTERLEAVES (this task compiles → hands the editor to 345 → resumes for PIE + commit)
- parallel-safe: no (EXCLUSIVE editor + Git; EDITOR-GATED — serialize with the whole queue, never during Jonathan's PIE)
- spec: >
    **(1) COMPILE** TASK-344 (code hard gate — a failure appends to `qa/TASK-344.md` and routes back to gameplay-programmer, counting
    as a QA loop). Editor-bounce so the new classes/delegate are live.
    **(2) HOST TASK-345** in this editor session (art-director's three assets), then resume.
    **(3) PIE MATRIX on `L_Arena` (real PIE — this feature needs the player pawn; observe EVERY row, capture the interesting ones):**
    (a) FLOW: R → three sequential cursor-following circles, each wheel-resizable with visible min/max clamps, earlier circles staying
    visible; sky-cursor hides + refuses; empty stage-1 selection refuses with the prompt; after stage 3 the Position+Attack circles
    PERSIST as group markers. (b) TIERS: attack-zone engage · position-zone engage · walk-spread-idle with NO re-path jitter over 30 s
    (the sunflower spread visibly distributes — no point-milling). (c) LEASHES: HOLD — kite a target out of BOTH zones ⇒ disengage +
    return to station; AMBUSH (F) — chase to the kill then return; tier escalation WITHOUT target ping-pong. (d) RELEASE: T/E clears
    every group + markers; a new R/F STEALS already-grouped units; all-members-dead removes markers ≤ 1 s. (e) CANCELS at every stage
    (RMB/Esc); match-end / Play-Again / hero-death teardown clean. (f) NO-REGRESSION sweep: stances T/E, legacy pre-command behavior,
    bot/Red untouched, miners + Siege/Support unaffected, placement + spell targeting intact, wheel INERT outside the flow.
    (g) Message Log clean (ensure / AccessedNone / Fatal = 0) + the `Failed to compile Material` grep (touched WBP/material assets).
    **(4) ONE FEATURE COMMIT** on main (ruling 11): `SiegePlayerController.{h,cpp}` + `SummonedUnit.{h,cpp}` + `UnitCommand.h` +
    `Content/Input/Actions/IA_CmdAmbush.uasset` + `Content/Input/IMC_Hero.uasset` + `Content/UI/WBP_HUD.uasset` (+ `M_SpellReticle` only
    if 345 did the optional param) + board/CONVENTIONS/handoffs/qa. `git diff --stat` clean of anything foreign; **NO push**; `L_Arena`
    NEVER saved; `git reset --hard`/`git clean -fd` BANNED.
    **(5) FLAG FOR JONATHAN'S FEEL-PASS (record in the handoff + 🔧 post):** wheel sensitivity (step 100), circle readability, spread
    feel, the six tunables, and AMBUSH-vs-building semantics (default shipped: besiege until destroyed). Post the matrix results +
    commit hash in 🔧 Build & Git.
- names: >
    Commit set: the five source files + `IA_CmdAmbush` + `IMC_Hero` + `WBP_HUD` (+ optional `M_SpellReticle`) + docs, ONE commit on
    main, no push. Law: CONVENTIONS "Group orders — 3-zone HOLD + AMBUSH", the hard gate (PASS QA before commit), the widget rules.

---

