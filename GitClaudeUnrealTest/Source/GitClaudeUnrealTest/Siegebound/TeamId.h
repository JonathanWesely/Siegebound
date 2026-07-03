// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TeamId.generated.h"

/**
 *  Team identifier for all Siegebound combat actors.
 *  Team contract (CONVENTIONS.md): the local player is always Blue; the enemy is Red.
 */
UENUM(BlueprintType)
enum class ETeamId : uint8
{
	Blue,
	Red
};

/**
 *  TeamAgent interface
 *  Implemented by every actor that belongs to a team (hero, summoned units, castles).
 *  This is the shared friendly-fire / team check used across Siegebound gameplay code.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UTeamAgent : public UInterface
{
	GENERATED_BODY()
};

class ITeamAgent
{
	GENERATED_BODY()

public:

	/** Returns the team this agent belongs to */
	UFUNCTION(BlueprintCallable, Category="Team")
	virtual ETeamId GetTeamId() const = 0;
};
