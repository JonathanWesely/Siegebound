// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/UnrealMemory.h" // FMemory::Memcpy — the sanctioned bit-pattern NaN (SiegeCastBarTest.cpp:129)
#include "Internationalization/Text.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/FogVolume.h" // AFogVolume::BrightSunWindowSeconds — TASK-991 calls the PURE static directly, to show that a higher perch yields a LONGER would-be window and therefore a DIFFERENT sentence
#include "Siegebound/SiegePlayerController.h" // ASiegePlayerController::WholeSecondsText — the ONE shared formatter, called directly

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for THE **TWO PREVENTION REFUSALS** — ⭐⭐ THE FIRST REFUSALS IN THIS
 *      GAME WHOSE VALUES CHANGE BETWEEN TWO CLICKS (TASK-989 tests 1-6; ⭐ TASK-991 tests 7-10;
 *      law `FOG-§10.7` (A), `FOG-§10.6`, `FOG-§10.2`, `FOG-§10.3`, `SC-§37`, `SC-§40` cl. 2,
 *      `SC-§53` cl. 3; rulings ✅ `J-F18` `J-F19` `J-F24` `J-F25` `J-F26`) ═══
 *
 *  ⭐⭐ TASK-991 GREW INTO THIS FILE RATHER THAN CREATING A THIRD ONE — which is what (c) below
 *  predicted it would do. Its gate `TASK-990` had RETURNED (PASS) before that row ran, so the
 *  direction that kept `Tests/SiegeBrightSunTest.cpp` untouched (a) permits this file: a returned
 *  verdict is not mutated by a later row's additions, an unreturned one is. ⛔ THREE PINS WERE
 *  RE-POINTED, ⛔ NONE DELETED, and each says so in place with its old value and its reason: two
 *  remainder-read counts in TEST 2 (1 ⇒ 2) and the formatted-refusal idiom count in TEST 5
 *  (1 ⇒ 2). ⭐ In every case the CLAIM is unchanged and the arithmetic followed the second refusal.
 *
 *  📌 Jonathan, verbatim (2026-09-04): *"the player should not lose gold, not have the card get
 *  casted, and instead get a message telling them bright sun is still up for 'x' amount of
 *  seconds, where the 'x' is the ACTUAL amount of time left for the fog prevention."*
 *
 *  ⭐⭐ WHY A NEW FILE RATHER THAN ROWS IN `Tests/SiegeBrightSunTest.cpp` — stated because that
 *  file IS the natural-looking home and the row's spec told me to look there first:
 *    (a) ⛔ IT IS UNDER AN UNRETURNED GATE. `TASK-982` shipped it `ready-for-qa` and its gate
 *        `TASK-986` has ⛔ not reported. Adding rows to it now would mutate a REVIEW SUBJECT
 *        MID-REVIEW — the gate-integrity failure ruled on 2026-09-04 (a changed artefact wearing
 *        a verdict it never received). `TASK-982` was allowed to edit `SiegeFogVolumeTest.cpp`
 *        for exactly the opposite reason: that file's gate had already returned.
 *    (b) ⛔ DIFFERENT SUBJECT. That file is about `AFogVolume` — the state machine, the formula,
 *        the one-way door. This one is about `ASiegePlayerController` — the card-play refusal,
 *        its wording, its ordering and its surface. ⛔ Not one assertion is duplicated between
 *        them; this file re-asserts nothing that file already covers.
 *    (c) ⭐ AND IT IS THE FRAME `TASK-991` SHOULD GROW INTO. The sun-on-sun refusal is this
 *        refusal's sibling, in this same function, consuming this same formatter — so it belongs
 *        HERE, and finding this file is what stops it creating a third one.
 *  ⛔ `Tests/SiegeBrightSunTest.cpp`, `Tests/SiegeFogVolumeTest.cpp`, `Tests/SiegeFogTest.cpp`,
 *  `Tests/SiegeFogClampTest.cpp` and `Tests/SiegePlacementTest.cpp` are ⛔ BYTE-UNTOUCHED by this
 *  row.
 *
 *  MECHANISM — ⛔ zero PIE, ⛔ zero `SpawnActor`, ⛔ zero asset loads, ⛔ zero writes. Two lanes:
 *    (a) ⭐ DIRECT CALLS on the pure static `ASiegePlayerController::WholeSecondsText` — the
 *        `EffectiveBuildingClearance` / `IsInsidePlacementFootprint` precedent in
 *        `SiegePlacementTest.cpp`. This is the lane that is genuinely EXECUTED, and it is where
 *        the "two clicks show two numbers" claim is proved rather than described.
 *    (b) SOURCE-TEXT structural probes with comment lines skipped, for the claims that are about
 *        WHICH CALL FEEDS WHICH and about what is ABSENT — a call graph and an absence cannot be
 *        seen from a running function.
 *
 *  ⚠️⚠️ THE DECLARED GAP, stated so it is honest rather than discovered by the gate: there is not
 *  one `SpawnActor` anywhere in `Siegebound/Tests/`, so the end-to-end behaviour — that clicking
 *  `Fog` twice, one second apart, during a live window really produces two different toasts — is
 *  ⛔ NOT executed here. What IS executed is the half that can be: that the formatter is INJECTIVE
 *  across two remainders a second apart (a formatter that ignored its argument, or returned a
 *  static string, goes RED). The other half — that the argument is a LIVE accessor call read at
 *  the click and never stored — is pinned structurally in test 2, and every form was chosen to be
 *  the one that dies on the actual hazard: a cached remainder dies as a member field, and a
 *  frozen one dies as a second source for the number.
 */

namespace SiegeFogRefusalFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these claims are about. */
	const TCHAR* PlayerControllerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp");
	const TCHAR* PlayerControllerH = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h");
	const TCHAR* CardHandWidgetCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/CardHandWidget.cpp");

	/** The two function signatures this file reads. ⛔ A stale one FAILS rather than scanning nothing. */
	const TCHAR* PlayHandSlotSignature = TEXT("void ASiegePlayerController::PlayHandSlot(int32 Slot)");
	const TCHAR* RefuseCardPlaySignature = TEXT("void ASiegePlayerController::RefuseCardPlay(FName CardID, const FText& Reason)");
	const TCHAR* WholeSecondsTextSignature = TEXT("FText ASiegePlayerController::WholeSecondsText(float Seconds)");

	/**
	 *  A quiet NaN built from its IEEE-754 bit pattern — copied verbatim from
	 *  `SiegeCastBarTest.cpp:129` / `SiegeBrightSunTest.cpp`, which already paid for this lesson.
	 *  ⛔ NOT `0.f / 0.f` and ⛔ NOT `FMath::Sqrt(-1.f)`: both are constant-foldable, and a
	 *  fast-math build may fold them into something finite — which would make the totality row
	 *  below pass while testing NOTHING. ⭐ Its use is paired with an `FMath::IsFinite` self-check.
	 */
	static float MakeQuietNaN()
	{
		const uint32 NaNBits = 0x7FC00000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &NaNBits, sizeof(Result));
		return Result;
	}

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
	 *  `SiegeBrightSunTest.cpp` / `SiegeFogVolumeTest.cpp` / `SiegeFogClampTest.cpp` /
	 *  `SiegeAcquisitionFunnelTest.cpp` so all of them agree character for character.
	 *  ⛔ Do not "improve" it here; a divergent counter would make two files disagree about the
	 *  same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL: the new refusal's own comment block NAMES
	 *  `SpendGold`, `ConfirmInstantDraw`, `ConfirmPlayFromHand` and `RaiseFog` in the paragraphs
	 *  that explain why none of them is reached. A scanner that counted comments would force that
	 *  code to choose between explaining the law and passing it — and tests 3 and 4 below assert
	 *  ZEROes that those very comments would break.
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

	/**
	 *  ⭐⭐ THE ORDERING LANE, AND ORDER IS THE ASSERTION IN THIS ROW (`TASK-989` item (6)).
	 *  Returns everything in Body that comes BEFORE the first occurrence of Marker, so the caller
	 *  can count what has and has not happened by the time execution reaches it.
	 *  ⛔ A MISSING MARKER FAILS: an ordering claim measured against a marker that is no longer
	 *  there would otherwise report "nothing after it" — i.e. green — which is the exact
	 *  quietly-passing shape `SC-§38` bans.
	 *  ⚠️ Every marker passed below is verified UNIQUE in the body, comments included, so the
	 *  split point cannot land inside a paragraph that merely MENTIONS the code.
	 */
	static bool SubstringBefore(FAutomationTestBase& Test, const FString& Body, const TCHAR* Marker, FString& OutPrefix)
	{
		const int32 MarkerIndex = Body.Find(Marker, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		if (MarkerIndex == INDEX_NONE)
		{
			Test.AddError(FString::Printf(TEXT("⛔ Ordering marker '%s' is gone — the ordering probe is stale, so it FAILS."), Marker));
			return false;
		}

		OutPrefix = Body.Left(MarkerIndex);
		return true;
	}

	/**
	 *  ⭐⭐ TASK-991 — THE SUN-ON-SUN GUARD'S OWN REGION: everything between its effect gate and its
	 *  message. Three of the rows below measure inside it, and measuring inside it rather than over
	 *  the whole function is what makes their claims about THIS refusal rather than about its
	 *  `FogCover` sibling, which lives above it in the same function and reads the same accessor.
	 *  ⛔ A MISSING BOUNDARY FAILS (`SC-§38`): a region probe that silently degraded to an empty
	 *  string would report every "must contain" row as red and every "must not contain" row as
	 *  green — i.e. it would pass the ban rows while testing nothing.
	 */
	static bool ExtractSunOnSunRegion(FAutomationTestBase& Test, const FString& PlayBody, FString& OutRegion)
	{
		const int32 GateIndex = PlayBody.Find(TEXT("if (Row->SpellEffect == ESpellEffect::FogClear)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		const int32 MessageIndex = PlayBody.Find(TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunWouldShorten\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
		if (GateIndex == INDEX_NONE || MessageIndex == INDEX_NONE || MessageIndex <= GateIndex)
		{
			Test.AddError(TEXT("⛔ The sun-on-sun effect gate and its message are not both present, in that order — the region probe is stale, so it FAILS."));
			return false;
		}

		OutRegion = PlayBody.Mid(GateIndex, MessageIndex - GateIndex);
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐ — THE COUNTDOWN IS SPELLED IN **WHOLE, ROUNDED SECONDS, EVEN PAST 60**
//  (✅ `J-F24`, closed by Jonathan). ⛔ "143 seconds", ⛔ NEVER "2 minutes 23
//  seconds". Executed by DIRECT CALL, so this is a real measurement of the shipped
//  function rather than a description of it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalSecondsFormatterTest,
	"Siegebound.FogRefusal.TheCountdownIsWholeRoundedSecondsEvenPastSixty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalSecondsFormatterTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	// ⭐ HIS OWN EXAMPLE, ARITHMETIC SHOWN — 2 min 23 s = 2 × 60 + 23 = 143. It is RE-DERIVED
	// here rather than re-typed so this row and his sentence cannot agree by both being wrong.
	const int32 HisExampleSeconds = 2 * 60 + 23;
	TestEqual(TEXT("SELF-CHECK: his worked example is 143 seconds"), HisExampleSeconds, 143);

	const FString HisExample = ASiegePlayerController::WholeSecondsText(static_cast<float>(HisExampleSeconds)).ToString();
	TestEqual(
		TEXT("⭐⭐ `J-F24`: 143 s reads as \"143 seconds\" — ONE unit, past 60, exactly as he wrote it"),
		HisExample, FString(TEXT("143 seconds")));
	TestFalse(
		TEXT("⛔ `J-F24`: it is NOT spelled \"2 minutes 23 seconds\" — a minutes/seconds split is the one form he ruled out"),
		HisExample.Contains(TEXT("minute"), ESearchCase::IgnoreCase));

	TestEqual(
		TEXT("⛔ 60 s stays SECONDS rather than rolling into a minute (the boundary the ruling is about)"),
		ASiegePlayerController::WholeSecondsText(60.f).ToString(), FString(TEXT("60 seconds")));
	TestEqual(
		TEXT("⛔ 120 s — the shipped BASE window — reads as \"120 seconds\", not \"2 minutes\""),
		ASiegePlayerController::WholeSecondsText(120.f).ToString(), FString(TEXT("120 seconds")));

	// ⛔ ROUNDED, ⛔ not floored and ⛔ not ceiled. These two rows are the ONLY inputs that can
	// tell the three apart, which is why they are here and why they straddle the half-second.
	TestEqual(
		TEXT("⛔ 42.4 s ROUNDS DOWN to 42 (a ceil would say 43)"),
		ASiegePlayerController::WholeSecondsText(42.4f).ToString(), FString(TEXT("42 seconds")));
	TestEqual(
		TEXT("⛔ 42.6 s ROUNDS UP to 43 (a floor would say 42) — round, exactly as ruled"),
		ASiegePlayerController::WholeSecondsText(42.6f).ToString(), FString(TEXT("43 seconds")));

	// ⭐ THE SINGULAR. "1 seconds" is the blemish a countdown reaches on its way to zero, and it
	// is handled ONCE, here, because the units live in the formatter rather than at the call site.
	TestEqual(
		TEXT("⭐ 1 s is SINGULAR — \"1 second\", never \"1 seconds\""),
		ASiegePlayerController::WholeSecondsText(1.f).ToString(), FString(TEXT("1 second")));

	// ⛔ TOTALITY. The formatter is public and `TASK-991` will feed it a COMPUTED duration, so the
	// degenerate inputs are reachable by a caller that does not exist yet.
	TestEqual(
		TEXT("⛔ 0 s reads as \"0 seconds\""),
		ASiegePlayerController::WholeSecondsText(0.f).ToString(), FString(TEXT("0 seconds")));
	TestEqual(
		TEXT("⛔ a NEGATIVE remainder is never spelled out — it reads as \"0 seconds\""),
		ASiegePlayerController::WholeSecondsText(-5.f).ToString(), FString(TEXT("0 seconds")));

	const float QuietNaN = MakeQuietNaN();
	TestFalse(
		TEXT("SELF-CHECK: the bit-pattern NaN really is non-finite (a folded constant would make the next row vacuous)"),
		FMath::IsFinite(QuietNaN));
	TestEqual(
		TEXT("⛔ a NON-FINITE input reads as \"0 seconds\" — it must never reach RoundToInt, whose answer would be meaningless"),
		ASiegePlayerController::WholeSecondsText(QuietNaN).ToString(), FString(TEXT("0 seconds")));

	// ⭐⭐ THE DISPLAY FLOOR, AND IT IS THE ROW THAT PROVES IT IS NOT A ROUNDING CHANGE: a
	// remainder of 0.2 s is a window that is genuinely UP — the caller refuses the card for it —
	// so rounding alone would print a message that contradicts its own refusal in the same breath.
	TestEqual(
		TEXT("⭐ a STRICTLY POSITIVE remainder below half a second still reads \"1 second\", never \"0 seconds\""),
		ASiegePlayerController::WholeSecondsText(0.2f).ToString(), FString(TEXT("1 second")));
	TestEqual(
		TEXT("⛔ and the floor NEVER invents time out of a true zero — 0 stays \"0 seconds\""),
		ASiegePlayerController::WholeSecondsText(0.f).ToString(), FString(TEXT("0 seconds")));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐⭐ — **THE ONE THAT DECIDES THE ROW** (`TASK-989` item (6)(a);
//  `FOG-§10.6`). TWO REFUSALS SEPARATED IN TIME MUST SHOW DIFFERENT NUMBERS.
//
//  ⛔ A single-click test cannot tell a LIVE read from a CACHED one and would
//  report SAFE forever, so this test is deliberately in two halves that fail on
//  different things: the formatter is INJECTIVE across a second of elapsed time
//  (executed), and the number it is handed has exactly ONE source — a live
//  accessor call at the click, stored nowhere (structural).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalLiveValueTest,
	"Siegebound.FogRefusal.TwoRefusalsASecondApartCannotShowTheSameNumber",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalLiveValueTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	// ── HALF ONE (EXECUTED): the same window, clicked twice a second apart ──────────────────
	const float WindowAtFirstClick = 143.f;
	const float WindowAtSecondClick = WindowAtFirstClick - 1.f; // one second of real time later

	const FString FirstMessage = ASiegePlayerController::WholeSecondsText(WindowAtFirstClick).ToString();
	const FString SecondMessage = ASiegePlayerController::WholeSecondsText(WindowAtSecondClick).ToString();

	TestEqual(TEXT("the first click reads the live remainder as \"143 seconds\""), FirstMessage, FString(TEXT("143 seconds")));
	TestEqual(TEXT("the second click, one second later, reads \"142 seconds\""), SecondMessage, FString(TEXT("142 seconds")));
	TestNotEqual(
		TEXT("⭐⭐⭐ TWO REFUSALS ONE SECOND APART PRODUCE DIFFERENT TEXT — a formatter that ignored ")
		TEXT("its argument, or returned a static \"Bright Sun is still active\", is RED here and only here"),
		FirstMessage, SecondMessage);

	// A wider separation, because a formatter could in principle be injective by accident on one
	// pair (e.g. by embedding a call counter) and this pins it to the VALUE.
	TestNotEqual(
		TEXT("⛔ and 83 s cannot read the same as 143 s — the text tracks the NUMBER, not the click"),
		ASiegePlayerController::WholeSecondsText(83.f).ToString(),
		ASiegePlayerController::WholeSecondsText(143.f).ToString());
	TestEqual(
		TEXT("⛔ while the SAME remainder is stable — the formatter is a function of its argument and nothing else"),
		ASiegePlayerController::WholeSecondsText(143.f).ToString(),
		ASiegePlayerController::WholeSecondsText(143.f).ToString());

	// ── HALF TWO (STRUCTURAL): the number has exactly ONE source, and it is a LIVE call ──────
	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⚠️⚠️ RE-POINTED 1 ⇒ 2 BY ⭐ `TASK-991`, ⛔ NOT DELETED — and the CLAIM is unchanged, which is
	// why it moved rather than went away. `TASK-989` wrote these as "1" when this entry raised ONE
	// prevention refusal; the sun-on-sun sibling raises a SECOND, and it reads its OWN live
	// remainder for its own `X`. The claim being made is ⛔ "ONE LIVE READ PER REFUSAL, and no
	// second, older read anywhere" — ⛔ never "one read in the file", which was only ever the
	// arithmetic of that claim with one refusal on the board. ⭐ The per-refusal half is pinned
	// EXACTLY (the local-binding rows below, plus `TASK-991`'s own region rows), so a build that
	// put both reads inside one refusal and none in the other dies there rather than here.
	TestEqual(
		TEXT("⛔⛔ the remainder is read EXACTLY ONCE PER REFUSAL in the card-play entry — two prevention ")
		TEXT("refusals, two live reads, so each number shown IS the number its refusal was decided on"),
		CountOccurrencesInCode(PlayBody, TEXT("GetFogPreventionSecondsRemaining()")), 2);
	TestEqual(
		TEXT("⛔ and those are the ONLY reads in the whole controller — there is no third, older read anywhere in this file"),
		CountOccurrencesInCode(ControllerSource, TEXT("GetFogPreventionSecondsRemaining()")), 2);

	// ⭐ THE ARGUMENT IS THAT CALL'S VALUE, VERBATIM. Pinning both statements (rather than just
	// the call) is what makes "live" mean live: a build that read the accessor and then formatted
	// something else would satisfy the counts above and die right here.
	TestEqual(
		TEXT("⭐⭐ the live remainder is bound to a LOCAL at the click …"),
		CountOccurrencesInCode(PlayBody, TEXT("const float PreventionSecondsRemaining = FogState->GetFogPreventionSecondsRemaining();")), 1);
	TestEqual(
		TEXT("⭐⭐ … and that same local is what the message formats — no other value reaches the toast"),
		CountOccurrencesInCode(PlayBody, TEXT("WholeSecondsText(PreventionSecondsRemaining)")), 1);

	// ⛔ NOTHING CACHES IT. A member field is the shape a cached remainder actually takes, and it
	// would live in the header, so that is where the ban is measured.
	FString ControllerHeader;
	if (!LoadProjectFile(*this, PlayerControllerH, ControllerHeader))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔⛔ the controller declares NO stored prevention remainder — a cached value is stale by ")
		TEXT("exactly the elapsed duration, which is the defect this whole row exists to prevent"),
		CountOccurrencesInCode(ControllerHeader, TEXT("PreventionSeconds")), 0);
	TestEqual(
		TEXT("⛔ and no fog-prevention state of any other spelling is mirrored onto the controller — ")
		TEXT("`AFogVolume` is the ONE state object (FOG-§10.1)"),
		CountOccurrencesInCode(ControllerHeader, TEXT("FogPrevent")), 0);

	// ⚠️ DECLARED, NOT ASSERTED: the true end-to-end claim — click, wait a second, click again,
	// see two different toasts — needs a live world, and there is not one `SpawnActor` anywhere in
	// `Siegebound/Tests/`. The two halves above are the strongest form available without one.
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐ — **ZERO GOLD AND THE CARD STAYS IN HAND, AS TWO SEPARATE
//  ASSERTIONS** (`FOG-§10.6` properties 1 and 2; ✅ `J-F19`).
//
//  ⛔ They are two because a build that REFUNDS THE GOLD BUT EATS THE CARD
//  satisfies exactly half his ruling, and a test written against either half alone
//  passes that broken build.
//  ⭐ Here they hold BY CONSTRUCTION rather than by refund: the refusal returns
//  before the routing switch, and every spend and every consume lives beyond it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalNetZeroTest,
	"Siegebound.FogRefusal.ARefusedFogSpendsZeroGoldAndKeepsTheCardInHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalNetZeroTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⭐ PROPERTY 1 — NO GOLD MOVES. Not "is refunded": never spent. There is nothing in this
	// entry that can move gold, so there is no refund to get wrong and no window in which the
	// player's gold is briefly lower.
	TestEqual(
		TEXT("⭐ PROPERTY 1 (no gold): the card-play entry never SPENDS — `SpendGold` appears zero times in it"),
		CountOccurrencesInCode(PlayBody, TEXT("SpendGold")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 1 (no gold): and it never REFUNDS either — `AddGold` is zero, because there is nothing to give back"),
		CountOccurrencesInCode(PlayBody, TEXT("AddGold")), 0);

	// ⭐ PROPERTY 2 — THE CARD STAYS IN HAND. Both shipped consume doors are absent from this
	// function entirely, so a refusal here cannot reach either.
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): `ConfirmPlayFromHand` — the placement/targeting consume door — is not in this entry"),
		CountOccurrencesInCode(PlayBody, TEXT("ConfirmPlayFromHand")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): `ConfirmInstantDraw` — the instant consume door — is not in this entry either"),
		CountOccurrencesInCode(PlayBody, TEXT("ConfirmInstantDraw")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): and it never bins a slot directly — `DiscardFromHand` is absent"),
		CountOccurrencesInCode(PlayBody, TEXT("DiscardFromHand")), 0);

	// ⭐⭐ AND THE REASON BOTH HOLD: the refusal RETURNS BEFORE the routing switch, which is the
	// only way this function reaches a resolver at all. ⛔ Order is the assertion.
	FString BeforeSwitch;
	if (!SubstringBefore(*this, PlayBody, TEXT("switch (Row->CardType)"), BeforeSwitch))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ the BrightSun refusal is raised BEFORE the CardType routing switch — so neither ")
		TEXT("resolver, and therefore neither spend and neither consume, is ever reached"),
		CountOccurrencesInCode(BeforeSwitch, TEXT("CardRefused_BrightSunActive")), 1);

	// ⛔ AND IT IS RAISED BEFORE THE ACCEPT SOUND. A refused play that still plays the card-play
	// stinger tells the player the opposite of what the toast says, and the sound is the only
	// thing in this function that fires between the gold gate and the switch.
	FString BeforeRefusal;
	if (!SubstringBefore(*this, PlayBody, TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunActive\""), BeforeRefusal))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔ the ACCEPT stinger is NOT played before the refusal — `CardPlaySoundPath` comes after it"),
		CountOccurrencesInCode(BeforeRefusal, TEXT("CardPlaySoundPath")), 0);

	// ⭐ THE SHIPPED PRECEDENCE IS PRESERVED: §3.5's spec order puts the gold refusal ahead of
	// every type refusal, so an unaffordable Fog still says "Not enough gold" rather than
	// advertising a window the player could not have used anyway.
	TestEqual(
		TEXT("⭐ the affordability gate still outranks this refusal (§3.5 spec order) — `CanAfford` runs first"),
		CountOccurrencesInCode(BeforeRefusal, TEXT("CanAfford")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — **`Fog` ONLY** (✅ `J-F26`, closed), AND THE **NEGATIVE CONTROL**
//  (`TASK-989` item (6)(e)): with no window up, `Fog` must play NORMALLY.
//
//  ⛔ A refusal that fires unconditionally would satisfy every other test in this
//  file. This is the one that goes red for it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalScopeTest,
	"Siegebound.FogRefusal.PreventionRefusesFogAloneAndOnlyWhileTheWindowIsUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalScopeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⭐ `J-F26` — the refusal is gated on the DATA (the one effect that raises fog), never on a
	// CardID literal, and on nothing else. A second effect gate here would silently disable a card
	// he never mentioned.
	TestEqual(
		TEXT("⭐ `J-F26`: exactly ONE effect gates this refusal — `ESpellEffect::FogCover`, the effect that raises fog"),
		CountOccurrencesInCode(PlayBody, TEXT("ESpellEffect::FogCover")), 1);
	TestEqual(
		TEXT("⭐ `J-F26`: and the refusal itself exists exactly ONCE in the whole controller — one call site, one format string"),
		CountOccurrencesInCode(ControllerSource, TEXT("CardRefused_BrightSunActive")), 1);

	// ⛔ THE NEGATIVE CONTROL, IN THE SHAPE THAT CAN ACTUALLY FAIL. Everything between the effect
	// gate and the refusal text must contain the POSITIVE-REMAINDER test; an unconditional refusal
	// deletes that substring and this row dies.
	const int32 GateIndex = PlayBody.Find(TEXT("if (Row->SpellEffect == ESpellEffect::FogCover)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 RefusalIndex = PlayBody.Find(TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunActive\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	if (GateIndex == INDEX_NONE || RefusalIndex == INDEX_NONE || RefusalIndex <= GateIndex)
	{
		AddError(TEXT("⛔ The effect gate and its refusal are not both present in order — the negative-control probe is stale, so it FAILS."));
		return false;
	}

	const FString GuardedRegion = PlayBody.Mid(GateIndex, RefusalIndex - GateIndex);
	TestEqual(
		TEXT("⛔⛔ NEGATIVE CONTROL: the refusal is reached ONLY when the live remainder is strictly ")
		TEXT("positive — with no window up, `Fog` falls through and plays normally"),
		CountOccurrencesInCode(GuardedRegion, TEXT("PreventionSecondsRemaining > 0.f")), 1);
	TestEqual(
		TEXT("⛔ and only when a fog-state actor already EXISTS — no volume means no window, which is the honest answer"),
		CountOccurrencesInCode(GuardedRegion, TEXT("AFogVolume::Find(GetWorld())")), 1);

	// ⛔ A REFUSAL PRE-CHECK MAY NOT SPAWN. `FindOrSpawn` is the WRITE door and it belongs to the
	// resolver; reaching for it here would create the fog actor as a side effect of being told no.
	TestEqual(
		TEXT("⛔ the controller never uses the WRITE door — `AFogVolume::FindOrSpawn(` is absent from this file"),
		CountOccurrencesInCode(ControllerSource, TEXT("AFogVolume::FindOrSpawn(")), 0);

	// ⛔ THE ROUTING SWITCH IS STILL THERE, AFTER THE GUARD. Without this row a build that ended
	// the function at the refusal would pass everything above: the card would be refused, no gold
	// would move, no card would be consumed — and `Fog` would be unplayable forever.
	const int32 SwitchIndex = PlayBody.Find(TEXT("switch (Row->CardType)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	TestTrue(
		TEXT("⛔⛔ NEGATIVE CONTROL: the CardType routing switch still follows the guard — the guard ")
		TEXT("SKIPS, it does not terminate the card-play entry"),
		SwitchIndex != INDEX_NONE && SwitchIndex > RefusalIndex);

	// ⭐⭐ THE FENCE, MADE EXECUTABLE: `TASK-989` was told not to touch the `GoldSteal` routing
	// predicate, because `TASK-1018` must fix it by CONSUMING `TASK-999`'s delivery-derived
	// answer rather than re-deriving one here. This row is that promise, kept in a form the gate
	// can check instead of taking on trust.
	//
	// ⛔⛔ RE-POINTED BY ⭐ `TASK-1018` (2026-09-04) — ⛔ MOVED, ⛔ NEVER DELETED, exactly as the
	// instruction that used to sit here demanded. ⛔ A DELETED PIN IS A GUARD THAT SILENTLY STOPS
	// GUARDING, and this file's own suite would have gone GREENER for the deletion.
	//   ⛔ OLD NEEDLE: `if (Row->SpellEffect == ESpellEffect::GoldSteal)`  — ⛔ OLD VALUE 1
	//   ✅ NEW NEEDLE: `USpellLibrary::SpellRequiresAiming(*Row)`          — ✅ NEW VALUE 1
	// ⭐ THE CLAIM IS UNCHANGED, WHICH IS THE WHOLE REASON IT MOVES INSTEAD OF DYING: "the spell
	// routing decision is made in EXACTLY ONE PLACE inside this entry, and this row dies if a
	// second one appears or the only one disappears." Only the SPELLING of that one place changed
	// — from an effect-enum comparison (a blacklist of one, which had already failed twice) to a
	// call into the ONE derivation of "does this spell have a reticle?".
	// ⛔ THE COUNT IS STILL 1, NOT 2, AND THAT IS LOAD-BEARING: `EnterTargetingMode` holds the
	// OTHER call to the same predicate, but it is a DIFFERENT function and is therefore outside
	// `PlayBody`. A 2 here would mean this entry asks the routing question twice.
	// ⛔ The comment lines above quoting the OLD guard cost nothing: `CountOccurrencesInCode`
	// skips comment lines, which is exactly why the old needle now measures 0 and this one is the
	// live claim.
	TestEqual(
		TEXT("⭐⭐ the spell routing predicate is DERIVED and lives in exactly ONE place in this entry — ")
		TEXT("`USpellLibrary::SpellRequiresAiming(*Row)`, once (⛔ TASK-1018 re-pointed this row from the ")
		TEXT("`== ESpellEffect::GoldSteal` blacklist it replaced; ⛔ a second occurrence means the question ")
		TEXT("is being asked twice, a zero means the derivation was inlined back into a list)"),
		CountOccurrencesInCode(PlayBody, TEXT("USpellLibrary::SpellRequiresAiming(*Row)")), 1);

	// ⛔⛔ AND THE BAN, PAIRED WITH IT — a re-point alone would pass if somebody kept BOTH: the
	// derived call for the fog cards and the old blacklist beside it for something else.
	// ⛔ `SC-§75`(B): an exclusion list anywhere in this routing is an automatic fail.
	TestEqual(
		TEXT("⛔⛔ THE BLACKLIST IS GONE, NOT MERELY BYPASSED: not one executable `ESpellEffect::GoldSteal` ")
		TEXT("comparison survives in the whole controller. ⛔ It failed OPEN and had ALREADY failed TWICE — ")
		TEXT("once per new no-reticle spell — so a THIRD occurrence is what this row exists to prevent."),
		CountOccurrencesInCode(ControllerSource, TEXT("SpellEffect == ESpellEffect::GoldSteal")), 0);

	// ⚠️ DELIBERATELY NOT PINNED, and the reason is recorded so a future reader does not "complete"
	// the census: neither the total `ESpellEffect::` count in this function nor a zero for
	// `ESpellEffect::FogClear` is asserted. `TASK-1018` changes the first and `TASK-991` adds the
	// second — a pin on either would go red on a CORRECT successor diff, which is a pin that
	// teaches the next author to weaken tests.
	// ⭐ AND THE PREDICTION LANDED: ⭐ `TASK-991` shipped the `FogClear` gate, and pins it at ONE in
	// TEST 7 below — where it is an assertion about the SUN-ON-SUN refusal rather than a stale zero
	// left behind here. ⛔ The `ESpellEffect::` total is still deliberately unpinned (`TASK-1018`).
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐ — **THE SURFACE, MEASURED RATHER THAN ASSUMED** (✅ `J-F25`).
//  The message rides the ONE shipped refusal channel every other refusal uses.
//  ⛔ No new path, ⛔ no new broadcast, ⛔ no new widget, ⛔ no new toast.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalSurfaceTest,
	"Siegebound.FogRefusal.TheMessageRidesTheOneShippedRefusalSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalSurfaceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⭐ THE PATTERN COPIED IS A REAL, SHIPPED ONE — verified at source rather than remembered.
	// `CardRefused_MaxStacks` is the precedent the row was told to copy: `FText::Format` at the
	// call site into `RefuseCardPlay`. If it ever stops being that, this row says so.
	TestEqual(
		TEXT("SELF-CHECK: the copied precedent still exists — a formatted refusal already shipped before this row"),
		CountOccurrencesInCode(ControllerSource, TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_MaxStacks\", \"{0} at max stacks\")")), 1);
	// ⚠️ RE-POINTED 1 ⇒ 2 BY ⭐ `TASK-991`, ⛔ NOT DELETED. The claim is "every prevention refusal in
	// this entry rides the shipped formatted-refusal idiom"; the sun-on-sun sibling is the second
	// one, and it rides the same idiom, so the count follows the number of refusals.
	TestEqual(
		TEXT("⭐ BOTH prevention refusals use that same idiom — `RefuseCardPlay` + `FText::Format` at the call site"),
		CountOccurrencesInCode(PlayBody, TEXT("RefuseCardPlay(CardID, FText::Format(")), 2);

	// ⛔ NO NEW SURFACE. The refusal must not reach for a delegate, a widget or a viewport of its
	// own — the whole point of `J-F25` is that this rides what already ships.
	TestEqual(
		TEXT("⛔ the card-play entry broadcasts no refusal delegate directly — it goes through the shipped choke point"),
		CountOccurrencesInCode(PlayBody, TEXT("OnCardRefused.Broadcast")), 0);
	TestEqual(
		TEXT("⛔ and it creates no widget of its own"),
		CountOccurrencesInCode(PlayBody, TEXT("CreateWidget")), 0);

	// ⭐ THE CHOKE POINT, MEASURED: `RefuseCardPlay` fans out to BOTH shipped refusal delegates,
	// so this message reaches the same two listeners "Not enough gold" does.
	FString RefuseBody;
	if (!ExtractFunctionBody(*this, ControllerSource, RefuseCardPlaySignature, RefuseBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐ the surface, measured (1/3): `RefuseCardPlay` fires the M1 card-context delegate"),
		CountOccurrencesInCode(RefuseBody, TEXT("OnCardPlayRefused.Broadcast")), 1);
	TestEqual(
		TEXT("⭐ the surface, measured (2/3): and the M2 combined refusal channel every refusal shares"),
		CountOccurrencesInCode(RefuseBody, TEXT("BroadcastRefusal(")), 1);

	// ⭐ AND THE LAST HOP, in the widget that actually renders it.
	FString HandWidgetSource;
	if (!LoadProjectFile(*this, CardHandWidgetCpp, HandWidgetSource))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐ the surface, measured (3/3): `UCardHandWidget` forwards it 1:1 to `OnCardRefusedMessage` — ")
		TEXT("the one shipped single-line HUD channel, rendered by WBP_CardHand"),
		CountOccurrencesInCode(HandWidgetSource, TEXT("OnCardRefusedMessage(Reason);")), 1);

	// ⚠️⚠️ REPORTED HERE, ⛔ NOT ACTED ON, AND DELIBERATELY NOT ASSERTED (`J-F25`, closed): the
	// show-then-hide LIFETIME of that message lives in WBP_CardHand — `CardHandWidget.h` calls it
	// "~2 s" and `VID-005` measured it at ≈1.8 s on two independent bursts. That is SHORT for a
	// number the player is meant to read. ⛔ Changing it is a GAME-WIDE UI change and therefore
	// Jonathan's call, not this lane's — and it is not assertable from C++ in any case, because
	// the timer is in a Blueprint asset.
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐ — **THE SHARED FORMATTER IS SHARED, HAS A CALLER, AND IS NOT A
//  MESSAGE BUILDER** (`SC-§40` cl. 2; `TASK-989` item (5)/(5a); `FOG-§10.7` (A)).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRefusalFormatterSeamTest,
	"Siegebound.FogRefusal.TheSecondsFormatterIsAScalarSeamWithALiveCaller",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRefusalFormatterSeamTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString ControllerHeader;
	if (!LoadProjectFile(*this, PlayerControllerH, ControllerHeader))
	{
		return false;
	}

	// ⛔ NOT DEAD SURFACE: the definition plus at least one caller. ⭐ Pinned as a MINIMUM rather
	// than an exact count on purpose — `TASK-991` adds two more calls, and a `== 2` here would go
	// red on a correct successor diff.
	TestTrue(
		TEXT("⛔ `SC-§40` cl. 2: the shared formatter is DEFINED and CALLED — it ships with a live caller, not as a seam waiting for one"),
		CountOccurrencesInCode(ControllerSource, TEXT("WholeSecondsText(")) >= 2);
	TestEqual(
		TEXT("⛔ and it is declared exactly once — ONE definition of how a countdown is spelled"),
		CountOccurrencesInCode(ControllerHeader, TEXT("static FText WholeSecondsText(float Seconds);")), 1);

	// ⭐⭐ IT IS A FORMATTER, NOT A MESSAGE BUILDER — the distinction `TASK-991` depends on. The
	// refusal's own wording stays at the CALL SITE, so the sibling refusal (which carries TWO
	// values, not one) cannot reuse a one-arity builder and end up with an optional parameter that
	// is dead half the time.
	FString FormatterBody;
	if (!ExtractFunctionBody(*this, ControllerSource, WholeSecondsTextSignature, FormatterBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ the formatter knows NOTHING about Bright Sun — the message text lives at the call site, ")
		TEXT("so `TASK-991`'s two-value sentence can reuse the scalar without inheriting this row's wording"),
		CountOccurrencesInCode(FormatterBody, TEXT("BrightSun")), 0);
	TestEqual(
		TEXT("⛔ and nothing about the card either — it takes a scalar and returns text, and that is its whole contract"),
		CountOccurrencesInCode(FormatterBody, TEXT("CardRefused_")), 0);

	// ⛔ PURE: no world, no actor, no member state. It is a `static` on the controller for the same
	// reason the seven shipped placement statics are — a decision locked inside a world-bound
	// member function is a decision nobody can test, and this one IS the units-and-rounding ruling.
	TestEqual(
		TEXT("⛔ the formatter touches no world"),
		CountOccurrencesInCode(FormatterBody, TEXT("GetWorld")), 0);
	TestEqual(
		TEXT("⛔ and no fog state — it never reads the remainder it is handed"),
		CountOccurrencesInCode(FormatterBody, TEXT("AFogVolume")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⭐⭐⭐ TASK-991 — THE SUN-ON-SUN CONDITIONAL REFUSAL (`J-F18`; law
//  `FOG-§10.7` (A), `FOG-§10.6`, `SC-§37`, `SC-§40` cl. 2).
//
//  📌 His words: *"the timer gets RESET to whatever the new time would be under the
//  new cast, UNLESS that new time would be LESS than the current time, then the
//  player will just get a message that says 'using bright sun right now would
//  reduce fog prevention time from "x" time to "y" time' … and the player is
//  basically prevented from playing the card."*
//
//  ⛔⛔ THE FOUR ROWS BELOW EXIST BECAUSE AN **UNCONDITIONAL** REFUSAL SATISFIES
//  THAT SENTENCE WORD FOR WORD, passes every "no gold / card kept / two values"
//  assertion, and DESTROYS THE CARD — `BrightSun` could never be re-cast at all.
//  Test 7 is the row that goes red for it.
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 ⭐⭐⭐ — **IT IS A BRANCH, NOT A POLICY: SHORTER REFUSES, LONGER FALLS
//  THROUGH AND CASTS.** ⛔ Not `max`, ⛔ not a refresh, ⛔ not a blanket refuse.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSunOnSunConditionalTest,
	"Siegebound.FogRefusal.SunOnSunRefusesONLYWhenTheNewWindowWouldBeShorter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSunOnSunConditionalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	FString SunRegion;
	if (!ExtractSunOnSunRegion(*this, PlayBody, SunRegion))
	{
		return false;
	}

	// ⭐⭐⭐ THE CONDITION ITSELF, AND IT IS THE ROW. Everything between the effect gate and the
	// message must contain the comparison; an unconditional refusal deletes that substring and
	// dies right here, having passed everything else in this file.
	TestEqual(
		TEXT("⭐⭐⭐ THE REFUSAL IS CONDITIONAL: it is reached ONLY when the would-be window is ")
		TEXT("shorter than what is left — the LONGER case never reaches the message at all"),
		CountOccurrencesInCode(SunRegion, TEXT("if (WouldBeWindowSeconds < RemainingWindowSeconds)")), 1);

	// ⛔⛔ THE ONE INSTRUMENT IN THIS SUITE THAT CAN SEE THE RULING AT ALL. `max(remaining, new)`
	// and "reset if longer" produce the IDENTICAL remaining time in the longer case, so ⛔ NO test
	// of the resulting DURATION could ever tell them apart. They diverge only in the shorter case —
	// where `max` silently keeps the timer while BILLING 60 gold and EATING the card.
	// ⭐ `TASK-982` asserts the same zero inside `ApplyBrightSun`; this is the entry-side half, and
	// the two are not duplicates because a `max` could have been introduced at either end.
	TestEqual(
		TEXT("⛔⛔ `J-F18` IS A BRANCH, ⛔ NOT `max`: the card-play entry contains no `FMath::Max` anywhere"),
		CountOccurrencesInCode(PlayBody, TEXT("FMath::Max")), 0);

	// ⚠️ THE BOUNDARY IS A DECLARED DEFAULT, NOT HIS WORD — he wrote "LESS than", so EQUAL RESETS.
	// Pinned as the ABSENCE of the non-strict form so the two readings cannot both be true.
	TestEqual(
		TEXT("⚠️ STRICT `<` (declared default): EQUAL RESETS, so the non-strict form is absent"),
		CountOccurrencesInCode(SunRegion, TEXT("WouldBeWindowSeconds <= RemainingWindowSeconds")), 0);

	// ⛔⛔ THE NEGATIVE CONTROL THAT DECIDES THE ROW: the routing switch STILL FOLLOWS the refusal.
	// Without this, a build that refused every `BrightSun` — no gold moved, card kept, two live
	// values in the message — would pass every other assertion here while making the card
	// permanently uncastable, which is the one outcome his ruling forbids.
	const int32 MessageIndex = PlayBody.Find(TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunWouldShorten\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 SwitchIndex = PlayBody.Find(TEXT("switch (Row->CardType)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	TestTrue(
		TEXT("⛔⛔ NEGATIVE CONTROL: the CardType routing switch still follows the sun-on-sun guard — ")
		TEXT("the guard SKIPS on a LONGER window, it does not terminate the card-play entry"),
		MessageIndex != INDEX_NONE && SwitchIndex != INDEX_NONE && SwitchIndex > MessageIndex);

	// ⭐ AND THE LONGER HALF IS NOT REIMPLEMENTED HERE. The reset to the new window belongs to
	// `AFogVolume::ApplyBrightSun` (`TASK-982` item 7a) — this entry writes no expiry of any kind,
	// which is what makes "the stored expiry stays bit-identical on a refusal" true by construction
	// rather than by a compensating write.
	TestEqual(
		TEXT("⛔ the entry never stamps a prevention expiry — the LONGER branch is the state object's, not this one's"),
		CountOccurrencesInCode(PlayBody, TEXT("ApplyBrightSun")), 0);
	TestEqual(
		TEXT("⛔ and it writes no fog deadline of any spelling — `AFogVolume` is the ONE writer (FOG-§10.1)"),
		CountOccurrencesInCode(PlayBody, TEXT("FogPreventedUntilTimeSeconds")), 0);

	// ⛔ `BrightSun` ONLY, and gated on the DATA rather than a CardID literal — the `FogCover`
	// sibling's rule, applied to the effect that OPENS the window instead of the one that raises fog.
	TestEqual(
		TEXT("⭐ exactly ONE effect gates this refusal — `ESpellEffect::FogClear`, the effect `BrightSun` carries"),
		CountOccurrencesInCode(PlayBody, TEXT("ESpellEffect::FogClear")), 1);
	TestEqual(
		TEXT("⛔ and never a CardID literal — a data gate, so a renamed card does not silently stop being refused"),
		CountOccurrencesInCode(PlayBody, TEXT("TEXT(\"BrightSun\")")), 0);

	// ⛔ THE READ DOOR, NEVER THE WRITE DOOR: a refusal pre-check may not spawn the state actor as
	// a side effect of saying no. (File-wide, so it also covers the sibling above.)
	TestEqual(
		TEXT("⛔ the sun-on-sun pre-check uses the READ door `Find` …"),
		CountOccurrencesInCode(SunRegion, TEXT("AFogVolume::Find(GetWorld())")), 1);
	TestEqual(
		TEXT("⛔ … and the controller still never touches `FindOrSpawn` — the resolver's own call still covers J-F17's first-cast-of-the-match case"),
		CountOccurrencesInCode(ControllerSource, TEXT("AFogVolume::FindOrSpawn(")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 8 ⭐⭐ — **`Y` IS A FULL CARD EFFECT, COMPUTED LIVE AND THEN THROWN AWAY.**
//  ⛔ The formula is NOT duplicated, and the card is NOT cast to find out whether
//  to cast it: `TASK-982`'s DURATION accessor is called from OUTSIDE the cast path.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSunOnSunLiveWindowTest,
	"Siegebound.FogRefusal.SunOnSunComputesTheWouldBeWindowLiveAndNeverDuplicatesTheFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSunOnSunLiveWindowTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	// ── EXECUTED: TWO PERCHES, TWO WINDOWS, TWO SENTENCES ───────────────────────────────────
	// ⛔ The five numbers below are ARGUMENTS to a pure function — ⛔ NOT a second copy of the
	// shipped tunables. The shipped VALUES are `Tests/SiegeBrightSunTest.cpp`'s subject and this
	// file asserts nothing about them. What is asserted here is the SHAPE this refusal depends on:
	// ⭐ a HIGHER hero yields a LONGER would-be window, therefore a DIFFERENT `Y`, therefore a
	// DIFFERENT sentence. ⛔ A `Y` sampled once and reused would read the same from both perches,
	// and a fixed-height test could never tell the two apart (`SC-§37`).
	const float GroundWindow = AFogVolume::BrightSunWindowSeconds(/*HeroZ*/ 0.f, /*Ground*/ 0.f, /*Base*/ 120.f, /*Bonus*/ 60.f, /*Step*/ 1524.f);
	const float TowerWindow = AFogVolume::BrightSunWindowSeconds(/*HeroZ*/ 3200.f, /*Ground*/ 0.f, /*Base*/ 120.f, /*Bonus*/ 60.f, /*Step*/ 1524.f);

	TestTrue(
		TEXT("⭐⭐ a higher perch would open a STRICTLY LONGER window — which is the whole reason the ")
		TEXT("refusal has to be conditional rather than flat"),
		TowerWindow > GroundWindow);
	TestNotEqual(
		TEXT("⭐⭐ ⇒ two refusals from two DIFFERENT HEIGHTS cannot show the same `Y` — the sentence ")
		TEXT("tracks the hero's altitude at the click"),
		ASiegePlayerController::WholeSecondsText(TowerWindow).ToString(),
		ASiegePlayerController::WholeSecondsText(GroundWindow).ToString());

	// ⭐ AND `X` IS LIVE TOO, re-asserted here rather than borrowed from test 2 because THIS call
	// site is new: two refusals a second apart cannot show the same `X` either.
	TestNotEqual(
		TEXT("⭐ two sun-on-sun refusals a second apart cannot show the same `X` — the remainder is re-read at each click"),
		ASiegePlayerController::WholeSecondsText(143.f).ToString(),
		ASiegePlayerController::WholeSecondsText(142.f).ToString());

	// ── STRUCTURAL: WHICH CALL FEEDS WHICH, AND WHAT IS ABSENT ──────────────────────────────
	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	FString SunRegion;
	if (!ExtractSunOnSunRegion(*this, PlayBody, SunRegion))
	{
		return false;
	}

	// ⭐⭐ THE SEAM IS USED, ONCE, AND ITS RESULT IS WHAT THE MESSAGE CARRIES. `TASK-982` shipped
	// `GetBrightSunWindowSeconds` public/const/side-effect-free EXACTLY so this row could reach it
	// without casting the card.
	TestEqual(
		TEXT("⭐⭐ `Y` comes from `TASK-982`'s DURATION accessor, called ONCE, from OUTSIDE the cast path"),
		CountOccurrencesInCode(SunRegion, TEXT("const float WouldBeWindowSeconds = FogState->GetBrightSunWindowSeconds(CasterTeam);")), 1);
	TestEqual(
		TEXT("⛔ and that is its only call in the whole controller — no second, older window anywhere in this file"),
		CountOccurrencesInCode(ControllerSource, TEXT("GetBrightSunWindowSeconds(")), 1);
	TestEqual(
		TEXT("⭐⭐ `X` is this refusal's OWN live read, bound to its own local — not the sibling refusal's value reused"),
		CountOccurrencesInCode(SunRegion, TEXT("const float RemainingWindowSeconds = FogState->GetFogPreventionSecondsRemaining();")), 1);
	TestEqual(
		TEXT("⛔ exactly one remainder read inside this guard — one read, one value, one decision"),
		CountOccurrencesInCode(SunRegion, TEXT("GetFogPreventionSecondsRemaining()")), 1);

	// ⛔⛔ THE BLOCKER-GRADE ABSENCE: the height formula is NOT duplicated here. Two copies drift,
	// and the moment they do the message starts LYING about the effect it is refusing to apply.
	TestEqual(
		TEXT("⛔⛔ the `floor` at the heart of the height formula is absent from this controller — one copy, in `AFogVolume`"),
		CountOccurrencesInCode(ControllerSource, TEXT("FMath::FloorToFloat")), 0);
	TestEqual(
		TEXT("⛔ and no integer-floor spelling of it either"),
		CountOccurrencesInCode(ControllerSource, TEXT("FMath::FloorToInt")), 0);
	TestEqual(
		TEXT("⛔ the PURE formula static is never called from here — the controller asks the ACTOR, which samples the hero"),
		CountOccurrencesInCode(ControllerSource, TEXT("AFogVolume::BrightSunWindowSeconds(")), 0);

	// ⛔ AND NOT ONE OF THE FOUR TUNABLES IS MIRRORED HERE. A hand-typed `120`/`60`/`1524`/datum in
	// this file would be the second site `FOG-§9.5` bans, retunable independently of the card.
	TestEqual(
		TEXT("⛔ the base duration is not mirrored onto the controller"),
		CountOccurrencesInCode(ControllerSource, TEXT("BrightSunBaseDurationSeconds")), 0);
	TestEqual(
		TEXT("⛔ nor the per-step bonus"),
		CountOccurrencesInCode(ControllerSource, TEXT("BrightSunBonusSecondsPerStep")), 0);
	TestEqual(
		TEXT("⛔ nor the height step"),
		CountOccurrencesInCode(ControllerSource, TEXT("BrightSunHeightStepUU")), 0);
	TestEqual(
		TEXT("⛔ nor the flat-grass datum (`J-F13`) — the controller reads no height at all"),
		CountOccurrencesInCode(ControllerSource, TEXT("ArenaGroundReferenceZUU")), 0);

	// ⛔ NOTHING CACHES EITHER VALUE. A cached window is the shape this defect actually takes, and
	// it would live in the header, so that is where the ban is measured.
	FString ControllerHeader;
	if (!LoadProjectFile(*this, PlayerControllerH, ControllerHeader))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔⛔ the controller declares NO stored would-be window — `Y` is computed at the click and thrown away"),
		CountOccurrencesInCode(ControllerHeader, TEXT("WouldBeWindow")), 0);
	TestEqual(
		TEXT("⛔ and no BrightSun window state of any other spelling is mirrored onto it"),
		CountOccurrencesInCode(ControllerHeader, TEXT("BrightSunWindow")), 0);

	// ⚠️ DECLARED, NOT ASSERTED: that the hero's REAL altitude reaches `Y` needs a live world and a
	// spawned hero, and there is not one `SpawnActor` anywhere in `Siegebound/Tests/`. The executed
	// half above proves the window varies with height; the structural half proves the call site asks
	// the LIVE accessor and stores nothing. `TASK-982` and `TASK-989` declared the identical gap.
	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 9 ⭐⭐ — **THE THREE PROPERTIES, BY NAME, NEVER ONE STANDING FOR ANOTHER**
//  (`FOG-§10.7` (A); `FOG-§10.6`): ⛔ ZERO gold · ⛔ card NOT consumed · ⛔ BOTH
//  live values in the message.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSunOnSunNetZeroTest,
	"Siegebound.FogRefusal.ARefusedBrightSunSpendsZeroGoldKeepsTheCardAndShowsBothValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSunOnSunNetZeroTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⭐ PROPERTIES 1 AND 2 HOLD BY THE SAME CONSTRUCTION AS THE SIBLING REFUSAL'S — this entry
	// neither spends nor confirms — but the ORDER that makes them true is what is asserted here,
	// because the sun-on-sun guard sits LOWER in the function and could in principle have been
	// placed past a spend that its sibling precedes.
	FString BeforeSunRefusal;
	if (!SubstringBefore(*this, PlayBody, TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunWouldShorten\""), BeforeSunRefusal))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐ PROPERTY 1 (no gold): nothing has been SPENT by the time this refusal is raised"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("SpendGold")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 1 (no gold): and nothing is REFUNDED either — there is nothing to give back"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("AddGold")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): the placement/targeting consume door has not been reached"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("ConfirmPlayFromHand")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): nor the instant consume door"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("ConfirmInstantDraw")), 0);
	TestEqual(
		TEXT("⭐ PROPERTY 2 (card kept): and no slot is binned directly"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("DiscardFromHand")), 0);
	TestEqual(
		TEXT("⛔ the ACCEPT stinger has not played either — a refused play must not sound like an accepted one"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("CardPlaySoundPath")), 0);
	TestEqual(
		TEXT("⭐ and the affordability gate still outranks it (§3.5 spec order) — `CanAfford` runs first"),
		CountOccurrencesInCode(BeforeSunRefusal, TEXT("CanAfford")), 1);

	// ⭐⭐ AND THE REASON BOTH HOLD: the refusal returns BEFORE the routing switch, the only way this
	// function reaches a resolver at all.
	FString BeforeSwitch;
	if (!SubstringBefore(*this, PlayBody, TEXT("switch (Row->CardType)"), BeforeSwitch))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ the sun-on-sun refusal is raised BEFORE the CardType routing switch — so no resolver, ")
		TEXT("and therefore no spend and no consume, is ever reached"),
		CountOccurrencesInCode(BeforeSwitch, TEXT("CardRefused_BrightSunWouldShorten")), 1);

	// ⭐⭐⭐ PROPERTY 3 — THE MESSAGE CARRIES **BOTH** LIVE VALUES, IN HIS ORDER. ⛔ This is the pin
	// that catches the one defect a reader cannot see: swapping the two arguments produces "would
	// reduce fog prevention time from 83 seconds to 143 seconds", which reads as an INCREASE while
	// refusing the card. The trailing punctuation is what makes each row identify a SLOT: `{0}` is
	// the argument followed by a comma, `{1}` is the one that closes the call.
	TestEqual(
		TEXT("⭐⭐ `{0}` is `X`, the CURRENT time left — his 'x'"),
		CountOccurrencesInCode(PlayBody, TEXT("WholeSecondsText(RemainingWindowSeconds),")), 1);
	TestEqual(
		TEXT("⭐⭐ `{1}` is `Y`, the NEW window under the current height calculation — his 'y'"),
		CountOccurrencesInCode(PlayBody, TEXT("WholeSecondsText(WouldBeWindowSeconds)));")), 1);
	TestEqual(
		TEXT("⭐⭐ and the sentence is HIS sentence, with both slots in his order — 'from x to y'"),
		CountOccurrencesInCode(PlayBody, TEXT("would reduce fog prevention time from {0} to {1}")), 1);

	// ⛔ ONE call site, ONE format string — the sibling refusal's rule, kept.
	TestEqual(
		TEXT("⛔ the sun-on-sun refusal exists exactly ONCE in the whole controller"),
		CountOccurrencesInCode(ControllerSource, TEXT("CardRefused_BrightSunWouldShorten")), 1);
	TestEqual(
		TEXT("⛔ and the sibling `FogCover` refusal is untouched by this row — still exactly one"),
		CountOccurrencesInCode(ControllerSource, TEXT("CardRefused_BrightSunActive")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 10 ⭐⭐ — **THE ARITY TRAP: THE SCALAR FORMATTER IS REUSED (CALLED TWICE),
//  ⛔ NOT COPIED AND ⛔ NOT GENERALISED INTO A MESSAGE BUILDER** (`SC-§40` cl. 2;
//  `FOG-§10.7` (A)).
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSunOnSunFormatterReuseTest,
	"Siegebound.FogRefusal.TheTwoValueSentenceReusesTheOneSharedFormatterTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSunOnSunFormatterReuseTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRefusalFixture;

	FString ControllerSource;
	if (!LoadProjectFile(*this, PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString ControllerHeader;
	if (!LoadProjectFile(*this, PlayerControllerH, ControllerHeader))
	{
		return false;
	}

	FString PlayBody;
	if (!ExtractFunctionBody(*this, ControllerSource, PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ⛔⛔ THERE IS STILL EXACTLY ONE FORMATTER. A second one is how the two refusals would come to
	// spell a countdown differently, and it is the automatic failure this row was warned about.
	TestEqual(
		TEXT("⛔⛔ ONE declaration of how a countdown is spelled — `TASK-991` added no second formatter"),
		CountOccurrencesInCode(ControllerHeader, TEXT("static FText WholeSecondsText(float Seconds);")), 1);
	TestEqual(
		TEXT("⛔ and ONE definition of it in the whole controller"),
		CountOccurrencesInCode(ControllerSource, TEXT("FText ASiegePlayerController::WholeSecondsText(float Seconds)")), 1);

	// ⭐ CALLED TWICE BY THIS SENTENCE, ONCE BY ITS SIBLING ⇒ three calls plus the definition line.
	TestEqual(
		TEXT("⭐⭐ the sun-on-sun sentence calls the SHARED scalar formatter TWICE — once per value"),
		CountOccurrencesInCode(PlayBody, TEXT("WholeSecondsText(")), 3);

	// ⭐⭐ AND IT IS STILL A FORMATTER, NOT A BUILDER. `TASK-989`'s one-value message was NOT
	// generalised to span two arities: had it been, it would carry an optional second value that is
	// dead for every `FogCover` refusal — the exact dead surface `SC-§40` cl. 2 bans. The proof is
	// that BOTH wordings still live at their OWN call sites, each with its own key.
	TestEqual(
		TEXT("⛔ the one-value sibling still owns its own inline wording — it was reused, not rewritten"),
		CountOccurrencesInCode(ControllerSource, TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunActive\", \"Bright Sun is still up for {0}\")")), 1);
	TestEqual(
		TEXT("⛔ and the two-value sentence owns its own, at its own call site — two keys, two arities, ONE formatter"),
		CountOccurrencesInCode(PlayBody, TEXT("NSLOCTEXT(\"Siegebound\", \"CardRefused_BrightSunWouldShorten\"")), 1);

	// ⛔ THE FORMATTER ITSELF IS UNCHANGED AND STILL KNOWS NOTHING ABOUT EITHER CARD — which is what
	// let this row consume it without inheriting the sibling's wording.
	FString FormatterBody;
	if (!ExtractFunctionBody(*this, ControllerSource, WholeSecondsTextSignature, FormatterBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔ the formatter gained no second parameter and no card knowledge — scalar in, `FText` out"),
		CountOccurrencesInCode(FormatterBody, TEXT("CardRefused_")), 0);
	TestEqual(
		TEXT("⛔ and it still knows nothing about Bright Sun"),
		CountOccurrencesInCode(FormatterBody, TEXT("BrightSun")), 0);

	// ⭐ THE SURFACE IS THE SHIPPED ONE (`J-F25`): same `RefuseCardPlay` + `FText::Format` idiom,
	// no new delegate, no new widget. ⛔ Both prevention refusals ride it.
	TestEqual(
		TEXT("⭐ both prevention refusals ride the ONE shipped refusal channel — no new toast for the longer sentence"),
		CountOccurrencesInCode(PlayBody, TEXT("RefuseCardPlay(CardID, FText::Format(")), 2);
	TestEqual(
		TEXT("⛔ and this row broadcasts no refusal delegate of its own"),
		CountOccurrencesInCode(PlayBody, TEXT("OnCardRefused.Broadcast")), 0);
	TestEqual(
		TEXT("⛔ nor creates a widget"),
		CountOccurrencesInCode(PlayBody, TEXT("CreateWidget")), 0);

	// ⚠️⚠️ REPORTED, ⛔ NOT ACTED ON (🧑 `J-F25`, closed — and it is SHARPER here than for the
	// sibling): this sentence is roughly twice as long as "Bright Sun is still up for 143 seconds"
	// and carries TWO numbers the player must read, on a HUD channel whose show-then-hide lifetime
	// `VID-005` measured at ≈1.8 s. ⛔ The lifetime lives in `WBP_CardHand`, is not assertable from
	// C++, and changing it is a GAME-WIDE UI change — 🧑 Jonathan's call, not this lane's.
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
