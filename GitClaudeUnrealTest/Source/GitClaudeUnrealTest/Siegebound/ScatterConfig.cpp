// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/ScatterConfig.h"

// USiegeScatterConfig is a pure data container (CONVENTIONS "Battlefield &
// procedural terrain (M6.5)"): all behavior lives on ASiegeBattlefieldScatter,
// which reads this asset at match start. No logic here — the .cpp exists so the
// module compiles the reflected UDataAsset/USTRUCT types (ScatterConfig.h).
