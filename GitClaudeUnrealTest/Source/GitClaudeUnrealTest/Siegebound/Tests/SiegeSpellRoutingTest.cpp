// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/CardRow.h"     // FCardRow + ESpellEffect + ESpellDelivery — the three types every claim here is about
#include "Siegebound/SpellLibrary.h" // USpellLibrary::SpellRequiresAiming — the ONE derivation, called directly
#include "UObject/Class.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ THE SPELL ROUTING GATE (TASK-1018; law ⭐⭐ `SC-§75`(B), ⭐⭐ `FOG-§10.1`, `SC-§37`,
 *      ⭐ `SC-§65`, ⭐⭐ `SC-§77`) ═══
 *
 *  📌 Jonathan, verbatim (2026-09-04): *"bright sun and fog seemed to have a placement circle
 *  for them, which is totally unnecessary, playing fog or bright sun should be instant and not
 *  have any placement circle (like the pickpocket card)."*
 *
 *  ⛔⛔ THE DEFECT THIS FILE CLOSES: `ASiegePlayerController` routed a spell to the NO-RETICLE
 *  instant path on `Row->SpellEffect == ESpellEffect::GoldSteal` — a BLACKLIST OF ONE, at TWO
 *  code sites plus a comment asserting it. It failed OPEN (a new global spell silently gets a
 *  reticle) and it had ALREADY FAILED TWICE by the time he saw it: once for `Fog`, once for
 *  `BrightSun`. ⛔ ADDING THE TWO FOG EFFECTS TO THAT LIST WOULD HAVE GUARANTEED A THIRD.
 *
 *  ── ⛔ THE TRAP THIS FILE ALSO PINS, BECAUSE IT IS THE ONE A REVIEWER WOULD ASK FOR ────────
 *  The obvious structural repair — `GetEffectiveDelivery(Row) == ESpellDelivery::GroundCircle`
 *  — is WORSE THAN THE BLACKLIST. That function's `Auto` arm returns `GroundCircle` for every
 *  effect that is not `AoEDamage`/`Freeze`, so `GoldSteal`, `FogCover` AND `FogClear` all
 *  answer `GroundCircle`. The blacklist at least got `Pickpocket` right; the "derivation" gets
 *  all three wrong. ⭐ THE STRUCTURAL REASON: `ESpellDelivery` HAS NO VALUE MEANING "NO AIM AT
 *  ALL" — delivery says WHICH aiming sentence, never WHETHER there is one.
 *  ⇒ TEST 1's Auto/zero-radius rows are written so that the `== GroundCircle` shortcut goes RED
 *  the moment anyone "simplifies" the predicate into it.
 *
 *  ── ⛔ WHAT IS ASSERTED ────────────────────────────────────────────────────────────────────
 *    TEST 1  the predicate itself, UNIVERSALLY QUANTIFIED over `StaticEnum<ESpellDelivery>()`
 *            × `StaticEnum<ESpellEffect>()` — ⛔ never by naming today's cards (`SC-§65`).
 *    TEST 2  `FOG-§10.1` in BEHAVIOUR: the shipped no-reticle shapes take the INSTANT path and
 *            the shipped aimed shapes take the TARGETING path, with paired controls.
 *    TEST 3  the ROUTING consumes that one predicate at BOTH sites, holds no blacklist, and
 *            the third site — a COMMENT — no longer asserts the deleted behaviour.
 *    TEST 4  there is EXACTLY ONE derivation of "has a reticle" in the shipping tree.
 *    TEST 5  the rider: a re-route moves a card between two paths that were EACH complete, so
 *            both fog REFUSAL gates and both REFUND sites are re-proven on the new lane.
 *
 *  ── ⛔ MECHANISM ───────────────────────────────────────────────────────────────────────────
 *  ⛔ Zero PIE, ⛔ zero `SpawnActor`, ⛔ zero asset loads, ⛔ zero writes. Two lanes:
 *    (a) ⭐ DIRECT CALLS on the pure static `USpellLibrary::SpellRequiresAiming` (the
 *        `EffectiveBuildingClearance` / `WholeSecondsText` precedent). This is the lane that is
 *        genuinely EXECUTED, and it is where the routing ANSWER is proved rather than described.
 *    (b) SOURCE-TEXT structural probes for the claims that are about WHICH CALL FEEDS WHICH,
 *        about ORDER, and about what is ABSENT — a call graph and an absence cannot be seen
 *        from a running function.
 *
 *  ── ⚠️ RED-PROOF (`SC-§37`), STATED PER TEST BECAUSE TWO OF THEM ARE ABSENCE CLAIMS ────────
 *    TEST 1  RED against the `== GroundCircle` shortcut (it answers TRUE for the three
 *            zero-radius `Auto` effects) and RED against the `!= GoldSteal` blacklist (it
 *            answers TRUE for `FogCover`/`FogClear`). ⚠️ DECLARED HONESTLY: it is NOT red
 *            against TASK-999's own derivation, because this predicate IS that derivation,
 *            moved. What TEST 1 guards is the CONTENT; TEST 3 guards the CONSUMPTION.
 *    TEST 2  the negative rows are PAIRED with positive controls in the same test, so a
 *            predicate that returned `false` unconditionally fails on the controls.
 *    TEST 3  RED against the pre-fix controller: `SpellRequiresAiming` appeared ZERO times in
 *            either function and the banned comparison appeared TWICE.
 *    TEST 4  the census SELF-CHECKS that its scan is alive, so a dead instrument cannot read
 *            as a clean zero (`SC-§40`).
 *    TEST 5  every ordering probe FAILS on a missing marker rather than reporting "nothing
 *            after it", i.e. rather than passing quietly (`SC-§38`).
 *
 *  ── ⛔ NO COORDINATES (`SC-§77`) ───────────────────────────────────────────────────────────
 *  No line number appears in any executable line. Every probe is anchored to a SYMBOL, and the
 *  `GoldSteal` predicate this row deletes had already moved once in a single day without a
 *  byte of it changing. Card SHAPES are rebuilt from their columns as FIXTURES and named by
 *  CardID only in prose — a fixture that transcribes the CSV becomes a second, silently
 *  diverging copy of it.
 *
 *  ⛔ NOT DUPLICATED HERE, ON PURPOSE: `Tests/SiegeCardGlossaryTest.cpp` owns the GLOSSARY side
 *  of this same predicate and `Tests/SiegeSpellDeliveryTest.cpp` (TASK-1017) owns
 *  `GetEffectiveDelivery`'s own coverage. This file asserts the ROUTING branch and nothing else.
 */

namespace SiegeSpellRoutingFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these claims are about. */
	const TCHAR* PlayerControllerCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp");

	/** The three function signatures this file reads. ⛔ A stale one FAILS rather than scanning nothing. */
	const TCHAR* PlayHandSlotSignature = TEXT("void ASiegePlayerController::PlayHandSlot(int32 Slot)");
	const TCHAR* EnterTargetingSignature = TEXT("void ASiegePlayerController::EnterTargetingMode(");
	const TCHAR* ResolveInstantSignature = TEXT("void ASiegePlayerController::ResolveSpellInstant(");
	const TCHAR* ConfirmTargetSignature = TEXT("void ASiegePlayerController::TryConfirmSpellTarget(");

	/** ⭐ THE ONE PREDICATE, as the routing spells its call. */
	const TCHAR* PredicateCall = TEXT("USpellLibrary::SpellRequiresAiming(*Row)");

	/** ⛔ THE BANNED SHAPE (`SC-§75`(B)) — an effect-enum comparison anywhere in the routing. */
	const TCHAR* BannedBlacklist = TEXT("SpellEffect == ESpellEffect::GoldSteal");

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
	 *  `SiegeFogRefusalTest.cpp` / `SiegeBrightSunTest.cpp` / `SiegeFogVolumeTest.cpp` /
	 *  `SiegeFogClampTest.cpp` so all of them agree character for character.
	 *  ⛔ Do not "improve" it here; a divergent counter would make two files disagree about the
	 *  same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL OTHERS: the repaired routing sites QUOTE the
	 *  guard they replaced, in their comments, precisely so the next reader knows what must not
	 *  come back. A scanner that counted comment lines would force that code to choose between
	 *  explaining the law and passing it, and TEST 3's ZERO would be unachievable without
	 *  deleting the explanation.
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
	 *  ⭐⭐ THE COMMENT-AWARE COUNTER, AND IT EXISTS FOR EXACTLY ONE ROW IN THIS FILE.
	 *  The third blacklist site was a COMMENT — *"only GoldSteal reaches this instant path"* —
	 *  and ⚖️ A FIX THAT REPAIRS THE CODE AND LEAVES THE PROSE SHIPS A CONFIDENT EXPLANATION OF
	 *  BEHAVIOUR THAT NO LONGER EXISTS (`SC-§77`). ⛔ The code-only counter above is BLIND to
	 *  that defect by construction, so asserting its absence needs a scanner that reads
	 *  comments. ⛔ Used ONLY for that claim; every other row here must stay comment-immune.
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

	/** Every `.h`/`.cpp` under `Source/`, absolute paths (the `SiegeFogClampTest.cpp` helper, verbatim). */
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

	/**
	 *  Returns everything in Body BEFORE the first occurrence of Marker, so a caller can count
	 *  what has and has not happened by the time execution reaches it.
	 *  ⛔ A MISSING MARKER FAILS: an ordering claim measured against a marker that is no longer
	 *  there would otherwise report "nothing after it" — i.e. green — which is the exact
	 *  quietly-passing shape `SC-§38` bans.
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
	 *  Every DECLARED `ESpellEffect` value, `None` excluded (it is not a spell) and UHT's hidden
	 *  `_MAX` sentinel skipped BY NAME — the `SiegeCardGlossaryTest.cpp` / `SiegeFogVolumeTest.cpp`
	 *  idiom, deliberately: one shape for one problem.
	 *
	 *  ⚠️ SPELLED `…Routing…`, NOT `GatherDeclaredSpellEffects`, ON PURPOSE (`TASK-1019` BLOCKER-2):
	 *  `SiegeCardGlossaryTest.cpp` declares THAT spelling inside an ANONYMOUS namespace, whose
	 *  members sit at GLOBAL scope unconditionally — so sharing the name made the two ambiguous
	 *  under a unity build. ⛔ Same SHAPE as the sibling, deliberately; ⛔ NOT the same NAME.
	 *  ⛔ Do not "restore" it to match — that is the collision, not a tidy-up.
	 *
	 *  ⚠️ The sentinel is identified by NAME rather than by position or by
	 *  `HasMetaData("Hidden")`: the position is a UHT detail, and that accessor is
	 *  `WITH_EDITOR`-only while this file compiles wherever WITH_DEV_AUTOMATION_TESTS is on.
	 */
	static TArray<ESpellEffect> GatherRoutingSpellEffects(const UEnum* EffectEnum)
	{
		TArray<ESpellEffect> Values;
		if (!EffectEnum)
		{
			return Values;
		}

		for (int32 Index = 0; Index < EffectEnum->NumEnums(); ++Index)
		{
			if (EffectEnum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive))
			{
				continue;
			}

			const int64 Value = EffectEnum->GetValueByIndex(Index);
			if (Value == static_cast<int64>(ESpellEffect::None))
			{
				continue;
			}

			Values.AddUnique(static_cast<ESpellEffect>(Value));
		}
		return Values;
	}

	/**
	 *  Every DECLARED `ESpellDelivery` value — ⛔ `Auto` INCLUDED, unlike the effect sweep above,
	 *  because `Auto` is the state EVERY shipped row but two is in and it is therefore the state
	 *  the defect lived in. Same `_MAX`-by-name skip.
	 */
	static TArray<ESpellDelivery> GatherDeclaredDeliveries(const UEnum* DeliveryEnum)
	{
		TArray<ESpellDelivery> Values;
		if (!DeliveryEnum)
		{
			return Values;
		}

		for (int32 Index = 0; Index < DeliveryEnum->NumEnums(); ++Index)
		{
			if (DeliveryEnum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive))
			{
				continue;
			}

			Values.AddUnique(static_cast<ESpellDelivery>(DeliveryEnum->GetValueByIndex(Index)));
		}
		return Values;
	}

	/**
	 *  A spell row in a chosen aiming SHAPE. ⛔ The magnitudes are FIXTURE values chosen to
	 *  exercise the branches, ⛔ NOT transcriptions of `cards.csv` (a fixture that mirrors the
	 *  CSV becomes a second, silently diverging copy of it).
	 */
	static FCardRow MakeSpellRow(ESpellEffect Effect, ESpellDelivery Delivery, float AoERadius)
	{
		FCardRow Row;
		Row.CardType = ECardType::Spell;
		Row.SpellEffect = Effect;
		Row.SpellDelivery = Delivery;
		Row.AoERadius = AoERadius;
		return Row;
	}

	/**
	 *  The floors every sweep self-checks against, so a null reflection object or an empty sweep
	 *  can never make a universally-quantified claim pass VACUOUSLY.
	 *  ⛔ They are FLOORS (`>=`), not equalities: appending an effect or a delivery must not turn
	 *  this file red for the wrong reason — it must turn it red at the CLAIM.
	 *
	 *  ⚠️ `MinimumRoutingSpellEffects` carries the `…Routing…` spelling for the same reason as the
	 *  effect sweep above (`TASK-1019` BLOCKER-2). `MinimumDeclaredDeliveries` keeps its name
	 *  because it never collided. The block comment below the namespace carries the full reasoning.
	 */
	constexpr int32 MinimumRoutingSpellEffects = 7;
	constexpr int32 MinimumDeclaredDeliveries = 3;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  ⛔⛔ THERE IS DELIBERATELY NO `using namespace SiegeSpellRoutingFixture;` IN THIS
//  FILE — NOT AT FILE SCOPE, AND NOT INSIDE THE `RunTest` BODIES EITHER. ⛔ EVERY
//  REFERENCE BELOW IS FULLY QUALIFIED. This is a DECLARED DEVIATION from the house
//  idiom (16 sibling test files put the directive inside each `RunTest`), and it is
//  declared here so the next reader restores neither form. (`TASK-1019` BLOCKER-1 +
//  BLOCKER-2 — a file-scope directive stood here and was the ONLY one in `Source/`.)
//
//  ⛔ WHY NOT AT FILE SCOPE: a file-scope directive leaks for the REST OF THE
//  TRANSLATION UNIT, and UBT unity-builds this module (there is no `bUseUnity = false`
//  in the Build.cs or either .Target.cs). `Tests/SiegeUnitNoticeRangeTest.cpp` declares
//  `LoadProjectFile`, `CountOccurrencesInCode` and `ExtractFunctionBody` with IDENTICAL
//  signatures in its OWN named fixture and calls them UNQUALIFIED. In a shared unity
//  blob those calls would find two candidates at global scope ([namespace.udir]/2)
//  ⇒ ambiguous ⇒ ⛔⛔ A FILE THIS ROW NEVER TOUCHED FAILS TO COMPILE, with an error
//  pointing away from the cause. ⚠️ Worse for being blob-conditional: it can be green
//  today and red the moment an unrelated test file lands in this alphabetical
//  neighbourhood and re-cuts the blob boundary.
//
//  ⛔⛔ AND WHY QUALIFICATION RATHER THAN JUST MOVING IT INTO EACH `RunTest`: moving it
//  does NOT fix the second hazard. `Tests/SiegeCardGlossaryTest.cpp` declares its own
//  `GatherDeclaredSpellEffects` / `MinimumDeclaredSpellEffects` in an ANONYMOUS
//  namespace, and anonymous-namespace members sit at GLOBAL scope UNCONDITIONALLY. A
//  function-scope directive still nominates this fixture's members into that SAME
//  global scope for the lookup, so the ambiguity SURVIVES the file-scope fix. ⛔ Only
//  qualification or a rename removes it — so this file does BOTH: it qualifies every
//  reference, AND the two names that collided are spelled `…Routing…` above, which
//  restores the tree-uniqueness rule `qa/TASK-1013.md` applied when they were
//  introduced. (`GatherDeclaredDeliveries` / `MinimumDeclaredDeliveries` keep their
//  spelling: they are already unique tree-wide, and renaming a name that never
//  collided would be churn. That asymmetry is the reason, not an oversight.)
// ═══════════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE PREDICATE, QUANTIFIED OVER `ESpellDelivery` × `ESpellEffect`.
//  ⛔ The board's item (4), written the way it was asked for: by ENUMERATION over
//  the delivery column, ⛔ never by naming today's cards (`SC-§65`). A spell whose
//  enum value does not exist yet is already covered by this test.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellRoutingEveryDeliveryValueRoutesCorrectlyTest,
	"Siegebound.SpellRouting.EveryDeliveryValueGetsTheRightRoutingAnswer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellRoutingEveryDeliveryValueRoutesCorrectlyTest::RunTest(const FString& Parameters)
{
	const UEnum* const EffectEnum = StaticEnum<ESpellEffect>();
	const UEnum* const DeliveryEnum = StaticEnum<ESpellDelivery>();
	if (!EffectEnum || !DeliveryEnum)
	{
		AddError(TEXT("SELF-CHECK FAILED: StaticEnum<ESpellEffect>() or StaticEnum<ESpellDelivery>() returned null — every claim in this file would be meaningless."));
		return false;
	}

	const TArray<ESpellEffect> DeclaredEffects = SiegeSpellRoutingFixture::GatherRoutingSpellEffects(EffectEnum);
	const TArray<ESpellDelivery> DeclaredDeliveries = SiegeSpellRoutingFixture::GatherDeclaredDeliveries(DeliveryEnum);

	TestTrue(
		TEXT("SELF-CHECK: the effect sweep found the declared ESpellEffect values (a zero would make every claim ")
		TEXT("below vacuous — the whole test would pass by describing nothing)."),
		DeclaredEffects.Num() >= SiegeSpellRoutingFixture::MinimumRoutingSpellEffects);

	TestTrue(
		TEXT("SELF-CHECK: the delivery sweep found the declared ESpellDelivery values. ⛔ `Auto`, `GroundCircle` ")
		TEXT("and `HeroLine` are ALL THREE that exist, and ⛔ NONE of them means \"no aim at all\" — which is ")
		TEXT("precisely why the routing needs a predicate rather than a comparison."),
		DeclaredDeliveries.Num() >= SiegeSpellRoutingFixture::MinimumDeclaredDeliveries);

	for (const ESpellEffect Effect : DeclaredEffects)
	{
		const FString EffectName = EffectEnum->GetNameStringByValue(static_cast<int64>(Effect));
		const bool bLineCapable = (Effect == ESpellEffect::AoEDamage || Effect == ESpellEffect::Freeze);

		for (const ESpellDelivery Delivery : DeclaredDeliveries)
		{
			const FString DeliveryName = DeliveryEnum->GetNameStringByValue(static_cast<int64>(Delivery));
			const bool bAuthored = (Delivery != ESpellDelivery::Auto);

			// ── (a) ⛔ AN AUTHORED CELL IS A DECLARED AIM, EVEN WITH NO RADIUS. The column
			//    exists to be the per-card override lever, and GetEffectiveDelivery itself
			//    branches on the cell FIRST — so the routing agrees with the data rather than
			//    second-guessing it. `Auto` rows fall through to (b)/(c).
			const FCardRow BareRow = SiegeSpellRoutingFixture::MakeSpellRow(Effect, Delivery, /*AoERadius=*/ 0.f);
			const bool bBareAimed = USpellLibrary::SpellRequiresAiming(BareRow);

			if (bAuthored)
			{
				TestTrue(
					*FString::Printf(
						TEXT("⛔ %s + an AUTHORED `%s` cell must ROUTE TO TARGETING even with a zero radius — an author ")
						TEXT("who pins a delivery has DECLARED an aim. ⛔ Do not \"fix\" this by ignoring the cell; a wrong ")
						TEXT("cell is a DATA defect for a data gate."),
						*EffectName, *DeliveryName),
					bBareAimed);
			}
			else
			{
				// ── (b) ⛔⛔ THE ROW THAT KILLS BOTH WRONG ANSWERS AT ONCE. `Auto` + zero radius:
				//    there is no aim point anywhere in the data, so a non-line effect resolves
				//    INSTANTLY with no reticle — the `Pickpocket` precedent, extended to `Fog`,
				//    `BrightSun` and every future global spell BY CONSTRUCTION.
				//    ⛔ RED-PROOF, and it is the whole reason this row is written as an
				//    enumeration: `GetEffectiveDelivery(Row) == GroundCircle` answers TRUE here
				//    for EVERY non-line effect (its `Auto` arm returns GroundCircle for all of
				//    them), and the deleted `!= GoldSteal` blacklist answers TRUE for every
				//    non-line effect except one. ⇒ BOTH of the plausible wrong repairs fail this
				//    single assertion, for several effects each.
				// ⛔ TestTrue/TestFalse rather than a bool TestEqual: the assertion text has to name
				// which answer was expected anyway, and this keeps the overload unambiguous.
				const FString BareClaim = FString::Printf(
					TEXT("⛔⛔ %s + `Auto` + NO radius: the routing answer must be %s. ⛔ There is no aim point in the ")
					TEXT("row and no cell declaring one, so a non-line spell has NOTHING to place — asking the player ")
					TEXT("to aim a spell that ignores where they aim is the defect 🧑 Jonathan reported. ⛔ A `== ")
					TEXT("ESpellDelivery::GroundCircle` guard fails HERE, and so does a `!= ESpellEffect::GoldSteal` one."),
					*EffectName, bLineCapable ? TEXT("TARGETING") : TEXT("INSTANT"));

				if (bLineCapable)
				{
					TestTrue(*BareClaim, bBareAimed);
				}
				else
				{
					TestFalse(*BareClaim, bBareAimed);
				}

				// ── (c) ⛔ THE PAIRED POSITIVE CONTROL, IN THE SAME TEST. Without it a predicate
				//    that simply returned `false` for every `Auto` row would satisfy (b) for the
				//    five non-line effects and quietly break `Lightning` and `BattleCry`, whose
				//    reticle is real. A positive radius IS the reticle's footprint.
				const FCardRow AimedRow = SiegeSpellRoutingFixture::MakeSpellRow(Effect, Delivery, /*AoERadius=*/ 400.f);
				TestTrue(
					*FString::Printf(
						TEXT("⛔ PAIRED CONTROL: %s + `Auto` + a POSITIVE radius must ROUTE TO TARGETING. A ground-placed ")
						TEXT("spell resolves INSIDE its radius, so the radius IS the reticle's footprint. ⛔ Without this ")
						TEXT("row, a predicate that answered `false` unconditionally would pass the assertion above."),
						*EffectName),
					USpellLibrary::SpellRequiresAiming(AimedRow));
			}
		}
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐ — `FOG-§10.1` SATISFIED IN BEHAVIOUR, NOT ONLY IN TEXT.
//  ⛔ The shipped SHAPES, rebuilt from their columns. This is the test that fails
//  if his actual complaint comes back.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellRoutingNoReticleSpellsAreInstantTest,
	"Siegebound.SpellRouting.TheNoReticleSpellShapesTakeTheInstantPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellRoutingNoReticleSpellsAreInstantTest::RunTest(const FString& Parameters)
{
	// ⛔ THE THREE NO-RETICLE SHAPES AS THE ROSTER AUTHORS THEM: a blank `SpellDelivery` cell
	// (deserialising to `Auto`) and a zero `AoERadius`. `Pickpocket` is the PRECEDENT 🧑 Jonathan
	// named — *"like the pickpocket card"* — and it is here as a REGRESSION guard: the old
	// blacklist got it right, so a repair that broke it while fixing the fog would be a trade,
	// not a fix.
	const FCardRow PickpocketShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::GoldSteal, ESpellDelivery::Auto, 0.f);
	const FCardRow FogShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::FogCover, ESpellDelivery::Auto, 0.f);
	const FCardRow BrightSunShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::FogClear, ESpellDelivery::Auto, 0.f);

	TestFalse(
		TEXT("⭐ REGRESSION GUARD — the `Pickpocket` SHAPE (GoldSteal, blank cell, zero radius) still resolves ")
		TEXT("INSTANTLY, and now BY CONSTRUCTION rather than by being named in a list. 🧑 His sentence was ")
		TEXT("\"like the pickpocket card\", so breaking this while fixing the fog would trade one defect for another."),
		USpellLibrary::SpellRequiresAiming(PickpocketShape));

	TestFalse(
		TEXT("⭐⭐ 🧑 HIS ACTUAL COMPLAINT, HALF ONE — the `Fog` SHAPE (FogCover, blank cell, zero radius) takes the ")
		TEXT("INSTANT path: ⛔ NO placement circle, ⛔ no cursor, ⛔ no confirm click (`FOG-§10.1`: \"NO RETICLE\"). ")
		TEXT("⛔ The fog is battlefield-wide and the resolver ignores TargetPoint entirely, so every frame of aiming ")
		TEXT("was asking him to choose something that could not matter."),
		USpellLibrary::SpellRequiresAiming(FogShape));

	TestFalse(
		TEXT("⭐⭐ 🧑 HIS ACTUAL COMPLAINT, HALF TWO — the `BrightSun` SHAPE (FogClear, blank cell, zero radius) takes ")
		TEXT("the INSTANT path, same law, same reason: the FogClear arm reads no TargetPoint either."),
		USpellLibrary::SpellRequiresAiming(BrightSunShape));

	// ⛔⛔ THE PAIRED POSITIVE CONTROLS, AND THEY ARE NOT OPTIONAL. Three `TestFalse` rows alone
	// are satisfied by a predicate that answers `false` for everything — which would delete
	// targeting mode from the game and pass this test. Each shipped AIMED shape is rebuilt here.
	const FCardRow LightningShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::TopTargetsDamage, ESpellDelivery::Auto, 700.f);
	const FCardRow BattleCryShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::AllyBuff, ESpellDelivery::Auto, 400.f);
	const FCardRow FireballShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::AoEDamage, ESpellDelivery::HeroLine, 300.f);
	const FCardRow FrostNovaShape = SiegeSpellRoutingFixture::MakeSpellRow(ESpellEffect::Freeze, ESpellDelivery::HeroLine, 350.f);

	TestTrue(
		TEXT("⛔ PAIRED CONTROL: the `Lightning` SHAPE (blank cell, positive radius) still ENTERS TARGETING — its ")
		TEXT("reticle is real and the spell resolves inside that radius."),
		USpellLibrary::SpellRequiresAiming(LightningShape));

	TestTrue(
		TEXT("⛔ PAIRED CONTROL: the `BattleCry` SHAPE (blank cell, positive radius) still ENTERS TARGETING."),
		USpellLibrary::SpellRequiresAiming(BattleCryShape));

	TestTrue(
		TEXT("⛔ PAIRED CONTROL: the `Fireball` SHAPE (AUTHORED `HeroLine`, positive radius) still ENTERS TARGETING — ")
		TEXT("a line spell aims too, it simply aims along a corridor instead of at a circle."),
		USpellLibrary::SpellRequiresAiming(FireballShape));

	TestTrue(
		TEXT("⛔ PAIRED CONTROL: the `FrostNova` SHAPE (AUTHORED `HeroLine`, positive radius) still ENTERS TARGETING."),
		USpellLibrary::SpellRequiresAiming(FrostNovaShape));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐⭐ — THE ROUTING CONSUMES THE ONE PREDICATE, AT ALL THREE SITES.
//  ⛔ Two of the three sites are code. ⛔ THE THIRD IS A COMMENT, and it is asserted
//  with a comment-AWARE scanner for exactly that reason.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellRoutingConsumesThePredicateTest,
	"Siegebound.SpellRouting.TheRoutingConsumesThePredicateAndHoldsNoBlacklist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellRoutingConsumesThePredicateTest::RunTest(const FString& Parameters)
{
	FString ControllerSource;
	if (!SiegeSpellRoutingFixture::LoadProjectFile(*this, SiegeSpellRoutingFixture::PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	FString TargetingBody;
	if (!SiegeSpellRoutingFixture::ExtractFunctionBody(*this, ControllerSource, SiegeSpellRoutingFixture::PlayHandSlotSignature, PlayBody)
		|| !SiegeSpellRoutingFixture::ExtractFunctionBody(*this, ControllerSource, SiegeSpellRoutingFixture::EnterTargetingSignature, TargetingBody))
	{
		return false;
	}

	// ── SITE 1: the hand-play entry.
	TestEqual(
		TEXT("⛔ SITE 1 — `PlayHandSlot`'s spell arm asks the ONE derivation, exactly once. ⛔ A ZERO means the ")
		TEXT("blacklist came back or the derivation was inlined; a TWO means the routing question is being asked ")
		TEXT("twice in one entry, which is how two answers start to disagree."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(PlayBody, SiegeSpellRoutingFixture::PredicateCall), 1);

	// ── SITE 2: the direct, hand-less entry. ⛔ FIXING ONLY SITE 1 WOULD LEAVE A CARD THAT IS
	//    INSTANT FROM THE HAND AND TARGETED FROM A DIRECT CALL — the same card, two behaviours.
	TestEqual(
		TEXT("⛔ SITE 2 — `EnterTargetingMode`'s head asks the SAME derivation, exactly once. ⛔ This is the site a ")
		TEXT("one-site fix forgets, and forgetting it gives one card TWO behaviours depending on how it was played."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(TargetingBody, SiegeSpellRoutingFixture::PredicateCall), 1);

	// ── THE BAN (`SC-§75`(B)). ⛔ A re-point alone would pass if somebody kept BOTH: the derived
	//    call for the fog cards and the old list beside it for something else.
	FString Where;
	const int32 BlacklistHits = SiegeSpellRoutingFixture::CountAcrossShippingSource(*this, SiegeSpellRoutingFixture::BannedBlacklist, Where);
	TestEqual(
		*FString::Printf(
			TEXT("⛔⛔ THE BLACKLIST IS GONE FROM THE WHOLE SHIPPING TREE, not merely bypassed in one function. ⛔ It ")
			TEXT("failed OPEN and had ALREADY failed TWICE — once per new no-reticle spell — so `SC-§75`(B) makes a ")
			TEXT("survivor an automatic fail. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nothing)") : *Where),
		BlacklistHits, 0);

	// ── SITE 3: ⛔⛔ THE COMMENT. ⚖️ REPAIRING THE TWO CODE SITES AND LEAVING THIS WOULD SHIP A
	//    CONFIDENT SENTENCE EXPLAINING BEHAVIOUR THAT NO LONGER EXISTS (`SC-§77`) — and this batch
	//    has already watched a row repairing false prose write fresh false prose.
	//    ⛔ MEASURED WITH THE COMMENT-AWARE COUNTER ON PURPOSE: the code-only scanner used by every
	//    other row in this file is BLIND to this defect by construction, so it would report a
	//    serene zero over the lie.
	TestEqual(
		TEXT("⛔⛔ SITE 3 — the sentence \"only GoldSteal reaches this instant path\" is GONE from the controller, ")
		TEXT("comments INCLUDED. ⚖️ A fix that repairs the code and leaves the prose ships an explanation of ")
		TEXT("behaviour that no longer exists: `Fog` and `BrightSun` reach that path now, and so will the next ")
		TEXT("spell the predicate answers `false` for."),
		SiegeSpellRoutingFixture::CountOccurrencesAnywhere(ControllerSource, TEXT("only GoldSteal reaches this instant path")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐⭐ — EXACTLY ONE DERIVATION OF "HAS A RETICLE" IN THE TREE.
//  ⛔ Two derivations is a defect EVEN WHILE BOTH ARE CORRECT: they will diverge,
//  and the divergence presents as the deck panel and the game disagreeing — which
//  reads as TWO bugs instead of one.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellRoutingOneDerivationTest,
	"Siegebound.SpellRouting.ThereIsExactlyOneDerivationOfHasAReticle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellRoutingOneDerivationTest::RunTest(const FString& Parameters)
{
	FString Where;

	// ⛔ ONE DEFINITION. The signature, not the call — a second `bool USpellLibrary::` body would
	// be a compile error, but a second FREE function computing the same thing would not be.
	const int32 DefinitionHits = SiegeSpellRoutingFixture::CountAcrossShippingSource(*this, TEXT("bool USpellLibrary::SpellRequiresAiming"), Where);
	TestEqual(
		*FString::Printf(TEXT("⛔ The aim predicate is DEFINED exactly once in shipping source. Found:%s"),
			Where.IsEmpty() ? TEXT(" (nothing — the scan is dead or the predicate is gone)") : *Where),
		DefinitionHits, 1);

	// ⛔⛔ AND THE TERMS IT IS BUILT FROM EXIST NOWHERE ELSE. This is the row that actually catches
	// a SECOND derivation: a copy-paste into the controller or back into the widget would compile,
	// review clean, agree with this one on today's data, and diverge on the first retune. ⭐ Each
	// term is named distinctively enough that a hit anywhere else IS a second copy.
	//
	// ⚠️ DECLARED EXCLUSION, so its absence reads as a decision rather than an oversight:
	// `bDeliversAsLine` is NOT in this list. `UDeckBuilderWidget`'s composer legitimately keeps a
	// local of that name to choose between the CIRCLE and LINE wordings of its two area effects —
	// that is "WHICH aiming sentence", a different question from "WHETHER there is one", and it is
	// not part of the aim derivation. Pinning it here would forbid a correct, unrelated local.
	// ⇒ the three terms below are the ones that exist ONLY as the aim derivation.
	const TCHAR* const DerivationTerms[] =
	{
		TEXT("bDeliveryAuthored"),
		TEXT("bRowCarriesAnAimPoint"),
		TEXT("bLineCapableEffect"),
	};

	for (const TCHAR* const Term : DerivationTerms)
	{
		const int32 Hits = SiegeSpellRoutingFixture::CountAcrossShippingSource(*this, Term, Where);
		TestEqual(
			*FString::Printf(
				TEXT("⛔⛔ `%s` — a term of the aim derivation — appears in exactly ONE shipping translation unit ")
				TEXT("(its declaration and its single use inside USpellLibrary::SpellRequiresAiming). ⛔ A HIGHER ")
				TEXT("count means a SECOND derivation exists: it will agree with the first on today's data and ")
				TEXT("diverge on the first retune, and the divergence presents as the glossary and the game ")
				TEXT("disagreeing — TWO bugs where there was one. Found:%s"),
				Term, Where.IsEmpty() ? TEXT(" (nothing)") : *Where),
			Hits, 2);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐⭐ — THE RIDER: A RE-ROUTE MOVES A CARD BETWEEN TWO PATHS THAT WERE
//  EACH COMPLETE, SO EVERYTHING THE OLD PATH DID MUST BE RE-PROVEN ON THE NEW ONE.
//  ⛔ THE FIX THAT ONLY RE-ROUTES IS THE EASIEST TO WRITE AND THE EASIEST TO SHIP
//  BROKEN.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeSpellRoutingRefusalsAndRefundsSurviveTest,
	"Siegebound.SpellRouting.BothRefusalGatesAndBothRefundSitesSurviveTheReRoute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeSpellRoutingRefusalsAndRefundsSurviveTest::RunTest(const FString& Parameters)
{
	FString ControllerSource;
	if (!SiegeSpellRoutingFixture::LoadProjectFile(*this, SiegeSpellRoutingFixture::PlayerControllerCpp, ControllerSource))
	{
		return false;
	}

	FString PlayBody;
	if (!SiegeSpellRoutingFixture::ExtractFunctionBody(*this, ControllerSource, SiegeSpellRoutingFixture::PlayHandSlotSignature, PlayBody))
	{
		return false;
	}

	// ── (a) ⛔⛔ BOTH FOG REFUSAL GATES STILL RUN BEFORE THE ROUTING DECISION. They are
	//    routing-agnostic BY DESIGN — each gates on `Row->SpellEffect` and `return`s — but
	//    "by design" is precisely the sentence that rots when a card changes lanes. ⛔ These are
	//    🧑 his own rulings: no gold lost, card kept, a message carrying the LIVE countdown.
	//    ⛔ Measured by ORDER against the NEW routing symbol, so the claim could not have been
	//    written before this diff and dies if anyone moves the routing above the gates.
	FString BeforeRouting;
	if (!SiegeSpellRoutingFixture::SubstringBefore(*this, PlayBody, SiegeSpellRoutingFixture::PredicateCall, BeforeRouting))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔ 🧑 J-F19's refusal (\"Bright Sun is still up for X\") is reached BEFORE the routing decision, so ")
		TEXT("moving `Fog` to the instant lane cannot swallow it. ⛔ No gold spent, card kept — and the number is ")
		TEXT("read live at the click."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(BeforeRouting, TEXT("CardRefused_BrightSunActive")), 1);

	TestEqual(
		TEXT("⛔ 🧑 J-F18's sun-on-sun refusal (\"would reduce fog prevention time from X to Y\") is likewise reached ")
		TEXT("BEFORE the routing decision, so moving `BrightSun` to the instant lane cannot swallow it either."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(BeforeRouting, TEXT("CardRefused_BrightSunWouldShorten")), 1);

	// ⛔ THE NEGATIVE CONTROL FOR THE PAIR ABOVE, AND IT IS WHAT MAKES THEM MEAN ANYTHING: the
	// routing switch must still FOLLOW them. A build whose refusal gates ate the whole function
	// would satisfy both rows and make `Fog` unplayable forever.
	const int32 SwitchIndex = PlayBody.Find(TEXT("switch (Row->CardType)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 RoutingIndex = PlayBody.Find(SiegeSpellRoutingFixture::PredicateCall, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	TestTrue(
		TEXT("⛔⛔ NEGATIVE CONTROL: the CardType routing switch still SITS BETWEEN the refusal gates and the routing ")
		TEXT("decision — the gates SKIP, they do not terminate the entry. ⛔ Without this row, a build that ended the ")
		TEXT("function at a refusal would pass every assertion above while making both fog cards unplayable."),
		SwitchIndex != INDEX_NONE && RoutingIndex != INDEX_NONE && SwitchIndex < RoutingIndex);

	// ── (b) ⛔⛔ BOTH REFUND SITES REMAIN REACHABLE, AND THEY ARE DIFFERENT CALL SITES. The
	//    instant path refunds `Row.Cost`; the targeting confirm refunds `TargetingCost`. Moving
	//    the fog cards CHANGES WHICH ONE THEY HIT, and the one they left still carries every
	//    aimed spell. ⛔ Losing either bills a player for a refusal.
	FString InstantBody;
	FString ConfirmBody;
	if (!SiegeSpellRoutingFixture::ExtractFunctionBody(*this, ControllerSource, SiegeSpellRoutingFixture::ResolveInstantSignature, InstantBody)
		|| !SiegeSpellRoutingFixture::ExtractFunctionBody(*this, ControllerSource, SiegeSpellRoutingFixture::ConfirmTargetSignature, ConfirmBody))
	{
		return false;
	}

	TestEqual(
		TEXT("⛔ REFUND SITE 1 (the INSTANT path — where `Fog` and `BrightSun` now fizzle): a resolver refusal refunds ")
		TEXT("the FULL cost. ⛔ Both fog arms genuinely CAN return false (no AFogVolume could be spawned; `RaiseFog` ")
		TEXT("refused under a live prevention window; `ApplyBrightSun` refused a window-shortening cast), so this is ")
		TEXT("a live path for them, not a theoretical one."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(InstantBody, TEXT("AddGold(Row.Cost)")), 1);

	TestEqual(
		TEXT("⛔ REFUND SITE 2 (the TARGETING path — where every AIMED spell still resolves): unchanged by the ")
		TEXT("re-route and still exactly one refund. ⛔ A re-route that quietly emptied the lane it left would pass ")
		TEXT("every other row in this file."),
		SiegeSpellRoutingFixture::CountOccurrencesInCode(ConfirmBody, TEXT("AddGold(TargetingCost)")), 1);

	// ⛔ AND THE SECOND HALF OF 🧑 HIS NET-ZERO RULING, WHICH IS A SEPARATE PROPERTY: a build can
	// refund the gold and still EAT THE CARD. Proven by ORDER on the lane the fog cards just
	// joined — the hand step sits AFTER the refusal's early `return`.
	const int32 RefundIndex = InstantBody.Find(TEXT("AddGold(Row.Cost)"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	const int32 DrawIndex = InstantBody.Find(TEXT("ConfirmInstantDraw("), ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	TestTrue(
		TEXT("⛔⛔ THE CARD IS NOT CONSUMED ON THE NEW LANE: `ConfirmInstantDraw` sits AFTER the refund-and-return, so ")
		TEXT("a refused instant play can never reach the hand step. ⛔ TWO assertions, never one — a build that ")
		TEXT("refunded the gold but ate the card satisfies exactly half of 🧑 his ruling, and a test written against ")
		TEXT("either half alone PASSES the broken build."),
		RefundIndex != INDEX_NONE && DrawIndex != INDEX_NONE && RefundIndex < DrawIndex);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
