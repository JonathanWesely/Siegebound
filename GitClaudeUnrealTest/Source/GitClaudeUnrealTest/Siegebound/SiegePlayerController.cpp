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
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/DeckLibrary.h" // UDeckLibrary::IsDeckLegal — gate the active saved deck before SetPendingDeckList (M6 TASK-114)
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeCheatManager.h" // TASK-121 — CheatClass complete-type (constructor assignment below)
#include "Siegebound/SiegeDeckSaveGame.h" // USiegeDeckSaveGame — active saved deck source (M6 TASK-114)
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SiegeSpawnConstants.h"
#include "Siegebound/SpellLibrary.h"
#include "Siegebound/SummonedUnit.h"

ASiegePlayerController::ASiegePlayerController()
{
	// the per-frame cursor-to-ground trace runs in PlayerTick, which early-outs
	// whenever placement mode is inactive
	PrimaryActorTick.bCanEverTick = true;

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
	}
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

	bMatchEnded = false;

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

	// back to M1 game-only free-look (both cursor owners are clear by now)
	ApplyCursorInputState();
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
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — no ground hit, enemy half (X > %.0f), off the navmesh, or on a castle plinth (GDD §3.5, TASK-030)."),
				*GetNameSafe(this), *PendingCardID.ToString(), PlacementMaxX);
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

	// Placement validity v3 (GDD §3.5 TASK-030 + GDD §5 M4.5 TASK-093),
	// evaluated in cost order:
	// (1) ground hit on the owner's half (X <= 0; centerline per CONVENTIONS —
	//     the M1 rule, unchanged);
	// (2) the point projects onto the navmesh within NavProjectionExtent —
	//     closes the M1 "castle roof is placeable" carry-over (roof and
	//     plinth-top hits sit far above any navmesh);
	// (3) outside every castle's plinth keep-out box — belt-and-braces so a
	//     walkable navmesh island on the plinth rim can never validate a point
	//     nothing can path to;
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
	bool bValid = bGroundHit && Hit.ImpactPoint.X <= PlacementMaxX;
	if (bValid)
	{
		bValid = IsPointOnNavmesh(PlacementLocation) && !IsPointInsideCastlePlinth(PlacementLocation);
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
	// no surface under the cursor (sky / outside the world): refuse, spend
	// NOTHING, STAY in mode — a different point can succeed (the placement
	// invalid-click law). This is the ONLY positional refusal in targeting
	// mode: spells land ANYWHERE a surface answers the trace — enemy half
	// included, no navmesh requirement (M5 ruling 8, §3.5).
	if (!bTargetingSurfaceValid)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': spell confirm refused for '%s' — no surface under the cursor."),
			*GetNameSafe(this), *TargetingCardID.ToString());
		RefuseCardPlay(TargetingCardID, NSLOCTEXT("Siegebound", "CardRefused_NoTarget", "No target under cursor"));
		return;
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
		// Fireball 300 / FrostNova 350 / Lightning 400 / BattleCry 400) so the
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
	// contract) — the hero's feet when we have one, world origin otherwise
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
	// NavProjectionExtent. Castle-roof/plinth-top hits sit ~90+ units above
	// the ground navmesh — beyond the small vertical extent — so they refuse.
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
	// (the plinth keep-out covers them, TASK-027 handoff); dying buildings no
	// longer claim clearance (IsBuildingDestroyed flags before Destroy lands).
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
	// only during placement mode, mirroring HasBuildingClearance /
	// IsPointInsideCastlePlinth; a cache would add mid-match staleness risk
	// for no measurable win at this N.
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

bool ASiegePlayerController::IsPointInsideCastlePlinth(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// M1 carry-over: SM_Castle's plinth collision (~814x820, ~90 high) blocks
	// standing/pathing across its whole footprint, and navmesh generation can
	// leave walkable islands on the exposed plinth rim — the 2D keep-out box
	// guarantees plinth points always read invalid. Castle HP is irrelevant:
	// the collision stands as long as the actor does (ResetCastle revives it).
	for (TActorIterator<ACastle> It(World); It; ++It)
	{
		const ACastle* Castle = *It;
		if (!IsValid(Castle))
		{
			continue;
		}
		const FVector CastleLocation = Castle->GetActorLocation();
		if (FMath::Abs(Point.X - CastleLocation.X) <= CastlePlinthClearance &&
			FMath::Abs(Point.Y - CastleLocation.Y) <= CastlePlinthClearance)
		{
			return true;
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

	// three cursor owners compose: placement mode (M1, unchanged), targeting
	// mode (M5 TASK-100 — ruling 8: "cursor posture mirrors placement mode", so
	// it joins the composition rather than inventing a new posture), and the
	// held IA_UICursor (M2 input ruling) — any one keeps the cursor up. The two
	// card modes are mutually exclusive, so at most two owners are ever live.
	const bool bWantCursor = bInPlacementMode || bInTargetingMode || bUICursorHeld;
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
