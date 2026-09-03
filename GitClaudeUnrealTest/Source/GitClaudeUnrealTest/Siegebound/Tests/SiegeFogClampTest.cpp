// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "Math/NumericLimits.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/SiegeCombatStatics.h"
#include "Siegebound/SiegeFogStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE FOG VISION CEILING'S **WIRING** (TASK-838; law `FOG-§4(a)`,
 *      `FOG-§6`, `FOG-§7`, `FOG-§7a`, `FOG-§7b`, ruling 🧑 `J-F9`) ═══
 *
 *  ⛔⛔ THIS FILE DELIBERATELY DOES **NOT** RE-ASSERT `TASK-837`'s ARITHMETIC. `SiegeFogTest.cpp`
 *  already pins the conversion, the quadratic falloff, the hard cut, the fog-off bit-identity,
 *  the never-lengthens sweep and `FOG-§2`'s whole consequence table, in nine tests. Re-running
 *  those here would grow the suite without growing its coverage, and a second copy of a table is
 *  a second place for it to rot. ⇒ **every row below asserts WHICH LANE IS WIRED TO THE CEILING
 *  AND WHICH IS NOT** — the one thing `TASK-838` actually decided.
 *
 *  ⭐⭐ THE CLAIM THESE TESTS DEFEND, in one sentence: `WITCH-§1` made ONE chokepoint, so the
 *  chokepoint is now the shared road for consumers that are ⛔ NOT acts of seeing, and a BLIND
 *  clamp there would answer three different questions with one answer (`FOG-§7`). The exemption
 *  is expressed by ⛔ what the call site hands over — never by a branch inside the funnel that
 *  inspects its caller, never by a magic list of exempt sites.
 *
 *  ⚠️⚠️ THE TRAP THESE ROWS ARE WRITTEN AGAINST: **almost every AoE radius in the shipped game is
 *  already UNDER the 609.6 ceiling** — Sapper 250, BombTower 250, Wizard 250, Fireball 300,
 *  FrostNova 350, BattleCry 400. ⇒ a blast-exemption test drawn from today's data ⛔ PASSES
 *  WHETHER THE EXEMPTION EXISTS OR NOT, and would report SAFE forever. The exemption is therefore
 *  asserted at a radius ⛔ DERIVED FROM THE CEILING (2×), never at a design number.
 *
 *  ⛔⛔ AND THE WORD "ALMOST" IS A **MEASURED CORRECTION TO THE LAW**, made while writing this
 *  file: `FOG-§7` and `TASK-838(5b)` both assert *every* AoE radius is under the ceiling, and
 *  ⛔ **that is FALSE — `Lightning` ships `AoERadius = 700`** (`Docs/Data/cards.csv`), 90.4 uu
 *  ABOVE it, today. ⚠️ `700` is also the value the spec proposed as its *synthetic* number, so a
 *  test written to the letter of the spec would have been drawn from live data after all — the
 *  exact failure (5b) exists to prevent, one level up from itself. ⭐ Nothing is broken by the
 *  finding: `Lightning` is a `FOG-§7` **row 3** directed-spell reticle and hands over no vision
 *  query, so it is exempt STRUCTURALLY. It does mean the ruling 🧑 `J-F9` covers a CLASS of
 *  surfaces (line sweep −32.3%, Lightning −12.9%), not one spell. Test 3 pins all of it.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero asset loads, ⛔ zero writes.
 *  Two lanes only, the house pattern (`SiegeAcquisitionFunnelTest.cpp:35-49`):
 *    (a) DIRECT CALLS into `FSiegeFogStatics`' pure entry points — real execution, no world;
 *    (b) SOURCE-TEXT structural probes with comment lines skipped (`CountOccurrencesInCode`).
 *
 *  ⚠️ WHY (b) AND NOT A SPAWNED FIXTURE, stated so the gap is honest: there is not one
 *  `UWorld::CreateWorld` and not one `SpawnActor` anywhere in `Siegebound/Tests/` (the house rule,
 *  `SiegeLadderClimbTest.cpp:39`). ⛔ **Green here is NOT "fog works".** It is "the ceiling is
 *  wired to exactly five lanes and to no others". The live behaviour needs `TASK-850`'s PIE pass.
 *
 *  ⚠️⚠️ AND THE HONEST HEADLINE, said here rather than left for a reader to discover: **fog does
 *  not exist at runtime yet.** `FSiegeCombatStatics::ReadFogState` — the one fog-state seam —
 *  returns `false` until `TASK-839` lands `AFogVolume`, so the cut can never fire and the
 *  acquisition surface is byte-for-byte the game that shipped. Test 8 pins that as a DECISION so
 *  it cannot be mistaken for coverage, and names the row `TASK-839` must INVERT.
 */

namespace SiegeFogClampFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files the wiring claims are about. */
	const TCHAR* CombatStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp");
	const TCHAR* CombatStaticsHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h");
	const TCHAR* FogStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.cpp");
	const TCHAR* FogStaticsHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.h");
	const TCHAR* SummonedUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp");
	const TCHAR* TowerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/Tower.cpp");
	const TCHAR* HeroCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp");
	const TCHAR* SpellLibraryCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp");
	const TCHAR* SpellLineSweepCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLineSweep.cpp");
	const TCHAR* SpellLineSweepHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLineSweep.h");
	const TCHAR* CheatManagerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCheatManager.cpp");
	const TCHAR* CardsCsv = TEXT("Docs/Data/cards.csv");

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
	 *  `SiegeAcquisitionFunnelTest.cpp` so the two agree character for character.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL: `SiegeCombatStatics.h` and `SiegeFogStatics.h`
	 *  NAME every symbol asserted below, repeatedly, in the paragraphs that explain the law. A
	 *  scanner that counted comments would force those files to choose between explaining the
	 *  rule and passing it — and the explanation would lose.
	 *  ⚠️ DECLARED LIMITATION, inherited and restated: a comment TRAILING a line of code IS still
	 *  scanned. Every probe below is a whole-line construct or a statement.
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
	 *  Extracts one function body by signature, ending at the first column-0 closing brace —
	 *  the house helper. ⛔ Deliberately NOT a parser: a signature that stops matching FAILS
	 *  rather than silently scanning nothing, which is the whole point under `SC-§38` (a probe
	 *  pinned to a stale coordinate must go RED, never quietly green).
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
	 *  Counts Needle across ⛔ SHIPPING source only (automation tests excluded), and reports WHERE
	 *  — a bare number tells a future reader nothing about which file grew a second consumer.
	 *  Returns -1 and errors if the scan itself is dead, so a broken instrument cannot read as a
	 *  clean zero (`SC-§40`: a claim of ABSENCE must be paired with proof the scanner was alive).
	 */
	static int32 CountAcrossShippingSource(FAutomationTestBase& Test, const TCHAR* Needle, FString& OutWhere)
	{
		TArray<FString> SourceFiles;
		FindAllSourceFiles(SourceFiles);

		if (SourceFiles.Num() < 20)
		{
			Test.AddError(FString::Printf(
				TEXT("SELF-CHECK FAILED: the recursive scan of Source/ found only %d file(s). The instrument is dead ")
				TEXT("and every count taken with it would be a meaningless zero."), SourceFiles.Num()));
			return -1;
		}

		int32 Total = 0;
		OutWhere.Reset();
		for (const FString& File : SourceFiles)
		{
			if (IsAutomationTestFile(File))
			{
				continue;
			}

			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *File))
			{
				continue;
			}

			const int32 Hits = CountOccurrencesInCode(Text, Needle);
			if (Hits > 0)
			{
				Total += Hits;
				OutWhere += FString::Printf(TEXT(" [%s ×%d]"), *FPaths::GetCleanFilename(File), Hits);
			}
		}
		return Total;
	}

	/** The shipped tuning — a default-constructed band IS the band the game runs with. */
	static FSiegeFogTuning ShippedTuning()
	{
		return FSiegeFogTuning();
	}

	/** Exact-equality tolerance, for the claims whose whole content is the word BIT-IDENTICAL. */
	constexpr float Exact = 0.f;

	/**
	 *  ⛔⛔ SYNTHETIC, ⛔ NOT SHIPPED — and it is ⛔ DERIVED FROM THE CEILING rather than typed,
	 *  which is the whole point (`TASK-838(5b)`, `SC-§37`): a test drawn from today's data cannot
	 *  discriminate a correct exemption from a broken one.
	 *
	 *  ⚠️⚠️ AND THIS CONSTANT IS ⛔ NOT `700`, WHICH IS THE NUMBER THE SPEC PROPOSED. ⛔ MEASURED
	 *  2026-09-03 while writing this file: **`Lightning` ships `AoERadius = 700` in
	 *  `Docs/Data/cards.csv`** — so `700` is a ⛔ LIVE SHIPPED VALUE, not a synthetic one, and a
	 *  "synthetic" test written at 700 would have been exactly the failure (5b) exists to prevent.
	 *  ⇒ deriving it from the ceiling makes the collision ⛔ structurally impossible, and test 3
	 *  additionally asserts this value appears NOWHERE in the shipped AoE column.
	 */
	static float SyntheticRadiusAboveCeilingUU()
	{
		// x2 the ceiling: unambiguously above it, unambiguously not a design number, and it moves
		// with the tunable so a ceiling retune can never make it stop discriminating.
		return ShippedTuning().FogVisionCeilingUU * 2.f;
	}

	/**
	 *  ⛔ NOT synthetic — `Lightning`'s SHIPPED `AoERadius`, used as its reticle radius in
	 *  `ResolveTopTargetsDamage` (`FOG-§7` row 3, DIRECTED SPELL GEOMETRY). ⭐ It ⛔ ALREADY
	 *  EXCEEDS the 609.6 ceiling, which makes it a ⛔ SECOND live member of `J-F9`'s class beside
	 *  the hero line spell — see test 3(d).
	 */
	constexpr float ShippedLightningReticleUU = 700.f;

	/**
	 *  ⛔ NOT synthetic — `ASpellLineSweep::LineRange`, read off the shipped header and asserted
	 *  against it in test 2. It ALREADY exceeds the ceiling, which is what makes `J-F9` a live
	 *  ruling rather than a hypothetical.
	 */
	constexpr float ShippedLineSweepRangeUU = 900.f;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE WIRING, AS A CENSUS: EXACTLY FIVE LANES HAND OVER AN ACT OF
//  SEEING, AND THE OTHER FOUR HAND OVER NOTHING. This is the test that catches a
//  SIXTH site being clamped (a nerf nobody asked for) or a FIFTH going missing.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampFiveVisionSitesTest,
	"Siegebound.Fog.ExactlyTheFiveVisionSitesHandOverAVisionQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampFiveVisionSitesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	struct FLaneExpectation
	{
		const TCHAR* RelativePath;
		const TCHAR* Label;
		/** `FOG-§7` row this file's gathers belong to, for the failure message. */
		const TCHAR* Row;
		int32 ExpectedVisionQueries;
		/** A construct that MUST still be present, so a zero beside it is a real read. */
		const TCHAR* LiveControlToken;
	};

	// ⚠️ READ THE ZEROS, THEY ARE THE FEATURE. `FOG-§7`'s taxonomy is that ONE funnel serves
	// FOUR different questions, and only the first is an act of seeing.
	const FLaneExpectation Lanes[] =
	{
		{ SummonedUnitCpp,   TEXT("ASummonedUnit (AcquireTarget + AcquireEnemyNearPoint)"),
		  TEXT("row 1 VISION"), 2, TEXT("FSiegeCombatStatics::GatherHostileAgents(") },
		{ TowerCpp,          TEXT("ATower (AcquireTarget + FireChainZapAt)"),
		  TEXT("row 1 VISION"), 2, TEXT("FSiegeCombatStatics::GatherHostileAgents(") },
		{ HeroCpp,           TEXT("AHeroCharacter::DoMeleeAttack"),
		  TEXT("row 1 VISION"), 1, TEXT("FSiegeCombatStatics::GatherHostileAgents(") },
		{ SpellLibraryCpp,   TEXT("USpellLibrary (Freeze + Lightning + BattleCry)"),
		  TEXT("rows 3 and 5 — AIMED geometry and a FRIENDLY buff"), 0, TEXT("FSiegeCombatStatics::Gather") },
		{ SpellLineSweepCpp, TEXT("ASpellLineSweep::ApplyLineEffectUpTo"),
		  TEXT("row 3 — the hero AIMS a line, he does not acquire along it (J-F9)"), 0, TEXT("FSiegeCombatStatics::GatherHostileAgents(") },
		{ CheatManagerCpp,   TEXT("USiegeCheatManager::FindNearestEnemy"),
		  TEXT("row 4 — the cheat lane, ROUTED but never clamped"), 0, TEXT("FSiegeCombatStatics::GatherHostileAgents(") },
		{ CombatStaticsCpp,  TEXT("FSiegeCombatStatics::ApplyRadialDamage (every AoE in the game)"),
		  TEXT("row 2 BLAST — a blast is not an act of seeing (WITCH-§2, J-W2)"), 0, TEXT("GatherHostileAgents(World, Team, HostileAgents, ") },
	};

	int32 TotalVisionQueries = 0;

	for (const FLaneExpectation& Lane : Lanes)
	{
		FString Text;
		if (!LoadProjectFile(*this, Lane.RelativePath, Text))
		{
			continue;
		}

		// ── POSITIVE CONTROL: this really is an acquisition file and the scanner really read it.
		TestTrue(
			FString::Printf(
				TEXT("SELF-CHECK: %s still routes through the funnel, so the count asserted beside it is a claim ")
				TEXT("about a real acquisition file rather than an empty read."),
				Lane.Label),
			CountOccurrencesInCode(Text, Lane.LiveControlToken) > 0);

		// `SeeingFrom(` is NOT a substring of `SeeingFromUnbounded(` (the next character differs),
		// so the two constructors are counted independently and summed deliberately.
		const int32 Bounded = CountOccurrencesInCode(Text, TEXT("FSiegeVisionQuery::SeeingFrom("));
		const int32 Unbounded = CountOccurrencesInCode(Text, TEXT("FSiegeVisionQuery::SeeingFromUnbounded("));
		const int32 Queries = Bounded + Unbounded;

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ %s hands over %d vision quer(ies) — it is %s. ⛔ An INCREASE here is a lane being ")
				TEXT("silently clamped (a live nerf under a fog card, which is exactly what J-F9 exists to stop); ")
				TEXT("a DROP is a lane going blind-proof, i.e. an acquisition that keeps its full range under fog ")
				TEXT("while every sibling is cut. Either way: read FOG-§7's table before changing this number."),
				Lane.Label, Lane.ExpectedVisionQueries, Lane.Row),
			Queries, Lane.ExpectedVisionQueries);

		TotalVisionQueries += Queries;
	}

	TestEqual(
		TEXT("⭐⭐ FIVE vision queries across the whole acquisition surface — FOG-§7 row 1's exact site list: ")
		TEXT("ASummonedUnit ×2 (target select + the commanded-zone retarget), ATower ×2 (acquire + the chain's ")
		TEXT("fire-time snapshot) and AHeroCharacter ×1. ⛔ The other four rows of that table hand over NOTHING, ")
		TEXT("which is why they CANNOT be clamped rather than merely being unclamped today."),
		TotalVisionQueries, 5);

	// ── AND THE SAME FIVE, COUNTED TREE-WIDE, so a sixth in a file nobody thought to list is
	//    caught too. The two factory definitions live in the header as `SeeingFrom(` /
	//    `SeeingFromUnbounded(` WITHOUT the class prefix, so they are not counted here.
	FString Where;
	const int32 TreeWide = CountAcrossShippingSource(*this, TEXT("FSiegeVisionQuery::Seeing"), Where);
	if (TreeWide < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ FIVE vision queries in ALL of shipping source — not five in the files this test happened to ")
			TEXT("list. ⛔ A SIXTH is a new lane that has quietly become fog-dependent; route it deliberately and ")
			TEXT("amend FOG-§7's table in the same commit. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		TreeWide, 5);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐⭐ — 🧑 `J-F9`: THE HERO'S LINE SPELL KEEPS ITS 900 UNDER FOG.
//  ⛔ THE SINGLE MOST OVERRULABLE ROW IN THIS TASK, AND THE ONLY ONE WHOSE
//  NUMBERS COME FROM LIVE SHIPPED CODE RATHER THAN FROM A DESIGN TABLE.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampHeroLineSpellIsNotClampedTest,
	"Siegebound.Fog.TheHeroLineSpellKeepsItsFullRangeUnderFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampHeroLineSpellIsNotClampedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// ── (a) THE PREMISE IS RE-MEASURED, NOT ASSUMED. If somebody retunes LineRange the whole
	//    ruling needs re-reading, so this row goes red and says so rather than silently
	//    continuing to defend a number that no longer exists (`SC-§38`: the SYMBOL is the key).
	FString SweepHeader;
	if (LoadProjectFile(*this, SpellLineSweepHeader, SweepHeader))
	{
		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ `ASpellLineSweep::LineRange` is still `%.1ff`. ⛔ THIS IS THE WHOLE PREMISE OF J-F9: it ")
				TEXT("ALREADY exceeds the 609.6 ceiling, so a blind clamp inside the funnel would be a LIVE nerf, ")
				TEXT("not a hypothetical. If this row is red, LineRange moved — re-read J-F9 before touching ")
				TEXT("anything else, because the ruling's reasoning is attached to this number."),
				ShippedLineSweepRangeUU),
			CountOccurrencesInCode(SweepHeader, TEXT("LineRange = 900.f")), 1);
	}

	TestTrue(
		TEXT("⭐ …and it really is above the ceiling. A ceiling retune that rose past 900 would make J-F9 moot ")
		TEXT("and this whole test vacuous, so the relation is asserted rather than assumed."),
		ShippedLineSweepRangeUU > Tuning.FogVisionCeilingUU);

	// ── (b) THE COUNTERFACTUAL, IN NUMBERS: what a blind clamp WOULD have cost. This is the row
	//    that makes the exemption load-bearing instead of decorative — it proves the ceiling is
	//    perfectly capable of biting this range, and that only the WIRING spares it.
	const float WouldBeClampedTo = FSiegeFogStatics::EffectiveVisionRadius(ShippedLineSweepRangeUU, /*bFogActive=*/ true, Tuning);
	TestEqual(
		TEXT("⭐⭐ A blind clamp WOULD cut the hero line spell to the ceiling. ⛔ This row is the counterfactual, ")
		TEXT("and it is here so nobody reads the exemption as inert: the arithmetic bites — it is the wiring that ")
		TEXT("spares the spell."),
		WouldBeClampedTo, Tuning.FogVisionCeilingUU, Exact);

	const float CutFraction = 1.f - (WouldBeClampedTo / ShippedLineSweepRangeUU);
	TestEqual(
		TEXT("⭐⭐ …and the cut it would inflict is 32.3% — an unrequested nerf to a card Jonathan has already ")
		TEXT("played, smuggled in under a fog card. ⚖️ His words were \"ranged UNITS will not be able to fire ")
		TEXT("beyond this range\"; a HERO SPELL IS NOT A UNIT, so his sentence does not answer this, and the ")
		TEXT("proceeding default (J-F9) is NOT CLAMPED."),
		CutFraction, 0.323f, 0.001f);

	// ── (c) AND THE WIRING THAT DELIVERS THE RULING. ⛔ The sweep hands over no act of seeing, so
	//    there is no radius at the funnel to clamp it with — it is exempt STRUCTURALLY.
	FString SweepCpp;
	if (LoadProjectFile(*this, SpellLineSweepCpp, SweepCpp))
	{
		TestTrue(
			TEXT("SELF-CHECK: SpellLineSweep.cpp still routes through the funnel, so the zeros below are claims ")
			TEXT("about a real acquisition file."),
			CountOccurrencesInCode(SweepCpp, TEXT("FSiegeCombatStatics::GatherHostileAgents(")) > 0);

		TestEqual(
			TEXT("⭐⭐ THE SWEEP HANDS OVER NO VISION QUERY. ⛔ If this is nonzero, the hero's line spell has been ")
			TEXT("clamped to 609.6 and J-F9 has been overruled IN CODE rather than by Jonathan. ⭐ If he DOES flip ")
			TEXT("it, the flip is exactly one line here and this row inverts to 1 — say so in the handoff."),
			CountOccurrencesInCode(SweepCpp, TEXT("FSiegeVisionQuery")), 0);
	}

	// ── (d) THE RESIDUAL, ASSERTED AS A DECISION so it can never be mistaken for an oversight.
	//    Under fog the hero can hit something at 900 uu that he literally cannot see. That is the
	//    cost of the default, it is stated in the handoff unsoftened, and it is what could flip him.
	TestTrue(
		TEXT("⚠️⚠️ DECLARED RESIDUAL, recorded as an assertion rather than as prose: under fog the hero's line ")
		TEXT("spell reaches 900 uu while his EYES reach 609.6, so he can hit something he cannot see. ⛔ This is ")
		TEXT("the known cost of the J-F9 default and it is deliberate — reversible on one word."),
		ShippedLineSweepRangeUU > Tuning.FogVisionCeilingUU);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐⭐ — THE BLAST EXEMPTION, ASSERTED AT A **SYNTHETIC** 700.
//  ⛔ A test drawn from today's AoE data cannot discriminate a correct exemption
//  from a broken one, because every shipped radius is already under the ceiling.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampBlastExemptionTest,
	"Siegebound.Fog.TheBlastLaneIsExemptAndTheExemptionIsStructural",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampBlastExemptionTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// ── (a) THE SYNTHETIC RADIUS DISCRIMINATES, AND THE SHIPPED ONES DO NOT. Asserted in that
	//    order so the REASON for the synthetic value is in the suite rather than only in a comment.
	const float SyntheticRadius = SyntheticRadiusAboveCeilingUU();

	TestTrue(
		TEXT("SELF-CHECK: the synthetic blast radius really is ABOVE the ceiling. ⛔ A synthetic value chosen ")
		TEXT("below it would make every row underneath pass exactly as vacuously as a shipped one."),
		SyntheticRadius > Tuning.FogVisionCeilingUU);

	const float SyntheticIfClamped = FSiegeFogStatics::EffectiveVisionRadius(SyntheticRadius, /*bFogActive=*/ true, Tuning);
	TestEqual(
		TEXT("⭐⭐ A 700-uu blast WOULD be cut to the ceiling if it were routed through the vision clamp. ⛔ This ")
		TEXT("is the counterfactual that makes the exemption meaningful: the ceiling bites this radius, so a ")
		TEXT("blast surviving it is a WIRING fact, not an arithmetic accident."),
		SyntheticIfClamped, Tuning.FogVisionCeilingUU, Exact);

	// ── (b) ⛔ THE EXEMPTION IS **STRUCTURAL**, NOT MERELY CURRENTLY-INERT (`TASK-838(5c)`).
	//    ApplyRadialDamage does not call the fog at all and hands over no vision query, so the day
	//    someone ships a 700-uu AoE it does NOT silently become a fog-dependent card.
	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	FString BlastBody;
	if (ExtractFunctionBody(*this, CombatCpp, TEXT("void FSiegeCombatStatics::ApplyRadialDamage("), BlastBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted ApplyRadialDamage body is substantial — an empty extraction would make ")
			TEXT("every assertion below pass vacuously."),
			BlastBody.Len() > 400);

		TestEqual(
			TEXT("⭐⭐ THE BLAST HANDS OVER NO VISION QUERY. ⚖️ WITCH-§2 / J-W2: a blast is not an act of seeing — ")
			TEXT("and FOG-§7 row 2 inherits that reasoning rather than re-litigating it. ⛔ If this is nonzero, ")
			TEXT("EVERY AoE in the game (Sapper suicide, Bomb Tower, Fireball, every blast projectile) is now ")
			TEXT("fog-dependent, and NOT ONE shipped radius test would go red, because they are all under the ceiling."),
			CountOccurrencesInCode(BlastBody, TEXT("FSiegeVisionQuery")), 0);

		TestEqual(
			TEXT("⭐ …and it never reaches for the fog rule by hand either. The exemption is the ABSENCE of an act ")
			TEXT("of seeing, not a special case written into the blast."),
			CountOccurrencesInCode(BlastBody, TEXT("FSiegeFogStatics")), 0);

		TestEqual(
			TEXT("⭐ …and its own blast radius gate is untouched — `Distance > Radius` is still what bounds an ")
			TEXT("explosion, exactly as it was before fog existed."),
			CountOccurrencesInCode(BlastBody, TEXT("Distance > Radius")), 1);
	}

	// ── (c) ⭐⭐ THE TRIPWIRE (`TASK-838(5c)`), AND IT SHIPS CARRYING A **MEASURED CORRECTION** TO
	//    THE LAW IT WAS WRITTEN FROM.
	//
	//    ⛔⛔ `FOG-§7` and `TASK-838(5b)` both state that *"every AoE radius in the game is UNDER
	//    the ceiling — Sapper 250, BombTower 250, Wizard 250, Fireball 300, FrostNova 350,
	//    BattleCry 400"*. ⛔ **THAT ENUMERATION IS INCOMPLETE AND THE CLAIM IS FALSE.** Measured
	//    from `Docs/Data/cards.csv` while writing this file: **`Lightning` ships `AoERadius = 700`**,
	//    which is 90.4 uu ABOVE the 609.6 ceiling — TODAY, in shipped data.
	//
	//    ⚖️ ⛔ NOTHING IS BROKEN BY IT, and that is precisely because row 2 and row 3 are
	//    STRUCTURAL rather than merely inert: `Lightning` is `ResolveTopTargetsDamage`, a `FOG-§7`
	//    ROW 3 directed-spell gather, and it hands over no vision query at all. ⭐ The finding is
	//    that the "inert by coincidence of current data" reasoning was ⛔ never even true — which
	//    makes the structural exemption more load-bearing than the law claimed, not less.
	//
	//    ⇒ this row therefore asserts the TRUE state and is a row somebody must UPDATE
	//    DELIBERATELY, ⛔ not a defect row. It is the only outcome (5c) was written to prevent:
	//    a silent change to this number.
	FString Csv;
	if (LoadProjectFile(*this, CardsCsv, Csv))
	{
		TArray<FString> Lines;
		Csv.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ true);

		if (Lines.Num() < 10)
		{
			AddError(FString::Printf(TEXT("SELF-CHECK FAILED: cards.csv parsed to %d line(s) — the probe is dead."), Lines.Num()));
			return false;
		}

		TArray<FString> HeaderFields;
		Lines[0].ParseIntoArray(HeaderFields, TEXT(","), /*InCullEmpty=*/ false);
		const int32 AoEColumn = HeaderFields.IndexOfByKey(FString(TEXT("AoERadius")));

		if (AoEColumn == INDEX_NONE)
		{
			AddError(TEXT("SELF-CHECK FAILED: no `AoERadius` column in cards.csv — the probe is stale, so it FAILS."));
			return false;
		}

		float LargestAoE = 0.f;
		FString LargestCard;
		int32 RowsRead = 0;
		int32 RowsAboveCeiling = 0;
		bool bSyntheticValueCollides = false;
		for (int32 Index = 1; Index < Lines.Num(); ++Index)
		{
			TArray<FString> Fields;
			Lines[Index].ParseIntoArray(Fields, TEXT(","), /*InCullEmpty=*/ false);

			// ⛔ A row whose field count disagrees with the header carries an embedded comma (the
			// free-text Notes column) and would mis-index. Skipped rather than guessed — a probe
			// that guessed would report a confident wrong number.
			if (Fields.Num() != HeaderFields.Num())
			{
				continue;
			}

			++RowsRead;
			const float Radius = FCString::Atof(*Fields[AoEColumn]);
			if (Radius > LargestAoE)
			{
				LargestAoE = Radius;
				LargestCard = Fields.IsValidIndex(0) ? Fields[0] : FString(TEXT("?"));
			}
			if (Radius > Tuning.FogVisionCeilingUU)
			{
				++RowsAboveCeiling;
			}
			if (FMath::IsNearlyEqual(Radius, SyntheticRadius, 0.01f))
			{
				bSyntheticValueCollides = true;
			}
		}

		TestTrue(
			FString::Printf(TEXT("SELF-CHECK: %d cards.csv rows aligned with the header and were read."), RowsRead),
			RowsRead >= 10);

		// ⭐⭐ THE TEST PROVES ITS OWN SYNTHETIC VALUE IS SYNTHETIC. ⛔ This row exists because the
		// spec's proposed "synthetic 700" turned out to BE a shipped number (`Lightning`), which is
		// exactly the class of mistake (5b) was written to prevent — one level up from itself.
		TestFalse(
			FString::Printf(
				TEXT("⭐⭐ The synthetic radius (%.1f uu, derived as 2× the ceiling) appears NOWHERE in the shipped ")
				TEXT("AoE column. ⛔ A 'synthetic' value that is actually a design number cannot discriminate a ")
				TEXT("correct exemption from a broken one — which is what happened to the spec's proposed 700."),
				SyntheticRadius),
			bSyntheticValueCollides);

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ TRIPWIRE (FOG-§7 rows 2/3, TASK-838(5c)): the largest shipped AoE radius is %.0f uu ")
				TEXT("('%s'), and %d shipped radi(us/i) EXCEED the %.1f ceiling TODAY. ⛔ THIS GOING RED IS NOT A ")
				TEXT("BUG — it means the AoE data moved. ⭐ Nothing breaks either way, and that is the POINT: the ")
				TEXT("blast and directed-spell exemptions are STRUCTURAL (neither hands over a vision query, ")
				TEXT("asserted above), so a card crossing the ceiling does NOT silently become fog-dependent. ")
				TEXT("⚠️ Re-read FOG-§7 rows 2 and 3, confirm the new radius is still meant to be unclamped, then ")
				TEXT("update this row deliberately."),
				LargestAoE, *LargestCard, RowsAboveCeiling, Tuning.FogVisionCeilingUU),
			RowsAboveCeiling, 1);

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ …and it is `Lightning` at %.0f uu — the ⛔ MEASURED COUNTEREXAMPLE to FOG-§7's and ")
				TEXT("TASK-838(5b)'s claim that *every* AoE radius is under the ceiling. That enumeration listed ")
				TEXT("six cards and missed this one. ⚖️ `Lightning` is `ResolveTopTargetsDamage` — a FOG-§7 ROW 3 ")
				TEXT("directed-spell reticle, ⛔ not a row 2 blast — so it is exempt for the same structural reason ")
				TEXT("as the hero line spell, and a blind clamp would have cut it by 12.9%% as a SECOND live nerf ")
				TEXT("beside J-F9's 32.3%%. ⛔ Manager amendment, ⛔ not a code fix."),
				ShippedLightningReticleUU),
			LargestAoE, ShippedLightningReticleUU, 0.5f);
	}

	// ── (d) ⭐⭐ THE SECOND LIVE MEMBER OF `J-F9`'s CLASS, ASSERTED IN NUMBERS. `Lightning`'s
	//    reticle already exceeds the ceiling, so the line spell was never the only surface a blind
	//    clamp would have nerfed — it was merely the only one anybody had measured.
	const float LightningIfClamped = FSiegeFogStatics::EffectiveVisionRadius(ShippedLightningReticleUU, /*bFogActive=*/ true, Tuning);
	TestEqual(
		TEXT("⭐⭐ A blind clamp WOULD have cut Lightning's 700-uu reticle to the ceiling as well. ⛔ Asserted so ")
		TEXT("the J-F9 ruling is understood to cover a CLASS of surfaces (FOG-§7 row 3) and not one spell: the ")
		TEXT("hero AIMS both of these, he does not acquire along them."),
		LightningIfClamped, Tuning.FogVisionCeilingUU, Exact);

	const float LightningCut = 1.f - (LightningIfClamped / ShippedLightningReticleUU);
	TestEqual(
		TEXT("⭐ …by 12.9%. ⛔ Small enough that nobody would have noticed it in a playtest and large enough to ")
		TEXT("change which three targets a 3-target strike picks — which is the worst size for an unrequested nerf."),
		LightningCut, 0.129f, 0.001f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — THE CLAMP LIVES AT EXACTLY ONE PLACE, AND IT IS INSIDE THE
//  FUNNEL. `FOG-§6`: "applied INSIDE GatherHostileAgents — never re-implemented
//  per site." A second consumer is a second guard point.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampLivesOnlyInTheFunnelTest,
	"Siegebound.Fog.TheCeilingIsAppliedInsideTheFunnelAndNowhereElse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampLivesOnlyInTheFunnelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	// ── (a) THE LEAF-GREP GATE, the `WITCH-§1` idiom applied to fog: the rule has exactly ONE
	//    consumer in shipping source, so there is exactly one place it can be forgotten.
	FString Where;
	const int32 QualifiedHits = CountAcrossShippingSource(*this, TEXT("FSiegeFogStatics::EffectiveVisionRadius("), Where);
	if (QualifiedHits < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ THE CEILING IS NAMED EXACTLY TWICE IN SHIPPING SOURCE, AND ONLY ONE OF THOSE IS A CALL: its ")
			TEXT("own DEFINITION in SiegeFogStatics.cpp, and the ONE call inside the funnel. (`IsVisibleThroughFog` ")
			TEXT("reaches it UNQUALIFIED from inside the same class, so it is deliberately not counted here — the ")
			TEXT("qualified needle counts EXTERNAL consumers, which is the population this gate is about.) ")
			TEXT("⛔ A THIRD hit is a per-site clamp, which is the forgotten-guard-point failure the funnel exists ")
			TEXT("to prevent and an automatic QA FAIL. ⛔ Do NOT fix a red here by clamping at a new site: route ")
			TEXT("the site through the funnel with a vision query. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		QualifiedHits, 2);

	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	// ── (b) AND THE ONE CALL IS INSIDE `GatherHostileAgents`, not merely somewhere in its file.
	FString FunnelBody;
	if (!ExtractFunctionBody(*this, CombatCpp, TEXT("void FSiegeCombatStatics::GatherHostileAgents("), FunnelBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted GatherHostileAgents body is substantial — an empty extraction would make ")
		TEXT("every assertion below pass vacuously."),
		FunnelBody.Len() > 400);

	TestEqual(
		TEXT("⭐⭐ The funnel applies the vision ceiling exactly once. ⛔ If this is ZERO, fog does nothing to any ")
		TEXT("unit, tower or hero in the game and the card is inert — while every arithmetic row in ")
		TEXT("SiegeFogTest.cpp stays green, because the rules would still be correct and unused."),
		CountOccurrencesInCode(FunnelBody, TEXT("FSiegeFogStatics::EffectiveVisionRadius(")), 1);

	TestEqual(
		TEXT("⭐ …and the per-candidate comparison is the SHIPPED predicate, not a hand-rolled `<=`. ")
		TEXT("`IsVisibleThroughFog` owns the INCLUSIVE boundary (FOG-§7a, measured from the project's own ")
		TEXT("acquisition idiom), so the clamp substitutes the OPERAND of that comparison and never its SHAPE. ")
		TEXT("Re-authoring it here would put a second, silently divergent copy of the boundary rule in the tree."),
		CountOccurrencesInCode(FunnelBody, TEXT("FSiegeFogStatics::IsVisibleThroughFog(")), 1);

	// ── (c) ⛔⛔ THE SEAM IS CALLED **UNCONDITIONALLY**. `EffectiveVisionRadius` returns the request
	//    BIT-IDENTICALLY with fog off, which is the entire property that keeps a fog regression
	//    attributable to fog — re-wrapping it in a hand-written state check re-introduces the branch
	//    it was built to delete, AND is weaker, because it sails past every degenerate tuning.
	TestEqual(
		TEXT("⭐⭐ The funnel contains NO hand-written `if (bFogActive)`. The seam is called unconditionally and ")
		TEXT("the no-op decision is taken on its OUTPUT (\"did the fog actually shorten the request?\"), which is a ")
		TEXT("STRICTLY STRONGER test: it is also correct for a NaN, negative or zero ceiling, where a state check ")
		TEXT("would run the cut with a broken band."),
		CountOccurrencesInCode(FunnelBody, TEXT("if (bFogActive")), 0);

	TestEqual(
		TEXT("⭐ …and it still consults the veil exactly once, in the same body. The two cards ride ONE piece of ")
		TEXT("engineering (FOG-§4(a)) and neither may be folded into the other: fog clamps HOW FAR the query ")
		TEXT("reaches; the veil filters WHICH ACTORS come back."),
		CountOccurrencesInCode(FunnelBody, TEXT("IsAgentVisibleTo(")), 1);

	TestEqual(
		TEXT("⛔ …and the funnel does NOT sort. Callers tie-break by strict improvement, so enumeration order ")
		TEXT("decides every tie; a Sort introduced by the fog filter would re-pick targets at five sites without ")
		TEXT("changing a single result SET."),
		CountOccurrencesInCode(FunnelBody, TEXT("Sort(")), 0);

	// ── (d) THE FRIENDLY LANE IS NEVER FOGGED (`FOG-§7` row 5). A player who cannot buff, heal or
	//    order his own units because the weather rolled in would call that a bug, and he would be right.
	FString FriendlyBody;
	if (ExtractFunctionBody(*this, CombatCpp, TEXT("void FSiegeCombatStatics::GatherFriendlyAgents("), FriendlyBody))
	{
		TestEqual(
			TEXT("⭐⭐ The FRIENDLY lane is never fog-clamped, and it takes NO vision parameter at all — a lane with ")
			TEXT("no parameter cannot be given one by accident. Battle Cry, the Cleric's heal and every ally-facing ")
			TEXT("query keep their full reach under fog."),
			CountOccurrencesInCode(FriendlyBody, TEXT("FSiegeFogStatics")), 0);
	}

	FString CombatHeader;
	if (LoadProjectFile(*this, CombatStaticsHeader, CombatHeader))
	{
		TestEqual(
			TEXT("⭐ `GatherFriendlyAgents` is still declared with no vision parameter — asserted at the DECLARATION ")
			TEXT("because that is where the impossibility lives, not in the body."),
			CountOccurrencesInCode(CombatHeader, TEXT("GatherFriendlyAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out)")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐ — SYMMETRY IS STRUCTURAL, AND THIS IS THE ONLY HONEST WAY TO
//  ASSERT IT. `FOG-§7a` / `J-F3`: not one function in the fog module takes a
//  team, a viewer, a controller or an actor ⇒ an asymmetric fog is
//  UNREPRESENTABLE. ⛔ A "both teams get the same answer" test is BANNED here —
//  it would be trivially true and would report SAFE forever.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampSymmetryIsUnrepresentableTest,
	"Siegebound.Fog.AnAsymmetricFogIsUnrepresentableBecauseNoFogFunctionTakesATeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampSymmetryIsUnrepresentableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	// ⛔ THE REAL SHIPPED SYMBOL NAMES, read off the landed headers — not plausible-looking
	// guesses. A guard that named a symbol nobody will ever write can never fire.
	const TCHAR* const AsymmetryTokens[] =
	{
		TEXT("ETeamId"),
		TEXT("ITeamAgent"),
		TEXT("AActor"),
		TEXT("AController"),
		TEXT("APlayerController"),
		TEXT("UWorld"),
		TEXT("ViewerTeam"),
	};

	const TCHAR* const FogFiles[] = { FogStaticsHeader, FogStaticsCpp };

	for (const TCHAR* File : FogFiles)
	{
		FString Text;
		if (!LoadProjectFile(*this, File, Text))
		{
			continue;
		}

		// ── POSITIVE CONTROL: the file really was read and really is the fog module.
		TestTrue(
			FString::Printf(
				TEXT("SELF-CHECK: %s really is the fog module (it names `FSiegeFogStatics` on a code line), so the ")
				TEXT("absences below are claims about a real read."),
				*FPaths::GetCleanFilename(FString(File))),
			CountOccurrencesInCode(Text, TEXT("FSiegeFogStatics")) > 0);

		for (const TCHAR* Token : AsymmetryTokens)
		{
			TestEqual(
				FString::Printf(
					TEXT("⭐⭐ %s contains no `%s` on any code line. ⛔ THIS IS THE WHOLE ENFORCEMENT OF J-F3 (\"And ")
					TEXT("that includes AI and all units\"): you cannot write the favouritism, because there is no ")
					TEXT("parameter to branch on. ⛔ Adding one to \"support\" symmetry converts a GUARANTEE into a ")
					TEXT("PROMISE and is an automatic FAIL. ⚠️ Note the deliberate contrast with the invisibility ")
					TEXT("predicate, which DOES take a viewer team — that feature IS asymmetric. The two signatures ")
					TEXT("disagree on purpose, and the disagreement is the design."),
					*FPaths::GetCleanFilename(FString(File)), Token),
				CountOccurrencesInCode(Text, Token), 0);
		}
	}

	// ── AND THE VISION QUERY ITSELF CARRIES NO TEAM EITHER. It is the one new type TASK-838 added
	//    to the fog path, so it is the one place the parameter could have crept back in.
	FString CombatHeader;
	if (LoadProjectFile(*this, CombatStaticsHeader, CombatHeader))
	{
		FString QueryBlock;
		const int32 Start = CombatHeader.Find(TEXT("struct FSiegeVisionQuery"), ESearchCase::CaseSensitive);
		const int32 End = CombatHeader.Find(TEXT("class GITCLAUDEUNREALTEST_API FSiegeCombatStatics"), ESearchCase::CaseSensitive);
		if (Start == INDEX_NONE || End == INDEX_NONE || End <= Start)
		{
			AddError(TEXT("⛔ Could not slice the FSiegeVisionQuery declaration — the probe is stale, so it FAILS."));
		}
		else
		{
			QueryBlock = CombatHeader.Mid(Start, End - Start);

			TestTrue(
				TEXT("SELF-CHECK: the FSiegeVisionQuery slice is substantial and really contains its two named ")
				TEXT("constructors — an empty slice would make the absences below vacuous."),
				CountOccurrencesInCode(QueryBlock, TEXT("SeeingFrom(")) == 1
				&& CountOccurrencesInCode(QueryBlock, TEXT("SeeingFromUnbounded(")) == 1);

			TestEqual(
				TEXT("⭐⭐ The vision query carries NO team. It is two facts and nothing else: where the query looks ")
				TEXT("FROM, and the reach it is asking for. ⛔ A team here would make an asymmetric fog typeable for ")
				TEXT("the first time, at the one seam both armies share."),
				CountOccurrencesInCode(QueryBlock, TEXT("ETeamId")), 0);

			TestEqual(
				TEXT("⭐ …and no actor either — it is a POINT and a RADIUS, so it cannot grow a per-target rule. ")
				TEXT("Fog is per-QUERY; the veil is per-ACTOR. Keeping them different shapes is what stops one card ")
				TEXT("acquiring the other's semantics."),
				CountOccurrencesInCode(QueryBlock, TEXT("AActor")), 0);
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐⭐⭐ — `FOG-§7b`: A CEILING OF **ZERO** MUST NOT STOP THE WAR.
//  BOTH HALVES, AND NEITHER ALONE IS SUFFICIENT.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampZeroCeilingCannotBlindTheArmyTest,
	"Siegebound.Fog.AZeroCeilingReturnsTheRequestAndNeverStopsCombat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampZeroCeilingCannotBlindTheArmyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	// ── HALF (b) — THE LOAD-BEARING ONE, ASSERTED FIRST. `ClampMin` constrains the editor
	//    spinner and NOTHING else: an `.ini`, a Blueprint default or a line of C++ can still
	//    write 0. This is what protects the GAME.
	FSiegeFogTuning ZeroCeiling = ShippedTuning();
	ZeroCeiling.FogVisionCeilingUU = 0.f;

	FSiegeFogTuning NegativeCeiling = ShippedTuning();
	NegativeCeiling.FogVisionCeilingUU = -1.f;

	// The Longbowman's range — the biggest cut in the game, so the loudest possible witness.
	const float LongbowmanRange = 3600.f;

	TestEqual(
		TEXT("⭐⭐⭐ A ceiling of ZERO returns the request BIT-IDENTICALLY. ⛔ Before FOG-§7b the guard was ")
		TEXT("`Ceiling < 0.f` (strict), so zero PASSED it and reached the min() ⇒ `min(Range, 0) == 0` for EVERY ")
		TEXT("acquisition in the game ⇒ ALL COMBAT STOPS WHILE FOG IS UP. ⛔ Reachable by dragging a slider to its ")
		TEXT("stop. This row is that trap, and it must stay red-able forever."),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/ true, ZeroCeiling), LongbowmanRange, Exact);

	TestEqual(
		TEXT("⭐ …EXACTLY as a NEGATIVE ceiling already did. That is the point: zero was the ONE member of the ")
		TEXT("degenerate class that a strict `<` let through, so this is the module's OWN totality law (degenerate ")
		TEXT("⇒ NO FOG, never no vision) applied to the input that was missing from it — not a new rule."),
		FSiegeFogStatics::EffectiveVisionRadius(LongbowmanRange, /*bFogActive=*/ true, NegativeCeiling), LongbowmanRange, Exact);

	TestTrue(
		TEXT("⭐⭐ …and COMBAT ACTUALLY CONTINUES: a Longbowman with a zero ceiling can still engage a target at ")
		TEXT("3000 uu. ⛔ Asserted through the PREDICATE and not only the radius, because \"returns 3600\" and \"can ")
		TEXT("still shoot\" are different claims and it is the second one Jonathan would notice."),
		FSiegeFogStatics::IsVisibleThroughFog(3000.f, LongbowmanRange, /*bFogActive=*/ true, ZeroCeiling));

	TestTrue(
		TEXT("⛔ …and a melee unit is not blinded either — the failure would have hit EVERY unit, not only the ")
		TEXT("ranged ones, which is why it stops the whole war rather than nerfing one card."),
		FSiegeFogStatics::IsVisibleThroughFog(100.f, 120.f, /*bFogActive=*/ true, ZeroCeiling));

	// ── AND THE GUARD'S SOURCE TEXT, because the numeric rows above would also pass if somebody
	//    "fixed" this by special-casing zero somewhere else. The shape is the fix.
	FString FogCpp;
	if (LoadProjectFile(*this, FogStaticsCpp, FogCpp))
	{
		FString Body;
		if (ExtractFunctionBody(*this, FogCpp, TEXT("float FSiegeFogStatics::EffectiveVisionRadius("), Body))
		{
			TestEqual(
				TEXT("⭐⭐ The sanitiser tests `Ceiling <= 0.f`. ⛔ NOT `< 0.f` — FOG-§7b half (b), and this needle is ")
				TEXT("the fix itself rather than a symptom of it."),
				CountOccurrencesInCode(Body, TEXT("Ceiling <= 0.f")), 1);

			TestEqual(
				TEXT("⛔ …and the strict form is GONE. A revert to `Ceiling < 0.f` re-opens the trap in one character."),
				CountOccurrencesInCode(Body, TEXT("Ceiling < 0.f")), 0);
		}
	}

	// ── HALF (a) — THE DESIGNER'S HALF. The ceiling's editor floor is the ONSET's own pinned
	//    value, and it must TRACK it: below the onset the model has no band at all.
	FString FogHeader;
	if (LoadProjectFile(*this, FogStaticsHeader, FogHeader))
	{
		const int32 ClampAtOnset = CountOccurrencesInCode(FogHeader, TEXT("ClampMin = \"304.8\""));

		TestEqual(
			TEXT("⭐⭐ The CEILING's editor floor is `ClampMin = \"304.8\"` — FOG-§7b half (a), and it is the ONSET's ")
			TEXT("OWN pinned value (FOG-§1, 10 ft × 30.48), ⛔ not a new invented number. Below the onset the model ")
			TEXT("has no band at all, so the slider's floor is the tightest ceiling that is still a FOG. ⛔ Documenting ")
			TEXT("`0` as a deliberate \"blind\" capability was REFUSED on the record."),
			ClampAtOnset, 1);

		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ …and it TRACKS the onset: the meta string parses to %.1f, which is the shipped ")
				TEXT("`FogVisionOnsetUU`. ⛔ THIS IS WHY THE ROW EXISTS — UHT meta values cannot reference a C++ ")
				TEXT("constant, so `\"304.8\"` is the SECOND textual occurrence of that number in Source/ and the two ")
				TEXT("can drift apart silently. An onset retune that forgets this line turns THIS row red instead of ")
				TEXT("leaving a slider floor that no longer means anything."),
				ShippedTuning().FogVisionOnsetUU),
			FCString::Atof(TEXT("304.8")), ShippedTuning().FogVisionOnsetUU, 1.e-4f);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 ⭐⭐ — WITH FOG OFF THE GAME IS BYTE-FOR-BYTE THE GAME THAT SHIPPED.
//  This is the property that keeps ANY regression in this batch attributable to
//  fog, and it is asserted at the exact radii the five vision sites hand over.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampIsInertWithFogOffTest,
	"Siegebound.Fog.EveryVisionSiteIsBitIdenticalWithFogOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampIsInertWithFogOffTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();

	// ⛔ These are the numbers the FIVE vision sites actually hand over — the unit's GDD §3.8
	// profile AggroRadius, the tower ranges from cards.csv, the hero's MeleeRange, and the
	// UNBOUNDED sentinel the two sites with no self-range use. ⚠️ NOT FOG-§2's card table:
	// SiegeFogTest.cpp already owns that, and this row is about the WIRING's operands.
	struct FSiteRadius
	{
		const TCHAR* Label;
		float RequestedUU;
	};

	const FSiteRadius SiteRadii[] =
	{
		{ TEXT("ASummonedUnit::AcquireTarget — AggroRadius (GDD §3.8 profile constant)"), 600.f },
		{ TEXT("ATower::AcquireTarget — ArrowTower AttackRange"), 900.f },
		{ TEXT("ATower::AcquireTarget — BallistaTower AttackRange"), 1400.f },
		{ TEXT("ATower::AcquireTarget — BombTower / CrystalTower AttackRange"), 800.f },
		{ TEXT("AHeroCharacter::DoMeleeAttack — MeleeRange"), 150.f },
		{ TEXT("the UNBOUNDED sentinel (AcquireEnemyNearPoint, FireChainZapAt)"), TNumericLimits<float>::Max() },
	};

	for (const FSiteRadius& Site : SiteRadii)
	{
		TestEqual(
			FString::Printf(
				TEXT("⭐⭐ With fog OFF, %s is returned BIT-IDENTICALLY. ⛔ Exact tolerance: not one ulp of drift is ")
				TEXT("allowed, because this is the property that makes a fog regression ATTRIBUTABLE TO FOG. An ")
				TEXT("unconditional clamp, a sanitising pass or a `clamp` in place of `min` all fail here."),
				Site.Label),
			FSiegeFogStatics::EffectiveVisionRadius(Site.RequestedUU, /*bFogActive=*/ false, Tuning),
			Site.RequestedUU, Exact);
	}

	// ── ⭐⭐ AND THE NO-OP GUARD THE FUNNEL RELIES ON, ASSERTED AS THE PROPERTY IT IS: with fog
	//    off the effective radius is never SHORTER than the request, so the funnel's distance cut
	//    provably cannot run and costs zero collision queries per acquisition poll.
	for (const FSiteRadius& Site : SiteRadii)
	{
		TestFalse(
			FString::Printf(
				TEXT("⭐⭐ With fog OFF the cut cannot run for %s — the funnel only removes candidates when the ")
				TEXT("ceiling ACTUALLY SHORTENED the request. ⛔ That guard is on the seam's OUTPUT rather than on ")
				TEXT("the fog's state, which is what makes it correct for a broken tuning too."),
				Site.Label),
			FSiegeFogStatics::EffectiveVisionRadius(Site.RequestedUU, /*bFogActive=*/ false, Tuning) < Site.RequestedUU);
	}

	// ── AND THE ONE SITE WHERE FOG GENUINELY DOES NOTHING EVEN WHEN IT IS UP, said plainly so it
	//    is not later mistaken for a wiring defect: a short reach is passed through, ⛔ never RAISED.
	TestEqual(
		TEXT("⭐ The hero's 150-uu melee is UNCHANGED under fog — `min`, never `clamp`, so a short range is never ")
		TEXT("lengthened to the ceiling. ⛔ Inventing a floor would be adjusting Jonathan's ruling (J-F2) while ")
		TEXT("appearing to honour it, and it would silently BUFF every melee unit in the game."),
		FSiegeFogStatics::EffectiveVisionRadius(150.f, /*bFogActive=*/ true, Tuning), 150.f, Exact);

	TestEqual(
		TEXT("⭐⭐ …and so is the unit AggroRadius of 600, because it already sits INSIDE the 609.6 ceiling. ")
		TEXT("📌 MEASURED FINDING, recorded here rather than buried: FOG-§2's -83.1% Longbowman figure is about the ")
		TEXT("card ROW's Range, which gates FIRING — the unit's ACQUISITION is the GDD §3.8 profile constant 600, ")
		TEXT("so this site is essentially untouched by fog. The tower sites are where the card bites."),
		FSiegeFogStatics::EffectiveVisionRadius(600.f, /*bFogActive=*/ true, Tuning), 600.f, Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 ⭐⭐ — THE FOG-STATE SEAM: ONE READ, AND IT IS **NOT LIVE YET**.
//  ⛔ Written so a green suite cannot be mistaken for a shipped feature, and so
//  `TASK-839` inherits a row it must INVERT rather than delete (the TASK-828 →
//  TASK-829 idiom, which is the one that worked).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogClampStateSeamTest,
	"Siegebound.Fog.TheFogStateIsReadInExactlyOnePlaceAndIsNotLiveUntilTask839",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogClampStateSeamTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogClampFixture;

	// ── (a) ONE READ. `FOG-§6`'s M8 clause: the ceiling derives from ONE replicated "fog is
	//    active until T" scalar, ⛔ never from per-actor visibility. A second read is a second
	//    guard point, and it is also the shape that would let fog leak per-client state.
	FString Where;
	const int32 SeamHits = CountAcrossShippingSource(*this, TEXT("ReadFogState("), Where);
	if (SeamHits < 0)
	{
		return false;
	}
	TestEqual(
		FString::Printf(
			TEXT("⭐⭐ `ReadFogState` is named exactly THREE times in shipping source: its declaration in the ")
			TEXT("header, its definition, and the ONE call inside the funnel. ⛔ A FOURTH is a second fog-state ")
			TEXT("read — which is how a symmetric, world-global card starts answering differently in two places. ")
			TEXT("Found:%s"),
			Where.IsEmpty() ? TEXT(" (nowhere — the needle is broken)") : *Where),
		SeamHits, 3);

	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	FString FunnelBody;
	if (ExtractFunctionBody(*this, CombatCpp, TEXT("void FSiegeCombatStatics::GatherHostileAgents("), FunnelBody))
	{
		TestEqual(
			TEXT("⭐ …and the ONE call is inside the funnel, once per gather — ⛔ not once per candidate, which on a ")
			TEXT("0.25 s acquisition poll over every agent on the field would be a world query per unit pair."),
			CountOccurrencesInCode(FunnelBody, TEXT("ReadFogState(")), 1);
	}

	// ── (b) ⛔⛔ THE HONEST ROW, AND IT IS THE ROW `TASK-839` MUST **INVERT, NOT DELETE**.
	//    `AFogVolume` is TASK-839's and TASK-839 is BLOCKED BY TASK-838, so the wiring lands first
	//    and the state lands second. While the seam returns false the clamp can never fire.
	FString SeamBody;
	if (ExtractFunctionBody(*this, CombatCpp, TEXT("bool FSiegeCombatStatics::ReadFogState("), SeamBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted ReadFogState body is substantial — an empty extraction would make the ")
			TEXT("assertion below pass vacuously."),
			SeamBody.Len() > 200);

		TestEqual(
			TEXT("⚠️⚠️ THE SEAM IS NOT WIRED TO A FOG SOURCE YET, AND THIS ROW SAYS SO OUT LOUD RATHER THAN LETTING ")
			TEXT("A GREEN SUITE IMPLY OTHERWISE: it always answers \"no fog\", so the ceiling never fires and the ")
			TEXT("acquisition surface is byte-for-byte the game that shipped. ⛔ TASK-839 lands `AFogVolume` and ")
			TEXT("REPLACES the seam's final `return false` with the volume read. ⭐ WHEN IT DOES, **INVERT THIS ROW, ")
			TEXT("DO NOT DELETE IT** — it becomes \"the seam really consults AFogVolume\", which is the assertion ")
			TEXT("that would catch a fog card that renders beautifully and blinds nobody."),
			CountOccurrencesInCode(SeamBody, TEXT("return false;")), 2);
	}

	// ── (c) AND THE TUNING IS ALWAYS ASSIGNED, on every path, so "no fog" can never hand back an
	//    uninitialised band into the one function every attack in the game routes through.
	if (!SeamBody.IsEmpty())
	{
		TestEqual(
			TEXT("⭐ The seam assigns OutTuning unconditionally, before any early-out. ⛔ A path that left it ")
			TEXT("untouched would push an uninitialised band into EffectiveVisionRadius, and a garbage ceiling in ")
			TEXT("the funnel is a global combat outage rather than a local glitch."),
			CountOccurrencesInCode(SeamBody, TEXT("OutTuning = FSiegeFogTuning();")), 1);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
