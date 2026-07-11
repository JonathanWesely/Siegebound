# TASK-130 handoff — gameplay-programmer (health-bar REBUILD, C++ half)

**Status:** ready-for-qa
**Scope:** C++ files only. Full teardown of the failed POLL system; rebuilt on the WORKING castle's PUSH/delegate model. No editor, no `.uasset`, no compile, no Git. `ACastle`/`FOnCastleHPChanged`/`UCastleHealthBarWidget` read-only as the template — untouched.

## Interface-or-direct-delegate choice
**Followed the castle's actual pattern: a per-actor delegate the widget BINDS to (seed-then-bind), NOT a poll.** Because ONE widget/component must bind across three unrelated base classes (`AHeroCharacter : AGitClaudeUnrealTestCharacter`, `ASummonedUnit : ACharacter`, `ABuilding : AActor`), I added a thin provider interface `IHealthBarProvider` that exposes the delegate + HP getters (the castle can hardcode `ACastle*` because there's one castle type; the unit bar can't). Team stays on the EXISTING `ITeamAgent::GetTeamId` — not duplicated.

## Delegate signature
`DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatantHPChanged, float, CurrentHP, float, MaxHP);` — declared in `HealthBarProvider.h`, mirrors `FOnCastleHPChanged` exactly. Each of the 3 base classes owns `UPROPERTY(BlueprintAssignable) FOnCombatantHPChanged OnHPChanged;` and returns it from `GetHPChangedDelegate()`.

## EVERY HP-mutation broadcast site (QA: verify completeness — "miss none")
One `OnHPChanged.Broadcast(CurrentHP, GetMaxHP())` immediately follows every `CurrentHP =` assignment. Verified by grep: 14 assignments, 14 broadcasts, 1:1.

**ASummonedUnit.cpp (4):**
- `:677` spawn-init (`CurrentHP = MaxHP` in the card-stats load) → broadcast `:681`
- `:1160` `ApplyHealing` (Cleric heal) → `:1161`
- `:1596` `TakeDamage` (after clamp, BEFORE `HandleDeath` — castle parity) → `:1600`
- `:1699` `HandleDeath` (`CurrentHP = 0`, covers non-damage deaths e.g. Sapper suicide) → `:1704`

**ABuilding.cpp (3):**
- `:226` spawn-init → `:229`
- `:299` `TakeDamage` (before `HandleDestroyed`) → `:303`
- `:323` `HandleDestroyed` (`CurrentHP = 0`) → `:327`

**AHeroCharacter.cpp (7):**
- `:78` BeginPlay init → `:81`
- `:101` Tick out-of-combat regen (guarded by the outer `!bDead && CurrentHP < max` — only fires on an actual rise, ACastle::HandleHealTick parity) → `:104`
- `:413` `TakeDamage` (before the `<=0` `HandleDeath`) → `:417`
- `:487` `HandleDeath` (`CurrentHP = 0`) → `:492`
- `:529` `ResetHero` respawn (full effective max) → `:533`
- `:617` `ApplyUpgrade` Plate Armor (raises BOTH current & the effective max → denominator changes) → `:620`
- `:650` `ResetUpgrades` (Play Again — effective max shrinks → denominator changes) → `:653`

Units/buildings `Destroy()` on death (bar dies with the actor — no explicit hide). The HERO hides/respawns, so it explicitly `HPBarWidget->HideBar()` in `HandleDeath` and `HPBarWidget->ShowBarIfEnabled()` in `ResetHero` (screen-space widgets don't follow `SetActorHiddenInGame` — the `ACastle::HandleDestroyed`/`ResetCastle` parity).

## Files
**Created (5, `Source/GitClaudeUnrealTest/Siegebound/`):**
- `HealthBarProvider.h` — `FOnCombatantHPChanged` delegate + `IHealthBarProvider`/`UHealthBarProvider` (`GetHPChangedDelegate()`, `GetHealthCurrent()`, `GetHealthMax()`, `IsHealthBarActorAlive()`), header-only, pure-C++ interface (ITeamAgent shape; GetHPChangedDelegate returns a delegate ref so none are UFUNCTIONs).
- `CombatantHealthBarWidget.h/.cpp` — `UCombatantHealthBarWidget : UUserWidget`, MIRRORS `UCastleHealthBarWidget`: `InitForCombatant(TScriptInterface<IHealthBarProvider>)` **SEEDS then BINDS**; float-only BIEs `OnHPChanged(float,float)` + `SetTeamColor(float,float,float)`; `UFUNCTION HandleHPChanged` bound via `AddUniqueDynamic` (the `HandleCastleHPChanged` shape); `TScriptInterface<IHealthBarProvider> ObservedProvider` for clean re-target/unbind.
- `CombatantHealthBarComponent.h/.cpp` — `UCombatantHealthBarComponent : UWidgetComponent`. Constructor: Screen space, DrawSize 90×12, NoCollision, soft class `/Game/UI/WBP_CombatantHealthBar.WBP_CombatantHealthBar_C`, **NOT hidden at construction (castle parity)**. BeginPlay: `SetRelativeLocation(Z=BarHeightZ)`, null-safe class resolve (missing = silent no-bar, logged once), `SetWidgetClass`, read owner as `IHealthBarProvider` → `InitForCombatant` (seed-then-bind), read owner as `ITeamAgent` → `SetTeamColor` ONCE (RED enemy / BLUE friendly, `BlueBarColor`(0.05,0.30,1.00)/`RedBarColor`(1.00,0.10,0.05)), then `ShowBarIfEnabled()`. **NO poll timer.** EditDefaultsOnly `bShowHealthBar`(true)/`BarHeightZ`(120)/tints. Public `ShowBarIfEnabled()`/`HideBar()` for the owner's respawn/death.

**Deleted (5) — the retired poll system:** `HealthBarTarget.h`, `HealthBarComponent.h`, `HealthBarComponent.cpp`, `UnitHealthBarWidget.h`, `UnitHealthBarWidget.cpp`. (Asset `/Game/UI/WBP_UnitHealthBar` delete = TASK-132.)

**Modified (7):**
- `SummonedUnit.h/.cpp`, `Building.h/.cpp`, `HeroCharacter.h/.cpp` — include swap `HealthBarTarget.h`→`HealthBarProvider.h`; fwd-decl + `HPBarWidget` type `UHealthBarComponent`→`UCombatantHealthBarComponent`; base interface `IHealthBarTarget`→`IHealthBarProvider` (+`GetHPChangedDelegate()` override); new `OnHPChanged` delegate member; `CreateDefaultSubobject<UCombatantHealthBarComponent>`; the 14 broadcasts + hero hide/show.
- `SiegeCheatManager.cpp` — its `IsCombatActorAlive` helper used the deleted `IHealthBarTarget`; swapped to `IHealthBarProvider` (same `IsHealthBarActorAlive()`), include swapped. The ONLY external consumer; no other dangling refs (grep-verified).

## What TASK-131 (art-director) must build — the WORKING castle widget is the template
1. **Duplicate the WORKING `WBP_CastleHealthBar`** (NOT the retired/tainted `WBP_UnitHealthBar`) → `/Game/UI/WBP_CombatantHealthBar`, and **reparent it to `UCombatantHealthBarWidget`** (READBACK-confirm the parent took — a silent reparent failure = a dead cast).
2. **Implement `OnHPChanged(float,float)` and `SetTeamColor(float,float,float)` as TRUE OVERRIDES** of the C++ BIEs (`bOverrideFunction=true`) — NOT `K2Node_CustomEvent`s. This is the exact defect that hid the bug 5×: a custom event is DSL-indistinguishable but NEVER fires from C++. `OnHPChanged` → `ProgressBar SetPercent(CurrentHP/MaxHP)` guard `MaxHP>0`; `SetTeamColor` → tint the FILL from the RGB.
3. **PRESERVE the castle's exact fill-brush setup** (diff against `WBP_CastleHealthBar`; the rebuild premise is the old bar DIVERGED). Unfilled track = neutral grey. Prime suspect if the fill still won't render: a `DefaultWhiteGrid_Low` MATERIAL fill brush (may not respond to `SetPercent`) — the castle's plain-image fill brush is the known-good reference; do not diverge from it.
4. The component drives it: seed-then-bind on the owner's `OnHPChanged` + one `SetTeamColor` push. No poll. RED = enemy, BLUE = friendly (both teams).

## QA scan notes (mandatory)
- **Inherited-reflected-member shadow scan: CLEAN.** New members: `OnHPChanged` (3 actors — no base has it; hero's other delegates are `OnHeroDied`/`OnRallyStateChanged`/`OnHeroUpgradesChanged`), component `HealthBarWidgetClass`/`bShowHealthBar`/`BarHeightZ`/`BlueBarColor`/`RedBarColor` (none collide with `UWidgetComponent`'s `WidgetClass`/`Space`/`DrawSize`/…), widget `ObservedProvider`. New locals (`LoadedWidgetClass`, `Bar`, `OwnerActor`, `Provider`, `ProviderInterface`, `TeamAgent`, `BarColor`, `ProviderPtr`, `OldProvider`) shadow no inherited reflected UPROPERTY; none named `Owner`/`Instigator`/`Controller`/`PlayerState`/`Slot`.
- **Complete-type-include scan: CLEAN.** `CombatantHealthBarComponent.cpp` includes `Blueprint/UserWidget.h` (GetWidget/CreateWidget path), `Components/ProgressBar.h`? (no — not used here), `Siegebound/CombatantHealthBarWidget.h` (Cast + InitForCombatant/SetTeamColor), `Siegebound/HealthBarProvider.h` (Cast<IHealthBarProvider>), `Siegebound/TeamId.h` (ITeamAgent/ETeamId), `GameFramework/Actor.h` (`TScriptInterface::SetObject(AActor*)` upcast needs the complete type), `Engine/CollisionProfile.h` (ECollisionEnabled). Actor `.cpp`s that call `HPBarWidget->HideBar()/ShowBarIfEnabled()` (hero) or `CreateDefaultSubobject<UCombatantHealthBarComponent>` (all 3) include `Siegebound/CombatantHealthBarComponent.h`.
- **No poll / no timer** anywhere in the new component (grep for `SetTimer` in the new files = none).

## Not changed / not touched
`ACastle` / `FOnCastleHPChanged` / `UCastleHealthBarWidget` / `WBP_CastleHealthBar` (the template — zero regression); `AGoldNode` (no HP bar); any `.uasset`; no combat/stat/damage math (only added delegate broadcasts + component swap). Did not compile, did not touch Git.

**Note for build-master/art-director:** the new `HPBarWidget` is a different component CLASS than the old one; any per-BP override that existed on the OLD `UHealthBarComponent` instance (e.g. a miner `bShowHealthBar=false`) does NOT carry to the new class (constructor-added defaults apply). The spec accepts this ("constructor-added, no per-BP rewiring"); flag if a specific per-type opt-out is needed.

---

# TASK-130 render-side fix (2026-07-10) — data ✓ / widget ✓ / render ✗

**Runtime data (from Jonathan's live PIE, relayed):** the widget's `OnHPChanged` receives sane DROPPING values (200→173; 80→68→…→8) and `SetTeamColor` receives correct colors (blue 0.05/0.3/1.0, red 1.0/0.1/0.05); the exec chain (event→`SetPercent`/`SetFillColorAndOpacity`) is intact, `Bar` non-null, no property binding. Yet the on-screen overhead bar stays grey/static. So the defect is the last hop: the widget-component doesn't reflect the widget's updated Percent/color on screen.

## Hosting diff — how the castle bar reaches the screen vs the unit bar
**They use the IDENTICAL mechanism** (I verified — the castle is a valid overhead reference, and there is NO separate castle HUD):
- **Castle:** `ACastle::HPBarWidget` is a `UWidgetComponent`, `EWidgetSpace::Screen`, DrawSize 256×32, Z+1050 (`Castle.cpp:34-38`). `ACastle::BeginPlay → InitHPBarWidget` does `SetWidgetClass` then `Cast<UCastleHealthBarWidget>(HPBarWidget->GetWidget())->InitForCastle(this)` (seed-then-bind on `FOnCastleHPChanged`). No HUD, no `AddToViewport` for castle HP (the only `AddToViewport` in Source is the unrelated SideScrolling template). The "top-center" position in the screenshot is just the distant enemy castle projected to screen — it IS the overhead component.
- **Unit:** `UCombatantHealthBarComponent`, `EWidgetSpace::Screen`, DrawSize 90×12, Z=BarHeightZ, `SetWidgetClass` then `InitForCombatant` (seed-then-bind on `FOnCombatantHPChanged`). Same mechanism, in the component's own BeginPlay instead of the actor's.
- **Engine host (both):** a Screen-space `UWidgetComponent` is added to `FWorldWidgetScreenLayer` as `GetUserWidgetObject()->TakeWidget()` — **LIVE Slate, NOT a render target** (`WidgetComponent.cpp:87-89`). So `SetPercent` on the inner ProgressBar repaints automatically; **`RequestRedraw` is a World-space-only concern** (World space renders to a render target and needs it — `UpdateWidget`'s body is entirely under `if (Space==World)`).

## The honest mechanism finding
By static analysis the castle (Screen, works) and my component (Screen, doesn't) are hosting-identical live-Slate bars, so mine SHOULD repaint like the castle. **I could NOT find a config difference that breaks it from source.** The failure is therefore a RUNTIME-only fact I cannot read statically, and it is one of three (all instrumented below): (a) my component isn't actually `EWidgetSpace::Screen` at runtime (a BP override → World-space stale render), (b) the widget the delegate drives is not the `GetUserWidgetObject()` on the screen layer (instance mismatch), or (c) the widget fell off / was never added to the screen layer. **It is NOT a "Screen-space needs redraw" issue — Screen-space is live Slate (engine-verified).**

## The fix (`CombatantHealthBarComponent.h/.cpp` only) — covers the component-side failure modes + instruments the rest
1. **Component-side delegate binding** — the COMPONENT now also `AddUniqueDynamic(this, &UCombatantHealthBarComponent::HandleOwnerHPChanged)` on the owner's `OnHPChanged` (additive to the widget's own seed-then-bind). `HandleOwnerHPChanged` drives the **CURRENT `GetWidget()`** directly (`Cast<UCombatantHealthBarWidget>(GetWidget())->OnHPChanged(cur,max)`) — **identity-proof:** if the widget's own binding is updating a stale/offscreen instance, this always hits the live one the screen layer hosts.
2. **`RequestRedraw()` on every update** — no-op for Screen-space live Slate, but the required repaint trigger IF the component is actually World-space at runtime.
3. **`SetTickMode(ETickMode::Enabled)` in the constructor** — guarantees `UpdateWidgetOnScreen()` (end of `TickComponent`) runs every frame, keeping the widget hosted on / following the screen layer (covers "fell off the screen layer").
4. **`[TASK130DIAG]` probes** (BeginPlay + one-shot-per-actor on first update) log the DECISIVE runtime facts: `space` (0=World/1=Screen), `DrawSize`, `tickEnabled`, `GetWidget` name, and `same` (delegate-bound widget == current `GetWidget()`). These pin the mechanism in ONE PIE pass. TEMPORARY — strip with TASK-128-style cleanup once confirmed. (Coordinated with art-director's widget-side `COMBATANT_OHC/STC` probes — disjoint.)

Kept the widget's seed-then-bind (castle parity) — the component drive is additive/redundant when there's no mismatch, and harmless (idempotent `SetPercent`).

## What the log will tell build-master (read after ONE PIE pass)
- `space=0 (World)` → the fix's `RequestRedraw` should now repaint it → **fixed, component-side**.
- `space=1 (Screen) same=0` → instance mismatch → the fix's drive of live `GetWidget()` should repaint it → **fixed, component-side**.
- `space=1 same=1 tickEnabled=1` **and it STILL doesn't repaint** → the component is exonerated; the fault is the WIDGET ASSET's Slate invalidation/volatility (e.g. an `SInvalidationBox`/`bIsVolatile=false` cache, or a fill brush that ignores `SetPercent`) → **bounces to art-director (TASK-131)**, not fixable in the component.

**NOT a machine-verifiable close:** per the CONVENTIONS verification law + OVERNIGHT-AUTH §3, the ONLY proof is Jonathan seeing the overhead bar visibly drop + show blue/red in PIE. I did NOT declare it fixed from code reading. Needs build-master to compile (editor close is Jonathan's) + Jonathan re-test + read `[TASK130DIAG]`.

**Files changed this pass:** `CombatantHealthBarComponent.h`, `CombatantHealthBarComponent.cpp` only. No compile, no Git, no `.uasset`, `ACastle`/`UCastleHealthBarWidget` read-only.
