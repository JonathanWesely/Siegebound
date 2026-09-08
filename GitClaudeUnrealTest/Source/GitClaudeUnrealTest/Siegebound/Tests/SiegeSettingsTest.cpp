// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/SiegeDeckSaveGame.h"
#include "Siegebound/SiegeSettingsSaveGame.h"
#include "Siegebound/SiegeSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for the persisted settings store (batch SETTINGS+CONFIRM,
 *  TASK-436; CONVENTIONS "Settings screen…" §2, §8, §10).
 *
 *  WHY THESE ARE CHEAP AND WORTH IT: the settings store needs no UWorld, no PIE
 *  session, no model and no widget. It is a bool, a slot and a delegate, so its
 *  whole contract — the default, the round-trip, the missing-slot fallback and
 *  the no-op rule — is assertable in-process. Everything that is NOT assertable
 *  here is named in handoffs/TASK-436-programmer.md rather than implied.
 *
 *  ⛔ THE TESTS NEVER TOUCH THE PLAYER'S REAL SETTINGS FILE, AND THAT IS A
 *  DELIBERATE PROPERTY, NOT LUCK. Every subsystem instance built below is
 *  redirected to the scratch slot "SiegeSettings_AutomationScratch" via
 *  USiegeSettingsSubsystem::SetSlotNameForAutomationTests BEFORE any load or
 *  save, and FScratchSlotGuard deletes that slot on the way into AND out of each
 *  test. A test run therefore cannot read, write or delete
 *  Saved/SaveGames/SiegeSettings.sav. The first test asserts the two slot names
 *  differ, so the guarantee is mechanical rather than a comment.
 *
 *  ⚠️ AND THE ONE PIECE OF ENGINE PLUMBING THAT IS NOT OPTIONAL HERE:
 *  UGameInstanceSubsystem is declared UCLASS(Abstract, Within = GameInstance),
 *  so a subsystem object MUST be constructed with a UGameInstance as its Outer.
 *  A bare NewObject<USiegeSettingsSubsystem>() lands in the transient package
 *  and trips the ClassWithin check in StaticAllocateObject
 *  (UObjectGlobals.cpp) — an ensure in the editor and a hard check elsewhere.
 *  Hence the throwaway UGameInstance in MakeScratchStore(). The subsystem's own
 *  Initialize(FSubsystemCollectionBase&) is never called: a collection cannot be
 *  fabricated outside the engine's own creation path, which is exactly why
 *  LoadSettingsFromSlot() is a separately callable public function.
 */

namespace SiegeSettingsTestUtils
{
	/** ⛔ NEVER the shipped slot. Asserted, not assumed — see the SlotContract test. */
	static const TCHAR* ScratchSlotName = TEXT("SiegeSettings_AutomationScratch");

	static void DeleteScratchSlot()
	{
		if (UGameplayStatics::DoesSaveGameExist(ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex))
		{
			UGameplayStatics::DeleteGameInSlot(ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex);
		}
	}

	/** Deletes the scratch slot on the way in AND on the way out, so no test inherits or leaves disk state. */
	struct FScratchSlotGuard
	{
		FScratchSlotGuard() { DeleteScratchSlot(); }
		~FScratchSlotGuard() { DeleteScratchSlot(); }
	};

	/**
	 *  A settings subsystem wired to the scratch slot, plus the throwaway
	 *  UGameInstance it must live inside (Within = GameInstance). Both are held
	 *  by TStrongObjectPtr so a GC pass mid-test cannot collect either — the
	 *  Outer chain does NOT keep an object alive on its own.
	 */
	struct FScratchStore
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<USiegeSettingsSubsystem> Settings;

		bool IsValid() const { return GameInstance.IsValid() && Settings.IsValid(); }
	};

	static FScratchStore MakeScratchStore()
	{
		FScratchStore Store;
		// GetTransientPackageAsObject() rather than GetTransientPackage(): it
		// hands back a UObject* directly, so this file needs no complete UPackage
		// type (it is the same object the default NewObject outer uses).
		Store.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		if (Store.GameInstance.IsValid())
		{
			Store.Settings.Reset(NewObject<USiegeSettingsSubsystem>(Store.GameInstance.Get()));
			if (Store.Settings.IsValid())
			{
				// BEFORE any load or save — this is what keeps the player's real
				// SiegeSettings.sav untouched.
				Store.Settings->SetSlotNameForAutomationTests(ScratchSlotName);
			}
		}
		return Store;
	}

	/** Writes a FOREIGN SaveGame class into the scratch slot (the corrupt/wrong-class case). */
	static bool WriteForeignSaveGameToScratchSlot()
	{
		USaveGame* Foreign = UGameplayStatics::CreateSaveGameObject(USiegeDeckSaveGame::StaticClass());
		return Foreign && UGameplayStatics::SaveGameToSlot(Foreign, ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex);
	}
}

/**
 *  THE SLOT CONTRACT. A changed slot name silently orphans every player's saved
 *  settings — no error, no warning, settings simply revert to defaults — so the
 *  string is asserted here rather than trusted to review (CONVENTIONS §10).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsSlotContractTest,
	"Siegebound.Settings.SlotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsSlotContractTest::RunTest(const FString& Parameters)
{
	TestEqualSensitive(TEXT("SettingsSlotName is exactly \"SiegeSettings\""),
		FString(USiegeSettingsSubsystem::SettingsSlotName), FString(TEXT("SiegeSettings")));

	TestEqual(TEXT("The settings slot uses user index 0"),
		USiegeSettingsSubsystem::SettingsUserIndex, 0);

	TestEqualSensitive(TEXT("The delegate payload name matches the SaveGame field name"),
		USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute.ToString(),
		FString(TEXT("bAssistantConfirmBeforeExecute")));

	// The hermeticity guarantee, made mechanical: no test below can address the
	// shipped slot, because the scratch name is a different string.
	TestNotEqual(TEXT("The automation scratch slot is NOT the shipped settings slot"),
		FString(SiegeSettingsTestUtils::ScratchSlotName), FString(USiegeSettingsSubsystem::SettingsSlotName));

	return true;
}

/**
 *  THE DEFAULT IS true (CONFIRM ON) ON A FRESH STORE, AND THE SUBSYSTEM'S
 *  COMPILED DEFAULT AGREES WITH THE SaveGame'S. The second half is the drift
 *  guard: the fallback is read off the SaveGame CDO at load time, so if someone
 *  retunes one default and not the other, this fails instead of shipping a
 *  store whose pre-load value disagrees with its post-fallback value.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsDefaultsTest,
	"Siegebound.Settings.DefaultIsConfirmOn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const USiegeSettingsSaveGame* SaveGameDefaults = GetDefault<USiegeSettingsSaveGame>();
	if (!SaveGameDefaults)
	{
		AddError(TEXT("USiegeSettingsSaveGame has no CDO — the fallback contract has no source of truth."));
		return false;
	}

	// The measured default (CONVENTIONS §5): 4 of 5 stable eval failures are
	// wrong-place / wrong-count, exactly what a ground preview catches at a
	// glance. A test that "fixes" this to false has changed the feature.
	TestTrue(TEXT("USiegeSettingsSaveGame defaults bAssistantConfirmBeforeExecute to TRUE"),
		SaveGameDefaults->bAssistantConfirmBeforeExecute);

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	// No load has run: this is the compiled-in value, not a disk value.
	TestTrue(TEXT("A fresh subsystem reports confirm ENABLED before any load"),
		Store.Settings->IsAssistantConfirmEnabled());

	TestEqual(TEXT("The subsystem's compiled default agrees with the SaveGame's (drift guard)"),
		Store.Settings->IsAssistantConfirmEnabled() ? 1 : 0,
		SaveGameDefaults->bAssistantConfirmBeforeExecute ? 1 : 0);

	// Nothing above touched the disk, so no broadcast can have happened.
	TestEqual(TEXT("Constructing a store broadcasts nothing"), Store.Settings->SettingsChangeBroadcastCount, 0);

	return true;
}

/**
 *  A MISSING SLOT YIELDS DEFAULTS AND DOES NOT CRASH — both on a first run and
 *  after the file is removed underneath a store that had loaded a non-default
 *  value (the fallback must ASSIGN the default, not leave stale memory).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsMissingSlotTest,
	"Siegebound.Settings.MissingSlotYieldsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsMissingSlotTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	TestFalse(TEXT("Pre-condition: the scratch slot does not exist"),
		UGameplayStatics::DoesSaveGameExist(SiegeSettingsTestUtils::ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex));

	// First run: load with nothing on disk.
	Store.Settings->LoadSettingsFromSlot();
	TestTrue(TEXT("A load with no save file yields the default (confirm ON)"),
		Store.Settings->IsAssistantConfirmEnabled());

	// Idempotent: loading a missing slot repeatedly must not crash or drift.
	Store.Settings->LoadSettingsFromSlot();
	Store.Settings->LoadSettingsFromSlot();
	TestTrue(TEXT("Repeated loads of a missing slot stay at the default and do not crash"),
		Store.Settings->IsAssistantConfirmEnabled());
	TestEqual(TEXT("Loading a missing slot over an unchanged value broadcasts nothing"),
		Store.Settings->SettingsChangeBroadcastCount, 0);

	// Now the harder half: a non-default value in memory and on disk, then the
	// file disappears. The fallback must restore the default.
	Store.Settings->SetAssistantConfirmEnabled(false);
	TestFalse(TEXT("Value is false before the slot is removed"), Store.Settings->IsAssistantConfirmEnabled());

	SiegeSettingsTestUtils::DeleteScratchSlot();
	TestFalse(TEXT("The scratch slot is gone"),
		UGameplayStatics::DoesSaveGameExist(SiegeSettingsTestUtils::ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex));

	Store.Settings->LoadSettingsFromSlot();
	TestTrue(TEXT("A load with the save file removed RESTORES the default rather than keeping the stale value"),
		Store.Settings->IsAssistantConfirmEnabled());

	return true;
}

/**
 *  A FOREIGN / WRONG-CLASS SLOT YIELDS DEFAULTS AND DOES NOT CRASH. This is the
 *  case the Cast<> exists for: the file is present and readable, but it is not a
 *  USiegeSettingsSaveGame (a corrupt file, a slot-name collision, an older
 *  build). CONVENTIONS §2: "missing / unreadable / wrong class ⇒ fall back to
 *  the C++ defaults, log once, never crash."
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsForeignSlotTest,
	"Siegebound.Settings.ForeignSlotClassYieldsDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsForeignSlotTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	// Drive the in-memory value away from the default so "we got the default
	// back" cannot be confused with "nothing happened".
	Store.Settings->SetAssistantConfirmEnabled(false);
	TestFalse(TEXT("Pre-condition: in-memory value is false"), Store.Settings->IsAssistantConfirmEnabled());

	if (!SiegeSettingsTestUtils::WriteForeignSaveGameToScratchSlot())
	{
		AddError(TEXT("Could not write a foreign SaveGame into the scratch slot — the wrong-class path is untested."));
		return false;
	}

	TestTrue(TEXT("The scratch slot now holds a readable but FOREIGN SaveGame"),
		UGameplayStatics::DoesSaveGameExist(SiegeSettingsTestUtils::ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex));

	Store.Settings->LoadSettingsFromSlot();

	TestTrue(TEXT("A foreign-class slot falls back to the default (confirm ON) instead of crashing"),
		Store.Settings->IsAssistantConfirmEnabled());

	return true;
}

/** SET → GET ROUND-TRIPS IN MEMORY, both directions, including a same-value write. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsSetGetRoundTripTest,
	"Siegebound.Settings.SetGetRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsSetGetRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	Store.Settings->SetAssistantConfirmEnabled(false);
	TestFalse(TEXT("set(false) → get() is false"), Store.Settings->IsAssistantConfirmEnabled());

	Store.Settings->SetAssistantConfirmEnabled(true);
	TestTrue(TEXT("set(true) → get() is true"), Store.Settings->IsAssistantConfirmEnabled());

	Store.Settings->SetAssistantConfirmEnabled(true);
	TestTrue(TEXT("A same-value write leaves the value alone"), Store.Settings->IsAssistantConfirmEnabled());

	return true;
}

/**
 *  SAVE → LOAD ROUND-TRIPS THE VALUE ACROSS INSTANCES. A second store reading
 *  the same slot is the closest in-process stand-in for the real contract, which
 *  is a value surviving an application restart between L_MainMenu and L_Arena.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsSaveLoadRoundTripTest,
	"Siegebound.Settings.SaveLoadRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsSaveLoadRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Writer = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Writer.IsValid())
	{
		AddError(TEXT("Could not construct the writing USiegeSettingsSubsystem."));
		return false;
	}

	// false is the non-default value, so a reader that returns it cannot be
	// returning its own compiled default by accident.
	Writer.Settings->SetAssistantConfirmEnabled(false);

	TestTrue(TEXT("Setting a NEW value wrote the slot to disk"),
		UGameplayStatics::DoesSaveGameExist(SiegeSettingsTestUtils::ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex));

	SiegeSettingsTestUtils::FScratchStore Reader = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Reader.IsValid())
	{
		AddError(TEXT("Could not construct the reading USiegeSettingsSubsystem."));
		return false;
	}

	TestTrue(TEXT("Pre-condition: the reader starts at its compiled default (true)"),
		Reader.Settings->IsAssistantConfirmEnabled());

	Reader.Settings->LoadSettingsFromSlot();
	TestFalse(TEXT("save(false) → load() returns false in a SEPARATE store"),
		Reader.Settings->IsAssistantConfirmEnabled());
	TestEqual(TEXT("A load that CHANGES the value broadcasts exactly once"),
		Reader.Settings->SettingsChangeBroadcastCount, 1);

	// A second load of the same file changes nothing and must not broadcast.
	Reader.Settings->LoadSettingsFromSlot();
	TestFalse(TEXT("Re-loading the same file keeps the value"), Reader.Settings->IsAssistantConfirmEnabled());
	TestEqual(TEXT("A load that changes NOTHING broadcasts nothing"),
		Reader.Settings->SettingsChangeBroadcastCount, 1);

	// And the other direction, so the round-trip is not a one-value fluke.
	SiegeSettingsTestUtils::FScratchStore SecondReader = SiegeSettingsTestUtils::MakeScratchStore();
	if (!SecondReader.IsValid())
	{
		AddError(TEXT("Could not construct the second reading USiegeSettingsSubsystem."));
		return false;
	}

	// ⚠️ WARN-436-2 (qa/TASK-439.md §5). THE READER IS DRIVEN OFF `true` BEFORE
	// THE WRITER STORES `true`, AND THAT ORDERING IS THE ENTIRE FIX. A fresh
	// store's compiled default is ALREADY `true`, so "load() returns true" used
	// to be satisfied by a load that did nothing whatsoever — the assertion could
	// not tell "loaded true from disk" from "never loaded at all", i.e. it was
	// satisfied by the very state it exists to rule out. ⚖️ THAT IS THE SAME
	// DEFECT SHAPE AS A COMPARATOR THAT CANNOT SEE CASING (CONVENTIONS §13), and
	// it is why this file's sweep and this fix landed in one task.
	// Only a store sitting at `false` can make the `true` come from the slot.
	// NOTE the write below also lands in the scratch slot — the writer's line
	// then deliberately overwrites it, which is exactly the disk state this
	// direction needs, and is why the two statements are in this order.
	SecondReader.Settings->SetAssistantConfirmEnabled(false);
	TestFalse(TEXT("Pre-condition: the second reader sits at false, NOT at its compiled default"),
		SecondReader.Settings->IsAssistantConfirmEnabled());

	Writer.Settings->SetAssistantConfirmEnabled(true);

	SecondReader.Settings->LoadSettingsFromSlot();
	TestTrue(TEXT("save(true) → load() returns true in a SEPARATE store that was sitting at false"),
		SecondReader.Settings->IsAssistantConfirmEnabled());

	return true;
}

/**
 *  THE DELEGATE FIRES ON A REAL CHANGE AND NEVER ON A NO-OP WRITE (the delegate
 *  law + the qa/TASK-005 major-2 lesson).
 *
 *  ⚠️ WHAT THIS OBSERVES, STATED PLAINLY: SettingsChangeBroadcastCount, which is
 *  incremented inside USiegeSettingsSubsystem::BroadcastSettingChanged — THE
 *  ONLY function in SiegeSettingsSubsystem.cpp that calls
 *  OnSettingsChanged.Broadcast. So the counter cannot diverge from the
 *  broadcasts, but the counter is what is being read. Binding the real dynamic
 *  multicast would require a UFUNCTION on a UCLASS, and a UCLASS cannot be
 *  declared in a test .cpp (it needs a UHT-generated .generated.h from a
 *  header). The end-to-end path — bind → toggle → widget updates — closes on
 *  Jonathan's pixel check at TASK-448, not here.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsDelegateTest,
	"Siegebound.Settings.DelegateFiresOnRealChangeOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsDelegateTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	TestEqual(TEXT("A fresh store has broadcast nothing"), Store.Settings->SettingsChangeBroadcastCount, 0);

	// A REAL change → exactly one broadcast, carrying the right setting name.
	Store.Settings->SetAssistantConfirmEnabled(false);
	TestEqual(TEXT("A real change broadcasts exactly once"), Store.Settings->SettingsChangeBroadcastCount, 1);
	TestEqualSensitive(TEXT("The broadcast names the confirm setting"),
		Store.Settings->LastBroadcastSettingName.ToString(),
		USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute.ToString());

	// A NO-OP write → no further broadcast. ⛔ This is the assertion that keeps
	// consumers able to trust the delegate.
	Store.Settings->SetAssistantConfirmEnabled(false);
	TestEqual(TEXT("A same-value write broadcasts NOTHING"), Store.Settings->SettingsChangeBroadcastCount, 1);
	TestFalse(TEXT("A same-value write leaves the value alone"), Store.Settings->IsAssistantConfirmEnabled());

	// Changing back is a real change again.
	Store.Settings->SetAssistantConfirmEnabled(true);
	TestEqual(TEXT("Changing back broadcasts once more"), Store.Settings->SettingsChangeBroadcastCount, 2);

	Store.Settings->SetAssistantConfirmEnabled(true);
	TestEqual(TEXT("A second same-value write still broadcasts NOTHING"),
		Store.Settings->SettingsChangeBroadcastCount, 2);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1120 [GFX-FPS] — SETTING #2: bShowFrameRateCounter (GFX-§7 / GFX-§3)
//
//  ⛔ WHY THESE LIVE HERE AND NOT IN Tests/SiegeGraphicsSettingsTest.cpp (which
//  is what the board's `names:` line says): the code under test is
//  USiegeSettingsSubsystem, and this file already owns its harness — the scratch
//  slot NAME, MakeScratchStore(), the throwaway UGameInstance and, decisively,
//  FScratchSlotGuard. Duplicating that harness elsewhere would give the SAME
//  scratch slot string two independent owners and two independent guards, which
//  is a hazard rather than a tidiness question. This is the qa/TASK-1119.md F-6
//  ruling applied unchanged ("the harness is there … duplicating it would be
//  strictly worse"), and it is declared as a flagged decision in the handoff
//  rather than left for the gate to notice.
// ═════════════════════════════════════════════════════════════════════════════

/**
 *  🚨⭐ THE STEP-5 CATCHER, AND IT IS THE MOST IMPORTANT TEST THIS ROW ADDS.
 *
 *  USiegeSettingsSubsystem::SaveSettingsToSlot() copies the in-memory values onto
 *  a FRESHLY CONSTRUCTED SaveGame object FIELD BY FIELD. A setting added by the
 *  header recipe's steps 1-4 alone — UPROPERTY, FName, getter/setter, loader —
 *  compiles, runs, broadcasts, updates the UI and reports a SUCCESSFUL save,
 *  while writing its C++ default to disk every single time. The preference then
 *  evaporates on the next launch with nothing in the log to point at.
 *
 *  ⛔ THE SHAPE THAT MAKES THIS DETECTABLE IS "LOAD INTO A **SECOND** STORE". A
 *  test that set the value and re-read the SAME store would pass with the save
 *  line missing, because the in-memory value is correct either way — it would be
 *  a test that cannot fail for the reason it exists.
 *
 *  ⭐ AND IT ASSERTS BOTH SETTINGS ACROSS ONE ROUND TRIP, which is the other half:
 *  a save that wrote setting #2 by clobbering setting #1 (or a load that applied
 *  one field's value to the other's member) would satisfy either assertion alone
 *  and fails this pair.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsFrameRateCounterRoundTripTest,
	"Siegebound.Settings.FrameRateCounterSaveLoadRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsFrameRateCounterRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	SiegeSettingsTestUtils::FScratchStore Writer = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Writer.IsValid())
	{
		AddError(TEXT("Could not construct the writing USiegeSettingsSubsystem."));
		return false;
	}

	// BOTH settings moved OFF their compiled defaults, so neither value a reader
	// returns can be its own default arriving by accident.
	Writer.Settings->SetFrameRateCounterEnabled(true);   // default is false
	Writer.Settings->SetAssistantConfirmEnabled(false);  // default is true

	TestTrue(TEXT("Setting the FPS-counter preference wrote the slot to disk"),
		UGameplayStatics::DoesSaveGameExist(SiegeSettingsTestUtils::ScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex));

	SiegeSettingsTestUtils::FScratchStore Reader = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Reader.IsValid())
	{
		AddError(TEXT("Could not construct the reading USiegeSettingsSubsystem."));
		return false;
	}

	TestFalse(TEXT("Pre-condition: a fresh reader starts at the compiled default (counter OFF)"),
		Reader.Settings->IsFrameRateCounterEnabled());
	TestTrue(TEXT("Pre-condition: a fresh reader starts at the compiled default (confirm ON)"),
		Reader.Settings->IsAssistantConfirmEnabled());

	Reader.Settings->LoadSettingsFromSlot();

	// ⛔ THE ASSERTION THAT REDDENS IF SaveSettingsToSlot() FORGETS THE FIELD.
	TestTrue(TEXT("save(counter=true) → a SEPARATE store loads true (recipe step 5 is present)"),
		Reader.Settings->IsFrameRateCounterEnabled());

	// ⛔ AND THE ONE THAT REDDENS IF SETTING #2 CLOBBERED SETTING #1.
	TestFalse(TEXT("The confirm toggle survived the same round trip unharmed"),
		Reader.Settings->IsAssistantConfirmEnabled());

	// Both changed, so the load broadcast twice — once per field, never once for
	// two fields and never three times for two.
	TestEqual(TEXT("A load that changes BOTH settings broadcasts exactly twice"),
		Reader.Settings->SettingsChangeBroadcastCount, 2);

	// Re-loading the identical file changes nothing and must broadcast nothing.
	Reader.Settings->LoadSettingsFromSlot();
	TestTrue(TEXT("Re-loading the same file keeps the counter preference"),
		Reader.Settings->IsFrameRateCounterEnabled());
	TestEqual(TEXT("A load that changes NOTHING broadcasts nothing"),
		Reader.Settings->SettingsChangeBroadcastCount, 2);

	// ⭐ THE OTHER DIRECTION, and its pre-condition is the load-bearing half: a
	// store already sitting at `true` is the only one for which "loads false" can
	// possibly have come from the slot rather than from never loading at all.
	// (This is WARN-436-2's lesson, applied to the new field on purpose.)
	SiegeSettingsTestUtils::FScratchStore SecondReader = SiegeSettingsTestUtils::MakeScratchStore();
	if (!SecondReader.IsValid())
	{
		AddError(TEXT("Could not construct the second reading USiegeSettingsSubsystem."));
		return false;
	}

	SecondReader.Settings->SetFrameRateCounterEnabled(true);
	TestTrue(TEXT("Pre-condition: the second reader sits at true, NOT at its compiled default"),
		SecondReader.Settings->IsFrameRateCounterEnabled());

	// The writer's save now overwrites what the line above just wrote to the same
	// scratch slot — which is exactly the disk state this direction needs.
	Writer.Settings->SetFrameRateCounterEnabled(false);

	SecondReader.Settings->LoadSettingsFromSlot();
	TestFalse(TEXT("save(counter=false) → a store sitting at true loads false"),
		SecondReader.Settings->IsFrameRateCounterEnabled());

	return true;
}

/**
 *  ⛔ DEFAULT OFF (board cl. 4), AND THE TWO COMPILED DEFAULTS AGREE.
 *
 *  The second half is the drift guard, and it is not decorative: the subsystem
 *  carries its own `bShowFrameRateCounter = false` member initialiser AND the
 *  SaveGame carries one, and only the SaveGame's is ever used as the load
 *  fallback. Retuning one and not the other would ship a store whose pre-load
 *  value disagrees with its post-fallback value — visible to a player as a
 *  counter that appears, then vanishes the first time anything reloads.
 *
 *  ⚠️ SCOPE, STATED RATHER THAN IMPLIED (`SC-§79`): board cl. (5) asks for "an
 *  ABSENT field in an older .sav loads at the C++ default", and ⛔ THAT EXACT
 *  CASE IS NOT PROVABLE IN THIS PROCESS. Producing a `.sav` that carries
 *  `bAssistantConfirmBeforeExecute` and NOT `bShowFrameRateCounter` requires a
 *  build in which the second UPROPERTY does not exist; any file this test can
 *  write carries both tags. What IS proved here is every reachable neighbour of
 *  it: the CDO default, the two defaults agreeing, the MISSING-slot fallback and
 *  the FOREIGN-class fallback — all three of which funnel through the same
 *  `Source = Loaded ? Loaded : GetDefault<>()` line the absent-field case would
 *  use. The tagged-property behaviour itself is UE's, asserted by UE, and is
 *  recorded in the handoff as an ACCEPTED PREMISE rather than a covered one.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsFrameRateCounterDefaultsTest,
	"Siegebound.Settings.FrameRateCounterDefaultsOffAndAgree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsFrameRateCounterDefaultsTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	const USiegeSettingsSaveGame* SaveDefaults = GetDefault<USiegeSettingsSaveGame>();
	if (SaveDefaults == nullptr)
	{
		AddError(TEXT("GetDefault<USiegeSettingsSaveGame>() returned null."));
		return false;
	}

	TestFalse(TEXT("⛔ The SaveGame's compiled default is OFF — a counter is opt-in, never opt-out"),
		SaveDefaults->bShowFrameRateCounter);

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	TestEqual(TEXT("The subsystem's compiled default AGREES with the SaveGame's (drift guard)"),
		Store.Settings->IsFrameRateCounterEnabled(), SaveDefaults->bShowFrameRateCounter);

	// ---- the MISSING-slot fallback -----------------------------------------
	// The guard deleted the scratch slot on the way in, so nothing is on disk.
	Store.Settings->LoadSettingsFromSlot();
	TestFalse(TEXT("A missing slot leaves the counter preference at the C++ default"),
		Store.Settings->IsFrameRateCounterEnabled());
	TestEqual(TEXT("A missing-slot load changes nothing and therefore broadcasts nothing"),
		Store.Settings->SettingsChangeBroadcastCount, 0);

	// ---- the FOREIGN-class fallback ----------------------------------------
	// ⛔ THE PRE-CONDITION IS THE TEST. Driving the store to `true` FIRST is what
	// makes the `false` below have to come from the CDO fallback: on a store still
	// at its default, "loads false" would be satisfied by a load that did nothing
	// whatsoever — the same defect shape WARN-436-2 named.
	SiegeSettingsTestUtils::FScratchStore ForeignReader = SiegeSettingsTestUtils::MakeScratchStore();
	if (!ForeignReader.IsValid())
	{
		AddError(TEXT("Could not construct the foreign-slot reading USiegeSettingsSubsystem."));
		return false;
	}

	ForeignReader.Settings->SetFrameRateCounterEnabled(true);
	TestTrue(TEXT("Pre-condition: the foreign-slot reader sits at true, NOT at its default"),
		ForeignReader.Settings->IsFrameRateCounterEnabled());

	if (!SiegeSettingsTestUtils::WriteForeignSaveGameToScratchSlot())
	{
		AddError(TEXT("Could not write a foreign SaveGame class into the scratch slot."));
		return false;
	}

	ForeignReader.Settings->LoadSettingsFromSlot();
	TestFalse(TEXT("A FOREIGN-class slot falls back to the C++ default (counter OFF), never a crash"),
		ForeignReader.Settings->IsFrameRateCounterEnabled());

	return true;
}

/**
 *  ⭐⭐ THE DELEGATE NAMES **WHICH** SETTING CHANGED — ASSERTED AS STATE, NOT AS A
 *  TALLY, AND THAT CHOICE IS THIS LANE'S OWN LESSON.
 *
 *  🚨 `qa/TASK-1119.md` § LOOP 1 found `M21` reading GREEN under both the fixed
 *  and the broken branch by arithmetic coincidence, and `qa/TASK-1114.md` found a
 *  facade counter reading `0` on broken and fixed alike. ⛔ A BROADCAST COUNT
 *  CANNOT DISCRIMINATE A SWAPPED PAYLOAD: a setter that forwarded the CONFIRM
 *  toggle's FName while writing the frame-counter member would produce exactly
 *  the same count as the correct code, and every count-based assertion below
 *  would stay green — while the Graphics panel's row and the in-match counter,
 *  both of which FILTER on this token, silently stopped updating.
 *  ⇒ every discriminating assertion here reads `LastBroadcastSettingName`.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSettingsFrameRateCounterDelegateTest,
	"Siegebound.Settings.FrameRateCounterDelegateNamesTheRightSetting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSettingsFrameRateCounterDelegateTest::RunTest(const FString& Parameters)
{
	SiegeSettingsTestUtils::FScratchSlotGuard SlotGuard;

	// The pinned token, character-for-character (`GFX-§10`). A changed string
	// silently unhooks every consumer that filters on it, with nothing failing.
	TestEqualSensitive(TEXT("SettingName_ShowFrameRateCounter is exactly \"bShowFrameRateCounter\""),
		USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter.ToString(),
		FString(TEXT("bShowFrameRateCounter")));

	TestNotEqual(TEXT("⛔ The two settings carry DIFFERENT payload names"),
		USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter.ToString(),
		USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute.ToString());

	SiegeSettingsTestUtils::FScratchStore Store = SiegeSettingsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeSettingsSubsystem inside a UGameInstance."));
		return false;
	}

	// STATE, not a tally: nothing has been broadcast, so there is no last name.
	TestTrue(TEXT("A fresh store has broadcast nothing (LastBroadcastSettingName is None)"),
		Store.Settings->LastBroadcastSettingName.IsNone());

	// ---- a NO-OP write ------------------------------------------------------
	// false → false. ⛔ The state assertion is what makes this row honest: if the
	// setter broadcast on a no-op, the NAME would stop being None even though the
	// value did not move.
	Store.Settings->SetFrameRateCounterEnabled(false);
	TestTrue(TEXT("A same-value write broadcasts NOTHING (still no last name)"),
		Store.Settings->LastBroadcastSettingName.IsNone());
	TestFalse(TEXT("A same-value write leaves the value alone"),
		Store.Settings->IsFrameRateCounterEnabled());

	// ---- a REAL change ------------------------------------------------------
	Store.Settings->SetFrameRateCounterEnabled(true);
	TestTrue(TEXT("A real change actually changed the value"),
		Store.Settings->IsFrameRateCounterEnabled());
	TestEqualSensitive(TEXT("⭐ The broadcast names the FRAME-COUNTER setting, not the confirm toggle"),
		Store.Settings->LastBroadcastSettingName.ToString(),
		USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter.ToString());

	// ---- the other setting still speaks for itself --------------------------
	// ⛔ THIS PAIR IS THE DISCRIMINATOR. Moving the OTHER setting must move the
	// name to the OTHER token; a store that answered "frame counter" to both would
	// pass every count-based assertion in this file and fail here.
	Store.Settings->SetAssistantConfirmEnabled(false);
	TestEqualSensitive(TEXT("⭐ Moving the confirm toggle names the CONFIRM setting"),
		Store.Settings->LastBroadcastSettingName.ToString(),
		USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute.ToString());

	Store.Settings->SetFrameRateCounterEnabled(false);
	TestEqualSensitive(TEXT("⭐ And moving the counter back names the counter again"),
		Store.Settings->LastBroadcastSettingName.ToString(),
		USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter.ToString());

	// Four writes, three of them real: the tally is a CORROBORATION of the names
	// above, deliberately not the thing being relied on.
	TestEqual(TEXT("Three real changes and one no-op ⇒ exactly three broadcasts"),
		Store.Settings->SettingsChangeBroadcastCount, 3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
