// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/UnitHealthBarWidget.h"

// UUnitHealthBarWidget is a pure C++ base: it declares the two float-only
// BlueprintImplementableEvents that WBP_UnitHealthBar (TASK-111) implements
// (fill percent + team tint) and carries no C++ logic of its own — the
// UHealthBarComponent owns all the polling/show-hide/tint-push behavior. This
// translation unit exists so the class is compiled into the module (the
// CastleHealthBarWidget precedent) and to host any future C++ helpers.
