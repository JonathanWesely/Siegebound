// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/Castle.h"

#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Siegebound/CastleHealthBarWidget.h"

ACastle::ACastle()
{
	// Pure event-driven objective — nothing to tick.
	PrimaryActorTick.bCanEverTick = false;

	CastleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CastleMesh"));
	SetRootComponent(CastleMesh);
	// Explicit blocking profile (QA TASK-002 WARN, pre-approved fix): TASK-004's unit
	// targeting/blocking contract must not rest on the engine's implicit default.
	CastleMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Overhead HP bar (playtest R1 finding 2, TASK-018). Screen space so it reads at
	// any camera angle/distance; relative Z +1050 clears the 900-tall castle mesh
	// (TASK-013). The widget CLASS is soft-resolved at BeginPlay (WBP_CastleHealthBar
	// arrives in TASK-019); the bare component always exists and draws nothing.
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(CastleMesh);
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidget->SetDrawSize(FVector2D(256.0f, 32.0f));
	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 1050.0f));
	// UI-only component: never collides, never blocks traces (placement cursor
	// trace TASK-007, unit acquisition TASK-004).
	HPBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Content contract (TASKBOARD TASK-002 names block / CONVENTIONS.md). These assets are
	// produced in parallel (TASK-013 mesh, TASK-012 materials) and are resolved null-safe
	// in OnConstruction — a missing asset must never crash.
	CastleMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/Meshes/SM_Castle.SM_Castle")));
	TeamMaterialBlue = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue")));
	TeamMaterialRed = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Instances/MI_TeamColor_Red.MI_TeamColor_Red")));
	// TASK-018 names block: widget asset built in TASK-019 — resolved null-safe at BeginPlay.
	HPBarWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_CastleHealthBar.WBP_CastleHealthBar_C")));
}

void ACastle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTeamVisuals();
}

void ACastle::BeginPlay()
{
	Super::BeginPlay();

	CurrentHP = MaxHP;
	bDestroyed = false;

	// Seed broadcast (TASK-018 spec): listeners that bound before BeginPlay
	// (level BP, game framework) start from the boot values without polling.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	InitHPBarWidget();
}

void ACastle::InitHPBarWidget()
{
	if (!HPBarWidget)
	{
		return;
	}

	// WBP_CastleHealthBar is built in TASK-019 and may not exist yet — a missing
	// class is a SILENT no-op per spec (LoadSynchronous returns nullptr for unset
	// paths and absent assets alike; the bare component simply draws nothing).
	UClass* LoadedWidgetClass = HPBarWidgetClass.LoadSynchronous();
	if (!LoadedWidgetClass)
	{
		return;
	}

	// Post-BeginPlay SetWidgetClass triggers the component's InitWidget, creating
	// the user widget instance (components have begun play — Super::BeginPlay ran
	// before this is called).
	HPBarWidget->SetWidgetClass(LoadedWidgetClass);

	// Seed-then-bind is the widget's own job (qa/TASK-005-report.md major 2):
	// InitForCastle pushes the current values FIRST, then binds OnCastleHPChanged.
	// A widget of some other class (mis-authored TASK-019 asset) is skipped, not a crash.
	if (UCastleHealthBarWidget* HealthBar = Cast<UCastleHealthBarWidget>(HPBarWidget->GetWidget()))
	{
		HealthBar->InitForCastle(this);
	}
}

void ACastle::ApplyTeamVisuals()
{
	if (!CastleMesh)
	{
		return;
	}

	// LoadSynchronous() returns nullptr for unset paths and not-yet-imported assets alike;
	// in either case we simply skip the assignment (never crash per spec).
	if (UStaticMesh* Mesh = CastleMeshAsset.LoadSynchronous())
	{
		if (CastleMesh->GetStaticMesh() != Mesh)
		{
			CastleMesh->SetStaticMesh(Mesh);
		}
	}

	const TSoftObjectPtr<UMaterialInterface>& TeamMaterial = (Team == ETeamId::Red) ? TeamMaterialRed : TeamMaterialBlue;
	if (UMaterialInterface* Material = TeamMaterial.LoadSynchronous())
	{
		// SM_Castle has a single material slot (TASK-013 spec); the same mesh serves both teams.
		CastleMesh->SetMaterial(0, Material);
	}
}

float ACastle::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// A destroyed castle absorbs nothing further; the destroyed event can never re-fire.
	if (bDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	// No friendly fire (GDD §3.0): ignore damage whose instigator is on our own team.
	ETeamId InstigatorTeam = ETeamId::Blue;
	if (TryGetInstigatorTeam(EventInstigator, DamageCauser, InstigatorTeam) && InstigatorTeam == Team)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (ActualDamage <= 0.0f)
	{
		return 0.0f;
	}

	// M1: only melee exists and it applies at 100%.
	// TODO(M2): scale by attack profile — projectiles apply at 50% vs castles,
	// Siege-profile attacks at 200% (GDD §3.9/§4).
	CurrentHP = FMath::Max(CurrentHP - ActualDamage, 0.0f);

	// Actual HP change -> broadcast (TASK-018). Ignored friendly fire and hits on a
	// destroyed castle returned above WITHOUT touching CurrentHP, so a broadcast here
	// always reports a real change (CurrentHP > 0 and ActualDamage > 0 guarantee the
	// clamp lowered the value). Fired BEFORE HandleDestroyed so listeners see the
	// 0-HP value before the destroyed event hides the bar.
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);

	if (CurrentHP <= 0.0f)
	{
		HandleDestroyed();
	}

	return ActualDamage;
}

void ACastle::HandleDestroyed()
{
	// Single-fire guard: cumulative overkill, duplicate calls, or re-entrancy
	// during the broadcast can never fire the event twice.
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;

	// Hide the mesh and stop colliding (GDD §3.9) BEFORE broadcasting, so any
	// listener querying this castle during the event already sees it destroyed.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// Screen-space widget components do NOT follow actor hidden-in-game state
	// (the viewport layer checks component visibility only) — hide explicitly
	// (TASK-018: bar disappears with the castle).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(false, /*bPropagateToChildren=*/true);
	}

	OnCastleDestroyed.Broadcast(this, Team);
}

void ACastle::ResetCastle()
{
	// Play Again (GDD §3.9): back to full HP, visible, solid, and armed to fire again.
	bDestroyed = false;
	CurrentHP = MaxHP;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// Counterpart of the HandleDestroyed hide — the bar returns with the castle (TASK-018).
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(true, /*bPropagateToChildren=*/true);
	}

	// Reset-path broadcast (TASK-018 + CONVENTIONS delegate rules): the bar refills
	// to MaxHP/MaxHP. Unconditional by spec — resetting an undamaged castle still
	// notifies (reset is an explicit reset event, not a suppressed no-op mutation).
	OnCastleHPChanged.Broadcast(CurrentHP, MaxHP);
}

bool ACastle::TryGetInstigatorTeam(AController* EventInstigator, AActor* DamageCauser, ETeamId& OutTeam)
{
	// 1) The instigating controller's pawn (hero melee reports its controller).
	if (EventInstigator)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(EventInstigator->GetPawn()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// 2) The damage causer itself (summoned units apply damage directly).
	if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser))
	{
		OutTeam = Agent->GetTeamId();
		return true;
	}

	// 3) The causer's instigator pawn (covers projectiles once M2 adds them).
	if (DamageCauser)
	{
		if (const ITeamAgent* Agent = Cast<ITeamAgent>(DamageCauser->GetInstigator()))
		{
			OutTeam = Agent->GetTeamId();
			return true;
		}
	}

	// No team could be resolved (e.g. world/kill-Z damage) — caller applies the damage.
	return false;
}
