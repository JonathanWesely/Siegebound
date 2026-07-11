# QA Report — TASK-130 (health-bar REBUILD, C++ push/delegate)
Verdict: **PASS** (0 BLOCKER · 0 WARN · 3 NIT) — **DATA-LAYER ONLY; see "The limit of this review" — a PASS here does NOT mean the feature works on screen.**

Pre-compile review of the from-scratch rebuild onto the castle's PUSH/delegate model. Scope reviewed:
the 5 new files (`HealthBarProvider.h`, `CombatantHealthBarWidget.h/.cpp`,
`CombatantHealthBarComponent.h/.cpp`) and the 7 modified files (`SummonedUnit.h/.cpp`,
`Building.h/.cpp`, `HeroCharacter.h/.cpp`, `SiegeCheatManager.cpp`). Cross-read (read-only): `Castle.h/.cpp`
+ `CastleHealthBarWidget.h` (the template — parity checks), `CONVENTIONS.md` §"Overhead combatant health
bars — REBUILT" (l.158-167), the TASK-130 board spec (l.698-752), `SpellLibrary.cpp` (HP-write scan).
No Git/Bash access by design — the deleted-file and reference checks are done via filesystem glob + a
whole-tree `Source/` grep, stated where it matters.

---

## THE thing to verify — broadcast completeness: VERIFIED 1:1, NO MISSES (independently, not on faith)

I grepped **every** `CurrentHP` assignment across all of `Source/GitClaudeUnrealTest/Siegebound` (not just
the 3 files) and matched each to a following `OnHPChanged.Broadcast(CurrentHP, GetMaxHP())`. Every actor HP
mutation broadcasts; the only non-broadcasting `CurrentHP =` hits are provably-correct exclusions.

**ASummonedUnit.cpp (4/4):**
- `:677` spawn-init (`CurrentHP = MaxHP`) → broadcast `:681`
- `:1160` `ApplyHealing` (Cleric heal, `Min(CurrentHP+Amount, MaxHP)`) → `:1161`
- `:1596` `TakeDamage` (`Max(CurrentHP-dmg,0)`) → broadcast `:1600` **BEFORE** the `<=0` death check `:1602`→`HandleDeath()` `:1604` ✓ castle parity
- `:1699` `HandleDeath` (`CurrentHP = 0`, covers non-damage deaths e.g. Sapper suicide) → `:1704`, then `Destroy()`

**ABuilding.cpp (3/3):**
- `:226` spawn-init → `:229`
- `:299` `TakeDamage` (`Max(CurrentHP-scaled,0)`) → broadcast `:303` **BEFORE** the `<=0` check `:305`→`HandleDestroyed()` `:307` ✓
- `:323` `HandleDestroyed` (`CurrentHP = 0`) → `:327`, then `Destroy()`

**AHeroCharacter.cpp (7/7):**
- `:78` BeginPlay init (`CurrentHP = GetEffectiveMaxHP()`) → `:81`
- `:101` Tick out-of-combat regen (outer guard `!bDead && CurrentHP < EffectiveMaxHP` + regen-delay; `Min(..,EffectiveMaxHP)` converges to exactly max then the guard stops it) → `:104`
- `:413` `TakeDamage` (`Clamp(CurrentHP-dmg, 0, EffectiveMaxHP)`) → broadcast `:417` **BEFORE** the `<=0` check `:425`→`HandleDeath()` `:427` ✓
- `:487` `HandleDeath` (`CurrentHP = 0`) → `:492`, then `HideBar()`
- `:529` `ResetHero` respawn (`GetEffectiveMaxHP()`) → `:533`, then `ShowBarIfEnabled()`
- `:617` `ApplyUpgrade` Plate Armor (raises current + the effective-max denominator) → `:620`
- `:650` `ResetUpgrades` (Play Again — effective-max shrinks, current clamped down) → `:653`

**Denominator (MaxHP / effective-max) completeness — also verified** (a stale denominator freezes the
percent just like a missed current-HP broadcast):
- Units/buildings: `MaxHP` is written exactly once (`SummonedUnit.cpp:676`, `Building.cpp:225`), at
  spawn-init, immediately before the broadcasting `CurrentHP = MaxHP`. It never changes again. ✓
- Hero: the effective max = `MaxHP + MaxHPBonus*PlateArmorStacks` (`HeroCharacter.h:294`); it changes ONLY
  when `PlateArmorStacks` changes — written at `:612` (ApplyUpgrade, broadcast `:620`) and `:643`
  (ResetUpgrades, broadcast `:653`). Both denominator-moving paths broadcast. ✓ Crucially,
  `AHeroCharacter::GetMaxHP()` returns `GetEffectiveMaxHP()` (`HeroCharacter.h:229`), so every hero
  broadcast carries the EFFECTIVE denominator (a Plate-Armored hero at 250/300 broadcasts 300, not 200).

**Non-broadcasting `CurrentHP =` hits — all correctly excluded:**
- `Castle.cpp:64/177/234/335` — ACastle's OWN `CurrentHP`, driven by its separate `FOnCastleHPChanged`
  push system. Not a IHealthBarProvider actor; out of scope; untouched.
- `SpellLibrary.cpp:216/241/248/256/264` — LOCAL float temporaries (`TargetCurrentHP`,
  `CandidateCurrentHP = Unit->GetCurrentHP()`) used for lowest-HP target selection. These are READS, not
  actor-HP writes. Spells apply HP only through `TakeDamage`/`ApplyHealing`, which are on the broadcasting
  paths above. No spell writes `CurrentHP` directly. ✓
- `*.h:231/187/466/691` — member default initializers, not runtime writes.

**Seed-then-bind closes the init-order gap:** `InitForCombatant` seeds `OnHPChanged(current,max)` THEN binds
(`CombatantHealthBarWidget.cpp:30,34`). Whether the actor's stats-load broadcast fires before the component
binds (lost, but the seed then reads the correct value) or after (caught by the live binding), the bar ends
correct. Hero comment `:79-80` documents this reconciliation explicitly (component seeds during
`Super::BeginPlay`, hero body then broadcasts the effective max at `:81`). Sound.

**Verdict on the mandate: no missing broadcast. The push plumbing is complete.**

---

## Delegate binding soundness — CONFIRMED
- `AddUniqueDynamic(this, &UCombatantHealthBarWidget::HandleHPChanged)` (`.cpp:34`) — no double-bind on a repeated `InitForCombatant`.
- `HandleHPChanged` is a `UFUNCTION()` (`.h:67`) — required for dynamic binding; it forwards to the BIE `OnHPChanged`.
- Seed BEFORE bind (`.cpp:30` then `:34`) — the qa/TASK-005 major-2 trap is avoided (a bind-only bar at a value that never changes again would stay stale).
- Re-target unbinds the previous provider (`RemoveDynamic`, `.cpp:19-25`) — the bar never receives two streams. (Not exercised in practice — one component per actor — but correct.)
- **No dangling binding on teardown:** dynamic-multicast delegates hold a weak object ref + function name; a destroyed widget is skipped on Broadcast, and the provider (actor) OWNS the delegate so it is destroyed with the actor — no broadcast to a dead listener, no crash. No explicit EndPlay unbind is needed (castle parity). Safe.

## Team-color mapping — CORRECT, not inverted
`CombatantHealthBarComponent.cpp:89`: `(TeamAgent && GetTeamId()==ETeamId::Red) ? RedBarColor : BlueBarColor`,
with `RedBarColor=(1.00,0.10,0.05)` (red) and `BlueBarColor=(0.05,0.30,1.00)` (blue). **Red team → red fill,
Blue team → blue fill; a null/absent TeamAgent defaults to Blue (friendly).** RED=enemy, BLUE=friendly, both
teams. Matches CONVENTIONS "Colors are LAW" (l.165). Pushed ONCE at BeginPlay.

## Hero hide/show on death/respawn — CONFIRMED (no bad path)
- `HandleDeath` (`:479-519`): broadcasts 0 (`:492`) → `if (HPBarWidget) HPBarWidget->HideBar()` (`:493-496`). A dead hero's screen-space bar is explicitly hidden (it does not follow `SetActorHiddenInGame`).
- `ResetHero` (`:521+`): broadcasts full (`:533`) → `if (HPBarWidget) HPBarWidget->ShowBarIfEnabled()` (`:534-537`). Respawn re-shows (iff opted-in).
- `FellOutOfWorld` (`:433-456`) routes through `HandleDeath` (`:455`) — the KillZ path also hides the bar.
- `HandleDeath` is the single `bDead`-guarded death side-effect sink; no other path hides the hero without it. No path leaves a dead hero's bar visible or a respawned hero's bar hidden. Both `HPBarWidget->` calls are null-guarded.

## Zero dangling references to the 5 deleted types — CONFIRMED
- The 5 files are GONE from disk (glob `{HealthBarComponent,HealthBarTarget,UnitHealthBarWidget}.*` → **no files found**). No orphaned `.cpp` for UBT to compile against a deleted header.
- Whole-`Source/` grep for `IHealthBarTarget | UHealthBarComponent | UUnitHealthBarWidget | HealthBarTarget.h | UnitHealthBarWidget.h | "…/HealthBarComponent.h"` returns ONLY: (a) false-positive substring matches inside the NEW `CombatantHealthBarComponent.h` includes, and (b) one historical COMMENT (`HealthBarProvider.h:26` "replaces the retired IHealthBarTarget"). No live include, forward-decl, `Cast<>`, or `TSubclassOf` of a deleted type anywhere.
- `SiegeCheatManager.cpp` — the one external consumer — swapped `IHealthBarTarget`→`IHealthBarProvider` (include `:14`, `Cast<IHealthBarProvider>` `:32`, `IsHealthBarActorAlive()` `:34`). The programmer's "only external consumer" claim is verified complete. No `.Build.cs` reference exists (module Build.cs lists no individual files).

## Mandatory scans
- **Inherited-reflected-member shadow scan: CLEAN.** New members: `OnHPChanged` (`FOnCombatantHPChanged`, `UPROPERTY(BlueprintAssignable)` on all 3 — `SummonedUnit.h:117`, `Building.h:72`, `HeroCharacter.h:124`) — no base of `ASummonedUnit`(ACharacter)/`ABuilding`(AActor)/`AHeroCharacter`(AGitClaudeUnrealTestCharacter) declares `OnHPChanged`; the hero's other delegates are `OnHeroDied`/`OnRallyStateChanged`/`OnHeroUpgradesChanged` (distinct). ACastle's own delegate is on an UNRELATED class — not a shadow. Component members (`HealthBarWidgetClass`/`bShowHealthBar`/`BarHeightZ`/`BlueBarColor`/`RedBarColor`) collide with no `UWidgetComponent` reflected member; widget member `ObservedProvider` is unique. New locals (`LoadedWidgetClass`, `Bar`, `OwnerActor`, `Provider`, `ProviderInterface`, `TeamAgent`, `BarColor`, `ProviderPtr`, `OldProvider`) shadow nothing; none named `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`.
- **Complete-type-include scan: CLEAN.** `CombatantHealthBarComponent.cpp` includes `Blueprint/UserWidget.h` (GetWidget/SetWidgetClass), `Siegebound/CombatantHealthBarWidget.h` (`Cast<>` + `InitForCombatant`/`SetTeamColor`), `Siegebound/HealthBarProvider.h` (`Cast<IHealthBarProvider>`), `Siegebound/TeamId.h` (`ITeamAgent`/`ETeamId`), **`GameFramework/Actor.h`** (the `TScriptInterface::SetObject(AActor*)` upcast + `Cast<>(OwnerActor)` need the complete `AActor`), `Engine/CollisionProfile.h` (`ECollisionEnabled`). **Regression guard verified:** the `HPBarWidget->SetupAttachment(GetCapsuleComponent())` upcast in `HeroCharacter.cpp` and `SummonedUnit.cpp` (the exact TASK-110 C2664 site) — both TUs include `Components/CapsuleComponent.h` (`HeroCharacter.cpp:7`, `SummonedUnit.cpp:6`); `Building.cpp` attaches to `VisualMesh` (its `UStaticMeshComponent` root). The 3 actor `.cpp`s include `Siegebound/CombatantHealthBarComponent.h`.
- **Deprecated UE 5.8 APIs: NONE.** `SetWidgetSpace`/`SetDrawSize`/`SetCollisionEnabled`/`SetGenerateOverlapEvents`/`SetVisibility`/`SetRelativeLocation`/`SetWidgetClass`/`GetWidget`/`SetWidget`/`LoadSynchronous`/`AddUniqueDynamic`/`RemoveDynamic`/`Broadcast`/`TScriptInterface::SetObject`/`SetInterface`/`GetInterface` are all current.
- **Null-safety: SOLID.** Missing widget class → silent no-bar, logged once (function-local static), early return (`.cpp:53-66`); `Cast<UCombatantHealthBarWidget>(GetWidget())` guarded by `if (Bar = …)`; `Cast<IHealthBarProvider>` / `Cast<ITeamAgent>` guarded / null-defaults to Blue; `InitForCombatant` null provider → warn + no-op (`widget .cpp:9-15`); hero `HPBarWidget->` calls null-guarded. No poll timer anywhere (grep `SetTimer` in the new component = none).

## Structural / component correctness — CONFIRMED
- Exactly ONE `HPBarWidget` (`UCombatantHealthBarComponent`) per combatant base, created in-constructor and attached to the actor hierarchy: `SummonedUnit.cpp:83-84` (→ capsule), `Building.cpp:53-54` (→ VisualMesh), `HeroCharacter.cpp:52-53` (→ capsule). Subclasses (AMinerUnit/ATower/ABarracks/ADeepMine) declare none → inherit exactly one, no duplicate. `ACastle.cpp:34` keeps its OWN plain `UWidgetComponent HPBarWidget` — separate system, untouched.
- The 3 bases implement `IHealthBarProvider` forwarding to existing getters with NO new HP state: `GetHealthCurrent→GetCurrentHP`, `GetHealthMax→GetMaxHP`, `IsHealthBarActorAlive→ !IsUnitDead / !IsBuildingDestroyed / !IsDead`, `GetHPChangedDelegate→OnHPChanged`.
- Zero combat/stat/damage-math change — only added delegate broadcasts + the component-class swap. `AGoldNode` has no bar. Castle bar untouched.

## Findings
- **[NIT / FLAG for manager]** Miner bar behavior change. The old poll component allowed a per-BP `bShowHealthBar=false` to suppress the miner bar; the rebuild's new component class does not inherit that per-BP override (constructor default `bShowHealthBar=true`), so **miners now show a bar unless a BP re-sets it false**. The handoff flags this and the spec accepts "no per-BP rewiring." Non-breaking; surfaced so the manager/Jonathan consciously accept the added on-screen clutter (or task a per-type opt-out).
- **[NIT]** `HeroCharacter.cpp:101-104` regen broadcasts every Tick frame while HP is rising (guarded so it stops exactly at max). One dynamic-multicast broadcast/frame to a single listener during regen — negligible, and it mirrors `ACastle::HandleHealTick` (which is timer-throttled). Optional: throttle to a timer for parity, but correct as written.
- **[NIT]** `CombatantHealthBarWidget` never explicitly unbinds on its own EndPlay/destruction. Safe by dynamic-delegate semantics (the provider owns the delegate and dies with it; a dead listener is skipped) — noted only so build-master doesn't read the absence as a leak.

---

## The limit of this review (STATED PER THE MANDATE — do not read a PASS as "done")
**This PASS certifies the DATA / BROADCAST layer only: the push plumbing is complete and correct — every HP
mutation on all three classes broadcasts, seed-then-bind is sound, the delegate/UFUNCTION wiring is right,
the team map is not inverted, and nothing references the deleted poll system.** It does **NOT** verify a
single rendered pixel. The five prior attempts ALL passed data-layer review while the on-screen fill stayed
frozen — because the real defect lived in the WBP (a `K2Node_CustomEvent` that is DSL-identical to a true
override but never fires from C++, and/or a material fill brush that ignores `SetPercent`). Per CONVENTIONS
l.166 ("Verification is LAW"), the feature is fixed ONLY when **TASK-131** authors `WBP_CombatantHealthBar`
as a fresh duplicate of the WORKING `WBP_CastleHealthBar`, reparented to `UCombatantHealthBarWidget`, with
`OnHPChanged`/`SetTeamColor` as TRUE overrides (`bOverrideFunction=true`) and the castle's plain-image fill
brush + grey track — and **TASK-132** proves it with a REAL GDI screenshot of a real-combat PIE showing the
fill visibly lower + correctly team-tinted (unit + hero + tower, both teams). **A green light from me is a
necessary precondition, not proof of the fix.**

## Notes for build-master (TASK-132)
- Expect a clean compile (warnings-as-errors): shadow-scan clean, includes complete (incl. the CapsuleComponent
  regression guard), deleted files gone, no dangling refs. If the compile DOES fail, the most likely site is a
  missing include in one of the 7 modified TUs — append the error here and route back.
- The pixel gate is the real acceptance. Do not commit on "machine checks pass"; require the screenshot per
  CONVENTIONS l.166.

## Board status line requested (I have no partial-edit tool — please proxy)
`- status: qa-passed (C++/data layer) — broadcast completeness VERIFIED 14/14 + denominator paths; zero dangling refs to the 5 deleted types (files gone from disk); team map correct (RED=enemy/BLUE=friendly); seed-then-bind + AddUniqueDynamic/UFUNCTION sound; scans CLEAN; castle/GoldNode untouched. DATA-LAYER PASS ONLY — does NOT prove rendered pixels; the true-override WBP (TASK-131) + real-combat screenshot gate (TASK-132) remain the actual proof of fix. 0 BLOCKER/0 WARN/3 NIT (miner-bar-now-shows FLAG for manager). Report: qa/TASK-130-report.md.`
