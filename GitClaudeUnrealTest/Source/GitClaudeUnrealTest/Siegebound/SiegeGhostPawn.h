// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Siegebound/TeamId.h"
#include "UObject/SoftObjectPtr.h"
#include "SiegeGhostPawn.generated.h"

class UAnimationAsset;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class USkeletalMesh;
class USpringArmComponent;

/**
 *  ═══ SIEGEBOUND GHOST PAWN (batch GHOST, TASK-749; law: CONVENTIONS `GHOST-§1`
 *      / `§3` / `§4` / `§5` / `§6`, `SC-§35`, `TOWER-§9.2`, `ACC-§8`) ═══
 *
 *  Jonathan's directive, verbatim, is the whole specification of this class:
 *  *"they instead get a ghost creature that they can control that is similar to
 *  their original body where it can command units, access the AI commander, and
 *  look at the map, it just cannot attack or be attacked. The enemy should be
 *  able to see this ghost as well."*
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐ THE ONE THING TO UNDERSTAND BEFORE EDITING THIS FILE: **"CANNOT BE
 *      ATTACKED" IS A STRUCTURAL PROPERTY OF THIS CLASS, ⛔ NOT A MECHANISM.**
 *
 *  ⛔ THERE IS NO `bInvulnerable`, ⛔ NO DAMAGE GUARD, ⛔ NO TARGETING FILTER, AND
 *  ⛔ NO SUPPRESSION CODE ANYWHERE IN THIS FILE — AND THERE MUST NEVER BE.
 *
 *  The reason is a single omission: **`ASiegeGhostPawn` does ⛔ NOT implement
 *  `ITeamAgent`.** Every hostile-actor selection in this project enumerates its
 *  candidate universe with
 *      `UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), …)`
 *  ⇒ **an actor outside that interface is never returned to any of them, so it
 *  can never be acquired, chained to, splashed, swept or struck.**
 *
 *  ⚠️ MEASURED AT SOURCE 2026-09-01 — this is a READING, ⛔ not an assumption.
 *  `GHOST-§1` names the first two; the remaining six were verified by TASK-749
 *  so that the safety claim covers **every** damage lane, ⛔ not merely the two
 *  the ruling happened to cite:
 *
 *    1. `ASummonedUnit::AcquireTarget`      — `SummonedUnit.cpp:1717`  (unit fleet)
 *    2. `ASummonedUnit::AcquireEnemyNearPoint` — `SummonedUnit.cpp:2262` (attack-zone orders)
 *    3. `ATower::AcquireTarget`             — `Tower.cpp:220`          (all four tower cards)
 *    4. `ATower` chain-zap selection        — `Tower.cpp:378`          (Crystal Tower bounces)
 *    5. `FSiegeCombatStatics::ApplyRadialDamage` — `SiegeCombatStatics.cpp:36` (⭐ EVERY AoE:
 *         Sapper suicide, Bomb Tower splash, Fireball — the lane that would otherwise
 *         catch a bystander who was never "acquired" at all)
 *    6. `USpellLibrary` resolver universe   — `SpellLibrary.cpp:64`    (spell targeting)
 *    7. `USpellLineSweep`                   — `SpellLineSweep.cpp:133` (line-sweep spells)
 *    8. `AHeroCharacter` melee acquisition  — `HeroCharacter.cpp:404`  (the enemy hero's own swing)
 *
 *  ⭐ AND THE TWO DAMAGE PATHS THAT ARE ⛔ NOT INTERFACE ENUMERATIONS WERE READ
 *  TOO, because a safety argument that only covers the easy paths is worthless:
 *    • `AProjectile` impact — `Projectile.cpp:352` applies damage to `Target.Get()`,
 *      an explicit pointer captured at FIRE TIME from an acquisition above. It is
 *      **target-locked, ⛔ never a collision sweep**, so a ghost standing in the
 *      flight path is not a victim and not a shield.
 *    • `AProjectile::FindTerrainHit` — `Projectile.cpp:434` line-traces
 *      `ECC_WorldStatic`/`ECC_WorldDynamic` **object types only** (its own comment:
 *      *"Pawn/Vehicle object types are NOT queried"*) and then requires a
 *      `Terrain`/`Obstacle` actor tag. ⇒ **the ghost is excluded TWICE**: it is an
 *      `ECC_Pawn` object and it carries neither tag.
 *      ⛔⛔ THIS IS WHY `GhostCapsuleObjectType` BELOW MUST STAY `ECC_Pawn`. Re-typing
 *      the capsule to `ECC_WorldStatic` — a naive way to read `G-2`'s "blocks
 *      WorldStatic" — would insert the ghost into that trace's hit list and hand a
 *      dead player a **projectile shield**. `G-2` is a RESPONSE ruling, ⛔ not an
 *      object-type ruling.
 *
 *  ⛔⛔ AND THE ALTERNATIVE THAT WAS REFUSED, RECORDED SO IT IS ⛔ NOT RE-PROPOSED
 *  AS A "SIMPLER" FIX (`GHOST-§1`): a `bIsGhost` flag on `AHeroCharacter` would
 *  force `ASummonedUnit::IsTargetAlive` to learn about it. A flag can be forgotten
 *  at a guard point; **an actor that is not in the candidate list cannot be
 *  forgotten anywhere.** ⭐ The better-designed shape and the schedulable shape
 *  were the same shape.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⭐ "CANNOT ATTACK" IS STRUCTURAL FOR THE SAME REASON, AND IT ALSO COST ZERO
 *     SUPPRESSION CODE — measured at `HeroCharacter.cpp:282-321`:
 *  The hero's offensive verbs (`IA_Attack` → `DoMeleeAttack`, `IA_Sprint`,
 *  `IA_Rally`) are bound in **`AHeroCharacter::SetupPlayerInputComponent`** — i.e.
 *  on the HERO PAWN. `SetupPlayerInputComponent` below binds **only Move and
 *  Look**. ⇒ while the ghost is possessed there is **no attack binding in
 *  existence to press**. ⛔ Do not add one, and ⛔ do not "restore parity" with the
 *  hero's bindings.
 *
 *  ⭐ CONVERSELY, EVERY POWER `G-3` GRANTS ARRIVES FOR FREE — measured at
 *  `SiegePlayerController.cpp:464-626`: the unit orders (`IA_CmdAttack`,
 *  `CmdHold`, `CmdDefend`, `CmdAmbush`, `CmdFollow`), the assistant console, the
 *  war map, the controls overlay and the card slots are bound on the **PLAYER
 *  CONTROLLER**, which does not change during a possession swap. Two further
 *  reads confirm the ghost is a first-class citizen of those systems:
 *    • `ASiegePlayerController::GetFollowAnchor` (`:3522`) returns `GetPawn()` as a
 *      plain `APawn*`; the `AHeroCharacter` cast there exists ONLY to apply the
 *      dead-hero gate. ⇒ ⭐ a possessed ghost becomes the follow anchor
 *      automatically, which is exactly what `G-8` wants and it costs no code.
 *    • `ASiegePlayerController::IsHeroInCommanderRange` (`:4819`) reads
 *      `const APawn* MyPawn = GetPawn()`. ⇒ ⭐ the ghost reaches the AI commander
 *      by standing near it, with no special case.
 *    • The player's TEAM lives on `ASiegePlayerState::GetTeam()` (`:127`),
 *      ⛔ **not** on the possessed pawn (`SiegePlayerController.cpp:4815`). ⇒ ⭐ order
 *      routing and commander lookup are entirely unaffected by the pawn swap, and
 *      **this is the deep reason the ghost needs no `ITeamAgent` and no team of its
 *      own to command anything.**
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ THE ONE THING THIS CLASS MUST ACTIVELY DO, AND IT IS `GHOST-§4`'s NAMED
 *      DISASTER: **RE-ADD THE INPUT MAPPING CONTEXT ON POSSESSION.**
 *  Measured at `HeroCharacter.cpp:228-280`: the mapping context is added by the
 *  **HERO PAWN** in `NotifyControllerChanged`, ⛔ not by `ASiegePlayerController`
 *  (whose own comment says so: *"ASiegePlayerController (TASK-007) does not add
 *  contexts the way the template controllers do"*). ⇒ **the instant the controller
 *  leaves the hero, nothing is re-adding IMC_Hero.** A ghost that did not do this
 *  itself would leave the player unable to move, order, or open the map **for the
 *  full `HeroRespawnDelay`** — the exact "boots the arena input-dead" failure
 *  `GHOST-§4` exists to prevent. `NotifyControllerChanged` below mirrors the
 *  hero's guard chain **including** the `KBD-§5`/`KBD-§6` positional-layout
 *  resolve, so the ghost is not accidentally QWERTY-only.
 *
 *  ⛔ CURSOR/INPUT-MODE OWNERSHIP IS ⛔ NOT TOUCHED HERE. `HELP-§5` binds it to
 *  `ApplyCursorInputState()` on the controller and **nowhere else**; this file
 *  contains no `SetInputMode`, no `bShowMouseCursor`, and no `EnableInput`/
 *  `DisableInput` call. The possession ordering is **TASK-750's**, by design.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ THE INITIALISATION-ORDERING TRAP — FOUND BY TASK-756, CLOSED BY TASK-758.
 *      **READ THIS BEFORE ADDING ONE LINE TO `BeginPlay`.**
 *
 *  ⚠️ MEASURED, ⛔ NOT ASSUMED (`SiegeGameMode.cpp:823-848`): the lifecycle owner
 *  runs `SpawnActor` → `InitializeGhost(Team)` → `Possess`, and **`BeginPlay`
 *  fires INSIDE `SpawnActor`.** ⇒ **`BeginPlay` runs BEFORE the team is ever
 *  set**, so `GhostTeam` is still its declaration default (`Blue`) for the whole
 *  of it. ⇒ ⛔⛔ **ANY TEAM-DEPENDENT WORK PLACED IN `BeginPlay` IS SILENTLY WRONG
 *  FOR RED, EVERY SINGLE TIME.** It compiles, it reviews as correct, it runs, and
 *  it is simply always Blue. ⚠️ It was **latent** — nothing team-dependent lived
 *  there yet — and the team tint a ghost material invites would have landed on
 *  exactly that line.
 *
 *  ✅ **THE FIX IS STRUCTURAL, ⛔ NOT A COMMENT SAYING "DON'T DO TEAM WORK IN
 *  `BeginPlay`".** `ApplyGhostTeamAppearance()` is **THE ONE PLACE TEAM-DEPENDENT
 *  INITIALISATION MAY LIVE**, and it is:
 *    • **GATED ON THE TEAM BEING KNOWN** (`bGhostTeamAssigned`) ⇒ it ⛔ **cannot
 *      fire at a moment that merely LOOKS like startup**; with no team assigned it
 *      does nothing at all rather than doing the wrong thing quietly, and
 *    • **CALLED FROM BOTH `BeginPlay` AND `InitializeGhost`** ⇒ **whichever of the
 *      two runs LAST is what drives it**, so the work always happens with the real
 *      team ⭐ **under EITHER ordering** — the shipped one
 *      (`SpawnActor`→`BeginPlay`→`InitializeGhost`) **and** a future deferred spawn
 *      (`SpawnActorDeferred`→`InitializeGhost`→`FinishSpawning`→`BeginPlay`).
 *
 *  ⛔⛔ **DO NOT ADD AN "ALREADY APPLIED" EARLY-OUT.** The re-run is ⛔ not
 *  redundancy — it is precisely what makes the second ordering correct, because
 *  the mesh whose material slots the step stamps **does not exist until
 *  `BeginPlay`** has assigned it.
 *  ⛔ **AND DO NOT MOVE TEAM-DEPENDENT WORK BACK INTO `BeginPlay`.**
 *  `FSiegeGhostPawnTeamKnownBeforeTeamWorkTest` goes red and names the reason.
 *
 *  ⚠️ **THE ALTERNATIVE THAT WAS WEIGHED AND REFUSED, RECORDED SO IT IS NOT
 *  RE-PROPOSED:** making the team a **spawn parameter** (deferred spawn, team set
 *  before `FinishSpawning`) is a legitimate shape, but it (a) requires editing
 *  `ASiegeGameMode`, which TASK-750 owns and TASK-758 was fenced out of, and
 *  (b) would fix only the **one caller that was edited** — a second caller written
 *  later with a plain `SpawnActor` would walk straight back into the trap.
 *  ⭐ The shape below is correct for **every** caller, present and future, and that
 *  is why it was chosen over the one that merely fixes today's call site.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⚖️ NET RELEVANCY TIER: **B — arena-scaled `SetNetCullDistanceSquared` from
 *  `SiegeNet::ArenaRelevancyDistanceSquared`** (declared per the CONVENTIONS NET
 *  RELEVANCY LAW declaration duty and `GHOST-§6`; set in the constructor, ⛔ never
 *  a hand-typed literal). ⛔ **"Nothing to declare" would be FALSE here and must
 *  not be copied from another batch's boilerplate**: this is a new pawn class,
 *  possessed by a player controller, that `G-4` requires the **ENEMY** to see.
 *  Rationale: identical to `AHeroCharacter`'s Tier B and deliberately the same
 *  tier — the ghost stands in for the hero, so an enemy who could see the hero
 *  across the 500 m arena must see the ghost across it too. Tier B, not A:
 *  ghosts are per-player, and a blanket always-relevant would not scale when P2
 *  adds the unit fleet to the same tier.
 *  ⛔ **NO RPC AND NO REPLICATED PROPERTY IS AUTHORED IN THIS BATCH** (`ACC-§8`'s
 *  reserved-not-authored discipline). The M8 shape is DECLARED and RESERVED:
 *  `GhostTeam` is the one property that would need `Replicated` + a
 *  `GetLifetimeReplicatedProps` registration when M8 lands, for exactly the
 *  reason `AHeroCharacter::Team` needs it — a client must know whose ghost it is
 *  to tint it. ⛔ A declaration is not an exemption and is not an authorization.
 *
 *  ─────────────────────────────────────────────────────────────────────────────
 *  ⚠️⚠️ DECLARED DEVIATION — **BASE CLASS: `ACharacter`, ⛔ NOT A DIRECT `APawn`.**
 *  `GHOST-§1` writes the shape as `ASiegeGhostPawn : public APawn`. This class
 *  derives from **`ACharacter`, which IS-A `APawn`** — so every statement the law
 *  makes about the type remains literally true, including `GHOST-§4`'s
 *  *"`ASiegeGhostPawn` is an `APawn`, so `TryGetPawnOwner()` resolves"*.
 *  ⭐ **REASON, AND IT IS `G-1`/`G-2` THEMSELVES:** `G-1` grants the hero's own
 *  movement and forbids flight; `G-2` requires it to *"walk the ground and not
 *  fall through the world"*. `UCharacterMovementComponent` — which delivers
 *  gravity, ground-walking, step-up and slope handling — **requires an
 *  `ACharacter` owner** and does not function on a bare `APawn`. The alternatives
 *  were weighed and refused: `UFloatingPawnMovement` **flies** (⛔ `G-1`), and
 *  hand-rolling gravity onto a bare `APawn` would mean re-implementing the engine
 *  movement component this project already relies on — strictly more code, more
 *  risk, and a worse ghost. `ACharacter` also supplies the
 *  `USkeletalMeshComponent` that `GHOST-§4` explicitly anticipates (*"If the
 *  ghost carries a `USkeletalMeshComponent`…"*) and the capsule `G-2` configures.
 *  ⭐ The law's binding content — **a pawn that does not implement `ITeamAgent`** —
 *  is honoured exactly, and `FSiegeGhostPawnIsAPawnTest` asserts the `APawn`
 *  claim **at the type** so it can never quietly stop being true.
 *
 *  ⚖️ THE THREE RULINGS TASK-749 WAS ASKED TO MAKE AND DECLARE (proceeding
 *     defaults; all three are one-word overrulable by Jonathan at TASK-755):
 *   • **Movement + vision (`G-1`)** — the hero's own, and **DERIVED, ⛔ not
 *     duplicated**: `BeginPlay` reads `GetDefault<AHeroCharacter>()` so the number
 *     cannot drift. ⛔ **No sprint**: sprint is a hero ability bound on the hero
 *     pawn, and `G-3`'s list is closed. The camera mirrors the hero's rig exactly
 *     (arm 400, `bUsePawnControlRotation`), so vision is identical — ⛔ not extended.
 *   • **Interaction (`G-3`)** — **orders only.** ⛔ No capture, ⛔ no interaction,
 *     ⛔ no pickups, ⛔ no card play (`G-5` default — FLAGGED, ⛔ not built either
 *     way). Enforced structurally: no such binding exists on this pawn.
 *   • **Collision (`G-2`)** — **BLOCKS `WorldStatic` ONLY; IGNORES `Pawn`.** ⭐ The
 *     asymmetry is the point: it needs a floor, and a ghost that blocked pawns
 *     would be a **free body-block wall handed to a dead player** — a combat
 *     effect on a pawn specified to have none.
 *
 *  ⚠️ TWO CONSEQUENCES OF `G-2` THAT ARE DECLARED RATHER THAN SILENTLY SHIPPED:
 *   (a) The ghost carries ⛔ **no team collision channel.** `AHeroCharacter`
 *       re-stamps its capsule to `SiegeTeamObjectChannel(GetTeamId())`
 *       (`HeroCharacter.cpp:113`) because those are the **combatant body**
 *       channels; a ghost is not a combatant body and must not wear one.
 *   (b) ⇒ `ACastle::GateBlockerVolume` — which ignores all channels and blocks
 *       only the enemy TEAM channel (`Castle.cpp:606-608`) — therefore **does not
 *       stop the ghost at either gate.** ⭐ Judged acceptable and FLAGGED for
 *       TASK-755: `G-3` makes the intrusion mechanically inert (nothing to
 *       capture, nothing to interact with), `G-4` makes it observable, and the
 *       castle's actual WALLS are `WorldStatic` and DO block it — so this is ⛔ not
 *       the wall-pass `G-1` forbids. Say the word and it is one line.
 *
 *  ⛔ FILES THIS TASK DID NOT TOUCH, AND MUST NOT: `SummonedUnit.{h,cpp}`,
 *  `Tower.{h,cpp}`, `ClimbableTower.{h,cpp}`, `HeroCharacter.{h,cpp}`,
 *  `SiegeGameMode.{h,cpp}`, `SiegePlayerController.{h,cpp}`. The `GHOST-§1`
 *  ruling is precisely what made that possible.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ASiegeGhostPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ASiegeGhostPawn();

	// ═════════════════════════════════════════════════════════════════════════
	//  THE API TASK-750 CONSUMES — and it is deliberately three calls, so the
	//  lifecycle owner cannot accidentally take on this class's concerns.
	//  ⛔ TASK-750 owns death → ghost → HeroRespawnDelay → respawn; this class
	//  owns ⛔ none of it and schedules ⛔ no timer of its own.
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  ① Call ONCE after `SpawnActor`, BEFORE `Possess`. Sets the ghost's owning
	 *  team for **identity and readability only**.
	 *
	 *  ⛔⛔ `InTeam` IS ⛔ NOT A COMBAT AFFILIATION AND MUST NEVER BECOME ONE. It
	 *  exists because `G-4` makes the ghost enemy-visible, and an observer who can
	 *  see a ghost needs to know **whose** it is. It feeds a material tint and
	 *  nothing else — ⛔ no targeting, ⛔ no friend/foe test, ⛔ no collision channel.
	 *  ⛔ **Exposing it by implementing `ITeamAgent` would destroy the entire
	 *  untargetability design** (`GHOST-§1`); `FSiegeGhostPawnNotATeamAgentTest`
	 *  fails loudly on the day someone tries.
	 *
	 *  ⭐⭐ **AND THIS IS THE MOMENT THE TEAM BECOMES KNOWN (TASK-758) — SO IT IS
	 *  THE MOMENT THE TEAM-DEPENDENT WORK RUNS.** ⛔ `BeginPlay` fires inside
	 *  `SpawnActor`, i.e. **before this call**, so team-dependent work placed there
	 *  would read the `Blue` default forever (see the ordering section at the top of
	 *  this file). This function therefore drives `ApplyGhostTeamAppearance()`
	 *  itself. ⛔ **Do not "tidy" that call away.**
	 *
	 *  Safe to call more than once, before or after possession, and **before or
	 *  after `BeginPlay`** — the team-dependent step is re-runnable on purpose, so
	 *  both orderings converge on the same correct result.
	 */
	void InitializeGhost(ETeamId InTeam);

	/**
	 *  ② Plain accessor — ⛔ deliberately ⛔ NOT `ITeamAgent::GetTeamId()`.
	 *  ⚠️ The name differs from the interface's on purpose: a distinct name keeps
	 *  a future reader from "completing the pattern" by adding the interface.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Ghost")
	ETeamId GetGhostTeam() const { return GhostTeam; }

	/**
	 *  ③ Teardown. Call at respawn AND at match end (`GHOST-§2`: match end retires
	 *  the ghost, the end screen shows, `PlayAgain()` restores the hero — ⛔ that
	 *  is the SHIPPED rule inherited unchanged, ⛔ not a second match-end rule).
	 *
	 *  **IDEMPOTENT and safe on an already-destroyed or never-possessed ghost**,
	 *  so the caller never needs a "did I already retire it?" flag.
	 *  ⚠️ It ⛔ does NOT un-possess and ⛔ does NOT re-possess the hero: possession
	 *  ordering is `GHOST-§4`'s law and belongs to TASK-750, which must re-possess
	 *  the hero FIRST and retire the ghost SECOND — retiring a **possessed** pawn
	 *  would leave the controller pawnless.
	 *  ⛔ This class owns no timer handle, so there is nothing here for
	 *  `PlayAgain()`'s clear-only-what-this-class-owns policy to leak.
	 */
	void RetireGhost();

	// ═════════════════════════════════════════════════════════════════════════
	//  ⛔ NOT PART OF THE THREE-CALL LIFECYCLE API ABOVE — an **OBSERVABILITY**
	//  read, added by TASK-758 so the initialisation ORDERING is measurable
	//  instead of merely asserted in prose (`SC-§37`: where correctness is
	//  invisible to review, the test must measure the PROPERTY).
	//  ⛔ TASK-750 neither calls this nor needs to; ⛔ do not build lifecycle
	//  behaviour on it.
	// ═════════════════════════════════════════════════════════════════════════

	/**
	 *  Returns **true once the team-dependent initialisation step has actually
	 *  run**, and writes into `OutAppliedTeam` **the team that step OBSERVED**.
	 *
	 *  ⭐ THE DISTINCTION IS THE WHOLE POINT AND IT IS ⛔ NOT PEDANTRY:
	 *  `GetGhostTeam()` says what the team **is now**; this says what the team
	 *  **was at the instant team-dependent work happened**. Under the pre-TASK-758
	 *  ordering those two differed on every Red ghost, and ⛔ nothing could see it.
	 *  ⚠️ `OutAppliedTeam` is **meaningless when this returns false** — `ETeamId`
	 *  has ⛔ no "unset" value (`TeamId.h`), which is exactly why the bool exists
	 *  rather than a sentinel team.
	 */
	bool HasAppliedTeamAppearance(ETeamId& OutAppliedTeam) const;

protected:
	virtual void BeginPlay() override;

	/**
	 *  ⚠️⚠️ THE INPUT-DEAD GUARD (`GHOST-§4`). Re-adds the mapping context when this
	 *  pawn is possessed, because `ASiegePlayerController` does not (measured:
	 *  `HeroCharacter.cpp:228-280` is where the hero does it for itself).
	 *  Null-safe at every step, and layout-correct via `KBD-§5`/`KBD-§6`.
	 */
	virtual void NotifyControllerChanged() override;

	/** Binds **Move and Look only** — `G-3`'s closed list, enforced by omission. */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ── components (the hero's rig, mirrored — `G-1` "similar to their original body") ──

	/** Spring arm mirroring the hero's (`GitClaudeUnrealTestCharacter.cpp:39-42`): length 400, uses control rotation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Ghost")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera mirroring the hero's (`:45-47`): does NOT use control rotation — the boom already does. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Ghost")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ── input (assigned on BP_SiegeGhostPawn) ──

	/**
	 *  ⚠️⚠️ ASSIGN **`/Game/Input/IMC_Hero`** — the SAME context the hero uses.
	 *  ⭐ That is the whole reason the ghost can command units, open the war map and
	 *  reach the assistant console: those actions are bound on the PLAYER
	 *  CONTROLLER (`SiegePlayerController.cpp:464-626`) and only need the context
	 *  to be present. ⛔ A separate ghost-only context would silently drop every one
	 *  of `G-3`'s powers, which is the failure this comment exists to prevent.
	 *  ⛔ Unassigned ⇒ warn once at possession; ⛔ never a crash.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Input")
	TObjectPtr<UInputMappingContext> GhostMappingContext;

	/** `IA_Move` — assigned on BP_SiegeGhostPawn. Unassigned ⇒ warn once, ghost cannot walk. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** `IA_Look` (gamepad) — assigned on BP_SiegeGhostPawn. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Input")
	TObjectPtr<UInputAction> LookAction;

	/** `IA_MouseLook` — assigned on BP_SiegeGhostPawn. Mirrors the template's twin look bindings. */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	// ── appearance (`G-4`: the ENEMY must be able to see this) ──

	/**
	 *  Skeletal mesh for the ghost body. ⚠️ **`G-4` MAKES THIS FUNCTIONALLY
	 *  REQUIRED, ⛔ not cosmetic**: an unset mesh is an INVISIBLE ghost, which
	 *  violates Jonathan's explicit *"The enemy should be able to see this ghost
	 *  as well."* ⇒ `BeginPlay` **warns loudly** when it is unset rather than
	 *  shipping a silent invisibility.
	 *  ⛔ Deliberately left UNSET in C++: ⛔ no art task in this batch produces a
	 *  ghost mesh, and inventing an asset path the Artist never agreed to would
	 *  break the CONVENTIONS name-match contract. ⭐ **An art task is OWED** —
	 *  flagged in the TASK-749 handoff. Assign the hero's own mesh on
	 *  BP_SiegeGhostPawn as the zero-art interim, which is also the most faithful
	 *  reading of *"similar to their original body."*
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Appearance")
	TSoftObjectPtr<USkeletalMesh> GhostMesh;

	/**
	 *  Translucent ghost material applied to every slot. Unset ⇒ the mesh's own
	 *  materials (visible, just not ghostly).
	 *  ⚠️ **APPLIED BY `ApplyGhostTeamAppearance()`, ⛔ NOT BY `BeginPlay` (TASK-758).**
	 *  ⭐ That is deliberate and it is the ordering fix: this material is **the seat
	 *  of the team tint** `G-4` readability wants, so it must be stamped at the
	 *  moment the team is KNOWN — ⛔ never at a moment that merely looks like
	 *  startup. In the shipped path the two instants are microseconds apart and
	 *  both precede `Possess`, so ⛔ nothing about what the player sees changes.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Appearance")
	TSoftObjectPtr<UMaterialInterface> GhostMaterial;

	/**
	 *  Idle animation, played by **SINGLE-NODE PLAYBACK**.
	 *  ⛔⛔ `SC-§35` / `GHOST-§4`: this is a `UAnimationAsset`, ⛔ deliberately ⛔ NOT a
	 *  `TSoftClassPtr<UAnimInstance>`. The sanctioned shape for a prop that merely
	 *  idles is `SetAnimationMode(AnimationSingleNode)` + `PlayAnimation(…, true)`,
	 *  which is **structurally incapable** of the owner-class mismatch defect —
	 *  there is no ABP, so there is no `TryGetPawnOwner()` to resolve wrongly.
	 *  ⚠️ `SC-§35` was bought by 1,806 Blueprint runtime errors that compiled clean
	 *  and cleared TWO QA gates. ⛔ Do not "upgrade" this to an Anim Blueprint
	 *  without re-reading that law and stating the assumed owner class at the
	 *  assignment.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Ghost|Appearance")
	TSoftObjectPtr<UAnimationAsset> GhostIdleAnimation;

private:
	/** Move input handler — mirrors `AGitClaudeUnrealTestCharacter::Move` (control-rotation-relative planar movement). */
	void Move(const struct FInputActionValue& Value);

	/** Look input handler — mirrors the template's. `G-1`: the hero's own vision, ⛔ nothing extended. */
	void Look(const struct FInputActionValue& Value);

	/**
	 *  ⛔⛔ **THE ONE PLACE TEAM-DEPENDENT INITIALISATION MAY LIVE (TASK-758).**
	 *  Read the ordering section at the top of this file before touching it.
	 *
	 *  Runs its body **only once the team is actually KNOWN** and is called from
	 *  **BOTH** `BeginPlay` **and** `InitializeGhost`, so whichever runs last drives
	 *  it. ⛔ **It is deliberately RE-RUNNABLE — do not add an "already applied"
	 *  early-out**; the re-run is what makes the `InitializeGhost`-before-`BeginPlay`
	 *  ordering correct, because the mesh it stamps does not exist until `BeginPlay`.
	 */
	void ApplyGhostTeamAppearance();

	/**
	 *  Owning team — **identity/readability only** (see `InitializeGhost`).
	 *  ⛔⛔ NOT a combat affiliation, ⛔ NOT exposed through `ITeamAgent`, and ⛔ NOT
	 *  stamped onto any collision channel.
	 *  📌 M8 (`GHOST-§6`, `ACC-§8`): this is the ONE property that would take
	 *  `Replicated` + a `GetLifetimeReplicatedProps` registration when M8 lands.
	 *  ⛔ RESERVED, ⛔ NOT AUTHORED in this batch.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Siegebound|Ghost")
	ETeamId GhostTeam = ETeamId::Blue;

	/**
	 *  ⛔⛔ **THE GATE THAT MAKES THE ORDERING TRAP UNABLE TO FIRE (TASK-758).**
	 *  False until `InitializeGhost()` has been called ⇒ `ApplyGhostTeamAppearance()`
	 *  does **nothing at all** rather than doing the wrong thing quietly.
	 *  ⚠️ It exists because **`ETeamId` has no "unset" value** (`TeamId.h` declares
	 *  `Blue` and `Red` only), so `GhostTeam == Blue` is genuinely ambiguous between
	 *  *"a Blue ghost"* and *"nobody has told me yet"* — and that ambiguity **is**
	 *  the defect. ⛔ Do not "simplify" this away by adding an `ETeamId::None`:
	 *  every team switch in the project would have to grow a third arm.
	 */
	bool bGhostTeamAssigned = false;

	/** True once `ApplyGhostTeamAppearance()` has actually run its body. */
	bool bTeamAppearanceApplied = false;

	/**
	 *  ⭐ The team `ApplyGhostTeamAppearance()` **actually observed** when it ran.
	 *  ⛔ Meaningless unless `bTeamAppearanceApplied`. This is what makes the
	 *  ordering **measurable** rather than merely claimed: under the old ordering
	 *  this would have read `Blue` on a Red ghost, and ⛔ nothing anywhere could
	 *  have noticed.
	 */
	ETeamId AppliedAppearanceTeam = ETeamId::Blue;

	/** Guard so `RetireGhost()` is idempotent and logs exactly once. */
	bool bRetired = false;
};
