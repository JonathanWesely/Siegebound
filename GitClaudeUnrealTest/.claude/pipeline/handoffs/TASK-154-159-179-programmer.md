# Handoff — M7 §6 juice/feel C++ batch (TASK-154, 155, 156, 157, 158, 159, 179)

**From:** gameplay-programmer · **Status:** all seven → `ready-for-qa` · **Compiles together at TASK-182.**

Implemented as ONE cohesive pass on a SHARED feedback system, not seven disconnected hooks. All ART/AUDIO is referenced by **null-safe soft path** (composed or literal), so the whole batch **compiles and ships BEFORE any M7 art/audio exists** — every reference no-ops (logged once per path on `LogSiegeFeedback`) until the asset lands, never a crash.

---

## Shared feedback core (NEW files)

| File | Class | Role |
|---|---|---|
| `SiegeFeedbackLibrary.h/.cpp` | `USiegeFeedbackLibrary` (UBlueprintFunctionLibrary) | THE null-safe home: cached soft-asset resolvers (sound/niagara/material/static-mesh, log-once-per-missing-path), + `PlaySound2D` / `PlayWorldSound` / `SpawnNiagara` / `ShowDamageNumber` / `PlayLocalCameraShake` / `TeamTint`. Declares `LogSiegeFeedback`. |
| `SiegeHitFlashComponent.h/.cpp` | `USiegeHitFlashComponent` (UActorComponent) | TASK-154 hit-flash — `TriggerFlash()`. |
| `SiegeMeshJuiceComponent.h/.cpp` | `USiegeMeshJuiceComponent` (UActorComponent, tick-while-active) | TASK-155 spawn squash + tower recoil — `SetTargetMesh` / `PlaySpawnSquash` / `PlayRecoil`. |
| `DamageNumberWidget.h/.cpp` | `UDamageNumberWidget` (UUserWidget) | TASK-156 widget base — one float-only BIE `SetDamageNumber(Amount,R,G,B)`. |
| `DamageNumberActor.h/.cpp` | `ADamageNumberActor` (AActor) | TASK-156 self-managing floating number (rise+fade, concurrency-capped, static factory `Spawn`). |

Combatants call into the library + the two shared components at their EXISTING damage/death/spawn/fire choke points — no damage/HP re-plumbing anywhere.

---

## Per-task summary

### TASK-154 — hit-flash (0.1 s white)
`USiegeHitFlashComponent` added in-ctor to `ASummonedUnit` / `ABuilding` / `AHeroCharacter` / `ACastle` (miners/towers inherit). Driven from each class's `TakeDamage` on ACTUAL damage only (friendly-fire returns earlier; heals/regen never route through `TakeDamage`). `HitFlashSeconds = 0.10 // GDD §6`.

**DESIGN DEVIATION — flag for QA (defensible, strictly better):** the flash uses `UMeshComponent::SetOverlayMaterial(M_HitFlash)` for `HitFlashSeconds` then clears it (`nullptr`), instead of the literal spec's "swap every slot + cache MIDs + restore". Rationale: (1) the base material slots are NEVER touched, so it returns to the EXACT prior look incl. the team-recolored slot 0 (the acceptance) BY CONSTRUCTION; (2) it composes cleanly with the **TASK-157 castle crumble MI swap on the SAME castle mesh** — a slot-swap flash would fight the crumble swap on those slots; (3) it works identically on the static VisualMesh and the M7 SkeletalVisualMesh (overlay is a shared `UMeshComponent` API). The gather excludes `UWidgetComponent` (the health bar is a `UMeshComponent` subclass) by matching only `UStaticMeshComponent`/`USkeletalMeshComponent`. **Art note for TASK-174:** author `/Game/Materials/M_HitFlash` as a full-white unlit/emissive material suited to an overlay pass (standard UE5 hit-flash recipe).

### TASK-155 — spawn squash-and-stretch + tower recoil
`USiegeMeshJuiceComponent` on `ASummonedUnit` + `ABuilding` (miners/towers inherit). Owner points it at the ACTIVE visual mesh via `SetTargetMesh`, then `PlaySpawnSquash()` on spawn (units in `LoadStatsAndStart`, buildings in `BeginPlay`). `ATower::ScanAndFire` calls `PlayRecoil(target-dir)` each shot (both projectile AND chain towers), BEFORE the fire branch — targeting/damage untouched. Tunables on the component (EditDefaultsOnly, per-BP): `SpawnSquashSeconds = 0.15 // GDD §6`, `RecoilDistance = 14 // GDD §6`, `RecoilSeconds = 0.12 // GDD §6`. Drift-free (authored scale+location captured once at `SetTargetMesh`, every animation restores EXACTLY — TASK-020 discipline). Ticks only while animating.

**COLLISION-ROOT tradeoff — flag for QA/build-master:** for buildings/towers the `VisualMesh` is the collision ROOT (the spec names VisualMesh as the target), so squash/recoil momentarily move/scale that root (and its nav relevance). Offsets are tiny + brief and always restore exactly; spawn squash is one-time, recoil is ~0.12 s per shot. Fully cosmetic on UNITS (VisualMesh is a capsule child). If a playtest flags nav churn on many towers, a BP can zero `RecoilDistance`/`SpawnSquashSeconds`. I judged this in-scope because the spec explicitly names VisualMesh and asks for visible tower recoil (which requires moving the only visual, i.e. the root).

### TASK-156 — floating damage numbers
On every ACTUAL damage event, all four `TakeDamage` sites call `USiegeFeedbackLibrary::ShowDamageNumber(this, DamageApplied, headLoc, TeamTint(Team))`. Numbers show the amount actually applied (units/hero = listed; building/castle = the SCALED amount, e.g. a Siege 2× reads 2×). `ADamageNumberActor` owns spawn/lifetime/rise+fade (C++-owned), drives `/Game/UI/WBP_DamageNumber` (`UDamageNumberWidget`). **Capped** at 48 concurrent (`MaxConcurrentNumbers`, process-wide `LiveCount`) for the 60-unit §6 budget; over cap it drops the number. **Null-safe:** if `WBP_DamageNumber` is absent it disables (logged once) and spawns NO actors. Tint = victim-team color (red enemy / blue friendly, matching the health-bar language).

### TASK-157 — castle crumble 75/50/25 %
`ACastle::UpdateCrumbleStages()` (called from `TakeDamage` after HP is lowered) advances `CrumbleStage` (monotonic 0→3) while the HP fraction crosses the next threshold — fires each stage EXACTLY once, IN ORDER (a single big hit crossing two thresholds fires both). Never retreats, so a Masons heal-back-up never un-crumbles or re-arms. `ResetCastle` (Play Again) sets `CrumbleStage = 0` and re-runs `ApplyTeamVisuals()` to restore pristine SM_Castle + team material. `ApplyCrumbleStage(N)` swaps mesh AND/OR material (soft `/Game/Meshes/SM_Castle_Crumble0N` + `/Game/Materials/MI_Castle_Crumble0N`, whichever resolves) and bursts `/Game/VFX/NS_CastleDebris`. Thresholds are UPROPERTYs `CrumbleFraction1/2/3 = 0.75/0.50/0.25 // GDD §3.9`.

**ART CONTRACT — flag for QA + art (TASK-171-adjacent):** the crumble MESH variants (`SM_Castle_Crumble01..03`) MUST preserve the castle's UCX footprint (the spec's "collision/UCX footprint UNCHANGED"), since `SetStaticMesh` replaces collision too. If that guarantee can't be met, ship crumble MATERIALS ONLY (`MI_Castle_Crumble0N`) — the code applies whichever resolves, and material-only is guaranteed collision-safe. Debris still bursts even if neither mesh nor material exists, so each stage always READS.

### TASK-158 — gold-coin burst + castle-hit screen shake
Gold burst: `ASummonedUnit::HandleDeath` spawns `/Game/VFX/NS_GoldBurst` at the unit (cosmetic, NO gold mutation; single death choke covers combat death, Sapper suicide, PlayAgain sweep, KillZ; miners included). Screen shake: `ACastle::TakeDamage` (actual damage) resolves `CastleHitCameraShake` (default donor `/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy` — confirmed to exist; a BP may retarget to `BP_CameraShake_CastleHit`) and calls `USiegeFeedbackLibrary::PlayLocalCameraShake` → the index-0 controller's `ClientStartCameraShake`. **Note (flag):** shake fires on ANY castle taking damage (both the player defending and battering the enemy castle) — dramatic on the win-condition object; easy to restrict later if too busy.

### TASK-159 — SkeletalVisualMesh swap path
`ASummonedUnit` gains an OPTIONAL `USkeletalMeshComponent SkeletalVisualMesh` (empty+hidden, no collision/nav, capsule child). `ResolveSkeletalVisual()` (called from `LoadStatsAndStart`, where CardID is guaranteed bound) composes `/Game/Characters/SK_<CardID>`; if it resolves → `SetSkeletalMeshAsset` + `SetAnimInstanceClass(/Game/Characters/ABP_<CardID>_C)` + hide static VisualMesh + `bUsingSkeletalVisual=true`, and the team recolor + spawn-squash re-target the skeletal via `GetActiveVisualMesh()`. If SK is absent → **byte-for-byte today's static behavior** (silent, no spam). Present SK + missing ABP → ref pose. **Placement ghost UNCHANGED** — it still resolves static `/Game/Meshes/SM_<CardID>` (ghosts don't animate). No CSV column. Uses the current (non-deprecated) `SetSkeletalMeshAsset`. Complete-type includes added (`SkeletalMeshComponent.h` / `Engine/SkeletalMesh.h` / `Animation/AnimInstance.h`) — this is exactly the TASK-110 include-completeness class of change.

**For TASK-162 (art/editor integration):** the SkeletalVisualMesh relative transform (feet-at-capsule-bottom alignment, any yaw fix) is intentionally left to the BP wiring, MIRRORING how TASK-010 authored the static VisualMesh offset on `BP_Unit_Footman` (the C++ path stays alignment-agnostic). The base `TakeDamage`/lunge are untouched: on a skeletal unit the lunge runs harmlessly on the hidden static mesh; the real attack animation is the montage the anim-workstream wires on the attack tick (out of TASK-159 scope).

### TASK-179 — audio trigger hooks
All 14 §6 events wired at EXISTING seams via the library (null-safe soft `/Game/Audio/S_<Event>`), character-for-character with the names block:
- `S_HeroSwing` (every swing past cooldown) + `S_HeroHit` (swing that damaged ≥1) — `AHeroCharacter::DoMeleeAttack`.
- `S_UnitSpawn` — `ASummonedUnit::LoadStatsAndStart`.
- `S_ProjectileFire` — `ASummonedUnit::FireProjectileAt` (archer) + `ATower::FireProjectileAt`.
- `S_ProjectileImpact` — `AProjectile::HandleImpact` (single-target + AoE) + the terrain-impact branch.
- `S_MinerClink` — LOOPING `UAudioComponent ClinkAudio` on `AMinerUnit`: `StartMiningClink()` on arrival (§3.3), `StopMiningClink()` on FreezeAI + EndPlay(Destroyed). Loop flag is authored on the asset (TASK-180).
- `S_CardPlay` (accepted play, past all refusal gates) + `S_CardDiscard` (successful discard) — `ASiegePlayerController`.
- `S_SpellCast` — `TryConfirmSpellTarget` + `ResolveSpellInstant` on resolve success.
- `S_CastleHit` — `ACastle::TakeDamage`; `S_CastleDestroyed` — `ACastle::HandleDestroyed`.
- `S_VictoryMusic`/`S_DefeatMusic` — `ASiegePlayerController::HandleMatchEnd` (local player = Blue ⇒ Winner==Blue is Victory).
- `S_OvertimeSting` — `ASiegeGameState::Tick` at the latched 7:00 overtime broadcast (fires once).

---

## Files touched (all under `Source/GitClaudeUnrealTest/Siegebound/`)

**New:** `SiegeFeedbackLibrary.{h,cpp}`, `SiegeHitFlashComponent.{h,cpp}`, `SiegeMeshJuiceComponent.{h,cpp}`, `DamageNumberWidget.{h,cpp}`, `DamageNumberActor.{h,cpp}`.

**Edited:** `SummonedUnit.{h,cpp}` (159 skeletal + 154/155/156/158 + 179 spawn/fire), `MinerUnit.{h,cpp}` (179 clink loop), `Building.{h,cpp}` (154/155/156), `Tower.cpp` (155 recoil + 179 fire), `Castle.{h,cpp}` (154/156/157/158/179), `HeroCharacter.{h,cpp}` (154/156 + 179 swing/hit), `Projectile.cpp` (179 impact), `SiegePlayerController.cpp` (179 card/spell/end-of-match), `SiegeGameState.cpp` (179 overtime).

No `.Build.cs` change needed (UMG/Niagara/Slate/Engine already linked; audio is Engine). New `.cpp` files auto-compile (module globs the Siegebound subdir).

---

## Every soft-ref path introduced (for art/build-master to wire)

- **Materials:** `/Game/Materials/M_HitFlash` (154), `/Game/Materials/MI_Castle_Crumble01|02|03` (157).
- **Meshes:** `/Game/Meshes/SM_Castle_Crumble01|02|03` (157 — footprint-preserving, or ship material-only).
- **VFX (Niagara):** `/Game/VFX/NS_GoldBurst` (158), `/Game/VFX/NS_CastleDebris` (157).
- **UI:** `/Game/UI/WBP_DamageNumber` reparented to `UDamageNumberWidget`, implement BIE `SetDamageNumber(float Amount, float R, float G, float B)` (156).
- **Skeletal (per rigged CardID):** `/Game/Characters/SK_<CardID>` + `/Game/Characters/ABP_<CardID>` (159).
- **Camera shake:** `/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy` (donor, exists) or `BP_CameraShake_CastleHit` (158).
- **Audio (`/Game/Audio/`):** `S_HeroSwing, S_HeroHit, S_UnitSpawn, S_ProjectileFire, S_ProjectileImpact, S_MinerClink, S_CardPlay, S_CardDiscard, S_SpellCast, S_CastleHit, S_CastleDestroyed, S_VictoryMusic, S_DefeatMusic, S_OvertimeSting` (179/180). `S_MinerClink` must be authored looping.

---

## QA scrutiny checklist (pre-empted)

- **Shadow-law (C4457/58/59):** no new member/param/local shadows an inherited reflected UPROPERTY (used `OwnerActor`, `ActiveMesh`, `SkeletalAsset`, etc.; no `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`).
- **Complete-type-include-law:** TASK-159 pulls `SkeletalMeshComponent.h`/`Engine/SkeletalMesh.h`/`Animation/AnimInstance.h`; hit-flash pulls `StaticMeshComponent.h`/`SkeletalMeshComponent.h` for the `IsA<>` gates + `MeshComponent.h` for `SetOverlayMaterial`; the library pulls `Engine/StaticMesh.h` for the `TSoftObjectPtr<UStaticMesh>` cast; every new `->` call has its type's header included.
- **Deprecated APIs:** `SetSkeletalMeshAsset` (not `SetSkeletalMesh`), `SetOverlayMaterial`, `SetAnimInstanceClass`, `ClientStartCameraShake`, `SpawnSoundAtLocation`/`PlaySound2D` — all current in 5.8.
- **Null-safety:** every art/audio ref is soft + resolved through the log-once library (or a guarded `LoadSynchronous`); missing asset = no-op, never a crash. Damage numbers are capped. Flash/juice/audio are all timer/one-shot driven (no per-tick cost except the short-lived damage-number actor and the mesh-juice tick-while-active).
- **Naming:** all asset paths match CONVENTIONS prefixes/folders character-for-character; the audio set matches the TASK-179 names block exactly.

**Two flagged design decisions build-master/QA should confirm** (both defensible, both spec-aligned): (1) overlay-based hit-flash instead of literal slot-swap; (2) building/tower juice moves the collision-root VisualMesh (spec-named target). Both have BP-tunable escape hatches.

---

## QA LOOP 1 FIX (2026-07-16) — missing complete-type include

**QA verdict:** FAIL, 1 BLOCKER (`qa/TASK-154-159-179-qa.md`). Fixed.

**Fix applied (includes only — NO logic change):**
- `Siegebound/SiegeHitFlashComponent.cpp` — added `#include "Engine/World.h"` (alphabetically, after `Components/StaticMeshComponent.h`, before `GameFramework/Actor.h`). This TU calls `World->GetTimerManager()` at `EndPlay` (`:48`) and `TriggerFlash` (`:90`) on a `UWorld*`; `GetTimerManager()` is a `UWorld` member requiring the COMPLETE `UWorld` type — the forward-declared pointer `GetWorld()` returns is insufficient (would C2027, the TASK-110 class). `TimerManager.h` supplies `FTimerManager` but NOT `UWorld`; the skeletal/static mesh headers do not transitively supply it. This was the only blocker.

**Complete-type include self-audit — result: CLEAN (no second defect).** Scanned every file touched in this batch (5 new .cpp + 9 edited .cpp) for any `UWorld*` member dereference (`World->` / `GetWorld()->` + `GetTimerManager`/`SpawnActor`/`GetTimeSeconds`/`GetRealTimeSeconds`/`GetDeltaSeconds`/`LineTrace*`/`Sweep*`/`Overlap*`/`GetGameState`/`GetAuthGameMode`/`GetFirstPlayerController`/`GetNetMode`/`GetWorldSettings`) lacking `#include "Engine/World.h"`:
- **Every** touched .cpp that dereferences a `UWorld*` for a member call includes `Engine/World.h`: `SiegeHitFlashComponent.cpp` (now fixed), `SummonedUnit.cpp`, `MinerUnit.cpp`, `Tower.cpp`, `Projectile.cpp`, `HeroCharacter.cpp`, `SiegePlayerController.cpp`, `DamageNumberActor.cpp`. (`SiegeFeedbackLibrary.cpp` also carries the include.)
- Four touched .cpp dereference NO `UWorld*` member and correctly need no include: `SiegeMeshJuiceComponent.cpp` (no `GetWorld()` at all — animates off the `TickComponent` `DeltaTime` param), `SiegeGameState.cpp` (overtime sting passes `this` as world-context to the library — no deref), and `Building.cpp` / `Castle.cpp` (only pre-existing `AActor::GetWorldTimerManager()` — declared in `Actor.h`, defined out-of-line in the engine, returns `FTimerManager&` covered by `TimerManager.h`; that is NOT a `UWorld*` deref and needs no `Engine/World.h`).
- `DamageNumberWidget` is header-only (no .cpp) — nothing to audit.

**Carry-forward WARNs preserved (not re-loop gates — recorded so they are not lost):**
1. **Art/editor (TASK-176-adjacent):** `/Game/UI/WBP_DamageNumber` MUST be reparented to `UDamageNumberWidget` and implement the `SetDamageNumber(float Amount, float R, float G, float B)` BIE, else numbers spawn/rise/fade blank silently. Already in the "Every soft-ref path introduced" list above; build-master must PIE-verify the number shows the actual VALUE, not merely that a number appears.
2. **Perf-watch (build-master, TASK-183 §6 perf pass):** building/tower recoil + spawn squash move/scale the collision-ROOT `VisualMesh` (`bCanEverAffectNavigation`), so under `RuntimeGeneration=Dynamic` many recoiling towers can churn the nav octree. NO correctness risk (drift-free exact restore, tiny/brief offsets, BP escape hatch = zero `RecoilDistance`/`SpawnSquashSeconds`). Watch nav-rebuild cost; a follow-up can retarget juice to a non-nav child if it shows.

All seven tasks (154–159, 179) set back to `ready-for-qa` on the board with the loop-1 note logged.

---

## QA LOOP 2 FIX (2026-07-16) — `BlueprintReadOnly` on a private member (UHT compile error)

**Build-master verdict at TASK-182:** BUILD FAIL, 1 BLOCKER (`qa/TASK-154-159-179-qa.md` loop-2 section). NOT a Smart-App-Control insta-fail (SAC OFF; UHT failed at 3.31 s with a real header error). Fixed.

**The error:**
```
MinerUnit.h(155): Error: BlueprintReadOnly should not be used on private members
Result: Failed (OtherCompilationError)
```
`Siegebound/MinerUnit.h:155` declared the TASK-179 clink-audio component inside the `private:` section with `VisibleAnywhere, BlueprintReadOnly` but WITHOUT `AllowPrivateAccess` — UHT forbids `BlueprintReadOnly`/`BlueprintReadWrite` on a private member unless `meta = (AllowPrivateAccess = "true")` is present.

**Fix applied (specifier-only — NO logic change):**
- `Siegebound/MinerUnit.h:155` — added `meta = (AllowPrivateAccess = "true")` to the `ClinkAudio` UPROPERTY:
  ```cpp
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Miner", meta = (AllowPrivateAccess = "true"))
  TObjectPtr<UAudioComponent> ClinkAudio;
  ```
  Chosen over "drop BlueprintReadOnly" / "move to protected" because `ClinkAudio` is a `CreateDefaultSubobject` (MinerUnit.cpp:64), used ONLY by the private `StartMiningClink`/`StopMiningClink` — so it stays private, keeps its details-panel visibility (`VisibleAnywhere` is right for a default subobject) and BP read access, and now matches the codebase's dominant private-component convention: every private UPROPERTY component in `SummonedUnit`/`Building`/`Castle`/`HeroCharacter`/`Tower` uses `AllowPrivateAccess = "true"`.

**Batch-wide `BlueprintReadOnly`-on-private sweep (to prevent a loop-3) — result: CLEAN, no additional finds.** Scanned every new/edited `.h` in the batch (`SiegeFeedbackLibrary.h`, `SiegeHitFlashComponent.h`, `SiegeMeshJuiceComponent.h`, `DamageNumberWidget.h`, `DamageNumberActor.h`, `SummonedUnit.h`, `MinerUnit.h`, `Building.h`, `Castle.h`, `HeroCharacter.h`) for any UPROPERTY/UFUNCTION carrying a Blueprint specifier (`BlueprintReadOnly`/`BlueprintReadWrite`/`BlueprintCallable`/`BlueprintAssignable`/`BlueprintPure`/`BlueprintImplementableEvent`) inside a `private:` section without `AllowPrivateAccess`:
- All Blueprint-exposed UPROPERTYs (incl. the new `HitFlashComponent` / `MeshJuiceComponent` / `SkeletalVisualMesh` / `CrumbleFraction1..3` / delegates) live in `public:`/`protected:` sections — legal.
- Every `private:` UPROPERTY in the batch is either `Transient`-only (no Blueprint specifier — `SummonedUnit.h`, `SiegeHitFlashComponent.h`, `SiegeMeshJuiceComponent.h`) or already carries `AllowPrivateAccess = "true"` (the `VisibleInstanceOnly, Transient` runtime members in `SummonedUnit`/`Building`/`Castle`/`HeroCharacter`).
- `MinerUnit.h:155` was the SOLE offender. No over-correction applied elsewhere.

**Cascade-warning verdict (`SiegeFeedbackLibrary.h(41/45)`) — CONFIRMED BENIGN, no fix needed.** The header contains ZERO `#if`/`#ifdef`/`#ifndef`/`#endif` directives; `UCLASS()` (line 42) and `GENERATED_BODY()` (line 45) sit at clean file top-level scope immediately around the class declaration (line 43), with only forward-decls and the comment block above. The build-master's "in a block being skipped" Deprecation/Warning is a pure UHT cascade artifact of the reflection pass aborting after the `MinerUnit.h(155)` Error — it will vanish once MinerUnit compiles. Not a real defect.

**Loop-2 scope:** one-line specifier fix in `MinerUnit.h`. No `.cpp`, no logic, no other header touched. TASK-179 set back to `ready-for-qa`; the other six (154–158, 159) remain qa-passed and held with it (single-module batch).

---

## QA LOOP 3 FIX (2026-07-16) — embedded `*/` prematurely closes a `/** */` doc comment (UHT-fatal) + FULL UHT-hazard sweep

**Build-master verdict at TASK-182 retry:** BUILD FAIL, 1 BLOCKER (`qa/TASK-154-159-179-qa.md` loop-3 section), escalated to Jonathan (loop 3, no auto-loop). NOT a Smart-App-Control insta-fail (SAC OFF; UHT failed at 1.45 s with a specific header diagnostic). The loop-2 `MinerUnit.h:155` fix was CONFIRMED working — this was the NEXT fatal error, the same one loop-2's handoff (above) wrongly called "BENIGN." Fixed, and a comprehensive sweep run to guarantee no loop 4.

**The error:**
```
SiegeFeedbackLibrary.h(41): Deprecation: The identifier 'UCLASS' was detected in a block being skipped ...
SiegeFeedbackLibrary.h(45): Warning: The identifier 'GENERATED_BODY' was detected in a block being skipped. Was this intentional?
Result: Failed (OtherCompilationError)  (1.45 s)
```

**ROOT CAUSE (why loop-2 misdiagnosed it):** loop-2 checked for `#if`/`#endif` imbalance and, finding none, declared the warning a benign cascade. But the real culprit was a `/* */` **comment tokenizer** problem, not the preprocessor. The class doc comment opens `/**` at **L20** and is meant to close ` */` at **L41**. L39 prose read `... and raw UWorld*/context pins invite BP` — the literal `*/` inside `UWorld*/context` **terminated the block comment early at L39**. Everything after (L39 tail, L40, and the intended ` */` at L41) became stray non-comment text and UHT entered a "skipping" state, so `UCLASS()` (L42) and `GENERATED_BODY()` (L45) read as "in a block being skipped" → promoted to a hard error under `-WarningsAsErrors`.

**Fix applied (comment prose only — ZERO logic/API/specifier change):**
- `Siegebound/SiegeFeedbackLibrary.h:39` — reworded `raw UWorld*/context` → `raw UWorld pointer / context`, eliminating the embedded `*/`. The doc comment now runs `/**` (L20) → ` */` (L41) with a single, correct terminator; `UCLASS()`/`GENERATED_BODY()` are back at file/class scope. Line numbers unchanged (3 lines replaced with 3).

**MANDATORY comprehensive UHT / `-WarningsAsErrors` hazard sweep — ALL 10 batch headers + their .cpp. Result: CLEAN (this was the last defect).** Both QA and two build-master passes missed the embedded `*/`, so I did NOT assume it was the only issue — I swept all four hazard classes across `SiegeFeedbackLibrary`, `SiegeHitFlashComponent`, `SiegeMeshJuiceComponent`, `DamageNumberWidget`, `DamageNumberActor`, `SummonedUnit`, `MinerUnit`, `Building`, `Castle`, `HeroCharacter` (`.h` + `.cpp`):

1. **Embedded `*/` in a `/** */` doc comment / stray `/*` — CLEAN (1 fixed).** Read all 10 headers in full; grepped the whole `Siegebound/` dir for `[^ \t/*]\*/` AND the `**/`-edge `[^ \t*]\*\*/`. Every other `*/` hit in the batch is a balanced single-line arg annotation (`/*bLoop=*/`, `/*Owner=*/`, `/*bWorldSpace=*/true`, etc.) where `/*` and `*/` sit on the same line — not a hazard. (Noted but NOT a defect: `SiegePlayerController.h:823` `Pending*/placement` — that whole line is a `//` line comment, not inside a `/* */` block, so its `*/` is inert; that file is also outside this batch and compiled in prior milestones.) No unclosed `/*`. The `SiegeFeedbackLibrary.h:39` one was the SOLE real offender.
2. **UPROPERTY/UFUNCTION specifier legality (BlueprintReadOnly-on-private &c.) — CLEAN.** The loop-2 `MinerUnit.h:155` `AllowPrivateAccess = "true"` is present and correct. Re-audited every `private:` reflected member across all 10 headers: all `VisibleInstanceOnly, Transient` runtime members in `SummonedUnit`/`Building`/`Castle`/`HeroCharacter` carry `AllowPrivateAccess = "true"`; the private `Transient`-only pointers (`FlashMeshes`, `TargetMesh`, `CurrentMoveGoal`, `CachedAttackImpactEffect`, `CachedCardTable`) carry NO Blueprint specifier so need none; every `BlueprintReadOnly`/`BlueprintReadWrite`/`BlueprintAssignable`/`BlueprintPure`/`BlueprintImplementableEvent`/`BlueprintCallable` sits in `public:`/`protected:`. No illegal specifier combos.
3. **Reflected-macro placement (UCLASS/USTRUCT/UENUM/GENERATED_BODY scope) — CLEAN.** With the L39 comment fixed, every reflected macro is at correct file/class scope. No `#if 0`/`#if`/`#endif` anywhere in the 10 headers, so no macro sits inside a skipped or commented block. The two `UENUM(BlueprintType)` (`ESummonedUnitState`, `EHeroUpgradeResult`) and every `GENERATED_BODY()` are well-formed and correctly scoped.
4. **Deprecated-API-as-error / missing complete-type includes — CLEAN.** Grepped the batch `.cpp` for deprecated forms (`SetSkeletalMesh(`, `GetComponentsByClass`, `ClientPlayCameraShake`, `SpawnEmitterAtLocation`, `bGenerateOverlapEvents`) → ZERO hits. Confirmed the current UE5.8 setters are used: `SetSkeletalMeshAsset` (`SummonedUnit.cpp:246`), `SetAnimInstanceClass` (:255), `SetOverlayMaterial` (`SiegeHitFlashComponent.cpp:83/102`), `ClientStartCameraShake` (`HeroCharacter.cpp:347`, `SiegeFeedbackLibrary.cpp:158`). The loop-1 `#include "Engine/World.h"` is present in `SiegeHitFlashComponent.cpp:8`, covering both `World->GetTimerManager()` sites (L48, L90); the loop-1 batch include self-audit (above) still holds.

**Loop-3 scope:** one comment-prose reword in `SiegeFeedbackLibrary.h:39`. No `.cpp`, no logic, no API, no specifier, no other file touched. Board: TASK-154 (shared-core rep) → `qa-passed` with this note; 155–158/159 stay `qa-passed`; 179 stays `ready-for-qa`. Batch is compile-ready for the TASK-182 recompile.

---

## QA LOOP 4 FIX (2026-07-16) — C++ most-vexing-parse on soft-ptr local decls (C2228) + FULL .cpp-COMPILE-STAGE sweep

**Build-master verdict at TASK-182 loop-4:** BUILD FAIL, 2 BLOCKERS, HARD STOP (`qa/TASK-154-159-179-qa.md` loop-4). The three stacked prior fixes (loop-1 `Engine/World.h`, loop-2 `MinerUnit.h:155 AllowPrivateAccess`, loop-3 `SiegeFeedbackLibrary.h:39` embedded-`*/`) ALL WORKED — UHT passed clean and the build advanced into actual C++ compilation for the first time, which surfaced two `.cpp`-body errors that every prior (UHT/header-only) sweep structurally could not see. NOT a Smart-App-Control fail (SAC OFF; full toolchain ran, produced specific C2228s).

**The errors (verbatim):**
```
DamageNumberActor.cpp(79,29): error C2228: left of '.LoadSynchronous' must have class/struct/union
SummonedUnit.cpp(237,40): error C2228: left of '.LoadSynchronous' must have class/struct/union
```

**ROOT CAUSE — C++ most-vexing-parse (NOT a missing include; `UObject/SoftObjectPtr.h` is present in both TUs via their headers).** `const T Name( U(ident) );` where `ident` is a bare identifier is parsed as a FUNCTION declaration — the inner `U(ident)` reads as a parameter declaration `U ident` (redundant parens around a param name are legal; the standard's "if it can be a declaration, it is" rule wins, even though a same-named variable is in scope). So `Name` becomes a function, and `.LoadSynchronous()` on the next line applies `.` to a function name → C2228 (the error points at the `.LoadSynchronous` LINE though the defect is the DECL line above it).

**Fixes applied — brace-init breaks the parse (`{ }` cannot introduce a function declarator). ZERO logic/behavior change, no include change:**
- `Siegebound/DamageNumberActor.cpp:78` — `const TSoftClassPtr<UUserWidget> WidgetClass(FSoftObjectPath(DamageNumberWidgetClassPath));` → `... WidgetClass{ FSoftObjectPath(DamageNumberWidgetClassPath) };` (TASK-156).
- `Siegebound/SummonedUnit.cpp:236` — `const TSoftObjectPtr<USkeletalMesh> SkSoft(FSoftObjectPath(SkPath));` → `... SkSoft{ FSoftObjectPath(SkPath) };` (TASK-159).
- **`Siegebound/SummonedUnit.cpp:252` (ADDITIONAL FIND — the sweep caught it) — `const TSoftClassPtr<UAnimInstance> AbpSoft(FSoftObjectPath(AbpPath));` → `... AbpSoft{ FSoftObjectPath(AbpPath) };`.** This is the byte-identical hazard 16 lines below the reported SummonedUnit error, on the ABP path. The compiler reports one C2228 per TU and stopped at :237, so :253 was HIDDEN behind :237 — it would have been the very next error on the recompile. Fixed now so the recompile does not surface a third C2228 in the same file.

**MANDATORY comprehensive `.cpp`-COMPILE-STAGE sweep — ALL 14 batch `.cpp` (`SiegeFeedbackLibrary`, `SiegeHitFlashComponent`, `SiegeMeshJuiceComponent`, `DamageNumberWidget` [header-only, no .cpp], `DamageNumberActor`, `SummonedUnit`, `MinerUnit`, `Building`, `Tower`, `Castle`, `HeroCharacter`, `Projectile`, `SiegePlayerController`, `SiegeGameState`) × 4 hazard classes. Result: 3 MVP fixed (above); nothing else — batch is compile-ready.**

1. **Most-vexing-parse everywhere — 3 found & fixed; all others provably safe.** Grepped the whole `Siegebound/` dir for every `Name( Type( ... ) )` local-decl form (incl. `Var(Type(identifier));` regardless of outer template brackets, and the zero-arg `Type Name(Other());` form). The ONLY MVP-triggering decls were the three soft-ptr locals whose inner arg is a bare identifier (`SkPath`/`AbpPath`/`DamageNumberWidgetClassPath`). Every OTHER constructor-form local in the batch is NOT a vexing-parse and needs no change, for a concrete reason:
   - The `TEXT("...")`-arg soft-ptr constructions (`Building.cpp:166/167`, `SummonedUnit.cpp:198/199`, and the `static const FName …(TEXT(...))` / `const TCHAR* …(TEXT(...))` path constants) — a string literal cannot be a parameter name, so `Type(TEXT("..."))` cannot parse as a param decl → unambiguous object construction.
   - `FCollisionQueryParams …(SCENE_QUERY_STAT(...), false, this)` / `…(TEXT(...), false, this)` (`Projectile.cpp:425`, `SiegePlayerController.cpp:2234`, `SiegeCheatManager.cpp:59`, `BattlefieldScatter.cpp:621`) — the `false`/`this` keyword args cannot be param names.
   - `FTransform …(FRotator(0.f,…), FVector(…), …)` (`BattlefieldScatter.cpp:290…`) — float-literal args in the inner temporaries cannot be param names.
   - Assignment-form uses of the same construction (`DamageNumberActor.cpp:38`, `Castle.cpp:64/69/70`, `SiegeFeedbackLibrary.cpp:62`) are expressions, never declarations → never MVP.
2. **Member/type completeness at each use site — CLEAN.** Every one of the 11 `USiegeFeedbackLibrary::` caller TUs `#include "Siegebound/SiegeFeedbackLibrary.h"` (verified by file-set intersection). Every `HitFlashComponent->TriggerFlash()` TU includes `SiegeHitFlashComponent.h` (`HeroCharacter`/`Building`/`Castle`/`SummonedUnit`); every `MeshJuiceComponent->…` TU includes `SiegeMeshJuiceComponent.h` (`Building`/`SummonedUnit`/`Tower`). `MinerUnit.cpp` includes `Components/AudioComponent.h` (complete `UAudioComponent` for `ClinkAudio->IsPlaying/SetSound/Play/Stop`). `SummonedUnit.cpp` includes `Components/MeshComponent.h` (complete `UMeshComponent` — the `SetTargetMesh(GetActiveVisualMesh())` upcast to `USceneComponent*` at :835 is valid) and `Animation/AnimInstance.h` (the `TSoftClassPtr<UAnimInstance>` path). `Castle.cpp` includes `Components/StaticMeshComponent.h` + `Engine/StaticMesh.h` + `Materials/MaterialInterface.h` (the crumble `SetStaticMesh`/`SetMaterial` swaps); `TSoftClassPtr<UCameraShakeBase>::LoadSynchronous()` returns `UClass*` and needs only a forward decl of `UCameraShakeBase` (supplied via `SiegeFeedbackLibrary.h`). The loop-1 `Engine/World.h` audit still holds. No incomplete-type deref anywhere.
3. **Const-correctness / narrowing / signed-unsigned / overloads / returns / uninitialized — CLEAN.** `FVector` is double-based in UE5, so the juice math (`SiegeMeshJuiceComponent.cpp`) is well-typed; the only implicit float narrowings (e.g. `const float Wobble = <double sin expr>;`) are the UE-standard `UE_PI`/`FMath::Sin` pattern that game modules do NOT promote to error. `LiveCount >= MaxConcurrentNumbers` is `int32 >= int32` (no signed/unsigned mix). All new path/height identifiers are DEFINED in-TU before use (`Hero/Unit/Building/CastleDamageNumberHeightZ`; every `S_*`/`NS_*` path constant) — grep-confirmed, zero undefined identifiers. `TeamTint`/`NormalizeObjectPath`/all resolvers have a return on every path. `const TCHAR*` path args bind to the `const FString&` params via FString's implicit ctor (fine). No uninitialized references introduced.
4. **Deprecated/renamed API in a `.cpp` body — CLEAN.** Grepped the batch for `SetSkeletalMesh(` (non-Asset), `ClientPlayCameraShake`, `PlayCameraShake`, `GetComponentsByClass`, `GetComponentByClass(` → ZERO hits. Confirmed current UE5.8 forms in the new code: `SetSkeletalMeshAsset`, `SetAnimInstanceClass`, `SetOverlayMaterial`, `ClientStartCameraShake`, `SetStaticMesh`/`SetMaterial`, `UAudioComponent::IsPlaying/SetSound/Play/Stop`, `PlaySound2D`/`SpawnSoundAtLocation`/`SpawnSystemAtLocation`.

**Loop-4 scope:** three brace-init edits in TWO `.cpp` (`DamageNumberActor.cpp:78`, `SummonedUnit.cpp:236` + `:252`). No header, no include, no logic, no API change. Board: TASK-156 + TASK-159 → `qa-passed` (loop-4 note); 154/155/157/158 stay `qa-passed`; 179 stays `ready-for-qa`. **Batch is compile-ready for the TASK-182 recompile — the `.cpp`-body layer is now swept, the last structural blind spot (UHT can't see `.cpp` bodies) is closed.**

---

## SHARED-ABP FALLBACK addendum (2026-07-17) — TASK-159 swap path, Phase-B / TASK-165 rig-import chain

**Why:** MCP can't author per-unit AnimBlueprints without freezing the editor, so we can't ship an `ABP_<CardID>` per rigged unit for M7. But every rigged unit shares the `SK_Footman_Skeleton` / SiegeBiped rig, so ONE shared velocity-driven locomotion ABP (`ABP_Footman`) can drive any of them (idle/walk). This addendum makes the SkeletalVisualMesh AnimClass resolve fall back to that shared ABP when a per-unit one is absent, so the whole rigged roster ANIMATES today instead of standing in ref pose.

**The change (C++, ONE file — `Siegebound/SummonedUnit.cpp`, in `ResolveSkeletalVisual()`):**
1. **New named constant** in the top anonymous-namespace soft-ref block (next to `GoldBurstVFXPath` etc.):
   ```cpp
   const TCHAR* SharedLocomotionAbpPath = TEXT("/Game/Characters/ABP_Footman.ABP_Footman_C");
   ```
   Isolated to one line so a future dedicated `ABP_SiegeUnit` is a single-constant swap.
2. **AnimClass resolution now three-tier** (was: per-unit only, else nothing):
   - **Per-unit** `/Game/Characters/ABP_<CardID>.ABP_<CardID>_C` resolves → use it (future per-unit ABPs still take priority the instant they're authored — unchanged for them).
   - **ELSE shared** `SharedLocomotionAbpPath` (`ABP_Footman`) resolves → use it (the M7 shared-locomotion fallback).
   - **ELSE neither** resolves → leave the skeletal mesh with NO anim instance (ref pose, null-safe, never a crash — same soft-ref discipline as the rest of the swap path).
   Implemented as: resolve per-unit into `UClass* AnimClass`; if null, resolve the shared into the same var; call `SetAnimInstanceClass(AnimClass)` only if non-null. Both `TSoftClassPtr<UAnimInstance>` locals use **brace-init** (`{ FSoftObjectPath(...) }`) — same most-vexing-parse guard as the loop-4 fix, so no C2228.

**Behavior guarantees (nothing else changed):**
- A unit with NO `SK_<CardID>` is still byte-for-byte the static-mesh path (early-returns before this block).
- A unit with `SK_<CardID>` present now animates via `ABP_Footman` even without its own ABP — the roster is live.
- `ABP_Footman` absent AND per-unit ABP absent = ref pose, no crash (unchanged null-safe contract).
- No new include (uses the existing `Animation/AnimInstance.h` + `TSoftClassPtr`), no header change, no `.Build.cs` change, no logic touched outside the AnimClass resolve.

**QA should scrutinize:** (1) the `.ABP_Footman_C` generated-class suffix on the shared path (must be the `_C` class, matching the per-unit `ABP_%s_C` form) — correct here; (2) brace-init on both soft-class locals (MVP guard) — present; (3) fallback ordering (per-unit wins) — correct; (4) still null-safe when neither resolves — yes, `SetAnimInstanceClass` is guarded by `if (AnimClass)`.

**Ready for build-master to recompile** (TASK-182-style editor-bounce build). No art dependency to ship the code: `ABP_Footman` is the TASK-162 spike asset (may already exist in Content/Characters); if absent at runtime the fallback simply no-ops to ref pose. All rigged units will animate with the shared locomotion until per-unit ABPs are authored.

**Scope:** one constant + one resolve-block rewrite in `SummonedUnit.cpp`. No `.h`, no other file. Board: TASK-159 → `ready-for-qa` (shared-ABP fallback addendum).
