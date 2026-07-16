// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeFeedbackLibrary.h"

#include "Camera/CameraShakeBase.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Siegebound/DamageNumberActor.h"

DEFINE_LOG_CATEGORY(LogSiegeFeedback);

namespace
{
	/**
	 *  Normalizes a soft object path to the "package.asset" object form. A short
	 *  "/Game/Audio/S_HeroSwing" (package only) resolves the PACKAGE, not the
	 *  object — appending ".<basename>" gives the object path LoadSynchronous
	 *  needs. A path that already carries a '.' (full object path, or an "_C"
	 *  class path) is returned untouched.
	 */
	FString NormalizeObjectPath(const FString& InPath)
	{
		if (InPath.IsEmpty() || InPath.Contains(TEXT(".")))
		{
			return InPath;
		}
		return InPath + TEXT(".") + FPaths::GetCleanFilename(InPath);
	}

	/**
	 *  Shared resolver core (TASK-154..158/179): normalize → cache the soft ptr
	 *  per object path → LoadSynchronous. A miss logs EXACTLY once per path on
	 *  LogSiegeFeedback and returns nullptr — the M7 batch ships before the art,
	 *  so 60+ actors must never spam. LoadSynchronous on an already-resident
	 *  asset is a hash lookup, not a disk load, so a per-event call is cheap.
	 *  One cache + one logged-set per template type T (function-local statics).
	 */
	template <typename T>
	T* ResolveSoftAsset(const FString& RawPath, const TCHAR* Kind)
	{
		if (RawPath.IsEmpty())
		{
			return nullptr;
		}

		const FString Path = NormalizeObjectPath(RawPath);

		static TMap<FString, TSoftObjectPtr<T>> Cache;
		static TSet<FString> LoggedMissing;

		TSoftObjectPtr<T>& Entry = Cache.FindOrAdd(Path);
		if (Entry.IsNull())
		{
			Entry = TSoftObjectPtr<T>(FSoftObjectPath(Path));
		}

		T* Resolved = Entry.LoadSynchronous();
		if (!Resolved && !LoggedMissing.Contains(Path))
		{
			LoggedMissing.Add(Path);
			UE_LOG(LogSiegeFeedback, Log,
				TEXT("%s '%s' unresolved — the feature no-ops until the asset lands (M7 art/audio arrives in TASK-174/180). Logged once."),
				Kind, *Path);
		}
		return Resolved;
	}
}

USoundBase* USiegeFeedbackLibrary::ResolveSound(const FString& AssetPath)
{
	return ResolveSoftAsset<USoundBase>(AssetPath, TEXT("Sound"));
}

UNiagaraSystem* USiegeFeedbackLibrary::ResolveNiagara(const FString& AssetPath)
{
	return ResolveSoftAsset<UNiagaraSystem>(AssetPath, TEXT("Niagara system"));
}

UMaterialInterface* USiegeFeedbackLibrary::ResolveMaterial(const FString& AssetPath)
{
	return ResolveSoftAsset<UMaterialInterface>(AssetPath, TEXT("Material"));
}

UStaticMesh* USiegeFeedbackLibrary::ResolveStaticMesh(const FString& AssetPath)
{
	return ResolveSoftAsset<UStaticMesh>(AssetPath, TEXT("Static mesh"));
}

void USiegeFeedbackLibrary::PlaySound2D(const UObject* WorldContextObject, const FString& SoundPath)
{
	if (!WorldContextObject)
	{
		return;
	}
	if (USoundBase* Sound = ResolveSound(SoundPath))
	{
		UGameplayStatics::PlaySound2D(WorldContextObject, Sound);
	}
}

void USiegeFeedbackLibrary::PlayWorldSound(const UObject* WorldContextObject, const FString& SoundPath, const FVector& Location)
{
	if (!WorldContextObject)
	{
		return;
	}
	if (USoundBase* Sound = ResolveSound(SoundPath))
	{
		UGameplayStatics::SpawnSoundAtLocation(WorldContextObject, Sound, Location);
	}
}

void USiegeFeedbackLibrary::SpawnNiagara(const UObject* WorldContextObject, const FString& SystemPath, const FVector& Location)
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

	if (UNiagaraSystem* System = ResolveNiagara(SystemPath))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, System, Location);
	}
}

void USiegeFeedbackLibrary::ShowDamageNumber(const UObject* WorldContextObject, float Amount, const FVector& WorldLocation, const FLinearColor& Tint)
{
	// ADamageNumberActor owns the widget resolution, the concurrency cap, and the
	// rise/fade lifetime — this is the single call site combatants use (TASK-156).
	ADamageNumberActor::Spawn(WorldContextObject, Amount, WorldLocation, Tint);
}

void USiegeFeedbackLibrary::PlayLocalCameraShake(const UObject* WorldContextObject, TSubclassOf<UCameraShakeBase> ShakeClass)
{
	if (!WorldContextObject || !ShakeClass)
	{
		return;
	}

	// Local (index-0) controller: M7 is single-player local, exactly like the
	// hero's TASK-016 ClientStartCameraShake. No controller (AI/headless) = no shake.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		PC->ClientStartCameraShake(ShakeClass);
	}
}

FLinearColor USiegeFeedbackLibrary::TeamTint(ETeamId Team)
{
	// §6 palette / MI_TeamColor linear values — mirrors UCombatantHealthBarComponent
	// (Blue friendly, Red enemy) so the damage-number color speaks the SAME team language.
	return (Team == ETeamId::Red)
		? FLinearColor(1.00f, 0.10f, 0.05f)
		: FLinearColor(0.05f, 0.30f, 1.00f);
}
