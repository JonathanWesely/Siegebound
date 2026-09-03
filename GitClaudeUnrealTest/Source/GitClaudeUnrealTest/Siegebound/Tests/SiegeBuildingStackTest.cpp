// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Containers/UnrealString.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/Building.h"
#include "Siegebound/ClimbableTower.h"
#include "Siegebound/Tower.h"
#include "UObject/Class.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for TOWER STACKING — the two series + the scale-exclusion
 *      predicate (TASK-812; STACK-§1 / STACK-§2 / STACK-§5 J-10 / STACK-§7) ═══
 *
 *  Jonathan, verbatim (2026-09-02): "instead of placing a new tower, it will instead double the
 *  height of the old one … twice as tall while keeping the same width and length and will gain
 *  1.5 times the health … 3 times taller than the original height and 2.25 times the original
 *  health … 4 times taller, and 3.375 times the health and so on … The maximum height it can
 *  reach is 5 times taller, and there is no maximum on the health."
 *
 *  Subject: `ABuilding::StackHeightMultiplier` / `ABuilding::StackHealthMultiplier` (the two
 *  `STACK-§7` pinned pure statics), `ABuilding::CanScaleFootprint` (the `STACK-§2` structural
 *  exclusion) and `ABuilding::ApplyStackUpgrade` (the one authoritative mutator).
 *
 *  ⛔ NOT covered here and boarded elsewhere: the blue ghost, the cost, the refusal message and
 *  the `EPlacementInvalidReason` value are TASK-813's; the placement wheel is TASK-815's.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────────
 *  ⭐⭐⭐ THE TRAP THIS WHOLE FILE IS POINTED AT — **N = 1 CANNOT TELL THE TWO READINGS APART**
 *  ───────────────────────────────────────────────────────────────────────────────────────────
 *
 *  His two sentences contradict each other. The ENUMERATION he wrote out gives ×2, ×3, ×4, ×5
 *  (ADDITIVE — `+1 ×` the ORIGINAL per upgrade); his SUMMARY sentence — "it basically gains
 *  **twice the current height**" — gives ×2, ×4, ×8, ×16 (DOUBLING). `STACK-§1` ruled for the
 *  enumeration, and ⭐ the argument is his own cap: under doubling "5 times taller" is
 *  ⛔ **NEVER REACHED**, so the ceiling he wrote would describe a state the game can never
 *  enter; under the enumeration ×5 lands ⛔ EXACTLY, on the 4th upgrade.
 *
 *  ⚠️⚠️ AND THE TWO SERIES **AGREE AT n = 0 AND n = 1** — both say ×1 and ×2. ⇒ ⛔ **EVERY
 *  ASSERTION AT n ≤ 1 IS VACUOUS FOR THIS PURPOSE AND WOULD REPORT SAFE AGAINST THE WRONG
 *  SERIES.** That agreement is not left implicit: test 2 ASSERTS it, so the file states its own
 *  blind spot rather than hiding inside it, and every discriminating row below is ⛔ n ≥ 2.
 *
 *  ⭐ The HEALTH series has the mirror-image property: it is `1.5ⁿ` under BOTH readings of his
 *  sentence (he wrote 2.25 = 1.5² and 3.375 = 1.5³ himself), so the health rows discriminate
 *  against a different wrong answer — an ADDITIVE health series — and they too only bite at
 *  n ≥ 2 (multiplicative 2.25 vs additive 2.00).
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────────
 *  ⛔⛔ AND NO EXPECTATION IN THIS FILE IS TRANSCRIBED FROM ITS SUBJECT (`SC-§37`, `SHIP-§9c`)
 *  ───────────────────────────────────────────────────────────────────────────────────────────
 *
 *  `TestEqual(StackHeightMultiplier(3), 4.f)` copied out of the spec table agrees with a wrong
 *  header by construction and reports SAFE for ever. So every number below is either
 *  ⭐ RE-DERIVED from the `EditDefaultsOnly` tunables (a retune moves code and expectation
 *  together) or ⭐ INDEPENDENTLY CONSTRUCTED in the fixture as the REJECTED reading (`2ⁿ` for
 *  height, `1 + n(step − 1)` for health) so the shipped series can be asserted to DISAGREE
 *  with it. What is asserted is the SHAPE: constant increments vs constant ratios, saturation
 *  vs unboundedness, and a cap that is LANDED ON rather than jumped over.
 *
 *  ───────────────────────────────────────────────────────────────────────────────────────────
 *  MECHANISM — ⛔ zero world, ⛔ zero PIE, ⛔ zero asset loads, ⛔ zero writes to the project.
 *  Pure statics, class-default objects, the reflection tables, four transient world-free actor
 *  instances (the `SiegeGhostPawnTest::MakeScratchGhost` precedent) and two reads of the
 *  shipped source (the `SiegeClimbableTowerTest` tests 14/15 precedent — a `virtual` override
 *  that returns a literal leaves nothing in the reflection tables, so "the predicate names no
 *  card" is unaskable any other way).
 *
 *  ⛔⛔ WHAT A GREEN BAR HERE IS ⛔ NOT (`SC-§32` — a mechanism never observed is not known to
 *  work):
 *    • ⛔ **`BeginPlay` NEVER RUNS.** An actor outside a world never begins play, so the
 *      instances below are STATLESS (`MaxHP` 0) until this file seeds `MaxHP`/`CurrentHP`
 *      through the reflection tables. ⇒ the DT_Cards bind itself is not exercised here.
 *    • ⛔ **NOTHING IS RENDERED.** The Z scale is asserted on the component transform, ⛔ not on
 *      pixels — the ×5 UV stretch is real, shipped as-is and named (`STACK-§3`, row `J-11`).
 *    • ⛔ **THE LADDER IS NOT RE-MEASURED.** This file asserts that `AClimbableTower` REFUSES to
 *      scale; it says nothing about the climb, which `TOWER-§8.*` owns and `STACK-§2` protects
 *      precisely by never letting the geometry change.
 *    • ⛔ **NO REPLICATION IS EXERCISED.** `HasAuthority()` is true on a world-free actor by
 *      construction, so test 7's authority guard is asserted as PRESENT (source + behaviour on
 *      the authoritative path), ⛔ not proven to reject a real client.
 *    • ⛔ **NO MODEL RUNS.** No `Capture()`, no `EnsureSnapshot()`. 🔒 The one-shot latch is
 *      untouched and unspent, Zone A is not read, and ⛔ no token figure appears in this file.
 */

namespace SiegeBuildingStackTestFixture
{
	/** Exact-equality tolerance, for the claims whose entire content is the word EXACTLY. */
	constexpr float Exact = 0.f;

	/** Loose enough that float32 rounding never fires it, far tighter than any wrong series' gap (the smallest of which is a whole 1.0). */
	constexpr float Tolerance = 1.e-4f;

	/** The height series, spelled once so no test re-types its argument list. */
	static float Height(int32 UpgradeCount)
	{
		return ABuilding::StackHeightMultiplier(UpgradeCount);
	}

	/** The health series, likewise. */
	static float Health(int32 UpgradeCount)
	{
		return ABuilding::StackHealthMultiplier(UpgradeCount);
	}

	/**
	 *  ⭐⭐ THE **REJECTED** HEIGHT READING, CONSTRUCTED INDEPENDENTLY IN THE TEST — `2ⁿ`, i.e.
	 *  Jonathan's summary sentence "twice the CURRENT height". ⛔ Nothing in the shipped code
	 *  produces this; it exists so the shipped series can be asserted to DISAGREE with it,
	 *  which is the only way an assertion about WHICH series shipped can fail.
	 */
	static float DoublingReadingHeight(int32 UpgradeCount)
	{
		float Multiplier = 1.f;
		for (int32 Index = 0; Index < FMath::Max(0, UpgradeCount); ++Index)
		{
			Multiplier *= 2.f;
		}
		return Multiplier;
	}

	/**
	 *  ⭐ THE **REJECTED** HEALTH READING — additive, `1 + n × (Step − 1)`. His health half was
	 *  right in both of his sentences, so the wrong answer health could plausibly ship with is
	 *  the same mistake in the opposite direction: making it match the height series in KIND.
	 *  ⚠️ It agrees with the shipped series at n = 0 and n = 1, exactly like the height pair.
	 */
	static float AdditiveReadingHealth(int32 UpgradeCount, float Step)
	{
		return 1.f + static_cast<float>(FMath::Max(0, UpgradeCount)) * (Step - 1.f);
	}

	/** ABuilding's CDO, or null with the error already reported. Both tunables live here (see MaxStackHeightMultiplier's declaration for why the statics read them from it). */
	static const ABuilding* BuildingDefaults(FAutomationTestBase& Test)
	{
		const ABuilding* const Defaults = GetDefault<ABuilding>();
		if (!Defaults)
		{
			Test.AddError(TEXT("SELF-CHECK FAILED: GetDefault<ABuilding>() returned null — every expectation below is derived from this object, so nothing after this line would mean anything."));
		}
		return Defaults;
	}

	/** Reads an int32 UPROPERTY off any object by name (the tunables and StackUpgradeCount are private/protected — the reflection tables are the sanctioned reach, and a missing name FAILS rather than defaulting). */
	static bool ReadInt(FAutomationTestBase& Test, const UObject* Object, const TCHAR* PropertyName, int32& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FIntProperty* const IntProperty = CastField<FIntProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)));
		if (!IntProperty)
		{
			Test.AddError(FString::Printf(TEXT("SELF-CHECK FAILED: '%s' is not a reflected int32 property on '%s' — the probe is stale, and a stale probe FAILS rather than passing vacuously."), PropertyName, *Object->GetClass()->GetName()));
			return false;
		}

		OutValue = IntProperty->GetPropertyValue_InContainer(Object);
		return true;
	}

	/** Reads a float UPROPERTY off any object by name. Same contract as ReadInt. */
	static bool ReadFloat(FAutomationTestBase& Test, const UObject* Object, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			Test.AddError(FString::Printf(TEXT("SELF-CHECK FAILED: '%s' is not a reflected float property on '%s' — the probe is stale."), PropertyName, *Object->GetClass()->GetName()));
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Object);
		return true;
	}

	/** Seeds a private float UPROPERTY (MaxHP / CurrentHP). ⚠️ Only ever on a TRANSIENT instance — never the CDO, which would leak into every other test in the suite. */
	static bool SeedFloat(FAutomationTestBase& Test, UObject* Object, const TCHAR* PropertyName, float Value)
	{
		if (!Object)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			Test.AddError(FString::Printf(TEXT("SELF-CHECK FAILED: '%s' is not a reflected float property on '%s' — the seed could not be planted, so the HP claims below would be measuring nothing."), PropertyName, *Object->GetClass()->GetName()));
			return false;
		}

		FloatProperty->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	/**
	 *  A transient, world-free building instance (the `SiegeGhostPawnTest::MakeScratchGhost`
	 *  precedent). ⛔ Never the CDO — `ApplyStackUpgrade` MUTATES, and mutating a CDO would
	 *  leak into every other test in this file and in the suite.
	 *  ⚠️ `BeginPlay` never runs on it, so it is statless until a test seeds MaxHP/CurrentHP.
	 */
	template <typename TBuilding>
	static TStrongObjectPtr<TBuilding> MakeScratchBuilding()
	{
		return TStrongObjectPtr<TBuilding>(
			NewObject<TBuilding>(GetTransientPackageAsObject(), TBuilding::StaticClass(), NAME_None, RF_Transient));
	}

	/**
	 *  FVector is DOUBLE-precision in UE5 while both series return float, so every transform
	 *  read is narrowed HERE, once and explicitly — ⛔ never left to an implicit conversion
	 *  inside a TestEqual overload resolution, which is how a comparison quietly picks the
	 *  wrong tolerance type.
	 */
	static float ScaleComponentAsFloat(double Component)
	{
		return static_cast<float>(Component);
	}

	/** VisualMesh is the ROOT (Building.h:43-51), so the public root accessor reaches the very component ApplyStackUpgrade scales — no protected member is touched. */
	static USceneComponent* ScaleRootOf(FAutomationTestBase& Test, AActor* Actor)
	{
		USceneComponent* const Root = Actor ? Actor->GetRootComponent() : nullptr;
		if (!Root)
		{
			Test.AddError(TEXT("SELF-CHECK FAILED: the scratch building has no root component — ABuilding's constructor makes VisualMesh the root, so this means the instance did not construct."));
			return nullptr;
		}

		// ⭐ SELF-CHECK ON THE INSTRUMENT ITSELF: the thing being measured must be the STATIC
		// MESH the upgrade scales, not some other scene component that happened to become root.
		if (!Cast<UStaticMeshComponent>(Root))
		{
			Test.AddError(TEXT("SELF-CHECK FAILED: the root is not a UStaticMeshComponent — the scale claims below would be measuring the wrong component."));
			return nullptr;
		}

		return Root;
	}

	/** Reads a shipped project source file. A probe that cannot read its subject FAILS (the SiegeClimbableTowerTest helper, same shape and same reason). */
	static bool LoadProjectSource(FAutomationTestBase& Test, const TCHAR* RelativePath, FString& OutText)
	{
		const FString FullPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / FString(RelativePath));
		if (!FPaths::FileExists(FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not find '%s'. This probe reads the SHIPPED source because a virtual override returning a literal leaves NOTHING in the reflection tables; a probe that cannot read its subject FAILS."), *FullPath));
			return false;
		}

		if (!FFileHelper::LoadFileToString(OutText, *FullPath))
		{
			Test.AddError(FString::Printf(TEXT("⛔ Could not read '%s' — a stale probe fails."), *FullPath));
			return false;
		}

		return true;
	}

	/**
	 *  Non-overlapping occurrences of Needle on CODE lines only — comment lines are skipped, so
	 *  the paragraphs that EXPLAIN why the WatchTower is excluded do not read as the name check
	 *  they forbid. (The `SiegeClimbableTowerTest::CountOccurrencesInCode` helper, same shape.)
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
			// `* text` or `*/`, while `*GetNameSafe(Foo)` — a dereferenced FString in a UE_LOG
			// argument list — starts a CODE line with the same character.
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

	static const TCHAR* const BuildingHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/Building.h");
	static const TCHAR* const BuildingSourcePath = TEXT("Source/GitClaudeUnrealTest/Siegebound/Building.cpp");
	static const TCHAR* const ClimbableTowerHeaderPath = TEXT("Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h");
	static const TCHAR* const ClimbableTowerSourcePath = TEXT("Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp");
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  1. ⭐⭐ THE TWO SERIES ARE DELIBERATELY DIFFERENT **IN KIND** — height INCREMENTS are
//     constant (additive), health RATIOS are constant (multiplicative), and ⛔ neither is the
//     other. `STACK-§1`: "the two series are deliberately different in kind — one saturates,
//     one does not."
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackSeriesKindTest,
	"Siegebound.BuildingStack.HeightIncrementsAreConstantWhileHealthRatiosAreConstant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackSeriesKindTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	int32 Cap = 0;
	float Step = 0.f;
	if (!ReadInt(*this, Defaults, TEXT("MaxStackHeightMultiplier"), Cap)
		|| !ReadFloat(*this, Defaults, TEXT("StackHealthStep"), Step))
	{
		return false;
	}

	// ── SELF-CHECKS: THE PREMISES THAT MAKE EVERY ROW BELOW ABLE TO FAIL ────────────────────
	// ⚠️⚠️ A "the increments are all equal" assertion passes brilliantly and permanently
	// against a CONSTANT function. Proving both series actually move first is what stops this
	// test from being a monument to nothing.
	TestTrue(TEXT("SELF-CHECK: the height series is not a constant function — it MOVES between n=0 and n=2, so 'the increments are equal' is a claim about a real series"),
		Height(2) > Height(0));
	TestTrue(TEXT("SELF-CHECK: the health series is not a constant function either"),
		Health(2) > Health(0));
	TestTrue(TEXT("SELF-CHECK: the shipped cap leaves at least three unsaturated terms, so the 'constant increment' walk below has something to walk"),
		Cap >= 3);
	TestTrue(TEXT("SELF-CHECK: the shipped health step is greater than 1, without which ratios and increments would both be trivially constant"),
		Step > 1.f);

	// ── (a) HEIGHT: THE INCREMENTS ARE CONSTANT BELOW THE CAP ⇒ ADDITIVE ────────────────────
	// The increment is READ from the series (Height(1) − Height(0)), ⛔ never typed as 1.0 —
	// so this asserts self-similarity rather than transcribing a spec table.
	const float HeightIncrement = Height(1) - Height(0);
	for (int32 Index = 1; Index + 1 <= Cap - 1; ++Index)
	{
		TestEqual(*FString::Printf(TEXT("(a) ⭐ HEIGHT is ADDITIVE: the step from n=%d to n=%d is the SAME as the first step. ⛔ Under the doubling reading this increment DOUBLES every term and this row goes red"), Index, Index + 1),
			Height(Index + 1) - Height(Index), HeightIncrement, Tolerance);
	}

	// ── (b) HEIGHT IS ⛔ NOT MULTIPLICATIVE — its RATIOS are NOT constant ────────────────────
	// ⭐ n ≥ 2 ONLY. Height(1)/Height(0) = 2 under BOTH readings, so a ratio comparison that
	// started at n=0 would be comparing the one place they agree.
	TestTrue(TEXT("(b) ⛔ HEIGHT is NOT multiplicative: the ratio from n=1→2 DIFFERS from the ratio n=0→1 (3/2 vs 2/1). ⛔ This row is the one that dies if somebody 'corrects' height toward the summary sentence"),
		!FMath::IsNearlyEqual(Height(2) / Height(1), Height(1) / Height(0), Tolerance));

	// ── (c) HEALTH: THE RATIOS ARE CONSTANT AND EQUAL THE TUNABLE ⇒ MULTIPLICATIVE ──────────
	// Deliberately walked well past the height cap: the health series has no ceiling term, so
	// there is no n at which its shape is allowed to change.
	for (int32 Index = 0; Index < 12; ++Index)
	{
		TestEqual(*FString::Printf(TEXT("(c) ⭐ HEALTH is MULTIPLICATIVE: the ratio from n=%d to n=%d is exactly the shipped StackHealthStep — and it is STILL that far past the height cap, because health has no cap"), Index, Index + 1),
			Health(Index + 1) / Health(Index), Step, Tolerance);
	}

	// ── (d) HEALTH IS ⛔ NOT ADDITIVE — and this row also only bites at n ≥ 2 ────────────────
	// The additive reading gives 1 + n(Step − 1): identical at n = 0 and n = 1, and 2.00 vs
	// 2.25 at n = 2. ⭐ The rejected series is CONSTRUCTED in the fixture, so this asserts a
	// disagreement between two independently written functions rather than restating a value.
	TestTrue(TEXT("SELF-CHECK: the additive-health reading AGREES with the shipped one at n=1 — which is exactly why the row below starts at n=2"),
		FMath::IsNearlyEqual(AdditiveReadingHealth(1, Step), Health(1), Tolerance));
	for (int32 Index = 2; Index <= 5; ++Index)
	{
		TestTrue(*FString::Printf(TEXT("(d) ⛔ HEALTH is NOT additive at n=%d: the shipped term DIFFERS from 1 + n(step−1). ⭐ 2.25 = 1.5² is Jonathan's own number and an additive series cannot produce it"), Index),
			!FMath::IsNearlyEqual(AdditiveReadingHealth(Index, Step), Health(Index), Tolerance));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  2. ⭐⭐⭐ THE DECISIVE ONE — THE CAP IS **REACHED EXACTLY**, AND THE DOUBLING READING CAN
//     ⛔ NEVER REACH IT. This is the property that falsifies the summary sentence, and it is
//     the reason `STACK-§1`'s ruling is not a coin flip.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackCapReachedExactlyTest,
	"Siegebound.BuildingStack.TheHeightCapIsReachedExactlyAndTheDoublingReadingCanNeverReachIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackCapReachedExactlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	int32 Cap = 0;
	if (!ReadInt(*this, Defaults, TEXT("MaxStackHeightMultiplier"), Cap))
	{
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: the shipped cap is at least 2, without which 'reached exactly at the (Cap−1)ᵗʰ upgrade' would be a statement about n = 0"),
		Cap >= 2);

	const float CapAsFloat = static_cast<float>(Cap);

	// ── ⚠️⚠️ THE BLIND SPOT, ASSERTED RATHER THAN HIDDEN ─────────────────────────────────────
	// ⭐ The two readings AGREE at n = 0 and n = 1 (×1 and ×2). Stating that as a passing
	// assertion is what makes "every discriminating row below is n ≥ 2" an auditable fact
	// instead of a claim in a comment. If this pair ever stops agreeing, one of the two
	// functions has been changed and the discrimination rows need re-reading.
	TestEqual(TEXT("SELF-CHECK ⭐: at n=0 the enumeration and the doubling readings AGREE (both ×1) — n=0 discriminates NOTHING"),
		Height(0), DoublingReadingHeight(0), Exact);
	TestEqual(TEXT("SELF-CHECK ⭐: at n=1 they AGREE TOO (both ×2) — ⛔ this is why no assertion in this file relies on n=1 to choose between them"),
		Height(1), DoublingReadingHeight(1), Exact);

	// ── (a) ⭐ THE DISCRIMINATING ROWS — n ≥ 2, BELOW THE CAP ────────────────────────────────
	// Both series are evaluated; the claim is that they DISAGREE. ⛔ No spec value is typed.
	for (int32 Index = 2; Index <= Cap - 1; ++Index)
	{
		TestTrue(*FString::Printf(TEXT("(a) ⭐⭐ AT n=%d THE SHIPPED SERIES DIFFERS FROM THE DOUBLING READING (%.1f vs %.1f). ⛔ This is the assertion that fails the moment somebody 'fixes' the code toward Jonathan's summary sentence"), Index, Height(Index), DoublingReadingHeight(Index)),
			!FMath::IsNearlyEqual(Height(Index), DoublingReadingHeight(Index), Tolerance));
	}

	// ── (b) ⭐⭐⭐ THE CAP IS **LANDED ON**, NOT JUMPED OVER ───────────────────────────────────
	// Three claims that only an additive integer series can satisfy together:
	//   • the term one BELOW the cap index is STRICTLY below the cap,
	//   • the term AT the cap index equals the cap EXACTLY (tolerance 0 — no near-miss),
	//   • and it stays there for ever.
	const int32 CapIndex = Cap - 1;

	TestTrue(*FString::Printf(TEXT("(b) the term before the cap (n=%d) is STRICTLY below it — so the cap is approached, ⛔ not started at"), CapIndex - 1),
		Height(CapIndex - 1) < CapAsFloat);

	TestEqual(*FString::Printf(TEXT("(b) ⭐⭐⭐ THE CAP IS REACHED **EXACTLY** AT n=%d — tolerance ZERO. ⚖️ This is the whole argument for the enumeration: it is the property Jonathan's own '5 times taller' sentence needs in order to mean anything"), CapIndex),
		Height(CapIndex), CapAsFloat, Exact);

	for (int32 Beyond = 1; Beyond <= 6; ++Beyond)
	{
		TestEqual(*FString::Printf(TEXT("(b) …and it SATURATES: n=%d is still exactly the cap, never above it"), CapIndex + Beyond),
			Height(CapIndex + Beyond), CapAsFloat, Exact);
	}

	// ── (c) ⭐⭐⭐ THE FALSIFIER — THE DOUBLING SERIES **STEPS OVER** THE CAP ──────────────────
	// Independently constructed `2ⁿ`, walked far past the ceiling: no term equals the cap, and
	// there is an n where it jumps from below it to above it. ⇒ under that reading Jonathan's
	// stated maximum describes a state the game can ⛔ NEVER enter.
	// ⚠️ Guarded on the cap not being a power of two: if he ever retunes the cap TO one (4, 8,
	// 16 …) this argument genuinely stops applying, and the guard says so out loud instead of
	// going quietly red on a legitimate retune.
	bool bDoublingEverHitsTheCap = false;
	bool bDoublingStepsOverTheCap = false;
	for (int32 Index = 0; Index < 24; ++Index)
	{
		if (FMath::IsNearlyEqual(DoublingReadingHeight(Index), CapAsFloat, Tolerance))
		{
			bDoublingEverHitsTheCap = true;
		}
		if (DoublingReadingHeight(Index) < CapAsFloat && DoublingReadingHeight(Index + 1) > CapAsFloat)
		{
			bDoublingStepsOverTheCap = true;
		}
	}

	TestTrue(TEXT("SELF-CHECK: the independently-constructed doubling series is live (2² = 4)"),
		FMath::IsNearlyEqual(DoublingReadingHeight(2), 4.f, Tolerance));

	if (!bDoublingEverHitsTheCap)
	{
		TestTrue(*FString::Printf(TEXT("(c) ⭐⭐⭐ THE DOUBLING READING **STEPS OVER** THE CAP OF ×%d — it goes below it to above it with no term equal to it, so Jonathan's own stated maximum would be UNREACHABLE. ⭐ The shipped series lands on it exactly. That asymmetry is why STACK-§1 is a ruling and not a preference"), Cap),
			bDoublingStepsOverTheCap);
	}
	else
	{
		AddWarning(*FString::Printf(TEXT("⚠️ MaxStackHeightMultiplier has been retuned to ×%d, which IS a power of two — the 'his cap would be unreachable under doubling' argument in STACK-§1 no longer applies at this value. The shipped series' behaviour above is unaffected and still asserted; this is a note for whoever retuned it."), Cap));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  3. ⭐ ONE SATURATES AND ONE DOES NOT — his closing sentence, as an assertion: "at some point
//     if they keep upgrading it would only upgrade health by 1.5 times and not height."
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackSaturationTest,
	"Siegebound.BuildingStack.HeightSaturatesAtTheCapWhileHealthGrowsWithoutBound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackSaturationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	int32 Cap = 0;
	if (!ReadInt(*this, Defaults, TEXT("MaxStackHeightMultiplier"), Cap))
	{
		return false;
	}

	const int32 CapIndex = Cap - 1;

	TestTrue(TEXT("SELF-CHECK: the health series is still growing at the cap index, so 'it keeps growing afterwards' is a claim about a live series"),
		Health(CapIndex) > Health(CapIndex - 1));

	// ── ⭐ PAST THE CAP: HEIGHT IS FROZEN, HEALTH IS STRICTLY INCREASING, EVERY SINGLE TERM ──
	// ⚠️ Both halves are asserted in the SAME loop on purpose. Asserting them apart would let
	// a "cap them both" regression satisfy the height half while the health half was checked
	// somewhere else against a different n.
	for (int32 Index = CapIndex; Index < CapIndex + 20; ++Index)
	{
		TestEqual(*FString::Printf(TEXT("(a) HEIGHT is frozen at n=%d — the ladder stops paying for height"), Index + 1),
			Height(Index + 1), Height(Index), Exact);

		TestTrue(*FString::Printf(TEXT("(b) ⭐ HEALTH is STILL growing at n=%d — ⛔ UNCAPPED, his explicit word, and it is what keeps the click at the cap worth making (STACK-§5 J-6)"), Index + 1),
			Health(Index + 1) > Health(Index));
	}

	// ── (c) UNBOUNDED, not merely "bigger" — health leaves the height ceiling far behind ─────
	TestTrue(TEXT("(c) ⭐ far up the ladder the health multiplier dwarfs the height ceiling — the two series are not on the same scale and were never meant to be"),
		Health(CapIndex + 20) > static_cast<float>(Cap) * 100.f);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  4. THE DOMAIN — identity at zero upgrades, and a defensive clamp below it.
//     ⚠️ NOTE HONESTLY: this test is the one that CANNOT discriminate the two readings, which
//     is exactly why it is small and separate. n = 0 is the trivial row (`SC-§37`).
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackDomainTest,
	"Siegebound.BuildingStack.BothSeriesAreExactlyIdentityAtZeroUpgradesAndClampNegativeInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackDomainTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	// An un-upgraded building must be BIT-FOR-BIT what the artist authored: the placement path
	// applies these multipliers unconditionally, so a 1.0001 here would silently resize every
	// building in the game at spawn.
	TestEqual(TEXT("(a) zero upgrades ⇒ the height multiplier is EXACTLY 1.0 — an un-upgraded building is untouched"),
		Height(0), 1.f, Exact);
	TestEqual(TEXT("(a) zero upgrades ⇒ the health multiplier is EXACTLY 1.0"),
		Health(0), 1.f, Exact);

	// The seams are public and pure; they answer for the whole int32 domain rather than
	// trusting callers. ⛔ Negative is not a state StackUpgradeCount can reach.
	TestEqual(TEXT("(b) a negative upgrade count clamps to identity on the height series — ⛔ never a shrunken building, ⛔ never a negative scale"),
		Height(-1), 1.f, Exact);
	TestEqual(TEXT("(b) …and on the health series — ⛔ never a fractional-HP building"),
		Health(-7), 1.f, Exact);

	// ⚠️ AND THE GUARD IS NOT A CEILING: a large count must still be finite and still be
	// growing, or the runaway break inside the health loop would have become a silent cap.
	TestTrue(TEXT("(c) a large upgrade count still returns a FINITE, still-growing health multiplier — the runaway break is a guard, ⛔ not a cap"),
		FMath::IsFinite(Health(60)) && Health(60) > Health(59));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  5. ⭐ THE SEAMS ARE WHAT `STACK-§7` PINNED — plain statics (⛔ not UFUNCTIONs) reading
//     EditDefaultsOnly tunables, so a retune moves the BEHAVIOUR and ⛔ no second literal of
//     either number exists in the series (`HIGH-§1`).
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackTunableWiringTest,
	"Siegebound.BuildingStack.TheSeriesReadTheirEditDefaultsOnlyTunablesAndAreNotUFunctions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackTunableWiringTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	UClass* const BuildingClass = ABuilding::StaticClass();
	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!BuildingClass || !Defaults)
	{
		return false;
	}

	// ── (a) THE CAP IS AN **INTEGER** PROPERTY (`STACK-§7`) ─────────────────────────────────
	// ⚖️ Not pedantry: "the cap is reached EXACTLY" is a promise only an integer ceiling can
	// keep. A float cap would make the decisive assertion in test 2 a coin toss on rounding.
	const FProperty* const CapProperty = BuildingClass->FindPropertyByName(TEXT("MaxStackHeightMultiplier"));
	TestNotNull(TEXT("SELF-CHECK: MaxStackHeightMultiplier is a reflected property — if this probe is stale the rows below prove nothing"), CapProperty);
	TestTrue(TEXT("(a) ⭐ the height cap is an int32, ⛔ NOT a float — the exactness of the cap depends on it"),
		CastField<FIntProperty>(CapProperty) != nullptr);

	const FProperty* const StepProperty = BuildingClass->FindPropertyByName(TEXT("StackHealthStep"));
	TestNotNull(TEXT("SELF-CHECK: StackHealthStep is a reflected property"), StepProperty);
	TestTrue(TEXT("(a) the health step is a float — it compounds, so it is not an integer quantity"),
		CastField<FFloatProperty>(StepProperty) != nullptr);

	// ── (b) BOTH ARE **EditDefaultsOnly** ⇒ Jonathan can retune them without a programmer ────
	if (CapProperty && StepProperty)
	{
		TestTrue(TEXT("(b) the height cap is EditDefaultsOnly — a designer-facing knob, not a hardcoded rule"),
			CapProperty->HasAnyPropertyFlags(CPF_Edit) && CapProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
		TestTrue(TEXT("(b) the health step is EditDefaultsOnly"),
			StepProperty->HasAnyPropertyFlags(CPF_Edit) && StepProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	// ── (c) ⭐⭐ THE SERIES ACTUALLY **READ** THEM — ⛔ no second literal ─────────────────────
	// If either number were typed into the series, the shipped value and the tunable could
	// drift apart the first time Jonathan retunes. These two rows tie them together.
	int32 Cap = 0;
	float Step = 0.f;
	if (ReadInt(*this, Defaults, TEXT("MaxStackHeightMultiplier"), Cap)
		&& ReadFloat(*this, Defaults, TEXT("StackHealthStep"), Step))
	{
		TestEqual(TEXT("(c) ⭐ the height series SATURATES at the value of the tunable itself — ⛔ so no second copy of '5' lives inside the function (HIGH-§1)"),
			Height(Cap + 50), static_cast<float>(Cap), Exact);
		TestEqual(TEXT("(c) ⭐ one health upgrade IS the tunable — ⛔ so no second copy of '1.5' lives inside the function"),
			Health(1), Step, Exact);
	}

	// ── (d) ⛔ NEITHER SERIES IS A UFUNCTION (`STACK-§7`, `SC-§33`) ──────────────────────────
	// ⭐ SELF-CHECK FIRST: the reflection walk must be able to FIND a function on this class,
	// or "these two are absent" would pass against a blind instrument.
	TestNotNull(TEXT("SELF-CHECK: the function reflection table for ABuilding is live — InitBuilding IS a UFUNCTION and is found"),
		BuildingClass->FindFunctionByName(TEXT("InitBuilding")));

	TestNull(TEXT("(d) ⛔ StackHeightMultiplier is a PLAIN C++ static, ⛔ not a UFUNCTION — it has no world, no actor instance and nothing to marshal"),
		BuildingClass->FindFunctionByName(TEXT("StackHeightMultiplier")));
	TestNull(TEXT("(d) ⛔ StackHealthMultiplier likewise"),
		BuildingClass->FindFunctionByName(TEXT("StackHealthMultiplier")));

	// ── (e) THE ONE SOURCE OF TRUTH EXISTS, AND ⛔ NO CACHED MULTIPLIER SITS BESIDE IT ───────
	const FProperty* const CountProperty = BuildingClass->FindPropertyByName(TEXT("StackUpgradeCount"));
	TestNotNull(TEXT("(e) StackUpgradeCount is the per-instance state both series derive from"), CountProperty);
	TestTrue(TEXT("(e) …and it is an int32 — the series' domain"),
		CastField<FIntProperty>(CountProperty) != nullptr);

	// ⛔ STACK-§7: "⛔ No second copy of height or health scale is stored anywhere." A cached
	// multiplier could not avoid being named after itself.
	static const TCHAR* const ForbiddenCacheNames[] =
	{
		TEXT("CachedHeightMultiplier"), TEXT("CurrentHeightMultiplier"), TEXT("StackHeightScale"),
		TEXT("CachedHealthMultiplier"), TEXT("CurrentHealthMultiplier"), TEXT("StackHealthScale")
	};
	for (const TCHAR* const ForbiddenName : ForbiddenCacheNames)
	{
		TestNull(*FString::Printf(TEXT("(e) ⛔ no '%s' is stored — every multiplier is RECOMPUTED from StackUpgradeCount, which is what lets a retune move live buildings"), ForbiddenName),
			BuildingClass->FindPropertyByName(ForbiddenName));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  6. ⛔⛔ THE EXCLUSION IS **STRUCTURAL** — virtual dispatch, ⛔ never a `CardID` string
//     compare (`STACK-§2`: a name check anywhere on this path is an automatic FAIL).
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackExclusionPredicateTest,
	"Siegebound.BuildingStack.CanScaleFootprintIsFalseOnTheClimbableTowerTrueElsewhereAndNamesNoCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackExclusionPredicateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const BuildingDefaultsObject = GetDefault<ABuilding>();
	const AClimbableTower* const TowerDefaults = GetDefault<AClimbableTower>();
	const ATower* const FiringTowerDefaults = GetDefault<ATower>();
	if (!BuildingDefaultsObject || !TowerDefaults || !FiringTowerDefaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: a GetDefault<>() returned null — the predicate cannot be asked."));
		return false;
	}

	TestTrue(TEXT("SELF-CHECK: AClimbableTower really is an ABuilding, so this IS an override and not two unrelated functions"),
		AClimbableTower::StaticClass()->IsChildOf(ABuilding::StaticClass()));

	// ── (a) THE TWO ANSWERS ─────────────────────────────────────────────────────────────────
	TestTrue(TEXT("(a) a plain ABuilding CAN be scaled — the default is permissive, so a future building inherits stacking for free"),
		BuildingDefaultsObject->CanScaleFootprint());
	TestFalse(TEXT("(a) ⛔⛔ AClimbableTower CANNOT — a scaled SM_WatchTower moves the LadderFoot/LadderTop sockets and the rung plane, fires TOWER-§8.5a's VOIDING condition, and the climb stops working ENTIRELY (STACK-§2)"),
		TowerDefaults->CanScaleFootprint());

	// ── (b) ⭐ THE FAMILY HE ACTUALLY MEANT KEEPS STACKING ───────────────────────────────────
	// He wrote "the tower button"; ArrowTower / BombTower / BallistaTower / CrystalTower are all
	// ATower, all of which he has played with for weeks. The exclusion removes the ⛔ one card
	// he probably was not pointing at — and it is the only one carrying a pinned socket contract.
	TestTrue(TEXT("(b) ⭐ ATower — the ArrowTower/BombTower/BallistaTower/CrystalTower family — CAN still be scaled. The exclusion costs Jonathan nothing he asked for"),
		FiringTowerDefaults->CanScaleFootprint());

	// ── (c) ⭐⭐ IT IS **VIRTUAL DISPATCH**, NOT A SHADOWED NON-VIRTUAL ───────────────────────
	// ⚠️ THIS IS THE ROW THAT MATTERS. A non-virtual `CanScaleFootprint` on the subclass would
	// make every one of the assertions above pass while the PLACEMENT PATH — which holds an
	// `ABuilding*` — silently got `true` and scaled the tower anyway. Asking through the BASE
	// pointer is the only way to observe the difference.
	const ABuilding* const TowerThroughBasePointer = TowerDefaults;
	TestFalse(TEXT("(c) ⭐⭐ asked through an ABuilding* — the way the placement path will ask — the climbable tower STILL refuses. ⛔ A shadowed non-virtual would pass every other row in this test and fail this one"),
		TowerThroughBasePointer->CanScaleFootprint());

	// ── (d) ⛔⛔ AND IT NAMES NO CARD (`STACK-§2`: a string compare here is an AUTOMATIC FAIL) ─
	// A `virtual` returning a literal leaves NOTHING in the reflection tables, so the shipped
	// source is the only place this claim can be asked.
	// ⚠️ All four are loaded UNCONDITIONALLY (⛔ not short-circuited) so a missing file reports
	// itself by name rather than hiding behind the first failure.
	FString ClimbableTowerSource;
	FString ClimbableTowerHeader;
	FString BuildingHeader;
	FString BuildingSource;
	const bool bLoadedTowerSource = LoadProjectSource(*this, ClimbableTowerSourcePath, ClimbableTowerSource);
	const bool bLoadedTowerHeader = LoadProjectSource(*this, ClimbableTowerHeaderPath, ClimbableTowerHeader);
	const bool bLoadedBuildingHeader = LoadProjectSource(*this, BuildingHeaderPath, BuildingHeader);
	const bool bLoadedBuildingSource = LoadProjectSource(*this, BuildingSourcePath, BuildingSource);

	if (bLoadedTowerSource && bLoadedTowerHeader && bLoadedBuildingHeader && bLoadedBuildingSource)
	{
		// ⭐⭐ THE INSTRUMENT SELF-CHECK, AND IT IS A GOOD ONE: the scanner is proven able to
		// FIND the token, on a file that legitimately contains it. `ClimbableTower.cpp` names
		// /Game/Meshes/SM_WatchTower in a socket-diagnostic UE_LOG — a message, ⛔ not a
		// decision. Without this row, "zero occurrences" everywhere else could just mean the
		// scanner was blind.
		TestTrue(TEXT("SELF-CHECK ⭐: the code scanner CAN find 'WatchTower' — it finds the socket-diagnostic log line in ClimbableTower.cpp, which is a MESSAGE and not a branch"),
			CountOccurrencesInCode(ClimbableTowerSource, TEXT("WatchTower")) >= 1);

		TestEqual(TEXT("(d) ⛔⛔ Building.h names 'WatchTower' ZERO times in code — the file that DECIDES whether a building may scale must not know the card exists"),
			CountOccurrencesInCode(BuildingHeader, TEXT("WatchTower")), 0);
		TestEqual(TEXT("(d) ⛔⛔ Building.cpp likewise — the upgrade mutator gates on CanScaleFootprint(), ⛔ never on a name"),
			CountOccurrencesInCode(BuildingSource, TEXT("WatchTower")), 0);
		TestEqual(TEXT("(d) ⛔⛔ and the OVERRIDE itself names no card: ClimbableTower.h has zero 'WatchTower' in code"),
			CountOccurrencesInCode(ClimbableTowerHeader, TEXT("WatchTower")), 0);
		TestEqual(TEXT("(d) ⛔ …and reads no CardID either. ⚖️ The next climbable building must be protected by INHERITING, not by somebody remembering a paragraph"),
			CountOccurrencesInCode(ClimbableTowerHeader, TEXT("CardID")), 0);

		// The predicate is spelled ONCE per class — two copies is how a rule and its exception
		// come to disagree.
		TestEqual(TEXT("(d) the base declares the predicate exactly once"),
			CountOccurrencesInCode(BuildingHeader, TEXT("CanScaleFootprint")), 1);
		TestEqual(TEXT("(d) the tower overrides it exactly once"),
			CountOccurrencesInCode(ClimbableTowerHeader, TEXT("CanScaleFootprint")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  7. ⭐ THE UPGRADE **GRANTS** THE NEW HIT POINTS AND ⛔ DOES NOT REPAIR THE OLD DAMAGE
//     (`STACK-§5` `J-10`). ⚖️ A full heal would make the upgrade a repair tool — that is the
//     Masons card's job — and would make upgrading strictly better than defending.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackHealthGrantTest,
	"Siegebound.BuildingStack.TheUpgradeGrantsTheNewHitPointsAndNeverRepairsExistingDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackHealthGrantTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	float Step = 0.f;
	if (!ReadFloat(*this, Defaults, TEXT("StackHealthStep"), Step))
	{
		return false;
	}

	TStrongObjectPtr<ABuilding> Scratch = MakeScratchBuilding<ABuilding>();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: NewObject<ABuilding>() returned null — nothing below would mean anything."));
		return false;
	}

	// ⭐ SELF-CHECK: the world-free instance is authoritative by construction (AActor's
	// constructor sets ROLE_Authority), which is the precondition ApplyStackUpgrade demands.
	// ⚠️ Stated honestly: this proves the SERVER path runs, ⛔ not that a real client is
	// rejected — no replication is exercised anywhere in this directory.
	TestTrue(TEXT("SELF-CHECK: the scratch building is authoritative, so the M8 guard is not what is being measured below"),
		Scratch->HasAuthority());

	// A DAMAGED building — the whole point of J-10. 300 max, 120 current ⇒ 180 damage taken.
	constexpr float SeedMaxHP = 300.f;
	constexpr float SeedCurrentHP = 120.f;
	if (!SeedFloat(*this, Scratch.Get(), TEXT("MaxHP"), SeedMaxHP)
		|| !SeedFloat(*this, Scratch.Get(), TEXT("CurrentHP"), SeedCurrentHP))
	{
		return false;
	}

	TestEqual(TEXT("SELF-CHECK: a fresh building starts at zero upgrades"),
		Scratch->GetStackUpgradeCount(), 0);

	// ── (a) ONE UPGRADE ─────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("(a) the upgrade lands on a plain, undamaged-in-principle, scalable building"),
		Scratch->ApplyStackUpgrade());
	TestEqual(TEXT("(a) …and the ONE source of truth advanced by exactly one"),
		Scratch->GetStackUpgradeCount(), 1);

	float MaxAfterOne = 0.f;
	float CurrentAfterOne = 0.f;
	if (!ReadFloat(*this, Scratch.Get(), TEXT("MaxHP"), MaxAfterOne)
		|| !ReadFloat(*this, Scratch.Get(), TEXT("CurrentHP"), CurrentAfterOne))
	{
		return false;
	}

	// The expectation is built from the SERIES and the seed, ⛔ not typed as 450.
	TestEqual(TEXT("(a) MaxHP is the seeded pool times the shipped step"),
		MaxAfterOne, SeedMaxHP * Health(1), Tolerance);
	TestEqual(TEXT("(a) ⭐ CurrentHP moved by the DELTA — the building was GRANTED the new hit points"),
		CurrentAfterOne, SeedCurrentHP + (MaxAfterOne - SeedMaxHP), Tolerance);

	// ── (b) ⭐⭐ THE ROW THAT MATTERS: THE DAMAGE IS STILL THERE ──────────────────────────────
	// The missing HP before and after must be IDENTICAL. ⛔ A full heal (CurrentHP = MaxHP)
	// passes row (a)'s MaxHP claim untouched and dies here.
	TestEqual(TEXT("(b) ⭐⭐ THE DAMAGE IS UNREPAIRED: the missing hit points are exactly what they were before the upgrade. ⚖️ A full heal would make this a repair tool — that is the Masons card's job"),
		MaxAfterOne - CurrentAfterOne, SeedMaxHP - SeedCurrentHP, Tolerance);
	TestTrue(TEXT("(b) …so the building is still damaged, ⛔ not topped up"),
		CurrentAfterOne < MaxAfterOne);

	// ── (c) ⭐ TWO UPGRADES COMPOUND — n ≥ 2, the discriminating depth ────────────────────────
	TestTrue(TEXT("(c) the second upgrade lands"), Scratch->ApplyStackUpgrade());
	TestEqual(TEXT("(c) the count is 2"), Scratch->GetStackUpgradeCount(), 2);

	float MaxAfterTwo = 0.f;
	float CurrentAfterTwo = 0.f;
	if (!ReadFloat(*this, Scratch.Get(), TEXT("MaxHP"), MaxAfterTwo)
		|| !ReadFloat(*this, Scratch.Get(), TEXT("CurrentHP"), CurrentAfterTwo))
	{
		return false;
	}

	// ⭐ THE SHIPPED STATE AGREES WITH THE PURE SERIES AT n = 2 — the depth at which the
	// multiplicative and additive health readings finally disagree (2.25× vs 2.00×).
	TestEqual(TEXT("(c) ⭐ after TWO upgrades MaxHP is the seed times the SERIES at n=2 — the applied state and the pure seam can never drift apart"),
		MaxAfterTwo, SeedMaxHP * Health(2), Tolerance);
	TestTrue(TEXT("(c) ⛔ and that is NOT what an additive health series would give at n=2 — this row is what fails if health is ever flattened to match height in kind"),
		!FMath::IsNearlyEqual(MaxAfterTwo, SeedMaxHP * AdditiveReadingHealth(2, Step), Tolerance));
	TestEqual(TEXT("(c) the damage is STILL exactly the original damage after a second upgrade — it is never repaired, at any depth"),
		MaxAfterTwo - CurrentAfterTwo, SeedMaxHP - SeedCurrentHP, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  8. ⭐ THE HEIGHT IS APPLIED **Z ONLY**, FROM THE AUTHORED BASELINE, AND ⛔ NEVER ACCUMULATES
//     (`STACK-§5` `J-4`, his own words: "keeping the same width and length").
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackHeightApplicationTest,
	"Siegebound.BuildingStack.TheUpgradeScalesZOnlyFromTheAuthoredBaselineAndNeverAccumulates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackHeightApplicationTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	TStrongObjectPtr<ABuilding> Scratch = MakeScratchBuilding<ABuilding>();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("SELF-CHECK FAILED: NewObject<ABuilding>() returned null."));
		return false;
	}

	USceneComponent* const Root = ScaleRootOf(*this, Scratch.Get());
	if (!Root)
	{
		return false;
	}

	// ⭐ A DELIBERATELY NON-UNIT BASELINE, AND BOTH HALVES OF IT DO WORK:
	//   • Z = 2.0 stands in for a building the designer authored TALL. If the code multiplied
	//     the SERIES by nothing (i.e. wrote the multiplier straight into the scale), every row
	//     below would be off by 2× — so this catches a baseline that is ignored.
	//   • X = Y = 1.4 stands in for a footprint the placement wheel already set (TASK-815,
	//     range [1.0, 1.5]). J-4 says an upgrade INHERITS it verbatim.
	constexpr float AuthoredZ = 2.f;
	constexpr float WheelSetXY = 1.4f;
	Root->SetRelativeScale3D(FVector(WheelSetXY, WheelSetXY, AuthoredZ));

	TestEqual(TEXT("SELF-CHECK: the seeded scale actually stuck — the rows below measure a real transform"),
		ScaleComponentAsFloat(Root->GetRelativeScale3D().Z), AuthoredZ, Tolerance);

	// ── (a) ONE UPGRADE ─────────────────────────────────────────────────────────────────────
	TestTrue(TEXT("(a) the upgrade lands"), Scratch->ApplyStackUpgrade());

	const FVector ScaleAfterOne = Root->GetRelativeScale3D();
	const float ZAfterOne = ScaleComponentAsFloat(ScaleAfterOne.Z);
	TestEqual(TEXT("(a) ⭐ Z is the AUTHORED baseline times the series — ⛔ not the series alone, which is what a forgotten baseline would give"),
		ZAfterOne, AuthoredZ * Height(1), Tolerance);
	TestEqual(TEXT("(a) ⛔ X is UNTOUCHED — J-4: 'keeping the same width and length'"),
		ScaleComponentAsFloat(ScaleAfterOne.X), WheelSetXY, Tolerance);
	TestEqual(TEXT("(a) ⛔ Y is UNTOUCHED"),
		ScaleComponentAsFloat(ScaleAfterOne.Y), WheelSetXY, Tolerance);

	// ── (b) ⭐⭐ THE SECOND UPGRADE — n = 2, WHERE THE TWO READINGS FINALLY DISAGREE ──────────
	// Additive: 2.0 × 3 = 6.0. Doubling: 2.0 × 4 = 8.0. An IN-PLACE multiply (`Scale.Z *= 2`)
	// would also give 8.0. ⇒ this single row catches BOTH the wrong series AND an accumulating
	// implementation, and it is the reason the test does not stop at one upgrade.
	TestTrue(TEXT("(b) the second upgrade lands"), Scratch->ApplyStackUpgrade());

	const FVector ScaleAfterTwo = Root->GetRelativeScale3D();
	const float ZAfterTwo = ScaleComponentAsFloat(ScaleAfterTwo.Z);
	TestEqual(TEXT("(b) ⭐⭐ Z is the baseline times the series AT n=2 — recomputed from the baseline, ⛔ not multiplied in place"),
		ZAfterTwo, AuthoredZ * Height(2), Tolerance);
	TestTrue(TEXT("(b) ⛔ …and that is NOT the doubling reading's answer. ⚖️ An in-place `*= 2` and the summary sentence produce the SAME wrong number here, so this row catches both"),
		!FMath::IsNearlyEqual(ZAfterTwo, AuthoredZ * DoublingReadingHeight(2), Tolerance));
	TestEqual(TEXT("(b) X still untouched after two upgrades"), ScaleComponentAsFloat(ScaleAfterTwo.X), WheelSetXY, Tolerance);
	TestEqual(TEXT("(b) Y still untouched after two upgrades"), ScaleComponentAsFloat(ScaleAfterTwo.Y), WheelSetXY, Tolerance);

	// ── (c) THE INCREMENT IS CONSTANT IN WORLD TERMS TOO ─────────────────────────────────────
	// The applied height grows by the SAME amount each upgrade — the additive series' defining
	// property, observed on the transform rather than on the pure function.
	TestEqual(TEXT("(c) ⭐ the applied Z grows by the SAME amount on the second upgrade as on the first — additive, all the way through to the component transform"),
		ZAfterTwo - ZAfterOne, ZAfterOne - AuthoredZ, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  9. ⭐ AT THE HEIGHT CAP THE CLICK **STILL BUYS HEALTH** (`STACK-§5` `J-6`) — his own
//     sentence: "it would only upgrade health by 1.5 times and not height."
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackAtTheCapTest,
	"Siegebound.BuildingStack.AtTheHeightCapTheClickStillBuysHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackAtTheCapTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	const ABuilding* const Defaults = BuildingDefaults(*this);
	if (!Defaults)
	{
		return false;
	}

	int32 Cap = 0;
	if (!ReadInt(*this, Defaults, TEXT("MaxStackHeightMultiplier"), Cap))
	{
		return false;
	}

	TStrongObjectPtr<ABuilding> Scratch = MakeScratchBuilding<ABuilding>();
	USceneComponent* const Root = ScaleRootOf(*this, Scratch.Get());
	if (!Scratch.IsValid() || !Root)
	{
		return false;
	}

	constexpr float SeedMaxHP = 200.f;
	if (!SeedFloat(*this, Scratch.Get(), TEXT("MaxHP"), SeedMaxHP)
		|| !SeedFloat(*this, Scratch.Get(), TEXT("CurrentHP"), SeedMaxHP))
	{
		return false;
	}

	Root->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));

	// Walk to the cap: the cap index is Cap − 1 upgrades.
	const int32 CapIndex = Cap - 1;
	for (int32 Index = 0; Index < CapIndex; ++Index)
	{
		if (!Scratch->ApplyStackUpgrade())
		{
			AddError(FString::Printf(TEXT("SELF-CHECK FAILED: upgrade %d was refused on a plain scalable building — the walk to the cap never got there."), Index + 1));
			return false;
		}
	}

	const float ZAtCap = ScaleComponentAsFloat(Root->GetRelativeScale3D().Z);
	float MaxHPAtCap = 0.f;
	float CurrentHPAtCap = 0.f;
	if (!ReadFloat(*this, Scratch.Get(), TEXT("MaxHP"), MaxHPAtCap)
		|| !ReadFloat(*this, Scratch.Get(), TEXT("CurrentHP"), CurrentHPAtCap))
	{
		return false;
	}

	TestEqual(TEXT("SELF-CHECK ⭐: the walk landed the building EXACTLY on the shipped cap — the premise of everything below"),
		ZAtCap, static_cast<float>(Cap), Tolerance);

	// ── ⭐⭐ ONE MORE CLICK, PAST THE CAP ─────────────────────────────────────────────────────
	TestTrue(TEXT("(a) ⭐ the upgrade PAST the cap still SUCCEEDS — the ghost stays blue because the click still does something (J-6). ⛔ Refusing here would be the silent behaviour change STACK-§5 refused"),
		Scratch->ApplyStackUpgrade());

	TestEqual(TEXT("(b) ⛔ HEIGHT did NOT move — it is pinned at the cap"),
		ScaleComponentAsFloat(Root->GetRelativeScale3D().Z), ZAtCap, Exact);

	float MaxHPPastCap = 0.f;
	float CurrentHPPastCap = 0.f;
	if (!ReadFloat(*this, Scratch.Get(), TEXT("MaxHP"), MaxHPPastCap)
		|| !ReadFloat(*this, Scratch.Get(), TEXT("CurrentHP"), CurrentHPPastCap))
	{
		return false;
	}

	TestEqual(TEXT("(c) ⭐⭐ but HEALTH DID move — the pool grew by exactly one step. ⚖️ His own sentence rules the behaviour at the cap: 'it would only upgrade health by 1.5 times and not height'"),
		MaxHPPastCap, MaxHPAtCap * Health(1), Tolerance);
	TestTrue(TEXT("(c) …and the count kept advancing, because health has no ceiling to stop at"),
		Scratch->GetStackUpgradeCount() == Cap);
	TestEqual(TEXT("(c) an UNDAMAGED building stays undamaged — the delta grant tops out at the new max, ⛔ never above it"),
		CurrentHPPastCap, MaxHPPastCap, Tolerance);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  10. ⛔⛔ THE REFUSAL IS ENFORCED **AT THE BUILDING**, NOT ONLY IN THE PLACEMENT PATH — two
//      independent mechanisms, so a future caller that forgets `STACK-§2` still cannot scale a
//      climbable tower.
// ═══════════════════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeBuildingStackClimbableTowerRefusalTest,
	"Siegebound.BuildingStack.AClimbableTowerRefusesTheUpgradeAtTheBuildingItselfNotOnlyInThePlacementPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeBuildingStackClimbableTowerRefusalTest::RunTest(const FString& Parameters)
{
	using namespace SiegeBuildingStackTestFixture;

	TStrongObjectPtr<AClimbableTower> Tower = MakeScratchBuilding<AClimbableTower>();
	USceneComponent* const Root = ScaleRootOf(*this, Tower.Get());
	if (!Tower.IsValid() || !Root)
	{
		return false;
	}

	constexpr float SeedMaxHP = 250.f;
	constexpr float SeedCurrentHP = 250.f;
	if (!SeedFloat(*this, Tower.Get(), TEXT("MaxHP"), SeedMaxHP)
		|| !SeedFloat(*this, Tower.Get(), TEXT("CurrentHP"), SeedCurrentHP))
	{
		return false;
	}

	Root->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));

	// ── SELF-CHECK ⭐⭐: THE SAME CALL SUCCEEDS ON A PLAIN BUILDING ───────────────────────────
	// ⚠️⚠️ WITHOUT THIS ROW THE TEST BELOW IS WORTHLESS. "ApplyStackUpgrade returned false"
	// would also be the answer if the mutator were broken, unreachable, or refused everything.
	// Proving the identical call succeeds on a scalable building is what turns a false into a
	// STATEMENT ABOUT THE TOWER.
	{
		TStrongObjectPtr<ABuilding> Control = MakeScratchBuilding<ABuilding>();
		if (!Control.IsValid())
		{
			AddError(TEXT("SELF-CHECK FAILED: the control building could not be created."));
			return false;
		}
		TestTrue(TEXT("SELF-CHECK ⭐⭐: the IDENTICAL call SUCCEEDS on a plain ABuilding — so a refusal below is about the TOWER and not about a broken mutator"),
			Control->ApplyStackUpgrade());
	}

	// ── (a) THE REFUSAL ─────────────────────────────────────────────────────────────────────
	TestFalse(TEXT("(a) ⛔⛔ ApplyStackUpgrade REFUSES on a climbable tower — the predicate is re-asked at the building itself, so a caller that forgets STACK-§2 still cannot fire TOWER-§8.5a's voiding condition"),
		Tower->ApplyStackUpgrade());

	// ── (b) AND IT IS A **NO-OP**, not a partial application ────────────────────────────────
	// ⚠️ A refusal that had already incremented the count or already scaled the mesh would be
	// worse than no refusal at all — it would leave a tower that reads as un-upgraded while
	// standing at the wrong height.
	TestEqual(TEXT("(b) ⛔ the upgrade count did NOT advance"),
		Tower->GetStackUpgradeCount(), 0);
	TestEqual(TEXT("(b) ⛔ the mesh Z was NOT scaled — the LadderFoot/LadderTop sockets and the rung plane are exactly where TOWER-§8.3 pinned them"),
		ScaleComponentAsFloat(Root->GetRelativeScale3D().Z), 1.f, Exact);

	float MaxHPAfter = 0.f;
	float CurrentHPAfter = 0.f;
	if (ReadFloat(*this, Tower.Get(), TEXT("MaxHP"), MaxHPAfter)
		&& ReadFloat(*this, Tower.Get(), TEXT("CurrentHP"), CurrentHPAfter))
	{
		TestEqual(TEXT("(b) ⛔ and it gained NO health either — the refusal is total, ⛔ not 'height only'"),
			MaxHPAfter, SeedMaxHP, Exact);
		TestEqual(TEXT("(b) ⛔ CurrentHP untouched"),
			CurrentHPAfter, SeedCurrentHP, Exact);
	}

	// ── (c) REPEATED CLICKS CHANGE NOTHING ──────────────────────────────────────────────────
	// The placement path will let a player click a WatchTower as often as they like; the
	// refusal must be stable, ⛔ not a one-shot latch that opens on the second press.
	for (int32 Attempt = 0; Attempt < 5; ++Attempt)
	{
		TestFalse(*FString::Printf(TEXT("(c) attempt %d is refused too — the exclusion is a PROPERTY of the class, ⛔ not a latch"), Attempt + 2),
			Tower->ApplyStackUpgrade());
	}
	TestEqual(TEXT("(c) …and after six attempts the tower is still exactly as it was authored"),
		Tower->GetStackUpgradeCount(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
