// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/Char.h"                  // FChar::IsWhitespace — named EXPLICITLY rather than leaned on transitively
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/SiegeFogStatics.h" // FSiegeFogStatics::EffectiveVisionRadius + FSiegeFogTuning — the rule behind the ceiling
#include "Siegebound/SummonedUnit.h"    // ASummonedUnit::UnitEngagementRadiusUU — read, never re-typed

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE **WIRING** OF FOG INTO UNIT RETENTION, FIRING AND THE COMMANDED
 *      NOTICE BOUND (TASK-1008; law ⭐⭐⭐ `FOG-§9.11`'s retention clause, ⭐⭐ `FOG-§9.6`,
 *      `FOG-§9.7`/`§9.7a`, `FOG-§9.10a`/`§9.10b`, `FOG-§7`, `SC-§37`, `SC-§38`, `SHIP-§9`) ═══
 *
 *  ⚖️ 🧑 Jonathan, verbatim: *"fog should make units DROP existing targets if they are outside the
 *  609 range"*, *"yes clamp retention under fog"*, and *"no ranged units will be able to fire onto
 *  anything above 20 feet away."*
 *
 *  ⭐⭐ WHAT THIS FILE IS DEFENDING, IN ONE PARAGRAPH. Before this row fog bound ⛔ ACQUISITION
 *  ONLY. A unit that had already acquired kept chasing to its full 8000 leash and firing at its
 *  full card `Range`, through fog it provably could not see. That is not a tuning miss: the
 *  acquisition funnel runs at ⛔ GATHER time, so *"do I STILL hold this target?"* is a question it
 *  ⛔ cannot be asked. ⇒ `TASK-1007` built a REACH seam and this row hands ⛔ NINE unit-side
 *  reaches to it through ⛔ ONE door, `ASummonedUnit::ApplyFogVisionCeilingUU`.
 *
 *  ⛔⛔⛔ THE FAILURE EVERY TEST BELOW IS WRITTEN AGAINST (`SC-§37`, `FOG-§9.7`): **AN
 *  IMPLEMENTATION THAT CLAMPS ONLY ACQUISITION PASSES EVERY TEST WRITTEN FROM THE ORDINARY CASE
 *  WHILE LEAVING ENGAGED UNITS SHOOTING THROUGH FOG.** And its sibling (`FOG-§9.7a`): **an
 *  implementation that routes only the FIRING gate fixes ranged units and leaves the melee charge
 *  fully intact — because `min(120, 609.6) == 120` and no value of the fog term can move a
 *  120-range gate at 1500 uu.** ⇒ ⭐ the RETENTION path is the load-bearing half, and TEST 3
 *  executes the arithmetic that proves the firing gate could never have substituted for it.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero asset loads, ⛔ zero writes.
 *  Two lanes, the house pattern:
 *    (a) DIRECT CALLS into `FSiegeFogStatics`' pure entry points — real execution of the ceiling
 *        arithmetic at ⛔ BOTH values of the fog boolean;
 *    (b) SOURCE-TEXT structural probes with comment lines skipped (`CountOccurrencesInCode`) plus
 *        body extraction by SIGNATURE (`ExtractFunctionBody`) — the only lane that can see WHICH
 *        function a site calls, and the only lane that can see a CALL GRAPH, without a world.
 *
 *  ⚠️⚠️ THE HONEST GAP, STATED SO IT IS NOT DISCOVERED. ⛔ NO UNIT IS EVER TICKED HERE. Nothing
 *  below observes a target actually being dropped; it observes that the ⛔ ONE expression capable
 *  of dropping it is present, at ⛔ both sites, reading the ⛔ right reach through the ⛔ right
 *  door, and that the arithmetic that expression performs returns the numbers 🧑 his ruling names.
 *  ⛔ Green here is *"the retention bound is wired to fog"*, ⛔ NOT *"a unit drops its target in a
 *  running match"*. The second one needs a playtest and is owed to him, not claimed by this file.
 */

namespace SiegeFogRetentionWiringFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the file every structural claim below is about. */
	const TCHAR* SummonedUnitCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp");

	/** ⛔ THE ONE UNIT-SIDE DOOR, by its DEFINITION signature. A stale one FAILS (`SC-§38`). */
	const TCHAR* ChokepointSignature =
		TEXT("float ASummonedUnit::ApplyFogVisionCeilingUU(float RequestedReachUU) const");

	/** The three clamped expressions, spelled exactly as they must appear at a site. */
	const TCHAR* RetentionExpression = TEXT("ApplyFogVisionCeilingUU(GetEffectiveLeashRangeUU())");
	const TCHAR* FiringExpression = TEXT("ApplyFogVisionCeilingUU(AttackRange)");
	const TCHAR* NoticeExpression = TEXT("ApplyFogVisionCeilingUU(GetEngagementRadiusUU())");

	/** Any call to the door at all — the needle the DERIVED total is taken with. */
	const TCHAR* AnyCeilingCall = TEXT("ApplyFogVisionCeilingUU(");

	/** The seam the door forwards to. ⛔ Pinned at ONE in the whole file by funnel test 9. */
	const TCHAR* SeamCall = TEXT("ResolveFogClampedReachUU(");

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
	 *  `SiegeAcquisitionFunnelTest.cpp` / `SiegeFogReachSeamTest.cpp` / `SiegeUnitNoticeRangeTest.cpp`
	 *  so every file that scans this source agrees character for character. ⛔ Do not "improve" it
	 *  here: a divergent counter would make two gates disagree about the same file.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL. `SummonedUnit.cpp` NAMES the ceiling, the leash and
	 *  the seam repeatedly in the paragraphs that explain them; a scanner that counted comments
	 *  would force that file to choose between explaining the law and passing it, and the
	 *  explanation would lose.
	 *  ⚠️ DECLARED LIMITATION, inherited and restated: a comment TRAILING a line of code IS still
	 *  scanned. Every needle below is a call shape carrying an open paren, never a bare token.
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
	 *  ⛔⛔⛔ THE ENGINE DEFECT THAT HUNG THIS FILE, RECORDED WHERE THE NEXT AUTHOR WILL HIT IT.
	 *
	 *  `FString::Find(…, ESearchDir::FromStart, StartPosition)` clamps `StartPosition` to
	 *  ⛔ `Len() - 1`, ⛔ NOT to `Len()` —
	 *  `Engine/Source/Runtime/Core/Private/Containers/String.cpp.inl:475` (UE 5.8):
	 *
	 *      Start += FMath::Clamp(StartPosition, 0, RemainingLength - 1);
	 *
	 *  ⇒ a scan-forward loop whose cursor reaches ⛔ EXACTLY `Len()` does not get `INDEX_NONE`.
	 *  It gets a ⛔ one-character search window over the LAST character and re-finds it. In
	 *  `CountOccurrencesInCode` above, `From = Found + NeedleLength` then recomputes the SAME
	 *  value, and the `for (;;)` ⛔ NEVER TERMINATES. No crash, no failure, no allocation — the
	 *  game thread simply stops returning. (Measured: TASK-1041 loop 1 burned >10 h on one test.)
	 *
	 *  ⭐⭐ THE TRIGGER IS EXACT, AND IT IS WHY THE HOUSE HELPER SURVIVED 566 CALLS: the loop only
	 *  pins when a match ⛔ ENDS AT THE END of a trimmed line, which for a re-find requires the
	 *  needle to be ⛔ EXACTLY ONE CHARACTER equal to that last character. Every other needle in
	 *  this tree is ≥ 2 characters — the fixture doc above says so in its own words ("a call shape
	 *  carrying an open paren, never a bare token") — and a ≥ 2-character needle cannot re-match a
	 *  1-character window. ⛔ `TEXT(";")` was the FIRST single-character needle ever passed, and a
	 *  `;` is by construction the last character of a C++ statement line, so it pinned on contact.
	 *
	 *  ⛔ DO NOT "fix" `CountOccurrencesInCode` here. It is copied VERBATIM into at least four test
	 *  files by the law stated above, and this file may repair only itself — a 3-of-N edit is
	 *  precisely the divergence that law forbids. ⇒ the two helpers below take the character case
	 *  ⛔ OUT of the string-needle helper entirely, and neither one calls `Find` with a
	 *  `StartPosition` at all. A tree-wide sweep is a MANAGER's row, not a smuggled one.
	 */

	/**
	 *  The source with every COMMENT LINE dropped — the same predicate `CountOccurrencesInCode`
	 *  applies, hoisted into a projection so that ⛔ index arithmetic can be comment-aware too.
	 *
	 *  ⭐⭐ THIS IS THE OTHER HALF OF THE FIX, AND IT CLOSES A ⛔ FALSE GREEN. The ordering anchor
	 *  below is a plain `Find`, which is ⛔ COMMENT-BLIND, while the count is comment-AWARE. Run
	 *  both over the same span and a commented-out `// return;` between the dispatch and its real
	 *  return would ⛔ ANCHOR the pin on a line that CANNOT EXECUTE, while the count — which skips
	 *  that line — still read a comfortable 1. ⇒ the exact fall-through this row exists to catch
	 *  could ship green, wearing a comment as camouflage. Projecting FIRST and measuring
	 *  ⛔ ONLY on the projection makes the two passes agree ⛔ BY CONSTRUCTION rather than by care.
	 *
	 *  ⚠️ Line endings are normalised to `\n` on the way out, so the projection is identical
	 *  whether the file on disk is CRLF or LF. No needle here contains a line terminator.
	 */
	static FString CodeLinesOnly(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		FString Out;
		Out.Reserve(Source.Len());
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStart();

			// ⛔ The five clauses are the house predicate, character for character. If the helper
			//    above ever learns a sixth, this MUST learn it in the same commit or the anchor
			//    and the count go back to disagreeing about which lines are real.
			const bool bIsCommentLine =
				Trimmed.StartsWith(TEXT("//"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("* "), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("*/"), ESearchCase::CaseSensitive)
				|| Trimmed.StartsWith(TEXT("/*"), ESearchCase::CaseSensitive)
				|| Trimmed.Equals(TEXT("*"), ESearchCase::CaseSensitive);

			if (!bIsCommentLine)
			{
				Out += Line;
				Out += TEXT("\n");
			}
		}
		return Out;
	}

	/**
	 *  ⭐⭐ THE SECOND PROJECTION STAGE (TASK-1045, from `qa/TASK-1042.md` W-4): the same lines with
	 *  the TAIL a trailing `//` comment leaves on a CODE line removed.
	 *
	 *  ⛔ WHY `CodeLinesOnly` ABOVE IS NOT ENOUGH — and this is the whole reason this exists:
	 *  that projection drops WHOLE COMMENT LINES ⛔ ONLY. A ⛔ TRAILING comment sits on a ⛔ CODE
	 *  line, so the line survives ⛔ VERBATIM, `;` tail and all, and the ownership count in test
	 *  4 (c) reads a comment's punctuation as though it were a statement. ⛔ BOTH directions were
	 *  ⛔ MEASURED on `ASummonedUnit::UpdateState()` before this stage existed (TASK-1045):
	 *    ⛔ FALSE ⛔ RED — `UpdateStateFollow(*FollowGroup); // dispatch; then return` counts ⛔ 2
	 *       on a ⛔ CORRECT tree. Loud — but ⛔ THE CHEAPEST WAY OUT OF A FALSE RED IS TO ⛔ WEAKEN
	 *       THE PIN, which is the failure the whole TASK-1041 sweep exists to prevent.
	 *    ⛔ FALSE ⛔ GREEN — `UpdateStateFollow(*FollowGroup); // return;` with the ⛔ REAL `return;`
	 *       ⛔ DELETED anchors `ReturnIndex` ⛔ INSIDE THE COMMENT and counts a comfortable ⛔ 1.
	 *       ⛔ THAT ONE IS ⛔ SILENT. ⭐ It is the SAME false green loop 2 closed for whole comment
	 *       LINES, ⛔ surviving in TRAILING form — the projection sealed the basis for lines and
	 *       this seals it for tails.
	 *
	 *  ⛔ DELIBERATELY ⛔ NOT A PARSER, in this fixture's house style. A `//` opens a comment ⛔ ONLY
	 *  at column 0 or when ⛔ PRECEDED BY WHITESPACE (`FChar::IsWhitespace`, the same predicate
	 *  `TrimStart` uses — ⛔ not `== ' '`, because a tab must count). ⭐ THAT QUALIFICATION IS THE
	 *  POINT, not decoration: it is what leaves `TEXT("http://…")` and `TEXT("a//b")` INTACT,
	 *  because truncating a ⛔ STRING LITERAL would delete ⛔ REAL CODE and could only ever ⛔ LOWER
	 *  the count — ⛔ a FALSE GREEN, the silent direction.
	 *  ⛔ TWO DECLARED RESIDUALS, and ⛔ BOTH FAIL LOUD (false RED), never silent:
	 *    (i) a tight `Foo();// x` is ⛔ NOT stripped — the house style always spaces the slashes;
	 *    (ii) an ⛔ INLINE C-style block comment is ⛔ NOT stripped. ⛔ Truncating at a block-comment
	 *         OPENER is ⛔ REFUSED on purpose: one that CLOSES and then resumes code on the SAME
	 *         line would lose the statement after it — a ⛔ REAL statement — and that trades this
	 *         row's loud residual for a ⛔ silent one.
	 *         (⛔ The opener is spelled in WORDS here, never as the token: a literal one inside a
	 *          block comment is a `-Wcomment` diagnostic on Clang, and this file must not buy a
	 *          warning with a doc comment.)
	 *
	 *  ⭐ Termination is ⛔ STRUCTURAL, exactly as `CountCharacter` below argues it: a counted `for`
	 *  over `Len()` that ⛔ NEVER consults `Find`, so the UE 5.8 `StartPosition` clamp documented
	 *  above ⛔ has no way in. ⛔ This helper is ⛔ NOT a third copy of the house comment predicate —
	 *  it answers a ⛔ DIFFERENT question (where does a comment ⛔ START on a code line) and it must
	 *  ⛔ NOT be kept in step with the five clauses; only `CodeLinesOnly` carries that duty.
	 *
	 *  ⛔ FEED IT THE OUTPUT OF `CodeLinesOnly`, ⛔ NEVER the raw body. This stage knows nothing
	 *  about whole comment lines, and that ORDER is what keeps ⛔ ONE notion of "a real line" for
	 *  all four rows of test 4 (c).
	 */
	static FString CodeWithoutTrailingComments(const FString& Source)
	{
		TArray<FString> Lines;
		Source.ParseIntoArrayLines(Lines, /*bCullEmpty=*/ false);

		FString Out;
		Out.Reserve(Source.Len());
		for (const FString& Line : Lines)
		{
			const int32 Length = Line.Len();
			int32 Cut = Length;
			for (int32 Index = 0; Index + 1 < Length; ++Index)
			{
				if (Line[Index] == TEXT('/')
					&& Line[Index + 1] == TEXT('/')
					&& (Index == 0 || FChar::IsWhitespace(Line[Index - 1])))
				{
					Cut = Index;
					break;
				}
			}

			Out += Line.Left(Cut);
			Out += TEXT("\n");
		}
		return Out;
	}

	/**
	 *  Occurrences of ONE character. ⛔ Deliberately not expressible through the string-needle
	 *  helper: a single-character needle is the one input that hangs it (see the note above).
	 *
	 *  ⭐ Termination is ⛔ STRUCTURAL, not argued: a counted `for` over `Len()` that never
	 *  consults `Find`, so the engine clamp has no way in. ⛔ Feed it a COMMENT-FREE projection —
	 *  it is a raw character scan and knows nothing about comments by itself.
	 */
	static int32 CountCharacter(const FString& Source, const TCHAR Character)
	{
		int32 Count = 0;
		const int32 Length = Source.Len();
		for (int32 Index = 0; Index < Length; ++Index)
		{
			if (Source[Index] == Character)
			{
				++Count;
			}
		}
		return Count;
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace —
	 *  the house helper, copied verbatim. ⛔ Deliberately NOT a parser: a signature that stops
	 *  matching FAILS rather than silently scanning nothing and reporting a comfortable zero.
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

	/** The shipped tuning — a default-constructed band IS the band the seam passes (`FOG-§9.4`). */
	static FSiegeFogTuning ShippedTuning()
	{
		return FSiegeFogTuning();
	}

	/** Exact-equality tolerance, for the claims whose whole content is the word BIT-IDENTICAL. */
	constexpr float Exact = 0.f;

	/**
	 *  ⛔ NOT synthetic — a Footman's shipped melee `AttackRange`. It is the value 🧑 `J-F23` turns
	 *  on: `min(120, 609.6) == 120` in BOTH fog states, so ⛔ no firing gate can EVER drop a melee
	 *  unit mid-charge, which is why RETENTION (not firing) has to carry his ruling.
	 */
	constexpr float ShippedMeleeReachUU = 120.f;

	/**
	 *  ⛔ NOT synthetic — the `Longbowman`'s shipped firing `Range`, the LONGEST in the roster
	 *  (`FOG-§9.11`'s ordering law). It sits far above the ceiling, so it is the reach that
	 *  discriminates a LIVE clamp from an INERT one.
	 */
	constexpr float ShippedLongestFiringReachUU = 3600.f;

	/**
	 *  ⛔⛔ THE NINE REACH SITES, AS A TABLE WITH A REASON PER ROW — ⛔ NEVER AS A TYPED TOTAL.
	 *
	 *  ⭐ The tree-wide count is DERIVED by summing this table plus the door's own definition, so a
	 *  TENTH site is added by writing down WHY it is a reach, and the number follows. ⛔ A site
	 *  added without a row here turns TEST 2 red, which is the entire point: the failure this
	 *  project keeps paying for is a guard that becomes a counter the moment somebody bumps it.
	 */
	struct FFogClampedReachSite
	{
		/** The enclosing function, by DEFINITION signature (`ExtractFunctionBody`'s needle). */
		const TCHAR* FunctionSignature;
		/** The exact clamped expression that must appear in that body. */
		const TCHAR* Expression;
		/** How many times — ⛔ per body, so a site that moves between bodies goes red. */
		int32 ExpectedCount;
		/** ⛔ Printed into every failure message. ⛔ A row with no reason is not a row. */
		const TCHAR* WhyThisIsAReach;
	};

	static const FFogClampedReachSite FogClampedReachSites[] =
	{
		{
			TEXT("void ASummonedUnit::UpdateState()"), RetentionExpression, 1,
			TEXT("RETENTION 1 of 2 — the uncommanded Standard drop. 'fog should make units DROP existing targets'; ")
			TEXT("this is the ONE line that can do it for a MELEE unit, whose 120 firing gate is a no-op in both ")
			TEXT("fog states."),
		},
		{
			TEXT("void ASummonedUnit::UpdateState()"), FiringExpression, 1,
			TEXT("FIRING 1 of 6 — the uncommanded Standard in-range gate ('no ranged units will be able to fire ")
			TEXT("onto anything above 20 feet away')."),
		},
		{
			TEXT("void ASummonedUnit::UpdateStateStandardCommanded(const ASiegePlayerController& PC)"), RetentionExpression, 1,
			TEXT("RETENTION 2 of 2 — this body 'mirrors the legacy Standard body EXACTLY', so fixing only ")
			TEXT("UpdateState would fix the game for units under NO player command and leave every commanded Blue ")
			TEXT("Standard unit running the other copy."),
		},
		{
			TEXT("void ASummonedUnit::UpdateStateStandardCommanded(const ASiegePlayerController& PC)"), FiringExpression, 2,
			TEXT("FIRING 2 and 3 of 6 — the DEFEND branch and the ATTACK branch. DEFEND is deliberately NOT ")
			TEXT("exempted: exempting one stance would make clear weather and fog differ on that stance alone."),
		},
		{
			TEXT("void ASummonedUnit::UpdateStateGrouped(const FSiegeUnitGroup& Group)"), FiringExpression, 1,
			TEXT("FIRING 4 of 6 — the zone-ordered lane's in-range gate, and the ONLY fog term this lane gets. ")
			TEXT("'commanded units DO NOT LOSE THEIR COMMANDS': it may not shoot past the ceiling, and it does not ")
			TEXT("abandon its circle."),
		},
		{
			TEXT("void ASummonedUnit::UpdateStateSiege()"), FiringExpression, 1,
			TEXT("FIRING 5 of 6 — the Siege in-range gate, which also triggers the Sapper's SUICIDE detonation. ")
			TEXT("ROUTED although INERT on shipped data (Range 120): exempting an inert site would turn a ")
			TEXT("structural rule into a list, and the next long-reach Siege card would be the hole."),
		},
		{
			TEXT("void ASummonedUnit::PerformAttack()"), FiringExpression, 1,
			TEXT("FIRING 6 of 6 — THE ONE THAT ACTUALLY STOPS THE ARROW. It runs on the attack CADENCE timer, not ")
			TEXT("the 0.25 s state poll, so it is what makes fog a STANDING CONDITION rather than an ")
			TEXT("acquisition-time filter (FOG-§9.7)."),
		},
		{
			TEXT("AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const"), NoticeExpression, 1,
			TEXT("THE COMMANDED NOTICE BOUND — the ninth reach. This lane gathers UNBOUNDED around a commanded ")
			TEXT("POINT, so the funnel has no self-reach to clamp and structurally cannot answer for it. 'if an ")
			TEXT("enemy unit walks into the circle … outside the range in which they can notice them due to fog OR ")
			TEXT("ANYTHING, the commanded unit still will not be able to detect them.'"),
		},
	};

	/**
	 *  ⛔⛔ THE CONSUMERS OF `AttackRange` THAT ARE ⛔ NOT FIRING — and routing any of them through
	 *  the firing clamp is a BLOCKER, not a style point (`TASK-978`'s census; `FOG-§9.10b`).
	 *  ⭐ Each row carries what the bug WOULD BE, because "this must stay zero" is not a claim a
	 *  reader can check.
	 */
	struct FUnclampedAttackRangeConsumer
	{
		const TCHAR* FunctionSignature;
		const TCHAR* WhatAClampWouldBreak;
	};

	static const FUnclampedAttackRangeConsumer UnclampedAttackRangeConsumers[] =
	{
		{
			TEXT("ASummonedUnit* ASummonedUnit::FindNearestDamagedFriendly() const"),
			TEXT("THE CLERIC'S HEAL CANDIDATE FILTER. A heal is not an act of shooting; clamping it would make the ")
			TEXT("Fog card shrink the Cleric's mend reach. Inert on today's data (400 < 609.6), which is exactly ")
			TEXT("why a data-drawn test cannot tell a correct exemption from a broken one (SC-§40 cl. 10)."),
		},
		{
			TEXT("void ASummonedUnit::PerformHeal()"),
			TEXT("THE CLERIC'S PER-TICK HEAL RE-VALIDATE — the second heal site, hiding in the tail of a six-term ")
			TEXT("|| chain. It is the one a name census skips."),
		},
		{
			TEXT("bool ASummonedUnit::ResolveWitchPositionCircle(FVector& OutCenter, float& OutRadius) const"),
			TEXT("⛔⛔ THE WITCH'S VEIL RADIUS (FOG-§9.10b) — the COMMON case for a fresh witch, not an edge case. ")
			TEXT("Routing it would make the FOG CARD SILENTLY SHRINK THE INVISIBILITY CARD."),
		},
		{
			TEXT("void ASummonedUnit::EnterAdvance(AActor* Goal)"),
			TEXT("THE APPROACH DISTANCE — MoveToActor(Goal, max(AttackRange × 0.8, 40)). Clamping it would change ")
			TEXT("how close every ranged unit WALKS whenever fog is up, with nothing in any log."),
		},
		{
			TEXT("AActor* ASummonedUnit::AcquireTarget() const"),
			TEXT("⛔ THE ACQUISITION GATHER, and its absence here is the SHARPEST of the five. This site is ALREADY ")
			TEXT("fog-clamped INSIDE the funnel (FOG-§7). A unit-side clamp as well would be arithmetically ")
			TEXT("harmless (min is idempotent) and STRUCTURALLY fatal: a second guard point on the acquisition ")
			TEXT("path, which is the one thing the whole funnel exists to prevent."),
		},
	};
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐⭐ — THE COMMANDED CASE, AND ⛔ THE STEP ORDER **IS** THE ASSERTION.
//
//  ⛔⛔ EACH HALF ALONE IS GREEN AGAINST A DIFFERENT BROKEN DESIGN, so neither is
//  evidence on its own and the ORDER is what makes the pair discriminating:
//    STEP 1 — THE ORDER SURVIVES.   ⛔ Alone it is green against an EMPTY diff.
//    STEP 2 — THE INTRUDER IS REFUSED. ⛔ Alone it is green against a diff that
//             dropped the order wholesale (a unit that abandoned its circle also
//             fails to acquire the intruder — for the wrong reason).
//  ⇒ assert the order survives FIRST, and the refusal SECOND, with the fog term
//  arriving AFTER the command exists. `SC-§37`.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRetentionCommandedOrderSurvivesAndTheIntruderIsRefusedTest,
	"Siegebound.Fog.TheCommandedOrderSurvivesTheFogAndTheIntruderIsStillRefused",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRetentionCommandedOrderSurvivesAndTheIntruderIsRefusedTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRetentionWiringFixture;

	FString UnitSource;
	if (!LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		return false;
	}

	// ══════════════════════════════════════════════════════════════════════════════════════════
	//  STEP 1 ⛔ FIRST — THE ORDER SURVIVES. 🧑 "commanded units DO NOT LOSE THEIR COMMANDS."
	// ══════════════════════════════════════════════════════════════════════════════════════════

	FString GroupedBody;
	if (ExtractFunctionBody(*this, UnitSource,
		TEXT("void ASummonedUnit::UpdateStateGrouped(const FSiegeUnitGroup& Group)"), GroupedBody))
	{
		TestEqual(
			TEXT("(1a) ⭐⭐ THE ZONE-ORDERED LANE GAINED ⛔ NO RETENTION TERM. A unit under a HOLD or AMBUSH order ")
			TEXT("keeps its target and its station when the fog lands. ⛔ An implementer told \"fog drops targets\" ")
			TEXT("could very easily have added a distance drop here, and it would have read as thorough — it would ")
			TEXT("in fact contradict Jonathan by name and make guarded circles empty out under a fog card."),
			CountOccurrencesInCode(GroupedBody, RetentionExpression), 0);

		TestEqual(
			TEXT("(1b) ⛔ …and no RAW leash appeared either. The absence is deliberate and predates fog: \"the zones ")
			TEXT("ARE the leash for HOLD, and AMBUSH's whole point is the unbounded chase.\" This row would catch a ")
			TEXT("drop term smuggled in without the ceiling as readily as one wearing it."),
			CountOccurrencesInCode(GroupedBody, TEXT("LeashRange")), 0);

		TestTrue(
			TEXT("(1c) ⭐ POSITIVE CONTROL — the ZONE machinery is still there, so (1a)/(1b) are absences measured ")
			TEXT("in a live body rather than in an empty read. The 2D disc tests that define zone membership must ")
			TEXT("survive; losing them would turn a zone order into a global sweep and would ALSO satisfy the two ")
			TEXT("zeros above."),
			CountOccurrencesInCode(GroupedBody, TEXT("DistSquared2D")) > 0
			&& CountOccurrencesInCode(GroupedBody, TEXT("Group.AttackRadius")) > 0);

		TestEqual(
			TEXT("(1d) ⭐⭐ AND THE LANE DID RECEIVE ⛔ EXACTLY ONE FOG TERM — its FIRING gate. ⛔ This is the row ")
			TEXT("that stops (1a)-(1c) from being satisfied by a diff that simply never touched this file: the ")
			TEXT("order survives AND the unit still cannot shoot past the ceiling. Both, or the pair proves nothing."),
			CountOccurrencesInCode(GroupedBody, FiringExpression), 1);
	}

	// ══════════════════════════════════════════════════════════════════════════════════════════
	//  STEP 2 ⛔ SECOND — AND ONLY NOW — THE INTRUDER IS REFUSED.
	// ══════════════════════════════════════════════════════════════════════════════════════════

	FString NearPointBody;
	if (ExtractFunctionBody(*this, UnitSource,
		TEXT("AActor* ASummonedUnit::AcquireEnemyNearPoint(const FVector& Center, float Radius) const"), NearPointBody))
	{
		TestEqual(
			TEXT("(2a) ⭐⭐ The commanded lane's NOTICE bound now reads through the ONE unit-side ceiling. ⛔ The ")
			TEXT("bound itself was introduced by TASK-979 and is UNCONDITIONAL (\"due to fog OR ANYTHING\") — this ")
			TEXT("row asserts it was ROUTED, ⛔ not that it was added. A second bound beside it would be two terms ")
			TEXT("racing."),
			CountOccurrencesInCode(NearPointBody, NoticeExpression), 1);

		TestEqual(
			TEXT("(2b) ⛔ …and it is still applied as ONE cut from SELF, hoisted above the candidate loop. Per-")
			TEXT("candidate re-evaluation would buy nothing (the reach is a property of the unit) and would pay a ")
			TEXT("fog-state read for every hostile on the field."),
			CountOccurrencesInCode(NearPointBody, TEXT("Distance > NoticeRadiusUU")), 1);

		TestTrue(
			TEXT("(2c) ⛔ …and the ZONE disc SURVIVES beside it. Two terms, never a replacement: a candidate must be ")
			TEXT("in the commanded circle AND within this unit's reach."),
			CountOccurrencesInCode(NearPointBody, TEXT("DistSquared2D")) > 0);

		TestEqual(
			TEXT("(2d) ⛔⛔ THE FENCE: the bound is still NOT gated on `bRangedAttack`. That flag is PROJECTILE ")
			TEXT("DELIVERY, not \"is ranged\" — CrystalTower ships bRanged=FALSE at Range 800 and would walk straight ")
			TEXT("through such a gate (FOG-§9.8c)."),
			CountOccurrencesInCode(NearPointBody, TEXT("bRangedAttack")), 0);
	}

	// ── (2e) ⭐⭐ THE REFUSAL, ⛔ EXECUTED. The structural rows above prove WHICH reach the site
	//    hands over; these prove what the ceiling DOES to it, at both values of the one boolean.
	//    ⛔ The intruder's distance is DERIVED, never picked: the midpoint between the ceiling and
	//    the shipped notice radius is inside one and outside the other BY CONSTRUCTION, so this
	//    pair cannot go vacuous when either number is retuned (`SC-§40` cl. 10).
	const FSiegeFogTuning Tuning = ShippedTuning();
	const float CeilingUU = Tuning.FogVisionCeilingUU;
	const float NoticeUU = ASummonedUnit::UnitEngagementRadiusUU;

	TestTrue(
		FString::Printf(
			TEXT("(2e) ⭐ PRECONDITION — the ceiling (%.1f) is strictly INSIDE the notice radius (%.1f), so a ")
			TEXT("\"refused under fog, admitted in sunshine\" band EXISTS at all. ⛔ If these two ever meet, the ")
			TEXT("rows below stop discriminating and must FAIL here rather than pass while proving nothing."),
			CeilingUU, NoticeUU),
		CeilingUU < NoticeUU);

	const float IntruderDistanceUU = 0.5f * (CeilingUU + NoticeUU);

	TestTrue(
		FString::Printf(
			TEXT("(2f) ⭐⭐ IN CLEAR WEATHER THE INTRUDER IS ACQUIRED: standing %.1f uu from the guarding unit it is ")
			TEXT("inside the unclamped notice radius %.1f, and the ceiling returns that radius BIT-IDENTICALLY. ")
			TEXT("⛔ This is the half that fails against an over-eager diff — one that clamped the commanded lane in ")
			TEXT("sunshine too would blind guards with no fog on the field."),
			IntruderDistanceUU, NoticeUU),
		IntruderDistanceUU <= FSiegeFogStatics::EffectiveVisionRadius(NoticeUU, /*bFogActive=*/ false, Tuning));

	TestTrue(
		FString::Printf(
			TEXT("(2g) ⭐⭐⭐ AND WITH THE FOG UP — RAISED AFTER THE ORDER EXISTS, WHICH IS WHY THIS ROW COMES ")
			TEXT("LAST — THE SAME INTRUDER IS REFUSED: %.1f uu is outside the fogged reach %.1f. 🧑 \"if an enemy ")
			TEXT("unit walks into the circle that a commanded unit is supposed to be guarding but that enemy unit ")
			TEXT("is outside the range in which they can notice them due to fog OR ANYTHING, the commanded unit ")
			TEXT("still will not be able to detect them.\" ⛔ The unit stays commanded and goes blind."),
			IntruderDistanceUU, FSiegeFogStatics::EffectiveVisionRadius(NoticeUU, /*bFogActive=*/ true, Tuning)),
		IntruderDistanceUU > FSiegeFogStatics::EffectiveVisionRadius(NoticeUU, /*bFogActive=*/ true, Tuning));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐⭐ — THE NINE REACH SITES, DERIVED FROM A TABLE, AND THE FIVE
//  `AttackRange` CONSUMERS THAT MUST ⛔ NOT BE ROUTED. The two halves are one
//  claim: the clamp is applied to REACHES and to nothing else.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRetentionNineReachSitesAndNoOthersTest,
	"Siegebound.Fog.TheCeilingReachesNineSitesAndTheNonFiringConsumersAreUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRetentionNineReachSitesAndNoOthersTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRetentionWiringFixture;

	FString UnitSource;
	if (!LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		return false;
	}

	// ── (a) ⭐⭐ EVERY TABLE ROW, ⛔ IN ITS OWN BODY. Body-scoped rather than file-scoped, so a
	//    clamp that MOVES from one function to another goes red even though the file total is
	//    unchanged — which is precisely the mutation a file-scoped census cannot see.
	int32 ExpectedCallsFromTable = 0;
	for (const FFogClampedReachSite& Site : FogClampedReachSites)
	{
		ExpectedCallsFromTable += Site.ExpectedCount;

		FString Body;
		if (!ExtractFunctionBody(*this, UnitSource, Site.FunctionSignature, Body))
		{
			continue; // already an AddError — a stale signature FAILS rather than scanning nothing
		}

		TestEqual(
			FString::Printf(
				TEXT("(a) ⭐⭐ `%s` appears EXACTLY %d time(s) in %s. ⛔ WHY THIS SITE IS A REACH AT ALL: %s"),
				Site.Expression, Site.ExpectedCount, Site.FunctionSignature, Site.WhyThisIsAReach),
			CountOccurrencesInCode(Body, Site.Expression), Site.ExpectedCount);
	}

	TestTrue(
		TEXT("(a) ⭐ SELF-CHECK: the reach-site table is NOT EMPTY. An emptied table would make the derived total ")
		TEXT("below equal 1 (the definition alone) and would quietly retire every per-body row above — the loop ")
		TEXT("would simply not run. A vacuous table must FAIL here rather than pass in silence."),
		static_cast<int32>(UE_ARRAY_COUNT(FogClampedReachSites)) > 0);

	// ── (b) ⭐⭐⭐ THE TREE-WIDE TOTAL, ⛔ DERIVED FROM THE TABLE AND NEVER TYPED. This is the row
	//    that catches a TENTH site nobody wrote down: the per-body rows above only know about the
	//    bodies somebody listed, so a clamp added to a body that is NOT in the table would pass
	//    every one of them.
	//    ⛔ `+ 1` is the door's OWN DEFINITION, which the needle also matches. It is named rather
	//    than folded into the table so that deleting the definition cannot be absorbed by adding a
	//    call somewhere.
	const int32 ExpectedTotalCalls = 1 /* the definition of the chokepoint itself */ + ExpectedCallsFromTable;

	TestEqual(
		FString::Printf(
			TEXT("(b) ⭐⭐⭐ `%s` appears EXACTLY %d times in SummonedUnit.cpp — 1 definition + %d call sites, ")
			TEXT("DERIVED from the reach-site table, ⛔ never typed. ⛔ AN EXTRA HIT IS A REACH NOBODY WROTE DOWN, ")
			TEXT("and it is the failure this shape exists to catch: the four NON-FIRING consumers of AttackRange ")
			TEXT("(the two Cleric heal sites, the Witch's veil radius and the approach distance) all look exactly ")
			TEXT("like reaches to a reader in a hurry, and routing any of them is a BLOCKER. ⛔ Do NOT fix a red ")
			TEXT("here by bumping the number: add a ROW, with its reason, or take the clamp back out."),
			AnyCeilingCall, ExpectedTotalCalls, ExpectedCallsFromTable),
		CountOccurrencesInCode(UnitSource, AnyCeilingCall), ExpectedTotalCalls);

	// ── (c) ⭐⭐ THE OTHER HALF OF THE SAME CLAIM: the consumers that must stay RAW. Each row
	//    carries the bug a clamp would cause, because "this must remain zero" is not checkable by
	//    a reader — and each is paired with a POSITIVE control proving the body was really read
	//    and really does still consume `AttackRange`.
	for (const FUnclampedAttackRangeConsumer& Consumer : UnclampedAttackRangeConsumers)
	{
		FString Body;
		if (!ExtractFunctionBody(*this, UnitSource, Consumer.FunctionSignature, Body))
		{
			continue;
		}

		TestEqual(
			FString::Printf(
				TEXT("(c) ⛔⛔ `%s` contains ⛔ NO fog ceiling call. ⛔ WHAT ROUTING IT WOULD BREAK: %s"),
				Consumer.FunctionSignature, Consumer.WhatAClampWouldBreak),
			CountOccurrencesInCode(Body, AnyCeilingCall), 0);

		TestTrue(
			FString::Printf(
				TEXT("(c) ⭐ POSITIVE CONTROL for the row above — `%s` was really read and really is a live body ")
				TEXT("(it still consumes a reach of its own). ⛔ Without this, deleting the whole function would ")
				TEXT("satisfy the zero above and read as compliance."),
				Consumer.FunctionSignature),
			Body.Len() > 0
			&& (CountOccurrencesInCode(Body, TEXT("AttackRange")) > 0
				|| CountOccurrencesInCode(Body, TEXT("FSiegeCombatStatics::Gather")) > 0));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐⭐ — THE MELEE PROOF. ⛔ THE FIRING GATE IS **NECESSARY BUT NOT
//  SUFFICIENT**, and this is the arithmetic that proves it rather than asserting
//  it (`FOG-§9.7a`, board item 5).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRetentionTheFiringGateCannotExpressTheMeleeDropTest,
	"Siegebound.Fog.TheFiringGateCannotExpressTheMeleeDropSoRetentionCarriesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRetentionTheFiringGateCannotExpressTheMeleeDropTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRetentionWiringFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float CeilingUU = Tuning.FogVisionCeilingUU;

	// ── (a) ⛔⛔ THE FIRING CEILING IS A NO-OP FOR MELEE — IN BOTH FOG STATES. Executed, at both
	//    values of the one boolean, against the shipped 120.
	TestEqual(
		FString::Printf(
			TEXT("(a) ⭐⭐ A Footman's firing reach with the fog DOWN is %.1f — bit-identical. `min` with no floor."),
			ShippedMeleeReachUU),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedMeleeReachUU, /*bFogActive=*/ false, Tuning),
		ShippedMeleeReachUU, Exact);

	TestEqual(
		FString::Printf(
			TEXT("(a) ⭐⭐⭐ …AND WITH THE FOG UP IT IS ⛔ STILL %.1f, because `min(%.1f, %.1f) = %.1f`. ⛔ THIS IS ")
			TEXT("THE WHOLE ARGUMENT: no value of the fog term can make a 120-range gate behave differently, so a ")
			TEXT("diff that routed ONLY the firing gates would leave a melee unit charging 1500 uu at something the ")
			TEXT("fog says it cannot see — with every ranged-only test still green (FOG-§9.7a)."),
			ShippedMeleeReachUU, ShippedMeleeReachUU, CeilingUU, ShippedMeleeReachUU),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedMeleeReachUU, /*bFogActive=*/ true, Tuning),
		ShippedMeleeReachUU, Exact);

	// ── (b) ⭐ THE DISCRIMINATOR. Without this row (a) would be equally green against a ceiling
	//    that had been quietly disabled — "unchanged" is only meaningful beside something that DID
	//    change under the same call.
	TestEqual(
		FString::Printf(
			TEXT("(b) ⭐⭐ …while the LONGEST shipped firing reach DOES move: the Longbowman's %.1f becomes %.1f ")
			TEXT("under fog. ⛔ The two rows together say the ceiling is LIVE and the melee no-op is arithmetic, ")
			TEXT("⛔ not a disabled clamp."),
			ShippedLongestFiringReachUU, CeilingUU),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedLongestFiringReachUU, /*bFogActive=*/ true, Tuning),
		CeilingUU, Exact);

	// ── (c) ⭐⭐⭐ ⇒ THEREFORE THE RETENTION PATH MUST CARRY `J-F23`, AND IT DOES — AT BOTH SITES.
	//    This is the structural consequence of (a): the count is the point, because ONE site
	//    covers only units under no player command.
	FString UnitSource;
	if (LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		TestEqual(
			TEXT("(c) ⭐⭐⭐ EXACTLY TWO retention sites bound the chase by the FOG-CLAMPED effective leash. ⛔ Both, ")
			TEXT("or the melee drop the arithmetic above proves is necessary exists only for uncommanded units — ")
			TEXT("and a Blue Standard unit under ANY player command runs the other copy of that body. ⛔ This is ")
			TEXT("the row that goes red against a diff which routed only the firing gates."),
			CountOccurrencesInCode(UnitSource, RetentionExpression), 2);

		TestEqual(
			TEXT("(c) ⛔ …and ZERO sites still bound retention by the RAW `LeashRange` member. The raw value is a ")
			TEXT("FLOOR, not the leash; a surviving `> LeashRange` is the pre-TASK-979 inversion still shipping."),
			CountOccurrencesInCode(UnitSource, TEXT("> LeashRange")), 0);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐⭐ — THE ONE-DOOR PROPERTY AT UNIT SCOPE, AND THE AMBUSH EXEMPTION
//  PROVED IN THE ⛔ CALL GRAPH RATHER THAN IN A NAME CENSUS.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRetentionOneDoorAndTheAmbushExemptionIsStructuralTest,
	"Siegebound.Fog.TheUnitSideCeilingIsOneDoorAndAmbushIsExemptByConstruction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRetentionOneDoorAndTheAmbushExemptionIsStructuralTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRetentionWiringFixture;

	FString UnitSource;
	if (!LoadProjectFile(*this, SummonedUnitCpp, UnitSource))
	{
		return false;
	}

	// ── (a) ⭐⭐⭐ ONE SEAM CALL IN THE WHOLE FILE, AND IT LIVES INSIDE THE DOOR. The nine sites
	//    call the DOOR; only the door names the RULE. ⛔ `Tests/SiegeAcquisitionFunnelTest.cpp`
	//    test 9 pins the same property from the other side, through an authorised-chokepoint
	//    table; this row is the local statement of it and would go red on the same mutation.
	TestEqual(
		TEXT("(a) ⭐⭐⭐ The fog REACH seam is named EXACTLY ONCE in SummonedUnit.cpp. ⛔ A second call is a second ")
		TEXT("guard point — the forgotten-guard-point failure the whole funnel exists to prevent — and it would ")
		TEXT("let one of the nine reach sites clamp for itself, which is how the notice/firing/retention gates ")
		TEXT("start disagreeing about the same weather."),
		CountOccurrencesInCode(UnitSource, SeamCall), 1);

	FString ChokepointBody;
	if (ExtractFunctionBody(*this, UnitSource, ChokepointSignature, ChokepointBody))
	{
		TestEqual(
			TEXT("(a) ⭐⭐ …and that ONE call is INSIDE the door. The file-scoped count above cannot see this: one ")
			TEXT("call in the right file but the wrong function satisfies it exactly while destroying the property."),
			CountOccurrencesInCode(ChokepointBody, SeamCall), 1);

		TestEqual(
			TEXT("(b) ⛔ THE DOOR ADDS ⛔ NO ARITHMETIC OF ITS OWN: zero `FMath::Min`. ⛔ The `min` belongs to ")
			TEXT("FSiegeFogStatics::EffectiveVisionRadius and to nowhere else — a second one here would be a rule ")
			TEXT("that could drift from the rule (FOG-§9.6)."),
			CountOccurrencesInCode(ChokepointBody, TEXT("FMath::Min")), 0);

		TestEqual(
			TEXT("(b) ⛔ …zero ceiling LITERALS. `609` must appear nowhere in this class; the number lives in the ")
			TEXT("tuning struct, once (FOG-§1's one-literal discipline)."),
			CountOccurrencesInCode(ChokepointBody, TEXT("609")), 0);

		TestEqual(
			TEXT("(b) ⛔ …and zero BRANCHES. The door cannot learn the fog state — the seam returns a REACH and ")
			TEXT("never the boolean — so there is nothing here to branch on, and an `if` appearing would mean ")
			TEXT("somebody found a way to ask (FOG-§9.6, and TASK-1007's declaration-level guarantee)."),
			CountOccurrencesInCode(ChokepointBody, TEXT("if (")), 0);
	}

	// ── (c) ⭐⭐⭐ THE AMBUSH EXEMPTION, PROVED WHERE IT ACTUALLY LIVES: THE CALL GRAPH.
	//    ⛔ A NAME CENSUS ANSWERS A DIFFERENT QUESTION. "Does the grouped lane read the leash?"
	//    counts symbols and returns "two sites, neither here" — which says nothing about
	//    REACHABILITY, and reachability is the whole claim. `UpdateState` dispatches to the FOLLOW
	//    and GROUPED bodies and `return`s UNCONDITIONALLY, BEFORE the retention line it contains.
	//    ⇒ HOLD / AMBUSH / FOLLOW cannot execute a retention read on ANY path, which is why this
	//    row needs ⛔ no exemption branch — and why adding one would be a second exception on top
	//    of a structural one (FOG-§9.11; TASK-1006 item (5) settled it the same way).
	FString UpdateStateBody;
	if (ExtractFunctionBody(*this, UnitSource, TEXT("void ASummonedUnit::UpdateState()"), UpdateStateBody))
	{
		// ⛔ EVERY index below is taken on the COMMENT-FREE PROJECTION, never on the raw body —
		//    see `CodeLinesOnly`. Mixing a comment-blind `Find` with a comment-aware count over
		//    the same span is what let a commented-out `// return;` anchor this pin on a line that
		//    cannot execute. ⭐ One input, one notion of "a real line", for all four rows.
		// ⛔ TASK-1045 added the SECOND stage, and the ORDER is load-bearing: `CodeLinesOnly` drops
		//    whole comment LINES, then `CodeWithoutTrailingComments` drops the TAIL a comment
		//    leaves on a CODE line. ⛔ Without it this same span was BOTH false-RED (a trailing
		//    `// dispatch;` counts 2 on a CORRECT tree) and false-GREEN (a trailing `// return;`
		//    anchored the pin with the real `return;` DELETED) — both MEASURED on this body.
		const FString UpdateStateCode = CodeWithoutTrailingComments(CodeLinesOnly(UpdateStateBody));

		const int32 RetentionIndex = UpdateStateCode.Find(RetentionExpression, ESearchCase::CaseSensitive);

		TestTrue(
			TEXT("(c) ⭐ PRECONDITION — UpdateState really does contain the retention line the two dispatches must ")
			TEXT("return before. ⛔ If it did not, every ordering row below would be vacuously true. ⛔ Taken on the ")
			TEXT("comment-free projection, so a retention call that survives only inside a COMMENT reads as ABSENT ")
			TEXT("— which is a red here, and correctly so: a commented call clamps nothing."),
			RetentionIndex != INDEX_NONE);

		const TCHAR* const ExemptDispatches[] =
		{
			TEXT("UpdateStateFollow(*FollowGroup);"),
			TEXT("UpdateStateGrouped(*Group);"),
		};

		for (const TCHAR* Dispatch : ExemptDispatches)
		{
			const int32 DispatchIndex = UpdateStateCode.Find(Dispatch, ESearchCase::CaseSensitive);
			const int32 ReturnIndex = (DispatchIndex == INDEX_NONE)
				? INDEX_NONE
				: UpdateStateCode.Find(TEXT("return;"), ESearchCase::CaseSensitive, ESearchDir::FromStart, DispatchIndex);

			// ⛔⛔⛔ THE OWNERSHIP TERM (TASK-1041, from `qa/TASK-1039.md` W-1) — AND WITHOUT IT THIS
			//    ROW WAS ⛔ GREEN AGAINST THE ⛔ EXACT REGRESSION ITS OWN MESSAGE NAMES.
			//    `Find(TEXT("return;"), …, DispatchIndex)` returns the FIRST `return;` ⛔ ANYWHERE
			//    after the dispatch — and `UpdateState` holds ⛔ THREE MORE of them before the
			//    retention line (the Siege, the Support and the Witch dispatches). ⇒ ⛔ delete THIS
			//    dispatch's own `return;` and the ordering below is STILL SATISFIED, because it
			//    finds somebody else's return and that one is also before the retention line.
			//    ⛔ Measured twice, independently (qa/TASK-1039 W-1, re-derived at the `12b8707` audit).
			//    ⭐⭐ THE FIX IS ⛔ OWNERSHIP, NOT ORDERING: exactly ⛔ ONE `;` — the dispatch call's
			//    own — may sit between the dispatch and the `return;` claimed for it. A fall-through
			//    puts a whole branch into that gap (the `}` `else` `ClearCommandGroup();` tail plus
			//    the next dispatch), so the count leaves 1 on the FIRST statement that appears.
			//    ⚠️ Counted on CODE lines only (`SC-§80`) — here because the SPAN itself is already
			//    comment-free, so the paragraphs that EXPLAIN this dispatch cannot inflate it; and
			//    `Between` is EMPTY when either index is stale, which reads 0 — ⛔ never a
			//    comfortable 1.
			//    ⛔ TASK-1041 loop 2: this count is ⛔ NOT `CountOccurrencesInCode(Between, TEXT(";"))`.
			//    A ONE-CHARACTER needle hangs that helper outright on the UE 5.8 `Find` clamp — the
			//    mechanism is written out in full beside the helper, and it cost a >10 h run.
			const FString Between = (DispatchIndex != INDEX_NONE && ReturnIndex != INDEX_NONE && ReturnIndex > DispatchIndex)
				? UpdateStateCode.Mid(DispatchIndex, ReturnIndex - DispatchIndex)
				: FString();

			TestTrue(
				FString::Printf(
					TEXT("(c) ⭐⭐⭐ `%s` is followed by ⛔ ITS OWN `return;` — exactly ONE statement sits between them ")
					TEXT("— and that `return;` comes ⛔ BEFORE the retention line, so that lane can NEVER execute a ")
					TEXT("leash read. ⛔ THIS IS THE ASSERTION THAT MAKES \"AMBUSH IS ")
					TEXT("EXEMPT\" A STRUCTURAL FACT RATHER THAN A CONVENTION — and it is the reason no `if` guards ")
					TEXT("the two drop sites. ⛔ If a future refactor lets this dispatch FALL THROUGH instead of ")
					TEXT("returning, an AMBUSH unit silently acquires a fog leash and abandons its chase, with ")
					TEXT("nothing in the diff naming fog. That is exactly the regression this row is here to catch. ")
					TEXT("⛔ The OWNERSHIP half is the load-bearing one: without it the deleted `return;` is replaced ")
					TEXT("by the next dispatch's return and this row stays green through the whole regression. ")
					TEXT("⛔ And it is measured on CODE ONLY — a projection that drops whole comment LINES and then ")
					TEXT("the TRAILING tail a comment leaves on a code line — so no commented-out `// return;` can ")
					TEXT("stand in for the real one, and no comment's punctuation can inflate the count."),
					Dispatch),
				DispatchIndex != INDEX_NONE
				&& ReturnIndex != INDEX_NONE
				&& RetentionIndex != INDEX_NONE
				&& ReturnIndex < RetentionIndex
				&& CountCharacter(Between, TEXT(';')) == 1);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
