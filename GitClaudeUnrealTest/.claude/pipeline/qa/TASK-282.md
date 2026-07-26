# QA Report — TASK-282
Verdict: PASS

Branch `m7.6-arena10x` · pre-compile review · files reviewed: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` (only files touched, per handoff). Diagnose-first runtime evidence read from `handoffs/TASK-282.md`.

Fix summary reviewed: the ATTACK-command no-aggro march goal is changed from the TASK-280 box-defender substitution (`FindNearestEnemyInSpawnBox`, gated within `EnemyBaseEngageRadius`) back to the STABLE enemy castle (`FindNearestEnemyCastle`), and both now-dead identifiers are removed.

## Findings
No BLOCKERs. No WARNs.

- [NIT] CONVENTIONS.md:200 — the "Unit commands (Shield Wall stances)" ATTACK clause still documents the removed `EnemyBaseEngageRadius` tunable + "clear spawn-box defenders FIRST" wording. Stale after this fix. Out of scope for TASK-282 (programmer correctly did NOT edit CONVENTIONS); the manager folds this out at integration per the handoff §7 record. Not a code issue.
- [NIT] Castle.h:122 and Castle.h:159 — doc-comments still name `ASummonedUnit::FindNearestEnemyInSpawnBox` and "read by IsPointInSpawnBox". `Castle.{h,cpp}` are OUTSIDE this task's file set, so they could not be touched. Harmless stale text; flagged for a future Castle-scoped pass (handoff §7).
- [NIT] `ACastle::IsPointInSpawnBox` (Castle.h:128 decl / Castle.cpp:364 def) is now ORPHANED — no live caller remains after `FindNearestEnemyInSpawnBox` was deleted. It still compiles (public BlueprintPure, reads the still-live 3-way paired tunable `SpawnBoxHalfExtent`) and breaks nothing. Out of scope; noted for future cleanup only.

## Verification (task checklist)

1. **ATTACK no-aggro goal now = enemy castle, legacy-equivalent — CONFIRMED.** The `case Attack:`/`default:` block (`SummonedUnit.cpp:1297-1328`) is functionally byte-identical to the legacy Standard body (`:1064-1101`):
   - leash/reacquire (`:1297-1300`) ≡ legacy `:1067-1070`
   - `AcquireTarget()` self-defense with `CurrentTarget` precedence (`:1302-1305`) ≡ legacy `:1075-1078` — UNCHANGED, so a defender entering `AggroRadius`(600) on the approach is still engaged first, and at the wall the castle is acquired as `CurrentTarget` → `EnterAttack` fires (the §3-proven castle-kill).
   - `AActor* Goal = CurrentTarget; if (!Goal) Goal = FindNearestEnemyCastle();` (`:1307-1311`) ≡ legacy `:1081-1085` — the STABLE castle goal.
   - null/destroyed-castle path `if (!Goal) { EnterIdle(); break; }` (`:1313-1318`) ≡ legacy Idle path (`break;` vs legacy `return;` is the required switch form).
   - attack/advance dispatch (`:1320-1327`) ≡ legacy `:1094-1101`.
   `MyLocation` is sourced from the single function-scope declaration at `:1209` (no re-declare, no shadow). The ATTACK no-aggro path is now truly legacy-equivalent.

2. **Clean removal — CONFIRMED.**
   - `EnemyBaseEngageRadius`: repo grep shows ZERO occurrences anywhere under `Source/` (gone from the header UPROPERTY and every cpp use). No dangling referrer.
   - `FindNearestEnemyInSpawnBox`: no declaration in `SummonedUnit.h`, no definition in `SummonedUnit.cpp`; the ONLY remaining hit under `Source/` is the explanatory comment at `SummonedUnit.cpp:1285` (text, not code). NO other `.cpp` references it.
   - No Blueprint/asset serializes `EnemyBaseEngageRadius`: `Content/` grep is CLEAN (0 files). The property was `EditDefaultsOnly` default-3500, TASK-280 was code-only, so no BP override exists — removing it cannot warn/break a defaults-override on load.
   - `ACastle::IsPointInSpawnBox` orphaned but not referenced anywhere in a breaking way (confirmed by grep) — harmless, out of scope. See NIT above.

3. **Non-regression — CONFIRMED.** Legacy body (`:1064-1101`), DEFEND case (`:1246-1278`), HOLD case (`:1213-1245`), and the shared `EnterAdvance`/`EnterAttack`/`AcquireTarget`/`FindNearestEnemyCastle` are all UNTOUCHED. The bot is untouched (SummonedUnit-only change). The TASK-280 anti-freeze is SUBSUMED and strengthened — the stable castle is now the goal across the ENTIRE approach, not just mid-field — with NO leftover TASK-280 artifact: the gate member and helper are fully removed and the removal is explained in-code. No conflict.

4. **Usual filter — CLEAN.**
   - Shadow/include: no variable shadowing; `Castle.h` still included (needed by `FindNearestEnemyCastle`/`FindOwnCastle`/`TActorIterator<ACastle>`), `class ACastle;` forward-decl still needed by `FindOwnCastle()` return type — none removed. No now-unused local (the deleted `EnemyCastle`/`EnemyCastleActor`/`BoxDefender` are gone; new block's `Goal`/`Acquired` are both used).
   - UE 5.8: no deprecated/removed APIs in the changed block — all calls are pre-existing project methods.
   - Brace balance: switch/case block braces balanced; function closes cleanly.
   - const-correctness: unchanged from the legacy pattern (`AActor* Goal` non-const, passed to `EnterAdvance(AActor*)`). Fine.
   - Doc-comments updated correctly: header `UpdateStateStandardCommanded` ATTACK bullet (`SummonedUnit.h:547-548`) now reads "mirrors the legacy Standard body exactly (TASK-282)"; cpp explanatory block (`:1282-1296`) accurately describes the removal + mechanism.

## Design-change ruling
The fix RETIRES TASK-275's "ATTACK prioritizes enemy spawn-box defenders BEFORE the castle" (Jonathan's original explicit spec). **As a CODE CHANGE this is ACCEPTABLE and correct** — NOT a code blocker. Rationale: (a) the code is correct and legacy-equivalent; (b) the two requirements are genuinely incompatible against a continuously-respawning bot box — box-FIRST means the box is never empty, so the castle is NEVER attacked, directly violating this task's PRIMARY requirement ("actually ATTACK the enemy castle"); (c) intent is PARTLY PRESERVED via the unchanged `AcquireTarget` self-defense — any box defender that enters `AggroRadius`(600) on the approach is still engaged first via `CurrentTarget` precedence. Treated as a NOTED spec change flagged to Jonathan for his morning design call, per the QA directive. It does not block PASS because the code itself is right.

## Notes for build-master (if PASS)
- Compile is a clean editor-bounce: NO new includes, NO new members (a member + a helper were REMOVED), no deprecated APIs — a net-simplifying change.
- WATCH for the TASK-283 PIE test (the direct capture the programmer could not drive headlessly): with a real T-key ATTACK, confirm Blue Standard units march the FULL 10x field AND enter `EnterAttack` on `Castle_Red` (no halt short) in BOTH conditions — (a) enemy units in/near the Red box, and (b) Red box empty. Regression-confirm DEFEND/HOLD and Siege/Support/miners/bot unaffected.
- Manager fold-out at integration: drop the `EnemyBaseEngageRadius` tunable line + "box-first" wording from CONVENTIONS.md:200 (recorded in handoff §7).
