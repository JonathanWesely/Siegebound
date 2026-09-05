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
 *       enumerator named and NO `default:` label, so a seventh card type cannot
 *       be silently swallowed. ⛔ WHICH HALF OF THAT GUARD IS LOAD-BEARING IS
 *       TOOLCHAIN-DEPENDENT, and this project sits on the pessimistic side:
 *       under Clang `-Wswitch` makes a 7th enumerator a COMPILE error, but AS
 *       MEASURED 2026-09-03 AT `09b9b50` this project's `Build.cs` sets NO
 *       warning configuration (zero hits for `CppCompileWarnings` /
 *       `bWarningsAsErrors` / `4062` anywhere under `Source/` — `qa/TASK-948.md`
 *       `W-2`) and `TASK-949` built it on MSVC 14.50 ⇒ ON THIS TOOLCHAIN THE
 *       COMPILE-TIME HALF MAY NOT FIRE AT ALL. ⭐ THE RUN-TIME TELL IS THE
 *       LOAD-BEARING HALF HERE, and it is UNAFFECTED: an enumerator nobody
 *       routed, and equally an out-of-range byte deserialized from the asset,
 *       BOTH fall to the `Unclassified` initializer, are reported BY NAME, and
 *       take the partition assertion red as a second independent tell. ⛔ There
 *       is no silent third outcome — but do not credit the compiler for that
 *       (`SC-§51` cl. 5).
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
 *  ── ⛔⛔ THIS TEST WAS **RED WHEN IT WAS WRITTEN**, AND THAT WAS THE PROOF ──
 *
 *  ⛔ PAST TENSE ON PURPOSE (`SC-§53` cl. 3 — a dated measurement cannot rot; a
 *  present-tense one starts lying the moment the repair lands in another commit).
 *  `BP_Unit_Witch` was ABSENT when this file was authored (`TASK-947`), so the
 *  `Witch` row failed BY NAME. `TASK-946` authored the asset and `TASK-949`
 *  committed both at `09b9b50` (2026-09-03), where this gate compiled and went
 *  GREEN over `32 row(s) read; 22 SPAWNABLE probed; 10 EXCLUDED`.
 *  ⇒ ⛔ THE GATE IS GREEN TODAY AND MUST STAY THAT WAY. ⛔⛔ IF IT GOES RED
 *  AGAIN, DO NOT "FIX" IT BY WEAKENING THE ASSERTION, SKIPPING THE ROW OR
 *  ALLOWLISTING A CARD ID — the only correct repair is the missing asset.
 *
 *  ── ⛔ THE HALF `09b9b50` COULD NOT BUY, RECORDED SO IT IS NOT RE-LOST ──────
 *
 *  The RED → GREEN transition was observed only on `TASK-947`'s SHELL MIRROR — a
 *  different program, in a different language, reading `cards.csv` rather than
 *  `DT_Cards.uasset`. ⛔ THE COMPILED GATE HAD BEEN SEEN ONLY PASSING.
 *  Synthesising a compiled red by moving the staged `BP_Unit_Witch.uasset` aside
 *  was REFUSED by the permission system and — correctly — NOT worked around
 *  (⭐ `SC-§54` cl. 1: a refusal that is routed around is worse than no permission
 *  system at all, because the next reader cannot tell which of the two happened).
 *  ⇒ `TASK-964` answered it with a METHOD CHANGE rather than a retry: the SIBLING
 *  TEST AT THE FOOT OF THIS FILE,
 *  `Siegebound.CardRoster.AnAbsentCardIDFailsToResolveItsComposedActorClassPath`,
 *  a PERMANENT negative control driven by a MEASURED-ABSENT SYNTHETIC CardID that
 *  touches no asset at all. It drives the SAME `ClassifyRow` →
 *  `ComposeActorClassPath` → `FPackageName::DoesPackageExist` chain this walk
 *  uses and INVERTS the assertion, so it is GREEN precisely BECAUSE the
 *  resolution fails. ⛔ Do not "simplify" the two into one.
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
	 *  ⛔⛔ NOTE THE ABSENT `default:` LABEL — deliberate, not an omission, and
	 *  `qa/TASK-948.md` §(c) RULED that it must be PRESERVED. Adding
	 *  `default: Category = NotSpawnable;` would SILENTLY SWALLOW every unrouted
	 *  card type into the excluded bucket: the partition would still balance, the
	 *  walk would still look healthy, and a whole card family would be invisible
	 *  to the gate written to find exactly that. ⛔ BOTH halves of the guard would
	 *  be disarmed by one "cleanup". ⛔ DO NOT ADD ONE (`SC-§51` cl. 5).
	 *
	 *  ⚠️ WHAT THE ABSENT LABEL BUYS, STATED PER TOOLCHAIN RATHER THAN
	 *  ABSOLUTELY. Under Clang, `-Wswitch` makes a seventh `ECardType` a COMPILE
	 *  failure here. ⛔ AS MEASURED 2026-09-03 AT `09b9b50` this project sets no
	 *  warning configuration and `TASK-949` built on MSVC 14.50, so on the
	 *  standing Win64 toolchain that compile-time half MAY NOT FIRE. ⭐ THE
	 *  RUN-TIME HALF IS WHAT ACTUALLY PROTECTS THIS SWITCH and it needs no
	 *  toolchain support: an enumerator nobody routed — and equally a byte
	 *  outside the enum entirely, deserialized from the asset — falls straight
	 *  through to the `Unclassified` initializer, is reported BY NAME, and takes
	 *  the partition assertion red as a second independent tell.
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

/**
 *  ═══ THE PERMANENT NEGATIVE CONTROL (TASK-964; law: CONVENTIONS `SC-§54`,
 *      `SHIP-§9`/`§9c` cl. 1, `SC-§51` cl. 3 + cl. 6, `SC-§39`, `SC-§38`,
 *      `SC-§40` cl. 10, `SC-§49`) ═══
 *
 *  ⛔⛔ WHY THIS TEST EXISTS: THE GATE ABOVE HAD NEVER BEEN SEEN **RED** ON A
 *  COMPILED RUN. `TASK-949` compiled it and ran it and it PASSED — but a gate
 *  only ever seen passing is `SHIP-§9c` cl. 1's *"a status line wearing a gate's
 *  clothes"*. The obvious way to buy a red was to move the staged
 *  `BP_Unit_Witch.uasset` aside; ⛔ THE PERMISSION SYSTEM REFUSED, and `TASK-949`
 *  correctly DID NOT WORK AROUND IT. ⭐ `SC-§54` cl. 3 supplies the answer, and it
 *  is a **METHOD**, not a retry with a new task ID: a negative control is
 *  synthesised against a MEASURED-ABSENT SYNTHETIC INPUT — ⛔ NEVER by moving,
 *  renaming, deleting or editing a REAL deliverable. ⛔ This test touches no
 *  asset, and it must never be rewritten into one that does.
 *
 *  ⭐ AND IT IS **PERMANENT**, WHICH IS THE WHOLE POINT (`SC-§54` cl. 3(c)): a
 *  disturbed asset buys ONE transcript that decays into a screenshot; a synthetic
 *  absent input buys an assertion that RE-PROVES, ON EVERY SINGLE RUN, that the
 *  probe is still capable of answering NO.
 *
 *  ── ⛔⛔ THE TRAP THIS TEST IS WRITTEN AROUND — READ BEFORE EDITING ─────────
 *
 *  The sibling walk reports a missing actor Blueprint BY FAILING. This test
 *  EXPECTS that condition, so it INVERTS THE ASSERTION rather than re-running the
 *  walk. ⛔ IT MUST NEVER `AddError` ON THE EXPECTED ABSENCE — every `AddError`
 *  below is a SELF-CHECK failure, i.e. the instrument could not reach its
 *  subject, which is a real failure. ⛔ A control that turns the suite RED is a
 *  BUG wearing a control's clothes and it would block the commit host.
 *  ⭐ THIS TEST IS GREEN, AND IT IS GREEN **BECAUSE** THE RESOLUTION FAILS.
 *
 *  ── ⛔ THE SAME SYMBOLS, NOT A COPY OF THEM (`SC-§38`) ──────────────────────
 *
 *  A hand-built path string asserted not to resolve would prove NOTHING about the
 *  gate — it would be a test of its own scratch work. This test therefore calls
 *  the SAME symbols the walk calls, in the SAME order:
 *      `ClassifyRow` → `ComposeActorClassPath` → `FPackageName::DoesPackageExist`
 *  ⛔ It composes NO path of its own and contains NO `/Game/...` literal. The
 *  CONVENTIONS string lives in exactly ONE place in this file
 *  (`ComposeActorClassPath`); this test CONSUMES it and adds no second copy.
 *
 *  ── ⭐ WHY IT CANNOT PASS VACUOUSLY — TWO LEDGERS, KEPT APART (`SC-§51` cl. 4)
 *
 *  ⛔ (a) REACHABILITY — the synthetic row is asserted to CLASSIFY **SPAWNABLE**.
 *         This is the assertion easiest to omit and most expensive to omit: a
 *         CardID the walk would route `NotSpawnable` is never probed at all, so a
 *         "negative control" built on one would stand for a red THAT COULD NEVER
 *         HAPPEN. Reachability is not decoration; it is the premise.
 *  ⛔ (b) INSTRUMENT CONTROLS (`SC-§39`) — the negative half alone is worthless: a
 *         probe stuck at ABSENT satisfies it for entirely the wrong reason. So
 *         the POSITIVE half runs the IDENTICAL call on a measured-PRESENT card
 *         and requires PRESENT, and a DISCRIMINATION assertion requires the two
 *         answers to DIFFER. ⇒ a probe stuck at EITHER polarity turns this test
 *         red. ⭐⭐ THAT PAIR IS THE COMPILED, PERMANENT FORM OF *"PROVE YOUR
 *         CONTROL CAN FAIL"*: the positive control IS the inverted assertion,
 *         running live on every pass, rather than a one-off source edit whose
 *         transcript nobody can re-run. The same discipline is applied a second
 *         time to `FindRow` — a lookup that answered null for EVERYTHING would
 *         satisfy the liveness check below for the wrong reason too.
 *  ⛔ (c) LIVENESS — the synthetic CardID is re-measured against the CDO-derived
 *         table AT RUN TIME. If it ever becomes a real card, this control is DEAD
 *         and says so BY FAILING, rather than passing quietly forever.
 *
 *  ── ⛔ `SC-§40` cl. 10 — THE SYNTHETIC WAS **MEASURED** ABSENT, NOT ASSUMED ──
 *
 *  It REUSES the fixture's single `NegativeControlCardID` rather than minting a
 *  second constant free to drift from the first. ⚠️ A near-miss name such as
 *  `ZzNoSuchCard` was REJECTED precisely because it is a SUBSTRING of the
 *  existing constant, so its "absence" would have been unmeasurable by grep.
 *  Re-measured 2026-09-03 over the tree at `09b9b50`: `ZzNoSuchCardZz` returns
 *  0 hits in `Docs/Data/cards.csv`, 0 in `Content/Data/DT_Cards.uasset` (binary)
 *  and 0 `Content/Blueprints/**` filenames, while the positive control `Sorcerer`
 *  through those same three instruments returns 1 / 4 / 1. ⚠️ Under `Source/` the
 *  synthetic now reads 1 FILE — THIS one, which declares it — and that is its
 *  intended and only home. (The file-header's `950d8c5` reading of `0` under
 *  `Source/` was taken before this test file was tracked; it is dated, it is
 *  correct as written, and per `SC-§53` cl. 3 it must NOT be "corrected".)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeCardRosterAbsentCardIDNegativeControlTest,
	"Siegebound.CardRoster.AnAbsentCardIDFailsToResolveItsComposedActorClassPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeCardRosterAbsentCardIDNegativeControlTest::RunTest(const FString& Parameters)
{
	using namespace SiegeCardRosterTestFixture;

	// ══ SELF-CHECKS ═════════════════════════════════════════════════════════
	// ⛔ Same discipline as the sibling walk, and for the same reason: a control
	//    that cannot reach its subject must FAIL, never report SAFE. An
	//    unreachable CDO and a correctly-absent asset would otherwise produce the
	//    identical green bar.

	const UObject* const ControllerDefaults = ASiegePlayerController::StaticClass()->GetDefaultObject();
	if (!ControllerDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController's CDO is null — both the card table and the Economy exception derive from it, so neither the absence re-measurement nor the classification below would mean anything."));
		return false;
	}

	const FSoftObjectPtr* const CardTableValue =
		FindSoftObjectField(ControllerDefaults, TEXT("CardTableAsset"), UDataTable::StaticClass());
	if (!CardTableValue)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::CardTableAsset (TSoftObjectPtr<UDataTable>) was not reachable on the CDO — it was RENAMED or RETYPED. The synthetic CardID's absence cannot be re-measured against the shipped roster, and a negative control that cannot confirm its own input is absent proves NOTHING."));
		return false;
	}

	const FSoftObjectPath CardTablePath = CardTableValue->ToSoftObjectPath();
	const UDataTable* const CardTable = TSoftObjectPtr<UDataTable>(CardTablePath).LoadSynchronous();
	if (!CardTable)
	{
		AddError(FString::Printf(TEXT("⛔ SELF-CHECK FAILED: the card table '%s' — the exact asset the shipped ResolveCardRow loads — did not load as a UDataTable."), *CardTablePath.ToString()));
		return false;
	}

	const TArray<FName>* const BuildingEconomyCardIDs =
		FindNameArrayField(ControllerDefaults, TEXT("BuildingEconomyCardIDs"));
	if (!BuildingEconomyCardIDs)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: ASiegePlayerController::BuildingEconomyCardIDs (TArray<FName>) was not reachable on the CDO — ClassifyRow cannot be called with the SHIPPED exception list, and calling it with a hand-made one would make this a test of a COPY (`SC-§38`)."));
		return false;
	}

	const FName SyntheticCardID(NegativeControlCardID);
	const FName PositiveCardID(PositiveControlCardID);
	const TCHAR* const LookupContext = TEXT("Siegebound.CardRoster.AnAbsentCardIDFailsToResolveItsComposedActorClassPath");

	// ══ (c) LIVENESS — THE SYNTHETIC IS STILL ABSENT FROM THE SHIPPED ROSTER ══
	// ⛔ Re-measured at run time against the CDO-derived table, never assumed from
	//    the header's grep (`SC-§40` cl. 10). ⚠️ And `FindRow` gets its OWN
	//    positive control: a lookup answering null for EVERYTHING would satisfy
	//    the absence assertion for entirely the wrong reason (`SC-§39`).
	const FCardRow* const SyntheticRow =
		CardTable->FindRow<FCardRow>(SyntheticCardID, LookupContext, /*bWarnIfRowMissing=*/ false);
	const FCardRow* const PositiveRow =
		CardTable->FindRow<FCardRow>(PositiveCardID, LookupContext, /*bWarnIfRowMissing=*/ false);

	TestTrue(FString::Printf(TEXT("LIVENESS — the synthetic CardID '%s' is NOT a row in the shipped card table (if it ever becomes a real card this control is DEAD and must be RE-CHOSEN, never weakened)"), NegativeControlCardID),
		SyntheticRow == nullptr);

	if (!TestNotNull(FString::Printf(TEXT("⭐ ROW-PROBE POSITIVE CONTROL — the known-present row '%s' IS found by the same FindRow<FCardRow> call (without it, 'the synthetic is absent' and 'FindRow answers null for everything' are the same result)"), PositiveControlCardID),
		PositiveRow))
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: the row probe is blind, so the absence it just reported is not a measurement. Nothing below would be evidence."));
		return false;
	}

	// ══ (a) REACHABILITY — THE SYNTHETIC ROUTES *SPAWNABLE* ═════════════════
	// ⛔ THE PREMISE OF THE WHOLE TEST. The walk only probes rows ClassifyRow
	//    routes to an ACTOR category; a control routed NotSpawnable would stand
	//    for a red that could never occur. Same symbol, same CDO-derived list.
	const ESpawnCategory SyntheticCategory =
		ClassifyRow(SyntheticCardID, ECardType::Unit, *BuildingEconomyCardIDs);

	if (!TestTrue(FString::Printf(TEXT("REACHABILITY — ClassifyRow routes a synthetic CardType::Unit row ('%s') down the SPAWNABLE unit-actor path, i.e. the walk WOULD have probed it"), NegativeControlCardID),
		SyntheticCategory == ESpawnCategory::UnitActor))
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: the synthetic row is not routed to an actor path, so it cannot demonstrate the walk's failure mode. This is a defect in the control, not in the roster."));
		return false;
	}

	const ESpawnCategory PositiveCategory =
		ClassifyRow(PositiveCardID, PositiveRow->CardType, *BuildingEconomyCardIDs);

	if (!TestTrue(FString::Printf(TEXT("REACHABILITY — the positive control '%s' (CardType %s) also routes SPAWNABLE, so both halves of the probe below travel the same code path"),
			PositiveControlCardID, *CardTypeSymbol(PositiveRow->CardType)),
		PositiveCategory == ESpawnCategory::UnitActor || PositiveCategory == ESpawnCategory::BuildingActor))
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: the positive control is no longer a spawnable card. It must be re-chosen (see the fixture comment) — that cost is deliberate and is what a control is for."));
		return false;
	}

	// ══ COMPOSE — THE SHIPPED FIXTURE SYMBOL, NEVER A LITERAL ═══════════════
	const FComposedActorClassPath SyntheticComposed = ComposeActorClassPath(SyntheticCardID, SyntheticCategory);
	const FComposedActorClassPath PositiveComposed  = ComposeActorClassPath(PositiveCardID,  PositiveCategory);

	// ⛔ Asserted STRUCTURALLY, from the CardID — writing the expected string here
	//    would put a second copy of the CONVENTIONS contract in this file, which
	//    is the exact drift the header warns about.
	TestTrue(FString::Printf(TEXT("COMPOSE — the composed package name '%s' is DERIVED from the CardID (a composer that ignored its argument would probe one fixed path forever)"), *SyntheticComposed.PackageName),
		SyntheticComposed.PackageName.Contains(SyntheticCardID.ToString(), ESearchCase::CaseSensitive));

	TestTrue(FString::Printf(TEXT("COMPOSE — the synthetic unit card requires base %s, the class ResolveCardActorClass demands before it will spawn (the SYMBOL, never a name)"), *GetNameSafe(ASummonedUnit::StaticClass())),
		SyntheticComposed.RequiredBase == ASummonedUnit::StaticClass());

	TestTrue(TEXT("COMPOSE — the two controls composed DIFFERENT package names (identical strings would make the discrimination assertion below vacuous)"),
		SyntheticComposed.PackageName != PositiveComposed.PackageName);

	// ══ (b) THE ASSERTION THIS TEST EXISTS FOR — INVERTED, NOT RE-RUN ═══════
	//
	// ⛔⛔ `bAbsentResolves` IS THE **IDENTICAL EXPRESSION** the sibling walk
	//    asserts TRUE for every spawnable row. Here it is asserted FALSE.
	// ⭐⭐ THAT IS THE COMPILED OBSERVATION OF THE RED-PRODUCING CONDITION WHICH
	//    `TASK-949` COULD NOT BUY: on a spawnable card with no actor Blueprint the
	//    walk's `TestTrue` receives `false` and its `AddError` fires. This test
	//    watches that happen without ever entering the walk, so the suite records
	//    the failure mode on every green pass instead of only when one is broken.
	const bool bAbsentResolves  = FPackageName::DoesPackageExist(SyntheticComposed.PackageName);
	const bool bPresentResolves = FPackageName::DoesPackageExist(PositiveComposed.PackageName);

	TestFalse(FString::Printf(TEXT("⭐ NEGATIVE CONTROL — the composed path for a MEASURED-ABSENT card ('%s') does NOT resolve. This is the sibling walk's own assertion expression, evaluated on an absent asset: it is FALSE here, which is precisely the value that takes that walk RED"), *SyntheticComposed.PackageName),
		bAbsentResolves);

	TestTrue(FString::Printf(TEXT("⭐ POSITIVE CONTROL — the SAME FPackageName::DoesPackageExist call returns PRESENT for '%s'. Without this, a probe that had gone blind and answered ABSENT for everything would satisfy the assertion above for entirely the wrong reason"), *PositiveComposed.PackageName),
		bPresentResolves);

	TestTrue(TEXT("⭐⭐ DISCRIMINATION — ONE probe symbol returned DIFFERENT answers for a measured-absent and a measured-present card in the SAME run. ⛔ This is the assertion that would have to be inverted for this control to lie, and a probe stuck at EITHER polarity fails HERE"),
		bAbsentResolves != bPresentResolves);

	// ══ THE REPORT ══════════════════════════════════════════════════════════
	// ⛔ Published so the COMPILED suite log carries the evidence verbatim rather
	//    than resting on this file's prose (`SC-§49`, `SHIP-§9c` cl. 5: a gate a
	//    reader cannot reach must at minimum ANNOUNCE ITSELF in the output).
	AddInfo(FString::Printf(TEXT("NEGATIVE CONTROL — card table derived from the shipped CDO: '%s'; Economy-building exception: %d CardID(s)."),
		*CardTablePath.ToString(), BuildingEconomyCardIDs->Num()));

	AddInfo(FString::Printf(TEXT("NEGATIVE CONTROL — DoesPackageExist('%s') = %s   [synthetic, measured absent]"),
		*SyntheticComposed.PackageName, bAbsentResolves ? TEXT("PRESENT") : TEXT("ABSENT")));

	AddInfo(FString::Printf(TEXT("NEGATIVE CONTROL — DoesPackageExist('%s') = %s   [positive control]"),
		*PositiveComposed.PackageName, bPresentResolves ? TEXT("PRESENT") : TEXT("ABSENT")));

	AddInfo(TEXT("⭐ ⇒ the resolver said NO to an absent card and YES to a present one, through ONE symbol, in ONE run. The sibling walk asserts that same expression is TRUE for all 22 spawnable rows; this run demonstrates it is capable of being FALSE."));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
