// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/StringConv.h"
#include "Containers/UnrealString.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Char.h"
#include "Misc/Guid.h"
#include "Misc/SecureHash.h"
#include "Siegebound/SiegeAccountSaveGame.h"
#include "Siegebound/SiegeAccountSubsystem.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegeSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the Phase-1 local account shell (batch ACCOUNTS,
 *  TASK-604; CONVENTIONS ACC-§1..§4 + ACC-§7, SC-§13).
 *
 *  ⚠️ WRITTEN AGAINST THE ACC-§7 PINNED SIGNATURE REGISTRY, NOT AGAINST THE
 *  SIBLING TASKS' IN-FLIGHT FILES. TASK-599 (the model) and TASK-600 (the
 *  subsystem) are authored in parallel with this file; the registry is the
 *  cross-task contract, character-for-character, and the compile gate
 *  (TASK-606) is where the two sides reconcile. A test here that fails at the
 *  gate against a registry-conformant implementation is MY defect; one that
 *  fails against a registry deviation is a FINDING.
 *
 *  WHY THIS IS THE PURE-FUNCTION/UNIT LANE: the account shell needs no UWorld,
 *  no PIE, no widget and no model. It is two statics, a registry array, three
 *  state transitions and two composed strings — all assertable in-process.
 *  Everything NOT assertable here is NAMED in handoffs/TASK-604-programmer.md
 *  rather than implied (registry reload across instances, the
 *  OnActiveProfileChanged broadcast count, RULING-8 seed-copy correctness, and
 *  the consumer-side no-subsystem fallback owned by TASK-601/602 — the last
 *  three close at TASK-605 review + TASK-609 pixels).
 *
 *  ⛔ THE TESTS NEVER WRITE THE PLAYER'S REAL SLOTS, AND THE GUARANTEE IS
 *  MECHANICAL (the SiegeSettingsTest idiom, cloned):
 *    - Every subsystem instance is redirected to the scratch registry slot
 *      "SiegeAccounts_AutomationScratch" via SetSlotNameForAutomationTests
 *      BEFORE any operation, so no test can address Saved/SaveGames/
 *      SiegeAccounts.sav. The SlotContract test asserts the two names differ.
 *    - CreateAccount performs the RULING-8 seed-copy: it READS the guest
 *      "SiegeDecks"/"SiegeSettings" slots if they exist on this machine (reads
 *      only — ACC-§1 says account code never mutates the guest slots) and
 *      WRITES "SiegeDecks_<Digits>"/"SiegeSettings_<Digits>" copies. Those
 *      suffixes are fresh GUIDs that exist nowhere else, and
 *      FAccountScratchGuard tracks every profile a test creates and deletes
 *      both copies on the way out. On a CI-clean machine the guest slots do
 *      not exist, so no copy is made at all — the tests pass either way and
 *      leave zero files behind in both worlds.
 *
 *  ⚠️ THE ONE PIECE OF ENGINE PLUMBING THAT IS NOT OPTIONAL (cloned from
 *  SiegeSettingsTest.cpp): UGameInstanceSubsystem is UCLASS(Abstract,
 *  Within = GameInstance), so the subsystem MUST be constructed with a
 *  UGameInstance outer or StaticAllocateObject trips the ClassWithin check.
 *  Initialize(FSubsystemCollectionBase&) is never called — a collection cannot
 *  be fabricated outside the engine's own creation path — so every store below
 *  starts with an EMPTY in-memory registry, which is exactly the CI-clean
 *  first-boot state the task requires the tests to hold in.
 *
 *  ACC-§2, repeated because comments are artifacts too: the credential fields
 *  exercised here are a LOCAL CONVENIENCE CREDENTIAL — real auth is the
 *  Phase-2 backend's job. Nothing in this file may (or does) call them secure.
 *  Assertion messages deliberately never embed a password value, so a test log
 *  cannot become the password sink ACC-§2 forbids.
 */

namespace SiegeAccountTestUtils
{
	/** ⛔ NEVER the shipped registry slot. Asserted mechanically in the SlotContract test, not assumed. */
	static const TCHAR* ScratchRegistrySlotName = TEXT("SiegeAccounts_AutomationScratch");

	static void DeleteSlotIfPresent(const FString& SlotName, int32 UserIndex)
	{
		if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
		{
			UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
		}
	}

	static void DeleteScratchRegistrySlot()
	{
		DeleteSlotIfPresent(ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex);
	}

	/**
	 *  Deletes the scratch registry slot on the way in AND out, and deletes the
	 *  seed-copy artifacts ("SiegeDecks_<Digits>" / "SiegeSettings_<Digits>")
	 *  of every profile the test registered via TrackActiveProfile. Only slots
	 *  this test run created can match those names — the suffix is a fresh
	 *  FGuid — so the janitor cannot touch a real player profile.
	 */
	struct FAccountScratchGuard
	{
		TArray<FString> ProfileSuffixes;

		FAccountScratchGuard()
		{
			DeleteScratchRegistrySlot();
		}

		~FAccountScratchGuard()
		{
			DeleteScratchRegistrySlot();
			for (const FString& Suffix : ProfileSuffixes)
			{
				DeleteSlotIfPresent(FString::Printf(TEXT("SiegeDecks_%s"), *Suffix), USiegeDeckSaveGame::UserIndex);
				DeleteSlotIfPresent(FString::Printf(TEXT("SiegeSettings_%s"), *Suffix), USiegeSettingsSubsystem::SettingsUserIndex);
			}
		}

		/** Call immediately after every SUCCESSFUL CreateAccount, while the new profile is still active. */
		void TrackActiveProfile(const USiegeAccountSubsystem& Accounts)
		{
			ProfileSuffixes.AddUnique(USiegeAccountSubsystem::MakeProfileSlotSuffix(Accounts.GetActiveProfileId()));
		}
	};

	/**
	 *  An account subsystem wired to the scratch registry slot, plus the
	 *  throwaway UGameInstance it must live inside (Within = GameInstance).
	 *  Both held by TStrongObjectPtr so a mid-test GC pass cannot collect
	 *  either — the Outer chain does NOT keep an object alive on its own.
	 */
	struct FScratchAccounts
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<USiegeAccountSubsystem> Accounts;

		bool IsValid() const { return GameInstance.IsValid() && Accounts.IsValid(); }
	};

	static FScratchAccounts MakeScratchAccounts()
	{
		FScratchAccounts Store;
		Store.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		if (Store.GameInstance.IsValid())
		{
			Store.Accounts.Reset(NewObject<USiegeAccountSubsystem>(Store.GameInstance.Get()));
			if (Store.Accounts.IsValid())
			{
				// BEFORE any operation — this is what keeps the player's real
				// SiegeAccounts.sav untouched (the ACC-§7 seam, cloned from
				// USiegeSettingsSubsystem).
				Store.Accounts->SetSlotNameForAutomationTests(ScratchRegistrySlotName);
			}
		}
		return Store;
	}
}

/**
 *  THE SLOT CONTRACT. A changed registry slot name silently orphans every
 *  local profile; changed guest constants orphan every player's decks and
 *  settings. All three strings are asserted here rather than trusted to
 *  review, byte-for-byte (SC-§13), against the ACC-§3 table.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountSlotContractTest,
	"Siegebound.Account.SlotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountSlotContractTest::RunTest(const FString& Parameters)
{
	TestEqualSensitive(TEXT("USiegeAccountSaveGame::SlotName is exactly \"SiegeAccounts\""),
		FString(USiegeAccountSaveGame::SlotName), FString(TEXT("SiegeAccounts")));

	TestEqual(TEXT("The account registry uses user index 0"),
		static_cast<int32>(USiegeAccountSaveGame::UserIndex), 0);

	// The guest halves of the ACC-§3 composed-name table. These are the shipped
	// constants ACC-§4 says stay byte-identical; the profile-scoped tests below
	// compose against these exact strings.
	TestEqualSensitive(TEXT("The guest deck slot is exactly \"SiegeDecks\""),
		FString(USiegeDeckSaveGame::SlotName), FString(TEXT("SiegeDecks")));
	TestEqualSensitive(TEXT("The guest settings slot is exactly \"SiegeSettings\""),
		FString(USiegeSettingsSubsystem::SettingsSlotName), FString(TEXT("SiegeSettings")));

	// The hermeticity guarantee, made mechanical: no test in this file can
	// address the shipped registry slot, because the scratch name is a
	// different string.
	TestNotEqual(TEXT("The automation scratch slot is NOT the shipped registry slot"),
		FString(SiegeAccountTestUtils::ScratchRegistrySlotName), FString(USiegeAccountSaveGame::SlotName));

	return true;
}

/**
 *  MakeCredentialHashHex: DETERMINISTIC, SALT-SENSITIVE, PASSWORD-SENSITIVE,
 *  AND EXACTLY THE ACC-§2 FORMULA — hex(FSHA1::HashBuffer(UTF8(Salt + ":" +
 *  Password))). Determinism is what makes Login's hash-compare meaningful;
 *  salt sensitivity is what stops two profiles with the same password from
 *  storing the same hash.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountCredentialHashTest,
	"Siegebound.Account.CredentialHashContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountCredentialHashTest::RunTest(const FString& Parameters)
{
	const FString Password = TEXT("correct horse battery");
	const FString OtherPassword = TEXT("correct horse batterz");
	const FString SaltA = TEXT("00112233445566778899AABBCCDDEEFF");
	const FString SaltB = TEXT("00112233445566778899AABBCCDDEE00");

	const FString HashA1 = USiegeAccountSubsystem::MakeCredentialHashHex(Password, SaltA);
	const FString HashA2 = USiegeAccountSubsystem::MakeCredentialHashHex(Password, SaltA);
	const FString HashB = USiegeAccountSubsystem::MakeCredentialHashHex(Password, SaltB);
	const FString HashC = USiegeAccountSubsystem::MakeCredentialHashHex(OtherPassword, SaltA);

	// Byte-identity claim ⇒ TestEqualSensitive (SC-§13).
	TestEqualSensitive(TEXT("Same password + same salt hashes identically twice (determinism)"),
		HashA1, HashA2);

	TestNotEqual(TEXT("Same password + a DIFFERENT salt hashes differently (salt sensitivity)"),
		HashA1, HashB);
	TestNotEqual(TEXT("A different password + the same salt hashes differently (password sensitivity)"),
		HashA1, HashC);

	// SHA-1 is 20 bytes ⇒ 40 hex characters, nothing else.
	TestEqual(TEXT("The hash is 40 characters (hex of a 20-byte SHA-1 digest)"), HashA1.Len(), 40);
	bool bAllHex = HashA1.Len() > 0;
	for (const TCHAR HashChar : HashA1)
	{
		bAllHex = bAllHex && FChar::IsHexDigit(HashChar);
	}
	TestTrue(TEXT("Every hash character is a hex digit"), bAllHex);

	// The ACC-§2 formula, computed INDEPENDENTLY here so the implementation
	// cannot drift from the law without this failing:
	//   CredentialHashHex = hex(FSHA1::HashBuffer(UTF8(SaltHex + ":" + Password)))
	const FString Combined = SaltA + TEXT(":") + Password;
	FTCHARToUTF8 CombinedUtf8(*Combined);
	uint8 Digest[FSHA1::DigestSize] = { 0 };
	FSHA1::HashBuffer(CombinedUtf8.Get(), CombinedUtf8.Length(), Digest);
	const FString ExpectedHex = BytesToHex(Digest, FSHA1::DigestSize);

	// ⚠️ DELIBERATE plain TestEqual, and this is NOT the SC-§13 trap: ACC-§2
	// pins the digest BYTES, not the hex CASE, so a case-insensitive compare is
	// exactly the pinned claim. Hex digits of different bytes can never differ
	// by case alone, so this assertion still fails on any byte deviation.
	// Every casing/byte-identity claim elsewhere in this file uses
	// TestEqualSensitive.
	TestEqual(TEXT("MakeCredentialHashHex matches the ACC-§2 formula computed independently"),
		HashA1, ExpectedHex);

	return true;
}

/**
 *  MakeProfileSlotSuffix IS ProfileId.ToString(EGuidFormats::Digits): 32 hex
 *  characters, no hyphens or braces, deterministic per GUID — so a profile's
 *  save file names are stable across sessions and machines (ACC-§3).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountProfileSlotSuffixTest,
	"Siegebound.Account.ProfileSlotSuffix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountProfileSlotSuffixTest::RunTest(const FString& Parameters)
{
	const FGuid KnownGuid(0x00112233, 0x44556677, 0x8899AABB, 0xCCDDEEFF);
	const FString Suffix = USiegeAccountSubsystem::MakeProfileSlotSuffix(KnownGuid);

	// The law's ONE implementation claim: the helper returns exactly what the
	// engine's Digits format returns — character-for-character (SC-§13).
	TestEqualSensitive(TEXT("MakeProfileSlotSuffix equals ToString(EGuidFormats::Digits) character-for-character"),
		Suffix, KnownGuid.ToString(EGuidFormats::Digits));

	TestEqual(TEXT("The suffix is 32 characters"), Suffix.Len(), 32);
	bool bAllHex = Suffix.Len() > 0;
	for (const TCHAR SuffixChar : Suffix)
	{
		bAllHex = bAllHex && FChar::IsHexDigit(SuffixChar);
	}
	TestTrue(TEXT("Every suffix character is a hex digit (filename-safe, no hyphens/braces)"), bAllHex);

	// Determinism made concrete: the Digits rendering of this exact GUID. If
	// this ever fails while the ToString assertion above passes, the ENGINE's
	// Digits format changed — a finding, not a test bug.
	TestEqualSensitive(TEXT("The suffix of the known GUID is the expected 32-hex rendering"),
		Suffix, FString(TEXT("00112233445566778899AABBCCDDEEFF")));

	TestEqual(TEXT("The same GUID always yields the same suffix"),
		USiegeAccountSubsystem::MakeProfileSlotSuffix(KnownGuid), Suffix);

	return true;
}

/**
 *  GUEST FALLBACK: a store with NO active profile hands back the bare shipped
 *  constants — today's behavior, byte-identical (ACC-§1/§4). Also the CI-clean
 *  cold boot: a login against a completely empty registry rejects with a
 *  reason and writes nothing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountGuestFallbackTest,
	"Siegebound.Account.GuestFallbackIsBareConstants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountGuestFallbackTest::RunTest(const FString& Parameters)
{
	SiegeAccountTestUtils::FAccountScratchGuard Guard;

	SiegeAccountTestUtils::FScratchAccounts Store = SiegeAccountTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	TestFalse(TEXT("A fresh store is not logged in"), Store.Accounts->IsLoggedIn());
	TestTrue(TEXT("A fresh store has an empty active display name"), Store.Accounts->GetActiveDisplayName().IsEmpty());
	TestFalse(TEXT("A fresh store has an INVALID active profile id (invalid = guest)"),
		Store.Accounts->GetActiveProfileId().IsValid());

	// The seam's guest answers, byte-for-byte the shipped constants (SC-§13).
	TestEqualSensitive(TEXT("Guest GetDeckSlotName() is the bare shipped deck constant"),
		Store.Accounts->GetDeckSlotName(), FString(USiegeDeckSaveGame::SlotName));
	TestEqualSensitive(TEXT("Guest GetDeckSlotName() is exactly \"SiegeDecks\""),
		Store.Accounts->GetDeckSlotName(), FString(TEXT("SiegeDecks")));
	TestEqualSensitive(TEXT("Guest GetSettingsSlotName() is the bare shipped settings constant"),
		Store.Accounts->GetSettingsSlotName(), FString(USiegeSettingsSubsystem::SettingsSlotName));
	TestEqualSensitive(TEXT("Guest GetSettingsSlotName() is exactly \"SiegeSettings\""),
		Store.Accounts->GetSettingsSlotName(), FString(TEXT("SiegeSettings")));

	// CI-clean cold boot: nothing on disk, nothing in memory — a login must
	// reject with a populated reason and must not crash.
	FString Reason;
	TestFalse(TEXT("Login against an empty registry is rejected"),
		Store.Accounts->Login(TEXT("NobodyHere"), TEXT("irrelevant"), Reason));
	TestFalse(TEXT("The empty-registry rejection populates OutReason"), Reason.IsEmpty());
	TestFalse(TEXT("A rejected login leaves the store logged out"), Store.Accounts->IsLoggedIn());

	// Load-once/save-on-CHANGE (the ACC-§7 tail): a rejected login changed
	// nothing, so it may not have written the registry slot.
	TestFalse(TEXT("A rejected login writes no registry file"),
		UGameplayStatics::DoesSaveGameExist(SiegeAccountTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	return true;
}

/**
 *  CREATE → LOGOUT → LOGIN ROUND TRIP on the scratch registry slot, plus the
 *  ACC-§3 composed slot names character-for-character and the case-insensitive
 *  login lookup. Single-instance on purpose: Initialize is the only registry
 *  LOAD path in the ACC-§7 registry and cannot be driven from a test (see the
 *  file header), so cross-instance reload closes at TASK-609, not here — and
 *  that gap is stated rather than papered over.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountRoundTripTest,
	"Siegebound.Account.CreateLoginLogoutRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeAccountTestUtils::FAccountScratchGuard Guard;

	SiegeAccountTestUtils::FScratchAccounts Store = SiegeAccountTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	TestFalse(TEXT("Pre-condition: the scratch registry slot does not exist"),
		UGameplayStatics::DoesSaveGameExist(SiegeAccountTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	// CREATE.
	FString Reason;
	TestTrue(TEXT("CreateAccount with a valid name and password succeeds"),
		Store.Accounts->CreateAccount(TEXT("Jonathan"), TEXT("hunter2"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	TestTrue(TEXT("Create sets the new profile active (logged in)"), Store.Accounts->IsLoggedIn());
	TestEqualSensitive(TEXT("The active display name is the created name, casing preserved"),
		Store.Accounts->GetActiveDisplayName(), FString(TEXT("Jonathan")));

	const FGuid CreatedId = Store.Accounts->GetActiveProfileId();
	TestTrue(TEXT("The created profile has a VALID ProfileId"), CreatedId.IsValid());

	// The ACC-§3 composed-name table, character-for-character (SC-§13). The
	// literal prefixes are the law's table; the SlotContract test pins the
	// guest constants to the same literals, so the two cannot drift apart.
	const FString Suffix = USiegeAccountSubsystem::MakeProfileSlotSuffix(CreatedId);
	TestEqualSensitive(TEXT("Logged-in GetDeckSlotName() is exactly \"SiegeDecks_<Digits>\""),
		Store.Accounts->GetDeckSlotName(), FString::Printf(TEXT("SiegeDecks_%s"), *Suffix));
	TestEqualSensitive(TEXT("Logged-in GetSettingsSlotName() is exactly \"SiegeSettings_<Digits>\""),
		Store.Accounts->GetSettingsSlotName(), FString::Printf(TEXT("SiegeSettings_%s"), *Suffix));
	TestNotEqual(TEXT("The profile deck slot differs from the guest deck slot"),
		Store.Accounts->GetDeckSlotName(), FString(USiegeDeckSaveGame::SlotName));
	TestNotEqual(TEXT("The profile settings slot differs from the guest settings slot"),
		Store.Accounts->GetSettingsSlotName(), FString(USiegeSettingsSubsystem::SettingsSlotName));

	// Save-on-change: the successful create wrote the (scratch) registry.
	TestTrue(TEXT("A successful create wrote the registry slot to disk"),
		UGameplayStatics::DoesSaveGameExist(SiegeAccountTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	// LOGOUT ⇒ guest again, bare constants again.
	Store.Accounts->Logout();
	TestFalse(TEXT("Logout leaves the store logged out"), Store.Accounts->IsLoggedIn());
	TestFalse(TEXT("Logout invalidates the active profile id"), Store.Accounts->GetActiveProfileId().IsValid());
	TestTrue(TEXT("Logout empties the active display name"), Store.Accounts->GetActiveDisplayName().IsEmpty());
	TestEqualSensitive(TEXT("After logout GetDeckSlotName() is the bare guest constant again"),
		Store.Accounts->GetDeckSlotName(), FString(USiegeDeckSaveGame::SlotName));
	TestEqualSensitive(TEXT("After logout GetSettingsSlotName() is the bare guest constant again"),
		Store.Accounts->GetSettingsSlotName(), FString(USiegeSettingsSubsystem::SettingsSlotName));

	// LOGIN with the correct password ⇒ the SAME profile returns.
	Reason.Reset();
	TestTrue(TEXT("Login with the correct password succeeds"),
		Store.Accounts->Login(TEXT("Jonathan"), TEXT("hunter2"), Reason));
	TestTrue(TEXT("Re-login is logged in"), Store.Accounts->IsLoggedIn());
	TestTrue(TEXT("Re-login resolves to the SAME ProfileId the create produced"),
		Store.Accounts->GetActiveProfileId() == CreatedId);
	TestEqualSensitive(TEXT("Re-login re-resolves the same profile deck slot"),
		Store.Accounts->GetDeckSlotName(), FString::Printf(TEXT("SiegeDecks_%s"), *Suffix));

	// CASE-INSENSITIVE LOOKUP (ACC-§3): the lookup ignores case, the STORED
	// name keeps its created casing.
	Store.Accounts->Logout();
	Reason.Reset();
	TestTrue(TEXT("Login with an UPPERCASED spelling of the name succeeds (case-insensitive lookup)"),
		Store.Accounts->Login(TEXT("JONATHAN"), TEXT("hunter2"), Reason));
	TestTrue(TEXT("The case-variant login resolves to the same ProfileId"),
		Store.Accounts->GetActiveProfileId() == CreatedId);
	TestEqualSensitive(TEXT("The display name still carries the CREATED casing, not the typed one"),
		Store.Accounts->GetActiveDisplayName(), FString(TEXT("Jonathan")));

	return true;
}

/**
 *  TWO PROFILES ⇒ DISJOINT SLOT NAMES. The whole point of profile scoping:
 *  each profile owns its own deck/settings files, neither collides with the
 *  other or with guest, and logging back into the first re-resolves ITS slots.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountTwoProfilesTest,
	"Siegebound.Account.TwoProfilesDisjointSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountTwoProfilesTest::RunTest(const FString& Parameters)
{
	SiegeAccountTestUtils::FAccountScratchGuard Guard;

	SiegeAccountTestUtils::FScratchAccounts Store = SiegeAccountTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	FString Reason;
	TestTrue(TEXT("Creating profile A succeeds"),
		Store.Accounts->CreateAccount(TEXT("AliceProfile"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);
	const FGuid IdA = Store.Accounts->GetActiveProfileId();
	const FString DeckSlotA = Store.Accounts->GetDeckSlotName();
	const FString SettingsSlotA = Store.Accounts->GetSettingsSlotName();

	Reason.Reset();
	TestTrue(TEXT("Creating profile B succeeds"),
		Store.Accounts->CreateAccount(TEXT("BobProfile"), TEXT("password2"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);
	const FGuid IdB = Store.Accounts->GetActiveProfileId();
	const FString DeckSlotB = Store.Accounts->GetDeckSlotName();
	const FString SettingsSlotB = Store.Accounts->GetSettingsSlotName();

	TestTrue(TEXT("Create B sets B active (a valid id that is not A's)"), IdB.IsValid());
	TestFalse(TEXT("The two profiles have DIFFERENT ProfileIds (fresh FGuid per create)"), IdA == IdB);

	TestNotEqual(TEXT("The two profiles' deck slots are disjoint"), DeckSlotA, DeckSlotB);
	TestNotEqual(TEXT("The two profiles' settings slots are disjoint"), SettingsSlotA, SettingsSlotB);
	TestNotEqual(TEXT("Profile A's deck slot is not the guest slot"), DeckSlotA, FString(USiegeDeckSaveGame::SlotName));
	TestNotEqual(TEXT("Profile B's deck slot is not the guest slot"), DeckSlotB, FString(USiegeDeckSaveGame::SlotName));
	TestNotEqual(TEXT("Profile A's settings slot is not the guest slot"),
		SettingsSlotA, FString(USiegeSettingsSubsystem::SettingsSlotName));
	TestNotEqual(TEXT("Profile B's settings slot is not the guest slot"),
		SettingsSlotB, FString(USiegeSettingsSubsystem::SettingsSlotName));

	// Switching back by login re-resolves A's slots, byte-for-byte.
	Store.Accounts->Logout();
	Reason.Reset();
	TestTrue(TEXT("Logging back into profile A succeeds"),
		Store.Accounts->Login(TEXT("AliceProfile"), TEXT("password1"), Reason));
	TestEqualSensitive(TEXT("Profile A's deck slot re-resolves identically after the switch"),
		Store.Accounts->GetDeckSlotName(), DeckSlotA);
	TestEqualSensitive(TEXT("Profile A's settings slot re-resolves identically after the switch"),
		Store.Accounts->GetSettingsSlotName(), SettingsSlotA);

	return true;
}

/**
 *  EVERY REJECTION CLASS POPULATES OutReason AND LEAVES STATE UNDISTURBED
 *  (the IsDeckLegal idiom): name too short / too long / whitespace-only,
 *  password under the ACC-§3 bar, duplicate display name (exact, case-variant
 *  and padded — uniqueness is case-insensitive on the TRIMMED name), unknown
 *  login name, wrong password. Boundary creates (3- and 24-char names, 4-char
 *  password) must SUCCEED, and a correct login lands at the end as the anchor
 *  proving the rejections above were selective, not "everything fails".
 *  OutReason is re-emptied before every call so a stale value can never
 *  satisfy the populated-assert (the vacuous-assert precedent).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAccountRejectionsTest,
	"Siegebound.Account.RejectionsPopulateOutReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAccountRejectionsTest::RunTest(const FString& Parameters)
{
	SiegeAccountTestUtils::FAccountScratchGuard Guard;

	SiegeAccountTestUtils::FScratchAccounts Store = SiegeAccountTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	FString Reason;

	// ── Create rejections while guest ────────────────────────────────────────
	Reason.Reset();
	TestFalse(TEXT("Create with an EMPTY name is rejected"),
		Store.Accounts->CreateAccount(TEXT(""), TEXT("password1"), Reason));
	TestFalse(TEXT("The empty-name rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("Create with a WHITESPACE-ONLY name is rejected (trim first, ACC-§3)"),
		Store.Accounts->CreateAccount(TEXT("   "), TEXT("password1"), Reason));
	TestFalse(TEXT("The whitespace-name rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("Create with a name under 3 chars AFTER trim is rejected"),
		Store.Accounts->CreateAccount(TEXT("  ab  "), TEXT("password1"), Reason));
	TestFalse(TEXT("The short-name rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("Create with a 25-char name is rejected (max is 24)"),
		Store.Accounts->CreateAccount(TEXT("AbcdefghijklmnopqrstuvwXY"), TEXT("password1"), Reason));
	TestFalse(TEXT("The long-name rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("Create with an EMPTY password is rejected"),
		Store.Accounts->CreateAccount(TEXT("ValidName"), TEXT(""), Reason));
	TestFalse(TEXT("The empty-password rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("Create with a 3-char password is rejected (bar is >= 4, ACC-§3)"),
		Store.Accounts->CreateAccount(TEXT("ValidName"), TEXT("abc"), Reason));
	TestFalse(TEXT("The short-password rejection populates OutReason"), Reason.IsEmpty());

	TestFalse(TEXT("No rejected create logged anybody in"), Store.Accounts->IsLoggedIn());

	// ── Boundary creates that must SUCCEED ──────────────────────────────────
	Reason.Reset();
	TestTrue(TEXT("A 3-char name is accepted (the minimum boundary)"),
		Store.Accounts->CreateAccount(TEXT("Zed"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	Reason.Reset();
	TestTrue(TEXT("A 24-char name is accepted (the maximum boundary)"),
		Store.Accounts->CreateAccount(TEXT("AbcdefghijklmnopqrstuvwX"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	Reason.Reset();
	TestTrue(TEXT("A 4-char password is accepted (the minimum boundary)"),
		Store.Accounts->CreateAccount(TEXT("BoundaryPw"), TEXT("1234"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	// ── Duplicate-name rejections (uniqueness is case-insensitive, ACC-§3) ──
	Reason.Reset();
	TestTrue(TEXT("Creating the reference profile succeeds"),
		Store.Accounts->CreateAccount(TEXT("AliceProfile"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);
	const FGuid AliceId = Store.Accounts->GetActiveProfileId();

	Reason.Reset();
	TestFalse(TEXT("An EXACT duplicate display name is rejected"),
		Store.Accounts->CreateAccount(TEXT("AliceProfile"), TEXT("otherpass"), Reason));
	TestFalse(TEXT("The exact-duplicate rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("A CASE-VARIANT duplicate display name is rejected (uniqueness is case-insensitive)"),
		Store.Accounts->CreateAccount(TEXT("aliceprofile"), TEXT("otherpass"), Reason));
	TestFalse(TEXT("The case-variant-duplicate rejection populates OutReason"), Reason.IsEmpty());

	Reason.Reset();
	TestFalse(TEXT("A PADDED duplicate display name is rejected (trim before uniqueness)"),
		Store.Accounts->CreateAccount(TEXT("  AliceProfile  "), TEXT("otherpass"), Reason));
	TestFalse(TEXT("The padded-duplicate rejection populates OutReason"), Reason.IsEmpty());

	TestTrue(TEXT("The rejected duplicates left the ACTIVE profile undisturbed"),
		Store.Accounts->GetActiveProfileId() == AliceId);
	TestTrue(TEXT("The rejected duplicates left the store logged in"), Store.Accounts->IsLoggedIn());

	// ── Login rejections ─────────────────────────────────────────────────────
	Store.Accounts->Logout();

	Reason.Reset();
	TestFalse(TEXT("Login with an UNKNOWN name is rejected"),
		Store.Accounts->Login(TEXT("NobodyHere"), TEXT("password1"), Reason));
	TestFalse(TEXT("The unknown-name rejection populates OutReason"), Reason.IsEmpty());
	TestFalse(TEXT("The unknown-name rejection leaves the store logged out"), Store.Accounts->IsLoggedIn());

	Reason.Reset();
	TestFalse(TEXT("Login with the WRONG password is rejected"),
		Store.Accounts->Login(TEXT("AliceProfile"), TEXT("not the password"), Reason));
	TestFalse(TEXT("The wrong-password rejection populates OutReason"), Reason.IsEmpty());
	TestFalse(TEXT("The wrong-password rejection leaves the store logged out"), Store.Accounts->IsLoggedIn());
	TestEqualSensitive(TEXT("After the wrong-password rejection the deck slot is still the guest constant"),
		Store.Accounts->GetDeckSlotName(), FString(USiegeDeckSaveGame::SlotName));

	// ── The anchor: the SAME store still accepts the correct credentials ────
	Reason.Reset();
	TestTrue(TEXT("Login with the CORRECT password still succeeds (the rejections were selective)"),
		Store.Accounts->Login(TEXT("AliceProfile"), TEXT("password1"), Reason));
	TestTrue(TEXT("The anchor login resolves to the reference profile"),
		Store.Accounts->GetActiveProfileId() == AliceId);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
