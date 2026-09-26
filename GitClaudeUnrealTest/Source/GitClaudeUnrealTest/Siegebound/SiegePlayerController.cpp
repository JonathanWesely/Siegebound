// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DataTable.h"
#include "Engine/DecalActor.h"
#include "Engine/GameInstance.h" // TASK-602: UGameInstance::GetSubsystem — resolve the ACC-§4 account seam at call time
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h" // UGameplayStatics::LoadGameFromSlot — active saved deck load (M6 TASK-114)
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Siegebound/Building.h"
#include "Siegebound/CaptureZone.h" // ACaptureZone — capture-spawn clause (complete type: CanTeamSpawnHere call, TASK-261)
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/CommanderNpc.h" // ACommanderNpc — the war map's proximity gate + the EnemyRevealCost the authority prices the reveal from (TASK-563; the class is TASK-559's). ⛔ READ ONLY: this controller never spawns, mutates or destroys one (TASK-562's ACastle owns that lifecycle).
#include "Siegebound/DeckComponent.h"
#include "Siegebound/DeckLibrary.h" // UDeckLibrary::IsDeckLegal — gate the active saved deck before SetPendingDeckList (M6 TASK-114)
#include "Siegebound/FogVolume.h" // AFogVolume — TASK-989 reads the LIVE prevention remainder at CLICK time (FOG-§10.6), and TASK-991 additionally reads the LIVE would-be window (the DURATION accessor, FOG-§10.7 (A)) to explain the sun-on-sun refusal. ⛔ READ ONLY: the READ door `Find` (⛔ never `FindOrSpawn`) and two `const` accessors; this controller never spawns, mutates or resets one — TASK-982's state machine owns every write, the height formula included.
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeAccountSubsystem.h" // TASK-602: USiegeAccountSubsystem — the ACC-§4 deck-slot seam (the class is TASK-600's, landing in the same batch — the TASK-442 parallel-header precedent)
#include "Siegebound/SiegeAssistantComponent.h" // USiegeAssistantComponent — complete type for the constructor's CreateDefaultSubobject (TASK-440; the class BODY is TASK-442's, so this header does not exist until that task lands — see the handoff's compile-order note)
#include "Siegebound/SiegeAssistantConsoleWidget.h" // USiegeAssistantConsoleWidget — complete type for CreateAndAddToViewport / Open / Close / the OnConsoleOpenChanged binding (TASK-449; the widget itself is TASK-444's)
#include "Siegebound/SiegeCheatManager.h" // TASK-121 — CheatClass complete-type (constructor assignment below)
#include "Siegebound/SiegeCombatStatics.h" // TASK-1132 (WITCH-§8): FSiegeCombatStatics — the ONE shipped veil predicate, consulted by the 30-gold enemy-reveal survey. ⛔ READ ONLY: this controller adds no rule and owns none.
#include "Siegebound/SiegeControlsHelpWidget.h" // USiegeControlsHelpWidget — complete type for CreateAndAddToViewport / OpenHelp / CloseHelp / the OnHelpOpenChanged binding (TASK-706, `HELP-§3`)
#include "Siegebound/SiegeGraphicsMenuWidget.h" // USiegeFrameRateCounterWidget — complete type for CreateAndAddToViewport (TASK-1120, `GFX-§7`); the counter class ships in the graphics lane's own file pair
#include "Siegebound/SiegeDeckSaveGame.h" // USiegeDeckSaveGame — active saved deck source (M6 TASK-114)
#include "Siegebound/SiegeFeedbackLibrary.h" // M7 §6 audio hooks (TASK-179): card play/discard/spell/end-of-match
#include "Siegebound/SiegeGameMode.h" // M8 (TASK-356): RequestPlayAgain resolves the server GameMode (doc §4.2)
#include "Siegebound/SiegeGhostPawn.h" // ASiegeGhostPawn — complete type for the ONE IsA() the death-state gate is built on (TASK-750; the class is TASK-749's, landing in the same batch — the TASK-442 parallel-header precedent)
#include "Siegebound/SiegeMenuInputSubsystem.h" // TASK-1482 [VICTORY-SCREEN-NAVIGABLE]: complete type for RegisterMenuNavTarget / UnregisterMenuNavTarget on the victory-screen open/close edges. ⛔ READ-ONLY DEPENDENCY: this controller CALLS that public API and never edits that file (it is a six-row queue).
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "Siegebound/SiegeSpawnConstants.h"
#include "Siegebound/SpellLibrary.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/WarMapWidget.h" // UWarMapWidget — complete type for CreateAndAddToViewport / OpenMap / CloseMap / ReceiveEnemyReveal and the three delegate binds (TASK-563; the widget itself is TASK-560's)
#include "TimerManager.h" // TASK-344: the 1 s group-order prune timer (SetTimer on the world timer manager)

namespace
{
	//~ §6 UI/stinger audio soft-ref paths (TASK-179) — 2D one-shots, null-safe (sounds arrive in TASK-180).
	const TCHAR* CardPlaySoundPath = TEXT("/Game/Audio/S_CardPlay");
	const TCHAR* CardDiscardSoundPath = TEXT("/Game/Audio/S_CardDiscard");
	const TCHAR* SpellCastSoundPath = TEXT("/Game/Audio/S_SpellCast");
	const TCHAR* VictoryMusicSoundPath = TEXT("/Game/Audio/S_VictoryMusic");
	const TCHAR* DefeatMusicSoundPath = TEXT("/Game/Audio/S_DefeatMusic");

	//~ M8 (TASK-356) — the D5 observer posture + the HUD PS-retry bound.

	/** The APPROVED refusal wording for the P1 client observer posture (doc §4.1, sign-off ruling §9.2 — character-for-character). Function-local static: FText must never construct at module static-init (localization may not be up yet). */
	const FText& GetObserverLockoutText()
	{
		static const FText ObserverLockoutText = NSLOCTEXT("Siegebound", "Refused_OnlineObserver", "Not available yet in online matches");
		return ObserverLockoutText;
	}

	/** Bounded next-tick retries while waiting for the client's PlayerState proxy before creating the HUD (doc §3.2). ~2 s at 60 fps; exhaustion logs and creates the HUD anyway. */
	constexpr int32 MaxHUDInitAttempts = 120;

	//~ TASK-344 group orders — implementation constants (like StructureMoveAcceptanceRadius
	//~ on the unit: impl details, NOT feel tunables; the six feel tunables are UPROPERTYs).

	/** Cadence of the all-dead group reaper (the CONVENTIONS "≤1 s" marker-removal law). */
	constexpr float UnitGroupPruneInterval = 1.f;

	/**
	 *  Golden angle in radians (2π · (1 − 1/φ) ≈ 2.399963): the deterministic
	 *  sunflower spread — station i sits at radius R·√((i+0.5)/N), angle i·this —
	 *  fills the position circle near-uniformly for ANY member count, so a group
	 *  never mills at a single point (the TASK-280/282 lesson's spread half).
	 */
	constexpr float GoldenAngleRadians = 2.399963f;

	//~ TASK-395 FOLLOW command — implementation constants (same class as the two
	//~ above: impl details, NOT feel tunables; the two FOLLOW feel tunables,
	//~ FollowFormationRadius and FollowRepathTolerance, are UPROPERTYs).

	/**
	 *  Nominal slot count the follow sunflower's RADIUS is scaled for. Station
	 *  index i takes radius R·sqrt(((i mod this)+0.5)/this) and angle i·golden:
	 *  the first 12 followers fill the ring near-uniformly exactly like the
	 *  shipped stage-3 spread, and past 12 the radius wraps (staying inside
	 *  FollowFormationRadius) while the golden angle keeps every angle distinct.
	 *  12 ≈ a comfortable escort ring at the 900 uu default; changing it changes
	 *  packing density only, never correctness.
	 */
	constexpr int32 FollowFormationSlots = 12;

	//~ TASK-563 WAR MAP — implementation constants (the MaxHUDInitAttempts class:
	//~ engine/transport bounds, ⛔ NOT feel tunables and ⛔ NOT mechanic rules, so
	//~ neither is a UPROPERTY and neither is a cards.csv column).

	/**
	 *  Viewport Z order for the war map. ⚠️ THE VALUE IS A RELATIONSHIP, NOT A
	 *  PREFERENCE, and it is the reason it is not left at CreateAndAddToViewport's
	 *  default of 0: the map must sit ABOVE the HUD (added at the AddToViewport()
	 *  default, 0) because it is a full-screen panel, and BELOW the assistant console
	 *  (USiegeAssistantConsoleWidget::CreateAndAddToViewport's ZOrder default, 5)
	 *  because a marker click writes into the console's input box and the player must
	 *  be able to SEE and CLICK that box while the map is still up. A map drawn over
	 *  the console would hide the one surface the click exists to fill.
	 */
	constexpr int32 WarMapWidgetZOrder = 4;

	/**
	 *  Viewport Z order for the TAB controls overlay (TASK-706). ⚠️ THE VALUE IS A
	 *  RELATIONSHIP, NOT A PREFERENCE, exactly as above: 6 puts it ABOVE the HUD (0),
	 *  above the war map (4) and above the assistant console (5).
	 *
	 *  ⚖️ ABOVE THE CONSOLE, AND THAT IS THE OPPOSITE CHOICE FROM THE MAP'S — because
	 *  the reason the map sits BELOW it does not apply here. The map is below the
	 *  console because a marker click writes into the console's input box and the
	 *  player must see and click that box. The help overlay delivers nothing into any
	 *  other surface; it is the thing the player just explicitly asked to READ, so it
	 *  is the thing that must be legible. ⛔ And it steals nothing by sitting on top:
	 *  its backdrop is SelfHitTestInvisible, so only its own panel's area is
	 *  hit-testable and every click outside it falls through unchanged.
	 */
	constexpr int32 ControlsHelpWidgetZOrder = 6;

	/**
	 *  Hard bound on the paid reveal's dot payload (WR-§7). A Reliable RPC carrying an
	 *  unbounded TArray is a bandwidth hazard rather than a gameplay one — this is the
	 *  MaxHUDInitAttempts class of constant, not a design number.
	 *
	 *  ⚠️ IT IS DELIBERATELY FAR ABOVE ANY REAL ARMY, so it is a tripwire and never a
	 *  silent truncation of a normal match: exceeding it logs a Warning naming the
	 *  drop. ⛔ The reveal is NOT re-priced or refunded on a truncation — the player
	 *  paid for a survey and got one; a partial refund would be exactly the partial
	 *  spend WR-§7 forbids.
	 */
	constexpr int32 MaxEnemyRevealDots = 512;

	/** Player-facing label for a group-order type — used by the pick prompts and the pick logs. */
	const TCHAR* GroupCommandTypeLabel(ESiegeGroupCommandType Type)
	{
		switch (Type)
		{
		case ESiegeGroupCommandType::Ambush: return TEXT("AMBUSH");
		case ESiegeGroupCommandType::Follow: return TEXT("FOLLOW");
		case ESiegeGroupCommandType::Hold:
		default:                             return TEXT("HOLD");
		}
	}
}

ASiegePlayerController::ASiegePlayerController()
{
	// the per-frame cursor-to-ground trace runs in PlayerTick, which early-outs
	// whenever placement mode is inactive
	PrimaryActorTick.bCanEverTick = true;

	// ⚖️ M8 (TASK-356 loop-1, FINDING-4 hardening + the CONVENTIONS NET
	// RELEVANCY LAW COROLLARY: "assert/verify bReplicates on any class whose
	// authority branch matters"). This controller is dense with authority
	// branches (the four D5 observer lockouts + the RequestPlayAgain routing),
	// so its replication state must be unambiguous rather than inherited.
	//
	// WHAT THIS FIXES (and what it does not): `APlayerController`'s CDO leaves
	// bReplicates FALSE — verified in the installed engine source, its ctor
	// never sets it (the only `bReplicates = true` in PlayerController.cpp is
	// ANoPawnPlayerController's at :6813) — and the SERVER turns it on per
	// instance at login (UWorld::SpawnPlayActor → SetReplicates(true) +
	// SetAutonomousProxy(true), World.cpp:4937-4938). Because bReplicates is not
	// itself a replicated property, a CLIENT's locally-constructed copy keeps the
	// CDO's false while the actor channel writes the real roles — which is
	// EXACTLY the "client PC reads bReplicates=False" reading TASK-357 measured
	// (normal engine behavior in every UE project, not a defect). Setting it here
	// makes the flag consistent on both machines — the same pattern APawn
	// (Pawn.cpp:86) and ANoPawnPlayerController use — so no future authority or
	// RPC reasoning on this class rests on an inherited default.
	//
	// SAFE + INERT: on the server the engine's login-time SetReplicates(true)
	// now early-outs (same value, same RemoteRole) and SetAutonomousProxy is
	// unchanged; in STANDALONE there are no connections, so a replication flag
	// on a controller changes nothing (doc §10 byte-identity holds).
	bReplicates = true;

	// deck & hand model (GDD §3.4, TASK-022) — subobject name is a spec contract
	DeckComponent = CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"));

	// LLM command assistant (TASK-440) — subobject name is a spec contract, same
	// as DeckComponent's. ⛔ THIS TASK CREATES THE SUBOBJECT AND AUTHORS NOTHING
	// INSIDE IT: the FSM skeleton is TASK-442's and the executor is TASK-443's.
	//
	// STRICTLY ADDITIVE (CONVENTIONS "In-match LLM command assistant" §2): owning
	// the component registers no tick here, routes no key through it, and leaves
	// every shipped command path byte-identical. With the console never opened the
	// component is inert, which is exactly the fault posture §2 demands — a missing
	// GGUF or a faulted model must never block match start or degrade a key.
	AssistantComponent = CreateDefaultSubobject<USiegeAssistantComponent>(TEXT("AssistantComponent"));

	// debug-exec cheats for headless verification (TASK-121). The engine only
	// instantiates a UCheatManager in non-shipping builds with cheats enabled, so
	// this can never leak into Shipping — additive, zero behavior change to play.
	CheatClass = USiegeCheatManager::StaticClass();

	// content contract (TASK-007/023 names blocks) — everything soft, resolved
	// null-safe at runtime; the assets are built by parallel tasks
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));                                  // TASK-008
	HUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));                                    // TASK-011
	VictoryScreenClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C")));            // TASK-011
	GhostFallbackMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));                    // engine asset, read-only (TASK-030 ghost fallback)
	GhostMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Ghost.M_Ghost")));                   // TASK-012
	SpellReticleMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_SpellReticle.M_SpellReticle"))); // TASK-108 (M5 reticle decal)
	Card1ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card1.IA_Card1")));                     // TASK-009
	Card2ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card2.IA_Card2")));                     // TASK-032
	Card3ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card3.IA_Card3")));                     // TASK-032
	Card4ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card4.IA_Card4")));                     // TASK-032
	Card5ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card5.IA_Card5")));                     // TASK-032
	Card6ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card6.IA_Card6")));                     // TASK-032
	UICursorActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_UICursor.IA_UICursor")));            // TASK-032
	CancelPlaceActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CancelPlace.IA_CancelPlace")));   // TASK-009
	CmdAttackActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CmdAttack.IA_CmdAttack")));         // TASK-273 (Shield Wall — T)
	CmdHoldActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CmdHold.IA_CmdHold")));               // TASK-273 (Shield Wall — R)
	CmdDefendActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CmdDefend.IA_CmdDefend")));         // TASK-273 (Shield Wall — E)
	CmdAmbushActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CmdAmbush.IA_CmdAmbush")));         // TASK-345 (Group orders — F; inert-null-safe until the asset lands)
	CmdFollowActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow")));         // TASK-399 (FOLLOW — C; inert-null-safe until the asset lands)
	AssistantConsoleActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole"))); // TASK-445 (assistant console open key; inert-null-safe until the asset lands — the IA_CmdAmbush/IA_CmdFollow precedent)
	WarMapActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_WarMap.IA_WarMap")));                             // TASK-568 (war-map toggle — M; inert-null-safe until the asset lands. Dvorak/positional remapping is FREE via IMC_Hero, KBD-§)
	WarMapWidgetClass = TSoftClassPtr<UWarMapWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_WarMap.WBP_WarMap_C")));                                    // TASK-568 (chrome only — the C++ painter draws and hit-tests every marker and dot without it)
	ControlsHelpActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_ControlsHelp.IA_ControlsHelp")));           // TASK-705 (controls overlay toggle — Tab; inert-null-safe until the asset lands. Dvorak/positional remapping is FREE via IMC_Hero, KBD-§)
	DiscardAllActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_DiscardAll.IA_DiscardAll")));                // TASK-820 (discard the WHOLE hand — the `H` POSITION, CARDBAR-§6/§7; inert-null-safe until the asset lands. MAPPED, so the Dvorak retarget is FREE via IMC_Hero — ⛔ never a raw key poll; the trap is spelled out on DiscardAllActionAsset in the header)
	ControlsHelpWidgetClass = TSoftClassPtr<USiegeControlsHelpWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_ControlsHelp.WBP_ControlsHelp_C")));       // `HELP-§3` RESERVED + UNAUTHORED — the code-authored tree renders the whole overlay without it
}

// ---------------------------------------------------------------------------
// TASK-1270 — the match-start "your active deck is illegal" HUD notice.
//
// A free function with EXTERNAL LINKAGE (the SiegeboundCardGlossary::
// AppendSpellLines precedent, TASK-999) rather than a static member: the
// BeginPlay arm that fires it is the only site this row may touch in this
// file, and the header is not on its write list — so the text is composed
// HERE, and Tests/SiegeDeckSlotsTest.cpp forward-declares this signature to
// assert the exact player-facing string (SC-§104: the string, not a tally).
// ⛔ Not in the anonymous namespace: internal linkage would make it
// unreachable from the test, and the gate would come back as a link error.
//
// Key + text are the row's pinned shape. The count is FText::AsNumber so a
// 68 reads "68" (no culture surprise at two digits); the deck name is the
// stored canonical name, verbatim.
// ---------------------------------------------------------------------------
namespace SiegeboundDeckNotice
{
	FText MakeIllegalActiveDeckNoticeText(const FString& DeckName, int32 CardCount)
	{
		return FText::Format(
			NSLOCTEXT("Siegebound", "DeckNotice_IllegalActiveDeck", "Deck '{0}' has {1} cards — playing the default deck"),
			FText::FromString(DeckName), FText::AsNumber(CardCount));
	}
}

void ASiegePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Input-posture normalization FIRST (TASK-074; CONVENTIONS "Input-mode
	// ownership (level-travel law)"): input-routing state set via SetInputMode
	// lives partly on the persistent UGameViewportClient, which SURVIVES
	// UGameplayStatics::OpenLevel* travel. Arriving from L_MainMenu,
	// BP_MenuGameMode's FInputModeUIOnly (SetInputMode_UIOnlyEx, TASK-049) left
	// the viewport with bIgnoreInput=true + EMouseCaptureMode::NoCapture, so a
	// fresh arena controller booted input-dead — UGameViewportClient::InputKey/
	// InputAxis swallowed WASD, the 1–6 hotkeys, and every click (Jonathan's
	// 2026-07-07 bug, both menu buttons). Establish OUR match posture instead of
	// trusting the traveler's: on a fresh controller bInPlacementMode /
	// bInTargetingMode / bUICursorHeld / bMatchEnded are all false, so
	// ApplyCursorInputState() applies exactly FInputModeGameOnly — whose
	// ApplyInputMode clears the viewport's ignore-input latch and restores
	// capture-on-click/lock-on-capture
	// — with the cursor hidden and click events off (the M1/TASK-023 free-look
	// posture). On a direct-PIE L_Arena boot every value written already matches
	// the fresh-viewport/fresh-controller defaults, so this is a no-op there.
	ApplyCursorInputState();

	// Deck build at match start (GDD §3.4). This controller owns the timing —
	// the component never self-builds (TASK-022 flagged decision 12). Built
	// BEFORE any widget below so a hand HUD created at BeginPlay (TASK-033)
	// seeds from an already-dealt hand (CONVENTIONS seed-then-bind law).
	if (DeckComponent)
	{
		// M6 (TASK-114): load the player's ACTIVE saved deck and, when it is legal
		// against DT_Cards, push it as the DeckComponent's pending override BEFORE
		// the build below. Null-safe fallback chain — no save file / empty
		// ActiveDeckName / name-not-found / illegal deck all leave the component
		// unset, so BuildAndShuffle uses the curated DeckCount default exactly as
		// before (backward-compatible). The SaveGame is the menu->match handoff
		// (M6 ruling 1); the active deck is NOT passed through the level-open URL.
		// TASK-602 (ACC-§4 seam law): the slot resolves AT CALL TIME through
		// USiegeAccountSubsystem — profile-scoped when a main-menu login happened
		// (the GameInstance outlives OpenLevel, so it is naturally live here at
		// match start), the bare guest constant otherwise or when the subsystem
		// is unresolvable (fail-safe — today's behavior, never a crash). No
		// cached slot member on purpose (ACC-§4 deck-lane clause).
		const UGameInstance* OwningGameInstance = GetGameInstance();
		const USiegeAccountSubsystem* AccountSubsystem =
			OwningGameInstance ? OwningGameInstance->GetSubsystem<USiegeAccountSubsystem>() : nullptr;
		const FString DeckSlotName =
			AccountSubsystem ? AccountSubsystem->GetDeckSlotName() : USiegeDeckSaveGame::SlotName;
		if (USiegeDeckSaveGame* DeckSave = Cast<USiegeDeckSaveGame>(
				UGameplayStatics::LoadGameFromSlot(DeckSlotName, USiegeDeckSaveGame::UserIndex)))
		{
			const FString& ActiveName = DeckSave->ActiveDeckName;
			if (!ActiveName.IsEmpty())
			{
				const FDeckList* ActiveDeck = DeckSave->SavedDecks.FindByPredicate(
					[&ActiveName](const FDeckList& Candidate) { return Candidate.DeckName == ActiveName; });
				if (ActiveDeck)
				{
					const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
					FString LegalityReason;
					if (UDeckLibrary::IsDeckLegal(CardTable, *ActiveDeck, LegalityReason))
					{
						DeckComponent->SetPendingDeckList(*ActiveDeck);
						UE_LOG(LogGitClaudeUnrealTest, Log,
							TEXT("ASiegePlayerController '%s': active saved deck '%s' is legal (%d cards) — using it this match (M6 TASK-114)."),
							*GetNameSafe(this), *ActiveName, ActiveDeck->TotalCount());
					}
					else
					{
						UE_LOG(LogGitClaudeUnrealTest, Warning,
							TEXT("ASiegePlayerController '%s': active saved deck '%s' is not legal (%s) — falling back to the curated DeckCount default (M6 TASK-114)."),
							*GetNameSafe(this), *ActiveName, LegalityReason.IsEmpty() ? TEXT("no reason") : *LegalityReason);

						// TASK-1270 — SAY SO ON THE HUD (the TASK-1230 R-DECK finding:
						// Jonathan played the default for days with the rim on a 68-card
						// deck1 and only this Warning to show for it). Same M2 refusal
						// surface every other notice uses (BroadcastRefusal →
						// OnCardRefused → UCardHandWidget::OnCardRefusedMessage), no new
						// path. The count is the deck's TotalCount (the row's pinned shape)
						// — the Warning above carries the precise reason when it is not the
						// count.
						//
						// ⚠️ LOOP 1 (qa/TASK-1270-verify.md VERIFY-FAILED): HELD, ⛔ NOT TIMED.
						// Loop 0 broadcast on a next-tick timer; it fired in the load frame's
						// WORLD tick (logged on the same GFrameCounter as LoadMap), but the
						// channel's only listener is WBP_CardHand, which WBP_HUD creates on
						// its FIRST widget Tick — the Slate phase, after the world tick — so
						// the broadcast met an unbound OnCardRefused and the refusal slot
						// stayed empty. Now the notice is queued on this controller and
						// spent at the LATER of (queued here, a hand binds): the Deliver call
						// below spends it only if a listener is already bound (never, in
						// today's order), otherwise UCardHandWidget::InitForController spends
						// it the moment it binds. Cleared on delivery ⇒ once per match start.
						// The client PS-retry edge (qa/TASK-1287-report.md WARN-1) closes by
						// the same mechanism: the notice waits however late the HUD is.
						QueueMatchStartNotice(
							SiegeboundDeckNotice::MakeIllegalActiveDeckNoticeText(ActiveName, ActiveDeck->TotalCount()));
						DeliverPendingMatchStartNotice();
					}
				}
				else
				{
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("ASiegePlayerController '%s': active deck name '%s' not found in SavedDecks — falling back to the curated DeckCount default (M6 TASK-114)."),
						*GetNameSafe(this), *ActiveName);
				}
			}
			// empty ActiveDeckName => no active deck => curated DeckCount fallback (silent — the default state)
		}
		// no save file (first run) => curated DeckCount fallback (silent — the common case)

		DeckComponent->BuildAndShuffle();
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DeckComponent subobject missing (TASK-022/023) — no deck or hand this match."),
			*GetNameSafe(this));
	}

	// HUD (TASK-011) — M8 (TASK-356 doc §3.2): routed through the bounded
	// PS-retry. On a CLIENT the ASiegePlayerState proxy can arrive a few frames
	// after PC BeginPlay, and a HUD constructed before it would seed from
	// nothing; TryInitHUD defers creation until the PS resolves (next-tick
	// retries, capped). Standalone/host: the PS exists on the FIRST check, so the
	// HUD is created synchronously right here — byte-identical order (deck built
	// above, widgets after; doc §10).
	TryInitHUD();

	// ⭐⭐ TASK-1120 (`GFX-§7`): the in-match FPS / frame-time counter, created
	// beside the HUD. ⛔ It is NOT gated on the preference here — the widget reads
	// the preference itself and collapses when it is off, so there is exactly ONE
	// creation site and ONE visibility writer. See TryInitFrameRateCounter.
	TryInitFrameRateCounter();

	// Group-order maintenance (TASK-344): the 1 s reaper removes all-dead groups
	// and their markers (the CONVENTIONS ≤1 s marker-removal law). Armed once for
	// the controller's lifetime — trivially cheap at 1 Hz while no groups exist.
	// M8 note (doc §4.3#8): on a client this prunes an ALWAYS-EMPTY array
	// (BeginGroupPick is D5-locked) — a harmless 1 Hz no-op, argued in §10.
	GetWorldTimerManager().SetTimer(UnitGroupPruneTimerHandle, this, &ASiegePlayerController::PruneUnitGroups,
		UnitGroupPruneInterval, /*bLoop=*/ true);
}

void ASiegePlayerController::TryInitHUD()
{
	// M8 HUD PS-retry (TASK-356 doc §3.2). Only the OWNING machine builds a HUD:
	// widgets are local-player UI (the doc §1 table) — a server-side copy of a
	// REMOTE client's PC must never create one (mirrors the HandleMatchEnd/Reset
	// local guards). Standalone/host: local ⇒ falls through.
	if (!IsLocalController())
	{
		return;
	}

	if (!GetPlayerState<ASiegePlayerState>())
	{
		++HUDInitAttempts;
		if (HUDInitAttempts < MaxHUDInitAttempts)
		{
			// PS proxy not here yet (client join edge) — retry next tick.
			GetWorldTimerManager().SetTimerForNextTick(this, &ASiegePlayerController::TryInitHUD);
			return;
		}

		UE_LOG(LogSiegeNet, Warning,
			TEXT("ASiegePlayerController '%s': PlayerState still unresolved after %d HUD-init retries — creating the HUD anyway (its binds are null-safe; gold/rate will seed on the first delegate)."),
			*GetNameSafe(this), MaxHUDInitAttempts);
	}

	// HUD (TASK-011). Missing widget = log once and keep playing — the IA_Card1
	// key path into EnterPlacementMode works without any UI.
	if (UClass* HUDClass = HUDWidgetClass.LoadSynchronous())
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': failed to create the HUD widget from '%s'."),
				*GetNameSafe(this), *HUDWidgetClass.ToString());
		}
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': HUD widget class '%s' not found (built in TASK-011) — continuing without a HUD."),
			*GetNameSafe(this), *HUDWidgetClass.ToString());
	}
}

void ASiegePlayerController::TryInitFrameRateCounter()
{
	// Local player UI, exactly like the HUD above: a server-side copy of a REMOTE
	// client's PC must never create one. (The counter is a per-machine reading of
	// a per-machine frame rate, so a server-drawn copy would be meaningless as
	// well as invisible.)
	if (!IsLocalController())
	{
		return;
	}

	// ⛔ NO PlayerState RETRY, unlike TryInitHUD. That retry exists because the HUD
	// seeds gold and team from the PS proxy; this widget reads NOTHING from the
	// world, the match or the player — only a preference off the game instance's
	// settings subsystem. Waiting on a PS would be waiting on something it never
	// touches.
	if (FrameRateCounterWidget != nullptr)
	{
		// Already built (a re-entry through a second BeginPlay would otherwise
		// stack two counters on the viewport, each with its own 0.5 s timer).
		return;
	}

	// ⛔ CreateAndAddToViewport takes THREE parameters, none defaulted (`SC-§33`):
	// a null class falls back to USiegeFrameRateCounterWidget itself, which is the
	// shipping state — there is no WidgetBlueprint for the counter and none is
	// reserved (`GFX-§2` is about the MENU; this widget needs no asset at all).
	// ⚠️ A NAMED `UClass*` LOCAL RATHER THAN A BARE `nullptr`, and it is not
	// fussiness: TSubclassOf carries both a non-explicit TSubclassOf(UClass*)
	// constructor and a non-explicit operator UClass*(), which is the same
	// two-equally-good-conversions trap that produced the C2445 documented at
	// USiegeGraphicsMenuWidget::CreateAndAddToViewport. The
	// USiegeControlsHelpWidget call site above spells its class argument out the
	// same way, for the same reason.
	UClass* const NoAuthoredCounterClass = nullptr;

	FrameRateCounterWidget = USiegeFrameRateCounterWidget::CreateAndAddToViewport(
		this,
		TSubclassOf<USiegeFrameRateCounterWidget>(NoAuthoredCounterClass),
		FrameRateCounterZOrder);

	if (FrameRateCounterWidget == nullptr)
	{
		// CreateAndAddToViewport already logged why. ⛔ Never fatal: a match with no
		// frame-rate readout is a match; a match that fails to start because a
		// diagnostic overlay could not be created is not.
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': in-match FPS/frame-time counter created at ZOrder %d (TASK-1120, GFX-§7). It obeys the profile preference bShowFrameRateCounter and is COLLAPSED with no timer when that is off (the default)."),
		*GetNameSafe(this), FrameRateCounterZOrder);
}

void ASiegePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// defensive exit path (QA TASK-003 warning 2): teardown mid-placement still
	// releases the melee suppression and destroys the ghost
	ExitPlacementMode();

	// same law for M5 targeting mode (TASK-100): teardown mid-targeting releases
	// the melee suppression and destroys the reticle decal
	ExitTargetingMode();

	// same law for the group-order pick (TASK-344): teardown mid-pick releases
	// the GroupPickHero melee suppression and destroys the pick circles
	CancelGroupPick();

	// symmetric teardown for the IA_UICursor hold (keeps the ignore-look counter balanced)
	ClearUICursorHold();

	// Same law for the assistant console (TASK-449): teardown mid-console drops
	// OUR posture binding, closes the widget and releases the posture flag through
	// its ONE writer. Unbinding FIRST is deliberate — it makes the release
	// deterministic (one explicit call) instead of depending on a broadcast
	// arriving during teardown, and it cannot leave a dynamic delegate pointing at
	// a controller that is going away.
	if (AssistantConsoleWidget)
	{
		AssistantConsoleWidget->OnConsoleOpenChanged.RemoveDynamic(this, &ASiegePlayerController::HandleAssistantConsoleOpenChanged);
		AssistantConsoleWidget->CloseConsole();
		AssistantConsoleWidget->RemoveFromParent();
		AssistantConsoleWidget = nullptr;
	}
	SetAssistantConsoleOpen(false); // never refused; no-op when it was already closed

	// Same law again for the war map (TASK-563), in the same order and for the same
	// reasons: unbind FIRST so the release is one explicit call rather than a
	// broadcast arriving during teardown, and so no dynamic delegate is left pointing
	// at a controller that is going away. CloseMap() also DISCARDS any paid reveal
	// (WR-§7 — the widget clears unconditionally), which is what makes "no persistence
	// across a match teardown" true without a second clear here.
	if (WarMapWidget)
	{
		WarMapWidget->OnMapOpenChanged.RemoveDynamic(this, &ASiegePlayerController::HandleWarMapOpenChanged);
		WarMapWidget->OnPlacePicked.RemoveDynamic(this, &ASiegePlayerController::HandleWarMapPlacePicked);
		WarMapWidget->OnRevealButtonClicked.RemoveDynamic(this, &ASiegePlayerController::HandleWarMapRevealButtonClicked);
		WarMapWidget->CloseMap();
		WarMapWidget->RemoveFromParent();
		WarMapWidget = nullptr;
	}
	SetWarMapOpen(false); // never refused; no-op when it was already closed

	// Same law a third time for the controls overlay (TASK-706), in the same order and for the
	// same reasons: unbind FIRST so the release is one explicit call rather than a broadcast
	// arriving during teardown, and so no dynamic delegate is left pointing at a controller
	// that is going away. ⛔ CloseHelp() mutates nothing in the world — the overlay is
	// read-only (`HELP-§5`), so this teardown can never cancel an order or refund a card.
	if (ControlsHelpWidget)
	{
		ControlsHelpWidget->OnHelpOpenChanged.RemoveDynamic(this, &ASiegePlayerController::HandleControlsHelpOpenChanged);
		ControlsHelpWidget->CloseHelp();
		ControlsHelpWidget->RemoveFromParent();
		ControlsHelpWidget = nullptr;
	}
	SetControlsHelpOpen(false); // never refused; no-op when it was already closed

	// ⭐ TASK-1120: the in-match counter. ⛔ SIMPLER THAN THE THREE ABOVE ON
	// PURPOSE — it owns no posture, no cursor, no melee suppression and no paid
	// reveal, so there is nothing to release and no delegate of ITS OWN pointing
	// back at this controller. Its own NativeDestruct unbinds it from the settings
	// subsystem and clears its timer, and RemoveFromParent is what causes that to
	// run. ⛔ Nothing here can cancel an order or refund a card.
	if (FrameRateCounterWidget)
	{
		FrameRateCounterWidget->RemoveFromParent();
		FrameRateCounterWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ASiegePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// slots resolve hard reference first (a BP subclass may assign them), then
	// fall back to the exact TASK-009/TASK-032 asset paths; all are optional —
	// every binding below is skipped null-safe until its asset exists
	Card1Action = ResolveInputAction(Card1Action, Card1ActionAsset, TEXT("IA_Card1"), TEXT("TASK-009"));
	Card2Action = ResolveInputAction(Card2Action, Card2ActionAsset, TEXT("IA_Card2"), TEXT("TASK-032"));
	Card3Action = ResolveInputAction(Card3Action, Card3ActionAsset, TEXT("IA_Card3"), TEXT("TASK-032"));
	Card4Action = ResolveInputAction(Card4Action, Card4ActionAsset, TEXT("IA_Card4"), TEXT("TASK-032"));
	Card5Action = ResolveInputAction(Card5Action, Card5ActionAsset, TEXT("IA_Card5"), TEXT("TASK-032"));
	Card6Action = ResolveInputAction(Card6Action, Card6ActionAsset, TEXT("IA_Card6"), TEXT("TASK-032"));
	UICursorAction = ResolveInputAction(UICursorAction, UICursorActionAsset, TEXT("IA_UICursor"), TEXT("TASK-032"));
	CancelPlaceAction = ResolveInputAction(CancelPlaceAction, CancelPlaceActionAsset, TEXT("IA_CancelPlace"), TEXT("TASK-009"));

	// Shield Wall unit-command actions (W1 TASK-274; keys T/R/E via IMC_Hero,
	// TASK-273). Same null-safe soft-resolve as the card actions — the TASK-273
	// assets may not exist yet at compile-review time, so an unresolved action
	// just skips its binding below (logged once by ResolveInputAction).
	CmdAttackAction = ResolveInputAction(CmdAttackAction, CmdAttackActionAsset, TEXT("IA_CmdAttack"), TEXT("TASK-273"));
	CmdHoldAction = ResolveInputAction(CmdHoldAction, CmdHoldActionAsset, TEXT("IA_CmdHold"), TEXT("TASK-273"));
	CmdDefendAction = ResolveInputAction(CmdDefendAction, CmdDefendActionAsset, TEXT("IA_CmdDefend"), TEXT("TASK-273"));
	CmdAmbushAction = ResolveInputAction(CmdAmbushAction, CmdAmbushActionAsset, TEXT("IA_CmdAmbush"), TEXT("TASK-345")); // group orders (TASK-344): F stays INERT until the asset lands
	CmdFollowAction = ResolveInputAction(CmdFollowAction, CmdFollowActionAsset, TEXT("IA_CmdFollow"), TEXT("TASK-399")); // FOLLOW (TASK-395): C stays INERT until TASK-399's asset lands — the DESIGNED compile-time state

	// Assistant console open key (TASK-449). SAME null-safe soft-resolve as every
	// action above and for the same reason: TASK-445 creates IA_AssistantConsole
	// and maps it in IMC_Hero, so until it lands this returns null and the key is
	// simply inert (one log line, no crash). ⛔ THAT IS THE DESIGNED STATE, NOT A
	// DEGRADATION TO FIX HERE — and it is why this task is not blocked on TASK-445.
	AssistantConsoleAction = ResolveInputAction(AssistantConsoleAction, AssistantConsoleActionAsset, TEXT("IA_AssistantConsole"), TEXT("TASK-445"));

	// War-map toggle key (TASK-563). SAME null-safe soft-resolve, SAME reason:
	// TASK-568 creates IA_WarMap and maps it in IMC_Hero to M, so until it lands this
	// returns null and M is simply inert (one log line, no crash). ⛔ THAT IS THE
	// DESIGNED STATE, and it is why this task is not blocked on TASK-568.
	WarMapAction = ResolveInputAction(WarMapAction, WarMapActionAsset, TEXT("IA_WarMap"), TEXT("TASK-568"));

	// Controls-overlay toggle key (TASK-706, `HELP-§4`). SAME null-safe soft-resolve, SAME
	// reason: TASK-705 creates IA_ControlsHelp and maps it in IMC_Hero to Tab, so until it
	// lands this returns null and Tab is simply inert (one log line, no crash). ⛔ THAT IS THE
	// DESIGNED STATE, and it is why this task was not blocked on TASK-705.
	ControlsHelpAction = ResolveInputAction(ControlsHelpAction, ControlsHelpActionAsset, TEXT("IA_ControlsHelp"), TEXT("TASK-705"));

	// Discard-the-whole-hand key (TASK-819, `CARDBAR-§6`). SAME null-safe soft-resolve, SAME
	// reason: TASK-820 creates IA_DiscardAll and appends the ONE IMC_Hero row at the `H`
	// position, so until it lands this returns null and the key is simply inert (one log line,
	// no crash). ⛔ THAT IS THE DESIGNED STATE, and it is why this task was not blocked on it.
	DiscardAllAction = ResolveInputAction(DiscardAllAction, DiscardAllActionAsset, TEXT("IA_DiscardAll"), TEXT("TASK-820"));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// key "1" (via IMC_Hero, TASK-009): hand slot 0, with the M1 Footman
		// fallback while the hand is empty (see OnCard1Pressed)
		if (Card1Action)
		{
			EnhancedInputComponent->BindAction(Card1Action, ETriggerEvent::Started, this, &ASiegePlayerController::OnCard1Pressed);
		}

		// keys "2".."6" -> hand slots 1..5 (M2 input ruling; keys mapped in
		// IMC_Hero by TASK-032). Bound with the hand slot index as payload
		// (engine-supported VarTypes overload of BindAction).
		const UInputAction* CardSlotActions[] = { Card2Action, Card3Action, Card4Action, Card5Action, Card6Action };
		for (int32 ActionIndex = 0; ActionIndex < static_cast<int32>(UE_ARRAY_COUNT(CardSlotActions)); ++ActionIndex)
		{
			if (CardSlotActions[ActionIndex])
			{
				EnhancedInputComponent->BindAction(CardSlotActions[ActionIndex], ETriggerEvent::Started, this, &ASiegePlayerController::OnCardSlotKeyPressed, ActionIndex + 1);
			}
		}

		// hold Left Alt (IA_UICursor, TASK-032) = cursor for HUD clicks (M2
		// input ruling). Completed AND Canceled both release, so the hold can
		// never stick regardless of the action's trigger setup.
		if (UICursorAction)
		{
			EnhancedInputComponent->BindAction(UICursorAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnUICursorPressed);
			EnhancedInputComponent->BindAction(UICursorAction, ETriggerEvent::Completed, this, &ASiegePlayerController::OnUICursorReleased);
			EnhancedInputComponent->BindAction(UICursorAction, ETriggerEvent::Canceled, this, &ASiegePlayerController::OnUICursorReleased);
		}

		// cancel placement (RMB/Esc). Also covered by direct key polling in
		// PlayerTick so a missing asset can never soft-lock placement mode.
		if (CancelPlaceAction)
		{
			EnhancedInputComponent->BindAction(CancelPlaceAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCancelPlacePressed);
		}

		// Shield Wall unit commands (W1 TASK-274): T = Attack, R = HOLD pick,
		// E = Defend (mapped in IMC_Hero by TASK-273). Each binding is skipped
		// null-safe until its IA_Cmd* asset exists — a missing asset just leaves
		// that key inert (never a crash).
		if (CmdAttackAction)
		{
			EnhancedInputComponent->BindAction(CmdAttackAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCmdAttackPressed);
		}
		if (CmdHoldAction)
		{
			EnhancedInputComponent->BindAction(CmdHoldAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCmdHoldPressed);
		}
		if (CmdDefendAction)
		{
			EnhancedInputComponent->BindAction(CmdDefendAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCmdDefendPressed);
		}

		// AMBUSH group order (TASK-344): F opens the same 3-stage pick as R, with
		// the chase-to-the-kill leash. The IA_CmdAmbush asset arrives in TASK-345 —
		// until then the resolve above returned null and F is simply inert.
		if (CmdAmbushAction)
		{
			EnhancedInputComponent->BindAction(CmdAmbushAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCmdAmbushPressed);
		}

		// FOLLOW group order (TASK-395): C opens the ONE-STAGE pick — a single
		// wheel-resizable select circle that confirms in place. The IA_CmdFollow
		// asset arrives in TASK-399; until then the resolve above returned null and
		// C is simply inert (one log line, no crash) — the designed state.
		if (CmdFollowAction)
		{
			EnhancedInputComponent->BindAction(CmdFollowAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnCmdFollowPressed);
		}

		// THE ASSISTANT CONSOLE OPEN KEY (TASK-449) — the ONE binding this task
		// adds. ⛔ IT RE-ROUTES NOTHING: every binding above is untouched, no key
		// is made to pass through the assistant, and with the console closed every
		// shipped command path is byte-identical (CONVENTIONS "In-match LLM command
		// assistant" §2, the strictly-additive law). The IA_AssistantConsole asset
		// arrives in TASK-445; until then the resolve above returned null and the
		// key is inert — the IA_CmdAmbush / IA_CmdFollow precedent exactly.
		if (AssistantConsoleAction)
		{
			EnhancedInputComponent->BindAction(AssistantConsoleAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnAssistantConsolePressed);
		}

		// THE WAR-MAP TOGGLE (TASK-563, WR-§5's input row) — Started, the same
		// trigger event every command key above uses. ⛔ IT RE-ROUTES NOTHING: every
		// binding above is untouched and no key is made to pass through the map. The
		// IA_WarMap asset arrives in TASK-568; until then the resolve above returned
		// null and M is inert — the IA_CmdAmbush / IA_CmdFollow / IA_AssistantConsole
		// precedent exactly.
		//
		// ✅ ⛔ NO GetPositionalKey CALL HERE, DELIBERATELY (KBD-§): this is a MAPPED
		// Enhanced Input action, and USiegeKeyboardLayoutSubsystem already rewrites
		// IMC_Hero's .Key fields wholesale — so Dvorak/positional remapping is
		// inherited for free. That API belongs to RAW polled keys; calling it here
		// would double-apply the remap.
		if (WarMapAction)
		{
			EnhancedInputComponent->BindAction(WarMapAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnWarMapPressed);
		}

		// THE CONTROLS-OVERLAY TOGGLE (TASK-706, `HELP-§4`'s input row) — Started, the same
		// trigger event every command key above uses. ⛔ IT RE-ROUTES NOTHING: every binding
		// above is untouched, no key is made to pass through the overlay, and with the overlay
		// closed every shipped command path is byte-identical. The IA_ControlsHelp asset
		// arrives in TASK-705; until then the resolve above returned null and Tab is inert —
		// the IA_CmdAmbush / IA_CmdFollow / IA_AssistantConsole / IA_WarMap precedent exactly.
		//
		// ✅ ⛔ NO GetPositionalKey CALL HERE, DELIBERATELY (`KBD-§`, `HELP-§1`): this is a
		// MAPPED Enhanced Input action, and USiegeKeyboardLayoutSubsystem already rewrites
		// IMC_Hero's .Key fields wholesale — so Dvorak/positional remapping is inherited for
		// free. That API belongs to RAW polled keys; calling it here would double-apply the
		// remap. ⭐ The overlay's own DISPLAYED label for this key reads the same already-
		// retargeted mapping back through QueryKeysMappedToAction, so the menu documents its
		// own key with zero hardcoded letters and zero second translations.
		if (ControlsHelpAction)
		{
			EnhancedInputComponent->BindAction(ControlsHelpAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnControlsHelpPressed);
		}

		// THE DISCARD-ALL KEY (TASK-819, `CARDBAR-§6`) — Started, the same trigger event every
		// command key above uses. ⛔ IT RE-ROUTES NOTHING: every binding above is untouched and no
		// key is made to pass through it. The IA_DiscardAll asset arrives in TASK-820; until then
		// the resolve above returned null and the key is inert — the IA_CmdAmbush / IA_CmdFollow /
		// IA_AssistantConsole / IA_WarMap / IA_ControlsHelp precedent exactly.
		//
		// ✅ ⛔ NO GetPositionalKey CALL HERE AND ⛔ NO RAW `EKeys::H` POLL ANYWHERE, DELIBERATELY
		// (`CARDBAR-§7`, `KBD-§`) — and for THIS action the distinction is not academic, because
		// unlike every digit above it is a LETTER and letters DO move. USiegeKeyboardLayoutSubsystem
		// already retargets IMC_Hero's .Key fields wholesale, so on Jonathan's US-Dvorak the `H`
		// POSITION resolves to `D` for free (TASK-818 verified `D` is vacated because `D` itself
		// moves to `E`, so nothing collides). A raw poll would instead fire on the physical `J`
		// position for him — a defect ⛔ invisible on every reviewer's QWERTY machine.
		if (DiscardAllAction)
		{
			EnhancedInputComponent->BindAction(DiscardAllAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnDiscardAllPressed);
		}
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': InputComponent is not a UEnhancedInputComponent — card/cancel action bindings skipped (key polling and the HUD button still work)."),
			*GetNameSafe(this));
	}
}

void ASiegePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Group-order 3-stage pick (TASK-344) — the third cursor mode on the SAME
	// input surface as placement/targeting. Mutually exclusive with the other
	// two, so at most one of the three branches runs. Polled RMB/Esc full-flow
	// cancel + POLLED wheel resize (the CONVENTIONS wheel law: the wheel is
	// inert everywhere but inside this branch) + per-frame trace + polled LMB
	// stage confirm — the targeting-branch shape.
	if (GroupPickStage != EGroupPickStage::None)
	{
		// cancel is free and leaves every existing group and stance unchanged
		if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape))
		{
			CancelGroupPick();
			return;
		}

		// wheel resize on the ACTIVE stage circle (polled — NO new InputAction)
		ApplyGroupPickWheel();

		// active circle = TRACE to the surface under the cursor (surface-projection law)
		UpdateGroupPickReticle();

		// confirm: LMB polled while in mode — the physical click also reaches the
		// hero's IA_Attack binding, where SetMeleeSuppressed(true) (via
		// GroupPickHero) makes DoMeleeAttack a cooldown-free no-op (the
		// placement/targeting note).
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			ConfirmGroupPickStage();
		}
		return;
	}

	// M5 TARGETING mode (TASK-100) — the sibling of placement mode below, on the
	// SAME input surface (M5 ruling 8: cursor posture mirrors placement; no new
	// input assets): polled RMB/Esc cancel (works even without the IA_CancelPlace
	// asset — the placement double-cover pattern), per-frame cursor-to-surface
	// trace, polled LMB confirm. The two modes are mutually exclusive (each
	// Enter* ignores while the other is live), so at most one branch runs.
	if (bInTargetingMode)
	{
		// cancel is FREE (ruling 8): no gold has moved before confirm
		if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape))
		{
			ExitTargetingMode();
			return;
		}

		// reticle = TRACE to the surface under the cursor (M4.5 carry-in LAW)
		UpdateSpellReticle();

		// confirm: LMB polled while in mode — same double-duty note as placement:
		// the physical click also reaches the hero's IA_Attack binding, where
		// SetMeleeSuppressed(true) makes DoMeleeAttack a cooldown-free no-op.
		if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			TryConfirmSpellTarget();
		}
		return;
	}

	// WAR MAP (TASK-563) — a POLLED CLOSE ONLY, and it is an ADDITION over the
	// spec's names block that is declared rather than smuggled (SC-§15). The map
	// itself needs no per-frame work: it paints and hit-tests itself, and its ally
	// dots run off a world timer.
	//
	// ⛔ WHY THE POLL EXISTS, AND IT IS A MECHANISM RATHER THAN A TASTE: a marker
	// click OPENS THE ASSISTANT CONSOLE, which takes Slate keyboard focus on its
	// input box. A focused UEditableTextBox consumes character keys, so pressing M
	// again TYPES "m" INTO THE SENTENCE instead of reaching IA_WarMap — and until
	// TASK-568's WBP_WarMap supplies a CloseButton there would be no other way out
	// of a full-screen panel. RMB/Esc polled here is exactly the double-cover the
	// three shipped cursor modes already use ("the player can always leave ... even
	// if the asset is missing"), and SetWarMapOpen(false) is never refused.
	//
	// ⚠️ bWarMapOpen IS FALSE ON EVERY PRE-EXISTING PATH, and the war map is mutually
	// exclusive with all three branches above, so this branch changes nothing about
	// placement, targeting or the group pick.
	if (bWarMapOpen)
	{
		if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape))
		{
			// ⛔ CLOSE THE WIDGET, NOT JUST THE FLAG: CloseMap() is what discards the
			// paid reveal (WR-§7), and its OnMapOpenChanged broadcast is what releases
			// the posture through its ONE writer. CloseWarMap() is the ONE sequence, so
			// no caller can perform half of it.
			CloseWarMap();
		}
		return;
	}

	if (!bInPlacementMode)
	{
		return;
	}

	// Cancel: the IA_CancelPlace binding is the primary path; RMB/Esc are ALSO
	// polled directly so the player can always leave placement mode even if the
	// TASK-009 asset is missing. Double-fire is harmless — ExitPlacementMode is
	// idempotent and un-suppresses melee exactly once.
	if (WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Escape))
	{
		ExitPlacementMode();
		return;
	}

	// ⭐⭐ TASK-815 (STACK-§4) — THE THIRD AND FINAL MARK-§4 WHEEL CONSUMER, and it
	// is the CHEAPEST one possible: the same polled mechanism as the group-pick
	// wheel above, in the same function, as a SIBLING branch that can ⛔ never run
	// in the same frame (the pick branch `return`s at :696, targeting at :724, the
	// war map at :754, and placement is reached only past the `!bInPlacementMode`
	// return above). ⛔ No new InputAction, ⛔ no guard for a state that cannot exist.
	//
	// ⚠️ BEFORE UpdatePlacementGhost, ⛔ NOT AFTER, AND THE ORDER IS LOAD-BEARING:
	// UpdatePlacementGhost reads the ghost's SCALED bounds for the footprint gates
	// (STACK-§6), so a wheel polled after it would validate this frame's click
	// against LAST frame's size — a green that the confirm could refuse, which is
	// the one thing this placement path was built never to do. It is also the exact
	// order the pick branch uses (ApplyGroupPickWheel then UpdateGroupPickReticle).
	ApplyPlacementFootprintWheel();

	// per-frame cursor-to-ground trace + ghost position/color (GDD §3.5)
	UpdatePlacementGhost();

	// Confirm: LMB polled while in mode. Deliberately NOT bound to the pawn's
	// AttackAction (protected on AHeroCharacter) — the same physical click still
	// reaches the hero's IA_Attack binding, where SetMeleeSuppressed(true) makes
	// DoMeleeAttack a cooldown-free no-op (TASK-003 handoff).
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		TryConfirmPlacement();
	}
}

void ASiegePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// hero death must exit placement mode (QA TASK-003 warning 2)
	if (AHeroCharacter* Hero = Cast<AHeroCharacter>(InPawn))
	{
		Hero->OnHeroDied.AddUniqueDynamic(this, &ASiegePlayerController::HandleHeroDied);
	}
}

void ASiegePlayerController::OnUnPossess()
{
	if (AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn()))
	{
		Hero->OnHeroDied.RemoveDynamic(this, &ASiegePlayerController::HandleHeroDied);
	}

	// defensive exit path: losing the pawn mid-placement releases the melee
	// suppression (on the recorded PlacementHero) and destroys the ghost
	ExitPlacementMode();

	// same law for M5 targeting mode (on the recorded TargetingHero)
	ExitTargetingMode();

	// same law for the group-order pick (on the recorded GroupPickHero, TASK-344)
	CancelGroupPick();

	Super::OnUnPossess();
}

void ASiegePlayerController::OnCard1Pressed()
{
	// M2: key "1" plays hand slot 0 when the hand holds a card there. While the
	// hand is empty — the qa/TASK-021-report.md WARN-2 window (DT_Cards ships
	// DeckCount=0 until the TASK-031 reimport) — fall back to the M1
	// always-available Footman placement so the M1 key-1 flow keeps working
	// unchanged (TASK-023 M1-preservation rule; TASK-033 retires the fallback).
	// With a real 6-card hand slot 0 is never empty (§3.4: plays/discards draw
	// replacements immediately), so the fallback cannot shadow a hand card.
	if (DeckComponent && !DeckComponent->GetHandCardID(0).IsNone())
	{
		PlayHandSlot(0);
	}
	else
	{
		EnterPlacementMode(Card1CardID);
	}
}

void ASiegePlayerController::OnCardSlotKeyPressed(int32 Slot)
{
	PlayHandSlot(Slot);
}

void ASiegePlayerController::OnUICursorPressed()
{
	// M2 input ruling: holding IA_UICursor (Left Alt) shows the cursor in
	// GameAndUI so HUD cards/discard buttons are clickable, with camera look
	// suspended (otherwise a click-drag would nudge the camera during capture —
	// qa/TASK-007-report.md nit 1). After match end the input is UI-only and
	// stays that way.
	if (bMatchEnded || bUICursorHeld)
	{
		return;
	}

	bUICursorHeld = true;
	SetIgnoreLookInput(true); // counter-based — paired 1:1 with ClearUICursorHold
	ApplyCursorInputState();
}

void ASiegePlayerController::OnUICursorReleased()
{
	// Completed and Canceled both land here; ClearUICursorHold is guarded so a
	// double release can never unbalance the ignore-look counter. Releasing
	// while placement mode is active leaves the cursor to placement mode
	// (its behavior is unchanged by IA_UICursor — TASK-023 spec).
	ClearUICursorHold();
	ApplyCursorInputState();
}

void ASiegePlayerController::ClearUICursorHold()
{
	if (bUICursorHeld)
	{
		bUICursorHeld = false;
		SetIgnoreLookInput(false);
	}
}

void ASiegePlayerController::PlayHandSlot(int32 Slot)
{
	// M8 D5 observer lockout (TASK-356 doc §4.1): a P1 CLIENT cannot mutate
	// gameplay — a client-side card play would fork local-only state (units the
	// server can't see, gold writes the guards refuse mid-flow). Locking this
	// ENTRY seals every downstream confirm (placement, spells, instants,
	// upgrades, Masons) without touching them. Surfaces the approved refusal
	// text on the existing message path. Standalone/host: authority ⇒ unreached.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this), Slot);
		BroadcastRefusal(GetObserverLockoutText());
		return;
	}

	// mirrors the M1 EnterPlacementMode early-outs: post-match and mid-placement
	// presses are IGNORED quietly (CONVENTIONS: no broadcast for ignored input),
	// not player-facing refusals
	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) ignored — match has ended."),
			*GetNameSafe(this), Slot);
		return;
	}

	// ⛔ THE DEATH-STATE CARD BAN (TASK-750, GHOST-§3 G-3/G-5): while the player is
	// driving the ghost, cards are refused. ⭐ SEALING THIS *ENTRY* SEALS EVERY
	// DOWNSTREAM CONFIRM — placement, spells, instants, hero upgrades, Masons — without
	// touching any of them, exactly as the observer lockout above does.
	// ⚠️ It is a player-facing REFUSAL, not a silent ignore: the player pressed a key
	// and is owed the reason (the shipped refusal doctrine). The message is deliberately
	// the SAME "Hero is down" string EnterPlacementMode already shows for a dead hero —
	// it is the same fact, and the ghost must not make it read differently.
	if (!CanPlayCardsWhilePossessing(GetPawn()))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) refused — the hero is dead and the ghost cannot play cards (GHOST-§ G-5)."),
			*GetNameSafe(this), Slot);
		RefuseCardPlay(DeckComponent ? DeckComponent->GetHandCardID(Slot) : NAME_None,
			NSLOCTEXT("Siegebound", "CardRefused_HeroDead", "Hero is down"));
		return;
	}

	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) ignored — already placing '%s'."),
			*GetNameSafe(this), Slot, *PendingCardID.ToString());
		return;
	}

	// mid-targeting presses are silent ignores too (M5 TASK-100 — the same
	// mode-exclusivity rule as placement above)
	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) ignored — already targeting '%s'."),
			*GetNameSafe(this), Slot, *TargetingCardID.ToString());
		return;
	}

	if (!DeckComponent)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) with no DeckComponent (TASK-022/023)."),
			*GetNameSafe(this), Slot);
		return;
	}

	// empty slot (or out-of-range — GetHandCardID logs and returns NAME_None for
	// those). The normal state for every slot during the WARN-2 empty-deck window.
	const FName CardID = DeckComponent->GetHandCardID(Slot);
	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) refused — no card in that slot (empty until TASK-031 reimports DT_Cards if this is the whole hand)."),
			*GetNameSafe(this), Slot);
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_EmptySlot", "No card in that hand slot"));
		return;
	}

	// stats come from the data table, NEVER from code (GDD §3.0)
	FString RowError;
	const FCardRow* Row = ResolveCardRow(CardID, RowError);
	if (!Row)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': cannot play hand slot %d ('%s') — %s"),
			*GetNameSafe(this), Slot, *CardID.ToString(), *RowError);
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_NoData", "Card data unavailable"));
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': PlayHandSlot(%d) — PlayerState is not an ASiegePlayerState (set on ASiegeGameMode, TASK-006), cannot gate the card cost."),
			*GetNameSafe(this), Slot);
		return;
	}

	// affordability gate first (§3.5 spec order: the gold refusal outranks the
	// type refusal; grey-out is the widget's job). EnterPlacementMode re-checks
	// the same gate — same call stack, so the two can never disagree.
	if (!SiegeState->CanAfford(Row->Cost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — cost %d, gold %d."),
			*GetNameSafe(this), Slot, *CardID.ToString(), Row->Cost, SiegeState->GetGold());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// ⭐⭐ THE `Fog`-DURING-PREVENTION REFUSAL (TASK-989; law FOG-§10.6, FOG-§10.3, ruling J-F19).
	// 📌 Jonathan, verbatim (2026-09-04): "the player should not lose gold, not have the card get
	// casted, and instead get a message telling them bright sun is still up for 'x' amount of
	// seconds, where the 'x' is the ACTUAL amount of time left for the fog prevention."
	//
	// ⛔⛔ THE VALUE IS READ HERE, AT THE CLICK, AND IT IS NEVER CACHED. This is the first refusal
	// in the game whose number CHANGES BETWEEN TWO CLICKS ONE SECOND APART: a remainder captured
	// when `BrightSun` was PLAYED would be stale by exactly the elapsed duration, so the message
	// would count down from the wrong number or never change at all. The accessor recomputes from
	// the world clock on every call, and this is its only call in this file.
	// ⭐ ONE READ, NOT TWO: the refusal is gated on the REMAINDER ITSELF rather than on
	// `IsFogPrevented()`, so the number the player is shown is bit-for-bit the number the refusal
	// was decided on. The accessor returns 0 whenever the machine is not SHIELDED, so `> 0` IS the
	// SHIELDED predicate, and the two can never disagree about which state the machine is in.
	//
	// ⭐ WHY THE ENTRY AND NOT THE RESOLVER (the one judgement call in this diff, declared):
	// refusing HERE moves NO gold and consumes NO card BY CONSTRUCTION rather than by refund —
	// this function neither spends nor confirms; `SpendGold`, `ConfirmInstantDraw` and
	// `ConfirmPlayFromHand` all live BEYOND the routing switch below, which this `return` never
	// reaches. That is his two default properties held without a compensating transaction.
	// ⛔ It is also ROUTING-AGNOSTIC on purpose: the switch below currently sends `Fog` into
	// TARGETING mode (a known defect boarded as TASK-1018, ⛔ NOT touched by this row), and after
	// that row lands it will resolve INSTANTLY instead. This gate fires identically either way.
	// ⛔ TASK-982's guard inside `AFogVolume::RaiseFog()` is NOT made redundant by this one and is
	// deliberately left alone: it is the STATE object's own rule and it still covers the bot,
	// which reaches `USpellLibrary::ResolveSpell` without ever passing through this entry.
	//
	// ⛔ `Fog` ONLY (J-F26, closed): prevention refuses nothing else in the game. The gate is the
	// DATA — the `FogCover` effect, the one thing that raises fog — never a CardID literal.
	// ⛔ The READ door `Find` (never `FindOrSpawn`): a refusal pre-check may not spawn a state
	// actor. No volume in the world means no prevention window, which is the honest answer.
	if (Row->SpellEffect == ESpellEffect::FogCover)
	{
		if (const AFogVolume* const FogState = AFogVolume::Find(GetWorld()))
		{
			const float PreventionSecondsRemaining = FogState->GetFogPreventionSecondsRemaining();
			if (PreventionSecondsRemaining > 0.f)
			{
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — the BrightSun prevention window has %.2f s left (read live at the click, FOG-§10.6); no gold spent, card kept."),
					*GetNameSafe(this), Slot, *CardID.ToString(), PreventionSecondsRemaining);
				// ⛔ The shipped `CardRefused_MaxStacks` idiom, verbatim: `FText::Format` at the
				// call site into the SAME `RefuseCardPlay` surface every other refusal uses
				// (J-F25 — ⛔ no new path, ⛔ no new widget, ⛔ no new toast).
				RefuseCardPlay(CardID, FText::Format(
					NSLOCTEXT("Siegebound", "CardRefused_BrightSunActive", "Bright Sun is still up for {0}"),
					WholeSecondsText(PreventionSecondsRemaining)));
				return;
			}
		}
	}

	// ⭐⭐⭐ THE SUN-ON-SUN CONDITIONAL REFUSAL (TASK-991; law FOG-§10.7 (A), FOG-§10.6, ruling J-F18).
	// 📌 Jonathan, verbatim (2026-09-04): "If bright sun is played during the bright sun window, then
	// the timer gets RESET to whatever the new time would be under the new cast, UNLESS that new time
	// would be LESS than the current time, then the player will just get a message that says 'using
	// bright sun right now would reduce fog prevention time from "x" time to "y" time' … and the
	// player is basically prevented from playing the card."
	//
	// ⛔⛔ IT IS A BRANCH, ⛔ NOT A POLICY — ⛔ not `max`, ⛔ not a refresh, ⛔ not a blanket refuse.
	// ⭐ THE LONGER (and EQUAL) HALF IS NOT HERE AND MUST NOT BE: it belongs to
	// `AFogVolume::ApplyBrightSun`, which RESETS the stored expiry on a normal, paid, consumed cast.
	// This guard owns exactly the SHORTER half, and it owns it by SKIPPING — falling through to the
	// routing switch below is what makes the card still playable. ⛔ An unconditional refusal here
	// would satisfy his sentence word for word while destroying the card, because `BrightSun` could
	// then never be re-cast at all.
	//
	// ⭐⭐⭐ AND THIS IS THE FIRST MESSAGE IN THIS GAME THAT MUST COMPUTE A FULL CARD EFFECT PURELY TO
	// EXPLAIN WHY IT REFUSES TO APPLY IT. His `Y` is "the new fog prevention time UNDER THE CURRENT
	// HEIGHT CALCULATION", so deciding whether to refuse requires working out the window this cast
	// WOULD have opened — and then throwing that result away while reporting it.
	// ⛔⛔ THE FORMULA IS NOT DUPLICATED AND THE CARD IS NOT CAST TO FIND OUT WHETHER TO CAST IT:
	// `GetBrightSunWindowSeconds` is TASK-982's DURATION accessor, shipped public, const and
	// side-effect-free precisely so this refusal can reach it from OUTSIDE the cast path. A second
	// copy of the height formula would drift from the real one, and the message would start lying.
	//
	// ⛔⛔ BOTH VALUES ARE LIVE, READ HERE, AT THE CLICK, AND NEITHER IS CACHED: `X` is the live
	// remainder (FOG-§10.6's law) and `Y` re-samples HERO HEIGHT at this instant — so `Y` changes as
	// he climbs, and two refusals from two perches show two different numbers.
	//
	// ⭐⭐ THE PREDICATE IS BIT-IDENTICAL TO `ApplyBrightSun`'s — same two accessors, same operand
	// order, same STRICT `<` — and that identity is the point rather than a coincidence: this entry
	// gate may never refuse a cast the state object would have accepted, nor wave through one it will
	// then refuse. ⛔ There is deliberately NO extra "only while a window is up" pre-gate: the
	// remainder is already 0 when the machine is not SHIELDED, and a window is always at least the
	// base duration, so the comparison alone is the whole condition. A second, differently-spelled
	// condition here is exactly how the two halves would come to disagree.
	// ⚠️ STRICT `<`, and the boundary is a DECLARED DEFAULT rather than his word: he wrote "LESS
	// than", so EQUAL RESETS (a legal, if pointless, cast). A float-equal window is unreachable in
	// practice, which is why the reading has to be the literal one rather than the convenient one.
	//
	// ⛔ The READ door `Find` (⛔ never `FindOrSpawn`): a refusal pre-check may not spawn a state
	// actor. No volume in the world means no prevention window, which is the honest answer — and the
	// resolver's own `FindOrSpawn` still covers the pre-emptive first cast of a match (J-F17).
	// ⛔ The gate is the DATA — the `FogClear` effect, the one thing that opens the window — never a
	// CardID literal, exactly as its `FogCover` sibling above.
	if (Row->SpellEffect == ESpellEffect::FogClear)
	{
		if (const AFogVolume* const FogState = AFogVolume::Find(GetWorld()))
		{
			// The caster team, derived the way BOTH shipped spell resolvers derive it (the controlled
			// hero, falling back to the local player's Blue) — deliberately the shipped idiom rather
			// than a second convention, so the `Y` shown here is the `Y` the cast would have used.
			const AHeroCharacter* const CasterHero = Cast<AHeroCharacter>(GetPawn());
			const ETeamId CasterTeam = IsValid(CasterHero) ? CasterHero->GetTeamId() : ETeamId::Blue;

			// `X` — the live remainder, and `Y` — the window this cast WOULD open right now. Read in
			// the order the sentence reads them ("from X to Y"); both are live, so the order is a
			// readability choice and nothing else.
			const float RemainingWindowSeconds = FogState->GetFogPreventionSecondsRemaining();
			const float WouldBeWindowSeconds = FogState->GetBrightSunWindowSeconds(CasterTeam);

			if (WouldBeWindowSeconds < RemainingWindowSeconds)
			{
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — a BrightSun cast from this height would SHORTEN the prevention window from %.2f s to %.2f s (both read live at the click, J-F18); no gold spent, card kept, the stored expiry untouched."),
					*GetNameSafe(this), Slot, *CardID.ToString(), RemainingWindowSeconds, WouldBeWindowSeconds);
				// ⛔ The same shipped `RefuseCardPlay` + `FText::Format` idiom its `FogCover` sibling
				// uses (J-F25 — ⛔ no new path, ⛔ no new widget, ⛔ no new toast), and the SAME shared
				// scalar formatter, called TWICE. ⛔ NOT a generalised message builder: the two
				// refusals differ in ARITY, and a builder spanning both would carry an optional second
				// value that is dead half the time (SC-§40 cl. 2).
				RefuseCardPlay(CardID, FText::Format(
					NSLOCTEXT("Siegebound", "CardRefused_BrightSunWouldShorten", "Using Bright Sun right now would reduce fog prevention time from {0} to {1}"),
					WholeSecondsText(RemainingWindowSeconds),
					WholeSecondsText(WouldBeWindowSeconds)));
				return;
			}
		}
	}

	// §6 card-play click (TASK-179): the play is ACCEPTED past every refusal gate above
	// (it now proceeds to placement/targeting/instant resolution) — a 2D click, null-safe.
	USiegeFeedbackLibrary::PlaySound2D(this, CardPlaySoundPath);

	switch (Row->CardType)
	{
	case ECardType::Unit:
	case ECardType::Building:
	case ECardType::Economy:
		// placement types (§3.5). The card leaves the hand ONLY at CONFIRM (M2
		// ruling): record the slot and route into the M1 placement path —
		// placement v2 internals (buildings, miner cap, clearance) are TASK-030's,
		// which replaces the internals behind this same entry.
		PendingHandSlot = Slot;
		EnterPlacementMode(CardID);
		if (!bInPlacementMode)
		{
			// interior refusal (dead hero, miner cap) — already logged/broadcast inside
			PendingHandSlot = INDEX_NONE;
		}
		break;

	case ECardType::Spell:
		// ⭐⭐⭐ M5 spell routing (GDD §3.11, TASK-100), ⛔ REPAIRED BY TASK-1018 — the
		// gate is now DERIVED (`SC-§75`(B)) instead of naming one effect.
		//
		// 📌 Jonathan, verbatim (2026-09-04): *"bright sun and fog seemed to have a
		// placement circle for them, which is totally unnecessary, playing fog or bright
		// sun should be instant and not have any placement circle (like the pickpocket
		// card)."*
		//
		// ⛔⛔ WHAT THIS LINE USED TO BE, AND WHY IT MUST NEVER GO BACK:
		// `if (Row->SpellEffect == ESpellEffect::GoldSteal)`. A BLACKLIST OF ONE. It was
		// correct only until the next global spell and it FAILS OPEN — a new no-reticle
		// effect silently gets a reticle — and it had ALREADY FAILED TWICE by the time he
		// saw it, once for `Fog` and once for `BrightSun`. ⛔ ADDING THE TWO FOG EFFECTS
		// TO IT WOULD HAVE BEEN THE SAME DEFECT WITH A LONGER LIST, and it would
		// GUARANTEE a third occurrence.
		//
		// ⛔⛔ AND IT IS NOT `GetEffectiveDelivery(Row) == GroundCircle` EITHER: that
		// arm returns GroundCircle for GoldSteal, FogCover AND FogClear, so the "obvious"
		// derivation would have printed a reticle for THREE cards — strictly worse than
		// the blacklist, which at least got `Pickpocket` right. `ESpellDelivery` has no
		// value meaning "no aim at all" (see USpellLibrary::SpellRequiresAiming).
		//
		// ⛔ THE PREDICATE IS TASK-999's, CONSUMED, ⛔ NOT RE-DERIVED. The deck builder's
		// glossary asks the SAME question to decide whether to print an aiming sentence.
		// ⚖️ A SECOND DERIVATION WOULD DIVERGE, and the divergence would present as the
		// panel and the game disagreeing — two bugs where there was one.
		//
		// ⇒ NOT aimed ⇒ resolve INSTANTLY on play, no reticle, no cursor (the
		// `Pickpocket` precedent, manager ruling 7; recorded §3.5 deviation, CardType
		// stays Spell) — today `Pickpocket`, `Fog` and `BrightSun`, and tomorrow's global
		// spell with nobody remembering to come here. Aimed ⇒ TARGETING mode, placement
		// mode's sibling: the card leaves the hand only at LMB CONFIRM (M2 law), so
		// cancel costs nothing.
		//
		// ⛔ BOTH REFUSAL GATES ABOVE ARE UNAFFECTED, AND THAT IS BY DESIGN, NOT BY LUCK:
		// the `FogCover` prevention refusal and the `FogClear` sun-on-sun refusal both
		// gate on `Row->SpellEffect` and both `return` BEFORE this switch is reached, so
		// they fire identically on either side of this branch (`qa/TASK-990.md` verified
		// that independence at source). Affordability was pre-checked above; SiegeState
		// is non-null here (checked above).
		if (!USpellLibrary::SpellRequiresAiming(*Row))
		{
			ResolveSpellInstant(Slot, CardID, *Row, *SiegeState);
		}
		else
		{
			// the PendingHandSlot pattern: record the slot BEFORE entry, roll it
			// back if the entry refused internally (dead hero — already
			// logged/broadcast inside EnterTargetingMode)
			TargetingHandSlot = Slot;
			EnterTargetingMode(CardID);
			if (!bInTargetingMode)
			{
				TargetingHandSlot = INDEX_NONE;
			}
		}
		break;

	case ECardType::HeroUpgrade:
	case ECardType::Utility:
		// Instant-resolving types (GDD §3.10/§4, TASK-059): resolve IMMEDIATELY with
		// NO placement step — the effect lands, Cost is spent, and a replacement is
		// drawn on success; every refusal moves NO gold (§3.0). Affordability was
		// pre-checked above; SiegeState is non-null here (checked above).
		ResolveInstantPlay(Slot, CardID, *Row, *SiegeState);
		break;

	default:
		// Unreachable — every ECardType is handled above (defensive refusal for a
		// future/corrupt CardType, no gold moved).
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — unhandled CardType %d."),
			*GetNameSafe(this), Slot, *CardID.ToString(), static_cast<int32>(Row->CardType));
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_Unsupported", "Card not available"));
		break;
	}
}

void ASiegePlayerController::DiscardHandSlot(int32 Slot)
{
	// ⚠️ RETIRED, ⛔ NOT DELETED (TASK-819, `CARDBAR-§6`): TASK-809 removes the six per-card
	// discard buttons that were this function's only shipped UI route, so no player gesture
	// reaches it as of this batch — DiscardEntireHand below replaces it. Kept because deleting a
	// BlueprintCallable a WBP may still reference is this project's silent-runtime-break class.

	// M8 D5 observer lockout (TASK-356 doc §4.1) — the second card ENTRY; same
	// rationale + approved refusal text as PlayHandSlot.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this), Slot);
		BroadcastRefusal(GetObserverLockoutText());
		return;
	}

	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) ignored — match has ended."),
			*GetNameSafe(this), Slot);
		return;
	}

	// discarding while placing is refused: discarding the slot being placed
	// would hand PendingHandSlot a DIFFERENT card at confirm (the §3.4 redraw
	// refills the slot immediately). Cancel placement first, then discard.
	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) refused — placement mode active for '%s'."),
			*GetNameSafe(this), Slot, *PendingCardID.ToString());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "DiscardRefused_Placing", "Cannot discard while placing a card"));
		return;
	}

	// same desync rule for M5 targeting mode (TASK-100): discarding the slot
	// being targeted would hand TargetingHandSlot a DIFFERENT card at confirm
	// (the §3.4 redraw refills the slot immediately). Cancel first, then discard.
	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) refused — targeting mode active for '%s'."),
			*GetNameSafe(this), Slot, *TargetingCardID.ToString());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "DiscardRefused_Targeting", "Cannot discard while targeting a spell"));
		return;
	}

	if (!DeckComponent)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) with no DeckComponent (TASK-022/023)."),
			*GetNameSafe(this), Slot);
		return;
	}

	// qa/TASK-022-report.md WARN-1 guard (BINDING): refuse the empty/out-of-range
	// slot BEFORE any gold moves. The spec's literal spend-then-discard order
	// leaks 1 gold on a NAME_None slot — guaranteed reachable while every slot
	// is empty in the WARN-2 window. ASiegePlayerState has no refund API (and
	// this task may not add one), so the pre-check is the correct closure.
	const FName CardID = DeckComponent->GetHandCardID(Slot);
	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) refused — no card in that slot (no gold spent)."),
			*GetNameSafe(this), Slot);
		BroadcastRefusal(NSLOCTEXT("Siegebound", "CardRefused_EmptySlot", "No card in that hand slot"));
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) — PlayerState is not an ASiegePlayerState (TASK-006), cannot charge the discard fee."),
			*GetNameSafe(this), Slot);
		return;
	}

	// the fixed §3.6 charge: SpendGold refuses (no change, no broadcast) below
	// the fee — "discard at 0 gold is refused" acceptance
	if (!SiegeState->SpendGold(DiscardCost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardHandSlot(%d) ('%s') refused — discard costs %d, gold %d."),
			*GetNameSafe(this), Slot, *CardID.ToString(), DiscardCost, SiegeState->GetGold());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// gold is spent — the pile movement can only refuse if the slot emptied
	// between the pre-check and here, impossible within one call stack. Kept as
	// a loud regression tripwire because a false return here means leaked gold.
	if (!DeckComponent->DiscardFromHand(Slot))
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DiscardFromHand(%d) refused AFTER SpendGold(%d) — %d gold leaked; the empty-slot pre-check should make this unreachable (qa/TASK-022-report.md WARN-1)."),
			*GetNameSafe(this), Slot, DiscardCost, DiscardCost);
		return;
	}

	// §6 card-discard click (TASK-179): the discard succeeded (gold spent, pile moved) — a 2D click, null-safe.
	USiegeFeedbackLibrary::PlaySound2D(this, CardDiscardSoundPath);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': discarded hand slot %d ('%s') for %d gold — replacement drawn (GDD §3.6)."),
		*GetNameSafe(this), Slot, *CardID.ToString(), DiscardCost);
}

void ASiegePlayerController::DiscardEntireHand()
{
	// ⭐⭐ THE ONE ENTRY POINT (`CARDBAR-§6`). OnDiscardAllPressed is its ONLY caller from the
	// player; there is no pointer route by Jonathan's own 2026-09-03 ruling. Every guard below is
	// DiscardHandSlot's, in DiscardHandSlot's order, with DiscardHandSlot's approved strings —
	// ⛔ reused rather than reworded, because a second wording of a shipped refusal is a UI
	// regression wearing a feature's clothes.

	// M8 D5 observer lockout (TASK-356 doc §4.1) — the THIRD card ENTRY; same rationale +
	// approved refusal text as PlayHandSlot / DiscardHandSlot.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this));
		BroadcastRefusal(GetObserverLockoutText());
		return;
	}

	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() ignored — match has ended."),
			*GetNameSafe(this));
		return;
	}

	// same desync rule as the per-slot discard: binning the slot being placed would hand
	// PendingHandSlot a DIFFERENT card at confirm (the §3.4 redraw refills the slot in the same
	// call). Cancel placement first, then discard.
	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() refused — placement mode active for '%s'."),
			*GetNameSafe(this), *PendingCardID.ToString());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "DiscardRefused_Placing", "Cannot discard while placing a card"));
		return;
	}

	// and the same for M5 targeting mode (TASK-100) — TargetingHandSlot would confirm a
	// different card than the one the reticle was opened for.
	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() refused — targeting mode active for '%s'."),
			*GetNameSafe(this), *TargetingCardID.ToString());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "DiscardRefused_Targeting", "Cannot discard while targeting a spell"));
		return;
	}

	if (!DeckComponent)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() with no DeckComponent (TASK-022/023)."),
			*GetNameSafe(this));
		return;
	}

	// ⛔⛔ THE EMPTY-HAND REFUSAL COMES **BEFORE** THE SPEND — qa/TASK-022-report.md WARN-1
	// generalised from one slot to the whole hand. Charging 20 gold to bin nothing is a pure
	// loss, ASiegePlayerState has NO refund API, and the empty hand is genuinely reachable (the
	// qa/TASK-021-report.md WARN-2 window, and a degenerate/exhausted deck). ⭐ The occupied
	// slots are collected ONCE here and reused as the loop's domain below, so the hand is read
	// on one side of the charge only — a second scan after the spend could disagree with the one
	// the refusal was decided on.
	const int32 NumHandSlots = DeckComponent->GetHandSize();
	TArray<int32> OccupiedSlots;
	OccupiedSlots.Reserve(NumHandSlots);
	for (int32 Slot = 0; Slot < NumHandSlots; ++Slot)
	{
		if (!DeckComponent->GetHandCardID(Slot).IsNone())
		{
			OccupiedSlots.Add(Slot);
		}
	}

	if (OccupiedSlots.Num() == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() refused — the hand is empty (no gold spent)."),
			*GetNameSafe(this));
		BroadcastRefusal(NSLOCTEXT("Siegebound", "DiscardAllRefused_EmptyHand", "No cards to discard"));
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() — PlayerState is not an ASiegePlayerState (TASK-006), cannot charge the discard-all fee."),
			*GetNameSafe(this));
		return;
	}

	// ⛔⛔ THE CHARGE IS **ONE** CALL FOR THE WHOLE HAND (`CARDBAR-§6`) — DiscardAllCost, flat,
	// whether the hand holds one card or six. SpendGold refuses below the fee with NO change and
	// NO broadcast, so a refusal here is net-zero.
	if (!SiegeState->SpendGold(DiscardAllCost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': DiscardEntireHand() refused — discarding the hand costs %d, gold %d (%d card(s) held)."),
			*GetNameSafe(this), DiscardAllCost, SiegeState->GetGold(), OccupiedSlots.Num());
		BroadcastRefusal(NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// ⛔⛔ THE LOOP CALLS DeckComponent->DiscardFromHand, ⛔ **NEVER** DiscardHandSlot. Looping the
	// per-slot entry point would re-run the entire ladder above six times AND charge six extra
	// DiscardCost on top of the flat fee already taken — a bug that leaves the hand looking
	// exactly right and the player 6 gold poorer. `CARDBAR-§6` calls it an automatic QA fail.
	// ⭐ Each DiscardFromHand moves the card to the discard pile and REDRAWS that slot in the
	// same call (§3.4), so the replacement hand is already dealt when this loop ends — ⛔ no new
	// DeckComponent API, and ⛔ no batch-broadcast optimisation: six broadcasts are the shipped
	// cost of six mutations and the bar re-reads every slot anyway.
	int32 DiscardedCount = 0;
	for (const int32 Slot : OccupiedSlots)
	{
		if (!DeckComponent->DiscardFromHand(Slot))
		{
			// gold is already spent — a false here means leaked gold, so it is LOUD rather than a
			// silent return (the shipped DiscardHandSlot tripwire, kept). The slot was non-empty
			// when it was collected above and nothing between then and here can empty it, so this
			// is unreachable by construction; the remaining slots are still binned because the
			// player paid for the whole hand.
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': DiscardFromHand(%d) refused AFTER SpendGold(%d) — the flat discard-all fee is already charged; the occupied-slot pre-scan should make this unreachable (qa/TASK-022-report.md WARN-1)."),
				*GetNameSafe(this), Slot, DiscardAllCost);
			continue;
		}
		++DiscardedCount;
	}

	if (DiscardedCount > 0)
	{
		// §6 card-discard click (TASK-179): ⛔ ONE sound for the gesture, ⛔ not one per card — a
		// 2D click, null-safe. Gated on a real mutation so the tripwire path above can never play
		// a success cue over nothing having moved.
		USiegeFeedbackLibrary::PlaySound2D(this, CardDiscardSoundPath);
	}

	// ⛔ ONE summary log for the whole gesture, ⛔ not one per card.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': discarded the whole hand — %d of %d occupied slot(s) binned for %d gold (flat), replacements drawn (GDD §3.6, CARDBAR-§6)."),
		*GetNameSafe(this), DiscardedCount, OccupiedSlots.Num(), DiscardAllCost);
}

void ASiegePlayerController::OnDiscardAllPressed()
{
	// ⛔ ZERO logic here by design (`CARDBAR-§6`): DiscardEntireHand owns the whole guard ladder,
	// the single charge and the loop, and this handler adds not one line to it. ⭐ It is also the
	// ONLY route into that function from the player — the right-click alternative was scrapped by
	// Jonathan on 2026-09-03 because right-click is already the placement-cancel gesture and a
	// 20-gold accident on a cancel is exactly the collision worth avoiding.
	DiscardEntireHand();
}

void ASiegePlayerController::OnCancelPlacePressed()
{
	if (bInPlacementMode)
	{
		ExitPlacementMode();
		return;
	}

	// M5 (TASK-100): the SAME cancel action leaves targeting mode at no cost
	// (ruling 8 — no new input assets; the modes are mutually exclusive, so the
	// early return above is ordering hygiene, not a behavior choice)
	if (bInTargetingMode)
	{
		ExitTargetingMode();
		return;
	}

	// TASK-344: the SAME cancel action aborts a group-order pick at any stage at
	// no cost, leaving every existing group and stance unchanged (the polled
	// RMB/Esc in PlayerTick double-covers this so a missing IA_CancelPlace can't
	// soft-lock it)
	if (GroupPickStage != EGroupPickStage::None)
	{
		CancelGroupPick();
	}
}

void ASiegePlayerController::SetUnitCommand(ESiegeUnitCommand NewCommand)
{
	// M8 D5 observer lockout (TASK-356 doc §4.1): the stance is controller-local
	// state SERVER units poll — a client-side latch would be invisible to the sim
	// (dishonest UI). Log-only (the stance/group entries have no card refusal
	// surface); ServerSetUnitCommand is the named P2 RPC (doc §4.3#9).
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': SetUnitCommand refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this));
		return;
	}

	// Latch the stance (Shield Wall, W1 TASK-274): units read CurrentCommand +
	// HasIssuedCommand() live each tick (TASK-275). bHasIssuedCommand flips true
	// on the FIRST command and stays true for the match (until Play Again), so
	// once the player commands, the legacy-body fallback never returns mid-match.
	// Always broadcasts — re-issuing the same stance re-affirms the HUD (TASK-276).
	CurrentCommand = NewCommand;
	bHasIssuedCommand = true;

	OnUnitCommandChanged.Broadcast(NewCommand);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': unit command set to %s (Shield Wall, TASK-274)."),
		*GetNameSafe(this),
		NewCommand == ESiegeUnitCommand::Attack ? TEXT("Attack") : (NewCommand == ESiegeUnitCommand::Hold ? TEXT("Hold") : TEXT("Defend")));
}

void ASiegePlayerController::ApplyArmyWideStance(ESiegeUnitCommand NewCommand)
{
	// TASK-454: the ONE implementation of the T/E release-then-latch sequence.
	// The four statements below were LIFTED VERBATIM out of OnCmdAttackPressed and
	// OnCmdDefendPressed, which were byte-identical mirrors of each other apart
	// from the stance argument. Nothing about the shipped key behaviour changed
	// here — same guard, same two calls, same order, same final latch.
	//
	// ⚠️ THE RELEASE RUNS BEFORE THE LATCH, AND THAT IS LOAD-BEARING.
	// ClearAllUnitGroups documents itself as "Called by T/E (before
	// SetUnitCommand)", and units re-read CurrentCommand on their next 0.25 s
	// state tick — releasing AFTER the latch would let a group that is about to be
	// destroyed re-assert its station for one tick.
	//
	// Every route that means "the whole army now does X" comes through here: the T
	// and E keys, and the in-match assistant's `charge`/`fallback` (CONVENTIONS §2
	// — the assistant calls the same public APIs the keys call, never a parallel
	// implementation and never a "better" one). SetUnitCommand ALONE is the bare
	// latch and leaves standing Hold/Ambush/Follow orders pinned to their zones;
	// that divergence — a global "charge" with squads still frozen on their ground
	// — is exactly what this entry point exists to close (TASK-443 found it and
	// correctly declined to re-implement the private primitives to fix it).
	if (bMatchEnded)
	{
		return;
	}
	CancelGroupPick();
	ClearAllUnitGroups();
	SetUnitCommand(NewCommand);
}

void ASiegePlayerController::OnCmdAttackPressed()
{
	// ATTACK (T) is immediate — no ground pick. Ignored after match end (the
	// input-ignore pattern). Abandon any mid-flight group pick first (no group
	// forms from the abort — CancelGroupPick is no-op-safe), then apply the
	// TASK-344 RELEASE law — a global stance replaces EVERY group order — and
	// only then latch Attack.
	// TASK-454: that entire sequence now lives in ApplyArmyWideStance — which the
	// assistant's `charge` reaches too — so this handler keeps no copy of it.
	ApplyArmyWideStance(ESiegeUnitCommand::Attack);
}

void ASiegePlayerController::OnCmdHoldPressed()
{
	// HOLD (R) enters the 3-stage group pick (TASK-344) — BeginGroupPick owns all
	// the guards (match-ended, and mutual exclusion with placement/targeting).
	BeginGroupPick(ESiegeGroupCommandType::Hold);
}

void ASiegePlayerController::OnCmdAmbushPressed()
{
	// AMBUSH (F, TASK-344) shares the 3-stage pick; only the leash differs
	// (chase-to-the-kill). Reached only once TASK-345's IA_CmdAmbush exists.
	BeginGroupPick(ESiegeGroupCommandType::Ambush);
}

void ASiegePlayerController::OnCmdFollowPressed()
{
	// FOLLOW (C, TASK-395) opens the ONE-STAGE pick: ONE wheel-resizable SELECT
	// circle that CONFIRMS IN PLACE. Deliberately NOT the 3-stage flow with two
	// stages disabled — ConfirmGroupPickStage's Select case terminates for Follow
	// (ConfirmFollowPick), so Position/AttackZone are structurally unreachable.
	//
	// UNLIKE T/E this does NOT release the existing groups: Follow ADDS the
	// circled units to the ONE default follow group, stealing them out of any
	// Hold/Ambush group per the shipped re-selection law (CONVENTIONS §2). Units
	// the player did not circle keep their current orders.
	//
	// BeginGroupPick owns every guard (match-ended, placement/targeting mutual
	// exclusion, already-picking, and the M8 D5 client observer lockout).
	BeginGroupPick(ESiegeGroupCommandType::Follow);
}

void ASiegePlayerController::OnCmdDefendPressed()
{
	// DEFEND (E) is immediate — mirror of OnCmdAttackPressed (pick abort +
	// group release, then the stance latch).
	// TASK-454: both immediate-stance keys now call the ONE shared entry point,
	// so "mirror of OnCmdAttackPressed" is now literally true rather than a
	// second copy that has to be kept in step by hand.
	ApplyArmyWideStance(ESiegeUnitCommand::Defend);
}

void ASiegePlayerController::HandleHeroDied(AHeroCharacter* DeadHero)
{
	// exit path required by qa/TASK-003-report.md warning 2: ResetHero preserves
	// bMeleeSuppressed, so death while placing must release it here
	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hero died during placement mode — cancelling placement."),
			*GetNameSafe(this));
	}
	ExitPlacementMode();

	// same law for M5 targeting mode (TASK-100): death while targeting cancels
	// the spell free (no gold has moved before confirm) and releases the
	// suppression on the recorded TargetingHero
	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hero died during targeting mode — cancelling spell targeting."),
			*GetNameSafe(this));
	}
	ExitTargetingMode();

	// same law for the group-order pick (TASK-344): death mid-pick cancels it
	// free (no group forms, stance unchanged) and releases the GroupPickHero
	// suppression
	if (GroupPickStage != EGroupPickStage::None)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hero died during a group-order pick — cancelling the pick (groups/stance unchanged)."),
			*GetNameSafe(this));
	}
	CancelGroupPick();
}

// ─────────────────────────────────────────────────────────────────────────────
// THE DEATH GHOST (TASK-750, GHOST-§3/§4)
//
// ⚠️ ORDERING, MEASURED AT SOURCE RATHER THAN ASSUMED — it is what makes the
// hand-off safe: AGameModeBase::FinishRestartPlayer calls Possess() (which runs
// THIS class's OnPossess, binding HandleHeroDied) BEFORE SetPlayerDefaults()
// (which is where ASiegeGameMode binds its own HandleHeroDied). Delegates fire in
// binding order ⇒ on death the CONTROLLER's handler above runs FIRST and has
// already cancelled placement / targeting / the group pick — each through its own
// ApplyCursorInputState() call — by the time the game mode spawns the ghost.
// ⇒ ⭐ the ghost never possesses under a live cursor mode. OnUnPossess then runs
// the same three exits again as no-ops when the possession actually swaps.
// ─────────────────────────────────────────────────────────────────────────────

bool ASiegePlayerController::IsGhostPossessed() const
{
	// Live, from the pawn's class. ⛔ Never a latch — see the header.
	const APawn* const CurrentPawn = GetPawn();
	return IsValid(CurrentPawn) && CurrentPawn->IsA(ASiegeGhostPawn::StaticClass());
}

bool ASiegePlayerController::CanPlayCardsWhilePossessing(const APawn* PossessedPawn)
{
	// ⛔ THE GHOST MAY NOT PLAY CARDS (G-5's proceeding default — Jonathan's
	// enumeration is closed and card play is not in it). ⚖️ FLAGGED to him: this
	// single `return false` is the whole ban, and flipping it to `true` is the whole
	// change if he rules the other way.
	//
	// ⭐ KEYED ON THE POSSESSED CLASS, NOT ON A DEATH FLAG. A flag can be forgotten at
	// a guard point; "what am I driving?" cannot. It is also why this function needs
	// no world, no controller and no member state, and can therefore be asserted
	// headlessly across its whole truth table.
	if (IsValid(PossessedPawn) && PossessedPawn->IsA(ASiegeGhostPawn::StaticClass()))
	{
		return false;
	}

	// ⛔ EVERY OTHER PAWN — INCLUDING NO PAWN AT ALL — KEEPS THE SHIPPED BEHAVIOUR
	// BYTE-FOR-BYTE. A dead HERO is still refused downstream by EnterPlacementMode's
	// own `Hero->IsDead()` clause, and re-deriving that rule here would be a second
	// source of truth for something that already has one.
	return true;
}

void ASiegePlayerController::HandleGhostPossessionChanged()
{
	// ⛔ END ANY IA_UICursor HOLD FIRST — the shipped HandleMatchEnd idiom, used here
	// for the identical reason it was written for there: a possession swap tears down
	// and rebuilds the pawn's input plumbing, and a RELEASE swallowed across that swap
	// would leave bUICursorHeld latched AND the counter-based SetIgnoreLookInput
	// unbalanced — a ghost that cannot look around for the whole 180 seconds.
	//
	// ⚠️ MEASURED, AND THE MEASUREMENT IS WHY THIS IS BELT RATHER THAN BRACES: the
	// hold's binding lives on THIS controller's own input component
	// (SetupInputComponent), and the KEY that reaches it is mapped by IMC_Hero, which
	// AHeroCharacter::NotifyControllerChanged only ever ADDS and never removes (there
	// is not one RemoveMappingContext in this module). ⇒ the release very probably
	// still arrives. ⛔ "Very probably" is exactly what GHOST-§4 refuses to stake a
	// three-minute input state on, and the cost of being wrong the other way is one
	// re-press of Left Alt. ClearUICursorHold is itself guarded, so this cannot
	// unbalance the counter.
	ClearUICursorHold();

	// ⛔⛔ THE ONE CURSOR/POSTURE OWNER, AND THE ONLY POSTURE CALL THIS ENTIRE FEATURE
	// MAKES (HELP-§5 / GHOST-§4). Re-asserts the COMPOSED posture against the new pawn.
	// ⛔ The ghost contributes NO term to that composition and the shipped owner ladder
	// is not re-ordered: the ghost is a free-look pawn with the hero's own movement and
	// vision (G-1), so with no owner live this re-applies FInputModeGameOnly — exactly
	// what BeginPlay applies, which is the normalization TASK-074's level-travel law
	// asks for after any state churn.
	// ⛔ At match end this is a deliberate no-op: ApplyCursorInputState early-outs while
	// bMatchEnded is latched, because HandleMatchEnd owns the UI-only end-screen
	// posture and nothing may override it.
	ApplyCursorInputState();
}

void ASiegePlayerController::EnterPlacementMode(FName CardID)
{
	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — match has ended."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — already placing '%s'."),
			*GetNameSafe(this), *CardID.ToString(), *PendingCardID.ToString());
		return;
	}

	// sibling-mode mutual exclusion (M5 TASK-100): a live targeting mode owns
	// the cursor/LMB — mirror of the already-placing silent ignore above
	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — already targeting '%s'."),
			*GetNameSafe(this), *CardID.ToString(), *TargetingCardID.ToString());
		return;
	}

	// third-mode mutual exclusion (TASK-344): a live group-order pick owns the
	// cursor/LMB — the same mutual-ignore the two modes above use
	if (GroupPickStage != EGroupPickStage::None)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — a group-order pick is active."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	// FOURTH-mode mutual exclusion (TASK-440): the assistant console owns the
	// cursor AND the keyboard while open — the mirror of CanOpenAssistantConsole's
	// placement clause, appended AFTER the shipped three so their precedence and
	// their log lines are untouched.
	if (bAssistantConsoleOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — the assistant console is open."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	// FIFTH-mode mutual exclusion (TASK-563): the war map owns the cursor AND the
	// LMB while open (its markers are hit-tested on mouse-down) — the mirror of
	// CanOpenWarMap's placement clause, appended AFTER the shipped four so their
	// precedence and their log lines are untouched.
	if (bWarMapOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode('%s') ignored — the war map is open."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': EnterPlacementMode called with no CardID."),
			*GetNameSafe(this));
		return;
	}

	// cost comes from the data table, NEVER from code (GDD §3.0)
	FString RowError;
	const FCardRow* Row = ResolveCardRow(CardID, RowError);
	if (!Row)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': cannot play card '%s' — %s"),
			*GetNameSafe(this), *CardID.ToString(), *RowError);
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_NoData", "Card data unavailable"));
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': PlayerState is not an ASiegePlayerState (set on ASiegeGameMode, TASK-006) — cannot gate the card cost."),
			*GetNameSafe(this));
		return;
	}

	// affordability gate (GDD §3.5 acceptance: the cost-3 card is refused at 2 gold)
	if (!SiegeState->CanAfford(Row->Cost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': card '%s' refused — cost %d, gold %d."),
			*GetNameSafe(this), *CardID.ToString(), Row->Cost, SiegeState->GetGold());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
	if (Hero && Hero->IsDead())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': card '%s' refused — hero is dead (respawn pending, TASK-006)."),
			*GetNameSafe(this), *CardID.ToString());
		// broadcast like every other player-facing refusal (qa/TASK-007-report.md nit 2)
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_HeroDead", "Hero is down"));
		return;
	}

	// ⛔⛔ THE SAME REFUSAL, KEPT ALIVE ACROSS THE POSSESSION CHANGE (TASK-750,
	// GHOST-§3 G-5). ⚠️ THIS IS A REGRESSION GUARD, NOT A NEW RULE: the clause directly
	// above resolves the hero with `Cast<AHeroCharacter>(GetPawn())`, and the instant
	// the ghost is possessed that cast returns NULL — so "Hero is down" would silently
	// STOP FIRING and a dead player could place units for three minutes. Gating on the
	// possessed CLASS restores exactly the shipped intent.
	// ⚠️ THIS ENTRY IS REACHED INDEPENDENTLY OF PlayHandSlot: OnCard1Pressed falls back
	// to EnterPlacementMode(Card1CardID) whenever hand slot 0 is empty (the M1
	// preservation path), so gating only PlayHandSlot would leave key 1 open.
	if (!CanPlayCardsWhilePossessing(GetPawn()))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': card '%s' refused — the hero is dead and the ghost cannot play cards (GHOST-§ G-5)."),
			*GetNameSafe(this), *CardID.ToString());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_HeroDead", "Hero is down"));
		return;
	}

	// miner cap at placement ENTRY (GDD §3.3, TASK-030): the 7th ALIVE miner is
	// refused before any gold or mode state moves — the §3.0 refund rule is
	// satisfied as a net-zero pre-check (M2 ruling); the message is the exact
	// §3.3 acceptance string. Re-gated at confirm (TryConfirmPlacement) in case
	// the cap somehow fills mid-placement.
	if (CardID == MinerCardID && !SiegeState->CanAddMiner())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': card '%s' refused — miner cap reached (%d alive; GDD §3.3)."),
			*GetNameSafe(this), *CardID.ToString(), SiegeState->GetAliveMinerCount());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_MinerCap", "Miner limit reached"));
		return;
	}

	bInPlacementMode = true;
	bPlacementValid = false;
	PlacementInvalidReason = EPlacementInvalidReason::Point;
	PendingCardID = CardID;
	PendingCost = Row->Cost;
	PendingCardType = Row->CardType; // selects the confirm spawn path + the §3.5 building clearance rule (TASK-030)
	PendingSwarmCount = Row->SwarmCount; // Swarm keyword (GDD §3.0): >0 spawns copies at confirm (Militia Mob = 4, TASK-059)
	bPendingIsBuilding = IsBuildingCard(CardID, Row->CardType); // building spawn path + §3.5 clearance, incl. Economy Deep Mine (TASK-059)
	bWarnedNoFootprintBounds = false; // TASK-735: the footprint degrade warning is per-CARD (the missing piece is this card's ghost mesh), so each session gets its ONE warning

	// TASK-813: the upgrade state belongs to a single placement session. Cleared on ENTRY as
	// well as on exit so a card can never inherit the previous card's blue hover — the very
	// first UpdatePlacementGhost call below rewrites both, but a member that is only ever
	// correct because something else runs immediately afterwards is a latent bug.
	PlacementUpgradeState = EPlacementUpgradeState::None;
	PlacementUpgradeTarget = nullptr;

	// ⭐⭐ TASK-815 (STACK-§4): the wheel's session state, seeded here and returned here.
	//
	// The scale starts at the TUNABLE, ⛔ never at a literal 1.0 — retuning
	// PlacementFootprintMin must retune the game with no second copy of the number to
	// find (HIGH-§1). Every session therefore opens at the authored footprint, and J-3's
	// "⛔ no shrinking" is enforced by the range rather than by a rule anyone must recall.
	PlacementFootprintScale = PlacementFootprintMin;

	// ⛔⛔ THE EXCLUSION, RESOLVED ONCE AND STRUCTURALLY (STACK-§2, spec (6)). ⛔ There is no
	// CardID compare here and there must never be one: the answer comes from the card's own
	// actor class, whose CDO answers ABuilding::CanScaleFootprint() — false on AClimbableTower,
	// so the WatchTower and every future climbable building are excluded by INHERITING.
	//
	// ⚠️ bPendingIsBuilding SHORT-CIRCUITS THE CLASS RESOLVE ITSELF, ⛔ not just the answer:
	// ResolveCardActorClass performs a LoadSynchronous, and a UNIT card must not pay for one
	// (nor emit its missing-class warning) for a rule that could only ever answer false for it.
	// This is the same short-circuit TryGetPlacementFootprintRadius applies to the bounds read.
	bPendingCardCanScaleFootprint =
		bPendingIsBuilding && CanCardActorScaleFootprint(ResolveCardActorClass(CardID, Row->CardType));

	// suppress hero melee while placement owns the LMB (TASK-003 API);
	// PlacementHero records exactly whose suppression we must release on exit
	PlacementHero = Hero;
	if (Hero)
	{
		Hero->SetMeleeSuppressed(true);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': entering placement mode without an AHeroCharacter pawn — no melee to suppress."),
			*GetNameSafe(this));
	}

	ApplyCursorInputState();
	SpawnPlacementGhost();
	UpdatePlacementGhost();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': placement mode entered for card '%s' (cost %d)."),
		*GetNameSafe(this), *CardID.ToString(), PendingCost);
}

void ASiegePlayerController::ExitPlacementMode()
{
	// QA-BINDING (qa/TASK-003-report.md warning 2): release the melee suppression
	// BEFORE any early-out. Every exit path — confirm, cancel (action or polled
	// RMB/Esc), match end, hero death, unpossess, EndPlay — funnels through here,
	// and AHeroCharacter::ResetHero deliberately does NOT clear the flag itself.
	if (IsValid(PlacementHero))
	{
		PlacementHero->SetMeleeSuppressed(false);
	}
	PlacementHero = nullptr;

	if (!bInPlacementMode)
	{
		return;
	}

	bInPlacementMode = false;
	bPlacementValid = false;
	PlacementInvalidReason = EPlacementInvalidReason::Point;
	PendingCardID = NAME_None;
	PendingCost = 0;
	PendingCardType = ECardType::Unit;
	PendingSwarmCount = 0;
	bPendingIsBuilding = false;
	// TASK-813: the blue state and its target die with the mode. ⛔ A surviving target would be
	// a building this controller could still upgrade with no ghost on screen to justify it.
	PlacementUpgradeState = EPlacementUpgradeState::None;
	PlacementUpgradeTarget = nullptr;
	// ⭐ TASK-815 (STACK-§4): the wheel setting dies with the card that was being placed.
	// ⛔ A surviving scale would size the NEXT building the player placed, with nothing on
	// screen to explain why it came out bigger — reset in BOTH Enter and Exit, the pattern
	// TASK-813's two members above established in exactly these two functions.
	PlacementFootprintScale = PlacementFootprintMin;
	bPendingCardCanScaleFootprint = false;
	PendingHandSlot = INDEX_NONE; // hand plays consume it in TryConfirmPlacement BEFORE this exit

	DestroyPlacementGhost();

	// restores game-only free-look — unless IA_UICursor is still held, in which
	// case the cursor stays up for the HUD (the two cursor owners compose)
	ApplyCursorInputState();
}

void ASiegePlayerController::HandleMatchEnd(ETeamId Winner)
{
	// exit placement AND targeting FIRST (spec + qa-note): destroys the
	// ghost/reticle and releases the melee suppression before the input mode
	// switches to UI-only (the modes are mutually exclusive — both calls are
	// no-op-safe)
	ExitPlacementMode();
	ExitTargetingMode();

	// same for the group-order pick (TASK-344): tear down the pick circles +
	// GroupPickHero suppression before the UI-only switch (no-op-safe). Formed
	// groups deliberately SURVIVE match end (their units are match-end frozen
	// anyway) — HandleMatchReset clears them for the next match.
	CancelGroupPick();

	// same for the war map (TASK-563): close it BEFORE the UI-only switch, so the
	// end screen is never under a full-screen panel. ⛔ CloseWarMap() also discards
	// any paid reveal — WR-§7's "no persistence across a match" half, delivered by
	// the same one call rather than by a second clear that could drift out of sync.
	// No-op-safe.
	CloseWarMap();

	// end any IA_UICursor hold too: UI-only input can swallow the action's
	// release event, which would leave the ignore-look counter stuck across
	// the end screen (the end screen owns the cursor from here anyway)
	ClearUICursorHold();

	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': HandleMatchEnd ignored — match already ended."),
			*GetNameSafe(this));
		return;
	}
	bMatchEnded = true;

	// ── M8 local-UI guard (TASK-356 doc §3.4.2): everything below is LOCAL
	// screen work — music, the end-screen widget, UI-only input. The SERVER-side
	// copy of a REMOTE client's PC (still walked by the GameMode/GameState server
	// paths for its state half — the bMatchEnded latch above feeds server-side
	// input validation) must never build widgets for a non-local player.
	// Standalone/host: local ⇒ the guard is a no-op (doc §10).
	if (!IsLocalController())
	{
		return;
	}

	// §6 victory/defeat music (TASK-179) — M8 own-team-relative (TASK-356 doc
	// §3.4.3/D14, retires the audit-§1b#8 "Blue won = Victory" hardcode): Victory
	// is MY team winning. Own PS team, Blue fallback (a dead-PS edge reads Blue —
	// the standalone identity, doc §10: MyTeam=Blue ⇒ identical branch).
	// Null-safe until S_VictoryMusic / S_DefeatMusic land (TASK-180).
	const ASiegePlayerState* MyPS = GetPlayerState<ASiegePlayerState>();
	const ETeamId MyTeam = MyPS ? MyPS->GetTeam() : ETeamId::Blue;
	USiegeFeedbackLibrary::PlaySound2D(this, (Winner == MyTeam) ? VictoryMusicSoundPath : DefeatMusicSoundPath);

	// end screen (TASK-011). Missing widget = log and continue — the match still
	// ends (input drops to UI-only; TASK-006's PlayAgain path recovers).
	if (UClass* ScreenClass = VictoryScreenClass.LoadSynchronous())
	{
		VictoryWidget = CreateWidget<UUserWidget>(this, ScreenClass);
		if (VictoryWidget)
		{
			// TASK-011 widget contract: an OPTIONAL BlueprintCallable function named
			// exactly "SetWinner" with a single ETeamId parameter. Called by name,
			// null-safe, BEFORE AddToViewport so Construct already sees the value.
			// M8 (D14): the ABSOLUTE-winner contract is UNCHANGED.
			static const FName SetWinnerName(TEXT("SetWinner"));
			if (UFunction* SetWinnerFunction = VictoryWidget->FindFunction(SetWinnerName))
			{
				if (SetWinnerFunction->ParmsSize == sizeof(ETeamId))
				{
					ETeamId WinnerParam = Winner;
					VictoryWidget->ProcessEvent(SetWinnerFunction, &WinnerParam);
				}
				else
				{
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("ASiegePlayerController '%s': WBP_VictoryScreen.SetWinner has an unexpected signature (expected exactly one ETeamId parameter) — Winner not passed."),
						*GetNameSafe(this));
				}
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegePlayerController '%s': WBP_VictoryScreen has no 'SetWinner' function — widget shows its defaults (contract in handoffs/TASK-007.md)."),
					*GetNameSafe(this));
			}

			// M8 OPTIONAL widget seam (TASK-356 doc §3.4.3/D14, sign-off §9.7): a
			// by-name, null-safe "SetLocalVictory" taking one bool — true when MY
			// team won — so a winning Red client can read "Victory" once the art
			// seam consumes it (TASK-355 optional item; absent = silent skip, the
			// widget's absolute SetWinner branch stands as the recorded P2 flag).
			static const FName SetLocalVictoryName(TEXT("SetLocalVictory"));
			if (UFunction* SetLocalVictoryFunction = VictoryWidget->FindFunction(SetLocalVictoryName))
			{
				if (SetLocalVictoryFunction->ParmsSize == sizeof(bool))
				{
					bool bLocalVictory = (Winner == MyTeam);
					VictoryWidget->ProcessEvent(SetLocalVictoryFunction, &bLocalVictory);
				}
				else
				{
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("ASiegePlayerController '%s': WBP_VictoryScreen.SetLocalVictory has an unexpected signature (expected exactly one bool parameter) — not passed."),
						*GetNameSafe(this));
				}
			}

			VictoryWidget->AddToViewport(/*ZOrder=*/ 10); // above the HUD
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': failed to create the victory screen from '%s'."),
				*GetNameSafe(this), *VictoryScreenClass.ToString());
		}
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': victory screen class '%s' not found (built in TASK-011) — match ended with no end screen."),
			*GetNameSafe(this), *VictoryScreenClass.ToString());
	}

	// ⭐ TASK-1319 (SC-§50) — UI-ONLY IS NOT APPLIED WHEN THERE IS NO UI. Both branches above
	// reach here with VictoryWidget still null — the class did not resolve, or CreateWidget
	// returned null — and until now they fell straight through into the UI-only block below.
	// ⛔ VictoryWidget != nullptr IS the "a screen is on the viewport" predicate, not a guess: it
	// is assigned in exactly ONE place (the CreateWidget above), AddToViewport is the last and
	// unconditional statement of that same branch, and HandleMatchReset nulls it again for the
	// next match — so reaching this line with it null means nothing was put on screen.
	//
	// ⛔ qa/TASK-007-report.md DECLARED THIS, verbatim: "HandleMatchEnd switches to
	// FInputModeUIOnly even when VictoryWidget is null ...: no UI exists to click and no game
	// input remains — a soft-lock in the degraded state. Suggest applying UIOnly only when the
	// widget was created (else keep GameAndUI + cursor so the session stays inspectable)."
	// It was declared there, named a second time by TASK-1311 and a third time at this row's
	// boarding, with no row against it until now (SC-§50: a declared gap with no row is an
	// orphan, and it ships).
	//
	// ⛔ WHAT IT COSTS — MEASURED AT SOURCE, NOT ASSUMED: FInputModeUIOnly::ApplyInputMode calls
	// GameViewportClient.SetIgnoreInput(true) (PlayerController.cpp:6384), and the first act of
	// UGameViewportClient::InputKey under that flag is
	//     if (IgnoreInput()) { return ViewportConsole ? ViewportConsole->InputKey(...) : false; }
	// — every key is swallowed before Enhanced Input or this controller ever sees it, so with no
	// widget on screen the ONLY surviving route is the developer console. ⛔ And that console does
	// not exist in the packaged game: ViewportConsole is constructed under #if ALLOW_CONSOLE
	// (GameViewportClient.cpp:2807-2809), and ALLOW_CONSOLE is ALLOW_CONSOLE_IN_SHIPPING == 0 in
	// Shipping (Core/Public/Misc/Build.h). ⇒ in the shipped build the degraded state is a TOTAL
	// soft-lock: no screen, no button, no key — only Alt+F4.
	//
	// ✅ THE FIX BINDS NOTHING AND INVENTS NOTHING (SC-§121 census: the set of keys this row makes
	// live is EMPTY — it adds no key constant, no input action, no mapping context and no input
	// binding of any kind, and the closed Escape ruling at AS-§6 A-2 is left exactly as it is).
	// It hands back the posture this controller ALREADY uses for every cursor surface in-match —
	// ApplyCursorInputState's GameAndUI arm, copied term for term — because "nothing on screen to
	// receive input" is precisely the state in which GAME input must stay live:
	// FInputModeGameAndUI::ApplyInputMode calls SetIgnoreInput(false) (PlayerController.cpp:6410),
	// so the player keeps every route they had one second earlier instead of a viewport that eats
	// all of them. ⛔ It opens no NEW route and is not meant to: the match-end refusals
	// (CanOpenAssistantConsole / CanOpenWarMap / CanOpenControlsHelp all return !bMatchEnded && ...)
	// are deliberately left exactly as they are.
	// ⚠️ ApplyCursorInputState() CANNOT be reused here: it early-outs while bMatchEnded is latched
	// (set above) BY DESIGN — HandleMatchEnd owns the end-of-match posture — so this arm states the
	// same terms inline rather than calling a function that would return without doing anything.
	if (!VictoryWidget)
	{
		bShowMouseCursor = true;
		bEnableClickEvents = true;

		FInputModeGameAndUI DegradedInputMode;
		DegradedInputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		DegradedInputMode.SetHideCursorDuringCapture(false);
		SetInputMode(DegradedInputMode);

		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': match ended with NO end screen — input left Game-and-UI with the cursor up instead of UI-only, because UI-only with nothing on screen swallows every key (qa/TASK-007-report.md WARN; TASK-1319)."),
			*GetNameSafe(this));

		// ⛔ THE SAME COMPLETION RECORD THE SUCCESS PATH WRITES BELOW, RE-EMITTED DELIBERATELY RATHER
		// THAN SHARED: hoisting it above that block would re-order the success path's logs, and
		// wrapping that block in an else would re-indent it — and TASK-1314 shipped it at 1d433ca on
		// the promise that it stays BYTE-IDENTICAL (TASK-1320 check 1 measures exactly that). A
		// duplicated three-line log is the cheaper of the two costs.
		//
		// ⛔⭐ THE TWO LINES BELOW ARE A PUBLISHED INTERFACE, NOT AN INTERNAL LOG (SC-§135). The
		// playtest-verifier lane greps the emitted line as the runtime evidence behind a VERIFIED
		// verdict — qa/TASK-1314-verify.md, qa/TASK-1311-verify.md and qa/TASK-1068-verify.md all
		// quote it — and ⛔ NO census over Source/, Tools/ or the suite can see that reader, because
		// it is an agent following VER-§, not a caller (SC-§50's orphan, inverted).
		// ⛔ THE PROTECTED SURFACE IS THE WHOLE EMITTED LINE: FORMAT STRING *AND* ARGUMENTS. What the
		// evidence quotes is the RENDERED text — `match ended — winner Red.` — and that "Red" is
		// produced by the argument line immediately below, not by the literal above it. So
		// TEXT("Blue") / TEXT("Red") are as published as the format string: renaming either token
		// (to TEXT("RED"), to a localised or UEnum-derived name) breaks the verifier's grep ⛔
		// IDENTICALLY to rewording the literal — and a reader who checks only the literal will find
		// it untouched and wrongly conclude they are safe.
		// ⛔ KEEP BOTH LINES BYTE-IDENTICAL WITH THE SUCCESS PATH'S COPY at the end of this function:
		// the argument line is duplicated too and must stay in sync for the same scraper reason. The
		// return just below makes the two sites mutually exclusive, so a scraper gets exactly one hit
		// per match end either way — a property preserved ONLY while BOTH pairs match.
		// ⛔ Do not de-duplicate WITHOUT A FRESH RULING: TASK-1320 TRADE 2 upheld the duplication as
		// the CHEAPER OF TWO COSTS (both alternatives break the byte-identity TASK-1314 shipped on),
		// and its WARN-2 sanctions a private LogMatchEnded(ETeamId) "at the point where re-indentation
		// is no longer a cost" — a conditional remedy, ⛔ NOT a permanent prohibition.
		// ⛔ REWORD EITHER COPY — LITERAL OR ARGUMENT — AND THE VERIFIER'S GREP RETURNS ZERO ⇒ the
		// lane reports UNOBSERVABLE, ⛔ NOT VERIFY-FAILED. It will not have observed a failure; it will
		// have failed to observe — so the breakage announces itself as "could not observe", which reads
		// at a glance like an ENVIRONMENT problem rather than a code change. That is a fail-silent in
		// the one lane whose entire job is to be the runtime witness (SC-§132), and it is why this
		// comment exists.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': match ended — winner %s."),
			*GetNameSafe(this), Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
		return;
	}

	// UI-only input for the end screen (GDD §3.9)
	// 🧑 KEYBOARD REACHABILITY — ANSWERED 2026-09-19 (TASK-1314-PLAYAGAIN-KEY-REACHABLE).
	// Jonathan ruled "yes, make Play Again keyboard reachable", closing the product question
	// TASK-1311 recorded as open at this very spot. THE TARGET IS THE BUTTON, NEVER THE ROOT:
	// /Game/UI/WBP_VictoryScreen's CDO is bIsFocusable = False AND its DesiredFocusWidget is
	// empty, so making the root focusable is necessary-but-NOT-sufficient — focus would sit on
	// a UUserWidget root, which is not an SButton, and Accept would activate nothing.
	// ⛔ Do not focus the root here. Focusing the BUTTON below bypasses DesiredFocusWidget
	// entirely, because it hands Slate one specific SWidget instead of asking the widget where
	// it wants focus. (Slate never discards a non-focusable target either — it re-homes UPWARD
	// to the nearest ancestor supporting keyboard focus, i.e. SViewport, which already holds
	// focus in-match. That, not a discard, is why TASK-1311's removed call was a true no-op.)
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	FInputModeUIOnly InputMode;

	// TASK-1314: put Slate keyboard focus on the Play Again button so the button's OWN Accept
	// path can reach it — Enter / SpaceBar / gamepad Accept → SButton::OnKeyDown (SButton.cpp:293,
	// via FNavigationConfig's Accept rules at NavigationConfig.cpp:32-34) → ExecuteOnClick.
	// ⛔ WE AUTHOR NO KEY BINDING AND MUST NOT: FInputModeUIOnly applies SetIgnoreInput(true)
	// (PlayerController.cpp:6384), and the FIRST statement of UGameViewportClient::InputKey under
	// that flag is an early return (GameViewportClient.cpp:767-770) — so Enhanced Input is deaf
	// under this mode by design, ~~and on L_Arena USiegeMenuInputSubsystem hard-returns before
	// binding anything at all (SiegeMenuInputSubsystem.cpp:47)~~. Slate's focus path is the ONLY
	// live route to this screen, and the one thing it needs from us is a focused, focusABLE
	// SButton.
	// ⚠️ TASK-1482 CORRECTED THE STRUCK CLAUSE RATHER THAN DELETING IT (SC-§120): TASK-1429 made
	// that subsystem arm ON DEMAND off the menu map, and TASK-1482's registration below is itself
	// such a demand — so it no longer "binds nothing" on L_Arena. The old clause was a second,
	// weaker argument for the same sentence, and it expired. (⚠️ Its line citation is quoted as it
	// was written and has since rotted; anchor that subsystem BY TEXT — the menu-map early return
	// in its Initialize() — per CITE-BY-TEXT-RULED-2026-09-24.)
	// ⛔⛔ AND THE SURVIVING SENTENCE IS TRUE OF A REAL KEY ONLY — NARROWED BY qa/TASK-1483.md, AND
	// THIS IS THE PREMISE THAT FAILED THIS ROW ONCE, SO IT IS CORRECTED WHERE IT WAS TAUGHT. The
	// viewport swallows every REAL key before Enhanced Input under UIOnly, so a human's Enter
	// reaches this button down Slate's focus path and by no other route. ⛔ BUT
	// InjectInputForAction NEVER TOUCHES THE VIEWPORT — it enters at the UEnhancedInputComponent,
	// DOWNSTREAM of that early return — so the armed IA_Menu* actions ARE live here for the agent
	// lane, which holds no other input verb at all. SiegeMenuInputSubsystem.h says exactly this in
	// a paragraph written about THIS screen; find it by text: "⚠️ THE ONE EXCEPTION, AND IT IS THE
	// VICTORY SCREEN" … "Injection is unaffected (`InjectInputForAction` never touches the
	// viewport)". ⇒ read "Slate's focus path is the ONLY live route" as "the only route a REAL key
	// has"; the register block below is where the injected route is argued in full.
	if (VictoryWidget)
	{
		// Read back from the asset's widget tree 2026-09-19 — the template-inherited name is the
		// SHIPPED one, so it is deliberately not "Btn_PlayAgain". WBP_VictoryScreen::Construct
		// parents a "Play Again" TextBlock into this button and binds its OnClicked to
		// RequestPlayAgain. ⛔ Renaming it in the asset silently breaks this lookup — the
		// Warning below is the only thing that would ever say so.
		static const FName PlayAgainButtonName(TEXT("Btn_Jump"));
		if (UWidget* PlayAgainButton = VictoryWidget->GetWidgetFromName(PlayAgainButtonName))
		{
			// Cached by the AddToViewport above, so this is a lookup and not a rebuild
			// (UWidget::TakeWidget_Private returns MyWidget when it is already valid).
			const TSharedRef<SWidget> PlayAgainSlate = PlayAgainButton->TakeWidget();

			// ⛔ THIS GUARD IS LOAD-BEARING, NOT DEFENSIVE PADDING. It asks the widget the EXACT
			// predicate the engine's focus-target setter asks at PlayerController.cpp:6343
			// (SWidget::SupportsKeyboardFocus), so this site is STRUCTURALLY INCAPABLE of
			// re-emitting the "InputMode:UIOnly - Attempting to focus Non-Focusable widget"
			// Error that TASK-1311 removed at 1c93610 — whatever the asset happens to say.
			// ⚠️ THE GUARD RESOLVES TRUE TODAY — AND THAT IS EXACTLY WHY IT STAYS. Btn_Jump once
			// carried an authored IsFocusable=False that overrode UButton's engine default of
			// true (Button.cpp:48), so the else-branch below was the live path. That property
			// was flipped in the Blueprint editor and hand-saved, and 1d433ca shipped the
			// asset; the package no longer serialises IsFocusable at all, which is how UE
			// records "equal to the default". UButton still exposes no runtime setter
			// (InitIsFocusable is constructor-time only, Button.h:205-206), so C++ cannot
			// re-assert this if the asset regresses — the guard above is the only backstop.
			if (PlayAgainSlate->SupportsKeyboardFocus())
			{
				InputMode.SetWidgetToFocus(PlayAgainSlate);
			}
			else
			{
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegePlayerController '%s': victory screen button '%s' resolved, but its Slate widget reports SupportsKeyboardFocus() == false — no focus target set, Play Again stays mouse-only. Check that button's IsFocusable in WBP_VictoryScreen."),
					*GetNameSafe(this), *PlayAgainButtonName.ToString());
			}
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': victory screen has no widget named '%s' — no focus target set, Play Again is not keyboard-reachable (TASK-1314)."),
				*GetNameSafe(this), *PlayAgainButtonName.ToString());
		}
	}

	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// ═══ ⭐ TASK-1482 [VICTORY-SCREEN-NAVIGABLE] — REGISTER ON OPEN ════════════════════════════
	// ⛔ THE ONE THING THAT WAS MISSING IS A REGISTRATION, NOT A KEY. TASK-1314 (twenty lines
	// above) already puts Slate focus on Btn_Jump and TASK-1431 already shipped that button's
	// IsFocusable = True, so the screen was REACHABLE. What it was not is ENUMERABLE: nothing
	// ever named it to USiegeMenuInputSubsystem, so GetActiveNavTarget() never pointed at it and
	// LogNavTargetRetarget never printed a focus-stop count for it — the instrument the board's
	// EVENTGRAPH-CENSUS-GAP ruling makes binding, and this call is the only path that emits it.
	//
	// 🚨 THE PLAIN RegisterMenuNavTarget — AND THE PREMISE THAT ONCE SAID OTHERWISE IS STRUCK HERE
	// RATHER THAN DELETED (SC-§120), BECAUSE THE FALSE HALF OF IT IS SUBTLE AND WILL BE REACHED
	// FOR AGAIN. This row first shipped RegisterSelfDrivingMenuNavTarget on this reasoning:
	// ~~⇒ the subsystem's IA_Menu* handlers CANNOT fire on this screen, so three of the four
	// behaviours the declaration suppresses — MoveFocus(±1), HandleMenuAccept() and
	// StepFocusedStop(±1) — are already structurally unreachable here and declining them
	// subtracts nothing that could ever have run.~~ ⛔ OVERRULED BY qa/TASK-1483.md, AND THE
	// CORRECTION IS ONE WORD WIDE: FInputModeUIOnly's SetIgnoreInput(true)
	// (PlayerController.cpp, "FInputModeUIOnly::ApplyInputMode" → SetIgnoreInput(true)) deafens
	// this screen TO A REAL KEY ONLY. UGameViewportClient::InputKey does early-return under
	// IgnoreInput() (GameViewportClient.cpp, the `if (IgnoreInput())` return, ABOVE the
	// OnInputKeyEvent.Broadcast / player-routing block — not literally its first statement), but
	// UEnhancedInputLocalPlayerSubsystem::InjectInputForAction NEVER TOUCHES THE VIEWPORT: it
	// enters at the UEnhancedInputComponent, downstream of that return.
	// ⛔ AND INJECTION IS THE ONLY INPUT VERB THE AGENT LANE HOLDS. SiegeMenuInputSubsystem.h says
	// both things already, the second in a paragraph written about THIS screen — search it by
	// text: "⚠️ THE ONE EXCEPTION, AND IT IS THE VICTORY SCREEN" … "Injection is unaffected
	// (`InjectInputForAction` never touches the viewport)", and, earlier, "What DOES reach them:
	// `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction` (Aura's `inject_input_action`,
	// and the automation test)" with "no real-input lane exists for an agent" alongside it.
	// ⇒ under the flag an injected IA_MenuAccept returns at HandleMenuAccept's FIRST statement,
	// DeclineIfActiveTargetSelfDriving(TEXT("IA_MenuAccept")), and nothing presses this button.
	// Registered-but-declining is ENUMERABLE. It is not NAVIGABLE.
	//
	// ⭐ THE DISCRIMINATOR TO CARRY FORWARD, IN ONE SENTENCE: the flag declares "this screen
	// DRIVES ITS OWN NAVIGATION", and THE VICTORY SCREEN HAS NO DRIVER. The deck builder holds
	// the flag legitimately because it does — its own HandleMenuNavAccept → HandleCardGridKey →
	// AcceptFocusedCard is a SEPARATE delegate on the same action, so gating the subsystem's
	// Accept does not cost it its card-pick (qa/TASK-1472.md). ⛔ SELF-DRIVING WITHOUT A DRIVER IS
	// JUST DEAF.
	//
	// ⛔ THE RACE THE FLAG WAS CHOSEN TO AVOID IS DETERMINATE AND CONVERGES — MEASURED IN
	// qa/TASK-1483.md, NOT ARGUED. (a) The admitted-class population of this tree is the SINGLETON
	// /Game/UI/WBP_VictoryScreen.WBP_VictoryScreen:WidgetTree.Btn_Jump — the two UI_Thumbstick_C
	// riders are Collapsed, are not one of the four admitted classes, and are not descended into —
	// so RegisterMenuNavTarget's closing FocusFirstNavStop() places the ring on stop 0, which IS
	// the button SetWidgetToFocus named above, by construction and not by luck. (b) This call sits
	// AFTER SetInputMode, which only DEFERS its focus request into the local player's FReply;
	// FocusWidget then performs an IMMEDIATE FSlateApplication::SetUserFocus(..., Navigation) and
	// either wins — whereupon FReply::CancelFocusRequest clears ONLY the focus fields, leaving the
	// input mode's capture and lock operations intact — or loses and writes the same widget into
	// the same FReply. ⭐ BOTH BRANCHES END ON THE SAME WIDGET WITH CAUSE Navigation.
	// (c) ⛔ THAT IS WHY SetWidgetToFocus ABOVE STAYS AND MUST NOT BE REMOVED: on the degenerate
	// zero-stop branch FocusFirstNavStop() returns false WITHOUT cancelling anything, and the
	// by-name placement survives verbatim. It is the fallback, not a duplicate.
	//
	// ⭐ AND THE COST NOBODY HAD PRICED, WHICH IS 🧑 LITERALLY THE ASK THIS MILESTONE STARTED FROM:
	// EFocusCause::Navigation is the ONLY cause that paints a ring. FSlateApplication::SetUserFocus
	// computes ShowFocus = (InCause == EFocusCause::Navigation); UGameViewportClient::QueryShowFocus
	// refuses any other cause under the ENGINE-DEFAULT ERenderFocusRule::NavigationOnly (not
	// overridden anywhere in this project's Config/); SWidget::Paint draws GetFocusBrush() only when
	// ShowUserFocus is true. And FInputModeDataBase::SetFocusAndLocking deposits SetWidgetToFocus's
	// request with the DEFAULT cause, SetDirectly. ⇒ under the self-driving flag this button could
	// NEVER wear the dashed FocusRectangle — the one screen of the ten with no outline, when the
	// outline is the first clause of the sitting. The plain call is what restores it.
	// ⭐ AND IT KEEPS EVERYTHING THE FLAG WAS KEEPING: the stack entry, the retarget line WITH THIS
	// SCREEN'S FOCUS-STOP COUNT, and the in-match arm. Registering is what makes the screen
	// enumerable; the PLAIN registration is what also makes it navigable by the agent lane.
	//
	// ⛔ ON OPEN — NOT IN A CONSTRUCTOR, AND NOT EARLIER IN THIS FUNCTION. qa/TASK-1430.md WARN-2
	// measured the arming backstop as DISARM-ONLY, so a screen that registers while hidden never
	// arms and NOTHING ERRORS. This site is past AddToViewport and past SetInputMode, so the
	// IsInViewport() && IsVisible() re-read inside IsInMatchScreenOpen() sees the true post-open
	// state and the retarget line cannot describe a screen the viewport has not got yet.
	// ⛔ VictoryWidget is non-null by construction here: the !VictoryWidget block above RETURNS.
	// ⛔ Paired with UnregisterMenuNavTarget(VictoryWidget) in HandleMatchReset — the only other
	// site in this class that touches this pointer, and the only close path there is.
	if (UWorld* World = GetWorld())
	{
		if (USiegeMenuInputSubsystem* MenuInput = World->GetSubsystem<USiegeMenuInputSubsystem>())
		{
			MenuInput->RegisterMenuNavTarget(VictoryWidget);
		}
		else
		{
			// Log, not Warning, on USettingsMenuWidget::RegisterAsMenuNavTarget's stated
			// rationale: the honest reading of a null here is "this world has no menu input"
			// (the subsystem declines Editor worlds outright, DoesSupportWorldType = Game | PIE),
			// and the end screen still works with the mouse and with the Slate focus set above.
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': no USiegeMenuInputSubsystem on this world — the victory screen is not registered as a nav target (mouse and Slate keyboard focus are unaffected; no focus-stop count will be logged)."),
				*GetNameSafe(this));
		}
	}

	// ⛔⭐ THE TWO LINES BELOW ARE A PUBLISHED INTERFACE, NOT AN INTERNAL LOG (SC-§135). The
	// playtest-verifier lane greps the emitted line as the runtime evidence behind a VERIFIED
	// verdict — qa/TASK-1314-verify.md, qa/TASK-1311-verify.md and qa/TASK-1068-verify.md all
	// quote it — and ⛔ NO census over Source/, Tools/ or the suite can see that reader, because
	// it is an agent following VER-§, not a caller (SC-§50's orphan, inverted).
	// ⛔ THE PROTECTED SURFACE IS THE WHOLE EMITTED LINE: FORMAT STRING *AND* ARGUMENTS. What the
	// evidence quotes is the RENDERED text — `match ended — winner Red.` — and that "Red" is
	// produced by the argument line immediately below, not by the literal above it. So
	// TEXT("Blue") / TEXT("Red") are as published as the format string: renaming either token
	// (to TEXT("RED"), to a localised or UEnum-derived name) breaks the verifier's grep ⛔
	// IDENTICALLY to rewording the literal — and a reader who checks only the literal will find
	// it untouched and wrongly conclude they are safe.
	// ⛔ KEEP BOTH LINES BYTE-IDENTICAL WITH THE DEGRADED PATH'S COPY inside the !VictoryWidget block
	// above: the argument line is duplicated too and must stay in sync for the same scraper reason.
	// That block's early return makes the two sites mutually exclusive, so a scraper gets exactly one
	// hit per match end either way — a property preserved ONLY while BOTH pairs match.
	// ⛔ Do not de-duplicate WITHOUT A FRESH RULING: TASK-1320 TRADE 2 upheld the duplication as
	// the CHEAPER OF TWO COSTS (both alternatives break the byte-identity TASK-1314 shipped on),
	// and its WARN-2 sanctions a private LogMatchEnded(ETeamId) "at the point where re-indentation
	// is no longer a cost" — a conditional remedy, ⛔ NOT a permanent prohibition.
	// ⛔ REWORD EITHER COPY — LITERAL OR ARGUMENT — AND THE VERIFIER'S GREP RETURNS ZERO ⇒ the
	// lane reports UNOBSERVABLE, ⛔ NOT VERIFY-FAILED. It will not have observed a failure; it will
	// have failed to observe — so the breakage announces itself as "could not observe", which reads
	// at a glance like an ENVIRONMENT problem rather than a code change. That is a fail-silent in
	// the one lane whose entire job is to be the runtime witness (SC-§132), and it is why this
	// comment exists.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': match ended — winner %s."),
		*GetNameSafe(this), Winner == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
}

void ASiegePlayerController::HandleMatchReset()
{
	// Defensive exit FIRST (qa/TASK-023-report.md WARN, QA-sanctioned fix): an
	// out-of-contract HandleMatchReset call mid-placement must never rebuild
	// the hand under a live PendingHandSlot — the later confirm would consume
	// an unrelated card from the fresh hand. In the contract flow (PlayAgain
	// after HandleMatchEnd) placement already exited and this is a no-op;
	// "reset restores play" either way.
	ExitPlacementMode();

	// same WARN rationale for M5 targeting mode (TASK-100): the deck rebuild
	// below must never happen under a live TargetingHandSlot
	ExitTargetingMode();

	// same defensive teardown for the group-order pick (TASK-344)
	CancelGroupPick();

	// same defensive teardown for the war map (TASK-563). ⛔ THIS IS WR-§7's
	// "⛔ NO PERSISTENCE ACROSS PLAY AGAIN / MATCH RESET" CLAUSE, hooked onto the
	// EXISTING reset path exactly as the spec asked rather than onto a new one:
	// CloseWarMap() discards the paid reveal through the widget's own unconditional
	// clear, so the next match starts with no red dots and no credit for the last
	// 30 gold. No-op-safe.
	CloseWarMap();

	bMatchEnded = false;

	// M8 local-UI guard (TASK-356 doc §3.4.2): widget/input work is LOCAL screen
	// state — the server-side copy of a remote client's PC (walked by PlayAgain
	// step 6 for its STATE half: the latch above, the deck + stance below) never
	// created a widget and must not touch input modes. Standalone: local ⇒ no-op.
	if (IsLocalController())
	{
		// idempotent with WBP_VictoryScreen's own RemoveFromParent (TASK-011)
		if (VictoryWidget)
		{
			// ═══ ⭐ TASK-1482 — UNREGISTER ON CLOSE, AND **BEFORE** RemoveFromParent ═══════════
			// The falling edge paired with HandleMatchEnd's RegisterMenuNavTarget. (qa/TASK-1483.md
			// loop 1 changed that open edge from RegisterSelfDrivingMenuNavTarget to the plain
			// call. UnregisterMenuNavTarget was already the correct pairing for BOTH, so not one
			// character of the call below moved — only the count in the next sentence.)
			// ⛔ TWO halves now, not three — struck in place (SC-§120): it drops the stack entry,
			// ~~it drops the self-driving declaration (UnregisterMenuNavTarget clears the mark
			// HERE and nowhere else, so the flag's lifetime is exactly the registration's),~~ and
			// its ReconcileInMatchArming REMOVES IMC_MainMenu once no live registration is left —
			// so the match gets its keys back on an EDGE instead of waiting up to
			// InMatchDemandPollSeconds for the disarm-only backstop poll to notice.
			// ⛔ BEFORE RemoveFromParent, on the subsystem's OWN stated shape — its unregister
			// comment reads "an unregister runs from `BackPressed`, BEFORE `RemoveFromParent`".
			// ⭐ Idempotent either way, which matters because WBP_VictoryScreen's graph may have
			// already removed itself from the viewport on the Play Again click: in that case
			// GetRegisteredNavTarget()'s IsInViewport() re-read has already dropped the entry,
			// and this call removes by IDENTITY and LOGS — does not warn — that there was
			// nothing to remove. Silent on a null subsystem, on
			// USettingsMenuWidget::UnregisterAsMenuNavTarget's rationale: if there was no
			// subsystem to register with there is nothing to give back.
			if (UWorld* World = GetWorld())
			{
				if (USiegeMenuInputSubsystem* MenuInput = World->GetSubsystem<USiegeMenuInputSubsystem>())
				{
					MenuInput->UnregisterMenuNavTarget(VictoryWidget);
				}
			}

			VictoryWidget->RemoveFromParent();
			VictoryWidget = nullptr;
		}

		// belt-and-braces: any IA_UICursor hold that survived the end screen (its
		// release is swallowed under UI-only input) is cleared before play resumes,
		// so free-look can never come back permanently suspended. A physically
		// still-held Alt re-arms on its next press (same rule as sprint,
		// qa/TASK-003-report.md nit 5).
		ClearUICursorHold();
	}

	// fresh §3.4 deal for the new match — the ResetDeck half of the TASK-023
	// deck-timing contract. ASiegeGameMode::PlayAgain already calls
	// HandleMatchReset on every controller (TASK-006), which makes this the
	// SINGLE controller-side §3.9 deck-reset entry point TASK-024's PlayAgain v2
	// consumes. qa/TASK-024-report.md WARN-1 (PlayAgain double ResetDeck) is now
	// CLOSED on TASK-030: the redundant game-mode-side ResetDeck call was dropped
	// from ASiegeGameMode::PlayAgain, so this is the only remaining reset.
	if (DeckComponent)
	{
		DeckComponent->ResetDeck();
	}

	// W1 unit-command reset (TASK-274 step 5): Play Again returns the stance to
	// the default Attack and clears the issued-latch (units run the legacy body
	// again until the player re-commands). Broadcast the reset so the HUD
	// indicator (TASK-276) clears — it re-checks HasIssuedCommand() (now false)
	// and shows nothing. CancelGroupPick above already tore down any in-progress
	// pick; ClearAllUnitGroups below is the TASK-344 Play-Again release — every
	// group order and its markers die, members explicitly cleared.
	CurrentCommand = ESiegeUnitCommand::Attack;
	bHasIssuedCommand = false;
	OnUnitCommandChanged.Broadcast(CurrentCommand);
	ClearAllUnitGroups();

	// back to M1 game-only free-look (both cursor owners are clear by now).
	// M8: input posture is local-only (the doc §3.4.2 UI-half guard).
	if (IsLocalController())
	{
		ApplyCursorInputState();
	}
}

void ASiegePlayerController::PerformLocalMatchReset()
{
	// M8 (TASK-356 doc §3.4.2): the CLIENT-local mirror of PlayAgain step 6,
	// driven by ASiegeGameState::OnRep_MatchEnded's false edge — the server-side
	// controller walk cannot reach this machine's local PC. HandleMatchReset
	// already carries the local deck ResetDeck() (the single controller-side
	// §3.9 deck-reset entry point), so the whole local reset is one call. All
	// other client state converges via the Castle/PS/GameState/scatter OnReps
	// (each reaction independent and order-tolerant — cross-actor OnRep order is
	// not guaranteed; noted for QA).
	HandleMatchReset();
}

void ASiegePlayerController::RequestPlayAgain()
{
	// M8 Play-Again routing (TASK-356 doc §3.4.2/§4.2): authority (host or
	// standalone) calls the GameMode directly — the exact call the widget made,
	// byte-identical (doc §10); a client relays through the ONE P1 RPC (the
	// GameMode does not exist on clients, D6).
	if (HasAuthority())
	{
		if (ASiegeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASiegeGameMode>() : nullptr)
		{
			GameMode->PlayAgain();
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': RequestPlayAgain — no ASiegeGameMode resolved (GameModeClass mis-config?)."),
				*GetNameSafe(this));
		}
		return;
	}

	UE_LOG(LogSiegeNet, Log,
		TEXT("ASiegePlayerController '%s': RequestPlayAgain — client relay via ServerRequestPlayAgain (M8 doc §4.2)."),
		*GetNameSafe(this));
	ServerRequestPlayAgain();
}

bool ASiegePlayerController::ServerRequestPlayAgain_Validate()
{
	// No input payload to validate (doc §4.2) — the implementation's
	// HasMatchEnded() check is the intent validation.
	return true;
}

void ASiegePlayerController::ServerRequestPlayAgain_Implementation()
{
	// ⚖️ M8 loop-1 FINDING-4 self-diagnosis (TASK-357 measured this body running
	// ON THE CLIENT). A Server RPC's implementation must only ever execute on the
	// authority; if it does not, the callspace resolved Local instead of Remote
	// and the request never reached the host. Say so precisely — the old code
	// fell through to "no ASiegeGameMode on the server (mis-config?)", which is
	// what made the symptom ambiguous — and refuse to act, so a client can never
	// locally reset a match. (Confirmed cause of the TASK-357 occurrence: the
	// python remote-exec trigger runs inside FEditorScriptExecutionGuard, whose
	// ctor sets GAllowActorScriptExecutionInEditor = true (ScriptCore.cpp:451-455),
	// and AActor::GetFunctionCallspace returns FunctionCallspace::Local on that
	// global as its VERY FIRST branch (Actor.cpp:5469-5474) — a tooling artifact
	// of the invoke path, not a routing defect. This guard makes the real
	// widget-button path self-reporting either way.)
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Error,
			TEXT("ASiegePlayerController '%s': ServerRequestPlayAgain executed WITHOUT authority — the RPC resolved LOCAL instead of routing to the host (callspace/tooling issue); refusing. Play Again must be pressed on the host until this is resolved."),
			*GetNameSafe(this));
		return;
	}

	// Runs ON THE SERVER for the owning client (Reliable). Only a genuinely
	// ended match may reset — a mid-match spam press reaches nothing (doc §4.2).
	ASiegeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASiegeGameMode>() : nullptr;
	if (!GameMode)
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("ASiegePlayerController '%s': ServerRequestPlayAgain — no ASiegeGameMode on the server (mis-config?)."),
			*GetNameSafe(this));
		return;
	}

	if (!GameMode->HasMatchEnded())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': ServerRequestPlayAgain ignored — the match has not ended (intent validation, doc §4.2)."),
			*GetNameSafe(this));
		return;
	}

	UE_LOG(LogSiegeNet, Log,
		TEXT("ASiegePlayerController '%s': ServerRequestPlayAgain — client-initiated Play Again accepted (M8 gate e)."),
		*GetNameSafe(this));
	GameMode->PlayAgain();
}

ASiegePlayerController* ASiegePlayerController::FindControllerForTeam(UWorld* World, ETeamId Team)
{
	// M8 owning-team-controller resolve (TASK-356 doc §3.7 — the ruling-4
	// replacement pattern). Null-safe: no world / no teamed PC ⇒ nullptr (the
	// unit call sites already handle a null PC exactly as they handled a null
	// first-controller). The bot is an AAIController — never in this iterator —
	// so a Red resolve in standalone is null exactly as the old code's
	// first-controller-then-team-gate produced.
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ASiegePlayerController* SiegePC = Cast<ASiegePlayerController>(It->Get());
		if (!SiegePC)
		{
			continue;
		}
		const ASiegePlayerState* SiegePS = SiegePC->GetPlayerState<ASiegePlayerState>();
		if (SiegePS && SiegePS->GetTeam() == Team)
		{
			return SiegePC;
		}
	}

	return nullptr;
}

void ASiegePlayerController::TryConfirmPlacement()
{
	// invalid click: refuse, spend NOTHING, STAY in placement mode (GDD §3.5) —
	// a different point can succeed, so the mode survives the refusal. The
	// reason recorded by UpdatePlacementGhost picks the player-facing message
	// (each building-only rule gets its own — §3.5 clearance plus the M4.5
	// slope and obstacle gates, TASK-093). Every branch runs BEFORE any gold
	// moves (§3.0 net-zero refusal law).
	if (!bPlacementValid)
	{
		switch (PlacementInvalidReason)
		{
		case EPlacementInvalidReason::Slope:
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — ground steeper than %.0f degrees (GDD §5 M4.5 slope rule, TASK-093)."),
				*GetNameSafe(this), *PendingCardID.ToString(), MaxPlacementSlopeDegrees);
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_TooSteep", "Too steep"));
			break;

		case EPlacementInvalidReason::Obstacle:
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — within %.0f units of an Obstacle-tagged actor (GDD §5 M4.5 obstacle rule, TASK-093)."),
				*GetNameSafe(this), *PendingCardID.ToString(), ObstaclePlacementClearance);
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_ObstacleClearance", "Too close to obstacles"));
			break;

		case EPlacementInvalidReason::Clearance:
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — within %.0f units of another building (GDD §3.5 clearance)."),
				*GetNameSafe(this), *PendingCardID.ToString(), BuildingClearance);
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_BuildingClearance", "Too close to another building"));
			break;

		case EPlacementInvalidReason::Units:
			// ⭐ TASK-735 (TOWER-§7 / T-6): the building's mesh-derived footprint
			// covers one of the player's OWN units. ⛔ NO gold moves and ⛔ NO unit
			// moves — the placement is simply refused, and a different point (or
			// an order to the army) succeeds. ⛔ Its OWN message, deliberately not
			// the Clearance one: "another building" would be a lie the player
			// cannot act on.
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — a live own-team unit stands inside the building's footprint (TASK-735, TOWER-§7; extra clearance %.0f)."),
				*GetNameSafe(this), *PendingCardID.ToString(), UnitPlacementClearance);
			RefuseCardPlay(PendingCardID, UnitFootprintRefusalText());
			break;

		case EPlacementInvalidReason::Upgrade:
			// ⭐ TASK-813 (STACK-§2, J-5): the cursor is over one of the player's OWN buildings
			// of the card in hand, and the UPGRADE is refused. TWO sub-cases, and each gets the
			// message the player can actually act on — read off PlacementUpgradeState, which
			// UpdatePlacementGhost recomputes in the SAME block as this reason, so the RED the
			// player saw and the line this click prints can ⛔ never disagree.
			// ⛔ NO gold moves on either branch (§3.0 net-zero refusal law) and the player STAYS
			// in placement mode, so a different building — or a different point — still works.
			if (PlacementUpgradeState == EPlacementUpgradeState::Unaffordable)
			{
				UE_LOG(LogGitClaudeUnrealTest, Log,
					TEXT("ASiegePlayerController '%s': upgrade click refused for '%s' — costs %d gold and the player cannot afford it (STACK-§5 J-5: blue never promises a click that refuses)."),
					*GetNameSafe(this), *PendingCardID.ToString(), PendingCost);
				// ⭐ the SHIPPED refusal, key for key — one "Not enough gold" in the game.
				RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
			}
			else
			{
				// ⚖️ Log, ⛔ NOT Warning and ⛔ NOT Error: a building that refuses to grow is a
				// NOT-A-TARGET, not a fault. Nothing here is exceptional — it is the fifth
				// entry in a family of four shipped refusals.
				// ⚠️ THE PREDICATE THIS LINE NAMES CHANGED ON 2026-09-03 (STACK-§8) and the
				// message was updated with it: a log line naming the WRONG predicate is how the
				// next playtest report gets misdiagnosed, which is exactly what happened here.
				UE_LOG(LogGitClaudeUnrealTest, Log,
					// ⛔ Predicate named WITHOUT its parentheses — see ConfirmStackUpgrade's own
					// note: a literal carrying the call shape would satisfy the gate census by
					// itself (`SC-§41`).
					TEXT("ASiegePlayerController '%s': upgrade click refused for '%s' — the hovered building refuses to be grown in HEIGHT (its CanStackHeight predicate is false; STACK-§8 — ⛔ this is NOT the placement wheel's)."),
					*GetNameSafe(this), *PendingCardID.ToString());
				RefuseCardPlay(PendingCardID, StackNotStackableRefusalText());
			}
			break;

		default:
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — no ground hit, outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh (W1-PREP additions 3, TASK-261; GDD §3.5, TASK-030; plinth keep-out retired, TASK-349)."),
				*GetNameSafe(this), *PendingCardID.ToString());
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_InvalidPoint", "Invalid placement location"));
			break;
		}
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': no ASiegePlayerState at confirm — cannot spend gold, staying in placement mode."),
			*GetNameSafe(this));
		return;
	}

	// ⭐⭐ TASK-813 — THE BLUE CLICK, AND IT RETURNS BEFORE EVERY SPAWN RULE BELOW.
	// "instead of placing a new tower, it will instead double the height of the old one" ⇒
	// ⛔ nothing is spawned, so the miner cap, the BP-class resolve and both spawn branches
	// are not merely skipped, they are INAPPLICABLE. Placed here — after the PlayerState
	// resolve it needs for gold, before anything that creates an actor — so the whole upgrade
	// path is one branch a reviewer can read in one place.
	if (PlacementUpgradeState == EPlacementUpgradeState::Ready)
	{
		ConfirmStackUpgrade(*SiegeState);
		return;
	}

	// miner cap re-gate at CONFIRM (GDD §3.3, TASK-030 — entry already gated;
	// the cap cannot grow mid-placement in M2, so this is belt-and-braces).
	// Unlike an invalid point, NO other click can fix a full cap — exit the
	// mode (funneled through ExitPlacementMode, so melee suppression is
	// released — qa/TASK-003 warning 2 law) with the card still in hand and
	// zero gold moved (§3.0 net-zero refund ruling).
	if (PendingCardID == MinerCardID && !SiegeState->CanAddMiner())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': confirm refused for '%s' — miner cap reached (%d alive; GDD §3.3)."),
			*GetNameSafe(this), *PendingCardID.ToString(), SiegeState->GetAliveMinerCount());
		RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_MinerCap", "Miner limit reached"));
		ExitPlacementMode();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// composed soft-class resolve by CardType (CONVENTIONS; TASK-030): a
	// missing/incompatible BP refuses the play with NO gold spent and exits
	// the mode — re-clicking cannot make the asset exist. Retires the M1
	// meshless-ASummonedUnit fallback per the spec.
	UClass* ActorClass = ResolveCardActorClass(PendingCardID, PendingCardType);
	if (!ActorClass)
	{
		RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoActorClass", "Card actor unavailable"));
		ExitPlacementMode();
		return;
	}

	// local player is always Blue (CONVENTIONS team contract); mirror the hero's
	// team when we have one so the two can never disagree — equals the TASK-027
	// contract's literal Team=Blue in every M2 world
	const ETeamId Team = IsValid(PlacementHero) ? PlacementHero->GetTeamId() : ETeamId::Blue;

	AActor* Spawned = nullptr;
	if (bPendingIsBuilding)
	{
		// Building path (TASK-027 spawn contract): buildings spawn AT the
		// clicked point, flush with the traced ground — the root is
		// Static-mobility and must never move post-spawn, so AlwaysSpawn (no
		// adjustment) keeps the actor exactly where the ghost stood.
		// Instigator deliberately nullptr: tower shots must stay unattributable
		// (qa/TASK-027-report.md ruling 11's verified property).
		//
		// ⭐⭐ TASK-815 (STACK-§4 spec (4)) — THE GHOST AND THE SPAWNED BUILDING MUST
		// AGREE, AND THEY AGREE BY CONSTRUCTION: the ghost's scale and this one are the
		// SAME member run through the SAME function, so there is no second expression to
		// drift. ⚖️ A ghost that lies about the thing it previews is the defect this whole
		// placement path exists to avoid — and it would be a lie with teeth here, because
		// the footprint clearance the click was validated against was measured off the
		// SCALED ghost (STACK-§6).
		//
		// ⭐ VisualMesh IS ABuilding's ROOT, so this transform's scale lands as its relative
		// scale — which is exactly what ApplyStackUpgrade later reads, preserves on X/Y and
		// recomputes on Z alone. ⇒ J-4 ("keeping the same width and length") is satisfied
		// with ⛔ zero coordination between the two features. Collision and the navmesh hole
		// scale with it for free (Building.h:43-51 — BlockAll + bCanEverAffectNavigation).
		//
		// ⛔ NO SECOND EXCLUSION CHECK IS NEEDED OR WANTED HERE (spec (6)): a card whose
		// class refuses footprint scaling never moved this member off PlacementFootprintMin,
		// because ApplyPlacementFootprintWheel refused to write it.
		const FTransform SpawnTransform(
			FRotator::ZeroRotator, PlacementLocation, MakePlacementFootprintScale3D(PlacementFootprintScale));
		ABuilding* Building = World->SpawnActorDeferred<ABuilding>(
			ActorClass,
			SpawnTransform,
			/*Owner=*/ this,
			/*Instigator=*/ nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Building)
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': SpawnActorDeferred failed for building '%s' (%s) — no gold spent, staying in placement mode."),
				*GetNameSafe(this), *PendingCardID.ToString(), *GetNameSafe(ActorClass));
			return;
		}

		// gold is the last gate before commit: a refusal destroys the
		// half-spawned actor, so exactly Cost is deducted if and only if a
		// building appears (§3.5)
		if (!SiegeState->SpendGold(PendingCost))
		{
			Building->Destroy();
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': SpendGold(%d) refused at confirm for '%s' — staying in placement mode."),
				*GetNameSafe(this), PendingCost, *PendingCardID.ToString());
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
			return;
		}

		// deferred init BEFORE FinishSpawning → BeginPlay binds stats with Team
		// and CardID already set. ALWAYS the REAL CardID, never NAME_None
		// (qa/TASK-027-report.md WARN-2, binding: NAME_None would wipe the BP
		// child's preset row and leave a statless 0-HP building).
		Building->InitBuilding(Team, PendingCardID);
		Building->FinishSpawning(SpawnTransform);
		Spawned = Building;
	}
	else
	{
		// Unit/Economy path — spawn via the SHARED swarm entry (TASK-059), which the
		// bot (TASK-060) reuses so player and bot swarms match. A Unit card with
		// SwarmCount > 0 (Militia Mob = 4) spawns that many copies in a
		// SwarmSpawnRadius circle for ONE Cost; everything else spawns a single unit
		// at the validated point (byte-for-byte with TASK-007/030). Spawn FIRST, then
		// gate gold — the M1 discipline generalized to N: gold moves iff at least one
		// unit committed, and a spend refusal unwinds EVERY copy (net-zero refund,
		// §3.0). Gold cannot actually drop during placement (plays/discards are refused
		// in-mode; income only adds), so the spend refusal below is defensive.
		TArray<ASummonedUnit*> SwarmUnits = SpawnUnitSwarm(
			World, ActorClass, PendingCardID, Team,
			/*Owner=*/ this, /*Instigator=*/ GetPawn(),
			PlacementLocation, PendingSwarmCount, SwarmSpawnRadius);
		if (SwarmUnits.Num() == 0)
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': SpawnUnitSwarm produced no units for '%s' (%s) — no gold spent, staying in placement mode."),
				*GetNameSafe(this), *PendingCardID.ToString(), *GetNameSafe(ActorClass));
			return;
		}

		// gold is the last gate before commit: a refusal destroys EVERY spawned copy,
		// so exactly one Cost is deducted if and only if the swarm appears (§3.5)
		if (!SiegeState->SpendGold(PendingCost))
		{
			for (ASummonedUnit* SwarmUnit : SwarmUnits)
			{
				if (IsValid(SwarmUnit))
				{
					SwarmUnit->Destroy();
				}
			}
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': SpendGold(%d) refused at confirm for '%s' — %d spawned unit(s) unwound, staying in placement mode."),
				*GetNameSafe(this), PendingCost, *PendingCardID.ToString(), SwarmUnits.Num());
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
			return;
		}

		// representative actor for the shared success log below (all copies are one play)
		Spawned = SwarmUnits[0];
	}

	// M2 ruling: the card leaves the hand at CONFIRM — only now, with gold spent
	// and the actor committed, does the hand slot move to the discard pile and
	// redraw (§3.4). INDEX_NONE on the M1 paths (WBP_HUD Footman button / key-1
	// empty-hand fallback), which never touch the hand. A false return is
	// impossible while nothing else mutates the hand mid-placement (PlayHandSlot
	// and DiscardHandSlot both refuse during placement mode) — logged loudly as
	// a regression tripwire, and the actor/gold outcome above stands either way.
	if (PendingHandSlot != INDEX_NONE && DeckComponent)
	{
		if (!DeckComponent->ConfirmPlayFromHand(PendingHandSlot))
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': ConfirmPlayFromHand(%d) refused at confirm for '%s' — hand mutated mid-placement (should be impossible; see TASK-023 handoff)."),
				*GetNameSafe(this), PendingHandSlot, *PendingCardID.ToString());
		}
	}

	// Spawned is non-null on every path that reaches here (both branches either
	// assigned it or returned early)
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': played card '%s' for %d gold — spawned '%s' at (%.0f, %.0f, %.0f)."),
		*GetNameSafe(this), *PendingCardID.ToString(), PendingCost, *GetNameSafe(Spawned),
		Spawned->GetActorLocation().X, Spawned->GetActorLocation().Y, Spawned->GetActorLocation().Z);

	// confirm exit path — also releases the melee suppression (qa-note)
	ExitPlacementMode();
}

void ASiegePlayerController::ConfirmStackUpgrade(ASiegePlayerState& SiegeState)
{
	// ⛔ RE-VALIDATED AT CONFIRM, AND THIS ONE IS NOT BELT-AND-BRACES. A building can die
	// between the frame that painted the ghost BLUE and the frame the player clicks — a siege
	// is happening around it — which is exactly why PlacementUpgradeTarget is weak. The
	// refusal STAYS in placement mode: another building of the same card is still a legal
	// target, so a different click can succeed.
	// ⭐ THE RIDER, STACK-§9(2): this branch reused StackNotStackableRefusalText() until
	// 2026-09-03 and the sentence was FALSE — the building it names can be stacked perfectly
	// well; it simply is not there any more. ⚖️ A message that is wrong on a rare path is how
	// the NEXT playtest report gets misdiagnosed, which is the whole reason this batch exists.
	ABuilding* const Target = PlacementUpgradeTarget.Get();
	if (!IsValid(Target) || Target->IsBuildingDestroyed())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': upgrade click refused for '%s' — the hovered building is gone or destroyed (it died between the ghost frame and the click)."),
			*GetNameSafe(this), *PendingCardID.ToString());
		RefuseCardPlay(PendingCardID, StackTargetGoneRefusalText());
		return;
	}

	// ⛔⛔ THE STACK (Z) PREDICATE, ASKED A SECOND TIME — the SECOND of the three gates
	// STACK-§8 cl. 3 repointed. UpdatePlacementGhost asked it a frame ago and
	// ABuilding::ApplyStackUpgrade asks it a third time inside itself — three independent
	// mechanisms, on purpose. ⛔ Structural (a virtual on ABuilding), ⛔ never a CardID string
	// compare (an automatic QA fail by STACK-§2).
	//
	// ⛔⛔ `CanStackHeight()`, ⛔ NOT `CanScaleFootprint()` — the wheel's predicate answering
	// this question is the shipped defect Jonathan filmed, and asking it here again would
	// reintroduce the refusal at the click even with the hover gate fixed.
	if (!Target->CanStackHeight())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			// ⛔⛔ THE PREDICATE IS NAMED ⛔ WITHOUT ITS PARENTHESES, AND THAT IS ⛔ NOT A TYPO —
			// it is `SC-§41` used the way `SC-§41` is meant to be used. SiegeBuildingStackTest's
			// gate census counts the CALL SHAPE `CanStackHeight(` on code lines of this
			// function, and a UE_LOG literal is a CODE line. ⇒ a message quoting the call
			// WITH its parens would satisfy the gate ⛔ ALL BY ITSELF: delete the consult four
			// lines up and the census would still read 1 and still report GREEN.
			// ⚖️ *A gate cannot honestly scan a file whose MESSAGES quote the thing it counts* —
			// and the open paren is exactly the discriminator that lets it, so the diagnostic
			// value of naming the predicate is kept and the false hit is not.
			// ⛔ The wheel's predicate is not named here at all; that contrast lives in the
			// comment above, where the scanner correctly ignores it.
			TEXT("ASiegePlayerController '%s': upgrade click refused for '%s' — the target's CanStackHeight predicate is false (STACK-§8: the HEIGHT gate, ⛔ not the placement wheel's)."),
			*GetNameSafe(this), *PendingCardID.ToString());
		RefuseCardPlay(PendingCardID, StackNotStackableRefusalText());
		return;
	}

	// GOLD IS THE LAST GATE BEFORE THE COMMIT — the shipped building path's discipline,
	// generalised to a mutation. J-2: the price is the card's OWN DT_Cards Cost (PendingCost),
	// ⛔ never a literal, so retuning the card retunes the upgrade with no code change. The
	// ghost already refused this case in RED (J-5), so reaching it means gold changed between
	// the frame and the click; the refusal is the same shipped line either way.
	if (!SiegeState.SpendGold(PendingCost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': SpendGold(%d) refused at upgrade confirm for '%s' — staying in placement mode."),
			*GetNameSafe(this), PendingCost, *PendingCardID.ToString());
		RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// ⭐⭐ THE ONE CALL. ⛔ NO ARITHMETIC HAPPENS IN THIS FILE: the additive height series, the
	// multiplicative health series, the ×5 cap, the MaxHP/CurrentHP delta rule (J-10 — grant
	// the new hit points, ⛔ never repair the old damage) and the StackUpgradeCount increment
	// are ALL inside ApplyStackUpgrade (TASK-812), because MaxHP, CurrentHP and
	// StackUpgradeCount are PRIVATE on ABuilding and are meant to stay that way.
	// ⛔ M8 (STACK-§7): that mutator refuses on !HasAuthority(), so the client can never author
	// the count — this controller asks, the building decides.
	const int32 UpgradesBefore = Target->GetStackUpgradeCount();
	if (!Target->ApplyStackUpgrade())
	{
		// ⛔ NET-ZERO REFUND (§3.0), because the mutator is the one thing here that cannot be
		// un-applied: gold moved and nothing grew, so the gold comes back. Reaching this means
		// a refusal INSIDE ApplyStackUpgrade that the three checks above do not cover — today
		// that is the authority guard alone, and a Warning is the right volume for it.
		// ⭐ THE RIDER, STACK-§9(2), second site: this branch also reused
		// StackNotStackableRefusalText() and that sentence was FALSE too — the building is
		// stackable, the SERVER refused the mutation. The player did nothing wrong and cannot
		// act on "that building cannot be stacked"; the gold is already back.
		SiegeState.AddGold(PendingCost);
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': ApplyStackUpgrade refused on '%s' after the spend — %d gold REFUNDED (net-zero, §3.0). The building is unchanged; check the STACK-§7 authority guard."),
			*GetNameSafe(this), *GetNameSafe(Target), PendingCost);
		RefuseCardPlay(PendingCardID, StackUpgradeFailedRefusalText());
		return;
	}

	const int32 UpgradesAfter = Target->GetStackUpgradeCount();

	// ⭐ J-6 — THE CAP IS MADE VISIBLE, AND ⛔ ONLY WHEN IT ACTUALLY BIT. His own sentence:
	// "at some point if they keep upgrading it would only upgrade health by 1.5 times and not
	// height." A silent behaviour change at ×5 is indistinguishable from a bug, so the click
	// that buys health-only says so — ONCE, HERE, on the click, ⛔ never per frame from the
	// ghost (which would spam the HUD for as long as the cursor rested on a maxed tower).
	// ⭐ The test is DERIVED FROM THE SERIES, ⛔ not from the number 5: the height did not move,
	// therefore the cap bit. Retune MaxStackHeightMultiplier and this still tells the truth.
	// ⭐⭐ AND THE CEILING COMES FROM **THE TARGET**, ⛔ not from ABuilding's CDO (STACK-§10
	// cl. 2): it is per class now, so a tower with a lower ceiling than the base class must see
	// this note fire at ITS cap and not at the base's. ⛔ Reading it off the CDO here would put
	// the note on the wrong upgrade for every subclass that sets its own — silently, and only
	// for the one building whose ceiling anybody cared about.
	const int32 TargetHeightCap = Target->GetMaxStackHeightMultiplier();
	if (ABuilding::StackHeightMultiplier(UpgradesAfter, TargetHeightCap) <= ABuilding::StackHeightMultiplier(UpgradesBefore, TargetHeightCap))
	{
		BroadcastRefusal(StackHeightCapNoticeText());
	}

	// The card leaves the hand at CONFIRM — the shipped M2 law (§3.4), byte-identical to the
	// spawn path's block, because an upgrade IS a play of that card: gold moved, the board
	// changed, so the card is spent. INDEX_NONE on the M1 direct-entry paths, which never
	// touch the hand.
	if (PendingHandSlot != INDEX_NONE && DeckComponent)
	{
		if (!DeckComponent->ConfirmPlayFromHand(PendingHandSlot))
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': ConfirmPlayFromHand(%d) refused at upgrade confirm for '%s' — hand mutated mid-placement (should be impossible; see TASK-023 handoff)."),
				*GetNameSafe(this), PendingHandSlot, *PendingCardID.ToString());
		}
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': UPGRADED '%s' with card '%s' for %d gold — stack %d -> %d (height ×%.2f, health ×%.3f of the original)."),
		*GetNameSafe(this), *GetNameSafe(Target), *PendingCardID.ToString(), PendingCost,
		UpgradesBefore, UpgradesAfter,
		ABuilding::StackHeightMultiplier(UpgradesAfter, TargetHeightCap), ABuilding::StackHealthMultiplier(UpgradesAfter));

	// same exit as the spawn confirm — releases the melee suppression (qa-note)
	ExitPlacementMode();
}

void ASiegePlayerController::UpdatePlacementGhost()
{
	FHitResult Hit;
	const bool bGroundHit = TraceCursorToGround(Hit);
	if (bGroundHit)
	{
		PlacementLocation = Hit.ImpactPoint;
	}

	// ⭐⭐ TASK-815 (STACK-§4/§6) — THE GHOST'S SIZE IS ESTABLISHED BEFORE ANYTHING
	// MEASURES IT, AND THE ORDER IS THE WHOLE POINT OF PUTTING IT HERE.
	//
	// ⛔ THIS IS THE ⛔ ONE PLACE THE GHOST IS SCALED. ApplyPlacementFootprintWheel (which
	// ran moments ago, in this same frame, from the branch above) writes only the NUMBER;
	// the transform is written here, unconditionally, every frame — so the ghost can ⛔
	// never disagree with the member the confirm will spawn from, not even on the first
	// frame of a session and not even if PlacementFootprintMin is retuned off 1.0.
	//
	// ⚠️ ABOVE THE FOOTPRINT READ, ⛔ NOT BESIDE SetActorLocation AT THE BOTTOM: the read
	// below takes the ghost's ⭐ SCALED bounds (STACK-§6). Written after it, this frame's
	// click would be validated against LAST frame's size — a ×1.0 clearance for a ×1.5
	// building, which is precisely the defect TOWER-§7 exists to close.
	//
	// ⛔ SetActorScale3D writes the AStaticMeshActor's ROOT (its mesh component's) relative
	// scale, which is exactly the transform CalcBounds consults below. It is cheap and
	// in-kind: this function already writes the ghost's visibility and location every frame.
	if (GhostActor)
	{
		GhostActor->SetActorScale3D(MakePlacementFootprintScale3D(PlacementFootprintScale));
	}

	// Placement validity v4 (GDD §3.5 TASK-030 + GDD §5 M4.5 TASK-093 +
	// W1-PREP additions 3 TASK-261 + Castle 3× HOLLOW TASK-349), in cost order:
	// (1) ground hit inside the player's spawn box — a 2D square around the owned
	//     Castle_Blue with half-extent SpawnBoxHalfExtent (7380 since TASK-557,
	//     WR-§2 row 1 — the 9× castle; 2460 was the TASK-349 3× value. It covers
	//     the castle's walkable interior at whichever scale is live, which is the
	//     point of the paired tunable) — OR inside a Blue-owned capture
	//     zone (TASK-261; REPLACES the retired X<=PlacementMaxX half-gate.
	//     Neutral/Red mid zone => not placeable there);
	// (2) the point projects onto the navmesh within NavProjectionExtent —
	//     closes the M1 "castle roof is placeable" carry-over (roof hits sit far
	//     above any navmesh) AND is the whole spawn-inside truth: the hollow
	//     castle's interior floor is navmesh'd, so interior points validate by
	//     construction. The old rule (3) — the castle plinth keep-out — was
	//     RETIRED here by TASK-349 (plinth-retirement law): it would refuse
	//     exactly the interior placement Jonathan asked for. Placement truth =
	//     nav projection + collision + the existing clearances below;
	// (4) Building cards only (M4.5 ruling 7): ground slope <=
	//     MaxPlacementSlopeDegrees, measured by straight-down traces
	//     (fail-closed on a miss at the CENTRE) — hill flanks refuse, crowns
	//     (<=10°) pass. ⭐ FOOTPRINT-AWARE since TASK-871: the candidate point AND
	//     its four footprint corners are sampled, so a building whose centre sits
	//     on a crown can no longer OVERHANG a steep flank and pass. ⛔ The gate is
	//     tightened IN PLACE — it is not moved, and every rule below still runs
	//     after it (see the order note);
	// (5) Building cards only (M4.5 ruling 7, Fab amendment): >=
	//     ObstaclePlacementClearance (2D) from every "Obstacle"-tagged actor
	//     (trees AND rocks);
	// (6) Building cards only: >= EffectiveBuildingClearance(BuildingClearance,
	//     footprint radius) from the nearest other ABuilding (§3.5; castles are
	//     NOT buildings for this rule). FOOTPRINT-COMPOSED since TASK-735 — see
	//     (7);
	// (7) ⭐ Building cards only, TASK-735 (TOWER-§7 / T-6): no LIVE OWN-TEAM unit
	//     inside the footprint the building would occupy. This is the FIRST unit
	//     term this validation has ever had — its absence was the finding, and it
	//     is why a player could drop a structure on top of their own army with
	//     nothing refusing it.
	// The FIRST failing rule is recorded so the confirm click can name its
	// reason ("Too steep" / "Too close to obstacles" get their own messages).
	//
	// ⭐⛔ THE FOOTPRINT IS READ ONCE PER FRAME, HERE, AND FROM THE GHOST'S MESH —
	// ⛔ never from a literal (TOWER-§7). Reading it once also means the THREE gates
	// below cannot disagree about how big the building is. ⭐ TASK-871 added the
	// THIRD consumer (the slope probe) as a new CONSUMER of this one measurement,
	// ⛔ not as a second measurement of its own — a second definition of "how wide
	// is this building" is exactly how the ghost and the validated footprint drift.
	//
	// ⛔ DEGRADE-OPEN, AND THE THREE GATES DEGRADE DIFFERENTLY ON PURPOSE:
	//   • the building clearance takes the radius by VALUE and gets 0 on the
	//     degrade path ⇒ max(200, 0) = 200 = the shipped rule, byte-for-byte;
	//   • the SLOPE probe also takes it by VALUE and gets 0 ⇒ its sample count
	//     collapses to ONE and it is the shipped single straight-down trace at the
	//     cursor, byte-for-byte (TASK-871 spec (4): the degrade is KEPT);
	//   • the unit gate is SKIPPED entirely (bFootprintKnown) rather than run at
	//     radius 0 ⇒ a card whose ghost mesh is missing behaves exactly as it did
	//     before this task, with ONE warning and no new refusal.
	// (House null-safety law: a missing piece degrades open, never bricks
	// placement, and never refuses on an absent asset.)
	//
	// ⭐ ORDER IS DELIBERATE — THE NEW GATE GOES LAST, AND THAT IS THE OTHER HALF
	// OF THE NON-REGRESSION ARGUMENT: cheapest-first is preserved (one trace, then
	// three iterations), and because every pre-existing rule is evaluated BEFORE
	// the unit rule, first-failing-rule-wins guarantees that ⛔ no placement which
	// refuses today can change WHICH message it shows.
	//
	// ⛔ bPendingIsBuilding SHORT-CIRCUITS THE READ ITSELF, not just the gates: units and
	// miners are exempt from every footprint rule exactly as they are exempt from the slope
	// and obstacle gates (M4.5 ruling 7), so a UNIT card must not pay for a per-frame bounds
	// transform — and must not emit the degrade warning for a ghost mesh no footprint rule
	// would have consulted.
	float FootprintRadius = 0.f;
	const bool bFootprintKnown = bPendingIsBuilding && TryGetPlacementFootprintRadius(FootprintRadius);

	PlacementInvalidReason = EPlacementInvalidReason::Point;
	bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));
	if (bValid)
	{
		bValid = IsPointOnNavmesh(PlacementLocation);
	}
	if (bValid && bPendingIsBuilding && !IsGroundSlopePlaceable(PlacementLocation, FootprintRadius))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Slope;
	}
	if (bValid && bPendingIsBuilding && !HasObstacleClearance(PlacementLocation))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Obstacle;
	}
	if (bValid && bPendingIsBuilding && !HasBuildingClearance(PlacementLocation, FootprintRadius))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Clearance;
	}
	if (bValid && bPendingIsBuilding && bFootprintKnown && !HasUnitClearance(PlacementLocation, FootprintRadius))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Units;
	}
	if (bValid)
	{
		PlacementInvalidReason = EPlacementInvalidReason::None;
	}
	bPlacementValid = bValid;

	// ═══ ⭐⭐ TASK-813 — THE THIRD GHOST STATE (STACK-§0/§2/§5) ══════════════════════════
	//
	// Jonathan, verbatim: "if you hover directly on another tower, the outline instead
	// appears blue, which means you can place it there, and instead of placing a new tower,
	// it will instead double the height of the old one."
	//
	// ⭐ THE HOVER TARGET COSTS ⛔ NOTHING NEW: it is the SAME cursor trace the ghost already
	// runs. Buildings root a BlockAll VisualMesh (Building.h:43-51) so they answer the
	// Visibility trace, and the ghost's own collision is fully disabled — so Hit.GetActor()
	// IS the building under the cursor. ⛔ No second trace, ⛔ no overlap sweep, ⛔ no proximity
	// search. ACastle is class-disjoint from ABuilding, so the Cast also excludes the castle.
	//
	// ⛔⛔ THIS BLOCK OVERRIDES THE VERDICT ABOVE RATHER THAN JOINING THE GATE CHAIN, AND THAT
	// IS DELIBERATE ON TWO COUNTS:
	//   (1) CORRECTNESS — the five gates answer "may a NEW building stand at this point", and
	//       a blue click puts no new building anywhere. Standing on top of an existing
	//       building fails the §3.5 clearance gate by construction (the distance to it is ~0),
	//       so composing with them could only ever refuse the very state we are adding.
	//   (2) DIFF SIZE — ⛔ not one shipped gate line above is edited or re-indented. TASK-735
	//       asked for that explicitly and TASK-815 edits these same lines next.
	//
	// The five gates still RUN in the upgrade state. They are side-effect-free (traces and
	// iterations, one Verbose log) and cost exactly what they cost today, and letting them run
	// keeps this an additive block instead of a restructure of a shipped path.
	ABuilding* const HoveredBuilding = bGroundHit ? Cast<ABuilding>(Hit.GetActor()) : nullptr;
	const ASiegePlayerState* const OwnPlayerState = GetPlayerState<ASiegePlayerState>();

	// ⛔ NO PLAYER STATE ⇒ no team to compare and no gold to price against ⇒ None, i.e. today's
	// behaviour byte-for-byte. The house degrade-open law: a missing piece never invents a
	// state, and it certainly never paints one blue (HasUnitClearance degrades the same way
	// rather than guessing a side).
	PlacementUpgradeState = OwnPlayerState
		? ResolvePlacementUpgradeState(
			HoveredBuilding,
			OwnPlayerState->GetTeam(),
			PendingCardID,
			bPendingIsBuilding,
			OwnPlayerState->GetGold(),
			PendingCost) // J-2: the card's OWN DT_Cards Cost, read at EnterPlacementMode — ⛔ never a literal
		: EPlacementUpgradeState::None;

	// ⛔ CLEARED EVERY FRAME BEFORE IT CAN BE SET: a target survives exactly as long as the
	// state that named it, so a stale building can never be upgraded by a later click.
	PlacementUpgradeTarget = nullptr;

	switch (PlacementUpgradeState)
	{
	case EPlacementUpgradeState::Ready:
		// ⭐ BLUE. The click will upgrade, so it must not be refused by the point rules.
		PlacementUpgradeTarget = HoveredBuilding;
		bPlacementValid = true;
		PlacementInvalidReason = EPlacementInvalidReason::None;
		break;

	case EPlacementUpgradeState::NotStackable:
	case EPlacementUpgradeState::Unaffordable:
		// ⛔ RED — and with a reason of its OWN, so the click can say WHY. Overriding
		// whatever the point rules concluded is the point: "too close to another building"
		// is an actively wrong thing to tell a player who is hovering that building on
		// purpose (STACK-§2: a feature that quietly does nothing is a bug report waiting to
		// happen; one that says why is a design).
		bPlacementValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Upgrade;
		break;

	case EPlacementUpgradeState::None:
	default:
		// ⛔ Untouched. Green/red exactly as it has always been — including for an ENEMY
		// building (J-7), which stays RED through the SHIPPED clearance gate rather than
		// through a new refusal invented for it.
		break;
	}

	if (GhostActor)
	{
		GhostActor->SetActorHiddenInGame(!bGroundHit);
		if (bGroundHit)
		{
			// M4.5 ghost-projection law (TASK-093): PlacementLocation IS the
			// cursor trace's ImpactPoint, so the ghost's Z is the traced SURFACE
			// height under the cursor — on the flat floor, a 250-high hill crown,
			// or a flank alike (the terrain's Use-Complex-As-Simple collision
			// answers the Visibility trace with the real surface). Rotation is
			// deliberately untouched here: the ghost keeps its spawn-time
			// yaw-only rotation (GhostYawOffset) and stays upright — NO
			// alignment to the surface normal. SM_Footman-family origins are
			// feet-center (TASK-014), so the ghost sits on the ground point.
			GhostActor->SetActorLocation(PlacementLocation);
		}
	}

	if (GhostMID)
	{
		// TASK-012 contract: M_Ghost exposes the vector parameter "GhostColor";
		// opacity is handled inside the material
		static const FName GhostColorParamName(TEXT("GhostColor"));

		// ⭐ TASK-813 (STACK-§0): BLUE is a THIRD FLinearColor on the SAME parameter, set by
		// the SAME call. ⛔ No new material, ⛔ no new MID, ⛔ no second ghost actor, ⛔ no new
		// render path — STACK-§0 measured that this line already exists and that the blue
		// state is the cheapest part of the ask, not the expensive one.
		//
		// The ternary chain is ordered, not arbitrary: Ready implies bPlacementValid (the
		// switch above sets it), so asking the upgrade state FIRST is what stops an upgrade
		// hover from painting plain green. ⛔ At the height cap the colour does NOT change
		// (J-6) — the cap is a HUD note at confirm, never a silent recolour.
		const FLinearColor ActiveGhostColor =
			(PlacementUpgradeState == EPlacementUpgradeState::Ready)
				? UpgradeGhostColor
				: (bPlacementValid ? ValidGhostColor : InvalidGhostColor);
		GhostMID->SetVectorParameterValue(GhostColorParamName, ActiveGhostColor);
	}
}

void ASiegePlayerController::ApplyPlacementFootprintWheel()
{
	// ⛔⛔ THE EXCLUSION FIRST, AND IT IS THE SAME PREDICATE THE UPGRADE USES (STACK-§2,
	// spec (6)): a card whose class refuses footprint scaling gets an INERT wheel — the
	// scale is never written, so the ghost keeps its authored size and TryConfirmPlacement
	// spawns at PlacementFootprintMin. ⛔ Not a second check and ⛔ not a name compare: the
	// flag was resolved once, in EnterPlacementMode, from the card class's CDO.
	//
	// ⚖️ INERT RATHER THAN REFUSED, deliberately: the wheel is a continuous adjustment,
	// not a click, so a HUD line here would fire on every notch of an idle scroll. The
	// WatchTower's refusal already has a voice — the RED ghost plus "That building cannot
	// be stacked" on the click (TASK-813) — and this is the same exclusion speaking once.
	if (!bPendingCardCanScaleFootprint)
	{
		return;
	}

	// POLLED wheel (MARK-§4's law, unchanged: ⛔ NO new InputAction — the wheel is
	// globally unbound and stays INERT outside its three named consumers). ⭐ The +=/-=
	// shape is ApplyGroupPickWheel's, character for character, including the property
	// that BOTH directions in one frame cancel to no movement at all.
	int32 NotchDelta = 0;
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		++NotchDelta;
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		--NotchDelta;
	}
	if (NotchDelta == 0)
	{
		return; // no notch this frame (or both fired) — nothing to resize
	}

	const float NewScale = StepPlacementFootprintScale(
		PlacementFootprintScale, NotchDelta, PlacementFootprintWheelStep, PlacementFootprintMin, PlacementFootprintMax);
	if (NewScale == PlacementFootprintScale)
	{
		return; // pinned at a clamp — the ApplyGroupPickWheel early-out, same reason
	}
	PlacementFootprintScale = NewScale;

	// ⛔⛔ THIS FUNCTION WRITES THE NUMBER AND ⛔ NOTHING ELSE — it deliberately does ⛔ not
	// touch the ghost. UpdatePlacementGhost, which runs immediately after it in the SAME
	// frame, is the ⭐ ONE writer of the ghost's transform, and it applies this scale
	// BEFORE it measures the footprint. ⚖️ One writer is what makes "the ghost and the
	// spawned building agree" an INVARIANT rather than an event: a scale applied only when
	// a notch lands would silently disagree with the member on the very first frame of a
	// session if PlacementFootprintMin were ever retuned away from 1.0.

	// Verbose, and only on an actual change — this runs inside a per-frame branch, and the
	// two early-outs above mean a player who never scrolls prints nothing at all. The line
	// carries the tunables as well as the result so a playtest log says WHY a notch moved
	// the ghost as far as it did (⚠️ Verbose needs `Log LogGitClaudeUnrealTest Verbose`).
	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASiegePlayerController '%s': placement footprint wheel — '%s' now x%.2f (notch %d, step %.2f, range [%.2f, %.2f]; TASK-815 STACK-§4)."),
		*GetNameSafe(this), *PendingCardID.ToString(), PlacementFootprintScale, NotchDelta,
		PlacementFootprintWheelStep, PlacementFootprintMin, PlacementFootprintMax);
}

void ASiegePlayerController::SpawnPlacementGhost()
{
	UWorld* World = GetWorld();
	if (!World || GhostActor)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	// GhostYawOffset turns the raw SM_Footman to face +X / the Red side (TASK-014 facing note)
	GhostActor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator(0.f, GhostYawOffset, 0.f), SpawnParams);
	if (!GhostActor)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': failed to spawn the placement ghost — placement continues without a preview."),
			*GetNameSafe(this));
		return;
	}

	// movable + fully non-colliding: the ghost must never block the cursor trace,
	// pathing, or the unit spawn
	GhostActor->SetMobility(EComponentMobility::Movable);
	GhostActor->SetActorEnableCollision(false);
	GhostActor->SetActorHiddenInGame(true); // shown on the first ground hit

	UStaticMeshComponent* GhostMesh = GhostActor->GetStaticMeshComponent();
	if (!GhostMesh)
	{
		return;
	}
	GhostMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GhostMesh->SetGenerateOverlapEvents(false);
	GhostMesh->SetCanEverAffectNavigation(false);

	// per-card ghost mesh (TASK-030 closes the M1 TODO): /Game/Meshes/SM_<CardID>
	// with the engine-sphere fallback — every ghost asset is optional; a missing
	// mesh/material degrades the preview but MUST NOT block the placement logic
	// (TASK-007 spec; ResolveGhostMesh logs each fallback tier)
	if (UStaticMesh* Mesh = ResolveGhostMesh(PendingCardID))
	{
		GhostMesh->SetStaticMesh(Mesh);
	}

	if (UMaterialInterface* GhostMaterial = GhostMaterialAsset.LoadSynchronous())
	{
		GhostMID = UMaterialInstanceDynamic::Create(GhostMaterial, this);
		if (GhostMID)
		{
			for (int32 SlotIndex = 0; SlotIndex < GhostMesh->GetNumMaterials(); ++SlotIndex)
			{
				GhostMesh->SetMaterial(SlotIndex, GhostMID);
			}
		}
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': ghost material '%s' not found (TASK-012) — ghost shows the mesh's default material."),
			*GetNameSafe(this), *GhostMaterialAsset.ToString());
	}
}

void ASiegePlayerController::DestroyPlacementGhost()
{
	if (GhostActor)
	{
		GhostActor->Destroy();
		GhostActor = nullptr;
	}
	GhostMID = nullptr;
}

void ASiegePlayerController::EnterTargetingMode(FName CardID)
{
	// the EnterPlacementMode early-out pattern: post-match and mid-mode calls
	// are silent ignores (no broadcast), not player-facing refusals
	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — match has ended."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — already placing '%s' (mode mutual exclusion)."),
			*GetNameSafe(this), *CardID.ToString(), *PendingCardID.ToString());
		return;
	}

	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — already targeting '%s'."),
			*GetNameSafe(this), *CardID.ToString(), *TargetingCardID.ToString());
		return;
	}

	// third-mode mutual exclusion (TASK-344): a live group-order pick owns the
	// cursor/LMB — mirror of the already-placing/targeting ignores
	if (GroupPickStage != EGroupPickStage::None)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — a group-order pick is active."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	// FOURTH-mode mutual exclusion (TASK-440) — mirror of the placement clause
	// above and of CanOpenAssistantConsole's targeting clause.
	if (bAssistantConsoleOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — the assistant console is open."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	// FIFTH-mode mutual exclusion (TASK-563) — mirror of the placement clause and of
	// CanOpenWarMap's targeting clause.
	if (bWarMapOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') ignored — the war map is open."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	if (CardID.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode called with no CardID."),
			*GetNameSafe(this));
		return;
	}

	// stats come from the data table, NEVER from code (GDD §3.0)
	FString RowError;
	const FCardRow* Row = ResolveCardRow(CardID, RowError);
	if (!Row)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': cannot target spell '%s' — %s"),
			*GetNameSafe(this), *CardID.ToString(), *RowError);
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_NoData", "Card data unavailable"));
		return;
	}

	// defensive type gate: PlayHandSlot routes only Spell cards here — a direct
	// (BlueprintCallable) call with anything else is a caller regression,
	// refused with no state moved
	if (Row->CardType != ECardType::Spell)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': EnterTargetingMode('%s') refused — CardType %d is not Spell (route plays through PlayHandSlot)."),
			*GetNameSafe(this), *CardID.ToString(), static_cast<int32>(Row->CardType));
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_Unsupported", "Card not available"));
		return;
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': PlayerState is not an ASiegePlayerState (set on ASiegeGameMode, TASK-006) — cannot gate the spell cost."),
			*GetNameSafe(this));
		return;
	}

	// affordability gate (same call stack as PlayHandSlot's pre-check, so the
	// two can never disagree); gold is DEDUCTED only at LMB confirm (ruling 8)
	if (!SiegeState->CanAfford(Row->Cost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': spell '%s' refused — cost %d, gold %d."),
			*GetNameSafe(this), *CardID.ToString(), Row->Cost, SiegeState->GetGold());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// ⛔ AN UNAIMED SPELL NEVER TARGETS (ruling 7; `FOG-§10.1`): a direct call with
	// one reroutes to the instant resolve — hand-less on this path (INDEX_NONE
	// skips the draw step; PlayHandSlot routes hand plays before ever reaching
	// here). Placed BEFORE the hero-dead gate so BOTH entries behave alike:
	// instants are not hero-gated (the Masons/instant-play precedent).
	//
	// ⛔⛔ SITE 2 OF 2 OF THE SAME BLACKLIST, REPAIRED IN THE SAME DIFF AS SITE 1
	// (TASK-1018). This read `if (Row->SpellEffect == ESpellEffect::GoldSteal)` —
	// a SECOND copy of PlayHandSlot's routing question, which is why fixing only
	// the other one would have left a card that is instant from the hand and
	// targeted from a direct call. ⛔ It asks the ONE predicate now, exactly as its
	// sibling does; ⛔ do not re-spell it here.
	//
	// ✅ RULED (board TASK-1018 item (2b)): the hero-dead gate below applies to
	// TARGETING MODE, so once `Fog`/`BrightSun` take this early return it never
	// applies to them and AFogVolume's DELIBERATE no-living-hero degradation
	// governs instead (`J-F15` samples height at cast; a dead hero has no height,
	// so the window is the BASE one). ⛔ That is the intended behaviour, not a
	// side effect: `BrightSun` is playable with a dead hero for the base window.
	if (!USpellLibrary::SpellRequiresAiming(*Row))
	{
		ResolveSpellInstant(INDEX_NONE, CardID, *Row, *SiegeState);
		return;
	}

	// hero-dead refusal (flagged decision): the spec is silent, mirrored from
	// EnterPlacementMode — targeting owns the LMB exactly like placement, and
	// the card-play-while-dead rule should not differ between the sibling modes
	AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
	if (Hero && Hero->IsDead())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': spell '%s' refused — hero is dead (respawn pending, TASK-006)."),
			*GetNameSafe(this), *CardID.ToString());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_HeroDead", "Hero is down"));
		return;
	}

	bInTargetingMode = true;
	bTargetingSurfaceValid = false;
	TargetingLocation = FVector::ZeroVector;
	TargetingCardID = CardID;
	TargetingCost = Row->Cost;
	// row SNAPSHOT for the confirm-time resolver call — the placement
	// scalar-snapshot pattern (PendingCost/PendingCardType) generalized, because
	// USpellLibrary::ResolveSpell consumes the whole row (and must not chase a
	// table pointer across a mid-mode reimport)
	TargetingRow = *Row;
	// TargetingHandSlot deliberately NOT written here: PlayHandSlot records it
	// just before this call (the PendingHandSlot pattern); direct entries leave
	// it INDEX_NONE — no hand interaction (the M1 hand-less placement mirror)

	// suppress hero melee while targeting owns the LMB (TASK-003 API) — the
	// confirm click must not also swing; released on EVERY exit path
	TargetingHero = Hero;
	if (Hero)
	{
		Hero->SetMeleeSuppressed(true);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': entering targeting mode without an AHeroCharacter pawn — no melee to suppress."),
			*GetNameSafe(this));
	}

	ApplyCursorInputState();
	SpawnSpellReticle();
	UpdateSpellReticle();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': targeting mode entered for spell '%s' (cost %d, GDD §3.11)."),
		*GetNameSafe(this), *CardID.ToString(), TargetingCost);
}

void ASiegePlayerController::ExitTargetingMode()
{
	// the ExitPlacementMode law (qa/TASK-003-report.md warning 2): release the
	// melee suppression BEFORE any early-out. Every exit path — confirm,
	// resolver-false confirm exit, cancel (action or polled RMB/Esc), match end,
	// hero death, unpossess, match reset, EndPlay — funnels through here.
	// TargetingHero is a SEPARATE record from PlacementHero, so a defensive
	// ExitPlacementMode call can never strand a live targeting suppression
	// (and SetMeleeSuppressed(false) is an idempotent flag write).
	if (IsValid(TargetingHero))
	{
		TargetingHero->SetMeleeSuppressed(false);
	}
	TargetingHero = nullptr;

	if (!bInTargetingMode)
	{
		return;
	}

	bInTargetingMode = false;
	bTargetingSurfaceValid = false;
	TargetingLocation = FVector::ZeroVector;
	TargetingCardID = NAME_None;
	TargetingCost = 0;
	TargetingRow = FCardRow();
	TargetingHandSlot = INDEX_NONE; // hand plays consume it in TryConfirmSpellTarget BEFORE this exit

	DestroySpellReticle();

	// restores game-only free-look — unless IA_UICursor is still held, in which
	// case the cursor stays up for the HUD (the cursor owners compose)
	ApplyCursorInputState();
}

void ASiegePlayerController::TryConfirmSpellTarget()
{
	// AIM PASS (TASK-236, CONVENTIONS "Spell delivery overhaul (2026-07-21)"):
	// HeroLine spells (Fireball/FrostNova) need only an AIM DIRECTION at
	// confirm — the reticle stays as the aim indicator, but a surface under
	// the cursor is no longer REQUIRED for them. GroundCircle spells keep the
	// M5 confirm gate byte-for-byte.
	const bool bLineSpell = USpellLibrary::IsLineDeliverySpell(TargetingRow);

	if (!bTargetingSurfaceValid)
	{
		if (!bLineSpell)
		{
			// no surface under the cursor (sky / outside the world): refuse, spend
			// NOTHING, STAY in mode — a different point can succeed (the placement
			// invalid-click law). This is the ONLY positional refusal in targeting
			// mode: circle spells land ANYWHERE a surface answers the trace — enemy
			// half included, no navmesh requirement (M5 ruling 8, §3.5).
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': spell confirm refused for '%s' — no surface under the cursor."),
				*GetNameSafe(this), *TargetingCardID.ToString());
			RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoTarget", "No target under cursor"));
			return;
		}

		// line spell with the cursor off every surface (sky): synthesize the
		// AIM-POINT from the deprojected cursor ray, FLATTENED horizontal and
		// anchored at the hero — the resolver only reads the DIRECTION
		// origin→aim-point, so the 1000 uu reach of the synthetic point is
		// arbitrary. A failed deproject / absent hero / near-vertical ray
		// (no horizontal component) refuses FREE and STAYS in mode — the
		// trace-miss law generalized to "no aim direction".
		FVector RayOrigin = FVector::ZeroVector;
		FVector RayDirection = FVector::ZeroVector;
		if (!IsValid(TargetingHero) || !DeprojectMousePositionToWorld(RayOrigin, RayDirection))
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': line-spell confirm refused for '%s' — no hero/deproject to derive an aim direction from."),
				*GetNameSafe(this), *TargetingCardID.ToString());
			RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoAim", "No aim direction"));
			return;
		}
		const FVector FlatAimDirection = RayDirection.GetSafeNormal2D();
		if (FlatAimDirection.IsNearlyZero())
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': line-spell confirm refused for '%s' — cursor ray is vertical (no horizontal aim)."),
				*GetNameSafe(this), *TargetingCardID.ToString());
			RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoAim", "No aim direction"));
			return;
		}
		TargetingLocation = TargetingHero->GetActorLocation() + FlatAimDirection * 1000.f;
	}
	else if (bLineSpell && IsValid(TargetingHero))
	{
		// degenerate-aim pre-check (TASK-236): a reticle sitting ON the hero
		// (zero horizontal offset) cannot make a direction. Refused FREE,
		// STAYING in mode — a position-DEPENDENT miss belongs here, not in the
		// resolver (whose refusals are position-independent by contract and
		// exit the mode with a refund).
		const FVector ToAim = TargetingLocation - TargetingHero->GetActorLocation();
		if (ToAim.SizeSquared2D() < 1.f)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': line-spell confirm refused for '%s' — reticle has no horizontal offset from the hero."),
				*GetNameSafe(this), *TargetingCardID.ToString());
			RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoAim", "No aim direction"));
			return;
		}
	}

	ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': no ASiegePlayerState at spell confirm — cannot spend gold, staying in targeting mode."),
			*GetNameSafe(this));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// caster team mirrors the placement confirm resolution: the local player is
	// always Blue (CONVENTIONS team contract), taken from the hero when we have
	// one so the two can never disagree
	const ETeamId CasterTeam = IsValid(TargetingHero) ? TargetingHero->GetTeamId() : ETeamId::Blue;

	// M5 confirm law (ruling 8): DEDUCT THEN RESOLVE. SpendGold cannot actually
	// fail after the entry-time CanAfford (in-mode plays/discards are refused;
	// income only adds) — the branch is the same defensive net-zero guard the
	// placement confirm carries.
	if (!SiegeState->SpendGold(TargetingCost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': SpendGold(%d) refused at spell confirm for '%s' — staying in targeting mode."),
			*GetNameSafe(this), TargetingCost, *TargetingCardID.ToString());
		RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// THE resolver (M5 ruling 1 — pinned entry, TASK-098; shared with the bot).
	// False means the spell did NOT resolve and no world state changed: a bad
	// row/world/state, NEVER a positional miss — a zero-target cast is a
	// SUCCESSFUL resolve per the SpellLibrary contract (spent like a wasted
	// Fireball), so no refund path exists for "hit nothing".
	// TASK-236 call-site flag: for HeroLine spells (Fireball/FrostNova),
	// TargetingLocation is the AIM-POINT — the resolver derives the origin
	// (this player's hero) and fires the line origin→aim-point, flattened
	// horizontal. GroundCircle spells keep it as the impact center.
	if (!USpellLibrary::ResolveSpell(World, TargetingCardID, TargetingRow, CasterTeam, TargetingLocation))
	{
		// FULL refund (§3.0 net-zero law) through the choke-pointed gold API,
		// then EXIT with the card still in hand: resolver refusals are
		// position-independent by contract, so nothing a different click could
		// fix — the missing-BP-class placement precedent (flagged decision).
		// The >0 guard only skips the no-op refund of a 0-cost card (AddGold
		// refuses non-positive grants with a log) — net-zero holds either way.
		if (TargetingCost > 0)
		{
			SiegeState->AddGold(TargetingCost);
		}
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': ResolveSpell('%s') refused at (%.0f, %.0f, %.0f) — %d gold refunded, card kept, exiting targeting mode (the SpellLibrary log names the cause)."),
			*GetNameSafe(this), *TargetingCardID.ToString(), TargetingLocation.X, TargetingLocation.Y, TargetingLocation.Z, TargetingCost);
		RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_SpellFizzled", "Spell fizzled"));
		ExitTargetingMode();
		return;
	}

	// §6 spell-cast audio (TASK-179): the spell RESOLVED (past the refusal above) — a
	// world one-shot, null-safe until S_SpellCast lands (TASK-180). TASK-236: a LINE
	// spell's cast sound plays at the HERO (the muzzle — the aim-point can be
	// anywhere on the map and would be inaudible); circle spells keep the reticle
	// point byte-for-byte.
	const FVector CastSoundPoint = (bLineSpell && IsValid(TargetingHero))
		? TargetingHero->GetActorLocation()
		: TargetingLocation;
	USiegeFeedbackLibrary::PlayWorldSound(this, SpellCastSoundPath, CastSoundPoint);

	// M2 law: the card leaves the hand at CONFIRM — only now, with gold spent
	// and the spell resolved, does the slot move to discard and redraw (§3.4).
	// INDEX_NONE = a direct hand-less entry. The false return is the same
	// regression tripwire as placement: in-mode plays/discards are refused, so
	// the hand cannot mutate mid-targeting.
	if (TargetingHandSlot != INDEX_NONE && DeckComponent)
	{
		if (!DeckComponent->ConfirmPlayFromHand(TargetingHandSlot))
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': ConfirmPlayFromHand(%d) refused at spell confirm for '%s' — hand mutated mid-targeting (should be impossible)."),
				*GetNameSafe(this), TargetingHandSlot, *TargetingCardID.ToString());
		}
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': cast spell '%s' for %d gold at (%.0f, %.0f, %.0f) (GDD §3.11)."),
		*GetNameSafe(this), *TargetingCardID.ToString(), TargetingCost,
		TargetingLocation.X, TargetingLocation.Y, TargetingLocation.Z);

	// confirm exit path — also releases the melee suppression (qa-note)
	ExitTargetingMode();
}

void ASiegePlayerController::UpdateSpellReticle()
{
	// REUSED cursor trace (TASK-093 / its QA report: the enemy-half restriction
	// lives in UpdatePlacementGhost, NOT in this helper — targeting inherits
	// nothing it must undo). The reticle point is the trace's ImpactPoint: the
	// SURFACE under the cursor — flat floor, hill crown, or flank alike (M4.5
	// terrain carry-in LAW: never the Z=0 plane). Deliberately NO half check,
	// NO navmesh projection, NO slope/obstacle/clearance gates: spells land
	// anywhere (M5 ruling 8, §3.5).
	FHitResult Hit;
	const bool bSurfaceHit = TraceCursorToGround(Hit);
	if (bSurfaceHit)
	{
		TargetingLocation = Hit.ImpactPoint;
	}
	bTargetingSurfaceValid = bSurfaceHit;

	if (SpellReticleActor)
	{
		// hidden while the cursor is off every surface (sky) — the confirm click
		// refuses on the same flag, so what the player sees is what the click does
		SpellReticleActor->SetActorHiddenInGame(!bSurfaceHit);
		if (bSurfaceHit)
		{
			// position only: the decal projects straight down (spawn-time
			// rotation) and drapes whatever surface it reaches — no normal
			// alignment, no rotation updates
			SpellReticleActor->SetActorLocation(TargetingLocation);
		}
	}
}

void ASiegePlayerController::SpawnSpellReticle()
{
	UWorld* World = GetWorld();
	if (!World || SpellReticleActor)
	{
		return;
	}

	// null-safe reticle material (M5 ruling 8): missing ⇒ NO reticle actor at
	// all — targeting still works on the OS cursor alone (the invisible-ghost
	// degradation precedent), logged ONCE per controller (the latch)
	UMaterialInterface* ReticleMaterial = SpellReticleMaterialAsset.LoadSynchronous();
	if (!ReticleMaterial)
	{
		if (!bWarnedNoReticleMaterial)
		{
			bWarnedNoReticleMaterial = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': spell reticle material '%s' not found (built in TASK-108) — targeting continues without a reticle visual."),
				*GetNameSafe(this), *SpellReticleMaterialAsset.ToString());
		}
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	// Spawn at IDENTITY rotation: ADecalActor's constructor already gives its
	// root decal component relative pitch -90 (DecalActor.cpp:30), and UE 5.8's
	// PostSpawnInitialize COMPOSES root ∘ spawn transform (MultiplyWithRoot
	// default, Actor.cpp:4310-4324) — it does NOT stomp it. Spawning with -90
	// here would compose to -180 and lay the projection axis horizontal
	// (qa/TASK-100 BLOCKER-1). Identity composes to the CDO's own -90.
	SpellReticleActor = World->SpawnActor<ADecalActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!SpellReticleActor)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': failed to spawn the spell reticle decal — targeting continues without a reticle visual."),
			*GetNameSafe(this));
		return;
	}

	// pitch -90° points the decal's local X (its projection axis) straight DOWN.
	// SetActorRotation is ABSOLUTE world rotation — immune to CDO/spawn transform
	// composition — so the downward projection is explicit and composition-proof.
	// The decal drapes whatever surface lies under the reticle point (hill crowns
	// and flanks included): the M4.5 surface-projection law in visual form.
	// Decals carry NO collision, so the reticle can never block the cursor trace.
	SpellReticleActor->SetActorRotation(FRotator(-90.f, 0.f, 0.f));
	SpellReticleActor->SetActorHiddenInGame(true); // shown on the first surface hit

	if (UDecalComponent* ReticleDecal = SpellReticleActor->GetDecal())
	{
		ReticleDecal->SetDecalMaterial(ReticleMaterial);

		// footprint = the spell's OWN AoERadius (data-driven, GDD §3.0 —
		// Fireball 300 / FrostNova 350 / Lightning 700 / BattleCry 400) so the
		// ring shows the true blast area; a radius-less spell falls back to
		// SpellReticleDefaultRadius. X (the projection half-depth) is 500 —
		// bracketing the M4.5 max terrain height (250) exactly like the ±500
		// slope-trace window, so the decal reaches the surface on every hill.
		const float ReticleRadius = (TargetingRow.AoERadius > 0.f) ? TargetingRow.AoERadius : SpellReticleDefaultRadius;
		ReticleDecal->DecalSize = FVector(500.f, ReticleRadius, ReticleRadius);
		ReticleDecal->MarkRenderStateDirty();
	}
}

void ASiegePlayerController::DestroySpellReticle()
{
	if (SpellReticleActor)
	{
		SpellReticleActor->Destroy();
		SpellReticleActor = nullptr;
	}
}

void ASiegePlayerController::BeginGroupPick(ESiegeGroupCommandType Type)
{
	// M8 D5 observer lockout (TASK-356 doc §4.1): group orders steal SERVER
	// units into controller-local groups — a client-side pick would build state
	// no unit can read. Locking the entry seals all three stages + the confirm
	// (doc §4.3#8; the P2 migration moves the store server-side). Log-only.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Log,
			TEXT("ASiegePlayerController '%s': BeginGroupPick refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this));
		return;
	}

	// EnterPlacementMode / EnterTargetingMode early-out pattern: post-match and
	// mid-mode calls are silent ignores (no broadcast), not player-facing refusals.
	if (bMatchEnded)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — match has ended."),
			*GetNameSafe(this));
		return;
	}

	if (bInPlacementMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — already placing '%s' (mode mutual exclusion)."),
			*GetNameSafe(this), *PendingCardID.ToString());
		return;
	}

	if (bInTargetingMode)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — already targeting '%s' (mode mutual exclusion)."),
			*GetNameSafe(this), *TargetingCardID.ToString());
		return;
	}

	if (GroupPickStage != EGroupPickStage::None)
	{
		// re-pressing R/F mid-flow is a silent ignore — RMB/Esc is the cancel surface
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — a group-order pick is already active."),
			*GetNameSafe(this));
		return;
	}

	// FOURTH-mode mutual exclusion (TASK-440) — mirror of CanOpenAssistantConsole's
	// group-pick clause. ⚠️ CONVENTIONS §2 SURVIVES THIS: R / F / C are refused
	// ONLY while the console is open, i.e. only while the player is typing into it,
	// and a closed console leaves every key byte-identical.
	if (bAssistantConsoleOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — the assistant console is open."),
			*GetNameSafe(this));
		return;
	}

	// FIFTH-mode mutual exclusion (TASK-563) — mirror of CanOpenWarMap's group-pick
	// clause. ⚠️ Same §2 note as the console clause above: R / F / C are refused ONLY
	// while the map is actually up, and a closed map leaves every key byte-identical.
	if (bWarMapOpen)
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': BeginGroupPick ignored — the war map is open."),
			*GetNameSafe(this));
		return;
	}

	GroupPickStage = EGroupPickStage::Select;
	GroupPickType = Type;
	GroupPickRadius = GroupSelectRadiusDefault;
	bGroupPickSurfaceValid = false;
	GroupPickLocation = FVector::ZeroVector;
	GroupPickSelectedMembers.Reset();
	GroupPickPositionCenter = FVector::ZeroVector;
	GroupPickPositionRadius = 0.f;

	// suppress hero melee while the pick owns the LMB (TASK-003 API) — the stage
	// confirm clicks must not also swing; released on EVERY pick exit path
	// (CancelGroupPick, melee-release-before-early-out). GroupPickHero records
	// exactly whose suppression we must release (the PlacementHero/TargetingHero
	// pattern; its OWN record so a defensive ExitPlacement/ExitTargeting call can
	// never strand this one).
	AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
	GroupPickHero = Hero;
	if (Hero)
	{
		Hero->SetMeleeSuppressed(true);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': entering a group-order pick without an AHeroCharacter pawn — no melee to suppress."),
			*GetNameSafe(this));
	}

	ApplyCursorInputState();

	// stage-1 circle follows the cursor. Null-safe: missing M_SpellReticle ⇒ no
	// circle visuals at all — the whole pick still works off the trace (the
	// invisible-ghost degradation precedent).
	GroupPickActiveDecal = SpawnGroupCircleDecal(GroupPickRadius);
	UpdateGroupPickReticle();

	// FOLLOW gets its OWN stage-1 prompt: its circle is the whole command, so the
	// wording must not promise a second stage the flow will never open (TASK-395).
	if (Type == ESiegeGroupCommandType::Follow)
	{
		BroadcastCommandPrompt(FString(
			TEXT("FOLLOW: circle the units to follow you — scroll to resize, LMB confirm, RMB/Esc cancel")));
	}
	else
	{
		BroadcastCommandPrompt(FString::Printf(
			TEXT("%s: circle your units — scroll to resize, LMB confirm, RMB/Esc cancel"),
			GroupCommandTypeLabel(Type)));
	}
}

void ASiegePlayerController::UpdateGroupPickReticle()
{
	// REUSED cursor trace (the UpdateSpellReticle shape): the pick point is the
	// trace's ImpactPoint — the SURFACE under the cursor (flat floor, hill crown,
	// or flank alike; never the Z=0 plane). Writes the pick's OWN disjoint
	// scratch so it can never touch the placement/targeting state.
	FHitResult Hit;
	const bool bSurfaceHit = TraceCursorToGround(Hit);
	if (bSurfaceHit)
	{
		GroupPickLocation = Hit.ImpactPoint;
	}
	bGroupPickSurfaceValid = bSurfaceHit;

	if (GroupPickActiveDecal)
	{
		// hidden while the cursor is off every surface (sky) — the confirm click
		// refuses on the same flag, so what the player sees is what the click does
		GroupPickActiveDecal->SetActorHiddenInGame(!bSurfaceHit);
		if (bSurfaceHit)
		{
			GroupPickActiveDecal->SetActorLocation(GroupPickLocation);
		}
	}
}

void ASiegePlayerController::ApplyGroupPickWheel()
{
	// POLLED wheel resize (CONVENTIONS wheel law: NO new InputAction — the wheel
	// is verified globally unbound and must stay INERT outside the pick; this is
	// only ever called from the pick branch of PlayerTick). One scroll notch =
	// one GroupRadiusWheelStep on the ACTIVE circle, clamped to
	// [GroupRadiusMin, GroupRadiusMax].
	float NewRadius = GroupPickRadius;
	if (WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		NewRadius += GroupRadiusWheelStep;
	}
	if (WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		NewRadius -= GroupRadiusWheelStep;
	}
	NewRadius = FMath::Clamp(NewRadius, GroupRadiusMin, GroupRadiusMax);
	if (NewRadius == GroupPickRadius)
	{
		return; // no notch this frame (or pinned at a clamp) — nothing to resize
	}
	GroupPickRadius = NewRadius;

	// resize the active circle in place (null-safe — no decal when the material
	// is missing; the radius still changes and the confirm uses it)
	if (GroupPickActiveDecal)
	{
		if (UDecalComponent* CircleDecal = GroupPickActiveDecal->GetDecal())
		{
			CircleDecal->DecalSize = FVector(500.f, GroupPickRadius, GroupPickRadius);
			CircleDecal->MarkRenderStateDirty();
		}
	}
}

void ASiegePlayerController::ConfirmGroupPickStage()
{
	// trace-miss (cursor on the sky): refuse free and STAY in the stage — a
	// different point can succeed (the placement/targeting trace-miss precedent).
	// No group or stance state moves; the circle is already hidden this frame.
	if (!bGroupPickSurfaceValid)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': group-pick confirm ignored — cursor is not over a surface (staying in the pick)."),
			*GetNameSafe(this));
		return;
	}

	const TCHAR* TypeLabel = GroupCommandTypeLabel(GroupPickType);

	// FOLLOW is the ONE-STAGE type (TASK-395): it must confirm at Select and can
	// never be here in any other stage. This is a TRIPWIRE, not a control path —
	// the Select case below returns for Follow, so reaching either zone stage
	// would mean the flow was corrupted. Fail loud and tear down rather than
	// build a follow group carrying a bogus zone.
	if (GroupPickType == ESiegeGroupCommandType::Follow && GroupPickStage != EGroupPickStage::Select)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': FOLLOW pick reached a zone stage (%d) — impossible by construction (TASK-395 one-stage law). Cancelling the pick."),
			*GetNameSafe(this), static_cast<int32>(GroupPickStage));
		CancelGroupPick();
		return;
	}

	switch (GroupPickStage)
	{
	case EGroupPickStage::Select:
	{
		// stage 1 — SELECT: every eligible unit inside the circle (2D) at confirm
		// joins. EMPTY = refuse-and-stay + HUD reason (the law): a different circle
		// can succeed, so the stage survives the refusal.
		//
		// ⚠️ THE PREDICATE SPLITS BY TYPE (CONVENTIONS §3, TASK-396): Hold/Ambush
		// are ZONE orders and keep IsGroupCommandEligible() — whose meaning narrows
		// to exactly that; FOLLOW uses IsFollowCommandEligible(), which is wider
		// (it also admits the Support Cleric and the Miner) and still excludes
		// Siege (Ogre/Sapper) and every Red/bot unit. Profile is private on the
		// unit, so these two public predicates are the whole sanctioned surface.
		const bool bFollowPick = (GroupPickType == ESiegeGroupCommandType::Follow);
		GroupPickSelectedMembers.Reset();
		if (UWorld* World = GetWorld())
		{
			const float SelectRadiusSq = FMath::Square(GroupPickRadius);
			for (TActorIterator<ASummonedUnit> It(World); It; ++It)
			{
				ASummonedUnit* Unit = *It;
				if (!IsValid(Unit))
				{
					continue;
				}
				const bool bEligible = bFollowPick ? Unit->IsFollowCommandEligible() : Unit->IsGroupCommandEligible();
				if (bEligible
					&& FVector::DistSquared2D(Unit->GetActorLocation(), GroupPickLocation) <= SelectRadiusSq)
				{
					GroupPickSelectedMembers.Add(Unit);
				}
			}
		}

		if (GroupPickSelectedMembers.Num() == 0)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': %s stage-1 select refused — no eligible unit inside the circle (radius %.0f; staying in the pick)."),
				*GetNameSafe(this), TypeLabel, GroupPickRadius);
			BroadcastRefusal(NSLOCTEXT("Siegebound", "GroupPickRefused_NoUnits", "No units in the circle"));
			return;
		}

		// ⛔ THE ONE-STAGE FORK (TASK-395): FOLLOW terminates HERE. There is no
		// position zone and no attack zone to place — the anchor is the hero, not
		// a piece of ground — so the flow enrols and tears down instead of opening
		// stage 2. Hold/Ambush fall through to the unchanged 3-stage path below.
		if (bFollowPick)
		{
			ConfirmFollowPick();
			return;
		}

		// drop the SELECT circle in place (earlier circles stay visible through
		// the flow; this one dies at the final confirm/cancel — it never becomes
		// a marker), then open stage 2 with its own cursor-following circle.
		GroupPickSelectDecal = GroupPickActiveDecal;
		GroupPickActiveDecal = nullptr;
		GroupPickStage = EGroupPickStage::Position;
		GroupPickRadius = GroupPositionRadiusDefault;
		GroupPickActiveDecal = SpawnGroupCircleDecal(GroupPickRadius);
		UpdateGroupPickReticle();

		BroadcastCommandPrompt(FString::Printf(
			TEXT("%s: %d unit(s) selected — place the POSITION zone (scroll to resize)"),
			TypeLabel, GroupPickSelectedMembers.Num()));
		break;
	}

	case EGroupPickStage::Position:
	{
		// stage 2 — POSITION: record the station zone, drop its circle (it
		// becomes the group's position marker at the final confirm), open stage 3.
		GroupPickPositionCenter = GroupPickLocation;
		GroupPickPositionRadius = GroupPickRadius;

		GroupPickPositionDecal = GroupPickActiveDecal;
		GroupPickActiveDecal = nullptr;
		GroupPickStage = EGroupPickStage::AttackZone;
		GroupPickRadius = GroupAttackRadiusDefault;
		GroupPickActiveDecal = SpawnGroupCircleDecal(GroupPickRadius);
		UpdateGroupPickReticle();

		BroadcastCommandPrompt(FString::Printf(
			TEXT("%s: place the ATTACK zone (scroll to resize)"), TypeLabel));
		break;
	}

	case EGroupPickStage::AttackZone:
	{
		// stage 3 — FINAL confirm. ⚠️ TASK-440: THE GROUP-BUILDING BODY THAT USED
		// TO LIVE HERE IS NOW ASiegePlayerController::CreateUnitGroup — the SAME
		// work, in the SAME order, with the SAME log lines. What stayed behind is
		// only what is about THE PICK rather than about the group: the HUD refusal
		// wording, nulling the marker refs the group now owns, the teardown, and
		// the completion prompt.
		//
		// The dropped Position circle and this Attack circle are handed over as
		// the group's persistent markers (delivers the TASK-276-deferred hold
		// marker) — CreateUnitGroup takes ownership of both.
		const int32 NewGroupId = CreateUnitGroup(
			GroupPickType,
			GroupPickPositionCenter, GroupPickPositionRadius,
			GroupPickLocation,       GroupPickRadius,
			GroupPickSelectedMembers,
			GroupPickPositionDecal,  GroupPickActiveDecal);

		if (NewGroupId == INDEX_NONE)
		{
			// every selected unit died mid-flow: no group to form — refuse with a
			// HUD reason and tear the whole pick down. CreateUnitGroup already
			// logged the reason and transferred NOTHING, so the marker refs are
			// still ours and CancelGroupPick destroys all three circles.
			BroadcastRefusal(NSLOCTEXT("Siegebound", "GroupPickRefused_UnitsDied", "Selected units are gone"));
			CancelGroupPick();
			return;
		}

		// The count the player is told is the count that JOINED, not the count that
		// was circled — CreateUnitGroup drops members that died between selection
		// and confirm, exactly as the inline body did. Reading it back off the
		// formed group is safe and exact: the group was just appended, and the
		// prune it ran cannot reap a group that still has members nor drop a member
		// that passed the same alive test one statement earlier.
		int32 MemberCount = 0;
		if (const FSiegeUnitGroup* FormedGroup = FindUnitGroup(NewGroupId))
		{
			MemberCount = FormedGroup->Members.Num();
		}
		else
		{
			// unreachable by the argument above; logged rather than assumed so a
			// future change to PruneUnitGroups cannot make the prompt lie silently.
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': %s group %d vanished between creation and the completion prompt — reporting 0 unit(s)."),
				*GetNameSafe(this), TypeLabel, NewGroupId);
		}

		// the pick is DONE: the Position + Attack circles now belong to the group
		// as its persistent markers — null the scratch refs so the shared
		// CancelGroupPick teardown below leaves them standing and destroys only
		// the SELECT circle.
		GroupPickPositionDecal = nullptr;
		GroupPickActiveDecal = nullptr;

		CancelGroupPick();

		// the completion prompt AFTER the teardown's empty broadcast, so "set"
		// is what remains on the HUD
		BroadcastCommandPrompt(FString::Printf(TEXT("%s set: %d unit(s)"), TypeLabel, MemberCount));
		break;
	}

	case EGroupPickStage::None:
	default:
		// unreachable — the PlayerTick pick branch only runs while a stage is live
		break;
	}
}

int32 ASiegePlayerController::CreateUnitGroup(
	ESiegeGroupCommandType Type,
	const FVector& PositionCenter,
	float PositionRadius,
	const FVector& AttackCenter,
	float AttackRadius,
	const TArray<TWeakObjectPtr<ASummonedUnit>>& Members,
	ADecalActor* PositionMarker /* = nullptr */,
	ADecalActor* AttackMarker /* = nullptr */)
{
	// ⚠️ EXTRACTED VERBATIM FROM ConfirmGroupPickStage's stage-3 body (TASK-440).
	// The acceptance criterion for this function is PROVABLY ZERO BEHAVIOR CHANGE,
	// so every statement below is the shipped one with pick-scratch members
	// rewritten to the parameters that carry the identical values. Nothing was
	// retyped, tidied or "improved" on the way across.
	//
	// EXACTLY ONE STATEMENT CHANGED POSITION — the formation UE_LOG at the bottom,
	// which used to sit after two marker-ref writes that are now the caller's. It
	// is flagged in full at its own site rather than only here, because a reviewer
	// checking "was anything reordered?" should find the answer AT the statement.
	const TCHAR* TypeLabel = GroupCommandTypeLabel(Type);

	FSiegeUnitGroup NewGroup;

	// ⚠️ THE ID IS TAKEN HERE, BEFORE THE ALIVE-FILTER, AND THAT IS DELIBERATE:
	// the shipped body did exactly this, so a refused confirm BURNS an id. Ids are
	// never reused and nothing keys off them being contiguous, so burning one is
	// harmless — but moving this line below the filter would silently change which
	// id every subsequent group gets, which is a behavior change wearing a tidy-up's
	// clothes. It stays where it was.
	NewGroup.GroupId = NextUnitGroupId++;
	NewGroup.Type = Type;
	NewGroup.PositionCenter = PositionCenter;
	NewGroup.PositionRadius = PositionRadius;
	NewGroup.AttackCenter = AttackCenter;
	NewGroup.AttackRadius = AttackRadius;
	NewGroup.PositionMarkerDecal = PositionMarker;
	NewGroup.AttackMarkerDecal = AttackMarker;

	// re-filter the caller's capture: members may have died since they were picked
	for (const TWeakObjectPtr<ASummonedUnit>& Member : Members)
	{
		const ASummonedUnit* Unit = Member.Get();
		if (Unit && !Unit->IsUnitDead())
		{
			NewGroup.Members.Add(Member);
		}
	}

	if (NewGroup.Members.Num() == 0)
	{
		// nobody survived: no group to form. The CALLER owns the player-facing
		// message and the teardown — nothing has been transferred or mutated here,
		// so the caller still owns both marker decals and can destroy them.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': %s final confirm refused — every selected unit died during the pick."),
			*GetNameSafe(this), TypeLabel);
		return INDEX_NONE;
	}

	// STEAL (the re-selection law): a re-selected unit leaves its older group.
	// PruneUnitGroups below reaps any group this empties — markers included —
	// synchronously, so it never outlives the confirm that emptied it.
	for (const TWeakObjectPtr<ASummonedUnit>& Member : NewGroup.Members)
	{
		for (FSiegeUnitGroup& OldGroup : UnitGroups)
		{
			OldGroup.Members.Remove(Member);
		}
	}

	// Per-unit stations: deterministic golden-angle sunflower inside the
	// position circle — station i at radius R·√((i+0.5)/N), angle i·golden —
	// computed ONCE here and nav-projected (the SpawnUnitSwarm ring-projection
	// pattern), then PUSHED to the unit as a scalar offset (no arrays on
	// units). This kills the mill-at-one-point clustering (TASK-280/282).
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	const int32 MemberCount = NewGroup.Members.Num();
	const float StationExtentXY = FMath::Max(NewGroup.PositionRadius * 0.5f, 100.f);
	const FVector StationProjectExtent(StationExtentXY, StationExtentXY, 200.f);
	for (int32 Index = 0; Index < MemberCount; ++Index)
	{
		ASummonedUnit* Unit = NewGroup.Members[Index].Get();
		if (!Unit)
		{
			continue; // filtered alive above; belt-and-braces
		}
		const float RingFraction = (static_cast<float>(Index) + 0.5f) / static_cast<float>(MemberCount);
		const float RingRadius = NewGroup.PositionRadius * FMath::Sqrt(RingFraction);
		const float RingAngle = static_cast<float>(Index) * GoldenAngleRadians;
		FVector Station = NewGroup.PositionCenter
			+ FVector(RingRadius * FMath::Cos(RingAngle), RingRadius * FMath::Sin(RingAngle), 0.f);
		if (NavSys)
		{
			FNavLocation Projected;
			if (NavSys->ProjectPointToNavigation(Station, Projected, StationProjectExtent))
			{
				Station = Projected.Location;
			}
		}
		Unit->AssignCommandGroup(NewGroup.GroupId, Station - NewGroup.PositionCenter);
	}

	const int32 NewGroupId = NewGroup.GroupId;
	UnitGroups.Add(MoveTemp(NewGroup));

	// reap any older group the steal emptied (its markers die with it)
	PruneUnitGroups();

	// ⚠️ THE ONE STATEMENT WHOSE POSITION MOVED, AND IT IS PROVABLY INERT: in the
	// shipped body this log sat AFTER the two lines that null the pick's marker
	// refs (GroupPickPositionDecal / GroupPickActiveDecal), which are now the
	// caller's to null. Those two writes are private member-pointer assignments
	// that this log reads NONE of, and UE_LOG cannot re-enter gameplay code, so
	// swapping their order changes nothing observable. The log TEXT is byte-
	// identical, including the "(TASK-344)" provenance tag.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': %s group %d formed — %d unit(s), position (%.0f, %.0f) r=%.0f, attack (%.0f, %.0f) r=%.0f (TASK-344)."),
		*GetNameSafe(this), TypeLabel, NewGroupId, MemberCount,
		PositionCenter.X, PositionCenter.Y, PositionRadius,
		AttackCenter.X, AttackCenter.Y, AttackRadius);

	return NewGroupId;
}

void ASiegePlayerController::CancelGroupPick()
{
	// the ExitTargetingMode law (qa/TASK-003-report.md warning 2): release the
	// melee suppression BEFORE any early-out. Every pick exit path — the final
	// confirm, RMB/Esc cancel (binding or polled), T/E release, match end, hero
	// death, unpossess, match reset, EndPlay — funnels through here.
	// GroupPickHero is a SEPARATE record from PlacementHero/TargetingHero, so a
	// defensive ExitPlacement/ExitTargeting call can never strand a live pick
	// suppression (and SetMeleeSuppressed(false) is an idempotent flag write).
	if (IsValid(GroupPickHero))
	{
		GroupPickHero->SetMeleeSuppressed(false);
	}
	GroupPickHero = nullptr;

	if (GroupPickStage == EGroupPickStage::None)
	{
		return;
	}

	GroupPickStage = EGroupPickStage::None;
	bGroupPickSurfaceValid = false;
	GroupPickLocation = FVector::ZeroVector;
	GroupPickRadius = 0.f;
	GroupPickSelectedMembers.Reset();
	GroupPickPositionCenter = FVector::ZeroVector;
	GroupPickPositionRadius = 0.f;

	// destroy every pick circle this flow still OWNS. The stage-3 confirm nulls
	// the Position + Attack refs first (they transferred to the group as its
	// persistent markers), so a completed flow only loses its SELECT circle; a
	// cancel at any stage destroys all live circles. Existing groups' markers
	// are NEVER touched here — they die with their group (prune/release).
	if (GroupPickActiveDecal)
	{
		GroupPickActiveDecal->Destroy();
		GroupPickActiveDecal = nullptr;
	}
	if (GroupPickSelectDecal)
	{
		GroupPickSelectDecal->Destroy();
		GroupPickSelectDecal = nullptr;
	}
	if (GroupPickPositionDecal)
	{
		GroupPickPositionDecal->Destroy();
		GroupPickPositionDecal = nullptr;
	}

	// empty prompt ⇒ the HUD falls back to its stance display (the additive-bind
	// contract). The stage-3 confirm re-broadcasts its completion prompt AFTER
	// funneling through here.
	BroadcastCommandPrompt(FString());

	// restores game-only free-look — unless IA_UICursor is still held, in which
	// case the cursor stays up for the HUD (the cursor owners compose)
	ApplyCursorInputState();
}

ADecalActor* ASiegePlayerController::SpawnGroupCircleDecal(float Radius)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// null-safe circle material (the SpawnSpellReticle recipe, cloned): missing
	// M_SpellReticle ⇒ NO circle actor — the pick still works off the trace (the
	// invisible-ghost degradation precedent), logged ONCE per controller (the
	// shared bWarnedNoReticleMaterial latch).
	UMaterialInterface* ReticleMaterial = SpellReticleMaterialAsset.LoadSynchronous();
	if (!ReticleMaterial)
	{
		if (!bWarnedNoReticleMaterial)
		{
			bWarnedNoReticleMaterial = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': circle material '%s' not found (built in TASK-108) — the group pick continues without circle visuals."),
				*GetNameSafe(this), *SpellReticleMaterialAsset.ToString());
		}
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	// Spawn at IDENTITY rotation (the TASK-100 composition lesson, verbatim from
	// SpawnSpellReticle): ADecalActor's constructor already gives its root decal
	// component relative pitch -90, and UE 5.8's PostSpawnInitialize COMPOSES
	// root ∘ spawn transform — spawning with -90 here would compose to -180 and
	// lay the projection axis horizontal. Identity composes to the CDO's own -90.
	ADecalActor* CircleActor = World->SpawnActor<ADecalActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CircleActor)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': failed to spawn a group-pick circle decal — the pick continues without this circle's visual."),
			*GetNameSafe(this));
		return nullptr;
	}

	// ABSOLUTE -90 pitch points the decal's projection axis straight DOWN —
	// SetActorRotation is world-absolute, immune to CDO/spawn composition. The
	// decal drapes whatever surface lies under it (hill crowns and flanks
	// included). Decals carry NO collision, so a circle can never block the
	// cursor trace.
	CircleActor->SetActorRotation(FRotator(-90.f, 0.f, 0.f));
	CircleActor->SetActorHiddenInGame(true); // shown on the first surface hit

	if (UDecalComponent* CircleDecal = CircleActor->GetDecal())
	{
		// OPTIONAL stage tint (flagged to art, TASK-345): pushed through an MID at
		// the "StageTint" vector parameter — M_SpellReticle does not carry it YET,
		// and SetVectorParameterValue on an absent parameter is a silent no-op, so
		// this costs nothing until the artist adds the param (stock nodes only).
		UMaterialInstanceDynamic* CircleMID = UMaterialInstanceDynamic::Create(ReticleMaterial, CircleActor);
		if (CircleMID)
		{
			static const FName StageTintParamName(TEXT("StageTint"));
			FLinearColor StageTint = FLinearColor::White; // Select
			if (GroupPickStage == EGroupPickStage::Position)
			{
				StageTint = FLinearColor(0.2f, 1.f, 0.3f); // position zone: green
			}
			else if (GroupPickStage == EGroupPickStage::AttackZone)
			{
				StageTint = FLinearColor(1.f, 0.35f, 0.2f); // attack zone: red
			}
			CircleMID->SetVectorParameterValue(StageTintParamName, StageTint);
			CircleDecal->SetDecalMaterial(CircleMID);
		}
		else
		{
			CircleDecal->SetDecalMaterial(ReticleMaterial);
		}

		// footprint = this circle's radius; X (the projection half-depth) is 500 —
		// bracketing the M4.5 max terrain height (250) exactly like the spell
		// reticle's window, so the decal reaches the surface on every hill.
		CircleDecal->DecalSize = FVector(500.f, Radius, Radius);
		CircleDecal->MarkRenderStateDirty();
	}

	return CircleActor;
}

void ASiegePlayerController::ConfirmFollowPick()
{
	// THE ONE-STAGE TERMINAL CONFIRM (TASK-395; CONVENTIONS §1). Reached ONLY from
	// ConfirmGroupPickStage's Select case with GroupPickType == Follow, and only
	// with a NON-EMPTY sweep (the caller refuses an empty circle and stays in the
	// stage). Nothing here places a zone: a follow group's anchor is the live hero.
	//
	// ORDERING NOTE: everything that reads the pick scratch (GroupPickSelectedMembers,
	// GroupPickRadius) must run BEFORE the CancelGroupPick teardown at the bottom,
	// which resets all of it.

	// enrol every circled unit into THE ONE follow group. EnrollInDefaultFollowGroup
	// creates it lazily, steals the unit out of any Hold/Ambush group, stations it
	// and drops its target — so C and the spawn default share one code path.
	for (const TWeakObjectPtr<ASummonedUnit>& Member : GroupPickSelectedMembers)
	{
		if (ASummonedUnit* Unit = Member.Get())
		{
			EnrollInDefaultFollowGroup(Unit);
		}
	}

	// Count what the player actually got: every circled unit whose group id now IS
	// the follow group. That deliberately includes units that were ALREADY
	// following (the idempotent enroll path) — "5 units are following you" is the
	// honest readout of the command just issued — and excludes anything refused.
	int32 FollowingCount = 0;
	if (DefaultFollowGroupId != INDEX_NONE)
	{
		for (const TWeakObjectPtr<ASummonedUnit>& Member : GroupPickSelectedMembers)
		{
			const ASummonedUnit* Unit = Member.Get();
			if (Unit && Unit->GetCommandGroupId() == DefaultFollowGroupId)
			{
				++FollowingCount;
			}
		}
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': FOLLOW confirmed — %d unit(s) following (group %d, select radius %.0f, formation radius %.0f) (TASK-395)."),
		*GetNameSafe(this), FollowingCount, DefaultFollowGroupId, GroupPickRadius, FollowFormationRadius);

	// CancelGroupPick is the ONE teardown call (the melee-release-before-early-out
	// law) and it destroys the SELECT circle — deliberately: unlike Hold/Ambush,
	// which transfer their zone circles to the group as persistent ground markers,
	// a follow group owns NO ground, so a leftover decal would sit at a stale spot
	// forever. Nothing is nulled first, so the transient pick visual dies here.
	CancelGroupPick();

	// completion prompt AFTER the teardown's empty broadcast, so this is what
	// remains on the HUD (the stage-3 confirm's ordering).
	BroadcastCommandPrompt(FString::Printf(TEXT("FOLLOW set: %d unit(s)"), FollowingCount));
}

int32 ASiegePlayerController::EnsureDefaultFollowGroup()
{
	// M8 D5 observer posture (doc §4.1 — the BeginGroupPick / SetUnitCommand
	// precedent): group state is host-side in P1, so a client-built follow group
	// would be state no unit can read. Callers treat INDEX_NONE as "skip silently".
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Verbose,
			TEXT("ASiegePlayerController '%s': EnsureDefaultFollowGroup refused — P1 client observer posture (M8 doc §4.1)."),
			*GetNameSafe(this));
		return INDEX_NONE;
	}

	// Re-validate the stored id against the LIVE array — the anti-leak SECOND line
	// of defence. PruneUnitGroups and ClearAllUnitGroups both reset the id when
	// they destroy the group, but validating here means even a missed reset
	// self-heals into a fresh group instead of leaving the spawn default
	// permanently pointed at a group that no longer exists (ids are never reused,
	// so a stale id can never alias a different group — it only ever goes dead).
	if (DefaultFollowGroupId != INDEX_NONE && FindUnitGroup(DefaultFollowGroupId) != nullptr)
	{
		return DefaultFollowGroupId;
	}

	// Create it: a NORMAL FSiegeUnitGroup with Type Follow and NO GROUND — zero
	// radii, zero centers, null marker decals (CONVENTIONS §1 — the struct is
	// unchanged, the defaults already are exactly that). The id comes from the
	// SAME never-reused counter the pick-built groups use.
	FSiegeUnitGroup FollowGroup;
	FollowGroup.GroupId = NextUnitGroupId++;
	FollowGroup.Type = ESiegeGroupCommandType::Follow;

	const int32 NewFollowGroupId = FollowGroup.GroupId; // captured BEFORE the move
	UnitGroups.Add(MoveTemp(FollowGroup));
	DefaultFollowGroupId = NewFollowGroupId;
	NextFollowStationIndex = 0; // fresh group ⇒ fresh sunflower, starting at the centre slot

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': default FOLLOW group %d created (TASK-395 — one per controller, re-derived lazily after any release)."),
		*GetNameSafe(this), NewFollowGroupId);

	return NewFollowGroupId;
}

void ASiegePlayerController::EnrollInDefaultFollowGroup(ASummonedUnit* Unit)
{
	// NULL-SAFE AND SILENT ON EVERY REFUSAL (CONVENTIONS §2). This runs on EVERY
	// unit spawn (the auto-enroll lives on ASummonedUnit::BeginPlay — TASK-396 —
	// which is what covers player placement, the ABarracks spawner and
	// SummonTestUnit from one insertion point), so a refusal must degrade to
	// today's behavior: never a crash, never a stall.
	if (!IsValid(Unit))
	{
		return;
	}

	// The eligibility gate is the UNIT's (TASK-396): CanFollowHero() && Blue &&
	// alive && not match-end frozen. That is what keeps the Ogre and the Sapper
	// auto-marching (Siege is not follow-eligible — Jonathan's explicit carve-out)
	// and every Red / bot unit out of the group.
	if (!Unit->IsFollowCommandEligible())
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': follow enroll skipped for '%s' — not follow-eligible (TASK-395)."),
			*GetNameSafe(this), *GetNameSafe(Unit));
		return;
	}

	const int32 FollowGroupId = EnsureDefaultFollowGroup();
	if (FollowGroupId == INDEX_NONE)
	{
		return; // non-authority (already logged) — the unit just runs its normal body
	}

	FSiegeUnitGroup* FollowGroup = FindUnitGroupMutable(FollowGroupId);
	if (!FollowGroup)
	{
		// tripwire: EnsureDefaultFollowGroup either validated or created this id in
		// the same call stack, so a miss means the array was mutated underneath us.
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': follow group %d vanished immediately after EnsureDefaultFollowGroup — '%s' not enrolled."),
			*GetNameSafe(this), FollowGroupId, *GetNameSafe(Unit));
		return;
	}

	const TWeakObjectPtr<ASummonedUnit> WeakUnit(Unit);

	// IDEMPOTENT: an already-following unit KEEPS the station it was given at its
	// own enroll. Re-stationing on every C press would shuffle a settled squad for
	// no reason, and the spawn default means C is routinely pressed over units
	// that are already in this group.
	if (FollowGroup->Members.Contains(WeakUnit))
	{
		return;
	}

	// STEAL (the shipped re-selection law): joining Follow takes the unit out of
	// any Hold/Ambush group. Removing from a group's Members never resizes
	// UnitGroups, so the FollowGroup pointer stays valid across this loop.
	for (FSiegeUnitGroup& OtherGroup : UnitGroups)
	{
		if (OtherGroup.GroupId != FollowGroupId)
		{
			OtherGroup.Members.Remove(WeakUnit);
		}
	}

	FollowGroup->Members.Add(WeakUnit);

	// The station is computed ONCE, here, from a never-reused ordinal, and pushed
	// to the unit as a plain scalar offset (per-unit scalars only — no arrays on
	// units). AssignCommandGroup also DROPS the unit's current target, which is
	// exactly right on the way into a body that must never attack.
	const int32 StationIndex = NextFollowStationIndex++;
	Unit->AssignCommandGroup(FollowGroupId, ComputeFollowStationOffset(StationIndex));

	UE_LOG(LogGitClaudeUnrealTest, Verbose,
		TEXT("ASiegePlayerController '%s': '%s' enrolled in follow group %d at station %d (TASK-395)."),
		*GetNameSafe(this), *GetNameSafe(Unit), FollowGroupId, StationIndex);

	// Reap any group the steal emptied — markers included — synchronously, the
	// stage-3 confirm's rule. ⚠️ This can RemoveAt on UnitGroups, so FollowGroup is
	// DANGLING from here on and is deliberately never touched again. The follow
	// group itself can never be the one reaped: it holds the unit just added.
	PruneUnitGroups();
}

AActor* ASiegePlayerController::GetFollowAnchor() const
{
	// ⚠️ LIVE RESOLVE, NEVER CACHED (CONVENTIONS §4). GetPawn() is re-read on every
	// call, so a post-respawn REPLACEMENT pawn actor is picked up for free — that
	// is the entire reason the ruling forbids caching. Callers reach this
	// controller through FindControllerForTeam(World, Team); GetFirstPlayerController
	// is BANNED in gameplay code (M8 TEAM LAW).
	APawn* HeroPawn = GetPawn();
	if (!IsValid(HeroPawn))
	{
		return nullptr;
	}

	// HERO-DEATH RULING (manager, CONVENTIONS §4 flag): a DEAD hero is not an
	// anchor. nullptr is the signal that turns the follow body into HOLD POSITION
	// (EnterIdle — no target, no march, no attack); following resumes the instant a
	// live pawn resolves again. Rejected and recorded: marching to the corpse, and
	// falling back to Defend (which would make followers fight, breaking Jonathan's
	// ruling that following units never attack).
	if (const AHeroCharacter* Hero = Cast<AHeroCharacter>(HeroPawn))
	{
		if (Hero->IsDead())
		{
			return nullptr;
		}
	}

	// ⭐⭐ AND WHILE THE DEATH GHOST IS POSSESSED THIS RETURNS THE GHOST — DELIBERATELY,
	// AND IT IS **RULED**, NOT INCIDENTAL (TASK-750, GHOST-§3 **G-8**): "while dead,
	// the `hero` place symbol resolves to THE GHOST'S LOCATION ... `follow` and `rally`
	// are hero-relative intents, so resolving `hero` to a hidden corpse would silently
	// walk the player's army to where he died ⇒ `rally` and `follow` keep working and
	// the ghost is the anchor." Jonathan's own words for the ghost are "it can command
	// units", and an army that abandons its commander the moment he dies is not that.
	//
	// ⛔ ZERO LINES WERE ADDED TO MAKE THIS TRUE, AND THAT IS THE POINT: the live
	// GetPawn() resolve above — the ruling that forbids caching — picks the ghost up on
	// its own. It is written down HERE rather than left as a happy accident so that the
	// day someone "tidies" this function by casting to AHeroCharacter first, they are
	// told that doing so breaks a Jonathan-level ruling.
	//
	// ⚠️ THE HONEST LIMIT: `rally` still refuses while ghosted, and correctly —
	// USiegeAssistantComponent::ExecuteRallyOrder casts this anchor to AHeroCharacter
	// because Rally() is a HERO ability with its own cooldown. Following anchors on the
	// ghost; the rally *ability* does not exist on it. (That file is TASK-746's lane and
	// is deliberately untouched here.)
	return HeroPawn;
}

FVector ASiegePlayerController::ComputeFollowStationOffset(int32 StationIndex) const
{
	// Deterministic golden-angle sunflower — the shipped stage-3 spread recipe
	// re-anchored on a MOVING point instead of a fixed circle. The RADIUS wraps
	// through FollowFormationSlots so any squad size stays inside
	// FollowFormationRadius; the ANGLE does not wrap, so the golden angle keeps
	// every live follower on its own bearing and the squad never mills at one
	// point (the TASK-280/282 lesson's spread half).
	//
	// NOT nav-projected on purpose (the one deviation from the stage-3 recipe):
	// this is an offset from a point that moves, so a projection taken at enroll
	// against a stale hero position would be meaningless. The unit's
	// EnterAdvanceToLocation already passes bProjectDestinationToNavigation.
	const int32 SafeIndex = FMath::Max(StationIndex, 0);
	const int32 Slot = SafeIndex % FollowFormationSlots;
	const float RingFraction = (static_cast<float>(Slot) + 0.5f) / static_cast<float>(FollowFormationSlots);
	const float RingRadius = FollowFormationRadius * FMath::Sqrt(RingFraction);
	const float RingAngle = static_cast<float>(SafeIndex) * GoldenAngleRadians;
	return FVector(RingRadius * FMath::Cos(RingAngle), RingRadius * FMath::Sin(RingAngle), 0.f);
}

FSiegeUnitGroup* ASiegePlayerController::FindUnitGroupMutable(int32 GroupId)
{
	// const_cast off the shipped const lookup so there is exactly ONE search
	// implementation (and one place the INDEX_NONE early-out lives). The pointed-to
	// group is not const — it aliases into this controller's own UnitGroups — so
	// this is well defined. Same aliasing rule as FindUnitGroup: use it within the
	// current call stack, NEVER cache it.
	return const_cast<FSiegeUnitGroup*>(FindUnitGroup(GroupId));
}

void ASiegePlayerController::PruneUnitGroups()
{
	// 1 s maintenance reaper (TASK-344; the CONVENTIONS ≤1 s marker-removal law):
	// compact stale/dead members out of every group, then destroy any group with
	// none left — its markers die with it. Also called synchronously by the
	// stage-3 steal so a steal-emptied group never outlives the confirm. Reverse
	// iteration keeps RemoveAt index-safe.
	for (int32 GroupIndex = UnitGroups.Num() - 1; GroupIndex >= 0; --GroupIndex)
	{
		FSiegeUnitGroup& Group = UnitGroups[GroupIndex];
		for (int32 MemberIndex = Group.Members.Num() - 1; MemberIndex >= 0; --MemberIndex)
		{
			const ASummonedUnit* Member = Group.Members[MemberIndex].Get();
			if (!Member || Member->IsUnitDead())
			{
				Group.Members.RemoveAt(MemberIndex);
			}
		}
		if (Group.Members.Num() == 0)
		{
			if (Group.PositionMarkerDecal)
			{
				Group.PositionMarkerDecal->Destroy();
				Group.PositionMarkerDecal = nullptr;
			}
			if (Group.AttackMarkerDecal)
			{
				Group.AttackMarkerDecal->Destroy();
				Group.AttackMarkerDecal = nullptr;
			}

			// ⚠️ ANTI-LEAK (TASK-395 spec item 6): an EMPTY default follow group is
			// legitimately reaped here — every follower died, or C was pressed and the
			// enrolments all failed. That is FINE, but the id must not be left
			// dangling: ids are never reused, so a stale DefaultFollowGroupId would
			// leave the spawn default permanently pointed at nothing. Reset it and let
			// EnsureDefaultFollowGroup re-derive lazily on the next enroll or C press.
			const bool bWasDefaultFollowGroup = (Group.GroupId == DefaultFollowGroupId);
			if (bWasDefaultFollowGroup)
			{
				DefaultFollowGroupId = INDEX_NONE;
			}

			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': unit group %d emptied — group and markers removed (TASK-344 prune)%s"),
				*GetNameSafe(this), Group.GroupId,
				bWasDefaultFollowGroup ? TEXT("; it was the default FOLLOW group — id reset, re-derives lazily (TASK-395).") : TEXT("."));
			UnitGroups.RemoveAt(GroupIndex);
		}
	}
}

void ASiegePlayerController::ClearAllUnitGroups()
{
	// the RELEASE law (TASK-344): T/E and Play Again replace EVERY group order.
	// Members are cleared EXPLICITLY (immediate — no 0.25 s self-heal wait, the
	// units adopt the new stance on their very next state tick), markers
	// destroyed, prompt emptied so the HUD returns to its stance display.
	//
	// ⚠️ TASK-395: "EVERY group order" now INCLUDES the default FOLLOW group, and
	// that is deliberate — T is therefore the "everyone attack" button (CONVENTIONS
	// §2 release law, now load-bearing). The id is reset FIRST, above the
	// empty-array early-out, so the anti-leak invariant holds on every path
	// regardless of what the prune already did. The group re-creates lazily on the
	// next unit spawn or C press; units still holding the dead id self-heal on
	// their next state tick (FindUnitGroup returns null).
	DefaultFollowGroupId = INDEX_NONE;
	NextFollowStationIndex = 0;

	if (UnitGroups.Num() == 0)
	{
		return;
	}

	for (FSiegeUnitGroup& Group : UnitGroups)
	{
		for (const TWeakObjectPtr<ASummonedUnit>& Member : Group.Members)
		{
			if (ASummonedUnit* Unit = Member.Get())
			{
				Unit->ClearCommandGroup();
			}
		}
		if (Group.PositionMarkerDecal)
		{
			Group.PositionMarkerDecal->Destroy();
			Group.PositionMarkerDecal = nullptr;
		}
		if (Group.AttackMarkerDecal)
		{
			Group.AttackMarkerDecal->Destroy();
			Group.AttackMarkerDecal = nullptr;
		}
	}

	const int32 ClearedCount = UnitGroups.Num();
	UnitGroups.Reset();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': %d unit group(s) cleared (release law, TASK-344)."),
		*GetNameSafe(this), ClearedCount);

	BroadcastCommandPrompt(FString());
}

const FSiegeUnitGroup* ASiegePlayerController::FindUnitGroup(int32 GroupId) const
{
	if (GroupId == INDEX_NONE)
	{
		return nullptr;
	}
	for (const FSiegeUnitGroup& Group : UnitGroups)
	{
		if (Group.GroupId == GroupId)
		{
			return &Group;
		}
	}
	// no such group (T/E release, all-dead prune, steal-emptied, Play Again):
	// the unit-side caller SELF-HEALS to the legacy stance gate on this null
	return nullptr;
}

void ASiegePlayerController::BroadcastCommandPrompt(const FString& Prompt)
{
	// stage prompts ALSO log (the law: the feature ships without the WBP bind).
	// Empty prompts (pick over / groups released) broadcast quietly.
	if (!Prompt.IsEmpty())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': command prompt — %s"),
			*GetNameSafe(this), *Prompt);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': command prompt cleared."),
			*GetNameSafe(this));
	}
	OnCommandPromptChanged.Broadcast(Prompt);
}

void ASiegePlayerController::ResolveSpellInstant(int32 Slot, FName CardID, const FCardRow& Row, ASiegePlayerState& SiegeState)
{
	// An UNAIMED spell resolves INSTANTLY on play (M5 ruling 7 — no reticle for a
	// global effect; recorded §3.5 deviation, CardType stays Spell). ⛔ WIDENED BY
	// TASK-1018 from "GoldSteal" to the derived answer: the two callers gate on
	// `USpellLibrary::SpellRequiresAiming`, so `Pickpocket`, `Fog` and `BrightSun`
	// all arrive here today (`FOG-§10.1`: "NO RETICLE"). The ruling-8 confirm shape
	// applied at PLAY time: deduct THEN resolve, refusal-safe — a resolver false
	// FULLY refunds (§3.0 net-zero) and keeps the card in hand.
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': ResolveSpellInstant('%s') with no world — nothing resolved, no gold moved."),
			*GetNameSafe(this), *CardID.ToString());
		return;
	}

	// deduct FIRST (deduct-then-resolve, ruling 7). CanAfford held in the SAME
	// synchronous call stack (PlayHandSlot / EnterTargetingMode), so a false
	// here is the instant-play hard-invariant tripwire, not a live path.
	if (!SiegeState.SpendGold(Row.Cost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': SpendGold(%d) failed for instant spell '%s' AFTER the CanAfford pre-check (should be unreachable) — no resolve, no draw."),
			*GetNameSafe(this), Row.Cost, *CardID.ToString());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
		return;
	}

	// caster team mirrors the confirm-path resolution (local player = Blue)
	const AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
	const ETeamId CasterTeam = IsValid(Hero) ? Hero->GetTeamId() : ETeamId::Blue;

	// TargetPoint is only the VFX anchor for an UNAIMED, global spell (SpellLibrary
	// contract) — the hero's feet when we have one, world origin otherwise.
	//
	// ⛔⛔ SITE 3 OF 3 (TASK-1018) — AND IT IS A COMMENT, WHICH IS EXACTLY WHY IT IS
	// PART OF THE FIX. It used to assert that GoldSteal was the sole effect able to
	// reach this instant path. ⛔ The old wording is DELIBERATELY NOT QUOTED here:
	// Tests/SiegeSpellRoutingTest.cpp asserts that sentence's ABSENCE with a
	// COMMENT-AWARE scanner (a code-only one is blind to a lie that lives in prose),
	// so reproducing it — even to explain it — would re-fail the very gate that
	// guards it. ⛔ Do not paste it back in.
	// That sentence was true when it was written and the two routing repairs above
	// make it FALSE: `Fog` (FogCover) and `BrightSun` (FogClear) reach here now, and
	// so will every future spell the aim predicate answers `false` for. ⚖️ REPAIRING
	// THE CODE AND LEAVING THE PROSE WOULD SHIP A CONFIDENT EXPLANATION OF BEHAVIOUR
	// THAT NO LONGER EXISTS — the stale-prose class (`SC-§77`) arriving inside the
	// fix for a different defect.
	//
	// TASK-236 call-site flag, restated correctly: every caller of this path is a
	// spell for which `USpellLibrary::SpellRequiresAiming` is FALSE, so there is no
	// aim point to shift — the resolver's FogCover, FogClear and GoldSteal arms all
	// state in place that TargetPoint plays no gameplay role for them. The anchor
	// below feeds the ruling-11 VFX spawn and the world one-shot, nothing else.
	const FVector AnchorPoint = IsValid(Hero) ? Hero->GetActorLocation() : FVector::ZeroVector;

	if (!USpellLibrary::ResolveSpell(World, CardID, Row, CasterTeam, AnchorPoint))
	{
		// FULL refund (§3.0) — refusal-safe by construction: e.g. Sandbox mode
		// has no Red economy to steal from, so the play refuses net-zero with
		// the card still in hand. (A 0-gold victim, by contrast, RESOLVES for
		// min(GoldSteal, 0) = 0 — the spell is spent, per the resolver contract.)
		// ⛔ TASK-1018: this is now ALSO the fog cards' fizzle site, and it is a
		// DIFFERENT call site from the targeted confirm's refund — the fog arms
		// return false for real, reachable reasons (no AFogVolume could be spawned;
		// `RaiseFog` refused under a live prevention window, J-F19; `ApplyBrightSun`
		// refused a window-shortening cast, J-F18), so both refunds stay live and
		// both keep the card in hand. The entry gates above catch the two common
		// cases FIRST with a specific, numbered message; this is the backstop that
		// still refunds net-zero when they do not (the bot's and the hand-less
		// direct entry's only protection).
		// The >0 guard only skips the no-op refund of a 0-cost card (AddGold
		// refuses non-positive grants with a log) — net-zero holds either way.
		if (Row.Cost > 0)
		{
			SiegeState.AddGold(Row.Cost);
		}
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': ResolveSpell('%s') refused (instant, M5 ruling 7) — %d gold refunded, card kept (the SpellLibrary log names the cause)."),
			*GetNameSafe(this), *CardID.ToString(), Row.Cost);
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_SpellFizzled", "Spell fizzled"));
		return;
	}

	// §6 spell-cast audio (TASK-179): the instant spell RESOLVED — a world one-shot at
	// the anchor (the hero's feet), null-safe until S_SpellCast lands (TASK-180).
	USiegeFeedbackLibrary::PlayWorldSound(this, SpellCastSoundPath, AnchorPoint);

	// hand step (§3.4): resolution IS the confirm for an instant — discard +
	// redraw. INDEX_NONE = a direct hand-less EnterTargetingMode call with an
	// unaimed spell (TASK-1018 widened the routing; the slot semantics are
	// unchanged).
	if (Slot != INDEX_NONE)
	{
		ConfirmInstantDraw(Slot, CardID);
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': played instant spell '%s' for %d gold (M5 ruling 7, GDD §3.11)."),
		*GetNameSafe(this), *CardID.ToString(), Row.Cost);
}

bool ASiegePlayerController::TraceCursorToGround(FHitResult& OutHit) const
{
	// Visibility-channel trace under the cursor. The arena ground (TASK-015)
	// blocks Visibility; pawn capsules ignore it; the ghost has collision fully
	// disabled — so a blocking hit is a placeable surface ("ground hit").
	return GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/ false, OutHit) && OutHit.bBlockingHit;
}

const FCardRow* ASiegePlayerController::ResolveCardRow(FName CardID, FString& OutError) const
{
	const UDataTable* CardTable = CardTableAsset.LoadSynchronous();
	if (!CardTable)
	{
		OutError = FString::Printf(TEXT("card table '%s' not found (imported from Docs/Data/cards.csv in TASK-008)."), *CardTableAsset.ToString());
		return nullptr;
	}

	const FCardRow* Row = CardTable->FindRow<FCardRow>(CardID, TEXT("ASiegePlayerController::ResolveCardRow"), /*bWarnIfRowMissing=*/ false);
	if (!Row)
	{
		OutError = FString::Printf(TEXT("row '%s' not found in '%s'."), *CardID.ToString(), *CardTableAsset.ToString());
	}
	return Row;
}

UClass* ASiegePlayerController::ResolveCardActorClass(FName CardID, ECardType CardType) const
{
	// CONVENTIONS composed soft-class paths, selected by the card's EFFECTIVE spawn
	// category (TASK-030 + TASK-059): a Building card OR an Economy-typed building
	// card (Deep Mine — CardType Economy but ADeepMine under /Blueprints/Buildings/,
	// TASK-057) resolves /Game/Blueprints/Buildings/BP_Building_<CardID> (ABuilding);
	// a Unit card OR an Economy-typed UNIT card (Miner) resolves
	// /Game/Blueprints/Units/BP_Unit_<CardID> (ASummonedUnit). Missing/incompatible =
	// nullptr; the caller refuses the play with NO gold spent (composed soft-class
	// law — the M1 meshless fallback is retired). IsBuildingCard is the single source
	// shared with the confirm spawn branch + the §3.5 clearance rule.
	const FString CardName = CardID.ToString();
	FString ClassPath;
	UClass* RequiredBase = nullptr;
	const TCHAR* CreatedInTask = TEXT("");

	if (IsBuildingCard(CardID, CardType))
	{
		ClassPath = FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C"), *CardName, *CardName);
		RequiredBase = ABuilding::StaticClass();
		CreatedInTask = TEXT("TASK-035/063");
	}
	else if (CardType == ECardType::Unit || CardType == ECardType::Economy)
	{
		ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);
		RequiredBase = ASummonedUnit::StaticClass();
		CreatedInTask = TEXT("TASK-010/034");
	}
	else
	{
		// PlayHandSlot's type switch keeps every other CardType out of placement
		// mode (instants resolve without a spawn) — reaching this is a caller regression.
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': ResolveCardActorClass('%s') — CardType %d is not a placement type (gated in PlayHandSlot)."),
			*GetNameSafe(this), *CardName, static_cast<int32>(CardType));
		return nullptr;
	}

	UClass* ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (ActorClass && ActorClass->IsChildOf(RequiredBase))
	{
		return ActorClass;
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("ASiegePlayerController '%s': card class '%s' missing or not a %s (built in %s) — play refused, no gold spent (CONVENTIONS composed soft-class law)."),
		*GetNameSafe(this), *ClassPath, *RequiredBase->GetName(), CreatedInTask);
	return nullptr;
}

bool ASiegePlayerController::IsBuildingCard(FName CardID, ECardType CardType) const
{
	// Single source of truth for the building spawn branch, the §3.5 clearance rule,
	// and the BP-class path (TASK-059). Every Building card is a building; so is an
	// Economy-typed card whose ACTOR is an ABuilding — currently only Deep Mine (its
	// row is CardType Economy for the §8 raidable-economy semantics, but ADeepMine
	// derives ABuilding). The set is the editable BuildingEconomyCardIDs UPROPERTY
	// (not a hardcoded CardID), so a future Economy-building needs no code change
	// (TASK-057 routing carry-forward).
	if (CardType == ECardType::Building)
	{
		return true;
	}
	return CardType == ECardType::Economy && BuildingEconomyCardIDs.Contains(CardID);
}

void ASiegePlayerController::ResolveInstantPlay(int32 Slot, FName CardID, const FCardRow& Row, ASiegePlayerState& SiegeState)
{
	// INSTANT resolution (GDD §3.5/§3.10/§4, TASK-059): HeroUpgrade and Utility cards
	// resolve IMMEDIATELY — NO placement step. Affordability was already gated by the
	// CanAfford pre-check in PlayHandSlot (no gold moved). The card leaves the hand
	// (ConfirmPlayFromHand → redraw) and gold is spent ONLY when the effect actually
	// lands — every refusal path below moves NO gold (§3.0 full refund).
	if (Row.CardType == ECardType::HeroUpgrade)
	{
		AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
		if (!Hero)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': hero upgrade '%s' refused — no hero pawn to upgrade (no gold spent)."),
				*GetNameSafe(this), *CardID.ToString());
			RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_NoHero", "Hero unavailable"));
			return;
		}

		// Apply FIRST — the result decides refund (TASK-058 contract): there is no
		// RemoveUpgrade, so we MUST know the result before spending. Spend + draw only
		// on Applied; RefusedAtMaxStacks / RefusedInvalidCard refuse with NO spend.
		const EHeroUpgradeResult Result = Hero->ApplyUpgrade(CardID);
		switch (Result)
		{
		case EHeroUpgradeResult::Applied:
			// Gold cannot fail here (CanAfford held in the SAME synchronous call stack;
			// nothing spent in between, income only adds). A false return would mean the
			// upgrade was granted without payment — logged as a hard-invariant tripwire.
			if (!SiegeState.SpendGold(Row.Cost))
			{
				UE_LOG(LogGitClaudeUnrealTest, Error,
					TEXT("ASiegePlayerController '%s': SpendGold(%d) failed AFTER ApplyUpgrade('%s') returned Applied — upgrade granted without payment (should be unreachable; CanAfford held at entry)."),
					*GetNameSafe(this), Row.Cost, *CardID.ToString());
			}
			ConfirmInstantDraw(Slot, CardID);
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': played hero upgrade '%s' for %d gold — stack applied, replacement drawn (GDD §3.10)."),
				*GetNameSafe(this), *CardID.ToString(), Row.Cost);
			break;

		case EHeroUpgradeResult::RefusedAtMaxStacks:
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': hero upgrade '%s' refused — already at max stacks (no gold spent, §3.10 full refund)."),
				*GetNameSafe(this), *CardID.ToString());
			RefuseCardPlay(CardID, FText::Format(
				NSLOCTEXT("Siegebound", "CardRefused_MaxStacks", "{0} at max stacks"),
				Row.DisplayName.IsEmpty() ? FText::FromName(CardID) : FText::FromString(Row.DisplayName)));
			break;

		case EHeroUpgradeResult::RefusedInvalidCard:
		default:
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': hero upgrade '%s' refused — ApplyUpgrade could not resolve the card/stack cap (no gold spent)."),
				*GetNameSafe(this), *CardID.ToString());
			RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_UpgradeInvalid", "Upgrade unavailable"));
			break;
		}
		return;
	}

	// Utility Instant (GDD §4). Masons repairs the friendly castle over time; any
	// other Utility CardID has no Instant effect yet and is refused with no gold moved.
	if (CardID == MasonsCardID)
	{
		// pre-check (§3.5 order): a living friendly castle to repair. None → refuse,
		// no spend (net-zero). Friendly team mirrors the confirm-path team resolution.
		const AHeroCharacter* Hero = Cast<AHeroCharacter>(GetPawn());
		const ETeamId FriendlyTeam = IsValid(Hero) ? Hero->GetTeamId() : ETeamId::Blue;
		ACastle* FriendlyCastle = FindFriendlyCastle(FriendlyTeam);
		if (!FriendlyCastle)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': Masons refused — no living friendly (%s) castle to repair (no gold spent)."),
				*GetNameSafe(this), FriendlyTeam == ETeamId::Blue ? TEXT("Blue") : TEXT("Red"));
			RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_NoCastle", "No castle to repair"));
			return;
		}

		// spend, then apply the effect, then draw (spec order). SpendGold cannot fail
		// after the same-stack CanAfford pre-check — the tripwire covers the invariant.
		if (!SiegeState.SpendGold(Row.Cost))
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': SpendGold(%d) failed for Masons AFTER the CanAfford pre-check — no heal, no draw (should be unreachable)."),
				*GetNameSafe(this), Row.Cost);
			RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
			return;
		}

		FriendlyCastle->HealOverTime(MasonsHealAmount, MasonsHealDuration);
		ConfirmInstantDraw(Slot, CardID);
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': played Masons for %d gold — repairing castle '%s' %.0f HP over %.0fs, replacement drawn (GDD §4)."),
			*GetNameSafe(this), Row.Cost, *GetNameSafe(FriendlyCastle), MasonsHealAmount, MasonsHealDuration);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': Utility card '%s' refused — no Instant effect implemented for it (no gold spent)."),
		*GetNameSafe(this), *CardID.ToString());
	RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_UtilityUnknown", "Card not available"));
}

void ASiegePlayerController::ConfirmInstantDraw(int32 Slot, FName CardID)
{
	// Instant plays leave the hand at RESOLUTION (there is no placement CONFIRM step):
	// move the slot to discard and redraw its replacement (§3.4). Null-safe; a false
	// return is a regression tripwire — the slot was validated non-empty in
	// PlayHandSlot and nothing mutated the hand since (instants are fully synchronous).
	if (!DeckComponent)
	{
		return;
	}
	if (!DeckComponent->ConfirmPlayFromHand(Slot))
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': ConfirmPlayFromHand(%d) refused for instant '%s' — hand mutated during a synchronous instant (should be impossible)."),
			*GetNameSafe(this), Slot, *CardID.ToString());
	}
}

ACastle* ASiegePlayerController::FindFriendlyCastle(ETeamId FriendlyTeam) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// The first living (non-destroyed) castle on the friendly team (M1 is one castle
	// per team). A destroyed friendly castle means that side already lost, so the
	// match has ended and this play is gated out upstream — the !IsCastleDestroyed
	// guard is belt-and-braces.
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		ACastle* Castle = *It;
		if (IsValid(Castle) && Castle->GetTeamId() == FriendlyTeam && !Castle->IsCastleDestroyed())
		{
			return Castle;
		}
	}
	return nullptr;
}

TArray<ASummonedUnit*> ASiegePlayerController::SpawnUnitSwarm(
	UWorld* World, UClass* UnitClass, FName CardID, ETeamId Team,
	AActor* SpawnOwner, APawn* SpawnInstigator, const FVector& Center, int32 Count, float Radius)
{
	TArray<ASummonedUnit*> Spawned;
	if (!World || !UnitClass)
	{
		return Spawned;
	}

	// One shared capsule-lift for the whole group — every copy is the same class
	// (mirrors the M1/M2 single-unit lift so Count<=1 stays byte-for-byte).
	float CapsuleHalfHeight = SiegeSpawn::DefaultCapsuleHalfHeight;
	if (const ASummonedUnit* UnitCDO = UnitClass->GetDefaultObject<ASummonedUnit>())
	{
		if (const UCapsuleComponent* Capsule = UnitCDO->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
	}
	const float LiftZ = CapsuleHalfHeight + SiegeSpawn::SpawnGroundClearance;

	const int32 SpawnCount = FMath::Max(1, Count);

	// Swarm (Count > 1, GDD §3.0 — Militia Mob = 4): arrange the copies evenly on a
	// circle of Radius around Center and navmesh-project EACH ring point so no copy
	// spawns off the walkable surface (falling back to the raw ring point when there
	// is no nav data / the projection misses). Count <= 1 is the single-unit case:
	// spawn AT Center (the confirm path already validated it on the navmesh — no
	// reprojection, so the existing single-unit spawn is unchanged).
	UNavigationSystemV1* NavSys = (SpawnCount > 1) ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	const float ExtentXY = FMath::Max(Radius * 0.5f, 100.f);
	const FVector RingProjectExtent(ExtentXY, ExtentXY, 200.f);

	for (int32 Index = 0; Index < SpawnCount; ++Index)
	{
		FVector GroundPoint = Center;
		if (SpawnCount > 1)
		{
			const float Angle = (2.f * PI * Index) / SpawnCount;
			GroundPoint = Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.f);
			if (NavSys)
			{
				FNavLocation Projected;
				if (NavSys->ProjectPointToNavigation(GroundPoint, Projected, RingProjectExtent))
				{
					GroundPoint = Projected.Location;
				}
			}
		}

		// deferred spawn (TASK-004 preferred path) so InitUnit binds the card BEFORE
		// BeginPlay reads DT_Cards — never a mis-teamed first state check.
		const FTransform SpawnTransform(FRotator::ZeroRotator, GroundPoint + FVector(0.f, 0.f, LiftZ));
		ASummonedUnit* Unit = World->SpawnActorDeferred<ASummonedUnit>(
			UnitClass,
			SpawnTransform,
			SpawnOwner,
			SpawnInstigator,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Unit)
		{
			continue;
		}
		Unit->InitUnit(Team, CardID);
		Unit->FinishSpawning(SpawnTransform);
		Spawned.Add(Unit);
	}

	return Spawned;
}

UStaticMesh* ASiegePlayerController::ResolveGhostMesh(FName CardID) const
{
	// CONVENTIONS per-card visual contract: the ghost preview resolves
	// /Game/Meshes/SM_<CardID> by string (TASK-030 names block)
	const FString CardName = CardID.ToString();
	const FString MeshPath = FString::Printf(TEXT("/Game/Meshes/SM_%s.SM_%s"), *CardName, *CardName);
	if (UStaticMesh* CardMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath)).LoadSynchronous())
	{
		return CardMesh;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': ghost mesh '%s' not found (meshes arrive in TASK-014/037/038) — using the fallback sphere."),
		*GetNameSafe(this), *MeshPath);

	if (UStaticMesh* FallbackMesh = GhostFallbackMeshAsset.LoadSynchronous())
	{
		return FallbackMesh;
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("ASiegePlayerController '%s': ghost fallback mesh '%s' not found — placement runs with an invisible ghost."),
		*GetNameSafe(this), *GhostFallbackMeshAsset.ToString());
	return nullptr;
}

bool ASiegePlayerController::IsPointOnNavmesh(const FVector& Point)
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys || !NavSys->GetDefaultNavDataInstance())
	{
		// no nav system / no nav data in this world: degrade OPEN to the M1
		// half+ground rule with one warning — a missing system must never
		// brick placement (house null-safety law). L_Arena always has nav
		// data (TASK-015 navmesh; units path on it), so this never fires
		// there.
		if (!bWarnedNoNavData)
		{
			bWarnedNoNavData = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': no navigation data in this world — the placement navmesh projection (GDD §3.5, TASK-030) is skipped."),
				*GetNameSafe(this));
		}
		return true;
	}

	// GDD §3.5 via TASK-030: the point must project onto the navmesh within
	// NavProjectionExtent. Elevated non-walkable hits (castle roof) sit far
	// above the ground navmesh — beyond the small vertical extent — so they
	// refuse; ground-level hits (incl. the hollow castle's navmesh'd interior
	// floor, TASK-349 spawn-inside) project within it and validate.
	FNavLocation Projected;
	return NavSys->ProjectPointToNavigation(Point, Projected, NavProjectionExtent);
}

bool ASiegePlayerController::HasBuildingClearance(const FVector& Point, float FootprintRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	// nearest-other-ABuilding rule (GDD §3.5): planar (2D) distance — every
	// placement surface is the arena floor, so Z never contributes; castles
	// are class-disjoint from ABuilding and deliberately NOT part of this rule
	// (and since TASK-349 carry no placement clearance at all — the plinth
	// keep-out that once covered them is retired, spawn-inside is the feature);
	// dying buildings no longer claim clearance (IsBuildingDestroyed flags
	// before Destroy lands).
	//
	// ⭐ TASK-735 (TOWER-§7 spec (4)): the radius is now COMPOSED with the ghost's
	// mesh-derived footprint as a MAX. ⛔ The shipped BuildingClearance = 200.f is
	// UNCHANGED and must stay so — it is every existing building's feel, and
	// retuning it is out of fence. Because max(200, r) == 200 for every r <= 200,
	// EVERY shipped small building keeps its exact current answer, and only a
	// structure genuinely wider than the clearance is separated by its own real
	// size (under the point rule a 2,700 uu tower could legally INTERSECT another
	// building — BuildingClearance could not even reach its own half-extent).
	const double ClearanceSq = FMath::Square(static_cast<double>(EffectiveBuildingClearance(BuildingClearance, FootprintRadius)));
	for (TActorIterator<ABuilding> It(World); It; ++It)
	{
		const ABuilding* Building = *It;
		if (!IsValid(Building) || Building->IsBuildingDestroyed())
		{
			continue;
		}
		if (FVector::DistSquared2D(Building->GetActorLocation(), Point) < ClearanceSq)
		{
			return false;
		}
	}
	return true;
}

bool ASiegePlayerController::IsGroundSlopePlaceable(const FVector& Point, float FootprintRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// no world = nothing to measure against — fail closed like a trace miss
		return false;
	}

	// M4.5 ruling 7 slope gate (TASK-093): straight-down line trace bracketing
	// each sample by ±500 Z (the spec's window; max terrain height is
	// 250, so the surface is always inside it). Channel choice (flagged
	// decision): ECC_Visibility — the SAME channel as the cursor trace
	// (TraceCursorToGround), so the surface that positioned the ghost is the
	// surface whose slope is measured; walkable terrain must block Visibility
	// to be cursor-placeable at all, and SM_ArenaTerrain's
	// Use-Complex-As-Simple collision answers simple traces with real
	// per-triangle normals (bTraceComplex stays false, matching the cursor
	// trace). Pawn capsules ignore Visibility and the ghost's collision is
	// fully disabled (ignored anyway, belt-and-braces below).
	//
	// ⭐⭐ TASK-871 (qa/TASK-816.md W-4 / R-3) — THE TRACE IS NOW A FOOTPRINT
	// PROBE RATHER THAN A POINT PROBE, AND THE REASON IS WORTH STATING PLAINLY:
	// a single trace at the cursor answers for an INFINITELY THIN building. The
	// real one is wide, and since TASK-815 the player can widen it by half again
	// with the wheel — so a structure whose CENTRE sat on a flat crown could
	// overhang a steep flank and PASS. ⭐ The wheel did not create that; it
	// widened an exposure that was already there. TOWER-§7 declared it as a
	// follow-on rather than smuggling it into TASK-735, and this is that
	// follow-on.
	//
	// ⛔ THE SAMPLE SET COMES FROM THE ⛔ SAME FootprintRadius THE TWO CLEARANCE
	// GATES BELOW ARE FED — read ONCE per frame, from the ghost's ⭐ SCALED
	// bounds (STACK-§6), one line above the gate chain. ⛔ Nothing here
	// re-derives the footprint: a second definition is exactly how the ghost the
	// player sees and the footprint the click is validated against drift apart.
	//
	// ⛔ THE QUERY PARAMS AND THE ±Z BRACKET ARE BUILT ⛔ ONCE AND SHARED BY EVERY
	// SAMPLE — five traces with five independently-spelled brackets is five
	// chances for one of them to be edited alone.
	const FVector TraceBracket(0.f, 0.f, 500.f);
	FCollisionQueryParams SlopeQueryParams(SCENE_QUERY_STAT(SiegeboundPlacementSlope), /*bInTraceComplex=*/ false);
	if (GhostActor)
	{
		SlopeQueryParams.AddIgnoredActor(GhostActor);
	}

	const int32 NumSamples = NumPlacementSlopeSamples(FootprintRadius);
	for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	{
		// ⭐ INDEX 0 IS THE CENTRE BY CONSTRUCTION (PlacementSlopeSampleOffset
		// answers the zero vector for it), so the shipped trace is still the
		// FIRST thing this function does and still at exactly the shipped point.
		const bool bIsCentreSample = (SampleIndex == 0);
		const FVector SamplePoint = Point + PlacementSlopeSampleOffset(SampleIndex, FootprintRadius);

		FHitResult SlopeHit;
		const bool bHit = World->LineTraceSingleByChannel(
			SlopeHit, SamplePoint + TraceBracket, SamplePoint - TraceBracket, ECC_Visibility, SlopeQueryParams)
			&& SlopeHit.bBlockingHit;

		if (!bHit)
		{
			if (bIsCentreSample)
			{
				// fail-closed (spec): a point the world cannot answer for is not
				// placeable. Verbose per the task block — the per-frame ghost update
				// would otherwise spam the log from empty space. ⛔ UNCHANGED by
				// TASK-871: this is the shipped refusal, at the shipped point.
				UE_LOG(LogGitClaudeUnrealTest, Verbose,
					TEXT("ASiegePlayerController '%s': placement slope trace missed at (%.0f, %.0f, %.0f) — refusing (fail-closed, GDD §5 M4.5)."),
					*GetNameSafe(this), Point.X, Point.Y, Point.Z);
				return false;
			}

			// ⛔⛔ A CORNER OVER NOTHING IS ⛔ SKIPPED, ⛔ NEVER REFUSED, AND THIS IS
			// THE ⛔ ONE PLACE THE NEW GATE DELIBERATELY DECLINES TO BITE. A corner
			// with no surface under it has no slope to measure; refusing on it would
			// invent a brand-new refusal class out of an ABSENT measurement — the
			// house null-safety law — and would silently make every arena edge and
			// every gap unbuildable with nothing in any log to explain it. ⭐ The
			// centre above is still mandatory, so this can ⛔ never let a placement
			// through that refuses today.
			UE_LOG(LogGitClaudeUnrealTest, Verbose,
				TEXT("ASiegePlayerController '%s': placement slope corner sample %d found no surface at (%.0f, %.0f, %.0f) — SKIPPED, not refused (TASK-871)."),
				*GetNameSafe(this), SampleIndex, SamplePoint.X, SamplePoint.Y, SamplePoint.Z);
			continue;
		}

		if (!IsSurfaceNormalWithinSlopeLimit(SlopeHit.ImpactNormal, MaxPlacementSlopeDegrees))
		{
			UE_LOG(LogGitClaudeUnrealTest, Verbose,
				TEXT("ASiegePlayerController '%s': placement slope sample %d of %d at (%.0f, %.0f, %.0f) reads %.2f deg (limit %.2f, footprint radius %.1f) — refusing (GDD §5 M4.5, TASK-871)."),
				*GetNameSafe(this), SampleIndex, NumSamples, SamplePoint.X, SamplePoint.Y, SamplePoint.Z,
				PlacementSurfaceSlopeDegrees(SlopeHit.ImpactNormal), MaxPlacementSlopeDegrees, FootprintRadius);
			return false;
		}
	}

	return true;
}

bool ASiegePlayerController::HasObstacleClearance(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	// M4.5 ruling 7 obstacle gate (TASK-093, Fab amendment): every tree AND
	// rock instance carries actor tag "Obstacle" (exact FName — set by
	// TASK-095; tag-driven so new obstacle types never need code changes).
	// Planar (2D) distance from the actor's location (trunk-base/rock-base
	// origin per CONVENTIONS), matching the §3.5 building-clearance math.
	// Flagged decision — NO caching: a plain world-actor iteration over the
	// ~20 obstacles (plus the rest of the arena's few-hundred actors) runs
	// only during placement mode, mirroring HasBuildingClearance; a cache
	// would add mid-match staleness risk for no measurable win at this N.
	static const FName ObstacleTagName(TEXT("Obstacle"));
	const double ObstacleClearanceSq = FMath::Square(static_cast<double>(ObstaclePlacementClearance));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const AActor* Candidate = *It;
		if (!IsValid(Candidate) || !Candidate->ActorHasTag(ObstacleTagName))
		{
			continue;
		}
		if (FVector::DistSquared2D(Candidate->GetActorLocation(), Point) < ObstacleClearanceSq)
		{
			return false;
		}
	}
	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  FOOTPRINT-AWARE PLACEMENT (TASK-735 — TOWER-§7, TOWER-§6 row T-6, STACK-§6)
//
//  ⛔⛔ THE FINDING, AND IT IS A MEASUREMENT RATHER THAN A SYMPTOM REPORT: every
//  shipped building-placement gate above is a POINT test, and
//  EPlacementInvalidReason carried NO unit term at all. With a point test the FAR
//  END of a large structure is entirely unvalidated — and a player could drop a
//  building on top of their own army with nothing refusing it.
//
//  ⭐ EVERYTHING BELOW SIZES ITSELF FROM THE GHOST'S MESH. ⛔ There is no literal
//  footprint anywhere in this file: a hardcoded 2700 would have been wrong for
//  the next large building and silently wrong the day SM_WatchTower was
//  re-authored — which it then was, 2,700 → ~750 uu, one day after the rule was
//  written. Mesh-derived sizing is also what makes this safe to add to a shipped
//  path: every existing small building has small bounds and therefore keeps its
//  behaviour BY CONSTRUCTION rather than by promise.
// ═════════════════════════════════════════════════════════════════════════════

float ASiegePlayerController::PlacementFootprintRadiusFromBounds(const FVector& ScaledBoxExtent)
{
	// FVector is double-precision in UE5; the placement family works in float
	// radii, and a cast that overflows becomes inf — which IsFinite then catches.
	const float ExtentX = FMath::Abs(static_cast<float>(ScaledBoxExtent.X));
	const float ExtentY = FMath::Abs(static_cast<float>(ScaledBoxExtent.Y));

	// a mesh with degenerate/NaN bounds cannot size a refusal — answer 0 and let
	// every consumer collapse to its pre-TASK-735 behaviour (degrade OPEN)
	if (!FMath::IsFinite(ExtentX) || !FMath::IsFinite(ExtentY))
	{
		return 0.f;
	}

	// 2D (XY) only: every placement surface is the arena floor, so Z never
	// contributes — the same planar reasoning the two shipped clearances use.
	return FMath::Max(ExtentX, ExtentY);
}

float ASiegePlayerController::EffectiveBuildingClearance(float BaseClearance, float FootprintRadius)
{
	const float SafeBase = (FMath::IsFinite(BaseClearance) && BaseClearance > 0.f) ? BaseClearance : 0.f;
	const float SafeRadius = (FMath::IsFinite(FootprintRadius) && FootprintRadius > 0.f) ? FootprintRadius : 0.f;

	// ⭐ MAX, ⛔ NOT SUM — and the choice is the entire non-regression argument.
	// A sum would push every shipped building apart by its own size and change
	// the feel of content that has been playable for months; the max leaves every
	// footprint <= BuildingClearance answering EXACTLY BuildingClearance, and only
	// bites where the shipped number genuinely cannot reach.
	return FMath::Max(SafeBase, SafeRadius);
}

bool ASiegePlayerController::IsInsidePlacementFootprint(const FVector& CandidatePoint, const FVector& PlacementPoint,
	float FootprintRadius, float BodyRadius, float ExtraClearance)
{
	const float SafeFootprint = (FMath::IsFinite(FootprintRadius) && FootprintRadius > 0.f) ? FootprintRadius : 0.f;
	const float SafeBody = (FMath::IsFinite(BodyRadius) && BodyRadius > 0.f) ? BodyRadius : 0.f;
	const float SafeExtra = (FMath::IsFinite(ExtraClearance) && ExtraClearance > 0.f) ? ExtraClearance : 0.f;

	const double TotalRadius = static_cast<double>(SafeFootprint) + static_cast<double>(SafeBody) + static_cast<double>(SafeExtra);
	if (TotalRadius <= 0.0)
	{
		// nothing can be inside a zero-size footprint — the degrade-open answer,
		// and it makes the predicate self-consistent rather than special-cased
		return false;
	}

	// the shipped idiom, character for character: planar squared distance against
	// a squared radius (HasBuildingClearance / HasObstacleClearance both do this)
	return FVector::DistSquared2D(CandidatePoint, PlacementPoint) < (TotalRadius * TotalRadius);
}

FText ASiegePlayerController::UnitFootprintRefusalText()
{
	// Function-local static (the GetObserverLockoutText precedent at the top of
	// this file): an FText must never construct at module static-init, because
	// localization may not be up yet.
	static const FText UnitFootprintText = NSLOCTEXT("Siegebound", "CardRefused_UnitFootprint", "Your units are in the way");
	return UnitFootprintText;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-813 — THE BLUE UPGRADE STATE'S THREE PURE SEAMS (STACK-§0/§2/§5)
//
//  They sit beside TASK-735's four for the same reason those four exist: the
//  decision they encode is the whole feature, and a decision locked inside a
//  world-bound member function is a decision nobody can test. ⛔ No world, ⛔ no
//  member state, ⛔ no allocation, ⛔ no parameter defaulted (SC-§33).
// ═══════════════════════════════════════════════════════════════════════════════

ASiegePlayerController::EPlacementUpgradeState ASiegePlayerController::ResolvePlacementUpgradeState(
	const ABuilding* HoveredBuilding,
	ETeamId OwnTeam,
	FName PendingCard,
	bool bPendingCardIsBuilding,
	int32 CurrentGold,
	int32 UpgradeCost)
{
	// (1) UNITS AND SPELLS NEVER UPGRADE ANYTHING (J-8: "every Building card with
	// CanScaleFootprint() == true ⇒ ⛔ not units, ⛔ not spells, ⛔ not castles"). The same
	// bPendingIsBuilding flag that exempts unit cards from the slope, obstacle, clearance and
	// footprint gates exempts them from this one — one source of truth for "is this a
	// building card", not a second opinion about it.
	if (!bPendingCardIsBuilding || PendingCard.IsNone())
	{
		return EPlacementUpgradeState::None;
	}

	// (2) NOTHING UPGRADEABLE UNDER THE CURSOR. Empty ground, terrain, an obstacle, a unit,
	// the hero — every one of them Casts to null. ⭐ ACastle is class-disjoint from ABuilding
	// (the same fact HasBuildingClearance is built on), so hovering the castle can never
	// produce a target, and a dying building stops being one the instant it flags destroyed —
	// the shipped IsBuildingDestroyed idiom, reused rather than re-derived.
	if (!IsValid(HoveredBuilding) || HoveredBuilding->IsBuildingDestroyed())
	{
		return EPlacementUpgradeState::None;
	}

	// (3) ⛔ ENEMY BUILDINGS ARE NOT TARGETS (J-7). ⭐ AND NO NEW REFUSAL IS INVENTED FOR THEM:
	// returning None hands the frame back to the shipped gates, which already refuse an enemy
	// building's ground on the §3.5 clearance rule and already say "Too close to another
	// building". The enemy case therefore costs ⛔ zero new code and ⛔ zero new vocabulary.
	if (HoveredBuilding->GetTeamId() != OwnTeam)
	{
		return EPlacementUpgradeState::None;
	}

	// (4) A DIFFERENT CARD ⇒ TODAY'S BEHAVIOUR, UNCHANGED (spec (2)). An ArrowTower in hand
	// does not grow a BombTower: the upgrade replaces "place THIS card", so the thing it grows
	// must be the thing the card would have placed.
	if (HoveredBuilding->GetCardID() != PendingCard)
	{
		return EPlacementUpgradeState::None;
	}

	// (5) ⛔⛔ THE EXCLUSION, AND IT IS THE REASON THIS FUNCTION EXISTS. ⭐ STRUCTURAL: a
	// virtual on ABuilding, so the NEXT building that refuses to grow inherits the protection
	// instead of depending on somebody remembering STACK-§2. ⛔ THERE IS NO CardID STRING
	// COMPARE ON THIS PATH AND THERE MUST NEVER BE ONE.
	//
	// ⛔⛔⛔ IT IS `CanStackHeight()` — THE **HEIGHT (Z)** PREDICATE — AND ⛔ NOT
	// `CanScaleFootprint()`, WHICH IS THE **WHEEL (X/Y)**'s (STACK-§8 cl. 3, the FIRST of the
	// three gates it repointed). ⚠️⚠️ THIS GATE ASKED THE WHEEL'S PREDICATE UNTIL 2026-09-03
	// AND THAT IS THE ⛔ DEFECT 🧑 JONATHAN FILMED: one virtual answered both questions, so the
	// single building in the project that refuses the wheel also refused to be stacked — and
	// gates (3) and (4) above mean that was reachable in normal play by ⛔ exactly one
	// combination, which is precisely the one he tried. ⛔ Restoring the wheel predicate here
	// restores the bug, and it would look like a tidy-up in the diff.
	//
	// ⚠️ ASKED BEFORE THE GOLD CHECK ON PURPOSE: "this can never be stacked" is permanent and
	// actionable; "you are short 4 gold" would be a misleading thing to say about a building
	// that will refuse at any price.
	// ⭐ AND THE CEILING IS DELIBERATELY NOT CONSULTED HERE either — a building AT its height
	// cap still answers true and still turns BLUE, because the click still buys health (J-6,
	// see gate (7)). ⛔ This gate is "may this building grow at all", ⛔ never "how far".
	if (!HoveredBuilding->CanStackHeight())
	{
		return EPlacementUpgradeState::NotStackable;
	}

	// (6) J-5 — ⛔ BLUE MAY NEVER PROMISE A CLICK THE PLAYER CANNOT PAY FOR. The codebase
	// already holds this standard in its own words ("the confirm click refuses on the same
	// flag, so what the player sees is what the click does"), and a blue-that-refuses would be
	// the first place it stopped being true.
	if (CurrentGold < UpgradeCost)
	{
		return EPlacementUpgradeState::Unaffordable;
	}

	// (7) ⭐ BLUE. ⛔ THE HEIGHT CAP IS DELIBERATELY NOT CONSULTED HERE: at the cap the click
	// still buys MaxHP ×1.5 and the count still advances (J-6), so it is still a click worth
	// making — the cap is surfaced as a HUD note at confirm, ⛔ never as a colour and ⛔ never
	// as a refusal.
	return EPlacementUpgradeState::Ready;
}

FText ASiegePlayerController::StackNotStackableRefusalText()
{
	// Function-local static, same reason as UnitFootprintRefusalText above.
	// ⚖️ THE WORDING IS THE RULING: it names the BUILDING (the thing under the cursor, which is
	// what the player must stop hovering) and it says WHY in the player's terms — ⛔ never
	// "WatchTower", because the exclusion is structural and the next climbable building must
	// inherit this message too.
	static const FText NotStackableText = NSLOCTEXT("Siegebound", "CardRefused_NotStackable", "That building cannot be stacked");
	return NotStackableText;
}

FText ASiegePlayerController::StackTargetGoneRefusalText()
{
	// ⭐ STACK-§9(2), site 1 — the target died between the ghost frame and the click. Function-
	// local static, same reason as the refusals above.
	// ⚖️ ITS OWN KEY BECAUSE IT IS ITS OWN CONDITION: this branch reused
	// CardRefused_NotStackable, and that told the player a PERMANENT rule about a building
	// ("that one can never be stacked") when what actually happened was TEMPORARY and not
	// about the rule at all — the siege killed it mid-click. The wording says the transient
	// thing so the player clicks again instead of giving up on the card.
	static const FText TargetGoneText = NSLOCTEXT("Siegebound", "CardRefused_StackTargetGone", "That building is gone");
	return TargetGoneText;
}

FText ASiegePlayerController::StackUpgradeFailedRefusalText()
{
	// ⭐ STACK-§9(2), site 2 — the mutator refused AFTER the spend, and the gold is already
	// refunded by the caller. Today the only way to reach it is the M8 authority guard.
	// ⚖️ ITS OWN KEY, and deliberately NOT phrased as a rule about the building: nothing the
	// player can see or do produced this, so a sentence implying they picked a bad target
	// would send them hunting for a rule that does not exist. ⛔ It also must not promise the
	// refund in words the other refusals do not — every refusal in this game is net-zero, so
	// saying so here would imply the others are not.
	static const FText UpgradeFailedText = NSLOCTEXT("Siegebound", "CardRefused_StackUpgradeFailed", "The upgrade could not be applied");
	return UpgradeFailedText;
}

FText ASiegePlayerController::StackHeightCapNoticeText()
{
	// ⛔ NOT A REFUSAL — the click SUCCEEDED. The wording is deliberately in the past tense of
	// something that happened, and it says HEALTH rather than height, because at the cap the
	// feedback must ⛔ not claim a height gain the upgrade did not deliver (J-6).
	static const FText HeightCapText = NSLOCTEXT("Siegebound", "StackUpgrade_HeightCapped", "Maximum height reached - the upgrade added health only");
	return HeightCapText;
}

FText ASiegePlayerController::WholeSecondsText(float Seconds)
{
	// ⭐⭐ THE ONE PLACE A COUNTDOWN IS SPELLED (TASK-989 item (5)/(5a); J-F24, closed).
	// 📌 He wrote "x amount of SECONDS" ⇒ ⛔ WHOLE SECONDS, ⛔ ROUNDED, ⛔ EVEN PAST 60:
	// "143 seconds", ⛔ never "2 minutes 23 seconds". A countdown is more legible in ONE unit,
	// and keeping the units HERE rather than at each call site is what makes his retune one word.
	//
	// ⛔ IT IS A FORMATTER, ⛔ NOT A MESSAGE BUILDER — one scalar in, one FText out. TASK-991's
	// sun-on-sun refusal calls it TWICE (its "from X to Y"); this row calls it once. A builder
	// spanning both would need an optional second value that is dead half the time, which is the
	// exact dead surface SC-§40 cl. 2 bans (FOG-§10.7 (A)).

	// ⛔ TOTAL BY CONSTRUCTION, in the order that matters. A non-finite input must never reach
	// FMath::RoundToInt (its result would be meaningless, and a "-2147483648 seconds" toast is
	// worse than no message at all). Zero and negative render as a plain zero: the accessor
	// already clamps at 0, and a NEGATIVE countdown must never be spelled out.
	if (!FMath::IsFinite(Seconds) || Seconds <= 0.f)
	{
		return NSLOCTEXT("Siegebound", "SecondsCount_Zero", "0 seconds");
	}

	// ⛔ THE DISPLAY FLOOR, AND IT IS ⛔ NOT A CHANGE TO HIS ROUNDING: it can only fire in the
	// (0, 0.5) band, where rounding alone would print "0 seconds" WHILE the caller is refusing the
	// card for that very window — a message that contradicts its own refusal in the same breath.
	// ⛔ It never invents time out of a zero (that case returned above) and it never touches any
	// value at or above half a second.
	const int32 WholeSeconds = FMath::Max(1, FMath::RoundToInt(Seconds));

	// ⛔ The singular gets its OWN key rather than an ICU plural form: this project ships no
	// localisation and no plural-form string anywhere, so a hand-rolled `|plural(…)` would be the
	// one instance of an untested idiom in the codebase. Two keys are legible to a future
	// translator and cannot mis-parse at runtime.
	if (WholeSeconds == 1)
	{
		return NSLOCTEXT("Siegebound", "SecondsCount_One", "1 second");
	}

	return FText::Format(
		NSLOCTEXT("Siegebound", "SecondsCount_Many", "{0} seconds"),
		FText::AsNumber(WholeSeconds));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-815 — THE PLACEMENT FOOTPRINT WHEEL'S THREE PURE SEAMS (STACK-§4)
//
//  They sit beside TASK-735's four and TASK-813's three for the same reason all
//  seven exist: the decisions they encode ARE the feature, and a decision locked
//  inside a world-bound member function is a decision nobody can test. ⛔ No
//  world, ⛔ no member state, ⛔ no allocation, ⛔ no parameter defaulted (SC-§33).
// ═══════════════════════════════════════════════════════════════════════════════

float ASiegePlayerController::StepPlacementFootprintScale(
	float CurrentScale, int32 NotchDelta, float Step, float MinScale, float MaxScale)
{
	// ⛔ THE TUNABLES ARE SANITISED BEFORE THEY ARE USED, ⛔ not trusted. ClampMin only
	// guards the editor FIELD; a hand-edited .uasset, a Blueprint default or a bad merge
	// can still deliver a NaN or a negative, and this function's whole job is to bound a
	// number that ends up multiplying a building's collision and navmesh footprint.
	//
	// ⭐ THE FALLBACK IS THE IDENTITY, ⛔ never a restated 1.5: if the minimum is
	// unusable the range collapses to 1.0 (today's exact, unscaled behaviour), which is
	// the one answer that cannot invent a size. Restating the ceiling here would be the
	// HIGH-§1 booby trap the day Jonathan retunes it.
	const float SafeMin = (FMath::IsFinite(MinScale) && MinScale > 0.f) ? MinScale : 1.f;

	// A maximum below the minimum is a misconfiguration, ⛔ not an inverted range: the
	// wheel collapses to a single value and goes inert, rather than clamping into a
	// negative-width interval where FMath::Clamp's own behaviour would decide the answer.
	const float SafeMax = (FMath::IsFinite(MaxScale) && MaxScale >= SafeMin) ? MaxScale : SafeMin;

	// A zero/negative/non-finite step is INERT — the honest failure. ⛔ Not "fall back to
	// a default step": a wheel that silently invents its own increment is worse than one
	// that does nothing, because nothing is diagnosable and a wrong number is not.
	const float SafeStep = (FMath::IsFinite(Step) && Step > 0.f) ? Step : 0.f;

	// A non-finite CURRENT scale can only come from a state this function never produced,
	// so it is re-seeded rather than propagated: NaN + anything is NaN, and a NaN scale
	// would reach FTransform and produce a building with no computable bounds at all.
	const float SafeCurrent = FMath::IsFinite(CurrentScale) ? CurrentScale : SafeMin;

	// ⭐ ONE NOTCH = ONE STEP. The multiply is done in double so a pathological NotchDelta
	// cannot overflow the float before the clamp gets to see it; the clamp then pins BOTH
	// endpoints exactly, which is what makes the ×1.5 ceiling bit-exact however the player
	// reaches it.
	const double Stepped = static_cast<double>(SafeCurrent) + static_cast<double>(SafeStep) * static_cast<double>(NotchDelta);
	return FMath::Clamp(static_cast<float>(Stepped), SafeMin, SafeMax);
}

FVector ASiegePlayerController::MakePlacementFootprintScale3D(float FootprintScale)
{
	// Degrade to the unscaled identity rather than passing a NaN or a negative into an
	// FTransform — a negative scale would MIRROR the mesh (inverted normals, inside-out
	// collision) and a NaN would make its bounds uncomputable.
	const float SafeScale = (FMath::IsFinite(FootprintScale) && FootprintScale > 0.f) ? FootprintScale : 1.f;

	// ⛔⛔ X AND Y ONLY — Z IS ALWAYS EXACTLY 1 (STACK-§4 spec (3), J-3). The Z axis is the
	// STACK upgrade's and nothing else may write it: ApplyStackUpgrade recomputes
	// VisualMesh's Z from its AUTHORED baseline every time, so a Z the wheel had smuggled
	// in here would either be erased by the first upgrade or become a second, silent
	// factor in a series that is meant to be a pure function of StackUpgradeCount.
	//
	// ⭐ X == Y IS ALSO LOAD-BEARING, ⛔ not a simplification: the footprint radius is
	// max(|X|, |Y|) of the ghost's rotated world bounds, and the ghost's only rotation is
	// GhostYawOffset's exact quarter turn — which SWAPS X and Y. With X == Y the radius
	// scales by exactly this factor whatever the yaw does; with X != Y the validated
	// footprint would depend on the ghost's facing.
	return FVector(SafeScale, SafeScale, 1.f);
}

bool ASiegePlayerController::CanCardActorScaleFootprint(const UClass* CardActorClass)
{
	if (!CardActorClass)
	{
		// A card whose BP class did not resolve gets an inert wheel — the same
		// degrade-open answer the confirm gives it (RefuseCardPlay + exit), reached here
		// without a second opinion about why the class is missing.
		return false;
	}

	// ⛔⛔ THE EXCLUSION IS STRUCTURAL AND THERE IS ⛔ NO CardID STRING COMPARE ON THIS
	// PATH (STACK-§2 — such a compare is an automatic QA fail). CanScaleFootprint() is the
	// virtual TASK-812 put on ABuilding and AClimbableTower overrides false; asking it
	// here means the WatchTower is excluded from the WHEEL by the same one rule that
	// excludes it from the UPGRADE, and the next climbable building inherits both.
	//
	// ⭐ THE CDO IS THE ORACLE BECAUSE THERE IS NO INSTANCE YET — the wheel runs while the
	// player is still deciding where to put the thing. That is sound rather than
	// convenient: both implementations are const and read ⛔ no instance state, so the
	// class answers for every instance it will ever make.
	//
	// ⛔ Cast, ⛔ not GetDefaultObject<ABuilding>(): the templated form asserts when the
	// class is not an ABuilding, and a UNIT card's class reaching here must answer FALSE
	// rather than trip a check. The cast is also the whole "is this even a building"
	// rule — ⛔ no separate card-type test is invented for the wheel.
	const ABuilding* const Defaults = Cast<ABuilding>(CardActorClass->GetDefaultObject());
	return Defaults != nullptr && Defaults->CanScaleFootprint();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-871 — THE FOUR-CORNER FOOTPRINT SLOPE PROBE'S FOUR PURE SEAMS
//  (qa/TASK-816.md W-4 / ruling R-3 — the follow-on TOWER-§7 DECLARED rather
//  than smuggled into TASK-735)
//
//  They sit beside TASK-735's four, TASK-813's three and TASK-815's three for
//  the same reason all ten exist: IsGroundSlopePlaceable needs a UWorld and a
//  live trace, so it can never be reached headlessly — but the GEOMETRY it
//  traces is the whole feature, and geometry nobody can test is geometry that
//  drifts. ⛔ No world, ⛔ no member state, ⛔ no allocation, ⛔ no parameter
//  defaulted (SC-§33).
// ═══════════════════════════════════════════════════════════════════════════════

int32 ASiegePlayerController::NumPlacementSlopeSamples(float FootprintRadius)
{
	// ⛔ THE ONE-SAMPLE ANSWER IS THE ⛔ SHIPPED DEGRADE, ⛔ NOT A NEW BEHAVIOUR
	// (spec (4)): TryGetPlacementFootprintRadius yields 0 when the ghost, its mesh
	// component or its bounds are missing, and 0 arrives here as "fall back to the
	// single trace" — the exact rule that has shipped since TASK-093. ⚖️ A gate that
	// HARD-REFUSED on a missing measurement would make an art-pipeline hiccup
	// unplayable, which is a far worse failure than the one this task closes.
	const bool bRadiusUsable = FMath::IsFinite(FootprintRadius) && FootprintRadius > 0.f;

	// ⭐ THE FOUR IS THE ⛔ SHAPE OF THE FIX, ⛔ NOT A TUNABLE: a rectangle has four
	// corners. A fifth sample would have no footprint feature to stand on, and a
	// third would leave one diagonal of every building unmeasured. ⛔ Deliberately
	// NOT EditDefaultsOnly — a "number of corners" the designer can retune is a
	// second, silent definition of what a footprint is (the HIGH-§1 booby trap).
	constexpr int32 CentreSamples = 1;
	constexpr int32 CornerSamples = 4;
	return bRadiusUsable ? (CentreSamples + CornerSamples) : CentreSamples;
}

FVector ASiegePlayerController::PlacementSlopeSampleOffset(int32 SampleIndex, float FootprintRadius)
{
	// Degrade to a zero displacement rather than propagating a NaN or a negative
	// into a trace endpoint: a non-finite offset makes the trace's own start and end
	// uncomputable, and the caller's sample count has already collapsed to the
	// centre for exactly these inputs — so this and NumPlacementSlopeSamples agree
	// about "unusable" by construction rather than by coincidence.
	const float SafeRadius = (FMath::IsFinite(FootprintRadius) && FootprintRadius > 0.f) ? FootprintRadius : 0.f;

	// ⭐⭐ EVERY OFFSET IS BUILT OUT OF SafeRadius AND ⛔ NOTHING ELSE (SC-§37).
	// There is ⛔ no transcribed corner here, which is what makes the whole probe
	// scale with the wheel for free: doubling the radius doubles every offset
	// EXACTLY, so a ×1.5 ghost is validated at ×1.5 with ⛔ zero further edits —
	// the same payoff STACK-§6 bought by reading the ghost's SCALED bounds.
	//
	// ⚖️ WHY (±R, ±R) AND ⛔ NOT DISTANCE R ON THE DIAGONAL, flagged rather than
	// assumed: PlacementFootprintRadiusFromBounds returns a ⭐ HALF-EXTENT
	// (max(|X|, |Y|)), ⛔ not a circumradius. The thing it half-describes is a BOX,
	// and a box of half-extent R has its corners at (±R, ±R) — deriving the samples
	// from what the value IS is the reading that cannot drift away from it.
	// ⚠️ THE PRICE, DECLARED: for a round or strongly oblong footprint the corners on
	// the short axis are sampled beyond the mesh, over-refusing by up to ~41% of the
	// radius. That is the mirror of the residual PlacementFootprintRadiusFromBounds
	// already declares in the other direction, and it is accepted HERE and refused
	// THERE on purpose — the unit gate over-refusing costs playability against a
	// dense, mobile hazard in a busy spawn box, while steep terrain is sparse and
	// static, and this is the one gate in the family that already FAILS CLOSED.
	// 🧑 One word flips it; it is one line.
	//
	// ⛔ Z IS ALWAYS EXACTLY 0 — a sample is a PLANAR displacement, and finding the
	// surface is the trace's ±Z bracket's job. An offset carrying a Z would move the
	// bracket instead of the sample and would silently shorten the search window.
	switch (SampleIndex)
	{
	case 1:  return FVector(+SafeRadius, +SafeRadius, 0.f);
	case 2:  return FVector(+SafeRadius, -SafeRadius, 0.f);
	case 3:  return FVector(-SafeRadius, +SafeRadius, 0.f);
	case 4:  return FVector(-SafeRadius, -SafeRadius, 0.f);
	default: return FVector::ZeroVector;
	}
}

float ASiegePlayerController::PlacementSurfaceSlopeDegrees(const FVector& ImpactNormal)
{
	// FVector is double-precision in UE5; the arithmetic stays in double and narrows
	// once, at the end, so the answer is compared in the precision of the float
	// tunable it will be measured against (SC-§37.1).
	const double NormalZ = static_cast<double>(ImpactNormal.Z);
	if (!FMath::IsFinite(NormalZ))
	{
		// ⛔ A NORMAL THE MATHS CANNOT ANSWER FOR READS AS THE STEEPEST SURFACE THERE
		// IS — facing straight DOWN. ⭐ 180 is acos's OWN range ceiling (acos(-1) in
		// degrees), ⛔ not an invented sentinel: it is legible in a log AND refused by
		// every finite limit, which is the fail-closed direction this gate has taken
		// since TASK-093. (A NaN would also refuse, but only through comparison
		// semantics that nobody reading this should have to know.)
		return 180.f;
	}

	// slope = angle between the surface normal and world up (+Z). ImpactNormal
	// is unit-length, so the angle is acos of its Z component (clamped for
	// float safety); a sideways or downward-facing normal reads >= 90° and
	// refuses naturally.
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(NormalZ, -1.0, 1.0))));
}

bool ASiegePlayerController::IsSurfaceNormalWithinSlopeLimit(const FVector& ImpactNormal, float MaxSlopeDegrees)
{
	// ⛔ A NON-FINITE LIMIT REFUSES rather than admitting everything. ClampMin guards
	// only the editor FIELD; a hand-edited .uasset, a Blueprint default or a bad
	// merge can still deliver a NaN, and a NaN limit must ⛔ not silently switch off
	// a shipped gate — the StepPlacementFootprintScale standard, applied to the
	// other end of the same problem.
	if (!FMath::IsFinite(MaxSlopeDegrees))
	{
		return false;
	}

	// ⭐ THE ⛔ ONE PLACE THE SLOPE COMPARISON IS WRITTEN. Both the gate and its test
	// call this, so they can ⛔ never disagree about what "flat enough" means.
	return PlacementSurfaceSlopeDegrees(ImpactNormal) <= MaxSlopeDegrees;
}

bool ASiegePlayerController::TryGetPlacementFootprintRadius(float& OutRadius)
{
	OutRadius = 0.f;

	const UStaticMeshComponent* GhostMesh = GhostActor ? GhostActor->GetStaticMeshComponent() : nullptr;
	const bool bHaveMesh = (GhostMesh != nullptr) && (GhostMesh->GetStaticMesh() != nullptr);

	float DerivedRadius = 0.f;
	if (bHaveMesh)
	{
		// ⭐⭐ THE SCALED BOUNDS, ⛔ NEVER THE LOCAL ONES (STACK-§6 — one word, and
		// it is load-bearing). CalcBounds applies the component's LIVE world
		// transform, so GetScale3D() is already in the answer: the day TASK-815
		// lets the player wheel this ghost to ×1.5 the footprint reads ×1.5 with
		// ⛔ zero further edits, and a 1.5× building can never be validated at
		// 1.0×. ⛔ UStaticMesh::GetBounds() — the LOCAL bounds — would do exactly
		// that, silently, in the class of defect TOWER-§7 exists to close. At the
		// shipped scale of 1 the two agree, so this costs nothing today.
		//
		// ⚠️ Rotation is in the answer too. The ghost's only rotation is the
		// yaw-only GhostYawOffset (-90°, an exact quarter turn: it swaps X and Y
		// and leaves max(|X|,|Y|) untouched), and UpdatePlacementGhost moves the
		// ghost without ever re-rotating it — so the world AABB is exact today.
		// An off-axis yaw would inflate it, which over-refuses and ⛔ never
		// under-refuses; recorded as a follow-on, ⛔ not fixed here.
		const FBoxSphereBounds ScaledBounds = GhostMesh->CalcBounds(GhostMesh->GetComponentTransform());
		DerivedRadius = PlacementFootprintRadiusFromBounds(ScaledBounds.BoxExtent);
	}

	if (DerivedRadius <= 0.f)
	{
		// ⛔ HOUSE NULL-SAFETY LAW: a missing ghost, a missing mesh or degenerate
		// bounds degrade OPEN to the pre-TASK-735 behaviour with ONE warning —
		// they ⛔ never brick placement and ⛔ never refuse on an absent asset. The
		// caller skips the unit gate entirely and composes the building clearance
		// with 0, which is the shipped 200 byte-for-byte. Warn-once is per
		// PLACEMENT SESSION (EnterPlacementMode clears the latch): the missing
		// piece belongs to THIS card's ghost, and this runs every frame.
		if (!bWarnedNoFootprintBounds)
		{
			bWarnedNoFootprintBounds = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': no usable ghost bounds for '%s' (ghost %s, mesh %s) — footprint-aware placement degrades OPEN to the point rules for this card (TASK-735)."),
				*GetNameSafe(this), *PendingCardID.ToString(),
				GhostActor ? TEXT("present") : TEXT("absent"),
				bHaveMesh ? TEXT("present but degenerate") : TEXT("absent"));
		}
		return false;
	}

	OutRadius = DerivedRadius;
	return true;
}

bool ASiegePlayerController::HasUnitClearance(const FVector& Point, float FootprintRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// degrade OPEN, exactly like HasBuildingClearance / HasObstacleClearance
		return true;
	}

	// ⛔ OWN-TEAM ONLY (spec (2)). An ENEMY standing in your spawn box is a
	// different problem with a different answer, and is deliberately not this
	// rule's. No player state = no team to compare against ⇒ degrade OPEN rather
	// than guess a side.
	const ASiegePlayerState* SiegeState = GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		return true;
	}
	const ETeamId OwnTeam = SiegeState->GetTeam();

	// Plain world-actor iteration, no caching — the HasObstacleClearance flagged
	// decision, for the same reasons: this runs only during placement mode, the
	// live-unit count is the same order as the obstacle count that function
	// already walks, and a cache would add mid-match staleness risk for no
	// measurable win. AMinerUnit and ASorcererUnit derive from ASummonedUnit, so
	// this iterator already covers them. ⛔ The HERO, ASiegeGhostPawn and
	// ACommanderNpc are NOT units for this rule — refusing because the player's
	// own body is under the cursor would fight the player rather than protect
	// them. Dead units no longer occupy ground (the shipped IsUnitDead idiom used
	// by the war map's dot survey).
	for (TActorIterator<ASummonedUnit> It(World); It; ++It)
	{
		const ASummonedUnit* Unit = *It;
		if (!IsValid(Unit) || Unit->IsUnitDead() || Unit->GetTeamId() != OwnTeam)
		{
			continue;
		}

		// ⭐ the unit's OWN scaled capsule radius, read off the unit — ⛔ never a
		// literal, and never the CDO's: a unit is refused when the building would
		// materialise THROUGH its body, not when its infinitely-thin centre is
		// covered. A missing capsule contributes 0 rather than refusing.
		const UCapsuleComponent* Capsule = Unit->GetCapsuleComponent();
		const float BodyRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 0.f;

		if (IsInsidePlacementFootprint(Unit->GetActorLocation(), Point, FootprintRadius, BodyRadius, UnitPlacementClearance))
		{
			// ⛔⛔ IT REFUSES. IT ⛔ NEVER MOVES THE UNIT (NAV-§, spec (3)).
			return false;
		}
	}

	return true;
}

//~ IsPointInsideCastlePlinth RETIRED by TASK-349 (CONVENTIONS "Castle 3× HOLLOW"
//~ plinth-retirement law): the M1 keep-out box existed because the SOLID castle's
//~ plinth left walkable navmesh islands nothing could path to. The 3× castle is
//~ hollow with a ground-level navmesh'd interior floor — interior placement is now
//~ the FEATURE, and the keep-out would refuse it. Placement truth = nav projection
//~ + collision + the existing clearances (see the placement-validity v4 comment).

bool ASiegePlayerController::IsPointInOwnSpawnBox(const FVector& Point)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// W1-PREP additions 3 (TASK-261): the player's spawn region is a 2D (XY)
	// square centered on the owned Castle_Blue with half-extent SpawnBoxHalfExtent
	// (7380 since TASK-557 / WR-§2 row 1 — the box covers the 9× castle's walkable
	// interior, so spawn-inside passes this gate by construction; it was 2460 for
	// the 3× castle, TASK-349. ⛔ PAIRED 3 WAYS — ACastle ≡ this ≡ ASiegeBotController;
	// a partial edit is the silent bug the pairing law exists for) — this REPLACES the retired
	// X<=PlacementMaxX half-line gate. The local player is always ETeamId::Blue
	// (team contract). The castle is found with the house team-filtered
	// TActorIterator<ACastle> pattern; the box is centered on the castle
	// regardless of its HP (if Blue's castle is destroyed the match has already
	// ended and placement is disabled).
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		const ACastle* Castle = *It;
		if (!IsValid(Castle) || Castle->GetTeamId() != ETeamId::Blue)
		{
			continue;
		}
		const FVector CastleLocation = Castle->GetActorLocation();
		return FMath::Abs(Point.X - CastleLocation.X) <= SpawnBoxHalfExtent.X &&
			FMath::Abs(Point.Y - CastleLocation.Y) <= SpawnBoxHalfExtent.Y;
	}

	// Null-safe fallback (house null-safety law): no Blue castle in the world =>
	// the box cannot be centered, so refuse rather than crash. Warn ONCE —
	// UpdatePlacementGhost polls this per tick during placement mode, so an
	// unlatched log would spam (mirrors IsPointOnNavmesh's warn-once latch).
	if (!bWarnedMissingSpawnCastle)
	{
		bWarnedMissingSpawnCastle = true;
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': no Blue ACastle found — the spawn box cannot be centered, so placement is refused (W1-PREP additions 3, TASK-261)."),
			*GetNameSafe(this));
	}
	return false;
}

bool ASiegePlayerController::IsPointInCapturedZone(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// W1-PREP additions 3 (TASK-261): the mid capture zone is spawnable for the
	// player only while Blue OWNS it. Find the single CaptureZone_Center via
	// TActorIterator<ACaptureZone> — null-safe if absent (pre-capture-feature
	// behavior: no zone => mid unspawnable). CanTeamSpawnHere folds the box test
	// AND the Blue-owner match (TASK-260 API), so it is the whole capture clause.
	for (TActorIterator<ACaptureZone> It(World); It; ++It)
	{
		const ACaptureZone* Zone = *It;
		if (IsValid(Zone))
		{
			return Zone->CanTeamSpawnHere(ETeamId::Blue, Point);
		}
	}
	return false;
}

void ASiegePlayerController::RefuseCardPlay(FName CardID, const FText& Reason)
{
	// M1 play-refusal hook (WBP_HUD may bind, TASK-011; refusals are also logged
	// at the call site) ...
	OnCardPlayRefused.Broadcast(CardID, Reason);
	// ... plus the M2 combined refusal surface (TASK-023: EVERY refused
	// play/discard reaches OnCardRefused; the TASK-033 hand HUD binds there)
	BroadcastRefusal(Reason);
}

void ASiegePlayerController::BroadcastRefusal(const FText& Reason)
{
	OnCardRefused.Broadcast(Reason.ToString());
}

// ---------------------------------------------------------------------------
// TASK-1270 loop 1 — THE MATCH-START NOTICE MAILBOX (contract on the header
// declaration). Queue holds one notice; Deliver spends it through the shipped
// BroadcastRefusal lane only when OnCardRefused has a listener, clears it
// first, and logs the line the verifier reads. Its two callers are BeginPlay's
// illegal-deck arm (right after queueing) and UCardHandWidget::InitForController
// (right after binding) — delivery lands at whichever comes LATER.
// ---------------------------------------------------------------------------
void ASiegePlayerController::QueueMatchStartNotice(const FText& Notice)
{
	// An empty FText is not a notice: ignoring it keeps "pending" meaning exactly
	// "there is something the player has not been shown yet".
	if (Notice.IsEmpty())
	{
		return;
	}

	PendingMatchStartNotice = Notice;
}

bool ASiegePlayerController::DeliverPendingMatchStartNotice()
{
	if (PendingMatchStartNotice.IsEmpty())
	{
		// nothing held: a legal deck, no save, or already delivered — silent
		return false;
	}

	if (!OnCardRefused.IsBound())
	{
		// ⭐ THE LOOP-0 DEFECT, NOW A HOLD: nobody is listening yet (the hand is
		// spawned by WBP_HUD's first widget Tick, after this controller's
		// BeginPlay and after the load frame's timers). Keep the notice; the
		// hand's InitForController asks again the moment it binds.
		return false;
	}

	// Clear BEFORE broadcasting: a handler that re-enters here finds nothing, so
	// the notice cannot double-fire.
	const FText Notice = PendingMatchStartNotice;
	PendingMatchStartNotice = FText::GetEmpty();

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': HUD notice broadcast (TASK-1270): \"%s\""),
		*GetNameSafe(this), *Notice.ToString());
	BroadcastRefusal(Notice);
	return true;
}

bool ASiegePlayerController::HasPendingMatchStartNotice() const
{
	return !PendingMatchStartNotice.IsEmpty();
}

const FText& ASiegePlayerController::GetPendingMatchStartNotice() const
{
	return PendingMatchStartNotice;
}

UInputAction* ASiegePlayerController::ResolveInputAction(const TObjectPtr<UInputAction>& HardSlot, const TSoftObjectPtr<UInputAction>& SoftAsset, const TCHAR* ActionName, const TCHAR* CreatedInTask) const
{
	if (HardSlot)
	{
		return HardSlot;
	}

	if (UInputAction* Action = SoftAsset.LoadSynchronous())
	{
		return Action;
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("ASiegePlayerController '%s': input action '%s' not resolved (created in %s at %s) — its bindings are skipped; the HUD button (TASK-011) and the polled RMB/Esc cancel still work."),
		*GetNameSafe(this), ActionName, CreatedInTask, *SoftAsset.ToString());
	return nullptr;
}

void ASiegePlayerController::ApplyCursorInputState()
{
	// HandleMatchEnd owns the UI-only end-screen state — never overridden here;
	// HandleMatchReset clears the latch before calling back in
	if (bMatchEnded)
	{
		return;
	}

	// cursor owners compose: placement mode (M1, unchanged), spell targeting mode
	// (M5 TASK-100 — ruling 8: "cursor posture mirrors placement mode"), the
	// group-order pick (TASK-344 — same reused posture), the assistant console
	// (TASK-440 — ONE MORE TERM, never a parallel path), the war map (TASK-563 —
	// ONE MORE TERM AGAIN, for exactly the same reason), and the held IA_UICursor
	// (M2 input ruling) — any one keeps the cursor up. The four card/command
	// cursor modes are mutually exclusive, so at most three owners are ever live
	// (one of them + the console + IA_UICursor).
	//
	// ⚠️ bAssistantConsoleOpen AND bWarMapOpen ARE BOTH FALSE ON EVERY PRE-EXISTING
	// PATH, so this expression evaluates exactly as it did before either term was
	// added — each changes the posture only while its own surface is actually open.
	//
	// ⭐ THIS IS ALSO THE CURSOR/POSTURE GAP TASK-444 FLAGGED (its §5 flag (a),
	// item (iii)) AND TASK-560 RE-FLAGGED FOR THE MAP: "one `|| bWarMapOpen` term in
	// ApplyCursorInputState's bWantCursor composition — that last one is how the law
	// says a new posture owner is added, and it composes with placement / targeting /
	// group-pick / IA_UICursor rather than fighting them." Adding the term HERE,
	// rather than calling SetInputMode from the widget, is what keeps this function
	// the ONLY owner of the match cursor posture (the level-travel law). ⛔ A map that
	// hit-tests markers with no visible cursor and no GameAndUI mode would look
	// perfectly painted and be entirely inert.
	// ⭐ TASK-706 ADDS THE SIXTH TERM — `bControlsHelpOpen` — AND ⛔ CHANGES NOTHING ELSE ON
	// THIS LINE. The five existing owners keep their EXACT shipped precedence; the help overlay
	// is APPENDED to the ladder and ⛔ never re-orders it (`HELP-§5`). Like every term before
	// it, it is FALSE on every pre-existing path, so this expression evaluates exactly as it
	// did before the term existed.
	//
	// ⚖️ AND IT COMPOSES RATHER THAN EXCLUDES, WHICH IS THE `bUICursorHeld` LANE AND NOT THE
	// FOUR-WAY LMB LANE: the overlay hit-tests ONE PANEL and never the world (its backdrop is
	// SelfHitTestInvisible by design), so ⛔ no shipped guard gained a clause against it and
	// every shipped cancel route keeps firing byte-identically underneath it. The reasoning is
	// argued in full at CanOpenControlsHelp's declaration and flagged in the TASK-706 handoff.
	//
	// ⛔ ADDING THE TERM *HERE*, rather than calling SetInputMode from the widget, is what
	// keeps this function the ONLY owner of the match cursor posture (the level-travel law,
	// TASK-074). USiegeControlsHelpWidget contains no SetInputMode and no bShowMouseCursor
	// write at all — grep it.
	const bool bWantCursor = bInPlacementMode || bInTargetingMode || (GroupPickStage != EGroupPickStage::None) || bAssistantConsoleOpen || bWarMapOpen || bControlsHelpOpen || bUICursorHeld;
	bShowMouseCursor = bWantCursor;
	bEnableClickEvents = bWantCursor;

	if (bWantCursor)
	{
		// cursor shown; Game+UI so mouse movement steers the cursor (not the camera)
		// while WASD keeps working. DoNotLock + visible-during-capture keeps the
		// cursor usable across clicks. (Byte-identical to the M1 placement state.)
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		// M1 feel: game-only free-look with the cursor hidden
		SetInputMode(FInputModeGameOnly());
	}
}

bool ASiegePlayerController::CanOpenAssistantConsole() const
{
	// THE CONSOLE HALF OF THE FOUR-WAY MUTUAL EXCLUSION (TASK-440). Each of the
	// three shipped modes carries the mirror clause against bAssistantConsoleOpen,
	// so the exclusion holds in BOTH directions — which is the requirement. A
	// one-directional guard is exactly how two cursor owners end up disagreeing
	// about the input mode, and this controller has paid for that lesson once
	// already (the level-travel input law).
	//
	// The match-ended clause is the shipped Enter*/BeginGroupPick idiom, and here
	// it is load-bearing rather than cosmetic: ApplyCursorInputState EARLY-OUTS
	// while bMatchEnded is latched (HandleMatchEnd owns the UIOnly end screen), so
	// a console opened on the end screen would never receive its cursor posture.
	return !bMatchEnded
		&& !bInPlacementMode
		&& !bInTargetingMode
		&& (GroupPickStage == EGroupPickStage::None);
}

bool ASiegePlayerController::SetAssistantConsoleOpen(bool bOpen)
{
	// RE-GATE ON OPEN so the guard cannot be bypassed by a caller that forgot to
	// ask. CLOSING IS NEVER REFUSED — a close that could fail is a close that can
	// strand the cursor in GameAndUI with no owner willing to release it.
	if (bOpen && !CanOpenAssistantConsole())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': assistant console open refused — another cursor mode is live (placement %d, targeting %d, group pick %d) or the match has ended (%d)."),
			*GetNameSafe(this), bInPlacementMode ? 1 : 0, bInTargetingMode ? 1 : 0,
			(GroupPickStage != EGroupPickStage::None) ? 1 : 0, bMatchEnded ? 1 : 0);
		return false;
	}

	// no-op writes never touch the input mode: re-applying a cursor posture that is
	// already correct is harmless, but a SetInputMode on every keystroke-driven
	// call would be a real per-frame cost and a real source of focus churn.
	if (bAssistantConsoleOpen == bOpen)
	{
		return true;
	}

	bAssistantConsoleOpen = bOpen;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': assistant console %s."),
		*GetNameSafe(this), bOpen ? TEXT("opened") : TEXT("closed"));

	// the ONE cursor-posture owner — never a parallel SetInputMode call
	ApplyCursorInputState();
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// THE OPEN-KEY SEAM (TASK-449)
//
// Both halves already existed and neither is rebuilt here: TASK-440 owns the
// posture (CanOpenAssistantConsole / SetAssistantConsoleOpen / the
// ApplyCursorInputState term / the four mutual-exclusion mirrors) and TASK-444
// owns the widget (CreateAndAddToViewport / OpenConsole / CloseConsole /
// OnConsoleOpenChanged). What follows is the JOIN, and nothing else.
// ─────────────────────────────────────────────────────────────────────────────

void ASiegePlayerController::OnAssistantConsolePressed()
{
	// ── CLOSE FIRST, AND THE CLOSE PATH IS NEVER GATED ──
	// The toggle asks "is it open?" BEFORE it asks "may it open?", because the
	// answer to the second question must never be able to block the first: a close
	// that can be refused is a close that can strand the cursor in GameAndUI with
	// no owner willing to release it (SetAssistantConsoleOpen's own contract).
	// Either half of the pair being live counts as open — they can only disagree
	// through a bug, and if they ever do, the key is what repairs it.
	if (bAssistantConsoleOpen || (AssistantConsoleWidget && AssistantConsoleWidget->IsConsoleOpen()))
	{
		if (AssistantConsoleWidget)
		{
			// Broadcasts OnConsoleOpenChanged(false) -> HandleAssistantConsoleOpenChanged
			// -> SetAssistantConsoleOpen(false). The explicit call below is therefore
			// usually a no-op, and it is kept anyway: it is the one line that
			// guarantees the flag cannot survive the key even if the widget is null,
			// already closed, or a future change drops the broadcast.
			AssistantConsoleWidget->CloseConsole();
		}

		SetAssistantConsoleOpen(false);
		return;
	}

	// ── OPEN ──
	// ⚠️ TASK-563 EXTRACTED THE WHOLE OPEN HALF INTO EnsureAssistantConsoleOpen()
	// AND MOVED IT VERBATIM — not one line of the sequence, its ordering or its log
	// text changed, and this key still ignores the return value exactly as it always
	// did (nothing here ever branched on it). The extraction exists because the war
	// map's marker click needs the SAME sequence and a second copy of a delicate
	// ordering is how the "attach before open" silent failure comes back (the
	// CreateUnitGroup / ApplyArmyWideStance precedent: one implementation, several
	// callers).
	EnsureAssistantConsoleOpen();
}

bool ASiegePlayerController::EnsureAssistantConsoleOpen()
{
	// ⛔ NO GATE WAS ADDED TO THIS FUNCTION AND NONE MAY EVER BE (WR-§5 RULING 5,
	// Jonathan verbatim: "the console still works anywhere"). There is no proximity
	// test, no ACommanderNpc reference and no war-map condition anywhere below — the
	// map is a CALLER of this sequence, never a requirement of it.

	// ── GUARD FIRST, UI SECOND — THE LOAD-BEARING ORDERING ──
	// ⛔ NOTHING IS CREATED AND NOTHING IS SHOWN UNTIL THE POSTURE IS GRANTED.
	// SetAssistantConsoleOpen(true) re-gates on CanOpenAssistantConsole() and
	// returns false while placement mode, spell targeting or a group-order pick
	// owns the cursor, or after match end. On that refusal this function does
	// LITERALLY NOTHING VISIBLE: no widget is constructed, nothing is added to the
	// viewport, no keyboard focus moves, no transcript line is written.
	//
	// ⚖️ A console that appears and THEN discovers it was not permitted is the
	// exact bug this ordering exists to prevent, and it is worse than one that
	// never appears — it seizes keyboard focus on the way past and leaves the
	// player typing into a box that is about to be taken away.
	if (!SetAssistantConsoleOpen(true))
	{
		// SetAssistantConsoleOpen already logged WHICH owner refused. No second
		// log line here: one refusal, one line.
		return false;
	}

	USiegeAssistantConsoleWidget* Console = GetOrCreateAssistantConsoleWidget();
	if (Console == nullptr)
	{
		// Creation failed (CreateAndAddToViewport logged it). ROLL THE POSTURE
		// BACK: a cursor owner with no UI behind it is precisely the soft-lock the
		// posture flag must never be left in.
		SetAssistantConsoleOpen(false);
		return false;
	}

	// ── HAND THE LIVE WIDGET TO THE COMPONENT (TASK-453) ──
	// The console's outbound seam (OnConsoleSubmitted / Confirmed / Cancelled /
	// OpenChanged) is declared "the owning USiegeAssistantComponent binds these",
	// and AttachConsoleWidget is where the component binds them. Creation is LAZY,
	// so this is the only moment that can possibly know a widget exists: a binder
	// at BeginPlay would find null and no-op FOREVER — no error, no log, no crash.
	//
	// ⛔ THIS MUST RUN BEFORE OpenConsole(), NOT AFTER, AND THE DIFFERENCE IS A
	// SILENT FAILURE RATHER THAN A STYLE POINT. OpenConsole() ENDS with
	// OnConsoleOpenChanged.Broadcast(true) (SiegeAssistantConsoleWidget.cpp), and
	// the component subscribes to that delegate INSIDE AttachConsoleWidget. Attach
	// afterwards and the component is not yet listening when the only "console
	// opened" broadcast of this press goes out — the FSM would sit in Idle while
	// the player types into a box it does not know is open, and it would recover
	// only on the SECOND open. Attaching first means the broadcast lands on a live
	// listener the very first time.
	//
	// ⚠️ CALLED ON EVERY OPEN, DELIBERATELY. AttachConsoleWidget is pinned
	// IDEMPOTENT and NULL-SAFE: it drops every binding before it makes it, so
	// repeated calls leave exactly one of each, and it RE-SEEDS the widget from
	// live FSM state on each call. That is what makes close-and-reopen correct BY
	// CONSTRUCTION rather than by the widget happening to persist — whatever might
	// have dropped the link (an FSM-side DetachConsoleWidget, a rebuilt widget, a
	// stale weak pointer), the next successful open re-establishes it. A one-shot
	// attach at the creation site would fire once and have no such recovery.
	//
	// ⚠️ NO COMPONENT ⇒ THE CONSOLE STILL OPENS, INERT. AssistantComponent is a
	// default subobject built in the constructor, so null here means a genuine
	// construction defect rather than a normal state. It must not crash and must
	// not refuse the open: the player gets a console whose Enter goes nowhere,
	// which is exactly how this seam behaved before this task existed.
	if (USiegeAssistantComponent* Assistant = GetAssistantComponent())
	{
		Assistant->AttachConsoleWidget(Console);
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': the assistant console opened with no USiegeAssistantComponent to attach it to — it will accept text but nothing will act on it. No key, card or command is affected."),
			*GetNameSafe(this));
	}

	Console->OpenConsole();

	// ⚠️ OpenConsole() CAN REFUSE, SILENTLY AND BY DESIGN: it returns without
	// showing anything while the console is DISABLED (the assistant fault latch —
	// SetConsoleEnabled(false); SiegeAssistantConsoleWidget.cpp's OpenConsole
	// guard). It has no return value, so ASK THE WIDGET rather than assume the
	// call worked, and release the posture we took. The flag must never describe
	// a console the player cannot see.
	if (!Console->IsConsoleOpen())
	{
		SetAssistantConsoleOpen(false);

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': assistant console did not open (the widget is disabled — a faulted assistant); cursor posture released, nothing shown. No key, card or command is affected."),
			*GetNameSafe(this));
		return false;
	}

	return true;
}

void ASiegePlayerController::HandleAssistantConsoleOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		// ⛔ THE WHOLE REASON THIS BINDING EXISTS. The console can close by routes
		// this controller never sees: Cancel with no confirm prompt up, the fault
		// latch calling SetConsoleEnabled(false), or any later FSM-driven close
		// (TASK-443). A posture flag left stuck TRUE is a cursor soft-locked in
		// GameAndUI with nobody willing to release it — and ApplyCursorInputState
		// is exactly where this project's input bugs have lived before.
		// Closing is never refused, so this cannot fail.
		SetAssistantConsoleOpen(false);
		return;
	}

	// Opened by a route other than the key (a future FSM-driven open — e.g. the
	// deferred-intent path raising a confirm prompt the player must actually see).
	// Take the posture; if the guard refuses it, CLOSE THE WIDGET rather than let
	// the two disagree: an open console with no cursor is unusable, and it would
	// be sitting on top of the very mode that refused it.
	//
	// TERMINATING BY CONSTRUCTION, not by luck: on the key path the flag is
	// already true, so SetAssistantConsoleOpen(true) takes its no-op early-out and
	// returns true. On a refusal, CloseConsole() clears bConsoleOpen BEFORE it
	// re-broadcasts, so the nested call lands on the !bOpen branch above and stops.
	if (!SetAssistantConsoleOpen(true) && AssistantConsoleWidget)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': the assistant console opened without the cursor posture (another cursor mode is live or the match ended) — closing it. The console is never a requirement for any action."),
			*GetNameSafe(this));

		AssistantConsoleWidget->CloseConsole();
	}
}

USiegeAssistantConsoleWidget* ASiegePlayerController::GetOrCreateAssistantConsoleWidget()
{
	if (AssistantConsoleWidget)
	{
		return AssistantConsoleWidget;
	}

	// LAZY BY SPEC (task item 3): the console is created on the first SUCCESSFUL
	// open, never at BeginPlay and never on a refused open — which is only true
	// because the guard above runs first. CreateAndAddToViewport adds it CLOSED
	// (NativeConstruct collapses it), so this call alone puts nothing on screen.
	AssistantConsoleWidget = USiegeAssistantConsoleWidget::CreateAndAddToViewport(this);

	if (AssistantConsoleWidget)
	{
		// THE POSTURE BINDING, TAKEN EXACTLY ONCE (this function is the only
		// creation site and it early-outs when the widget already exists, so the
		// delegate can never be double-bound and fire twice).
		//
		// ⚠️ THIS IS NOT THE ONLY SUBSCRIBER THIS DELEGATE WILL CARRY: TASK-443
		// binds the SAME OnConsoleOpenChanged to the component's
		// NotifyConsoleOpened / NotifyConsoleClosed. Both are wanted — posture is
		// the controller's, the FSM is the component's — and neither may be
		// "consolidated" into the other.
		AssistantConsoleWidget->OnConsoleOpenChanged.AddDynamic(this, &ASiegePlayerController::HandleAssistantConsoleOpenChanged);
	}

	return AssistantConsoleWidget;
}

// ─────────────────────────────────────────────────────────────────────────────
// THE WAR MAP (TASK-563) — input, proximity, and the 30-gold enemy reveal
//
// Three seams meet here and NONE of them is rebuilt: TASK-559 owns the NPC
// (InteractRadius / IsPlayerInRange / EnemyRevealCost / FindCommanderNpcForTeam),
// TASK-560 owns the widget (the C++ painter, the markers, the dots, OnPlacePicked)
// and TASK-561 owns the console's AppendToInput. What follows is the JOIN plus the
// one thing only an authority can do: move gold.
//
// ⭐ THE AIRLOCK PROPERTY OF THIS WHOLE BLOCK (WR-§6): the only thing that ever
// crosses from the map toward the model is ONE PLACE SYMBOL, moved as an opaque
// FName the player then sends himself. ⛔ No coordinate, dot, count, marker rect or
// arena figure is written anywhere near a prompt zone, no Zone builder is opened, no
// grammar rule or `who` shape is added, and no place-symbol literal appears in this
// file. ⇒ This task spends ZERO prompt characters.
// ─────────────────────────────────────────────────────────────────────────────

bool ASiegePlayerController::CanOpenWarMap() const
{
	// THE MAP HALF OF THE FIVE-WAY MUTUAL EXCLUSION (TASK-563) — the SAME clause
	// list as CanOpenAssistantConsole, for the same reasons, and each of the three
	// modes carries the mirror clause against bWarMapOpen so the exclusion holds in
	// BOTH directions. A one-directional guard is exactly how two cursor owners end
	// up disagreeing about the input mode.
	//
	// The match-ended clause is load-bearing rather than cosmetic here for the same
	// reason it is for the console: ApplyCursorInputState EARLY-OUTS while bMatchEnded
	// is latched (HandleMatchEnd owns the UIOnly end screen), so a map opened on the
	// end screen would never receive its cursor posture and its markers would be inert.
	//
	// ⛔ NO bAssistantConsoleOpen CLAUSE, DELIBERATELY — see the declaration comment.
	// The map's whole purpose is to write a symbol into the console's box, so the two
	// are a PAIR; refusing each other would be a deadlock by construction.
	//
	// ⛔ AND NO PROXIMITY CLAUSE — that gate is IsHeroInCommanderRange()'s and is
	// applied by OnWarMapPressed, so the refusal can NAME which of the two reasons
	// fired instead of collapsing them into one silent "no".
	return !bMatchEnded
		&& !bInPlacementMode
		&& !bInTargetingMode
		&& (GroupPickStage == EGroupPickStage::None);
}

bool ASiegePlayerController::SetWarMapOpen(bool bOpen)
{
	// RE-GATE ON OPEN so the guard cannot be bypassed by a caller that forgot to
	// ask. CLOSING IS NEVER REFUSED — a close that could fail is a close that can
	// strand the cursor in GameAndUI with no owner willing to release it. This is
	// SetAssistantConsoleOpen's contract, cloned rather than re-invented.
	if (bOpen && !CanOpenWarMap())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': war map open refused — another cursor mode is live (placement %d, targeting %d, group pick %d) or the match has ended (%d)."),
			*GetNameSafe(this), bInPlacementMode ? 1 : 0, bInTargetingMode ? 1 : 0,
			(GroupPickStage != EGroupPickStage::None) ? 1 : 0, bMatchEnded ? 1 : 0);
		return false;
	}

	// no-op writes never touch the input mode (the console's reasoning verbatim: a
	// SetInputMode on every toggle-driven call is a real cost and a real source of
	// focus churn).
	if (bWarMapOpen == bOpen)
	{
		return true;
	}

	bWarMapOpen = bOpen;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': war map %s."),
		*GetNameSafe(this), bOpen ? TEXT("opened") : TEXT("closed"));

	// the ONE cursor-posture owner — never a parallel SetInputMode call
	ApplyCursorInputState();
	return true;
}

ACommanderNpc* ASiegePlayerController::FindOwnTeamCommanderNpc() const
{
	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	// ⛔ NEVER GUESS A TEAM (the ResolveOrderingTeam doctrine, and the same refusal
	// UWarMapWidget::RefreshAllyDots takes): with no ASiegePlayerState there is no
	// honest answer, and a Blue default on a Red client would gate the map on the
	// ENEMY's commander and price the reveal off the wrong actor. No PS ⇒ no NPC.
	const ASiegePlayerState* const OwningState = GetPlayerState<ASiegePlayerState>();
	if (OwningState == nullptr)
	{
		return nullptr;
	}

	// The finder lives on the class being FOUND (ACommanderNpc::FindCommanderNpcForTeam,
	// the AGoldNode::FindBestMineFor / ACastle::FindNearestCastleForTeam house law),
	// which is what keeps a TActorIterator out of this controller. Deliberately not
	// cached: TASK-562 destroys and re-spawns the NPC on castle destruction and on
	// ResetCastle / Play Again.
	return ACommanderNpc::FindCommanderNpcForTeam(World, OwningState->GetTeam());
}

bool ASiegePlayerController::IsHeroInCommanderRange() const
{
	const APawn* const MyPawn = GetPawn();
	if (MyPawn == nullptr)
	{
		return false;
	}

	const ACommanderNpc* const Npc = FindOwnTeamCommanderNpc();
	if (Npc == nullptr)
	{
		return false;
	}

	// ⛔ THE RADIUS AND THE TEST BOTH BELONG TO THE NPC (WR-§5: InteractRadius is
	// its EditDefaultsOnly feel tunable, and IsPlayerInRange is the 2D-distance
	// comparison that reads it). This controller re-implements neither and invents
	// no distance of its own — a second copy of an interaction radius is a second
	// number to get wrong, and it would silently disagree with the NPC's the first
	// time Jonathan tunes one of them.
	return Npc->IsPlayerInRange(MyPawn->GetActorLocation());
}

void ASiegePlayerController::OnWarMapPressed()
{
	// ── CLOSE FIRST, AND THE CLOSE PATH IS NEVER GATED ──
	// The toggle asks "is it open?" BEFORE it asks "may it open?" — the console
	// key's contract exactly, and for the same reason: a close that can be refused
	// is a close that can strand the cursor in GameAndUI with no owner willing to
	// release it. Either half of the pair being live counts as open; they can only
	// disagree through a bug, and if they ever do, the key is what repairs it.
	if (bWarMapOpen || (WarMapWidget && WarMapWidget->IsMapOpen()))
	{
		CloseWarMap();
		return;
	}

	// ── PROXIMITY BEFORE POSTURE (spec item 2) ──
	// ⚠️ CHECKED BEFORE SetWarMapOpen, DELIBERATELY: an out-of-range press must not
	// touch the cursor at all, so a refusal cannot be felt as a one-frame posture
	// flicker in the middle of a fight.
	//
	// ⛔⛔ THIS GATE IS THE MAP'S AND THE MAP'S ONLY. It is NOT applied to the
	// console — WR-§5 RULING 5, Jonathan verbatim: "the console still works
	// anywhere". EnsureAssistantConsoleOpen / SetAssistantConsoleOpen /
	// CanOpenAssistantConsole / OnAssistantConsolePressed carry no range test, no NPC
	// reference and no map state, and nothing below reaches into them to add one.
	if (!IsHeroInCommanderRange())
	{
		// One HUD line NAMING the reason, on the shipped refusal surface the hand HUD
		// already binds (OnCardRefused via BroadcastRefusal — the "Not enough gold" /
		// "Hero is down" route). The log distinguishes the three causes the player
		// cannot; the player only needs to know where to walk.
		BroadcastRefusal(NSLOCTEXT("Siegebound", "WarMapRefused_OutOfRange", "Walk up to your commander in the castle to use the war map"));

		if (FindOwnTeamCommanderNpc() == nullptr)
		{
			// Warn ONCE: a missing commander is a furnishing problem (TASK-562 spawns
			// it from ACastle::BeginPlay), not something the player can act on, and an
			// unlatched Warning would fire on every M press.
			if (!bWarnedNoCommanderNpc)
			{
				bWarnedNoCommanderNpc = true;
				UE_LOG(LogGitClaudeUnrealTest, Warning,
					TEXT("ASiegePlayerController '%s': war map refused — no OWN-TEAM ACommanderNpc in the world (or no ASiegePlayerState to resolve the team from). The castle furnishing (TASK-562) spawns it at BeginPlay; ⛔ the CONSOLE is UNAFFECTED and still opens anywhere (WR-§5 RULING 5)."),
					*GetNameSafe(this));
			}
			return;
		}

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': war map refused — out of the commander's interact radius (pawn %s)."),
			*GetNameSafe(this), GetPawn() ? TEXT("present") : TEXT("MISSING"));
		return;
	}

	// ── OPEN: GUARD FIRST, UI SECOND — THE LOAD-BEARING ORDERING ──
	// ⛔ NOTHING IS CREATED AND NOTHING IS SHOWN UNTIL THE POSTURE IS GRANTED. On a
	// refusal this function does LITERALLY NOTHING VISIBLE: no widget is constructed,
	// nothing is added to the viewport, no cursor moves. A FULL-SCREEN panel that
	// appears and then discovers it was not permitted is worse than the console's
	// version of the same bug — it covers the battlefield on the way past.
	if (!SetWarMapOpen(true))
	{
		// SetWarMapOpen already logged WHICH owner refused. One refusal, one line.
		return;
	}

	UWarMapWidget* const Map = GetOrCreateWarMapWidget();
	if (Map == nullptr)
	{
		// Creation failed (CreateAndAddToViewport logged it). ROLL THE POSTURE BACK:
		// a cursor owner with no UI behind it is the soft-lock the flag must never be
		// left in.
		SetWarMapOpen(false);
		return;
	}

	Map->OpenMap();

	// ⚠️ ASK THE WIDGET RATHER THAN ASSUME THE CALL WORKED — the console key's
	// post-hoc IsConsoleOpen() check, cloned. OpenMap() has no return value, and the
	// flag must never describe a map the player cannot see.
	if (!Map->IsMapOpen())
	{
		SetWarMapOpen(false);

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': the war map did not open; cursor posture released, nothing shown. No key, card or command is affected."),
			*GetNameSafe(this));
	}
}

void ASiegePlayerController::CloseWarMap()
{
	if (WarMapWidget)
	{
		// ⛔ CloseMap() IS WHAT DISCARDS THE PAID REVEAL, UNCONDITIONALLY (WR-§7 /
		// WR-§9 outcome 4: "red dots vanish the moment the map closes, even one
		// second after paying — that is the mechanic"). It also broadcasts
		// OnMapOpenChanged(false) -> HandleWarMapOpenChanged -> SetWarMapOpen(false),
		// which makes the explicit call below usually a no-op. It is kept anyway: it
		// is the one line that guarantees the posture flag cannot survive a close
		// even if the widget is null, already closed, or a future change drops the
		// broadcast.
		WarMapWidget->CloseMap();
	}

	SetWarMapOpen(false); // never refused
}

void ASiegePlayerController::HandleWarMapOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		// ⛔ THE WHOLE REASON THIS BINDING EXISTS. The map can close by routes this
		// controller never sees — WBP_WarMap's CloseButton (TASK-568 wires it to
		// CloseMap directly), or any later widget-side close. A posture flag left
		// stuck TRUE is a cursor soft-locked in GameAndUI with nobody willing to
		// release it. Closing is never refused, so this cannot fail.
		SetWarMapOpen(false);
		return;
	}

	// Opened by a route other than the key. Take the posture; if the guard refuses
	// it, CLOSE THE WIDGET rather than let the two disagree — a full-screen map with
	// no cursor is unusable, and it would be sitting on top of the very mode that
	// refused it.
	//
	// TERMINATING BY CONSTRUCTION, not by luck: on the key path the flag is already
	// true, so SetWarMapOpen(true) takes its no-op early-out and returns true. On a
	// refusal, CloseMap() clears bMapOpen BEFORE it re-broadcasts, so the nested call
	// lands on the !bOpen branch above and stops.
	if (!SetWarMapOpen(true) && WarMapWidget)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': the war map opened without the cursor posture (another cursor mode is live or the match ended) — closing it. The map is an advantage, never a requirement."),
			*GetNameSafe(this));

		WarMapWidget->CloseMap();
	}
}

void ASiegePlayerController::HandleWarMapPlacePicked(FName PlaceSymbol)
{
	// ⭐ THE CLICK → SYMBOL SEAM (WR-§6). ⛔ Nobody bound OnPlacePicked before this
	// task, so a marker click painted a status line and reached nothing.
	if (PlaceSymbol.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': war-map place pick carried no symbol — ignored."),
			*GetNameSafe(this));
		return;
	}

	// ⛔ THE CONSOLE MUST BE OPENED FIRST, THROUGH THE POSTURE-OWNING PATH, AND THAT
	// IS TASK-561's PINNED CONTRACT RATHER THAN A PREFERENCE: AppendToInput NEVER
	// opens the console and NEVER submits, and a write into a CLOSED console would be
	// destroyed anyway — OpenConsole() calls InputBox->SetText(empty) on EVERY open,
	// so appending first and opening second would silently eat the player's click.
	if (!EnsureAssistantConsoleOpen())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': war-map place pick could not open the assistant console — nothing inserted (the click is lost, not mis-delivered). EnsureAssistantConsoleOpen logged the cause."),
			*GetNameSafe(this));
		return;
	}

	USiegeAssistantConsoleWidget* const Console = GetAssistantConsoleWidget();
	if (Console == nullptr)
	{
		// Unreachable while EnsureAssistantConsoleOpen returned true (it only returns
		// true after resolving a live widget), kept as a loud regression tripwire.
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': the assistant console reports open with no widget — war-map insert dropped."),
			*GetNameSafe(this));
		return;
	}

	// ⛔ THE SYMBOL IS MOVED OPAQUELY AND IS NEVER SPELLED HERE. This file knows
	// nothing about PlaceVocabulary and must not learn: which symbols exist is
	// USiegeAssistantVocabulary's, which are clickable is UWarMapWidget's, and a
	// validation branch here would be a second, drifting copy of a vocabulary this
	// class does not own.
	//
	// ⚠️ AND THE RETURN VALUE IS CHECKED, WHICH TASK-561 ASKED FOR BY NAME: a caller
	// that ignores it gets a refusal it never sees.
	if (!Console->AppendToInput(PlaceSymbol.ToString()))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': AppendToInput refused the war-map symbol (the console is disabled, or its input box is missing) — nothing inserted."),
			*GetNameSafe(this));
		return;
	}

	// ⛔ NOTHING IS SUBMITTED HERE, AND THAT IS THE RULING (WR-§6): the map writes
	// the symbol into the box and THE PLAYER SENDS THE SENTENCE HIMSELF. An
	// auto-submit would turn a click into an order, which is exactly the coordinate-
	// shaped power this whole feature is designed not to hand the model.
	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': war-map marker click inserted one place symbol into the console input box (not submitted)."),
		*GetNameSafe(this));
}

void ASiegePlayerController::HandleWarMapRevealButtonClicked()
{
	// ── THE LOCAL PRE-CHECK IS A MESSAGE, ⛔ NEVER THE GATE ──
	// It exists so the refusal can carry a HUD LINE: refusals decided on the
	// authority reach a REMOTE client's screen through nothing (the server-side copy
	// of that PC broadcasts OnCardRefused to no widget), and WR-§8 fixes this batch
	// at exactly TWO RPCs, so a third "reveal refused" RPC is not available. The
	// shipped PlayHandSlot / DiscardHandSlot pattern does the same thing for the same
	// reason: pre-check affordability locally for the message, let the authority own
	// the actual SpendGold. Gold is DOREPLIFETIME_CONDITION(..., COND_OwnerOnly), so
	// the value read here is the owning client's live balance, not a guess.
	const ASiegePlayerState* const SiegeState = GetPlayerState<ASiegePlayerState>();
	const ACommanderNpc* const Npc = FindOwnTeamCommanderNpc();

	if (SiegeState && Npc && !SiegeState->CanAfford(Npc->GetEnemyRevealCost()))
	{
		// ⛔ NET-ZERO REFUSAL (WR-§7 / spec item 4): refused BEFORE any gold moves —
		// no partial spend, and never a silent no-op.
		BroadcastRefusal(NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': enemy reveal refused locally — cost %d, gold %d. No gold moved, no request sent."),
			*GetNameSafe(this), Npc->GetEnemyRevealCost(), SiegeState->GetGold());
		return;
	}

	// ── ROUTE TO THE AUTHORITY (the RequestPlayAgain shape, TASK-356 doc §4.2) ──
	// Authority (host or standalone) does the work directly — the RPC would resolve
	// Local anyway, and branching explicitly is what makes the client path
	// unambiguous (FINDING-4: a Server RPC body running on the caller means the
	// callspace resolved Local and the host never heard it).
	if (HasAuthority())
	{
		PerformEnemyReveal();
		return;
	}

	UE_LOG(LogSiegeNet, Log,
		TEXT("ASiegePlayerController '%s': enemy reveal — client relay via ServerRequestEnemyReveal (M8 RPC law, WR-§7)."),
		*GetNameSafe(this));
	ServerRequestEnemyReveal();
}

UWarMapWidget* ASiegePlayerController::GetOrCreateWarMapWidget()
{
	if (WarMapWidget)
	{
		return WarMapWidget;
	}

	// LAZY, exactly like the console: created on the first SUCCESSFUL open, never at
	// BeginPlay and never on a refused open (mode exclusion OR the proximity gate) —
	// which is only true because both guards run first. CreateAndAddToViewport adds
	// it CLOSED, so this call alone puts nothing on screen.
	//
	// ⛔⛔ BOTH TRAILING DEFAULTS ARE PASSED EXPLICITLY, AND THAT IS THE `SC-§33`
	// OBLIGATION TASK-560's HANDOFF §4(d) NAMED TO THIS TASK BY NUMBER:
	// CreateAndAddToViewport(OwningController, MapClass = nullptr, ZOrder = 0) — left
	// at its defaults, the map would silently fall back to the bare C++ class the
	// moment TASK-568's WBP_WarMap lands (functional, but with no background panel
	// and no buttons) and would draw at the HUD's own Z order. Neither failure raises
	// an error; both are exactly the silent-default class the law exists to catch.
	//
	// ⚠️ A NULL CLASS IS STILL FULLY SUPPORTED AND IS THE STATE AT THIS COMPILE:
	// WBP_WarMap does not exist until TASK-568, LoadSynchronous returns null, and
	// CreateAndAddToViewport falls back to UWarMapWidget::StaticClass() — which
	// paints and hit-tests every marker and dot on its own. ⛔ Not a degradation to
	// "fix" here.
	UClass* const ResolvedMapClass = WarMapWidgetClass.LoadSynchronous();
	if (ResolvedMapClass == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': war map class '%s' not found (built in TASK-568) — using the C++ UWarMapWidget, which still draws and hit-tests every marker and dot."),
			*GetNameSafe(this), *WarMapWidgetClass.ToString());
	}

	WarMapWidget = UWarMapWidget::CreateAndAddToViewport(
		this,
		TSubclassOf<UWarMapWidget>(ResolvedMapClass),
		WarMapWidgetZOrder);

	if (WarMapWidget)
	{
		// THE THREE BINDINGS, TAKEN EXACTLY ONCE (this function is the only creation
		// site and it early-outs when the widget already exists, so no delegate can
		// be double-bound and fire twice).
		//
		// ⭐ OnPlacePicked IS THE ONE THAT WAS MISSING: it is the widget's ENTIRE
		// outbound surface (TASK-560 handoff §7.4 — "until it binds, a click paints a
		// status line and reaches nothing"), and this controller is the only class
		// that owns BOTH the map instance and the console instance, which is why the
		// binding belongs here rather than inside the widget.
		WarMapWidget->OnMapOpenChanged.AddDynamic(this, &ASiegePlayerController::HandleWarMapOpenChanged);
		WarMapWidget->OnPlacePicked.AddDynamic(this, &ASiegePlayerController::HandleWarMapPlacePicked);
		WarMapWidget->OnRevealButtonClicked.AddDynamic(this, &ASiegePlayerController::HandleWarMapRevealButtonClicked);
	}

	return WarMapWidget;
}

// ─────────────────────────────────────────────────────────────────────────────
// THE TAB CONTROLS OVERLAY (TASK-706; CONVENTIONS `HELP-§1`/`§3`/`§4`/`§5`)
//
// ⛔⛔ EVERY FUNCTION BELOW IS READ-ONLY ON THE WORLD. Grep this block: there is no
// order issued, no group cancelled, no card played, no gold moved and no pause —
// `HELP-§5`, and it is a law rather than an observation.
//
// ⛔⛔ AND `Escape` APPEARS NOWHERE IN IT. The overlay closes on its toggle key and on
// its own Close button, and that is the complete list (`AS-§6 A-2`, a PERMANENT
// Jonathan ruling). Every shipped cancel route keeps firing byte-identically while
// the overlay is open.
// ─────────────────────────────────────────────────────────────────────────────

bool ASiegePlayerController::CanOpenControlsHelp() const
{
	// ⛔ ONE CLAUSE. The match-ended test is the shipped Enter*/BeginGroupPick idiom, and here
	// it is load-bearing rather than cosmetic for exactly CanOpenAssistantConsole's reason:
	// ApplyCursorInputState EARLY-OUTS while bMatchEnded is latched (HandleMatchEnd owns the
	// UIOnly end screen), so an overlay opened on the end screen would never receive its cursor
	// posture.
	//
	// ⚖️⭐ THE THREE ABSENT CLAUSES ARE A RULING, NOT AN OVERSIGHT — see the declaration
	// comment. Jonathan's own named question for this feature is "how to exit the command",
	// and the moment he needs that answer is DURING a group-order pick. Refusing there would
	// make the help screen unavailable in precisely the situation it was built for.
	return !bMatchEnded;
}

bool ASiegePlayerController::SetControlsHelpOpen(bool bOpen)
{
	// RE-GATE ON OPEN so the guard cannot be bypassed by a caller that forgot to ask. CLOSING
	// IS NEVER REFUSED — a close that could fail is a close that can strand the cursor in
	// GameAndUI with no owner willing to release it. SetAssistantConsoleOpen's contract,
	// cloned rather than re-invented.
	if (bOpen && !CanOpenControlsHelp())
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': controls overlay open refused — the match has ended (%d) and HandleMatchEnd owns the end-screen posture."),
			*GetNameSafe(this), bMatchEnded ? 1 : 0);
		return false;
	}

	// no-op writes never touch the input mode (the console's reasoning verbatim: a SetInputMode
	// on every toggle-driven call is a real cost and a real source of focus churn).
	if (bControlsHelpOpen == bOpen)
	{
		return true;
	}

	bControlsHelpOpen = bOpen;

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': controls overlay %s."),
		*GetNameSafe(this), bOpen ? TEXT("opened") : TEXT("closed"));

	// ⛔ THE ONE CURSOR-POSTURE OWNER — never a parallel SetInputMode call, and never one from
	// the widget (TASK-074, the level-travel law).
	ApplyCursorInputState();
	return true;
}

void ASiegePlayerController::OnControlsHelpPressed()
{
	// ── CLOSE FIRST, AND THE CLOSE PATH IS NEVER GATED ──
	// The toggle asks "is it open?" BEFORE it asks "may it open?" — the console and war-map
	// keys' contract exactly, and for the same reason. Either half of the pair being live
	// counts as open; they can only disagree through a bug, and if they ever do, the key is
	// what repairs it.
	if (bControlsHelpOpen || (ControlsHelpWidget && ControlsHelpWidget->IsHelpOpen()))
	{
		CloseControlsHelp();
		return;
	}

	// ── OPEN: GUARD FIRST, UI SECOND — THE LOAD-BEARING ORDERING ──
	// ⛔ NOTHING IS CREATED AND NOTHING IS SHOWN UNTIL THE POSTURE IS GRANTED. On a refusal
	// this function does LITERALLY NOTHING VISIBLE.
	if (!SetControlsHelpOpen(true))
	{
		// SetControlsHelpOpen already logged the reason. One refusal, one line.
		return;
	}

	USiegeControlsHelpWidget* const Help = GetOrCreateControlsHelpWidget();
	if (Help == nullptr)
	{
		// Creation failed (CreateAndAddToViewport logged it). ROLL THE POSTURE BACK: a cursor
		// owner with no UI behind it is the soft-lock the flag must never be left in.
		SetControlsHelpOpen(false);
		return;
	}

	// ⭐ OpenHelp() is what RE-DERIVES EVERY KEY LABEL (`HELP-§1`): it refreshes the keyboard
	// layout once and rebuilds every row from scratch. ⛔ This controller does NOT derive
	// anything itself and does NOT call GetPositionalKey — the widget owns the label lane and
	// this key is a MAPPED action whose remap is already inherited (`KBD-§`).
	Help->OpenHelp();

	// ⚠️ ASK THE WIDGET RATHER THAN ASSUME THE CALL WORKED — the console/map keys' post-hoc
	// check, cloned. The flag must never describe an overlay the player cannot see.
	if (!Help->IsHelpOpen())
	{
		SetControlsHelpOpen(false);

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': the controls overlay did not open; cursor posture released, nothing shown. No key, card or command is affected."),
			*GetNameSafe(this));
	}
}

void ASiegePlayerController::CloseControlsHelp()
{
	if (ControlsHelpWidget)
	{
		// CloseHelp() broadcasts OnHelpOpenChanged(false) -> HandleControlsHelpOpenChanged ->
		// SetControlsHelpOpen(false), which makes the explicit call below usually a no-op. It
		// is kept anyway: it is the one line that guarantees the posture flag cannot survive a
		// close even if the widget is null, already closed, or a future change drops the
		// broadcast.
		ControlsHelpWidget->CloseHelp();
	}

	SetControlsHelpOpen(false); // never refused
}

void ASiegePlayerController::HandleControlsHelpOpenChanged(bool bOpen)
{
	if (!bOpen)
	{
		// ⛔ THE WHOLE REASON THIS BINDING EXISTS. The overlay can close by a route this
		// controller never sees — its own Close button. A posture flag left stuck TRUE is a
		// cursor soft-locked in GameAndUI with nobody willing to release it. Closing is never
		// refused, so this cannot fail.
		SetControlsHelpOpen(false);
		return;
	}

	// Opened by a route other than the key. Take the posture; if the guard refuses it, CLOSE
	// THE WIDGET rather than let the two disagree.
	//
	// TERMINATING BY CONSTRUCTION, not by luck: on the key path the flag is already true, so
	// SetControlsHelpOpen(true) takes its no-op early-out and returns true. On a refusal,
	// CloseHelp() clears bHelpOpen BEFORE it re-broadcasts, so the nested call lands on the
	// !bOpen branch above and stops.
	if (!SetControlsHelpOpen(true) && ControlsHelpWidget)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': the controls overlay opened without the cursor posture (the match has ended) — closing it. A help screen is never a requirement."),
			*GetNameSafe(this));

		ControlsHelpWidget->CloseHelp();
	}
}

USiegeControlsHelpWidget* ASiegePlayerController::GetOrCreateControlsHelpWidget()
{
	if (ControlsHelpWidget)
	{
		return ControlsHelpWidget;
	}

	// LAZY, exactly like the console and the map: created on the first SUCCESSFUL open, never
	// at BeginPlay and never on a refused open — which is only true because the guard runs
	// first. CreateAndAddToViewport adds it CLOSED, so this call alone puts nothing on screen.
	//
	// ⚠️ A NULL CLASS IS FULLY SUPPORTED AND IS THE STATE AT THIS COMPILE: /Game/UI/
	// WBP_ControlsHelp is `HELP-§3`-RESERVED and UNAUTHORED, LoadSynchronous returns null, and
	// CreateAndAddToViewport falls back to USiegeControlsHelpWidget::StaticClass() — which
	// builds its whole tree in C++. ⛔ Not a degradation to "fix" here.
	//
	// ⛔ `SC-§33`: CreateAndAddToViewport defaults NONE of its three parameters, so there is no
	// trailing default that could silently discard the WBP the moment it lands — and all three
	// are passed explicitly here regardless.
	UClass* const ResolvedHelpClass = ControlsHelpWidgetClass.LoadSynchronous();
	if (ResolvedHelpClass == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': controls overlay class '%s' not found (RESERVED and unauthored by HELP-§3) — using the code-authored C++ USiegeControlsHelpWidget, which renders the full list."),
			*GetNameSafe(this), *ControlsHelpWidgetClass.ToString());
	}

	ControlsHelpWidget = USiegeControlsHelpWidget::CreateAndAddToViewport(
		this,
		TSubclassOf<USiegeControlsHelpWidget>(ResolvedHelpClass),
		ControlsHelpWidgetZOrder);

	if (ControlsHelpWidget)
	{
		// THE ONE BINDING, TAKEN EXACTLY ONCE (this function is the only creation site and it
		// early-outs when the widget already exists, so the delegate can never be double-bound).
		// ⛔ OnRowSelected is deliberately NOT bound here: the row click is the WIDGET's own
		// business until TASK-707 gives it a destination, and a controller binding today would
		// be a second owner of a seam that has no consumer.
		ControlsHelpWidget->OnHelpOpenChanged.AddDynamic(this, &ASiegePlayerController::HandleControlsHelpOpenChanged);
	}

	return ControlsHelpWidget;
}

// ─────────────────────────────────────────────────────────────────────────────
// THE 30-GOLD ENEMY REVEAL (WR-§7) — the only gold this batch moves
// ─────────────────────────────────────────────────────────────────────────────

void ASiegePlayerController::PerformEnemyReveal()
{
	// ⛔ EVERY MUTATION SITE CARRIES ITS OWN AUTHORITY GUARD (M8: an unguarded
	// mutation is an automatic QA FAIL). This function is reached from exactly two
	// places — HandleWarMapRevealButtonClicked's HasAuthority() branch and the
	// Server RPC's implementation — and it re-asserts rather than trusting either,
	// because a Server RPC body CAN execute on the caller when the callspace
	// resolves Local (TASK-357 FINDING-4 measured it).
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Error,
			TEXT("ASiegePlayerController '%s': PerformEnemyReveal reached WITHOUT authority — refusing. No gold moved."),
			*GetNameSafe(this));
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	ASiegePlayerState* const SiegeState = GetPlayerState<ASiegePlayerState>();
	if (SiegeState == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': enemy reveal — PlayerState is not an ASiegePlayerState; cannot charge or resolve a team. No gold moved."),
			*GetNameSafe(this));
		return;
	}

	// ⛔ THE COST IS READ OFF THE OWN-TEAM ACommanderNpc AND IS NEVER RE-TYPED HERE
	// (WR-§7: EnemyRevealCost is the NPC's EditDefaultsOnly UPROPERTY, ⛔ never a
	// cards.csv column and ⛔ never a second literal in a second file). No commander
	// ⇒ nothing to price the purchase against ⇒ FAIL CLOSED: refuse with no gold
	// moved, rather than invent a fallback price.
	const ACommanderNpc* const Npc = FindOwnTeamCommanderNpc();
	if (Npc == nullptr)
	{
		if (!bWarnedNoCommanderNpc)
		{
			bWarnedNoCommanderNpc = true;
			UE_LOG(LogGitClaudeUnrealTest, Warning,
				TEXT("ASiegePlayerController '%s': enemy reveal refused on the authority — no OWN-TEAM ACommanderNpc to read EnemyRevealCost from (TASK-562 spawns it from ACastle::BeginPlay). ⛔ No gold moved."),
				*GetNameSafe(this));
		}
		return;
	}

	const int32 RevealCost = Npc->GetEnemyRevealCost();

	// ⛔⛔ NET-ZERO REFUSAL (WR-§7 / spec item 4) — REFUSE BEFORE ANY GOLD MOVES.
	// SpendGold is itself refusal-safe (it changes nothing and broadcasts nothing on
	// an unaffordable cost), so this branch cannot leak; it exists so the LOG names
	// the reason and so the shape reads as the shipped refusal doctrine rather than
	// as a lucky property of the callee.
	if (!SiegeState->SpendGold(RevealCost))
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': enemy reveal refused — cost %d, gold %d. ⛔ No gold moved, no dots sent (net-zero refusal)."),
			*GetNameSafe(this), RevealCost, SiegeState->GetGold());
		return;
	}

	// ⛔⛔ THE GOLD IS NOW SPENT. Everything below must reach ClientReceiveEnemyReveal
	// on every path — a refusal AFTER the spend would be exactly the partial spend
	// WR-§7 forbids, so there is deliberately no early-out from here on. An EMPTY
	// survey (the enemy army really is wiped out) is a legitimate, paid-for answer
	// and is sent as one.

	// ⛔ THE STANDING RULING "THE AI NEVER SPENDS GOLD" IS UNTOUCHED BY THIS LINE,
	// AND IT IS WRITTEN DOWN HERE SO NOBODY LATER "FIXES" IT. The spend above was
	// initiated by the PLAYER'S OWN CLICK on a UI button and is unreachable from
	// every assistant path — no intent, no `who`, no `where`, no executor branch and
	// no confirm step can call this function; USiegeAssistantComponent does not name
	// PerformEnemyReveal, ServerRequestEnemyReveal, SpendGold or ACommanderNpc at
	// all. ⚖️ The player spending gold at a map his AI happens to stand next to is
	// not the AI spending gold, and the distinction is recorded because it will look
	// like a violation to someone reading fast.

	const ETeamId OwnTeam = SiegeState->GetTeam();
	const ETeamId EnemyTeam = (OwnTeam == ETeamId::Blue) ? ETeamId::Red : ETeamId::Blue; // the shipped inline idiom (SiegeAssistantSnapshot.cpp / SiegeBotController.cpp) — there is no shared opposing-team helper to reuse

	// ⛔ WORLD-SPACE (X, Y) IN UNREAL UNITS — ⛔ NOT map UV and ⛔ NOT screen pixels
	// (TASK-560 named this to this task by number: TArray<FVector2D> cannot tell the
	// two apart, and a silent mismatch would put every red dot in a plausible wrong
	// place). The projection has exactly ONE owner, FSiegeWarMapProjection inside the
	// widget, shared byte-for-byte with the ally dots.
	//
	// ⚠️ A FROZEN SNAPSHOT, TAKEN AT PURCHASE (WR-§7's frozen-snapshot ruling, D6's
	// shipping default): these dots do NOT track. Jonathan's own sentence presupposes
	// it — "pay another 30 gold to reveal the NEW locations" only makes sense if the
	// first set went stale. Flipping to live-until-close is one re-push on a timer
	// and changes nothing in the widget.
	//
	// The survey mirrors UWarMapWidget::RefreshAllyDots with the team comparison
	// inverted — same actor classes, same alive tests, same order — so the red dots and
	// the blue dots can never mean different things.
	//
	// ⚠️⚠️ CORRECTED BY TASK-1132: THAT MIRROR IS NO LONGER EXACT, AND THE ONE PLACE IT
	// BREAKS IS DELIBERATE, ONE-SIDED, AND NAMED HERE SO NOBODY "RESTORES SYMMETRY".
	// The UNIT loop below additionally consults FSiegeCombatStatics::IsAgentVisibleTo —
	// the ONE shipped veil predicate (WITCH-§8's raw-scan rule: a TActorIterator that
	// bypasses the acquisition funnel must call the funnel's own predicate, ⛔ never
	// re-read the flag). Without it, a paying player's map published a VEILED enemy
	// unit's exact world (X, Y) at full precision across the whole arena — the very
	// information the 50-gold veil exists to withhold, sold for 30.
	//
	// ⛔ AND THE ASYMMETRY IS CORRECT RATHER THAN AN OVERSIGHT, IN BOTH DIRECTIONS:
	//   • RefreshAllyDots gets NO consult. It filters to the viewer's OWN team, and
	//     IsAgentVisibleTo returns TRUE for every own-team actor (WITCH-§2's fourth
	//     lane: a unit its own player cannot see is a BUG, not a feature; the ally scan
	//     in SiegeBotController.cpp records the identical finding in place). ⇒ a consult
	//     there is dead code that would imply a player can lose sight of his own army.
	//   • The HERO loop below gets NO consult. J-W10 / WITCH-§6: ASummonedUnit is the
	//     ONLY veilable class. ⇒ a consult there is dead code that would imply a hero
	//     can be veiled.
	// ⇒ FALSIFIABLE, and pinned in BOTH directions rather than described: exactly ONE
	// veil consult in this whole function, and exactly ZERO in RefreshAllyDots. Test 36
	// in Tests/SiegeWarMapTest.cpp asserts both numbers; there is no third state.
	//
	// 🧑 DECLARED CONSEQUENCE (TASK-1135, J-W18): 30 gold can now return FEWER dots —
	// possibly ZERO. That is the shipping default, because "undetectable to enemy
	// AI/players" is Jonathan's own sentence and the war map is a player instrument. If
	// he ever rules that the PAID reveal should PIERCE the veil (gold as counterplay),
	// it is a one-line inversion at this exact site. ⛔ There is deliberately no toggle.
	TArray<FVector2D> EnemyWorldXY;

	for (TActorIterator<ASummonedUnit> UnitIt(World); UnitIt; ++UnitIt)
	{
		const ASummonedUnit* const Unit = *UnitIt;

		// ⭐ THE VEIL TERM IS LAST ON PURPOSE, and it is not a style choice: || short-
		// circuits, so the three cheap filters run first and the predicate is only ever
		// asked about a LIVE ENEMY unit — which is the only case where it can answer
		// anything but true. ⚠️ ITS FIRST ARGUMENT IS THE VIEWER'S TEAM (OwnTeam), ⛔ NOT
		// EnemyTeam: IsAgentVisibleTo(ViewerTeam, Candidate) asks "can THIS side see
		// that actor", and passing EnemyTeam would ask whether the veiled unit's own
		// side can see it — always TRUE — making this guard silently dead.
		if (!IsValid(Unit) || Unit->IsUnitDead() || Unit->GetTeamId() != EnemyTeam
			|| !FSiegeCombatStatics::IsAgentVisibleTo(OwnTeam, Unit))
		{
			continue;
		}

		const FVector Location = Unit->GetActorLocation();
		EnemyWorldXY.Emplace(Location.X, Location.Y);
	}

	for (TActorIterator<AHeroCharacter> HeroIt(World); HeroIt; ++HeroIt)
	{
		const AHeroCharacter* const Hero = *HeroIt;
		if (!IsValid(Hero) || Hero->IsDead() || Hero->GetTeamId() != EnemyTeam)
		{
			continue;
		}

		const FVector Location = Hero->GetActorLocation();
		EnemyWorldXY.Emplace(Location.X, Location.Y);
	}

	// ⚠️ DECLARED SCOPE DECISION (SC-§15): buildings, towers and the enemy CASTLE are
	// deliberately NOT dotted. Jonathan's sentence is "reveal all enemy LOCATIONS"
	// about a map that "updates with dots that show ally locations", and the ally
	// side is units + hero — so the paid reveal is the SAME survey for the other
	// team. A castle is at a fixed, already-known place (it is a place SYMBOL), and
	// paying 30 gold to be told where it is would be a worse purchase, not a richer
	// one. ⇒ FLAGGED for Jonathan's playtest rather than assumed in either direction.

	if (EnemyWorldXY.Num() > MaxEnemyRevealDots)
	{
		UE_LOG(LogSiegeNet, Warning,
			TEXT("ASiegePlayerController '%s': enemy reveal produced %d dots, over the %d transport bound — truncating. ⛔ The purchase is NOT refunded (a partial refund would be the partial spend WR-§7 forbids)."),
			*GetNameSafe(this), EnemyWorldXY.Num(), MaxEnemyRevealDots);
		EnemyWorldXY.SetNum(MaxEnemyRevealDots);
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': enemy reveal PURCHASED for %d gold (gold now %d) — %d dots surveyed and sent. Frozen at purchase; cleared the moment the map closes."),
		*GetNameSafe(this), RevealCost, SiegeState->GetGold(), EnemyWorldXY.Num());

	ClientReceiveEnemyReveal(EnemyWorldXY);
}

bool ASiegePlayerController::ServerRequestEnemyReveal_Validate()
{
	// No input payload to validate (the ServerRequestPlayAgain shape) — the
	// implementation's authority guard, own-team commander lookup and SpendGold
	// refusal ARE the intent validation.
	return true;
}

void ASiegePlayerController::ServerRequestEnemyReveal_Implementation()
{
	// The FINDING-4 self-diagnosis, cloned from ServerRequestPlayAgain: a Server
	// RPC's implementation must only ever execute on the authority; if it does not,
	// the callspace resolved Local instead of Remote and the request never reached
	// the host. PerformEnemyReveal re-asserts this itself — belt AND braces, because
	// the thing on the other side of the guard is gold.
	if (!HasAuthority())
	{
		UE_LOG(LogSiegeNet, Error,
			TEXT("ASiegePlayerController '%s': ServerRequestEnemyReveal executed WITHOUT authority — the RPC resolved LOCAL instead of routing to the host; refusing. ⛔ No gold moved."),
			*GetNameSafe(this));
		return;
	}

	// ⚠️ NO SEPARATE SERVER-SIDE PROXIMITY RE-TEST, AND IT IS A DECISION RATHER THAN
	// AN OMISSION (WR-§7, manager-recorded and not to be re-argued): the reveal is an
	// ECONOMY/UI GATE, ⛔ NOT an anti-cheat boundary — on a listen server the client
	// already holds every enemy actor under Tier-B relevancy, so nothing is being
	// concealed and there is nothing to protect. A position re-test would only add a
	// failure mode: a legitimately in-range player whose replicated pawn position lags
	// by a frame would be refused. ⭐ AND THE ECONOMY IS ITS OWN LIMIT — every request
	// costs the full price, so a spammed RPC drains the spammer's own gold and stops.
	PerformEnemyReveal();
}

void ASiegePlayerController::ClientReceiveEnemyReveal_Implementation(const TArray<FVector2D>& EnemyWorldXY)
{
	// ⛔ THIS IS THE PAID SURVEY ARRIVING, AND THE WIDGET IS THE SINK. Nothing here
	// re-checks a cost or a balance: a second economy check would be a second economy
	// rule to get wrong, and the authority has already decided.
	if (WarMapWidget == nullptr)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': enemy reveal arrived with no war map widget — %d dots dropped. The gold was spent on the authority; the player closed the map inside the round trip."),
			*GetNameSafe(this), EnemyWorldXY.Num());
		return;
	}

	if (!bWarMapOpen)
	{
		// ⛔ A CLOSED MAP KEEPS NOTHING (WR-§7: "cleared on close, ALWAYS"). Handing
		// the dots to a collapsed widget would resurrect them on the next open, which
		// is precisely the persistence the mechanic forbids.
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': enemy reveal arrived after the map closed — %d dots discarded (WR-§7: nothing survives a close). Re-opening costs another purchase, by design."),
			*GetNameSafe(this), EnemyWorldXY.Num());
		return;
	}

	WarMapWidget->ReceiveEnemyReveal(EnemyWorldXY);
}
