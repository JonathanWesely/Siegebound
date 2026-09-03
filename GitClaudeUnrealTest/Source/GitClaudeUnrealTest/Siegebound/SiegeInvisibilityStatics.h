// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// ETeamId is the ONLY project type this pair names. No UWorld, no AActor, no UObject, no
// component, no clock, no allocation, no timer handle — the FSiegeLadderClimbStatics /
// FSiegeStuckStatics / FSiegeCombatStatics idiom (complete-type include law, TASK-110).
// That is not tidiness: it is the property the headless tests in SiegeInvisibilityTest.cpp
// are built on, and it is WITCH-§6's stated requirement for this file.
#include "CoreMinimal.h"
#include "Siegebound/TeamId.h"

/**
 *  ═══ TASK-827 (WITCH-§3 + WITCH-§6): THE PURE INVISIBILITY RULES ═══
 *
 *  Jonathan, verbatim (the directive lives in CONVENTIONS "THE WITCH + INVISIBILITY",
 *  quoted here ONCE because this file is where his sentence becomes a type):
 *
 *    "Invisible units will remain invisble until they attack/heal/mine/power up something
 *     (basically if they do anything other than walk), to which then they permanently go
 *     back to visible (unless they are later made invisible by a witch again)."
 *
 *  ⛔⛔ THIS FILE CHANGES ⛔ NO BEHAVIOUR. Nothing in the shipped game consults it yet.
 *  TASK-829 owns the per-instance `bool bIsInvisible` on ASummonedUnit and the
 *  `BreakInvisibility(ESiegeVeilBreakReason)` call sites; TASK-830 owns the witch's cast.
 *  ⛔ A landed file is ⛔ not a landed feature — see handoffs/TASK-827-programmer.md.
 *
 *  ── WHY PURE STATICS AND ⛔ NOT AN ACTOR / COMPONENT / SUBSYSTEM (WITCH-§6) ─────────────
 *  WITCH-§0 recorded, in writing, that invisibility CANNOT ride the ghost's mechanism: the
 *  ghost is untargetable because ASiegeGhostPawn does not implement ITeamAgent at all, which
 *  is a COMPILE-TIME, CLASS-LEVEL, PERMANENT property, while a veil is per-instance, runtime,
 *  and — by his own interruption sentence — must PRESERVE ATTACKABILITY. So the veil has to be
 *  a flag consulted at a guard point, which is exactly the weaker pattern GHOST-§1 refused.
 *  ⭐ WITCH-§1's funnel is one half of the mitigation (one gatherer, nine sites, TASK-828).
 *  ⭐ THIS FILE IS THE OTHER HALF: every rule that can be stated without a world is stated
 *  here, once, where a headless test can hold it. A rule that lives only inside an actor tick
 *  is a rule nobody can assert.
 *
 *  ── ⛔⛔ THE "PERMANENTLY" LAW IS ENFORCED BY THE ⛔ SHAPE OF THIS API, ⛔ NOT BY A COMMENT ──
 *  His word is "permanently". WITCH-§3 binds it: once broken, the veil ⛔ NEVER self-restores —
 *  ⛔ no timer, ⛔ no cooldown, ⛔ no decay, ⛔ no re-veil except a NEW witch cast.
 *  ⭐ THE MECHANISM: ⛔ NOT ONE FUNCTION IN THIS FILE TAKES A TIME PARAMETER. There is no
 *  `float DeltaSeconds`, no `Duration`, no `Seconds`, no `RemainingSeconds`, no clock, no
 *  handle, and no state that could hold one. A cooldown is ⛔ UNREPRESENTABLE in this API —
 *  a future reader cannot drift into one without changing a signature, which is a review event.
 *  ⛔ There is deliberately ⛔ NO RestoreVeil / RefreshVeil / TickVeil. The ONLY write-true
 *  site is ApplyVeil (TASK-830's cast completion); the ONLY write-false site is ApplyBreak.
 *  ⇒ a grep for veil writes finds exactly TWO functions, both in this file.
 *
 *  📌 NOTE FOR QA — ⛔ NOT A ONE-CLASS-PER-HEADER VIOLATION. ESiegeVeilBreakReason and
 *  FSiegeInvisibilityStatics share this header under the shipped "pure data types may share a
 *  header when they form one concept" exception (the TeamId.h precedent; CONVENTIONS records it
 *  for FSiegeStuckState/FSiegeStuckTuning/ESiegeStuckAction in SiegeStuckStatics.h and for
 *  FSiegePositionalKeyProbe/FSiegeKeyResolver in SiegeKeyboardLayoutStatics.h). WITCH-§6 names
 *  ⛔ this one file as the home of both, and splitting them would put the closed break-set in a
 *  header separate from the ⛔ only function that consumes it.
 */

/**
 *  ⭐⭐ THE CLOSED BREAK-SET (WITCH-§3). Every enumerator was read out of the SHIPPED code —
 *  ⛔ this list is MEASURED, ⛔ never paraphrased from his sentence, because a list built from
 *  his sentence would miss exactly the verbs he did not think to type. The measured file:line
 *  for each shipped site is recorded beside it so a reviewer can re-verify without a search.
 *
 *  ⛔⛔ ABSENCE IS THE LAW HERE, AND IT IS LOAD-BEARING:
 *    ⛔ There is ⛔ NO `Walk`. ⛔ There is ⛔ NO `TakeDamage`. ⛔ There is ⛔ NO `Order`
 *    (and no `Select`, no `Climb`, no `Acquire`, no `Capture`, no `ReceiveHeal`, no
 *    `ReceiveBuff`, no `Freeze`).
 *  ⭐ THIS IS ⛔ NOT AN OVERSIGHT AND A FUTURE READER MUST ⛔ NOT "HELPFULLY" ADD ONE.
 *    - `Walk` is his explicit carve-out — the whole point of the card is that a veiled unit
 *      can cross the field.
 *    - `TakeDamage` is refused because ⛔ HIS RULE BREAKS THE VEIL ON ***ACTING***, ⛔ NOT ON
 *      BEING ACTED UPON. ⚠️ Taking damage DOES interrupt an in-progress witch CAST
 *      (WITCH-§4, TASK-830) — ⭐ that is a ⛔ DIFFERENT RULE ON A ⛔ DIFFERENT SUBJECT and the
 *      two must ⛔ never be merged. Merging them would make every veiled unit that is hit
 *      once — including by a stray AoE it cannot see coming — permanently visible, which is
 *      not what he wrote.
 *    - `Order` / `Select` are refused for the same reason plus WITCH-§2's fourth lane: an
 *      invisible unit its own player cannot order is a ⛔ BUG, not a feature.
 *  ⭐ THE TYPE SYSTEM ⛔ IS THE PREDICATE. An action that does not break the veil has ⛔ no
 *  enumerator, so it ⛔ cannot be passed to ApplyBreak. There is deliberately no
 *  `bool ActionBreaksVeil(...)` classifier taking a wider action enum — a classifier can be
 *  called with the wrong argument; a missing enumerator ⛔ cannot compile.
 *  ⭐ AND THE TRIPWIRE, so absence survives a future edit: FSiegeInvisibilityStatics::
 *  VeilBreakReasonCount + ToString + the Siegebound.Invisibility.BreakReasonCensusIsClosed
 *  test FAIL LOUDLY the moment a seventh enumerator appears. ⛔ Adding one is a design change
 *  that goes back through the manager, ⛔ not an edit.
 *
 *  Plain enum, ⛔ NOT a UENUM — the ESiegeLadderExit / ESiegeStuckAction discipline (NAV-§8/§11,
 *  SummonedUnit.h:84): it is never a UPROPERTY, and an unreflected type ⛔ cannot be replicated
 *  by accident. That matters more here than anywhere: WITCH-§6's M8 declaration records that
 *  `bIsInvisible` is authoritative, ASYMMETRIC-PER-CLIENT state, and that a naive replicated
 *  broadcast ⛔ LEAKS THE VEILED UNIT'S POSITION to the enemy client's renderer. If TASK-829
 *  ever needs this reflected for a Blueprint surface, that is a deliberate `UENUM(BlueprintType)`
 *  + `.generated.h` amendment with the leak re-argued — ⛔ not a silent convenience.
 */
enum class ESiegeVeilBreakReason : uint8
{
	/**
	 *  MELEE / RANGED / SUICIDE DAMAGE DEALT BY THE VEILED ACTOR. ⛔ His word, first in the list.
	 *  MEASURED SHIPPED SITES (TASK-829 wires these; ⛔ this file only names them):
	 *    • ASummonedUnit::PerformAttack melee  — SummonedUnit.cpp:3050 (UGameplayStatics::ApplyDamage);
	 *      the swing is armed in EnterAttack (SummonedUnit.cpp:2679) with a SYNCHRONOUS first hit at
	 *      SummonedUnit.cpp:2753 and the cadence timer at :2756.
	 *    • ASummonedUnit::PerformAttack ranged — SummonedUnit.cpp:3031 (FireProjectileAt, def :3516);
	 *      the projectile is actually spawned at SummonedUnit.cpp:3545 and armed at :3554.
	 *    • ⭐⭐ ASummonedUnit::ApplyDetonation — SummonedUnit.cpp:4209 (the SAPPER's suicide blast,
	 *      via FSiegeCombatStatics::ApplyRadialDamage; def :4180, compose point :4202).
	 *      ⚠️⚠️ THIS SITE IS ⛔ NOT REACHED THROUGH PerformAttack — it is entered from
	 *      UpdateStateSiege contact (SummonedUnit.cpp:2418) and from HandleDeath (:4227).
	 *      ⛔ A guard placed only in PerformAttack would let a veiled Sapper blow a building open
	 *      while still invisible. ⭐⭐ THIS IS THE VERB HIS SENTENCE DOES NOT CONTAIN, and it is
	 *      exactly the class of miss WITCH-§3's "measured, never paraphrased" rule exists to catch.
	 *    • ATower::FireProjectileAt — Tower.cpp:307 (spawn at Tower.cpp:337) · ATower::FireChainZapAt
	 *      — Tower.cpp:359 (damage at Tower.cpp:469). ⚠️ Towers are ⛔ not veilable today (the witch
	 *      targets UNITS) — recorded so a future "veil a tower" card inherits the site list.
	 *    • AHeroCharacter::DoMeleeAttack — HeroCharacter.cpp:463 (damage at HeroCharacter.cpp:611).
	 *      ⚠️ Hero-veilability is TASK-830's targeting question; WITCH-§4's default is "nearest
	 *      friendly UNIT", so this is dormant unless that default moves.
	 *  ⚠️ MEASURED AND ⛔ DELIBERATELY EXCLUDED FROM THIS ENUMERATOR: the Cavalry CHARGE wind-up
	 *  (ASummonedUnit::TrackChargeMovement, SummonedUnit.cpp:3081). ⭐ A charge is ⛔ WALKING — it
	 *  only accumulates a multiplier consumed later by ComputeOutputDamage (:3389). The veil breaks
	 *  when the ⛔ BLOW LANDS at :3050, ⛔ not when the horse starts running.
	 */
	Attack,

	/**
	 *  HEALING APPLIED ***BY*** THE VEILED ACTOR (the Cleric / ECardProfile::Support lane). ⛔ His word.
	 *    • ASummonedUnit::PerformHeal — SummonedUnit.cpp:2662 (`HealTarget->ApplyHealing(...)`)
	 *      driven by the SupportHeal timer armed in StartHealing (SummonedUnit.cpp:2619) from
	 *      UpdateStateSupport (SummonedUnit.cpp:2429).
	 *  ⛔⛔ THE DIRECTION IS LOAD-BEARING AND IT IS THE EASY BUG: the ⛔ HEALER breaks. The
	 *  ⛔ PATIENT does ⛔ NOT — ASummonedUnit::ApplyHealing (SummonedUnit.cpp:2665) is the
	 *  RECEIVER side, and a veiled unit that gets mended by a friendly Cleric ⛔ STAYS VEILED.
	 *  Being healed is being acted upon.
	 */
	Heal,

	/**
	 *  MINING — the AMinerUnit's arrival at a gold node and the income it then earns. ⛔ His word.
	 *  ⭐⭐ THE ⛔ UNIT-SIDE SITES — these are the ones TASK-829 can actually hook:
	 *    • AMinerUnit::UpdateMining — MinerUnit.cpp:514 (`Node->TryRegisterArrivedMiner(this)`),
	 *      the ⭐ ARRIVAL AND CLAIM: the arrival-radius test is :504-506, the miner stops at :528
	 *      and the mining "clink" loop starts at :533.
	 *    • AMinerUnit::UpdateMining — MinerUnit.cpp:541 (`OwnerState->AddMinerIncome()`), once per
	 *      tenure.
	 *  ⚠️⚠️ MEASURED TRAP FOR TASK-829 — ⛔ THE GOLD IS ⛔ NOT GRANTED ON THE MINER'S CALL STACK.
	 *  The node's reserve drains on the NODE's timer (AGoldNode::HandleDrainTick, GoldNode.cpp:317;
	 *  the claim lands in AGoldNode::TryRegisterArrivedMiner at GoldNode.cpp:124/150) and the
	 *  player's +gold/s is granted on the PLAYER STATE's 1 s timer
	 *  (ASiegePlayerState::HandleGoldTick, SiegePlayerState.cpp:197 → SetGold at :238).
	 *  ⇒ ⛔ A hook on "where gold appears" would break the veil of ⛔ every miner the player owns,
	 *  ⛔ including ones still walking. ⭐ THE BREAK BELONGS AT THE ⛔ ARRIVAL (MinerUnit.cpp:514),
	 *  which fires once per tenure on the ⛔ acting miner. ApplyBreak below is idempotent by
	 *  construction, so a belt at :541 is harmless — but the arrival is the correct one.
	 */
	Mine,

	/**
	 *  "POWER UP SOMETHING" — the Sorcerer's Ancient-Ground empowerment. ⛔ His phrase; WITCH-§3
	 *  rules this is the referent, and it is the ⛔ ONLY "power up" a UNIT performs.
	 *    • AAncientGround::ApplyBoostTick — AncientGround.cpp:270 (`Unit->AddPermanentDamageStacks(Grant)`),
	 *      the gated tick that is that mutator's sole caller (AncientGround.cpp:197).
	 *  ⛔⛔ THE DIRECTION IS AGAIN LOAD-BEARING, AND HERE IT IS ⛔ INVERTED FROM WHERE THE CODE IS.
	 *  The ⛔ SORCERER is the one acting — but the sorcerer ⛔ CALLS NOTHING. It is PASSIVE: it is
	 *  merely counted (`Unit->IsAncientGroundEmpowerer()`, AncientGround.cpp:233 / SorcererUnit.h:81)
	 *  and the GROUND does the granting. ⇒ TASK-829 must break the veil on the ⛔ COUNTED SORCERERS,
	 *  ⛔ not on the units in the Occupants loop that receive stacks. ⚠️ The receivers are being
	 *  acted upon and ⛔ STAY VEILED. A naive "break at the AddPermanentDamageStacks call" would
	 *  un-veil ⛔ exactly the wrong actors.
	 *  ⚠️ ALSO MAPPED HERE IF THE HERO EVER BECOMES VEILABLE: AHeroCharacter::Rally
	 *  (HeroCharacter.cpp:706, ApplyMoveSpeedBuff on friendlies) and AHeroCharacter::PulseWarBannerAura
	 *  (HeroCharacter.cpp:1259, SetAuraDamageBonus). ⛔ Both are the HERO powering up units; WITCH-§3
	 *  scopes "power up" to a unit's act, so these are ⛔ dormant unless TASK-830 rules the hero veilable.
	 */
	Empower,

	/**
	 *  THE WITCH'S ⛔ OWN VEIL CAST — she is acting (WITCH-§3, ruling J-W3).
	 *  ⛔ NO SHIPPED SITE EXISTS YET: the cast is TASK-830 and this enumerator is its landing pad.
	 *  ⭐ WHY IT BREAKS, recorded so nobody "fixes" it into a special case: consistency beats
	 *  special-casing, and a self-veiling witch that never broke would be ⛔ PERMANENTLY
	 *  untargetable-by-acquisition — ⛔ the exact outcome WITCH-§0 refuses.
	 *  ⛔ This is the SUCCESSFUL cast only. An INTERRUPTED cast produces ⛔ no veil, ⛔ no partial
	 *  state, ⛔ no cost — and therefore ⛔ no break (WITCH-§4).
	 *  ⚠️⚠️ SCOPE, MEASURED AND ⛔ REFUTED: the shipped card SPELLS (Fireball / FrostNova /
	 *  Lightning / BattleCry / Pickpocket) are cast by a ⛔ CONTROLLER, ⛔ never by a pawn.
	 *  ⛔ NO unit, hero or pawn anywhere calls USpellLibrary::ResolveSpell (def SpellLibrary.cpp:572)
	 *  — every caller is ASiegePlayerController (SiegePlayerController.cpp:3128 targeted, :4380
	 *  instant) or ASiegeBotController (SiegeBotController.cpp:777, :827). The HeroLine delivery
	 *  uses the hero only as a geometric ORIGIN (SpellLibrary.cpp:598/609), ⛔ never as an executor.
	 *  ⇒ ⛔ the shipped spells are ⛔ NOT break sites. If a future card ever gives a UNIT a spell,
	 *  it lands here.
	 */
	Cast,

	/**
	 *  DEATH — WITCH-§3 ruling J-W4: the veil is ⛔ CLEARED, not "broken"; ⛔ no corpse is invisible.
	 *    • ASummonedUnit::HandleDeath — SummonedUnit.cpp:4213
	 *    • AHeroCharacter::HandleDeath — HeroCharacter.cpp:831 (if the hero is ever veilable)
	 *  ⭐ It is an enumerator rather than a bare assignment for ⛔ exactly one reason, and it is
	 *  WITCH-§6's: an inlined `bIsInvisible = false` ⛔ ANYWHERE is an automatic QA FAIL. Death
	 *  clears the flag through the ⛔ same one door as every other reason, so the door stays the
	 *  only door and a grep for it is complete.
	 */
	Death
};

/**
 *  ⭐⭐ THE ⛔ INVERSE LEDGER — WHAT MUST ⛔ NOT BREAK THE VEIL, ⛔ EACH ONE MEASURED AND ⛔ NAMED.
 *
 *  ⛔ This block has ⛔ no code and that is the point: these entries have ⛔ no enumerator above,
 *  and a reader who wonders "did they forget X?" finds X ⛔ here, ⛔ with the reason. ⭐ An
 *  unwritten exclusion is indistinguishable from an oversight, and the next reader "fixes" it.
 *
 *  ── LOCOMOTION AND POSTURE — his explicit carve-out, "anything other than walk" ──────────────
 *    ⛔ WALK / ADVANCE   — ASummonedUnit::EnterAdvance (SummonedUnit.cpp:2760),
 *                          EnterAdvanceToLocation (:2892). ⭐ The whole card is that a veiled
 *                          unit crosses the field.
 *    ⛔ IDLE / STAND     — EnterIdle (SummonedUnit.cpp:2947), AMinerUnit::StandInPlace
 *                          (MinerUnit.cpp:845).
 *    ⛔ LADDER CLIMB     — ASummonedUnit::BeginLadderClimb (SummonedUnit.cpp:3792) /
 *                          TickLadderClimb (:3975); hero at HeroCharacter.cpp:1737/:1989.
 *                          ⭐ MEASURED AS LOCOMOTION, ⛔ not an act: it is MOVE_Flying +
 *                          AddMovementInput along one line, and it DISARMS attacking at all three
 *                          guard points via FSiegeLadderClimbStatics::IsAttackAllowed. ⭐ A unit
 *                          that cannot attack while doing it ⛔ cannot be "acting".
 *    ⛔ STUCK RECOVERY   — sidestep (SummonedUnit.cpp:3265), widen-and-repath (:3288-3290),
 *                          abandon (:3314); AMinerUnit::HandleStuckEscalation (MinerUnit.cpp:999).
 *                          ⭐ These are ⛔ walking that failed, ⛔ not a new verb.
 *    ⛔ CHARGE WIND-UP   — TrackChargeMovement (SummonedUnit.cpp:3081). ⭐ Running, not hitting.
 *    ⛔ HERO SPRINT      — HeroCharacter.cpp:446/:457. ⭐ Faster walking is still walking.
 *    ⛔ FACING A TARGET  — FaceTarget (SummonedUnit.cpp:3501). ⭐ Turning to look is not acting.
 *    ⛔⛔ ACQUIRING A TARGET — AcquireTarget (SummonedUnit.cpp:1654), AcquireEnemyNearPoint (:2199).
 *                          ⭐⭐ THE SUBTLE ONE, STATED SO IT IS ⛔ NOT "TIDIED" LATER: a veiled unit
 *                          may walk up to an enemy, pick it, close to range and raise its weapon —
 *                          and it stays VEILED. ⛔ Acquisition is SEEING, ⛔ not acting. The veil
 *                          breaks when the ⛔ BLOW LANDS (Attack above), ⛔ not when the target is
 *                          chosen. ⚠️ Breaking here would un-veil the unit a full approach early
 *                          and the card would read as broken.
 *
 *  ── BEING ACTED UPON — ⛔ HIS RULE BREAKS ON ***ACTING***, ⛔ NOT ON BEING ACTED UPON ──────────
 *    ⛔ TAKING DAMAGE    — ASummonedUnit::TakeDamage (SummonedUnit.cpp:4086),
 *                          AHeroCharacter::TakeDamage (HeroCharacter.cpp:721).
 *                          ⚠️ It ⛔ DOES interrupt an in-progress witch CAST (WITCH-§4, TASK-830)
 *                          — ⭐ a ⛔ DIFFERENT RULE ON A ⛔ DIFFERENT SUBJECT. ⛔ Do not merge them.
 *    ⛔ BEING HEALED     — ASummonedUnit::ApplyHealing (SummonedUnit.cpp:2665, HP mutation :2675).
 *                          ⭐ The HEALER breaks (Heal above); the ⛔ PATIENT ⛔ does not.
 *    ⛔ BEING BUFFED     — ApplyMoveSpeedBuff (:744) · SetAuraDamageBonus (:810) ·
 *                          ApplyCombatBuff (:1042) · AddPermanentDamageStacks (:881).
 *                          ⭐ Receiving a Rally, a War Banner pulse, a Battle Cry or an
 *                          ancient-ground stack is ⛔ something done TO the unit.
 *    ⛔ BEING FROZEN     — ApplyFreeze (SummonedUnit.cpp:916), FreezeAI (:661).
 *    ⛔ CAUGHT IN AN AoE — WITCH-§2 / J-W2: a veiled unit ⛔ IS caught by a blast and ⛔ stays
 *                          veiled. ⭐ A blast is not an act of seeing, and it is the card's counter.
 *    ⛔ MOVE BLOCKED     — NotifyMoveBlocked (SummonedUnit.cpp:3326) — records evidence, takes no action.
 *
 *  ── BEING COMMANDED — WITCH-§2's fourth lane; ⛔ an unorderable unit is a BUG ─────────────────
 *    ⛔ BEING ORDERED    — AssignCommandGroup (SummonedUnit.cpp:2111), ClearCommandGroup (:2145),
 *                          TryAutoEnrollInFollowGroup (:1318), the stance read (:1594-1612).
 *    ⛔ BEING SELECTED   — selection is ASiegePlayerController state; the unit does nothing.
 *    ⛔ BEING SPAWNED    — InitUnit (SummonedUnit.cpp:622). ⛔ A witch cannot veil at spawn anyway.
 *
 *  ── ⚖️ THE THREE JUDGMENT CALLS — ⛔ RULED HERE, ⛔ AND FLAGGED FOR JONATHAN ─────────────────
 *    ⚖️ CAPTURING A ZONE — ⛔ RULED: does ⛔ NOT break. ACaptureZone::EvaluateCapture
 *       (CaptureZone.cpp:136) is a ZONE-SIDE poll that reads only GetActorLocation() of every
 *       unit (:150-169) and hero (:171-190). ⛔ There is ⛔ no unit-side call, ⛔ no channel,
 *       ⛔ no progress bar — capture is PRESENCE, and presence is indistinguishable from standing.
 *       ⭐ Breaking here would un-veil ⛔ every unit that merely WALKS THROUGH a zone, which is the
 *       main route across the field — the card would be worthless. ⚠️ SIDE EFFECT TASK-829 MUST
 *       ACCEPT: a veiled unit ⛔ still captures, so a capture flipping with no visible cause is a
 *       real (and deliberate) information leak. ⛔ Do not "fix" it here; it is a design question.
 *    ⚖️ THE SORCERER'S EMPOWERMENT — ⛔ RULED: DOES break, as `Empower` above — ⛔ but see that
 *       enumerator's warning, because the sorcerer ⛔ executes no code and only the GROUND does.
 *    ⚖️ THE HERO'S RECALL CHANNEL — AHeroCharacter::BeginRecall (HeroCharacter.cpp:1457) /
 *       TickRecall (:1493) / EndRecall (:1525). ⛔ RULED: ⛔ out of scope, ⛔ not mapped to any
 *       enumerator. It is a real 3-part CHANNEL with its own attack-disarm (:1407 consumed at :490)
 *       and it is ⛔ HERO-ONLY, while WITCH-§4's target is "nearest friendly UNIT". ⇒ ⛔ unreachable
 *       today. ⚠️ IF TASK-830 EVER RULES THE HERO VEILABLE, recall (⛔ and Rally, ⛔ and the War
 *       Banner pulse) need a ruling — ⛔ recall does ⛔ not fit any of the six names, so it would be
 *       a MANAGER AMENDMENT to WITCH-§3, ⛔ never a quiet seventh enumerator.
 *    ⛔ ALSO OUT OF SCOPE, ⛔ MEASURED AND ⛔ NAMED: AHeroCharacter::ApplyUpgrade
 *       (HeroCharacter.cpp:953) — WITCH-§3 says the HeroUpgrade cards are ⛔ the hero's, ⛔ not a
 *       unit's · ABarracks::SpawnUnit (Barracks.cpp:80) — a ⛔ BUILDING spawns, units do not ·
 *       ADeepMine::TryRegisterIncome (DeepMine.cpp:60) — ⛔ flat building income, ⛔ zero unit code.
 *
 *  ── ⛔ VERBS SWEPT FOR AND ⛔ CONFIRMED ABSENT FROM THE SHIPPED GAME ─────────────────────────
 *    ⛔ taunt · ⛔ revive/resurrect · ⛔ unit-side repair (the Masons card is a PLAYER spell on the
 *    CASTLE — Castle.cpp:1218/:1414) · ⛔ shield/block/parry/dodge (the "Shield Wall" at
 *    SummonedUnit.cpp:1582 is a ⛔ STANCE, not an action) · ⛔ item pickup · ⛔ flag planting ·
 *    ⛔ unit-deployed siege ladders (AClimbableTower is a ⛔ pre-placed level actor) · ⛔ ram/batter
 *    as a distinct verb (Siege-typed MELEE, routes through SummonedUnit.cpp:3050) · ⛔ unit-side
 *    summoning · ⛔ unit-side building placement · ⛔ generic ability cooldowns.
 *    ⇒ ⭐ the six enumerators above are ⛔ COMPLETE against the shipped action set as measured
 *    2026-09-02. ⛔ A new card that adds a verb must come back through WITCH-§3.
 */

/**
 *  ⭐ THE PURE INVISIBILITY RULES (WITCH-§3 + WITCH-§6, TASK-827).
 *
 *  A `struct` with only statics — the FSiegeLadderClimbStatics / FSiegeLadderContactStatics
 *  shape named by WITCH-§6 as the precedent. ⛔ No GITCLAUDEUNREALTEST_API: every consumer
 *  (TASK-828's gatherer, TASK-829's ASummonedUnit, TASK-830's witch, the tests) lives in this
 *  same module, and an export macro nothing needs is surface pretending to be a contract.
 *  ⛔ Not reflected, ⛔ no .generated.h, ⛔ no UFUNCTION — WITCH-§6 says so explicitly.
 */
struct FSiegeInvisibilityStatics
{
	/**
	 *  ⭐⭐ THE ⛔ ONE SUPPRESSION PREDICATE (WITCH-§2). Given a VIEWER's team, the TARGET's team
	 *  and the target's veil flag ⇒ is the target visible to that viewer?
	 *
	 *  ⛔⛔ THE FRIENDLY LANE IS CHECKED ⛔ FIRST AND IT IS ⛔ UNCONDITIONAL. WITCH-§2's fourth
	 *  lane: FRIENDLY acquisition and the OWNER'S OWN ORDERS are ⛔ NEVER suppressed, because
	 *  ⛔ an invisible unit its own player cannot select, order or heal is a ⛔ BUG, ⛔ not a
	 *  feature. ⭐ This is the case a careless implementation gets wrong — it writes
	 *  `return !bTargetIsInvisible` and ships a unit its owner cannot command. The
	 *  Siegebound.Invisibility.FriendlyVeilNeverSuppressed test exists ⛔ solely to fail on that.
	 *
	 *  ⚠️ WHAT THIS PREDICATE IS ⛔ NOT FOR, so TASK-829 does not over-apply it:
	 *    ⛔ NOT for AoE. FSiegeCombatStatics::ApplyRadialDamage is ⛔ EXEMPT (WITCH-§2, J-W2) —
	 *      ⭐ a blast is ⛔ not an act of seeing, and it is the card's counter. A veiled unit
	 *      ⛔ IS caught by a blast.
	 *    ⛔ NOT for in-flight projectiles. AProjectile is target-locked at FIRE time
	 *      (Projectile.cpp:352) — ⭐ going invisible ⛔ does not delete an arrow already in the air.
	 *    ⛔ NOT for rendering. The player's own see-through look is WITCH-§5's material
	 *      (MI_Unit_Invisible), a ⛔ separate lane with ⛔ separate law.
	 *
	 *  ⛔ Pure: no world, no actor, no clock, ⛔ no side effect. Total over its inputs — every one
	 *  of the 2×2×2 input combinations is pinned by
	 *  Siegebound.Invisibility.VisibilityPredicateTruthTable.
	 *
	 *  @param ViewerTeam          the team DOING the looking (the acquirer)
	 *  @param TargetTeam          the team of the candidate being looked at
	 *  @param bTargetIsInvisible  the candidate's veil flag (TASK-829's `bIsInvisible`)
	 *  @return true if the viewer may see/acquire the target; false ⛔ only for a veiled ENEMY
	 */
	static bool IsVisibleTo(ETeamId ViewerTeam, ETeamId TargetTeam, bool bTargetIsInvisible);

	/**
	 *  ⭐ THE ⛔ ONLY WRITE-TRUE SITE. Grants the veil (TASK-830's cast COMPLETION, ⛔ never its start).
	 *
	 *  @param bIsInvisible  the unit's ONE source of truth (WITCH-§6), mutated in place
	 *  @return true if this call ⛔ actually veiled the unit; false if it was ⛔ already veiled
	 *          (⭐ which is the WITCH-§4 "never target an already-invisible unit" belt, and it makes
	 *          a double-veil a ⛔ measurable no-op rather than a silent one)
	 */
	static bool ApplyVeil(bool& bIsInvisible);

	/**
	 *  ⭐⭐ THE ⛔ ONLY WRITE-FALSE SITE, AND THE PLACE "PERMANENTLY" LIVES.
	 *
	 *  Clears the veil for one of the WITCH-§3 reasons. ⛔ IDEMPOTENT AND ⛔ MONOTONE: once false,
	 *  every later call is a no-op returning false. ⛔ There is ⛔ NO inverse — nothing in this
	 *  file, and nothing TASK-829 may write, turns the flag back on except a fresh
	 *  ApplyVeil from a ⛔ NEW witch cast. ⭐ That is his parenthetical, exactly: "(unless they are
	 *  later made invisible by a witch again)".
	 *
	 *  ⛔ THE `Reason` IS ⛔ NOT A CONDITION — it is a ⛔ LABEL. Every enumerator of the closed set
	 *  breaks, by construction (see the enum's absence comment: a non-breaking action has no
	 *  enumerator). ⭐ It is taken by value so TASK-829's ONE `BreakInvisibility(Reason)` method
	 *  can log WHICH act un-veiled a unit — the difference between a bug report that says
	 *  "invisibility is broken" and one that says "the Sapper's blast un-veiled it".
	 *  ⛔ A future edit that makes this return false for some Reason would be a ⛔ silent
	 *  behaviour change and is banned; add the gate at the CALL SITE if a card ever needs one.
	 *
	 *  @param bIsInvisible  the unit's ONE source of truth (WITCH-§6), mutated in place
	 *  @param Reason        which WITCH-§3 act did it (⛔ label only, ⛔ never a condition)
	 *  @return true ⛔ exactly once per veil — on the call that transitioned true→false. ⭐ This is
	 *          what lets TASK-829 fire its one-shot side effects (material swap, log) without a
	 *          "was visible" cache, which WITCH-§6 ⛔ forbids as a second source of truth.
	 */
	static bool ApplyBreak(bool& bIsInvisible, ESiegeVeilBreakReason Reason);

	/**
	 *  ⛔⛔ THE ABSENCE TRIPWIRE (see the enum comment). The number of enumerators in
	 *  ESiegeVeilBreakReason — WITCH-§3's closed set is ⛔ SIX and a seventh is a design change.
	 *  ⭐ Paired with ToString below, this makes "Walk was quietly added" a ⛔ SUITE FAILURE
	 *  rather than a comment somebody skimmed past. ⛔ Do ⛔ not bump this to make a test pass.
	 */
	static constexpr int32 VeilBreakReasonCount = 6;

	/**
	 *  Stable log token for a break reason — TASK-829's `BreakInvisibility` log line and the
	 *  census test's readable failure both use it. ⛔ Returns UnrecognisedReasonToken for any
	 *  value outside the closed set, which is ⛔ how the census test detects a seventh enumerator
	 *  (⭐ it asserts that index VeilBreakReasonCount is ⛔ STILL unrecognised).
	 */
	static const TCHAR* ToString(ESiegeVeilBreakReason Reason);

	/** What ToString returns for a value outside the closed set. Pinned so the census test can compare it. */
	static const TCHAR* UnrecognisedReasonToken();
};
