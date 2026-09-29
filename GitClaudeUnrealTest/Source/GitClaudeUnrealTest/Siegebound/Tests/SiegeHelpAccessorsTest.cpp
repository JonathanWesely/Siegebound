// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeGameMode.h"
#include "Siegebound/SiegePlayerController.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
#include "Engine/Blueprint.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for the Controls help's owner-file accessors (TASK-1592) ═══
 *
 *  Subject: the members TASK-1592 made readable from outside so the Controls help can SHOW his
 *  answer A (each building type's height limit) and "all of it" (the discard fee, Rally's four
 *  values, Attack's three) as live numbers, read from their owners and never typed (`HELP-§2`):
 *    • ASiegePlayerController — ResolveCardActorClass / IsBuildingCard (moved from `private:`,
 *      unchanged), GetCardTableAsset() (new, plain C++), GetDiscardAllCost() (new UFUNCTION);
 *    • AHeroCharacter — GetMeleeRange / GetMeleeHalfAngleDegrees / GetMeleeCooldown /
 *      GetRallyRadius / GetRallySpeedBonus / GetRallyDuration / GetRallyCooldown (new UFUNCTIONs);
 *    • ASiegeGameMode — GetHeroPawnClassAsset() (new, plain C++).
 *  QA gate: TASK-1593. Compile, arms and suite run: TASK-1596.
 *
 *  ⚠️ EVERY PIN HERE CAN FAIL (`SHIP-§9`), and the reason is written beside it:
 *    1. A getter is compared with ITS OWN property, read by reflection BY THE PROPERTY'S NAME on
 *       the same class default, so the expectation never comes from the getter. A getter that
 *       returns a sibling property goes red as long as the two values differ, which is why the
 *       test also asserts that at least two of the 8 values differ, and logs every pair inside
 *       one class that is equal today (a swap between those two would be invisible).
 *    2. The building walk goes through the PUBLIC surface only, on the controller's class
 *       default: the table GetCardTableAsset() names, IsBuildingCard, ResolveCardActorClass. It
 *       asserts the set it walked is non-empty, so a rule that answers false for every card goes
 *       red instead of passing over nothing.
 *    3. The hero class is read through GetHeroPawnClassAsset() on the game mode the arena runs
 *       (the native ASiegeGameMode, via GlobalDefaultGameMode; handoffs/TASK-1592-programmer.md).
 *  Under WITH_EDITOR every resolved class is also checked against its generating Blueprint's
 *  status (UBlueprint::GetBlueprintFromClass, then UBlueprint::Status, compared with BS_Error),
 *  and each class and status is written with AddInfo under the prefix "[HelpAccessors]". Those
 *  lines are TASK-1596's status reading (`VER-§12` cl. 7g): this suite's process is where this
 *  wave first loads the building and hero Blueprints, never the GUI editor.
 *
 *  Mutation arms (owed by TASK-1596, named in handoffs/TASK-1592-programmer.md): arm-G (a hero
 *  getter returns a sibling) reddens test 1; arm-R (IsBuildingCard false for every card)
 *  reddens test 2 on its non-empty guard; arm-H (optional: the hero soft class emptied)
 *  reddens test 3.
 *
 *  🔒 Headless: no world, no spawn, no PIE. Nothing here writes a property, saves an asset or
 *  touches Git.
 */

namespace SiegeHelpAccessorsTestFixture
{
	/** One hero float getter under pin: the property's reflected name, the getter's name for the label, and the getter itself. */
	struct FHeroFloatGetterPin
	{
		const TCHAR* PropertyName;
		const TCHAR* GetterName;
		float (AHeroCharacter::*Getter)() const;
	};

	/** The seven hero getters TASK-1592 added, each beside the ONE property its name says it returns. */
	static const FHeroFloatGetterPin HeroFloatGetterPins[] =
	{
		{ TEXT("MeleeRange"),            TEXT("GetMeleeRange"),            &AHeroCharacter::GetMeleeRange },
		{ TEXT("MeleeHalfAngleDegrees"), TEXT("GetMeleeHalfAngleDegrees"), &AHeroCharacter::GetMeleeHalfAngleDegrees },
		{ TEXT("MeleeCooldown"),         TEXT("GetMeleeCooldown"),         &AHeroCharacter::GetMeleeCooldown },
		{ TEXT("RallyRadius"),           TEXT("GetRallyRadius"),           &AHeroCharacter::GetRallyRadius },
		{ TEXT("RallySpeedBonus"),       TEXT("GetRallySpeedBonus"),       &AHeroCharacter::GetRallySpeedBonus },
		{ TEXT("RallyDuration"),         TEXT("GetRallyDuration"),         &AHeroCharacter::GetRallyDuration },
		{ TEXT("RallyCooldown"),         TEXT("GetRallyCooldown"),         &AHeroCharacter::GetRallyCooldown },
	};

	/** The number of getters test 1 pins: the seven hero floats plus the discard fee. */
	constexpr int32 PinnedGetterCount = 8;

#if WITH_EDITOR
	/** The name of a Blueprint status, spelled from the engine's own enumerators (Engine/Blueprint.h). */
	static const TCHAR* BlueprintStatusName(const EBlueprintStatus Status)
	{
		switch (Status)
		{
		case BS_Unknown:              return TEXT("BS_Unknown");
		case BS_Dirty:                return TEXT("BS_Dirty");
		case BS_Error:                return TEXT("BS_Error");
		case BS_UpToDate:             return TEXT("BS_UpToDate");
		case BS_BeingCreated:         return TEXT("BS_BeingCreated");
		case BS_UpToDateWithWarnings: return TEXT("BS_UpToDateWithWarnings");
		default:                      return TEXT("BS_(unlisted)");
		}
	}

	/**
	 *  Asserts that Class is not generated by a Blueprint whose status is BS_Error, and returns a
	 *  description of that status for the AddInfo line. The engine call is
	 *  UBlueprint::GetBlueprintFromClass (Engine/Blueprint.h, under WITH_EDITORONLY_DATA; its body
	 *  is Cast<UBlueprint>(InClass->ClassGeneratedBy)), then the transient UBlueprint::Status. A
	 *  native class has no generating Blueprint and passes.
	 */
	static FString CheckAndDescribeBlueprintStatus(FAutomationTestBase& Test, const UClass* Class, const FString& Subject)
	{
		const UBlueprint* const Blueprint = UBlueprint::GetBlueprintFromClass(Class);
		if (Blueprint == nullptr)
		{
			return TEXT("native (no generating Blueprint)");
		}

		const EBlueprintStatus Status = Blueprint->Status.GetValue();
		Test.TestTrue(
			*FString::Printf(TEXT("%s is not generated by a Blueprint in BS_Error ('%s' is %s)"),
				*Subject, *Blueprint->GetPathName(), BlueprintStatusName(Status)),
			Status != BS_Error);
		return FString::Printf(TEXT("%s (Blueprint '%s')"), BlueprintStatusName(Status), *Blueprint->GetPathName());
	}
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
//  1. EACH OF THE 8 GETTERS RETURNS ITS OWN PROPERTY
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHelpAccessorsGetterPinTest,
	"Siegebound.HelpAccessors.EachGetterReturnsItsOwnProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHelpAccessorsGetterPinTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHelpAccessorsTestFixture;

	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	const ASiegePlayerController* const ControllerDefaults = GetDefault<ASiegePlayerController>();
	if (!TestNotNull(TEXT("SELF-CHECK: the AHeroCharacter class default resolves"), HeroDefaults)
		|| !TestNotNull(TEXT("SELF-CHECK: the ASiegePlayerController class default resolves"), ControllerDefaults))
	{
		return false;
	}

	// Every value read below, in pin order, for the sibling guard at the end.
	TArray<double> PinnedValues;
	TArray<FString> PinnedNames;
	TArray<double> HeroValues;

	// ── The seven hero getters, each against its own property on the hero's class default ──
	// ⭐ The expectation is the PROPERTY, found by its NAME through reflection: it can never be
	// supplied by the getter under test.
	for (const FHeroFloatGetterPin& Pin : HeroFloatGetterPins)
	{
		const FFloatProperty* const Property = FindFProperty<FFloatProperty>(AHeroCharacter::StaticClass(), Pin.PropertyName);
		if (Property == nullptr)
		{
			AddError(FString::Printf(
				TEXT("⛔ AHeroCharacter has no float UPROPERTY '%s'. %s() is pinned to it by name, so its disappearance or retyping is an ERROR, not a skip."),
				Pin.PropertyName, Pin.GetterName));
			continue;
		}

		const float PropertyValue = Property->GetPropertyValue_InContainer(HeroDefaults);
		const float GetterValue = (HeroDefaults->*Pin.Getter)();

		TestEqual(
			*FString::Printf(TEXT("%s() returns %s (AHeroCharacter class default; the property read by reflection by its name)"),
				Pin.GetterName, Pin.PropertyName),
			GetterValue, PropertyValue, 0.f);

		AddInfo(FString::Printf(TEXT("[HelpAccessors] AHeroCharacter::%s() = %.6g, property %s = %.6g"),
			Pin.GetterName, static_cast<double>(GetterValue), Pin.PropertyName, static_cast<double>(PropertyValue)));

		PinnedValues.Add(static_cast<double>(PropertyValue));
		PinnedNames.Add(FString::Printf(TEXT("AHeroCharacter::%s"), Pin.PropertyName));
		HeroValues.Add(static_cast<double>(PropertyValue));
	}

	// ── The discard fee, against its own property on the controller's class default ────────
	{
		const FIntProperty* const Property = FindFProperty<FIntProperty>(ASiegePlayerController::StaticClass(), TEXT("DiscardAllCost"));
		if (Property == nullptr)
		{
			AddError(TEXT("⛔ ASiegePlayerController has no int32 UPROPERTY 'DiscardAllCost'. GetDiscardAllCost() is pinned to it by name, so its disappearance or retyping is an ERROR, not a skip."));
		}
		else
		{
			const int32 PropertyValue = Property->GetPropertyValue_InContainer(ControllerDefaults);
			const int32 GetterValue = ControllerDefaults->GetDiscardAllCost();

			TestEqual(TEXT("GetDiscardAllCost() returns DiscardAllCost (ASiegePlayerController class default; the property read by reflection by its name)"),
				GetterValue, PropertyValue);

			AddInfo(FString::Printf(TEXT("[HelpAccessors] ASiegePlayerController::GetDiscardAllCost() = %d, property DiscardAllCost = %d"),
				GetterValue, PropertyValue));

			PinnedValues.Add(static_cast<double>(PropertyValue));
			PinnedNames.Add(TEXT("ASiegePlayerController::DiscardAllCost"));
		}
	}

	// ── The sibling guard ──────────────────────────────────────────────────────────────────
	// ⭐ A getter that returns a SIBLING property is only visible when the two values differ. If
	// all 8 were equal, every assertion above would pass against any swap; so at least two must
	// differ, and every equal pair inside one class is named (the swaps this pin cannot see
	// today). Cross-class pairs cannot be swapped: a getter can only return its own class's field.
	TestEqual(TEXT("SELF-CHECK: all 8 getters were pinned (the seven hero floats and the discard fee)"),
		PinnedValues.Num(), PinnedGetterCount);

	TArray<double> DistinctValues;
	for (const double Value : PinnedValues)
	{
		DistinctValues.AddUnique(Value);
	}
	TestTrue(TEXT("At least two of the 8 pinned values differ, so a getter that returns a sibling property can go red"),
		DistinctValues.Num() >= 2);

	int32 InvisibleHeroSwaps = 0;
	for (int32 First = 0; First < HeroValues.Num(); ++First)
	{
		for (int32 Second = First + 1; Second < HeroValues.Num(); ++Second)
		{
			if (HeroValues[First] == HeroValues[Second])
			{
				++InvisibleHeroSwaps;
				AddInfo(FString::Printf(TEXT("[HelpAccessors] %s and %s are equal today (%.6g): a swap between their two getters is invisible to this pin."),
					*PinnedNames[First], *PinnedNames[Second], HeroValues[First]));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("[HelpAccessors] %d distinct values among the %d pinned; %d equal pair(s) inside AHeroCharacter."),
		DistinctValues.Num(), PinnedValues.Num(), InvisibleHeroSwaps));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  2. EVERY BUILDING CARD RESOLVES, THROUGH THE CONTROLLER'S OWN PUBLIC RESOLUTION,
//     TO AN ABuilding CLASS — AND THE SET IS NOT EMPTY
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHelpAccessorsBuildingResolutionTest,
	"Siegebound.HelpAccessors.BuildingCardsResolveThroughTheControllersOwnResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHelpAccessorsBuildingResolutionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHelpAccessorsTestFixture;

	// ⭐ The controller's CLASS DEFAULT is the object the Controls help reads. The game's
	// controller is spawned from the native ASiegePlayerController (ASiegeGameMode's constructor
	// sets PlayerControllerClass to it), and neither member reads per-instance state that the
	// running instance could hold differently (handoffs/TASK-1592-programmer.md §1).
	const ASiegePlayerController* const ControllerDefaults = GetDefault<ASiegePlayerController>();
	if (!TestNotNull(TEXT("SELF-CHECK: the ASiegePlayerController class default resolves"), ControllerDefaults))
	{
		return false;
	}

	// ── The table, named by the public getter (never a typed path) ─────────────────────────
	const TSoftObjectPtr<UDataTable>& CardTableAsset = ControllerDefaults->GetCardTableAsset();
	if (CardTableAsset.IsNull())
	{
		AddError(TEXT("⛔ GetCardTableAsset() is null on the controller's class default. The shipped constructor assigns the card table, so an empty pointer means that assignment was removed, and a walk over nothing would pass VACUOUSLY."));
		return false;
	}

	const UDataTable* const CardTable = CardTableAsset.LoadSynchronous();
	if (CardTable == nullptr)
	{
		AddError(FString::Printf(TEXT("⛔ The card table GetCardTableAsset() names ('%s') did not load as a UDataTable."), *CardTableAsset.ToString()));
		return false;
	}

	if (CardTable->GetRowStruct() != FCardRow::StaticStruct())
	{
		AddError(FString::Printf(TEXT("⛔ The card table '%s' has row struct '%s', not FCardRow: every row read below would be null and the walk would check nothing."),
			*CardTableAsset.ToString(), *GetNameSafe(CardTable->GetRowStruct())));
		return false;
	}

	const UEnum* const CardTypeEnum = StaticEnum<ECardType>();
	AddInfo(FString::Printf(TEXT("[HelpAccessors] card table read through GetCardTableAsset(): '%s'"), *CardTableAsset.ToString()));

	// ── The walk: every row, through IsBuildingCard, then ResolveCardActorClass ──────────────
	int32 RowsRead = 0;
	int32 BuildingCards = 0;
	int32 BuildingClassesResolved = 0;

	for (const FName& CardID : CardTable->GetRowNames())
	{
		const FCardRow* const Row = CardTable->FindRow<FCardRow>(CardID, TEXT("SiegeHelpAccessorsTest"), /*bWarnIfRowMissing=*/ false);
		if (Row == nullptr)
		{
			continue;
		}
		++RowsRead;

		if (!ControllerDefaults->IsBuildingCard(CardID, Row->CardType))
		{
			continue;
		}
		++BuildingCards;

		const FString CardTypeName = CardTypeEnum
			? CardTypeEnum->GetNameStringByValue(static_cast<int64>(Row->CardType))
			: FString::FromInt(static_cast<int32>(Row->CardType));

		UClass* const ActorClass = ControllerDefaults->ResolveCardActorClass(CardID, Row->CardType);
		const bool bIsBuildingClass = ActorClass != nullptr && ActorClass->IsChildOf(ABuilding::StaticClass());
		TestTrue(*FString::Printf(TEXT("Building card '%s' (%s) resolves through ResolveCardActorClass to a class that IsChildOf(ABuilding)"),
			*CardID.ToString(), *CardTypeName), bIsBuildingClass);

		FString StatusText = TEXT("(no class resolved)");
		if (ActorClass != nullptr)
		{
			++BuildingClassesResolved;
#if WITH_EDITOR
			StatusText = CheckAndDescribeBlueprintStatus(*this,
				ActorClass, FString::Printf(TEXT("Building card '%s' class '%s'"), *CardID.ToString(), *ActorClass->GetPathName()));
#else
			StatusText = TEXT("(status not read: not an editor build)");
#endif
		}

		AddInfo(FString::Printf(TEXT("[HelpAccessors] card '%s' (%s) -> class '%s', status %s"),
			*CardID.ToString(), *CardTypeName, ActorClass ? *ActorClass->GetPathName() : TEXT("null"), *StatusText));
	}

	TestTrue(TEXT("SELF-CHECK: the walk read at least one FCardRow from the table GetCardTableAsset() names"), RowsRead > 0);

	// ⭐ THE NON-EMPTY GUARD. Without it, a building rule that answered false for every card would
	// make the walk above check nothing and pass.
	TestTrue(TEXT("At least one card in the table is a building card by IsBuildingCard (the set the Controls help lists is non-empty)"),
		BuildingCards > 0);

	AddInfo(FString::Printf(TEXT("[HelpAccessors] %d rows read, %d building card(s), %d class(es) resolved."),
		RowsRead, BuildingCards, BuildingClassesResolved));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  3. THE HERO CLASS THE ARENA'S GAME MODE NAMES RESOLVES TO AN AHeroCharacter
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeHelpAccessorsHeroClassTest,
	"Siegebound.HelpAccessors.HeroClassResolvesThroughTheGameModesOwnAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeHelpAccessorsHeroClassTest::RunTest(const FString& Parameters)
{
	using namespace SiegeHelpAccessorsTestFixture;

	// ⭐ The arena runs the native ASiegeGameMode (GlobalDefaultGameMode; L_Arena carries no
	// game-mode override, and no Blueprint subclass of it exists): its class default is the
	// object whose HeroPawnClassAsset the game spawns from.
	const ASiegeGameMode* const GameModeDefaults = GetDefault<ASiegeGameMode>();
	if (!TestNotNull(TEXT("SELF-CHECK: the ASiegeGameMode class default resolves"), GameModeDefaults))
	{
		return false;
	}

	const TSoftClassPtr<AHeroCharacter>& HeroClassAsset = GameModeDefaults->GetHeroPawnClassAsset();
	UClass* const HeroClass = HeroClassAsset.LoadSynchronous();

	TestTrue(*FString::Printf(TEXT("GetHeroPawnClassAsset() ('%s') resolves to a class that IsChildOf(AHeroCharacter)"), *HeroClassAsset.ToString()),
		HeroClass != nullptr && HeroClass->IsChildOf(AHeroCharacter::StaticClass()));

	FString StatusText = TEXT("(no class resolved)");
	if (HeroClass != nullptr)
	{
#if WITH_EDITOR
		StatusText = CheckAndDescribeBlueprintStatus(*this,
			HeroClass, FString::Printf(TEXT("Hero class '%s'"), *HeroClass->GetPathName()));
#else
		StatusText = TEXT("(status not read: not an editor build)");
#endif
	}

	AddInfo(FString::Printf(TEXT("[HelpAccessors] hero class '%s' -> '%s', status %s"),
		*HeroClassAsset.ToString(), HeroClass ? *HeroClass->GetPathName() : TEXT("null"), *StatusText));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
