// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SiegeAccountSaveGame.generated.h"

/**
 *  ONE LOCAL PLAYER PROFILE (batch ACCOUNTS, TASK-599; CONVENTIONS ACC-§3 and
 *  the ACC-§7 pinned signature registry — every field below is registry-pinned
 *  character-for-character).
 *
 *  Identity is ProfileId (FGuid) — NEVER the display name (ACC-§3: users type
 *  anything; filenames + collisions + renames). The profile-scoped save slots
 *  are derived from it as SiegeDecks_<Digits> / SiegeSettings_<Digits>
 *  where <Digits> = ProfileId.ToString(EGuidFormats::Digits) — the ONE
 *  implementation of that derivation is USiegeAccountSubsystem::
 *  MakeProfileSlotSuffix (TASK-600), not here. Model only: this struct carries
 *  data and no logic.
 *
 *  ⛔ THE HONEST-CREDENTIAL LAW (ACC-§2), STATED WHERE THE FIELDS LIVE:
 *  CredentialSaltHex + CredentialHashHex are a LOCAL CONVENIENCE CREDENTIAL,
 *  NOT SECURITY — real auth is the Phase-2 backend's job. The stored value is
 *  hex(FSHA1::HashBuffer(UTF8(CredentialSaltHex + ":" + Password))) with a
 *  fresh per-profile FGuid Digits-hex salt. That stops shoulder-surfing and
 *  accidental plaintext on disk and NOTHING MORE: anyone with disk access can
 *  edit SiegeAccounts.sav. The plaintext password is NEVER persisted and NEVER
 *  logged (ACC-§2) — there is no field for it here, by design.
 */
USTRUCT(BlueprintType)
struct FSiegeProfileInfo
{
	GENERATED_BODY()

	/** Stable profile identity — the slot-suffix source (ACC-§3). Never the display name. */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FGuid ProfileId;

	/** In-game handle: trimmed, 3–24 chars, unique case-insensitive among local profiles (ACC-§3). Login lookup is case-insensitive. */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FString DisplayName;

	/** Per-profile salt: a fresh FGuid in Digits hex. ACC-§2: convenience, NOT security. */
	UPROPERTY(SaveGame)
	FString CredentialSaltHex;

	/** hex(SHA1(Salt + ":" + Password)) — the local convenience credential (ACC-§2); the plaintext is dropped after hashing, never stored. */
	UPROPERTY(SaveGame)
	FString CredentialHashHex;

	/** UTC timestamp at CreateAccount. */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FDateTime CreatedUtc;

	/** UTC timestamp of the most recent successful Login (== CreatedUtc until then). */
	UPROPERTY(SaveGame, BlueprintReadOnly)
	FDateTime LastLoginUtc;
};

/**
 *  THE LOCAL ACCOUNT REGISTRY (batch ACCOUNTS, TASK-599; CONVENTIONS ACC-§3 /
 *  ACC-§6 rows 1–2 / the ACC-§7 registry).
 *
 *  Persisted with UGameplayStatics::SaveGameToSlot / LoadGameFromSlot to the
 *  fixed slot USiegeAccountSaveGame::SlotName ("SiegeAccounts") at user index
 *  USiegeAccountSaveGame::UserIndex (0) => Saved/SaveGames/SiegeAccounts.sav —
 *  the USiegeDeckSaveGame idiom, cloned. USiegeAccountSubsystem (TASK-600) is
 *  the ONLY reader and the ONLY writer; nothing else may load or save this
 *  slot. Model only — registry state, no logic.
 *
 *  GUEST-DEFAULT CONTRACT (ACC-§1): an invalid ActiveProfileId — the
 *  default-constructed FGuid — means GUEST, and guest is the default. A
 *  missing, unreadable or foreign-class slot casts to nullptr and the
 *  subsystem falls back to an empty registry + guest, logged once on
 *  LogSiegeAccount, never a crash (the USiegeDeckSaveGame null-safety
 *  contract). The guest slots (USiegeDeckSaveGame::SlotName /
 *  USiegeSettingsSubsystem::SettingsSlotName) are never mutated by account
 *  code.
 *
 *  ⛔ ACC-§2 HONESTY STATEMENT: the credential material stored in Profiles is
 *  a local convenience, NOT security — see FSiegeProfileInfo above. No
 *  artifact may describe it as anything stronger.
 *
 *  VERSIONING: tagged-property serialization (the USiegeSettingsSaveGame
 *  precedent) — a field added later is absent from an older .sav and loads at
 *  its C++ default; no version int32 is carried and none is needed.
 *
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no
 *  new replicated class, no new relevancy tier, no RPC. All account state is
 *  client-local (UGameInstanceSubsystem + local USaveGame); the display name
 *  touches no session/player name (A7). Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeAccountSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	/** Fixed SaveGame slot name — TEXT("SiegeAccounts") — the SiegeDeckSaveGame.cpp:7 idiom: defined once in the .cpp so every translation unit links the same string. */
	static const FString SlotName;

	/** User index for the account registry slot (single local player => 0). Registry-pinned as static const, defined in the .cpp (ACC-§7). */
	static const int32 UserIndex;

	/** Every local profile on this machine, in creation order. Uniqueness (case-insensitive DisplayName) is enforced by the subsystem at CreateAccount, not here. */
	UPROPERTY(SaveGame)
	TArray<FSiegeProfileInfo> Profiles;

	/** The logged-in profile. Invalid GUID = guest (ACC-§1) — the accountless default. */
	UPROPERTY(SaveGame)
	FGuid ActiveProfileId;
};
