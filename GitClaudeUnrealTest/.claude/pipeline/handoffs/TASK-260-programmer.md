# TASK-260 handoff — ACaptureZone actor: ownership state + interval capture eval + spawn API

Status: **ready-for-qa**
Agent: gameplay-programmer
Branch: m7.6-arena10x
Date: 2026-07-23
Lane: branch (frozen on main until the Phase-6 merge)

## What I built
NEW gameplay actor `ACaptureZone` — the capturable mid zone (W1-PREP additions 3). Code-only, not compiled (build-master owns compile at TASK-264).

### New files
- `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.h`
- `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.cpp`

### Edited file (reset wiring — see "Play-Again reset" below)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp` — added `#include "Siegebound/CaptureZone.h"` and a one-loop reset step in `PlayAgain()`.

## JONATHAN RULING (2026-07-23) — Contested → NEUTRAL (shipped)
The board defaulted contested behavior to STICKY. Jonathan RULED the opposite: a contested zone **neutralizes** ("live tug-of-war, must be held"). Implemented as the shipped behavior.
- `bNeutralizeWhenContested` UPROPERTY(EditDefaultsOnly) is KEPT (sticky survives as the off-state) but now **DEFAULTS TRUE**.
- Board updated: a RULING line was added under TASK-260's status so the board stays authoritative.
- **Flag for the manager:** CONVENTIONS.md "W1-PREP additions 3" (capture-rule bullet, ~line 182) still reads "STICKY = interpretation A ... default false". That law file is manager-owned — please reconcile it to the ruling (Contested→Neutral, toggle default true) so CONVENTIONS matches the board.

## Exact capture state machine (as implemented)
`ECaptureState CaptureOwner` (latched, init Neutral). Every `CaptureEvalInterval` (0.5 s) an **authority-only** repeating timer counts units inside the box:
- `blue` = Blue-team units inside, `red` = Red-team units inside.
- **"Unit"** = any `ASummonedUnit` (miners included — `AMinerUnit` is a subclass, so `TActorIterator<ASummonedUnit>` covers them) OR the `AHeroCharacter`. Dead units/hero are skipped (`IsUnitDead()` / `IsDead()`). Buildings, towers, castles, and gold nodes are excluded by type (they are neither `ASummonedUnit` nor `AHeroCharacter`).
- Transitions each interval:
  - `blue>0 && red==0` → Blue
  - `red>0 && blue==0` → Red
  - `blue>0 && red>0`  → **Neutral** (the ruling; sticky if the toggle is off)
  - `blue==0 && red==0` → UNCHANGED (empty latches the last owner; a Neutral zone stays Neutral)
- On an actual change: re-tint the decal MID + `OnCaptureOwnerChanged.Broadcast(this, CaptureOwner)`.

## Public API (for TASK-261 player + TASK-262 bot to consume)
All on `ACaptureZone`; both controllers find the single instance via `TActorIterator<ACaptureZone>` (null-safe if absent = pre-capture behavior, mid unspawnable):

```cpp
// THE spawn-enable seam: true iff Point is inside the box AND this team holds the zone.
// Neutral owner => false for both teams.
bool CanTeamSpawnHere(ETeamId Team, const FVector& Point) const;   // BlueprintPure

// 2D (XY) box test about the actor origin vs ZoneHalfExtent (Z ignored). The shared
// "is this point in the mid zone" geometry test.
bool IsPointInZone(const FVector& Point) const;                    // BlueprintPure

ECaptureState GetCaptureOwner() const;                             // BlueprintPure
FVector2D    GetZoneHalfExtent() const;                            // BlueprintPure  (default (840,840))

void ResetCaptureZone();                                           // BlueprintCallable (§3.9 reset path)
```
Delegate: `FOnCaptureZoneOwnerChanged` (2 params: `ACaptureZone* Zone`, `ECaptureState NewOwner`) → `UPROPERTY(BlueprintAssignable) OnCaptureOwnerChanged`. Nothing binds it this pass (HUD/VFX hook).

**Recommended 261/262 usage:** `Zone && Zone->CanTeamSpawnHere(MyTeam, Point)` is the whole capture-spawn clause — it already folds the box test AND the owner match. (`IsPointInZone` + `GetCaptureOwner` are exposed too if a controller wants them split.) TASK-261 is Blue, TASK-262 is Red.

## Team identity + authority
- **Team enum reused (not reinvented):** the project's `ETeamId { Blue, Red }` + `ITeamAgent::GetTeamId()` from `Siegebound/TeamId.h` — the exact identity `ASummonedUnit`/`AHeroCharacter` already expose (both `implements ITeamAgent`, `GetTeamId()` public). `CanTeamSpawnHere` takes `ETeamId`; `TeamToState()` maps Blue↔Blue / Red↔Red. No parallel team concept introduced.
- **Authority:** the eval timer is armed only under `HasAuthority()` and ownership lives in the (unreplicated) local-authority world — consistent with `ASiegeGameState`/`ASiegeGameMode` owning match state locally through M7. The class doc + a code comment flag the M8 replication revisit (replicate `CaptureOwner` + the tint) exactly like SiegeGameState's own local-only note. In today's single-player + bot PIE the standalone world is authority, so the eval runs.

## Decal wiring (TASK-263 contract)
- `SceneRoot` (USceneComponent) is the root; `ZoneDecal` (UDecalComponent) attaches with relative rotation **pitch -90** so it projects straight down (−Z) — same recipe as the M_SpellReticle DecalActor.
- `DecalSize = (DecalProjectionDepth=1024, ZoneHalfExtent.X=840, ZoneHalfExtent.Y=840)` — X is the projection half-depth (reaches terrain above/below the origin across the arena's hills), Y/Z are the square footprint. Re-applied in the constructor, `OnConstruction`, and `BeginPlay` so an instance edit to ZoneHalfExtent takes.
- BeginPlay soft-loads `/Game/Materials/M_CaptureZone`, `SetDecalMaterial` then `CreateDynamicMaterialInstance()`, and drives the exact `ZoneColor` vector param by owner: Neutral `(0.5,0.5,0.5)` / Blue `(0.05,0.30,1.00)` / Red `(1.00,0.10,0.05)` (FLinearColor).
- **Null-safe:** missing material ⇒ no MID, no visual, the capture mechanic still runs, logged once (`bWarnedMissingMaterial`).

## Play-Again reset (why the game-mode edit)
Spec: "Play-Again resets CaptureOwner→Neutral (expose a public reset the reset path can call; match how ACastle/AGoldNode reset)." `ACastle` is reset by a `TActorIterator<ACastle>→ResetCastle()` loop inside `ASiegeGameMode::PlayAgain`. I mirrored that exactly for the zone (new step "3a2", right after the ResetCastle loop): `TActorIterator<ACaptureZone>→ResetCaptureZone()`.
- **Why it's needed, not optional:** PlayAgain destroys every unit (step 2) BEFORE the next eval, so the empty-zone eval would LATCH the pre-reset owner (empty = unchanged) and a Blue/Red mid zone would carry into the new match. TASK-264's PIE suite (g) explicitly tests "Play-Again ×3 ⇒ CaptureOwner resets to Neutral" — without this wiring it fails, and build-master writes no code, so this is the correct place for it.
- It is purely additive, mirrors the adjacent ResetCastle/scatter loops, and is null-safe (no zone in the level = no-op). If the orchestrator/QA would rather relocate the wiring to TASK-264, it is a ~10-line revert.
- `ResetCaptureZone()` broadcasts `OnCaptureOwnerChanged` **unconditionally** (reset-path broadcast law, mirroring `ResetClock` broadcasting `OnMatchClockChanged(0)`).

## What QA should scrutinize
- **Shadow law:** member is `CaptureOwner`, never `Owner`. No local shadows of `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`.
- **Complete-type include law (CaptureZone.cpp):** DecalComponent, SceneComponent, MaterialInstanceDynamic, MaterialInterface, SummonedUnit, HeroCharacter, World, TimerManager, EngineUtils, GitClaudeUnrealTest (log) all `#include`d for the members/methods dereferenced.
- The capture rule is symmetric (Blue and Red paths identical); the eval never dereferences a null world/unit and skips dead actors.
- `UDecalComponent::CreateDynamicMaterialInstance()` is the intended MID path (matches the TASK-263 handoff's prescription).
- `HasAuthority()`-gated eval + the M8 replication flag.

## NOT in scope (per the task)
- SiegePlayerController (TASK-261) / SiegeBotController (TASK-262) — untouched; they consume this API next.
- Level placement of `CaptureZone_Center` + centerline (`M_CenterlineStripe` actor) deletion — build-master's TASK-264. Default `ZoneHalfExtent` (840,840) and the (0,0,0) placement need no BP; a plain `ACaptureZone` instance at origin is correct.
- No compile, no Git (build-master, TASK-264).
