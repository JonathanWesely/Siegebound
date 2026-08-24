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

// ─────────────────────────────────────────────────────────────────────────────
// PHASE-2 CLOUD-LINK API (batch ACCOUNTS-P2, TASK-644 — ACC-§15 block 4,
// ACC-§11/§13). Persistence of the cloud link ONLY: ⛔ no HTTP here (643's),
// ⛔ no sync logic (645's). Every mutator below follows the P1 contract —
// mutate ONLY the active profile, save the registry slot, broadcast — and the
// delegate law: a no-op saves nothing and broadcasts NOTHING.
// ─────────────────────────────────────────────────────────────────────────────

bool USiegeAccountSubsystem::IsCloudLinked() const
{
	// The ACC-§15 predicate, verbatim: active profile has LinkedEmail +
	// CloudUserId. The refresh token is deliberately NOT part of it — a link
	// with an expired/absent token is still a link (re-auth refills it).
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active && !Active->LinkedEmail.IsEmpty() && !Active->CloudUserId.IsEmpty();
}

FString USiegeAccountSubsystem::GetLinkedEmail() const
{
	// Guest => empty; an unlinked profile's LinkedEmail is empty by contract,
	// so one expression covers both registry-comment cases ("empty when
	// guest/unlinked").
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active ? Active->LinkedEmail : FString();
}

void USiegeAccountSubsystem::SetCloudLink(const FString& Email, const FString& UserId, const FString& RefreshToken)
{
	FSiegeProfileInfo* Active = FindActiveProfileMutable();
	if (!Active)
	{
		// Guest has no cloud identity (ACC-§13) and no profile row to mutate.
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] SetCloudLink while guest — no active profile to link; no-op (no save, no broadcast)."));
		return;
	}

	const FString TrimmedEmail  = Email.TrimStartAndEnd();
	const FString TrimmedUserId = UserId.TrimStartAndEnd();
	if (TrimmedEmail.IsEmpty() || TrimmedUserId.IsEmpty())
	{
		// Storing a half-link would break the IsCloudLinked predicate while
		// leaving a token on disk — refuse whole. ⛔ Neither value is echoed:
		// one is empty and the other is not needed to state the rule.
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] SetCloudLink refused: e-mail and cloud user id must both be non-empty after trimming — no-op (no save, no broadcast)."));
		return;
	}

	if (Active->LinkedEmail == TrimmedEmail
		&& Active->CloudUserId == TrimmedUserId
		&& Active->CloudRefreshToken == RefreshToken)
	{
		// Nothing changed — the delegate law forbids broadcasting a no-op.
		UE_LOG(LogSiegeAccount, Verbose,
			TEXT("[SiegeAccount] SetCloudLink with identical values — no-op (no save, no broadcast)."));
		return;
	}

	Active->LinkedEmail       = TrimmedEmail;
	Active->CloudUserId       = TrimmedUserId;
	Active->CloudRefreshToken = RefreshToken;

	SaveAccountsToSlot();

	// ⛔ ACC-§11 TOKEN LAW: the refresh token value is NEVER logged — the line
	// below states only whether one is held. (The e-mail is not token/credential
	// material; the widget prints it on screen as "Linked as <email>".)
	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Profile '%s' cloud-linked as '%s' (cloud user id %s, refresh token: %s)."),
		*Active->DisplayName, *Active->LinkedEmail, *Active->CloudUserId,
		Active->CloudRefreshToken.IsEmpty() ? TEXT("none") : TEXT("held"));

	BroadcastActiveProfileChanged();
}

void USiegeAccountSubsystem::ClearCloudLink()
{
	FSiegeProfileInfo* Active = FindActiveProfileMutable();
	if (!Active)
	{
		UE_LOG(LogSiegeAccount, Verbose,
			TEXT("[SiegeAccount] ClearCloudLink while guest — no-op (no save, no broadcast)."));
		return;
	}

	const bool bHadAnyCloudState =
		!Active->LinkedEmail.IsEmpty()
		|| !Active->CloudUserId.IsEmpty()
		|| !Active->CloudRefreshToken.IsEmpty()
		|| Active->LastSyncUtc != FDateTime();
	if (!bHadAnyCloudState)
	{
		// Already fully unlinked — a complete no-op (the delegate law).
		UE_LOG(LogSiegeAccount, Verbose,
			TEXT("[SiegeAccount] ClearCloudLink on an already-unlinked profile — no-op (no save, no broadcast)."));
		return;
	}

	const FString PreviousEmail = Active->LinkedEmail;

	// Cloud sign-out clears the persisted token (ACC-§11) and the whole cloud
	// identity. LastSyncUtc resets too: a stale sync clock surviving into a
	// future re-link would suppress the ACC-§13 trigger-1 pull for every cloud
	// row older than it (updated_at > LastSyncUtc), silently losing data on a
	// re-link — "never synced" is the only honest state for an unlinked
	// profile. The LOCAL profile — identity, credential, decks, settings —
	// SURVIVES untouched (ACC-§11).
	Active->LinkedEmail.Reset();
	Active->CloudUserId.Reset();
	Active->CloudRefreshToken.Reset();
	Active->LastSyncUtc = FDateTime();

	SaveAccountsToSlot();

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Profile '%s' cloud link cleared (was '%s') — local profile survives; sync clock reset."),
		*Active->DisplayName,
		PreviousEmail.IsEmpty() ? TEXT("<no e-mail stored>") : *PreviousEmail);

	BroadcastActiveProfileChanged();
}

void USiegeAccountSubsystem::SetLastSyncUtc(const FDateTime& WhenUtc)
{
	FSiegeProfileInfo* Active = FindActiveProfileMutable();
	if (!Active)
	{
		// Guest never syncs (ACC-§13) — a stamp with no active profile is a
		// caller error worth a Warning, not a crash.
		UE_LOG(LogSiegeAccount, Warning,
			TEXT("[SiegeAccount] SetLastSyncUtc while guest — no active profile; no-op (no save, no broadcast)."));
		return;
	}

	if (Active->LastSyncUtc == WhenUtc)
	{
		UE_LOG(LogSiegeAccount, Verbose,
			TEXT("[SiegeAccount] SetLastSyncUtc with the already-stored value — no-op (no save, no broadcast)."));
		return;
	}

	Active->LastSyncUtc = WhenUtc;
	SaveAccountsToSlot();

	UE_LOG(LogSiegeAccount, Log,
		TEXT("[SiegeAccount] Profile '%s' LastSyncUtc -> %s (server time — the A3 sync clock)."),
		*Active->DisplayName, *WhenUtc.ToIso8601());

	BroadcastActiveProfileChanged();
}

FDateTime USiegeAccountSubsystem::GetLastSyncUtc() const
{
	// P2.1 (TASK-653, rider R2 — 645-D1's named cure): PURE IN-MEMORY read of
	// the A3 sync clock, consumed by FSiegeCloudSync's MakeContext() as the one
	// pull baseline. Guest => FDateTime() (zero ticks — the never-synced
	// default every real cloud row pulls against, ACC-§13 trigger 1); an
	// unlinked profile's stored value is FDateTime() by contract (fresh profiles
	// default it, ClearCloudLink resets it), so one expression covers both
	// registry-comment cases. No mutation, no save, no broadcast — a getter.
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active ? Active->LastSyncUtc : FDateTime();
}

FString USiegeAccountSubsystem::GetCloudRefreshToken() const
{
	// P2.1 (TASK-653, rider R1): THE ONE LAWFUL READER of the persisted refresh
	// token (the ACC-§11 dated 2026-08-23 addition) — its single sanctioned
	// consumer is the UAccountMenuWidget re-auth path, which hands the value to
	// USiegeCloudClient::RefreshSession's HTTPS grant and NOTHING else.
	// ⛔ ACC-§11/P2-R6: the value is NEVER logged, NEVER displayed, NEVER sent
	// anywhere but that grant — this getter must not grow other callers.
	// Guest/unlinked => empty. No mutation, no save, no broadcast — a getter.
	const FSiegeProfileInfo* Active = FindActiveProfile();
	return Active ? Active->CloudRefreshToken : FString();
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

FSiegeProfileInfo* USiegeAccountSubsystem::FindActiveProfileMutable()
{
	// TASK-644: the Meyers const-twin — implemented ON TOP of the const funnel
	// so there is exactly ONE active-profile predicate in this file and the P2
	// mutators cannot disagree with the P1 getters. The const_cast is sound:
	// Profiles is a non-const member and *this* is non-const here — the
	// underlying storage was never const.
	return const_cast<FSiegeProfileInfo*>(FindActiveProfile());
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
