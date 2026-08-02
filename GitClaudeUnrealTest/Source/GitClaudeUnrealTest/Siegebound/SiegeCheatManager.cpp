// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeCheatManager.h"

#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GitClaudeUnrealTest.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPtr.h"
#include "Siegebound/Castle.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SiegePlayerState.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"

namespace
{
	/** How far the "crosshair" (camera forward) trace reaches for both cheats. */
	constexpr float SiegeCheatTraceDistance = 100000.f;

	/**
	 *  SetTestDamageBoost — absurd-input guard ONLY (it keeps a typo'd
	 *  `SetTestDamageBoost 1e30` from overflowing the int32 stack count). The REAL
	 *  cap is ASummonedUnit::MaxPermanentDamageStacks, applied inside
	 *  AddPermanentDamageStacks — which is exactly why the PIE checklist's `500`
	 *  row must render identically to its `400` row.
	 */
	constexpr double SiegeCheatMaxRequestableStacks = 1000000.0;

	/**
	 *  SetTestDamageBoost — tolerance in STACKS (not percent) subtracted before the
	 *  ceil, so an EXACT band boundary never over-ceils by one stack.
	 *
	 *  WHY IT IS NEEDED: PermanentDamageBonusPerStack is a float, and 0.05f is
	 *  really 0.05000000074…, so 1.0 / 0.05f evaluates to 19.9999997 — ceil gives
	 *  the wanted 20. But the sign of that representation error is a property of the
	 *  authored value: a per-stack bonus that happened to round DOWN would make the
	 *  same division 20.0000003, and ceil would return 21 — i.e. `SetTestDamageBoost 100`
	 *  would silently produce 105% and the "exactly 100% is FULL light blue" row of
	 *  the gate would be untestable.
	 *
	 *  WHY THIS SIZE: it has to sit between the error it absorbs and the smallest
	 *  real request it must not swallow. Float error at the +400% cap is ~80 stacks
	 *  × 6e-8 ≈ 5e-6 stacks, so 1e-4 is ~20× above it; the finest distinction the
	 *  gate makes is `100` vs `101` = 0.2 stacks apart, so 1e-4 is ~2000× below it.
	 */
	constexpr double SiegeCheatStackEpsilon = 1.e-4;

	/**
	 *  Alive across the combat types: units/buildings/hero answer IHealthBarProvider;
	 *  ACastle keeps its own destroyed latch (it does NOT implement IHealthBarProvider,
	 *  CONVENTIONS note). Unknown ITeamAgent types are treated as alive.
	 */
	bool IsCombatActorAlive(const AActor* Actor)
	{
		if (const IHealthBarProvider* Bar = Cast<IHealthBarProvider>(Actor))
		{
			return Bar->IsHealthBarActorAlive();
		}
		if (const ACastle* Castle = Cast<ACastle>(Actor))
		{
			return !Castle->IsCastleDestroyed();
		}
		return true;
	}

	/**
	 *  Camera-forward "crosshair" line trace on ECC_Visibility (the same channel the
	 *  controller's cursor trace uses), ignoring the controlled pawn. True on a
	 *  blocking hit.
	 */
	bool TraceFromCrosshair(APlayerController* PC, UWorld* World, FHitResult& OutHit)
	{
		if (!PC || !World)
		{
			return false;
		}
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * SiegeCheatTraceDistance;

		FCollisionQueryParams Params(FName(TEXT("SiegeCheatCrosshair")), /*bTraceComplex=*/false);
		if (const APawn* ViewPawn = PC->GetPawn())
		{
			Params.AddIgnoredActor(ViewPawn);
		}
		return World->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, Params);
	}

	/**
	 *  The point the "nearest X" fallbacks measure from: the controlled pawn if
	 *  there is one, else the camera location. Extracted verbatim from
	 *  ApplyTestDamage so SetTestDamageBoost mirrors its targeting by SHARING the
	 *  code rather than by copying it — behaviour is unchanged in both.
	 */
	FVector ResolveNearestSearchOrigin(APlayerController* PC)
	{
		if (const APawn* RefPawn = PC ? PC->GetPawn() : nullptr)
		{
			return RefPawn->GetActorLocation();
		}
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		if (PC)
		{
			PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		}
		return ViewLocation;
	}

	/** Nearest alive ITeamAgent whose team differs from MyTeam, measured from RefLocation. */
	AActor* FindNearestEnemy(UWorld* World, ETeamId MyTeam, const FVector& RefLocation)
	{
		if (!World)
		{
			return nullptr;
		}
		TArray<AActor*> TeamAgents;
		UGameplayStatics::GetAllActorsWithInterface(World, UTeamAgent::StaticClass(), TeamAgents);

		AActor* Best = nullptr;
		float BestDistSq = TNumericLimits<float>::Max();
		for (AActor* Candidate : TeamAgents)
		{
			if (!IsValid(Candidate))
			{
				continue;
			}
			const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
			if (!Agent || Agent->GetTeamId() == MyTeam)
			{
				continue;
			}
			if (!IsCombatActorAlive(Candidate))
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(RefLocation, Candidate->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Candidate;
			}
		}
		return Best;
	}

	/**
	 *  Every FRIENDLY ASummonedUnit that can actually receive a permanent damage
	 *  boost, appended to Out.
	 *
	 *  Eligibility is the SHIPPING predicate CanReceiveDamageBoost() (alive && able
	 *  to attack at all && positive row Damage && not a Support profile) — the very
	 *  same gate AAncientGround's boost tick uses. That is the whole point of
	 *  routing through the shipping path: this cheat must never be able to grant a
	 *  stack the real granter would refuse. It also means the nearest-friendly
	 *  fallback below never silently lands on a Sorcerer / Cleric / Miner, whose
	 *  damage routes through neither compose point and whose boost row is hidden by
	 *  design — a "nothing visibly happened" result during the human PIE gate.
	 *
	 *  GetAllActorsOfClass is a full-world scan (the ARally / hero-heal precedent at
	 *  HeroCharacter.cpp:502), acceptable here because this runs once per typed
	 *  console command and never on a tick.
	 */
	void GatherBoostableFriendlyUnits(UWorld* World, ETeamId MyTeam, TArray<ASummonedUnit*>& Out)
	{
		if (!World)
		{
			return;
		}
		TArray<AActor*> UnitActors;
		UGameplayStatics::GetAllActorsOfClass(World, ASummonedUnit::StaticClass(), UnitActors);
		for (AActor* Candidate : UnitActors)
		{
			ASummonedUnit* Unit = Cast<ASummonedUnit>(Candidate);
			if (!IsValid(Unit) || Unit->GetTeamId() != MyTeam || !Unit->CanReceiveDamageBoost())
			{
				continue;
			}
			Out.Add(Unit);
		}
	}

	/** Nearest boost-eligible friendly ASummonedUnit to RefLocation, or null on an empty field. */
	ASummonedUnit* FindNearestBoostableFriendlyUnit(UWorld* World, ETeamId MyTeam, const FVector& RefLocation)
	{
		TArray<ASummonedUnit*> Candidates;
		GatherBoostableFriendlyUnits(World, MyTeam, Candidates);

		ASummonedUnit* Best = nullptr;
		float BestDistSq = TNumericLimits<float>::Max();
		for (ASummonedUnit* Unit : Candidates)
		{
			const float DistSq = FVector::DistSquared(RefLocation, Unit->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Unit;
			}
		}
		return Best;
	}

	/** The local player's team, read from the controlled pawn if it is an ITeamAgent; Blue otherwise (CONVENTIONS team contract). */
	ETeamId ResolveLocalTeam(const APlayerController* PC)
	{
		if (PC)
		{
			if (const ITeamAgent* PawnAgent = Cast<ITeamAgent>(PC->GetPawn()))
			{
				return PawnAgent->GetTeamId();
			}
		}
		return ETeamId::Blue;
	}
}

void USiegeCheatManager::SummonTestUnit(FString CardID, bool bRed)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — no owning PlayerController; nothing spawned."));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — no World; nothing spawned."));
		return;
	}

	const FName CardName(*CardID);
	if (CardName.IsNone())
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SummonTestUnit — empty CardID; nothing spawned."));
		return;
	}

	// Compose the SAME unit BP path ASiegePlayerController::ResolveCardActorClass /
	// ASiegeBotController::ResolveBotCardActorClass compose for a unit card
	// (CONVENTIONS composed soft-class law). A building/spell CardID has no
	// BP_Unit_ class, so this refuses cleanly — this cheat summons UNIT cards.
	const FString ClassPath = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardID, *CardID);
	UClass* UnitClass = TSoftClassPtr<AActor>(FSoftObjectPath(ClassPath)).LoadSynchronous();
	if (!UnitClass || !UnitClass->IsChildOf(ASummonedUnit::StaticClass()))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USiegeCheatManager::SummonTestUnit — '%s' missing or not an ASummonedUnit (composed '%s'); this cheat summons UNIT cards only."),
			*CardID, *ClassPath);
		return;
	}

	const ETeamId Team = bRed ? ETeamId::Red : ETeamId::Blue;

	// Ground point = the surface under the crosshair (camera forward trace); fall
	// back to the controlled pawn's location. SpawnUnitSwarm applies the capsule
	// lift, so this must be a GROUND point (the trace ImpactPoint is exactly that).
	FVector GroundPoint;
	FHitResult Hit;
	if (TraceFromCrosshair(PC, World, Hit))
	{
		GroundPoint = Hit.ImpactPoint;
	}
	else if (const APawn* ViewPawn = PC->GetPawn())
	{
		GroundPoint = ViewPawn->GetActorLocation();
	}
	else
	{
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
		GroundPoint = ViewLocation + ViewRotation.Vector() * 800.f;
	}

	// Spawn via the SHARED shipping unit-spawn entry (the player confirm path AND
	// the bot both call this). Instigator = nullptr (team attribution resolves via
	// each unit's own ITeamAgent — the bot's team-neutral choice); Count 1 = a
	// single test unit (call again for a swarm).
	const TArray<ASummonedUnit*> Spawned = ASiegePlayerController::SpawnUnitSwarm(
		World, UnitClass, CardName, Team,
		/*SpawnOwner=*/ PC, /*SpawnInstigator=*/ nullptr,
		GroundPoint, /*Count=*/ 1, /*Radius=*/ 0.f);

	if (Spawned.Num() == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Error,
			TEXT("USiegeCheatManager::SummonTestUnit — SpawnUnitSwarm produced no unit for '%s'."), *CardID);
		return;
	}

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::SummonTestUnit — spawned '%s' for team %s at %s."),
		*CardID, (Team == ETeamId::Red) ? TEXT("Red") : TEXT("Blue"), *GroundPoint.ToString());
}

void USiegeCheatManager::ApplyTestDamage(float Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no owning PlayerController; no damage applied."));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no World; no damage applied."));
		return;
	}
	if (Amount <= 0.f)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — Amount %.1f must be positive; no damage applied."), Amount);
		return;
	}

	const ETeamId MyTeam = ResolveLocalTeam(PC);

	// Prefer the combat actor under the crosshair; else the nearest enemy — the
	// deterministic locked-desktop fallback (no aiming needed to hit the enemy
	// castle / units and drive their HP + overhead bars).
	AActor* Target = nullptr;
	FHitResult Hit;
	if (TraceFromCrosshair(PC, World, Hit))
	{
		AActor* HitActor = Hit.GetActor();
		if (IsValid(HitActor) && Cast<ITeamAgent>(HitActor))
		{
			Target = HitActor;
		}
	}
	if (!Target)
	{
		Target = FindNearestEnemy(World, MyTeam, ResolveNearestSearchOrigin(PC));
	}
	if (!Target)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::ApplyTestDamage — no combat actor under the crosshair and no enemy found; no damage applied."));
		return;
	}

	// Normal engine TakeDamage path. Instigator + causer are BOTH null, so the
	// receiver's team resolver reads WORLD damage and applies regardless of the
	// target's team (a debug tool damages friend or foe); null damage-type = base
	// UDamageType (100%, no fortification scaling).
	const float Applied = UGameplayStatics::ApplyDamage(Target, Amount, /*EventInstigator=*/nullptr, /*DamageCauser=*/nullptr, /*DamageTypeClass=*/nullptr);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::ApplyTestDamage — requested %.1f on '%s'; %.1f applied (after receiver-side scaling)."),
		Amount, *GetNameSafe(Target), Applied);
}

void USiegeCheatManager::AddTestGold(int32 Amount)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::AddTestGold — no owning PlayerController; no gold granted."));
		return;
	}

	ASiegePlayerState* SiegeState = PC->GetPlayerState<ASiegePlayerState>();
	if (!SiegeState)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::AddTestGold — no ASiegePlayerState on the local player; no gold granted."));
		return;
	}

	// Gold choke-point API — clamp + OnGoldChanged broadcast preserved; AddGold
	// itself refuses+logs a non-positive amount.
	SiegeState->AddGold(Amount);

	UE_LOG(LogGitClaudeUnrealTest, Log,
		TEXT("USiegeCheatManager::AddTestGold — requested +%d gold on '%s' (now %d)."),
		Amount, *GetNameSafe(SiegeState), SiegeState->GetGold());
}

void USiegeCheatManager::SetTestDamageBoost(float Percent, bool bAllFriendly)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!PC)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SetTestDamageBoost — no owning PlayerController; no boost changed."));
		return;
	}
	UWorld* World = PC->GetWorld();
	if (!World)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SetTestDamageBoost — no World; no boost changed."));
		return;
	}
	// A console-typed token that is not a number parses to 0 and legitimately means
	// "clear"; an infinity or NaN does NOT, and NaN would slip past every ordered
	// comparison below straight into the stack math.
	if (!FMath::IsFinite(Percent))
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SetTestDamageBoost — Percent is not a finite number; no boost changed."));
		return;
	}

	const ETeamId MyTeam = ResolveLocalTeam(PC);
	const TCHAR* TeamName = (MyTeam == ETeamId::Red) ? TEXT("Red") : TEXT("Blue");

	// ---- Target set ---------------------------------------------------------
	TArray<ASummonedUnit*> Targets;
	if (bAllFriendly)
	{
		GatherBoostableFriendlyUnits(World, MyTeam, Targets);
	}
	else
	{
		// Same targeting shape as ApplyTestDamage: the actor under the crosshair
		// first, the nearest one otherwise — the deterministic locked-desktop
		// fallback that needs no aiming.
		ASummonedUnit* Target = nullptr;
		FHitResult Hit;
		if (TraceFromCrosshair(PC, World, Hit))
		{
			ASummonedUnit* HitUnit = Cast<ASummonedUnit>(Hit.GetActor());
			if (IsValid(HitUnit) && HitUnit->GetTeamId() == MyTeam)
			{
				if (HitUnit->CanReceiveDamageBoost())
				{
					Target = HitUnit;
				}
				else
				{
					// Said out loud on purpose: aiming squarely at a Sorcerer,
					// Cleric, Miner or a dying unit and having the boost quietly
					// appear on some other unit is the single most confusing thing
					// this lever could do during the human PIE gate.
					UE_LOG(LogGitClaudeUnrealTest, Warning,
						TEXT("USiegeCheatManager::SetTestDamageBoost — '%s' is under the crosshair but cannot receive a damage boost (dead, never-attacks, zero row Damage, or a Support profile); falling back to the nearest eligible friendly."),
						*GetNameSafe(HitUnit));
				}
			}
		}
		if (!Target)
		{
			Target = FindNearestBoostableFriendlyUnit(World, MyTeam, ResolveNearestSearchOrigin(PC));
		}
		if (Target)
		{
			Targets.Add(Target);
		}
	}

	if (Targets.Num() == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USiegeCheatManager::SetTestDamageBoost — no boost-eligible friendly (team %s) ASummonedUnit found; no boost changed. Summon one first: SummonTestUnit Footman false"),
			TeamName);
		return;
	}

	// ---- Percent <= 0 ⇒ clear only ------------------------------------------
	// Deliberately BEFORE the per-stack read: `SetTestDamageBoost 0 true` — the
	// "boost bar and frame vanish" row of the gate — is a CLEAR, not a conversion,
	// and the percent→stacks division below is undefined for it.
	if (Percent <= 0.f)
	{
		for (ASummonedUnit* Unit : Targets)
		{
			Unit->ClearPermanentDamageStacks();
		}
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USiegeCheatManager::SetTestDamageBoost — Percent %.1f <= 0: cleared the permanent damage boost on %d friendly unit(s) (team %s)."),
			Percent, Targets.Num(), TeamName);
		return;
	}

	// ---- Percent → integer stacks, from the unit's OWN mechanic rule ---------
	// NEVER a hardcoded 0.05 (manager ruling): that literal would silently drift
	// the moment PermanentDamageBonusPerStack is tuned, and this lever's entire
	// value is that the number Jonathan types is the number the bar shows.
	//
	// TASK-379 retired the reflection read this used to perform
	// (CastField<FFloatProperty> + FindPropertyByName + a "refusing to guess" Error
	// path). It existed ONLY because the property sat in ASummonedUnit's `protected:`
	// block, so a direct member read would not compile; the public BlueprintPure
	// getter added there (qa/TASK-365-report.md "THE SIMPLIFICATION VERDICT") makes it
	// dead weight. The read below is still PER-INSTANCE — a per-Blueprint override is
	// honoured exactly as before — and it can no longer fail to resolve at all.

	int32 AppliedCount = 0;
	int32 SkippedCount = 0;
	int32 LastStacks = 0;
	ASummonedUnit* LastApplied = nullptr;
	float MinAchieved = TNumericLimits<float>::Max();
	float MaxAchieved = -TNumericLimits<float>::Max();

	for (ASummonedUnit* Unit : Targets)
	{
		const float PerStack = Unit->GetPermanentDamageBonusPerStack();
		if (PerStack <= 0.f)
		{
			++SkippedCount;
			continue;
		}

		// Round UP, not to nearest: the stack grid is coarse (5% at the shipped
		// rule), so a request that falls between two stacks must land on the first
		// boost STRICTLY ABOVE it — that is what makes the gate's "just past a band
		// boundary" row possible at all. `101` ⇒ 21 stacks ⇒ 105% (band 2, nearly
		// empty); rounding to nearest would give 20 stacks ⇒ 100% and silently
		// re-test the row above it. The epsilon keeps an EXACT boundary exact.
		//
		// The whole conversion runs in DOUBLE — a float would carry the division's
		// error into the epsilon's own magnitude. CeilToInt64 (not CeilToInt, whose
		// double overload also returns int64) makes the width explicit; the clamp
		// above bounds the result far below INT32_MAX, so the narrowing cast is
		// exact by construction rather than by hope.
		const double RawStacks = static_cast<double>(Percent) / 100.0 / static_cast<double>(PerStack);
		const double ClampedStacks = FMath::Clamp(RawStacks - SiegeCheatStackEpsilon, 0.0, SiegeCheatMaxRequestableStacks);
		const int32 Stacks = static_cast<int32>(FMath::CeilToInt64(ClampedStacks));

		// THE ORDER IS LOAD-BEARING. Clear first: AddPermanentDamageStacks is
		// ADDITIVE and broadcasts only on an ACTUAL change, so without the Clear
		// this command would pile onto the current value instead of SETTING it, and
		// re-issuing the same Percent twice would broadcast nothing at all. Clear
		// broadcasts unconditionally, Add then broadcasts the new value — the
		// overhead bar is driven end-to-end through the shipping delegate, never a
		// raw field write.
		Unit->ClearPermanentDamageStacks();
		if (Stacks > 0)
		{
			Unit->AddPermanentDamageStacks(Stacks);
		}

		// Read back what the BAR will read (GetDamageBoostPercent is the exact value
		// UCombatantHealthBarComponent bands on), so the log reports what actually
		// landed after the cap clamp rather than what was asked for.
		const float Achieved = Unit->GetDamageBoostPercent();
		MinAchieved = FMath::Min(MinAchieved, Achieved);
		MaxAchieved = FMath::Max(MaxAchieved, Achieved);
		LastStacks = Stacks;
		LastApplied = Unit;
		++AppliedCount;
	}

	if (SkippedCount > 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning,
			TEXT("USiegeCheatManager::SetTestDamageBoost — skipped %d unit(s) whose PermanentDamageBonusPerStack is <= 0 (percent-to-stacks is undefined there)."),
			SkippedCount);
	}
	if (AppliedCount == 0)
	{
		UE_LOG(LogGitClaudeUnrealTest, Warning, TEXT("USiegeCheatManager::SetTestDamageBoost — no unit could be converted to a stack count; no boost changed."));
		return;
	}

	if (AppliedCount == 1 && LastApplied)
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USiegeCheatManager::SetTestDamageBoost — '%s' (team %s): requested %.1f%%, applied %d stack(s) → bar reads %.1f%%, damage x%.3f. Over-cap requests clamp inside AddPermanentDamageStacks."),
			*GetNameSafe(LastApplied), TeamName, Percent, LastStacks, MinAchieved, LastApplied->GetPermanentDamageMultiplier());
	}
	else
	{
		UE_LOG(LogGitClaudeUnrealTest, Log,
			TEXT("USiegeCheatManager::SetTestDamageBoost — requested %.1f%% on %d friendly unit(s) (team %s): bar reads %.1f%%..%.1f%%. Over-cap requests clamp inside AddPermanentDamageStacks."),
			Percent, AppliedCount, TeamName, MinAchieved, MaxAchieved);
	}
}
