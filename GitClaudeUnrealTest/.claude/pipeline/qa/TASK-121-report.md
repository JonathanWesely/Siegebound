# QA Report — TASK-121 (Debug exec cheats: USiegeCheatManager)
Verdict: **PASS**  ·  Blockers: 0  ·  Warnings: 1  ·  Nits: 3
Reviewed 2026-07-09 (pre-compile; TASK-117 compiles the M6 batch). Files only — not compiled, not committed.

Files reviewed (read in full):
- NEW `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.h`
- NEW `Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp`
- EDIT `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
Cross-read to verify reused paths: `ASiegePlayerController::SpawnUnitSwarm` (.h:305 / .cpp:1980), `ASiegePlayerController::ResolveCardActorClass` (.cpp:1769), `ASiegePlayerState::AddGold`/`SetGold` (.cpp:86/105), `ACastle::TakeDamage`/`TryGetInstigatorTeam` (.cpp:129/250), `ASummonedUnit::TakeDamage`/`TryGetDamageTeam`/`PerformAttack` (.cpp:1570/1601/1266+1323), `ITeamAgent`/`IHealthBarTarget` (TeamId.h:31/39, HealthBarTarget.h:33/49), `ACastle::IsCastleDestroyed` (Castle.h:114).

---

## The crux — shipping-safety: CONFIRMED byte-for-byte

- The ENTIRE footprint of the cheat manager on normal play is one constructor assignment, `CheatClass = USiegeCheatManager::StaticClass();` (SiegePlayerController.cpp:47). A grep of all of `Source/` finds exactly two references outside the cheat's own files: the `#include` (SiegePlayerController.cpp:29) and that assignment (line 47). Nothing else constructs, holds, or invokes `USiegeCheatManager`.
- `CheatClass` is only realized into an instance by the engine via `APlayerController::AddCheats`/`EnableCheats`, which is gated to non-shipping builds with cheats enabled — never in Shipping. Assigning the class in the ctor is inert; no command auto-fires. Normal play (Blue player, bot, gold, combat) is unchanged.
- No `Build.cs` change needed and none made — `UCheatManager`/`UGameplayStatics` are Engine module, already linked.

## Each exec routes through the EXISTING shipping path — CONFIRMED

1. **SummonTestUnit** → `ASiegePlayerController::SpawnUnitSwarm(World, UnitClass, CardName, Team, PC, nullptr, GroundPoint, 1, 0.f)`. Argument order/types match the static signature exactly (.h:305). No bespoke `SpawnActor`. The BP class is composed with the IDENTICAL string the real unit path uses — cheat `/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C` (cpp:144) vs `ResolveCardActorClass` unit branch (cpp:1793), same `LoadSynchronous`, same `IsChildOf(ASummonedUnit::StaticClass())` gate. Composition matches; a valid unit CardID resolves the same class the confirm/bot path would.
2. **ApplyTestDamage** → `UGameplayStatics::ApplyDamage(Target, Amount, nullptr, nullptr, nullptr)` — the exact call units make in `PerformAttack` (SummonedUnit.cpp:1323). No raw HP write. `nullptr` DamageTypeClass is engine-safe (ApplyDamage substitutes `UDamageType::StaticClass()`); the `Amount > 0` guard satisfies ApplyDamage's non-zero requirement.
3. **AddTestGold** → `ASiegePlayerState::AddGold(Amount)` (public, cpp:86) which routes through the private `SetGold` choke-point: `FMath::Clamp(NewGold, 0, MaxGold)` + `OnGoldChanged.Broadcast` on real change (cpp:105-119). No raw `Gold` write. AddGold self-refuses+logs a non-positive amount, so the cheat correctly delegates that guard.

## Null-safety — CONFIRMED (graceful log, never crash)

Every exec early-returns with a `Warning` log on: no owning PC (`GetOuterAPlayerController()` null), no World, and — per command — empty CardID / missing-or-wrong-base BP class / non-positive Amount / no `ASiegePlayerState` / no resolvable damage target. `FindNearestEnemy` and the trace helpers null-check World/PC and `IsValid()` every candidate. Crosshair damage target isn't re-checked for "alive", but `TakeDamage` guards `bDead`/`bDestroyed` and returns 0 — graceful, no crash.

## MANDATORY scans

- **Inherited-reflected shadow (C4457/58/59): CLEAN.** `USiegeCheatManager` derives from `UCheatManager` (a `UObject`, not AActor/APawn/AController/UWidget), so no inherited reflected `Owner`/`Instigator`/`PlayerState`/`Controller` UPROPERTY is in scope. Locals use non-colliding names (`PC`, `World`, `SiegeState`, `MyTeam`, `RefPawn`, `ViewPawn`, `Candidate`, `Team`, `Target`). The controller edit ASSIGNS the inherited `CheatClass` (assignment, not a shadowing redeclaration). No shadow.
- **Complete-type-include (the TASK-110 failure class): CLEAN.** Every dereferenced/cast/spawned/static-called type has its full header in SiegeCheatManager.cpp: PlayerController.h, Pawn.h, Engine/World.h, CollisionQueryParams.h, Engine/EngineTypes.h (`ECC_Visibility`/`FHitResult`), Kismet/GameplayStatics.h, UObject/SoftObjectPtr.h, Castle.h, HealthBarTarget.h, SiegePlayerController.h (`SpawnUnitSwarm`), SiegePlayerState.h (`AddGold`/`GetGold`), SummonedUnit.h, TeamId.h, GitClaudeUnrealTest.h. `GetOuterAPlayerController` comes via CheatManager.h (through the paired header). **Critically, SiegePlayerController.cpp:29 adds the SiegeCheatManager.h complete-type include** required for `USiegeCheatManager::StaticClass()` at line 47 — a forward decl would not suffice. No incomplete-type deref anywhere.

## Findings

- [WARN] SiegePlayerController.cpp:44-47 — Task brief required the edit be "EXACTLY two lines and nothing else." The two FUNCTIONAL lines are present and correct (include at :29, `CheatClass` assignment at :47), and a full-file grep confirms **no other functional change**. However the programmer also added 3 explanatory comment lines (:44-46) plus an inline comment on :29. These are non-functional and harmless (no behavior, no landmine), but they are strictly "more than two lines." Not a blocker; flagged for transparency and for TASK-114's carry-forward (line numbers below).
- [NIT] SiegeCheatManager.cpp:225-232 — ApplyTestDamage's crosshair branch accepts ANY `ITeamAgent` under the crosshair (friend or foe) and, combined with the double-null world-damage choice, will damage a friendly you point at. Intentional per flagged Decision 1 (see ruling). No correctness issue; never crashes.
- [NIT] SiegeCheatManager.cpp:227-231 — crosshair target is not filtered by `IsCombatActorAlive` (unlike the nearest-enemy path). Benign: `TakeDamage` guards dead/destroyed and returns 0. Consider mirroring the alive check only if you want a cleaner "no target" log; not required.
- [NIT] SiegeCheatManager.cpp:174 — the view-forward spawn fallback (`* 800.f`) is a magic literal where the trace path uses the named `SiegeCheatTraceDistance`; cosmetic, defensive-only branch (the arena forward trace essentially always hits ground). No action needed.

## Rulings on the programmer's flagged decisions

1. **Crosshair may hit a friendly (ApplyTestDamage) — ACCEPTED.** For a dev-only cheat, "damage what you point at" is defensible and never crashes. Non-shipping. PASS.
2. **World-damage via double-null instigator/causer — ACCEPTED / VERIFIED.** With `EventInstigator == nullptr && DamageCauser == nullptr`, both `ACastle::TryGetInstigatorTeam` (Castle.cpp:250-280) and `ASummonedUnit::TryGetDamageTeam` (SummonedUnit.cpp:1601-1634) return false → the no-friendly-fire check (Castle.cpp:139 / SummonedUnit.cpp:1580) is skipped → damage lands on either side at base `UDamageType` (100%, no fortification scaling). This is the exact shipping `TakeDamage` path, correctly driven. PASS.
3. **Unit-only, Count = 1 — ACCEPTED.** Composition + `IsChildOf(ASummonedUnit)` matches the real unit resolver; building/spell CardIDs have no `BP_Unit_` class and are refused with a log (no bespoke building spawn — the shared public entry is unit-only). Count 1 is a sensible default; call again for a swarm. PASS.
4. **Spawn location: crosshair ImpactPoint → pawn → view-forward — ACCEPTED.** All three yield a ground point suitable for `SpawnUnitSwarm`'s capsule lift; fallbacks are defensive. PASS.
5. **No board/compile/git — correct per task constraints.** TASK-117 compiles; orchestrator flips the board.

## Notes for build-master (post-compile, headless PIE)
Once TASK-117 compiles: in PIE console, `SummonTestUnit <CardID> <0|1>` (e.g. `SummonTestUnit Footman 0`), `ApplyTestDamage <Amount>`, `AddTestGold <Amount>`. Cheats require an enabled CheatManager (non-shipping PIE has it). Use these to drive TASK-120 and all future combat-side headless checks (overhead bars, castle HP, gold HUD) on the locked desktop.

## Carry-forward for TASK-114 (inherits this SiegePlayerController.cpp state)
TASK-114 edits the SAME file next. Current post-TASK-121 state to preserve:
- **:29** — `#include "Siegebound/SiegeCheatManager.h"` in the alphabetical Siegebound/ block (between HeroCharacter.h and SiegePlayerState.h). Keep it.
- **:44-47** — a 3-line comment block + `CheatClass = USiegeCheatManager::StaticClass();`, immediately after the `DeckComponent` subobject line (:42) in the constructor. Keep the assignment.
- No other line changed. Line numbers below the constructor block shifted by the added lines — TASK-114 should read the file fresh before editing. Do not remove or relocate the two functional TASK-121 lines.
