// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/TeamId.h"
#include "SpellLibrary.generated.h"

class UWorld;

/**
 *  Siegebound spell resolver (GDD §3.5/§3.11/§4 Set III; M5 rulings 1-7 + 11,
 *  TASK-098). The ONE home for spell resolution — shared by
 *  ASiegePlayerController (targeting mode, TASK-100) and ASiegeBotController
 *  (M5 bot spell rules, TASK-102). Spells have NO actor blueprints; everything
 *  resolves through ResolveSpell, dispatched on the row's SpellEffect column
 *  (TASK-097).
 *
 *  Effects (per the M5 manager rulings):
 *  - AoEDamage (Fireball): row Damage to every ENEMY within row AoERadius of
 *    TargetPoint, via the shared TASK-055 radial helper
 *    (FSiegeCombatStatics::ApplyRadialDamage) — closest-point distance, no
 *    friendly fire, each hit routed through the receiver's TakeDamage. Castle
 *    hits therefore flow through ACastle::TakeDamage tagged
 *    USiegeDamageType_Spell and take 50% (§3.11 acceptance: 50, not 100).
 *  - Freeze (FrostNova): ApplyFreeze(row EffectDuration) on every enemy
 *    ASummonedUnit and ABuilding within row AoERadius (TASK-099 pinned API).
 *    The castle is NEVER freezable and the hero is NOT freezable in M5
 *    (ruling 5) — neither type is touched here.
 *  - TopTargetsDamage (Lightning): the row MaxTargets HIGHEST-CURRENT-HP enemy
 *    actors (units, hero, buildings/towers — castle EXCLUDED, anti-sniping)
 *    within row AoERadius of TargetPoint each take row Damage; ties broken by
 *    distance to the reticle (deterministic, ruling 4). Current HP, not MaxHP
 *    (Slayer's MaxHP gate is a different concept). Buildings take FULL spell
 *    damage — Lightning (200) kills an Arrow Tower (150), "the tower-killer".
 *  - AllyBuff (Battle Cry): ApplyCombatBuff(GetBattleCryMoveSpeedMultiplier(),
 *    GetBattleCryAttackSpeedMultiplier(), row EffectDuration) on every
 *    FRIENDLY ASummonedUnit within row AoERadius (TASK-099 pinned API +
 *    composition). Magnitudes are the UNIT'S OWN BattleCry* mechanic
 *    UPROPERTYs (M5 ruling 6), read per-unit through those accessors —
 *    GDD §4 defaults +25% move / +50% attack; nothing hardcoded here.
 *  - GoldSteal (Pickpocket): instant global effect (ruling 7 — no reticle);
 *    steals min(row GoldSteal, victim's gold) from the enemy team's
 *    ASiegePlayerState to the caster's, composed EXCLUSIVELY from the existing
 *    choke-pointed gold APIs (SpendGold on the victim + AddGold on the caster;
 *    SetGold choke-point law). TargetPoint is only the VFX anchor here.
 *
 *  VFX contract (ruling 11): every SUCCESSFUL resolve spawns
 *  /Game/VFX/NS_Spell_<CardID> at TargetPoint — soft path COMPOSED from the
 *  CardID, null-safe: a missing system logs once per CardID and the spell
 *  still resolves.
 *
 *  Refusal semantics (the caller REFUNDS on false — card-leaves-hand-at-
 *  CONFIRM law, TASK-100): false means the spell did NOT resolve and no state
 *  changed — null World, NAME_None CardID, SpellEffect None/unknown, a
 *  malformed row (non-positive Damage/AoERadius/EffectDuration/MaxTargets/
 *  GoldSteal where the effect needs them), or GoldSteal with an unresolvable
 *  game state / player state (e.g. Sandbox mode has no Red economy). A
 *  well-formed cast that merely catches ZERO targets (Fireball on empty
 *  ground, FrostNova nobody in radius, Pickpocket on a 0-gold victim) is a
 *  SUCCESSFUL resolve — the spell is spent, exactly like a wasted Fireball.
 *
 *  Never crashes on bad input: every failure path logs and returns false.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USpellLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 *  Resolves the spell carried by Row (dispatch on Row.SpellEffect) for
	 *  CasterTeam at TargetPoint. Pinned entry point (CONVENTIONS "Spells &
	 *  Set III (M5)") — both the player targeting mode (TASK-100) and the bot
	 *  (TASK-102) call EXACTLY this. Returns true when the spell resolved
	 *  (gold stays spent); false when it refused (caller must refund — see the
	 *  class comment for the exact refusal set). CardID names the VFX
	 *  (/Game/VFX/NS_Spell_<CardID>) and the log lines; Row supplies every
	 *  stat (nothing hardcoded, GDD §3.0). C++-only entry, deliberately NOT
	 *  BlueprintCallable (qa/TASK-098 NIT): both pinned callers are C++, and a
	 *  raw UWorld* pin on a static library node invites BP misuse.
	 */
	static bool ResolveSpell(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint);
};
