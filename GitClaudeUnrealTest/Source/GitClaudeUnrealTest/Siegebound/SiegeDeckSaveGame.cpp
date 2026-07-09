// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeDeckSaveGame.h"

// Fixed slot name shared by every reader/writer (M6 ruling 1). One definition
// here so all translation units link to the same string.
const FString USiegeDeckSaveGame::SlotName = TEXT("SiegeDecks");
