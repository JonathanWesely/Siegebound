<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-330 — [CASTLE-int] Same-path `SM_Castle` import + LOD chain + hard LOD readback + Simulate verify + commit (build-master)
- assignee: build-master
- status: **done** (2026-07-27, commit `fcb1ec0`) — same-path import + explicit textures; hard LOD gate PASS (lod_count 3, tris 20000/10000/5000, LOD0 24,333 verts ≠ tris×3, bounds 814.52×820.56×894.87); Simulate verify (a)–(e) PASS both castles; handoff `handoffs/TASK-330-buildmaster.md`. Raw sources were already committed in `7bedf58`.
- blocked-by: TASK-329
- parallel-safe: no (EXCLUSIVE editor + Git; EDITOR-GATED — serialize with every other editor task and never during Jonathan's PIE)
- spec: >
    Exclusive editor session on **main**, **Simulate STOPPED before any import** (lane-knowledge 8 — package saves are silently blocked
    while Simulate runs; that is exactly how the broken `SM_MilitiaMob` chain reached a commit). Execute the turnkey recipe in
    `handoffs/TASK-329-artist.md`.
    **(1) SAME-PATH IMPORT — never delete+recreate:** overwrite `/Game/Meshes/SM_Castle` from `Content/RawAssets/Castle.fbx`, and
    `/Game/Textures/T_Castle_{D,N,ORM}` from `Content/RawAssets/Textures/Castle/`; point/refresh `/Game/Materials/Instances/MI_Castle_PBR`
    at them. `_D` sRGB **ON**, `_N` + `_ORM` **LINEAR**. Nanite **OFF**.
    **(2) LOD CHAIN — the CASTLE LANDMARK EXCEPTION, NOT `LargeProp`:** explicit reduction **LOD1 50 % @ screen 0.4 / LOD2 25 % @ 0.15**
    ⇒ readback **`lod_count == 3`**. Do NOT apply `lod_group='LargeProp'` to this asset (manager ruling 5).
    **(3) HARD LOD GATE (manager ruling 6 — the `SM_MilitiaMob` lesson):** read back **LOD0 welded vertex count AND per-LOD triangle
    counts** and quote them in the handoff. **A 0-triangle LOD, or LOD0 verts == tris × 3 (the unweld signature), is a HARD FAILURE:**
    restore the last-known-good `SM_Castle`, **do NOT commit**, append the evidence to the handoff and route back to the manager.
    `lod_count` alone does NOT catch this. Never "fix" it by delete+recreate.
    **(4) VERIFY IN SIMULATE on `L_Arena` (never PIE-in-viewport — lane-knowledge 1):** (a) both castles render the new VIVID warm
    sandstone under the real arena sun and are **no longer darker than the grass** — capture the same framing as
    `handoffs/TASK-309-audit-Castle-in-arena.png` so the before/after is directly comparable; (b) **team colour still reads on BOTH** —
    Blue castle blue, Red castle red (slot 0 `TeamRegion` intact and first); (c) bounds/footprint unchanged within ±10 % and the castle
    still sits on the ground at its anchor; (d) placement still refuses inside the plinth dead-zone (`CastlePlinthClearance`); (e) Message
    Log clean (ensure / AccessedNone / Fatal = 0). **Locate the castles by `TActorIterator<ACastle>` / class filter, NEVER by actor label**
    (convention says `Castle_Blue`/`Castle_Red`, the audit observed `Castle_0`/`Castle_1`; `Castle_Red` carries yaw 180 **[claim CORRECTED 2026-07-28: never reproduced, both castles yaw 0.0 — see the CASTLE-3X record correction]**) — manager ruling 8.
    **`L_Arena` IS NEVER SAVED.** At any editor-close save prompt: SAVE the `SM_`/`T_`/`MI_` Castle assets, **DECLINE `L_Arena.umap`**.
    **(5) COMMIT** on main with explicit pathspecs (`Content/Meshes/SM_Castle.uasset`, `Content/Textures/T_Castle_*.uasset`,
    `Content/Materials/Instances/MI_Castle_PBR.uasset`, `Content/RawAssets/Castle.fbx`, `Content/RawAssets/Textures/Castle/*`,
    + board/CONVENTIONS/handoff). `git diff --stat` must show nothing foreign. **NO push.** **`git reset --hard` / `git clean -fd` are
    BANNED** (standing lesson 2 — this board was destroyed once today by exactly that).
    Post the readbacks + before/after shots + commit hash in 🔧 Build & Git. **TASK-331 is REQUIRED to follow — the castle is not
    considered shipped until the crumble stages are re-derived; say so in the handoff.**
- names: >
    Same-path: `/Game/Meshes/SM_Castle` ← `Content/RawAssets/Castle.fbx`; `/Game/Textures/T_Castle_{D,N,ORM}` ←
    `Content/RawAssets/Textures/Castle/`; `/Game/Materials/Instances/MI_Castle_PBR`. UNTOUCHED: `L_Arena`, `ACastle` C++, every Blueprint,
    `SM_Castle_Crumble0N` (TASK-331 owns those). Commit on main, no push. Law: CONVENTIONS "Castle remaster — the BUILDING same-path
    variant" (LOD + bounds + slot order), "Fleet Meshy remaster" → **SAME-PATH STATIC REIMPORT — KNOWN TRAP**, the hard gate.

#### TASK-331 — [CASTLE-crumble] Re-derive `SM_Castle_Crumble01/02/03` from the rebuilt castle + verify the 75/50/25 % states + commit (build-master)
- assignee: build-master
- status: **done** (2026-07-27, commit `a442ad6`) — all 3 re-derived same-path from the rebuilt castle (bounds bit-identical, LOD 3-chain, both slots `MI_Castle_Crumble0N`); 75/50/25 % fired once each in order + ResetCastle restores pristine+team accent; UV mapping clean (no scramble). **⚠️ MANAGER FLAG:** vs the new bright base, stage 1 no longer reads "battle-worn but standing" — pristine→stage1 wall luma 77.5→28.0 and stages 1/2/3 measure 28.0/28.9/29.2 (visually flat between stages; stage 3 still reads near-dead charred, PASS). MI params NOT touched per ruling — see `handoffs/TASK-331-buildmaster.md`. **FLAG ADJUDICATED 2026-07-27 (manager): RETUNE ORDERED — TASK-337 (art re-spread) → TASK-338 (verify + commit); ruling block + the STAGE-LEGIBILITY band below TASK-333.** **RECORD CORRECTED same day (TASK-337 first-run finding): the stage renders this task measured were the engine DEFAULT MATERIAL — `M_CastleCrumble` has failed SM6 compilation since TASK-330's `T_Castle_ORM`→`TC_Masks` reimport (CONVENTIONS SAMPLER-TYPE TRAP). THE COMMIT STANDS — the derivation is exonerated (TASK-337 cross-test: crumble mesh + `MI_Castle_PBR` renders 0.957×P; bounds/LOD/collision/slot/stage-drive/ResetCastle readbacks are object-level and all real) — but the render-level claims are re-scoped: "UV mapping clean / char-speckle follows the walls" and the flag's own "stage 3 PASSES" observed the Default Material's grey mottle, not the crumble materials, and are VOID as material evidence. Fix chain: TASK-339 → 337 (measure-first) → 338.**
- blocked-by: TASK-330
- parallel-safe: no (EXCLUSIVE editor + Git; **MAY run inside TASK-330's editor session** — but it is its own deliverable and its own commit)
- **WHY THIS TASK EXISTS (manager ruling 4 — do not drop it):** `SM_Castle_Crumble01/02/03` are **byte-copy duplicates of the OLD
  `SM_Castle`** (TASK-157 recipe) and `M_CastleCrumble` samples the SAME `T_Castle_{D,N,ORM}` that TASK-330 just overwrote. Old geometry +
  old UVs + NEW textures = **scrambled texture mapping on every damage state**. Without this task the castle looks great at full HP and
  falls apart visually the instant it takes damage — on the game's win-condition actor. Nobody flagged this; it is not optional.
- spec: >
    Same exclusive editor session is fine (Simulate STOPPED). Re-run the TASK-157 derivation against the REBUILT castle:
    **(1)** Duplicate the rebuilt `/Game/Meshes/SM_Castle` **over** `/Game/Meshes/SM_Castle_Crumble01`, `…02`, `…03` — **same paths, never
    delete+recreate** (`ACastle` soft-references these strings character-for-character; `Castle.cpp:315-316,26`). A duplicate is a byte-copy,
    so UCX/simple collision + footprint come across IDENTICAL by construction — verify crumble bounds == pristine bounds exactly, and
    Nanite OFF inherited.
    **(2)** Re-assign `/Game/Materials/MI_Castle_Crumble01|02|03` to **BOTH slots** of its matching stage mesh (that is what makes the WHOLE
    castle read damaged rather than only the accent). **The MIs live at `/Game/Materials/`, NOT `Instances/`** — the code path is the
    contract and wins over the prefix-table folder row. **Do NOT re-author `M_CastleCrumble` or the three MIs** — they sample the same-path
    `T_Castle_*` and pick the new brighter albedo up for free. Keep the shipped stage tuning
    (01 `Darken 0.80 / Scorch 0.12 / RoughBoost 0.30`, 02 `0.50 / 0.45 / 0.60`, 03 `0.30 / 0.80 / 0.85`).
    **(3) VERIFY in Simulate on `L_Arena`:** drive one castle down through **75 % → 50 % → 25 %** and confirm each stage fires ONCE, in
    order, with a monotonic darken/char progression on the NEW geometry (textures mapped correctly — no scrambling, no UV smear), debris
    pops, and the progression still reads as the same granite. Then confirm **`ResetCastle` / Play Again restores the pristine rebuilt
    `SM_Castle` + the per-team accent** via `ApplyTeamVisuals`. Message Log clean.
    **(4)** Recheck the darkest stage against the new brighter base: stage 03 must still read as a near-dead charred silhouette, and stage
    01 must still read as clearly damaged rather than pristine. If the new brighter albedo makes a stage read wrong, **flag it to the
    manager with the observation — do NOT re-tune the MI parameters unilaterally** (that is an art call).
    **(5) COMMIT** on main, explicit pathspecs (`Content/Meshes/SM_Castle_Crumble01|02|03.uasset` + board/handoff), `git diff --stat` clean
    of anything foreign, **NO push**. `L_Arena` NEVER saved; `git reset --hard` / `git clean -fd` BANNED.
    Post the stage-progression shots + commit hash in 🔧 Build & Git.
- names: >
    Same-path duplicates: `/Game/Meshes/SM_Castle_Crumble01|02|03` ← the rebuilt `/Game/Meshes/SM_Castle`. Re-assigned (not re-authored):
    `/Game/Materials/MI_Castle_Crumble01|02|03` (master `/Game/Materials/M_CastleCrumble`, debris `/Game/VFX/NS_CastleDebris`).
    Code refs (read-only): `ACastle::ApplyCrumbleStage` / `ResetCastle` / `ApplyTeamVisuals`. Commit on main, no push.
    Law: CONVENTIONS "Castle remaster — the BUILDING same-path variant" → **CRUMBLE-DERIVATION LAW**; precedent `handoffs/TASK-157-artist.md`.

#### TASK-332 — [GOLD-glow] Dial back the GoldNode emissive on `M_GoldGlow` — ONE lever, no rebuild (art-director, editor)
- assignee: art-director
- status: **done** (2026-07-27, integrated + committed at the TASK-333 commit — the (b) ember watch CLOSED by TASK-333: depleted ember visible in-arena, no C++ remedy needed) ← was: **ready-for-integration** (2026-07-27 — base emissive Constant 6.0 → 0.45 inside `M_GoldGlow`; blown-past-0.85 86.0 % → **6.6 %** audit-comparable [CrystalTower band]; `GlowIntensity` param + TASK-226 pulse + TASK-296c superset verified intact by graph readback; handoff `handoffs/TASK-332-artist.md` + before/after PNGs. ⚠️ TASK-333(b) watch: depleted ember now 0.45×0.05 = 0.0225 effective — if invisible in-arena, sanctioned remedy = C++ `GlowIntensityDepleted` raise, NOT another material edit)
- blocked-by: none
- parallel-safe: **yes vs TASK-329** (different asset, different discipline) — **no vs any other EDITOR task** (serialize with TASK-310 / 323 / 325 / 326 / 330 / 331)
- **JONATHAN'S GATE IS SATISFIED.** The TASK-309 audit said this needed "Jonathan's eye on the intended look before anyone changes it".
  His ruling *"dial back the goldnode glow"* IS that eye. Approved — but the DIRECTION is approved, not a specific number; the number is the
  art-director's call inside the acceptance band below.
- spec: >
    **THE DEFECT (measured, TASK-309):** `M_GoldGlow` drives the emissive so hard that **86.3 % of `SM_GoldNode` renders past luma 0.85** —
    a featureless cream blob that destroys the grey-rock / gold-crystal / orange-crack separation the concept has. Reference point that
    WORKS: CrystalTower's comparable glow blows only **6.5 %** and still reads as a crystal.
    **⚠️ THE OBVIOUS LEVER IS THE WRONG ONE — READ THIS BEFORE TOUCHING THE MATERIAL.** `GlowIntensity` is a **runtime-driven** scalar:
    `AGoldNode::UpdateGlowGauge()` creates a lazy MID on slot 0 and writes
    `GlowIntensity = Lerp(GlowIntensityDepleted 0.05, GlowIntensityFull 1.0, Reserve/InitialReserve)` every gauge tick (the depleting-mine
    reserve gauge, TASK-253/257). **Lowering the material's DEFAULT `GlowIntensity` is therefore a NO-OP in play — the MID overwrites it
    with 1.0 at full reserve.** **THE CORRECT LEVER: reduce the BASE EMISSIVE STRENGTH authored INSIDE `M_GoldGlow` — the value that the
    `GlowIntensity` parameter multiplies.** Zero C++ change, the gauge keeps its exact semantics (`GlowIntensityFull = 1.0` still means
    "the authored look", which is simply dimmer), and the depleted-ember floor scales down proportionally.
    **MUST NOT REGRESS (the regression set):** (a) `GlowIntensity` stays a **live scalar parameter with that exact name** — renaming or
    deleting it silently kills the reserve gauge with no error (`AGoldNode::GlowIntensityParamName`); (b) the **TASK-226 0.1 Hz ±12 % sine
    emissive pulse** stays; (c) the **TASK-296c "keep-both" superset** (the `GlowIntensity` Multiply AND the sine pulse merged together)
    stays intact; (d) **STOCK NODES AND PARAMETERS ONLY — the Custom-HLSL BAN applies** (CONVENTIONS "Material & Niagara lane laws":
    a Custom HLSL node detonates shader permutations and wedges the editor under this Substrate + HW-RT stack); (e) gold nodes still glow
    **regardless of team** (the Team-contract exception) — intensity MODULATION, never material replacement.
    **DO NOT TOUCH THE TEXTURES.** `T_GoldNode_{D,N,ORM}` are **ORPHANED** — `SM_GoldNode` slot 0 carries `M_GoldGlow` directly and never
    samples them. A texture re-bake changes NOTHING on screen and burns a session (CONVENTIONS "GoldNode / CrystalTower glow materials").
    **ACCEPTANCE (measure it the same way the audit did, so the numbers are comparable):** `CaptureAssetImage` on `SM_GoldNode` with its
    real material → blown-past-luma-0.85 fraction **≤ 15 %** (target the CrystalTower band, ~5–10 %), **AND** the rock / gold / orange-crack
    separation is legible again, **AND** the node still reads as unmistakably warm-yellow **glowing** at gameplay camera distance — the
    failure mode on the other side is a dull rock nobody notices. Capture before/after and quote both blown-% numbers.
    Save the material; **do NOT commit** (build-master owns Git — TASK-333). **No C++, no Blueprint, no `L_Arena` save, no Git.**
    Post before/after + both numbers in 🎨 Art; hand off to TASK-333.
- names: >
    `/Game/Materials/M_GoldGlow` (base emissive strength ↓; scalar param **`GlowIntensity` PRESERVED by name**; TASK-226 sine pulse +
    TASK-296c superset preserved). Consumer: `/Game/Meshes/SM_GoldNode` slot 0. Runtime driver (read-only):
    `Source/GitClaudeUnrealTest/Siegebound/GoldNode.{h,cpp}` — `UpdateGlowGauge`, `GlowIntensityParamName`, `GlowIntensityFull 1.0`,
    `GlowIntensityDepleted 0.05`. **Orphaned, DO NOT TOUCH:** `/Game/Textures/T_GoldNode_*`. Report `handoffs/TASK-332-artist.md`.
    Law: CONVENTIONS **"GoldNode / CrystalTower glow materials — as-shipped truth + the emissive brightness lever"** (NEW 2026-07-27),
    "Material & Niagara lane laws" (Custom-HLSL BAN), "Team contract" (gold-node exception), "Mirrored depleting mines" (the gauge).

#### TASK-333 — [GOLD-glow-int] Verify the dialled-back glow at BOTH gauge extremes in `L_Arena` + commit (build-master)
- assignee: build-master
- status: **done** (2026-07-27, build-master — commit `c1d4add`; this commit = `M_GoldGlow` + TASK-332/333 docs only). Two-point verify in Simulate on `L_Arena`, two sessions: **(a)** full-reserve blown-fraction independently re-measured **9.1 %** mean-RGB (method reproduces the 86.3 % audit anchor at 85.3 %; TASK-332's own 6.6 %; gate ≤15 %, CrystalTower 6.5 %) — in-arena warm-yellow glowing, rock/gold/crack separation legible, no cream blob; **(b)** depleted ember via the REAL `Deplete()` path: MID 0.0500 exactly, VISIBLE (mine-region luma 58.3 vs shadow-grass 16.7 — dimmer, not black) — the TASK-332 ember watch is CLOSED, no `GlowIntensityDepleted` C++ raise needed; **(c)** gauge continuity exact at 7 sampled reserve points both directions (MID readback == 0.05+0.95×R/300 to 4 dp at every point); **(d)** 6/6 mines identical `MID(parent=M_GoldGlow)`, both team sides glow identically (mirrored-pair captures), Message Log ensure/AccessedNone/Fatal/Error = 0. Captures + follow-up observations (miner walk-stall 713 uu short of a corner-rise mine, pre-freeze — gameplay-lane candidate) in `handoffs/TASK-333-buildmaster.md`. ← was: backlog
- blocked-by: TASK-332
- parallel-safe: no (exclusive editor + Git; EDITOR-GATED)
- spec: >
    On **main**, exclusive editor, **Simulate STOPPED for any save**, verify in **Simulate** on `L_Arena` (never PIE-in-viewport —
    lane-knowledge 1). The point of this task is that the reserve gauge makes this a TWO-POINT check, not one:
    **(a) FULL RESERVE (MID writes `GlowIntensity` 1.0 = the authored look):** the mine reads warm-yellow and clearly glowing under the real
    arena sun, but the rock / gold / orange-crack separation is now legible — no cream blob. Quote the blown-past-luma-0.85 fraction and
    compare it to the 86.3 % before / the ≤15 % target / CrystalTower's 6.5 % reference.
    **(b) DEPLETED (MID writes 0.05):** the depleted ember is **still visible as an ember** — dimmer, not black. A dial-back that makes a
    depleted mine invisible has broken the zero-UI reserve signal and is a FAIL; report it rather than shipping it.
    **(c) The gauge still FUNCTIONS:** drive a mine's reserve down (miners or a direct reserve set) and confirm the glow dims
    **continuously** with it — proof the `GlowIntensity` param is still wired and the MID still finds it by name.
    **(d)** All six mines behave identically and gold nodes still glow **regardless of team**; Message Log clean (ensure / AccessedNone /
    Fatal = 0). Capture before/after for Jonathan.
    **COMMIT** on main with explicit pathspecs (`Content/Materials/M_GoldGlow.uasset` + board/CONVENTIONS/handoff), `git diff --stat` shows
    nothing foreign, **NO push**. `L_Arena` NEVER saved. **`git reset --hard` / `git clean -fd` BANNED.**
    If (b) or (c) fails, restore the last-known-good `M_GoldGlow`, do NOT commit, and route back to art-director with the observation.
    Post the readbacks + commit hash in 🔧 Build & Git.
- names: >
    `/Game/Materials/M_GoldGlow` · consumers `/Game/Meshes/SM_GoldNode` (6 mine instances in `L_Arena`, read-only) · gauge driver
    `AGoldNode::UpdateGlowGauge` (read-only). Commit on main, no push. Law: CONVENTIONS "GoldNode / CrystalTower glow materials",
    "Mirrored depleting mines" (gauge law), the hard gate (integration check before commit).

#### TASK-337 — [CASTLE-crumble-tune] Re-spread the crumble stage params against the bright base — MI scalar VALUES only (art-director, editor)
- assignee: art-director
- status: **done** (2026-07-27 — MEASUREMENT-FIRST re-run executed after TASK-339 in the same art session: shipped TASK-157 values FAIL the band on real render (S1 0.906×P / S2 0.643×P — too BRIGHT; the withdrawn "ScorchAmount dominates" direction was backwards) → conditional retune RAN → final values **MI01 `Darken 0.38 / Scorch 0.18`, MI02 `0.24 / 0.52`, MI03 untouched** → band PASS all five criteria (S1 0.664 / S2 0.439 / S3 0.336 / gaps 0.225/0.103); full table + protocol in `handoffs/TASK-337-artist.md` §RE-RUN. Integrated + committed at the TASK-338 commit) ← was: re-scoped 2026-07-27 — MEASUREMENT-FIRST re-run (first run returned BLOCKED with the SAMPLER-TYPE finding, `handoffs/TASK-337-artist.md` — nothing saved, MI01 probe values restored bit-exact, no Git; that return was CORRECT per the flag-don't-fix doctrine).
- blocked-by: TASK-339 (the master must COMPILE before any stage measurement means anything)
- parallel-safe: no (EDITOR-GATED — the one-editor law; no Git)
- spec: >
    **⚠️ RE-ADJUDICATION (2026-07-27) — READ FIRST; supersedes the "THE DEFECT" framing and the numeric direction below.** The first
    run proved the 28.0/28.9/29.2 table measured the engine Default Material (master compile failure — TASK-339 fixes it).
    **STEP 0 (NEW, GATING): with TASK-339 landed, re-measure P + S1/S2/S3 against the FIRST REAL RENDER of the TASK-157 SHIPPED values**
    (MIs untouched) using the protocol below. **IF the band PASSES as authored: STOP — NO retune.** Record the table + captures, close
    this task as measurement-only, hand straight to TASK-338 (which still commits the TASK-339 master fix). **ONLY if the band FAILS
    with real rendering does the re-spread below execute** — and IGNORE the old "ScorchAmount dominates / Darken up, Scorch down"
    direction (it was derived from the broken render; the cross-test headroom 0.957×P suggests the authored values may land near
    TASK-157's approved intent). Everything else below — the fence, the param-name law, the band, the protocol, no-Git — stands.
    **THE DEFECT (measured, TASK-331 — HISTORICAL, see Step 0):** pristine wall luma 77.5 → stages 28.0 / 28.9 / 29.2. The TASK-157 stage spread
    (01 `Darken 0.80 / ScorchAmount 0.12 / RoughBoost 0.30`, 02 `0.50 / 0.45 / 0.60`, 03 `0.30 / 0.80 / 0.85`) was authored against the
    old 0.0078 albedo; against the ~21×-brighter rebuilt base all three stages collapse into one dark band and stage 1 no longer reads
    "battle-worn but standing".
    **THE LEVER: scalar parameter OVERRIDE VALUES on `/Game/Materials/MI_Castle_Crumble01` and `…02` ONLY.** Do NOT touch the
    `M_CastleCrumble` master graph, `MI_Castle_Crumble03` (stage 3 PASSES as shipped — leave it unless the gap rule forces a nudge, and if
    touched it stays ≤ 0.40×P), any `T_Castle_*`, any `SM_*`, `ACastle` C++, or `L_Arena`. **NO re-derivation** — TASK-331's meshes, slots
    and UVs are correct and CLOSED.
    **Direction (the numbers are YOUR call inside the band — the TASK-332 pattern):** the measurement proves `ScorchAmount` dominates
    (stage 1's Darken 0.80 alone predicts ~62 luma; scorch 0.12 dragged it to 28.0). Expect stage 1 to need Darken UP toward ~0.85–0.90
    AND ScorchAmount DOWN toward ~0.03–0.06; stage 2 mid, keeping its "blackened charcoal-grey, clearly damaged" read.
    `RoughBoost`/`CharColor`/`EmberColor`/`EmberAmount` are NON-GATING — keep shipped values unless you have a reason.
    **PARAM NAMES ARE LAW:** `Darken`, `ScorchAmount`, `CharColor`, `RoughBoost` (+ dormant `EmberColor`/`EmberAmount` at 0) — override
    values only; never rename or delete a parameter.
    **ACCEPTANCE (measure, don't eyeball — the TASK-331 protocol):** in **Simulate** on `L_Arena` (never PIE-in-viewport), drive ONE castle
    75 → 50 → 25 % via world damage; same lit wall region, identical camera pose, exposure-consistent scene (grass control ~11.4–11.6);
    measure **P (pristine) + S1/S2/S3 in the SAME session**. Gate = the STAGE-LEGIBILITY BAND (ruling block above): S1 ∈ 0.55–0.80×P ·
    S2 ∈ 0.35–0.55×P · S3 ≤ 0.40×P · S1−S2 ≥ 0.10×P · S2−S3 ≥ 0.05×P — AND stage 1 visually reads TASK-157's intent
    *"dimmed, dusty, scorch-tinged — battle-worn but standing"* at gameplay distance, AND stage 3 still reads near-dead charred.
    **Simulate STOPPED before saving** (lane-knowledge 8). Save the touched MIs; **do NOT commit** (build-master owns Git — TASK-338).
    `L_Arena` NEVER saved. Capture pristine + all three stages for the handoff. Write `handoffs/TASK-337-artist.md` (params before/after +
    the luma table). Post before/after + the numbers in 🎨 Art.
- names: >
    `/Game/Materials/MI_Castle_Crumble01|02` (scalar override VALUES only; `…03` untouched by default). READ-ONLY: master
    `/Game/Materials/M_CastleCrumble`, `/Game/Meshes/SM_Castle_Crumble01|02|03`, `/Game/Textures/T_Castle_*`,
    `ACastle::ApplyCrumbleStage`/`ResetCastle`. Report `handoffs/TASK-337-artist.md`. Law: CONVENTIONS "Castle remaster" →
    **CRUMBLE STAGE-LEGIBILITY law** (NEW 2026-07-27).

#### TASK-338 — [CASTLE-crumble-tune-int] Independently verify the stage band in Simulate + commit the TASK-339 master fix (+ retuned MIs if any) (build-master)
- assignee: build-master
- status: **done** (2026-07-27, build-master — commit = this task's own, immediately after TASK-333's `c1d4add`; hash in `handoffs/TASK-338-buildmaster.md` Slack post). **SAMPLER-TYPE sweep PASS:** `Failed to compile Material` = 0 hits post-TASK-339-fix (last hit 22.33.20 pre-fix) across three Simulate loads + a forced `MaterialTools.recompile` (raises-on-failure, succeeded clean); the distinct stage renders are the terminal live proof. **Independent band re-measure PASS (all five criteria), reproducing TASK-337 to ±0.001:** P 171.22 · S1 **0.665**×P · S2 **0.439**×P · S3 **0.335**×P · gaps **0.226/0.104** (grass control 150.95–151.16, spread 0.21; frozen protocol pose; stages fired once in order with correct mesh + MI on BOTH slots; bot stopped pre-summon, 0 units all session). ResetCastle restores pristine + Blue accent (reset wall 171.50 ≈ P), RED castle unaffected, Message Log ensure/AccessedNone/Fatal/Error = 0. Commit: `M_CastleCrumble.uasset` + `MI_Castle_Crumble01|02.uasset` (MI03 untouched, excluded) + TASK-337/338/339 docs. **WATCH recorded: Jonathan's playtest eye is the final authority on the damage read.** ← was: backlog
- blocked-by: TASK-337 (full chain TASK-339 → 337 → 338 — do NOT run before BOTH land; the TASK-339 master fix is part of THIS task's commit)
- parallel-safe: no (EXCLUSIVE editor + Git; **MAY share TASK-333's editor session** — own deliverable, own commit, per the TASK-330/331 precedent)
- spec: >
    On **main**, exclusive editor. Independently re-run the TASK-331 measurement (do not take TASK-337's numbers on trust): in **Simulate**
    on `L_Arena`, drive one castle 75 → 50 → 25 %, confirm each stage fires ONCE in order, measure **P/S1/S2/S3** same-region /
    same-pose / exposure-consistent, and gate on the **STAGE-LEGIBILITY BAND** (ruling block above). Confirm `ResetCastle` restores the
    pristine rebuilt castle + per-team accent, the OTHER castle is unaffected, and the Message Log is clean (ensure / AccessedNone /
    Fatal = 0) **PLUS the SAMPLER-TYPE TRAP sweep (law, added at re-adjudication): `Failed to compile Material` = 0 hits on a fresh
    load — `M_CastleCrumble` AND all three MIs COMPILE (no Stats-panel error, no "recompiles every editor launch" warning). The
    Default-Material fallback is a `LogMaterial: Warning`, NOT an Error — the standard sweep MISSES it; grep for it explicitly, and any
    Default-Material fallback anywhere is an automatic FAIL.** Any save happens with Simulate STOPPED; `L_Arena` NEVER saved.
    **WATCH (recorded, non-blocking): Jonathan's playtest eye is the FINAL authority on the damage read** — his next playtest verdict
    closes this chain or reopens it as a new task; say so in the handoff.
    **COMMIT** on main with explicit pathspecs (**`Content/Materials/M_CastleCrumble.uasset` ALWAYS — the TASK-339 sampler fix rides
    in THIS commit** — plus `MI_Castle_Crumble01|02.uasset` ONLY if TASK-337's conditional retune ran, plus `…03` only if touched,
    + board/handoffs), `git diff --stat` shows nothing foreign, **NO push**. **`git reset --hard` / `git clean -fd`
    BANNED** (standing lesson 2). If the band FAILS: do NOT commit, append the measured table to `handoffs/TASK-337-artist.md`, route back
    to art-director (counts as a loop; max 3 then escalate to Jonathan via 🚨 Blockers).
    Post the luma table + before/after shots + commit hash in 🔧 Build & Git.
- names: >
    Commit set: `Content/Materials/M_CastleCrumble.uasset` (ALWAYS — the TASK-339 fix) + `MI_Castle_Crumble01|02.uasset` only if the
    retune ran (+ `…03` only if touched) + docs. READ-ONLY: `SM_Castle_Crumble01|02|03`, `T_Castle_ORM` (stays `TC_Masks`),
    `ACastle::ApplyCrumbleStage`/`ResetCastle`/`ApplyTeamVisuals`, `L_Arena`. Commit on main, no push. Law: CONVENTIONS "Castle
    remaster" → CRUMBLE STAGE-LEGIBILITY law + "Fleet Meshy remaster" → SAMPLER-TYPE TRAP; the hard gate (integration check before commit).

#### TASK-339 — [CASTLE-crumble-fix] ONE-ENUM master repair: `M_CastleCrumble` ORM sampler `LinearColor → Masks` + master resave (art-director, editor)
- assignee: art-director
- status: **done** (2026-07-27 — the one-enum fix executed: ORM sampler `SAMPLERTYPE_LinearColor → SAMPLERTYPE_Masks` on `MaterialExpressionTextureSampleParameter2D_2`, nothing else; recompile clean in 46 ms (no wedge), `Failed to compile Material` = 0 post-fix, all three MIs render distinct again, master resaved with Simulate stopped (clears the recompiles-every-launch debt); evidence `handoffs/TASK-339-artist.md`. Integrated + committed at the TASK-338 commit) ← was: backlog — dispatchable NOW
- blocked-by: none (RUNS FIRST in the re-adjudicated chain 339 → 337 → 338)
- parallel-safe: no (EDITOR-GATED; no Git)
- spec: >
    **THE DEFECT (proven, TASK-337 first run — `handoffs/TASK-337-artist.md` + `TASK-337-M_CastleCrumble-compile-error.png`):**
    `M_CastleCrumble`'s ORM node (`TextureSampleParameter2D_2`, param `ORM`) samples as `SAMPLERTYPE_LinearColor` with `T_Castle_ORM`
    baked in as the NODE default. TASK-330's same-path reimport correctly made that texture `TC_Masks` (the fleet import law — the
    texture is RIGHT and stays), and UE validates node-default textures at translation ⇒ hard SM6 error
    (`Sampler Type is Linear Color, should be Masks`) ⇒ the master AND all three `MI_Castle_Crumble0N` render the **engine Default
    Material in game** — live on main since `fcb1ec0`. (`M_AssetPBR` survives the same texture because its node default is the
    `TC_Default` NeutralORM and MI-level texture overrides are never re-validated — CONVENTIONS SAMPLER-TYPE TRAP.)
    **THE FIX — exactly ONE enum:** on that ORM sampler node, `SamplerType: Linear Color → Masks`. NOTHING else: zero graph-topology
    change, zero param change (`Darken`/`ScorchAmount`/`CharColor`/`RoughBoost`/`EmberColor`/`EmberAmount` names AND values untouched),
    `.rgb` → O/R/M channel semantics preserved (Masks compression, linear channels — the combination `M_AssetPBR` effectively runs
    fleet-wide). **The graph is STOCK NODES (TASK-157) and STAYS stock — the Custom-HLSL BAN is not in play, but the WEDGE-WATCH is:**
    this triggers a shader recompile under the Substrate + HW-RT stack; a single-material stock recompile is bounded (the first run's
    `RecompileShaders` round-tripped in 46 ms), but if the recompile cascades abnormally or the editor wedges, STOP and post 🚨 Blockers —
    do not iterate.
    **ACCEPTANCE:** (a) Stats panel CLEAN — the SM6 sampler error gone; (b) `RecompileShaders Material M_CastleCrumble` SUCCEEDS;
    (c) zero `Failed to compile Material` / `Failed to compile Material Instance with Base M_CastleCrumble` lines on a fresh compile/load;
    (d) in Simulate on `L_Arena`, ONE driven stage renders VISIBLY non-default (scorch-tinted castle, NOT the grey sparkle-mottle —
    capture it); (e) **RESAVE the master** (Simulate STOPPED — this also clears the standing "recompiles every editor launch" warning;
    say so in the handoff). **Save `M_CastleCrumble` ONLY** — no MI save (TASK-337 owns MI values; MI01 was restored bit-exact), no
    `T_Castle_*` change (the texture is correct — never "fix" this by flipping it off `TC_Masks`), `L_Arena` NEVER saved, **no Git**
    (TASK-338 commits the master). Write `handoffs/TASK-339-artist.md` (before/after Stats + the stage capture). Post in 🎨 Art.
- names: >
    `/Game/Materials/M_CastleCrumble` (ORM node `SamplerType` enum ONLY; then resave). READ-ONLY: `/Game/Textures/T_Castle_ORM`
    (stays `TC_Masks`), `MI_Castle_Crumble01|02|03`, `SM_Castle_Crumble01|02|03`, `M_AssetPBR` (reference pattern only). Report
    `handoffs/TASK-339-artist.md`. Law: CONVENTIONS "Fleet Meshy remaster" → **SAMPLER-TYPE TRAP** (NEW 2026-07-27) + "Material &
    Niagara lane laws" (stock nodes); precedent `handoffs/TASK-157-artist.md` (the authored graph).

---

