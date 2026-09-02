// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GitClaudeUnrealTestCharacter.h"
#include "Siegebound/HealthBarProvider.h"
// TASK-778 (CONTACT-§4.4 / CONTACT-§8): ILadderClimber is a BASE of the UCLASS below, so the
// complete type is required here — ⛔ a forward declaration cannot serve a base list. It is the
// seam AClimbableTower::EndPlay reaches this hero through when the tower dies mid-climb (exit
// H-10): MOVE_Flying ignores gravity, so a climber the tower cannot reach hangs in the air forever
// — and for the hero that is the PLAYER'S OWN BODY.
// ⭐ TASK-787 (CONTACT-§12.3): this header ALSO supplies `FSiegeLadderClimbEnded`, which moved here
// from `SummonedUnit.h` — so the by-value completion `UPROPERTY` below has its complete type
// through an include this class ALREADY had. ⛔ No new include, and ⛔ no heavy `SummonedUnit.h`
// dragged into this header (which is why a forward declaration was not good enough).
#include "Siegebound/LadderClimber.h"
// TASK-778 (CONTACT-§2): FSiegeLadderClimbState is held BY VALUE below (the complete-type include
// law, TASK-110). ⛔ The rules in that pair are CONSUMED UNCHANGED — ⛔ no second copy, ⛔ no
// hero-specific variant of any of them, and ⛔ nothing is added to that deliberately-unreflected pair.
#include "Siegebound/SiegeLadderClimbStatics.h"
#include "Siegebound/TeamId.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "HeroCharacter.generated.h"

class AClimbableTower;
class UAnimMontage;
class UCameraShakeBase;
class UDataTable;
class UCombatantHealthBarComponent;
class UInputAction;
class UInputMappingContext;
class UNiagaraComponent;
class UNiagaraSystem;
class USiegeHitFlashComponent;
class AHeroCharacter;

/**
 *  Broadcast exactly once each time the hero dies (HP reaches 0).
 *  The game mode (TASK-006) binds here to drive respawn timing — the hero
 *  itself knows nothing about respawn schedules (loose coupling, GDD §3.1).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroDied, AHeroCharacter*, DeadHero);

/**
 *  Broadcast on every Rally cooldown-state change (TASK-042, GDD §4):
 *  - on a successful Rally: (bReady=false, CooldownRemaining=RallyCooldown)
 *  - when the cooldown elapses:  (bReady=true,  CooldownRemaining=0)
 *  - (optional refusal) on a press during cooldown: (bReady=false, remaining).
 *  The HUD (TASK-050) binds here to drive the Rally readiness indicator; the hero
 *  itself owns no UI (loose coupling, mirrors OnHeroDied / the CONVENTIONS delegate law).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRallyStateChanged, bool, bReady, float, CooldownRemaining);

/**
 *  Broadcast whenever the hero's Instant-upgrade loadout changes (TASK-058, GDD §3.10/§7):
 *  on every ApplyUpgrade that adds a stack, on ResetUpgrades, and on respawn re-apply
 *  (ResetHero) so a freshly rebuilt HUD reflects the persisted upgrades. Carries the current
 *  stack count of each of the four Instant upgrades (0 = not owned). The §7 HUD icon row
 *  (TASK-064) seeds from the Get*Stacks getters FIRST, THEN binds here (seed-then-bind law,
 *  CONVENTIONS). Four plain int params keep it MCP-authorable — no enums/structs (the
 *  CONVENTIONS MCP-param rule); the per-upgrade cap for the pips comes from GetUpgradeStackCap.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHeroUpgradesChanged, int32, SharpenedBladeStacks, int32, PlateArmorStacks, int32, SwiftBootsStacks, int32, WarBannerStacks);

/**
 *  Result of AHeroCharacter::ApplyUpgrade (TASK-058, GDD §3.0 refund contract). The Instant
 *  play path (TASK-059) spends the card's Cost ONLY on Applied; any Refused* result means the
 *  play is refused with NO spend (full-refund rule, §3.10) — RefusedAtMaxStacks drives the
 *  "… at max stacks" OnCardRefused message, RefusedInvalidCard covers an unknown CardID or an
 *  unavailable DT_Cards row (the stack cap is never guessed).
 */
UENUM(BlueprintType)
enum class EHeroUpgradeResult : uint8
{
	/** A stack was added and the mods applied — the caller SPENDS the cost and draws a replacement. */
	Applied,
	/** Already at the card's MaxCopies cap — the caller REFUSES the play with no spend and the "at max stacks" message. */
	RefusedAtMaxStacks,
	/** Unknown upgrade CardID, or DT_Cards/its row unavailable (cap unresolved) — the caller REFUSES with no spend. */
	RefusedInvalidCard
};

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  RECALL — the 10-second channel home (TASK-748, CONVENTIONS `RECALL-§`)
//
//  Jonathan, verbatim (2026-09-01): "you can press 'b', and that will allow the player to
//  starting a recall animation similar to league of legends where after 10 seconds they
//  teleport back to their castle and they completely refill their health. During this recall
//  animation the player cannot attack and if they get hit with an attack it interupts the
//  channel and they would have to press 'b' again to start it from the beginning."
//
//  ⭐⭐ WHY THE CHANNEL'S EVERY DECISION LIVES IN `FSiegeRecallStatics` AND NOT IN THE ACTOR,
//  AND IT IS A MEASUREMENT RATHER THAN A STYLE CHOICE:
//    • Every automation test in this project is HEADLESS — there is not one `UWorld::CreateWorld`
//      and not one `SpawnActor` in `Siegebound/Tests/` (stated at `SiegeLadderClimbTest.cpp:39`).
//    • A world-less `AHeroCharacter` cannot be driven through these paths at all: the channel
//      reads the world clock, broadcasts dynamic delegates and spawns a component.
//    ⇒ so the RULES are pure statics (the `FSiegeLadderClimbStatics` / `FSiegeStuckStatics` /
//      `HeightAdvantageMultiplier` seam precedent — a testability obligation gets a testability
//      seam) and the WIRING of each call site is the QA gate's diff read (`SC-§32`: a mechanism
//      never observed to function is not known to function; that split is stated, not hidden).
// ═══════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ EVERY WAY A RUNNING RECALL CHANNEL CAN END — the SEVEN enumerated exits of `RECALL-§4`,
 *  in one closed list. `TOWER-§8`'s hanging-unit lesson generalises to any timed state: a
 *  channel whose exits are not enumerated WILL strand the player in one of them.
 *
 *  ⭐ EXACTLY ONE of these grants the arrival effects (`ExitGrantsArrival`). The other six end
 *  the channel with NOTHING applied — the hero stays where it stands at the HP it had.
 */
UENUM(BlueprintType)
enum class ESiegeRecallExit : uint8
{
	/** 1. The full RecallChannelSeconds elapsed uninterrupted ⇒ teleport home + heal to the effective max. */
	Completed,
	/** 2. The player re-pressed the recall key (`R-2`). ⛔ Never Escape — the channel adds NO key handler (`RECALL-§3`). */
	CancelledByInput,
	/** 3. The hero left its anchor (`R-2` — the League convention he invoked by name). Movement itself is NEVER restricted. */
	CancelledByMovement,
	/** 4. A `TakeDamage` that LANDED (> 0 after mitigation, `R-1`). A miss / blocked / 0-damage event does NOT reach here. */
	InterruptedByDamage,
	/** 5. The hero died (`GHOST-§ G-6`) — combat death or KillZ. A channel that survives its own caster is the hanging-unit class in a new costume. */
	InterruptedByDeath,
	/** 6. The match ended under the channel. Inherits the shipped match-end rule; ⛔ invents no second one. */
	CancelledByMatchEnd,
	/** 7. The actor left play (level teardown / travel / destroy). */
	CancelledByEndPlay
};

/**
 *  ⭐ THE COMPLETE, CLOSED LIST OF WHAT A RECALL ARRIVAL DOES — and it is TWO THINGS
 *  (`RECALL-§1`: *"Recall performs exactly two effects: the teleport, and the heal."*).
 *
 *  ⛔⛔ THIS STRUCT IS THE FIREBREAK IN FRONT OF THE BATCH'S SHARPEST TRAP. The death-path
 *  restore function on this class does FIVE further things — it re-applies the cumulative
 *  upgrade mods onto a freshly-restored base, re-arms the War Banner aura, resets the Rally
 *  cooldown, restores input that death disabled and re-broadcasts the upgrade loadout. On a
 *  LIVE hero that DOUBLE-APPLIES upgrades and re-arms a running aura. ⇒ NONE of those five is
 *  expressible in this payload, and `SiegeRecallTest.cpp` enumerates this struct's properties
 *  by reflection and fails the day a sixth field appears.
 */
USTRUCT(BlueprintType)
struct FSiegeRecallArrival
{
	GENERATED_BODY()

	/**
	 *  HP to write into CurrentHP on arrival, or 0 when this exit grants nothing.
	 *  ⛔⛔ SOURCED FROM `GetEffectiveMaxHP()` AT THE CALL SITE — the single source the header
	 *  names for *"every HP clamp / regen cap / full-heal"*. Reading the raw base field instead
	 *  silently under-heals a Plate-Armor hero by up to 200 HP and looks correct in review.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Recall")
	float HealTargetHP = 0.f;

	/**
	 *  True when the destination owner must be asked to teleport the hero home.
	 *  ⛔ The hero does NOT own the destination and must never learn it (see `OnHeroRecallArrived`).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Recall")
	bool bTeleportHome = false;
};

/**
 *  The channel's ENTIRE mutable state — ONE struct, so there is exactly ONE thing for each of
 *  the seven exits to clear, and clearing it is a single assignment that cannot half-succeed.
 *  ⛔ Deliberately NOT a timer handle: a timer is a second thing to leak, and `GHOST-§0`'s
 *  180-second respawn is the standing reminder that a leaked handle outlives its own feature.
 */
struct FSiegeRecallState
{
	/** True while the channel is running. ⭐ TRANSIENT per-instance state — ⛔ never a class-identity seal. */
	bool bChannelling = false;

	/** World time the CURRENT channel started. `R-4`: a re-press builds a fresh state, so this always restarts from zero. */
	double StartTimeSeconds = 0.0;

	/** Where the hero stood when the channel began — the movement-cancel datum (`R-2`). */
	FVector AnchorLocation = FVector::ZeroVector;
};

/**
 *  ⭐ THE CHANNEL'S RULES, AS PURE FUNCTIONS — no `UWorld`, no `AActor`, no `UObject`, no
 *  allocation, no clock read, no RNG. Every clock value and every location arrives as an
 *  argument, which is what lets `SiegeRecallTest.cpp` drive a 10-second channel to 9.9 s and
 *  to 10.0 s in a headless suite with no PIE session.
 *
 *  ⚠️ If a rule here ever starts needing a world, this purity has been broken and that is a
 *  FINDING, not a reason to add a fixture.
 */
struct FSiegeRecallStatics
{
	/** May a channel start? Alive, not already channelling, match still running. All three terms are load-bearing (see the test's truth table). */
	static bool CanBegin(const FSiegeRecallState& State, bool bDead, bool bMatchEnded);

	/** ⭐ `R-4` — a FRESH state from the clock, ⛔ never a resume and ⛔ never partial credit. His words: "start it from the beginning." */
	static FSiegeRecallState Begin(double NowSeconds, const FVector& AnchorLocation);

	/** The not-channelling state. The one value every exit assigns. */
	static FSiegeRecallState Cleared();

	/** Seconds the current channel has run (0 when not channelling; never negative even if the clock is handed back to it). */
	static float ElapsedSeconds(const FSiegeRecallState& State, double NowSeconds);

	/** Channel progress in [0, 1] — the HUD tell's datum (`R-3`). 0 when not channelling, and 0 on the frame a re-press restarts it. */
	static float Progress01(const FSiegeRecallState& State, double NowSeconds, float ChannelSeconds);

	/** True once the FULL ChannelSeconds have elapsed. At 9.9 s of a 10 s channel this is FALSE — which is the interrupt test's whole point. */
	static bool IsComplete(const FSiegeRecallState& State, double NowSeconds, float ChannelSeconds);

	/** `R-2` movement-cancel: the hero has left its anchor by more than the tolerance. ⛔ Movement is NEVER restricted — moving CANCELS, which is a different rule. */
	static bool HasLeftAnchor(const FSiegeRecallState& State, const FVector& CurrentLocation, float ToleranceUU);

	/** ⛔ `R-1` — ONLY DAMAGE THAT LANDS INTERRUPTS. A miss / blocked / 0-damage / friendly-fire event returns 0 applied and does NOT interrupt. */
	static bool DamageInterrupts(float AppliedDamage);

	/**
	 *  ⛔ `R-5` — the attack disarm, AS A FUNCTION OF STATE.
	 *  ⭐⭐ THAT SIGNATURE IS THE RULING: it takes the channel's state, so the same hero answers
	 *  TRUE and then FALSE ten seconds later. A permanent class-identity seal (a `const` "can
	 *  this thing ever attack" query) structurally cannot do that, and using one here would make
	 *  a 10-second channel indistinguishable from a unit that can never attack.
	 */
	static bool IsAttackDisarmed(const FSiegeRecallState& State);

	/**
	 *  ⭐ EXACTLY ONE of the seven exits grants the arrival, AND ONLY WHEN A DESTINATION OWNER
	 *  IS BOUND. The second term is the ATOMICITY ruling: with nobody to resolve the teleport
	 *  there is no teleport, and therefore NO HEAL — otherwise a missing integration would
	 *  quietly become a free full heal anywhere on the map, which is a worse bug than an inert key.
	 */
	static bool ExitGrantsArrival(ESiegeRecallExit Exit, bool bDestinationOwnerBound);

	/** The arrival payload for an exit. Non-granting exits yield {0, false} — nothing applied, hero unmoved, HP unchanged. */
	static FSiegeRecallArrival BuildArrival(ESiegeRecallExit Exit, bool bDestinationOwnerBound, float EffectiveMaxHP);
};

/**
 *  Broadcast the instant a recall channel COMPLETES — immediately before the heal (TASK-748).
 *
 *  ⭐ THE HERO DOES NOT OWN THE DESTINATION AND MUST NOT LEARN IT. `ASiegeGameMode` already
 *  ships the teleport-home rule (`SiegeGameMode.h:46-51`: the team-keyed PlayerStart when it
 *  lies on the hero's own side and outside its own castle's colliding bounds, else beside that
 *  castle offset toward the centerline). Recall is a CHANNEL IN FRONT OF A DESTINATION RULE
 *  THAT ALREADY EXISTS — it reuses that resolution instead of hand-typing a location, and this
 *  delegate is the seam, exactly as `OnHeroDied` is the seam for respawn TIMING (the loose-coupling
 *  law this class already ships). `handoffs/TASK-569-buildmaster.md` row (n) — "hero spawns
 *  OUTSIDE the keep" — is the recorded precedent for getting this exact thing wrong.
 *
 *  ⛔⛔ THE BINDER TELEPORTS AND DOES NOTHING ELSE. ⛔ IT MAY NEVER CALL THIS CLASS'S DEATH-PATH
 *  RESTORE FUNCTION: on a LIVE hero that double-applies every upgrade stack and re-arms a
 *  running aura. The heal is the hero's own and is already applied here.
 *
 *  ⚠️ NOT BOUND ⇒ NO ARRIVAL AT ALL (the atomicity ruling above): the channel ends, one warning
 *  is logged, and the hero is neither moved nor healed.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeroRecallArrived, AHeroCharacter*, RecallingHero);

/**
 *  Broadcast on every recall channel start and on every one of the seven ends (`R-3`, the
 *  visible tell). Carries whether a channel is now running and its full duration, so a HUD can
 *  seed a countdown ring without polling. Two plain params keep it MCP-authorable (the
 *  CONVENTIONS MCP-param rule) — ⛔ no enum, ⛔ no struct.
 *
 *  ⚠️ HONEST LIMIT, STATED SO NOBODY OVERSELLS IT: today's only opponent is `ASiegeBotController`,
 *  which does NOT look at this. The tell is for the HUMAN observer and costs nothing now — ⛔ it
 *  is not counterplay the bot exercises.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroRecallStateChanged, bool, bChannelling, float, ChannelSeconds);

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  THE LADDER CLIMB — the HERO's half (TASK-778, CONVENTIONS `CONTACT-§3`)
//
//  Jonathan, verbatim (2026-09-01): "lets just make sure the playable character and any units can
//  climb the ladder by simplying walking up to it and walking against it."
//
//  ⭐⭐ THIS IS NEW CONSTRUCTION, ⛔ NOT A REPAIR (`CONTACT-§1` C-1): the hero's climb was never
//  built, and it was ruled OUT on the record (`GHOST-§ G-7`: *"BeginLadderClimb is on
//  ASummonedUnit. The HERO ⛔ NEVER CLIMBS."*). His directive above OVERRULES that ruling. ⇒ there
//  is ⛔ no bug to hunt here; the rules module below it is shipped, tested and consumed UNCHANGED.
//
//  ⚠️⚠️ AND THE ONE THING THIS FEATURE CAN SHIP BROKEN: `MOVE_Flying` IGNORES GRAVITY. AN EXIT
//  THAT FAILS TO RESTORE THE MOVEMENT MODE HANGS THE PLAYER'S OWN BODY IN MID-AIR ***FOREVER***,
//  in a match that keeps running around it. ⚖️ The unit version of that bug costs 30 gold; the
//  hero version ends the match. That is why the exits are ENUMERATED (below), why every one of
//  them routes through ONE teardown, and why the hero arms a watchdog it does not otherwise have.
// ═══════════════════════════════════════════════════════════════════════════════════════════

/**
 *  ⛔ EVERY WAY THE HERO'S LADDER CLIMB CAN END — `CONTACT-§3.1`'s TEN EXITS, ENUMERATED
 *  ***FRESH*** AND ⛔ NOT MAPPED ACROSS FROM `ESiegeLadderExit` (which is `ASummonedUnit` driver
 *  vocabulary and stays in that class's header for exactly this reason — see the note in
 *  `SiegeLadderClimbStatics.h`). ⭐ The `ESiegeRecallExit` shape, second application on this class.
 *
 *  📌 HOW THE LAW'S TEN MAP ONTO THESE TEN ENUMERATORS — stated because it is ⛔ NOT one-to-one:
 *    H-1 Arrival · H-2 AbortLadderClimb · H-3 the player releases the climb input · H-4 death ·
 *    H-5 UnPossessed · H-6 respawn (ResetHero) · H-7 the recall channel's teleport · H-8 match end ·
 *    H-9 EndPlay · H-10 the TOWER dies mid-climb.
 *  ⚠️ **H-10 ARRIVES THROUGH `AbortLadderClimb()` AND THEREFORE REPORTS `Abort`** — deliberately:
 *  `ILadderClimber::AbortLadderClimb()` takes ⛔ NO reason (`TOWER-§8.4(B)`: the teardown is
 *  REASON-AGNOSTIC by design, and a reason parameter is the first line of exit bookkeeping). So
 *  H-2 and H-10 share one enumerator and the WATCHDOG contributes its own — ⇒ ten exits, ten
 *  enumerators, and ⛔ not one of them is unreachable dead vocabulary.
 *
 *  ⛔ NONE of these grants anything. Every one of them restores the movement mode, clears the
 *  climb state, kills the watchdog and lets the hero DROP from wherever it actually is —
 *  `TOWER-§8.5a` clause 5: a timed-out or aborted climb is ⛔ NEVER handed the deck it failed to
 *  reach. ✅ Dropping is free: there is ⛔ no fall damage anywhere in Siegebound (`TOWER-§4a`).
 */
UENUM(BlueprintType)
enum class ESiegeHeroLadderExit : uint8
{
	/** H-1. The capsule centre reached the far endpoint. ⭐ The ONLY exit that reports bReachedTop, and the ONLY one permitted the arrival snap (`TOWER-§8.5a` clause 5). */
	Arrival,
	/** H-2 — and H-10. `AbortLadderClimb()`, from anywhere: a blueprint, a caller, or `AClimbableTower::EndPlay` reaching in through `ILadderClimber` when the tower dies under the climber. */
	Abort,
	/** H-3. ⭐ THE HERO'S MOST FREQUENT EXIT AND THE ONE A UNIT DOES ⛔ NOT HAVE (`K-B` hold-to-climb): the player stopped pressing INTO the ladder, so the climb ends and the body drops. */
	InputReleased,
	/** H-4. `HandleDeath` — combat death or KillZ. ⚠️ `DisableMovement()` there covers the MODE; it does ⛔ nothing about the state, the driver or the watchdog, which is why this exit exists. */
	Death,
	/** H-5. ⛔⛔ THE NASTIEST ONE, AND A SEPARATE SEAM FROM H-4: a hero that dies mid-climb is un-possessed INTO THE GHOST while still in `MOVE_Flying`. An INDEPENDENT belt. */
	Unpossessed,
	/** H-6. `ResetHero` — the respawn backstop. ⚠️ ⛔ NOT relied on: it is 180 s downstream of the death (`GHOST-§0`), so a hero rescued only here has been broken for three minutes. */
	Respawn,
	/** H-7. The recall channel COMPLETED and the destination owner is about to teleport the hero home. ⚠️ Near-unreachable in practice (a climb cancels a running recall by POSITION within ~0.07 s — `CONTACT-§3.4`), and ⛔ kept anyway: an exit you cannot reach is free; an exit you removed is a hang. */
	RecallArrival,
	/** H-8. The match ended under the climb. ⛔ The hero has ⛔ NO `bAIFrozen`; the shipped equivalent is `IsMatchOver()`, the SAME rule the recall channel's own match-end exit reads. */
	MatchEnd,
	/** H-9. The actor is leaving play (level travel, teardown, destroy). */
	EndPlay,
	/** ⚠️ ⛔ NOT one of the ten — `TOWER-§8.5a` clause 7's LOUD failure: the climb did not arrive inside its own budget, so the hero is DROPPED with a warning rather than left hanging. The only exit that logs. */
	Watchdog
};

/**
 *  Siegebound player hero (GDD §3.1).
 *
 *  Subclasses the abstract third-person template character to inherit the
 *  camera boom and Move/Look/Jump plumbing (template files untouched).
 *
 *  - Team Blue by default; implements ITeamAgent (shared friendly-fire check).
 *  - Walks at 500 u/s, sprints at 750 u/s while IA_Sprint is held.
 *  - Melee on IA_Attack: MeleeDamage to ALL enemy ITeamAgent actors within
 *    MeleeRange AND inside a ±MeleeHalfAngleDegrees forward cone, rate-limited
 *    to one swing per MeleeCooldown seconds. No friendly fire.
 *  - Attack feedback (TASK-016, playtest R1): AttackMontage on every
 *    non-suppressed swing that passes the cooldown (hit or whiff), HitImpactEffect
 *    per enemy actually damaged, HitCameraShake once per swing that damaged >= 1
 *    enemy. Purely visual — damage timing/numbers never depend on any of it; all
 *    four assets are optional (wired on BP_HeroCharacter in TASK-017).
 *  - 200 max HP; regenerates RegenRate HP/s starting RegenDelay seconds after
 *    last taking OR dealing damage, stopping at max.
 *  - At 0 HP: hidden, input + collision disabled, OnHeroDied broadcast once.
 *    ResetHero() restores the hero (called by the game mode on respawn/Play Again).
 *  - Falling past the world's KillZ (TASK-036 arena boundary) is a DEATH, not
 *    a Destroy: FellOutOfWorld routes into the same path as lethal damage, so
 *    the standard 5 s respawn brings the hero back (TASK-024, M1 carry-over).
 *  - SetMeleeSuppressed(true) disables melee while the placement mode owns
 *    the LMB (TASK-007).
 *  - RECALL (TASK-748, GDD-adjacent — CONVENTIONS `RECALL-§`): IA_Recall starts a
 *    RecallChannelSeconds channel; on completion the destination owner teleports
 *    the hero home (OnHeroRecallArrived) and the hero heals to its EFFECTIVE max.
 *    Melee is disarmed for the duration by a state term at the EXISTING melee
 *    guard — ⛔ not a new suppression mechanism and ⛔ not a fourth guard point.
 *    Movement is NOT restricted; moving CANCELS. Seven exits, one clear.
 *
 *  Input assets (IMC_Hero, IA_Sprint, IA_Attack) are assigned on the derived
 *  blueprint BP_HeroCharacter in TASK-009; every input reference is null-safe
 *  so the raw C++ class also runs (game mode fallback pawn, TASK-006).
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  📌 M8 DECLARATION FOR THE RECALL CHANNEL (`RECALL-§6`) — DECLARED, ⛔ NOT BUILT.
 *  ⛔ "Nothing to declare" is FALSE here and this is NOT copied boilerplate: the
 *  channel is authority-relevant state (it ends in a teleport and a full heal) and
 *  `R-3` names an ENEMY-VISIBLE tell, so it acquires a replication design the day
 *  M8 combat authority lands. The RESERVED shape, in `ACC-§8`'s reserved-not-authored
 *  discipline (⛔ no RPC and ⛔ no replicated property is AUTHORED in this batch):
 *    • `ServerBeginRecall()` / `ServerCancelRecall()` — Server, Reliable, WithValidation.
 *      The client PREDICTS the local tell; the server owns the clock, the teleport and
 *      the heal. ⛔ A client-authored heal is the flicker-then-snap lie the melee
 *      authority gate (`DoMeleeAttack`'s HasAuthority early-out) already refuses.
 *    • Replicated `bRecallChannelling` + `RecallStartTimeSeconds` (server clock), so a
 *      remote observer can draw `R-3`'s tell from the same two values the local HUD uses.
 *    • RELEVANCY TIER: ⛔ NO new tier — it rides the hero's existing Tier B
 *      (`SetNetCullDistanceSquared` from `SiegeNet::ArenaRelevancyDistanceSquared`).
 *      An enemy already in relevancy to be seen is already in relevancy to be seen
 *      CHANNELLING; the tell needs no extra reach.
 *    • ⚠️ KNOWN P1 GAP, NAMED RATHER THAN PAPERED OVER: the match-end exit reads
 *      `UWorld::GetAuthGameMode()`, which is NULL on a client. On a listen-server host
 *      and in standalone (every shipped configuration today) it is authoritative; a
 *      future client build must move that exit onto replicated match state.
 *  ─────────────────────────────────────────────────────────────────────────────
 */
UCLASS()
class GITCLAUDEUNREALTEST_API AHeroCharacter : public AGitClaudeUnrealTestCharacter, public ITeamAgent, public IHealthBarProvider, public ILadderClimber
{
	GENERATED_BODY()

public:

	AHeroCharacter();

	/** Fired exactly once per death. The game mode (TASK-006) binds here for respawn timing. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnHeroDied OnHeroDied;

	/** Fired on every Rally cooldown-state change (TASK-042). The HUD (TASK-050) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnRallyStateChanged OnRallyStateChanged;

	/** Fired whenever the Instant-upgrade loadout changes (TASK-058). The §7 HUD row (TASK-064) binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnHeroUpgradesChanged OnHeroUpgradesChanged;

	/**
	 *  Fired exactly once per COMPLETED recall, immediately before the heal (TASK-748).
	 *  ⭐ THE DESTINATION OWNER BINDS HERE and teleports this hero to the start it already
	 *  resolves for respawn — see FOnHeroRecallArrived's own comment for the full contract and
	 *  for the one thing the binder may never do. ⚠️ Nothing bound ⇒ no teleport AND no heal.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Recall")
	FOnHeroRecallArrived OnHeroRecallArrived;

	/** Fired on every recall start and on every one of the seven ends (`R-3`). The HUD binds here for the channel tell — mirrors OnRallyStateChanged. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Recall")
	FOnHeroRecallStateChanged OnRecallStateChanged;

	//~ Begin ITeamAgent Interface
	virtual ETeamId GetTeamId() const override { return Team; }
	//~ End ITeamAgent Interface

	/** Fired on every ACTUAL HP change (spawn-init, regen, damage, death, respawn, Plate-Armor upgrade/reset) — drives the overhead bar (UCombatantHealthBarWidget) via the castle-parity PUSH model (TASK-130, mirrors FOnCastleHPChanged). Additive to the M1 WBP_HUD HP. */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero")
	FOnCombatantHPChanged OnHPChanged;

	//~ Begin IHealthBarProvider Interface (TASK-130 push model) — forwards to the EXISTING getters + the OnHPChanged delegate; adds NO HP state. GetMaxHP() already returns the EFFECTIVE max (Plate Armor composed).
	virtual FOnCombatantHPChanged& GetHPChangedDelegate() override { return OnHPChanged; }
	virtual float GetHealthCurrent() const override { return GetCurrentHP(); }
	virtual float GetHealthMax() const override { return GetMaxHP(); }
	virtual bool IsHealthBarActorAlive() const override { return !IsDead(); }
	//~ End IHealthBarProvider Interface

	/** Applies incoming damage (no friendly fire), tracks combat time for regen, and triggers death at 0 HP. */
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/**
	 *  KillZ handler (M1 "sprints off the slab and falls forever" carry-over,
	 *  closed by TASK-024; TASK-036 sets L_Arena's KillZ = -2000 and the
	 *  boundary volumes). A hero falling out of the world dies through the
	 *  EXACT combat-death path — HandleDeath() hides it, stops movement and
	 *  input, and broadcasts OnHeroDied once, so the game mode's standard 5 s
	 *  respawn (§3.1) brings it back at its castle. Deliberately does NOT call
	 *  Super: AActor::FellOutOfWorld() would Destroy() the pawn, and the hero
	 *  must survive falling off the world. The engine re-checks per movement
	 *  tick while an actor sits below KillZ, so repeat calls on the hidden
	 *  corpse early-out on the death latch until the respawn teleports it back
	 *  above ground (or PlayAgain does, if the match has ended).
	 */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	/**
	 *  Performs the melee swing if allowed (alive, not suppressed, off cooldown):
	 *  MeleeDamage to every enemy ITeamAgent within MeleeRange and inside the
	 *  ±MeleeHalfAngleDegrees forward cone. Bound to IA_Attack; also callable
	 *  from blueprint/UI for testing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void DoMeleeAttack();

	/**
	 *  While true, DoMeleeAttack is a no-op (does not even consume the cooldown).
	 *  TASK-007's placement mode sets this while the LMB confirms card placement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void SetMeleeSuppressed(bool bSuppressed) { bMeleeSuppressed = bSuppressed; }

	/** True while placement mode (TASK-007) owns the LMB. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
	bool IsMeleeSuppressed() const { return bMeleeSuppressed; }

	/**
	 *  Hero active ability (GDD §4, bound to IA_Rally in TASK-048; also callable from
	 *  blueprint/UI). If off cooldown: buffs every friendly (same-team) ASummonedUnit
	 *  within RallyRadius by +RallySpeedBonus move speed for RallyDuration seconds
	 *  (via ApplyMoveSpeedBuff), starts the RallyCooldown, and broadcasts
	 *  OnRallyStateChanged(false, RallyCooldown); a second broadcast (true, 0) fires
	 *  when the cooldown elapses. Does NOTHING to the hero's own speed and never
	 *  touches enemy units. A press while on cooldown is a no-op (optional refusal
	 *  broadcast). No-op while dead. Null-safe with no world/units present.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Combat")
	void Rally();

	/**
	 *  Restores the hero to a playable state after death or Play Again (GDD §3.9):
	 *  full HP, visible, collision + movement + input re-enabled.
	 *  Respawn timing and placement belong to the game mode (TASK-006) — it moves
	 *  the hero (or respawns the pawn) and then calls this.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	void ResetHero();

	/**
	 *  Applies one stack of an Instant hero upgrade (GDD §3.10/§4, TASK-058) — called by the
	 *  Instant play path (TASK-059) with the card's CardID. The stack cap is the card's MaxCopies
	 *  read from DT_Cards (never guessed). On success the per-stack mod is applied on top of the
	 *  IMMUTABLE base stats (every bonus derives live from the stack count — drift-free):
	 *    - SharpenedBlade → +MeleeDamageBonus melee damage per stack (cap 2): cone melee 20→30→40.
	 *    - PlateArmor     → +MaxHPBonus max HP per stack AND an immediate heal of MaxHPBonus (cap 2): 200→300→400.
	 *    - SwiftBoots     → +MoveSpeedBonus fractional move speed to BOTH walk and sprint (cap 1).
	 *    - WarBanner      → enables the friendly-unit damage aura (cap 1): a pulse timer that calls
	 *                       ASummonedUnit::SetAuraDamageBonus(WarBannerDamageBonus, …) on same-team
	 *                       units within WarBannerAuraRadius (TASK-055 / mirrors Rally's TASK-042 loop).
	 *  Returns EHeroUpgradeResult so the caller can REFUND an over-cap or invalid play with NO spend
	 *  (§3.0/§3.10). Works whether the hero is alive or dead (the persistent stack is always added);
	 *  alive-only side effects — the Plate Armor heal, the active aura pulsing — are deferred to
	 *  respawn (ResetHero re-applies them). Broadcasts OnHeroUpgradesChanged on Applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	EHeroUpgradeResult ApplyUpgrade(FName UpgradeCardID);

	/**
	 *  Clears ALL upgrade stacks and mods back to the base hero, ends the War Banner aura, and
	 *  broadcasts OnHeroUpgradesChanged (TASK-058, GDD §3.9). Upgrades PERSIST through hero death
	 *  (ResetHero re-applies the cumulative mods on respawn); they RESET only here, on match end /
	 *  Play Again. INTEGRATION: the match-reset owner (ASiegeGameMode::PlayAgain) must call this on
	 *  the hero — see handoffs/TASK-058.md (kept out of the game mode per this task's files-only scope).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero")
	void ResetUpgrades();

	/** Current hit points, in [0, effective MaxHP]. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetCurrentHP() const { return CurrentHP; }

	/** EFFECTIVE maximum hit points: base MaxHP (§3.1: 200) + Plate Armor bonus (§3.10). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	float GetMaxHP() const { return GetEffectiveMaxHP(); }

	/** EFFECTIVE melee damage per swing: base MeleeDamage (§3.1: 20) + Sharpened Blade bonus (§3.10). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Combat")
	float GetEffectiveMeleeDamage() const { return MeleeDamage + (MeleeDamageBonus * SharpenedBladeStacks); }

	/**
	 *  EFFECTIVE base walk speed in u/s: base WalkSpeed (§3.1: 500) scaled by the Swift Boots
	 *  bonus (§3.10). The PUBLIC read of the protected GetEffectiveWalkSpeed() — exactly the
	 *  GetMaxHP()/GetEffectiveMaxHP() pairing above, for the speed half of the stat block.
	 *  ⛔ NOT unused: ASiegeGhostPawn::BeginPlay derives the ghost's MaxWalkSpeed from this on
	 *  the CDO (GHOST-§3 G-1 "the hero's OWN speed"), and SiegeGhostPawnTest test (d) asserts it
	 *  is > 0. Deleting it as dead code re-duplicates the 500.f literal it exists to prevent.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Movement")
	float GetWalkSpeed() const { return GetEffectiveWalkSpeed(); }

	/** Current stacks of Sharpened Blade (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetSharpenedBladeStacks() const { return SharpenedBladeStacks; }

	/** Current stacks of Plate Armor (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetPlateArmorStacks() const { return PlateArmorStacks; }

	/** Current stacks of Swift Boots (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetSwiftBootsStacks() const { return SwiftBootsStacks; }

	/** Current stacks of War Banner (0..cap). HUD-seed getter (TASK-064). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetWarBannerStacks() const { return WarBannerStacks; }

	/** Current stacks of the given upgrade CardID, or 0 for a non-upgrade CardID (TASK-064 generic seed). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetUpgradeStackCount(FName UpgradeCardID) const;

	/** Stack cap (MaxCopies from DT_Cards) for the given upgrade CardID; 0 if unknown/unavailable (TASK-064 pip cap). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	int32 GetUpgradeStackCap(FName UpgradeCardID) const;

	/** True from the moment HP hits 0 until ResetHero() is called. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero")
	bool IsDead() const { return bDead; }

	//~ ─── RECALL (TASK-748, `RECALL-§`) — the 10-second channel home ────────────────────────

	/**
	 *  THE RECALL KEY'S ONE ENTRY POINT — bound to IA_Recall (`B`, TASK-747) and callable from
	 *  blueprint/UI. Channelling ⇒ CANCEL (`R-2`, his deliberate exit); otherwise ⇒ BEGIN.
	 *
	 *  ⛔⛔ THE KEY IS NEVER NAMED IN CODE. IA_Recall is mapped to `B` inside IMC_Hero, and the
	 *  whole context goes through `USiegeKeyboardLayoutSubsystem::GetPositionalContext` in
	 *  NotifyControllerChanged — so `B` inherits Dvorak support with ZERO extra code
	 *  (`KBD-§4` tables all 26 letters). ⛔ There is no hard-coded key constant for `B` anywhere
	 *  on a shipped path — the test asserts the engine's key-constant namespace appears in
	 *  neither of this class's two files — and
	 *  ⛔ the subsystem's SINGLE-KEY positional lookup is NOT called for it: that API is for RAW POLLED keys, and calling
	 *  it on a key already remapped inside the context would DOUBLE-TRANSLATE
	 *  (`SiegePlayerController.h:1223-1225`) — wrong only on non-QWERTY layouts, i.e. invisible
	 *  to every reviewer here and immediately visible to Jonathan.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Recall")
	void HandleRecallInput();

	/**
	 *  Starts the channel from ZERO if allowed (alive, not already channelling, match running).
	 *  Returns true when a channel actually started. ⛔ `R-4`: a re-press ALWAYS restarts from
	 *  the beginning — there is no resume and no partial credit (his words).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Recall")
	bool BeginRecall();

	/** The player's deliberate cancel (`R-2`). A no-op when no channel is running. ⛔ Escape is NOT a cancel route here and the channel adds no key handler at all (`RECALL-§3`). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Recall")
	void CancelRecall();

	/** True while the channel is running. ⭐ TRANSIENT state — this is the same hero that could attack a moment ago and can again the instant it ends. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Recall")
	bool IsRecalling() const { return RecallState.bChannelling; }

	/** The channel's full duration in seconds — the value the `HELP-§` row DERIVES rather than restating in prose. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Recall")
	float GetRecallChannelSeconds() const { return RecallChannelSeconds; }

	/** Channel progress in [0, 1] for the `R-3` tell (0 when not channelling). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Recall")
	float GetRecallProgress01() const;

	/** Seconds left before arrival (0 when not channelling). */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Recall")
	float GetRecallRemainingSeconds() const;

	//~ ─── THE LADDER CLIMB (TASK-778, `CONTACT-§3`) ─────────────────────────────────────────
	//~
	//~ 📌 M8 DECLARATION FOR THE CLIMB (`CONTACT-§9`) — DECLARED, ⛔ NOT BUILT. ⛔ "Nothing to
	//~ declare" is FALSE here and this is ⛔ NOT copied from another batch (`WR-§8`'s standing
	//~ warning): a climb is AUTHORITY-RELEVANT MOVEMENT STATE on a PLAYER-POSSESSED pawn — it
	//~ writes the movement mode, it drives the capsule non-swept through a deck slab, and it
	//~ disarms the melee guard. The RESERVED shape, in `ACC-§8`'s reserved-not-authored discipline
	//~ (⛔ no RPC and ⛔ no replicated property is AUTHORED here):
	//~   • `ServerBeginLadderClimb(FVector From, FVector To)` / `ServerAbortLadderClimb()` —
	//~     Server, Reliable, WithValidation. The client PREDICTS the ascent locally; the server
	//~     owns the admission (team, occupancy), the drive and every exit. ⛔ A client-authored
	//~     climb is a client-authored TELEPORT ONTO A DEFENDED DECK — strictly worse than the
	//~     client-authored heal the melee authority gate already refuses.
	//~   • Replicated `bLadderClimbing` + the two endpoints, so a remote observer draws the same
	//~     ascent from the same line rather than from a corrected position stream.
	//~   • RELEVANCY TIER: ⛔ NO new tier — it rides the hero's existing Tier B. A climber 1,200 uu
	//~     up is no further away than the tower it is on, which is already relevant.
	//~   • ⚠️ KNOWN P1 GAP, NAMED RATHER THAN PAPERED OVER: exit H-8 reads
	//~     `UWorld::GetAuthGameMode()`, which is NULL on a client — the identical gap the recall
	//~     channel's match-end exit already carries, and it moves onto replicated match state in
	//~     the same P1 pass rather than acquiring a second, divergent answer here.
	//~ ⭐ `FSiegeLadderClimbState` is deliberately UNREFLECTED, so ⛔ nothing in it can be
	//~ replicated by accident even after a later refactor — a STRUCTURAL guarantee, not a promise.
	//~   • ⭐ AND THE ONE MEMBER TASK-787 ADDED IS COVERED HERE RATHER THAN LEFT UNSAID
	//~     (`CONTACT-§9`): `OnLadderClimbEnded` is a LOCAL event, ⛔ not replicated state. A
	//~     multicast delegate cannot be a replicated property at all, and it is ⛔ not in
	//~     `GetLifetimeReplicatedProps`. In the P1 shape above the SERVER owns the climb and its
	//~     exits, so it is the server's broadcast that releases the tower's slot; a client's copy
	//~     fires locally off the replicated `bLadderClimbing` edge and ⛔ authorises nothing.

	/**
	 *  Drives a scripted traversal along the ONE straight world-space segment FromWorld -> ToWorld,
	 *  in `MOVE_Flying`, at the SHIPPED ladder rate. Returns false and changes ⛔ NOTHING when the
	 *  hero is dead, match-end frozen, RECALL-CHANNELLING, or already climbing.
	 *
	 *  ⭐ THE SHAPE IS `TOWER-§8.4(B)`'s PINNED CONTRACT, re-declared on this class because
	 *  `CONTACT-§2` ruled a PER-CLASS DRIVER (⛔ not a component, ⛔ not a shared base class): the
	 *  EXITS are the risk surface and they stay per-class, so only the RULES are shared.
	 *
	 *  ⛔ NO Z-ORDERING IS ENFORCED OR IMPLIED, AND THAT IS THE CONTRACT: the link is BothWays
	 *  (`TOWER-§8.7`), so a DESCENT passes the same two points the other way round. Which end is
	 *  the deck is resolved by Z inside (`TOWER-§8.5a` clause 1), ⛔ never by argument order.
	 *  ⛔ Do not "fix" this into Foot-then-Top.
	 *
	 *  ⭐ The two points are SURFACE positions (the mesh's sockets, which sit on generated
	 *  navmesh); the driver lifts BOTH by this hero's OWN `GetScaledCapsuleHalfHeight()` so it
	 *  finishes standing ON the deck rather than buried one half-height inside it. ⛔ The lift is
	 *  RE-DERIVED from the capsule, ⛔ never inherited as a number from the unit and ⛔ never a
	 *  literal (`CONTACT-§3.3` #1: the hero's capsule is 96, the unit's is 88).
	 *
	 *  ⚠️ FIVE REFUSAL REASONS, ⛔ NOT THE PINNED FOUR — the fifth is `CONTACT-§3.4` bullet 1: a
	 *  hero may ⛔ NOT START a climb while a recall channel is running. It is stated here because
	 *  a contract that quietly grew a reason is how a caller learns the wrong rule.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero|Climb")
	bool BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld);

	//~ Begin ILadderClimber Interface (TASK-777's capability seam — `CONTACT-§4.4`)

	/**
	 *  Ends an in-flight climb WHEREVER the hero is: restores the movement mode, clears the state,
	 *  kills the watchdog and lets the body drop. Idempotent (the teardown's exactly-once latch).
	 *
	 *  ⭐ THIS IS ALSO THE TOWER-DESTROYED PATH (exit H-10, `TOWER-§10 L-5`): `AClimbableTower::
	 *  EndPlay` calls it on the climber it started, because a `MOVE_Flying` hero does ⛔ NOT fall
	 *  when the floor disappears. ✅ It then falls and survives — ⛔ no fall damage (`TOWER-§4a`).
	 *
	 *  ⭐ The `UFUNCTION` is this class's OWN, ⛔ not a re-declaration of a parent's (the
	 *  `handoffs/TASK-028.md` rule): `ILadderClimber`'s methods are deliberately plain C++
	 *  pure-virtuals with ⛔ no reflection, exactly as `IHealthBarProvider`'s are — so both
	 *  implementers are free to expose them, and `ASummonedUnit` does the same.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Hero|Climb")
	virtual void AbortLadderClimb() override;

	/**
	 *  True for the whole traversal, false at every one of the ten exits.
	 *
	 *  ⛔⛔ THIS IS ALSO THE `K-4` DISARM TERM, and it is read at the hero's ONE SHIPPED melee
	 *  guard point (`DoMeleeAttack`'s existing early-out) — ⛔ never a new suppression mechanism
	 *  and ⛔ never a fourth guard point (the `TOWER-§9.2` / `RECALL-§ R-5` idiom, third
	 *  application). One bool backs the climb AND the disarm, so "the disarm ends the INSTANT the
	 *  hero reaches the deck" is true by construction rather than by discipline.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Hero|Climb")
	virtual bool IsClimbing() const override;

	/**
	 *  ⭐⭐ `ILadderClimber`'s COMPLETION ACCESSOR (TASK-787, `CONTACT-§12.3`) — how
	 *  `AClimbableTower` binds and unbinds WITHOUT naming this class, and ⛔ THE HALF OF THE
	 *  WIDENING THAT KEEPS THE LADDER FROM BRICKING.
	 *
	 *  ⚠️⚠️ BEFORE THIS EXISTED, A HERO ADMITTED TO THE OCCUPANCY SLOT COULD ⛔ NEVER RELEASE IT:
	 *  the tower learns a climb ended through ⛔ exactly one channel, and this class had ⛔ no
	 *  completion delegate at all (its own teardown said so in-source). ⇒ the FIRST hero attempt
	 *  would have disabled that tower for EVERYONE — hero and unit alike — for the rest of the
	 *  match (`CONTACT-§12.1`).
	 *
	 *  ⚠️ INLINE, and ⛔ NOT a `UFUNCTION`: a reflected function cannot return a delegate reference,
	 *  and `ILadderClimber`'s methods are plain C++ pure virtuals by design. ⭐ Mirrors
	 *  `ASummonedUnit`'s accessor exactly — ⛔ one shape for both climbers.
	 *
	 *  ✅⭐⭐ AND IT IS THE SHAPE ⛔ THIS CLASS ALREADY SHIPS, ⛔ NOT A NEW ONE: `GetHPChangedDelegate()`
	 *  earlier in this same class (`HeroCharacter.h:437`/`:440`) is `IHealthBarProvider`'s identical accessor over
	 *  `FOnCombatantHPChanged`, whose declaration likewise lives in the INTERFACE's header. ⇒ the
	 *  pattern `CONTACT-§12.3` ordered is one this very class has compiled since M2.
	 */
	virtual FSiegeLadderClimbEnded& GetOnLadderClimbEnded() override { return OnLadderClimbEnded; }

	//~ End ILadderClimber Interface

	/**
	 *  Broadcast EXACTLY ONCE per successful `BeginLadderClimb`, from inside the ONE teardown all
	 *  TEN exits route through — so the exactly-once property is ⛔ INHERITED from the
	 *  ten-exits-one-teardown design (TASK-778) rather than latched a second time here.
	 *
	 *  ⭐ THE TOWER'S ONLY COMPLETION SIGNAL (`CONTACT-§12.3`), and the ⛔ SAME TYPE the unit
	 *  broadcasts — `FSiegeLadderClimbEnded`, which lives in `LadderClimber.h`. ⛔ A second,
	 *  hero-only delegate type was REFUSED (`CONTACT-§12.4`): one type serves both pawns, which is
	 *  precisely what `CONTACT-§4.4` widened the first parameter to `ACharacter*` for.
	 *
	 *  ⚠️ BROADCAST LAST, AFTER THE MOVEMENT MODE IS RESTORED AND AFTER THE EXACTLY-ONCE LATCH IS
	 *  CONSUMED — the unit's own pinned ordering. A listener that inspects the hero (or hands it
	 *  back to path following) must never see it mid-teardown, and one that re-enters
	 *  `AbortLadderClimb()` from inside the delegate is then inert rather than recursive.
	 *
	 *  ⭐ `BlueprintAssignable`, mirroring `ASummonedUnit::OnLadderClimbEnded`'s specifier exactly;
	 *  only the Category is this class's.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Siegebound|Hero|Climb")
	FSiegeLadderClimbEnded OnLadderClimbEnded;

	/**
	 *  ⭐⭐ THE HOLD-TO-CLIMB SEAM, AND IT IS A SHIPPED TEMPLATE VIRTUAL RATHER THAN A NEW
	 *  MECHANISM: while a climb runs this CAPTURES the player's steer and ⛔ does NOT forward it
	 *  to the movement component; otherwise it is `Super` byte-for-byte.
	 *
	 *  ⛔⛔ TWO THINGS AT ONCE, AND BOTH ARE REQUIRED:
	 *    (1) ONE STEERING AUTHORITY (the `NAV-§3` no-double-driver law, applied to the hero). The
	 *        template's `DoMove` calls `AddMovementInput` twice; in `MOVE_Flying` that is
	 *        UNCONSTRAINED 3D flight, so the player's steer would fight the climb driver and pull
	 *        the capsule OFF the pinned line — ⛔ not a cosmetic drift: off the line there is no
	 *        deck-breach window and no arrival.
	 *    (2) The exit `H-3` READ. `K-B` says the climb runs while the pawn keeps supplying input
	 *        INTO the ladder — so the steer must still be OBSERVED after it is suppressed, and
	 *        this is the only place it exists before the movement component eats it.
	 *  ⛔ It reads a DIRECTION and ⛔ never a key (`CONTACT-§4.1`'s INTENT term, same discipline):
	 *  ⛔ no key constant, ⛔ no new binding, ⛔ no `Escape` handler (`AS-§6` A-2).
	 */
	virtual void DoMove(float Right, float Forward) override;

protected:

	virtual void BeginPlay() override;

	/** Recall exit 7 of 7 (`RECALL-§4`): a channel running when the actor leaves play is cleared here — the tell is torn down and no arrival fires. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 *  M8 hero team assignment (TASK-356 doc §2.3/D4 — closes audit §1b#4:
	 *  Team was NEVER assigned): server-side by engine contract. Resolves the
	 *  possessing controller's ASiegePlayerState → Team (warn + keep-default when
	 *  unresolvable), then RE-STAMPS the capsule's CASTLE-3X team channel — the
	 *  server-side BeginPlay stamp ran BEFORE possession (SpawnDefaultPawnFor
	 *  begins play, then Possess) with the default Blue; idempotent for the host.
	 *  Runs on every possession (respawn repossess + Play-Again included).
	 *  SINGLE seam: SetPlayerDefaults does NOT also write Team (no double-writer).
	 *  Standalone: the one PS is Blue ⇒ Team stays Blue, the re-stamp re-applies
	 *  the identical channel (doc §10).
	 */
	virtual void PossessedBy(AController* NewController) override;

	/**
	 *  ⚖️ NET RELEVANCY TIER: **B — arena-scaled `SetNetCullDistanceSquared`
	 *  from `SiegeNet::ArenaRelevancyDistanceSquared`** (declared per the
	 *  CONVENTIONS NET RELEVANCY LAW declaration duty; set in the constructor,
	 *  never a hand-typed literal). Rationale: the pawn replicates already
	 *  (APawn ctor); a player's OWN hero is owner-relevant regardless, but the
	 *  ENEMY hero must remain relevant across the full 500 m arena — both to be
	 *  seen and so its replicated `Team` (the CASTLE-3X gate-channel truth)
	 *  arrives. Tier B, not A: heroes are per-player and P2 adds the unit fleet
	 *  to the same tier, where a blanket always-relevant would not scale.
	 */

	/** Registers Team (the one P1 hero property — doc §3.6; the rest of hero replication is P2). */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 *  CLIENT team arrival (M8, TASK-356 — the addendum §1 resolution of the doc
	 *  §7.1 reserved seam): log + CAPSULE RE-STAMP. The client proxy's BeginPlay
	 *  stamps whatever Team it holds at that moment; the normal path carries the
	 *  correct value in the initial bunch, but any ordering edge (PIE login
	 *  timing, late correction) would leave the predicted hero on the WRONG
	 *  CASTLE-3X channel — a live gate-truth defect. Re-stamping here closes the
	 *  class unconditionally: whichever of {proxy BeginPlay, Team rep} lands
	 *  second, the channel ends correct. Idempotent.
	 */
	UFUNCTION()
	void OnRep_Team();

	/** Out-of-combat regen (GDD §3.1): RegenRate HP/s once RegenDelay seconds have passed since last combat. */
	virtual void Tick(float DeltaSeconds) override;

	/** Adds HeroMappingContext to the enhanced input subsystem (null-safe) when possessed by a player. */
	virtual void NotifyControllerChanged() override;

	/** Binds Sprint/Attack on top of the template's Jump/Move/Look bindings. All bindings null-safe. */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Sprint input pressed: raise max walk speed to SprintSpeed. */
	void StartSprint();

	/** Sprint input released/canceled: return max walk speed to WalkSpeed. */
	void StopSprint();

	/** Kills the hero exactly once: hide, disable input/collision/movement, broadcast OnHeroDied. */
	void HandleDeath();

	/** Rally cooldown-timer callback (TASK-042): broadcasts OnRallyStateChanged(true, 0) — Rally usable again. */
	void OnRallyReady();

	/** True when the damage is attributable to the hero's own team (DamageCauser first, then EventInstigator's pawn). */
	bool IsFriendlyDamage(AController* EventInstigator, AActor* DamageCauser) const;

	//~ Hero upgrades (TASK-058) — base stats are IMMUTABLE; every bonus derives live from a stack count.

	/** EFFECTIVE max HP: base MaxHP + PlateArmor bonus. The single source used by every HP clamp / regen cap / full-heal. */
	float GetEffectiveMaxHP() const { return MaxHP + (MaxHPBonus * PlateArmorStacks); }

	/** Fractional Swift Boots move-speed bonus (0 = none). Applied to BOTH walk and sprint. */
	float GetMoveSpeedBonusFraction() const { return MoveSpeedBonus * SwiftBootsStacks; }

	/** EFFECTIVE base walk speed (Swift Boots composed). */
	float GetEffectiveWalkSpeed() const { return WalkSpeed * (1.f + GetMoveSpeedBonusFraction()); }

	/** EFFECTIVE sprint speed (Swift Boots composed). */
	float GetEffectiveSprintSpeed() const { return SprintSpeed * (1.f + GetMoveSpeedBonusFraction()); }

	/** Pushes the correct effective speed (sprint vs walk per bSprinting) into the movement component. Null-safe. */
	void ApplyMovementSpeed();

	/**
	 *  Applies the M6.6 climbable-terrain tunables (HeroMaxStepHeight / HeroWalkableFloorAngle /
	 *  HeroJumpZVelocity) onto the CharacterMovementComponent (WalkableFloorAngle via the
	 *  SetWalkableFloorAngle setter so the cached WalkableFloorZ recomputes). Null-safe. Called from
	 *  BOTH the constructor and BeginPlay (mirrors ApplyMovementSpeed) so a BP_HeroCharacter tweak survives.
	 */
	void ApplyTerrainMovementTuning();

	/** Stack cap (MaxCopies) for an upgrade CardID read from DT_Cards; 0 when the table/row is unavailable (caller refuses — never guesses). */
	int32 GetStackCapForUpgrade(FName UpgradeCardID) const;

	/** Begins the War Banner aura pulse (immediate pulse + looping timer). No-op while dead or when WarBanner is not owned; resumes on respawn via ResetHero. */
	void StartWarBannerAura();

	/** Clears the War Banner aura pulse timer (does NOT clear the stack). Called on death, ResetUpgrades, and re-arm. Null-safe/idempotent. */
	void StopWarBannerAura();

	/** War Banner pulse callback: SetAuraDamageBonus(WarBannerDamageBonus, …) on every friendly ASummonedUnit within WarBannerAuraRadius (mirrors Rally's iterate-friendlies loop). */
	void PulseWarBannerAura();

	/** Broadcasts OnHeroUpgradesChanged with the four current stack counts. */
	void BroadcastUpgradesChanged();

	//~ ─── RECALL internals (TASK-748) ───────────────────────────────────────────────────────

	/**
	 *  ⭐⭐ THE ONE PLACE A CHANNEL EVER ENDS, AND ALL SEVEN EXITS COME THROUGH IT.
	 *  Idempotent by its first line, so an exit that fires twice (damage-that-kills reaches
	 *  both the damage exit and the death exit in one call stack) still clears EXACTLY ONCE.
	 *  Clears the state, tears the tell down, broadcasts the state change — and then, for the
	 *  ONE granting exit only, performs the arrival.
	 */
	void EndRecall(ESiegeRecallExit Exit);

	/** Per-frame channel service, driven from Tick — checks exits 6 (match end), 3 (movement) and 1 (completion), in that order. Self-guards on the channel state. */
	void TickRecall(double NowSeconds);

	/** True when the match has been decided. ⚠️ Reads GetAuthGameMode(), which is NULL on a client — see the M8 declaration in the class comment. */
	bool IsMatchOver() const;

	/** Spawns the `R-3` channel tell attached to the hero, if an emitter is wired. Null-safe and idempotent — no asset means no tell, never a crash. */
	void StartRecallChannelEffect();

	/** Destroys the `R-3` channel tell. Called from the single EndRecall choke point, so it cannot outlive the channel on any of the seven exits. Idempotent. */
	void StopRecallChannelEffect();

	//~ ─── LADDER-CLIMB internals (TASK-778, `CONTACT-§3`) ───────────────────────────────────

	/**
	 *  ⛔⛔ EXIT `H-5`, AND IT IS AN ***INDEPENDENT BELT***, ⛔ NOT A DUPLICATE OF `H-4`.
	 *
	 *  ⚠️⚠️ A hero that dies mid-climb is UN-POSSESSED WHILE IN `MOVE_Flying` and an
	 *  `ASiegeGhostPawn` takes the controller (`GHOST-§`). If the possession change were the only
	 *  thing that happened — a debug possess, a seamless-travel repossess, a game-mode restart —
	 *  ⛔ nothing else in this class would ever end the climb, and the abandoned body would hang
	 *  in the air for the rest of the match with nobody driving it.
	 *  ⚖️ A possession change for ***any*** reason ends a climb, and enumerating the reasons is
	 *  how you miss one. Idempotent with `H-4` by the teardown's exactly-once latch.
	 *  ⭐ The `ASummonedUnit::EndPlay` "independent belt" idiom (`SummonedUnit.cpp:608-615`),
	 *  second application. ⛔ `SiegeGhostPawn.{h,cpp}` and `SiegeGameMode.{h,cpp}` are ⛔ NOT
	 *  touched: the ghost seam is closed from the hero's OWN side, which is what keeps them out.
	 */
	virtual void UnPossessed() override;

	/**
	 *  Per-frame climb service, driven from Tick. Self-guards on the climb state, so this is a
	 *  single bool read on every frame no climb is running. Checks exit `H-8` (match end) and exit
	 *  `H-3` (the input release) BEFORE advancing, then drives one frame along the pinned line.
	 */
	void TickLadderClimb(float DeltaSeconds);

	/**
	 *  ⭐⭐ THE ONE PLACE A CLIMB EVER ENDS, AND ALL TEN EXITS COME THROUGH IT. The exactly-once
	 *  latch is consumed FIRST, before any effect, so a double exit (death then `EndPlay` is the
	 *  ORDINARY case, ⛔ not an edge one) is inert by construction.
	 *
	 *  ⛔⛔ THE RESTORE IS THE LINE THE WHOLE FEATURE TURNS ON: `MOVE_Flying` ignores gravity, so
	 *  an exit that skips it leaves the PLAYER'S OWN BODY hanging forever. `SetDefaultMovementMode`
	 *  is this project's shipped idiom for exactly that (`ASummonedUnit::EndLadderClimb`,
	 *  `EndSpellFreeze`), and in mid-air it goes straight to `MOVE_Falling`.
	 *
	 *  ⭐⭐ AND IT IS ALSO THE ONE PLACE `OnLadderClimbEnded` IS BROADCAST (TASK-787,
	 *  `CONTACT-§12.3`) — LAST, after the restore, so the tower RELEASES its occupancy slot on every
	 *  one of the ten exits. ⛔ Exactly-once is INHERITED from this function's latch, ⛔ never a
	 *  second flag: ten exits, one teardown, one signal.
	 */
	void EndLadderClimb(bool bReachedTop, ESiegeHeroLadderExit Reason);

	/**
	 *  ⭐⭐ THE HERO'S CALL SITE — **THE PAWN ASKS, THE TOWER DECIDES** (`CONTACT-§4.1` / `WR-§5`,
	 *  the shipped `ACommanderNpc::IsPlayerInRange` idiom polled by its caller at
	 *  `SiegePlayerController.cpp:4970`). ⛔ The radius, the intent cone, the dwell and `K-C`'s
	 *  re-arm latch ALL live on `AClimbableTower` — ⛔ there is deliberately ⛔ NOT ONE of them on
	 *  this class, and a second copy would be a silent drift waiting to happen.
	 *
	 *  ⛔⛔ AND IT MAY ⛔ NEVER START A CLIMB ITSELF. `AClimbableTower::TryBeginContactClimb` runs
	 *  `CanTeamAscend` (`T-3`: enemies may not ascend, ⛔ no hero exemption) and the single
	 *  occupancy slot (`TOWER-§10 L-1`) and CLAIMS that slot — a hero that self-started around it
	 *  would be a ⛔ SILENT BACK DOOR around a Jonathan ruling AND would be invisible to
	 *  `AClimbableTower::EndPlay`, i.e. exit `H-10` would not exist for it.
	 */
	void PollLadderContact(float DeltaSeconds);

	/**
	 *  Rebuilds the cached tower list at a fixed cadence. ⛔ NOT a proximity filter — ⛔ nothing
	 *  here duplicates `LadderContactRadiusUU`; the tower's own cheap 2D term drops a passer-by
	 *  for one squared-distance compare, which is exactly what it was designed to do.
	 */
	void RefreshNearbyClimbableTowers();

	/**
	 *  ⭐⭐ THE WATCHDOG THIS CLASS DOES ⛔ NOT OTHERWISE HAVE, AND ITS ABSENCE IS ***MEASURED***
	 *  (`CONTACT-§3.5`): `ASummonedUnit`'s driver rides `StateTimerHandle`, a looping 0.25 s entry
	 *  that ALWAYS runs. This class has exactly TWO `SetTimer` calls — `RallyCooldownTimerHandle`
	 *  (one-shot) and `WarBannerAuraTimerHandle` (armed only while the upgrade is owned, and PAUSED
	 *  by `HandleDeath`) ⇒ ⛔⛔ A DRIVER COPIED FROM THE UNIT'S WOULD SHIP WITH ⛔ NO WATCHDOG AT
	 *  ALL AND LOOK IDENTICAL IN REVIEW.
	 *
	 *  ⭐ AND IT IS STRUCTURALLY SAFER THAN THE UNIT'S, WHICH IS WORTH SAYING: an `FTimerManager`
	 *  entry lives on the WORLD and ⛔ cannot be killed by an actor tick-flag write, so TASK-760's
	 *  hazard class (*"the watchdog rides the very driver it watches"*) ⛔ CANNOT EXIST HERE — ⛔ do
	 *  ⛔ not "harmonise" this onto the unit's tick-flag shape. It is armed ONLY for a climb's
	 *  duration (~3.5 s) and cleared by every one of the ten exits.
	 */
	void OnLadderClimbWatchdog();

	/**
	 *  Resolves the ONE shipped ladder rate. ⛔ THERE IS DELIBERATELY ⛔ NO `LadderClimbSpeedUU`
	 *  ON THIS CLASS (`CONTACT-§8`: *"⛔ never a second copy on the hero"* — ⚖️ two speed
	 *  properties is how they drift). Returns false, warns ONCE and REFUSES the climb rather than
	 *  inventing a number, so a rename fails LOUDLY instead of shipping a hero that crawls.
	 */
	bool TryResolveLadderClimbSpeedUU(float& OutClimbSpeedUU);

	/** ⭐ Exit `H-3`'s predicate: is the player still steering INTO the ladder this frame? ⛔ Reads a DIRECTION, ⛔ never a key, and ⛔ carries no cone tunable of its own — see the .cpp. */
	bool IsLadderClimbInputHeld() const;

protected:

	/** Team this hero fights for. M8 (TASK-356, doc §2.3/D4 — retires "the local player is always Blue"): ASSIGNED from the owning ASiegePlayerState in PossessedBy (host=Blue / client=Red seat), replicated so every machine's proxy reads the truth (OnRep_Team re-stamps the CASTLE-3X capsule channel — addendum §1). Default Blue = the standalone identity. */
	UPROPERTY(ReplicatedUsing = OnRep_Team, EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Team")
	ETeamId Team = ETeamId::Blue;

	/** Overhead poll-driven health bar (M5.5, TASK-110): hide-at-full, team-tinted. ADDITIVE to the hero's own WBP_HUD HP readout (M1) — that stays. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero")
	TObjectPtr<UCombatantHealthBarComponent> HPBarWidget;

	/** §6 white hit-flash on every actual damage event (M7, TASK-154). Driven from TakeDamage; overlay-based (flashes the template skeletal GetMesh()), null-safe. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Feedback")
	TObjectPtr<USiegeHitFlashComponent> HitFlashComponent;

	/** Mapping context slot for /Game/Input/IMC_Hero — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> HeroMappingContext;

	/** Input action slot for /Game/Input/Actions/IA_Sprint — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	/** Input action slot for /Game/Input/Actions/IA_Attack — assigned on BP_HeroCharacter in TASK-009. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	/** Input action slot for /Game/Input/Actions/IA_Rally (key Q) — assigned + bound on BP_HeroCharacter in TASK-048. Null-safe until then. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RallyAction;

	/**
	 *  IA_Recall slot (mapped to `B` inside IMC_Hero by TASK-747). Left unset, it soft-resolves
	 *  from RecallActionAsset — a missing asset skips the binding, logs ONE line and leaves the
	 *  key completely INERT (⛔ never a crash). ⭐ THAT IS THE DESIGNED COMPILE-TIME STATE and it
	 *  is why this feature does not serialize behind the asset: the shipped `IA_Cmd*` pattern.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RecallAction;

	/** Soft path for IA_Recall (/Game/Input/Actions/IA_Recall, created in TASK-747). Null-safe — see RecallAction. */
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> RecallActionAsset;

	/** Base movement speed in u/s (GDD §3.1: 500). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float WalkSpeed = 500.f;

	/** Movement speed in u/s while IA_Sprint is held (GDD §3.1: 750). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float SprintSpeed = 750.f;

	//~ Climbable-terrain movement tuning (M6.6, TASK-141) — pushed onto the CharacterMovementComponent
	//~ by ApplyTerrainMovementTuning() from BOTH the ctor and BeginPlay (mirrors ApplyMovementSpeed).
	//~ The `Hero` prefix is MANDATORY and load-bearing: an un-prefixed name would SHADOW the identically
	//~ named UCharacterMovementComponent field it drives (MaxStepHeight / WalkableFloorAngle /
	//~ JumpZVelocity), which UHT compiles as the C4457/58/59 shadow HARD ERROR (CONVENTIONS shadow law).
	//~ Retune is comfort/margin only — the ≤30° hill faces are already climbable without it.

	/** Max vertical step the hero walks up without jumping, in units. Drives CharacterMovement MaxStepHeight (M6.6: 50, was 45). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float HeroMaxStepHeight = 50.f;

	/** Steepest floor the hero can stand/walk on, in degrees. Drives CharacterMovement WalkableFloorAngle via SetWalkableFloorAngle (M6.6: 50, was 44.76). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0", ClampMax = "90"))
	float HeroWalkableFloorAngle = 50.f;

	/** Upward launch speed of a jump, in u/s. Drives CharacterMovement JumpZVelocity (M6.6: 600 ⇒ ~184 cm apex, was 500). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Movement", meta = (ClampMin = "0"))
	float HeroJumpZVelocity = 600.f;

	/** Damage per melee swing (GDD §3.1: 20). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeDamage = 20.f;

	/** Melee reach in units (GDD §3.1: 150). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeRange = 150.f;

	/** Half-angle of the forward melee cone in degrees (GDD §3.1: ±30 = 60° cone). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0", ClampMax = "180"))
	float MeleeHalfAngleDegrees = 30.f;

	/** Minimum seconds between melee swings (GDD §3.1: 0.5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float MeleeCooldown = 0.5f;

	/** Rally: radius in units within which friendly units are buffed. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyRadius = 600.f;

	/** Rally: fractional move-speed bonus applied to each buffed unit (0.25 = +25%). // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallySpeedBonus = 0.25f;

	/** Rally: seconds each friendly unit keeps the move-speed buff. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyDuration = 5.f;

	/** Rally: seconds before Rally can be used again. // GDD §4 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Combat", meta = (ClampMin = "0"))
	float RallyCooldown = 20.f;

	/**
	 *  Montage played on EVERY swing that passes the cooldown gate — hit or whiff —
	 *  while melee is not suppressed. VISUAL ONLY: damage is applied immediately in
	 *  DoMeleeAttack and never gated on anim notifies (playtest R1 finding 1).
	 *  Wired on BP_HeroCharacter in TASK-017 (/Game/Variant_Combat/Anims/AM_ComboAttack
	 *  or AM_ChargedAttack); null-safe — unset means no montage, damage unchanged.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Optional montage section to start at so exactly one swing plays (chosen in TASK-017); NAME_None plays from the start. */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	FName AttackMontageSection = NAME_None;

	/**
	 *  Impact effect spawned once per enemy actually damaged this swing, at the closest
	 *  point on that enemy's collision to the hero (fallback: its actor location).
	 *  Wired in TASK-017 (/Game/Variant_Combat/VFX/NS_Damage); null-safe.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TObjectPtr<UNiagaraSystem> HitImpactEffect;

	/**
	 *  Camera shake played once per swing on the local player controller when the
	 *  swing damaged >= 1 enemy, via ClientStartCameraShake. Wired in TASK-017
	 *  (/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy); null-safe.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat|Feedback")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Maximum hit points (GDD §3.1: 200). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "1"))
	float MaxHP = 200.f;

	/** Seconds after last taking OR dealing damage before regen begins (GDD §3.1: 8). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float RegenDelay = 8.f;

	/** Out-of-combat regeneration in HP/s (GDD §3.1: 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Hero", meta = (ClampMin = "0"))
	float RegenRate = 5.f;

	//~ Hero-upgrade magnitudes (TASK-058) — UPROPERTY defaults per GDD §3.10/§4, NEVER from CSV
	//~ (CONVENTIONS: mechanic magnitudes are class UPROPERTYs; only the stack CAP is a DT_Cards column).

	/** Sharpened Blade: melee damage added PER stack (GDD §3.10: 10). Cap 2 ⇒ cone melee 20→30→40. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MeleeDamageBonus = 10.f; // GDD §3.10

	/** Plate Armor: max HP added PER stack AND healed immediately on apply (GDD §3.10: 100). Cap 2 ⇒ 200→300→400. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MaxHPBonus = 100.f; // GDD §3.10

	/** Swift Boots: fractional move-speed bonus PER stack applied to BOTH walk and sprint (GDD §3.10: 0.25 = +25%). Cap 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float MoveSpeedBonus = 0.25f; // GDD §3.10

	/** War Banner: aura radius in units within which friendly units are buffed (GDD §4: 600). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float WarBannerAuraRadius = 600.f; // GDD §4

	/** War Banner: fractional damage bonus applied to each friendly unit in the aura (GDD §4: 0.20 = +20%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0"))
	float WarBannerDamageBonus = 0.20f; // GDD §4

	/** War Banner: seconds between aura pulses. The bonus is (re)applied to in-range units each pulse; the SetAuraDamageBonus window is 2× this so a unit that stays in range never flickers and a unit that leaves loses the bonus shortly after. Impl detail — not a GDD stat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Siegebound|Upgrades", meta = (ClampMin = "0.05"))
	float WarBannerPulseInterval = 0.5f;

	/** DT_Cards asset the stack caps (MaxCopies) are read from (GDD §3.0). Resolved null-safe at BeginPlay; a missing table refuses upgrade plays. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Upgrades")
	TSoftObjectPtr<UDataTable> CardTableAsset;

	//~ Recall tunables (TASK-748, `RECALL-§4`) — `HIGH-§1`: a number whose CONSEQUENCE is not
	//~ written beside it gets retuned by someone who does not know what they are changing.

	/**
	 *  Seconds the channel must run UNINTERRUPTED before the hero teleports home and heals to
	 *  full. Jonathan, verbatim: *"after 10 seconds."*
	 *
	 *  ⚠️ CONSEQUENCE OF CHANGING IT, IN BOTH DIRECTIONS:
	 *    • SHORTER turns recall into a near-free repositioning tool and deletes the counterplay
	 *      window that is the entire reason the League mechanic he named HAS a duration — the
	 *      enemy's chance to walk over and interrupt it IS the mechanic.
	 *    • LONGER makes it unusable under any pressure at all, since a single landed hit from
	 *      any source resets it to zero (`R-1`/`R-4`).
	 *  ⛔ And it must be re-read against the hero respawn delay (`GHOST-§0`) before any retune:
	 *  recall is the only way to spend accumulated HP loss WITHOUT paying that death timer.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Recall", meta = (ClampMin = "0.1"))
	float RecallChannelSeconds = 10.f;

	/**
	 *  How far the hero may drift from where it started channelling before the movement-cancel
	 *  (`R-2`) fires, in unreal units.
	 *
	 *  ⚠️ CONSEQUENCE: at 0 the channel would cancel on animation settle, capsule
	 *  penetration-resolve and knockback jitter — i.e. it would look broken and random. Made
	 *  LARGE it becomes a free repositioning budget, which is the defect the cancel exists to
	 *  prevent. 25 uu is a quarter of the capsule radius: no walk input can stay inside it and
	 *  no idle hero can leave it.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Recall", meta = (ClampMin = "0"))
	float RecallMoveCancelToleranceUU = 25.f;

	/**
	 *  `R-3` THE CHANNEL TELL — spawned attached to the hero for the channel's life and
	 *  destroyed on every one of the seven exits. World-space and therefore visible to an enemy
	 *  observer, which is the ruling.
	 *
	 *  ⛔ DELIBERATELY UNSET AND WITH ⛔ NO DEFAULT ASSET PATH: no task in this batch produces a
	 *  recall emitter, and naming a content path the Artist is not making would be an invented
	 *  asset reference (CONVENTIONS: code names must match exactly what the spec says the Artist
	 *  will produce). ⇒ FOR THE MANAGER: `R-3`'s world-space tell needs an art task; until then
	 *  the shipped tell is the HUD's, driven by OnRecallStateChanged + GetRecallProgress01.
	 *  Null-safe: unset means no emitter, and the channel is otherwise byte-identical.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|Recall")
	TObjectPtr<UNiagaraSystem> RecallChannelEffect;

private:

	/** Current hit points. Mutated only by TakeDamage, Tick (regen), and ResetHero. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Hero", meta = (AllowPrivateAccess = "true"))
	float CurrentHP = 200.f;

	/** True from HP hitting 0 until ResetHero(). Death side effects run exactly once. */
	bool bDead = false;

	/** True while placement mode (TASK-007) suppresses melee. */
	bool bMeleeSuppressed = false;

	/** World time of the last melee swing (cooldown gate). Seeded far in the past so the first swing is always allowed. */
	double LastMeleeTime = -1.0e9;

	/** World time the hero last took or dealt damage (regen gate). */
	double LastCombatTime = -1.0e9;

	/** World time of the last successful Rally (cooldown gate). Seeded far in the past so the first Rally is always allowed. */
	double LastRallyTime = -1.0e9;

	/** Drives OnRallyReady once RallyCooldown elapses after a successful Rally, to broadcast the ready state. */
	FTimerHandle RallyCooldownTimerHandle;

	//~ Hero-upgrade state (TASK-058). Stack counts are the ONLY mutated state — every effective
	//~ stat derives live from them, so they persist through respawn (ResetHero does not clear them)
	//~ and reset only in ResetUpgrades. VisibleInstanceOnly for PIE inspection; not saved.

	/** Sharpened Blade stacks (0..MaxCopies). Drives GetEffectiveMeleeDamage. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 SharpenedBladeStacks = 0;

	/** Plate Armor stacks (0..MaxCopies). Drives GetEffectiveMaxHP. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 PlateArmorStacks = 0;

	/** Swift Boots stacks (0..MaxCopies). Drives GetMoveSpeedBonusFraction. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 SwiftBootsStacks = 0;

	/** War Banner stacks (0..MaxCopies). >0 ⇒ the aura pulse timer runs while alive. */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|Upgrades", meta = (AllowPrivateAccess = "true"))
	int32 WarBannerStacks = 0;

	/** True while IA_Sprint is held (drives ApplyMovementSpeed's walk-vs-sprint choice so upgrades recompute the right speed). */
	bool bSprinting = false;

	/** Hard-resolved DT_Cards (cached at BeginPlay) that ApplyUpgrade reads MaxCopies from. */
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedCardTable;

	/** Drives PulseWarBannerAura every WarBannerPulseInterval while War Banner is owned and the hero is alive. */
	FTimerHandle WarBannerAuraTimerHandle;

	//~ Recall channel state (TASK-748). ⭐ ONE struct and ONE spawned component — the complete
	//~ list of what the seven exits have to undo, and both are undone in EndRecall and nowhere else.

	/** The running channel, or the cleared state. ⛔ Not a UPROPERTY: pure transient runtime state, never authored and never saved. */
	FSiegeRecallState RecallState;

	/** The live `R-3` tell component while channelling (null otherwise). Transient so a PIE inspect shows it and no save ever carries it. */
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RecallChannelEffectComponent;

	/** One-shot latch so a missing destination binding warns ONCE per hero instead of on every completed channel. */
	bool bWarnedRecallDestinationUnbound = false;

	//~ ─── Ladder-climb state (TASK-778). ⭐ THE COMPLETE LIST OF WHAT THE TEN EXITS HAVE TO UNDO,
	//~ and all of it is undone in EndLadderClimb and NOWHERE else (the RecallState discipline
	//~ directly above, second application on this class).

	/** The running climb, or the cleared state. ⛔ Not a UPROPERTY: pure transient runtime state, never authored, never saved and — by construction — never replicable by accident (`CONTACT-§9`). */
	FSiegeLadderClimbState LadderClimb;

	/**
	 *  ⭐ THE HERO'S OWN 0.25 s WATCHDOG (`CONTACT-§3.5`) — armed at `BeginLadderClimb`, cleared by
	 *  every one of the ten exits, and running ⛔ ONLY for a climb's ~3.5 s. ⛔ Never a permanent
	 *  poll added to this class for a rare feature.
	 */
	FTimerHandle LadderClimbWatchdogTimerHandle;

	/** Wall-clock deadline the watchdog compares against, fixed at Begin from the climb's OWN budget. ⛔ Not a second budget: it is `LadderClimb.TimeoutSeconds` expressed on the world clock, so a stalled Tick cannot postpone it. */
	double LadderClimbWatchdogDeadlineSeconds = 0.0;

	/** `MaxFlySpeed` as it was before the ascent, restored EXACTLY on every exit (zero residual). ⛔ Nothing else in this project writes that field on the hero. */
	float LadderClimbSavedMaxFlySpeed = 0.f;

	/**
	 *  The rate RESOLVED from the one shipped `ASummonedUnit::LadderClimbSpeedUU` at Begin and held
	 *  for this climb only. ⛔ NOT a tunable and ⛔ not authored — there is nothing here to edit.
	 *  ⭐ Cached for the same reason `Begin` fixes the watchdog budget at arming time: a mid-climb
	 *  retune must not be able to change a climb that is already running.
	 */
	float LadderClimbResolvedSpeedUU = 0.f;

	/** The player's last steer, in WORLD space, captured by `DoMove` while a climb runs (exit `H-3`'s only evidence — the movement component never sees it). */
	FVector LadderClimbSteerWorld = FVector::ZeroVector;

	/** The frame that steer arrived on. ⭐ A FRAME-ADJACENCY test, ⛔ not a timeout: it needs ⛔ no tunable and tolerates either tick order between the player controller and this pawn. */
	uint64 LadderClimbSteerFrame = 0;

	/** Live `AClimbableTower`s, rebuilt on a cadence and pruned on use. ⛔ Weak on purpose — a tower destroyed mid-match must ⛔ not be polled, and a table that only grows is a leak nobody notices inside a five-minute match. */
	TArray<TWeakObjectPtr<AClimbableTower>> NearbyClimbableTowers;

	/** World time the tower list is next rebuilt at. */
	double NextClimbableTowerScanSeconds = 0.0;

	//~ ⭐ TASK-787: `bWarnedLadderStartSeamClosed` is GONE, and its Warning with it. It latched the
	//~ `NotAnAdmittedClimber` verdict TASK-778 could only report — and `CONTACT-§12` closed that
	//~ blocker by WIDENING the tower's identity term, so a hero can no longer produce that verdict
	//~ at all (an `AHeroCharacter` cannot fail a `Cast<ILadderClimber>`; the interface is a
	//~ compile-time base). ⚖️ `SC-§36` INVERTED: a warning that ⛔ cannot fire is indistinguishable
	//~ from one that works, and this one would have read as a live diagnostic forever. ⛔ Its
	//~ content is not lost — it is `CONTACT-§12`, which is where it belongs.

	/** One-shot latch: the shipped ladder rate could not be resolved, so climbs are refused. Warns ONCE per hero. */
	bool bWarnedLadderRateUnresolved = false;
};
