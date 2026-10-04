<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## W1 ATTACK-command bugfix (decomposed 2026-07-24) — Standard units freeze just past midfield under ATTACK (TASK-280..281)

**Playtest report (verbatim, Jonathan, 2026-07-24):** *"the attack button doesnt seem to work very well, when the units are under the attack command they will move up, but once they get a little bit past the midpoint, they freeze and stop moving forward, I'm not sure why. The defend and hold commands seem to work fine"*

**W1-SCOPED, NOT held. This is a W1-BLOCKER for the ATTACK stance only** (DEFEND/HOLD are confirmed-good by Jonathan). Same lane as the rest of the W1 work: develops on `m7.6-arena10x`, folds into the W1 build. The Shield Wall command feature shipped at `70487d5` (TASK-273..277) with this latent ATTACK bug; DEFEND/HOLD are unaffected. No new asset/class identifiers expected → NO CONVENTIONS naming additions up front; if the fix introduces a durable new tunable UPROPERTY, the manager folds a one-line entry into CONVENTIONS "Unit commands (Shield Wall stances)…" at integration (see naming note in TASK-280).

**DIAGNOSE-FIRST MANDATE (binding — do NOT fix on faith):** the recent health-bar saga proved confident static hypotheses mislead. TASK-280 MUST reproduce, instrument, and CONFIRM the actual runtime mechanism BEFORE changing any logic. A fix committed without runtime evidence of the root cause is a QA FAIL by construction.

**Orchestrator code findings (static read — handed to the programmer, NOT yet confirmed):**
- The ATTACK branch is `ASummonedUnit::UpdateStateStandardCommanded`, the `case ESiegeUnitCommand::Attack` body at `SummonedUnit.cpp:1279-1322`. It is FUNCTIONALLY IDENTICAL to the legacy Standard body (`SummonedUnit.cpp:1064-1102`) EXCEPT ONE thing — the no-aggro march goal:
  - legacy (works — base game / DEFEND / HOLD fall-through): `Goal = FindNearestEnemyCastle()` — a STABLE actor (~X +25,000).
  - ATTACK (`SummonedUnit.cpp:1297-1303`): `Goal = FindNearestEnemyInSpawnBox(EnemyCastle) ? that : EnemyCastleActor` — `FindNearestEnemyInSpawnBox` (`SummonedUnit.cpp:1401-1451`) returns the nearest ENEMY unit/building inside the ENEMY castle's spawn box, RE-EVALUATED EVERY TICK.
- `FindNearestEnemyInSpawnBox` + `ACastle::IsPointInSpawnBox` (`Castle.cpp:364-371`, box centered on the enemy castle, half-extent (840,840)) are null-safe, correctly team-filtered, and exclude the castle itself on a static read.
- The re-path gate is `EnterAdvance` (`SummonedUnit.cpp:1790`): `const bool bGoalChanged = (CurrentMoveGoal != Goal); if (bGoalChanged || MoveStatus==Idle) { MoveToActor(...); }` — so a goal-ACTOR pointer that changes every tick re-issues `MoveToActor` every tick → path restart → stutter → effective freeze. `FindNearestEnemyInSpawnBox` is anchored on the FAR enemy-castle box, NOT the marching unit — so it can return a jittering bot pawn even while the player unit is at MIDFIELD.

**Leading hypothesis (MUST be confirmed at runtime, NOT fixed on faith):** Goal jitter. Behavior splits cleanly by goal STABILITY — DEFEND (fixed own castle) works, HOLD (fixed clicked point) works, ATTACK (an every-tick-recomputed "nearest enemy in the enemy box", plus `AcquireTarget` flicker where the two armies meet just past midfield) freezes. After the TASK-265 bot-spawn fix the bot keeps units INSIDE its own spawn box, so `FindNearestEnemyInSpawnBox` frequently returns a bot pawn, and "nearest" flips tick-to-tick as bot units mill → `Goal` changes almost every tick → `EnterAdvance` re-issues the path every tick → stutter/freeze. Legacy never shows this because its fallback is the STABLE castle. HYPOTHESIS ONLY — demand runtime proof.

**Dispatch shape:** TASK-280 (gameplay-programmer: instrument → confirm → fix, file-only, no compile/Git) → QA (status-flow gate; QA reads the report WITH the attached runtime evidence — a fix whose root-cause claim is not backed by the captured logs is a FAIL) → TASK-281 (build-master: compile + PIE ATTACK full-field-march verification both WITH and WITHOUT enemy presence + branch commit, NO push). QA is the implied gate between them (not a numbered task).

#### TASK-280 — Diagnose (instrument first) + fix the ATTACK march-freeze (C++: `ASummonedUnit`, maybe `EnterAdvance`)
- assignee: gameplay-programmer
- status: done (INTEGRATED at TASK-281, build-master 2026-07-24, commit `0295f75` on m7.6-arena10x — compile GREEN, `EnemyBaseEngageRadius=3500` CDO-confirmed, clean PIE load; `EnemyBaseEngageRadius` folded into CONVENTIONS "Unit commands". --- Prior QA 2026-07-24 — PASS, 0 blockers / 0 warns / 1 wording NIT / 3 TASK-281 WATCH, report `qa/TASK-280.md`. ROOT CAUSE CONFIRMED (evidence-sufficient, verified at source): ATTACK's no-aggro goal `FindNearestEnemyInSpawnBox() ?? castle` flips its nearest-in-box result every 0.25s tick when the bot's spawn box is populated → `EnterAdvance:1832` `bGoalChanged` re-issues a FULL-FIELD `MoveToActor` every tick that never completes on the 10× arena = freeze (NEW to the widened map). Empty-box path proven byte-identical to the legacy `EnterAdvance(FindNearestEnemyCastle())` march (captured healthy full-field +25000→−24520); navmesh disproven (bounds cover both halves). FIX: gate box-defender preference by proximity — prefer a box defender only within new `EnemyBaseEngageRadius` (EditDefaultsOnly, 3500, FLAGGED tunable) of the enemy castle, else march the stable castle; confined to the ATTACK no-aggro block. Non-regression verified: legacy body byte-identical, DEFEND/HOLD/`EnterAdvance`/`FindNearestEnemyInSpawnBox`/Castle/bot UNTOUCHED. Geometry: 3500 >> the 840 box, so "clear box defenders before the castle" holds close-in; residual short-path re-issue harmless. Live Blue-ATTACK T-key capture = TASK-281 WATCH, not a gate. Fold `EnemyBaseEngageRadius` into CONVENTIONS "Unit commands" at integration.)
- blocked-by: none — **dispatchable NOW**
- parallel-safe: yes (owns `SummonedUnit.{h,cpp}`, and possibly `EnterAdvance` within it — already branch-frozen for W1; no other OPEN task touches these files; disjoint from L_Arena.umap / DA_BattlefieldScatter / SiegeBotController / SiegePlayerController / CaptureZone / DeckBuilderWidget)
- spec: >
    On `m7.6-arena10x`. FILE-ONLY — NO compile, NO Git (TASK-281 owns those). Fix the ATTACK stance so Standard PLAYER (Blue)
    units march the FULL field to the enemy base instead of freezing just past midfield. DEFEND/HOLD are confirmed-good and
    MUST NOT be disturbed. **This task is DIAGNOSE-FIRST — the mechanism is CONFIRMED at runtime before any logic changes.**
    (1) **REPRODUCE + INSTRUMENT FIRST.** Add TEMPORARY `UE_LOG` (a dedicated log category or a clearly-marked temp block —
    something you will strip/demote before ready-for-qa) on an ATTACK-commanded Standard Blue unit, once per state tick,
    printing: the unit name, its world position (esp. X so "past the midpoint" is locatable — castles are at ±25,000, midfield
    X=0), `State`, `CurrentTarget` (name or null), the COMPUTED `Goal` (name), whether `Goal` CHANGED since the previous tick
    (the exact thing `EnterAdvance`'s `bGoalChanged` tests), and the `AAIController` MoveStatus (`GetMoveStatus()`). Run PIE,
    issue ATTACK (or drive the command state directly if hardware input can't be injected headless — command state is not a
    UPROPERTY; note honestly what you could/couldn't script), let units march past midfield, and READ `Saved/Logs/`. CONFIRM
    the actual freeze mechanism from the log BEFORE touching logic.
    (2) **THE DECISIVE CONTROLLED TEST — run BOTH conditions and record both:** issue ATTACK with the enemy half (a) POPULATED
    with bot units in/near the enemy spawn box, and (b) EMPTY of any enemy unit/building inside the enemy spawn box.
    • If units freeze past midfield EVEN with an EMPTY enemy box → `FindNearestEnemyInSpawnBox` returned null → `Goal` was the
      STABLE enemy castle → the goal-jitter hypothesis is WRONG; the cause is elsewhere (navmesh/`MoveToActor` partial-path at
      the enemy-half boundary on the widened field, `bAllowPartialPath` ending the path short, a scatter obstacle, or something
      the static read missed) — chase THAT.
    • If units freeze ONLY when bot units are present in/near the box → goal-jitter is supported (the `Goal` actor pointer flips
      tick-to-tick between box defenders and/or acquired targets → `EnterAdvance` re-issues `MoveToActor` every tick).
    The two conditions tell you WHERE to look — do NOT skip either.
    (3) **FIX that preserves intent + does NOT regress DEFEND/HOLD/legacy.** ATTACK must STILL "clear enemy units/buildings in
    the enemy spawn box before the castle," but the long mid-field march must be STABLE. Exact shape is YOUR call AFTER the
    diagnosis; likely candidates (pick per the confirmed cause, combine as needed):
      • only switch the no-aggro goal to a box-defender when the unit is actually NEAR the enemy base (march to the STABLE
        enemy castle otherwise) — so midfield marching always targets the stable castle;
      • and/or STABILIZE the goal / the re-path so `EnterAdvance` does not re-issue `MoveToActor` every tick when the goal is
        essentially the same direction or the unit is still making forward progress (e.g. tolerate a co-directional goal-actor
        swap, or hysteresis on goal switching);
      • and/or if the cause is navmesh/partial-path, address the pathing directly (do NOT paper over it with a goal change).
    DO NOT touch: the DEFEND and HOLD branches (fixed-goal, working), the legacy Standard body at `SummonedUnit.cpp:1064-1102`
    (must stay byte-identical when the command gate is false — bot/Red, miners/None, Siege/Support, pre-first-command all fall
    through it), `Castle`'s box helper beyond what the fix strictly needs, the capture-zone code, and the bot. **If the fix
    lands in shared `EnterAdvance`, it MUST be scoped so legacy marching behavior is effectively unchanged** (any change to how
    ALL marching units re-path is a WATCH — flag it explicitly in the handoff and keep it a no-op for the legacy fall-through).
    (4) **BEFORE ready-for-qa:** strip the temporary diagnostic logs OR demote them to a dedicated `Verbose`-level category
    (no per-tick spam at default verbosity). Keep any log a genuine, permanent, low-noise diagnostic only if it earns its place.
    (5) **HANDOFF (`handoffs/TASK-280.md`) MUST contain:** the captured log evidence for BOTH test conditions (2), the
    CONFIRMED mechanism (with the log lines that prove it), the chosen fix shape + why it follows from the evidence, an explicit
    statement that DEFEND / HOLD / the legacy body / the bot are unchanged (and — if `EnterAdvance` was touched — proof the
    legacy re-path is a no-op delta), and any new identifier introduced (see names). QA implied (shadow scan — no shadow of
    inherited reflected members; complete-type include scan; null-safety on all lookups; confirm the legacy/DEFEND/HOLD/bot
    paths are untouched) and QA reads this report WITH the runtime evidence. NOT IN SCOPE: input assets, the HUD, Castle logic
    changes beyond the fix's need, the bot, compiling, Git. Post in ⚙️ Dev & QA (dispatch + progress + ready-for-qa).
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (the ATTACK branch in `UpdateStateStandardCommanded`, and/or
    `EnterAdvance` / `FindNearestEnemyInSpawnBox` as the diagnosis dictates). Consumed as-is: `ESiegeUnitCommand`,
    `FindNearestEnemyCastle`, `FindNearestEnemyInSpawnBox`, `AcquireTarget`, `EnterAdvance`, `EnterAttack`, `EnterIdle`,
    `CurrentMoveGoal`, `ACastle::IsPointInSpawnBox`, `StructureMoveAcceptanceRadius`. **Minimize new identifiers.** IF the fix
    gates the box-defender goal by proximity to the enemy base, use the manager-sanctioned tunable name
    `EnemyBaseEngageRadius` (`float`, `UPROPERTY(EditDefaultsOnly)`, ClampMin 0, doc comment citing this task + FLAGGED-tunable
    note); ANY durable new tunable follows the existing SummonedUnit UPROPERTY pattern and is RECORDED in the handoff — the
    manager folds a one-line entry into CONVENTIONS "Unit commands (Shield Wall stances)…" at integration (do NOT edit
    CONVENTIONS in this task). A goal-stabilization fix that needs NO new member is preferred where it is equally correct.
    Law: CONVENTIONS "Unit commands (Shield Wall stances) — ATTACK / HOLD / DEFEND (W1, 2026-07-23)".

#### TASK-281 — Integration: compile + PIE ATTACK full-field-march verify (with/without enemy) + branch commit (build)
- assignee: build-master
- status: done (BUILD-MASTER 2026-07-24 — ATTACK march-freeze fix integrated on m7.6-arena10x, commit `0295f75` (no push). Step-0 PIE check idle (Jonathan NOT mid-playtest); editor closed clean + recompiled GREEN (~14s); relaunched on L_Arena. **GIT-HYGIENE VERDICT:** the QA-flagged files (`SiegeBotController`/`SiegeGameMode`/`SiegePlayerController`/`CaptureZone`) are NOT modified — all clean/committed (SiegePlayerController @70487d5, SiegeBotController @3c32e25, CaptureZone/SiegeGameMode in the base); the QA warning reflected a stale pre-commit snapshot (my TASK-277/279 commits already integrated the genuinely in-flight ones). Only unexpected modified code = `DeckBuilderWidget.{h,cpp}` (TASK-268 parked, left UNSTAGED). TASK-280 diff CONFINED to `SummonedUnit.{h,cpp}` (verified: ATTACK no-aggro block gate + new `EnemyBaseEngageRadius` UPROPERTY, nothing else). PIE: `EnemyBaseEngageRadius=3500` CDO-confirmed; build loads/runs a clean match (no errors/ensures/Accessed-None); bot marches normally. Live Blue-ATTACK T-key full-field march (populated + empty Red box), close-in stutter inside 3500, and the DEFEND/HOLD/Siege/Support/miner/bot regression pass = Jonathan W1 WATCH (no blind input injection into his active desktop — QA proves the fix by equivalence). Committed: `SummonedUnit.{h,cpp}`, CONVENTIONS (`EnemyBaseEngageRadius` line), board + `handoffs/TASK-280.md` + `qa/TASK-280.md`. No push. Was: backlog.)
- blocked-by: TASK-280 (qa-passed)
- parallel-safe: no (single editor + compiler + Git)
- spec: >
    On `m7.6-arena10x`. (1) Compile TASK-280's C++ (editor bounce as usual — Jonathan's close/reopen grant covers this
    session). GREEN, report time + `Result: Succeeded`. Failure → append errors to `qa/TASK-280.md`, route back to
    gameplay-programmer (counts as a QA loop). (2) **VERIFY branch-owned files untouched:** `git diff --stat` should show ONLY
    `SummonedUnit.{h,cpp}` (+ board/CONVENTIONS if the manager folded a tunable line); if `L_Arena.umap` /
    `DA_BattlefieldScatter` / `SiegeBotController` / `SiegePlayerController` / `Castle.{h,cpp}` / `CaptureZone` /
    `DeckBuilderWidget` changed unexpectedly, STOP and report. (3) **PIE ATTACK full-field-march verify — THE fix criterion,
    run BOTH conditions:** spawn player Standard (Blue) units, put them under ATTACK, and confirm they march the FULL field and
    reach the enemy base (Castle_Red / its spawn box), NO freeze past midfield — (a) WITH enemy units/buildings present in/near
    the Red spawn box (units clear the box defenders, then the castle), and (b) WITHOUT any enemy in the Red spawn box (units
    march straight to Castle_Red). Best-effort + honest about what needs Jonathan's hands (real T-key input on an unlocked
    desktop; command state is non-UPROPERTY + no headless input injection — drive it however the source allows and say plainly
    what was machine-verified vs. structural). Also confirm DEFEND and HOLD still behave (regression check — they were the
    working stances), and that Siege/Support/miners/bot units are unaffected. (4) **COMMIT on the branch** referencing
    TASK-280/281 + Jonathan's report. **DO NOT PUSH.** (5) Record the human WATCH (ATTACK units cross the full 10× field and
    reach the enemy base with real T-key input; the box-defender-first priority still reads) for Jonathan's W1 look. Leave the
    editor running + saved. Post results + hash in 🔧 Build & Git.
- names: >
    Branch `m7.6-arena10x` commit (NO push); `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (+ board/CONVENTIONS
    if a tunable line was folded in). Law: CLAUDE.md hard gates (PASS QA before commit, never push unasked), M7.6
    branch-ownership, CONVENTIONS "Unit commands (Shield Wall stances)…".

---

