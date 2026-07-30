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
#include "Siegebound/DeckComponent.h"
#include "Siegebound/DeckLibrary.h" // UDeckLibrary::IsDeckLegal — gate the active saved deck before SetPendingDeckList (M6 TASK-114)
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeCheatManager.h" // TASK-121 — CheatClass complete-type (constructor assignment below)
#include "Siegebound/SiegeDeckSaveGame.h" // USiegeDeckSaveGame — active saved deck source (M6 TASK-114)
#include "Siegebound/SiegeFeedbackLibrary.h" // M7 §6 audio hooks (TASK-179): card play/discard/spell/end-of-match
#include "Siegebound/SiegeGameMode.h" // M8 (TASK-356): RequestPlayAgain resolves the server GameMode (doc §4.2)
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeSessionSubsystem.h" // LogSiegeNet (CONVENTIONS M8)
#include "Siegebound/SiegeSpawnConstants.h"
#include "Siegebound/SpellLibrary.h"
#include "Siegebound/SummonedUnit.h"
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
		if (USiegeDeckSaveGame* DeckSave = Cast<USiegeDeckSaveGame>(
				UGameplayStatics::LoadGameFromSlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex)))
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
		// M5 spell routing (GDD §3.11, TASK-100): GoldSteal resolves INSTANTLY on
		// play — no reticle for a global effect (manager ruling 7; recorded §3.5
		// deviation, CardType stays Spell). Every other SpellEffect enters
		// TARGETING mode, placement mode's sibling: the card leaves the hand only
		// at LMB CONFIRM (M2 law), so cancel costs nothing. Affordability was
		// pre-checked above; SiegeState is non-null here (checked above).
		if (Row->SpellEffect == ESpellEffect::GoldSteal)
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

void ASiegePlayerController::OnCmdAttackPressed()
{
	// ATTACK (T) is immediate — no ground pick. Ignored after match end (the
	// input-ignore pattern). Abandon any mid-flight group pick first (no group
	// forms from the abort — CancelGroupPick is no-op-safe), then apply the
	// TASK-344 RELEASE law — a global stance replaces EVERY group order — and
	// only then latch Attack.
	if (bMatchEnded)
	{
		return;
	}
	CancelGroupPick();
	ClearAllUnitGroups();
	SetUnitCommand(ESiegeUnitCommand::Attack);
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

void ASiegePlayerController::OnCmdDefendPressed()
{
	// DEFEND (E) is immediate — mirror of OnCmdAttackPressed (pick abort +
	// group release, then the stance latch).
	if (bMatchEnded)
	{
		return;
	}
	CancelGroupPick();
	ClearAllUnitGroups();
	SetUnitCommand(ESiegeUnitCommand::Defend);
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

	// UI-only input for the end screen (GDD §3.9)
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	FInputModeUIOnly InputMode;
	if (VictoryWidget)
	{
		InputMode.SetWidgetToFocus(VictoryWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

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
		const FTransform SpawnTransform(FRotator::ZeroRotator, PlacementLocation);
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

void ASiegePlayerController::UpdatePlacementGhost()
{
	FHitResult Hit;
	const bool bGroundHit = TraceCursorToGround(Hit);
	if (bGroundHit)
	{
		PlacementLocation = Hit.ImpactPoint;
	}

	// Placement validity v4 (GDD §3.5 TASK-030 + GDD §5 M4.5 TASK-093 +
	// W1-PREP additions 3 TASK-261 + Castle 3× HOLLOW TASK-349), in cost order:
	// (1) ground hit inside the player's spawn box — a 2D square around the owned
	//     Castle_Blue with half-extent SpawnBoxHalfExtent (2460: covers the
	//     castle's walkable interior, TASK-349) — OR inside a Blue-owned capture
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
	// (4) Building cards only (M4.5 ruling 7): ground slope at the candidate
	//     <= MaxPlacementSlopeDegrees, measured by a straight-down trace
	//     (fail-closed on a miss) — hill flanks refuse, crowns (<=10°) pass;
	// (5) Building cards only (M4.5 ruling 7, Fab amendment): >=
	//     ObstaclePlacementClearance (2D) from every "Obstacle"-tagged actor
	//     (trees AND rocks);
	// (6) Building cards only: >= BuildingClearance from the nearest other
	//     ABuilding (§3.5; castles are NOT buildings for this rule).
	// The FIRST failing rule is recorded so the confirm click can name its
	// reason ("Too steep" / "Too close to obstacles" get their own messages).
	PlacementInvalidReason = EPlacementInvalidReason::Point;
	bool bValid = bGroundHit && (IsPointInOwnSpawnBox(Hit.ImpactPoint) || IsPointInCapturedZone(Hit.ImpactPoint));
	if (bValid)
	{
		bValid = IsPointOnNavmesh(PlacementLocation);
	}
	if (bValid && bPendingIsBuilding && !IsGroundSlopePlaceable(PlacementLocation))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Slope;
	}
	if (bValid && bPendingIsBuilding && !HasObstacleClearance(PlacementLocation))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Obstacle;
	}
	if (bValid && bPendingIsBuilding && !HasBuildingClearance(PlacementLocation))
	{
		bValid = false;
		PlacementInvalidReason = EPlacementInvalidReason::Clearance;
	}
	if (bValid)
	{
		PlacementInvalidReason = EPlacementInvalidReason::None;
	}
	bPlacementValid = bValid;

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
		GhostMID->SetVectorParameterValue(GhostColorParamName, bPlacementValid ? ValidGhostColor : InvalidGhostColor);
	}
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

	// GoldSteal never targets (ruling 7): a direct call with a GoldSteal card
	// reroutes to the instant resolve — hand-less on this path (INDEX_NONE skips
	// the draw step; PlayHandSlot routes hand plays before ever reaching here).
	// Placed BEFORE the hero-dead gate so BOTH GoldSteal entries behave alike:
	// instants are not hero-gated (the Masons/instant-play precedent).
	if (Row->SpellEffect == ESpellEffect::GoldSteal)
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

	BroadcastCommandPrompt(FString::Printf(
		TEXT("%s: circle your units — scroll to resize, LMB confirm, RMB/Esc cancel"),
		Type == ESiegeGroupCommandType::Ambush ? TEXT("AMBUSH") : TEXT("HOLD")));
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

	const TCHAR* TypeLabel = (GroupPickType == ESiegeGroupCommandType::Ambush) ? TEXT("AMBUSH") : TEXT("HOLD");

	switch (GroupPickStage)
	{
	case EGroupPickStage::Select:
	{
		// stage 1 — SELECT: every group-eligible unit (Standard + Blue + alive +
		// unfrozen, IsGroupCommandEligible — the new public API; Profile is
		// private) inside the circle (2D) at confirm joins. EMPTY = refuse-and-
		// stay + HUD reason (the law): a different circle can succeed, so the
		// stage survives the refusal.
		GroupPickSelectedMembers.Reset();
		if (UWorld* World = GetWorld())
		{
			const float SelectRadiusSq = FMath::Square(GroupPickRadius);
			for (TActorIterator<ASummonedUnit> It(World); It; ++It)
			{
				ASummonedUnit* Unit = *It;
				if (IsValid(Unit) && Unit->IsGroupCommandEligible()
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
		// stage 3 — FINAL confirm: build the group, steal re-selected units from
		// older groups, compute + push the per-unit sunflower stations ONCE, and
		// transfer the dropped Position + this Attack circle to the group as
		// persistent markers (delivers the TASK-276-deferred hold marker).
		FSiegeUnitGroup NewGroup;
		NewGroup.GroupId = NextUnitGroupId++;
		NewGroup.Type = GroupPickType;
		NewGroup.PositionCenter = GroupPickPositionCenter;
		NewGroup.PositionRadius = GroupPickPositionRadius;
		NewGroup.AttackCenter = GroupPickLocation;
		NewGroup.AttackRadius = GroupPickRadius;
		NewGroup.PositionMarkerDecal = GroupPickPositionDecal;
		NewGroup.AttackMarkerDecal = GroupPickActiveDecal;

		// re-filter the stage-1 capture: members may have died during the flow
		for (const TWeakObjectPtr<ASummonedUnit>& Member : GroupPickSelectedMembers)
		{
			const ASummonedUnit* Unit = Member.Get();
			if (Unit && !Unit->IsUnitDead())
			{
				NewGroup.Members.Add(Member);
			}
		}

		if (NewGroup.Members.Num() == 0)
		{
			// every selected unit died mid-flow: no group to form — refuse with a
			// HUD reason and tear the whole pick down (nothing was transferred, so
			// CancelGroupPick destroys all three circles).
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': %s final confirm refused — every selected unit died during the pick."),
				*GetNameSafe(this), TypeLabel);
			BroadcastRefusal(NSLOCTEXT("Siegebound", "GroupPickRefused_UnitsDied", "Selected units are gone"));
			CancelGroupPick();
			return;
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

		// the pick is DONE: the Position + Attack circles now belong to the group
		// as its persistent markers — null the scratch refs FIRST so the shared
		// CancelGroupPick teardown below leaves them standing and destroys only
		// the SELECT circle.
		GroupPickPositionDecal = nullptr;
		GroupPickActiveDecal = nullptr;

		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': %s group %d formed — %d unit(s), position (%.0f, %.0f) r=%.0f, attack (%.0f, %.0f) r=%.0f (TASK-344)."),
			*GetNameSafe(this), TypeLabel, NewGroupId, MemberCount,
			GroupPickPositionCenter.X, GroupPickPositionCenter.Y, GroupPickPositionRadius,
			GroupPickLocation.X, GroupPickLocation.Y, GroupPickRadius);

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
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': unit group %d emptied — group and markers removed (TASK-344 prune)."),
				*GetNameSafe(this), Group.GroupId);
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
	// GoldSteal resolves INSTANTLY on play (M5 ruling 7 — no reticle for a
	// global effect; recorded §3.5 deviation, CardType stays Spell). The
	// ruling-8 confirm shape applied at PLAY time: deduct THEN resolve,
	// refusal-safe — a resolver false FULLY refunds (§3.0 net-zero) and keeps
	// the card in hand.
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

	// TargetPoint is only the VFX anchor for a global GoldSteal (SpellLibrary
	// contract) — the hero's feet when we have one, world origin otherwise.
	// TASK-236 call-site flag: only GoldSteal reaches this instant path (its
	// delivery is neither GroundCircle nor HeroLine — the aim-point semantics
	// shift does not apply here).
	const FVector AnchorPoint = IsValid(Hero) ? Hero->GetActorLocation() : FVector::ZeroVector;

	if (!USpellLibrary::ResolveSpell(World, CardID, Row, CasterTeam, AnchorPoint))
	{
		// FULL refund (§3.0) — refusal-safe by construction: e.g. Sandbox mode
		// has no Red economy to steal from, so the play refuses net-zero with
		// the card still in hand. (A 0-gold victim, by contrast, RESOLVES for
		// min(GoldSteal, 0) = 0 — the spell is spent, per the resolver contract.)
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
	// redraw. INDEX_NONE = a direct hand-less EnterTargetingMode(GoldSteal) call.
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

bool ASiegePlayerController::HasBuildingClearance(const FVector& Point) const
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
	const double ClearanceSq = FMath::Square(static_cast<double>(BuildingClearance));
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

bool ASiegePlayerController::IsGroundSlopePlaceable(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// no world = nothing to measure against — fail closed like a trace miss
		return false;
	}

	// M4.5 ruling 7 slope gate (TASK-093): straight-down line trace bracketing
	// the candidate point by ±500 Z (the spec's window; max terrain height is
	// 250, so the surface is always inside it). Channel choice (flagged
	// decision): ECC_Visibility — the SAME channel as the cursor trace
	// (TraceCursorToGround), so the surface that positioned the ghost is the
	// surface whose slope is measured; walkable terrain must block Visibility
	// to be cursor-placeable at all, and SM_ArenaTerrain's
	// Use-Complex-As-Simple collision answers simple traces with real
	// per-triangle normals (bTraceComplex stays false, matching the cursor
	// trace). Pawn capsules ignore Visibility and the ghost's collision is
	// fully disabled (ignored anyway, belt-and-braces below).
	const FVector TraceStart = Point + FVector(0.f, 0.f, 500.f);
	const FVector TraceEnd = Point - FVector(0.f, 0.f, 500.f);
	FCollisionQueryParams SlopeQueryParams(SCENE_QUERY_STAT(SiegeboundPlacementSlope), /*bInTraceComplex=*/ false);
	if (GhostActor)
	{
		SlopeQueryParams.AddIgnoredActor(GhostActor);
	}

	FHitResult SlopeHit;
	if (!World->LineTraceSingleByChannel(SlopeHit, TraceStart, TraceEnd, ECC_Visibility, SlopeQueryParams) || !SlopeHit.bBlockingHit)
	{
		// fail-closed (spec): a point the world cannot answer for is not
		// placeable. Verbose per the task block — the per-frame ghost update
		// would otherwise spam the log from empty space.
		UE_LOG(LogGitClaudeUnrealTest, Verbose,
			TEXT("ASiegePlayerController '%s': placement slope trace missed at (%.0f, %.0f, %.0f) — refusing (fail-closed, GDD §5 M4.5)."),
			*GetNameSafe(this), Point.X, Point.Y, Point.Z);
		return false;
	}

	// slope = angle between the surface normal and world up (+Z). ImpactNormal
	// is unit-length, so the angle is acos of its Z component (clamped for
	// float safety); a sideways or downward-facing normal reads >= 90° and
	// refuses naturally.
	const double SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(SlopeHit.ImpactNormal.Z, -1.0, 1.0)));
	return SlopeDegrees <= MaxPlacementSlopeDegrees;
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
	// (2460 since TASK-349 — the box covers the 3× castle's walkable interior, so
	// spawn-inside passes this gate by construction) — this REPLACES the retired
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
	// group-order pick (TASK-344 — same reused posture), and the held IA_UICursor
	// (M2 input ruling) — any one keeps the cursor up. The three card/command
	// cursor modes are mutually exclusive, so at most two owners are ever live
	// (one of them + IA_UICursor).
	const bool bWantCursor = bInPlacementMode || bInTargetingMode || (GroupPickStage != EGroupPickStage::None) || bUICursorHeld;
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
