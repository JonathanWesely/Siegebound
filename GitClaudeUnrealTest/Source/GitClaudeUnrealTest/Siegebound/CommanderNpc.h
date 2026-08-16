// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/TeamId.h"
#include "CommanderNpc.generated.h"

class UAnimInstance;
class UAnimSequence;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 *  Siegebound COMMANDER NPC (batch WAR-ROOM, TASK-559; law: CONVENTIONS
 *  WR-§5 + WR-§7 + WR-§8). Jonathan's directive, verbatim: "add an NPC
 *  character that you can talk to that will be the avatar for the AI model
 *  that we talk to that commands units."
 *
 *  ⛔⛔ AN AVATAR, NOT A SECOND AI — THE ONE SENTENCE THIS CLASS EXISTS TO
 *  OBEY (WR-§5). There is ONE assistant: one USiegeAssistantComponent, one
 *  prompt, one model, one console widget. This actor is a BODY the player
 *  walks up to and a TABLE he reads. It holds NO conversation state, NO
 *  prompt, NO model handle, and NO reference of any kind into the assistant.
 *  ⛔ THE CHECKABLE FORM OF THAT CLAIM, stated so a reviewer can run it rather
 *  than trust it: grep CommanderNpc.{h,cpp} for "Assistant" / "Prompt" /
 *  "ZoneA" / "Llama" / "SpendGold" and every hit lands inside a COMMENT — zero
 *  #includes, zero symbols, zero code lines. That is a QA-checked invariant,
 *  not a preference. A future task that gives this class a prompt, a model, or
 *  a parallel conversation has misread WR-§5.
 *
 *  ⛔⛔ THE NPC IS ADDITIVE AND NEVER A GATE. Jonathan, in terms: "the console
 *  still works anywhere." ⇒ NOTHING in this class may ever be consulted by
 *  the console's open path, and this class deliberately imports nothing that
 *  could reach it. The proximity gate this actor supplies (InteractRadius)
 *  belongs to the WAR MAP ONLY (TASK-563). Anyone who later "fixes" the
 *  design by routing orders through the NPC has deleted a shipped feature
 *  (WR-§5 RULING 5, WR-§9 outcome 7).
 *
 *  ⚖️ NET RELEVANCY TIER: **C — NOT REPLICATED** (declared per the CONVENTIONS
 *  NET RELEVANCY LAW declaration duty + WR-§8; "Tier not declared" is a QA
 *  FAIL and Tier C is a DECLARATION, not an exemption). `bReplicates` stays at
 *  the AActor default (false) and is never set. Rationale: this actor carries
 *  NO gameplay truth. It is a purely local/cosmetic prop — a mesh, a table,
 *  and two EditDefaultsOnly numbers that are IDENTICAL on both machines
 *  because they are class defaults, not state. ACastle::BeginPlay runs on the
 *  server AND on the client, so TASK-562 spawns one LOCAL copy per machine and
 *  pushes the castle's already-replicated Team into it — exactly the
 *  AAncientGround / AGoldNode precedent (a local projection of replicated
 *  truth). This class adds NO replicated property, NO new replicated class,
 *  NO new relevancy tier and NO RPC. The batch's two RPCs
 *  (ServerRequestEnemyReveal / ClientReceiveEnemyReveal) live on
 *  ASiegePlayerController and belong to TASK-563, because GOLD is
 *  authority-owned and this actor never touches gold.
 *
 *  ⚠️ WHY AActor AND NOT APawn/ACharacter — LOAD-BEARING, DO NOT "UPGRADE" IT.
 *  A Pawn brings a capsule (a blocking primitive that would carve the castle's
 *  interior navmesh in the grand hall the units now spawn into and walk
 *  through), an auto-possession surface, and a controller the AI systems would
 *  have to learn to ignore. The NPC never moves, never fights and never
 *  pathfinds; a plain AActor is the honest shape.
 *
 *  ⛔⛔ AND THE "MAKE IT A PAWN" TEMPTATION IS REFUSED IN ADVANCE, BY NAME,
 *  BECAUSE IT HAS ALREADY BEEN PROPOSED ONCE (TASK-591 / SC-§35 item 6). The
 *  proposal arrives dressed as a bug fix — "make the actor a Pawn so the
 *  AnimBlueprint is happy" — and it is the WRONG DIRECTION twice over. It
 *  inverts the dependency (the prop bends to the asset), and on THIS codebase
 *  it would additionally drop a decorative prop into every pawn-shaped query in
 *  the project — GetPawnIterator, AI perception, targeting — i.e. it would buy
 *  an idle animation at the price of making the commander ACQUIRABLE AS A
 *  COMBAT TARGET. That is the ITeamAgent failure measured below, arriving
 *  through a second door. ⇒ ⛔ FIX THE ASSIGNMENT, ⛔ NEVER THE HIERARCHY.
 *
 *  ⛔ BLOCKS NOTHING, CARVES NOTHING. Both meshes ship on the NoCollision
 *  profile with SetCanEverAffectNavigation(false) — the AGoldNode "blocks
 *  NOTHING" posture, applied for the same reason: the NPC and its table stand
 *  INSIDE the castle interior, which is navigable space the units spawn in and
 *  the hero walks through, and a blocking prop there is a traversability
 *  hazard, not a feature. ⚠️ DESIGNED, FLAGGED CONSEQUENCE, NOT A BUG: the
 *  player can walk THROUGH the war table in v1. Making the table solid is a
 *  one-line change (a Blockall profile on WarTableMesh) and is Jonathan's
 *  call — it is deliberately NOT taken unasked, because it trades a cosmetic
 *  wrinkle for a navmesh risk.
 *
 *  ⛔⛔ IT DELIBERATELY DOES **NOT** IMPLEMENT ITeamAgent — DIAGNOSED, NOT
 *  ASSUMED (the TASK-559 spec required the diagnosis before the decision).
 *  THE MEASUREMENT: eight shipped call sites treat "is an ITeamAgent" as
 *  "is a legal combat target" —
 *      HeroCharacter.cpp:404 (melee cone) · SummonedUnit.cpp:1536 (AcquireTarget)
 *      · SummonedUnit.cpp:2053 · Tower.cpp:220 · Tower.cpp:378 ·
 *      SpellLibrary.cpp:64 · SpellLineSweep.cpp:133 · SiegeCheatManager.cpp:130
 *  — every one of them a full-world GetAllActorsWithInterface(UTeamAgent).
 *  AND THE SECOND HALF IS WHAT MAKES IT WORSE THAN "IT GETS SHOT":
 *  ASummonedUnit::IsTargetAlive (SummonedUnit.cpp:3528) ends with "unknown
 *  ITeamAgent types have no death API yet — treat as alive", so an
 *  ACommanderNpc implementing the interface would be a PERMANENTLY-ALIVE,
 *  UNKILLABLE aggro sink standing in the enemy castle's grand hall: enemy
 *  units would walk in, attack it forever, and never re-target. This is the
 *  EXACT hazard AGoldNode records at GoldNode.h:78 ("deliberately does NOT
 *  implement ITeamAgent — unit acquisition scans ITeamAgent actors, so
 *  implementing it would make enemy units target the mine"), and it is the
 *  reason that class REMOVED its old GetTeam accessor outright.
 *  ⇒ THE TEAM ACCESSOR IS NAMED **GetCommanderTeam()**, NOT GetTeamId(), and
 *  the rename is the control: a non-virtual GetTeamId() on a class that does
 *  not implement the interface is an invitation to write
 *  Cast<ITeamAgent>(Npc)->GetTeamId() and get a null cast. DECLARED DEPARTURE
 *  from the spec's suggested name (SC-§15) — the spec conditioned it on the
 *  diagnosis, and the diagnosis says no.
 *
 *  PLACEMENT — ⛔ NEVER LEVEL-PLACED (WR-§5). ACastle spawns this actor at
 *  BeginPlay from its CommanderNpcAnchor and attaches it to CastleMesh
 *  (TASK-562, which also owns the destroy/respawn lifecycle). Three
 *  load-bearing reasons, identical to the torches': L_Arena is under standing
 *  orders never to be saved; both castles get identical furnishing BY
 *  CONSTRUCTION with no mirror step to get wrong; and the anchor moves with
 *  the castle actor forever. This class does not know where it is and must
 *  never learn.
 *
 *  ASSETS — soft, null-safe, resolved twice (OnConstruction for the editor
 *  viewport, BeginPlay for the runtime), exactly the AGoldNode discipline. A
 *  missing asset means an invisible-but-functional NPC and one log line, NEVER
 *  a crash: /Game/Characters/SK_Sorcerer ships today; /Game/Meshes/SM_WarTable
 *  is authored in parallel (TASK-556) and imported at TASK-566, so this class
 *  is written to survive its absence.
 *
 *  ⚠️ D5 — WHAT THE COMMANDER LOOKS LIKE — IS STILL JONATHAN'S TO DECIDE, AND
 *  THIS CLASS IS BUILT NOT TO FORECLOSE IT. WR-§5 rules the shipped
 *  SK_Sorcerer in FOR THIS BATCH on cost grounds (zero Meshy credits, zero rig
 *  work, already on the shared SK_Footman_Skeleton) — that is a BATCH ruling,
 *  ⛔ not an answer to the design question. A bespoke commander model is a
 *  RECORDED FOLLOW-UP, ⛔ never smuggled in here. Swapping it later is ONE soft
 *  path (AvatarMeshAsset) in BP_CommanderNpc and touches no logic at all, which
 *  is precisely what keeps his choice open and free.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API ACommanderNpc : public AActor
{
	GENERATED_BODY()

public:

	ACommanderNpc();

	/**
	 *  THE TEAM PUSH (the house Init<Thing> law — InitBuilding / InitUnit /
	 *  InitMine / InitAncientGround). Called by ACastle immediately after
	 *  spawning this NPC (TASK-562), threading the castle's own Team so the
	 *  NPC is owned by the castle it stands in and BOTH castles are furnished
	 *  identically by construction.
	 *
	 *  ⚠️ PUSHED, NEVER DERIVED — do NOT "simplify" this into a GetOwner()
	 *  walk or a nearest-castle search. A search would be wrong the moment the
	 *  two castles are close enough to confuse (and would silently pick the
	 *  ENEMY castle on the mirrored side), and it would make this actor
	 *  depend on ACastle, which it deliberately does not include.
	 *
	 *  Safe before or after BeginPlay; safe to call more than once. Defaults
	 *  to Blue if never called, per the TASK-559 spec.
	 */
	void InitCommanderNpc(ETeamId InTeam);

	/**
	 *  Which team owns this NPC — i.e. whose player may interact with its war
	 *  table (WR-§5: "only the OWNING team's player may interact").
	 *
	 *  ⚠️ DELIBERATELY **NOT** NAMED GetTeamId(), AND THIS CLASS DELIBERATELY
	 *  DOES NOT IMPLEMENT ITeamAgent — see the class doc for the eight-call-site
	 *  measurement and the unkillable-aggro-sink failure that name would invite.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|CommanderNpc")
	ETeamId GetCommanderTeam() const { return CommanderTeam; }

	/**
	 *  2D (XY) proximity test against InteractRadius, measured from this
	 *  actor's origin (where the avatar stands). Z is IGNORED — the house
	 *  arena metric (FVector::DistSquared2D vs FMath::Square(Radius), matching
	 *  AGoldNode::FindBestMineFor, AAncientGround::IsPointInZone and the
	 *  miner's arrival test), so a player on the interior floor's raised
	 *  threshold still counts.
	 *
	 *  ⛔ THIS IS THE WAR MAP'S GATE AND NOTHING ELSE'S. TASK-563 calls it to
	 *  decide whether M opens the map. ⛔ It may NEVER be consulted by the
	 *  assistant console's open path (WR-§5 RULING 5). Pure and read-only: it
	 *  mutates nothing and reads no authority.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|CommanderNpc")
	bool IsPlayerInRange(const FVector& PlayerLocation) const;

	/** Interaction radius in uu — the war map's proximity gate (see InteractRadius). Read by TASK-563. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|CommanderNpc")
	float GetInteractRadius() const { return InteractRadius; }

	/** Gold price of one enemy reveal (see EnemyRevealCost). Read by TASK-563, which owns the SPEND. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|CommanderNpc")
	int32 GetEnemyRevealCost() const { return EnemyRevealCost; }

	/**
	 *  ⭐ ADDITION OVER THE TASK-559 SPEC, DECLARED NOT SILENT (SC-§15). The
	 *  spec's names: block does not list a finder; this is added because the
	 *  house law puts a finder on the class being FOUND
	 *  (AGoldNode::FindBestMineFor · AAncientGround::FindNearestAncientGround ·
	 *  ACastle::FindNearestCastleForTeam), and its named consumer already
	 *  exists on the board: TASK-563 must locate the OWN-TEAM NPC to run the
	 *  proximity gate and to read EnemyRevealCost on the authority. Shipping it
	 *  here keeps a TActorIterator out of ASiegePlayerController. ⚠️ Shipping
	 *  with zero call sites is legal and intended (SC-§33's own scope note,
	 *  FSiegeAssistantRegionStatics::IsPointInRegion precedent); TASK-563 is
	 *  free to ignore it.
	 *
	 *  Returns the FIRST valid ACommanderNpc whose GetCommanderTeam() == Team.
	 *  There is exactly ONE per castle and one castle per team, so "first" is
	 *  "the only one" by construction; TActorIterator's order is stable for a
	 *  fixed world, so the answer does not churn between calls.
	 *
	 *  A null World, or a world with no NPCs at all (a fallback boot, a map
	 *  without castles, a castle whose spawn was refused), answers nullptr and
	 *  NEVER crashes — the caller degrades to today's behaviour. The NPC is
	 *  castle-spawned and destroyed/respawned across Play Again, so ALWAYS
	 *  ASK, ⛔ never cache the pointer across a match reset.
	 */
	static ACommanderNpc* FindCommanderNpcForTeam(UWorld* World, ETeamId Team);

	/** Resolves the soft meshes in-editor so the NPC is visible while placed/previewed (null-safe, silent). */
	virtual void OnConstruction(const FTransform& Transform) override;

protected:

	/** Runtime mesh resolve, THEN the anim ladder — ⛔ the only place animation is ever applied (TASK-591 (d)). */
	virtual void BeginPlay() override;

	/**
	 *  Scene root. The avatar and the table hang off it, so the actor's origin
	 *  is the ANCHOR POINT TASK-562 places — a floor point in the grand hall
	 *  with the actor's +X facing into the room. No collision, no nav.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|CommanderNpc")
	TObjectPtr<USceneComponent> SceneRoot;

	/**
	 *  The avatar body — the thing the player walks up to. Skeletal, because the
	 *  re-used SK_Sorcerer is a rigged fleet asset on the shared
	 *  SK_Footman_Skeleton, so a fleet idle CLIP can drive it directly (see
	 *  AvatarIdleAnimAsset). Empty until AvatarMeshAsset resolves; an unresolved
	 *  mesh is an invisible NPC, never a crash.
	 *  ⚠️ CORRECTED 2026-08-16 (TASK-591) — the retired sentence is named so the
	 *  correction is recognisable: this comment used to say the shared locomotion
	 *  ABP "gives it an idle for free". ⛔ IT DID NOT. It gave 1,806 Blueprint
	 *  runtime errors in 49 s, because that ABP resolves its owner as a Pawn and
	 *  this actor is an AActor (SC-§35). The idle now comes from a SINGLE-NODE
	 *  clip, which runs no graph and therefore asks nothing about its owner.
	 *  ⛔ NoCollision + no nav influence — see the class doc.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|CommanderNpc")
	TObjectPtr<USkeletalMeshComponent> AvatarMesh;

	/**
	 *  The war table — the map's physical home in the fiction. ONE ACTOR OWNS
	 *  BOTH (WR-§5): they are one interactable object to the player and one
	 *  lifecycle in code, so the castle's destroy/respawn (TASK-562) can never
	 *  leave a table without its commander or the reverse.
	 *  SM_WarTable's origin is its FLOOR-CONTACT plane (min-Z ≈ 0, TASK-556
	 *  spec), and SK_Sorcerer is feet-origin, so both sit on the floor at
	 *  relative Z = 0 with no per-asset Z fudge.
	 *  ⛔ NoCollision + no nav influence — see the class doc.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|CommanderNpc")
	TObjectPtr<UStaticMeshComponent> WarTableMesh;

	/**
	 *  How close the player must stand for the war map to open (TASK-563's
	 *  gate). 400 uu.
	 *
	 *  ⚠️⛔ THIS IS AN INVENTED NUMBER AND THIS COMMENT DECLARES IT AS ONE —
	 *  AND IT DOES *NOT* VIOLATE `AS-§21.4`. THE DISTINCTION IS LOAD-BEARING
	 *  AND IS WRITTEN DOWN HERE SO NOBODY CITES THAT LAW TO BLOCK THIS ONE,
	 *  AND SO NOBODY CITES THIS ONE TO JUSTIFY A PLACE RADIUS (WR-§5, pasted):
	 *  `AS-§21.4` forbids inventing a radius for a PLACE SYMBOL, because such a
	 *  radius becomes a SEMANTIC CLAIM THE MODEL REASONS OVER and a wrong value
	 *  SILENTLY MIS-SELECTS UNITS. An INTERACTION radius is a UI AFFORDANCE THE
	 *  PLAYER FEELS DIRECTLY, tunes by walking, and CAN NEVER MIS-SELECT
	 *  ANYTHING. ⇒ permitted, and FLAGGED to Jonathan as a feel tunable.
	 *
	 *  ⚠️ HUMAN-SCALE — DO NOT SCALE IT WITH THE CASTLE (SC-§34's human-scale
	 *  exemption, WR-§1): this number is keyed to a BODY walking up to a table,
	 *  not to the castle's bounds. The castle went 9× in volume this batch and
	 *  this radius is deliberately UNCHANGED by that; multiplying it would be
	 *  the defect.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|CommanderNpc", meta = (ClampMin = "0"))
	float InteractRadius = 400.f;

	/**
	 *  Gold price of ONE enemy reveal on the war map — Jonathan's own number:
	 *  "You can pay 30 gold to reveal all enemy locations."
	 *
	 *  ⛔ A MECHANIC RULE ⇒ A UPROPERTY DEFAULT, ⛔ NEVER A cards.csv COLUMN
	 *  (the GDD §3.0 "mechanic rules aren't card stats" law). ⛔ THIS CLASS
	 *  ONLY HOLDS THE NUMBER — it never reads a balance, never calls
	 *  ASiegePlayerState::SpendGold, and never touches gold in any direction.
	 *  The spend is initiated by the PLAYER'S OWN CLICK and executed on the
	 *  AUTHORITY in TASK-563 (WR-§7), which is what keeps the standing ruling
	 *  "THE AI NEVER SPENDS GOLD" true.
	 *  // GDD §3.15
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|CommanderNpc", meta = (ClampMin = "0"))
	int32 EnemyRevealCost = 30; // GDD §3.15

	/**
	 *  Soft reference to the avatar body: /Game/Characters/SK_Sorcerer — the
	 *  SHIPPED fleet rig, re-used by ruling (WR-§5 / D5). Null-safe on load;
	 *  clearing it in a child/instance is a silent designer opt-out (the
	 *  AttackImpactEffect IsNull pattern, TASK-020). Swapping in a bespoke
	 *  commander model later is an edit to THIS field and nothing else.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc")
	TSoftObjectPtr<USkeletalMesh> AvatarMeshAsset;

	/**
	 *  Soft reference to the war table: /Game/Meshes/SM_WarTable (authored
	 *  TASK-556, imported TASK-566 — it does NOT exist as this file is
	 *  written, which is exactly why the resolve is null-safe and logged once).
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc")
	TSoftObjectPtr<UStaticMesh> WarTableMeshAsset;

	/**
	 *  OPTIONAL AnimBlueprint class for the avatar body. ⛔⛔ IT SHIPS EMPTY, ⛔ ON
	 *  PURPOSE, AND THE EMPTINESS IS THE FIX (TASK-591 / SC-§35).
	 *
	 *  ⛔⛔ THE OWNER-CLASS CONTRACT, STATED BECAUSE AN UNSTATED HALF IS AN
	 *  UNCHECKED HALF: an AnimBlueprint is a CONTRACT WITH ITS OWNER'S CLASS, ⛔
	 *  not a decoration on a skeletal mesh. It runs a graph against an owner, and
	 *  the owner's class is half of what that graph assumes. ⇒ ⭐ THE CONTRACT
	 *  THIS FIELD DEMANDS: the assigned ABP's graph must make ⛔ NO PAWN
	 *  ASSUMPTION about its owner. ACommanderNpc is an AActor and (see the class
	 *  doc, where the conversion is refused with reasons) will stay one — so ANY
	 *  node that resolves the owner as a Pawn (TryGetPawnOwner, Get Pawn Owner, a
	 *  CharacterMovement read) returns None here, every frame, on every instance.
	 *
	 *  ⛔⛔ TWO FAILURE CASES, AND BOTH ARE NAMED — because naming ONLY THE FIRST
	 *  is exactly what shipped the defect:
	 *    (1) THE ABP IS MISSING, CLEARED OR UNRESOLVABLE ⇒ no anim instance ⇒ ref
	 *        pose. Harmless, silent, never a crash. ⚠️ This is the case the
	 *        previous version of this comment reasoned about carefully, and its
	 *        reasoning was correct.
	 *    (2) THE ABP RESOLVES AND ASSUMES A PAWN ⇒ ⛔ A GRAPH THAT FAILS EVERY
	 *        FRAME, PER INSTANCE, FOR THE WHOLE MATCH. ⚠️ This is the case the
	 *        previous version was SILENT about, and the silence is what shipped:
	 *        assigning /Game/Characters/ABP_Footman here cost 1,806 Blueprint
	 *        runtime errors in 49 s with two commanders alive — after compiling
	 *        clean, passing 111/111 tests and clearing two review gates.
	 *  ⇒ ⭐ "NULL-SAFE" IS NOT "OWNER-SAFE". A guard against ABSENCE says NOTHING
	 *  about a PRESENT-BUT-WRONG asset, and only the second one is expensive.
	 *
	 *  ⛔ /Game/Characters/ABP_Footman IS NOT A LEGAL VALUE FOR THIS FIELD. It is
	 *  the entire rigged fleet's locomotion asset, and its use by ASummonedUnit is
	 *  CORRECT — that class is an ACharacter, i.e. a Pawn (SC-§35 item 3). ⛔ The
	 *  repair belongs at THIS consumer; the shared ABP is never edited, never
	 *  null-guarded and never forked to accommodate one decorative prop.
	 *
	 *  ✅ KEPT rather than deleted, as the escape hatch for a future,
	 *  purpose-built ABP_Commander. Until one exists the idle comes from
	 *  AvatarIdleAnimAsset, which runs NO GRAPH and therefore cannot reproduce
	 *  this defect class at all. PRECEDENCE (ApplyAvatarAnimation): this field if
	 *  it resolves; else the single-node idle; else ref pose.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc")
	TSoftClassPtr<UAnimInstance> AvatarAnimClassAsset;

	/**
	 *  Soft reference to the avatar's looping IDLE clip:
	 *  /Game/Characters/Anims/A_Sorcerer_Idle. Applied as SINGLE-NODE playback,
	 *  ⛔ never through an AnimBlueprint.
	 *
	 *  ⭐ THE SHAPE IS THE WHOLE POINT (SC-§35 item 2): a single-node anim
	 *  instance HAS NO GRAPH, so it cannot query its owner, so it is
	 *  STRUCTURALLY INCAPABLE of the owner-class defect AvatarAnimClassAsset
	 *  documents above. It is the sanctioned way for a NON-PAWN prop to idle, and
	 *  it is the same API the fleet already uses for its per-unit attack/death
	 *  clips (ASummonedUnit::PlaySkeletalAttackAnim).
	 *
	 *  ⚠️ AN UNVERIFIED SKELETON, DECLARED AS ONE (SC-§20) — this is a HYPOTHESIS,
	 *  ⛔ not a measurement, and the code is built to survive it being wrong. The
	 *  clip is EXPECTED to sit on SK_Sorcerer's SK_Footman_Skeleton (the fleet is
	 *  retargeted onto that single skeleton, and this sequence sits beside
	 *  A_Footman_Idle in the same folder) ⛔ but nobody has opened the asset to
	 *  confirm it. ApplyAvatarAnimation therefore checks the skeleton itself and
	 *  falls through to REF POSE on a mismatch, logging once at Log. Either
	 *  outcome ships: a ref-pose commander is a COSMETIC downgrade, and the
	 *  war-map gate, InteractRadius and EnemyRevealCost are unaffected by the
	 *  pose in either direction.
	 *
	 *  Clearing it in a child/instance is a silent designer opt-out (the
	 *  AttackImpactEffect IsNull pattern) and yields ref pose with no log line.
	 */
	UPROPERTY(EditAnywhere, Category = "Siegebound|CommanderNpc")
	TSoftObjectPtr<UAnimSequence> AvatarIdleAnimAsset;

	/**
	 *  Which team owns this NPC. PUSHED by ACastle via InitCommanderNpc
	 *  (TASK-562) — never derived here. Transient: it is re-pushed on every
	 *  spawn, including the Play Again respawn. Defaults Blue per the TASK-559
	 *  spec, so a hand-placed/debug instance is a Blue commander rather than an
	 *  undefined one.
	 */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Siegebound|CommanderNpc")
	ETeamId CommanderTeam = ETeamId::Blue;

private:

	/**
	 *  Loads AvatarMeshAsset onto AvatarMesh; optionally warns (once) when it does
	 *  not resolve. ⛔ TOUCHES NO ANIMATION — it is shared with OnConstruction, and
	 *  TASK-591 (d) forbids starting playback from the construction script.
	 */
	void ResolveAvatarMesh(bool bWarnIfMissing);

	/**
	 *  The anim precedence ladder: AvatarAnimClassAsset ⇒ single-node
	 *  AvatarIdleAnimAsset ⇒ ref pose. Every rung null-safe; the fall-through logs
	 *  at most one line, at Log.
	 *
	 *  ⛔ CALLED FROM BeginPlay AND NOWHERE ELSE (TASK-591 (d)). That single call
	 *  site is not incidental — it is what gates playback on begun-play and what
	 *  makes the fall-through log structurally once-per-spawn rather than
	 *  once-per-editor-property-tweak. ⛔ Do not call it from OnConstruction.
	 */
	void ApplyAvatarAnimation();

	/** Loads WarTableMeshAsset onto WarTableMesh if resolvable; optionally warns (once) when it is not. */
	void ResolveWarTableMesh(bool bWarnIfMissing);

	/** One-shot guard for the missing-avatar warning. */
	bool bWarnedMissingAvatarMesh = false;

	/** One-shot guard for the missing-war-table warning. */
	bool bWarnedMissingWarTableMesh = false;
};
