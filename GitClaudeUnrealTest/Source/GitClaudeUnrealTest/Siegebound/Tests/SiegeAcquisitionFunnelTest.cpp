// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/SiegeCombatStatics.h"
#include "Siegebound/TeamId.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE ACQUISITION FUNNEL (TASK-828; law `WITCH-§1`, with
 *      `WITCH-§0` for why it had to exist and `WITCH-§2` for the friendly lane) ═══
 *
 *  ⭐⭐ WHAT THIS FILE IS DEFENDING, IN ONE PARAGRAPH. Invisibility cannot copy the
 *  ghost's mechanism: `ASiegeGhostPawn` is untargetable because it does not implement
 *  `ITeamAgent`, which is compile-time, class-level and permanent, while a veil is
 *  per-instance, runtime, reversible and must PRESERVE attackability (Jonathan's own
 *  interruption rule needs the veiled unit to still be hittable). A veil is therefore a
 *  FLAG AT A GUARD POINT — precisely the design `GHOST-§1` refused in writing, because
 *  "a flag can be forgotten at a guard point". ⇒ the only honest mitigation is to make
 *  sure there is exactly ONE guard point, and these tests are what keeps it at one.
 *
 *  ⚠️⚠️ THE LESSON THESE ASSERTIONS ARE WRITTEN AGAINST: **A REFACTOR'S TEST PASSES
 *  TRIVIALLY IF IT ONLY ASSERTS THE NEW FUNCTION RETURNS SOMETHING.** Every claim below
 *  therefore ships with a CONTROL that would go red if the thing it cares about were
 *  deleted — the team term is asserted through a value that must be FALSE, the "old
 *  filter is gone" counts are paired with a POSITIVE count in the same file proving the
 *  scanner still sees real code, and the grep gate is paired with a comment-only mention
 *  that must count ZERO, proving the comment-skip is live rather than the file empty.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero asset loads, ⛔ zero
 *  writes. Two lanes only:
 *    (a) DIRECT CALLS into `FSiegeCombatStatics`' pure entry points (`IsHostileTeam`, and
 *        `GatherHostileAgents` with a null world) — real execution, no world needed;
 *    (b) SOURCE-TEXT structural probes with comment lines skipped, the house
 *        `CountOccurrencesInCode` idiom (`SiegeClimbableTowerTest.cpp:490`).
 *
 *  ⚠️ WHY (b) AND NOT A SPAWNED FIXTURE, STATED SO THE GAP IS HONEST: there is not one
 *  `UWorld::CreateWorld` and not one `SpawnActor` anywhere in `Siegebound/Tests/`
 *  (`SiegeLadderClimbTest.cpp:39` states the house rule). The set-equality claim at nine
 *  call sites is therefore proved by (1) showing the lifted predicate is EXACTLY the one
 *  that used to be inline, term for term, (2) showing every site's OWN filtering is still
 *  present and unlifted, and (3) showing no site retains a second, divergent copy of the
 *  team filter. ⛔ None of that is a substitute for a PIE combat pass, and nothing here
 *  should be read as covering ordering effects the compiler could introduce.
 *
 *  ⚠️⚠️ DECLARED LIMITATION of the scanner — ⛔ CORRECTED BY TASK-868, because the version
 *  inherited here was ⛔ TRUE ABOUT THE INSTRUMENT AND ⛔ FALSE ABOUT THIS FILE. It read:
 *  "a comment TRAILING a line of code is still scanned. Every probe here is a whole-line
 *  construct or a statement, ⛔ so none is exposed to it." ⛔ The second sentence did not
 *  survive contact: test 9 counted a bare token in `SummonedUnit.cpp`, whose `#include` of
 *  the invisibility header carries a ⛔ TRAILING comment NAMING that token — so the probe
 *  read ⛔ ONE MORE than the truth for two separate tokens. ⭐ The exposure was never about
 *  how the PROBE is written; it is about how the ⛔ SUBJECT is written, and the subject is
 *  somebody else's file.
 *  ✅ THE STANDING RULE FOR THIS FILE (`SC-§39`): ⛔ pin to a CALL SHAPE — a needle carrying
 *  an open paren or an assignment operator — ⛔ never a bare token, and where a number is
 *  pinned, ⛔ ASSERT its immunity by reading it with the skip rule both on and off. Test 9
 *  does exactly that and carries a synthetic probe exercising ⛔ BOTH blind directions.
 */

namespace SiegeAcquisitionFunnelFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files the grep gate is about. */
	const TCHAR* CombatStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp");
	const TCHAR* CombatStaticsHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h");
	const TCHAR* SummonedUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp");
	const TCHAR* TowerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/Tower.cpp");
	const TCHAR* HeroCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp");
	const TCHAR* SpellLibraryCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp");
	const TCHAR* SpellLineSweepCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLineSweep.cpp");
	const TCHAR* CheatManagerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp");

	/** A file that mentions the forbidden token ONLY in a comment — the comment-skip control. */
	const TCHAR* GhostPawnHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeGhostPawn.h");

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
	 *  Occurrences of Needle on CODE lines only (the `SiegeClimbableTowerTest` /
	 *  `SiegePlacementTest` helper, copied verbatim so the three agree).
	 *
	 *  ⚠️⚠️ LOAD-BEARING HERE ABOVE ALL OTHER FILES. `SiegeCombatStatics.h` NAMES
	 *  `GetAllActorsWithInterface` four times in the paragraphs that explain why it may
	 *  appear only once; `SiegeGhostPawn.h`, `SiegeGameMode.h`, `Projectile.h`,
	 *  `CommanderNpc.h`, `SiegeStuckStatics.cpp` and `SummonedUnit.cpp` all name it in
	 *  prose too. A scanner that counted comments would force every one of those files to
	 *  choose between explaining the law and passing it — and the explanation would lose.
	 *  ⚖️ The prose is the guard. The instrument got smarter instead.
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

			// ⚠️ `*` is qualified rather than bare on purpose: a doc-comment continuation is
			// `* text` or `*/`, while `*GetNameSafe(Foo)` starts a CODE line with the same
			// character. Skipping those would be a silent blind spot.
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
	 *  ⛔ THE SAME SCAN WITH THE COMMENT-SKIP ⛔ DISABLED. ⛔ NOT a better scanner — a ⛔ CONTROL.
	 *
	 *  `SC-§39` records that `CountOccurrencesInCode` is blind in ⛔ BOTH directions — a leading
	 *  BLOCK-COMMENT OPENER ⛔ HIDES a real hit, and a ⛔ TRAILING line-comment marker on a code
	 *  line ⛔ MANUFACTURES a false one — and that ⛔ a single positive control only ever proves
	 *  ⛔ ONE of them. (⛔ Both markers are spelled out in prose rather than as digraphs on
	 *  purpose: a literal opener inside a block comment is a compiler warning.) Reading a needle
	 *  ⛔ twice, once each way, turns both directions into ⛔ EXECUTED assertions instead of a
	 *  sentence in a handoff: ⭐ where the two readings ⛔ AGREE the number is
	 *  ⛔ instrument-independent, and where they ⛔ DIFFER the difference ⛔ IS the blind spot.
	 *
	 *  ⛔ TEST 9 ONLY. ⛔ Nothing else in this file may take a COUNT with it — an unfiltered count
	 *  is precisely what the house helper exists to avoid (see its own docstring above).
	 */
	static int32 CountOccurrencesIncludingComments(const FString& Source, const TCHAR* Needle)
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
	 *  Extracts one function body by signature, ending at the first column-0 closing brace
	 *  (`\n}`) — how every free/member function in this codebase ends. ⛔ Deliberately NOT a
	 *  parser: a signature that stops matching FAILS rather than silently scanning nothing.
	 *  ⚠️ Unusable on the file-local resolvers inside `SpellLibrary.cpp`'s anonymous
	 *  namespace (their closing brace is indented), which is why those are asserted at
	 *  whole-file scope instead — stated so the asymmetry is not read as an oversight.
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
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE GREP GATE. THE BINDING ACCEPTANCE CRITERION OF TASK-828.
//  `WITCH-§1`: a SECOND `GetAllActorsWithInterface` in `Source/` is an automatic
//  QA BLOCKER, because the tenth site written next month would silently see
//  through the veil. This is that grep, shipped as a gate rather than a report.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelGrepGateTest,
	"Siegebound.Acquisition.GetAllActorsWithInterfaceAppearsExactlyOnceInSource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelGrepGateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	TArray<FString> SourceFiles;
	FindAllSourceFiles(SourceFiles);

	// ── SELF-CHECK: a scan that found no files would pass every count below vacuously. ──
	if (SourceFiles.Num() < 20)
	{
		AddError(FString::Printf(
			TEXT("SELF-CHECK FAILED: the recursive scan of Source/ found only %d file(s). The instrument is ")
			TEXT("dead and every count below would be a meaningless zero."), SourceFiles.Num()));
		return false;
	}

	// ⭐⭐ THE NEEDLES ARE COMPOSED AT RUNTIME, AND THAT IS ⛔ NOT CLEVERNESS — IT IS THE ONLY
	// WAY THIS GATE CAN INCLUDE ITS OWN FILE IN THE SCAN. A gate that spelled the forbidden
	// token as a literal would count ITSELF and fail, so it would have to exempt its own file
	// — and an instrument that cannot see the file it lives in is exactly the blind spot a
	// tenth enumeration would be written into. Split here, whole everywhere else.
	const FString BareNeedle = FString(TEXT("GetAllActors")) + TEXT("WithInterface(");
	const FString QualifiedNeedle = FString(TEXT("UGameplayStatics::")) + BareNeedle;
	const FString BareNameOnly = FString(TEXT("GetAllActors")) + TEXT("WithInterface");

	// ── SELF-CHECK on the composition itself: a typo'd needle would find nothing and every
	// "exactly once" below would read as "exactly zero", i.e. the gate would pass by being blind.
	TestEqual(TEXT("SELF-CHECK: the composed bare needle is the real token, correctly spelled."),
		BareNeedle, FString(TEXT("GetAllActorsWithInterfac")) + TEXT("e("));

	// ── LANE A: the QUALIFIED call. Strict form, and it covers the WHOLE tree — automation ──
	// tests included — because the class prefix plus the open paren can only be a real call.
	int32 QualifiedTotal = 0;
	FString QualifiedWhere;
	// ── LANE B: the BARE token, SHIPPING source only. Wider net (it would catch a call ──
	// written through a `using` or an alias) at the price of one named exception below.
	int32 BareShippingTotal = 0;
	FString BareShippingWhere;
	int32 BareTestLaneTotal = 0;
	FString BareTestLaneWhere;

	for (const FString& File : SourceFiles)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *File))
		{
			AddError(FString::Printf(TEXT("⛔ Could not read '%s' — an unreadable file FAILS the gate rather than being skipped."), *File));
			return false;
		}

		const int32 Qualified = CountOccurrencesInCode(Text, *QualifiedNeedle);
		if (Qualified > 0)
		{
			QualifiedTotal += Qualified;
			QualifiedWhere += FString::Printf(TEXT("\n    %d x  %s"), Qualified, *FPaths::GetCleanFilename(File));
		}

		const int32 Bare = CountOccurrencesInCode(Text, *BareNeedle);
		if (IsAutomationTestFile(File))
		{
			BareTestLaneTotal += Bare;
			if (Bare > 0)
			{
				BareTestLaneWhere += FString::Printf(TEXT("\n    %d x  %s"), Bare, *FPaths::GetCleanFilename(File));
			}
		}
		else if (Bare > 0)
		{
			BareShippingTotal += Bare;
			BareShippingWhere += FString::Printf(TEXT("\n    %d x  %s"), Bare, *FPaths::GetCleanFilename(File));
		}
	}

	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ THE GATE (WITCH-§1): the qualified UGameplayStatics interface-enumeration call appears ")
			TEXT("EXACTLY ONCE in the whole of Source/, and that once is ")
			TEXT("FSiegeCombatStatics::GatherTeamAgentsFiltered. A second hit is an AUTOMATIC QA BLOCKER: it is ")
			TEXT("a hostile-actor enumeration that bypasses the funnel, which means it will NOT honour the ")
			TEXT("invisibility veil (TASK-829) and will NOT honour the fog range clamp (TASK-838) — and nobody ")
			TEXT("will notice until a witch walks past it. ⛔ Do NOT fix a red here by adding a second veil ")
			TEXT("check at the new site: route the new site through FSiegeCombatStatics::GatherHostileAgents ")
			TEXT("or ::GatherFriendlyAgents. Found:%s"),
			QualifiedWhere.IsEmpty() ? TEXT(" (nowhere — the needle is broken, see the SELF-CHECK above)") : *QualifiedWhere),
		QualifiedTotal, 1);

	TestEqual(
		FString::Printf(
			TEXT("⭐ THE WIDER NET: the bare interface-enumeration token appears exactly once on a code line in ")
			TEXT("SHIPPING source (Source/, excluding Tests/). This catches a call written without the ")
			TEXT("UGameplayStatics:: prefix, which the strict lane above would miss. Found:%s"),
			BareShippingWhere.IsEmpty() ? TEXT(" (nowhere)") : *BareShippingWhere),
		BareShippingTotal, 1);

	// ── THE ONE EXCEPTION, EXEMPTED BY NAME RATHER THAN LEFT SILENT (WITCH-§1's own rule). ──
	// `SiegeGhostPawnTest.cpp` quotes the token inside a TEXT("…") FAILURE MESSAGE — prose that
	// happens to sit on a code line, not a call. Asserting its exact count means the exemption
	// cannot quietly grow into a second real enumeration in the automation lane.
	TestEqual(
		FString::Printf(
			TEXT("⛔ NAMED EXEMPTION, and it is exactly one: Siegebound/Tests/ contains ONE bare hit — the string ")
			TEXT("literal inside SiegeGhostPawnTest's failure message, which is prose sitting on a code line, not ")
			TEXT("a call. ⭐ THIS FILE contributes ZERO by construction (its needles are composed at runtime), ")
			TEXT("which is what lets the gate scan itself. If this becomes 2, someone added a real enumeration to ")
			TEXT("the automation lane — name it here or route it through the funnel. Found:%s"),
			BareTestLaneWhere.IsEmpty() ? TEXT(" (nowhere)") : *BareTestLaneWhere),
		BareTestLaneTotal, 1);

	// ── THE COMMENT-SKIP CONTROL. Without this the gate would pass just as happily if the ──
	// scanner were broken and returning 0 for everything. `SiegeGhostPawn.h` names the token
	// in a doc comment and NOWHERE else; it MUST count zero, and the file MUST contain it.
	FString GhostHeader;
	if (LoadProjectSource(*this, GhostPawnHeader, GhostHeader))
	{
		TestTrue(
			TEXT("SELF-CHECK: SiegeGhostPawn.h really does name the interface enumeration in its prose — if it ")
			TEXT("did not, the comment-skip control below would be proving nothing at all."),
			GhostHeader.Contains(*BareNameOnly, ESearchCase::CaseSensitive));

		TestEqual(
			TEXT("⭐ COMMENT-SKIP CONTROL: that prose mention counts ZERO on code lines. This is what lets ")
			TEXT("SiegeCombatStatics.h explain the law it enforces without breaking it — and if this ever ")
			TEXT("returns non-zero, the scanner has stopped skipping comments and every count in this file, ")
			TEXT("and in four other test files that share the helper, is silently wrong."),
			CountOccurrencesInCode(GhostHeader, *BareNameOnly), 0);
	}

	// ── The second, independent needle: the INTERFACE ARGUMENT. A call could in principle ──
	// be reshaped to dodge the function-name needle; it cannot dodge naming the interface.
	FString CombatStaticsSource;
	if (LoadProjectSource(*this, CombatStaticsCpp, CombatStaticsSource))
	{
		TestEqual(
			TEXT("⭐ `UTeamAgent::StaticClass()` appears exactly once on a code line in SiegeCombatStatics.cpp — ")
			TEXT("the funnel's own enumeration. Paired with the shipping-source total below, this is a second ")
			TEXT("independent lane of the same guarantee."),
			CountOccurrencesInCode(CombatStaticsSource, TEXT("UTeamAgent::StaticClass()")), 1);
	}

	int32 InterfaceArgShippingTotal = 0;
	for (const FString& File : SourceFiles)
	{
		if (IsAutomationTestFile(File))
		{
			continue;
		}
		FString Text;
		if (FFileHelper::LoadFileToString(Text, *File))
		{
			InterfaceArgShippingTotal += CountOccurrencesInCode(Text, TEXT("UTeamAgent::StaticClass()"));
		}
	}
	TestEqual(
		TEXT("⭐ `UTeamAgent::StaticClass()` appears exactly once across ALL shipping source. The ONLY place the ")
		TEXT("project may ask the world for team agents is FSiegeCombatStatics::GatherTeamAgentsFiltered."),
		InterfaceArgShippingTotal, 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐ — THE TEAM TERM, EXECUTED AND EXHAUSTIVE. The control that goes red
//  the moment the friendly-fire filter is dropped.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelHostilePredicateTest,
	"Siegebound.Acquisition.IsHostileTeamIsExhaustiveAndDiscriminating",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelHostilePredicateTest::RunTest(const FString& Parameters)
{
	// ⛔ ALL FOUR COMBINATIONS. `ETeamId` has exactly two values (Blue, Red), so this is the
	// complete truth table of the term every one of the nine sites used to write inline.
	//
	// ⭐⭐ THESE TWO FALSE ROWS ARE THE WHOLE POINT OF THE TEST. A refactor test that only
	// asserted "the gatherer returns something" would pass with the team filter DELETED —
	// and a deleted team filter means EVERY unit, tower, spell, melee swing and AoE blast in
	// the game attacks its own side. The false rows cannot pass if that happens.
	TestFalse(
		TEXT("⭐ CONTROL: Blue is NOT hostile to Blue. If this goes red the friendly-fire filter is gone and ")
		TEXT("every acquisition in the game now returns allies (GDD §3.0)."),
		FSiegeCombatStatics::IsHostileTeam(ETeamId::Blue, ETeamId::Blue));

	TestFalse(
		TEXT("⭐ CONTROL: Red is NOT hostile to Red — the same guarantee from the bot's side."),
		FSiegeCombatStatics::IsHostileTeam(ETeamId::Red, ETeamId::Red));

	TestTrue(
		TEXT("Blue viewer, Red candidate: hostile. Without this row the two FALSE rows above could be ")
		TEXT("satisfied by a predicate that always returns false — i.e. an acquisition that finds nothing."),
		FSiegeCombatStatics::IsHostileTeam(ETeamId::Blue, ETeamId::Red));

	TestTrue(
		TEXT("Red viewer, Blue candidate: hostile. The relation is symmetric, which is why one funnel serves ")
		TEXT("both the player's fleet and the bot's."),
		FSiegeCombatStatics::IsHostileTeam(ETeamId::Red, ETeamId::Blue));

	// ── The two lanes are strict complements: hostile and friendly partition the roster, so
	// no ITeamAgent can be dropped by both gathers (which would make it unattackable AND
	// unorderable — an invisible-by-accident actor, the exact bug WITCH-§0 refuses).
	const ETeamId AllTeams[] = { ETeamId::Blue, ETeamId::Red };
	for (const ETeamId Viewer : AllTeams)
	{
		for (const ETeamId Candidate : AllTeams)
		{
			const bool bHostile = FSiegeCombatStatics::IsHostileTeam(Viewer, Candidate);
			const bool bSameTeam = (Viewer == Candidate);
			TestEqual(
				FString::Printf(
					TEXT("PARTITION: viewer %d / candidate %d is in EXACTLY ONE of the two lanes. An actor that ")
					TEXT("fell out of both gathers would be untargetable AND unorderable."),
					static_cast<int32>(Viewer), static_cast<int32>(Candidate)),
				bHostile, !bSameTeam);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 — THE GATHER EXECUTES: Out is RESET and a null world is safe.
//  A real call, not a text scan.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelGatherContractTest,
	"Siegebound.Acquisition.GatherResetsOutAndIsNullWorldSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelGatherContractTest::RunTest(const FString& Parameters)
{
	// Pre-seeded with nulls the gather must clear. ⛔ Nothing is ever dereferenced — only the
	// element COUNT is read, so no fabricated actor pointer is needed or wanted.
	TArray<AActor*> Out;
	Out.SetNumZeroed(3);

	// ── SELF-CHECK: prove the array really was non-empty before the call, or "it is empty
	// afterwards" would be a claim about nothing.
	TestEqual(TEXT("SELF-CHECK: the array holds 3 stale entries before the gather runs."), Out.Num(), 3);

	FSiegeCombatStatics::GatherHostileAgents(/*World=*/ nullptr, ETeamId::Blue, Out);
	TestEqual(
		TEXT("⭐ GatherHostileAgents RESETS Out before filling it, and a null world yields an empty result ")
		TEXT("rather than a crash or a stale carry-over. Callers pass a fresh local, but a gather that ")
		TEXT("APPENDED would silently double every candidate the day someone reuses a buffer across ticks."),
		Out.Num(), 0);

	Out.SetNumZeroed(3);
	FSiegeCombatStatics::GatherFriendlyAgents(/*World=*/ nullptr, ETeamId::Red, Out);
	TestEqual(
		TEXT("⭐ GatherFriendlyAgents honours the same Out contract — the two lanes are not allowed to differ ")
		TEXT("in their bookkeeping, only in their team term."),
		Out.Num(), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — ALL NINE ENUMERATION SITES ROUTE THROUGH THE FUNNEL, AND THE
//  SITE COUNT ITSELF IS ASSERTED. This is the test that catches a tenth site.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelAllSitesRoutedTest,
	"Siegebound.Acquisition.AllNineEnumerationSitesRouteThroughTheFunnel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelAllSitesRoutedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	struct FSiteExpectation
	{
		const TCHAR* RelativePath;
		const TCHAR* Label;
		/**
		 *  ⚠️ Per-row needle. Every site outside FSiegeCombatStatics must NAME the class, so
		 *  `FSiegeCombatStatics::GatherHostileAgents(` counts calls exactly. ApplyRadialDamage
		 *  lives INSIDE the class and calls it unqualified — and the file also contains the
		 *  function's own DEFINITION, which is not a call. Its row therefore uses the full
		 *  call expression, so a definition can never be mistaken for a call site.
		 */
		const TCHAR* HostileNeedle;
		/** Same definition-is-not-a-call reasoning, mirrored for the friendly lane. */
		const TCHAR* FriendlyNeedle;
		int32 ExpectedHostileCalls;
		int32 ExpectedFriendlyCalls;
	};

	// ⚠️⚠️ READ THE NUMBERS, THEY CARRY THE AUDIT'S REAL FINDING. There were NINE
	// enumerations but ELEVEN team-filter consumers, because `SpellLibrary.cpp`'s single
	// `GetAllActorsWithInterface` fed THREE filter loops — Freeze, Lightning and Battle Cry
	// — and Battle Cry wants FRIENDLIES. Counting call sites by grepping the enumeration
	// under-counts the work by two and hides a lane that must NOT be veil-suppressed.
	const TCHAR* const QualifiedHostileNeedle = TEXT("FSiegeCombatStatics::GatherHostileAgents(");
	const TCHAR* const QualifiedFriendlyNeedle = TEXT("FSiegeCombatStatics::GatherFriendlyAgents(");

	// ⚠️ The two in-class needles below are `Gather*Agents(World,` on purpose: the DEFINITIONS
	// in this same file read `Gather*Agents(const UWorld* World,`, so a definition can never be
	// counted as a call site. That distinction is not pedantry — without it this row reports a
	// friendly gather inside ApplyRadialDamage that does not exist.
	// ⚠️ AMENDED BY TASK-829: the in-class call now names its veil policy
	// (`ESiegeVeilPolicy::IncludeVeiled` — WITCH-§2 / J-W2, a blast is not an act of seeing), so the
	// needle ends at the comma rather than the closing paren. It still cannot match the DEFINITION,
	// which reads `GatherHostileAgents(const UWorld* World, …`, which is the whole point of this form.
	const TCHAR* const InClassHostileNeedle = TEXT("GatherHostileAgents(World, Team, HostileAgents, ");
	const TCHAR* const InClassFriendlyNeedle = TEXT("GatherFriendlyAgents(World,");

	const FSiteExpectation Sites[] =
	{
		// sites 1-2: ASummonedUnit::AcquireTarget + ::AcquireEnemyNearPoint
		{ SummonedUnitCpp,    TEXT("ASummonedUnit (AcquireTarget + AcquireEnemyNearPoint)"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 2, 0 },
		// sites 3-4: ATower::AcquireTarget + ::FireChainZapAt (the chain's fire-time snapshot)
		{ TowerCpp,           TEXT("ATower (AcquireTarget + FireChainZapAt)"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 2, 0 },
		// site 5: FSiegeCombatStatics::ApplyRadialDamage — EVERY AoE in the game
		{ CombatStaticsCpp,   TEXT("FSiegeCombatStatics::ApplyRadialDamage (every AoE)"),
		  InClassHostileNeedle, InClassFriendlyNeedle, 1, 0 },
		// site 6: USpellLibrary — ONE enumeration, THREE consumers (2 hostile + 1 friendly)
		{ SpellLibraryCpp,    TEXT("USpellLibrary (Freeze + Lightning hostile, BattleCry friendly)"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 2, 1 },
		// site 7: ASpellLineSweep::ApplyLineEffectUpTo
		{ SpellLineSweepCpp,  TEXT("ASpellLineSweep::ApplyLineEffectUpTo"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 1, 0 },
		// site 8: AHeroCharacter::DoMeleeAttack
		{ HeroCpp,            TEXT("AHeroCharacter::DoMeleeAttack"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 1, 0 },
		// site 9: USiegeCheatManager's FindNearestEnemy — the cheat lane, routed not exempted
		{ CheatManagerCpp,    TEXT("USiegeCheatManager::FindNearestEnemy (cheat lane)"),
		  QualifiedHostileNeedle, QualifiedFriendlyNeedle, 1, 0 },
	};

	int32 TotalHostile = 0;
	int32 TotalFriendly = 0;

	for (const FSiteExpectation& Site : Sites)
	{
		FString Text;
		if (!LoadProjectSource(*this, Site.RelativePath, Text))
		{
			continue;
		}

		const int32 Hostile = CountOccurrencesInCode(Text, Site.HostileNeedle);
		const int32 Friendly = CountOccurrencesInCode(Text, Site.FriendlyNeedle);

		TestEqual(
			FString::Printf(
				TEXT("⭐ %s routes %d hostile gather(s) through the funnel. A DROP here means a site went back to ")
				TEXT("enumerating for itself and will not honour the veil; an INCREASE means a new acquisition ")
				TEXT("appeared — route it through the funnel and update this row deliberately."),
				Site.Label, Site.ExpectedHostileCalls),
			Hostile, Site.ExpectedHostileCalls);

		TestEqual(
			FString::Printf(TEXT("⭐ %s routes %d friendly gather(s)."), Site.Label, Site.ExpectedFriendlyCalls),
			Friendly, Site.ExpectedFriendlyCalls);

		TotalHostile += Hostile;
		TotalFriendly += Friendly;
	}

	TestEqual(
		TEXT("⭐⭐ TEN hostile consumers, across the nine measured enumeration sites. `WITCH-§0` counted NINE ")
		TEXT("`GetAllActorsWithInterface` calls, which is right — but SpellLibrary's ONE call fed THREE loops, ")
		TEXT("so the number of places that actually apply a team filter is eleven, not nine."),
		TotalHostile, 10);

	TestEqual(
		TEXT("⭐⭐ ONE friendly consumer — Battle Cry (`ResolveAllyBuff`). This is the row that would have been ")
		TEXT("SILENTLY BROKEN by a naive nine-site refactor: routed through the hostile gather, Battle Cry ")
		TEXT("would buff nobody, and the suite would still have been green."),
		TotalFriendly, 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐ — NO SITE KEPT ITS OWN COPY OF THE TEAM FILTER, EACH ZERO PAIRED
//  WITH A PER-FILE POSITIVE CONTROL PROVING THE SCANNER STILL SEES REAL CODE
//  IN THAT SAME FILE. (⚠️ The controls are NOT uniformly `GetTeamId()` — one
//  file now has zero of those, and that is a finding, not a failure. See below.)
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelNoSecondFilterTest,
	"Siegebound.Acquisition.NoSiteRetainsItsOwnAcquisitionTeamFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelNoSecondFilterTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	// The EXACT shape that used to sit at all nine sites, character for character:
	//     const ITeamAgent* Agent = Cast<ITeamAgent>(Candidate);
	//     if (!Agent || Agent->GetTeamId() == Team) { continue; }
	// ⛔ Its survival anywhere would mean a site is filtering twice — and the day TASK-829
	// adds veil suppression to the funnel, a site with its OWN filter would be the one place
	// the veil silently does not apply.
	const TCHAR* const BannedForms[] =
	{
		TEXT("!Agent || Agent->GetTeamId()"),
		TEXT("Cast<ITeamAgent>(Candidate)"),
		TEXT("Cast<ITeamAgent>(Target)"),
	};

	struct FSiteScan
	{
		const TCHAR* RelativePath;
		const TCHAR* Label;
		// `Cast<ITeamAgent>(Candidate)` legitimately survives in ATower::IsAcquirableEnemy —
		// the §3.7 class gate that the CHAIN BOUNCE search also calls on candidates it did
		// not gather. That is site-local filtering, which this task deliberately did NOT lift.
		int32 AllowedCandidateCasts;
		/**
		 *  ⭐ THE POSITIVE CONTROL, PER FILE — a construct that MUST still be present, so the
		 *  zeros asserted beside it are claims about a real read rather than an empty one.
		 *
		 *  ⚠️ THESE ARE ⛔ NOT ALL `GetTeamId()`, AND THE REASON IS A FINDING: after the lift,
		 *  `SpellLineSweep.cpp` contains ⛔ ZERO `GetTeamId()` calls — its ONLY team logic ever
		 *  was the acquisition filter, and that is now entirely the funnel's. A uniform
		 *  `GetTeamId() > 0` control would have gone red on a file that is CORRECT, which is
		 *  its own small lesson about controls chosen by habit rather than by measurement.
		 */
		const TCHAR* LiveControlToken;
	};

	const FSiteScan Scans[] =
	{
		// damage-attribution casts survive everywhere — they read the CAUSER's team, which has
		// nothing to do with acquisition and was never part of this lift.
		{ SummonedUnitCpp,   TEXT("SummonedUnit.cpp"),      0, TEXT("Cast<ITeamAgent>(DamageCauser)") },
		{ TowerCpp,          TEXT("Tower.cpp"),             1, TEXT("IsAcquirableEnemy(") },
		{ HeroCpp,           TEXT("HeroCharacter.cpp"),     0, TEXT("Cast<ITeamAgent>(DamageCauser)") },
		{ SpellLibraryCpp,   TEXT("SpellLibrary.cpp"),      0, TEXT("Cast<ASummonedUnit>(Candidate)") },
		// ⭐ zero GetTeamId() left in this file — see the note above; its control is the
		// caster-team member the sweep still carries and hands to the funnel.
		{ SpellLineSweepCpp, TEXT("SpellLineSweep.cpp"),    0, TEXT("CasterTeam") },
		{ CheatManagerCpp,   TEXT("SiegeCheatManager.cpp"), 0, TEXT("Cast<ITeamAgent>(PC->GetPawn())") },
	};

	for (const FSiteScan& Scan : Scans)
	{
		FString Text;
		if (!LoadProjectSource(*this, Scan.RelativePath, Text))
		{
			continue;
		}

		// ── POSITIVE CONTROL, and it is what makes the zeros below mean something. If the
		// scanner or the file read were broken, this would be zero too and every absence
		// asserted underneath it would be vacuous.
		const int32 LiveControlHits = CountOccurrencesInCode(Text, Scan.LiveControlToken);
		TestTrue(
			FString::Printf(
				TEXT("SELF-CHECK: %s still contains `%s` on a code line (found %d). Without this the absences ")
				TEXT("asserted below would pass on an empty read."),
				Scan.Label, Scan.LiveControlToken, LiveControlHits),
			LiveControlHits > 0);

		TestEqual(
			FString::Printf(
				TEXT("⭐ %s no longer contains the acquisition team-filter form `!Agent || Agent->GetTeamId()`. ")
				TEXT("That predicate now exists exactly once, inside the funnel. A second copy is the ")
				TEXT("forgotten-guard-point failure GHOST-§1 warned about and WITCH-§1 exists to prevent."),
				Scan.Label),
			CountOccurrencesInCode(Text, BannedForms[0]), 0);

		TestEqual(
			FString::Printf(
				TEXT("⭐ %s narrows `Cast<ITeamAgent>(Candidate)` exactly %d time(s) — the count that is ")
				TEXT("site-local class-gating, not acquisition. Anything above it is a re-implemented funnel."),
				Scan.Label, Scan.AllowedCandidateCasts),
			CountOccurrencesInCode(Text, BannedForms[1]), Scan.AllowedCandidateCasts);

		TestEqual(
			FString::Printf(TEXT("⭐ %s contains no `Cast<ITeamAgent>(Target)` acquisition narrowing."), Scan.Label),
			CountOccurrencesInCode(Text, BannedForms[2]), 0);
	}

	// ── And the funnel itself DOES carry the cast, exactly once — the mirror image of every
	// zero above. If this is zero, the team filter was lifted out of the sites and then
	// dropped on the floor, and every acquisition in the game is now friendly-fire capable.
	FString CombatStaticsSource;
	if (LoadProjectSource(*this, CombatStaticsCpp, CombatStaticsSource))
	{
		TestEqual(
			TEXT("⭐⭐ The funnel narrows with `Cast<ITeamAgent>(Candidate)` exactly once — the ONE surviving ")
			TEXT("copy of the predicate the nine sites used to each own."),
			CountOccurrencesInCode(CombatStaticsSource, TEXT("Cast<ITeamAgent>(Candidate)")), 1);

		TestEqual(
			TEXT("⭐ …and it consults the named relation rather than an inlined `!=`, so the term is testable. ")
			TEXT("Two hits: the definition of IsHostileTeam and its one use inside the filter."),
			CountOccurrencesInCode(CombatStaticsSource, TEXT("IsHostileTeam(")), 2);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐⭐ — THE ANTI-OVER-LIFT CONTROL. THIS TASK LIFTED THE ENUMERATE-AND-
//  TEAM-FILTER STEP AND ⛔ NOTHING ELSE. Every site's OWN filtering must still be
//  where it was, or the "byte-identical results" claim is false in the other
//  direction — a funnel that swallowed a range check would make melee global.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelSiteLocalFilteringSurvivedTest,
	"Siegebound.Acquisition.EverySitesOwnFilteringSurvivedTheLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelSiteLocalFilteringSurvivedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	struct FBodyProbe
	{
		const TCHAR* RelativePath;
		const TCHAR* Signature;
		const TCHAR* MustContain;
		const TCHAR* Why;
	};

	// Each row: the site's body must still contain the filter that makes it THAT site rather
	// than a world-wide effect. ⛔ These are the terms a careless "consolidation" would move
	// into the funnel, and every one of them would be a real, shipped behaviour change.
	const FBodyProbe Probes[] =
	{
		{ SummonedUnitCpp, TEXT("AActor* ASummonedUnit::AcquireTarget() const"), TEXT("AggroRadius"),
		  TEXT("aggro radius — without it a unit acquires across the whole 50,000 uu field") },
		{ SummonedUnitCpp, TEXT("AActor* ASummonedUnit::AcquireTarget() const"), TEXT("TieBreakDistance"),
		  TEXT("the GDD §3.8 Standard pawn-over-building tie-break") },
		{ SummonedUnitCpp, TEXT("AActor* ASummonedUnit::AcquireTarget() const"), TEXT("IsTargetAlive(Candidate)"),
		  TEXT("per-type liveness — destroyed castle / hidden dead hero / bDead same-frame window") },
		{ SummonedUnitCpp, TEXT("AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const"), TEXT("DistSquared2D"),
		  TEXT("the 2D position-disc gate that makes this a ZONE order and not a global sweep") },
		{ TowerCpp, TEXT("AActor* ATower::AcquireTarget() const"), TEXT("IsAcquirableEnemy("),
		  TEXT("the §3.7 class gate + liveness — a tower must never shoot a castle or a wall") },
		{ TowerCpp, TEXT("AActor* ATower::AcquireTarget() const"), TEXT("MinRangeSq"),
		  TEXT("the BallistaTower blind spot") },
		{ TowerCpp, TEXT("void ATower::FireChainZapAt(AActor* PrimaryTarget)"), TEXT("ChainBounceRadius"),
		  TEXT("bounce radius measured from the PREVIOUS target, not the tower") },
		{ HeroCpp, TEXT("void AHeroCharacter::DoMeleeAttack()"), TEXT("MeleeRange"),
		  TEXT("melee reach — without it every swing is a world-wide smite") },
		{ HeroCpp, TEXT("void AHeroCharacter::DoMeleeAttack()"), TEXT("MinCosAngle"),
		  TEXT("the ±MeleeHalfAngleDegrees cone") },
		{ SpellLineSweepCpp, TEXT("void ASpellLineSweep::ApplyLineEffectUpTo(float FrontDistance)"), TEXT("LineHalfWidth"),
		  TEXT("the line's reach — without it the sweep hits the whole map") },
		{ SpellLineSweepCpp, TEXT("void ASpellLineSweep::ApplyLineEffectUpTo(float FrontDistance)"), TEXT("AppliedTargets"),
		  TEXT("the once-only bookkeeping that stops a per-tick sweep re-applying its effect") },
		{ CombatStaticsCpp, TEXT("void FSiegeCombatStatics::ApplyRadialDamage("), TEXT("ActorGetDistanceToCollision"),
		  TEXT("the closest-point blast measure — the reason a blast at a castle WALL reaches it") },
		{ CombatStaticsCpp, TEXT("void FSiegeCombatStatics::ApplyRadialDamage("), TEXT("Distance > Radius"),
		  TEXT("the blast radius itself — without it EVERY AoE becomes map-wide") },
	};

	for (const FBodyProbe& Probe : Probes)
	{
		FString Text;
		if (!LoadProjectSource(*this, Probe.RelativePath, Text))
		{
			continue;
		}

		FString Body;
		if (!ExtractFunctionBody(*this, Text, Probe.Signature, Body))
		{
			continue;
		}

		TestTrue(
			FString::Printf(
				TEXT("⭐ `%s` still performs its OWN filtering: `%s` is present (%s). TASK-828 lifted the ")
				TEXT("enumerate-and-team-filter step and NOTHING else — a funnel that swallowed this term ")
				TEXT("would be a shipped behaviour change wearing a refactor's commit message."),
				Probe.Signature, Probe.MustContain, Probe.Why),
			CountOccurrencesInCode(Body, Probe.MustContain) > 0);
	}

	// ── The reverse control: the FUNNEL must contain NONE of the site-local vocabulary. If
	// any of these appeared inside it, the lift went too far and the sites are no longer
	// deciding their own reach.
	FString CombatStaticsSource;
	if (LoadProjectSource(*this, CombatStaticsCpp, CombatStaticsSource))
	{
		FString FunnelBody;
		if (ExtractFunctionBody(*this, CombatStaticsSource,
			TEXT("void FSiegeCombatStatics::GatherTeamAgentsFiltered(const UWorld* World, ETeamId ViewerTeam, bool bWantHostile, TArray<AActor*>& Out)"),
			FunnelBody))
		{
			const TCHAR* const MustNotContain[] =
			{
				TEXT("Radius"), TEXT("Distance"), TEXT("AggroRadius"), TEXT("IsA<"), TEXT("IsDead"), TEXT("IsUnitDead"),
			};
			for (const TCHAR* Banned : MustNotContain)
			{
				TestEqual(
					FString::Printf(
						TEXT("⭐ The funnel contains no `%s` — it is an enumerate-and-team-filter step and nothing ")
						TEXT("more. Distance, liveness and class gating belong to the CALL SITES, which is what ")
						TEXT("keeps this refactor behaviour-neutral at all nine of them."),
						Banned),
					CountOccurrencesInCode(FunnelBody, Banned), 0);
			}

			// ⭐ AND THE ORDER GUARANTEE, which is the subtlest way this refactor could have
			// changed behaviour without changing any SET. Every call site breaks ties by strict
			// improvement (`Distance < Best`, `DistSq >= BestDistSq`), so the FIRST candidate
			// wins a tie. Sorting or reversing here would silently re-pick targets everywhere.
			TestEqual(
				TEXT("⭐⭐ The funnel does NOT sort. Callers break ties by strict improvement, so the enumeration ")
				TEXT("order decides every tie — a Sort here would change which target nine sites pick without ")
				TEXT("changing a single result SET, which is exactly the kind of regression a set-based test misses."),
				CountOccurrencesInCode(FunnelBody, TEXT("Sort(")), 0);

			TestEqual(
				TEXT("⭐ …and it appends in loop order, once per surviving candidate."),
				CountOccurrencesInCode(FunnelBody, TEXT("Out.Add(Candidate)")), 1);

			TestEqual(
				TEXT("⭐ …after resetting Out exactly once."),
				CountOccurrencesInCode(FunnelBody, TEXT("Out.Reset()")), 1);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 ⭐⭐ — `ApplyRadialDamage`: THE MOST DANGEROUS SITE IN THE PROJECT.
//  EVERY AoE routes through it, and its friendly-fire guarantee is re-verified
//  explicitly here because the spec asked for it by name.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelRadialDamageFriendlyFireTest,
	"Siegebound.Acquisition.ApplyRadialDamageStillCannotFriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelRadialDamageFriendlyFireTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	FString Text;
	if (!LoadProjectSource(*this, CombatStaticsCpp, Text))
	{
		return false;
	}

	FString Body;
	if (!ExtractFunctionBody(*this, Text, TEXT("void FSiegeCombatStatics::ApplyRadialDamage("), Body))
	{
		return false;
	}

	// ── SELF-CHECK: prove the extracted body is the real function and not an empty string. ──
	TestTrue(
		TEXT("SELF-CHECK: the extracted ApplyRadialDamage body is substantial — an empty extraction would make ")
		TEXT("every assertion below pass vacuously."),
		Body.Len() > 400);

	// ⭐ THE GUARANTEE: the candidate set is HOSTILE-ONLY, sourced from the funnel. The
	// friendly-fire authority did not move — it was the team filter on the candidate set
	// before, and it is the team filter on the candidate set now. It is deliberately NOT the
	// receiver's instigator chain, because a tower-fired projectile has no resolvable
	// instigator and would otherwise be able to blast its own side.
	// ⚠️ AMENDED BY TASK-829, WHICH THIS ROW'S ORIGINAL COMMENT ASKED FOR BY NAME. The call now
	// carries the veil policy, so the needle carries it too — the friendly-fire guarantee is
	// unchanged (it is still the team filter on the candidate set), and the exemption below is
	// now asserted rather than merely anticipated.
	TestEqual(
		TEXT("⭐⭐ ApplyRadialDamage gathers HOSTILES ONLY, exactly once. If this is zero, EVERY AoE in the game ")
		TEXT("— Sapper suicide, Bomb Tower, Fireball, every blast projectile — is now capable of friendly fire, ")
		TEXT("and no receiver-side check will stop it (GDD §3.0 puts the authority here on purpose)."),
		CountOccurrencesInCode(Body, TEXT("GatherHostileAgents(World, Team, HostileAgents, ESiegeVeilPolicy::IncludeVeiled)")), 1);

	TestEqual(
		TEXT("⭐ …and it gathers FRIENDLIES nowhere. A blast that iterated the friendly lane would damage ")
		TEXT("exactly the actors it must never touch."),
		CountOccurrencesInCode(Body, TEXT("GatherFriendlyAgents")), 0);

	TestEqual(
		TEXT("⭐ The blast reads the world exactly ONCE per call — one gather, not one per candidate. A gather ")
		TEXT("inside the loop would be an O(n^2) full-world scan on every explosion."),
		CountOccurrencesInCode(Body, TEXT("GatherHostileAgents")), 1);

	// ── The damage hop is unchanged: routed through the target's own TakeDamage so
	// per-fortification scaling still applies (Siege → 200% vs castle/buildings).
	TestEqual(
		TEXT("⭐ Damage still routes through the target's own TakeDamage exactly once, so per-fortification ")
		TEXT("scaling (Siege-typed = 200% vs castle/buildings, TASK-054) is untouched by this refactor."),
		CountOccurrencesInCode(Body, TEXT("UGameplayStatics::ApplyDamage(")), 1);

	// ── ⭐⭐ THE EXEMPTION, NOW **ASSERTED** RATHER THAN ANTICIPATED (TASK-829; WITCH-§2 / J-W2).
	// This row was written by TASK-828 as "must NOT be here YET", with the instruction that 829
	// INVERT rather than delete it. Inverted here: a blast is not an act of seeing, so this lane
	// must ASK to see through the veil, out loud, exactly once.
	TestEqual(
		TEXT("⭐⭐ THE BLAST LANE IS EXEMPT FROM VEIL SUPPRESSION, AND IT SAYS SO (WITCH-§2, J-W2): ")
		TEXT("ApplyRadialDamage passes ESiegeVeilPolicy::IncludeVeiled exactly once. ⛔ If this is ZERO, every ")
		TEXT("AoE in the game — Sapper suicide, Bomb Tower, Fireball — silently MISSES invisible units, and no ")
		TEXT("friendly-fire, damage, radius or ordering test in this file would go red. That would delete the ")
		TEXT("50-gold card's ONLY counter and make a veiled push an auto-win."),
		CountOccurrencesInCode(Body, TEXT("ESiegeVeilPolicy::IncludeVeiled")), 1);

	TestEqual(
		TEXT("⛔ …and the blast NEVER asks for suppression. A SuppressVeiled here would be the same defect ")
		TEXT("spelled the other way round."),
		CountOccurrencesInCode(Body, TEXT("SuppressVeiled")), 0);

	// ⭐ THE ORIGINAL ROW SURVIVES UNCHANGED, and it is now saying something different and still
	// worth saying: the exemption is expressed through the NAMED POLICY, never by reading the
	// flag inline. An inline `bIsInvisible` read here would be a second guard point (WITCH-§1)
	// and a second source of truth (WITCH-§6) in the one function every AoE routes through.
	TestEqual(
		TEXT("⛔ ApplyRadialDamage never reads `bIsInvisible` itself — the exemption is a NAMED POLICY passed ")
		TEXT("to the ONE funnel, not a hand-rolled veil check. WITCH-§1 allows exactly one guard point and ")
		TEXT("WITCH-§6 exactly one source of truth; an inline read here would break both in the single most ")
		TEXT("trafficked function in the combat code."),
		CountOccurrencesInCode(Body, TEXT("bIsInvisible")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 ⭐ — THE FRIENDLY LANE IS NAMED, NOT SILENT, AND BATTLE CRY USES IT.
//  The regression this task came closest to shipping.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelFriendlyLaneTest,
	"Siegebound.Acquisition.BattleCryUsesTheFriendlyLaneNotTheHostileOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelFriendlyLaneTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	FString Header;
	if (LoadProjectSource(*this, CombatStaticsHeader, Header))
	{
		TestEqual(
			TEXT("⭐ `GatherFriendlyAgents` is declared exactly once — a NAMED lane, because WITCH-§2's fourth ")
			TEXT("ruling is that friendly acquisition is NEVER veil-suppressed. An invisible unit its own player ")
			TEXT("cannot select, order, heal or buff is a BUG, not a feature, and a lane that is named cannot be ")
			TEXT("accidentally folded into the suppressed one."),
			CountOccurrencesInCode(Header, TEXT("GatherFriendlyAgents")), 1);

		TestEqual(
			TEXT("⭐ `GatherHostileAgents` is declared exactly once beside it."),
			CountOccurrencesInCode(Header, TEXT("GatherHostileAgents")), 1);

		TestEqual(
			TEXT("⭐ The raw enumeration is a SINGLE private helper — one declaration, so there is exactly one ")
			TEXT("place a future veil check can be forgotten."),
			CountOccurrencesInCode(Header, TEXT("GatherTeamAgentsFiltered")), 1);
	}

	FString SpellLibrary;
	if (LoadProjectSource(*this, SpellLibraryCpp, SpellLibrary))
	{
		// ⛔ THE FILE-LOCAL UNFILTERED GATHER IS GONE. It returned the WHOLE roster with no
		// team term at all, which is precisely the shape WITCH-§1 forbids — a caller could
		// have consumed it and seen every veiled unit on the map.
		TestEqual(
			TEXT("⭐⭐ SpellLibrary's file-local `GatherTeamAgents(` helper is GONE. It handed out the ")
			TEXT("UNFILTERED roster — no team term, no veil hook — to three different consumers. Its survival ")
			TEXT("would be a tenth enumeration wearing a local name."),
			CountOccurrencesInCode(SpellLibrary, TEXT("GatherTeamAgents(")), 0);

		FString AllyBuffSlice;
		const int32 AllyBuffIndex = SpellLibrary.Find(TEXT("bool ResolveAllyBuff("), ESearchCase::CaseSensitive);
		if (AllyBuffIndex == INDEX_NONE)
		{
			AddError(TEXT("⛔ `ResolveAllyBuff(` not found in SpellLibrary.cpp — the probe is stale, so it FAILS."));
		}
		else
		{
			// the resolver is inside an anonymous namespace, so ExtractFunctionBody's column-0
			// terminator does not apply; slice forward to the next resolver instead.
			const int32 NextResolver = SpellLibrary.Find(TEXT("bool ResolveGoldSteal("), ESearchCase::CaseSensitive, ESearchDir::FromStart, AllyBuffIndex);
			AllyBuffSlice = (NextResolver == INDEX_NONE)
				? SpellLibrary.Mid(AllyBuffIndex)
				: SpellLibrary.Mid(AllyBuffIndex, NextResolver - AllyBuffIndex);

			TestTrue(
				TEXT("SELF-CHECK: the ResolveAllyBuff slice is substantial — an empty slice would make the two ")
				TEXT("assertions below vacuous."),
				AllyBuffSlice.Len() > 300);

			TestEqual(
				TEXT("⭐⭐ Battle Cry gathers FRIENDLIES. This is the assertion that would have caught the one ")
				TEXT("real regression available in this refactor: routed through the hostile gather, ")
				TEXT("`ResolveAllyBuff` would buff NOBODY — a card silently doing nothing, with a green suite."),
				CountOccurrencesInCode(AllyBuffSlice, TEXT("FSiegeCombatStatics::GatherFriendlyAgents(")), 1);

			TestEqual(
				TEXT("⛔ …and it gathers hostiles NOWHERE. Battle Cry buffing enemies would be worse than ")
				TEXT("buffing nobody."),
				CountOccurrencesInCode(AllyBuffSlice, TEXT("GatherHostileAgents")), 0);

			TestTrue(
				TEXT("⭐ The site's OWN filtering survived: the unit type gate and the liveness check are still ")
				TEXT("inside ResolveAllyBuff (the hero is not a unit and is never buffed; a dead unit is skipped)."),
				CountOccurrencesInCode(AllyBuffSlice, TEXT("IsUnitDead()")) > 0
				&& CountOccurrencesInCode(AllyBuffSlice, TEXT("Cast<ASummonedUnit>(Candidate)")) > 0);

			TestTrue(
				TEXT("⭐ …as did its radius gate — Battle Cry is an AoE buff, not a global one."),
				CountOccurrencesInCode(AllyBuffSlice, TEXT("Row.AoERadius")) > 0);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 9 ⭐⭐ — ⛔ NO SUPPRESSION DECISION OUTSIDE THE FUNNEL, EVER — AND THE VEIL'S
//  WRITE-DOORS ARE A ⛔ CLOSED, ⛔ EXACTLY-COUNTED SET ON THE ONE CLASS THAT OWNS THEM.
//
//  ⛔⛔ AMENDED BY TASK-868, AND THE AMENDMENT IS THE INTERESTING PART. As written for
//  TASK-828 this row claimed five tokens read ZERO in five files "EVER, INCLUDING AFTER
//  TASK-829 LANDS". ⛔ THAT CLAIM WAS FALSE THE MOMENT TASK-829 LANDED: `WITCH-§6`
//  requires the two write-doors to live on `ASummonedUnit`, so two of the five tokens
//  became legitimately non-zero in one of the five files and the row went red on
//  ⛔ CORRECT CODE. ⛔ The guard was out of date; the code it guarded was right.
//
//  ⭐ THE LESSON, WORTH MORE THAN THE FIX: the original row bundled a DECISION ("does
//  this site decide whether a unit is visible?" — the real law) with a STATE mention
//  ("does this site name the flag at all?" — a proxy that only held while the veil had
//  no implementation). ⛔ A guard whose subject is a PROXY expires when the thing it
//  proxies for gets built, and it expires by going RED ON THE FEATURE LANDING — which
//  is the moment everyone is most tempted to loosen it.
// ═══════════════════════════════════════════════════════════════════════════════

// ⚠️ ON THE REGISTERED NAME, RAISED BY TASK-1008 RATHER THAN QUIETLY CHANGED. Since that row the
// fog clamp lives at ⛔ TWO chokepoints, not one: the acquisition FUNNEL, and the unit-side reach
// door the funnel structurally cannot stand in for. ⇒ read "the funnel" below as "THE CHOKEPOINT",
// which is the property this test has always actually defended — the DECISION is never made at a
// call site. ⛔ The identifier is left alone deliberately: an automation test's registered name is
// referenced from outside this file, and renaming it is a different kind of change from extending
// a token list. Routed as a finding.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeAcquisitionFunnelSuppressionLivesOnlyInTheFunnelTest,
	"Siegebound.Acquisition.VeilAndFogSuppressionLiveOnlyInTheFunnel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeAcquisitionFunnelSuppressionLivesOnlyInTheFunnelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeAcquisitionFunnelFixture;

	// ⛔ The five files that hold acquisition CALL SITES but must never hold suppression.
	// `SiegeCombatStatics.cpp` is deliberately absent: it is where the veil check and the fog
	// clamp are SUPPOSED to land (WITCH-§1). ⇒ this test survives TASK-829 and TASK-838
	// unchanged, and stays load-bearing for every task after them.
	const TCHAR* const CallSiteFiles[] =
	{
		SummonedUnitCpp, TowerCpp, HeroCpp, SpellLineSweepCpp, CheatManagerCpp,
	};

	// ⭐⭐ THESE ARE THE REAL SHIPPED SYMBOL NAMES, READ OFF TASK-827's AND TASK-837's LANDED
	// HEADERS — ⛔ not plausible-looking guesses. A guard that named a symbol nobody will ever
	// write is a guard that can never fire, which is the most expensive kind of green.
	//   • `FSiegeInvisibilityStatics` / `IsVisibleTo(`  — SiegeInvisibilityStatics.h (TASK-827)
	//   • `bIsInvisible`                                — the per-instance veil state (TASK-829)
	//   • `FSiegeFogStatics` / `EffectiveVisionRadius`  — SiegeFogStatics.h (TASK-837)
	//   • `ResolveFogClampedReachUU(`                   — SiegeCombatStatics.h (TASK-1007), the fog
	//                                                     REACH seam. ⛔ ADDED BY TASK-1008, which
	//                                                     is the row that gave it its first caller.
	// ⛔ If a future task renames one of these, this list is updated in the SAME commit — a
	// renamed symbol must never be allowed to silently retire its own guard.
	// ⛔⛔ AND THE SAME RULE BINDS A ⛔ NEW DOOR, WHICH IS THE HARDER HALF AND THE ONE THIS FILE
	// LEARNED IN 2026-09: a symbol nobody thought to add is INDISTINGUISHABLE, from inside this
	// test, from a symbol that is genuinely absent. The seam above matched none of the three
	// needles that preceded it, so this row was ⛔ GREEN over its own subject for a full day.
	//
	// ⚠️ `SpellLibrary.cpp` is deliberately NOT in the file list below: it is a spell resolver
	// and may legitimately gain a "reveal" spell that reads veil state for its own effect,
	// which is not an acquisition guard point.
	//
	// ⛔⛔ TASK-868 — WHY THIS TEST NO LONGER ASSERTS ONE FLAT LIST OF FIVE ZEROS, AND WHY THAT IS
	// ⛔ NOT A RELAXATION. As shipped by TASK-828 the row demanded all five tokens read ZERO in all
	// five files. TASK-829 then landed the veil's TWO WRITE-DOORS — `GrantInvisibility` and
	// `BreakInvisibility` — on `ASummonedUnit`, because `WITCH-§6` requires them to live on the
	// actor that OWNS the flag. ⇒ two of the five tokens became legitimately NON-ZERO in exactly
	// ONE of the five files, and the row went red on CORRECT code.
	// ⭐⭐ THE DISTINCTION THE ORIGINAL ROW BLURRED, AND WHICH IS THE WHOLE FIX: a call site must
	// never make the suppression DECISION (that is the forgotten-guard-point failure `WITCH-§1`
	// exists to prevent) — but `SummonedUnit.cpp` does not decide anything. It HOLDS the state and
	// exposes the two doors that mutate it. `IsVisibleTo(` — the DECISION — still reads ZERO there,
	// and that is the assertion that was actually load-bearing all along.
	// ⇒ the tokens are split into DECISION tokens and STATE tokens (zero in four, with the owner
	// exempted BY NAME), and the exemption is ⛔ NOT a hole: it is replaced below by a STRICTER,
	// EXACT, ENUMERATED census of the owner's write-doors.
	// ⚠️⚠️ AMENDED BY TASK-1008 — THE DECISION HALF IS ⛔ NO LONGER "ZERO IN ALL FIVE", AND THAT
	// SENTENCE IS ⛔ RETIRED RATHER THAN LEFT LYING. Jonathan ruled fog must clamp RETENTION and
	// FIRING, not only acquisition, and a gather-time funnel structurally cannot answer either —
	// so `SummonedUnit.cpp` now legitimately names the fog REACH seam ⛔ exactly once, inside one
	// function. ⭐ THE REPAIR IS THE SAME SHAPE TASK-868 CHOSE AND FOR THE SAME REASON, ⛔ one
	// generation stricter: the DECISION half is now zero ⛔ EXCEPT where an AUTHORISED-CHOKEPOINT
	// TABLE names a file, a token, a FUNCTION and a REASON — and every expected count is ⛔ DERIVED
	// from that table. ⇒ ⛔ nothing was exempted; a door was ⛔ described.

	// ── DECISION tokens: the suppression/clamp DECISION is made at a CHOKEPOINT and nowhere else.
	//    ⛔ ZERO in all five files — ⛔ EXCEPT where the AUTHORISED-CHOKEPOINT TABLE below says
	//    otherwise, and there the expected count is ⛔ DERIVED FROM THAT TABLE, ⛔ never typed.
	//
	// ⛔⛔⛔ TASK-1008 ADDED THE FOURTH TOKEN, AND WHY IT HAD TO IS THE WHOLE LESSON OF THIS BLOCK.
	// Until that row this list named exactly three needles — `IsVisibleTo(`, `FSiegeFogStatics`
	// and `EffectiveVisionRadius`. TASK-1007 then shipped the fog REACH seam,
	// `FSiegeCombatStatics::ResolveFogClampedReachUU`, whose name matches ⛔ NONE OF THE THREE.
	// ⇒ ⛔⛔ THIS ROW WOULD HAVE GONE GREEN OVER A DIFF THAT WIRED FOG INTO ⛔ NINE REACH SITES IN
	// `SummonedUnit.cpp`, ⛔ WHILE BELIEVING IT WAS WATCHING — measured, not predicted, and
	// recorded as OWED in `qa/TASK-1009.md` W-5 before the wiring was written.
	// ⚖️ ⛔ A GUARD THAT PASSES A CHANGE BY ⛔ ACCIDENT IS ⛔ WORSE THAN NO GUARD: it does not merely
	// fail to catch, it ⛔ CERTIFIES. `SHIP-§9` — ⛔ validate a gate against the FAILURE it detects,
	// ⛔ never merely against success. (This one was seen RED before it was trusted; the handoff
	// records the two mutations and both counts.)
	// ⭐ AND THE SYMBOL WAS NAMED HERE IN THE ⛔ SAME COMMIT THAT CREATED ITS CALLER — the rule the
	// paragraph above already states for a RENAME, applied identically to a ⛔ NEW DOOR. A guard is
	// extended by the row that makes it extendable, or ⛔ nobody ever extends it.
	const TCHAR* const DecisionTokens[] =
	{
		TEXT("IsVisibleTo("),
		TEXT("FSiegeFogStatics"),
		TEXT("EffectiveVisionRadius"),
		TEXT("ResolveFogClampedReachUU("),
	};

	// ═══════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐⭐ THE AUTHORISED-CHOKEPOINT TABLE — ⛔ THE ONLY WAY A DECISION TOKEN MAY READ NON-ZERO.
	//
	//  ⛔⛔ IT IS ⛔ NOT AN EXEMPTION LIST, AND THAT DIFFERENCE IS THE ENTIRE DESIGN. A bare
	//  exemption ("`SummonedUnit.cpp` may name the reach seam") would license ⛔ NINE calls as
	//  happily as one, and would say ⛔ NOTHING about WHERE they live — which is exactly the
	//  per-site-clamp failure this whole test exists to prevent, re-admitted through the door
	//  marked "allowed". Every entry below therefore carries ⛔ THREE things:
	//    (a) the ⛔ ONE function in that file that may hold the token (matched by SIGNATURE, so a
	//        call that moves to another function goes RED even though the file count is right);
	//    (b) a ⛔ WRITTEN-DOWN WHY, printed into every failure message this table produces;
	//    (c) an implied count of ⛔ EXACTLY ONE — asserted per entry, below the file loop.
	//  ⛔ The per-file expectation is DERIVED by counting the entries naming that file and token,
	//  ⛔ never typed (`handoffs/TASK-980-programmer.md` §4(f), the method TASK-1007 applied to the
	//  sibling pin in `Tests/SiegeFogClampTest.cpp`).
	//
	//  ⇒ ⭐⭐ AN AUTHORISED CONSUMER IS ADDED BY ⛔ WRITING DOWN A REASON. ⛔ The number follows.
	//  ⛔ Bumping a number to clear a red is the move this shape is built to make impossible, and
	//  relaxing the token list is an ⛔ AUTOMATIC QA FAIL by this row's own message below.
	// ═══════════════════════════════════════════════════════════════════════════════════════════

	struct FAuthorisedFogCeilingChokepoint
	{
		/** The call-site file permitted to hold the token — ⛔ one of CallSiteFiles above. */
		const TCHAR* File;
		/** ⛔ WHICH decision token this entry authorises. An entry licenses ONE, ⛔ never all. */
		const TCHAR* Token;
		/** The ⛔ ONE function that may hold it, by signature (ExtractFunctionBody's needle). */
		const TCHAR* ChokepointSignature;
		/** ⛔ Printed into every message below. ⛔ An entry with no reason is not an entry. */
		const TCHAR* WhyThisChokepointIsAuthorised;
	};

	const FAuthorisedFogCeilingChokepoint AuthorisedFogCeilingChokepoints[] =
	{
		{
			SummonedUnitCpp,
			TEXT("ResolveFogClampedReachUU("),
			TEXT("float ASummonedUnit::ApplyFogVisionCeilingUU(float RequestedReachUU) const"),
			TEXT("THE UNIT-REACH CHOKEPOINT (TASK-1008; law FOG-§9.11's retention clause, FOG-§9.6). ")
			TEXT("The acquisition funnel runs at GATHER time, so it is STRUCTURALLY incapable of ")
			TEXT("bounding what an ALREADY-ACQUIRED unit keeps chasing, what it shoots at between ")
			TEXT("gathers, or what it notices from inside a commanded zone it gathered UNBOUNDED. ")
			TEXT("Jonathan's 'yes clamp retention under fog' is a question the funnel cannot be asked. ")
			TEXT("NINE reach sites in that file hand their OWN reach to THIS one function — six firing ")
			TEXT("gates, two leash drops and the commanded notice bound — and it is the ONLY place in ")
			TEXT("ASummonedUnit permitted to name the fog rule."),
		},
	};

	TestTrue(
		TEXT("⭐ SELF-CHECK: the authorised-chokepoint table is NOT EMPTY. ⛔ An emptied table would make ")
		TEXT("every derived expectation below ZERO — which is still the SAFE direction (a live call would go ")
		TEXT("red), but it would silently retire the per-entry assertions, which are the half that pins WHERE ")
		TEXT("the call lives. A vacuous table must fail here rather than pass quietly."),
		static_cast<int32>(UE_ARRAY_COUNT(AuthorisedFogCeilingChokepoints)) > 0);

	// ── STATE tokens: ⛔ ZERO IN THE FOUR NON-OWNER FILES. A mirrored veil flag on ATower, on
	//    AHeroCharacter, in the line sweep or in the cheat manager is a SECOND source of truth for
	//    a per-instance state, which is a different defect from a second guard point and just as
	//    fatal. ⛔ `SummonedUnit.cpp` is absent BY NAME, not by accident — see the census below.
	const TCHAR* const StateTokens[] =
	{
		TEXT("bIsInvisible"),
		TEXT("FSiegeInvisibilityStatics"),
	};

	const TCHAR* const NonOwnerCallSiteFiles[] =
	{
		TowerCpp, HeroCpp, SpellLineSweepCpp, CheatManagerCpp,
	};

	for (const TCHAR* File : CallSiteFiles)
	{
		FString Text;
		if (!LoadProjectSource(*this, File, Text))
		{
			continue;
		}

		// ── POSITIVE CONTROL: the file really was read and really does contain a funnel call. ──
		TestTrue(
			FString::Printf(
				TEXT("SELF-CHECK: %s contains at least one funnel call, so the absences below are claims about a ")
				TEXT("real acquisition file rather than an empty read."),
				*FPaths::GetCleanFilename(FString(File))),
			CountOccurrencesInCode(Text, TEXT("FSiegeCombatStatics::Gather")) > 0);

		for (const TCHAR* Token : DecisionTokens)
		{
			// ⛔ DERIVED, ⛔ NEVER TYPED: how many table entries authorise THIS token in THIS file.
			// ⛔ Zero for every (file, token) pair nobody wrote a reason for — which is still all
			// twenty of them but one — and EXACTLY ONE where a reason exists. ⭐ THAT ONE IS THE
			// PER-FILE CAP, and it is what keeps "one door" a PROPERTY rather than an aspiration:
			// the nine reach sites in SummonedUnit.cpp can never clamp individually, because the
			// second of them would make this count 2 against a derived 1.
			int32 AuthorisedHere = 0;
			for (const FAuthorisedFogCeilingChokepoint& Entry : AuthorisedFogCeilingChokepoints)
			{
				if (FCString::Strcmp(Entry.File, File) == 0 && FCString::Strcmp(Entry.Token, Token) == 0)
				{
					++AuthorisedHere;
				}
			}

			TestEqual(
				FString::Printf(
					TEXT("⭐⭐ %s names `%s` EXACTLY %d time(s) — a count DERIVED from the authorised-chokepoint ")
					TEXT("table, ⛔ never typed. THIS IS THE PERMANENT LAW, not a TASK-828 snapshot: the veil ")
					TEXT("SUPPRESSION DECISION and the fog range clamp are applied at a CHOKEPOINT — inside ")
					TEXT("FSiegeCombatStatics::GatherHostileAgents for ACQUISITION (WITCH-§1), and for the ")
					TEXT("reaches a gather-time funnel structurally cannot see (FIRING / RETENTION / the ")
					TEXT("commanded notice bound) at the ONE unit-side door named in that table. A per-SITE ")
					TEXT("check is the forgotten-guard-point failure the whole funnel exists to prevent, and it ")
					TEXT("is an automatic QA FAIL. ⛔ Do NOT fix a red here by relaxing this row and ⛔ do NOT fix ")
					TEXT("it by bumping a number — route an acquisition through the funnel, or route a reach ")
					TEXT("through the authorised chokepoint. A NEW chokepoint is added by writing its REASON ")
					TEXT("into the table, and then the count moves by itself.%s"),
					*FPaths::GetCleanFilename(FString(File)), Token, AuthorisedHere,
					(AuthorisedHere > 0)
						? TEXT("\n  ⭐ AUTHORISED HERE BECAUSE: see the table entry's own reason, asserted per entry below.")
						: TEXT("")),
				CountOccurrencesInCode(Text, Token), AuthorisedHere);
		}
	}

	// ═══════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐⭐ THE PER-ENTRY HALF — ⛔ AND WITHOUT IT THE TABLE IS ARITHMETIC RATHER THAN A CLAIM.
	//
	//  ⛔ The file-scoped count above says HOW MANY. It says ⛔ NOTHING about ⛔ WHERE. A diff that
	//  deleted the chokepoint's body and clamped at ⛔ one reach site instead would satisfy it
	//  exactly — one call, right file, ⛔ wrong place, and the "one door" property gone with
	//  nothing red. ⇒ each entry is re-asserted ⛔ INSIDE the ONE function it names.
	//  ⛔ A stale signature FAILS here (ExtractFunctionBody AddErrors) rather than scanning nothing
	//  and passing, which is the failure mode a body-scoped probe has to be built against.
	// ═══════════════════════════════════════════════════════════════════════════════════════════

	for (const FAuthorisedFogCeilingChokepoint& Entry : AuthorisedFogCeilingChokepoints)
	{
		FString EntryFileText;
		if (!LoadProjectSource(*this, Entry.File, EntryFileText))
		{
			continue;
		}

		FString ChokepointBody;
		if (!ExtractFunctionBody(*this, EntryFileText, Entry.ChokepointSignature, ChokepointBody))
		{
			continue; // already an AddError — a stale probe FAILS rather than reporting safe
		}

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐⭐ THE AUTHORISED CALL IS ⛔ INSIDE ITS OWN CHOKEPOINT: `%s` appears EXACTLY ONCE in ")
				TEXT("`%s`, in %s. ⛔ The file-level count above cannot see this: one call in the RIGHT file but ")
				TEXT("the WRONG function passes it and destroys the property it was protecting. ⛔ WHY THIS ")
				TEXT("CHOKEPOINT IS AUTHORISED AT ALL: %s"),
				Entry.Token, *FPaths::GetCleanFilename(FString(Entry.File)),
				Entry.ChokepointSignature, Entry.WhyThisChokepointIsAuthorised),
			CountOccurrencesInCode(ChokepointBody, Entry.Token), 1);

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ PROSE-IMMUNITY of `%s` inside %s: the count reads IDENTICALLY with the comment-skip ")
				TEXT("on and off, so the pin reads STRUCTURE and not DOCUMENTATION. ⛔ A red here means the ")
				TEXT("needle is now being WRITTEN INSIDE A COMMENT in that body, which makes both numbers above ")
				TEXT("untrustworthy in BOTH directions — fix the comment, ⛔ do NOT adjust the pin."),
				Entry.Token, Entry.ChokepointSignature),
			CountOccurrencesIncludingComments(ChokepointBody, Entry.Token),
			CountOccurrencesInCode(ChokepointBody, Entry.Token));
	}

	for (const TCHAR* File : NonOwnerCallSiteFiles)
	{
		FString Text;
		if (!LoadProjectSource(*this, File, Text))
		{
			continue;
		}

		for (const TCHAR* Token : StateTokens)
		{
			TestEqual(
				FString::Printf(
					TEXT("⭐⭐ %s contains no `%s`. The veil is per-instance state owned by ASummonedUnit and ")
					TEXT("mutated through exactly two doors on that class (WITCH-§6). A copy of the flag, or a ")
					TEXT("direct reach into FSiegeInvisibilityStatics, ANYWHERE else is a second source of truth ")
					TEXT("for a value that must have one — and it would disagree with the funnel silently. ")
					TEXT("⛔ SummonedUnit.cpp is exempt from THIS row and only this row; its own write-doors are ")
					TEXT("pinned EXACTLY below."),
					*FPaths::GetCleanFilename(FString(File)), Token),
				CountOccurrencesInCode(Text, Token), 0);
		}
	}

	// ═══════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ THE INSTRUMENT CONTROL — ⛔ BOTH DIRECTIONS, ⛔ BEFORE ANY NUMBER BELOW IS TRUSTED.
	//
	//  `SC-§39`: `CountOccurrencesInCode` is blind ⛔ TWO WAYS, and ⛔ a single control only proves
	//  ⛔ ONE of them. TASK-867 measured the skip rule ⛔ EATING 16 REAL HITS in one header;
	//  TASK-851 measured a ⛔ TRAILING line-comment on an `#include` ⛔ MANUFACTURING a false hit
	//  and a bare-token needle reading ⛔ 4 where the truth was ⛔ 3.
	//
	//  ⛔ THE SECOND ONE IS ⛔ LIVE IN THIS TEST'S OWN SUBJECT: `SummonedUnit.cpp`'s
	//  `#include "Siegebound/SiegeInvisibilityStatics.h"` carries a trailing comment that NAMES
	//  both state tokens, so a BARE-token census of that file reads ⛔ ONE MORE than the truth for
	//  each. ⇒ ⛔ pinning a bare token there would pin the ⛔ WORDING OF A COMMENT, which is the
	//  exact way this guard would rot a second time.
	//
	//  ⭐ THE PROBE IS ⛔ SYNTHETIC AND ⛔ IN-MEMORY on purpose: a control read off the live tree
	//  goes red when somebody reflows a comment, which is a false alarm on a row that blocks
	//  commits. Its symbol is ⛔ MEASURED ABSENT from Source/ (0 hits across the tree at authoring
	//  time, with FSiegeInvisibilityStatics at 75 hits as the positive control that the absence
	//  scan was not itself blind) — `SC-§40` cl. 10: an invented token is measured absent, ⛔ never
	//  assumed absent.
	// ═══════════════════════════════════════════════════════════════════════════════════════════

	const FString InstrumentProbe =
		FString(TEXT("#include \"X.h\" // this trailing comment NAMES FSiegeVeilProbeControlSymbol\n"))
		+ TEXT("/*  FSiegeVeilProbeControlSymbol(HiddenByTheSkipRule);  */\n")
		+ TEXT("\tFSiegeVeilProbeControlSymbol(TheOneRealCall);\n");

	// The probe holds exactly ONE real call. Everything else is comment text.
	const int32 ProbeBareInCode = CountOccurrencesInCode(InstrumentProbe, TEXT("FSiegeVeilProbeControlSymbol"));
	const int32 ProbeCallInCode = CountOccurrencesInCode(InstrumentProbe, TEXT("FSiegeVeilProbeControlSymbol("));
	const int32 ProbeCallRaw = CountOccurrencesIncludingComments(InstrumentProbe, TEXT("FSiegeVeilProbeControlSymbol("));

	TestEqual(
		TEXT("⭐ CONTROL, DIRECTION 1 — OVER-COUNT (the false hit). The probe contains ONE real call, but a ")
		TEXT("BARE-token needle reads TWO: the trailing line-comment on the #include sits on a CODE line, so it ")
		TEXT("survives every skip rule and is counted. ⛔ This is why every pinned needle below carries an open ")
		TEXT("paren — the paren is what tells a CALL apart from PROSE."),
		ProbeBareInCode, 2);

	TestEqual(
		TEXT("⭐ …and the CALL-SHAPED needle reads the TRUTH (one) on the same text. The two numbers differing is ")
		TEXT("the whole justification for the needle style used below; if they ever agree, the probe has stopped ")
		TEXT("exercising the pathology and this control has gone vacuous."),
		ProbeCallInCode, 1);

	TestEqual(
		TEXT("⭐ CONTROL, DIRECTION 2 — UNDER-COUNT (the eaten hit). Read WITHOUT the skip rule the same ")
		TEXT("call-shaped needle finds TWO, because the block-comment line hides one from the house helper. ⛔ A ")
		TEXT("guard whose subject is written one-argument-per-line, or inside a block comment, is INVISIBLE to ")
		TEXT("the very test enforcing it — that is SC-§39's amendment, reproduced here rather than cited."),
		ProbeCallRaw, 2);

	// ═══════════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐⭐ THE CLOSED WRITE-DOOR CENSUS — `SummonedUnit.cpp`, EXACT, ENUMERATED, PROSE-IMMUNE.
	//
	//  ⛔⛔ THIS IS THE GUARANTEE THE WHOLE TEST EXISTS FOR, AND IT IS ⛔ STRICTER THAN THE ZERO IT
	//  REPLACES: the veil has a ⛔ CLOSED SET of write-doors, so a grep for the break door is the
	//  ⛔ COMPLETE list of ways a unit loses its veil. TASK-829 designed the feature around that
	//  property being checkable. ⛔ A `>=`, a range or a tolerance here would DELETE it.
	//
	//  ⛔ THE THREE ADMITTED SITES, NAMED, WITH THE REASON EACH IS LEGITIMATE:
	//    1. `FSiegeInvisibilityStatics::ApplyVeil(bIsInvisible)` — inside ASummonedUnit::
	//       GrantInvisibility. The ⛔ ONE write-true door. `WITCH-§6` puts it on this class.
	//    2. `FSiegeInvisibilityStatics::ApplyBreak(bIsInvisible, Reason)` — inside ASummonedUnit::
	//       BreakInvisibility. The ⛔ ONE write-false door, and the edge that fires exactly once
	//       per veil is why `WITCH-§6` can ban a "was visible" cache.
	//    3. `FSiegeInvisibilityStatics::ToString(Reason)` — the VERBOSE log line in the same
	//       function. ⭐ ADMITTED AND DELIBERATELY ⛔ NOT PINNED: it is a pure diagnostic that
	//       touches no state. Pinning it would make ADDING A LOG LINE a suite failure, and a pin
	//       that fires on a log line is how a guard gets loosened by the next person in a hurry.
	//
	//  ⛔ WHY THESE NEEDLES AND NOT THE BARE TOKENS: every needle below carries an open paren or an
	//  assignment operator, so ⛔ none of them can be moved by editing a comment. That is asserted,
	//  not asserted-by-hope — see the IMMUNITY row at the end of this block.
	// ═══════════════════════════════════════════════════════════════════════════════════════════

	FString OwnerText;
	if (LoadProjectSource(*this, SummonedUnitCpp, OwnerText))
	{
		TestEqual(
			TEXT("⭐⭐ THE ONE WRITE-TRUE DOOR: `FSiegeInvisibilityStatics::ApplyVeil(` is called EXACTLY ONCE in ")
			TEXT("SummonedUnit.cpp — inside ASummonedUnit::GrantInvisibility. ⛔ A SECOND call is a second way to ")
			TEXT("veil a unit, which breaks the closed-set property TASK-829's design rests on. ⛔ Do NOT fix a red ")
			TEXT("here by bumping the number: route the new caller through GrantInvisibility(), which is what every ")
			TEXT("legitimate consumer already does. ⛔ If the door genuinely moved, this row moves in the SAME commit ")
			TEXT("and the handoff names the new site."),
			CountOccurrencesInCode(OwnerText, TEXT("FSiegeInvisibilityStatics::ApplyVeil(")), 1);

		TestEqual(
			TEXT("⭐⭐ THE ONE WRITE-FALSE DOOR: `FSiegeInvisibilityStatics::ApplyBreak(` is called EXACTLY ONCE in ")
			TEXT("SummonedUnit.cpp — inside ASummonedUnit::BreakInvisibility. ⛔ This is the grep WITCH-§6 promises ")
			TEXT("is complete: if it is complete, 'why did the unit become visible?' has a bounded answer. ⛔ A ")
			TEXT("second call makes that question unanswerable by reading. Route new callers through ")
			TEXT("BreakInvisibility(ESiegeVeilBreakReason::…) instead — the six shipped verb sites all do."),
			CountOccurrencesInCode(OwnerText, TEXT("FSiegeInvisibilityStatics::ApplyBreak(")), 1);

		// ⭐⭐ THE CLOSURE PIN, and it is the one that catches a door NOBODY HAS WRITTEN YET. The two
		// pins above only know the names of TODAY's two statics; a THIRD static invented next month
		// (`ApplyDecay`, `ApplyRefresh`, …) would slip past both. Every write-door must hand the flag
		// to something BY REFERENCE, so counting the parenthesised mentions of the flag itself bounds
		// the doors WITHOUT knowing their names.
		TestEqual(
			TEXT("⭐⭐⭐ THE CLOSURE PIN: the raw veil flag is handed to a call EXACTLY TWICE in SummonedUnit.cpp, ")
			TEXT("and both are the doors named above (ApplyVeil, ApplyBreak). ⛔ THIS IS THE ROW THAT CATCHES A ")
			TEXT("WRITE-DOOR WHOSE NAME DOES NOT EXIST YET: a third FSiegeInvisibilityStatics entry point taking ")
			TEXT("`bool&` would pass both name pins above and fail HERE. ⚠️ It also fires on a raw parenthesised ")
			TEXT("READ of the flag — that over-catch is DELIBERATE: the class already exposes IsInvisible() for ")
			TEXT("reads, so a raw read is off-pattern and worth one look. ⛔ If a new site is legitimate, name it ")
			TEXT("here and move the number in the SAME commit."),
			CountOccurrencesInCode(OwnerText, TEXT("(bIsInvisible")), 2);

		// ⭐ THE INLINED-ASSIGNMENT BAN, which `WITCH-§6` states BY NAME. The closure pin above cannot
		// see this one: `bIsInvisible = false;` written as a bare statement has no parenthesis.
		// ⛔ The comparison count is SUBTRACTED rather than ignored — `bIsInvisible ==` contains
		// `bIsInvisible =` as a prefix, so an honest future `if (bIsInvisible == …)` would otherwise
		// read as an assignment. The subtraction is exact: the needle matches a comparison once.
		const int32 SpacedAssignments =
			CountOccurrencesInCode(OwnerText, TEXT("bIsInvisible ="))
			- CountOccurrencesInCode(OwnerText, TEXT("bIsInvisible =="));
		const int32 TightAssignments =
			CountOccurrencesInCode(OwnerText, TEXT("bIsInvisible="))
			- CountOccurrencesInCode(OwnerText, TEXT("bIsInvisible=="));

		TestEqual(
			TEXT("⭐⭐ ZERO INLINED ASSIGNMENTS: `WITCH-§6` bans an inlined `bIsInvisible = false` ANYWHERE by name, ")
			TEXT("because an assignment bypasses ApplyBreak's true-exactly-once edge — the one-shot side effects ")
			TEXT("(the log, and the WITCH-§5 material swap that will hang off the same edge) would simply not ")
			TEXT("happen, and the unit would be visible with nothing in any log to say why. ⛔ Both spellings are ")
			TEXT("counted and comparisons are subtracted, so an `==` read cannot masquerade as a violation."),
			SpacedAssignments + TightAssignments, 0);

		// ── IMMUNITY: the three pinned positive counts read the SAME with the skip rule OFF. ──
		// ⛔ This is what makes the numbers above safe to pin in a file that is under active edit:
		// none of them can be moved by writing, reflowing or deleting a COMMENT. Contrast the bare
		// tokens, which demonstrably can — the row after this one measures exactly that.
		// ⭐ TASK-1008 adds the fog reach seam's needle here for the SAME reason the three above are
		// here: its file-scoped count is now PINNED (at the table-derived 1), and a pinned number
		// in a file under active edit must be provably immune to somebody writing, reflowing or
		// deleting a COMMENT. ⛔ The consequence, stated so it is a choice and not a surprise: the
		// shipped comments in `SummonedUnit.cpp` deliberately name the seam WITHOUT its open paren,
		// so that a paragraph explaining the rule can never be counted as an application of it.
		const TCHAR* const ImmuneNeedles[] =
		{
			TEXT("FSiegeInvisibilityStatics::ApplyVeil("),
			TEXT("FSiegeInvisibilityStatics::ApplyBreak("),
			TEXT("(bIsInvisible"),
			TEXT("ResolveFogClampedReachUU("),
		};

		for (const TCHAR* Needle : ImmuneNeedles)
		{
			TestEqual(
				FString::Printf(
					TEXT("⭐⭐ PROSE-IMMUNITY of `%s`: the count is IDENTICAL with the comment-skip on and off, so ")
					TEXT("this pin reads STRUCTURE and not DOCUMENTATION. ⛔ A red here means the needle is now ")
					TEXT("being written inside a comment somewhere in SummonedUnit.cpp, which makes the pinned ")
					TEXT("number above untrustworthy in BOTH directions — fix the comment, do NOT adjust the pin."),
					Needle),
				CountOccurrencesIncludingComments(OwnerText, Needle),
				CountOccurrencesInCode(OwnerText, Needle));
		}

		// ── ⛔ WHY THERE IS NO LIVE-FILE COUNTER-EXAMPLE ROW HERE, STATED SO THE ABSENCE IS NOT READ
		//    AS AN OVERSIGHT. An earlier draft of TASK-868 asserted, on the live file, that the bare
		//    token reads STRICTLY MORE than the structural needle — a true and vivid demonstration
		//    that the bare token is the wrong instrument (measured: 7 raw / 3 skip-aware / 2
		//    structural). ⛔ IT WAS REMOVED ON PURPOSE. Its truth depends on how many COMMENT LINES
		//    happen to quote `WITCH-§6`, so deleting documentation would turn it red — and a row that
		//    goes red for a reason that is NOT IN THE DIFF is precisely what tempts the next person
		//    to loosen the guard (`SC-§39`'s closing warning), on a test that blocks commits.
		//    ⭐ Nothing is lost: the synthetic probe above proves BOTH blind directions
		//    deterministically and can never false-fire, and the IMMUNITY rows prove every number
		//    actually pinned here is comment-independent. The demonstration belongs in prose; only
		//    the invariant belongs in an assertion.
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
