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
 *  DELIVERY (spell delivery overhaul, 2026-07-21 — TASK-236, CONVENTIONS
 *  "Spell delivery overhaul (2026-07-21)"): GetEffectiveDelivery(Row) selects
 *  HOW an effect reaches its targets. HeroLine spells (AoEDamage + Freeze
 *  under the Auto default — Fireball and FrostNova, the two directive cards)
 *  fire FROM the caster along a horizontal line in the air: origin = the
 *  caster team's living hero (capsule center ≈ chest), or — the bot, which
 *  has no hero (flagged design default) — its own castle at
 *  ASpellLineSweep::CastleMuzzleHeight; direction = origin → TargetPoint,
 *  FLATTENED horizontal. For these spells TargetPoint is the AIM-POINT, not
 *  an impact center — the pinned signature is unchanged, the semantics shift
 *  is flagged at every call site. Delivery = a spawned ASpellLineSweep
 *  (fast-travel front, per-segment exactly-once application — its class
 *  comment records the design); line length/width/travel are that actor's
 *  UPROPERTY tunables. Effect magnitudes, costs, castle-50%, and
 *  friendly-fire semantics are UNCHANGED by delivery. GroundCircle spells
 *  (Lightning/BattleCry/everything else) are byte-untouched below.
 *
 *  Effects (per the M5 manager rulings):
 *  - AoEDamage (Fireball) — GroundCircle path (data-overridable): row Damage
 *    to every ENEMY within row AoERadius of TargetPoint, via the shared
 *    TASK-055 radial helper (FSiegeCombatStatics::ApplyRadialDamage) —
 *    closest-point distance, no friendly fire, each hit routed through the
 *    receiver's TakeDamage. Castle hits therefore flow through
 *    ACastle::TakeDamage tagged USiegeDamageType_Spell and take 50% (§3.11
 *    acceptance: 50, not 100). HeroLine path (the Auto default since
 *    2026-07-21): the same per-hit semantics applied along the line by
 *    ASpellLineSweep instead of inside a circle.
 *  - Freeze (FrostNova) — GroundCircle path (data-overridable): ApplyFreeze
 *    (row EffectDuration) on every enemy ASummonedUnit and ABuilding within
 *    row AoERadius (TASK-099 pinned API). The castle is NEVER freezable and
 *    the hero is NOT freezable in M5 (ruling 5) — neither type is touched.
 *    HeroLine path (the Auto default since 2026-07-21): the same ruling-5
 *    type filter applied along the line by ASpellLineSweep.
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
 *  game state / player state (e.g. Sandbox mode has no Red economy).
 *  HeroLine additions (TASK-236, all position-independent for the PLAYER —
 *  the controller pre-checks the one aim-dependent case free-of-charge):
 *  no resolvable line origin (no living caster-team hero AND no standing
 *  caster-team castle), a degenerate flattened aim direction (aim-point
 *  directly above/below the origin), a non-positive LineRange/LineHalfWidth
 *  config, or a failed sweep spawn. A well-formed cast that merely catches
 *  ZERO targets (Fireball on empty ground, a line through empty air,
 *  FrostNova nobody in reach, Pickpocket on a 0-gold victim) is a SUCCESSFUL
 *  resolve — the spell is spent, exactly like a wasted Fireball. THERE IS NO
 *  REFUND FOR A LINE THAT HITS NOTHING (the recorded no-hit rule).
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
	 *
	 *  TargetPoint semantics (TASK-236 overhaul — signature unchanged): for
	 *  GroundCircle spells it stays the impact center; for HeroLine spells
	 *  (IsLineDeliverySpell) it is the AIM-POINT — the resolver derives the
	 *  origin (caster hero / bot castle) and fires the line origin→TargetPoint,
	 *  flattened horizontal. Every call site carries the flag comment.
	 */
	static bool ResolveSpell(UWorld* World, FName CardID, const FCardRow& Row, ETeamId CasterTeam, const FVector& TargetPoint);

	/**
	 *  The row's EFFECTIVE delivery (TASK-236): an explicit SpellDelivery cell
	 *  wins; Auto (the sparse default — cards.csv carries no cells this wave)
	 *  resolves per-effect: AoEDamage/Freeze → HeroLine (the 2026-07-21
	 *  directive — today exactly Fireball + FrostNova), everything else →
	 *  GroundCircle. Pure row math — safe on any row, spell or not.
	 */
	static ESpellDelivery GetEffectiveDelivery(const FCardRow& Row);

	/**
	 *  True when the row delivers as a hero-origin line (TASK-236). The
	 *  targeting-mode aim pass gates on this: line spells need only an AIM
	 *  DIRECTION at confirm — the surface-under-cursor requirement is relaxed
	 *  for them (a deprojected cursor ray suffices), while GroundCircle spells
	 *  keep the M5 confirm gate byte-for-byte.
	 */
	static bool IsLineDeliverySpell(const FCardRow& Row);

	/**
	 *  ⭐⭐ THE ONE DERIVATION OF *"DOES THIS SPELL HAVE A RETICLE?"* IN THE WHOLE
	 *  TREE (TASK-999 authored it, TASK-1018 MOVED it here). True when the row is
	 *  AIMED — the deck-builder prints an aiming sentence for it and
	 *  ASiegePlayerController routes it into TARGETING mode. False means the spell
	 *  resolves INSTANTLY, with no reticle and no cursor: the `Pickpocket`
	 *  precedent (M5 ruling 7), which `Fog` and `BrightSun` now share
	 *  (`FOG-§10.1`: "NO RETICLE").
	 *
	 *  ⛔⛔ IT IS NOT `GetEffectiveDelivery(Row) == GroundCircle`, AND THAT TRAP
	 *  SITS RIGHT NEXT TO THIS FUNCTION: the `Auto` arm above returns
	 *  `GroundCircle` for EVERY effect that is not AoEDamage/Freeze — GoldSteal,
	 *  FogCover AND FogClear included — so a guard written that way would claim a
	 *  reticle for all three. ⭐ THE STRUCTURAL REASON: `ESpellDelivery` has no
	 *  value meaning "no aim at all". Delivery says WHICH aiming sentence; it can
	 *  never say WHETHER there is one. That is why this is a separate predicate
	 *  rather than a comparison (`SC-§75`(B): when no field can express the
	 *  negative answer, the honest repair is a NEW predicate).
	 *
	 *  ⛔ AND IT IS NOT A BLACKLIST. The shipped `!= ESpellEffect::GoldSteal`
	 *  guard was correct only until the next value and had ALREADY FAILED TWICE,
	 *  once per new no-reticle spell. This derivation is correct for a spell whose
	 *  enum value does not exist yet, and it fails CLOSED — an effect it cannot
	 *  place gets NO aim, which is a gap rather than a lie.
	 *
	 *  The derivation, in the same precedence order GetEffectiveDelivery uses:
	 *    1. an AUTHORED SpellDelivery cell — the per-card override lever the
	 *       column exists to be: an author who pins a delivery has DECLARED an
	 *       aim, and we agree with the data rather than second-guessing it;
	 *    2. otherwise the cell is `Auto`, and the row's own AIM EVIDENCE answers:
	 *       a ground-placed spell resolves INSIDE AoERadius, so a positive radius
	 *       IS the reticle's footprint and a zero radius means there is nothing on
	 *       the ground to place;
	 *    3. plus the line case, for a line spell whose corridor is its own rather
	 *       than the row's radius.
	 *  Pure row math — safe on any row, spell or not (a non-spell row simply
	 *  answers for its own columns; every caller gates on CardType/SpellEffect
	 *  before it ever asks).
	 */
	static bool SpellRequiresAiming(const FCardRow& Row);
};
