// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

/**
 *  C++ base for the floating damage-number face (M7 §6, TASK-156). The UMG asset
 *  /Game/UI/WBP_DamageNumber (authored in the editor-wiring task, TASK-176-adjacent)
 *  is reparented to this class (CONVENTIONS "Widgets with C++ bases": U<Name>Widget
 *  ↔ WBP_<Name>). ADamageNumberActor drives it once at spawn.
 *
 *  The single BlueprintImplementableEvent takes FLOAT PARAMS ONLY (the MCP
 *  BP-param rule / CONVENTIONS widget law) so the WBP can be authored by MCP.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Pushed ONCE by ADamageNumberActor at spawn (TASK-156): the WBP sets its
	 *  TextBlock to a readable "-<Amount>" and tints it (R,G,B). Rise + fade are
	 *  driven by the actor (C++-owned animation) — the widget only paints the value.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Feedback")
	void SetDamageNumber(float Amount, float R, float G, float B);
};
