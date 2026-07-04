# TASK-058 — Hero upgrade system + War Banner aura (handoff)

**Status:** ready-for-qa · files only, no compile, no Git, no board edit · TASK-056's files (Projectile/Tower) NOT touched.

## Files changed (only these two)
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`

**No new module / no Build.cs change.** `Engine/DataTable.h` + `Siegebound/CardRow.h` are already in the module's link set (used by ASummonedUnit); just added the two `#include`s to HeroCharacter.cpp.

## Design decision: upgrade state ON THE HERO (no UHeroUpgradeComponent)
The spec allowed either. I put the state directly on `AHeroCharacter` because every effect point is a hero member — the melee damage (`DoMeleeAttack`), the walk/sprint speeds (`CharacterMovement->MaxWalkSpeed`), `CurrentHP`/`MaxHP`, the aura's world/location/team, and the death→respawn hook (`ResetHero`). A component would need a back-reference to the hero and to reach into `CurrentHP`/movement anyway, so a component adds plumbing without separation. It also keeps the delegate reachable exactly like the existing `OnHeroDied`/`OnRallyStateChanged` (the HUD in TASK-064 binds the hero, same as TASK-050 did). `HeroUpgradeComponent.h/.cpp` were NOT created.

## Drift-free composition (the core discipline)
**Base stats are never mutated.** The ONLY mutated state is four `int32` stack counts (`SharpenedBladeStacks`, `PlateArmorStacks`, `SwiftBootsStacks`, `WarBannerStacks`). Every bonus derives LIVE from the stack count via inline getters:
- `GetEffectiveMeleeDamage() = MeleeDamage + MeleeDamageBonus * SharpenedBladeStacks`  (20 → 30 → 40)
- `GetEffectiveMaxHP() = MaxHP + MaxHPBonus * PlateArmorStacks`  (200 → 300 → 400)
- `GetMoveSpeedBonusFraction() = MoveSpeedBonus * SwiftBootsStacks`; `GetEffectiveWalkSpeed()/GetEffectiveSprintSpeed()` multiply the base by `(1 + fraction)`

Because bonuses are computed from stacks and stacks are the only state, there is literally no accumulator to drift (a stronger guarantee than Rally's cache-once). Melee and MaxHP therefore persist through respawn **automatically** (the stack survives). Only move speed needs an active re-push (it lives in the movement component) — done by `ApplyMovementSpeed()`, which is the single writer of `MaxWalkSpeed` (walk vs sprint per a new `bSprinting` flag). `GetMaxHP()` now returns the EFFECTIVE max; all internal clamps (Tick regen cap, TakeDamage clamp, ResetHero full-heal) route through `GetEffectiveMaxHP()`.

### Base non-regression (0 stacks ⇒ effective == base, verified by reasoning)
- Walk 500 / sprint 750: `GetEffectiveWalkSpeed()=500*(1+0)`, sprint `750*(1+0)`. StartSprint/StopSprint now set `bSprinting` then `ApplyMovementSpeed()` — identical values.
- Cone melee 20: `GetEffectiveMeleeDamage()=20+0`. Applied at the same `ApplyDamage` call site.
- 200 HP + regen: `GetEffectiveMaxHP()=200+0`; regen still caps at it.
- M3 Rally (TASK-042) + its input binding: untouched. ResetHero's Rally reset block is unchanged.

## ApplyUpgrade — EXACT signature + refund contract (TASK-059 consumes this)
```cpp
UENUM(BlueprintType)
enum class EHeroUpgradeResult : uint8 { Applied, RefusedAtMaxStacks, RefusedInvalidCard };

// AHeroCharacter, BlueprintCallable
EHeroUpgradeResult ApplyUpgrade(FName UpgradeCardID);
```
**TASK-059 Instant path rule:** spend the row Cost + `ConfirmPlayFromHand(Slot)` ONLY when the return is `Applied`. Any other value ⇒ refuse the play with **NO spend** (§3.0/§3.10 full refund):
- `RefusedAtMaxStacks` → `OnCardRefused("… at max stacks")` (already at the card's MaxCopies).
- `RefusedInvalidCard` → unknown CardID, or DT_Cards/its row unavailable so the cap can't be resolved (never guessed). Refuse.

`ApplyUpgrade` works whether the hero is alive or dead (the persistent stack is always added). Alive-only side effects — the Plate Armor immediate heal and the active aura pulse — are guarded by `!bDead` and re-established on respawn by `ResetHero`.

### Stack caps come from DT_Cards (MaxCopies), never hardcoded
`GetStackCapForUpgrade(FName)` reads `MaxCopies` from `/Game/Data/DT_Cards` (cached at BeginPlay via a new `CardTableAsset` soft ref, mirroring ASummonedUnit). CSV confirms SharpenedBlade=2, PlateArmor=2, SwiftBoots=1, WarBanner=1 — matching the spec caps. Table/row missing ⇒ cap 0 ⇒ `RefusedInvalidCard` (same "no table → no stats" degradation as the unit).

## Persistence-through-respawn + reset hooks (SCRUTINIZE)
- **Persist on death:** `HandleDeath` does NOT clear stacks; it only pauses the aura (`StopWarBannerAura`, timer only) and clears `bSprinting`.
- **Re-apply on respawn:** `ResetHero` (the existing death→respawn path, called by the game mode) sets `CurrentHP = GetEffectiveMaxHP()` (full effective HP), `ApplyMovementSpeed()` (re-pushes Swift Boots), `StartWarBannerAura()` (re-arms the pulse if still owned), and `BroadcastUpgradesChanged()` (so a HUD rebuilt around respawn is accurate). Melee/HP bonuses are live so they need no explicit re-apply.
- **Reset on Play Again:** `ResetUpgrades()` zeros all four stacks, stops the aura, re-applies base speed, clamps `CurrentHP` down to the base max, and broadcasts the now-empty row.

### ⚠️ REQUIRED INTEGRATION (one line, outside this task's files)
`ResetHero()` is called by the game mode on BOTH respawn and Play Again, so the hero cannot self-distinguish them. Per the files-only constraint I did NOT edit the game mode. **The match-reset owner must call `ResetUpgrades()` on the hero.** Suggested site — `ASiegeGameMode::PlayAgain()`, step 5, immediately BEFORE `RestoreHeroAtStart()`:
```cpp
if (IsValid(TrackedHero)) { TrackedHero->ResetUpgrades(); }  // TASK-058: clear upgrades on Play Again
RestoreHeroAtStart();
```
Placing it before `RestoreHeroAtStart()` means `ResetHero` then re-applies zero stacks → clean base hero (calling it after also works — `ResetUpgrades` re-syncs and clamps). Without this call, upgrades correctly PERSIST but never RESET. Build-master / a follow-up owns this wire-up.

## War Banner aura loop (TASK-055 consumer)
`WarBanner` (cap 1) enables a looping `WarBannerAuraTimerHandle` at `WarBannerPulseInterval` (0.5s). `PulseWarBannerAura()` mirrors Rally's TASK-042 loop: `GetAllActorsOfClass(ASummonedUnit)`, skip dead / wrong-team, distance-square vs `WarBannerAuraRadius` (600), then `FriendlyUnit->SetAuraDamageBonus(WarBannerDamageBonus /*0.20*/, PulseDuration)`. `PulseDuration = interval*2` so a unit staying in range never flickers and a unit leaving loses the bonus one window later (unit-side self-expiry, TASK-055 refresh-not-stack). Pulses immediately on enable (no interval wait). Guarded by `bDead`/`WarBannerStacks<=0`. Match-end safety: `SetAuraDamageBonus` no-ops on `bAIFrozen`/`bDead` units (verified in SummonedUnit.cpp:318), so a hero still pulsing under the Victory screen buffs nobody; Play Again destroys units + `ResetUpgrades` clears the aura.

## FOnHeroUpgradesChanged delegate (TASK-064 consumes this)
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHeroUpgradesChanged,
    int32, SharpenedBladeStacks, int32, PlateArmorStacks, int32, SwiftBootsStacks, int32, WarBannerStacks);
// member: UPROPERTY(BlueprintAssignable) FOnHeroUpgradesChanged OnHeroUpgradesChanged;
```
Four plain int params (0 = not owned) — MCP-authorable, no enums/structs (CONVENTIONS MCP-param rule). Broadcast on every `Applied` ApplyUpgrade, on `ResetUpgrades`, and on `ResetHero` (respawn). **Seed-then-bind for TASK-064:** seed from `GetSharpenedBladeStacks()/GetPlateArmorStacks()/GetSwiftBootsStacks()/GetWarBannerStacks()` (or the generic `GetUpgradeStackCount(FName)`), read caps for pips via `GetUpgradeStackCap(FName)`, THEN bind `OnHeroUpgradesChanged`.

## What QA should scrutinize
1. **Play Again reset requires the game-mode one-liner above** — logic is correct and self-contained in `ResetUpgrades`, but the reset does not fire until wired. Confirm this is acceptable as a flagged integration (analogous to the ApplyUpgrade/delegate cross-task consumers).
2. `GetMaxHP()` semantics changed to EFFECTIVE. Only external readers: `ASummonedUnit::GetTargetMaxHP` (Slayer gate — buffed hero still ≥150, no behavior change) and the castle's own bar (separate class). No hero HP-bar widget binds it.
3. C4458 shadow scan: new locals are `Movement`, `World`, `CardTable`, `Row`, `Context`, `EffectiveMaxHP`, `UnitActor`, `FriendlyUnit`, `UpgradeCardID`, `StackCap`, `bKnownUpgrade` — none shadow an inherited reflected UPROPERTY (Owner/Instigator/Controller/PlayerState/etc.). The delegate param names live in the generated delegate struct, not on the hero.
4. Plate Armor heal while dead is intentionally skipped (a corpse isn't revived; respawn heals to full effective max).
