// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeSettingsSaveGame.h"

// ⚠️ INTENTIONALLY EMPTY OF DEFINITIONS — this is not a stub.
//
// The v1 field is an inline-initialized UPROPERTY (its default IS the fallback
// contract, read off the CDO by USiegeSettingsSubsystem), and the slot contract
// itself — SettingsSlotName / SettingsUserIndex — is pinned onto
// USiegeSettingsSubsystem by the §8 signature registry, NOT onto this class.
// That is the one deliberate difference from the USiegeDeckSaveGame precedent,
// where SlotName/UserIndex live on the SaveGame.
//
// The file exists because CONVENTIONS §2 pins the .h/.cpp pair, and because the
// out-of-line definitions the NEXT setting needs (a static const FString table,
// a migration helper) belong here rather than in the subsystem.
