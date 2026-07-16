// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "Siegebound/TeamId.h"
#include "SiegeFeedbackLibrary.generated.h"

class UCameraShakeBase;
class UMaterialInterface;
class UNiagaraSystem;
class UStaticMesh;
class USoundBase;

/** Log category for the M7 §6 juice/feel feedback system (soft-asset misses, once-per-path). */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeFeedback, Log, All);

/**
 *  Siegebound §6 juice/feel feedback library (M7, TASK-154..158 + 179). The ONE
 *  home for the batch's NULL-SAFE, soft-referenced cosmetic spawns — sounds,
 *  Niagara bursts, floating damage numbers, camera shake — and for the shared
 *  soft-asset resolvers (sounds, systems, materials, meshes). Every M7 juice
 *  event site (hit flash, spawn squash, damage numbers, tower recoil, castle
 *  crumble, gold burst, screen shake, and the whole §6 audio list) routes its
 *  ART references through here, so the entire batch COMPILES AND SHIPS BEFORE
 *  any M7 art/audio exists (TASK-174/180): a missing asset logs ONCE per path
 *  on LogSiegeFeedback and no-ops — never a crash (M7 manager decision 5).
 *
 *  Resolvers cache the resolved soft asset per full object path (a fast hash
 *  lookup after the first load — never a per-event disk hitch) and the loaded
 *  asset is kept alive by whatever consumes it (a spawned component / an audio
 *  component / a material slot) for its own lifetime. Paths may be passed short
 *  ("/Game/Audio/S_HeroSwing") — they are normalized to the "package.asset"
 *  object form automatically.
 *
 *  C++-only static entries (NOT BlueprintCallable — the USpellLibrary qa/NIT
 *  precedent: every caller is C++, and raw UWorld pointer / context pins invite BP
 *  misuse).
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeFeedbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	//~ Soft-asset resolvers (cached, null-safe, log-once-per-missing-path) ---------

	/** Resolves a sound by (short or full) object path; nullptr + one log if absent. */
	static USoundBase* ResolveSound(const FString& AssetPath);

	/** Resolves a Niagara system by object path; nullptr + one log if absent. */
	static UNiagaraSystem* ResolveNiagara(const FString& AssetPath);

	/** Resolves a material/instance by object path (hit-flash overlay, castle crumble MI); nullptr + one log if absent. */
	static UMaterialInterface* ResolveMaterial(const FString& AssetPath);

	/** Resolves a static mesh by object path (castle crumble mesh swap); nullptr + one log if absent. */
	static UStaticMesh* ResolveStaticMesh(const FString& AssetPath);

	//~ Fire-and-forget cosmetic spawns (all null-safe) -----------------------------

	/** Plays a 2D (non-spatial) sound — UI clicks, stingers, music. No-op if the sound is absent. */
	static void PlaySound2D(const UObject* WorldContextObject, const FString& SoundPath);

	/** Plays a spatial sound at a world location — swings, spawns, fires, impacts. No-op if absent. */
	static void PlayWorldSound(const UObject* WorldContextObject, const FString& SoundPath, const FVector& Location);

	/** Spawns a one-shot Niagara system at a world location — gold burst, castle debris. No-op if absent. */
	static void SpawnNiagara(const UObject* WorldContextObject, const FString& SystemPath, const FVector& Location);

	/**
	 *  Spawns a short-lived floating damage number over WorldLocation (TASK-156),
	 *  tinted by RGB. Concurrency is CAPPED (ADamageNumberActor) and the whole
	 *  thing no-ops (logged once) until /Game/UI/WBP_DamageNumber exists.
	 */
	static void ShowDamageNumber(const UObject* WorldContextObject, float Amount, const FVector& WorldLocation, const FLinearColor& Tint);

	/**
	 *  Plays a camera shake on the LOCAL player controller (index 0) via
	 *  ClientStartCameraShake — castle-hit screen shake (TASK-158). No-op on a
	 *  null shake class or with no local controller (AI/headless).
	 */
	static void PlayLocalCameraShake(const UObject* WorldContextObject, TSubclassOf<UCameraShakeBase> ShakeClass);

	//~ Helpers ---------------------------------------------------------------------

	/** §6 team color language (mirrors UCombatantHealthBarComponent): Red enemy, Blue friendly. Used to tint damage numbers. */
	static FLinearColor TeamTint(ETeamId Team);
};
