<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-326 — [FACE-diag] Verify Archer + Ogre facing in REAL play + measure the fleet yaw table (gameplay-programmer, editor/PIE, DIAGNOSE-ONLY)
- assignee: gameplay-programmer
- status: **done** (2026-07-27) — diagnose-only, NOTHING edited (no C++, no BP, no asset, no Git, no `L_Arena` save); handoff `handoffs/TASK-326-programmer.md`
- blocked-by: none
- parallel-safe: no (exclusive editor/MCP; serialize with TASK-310/323/325)
- outcome: >
    **VERDICT: CONFIRMED REAL DEFECT — and it is THREE units, not the two the board suspected: Archer, Ogre AND Wizard.**
    Jonathan independently confirmed the sideways-walk in live play. **The Wizard was caught ONLY because the diagnosis swept all
    12 units instead of checking the two named in the finding** — the board premise ("0 on Archer and Ogre") was incomplete, and a
    two-unit-only investigation would have shipped a fix that left the newest unit broken. Sweep-the-population, don't spot-check the accused.
    **Measured truth:** all 12 `SK_<CardID>` bake forward on UE-local **+Y** (one skeleton `SK_Footman_Skeleton` 12/12; bounds put the
    lateral axis on X with Cavalry decisive at 2.16× on Y; `rig_character.py:337` authors front on Blender −Y; raw `SkeletalMeshActor`s
    from BOTH pipeline generations face a +Y camera). ⇒ the ONE correct `SkeletalVisualMesh` yaw is **−90** for every unit. Nine units
    measure exactly `(0,−90,0)`; Archer/Ogre/Wizard measure yaw **0 AND Z 0** — i.e. their component was **never authored at all** and
    sits at the C++ constructor default. **This is the YAW half of the TASK-306/307 bug, not a new one** (Z half closed, yaw half left open).
    Ruling 3's per-unit escape hatch is **NOT triggered** ⇒ **SYSTEMIC fix confirmed; TASK-327 proceeds** with the constant `-90.f`.
    The 2026-07-26 "0/180/270 all occur" observation is **DEBUNKED** — camera-side artifact; the data has two values and one baked forward.
    **Incidental finding (out of TASK-327's scope, ruled by the manager):** `BP_Unit_Wizard`'s STATIC `VisualMesh` is yaw 0 / Z 0 while all
    11 others are −90 / −CapsuleHalfHeight ⇒ **TASK-334** (BP-data restore) + **TASK-335** (parked C++ derivation debt).
    Evidence: `handoffs/TASK-326-bakedforward-from-plusY.png`, `handoffs/TASK-326-shipping-geometry-from-travel-axis.png`.
- spec: >
    **DIAGNOSE ONLY — no code edit, no asset edit, no Blueprint edit, no Git.** Answer three questions with evidence, then recommend.
    (1) **REAL-PLAY CHECK (the verdict that matters).** In a real PIE match on `L_Arena`, field **Archer** and **Ogre** on BOTH teams alongside a known-good control unit (Footman, yaw −90) in the same shot, and observe all three states: **MARCHING** (does the body face its direction of travel, or is it walking sideways/backwards?), **ATTACKING** (does it face its target when it fires/strikes?), and **DEATH** (does the death animation play in a sane orientation?). Report the OBSERVATION, not the conclusion; capture screenshots and state plainly for each unit: correct / sideways / backwards / other. Do the same for the **Wizard** (same code path, 12 units in the fleet).
    (2) **MEASURE the table.** For all 11 fleet units + Wizard, record: `SkeletalVisualMesh` `RelativeRotation.Yaw` at runtime, the actor's forward vs the mesh's apparent visual front, and the mesh's own baked forward axis as it comes off the rig pipeline. Establish whether the 11 rebuilt SKs share ONE baked forward (they came off one pipeline + one shared skeleton — if they do, the divergent component yaw is pure per-BP authoring drift).
    (3) **ROOT-CAUSE the divergence.** Confirm or refute the manager's read: C++ never sets this rotation (no `SetRelativeRotation` in `SummonedUnit.cpp`), so the value is per-BP authored on each `BP_Unit_<Unit>` component template. If confirmed, name the ONE value the fleet should carry.
    **Recommend: SYSTEMIC vs PER-UNIT**, with the reasoning and the exact constant if systemic. If real play shows Archer/Ogre look CORRECT as-is, say so — "their SK bakes a different forward that yaw 0 compensates" is a legitimate and expected possible answer, and it retires TASK-327/328 rather than shipping a cosmetic churn.
    **CARRY THIS CONTEXT (ruling 4):** capsule half-heights differ per unit (Ogre 145 / Cavalry 104 / Knight 95 / MilitiaMob 74.5 / Footman 90) — any check assuming 90 lies; grounding is already CLOSED at 2.15–2.40 cm fleet-wide and is NOT re-opened here.
    Write `handoffs/TASK-326-programmer.md` (per-unit table + the three answers + the recommendation). Post the verdict in ⚙️ Dev & QA; a confirmed visible bug also gets a one-liner in 🚨 Blockers.
- names: >
    Read-only: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (`ResolveSkeletalVisual` :266-360, constructor :129-156) · `SkeletalVisualMesh` component on every `/Game/Blueprints/Units/BP_Unit_<Unit>` (Footman, Archer, Knight, Miner, Cleric, Ogre, Sapper, Pikeman, Cavalry, MilitiaMob, Longbowman, Wizard) · `/Game/Characters/SK_<Unit>` · skeleton `SK_Footman_Skeleton`. Report `handoffs/TASK-326-programmer.md`. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" (SkeletalMeshComponent swap contract), "Fleet Meshy remaster".

#### TASK-327 — [FACE-fix] Systemic facing normalisation in `ResolveSkeletalVisual` (gameplay-programmer, C++ file-only) — CONDITIONAL on TASK-326
- assignee: gameplay-programmer
- status: **done** (2026-07-27) — qa-passed (`qa/TASK-327.md`, PASS, 0 blockers); code landed on main in Jonathan's own commit `7bedf58`; integrated + PIE-verified at TASK-328
- blocked-by: TASK-326 ✅ **done** (2026-07-27, defect CONFIRMED, fleet constant `-90`) **and** the manager's CONVENTIONS "Unit mesh facing" clause ✅ **RATIFIED** (2026-07-27) — **BOTH DEPENDENCIES CLEARED, unblocked**
- parallel-safe: yes (file-only; no editor, no compile, no Git)
- spec: >
    **Runs ONLY if TASK-326 confirms a real facing defect.** If 326 finds Archer/Ogre look correct in play, this task closes `wont-do` and the manager records the measured per-unit facing table in CONVENTIONS instead — **do NOT normalise a fleet that already looks right.**
    If confirmed: normalise the facing in C++ at the SAME single site the grounding fix already owns — `ASummonedUnit::ResolveSkeletalVisual`, immediately after the grounding block (`SummonedUnit.cpp` ~:317-325) — so no Blueprint can drift again:
      • add `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siegebound|Unit") float SkeletalVisualYawOffset` defaulted to the fleet constant TASK-326 measured (expected `-90.f`);
      • apply it as the component's relative yaw at swap time, preserving authored pitch/roll exactly as the grounding fix preserves authored X/Y;
      • keep the UPROPERTY as the documented **exception hatch**: a genuinely differently-baked mesh may override it on its own BP, and that is a NON-DEFAULT requiring an explicit manager ruling (same doctrine as a bespoke skeleton).
    **NO-REGRESSION IS THE LOAD-BEARING CLAIM** (mirror TASK-307's argument shape): the change must be a provable NO-OP for the 9 units already at −90, and must be null-safe on the static-fallback path (un-rigged units early-return before this block). Do not touch the grounding math, the lunge (`VisualMeshBaseRelativeLocation`), the static `VisualMesh` path, or the placement ghost.
    File-only: no compile, no editor, no Git, no MCP. Write `handoffs/TASK-327-programmer.md` with the before/after and the no-op table. Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (`ResolveSkeletalVisual`, new `SkeletalVisualYawOffset`). Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" (swap contract) + the new "Unit mesh facing" clause.

#### TASK-328 — [FACE-int] Compile the facing fix + verify all 12 units in PIE + commit (build-master)
- assignee: build-master
- status: **done** (2026-07-27) — compile green (14.79 s); real-PIE verified on L_Arena: 16/16 live units (both teams) read `SkeletalVisualMesh (0,-90,0)`, Archer/Ogre/Wizard march/attack/die correctly, 9 controls unchanged, ensure/AccessedNone/Fatal = 0; shots `handoffs/TASK-328-verify-*.png`, report `handoffs/TASK-328-buildmaster.md`; code was already on main in `7bedf58` (Jonathan self-commit) — docs/shots committed at `aad4b08`
- blocked-by: TASK-327-QA
- parallel-safe: no (exclusive editor + Git)
- spec: >
    Compile TASK-327 (code hard gate — a build failure appends to `qa/TASK-327.md` and routes back to gameplay-programmer, counting as a QA loop). Then verify in **real PIE** on `L_Arena`, not Simulate: (a) **Archer, Ogre and Wizard** now face their direction of travel while marching, face their target while attacking, and die in a sane orientation; (b) the **9 previously-correct units are visually UNCHANGED** — this is the regression gate, and it fails the task if any of them shifts; (c) Message Log clean (ensure / AccessedNone / Fatal = 0). Capture before/after shots for Jonathan. Commit on main with explicit pathspecs (`SummonedUnit.{h,cpp}` + board + CONVENTIONS + handoff/QA), `git diff --stat` shows nothing foreign, **NO push**.
    **Ruling 4 applies to any measurement taken here:** read the real capsule half-height per unit (Ogre 145 / Cavalry 104 / Knight 95 / MilitiaMob 74.5 …); never assume 90. Grounding is already closed at 2.15–2.40 cm — do not re-litigate it. **`git reset --hard` / `git clean -fd` are BANNED** (standing lesson 2).
    Post the result + commit hash in 🔧 Build & Git.
- names: > `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` · commit on main, no push. Law: the hard gate (PASS QA report before commit).

#### TASK-334 — [FACE-wizstatic] Bring `BP_Unit_Wizard`'s static `VisualMesh` in line with the fleet (build-master, BP data only)
- assignee: build-master
- status: **done** (2026-07-27) — static `VisualMesh` template set to `(0,-90,0)` / `Z = -88` (live `CollisionCylinder.CapsuleHalfHeight` readback = 88, agrees with TASK-326); `SkeletalVisualMesh` untouched at constructor default (C++-owned); live PIE Wizard unchanged — still resolves `SK_Wizard` (SK visible at C++-derived −90/−88.07, static hidden), ensure/AccessedNone/Fatal = 0; own commit on main (hash in 🔧 Build & Git post)
- blocked-by: TASK-328
- parallel-safe: no (exclusive editor + Git; **MAY run inside TASK-328's editor session** after 328's commit lands — but it is its own deliverable and its own commit, per the TASK-330/331 precedent)
- spec: >
    **BP DATA ONLY — one component, two values, on ONE Blueprint.** On `/Game/Blueprints/Units/BP_Unit_Wizard`, set the **static `VisualMesh`** component's relative transform to match every other unit: **`RelativeRotation = (Pitch 0, Yaw -90, Roll 0)`** and **`RelativeLocation.Z = -CapsuleHalfHeight`**. **Ruling 4 applies — READ the real half-height** off the Wizard's `CollisionCylinder` (`CapsuleHalfHeight`, measured **88** at TASK-326, so Z = **-88**); **never assume 90**, and if the readback disagrees with 88, the READBACK WINS. Leave X and Y as authored.
    **DO NOT TOUCH `SkeletalVisualMesh` on this or any BP.** Its yaw and Z are C++-owned as of TASK-327 (CONVENTIONS "Unit mesh facing"); re-authoring it per-BP is the exact trap that batch closed, and on the Wizard it would also mask the fix. Leave `BP_Unit_{Archer,Ogre,Wizard}`'s `SkeletalVisualMesh` at 0 — C++ overwrites it at swap time.
    **Verify** by readback (both values on the CDO component template) **and** visually: temporarily confirm the static fallback reads correctly — either in the BP viewport or by a scratch spawn — feet on the ground and facing +X. **Then confirm the live path is unaffected:** in PIE the Wizard still resolves `SK_Wizard`, so its on-screen appearance must be **byte-identical to post-TASK-328** — this task changes only the dormant fallback. Message Log clean.
    Commit on **main, NO push**, explicit pathspec (`Content/Blueprints/Units/BP_Unit_Wizard.uasset` + board), `git diff --stat` shows nothing foreign. **No C++, no asset re-import, no `L_Arena` save, no `git reset --hard` / `git clean -fd`** (standing lesson 2). Post before/after values + commit hash in 🔧 Build & Git.
- names: >
    `/Game/Blueprints/Units/BP_Unit_Wizard` → component `VisualMesh` (static) → `RelativeRotation (0,-90,0)`, `RelativeLocation.Z = -88` (verify vs live `CollisionCylinder.CapsuleHalfHeight`). Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" → **"Unit mesh facing"** → the STATIC `VisualMesh` clause.

