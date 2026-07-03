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
#include "GitClaudeUnrealTest.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/CardRow.h"
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

	// content contract (TASK-007 names block) — everything soft, resolved
	// null-safe at runtime; the assets are built by parallel tasks
	CardTableAsset = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Data/DT_Cards.DT_Cards")));                                  // TASK-008
	HUDWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));                                    // TASK-011
	VictoryScreenClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C")));            // TASK-011
	GhostMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_Footman.SM_Footman")));                           // TASK-014
	GhostMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_Ghost.M_Ghost")));                   // TASK-012
	Card1ActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Card1.IA_Card1")));                     // TASK-009
	CancelPlaceActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_CancelPlace.IA_CancelPlace")));   // TASK-009
}

void ASiegePlayerController::BeginPlay()
{
	Super::BeginPlay();

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

	Super::EndPlay(EndPlayReason);
}

void ASiegePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// slots resolve hard reference first (a BP subclass may assign them), then
	// fall back to the exact TASK-009 asset paths; both are optional
	Card1Action = ResolveInputAction(Card1Action, Card1ActionAsset, TEXT("IA_Card1"));
	CancelPlaceAction = ResolveInputAction(CancelPlaceAction, CancelPlaceActionAsset, TEXT("IA_CancelPlace"));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// play the M1 card slot (keyboard "1" via IMC_Hero, TASK-009)
		if (Card1Action)
		{
			EnhancedInputComponent->BindAction(Card1Action, ETriggerEvent::Started, this, &ASiegePlayerController::OnCard1Pressed);
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
	EnterPlacementMode(Card1CardID);
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
		return;
	}

	bInPlacementMode = true;
	bPlacementValid = false;
	PendingCardID = CardID;
	PendingCost = Row->Cost;

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

	ApplyPlacementInputState(true);
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
	PendingCardID = NAME_None;
	PendingCost = 0;

	DestroyPlacementGhost();
	ApplyPlacementInputState(false);
}

void ASiegePlayerController::HandleMatchEnd(ETeamId Winner)
{
	// exit placement FIRST (spec + qa-note): destroys the ghost and releases the
	// melee suppression before the input mode switches to UI-only
	ExitPlacementMode();

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
	bMatchEnded = false;

	// idempotent with WBP_VictoryScreen's own RemoveFromParent (TASK-011)
	if (VictoryWidget)
	{
		VictoryWidget->RemoveFromParent();
		VictoryWidget = nullptr;
	}

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	SetInputMode(FInputModeGameOnly());
}

void ASiegePlayerController::TryConfirmPlacement()
{
	// invalid click: refuse, spend NOTHING, STAY in placement mode (GDD §3.5)
	if (!bPlacementValid)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("ASiegePlayerController '%s': placement click refused for '%s' — no ground hit or X > %.0f (Red half; Blue placement half is X <= 0)."),
			*GetNameSafe(this), *PendingCardID.ToString(), PlacementMaxX);
		RefuseCardPlay(PendingCardID, NSLOCTEXT("Siegebound", "CardRefused_InvalidPoint", "Invalid placement location"));
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

	UWorld* World = GetWorld();
	UClass* UnitClass = ResolveUnitClass(PendingCardID);
	if (!World || !UnitClass)
	{
		return;
	}

	// lift the spawn so the capsule stands on the traced ground point
	float CapsuleHalfHeight = DefaultCapsuleHalfHeight;
	if (const ASummonedUnit* UnitCDO = UnitClass->GetDefaultObject<ASummonedUnit>())
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
		UnitClass,
		SpawnTransform,
		/*Owner=*/ this,
		/*Instigator=*/ GetPawn(),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("ASiegePlayerController '%s': SpawnActorDeferred failed for '%s' (%s) — no gold spent, staying in placement mode."),
			*GetNameSafe(this), *PendingCardID.ToString(), *GetNameSafe(UnitClass));
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

	// local player is always Blue (CONVENTIONS team contract); mirror the hero's
	// team when we have one so the two can never disagree
	const ETeamId Team = IsValid(PlacementHero) ? PlacementHero->GetTeamId() : ETeamId::Blue;
	Unit->InitUnit(Team, PendingCardID);
	Unit->FinishSpawning(SpawnTransform);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("ASiegePlayerController '%s': played card '%s' for %d gold — spawned '%s' at (%.0f, %.0f, %.0f)."),
		*GetNameSafe(this), *PendingCardID.ToString(), PendingCost, *GetNameSafe(Unit),
		SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y, SpawnTransform.GetLocation().Z);

	// confirm exit path — also releases the melee suppression (qa-note)
	ExitPlacementMode();
}

void ASiegePlayerController::UpdatePlacementGhost()
{
	FHitResult Hit;
	const bool bGroundHit = TraceCursorToGround(Hit);

	// valid = ground hit AND on the Blue half (X <= 0; centerline X=0 per CONVENTIONS)
	bPlacementValid = bGroundHit && Hit.ImpactPoint.X <= PlacementMaxX;
	if (bGroundHit)
	{
		PlacementLocation = Hit.ImpactPoint;
	}

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

	// every ghost asset is optional — a missing mesh/material degrades the preview
	// but MUST NOT block the placement logic (TASK-007 spec)
	if (UStaticMesh* Mesh = GhostMeshAsset.LoadSynchronous())
	{
		GhostMesh->SetStaticMesh(Mesh); // TODO(M2): pick the ghost mesh per card instead of the fixed SM_Footman
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("ASiegePlayerController '%s': ghost mesh '%s' not found (TASK-014) — placement runs with an invisible ghost."),
			*GetNameSafe(this), *GhostMeshAsset.ToString());
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

UClass* ASiegePlayerController::ResolveUnitClass(FName CardID) const
{
	// CONVENTIONS blueprint-subclass pattern: /Game/Blueprints/Units/BP_Unit_<CardID>
	// (Footman -> /Game/Blueprints/Units/BP_Unit_Footman.BP_Unit_Footman_C, TASK-010)
	const FString CardName = CardID.ToString();
	const FString ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);

	UClass* UnitClass = TSoftClassPtr<ASummonedUnit>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (UnitClass && UnitClass->IsChildOf(ASummonedUnit::StaticClass()))
	{
		return UnitClass;
	}

	UE_LOG(LogGitClaudeUnrealTest, Warning,
		TEXT("ASiegePlayerController '%s': unit class '%s' not found (built in TASK-010) — falling back to ASummonedUnit (logic runs, no visual mesh)."),
		*GetNameSafe(this), *ClassPath);
	return ASummonedUnit::StaticClass();
}

void ASiegePlayerController::RefuseCardPlay(FName CardID, const FText& Reason)
{
	// HUD-message hook (TASK-011 may bind; refusals are also logged at the call site)
	OnCardPlayRefused.Broadcast(CardID, Reason);
}

UInputAction* ASiegePlayerController::ResolveInputAction(const TObjectPtr<UInputAction>& HardSlot, const TSoftObjectPtr<UInputAction>& SoftAsset, const TCHAR* ActionName) const
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
		TEXT("ASiegePlayerController '%s': input action '%s' not resolved (created in TASK-009 at %s) — the HUD button (TASK-011) and the polled RMB/Esc cancel still work."),
		*GetNameSafe(this), ActionName, *SoftAsset.ToString());
	return nullptr;
}

void ASiegePlayerController::ApplyPlacementInputState(bool bEnteringPlacement)
{
	if (bEnteringPlacement)
	{
		// cursor shown; Game+UI so mouse movement steers the cursor (not the camera)
		// while WASD keeps working. DoNotLock + visible-during-capture keeps the
		// cursor usable across the confirm click.
		bShowMouseCursor = true;
		bEnableClickEvents = true;

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		bShowMouseCursor = false;
		bEnableClickEvents = false;
		SetInputMode(FInputModeGameOnly());
	}
}
