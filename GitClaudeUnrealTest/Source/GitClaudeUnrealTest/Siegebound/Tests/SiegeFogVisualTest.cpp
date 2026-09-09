// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Engine/EngineBaseTypes.h"      // TASK-1173: FURL — InitializeActorsForPlay's argument
#include "Engine/ExponentialHeightFog.h" // TASK-1173: the height-fog actor AFogVolume::FindHeightFogComponent looks for — a world without one makes EnforceFogRenderFloor log an Error, i.e. a RED for a property of the RIG
#include "Engine/Engine.h"               // TASK-1173: GEngine — CreateNewWorldContext / DestroyWorldContext / ShutdownWorldNetDriver
#include "Engine/World.h"                // TASK-1173: UWorld::CreateWorld — ⛔ the FIRST real world in this project's suite
#include "EngineUtils.h"                 // TASK-1173: TActorIterator / FActorRange — finding the spawned visual and routing EndPlay on teardown
#include "GameFramework/Actor.h"         // TASK-1173: AActor::RouteEndPlay (explicit IWYU — no compile verifies a transitive pull)
#include "GitClaudeUnrealTest.h"         // TASK-1173: LogGitClaudeUnrealTest — the category whose Log lines ARE this row's proof of execution
#include "HAL/IConsoleManager.h"         // TASK-1173: reading r.VolumetricFog BACK, to prove the integrity floor was RELEASED and not left stranded across the suite
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

	//~ ⭐⭐⭐ TASK-1147 — the integrity floor's pair. ⛔ A stale signature FAILS (`SC-§38`).
	const TCHAR* EnforceFogRenderFloorSignature = TEXT("void AFogVolume::EnforceFogRenderFloor()");
	const TCHAR* ReleaseFogRenderFloorSignature = TEXT("void AFogVolume::ReleaseFogRenderFloor()");

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

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⭐ LANE A (EXECUTED) — TEST 8: THE INTEGRITY FLOOR'S RE-ENTRANCE GUARD (`TASK-1147`)
// ─────────────────────────────────────────────────────────────────────────────────────────────
//
// ⛔⛔⛔ THE DEFECT THIS TEST EXISTS FOR, AND THE BOARD NAMED IT AS THE MOST LIKELY BUG IN THE ROW:
// `RefreshFogVisual()` runs on ⛔ EVERY raise, ⛔ EVERY refresh and ⛔ EVERY timer wake-up. A
// second enforce that RE-CAPTURED would read back the ⛔ ALREADY-FLOORED machine and record ⛔ THE
// FLOOR ITSELF as the player's own choice ⇒ the release would then "restore" the floor, i.e.
// become a ⛔ PERMANENT NO-OP, silently ⛔ UPGRADING a Low-settings player's shadows for the rest
// of his session, with ⛔ nothing in any log.
//
// ⭐⭐⭐ `SC-§104`: EVERY ROW BELOW ASSERTS ⛔ STATE — ⛔ WHICH VALUE IS HELD — AND ⛔ NEVER A TALLY.
// A call count here is ⛔ EQUAL ON BOTH BRANCHES by construction: the fixed build and the broken
// build both call the capture ⛔ TWICE. The only thing that differs is ⛔ WHAT IS IN IT.
// ⚠️ AND THE ROWS THAT DO ⛔ NOT DISCRIMINATE ARE ⛔ LABELLED RATHER THAN QUIETLY COUNTED
// (`SC-§104` cl. 5a): the FIRST capture is ⛔ IDENTICAL on both branches, so every assertion about
// it is a ⛔ FIXTURE SELF-CHECK, not evidence. The ⭐ rows are the ⛔ SECOND capture.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRenderFloorGuardTest,
	"Siegebound.Fog.TheIntegrityFloorCannotRecordItsOwnFlooredValuesAsThePlayersChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRenderFloorGuardTest::RunTest(const FString& Parameters)
{
	// ⭐ A PLAYER AT SHADOWS=LOW WHO ARRIVED THERE FROM EPIC. Two facts in one fixture, and both
	// are the real shipped situation rather than round numbers:
	//   • the switch reads 0 — [ShadowQuality@0] wrote it, and that IS the bug he can exploit;
	//   • the GRID still reads the EPIC figures, because @0 and @1 never RESET the grid and
	//     scalability applies only a section's own lines. That is the hysteresis the floor's grid
	//     clause exists for, and it is what makes these numbers distinguishable from the floor's.
	FFogRenderFloorObservation PlayerAtLow;
	PlayerAtLow.VolumetricFogEnabled = 0;
	PlayerAtLow.GridPixelSize = 8;
	PlayerAtLow.GridSizeZ = 128;
	PlayerAtLow.bHeightFogVolumetricEnabled = true;
	PlayerAtLow.bHeightFogFound = true;

	// ⛔ THE SAME MACHINE, ONE REFRESH LATER, WITH THE FLOOR ALREADY ON IT. This is exactly what a
	// re-entrant enforce would read back — and recording it would be the defect.
	FFogRenderFloorObservation TheFlooredMachine;
	TheFlooredMachine.VolumetricFogEnabled = AFogVolume::FogRenderFloorVolumetricFogOn;
	TheFlooredMachine.GridPixelSize = AFogVolume::FogRenderFloorGridPixelSize;
	TheFlooredMachine.GridSizeZ = AFogVolume::FogRenderFloorGridSizeZ;
	TheFlooredMachine.bHeightFogVolumetricEnabled = true;
	TheFlooredMachine.bHeightFogFound = true;

	// ⛔ THE FIXTURE IS ONLY MEANINGFUL IF THE TWO OBSERVATIONS DISAGREE. If a future edit made the
	// floor's grid equal the Epic grid, every ⭐ row below would go green on both branches and this
	// test would silently stop measuring anything — the exact SC-§104 disease, arriving through
	// the fixture instead of through the assertion.
	TestNotEqual(TEXT("FIXTURE SELF-CHECK: the player's switch and the floor's switch DIFFER, or nothing below discriminates"),
		PlayerAtLow.VolumetricFogEnabled, TheFlooredMachine.VolumetricFogEnabled);
	TestNotEqual(TEXT("FIXTURE SELF-CHECK: the hysteresis grid and the floor's grid DIFFER too"),
		PlayerAtLow.GridPixelSize, TheFlooredMachine.GridPixelSize);

	// ── ⭐⭐ (a) THE RELEASED STATE IS THE NOT-ENGAGED ONE ────────────────────────────────────
	// ⛔ Not a tautology: a release that forgot to reset would leave the guard UP, and then the
	// NEXT match's enforce would hit it and never floor at all — the exploit back, one match
	// later, with a green suite and nothing in any log.
	const FFogRenderFloorPriorState Released = AFogVolume::ReleasedFogRenderFloorState();
	TestFalse(TEXT("⭐⭐ A released floor is NOT engaged — so the next match can floor again. ⛔ Leave the guard up and the fog stops being floored for the rest of the session"),
		AFogVolume::IsFogRenderFloorEngaged(Released));

	// ── (b) THE FIRST CAPTURE — ⚠️ FIXTURE SELF-CHECKS, NOT EVIDENCE ─────────────────────────
	// ⛔ IDENTICAL ON BOTH BRANCHES of the guard mutation, and said so here rather than counted as
	// a red: with or without the early return, the FIRST capture records the observation.
	const FFogRenderFloorPriorState AfterFirst = AFogVolume::CaptureFogRenderFloorPriorState(Released, PlayerAtLow);
	TestTrue(TEXT("FIXTURE SELF-CHECK (equal on both branches): the first enforce engages the floor"),
		AFogVolume::IsFogRenderFloorEngaged(AfterFirst));
	TestEqual(TEXT("FIXTURE SELF-CHECK (equal on both branches): the first enforce records the PLAYER's switch"),
		AfterFirst.Observed.VolumetricFogEnabled, PlayerAtLow.VolumetricFogEnabled);
	TestEqual(TEXT("FIXTURE SELF-CHECK (equal on both branches): …and the grid it actually found, hysteresis and all"),
		AfterFirst.Observed.GridPixelSize, PlayerAtLow.GridPixelSize);

	// ── ⭐⭐⭐ (c) THE SECOND CAPTURE — ⛔ THIS IS THE WHOLE TEST ─────────────────────────────
	// ⛔ BOTH BRANCHES, COMPUTED (SC-§104 cl. 5a): FIXED holds 0 / 8 / 128 (the player's);
	// BROKEN holds 1 / 16 / 64 (the floor's). ⛔ NOT EQUAL ⇒ these are genuine reds.
	const FFogRenderFloorPriorState AfterSecond = AFogVolume::CaptureFogRenderFloorPriorState(AfterFirst, TheFlooredMachine);
	TestEqual(TEXT("⭐⭐⭐ A SECOND enforce does NOT re-capture: the recorded switch is STILL the player's, never the floor's. ⛔ Broken, the release restores the FLOOR and silently upgrades a Low-settings player's shadows for the rest of his session"),
		AfterSecond.Observed.VolumetricFogEnabled, PlayerAtLow.VolumetricFogEnabled);
	TestEqual(TEXT("⭐⭐ …and the recorded GRID is still the one the player arrived with, not the one the floor wrote"),
		AfterSecond.Observed.GridPixelSize, PlayerAtLow.GridPixelSize);
	TestEqual(TEXT("⭐⭐ …on the Z axis too"),
		AfterSecond.Observed.GridSizeZ, PlayerAtLow.GridSizeZ);

	// ⛔ A THIRD, FOURTH AND FIFTH CALL CHANGE NOTHING EITHER. The reconciler really is re-entered
	// many times per fog window (raise, refresh, BrightSun refusal, wake-up), so "idempotent" has
	// to mean idempotent, not "survives exactly one repeat".
	FFogRenderFloorPriorState Repeated = AfterSecond;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Repeated = AFogVolume::CaptureFogRenderFloorPriorState(Repeated, TheFlooredMachine);
	}
	TestEqual(TEXT("⭐⭐ …and it is still the player's value after five enforces — 'idempotent' means idempotent, not 'survives one repeat'"),
		Repeated.Observed.VolumetricFogEnabled, PlayerAtLow.VolumetricFogEnabled);

	// ── ⭐⭐ (d) RELEASE THEN RE-ENGAGE — the NEXT match must capture AFRESH ──────────────────
	// ⛔ BOTH BRANCHES, COMPUTED: FIXED records the machine's live 1; a release that failed to
	// reset returns a still-engaged state whose Observed is the default 0. ⛔ NOT EQUAL ⇒ a red.
	const FFogRenderFloorPriorState AfterReRaise =
		AFogVolume::CaptureFogRenderFloorPriorState(AFogVolume::ReleasedFogRenderFloorState(), TheFlooredMachine);
	TestEqual(TEXT("⭐⭐ After a release, the NEXT fog captures the machine AFRESH rather than reusing a stale record"),
		AfterReRaise.Observed.VolumetricFogEnabled, TheFlooredMachine.VolumetricFogEnabled);
	TestTrue(TEXT("⭐ …and it is engaged again"),
		AFogVolume::IsFogRenderFloorEngaged(AfterReRaise));

	// ── ⛔ (e) A COMPONENT NEVER OBSERVED IS NEVER INVENTED ──────────────────────────────────
	// ⛔ Without bHeightFogFound, the default-constructed `false` beside it would look like a
	// genuine reading and the release would TURN OFF a fog nobody had turned on.
	FFogRenderFloorObservation NoHeightFog;
	NoHeightFog.VolumetricFogEnabled = 0;
	NoHeightFog.bHeightFogFound = false;
	NoHeightFog.bHeightFogVolumetricEnabled = false;
	const FFogRenderFloorPriorState WithoutComponent =
		AFogVolume::CaptureFogRenderFloorPriorState(AFogVolume::ReleasedFogRenderFloorState(), NoHeightFog);
	TestFalse(TEXT("⛔ A level with no height fog is RECORDED as such, so the release cannot write a component it never read"),
		WithoutComponent.Observed.bHeightFogFound);
	TestTrue(TEXT("⛔ …and it still engages, because the CVAR half of the floor is real either way"),
		AFogVolume::IsFogRenderFloorEngaged(WithoutComponent));

	// ── ⛔ (f) THE FLOOR'S GRID IS THE CHEAPEST ONE THE ENGINE SHIPS FOG ON ──────────────────
	// ⛔ Asserted as the ENGINE DEFAULTS rather than as literals typed twice: [ShadowQuality@2]
	// writes 16 / 64 and GVolumetricFogGridPixelSize / GVolumetricFogGridSizeZ default to the
	// same pair. ⛔ A finer grid here charges the cheap-PC player GFX-§8 was written for; a
	// coarser one thins a fog the army is 87.8% blind behind.
	TestEqual(TEXT("⛔ The floor runs fog on the [ShadowQuality@2] grid — the cheapest the engine ships it on, and the engine's own default"),
		AFogVolume::FogRenderFloorGridPixelSize, 16);
	TestEqual(TEXT("⛔ …on Z as well"),
		AFogVolume::FogRenderFloorGridSizeZ, 64);

	return true;
}

// ─────────────────────────────────────────────────────────────────────────────────────────────
// ⚠️ LANE B (SOURCE-TEXT) — TEST 9: the floor is CALLED, from the ONE reconciler, on BOTH branches
// ─────────────────────────────────────────────────────────────────────────────────────────────
//
// ⛔⛔ `SC-§83`'s ADDENDUM: WHEN A FIX HAS A HELPER AND A CALL SITE, AT LEAST ONE MUTATION TARGETS
// THE CALL SITE. Test 8 proves the guard is CORRECT; it cannot prove the floor is ⛔ REACHED.
// ⭐ This project has already shipped a built, integration-checked, committed asset with ⛔ ZERO
// CALLERS while nothing failed (`SC-§36.1`, and it is the very defect the rest of this file
// exists for) — so a mutation set that only breaks the callee cannot detect that state.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRenderFloorWiringTest,
	"Siegebound.Fog.TheIntegrityFloorIsEngagedAndReleasedByTheOneReconcilerAndByNothingElse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRenderFloorWiringTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVisualFixture;

	FString FogCpp;
	FString FogH;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp) || !LoadProjectFile(*this, FogVolumeH, FogH))
	{
		return false;
	}

	// ── ⭐⭐⭐ (a) THE CALL SITES: ONE ENFORCE, AND ⛔ TWO RELEASES ─────────────────────────────
	// ⛔ `FOG-§12.5`: the floor is TAKEN from the ONE reconciler and from NOWHERE ELSE — a second
	// enforce site is a second source of truth for a fact that has one, the same rule that makes
	// RefreshFogVisual the only function allowed to spawn the visual.
	// ⛔⛔ THE RELEASE IS NOT SYMMETRIC WITH IT, AND THE ASYMMETRY IS THE FINDING (TASK-1148
	// BLOCKER-1): what the enforce takes hold of is PROCESS-WIDE and OUTLIVES THIS ACTOR, so the
	// release has to run on every way this actor can stop holding it — the reconciler's
	// fog-is-down branch AND teardown. ⭐ That is the same two-site shape DestroyFogVisual()
	// already has, for the same reason, and the release's own doc has always claimed it.
	TestEqual(TEXT("⭐⭐⭐ THE FLOOR IS CALLED: exactly one EnforceFogRenderFloor() call in the whole file. ⛔ Delete it and the fog's PRESENCE goes back to being a graphics option, with every other test in this suite still green"),
		CountOccurrencesInCode(FogCpp, TEXT("EnforceFogRenderFloor();")), 1);
	TestEqual(TEXT("⭐⭐⭐ …and exactly TWO ReleaseFogRenderFloor() calls — the reconciler's down branch AND teardown. ⛔ 'A seam that can be entered and not left is HALF A SEAM': at ONE site, leaving a match with fog up strands the player's console variables at SetByCode for the whole PROCESS"),
		CountOccurrencesInCode(FogCpp, TEXT("ReleaseFogRenderFloor();")), 2);

	// ⭐⭐⭐ AND THE ANALOGY THE DECLARATION MAKES IS NOW EXECUTABLE. `FogVolume.h` says the release
	// is called "exactly like DestroyFogVisual()" — this row is what stops that sentence being
	// prose. ⛔ It reddens the day a THIRD way out of this actor is added and only one of the two
	// is let go there, which is precisely how BLOCKER-1 arrived: an exit nobody enumerated.
	TestEqual(TEXT("⭐⭐⭐ THE PAIRING IS THE RULE: the release has exactly as many call sites as the despawn. ⛔ A new way out that takes the box down and leaves the FLOOR up is the defect this row shipped with, one loop ago"),
		CountOccurrencesInCode(FogCpp, TEXT("ReleaseFogRenderFloor();")),
		CountOccurrencesInCode(FogCpp, TEXT("DestroyFogVisual();")));

	// ── ⭐⭐⭐ (a2) TEARDOWN — THE SITE THE FIRST DRAFT MISSED ─────────────────────────────────
	// ⛔ EndPlay is NOT a fourth exit from FOGGED: the state does not change there, its OWNER
	// ceases to exist — and that is exactly why it must release. Teardown never reaches the
	// reconciler, so nothing else can. ⛔ Reachable in the shipped game, not hypothetically:
	// SessionMenuWidget's Back → USiegeSessionSubsystem::LeaveMatch() → an ABSOLUTE OpenLevel to
	// L_MainMenu, with fog still up ⇒ EndPlay(LevelTransition). And L_MainMenu is the ONLY place
	// the graphics panel is reachable, so the stranded pin lands exactly where the player acts.
	FString EndPlayBody;
	if (ExtractFunctionBody(*this, FogCpp, EndPlaySignature, EndPlayBody))
	{
		TestEqual(TEXT("⭐⭐⭐ TEARDOWN LETS THE FLOOR GO: exactly one ReleaseFogRenderFloor() in EndPlay. ⛔ Without it, leaving a match (or stopping PIE) with fog up pins r.VolumetricFog and both grid axes at SetByCode for the rest of the PROCESS — the player's Shadows slider silently refused at the main menu while the panel prints 'ambient fog OFF'. ⛔ INVISIBLE: no release runs, so no Error line exists"),
			CountOccurrencesInCode(EndPlayBody, TEXT("ReleaseFogRenderFloor();")), 1);
		TestEqual(TEXT("⭐⭐ …beside the despawn, in the same order as the fog-is-down branch (release, then destroy), so the two exit sites read the same way"),
			CountOccurrencesInCode(EndPlayBody, TEXT("DestroyFogVisual();")), 1);

		// ⛔ TEARDOWN HOLDS NO POLICY EITHER — it does not re-decide anything, it lets go. An
		// EnforceFogRenderFloor() here would floor the cvars of a world that is dying, and a
		// RefreshFogVisual() here would take the fog-is-UP branch and re-spawn during teardown.
		TestEqual(TEXT("⛔ Teardown NEVER enforces — flooring a dying world's cvars is the leak with the sign flipped"),
			CountOccurrencesInCode(EndPlayBody, TEXT("EnforceFogRenderFloor();")), 0);
		TestEqual(TEXT("⛔ …and never calls the reconciler, which would take the fog-is-UP branch and re-spawn the box mid-teardown"),
			CountOccurrencesInCode(EndPlayBody, TEXT("RefreshFogVisual();")), 0);
	}

	FString RefreshBody;
	if (ExtractFunctionBody(*this, FogCpp, RefreshFogVisualSignature, RefreshBody))
	{
		// ⛔ …and both of those single calls are inside the ONE reconciler, not merely somewhere.
		TestEqual(TEXT("⭐⭐ The enforce lives in the ONE reconciler"),
			CountOccurrencesInCode(RefreshBody, TEXT("EnforceFogRenderFloor();")), 1);
		TestEqual(TEXT("⭐⭐ …and so does the release"),
			CountOccurrencesInCode(RefreshBody, TEXT("ReleaseFogRenderFloor();")), 1);

		// ── ⭐⭐ (b) EACH ON ITS OWN BRANCH, AND THE ORDER IS THE CLAIM ──────────────────────
		// ⛔ Both on the fog-is-up branch would mean the floor is never released; both on the
		// fog-is-down branch would mean it is never engaged. BOTH COMPILE AND BOTH REVIEW CLEAN.
		// ⇒ the release must appear BEFORE the enforce in the source, because the down-branch
		// returns early — and the enforce must appear BEFORE the spawn, so the first frames of
		// every fog render through an already-floored path.
		FString BeforeEnforce;
		if (SubstringBefore(*this, RefreshBody, TEXT("EnforceFogRenderFloor();"), BeforeEnforce))
		{
			TestEqual(TEXT("⭐⭐ THE RELEASE IS ON THE FOG-IS-DOWN BRANCH: it sits before the enforce, i.e. inside the early-returning !IsFogActive() block"),
				CountOccurrencesInCode(BeforeEnforce, TEXT("ReleaseFogRenderFloor();")), 1);
			TestEqual(TEXT("⭐ …and that branch really does return before reaching the enforce (the no-world guard plus the fog-is-down exit)"),
				CountOccurrencesInCode(BeforeEnforce, TEXT("return;")), 2);
			TestEqual(TEXT("⭐⭐ THE ENFORCE PRECEDES THE SPAWN — floored after the box exists would leave the first frames of every fog looking exactly like the bug, and those are the frames 🧑 he watches for the card to take effect"),
				CountOccurrencesInCode(BeforeEnforce, TEXT("SpawnFogVisual();")), 0);
			TestEqual(TEXT("⭐ …and the release really is paired with the despawn on that same branch"),
				CountOccurrencesInCode(BeforeEnforce, TEXT("DestroyFogVisual();")), 1);
		}

		// ⛔ THE RECONCILER STILL DECIDES NOTHING ITSELF. It does not read the tunable, does not
		// touch a console variable and does not know a cvar name — it calls the pair, exactly as
		// it calls Spawn/Destroy. A floor spelled inline here would be a second policy site.
		TestEqual(TEXT("⛔ The reconciler holds no floor POLICY — it never reads the tunable"),
			CountOccurrencesInCode(RefreshBody, TEXT("bEnforceFogRenderFloor")), 0);
		TestEqual(TEXT("⛔ …and never touches a console variable itself"),
			CountOccurrencesInCode(RefreshBody, TEXT("IConsoleManager")), 0);
	}

	// ── ⭐⭐⭐ (c) THE RELEASE IS **NOT** GATED ON THE TUNABLE, AND THAT IS A REAL FINDING ────
	// ⛔ If the release honoured bEnforceFogRenderFloor, flipping the switch off while the floor
	// was HELD would STRAND the console variables at code priority FOREVER — killing the Shadows
	// slider's effect on fog for the rest of the session, silently. ⇒ a release must always be
	// able to let go of something an earlier enforce took. ⛔ The symmetry is the bug here.
	FString ReleaseBody;
	if (ExtractFunctionBody(*this, FogCpp, ReleaseFogRenderFloorSignature, ReleaseBody))
	{
		TestEqual(TEXT("⭐⭐⭐ THE RELEASE NEVER READS bEnforceFogRenderFloor. ⛔ Gate it and flipping the switch off mid-fog pins the player's console variables at code priority for the rest of the session, with nothing in any log"),
			CountOccurrencesInCode(ReleaseBody, TEXT("bEnforceFogRenderFloor")), 0);

		// ⭐⭐ UNSET, NOT A CAPTURED-LITERAL WRITE-BACK — the row's one substantive departure from
		// the shape FOG-§12.5 sketched, and the reason is that a Set-based restore leaves the
		// variable pinned at code priority forever after.
		TestEqual(TEXT("⭐⭐⭐ The release UNSETS the code layer on all three floored variables. ⛔ Set(prior, ECVF_SetByCode) instead would PIN them at code priority forever and silently kill the Shadows slider's effect on fog"),
			CountOccurrencesInCode(ReleaseBody, TEXT("Unset(ECVF_SetByCode)")), 3);
		TestEqual(TEXT("⛔ …and it never writes a console variable back by value — that is the failure mode Unset was chosen over"),
			CountOccurrencesInCode(ReleaseBody, TEXT("->Set(")), 0);

		// ⛔ THE COMPONENT HALF IS THE ONE THING THAT MUST RESTORE A CAPTURED VALUE, because a
		// map-actor property has no priority stack to fall back through — and it is guarded by
		// the observation, so a component we never read is never written.
		TestEqual(TEXT("⭐⭐ The component half restores the CAPTURED value, never a literal — a map-actor property has no priority stack to fall back through"),
			CountOccurrencesInCode(ReleaseBody, TEXT("SetVolumetricFog(Prior.bHeightFogVolumetricEnabled)")), 1);
		TestEqual(TEXT("⛔ …and only when one was actually observed. Without this guard the default-constructed false would look like a reading and TURN OFF a fog nobody turned on"),
			CountOccurrencesInCode(ReleaseBody, TEXT("Prior.bHeightFogFound")), 1);

		// ⛔ AND THE GUARD IS RESET, ON EVERY PATH OUT. A release that reported a failure and kept
		// the guard up would make the NEXT match's enforce a no-op — one bad session becoming
		// every session after it.
		TestEqual(TEXT("⭐⭐ The guard is reset exactly once, outside both log branches, so a failure path cannot leave the floor 'engaged' forever"),
			CountOccurrencesInCode(ReleaseBody, TEXT("FogRenderFloorPriorState = ReleasedFogRenderFloorState();")), 1);
	}

	// ── ⭐⭐ (d) THE ENFORCE READS BEFORE IT WRITES, AND READS BACK AFTER (`SC-§94` cl. B) ────
	FString EnforceBody;
	if (ExtractFunctionBody(*this, FogCpp, EnforceFogRenderFloorSignature, EnforceBody))
	{
		// ⛔ THE OBSERVATION IS TAKEN BEFORE ANY WRITE. Taken afterwards it would record the
		// floor's own values — the same defect as a second capture, arriving through ordering
		// rather than through re-entry, and invisible to test 8.
		FString BeforeFirstWrite;
		if (SubstringBefore(*this, EnforceBody, TEXT("->Set("), BeforeFirstWrite))
		{
			TestTrue(TEXT("⭐⭐⭐ THE READ HAPPENS BEFORE THE FIRST WRITE — a capture taken afterwards records the FLOOR's values, which is the row's defect arriving through ORDERING instead of through re-entry"),
				CountOccurrencesInCode(BeforeFirstWrite, TEXT("Observed.VolumetricFogEnabled = ")) == 1
				&& CountOccurrencesInCode(BeforeFirstWrite, TEXT("CaptureFogRenderFloorPriorState(")) == 1);
		}

		// ⛔ THE TRANSITION GOES THROUGH THE PURE FUNCTION. Spelled inline, the guard would be a
		// check NO TEST COULD RUN — which is exactly what FogVisualScaleMatches exists to avoid,
		// one function up.
		TestEqual(TEXT("⭐⭐ The guard is the PURE function a test can execute, called exactly once — inlined here it would be a check nothing could ever run"),
			CountOccurrencesInCode(EnforceBody, TEXT("CaptureFogRenderFloorPriorState(")), 1);

		// ⭐⭐⭐ AND THE INSTRUMENT MEASURES THE MACHINE, NEVER THE REQUEST. This is the half that
		// let the fog stay invisible for three of 🧑 his reports, one function down.
		TestTrue(TEXT("⭐⭐⭐ THE INSTRUMENT: the enforce READS THE VALUES BACK off the console after writing them. ⛔ A log that echoed the request would report a floor that a higher-priority console write had defeated"),
			CountOccurrencesInCode(EnforceBody, TEXT("AchievedVolumetricFog")) >= 2);
		TestTrue(TEXT("⭐⭐ …and it reads the height-fog component back too — half a readback is the same bug with better odds"),
			CountOccurrencesInCode(EnforceBody, TEXT("bAchievedHeightFogVolumetric")) >= 2);

		// ⛔ THE GRID GOES WITH THE SWITCH. Without it the floor makes the CHEAPEST setting run the
		// MOST EXPENSIVE fog, because [ShadowQuality@0]/@1 never reset the grid.
		TestEqual(TEXT("⭐⭐ The grid is floored alongside the switch — three writes, not one. ⛔ Switch-only, an Epic→Low player keeps the EPIC froxel grid and the floor bills him for it"),
			CountOccurrencesInCode(EnforceBody, TEXT("->Set(")), 3);

		// ⚖️ AND THE BOUNDARY OF "LOUD", INHERITED FROM THE SPAWN PATH: an integrity floor that
		// fails is REPORTED, never a refusal. It may not change one clamp, refuse a 50-gold card
		// or touch a deadline.
		TestEqual(TEXT("⚖️ ⛔ The floor never touches a fog deadline — a rendering failure may NEVER change one vision clamp"),
			CountOccurrencesInCode(EnforceBody, TEXT("FogActiveUntilTimeSeconds")), 0);
		TestEqual(TEXT("⚖️ ⛔ …and never asks whether fog is up: the reconciler already decided that, and re-asking would be a second predicate"),
			CountOccurrencesInCode(EnforceBody, TEXT("IsFogActive()")), 0);
	}

	// ── ⭐ (e) THE SEAMS ARE REACHABLE, AND THE CVAR NAMES ARE WRITTEN ONCE ──────────────────
	// ⛔ The FogVisualClassPath discipline applied to a console variable: a name typed at a call
	// site is a name no test can hold.
	TestEqual(TEXT("⭐⭐ The state transition is a PUBLIC STATIC SEAM a test can execute"),
		CountOccurrencesInCode(FogH, TEXT("static FFogRenderFloorPriorState CaptureFogRenderFloorPriorState(")), 1);
	TestEqual(TEXT("⭐ …declared once, defined once"),
		CountOccurrencesInCode(FogCpp, TEXT("FFogRenderFloorPriorState AFogVolume::CaptureFogRenderFloorPriorState(")), 1);
	// ⛔ THE CVAR NAMES HAVE EXACTLY ONE HOME EACH, and it is a definition rather than a call site —
	// the FogVisualClassPath discipline applied to a console variable. ⛔ A name typed at a call
	// site is a name no test can hold, and a mistyped one floors a variable that does not exist
	// while every read-back below reports the machine it failed to change.
	TestEqual(TEXT("⭐ The three floored cvar names are DECLARED in the header, once each (the SettingsSlotName house pattern)"),
		CountOccurrencesInCode(FogH, TEXT("static const TCHAR* FogRenderFloorCVar")), 3);
	TestEqual(TEXT("⭐ …and each string literal is written exactly once, in the implementation"),
		CountOccurrencesInCode(FogCpp, TEXT("TEXT(\"r.VolumetricFog")), 3);
	if (!EnforceBody.IsEmpty())
	{
		TestEqual(TEXT("⛔ The ENFORCE holds no cvar-name literal of its own — it asks the named constants"),
			CountOccurrencesInCode(EnforceBody, TEXT("TEXT(\"r.VolumetricFog")), 0);
	}
	if (!ReleaseBody.IsEmpty())
	{
		TestEqual(TEXT("⛔ …and neither does the RELEASE, so the two halves can never disagree about WHICH variable they own"),
			CountOccurrencesInCode(ReleaseBody, TEXT("TEXT(\"r.VolumetricFog")), 0);
	}

	// ⛔ THE THREE FORBIDDEN SHORTCUTS (`FOG-§12.2`), AS AN EXECUTABLE BAN. Each looks like the
	// obvious fix and each is a blocker at the gate rather than a nit.
	const TCHAR* ForbiddenShortcuts[] =
	{
		TEXT("MarkPackageDirty"),   // ⛔ (i) the map is NEVER saved (GFX-§11)
		TEXT("SavePackage"),        // ⛔ (i) likewise
		TEXT("GConfig"),            // ⛔ (ii) an ini line scalability would overwrite at every quality change
		TEXT("SetQualityLevels"),   // ⛔ (iii) clamping the Shadows group charges the cheap-PC player for the whole match
		TEXT("ScalabilityQuality"), // ⛔ (iii) likewise
	};
	for (const TCHAR* Shortcut : ForbiddenShortcuts)
	{
		TestEqual(FString::Printf(TEXT("⛔⛔ FOG-§12.2's forbidden shortcuts stay refused — no '%s' anywhere in FogVolume.cpp"), Shortcut),
			CountOccurrencesInCode(FogCpp, Shortcut), 0);
	}

	// ⛔ AND THE FOG MECHANIC IS UNTOUCHED BY THIS ROW: the floor makes the VISUAL match the
	// MECHANIC, it does not retune either. The clamp lives behind FSiegeFogStatics and this class
	// still asks it nothing.
	TestEqual(TEXT("⛔ The integrity floor never reaches the fog MECHANIC — zero FSiegeFogStatics anywhere in FogVolume.cpp"),
		CountOccurrencesInCode(FogCpp, TEXT("FSiegeFogStatics")), 0);

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐⭐ LANE C — ⛔ **GENUINELY EXECUTED, IN A REAL WORLD** (`TASK-1173`; law ⭐ `SC-§113`)
//  TEST 10: `AFogVolume::RaiseFog()` IS ⛔ ACTUALLY CALLED, THE VISUAL ⛔ ACTUALLY SPAWNS, AND
//           `ResetFog()` ⛔ ACTUALLY TAKES IT AWAY AGAIN.
// ═════════════════════════════════════════════════════════════════════════════════════════════
//
//  ⛔⛔⛔ READ THE FILE HEADER'S *"WHAT NEITHER LANE COVERS"* PARAGRAPH FIRST — ⛔ THIS TEST IS THE
//  THING IT SAYS DOES NOT EXIST. Verbatim, and true until this test landed:
//      *"there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so ⛔ NOTHING HERE RUNS
//       THE ACTUAL SPAWN. The end-to-end claim — play `Fog`, a box appears; wait, it goes — is
//       ⛔ NOT EXECUTED by this file and is ⛔ NOT executed by the suite."*
//  ⇒ ⛔ THAT SENTENCE IS NOW ⛔ HALF FALSE, and ⛔ ONLY half: the ⛔ SPAWN and the ⛔ DESPAWN are
//  executed here. ⛔ The 300-second WAIT is not, and ⛔ neither is anything about how the fog
//  ⛔ LOOKS — 🧑 his eye remains the only instrument for legibility (`AS-§6 A(e)`).
//
//  ⛔⛔ WHY IT MATTERS MORE THAN AN ORDINARY TEST, said plainly: ⭐ `SC-§113` recorded that
//  ⛔ NOBODY IN THIS PIPELINE COULD EXECUTE ⛔ ONE LINE of `AFogVolume`. `ApplyFogVisualMaterial`
//  and `VerifyFogVisualMaterial` were built, reviewed, mutation-tested and ⛔ NEVER RUN. A `grep`
//  over the entire editor log for `AFogVolume` returned ⛔ ZERO lines — and ⛔ an error that
//  ⛔ CANNOT fire and one that ⛔ CHOSE not to fire ⛔ produce byte-identical logs. ⇒ every gate
//  this lane ever passed was, in part, ⛔ ceremony. ⛔ THIS TEST IS THE POSITIVE CONTROL THAT ENDS
//  THAT (`SC-§113` cl. 3(c)).
//
//  ⭐⭐⭐ AND THE PART THAT IS ⛔ NOT MINE, WHICH IS THE BEST PART: ⛔ I ASSERT ALMOST NOTHING ABOUT
//  THE FOG'S CORRECTNESS HERE, ⛔ ON PURPOSE. `FogVolume.cpp` already contains ⛔ SEVEN `Error`
//  sites, and the automation framework routes ⛔ EVERY `UE_LOG(..., Error, ...)` raised during a
//  test into `AddError` ⇒ ⛔ THE SHIPPED INSTRUMENTS BECOME THIS TEST'S ASSERTIONS, for free, and
//  they redden on ⛔ their own terms rather than on a paraphrase of them I typed here. That is
//  worth more than any predicate I could add: the visual failing to load, the world refusing the
//  spawn, ⛔ THE `TASK-1071` SCALE SUBSTITUTION RETURNING, and the integrity floor not taking are
//  ⛔ ALL now suite-visible. ⛔ DO NOT ADD `AddExpectedError` TO THIS TEST — that would restore the
//  exact silence `SC-§113` was written about.
//
//  ⚠️⚠️ DECLARED, BECAUSE IT IS A REAL COST AND A REVIEWER MUST WEIGH IT: this test ⛔ LOADS
//  `/Game/Blueprints/BP_SiegeFog` and, through it, the ⛔ READ-ONLY VENDOR PACK it is parented to.
//  `TASK-841` §5.3 measured that loading a vendor package ⛔ DIRTIES it. ⛔ Nothing here saves
//  anything, and the sanctioned runner (`UnrealEditor-Cmd.exe … -unattended … ;Quit`) never
//  saves either ⇒ safe in the lane we actually use. 🚨 ⛔ IT IS ⛔ NOT SAFE TO RUN THIS FROM THE
//  SESSION FRONTEND IN A LIVE EDITOR AND THEN PRESS *SAVE ALL* (`FOG-§6`, `GFX-§11`). ⛔ The world
//  itself is created inside `GetTransientPackage()`, so ⛔ no map is touched — the hazard is the
//  vendor asset chain only, and it is disclosed rather than dodged.
// ═════════════════════════════════════════════════════════════════════════════════════════════

namespace SiegeFogRealWorldFixture
{
	/**
	 *  ⛔⛔ A REAL, PLAYING `UWorld` — ⛔ THE FIRST ONE IN THIS PROJECT'S SUITE, AND THAT IS A
	 *  ⛔ DELIBERATE PRECEDENT BREAK RATHER THAN AN OVERSIGHT BEING CORRECTED.
	 *  ⛔ Nine files under `Siegebound/Tests/` currently assert, in their own headers, that
	 *  *"every automation test in this project is HEADLESS — there is not one `UWorld::CreateWorld`
	 *  and not one `SpawnActor` anywhere in `Siegebound/Tests/`"*. ⇒ ⛔ THOSE SENTENCES ARE NOW
	 *  STALE (`SC-§91`), and this comment is where a reader who trusted one of them lands.
	 *  ⛔ They are ⛔ NOT edited here: they are PROSE in files this row does not own, the claim they
	 *  each make is about their ⛔ OWN reasoning, and a nine-file sweep inside a capability row is
	 *  how a capability row becomes a refactor. ⛔ The divergence is DECLARED to the gate instead.
	 *
	 *  ⭐ THE SHAPE IS ⛔ NOT INVENTED — it is the engine's own `FActorTestSpawner`
	 *  (`Developer/CQTest/Private/Components/ActorTestSpawner.cpp`), copied step for step:
	 *  context ⇒ world ⇒ root ⇒ `SetCurrentWorld` ⇒ `InitializeActorsForPlay` ⇒ (ours) `BeginPlay`.
	 *  ⛔ Writing a bespoke one would be inventing a rig whose failure modes nobody has paid for.
	 *
	 *  ⛔ `GetTransientPackage()` AS THE WORLD PACKAGE IS LOAD-BEARING, ⛔ not tidiness: it is what
	 *  guarantees this test can ⛔ NEVER dirty or save a map, which is the one thing `GFX-§11`
	 *  cares about.
	 *
	 *  ⚠️ `BeginPlay()` IS CALLED, and the reason is measured rather than assumed: this project has
	 *  ⛔ ZERO `UWorldSubsystem`s (all six of its subsystems are `UGameInstanceSubsystem`s, and this
	 *  world has no game instance), so `UWorld::BeginPlay` runs ⛔ no project code — it is null-safe
	 *  on the absent game mode (`World.cpp`, `GetAuthGameMode()` branch). ⛔ Without it the world
	 *  never sets `bBegunPlay`, so `AActor::RouteEndPlay` would be a ⛔ NO-OP at teardown and
	 *  `AFogVolume::EndPlay` — which is what RELEASES the integrity floor — would ⛔ never run.
	 */
	struct FScopedPlayWorld
	{
		UWorld* World = nullptr;

		FScopedPlayWorld()
		{
			if (!GEngine)
			{
				return;
			}

			const FName WorldName = MakeUniqueObjectName(
				nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique);

			// ⛔ THE WORLD IS CREATED ⛔ BEFORE THE CONTEXT, which is the ONE place this deviates
			// from `FActorTestSpawner` and it is deliberate: `DestroyWorldContext` is keyed BY
			// WORLD, so a context created first and then orphaned by a failed `CreateWorld` could
			// not be cleaned up at all. ⛔ Creating the world first makes the failure path a plain
			// early return with nothing leaked. The ORDER of the two calls is otherwise immaterial
			// — `SetCurrentWorld` is what binds them, and it still runs after both.
			World = UWorld::CreateWorld(
				EWorldType::Game, /*bInformEngineOfWorld=*/ false, WorldName, GetTransientPackage());

			if (!World)
			{
				return;
			}

			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);

			World->AddToRoot();
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FScopedPlayWorld()
		{
			if (!World || !GEngine)
			{
				return;
			}

			// ⛔ `RouteEndPlay` FIRST, and it is the reason `BeginPlay` was called above: this is
			// what fires `AFogVolume::EndPlay`, which RELEASES the integrity floor. ⛔ A teardown
			// that skipped it would leave `r.VolumetricFog` pinned at `SetByCode` for the ⛔ REST
			// OF THE SUITE PROCESS — 553 other tests running under a console variable this one
			// stranded, with nothing red anywhere.
			if (World->AreActorsInitialized())
			{
				for (AActor* const Actor : FActorRange(World))
				{
					if (Actor)
					{
						Actor->RouteEndPlay(EEndPlayReason::LevelTransition);
					}
				}
			}

			GEngine->ShutdownWorldNetDriver(World);
			World->DestroyWorld(/*bInformEngineOfWorld=*/ true);
			World->SetPhysicsScene(nullptr);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
			World = nullptr;
		}

		FScopedPlayWorld(const FScopedPlayWorld&) = delete;
		FScopedPlayWorld& operator=(const FScopedPlayWorld&) = delete;
	};

	/**
	 *  The fog visual, found the way the shipped code makes it findable: `SpawnFogVisual` sets
	 *  `SpawnParams.Owner = this`, so the visual is the state actor's ⛔ ONE owned actor.
	 *  ⛔ Deliberately ⛔ NOT a class-name match and ⛔ NOT a second `TryLoadClass`: asking the
	 *  OWNERSHIP measures the wiring the production code actually established, whereas a class
	 *  sweep would answer *"an actor of that class exists"* — a ⛔ weaker and different claim that
	 *  would stay green if the handle were never attached to anything.
	 */
	static AActor* FindOwnedVisual(UWorld* World, const AActor* Owner)
	{
		if (!World || !Owner)
		{
			return nullptr;
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* const Candidate = *It;
			if (IsValid(Candidate) && Candidate->GetOwner() == Owner)
			{
				return Candidate;
			}
		}

		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogRaisePathActuallyExecutesTest,
	"Siegebound.Fog.RaiseFogIsActuallyExecutedInARealWorldAndTheVisualAppearsThenGoes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogRaisePathActuallyExecutesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogRealWorldFixture;

	FScopedPlayWorld Scoped;
	if (!Scoped.World)
	{
		AddError(TEXT("⛔ The test world could not be created, so NOTHING below ran. ⛔ Read this as 'NOT MEASURED', never as a pass."));
		return false;
	}

	UWorld* const World = Scoped.World;

	// ── ⭐ (0) THE HEIGHT-FOG ACTOR — ⛔ FIDELITY, ⛔ NOT A WORKAROUND ────────────────────────────
	//  `L_Arena` has an `AExponentialHeightFog`; `AFogVolume::EnforceFogRenderFloor` looks for one
	//  and logs an ⛔ `Error` when there is none, because in a real match its absence means terms
	//  5-7 of `ShouldRenderVolumetricFog` are unsatisfiable. ⇒ a bare world would red this test on
	//  a condition that is ⛔ TRUE OF THE RIG rather than of the code — ⛔ exactly the artefact
	//  `SC-§112` is about. Spawning one makes the world resemble the shipping world; it does
	//  ⛔ NOT suppress the check, and a floor that failed for any OTHER reason still reds.
	AExponentialHeightFog* const HeightFog = World->SpawnActor<AExponentialHeightFog>();
	TestNotNull(TEXT("⭐ (0) The test world has an AExponentialHeightFog, as L_Arena does — without one the integrity floor errors on a property of the RIG"), HeightFog);

	// ── ⛔ (1) THE BEFORE-PICTURE, TAKEN BEFORE THE FIX EXISTS (`SC-§107`) ──────────────────────
	//  ⛔ A negative control on the finder itself: if `Find` answered non-null here, every
	//  non-null below would be meaningless.
	TestNull(TEXT("⛔ (1) NEGATIVE CONTROL: a fresh world holds NO fog-state actor, so the non-null below is evidence rather than noise"),
		AFogVolume::Find(World));

	// ── ⭐⭐⭐ (2) THE WRITE DOOR — ⛔ THE CARD'S OWN, ⛔ NOT A RE-IMPLEMENTATION ──────────────────
	//  `AFogVolume::FindOrSpawn(World)` is character-for-character what `USpellLibrary::ResolveSpell`'s
	//  `FogCover` arm calls. ⛔ Nothing here constructs a volume by hand.
	AFogVolume* const Volume = AFogVolume::FindOrSpawn(World);
	TestNotNull(TEXT("⭐⭐ (2) AFogVolume::FindOrSpawn EXECUTED and produced the one fog-state actor (FOG-§10.1) — this is the card's own write door"), Volume);
	if (!Volume)
	{
		return false;
	}

	TestFalse(TEXT("⛔ (2) …and a freshly spawned volume starts CLEAR — 0.0 is an unambiguous 'no fog'"), Volume->IsFogActive());
	TestFalse(TEXT("⛔ (2) …and unshielded, so RaiseFog below cannot be refused by J-F19 for a reason the rig invented"), Volume->IsFogPrevented());
	TestNull(TEXT("⛔ (2) …and it owns NO actor yet, so the visual found in (4) can only have come from the raise"),
		FindOwnedVisual(World, Volume));

	// ── ⭐⭐⭐ (3) THE REAL ENTRY POINT — ⛔ THIS LINE IS THE WHOLE ROW ────────────────────────────
	//  ⛔ Everything `SC-§113` describes was true right up to this call: `RaiseFog` had never been
	//  executed by anything in this pipeline. From here on, `RefreshFogVisual`, `EnforceFogRenderFloor`
	//  and `SpawnFogVisual` all run — and every `Error` any of them raises reddens this test.
	const bool bRaised = Volume->RaiseFog();
	TestTrue(TEXT("⭐⭐⭐ (3) AFogVolume::RaiseFog() EXECUTED and returned TRUE — the fog path has now genuinely run (SC-§113 cl. 3(c))"), bRaised);

	// ── ⭐⭐ (4) STATE, ⛔ NOT TALLIES (`SC-§104`) ────────────────────────────────────────────────
	TestTrue(TEXT("⭐⭐ (4) The MACHINE says fog is UP — asked of IsFogActive(), the one predicate, rather than counted"), Volume->IsFogActive());

	AActor* const Visual = FindOwnedVisual(World, Volume);
	TestNotNull(TEXT("⭐⭐⭐ (4) THE VISUAL ACTUALLY SPAWNED — an actor OWNED by the fog volume now exists in the world. ⛔ This is the end-to-end claim the file header says the suite does not make; it makes it now"), Visual);

	if (Visual)
	{
		// ⭐ …and it is the class the shipped path names. `ResolveClass()` rather than
		// `TryLoadClass()` on purpose: after a successful raise the class is ALREADY loaded, so a
		// null here would mean `SpawnFogVisual` produced an owned actor WITHOUT loading the
		// visual class — a contradiction worth catching, and asking this way adds no second load.
		UClass* const VisualClass = AFogVolume::FogVisualClassPath().ResolveClass();
		TestNotNull(TEXT("⭐ (4) The fog visual class is RESOLVED in memory after the raise — SpawnFogVisual really loaded it"), VisualClass);
		if (VisualClass)
		{
			TestTrue(TEXT("⭐⭐ (4) …and the actor the volume owns IS that class — the owned actor is the fog visual, not something incidental"),
				Visual->IsA(VisualClass));
		}

		// ⛔⛔ THE `TASK-1071` SCALE, ⛔ REPORTED AND ⛔ NOT RE-ASSERTED, AND THE REASON IS
		// DELIBERATE: `SpawnFogVisual` ⛔ ALREADY tests exactly this and raises an `Error` when it
		// disagrees, which this test's own capture turns into a FAILURE. ⇒ writing a second
		// predicate here would be a ⛔ PARAPHRASE of the shipped one that could drift away from it,
		// and if it ever disagreed nobody could say which was right. ⛔ The numbers are surfaced so
		// a human reading the run has them; ⛔ the judgement stays where the code makes it.
		// ⛔⛔ ⛔ MEASURED VALUES ⛔ ONLY. The REQUEST is deliberately ⛔ NOT recomputed here: this
		// test has no access to the volume's `ArenaGroundReferenceZUU` (it is `protected`), so any
		// "request" it printed would be a ⛔ RECONSTRUCTION that could silently disagree with the
		// real one — an ⛔ ECHO of a guess, which is the eleventh-time-lucky lie this file exists
		// to refuse. ⛔ The spawn log already prints ACHIEVED beside REQUESTED from inside the
		// function that owns both.
		const FVector AchievedScale3D = Visual->GetActorScale3D();
		AddInfo(FString::Printf(
			TEXT("⭐ (4) MEASURED off the spawned actor — ACHIEVED scale (%.3f, %.3f, %.3f), ACHIEVED Z %.1f. ")
			TEXT("⛔ Read back with GetActorScale3D/GetActorLocation, never echoed from a request. ")
			TEXT("⛔ The VERDICT on scale is SpawnFogVisual's own Error site, ⛔ not a predicate in this test — ")
			TEXT("if the TASK-1071 substitution ever returns, this test reds through THAT instrument."),
			AchievedScale3D.X, AchievedScale3D.Y, AchievedScale3D.Z,
			Visual->GetActorLocation().Z));
	}

	// ── ⭐⭐⭐ (5) THE OTHER HALF OF THE SEAM — ⛔ ENTERED ⛔ AND LEFT ─────────────────────────────
	//  ⛔ *"A seam that can be entered and not left is HALF A SEAM."* A build that spawns and never
	//  destroys is ⛔ PERMANENT FOG with a green suite, so the despawn carries the same weight here
	//  as the spawn — the file's own standing rule, now executed instead of counted.
	Volume->ResetFog();

	TestFalse(TEXT("⭐⭐ (5) AFogVolume::ResetFog() EXECUTED — the machine now says fog is DOWN"), Volume->IsFogActive());
	TestFalse(TEXT("⭐ (5) …and prevention is down too: ResetFog zeroes BOTH deadlines (FOG-§10.3)"), Volume->IsFogPrevented());
	TestNull(TEXT("⭐⭐⭐ (5) THE VISUAL IS GONE — the volume owns no actor again. ⛔ A fog corpse surviving here is match-1 fog blinding a match-2 player"),
		FindOwnedVisual(World, Volume));

	// ── ⭐⭐ (6) THE FLOOR WAS ⛔ RELEASED, ⛔ NOT MERELY ENGAGED ──────────────────────────────────
	//  ⛔ This one protects the ⛔ REST OF THE SUITE, not the fog: `EnforceFogRenderFloor` writes
	//  `r.VolumetricFog` at `ECVF_SetByCode`, which is ⛔ PROCESS-GLOBAL. A release that failed
	//  would leave every later test in this run under a console variable this test pinned — and
	//  ⛔ nothing would go red anywhere. ⇒ read the priority BACK off the machine.
	//  ⛔ The cvar NAME is asked of the shipped constant, never retyped: a second copy of that
	//  string is a second thing that can drift.
	if (IConsoleVariable* const VolumetricFogCVar =
			IConsoleManager::Get().FindConsoleVariable(AFogVolume::FogRenderFloorCVarVolumetricFog))
	{
		// ⛔ The priority read is the house spelling, copied from `ReleaseFogRenderFloor` character
		// for character — two spellings of one predicate is two things that can drift apart.
		const bool bStillPinnedByCode =
			(static_cast<EConsoleVariableFlags>(VolumetricFogCVar->GetFlags() & ECVF_SetByMask) == ECVF_SetByCode);
		TestFalse(TEXT("⭐⭐ (6) The integrity floor was RELEASED — the volumetric-fog console variable is no longer held at SetByCode. ⛔ A true here means this test stranded a process-global cvar across the whole suite run"),
			bStillPinnedByCode);
	}
	else
	{
		AddInfo(TEXT("ℹ️ (6) The volumetric-fog console variable does not exist in this build, so the release could not be read back. ⛔ NOT MEASURED — never a pass."));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
