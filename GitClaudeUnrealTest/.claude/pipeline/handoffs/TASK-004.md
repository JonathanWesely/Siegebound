# TASK-004 Handoff — Summoned unit AI, Standard profile (C++)

- author: gameplay-programmer
- date: 2026-07-02
- status: implementation complete, files-only (no compile, no editor, no Git per task constraints)

## Files

1. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` (new)
2. `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (new)
3. `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp` (sanctioned addendum only, see below)
4. `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` — **NOT touched.** `AIModule` was already a public dependency; `NavigationSystem` is not needed (only `AAIController::MoveToActor` + `Navigation/PathFollowingComponent.h` enums are used, both AIModule; no direct `UNavigationSystemV1` calls).

Include as `#include "Siegebound/SummonedUnit.h"` (module root already in PublicIncludePaths).

## Castle.cpp addendum (pre-approved, qa/TASK-002-report.md WARN)

One functional line in `ACastle`'s constructor: `CastleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);` plus the required `#include "Engine/CollisionProfile.h"`. This makes the BlockAll profile the handoff promised explicit instead of an engine default. No QA loop needed per the report ("fold the one-line fix into the next Castle.cpp touch").

## Class contract

`UCLASS() class GITCLAUDEUNREALTEST_API ASummonedUnit : public ACharacter, public ITeamAgent`

- `AutoPossessAI = PlacedInWorldOrSpawned`, `AIControllerClass = AAIController` — works placed in a level (TASK-010 acceptance) and spawned at runtime (TASK-007).
- `EditAnywhere FName CardID` (default `NAME_None` — deliberately NOT `Footman`; nothing is implicitly a Footman, §3.0), `EditAnywhere ETeamId Team` (default Blue), `GetTeamId()` override.
- Component `VisualMesh` (`UStaticMeshComponent`, subobject name exactly `VisualMesh`) attached to the capsule; **no mesh set in C++**; NoCollision profile, no overlaps, `SetCanEverAffectNavigation(false)` so units never carve nav under their own feet. Capsule owns all collision.
- Stats bound at BeginPlay from soft ref `/Game/Data/DT_Cards.DT_Cards` (`FCardRow` row = CardID): HP → MaxHP/CurrentHP, Speed → MaxWalkSpeed, Damage/Range/Cadence → attack params. Missing table, missing row, or unset CardID → `UE_LOG` Error + unit idles. Non-Standard profile rows log a warning and run Standard (TODO(M2) marked).
- BlueprintPure: `GetCurrentHP()`, `GetMaxHP()` (TASK-010 PIE verification), plus `IsUnitDead()`, `GetCardID()`, `GetUnitState()`.

## State machine (Standard profile, GDD §3.8)

Runs on a looping timer (`StateCheckInterval` = **0.25 s**, EditAnywhere), never per-tick. Persistent states: Idle / Advance / Attack (`ESummonedUnitState`); Acquire and Reacquire are the per-check decisions.

Parameters (EditAnywhere UPROPERTY defaults; these are GDD §3.8 profile constants, not table stats):
- `AggroRadius` = 600
- `LeashRange` = 900
- `TieBreakDistance` = 100

Per check (`UpdateState`):
1. **Reacquire/leash:** current target dropped if `!IsValid`, castle `IsCastleDestroyed()`, hero `IsDead()`, unit `IsUnitDead()`, or distance > 900.
2. **Acquire:** nearest alive enemy `ITeamAgent` within 600, re-evaluated every check. Tie-break: candidates split into pawns (units/hero) vs non-pawns (buildings/castle); if the nearest overall is a non-pawn and the best pawn stands within 100 units of it (collision-aware distance), the pawn wins. Only the two bucket winners are compared — adequate for the spec's two-candidate wording. A still-leashed target beyond 600 keeps being chased when nothing is inside aggro.
3. **Attack:** target within `Range` (closest-point-on-collision distance, mirroring TASK-003 hero melee — mandatory for the castle's ~800x800 footprint) → stop movement, dedicated attack timer at `Cadence`. First hit is immediate if the cadence has elapsed since the last landed hit, else fires after the remainder (`LastAttackTime` gate) — no machine-gunning via target swaps or range flapping. Damage via `UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, UDamageType)` — unit is DamageCauser AND its AI controller is EventInstigator, satisfying the TASK-002 castle attribution contract via both resolution steps.
4. **Advance:** `MoveToActor` toward the acquired target, else the nearest standing enemy castle (actor iteration, skips same-team and `IsCastleDestroyed()`). No enemy castle at all → Idle. Moves are only (re)issued when the goal changes or path following goes Idle (paths tether to moving goal actors).

**Castle-approach detail (QA please scrutinize):** for non-pawn goals, `MoveToActor` is issued with `bStopOnOverlap = false`, acceptance 50, `bAllowPartialPath = true`. With overlap-reach enabled, the reach test uses the goal's bounding CYLINDER (radius ~566 for the 800x800 castle) and would stop units ~165+ from the wall — permanently out of the 120 attack range. The partial path instead ends at the nav edge flush against the castle wall; the closest-point range check then flips the unit into Attack. Pawn goals use `bStopOnOverlap = true` with acceptance `max(Range*0.8, 40)`.

## Damage in/out

- **Incoming:** `TakeDamage` override mirrors `ACastle::TryGetInstigatorTeam`'s exact chain (instigator controller's pawn → damage causer → causer's instigator pawn); same-team damage returns 0 without calling Super (the pattern QA approved in TASK-002 ruling 1). Unattributable damage applies.
- **Death:** at 0 HP — timers cleared, movement stopped, `Destroy()`. Units don't respawn. The auto-spawned `AAIController` has no PlayerState, so `AController::PawnPendingDestroy` destroys it with the pawn (verified against 5.8 engine source) — no controller leak across repeated summons.

## What TASK-007 (spawner) needs

- Preferred: `SpawnActorDeferred<ASummonedUnit>` (or the BP_Unit_Footman class) → `InitUnit(ETeamId::Blue, FName("Footman"))` → `FinishSpawning(Transform)`. BeginPlay then binds the stats.
- Also supported: plain `SpawnActor` + `InitUnit(...)` immediately after — if BeginPlay ran without a CardID (logged error, idled), `InitUnit` late-binds the stats and starts the state machine.
- When spawning `BP_Unit_Footman` (which sets CardID = Footman itself, TASK-010), `InitUnit(Team, FName("Footman"))` is still correct — matching CardID is a no-op on the card, and Team is applied regardless.
- CardID must match a DT_Cards row name exactly (`Footman`); Team must be set before the first state check (0.25 s after BeginPlay) or the unit may briefly target as Blue — deferred spawn avoids this entirely.
- Spawn collision handling: unit is a normal Character capsule (default size until TASK-010); use AdjustIfPossibleButAlwaysSpawn.

## What TASK-010 (BP_Unit_Footman) must set

- Parent class `ASummonedUnit`; `CardID = Footman`; default `Team = Blue`.
- `VisualMesh` component: assign `/Game/Meshes/SM_Footman` + `/Game/Materials/Instances/MI_TeamColor_Blue`. Leave its collision profile at NoCollision (set in C++); do not enable collision on it.
- Size the capsule to the mesh (~90 half-height) and offset `VisualMesh` down by the capsule half-height so feet touch ground (mesh origin is feet-center per TASK-014).
- Set NOTHING stat-like — HP/speed/damage all come from DT_Cards; the C++ logs an error and idles if the row is missing (that is the §3.0 guardrail working, not a bug).
- PIE verification hooks: `GetCurrentHP` / `GetMaxHP` (expect 80/80), `GetUnitState`, plus MaxWalkSpeed 400 on the movement component after BeginPlay.

## QA scrutiny points

1. `Cast<ITeamAgent>` native casts throughout — sanctioned by TASK-002 QA ruling 2 (UTeamAgent is NotBlueprintable).
2. `IsTargetAlive` includes `Siegebound/HeroCharacter.h` (TASK-003, currently ready-for-qa) to call the handoff-documented `IsDead()`. If TASK-003 QA renames that API, this file needs the same rename.
3. Acquisition re-evaluates "nearest" every 0.25 s per the literal §3.8 wording — a unit mid-fight switches to a strictly nearer enemy. Two equidistant pawns could in principle flip-flop; deterministic tie-break exists only for the pawn-vs-building case (the one the GDD defines).
4. Attack cadence uses a dedicated looping timer at the exact row Cadence (not quantized to the 0.25 s state check); first-hit gating via `LastAttackTime`. Footman: 12 damage every 1.0 s, first hit on entering range.
5. `EnterAdvance`'s move-failed warning fires only on goal changes (throttled); missing navmesh (TASK-015 not integrated) logs once per goal, retries every check by design.
6. Aggro/range/leash distances are closest-point-on-collision (`ActorGetDistanceToCollision`, ECC_Pawn) with origin-distance fallback when a target has no collision (e.g. SM_Castle not yet imported) — identical approach QA reviewed on the hero (TASK-003).
7. `MinAttackCadence` (0.05 s) clamp exists only to keep the looping timer legal if a CSV row ever ships Cadence ≤ 0; it does not alter Footman's 1.0.
