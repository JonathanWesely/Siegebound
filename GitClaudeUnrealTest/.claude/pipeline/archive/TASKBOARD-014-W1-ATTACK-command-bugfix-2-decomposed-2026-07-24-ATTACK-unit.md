<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## W1 ATTACK-command bugfix 2 (decomposed 2026-07-24) — ATTACK units halt JUST SHORT of the enemy castle (TASK-282..283)

**Jonathan's report (post-W1 sign-off, deferred residual — 2026-07-24):** after the TASK-280/281 mid-field march-freeze fix, under the ATTACK command *"the units now move closer to the castle but they stop just short of it."* The mid-field FREEZE is gone; the NEW residual is a FINAL-APPROACH HALT — ATTACK-commanded Standard Blue units reach the vicinity of Castle_Red but stop just short instead of attacking it. **Jonathan APPROVED W1 despite this (TASK-219) and said it "can resolve later" — so this is a DEFERRED, scheduled-for-overnight follow-up, NOT a W1 blocker.**

**Same lane as the rest of the W1 work:** develops on `m7.6-arena10x`, folds into the W1 build. Owns `SummonedUnit.{h,cpp}` (already branch-frozen; the same file TASK-280/281 fixed). No new asset/class identifiers expected up front → NO CONVENTIONS naming additions issued in advance; if the fix introduces a durable new tunable UPROPERTY, the manager folds a one-line entry into CONVENTIONS "Unit commands (Shield Wall stances)…" at integration.

**DIAGNOSE-FIRST MANDATE (binding — do NOT fix on faith; the TASK-280 precedent + the health-bar saga):** TASK-282 MUST reproduce, instrument, and CONFIRM the actual runtime mechanism — WHY the unit stops at distance X instead of entering `EnterAttack` on the castle — BEFORE changing any logic. A fix committed without runtime evidence of the root cause is a QA FAIL by construction.

**ATTACK-SPECIFIC (load-bearing clue):** the LEGACY (base-game) Standard body DOES destroy the enemy castle — legacy units march to `FindNearestEnemyCastle()` and enter `EnterAttack` on arrival. DEFEND/HOLD and the legacy fall-through are confirmed-good; only the ATTACK stance halts short. The programmer should DIRECTLY COMPARE the ATTACK final-approach to the LEGACY final-approach and find the diverging line in the last stretch.

**Orchestrator hypotheses (static, UNCONFIRMED — hand to the programmer, demand runtime proof):**
- (a) **Standoff/acceptance distance > `AttackRange`.** `EnterAdvance` toward the enemy castle stops at an acceptance/standoff radius (the `MoveToActor` AcceptanceRadius / `StructureMoveAcceptanceRadius`) that EXCEEDS the castle `AttackRange`, so the unit arrives, stops at the standoff point, and the range check for `EnterAttack` on the castle never passes. Compare the ATTACK acceptance radius to the legacy one and to `AttackRange`.
- (b) **The enemy CASTLE is never picked up as `CurrentTarget`.** `AcquireTarget` buckets non-pawns as "BestOther" — verify the enemy castle is actually acquired within `AggroRadius` from the stop position, and that `AggroRadius` REACHES the castle from wherever the unit halts. The TASK-280 `EnemyBaseEngageRadius=3500` gate makes the unit prefer the STABLE castle-as-goal until within 3500 of it — confirm the handoff from "march to castle actor" to "acquire + `EnterAttack` the castle" actually fires (the unit may sit in the seam: past the march-goal acceptance radius but the castle not yet an acquired target).
- (c) **The new `EnemyBaseEngageRadius=3500` box-defender gate interacting with the final approach.** Within 3500 the no-aggro goal switches to a box defender; if there is no live box defender (empty Red box) or the box-defender lookup returns null/jitters right at the boundary, the goal/target may thrash or resolve to a point the unit is already at → it stops without attacking. Run BOTH conditions (WITH vs WITHOUT enemy in the Red box — the TASK-280 controlled-test discipline).

**Dispatch shape:** TASK-282 (gameplay-programmer: instrument → confirm → fix, file-only, no compile/Git) → QA (implied status-flow gate; QA reads the report WITH the attached runtime evidence — a root-cause claim not backed by captured logs is a FAIL) → TASK-283 (build-master: compile + PIE ATTACK reach-AND-attack verify [with/without enemy in the Red box] + branch commit, NO push; TASK-283 IS the overnight `SummonedUnit`-touching integration and folds in any concurrent SummonedUnit work). QA is the implied gate between them (not a numbered task).

#### TASK-282 — Diagnose (instrument first) + fix the ATTACK final-approach halt (C++: `ASummonedUnit`; likely `EnterAttack` / `AcquireTarget` / `EnterAdvance` acceptance)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-283 batch, build-master 2026-07-24, commit on m7.6-arena10x — compile GREEN, clean PIE load; CONVENTIONS box-first/`EnemyBaseEngageRadius` wording folded out. ⚠ DESIGN CHANGE (retires TASK-275 box-defenders-FIRST) still flagged for Jonathan's morning call. --- Prior QA 2026-07-24 PASS, 0 blockers, `qa/TASK-282.md`. Cause CONFIRMED (Simulate-In-Editor: a full-field marcher reached+destroyed Castle_Red, isolating the halt to the box-substitution): within EnemyBaseEngageRadius(3500) `FindNearestEnemyInSpawnBox` flipped every 0.25s tick → EnterAdvance re-path thrash at the ring + castle never became the goal → EnterAttack never fired. FIX: removed the box-defender substitution → ATTACK no-aggro goal = STABLE enemy castle (byte-identical to legacy castle-kill); removed the now-dead `EnemyBaseEngageRadius` UPROPERTY + `FindNearestEnemyInSpawnBox` helper. Clean-removal verified (0 Source/ + 0 Content/ referrers; no BP serializes the removed UPROPERTY). Non-regression: legacy/DEFEND/HOLD/EnterAdvance/EnterAttack/AcquireTarget/bot untouched; TASK-280 anti-freeze subsumed. ⚠ DESIGN CHANGE flagged for Jonathan's morning call: retires TASK-275 "box-defenders FIRST" (incompatible w/ a continuously-respawning bot box; defenders still fought via aggro-600). Awaits batch build TASK-283 w/ TASK-267. Integration TODO: fold `EnemyBaseEngageRadius`/box-first out of CONVENTIONS "Unit commands".)
- blocked-by: none (TASK-280/281 done @ `0295f75`; sole owner of `SummonedUnit.{h,cpp}` this pass — the file is branch-frozen for W1, no other OPEN task touches it. ⚠ FILE-OVERLAP NOTE: the PLANNED Phase-2 URO task TASK-285 also edits `SummonedUnit.{h,cpp}`; it is `planned — DO NOT DISPATCH`, so no concurrency now — if Jonathan green-lights Phase 2 while this is in flight, they serialize on the file, TASK-282 first.)
- parallel-safe: yes (disjoint from L_Arena.umap / DA_BattlefieldScatter / SiegeBotController / SiegePlayerController / CaptureZone / DeckBuilderWidget; the planned Phase-1 TASK-284 owns BattlefieldScatter/ScatterConfig, not this file)
- spec: >
    On `m7.6-arena10x`. FILE-ONLY — NO compile, NO Git (TASK-283 owns those). Make ATTACK-commanded Standard PLAYER (Blue)
    units march the FULL field AND actually ATTACK the enemy castle (Castle_Red) on arrival — not halt just short. DEFEND/HOLD
    and the legacy body are confirmed-good and MUST NOT be disturbed. **DIAGNOSE-FIRST — confirm the mechanism at runtime before
    any logic change.**
    (1) **REPRODUCE + INSTRUMENT FIRST.** Add TEMPORARY per-state-tick `UE_LOG` (dedicated/`Verbose` category — stripped or
    demoted before ready-for-qa) on an ATTACK-commanded Standard Blue unit near the enemy base, printing: unit name + world
    position (esp. X — castles at ±25,000), `State`, `CurrentTarget` (name/null), the computed `Goal` (name), the 2D distance
    to Castle_Red, the ACCEPTANCE/standoff radius actually in force (`MoveToActor` AcceptanceRadius / `StructureMoveAcceptanceRadius`),
    the castle `AttackRange`, and `GetMoveStatus()`. Run PIE, drive the ATTACK stance, let a unit reach the enemy base, read
    `Saved/Logs/`, and CONFIRM from the log WHY it stops (arrived-at-standoff-but-out-of-`AttackRange`? castle never becomes
    `CurrentTarget`? goal/target thrash at the 3500 boundary?) BEFORE touching logic.
    (2) **THE CONTROLLED TEST — run BOTH and record both:** issue ATTACK with the Red spawn box (a) POPULATED with bot
    units/buildings and (b) EMPTY of any enemy in/near the box. This isolates whether the halt is the box-defender gate
    (hypothesis c) or the plain castle final-approach (hypotheses a/b). DIRECTLY COMPARE the ATTACK final-approach to the
    LEGACY Standard body (`SummonedUnit.cpp:1064-1102`), which DOES reach + destroy the castle — find the diverging line.
    (3) **FIX per the confirmed cause, preserving intent, without regressing DEFEND/HOLD/legacy.** Likely candidates (pick per
    the evidence, combine as needed): bring the ATTACK march acceptance radius in line with `AttackRange` so `EnterAttack`
    triggers on arrival; and/or ensure the castle is acquired as `CurrentTarget` within range at the stop point; and/or fix the
    3500-boundary goal/target thrash. DO NOT touch the DEFEND/HOLD branches or the legacy body (must stay byte-identical when
    the command gate is false). If the fix lands in shared `EnterAdvance`/`EnterAttack`/`AcquireTarget`, scope it so legacy
    marching + attacking is unchanged (any change to how ALL units re-path/attack is a WATCH — flag it, keep it a no-op for the
    legacy fall-through). **Minimize new identifiers.**
    (4) **BEFORE ready-for-qa:** strip the temporary diagnostic logs OR demote to a dedicated `Verbose` category (no per-tick
    spam at default verbosity).
    (5) **HANDOFF (`handoffs/TASK-282.md`) MUST contain:** the captured log evidence for BOTH test conditions, the CONFIRMED
    mechanism (with the proving log lines), the legacy-vs-ATTACK divergence you found, the chosen fix shape + why it follows
    from the evidence, and an explicit statement that DEFEND / HOLD / the legacy body / the bot are unchanged (and — if a
    shared helper was touched — proof the legacy path is a no-op delta). QA implied (shadow scan, complete-type include scan,
    null-safety, confirm legacy/DEFEND/HOLD/bot untouched); QA reads this report WITH the runtime evidence. NOT IN SCOPE: input
    assets, HUD, Castle logic beyond the fix's need, the bot, compiling, Git. Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (the ATTACK branch in `UpdateStateStandardCommanded`, plus
    `EnterAttack` / `EnterAdvance` / `AcquireTarget` / `FindNearestEnemyInSpawnBox` / `FindNearestEnemyCastle` /
    `EnemyBaseEngageRadius` / `StructureMoveAcceptanceRadius` as the diagnosis dictates). Minimize new identifiers; any durable
    new tunable follows the existing SummonedUnit UPROPERTY pattern (EditDefaultsOnly, ClampMin, doc comment citing this task +
    FLAGGED-tunable note) and is RECORDED in the handoff — the manager folds a one-line entry into CONVENTIONS "Unit commands
    (Shield Wall stances)…" at integration (do NOT edit CONVENTIONS in this task). Law: CONVENTIONS "Unit commands (Shield Wall
    stances) — ATTACK / HOLD / DEFEND (W1, 2026-07-23)".

#### TASK-283 — Integration: compile + PIE ATTACK reach-and-attack verify (with/without enemy) + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — BATCHED integration of TASK-282 + TASK-267 on m7.6-arena10x, commit `5fb8058` (no push). Overnight autonomous (Jonathan asleep): Step-0 PIE idle; SAVE-ALL (`save_assets([])`=true) THEN graceful close → editor exited cleanly, NO Save-Content dialog wedge, NO force-kill needed. Recompiled GREEN (~16s); relaunched on L_Arena. PIE: clean load (no errors/ensures/Accessed-None). TASK-267: bot reached + fired Rule 4 (Knight, gold 36→18) — rules 1/2a/2b/3 fell THROUGH, no ladder stall (Rule 2 Economy did not fire in the ~30 s window — attack-bank priority + ×3 economy costs, NOT a stall). TASK-282: ATTACK now = stable-castle march (byte-identical to legacy castle-kill); the live full-field march + EnterAttack on Castle_Red = Jonathan WATCH (QA equivalence proof). CONVENTIONS box-first/`EnemyBaseEngageRadius` wording folded out. Committed: `SummonedUnit.{h,cpp}`, `SiegeBotController.{h,cpp}`, `CONVENTIONS.md`, board + `handoffs/qa` for TASK-267+282. DeckBuilderWidget (TASK-268) stayed parked/unstaged. No push. ⚠ TASK-282 retires the TASK-275 box-first spec — flagged for Jonathan's morning design call. Was: backlog.)
- blocked-by: TASK-282 (qa-passed)
- parallel-safe: no (single editor + compiler + Git)
- spec: >
    On `m7.6-arena10x` — **THE overnight `SummonedUnit`-touching integration** (folds in TASK-282 and any other SummonedUnit
    work that lands the same window). (1) COMPILE TASK-282's C++ (editor bounce as usual — Jonathan's close/reopen grant covers
    the session). GREEN, report time + `Result: Succeeded`. Failure → append errors to `qa/TASK-282.md`, route back to
    gameplay-programmer (counts as a QA loop). (2) **VERIFY branch-owned files untouched:** `git diff --stat` should show ONLY
    `SummonedUnit.{h,cpp}` (+ board/CONVENTIONS if the manager folded a tunable line); if `L_Arena.umap` / `DA_BattlefieldScatter`
    / `SiegeBotController` / `SiegePlayerController` / `Castle.{h,cpp}` / `CaptureZone` / `DeckBuilderWidget` changed
    unexpectedly, STOP and report. (3) **PIE ATTACK reach-AND-ATTACK verify — THE fix criterion, run BOTH conditions:** put
    Blue Standard units under ATTACK and confirm they march the FULL field AND enter `EnterAttack` on Castle_Red (no halt short)
    — (a) WITH enemy units/buildings in/near the Red spawn box, and (b) WITHOUT any enemy in the Red spawn box. Best-effort +
    honest about what needs Jonathan's real T-key input on an unlocked desktop (command state is non-UPROPERTY + no headless input
    injection — prove by equivalence where hands are needed). Regression-check DEFEND/HOLD + Siege/Support/miners/bot. (4) COMMIT
    on the branch referencing TASK-282/283 + Jonathan's report. **DO NOT PUSH.** (5) Record the human WATCH (ATTACK units cross
    the full 10× field and ATTACK the enemy castle with real T-key input) for Jonathan. Leave the editor running + saved. Post
    results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` commit (NO push); `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (+ board/CONVENTIONS
    if a tunable line was folded in). Law: CLAUDE.md hard gates (PASS QA before commit, never push unasked), M7.6
    branch-ownership, CONVENTIONS "Unit commands (Shield Wall stances)…".

---

