// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
//~ TASK-830 item (8): tests 31/32 CALL the cast surface on class default objects rather than only
//~ reading it as source text, so they need the complete provider types. ⛔ Everything above this
//~ line is still a pure-rules / source-probe lane — no world, no actor, no SpawnActor (the house
//~ rule), and a CDO is precisely the world-less caller the defaulted virtuals must survive.
#include "Siegebound/Building.h"
#include "Siegebound/HealthBarProvider.h"
#include "Siegebound/HeroCharacter.h"
#include "Siegebound/SiegeInvisibilityStatics.h"
#include "Siegebound/SummonedUnit.h"
#include "Siegebound/TeamId.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE VEIL — the pure invisibility rules (TASK-827, WITCH-§3/§6) ═══
 *
 *  Jonathan, verbatim (CONVENTIONS "THE WITCH + INVISIBILITY"):
 *
 *    "Invisible units will remain invisble until they attack/heal/mine/power up something
 *     (basically if they do anything other than walk), to which then they permanently go back
 *     to visible (unless they are later made invisible by a witch again)."
 *
 *  ⭐ TASK-827's spec calls the tests THE DELIVERABLE, ⛔ not a rider — because this file ships
 *  ⛔ zero behaviour. Nothing in the game consults FSiegeInvisibilityStatics yet; the ONLY thing
 *  standing between these rules and a silent drift is this suite.
 *
 *  ── ⛔⛔ WHY A NEW FILE (the "extend, never a parallel new frame unless none fits" law) ──────
 *  Named by TASK-827 and WITCH-§6 as the frame for the whole batch: TASK-829 extends it with the
 *  wired cases (not-acquired · still-caught-by-AoE · still-visible-to-own-team · each reason
 *  clears exactly once) and TASK-830 with the cast cases. No shipped Tests/ file is scoped to
 *  invisibility, and folding the veil into one of the combat frames would put the batch's whole
 *  regression surface somewhere a reader would never look for it.
 *
 *  ── ⛔⛔ THE ⛔ CONTROL DISCIPLINE, STATED ONCE AND APPLIED EVERYWHERE BELOW ────────────────
 *  ⚠️ A veil-break assertion PASSES TRIVIALLY IF THE UNIT WAS ⛔ NEVER VEILED. `TestFalse(flag)`
 *  on a flag that started false is a test that can ⛔ never fail. So every break test here does
 *  ⛔ BOTH of:
 *    (1) ⭐ ASSERTS THE VEIL IS ACTUALLY ON before breaking it — the precondition is checked,
 *        ⛔ never assumed (the SiegeCastleTransformTest "assert the precondition" discipline);
 *    (2) ⭐ CARRIES A ⛔ CONTROL VEIL — a second bool, veiled at the same moment and ⛔ never
 *        passed to ApplyBreak — asserted STILL TRUE at the end. ⇒ an implementation that
 *        clobbers every veil, or one where ApplyVeil silently no-ops, ⛔ CANNOT pass.
 *
 *  ── WHAT THIS FILE DOES ⛔ NOT COVER (SC-§32 — green here is ⛔ NOT "invisibility works") ────
 *  ⛔ Pure rules only. ⛔ No world, ⛔ no actor, ⛔ no acquisition, ⛔ no rendering. That a veiled
 *  unit is actually skipped by GatherHostileAgents is TASK-829's; that a witch casts for 3 s is
 *  TASK-830's; that the veil LOOKS refracted is WITCH-§5's material and is a PIXEL question no
 *  automation test can answer.
 */

namespace SiegeInvisibilityTestFixture
{
	/**
	 *  The six enumerators of the WITCH-§3 closed set, listed literally so the census test compares
	 *  against a hand-written list rather than against the enum it is supposed to be policing.
	 *  ⛔ Keep in declaration order — the census test indexes by cast.
	 */
	static const ESiegeVeilBreakReason AllBreakReasons[] =
	{
		ESiegeVeilBreakReason::Attack,
		ESiegeVeilBreakReason::Heal,
		ESiegeVeilBreakReason::Mine,
		ESiegeVeilBreakReason::Empower,
		ESiegeVeilBreakReason::Cast,
		ESiegeVeilBreakReason::Death
	};

	/** The expected ToString token for each of the six, in the same order. */
	static const TCHAR* const AllBreakReasonNames[] =
	{
		TEXT("Attack"),
		TEXT("Heal"),
		TEXT("Mine"),
		TEXT("Empower"),
		TEXT("Cast"),
		TEXT("Death")
	};

	/**
	 *  ⛔⛔ THE NAMES THAT MUST ⛔ NEVER APPEAR. WITCH-§3 refuses these three by name: walking is
	 *  his explicit carve-out, and taking damage / being ordered are things done TO a unit — his
	 *  rule breaks the veil on ACTING. The census test greps the shipped names for them so the
	 *  refusal is enforced, ⛔ not merely commented.
	 */
	static const TCHAR* const ForbiddenReasonNames[] =
	{
		TEXT("Walk"),
		TEXT("TakeDamage"),
		TEXT("Order")
	};

	static constexpr int32 AllBreakReasonsNum = UE_ARRAY_COUNT(AllBreakReasons);
	static constexpr int32 AllBreakReasonNamesNum = UE_ARRAY_COUNT(AllBreakReasonNames);
	static constexpr int32 ForbiddenReasonNamesNum = UE_ARRAY_COUNT(ForbiddenReasonNames);

	/** One row of the visibility truth table: the three inputs and the HAND-WRITTEN expectation. */
	struct FVisibilityRow
	{
		ETeamId ViewerTeam;
		ETeamId TargetTeam;
		bool bTargetIsInvisible;
		bool bExpectedVisible;
		const TCHAR* Label;
	};

	/**
	 *  ⭐⭐ ALL 2×2×2 INPUT COMBINATIONS, WITH THE EXPECTATION WRITTEN AS A ⛔ LITERAL.
	 *  ⛔ Deliberately NOT computed as `(Viewer == Target) || !bInvisible` — re-deriving the
	 *  formula under test asserts only that the author typed it twice. These eight booleans were
	 *  read off WITCH-§2's lane table by hand.
	 */
	static const FVisibilityRow VisibilityTruthTable[] =
	{
		{ ETeamId::Blue, ETeamId::Blue, false, true,  TEXT("Blue viewer, Blue target, unveiled  -> VISIBLE (ordinary ally)") },
		{ ETeamId::Blue, ETeamId::Blue, true,  true,  TEXT("Blue viewer, Blue target, VEILED    -> VISIBLE (WITCH-2 lane 4: own team ALWAYS sees)") },
		{ ETeamId::Blue, ETeamId::Red,  false, true,  TEXT("Blue viewer, Red  target, unveiled  -> VISIBLE (ordinary enemy)") },
		{ ETeamId::Blue, ETeamId::Red,  true,  false, TEXT("Blue viewer, Red  target, VEILED    -> HIDDEN  (THE FEATURE)") },
		{ ETeamId::Red,  ETeamId::Blue, false, true,  TEXT("Red  viewer, Blue target, unveiled  -> VISIBLE (ordinary enemy)") },
		{ ETeamId::Red,  ETeamId::Blue, true,  false, TEXT("Red  viewer, Blue target, VEILED    -> HIDDEN  (THE FEATURE, mirrored)") },
		{ ETeamId::Red,  ETeamId::Red,  false, true,  TEXT("Red  viewer, Red  target, unveiled  -> VISIBLE (ordinary ally)") },
		{ ETeamId::Red,  ETeamId::Red,  true,  true,  TEXT("Red  viewer, Red  target, VEILED    -> VISIBLE (WITCH-2 lane 4, mirrored)") }
	};

	static constexpr int32 VisibilityTruthTableNum = UE_ARRAY_COUNT(VisibilityTruthTable);

	/** Both teams, for tests that must prove a rule is symmetric rather than accidentally Blue-shaped. */
	static const ETeamId BothTeams[] = { ETeamId::Blue, ETeamId::Red };
	static constexpr int32 BothTeamsNum = UE_ARRAY_COUNT(BothTeams);

	static const TCHAR* TeamName(ETeamId Team)
	{
		return (Team == ETeamId::Blue) ? TEXT("Blue") : TEXT("Red");
	}

	// ═════════════════════════════════════════════════════════════════════════════════════════
	//  TASK-829 — THE ⛔ WIRING LANE. Everything below this line is about ⛔ WHERE the rules
	//  above are CALLED FROM, which is the ⛔ only thing that can distinguish a correct
	//  implementation of this feature from the obvious wrong one.
	//
	//  ⚠️⚠️ WHY SOURCE-TEXT PROBES AND ⛔ NOT A SPAWNED FIXTURE, STATED SO THE GAP IS HONEST:
	//  there is ⛔ not one `UWorld::CreateWorld` and ⛔ not one `SpawnActor` anywhere in
	//  `Siegebound/Tests/` (the house rule, `SiegeLadderClimbTest.cpp:39`). A veil break is an
	//  ACTOR event on a TIMER inside a WORLD, so it is ⛔ unreachable headlessly. ⇒ the wiring is
	//  pinned STRUCTURALLY: the break call is present at the measured site, absent at the
	//  measured non-site, and ordered correctly relative to the code around it.
	//  ⛔ NONE of that is a substitute for a PIE pass with a witch on the field, and ⛔ green here
	//  must ⛔ never be reported as "invisibility works" (`SC-§32`).
	//
	//  ⭐⭐ AND THE ⛔ DISCRIMINATION RULE THIS LANE IS BUILT ON (`TASK-829(6a)`): asserting that a
	//  break EXISTS is nearly worthless — the obvious wrong implementation has breaks too. Every
	//  trap test below therefore asserts a ⛔ PAIR: the break IS at the right site ⛔ AND is ⛔ NOT
	//  at the wrong one. It is the ⛔ second half that fails a naive wiring.
	// ═════════════════════════════════════════════════════════════════════════════════════════

	/** Shipping source (⛔ NOT `Tests/`) — the files this feature is wired into. */
	const TCHAR* SummonedUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp");
	const TCHAR* SummonedUnitHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h");
	const TCHAR* MinerUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp");
	const TCHAR* AncientGroundCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/AncientGround.cpp");
	const TCHAR* CombatStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp");
	const TCHAR* CombatStaticsHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h");
	const TCHAR* CaptureZoneCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/CaptureZone.cpp");
	const TCHAR* PlayerStateCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp");
	const TCHAR* GoldNodeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/GoldNode.cpp");
	const TCHAR* TowerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/Tower.cpp");
	const TCHAR* HeroCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp");
	/** TASK-851 (WITCH-§8): the bot's brain — the ONE acquisition surface that is NOT behind the funnel. */
	const TCHAR* BotControllerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp");

	/** Reads a shipped project source file. ⛔ A probe that cannot read its subject FAILS. */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
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
	 *  True when a TRIMMED line is a comment line rather than code — the house rule, copied
	 *  verbatim from `SiegeAcquisitionFunnelTest` / `SiegeClimbableTowerTest` so all of them agree.
	 *  ⚠️ `*` is qualified rather than bare on purpose: a doc-comment continuation is `* text`,
	 *  while `*GetNameSafe(Foo)` starts a CODE line with the same character.
	 */
	static bool IsCommentLine(const FString& Trimmed)
	{
		return Trimmed.StartsWith(TEXT("//"), ESearchCase::CaseSensitive)
			|| Trimmed.StartsWith(TEXT("* "), ESearchCase::CaseSensitive)
			|| Trimmed.StartsWith(TEXT("*/"), ESearchCase::CaseSensitive)
			|| Trimmed.StartsWith(TEXT("/*"), ESearchCase::CaseSensitive)
			|| Trimmed.Equals(TEXT("*"), ESearchCase::CaseSensitive);
	}

	/**
	 *  Occurrences of Needle on CODE lines only.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FEATURE ABOVE ALL OTHERS. Every call site TASK-829 touched
	 *  carries a long trap comment that ⛔ NAMES the wrong wiring point it is refusing — the
	 *  `AddPermanentDamageStacks` loop says "⛔ do not add a BreakInvisibility here" in prose,
	 *  three lines above the loop it is protecting. A scanner that counted comments would report
	 *  that refusal as the defect it exists to prevent.
	 *  ⚖️ The prose is the guard. The instrument has to be the one that gets smarter.
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
			if (IsCommentLine(Trimmed))
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
	 *  The same text with every comment LINE removed, so an ORDERING probe (`Find(A) < Find(B)`)
	 *  cannot be fooled by a comment that names the token it is refusing. ⛔ Required here: the
	 *  ancient-ground and miner traps are both asserted by ORDER, and both sites carry prose
	 *  naming the other half of the trap.
	 */
	static FString CodeLinesOnly(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		FString Out;
		Out.Reserve(Source.Len());
		for (const FString& Line : Lines)
		{
			if (IsCommentLine(Line.TrimStart()))
			{
				continue;
			}
			Out += Line;
			Out += TEXT("\n");
		}
		return Out;
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace —
	 *  how every free/member function in this codebase ends. ⛔ Deliberately NOT a parser: a
	 *  signature that stops matching FAILS rather than silently scanning nothing, which is the
	 *  whole reason `SC-§38` (locate by SYMBOL, never by line) is safe to obey mechanically.
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

	/** Every `.h`/`.cpp` under `Source/`, absolute paths. */
	static void FindAllSourceFiles(TArray<FString>& OutFiles)
	{
		OutFiles.Reset();
		const FString SourceRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source"));
		IFileManager::Get().FindFilesRecursive(OutFiles, *SourceRoot, TEXT("*.cpp"), /*Files=*/ true, /*Directories=*/ false, /*bClearFileNames=*/ false);
		IFileManager::Get().FindFilesRecursive(OutFiles, *SourceRoot, TEXT("*.h"), /*Files=*/ true, /*Directories=*/ false, /*bClearFileNames=*/ false);
	}

	/** True when Path lives under `Siegebound/Tests/` — the automation lane, not shipping code. */
	static bool IsAutomationTestFile(const FString& Path)
	{
		FString Normalized = Path;
		Normalized.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Normalized.Contains(TEXT("/Tests/"), ESearchCase::CaseSensitive);
	}

	/**
	 *  Counts Needle across every SHIPPING source file (⛔ `Tests/` excluded), and records WHERE.
	 *  ⛔ Tests are excluded because this very file quotes each forbidden token as a string
	 *  literal on a code line — a scan that included itself would count its own accusations.
	 *  ⛔ An unreadable file FAILS the scan rather than being skipped.
	 */
	static int32 CountAcrossShippingSource(FAutomationTestBase& Test, const TCHAR* Needle, FString& OutWhere)
	{
		OutWhere.Reset();

		TArray<FString> SourceFiles;
		FindAllSourceFiles(SourceFiles);

		// ── SELF-CHECK: a scan that found no files would pass every count below vacuously.
		if (SourceFiles.Num() < 20)
		{
			Test.AddError(FString::Printf(
				TEXT("SELF-CHECK FAILED: the recursive scan of Source/ found only %d file(s). The instrument is ")
				TEXT("dead and every count taken from it would be a meaningless zero."), SourceFiles.Num()));
			return -1;
		}

		int32 Total = 0;
		for (const FString& File : SourceFiles)
		{
			if (IsAutomationTestFile(File))
			{
				continue;
			}

			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *File))
			{
				Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — an unreadable file FAILS the gate rather than being skipped."), *File));
				return -1;
			}

			const int32 Hits = CountOccurrencesInCode(Text, Needle);
			if (Hits > 0)
			{
				Total += Hits;
				OutWhere += FString::Printf(TEXT("\n    %d x  %s"), Hits, *FPaths::GetCleanFilename(File));
			}
		}
		return Total;
	}
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  1. THE PREDICATE — every input combination, against a hand-written expectation.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityVisibilityPredicateTruthTableTest,
	"Siegebound.Invisibility.VisibilityPredicateTruthTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityVisibilityPredicateTruthTableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⭐ THE ANTI-DEGENERACY GUARD, FIRST. A truth table whose expectations were all-true (or
	// all-false) would "pass" against a constant-return implementation. Pinning the split means
	// an edit to the TABLE ITSELF that flattens it fails here rather than quietly weakening
	// every row below.
	int32 ExpectedVisibleCount = 0;
	for (int32 Index = 0; Index < VisibilityTruthTableNum; ++Index)
	{
		if (VisibilityTruthTable[Index].bExpectedVisible)
		{
			++ExpectedVisibleCount;
		}
	}
	TestEqual(TEXT("The truth table covers all 2x2x2 input combinations"), VisibilityTruthTableNum, 8);
	TestEqual(TEXT("The truth table is NOT degenerate: exactly SIX rows expect VISIBLE"), ExpectedVisibleCount, 6);
	TestEqual(TEXT("The truth table is NOT degenerate: exactly TWO rows expect HIDDEN (the two cross-team veiled rows)"),
		VisibilityTruthTableNum - ExpectedVisibleCount, 2);

	for (int32 Index = 0; Index < VisibilityTruthTableNum; ++Index)
	{
		const FVisibilityRow& Row = VisibilityTruthTable[Index];
		const bool bActual = FSiegeInvisibilityStatics::IsVisibleTo(Row.ViewerTeam, Row.TargetTeam, Row.bTargetIsInvisible);

		// TestTrue/TestFalse rather than TestEqual on bools: bool converts to int32/int64/SIZE_T/
		// float/double, all of which FAutomationTestBase overloads, so TestEqual on a pair of
		// bools is a needless overload-resolution question in a file nobody may compile today.
		if (Row.bExpectedVisible)
		{
			TestTrue(Row.Label, bActual);
		}
		else
		{
			TestFalse(Row.Label, bActual);
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  2. THE ONE A CARELESS IMPLEMENTATION GETS WRONG — WITCH-§2's fourth lane, on its own.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityFriendlyVeilNeverSuppressedTest,
	"Siegebound.Invisibility.FriendlyVeilNeverSuppressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityFriendlyVeilNeverSuppressedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔⛔ WITCH-§2, FOURTH LANE: "An invisible unit that its own player cannot select, order or
	// heal is a BUG, not a feature." This case is covered by row 2 and row 8 of the truth table
	// above — it is asserted AGAIN, alone, on purpose: a failure here names the actual defect in
	// one line instead of surfacing as "row 2 of some table", and TASK-827's spec requires it be
	// asserted explicitly.
	//
	// ⭐ THE FAILURE THIS EXISTS TO CATCH: `return !bTargetIsInvisible` — the one-liner that looks
	// obviously right, ships a unit its owner cannot command, and passes every enemy-side test.
	for (int32 TeamIndex = 0; TeamIndex < BothTeamsNum; ++TeamIndex)
	{
		const ETeamId Team = BothTeams[TeamIndex];

		TestTrue(FString::Printf(TEXT("A VEILED %s unit is STILL VISIBLE to a %s viewer (own team) — the WITCH-2 lane-4 bug guard"),
			TeamName(Team), TeamName(Team)),
			FSiegeInvisibilityStatics::IsVisibleTo(Team, Team, /*bTargetIsInvisible*/ true));

		// The paired control: the SAME viewer/target teams with the veil OFF must also be visible.
		// Without it, a broken `return true` for allies would look like a pass above; with it,
		// both rows are pinned and only the correct implementation satisfies the pair.
		TestTrue(FString::Printf(TEXT("An UNVEILED %s unit is visible to a %s viewer (the paired control)"),
			TeamName(Team), TeamName(Team)),
			FSiegeInvisibilityStatics::IsVisibleTo(Team, Team, /*bTargetIsInvisible*/ false));
	}

	// ⭐ AND THE DISCRIMINATOR: the same veil flag on a CROSS-team target MUST hide. If this
	// failed, the "friendly always visible" assertions above would be satisfied by an
	// implementation that simply returns true for everything — which is not a veil at all.
	TestFalse(TEXT("The SAME veil flag on a CROSS-team target HIDES — proving the friendly lane is a lane, not a constant true"),
		FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Blue, ETeamId::Red, /*bTargetIsInvisible*/ true));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  3. THE GRANT — the one write-true site.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityVeilGrantIsIdempotentTest,
	"Siegebound.Invisibility.VeilGrantIsIdempotentAndReportsTheEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityVeilGrantIsIdempotentTest::RunTest(const FString& Parameters)
{
	bool bIsInvisible = false;

	TestTrue(TEXT("The FIRST ApplyVeil reports the true edge (it did the veiling)"),
		FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible));
	TestTrue(TEXT("...and the flag is now set"), bIsInvisible);

	// WITCH-§4's "never target an already-invisible unit" belt: a second grant is a MEASURABLE
	// no-op, so TASK-830 can distinguish "I veiled someone" from "I wasted a cast".
	TestFalse(TEXT("A SECOND ApplyVeil on an already-veiled unit reports NO edge"),
		FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible));
	TestTrue(TEXT("...and the flag is still set (a redundant grant never un-veils)"), bIsInvisible);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  4. ⭐⭐ "PERMANENTLY" — the break happens EXACTLY ONCE and NOTHING restores it.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBreakIsExactlyOnceTest,
	"Siegebound.Invisibility.BreakIsExactlyOnceAndNeverSelfRestores",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBreakIsExactlyOnceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	bool bSubjectVeil = false;
	bool bControlVeil = false;

	// ── (2) THE CONTROL: veiled at the same moment, and ⛔ NEVER passed to ApplyBreak.
	FSiegeInvisibilityStatics::ApplyVeil(bControlVeil);
	FSiegeInvisibilityStatics::ApplyVeil(bSubjectVeil);

	// ── (1) THE PRECONDITION, ASSERTED RATHER THAN ASSUMED. Without this, every TestFalse below
	//        would pass against a no-op ApplyVeil — the trivial-pass failure mode this file's
	//        header names. If ApplyVeil is broken, the suite fails HERE with a clear message.
	if (!bSubjectVeil || !bControlVeil)
	{
		AddError(TEXT("PRECONDITION FAILED: ApplyVeil did not set the flag, so no break assertion below could fail. Fix ApplyVeil first."));
		return false;
	}

	// The break edge fires exactly once.
	TestTrue(TEXT("The FIRST ApplyBreak reports the true->false edge"),
		FSiegeInvisibilityStatics::ApplyBreak(bSubjectVeil, ESiegeVeilBreakReason::Attack));
	TestFalse(TEXT("...and the veil is gone"), bSubjectVeil);

	// ⛔ PERMANENTLY: repeated breaks, with DIFFERENT reasons, never restore and never re-fire.
	// A different reason each time is deliberate — it also proves ApplyBreak does not branch on
	// Reason (a per-reason condition would be the back door that re-opens WITCH-§3's closed set).
	for (int32 Index = 0; Index < AllBreakReasonsNum; ++Index)
	{
		const ESiegeVeilBreakReason Reason = AllBreakReasons[Index];
		TestFalse(FString::Printf(TEXT("A repeat ApplyBreak(%s) on an already-broken veil reports NO edge"),
			FSiegeInvisibilityStatics::ToString(Reason)),
			FSiegeInvisibilityStatics::ApplyBreak(bSubjectVeil, Reason));
		TestFalse(FString::Printf(TEXT("...and the veil STAYS broken after ApplyBreak(%s) — no self-restore, no cooldown"),
			FSiegeInvisibilityStatics::ToString(Reason)),
			bSubjectVeil);
	}

	// ── (2) THE CONTROL, CHECKED LAST: an implementation that clobbers every veil, or one where
	//        the break leaks across units, cannot reach this line green.
	TestTrue(TEXT("CONTROL: the untouched veil is STILL SET after seven breaks on its neighbour"), bControlVeil);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  5. EVERY reason in the closed set actually breaks — each on a freshly veiled subject.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityEveryBreakReasonClearsTheVeilTest,
	"Siegebound.Invisibility.EveryBreakReasonClearsTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityEveryBreakReasonClearsTheVeilTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// The control is veiled ONCE, before the loop, and survives all six iterations untouched.
	bool bControlVeil = false;
	FSiegeInvisibilityStatics::ApplyVeil(bControlVeil);
	if (!bControlVeil)
	{
		AddError(TEXT("PRECONDITION FAILED: ApplyVeil did not set the control flag — no assertion below could fail."));
		return false;
	}

	for (int32 Index = 0; Index < AllBreakReasonsNum; ++Index)
	{
		const ESiegeVeilBreakReason Reason = AllBreakReasons[Index];
		const TCHAR* ReasonName = FSiegeInvisibilityStatics::ToString(Reason);

		// FRESH subject per reason — so reason N's verdict cannot be inherited from reason N-1.
		bool bSubjectVeil = false;
		FSiegeInvisibilityStatics::ApplyVeil(bSubjectVeil);

		// The per-iteration precondition: this reason's assertion is only meaningful if the
		// subject was genuinely veiled a line earlier.
		TestTrue(FString::Printf(TEXT("PRECONDITION for %s: the subject IS veiled before the break"), ReasonName), bSubjectVeil);
		if (!bSubjectVeil)
		{
			continue; // a trivially-passing break assertion is worse than a skipped one
		}

		TestTrue(FString::Printf(TEXT("ApplyBreak(%s) reports the edge — %s is a REAL member of the WITCH-3 closed set"), ReasonName, ReasonName),
			FSiegeInvisibilityStatics::ApplyBreak(bSubjectVeil, Reason));
		TestFalse(FString::Printf(TEXT("ApplyBreak(%s) CLEARED the veil"), ReasonName), bSubjectVeil);

		// The veil is broken; the enemy can see it again. This ties the state transition to the
		// PREDICATE, so a break that clears the flag without changing visibility cannot pass.
		TestTrue(FString::Printf(TEXT("After %s, the enemy CAN see the unit again (the predicate agrees with the flag)"), ReasonName),
			FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Red, ETeamId::Blue, bSubjectVeil));
	}

	TestTrue(TEXT("CONTROL: the untouched veil survived all six break reasons"), bControlVeil);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  6. Breaking an unveiled unit is inert — there is no "un-break".
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBreakOnUnveiledIsNoOpTest,
	"Siegebound.Invisibility.BreakOnAnUnveiledUnitIsANoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBreakOnUnveiledIsNoOpTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// Every unit in the game is unveiled for its whole life unless a witch acts, so ApplyBreak
	// will be called on unveiled units constantly once TASK-829 wires the six sites. It must be
	// free of side effects and must never flip a flag ON.
	for (int32 Index = 0; Index < AllBreakReasonsNum; ++Index)
	{
		const ESiegeVeilBreakReason Reason = AllBreakReasons[Index];
		bool bIsInvisible = false;

		TestFalse(FString::Printf(TEXT("ApplyBreak(%s) on an unveiled unit reports NO edge"),
			FSiegeInvisibilityStatics::ToString(Reason)),
			FSiegeInvisibilityStatics::ApplyBreak(bIsInvisible, Reason));
		TestFalse(FString::Printf(TEXT("ApplyBreak(%s) on an unveiled unit leaves it unveiled — there is no 'un-break'"),
			FSiegeInvisibilityStatics::ToString(Reason)),
			bIsInvisible);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  7. ⭐⭐ THE ABSENCE GATE — the closed set stays closed.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBreakReasonCensusIsClosedTest,
	"Siegebound.Invisibility.BreakReasonCensusIsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBreakReasonCensusIsClosedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔⛔ THIS TEST IS THE REASON THE ABSENCE COMMENT IN THE HEADER HAS TEETH.
	// WITCH-§3's set is CLOSED at six. Walk / TakeDamage / Order are DELIBERATELY absent, and a
	// future reader "helpfully" adding one is the named failure mode. A comment cannot stop that;
	// a red test can. If you are here because this test failed after you added an enumerator:
	// ⛔ STOP. Adding a break reason is a DESIGN CHANGE that goes back through the manager and
	// amends WITCH-§3 — it is not an edit.
	TestEqual(TEXT("The WITCH-3 closed break-set holds exactly SIX reasons"),
		FSiegeInvisibilityStatics::VeilBreakReasonCount, 6);
	TestEqual(TEXT("The test fixture lists all six"), AllBreakReasonsNum, FSiegeInvisibilityStatics::VeilBreakReasonCount);
	TestEqual(TEXT("The fixture's name list matches its reason list"), AllBreakReasonNamesNum, AllBreakReasonsNum);

	// Every declared index maps to its expected name, in declaration order. This catches a
	// REORDER (which would silently re-label every log line TASK-829 writes) as well as a rename.
	for (int32 Index = 0; Index < AllBreakReasonsNum; ++Index)
	{
		const ESiegeVeilBreakReason ByCast = static_cast<ESiegeVeilBreakReason>(Index);
		TestEqual(FString::Printf(TEXT("Enumerator at index %d is '%s' (declaration order is pinned)"), Index, AllBreakReasonNames[Index]),
			FString(FSiegeInvisibilityStatics::ToString(ByCast)), FString(AllBreakReasonNames[Index]));
		TestEqual(FString::Printf(TEXT("The fixture's reason #%d agrees with the cast-by-index value"), Index),
			FString(FSiegeInvisibilityStatics::ToString(AllBreakReasons[Index])), FString(AllBreakReasonNames[Index]));
	}

	// ⭐⭐ THE TRIPWIRE ITSELF: one PAST the end must still be unrecognised. Add a seventh
	// enumerator — even WITH a matching ToString arm — and this line goes red, because index 6
	// would then return a real name.
	const ESiegeVeilBreakReason OnePastTheEnd =
		static_cast<ESiegeVeilBreakReason>(FSiegeInvisibilityStatics::VeilBreakReasonCount);
	TestEqual(TEXT("ONE PAST THE END is unrecognised — a SEVENTH break reason has NOT been added (see WITCH-3 before you 'fix' this)"),
		FString(FSiegeInvisibilityStatics::ToString(OnePastTheEnd)),
		FString(FSiegeInvisibilityStatics::UnrecognisedReasonToken()));

	// And the three refusals by name: walking is his carve-out; taking damage and being ordered
	// are things done TO a unit. None of them may ever be a break reason.
	for (int32 NameIndex = 0; NameIndex < AllBreakReasonsNum; ++NameIndex)
	{
		const FString ShippedName(FSiegeInvisibilityStatics::ToString(AllBreakReasons[NameIndex]));
		for (int32 ForbiddenIndex = 0; ForbiddenIndex < ForbiddenReasonNamesNum; ++ForbiddenIndex)
		{
			TestNotEqual(FString::Printf(TEXT("No break reason is named '%s' — WITCH-3 refuses it BY NAME (his rule breaks on ACTING, not on being acted upon)"),
				ForbiddenReasonNames[ForbiddenIndex]),
				ShippedName, FString(ForbiddenReasonNames[ForbiddenIndex]));
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  8. ⭐ HIS PARENTHETICAL — "(unless they are later made invisible by a witch again)".
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityFreshCastReVeilsTest,
	"Siegebound.Invisibility.AFreshWitchCastReVeilsAfterABreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityFreshCastReVeilsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⭐ "Permanently" is not "forever" — it means the veil never restores ITSELF. A NEW witch
	// cast may re-veil the same unit, and WITCH-§4's J-W7 says a witch who veils A then B leaves
	// BOTH invisible. This test walks one unit through the full cycle and checks the PREDICATE at
	// every stage, so a flag that flips without changing visibility cannot pass.
	bool bIsInvisible = false;
	bool bControlVeil = false;
	FSiegeInvisibilityStatics::ApplyVeil(bControlVeil);

	// Stage 1 — veiled by the first cast.
	TestTrue(TEXT("Stage 1: the first cast veils the unit"), FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible));
	if (!bIsInvisible)
	{
		AddError(TEXT("PRECONDITION FAILED: the first ApplyVeil did not set the flag — the re-veil cycle cannot be tested."));
		return false;
	}
	TestFalse(TEXT("Stage 1: the enemy CANNOT see it"),
		FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Red, ETeamId::Blue, bIsInvisible));
	TestTrue(TEXT("Stage 1: its OWN team can (WITCH-2 lane 4 holds through the cycle)"),
		FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Blue, ETeamId::Blue, bIsInvisible));

	// Stage 2 — it attacks, so it permanently goes back to visible.
	TestTrue(TEXT("Stage 2: attacking breaks the veil"),
		FSiegeInvisibilityStatics::ApplyBreak(bIsInvisible, ESiegeVeilBreakReason::Attack));
	TestTrue(TEXT("Stage 2: the enemy CAN see it again"),
		FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Red, ETeamId::Blue, bIsInvisible));

	// Stage 3 — a SECOND witch cast re-veils the same unit. This is the ONLY thing that may.
	TestTrue(TEXT("Stage 3: a FRESH witch cast re-veils the unit ('unless they are later made invisible by a witch again')"),
		FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible));
	TestFalse(TEXT("Stage 3: the enemy CANNOT see it again"),
		FSiegeInvisibilityStatics::IsVisibleTo(ETeamId::Red, ETeamId::Blue, bIsInvisible));

	// Stage 4 — and the second veil is just as breakable as the first (no accumulated immunity).
	TestTrue(TEXT("Stage 4: the SECOND veil breaks on the very next act, exactly like the first"),
		FSiegeInvisibilityStatics::ApplyBreak(bIsInvisible, ESiegeVeilBreakReason::Heal));
	TestFalse(TEXT("Stage 4: and it is gone"), bIsInvisible);

	TestTrue(TEXT("CONTROL: the neighbouring veil was untouched by the whole cycle"), bControlVeil);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
// ═══  TASK-829 — THE WIRING. Everything above proved the RULES; everything below proves    ═══
// ═══  the rules are CALLED FROM THE RIGHT PLACES, which is the only half a player feels.   ═══
// ═════════════════════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  9. ⭐⭐ ONE FLAG, ONE DOOR — the WITCH-§6 gate, shipped as a scan rather than a comment.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityOneFlagOneDoorTest,
	"Siegebound.Invisibility.TheVeilHasOneSourceOfTruthAndOneWriteDoor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityOneFlagOneDoorTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔⛔ WITCH-§6, VERBATIM: "⛔ An inlined `bIsInvisible = false` ANYWHERE is an automatic QA
	// FAIL", and "⛔ ONE source of truth, ⛔ no second copy, ⛔ no mirrored bool on any other
	// class". Those are the two claims this test turns into a red line.
	//
	// ⭐ WHY A WHOLE-TREE SCAN RATHER THAN A LOOK AT THE THREE FILES WE EDITED: the failure this
	// guards is a FUTURE one. A mirrored `bIsInvisible` on ATower, on AHeroCharacter, or on
	// ASiegeBotController (TASK-851 is about to edit that file) would compile, review cleanly,
	// pass every test above, and produce a veil that disagrees with itself.
	TArray<FString> SourceFiles;
	FindAllSourceFiles(SourceFiles);

	if (SourceFiles.Num() < 20)
	{
		AddError(FString::Printf(
			TEXT("SELF-CHECK FAILED: the recursive scan of Source/ found only %d file(s) — every count below ")
			TEXT("would be a meaningless zero."), SourceFiles.Num()));
		return false;
	}

	// The ONLY four shipping files that may name the flag at all: the pure rules that mutate it
	// through a reference, and the ONE actor class that owns it.
	const TCHAR* const AllowedFlagFiles[] =
	{
		TEXT("SiegeInvisibilityStatics.h"),
		TEXT("SiegeInvisibilityStatics.cpp"),
		TEXT("SummonedUnit.h"),
		TEXT("SummonedUnit.cpp")
	};

	// The ONLY two shipping files that may ASSIGN it: the statics (the one write-true and the one
	// write-false, both through the reference parameter) and the header's member initialiser
	// (`bool bIsInvisible = false;` — a unit is born VISIBLE).
	const TCHAR* const AllowedAssignFiles[] =
	{
		TEXT("SiegeInvisibilityStatics.cpp"),
		TEXT("SummonedUnit.h")
	};

	FString StrayFlagFiles;
	FString StrayAssignFiles;
	int32 FlagHitTotal = 0;

	for (const FString& File : SourceFiles)
	{
		if (IsAutomationTestFile(File))
		{
			continue;
		}

		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *File))
		{
			AddError(FString::Printf(TEXT("⛔ Could not read '%s' — an unreadable file FAILS the gate."), *File));
			return false;
		}

		const FString Clean = FPaths::GetCleanFilename(File);

		const int32 FlagHits = CountOccurrencesInCode(Text, TEXT("bIsInvisible"));
		if (FlagHits > 0)
		{
			FlagHitTotal += FlagHits;

			bool bAllowed = false;
			for (const TCHAR* Allowed : AllowedFlagFiles)
			{
				bAllowed = bAllowed || Clean.Equals(Allowed, ESearchCase::CaseSensitive);
			}
			if (!bAllowed)
			{
				StrayFlagFiles += FString::Printf(TEXT("\n    %d x  %s"), FlagHits, *Clean);
			}
		}

		const int32 AssignHits = CountOccurrencesInCode(Text, TEXT("bIsInvisible ="));
		if (AssignHits > 0)
		{
			bool bAllowed = false;
			for (const TCHAR* Allowed : AllowedAssignFiles)
			{
				bAllowed = bAllowed || Clean.Equals(Allowed, ESearchCase::CaseSensitive);
			}
			if (!bAllowed)
			{
				StrayAssignFiles += FString::Printf(TEXT("\n    %d x  %s"), AssignHits, *Clean);
			}
		}
	}

	// ── THE POSITIVE CONTROL, FIRST. Both absences below would pass vacuously against a broken
	//    scanner or an empty read; this proves the instrument sees the flag where it really lives.
	TestTrue(
		FString::Printf(
			TEXT("SELF-CHECK: the scan really does find `bIsInvisible` in shipping source (found %d hits). If ")
			TEXT("this were zero the two absence assertions below would be proving nothing at all."), FlagHitTotal),
		FlagHitTotal > 0);

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ NO MIRRORED VEIL BOOL ON ANY OTHER CLASS (WITCH-§6). `bIsInvisible` is named ONLY by the ")
			TEXT("pure rules and by ASummonedUnit, the one class that owns it. ⛔ A second copy on a tower, on ")
			TEXT("the hero, or on the bot controller would be a veil that can disagree with itself — and the ")
			TEXT("disagreement would show up as a unit that is invisible to one system and visible to another. ")
			TEXT("Stray files:%s"),
			StrayFlagFiles.IsEmpty() ? TEXT(" (none)") : *StrayFlagFiles),
		StrayFlagFiles.Len(), 0);

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ NO INLINED `bIsInvisible = …` ANYWHERE (WITCH-§6 calls one an AUTOMATIC QA FAIL). Every ")
			TEXT("write goes through FSiegeInvisibilityStatics::ApplyVeil / ApplyBreak, which is what makes a ")
			TEXT("grep for BreakInvisibility the COMPLETE list of ways a unit loses its veil. Stray files:%s"),
			StrayAssignFiles.IsEmpty() ? TEXT(" (none)") : *StrayAssignFiles),
		StrayAssignFiles.Len(), 0);

	// ── The two doors are exactly one call each, in exactly one file: the actor-side seams.
	FString UnitCpp;
	if (LoadProjectSource(*this, SummonedUnitCpp, UnitCpp))
	{
		TestEqual(
			TEXT("⭐ ASummonedUnit calls ApplyVeil exactly ONCE (GrantInvisibility — the only write-true door)."),
			CountOccurrencesInCode(UnitCpp, TEXT("FSiegeInvisibilityStatics::ApplyVeil(")), 1);

		TestEqual(
			TEXT("⭐ …and ApplyBreak exactly ONCE (BreakInvisibility — the only write-false door). Two calls here ")
			TEXT("would mean a second break path that a grep for the door name would MISS."),
			CountOccurrencesInCode(UnitCpp, TEXT("FSiegeInvisibilityStatics::ApplyBreak(")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  10. ⭐ THE CENSUS — every act in the closed set is wired, at the measured site, once.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityWiringCensusTest,
	"Siegebound.Invisibility.EveryShippedActInTheClosedSetIsWired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityWiringCensusTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔ The needle carries `::` on purpose: the DEFINITION reads
	// `void ASummonedUnit::BreakInvisibility(ESiegeVeilBreakReason Reason)` and the DECLARATION
	// reads the same, so neither can ever be counted as a call site.
	const TCHAR* const CallNeedle = TEXT("BreakInvisibility(ESiegeVeilBreakReason::");

	struct FSiteCount
	{
		const TCHAR* RelativePath;
		const TCHAR* Needle;
		int32 Expected;
		const TCHAR* Why;
	};

	const FSiteCount Sites[] =
	{
		{ SummonedUnitCpp, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Attack)"), 3,
		  TEXT("melee (the blow lands), ranged (the shot is loosed), and the SAPPER's blast inside ApplyDetonation") },
		{ SummonedUnitCpp, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Heal)"), 1,
		  TEXT("PerformHeal — the HEALER, never the patient") },
		{ SummonedUnitCpp, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Death)"), 1,
		  TEXT("HandleDeath — J-W4, no corpse is invisible") },
		{ SummonedUnitCpp, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Cast)"), 1,
		  TEXT("CompleteWitchCast — J-W3, the witch's own COMPLETED cast is an ACT. ⛔ TASK-830 landed this row; it was 0 by design while the enumerator had no site") },
		{ MinerUnitCpp,    TEXT("BreakInvisibility(ESiegeVeilBreakReason::Mine)"), 1,
		  TEXT("the ARRIVAL claim in UpdateMining, once per tenure") },
		{ AncientGroundCpp, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Empower)"), 1,
		  TEXT("the COUNTED sorcerer — the ground's tick is where the sorcerer's act is observable") },
	};

	int32 Total = 0;
	for (const FSiteCount& Site : Sites)
	{
		FString Text;
		if (!LoadProjectSource(*this, Site.RelativePath, Text))
		{
			continue;
		}

		const int32 Hits = CountOccurrencesInCode(Text, Site.Needle);
		TestEqual(
			FString::Printf(
				TEXT("⭐ `%s` appears %d time(s) — %s. ⛔ A DROP means one of Jonathan's six verbs no longer ")
				TEXT("un-veils and the unit stays invisible while doing it; an INCREASE means a site was added ")
				TEXT("without a ruling."),
				Site.Needle, Site.Expected, Site.Why),
			Hits, Site.Expected);
		Total += Hits;
	}

	// ── AND THE TREE-WIDE TOTAL, which is the row that catches a break added somewhere nobody
	//    listed — the failure a per-file census cannot see.
	FString Where;
	const int32 ShippingTotal = CountAcrossShippingSource(*this, CallNeedle, Where);
	if (ShippingTotal < 0)
	{
		return false;
	}

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ EIGHT break calls in the whole of shipping Source/, and these are ALL of them. ⛔ UPDATED ")
			TEXT("FROM SEVEN BY TASK-830, DELIBERATELY AND BY NAME: `Cast` was ZERO while WITCH-§7's refutation ")
			TEXT("held (no unit, hero or pawn casts a shipped spell — every card spell is cast by a CONTROLLER), ")
			TEXT("and the WITCH is the one caster his sentence adds. ⚠️ If this number grew AGAIN, read WITCH-§3 ")
			TEXT("before you accept it: a break in a place his sentence does not cover un-veils a unit for doing ")
			TEXT("nothing. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		ShippingTotal, 8);

	TestEqual(TEXT("SELF-CHECK: the per-file census and the tree-wide scan agree."), Total, ShippingTotal);

	// ── The `Cast` enumerator, now WIRED — the row TASK-829 wrote as "when 830 lands this becomes
	//    1 and this row is updated DELIBERATELY". It is inverted here rather than deleted, exactly
	//    as that instruction asked, and it still says something worth saying: `Cast` has ⛔ ONE
	//    site, so the witch is the ⛔ only caster in the game and WITCH-§7's refutation (no shipped
	//    card spell is cast by a pawn) still holds around it.
	FString CastWhere;
	const int32 CastCalls = CountAcrossShippingSource(*this, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Cast)"), CastWhere);
	TestEqual(
		FString::Printf(
			TEXT("⭐ `Cast` is wired EXACTLY ONCE — TASK-830's CompleteWitchCast, and nowhere else. ⛔ TWO would ")
			TEXT("mean a second thing in the game counts as 'the witch casting', which WITCH-§7 measured does not ")
			TEXT("exist; ⛔ ZERO would mean the witch un-veils herself nowhere and J-W3 is unimplemented, leaving a ")
			TEXT("self-veiled witch PERMANENTLY untargetable-by-acquisition — the outcome WITCH-§0 refuses. Found:%s"),
			CastWhere.IsEmpty() ? TEXT(" (nowhere — the witch's own veil never breaks)") : *CastWhere),
		CastCalls, 1);

	// ── AND WHERE IT LIVES. The break must be on the COMPLETION path: an INTERRUPTED cast produces
	//    no veil, no partial state, no cost — and therefore NO BREAK (WITCH-§4). A break placed in
	//    BeginWitchCast instead would compile, would keep the count above at 1, and would un-veil
	//    the witch for a cast that never lands — i.e. she would pay for every interrupted attempt.
	FString UnitText;
	if (LoadProjectSource(*this, SummonedUnitCpp, UnitText))
	{
		FString CompleteBody;
		if (ExtractFunctionBody(*this, UnitText, TEXT("void ASummonedUnit::CompleteWitchCast()"), CompleteBody))
		{
			TestEqual(
				TEXT("⭐⭐ …and it is inside CompleteWitchCast — the act is the LANDING, never the attempt (J-W3 ")
				TEXT("+ WITCH-§4's 'no veil, no partial state, no cost')."),
				CountOccurrencesInCode(CompleteBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Cast)")), 1);
		}

		FString BeginBody;
		if (ExtractFunctionBody(*this, UnitText, TEXT("void ASummonedUnit::BeginWitchCast(ASummonedUnit* Subject)"), BeginBody))
		{
			TestTrue(
				TEXT("SELF-CHECK: the extracted BeginWitchCast body is substantial — an empty extraction would make ")
				TEXT("the assertion below pass vacuously."),
				BeginBody.Len() > 200);

			TestEqual(
				TEXT("⛔⛔ …and BeginWitchCast breaks NOTHING and veils NOBODY. Starting a cast must have nothing to ")
				TEXT("undo, which is the only way 'an interrupted cast leaves no partial state' can be guaranteed ")
				TEXT("rather than remembered."),
				CountOccurrencesInCode(BeginBody, TEXT("BreakInvisibility(")), 0);

			TestEqual(
				TEXT("⛔ …and grants no veil at the START. An optimistic veil at cast start would leave a unit ")
				TEXT("invisible after an interrupt, which is exactly the bug WITCH-§4's 'no partial state' names."),
				CountOccurrencesInCode(BeginBody, TEXT("GrantInvisibility()")), 0);
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  11. ⛔⛔ TRAP 1 of 3 — THE SAPPER. The single highest-value row in this file.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilitySapperTrapTest,
	"Siegebound.Invisibility.TheSapperBreaksInsideApplyDetonationNotOnlyInPerformAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilitySapperTrapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	// ⛔⛔⛔ THE DEFECT THIS ROW EXISTS FOR, IN ONE SENTENCE: `ApplyDetonation` is NOT reached
	// through `PerformAttack`. It has its own two entries — contact (`Detonate()` from
	// UpdateStateSiege) and death (`HandleDeath()`). A wiring that guarded only `PerformAttack`
	// compiles, reviews cleanly, passes every test in this file except this one, and ships a
	// ⛔ VEILED SAPPER THAT BREACHES A BUILDING WHILE STILL INVISIBLE.
	FString DetonationBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::ApplyDetonation()"), DetonationBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted ApplyDetonation body is substantial — an empty extraction would make ")
		TEXT("the assertion below pass vacuously."),
		DetonationBody.Len() > 200);

	TestEqual(
		TEXT("⭐⭐⭐ THE SAPPER'S BLAST BREAKS THE VEIL, AND IT BREAKS IT ***HERE*** — inside ApplyDetonation, ")
		TEXT("the ONE function both the contact entry and the death entry share. ⛔ If this is ZERO the card is ")
		TEXT("broken in the way a player would actually report it: an invisible Sapper walks into a building ")
		TEXT("and blows it open without ever becoming visible."),
		CountOccurrencesInCode(DetonationBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Attack)")), 1);

	// ── THE DISCRIMINATOR (TASK-829(6a)): the break must be at the SHARED function, not at the
	//    two entries. A wiring that put it at Detonate() would miss the death path entirely.
	FString DetonateBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::Detonate()"), DetonateBody))
	{
		TestEqual(
			TEXT("⛔ `Detonate()` (the CONTACT entry) carries no break of its own — it delegates. A break here ")
			TEXT("instead of in ApplyDetonation would cover the contact path and MISS a Sapper that is shot down ")
			TEXT("en route, whose blast fires from HandleDeath."),
			CountOccurrencesInCode(DetonateBody, TEXT("BreakInvisibility(")), 0);
	}

	// ── And PerformAttack still breaks — twice, once per delivery mode. This is the half a naive
	//    implementation gets RIGHT, asserted so the row above cannot be "fixed" by moving code.
	FString AttackBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::PerformAttack()"), AttackBody))
	{
		TestEqual(
			TEXT("⭐ PerformAttack breaks the veil TWICE — once in the ranged branch (at FireProjectileAt) and ")
			TEXT("once in the melee branch (at ApplyDamage). They are mutually exclusive at runtime; the pair ")
			TEXT("exists so a reviewer verifying BY SYMBOL finds a break adjacent to EACH act."),
			CountOccurrencesInCode(AttackBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Attack)")), 2);

		// ⭐ ORDERING, ON CODE LINES ONLY: the break precedes the delivery in each branch, so a
		//   receiver that re-acquires inside the same call stack sees a unit that already revealed
		//   itself. Comment lines are stripped first — this function's own prose names both tokens.
		const FString AttackCode = CodeLinesOnly(AttackBody);
		const int32 FirstBreak = AttackCode.Find(TEXT("BreakInvisibility("), ESearchCase::CaseSensitive);
		const int32 FireIndex = AttackCode.Find(TEXT("FireProjectileAt(Target"), ESearchCase::CaseSensitive);
		TestTrue(
			TEXT("⭐ the ranged break precedes the shot — the veil is gone before the projectile exists."),
			FirstBreak != INDEX_NONE && FireIndex != INDEX_NONE && FirstBreak < FireIndex);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  12. ⛔⛔ TRAP 2 of 3 — EMPOWER. The break lands on the SORCERER, never on the recipients.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityEmpowerTrapTest,
	"Siegebound.Invisibility.TheEmpowerBreakLandsOnTheSorcererNotTheRecipients",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityEmpowerTrapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, AncientGroundCpp, Text))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Text, TEXT("void AAncientGround::ApplyBoostTick()"), Body))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the extracted ApplyBoostTick body is substantial."), Body.Len() > 400);

	TestEqual(
		TEXT("⭐ Exactly ONE break in the whole boost tick. Two would mean the recipients loop got one as well."),
		CountOccurrencesInCode(Body, TEXT("BreakInvisibility(")), 1);

	// ⛔⛔ THE DISCRIMINATOR, AND IT IS AN ORDERING CLAIM BECAUSE BOTH CANDIDATE SITES LIVE IN THE
	// SAME FUNCTION. The sorcerer is COUNTED in the first sweep (`IsAncientGroundEmpowerer()`) and
	// `continue`s BEFORE `Occupants`; the grant happens in a SEPARATE, LATER loop over the
	// RECIPIENTS. A break placed at `AddPermanentDamageStacks` would un-veil ⛔ exactly the wrong
	// actors — units that are being ACTED UPON, which his rule never breaks on.
	// ⭐ Comment lines are stripped first: this function now carries prose that NAMES
	// `BreakInvisibility` beside the loop it is forbidding it in, and a raw Find would read that
	// refusal as the defect.
	const FString Code = CodeLinesOnly(Body);
	const int32 EmpowererIndex = Code.Find(TEXT("IsAncientGroundEmpowerer()"), ESearchCase::CaseSensitive);
	const int32 BreakIndex = Code.Find(TEXT("BreakInvisibility("), ESearchCase::CaseSensitive);
	const int32 GrantIndex = Code.Find(TEXT("AddPermanentDamageStacks("), ESearchCase::CaseSensitive);

	TestTrue(
		TEXT("SELF-CHECK: all three landmarks are present in the code-only body — otherwise the ordering claim ")
		TEXT("below would be comparing INDEX_NONE against INDEX_NONE."),
		EmpowererIndex != INDEX_NONE && BreakIndex != INDEX_NONE && GrantIndex != INDEX_NONE);

	TestTrue(
		TEXT("⭐⭐⭐ THE BREAK IS INSIDE THE `IsAncientGroundEmpowerer()` ARM — i.e. on the COUNTED SORCERER, the ")
		TEXT("actor that is ACTING even though it executes no code of its own. ⛔ The sorcerer is PASSIVE here: ")
		TEXT("the GROUND does the granting, which is precisely why the obvious wiring point is the wrong one."),
		EmpowererIndex != INDEX_NONE && BreakIndex != INDEX_NONE && EmpowererIndex < BreakIndex);

	TestTrue(
		TEXT("⭐⭐⭐ …and it is STRICTLY BEFORE the `AddPermanentDamageStacks` grant loop, which is the ONLY thing ")
		TEXT("that distinguishes a correct wiring from the wrong one. ⛔ A break at the grant would un-veil the ")
		TEXT("RECIPIENTS — units being acted upon, who must STAY veiled exactly as a healed unit does."),
		BreakIndex != INDEX_NONE && GrantIndex != INDEX_NONE && BreakIndex < GrantIndex);

	// ── And the receiving mutator itself carries no break, on the class where it is defined.
	FString UnitCpp;
	if (LoadProjectSource(*this, SummonedUnitCpp, UnitCpp))
	{
		FString StacksBody;
		if (ExtractFunctionBody(*this, UnitCpp, TEXT("void ASummonedUnit::AddPermanentDamageStacks(int32 Stacks)"), StacksBody))
		{
			TestEqual(
				TEXT("⛔ ASummonedUnit::AddPermanentDamageStacks — the RECEIVING side — carries no break. Being ")
				TEXT("empowered is being acted upon, the same category as being healed, buffed or frozen."),
				CountOccurrencesInCode(StacksBody, TEXT("BreakInvisibility(")), 0);
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  13. ⛔⛔ TRAP 3 of 3 — MINE. The break is the ARRIVAL, never the payout.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityMineTrapTest,
	"Siegebound.Invisibility.TheMineBreakLandsOnArrivalNotOnTheGoldPayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityMineTrapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, MinerUnitCpp, Text))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Text, TEXT("void AMinerUnit::UpdateMining()"), Body))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the extracted UpdateMining body is substantial."), Body.Len() > 400);

	TestEqual(
		TEXT("⭐ Exactly ONE break in the mining poll — this runs on a timer, and a second call would be a ")
		TEXT("per-tick break on a unit that has already revealed itself."),
		CountOccurrencesInCode(Body, TEXT("BreakInvisibility(")), 1);

	// ⛔⛔ THE DISCRIMINATOR: it must sit INSIDE the registration arm, i.e. AFTER
	// `TryRegisterArrivedMiner(this)` — the atomic claim that fires ONCE PER TENURE on the ACTING
	// miner. A miner REFUSED by an enemy-claimed node falls to WAIT MODE and must stay veiled.
	const FString Code = CodeLinesOnly(Body);
	const int32 ClaimIndex = Code.Find(TEXT("TryRegisterArrivedMiner(this)"), ESearchCase::CaseSensitive);
	const int32 BreakIndex = Code.Find(TEXT("BreakInvisibility("), ESearchCase::CaseSensitive);

	TestTrue(
		TEXT("SELF-CHECK: both landmarks are present in the code-only body."),
		ClaimIndex != INDEX_NONE && BreakIndex != INDEX_NONE);

	TestTrue(
		TEXT("⭐⭐⭐ THE BREAK IS INSIDE THE ARRIVAL-CLAIM ARM, after TryRegisterArrivedMiner succeeded. ⛔ A break ")
		TEXT("placed above the claim would un-veil a miner that was REFUSED by an enemy-held node and is now ")
		TEXT("merely standing in a queue — and waiting is standing still, which is his explicit carve-out."),
		ClaimIndex != INDEX_NONE && BreakIndex != INDEX_NONE && ClaimIndex < BreakIndex);

	// ⛔⛔ AND THE HALF THAT MAKES THIS A TRAP AT ALL: the gold is NEVER granted on the miner's
	// call stack. The node's reserve drains on the NODE's timer and the player's +gold/s lands on
	// the PLAYER STATE's timer. A hook on "where the gold appears" would break the veil of EVERY
	// miner the player owns, including ones still walking across the field.
	const TCHAR* const PayoutFiles[] = { PlayerStateCpp, GoldNodeCpp };
	for (const TCHAR* PayoutFile : PayoutFiles)
	{
		FString PayoutText;
		if (!LoadProjectSource(*this, PayoutFile, PayoutText))
		{
			continue;
		}
		TestEqual(
			FString::Printf(
				TEXT("⛔⛔ `%s` contains ZERO veil breaks. This is the PAYOUT side, and it runs on its own timer ")
				TEXT("with no acting miner on the stack — a break here would un-veil every miner the player owns, ")
				TEXT("including ones still walking. ⭐ That is the entire reason WITCH-§3a calls this a trap."),
				PayoutFile),
			CountOccurrencesInCode(PayoutText, TEXT("BreakInvisibility(")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  14. ⭐ DIRECTION — the HEALER breaks; the PATIENT does not.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityHealDirectionTest,
	"Siegebound.Invisibility.TheHealerBreaksAndThePatientStaysVeiled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityHealDirectionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString HealerBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::PerformHeal()"), HealerBody))
	{
		TestEqual(
			TEXT("⭐ PerformHeal — the HEALER — breaks the veil exactly once. Healing is one of the four verbs ")
			TEXT("Jonathan typed."),
			CountOccurrencesInCode(HealerBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Heal)")), 1);

		// ⭐ Placed after every early-out: a Cleric whose target died, topped off or drifted out of
		//   range this tick returns WITHOUT healing and WITHOUT un-veiling. The break is the ACT.
		const FString HealerCode = CodeLinesOnly(HealerBody);
		const int32 BreakIndex = HealerCode.Find(TEXT("BreakInvisibility("), ESearchCase::CaseSensitive);
		const int32 ApplyIndex = HealerCode.Find(TEXT("ApplyHealing("), ESearchCase::CaseSensitive);
		TestTrue(
			TEXT("⭐ …and it sits immediately before the heal itself, after every early-out — a Cleric that ")
			TEXT("returns because its target topped off or drifted out of range stays VEILED. The break is the ")
			TEXT("ACT, not the intention."),
			BreakIndex != INDEX_NONE && ApplyIndex != INDEX_NONE && BreakIndex < ApplyIndex);
	}

	FString PatientBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::ApplyHealing(float Amount)"), PatientBody))
	{
		TestEqual(
			TEXT("⭐⭐⭐ ApplyHealing — the RECEIVER — carries NO break, and this is the row that pins the ")
			TEXT("direction. ⛔ A veiled unit mended by a friendly Cleric STAYS VEILED: being healed is being ")
			TEXT("ACTED UPON, and his rule breaks the veil on ACTING. ⚠️ Getting this backwards would un-veil ")
			TEXT("every wounded unit on the field the moment a Cleric walked past it."),
			CountOccurrencesInCode(PatientBody, TEXT("BreakInvisibility(")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  15. ⭐⭐ THE INVERSE LEDGER, AS A TEST — walking, being acted upon and being commanded.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityNonActsDoNotBreakTest,
	"Siegebound.Invisibility.WalkingClimbingAndBeingActedUponDoNotBreakTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityNonActsDoNotBreakTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	struct FNonSite
	{
		const TCHAR* Signature;
		const TCHAR* Why;
	};

	// ⛔ Each of these is a place a "for safety" break would look reasonable and would DELETE the
	// card. They are the shipped counterpart of the inverse ledger in SiegeInvisibilityStatics.h.
	const FNonSite NonSites[] =
	{
		{ TEXT("AActor* ASummonedUnit::AcquireTarget() const"),
		  TEXT("ACQUIRING IS NOT ACTING — a veiled unit may pick a target, cross the field, close to range and raise its weapon, and STAY VEILED. A break here un-veils the ENTIRE approach and the card reads as broken") },
		{ TEXT("AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const"),
		  TEXT("the zone-order acquisition — same rule, second entry point") },
		{ TEXT("float ASummonedUnit::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)"),
		  TEXT("TAKING DAMAGE IS NOT ACTING. ⚠️ It DOES interrupt an in-progress witch CAST (WITCH-§4, TASK-830) — a DIFFERENT rule on a DIFFERENT subject, and merging them would make every unit clipped by a stray AoE permanently visible") },
		{ TEXT("bool ASummonedUnit::BeginLadderClimb(const FVector& FromWorld, const FVector& ToWorld)"),
		  TEXT("CLIMBING IS LOCOMOTION — it is MOVE_Flying plus AddMovementInput along one line, and it DISARMS attacking at all three guard points. A unit that cannot attack while doing it cannot be acting") },
		{ TEXT("void ASummonedUnit::AssignCommandGroup(int32 GroupId, const FVector& StationOffset)"),
		  TEXT("BEING ORDERED IS NOT ACTING (WITCH-§2 lane 4) — an invisible unit its own player cannot command is a BUG, not a feature") },
	};

	for (const FNonSite& NonSite : NonSites)
	{
		FString Body;
		if (!ExtractFunctionBody(*this, Text, NonSite.Signature, Body))
		{
			continue;
		}

		TestEqual(
			FString::Printf(TEXT("⛔ `%s` carries NO veil break — %s."), NonSite.Signature, NonSite.Why),
			CountOccurrencesInCode(Body, TEXT("BreakInvisibility(")), 0);
	}

	// ── CAPTURE (ruling J-W9) — a whole FILE that must carry none, because capture is a ZONE-SIDE
	//    poll of positions. It is indistinguishable from standing still, and breaking here would
	//    un-veil every unit that merely WALKS THROUGH a zone — the main route across the field.
	//    ⚠️ DECLARED, ACCEPTED LEAK, recorded so it is not read as an oversight: a veiled unit
	//    STILL CAPTURES, so a zone can flip with no visible cause. That is a DESIGN question.
	FString ZoneText;
	if (LoadProjectSource(*this, CaptureZoneCpp, ZoneText))
	{
		TestEqual(
			TEXT("⛔ CaptureZone.cpp carries no veil break at all (ruling J-W9): capture is PRESENCE, and presence ")
			TEXT("is indistinguishable from standing still. ⚠️ The accepted consequence — a veiled unit still ")
			TEXT("captures — is DECLARED, not hidden."),
			CountOccurrencesInCode(ZoneText, TEXT("BreakInvisibility(")), 0);
	}

	// ── THE HERO AND THE TOWERS — zero, because J-W10 rules the hero NOT veilable and WITCH-§7
	//    records towers as not veilable today. WITCH-§6 forbids a mirrored flag on either, so a
	//    break there would be dead code contradicting a live ruling. Both files carry a NAMED
	//    comment saying so, which is why this absence is a decision rather than a gap.
	const TCHAR* const DormantFiles[] = { TowerCpp, HeroCpp };
	for (const TCHAR* DormantFile : DormantFiles)
	{
		FString DormantText;
		if (!LoadProjectSource(*this, DormantFile, DormantText))
		{
			continue;
		}
		TestEqual(
			FString::Printf(
				TEXT("⛔ `%s` carries no veil break — the hero (J-W10) and towers (WITCH-§7) are NOT veilable ")
				TEXT("today, and WITCH-§6 forbids a mirrored bIsInvisible on either. ⭐ Their ACQUISITION is ")
				TEXT("suppressed anyway, for free, because both acquire through the ONE funnel."),
				DormantFile),
			CountOccurrencesInCode(DormantText, TEXT("BreakInvisibility(")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  16. ⭐⭐ SUPPRESSION — a veiled enemy is dropped, and it is dropped in exactly ONE place.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityFunnelSuppressionTest,
	"Siegebound.Invisibility.AVeiledEnemyIsDroppedByTheOneAcquisitionFunnel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityFunnelSuppressionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, CombatStaticsCpp, Text))
	{
		return false;
	}

	// ── (a) THE CONSULT IS INSIDE THE FUNNEL, and the funnel is the only acquisition road.
	FString GatherBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("void FSiegeCombatStatics::GatherHostileAgents("), GatherBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ GatherHostileAgents consults the veil exactly once. ⛔ If this is ZERO, a veiled unit is ")
		TEXT("acquirable by every unit, tower and hero in the game and the card does nothing at all — while ")
		TEXT("every rule test in this file stays green, because the rules would still be correct and unused."),
		CountOccurrencesInCode(GatherBody, TEXT("IsAgentVisibleTo(")), 1);

	// ── (b) AND THE SUPPRESSION IS ORDER-PRESERVING. Every acquisition site breaks ties by STRICT
	//    improvement, so the FIRST surviving candidate wins. A filter that reordered survivors
	//    would silently re-pick targets at eight sites without changing a single result SET.
	TestEqual(
		TEXT("⭐ …and it filters WITHOUT sorting. Callers tie-break by strict improvement, so enumeration order ")
		TEXT("decides every tie; a Sort here would re-pick targets everywhere without changing any result set."),
		CountOccurrencesInCode(GatherBody, TEXT("Sort(")), 0);

	// ── (c) THE PREDICATE IS CALLED EXACTLY ONCE IN THE WHOLE OF SHIPPING SOURCE. This is the
	//    WITCH-§1 leaf-grep idiom used as a GATE: the rule has ONE consumer, so there is exactly
	//    one place it can be forgotten — which is the entire mitigation WITCH-§0 signed up for.
	FString Where;
	const int32 QualifiedHits = CountAcrossShippingSource(*this, TEXT("FSiegeInvisibilityStatics::IsVisibleTo("), Where);
	if (QualifiedHits < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ THE SUPPRESSION PREDICATE IS NAMED EXACTLY TWICE IN SHIPPING SOURCE, AND ONLY ONE OF THOSE ")
			TEXT("IS A CALL: its own DEFINITION in SiegeInvisibilityStatics.cpp, and the ONE call inside ")
			TEXT("FSiegeCombatStatics::IsAgentVisibleTo. ⛔ A THIRD hit is a SECOND guard point, which is precisely ")
			TEXT("the pattern GHOST-§1 refused in writing and WITCH-§1 exists to hold at one. ⛔ Do NOT fix a red ")
			TEXT("here by adding a veil check at a new site: route the site through the funnel, or call ")
			TEXT("IsAgentVisibleTo (which is what TASK-851's bot scans do). Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		QualifiedHits, 2);

	TestEqual(
		TEXT("⭐ …and the ONE call lives in SiegeCombatStatics.cpp. Paired with the total above, that pins both ")
		TEXT("the count and the location: the rule has exactly one consumer, in the funnel's own file."),
		CountOccurrencesInCode(Text, TEXT("FSiegeInvisibilityStatics::IsVisibleTo(")), 1);

	// ── (d) THE NAMED-ARGUMENT GUARD (TASK-829(3b), qa/TASK-847.md WARN-2). The predicate is
	//    EXACTLY SYMMETRIC under exchange of its two ETeamId arguments for all eight inputs, so a
	//    SWAPPED call is undetectable by any test writable today. The comments are the cheap
	//    structural guard; the enclosing signature (one team, one actor) is the real one.
	FString VisibleToBody;
	if (ExtractFunctionBody(*this, Text, TEXT("bool FSiegeCombatStatics::IsAgentVisibleTo("), VisibleToBody))
	{
		TestEqual(
			TEXT("⭐ the one symmetric-predicate call names its arguments (`/*ViewerTeam=*/`). The predicate ")
			TEXT("cannot distinguish a swap — rows 4 and 6 of the truth table MIRROR each other rather than ")
			TEXT("discriminating — so a swapped call would be silent today and a real defect the day an ")
			TEXT("asymmetric term (a see-through-veils detector, a per-team reveal) arrives."),
			CountOccurrencesInCode(VisibleToBody, TEXT("/*ViewerTeam=*/")), 1);

		TestEqual(
			TEXT("⭐ …and its target argument too (`/*TargetTeam=*/`)."),
			CountOccurrencesInCode(VisibleToBody, TEXT("/*TargetTeam=*/")), 1);

		// ⚠️⚠️ MEASURED WHILE WRITING THIS GATE, AND RECORDED BECAUSE IT ALMOST DEFEATED IT:
		// CountOccurrencesInCode SKIPS any line whose trimmed form starts with `/*`. The natural
		// one-argument-per-line layout puts each `/*ViewerTeam=*/` at the head of its own line, so
		// all three rows here would have read ZERO — and the obvious "fix" is to expect zero, which
		// would delete the guard while leaving a green test claiming to enforce it. The shipped call
		// is therefore written on ONE line, and SiegeCombatStatics.cpp says so beside it.
		TestEqual(
			TEXT("⭐ …and the flag argument (`/*bTargetIsInvisible=*/`). ⚠️ All three must sit on a line that ")
			TEXT("does NOT begin with `/*`, or the scanner skips them and this gate silently reads zero — which ")
			TEXT("is why the shipped call is deliberately not re-wrapped."),
			CountOccurrencesInCode(VisibleToBody, TEXT("/*bTargetIsInvisible=*/")), 1);

		TestEqual(
			TEXT("⭐⭐ ONE cast to ASummonedUnit — the only veilable class (WITCH-§6 bans a mirrored flag ")
			TEXT("elsewhere; J-W10 rules the hero out). A SECOND cast here would be the one somebody forgets to ")
			TEXT("keep in step; a second veilable class belongs on the interface instead."),
			CountOccurrencesInCode(VisibleToBody, TEXT("Cast<ASummonedUnit>(")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  17. ⛔⛔ THE BLAST EXEMPTION — an invisible unit IS caught by a blast (WITCH-§2, J-W2).
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBlastExemptionTest,
	"Siegebound.Invisibility.BlastLaneIsExemptFromTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBlastExemptionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, CombatStaticsCpp, Text))
	{
		return false;
	}

	FString BlastBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("void FSiegeCombatStatics::ApplyRadialDamage("), BlastBody))
	{
		return false;
	}

	// ⚖️ "A blast is not an act of seeing" — and it is the 50-gold card's ONLY counter. Without
	// this, a veiled push is an auto-win: nothing on the field can target it and nothing can
	// flush it either. ⛔ THE DANGEROUS PROPERTY OF THIS DEFECT is that it is INVISIBLE to the
	// rest of the suite — deleting the argument breaks no friendly-fire, damage, radius or
	// ordering assertion anywhere.
	TestEqual(
		TEXT("⭐⭐⭐ THE BLAST LANE ASKS TO SEE THROUGH THE VEIL, EXPLICITLY AND EXACTLY ONCE. Every AoE in the ")
		TEXT("game routes through ApplyRadialDamage — Sapper suicide, Bomb Tower, Fireball, every blast ")
		TEXT("projectile — so a ZERO here means all of them silently miss invisible units and the card has no ")
		TEXT("counter at all."),
		CountOccurrencesInCode(BlastBody, TEXT("ESiegeVeilPolicy::IncludeVeiled")), 1);

	TestEqual(
		TEXT("⛔ …and it never asks for suppression. The same defect, spelled the other way round."),
		CountOccurrencesInCode(BlastBody, TEXT("SuppressVeiled")), 0);

	TestEqual(
		TEXT("⛔ The exemption is a NAMED POLICY handed to the ONE funnel — ApplyRadialDamage never reads the ")
		TEXT("flag itself. An inline read here would be a second guard point (WITCH-§1) and a second source of ")
		TEXT("truth (WITCH-§6) in the most trafficked function in the combat code."),
		CountOccurrencesInCode(BlastBody, TEXT("bIsInvisible")), 0);

	// ── AND IT IS THE ONLY ONE IN THE TREE. A second IncludeVeiled would be an ACQUISITION site
	//    quietly opting out of the feature, which is the one thing the policy enum makes greppable.
	FString Where;
	const int32 ExemptTotal = CountAcrossShippingSource(*this, TEXT("ESiegeVeilPolicy::IncludeVeiled"), Where);
	if (ExemptTotal < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ EXACTLY ONE lane in the whole project sees through the veil, and it is the blast. ⛔ A ")
			TEXT("SECOND IncludeVeiled would be an ACQUISITION site opting out of the feature — the failure the ")
			TEXT("named policy exists to make greppable. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		ExemptTotal, 1);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  18. ⭐ THE FRIENDLY LANE CANNOT BE SUPPRESSED — WITCH-§2's fourth lane, structurally.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityFriendlyLaneHasNoPolicyTest,
	"Siegebound.Invisibility.TheFriendlyLaneCannotBeVeilSuppressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityFriendlyLaneHasNoPolicyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔⛔ "An invisible unit that its own player cannot select, order or heal is a BUG, not a
	// feature" (WITCH-§2, fourth lane). This is enforced by SHAPE: the friendly gather takes no
	// veil policy at all, so the favouritism is UNREPRESENTABLE rather than merely absent.
	// ⚖️ Deliberate contrast with FOG-§, where the symmetry is enforced by the ABSENCE of a team
	// parameter. Same technique, opposite feature: fog is symmetric, the veil is not.
	FString Header;
	if (LoadProjectSource(*this, CombatStaticsHeader, Header))
	{
		TestEqual(
			TEXT("⭐ `GatherFriendlyAgents` is still declared exactly once — a NAMED lane, so it cannot be ")
			TEXT("accidentally folded into the suppressed one."),
			CountOccurrencesInCode(Header, TEXT("GatherFriendlyAgents")), 1);

		TestEqual(
			TEXT("⭐⭐ Exactly ONE function in the whole header takes a veil policy, and it is the HOSTILE ")
			TEXT("gather. ⛔ A second `ESiegeVeilPolicy VeilPolicy` parameter would mean the friendly lane grew ")
			TEXT("a switch — and a lane with a switch is a lane somebody can flip. WITCH-§2's fourth ruling is ")
			TEXT("that friendly acquisition is NEVER veil-suppressed, so the correct number of switches is zero."),
			CountOccurrencesInCode(Header, TEXT("ESiegeVeilPolicy VeilPolicy")), 1);
	}

	FString Text;
	if (LoadProjectSource(*this, CombatStaticsCpp, Text))
	{
		FString FriendlyBody;
		if (ExtractFunctionBody(*this, Text, TEXT("void FSiegeCombatStatics::GatherFriendlyAgents("), FriendlyBody))
		{
			TestEqual(
				TEXT("⭐⭐⭐ THE FRIENDLY GATHER NEVER CONSULTS THE VEIL. ⛔ If this is non-zero, a player's own ")
				TEXT("veiled unit becomes unselectable, unorderable and unhealable — the exact outcome WITCH-§2 ")
				TEXT("calls a BUG rather than a feature."),
				CountOccurrencesInCode(FriendlyBody, TEXT("IsAgentVisibleTo(")), 0);

			TestEqual(
				TEXT("⛔ …and it names no veil policy either. The lane has no switch to flip."),
				CountOccurrencesInCode(FriendlyBody, TEXT("ESiegeVeilPolicy")), 0);
		}
	}

	// ── AND THE RUNTIME HALF, which the structure alone cannot prove: the predicate itself is
	//    inert for allies. Asserted here as well as in test 2 because THIS is the claim the
	//    friendly lane's structural exemption is standing on.
	for (int32 TeamIndex = 0; TeamIndex < BothTeamsNum; ++TeamIndex)
	{
		const ETeamId Team = BothTeams[TeamIndex];
		TestTrue(
			FString::Printf(TEXT("BELT AND BRACES: even if the friendly lane DID suppress, a veiled %s unit reads VISIBLE to a %s viewer"),
				TeamName(Team), TeamName(Team)),
			FSiegeInvisibilityStatics::IsVisibleTo(Team, Team, /*bTargetIsInvisible*/ true));
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  19. ⛔⛔ "PERMANENTLY" — enforced by the SHAPE of the shipped API, tree-wide.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityPermanentlyIsStructuralTest,
	"Siegebound.Invisibility.PermanentlyIsEnforcedByShapeNotByComment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityPermanentlyIsStructuralTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔⛔ HIS WORD IS "PERMANENTLY", AND IT BINDS: once broken, the veil NEVER self-restores —
	// no timer, no cooldown, no decay, no re-veil except a NEW witch cast. TASK-827 made that
	// unrepresentable in the pure rules (not one function there takes a time). This row extends
	// the same guarantee to the ACTOR side, where a timer would actually be easy to write:
	// ASummonedUnit is full of FTimerHandles, and adding a veil one would look completely normal.
	const TCHAR* const BannedNames[] =
	{
		TEXT("RestoreVeil"),
		TEXT("RefreshVeil"),
		TEXT("TickVeil"),
		TEXT("EndVeil"),
		TEXT("VeilTimerHandle"),
		TEXT("VeilDuration"),
		TEXT("VeilSeconds"),
		TEXT("InvisibilityDuration"),
		TEXT("InvisibilitySeconds"),
		// ⛔ WITCH-§6 bans a "was visible" cache BY NAME. It is unnecessary because ApplyBreak
		// returns true EXACTLY ONCE, on the true→false edge — that edge IS the one-shot signal a
		// cache would exist to provide, and a cache is a second source of truth that can drift.
		TEXT("bWasVisible"),
		TEXT("bWasInvisible"),
		TEXT("bPreviouslyVisible"),
		TEXT("bPreviouslyInvisible")
	};

	// ⭐ ONE PASS over shipping source for all FOURTEEN needles (the THIRTEEN bans above plus one
	// control) rather than fourteen recursive scans — the tree is walked once and every count comes
	// out of the same read, so the control below is a control over the SAME instrument the bans used.
	constexpr int32 BannedCount = UE_ARRAY_COUNT(BannedNames);
	int32 BannedHits[BannedCount] = { 0 };
	FString BannedWhere[BannedCount];
	int32 ControlHits = 0;

	TArray<FString> SourceFiles;
	FindAllSourceFiles(SourceFiles);
	if (SourceFiles.Num() < 20)
	{
		AddError(FString::Printf(
			TEXT("SELF-CHECK FAILED: the recursive scan of Source/ found only %d file(s) — every count below ")
			TEXT("would be a meaningless zero."), SourceFiles.Num()));
		return false;
	}

	for (const FString& File : SourceFiles)
	{
		if (IsAutomationTestFile(File))
		{
			continue;
		}

		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *File))
		{
			AddError(FString::Printf(TEXT("⛔ Could not read '%s' — an unreadable file FAILS the gate."), *File));
			return false;
		}

		for (int32 Index = 0; Index < BannedCount; ++Index)
		{
			const int32 Hits = CountOccurrencesInCode(Text, BannedNames[Index]);
			if (Hits > 0)
			{
				BannedHits[Index] += Hits;
				BannedWhere[Index] += FString::Printf(TEXT("\n    %d x  %s"), Hits, *FPaths::GetCleanFilename(File));
			}
		}

		ControlHits += CountOccurrencesInCode(Text, TEXT("BreakInvisibility"));
	}

	// ── THE POSITIVE CONTROL FIRST. Without it, the THIRTEEN zeros below would be
	//    indistinguishable from a scanner that reads nothing at all.
	TestTrue(
		FString::Printf(
			TEXT("SELF-CHECK: the same pass DOES find `BreakInvisibility` in shipping source (%d hits). If this ")
			TEXT("were zero, every absence below would be vacuous."), ControlHits),
		ControlHits > 0);

	for (int32 Index = 0; Index < BannedCount; ++Index)
	{
		TestEqual(
			FString::Printf(
				TEXT("⛔ `%s` appears NOWHERE in shipping source. \"Permanently\" is his word and it binds: the ")
				TEXT("veil never self-restores, and there is no cache of a previous visibility to drift out of ")
				TEXT("step with the flag. ⚠️ If you are here because you added one, the answer is almost ")
				TEXT("certainly ApplyBreak's true-exactly-once edge, which already gives you the one-shot signal. ")
				TEXT("Found:%s"),
				BannedNames[Index], BannedWhere[Index].IsEmpty() ? TEXT(" (nowhere — correct)") : *BannedWhere[Index]),
			BannedHits[Index], 0);
	}

	// ── And the actor-side API is exactly three functions: read, grant, break. A fourth veil
	//    function on ASummonedUnit is the shape a cooldown would arrive in.
	FString UnitHeader;
	if (LoadProjectSource(*this, SummonedUnitHeader, UnitHeader))
	{
		TestEqual(
			TEXT("⭐ ASummonedUnit declares exactly ONE veil reader (`IsInvisible()`)."),
			CountOccurrencesInCode(UnitHeader, TEXT("bool IsInvisible() const")), 1);

		TestEqual(
			TEXT("⭐ …exactly ONE grant door (`GrantInvisibility()`), which is TASK-830's landing pad and ships ")
			TEXT("now so the funnel's suppression branch is reachable rather than dead."),
			CountOccurrencesInCode(UnitHeader, TEXT("bool GrantInvisibility()")), 1);

		TestEqual(
			TEXT("⭐ …and exactly ONE break door, taking a REASON and no time. ⛔ A veil function that took a ")
			TEXT("float would be the cooldown WITCH-§3 forbids, arriving as a signature change — which is a ")
			TEXT("review event rather than a silent drift."),
			CountOccurrencesInCode(UnitHeader, TEXT("void BreakInvisibility(ESiegeVeilBreakReason Reason)")), 1);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════════
//  TASK-851 — THE BOT'S THREAT READ (WITCH-§8). Everything below this line is about the ONE
//  acquisition surface in the game that is ⛔ NOT behind FSiegeCombatStatics::GatherHostileAgents.
//
//  ⛔⛔ WHY THESE THREE TESTS EXIST AT ALL, IN ONE SENTENCE: every rule test above can be GREEN
//  while a veiled unit is FULLY VISIBLE TO THE BOT — ASiegeBotController enumerates with
//  TActorIterator and a grep for the funnel's own name returns it ⛔ ZERO times, so the census that
//  produced WITCH-§1 looked complete and was not.
//
//  ⚠️⚠️ AND THE MEASUREMENT THAT MATTERS MOST: `WITCH-§8`'s own table lists ⛔ THREE scans across
//  TWO functions. TASK-851 re-measured BY SYMBOL (`SC-§38`) and found ⛔ FIVE unit-perception scans
//  across ⛔ FOUR functions — the missing one being FindLightningTowerTarget, which aims the bot's
//  OTHER offensive spell by counting enemy units around a tower. The counts asserted below are the
//  MEASURED ones, ⛔ not the ones the law predicted.
// ═════════════════════════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  20. ⭐⭐ THE BOT'S THREE AIMING SCANS CONSULT THE VEIL — with the SAME rule, never a second one.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBotAimingScansHonourTheVeilTest,
	"Siegebound.Invisibility.TheBotsThreeAimingScansHonourTheVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBotAimingScansHonourTheVeilTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, BotControllerCpp, Text))
	{
		return false;
	}

	// ── (a) ⛔⛔ THE POSITIVE CONTROL COMES FIRST, AND IT IS ⛔ NOT CEREMONY (`SC-§39`).
	//    Every assertion in this test is a COUNT taken with CountOccurrencesInCode, and that helper
	//    has a MEASURED blind spot (TASK-829 §6: it skips any line whose trimmed form starts with
	//    `/*`). A needle that cannot see a token that IS there reports the same zero as a clean
	//    tree. ⇒ prove the instrument can see this file's most characteristic token before trusting
	//    a single number below.
	//
	//    ⭐⭐ AND THIS ROW IS ⛔ DOING TWO JOBS: it is also the UNIT-SCAN CENSUS. FOUR is the number
	//    TASK-851 measured, and it is ⛔ ONE MORE than WITCH-§8's table predicted. A FIFTH scan means
	//    somebody added a new way for the bot to perceive units — which needs a RULING (threat read
	//    ⇒ suppress; physical/geometric ⇒ leave it and say why), ⛔ never a silent addition.
	const int32 UnitScans = CountOccurrencesInCode(Text, TEXT("TActorIterator<ASummonedUnit>"));
	TestEqual(
		FString::Printf(
			TEXT("SELF-CHECK + CENSUS: SiegeBotController.cpp contains exactly FOUR `TActorIterator<ASummonedUnit>` ")
			TEXT("scans (found %d). ⛔ If this is ZERO the scanner is blind and every count below is vacuous. ")
			TEXT("⛔ If it is FIVE, a new unit-perception path was added and it needs a WITCH-§8 ruling before it ")
			TEXT("ships: three of the four are THREAT/AIMING reads and are suppressed; the fourth is a PHYSICAL ")
			TEXT("spawn-clearance test and is deliberately not."),
			UnitScans),
		UnitScans, 4);

	TestEqual(
		TEXT("SELF-CHECK + CENSUS: …and exactly ONE `TActorIterator<AHeroCharacter>` scan. That one is left ")
		TEXT("unsuppressed ON PURPOSE (J-W10 — the hero is not veilable); test 21 proves the omission is decided."),
		CountOccurrencesInCode(Text, TEXT("TActorIterator<AHeroCharacter>")), 1);

	// ── (b) ⭐⭐ THREE CONSULTS, AND THREE IS THE MEASURED NUMBER — NOT THE BOARDED ONE.
	//    ⚠️ THE NEEDLE CARRIES ITS OPEN PAREN ON PURPOSE: the `#include` line for
	//    SiegeCombatStatics.h names `IsAgentVisibleTo` in a trailing `//` comment, and a trailing
	//    comment sits on a line whose TRIMMED form starts with `#include` — a CODE line. A bare-token
	//    needle would count that doc mention as a fourth call and this gate would enforce a number
	//    nobody meant. The paren makes it a CALL count.
	TestEqual(
		TEXT("⭐⭐ THE BOT CONSULTS THE VEIL AT EXACTLY THREE SITES. ⛔ If this is TWO, one of the bot's two ")
		TEXT("offensive spells still aims at units nobody can see. ⛔ If it is ZERO, a veiled unit is invisible ")
		TEXT("to every tower, unit and hero in the game AND STILL FULLY VISIBLE TO THE BOT'S BRAIN — which, ")
		TEXT("against the only shipped opponent, is close to the whole 50-gold card not working."),
		CountOccurrencesInCode(Text, TEXT("FSiegeCombatStatics::IsAgentVisibleTo(")), 3);

	// ── (c) AND THEY ARE THE THREE INTENDED SITES — one per function, located BY SYMBOL (`SC-§38`).
	//    ⛔ A file-wide count of 3 would also be satisfied by three consults in one function and none
	//    in the other two. Extracting each body is what makes the placement, not merely the total, the
	//    thing under test.
	struct FBotScanRow
	{
		const TCHAR* Signature;
		const TCHAR* Why;
	};
	const FBotScanRow SuppressedScans[] =
	{
		{
			TEXT("AActor* ASiegeBotController::FindNearestEnemyIntruderOnBotHalf() const"),
			TEXT("the DEFEND read (rule 1) — without it the bot answers a push it is promised not to see")
		},
		{
			TEXT("bool ASiegeBotController::FindFireballClusterTarget("),
			TEXT("the FIREBALL aim point (rule 3a) — WITCH-§8's own site 3")
		},
		{
			TEXT("AActor* ASiegeBotController::FindLightningTowerTarget("),
			TEXT("⭐ the LIGHTNING aim point (rule 3b) — the scan WITCH-§8's table MISSED, measured by TASK-851")
		}
	};

	for (const FBotScanRow& Row : SuppressedScans)
	{
		FString Body;
		if (!ExtractFunctionBody(*this, Text, Row.Signature, Body))
		{
			continue; // ExtractFunctionBody already raised the error — a stale probe FAILS, never passes.
		}

		TestEqual(
			FString::Printf(
				TEXT("⭐ EXACTLY ONE veil consult inside `%s` — %s."),
				Row.Signature, Row.Why),
			CountOccurrencesInCode(Body, TEXT("FSiegeCombatStatics::IsAgentVisibleTo(")), 1);
	}

	// ── (d) ⛔⛔ AND IT IS THE SAME RULE, NOT A SECOND ONE. This is the entire reason TASK-829 shipped
	//    IsAgentVisibleTo with a one-team/one-actor signature: the bot's iterators cannot route
	//    through a gather, so calling the same FUNCTION is the only way they can honour the same rule.
	//    Two implementations of "can this side see that unit" is the divergence WITCH-§1 exists to
	//    prevent, and it would show up in play as a unit hidden from towers and visible to the bot.
	const TCHAR* const SecondRuleTokens[] =
	{
		TEXT("bIsInvisible"),              // the flag itself — WITCH-§6: one source of truth, read through the door
		TEXT("IsInvisible()"),             // the actor-side reader — an inline read IS a second rule
		TEXT("FSiegeInvisibilityStatics"), // the symmetric predicate — its ONE call lives in SiegeCombatStatics.cpp
		TEXT("ESiegeVeilPolicy")           // the blast opt-out — the bot opts nothing in or out; that is the funnel's
	};

	for (const TCHAR* Token : SecondRuleTokens)
	{
		TestEqual(
			FString::Printf(
				TEXT("⛔⛔ SiegeBotController.cpp contains no `%s`. The bot asks the ONE shipped predicate and reads ")
				TEXT("NOTHING about the veil itself — no flag, no policy, no second copy of the rule. ⚠️ This zero is ")
				TEXT("meaningful ONLY because row (a) proved the same scanner sees this file; on its own it is ")
				TEXT("indistinguishable from a broken needle."),
				Token),
			CountOccurrencesInCode(Text, Token), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  21. ⛔⛔ THE TWO OMISSIONS ARE ⛔ DECIDED — the hero arm (J-W10) and the spawn clearance (J-W2).
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBotDecidedOmissionsTest,
	"Siegebound.Invisibility.TheBotsHeroArmAndSpawnClearanceAreDeliberatelyNotSuppressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBotDecidedOmissionsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⚖️ AN OMISSION A READER CANNOT TELL FROM AN OVERSIGHT WILL BE "FIXED" BY THE NEXT PERSON.
	// Both non-suppressed scans below are RULED, both carry a named comment, and this test pins the
	// omission so that "helpfully" adding a consult turns something RED instead of shipping.

	FString Text;
	if (!LoadProjectSource(*this, BotControllerCpp, Text))
	{
		return false;
	}

	// ── (a) THE HERO ARM. It shares a function with suppressed scan 1, so the proof has to be that
	//    the ONE consult in that body sits in the UNIT loop and the hero loop is in the SAME body
	//    without one. The `TActorIterator<AHeroCharacter>` row is the positive control that the
	//    extraction really did reach the second arm — without it, "one consult" would also be true
	//    of a body that got truncated before the hero loop even began.
	FString IntruderBody;
	if (ExtractFunctionBody(*this, Text, TEXT("AActor* ASiegeBotController::FindNearestEnemyIntruderOnBotHalf() const"), IntruderBody))
	{
		TestEqual(
			TEXT("SELF-CHECK: the extracted body really does contain BOTH arms — the hero loop is inside it, so ")
			TEXT("the single-consult count below is a claim about two loops rather than about a truncated read."),
			CountOccurrencesInCode(IntruderBody, TEXT("TActorIterator<AHeroCharacter>")), 1);

		TestEqual(
			TEXT("SELF-CHECK: …and the unit loop too."),
			CountOccurrencesInCode(IntruderBody, TEXT("TActorIterator<ASummonedUnit>")), 1);

		TestEqual(
			TEXT("⛔⛔ ONE consult for TWO loops: the UNIT arm is suppressed and the HERO arm is NOT. 🧑 J-W10 rules ")
			TEXT("the hero NOT veilable, so a consult in the hero loop would be DEAD CODE THAT CONTRADICTS A LIVE ")
			TEXT("RULING — it would read as if the hero could be hidden while never once being false."),
			CountOccurrencesInCode(IntruderBody, TEXT("FSiegeCombatStatics::IsAgentVisibleTo(")), 1);

		TestTrue(
			TEXT("⭐ …and the omission is NAMED at the site. The hero arm cites J-W10 in prose, so the next reader ")
			TEXT("knows it was DECIDED. ⛔ An unexplained omission is the one that gets 'fixed'."),
			IntruderBody.Contains(TEXT("J-W10"), ESearchCase::CaseSensitive));
	}

	// ── (b) THE SPAWN CLEARANCE. A physical occupancy test, not an act of seeing — the same category
	//    as the blast (J-W2). Three independent reasons, all in the site's comment: a veiled unit
	//    still has a capsule; the loop is NOT team-filtered (a consult would suppress only enemies and
	//    make a symmetric physics rule asymmetric); and suppressing it would let a player park a
	//    veiled unit in the bot's spawn box to make the bot spawn INSIDE it.
	FString ClearanceBody;
	if (ExtractFunctionBody(*this, Text, TEXT("bool ASiegeBotController::IsBotHalfPointClear("), ClearanceBody))
	{
		TestEqual(
			TEXT("SELF-CHECK: the extracted clearance body really does contain the unit loop, so the zero below is ")
			TEXT("a claim about a real scan rather than an empty read."),
			CountOccurrencesInCode(ClearanceBody, TEXT("TActorIterator<ASummonedUnit>")), 1);

		TestEqual(
			TEXT("⛔⛔ ZERO veil consults in the SPAWN CLEARANCE scan, and that is the RULING, not an oversight. ")
			TEXT("⚖️ Presence is not perception (J-W2): the veil hides a unit, it does not make it incorporeal. ")
			TEXT("⛔ Adding one here would suppress only ENEMIES in a loop that rejects live bodies of EITHER team, ")
			TEXT("and would re-open the identical-XY pile-up TASK-265 fixed."),
			CountOccurrencesInCode(ClearanceBody, TEXT("FSiegeCombatStatics::IsAgentVisibleTo(")), 0);

		TestTrue(
			TEXT("⭐ …and this omission is NAMED at the site too (J-W2)."),
			ClearanceBody.Contains(TEXT("J-W2"), ESearchCase::CaseSensitive));
	}

	// ── (c) ⭐⭐ THE DEAD-CODE PROOF FOR (a), TAKEN OFF THE SHIPPED PREDICATE RATHER THAN ASSERTED.
	//    IsAgentVisibleTo casts to ASummonedUnit and returns TRUE for everything that is not one.
	//    AHeroCharacter is not an ASummonedUnit ⇒ a consult in the hero arm could never once evaluate
	//    false. That is WHY the omission is correct, and it is measured here rather than argued.
	FString CombatText;
	if (LoadProjectSource(*this, CombatStaticsCpp, CombatText))
	{
		FString PredicateBody;
		if (ExtractFunctionBody(*this, CombatText, TEXT("bool FSiegeCombatStatics::IsAgentVisibleTo("), PredicateBody))
		{
			const int32 CastIndex = PredicateBody.Find(TEXT("Cast<ASummonedUnit>("), ESearchCase::CaseSensitive);
			const int32 CallIndex = PredicateBody.Find(TEXT("FSiegeInvisibilityStatics::IsVisibleTo("), ESearchCase::CaseSensitive);

			TestTrue(
				TEXT("⭐⭐ THE PREDICATE FILTERS BY CLASS BEFORE IT ASKS THE RULE: the Cast to ASummonedUnit precedes ")
				TEXT("the symmetric-predicate call, so every non-unit actor — the hero, a tower, a castle — takes the ")
				TEXT("early `return true`. ⇒ suppressing the bot's HERO arm would be provably dead code, which is ")
				TEXT("exactly why J-W10 leaves it alone rather than 'being safe'."),
				CastIndex != INDEX_NONE && CallIndex != INDEX_NONE && CastIndex < CallIndex);
		}
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  22. ⭐ THE RULE, IN THE EXACT SHAPE THE BOT CALLS IT — cross-team, veiled ⇒ NOT acquired.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityBotSeesWhatEveryUnitSeesTest,
	"Siegebound.Invisibility.TheBotAcquiresWithTheSameRuleAsEveryUnit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityBotSeesWhatEveryUnitSeesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔ The bot always calls the predicate the SAME way: viewer = its OWN team (BotTeam), target =
	// the unit under the iterator. The three rows below are that exact shape, run for BOTH possible
	// bot teams so the guarantee is not accidentally Red-shaped (the shipped bot is Red; nothing in
	// the rule may depend on that).
	//
	// ⚠️ These call FSiegeInvisibilityStatics::IsVisibleTo directly because IsAgentVisibleTo takes an
	// AActor* and there is no world to spawn one in (the house rule: not one SpawnActor anywhere in
	// Siegebound/Tests/). ⛔ The WIRING — that the bot reaches this rule at all, and reaches THIS one
	// rather than a copy — is tests 20 and 21's job; this test owns only the ANSWER it gets back.
	for (int32 Index = 0; Index < BothTeamsNum; ++Index)
	{
		const ETeamId TheBotsTeam = BothTeams[Index];
		const ETeamId ThePlayersTeam = (TheBotsTeam == ETeamId::Red) ? ETeamId::Blue : ETeamId::Red;

		TestFalse(
			FString::Printf(
				TEXT("⭐⭐ THE FEATURE, IN THE BOT'S OWN SHAPE: a %s bot asking about a VEILED %s unit gets ")
				TEXT("NOT-VISIBLE ⇒ the unit never enters the DEFEND read and never anchors a Fireball or Lightning ")
				TEXT("aim point."),
				TeamName(TheBotsTeam), TeamName(ThePlayersTeam)),
			FSiegeInvisibilityStatics::IsVisibleTo(TheBotsTeam, ThePlayersTeam, /*bTargetIsInvisible*/ true));

		TestTrue(
			FString::Printf(
				TEXT("⭐ …and the CONTROL that stops the row above passing for the wrong reason: the same %s bot ")
				TEXT("asking about an UNVEILED %s unit gets VISIBLE. ⛔ Without this, a predicate that answered ")
				TEXT("'hidden' to everything would pass — and would blind the bot completely."),
				TeamName(TheBotsTeam), TeamName(ThePlayersTeam)),
			FSiegeInvisibilityStatics::IsVisibleTo(TheBotsTeam, ThePlayersTeam, /*bTargetIsInvisible*/ false));

		TestTrue(
			FString::Printf(
				TEXT("⭐ …and a %s bot still sees its OWN veiled units (WITCH-§2 lane 4). The bot's scans filter by ")
				TEXT("enemy team before they ask, so this row is about the RULE staying one-sided rather than about ")
				TEXT("a path the bot takes — but a rule that hid a unit from its own side would break the moment a ")
				TEXT("friendly read is added."),
				TeamName(TheBotsTeam)),
			FSiegeInvisibilityStatics::IsVisibleTo(TheBotsTeam, TheBotsTeam, /*bTargetIsInvisible*/ true));
	}

	// ⛔⛔ AND THE HALF THAT IS ⛔ NOT SUPPRESSED, RESTATED HERE BECAUSE A DIFF THAT BROKE IT WOULD
	// DELETE THE CARD'S ONLY COUNTER. ⚖️ Choosing where to throw a Fireball is an ACT OF SEEING; the
	// explosion is NOT (J-W2). The bot cannot AIM at a veiled cluster — but a Fireball it aimed at
	// something else STILL CATCHES them, because ApplyRadialDamage asks for IncludeVeiled. That
	// exemption is asserted by test 17 and lives in a file TASK-851 never touched; this row only
	// proves the bot did not reach across and disturb it.
	FString BotText;
	if (LoadProjectSource(*this, BotControllerCpp, BotText))
	{
		TestEqual(
			TEXT("⛔ The bot names ApplyRadialDamage NOWHERE — it decides WHERE to cast and the resolver owns what ")
			TEXT("the blast hits. ⇒ suppressing the bot's aim CANNOT have narrowed any blast."),
			CountOccurrencesInCode(BotText, TEXT("ApplyRadialDamage")), 0);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  TASK-830 — THE WITCH'S CAST. ⛔ SAME SOURCE-PROBE LANE AND THE ⛔ SAME HONEST LIMIT: a cast
//  is a TIMER on an ACTOR in a WORLD, and the house rule forbids SpawnActor / CreateWorld in
//  Siegebound/Tests/, so ⛔ none of this drives a witch. What it pins is the ⛔ SHAPE: the veil
//  is granted on COMPLETION and nowhere else, damage cancels a CAST and never a VEIL, the
//  circle is the group's POSITION zone with the J-W5 fallback beneath it, and "visible" is read
//  through the ⛔ ONE shipped rule ⛔ from the enemy's side.
//  ⛔ Green here is ⛔ NOT "the witch works" (SC-§32) — that needs a PIE pass with a Witch card.
//
//  ⚠️⚠️ AND THE INSTRUMENT HAZARD TASK-829 MEASURED, HONOURED THROUGHOUT: CountOccurrencesInCode
//  SKIPS any line whose trimmed form starts with `//` or `/*`. Every needle below is asserted
//  against a value it can ⛔ actually reach — the guards these rows police all sit on lines that
//  also carry code, and every whole-tree scan carries a POSITIVE CONTROL in the same pass so a
//  dead instrument cannot report "safe".
// ═════════════════════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  23. ⭐⭐ THE GRANT DOOR HAS EXACTLY ONE CALLER, AND IT IS THE COMPLETED CAST.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityWitchIsTheOnlyGrantCallerTest,
	"Siegebound.Invisibility.TheCompletedWitchCastIsTheOnlyWayAUnitBecomesInvisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityWitchIsTheOnlyGrantCallerTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ⛔ THE NEEDLE CARRIES `->` ON PURPOSE: the DECLARATION reads `bool GrantInvisibility();` and
	// the DEFINITION `bool ASummonedUnit::GrantInvisibility()`, so neither can ever be counted as
	// a call. The witch is the only actor that hands a veil to somebody else, so the call is
	// always through a pointer.
	FString Where;
	const int32 GrantCalls = CountAcrossShippingSource(*this, TEXT("->GrantInvisibility()"), Where);
	if (GrantCalls < 0)
	{
		return false;
	}

	// ── THE POSITIVE CONTROL, FIRST. A scan that reads nothing would report the count below as a
	//    confident 0 or 1 and prove nothing at all.
	FString ControlWhere;
	const int32 ControlHits = CountAcrossShippingSource(*this, TEXT("GrantInvisibility"), ControlWhere);
	TestTrue(
		FString::Printf(
			TEXT("SELF-CHECK: the same pass DOES see `GrantInvisibility` in shipping source (%d hits, incl. the ")
			TEXT("declaration and definition). If this were zero the count below would be vacuous."), ControlHits),
		ControlHits > 0);

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ EXACTLY ONE call to the grant door in the whole of shipping Source/. ⛔ ZERO means the ")
			TEXT("feature is INERT — bIsInvisible can never be true, the suppression branch inside the funnel is ")
			TEXT("unreachable code, and every rule test in this file stays green while the card does nothing (that ")
			TEXT("was the state TASK-829 shipped and declared). ⛔ TWO means a second way to become invisible that ")
			TEXT("a grep for this name would have to find twice — WITCH-§6 keeps the flag private and unreflected ")
			TEXT("precisely so this count is the complete answer. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the witch grants nothing)") : *Where),
		GrantCalls, 1);

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString CompleteBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::CompleteWitchCast()"), CompleteBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted CompleteWitchCast body is substantial — an empty extraction would make ")
			TEXT("every assertion below pass vacuously."),
			CompleteBody.Len() > 200);

		TestEqual(
			TEXT("⭐ …and that ONE call lives in CompleteWitchCast — the timer's expiry path, i.e. the full ")
			TEXT("WitchCastSeconds elapsed with no interrupt. Paired with the count above, this pins both HOW MANY ")
			TEXT("and WHERE."),
			CountOccurrencesInCode(CompleteBody, TEXT("->GrantInvisibility()")), 1);

		// ⭐⭐ J-W7, AS A STRUCTURAL PROPERTY: "one at a time" is one concurrent CAST, ⛔ NOT one
		//    veiled unit ever. A witch who veils A, finishes, then veils B must leave BOTH
		//    invisible — so the completion path must never break a veil on ANY unit but herself.
		//    The alternative reading would arrive as `Subject->BreakInvisibility(...)` right here.
		TestEqual(
			TEXT("⛔⛔ J-W7: the completion path breaks NO other unit's veil. A witch who veils A and then B leaves ")
			TEXT("BOTH invisible — 'one at a time' is one concurrent CAST, never one veiled unit ever, and the ")
			TEXT("wrong reading would make the 50-gold card nearly worthless."),
			CountOccurrencesInCode(CompleteBody, TEXT("Subject->BreakInvisibility")), 0);

		TestEqual(
			TEXT("⭐ …while her OWN veil does break, exactly once, on this same completed act (J-W3 — she is acting)."),
			CountOccurrencesInCode(CompleteBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Cast)")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  24. ⛔⛔ DAMAGE INTERRUPTS A ***CAST***. IT DOES ⛔ NOT BREAK A ***VEIL***.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityDamageInterruptsTheCastOnlyTest,
	"Siegebound.Invisibility.DamageInterruptsTheCastForBothActorsAndBreaksNoVeil",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityDamageInterruptsTheCastOnlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString DamageBody;
	if (!ExtractFunctionBody(*this, Text,
		TEXT("float ASummonedUnit::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)"),
		DamageBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted TakeDamage body is substantial."),
		DamageBody.Len() > 200);

	// ⭐⭐ TWO CALLS, AND THE ⛔ SECOND ONE IS WHAT A NAIVE IMPLEMENTATION MISSES. His sentence names
	//    BOTH actors: "interupted if the witch OR unit that is turning invisible are attacked".
	//    Cancelling only the witch's own cast reads as complete, compiles, and ships a card where
	//    shooting the SUBJECT does nothing — which is half the counterplay gone.
	TestEqual(
		TEXT("⭐ TakeDamage cancels the cast THIS unit is performing (we are the witch)."),
		CountOccurrencesInCode(DamageBody, TEXT("CancelWitchCast(")), 1);

	TestEqual(
		TEXT("⭐⭐ …AND the cast being performed ON this unit (we are the subject). ⛔ His sentence names BOTH ")
		TEXT("actors, they are DIFFERENT OBJECTS, and no single call can cover both. A zero here ships a card ")
		TEXT("where shooting the unit that is turning invisible does nothing at all."),
		CountOccurrencesInCode(DamageBody, TEXT("InterruptIncomingWitchCast(")), 1);

	// ⛔ AND THE HALF THAT MUST STAY ABSENT — the merge WITCH-§3 warns about in writing.
	TestEqual(
		TEXT("⛔⛔ TakeDamage breaks NO veil. WITCH-§3 states the two rules on one row and then says not to merge ")
		TEXT("them: damage interrupts a CAST; being hit is NOT acting, so it does not break a VEIL. A 'for ")
		TEXT("symmetry' break here would make every veiled unit clipped by a stray AoE it cannot see coming ")
		TEXT("permanently visible."),
		CountOccurrencesInCode(DamageBody, TEXT("BreakInvisibility(")), 0);

	// ── AND THE CANCEL PATH ITSELF: an interrupted cast produces no veil, no partial state, no cost.
	FString CancelBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::CancelWitchCast(const TCHAR* Reason)"), CancelBody))
	{
		TestEqual(
			TEXT("⛔ The cancel path grants NO veil — WITCH-§4: an interrupted cast produces no veil, no partial ")
			TEXT("state and no cost."),
			CountOccurrencesInCode(CancelBody, TEXT("GrantInvisibility")), 0);

		TestEqual(
			TEXT("⛔ …and breaks none either. The WITCH's own veil is broken by a SUCCESSFUL cast only (J-W3): an ")
			TEXT("interrupted attempt is not an act that completed, so she must not pay for it."),
			CountOccurrencesInCode(CancelBody, TEXT("BreakInvisibility(")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  25. ⭐ ONE CAST AT A TIME, AND THE THREE NON-DAMAGE CANCELS.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityOneCastAtATimeTest,
	"Siegebound.Invisibility.OneCastAtATimeAndTheSubjectIsRevalidatedEveryPoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityOneCastAtATimeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString DriverBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::UpdateWitchCast()"), DriverBody))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the extracted UpdateWitchCast body is substantial."), DriverBody.Len() > 200);

	TestEqual(
		TEXT("⭐ The driver re-validates an in-flight cast against WITCH-§4's candidate rule every poll, so a ")
		TEXT("subject that dies, is veiled by another witch, or LEAVES THE POSITION CIRCLE cancels it."),
		CountOccurrencesInCode(DriverBody, TEXT("IsWitchVeilCandidate(")), 1);

	TestEqual(
		TEXT("⭐ …and the cancel is a CANCEL, not a retarget. WITCH-§4 says a cast CANCELS when its subject leaves ")
		TEXT("the circle; silently swapping to a nearer unit would make that rule unreachable."),
		CountOccurrencesInCode(DriverBody, TEXT("CancelWitchCast(")), 1);

	// ⭐⭐ "ONE AT A TIME" AS AN ORDERING PROPERTY, WHICH IS THE ONLY WAY IT IS CHECKABLE WITHOUT A
	//    WORLD: the latch test must come BEFORE the acquire, so the acquire is unreachable while a
	//    cast is live. An implementation that acquired first and then checked would start a second
	//    cast on a different subject and leak the first one's back-pointer.
	//    ⛔ Comment lines are stripped, so prose naming either token cannot fool the order test.
	const FString DriverCode = CodeLinesOnly(DriverBody);
	const int32 LatchIndex = DriverCode.Find(TEXT("IsCastingVeil()"), ESearchCase::CaseSensitive);
	const int32 AcquireIndex = DriverCode.Find(TEXT("FindWitchVeilTarget("), ESearchCase::CaseSensitive);

	TestTrue(
		TEXT("SELF-CHECK: both the latch and the acquire are present on CODE lines of UpdateWitchCast — an ")
		TEXT("absent token would make the ordering assertion below meaningless."),
		LatchIndex != INDEX_NONE && AcquireIndex != INDEX_NONE);

	if (LatchIndex != INDEX_NONE && AcquireIndex != INDEX_NONE)
	{
		TestTrue(
			TEXT("⭐⭐ The 'am I already casting?' latch is tested BEFORE the acquire, so a second concurrent cast ")
			TEXT("is UNREPRESENTABLE rather than merely guarded — his 'can only make one unit at a time invisible'."),
			LatchIndex < AcquireIndex);
	}

	// ── THE FOURTH CANCEL — "the witch is ordered away" — is at the ORDER, not on a poll.
	FString OrderBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::AssignCommandGroup(int32 GroupId, const FVector& StationOffset)"), OrderBody))
	{
		TestEqual(
			TEXT("⭐ A NEW ORDER cancels an in-flight cast at the press (WITCH-§4) — the order moves the position ")
			TEXT("circle out from under the subject, so waiting up to 0.25 s for a poll would read as lag."),
			CountOccurrencesInCode(OrderBody, TEXT("CancelWitchCast(")), 1);

		TestEqual(
			TEXT("⛔ …and being ORDERED still breaks NO veil (WITCH-§2 lane 4 — an invisible unit its own player ")
			TEXT("cannot command is a BUG). Only the CAST dies."),
			CountOccurrencesInCode(OrderBody, TEXT("BreakInvisibility(")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  26. ⭐⭐ THE POSITION CIRCLE — the group's POSITION zone, with the J-W5 fallback beneath it.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityPositionCircleTest,
	"Siegebound.Invisibility.ThePositionCircleIsTheGroupPositionZoneWithTheRangeFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityPositionCircleTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString CircleBody;
	if (!ExtractFunctionBody(*this, Text,
		TEXT("bool ASummonedUnit::ResolveWitchPositionCircle(FVector& OutCenter, float& OutRadius) const"), CircleBody))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the extracted ResolveWitchPositionCircle body is substantial."), CircleBody.Len() > 200);

	TestTrue(
		TEXT("⭐⭐ WITCH-§4: the circle IS FSiegeUnitGroup::PositionCenter / PositionRadius — his own shipped ")
		TEXT("vocabulary, the same phrase the miner ruling uses verbatim."),
		CountOccurrencesInCode(CircleBody, TEXT("PositionCenter")) > 0
		&& CountOccurrencesInCode(CircleBody, TEXT("PositionRadius")) > 0);

	// ⛔ THE WRONG CIRCLE, ASSERTED ABSENT. The ATTACK zone is the tier-1 ENGAGE trigger and a
	//    witch never engages; MARK-§'s circle_1..9 are war-map marks in WIDGET space. Both are
	//    plausible readings of "circle" and both are refuted by WITCH-§4 in writing.
	TestEqual(
		TEXT("⛔ …and NOT the ATTACK zone. AttackCenter/AttackRadius are the tier-1 ENGAGE trigger; a witch never ")
		TEXT("engages, and WITCH-§4 names the POSITION zone specifically."),
		CountOccurrencesInCode(CircleBody, TEXT("AttackCenter")) + CountOccurrencesInCode(CircleBody, TEXT("AttackRadius")), 0);

	// ⭐⭐ THE FOLLOW TRAP — the highest-value row here, because it is the COMMON case rather than
	//    an edge one. FSiegeUnitGroup is REUSED UNCHANGED for Follow, which carries
	//    PositionRadius == 0 and PositionCenter == ZeroVector; Follow is ALSO the SPAWN DEFAULT for
	//    every follow-eligible Blue unit, and the witch is follow-eligible. ⇒ a resolver that
	//    tested only `Group != nullptr` would centre her circle on the WORLD ORIGIN and she would
	//    never veil anybody, silently, forever.
	TestEqual(
		TEXT("⭐⭐⭐ A FOLLOW GROUP IS NOT A POSITION CIRCLE. Follow reuses FSiegeUnitGroup unchanged with ")
		TEXT("PositionRadius 0 / PositionCenter ZeroVector, and it is the SPAWN DEFAULT — so the common case is a ")
		TEXT("witch who HAS a group and has no circle. ⛔ Without this term her circle is centred on the world ")
		TEXT("origin and the card silently does nothing."),
		CountOccurrencesInCode(CircleBody, TEXT("ESiegeGroupCommandType::Follow")), 1);

	// ⭐ J-W5 — the ungrouped fallback is the card's own Range column, centred on herself.
	TestEqual(
		TEXT("⭐ J-W5: the ungrouped fallback is the card's own Range (AttackRange), with the tunable only as the ")
		TEXT("Range-0 backstop — a radius of 0 would be a 50-gold card that does nothing with nothing in any log."),
		CountOccurrencesInCode(CircleBody, TEXT("WitchVeilRadiusFallbackUU")), 1);

	TestTrue(
		TEXT("⭐ …and the fallback is CENTRED ON THE WITCH (GetActorLocation), which is the Cleric's shipped shape ")
		TEXT("— 'heals nearest damaged friendly … in 400' is a ring around the healer."),
		CountOccurrencesInCode(CircleBody, TEXT("GetActorLocation()")) > 0);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  27. ⭐⭐⭐ "VISIBLE" — READ THROUGH THE ONE SHIPPED RULE, AND FROM THE ***ENEMY'S*** SIDE.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityWitchTargetsOnlyStillVisibleUnitsTest,
	"Siegebound.Invisibility.TheWitchTargetsTheNearestUnitTheEnemyCanStillSee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityWitchTargetsOnlyStillVisibleUnitsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString CandidateBody;
	if (!ExtractFunctionBody(*this, Text,
		TEXT("bool ASummonedUnit::IsWitchVeilCandidate(const ASummonedUnit* Candidate, const FVector& CircleCenter, float CircleRadius) const"),
		CandidateBody))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the extracted IsWitchVeilCandidate body is substantial."), CandidateBody.Len() > 200);

	// ⛔⛔⛔ THE ROW THIS WHOLE TEST EXISTS FOR, AND THE ONE THAT IS EASY TO GET BACKWARDS.
	// FSiegeInvisibilityStatics::IsVisibleTo checks SAME-TEAM FIRST AND UNCONDITIONALLY (WITCH-§2
	// lane 4 — an invisible unit its own player cannot see is a BUG). ⇒ IsAgentVisibleTo(Team, X)
	// is TRUE FOR EVERY FRIENDLY, veiled or not, so a witch asking it with her OWN team filters
	// NOTHING and happily burns three seconds re-veiling somebody who is already invisible.
	// ⭐ Asking through the ENEMY's eyes is the only phrasing that has a veil in its answer — and
	// it is the card's own sentence: she veils the units the enemy can still see.
	TestEqual(
		TEXT("⭐⭐⭐ THE VEIL TERM IS ASKED WITH THE ***ENEMY'S*** TEAM. The shipped predicate answers 'visible' ")
		TEXT("UNCONDITIONALLY for a same-team query (WITCH-§2 lane 4), so IsAgentVisibleTo(Team, Candidate) would ")
		TEXT("be TRUE for every friendly — veiled or not — and would filter nothing at all."),
		CountOccurrencesInCode(CandidateBody, TEXT("IsAgentVisibleTo(EnemyTeam,")), 1);

	TestEqual(
		TEXT("⛔⛔ …and NEVER with her own. This is the exact backwards reading: it compiles, it reads sensibly, ")
		TEXT("and it silently deletes his word 'visible' from the targeting rule."),
		CountOccurrencesInCode(CandidateBody, TEXT("IsAgentVisibleTo(Team,")), 0);

	TestEqual(
		TEXT("⛔ …and 'already veiled' is NOT re-expressed as an inline IsInvisible() read. WITCH-§1 exists to hold ")
		TEXT("this rule at ONE consumer; a second expression would not inherit the day 'visible' grows a term."),
		CountOccurrencesInCode(CandidateBody, TEXT("IsInvisible()")), 0);

	// ⭐ The fourth WITCH-§4 term: not-currently-being-veiled — and the `!= this` that keeps a
	//   witch's own in-flight cast from disqualifying its own subject on the first validation pass.
	TestTrue(
		TEXT("⭐ WITCH-§4's 'not-currently-being-veiled' term is present, so two witches never both burn three ")
		TEXT("seconds on the same unit."),
		CountOccurrencesInCode(CandidateBody, TEXT("IncomingWitchCaster")) > 0);

	TestTrue(
		TEXT("⭐⭐ …and it exempts THIS caster. Without the `!= this` term the driver's own re-validation would ")
		TEXT("read its own in-flight cast as 'someone else is casting' and cancel it on the very next poll — a ")
		TEXT("witch who can start a cast and can never finish one."),
		CountOccurrencesInCode(CandidateBody, TEXT("!= this")) > 0);

	// ── AND THE FUNNEL IS STILL HELD AT ONE GUARD POINT. TASK-829's row pins the symmetric
	//    predicate at exactly two shipping mentions (its definition + the one call inside
	//    IsAgentVisibleTo); this task added a CONSUMER of IsAgentVisibleTo, never a second guard.
	FString Where;
	const int32 QualifiedHits = CountAcrossShippingSource(*this, TEXT("FSiegeInvisibilityStatics::IsVisibleTo("), Where);
	if (QualifiedHits < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ THE WITCH ADDED A CONSUMER, NOT A SECOND GUARD POINT: the symmetric predicate is still named ")
			TEXT("exactly TWICE in shipping source (its definition, and the one call inside IsAgentVisibleTo). ")
			TEXT("⛔ A third would be the pattern GHOST-§1 refused in writing and WITCH-§1 exists to hold at one. ")
			TEXT("Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		QualifiedHits, 2);

	// ── "NEAREST" — measured from the WITCH, which is his "walk up to NEARBY units". Ranking from
	//    the circle's centre instead would send her past a unit at her elbow to one across the zone.
	FString FindBody;
	if (ExtractFunctionBody(*this, Text,
		TEXT("ASummonedUnit* ASummonedUnit::FindWitchVeilTarget(const FVector& CircleCenter, float CircleRadius) const"), FindBody))
	{
		TestTrue(
			TEXT("⭐ The search ranks by distance from the WITCH (MyLocation), the Cleric's shipped ")
			TEXT("FindNearestDamagedFriendly shape — not from the circle's centre."),
			CountOccurrencesInCode(FindBody, TEXT("GetDistanceToTarget(MyLocation,")) > 0);

		TestEqual(
			TEXT("⭐ …and every candidate goes through the ONE predicate, so the acquire pass and the two ")
			TEXT("re-validation passes can never disagree about who qualifies."),
			CountOccurrencesInCode(FindBody, TEXT("IsWitchVeilCandidate(")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  28. ⛔⛔ THE ***CAST*** HAS A DURATION. THE ***VEIL*** DOES NOT.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityCastHasADurationTheVeilDoesNotTest,
	"Siegebound.Invisibility.TheCastHasADurationAndTheVeilStillDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityCastHasADurationTheVeilDoesNotTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString BeginBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::BeginWitchCast(ASummonedUnit* Subject)"), BeginBody))
	{
		return false;
	}

	// ⭐ THE CAST CLOCK IS READ IN EXACTLY ONE FUNCTION, expressed WITHOUT an absolute so the row
	//   cannot rot: whatever the whole file's count is, ALL of it lives in BeginWitchCast. A read
	//   anywhere else is the beginning of a veil that expires.
	const int32 FileSeconds = CountOccurrencesInCode(Text, TEXT("WitchCastSeconds"));
	const int32 BeginSeconds = CountOccurrencesInCode(BeginBody, TEXT("WitchCastSeconds"));

	TestTrue(
		FString::Printf(TEXT("SELF-CHECK: WitchCastSeconds is read at all in SummonedUnit.cpp (%d hits)."), FileSeconds),
		FileSeconds > 0);

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ EVERY read of the cast clock is inside BeginWitchCast (%d of %d). ⛔ A read anywhere else — ")
			TEXT("a re-arm, a 'refresh', a countdown displayed and then acted on — is how a CAST duration becomes a ")
			TEXT("VEIL duration, which WITCH-§3's 'permanently' forbids."),
			BeginSeconds, FileSeconds),
		BeginSeconds, FileSeconds);

	// ⛔ ONE-SHOT. A looping cast timer would re-veil the same subject every three seconds forever —
	//   a cooldown wearing a cast's clothes.
	// ⚠️ The needle sits on a line that also carries code (the FMath::Max argument), which is
	//   REQUIRED: CountOccurrencesInCode skips lines whose trimmed form starts with `/*`, so a
	//   named-argument comment on its own line would be invisible to this very assertion. The
	//   positive control immediately below proves the needle can be seen at all.
	TestEqual(
		TEXT("SELF-CHECK: the bLoop named-argument comment is on a CODE line and therefore visible to the scanner ")
		TEXT("— the TASK-829 instrument hazard, controlled for rather than assumed away."),
		CountOccurrencesInCode(BeginBody, TEXT("/*bLoop=*/")), 1);

	TestEqual(
		TEXT("⛔⛔ The cast timer is ONE-SHOT. A looping one would re-veil the same subject every WitchCastSeconds ")
		TEXT("forever, which is a cooldown wearing a cast's clothes — the exact drift WITCH-§3 forbids."),
		CountOccurrencesInCode(BeginBody, TEXT("/*bLoop=*/ false")), 1);

	TestEqual(
		TEXT("⛔ …and it is armed in exactly ONE place, so there is no second site that could arm it looping or ")
		TEXT("re-arm it mid-cast (which would silently extend a cast past his three seconds)."),
		CountOccurrencesInCode(Text, TEXT("SetTimer(WitchCastTimerHandle")), 1);

	// ── THE LATCH IS THE TIMER, NOT A SECOND BOOL. A bool left true by an early return would seal
	//    the witch forever; a bool left false would let her start a second cast.
	FString LatchBody;
	if (ExtractFunctionBody(*this, Text, TEXT("bool ASummonedUnit::IsCastingVeil() const"), LatchBody))
	{
		TestEqual(
			TEXT("⭐ The 'one at a time' latch reads the LIVE timer — one representation of the state, so it cannot ")
			TEXT("drift out of step with whether a cast is actually running."),
			CountOccurrencesInCode(LatchBody, TEXT("IsTimerActive(WitchCastTimerHandle)")), 1);
	}

	// ── THE TEARDOWN IS ONE FUNCTION, SO NEITHER END OF THE LINK CAN BE LEFT DANGLING BY ONE PATH.
	//    Asserted as "both exits use it" rather than as a tree-wide absolute, so the row does not
	//    rot the moment a third exit is added for a good reason.
	FString CancelBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::CancelWitchCast(const TCHAR* Reason)"), CancelBody))
	{
		TestEqual(
			TEXT("⭐ The CANCEL exit tears the channel down through the ONE shared teardown."),
			CountOccurrencesInCode(CancelBody, TEXT("ClearWitchCastChannel()")), 1);
	}

	FString CompleteBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::CompleteWitchCast()"), CompleteBody))
	{
		TestEqual(
			TEXT("⭐⭐ …and so does the COMPLETION exit. Both halves of the caster↔subject link are released by the ")
			TEXT("same function, so neither path can free one end and leak the other — a leaked back-pointer makes ")
			TEXT("a unit permanently un-targetable by every future witch, silently."),
			CountOccurrencesInCode(CompleteBody, TEXT("ClearWitchCastChannel()")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  29. ⛔⛔ THE WITCH MUST NEVER ENTER THE HEAL LANE — it carries a veil break she would trip.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityWitchNeverHealsTest,
	"Siegebound.Invisibility.TheWitchNeverEntersTheHealLaneThatWouldUnVeilHer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityWitchNeverHealsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	// ⛔⛔ THE DEFECT THIS ROW EXISTS FOR, IN ONE SENTENCE: the witch is ECardProfile::Support, the
	// Cleric's heal RATE is the row Damage, and her row Damage is 0 — so without a guard she would
	// arm the heal timer beside any damaged friendly and PerformHeal would call
	// BreakInvisibility(Heal) every 0.1 s to deliver ZERO HP. A veiled witch would un-veil herself
	// instantly for an act with no observable effect, and the report would read "the witch cannot
	// stay invisible" with nothing in the heal code wrong.
	FString HealTargetingBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("ASummonedUnit* ASummonedUnit::UpdateSupportHealTargeting()"), HealTargetingBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ The veil caster is refused the heal lane at its ONE shared entry — this function has exactly two ")
		TEXT("callers (UpdateStateSupport and the FOLLOW body) and a FOLLOWING witch is the common case, since ")
		TEXT("Follow is the spawn default."),
		CountOccurrencesInCode(HealTargetingBody, TEXT("IsVeilCaster()")), 1);

	// ⭐ ORDERING: the guard must precede the shipped four statements, or the heal timer is armed
	//   before it fires. Comment lines are stripped so prose cannot fool this.
	const FString HealCode = CodeLinesOnly(HealTargetingBody);
	const int32 GuardIndex = HealCode.Find(TEXT("IsVeilCaster()"), ESearchCase::CaseSensitive);
	const int32 AcquireIndex = HealCode.Find(TEXT("FindNearestDamagedFriendly()"), ESearchCase::CaseSensitive);

	TestTrue(
		TEXT("SELF-CHECK: both tokens are present on CODE lines — an absent one would make the order test vacuous."),
		GuardIndex != INDEX_NONE && AcquireIndex != INDEX_NONE);

	if (GuardIndex != INDEX_NONE && AcquireIndex != INDEX_NONE)
	{
		TestTrue(
			TEXT("⭐⭐ …and the guard sits ABOVE the Cleric's four extracted statements, so the heal timer is never ")
			TEXT("armed for her at all. Below them it would refuse the heal but leave the timer running."),
			GuardIndex < AcquireIndex);
	}

	// ── AND THE REASON IT MATTERS, ASSERTED RATHER THAN ASSERTED-IN-PROSE: the lane she is being
	//    kept out of really does carry a veil break.
	FString PerformHealBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::PerformHeal()"), PerformHealBody))
	{
		TestEqual(
			TEXT("SELF-CHECK / THE STAKES: PerformHeal really does break the veil (the HEALER's, TASK-829). That is ")
			TEXT("what a 0-HP witch 'heal' would trip every 0.1 s, and it is why the guard above is a defect fix ")
			TEXT("rather than tidiness."),
			CountOccurrencesInCode(PerformHealBody, TEXT("BreakInvisibility(ESiegeVeilBreakReason::Heal)")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  30. ⭐ THE CAST RUNS UNDER EVERY COMMAND, AND THE WITCH CAN BE GIVEN A POSITION CIRCLE AT ALL.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityWitchIsCommandableTest,
	"Siegebound.Invisibility.TheWitchIsZoneOrderableAndCastsUnderEveryCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityWitchIsCommandableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	// ⭐⭐ WITHOUT THIS TERM WITCH-§4's CENTRAL RULING IS UNREACHABLE CODE. The R/F stage-3 confirm
	// is the ONLY thing in the game that ever writes FSiegeUnitGroup::PositionCenter/PositionRadius,
	// so a witch who cannot take a zone order can never HAVE a position circle — every cast would
	// silently take the J-W5 ungrouped fallback forever and nobody would notice.
	FString ZoneOrderBody;
	if (ExtractFunctionBody(*this, Text, TEXT("bool ASummonedUnit::CanTakeZoneOrders() const"), ZoneOrderBody))
	{
		TestEqual(
			TEXT("⭐⭐ The witch is zone-orderable (R/F), which is what makes a POSITION CIRCLE reachable for her ")
			TEXT("at all. ⛔ Zero here and WITCH-§4's ruling is dead code — she would always fall back to J-W5."),
			CountOccurrencesInCode(ZoneOrderBody, TEXT("IsVeilCaster()")), 1);

		TestTrue(
			TEXT("⛔ …and the Cleric's shipped FOLLOW-ONLY ruling is untouched: the Standard term is still there, ")
			TEXT("and the witch term is an OR beside it rather than a replacement."),
			CountOccurrencesInCode(ZoneOrderBody, TEXT("ECardProfile::Standard")) == 1);
	}

	// ── THE DRIVER'S PLACEMENT: one call, ABOVE the dispatches, so the cast runs identically under
	//    Follow, Hold, Ambush, a latched stance, and no command at all. That is what makes his
	//    "controllable by all commands" cost zero per-body wiring.
	FString StateBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::UpdateState()"), StateBody))
	{
		TestEqual(
			TEXT("⭐ The cast driver is called exactly ONCE from the state poll."),
			CountOccurrencesInCode(StateBody, TEXT("UpdateWitchCast();")), 1);

		const FString StateCode = CodeLinesOnly(StateBody);
		const int32 DriverIndex = StateCode.Find(TEXT("UpdateWitchCast();"), ESearchCase::CaseSensitive);
		const int32 FollowIndex = StateCode.Find(TEXT("UpdateStateFollow("), ESearchCase::CaseSensitive);
		const int32 BodyIndex = StateCode.Find(TEXT("UpdateStateWitch();"), ESearchCase::CaseSensitive);

		TestTrue(
			TEXT("SELF-CHECK: the driver, the follow dispatch and the witch body are all present on CODE lines."),
			DriverIndex != INDEX_NONE && FollowIndex != INDEX_NONE && BodyIndex != INDEX_NONE);

		if (DriverIndex != INDEX_NONE && FollowIndex != INDEX_NONE && BodyIndex != INDEX_NONE)
		{
			TestTrue(
				TEXT("⭐⭐ …ABOVE the follow hoist and every profile/order dispatch — the TickStuckWatchdog placement ")
				TEXT("idiom. A witch under ANY order, or none, casts identically, and no body has to know about it. ")
				TEXT("⛔ Below the dispatches it would be dead for four of the five commands."),
				DriverIndex < FollowIndex && DriverIndex < BodyIndex);

			TestTrue(
				TEXT("⛔ …and her MOVEMENT body sits BELOW the zone-order dispatch, so a zone-ordered witch is run by ")
				TEXT("the shipped UpdateStateGrouped (station-keeping inside her circle via the sealed sorcerer ")
				TEXT("path) rather than by her ungrouped body."),
				FollowIndex < BodyIndex);
		}

		TestEqual(
			TEXT("⛔ …and she never reaches the legacy Standard acquire-and-march body: her own body returns above ")
			TEXT("the stance gate. A 0-damage support unit dropped into the attack machine would walk into the ")
			TEXT("enemy fleet and stand there."),
			CountOccurrencesInCode(StateBody, TEXT("UpdateStateWitch();")), 1);
	}

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════════
//  TASK-830 ITEM (8) — THE CAST'S ⛔ READ-ONLY SURFACE (law WITCH-§9.1 / §9.2 / §9.3 / §9.6).
//
//  ⛔⛔ WHY THE SURFACE EXISTS: SK_Witch does not exist, so the witch spawns STATIC, and on a
//  3-second INTERRUPTIBLE channel "starting", "running" and "broken" are ⛔ IDENTICAL PIXELS.
//  Jonathan did not ask for a 3-second cast; he asked for one ⛔ THAT CAN BE INTERRUPTED — and
//  counterplay the player cannot perceive is not counterplay (WITCH-§9).
//
//  ⭐⭐ THE TWO ROWS BELOW ARE THE TWO THE SPEC NAMES, AND THEY GUARD OPPOSITE FAILURES:
//    · 31 guards the FLEET — every non-witch must answer through the DEFAULT and render
//      pixel-identically to today. WITCH-§9.6 calls that a REQUIREMENT, not an expectation.
//    · 32 guards the FEATURE — an interrupt must reach the surface as "false, mid-flight",
//      never as "false, completed". That distinction IS requirement 4, the one perception with
//      ⛔ no tell at all in the shipped game.
//
//  ⚠️ BOTH ROWS RUN REAL C++ ON CLASS DEFAULT OBJECTS rather than only reading source text. That
//  is possible here — and only here in this file — precisely because the accessors are ⛔ const
//  and ⛔ world-guarded: a CDO has no world (AActor::GetWorld's RF_ClassDefaultObject early-out),
//  so an unguarded implementation would ⛔ CRASH the runner instead of failing it.
// ═════════════════════════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  31. ⭐⭐ THE SURFACE IS TWO *DEFAULTED* VIRTUALS, AND EVERY NON-WITCH ANSWERS THROUGH THEM.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityCastSurfaceIsDefaultedForTheWholeFleetTest,
	"Siegebound.Invisibility.EveryNonWitchAnswersTheCastSurfaceThroughTheDefaultedVirtual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityCastSurfaceIsDefaultedForTheWholeFleetTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	// ══ (A) THE BEHAVIOURAL HALF — the accessors are CALLED, not merely read as text ══════════
	// ⛔ CDOs, because the house rule bans SpawnActor/CreateWorld under Siegebound/Tests/. That
	//    limitation is what makes this lane structural everywhere else in this file; here it is an
	//    ADVANTAGE, because a CDO is exactly the world-less caller the defaults must survive.
	const ABuilding* const BuildingDefaults = GetDefault<ABuilding>();
	const AHeroCharacter* const HeroDefaults = GetDefault<AHeroCharacter>();
	const ASummonedUnit* const UnitDefaults = GetDefault<ASummonedUnit>();

	if (!BuildingDefaults || !HeroDefaults || !UnitDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a GetDefault<>() lookup returned null — every assertion below would be vacuous."));
		return false;
	}

	const IHealthBarProvider* const BuildingProvider = Cast<IHealthBarProvider>(BuildingDefaults);
	const IHealthBarProvider* const HeroProvider = Cast<IHealthBarProvider>(HeroDefaults);
	const IHealthBarProvider* const UnitProvider = Cast<IHealthBarProvider>(UnitDefaults);

	TestTrue(
		TEXT("SELF-CHECK: all three CDOs really do resolve to IHealthBarProvider — without this the calls below ")
		TEXT("would be skipped and their 'false / 0' results would prove nothing at all."),
		BuildingProvider != nullptr && HeroProvider != nullptr && UnitProvider != nullptr);

	if (!BuildingProvider || !HeroProvider || !UnitProvider)
	{
		return false;
	}

	// ── ⭐⭐ THE POSITIVE CONTROL, AND IT IS THE ONE THAT MATTERS: it proves the DEFAULTED-VIRTUAL
	//    MECHANISM ITSELF DIFFERENTIATES on these very objects. `GetDamageBoostChangedDelegate` is
	//    the shipped precedent this surface is modelled on — ASummonedUnit OVERRIDES it (non-null)
	//    while ABuilding and AHeroCharacter keep the interface default (nullptr).
	//    ⛔ Without this, "everything answered false/0" is indistinguishable from "virtual dispatch
	//    never reached anything", which is precisely the blind-instrument failure SC-§39 exists for.
	IHealthBarProvider* const MutableUnitProvider = Cast<IHealthBarProvider>(GetMutableDefault<ASummonedUnit>());
	IHealthBarProvider* const MutableBuildingProvider = Cast<IHealthBarProvider>(GetMutableDefault<ABuilding>());

	if (MutableUnitProvider && MutableBuildingProvider)
	{
		TestTrue(
			TEXT("SELF-CHECK / THE CONTROL: on the SAME CDOs, the shipped defaulted pair this surface copies really ")
			TEXT("does discriminate — ASummonedUnit's OVERRIDE returns a real delegate while ABuilding takes the ")
			TEXT("interface DEFAULT (nullptr). So virtual dispatch is live and 'defaulted' genuinely means ")
			TEXT("'answered without an override'."),
			MutableUnitProvider->GetDamageBoostChangedDelegate() != nullptr
			&& MutableBuildingProvider->GetDamageBoostChangedDelegate() == nullptr);
	}

	// ── AND NOW THE ROW THE SPEC ASKS FOR: a non-witch answers false / 0 THROUGH THE DEFAULT.
	//    ⛔ A building and the hero have no override at all; the unit CDO reaches ASummonedUnit's
	//    override, whose world guard must return the same answer rather than dereferencing null.
	TestFalse(
		TEXT("⭐⭐ ABuilding is NOT casting — through the interface DEFAULT, with ZERO changes to Building.{h,cpp}. ")
		TEXT("WITCH-§9.6: a pixel-identical non-witch bar is a REQUIREMENT. Every tower, wall, Barracks and Deep ")
		TEXT("Mine in the game renders exactly as it does today."),
		BuildingProvider->IsCastInProgress());

	TestEqual(
		TEXT("⭐ …and its cast fill is EXACTLY zero, so the row seeds empty and stays collapsed."),
		BuildingProvider->GetCastProgressPercent(), 0.f);

	TestFalse(
		TEXT("⭐⭐ AHeroCharacter is NOT casting — same default, same zero changes. ⚠️ The hero's RECALL is a real ")
		TEXT("3-part channel, and WITCH-§9.6 rules it explicitly OUT of scope: J-W10 keeps the hero unveilable, so ")
		TEXT("a Recall bar would be a general cast-bar framework nobody asked for."),
		HeroProvider->IsCastInProgress());

	TestEqual(
		TEXT("⭐ …and the hero's cast fill is EXACTLY zero."),
		HeroProvider->GetCastProgressPercent(), 0.f);

	// ⛔⛔ THE ROW THAT WOULD HAVE CRASHED THE RUNNER RATHER THAN FAILED IT. ASummonedUnit DOES
	//    override both, and its override reads the LIVE cast timer — via AActor::GetWorldTimerManager,
	//    which is a bare `GetWorld()->GetTimerManager()`. A CDO's GetWorld() is nullptr BY
	//    CONSTRUCTION. ⇒ this pair is simultaneously the "non-witch reads false/0" row AND the
	//    negative control proving the world guard in ResolveCastClockOwner is real.
	TestFalse(
		TEXT("⭐⭐ The ASummonedUnit CDO — a unit CLASS rather than a unit, with no world and no timer — answers ")
		TEXT("NOT CASTING rather than dereferencing a null world. ⛔ If this row ever CRASHES instead of failing, ")
		TEXT("the class-default-object guard has been removed from ResolveCastClockOwner."),
		UnitProvider->IsCastInProgress());

	TestEqual(
		TEXT("⭐ …and it reads exactly 0 percent, on the same guard."),
		UnitProvider->GetCastProgressPercent(), 0.f);

	// ══ (B) THE STRUCTURAL HALF — DEFAULTED, never pure, and overridden in exactly ONE class ══
	FString InterfaceText;
	if (!LoadProjectSource(*this, TEXT("Source/GitClaudeUnrealTest/Siegebound/HealthBarProvider.h"), InterfaceText))
	{
		return false;
	}

	// ⭐ THE POSITIVE CONTROL FOR THE PURE-VIRTUAL NEEDLE SHAPE, FIRST: this interface really does
	//   carry `= 0;` methods, and the scanner really does see them. Otherwise the two zeros below
	//   would be indistinguishable from a needle that can never match anything.
	TestEqual(
		TEXT("SELF-CHECK: the `= 0;` needle shape IS visible on this interface — GetHealthCurrent is genuinely pure."),
		CountOccurrencesInCode(InterfaceText, TEXT("virtual float GetHealthCurrent() const = 0;")), 1);

	TestEqual(
		TEXT("SELF-CHECK: and the DEFAULTED needle shape is visible too — GetDamageBoostPercent is the shipped ")
		TEXT("precedent this pair copies (TASK-362), so both halves of the comparison are proven readable."),
		CountOccurrencesInCode(InterfaceText, TEXT("virtual float GetDamageBoostPercent() const { return 0.f; }")), 1);

	TestEqual(
		TEXT("⭐⭐ GetCastProgressPercent is DEFAULTED to 0.f on the interface. ⛔ DEFAULTED is the load-bearing ")
		TEXT("word: pure would force ABuilding, AHeroCharacter and every future provider to implement a witch ")
		TEXT("mechanic they have no concept of, which is the opposite of WITCH-§9.6's fleet-wide plumbing rule."),
		CountOccurrencesInCode(InterfaceText, TEXT("virtual float GetCastProgressPercent() const { return 0.f; }")), 1);

	TestEqual(
		TEXT("⭐⭐ IsCastInProgress is DEFAULTED to false on the interface, same reasoning."),
		CountOccurrencesInCode(InterfaceText, TEXT("virtual bool IsCastInProgress() const { return false; }")), 1);

	TestEqual(
		TEXT("⛔ …and NEITHER is pure. A `= 0;` here would not merely be a style choice: it would break the build ")
		TEXT("of every provider in the game and force a witch-shaped method onto a wall."),
		CountOccurrencesInCode(InterfaceText, TEXT("virtual float GetCastProgressPercent() const = 0;"))
		+ CountOccurrencesInCode(InterfaceText, TEXT("virtual bool IsCastInProgress() const = 0;")), 0);

	// ── EXACTLY ONE OVERRIDING CLASS IN THE WHOLE TREE. This is the pixel-identical guarantee
	//    stated as a MEASUREMENT rather than as a promise: if a second class ever overrides these,
	//    a second actor type starts drawing a cast row and the fleet is no longer unchanged.
	FString PercentWhere;
	FString CastingWhere;
	const int32 PercentOverrides = CountAcrossShippingSource(*this, TEXT("virtual float GetCastProgressPercent() const override;"), PercentWhere);
	const int32 CastingOverrides = CountAcrossShippingSource(*this, TEXT("virtual bool IsCastInProgress() const override;"), CastingWhere);

	if (PercentOverrides < 0 || CastingOverrides < 0)
	{
		return false;
	}

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ EXACTLY ONE class overrides GetCastProgressPercent — ASummonedUnit, the only class in the ")
			TEXT("game that can channel. ⛔ TWO means a second actor type grew a cast bar without a ruling; ⛔ ZERO ")
			TEXT("means the witch answers through the default and her own tell is inert. Found:%s"),
			PercentWhere.IsEmpty() ? TEXT(" (nowhere — the witch has no tell)") : *PercentWhere),
		PercentOverrides, 1);

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ …and exactly one overrides IsCastInProgress, in the same class. A split — one overridden and ")
			TEXT("not the other — is a bar whose GATE and whose FILL disagree. Found:%s"),
			CastingWhere.IsEmpty() ? TEXT(" (nowhere)") : *CastingWhere),
		CastingOverrides, 1);

	// ── AND THE TWO HEADERS THE LAW NAMES BY NAME, ASSERTED DIRECTLY, because "one override
	//    tree-wide" would still pass if the one lived in the wrong class.
	const TCHAR* const UntouchedProviderHeaders[] =
	{
		TEXT("Source/GitClaudeUnrealTest/Siegebound/Building.h"),
		TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h")
	};

	for (const TCHAR* const HeaderPath : UntouchedProviderHeaders)
	{
		FString HeaderText;
		if (!LoadProjectSource(*this, HeaderPath, HeaderText))
		{
			continue;
		}

		// ⭐ CONTROL FIRST: the file really was read and really is a provider — otherwise the two
		//   zeros below are the "I searched and found nothing" non-evidence SC-§40 cl. 1 refuses.
		TestTrue(
			FString::Printf(TEXT("SELF-CHECK: '%s' was read and DOES implement IHealthBarProvider, so the zeros below mean something."), HeaderPath),
			CountOccurrencesInCode(HeaderText, TEXT("IHealthBarProvider")) > 0
			&& CountOccurrencesInCode(HeaderText, TEXT("GetHealthCurrent")) > 0);

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ '%s' names the cast surface ZERO times — WITCH-§9.6's pixel-identical requirement, as a ")
				TEXT("measurement. ⛔ It also mirrors the shipped boost precedent exactly: neither header names ")
				TEXT("GetDamageBoostPercent either, and both bars have rendered correctly for a hundred tasks."),
				HeaderPath),
			CountOccurrencesInCode(HeaderText, TEXT("GetCastProgressPercent"))
			+ CountOccurrencesInCode(HeaderText, TEXT("IsCastInProgress")), 0);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  32. ⭐⭐ AN INTERRUPT REACHES THE SURFACE AS "FALSE, MID-FLIGHT" — never as "false, completed".
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityInterruptIsDistinguishableAtTheCastSurfaceTest,
	"Siegebound.Invisibility.AnInterruptedCastNeverReachesFullAtTheCastSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityInterruptIsDistinguishableAtTheCastSurfaceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString Text;
	if (!LoadProjectSource(*this, SummonedUnitCpp, Text))
	{
		return false;
	}

	FString PercentBody;
	if (!ExtractFunctionBody(*this, Text, TEXT("float ASummonedUnit::GetCastProgressPercent() const"), PercentBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted GetCastProgressPercent body is substantial — an empty extraction would make ")
		TEXT("every assertion below pass vacuously."),
		PercentBody.Len() > 200);

	// ══ (1) THE FILL IS DERIVED FROM THE LIVE CLOCK, SO IT CANNOT SURVIVE THE INTERRUPT ══════
	// ⭐⭐ THIS IS THE WHOLE MECHANISM. An interrupt runs CancelWitchCast -> ClearWitchCastChannel
	//    -> ClearTimer. Because the percent is COMPUTED from that timer rather than remembered,
	//    the very next read is 0 and IsCastInProgress() is false, with NOTHING to reset. A cached
	//    percent would survive the cancel and freeze the bar mid-flight — a tell that says a cast
	//    is still running after the player has already interrupted it, which is worse than no tell.
	TestEqual(
		TEXT("⭐ The elapsed half of the fill is read from the LIVE timer, exactly once."),
		CountOccurrencesInCode(PercentBody, TEXT("GetTimerElapsed(")), 1);

	TestEqual(
		TEXT("⭐⭐ …and so is the DENOMINATOR. ⛔ This is the row that is easy to get wrong in the reassuring ")
		TEXT("direction: dividing by the WitchCastSeconds tunable compiles, reads sensibly, and is wrong twice — ")
		TEXT("(a) BeginWitchCast arms with FMath::Max(tunable, 0.05f), so a row tuned below the floor makes the ")
		TEXT("tunable the WRONG denominator and the fill runs past 100 with the clamp hiding it; and (b) the ")
		TEXT("cast-duration row above pins EVERY read of that tunable in this file inside BeginWitchCast, so the ")
		TEXT("obvious phrasing turns a GREEN test RED under a name that mentions neither this surface nor its task."),
		CountOccurrencesInCode(PercentBody, TEXT("GetTimerRate(")), 1);

	TestEqual(
		TEXT("⛔⛔ …and the tunable is read ZERO times here. Stated as its own row because the coupling above is ")
		TEXT("invisible from this file: a future author 'simplifying' the divide would reintroduce both defects at ")
		TEXT("once. (The whole-file/BeginWitchCast equality is asserted by the cast-duration row; this one says why ")
		TEXT("THIS body must stay out of it.)"),
		CountOccurrencesInCode(PercentBody, TEXT("WitchCastSeconds")), 0);

	// ── THE CLAMP: the surface can never report MORE than full, so "the fill reached the end" is
	//    only ever produced by a cast that ran its whole window. That asymmetry IS requirement 4.
	TestEqual(
		TEXT("⭐⭐ The fill is clamped, and its ceiling is the 0..100 convention's own — so a broken cast reports ")
		TEXT("wherever it broke and a completed one is the only thing that can look full (WITCH-§9.1 row 4: ")
		TEXT("COMPLETED vs BROKEN is the one perception with no tell at all in the shipped game)."),
		CountOccurrencesInCode(PercentBody, TEXT("FMath::Clamp(")), 1);

	TestEqual(
		TEXT("⛔ …and the range is 0..100, NEVER 0..1 — the shipped BoostPercent convention (WITCH-§9.3). A 0..1 ")
		TEXT("producer read by a /100 consumer paints a bar that never visibly moves."),
		CountOccurrencesInCode(PercentBody, TEXT(", 0.f, 100.f)")), 1);

	// ══ (2) THE UNIT STORES NO COPY OF THE TELL — the second-source-of-truth ban ══════════════
	// ⛔ A remembered percent or a second bool beside the timer is how "an interrupt leaves NO
	//    partial state" (WITCH-§4) stops being true of the TELL while staying true of the veil.
	//
	// ⛔⛔ SCOPED TO `SummonedUnit.h` DELIBERATELY, AND THE SCOPING IS THE POINT — a whole-tree ban
	//    on these names would be a COUNT-PIN TRAP AIMED AT THE NEXT TASK. TASK-860's component will
	//    legitimately write `const float CastPercent = Provider->GetCastProgressPercent();` and
	//    `const bool bIsCasting = Provider->IsCastInProgress();` as ORDINARY LOCALS, and a tree-wide
	//    scan would turn RED because a downstream caller EXISTED — in a test naming neither that
	//    task nor its file. ⚖️ A LOCAL that reads the surface is not a second source of truth;
	//    a MEMBER on the actor that owns the cast IS. ⭐ A header can only declare members, so
	//    scoping here bans exactly the defect and nothing else.
	FString UnitHeader;
	if (!LoadProjectSource(*this, SummonedUnitHeader, UnitHeader))
	{
		return false;
	}

	// ⭐ THE POSITIVE CONTROL FIRST: the probe really can see a member declaration in this header.
	//   Without it the zeros below are the "I looked and found nothing" non-evidence SC-§40 cl. 1
	//   refuses — and the thing it finds is the one piece of state the cast is ALLOWED to keep.
	TestEqual(
		TEXT("SELF-CHECK: the scan DOES see a cast-state member declaration in SummonedUnit.h — the one-shot timer ")
		TEXT("handle, which is the ONLY thing the cast stores and the thing both ends of the bar read."),
		CountOccurrencesInCode(UnitHeader, TEXT("FTimerHandle WitchCastTimerHandle;")), 1);

	const TCHAR* const BannedStoredTellMembers[] =
	{
		TEXT("CachedCastPercent"),
		TEXT("LastCastPercent"),
		TEXT("StoredCastPercent"),
		TEXT("CastProgressPercent;"),
		TEXT("bCastInProgress"),
		TEXT("bIsCasting")
	};

	for (const TCHAR* const Banned : BannedStoredTellMembers)
	{
		TestEqual(
			FString::Printf(
				TEXT("⛔ `%s` is declared NOWHERE on ASummonedUnit. The cast tell is DERIVED per call, never stored — ")
				TEXT("a cached copy survives the cancel and strands the bar mid-flight, and a second bool beside the ")
				TEXT("timer is the same drift the 'one at a time' latch already refuses by reading the LIVE handle."),
				Banned),
			CountOccurrencesInCode(UnitHeader, Banned), 0);
	}

	// ⭐ AND THE SURFACE ITSELF IS PRESENT — otherwise every absence above would be satisfied by a
	//   feature that simply does not exist. ⛔ `> 0` rather than an absolute ON PURPOSE: TASK-860 is
	//   about to add CALLERS, and a pinned integer here would go red for the crime of being consumed.
	FString ControlWhere;
	const int32 ControlHits = CountAcrossShippingSource(*this, TEXT("IsCastInProgress"), ControlWhere);
	if (ControlHits < 0)
	{
		return false;
	}

	TestTrue(
		FString::Printf(
			TEXT("SELF-CHECK: a whole-tree pass finds `IsCastInProgress` in shipping source (%d hits — the interface ")
			TEXT("default, the override and the definition, plus any consumer). If this were zero the surface would ")
			TEXT("not exist and every row above would be vacuous. Found:%s"),
			ControlHits, ControlWhere.IsEmpty() ? TEXT(" (nowhere — the surface is absent)") : *ControlWhere),
		ControlHits > 0);

	// ══ (3) TWO-ENDED, AND THE SUBJECT *PULLS* ═══════════════════════════════════════════════
	FString ResolverBody;
	if (ExtractFunctionBody(*this, Text, TEXT("const ASummonedUnit* ASummonedUnit::ResolveCastClockOwner() const"), ResolverBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted ResolveCastClockOwner body is substantial."),
			ResolverBody.Len() > 200);

		TestEqual(
			TEXT("⭐⭐ THE SUBJECT END EXISTS. ⛔ Dropping this reduces the tell to ONE bar on the caster — the cheap ")
			TEXT("default every game ships, which WITCH-§9.2 refuses BY NAME because it fails requirement 2, and ")
			TEXT("requirement 2 is Jonathan's own sentence: 'interupted if the witch OR unit that is turning ")
			TEXT("invisible are attacked.' A counter you cannot aim is not a counter."),
			CountOccurrencesInCode(ResolverBody, TEXT("IncomingWitchCaster")), 1);

		TestEqual(
			TEXT("⭐⭐ …and BOTH ends gate on the SAME live latch, so the witch's bar and her subject's bar can never ")
			TEXT("disagree about whether a cast is running. Two derivations would be two clocks."),
			CountOccurrencesInCode(ResolverBody, TEXT("IsCastingVeil()")), 2);

		TestEqual(
			TEXT("⛔ …and the subject re-checks that the caster still points BACK at it. The back-pointer alone is ")
			TEXT("not enough: ClearWitchCastChannel deliberately releases a subject's link only when it still names ")
			TEXT("the clearing witch, so a subject re-targeted by a SECOND witch keeps a link the first no longer ")
			TEXT("owns — and a bar would be painted for a cast no longer aimed here."),
			CountOccurrencesInCode(ResolverBody, TEXT("WitchCastTarget")), 1);

		// ⛔⛔ THE WORLD GUARD. Without it this surface CRASHES rather than fails on any world-less
		//    caller, and the interface makes world-less callers reachable from anywhere.
		TestTrue(
			TEXT("⛔⛔ The class-default-object / world guard is present. AActor::GetWorld() returns nullptr for a CDO ")
			TEXT("by construction and GetWorldTimerManager() is a bare GetWorld()->GetTimerManager(), so removing ")
			TEXT("this turns test 31's non-witch rows from a FAILURE into a CRASH."),
			CountOccurrencesInCode(ResolverBody, TEXT("GetWorld() == nullptr")) >= 1);
	}

	// ══ (4) ⛔ NOTHING ON THE CANCEL PATH TOUCHES THE SURFACE ═════════════════════════════════
	// The tell must fall out of the timer teardown that already exists — never out of a second
	// write somebody has to remember to make. This is WITCH-§9.3's "no push" rule, checked from
	// the inside: if the cast lifecycle ever WRITES the tell, the tell has become state.
	const TCHAR* const LifecycleSignatures[] =
	{
		TEXT("void ASummonedUnit::BeginWitchCast(ASummonedUnit* Subject)"),
		TEXT("void ASummonedUnit::CancelWitchCast(const TCHAR* Reason)"),
		TEXT("void ASummonedUnit::InterruptIncomingWitchCast(const TCHAR* Reason)"),
		TEXT("void ASummonedUnit::ClearWitchCastChannel()"),
		TEXT("void ASummonedUnit::CompleteWitchCast()")
	};

	for (const TCHAR* const Signature : LifecycleSignatures)
	{
		FString LifecycleBody;
		if (!ExtractFunctionBody(*this, Text, Signature, LifecycleBody))
		{
			continue;
		}

		TestEqual(
			FString::Printf(
				TEXT("⛔ `%s` names the cast surface ZERO times — the tell is a WINDOW onto the cast, never a thing ")
				TEXT("the cast has to keep up to date. One write missed here is a bar stranded on a unit forever."),
				Signature),
			CountOccurrencesInCode(LifecycleBody, TEXT("GetCastProgressPercent"))
			+ CountOccurrencesInCode(LifecycleBody, TEXT("IsCastInProgress")), 0);
	}

	// ── AND THE ONE POSITIVE CLAIM THAT PAIRS WITH THOSE FIVE ZEROS: the teardown really does clear
	//    the clock, which is the single mechanism that drives the surface false on an interrupt.
	FString TeardownBody;
	if (ExtractFunctionBody(*this, Text, TEXT("void ASummonedUnit::ClearWitchCastChannel()"), TeardownBody))
	{
		TestEqual(
			TEXT("⭐⭐ SELF-CHECK / THE STAKES: the ONE teardown really does clear the cast timer. That single line is ")
			TEXT("what turns an interrupt into 'false' at the surface — for BOTH ends at once, since both read that ")
			TEXT("same handle. ⛔ Zero here and an interrupted cast would leave two bars running forever."),
			CountOccurrencesInCode(TeardownBody, TEXT("ClearTimer(WitchCastTimerHandle)")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
//  33. ⭐⭐⭐ TASK-923 — THE MATERIAL SWAP: ⛔ BOTH EDGES, ⛔ EVERY SLOT, ⛔ NEVER SLOT 1.
//
//  ⛔⛔⛔ READ THIS BEFORE TRUSTING A GREEN, BECAUSE THE HONESTY IS THE DELIVERABLE HERE AND
//  ⛔ NOT THE COUNT (`TASK-923(7)`):
//
//  ⛔ WHAT THIS TEST ⛔ CANNOT DO: it ⛔ CANNOT prove the veil LOOKS like anything. A material
//  swap is a RENDERING fact — it needs a mesh, a material that has actually compiled, a scene
//  and a frame. There is ⛔ not one `UWorld::CreateWorld` and ⛔ not one `SpawnActor` anywhere in
//  `Siegebound/Tests/` (the house rule), and even with one, `TASK-832` measured that this exact
//  effect is invisible to every instrument except a ⛔ REGION-RESTRICTED FRAME DIFF AGAINST A
//  STRENGTH-0 CONTROL — a thumbnail could not see it, and neither could the artist by eye.
//  ⇒ ⛔⛔ **THIS ROW IS ⛔ PIXEL-GATED AND IS ⛔ DECLARED AS SUCH.** ⛔ Green here must ⛔ NEVER be
//  reported as "the veil works" (`SC-§32`). It is reported as "the swap is WIRED, on both edges,
//  on every slot, and nobody has quietly removed it".
//
//  ⭐⭐ WHAT IT ⛔ CAN DO, AND WHY IT IS ⛔ NOT THE SIXTH BLIND INSTRUMENT: `TASK-923(7)` demands
//  at least one row that goes ⛔ RED IF THE SWAP IS REMOVED, and forbids a test that merely
//  asserts the material PATH STRING and calls that coverage. Every row below is built against a
//  ⛔ NAMED WRONG IMPLEMENTATION that a reviewer could plausibly ship:
//    · ⛔ the swap deleted            → rows (1a)/(1b) go red
//    · ⛔ the RESTORE deleted         → row (2a) goes red   ⚖️ the failure that is WORSE than no veil
//    · ⛔ restore-to-default, no team → row (2b)/(2c) go red ⚖️ a RED unit comes back BLUE
//    · ⛔ a slot index hard-coded     → rows (3a)/(3b)/(3c) go red ⚖️ the opaque chrome hat
//    · ⛔ `SetVisibility(false)` used → row (4) goes red     ⚖️ the unkillable invisible solid
//    · ⛔ a cached "originals" array  → row (5) goes red
//    · ⛔ a THIRD painting site added → row (6) goes red
//  ⛔ Each of those is a defect that ⛔ COMPILES, ⛔ reviews plausibly and would ship silently.
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInvisibilityVeilMaterialSwapTest,
	"Siegebound.Invisibility.TheVeilMaterialIsSwappedOnBothEdgesAndOnEverySlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInvisibilityVeilMaterialSwapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeInvisibilityTestFixture;

	FString UnitCpp;
	if (!LoadProjectSource(*this, SummonedUnitCpp, UnitCpp))
	{
		return false;
	}

	// ══ (1) ⛔ THE APPLY EDGE — the row TASK-923(7) demands ═══════════════════════════════════
	FString GrantBody;
	if (ExtractFunctionBody(*this, UnitCpp, TEXT("bool ASummonedUnit::GrantInvisibility()"), GrantBody))
	{
		// ⛔ SELF-CHECK FIRST: an empty or truncated extraction would make every count below a
		//    meaningless zero, and a zero is exactly what "the swap was deleted" also looks like.
		TestTrue(
			TEXT("SELF-CHECK: the extracted GrantInvisibility body is substantial. ⛔ Without this, a stale ")
			TEXT("signature would report the swap as ABSENT — the same red as a real defect, for the wrong reason."),
			GrantBody.Len() > 200);

		// (1a) ⭐⭐ THE ROW THAT GOES RED IF THE SWAP IS REMOVED.
		TestEqual(
			TEXT("⭐⭐⭐ (1a) THE VEIL IS PAINTED ON THE GRANT EDGE: GrantInvisibility calls ApplyVeilMaterial ")
			TEXT("EXACTLY ONCE. ⛔ ZERO HERE IS THE STATE TASK-923 EXISTED TO END — six green tasks, a material ")
			TEXT("measured at 13x the background noise floor, and NOTHING applying it, so the witch did not look ")
			TEXT("invisible in play. ⚖️ An edge is nobody's deliverable, which is exactly why it went unwired."),
			CountOccurrencesInCode(GrantBody, TEXT("ApplyVeilMaterial();")), 1);

		// (1b) ⛔ AND IT IS BEHIND THE EDGE, not run unconditionally. ApplyVeil returns the
		//      false->true transition; re-painting an already-veiled unit would make WITCH-§4's
		//      "never target an already-invisible unit" belt cost a full material re-stamp.
		TestEqual(
			TEXT("⛔ (1b) …and it is GUARDED by the edge ApplyVeil returned, never run unconditionally. The one ")
			TEXT("branch in this function is that guard."),
			CountOccurrencesInCode(GrantBody, TEXT("if (bNewlyVeiled)")), 1);
	}

	// ══ (2) ⛔⛔ THE RESTORE EDGE — HALF THE TASK, ⛔ NOT A POSTSCRIPT ═════════════════════════
	FString BreakBody;
	if (ExtractFunctionBody(*this, UnitCpp, TEXT("void ASummonedUnit::BreakInvisibility(ESiegeVeilBreakReason Reason)"), BreakBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted BreakInvisibility body is substantial."),
			BreakBody.Len() > 200);

		// (2a) ⚖️ A VEIL THAT NEVER REVERTS IS WORSE THAN ONE THAT NEVER APPLIES. His rule is that
		//      it reverts PERMANENTLY on any act other than walking (WITCH-§3), so a missing
		//      restore makes ALL SIX break reasons silently dead: each would clear the flag and
		//      change nothing the player can see.
		TestEqual(
			TEXT("⭐⭐⭐ (2a) THE VEIL IS TAKEN OFF ON THE BREAK EDGE: BreakInvisibility calls ClearVeilMaterial ")
			TEXT("EXACTLY ONCE. ⛔ A zero here ships a unit that goes invisible and NEVER comes back — which is ")
			TEXT("worse than no feature, because it silently kills all six of the break reasons above it."),
			CountOccurrencesInCode(BreakBody, TEXT("ClearVeilMaterial();")), 1);

		// ⛔ AND IT IS BELOW THE EARLY-OUT. ApplyBreak returns true only on the true->false edge;
		//    hoisted above it, every ordinary attack by an UNVEILED unit would re-stamp its own
		//    materials on every swing. ⛔ Ordering probe on CODE LINES ONLY — the prose around
		//    both lines names the other, and a comment-blind probe would be fooled by it.
		// ⛔ NEEDLE DISCIPLINE (SC-§39): the early-out needle ends at the SEMICOLON and never
		//    includes trailing whitespace. `CodeLinesOnly` drops whole comment LINES but does NOT
		//    strip a trailing `//` from a code line, so a `"return; "` needle would be matching the
		//    presence of SummonedUnit.cpp's end-of-line comment rather than the return itself —
		//    deleting that unrelated comment would turn this row red with nothing in the behaviour
		//    to explain it. Every other ordering needle in this file is terminator-anchored; so is
		//    this one. ⚖️ `return;` cannot match a value-returning statement (`return true;` has a
		//    token between), so shortening it costs no discrimination.
		const FString BreakCode = CodeLinesOnly(BreakBody);
		const int32 EarlyOutAt = BreakCode.Find(TEXT("return;"), ESearchCase::CaseSensitive);
		const int32 ClearAt = BreakCode.Find(TEXT("ClearVeilMaterial();"), ESearchCase::CaseSensitive);
		TestTrue(
			TEXT("⛔ (2a-ii) …and the restore sits BELOW the no-edge early-out, so it runs EXACTLY ONCE PER VEIL ")
			TEXT("rather than on every attack of every unveiled unit in the fleet. ⛔ This is an ORDER claim on ")
			TEXT("code lines only."),
			EarlyOutAt != INDEX_NONE && ClearAt != INDEX_NONE && EarlyOutAt < ClearAt);
	}

	FString ClearBody;
	if (ExtractFunctionBody(*this, UnitCpp, TEXT("void ASummonedUnit::ClearVeilMaterial()"), ClearBody))
	{
		// (2b) ⛔⛔ SLOT 0 GOES BACK THROUGH THE SHIPPED TEAM RECOLOR. WITCH-§6 states this as law:
		//      a blanket restore-to-default DROPS the team colour, because MI_TeamColor_<Team> is a
		//      RUNTIME override (TASK-044 — the bot reuses the player's Blue-authored BP_Unit_*).
		//      ⚖️ A RED bot unit would come back BLUE: an enemy standing in your half wearing your
		//      colour, which reads as a far worse bug than the one the restore was fixing.
		TestEqual(
			TEXT("⭐⭐ (2b) THE TEAM COLOUR IS RE-DERIVED, NOT DROPPED: ClearVeilMaterial calls the SHIPPED ")
			TEXT("ApplyTeamMaterial() exactly once, which recomputes MI_TeamColor_<Team> from the CURRENT Team ")
			TEXT("and re-writes slot 0. ⛔ Zero here and a veiled RED unit comes back BLUE."),
			CountOccurrencesInCode(ClearBody, TEXT("ApplyTeamMaterial();")), 1);

		// (2c) ⛔ …and it CALLS that function rather than re-implementing the resolve. A second
		//      copy of the team-material paths is a second thing to keep in sync, and the two
		//      would disagree the first time either moved.
		TestEqual(
			TEXT("⛔ (2c) …and it does NOT re-implement the resolve: the team instance names appear ZERO times ")
			TEXT("inside ClearVeilMaterial. ⛔ WITCH-§6 says CALL the shipped ApplyTeamMaterial, never mirror it."),
			CountOccurrencesInCode(ClearBody, TEXT("MI_TeamColor")), 0);

		// (3c) ⛔ the clear loop is count-driven and index-free, exactly like the apply loop.
		TestEqual(
			TEXT("⛔ (3c) THE CLEAR IS THE APPLY'S MIRROR IMAGE: it iterates GetNumMaterials() and writes ")
			TEXT("SetMaterial(SlotIndex, nullptr) — so every slot the apply could have painted is dropped back to ")
			TEXT("the ASSET's authored material. A clear that covered fewer slots than the apply would strand the ")
			TEXT("veil on the ones it missed."),
			CountOccurrencesInCode(ClearBody, TEXT("SetMaterial(SlotIndex, nullptr)")), 1);

		TestEqual(
			TEXT("⛔ (3d) …and the clear hard-codes NO slot index either (neither 0 nor 1)."),
			CountOccurrencesInCode(ClearBody, TEXT("SetMaterial(0,"))
			+ CountOccurrencesInCode(ClearBody, TEXT("SetMaterial(1,")), 0);
	}

	// ══ (3) ⛔⛔⛔ THE SLOT-1 TRAP — the clause that, got wrong, ships a VISIBLY BROKEN feature ══
	//    rather than a missing one. ApplyTeamMaterial writes MI_TeamColor_<Team> to SLOT 0 of the
	//    two-slot [TeamRegion, <CardID>PBR] contract, so a slot-1-only swap leaves slot 0 FULLY
	//    OPAQUE. TASK-833 measured team_region at 1.7-4.1% of a normal unit — but 18% ON THE WITCH,
	//    and it is her HAT BRIM. ⚖️ The unit whose entire card is invisibility is the unit the
	//    naive implementation breaks WORST.
	FString ApplyBody;
	if (ExtractFunctionBody(*this, UnitCpp, TEXT("void ASummonedUnit::ApplyVeilMaterial()"), ApplyBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted ApplyVeilMaterial body is substantial."),
			ApplyBody.Len() > 200);

		TestEqual(
			TEXT("⭐⭐⭐ (3a) EVERY SLOT: the apply loop is driven by GetNumMaterials() on the ACTIVE visual mesh. ")
			TEXT("⛔ The count is never assumed to be 2 — a re-baked mesh with three slots must still veil whole."),
			CountOccurrencesInCode(ApplyBody, TEXT("GetNumMaterials()")), 1);

		TestEqual(
			TEXT("⭐⭐⭐ (3b) ⛔ NEVER SLOT 1, AND NEVER SLOT 0 EITHER: NO hard-coded slot index appears anywhere in ")
			TEXT("the apply. ⛔ A `SetMaterial(1, …)` here is the single highest-consequence defect in this feature ")
			TEXT("— it leaves an OPAQUE CHROME HAT floating over a ghostly witch, and it would read to a player as ")
			TEXT("'invisibility is broken' rather than as a slot bug."),
			CountOccurrencesInCode(ApplyBody, TEXT("SetMaterial(0,"))
			+ CountOccurrencesInCode(ApplyBody, TEXT("SetMaterial(1,")), 0);

		TestEqual(
			TEXT("⛔ (3b-ii) …the ONE write in the apply is the index-free loop body."),
			CountOccurrencesInCode(ApplyBody, TEXT("SetMaterial(SlotIndex, ResolvedVeil)")), 1);

		// (4) ⛔⛔ THE RULED FAILURE DIRECTION: a material that will not resolve leaves the unit
		//     VISIBLE. ⚖️ A veil that fails to a visible unit is a missing effect; one that fails to
		//     an INVISIBLE-BUT-SOLID unit is an unkillable ghost that still blocks placement and
		//     still deals damage — WITCH-§0 refuses exactly that.
		TestEqual(
			TEXT("⭐⭐ (4) ⛔ THE MESH IS NEVER HIDDEN. No SetVisibility / SetHiddenInGame anywhere in the apply: ")
			TEXT("the ONLY mechanism is a material swap. ⛔ Hiding the mesh as a 'fallback' when the material is ")
			TEXT("missing would ship an UNKILLABLE INVISIBLE SOLID that still blocks placement and still deals ")
			TEXT("damage. ⚖️ Failing to a VISIBLE unit is the ruled direction."),
			CountOccurrencesInCode(ApplyBody, TEXT("SetVisibility"))
			+ CountOccurrencesInCode(ApplyBody, TEXT("SetHiddenInGame")), 0);
	}

	// ══ (5) ⛔⛔ NO CACHE — banned for TWO independent reasons, and the second is the subtle one ══
	//    (a) it is the same species as the "was visible" cache WITCH-§6 bans by name; (b)
	//    GetActiveVisualMesh() can return a DIFFERENT COMPONENT than it did at veil time, because
	//    ResolveSkeletalVisual latches bUsingSkeletalVisual — so a cached pointer, or a material
	//    list keyed to the static component, would be restored onto the WRONG mesh.
	FString CacheWhere;
	const int32 CachedMaterialArrays = CountAcrossShippingSource(*this, TEXT("TArray<UMaterialInterface*>"), CacheWhere);
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ (5) ⛔ NO CACHED 'ORIGINALS' ARRAY ANYWHERE IN SHIPPING SOURCE. The restore RE-DERIVES ")
			TEXT("through ApplyTeamMaterial on the CURRENTLY-active mesh. ⛔ A remembered TArray<UMaterialInterface*> ")
			TEXT("would be restored onto the wrong component the moment the skeletal swap had taken. Sites:%s"),
			CacheWhere.IsEmpty() ? TEXT(" (none)") : *CacheWhere),
		CachedMaterialArrays, 0);

	// ══ (6) ⛔⛔ NO THIRD SITE — the property that makes a grep for the two doors COMPLETE ══════
	//    TASK-923(1): "a grep for these two symbols must remain the COMPLETE list of ways a unit's
	//    look can change for the veil." These two rows are what make that checkable rather than
	//    merely promised. ⛔ 3 = one declaration + one definition + one call, each.
	const struct { const TCHAR* Symbol; const TCHAR* Door; } Painters[] =
	{
		{ TEXT("ApplyVeilMaterial"), TEXT("GrantInvisibility") },
		{ TEXT("ClearVeilMaterial"), TEXT("BreakInvisibility") }
	};

	for (const auto& Painter : Painters)
	{
		FString Where;
		const int32 Mentions = CountAcrossShippingSource(*this, Painter.Symbol, Where);
		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ (6) `%s` appears EXACTLY THREE TIMES in shipping source — its declaration, its ")
				TEXT("definition, and the ONE call inside %s. ⛔ A FOURTH is a second place a unit's look can ")
				TEXT("change for the veil, which breaks the completeness property the whole design rests on ")
				TEXT("(⛔ no tick, ⛔ no timer, ⛔ no BeginPlay branch, ⛔ no second bool). ⛔ Do NOT fix a red here ")
				TEXT("by bumping the number: route the new caller through the door. Sites:%s"),
				Painter.Symbol, Painter.Door, *Where),
			Mentions, 3);
	}

	// ══ (7) ⛔ THE PINNED ASSET PATH — WITCH-§6's name, character-for-character, resolved ONCE ══
	//    ⚠️ This row on its own would be exactly the "asserts the material path string and calls it
	//    coverage" that TASK-923(7) forbids. It is here as the SIXTH row of seven, not the first,
	//    and it carries a claim the string alone does not: that there is only ONE of it.
	FString PathWhere;
	const int32 VeilPathMentions = CountAcrossShippingSource(
		*this, TEXT("/Game/Materials/MI_Unit_Invisible.MI_Unit_Invisible"), PathWhere);
	TestEqual(
		FString::Printf(
			TEXT("⛔ (7) The veil material is referenced by WITCH-§6's pinned soft path EXACTLY ONCE tree-wide ")
			TEXT("(a function-local static, resolved once per process and shared by every veiled unit — the ")
			TEXT("ApplyTeamMaterial idiom). ⛔ A second mention is a second copy of the path to keep in sync. ")
			TEXT("⚠️ THIS ROW PROVES A REFERENCE, ⛔ NEVER A PIXEL. Sites:%s"),
			PathWhere.IsEmpty() ? TEXT(" (none)") : *PathWhere),
		VeilPathMentions, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
