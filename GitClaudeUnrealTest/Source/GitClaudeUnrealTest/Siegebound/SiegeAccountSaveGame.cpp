// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeAccountSaveGame.h"

// Fixed slot name + user index for the local account registry (ACC-§3:
// "SiegeAccounts" @ user index 0 => Saved/SaveGames/SiegeAccounts.sav).
// One definition here so all translation units link to the same values —
// the SiegeDeckSaveGame.cpp:7 idiom, cloned per the ACC-§7 registry.
const FString USiegeAccountSaveGame::SlotName = TEXT("SiegeAccounts");
const int32   USiegeAccountSaveGame::UserIndex = 0;
