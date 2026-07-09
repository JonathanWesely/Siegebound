# QA Report — TASK-110
Verdict: **PASS**  (0 BLOCKER · 1 WARN · 3 NIT)

Overhead health-bar system (C++, files only). Pre-compile review; TASK-112 phase A compiles it.
Scope reviewed: new HealthBarTarget.h, HealthBarComponent.h/.cpp, UnitHealthBarWidget.h/.cpp;
modified SummonedUnit.h/.cpp, Building.h/.cpp, HeroCharacter.h/.cpp. Cross-read (read-only):
TeamId.h (ITeamAgent), Castle.h/.cpp + CastleHealthBarWidget.h (mirrored pattern), GoldNode.h.

## Headline — MANDATORY shadow-scan (C4457/58/59): CLEAN
- `UHealthBarComponent : UWidgetComponent` new members — `HealthBarWidgetClass, PollInterval,
  bShowHealthBar, BarHeightZ, BlueBarColor, RedBarColor, BarWidget, bBarShown, PollTimerHandle`
  — NONE collide with any `UWidgetComponent` reflected member (`Space, DrawSize, WidgetClass,
  Widget, TintColorAndOpacity, BackgroundColor, Pivot, GeometryMode, …`). In particular
  `BarWidget`≠`Widget` and `HealthBarWidgetClass`≠`WidgetClass`. No new member/param/local named
  `Owner`, `Instigator`, or `Slot` anywhere in the batch.
- `HPBarWidget` (TObjectPtr<UHealthBarComponent>) added to ASummonedUnit / ABuilding /
  AHeroCharacter. Project-wide grep confirms the only other `HPBarWidget` declaration is
  `ACastle::HPBarWidget` (a `UWidgetComponent` on the UNRELATED `AActor`-direct hierarchy) — not
  a base of any of the three, so the shared subobject-name string is NOT a collision (subobject
  names are per-class-instance, not global) and NOT a member shadow. Confirmed no parent in the
  ACharacter/APawn/AActor/AGitClaudeUnrealTestCharacter chains declares `HPBarWidget`.
- New `HealthBarComponent.cpp::PollHealth` locals (`Target, bAlive, Current, Max, bShouldShow,
  BarColor, TeamAgent, LoadedWidgetClass, World`) shadow nothing.

## Findings
- [WARN] UnitHealthBarWidget.h / HealthBarComponent.cpp:80 — TASK-111 CONTRACT (carry-forward, not
  a code defect): the component drives HP/tint ONLY through `BarWidget = Cast<UUnitHealthBarWidget>(
  GetWidget())`. If TASK-111's WBP_UnitHealthBar is NOT reparented to `UUnitHealthBarWidget`, the
  cast returns null → the bar renders but never receives `OnHPChanged`/`SetTeamColor` (blank/untinted
  fill), with NO crash and NO log. Silent by design. build-master/TASK-112 must PIE-verify the fill
  actually fills and the tint applies, not merely that a widget appears.
- [NIT] HealthBarComponent.cpp:117 — `Cast<IHealthBarTarget>(GetOwner())` runs every poll (~0.15 s).
  The owner is fixed for the component's life; caching the interface pointer at BeginPlay (alongside
  `BarWidget`) would drop a per-poll dynamic cast. Negligible at 60 actors; current form is
  correctly defensive. Optional.
- [NIT] HealthBarComponent.cpp:68 — the missing-widget log uses `LogGitClaudeUnrealTest` rather than a
  `LogSiege<Domain>` category. Consistent with existing SummonedUnit/Castle practice for one-off
  asset-missing warnings, so acceptable; noted only for the CONVENTIONS logging rule.
- [NIT] HealthBarComponent.cpp — component render-tick is left enabled (screen-space widget must tick
  to follow the actor on screen — the ACastle precedent). Correct; no gameplay work on tick (HP logic
  is timer-driven). Called out so build-master does not mistake it for a per-tick smell.

## Explicit rulings on all 8 FLAGGED DECISIONS
1. **UFUNCTION(BlueprintCallable) on each pure-virtual const interface method — ACCEPTED (VERIFIED, not
   taken on faith).** Read TeamId.h:25-40: `UTeamAgent : UInterface` is `UINTERFACE(MinimalAPI,
   NotBlueprintable)` and `ITeamAgent::GetTeamId` is `UFUNCTION(BlueprintCallable, Category="Team")
   virtual ETeamId GetTeamId() const = 0;`. `IHealthBarTarget` is character-for-character the same
   shape (three such methods). Implementers override in PLAIN C++ with NO UFUNCTION on the override —
   exactly as ASummonedUnit/ABuilding/AHeroCharacter already do for `GetTeamId` (SummonedUnit.h:113
   vs :117-119). This is the shipped ITeamAgent pattern; it compiles. NOT a BlueprintNativeEvent —
   correct (C++-only poll path, no BP override).
2. **Constructor vs BeginPlay init split — ACCEPTED.** Static, instance-invariant config (Screen space,
   DrawSize 90×12, NoCollision, start-hidden, default soft class) in the ctor; world/BP-tunable-dependent
   work (relative-Z from BarHeightZ, class LoadSynchronous, `SetWidgetClass`+widget cache, one-time team
   tint, timer arm) in BeginPlay — mirrors the ACastle configure-in-ctor / resolve-at-BeginPlay pattern.
   BarHeightZ correctly deferred so a BP default override (which lands before BeginPlay) is honored.
3. **`WBP_UnitHealthBar.WBP_UnitHealthBar_C` soft path — ACCEPTED.** The `_C` runtime generated-class
   suffix mirrors `ACastle::HPBarWidgetClass` (Castle.cpp:50 `WBP_CastleHealthBar.WBP_CastleHealthBar_C`).
   Asset-name portion matches the names block (`/Game/UI/WBP_UnitHealthBar`). Carry-forward to TASK-111.
4. **Hide-on-death latency ≤1 poll for the HERO only — ACCEPTED.** Units/buildings Destroy() on death →
   component EndPlay clears the timer and the bar vanishes with the actor. The hero HIDES (not destroyed);
   `PollHealth` gates on `IsHealthBarActorAlive()` = `!IsDead()` and hides within ≤PollInterval (0.15 s).
   Robust even if actor-hidden also hides the screen-space widget (double-hide, harmless). Imperceptible.
5. **Own `bBarShown` latch instead of engine IsVisible() — ACCEPTED.** Deterministic show/hide with no
   dependency on `USceneComponent::IsVisible()`/hidden-in-game semantics for screen-space widgets; the
   latch is seeded false and the ctor hides the component, so initial state is consistent. Avoids
   redundant render-state dirties.
6. **"Log once" run-wide via function-local `static bool` — ACCEPTED.** One line total for a missing WBP
   across 60+ actors; BeginPlay is game-thread only, so the non-atomic static is race-free. Level `Log`
   is appropriate (expected transient state until TASK-111 authors the asset).
7. **Epsilon 0.01 for the full-HP hide test (`Current < Max − 0.01`) — ACCEPTED.** Guards a 1-px sliver at
   exactly full and during fractional hero regen approaching max; trivial vs the smallest unit HP (~80)
   and vs the hero's effective max. Also correctly makes a statless actor (Max≤0) test false → stays hidden.
8. **`bShowHealthBar` default true — ACCEPTED.** Matches M5.5 manager ruling 1 (miners inherit the bar,
   suppressible per-BP with zero code). EditDefaultsOnly, so a playtest opt-out needs no recompile.

## Interface correctness (verified)
- IHealthBarTarget implemented on all three bases forwarding to the correct EXISTING getters, NO new HP
  state: ASummonedUnit → `GetCurrentHP`/`GetMaxHP`/`!IsUnitDead` (SummonedUnit.h:117-119); ABuilding →
  `GetCurrentHP`/`GetMaxHP`/`!IsBuildingDestroyed` (Building.h:72-74); AHeroCharacter → `GetCurrentHP`/
  `GetMaxHP`(already effective Plate-Armor max)/`!IsDead` (HeroCharacter.h:124-126). Overrides carry no
  UFUNCTION — matches the GetTeamId override style.
- Multiple native-interface inheritance (`: ITeamAgent, public IHealthBarTarget`) is valid UHT; the two
  interfaces are independent (no diamond). `Cast<IHealthBarTarget>`/`Cast<ITeamAgent>` on GetOwner() is
  the established codebase pattern.
- ACastle NOT modified for the interface (Castle.h:53 `: public AActor, public ITeamAgent` only; its M1
  `UWidgetComponent HPBarWidget` + delegate bar untouched). AGoldNode has ZERO health-bar references
  (GoldNode.h grep empty). Both correct per ruling 1.

## Component / wiring correctness (verified)
- Exactly one `UHealthBarComponent` named `HPBarWidget` per base ctor: ASummonedUnit → capsule
  (SummonedUnit.cpp:83-84); ABuilding → VisualMesh root, created AFTER VisualMesh is made root
  (Building.cpp:28-29 then :54-55); AHeroCharacter → capsule (HeroCharacter.cpp:51-52). Subclasses
  (ATower/ABarracks/ADeepMine/AMinerUnit) declare NO `HPBarWidget` → inherit exactly one. No duplicate.
- Poll logic matches spec exactly: `bShouldShow = bShowHealthBar && bAlive && (Current < Max −
  0.01)` (HealthBarComponent.cpp:130) = hide iff `!bShowHealthBar || !alive || Current ≥ Max−eps`; else
  SHOW + `OnHPChanged(Current, Max)`. Null-safe on every path: unset/missing widget class → silent no-bar,
  timer never armed, early return (`:62-75`); non-target owner → guarded return (`:117-121`); mis-authored
  widget (wrong class) → BarWidget null, tint + drive skipped, no crash (`:86, :152`). EndPlay clears the
  timer (`:103-111`) — no dangling poll after the owner leaves play.
- BIE signatures are FLOAT-ONLY exactly (the TASK-111 contract): `OnHPChanged(float CurrentHP, float
  MaxHP)` and `SetTeamColor(float R, float G, float B)` (UnitHealthBarWidget.h:35-45).
- Tunable defaults match CONVENTIONS/names block: PollInterval 0.15 (ClampMin 0.02), bShowHealthBar true,
  BarHeightZ 120, BlueBarColor (0.05,0.30,1.00), RedBarColor (1.00,0.10,0.05), DrawSize 90×12, Screen
  space, NoCollision.
- Zero combat/stats behavior change: interface methods are pure const reads of existing getters; the
  component adds a NoCollision, non-nav screen-space widget with a read-only HP poll — it never mutates
  HP/damage/movement/timers of the host, and NoCollision keeps it invisible to the ECC_Pawn reach/
  acquisition/placement traces. Inert to gameplay. Confirmed.
- Includes are complete: bases include HealthBarComponent.h in .cpp (full type for CreateDefaultSubobject)
  and HealthBarTarget.h in .h (interface base); HealthBarComponent.cpp includes UserWidget, World,
  TimerManager, HealthBarTarget, TeamId, UnitHealthBarWidget. PollHealth/BeginPlay/EndPlay are plain
  member functions (timer callbacks need no UFUNCTION). GC-safe pointers throughout (UPROPERTY +
  TObjectPtr on BarWidget and the three HPBarWidget members).

## Notes for build-master (TASK-112)
- Compiles clean expected (warnings-as-errors): shadow-scan CLEAN, includes complete, ITeamAgent-shape
  interface proven. No Build.cs change (UMG/Slate already deps).
- PIE exit-criteria to verify: damage a unit / tower / wall / Barracks / Deep Mine / miner / hero and
  confirm each bar (a) hidden at full, (b) appears + tracks down on first damage, (c) team-tinted (blue
  friendly, red via the bot's units/towers), (d) hides on death/destruction, (e) NEVER on a castle
  (its own bar only) or gold node. The WARN above is the thing most likely to slip: verify the fill
  actually FILLS and the tint applies (proves the WBP is reparented to UUnitHealthBarWidget), not merely
  that a bar shows.

## Carry-forwards for TASK-111 (art)
- Author `WBP_UnitHealthBar` at EXACTLY `/Game/UI/WBP_UnitHealthBar` (the ctor soft path is
  `…WBP_UnitHealthBar.WBP_UnitHealthBar_C`); duplicate the WBP_CastleHealthBar donor and REPARENT to
  `UUnitHealthBarWidget` (mandatory — the component casts to it; wrong parent = silent blank bar).
- Implement `OnHPChanged(float,float)` → ProgressBar `SetPercent(CurrentHP/MaxHP)` guarding `MaxHP > 0`;
  `SetTeamColor(float,float,float)` → tint the FILL brush from the RGB. Both BIEs already exist on the base.
- Design for DrawSize 90×12, Screen space (reads at ~150 px). No enum/struct params — floats only.

---

## BUILD-MASTER — TASK-112 phase A compile: FAILED (2026-07-09)

Editor-bounce compile (editor gracefully closed, DLL released, PIE was off). Build.bat
`GitClaudeUnrealTestEditor Win64 Development` **FAILED — Result: Failed (OtherCompilationError),
exit code 6.** 1 error, 0 warnings. Shadow-scan HELD: zero C4457/C4458/C4459 (QA's clean scan
was correct). The failure is unrelated to any shadow — it is an include-completeness upcast error
in ONE translation unit (HeroCharacter.cpp). The other four batch files compiled clean:
UnitHealthBarWidget.cpp, HealthBarComponent.cpp, Building.cpp, SummonedUnit.cpp all built OK
(SummonedUnit attaches HPBarWidget to its capsule the same way and compiled fine).

### Full compiler error (verbatim)
```
[4/11] Compile [x64] HeroCharacter.cpp
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\HeroCharacter.cpp(52,15): error C2664: 'void USceneComponent::SetupAttachment(USceneComponent *,FName)': cannot convert argument 1 from 'UCapsuleComponent *' to 'USceneComponent *'
	HPBarWidget->SetupAttachment(GetCapsuleComponent());
	             ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\HeroCharacter.cpp(52,50): note: Types pointed to are unrelated; conversion requires reinterpret_cast, C-style cast or parenthesized function-style cast
	HPBarWidget->SetupAttachment(GetCapsuleComponent());
	                                                ^
C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\Components\SceneComponent.h(734,18): note: see declaration of 'USceneComponent::SetupAttachment'
	ENGINE_API void SetupAttachment(USceneComponent* InParent, FName InSocketName = NAME_None);
	                ^
C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\HeroCharacter.cpp(52,15): note: while trying to match the argument list '(UCapsuleComponent *)'
	HPBarWidget->SetupAttachment(GetCapsuleComponent());
	             ^

Result: Failed (OtherCompilationError)
```

### Build-master observation (diagnostic only — NOT a fix; gameplay-programmer owns the change)
`UCapsuleComponent` is incomplete (forward-declared only) in HeroCharacter.cpp's translation unit,
so the compiler cannot see it derives from `USceneComponent` and rejects the implicit upcast at
HeroCharacter.cpp:52. SummonedUnit.cpp performs the identical `HPBarWidget->SetupAttachment(
GetCapsuleComponent())` and compiled clean — so the difference is which headers each .cpp pulls in.
Likely resolution is on the gameplay-programmer's side (e.g. the full CapsuleComponent definition
reaching HeroCharacter.cpp). Not evaluated further here per role boundary.

STATUS → routes back to gameplay-programmer (counts as a QA loop). Editor left CLOSED (DLL free for
the recompile after the fix). Phase A will re-run on the next build-master dispatch.
