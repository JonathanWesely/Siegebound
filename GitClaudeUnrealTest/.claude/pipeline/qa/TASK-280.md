# QA Report — TASK-280

**Verdict: PASS** (0 BLOCKER · 0 WARN · 1 NIT · 3 WATCH for TASK-281)
Branch `m7.6-arena10x` · pre-compile review · files reviewed: `SummonedUnit.h`, `SummonedUnit.cpp` (content only — QA has no Git; diff-confinement is TASK-281 step 2).

---

## Evidence-sufficiency ruling: SUFFICIENT — proceed. Live Blue-ATTACK capture is a TASK-281 WATCH, not a gate.

The task's rule ("a root-cause claim not backed by captured evidence is a FAIL") is MET, because the decisive parts of the diagnosis are captured or code-provable, and the one part that is inferential is not load-bearing for the fix's correctness:

1. **Navmesh branch DISPROVEN by capture.** `NavMeshBoundsVolume_0` AABB X∈[−28000,28000], Y∈[−12500,12500] covers BOTH bases; downward traces hit floor across midfield into the enemy half. The "empty box still freezes → navmesh/partial-path at the enemy-half boundary" branch is ruled out.

2. **Empty-box ATTACK ≡ legacy march — CONFIRMED at the source, and the legacy march is a captured full-field success.** I verified the code equivalence directly:
   - Legacy Standard body (`SummonedUnit.cpp:1082-1084`): `Goal = FindNearestEnemyCastle()` → `EnterAdvance(Goal)`.
   - ATTACK no-aggro block with an EMPTY box: `FindNearestEnemyInSpawnBox` returns null → `Goal = EnemyCastleActor` (where `EnemyCastleActor = FindNearestEnemyCastle()`) → `EnterAdvance(Goal)`.
   These are the **identical call** `EnterAdvance(FindNearestEnemyCastle())` against a stable castle actor. The captured proof: a Red `MilitiaMob` (legacy body, stable Blue-castle goal) marched +25000 → −24520 across the ENTIRE 10x field, `State=Advance`, no freeze. The navmesh is symmetric (bounds cover both halves), so the mirror-direction Blue march traverses the same proven-navigable field. → The stable-goal full-field path is decisively healthy.

3. **`EnterAdvance` re-issue mechanism — CONFIRMED at the source.** `EnterAdvance` (`:1830-1856`): `bGoalChanged = (CurrentMoveGoal != Goal)`; `if (bGoalChanged || MoveStatus==Idle) { MoveToActor(...); CurrentMoveGoal = Goal; }`. A per-tick change of the `Goal` actor pointer therefore tears down and re-issues a fresh `MoveToActor` every 0.25 s state tick, and `CurrentMoveGoal` chases it. On a full-field path this re-request cadence prevents any one long async path query from being followed to completion → near-zero net progress = the freeze. Mechanism is exactly as the handoff claims.

4. **Populated-box precondition — captured live** (bursty spawns 6→24; fresh units sit in the Red box ~1–1.5 s, caught turning over at the box edge). The `FindNearestEnemyInSpawnBox(...) ?? castle` substitution is the ONLY behavioral difference between the ATTACK block and the proven-healthy legacy/empty-box case, so it is isolated by elimination as the cause.

**What is inferential (and why it does not block):** the programmer honestly could NOT drive a live Blue-ATTACK unit headlessly (summon needs a placement click + gold; ATTACK needs a T keypress; `CurrentCommand`/`bHasIssuedCommand` are plain C++, not UPROPERTY, so unsettable via MCP; MCP cannot spawn in PIE; blind input injection into Jonathan's active desktop was rightly declined). So there is no captured `Goal-changed→freeze` log on a live Blue ATTACK unit, and the exact goal-flip cadence (every tick vs every ~1.5 s wave) is not directly measured. This does NOT weaken the verdict because the cause is established by **elimination** (navmesh disproven; box-goal substitution is the sole remaining code difference from a proven-healthy path), and the **fix is correct regardless of the exact flip cadence** — it removes mid-field goal instability entirely. The direct T-key freeze-capture is the correct TASK-281 PIE WATCH.

---

## Fix review — confinement, non-regression, intent, geometry

**Confined to the ATTACK no-aggro block.** The only logic change is inside `case Attack`'s `if (!Goal)` block (`:1300-1320`): compute `EnemyCastleActor`/`EnemyCastle`, gate the box-defender pick by `GetDistanceToTarget(MyLocation, EnemyCastleActor) <= EnemyBaseEngageRadius`, then `Goal = BoxDefender ? BoxDefender : EnemyCastleActor`. Plus the one new UPROPERTY.

**Non-regression — each verified by direct read (not just taken on the handoff's word):**
- **Legacy Standard body** (`:1064-1102`): untouched — `Goal = FindNearestEnemyCastle()` byte-for-byte. Bot/Red, miners/None, Siege/Support, pre-first-command all still fall through it. ✔
- **DEFEND** (`:1246-1277`): untouched. ✔
- **HOLD** (`:1213-1244`): untouched. ✔
- **`EnterAdvance`** (`:1806-1857`): untouched — no shared re-path change, so no all-marching-units WATCH. ✔
- **`FindNearestEnemyInSpawnBox`** (`:1417-1467`) and **`FindNearestEnemyCastle`** (`:1177-1202`): untouched; `FindNearestEnemyCastle` only ever returns `ACastle*`, so `Cast<ACastle>(EnemyCastleActor)` is exact. ✔
- `Castle`, capture-zone, bot: not in the diff. ✔

**Intent preserved + geometry confirmed.** `ACastle::SpawnBoxHalfExtent = (840,840)` (`Castle.h:167`), Z-ignored box test. Gate 3500 >> 840, and the header/comment correctly frame the gate as distance "from the castle walls" (surface distance). The box wall sits ~440 uu beyond the castle collision surface; 3500 opens the box-defender-first preference a comfortable ~3000+ uu before the unit reaches the box, so "clear box defenders before the castle" still holds close-in, with a SHORT remaining path (≤ ~4340 uu = 3500 gate + box span). ✔

**Residual close-in re-path is harmless (reasoning holds).** Inside 3500 the nearest-box-defender can still flip → `EnterAdvance` re-issues — but the path is now short, so a short async path query resolves quickly and the unit keeps advancing (and once within aggro 600, `AcquireTarget` pins `CurrentTarget` as the stable Goal / `EnterAttack` fires). Short-path re-issue ≠ the full-field re-issue that froze. Possible cosmetic stutter close-in only — see WATCH-2.

**Incidental perf note (favorable):** gating the box pick also SKIPS `FindNearestEnemyInSpawnBox`'s `GetAllActorsWithInterface` full-actor scan on every mid-field no-aggro tick (now only runs within 3500). Net reduction in per-tick work vs the pre-fix ATTACK block. No new per-tick cost introduced.

---

## Usual filter
- **UE 5.8 API:** no deprecated/removed calls in the changed block (`GetDistanceToTarget`, `FindNearestEnemyCastle`, `FindNearestEnemyInSpawnBox`, `Cast<>`; `EnterAdvance` internals `MoveToActor`/`GetMoveStatus`/`EPathFollowingStatus` unchanged & current). ✔
- **Null-safety:** `FindNearestEnemyCastle()` may return null → `Cast<ACastle>` is null-safe (null in ⇒ null out) → gate `if (EnemyCastle && …)` short-circuits → `FindNearestEnemyInSpawnBox` (also internally `if(!EnemyCastle) return nullptr`) only called with a valid castle → `Goal = BoxDefender ? BoxDefender : EnemyCastleActor`; if both null, `if (!Goal) EnterIdle(); break;` (match-over, identical to prior behavior). `EnterAdvance(Goal)` never receives null. ✔
- **Shadow law:** locals `EnemyCastleActor`/`EnemyCastle`/`BoxDefender`/`Goal` — none shadow an inherited reflected member (Owner/Instigator/Controller/PlayerState/Slot). ✔
- **Complete-type include law:** `Cast<ACastle>` needs `ACastle::StaticClass()` → `#include "Siegebound/Castle.h"` present at `SummonedUnit.cpp:29` (and ACastle is already dereferenced throughout the compiled file). No new include required; handoff's "no new include" confirmed. ✔
- **const-correctness:** `const ACastle* EnemyCastle` passed to `FindNearestEnemyInSpawnBox(const ACastle*)`; distance/advance take the non-const `AActor*` fine. ✔
- **UPROPERTY / naming:** `float EnemyBaseEngageRadius = 3500.f;` `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siegebound|Commands", meta=(ClampMin="0"))` — mirrors `DefendRadius` exactly, doc comment cites TASK-280 + FLAGGED-tunable. Manager-sanctioned name from the spec. CONVENTIONS correctly NOT edited in this task. ✔
- **No stray diagnostics:** grep for temp `UE_LOG`/`DEBUG`/`TEMP`/`TODO` in the block — none. Handoff's "nothing to strip" confirmed. ✔

## Findings
- [NIT] `SummonedUnit.h:389` / handoff — the gate is described as a "2D closest-point distance," but `GetDistanceToTarget` (`SummonedUnit.cpp:2488`) returns a **3D** closest-point-on-collision distance (`ActorGetDistanceToCollision`). On the near-flat arena (unit & castle at similar Z) 3D ≈ 2D, and this is the same function every other distance gate in the block uses (AttackRange/LeashRange), so it is internally consistent and immaterial. Documentation-wording only — no fix required.

## Notes for build-master (PASS)
- **git diff --stat MUST show ONLY `SummonedUnit.{h,cpp}`** (+ TASKBOARD/CONVENTIONS if the manager folds the tunable line). QA reviewed source CONTENT only (no Git by design); the file-confinement check is your TASK-281 step 2. (Heads-up: this branch also carries unrelated in-flight edits — `SiegeBotController`, `SiegeGameMode`, `SiegePlayerController`, `CaptureZone` — from other tasks; TASK-280 does not touch them, but they will appear in the working tree.)
- Compile is a clean editor-bounce: no new includes, one new member (`EnemyBaseEngageRadius`), no deprecated APIs.

## WATCH (for TASK-281 PIE — carry these into the integration test)
1. **LIVE BLUE-ATTACK FULL-FIELD CAPTURE (the one uncaptured piece).** With a REAL T-key ATTACK, confirm Blue Standard units march the FULL 10x field to Castle_Red / its box with NO freeze past midfield, in BOTH conditions: (a) Red box populated (units clear box defenders, then the castle) and (b) Red box empty (straight to Castle_Red). This is the direct freeze-capture the programmer honestly could not drive headlessly — decisive to close the loop, but not a pre-compile blocker (cause is proven by elimination + the mirror full-field-march proof).
2. **CLOSE-IN STUTTER (cosmetic).** As a unit crosses inside `EnemyBaseEngageRadius` (3500) with a populated box, watch for any residual hitch/stutter from the short-path re-issue. Expected harmless (short path resolves fast; AcquireTarget pins the goal within aggro 600). Flag only if visibly stuttering — it is not a freeze.
3. **REGRESSION CONFIRM.** DEFEND and HOLD still behave (the working stances); Siege/Support/miners and the Red bot (legacy body) unaffected.
