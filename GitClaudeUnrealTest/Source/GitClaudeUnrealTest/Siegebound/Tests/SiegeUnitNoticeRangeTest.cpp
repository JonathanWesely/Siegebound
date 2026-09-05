// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Misc/CString.h" // TASK-1003: test 5's roster lane parses cards.csv cells with FCString::Atof
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/MinerUnit.h"
#include "Siegebound/SorcererUnit.h"
#include "Siegebound/SummonedUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE PER-UNIT NOTICE (ENGAGEMENT) RADIUS CHANNEL ═══
 *      TASK-979, re-valued by TASK-1003; law ⭐⭐⭐ FOG-§9.11 (which SUPERSEDES the NUMBERS in
 *      FOG-§9.9 / FOG-§9.10 and RETIRES FOG-§9.10a's identity rule), FOG-§9.6, FOG-§9.6b,
 *      FOG-§9.8a, FOG-§9.8b, FOG-§9.8e, HIGH-§1, SC-§37, SC-§45, SC-§60, SHIP-§9c.
 *
 *  ⚖️ 🧑 Jonathan, verbatim 2026-09-04, ⛔ THE LIVE RULINGS: *"lets fix it by changing the notice
 *  radius for all units to 5000 with no fog and still 609 under fog"* (`J-F28`) and *"lets make
 *  the leash radius 8000"* (`J-F27`).
 *  ⛔ BOTH SUPERSEDE HIS OWN EARLIER SENTENCES FROM THE SAME DAY, kept here so a reader who
 *  remembers them finds their replacement: *"notice enemies within 2000 units instead of 600
 *  units"* … *"make the 3600 the NOTICE range for the longbowman."*
 *
 *  ⛔⛔ WHY THIS FILE EXISTS AND WHY IT IS NOT ONE `TestEqual(AggroRadius, 5000)`. His sentences
 *  are not a number change: they make **the default a DEFAULT and not a CAP**, and that
 *  difference is invisible to the obvious test. An implementer who reads *"the MAX range that all
 *  ranged units can fire at"* as a ceiling writes `FMath::Min(Range, TheDefault)`; a bigger card
 *  reach is silently clamped back — and ⛔ nothing errors, ⛔ nothing logs, and a test asserting
 *  *"nothing exceeds the default"* is ⛔ **GREEN AGAINST THE DEFECT**. This project has already
 *  shipped that shape twice (the tower-stacking pair, `SC-§60`).
 *
 *  ⚠️⚠️ AND HIS `5000` ⛔ INVERTED WHICH MISREADING IS DANGEROUS, WHICH IS THE SINGLE MOST
 *  IMPORTANT THING ABOUT THIS EDIT AND THE ONE A VALUE-ONLY UPDATE WOULD HAVE MISSED:
 *    • at the retired 2000, the Longbowman's 3600 sat ABOVE the default, so ⛔ a real card
 *      demonstrated the anti-cap property and test 2(a) could ride it;
 *    • at 5000 ⛔ NO SHIPPED CARD SITS ABOVE THE DEFAULT AT ALL. `3600 > 5000` is FALSE, so that
 *      row would have gone ⛔ RED — and, worse, the sentence beside it (*"a `FMath::Min` returns
 *      the default here and this goes red"*) became ⛔ FALSE: `min(3600, 5000) = 3600`, so a clamp
 *      would now pass that row. ⇒ ⭐ the anti-cap property is carried by a ⛔ SYNTHETIC value and
 *      by a structural probe, ⛔ never again by a card;
 *    • and the ⛔ NEW danger is the opposite spelling — an `FMath::Max(Row, Default)` would WIDEN
 *      a card that asked for LESS. ⛔ Test 2 asserts BOTH refusals, by value AND structurally.
 *
 *  ⭐⭐ THE SECOND THING THIS FILE DEFENDS IS AN ORDERING, NOT A VALUE (tests 3 and 5).
 *  `UpdateState` drops a target beyond the leash and then, with ⛔ no intervening `return` and
 *  ⛔ no branch, calls `AcquireTarget()` in the SAME 0.25 s poll — which reaches to the notice
 *  radius. The old pair (leash 900 / notice 600) made that safe by ordering; raising notice
 *  INVERTED it and the leash silently stopped being a leash. ⚠️ The inversion lives in the CALL
 *  GRAPH, not in either number, so no test of the `AggroRadius` value alone can see it.
 *
 *  ⭐⭐⭐ TEST 5 IS NEW (TASK-1003) AND IT IS THE ⛔ LAW ITSELF: **`LEASH > NOTICE > LONGEST FIRING
 *  RANGE`** — today `8000 > 5000 > 3600`. ⛔ EVERY ONE OF THOSE THREE NUMBERS IS ⛔ DERIVED: the
 *  firing range by scanning every `bRanged` row of `Docs/Data/cards.csv`, the notice radius by
 *  resolving ⛔ every row through `ResolveNoticeRadiusUU`, and the leash by feeding the winner
 *  through `ResolveEffectiveLeashRangeUU`. ⛔ A test asserting `8000 > 5000 > 3600` as three
 *  LITERALS is green against a roster it has never read (`FOG-§9.11`, `SC-§45`).
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero writes. FOUR lanes, all
 *  house patterns:
 *    (a) DIRECT CALLS into the two pure static seams — real execution, no world;
 *    (b) CDO READS via `GetDefault<T>()` — ⛔ per class, never the base (`SiegeBuildingStackTest`'s
 *        lane, and the defect it was written from is re-asserted here in test 1(d));
 *    (c) SOURCE-TEXT structural probes with comment lines skipped, for the claims that are about
 *        WHICH CALL a site makes rather than what a number equals;
 *    (d) ⭐ NEW (TASK-1003): a DATA lane — `Docs/Data/cards.csv` parsed by header name, the
 *        `SiegeFogClampTest.cpp` test-7 pattern, ⛔ with its silent `continue` on a misaligned
 *        row turned into a HARD FAILURE. ⚠️ That CSV's FIRST COLUMN HEADER IS EMPTY
 *        (`FOG-§9.8e`), so the CardID column is addressed as index 0 and never by name, and the
 *        row count is asserted as a POSITIVE CONTROL — ⛔ a well-formed table of blanks is the
 *        failure mode this lane is most likely to have.
 *
 *  ⚠️ DECLARED GAP, stated rather than left for a reader to find (`SC-§40`): **green here is not
 *  "units notice at 5000 in a match."** There is no `UWorld::CreateWorld` and no `SpawnActor`
 *  anywhere in `Siegebound/Tests/` (the house rule), so `LoadStatsAndStart`'s binding is asserted
 *  STRUCTURALLY (test 4) and its arithmetic is asserted through the pure seam it calls — the live
 *  behaviour needs a PIE pass. ⚠️ The non-finite guards are likewise asserted structurally: no test
 *  in this module constructs a NaN, and inventing one here would be a new idiom for one row.
 *
 *  ⛔ FOG IS DELIBERATELY NOT TESTED IN THIS FILE. `SiegeFogClampTest.cpp` owns every fog claim
 *  about this site — including the two rows TASK-979 had to RE-DERIVE there (`SC-§60`) — and a
 *  second copy of a fog assertion is a second place for it to rot.
 */

namespace SiegeUnitNoticeRangeFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these structural claims are about. */
	const TCHAR* SummonedUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp");
	const TCHAR* SummonedUnitHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h");
	const TCHAR* PlayerControllerHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h");

	/**
	 *  ⭐ THE ROSTER (TASK-1003, test 5). ⛔ The CSV and ⛔ not `/Game/Data/DT_Cards`: this module
	 *  has no engine, and the CSV is the authored source every other data-reading test in
	 *  `Siegebound/Tests/` uses. ⚠️ DECLARED, ⛔ not hidden (`FOG-§9.11a`): the RUNNING GAME reads
	 *  the DataTable, so a CSV-only assertion cannot prove what ships — it proves what was
	 *  AUTHORED. The two halves are kept in step by a paired task, ⛔ never by this file.
	 */
	const TCHAR* CardsCsv = TEXT("Docs/Data/cards.csv");

	/** Exact-equality tolerance, for the claims whose whole content is the word BIT-IDENTICAL. */
	constexpr float Exact = 0.f;

	/** Reads a shipped project file. ⛔ A probe that cannot read its subject FAILS. */
	static bool LoadProjectFile(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath) || !FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — a stale probe FAILS rather than reporting safe."), *FullPath));
			return false;
		}
		return true;
	}

	/**
	 *  Occurrences of Needle on CODE lines only — the house helper, copied verbatim from
	 *  `SiegeAcquisitionFunnelTest.cpp` / `SiegeFogClampTest.cpp` so the three agree character
	 *  for character.
	 *
	 *  ⚠️⚠️ LOAD-BEARING HERE ABOVE ALL: the paragraphs beside every symbol below NAME the
	 *  constructs this file asserts are ABSENT — `FMath::Min`, `bRangedAttack`, `GetDefault<`.
	 *  A scanner that counted comments would force `SummonedUnit.{h,cpp}` to choose between
	 *  explaining the refusal and passing it, and the explanation would lose.
	 *  ⚠️ DECLARED LIMITATION, inherited and restated: a comment TRAILING a line of code IS
	 *  still scanned. Every probe below is a whole-line construct or a statement.
	 */
	static int32 CountOccurrencesInCode(const FString& Source, const TCHAR* Needle)
	{
		const int32 NeedleLength = FCString::Strlen(Needle);
		if (NeedleLength <= 0)
		{
			return 0;
		}

		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		int32 Count = 0;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();

			// `*` is qualified rather than bare on purpose: a doc-comment continuation is `* text`
			// or `*/`, while `*GetNameSafe(Foo)` starts a CODE line with the same character.
			const bool bIsCommentLine =
				Trimmed.StartsWith(TEXT("//"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("* "), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("*/"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("/*"), ESearchCase::CaseSensitive)
				|| Trimmed.Equals(TEXT("*"), ESearchCase::CaseSensitive);

			if (bIsCommentLine)
			{
				continue;
			}

			int32 From = 0;
			for (;;)
			{
				const int32 Found = Trimmed.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
				if (Found == INDEX_NONE)
				{
					break;
				}
				++Count;
				From = Found + NeedleLength;
			}
		}
		return Count;
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace — the
	 *  house helper. ⛔ Deliberately NOT a parser: a signature that stops matching FAILS rather
	 *  than silently scanning nothing (`SC-§38` — a probe pinned to a stale coordinate must go
	 *  RED, never quietly green).
	 */
	static bool ExtractFunctionBody(FAutomationTestBase& Test, const FString& Source, const TCHAR* Signature, FString& OutBody)
	{
		const int32 SignatureIndex = Source.Find(Signature, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		if (SignatureIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ '%s' not found — the probe is stale, so it FAILS."), Signature));
			return false;
		}

		const int32 BodyEnd = Source.Find(TEXT("\n}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SignatureIndex);
		if (BodyEnd == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find the end of '%s' — the probe is stale, so it FAILS."), Signature));
			return false;
		}

		OutBody = Source.Mid(SignatureIndex, BodyEnd - SignatureIndex);
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 — THE DEFAULT, AND THE TWO CLASSES THAT MUST NOT HAVE RECEIVED IT.
//  ⭐ Item (d) is the one that matters: it fails if the per-class read is written
//  the way this project has ALREADY SHIPPED it once (Building.cpp:302).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeUnitNoticeDefaultAndSealsTest,
	"Siegebound.Notice.TheDefaultIsTheEngagementRadiusAndTheSealedClassesAreStillZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeUnitNoticeDefaultAndSealsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeUnitNoticeRangeFixture;

	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	const AMinerUnit* const MinerDefaults = GetDefault<AMinerUnit>();
	const ASorcererUnit* const SorcererDefaults = GetDefault<ASorcererUnit>();
	if (!UnitDefaults || !MinerDefaults || !SorcererDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: a GetDefault<>() returned null — every expectation below is derived from ")
			TEXT("these objects, so nothing after this line would mean anything."));
		return false;
	}

	// ── (a) THE NUMBER, PINNED TO HIS SENTENCE EXACTLY ONCE. ⚠️ This is the ONLY place a `5000`
	//    literal is typed as THE NOTICE RADIUS outside the constant's own definition, and it is
	//    deliberate: the shipped source carries exactly one (FOG-§1's one-literal discipline; the
	//    project's own shipping-source census excludes `Tests/` by construction), while a rule
	//    with no pin at all cannot catch a fat-fingered 50000.
	//    ⚠️ `USiegePlayerController::GroupRadiusMax` is ALSO 5000 and is quoted in test 4(c) — it
	//    is a DIFFERENT QUANTITY that his ruling deliberately made numerically equal (FOG-§9.5's
	//    "when one moves, MUST the other?" ⇒ NO). ⛔ Never substitute one for the other.
	TestEqual(
		TEXT("⭐⭐ `UnitEngagementRadiusUU` is 5000 — 🧑 Jonathan, 2026-09-04: \"lets fix it by changing the notice ")
		TEXT("radius for all units to 5000 with no fog and still 609 under fog\" (J-F28). ⛔ This SUPERSEDES the 2000 ")
		TEXT("he ruled EARLIER THE SAME DAY. ⛔ It applies to EVERY unit including MELEE — he ruled that in by name ")
		TEXT("(J-F21) — and to every RANGED unit too, which is what retires the old \"notice == firing\" identity ")
		TEXT("rule (FOG-§9.10a, retired by FOG-§9.11)."),
		ASummonedUnit::UnitEngagementRadiusUU, 5000.f, Exact);

	// ── (b) …AND THE MEMBER IS INITIALISED FROM THE CONSTANT, not independently typed. If someone
	//    writes `float AggroRadius = 5000.f;` this row still passes — which is why the structural
	//    probe in test 4 asserts the header carries the SYMBOL rather than a second literal.
	// ⛔ Read through the PUBLIC accessor throughout this file: `AggroRadius`, `LeashRange` and
	// `LeashMarginMultiplier` are PROTECTED members, and every non-member CDO read in this codebase
	// goes through an accessor (the `UnitCDO->GetCapsuleComponent()` idiom, Barracks.cpp:128) —
	// never a raw member.
	TestEqual(
		TEXT("⭐ The shipped CDO default for AggroRadius IS the engagement radius (600 → 2000 → 5000, twice in one ")
		TEXT("day). ⛔ The old value was a GDD §3.8 profile constant no card could influence; it is a card stat now."),
		UnitDefaults->GetEngagementRadiusUU(), ASummonedUnit::UnitEngagementRadiusUU, Exact);

	// ── (c) THE TWO CLASS CONTRACTS. Raising either to the default un-seals a unit that is sealed by
	//    design (TASK-979 item 3: AUTOMATIC FAIL). They are ASSERTED here and CHANGED nowhere.
	TestEqual(
		TEXT("⛔⛔ AMinerUnit still has AggroRadius 0 — seal #2 of its \"never attacks\" class contract ")
		TEXT("(MinerUnit.cpp constructor). AcquireTarget rejects every candidate farther than this, so at 0 it can ")
		TEXT("never return one."),
		MinerDefaults->GetEngagementRadiusUU(), 0.f, Exact);

	TestEqual(
		TEXT("⛔⛔ ASorcererUnit still has AggroRadius 0 — the same behavioural quieting (SorcererUnit.cpp ")
		TEXT("constructor; its real seal is CanEverAttack(), this is what stops it ACTING like it wants to fight)."),
		SorcererDefaults->GetEngagementRadiusUU(), 0.f, Exact);

	// ── (d) ⭐⭐ THE PER-CLASS READ — THE ROW THIS FILE WAS WORTH WRITING FOR.
	//    ⛔ Building.cpp:302 is this project's ALREADY-SHIPPED instance of the wrong spelling: a
	//    per-class ceiling read off `GetDefault<ABuilding>()`, so every subclass value is read
	//    straight past — and the comment beside it claiming it reads "THIS CLASS'S CDO" is false.
	//    Written that way here, GetClassDefaultEngagementRadiusUU() would return 5000 for a miner
	//    and LoadStatsAndStart would stamp 5000 over both class seals, un-sealing two unit classes
	//    from a line that reads like a no-op. ⚠️ The STAKE GREW with his ruling: an un-sealed miner
	//    would now sweep 6.25× the area it would have a day earlier. These three rows go red
	//    together if it ever is.
	TestEqual(
		TEXT("⭐⭐ AMinerUnit's CLASS default reads 0 — `GetClass()->GetDefaultObject<>()`, ⛔ NOT ")
		TEXT("`GetDefault<ASummonedUnit>()`. A base-CDO read returns 5000 here and this row is the tripwire."),
		MinerDefaults->GetClassDefaultEngagementRadiusUU(), 0.f, Exact);

	TestEqual(
		TEXT("⭐⭐ ASorcererUnit's CLASS default reads 0, for the same reason."),
		SorcererDefaults->GetClassDefaultEngagementRadiusUU(), 0.f, Exact);

	TestEqual(
		TEXT("⭐ …and the BASE class's own class-default is the engagement radius — the positive control that ")
		TEXT("stops the two zeros above from passing on a function that returns 0 unconditionally."),
		UnitDefaults->GetClassDefaultEngagementRadiusUU(), ASummonedUnit::UnitEngagementRadiusUU, Exact);

	// ── (e) ⭐ THE ACCESSOR TASK-980 CONSUMES IS PER-UNIT, ⛔ NOT A CONSTANT.
	//    ⚠️ RE-DERIVED while writing this file, and the first version is worth recording: it read
	//    `GetEngagementRadiusUU() == AggroRadius`, which — once both sides go through the accessor,
	//    as they must, since the member is protected — is the TAUTOLOGY `x == x`. It would have been
	//    permanently green against every possible implementation, including
	//    `return UnitEngagementRadiusUU;`. The claim is therefore asserted as a DIFFERENCE between
	//    two classes, which a constant cannot produce.
	TestTrue(
		TEXT("⭐⭐ GetEngagementRadiusUU() VARIES BY UNIT — the base answers the default and a sealed class answers 0. ")
		TEXT("⛔ THIS IS THE ROW TASK-980 DEPENDS ON: its firing seam consumes this accessor, so an implementation that ")
		TEXT("returned the CONSTANT would compile, read correctly, pass every value row above, and silently nerf the ")
		TEXT("one excepted card back to the default."),
		UnitDefaults->GetEngagementRadiusUU() != MinerDefaults->GetEngagementRadiusUU());

	// ── (f) THE STRUCTURAL MIRROR: the unit never performs a base-CDO read at all.
	FString UnitSource;
	if (LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		TestEqual(
			TEXT("⭐⭐ `SummonedUnit.cpp` performs ZERO `GetDefault<ASummonedUnit>()` reads. That exact call is what ")
			TEXT("pinned Building's stacking cap to the base CDO and discarded every subclass ceiling; a value read ")
			TEXT("off the base here would discard both class seals the same way."),
			CountOccurrencesInCode(UnitSource, TEXT("GetDefault<ASummonedUnit>()")), 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐ — THE HEADLINE. **THE DEFAULT IS A DEFAULT, NOT A CAP — AND SINCE
//  TASK-1003, NOT A FLOOR EITHER.**
//
//  ⛔⛔ RE-DERIVED AT HIS 5000, ⛔ NOT RE-SIGNED, AND THE REASON IS THE WHOLE
//  POINT (SHIP-§9: validate a gate against the FAILURE it detects, never merely
//  against success). As shipped by TASK-979 this test rode the Longbowman's 3600
//  against a 2000 default: `3600 > 2000` proved the anti-cap property from LIVE
//  DATA. ⛔ At 5000 that row is `3600 > 5000` — FALSE — so it would have gone RED,
//  and the sentence beside it ("a `FMath::Min` returns the default here and this
//  goes RED") would have become a LIE, because `min(3600, 5000) = 3600` and a
//  clamp now PASSES it. ⇒ ⭐ the anti-cap property moves onto a SYNTHETIC value
//  above the default plus the structural probe, and the 3600 row is re-pointed at
//  the property his ruling actually made reachable: a cell BELOW the default is
//  honoured verbatim and ⛔ NEVER WIDENED. That second refusal is new, and it is
//  now the one a live card could plausibly trip.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeUnitNoticeChannelIsNotCappedTest,
	"Siegebound.Notice.APerUnitNoticeRangeAboveTheDefaultSurvivesTheChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeUnitNoticeChannelIsNotCappedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeUnitNoticeRangeFixture;

	const float DefaultUU = ASummonedUnit::UnitEngagementRadiusUU;

	// ── (a) ⭐⭐ THE ANTI-CAP PROPERTY, ON A SYNTHETIC VALUE — ⛔ BECAUSE NO SHIPPED CARD CAN
	//    CARRY IT ANY MORE. The value is DERIVED from the default (×2) rather than typed, so a
	//    clamp cannot be hidden behind an allow-list of known card numbers, and so this row
	//    tracks the constant if it ever moves again.
	//    ⚠️ DECLARED, because it is a real loss and not a wash: at the retired 2000 this property
	//    was demonstrated by DATA (the Longbowman's 3600) and it is now demonstrated only by a
	//    SYNTHETIC. A synthetic cannot go stale, but it also cannot notice that the roster stopped
	//    exercising the path — which is exactly why (f)'s structural refusal carries equal weight.
	const float SyntheticAboveDefault = DefaultUU * 2.f;
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ A card row of %.1f comes back out UNCHANGED — ⛔ NOT clamped to the %.1f default. ⛔ A ")
			TEXT("`FMath::Min(RowNoticeRangeUU, UnitEngagementRadiusUU)` returns the default here and this goes RED. ")
			TEXT("⚠️ The value is SYNTHETIC (default ×2) on purpose: since 🧑 his 5000 ruling ⛔ NO SHIPPED CARD SITS ")
			TEXT("ABOVE THE DEFAULT, so a clamp is INERT against the roster and every data-derived row would pass it."),
			SyntheticAboveDefault, DefaultUU),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, SyntheticAboveDefault), SyntheticAboveDefault, Exact);

	TestTrue(
		FString::Printf(
			TEXT("⭐⭐ …stated as the PROPERTY rather than the number, so it cannot be satisfied by special-casing one ")
			TEXT("value: a unit's notice radius CAN EXCEED the %.1f default. ⛔ Without this assertion the channel's ")
			TEXT("whole reason for existing is untestable (SC-§37), and a test asserting \"nothing exceeds the ")
			TEXT("default\" would be green against the defect."),
			DefaultUU),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, SyntheticAboveDefault) > DefaultUU);

	// ── (b) ⭐⭐ THE REFUSAL HIS 5000 MADE REACHABLE, AND ⛔ THE NEW ONE: A CELL **BELOW** THE
	//    DEFAULT IS HONOURED VERBATIM AND ⛔ NEVER WIDENED UP TO IT. 3600 is the Longbowman's
	//    historic notice cell — the value that used to prove the anti-CAP property from above and
	//    now proves the anti-WIDEN property from below. ⛔ `FMath::Max(Row, Default)`, or any
	//    "normalise the sparse column to the default" pass, returns the DEFAULT here and this
	//    row goes RED.
	//    ⚠️ SUPPLIED DIRECTLY rather than read from `cards.csv`: TASK-1004 BLANKS that cell (at a
	//    5000 default a 3600 would make the Longbowman notice LESS than everyone else), so a
	//    data-driven row here would be red for a reason that has nothing to do with this channel.
	//    ⭐ The CLAIM under test is the channel's ARITHMETIC; test 5 is where the DATA is read.
	const float BelowDefaultCellUU = 3600.f;
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ A card row of %.1f — BELOW the %.1f default — comes back out as %.1f, ⛔ NOT raised to the ")
			TEXT("default. ⛔ This is the spelling 🧑 his 5000 made dangerous: an `FMath::Max` here would WIDEN every ")
			TEXT("card that asked for LESS, and the sparse column would stop being an opt-in channel and become a ")
			TEXT("floor nobody voted for. ⛔ Nothing errors and nothing logs if it does."),
			BelowDefaultCellUU, DefaultUU, BelowDefaultCellUU),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, BelowDefaultCellUU), BelowDefaultCellUU, Exact);

	TestTrue(
		FString::Printf(
			TEXT("⭐ …stated as the PROPERTY: a resolved notice radius CAN BE SMALLER than the %.1f default. ⛔ Both ")
			TEXT("directions are asserted because the channel is a PASS-THROUGH, not a clamp in either sense — and ")
			TEXT("because which direction is dangerous INVERTED when his number moved (FOG-§9.11)."),
			DefaultUU),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, BelowDefaultCellUU) < DefaultUU);

	// ── (c) THE SPARSE CASE — how ⛔ EVERY card in the table carries the default without one copy
	//    of the number existing in the data at all. ⭐ Since TASK-1004 this is not "~every row" but
	//    ⛔ ALL 32 of them: the column is ENTIRELY sparse, which is the CORRECT shape and not an
	//    unfinished one (FOG-§9.9 — the CHANNEL is the deliverable, a future card opts in from its
	//    own row with zero code).
	TestEqual(
		TEXT("⭐ A blank/0 cell resolves to the class default — 🧑 \"the notice radius for all units to 5000\", ")
		TEXT("expressed as an ABSENCE of data rather than 32 hand-typed cells (the drift surface this batch has ")
		TEXT("already fought three times)."),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, 0.f), DefaultUU, Exact);

	TestEqual(
		TEXT("⭐ …and a negative cell (the realistic corrupt-data case) falls BACK to the class default rather than ")
		TEXT("blinding the unit. ⛔ It fails toward the shipped behaviour, never toward no vision."),
		ASummonedUnit::ResolveNoticeRadiusUU(DefaultUU, -50.f), DefaultUU, Exact);

	// ── (d) ⭐⭐ THE SEAL — A CARD CELL CAN NEVER UN-SEAL A SEALED CLASS. This is the row that
	//    makes the channel safe to add at all: without it, data alone could give a miner an
	//    acquisition radius with no code change for anyone to review.
	TestEqual(
		TEXT("⛔⛔ A sealed class (AggroRadius 0) IGNORES the row entirely — even a 3600 cell. The class contract ")
		TEXT("outranks the data, so AMinerUnit/ASorcererUnit cannot be un-sealed from cards.csv."),
		ASummonedUnit::ResolveNoticeRadiusUU(0.f, 3600.f), 0.f, Exact);

	TestEqual(
		TEXT("⛔ …and with no cell either, the seal is simply preserved."),
		ASummonedUnit::ResolveNoticeRadiusUU(0.f, 0.f), 0.f, Exact);

	// ── (e) A BLUEPRINT CDO OVERRIDE SURVIVES a sparse row. ⚠️ This is a hazard being made
	//    explicit rather than a feature: AggroRadius is EditAnywhere, so a BP number silently
	//    WINS. The per-class read is what keeps it from being stamped over — but a number in a
	//    .uasset is un-greppable and un-diffable, which is why the card ROW is the sanctioned
	//    channel and a BP override is not.
	TestEqual(
		TEXT("⭐ A per-class default that is neither 0 nor the base value (i.e. a Blueprint CDO override) survives a ")
		TEXT("blank cell — the resolver returns the CLASS's number, never the base class's."),
		ASummonedUnit::ResolveNoticeRadiusUU(1234.f, 0.f), 1234.f, Exact);

	// ── (f) THE STRUCTURAL REFUSAL, because (a)–(e) all pass on a `min` whose threshold happens to
	//    sit above every value tested. ⛔ There is no ceiling — and since TASK-1003, no FLOOR —
	//    arithmetic in the resolver AT ALL.
	FString UnitSource;
	if (LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		FString ResolverBody;
		if (ExtractFunctionBody(*this, UnitSource,
			TEXT("float ASummonedUnit::ResolveNoticeRadiusUU(float ClassDefaultRadiusUU, float RowNoticeRangeUU)"),
			ResolverBody))
		{
			TestEqual(
				TEXT("⭐⭐ The resolver contains ZERO `FMath::Min` — the refusal is structural, not merely arithmetic. ")
				TEXT("⛔ The ONE universal ceiling in this system is fog's 609.6 uu vision ceiling, applied as a `min` ")
				TEXT("at the ONE chokepoint inside the acquisition funnel (FOG-§7), and a second one here would be ")
				TEXT("invisible to every value-based row above. ⚠️ It carries MORE weight since 🧑 his 5000: no shipped ")
				TEXT("card exceeds the default any more, so this probe and the synthetic in (a) are the ONLY two ")
				TEXT("things that can see a clamp land."),
				CountOccurrencesInCode(ResolverBody, TEXT("FMath::Min")), 0);

			// ⭐⭐ NEW (TASK-1003) AND IT IS THE ROW THE NUMBER CHANGE CREATED. At a 2000 default the
			// dangerous spelling was `min`; at 5000 it is `max`, because every populated cell a
			// designer can now write is BELOW the default and a "normalise it up" pass would be
			// invisible to (b) only if (b) were ever deleted. Both are pinned so neither can be
			// re-introduced under cover of the other being absent.
			TestEqual(
				TEXT("⭐⭐ …and ZERO `FMath::Max` — the OPPOSITE spelling, and the one 🧑 his 5000 ruling made the live ")
				TEXT("danger. ⛔ A `FMath::Max(RowNoticeRangeUU, UnitEngagementRadiusUU)` would WIDEN every card that ")
				TEXT("asked for a shorter reach than the class, turning an opt-in channel into a floor nobody voted ")
				TEXT("for — with nothing in any log to say so."),
				CountOccurrencesInCode(ResolverBody, TEXT("FMath::Max")), 0);

			TestEqual(
				TEXT("⭐ …and ZERO `FMath::Clamp` — same argument, and it covers BOTH directions at once."),
				CountOccurrencesInCode(ResolverBody, TEXT("Clamp")), 0);

			TestTrue(
				TEXT("⭐ …while the non-finite MATH GUARD is present (`FMath::IsFinite`). ⚠️ DECLARED: this guard is ")
				TEXT("asserted STRUCTURALLY and not by execution — no test in this module constructs a NaN, and ")
				TEXT("inventing an idiom for one row would be worse than naming the gap."),
				CountOccurrencesInCode(ResolverBody, TEXT("FMath::IsFinite")) > 0);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐ — THE LEASH ORDERING. The defect here is not in either number; it
//  is that a drop and a re-acquire sit in ONE poll with nothing between them.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeUnitNoticeLeashOrderingTest,
	"Siegebound.Notice.TheLeashStillOutranksTheNoticeRadiusAtEveryValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeUnitNoticeLeashOrderingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeUnitNoticeRangeFixture;

	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!UnitDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: GetDefault<ASummonedUnit>() returned null."));
		return false;
	}

	// The pre-TASK-979 pair, recorded so the margin's provenance is checkable rather than asserted.
	constexpr float HistoricNoticeUU = 600.f;
	constexpr float HistoricLeashUU = 900.f;

	// ── (a) ⭐ WHERE 1.5 CAME FROM — ⛔ PROVENANCE, AND SINCE TASK-1003 ⛔ NO LONGER THE REASON IT
	//    IS STILL HERE. The game shipped leash 900 against notice 600 — exactly 1.5 — so the
	//    default reproduced the shipped ORDERING. ⛔ THAT ARGUMENT IS DEAD AT THE SHIPPED VALUES:
	//    at 🧑 his 8000 floor against a 5000 notice radius the FLOOR WINS and this multiplier is
	//    INERT for every card in the game. Rows (e) and (f) below carry its REPLACEMENT
	//    justification — the future-card case — as EXECUTED assertions rather than as a comment,
	//    because "an inert term with a good reason" and "dead code" are indistinguishable to the
	//    next reader unless something goes red when it is deleted (FOG-§9.11).
	TestEqual(
		TEXT("⭐ The ordering ratio the game ONCE shipped was 900/600 = 1.5, and that is where LeashMarginMultiplier's ")
		TEXT("default came from — ⛔ measured, not a taste call. ⚠️ This row is PROVENANCE ONLY now: at 8000/5000 the ")
		TEXT("floor wins and this multiplier changes NOTHING for any shipped card. See (e)/(f) for why it stays."),
		UnitDefaults->GetLeashMarginMultiplier(), HistoricLeashUU / HistoricNoticeUU, Exact);

	// ── (b) ⭐ …AND AT THE OLD PAIR THE FUNCTION IS STILL A NO-OP. ⚠️ RE-READ THIS ROW CORRECTLY:
	//    it feeds the historic 900 in as the FLOOR ARGUMENT, so it is a claim about the FUNCTION's
	//    arithmetic and ⛔ NOT about the shipped member (which is 8000 now). It was written when it
	//    also proved "the mechanism changed and the behaviour did not"; that second reading expired
	//    the moment 🧑 he ruled 8000, and it is a pure regression pin on the seam today.
	TestEqual(
		TEXT("⭐ Fed the pre-ruling pair (900 / 600 × 1.5) the resolver still returns EXACTLY 900 — bit-identical. ")
		TEXT("⚠️ A claim about the FUNCTION, ⛔ not about the shipped leash: LeashRange is 8000 now (J-F27), so this ")
		TEXT("row proves the seam is unchanged, ⛔ not that behaviour is."),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(HistoricLeashUU, HistoricNoticeUU, UnitDefaults->GetLeashMarginMultiplier()),
		HistoricLeashUU, Exact);

	// ── (b2) ⭐⭐⭐ HIS NUMBER, ⛔ DERIVED THROUGH THE EXPRESSION AND ⛔ NEVER HARD-CODED. This is
	//    the row that would go red against the single most tempting "simplification" available in
	//    this file — replacing `max(LeashRange, Notice × Margin)` with a bare `return 8000.f`.
	//    ⛔ Both operands are READ (CDO + constant); the only literal is the SHIPPED FLOOR, which
	//    is read back through its own accessor rather than typed.
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐⭐ At the SHIPPED pair the effective leash is the FLOOR — max(%.1f, %.1f × %.2f = %.1f) = %.1f, ")
			TEXT("🧑 his J-F27 number EXACTLY (\"lets make the leash radius 8000\"). ⛔ The floor winning is the ")
			TEXT("INTENDED outcome, ⛔ not evidence the expression is redundant: it is what makes his ruling ship ")
			TEXT("verbatim while the ordering stays DERIVED."),
			UnitDefaults->GetLeashRangeFloorUU(), ASummonedUnit::UnitEngagementRadiusUU,
			UnitDefaults->GetLeashMarginMultiplier(),
			ASummonedUnit::UnitEngagementRadiusUU * UnitDefaults->GetLeashMarginMultiplier(),
			UnitDefaults->GetLeashRangeFloorUU()),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(
			UnitDefaults->GetLeashRangeFloorUU(), ASummonedUnit::UnitEngagementRadiusUU,
			UnitDefaults->GetLeashMarginMultiplier()),
		UnitDefaults->GetLeashRangeFloorUU(), Exact);

	// ── (c) ⭐⭐ THE ORDERING ITSELF, AT EVERY NOTICE RADIUS THAT MATTERS. Against a flat leash of
	//    900 the ordering INVERTS at any of these: UpdateState drops at `> leash` and
	//    AcquireTarget re-takes at `<= notice` with ⛔ no return between them, so every release is
	//    re-takeable inside the same 0.25 s poll and units NEVER DISENGAGE.
	//    ⭐ Test 5 asserts the same ordering against the LIVE ROSTER; this row asserts it against
	//    a spread of values including two that are in NO data at all, so the claim is a PROPERTY
	//    of the function and not a snapshot of today's cards.
	const float NoticeRadiiToCheck[] =
	{
		HistoricNoticeUU,                           // the long-retired value — the ordering held here, and must still
		3600.f,                                     // the Longbowman's historic notice cell (blanked by TASK-1004)
		ASummonedUnit::UnitEngagementRadiusUU,      // 🧑 his shipped default — where a 900 leash inverted
		ASummonedUnit::UnitEngagementRadiusUU * 4.f // a synthetic beyond anything shipped, so the claim is general
	};

	for (const float NoticeUU : NoticeRadiiToCheck)
	{
		const float EffectiveLeashUU = ASummonedUnit::ResolveEffectiveLeashRangeUU(
			UnitDefaults->GetLeashRangeFloorUU(), NoticeUU, UnitDefaults->GetLeashMarginMultiplier());

		TestTrue(
			FString::Printf(
				TEXT("⭐⭐ At notice %.1f the effective leash (%.1f) is STRICTLY GREATER — so a target released by the ")
				TEXT("leash is provably OUTSIDE AcquireTarget's `Distance > AggroRadius` gate and cannot be re-taken ")
				TEXT("in the same poll. ⛔ THIS IS THE WHOLE FIX: the inversion lives in the CALL GRAPH — UpdateState ")
				TEXT("drops, then calls AcquireTarget with no intervening return — so no assertion about either NUMBER ")
				TEXT("alone could see it. ⛔ Derived from the unit's OWN reach, so a bigger card needs no second ")
				TEXT("number. ⛔ No line numbers cited on purpose: a coordinate in prose is a citation that rots ")
				TEXT("(SC-§40 cl. 9), and item (g) below pins the two sites by COUNT instead."),
				NoticeUU, EffectiveLeashUU),
			EffectiveLeashUU > NoticeUU);
	}

	// ── (d) THE DEGENERATE MULTIPLIER IS FLOORED AT 1.0 — the leash may never end up SHORTER than
	//    the notice radius, which would be the inversion restored by a designer typing 0.5.
	TestTrue(
		TEXT("⭐ A zero/negative margin is floored at 1.0, so the leash is never SHORTER than the notice radius. ⛔ It ")
		TEXT("degrades to the boundary case (drop and re-take at the same distance), never past it into the ")
		TEXT("never-disengage inversion."),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(UnitDefaults->GetLeashRangeFloorUU(), ASummonedUnit::UnitEngagementRadiusUU, 0.f)
			>= ASummonedUnit::UnitEngagementRadiusUU);

	// ══════════════════════════════════════════════════════════════════════════════════════════
	// ⭐⭐⭐ (e) + (f) — **THE MULTIPLIER'S SURVIVING JUSTIFICATION, AS TWO EXECUTED ASSERTIONS.**
	//
	// ⛔⛔ THE PROBLEM THESE ROWS EXIST FOR, STATED PLAINLY: `LeashMarginMultiplier` is now INERT.
	// At 8000/5000 the floor wins, deleting the whole `NoticeRadiusUU * SafeMultiplier` term would
	// change ⛔ NOTHING that ships, and every test above would stay ⛔ GREEN. ⇒ the next reader has
	// a defensible-looking case for removing it, and the ONLY thing standing against that is a
	// comment — which is exactly the class of protection this project has watched fail.
	// ⭐ So the future-card case is asserted rather than argued: the crossover is DERIVED
	// (`LeashRange / MarginMultiplier`), a notice radius just under it must still yield the FLOOR,
	// and one just over it must yield the PRODUCT. ⛔ Collapse the expression to a bare 8000 and
	// (f) goes RED, naming the reason.
	// ══════════════════════════════════════════════════════════════════════════════════════════

	const float LeashFloorUU = UnitDefaults->GetLeashRangeFloorUU();
	const float MarginUU = UnitDefaults->GetLeashMarginMultiplier();

	// ⛔ DERIVED, never typed: the notice radius at which the product overtakes the floor.
	const float CrossoverNoticeUU = LeashFloorUU / MarginUU;

	// ── (e) BELOW THE CROSSOVER (and the SHIPPED default sits here) the FLOOR wins — his number.
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ Below the derived crossover (%.2f uu = %.1f / %.2f) the FLOOR wins: at a notice radius of ")
			TEXT("%.2f the effective leash is %.1f. ⭐ 🧑 His shipped default (%.1f) sits BELOW that crossover, which ")
			TEXT("is why %.1f ships verbatim and why the multiplier changes nothing today."),
			CrossoverNoticeUU, LeashFloorUU, MarginUU, CrossoverNoticeUU * 0.99f, LeashFloorUU,
			ASummonedUnit::UnitEngagementRadiusUU, LeashFloorUU),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(LeashFloorUU, CrossoverNoticeUU * 0.99f, MarginUU),
		LeashFloorUU, Exact);

	TestTrue(
		FString::Printf(
			TEXT("⭐ …and the SHIPPED default (%.1f) really is below the crossover (%.2f) — the premise of the row ")
			TEXT("above, asserted separately so a future retune fails HERE with an explanation instead of failing ")
			TEXT("with a bare number mismatch."),
			ASummonedUnit::UnitEngagementRadiusUU, CrossoverNoticeUU),
		ASummonedUnit::UnitEngagementRadiusUU < CrossoverNoticeUU);

	// ── (f) ⭐⭐⭐ ABOVE THE CROSSOVER THE PRODUCT TAKES OVER. ⛔ THE ROW THAT GOES RED AGAINST A
	//    HARD-CODED LEASH, and the only thing that makes the inert term provably not dead code.
	const float AboveCrossoverNoticeUU = CrossoverNoticeUU * 1.5f;
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐⭐ Above the crossover the PRODUCT wins: a future card with a NoticeRange of %.1f yields %.1f, ")
			TEXT("⛔ NOT the %.1f floor. ⛔⛔ THIS IS THE ROW THAT FAILS AGAINST A HARD-CODED `return 8000.f` — and ")
			TEXT("that is the whole reason `LeashMarginMultiplier` survives its own justification going inert ")
			TEXT("(FOG-§9.11). ⛔ Without the derivation, that card re-opens the drop-then-re-acquire thrash SILENTLY, ")
			TEXT("with no diff to point at."),
			AboveCrossoverNoticeUU, AboveCrossoverNoticeUU * MarginUU, LeashFloorUU),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(LeashFloorUU, AboveCrossoverNoticeUU, MarginUU),
		AboveCrossoverNoticeUU * MarginUU, Exact);

	TestTrue(
		FString::Printf(
			TEXT("⭐⭐ …and the ORDERING still holds there: leash %.1f > notice %.1f. ⛔ That is the property, ⛔ not ")
			TEXT("the number — the derivation re-establishes it for a card nobody has designed yet, with zero edits."),
			AboveCrossoverNoticeUU * MarginUU, AboveCrossoverNoticeUU),
		ASummonedUnit::ResolveEffectiveLeashRangeUU(LeashFloorUU, AboveCrossoverNoticeUU, MarginUU) > AboveCrossoverNoticeUU);

	// ── (g) ⭐⭐ BOTH SITES, ASSERTED AS A COUNT. `UpdateState` and
	//    `UpdateStateStandardCommanded`'s ATTACK case carry the SAME drop-then-re-acquire shape —
	//    the second body "mirrors the legacy Standard body EXACTLY" by its own comment — so fixing
	//    one and not the other fixes the game for uncommanded units only.
	FString UnitSource;
	if (LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		// ⚠️⚠️ THE NEEDLE MOVED WITH THE CODE IT WATCHES (TASK-1008), AND IT MOVED ⛔ STRICTER, NOT
		// LOOSER. It read `"> GetEffectiveLeashRangeUU()"`; Jonathan then ruled *"yes clamp
		// retention under fog"* (FOG-§9.11) and both drop sites became
		// `> ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())`. ⛔ Leaving the old needle would
		// have read ZERO against ⛔ CORRECT code — the TASK-868 failure shape, arriving again — and
		// bumping it to 0 would have deleted the assertion entirely.
		// ⭐ THE REPLACEMENT PINS ⛔ BOTH CLAIMS AT ONCE, which is why it is a strengthening: the
		// site must still read the EFFECTIVE leash (never the raw floor) ⛔ AND must pass it through
		// the ONE unit-side fog ceiling. Dropping either half turns this red.
		TestEqual(
			TEXT("⭐⭐ EXACTLY TWO drop sites bound retention by the FOG-CLAMPED EFFECTIVE leash. ⛔ Both, or the fix ")
			TEXT("covers only units with no player command — and a Blue Standard unit under any command runs the ")
			TEXT("OTHER copy. ⛔ The expression is `ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())` and it is ")
			TEXT("NOT `min(LeashRange, effective notice)`: that form was MEASURED to cut the CLEAR-WEATHER leash and ")
			TEXT("to put leash EQUAL to notice, re-creating the drop-then-re-acquire thrash. ⛔ Bit-identical with ")
			TEXT("the fog down, 609.6 with it up."),
			CountOccurrencesInCode(UnitSource, TEXT("> ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())")), 2);

		TestEqual(
			TEXT("⛔⛔ …and ZERO sites still compare against the RAW `LeashRange` member. The raw value is a FLOOR ")
			TEXT("now, not the leash; a surviving `> LeashRange` is the inversion still shipping at that site."),
			CountOccurrencesInCode(UnitSource, TEXT("> LeashRange")), 0);

		// The GROUPED lane is deliberately untouched — it has no distance-from-self drop at all
		// ("the zones ARE the leash for HOLD, and AMBUSH's whole point is the unbounded chase"),
		// and an implementer told "fog drops targets" could easily have added one wholesale.
		FString GroupedBody;
		if (ExtractFunctionBody(*this, UnitSource,
			TEXT("void ASummonedUnit::UpdateStateGrouped(const FSiegeUnitGroup& Group)"), GroupedBody))
		{
			TestEqual(
				TEXT("⛔⛔ The GROUPED lane gained NO leash. Its absence is deliberate (the zones are the leash for ")
				TEXT("HOLD; AMBUSH's point is the unbounded chase), and Jonathan ruled the opposite of adding one: ")
				TEXT("\"commanded units DO NOT LOSE THEIR COMMANDS\". A wholesale drop path here would make units ")
				TEXT("abandon their guard circles."),
				CountOccurrencesInCode(GroupedBody, TEXT("LeashRange")), 0);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — THE COMMANDED-LANE NOTICE BOUND (item 6b) AND THE ROW BINDING.
//  Both are claims about WHICH CALL a site makes, so both are structural.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeUnitNoticeCommandedBoundAndBindingTest,
	"Siegebound.Notice.TheCommandedLaneIsBoundedAndTheChannelIsBoundFromTheCardRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeUnitNoticeCommandedBoundAndBindingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeUnitNoticeRangeFixture;

	FString UnitSource;
	if (!LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		return false;
	}

	// ── (a) THE BINDING. The channel is worthless if the row never reaches the member.
	FString LoadStatsBody;
	if (ExtractFunctionBody(*this, UnitSource, TEXT("void ASummonedUnit::LoadStatsAndStart()"), LoadStatsBody))
	{
		TestEqual(
			TEXT("⭐ `LoadStatsAndStart` binds the row's NoticeRange EXACTLY ONCE — beside `AttackRange = Row->Range`, ")
			TEXT("because they are the two halves of a card's reach and a reader who finds one must find the other."),
			CountOccurrencesInCode(LoadStatsBody, TEXT("Row->NoticeRange")), 1);

		TestEqual(
			TEXT("⛔⛔ …and it goes through the RESOLVER, never straight over the member. That function carries the ")
			TEXT("class seal AND the refusal to clamp; a direct `AggroRadius = Row->NoticeRange` would un-seal ")
			TEXT("AMinerUnit/ASorcererUnit from a blank cell (0 ⇒ 0 is fine, but any cell at all would land)."),
			CountOccurrencesInCode(LoadStatsBody, TEXT("ResolveNoticeRadiusUU(")), 1);

		TestEqual(
			TEXT("⛔⛔ …fed from THIS INSTANCE'S CLASS default. `GetDefault<ASummonedUnit>()` here is the ")
			TEXT("Building.cpp:302 defect, and it would stamp 5000 over both class seals."),
			CountOccurrencesInCode(LoadStatsBody, TEXT("GetClassDefaultEngagementRadiusUU()")), 1);
	}

	// ── (b) ⭐⭐ THE COMMANDED-LANE BOUND (item 6b). ⚖️ Jonathan: "if an enemy unit walks into the
	//    circle that a commanded unit is supposed to be guarding but that enemy unit is outside the
	//    range in which they can notice them ⛔ DUE TO FOG OR ANYTHING, the commanded unit still
	//    will not be able to detect them." ⛔ "OR ANYTHING" is why the bound is the ENGAGEMENT
	//    RADIUS's property and must hold in CLEAR WEATHER — TASK-980 routes this existing read
	//    through the fog accessor and does not add the bound.
	FString NearPointBody;
	if (ExtractFunctionBody(*this, UnitSource,
		TEXT("AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const"), NearPointBody))
	{
		TestEqual(
			TEXT("⭐⭐ The commanded lane now has a NOTICE BOUND, read from this unit's own engagement radius. ⛔ It ")
			TEXT("had NONE before: it gathers unbounded and filtered only by zone membership, so this is a bound ")
			TEXT("being INTRODUCED. ⛔ Boarded on the fog row alone, clear weather would silently differ from fog."),
			CountOccurrencesInCode(NearPointBody, TEXT("GetEngagementRadiusUU()")), 1);

		TestEqual(
			TEXT("⭐ …applied as a distance cut from SELF, with the same bounds-aware metric AcquireTarget uses. ⛔ The ")
			TEXT("question fog answers is \"what can THIS UNIT see\", never \"what is near the flag\"."),
			CountOccurrencesInCode(NearPointBody, TEXT("Distance > NoticeRadiusUU")), 1);

		TestTrue(
			TEXT("⛔ …and the ZONE gate SURVIVES beside it. Two terms, not a replacement: a candidate must be in the ")
			TEXT("commanded circle AND within this unit's reach. Losing the disc would turn a zone order into a ")
			TEXT("global sweep."),
			CountOccurrencesInCode(NearPointBody, TEXT("DistSquared2D")) > 0);

		TestEqual(
			TEXT("⛔⛔ THE FENCE (item 6c): the bound is NOT gated on `bRangedAttack`. That flag is PROJECTILE ")
			TEXT("DELIVERY, not \"is ranged\" — CrystalTower ships bRanged=FALSE at Range 800 and would escape such a ")
			TEXT("gate entirely (FOG-§9.8c)."),
			CountOccurrencesInCode(NearPointBody, TEXT("bRangedAttack")), 0);
	}

	// ── (c) ✅⭐⭐ J-F28 IS **ANSWERED**, AND THIS ROW RECORDS THE ANSWER RATHER THAN THE
	//    COMPLAINT. It used to read: *"a legal maximum guard circle is 2.5× wider than the 2000
	//    default, so a unit standing at its centre is blind to ~84% of its own circle's AREA."*
	//    🧑 He ruled on exactly that: *"lets fix it by changing the notice radius for all units to
	//    5000"* — and `GroupRadiusMax` is 5000, so the blind area is now ⛔ ZERO. ⭐ THAT is why
	//    his number is 5000 and not a rounder one (FOG-§9.11): it resolves the finding EXACTLY.
	//
	//    ⛔⛔ AND THE TRAP THAT CREATES, NAMED BECAUSE IT IS `FOG-§9.5` VERBATIM: two numbers that
	//    are EQUAL TODAY ARE NOT THE SAME NUMBER. `GroupRadiusMax` and `UnitEngagementRadiusUU`
	//    are both 5000 uu and both reaches — and the test is never "do they match" but ⛔ "WHEN
	//    ONE MOVES, MUST THE OTHER?", to which the answer here is ⛔ NO. ⇒ this row READS
	//    GroupRadiusMax structurally out of its OWN header and ⛔ never substitutes the unit
	//    constant for it; the coverage figure below is computed from the TWO separate operands so
	//    that retuning either one shows up as a real change rather than as `x/x = 1`.
	FString ControllerHeader;
	if (LoadProjectFile(*this, PlayerControllerHeader, ControllerHeader))
	{
		const bool bGroupMaxUnchanged =
			CountOccurrencesInCode(ControllerHeader, TEXT("GroupRadiusMax = 5000.f")) == 1;

		TestTrue(
			TEXT("⚠️⚠️ `USiegePlayerController::GroupRadiusMax` is still 5000 — READ, ⛔ never retuned, ⛔ never merged ")
			TEXT("with the unit's own constant. It is quoted because it SIZES the answer to 🧑 J-F28: at a notice ")
			TEXT("radius of 5000 a unit at the centre of a maximum guard circle covers 100% of it, where at the ")
			TEXT("retired 2000 it was blind to ~84% (1 − 2000²/5000²) in CLEAR WEATHER. ⛔ If this row goes red, that ")
			TEXT("figure must be RE-DERIVED, not re-quoted."),
			bGroupMaxUnchanged);

		if (bGroupMaxUnchanged)
		{
			// ⛔ The guard-circle radius is the number just verified in this file's SUBJECT header
			// (SiegePlayerController.h), and it is written here as its own operand precisely so the
			// two 5000s never collapse into one symbol. See FOG-§9.5.
			const float GuardCircleRadiusUU = 5000.f;
			const float CoveredFraction = FMath::Min(
				1.f,
				(ASummonedUnit::UnitEngagementRadiusUU * ASummonedUnit::UnitEngagementRadiusUU)
					/ (GuardCircleRadiusUU * GuardCircleRadiusUU));

			AddInfo(FString::Printf(
				TEXT("J-F28 measured at the shipped values: notice %.0f uu inside a %.0f uu guard circle ⇒ the unit ")
				TEXT("at its centre covers %.1f%% of the circle's area and is blind to %.1f%%. (At the retired 2000 ")
				TEXT("it covered 16.0%% and was blind to 84.0%%.)"),
				ASummonedUnit::UnitEngagementRadiusUU, GuardCircleRadiusUU,
				100.f * CoveredFraction, 100.f - 100.f * CoveredFraction));

			TestTrue(
				FString::Printf(
					TEXT("✅⭐⭐ 🧑 J-F28 IS ANSWERED IN FULL FOR THE GUARD CIRCLE: a unit at the centre of a maximum ")
					TEXT("circle now covers ALL of it (%.1f%%). ⚠️⚠️ ⛔ DO NOT READ THIS AS \"the DEFEND gap is ")
					TEXT("closed\" — a DIFFERENT measurement did NOT clear: the castle's colliding footprint is ")
					TEXT("≈7313.7 uu across, i.e. ≈1.46× this radius, so a defender at one face still does not ")
					TEXT("acquire a besieger at the opposite one. Two measurements, only one of them resolved."),
					100.f * CoveredFraction),
				ASummonedUnit::UnitEngagementRadiusUU >= GuardCircleRadiusUU);
		}
	}

	// ── (d) THE HEADER CARRIES THE SYMBOL, not a second literal — FOG-§1's one-literal discipline
	//    at the one place a copy would be invisible (an initialiser reads like a default, not like
	//    a duplicate).
	FString UnitHeader;
	if (LoadProjectFile(*this, SummonedUnitHeader, UnitHeader))
	{
		TestEqual(
			TEXT("⭐ `AggroRadius` is initialised FROM `UnitEngagementRadiusUU`, ⛔ not from a second `5000.f`. One ")
			TEXT("symbol, one number — which matters MORE now that a DIFFERENT 5000 (GroupRadiusMax) lives one ")
			TEXT("header away: a second literal here would be indistinguishable from a copy of that one."),
			CountOccurrencesInCode(UnitHeader, TEXT("float AggroRadius = UnitEngagementRadiusUU;")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐⭐ — **THE ENGAGEMENT ORDERING LAW, DERIVED FROM THE ROSTER.**
//      `LEASH > NOTICE > LONGEST FIRING RANGE`   (⭐⭐⭐ FOG-§9.11)
//      Today that reads `8000 > 5000 > 3600` — ⛔ AND NOT ONE OF THOSE THREE
//      NUMBERS IS TYPED IN THIS TEST.
//
//  ⛔⛔ WHY THE DERIVATION IS THE WHOLE ASSERTION, AND A LITERAL VERSION WOULD BE
//  A GATE FAILURE RATHER THAN A SHORTCUT: `TestTrue(8000.f > 5000.f && 5000.f >
//  3600.f)` is ⛔ PERMANENTLY GREEN — including against a roster it has never
//  opened, including after somebody adds a card with a 9000 firing range, and
//  including after the whole CSV is replaced with blanks. It asserts arithmetic
//  about three constants, ⛔ not a property of the game. ⇒ the firing range is
//  read out of ⛔ EVERY `bRanged` row of `Docs/Data/cards.csv`, the notice radius
//  is obtained by running ⛔ EVERY row through the SHIPPED resolver (so a
//  populated `NoticeRange` cell is CAUGHT rather than assumed absent), and the
//  leash is obtained by running the winner through the SHIPPED leash derivation.
//
//  ⚠️⚠️ THE INSTRUMENT HAZARDS, BOTH HANDLED IN THE OPEN:
//    • `FOG-§9.8e` — that CSV's ⛔ FIRST COLUMN HEADER IS EMPTY. A parser that
//      keys the CardID column BY NAME silently produces a well-formed table of
//      BLANKS, every number 0, and every comparison below trivially true. ⇒ the
//      CardID is addressed as ⛔ INDEX 0, and (a) is a POSITIVE CONTROL that
//      fails if the read produced nothing.
//    • The house CSV probe (`SiegeFogClampTest.cpp` test 7) SKIPS a row whose
//      field count disagrees with the header, because the free-text `Notes`
//      column can contain a comma. ⛔ Skipping is WRONG HERE and the difference
//      matters: a skipped row could hide the longest firing range in the game or
//      a populated notice cell, and this test would report a confident, smaller,
//      GREEN answer. ⇒ a misaligned row is a ⛔ HARD FAILURE, not a `continue`.
//
//  ⚠️ DECLARED SCOPE (`FOG-§9.11a`): this reads the AUTHORED CSV. The RUNNING
//  GAME reads `/Game/Data/DT_Cards`, which this module cannot open. ⛔ Green here
//  is a claim about the authored roster, ⛔ never about the shipped asset — the
//  two halves are kept in step by a paired task, ⛔ never by this file.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeUnitNoticeEngagementOrderingTest,
	"Siegebound.Notice.TheLeashOutranksTheNoticeRadiusWhichOutranksTheLongestFiringRangeInTheRoster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeUnitNoticeEngagementOrderingTest::RunTest(const FString& Parameters)
{
	using namespace SiegeUnitNoticeRangeFixture;

	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();
	if (!UnitDefaults)
	{
		AddError(TEXT("⛔ SELF-CHECK FAILED: GetDefault<ASummonedUnit>() returned null — every number below is ")
			TEXT("derived from this object or from the roster, so nothing after this line would mean anything."));
		return false;
	}

	FString Csv;
	if (!LoadProjectFile(*this, CardsCsv, Csv))
	{
		return false;
	}

	TArray<FString> Lines;
	Csv.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ true);
	if (Lines.Num() < 10)
	{
		AddError(FString::Printf(
			TEXT("⛔ SELF-CHECK FAILED: cards.csv parsed to %d line(s) — the probe is dead and its silence would ")
			TEXT("otherwise read as a passing ordering."),
			Lines.Num()));
		return false;
	}

	TArray<FString> HeaderFields;
	Lines[0].ParseIntoArray(HeaderFields, TEXT(","), /*InCullEmpty=*/ false);

	const int32 RangeColumn = HeaderFields.IndexOfByKey(FString(TEXT("Range")));
	const int32 RangedColumn = HeaderFields.IndexOfByKey(FString(TEXT("bRanged")));
	const int32 NoticeColumn = HeaderFields.IndexOfByKey(FString(TEXT("NoticeRange")));

	// ⛔ The CardID column is INDEX 0 and is deliberately NOT looked up by name — its header cell
	// is EMPTY in the shipped file (FOG-§9.8e). A by-name lookup returns INDEX_NONE and every
	// card label below becomes a blank, which is the "well-formed table of blanks" failure this
	// test is most exposed to.
	constexpr int32 CardIdColumn = 0;

	if (RangeColumn == INDEX_NONE || RangedColumn == INDEX_NONE || NoticeColumn == INDEX_NONE)
	{
		AddError(FString::Printf(
			TEXT("⛔ SELF-CHECK FAILED: cards.csv is missing a column this law is derived from ")
			TEXT("(Range=%d, bRanged=%d, NoticeRange=%d) — the probe is stale, so it FAILS rather than reporting a ")
			TEXT("safe ordering. ⛔ NoticeRange in particular: if that column were dropped, every unit would resolve ")
			TEXT("to the class default and the ordering would look correct while a card's own reach went unread."),
			RangeColumn, RangedColumn, NoticeColumn));
		return false;
	}

	// ── THE DERIVATION. Every row, every column read by NAME (except index 0), nothing typed. ──
	int32 RowsRead = 0;
	int32 RowsMisaligned = 0;
	int32 RangedRows = 0;
	int32 PopulatedNoticeCells = 0;

	float LongestFiringUU = 0.f;
	FString LongestFiringCard;
	float WidestNonRangedReachUU = 0.f;
	FString WidestNonRangedReachCard;
	float WidestNoticeUU = 0.f;
	FString WidestNoticeCard;
	bool bEveryPopulatedCellSurvivedTheChannel = true;

	for (int32 Index = 1; Index < Lines.Num(); ++Index)
	{
		TArray<FString> Fields;
		Lines[Index].ParseIntoArray(Fields, TEXT(","), /*InCullEmpty=*/ false);

		if (Fields.Num() != HeaderFields.Num())
		{
			++RowsMisaligned;
			continue;
		}

		++RowsRead;
		const FString CardId = Fields.IsValidIndex(CardIdColumn) ? Fields[CardIdColumn] : FString(TEXT("?"));

		// ⭐ NOTICE — every row through the SHIPPED resolver, ⛔ never a re-implementation of it and
		//   ⛔ never an assumption that the cell is blank. A populated cell is therefore CAUGHT: it
		//   arrives here exactly as LoadStatsAndStart would deliver it.
		const FString RawNoticeCell = Fields[NoticeColumn].TrimStartAndEnd();
		const float RowNoticeUU = FCString::Atof(*RawNoticeCell);
		const float ResolvedNoticeUU =
			ASummonedUnit::ResolveNoticeRadiusUU(ASummonedUnit::UnitEngagementRadiusUU, RowNoticeUU);

		if (!RawNoticeCell.IsEmpty() && RowNoticeUU > 0.f)
		{
			++PopulatedNoticeCells;
			// ⛔ A populated cell must survive the channel UNCHANGED — the anti-clamp AND the
			//   anti-widen property, asserted here against LIVE DATA rather than a synthetic.
			//   ⚠️ Vacuous while the column is entirely sparse (TASK-1004), and that is stated
			//   rather than hidden: the count is reported below so a reader can see it was zero.
			bEveryPopulatedCellSurvivedTheChannel =
				bEveryPopulatedCellSurvivedTheChannel && FMath::IsNearlyEqual(ResolvedNoticeUU, RowNoticeUU, 0.01f);
		}

		if (ResolvedNoticeUU > WidestNoticeUU)
		{
			WidestNoticeUU = ResolvedNoticeUU;
			WidestNoticeCard = CardId;
		}

		// ⭐ FIRING — the `bRanged` rows, exactly as FOG-§9.11 words the law.
		// ⚠️⚠️ AND THE TRAP IN THAT WORDING, HANDLED RATHER THAN INHERITED: `bRanged` is the
		//   PROJECTILE-DELIVERY flag, ⛔ NOT an "is ranged" flag (FOG-§9.8c). ⛔ `CrystalTower`
		//   ships `bRanged=FALSE` at `Range 800` and ⛔ still shoots. ⇒ the law's set is scanned
		//   AS WRITTEN (so the pin matches the law it cites), and the ⛔ COMPLEMENT is scanned too
		//   so the ordering is checked against reach the law's own wording would have missed.
		const float RowRangeUU = FCString::Atof(*Fields[RangeColumn]);
		if (Fields[RangedColumn].TrimStartAndEnd().Equals(TEXT("true"), ESearchCase::IgnoreCase))
		{
			++RangedRows;
			if (RowRangeUU > LongestFiringUU)
			{
				LongestFiringUU = RowRangeUU;
				LongestFiringCard = CardId;
			}
		}
		else if (RowRangeUU > WidestNonRangedReachUU)
		{
			WidestNonRangedReachUU = RowRangeUU;
			WidestNonRangedReachCard = CardId;
		}
	}

	// ── (a) ⭐⭐ THE POSITIVE CONTROLS. ⛔ WITHOUT THESE THE WHOLE TEST IS GREEN ON AN EMPTY READ.
	TestEqual(
		*FString::Printf(
			TEXT("⛔⛔ SELF-CHECK: ZERO cards.csv rows were skipped (%d misaligned of %d data lines). ⛔ The house ")
			TEXT("probe `continue`s past a row whose field count disagrees with the header — a comma inside the ")
			TEXT("free-text Notes column — but skipping is UNSAFE HERE: a skipped row could carry the longest firing ")
			TEXT("range in the game, or a populated NoticeRange cell, and this test would then report a smaller ")
			TEXT("number and pass. ⇒ a misaligned row FAILS instead of being quietly dropped."),
			RowsMisaligned, Lines.Num() - 1),
		RowsMisaligned, 0);

	TestTrue(
		FString::Printf(
			TEXT("⛔⛔ SELF-CHECK (`FOG-§9.8e`): the roster really was READ — %d rows, %d of them `bRanged`, and the ")
			TEXT("longest firing range found is %.1f uu on `%s`. ⛔ That CSV's FIRST COLUMN HEADER IS EMPTY, so a ")
			TEXT("parser that keys the CardID by NAME yields a well-formed table of BLANKS: every number 0, every ")
			TEXT("comparison below trivially true, and a permanently green law. This row is what makes that ")
			TEXT("impossible."),
			RowsRead, RangedRows, LongestFiringUU, *LongestFiringCard),
		RowsRead >= 10 && RangedRows >= 2 && LongestFiringUU > 0.f && !LongestFiringCard.IsEmpty());

	TestTrue(
		FString::Printf(
			TEXT("⛔ SELF-CHECK: the widest RESOLVED notice radius in the roster is %.1f uu (`%s`), which is ")
			TEXT("positive and therefore really came from the resolver rather than from an unread column."),
			WidestNoticeUU, *WidestNoticeCard),
		WidestNoticeUU > 0.f);

	AddInfo(FString::Printf(
		TEXT("Roster derivation: %d rows read, %d bRanged, %d populated NoticeRange cell(s). ")
		TEXT("LEASH %.1f > NOTICE %.1f > LONGEST FIRING %.1f (`%s`)."),
		RowsRead, RangedRows, PopulatedNoticeCells,
		ASummonedUnit::ResolveEffectiveLeashRangeUU(
			UnitDefaults->GetLeashRangeFloorUU(), WidestNoticeUU, UnitDefaults->GetLeashMarginMultiplier()),
		WidestNoticeUU, LongestFiringUU, *LongestFiringCard));

	// ── (b) ⭐⭐ A POPULATED CELL SURVIVES THE CHANNEL, ASSERTED AGAINST LIVE DATA.
	//    ⚠️ VACUOUS TODAY and said so out loud: TASK-1004 blanks the Longbowman's cell, so the
	//    column is entirely sparse and this loop body never ran. The COUNT is reported above so a
	//    reader can see that rather than infer coverage that does not exist. ⭐ The row is kept
	//    because the channel's whole purpose is that a FUTURE card populates a cell, and on the
	//    day one does, this assertion is the one that notices.
	TestTrue(
		FString::Printf(
			TEXT("⭐ Every populated `NoticeRange` cell in the roster (%d of them) survives `ResolveNoticeRadiusUU` ")
			TEXT("UNCHANGED — ⛔ neither clamped down to the default nor widened up to it. ⚠️ DECLARED: at %d ")
			TEXT("populated cells this row is VACUOUS; tests 2(a)/2(b) carry both refusals on values instead."),
			PopulatedNoticeCells, PopulatedNoticeCells),
		bEveryPopulatedCellSurvivedTheChannel);

	// ── (c) ⭐⭐⭐ THE SECOND INEQUALITY: **NOTICE > LONGEST FIRING RANGE.**
	//    ⛔ This is the half FOG-§9.11 uses to RETIRE FOG-§9.10a's identity rule ("notice ==
	//    firing for ranged units"). Every ranged unit must now notice STRICTLY BEYOND its reach —
	//    a unit that fires further than it sees can never acquire what it could hit.
	TestTrue(
		FString::Printf(
			TEXT("⭐⭐⭐ NOTICE (%.1f uu, widest in the roster, from `%s`) > LONGEST FIRING RANGE (%.1f uu, from ")
			TEXT("`%s`). ⛔ BOTH SIDES ARE DERIVED: the left by running every row through the shipped resolver, the ")
			TEXT("right by scanning every `bRanged` row of the shipped CSV. ⚖️ This is what RETIRES the old ")
			TEXT("\"notice == firing for ranged units\" identity rule (FOG-§9.10a ⇒ FOG-§9.11): a unit that fires ")
			TEXT("further than it sees can never acquire what it could hit."),
			WidestNoticeUU, *WidestNoticeCard, LongestFiringUU, *LongestFiringCard),
		WidestNoticeUU > LongestFiringUU);

	// ── (c2) ⭐⭐ THE SAME INEQUALITY AGAINST THE SET THE LAW'S OWN WORDING EXCLUDES (`FOG-§9.8c`).
	//    ⛔ `bRanged` is PROJECTILE DELIVERY, not "is ranged": `CrystalTower` ships `bRanged=FALSE`
	//    at `Range 800` and shoots anyway. ⇒ a pin that trusted the flag would be blind to exactly
	//    the card the convention warns about, and it would be blind SILENTLY.
	TestTrue(
		FString::Printf(
			TEXT("⭐⭐ NOTICE (%.1f uu) also exceeds the widest reach among the rows `bRanged` EXCLUDES (%.1f uu, ")
			TEXT("from `%s`). ⛔ The law is worded around `bRanged`, but that flag means PROJECTILE DELIVERY and not ")
			TEXT("\"is ranged\" (FOG-§9.8c) — so the complement is scanned too, or a `bRanged=false` shooter would ")
			TEXT("be able to out-reach its own eyesight with nothing pointing at it."),
			WidestNoticeUU, WidestNonRangedReachUU,
			WidestNonRangedReachCard.IsEmpty() ? TEXT("none") : *WidestNonRangedReachCard),
		WidestNoticeUU > WidestNonRangedReachUU);

	// ── (d) ⭐⭐⭐ THE FIRST INEQUALITY: **LEASH > NOTICE**, at the roster's widest reach.
	//    ⛔ The leash is DERIVED through the shipped seam from the shipped floor and margin.
	//    ⚠️ DECLARED HONESTLY: a hard-coded `return 8000.f` inside the seam would pass THIS row —
	//    the floor wins today, so this row cannot discriminate a derivation from a constant. ⭐ It
	//    is TEST 3(f) that goes red against that collapse, by pushing a synthetic notice radius
	//    past the derived crossover. ⛔ Two rows, one property; neither alone is sufficient.
	const float EffectiveLeashAtWidestUU = ASummonedUnit::ResolveEffectiveLeashRangeUU(
		UnitDefaults->GetLeashRangeFloorUU(), WidestNoticeUU, UnitDefaults->GetLeashMarginMultiplier());

	TestTrue(
		FString::Printf(
			TEXT("⭐⭐⭐ LEASH (%.1f uu) > NOTICE (%.1f uu). ⛔ The leash is DERIVED — `max(floor %.1f, notice %.1f × ")
			TEXT("margin %.2f)` — ⛔ never typed. Without this the drop site releases a target and `AcquireTarget` ")
			TEXT("re-takes it in the SAME 0.25 s poll and units never disengage (FOG-§9.8b)."),
			EffectiveLeashAtWidestUU, WidestNoticeUU, UnitDefaults->GetLeashRangeFloorUU(), WidestNoticeUU,
			UnitDefaults->GetLeashMarginMultiplier()),
		EffectiveLeashAtWidestUU > WidestNoticeUU);

	// ── (e) ⭐⭐⭐ THE ORDERING HOLDS FOR **EVERY** ROW, NOT ONLY THE WIDEST ONE — re-derived
	//    per card, so a future roster where one card carries a huge notice cell and another
	//    carries the longest gun cannot slip between the two maxima above.
	int32 RowsFailingOrdering = 0;
	FString FirstFailingCard;
	for (int32 Index = 1; Index < Lines.Num(); ++Index)
	{
		TArray<FString> Fields;
		Lines[Index].ParseIntoArray(Fields, TEXT(","), /*InCullEmpty=*/ false);
		if (Fields.Num() != HeaderFields.Num())
		{
			continue; // already counted and FAILED in (a); not double-reported here
		}

		const float ResolvedNoticeUU = ASummonedUnit::ResolveNoticeRadiusUU(
			ASummonedUnit::UnitEngagementRadiusUU, FCString::Atof(*Fields[NoticeColumn].TrimStartAndEnd()));
		const float RowLeashUU = ASummonedUnit::ResolveEffectiveLeashRangeUU(
			UnitDefaults->GetLeashRangeFloorUU(), ResolvedNoticeUU, UnitDefaults->GetLeashMarginMultiplier());

		if (!(RowLeashUU > ResolvedNoticeUU))
		{
			++RowsFailingOrdering;
			if (FirstFailingCard.IsEmpty())
			{
				FirstFailingCard = Fields.IsValidIndex(CardIdColumn) ? Fields[CardIdColumn] : FString(TEXT("?"));
			}
		}
	}

	TestEqual(
		*FString::Printf(
			TEXT("⭐⭐⭐ `LEASH > NOTICE` holds for ⛔ EVERY ONE of the %d roster rows, re-derived per card (%d fail; ")
			TEXT("first would be `%s`). ⛔ The two maxima in (c)/(d) are taken from DIFFERENT rows in general, so ")
			TEXT("comparing only maxima could pass while an individual card inverted. ⭐ This is the property, ⛔ not ")
			TEXT("today's arithmetic."),
			RowsRead, RowsFailingOrdering, FirstFailingCard.IsEmpty() ? TEXT("none") : *FirstFailingCard),
		RowsFailingOrdering, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
