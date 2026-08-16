// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeAccountSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
// ⚠️ ACC-§4 dependency direction: Settings and Deck code depend on the account
// subsystem, never the reverse. The two includes below exist ONLY to reference
// the two guest slot-name constants (+ their user indices) — no Deck/Settings
// TYPE is used in this file, and neither header is included from our .h.
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegeSettingsSubsystem.h"

// The ONE definition of the account log category (CONVENTIONS ACC-§6 —
// LogSiegeAccount lives in SiegeAccountSubsystem.{h,cpp}).
DEFINE_LOG_CATEGORY(LogSiegeAccount);

void USiegeAccountSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// LOAD ONCE. This is the only disk read on any shipped path — every later
	// read is an in-memory getter (the settings-lane contract, cloned).
	// USiegeSettingsSubsystem::Initialize declares InitializeDependency on this
	// class (TASK-601), so this load has completed before the settings lane
	// asks the seam for its slot name.
	LoadAccountsFromSlot();

	const FSiegeProfileInfo* Active = FindActiveProfile();
	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Subsystem initialized — slot '%s' (user %d), %d profile(s), active: %s."),
		*ResolveSlotName(), USiegeAccountSaveGame::UserIndex, Profiles.Num(),
		Active ? *Active->DisplayName : TEXT("guest"));
}

bool USiegeAccountSubsystem::CreateAccount(const FString& DisplayName, const FString& Password, FString& OutReason)
{
	// ── Validation: FIRST violation into OutReason, deterministically, then
	// return false (the UDeckLibrary::IsDeckLegal idiom). ⛔ ACC-§2: no branch
	// below echoes, stores, or logs the password.
	const FString Trimmed = DisplayName.TrimStartAndEnd();

	if (Trimmed.Len() < 3 || Trimmed.Len() > 24)
	{
		OutReason = FString::Printf(
			TEXT("Display name must be 3-24 characters after trimming (got %d)."), Trimmed.Len());
		return false;
	}

	if (Password.Len() < 4)
	{
		// ACC-§3: a convenience bar, not a policy. The message states the rule
		// and NEVER the typed value (ACC-§2).
		OutReason = TEXT("Password must be at least 4 characters.");
		return false;
	}

	if (FindProfileByDisplayName(Trimmed))
	{
		OutReason = FString::Printf(
			TEXT("The display name '%s' is already taken on this machine (names are unique, case-insensitive)."), *Trimmed);
		return false;
	}

	// ── Build the profile: fresh FGuid identity + fresh Digits-hex salt, hash
	// per ACC-§2. The plaintext password's last use is the hash call — dropped.
	FSiegeProfileInfo NewProfile;
	NewProfile.ProfileId         = FGuid::NewGuid();
	NewProfile.DisplayName       = Trimmed;
	NewProfile.CredentialSaltHex = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	NewProfile.CredentialHashHex = MakeCredentialHashHex(Password, NewProfile.CredentialSaltHex);

	const FDateTime NowUtc = FDateTime::UtcNow();
	NewProfile.CreatedUtc   = NowUtc;
	NewProfile.LastLoginUtc = NowUtc;

	// ── Register, set active, save, SEED-COPY, broadcast — the TASK-600 spec
	// order, character-for-character.
	Profiles.Add(NewProfile);
	ActiveProfileId = NewProfile.ProfileId;
	SaveAccountsToSlot();

	// RULING 8 (FLAGGED A6, default TAKEN): the guest decks + settings follow
	// the player into their first profile. Once, at create only, never at
	// login; the guest originals are read, never written (ACC-§1/§3).
	const FString Suffix = MakeProfileSlotSuffix(NewProfile.ProfileId);
	SeedCopySlot(USiegeDeckSaveGame::SlotName, USiegeDeckSaveGame::UserIndex,
		USiegeDeckSaveGame::SlotName + TEXT("_") + Suffix);
	SeedCopySlot(FString(USiegeSettingsSubsystem::SettingsSlotName), USiegeSettingsSubsystem::SettingsUserIndex,
		FString(USiegeSettingsSubsystem::SettingsSlotName) + TEXT("_") + Suffix);

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Created profile '%s' (%s) — now active."),
		*NewProfile.DisplayName, *Suffix);

	BroadcastActiveProfileChanged();

	OutReason.Reset();
	return true;
}

bool USiegeAccountSubsystem::Login(const FString& DisplayName, const FString& Password, FString& OutReason)
{
	const FString Trimmed = DisplayName.TrimStartAndEnd();

	if (Trimmed.IsEmpty())
	{
		OutReason = TEXT("Enter a display name.");
		return false;
	}

	// Case-insensitive lookup (ACC-§3), trimmed like CreateAccount so a stray
	// space cannot hide a profile.
	FSiegeProfileInfo* Found = FindProfileByDisplayName(Trimmed);
	if (!Found)
	{
		OutReason = FString::Printf(TEXT("No profile named '%s' on this machine."), *Trimmed);
		return false;
	}

	// Salted-hash compare through the ONE hash implementation (ACC-§2). The
	// password parameter's only use — never stored, never logged, and the
	// failure reason below does not echo it.
	if (Found->CredentialHashHex != MakeCredentialHashHex(Password, Found->CredentialSaltHex))
	{
		OutReason = FString::Printf(TEXT("Wrong password for '%s'."), *Found->DisplayName);
		return false;
	}

	Found->LastLoginUtc = FDateTime::UtcNow();
	ActiveProfileId     = Found->ProfileId;
	SaveAccountsToSlot();

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Logged in as '%s'."), *Found->DisplayName);

	BroadcastActiveProfileChanged();

	OutReason.Reset();
	return true;
}

void USiegeAccountSubsystem::Logout()
{
	if (!ActiveProfileId.IsValid())
	{
		// ⛔ NO-OP: already guest — no disk write and NO BROADCAST (the delegate
		// law; a delegate that fires on refused or meaningless mutations trains
		// its consumers to ignore it).
		UE_LOG(LogSiegeAccount, Verbose,
			TEXT("[SiegeAccount] Logout while already guest — no-op (no save, no broadcast)."));
		return;
	}

	const FString PreviousName = GetActiveDisplayName();
	ActiveProfileId.Invalidate();
	SaveAccountsToSlot();

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Logged out of '%s' — back to guest."), *PreviousName);

	BroadcastActiveProfileChanged();
}

bool USiegeAccountSubsystem::IsLoggedIn() const
{
	// Funnels through FindActiveProfile so a stale/unknown id (already
	// sanitized at load, but defensive here too) reads as guest, never as a
	// half-logged-in state.
	return FindActiveProfile() != nullptr;
}

FString USiegeAccountSubsystem::GetActiveDisplayName() const
{
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active ? Active->DisplayName : FString();
}

FGuid USiegeAccountSubsystem::GetActiveProfileId() const
{
	// Same funnel: an id that matches no registered profile reports guest
	// (invalid), keeping IsLoggedIn / GetActiveProfileId / the seam agreeing
	// structurally rather than by promise.
	return FindActiveProfile() ? ActiveProfileId : FGuid();
}

FString USiegeAccountSubsystem::GetDeckSlotName() const
{
	// THE SEAM, deck lane (ACC-§4). Guest => the bare shipped constant,
	// byte-identical. ⛔ No slot literal here — the profile slot is BUILT FROM
	// USiegeDeckSaveGame::SlotName, so the constant cannot drift from the law.
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active
		? USiegeDeckSaveGame::SlotName + TEXT("_") + MakeProfileSlotSuffix(Active->ProfileId)
		: USiegeDeckSaveGame::SlotName;
}

FString USiegeAccountSubsystem::GetSettingsSlotName() const
{
	// THE SEAM, settings lane (ACC-§4). Same shape as the deck lane, built
	// from USiegeSettingsSubsystem::SettingsSlotName.
	const FSiegeProfileInfo* Active = FindActiveProfile();
	const FString Base(USiegeSettingsSubsystem::SettingsSlotName);
	return Active ? Base + TEXT("_") + MakeProfileSlotSuffix(Active->ProfileId) : Base;
}

FString USiegeAccountSubsystem::MakeCredentialHashHex(const FString& Password, const FString& SaltHex)
{
	// ACC-§2, character-for-character:
	//     CredentialHashHex = hex(FSHA1::HashBuffer(UTF8(SaltHex + ":" + Password)))
	// ⛔ Local convenience credential, NOT security: this stops shoulder-surfing
	// and accidental plaintext on disk, nothing more. Real auth is the Phase-2
	// backend's job (server-side bcrypt via Supabase GoTrue). The plaintext
	// exists only in this transient buffer and is never persisted or logged.
	const FString Combined = SaltHex + TEXT(":") + Password;
	const FTCHARToUTF8 CombinedUtf8(*Combined);

	uint8 Digest[FSHA1::DigestSize];
	FSHA1::HashBuffer(CombinedUtf8.Get(), CombinedUtf8.Length(), Digest);

	return BytesToHex(Digest, FSHA1::DigestSize);
}

FString USiegeAccountSubsystem::MakeProfileSlotSuffix(const FGuid& ProfileId)
{
	// ACC-§3: the ONE implementation of the suffix derivation — 32 hex chars,
	// filename-safe, and NEVER derived from the display name.
	return ProfileId.ToString(EGuidFormats::Digits);
}

void USiegeAccountSubsystem::SetSlotNameForAutomationTests(const FString& InSlotName)
{
	// ⛔ AUTOMATION ONLY (the USiegeSettingsSubsystem seam, cloned). Exists so
	// a test run can never overwrite the player's real
	// Saved/SaveGames/SiegeAccounts.sav.
	SlotNameOverride = InSlotName;
}

void USiegeAccountSubsystem::LoadAccountsFromSlot()
{
	const FString SlotName = ResolveSlotName();

	// DoesSaveGameExist FIRST so the normal first run (nothing saved yet) is
	// SILENT — LoadGameFromSlot on a missing slot emits an engine warning of
	// its own (the LoadSettingsFromSlot idiom, copied not reinvented).
	USiegeAccountSaveGame* Loaded = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, USiegeAccountSaveGame::UserIndex))
	{
		// Cast, never assume: a corrupt or FOREIGN-CLASS slot casts to nullptr
		// rather than crashing (the TASK-113 carry-forward).
		Loaded = Cast<USiegeAccountSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, USiegeAccountSaveGame::UserIndex));
	}

	if (!Loaded)
	{
		if (!bLoadFallbackLogged)
		{
			// LOG ONCE. Log, not Warning: an absent registry is the normal
			// first run, not a fault (ACC-§1: guest is the default).
			bLoadFallbackLogged = true;
			UE_LOG(LogSiegeAccount, Log,
				TEXT("[SiegeAccount] No readable account registry in slot '%s' (user %d) — empty registry + guest. Normal on a first run; also covers an unreadable or foreign-class slot."),
				*SlotName, USiegeAccountSaveGame::UserIndex);
		}
		Profiles.Reset();
		ActiveProfileId.Invalidate();
		return;
	}

	Profiles        = Loaded->Profiles;
	ActiveProfileId = Loaded->ActiveProfileId;

	// SANITIZE: a persisted active id that matches no profile (hand-edited or
	// truncated .sav) reads as guest. In-memory only — the load path never
	// writes the disk (the settings-lane contract); the next real change
	// persists the corrected state.
	if (ActiveProfileId.IsValid() && !FindActiveProfile())
	{
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] Registry slot '%s' names an active profile id that matches no profile — treating as guest."),
			*SlotName);
		ActiveProfileId.Invalidate();
	}
}

const FSiegeProfileInfo* USiegeAccountSubsystem::FindActiveProfile() const
{
	if (!ActiveProfileId.IsValid())
	{
		return nullptr;
	}
	return Profiles.FindByPredicate([this](const FSiegeProfileInfo& Profile)
	{
		return Profile.ProfileId == ActiveProfileId;
	});
}

FSiegeProfileInfo* USiegeAccountSubsystem::FindProfileByDisplayName(const FString& TrimmedName)
{
	// ACC-§3: uniqueness and login lookup are case-insensitive.
	return Profiles.FindByPredicate([&TrimmedName](const FSiegeProfileInfo& Profile)
	{
		return Profile.DisplayName.Equals(TrimmedName, ESearchCase::IgnoreCase);
	});
}

bool USiegeAccountSubsystem::SaveAccountsToSlot() const
{
	const FString SlotName = ResolveSlotName();

	USiegeAccountSaveGame* SaveObj = Cast<USiegeAccountSaveGame>(
		UGameplayStatics::CreateSaveGameObject(USiegeAccountSaveGame::StaticClass()));
	if (!SaveObj)
	{
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] SaveAccountsToSlot: could not create a USiegeAccountSaveGame — slot '%s' NOT written."), *SlotName);
		return false;
	}

	SaveObj->Profiles        = Profiles;
	SaveObj->ActiveProfileId = ActiveProfileId;

	if (!UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, USiegeAccountSaveGame::UserIndex))
	{
		// A failed write is a Warning and is DELIBERATELY not reverted: the
		// login/create takes effect for this session (the settings-lane
		// ruling — snapping state back under the player's finger because a
		// disk write failed is the worse of the two behaviours).
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] SaveGameToSlot('%s', user %d) FAILED — the change is live for this session but was NOT persisted."),
			*SlotName, USiegeAccountSaveGame::UserIndex);
		return false;
	}

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Saved registry slot '%s' (user %d): %d profile(s), active: %s."),
		*SlotName, USiegeAccountSaveGame::UserIndex, Profiles.Num(),
		ActiveProfileId.IsValid() ? TEXT("yes") : TEXT("guest"));
	return true;
}

void USiegeAccountSubsystem::BroadcastActiveProfileChanged()
{
	// ⚠️ THE ONLY OnActiveProfileChanged.Broadcast IN THIS FILE. The counter is
	// bumped here and nowhere else, which is what lets an automation test
	// observe "did it broadcast?" without a bound UFUNCTION listener (the
	// SettingsChangeBroadcastCount precedent — a dynamic multicast needs one,
	// and a UCLASS cannot be declared in a test .cpp).
	++ActiveProfileChangedBroadcastCount;

	const FSiegeProfileInfo* Active = FindActiveProfile();
	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Active profile changed — now %s (broadcast #%d)."),
		Active ? *Active->DisplayName : TEXT("guest"), ActiveProfileChangedBroadcastCount);

	OnActiveProfileChanged.Broadcast();
}

void USiegeAccountSubsystem::SeedCopySlot(const FString& GuestSlotName, int32 InUserIndex, const FString& ProfileSlotName)
{
	// RULING 8: load → re-save under the profile slot. The guest slot is READ,
	// never written (ACC-§1). SaveGameToSlot serializes the object with its
	// CONCRETE class, so the copy needs no Deck/Settings type here and the
	// ACC-§4 dependency direction holds.
	if (!UGameplayStatics::DoesSaveGameExist(GuestSlotName, InUserIndex))
	{
		// Silent: a fresh install has nothing to seed — the profile slot stays
		// absent and its lane falls back to defaults exactly as guest does.
		return;
	}

	USaveGame* GuestData = UGameplayStatics::LoadGameFromSlot(GuestSlotName, InUserIndex);
	if (!GuestData)
	{
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] Seed-copy: guest slot '%s' (user %d) exists but did not load — profile slot '%s' NOT seeded (its lane will use defaults)."),
			*GuestSlotName, InUserIndex, *ProfileSlotName);
		return;
	}

	if (!UGameplayStatics::SaveGameToSlot(GuestData, ProfileSlotName, InUserIndex))
	{
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] Seed-copy: writing '%s' (user %d) FAILED — the new profile starts from defaults for that lane."),
			*ProfileSlotName, InUserIndex);
		return;
	}

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Seed-copied guest slot '%s' -> profile slot '%s' (user %d)."),
		*GuestSlotName, *ProfileSlotName, InUserIndex);
}

FString USiegeAccountSubsystem::ResolveSlotName() const
{
	// SlotNameOverride is empty on every shipped path — only an automation
	// test ever sets it (SetSlotNameForAutomationTests).
	return SlotNameOverride.IsEmpty() ? USiegeAccountSaveGame::SlotName : SlotNameOverride;
}
