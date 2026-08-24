// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Siegebound/SiegeAccountSaveGame.h"
#include "SiegeAccountSubsystem.generated.h"

/**
 *  The account log category (CONVENTIONS ACC-§6: LogSiegeAccount is declared and
 *  defined in SiegeAccountSubsystem.{h,cpp} by law — the LogSiegeSettings shape,
 *  cloned).
 */
DECLARE_LOG_CATEGORY_EXTERN(LogSiegeAccount, Log, All);

/**
 *  Fired whenever the ACTIVE PROFILE actually changes — a successful
 *  CreateAccount, a successful Login, or a real Logout. No payload: consumers
 *  re-read the seam (GetDeckSlotName / GetSettingsSlotName /
 *  GetActiveDisplayName), which is the ACC-§4 contract. ⛔ Never broadcast on a
 *  no-op (a Logout while already guest broadcasts nothing — the delegate law).
 *
 *  ⭐ 2026-08-23, TASK-644 (Phase 2): ALSO fired when the active profile's
 *  CLOUD-LINK state actually mutates — SetCloudLink / ClearCloudLink /
 *  SetLastSyncUtc (the TASK-644 spec's save-on-change contract; consumers
 *  additionally re-read IsCloudLinked / GetLinkedEmail). The no-op law is
 *  unchanged: a refused or valueless mutation still broadcasts NOTHING.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSiegeActiveProfileChanged);

/**
 *  THE LOCAL ACCOUNT OWNER (batch ACCOUNTS, TASK-600; CONVENTIONS ACC-§1..§4,
 *  ACC-§6 rows 3–5, and the ACC-§7 pinned signature registry — the public
 *  surface below is registry-pinned character-for-character. ⭐ 2026-08-23,
 *  TASK-644: the Phase-2 cloud-link block is pinned by the ACC-§15 block-4
 *  registry the same way; the Phase-1 surface is untouched).
 *
 *  Owns the in-memory profile registry (loaded once from
 *  USiegeAccountSaveGame::SlotName at Initialize), the create/login/logout
 *  flows, and THE SEAM: every profile-scoped slot name in the game resolves
 *  through GetDeckSlotName() / GetSettingsSlotName() (ACC-§4). It is a
 *  UGameInstanceSubsystem because identity must survive OpenLevel — a login in
 *  L_MainMenu is naturally live when SiegePlayerController loads the active
 *  deck in L_Arena (the USiegeSettingsSubsystem lesson, same clothes).
 *
 *  ⛔ THE GUEST-DEFAULT LAW (ACC-§1): guest is the default and login gates
 *  NOTHING. With no active profile the seam returns the bare shipped constants
 *  (USiegeDeckSaveGame::SlotName / USiegeSettingsSubsystem::SettingsSlotName)
 *  and every shipped flow behaves byte-identically to today. Account code
 *  NEVER mutates the guest slots; CreateAccount SEED-COPIES the guest payloads
 *  into the new profile's slots once, at create only (ACC-§3, RULING 8 /
 *  FLAGGED A6 default TAKEN).
 *
 *  ⛔ THE HONEST-CREDENTIAL LAW (ACC-§2): what this class stores is a LOCAL
 *  CONVENIENCE CREDENTIAL, NOT SECURITY — a salted FSHA1 hex
 *  (hex(SHA1(SaltHex + ":" + Password)), salt = a fresh per-profile FGuid in
 *  Digits hex). It stops shoulder-surfing and accidental plaintext on disk and
 *  nothing more; anyone with disk access can edit SiegeAccounts.sav. Real auth
 *  is the Phase-2 backend's job. The plaintext password parameter is hashed
 *  and DROPPED — never stored, never logged, never echoed into OutReason.
 *
 *  ⚠️ LOAD ONCE, SAVE ON CHANGE, NEVER TOUCH THE DISK FROM A GAMEPLAY PATH
 *  (the settings-lane contract, cloned per ACC-§7). The seam getters are PURE
 *  IN-MEMORY reads, safe at any call site. The only disk reads are
 *  Initialize()'s single load, an explicit LoadAccountsFromSlot() call
 *  (automation only), and CreateAccount's one-time seed-copy (a menu action,
 *  never a match path).
 *
 *  ⚠️ DEPENDENCY DIRECTION (ACC-§4): Settings and Deck code depend on THIS
 *  class; this class references ONLY their two slot-name constants (.cpp
 *  includes only — no Settings/Deck type is used here beyond those statics).
 *
 *  HOW CONSUMERS RESOLVE IT (the USiegeSettingsSubsystem snippet, cloned):
 *
 *      UGameInstance* GI = GetGameInstance();               // or Actor->GetGameInstance()
 *      USiegeAccountSubsystem* Accounts =
 *          GI ? GI->GetSubsystem<USiegeAccountSubsystem>() : nullptr;
 *      const FString Slot = Accounts ? Accounts->GetDeckSlotName()
 *                                    : USiegeDeckSaveGame::SlotName;  // ⚠️ FAIL SAFE = guest
 *
 *  ⛔ AND THE FALLBACK WHEN THE SUBSYSTEM DOES NOT RESOLVE IS THE BARE GUEST
 *  CONSTANT — today's shipped behavior, never a crash (ACC-§4).
 *
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no
 *  new replicated class, no new relevancy tier, no RPC. All account state is
 *  client-local (UGameInstanceSubsystem + local USaveGame); the display name
 *  touches no session/player name (A7). Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeAccountSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	//~ Begin USubsystem interface
	/** Loads the account registry ONCE. Missing / unreadable / foreign-class slot => empty registry + guest, logged once on LogSiegeAccount, never a crash. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End USubsystem interface

	/**
	 *  Creates a local profile and makes it ACTIVE. Validation (ACC-§3, first
	 *  violation into OutReason — the UDeckLibrary::IsDeckLegal idiom):
	 *  trimmed display name 3–24 chars · password ≥ 4 chars · display name
	 *  unique case-insensitive among local profiles. On success: fresh FGuid
	 *  ProfileId + fresh Digits-hex salt, hash per ACC-§2, register, set
	 *  active, save the registry, SEED-COPY the guest decks + settings into
	 *  the new profile's slots (RULING 8 — once, at create only; guest slots
	 *  never mutated), then broadcast OnActiveProfileChanged.
	 *  ⛔ The password parameter is hashed and dropped (ACC-§2).
	 */
	UFUNCTION(BlueprintCallable) bool CreateAccount(const FString& DisplayName, const FString& Password, FString& OutReason);

	/**
	 *  Case-insensitive display-name lookup + salted-hash compare (ACC-§2/§3).
	 *  On success: sets the profile active, stamps LastLoginUtc, saves the
	 *  registry, broadcasts. On failure OutReason says what went wrong —
	 *  ⛔ never echoing the password (ACC-§2).
	 */
	UFUNCTION(BlueprintCallable) bool Login(const FString& DisplayName, const FString& Password, FString& OutReason);

	/**
	 *  Back to guest: clears the active profile, saves the registry,
	 *  broadcasts. A Logout while already guest is a complete no-op — no disk
	 *  write, NO BROADCAST (the delegate law). Never touches any deck/settings
	 *  slot: the guest data was never mutated (ACC-§1).
	 */
	UFUNCTION(BlueprintCallable) void Logout();

	/** PURE IN-MEMORY: true iff a registered profile is active. Guest (the default) => false. */
	UFUNCTION(BlueprintPure) bool    IsLoggedIn() const;

	/** PURE IN-MEMORY: the active profile's display name; EMPTY when guest (the registry contract). */
	UFUNCTION(BlueprintPure) FString GetActiveDisplayName() const;

	/** PURE IN-MEMORY: the active ProfileId; INVALID FGuid when guest (the registry contract). Not a UFUNCTION — FGuid identity is a C++ concern. */
	FGuid GetActiveProfileId() const;

	/**
	 *  THE SEAM, deck lane (ACC-§4): guest => USiegeDeckSaveGame::SlotName
	 *  byte-identical; active profile => SiegeDecks_<Digits>, built FROM that
	 *  constant + MakeProfileSlotSuffix (no slot literal lives in this class).
	 *  The five enumerated deck call sites resolve through here AT CALL TIME.
	 */
	UFUNCTION(BlueprintPure) FString GetDeckSlotName() const;

	/**
	 *  THE SEAM, settings lane (ACC-§4): guest =>
	 *  USiegeSettingsSubsystem::SettingsSlotName byte-identical; active
	 *  profile => SiegeSettings_<Digits>, built from that constant.
	 *  USiegeSettingsSubsystem::ResolveSlotName() consults this BETWEEN its
	 *  test override and its own pinned constant (TASK-601).
	 */
	UFUNCTION(BlueprintPure) FString GetSettingsSlotName() const;

	/** Broadcast on every REAL active-profile change (create / login / logout — and, since TASK-644, every real cloud-link mutation), never on a no-op. Consumers seed from the getters first, then bind. */
	UPROPERTY(BlueprintAssignable) FOnSiegeActiveProfileChanged OnActiveProfileChanged;

	// ── PHASE-2 CLOUD-LINK API (batch ACCOUNTS-P2, TASK-644; the ACC-§15
	// block-4 pinned registry, character-for-character; CONVENTIONS ACC-§11/
	// ACC-§13). ADDITIVE ONLY: the Phase-1 surface above is qa-passed and
	// committed — nothing there moved. ⛔ No HTTP lives here (USiegeCloudClient,
	// TASK-643) and no sync logic (FSiegeCloudSync, TASK-645): this API is the
	// PERSISTENCE of the link — who the active profile is linked to, its
	// refresh token, and the A3 sync clock. ⛔ P2-R6: CredentialHashHex /
	// CredentialSaltHex are NEVER surfaced through any method below — the P1
	// local hash never leaves the machine (ACC-§11).
	//
	// M8 DECLARATION (P2 batch header, verbatim): Adds no replicated property,
	// no new replicated class, no new relevancy tier, no RPC. All cloud traffic
	// is client-local HTTPS from USiegeCloudClient (a UGameInstanceSubsystem);
	// nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1
	// checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

	/** PURE IN-MEMORY: true iff the ACTIVE profile is cloud-linked — LinkedEmail AND CloudUserId both non-empty (the ACC-§15 predicate). Guest => false, always. */
	UFUNCTION(BlueprintPure) bool    IsCloudLinked() const;

	/** PURE IN-MEMORY: the active profile's linked cloud e-mail; EMPTY when guest or unlinked (the widget's "Linked as <email>" source). */
	UFUNCTION(BlueprintPure) FString GetLinkedEmail() const;

	/**
	 *  Records a successful cloud link/sign-in on the ACTIVE profile: stores
	 *  Email (trimmed) + UserId (trimmed) + RefreshToken, saves the registry
	 *  slot, broadcasts OnActiveProfileChanged (the P1 save-on-change
	 *  contract). Mutates ONLY the active profile. No-ops (log line, no save,
	 *  no broadcast — the delegate law): guest · empty Email or UserId after
	 *  trimming (the IsCloudLinked predicate would be broken) · values
	 *  identical to what is already stored. An empty RefreshToken is ACCEPTED
	 *  (the link predicate does not include it; the next sign-in can supply
	 *  one). ⛔ ACC-§11: the RefreshToken value is NEVER logged — the log line
	 *  says only whether one is held.
	 */
	void SetCloudLink(const FString& Email, const FString& UserId, const FString& RefreshToken);

	/**
	 *  Cloud sign-out; the LOCAL profile survives (ACC-§11) — nothing local is
	 *  deleted, the profile merely returns to the unlinked state. Clears ALL
	 *  FOUR cloud fields on the active profile (LinkedEmail, CloudUserId,
	 *  CloudRefreshToken — the ACC-§11 sign-out-clears-it law — AND
	 *  LastSyncUtc: a stale sync clock surviving into a future re-link would
	 *  silently suppress the ACC-§13 trigger-1 pull for every row older than
	 *  it, so the clock resets to "never synced"). Saves the registry,
	 *  broadcasts. Guest or already-unlinked (no cloud state at all) => a
	 *  complete no-op: no save, NO BROADCAST (the delegate law).
	 */
	void ClearCloudLink();

	/**
	 *  Stamps the active profile's LastSyncUtc — SERVER time, supplied by the
	 *  sync engine after a completed SyncNow (A3/ACC-§13 trigger 3; never
	 *  client wall-clock, which the A3 last-write-wins compare cannot trust).
	 *  Saves the registry, broadcasts OnActiveProfileChanged (the TASK-644
	 *  spec's save-on-change contract — the broadcast is what lets the
	 *  settings lane reload freshly PULLED data). Mutates ONLY the active
	 *  profile. Guest, or a value identical to the stored one => no-op: no
	 *  save, no broadcast.
	 */
	void SetLastSyncUtc(const FDateTime& WhenUtc);

	/**
	 *  THE ONE hash implementation (ACC-§2, character-for-character):
	 *  hex(FSHA1::HashBuffer(UTF8(SaltHex + ":" + Password))). Static + pure so
	 *  CreateAccount and Login cannot drift, and so an automation test can
	 *  assert the derivation without a subsystem instance.
	 *  ⛔ Local convenience credential, NOT security (ACC-§2).
	 */
	static FString MakeCredentialHashHex(const FString& Password, const FString& SaltHex);

	/** THE ONE slot-suffix derivation (ACC-§3): ProfileId.ToString(EGuidFormats::Digits) — 32 hex chars, filename-safe. Never derived from the display name. */
	static FString MakeProfileSlotSuffix(const FGuid& ProfileId);

	/**
	 *  ⛔ AUTOMATION TESTS ONLY — nothing in the game may call this (the
	 *  USiegeSettingsSubsystem seam, cloned). Redirects THIS INSTANCE's
	 *  registry load/save to a scratch slot so a test run can never overwrite
	 *  the player's real Saved/SaveGames/SiegeAccounts.sav. An empty string
	 *  restores the pinned USiegeAccountSaveGame::SlotName. It does not touch
	 *  the in-memory registry — a test wanting a clean slate calls
	 *  LoadAccountsFromSlot() after redirecting.
	 */
	void SetSlotNameForAutomationTests(const FString& InSlotName);

	/**
	 *  Loads the account registry into memory (missing / unreadable /
	 *  foreign-class => empty registry + guest + one log line; a persisted
	 *  ActiveProfileId that matches no profile is sanitized back to guest).
	 *  ⛔ NEVER writes the disk and NEVER broadcasts (Initialize-time — no
	 *  consumer can be bound yet; the LoadSettingsFromSlot precedent).
	 *
	 *  ⛔ NOT A GAMEPLAY PATH. The game calls this exactly once, from
	 *  Initialize(). Public only so an automation test can drive a reload
	 *  after SetSlotNameForAutomationTests without fabricating an
	 *  FSubsystemCollectionBase.
	 */
	void LoadAccountsFromSlot();

	/**
	 *  DIAGNOSTICS + THE AUTOMATION TESTS' OBSERVATION POINT (the
	 *  SettingsChangeBroadcastCount precedent, cloned): how many times
	 *  OnActiveProfileChanged has been broadcast this session. Bumped inside
	 *  BroadcastActiveProfileChanged(), the ONLY place in the .cpp that calls
	 *  OnActiveProfileChanged.Broadcast — so the count cannot diverge from the
	 *  broadcasts. Read-only for consumers; no logic anywhere reads it.
	 */
	int32 ActiveProfileChangedBroadcastCount = 0;

private:

	/** The active profile's registry entry, or nullptr when guest / when the id matches no profile (defensive — the getters and the seam all funnel through here so they cannot disagree). */
	const FSiegeProfileInfo* FindActiveProfile() const;

	/** The non-const twin (TASK-644) for the cloud-link mutators — implemented ON TOP of FindActiveProfile (one predicate, structurally incapable of disagreeing with the P1 getters about which profile is active). */
	FSiegeProfileInfo* FindActiveProfileMutable();

	/** Case-insensitive display-name lookup among local profiles (ACC-§3). Expects a TRIMMED name. */
	FSiegeProfileInfo* FindProfileByDisplayName(const FString& TrimmedName);

	/** Serializes the in-memory registry to the resolved slot. Returns false (and logs a Warning) when the write fails; the in-memory change is DELIBERATELY not reverted (the settings-lane ruling). */
	bool SaveAccountsToSlot() const;

	/** The ONLY caller of OnActiveProfileChanged.Broadcast — also bumps the diagnostics counter and logs the change. */
	void BroadcastActiveProfileChanged();

	/**
	 *  RULING 8 seed-copy: if the guest slot exists, load it (as a plain
	 *  USaveGame — no Deck/Settings class is named, preserving the ACC-§4
	 *  dependency direction) and re-save the SAME object under the profile
	 *  slot. The guest original is read, never written. A missing guest slot
	 *  is silent (a fresh install has nothing to seed); a failed copy is a
	 *  Warning, never a failed CreateAccount.
	 */
	static void SeedCopySlot(const FString& GuestSlotName, int32 InUserIndex, const FString& ProfileSlotName);

	/** SlotNameOverride when a test set one, otherwise the pinned USiegeAccountSaveGame::SlotName (the ResolveSlotName shape, cloned). */
	FString ResolveSlotName() const;

	/** In-memory profile registry — the single authority between Initialize's load and each save-on-change. */
	TArray<FSiegeProfileInfo> Profiles;

	/** Active profile id; INVALID = guest (ACC-§1). Persisted in the registry slot, so a login survives a game restart. */
	FGuid ActiveProfileId;

	/** ⛔ Automation only (SetSlotNameForAutomationTests). Empty in every shipped path. */
	FString SlotNameOverride;

	/** Latches the "no readable registry, empty + guest" line to ONE emission per subsystem instance. */
	bool bLoadFallbackLogged = false;
};
