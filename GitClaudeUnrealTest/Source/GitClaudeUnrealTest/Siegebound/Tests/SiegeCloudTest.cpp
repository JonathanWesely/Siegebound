// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Timespan.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Siegebound/SiegeAccountSaveGame.h"
#include "Siegebound/SiegeAccountSubsystem.h"
#include "Siegebound/SiegeCloudClient.h"
#include "Siegebound/SiegeCloudSync.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegeSettingsSubsystem.h"
#include "Templates/SharedPointer.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  OFFLINE AUTOMATION TESTS for the Phase-2 cloud client lane (batch ACCOUNTS
 *  PHASE 2, TASK-647; CONVENTIONS ACC-§10..§15, SC-§13).
 *
 *  ⚠️ WRITTEN AGAINST THE ACC-§15 PINNED SIGNATURE REGISTRY, NOT AGAINST THE
 *  SIBLING TASKS' IN-FLIGHT FILES. TASK-643 (USiegeCloudClient), TASK-644 (the
 *  cloud-link model) and TASK-645 (FSiegeCloudSync) are authored in parallel
 *  with this file; the registry is the cross-task contract,
 *  character-for-character, and the compile gate (TASK-649) is where the two
 *  sides reconcile. A test here that fails at the gate against a
 *  registry-conformant implementation is MY defect; one that fails against a
 *  registry deviation is a FINDING.
 *
 *  ⛔ THE ZERO-NETWORK LAW, AND WHY IT HOLDS MECHANICALLY — no test in this
 *  file can touch an HTTP endpoint:
 *    - No test ever calls SignUp / SignIn / RefreshSession / FetchRows /
 *      UpsertRow / PullAll / PushAll / SyncNow — the only registry members
 *      that perform I/O.
 *    - The one USiegeCloudClient instance constructed here is NEVER
 *      Initialize()d (a FSubsystemCollectionBase cannot be fabricated outside
 *      the engine's creation path — the SiegeAccountTest precedent), so it
 *      never loads Config/SiegeCloudDev.ini and holds NO ProjectUrl: even a
 *      hypothetical buggy request could not name an endpoint. The single
 *      mutation exercised on it, SignOut(), is pinned by ACC-§11/TASK-643 to
 *      no-op gracefully while unconfigured.
 *    - The ini-parse test parses a SCRATCH STRING BUFFER through FConfigFile —
 *      no file on disk, no real key, no real project ref. Everything
 *      network-shaped in this file tests the PURE seams the registry promises
 *      (config validity, pull arithmetic, row-JSON shape, link-state
 *      transitions).
 *
 *  ⛔ THE TESTS NEVER WRITE THE PLAYER'S REAL SLOTS, AND THE GUARANTEE IS
 *  MECHANICAL (the SiegeAccountTest/SiegeSettingsTest idiom, cloned): every
 *  account subsystem instance is redirected to the scratch registry slot
 *  "SiegeAccounts_CloudAutomationScratch" via SetSlotNameForAutomationTests
 *  BEFORE any operation (a name deliberately distinct from BOTH the shipped
 *  "SiegeAccounts" and SiegeAccountTest's scratch slot, so the two test files
 *  can never collide in one session), and FCloudScratchGuard deletes the
 *  scratch registry plus every RULING-8 seed-copy artifact on the way out.
 *  CI-clean both ways: on a machine with no guest slots nothing is copied,
 *  and either way zero files remain.
 *
 *  ⛔ NO SECRET MATERIAL LIVES IN THIS FILE (ACC-§11 / P2-R2): the "anon key"
 *  and "refresh token" literals below are obviously-scratch placeholder
 *  strings, not JWTs; the project ref is "scratchref", not the live project's.
 *  Assertion messages never embed them as credentials.
 *
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no
 *  new replicated class, no new relevancy tier, no RPC. All cloud traffic is
 *  client-local HTTPS from USiegeCloudClient (a UGameInstanceSubsystem);
 *  nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 *  (This file itself performs no cloud traffic at all — see the zero-network
 *  law above.)
 *
 *  WHAT IS DELIBERATELY NOT COVERED HERE (named, not implied — the honest gap
 *  list lives in handoffs/TASK-647-programmer.md and is owned downstream by
 *  the TASK-650 live smoke + the TASK-651 sitting): the real Initialize()
 *  ini-load off Config/SiegeCloudDev.ini · every HTTP request/response ·
 *  the async error shape of cloud calls while unconfigured (unpinned) ·
 *  LastSyncUtc / CloudRefreshToken value readback (no pinned getter) ·
 *  OnCloudStateChanged broadcasts · PullAll/PushAll/SyncNow end-to-end.
 *
 *  ⭐ 2026-08-23, TASK-653 (riders R1+R2 — ACC-§15 P2.1): the "value readback"
 *  gap above is CLOSED — GetLastSyncUtc / GetCloudRefreshToken are now
 *  registry-pinned, and Siegebound.Cloud.ReAuthSeamGetters below covers the
 *  defaults, both round trips, the rotation-through-644's-guards shape, and
 *  cross-instance persistence. The zero-network law is UNCHANGED:
 *  RefreshSession still has zero callers in this file (the widget's R1 wire is
 *  live-smoke territory, not offline-testable), and no other gap-list line
 *  moves.
 */

namespace SiegeCloudTestUtils
{
	/** ⛔ NEVER the shipped registry slot, and NEVER SiegeAccountTest's scratch slot. Asserted mechanically in the link round-trip test, not assumed. */
	static const TCHAR* ScratchRegistrySlotName = TEXT("SiegeAccounts_CloudAutomationScratch");

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
	 *  RULING-8 seed-copy artifacts ("SiegeDecks_<Digits>" /
	 *  "SiegeSettings_<Digits>") of every profile the test registered via
	 *  TrackActiveProfile. Only slots this test run created can match those
	 *  names — the suffix is a fresh FGuid — so the janitor cannot touch a
	 *  real player profile. (FAccountScratchGuard, cloned.)
	 */
	struct FCloudScratchGuard
	{
		TArray<FString> ProfileSuffixes;

		FCloudScratchGuard()
		{
			DeleteScratchRegistrySlot();
		}

		~FCloudScratchGuard()
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
	 *  throwaway UGameInstance it must live inside (UGameInstanceSubsystem is
	 *  UCLASS(Abstract, Within = GameInstance)). Both held by TStrongObjectPtr
	 *  so a mid-test GC pass cannot collect either.
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
				// SiegeAccounts.sav untouched.
				Store.Accounts->SetSlotNameForAutomationTests(ScratchRegistrySlotName);
			}
		}
		return Store;
	}

	/** Sorted, '|'-joined top-level key list of a parsed JSON object — the SC-§13 exact-shape instrument for the row-JSON tests. */
	static FString JoinedSortedKeys(const TSharedPtr<FJsonObject>& JsonObject)
	{
		TArray<FString> Keys;
		if (JsonObject.IsValid())
		{
			// UE 5.8: FJsonObject::Values is keyed on UE::FSharedString
			// (TSharedString<TCHAR>), not FString (JsonObject.h:99/237/324), so
			// GenerateKeyArray(TArray<FString>&) cannot deduce (TASK-649 C2672).
			// TSharedString::operator*() returns the null-terminated TCHAR*
			// (SharedString.h:79-83) — construct each FString key from it.
			Keys.Reserve(JsonObject->Values.Num());
			for (const auto& Pair : JsonObject->Values)
			{
				Keys.Emplace(*Pair.Key);
			}
		}
		Keys.Sort();
		return FString::Join(Keys, TEXT("|"));
	}

	/** Parses a JSON string into an object; invalid JSON or a non-object root yields an invalid pointer the caller must assert on. */
	static TSharedPtr<FJsonObject> ParseJsonObject(const FString& JsonString)
	{
		TSharedPtr<FJsonObject> Parsed;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (!FJsonSerializer::Deserialize(Reader, Parsed))
		{
			Parsed.Reset();
		}
		return Parsed;
	}
}

/**
 *  FSiegeCloudConfig::IsValid — THE PINNED TRUTH TABLE ("both non-empty",
 *  ACC-§15 block 1). Four rows: neither / only ProjectUrl / only AnonKey /
 *  both. A default-constructed config is the missing-ini outcome, and it MUST
 *  be invalid — IsValid()==false is the cloud-OFF switch the whole ACC-§11
 *  degrade-to-Phase-1 law hangs on. (Whitespace-only strings are NOT pinned
 *  either way by the registry and are deliberately not asserted.)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudConfigValidityTest,
	"Siegebound.Cloud.ConfigValidityTruthTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudConfigValidityTest::RunTest(const FString& Parameters)
{
	FSiegeCloudConfig Config;
	TestFalse(TEXT("A default-constructed config (both empty) is INVALID — the missing-ini cloud-OFF state"),
		Config.IsValid());

	Config.ProjectUrl = TEXT("https://scratchref.supabase.co");
	Config.AnonKey.Reset();
	TestFalse(TEXT("ProjectUrl alone is INVALID (AnonKey empty)"), Config.IsValid());

	Config.ProjectUrl.Reset();
	Config.AnonKey = TEXT("scratch-anon-key-not-a-real-credential");
	TestFalse(TEXT("AnonKey alone is INVALID (ProjectUrl empty)"), Config.IsValid());

	Config.ProjectUrl = TEXT("https://scratchref.supabase.co");
	Config.AnonKey = TEXT("scratch-anon-key-not-a-real-credential");
	TestTrue(TEXT("Both fields non-empty is VALID — the registry's pinned predicate"), Config.IsValid());

	return true;
}

/**
 *  THE ACC-§11 CONFIG-HOME FORMAT SEAM, from a SCRATCH STRING BUFFER — no
 *  file, no real key, zero network. TASK-643's Initialize is pinned to the
 *  FConfigCacheIni family; this test proves that a buffer in the law's exact
 *  shape — section [SiegeCloud], keys ProjectUrl / AnonKey, plus the
 *  "; DbPassword=" custody line — yields the two values through that same
 *  engine family, that the custody comment is INVISIBLE to the parser (the
 *  game never reads it — ACC-§11), and that an incomplete buffer degrades to
 *  an INVALID config (the cloud-OFF outcome). The real file load inside
 *  Initialize() is NOT drivable in-process (it would also read the
 *  developer's real ini — a hermeticity violation); that gap is stated in the
 *  handoff and owned by TASK-650/651.
 *
 *  ⭐ LOOP-2 FINDING (TASK-649 re-run, qa/TASK-648.md §10 — measured live,
 *  then proven at the installed engine source): UE 5.8's ini line reader
 *  swallows an UNQUOTED `//` as an inline comment-start
 *  (FParse::LineExtended with ELineExtendedFlags::SwallowDoubleSlashComments,
 *  Parse.cpp:1245-1250) — and that reader is reached by BOTH
 *  FConfigFile::CombineFromBuffer AND the FConfigFile::Read(FilePath) that
 *  production Initialize() uses, through the ONE shared FillFileFromBuffer
 *  (ConfigCacheIni.cpp:1930-1934 / 2201-2204 / 2216-2219). So an unquoted
 *  ProjectUrl=https://… truncates to "https:" in production too. The SHIPPING
 *  format is therefore the DOUBLE-QUOTED value — the comment swallow is
 *  quote-aware (the !bIsQuoted guard, Parse.cpp:1245/1287) and the quotes are
 *  stripped at parse time (ConfigCacheIni.cpp:2148-2151 →
 *  FParse::QuotedString, Parse.cpp:391-410), so GetString hands back the bare
 *  URL. This test exercises exactly that shipping format, and pins the
 *  truncation hazard below so nobody ever un-quotes the ini.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudConfigIniParseTest,
	"Siegebound.Cloud.ConfigIniParseSeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudConfigIniParseTest::RunTest(const FString& Parameters)
{
	// The ACC-§11 shape, verbatim keys, scratch values (P2-R2: no real ref, no
	// JWT-shaped literal anywhere in this file). ⛔ ProjectUrl is DOUBLE-QUOTED
	// — the shipping format of Config/SiegeCloudDev.ini{,.example} since the
	// loop-2 finding: unquoted, the // would truncate it (pinned below).
	const FString IniBuffer =
		TEXT("; Scratch buffer in the exact ACC-§11 Config/SiegeCloudDev.ini shape — never a real key.\n")
		TEXT("[SiegeCloud]\n")
		TEXT("ProjectUrl=\"https://scratchref.supabase.co\"\n")
		TEXT("AnonKey=scratch-anon-key-not-a-real-credential\n")
		TEXT("; DbPassword=custody-comment-the-game-never-reads\n");

	FConfigFile ConfigFile;
	ConfigFile.CombineFromBuffer(IniBuffer, TEXT("SiegeCloudDevScratch"));

	FString ProjectUrl;
	TestTrue(TEXT("[SiegeCloud] ProjectUrl parses out of the ACC-§11-shaped buffer"),
		ConfigFile.GetString(TEXT("SiegeCloud"), TEXT("ProjectUrl"), ProjectUrl));
	TestEqualSensitive(TEXT("The QUOTED ProjectUrl reads back as the bare URL — quotes stripped, // intact (the shipping-format proof)"),
		ProjectUrl, FString(TEXT("https://scratchref.supabase.co")));

	FString AnonKey;
	TestTrue(TEXT("[SiegeCloud] AnonKey parses out of the ACC-§11-shaped buffer"),
		ConfigFile.GetString(TEXT("SiegeCloud"), TEXT("AnonKey"), AnonKey));
	TestEqualSensitive(TEXT("The parsed AnonKey is byte-identical to the buffer's value"),
		AnonKey, FString(TEXT("scratch-anon-key-not-a-real-credential")));

	// The custody line is a COMMENT: the parser must not surface it as a key.
	// This is the format proof behind ACC-§11's "the game NEVER reads it".
	FString DbPassword;
	TestFalse(TEXT("The '; DbPassword=' custody line is invisible to the parser (comment, not key)"),
		ConfigFile.GetString(TEXT("SiegeCloud"), TEXT("DbPassword"), DbPassword));

	// A complete parse composes into a VALID FSiegeCloudConfig.
	FSiegeCloudConfig ParsedConfig;
	ParsedConfig.ProjectUrl = ProjectUrl;
	ParsedConfig.AnonKey = AnonKey;
	TestTrue(TEXT("A fully-parsed buffer composes into a VALID config"), ParsedConfig.IsValid());

	// ⛔ THE PINNED TRUNCATION HAZARD (TASK-649 loop 2, measured 2026-08-23,
	// engine citations in the doc comment above): an UNQUOTED https:// value is
	// cut at the // by the engine's line reader — and the reader is SHARED with
	// FConfigFile::Read, i.e. the REAL Config/SiegeCloudDev.ini load inside
	// Initialize(). This block pins that behavior so nobody un-quotes the
	// shipping ini: the readback is "https:", and — the silent-break mechanism
	// — it still composes IsValid()==true, so cloud would come up "configured"
	// with a garbage URL instead of degrading to the clean ACC-§11 OFF state.
	FConfigFile UnquotedFile;
	UnquotedFile.CombineFromBuffer(
		TEXT("[SiegeCloud]\nProjectUrl=https://scratchref.supabase.co\nAnonKey=scratch-anon-key-not-a-real-credential\n"),
		TEXT("SiegeCloudDevScratchUnquotedHazard"));
	FString TruncatedUrl;
	TestTrue(TEXT("HAZARD PIN: an unquoted ProjectUrl still yields a value (the failure is silent, not a parse error)"),
		UnquotedFile.GetString(TEXT("SiegeCloud"), TEXT("ProjectUrl"), TruncatedUrl));
	TestEqualSensitive(TEXT("HAZARD PIN: the unquoted https:// value TRUNCATES to 'https:' (// = ini inline comment — the shipping ini value must STAY quoted)"),
		TruncatedUrl, FString(TEXT("https:")));
	FSiegeCloudConfig TruncatedConfig;
	TruncatedConfig.ProjectUrl = TruncatedUrl;
	TruncatedConfig.AnonKey = AnonKey;
	TestTrue(TEXT("HAZARD PIN: the truncated config still passes IsValid() — the silent-break shape the quoting rule exists to prevent"),
		TruncatedConfig.IsValid());

	// An INCOMPLETE buffer (AnonKey missing) degrades to an INVALID config —
	// the ACC-§11 cloud-OFF outcome, not an error state.
	const FString IncompleteBuffer =
		TEXT("[SiegeCloud]\n")
		TEXT("ProjectUrl=\"https://scratchref.supabase.co\"\n");

	FConfigFile IncompleteFile;
	IncompleteFile.CombineFromBuffer(IncompleteBuffer, TEXT("SiegeCloudDevScratchIncomplete"));

	FString MissingKey;
	TestFalse(TEXT("A buffer without AnonKey= yields no AnonKey value"),
		IncompleteFile.GetString(TEXT("SiegeCloud"), TEXT("AnonKey"), MissingKey));

	FSiegeCloudConfig IncompleteConfig;
	IncompleteConfig.ProjectUrl = ProjectUrl;
	IncompleteConfig.AnonKey = MissingKey; // stays empty — GetString failed
	TestFalse(TEXT("The incomplete parse composes into an INVALID config (cloud OFF, ACC-§11)"),
		IncompleteConfig.IsValid());

	// A buffer with no [SiegeCloud] section at all: the lookup fails cleanly.
	FConfigFile EmptyFile;
	EmptyFile.CombineFromBuffer(TEXT("[SomeOtherSection]\nIrrelevant=1\n"), TEXT("SiegeCloudDevScratchEmpty"));
	FString NoValue;
	TestFalse(TEXT("A buffer without [SiegeCloud] yields nothing for ProjectUrl"),
		EmptyFile.GetString(TEXT("SiegeCloud"), TEXT("ProjectUrl"), NoValue));

	return true;
}

/**
 *  FSiegeCloudSync::ShouldPullRow — THE PURE PULL ARITHMETIC, pinned by
 *  ACC-§13 trigger 1: "rows with updated_at > LastSyncUtc land locally".
 *  STRICTLY greater — an equal timestamp does NOT pull (pulling on equality
 *  would re-apply every row on every sync and turn last-write-wins into
 *  last-sync-wins). The unset FDateTime() (zero ticks) is the never-synced
 *  profile default: everything real pulls against it, and an unset CLOUD
 *  stamp is never newer than anything.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudShouldPullRowTest,
	"Siegebound.Cloud.ShouldPullRowBoundaryMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudShouldPullRowTest::RunTest(const FString& Parameters)
{
	const FDateTime LastSync(2026, 8, 23, 12, 0, 0);
	const FDateTime OneSecondNewer = LastSync + FTimespan::FromSeconds(1.0);
	const FDateTime OneTickNewer = LastSync + FTimespan(1); // the strictness boundary at tick resolution
	const FDateTime OneSecondOlder = LastSync - FTimespan::FromSeconds(1.0);
	const FDateTime Unset; // zero ticks — the never-synced default

	TestTrue(TEXT("A cloud row one second NEWER than LastSyncUtc pulls"),
		FSiegeCloudSync::ShouldPullRow(OneSecondNewer, LastSync));

	TestTrue(TEXT("A cloud row one TICK newer than LastSyncUtc pulls (strictness boundary)"),
		FSiegeCloudSync::ShouldPullRow(OneTickNewer, LastSync));

	TestFalse(TEXT("A cloud row one second OLDER than LastSyncUtc does not pull"),
		FSiegeCloudSync::ShouldPullRow(OneSecondOlder, LastSync));

	TestFalse(TEXT("A cloud row EQUAL to LastSyncUtc does not pull (ACC-§13 is strictly greater)"),
		FSiegeCloudSync::ShouldPullRow(LastSync, LastSync));

	TestTrue(TEXT("Against an UNSET LastSyncUtc (never synced) every real cloud row pulls"),
		FSiegeCloudSync::ShouldPullRow(LastSync, Unset));

	TestFalse(TEXT("An UNSET cloud stamp never pulls against a real LastSyncUtc"),
		FSiegeCloudSync::ShouldPullRow(Unset, LastSync));

	TestFalse(TEXT("Both stamps unset: nothing is newer, nothing pulls"),
		FSiegeCloudSync::ShouldPullRow(Unset, Unset));

	return true;
}

/**
 *  MakeDeckRowJson — THE ACC-§12 decks ROW SHAPE, exactly. The client owns
 *  exactly three columns: user_id · deck_name · payload. ⛔ updated_at is
 *  ABSENT (server-owned — the A3 sync clock; a client-written updated_at
 *  would forward-date last-write-wins, the exact hole the TASK-640 trigger
 *  ruling closed server-side). ⛔ id is ABSENT (server-defaulted
 *  gen_random_uuid(); the registry's MakeDeckRowJson takes no id, and a
 *  client-sent id would fight the Prefer: resolution=merge-duplicates upsert
 *  lane pinned on UpsertRow). The key-set assertion is EXACT and
 *  case-sensitive (SC-§13): sorted keys, joined, byte-compared.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudDeckRowJsonTest,
	"Siegebound.Cloud.DeckRowJsonShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudDeckRowJsonTest::RunTest(const FString& Parameters)
{
	const FString CloudUserId = TEXT("0f8fad5b-d9cb-469f-a165-70867728950e"); // arbitrary uuid, not a live account
	const FString DeckName = TEXT("Alpha \"Siege\" Deck");                    // embedded quotes: escaping must survive the round trip

	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("ActiveDeckName"), TEXT("Alpha"));
	Payload->SetNumberField(TEXT("SchemaVersion"), 3.0);
	TArray<TSharedPtr<FJsonValue>> Cards;
	Cards.Add(MakeShared<FJsonValueString>(TEXT("Knight")));
	Cards.Add(MakeShared<FJsonValueString>(TEXT("Catapult")));
	Payload->SetArrayField(TEXT("Cards"), Cards);

	const FString RowJson = FSiegeCloudSync::MakeDeckRowJson(CloudUserId, DeckName, Payload);

	const TSharedPtr<FJsonObject> Parsed = SiegeCloudTestUtils::ParseJsonObject(RowJson);
	if (!Parsed.IsValid())
	{
		AddError(FString::Printf(TEXT("MakeDeckRowJson did not produce a parseable JSON object. Output: %s"), *RowJson));
		return false;
	}

	// THE EXACT SHAPE (SC-§13): three keys, these spellings, nothing else.
	TestEqualSensitive(TEXT("The deck row's top-level key set is EXACTLY deck_name|payload|user_id (sorted, byte-identical)"),
		SiegeCloudTestUtils::JoinedSortedKeys(Parsed), FString(TEXT("deck_name|payload|user_id")));

	// The two hard absences, asserted by name for legibility even though the
	// exact-key-set line above already forbids them.
	TestFalse(TEXT("updated_at is ABSENT from the deck row (server-owned — the A3 clock, ACC-§12)"),
		Parsed->HasField(TEXT("updated_at")));
	TestFalse(TEXT("id is ABSENT from the deck row (server-defaulted; merge-duplicates resolves on (user_id, deck_name))"),
		Parsed->HasField(TEXT("id")));

	// Values round-trip byte-identically.
	TestEqualSensitive(TEXT("user_id carries the CloudUserId passed in"),
		Parsed->GetStringField(TEXT("user_id")), CloudUserId);
	TestEqualSensitive(TEXT("deck_name carries the deck name passed in, quotes and all (escaping survived)"),
		Parsed->GetStringField(TEXT("deck_name")), DeckName);

	// payload is a JSON OBJECT (the jsonb projection, ACC-§13) — not a
	// double-encoded string — and its content survives intact.
	const TSharedPtr<FJsonObject>* PayloadOut = nullptr;
	if (!Parsed->TryGetObjectField(TEXT("payload"), PayloadOut) || PayloadOut == nullptr || !PayloadOut->IsValid())
	{
		AddError(TEXT("The deck row's payload field is not a JSON object (jsonb projections must embed as objects, never re-encoded strings)."));
		return false;
	}
	TestEqualSensitive(TEXT("payload.ActiveDeckName round-trips"),
		(*PayloadOut)->GetStringField(TEXT("ActiveDeckName")), FString(TEXT("Alpha")));
	TestEqual(TEXT("payload.SchemaVersion round-trips"),
		(*PayloadOut)->GetIntegerField(TEXT("SchemaVersion")), 3);
	const TArray<TSharedPtr<FJsonValue>>& CardsOut = (*PayloadOut)->GetArrayField(TEXT("Cards"));
	TestEqual(TEXT("payload.Cards keeps both entries"), CardsOut.Num(), 2);
	if (CardsOut.Num() == 2)
	{
		TestEqualSensitive(TEXT("payload.Cards[0] round-trips"), CardsOut[0]->AsString(), FString(TEXT("Knight")));
		TestEqualSensitive(TEXT("payload.Cards[1] round-trips"), CardsOut[1]->AsString(), FString(TEXT("Catapult")));
	}

	return true;
}

/**
 *  MakeSettingsRowJson — THE ACC-§12 settings ROW SHAPE, exactly. The client
 *  owns exactly two columns: user_id · payload (settings is keyed on user_id
 *  as PRIMARY KEY — one row per user, no deck_name, no id column at all).
 *  ⛔ updated_at ABSENT, same law as the deck row.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudSettingsRowJsonTest,
	"Siegebound.Cloud.SettingsRowJsonShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudSettingsRowJsonTest::RunTest(const FString& Parameters)
{
	const FString CloudUserId = TEXT("7c9e6679-7425-40de-944b-e07fc1f90ae7"); // arbitrary uuid, not a live account

	const TSharedRef<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetNumberField(TEXT("MouseSensitivity"), 0.75);
	Payload->SetBoolField(TEXT("bInvertY"), true);
	Payload->SetStringField(TEXT("KeyboardLayout"), TEXT("QWERTY"));

	const FString RowJson = FSiegeCloudSync::MakeSettingsRowJson(CloudUserId, Payload);

	const TSharedPtr<FJsonObject> Parsed = SiegeCloudTestUtils::ParseJsonObject(RowJson);
	if (!Parsed.IsValid())
	{
		AddError(FString::Printf(TEXT("MakeSettingsRowJson did not produce a parseable JSON object. Output: %s"), *RowJson));
		return false;
	}

	// THE EXACT SHAPE (SC-§13): two keys, these spellings, nothing else.
	TestEqualSensitive(TEXT("The settings row's top-level key set is EXACTLY payload|user_id (sorted, byte-identical)"),
		SiegeCloudTestUtils::JoinedSortedKeys(Parsed), FString(TEXT("payload|user_id")));

	TestFalse(TEXT("updated_at is ABSENT from the settings row (server-owned — the A3 clock, ACC-§12)"),
		Parsed->HasField(TEXT("updated_at")));
	TestFalse(TEXT("deck_name is ABSENT from the settings row (settings has no such column)"),
		Parsed->HasField(TEXT("deck_name")));

	TestEqualSensitive(TEXT("user_id carries the CloudUserId passed in"),
		Parsed->GetStringField(TEXT("user_id")), CloudUserId);

	const TSharedPtr<FJsonObject>* PayloadOut = nullptr;
	if (!Parsed->TryGetObjectField(TEXT("payload"), PayloadOut) || PayloadOut == nullptr || !PayloadOut->IsValid())
	{
		AddError(TEXT("The settings row's payload field is not a JSON object (jsonb projections must embed as objects)."));
		return false;
	}
	TestEqual(TEXT("payload.MouseSensitivity round-trips"),
		(*PayloadOut)->GetNumberField(TEXT("MouseSensitivity")), 0.75);
	TestTrue(TEXT("payload.bInvertY round-trips"),
		(*PayloadOut)->GetBoolField(TEXT("bInvertY")));
	TestEqualSensitive(TEXT("payload.KeyboardLayout round-trips"),
		(*PayloadOut)->GetStringField(TEXT("KeyboardLayout")), FString(TEXT("QWERTY")));

	return true;
}

/**
 *  THE 644 LINK API ROUND TRIP on a scratch registry slot: create a local
 *  profile → link (IsCloudLinked / GetLinkedEmail flip, the registry SAVES,
 *  OnActiveProfileChanged broadcasts once) → SetLastSyncUtc saves → the link
 *  state SURVIVES a cross-instance reload through the automation seam →
 *  ClearCloudLink unlinks while THE LOCAL PROFILE SURVIVES (the ACC-§11 pin:
 *  cloud sign-out is not a local logout). The refresh-token and LastSyncUtc
 *  VALUES have no pinned getter and are not asserted — the save-write and the
 *  reloaded link state are the observables; the value gap is named in the
 *  handoff.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudLinkRoundTripTest,
	"Siegebound.Cloud.CloudLinkRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudLinkRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeCloudTestUtils::FCloudScratchGuard Guard;

	// The hermeticity guarantee, made mechanical (the SlotContract idiom).
	TestNotEqual(TEXT("The cloud automation scratch slot is NOT the shipped registry slot"),
		FString(SiegeCloudTestUtils::ScratchRegistrySlotName), FString(USiegeAccountSaveGame::SlotName));

	SiegeCloudTestUtils::FScratchAccounts Store = SiegeCloudTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	// CREATE the local profile the link will decorate.
	FString Reason;
	TestTrue(TEXT("CreateAccount succeeds"),
		Store.Accounts->CreateAccount(TEXT("CloudLinkTester"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	const FGuid ProfileId = Store.Accounts->GetActiveProfileId();
	const FString DeckSlotBefore = Store.Accounts->GetDeckSlotName();
	const FString SettingsSlotBefore = Store.Accounts->GetSettingsSlotName();

	// PRE-LINK: a fresh local profile is NOT cloud-linked.
	TestFalse(TEXT("A freshly created profile is not cloud-linked"), Store.Accounts->IsCloudLinked());
	TestTrue(TEXT("A freshly created profile has an empty linked email"), Store.Accounts->GetLinkedEmail().IsEmpty());

	// LINK. Scratch identity only: lowercase email (no casing claim rides on
	// it), arbitrary uuid, placeholder token (⛔ not JWT-shaped — P2-R2).
	const FString LinkEmail = TEXT("cloud.link.tester@example.com");
	const FString LinkUserId = TEXT("3d813cbb-47fb-42ba-91df-831e1593ac29");
	const FString LinkRefreshToken = TEXT("scratch-refresh-token-not-a-real-credential");

	// Delete the registry file the create wrote, so the file's REAPPEARANCE
	// isolates SetCloudLink's own save (the pinned "saves the registry slot").
	SiegeCloudTestUtils::DeleteScratchRegistrySlot();
	TestFalse(TEXT("Pre-condition: the scratch registry file is deleted before SetCloudLink"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	const int32 BroadcastsBeforeLink = Store.Accounts->ActiveProfileChangedBroadcastCount;
	Store.Accounts->SetCloudLink(LinkEmail, LinkUserId, LinkRefreshToken);

	TestTrue(TEXT("SetCloudLink flips IsCloudLinked to true"), Store.Accounts->IsCloudLinked());
	TestEqualSensitive(TEXT("GetLinkedEmail returns the linked email byte-identically"),
		Store.Accounts->GetLinkedEmail(), LinkEmail);
	TestEqual(TEXT("SetCloudLink broadcasts OnActiveProfileChanged exactly once (the ACC-§15 tail)"),
		Store.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBeforeLink + 1);
	TestTrue(TEXT("SetCloudLink SAVED the registry slot (the file reappeared)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	// LINKING CHANGED NO LOCAL IDENTITY: same profile, same name, same slots.
	TestTrue(TEXT("The store is still logged in after linking"), Store.Accounts->IsLoggedIn());
	TestTrue(TEXT("The active ProfileId is unchanged by linking"), Store.Accounts->GetActiveProfileId() == ProfileId);
	TestEqualSensitive(TEXT("The display name is unchanged by linking"),
		Store.Accounts->GetActiveDisplayName(), FString(TEXT("CloudLinkTester")));
	TestEqualSensitive(TEXT("The deck slot name is unchanged by linking"),
		Store.Accounts->GetDeckSlotName(), DeckSlotBefore);
	TestEqualSensitive(TEXT("The settings slot name is unchanged by linking"),
		Store.Accounts->GetSettingsSlotName(), SettingsSlotBefore);

	// SetLastSyncUtc SAVES (pinned "saves registry"). No broadcast assertion:
	// the ACC-§15 tail names only SetCloudLink/ClearCloudLink as broadcasters
	// while the TASK-644 spec sentence includes SetLastSyncUtc — an ambiguity
	// this file must not pre-judge (named in the handoff for QA).
	SiegeCloudTestUtils::DeleteScratchRegistrySlot();
	Store.Accounts->SetLastSyncUtc(FDateTime(2026, 8, 23, 12, 0, 0));
	TestTrue(TEXT("SetLastSyncUtc SAVED the registry slot (the file reappeared)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));
	TestTrue(TEXT("SetLastSyncUtc left the profile linked"), Store.Accounts->IsCloudLinked());

	// CROSS-INSTANCE PERSISTENCE through the automation seam: a second
	// subsystem instance pointed at the same scratch slot reloads the link.
	SiegeCloudTestUtils::FScratchAccounts Reloaded = SiegeCloudTestUtils::MakeScratchAccounts();
	if (!Reloaded.IsValid())
	{
		AddError(TEXT("Could not construct the second USiegeAccountSubsystem for the reload check."));
		return false;
	}
	Reloaded.Accounts->LoadAccountsFromSlot();

	TestTrue(TEXT("The reloaded instance restores the persisted active profile"), Reloaded.Accounts->IsLoggedIn());
	TestTrue(TEXT("The reloaded active ProfileId matches the created one"),
		Reloaded.Accounts->GetActiveProfileId() == ProfileId);
	TestTrue(TEXT("The cloud link SURVIVED the save/load round trip (SaveGame-tagged fields persisted)"),
		Reloaded.Accounts->IsCloudLinked());
	TestEqualSensitive(TEXT("The linked email survived the save/load round trip byte-identically"),
		Reloaded.Accounts->GetLinkedEmail(), LinkEmail);

	// CLEAR on the reloaded instance: cloud sign-out, LOCAL PROFILE SURVIVES.
	SiegeCloudTestUtils::DeleteScratchRegistrySlot();
	const int32 BroadcastsBeforeClear = Reloaded.Accounts->ActiveProfileChangedBroadcastCount;
	Reloaded.Accounts->ClearCloudLink();

	TestFalse(TEXT("ClearCloudLink flips IsCloudLinked to false"), Reloaded.Accounts->IsCloudLinked());
	TestTrue(TEXT("ClearCloudLink empties the linked email"), Reloaded.Accounts->GetLinkedEmail().IsEmpty());
	TestEqual(TEXT("ClearCloudLink broadcasts OnActiveProfileChanged exactly once (the ACC-§15 tail)"),
		Reloaded.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBeforeClear + 1);
	TestTrue(TEXT("ClearCloudLink SAVED the registry slot (the file reappeared)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	TestTrue(TEXT("The LOCAL profile survives the cloud sign-out: still logged in (ACC-§11)"),
		Reloaded.Accounts->IsLoggedIn());
	TestTrue(TEXT("The local ProfileId survives the cloud sign-out"),
		Reloaded.Accounts->GetActiveProfileId() == ProfileId);
	TestEqualSensitive(TEXT("The local display name survives the cloud sign-out"),
		Reloaded.Accounts->GetActiveDisplayName(), FString(TEXT("CloudLinkTester")));

	return true;
}

/**
 *  GUEST NEVER SYNCS AND NEVER LINKS (ACC-§13 / ACC-§1): with no active
 *  profile the cloud queries return the safe empty defaults, and the three
 *  644 mutators are complete no-ops — no fabricated profile, no broadcast
 *  (the P1 delegate law: never broadcast on a no-op), no registry write (the
 *  P1 save-on-CHANGE law: nothing changed). CI-clean: this test writes
 *  nothing at all.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudGuestSafeDefaultsTest,
	"Siegebound.Cloud.GuestSafeDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudGuestSafeDefaultsTest::RunTest(const FString& Parameters)
{
	SiegeCloudTestUtils::FCloudScratchGuard Guard;

	SiegeCloudTestUtils::FScratchAccounts Store = SiegeCloudTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	// THE QUERIES: guest answers, the pinned empties.
	TestFalse(TEXT("Guest is not cloud-linked"), Store.Accounts->IsCloudLinked());
	TestTrue(TEXT("Guest GetLinkedEmail is empty (the ACC-§15 pin: empty when guest/unlinked)"),
		Store.Accounts->GetLinkedEmail().IsEmpty());

	// THE MUTATORS: all three are no-ops with no active profile to mutate
	// ("mutate ONLY the active profile" — there is none).
	const int32 BroadcastsBefore = Store.Accounts->ActiveProfileChangedBroadcastCount;

	Store.Accounts->SetCloudLink(
		TEXT("guest.should.not.link@example.com"),
		TEXT("11111111-2222-3333-4444-555555555555"),
		TEXT("scratch-refresh-token-not-a-real-credential"));

	TestFalse(TEXT("SetCloudLink as guest links nothing"), Store.Accounts->IsCloudLinked());
	TestTrue(TEXT("SetCloudLink as guest leaves the linked email empty"), Store.Accounts->GetLinkedEmail().IsEmpty());
	TestFalse(TEXT("SetCloudLink as guest fabricates no profile (still logged out)"), Store.Accounts->IsLoggedIn());

	Store.Accounts->ClearCloudLink();
	TestFalse(TEXT("ClearCloudLink as guest is a safe no-op (still guest)"), Store.Accounts->IsLoggedIn());

	Store.Accounts->SetLastSyncUtc(FDateTime(2026, 8, 23, 12, 0, 0));
	TestFalse(TEXT("SetLastSyncUtc as guest is a safe no-op (still guest)"), Store.Accounts->IsLoggedIn());

	// No-op ⇒ NO broadcast (the P1 delegate law) and NO registry write (the
	// P1 save-on-change law) — asserted once over all three mutators.
	TestEqual(TEXT("No guest mutator broadcast OnActiveProfileChanged (never broadcast on a no-op)"),
		Store.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBefore);
	TestFalse(TEXT("No guest mutator wrote the registry slot (save on CHANGE only — nothing changed)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	return true;
}

/**
 *  AN UNCONFIGURED USiegeCloudClient ANSWERS THE SAFE DEFAULTS — the ACC-§11
 *  cloud-OFF law seen from the client's own surface. The instance is
 *  constructed but NEVER Initialize()d (see the file header), so no config is
 *  loaded and no ProjectUrl exists: this is exactly the missing-ini state,
 *  and it must be inert. SignOut() — pinned "clears in-memory tokens" — must
 *  no-op gracefully here (ACC-§11: everything no-ops when unconfigured); with
 *  no URL and no token it cannot name an endpoint, preserving the
 *  zero-network law even against a defect. ⛔ The async methods (SignUp /
 *  SignIn / FetchRows / …) are NOT called: their unconfigured error shape is
 *  unpinned and network-adjacent — that seam belongs to QA's reading of
 *  TASK-643 and to the TASK-650 live smoke.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudClientUnconfiguredTest,
	"Siegebound.Cloud.ClientUnconfiguredDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudClientUnconfiguredTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GetTransientPackageAsObject()));
	if (!GameInstance.IsValid())
	{
		AddError(TEXT("Could not construct the throwaway UGameInstance."));
		return false;
	}

	TStrongObjectPtr<USiegeCloudClient> Client(NewObject<USiegeCloudClient>(GameInstance.Get()));
	if (!Client.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeCloudClient inside a UGameInstance."));
		return false;
	}

	// The safe defaults, before anything is loaded or called.
	TestFalse(TEXT("An uninitialized client is NOT configured (no ini was loaded)"), Client->IsCloudConfigured());
	TestFalse(TEXT("An uninitialized client is NOT authenticated (no token exists)"), Client->IsCloudAuthenticated());
	TestTrue(TEXT("An uninitialized client's GetCloudUserId is empty (the ACC-§15 pin: empty when signed out)"),
		Client->GetCloudUserId().IsEmpty());

	// SignOut while unconfigured and signed out: a graceful no-op (ACC-§11).
	Client->SignOut();
	TestFalse(TEXT("SignOut on the unconfigured client leaves it unconfigured"), Client->IsCloudConfigured());
	TestFalse(TEXT("SignOut on the unconfigured client leaves it unauthenticated"), Client->IsCloudAuthenticated());
	TestTrue(TEXT("SignOut on the unconfigured client leaves GetCloudUserId empty"),
		Client->GetCloudUserId().IsEmpty());

	return true;
}

/**
 *  THE ACC-§15 P2.1 RE-AUTH SEAM GETTERS (TASK-653, riders R1+R2):
 *  GetLastSyncUtc / GetCloudRefreshToken — guest and unlinked defaults, the
 *  SetLastSyncUtc -> GetLastSyncUtc round trip (the R2 pull-baseline value),
 *  the SetCloudLink -> GetCloudRefreshToken round trip (the R1 token value),
 *  pure-read discipline (no broadcast, no save), the token-ROTATION shape
 *  through 644's guards (a new token with unchanged email/user id is a REAL
 *  mutation — the identical-values guard needs all THREE identical), and
 *  cross-instance persistence — the exact values a next session's
 *  MakeContext() baseline (R2) and re-auth attempt (R1) read. FDateTime
 *  comparisons ride TestTrue(==) — the file's own FGuid idiom (the generic
 *  TestEqual has no FDateTime debug-print lane to lean on).
 *  ⛔ The zero-network law holds: RefreshSession is never called; every
 *  surface driven here is a pure in-memory getter or a P1-pinned local-save
 *  mutator. Scratch identity only (P2-R2): placeholder tokens, not JWTs.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCloudReAuthSeamGettersTest,
	"Siegebound.Cloud.ReAuthSeamGetters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCloudReAuthSeamGettersTest::RunTest(const FString& Parameters)
{
	SiegeCloudTestUtils::FCloudScratchGuard Guard;

	SiegeCloudTestUtils::FScratchAccounts Store = SiegeCloudTestUtils::MakeScratchAccounts();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeAccountSubsystem inside a UGameInstance."));
		return false;
	}

	// GUEST: both getters answer the pinned safe defaults (P2.1: "FDateTime()
	// when guest/unlinked" / "empty when guest/unlinked").
	TestTrue(TEXT("Guest GetLastSyncUtc is FDateTime() (zero ticks — the never-synced default)"),
		Store.Accounts->GetLastSyncUtc() == FDateTime());
	TestTrue(TEXT("Guest GetCloudRefreshToken is empty (the P2.1 pin)"),
		Store.Accounts->GetCloudRefreshToken().IsEmpty());

	// UNLINKED PROFILE: same defaults — a fresh local profile has no cloud state.
	FString Reason;
	TestTrue(TEXT("CreateAccount succeeds"),
		Store.Accounts->CreateAccount(TEXT("ReAuthGetterTester"), TEXT("password1"), Reason));
	Guard.TrackActiveProfile(*Store.Accounts);

	TestTrue(TEXT("An unlinked profile's GetLastSyncUtc is FDateTime()"),
		Store.Accounts->GetLastSyncUtc() == FDateTime());
	TestTrue(TEXT("An unlinked profile's GetCloudRefreshToken is empty"),
		Store.Accounts->GetCloudRefreshToken().IsEmpty());

	// LINK + STAMP, then the two value round trips (the 647 header's named
	// "value readback" gap, closed by the P2.1 getters).
	const FString LinkEmail = TEXT("reauth.getter.tester@example.com");
	const FString LinkUserId = TEXT("9a1b2c3d-4e5f-4a6b-8c7d-0e1f2a3b4c5d"); // arbitrary uuid, not a live account
	const FString LinkRefreshToken = TEXT("scratch-refresh-token-not-a-real-credential");
	const FDateTime SyncStamp(2026, 8, 23, 18, 30, 0);

	Store.Accounts->SetCloudLink(LinkEmail, LinkUserId, LinkRefreshToken);
	TestEqualSensitive(TEXT("SetCloudLink -> GetCloudRefreshToken round-trips the stored token byte-identically"),
		Store.Accounts->GetCloudRefreshToken(), LinkRefreshToken);

	Store.Accounts->SetLastSyncUtc(SyncStamp);
	TestTrue(TEXT("SetLastSyncUtc -> GetLastSyncUtc round-trips the stamp (the R2 pull-baseline value)"),
		Store.Accounts->GetLastSyncUtc() == SyncStamp);

	// PURE-READ DISCIPLINE (the P2.1 trailing comments: "pure read, no
	// mutation, no broadcast"): a burst of reads moves neither the broadcast
	// counter nor the (deleted) registry file.
	SiegeCloudTestUtils::DeleteScratchRegistrySlot();
	const int32 BroadcastsBeforeReads = Store.Accounts->ActiveProfileChangedBroadcastCount;
	for (int32 ReadIndex = 0; ReadIndex < 3; ++ReadIndex)
	{
		Store.Accounts->GetLastSyncUtc();
		Store.Accounts->GetCloudRefreshToken();
	}
	TestEqual(TEXT("The getters broadcast nothing (pure reads — no delegate)"),
		Store.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBeforeReads);
	TestFalse(TEXT("The getters wrote no registry file (pure reads — no save)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	// TOKEN ROTATION through 644's guards — the R1 re-store shape,
	// SetCloudLink(GetLinkedEmail(), <same uid>, NewToken): a NEW token with
	// the SAME email + user id is a REAL mutation (the identical-values guard
	// needs all THREE identical) — exactly one broadcast, a registry save, and
	// the rotated value reads back.
	const FString RotatedRefreshToken = TEXT("scratch-refresh-token-rotated-not-a-real-credential");
	const int32 BroadcastsBeforeRotate = Store.Accounts->ActiveProfileChangedBroadcastCount;
	Store.Accounts->SetCloudLink(Store.Accounts->GetLinkedEmail(), LinkUserId, RotatedRefreshToken);
	TestEqualSensitive(TEXT("A rotated token with identical email/user id STORES (644's no-op guard needs all three identical)"),
		Store.Accounts->GetCloudRefreshToken(), RotatedRefreshToken);
	TestEqual(TEXT("The rotation broadcast exactly once (a REAL mutation — the upheld 644 decision 4)"),
		Store.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBeforeRotate + 1);
	TestTrue(TEXT("The rotation SAVED the registry slot (save-on-change)"),
		UGameplayStatics::DoesSaveGameExist(SiegeCloudTestUtils::ScratchRegistrySlotName, USiegeAccountSaveGame::UserIndex));

	// IDENTICAL RE-STORE: the same triple is a complete no-op (the delegate law).
	const int32 BroadcastsBeforeNoOp = Store.Accounts->ActiveProfileChangedBroadcastCount;
	Store.Accounts->SetCloudLink(LinkEmail, LinkUserId, RotatedRefreshToken);
	TestEqual(TEXT("Re-storing the identical triple broadcasts nothing (the delegate law)"),
		Store.Accounts->ActiveProfileChangedBroadcastCount, BroadcastsBeforeNoOp);

	// CROSS-INSTANCE: the persisted values a NEXT session's engine reads — the
	// closest offline pin of the R2 baseline-consumption seam (MakeContext()
	// itself sits behind the network preflight and is live-smoke territory).
	SiegeCloudTestUtils::FScratchAccounts Reloaded = SiegeCloudTestUtils::MakeScratchAccounts();
	if (!Reloaded.IsValid())
	{
		AddError(TEXT("Could not construct the second USiegeAccountSubsystem for the reload check."));
		return false;
	}
	Reloaded.Accounts->LoadAccountsFromSlot();
	TestTrue(TEXT("GetLastSyncUtc survives the save/load round trip (the persisted R2 baseline)"),
		Reloaded.Accounts->GetLastSyncUtc() == SyncStamp);
	TestEqualSensitive(TEXT("GetCloudRefreshToken survives the save/load round trip (the persisted R1 token)"),
		Reloaded.Accounts->GetCloudRefreshToken(), RotatedRefreshToken);

	// CLEAR: both getters return to the safe defaults (ClearCloudLink resets
	// ALL FOUR cloud fields — the stale-clock law).
	Reloaded.Accounts->ClearCloudLink();
	TestTrue(TEXT("ClearCloudLink resets GetLastSyncUtc to FDateTime()"),
		Reloaded.Accounts->GetLastSyncUtc() == FDateTime());
	TestTrue(TEXT("ClearCloudLink empties GetCloudRefreshToken"),
		Reloaded.Accounts->GetCloudRefreshToken().IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
