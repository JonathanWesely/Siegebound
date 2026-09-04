// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/UnrealString.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "Siegebound/Building.h"
#include "Siegebound/CardRow.h"
#include "Siegebound/SiegePlayerController.h"
#include "Siegebound/SummonedUnit.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE SPAWNABLE-ROSTER GATE (TASK-947; law: CONVENTIONS `SC-§50` cl. 4,
 *      `SHIP-§9`, `SC-§39`, `SC-§49`, `SC-§38`, `SC-§40` cl. 10, `TL-§5d`) ═══
 *
 *  ⛔⛔ WHY THIS FILE EXISTS, AND IT IS NOT A HYPOTHETICAL. A 50-gold card
 *  SHIPPED THAT CANNOT SPAWN. `Content/Blueprints/Units/BP_Unit_Witch.uasset`
 *  does not exist and was never tracked in any commit, while her `DT_Cards` row,
 *  her mesh, her materials and her card art all shipped in `1aa0fee`. In play the
 *  ghost draws GREEN, she vanishes on click, NO GOLD IS DEDUCTED, and the log
 *  says "Card actor unavailable" — because `ResolveCardActorClass` is CORRECT and
 *  refuses the play. ⭐ THE GRACEFUL DEGRADE IS EXACTLY WHAT HID IT (`SC-§50`
 *  cl. 2): the code path looks low-severity while the player-visible outcome is a
 *  card that can never be played. Jonathan found it by clicking the card.
 *
 *  ⛔⛔ AND THE PART THAT MAKES THIS A **GATE** AND NOT A FIX: the gap was
 *  DECLARED IN WRITING BEFORE IT SHIPPED. `.claude/pipeline/qa/TASK-849.md` `N-5`,
 *  verbatim: "`BP_Unit_Witch` and `T_CardArt_Witch` do not exist yet (`TASK-835` /
 *  `TASK-834`; the card face degrades to text-only, never a crash)."
 *  `T_CardArt_Witch` was subsequently authored. `BP_Unit_Witch` was not, NO BOARD
 *  ROW CARRIED IT, and the lane shipped anyway. ⇒ `SC-§50` cl. 4: PROSE CANNOT
 *  CLOSE THIS CLASS; ONLY A MECHANISM CAN. This is the mechanism.
 *
 *  ── ⭐⭐ THE PREDICATE, QUOTED VERBATIM FROM `TASK-947` (`SC-§49` cl. 4a — the
 *     paraphrase IS the defect, so the spec's own words are pinned here) ───────
 *
 *      "WALK EVERY `DT_Cards` ROW OF A SPAWNABLE `CardType` (Unit · Economy ·
 *       Building) AND ASSERT ITS COMPOSED CONVENTIONS PATH RESOLVES."
 *
 *  ── ⛔⛔⭐ SCOPE — READ THIS BEFORE TRUSTING A GREEN BAR (`SC-§49`) ──────────
 *
 *  ⭐ THIS GATE PINS THE **ASSET SIDE** AND ONLY THE ASSET SIDE: *"the asset the
 *  law names EXISTS at the path the law names."*
 *
 *  ⛔ IT DOES **NOT** DETECT A **COMPOSER DRIFT.** If the shipped
 *  `ASiegePlayerController::ResolveCardActorClass` changes HOW it builds the
 *  string — a renamed folder, a dropped `BP_Unit_` prefix, a lost `_C` — this
 *  file keeps composing the CONVENTIONS path independently, keeps finding the
 *  assets, and STAYS GREEN while every card in the game fails to spawn.
 *
 *  ⚖️ THE MITIGATING MEASUREMENT, and it is why that residual is acceptable
 *  rather than merely admitted: A COMPOSER DRIFT BREAKS **EVERY** CARD AT ONCE,
 *  which is instantly visible to the first person who plays anything. ONE ABSENT
 *  ASSET breaks exactly ONE card, is invisible to 21 working ones, and survived a
 *  QA declaration, a commit and a playtest — that is the SILENT case, and it is
 *  the case this file closes. ⛔ The durable repair for the composer half is to
 *  extract a shared path-composer that both the controller and this test consume;
 *  it was CONSIDERED AND REFUSED at boarding (it would put this row inside
 *  `TASK-942`'s file and serialise two independent lanes), and it is carried in
 *  `handoffs/TASK-947-programmer.md` as a proposed manager row. ⛔ Do not
 *  self-serve it here.
 *
 *  ── ⭐⭐ EVERYTHING IS DERIVED. NOTHING IS TRANSCRIBED. ──────────────────────
 *
 *  ⛔ A HAND-WRITTEN LIST OF 22 CARD NAMES WOULD BUY ONE RELEASE OF CORRECTNESS
 *  AND RE-ARM THE IDENTICAL TRAP FOR CARD #23 — which is precisely the mistake
 *  `TASK-874` was boarded to UNDO (it DELETED a hardcoded 13-name roster rather
 *  than bumping it to 14). ⇒ every input below is read at run time from the same
 *  source the shipped spawn path reads:
 *
 *   (a) THE ROSTER comes from the DataTable the CDO itself points at —
 *       `ASiegePlayerController::CardTableAsset`, read off the CDO BY REFLECTION
 *       because the property is `protected`, then loaded and walked by
 *       `GetRowNames()`. ⭐ This is the SAME UDataTable object
 *       `ASiegePlayerController::ResolveCardRow` loads, so a row that exists in
 *       the asset but not in `Docs/Data/cards.csv` is STILL WALKED. (The
 *       neighbouring `SiegeAssistantSelectionTest` reads the CSV and declares
 *       that reimport gap as a residual; this file does not inherit it.)
 *   (b) THE SPAWNABLE/EXCLUDED SPLIT is a switch over `ECardType` with EVERY
 *       enumerator named, so adding a seventh card type is a COMPILE-TIME
 *       `-Wswitch` failure here, and an out-of-range byte deserialized from the
 *       asset lands in `Unclassified` and goes RED at run time. ⛔ There is no
 *       silent third outcome.
 *   (c) THE ECONOMY-BUILDING EXCEPTION (Deep Mine: `CardType` Economy, actor
 *       `ABuilding`) comes from the shipped controller's own
 *       `BuildingEconomyCardIDs` default, read off the CDO by reflection.
 *       ⛔ Hard-typing `DeepMine` here would put a SECOND COPY of that list in
 *       the tree, free to drift from the first.
 *   (d) THE REQUIRED BASE CLASSES are `ABuilding::StaticClass()` and
 *       `ASummonedUnit::StaticClass()` — the SYMBOLS (`SC-§38`), never a name.
 *
 *  ⚠️ EVERY REFLECTION LOOKUP CHECKS THE **TYPE**, NOT JUST THE NAME, and a miss
 *  is a HARD ERROR RETURN rather than a skip. A renamed or retyped property that
 *  merely made this file walk zero rows would produce THE SAME GREEN BAR as a
 *  clean roster — the exact vacuous-pass defect this gate exists to prevent.
 *
 *  ── ⭐ `SHIP-§9` — THE CONTROLS, BECAUSE A GATE ONLY EVER SEEN PASSING IS
 *     INDISTINGUISHABLE FROM NO GATE ────────────────────────────────────────
 *
 *   POSITIVE CONTROL: `Sorcerer` — a known-present row whose
 *       `/Game/Blueprints/Units/BP_Unit_Sorcerer` MUST be walked AND MUST
 *       resolve. Without it, "no missing assets" and "the walk never ran" are
 *       the same answer (`SC-§39`).
 *   NEGATIVE CONTROL: `ZzNoSuchCardZz` — asserted NOT to be a table row and its
 *       composed path asserted NOT to resolve. ⭐ THIS RUNS ON EVERY GREEN PASS,
 *       so every green carries a live demonstration that the instrument CAN
 *       still return ABSENT. If the existence probe ever silently starts
 *       answering "present" for everything, THIS assertion goes red.
 *       ⛔ `SC-§40` cl. 10 — THE SYNTHETIC VALUE WAS MEASURED ABSENT, NOT
 *       ASSUMED. Measured 2026-09-03 over the tree at `950d8c5`:
 *       `ZzNoSuchCardZz` returns 0 hits in `Docs/Data/cards.csv`, 0 in
 *       `Content/Data/DT_Cards.uasset` (binary), 0 `Content/Blueprints/**`
 *       filenames and 0 files under `Source/`; the positive control `Sorcerer`
 *       run through the same four instruments returns 1 / 3 / 1 / 27.
 *
 *  ── ⛔⛔ THIS TEST IS **RED TODAY**, AND THAT IS THE PROOF, NOT A DEFECT ────
 *
 *  `BP_Unit_Witch` is absent as this file is written (`TASK-946` is authoring it
 *  in parallel). The row for `Witch` MUST fail, by name, until that asset lands.
 *  ⛔⛔ DO NOT "FIX" THIS BY WEAKENING THE ASSERTION, SKIPPING THE ROW, OR
 *  ALLOWLISTING A CARD ID. The only correct repair is the missing asset.
 *  ⭐ `TASK-949` owns the RED → GREEN transition and it is the machine-checkable
 *  proof that the authored Blueprint actually resolves at the composed path.
 *
 *  ── MECHANISM — read-only; ⛔ zero writes, ⛔ no world, ⛔ no PIE, ⛔ no spawns
 *
 *  `FPackageName::DoesPackageExist` on the composed PACKAGE name answers the
 *  asset-side predicate WITHOUT attempting a load, so a known-absent asset costs
 *  no failed-load log noise and cannot itself perturb the run. Only when the
 *  package exists does the file go on to LOAD the `_C` generated class and assert
 *  it derives the required base — that second assertion covers the third failure
 *  mode of the shipped resolver (a class that loads but is not an `ABuilding` /
 *  `ASummonedUnit`), and by construction it never runs against a missing package.
 */

namespace SiegeCardRosterTestFixture
{
	/**
	 *  ⭐ THE POSITIVE CONTROL (`SC-§39`). A known-present spawnable row. If this
	 *  card is ever retired from the game the control goes red and a new one must
	 *  be chosen — that cost is deliberate and is what a control is for.
	 */
	static const TCHAR* const PositiveControlCardID = TEXT("Sorcerer");

	/**
	 *  ⭐ THE NEGATIVE CONTROL (`SC-§39`, `SC-§40` cl. 10). MEASURED absent from
	 *  `cards.csv`, `DT_Cards.uasset`, `Content/Blueprints/**` and `Source/**` —
	 *  see the file header for the four counts and their positive control.
	 */
	static const TCHAR* const NegativeControlCardID = TEXT("ZzNoSuchCardZz");

	/** Which ACTOR a card row spawns — the shipped `IsBuildingCard` + `ResolveCardActorClass` split. */
	enum class ESpawnCategory : uint8
	{
		BuildingActor,  // /Game/Blueprints/Buildings/BP_Building_<CardID> — ABuilding
		UnitActor,      // /Game/Blueprints/Units/BP_Unit_<CardID>         — ASummonedUnit
		NotSpawnable,   // Spell / HeroUpgrade / Utility — never enters placement mode
		Unclassified    // ⛔ a byte the enum does not name, or a type nobody classified
	};

	/** A composed CONVENTIONS path plus the base class the shipped resolver requires of it. */
	struct FComposedActorClassPath
	{
		FString PackageName;
		FString ClassPath;
		const UClass* RequiredBase = nullptr;
	};

	/**
	 *  ⚠️ THE LOOKUP CHECKS THE POINTED-TO TYPE, NOT JUST THE FIELD NAME. A
	 *  property renamed OR retyped must be reported BY NAME, because the
	 *  alternative is a roster that stays empty and a gate that passes because
	 *  there was nothing to check.
	 */
	static const FSoftObjectPtr* FindSoftObjectField(const UObject* Object, const TCHAR* FieldName, const UClass* ExpectedPointeeClass)
	{
		const FSoftObjectProperty* const SoftProperty = FindFProperty<FSoftObjectProperty>(Object->GetClass(), FieldName);
		// `.Get()` rather than comparing the TObjectPtr directly — the raw-pointer
		// comparison is well-defined across every build configuration.
		if (!SoftProperty || SoftProperty->PropertyClass.Get() != ExpectedPointeeClass)
		{
			return nullptr;
		}
		return SoftProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Object);
	}

	/** Same discipline for the `TArray<FName>` exception list — the INNER property's type is checked too. */
	static const TArray<FName>* FindNameArrayField(const UObject* Object, const TCHAR* FieldName)
	{
		const FArrayProperty* const ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), FieldName);
		if (!ArrayProperty || !ArrayProperty->Inner || !ArrayProperty->Inner->IsA<FNameProperty>())
		{
			return nullptr;
		}
		return ArrayProperty->ContainerPtrToValuePtr<TArray<FName>>(Object);
	}

	/**
	 *  The shipped split, mirrored from `ASiegePlayerController::IsBuildingCard` +
	 *  `ResolveCardActorClass`.
	 *
	 *  ⛔ NOTE THE ABSENT `default:` LABEL — that is deliberate, not an omission.
	 *  Every enumerator is named, so ADDING a seventh `ECardType` fails to COMPILE
	 *  here (`-Wswitch`) instead of being silently swallowed as non-spawnable; and
	 *  a value outside the enum entirely (a byte deserialized from the asset)
	 *  falls straight through to the `Unclassified` initializer and goes red at
	 *  run time. Both directions are covered and neither is silent.
	 */
	static ESpawnCategory ClassifyRow(FName CardID, ECardType CardType, const TArray<FName>& BuildingEconomyCardIDs)
	{
		ESpawnCategory Category = ESpawnCategory::Unclassified;

		switch (CardType)
		{
		case ECardType::Building:
			Category = ESpawnCategory::BuildingActor;
			break;

		case ECardType::Economy:
			// Deep Mine's row is Economy for the GDD §8 raidable-economy semantics,
			// but `ADeepMine` derives `ABuilding` and lives under /Blueprints/Buildings.
			Category = BuildingEconomyCardIDs.Contains(CardID) ? ESpawnCategory::BuildingActor : ESpawnCategory::UnitActor;
			break;

		case ECardType::Unit:
			Category = ESpawnCategory::UnitActor;
			break;

		case ECardType::Spell:
		case ECardType::HeroUpgrade:
		case ECardType::Utility:
			Category = ESpawnCategory::NotSpawnable;
			break;
		}

		return Category;
	}

	/**
	 *  The CONVENTIONS composition, character-for-character as
	 *  `ResolveCardActorClass` builds it:
	 *      /Game/Blueprints/Buildings/BP_Building_<CardID>.BP_Building_<CardID>_C
	 *      /Game/Blueprints/Units/BP_Unit_<CardID>.BP_Unit_<CardID>_C
	 *  ⛔ See the file header: this is an INDEPENDENT copy of the contract, which
	 *  is exactly why this gate cannot detect a drift in the shipped composer.
	 */
	static FComposedActorClassPath ComposeActorClassPath(FName CardID, ESpawnCategory Category)
	{
		const FString CardName = CardID.ToString();
		FComposedActorClassPath Out;

		if (Category == ESpawnCategory::BuildingActor)
		{
			Out.PackageName  = FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s"), *CardName);
			Out.ClassPath    = FString::Printf(TEXT("/Game/Blueprints/Buildings/BP_Building_%s.BP_Building_%s_C"), *CardName, *CardName);
			Out.RequiredBase = ABuilding::StaticClass();
		}
		else
		{
			Out.PackageName  = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s"), *CardName);
			Out.ClassPath    = FString::Printf(TEXT("/Game/Blueprints/Units/BP_Unit_%s.BP_Unit_%s_C"), *CardName, *CardName);
			Out.RequiredBase = ASummonedUnit::StaticClass();
		}

		return Out;
	}

	/** The `ECardType` entry NAME (the symbol, `SC-§38`) for reporting — never a raw integer in a message a human reads. */
	static FString CardTypeSymbol(ECardType CardType)
	{
		const UEnum* const CardTypeEnum = StaticEnum<ECardType>();
		if (!CardTypeEnum)
		{
			return FString::Printf(TEXT("<ECardType reflection unavailable; raw %d>"), static_cast<int32>(CardType));
		}

		FString Symbol = CardTypeEnum->GetNameStringByValue(static_cast<int64>(CardType));
		if (Symbol.IsEmpty())
		{
			Symbol = FString::Printf(TEXT("<not an ECardType enumerator; raw %d>"), static_cast<int32>(CardType));
		}
		return Symbol;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardRosterSpawnableActorClassPathTest,
	"Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardRosterSpawnableActorClassPathTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardRosterTestFixture;

	// ══ SELF-CHECKS ═════════════════════════════════════════════════════════
	// ⛔ EVERY ONE OF THESE RETURNS FALSE RATHER THAN CONTINUING. A gate that
	//    cannot reach its subject must FAIL, never report SAFE — an unreadable
	//    instrument and a clean roster otherwise produce the identical green bar.

	const UObject* const ControllerDefaults = ASiegePlayerController::StaticClass()->GetDefaultObject();
	if (!ControllerDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController's CDO is null — the roster, the card table and the Economy exception all derive from it, so nothing below would mean anything."));
		return false;
	}

	// ── (a) THE CARD TABLE, OFF THE SHIPPED CDO ──────────────────────────────
	const FSoftObjectPtr* const CardTableValue =
		FindSoftObjectField(ControllerDefaults, TEXT("CardTableAsset"), UDataTable::StaticClass());
	if (!CardTableValue)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::CardTableAsset (TSoftObjectPtr<UDataTable>) was not reachable on the CDO — it was RENAMED or RETYPED. The roster cannot be derived, and a gate that walked zero rows would pass VACUOUSLY."));
		return false;
	}

	const FSoftObjectPath CardTablePath = CardTableValue->ToSoftObjectPath();
	if (CardTablePath.IsNull())
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::CardTableAsset is UNSET on the CDO — the shipped constructor sets it to /Game/Data/DT_Cards, so an empty path means the assignment was removed."));
		return false;
	}

	const UDataTable* const CardTable = TSoftObjectPtr<UDataTable>(CardTablePath).LoadSynchronous();
	if (!CardTable)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: the card table '%s' — the exact asset the shipped ResolveCardRow loads — did not load as a UDataTable."), *CardTablePath.ToString()));
		return false;
	}

	if (CardTable->GetRowStruct() != FCardRow::StaticStruct())
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: card table '%s' has row struct '%s', not FCardRow — every FindRow<FCardRow> below would return null and the walk would silently check nothing."),
			*CardTablePath.ToString(), *GetNameSafe(CardTable->GetRowStruct())));
		return false;
	}

	AddInfo(FString::Printf(TEXT("card table derived from the shipped CDO (never typed here): '%s', row struct FCardRow."), *CardTablePath.ToString()));

	// ── (b) THE ECONOMY-BUILDING EXCEPTION, OFF THE SHIPPED CDO ──────────────
	const TArray<FName>* const BuildingEconomyCardIDs =
		FindNameArrayField(ControllerDefaults, TEXT("BuildingEconomyCardIDs"));
	if (!BuildingEconomyCardIDs)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::BuildingEconomyCardIDs (TArray<FName>) was not reachable on the CDO — it was RENAMED or RETYPED, and the Economy-vs-building split cannot be derived. Every Economy card would then be routed down the UNIT path and this gate would report a defect that does not exist."));
		return false;
	}
	AddInfo(FString::Printf(TEXT("Economy-building exception derived from the shipped CDO: %d CardID(s)."), BuildingEconomyCardIDs->Num()));

	// ══ THE WALK ════════════════════════════════════════════════════════════
	const TArray<FName> RowNames = CardTable->GetRowNames();

	int32 TotalRows       = 0;   // rows the walk actually READ (not what GetRowNames promised)
	int32 SpawnableRows   = 0;   // Unit | Economy | Building
	int32 ExcludedRows    = 0;   // Spell | HeroUpgrade | Utility
	int32 ProbesExecuted  = 0;   // existence probes actually RUN — must equal SpawnableRows
	int32 UnresolvedRows  = 0;

	TMap<FString, int32> RowsByTypeSymbol;
	TMap<FString, int32> ExcludedByTypeSymbol;
	TSet<FString> DistinctComposedPackages;
	TArray<FString> UnresolvedCardIDs;

	bool bPositiveControlWalked   = false;
	bool bPositiveControlResolved = false;
	bool bNegativeControlIsARow   = false;

	for (const FName& RowName : RowNames)
	{
		const FCardRow* const Row = CardTable->FindRow<FCardRow>(
			RowName, TEXT("Siegebound.CardRoster.EverySpawnableCardRowResolvesItsComposedActorClassPath"), /*bWarnIfRowMissing=*/ false);

		if (!Row)
		{
			AddError(FString::Printf(TEXT("card table row '%s' is named by GetRowNames() but FindRow<FCardRow> returned null — the row exists and cannot be read, which is NOT the same as a row that is absent."), *RowName.ToString()));
			continue;
		}

		++TotalRows;

		const FString TypeSymbol = CardTypeSymbol(Row->CardType);
		RowsByTypeSymbol.FindOrAdd(TypeSymbol)++;

		if (RowName.ToString().Equals(NegativeControlCardID, ESearchCase::IgnoreCase))
		{
			bNegativeControlIsARow = true;
		}

		const ESpawnCategory Category = ClassifyRow(RowName, Row->CardType, *BuildingEconomyCardIDs);

		if (Category == ESpawnCategory::Unclassified)
		{
			// A byte the enum does not name, or a new ECardType nobody routed.
			// ⛔ Counted as NEITHER spawnable nor excluded, so the partition
			//    assertion below ALSO goes red — one defect, two independent tells.
			AddError(FString::Printf(TEXT("card row '%s' carries CardType %s, which ClassifyRow does not route. A card type nobody classified is a card that may or may not need an actor Blueprint — decide it here rather than letting it fall silently out of this gate."),
				*RowName.ToString(), *TypeSymbol));
			continue;
		}

		if (Category == ESpawnCategory::NotSpawnable)
		{
			++ExcludedRows;
			ExcludedByTypeSymbol.FindOrAdd(TypeSymbol)++;
			continue;
		}

		++SpawnableRows;

		const FComposedActorClassPath Composed = ComposeActorClassPath(RowName, Category);
		DistinctComposedPackages.Add(Composed.PackageName);

		// ── THE ASSERTION THIS WHOLE FILE EXISTS FOR ─────────────────────────
		// Package existence, NOT a load: it answers the asset-side predicate
		// exactly, and it costs no failed-load log noise on the rows that are
		// missing (which, today, is the entire point of the run).
		++ProbesExecuted;
		const bool bPackageExists = FPackageName::DoesPackageExist(Composed.PackageName);

		TestTrue(FString::Printf(TEXT("card '%s' (%s) — its composed actor Blueprint EXISTS at '%s'"),
			*RowName.ToString(), *TypeSymbol, *Composed.PackageName), bPackageExists);

		if (!bPackageExists)
		{
			++UnresolvedRows;
			UnresolvedCardIDs.Add(RowName.ToString());
			AddError(FString::Printf(TEXT("⛔ SPAWNABLE CARD WITH NO ACTOR BLUEPRINT: '%s' (CardType %s) is a playable, drawable card whose composed class path '%s' does not exist. ResolveCardActorClass will return null, the play will be refused with 'Card actor unavailable' and NO gold spent — a card the player can hold and can never play. ⛔ The repair is the missing asset, NEVER a weakened assertion here (CONVENTIONS SC-§50 cl. 4)."),
				*RowName.ToString(), *TypeSymbol, *Composed.ClassPath));
			continue;
		}

		// ── The package is present, so the generated class can be loaded without
		//    ever attempting a known-absent load. This covers the shipped
		//    resolver's THIRD null path: a class that loads but is the wrong kind.
		const UClass* const ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(Composed.ClassPath)).LoadSynchronous();

		if (!TestNotNull(FString::Printf(TEXT("card '%s' — '%s' resolves to a generated CLASS (an asset present but a `_C` that will not load is non-null in the editor and null at play time)"),
			*RowName.ToString(), *Composed.ClassPath), ActorClass))
		{
			++UnresolvedRows;
			UnresolvedCardIDs.Add(RowName.ToString());
			continue;
		}

		TestTrue(FString::Printf(TEXT("card '%s' — its actor class derives %s, the base ResolveCardActorClass requires before it will spawn"),
			*RowName.ToString(), *GetNameSafe(Composed.RequiredBase)), ActorClass->IsChildOf(Composed.RequiredBase));

		if (RowName.ToString().Equals(PositiveControlCardID, ESearchCase::IgnoreCase))
		{
			bPositiveControlWalked   = true;
			bPositiveControlResolved = true;
		}
	}

	// Walked but unresolved still counts as WALKED for the control.
	if (!bPositiveControlWalked)
	{
		for (const FString& Unresolved : UnresolvedCardIDs)
		{
			if (Unresolved.Equals(PositiveControlCardID, ESearchCase::IgnoreCase))
			{
				bPositiveControlWalked = true;
			}
		}
	}

	// ══ THE COUNTS, ASSERTED — ⛔ NOT MERELY LOGGED ═════════════════════════
	//
	// ⛔⛔ THE DEFECT THESE EXIST TO PREVENT: A WALK OVER ZERO ROWS PASSES
	//    VACUOUSLY AND REPORTS THE IDENTICAL GREEN AS ONE THAT CHECKED THE WHOLE
	//    ROSTER. Every assertion below is therefore about the count itself.
	//
	// ⭐ AND THEY PIN **RELATIONSHIPS, NOT LITERALS** (`SC-§40` cl. 10). A
	//    `TestEqual(SpawnableRows, 22)` would be the TASK-874 trap rebuilt inside
	//    the very gate written to close it: card #23 would turn this file red for
	//    the wrong reason, and the obvious "fix" would be to bump the number.
	//    These four hold no matter how the roster grows, and every one of them
	//    still goes red on an empty or truncated walk.

	TestTrue(TEXT("COUNT — the card table yielded rows to walk (a zero-row walk passes vacuously and is indistinguishable from a clean roster)"),
		TotalRows > 0);

	TestTrue(TEXT("COUNT — at least one SPAWNABLE row was walked (if the type filter matched nothing, every assertion above was skipped and this gate checked NOTHING)"),
		SpawnableRows > 0);

	TestEqual(TEXT("COUNT — the partition is TOTAL: spawnable + excluded == rows read (a row that is neither was silently dropped from the gate)"),
		SpawnableRows + ExcludedRows, TotalRows);

	// ⚠️ HONEST SCOPE (`SC-§49`): as the loop is written TODAY these two counters
	//    cannot diverge, so this assertion discriminates NOTHING on this diff. It
	//    is a TRIPWIRE FOR THE NEXT EDIT — the cheap-looking `continue` between
	//    "count it as spawnable" and "probe it" is exactly how a roster gate
	//    quietly stops probing rows while still reporting them.
	TestEqual(TEXT("COUNT — one existence probe was EXECUTED per spawnable row (a probe count below the row count means rows were skipped, not passed)"),
		ProbesExecuted, SpawnableRows);

	TestEqual(TEXT("COUNT — every spawnable row composed a DISTINCT path (two rows composing one path means one card is silently borrowing another's actor)"),
		DistinctComposedPackages.Num(), SpawnableRows);

	// ══ THE CONTROLS (`SC-§39`, `SHIP-§9`) ══════════════════════════════════
	TestTrue(FString::Printf(TEXT("POSITIVE CONTROL — the known-present row '%s' was reached by the walk (proves the walk ran over real data)"), PositiveControlCardID),
		bPositiveControlWalked);

	TestTrue(FString::Printf(TEXT("POSITIVE CONTROL — '%s' RESOLVED (proves the existence probe can return PRESENT; without it, 'nothing is missing' and 'the probe is blind' are the same answer)"), PositiveControlCardID),
		bPositiveControlResolved);

	TestFalse(FString::Printf(TEXT("NEGATIVE CONTROL — '%s' is not a real card row (if it ever becomes one, this control is dead and must be re-chosen)"), NegativeControlCardID),
		bNegativeControlIsARow);

	const FComposedActorClassPath BogusComposed = ComposeActorClassPath(FName(NegativeControlCardID), ESpawnCategory::UnitActor);
	TestFalse(FString::Printf(TEXT("⭐ NEGATIVE CONTROL — the composed path for a card that does not exist ('%s') does NOT resolve. THIS IS THE ASSERTION THAT KEEPS EVERY GREEN ABOVE HONEST: it demonstrates, on every single run, that the existence probe is still capable of returning ABSENT"), *BogusComposed.PackageName),
		FPackageName::DoesPackageExist(BogusComposed.PackageName));

	// ══ THE REPORT ══════════════════════════════════════════════════════════
	// Published under the SAME predicate it was measured under (`SC-§49`): the
	// exclusions are NAMED with their counts, never summarised as "the rest".
	AddInfo(FString::Printf(TEXT("ROSTER — %d row(s) read; %d SPAWNABLE (Unit | Economy | Building) probed; %d EXCLUDED (not a spawnable CardType)."),
		TotalRows, SpawnableRows, ExcludedRows));

	for (const TPair<FString, int32>& Entry : RowsByTypeSymbol)
	{
		AddInfo(FString::Printf(TEXT("  CardType %-12s %3d row(s)%s"), *Entry.Key, Entry.Value,
			ExcludedByTypeSymbol.Contains(Entry.Key) ? TEXT("   [EXCLUDED — not a spawnable CardType]") : TEXT("")));
	}

	if (UnresolvedRows > 0)
	{
		AddInfo(FString::Printf(TEXT("⛔ UNRESOLVED: %d of %d spawnable card(s) have NO actor Blueprint at their composed CONVENTIONS path: %s"),
			UnresolvedRows, SpawnableRows, *FString::Join(UnresolvedCardIDs, TEXT(", "))));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
