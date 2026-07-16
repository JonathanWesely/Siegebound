// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/DamageNumberActor.h"

#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GitClaudeUnrealTest.h"
#include "Siegebound/DamageNumberWidget.h"

namespace
{
	/** WBP_DamageNumber runtime class path (the _C generated-class suffix, ACastle::HPBarWidgetClass precedent). */
	const TCHAR* DamageNumberWidgetClassPath = TEXT("/Game/UI/WBP_DamageNumber.WBP_DamageNumber_C");

	/** Compact screen-space face (reads at ~150 px, mirrors the health-bar DrawSize discipline). */
	const FVector2D DamageNumberDrawSize(120.f, 48.f);
}

int32 ADamageNumberActor::LiveCount = 0;

ADamageNumberActor::ADamageNumberActor()
{
	// Rise + fade + self-destruct is genuinely per-tick for its short life.
	PrimaryActorTick.bCanEverTick = true;

	NumberWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NumberWidget"));
	SetRootComponent(NumberWidget);
	NumberWidget->SetWidgetSpace(EWidgetSpace::Screen);
	NumberWidget->SetDrawSize(DamageNumberDrawSize);
	// UI-only: never collides or blocks traces (placement cursor / acquisition all query ECC_Pawn).
	NumberWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NumberWidget->SetGenerateOverlapEvents(false);

	// Soft class default (CONVENTIONS names block); resolved null-safe at BeginPlay.
	NumberWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(DamageNumberWidgetClassPath));
}

void ADamageNumberActor::Spawn(const UObject* WorldContextObject, float Amount, const FVector& WorldLocation, const FLinearColor& Tint)
{
	if (!WorldContextObject)
	{
		return;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	// Skip entirely until the WBP exists (no empty-actor churn pre-art) and honor
	// the concurrency cap (§6 perf — 60+ units can't leak floating widgets).
	if (!AreDamageNumbersAvailable() || LiveCount >= MaxConcurrentNumbers)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ADamageNumberActor* Number = World->SpawnActor<ADamageNumberActor>(StaticClass(), FTransform(WorldLocation), SpawnParams))
	{
		Number->Init(Amount, Tint);
	}
}

bool ADamageNumberActor::AreDamageNumbersAvailable()
{
	// Resolve ONCE per session (art doesn't change mid-PIE): if WBP_DamageNumber
	// is absent, disable + log once so combatants never spawn empty actors.
	static bool bAttempted = false;
	static bool bAvailable = false;
	if (!bAttempted)
	{
		bAttempted = true;
		const TSoftClassPtr<UUserWidget> WidgetClass{ FSoftObjectPath(DamageNumberWidgetClassPath) };
		bAvailable = (WidgetClass.LoadSynchronous() != nullptr);
		if (!bAvailable)
		{
			UE_LOG(LogGitClaudeUnrealTest, Log,
				TEXT("ADamageNumberActor: '%s' unresolved — floating damage numbers disabled until the WBP is authored (TASK-176-adjacent). Logged once."),
				DamageNumberWidgetClassPath);
		}
	}
	return bAvailable;
}

void ADamageNumberActor::BeginPlay()
{
	Super::BeginPlay();

	++LiveCount;

	// Create the number face (the ACastle::InitHPBarWidget pattern — post-BeginPlay
	// SetWidgetClass creates the widget synchronously). Absent class = no widget;
	// the actor still runs its (invisible) lifetime and cleans up (Spawn already
	// gates on availability, so in practice the class resolves here).
	if (NumberWidget)
	{
		if (UClass* LoadedClass = NumberWidgetClass.IsNull() ? nullptr : NumberWidgetClass.LoadSynchronous())
		{
			NumberWidget->SetWidgetClass(LoadedClass);
		}
	}
}

void ADamageNumberActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	--LiveCount;
	Super::EndPlay(EndPlayReason);
}

void ADamageNumberActor::Init(float Amount, const FLinearColor& Tint)
{
	PendingAmount = Amount;
	PendingTint = Tint;

	// GetWidget() is valid here: BeginPlay ran during SpawnActor (world has begun
	// play), so SetWidgetClass already created the instance. A mis-authored/absent
	// widget is skipped (null-safe) — the number just shows nothing and fades out.
	if (NumberWidget)
	{
		if (UDamageNumberWidget* Face = Cast<UDamageNumberWidget>(NumberWidget->GetWidget()))
		{
			Face->SetDamageNumber(PendingAmount, PendingTint.R, PendingTint.G, PendingTint.B);
		}
	}
}

void ADamageNumberActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;

	// C++-owned animation (spec): rise in world space, fade the widget's render
	// opacity linearly to 0 over Lifetime, then self-destruct.
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaSeconds));

	const float Alpha = (Lifetime > 0.f) ? FMath::Clamp(Elapsed / Lifetime, 0.f, 1.f) : 1.f;
	if (NumberWidget)
	{
		if (UUserWidget* Face = NumberWidget->GetWidget())
		{
			Face->SetRenderOpacity(1.f - Alpha);
		}
	}

	if (Elapsed >= Lifetime)
	{
		Destroy();
	}
}
