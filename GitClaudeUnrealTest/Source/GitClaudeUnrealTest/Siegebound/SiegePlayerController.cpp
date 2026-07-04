// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GitClaudeUnrealTest.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/Castle.h"
#include "Siegebound/DeckComponent.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"

namespace
{
	/** Small lift above the traced ground point so the spawned capsule never starts interpenetrating the floor. */
	constexpr float SpawnGroundClearance = 2.f;

	/** ACharacter's default capsule half-height — fallback when the unit class CDO has no capsule to measure. */
	constexpr float DefaultCapsuleHalfHeight = 88.f;
}

ASiegePlayerController::ASiegePlayerController()
{
	// the per-frame cursor-to-ground trace runs in PlayerTick, which early-outs
	// whenever placement mode is inactive
	PrimaryActorTick.bCanEverTick = true;

	// deck & hand model (GDD §3.4, TASK-022) — subobject name is a spec contract
	DeckComponent = CreateDefaultSubobject<UDeckComponent>(TEXT("DeckComponent"));

	// content contract (TASK-007/023 names blocks) — everything soft, resolved
	// null-safe at runtime; the assets are built by parallel tasks
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));                                  // TASK-008
	HUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));                                    // TASK-011
	VictoryScreenClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C")));            // TASK-011
	GhostFallbackMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));                    // engine asset, read-only (TASK-030 ghost fallback)
	GhostMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Ghost.M_Ghost")));                   // TASK-012
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

	// Deck build at match start (GDD §3.4). This controller owns the timing —
	// the component never self-builds (TASK-022 flagged decision 12). Built
	// BEFORE any widget below so a hand HUD created at BeginPlay (TASK-033)
	// seeds from an already-dealt hand (CONVENTIONS seed-then-bind law).
	if (DeckComponent)
	{
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
		// targeting mode arrives with the spell system (GDD §3.11, M5)
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — Spell cards arrive in M5 (GDD §3.11)."),
			*GetNameSafe(this), Slot, *CardID.ToString());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_SpellsM5", "Spells are not available yet"));
		break;

	case ECardType::HeroUpgrade:
	case ECardType::Utility:
	default:
		// Instant-resolving types arrive with Set II (GDD §3.10, M4)
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': hand slot %d ('%s') refused — Instant card types arrive in M4 (GDD §3.10)."),
			*GetNameSafe(this), Slot, *CardID.ToString());
		RefuseCardPlay(CardID, NSLOCTEXT("Siegebound", "CardRefused_InstantsM4", "Instant cards are not available yet"));
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
	PendingHandSlot = INDEX_NONE; // hand plays consume it in TryConfirmPlacement BEFORE this exit

	DestroyPlacementGhost();

	// restores game-only free-look — unless IA_UICursor is still held, in which
	// case the cursor stays up for the HUD (the two cursor owners compose)
	ApplyCursorInputState();
}

void ASiegePlayerController::HandleMatchEnd(ETeamId Winner)
{
	// exit placement FIRST (spec + qa-note): destroys the ghost and releases the
	// melee suppression before the input mode switches to UI-only
	ExitPlacementMode();

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
	// (the §3.5 building-clearance rule gets its own).
	if (!bPlacementValid)
	{
		if (PlacementInvalidReason == EPlacementInvalidReason::Clearance)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — within %.0f units of another building (GDD §3.5 clearance)."),
				*GetNameSafe(this), *PendingCardID.ToString(), BuildingClearance);
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_BuildingClearance", "Too close to another building"));
		}
		else
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': placement click refused for '%s' — no ground hit, enemy half (X > %.0f), off the navmesh, or on a castle plinth (GDD §3.5, TASK-030)."),
				*GetNameSafe(this), *PendingCardID.ToString(), PlacementMaxX);
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_InvalidPoint", "Invalid placement location"));
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
	if (PendingCardType == ECardType::Building)
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
		// Unit/Economy path — the M1 TASK-007 flow, class now resolved per
		// CardType. Lift the spawn so the capsule stands on the traced ground.
		float CapsuleHalfHeight = DefaultCapsuleHalfHeight;
		if (const ASummonedUnit* UnitCDO = ActorClass->GetDefaultObject<ASummonedUnit>())
		{
			if (const UCapsuleComponent* Capsule = UnitCDO->GetCapsuleComponent())
			{
				CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
		}
		const FTransform SpawnTransform(FRotator::ZeroRotator, PlacementLocation + FVector(0.f, 0.f, CapsuleHalfHeight + SpawnGroundClearance));

		// deferred spawn (TASK-004 handoff: preferred path) so InitUnit binds the
		// card BEFORE BeginPlay reads DT_Cards — never a mis-teamed first state check
		ASummonedUnit* Unit = World->SpawnActorDeferred<ASummonedUnit>(
			ActorClass,
			SpawnTransform,
			/*Owner=*/ this,
			/*Instigator=*/ GetPawn(),
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Unit)
		{
			UE_LOG(LogGitClaudeUnrealTest, Error,
				TEXT("ASiegePlayerController '%s': SpawnActorDeferred failed for '%s' (%s) — no gold spent, staying in placement mode."),
				*GetNameSafe(this), *PendingCardID.ToString(), *GetNameSafe(ActorClass));
			return;
		}

		// gold is the last gate before commit: a refusal destroys the half-spawned
		// actor, so exactly Cost is deducted if and only if a unit appears (§3.5)
		if (!SiegeState->SpendGold(PendingCost))
		{
			Unit->Destroy();
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ASiegePlayerController '%s': SpendGold(%d) refused at confirm for '%s' — staying in placement mode."),
				*GetNameSafe(this), PendingCost, *PendingCardID.ToString());
			RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_CantAfford", "Not enough gold"));
			return;
		}

		Unit->InitUnit(Team, PendingCardID);
		Unit->FinishSpawning(SpawnTransform);
		Spawned = Unit;
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

	// Placement validity v2 (GDD §3.5, TASK-030), evaluated in cost order:
	// (1) ground hit on the owner's half (X <= 0; centerline per CONVENTIONS —
	//     the M1 rule, unchanged);
	// (2) the point projects onto the navmesh within NavProjectionExtent —
	//     closes the M1 "castle roof is placeable" carry-over (roof and
	//     plinth-top hits sit far above any navmesh);
	// (3) outside every castle's plinth keep-out box — belt-and-braces so a
	//     walkable navmesh island on the plinth rim can never validate a point
	//     nothing can path to;
	// (4) Building cards only: >= BuildingClearance from the nearest other
	//     ABuilding (§3.5; castles are NOT buildings for this rule).
	// The failing rule is recorded so the confirm click can name its reason.
	PlacementInvalidReason = EPlacementInvalidReason::Point;
	bool bValid = bGroundHit && Hit.ImpactPoint.X <= PlacementMaxX;
	if (bValid)
	{
		bValid = IsPointOnNavmesh(PlacementLocation) && !IsPointInsideCastlePlinth(PlacementLocation);
	}
	if (bValid && PendingCardType == ECardType::Building && !HasBuildingClearance(PlacementLocation))
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
			// SM_Footman's origin is feet-center (TASK-014), so the ghost sits on the ground point
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
	// CONVENTIONS composed soft-class paths, selected by CardType (TASK-030):
	// Unit/Economy -> /Game/Blueprints/Units/BP_Unit_<CardID>   (TASK-010/034)
	// Building     -> /Game/Blueprints/Buildings/BP_Building_<CardID> (TASK-035)
	// Missing/incompatible = nullptr; the caller refuses the play with NO gold
	// spent (composed soft-class law — the M1 meshless fallback is retired).
	const FString CardName = CardID.ToString();
	FString ClassPath;
	UClass* RequiredBase = nullptr;
	const TCHAR* CreatedInTask = TEXT("");
	switch (CardType)
	{
	case ECardType::Unit:
	case ECardType::Economy:
		ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);
		RequiredBase = ASummonedUnit::StaticClass();
		CreatedInTask = TEXT("TASK-010/034");
		break;

	case ECardType::Building:
		ClassPath = FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C"), *CardName, *CardName);
		RequiredBase = ABuilding::StaticClass();
		CreatedInTask = TEXT("TASK-035");
		break;

	default:
		// PlayHandSlot's type switch keeps every other CardType out of
		// placement mode — reaching this is a caller regression
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

	// two cursor owners compose: placement mode (M1, unchanged) and the held
	// IA_UICursor (M2 input ruling) — either one keeps the cursor up
	const bool bWantCursor = bInPlacementMode || bUICursorHeld;
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
