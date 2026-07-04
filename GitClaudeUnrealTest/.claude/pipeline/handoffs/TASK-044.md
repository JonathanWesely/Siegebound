# Handoff — TASK-044: Team-driven visuals (MI_TeamColor by Team at BeginPlay)

**Assignee:** gameplay-programmer
**Status:** ready-for-qa
**Files only. No compile, no Git, no board edit. TASK-045's files (SiegeBotController, SiegeGameMode) NOT touched.**

## What this does
At runtime, every team-owned card actor recolors its `VisualMesh` slot 0 to the
`MI_TeamColor` matching its **actual** `Team`, so the M3 bot's Red-spawned units/
buildings/miners read red while reusing the SAME `BP_Unit_*` / `BP_Building_*`
assets the player uses (no Red BP duplicates — the M3 design ruling). The
BP-authored `MI_TeamColor_Blue` stays the design-time placeholder; this overrides
by `Team` at BeginPlay.

- Blue → `/Game/Materials/Instances/MI_TeamColor_Blue`
- Red  → `/Game/Materials/Instances/MI_TeamColor_Red`

`AGoldNode` is untouched (keeps `M_GoldGlow`) — exempt per spec/CONVENTIONS.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` — private `void ApplyTeamMaterial();` decl + doc.
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — `#include "Materials/MaterialInterface.h"`; `ApplyTeamMaterial()` def; call in `BeginPlay()`; idempotent re-apply in `InitUnit()` (HasActorBegunPlay branch).
- `Source/GitClaudeUnrealTest/Siegebound/Building.h` — private `void ApplyTeamMaterial();` decl + doc.
- `Source/GitClaudeUnrealTest/Siegebound/Building.cpp` — `#include "Materials/MaterialInterface.h"`; `ApplyTeamMaterial()` def; call in `BeginPlay()`; idempotent re-apply in `InitBuilding()` (HasActorBegunPlay branch).
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h` — new one-shot guard `bWarnedNoTeamPlayerState` (TASK-043 WARN closure).
- `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` — one-shot guard in `ResolveOwningPlayerState()` (TASK-043 WARN closure). **No material code — miners are covered by the base (see below).**

## Where the apply lives per actor type (base vs override)
- **ASummonedUnit** — `ApplyTeamMaterial()` called at the end of `BeginPlay()` (after the impact-effect resolve, before `LoadStatsAndStart()`).
- **AMinerUnit** — **inherited, no new code.** `AMinerUnit::BeginPlay()` calls `Super::BeginPlay()` first, so the base apply runs with the miner's own `Team`. Confirmed: the base apply covers miners; MinerUnit.cpp gets ONLY the TASK-043 WARN guard.
- **ABuilding** — `ApplyTeamMaterial()` called at the top of `BeginPlay()` (right after `Super::BeginPlay()`, before `LoadStats()`).

All three end up with the correct team material.

## How `Team` is correct at BeginPlay (spawn-timing)
The real spawn path is **deferred**: `SiegePlayerController` (and the TASK-046 bot,
reusing the same rules) does `SpawnActorDeferred → InitUnit/InitBuilding(Team, CardID)
→ FinishSpawning` (SiegePlayerController.cpp:865-943). `Team` is therefore set
**before** BeginPlay, so the single BeginPlay apply lands the correct color for
every real spawn (Blue player + Red bot).

Belt-and-braces for the documented plain-`SpawnActor` + `Init*` path (BeginPlay
already ran with the default team): `InitUnit`/`InitBuilding` re-apply the material
in the `HasActorBegunPlay()` branch, after `Team = InTeam`. Deferred callers run
`Init*` pre-BeginPlay (`HasActorBegunPlay()` false), so they hit exactly one apply
(in BeginPlay); plain-SpawnActor callers get the re-apply. Idempotent and null-safe.
Mirrors AProjectile's `ApplyTeamVisuals` idempotent-reapply precedent.

## Soft-ref caching ("cached static resolve")
`ApplyTeamMaterial()` resolves the two MI instances via **function-local `static
const TSoftObjectPtr<UMaterialInterface>`** (one Blue, one Red), shared process-wide
across every unit/miner/building — resolved once, never a per-attack/per-frame load.
`LoadSynchronous()` re-resolves through the soft path if GC ever unloaded them and
returns nullptr for a missing asset, in which case the slot is left as authored
(**null-safe — never a crash**). This mirrors the QA-blessed `ACastle` / `AProjectile`
team-material pattern (they use UPROPERTY soft refs; I used cached statics per the
spec's "cached static resolve" wording, avoiding two extra UPROPERTYs on three
classes). The MI paths are duplicated by hand across SummonedUnit.cpp and
Building.cpp (the file's documented `TryGetDamageTeam` / `GetDistanceToTarget`
mirror precedent) — **keep the two path pairs in sync by hand** if MI paths ever change.

## How Blue-side M1/M2 visuals stay byte-for-byte identical
Per CONVENTIONS + spec, the BP authors `MI_TeamColor_Blue` on `VisualMesh` slot 0.
For a Blue actor, `ApplyTeamMaterial()` re-applies the **identical** `MI_TeamColor_Blue`
asset to slot 0 → same `UMaterialInterface`, no MID created, identical rendering.
The apply is material-slot-0 only: the **-90° yaw VisualMesh convention**, the M1
melee/TASK-020 lunge path, the M2 ranged path, and TASK-042's `ApplyMoveSpeedBuff`
are all untouched. Blue actors look exactly as they do today.

## TASK-043 WARN — CLOSED
The mis-teamed-miner retry poll re-logged ~4/s because the null-PS-**for-team**
branch lacked a one-shot guard (`bWarnedNoOwnerState` only covered the no-GameState
branch). Added `bWarnedNoTeamPlayerState`: in `ResolveOwningPlayerState`, once
`GetPlayerStateForTeam(Team)` returns null **with a live GameState**, the flag latches
and subsequent polls short-circuit before re-calling the accessor — so
`GetPlayerStateForTeam`'s unconditional not-found log fires **exactly once** instead
of every 0.25 s. The no-GameState-yet branch (case 1) is a SEPARATE branch and keeps
retrying (unchanged). In all designed flows the owning economy exists before any
miner spawns, so this guard never engages (first lookup succeeds); it only affects a
genuinely mis-teamed miner, which correctly stops re-querying and idles without income.
**No SiegeGameState.cpp edit** (the alternative "rate-limit inside GetPlayerStateForTeam"
was NOT taken — kept the fix in MinerUnit.cpp per the bonus scope).

## For QA to scrutinize
- **C4458 shadow:** material var named `TeamMat` (const-ref) + `ResolvedTeamMat` local; miner local `OwnerState` (already an established safe local name in this file). None shadow inherited reflected UPROPERTYs (Owner/PlayerState/Instigator/Controller/Slot).
- **Byte-for-byte Blue:** re-applying the identical `MI_TeamColor_Blue` to slot 0 is a visual no-op — confirm the BP-authored slot-0 material is indeed `MI_TeamColor_Blue` (the CONVENTIONS/spec contract). If any BP authored a different slot-0 material, my apply would force Blue — that is an art discrepancy outside this task's scope.
- **Includes:** `Materials/MaterialInterface.h` added to both cpp (needed for `LoadSynchronous`/`SetMaterial` full type); mirrors Castle.cpp/Projectile.cpp.
- **WARN guard:** confirm the one-shot short-circuit sits AFTER the `!SiegeGameState` early return (it does — MinerUnit.cpp:365, after line 349).
- **AMinerUnit coverage:** confirm reliance on `Super::BeginPlay()` (MinerUnit.cpp:69) for the miner's material apply is acceptable (no material code duplicated into the miner).

## Constraints honored
No compile, no Git, no TASKBOARD.md edit. SiegeBotController / SiegeGameMode (TASK-045) untouched. AGoldNode untouched.
