# QA Report — TASK-002 (Castle actor, C++)

Verdict: PASS
Blockers: 0 | Warnings: 1 | Nits: 3

- reviewer: qa-reviewer
- date: 2026-07-02
- reviewed: `Source/GitClaudeUnrealTest/Siegebound/Castle.h`, `Source/GitClaudeUnrealTest/Siegebound/Castle.cpp`
- against: TASKBOARD TASK-002 spec + names block, CONVENTIONS.md, handoffs/TASK-002.md, Docs/GDD.md §3.0/§3.9, Siegebound/TeamId.h (TASK-001, qa-passed)

## Findings

- [WARN] Castle.cpp:17-18 — Collision profile is never set explicitly; the root `CastleMesh` inherits `UStaticMeshComponent`'s engine default (`BlockAllDynamic`), while the handoff (handoffs/TASK-002.md, "Properties") documents "default collision profile BlockAll". Behaviorally the castle blocks pawns and affects nav today, but TASK-004's targeting/blocking contract rests on an undocumented engine default, and the handoff over-promises. Suggested fix (next touch of this file, not worth a QA loop): `CastleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);` in the constructor (include `Engine/CollisionProfile.h`), or correct the handoff wording.
- [NIT] Castle.h:126 — `CurrentHP = 2000.0f` duplicates MaxHP's default as a literal. A per-instance MaxHP edit shows a stale 2000 in the details panel until BeginPlay seeds it (Castle.cpp:39). Suggested: initialize `CurrentHP = MaxHP` in the constructor. Cosmetic only — gameplay always passes through BeginPlay.
- [NIT] Castle.cpp:52,61 — `LoadSynchronous()` re-attempts a disk load on every OnConstruction run while the assets are missing (editor property edits/drags run construction repeatedly). Programmer flagged this in the handoff; ACCEPTED for two castle actors and blockout-tier assets. Revisit only if castles become numerous.
- [NIT] Castle.cpp:64 — `SetMaterial(0, ...)` is called on every construction run without the diff the mesh gets at cpp:54. Harmless (`UMeshComponent::SetMaterial` early-outs on unchanged material); noted for symmetry only. No action required.

## Rulings on handoff-flagged decisions

1. **Skipping `Super::TakeDamage` on friendly hits (Castle.cpp:76-81) — APPROVED.** `AActor::TakeDamage`'s observable side effects are the `OnTakeAnyDamage`/point-damage broadcasts; firing those for same-team hits would leak damage events the spec says must be "fully ignored" (§3.0). Constraint recorded: no listener (BP or C++) bound to this actor's damage delegates will ever observe friendly hits — that is the intended contract, do not "fix" it later without a spec change.
2. **Native `Cast<ITeamAgent>` for team resolution (Castle.cpp:134,142,151) — APPROVED with recorded constraint.** `Cast<>` to an interface resolves only for classes implementing it in C++. Valid here because `UTeamAgent` is `NotBlueprintable` (TeamId.h:25), so every implementer is native; Blueprint subclasses of native implementers (e.g. future BP_HeroCharacter) also cast fine via their native parent. CONSTRAINT: if any future task makes UTeamAgent Blueprintable and something implements it in BP, these casts silently return nullptr and that attacker's damage would apply as "teamless" — any such change must migrate all `Cast<ITeamAgent>` sites project-wide to `Implements<UTeamAgent>()` + `Execute_` dispatch.

## Verified (no issues)

- **UE 5.8 API correctness:** `TakeDamage` override signature matches `AActor` exactly (Castle.h:58); `#include "Engine/DamageEvents.h"` present (Castle.cpp:6 — required, UE5 moved FDamageEvent out of Actor.h); no deprecated APIs (`SetActorHiddenInGame`, `SetActorEnableCollision`, `LoadSynchronous`, `CreateDefaultSubobject` all current).
- **Generated header / macros:** `Castle.generated.h` is the last include (Castle.h:9); `GITCLAUDEUNREALTEST_API` matches the module; `GENERATED_BODY()` present; interface inherited publicly.
- **Delegate declaration:** `DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCastleDestroyed, ACastle*, DestroyedCastle, ETeamId, CastleTeam)` — declared before the UCLASS with `class ACastle;` forward-declared (Castle.h:11,22); UCLASS-pointer + UENUM params are UHT-legal; name matches the spec's `FOnCastleDestroyed` exactly; `BlueprintAssignable` UPROPERTY correct. GameMode (TASK-006) must bind with a `UFUNCTION()` handler via `AddDynamic` (documented in handoff).
- **Single-fire guarantee:** `bDestroyed` checked at TakeDamage entry (cpp:71) AND in `HandleDestroyed` (cpp:107), set BEFORE the broadcast (cpp:110-117) so re-entrancy during the callback cannot re-fire; re-armed only by `ResetCastle` (cpp:123). Acceptance math: 167 Footman hits x 12 = 2004 damage → clamp to 0 (cpp:92), event fires exactly once.
- **Friendly-fire chain null-safety:** `EventInstigator` null-checked before `GetPawn()` (cpp:132); `Cast<>` tolerates a null pawn (cpp:134); `DamageCauser` goes through null-safe `Cast<>` (cpp:142) and is null-checked before `GetInstigator()` (cpp:149). Failed casts FALL THROUGH to the next resolver rather than returning "no team" — no misattribution path found. Unresolvable team (world damage) applies at 100%, per handoff contract. `InstigatorTeam` is initialized and only consumed when resolution succeeds (cpp:77-78).
- **ResetCastle completeness:** restores `CurrentHP = MaxHP` (not a literal), visibility, collision, and re-arms the event (cpp:120-127). A destroyed castle returns to 2000/2000 and visible.
- **Spec conformance:** MaxHP default 2000 with ClampMin 1 (Castle.h:93-94); at 0 HP hides + disables collision BEFORE broadcasting (cpp:112-117); castle has no attack and no tick (cpp:15); melee 100% with marked `TODO(M2)` for projectile 50% / Siege 200% (Castle.h:55-56, cpp:89-91); `EditAnywhere Team` for per-instance Castle_Red setup (Castle.h:89-90).
- **Names block / conventions:** class `ACastle`, files `Siegebound/Castle.h/.cpp`, delegate `FOnCastleDestroyed` — exact. Soft paths `/Game/Meshes/SM_Castle.SM_Castle`, `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue`, `/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red` (cpp:23-25) are the correct full object-path form of the spec'd asset paths. `TObjectPtr` used for the component (Castle.h:86); include style `Siegebound/...` resolves via the module-root PublicIncludePath.
- **OnConstruction safety:** never runs on the CDO; `LoadSynchronous()` returns nullptr for unset/missing assets and is skipped; `CastleMesh` null-checked (cpp:45); material only applied to slot 0 per the single-slot SM_Castle contract (TASK-013). No crash path with missing assets, in editor or PIE.

## Notes for build-master

- Compile requires TASK-001's `TeamId.h` in the same module (blocked-by satisfied: TASK-001 is qa-passed). No new module dependencies; Build.cs untouched — correct.
- Integration per board: place `Castle_Blue` (Team = Blue, the default) at CastleAnchor_Blue and `Castle_Red` (set Team = Red per instance) at CastleAnchor_Red in L_Arena.
- SM_Castle / MI_TeamColor_* may not be imported yet at compile time — by design the castle just renders empty until TASK-013 lands; expect at most one-time asset-load misses in the log, not errors.
- WARN above (explicit collision profile) does not block integration; fold the one-line fix into the next Castle.cpp touch (e.g. if TASK-004/006 QA sends anything back).
