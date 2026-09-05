// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Math/NumericLimits.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/SiegeCombatStatics.h" // FSiegeCombatStatics::ResolveFogClampedReachUU — the subject
#include "Siegebound/SiegeFogStatics.h"    // FSiegeFogStatics::EffectiveVisionRadius + FSiegeFogTuning — the rule the seam delegates to

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for `FSiegeCombatStatics::ResolveFogClampedReachUU`, THE FOG-CLAMPED
 *      **REACH** SEAM (TASK-1007; law `FOG-§9.11`'s retention clause, `FOG-§9.6`, `FOG-§9.10a`,
 *      `FOG-§7`, `FOG-§7b`, `SC-§38`, `SC-§60`, `SC-§80`) ═══
 *
 *  ⭐⭐ WHAT THE SEAM IS FOR, in one sentence: 🧑 *"yes clamp retention under fog"*. Fog currently
 *  binds ⛔ ACQUISITION only — a unit that already holds a target keeps chasing to its leash and
 *  firing at full range — and the acquisition funnel ⛔ structurally cannot fix that, because it
 *  runs at GATHER time and knows nothing about what an already-acquired unit does between
 *  gathers. ⇒ this row builds the door that `TASK-1008` walks through. ⛔ It wires ⛔ NOTHING; the
 *  per-site retention and firing changes are `TASK-1008`'s.
 *
 *  ⛔⛔⛔ THE PROPERTY THIS FILE EXISTS TO PIN, AND IT IS NOT THE ARITHMETIC. `bFogActive` and
 *  `FSiegeFogTuning` ⛔ NEVER CROSS THE SEAM'S BOUNDARY, in ⛔ EITHER DIRECTION. ⇒ a caller cannot
 *  learn the fog state, cannot branch on it, and therefore ⛔ cannot become a second fog-state
 *  door. ⭐ That is a ⛔ STRUCTURAL property rather than a convention, and TEST 3 is the row that
 *  makes it red-able: it asserts the ⛔ DECLARATION, because that is where the impossibility
 *  lives — a body can be rewritten, but a signature that never mentions the state cannot leak it.
 *
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero asset loads, ⛔ zero writes.
 *  Three lanes, the house pattern:
 *    (a) DIRECT CALLS into the seam and into `FSiegeFogStatics`' pure entry points — real
 *        execution, no world. A ⛔ null World is a legitimate, shipped input meaning "never under
 *        fog", so the fog-OFF half of every claim is genuinely EXECUTED rather than described.
 *    (b) SOURCE-TEXT structural probes with comment lines skipped (`CountOccurrencesInCode`) —
 *        the only lane that can see a call graph without a world.
 *    (c) A ⛔ COMMENT-AWARE probe (`CountOccurrencesAnywhere`, copied verbatim from
 *        `Tests/SiegeSpellRoutingTest.cpp` — ⛔ NOT a third implementation) for TEST 4, whose
 *        subject IS prose and which lane (b) is ⛔ structurally blind to (`SC-§80`).
 *
 *  ⚠️⚠️ THE HONEST GAP, STATED SO IT IS NOT DISCOVERED: THE FOG-**ON** BRANCH OF THE SEAM IS
 *  ⛔ NOT EXECUTED ANYWHERE IN THIS SUITE, AND IT ⛔ CANNOT BE. Fog is ON only when an
 *  `AFogVolume` exists in a live world with an unexpired timer, and there is not one
 *  `UWorld::CreateWorld` and not one `SpawnActor` anywhere in `Siegebound/Tests/` (the house
 *  rule). ⇒ ⭐ THE PROOF IS A ⛔ TOTAL CASE ANALYSIS OVER THE ONE BOOLEAN INSTEAD, and it is
 *  complete rather than partial: the seam's body is exactly
 *  `EffectiveVisionRadius(Request, ReadFogState(...), Tuning)` — TEST 2 pins that shape
 *  structurally, with ⛔ no arithmetic and ⛔ no branch of its own — and TEST 1 EXECUTES
 *  `EffectiveVisionRadius` at ⛔ BOTH values of that boolean, against the ⛔ default-constructed
 *  tuning the seam is proven to pass. ⇒ every reachable outcome of the composition is exercised;
 *  what is unexercised is only whether `AFogVolume` flips the boolean, which
 *  `Tests/SiegeFogVolumeTest.cpp` owns. ⛔ Green here is "the seam computes the right reach for
 *  whatever the state says", ⛔ NOT "fog works at runtime".
 */

namespace SiegeFogReachSeamFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these claims are about. */
	const TCHAR* CombatStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp");
	const TCHAR* CombatStaticsHeader = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h");

	/** ⛔ The seam's DEFINITION signature — matched by `ExtractFunctionBody`; a stale one FAILS (`SC-§38`). */
	const TCHAR* SeamDefinitionSignature = TEXT("float FSiegeCombatStatics::ResolveFogClampedReachUU(");

	/** ⛔ The seam's DECLARATION, as it must read in the header. ⛔ Locate by SYMBOL, never by line. */
	const TCHAR* SeamDeclarationNeedle = TEXT("static float ResolveFogClampedReachUU(");

	/** ⛔ The private state seam's DECLARATION. Its ACCESS is the subject of test 3, not its body. */
	const TCHAR* StateSeamDeclarationNeedle = TEXT("static bool ReadFogState(");

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
	 *  `SiegeFogClampTest.cpp` / `SiegeFogVolumeTest.cpp` / `SiegeSpellRoutingTest.cpp` so all of
	 *  them agree character for character. ⛔ Do not "improve" it here; a divergent counter would
	 *  make two files disagree about the same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL: `SiegeCombatStatics.h`'s doc for the seam NAMES
	 *  `bFogActive` and `FSiegeFogTuning` repeatedly — in the paragraphs that explain why they may
	 *  never appear in the signature. A scanner that counted comments would force that header to
	 *  choose between explaining the law and passing it, and TEST 3's zeros would be achievable
	 *  only by deleting the explanation.
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
	 *  ⭐⭐ THE COMMENT-AWARE COUNTER, ⛔ COPIED VERBATIM FROM `Tests/SiegeSpellRoutingTest.cpp`
	 *  RATHER THAN RE-INVENTED, and it exists for ⛔ EXACTLY ONE ROW in this file (TEST 4).
	 *  `CountOccurrencesInCode` above SKIPS comment lines, so it is ⛔ STRUCTURALLY BLIND to a lie
	 *  told in prose (`SC-§80`) — and ⚖️ ***a fix that repairs the code and leaves the prose ships
	 *  a confident explanation of behaviour that no longer exists*** (`SC-§77`). ⛔ Used ONLY for
	 *  that claim; every other row here must stay comment-immune.
	 */
	static int32 CountOccurrencesAnywhere(const FString& Source, const TCHAR* Needle)
	{
		const int32 NeedleLength = FCString::Strlen(Needle);
		if (NeedleLength <= 0)
		{
			return 0;
		}

		int32 Count = 0;
		int32 From = 0;
		for (;;)
		{
			const int32 Found = Source.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, From);
			if (Found == INDEX_NONE)
			{
				break;
			}
			++Count;
			From = Found + NeedleLength;
		}
		return Count;
	}

	/**
	 *  Extracts one function body by signature, ending at the first column-0 closing brace —
	 *  the house helper. ⛔ Deliberately NOT a parser: a signature that stops matching FAILS
	 *  rather than silently scanning nothing (`SC-§38`).
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
	 *  on: `min(120, 609.6) == 120` in BOTH fog states, so ⛔ no firing gate can ever drop a melee
	 *  unit mid-charge, which is why the RETENTION path (not the firing gate) has to carry it.
	 */
	constexpr float ShippedMeleeReachUU = 120.f;

	/**
	 *  ⛔ NOT synthetic — the `Longbowman`'s shipped firing `Range`, the LONGEST in the roster
	 *  (`FOG-§9.11`'s ordering law). It sits far ABOVE the ceiling, so it is the reach that
	 *  discriminates a live clamp from an inert one.
	 */
	constexpr float ShippedLongestFiringReachUU = 3600.f;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE ARITHMETIC, EXECUTED. Fog OFF returns the request BIT-
//  IDENTICALLY; fog ON returns `min(request, ceiling)`; a MELEE reach returns
//  unchanged in BOTH states. ⛔ It is a `min` with NO FLOOR, at every site,
//  forever (`FOG-§9.10a`).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogReachSeamArithmeticTest,
	"Siegebound.Fog.TheClampedReachSeamIsAMinWithNoFloorInBothFogStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogReachSeamArithmeticTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogReachSeamFixture;

	const FSiegeFogTuning Tuning = ShippedTuning();
	const float CeilingUU = Tuning.FogVisionCeilingUU;

	// ── (a) ⛔⛔ THE FOG-OFF HALF IS **EXECUTED THROUGH THE REAL SEAM**, not simulated. A null
	//    World is a legitimate shipped input meaning "never under fog" (`ReadFogState`'s own
	//    contract), so these rows call the shipped function and compare its actual return.
	//    ⭐ BIT-IDENTICAL is the whole claim: with fog down the seam must be indistinguishable from
	//    not calling it, which is what keeps any fog regression ATTRIBUTABLE TO FOG.
	TestEqual(
		TEXT("(a) ⭐⭐ With no fog the seam returns the request BIT-IDENTICALLY — a Longbowman's 3600 comes back as ")
		TEXT("3600, not 'about 3600'. ⛔ Any sanitising, rounding or clamping here would make every clear-weather ")
		TEXT("engagement in the game a fog behaviour change wearing a fog card's commit message."),
		FSiegeCombatStatics::ResolveFogClampedReachUU(/*World=*/ nullptr, ShippedLongestFiringReachUU),
		ShippedLongestFiringReachUU, Exact);

	TestEqual(
		TEXT("(a) ⭐ …and a MELEE reach likewise. 120 in, 120 out."),
		FSiegeCombatStatics::ResolveFogClampedReachUU(/*World=*/ nullptr, ShippedMeleeReachUU),
		ShippedMeleeReachUU, Exact);

	TestEqual(
		TEXT("(a) ⭐ …and the UNBOUNDED sentinel survives too. `FSiegeVisionQuery` uses TNumericLimits<float>::Max() ")
		TEXT("for \"this site has no reach of its own\"; a seam that quietly finitised it would narrow two acquisition ")
		TEXT("sites WITH FOG OFF."),
		FSiegeCombatStatics::ResolveFogClampedReachUU(/*World=*/ nullptr, TNumericLimits<float>::Max()),
		TNumericLimits<float>::Max(), Exact);

	// ── (b) ⛔⛔ THE FOG-ON HALF, AND THE HONESTY ABOUT WHAT IS BEING EXECUTED. The seam's own
	//    fog-ON branch needs a live `AFogVolume`, which this suite cannot build (see the file
	//    header). ⇒ these rows execute the RULE the seam is pinned — by TEST 2, structurally — to
	//    delegate to, at the SAME default-constructed tuning it is pinned to pass. Together the
	//    two tests are a TOTAL case analysis over the one boolean, and neither is sufficient alone.
	TestEqual(
		FString::Printf(
			TEXT("(b) ⭐⭐ With fog UP the ceiling bites: the Longbowman's 3600 becomes %.1f — the same ceiling every ")
			TEXT("other unit gets. 🧑 \"it still gets reduced down to 609 in fog\"."), CeilingUU),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedLongestFiringReachUU, /*bFogActive=*/ true, Tuning),
		CeilingUU, Exact);

	TestEqual(
		FString::Printf(
			TEXT("(b) ⛔⛔⛔ AND THE MELEE REACH COMES BACK **UNCHANGED**: min(120, %.1f) == 120. ⛔ NOT an assignment, ")
			TEXT("⛔ NOT a clamp-to-fog, ⛔ NOT a raise to the ceiling — a `min` with NO FLOOR (`FOG-§9.10a`). ⭐ THIS ")
			TEXT("IS THE ROW THAT PROVES THE FIRING GATE IS NECESSARY BUT **NOT SUFFICIENT** FOR 🧑 J-F23: because ")
			TEXT("this number cannot move, no firing-range clamp can EVER drop a Footman charging across 1500 uu. ")
			TEXT("⇒ the RETENTION path has to carry melee, and a TASK-1008 diff that routes only the firing gate is ")
			TEXT("a blocker."), CeilingUU),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedMeleeReachUU, /*bFogActive=*/ true, Tuning),
		ShippedMeleeReachUU, Exact);

	// ⛔ THE RED-PROOF FOR (b), stated as an assertion rather than as a claim: the two fog states
	// must DISAGREE for a reach above the ceiling. Without this row, every assertion above would
	// still pass against an `EffectiveVisionRadius` that ignored `bFogActive` entirely.
	// ⚠️ Written as `TestTrue` on a `!=` deliberately: `FAutomationTestBase::TestNotEqual` ships
	// only string-shaped overloads (TCHAR*/FStringView/FText/FName) — there is NO numeric one, so
	// the natural spelling would not compile.
	TestTrue(
		TEXT("(b) ⛔ DISCRIMINATOR: the two fog states must disagree somewhere, or every row above is green against ")
		TEXT("a rule that ignores the fog state altogether."),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedLongestFiringReachUU, /*bFogActive=*/ true, Tuning)
			!= FSiegeFogStatics::EffectiveVisionRadius(ShippedLongestFiringReachUU, /*bFogActive=*/ false, Tuning));

	// ⛔ …and they must AGREE for the melee reach, which is the other half of the same claim and
	// the one a "clamp everything to the ceiling" implementation would fail.
	TestEqual(
		TEXT("(b) ⛔ CONTROL: for a reach BELOW the ceiling the two fog states agree exactly. A clamp-to-fog (rather ")
		TEXT("than a min) would raise 120 to the ceiling and turn this red — which is the defect FOG-§9.10a names."),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedMeleeReachUU, /*bFogActive=*/ true, Tuning),
		FSiegeFogStatics::EffectiveVisionRadius(ShippedMeleeReachUU, /*bFogActive=*/ false, Tuning), Exact);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐ — THE SEAM IS **EXACTLY** ONE STATE READ HANDED TO THE CEILING.
//  ⛔ No arithmetic of its own, ⛔ no branch of its own, ⛔ no second ceiling.
//  This is the row that makes TEST 1's total case analysis COMPLETE rather than
//  merely suggestive.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogReachSeamIsPureDelegationTest,
	"Siegebound.Fog.TheClampedReachSeamDelegatesToTheOneRuleAndAddsNoArithmetic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogReachSeamIsPureDelegationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogReachSeamFixture;

	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	FString SeamBody;
	if (!ExtractFunctionBody(*this, CombatCpp, SeamDefinitionSignature, SeamBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted seam body is substantial — an empty extraction would make every assertion ")
		TEXT("below pass vacuously."),
		SeamBody.Len() > 200);

	TestEqual(
		TEXT("⭐ ONE fog-state read per call, from the ONE private seam. ⛔ Two reads inside one function would be a ")
		TEXT("torn answer: the fog could expire between them."),
		CountOccurrencesInCode(SeamBody, TEXT("ReadFogState(")), 1);

	TestEqual(
		TEXT("⭐⭐ …handed to the ONE rule, ONCE. ⛔ If this is ZERO the seam returns the request untouched and every ")
		TEXT("caller TASK-1008 wires becomes inert — while TEST 1 stays fully green, because TEST 1 exercises the ")
		TEXT("RULE and this row is the only thing asserting the seam actually reaches it."),
		CountOccurrencesInCode(SeamBody, TEXT("FSiegeFogStatics::EffectiveVisionRadius(")), 1);

	// ⛔⛔ CALLED UNCONDITIONALLY, exactly as the funnel calls it. A hand-written state check would
	// re-introduce the branch the rule was built to delete AND is strictly weaker: it sails past
	// every degenerate tuning (`FOG-§7b`), where the rule's own totality guard is what saves the
	// game from a NaN, negative or zero ceiling.
	TestEqual(
		TEXT("⛔⛔ The seam contains NO hand-written `if (bFogActive`. The rule is called unconditionally and returns ")
		TEXT("the request bit-identically with fog off, which is a STRICTLY STRONGER shape than a state check: it is ")
		TEXT("also correct for a NaN, negative or zero ceiling, where a state check would run with a broken band."),
		CountOccurrencesInCode(SeamBody, TEXT("if (bFogActive")), 0);

	TestEqual(
		TEXT("⛔⛔ …and the seam performs NO `FMath::Min` of its own. The `min` belongs to `EffectiveVisionRadius` and ")
		TEXT("to nowhere else — a second copy of the comparison is a second place for the no-floor rule to rot, and it ")
		TEXT("would diverge silently the day the band grows a term."),
		CountOccurrencesInCode(SeamBody, TEXT("FMath::Min")), 0);

	TestEqual(
		TEXT("⛔ …and it never reads the ceiling member directly. A seam that touched `FogVisionCeilingUU` itself would ")
		TEXT("have re-implemented the rule's guard clauses by omission (`FOG-§7b`: a zero ceiling must degrade to NO ")
		TEXT("FOG, never to no vision)."),
		CountOccurrencesInCode(SeamBody, TEXT("FogVisionCeilingUU")), 0);

	// ⛔ THE POSITIVE CONTROL (`SC-§39`) — a dead extraction must not read as a clean set of zeros.
	// This exercises the FALSE-NEGATIVE direction with a needle that IS present in the same body.
	TestEqual(
		TEXT("⛔ POSITIVE CONTROL: the same body, the same scanner, finds the ONE thing the seam does return. Without ")
		TEXT("this row the zeros above would be indistinguishable from a probe scanning an empty string."),
		CountOccurrencesInCode(SeamBody, TEXT("return FSiegeFogStatics::EffectiveVisionRadius(")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⛔⛔⭐⭐ — **THE SECOND DOOR IS UNREPRESENTABLE.** `bFogActive` and
//  `FSiegeFogTuning` never cross the seam's boundary in either direction, and
//  `ReadFogState` is still `private:`. ⛔ ASSERTED AT THE **DECLARATION**,
//  because that is where the impossibility lives — a body can be rewritten, but
//  a signature that never mentions the state cannot leak it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogReachSeamCannotLeakTheFogStateTest,
	"Siegebound.Fog.TheClampedReachSeamCannotLeakTheStateAndReadFogStateStaysPrivate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogReachSeamCannotLeakTheFogStateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogReachSeamFixture;

	FString CombatHeader;
	if (!LoadProjectFile(*this, CombatStaticsHeader, CombatHeader))
	{
		return false;
	}

	// ── (a) ⛔ THE SEAM IS DECLARED, ONCE, AND ITS DECLARATION IS THE SUBJECT. ────────────────
	TestEqual(
		TEXT("(a) ⛔ The reach seam is declared exactly once in the header. Two declarations would mean an overload, ")
		TEXT("and an overload is where a state-carrying variant would arrive without anybody calling it a new door."),
		CountOccurrencesInCode(CombatHeader, SeamDeclarationNeedle), 1);

	// ⛔ The two Finds are ORDERED, not folded: the second takes the first as its start position, so
	// a stale needle must be rejected BEFORE it is used as an index (`SC-§38` — a probe pinned to a
	// coordinate that stopped matching goes RED, it does not quietly scan from somewhere else).
	const int32 SeamDeclarationIndex = CombatHeader.Find(SeamDeclarationNeedle, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	if (SeamDeclarationIndex == INDEX_NONE)
	{
		AddError(FString::Printf(TEXT("⛔ '%s' not found in the header — the probe is stale, so it FAILS."), SeamDeclarationNeedle));
		return false;
	}

	const int32 DeclarationEnd = CombatHeader.Find(TEXT(";"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SeamDeclarationIndex);
	if (DeclarationEnd == INDEX_NONE || DeclarationEnd <= SeamDeclarationIndex)
	{
		AddError(TEXT("⛔ Could not isolate the seam's declaration — no terminating ';' after it, so the probe FAILS."));
		return false;
	}

	const FString SeamDeclaration = CombatHeader.Mid(SeamDeclarationIndex, DeclarationEnd - SeamDeclarationIndex);

	TestTrue(
		FString::Printf(TEXT("SELF-CHECK: the isolated declaration is a signature and not an empty span — read: '%s'"), *SeamDeclaration),
		SeamDeclaration.Len() > 30 && SeamDeclaration.Contains(TEXT("const UWorld* World"), ESearchCase::CaseSensitive));

	// ── (b) ⛔⛔⛔ THE WHOLE SAFETY ARGUMENT, IN THREE ZEROS. ───────────────────────────────────
	//    ⛔ Measured over the DECLARATION ALONE, deliberately: the doc block above it NAMES all
	//    three of these symbols, repeatedly, in the paragraphs that explain why they may never
	//    appear here. Scanning the doc too would force the header to choose between explaining the
	//    law and passing it.
	const TCHAR* const ForbiddenInTheSignature[] =
	{
		TEXT("FSiegeFogTuning"), // the BAND — in or out, by value, reference or pointer
		TEXT("bFogActive"),      // the STATE, by its own name
		TEXT("bool"),            // ⛔ ANY bool: an out-param, a return, or a "bIsFogged" under a new spelling
	};

	for (const TCHAR* Forbidden : ForbiddenInTheSignature)
	{
		TestEqual(
			*FString::Printf(
				TEXT("(b) ⛔⛔⛔ '%s' does NOT appear in the seam's signature, in EITHER direction. ⭐ THIS IS THE ")
				TEXT("ENTIRE SAFETY ARGUMENT AND IT IS STRUCTURAL, NOT A CONVENTION: a caller that cannot LEARN the ")
				TEXT("fog state cannot BRANCH on it, and therefore cannot grow into a second fog-state door. ")
				TEXT("⛔ A signature that lets a caller reconstruct the state is the WRONG signature, however ")
				TEXT("convenient — including a `bool` out-parameter that only tells you \"it was foggy\". ")
				TEXT("Declaration read: '%s'"),
				Forbidden, *SeamDeclaration),
			CountOccurrencesInCode(SeamDeclaration, Forbidden), 0);
	}

	// ⛔ POSITIVE CONTROL for (b): the scanner is alive on this exact span. Without it the three
	// zeros above are a statement about the reader rather than about the signature.
	TestEqual(
		TEXT("(b) ⛔ POSITIVE CONTROL: the same scanner, over the same isolated declaration, finds the two things that ")
		TEXT("ARE there — a world and a float reach."),
		CountOccurrencesInCode(SeamDeclaration, TEXT("float")), 2);

	// ── (c) ⛔⛔ `ReadFogState` STAYS `private:`. ⛔ No access move, no re-exporting wrapper, no
	//    `friend`. The ordering test below is the honest form: there is exactly ONE `public:` and
	//    exactly ONE `private:` in this class, so "after private:" IS "private".
	const int32 PublicIndex = CombatHeader.Find(TEXT("\npublic:"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 PrivateIndex = CombatHeader.Find(TEXT("\nprivate:"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 StateSeamIndex = CombatHeader.Find(StateSeamDeclarationNeedle, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);

	TestTrue(
		TEXT("SELF-CHECK: the header's one `public:`, its one `private:` and the state seam's declaration were all ")
		TEXT("located — a missing one would make the ordering claims below vacuous."),
		PublicIndex != INDEX_NONE && PrivateIndex != INDEX_NONE && StateSeamIndex != INDEX_NONE);

	TestEqual(
		TEXT("(c) ⛔ EXACTLY ONE `public:` and ONE `private:` in this header. The ordering assertions below mean ")
		TEXT("\"private\" ONLY while that is true — a third access specifier would silently turn a position test into ")
		TEXT("a coin flip."),
		CountOccurrencesInCode(CombatHeader, TEXT("public:")) + CountOccurrencesInCode(CombatHeader, TEXT("private:")), 2);

	TestTrue(
		TEXT("(c) ⛔⛔ `ReadFogState` is declared AFTER `private:` — it is still PRIVATE, and it stays private. ")
		TEXT("⛔ No access-specifier move, ⛔ no wrapper that re-exports it, ⛔ no friend. The TASK-980 author refused ")
		TEXT("to route around this and was ruled CORRECT; the reach seam is the sanctioned alternative and it hands ")
		TEXT("back a REACH, never the state."),
		StateSeamIndex > PrivateIndex);

	TestTrue(
		TEXT("(c) ⭐ …and the reach seam is declared BEFORE `private:` — it is the PUBLIC door, which is the whole ")
		TEXT("point of building it."),
		SeamDeclarationIndex > PublicIndex && SeamDeclarationIndex < PrivateIndex);

	TestEqual(
		TEXT("(c) ⛔ ZERO `friend` declarations in this header. A friend is the quietest possible way to re-open the ")
		TEXT("private state seam — it needs no signature change and no access-specifier move, so nothing else here ")
		TEXT("would notice."),
		CountOccurrencesInCode(CombatHeader, TEXT("friend ")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⛔⛔⭐⭐ — THE PROSE THE OTHER THREE ROWS ARE **BLIND** TO.
//  `CountOccurrencesInCode` skips comment lines by design (`SC-§80`), so nothing
//  in this suite can see a doc block that confidently describes a game that
//  stopped existing. ⚖️ `SC-§77`: ***a fix that repairs the code and leaves the
//  prose ships a confident explanation of behaviour that no longer exists.***
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogReachSeamProseIsNotStaleTest,
	"Siegebound.Fog.TheFogSeamsProseDoesNotDescribeAGameThatStoppedExisting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogReachSeamProseIsNotStaleTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogReachSeamFixture;

	FString CombatHeader;
	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsHeader, CombatHeader) || !LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	// ⛔⛔ WHY THESE NEEDLES AND NOT THE SENTENCES THEMSELVES, said out loud because it is the one
	// non-obvious decision in this row. The repaired blocks DESCRIBE the claims they retired
	// rather than QUOTING them, precisely so this probe can exist: a comment-aware scanner
	// ⛔ cannot tell a quoted retirement from a live claim, so a needle that the repair reproduces
	// would make its own guard unpassable and force the source to stop explaining itself (the
	// trade-off `Tests/SiegeSpellRoutingTest.cpp` records for its own blacklist row).
	// ⭐ Each needle below is the FUTURE-FRAMED OWNERSHIP claim — "somebody will build this later"
	// — which is the shape that actually went false, and which no honest retirement needs to
	// reproduce verbatim.
	struct FRetiredClaim
	{
		const TCHAR* Needle;
		const TCHAR* WhyItIsFalseNow;
	};

	const FRetiredClaim RetiredHeaderClaims[] =
	{
		{
			TEXT("TASK-839 is BLOCKED BY THIS TASK"),
			TEXT("TASK-839 DECLINED this seam and TASK-998 wired it. A doc block that still routes a reader to a ")
			TEXT("task that never did the work sends them to the wrong diff.")
		},
		{
			TEXT("WHAT TASK-839 DOES WITH IT"),
			TEXT("It described a FUTURE edit — replacing the final `return false` with the AFogVolume read — that ")
			TEXT("has already happened, under a different task number.")
		},
		{
			TEXT("TODAY THIS RETURNS FALSE"),
			TEXT("The seam consults AFogVolume and the cut CAN fire. This sentence told every future reader that a ")
			TEXT("green suite implied an inert card, which was true when written and is the opposite of true now.")
		},
	};

	for (const FRetiredClaim& Claim : RetiredHeaderClaims)
	{
		TestEqual(
			*FString::Printf(
				TEXT("⛔⛔ `SiegeCombatStatics.h` no longer carries '%s' — comments INCLUDED. ⇒ %s"),
				Claim.Needle, Claim.WhyItIsFalseNow),
			CountOccurrencesAnywhere(CombatHeader, Claim.Needle), 0);
	}

	TestEqual(
		TEXT("⛔⛔ `SiegeCombatStatics.cpp` no longer carries 'TASK-839 inherits' — comments INCLUDED. ⇒ that sentence ")
		TEXT("sat SEVEN LINES above one announcing THE SEAM IS WIRED: two adjacent sentences with opposite temporal ")
		TEXT("framings, both shipped, leaving a reader two confident statements and no way to tell which was current. ")
		TEXT("⚖️ Fixing one of two contradictory adjacent sentences is WORSE than fixing neither."),
		CountOccurrencesAnywhere(CombatCpp, TEXT("TASK-839 inherits")), 0);

	// ⭐ THE OTHER HALF OF THE SAME CLAIM: the sentence that was TRUE is still there. Without this
	// row, deleting BOTH contradictory sentences would pass — and the file would have lost the one
	// statement a reader actually needs.
	TestTrue(
		TEXT("⭐⭐ …and the TRUE half SURVIVES: `SiegeCombatStatics.cpp` still announces THE SEAM IS WIRED. ⛔ The pair ")
		TEXT("moved TOGETHER — the stale framing retired, the accurate one kept — which is the only outcome that ")
		TEXT("leaves a reader better off than before."),
		CombatCpp.Contains(TEXT("THE SEAM IS WIRED"), ESearchCase::CaseSensitive));

	// ⛔⛔ THE POSITIVE CONTROLS (`SC-§39`). Every assertion above is an ABSENCE, and an absence
	// measured with a dead instrument is the most reassuring failure available. These rows prove
	// the comment-aware scanner (a) read each file and (b) really does see COMMENT text — which is
	// the exact capability `CountOccurrencesInCode` lacks and this row exists to supply.
	TestTrue(
		TEXT("⛔ POSITIVE CONTROL: the comment-aware scanner finds text that exists ONLY inside a comment in the ")
		TEXT("header. A scanner that could not see prose would report every absence above as clean."),
		CountOccurrencesAnywhere(CombatHeader, TEXT("TWO AUTHORISED READERS")) > 0);

	TestTrue(
		TEXT("⛔ POSITIVE CONTROL: …and likewise in the .cpp."),
		CountOccurrencesAnywhere(CombatCpp, TEXT("REFUSAL PATH 1 OF 2")) > 0);

	// ⛔ AND THE CONTROL THAT PROVES THE TWO SCANNERS DIFFER — without it, nothing here shows this
	// row is doing anything the comment-blind lane could not already do.
	TestTrue(
		TEXT("⛔⛔ INSTRUMENT CONTROL: the comment-AWARE scanner sees strictly more of the header than the ")
		TEXT("comment-BLIND one for a needle that lives only in prose. ⇒ this row is measuring something the rest of ")
		TEXT("the suite is structurally incapable of measuring (`SC-§80`), rather than duplicating it."),
		CountOccurrencesAnywhere(CombatHeader, TEXT("TWO AUTHORISED READERS"))
			> CountOccurrencesInCode(CombatHeader, TEXT("TWO AUTHORISED READERS")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
