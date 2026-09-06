// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "HAL/UnrealMemory.h" // FMemory::Memcpy — the sanctioned bit-pattern NaN (SiegeCastBarTest.cpp:129, via SiegeBrightSunTest.cpp)
#include "Math/Transform.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Siegebound/FogVolume.h"     // AFogVolume::FogVisualTransform / FogVisualClassPath — the two seams this file EXECUTES
#include "Siegebound/ScatterConfig.h" // USiegeScatterConfig::ArenaHalfExtent — the ONE owner of the arena extent the box is derived from
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"   // GetDefault<> (explicit IWYU — the SiegeWarMapTest.cpp precedent; no compile verifies transitive pulls)

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for ⭐⭐⭐ **THE FOG VISUAL'S CALLER** (`TASK-1068`; gate `TASK-1069`,
 *      host `TASK-1070`; law `SC-§36.1` INSTANCE 2, `SC-§79`, `SC-§83`, `SC-§34`, `SC-§39`,
 *      `SC-§91`, `FOG-§6`, `FOG-§10.1`, `FOG-§10.3`) ═══
 *
 *  ⛔⛔⛔ THE DEFECT THIS FILE EXISTS TO MAKE UNREPEATABLE, AND IT IS WORTH READING BEFORE THE
 *  ASSERTIONS: `/Game/Blueprints/BP_SiegeFog` shipped ⛔ CORRECT — right vendor parent, tuned to
 *  the live Beer-Lambert targets, corner coverage derived from `ArenaHalfExtent`,
 *  integration-checked under `TASK-1043`, committed `ef2c901` — and ⛔ COMPLETELY INERT. Measured
 *  at `b6a44b8`: ⛔ EVERY `BP_SiegeFog` reference in all of `Source/` was ⛔ INSIDE A COMMENT, and
 *  `L_Arena.umap` held ⛔ ZERO occurrences. ⇒ 🧑 Jonathan played a 50-gold card, ⛔ every unit on
 *  both sides was cut to the fog vision ceiling — an ⛔ 87.8% reduction in acquisition reach — and
 *  ⛔ THE SCREEN DID NOT CHANGE. ⭐ An asset with no caller is not a feature; it is a file.
 *
 *  ⛔⛔⛔ AND THE SECOND DEFECT, `TASK-1072` — READ THIS ONE TOO, BECAUSE THE FILE ABOVE WENT FULLY
 *  GREEN WHILE IT WAS LIVE AND 🧑 JONATHAN REPORTED *"NO FOG"* ⛔ THREE MORE TIMES:
 *  `TASK-1068` gave the visual a caller and the caller worked — and the box was still invisible.
 *  `SpawnFogVisual` asked for a scale of `(640, 360, 260)` and the ⛔ ENGINE HANDED BACK
 *  `(20, 20, 5)`, the vendor component template's own scale, substituted at `SCS_Node.cpp:147`
 *  because `BP_SiegeFog`'s root is an ⛔ INHERITED SCS component rather than a native one.
 *  ⇒ a `64,000 × 36,000 × 26,000` uu volume ⛔ AROUND the battlefield became a
 *  `2,000 × 2,000 × 500` uu slab `6,750` uu ⛔ ABOVE it — `0.17 %` of the intended plan area —
 *  outside `L_Arena`'s `6000` uu froxel grid at ⛔ EVERY camera angle. Measured against a
 *  zero-control at his own vantage: ⛔ `+0.04 %` mean luma, inside the pixel noise floor, against
 *  `+73 %` at the intended scale. ⛔ THAT IS NOT FAINT FOG; IT IS NO FOG.
 *
 *  ⛔⛔⛔ **AND THE REASON IT SURVIVED A GREEN SUITE AND A PASSED GATE — THE PART THAT MATTERS MORE
 *  THAN THE SCALE.** The spawn log printed `SpawnTransform`, i.e. ⛔ THE VALUE IT ASKED FOR. It
 *  ⛔ NEVER ONCE CALLED `GetActorScale3D()`. Every sentence that log wrote was TRUE — the actor
 *  really spawned, was really held, was really destroyed on time — and ⛔ the one number that
 *  mattered was never taken. ⇒ a box the engine had shrunk by a factor of 32 read as a ⛔ CLEAN
 *  SPAWN, the gate passed, and he was told the actor had spawned *"at the right transform"*.
 *  ⭐⭐⭐ THE STANDING LAW, and this file's tests 6 and 7 are its executable form:
 *  ⛔ **AN INSTRUMENT THAT ECHOES THE REQUEST INSTEAD OF MEASURING THE RESULT IS NOT AN
 *  INSTRUMENT.** It is the failure wearing the evidence's clothes — the eleventh time this project
 *  has hit that class of lie. ⭐ *A success return is not evidence.*
 *  ⇒ ⛔ TEST 6 (Lane A, ⭐ EXECUTED) hands the shipped detector the MEASURED vendor substitute and
 *  asserts it answers ⛔ NO — *a test that passes when the scale is clobbered is not a test.*
 *  ⇒ ⛔ TEST 7 (Lane B) pins the correction, the readback, and the ⛔ ABSENCE of the old echo.
 *
 *  ⛔⛔ **SPAWN AND DESPAWN ARE ONE SUBJECT HERE, NEVER TWO.** A build that spawns and never
 *  destroys is ⛔ NOT a smaller increment — it is ⛔ PERMANENT FOG, FOREVER, with a green suite
 *  and nothing red anywhere. *"A seam that can be entered and not left is half a seam."* ⇒ every
 *  despawn claim below carries the same weight as its spawn twin, and the mutation list in
 *  `handoffs/TASK-1068-programmer.md` names one mutation for EACH.
 *
 *  ─── ⭐⭐⭐ WHAT EACH INSTRUMENT CAN AND CANNOT ANSWER (`TASK-1068` cl. 8a) ───────────────────
 *  ⛔⛔ *"A CALLER EXISTS"* AND *"THE CALLER IS REACHED"* ARE ⛔ DIFFERENT CLAIMS, and this file
 *  answers them with different instruments. ⛔ It says so rather than letting one green imply
 *  both:
 *    • ⭐ **LANE A — GENUINELY EXECUTED.** Tests 1, 2 and 6 CALL `AFogVolume::FogVisualTransform`,
 *      `AFogVolume::FogVisualClassPath` and `AFogVolume::FogVisualScaleMatches` and assert on the
 *      values that come back. These are pure statics with no world, no actor and no asset load,
 *      exactly like the shipped `BrightSunWindowSeconds` testability seam. ⛔ A wrong number here
 *      is a wrong number in game.
 *      ⚠️⚠️ AND THE LIMIT OF TEST 6, STATED SO NO GREEN IS MISTAKEN FOR MORE THAN IT IS: it proves
 *      the ⛔ DETECTOR rejects the engine's substitution. ⛔ IT DOES NOT SPAWN AN ACTOR, so it does
 *      ⛔ NOT prove the substitution is actually corrected in a running game — that claim rests on
 *      test 7's structural pin plus 🧑 his eye. ⛔ The two are different claims and this file will
 *      not let one stand in for the other; that conflation is the whole subject of `TASK-1072`.
 *    • ⚠️ **LANE B — SOURCE-TEXT STRUCTURE.** Tests 3-5 and 7 read `FogVolume.h` / `FogVolume.cpp` /
 *      `SpellLibrary.cpp` / `SiegeGameMode.cpp` off disk and count constructs on ⛔ CODE LINES
 *      ONLY. ⛔⛔ A SOURCE CENSUS PROVES A CALLER **EXISTS**; ⛔ IT CANNOT PROVE IT IS
 *      **REACHED**. A call sitting inside `if (false)` passes every one of tests 3-6. ⇒ what
 *      Lane B really buys is that a ⛔ FUTURE REFACTOR CANNOT SILENTLY DROP AN EXIT — which is
 *      the failure mode `TASK-1068` cl. 3a names, and it is the one a running test could not see
 *      either, because two of the three exits (`ResetFog`, natural expiry) have no card behind
 *      them to press.
 *    • ⛔ **WHAT NEITHER LANE COVERS, STATED SO NO GREEN IS MISTAKEN FOR IT:** there is not one
 *      `SpawnActor` anywhere in `Siegebound/Tests/`, so ⛔ NOTHING HERE RUNS THE ACTUAL SPAWN. The
 *      end-to-end claim — play `Fog`, a box appears; wait 300 s, it goes — is ⛔ NOT EXECUTED by
 *      this file and is ⛔ NOT executed by the suite.
 *
 *  ─── 🧑🚨 AND THE HONEST SCOPE OF EVERY GREEN THIS FILE CAN PRODUCE ──────────────────────────
 *  ⛔⛔ **AN AUTOMATED TEST CAN PROVE THE SPAWN HAPPENS. ⛔ IT CANNOT PROVE THE FOG LOOKS RIGHT.**
 *  🧑 Jonathan's eye is the ⛔ ONLY instrument for LEGIBILITY (`AS-§6 A(e)`), and the stake is
 *  concrete rather than decorative: under fog every unit on both sides is ⛔ 87.8% blind and
 *  ⛔ DROPS ITS TARGET. ⇒ a visual that reads as ⛔ LIGHT HAZE is a ⛔ MISMATCH between what he
 *  sees and what the simulation is doing — and ⛔ a fully green suite says ⛔ NOTHING about that.
 *  ⚠️ ⛔ Do ⛔ NOT try to settle it with a screenshot either: UE's volumetric fog is temporally
 *  accumulated, and `TASK-841` §3.2 measured ⛔ 55.8% vs ⛔ 100.8% obscuration at ⛔ IDENTICAL
 *  settings. ⛔ A fog number needs a CONVERGENCE SERIES, ⛔ never a single capture.
 *
 *  ⚠️⚠️ DECLARED — THE 19th `CountOccurrencesInCode` SITE (`TASK-1052`). This file adds a
 *  ⛔ VERBATIM copy of the house character-scan helper, so `TASK-1052`'s census of 18 sites across
 *  15 files becomes ⛔ 19 across 16 when it re-derives (`SC-§91`: its count is a LOWER BOUND).
 *  ⛔ Its guard is ⛔ NOT PRE-ADOPTED here — it is not authored yet, and adopting an unwritten
 *  fence is inventing one. ⛔ The site is DECLARED so its author-derived sweep includes this file.
 */

namespace SiegeFogVisualFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these structural claims are about. */
	const TCHAR* FogVolumeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp");
	const TCHAR* FogVolumeH = TEXT("Source/GitClaudeUnrealTest/Siegebound/FogVolume.h");
	const TCHAR* SpellLibraryCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp");
	const TCHAR* GameModeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp");

	/**
	 *  The function signatures this file reads. ⛔ A stale one FAILS rather than scanning nothing
	 *  (`SC-§38` — a probe pinned to a stale coordinate must go RED, never quietly green).
	 */
	const TCHAR* RaiseFogSignature = TEXT("bool AFogVolume::RaiseFog()");
	const TCHAR* ApplyBrightSunSignature = TEXT("bool AFogVolume::ApplyBrightSun(ETeamId CasterTeam)");
	const TCHAR* ResetFogSignature = TEXT("void AFogVolume::ResetFog()");
	const TCHAR* RefreshFogVisualSignature = TEXT("void AFogVolume::RefreshFogVisual()");
	const TCHAR* SpawnFogVisualSignature = TEXT("void AFogVolume::SpawnFogVisual()");
	const TCHAR* DestroyFogVisualSignature = TEXT("void AFogVolume::DestroyFogVisual()");
	const TCHAR* FogVisualTransformSignature = TEXT("FTransform AFogVolume::FogVisualTransform(const FVector2D& ArenaHalfExtentUU, float GroundReferenceZUU)");
	const TCHAR* FogVisualClassPathSignature = TEXT("const FSoftClassPath& AFogVolume::FogVisualClassPath()");
	const TCHAR* EndPlaySignature = TEXT("void AFogVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)");

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
	 *  Occurrences of Needle on CODE lines only — the house helper, copied ⛔ VERBATIM from
	 *  `SiegeFogRefusalTest.cpp` / `SiegeBrightSunTest.cpp` / `SiegeFogVolumeTest.cpp` /
	 *  `SiegeFogClampTest.cpp` / `SiegeAcquisitionFunnelTest.cpp` so all of them agree character
	 *  for character. ⛔ Do not "improve" it here; a divergent counter would make two files
	 *  disagree about the same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL. `FogVolume.h` and `FogVolume.cpp` now carry long
	 *  paragraphs that NAME `BP_SiegeFog`, `RefreshFogVisual`, `SpawnFogVisual`,
	 *  `DestroyFogVisual` and `FogDurationSeconds` in the prose that explains the design — and
	 *  several assertions below are ⛔ ZEROes that those very comments would break. A scanner that
	 *  counted comments would force that code to choose between ⛔ EXPLAINING the law and
	 *  ⛔ PASSING it, which is the trade this project refuses to make.
	 *  ⚠️ DECLARED LIMITATION, inherited and restated: a comment TRAILING a line of code IS still
	 *  scanned. Every probe below is a whole-line construct or a statement.
	 *  ⚠️⚠️ AND THIS IS THE ⛔ 19th SITE OF THIS HELPER (`TASK-1052`) — see the file header.
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
	 *  than silently scanning nothing.
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
	 *  ⭐ THE ORDERING LANE. Returns everything in Body BEFORE the first occurrence of Marker, so
	 *  the caller can count what has already happened by the time execution reaches it.
	 *  ⛔ A MISSING MARKER FAILS: an ordering claim measured against a marker that is no longer
	 *  there would otherwise report "nothing before it" — i.e. green.
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
	 *  ⭐ The shipped arena half-extent, read from the ⛔ SAME FIELD ON THE SAME CLASS the runtime
	 *  reads — ⛔ NOT a transcribed `(26000, 12000)`. `SC-§34`'s structural escape, and the exact
	 *  expression `AFogVolume::SpawnFogVisual` uses, so this file cannot quietly test a different
	 *  arena from the one the game builds.
	 *  ⚠️ IT IS THE **CDO** AND NOT THE ASSET, and that limit is stated rather than glossed: a
	 *  saved `DA_BattlefieldScatter` overrides this value for the SCATTER actor. That residual is
	 *  declared on `SpawnFogVisual` itself; what is asserted here is the lane the fog box uses.
	 */
	static FVector2D ShippedArenaHalfExtent()
	{
		return GetDefault<USiegeScatterConfig>()->ArenaHalfExtent;
	}

	/**
	 *  ⭐ Recovers the spawned box's HALF-EXTENT in world units from the transform the shipped
	 *  code produces: `Scale × CubeEdge / 2`. ⛔ Derived from the same named constant the
	 *  production path divides by, so the two can never drift apart — asserting a literal `640`
	 *  here would re-create the very defect `SC-§34` exists to prevent.
	 */
	static FVector HalfExtentFromTransform(const FTransform& Transform)
	{
		return Transform.GetScale3D() * static_cast<double>(AFogVolume::FogVisualUnitCubeEdgeUU) * 0.5;
	}

	/**
	 *  A quiet NaN built from its IEEE-754 bit pattern — copied ⛔ VERBATIM from
	 *  `SiegeBrightSunTest.cpp` / `SiegeCastBarTest.cpp:129`, which already paid for this lesson.
	 *  ⛔ NOT `0.f / 0.f`, ⛔ NOT `FMath::Sqrt(-1.f)` and ⛔ NOT the bare `NAN` macro: all three are
	 *  constant-foldable, and a fast-math build may evaluate them into something that is no longer
	 *  non-finite — which would make test 6's NaN clauses pass while testing ⛔ NOTHING. ⭐ Each use
	 *  is paired with a `ContainsNaN` self-check for exactly that reason.
	 *
	 *  ⚠️⚠️ AND THE FIXTURE HAZARD, DECLARED BECAUSE THIS BUILD RUNS WITH `ENABLE_NAN_DIAGNOSTIC == 1`
	 *  (`SiegeLadderClimbTest.cpp` measured it): ⛔ NEVER DO ARITHMETIC WITH A NaN VECTOR HERE.
	 *  `TVector`'s operators call `DiagnosticCheckNaN()` and raise an engine error, which would fail
	 *  the test ⛔ ON ITS OWN FIXTURE rather than on its subject. ⭐ CONSTRUCTING one and READING it
	 *  is safe and is proven green in this very suite (`SiegeLadderClimbTest.cpp`'s `NaNLocation`),
	 *  and that is the only thing test 6 does with it: `FogVisualScaleMatches` short-circuits on
	 *  `ContainsNaN()` ⛔ BEFORE it ever reaches the subtraction inside `FVector::Equals`.
	 */
	static float MakeQuietNaN()
	{
		const uint32 NaNBits = 0x7FC00000u;
		float Result = 0.f;
		FMemory::Memcpy(&Result, &NaNBits, sizeof(Result));
		return Result;
	}
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⭐ LANE A (EXECUTED) — TEST 1: the geometry, asserted as a RELATION and never as a literal
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualGeometryTest,
	"Siegebound.Fog.TheFogVisualBoxOverhangsTheArenaOnEveryAxisAndMovesWhenTheArenaDoes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualGeometryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	// ⛔⛔ WHY THIS TEST EXISTS AT ALL, IN ONE MEASUREMENT (`TASK-841` §5.2): sized to the arena
	// EXACTLY, the fog was NOT THERE AT THE ARENA EDGE. From `(24000, 11000, 1200)` the view was
	// essentially clear — green grass, the blue castle, crisp trees — mean luma `0.6249`, against
	// `0.6454` (a total white-out) once the box overhung by the margin.
	// ⇒ ⛔ A PLAYER STANDING AT THE WALL WOULD HAVE BEEN THE ONLY ONE WHO COULD SEE, in a
	// world-global, symmetric mechanic whose entire premise is that nobody can.

	const FVector2D Arena = ShippedArenaHalfExtent();

	TestTrue(TEXT("The shipped arena half-extent is strictly positive on X (a degenerate arena would make every claim below vacuous)"), Arena.X > 0.0);
	TestTrue(TEXT("The shipped arena half-extent is strictly positive on Y"), Arena.Y > 0.0);

	// The two named constants must themselves be usable, or the relations below prove nothing.
	TestTrue(TEXT("⭐ The horizontal margin is strictly positive — it IS the overhang, and at 0 the measured clear corner comes straight back"),
		AFogVolume::FogVisualHorizontalMarginUU > 0.f);
	TestTrue(TEXT("⭐ The ceiling above the ground datum is strictly positive"),
		AFogVolume::FogVisualCeilingAboveGroundUU > 0.f);
	TestTrue(TEXT("The engine cube edge used for the scale conversion is strictly positive (it is a divisor)"),
		AFogVolume::FogVisualUnitCubeEdgeUU > 0.f);

	// ── The shipped case: ground datum at J-F13's flat-grass zero ────────────────────────────
	const FTransform AtGroundZero = AFogVolume::FogVisualTransform(Arena, 0.f);
	const FVector HalfExtent = HalfExtentFromTransform(AtGroundZero);

	// ⭐⭐ THE RELATION, ⛔ NEVER THE LITERAL (`SC-§36`'s derived-ceiling preference): the box's
	// half-extent must EXCEED the arena's on BOTH axes by AT LEAST the margin. ⇒ a hand-typed
	// `640` goes RED the day the arena is resized, which is exactly what a literal must do.
	TestTrue(TEXT("⭐⭐ The fog box overhangs the arena on X by at least the margin (a clear corner is the measured failure)"),
		HalfExtent.X >= Arena.X + static_cast<double>(AFogVolume::FogVisualHorizontalMarginUU) - UE_KINDA_SMALL_NUMBER);
	TestTrue(TEXT("⭐⭐ The fog box overhangs the arena on Y by at least the margin"),
		HalfExtent.Y >= Arena.Y + static_cast<double>(AFogVolume::FogVisualHorizontalMarginUU) - UE_KINDA_SMALL_NUMBER);

	// ⛔ STRICTLY greater, stated separately: ">= arena + margin" would still pass with a margin of
	// zero if the arithmetic ever collapsed, and the whole point is that the box is BIGGER.
	TestTrue(TEXT("The fog box is strictly wider than the arena on X"), HalfExtent.X > Arena.X);
	TestTrue(TEXT("The fog box is strictly wider than the arena on Y"), HalfExtent.Y > Arena.Y);

	// ── The vertical span: below the walk surface, up to the ceiling ─────────────────────────
	const double CentreZ = AtGroundZero.GetLocation().Z;
	const double BottomZ = CentreZ - HalfExtent.Z;
	const double TopZ = CentreZ + HalfExtent.Z;

	TestTrue(TEXT("⭐ The box STARTS BELOW the ground datum — a camera standing on the grass must be INSIDE the volume, not on its bottom face"),
		BottomZ < 0.0);
	TestTrue(TEXT("⭐ The box reaches the ceiling above the ground datum"),
		FMath::IsNearlyEqual(TopZ, static_cast<double>(AFogVolume::FogVisualCeilingAboveGroundUU), 0.5));
	TestTrue(TEXT("The below-ground depth is the same named margin, reused rather than a second invented number"),
		FMath::IsNearlyEqual(-BottomZ, static_cast<double>(AFogVolume::FogVisualHorizontalMarginUU), 0.5));

	// ── Centring and orientation ─────────────────────────────────────────────────────────────
	TestTrue(TEXT("The box is centred on X (the arena is symmetric about the origin — an offset would invent an asymmetry the fog may not have)"),
		FMath::IsNearlyZero(AtGroundZero.GetLocation().X, 0.5));
	TestTrue(TEXT("The box is centred on Y"), FMath::IsNearlyZero(AtGroundZero.GetLocation().Y, 0.5));
	TestTrue(TEXT("The box is axis-aligned (an unrotated box mask is what the vendor material expects)"),
		AtGroundZero.GetRotation().Rotator().IsNearlyZero());

	// ── ⭐⭐ THE MOVING TEST: change the arena, the box MUST follow ───────────────────────────
	// ⛔ THIS IS THE ASSERTION A HARDCODED `640` DIES ON. A transform built from literals returns
	// the same box for every arena, and the two calls below would come back identical.
	const FVector2D DoubledArena = Arena * 2.0;
	const FTransform ForDoubledArena = AFogVolume::FogVisualTransform(DoubledArena, 0.f);
	const FVector DoubledHalfExtent = HalfExtentFromTransform(ForDoubledArena);

	TestTrue(TEXT("⭐⭐ Doubling the arena's X half-extent makes the fog box strictly wider — a literal transform would return the same box and go RED here"),
		DoubledHalfExtent.X > HalfExtent.X);
	TestTrue(TEXT("⭐⭐ Doubling the arena's Y half-extent makes the fog box strictly deeper"),
		DoubledHalfExtent.Y > HalfExtent.Y);
	TestTrue(TEXT("The overhang RELATION survives the resize — the box still clears the bigger arena by the margin on X"),
		DoubledHalfExtent.X >= DoubledArena.X + static_cast<double>(AFogVolume::FogVisualHorizontalMarginUU) - UE_KINDA_SMALL_NUMBER);
	TestTrue(TEXT("…and on Y"),
		DoubledHalfExtent.Y >= DoubledArena.Y + static_cast<double>(AFogVolume::FogVisualHorizontalMarginUU) - UE_KINDA_SMALL_NUMBER);
	TestTrue(TEXT("A horizontal resize leaves the VERTICAL span alone — the ceiling is not a function of the arena's width"),
		FMath::IsNearlyEqual(DoubledHalfExtent.Z, HalfExtent.Z, 0.5));

	// ── ⭐ THE GROUND DATUM: raise J-F13's zero and the whole box translates with it ──────────
	// `ArenaGroundReferenceZUU` is `EditDefaultsOnly`, so this is a value a designer can really
	// move; the box must follow it rather than sitting at absolute world zero forever.
	const float RaisedGroundZUU = 1000.f;
	const FTransform AtRaisedGround = AFogVolume::FogVisualTransform(Arena, RaisedGroundZUU);
	const FVector RaisedHalfExtent = HalfExtentFromTransform(AtRaisedGround);

	TestTrue(TEXT("⭐ Raising the ground datum translates the box by exactly that much"),
		FMath::IsNearlyEqual(AtRaisedGround.GetLocation().Z - AtGroundZero.GetLocation().Z, static_cast<double>(RaisedGroundZUU), 0.5));
	TestTrue(TEXT("…and changes its SIZE not at all — the datum moves the box, it does not stretch it"),
		FMath::IsNearlyEqual(RaisedHalfExtent.Z, HalfExtent.Z, 0.5));
	TestTrue(TEXT("…nor its horizontal extent"),
		FMath::IsNearlyEqual(RaisedHalfExtent.X, HalfExtent.X, 0.5));

	// ── Totality: no NaN, no zero-volume box, on a degenerate arena ──────────────────────────
	// ⛔ `ArenaHalfExtent` is an editable field on a DataAsset. A zeroed one must still produce a
	// finite box (the margin alone), ⛔ never a NaN and ⛔ never a degenerate scale that would
	// make the vendor mask undefined.
	const FTransform ForZeroArena = AFogVolume::FogVisualTransform(FVector2D::ZeroVector, 0.f);
	const FVector ZeroArenaScale = ForZeroArena.GetScale3D();
	TestTrue(TEXT("⛔ A zeroed arena extent still yields a FINITE transform (a DataAsset field a designer can clear must never produce NaN)"),
		FMath::IsFinite(ZeroArenaScale.X) && FMath::IsFinite(ZeroArenaScale.Y) && FMath::IsFinite(ZeroArenaScale.Z)
		&& FMath::IsFinite(ForZeroArena.GetLocation().Z));
	TestTrue(TEXT("⛔ …and a strictly positive scale on every axis — the margin alone keeps the box non-degenerate"),
		ZeroArenaScale.X > 0.0 && ZeroArenaScale.Y > 0.0 && ZeroArenaScale.Z > 0.0);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⭐ LANE A (EXECUTED) — TEST 2: the content dependency, made VISIBLE to a test
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualAssetPathTest,
	"Siegebound.Fog.TheFogVisualAssetTheCodeNamesIsReallyOnDiskAndKeepsItsGeneratedClassSuffix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualAssetPathTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	// ⛔⛔ THE HAZARD THIS TEST CONVERTS INTO A RED: a hardcoded `/Game/` path in C++ is a CONTENT
	// DEPENDENCY NO TEST CAN SEE BREAK. Rename or move `BP_SiegeFog` and every line still
	// compiles, every other test still passes, and the fog silently stops appearing — which is
	// ⛔ VERBATIM the defect `TASK-1068` exists to repair, arriving one layer up.

	const FSoftClassPath& VisualPath = AFogVolume::FogVisualClassPath();
	const FString VisualPathString = VisualPath.ToString();

	TestTrue(TEXT("⛔ The fog visual class path is non-empty (an empty path spawns nothing and says nothing)"), VisualPath.IsValid());

	const FString PackageName = VisualPath.GetLongPackageName();
	const FString AssetName = VisualPath.GetAssetName();

	TestTrue(FString::Printf(TEXT("The fog visual lives under /Game/ rather than in an engine or plugin root ('%s')"), *VisualPathString),
		PackageName.StartsWith(TEXT("/Game/"), ESearchCase::CaseSensitive));

	// ⚠️ THE `_C` SUFFIX IS LOAD-BEARING AND ITS FAILURE IS SILENT: without it the path resolves
	// to the `UBlueprint` ASSET, which is not a `UClass` and cannot be spawned — and it fails by
	// returning null rather than by complaining.
	TestTrue(FString::Printf(TEXT("⭐ The path names the GENERATED CLASS (a '_C' suffix), not the Blueprint asset — '%s'"), *AssetName),
		AssetName.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive));

	// ⭐⭐ THE INSTRUMENT IS VALIDATED AGAINST THE FAILURE IT DETECTS, ⛔ NOT MERELY AGAINST
	// SUCCESS (`SC-§39`, `SC-§79`): a package existence check that answered "yes" to everything
	// would pass this file while proving nothing at all.
	const FString DeliberatelyAbsentPackage = PackageName + TEXT("_ThisPackageDoesNotExist_NegativeControl");
	TestFalse(TEXT("⭐ NEGATIVE CONTROL: the existence probe answers NO for a package that is not there (so its YES below is evidence, not noise)"),
		FPackageName::DoesPackageExist(DeliberatelyAbsentPackage));

	// ⛔ THE CLAIM. ⚠️ EXISTENCE ONLY, and the narrowness is deliberate: this probe does ⛔ NOT
	// LOAD the asset. Loading `BP_SiegeFog` pulls in its VENDOR parent `BP_FogArea`, which
	// `TASK-841` §5.3 measured DIRTIES ITS OWN PACKAGE ON LOAD — and dirtying a vendor package
	// inside an editor with autosave on is exactly how `FOG-§6`'s read-only vendor rule gets
	// broken by a test. ⇒ the parent is covered by `TASK-1043`'s live read-back, and a failed
	// LOAD at runtime is covered by the `Error` log in `AFogVolume::SpawnFogVisual`.
	TestTrue(FString::Printf(TEXT("⭐⭐ The package the shipped code names is really on disk: '%s'"), *PackageName),
		FPackageName::DoesPackageExist(PackageName));

	// And the `Content/…uasset` form — the half an ARTIST acts on — DERIVED from the same path
	// rather than typed a second time.
	FString ExpectedFilename;
	if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, ExpectedFilename, TEXT(".uasset")))
	{
		TestTrue(FString::Printf(TEXT("The asset file exists where the package name says it does ('%s')"), *ExpectedFilename),
			FPaths::FileExists(ExpectedFilename));
	}
	else
	{
		AddError(FString::Printf(TEXT("⛔ '%s' could not be converted to a filename — the path is malformed, so this FAILS rather than reporting safe."), *PackageName));
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⚠️ LANE B (SOURCE-TEXT) — TEST 3: EXACTLY THREE EXITS, and every one of them reconciles
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualExitsTest,
	"Siegebound.Fog.EveryExitFromFoggedReconcilesTheVisualAndThereAreExactlyThreeOfThem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualExitsTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	// ⭐⭐⭐ THE ENUMERATION, MADE EXECUTABLE. `FogVolume.h`'s three-state block asserts that
	// `RaiseFog`, `ApplyBrightSun` and `ResetFog` are the ONLY writers of either deadline and
	// bans a third BY NAME. ⇒ the exits from FOGGED are EXACTLY THREE, and this count is what
	// keeps that sentence honest: ⛔ A FOURTH WRITER IS A FOURTH EXIT, and an exit with no
	// reconciler call is FOG THAT NEVER LIFTS.
	const int32 FogDeadlineWrites = CountOccurrencesInCode(FogCpp, TEXT("FogActiveUntilTimeSeconds ="));
	TestEqual(TEXT("⭐⭐⭐ The fog deadline is WRITTEN in exactly three places — RaiseFog (stamps), ApplyBrightSun (zeroes), ResetFog (zeroes). ⛔ A FOURTH is a fourth exit and needs its own RefreshFogVisual() call"),
		FogDeadlineWrites, 3);

	// Each of the three writes the deadline and THEN reconciles — order matters, because the
	// reconciler reads `IsFogActive()`, which reads what the assignment just wrote.
	struct FExit
	{
		const TCHAR* Signature;
		const TCHAR* What;
	};

	const FExit Exits[] =
	{
		{ RaiseFogSignature,       TEXT("(entry + refresh) RaiseFog — the fog rises, the box must appear") },
		{ ApplyBrightSunSignature, TEXT("(exit ii) ApplyBrightSun — 🧑 his live FogClear card; without this, 60 gold buys the removal of fog that is STILL ON SCREEN") },
		{ ResetFogSignature,       TEXT("(exit iii) ResetFog — without this, a FOG CORPSE survives into the next match") },
	};

	for (const FExit& Exit : Exits)
	{
		FString Body;
		if (!ExtractFunctionBody(*this, FogCpp, Exit.Signature, Body))
		{
			continue;
		}

		TestEqual(FString::Printf(TEXT("⭐⭐ %s calls RefreshFogVisual() exactly once"), Exit.What),
			CountOccurrencesInCode(Body, TEXT("RefreshFogVisual();")), 1);

		FString BeforeReconcile;
		if (SubstringBefore(*this, Body, TEXT("RefreshFogVisual();"), BeforeReconcile))
		{
			TestEqual(FString::Printf(TEXT("⭐ %s writes the deadline BEFORE it reconciles (a reconciler called first reads the OLD state and does exactly the wrong thing)"), Exit.What),
				CountOccurrencesInCode(BeforeReconcile, TEXT("FogActiveUntilTimeSeconds =")), 1);
		}
	}

	// ⛔ EXIT (i) — NATURAL EXPIRY — HAS NO WRITER AT ALL. The deadline simply passes and nothing
	// runs, which is why it is the one exit that has to be MANUFACTURED. Its manufacture is the
	// wake-up armed inside the reconciler, and this is the assertion that keeps it there:
	// ⛔ delete the SetTimer and the fog becomes PERMANENT with every other test still green.
	FString RefreshBody;
	if (ExtractFunctionBody(*this, FogCpp, RefreshFogVisualSignature, RefreshBody))
	{
		TestEqual(TEXT("⭐⭐⭐ EXIT (i) NATURAL EXPIRY is manufactured: the reconciler arms exactly one wake-up. ⛔ Remove it and fog NEVER LIFTS on screen, with a green suite"),
			CountOccurrencesInCode(RefreshBody, TEXT("SetTimer(")), 1);

		// ⛔ The wake-up instant is DERIVED from the deadline and NEVER from the duration: a
		// re-typed `FogDurationSeconds` would be a SECOND copy of the number and would be wrong on
		// every path but the very first cast (a refresh, a re-arm, a BrightSun that was refused).
		TestEqual(TEXT("⭐⭐ The reconciler never re-types FogDurationSeconds — its wake-up is derived from FogActiveUntilTimeSeconds, the ONE source of truth"),
			CountOccurrencesInCode(RefreshBody, TEXT("FogDurationSeconds")), 0);
		TestTrue(TEXT("⭐ …and it derives that wake-up from the deadline itself"),
			CountOccurrencesInCode(RefreshBody, TEXT("FogActiveUntilTimeSeconds")) >= 1);

		// ⭐ `IsFogActive()` STAYS THE ONE PREDICATE — the reconciler ASKS it rather than
		// remembering what it was told, which is what makes an early or late wake-up harmless.
		TestEqual(TEXT("⭐⭐ IsFogActive() is the one predicate the reconciler branches on"),
			CountOccurrencesInCode(RefreshBody, TEXT("IsFogActive()")), 1);
	}

	// ⛔ TEARDOWN is NOT a fourth exit — the state does not change, the state's OWNER dies — but
	// it must still take the visual with it, or a `Destroyed`/`LevelTransition` leaves a fully
	// opaque box behind with NOBODY HOLDING ITS REFERENCE.
	FString EndPlayBody;
	if (ExtractFunctionBody(*this, FogCpp, EndPlaySignature, EndPlayBody))
	{
		TestEqual(TEXT("⭐ Teardown destroys the visual (EndPlay also fires for Destroyed and LevelTransition, where the WORLD SURVIVES)"),
			CountOccurrencesInCode(EndPlayBody, TEXT("DestroyFogVisual();")), 1);
		TestEqual(TEXT("⭐ …and clears the wake-up, so no timer fires into a dying actor"),
			CountOccurrencesInCode(EndPlayBody, TEXT("ClearTimer(")), 1);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⚠️ LANE B (SOURCE-TEXT) — TEST 4: one spawn, one despawn, no tick, no second source of truth
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualLifetimeShapeTest,
	"Siegebound.Fog.TheVisualHasOneSpawnSiteAndOneDespawnSiteAndTheStateActorStillNeverTicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualLifetimeShapeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	FString FogCpp;
	FString FogH;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp) || !LoadProjectFile(*this, FogVolumeH, FogH))
	{
		return false;
	}

	// ── ⭐⭐ SPAWN AND DESPAWN, AS ONE SEAM ───────────────────────────────────────────────────
	FString RefreshBody;
	if (ExtractFunctionBody(*this, FogCpp, RefreshFogVisualSignature, RefreshBody))
	{
		TestEqual(TEXT("⭐⭐ THE SPAWN HALF: the reconciler spawns in exactly one place"),
			CountOccurrencesInCode(RefreshBody, TEXT("SpawnFogVisual();")), 1);
		TestEqual(TEXT("⭐⭐ THE DESPAWN HALF: the reconciler destroys in exactly one place. ⛔ 'A seam that can be entered and not left is HALF A SEAM' — a spawn-only build is PERMANENT FOG with a green suite"),
			CountOccurrencesInCode(RefreshBody, TEXT("DestroyFogVisual();")), 1);
	}

	FString SpawnBody;
	if (ExtractFunctionBody(*this, FogCpp, SpawnFogVisualSignature, SpawnBody))
	{
		TestEqual(TEXT("⭐⭐ The fog visual is spawned by exactly one SpawnActor call"),
			CountOccurrencesInCode(SpawnBody, TEXT("SpawnActor<AActor>(")), 1);

		// ⛔⛔ LOUD IN THE LOG, NEVER IN THE GAMEPLAY. 🧑 His spell VFX were INVISIBLE rather than
		// ERRORING because the cards' spawn is null-safe — that silence is why nobody noticed.
		// ⚠️⚠️ THIS EXPECTATION MOVED FROM 2 TO 3 UNDER `TASK-1072`, AND THE MOVE IS DECLARED RATHER
		// THAN QUIETLY MADE (`TL-§5b` — ⛔ a red is a finding to route, never a number to overwrite;
		// this one is a THIRD ERROR SITE THIS DIFF DELIBERATELY ADDS, not a probe drifting off its
		// subject). The third site is the ⛔ SCALE-READBACK MISMATCH: `SpawnFogVisual` now reads
		// `GetActorScale3D()` back off the spawned actor and screams if the engine did not give it
		// what it asked for. ⭐ The mismatch could have been hidden from this count by moving it into
		// a private helper — ⛔ and that is exactly what a probe counting error sites exists to stop,
		// so it stays inline and the number is corrected in the open.
		TestEqual(TEXT("⭐⭐ A missing, refused OR WRONGLY-SCALED visual is announced at Error level, three times: a class that will not load, a world that refuses the spawn, and a scale the engine substituted"),
			CountOccurrencesInCode(SpawnBody, TEXT("LogGitClaudeUnrealTest, Error,")), 3);

		// ⚖️ AND THE BOUNDARY OF "LOUD": the fog MECHANIC keeps working with no visual. An art
		// failure may never refuse a 50-gold card, consume-and-abort, or change one clamp.
		TestEqual(TEXT("⚖️ ⛔ The spawn path never returns a refusal — it is void, and an art failure may NEVER brick a 50-gold card"),
			CountOccurrencesInCode(SpawnBody, TEXT("return false")), 0);
		TestEqual(TEXT("⚖️ ⛔ …and it never touches a fog deadline, so a failed visual cannot change one clamp"),
			CountOccurrencesInCode(SpawnBody, TEXT("FogActiveUntilTimeSeconds")), 0);

		// ⛔ The box must never be BAKED INTO A SAVED LEVEL: `L_Arena` is never-save law, and a
		// world-sized opaque actor persisted into the map would be unreachable forever (`Find`
		// cannot see it — `BP_SiegeFog` is not an `AFogVolume` subclass).
		TestEqual(TEXT("⛔ The spawned visual is RF_Transient — it can never be written into a saved level"),
			CountOccurrencesInCode(SpawnBody, TEXT("RF_Transient")), 1);
	}

	FString DestroyBody;
	if (ExtractFunctionBody(*this, FogCpp, DestroyFogVisualSignature, DestroyBody))
	{
		TestEqual(TEXT("⭐⭐ The despawn really destroys the actor (exactly one Destroy call)"),
			CountOccurrencesInCode(DestroyBody, TEXT("->Destroy();")), 1);
		TestEqual(TEXT("⭐ …and clears the handle, so nothing can ever observe a pointer to an actor on its way out"),
			CountOccurrencesInCode(DestroyBody, TEXT("FogVisualActor = nullptr;")), 1);
	}

	// ── ⛔ NO TICK. `TASK-004`'s never-per-tick law, and `FOG-§10.1`'s one-source rule ────────
	TestEqual(TEXT("⛔⛔ The state actor still NEVER TICKS — giving the visual a lifetime did not buy a poll"),
		CountOccurrencesInCode(FogCpp, TEXT("PrimaryActorTick.bCanEverTick = false;")), 1);
	TestEqual(TEXT("⛔ …and nothing turned ticking on"),
		CountOccurrencesInCode(FogCpp, TEXT("bCanEverTick = true")), 0);
	TestEqual(TEXT("⛔ There is no Tick override on this class"),
		CountOccurrencesInCode(FogH, TEXT("virtual void Tick(")), 0);

	// ── ⛔⛔ NO SECOND SOURCE OF TRUTH. `FogVolume.h` bans a companion flag BY NAME; this is the
	// executable form of that ban, extended to the shapes THIS row could have introduced. ⛔ The
	// `TObjectPtr` handle is NOT one of them: it is the actor's own presence, not an answer to
	// "is fog up?", and nothing branches on fog state by reading it.
	const TCHAR* BannedFlags[] =
	{
		TEXT("bFogActive"),
		TEXT("bFogVisible"),
		TEXT("bFogCleared"),
		TEXT("bFogPrevented"),
		TEXT("bVisualSpawned"),
		TEXT("FogVisualUntilTimeSeconds"),
		TEXT("FogVisualDeadline"),
	};

	for (const TCHAR* Banned : BannedFlags)
	{
		TestEqual(FString::Printf(TEXT("⛔⛔ No '%s' anywhere in FogVolume.cpp — two representations of one fact is how a state machine starts answering differently in two places"), Banned),
			CountOccurrencesInCode(FogCpp, Banned), 0);
		TestEqual(FString::Printf(TEXT("⛔⛔ …nor in FogVolume.h ('%s')"), Banned),
			CountOccurrencesInCode(FogH, Banned), 0);
	}

	// The visual handle exists and is declared exactly once, `Transient`, on the state owner.
	TestEqual(TEXT("⭐ The visual is HELD, not re-found: exactly one TObjectPtr<AActor> handle. ⛔ BP_SiegeFog is not an AFogVolume subclass, so TActorIterator<AFogVolume> NEVER sees it and a Find-style sweep returns nothing, SILENTLY"),
		CountOccurrencesInCode(FogH, TEXT("TObjectPtr<AActor> FogVisualActor;")), 1);
	TestEqual(TEXT("⛔ …and nothing tries to re-find the visual with an iterator"),
		CountOccurrencesInCode(FogCpp, TEXT("TActorIterator<AActor>")), 0);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⚠️ LANE B (SOURCE-TEXT) — TEST 5: one asset reference, and the owner keeps its boundaries
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualOwnershipTest,
	"Siegebound.Fog.TheVisualAssetIsNamedInExactlyOnePlaceAndNoCallerOutsideTheStateOwnerKnowsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualOwnershipTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	FString FogCpp;
	FString FogH;
	FString SpellCpp;
	FString GameMode;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp)
		|| !LoadProjectFile(*this, FogVolumeH, FogH)
		|| !LoadProjectFile(*this, SpellLibraryCpp, SpellCpp)
		|| !LoadProjectFile(*this, GameModeCpp, GameMode))
	{
		return false;
	}

	// ⭐⭐ ONE REFERENCE, IN A FUNCTION A TEST CAN HOLD. ⛔ The asset path is never inlined at a
	// call site — that is the whole reason `FogVisualClassPath()` exists rather than a literal.
	FString ClassPathBody;
	if (ExtractFunctionBody(*this, FogCpp, FogVisualClassPathSignature, ClassPathBody))
	{
		// ⚠️ TWO, not one: a single path literal spells the asset twice — once as the PACKAGE and
		// once as the `_C` GENERATED CLASS (`/Game/Blueprints/BP_SiegeFog.BP_SiegeFog_C`). The
		// claim is "one literal", and this is what one literal counts as.
		TestEqual(TEXT("⭐⭐ The fog visual asset is named in exactly ONE path literal in the whole project, inside AFogVolume::FogVisualClassPath"),
			CountOccurrencesInCode(ClassPathBody, TEXT("BP_SiegeFog")), 2);
	}

	TestEqual(TEXT("⛔ BP_SiegeFog appears on CODE lines of FogVolume.cpp only inside that one path literal"),
		CountOccurrencesInCode(FogCpp, TEXT("/Game/Blueprints/BP_SiegeFog")), 1);
	TestEqual(TEXT("⛔ …and never on a code line of the header (the header names it in PROSE only, which is where a design note belongs)"),
		CountOccurrencesInCode(FogH, TEXT("BP_SiegeFog")), 0);

	FString SpawnBody;
	if (ExtractFunctionBody(*this, FogCpp, SpawnFogVisualSignature, SpawnBody))
	{
		TestEqual(TEXT("⛔ The spawn site holds no /Game/ literal of its own — it asks FogVisualClassPath()"),
			CountOccurrencesInCode(SpawnBody, TEXT("/Game/")), 0);
		TestTrue(TEXT("⭐ …and it really consults that one function"),
			CountOccurrencesInCode(SpawnBody, TEXT("FogVisualClassPath()")) >= 1);
	}

	// ⭐⭐ THE BOUNDARY THAT `TASK-998`'s `SC-§62` EXCEPTION DEPENDS ON: the game mode may tell the
	// volume to clear; it may not know a duration, a ceiling, a density, a window ⛔ OR AN ASSET
	// PATH. `TASK-982` kept `ASiegeGameMode::PlayAgain` byte-unchanged and so does this row.
	const TCHAR* OwnerOnlySymbols[] = { TEXT("RefreshFogVisual"), TEXT("SpawnFogVisual"), TEXT("DestroyFogVisual"), TEXT("BP_SiegeFog") };
	for (const TCHAR* Symbol : OwnerOnlySymbols)
	{
		TestEqual(FString::Printf(TEXT("⭐ ASiegeGameMode learns no fog policy — '%s' appears nowhere in it"), Symbol),
			CountOccurrencesInCode(GameMode, Symbol), 0);
		TestEqual(FString::Printf(TEXT("⭐ USpellLibrary learns no fog policy either — '%s' appears nowhere in it"), Symbol),
			CountOccurrencesInCode(SpellCpp, Symbol), 0);
	}

	// ⛔ The card path is untouched by this row: the spell library still reaches fog state through
	// the two doors it always used, and neither of them grew a visual argument.
	TestTrue(TEXT("The FogCover / FogClear arms still go through the WRITE door (FindOrSpawn) exactly as before"),
		CountOccurrencesInCode(SpellCpp, TEXT("AFogVolume::FindOrSpawn(World)")) == 2);

	// ⛔⛔ THE GEOMETRY IS DERIVED, AND NO EXTENT LITERAL IS TYPED (`SC-§34`). The measured
	// transform is `loc (0,0,7000)`, `scale (640,360,260)` with bounds `±32000 / ±18000` — ⛔ none
	// of those numbers may appear on a code line of the production file.
	// ⚠️ DECLARED LIMITATION: these are SUBSTRING matches, so a future code line containing e.g.
	// `1260` would trip `260`. ⛔ That direction is the SAFE one — a FALSE ALARM, never a false
	// pass — and `SC-§38` prefers a probe that goes red on a stale coordinate to one that goes
	// quietly green.
	const TCHAR* BannedLiterals[] = { TEXT("640"), TEXT("360"), TEXT("260"), TEXT("32000"), TEXT("18000"), TEXT("7000"), TEXT("26000"), TEXT("12000") };
	for (const TCHAR* Literal : BannedLiterals)
	{
		TestEqual(FString::Printf(TEXT("⛔⛔ No hand-typed '%s' on any code line of FogVolume.cpp — the box is DERIVED from ArenaHalfExtent, so resizing the arena moves it"), Literal),
			CountOccurrencesInCode(FogCpp, Literal), 0);
	}

	// ⭐ …and it is derived from the ONE owner of the arena extent, in exactly one place.
	TestEqual(TEXT("⭐ The arena extent is read from its one owner, once"),
		CountOccurrencesInCode(FogCpp, TEXT("GetDefault<USiegeScatterConfig>()->ArenaHalfExtent")), 1);

	FString TransformBody;
	if (ExtractFunctionBody(*this, FogCpp, FogVisualTransformSignature, TransformBody))
	{
		// ⛔ A body that ignored its parameters would return the same box for every arena and
		// would pass a census that only looked for absent literals.
		TestTrue(TEXT("⭐ The transform really uses the arena half-extent it is handed (X)"),
			CountOccurrencesInCode(TransformBody, TEXT("ArenaHalfExtentUU.X")) >= 1);
		TestTrue(TEXT("⭐ …and on Y"),
			CountOccurrencesInCode(TransformBody, TEXT("ArenaHalfExtentUU.Y")) >= 1);
		TestTrue(TEXT("⭐ …and the ground datum it is handed"),
			CountOccurrencesInCode(TransformBody, TEXT("GroundReferenceZUU")) >= 1);
		TestTrue(TEXT("⭐ …and the two named constants, rather than the numbers behind them"),
			CountOccurrencesInCode(TransformBody, TEXT("FogVisualHorizontalMarginUU")) >= 1
			&& CountOccurrencesInCode(TransformBody, TEXT("FogVisualCeilingAboveGroundUU")) >= 1);
	}

	// The two non-derivable numbers are transcribed EXACTLY ONCE EACH, in the header, beside their
	// derivation — ⛔ never at a call site (`TASK-1068` cl. 5).
	TestEqual(TEXT("⭐ The 6,000 uu margin (L_Arena's VolumetricFogDistance — an actor in a map we may never save) is written once, in the header"),
		CountOccurrencesInCode(FogH, TEXT("FogVisualHorizontalMarginUU = 6000.f;")), 1);
	TestEqual(TEXT("⭐ The 20,000 uu ceiling (a tuned choice with no owner in code) is written once, in the header"),
		CountOccurrencesInCode(FogH, TEXT("FogVisualCeilingAboveGroundUU = 20000.f;")), 1);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⭐ LANE A (EXECUTED) — TEST 6: the scale the ENGINE substituted, rejected by the shipped detector
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualScaleReadbackTest,
	"Siegebound.Fog.TheScaleReadbackRejectsTheEngineSubstitutionThatMadeTheFogInvisibleThreeTimes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualScaleReadbackTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	// ⛔⛔⛔ THE DEFECT THIS TEST EXISTS TO MAKE UNREPEATABLE, AND IT IS THE ONE THAT ACTUALLY COST
	// 🧑 JONATHAN THREE *"NO FOG"* REPORTS (`TASK-1071`): `AFogVolume::SpawnFogVisual` asked for a
	// scale of `(640, 360, 260)` and the ENGINE handed back `(20, 20, 5)` — the vendor component
	// template's own scale, substituted at `SCS_Node.cpp:147` because `BP_SiegeFog`'s root is an
	// INHERITED SCS component rather than a native one. ⇒ a `64,000 × 36,000 × 26,000` uu volume
	// around the battlefield silently became a `2,000 × 2,000 × 500` uu slab `6,750` uu ABOVE it
	// and `~21,200` uu from his hero, outside the `6000` uu froxel grid in every direction.
	// ⭐ MEASURED against a zero-control at his own gameplay vantage: `+0.04 %` mean luma at the
	// achieved scale — INSIDE the pixel noise floor — against `+73 %` at the intended one.
	// ⛔⛔ AND THE EVERY-EXISTING-TEST-STAYED-GREEN PART, WHICH IS WHY THIS FILE NEEDED A SIXTH
	// TEST AT ALL: `FogVisualTransform` was RIGHT, and test 1 above proves it is right, in detail,
	// on five separate relations. ⛔ NONE OF THAT MATTERED, because nothing asserted the number
	// ever reached the actor. *"A test that passes when the scale is clobbered is not a test."*

	// ── ⭐ THE MEASURED SUBSTITUTE ────────────────────────────────────────────────────────────
	// ⚠️ TYPED HERE ON PURPOSE, AND ONLY HERE: this is ⛔ NOT a design number we own and ⛔ NOT
	// something derivable from our code. It is a VENDOR fact — `RelativeScale3D` read live off
	// `/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE` under `TASK-1071` §2 —
	// and it belongs in the TEST rather than in the production file, which is why `SC-§34`'s
	// no-literals rule (enforced against `FogVolume.cpp` in test 5) does not reach it.
	// ⚠️ IF THE VENDOR PACK IS EVER UPDATED this constant goes stale. ⛔ That direction is SAFE:
	// a stale donor value makes this test assert the detector rejects some OTHER wrong scale,
	// which it still must. The claim degrades, it does not invert.
	const FVector VendorTemplateScale(20.0, 20.0, 5.0);

	// ── The intended scale, ⛔ DERIVED from the shipped function, never transcribed ────────────
	const FVector IntendedScale = AFogVolume::FogVisualTransform(ShippedArenaHalfExtent(), 0.f).GetScale3D();

	TestTrue(TEXT("The derived scale is strictly positive on every axis (a degenerate one would make every claim below vacuous)"),
		IntendedScale.X > 0.0 && IntendedScale.Y > 0.0 && IntendedScale.Z > 0.0);

	// ⛔ THE PREMISE, ASSERTED RATHER THAN ASSUMED: the substitute really is a different box. If a
	// future arena resize ever made the two coincide, the `TestFalse` below would be asserting
	// nothing at all — and it would still be GREEN, which is the shape this whole row is about.
	TestTrue(TEXT("⛔ PREMISE: the vendor template's scale really does differ from the derived one on every axis (otherwise the rejection below proves nothing)"),
		!FMath::IsNearlyEqual(IntendedScale.X, VendorTemplateScale.X, 1.0)
		&& !FMath::IsNearlyEqual(IntendedScale.Y, VendorTemplateScale.Y, 1.0)
		&& !FMath::IsNearlyEqual(IntendedScale.Z, VendorTemplateScale.Z, 1.0));

	// ── ⭐⭐⭐ THE ASSERTION THE OLD BUILD COULD NOT HAVE MADE ─────────────────────────────────
	// ⛔ THIS IS THE ONE. Hand the shipped detector exactly what the engine actually handed the
	// shipped code, and it must answer NO. A build in which this returns `true` is a build in
	// which 🧑 he plays a 50-gold card, the army goes 87.8% blind, and the screen does not change
	// — with a fully green suite, which is precisely what happened.
	TestFalse(TEXT("⭐⭐⭐ THE ENGINE'S SUBSTITUTION IS REJECTED: the shipped detector says NO when handed the vendor template scale the engine really substituted (TASK-1071 §2). ⛔ A build where this is YES is a build with no fog and a green suite"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, VendorTemplateScale));

	// ⭐ AND THE POSITIVE CONTROL, so the NO above is a JUDGEMENT rather than a detector that
	// refuses everything (`SC-§79`, `SHIP-§9`: validate a gate against the failure it detects
	// ⛔ AND against the success it must not block).
	TestTrue(TEXT("⭐ POSITIVE CONTROL: the detector says YES when the achieved scale IS the derived one — it is a judgement, not a blanket refusal"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, IntendedScale));

	// ── The tolerance is real, and it is TIGHT ────────────────────────────────────────────────
	// ⛔ A future *"the test is flaky"* edit that widened this by orders of magnitude would be the
	// first step back toward the silence, so both directions are pinned.
	const double Tolerance = static_cast<double>(AFogVolume::FogVisualScaleTolerance);
	TestTrue(TEXT("The scale tolerance is strictly positive (a zero tolerance would fail on the float round-trip through UpdateComponentToWorld)"),
		Tolerance > 0.0);

	const FVector JustInsideTolerance = IntendedScale + FVector(Tolerance * 0.25, 0.0, 0.0);
	const FVector WellOutsideTolerance = IntendedScale + FVector(Tolerance * 100.0, 0.0, 0.0);
	TestTrue(TEXT("⭐ A perturbation well inside the tolerance still MATCHES — the float round-trip a scale takes through the component hierarchy must not cry wolf"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, JustInsideTolerance));
	TestFalse(TEXT("⭐ …and a perturbation well outside it does NOT — the tolerance is a rounding allowance, not a blindfold"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, WellOutsideTolerance));

	// ⛔ EVERY AXIS IS CHECKED, not just the first: a substitution that got X right and Z wrong
	// would still be a box of the wrong shape, and Z is the axis the vendor template misses by the
	// widest factor.
	TestFalse(TEXT("⛔ A mismatch on Y ALONE is caught (a per-axis check, never a magnitude comparison)"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, FVector(IntendedScale.X, VendorTemplateScale.Y, IntendedScale.Z)));
	TestFalse(TEXT("⛔ A mismatch on Z ALONE is caught — Z is the axis the vendor template misses by the widest factor, and a short box is a fog ceiling nobody asked for"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, FVector(IntendedScale.X, IntendedScale.Y, VendorTemplateScale.Z)));

	// ⛔ NaN IS A MISMATCH. A scale that cannot be compared has not been achieved, and the
	// fail-toward-loud direction is the one every degenerate input in this class takes.
	// ⚠️ CONSTRUCTED AND READ, ⛔ NEVER ARITHMETIC'D — see `MakeQuietNaN`'s ENABLE_NAN_DIAGNOSTIC
	// note. `FogVisualScaleMatches` short-circuits on `ContainsNaN()` before `FVector::Equals`
	// would subtract anything, which is what makes these two rows safe to run at all.
	const double NaNValue = MakeQuietNaN();
	const FVector NaNScale(NaNValue, NaNValue, NaNValue);
	TestTrue(TEXT("SELF-CHECK: the fixture's NaN really is a NaN. ⛔ A folded constant here would make both rows below vacuous — which is exactly how a totality guard gets certified without ever being exercised"),
		NaNScale.ContainsNaN());
	TestFalse(TEXT("⛔ A NaN achieved scale is a MISMATCH — a value that cannot be compared has not been achieved"),
		AFogVolume::FogVisualScaleMatches(IntendedScale, NaNScale));
	TestFalse(TEXT("⛔ …and so is a NaN REQUEST, from the other side"),
		AFogVolume::FogVisualScaleMatches(NaNScale, IntendedScale));

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⚠️ LANE B (SOURCE-TEXT) — TEST 7: the spawn site FORCES the scale, then logs what it GOT
// ─────────────────────────────────────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVisualAchievedTransformTest,
	"Siegebound.Fog.TheSpawnSiteForcesTheScaleAndLogsWhatItGotRatherThanWhatItAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVisualAchievedTransformTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	FString FogCpp;
	FString FogH;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp) || !LoadProjectFile(*this, FogVolumeH, FogH))
	{
		return false;
	}

	FString SpawnBody;
	if (!ExtractFunctionBody(*this, FogCpp, SpawnFogVisualSignature, SpawnBody))
	{
		return false;
	}

	// ── ⭐⭐⭐ (a) THE REPAIR ITSELF ───────────────────────────────────────────────────────────
	// ⛔ Delete this line and the engine's substitution stands: the fog box goes back to being a
	// small slab thousands of units above the field, invisible from every camera angle.
	TestEqual(TEXT("⭐⭐⭐ THE REPAIR: the spawn site FORCES the derived scale onto the actor after spawning it, exactly once. ⛔ Without it the engine substitutes the vendor template's scale and there is NO FOG (TASK-1071)"),
		CountOccurrencesInCode(SpawnBody, TEXT("Spawned->SetActorScale3D(RequestedScale3D);")), 1);

	// ⛔⛔ THE TRAP, PINNED BY NAME. `SpawnParams.TransformScaleMethod` is the obvious reach and it
	// ⛔ CHANGES NOTHING — the `bIsDefaultTransform` block runs AFTER that switch is consulted and
	// overwrites the scale whichever method was chosen. A future editor "simplifying" the explicit
	// set into that flag would restore the bug in full, so the reach itself goes RED here.
	TestEqual(TEXT("⛔⛔ The fix is NOT spelled as SpawnParams.TransformScaleMethod — that switch is consulted BEFORE the clobber and changes nothing (TASK-1071 §2's named trap)"),
		CountOccurrencesInCode(FogCpp, TEXT("TransformScaleMethod")), 0);

	// ── ⭐⭐⭐ (b) THE INSTRUMENT — AND THIS IS THE HALF THAT LET THE BUG SURVIVE ──────────────
	// ⛔⛔ THE OLD LOG PRINTED `SpawnTransform`, i.e. THE REQUEST. It never once read the actor.
	// ⇒ every sentence it wrote was true, the one number that mattered was never taken, and a box
	// the engine had shrunk by a factor of 32 read as a CLEAN SPAWN — which is how a gate passed
	// and 🧑 he was told the actor had spawned "at the right transform".
	// ⭐ THE STANDING LAW, now executable: ⛔ AN INSTRUMENT THAT ECHOES THE REQUEST INSTEAD OF
	// MEASURING THE RESULT IS NOT AN INSTRUMENT.
	TestTrue(TEXT("⭐⭐⭐ THE INSTRUMENT: the spawn site READS THE SCALE BACK off the spawned actor. ⛔ The old line echoed the request and never called this"),
		CountOccurrencesInCode(SpawnBody, TEXT("Spawned->GetActorScale3D()")) >= 1);
	TestTrue(TEXT("⭐⭐ …and the LOCATION too — the old line asserted a Z it had likewise never looked at, and half a readback is the same bug with better odds"),
		CountOccurrencesInCode(SpawnBody, TEXT("Spawned->GetActorLocation()")) >= 1);

	// ⛔⛔ AND THE REQUEST IS NEVER RE-SPELLED AT A REPORTING SITE. `SpawnTransform.GetScale3D()`
	// is spent EXACTLY ONCE, deriving the named local; every later use is the local. ⇒ restoring
	// `SpawnTransform.GetScale3D().X` into the log's argument list — the literal old bug — pushes
	// this count to 4 and goes RED.
	TestEqual(TEXT("⭐⭐⭐ The requested scale is derived ONCE and never re-spelled at a reporting site. ⛔ Putting SpawnTransform.GetScale3D().X back into the log args is the original defect, and it reds HERE"),
		CountOccurrencesInCode(SpawnBody, TEXT("SpawnTransform.GetScale3D()")), 1);
	TestEqual(TEXT("⭐⭐ …and the LOCATION half of that lie is gone entirely: the log's Z comes from the ACTOR, never from the spawn transform"),
		CountOccurrencesInCode(SpawnBody, TEXT("SpawnTransform.GetLocation()")), 0);

	// ⭐ The achieved values really reach the log, rather than being read and discarded — a
	// readback nobody prints is a readback nobody can act on.
	TestTrue(TEXT("⭐ The ACHIEVED scale reaches the log line"),
		CountOccurrencesInCode(SpawnBody, TEXT("AchievedScale3D.X")) >= 1);
	TestTrue(TEXT("⭐ …and the ACHIEVED Z does too"),
		CountOccurrencesInCode(SpawnBody, TEXT("AchievedLocation.Z")) >= 1);

	// ── ⭐⭐ (c) A DISAGREEMENT IS COMPARED, NOT MERELY PRINTED ───────────────────────────────
	// ⛔ Printing both numbers and trusting a human to diff them is not a gate: nobody reads a
	// `Log`-level line until something is already wrong. The comparison is made in code, and it is
	// LOUD, which is what the artist's note asks for.
	TestEqual(TEXT("⭐⭐ The readback is COMPARED against the request in code, exactly once — printing both and hoping a human diffs them is not a gate"),
		CountOccurrencesInCode(SpawnBody, TEXT("FogVisualScaleMatches(RequestedScale3D, AchievedScale3D)")), 1);
	TestEqual(TEXT("⭐⭐ …and the MISMATCH is the branch that fires (a NEGATED predicate — a match must stay silent, or the loud case stops meaning anything)"),
		CountOccurrencesInCode(SpawnBody, TEXT("if (!FogVisualScaleMatches(")), 1);

	// ⚖️ AND THE BOUNDARY OF "LOUD", RESTATED FOR THE NEW SITE: a wrong scale is an ART failure,
	// and an art failure may ⛔ NEVER refuse a 50-gold card or change one vision clamp. Test 4
	// pins the `return false` and deadline counts at zero for this whole body; this is the same
	// ruling applied to the branch this row adds.
	// ⚠️ THREE, and the number is DERIVED rather than guessed: the spawn path's early exits are
	// (1) no world, (2) the class would not load, (3) the world refused the spawn. ⛔ A FOURTH
	// `return;` would mean the mismatch branch had learned to abort — which would hand an ART
	// failure the power to leave the visual UNHELD, i.e. an opaque box nobody can ever destroy.
	TestEqual(TEXT("⚖️ ⛔ The mismatch branch REPORTS and carries on — the spawn path still has exactly its three original early exits, so a wrong scale can never leave the visual unheld and undestroyable"),
		CountOccurrencesInCode(SpawnBody, TEXT("return;")), 3);

	// ── ⭐⭐ (d) ORDERING — the two ways to get this exactly backwards ────────────────────────
	// ⛔ A readback taken BEFORE the correction measures the clobber and screams on every single
	// cast, forever; a correction attempted BEFORE the spawn has no actor to apply to. Both
	// compile, both review clean, and both are silent about which one happened.
	FString BeforeReadback;
	if (SubstringBefore(*this, SpawnBody, TEXT("Spawned->GetActorScale3D()"), BeforeReadback))
	{
		TestEqual(TEXT("⭐⭐ THE CORRECTION HAPPENS BEFORE THE MEASUREMENT — a readback taken first would measure the engine's substitute and scream on every cast forever"),
			CountOccurrencesInCode(BeforeReadback, TEXT("SetActorScale3D(")), 1);
	}

	FString BeforeCorrection;
	if (SubstringBefore(*this, SpawnBody, TEXT("Spawned->SetActorScale3D(RequestedScale3D);"), BeforeCorrection))
	{
		TestEqual(TEXT("⭐ …and the SPAWN happens before the correction — the clobber is applied during SpawnActor, so a scale set any earlier is the value that gets discarded"),
			CountOccurrencesInCode(BeforeCorrection, TEXT("SpawnActor<AActor>(")), 1);
	}

	// ── ⭐ (e) THE DETECTOR IS A REACHABLE SEAM, not an inline expression ─────────────────────
	// ⛔⛔ THIS IS WHY TEST 6 CAN EXIST AT ALL. The same comparison written inline at the spawn
	// site would be a check ⛔ NO TEST COULD EVER RUN — and an unrunnable check is the exact shape
	// of the thing this row is repairing, one layer up.
	TestEqual(TEXT("⭐⭐ The comparison is a PUBLIC STATIC SEAM a test can execute (the BrightSunWindowSeconds precedent). ⛔ Inlined at the call site it would be a check nothing could ever run — which is the defect, one layer up"),
		CountOccurrencesInCode(FogH, TEXT("static bool FogVisualScaleMatches(const FVector& RequestedScale3D, const FVector& AchievedScale3D);")), 1);
	TestEqual(TEXT("⭐ …declared once, defined once"),
		CountOccurrencesInCode(FogCpp, TEXT("bool AFogVolume::FogVisualScaleMatches(")), 1);
	TestEqual(TEXT("⭐ The tolerance is a NAMED constant carrying its derivation, never a bare epsilon at the comparison"),
		CountOccurrencesInCode(FogH, TEXT("static constexpr float FogVisualScaleTolerance = 0.01f;")), 1);

	// ⛔ AND THE ROW'S STANDING FENCES SURVIVE IT: no second deadline, no bool, no tick. The
	// readback stores NOTHING — it is read, compared, logged and dropped inside one function.
	// ⚠️ THE NEEDLE CARRIES ITS SEMICOLON ON PURPOSE. `AchievedScale3D` DOES appear in the header —
	// it is the second PARAMETER NAME on the seam's declaration — so a bare needle would be 1 and
	// this row would assert nothing it means. `AchievedScale3D;` is the shape a MEMBER FIELD takes
	// and the parameter list (`AchievedScale3D);`) can never wear.
	TestEqual(TEXT("⛔ The readback introduces NO stored state — the achieved scale is read, compared, logged and dropped inside one function; it is never a member (the TObjectPtr stays the only thing this class remembers about the visual)"),
		CountOccurrencesInCode(FogH, TEXT("AchievedScale3D;")), 0);
	TestEqual(TEXT("⛔ …and IsFogActive() is still the one predicate: the spawn site learns nothing about fog STATE from a scale"),
		CountOccurrencesInCode(SpawnBody, TEXT("IsFogActive()")), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
