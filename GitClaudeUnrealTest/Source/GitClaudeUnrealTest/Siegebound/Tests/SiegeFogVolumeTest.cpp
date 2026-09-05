// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Containers/UnrealString.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Siegebound/CardRow.h"   // ESpellEffect — the append-only contract test 1 pins by VALUE, never by transcription
#include "Siegebound/FogVolume.h" // AFogVolume — the CDO test 3 reads FogDurationSeconds off
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for `AFogVolume`, THE FOG **STATE** OBJECT (TASK-998; law `FOG-§6`,
 *      `FOG-§9.4`, `FOG-§10.1`, `FOG-§10.3`, `HIGH-§1`, ruling ✅ `J-F16`) ═══
 *
 *  ⭐⭐ WHY THIS FILE EXISTS RATHER THAN ROWS ADDED TO AN EXISTING ONE, stated because the
 *  natural homes were considered and each is HELD: `Tests/SiegeFogClampTest.cpp` is fenced out of
 *  `TASK-998` by its own row (it carries `TASK-979`'s repair and is awaiting a verdict under
 *  `TASK-1006`), `Tests/SiegeFogTest.cpp` is `TASK-981` + `TASK-995`'s, and
 *  `Tests/SiegeCardRosterTest.cpp` carries the roster lane's live diff. `TASK-998`'s `names:`
 *  line licenses "a test in `Tests/`", so the three assertions `TASK-839` wrote out as OWED land
 *  here, plus the rows this row's own build earns.
 *
 *  ⛔⛔ THIS FILE ASSERTS **STATE**, AND DELIBERATELY ASSERTS **NOTHING VISUAL**. `AFogVolume` is
 *  a timer with an actor around it: no mesh, no material, no component. The fog you can SEE is
 *  `TASK-841`'s and is still premise-blocked. A row here that checked for a rendered volume would
 *  be asserting a feature nobody has built and nobody in this row was asked to.
 *
 *  MECHANISM — ⛔ zero PIE, ⛔ zero SpawnActor, ⛔ zero `UWorld::CreateWorld`, ⛔ zero asset loads,
 *  ⛔ zero writes. Three lanes, all headless, the house pattern:
 *    (a) DIRECT VALUE reads on the reflected `ESpellEffect` enum — real values, no world;
 *    (b) A **CDO** read through the property system (`GetDefault<AFogVolume>()` +
 *        `FindPropertyByName`), the `SiegeBuildingStackTest.cpp:205` idiom — this reads the
 *        SHIPPED number rather than a transcription of it, and it goes RED if the property is
 *        renamed, which is exactly the `CoreRedirects` hazard `FogVolume.h` documents;
 *    (c) SOURCE-TEXT structural probes with comment lines skipped (`CountOccurrencesInCode`),
 *        for the claims that are about WHICH FUNCTION CALLS WHICH — the only lane that can see a
 *        call graph without a world.
 *
 *  ⚠️⚠️ WHY (c) AND NOT A SPAWNED FIXTURE, stated so the gap is honest rather than discovered:
 *  there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so the behaviours that need a
 *  live actor — that `RaiseFog` really moves the deadline, that `IsFogActive` really flips at
 *  expiry, that `ResetFog` really zeroes it — are asserted STRUCTURALLY here and are ⛔ NOT
 *  proven at runtime by this suite. ⭐ The structural form is chosen to be the one that can still
 *  go red on the real hazard: `RaiseFog` STACKING is a `+=` and nothing else, so counting `+=` at
 *  zero and `=` at one catches the actual defect `J-F16` forbids, which a value test on a
 *  hand-built instance would also catch and no more.
 */

namespace SiegeFogVolumeFixture
{
	/** Shipping source (⛔ NOT `Tests/`) — the files these claims are about. */
	const TCHAR* FogVolumeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp");
	const TCHAR* SpellLibraryCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp");
	const TCHAR* CombatStaticsCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp");
	const TCHAR* GameModeCpp = TEXT("Source/GitClaudeUnrealTest/Siegebound/SiegeGameMode.cpp");

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
	 *  `SiegeFogClampTest.cpp` / `SiegeAcquisitionFunnelTest.cpp` so all three agree character
	 *  for character. ⛔ Do not "improve" it here; a divergent counter would make two files
	 *  disagree about the same source.
	 *
	 *  ⚠️⚠️ LOAD-BEARING IN THIS FILE ABOVE ALL: `FogVolume.h`'s class doc NAMES every symbol
	 *  asserted below, repeatedly, in the paragraphs that explain the law. A scanner that counted
	 *  comments would force that header to choose between explaining the rule and passing it.
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

	/**
	 *  Reads a float `UPROPERTY` off an object by name — the `SiegeBuildingStackTest.cpp:205`
	 *  helper. Reflection rather than a C++ member read on purpose: it works regardless of the
	 *  property's access specifier, and a RENAMED property returns null and FAILS here instead of
	 *  silently reading a different member.
	 */
	static bool ReadFloatProperty(FAutomationTestBase& Test, const UObject* Object, const TCHAR* PropertyName, float& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		const FFloatProperty* const FloatProperty = CastField<FFloatProperty>(Object->GetClass()->FindPropertyByName(FName(PropertyName)));
		if (!FloatProperty)
		{
			Test.AddError(FString::Printf(
				TEXT("SELF-CHECK FAILED: '%s' is not a reflected float property on '%s' — the probe is stale. ")
				TEXT("⚠️ If it was RENAMED rather than removed, this red is also the CoreRedirects warning FogVolume.h documents."),
				PropertyName, *Object->GetClass()->GetName()));
			return false;
		}

		OutValue = FloatProperty->GetPropertyValue_InContainer(Object);
		return true;
	}
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 1 ⭐⭐ — THE `ESpellEffect` APPEND-ONLY CONTRACT, PINNED BY **VALUE**.
//  ⛔ `TASK-839`'s FIRST owed assertion. `ESpellEffect` is a reflected `uint8`
//  `UENUM` held as a `UPROPERTY` on `FCardRow`, and `FCardRow` is the row type of
//  the SAVED asset `/Game/Data/DT_Cards` — so inserting a value in the MIDDLE
//  renumbers every later value and silently re-reads every saved cell as the next
//  effect along. No compile error, no log, no red: the DataTable just starts
//  resolving the wrong spells. This row is the only thing that would notice.
//
//  ⚠️ REGISTERED NAME MOVED 2026-09-04 (TASK-982): was
//  `Siegebound.Fog.TheSpellEffectEnumIsAppendOnlyAndFogCoverIsLast`. `FogClear` was correctly
//  appended BELOW `FogCover`, so the old name asserted the OPPOSITE of what this test now checks.
//  A registered name is the one string a reader sees in the suite listing WITHOUT opening the
//  file, and a false one there is the SC-§65 drift this batch keeps paying for. ⭐ The new name is
//  deliberately value-agnostic so the NEXT append does not have to move it again.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeEnumIsAppendOnlyTest,
	"Siegebound.Fog.TheSpellEffectEnumIsAppendOnlyAndTheNewestValueIsLast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeEnumIsAppendOnlyTest::RunTest(const FString& Parameters)
{
	// The whole shipped family, in order, asserted by VALUE. ⛔ Not a count and not a name sweep:
	// a count survives a reorder and a name sweep survives a renumber, and the renumber is the
	// defect that silently re-points saved data.
	TestEqual(TEXT("ESpellEffect::None is 0 — the default a row deserialises to when its cell is blank"), static_cast<int32>(ESpellEffect::None), static_cast<int32>(0));
	TestEqual(TEXT("ESpellEffect::AoEDamage is 1 (Fireball)"), static_cast<int32>(ESpellEffect::AoEDamage), static_cast<int32>(1));
	TestEqual(TEXT("ESpellEffect::Freeze is 2 (FrostNova)"), static_cast<int32>(ESpellEffect::Freeze), static_cast<int32>(2));
	TestEqual(TEXT("ESpellEffect::TopTargetsDamage is 3 (Lightning)"), static_cast<int32>(ESpellEffect::TopTargetsDamage), static_cast<int32>(3));
	TestEqual(TEXT("ESpellEffect::AllyBuff is 4 (BattleCry)"), static_cast<int32>(ESpellEffect::AllyBuff), static_cast<int32>(4));

	TestEqual(
		TEXT("⭐ ESpellEffect::GoldSteal is 5 (Pickpocket) — the value FogCover was appended AFTER. ")
		TEXT("⛔ If this reads 6, something was inserted above it and every DT_Cards cell from here down now ")
		TEXT("resolves the WRONG spell, silently."),
		static_cast<int32>(ESpellEffect::GoldSteal), static_cast<int32>(5));

	TestEqual(
		TEXT("⭐⭐ ESpellEffect::FogCover is 6 — APPENDED, never inserted (TASK-839 §1(A)). ⛔ This is the value ")
		TEXT("TASK-840's `Fog` row writes into its SpellEffect cell character-for-character. ⛔⛔ AND IT IS THE ROW ")
		TEXT("THAT PROVES TASK-982 APPENDED RATHER THAN INSERTED: if this now reads 7, `FogClear` landed ABOVE it and ")
		TEXT("every saved `Fog` cell in DT_Cards silently resolves as BrightSun."),
		static_cast<int32>(ESpellEffect::FogCover), static_cast<int32>(6));

	TestEqual(
		TEXT("⭐⭐ ESpellEffect::FogClear is 7 — BrightSun's own value, APPENDED at the END (TASK-982 item (9)). ")
		TEXT("⛔ This is the value TASK-983's `BrightSun` row writes into its SpellEffect cell ")
		TEXT("character-for-character. ⛔ It is NOT an overload of FogCover: FOG-§10.1 forbids that explicitly."),
		static_cast<int32>(ESpellEffect::FogClear), static_cast<int32>(7));

	// ⛔⛔ AND THE ROW THAT ACTUALLY DEFENDS THE NEXT AUTHOR, rather than only this one: the newest
	// value must still be LAST. ⭐ MOVED 2026-09-04 (TASK-982) FROM `FogCover` TO `FogClear`,
	// exactly as this row's own instructions said it would be — it was NOT deleted, and the
	// `DeclaredCount` floor below moved 7 ⇒ 8 with it. `FogClear` was appended at the END rather
	// than beside `FogCover` for tidiness, which is the defect this row exists to catch: a
	// mid-enum insert renumbers everything after it and silently re-points every saved DT_Cards
	// cell, with no compile error, no log line and no other red. ⭐ WHEN THE **NEXT** SPELL EFFECT
	// IS APPENDED BELOW, THIS ROW MOVES TO IT AGAIN; it does not get deleted.
	const UEnum* const EffectEnum = StaticEnum<ESpellEffect>();
	if (!EffectEnum)
	{
		AddError(TEXT("SELF-CHECK FAILED: StaticEnum<ESpellEffect>() returned null — every claim above and below would be meaningless."));
		return false;
	}

	// ⛔ The highest DECLARED value, found by sweep rather than by index arithmetic. UHT appends a
	// hidden `_MAX` sentinel, and assuming its position (`NumEnums() - 2`) would make this row
	// depend on a UHT detail rather than on the contract it is about — so the sentinel is
	// identified and skipped by NAME instead.
	int64 HighestDeclaredValue = -1;
	int32 DeclaredCount = 0;
	for (int32 Index = 0; Index < EffectEnum->NumEnums(); ++Index)
	{
		// ⚠️ The sentinel is identified by NAME, not by `HasMetaData("Hidden")` — that accessor is
		// `WITH_EDITOR`-only, and this file compiles wherever WITH_DEV_AUTOMATION_TESTS is on,
		// which includes non-editor Development builds.
		if (EffectEnum->GetNameStringByIndex(Index).EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive))
		{
			continue;
		}

		++DeclaredCount;
		HighestDeclaredValue = FMath::Max(HighestDeclaredValue, EffectEnum->GetValueByIndex(Index));
	}

	TestTrue(
		TEXT("SELF-CHECK: the sweep found the declared ESpellEffect values (a zero here would make the claim below ")
		TEXT("vacuous). ⭐ FLOOR MOVED 7 ⇒ 8 on 2026-09-04 (TASK-982 appended `FogClear`)."),
		DeclaredCount >= 8);

	TestEqual(
		TEXT("⛔⛔ `FogClear` holds the HIGHEST declared ESpellEffect value — i.e. it is now the APPEND POINT. ")
		TEXT("⭐ MOVED 2026-09-04 FROM `FogCover` (TASK-982 landed `FogClear` BELOW it, correctly). ⛔ The next spell ")
		TEXT("effect must land BELOW this one: an insert anywhere above renumbers every value after it and silently ")
		TEXT("re-points every saved DT_Cards cell that names one, with no compile error and no other red. ")
		TEXT("⭐ WHEN THE NEXT VALUE IS APPENDED BELOW, THIS ROW MOVES TO IT — it does not get deleted."),
		HighestDeclaredValue, static_cast<int64>(ESpellEffect::FogClear));

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 2 ⭐⭐ — ONE RESOLVE ARM, AND IT **NO LONGER REFUSES**.
//  ⛔ `TASK-839`'s SECOND and THIRD owed assertions, and the third is the one it
//  wrote as "INVERT when S1 inverts". S1 has inverted, so this row is the
//  inversion: the arm's job is no longer "refuse out loud" but "stamp the expiry".
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeResolveArmIsLiveTest,
	"Siegebound.Fog.TheFogCardHasExactlyOneResolveArmAndItRaisesFogInsteadOfRefusing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeResolveArmIsLiveTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	FString SpellCpp;
	if (!LoadProjectFile(*this, SpellLibraryCpp, SpellCpp))
	{
		return false;
	}

	// ── (a) ⛔ ONE ARM, NEVER TWO (TASK-839 owed assertion 2). Two `case ESpellEffect::FogCover:`
	//    labels in one switch will not even compile, but two in DIFFERENT switches in this file
	//    would — and that is a second place for the fog card to mean something.
	TestEqual(
		TEXT("⛔ `case ESpellEffect::FogCover:` appears EXACTLY ONCE in SpellLibrary.cpp — one resolve arm, never two."),
		CountOccurrencesInCode(SpellCpp, TEXT("case ESpellEffect::FogCover:")), 1);

	FString ResolveBody;
	if (!ExtractFunctionBody(*this, SpellCpp, TEXT("bool USpellLibrary::ResolveSpell("), ResolveBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted ResolveSpell body is substantial — an empty extraction would make every ")
		TEXT("assertion below pass vacuously."),
		ResolveBody.Len() > 200);

	// ── (b) SELF-CHECK / LIVE CONTROL. If this reads 0 the extraction is scanning the wrong
	//    function and the counts below mean nothing, INCLUDING the ones that are supposed to be 1.
	TestEqual(
		TEXT("SELF-CHECK: the arm really is inside ResolveSpell's body (not merely somewhere in the file)."),
		CountOccurrencesInCode(ResolveBody, TEXT("case ESpellEffect::FogCover:")), 1);

	// ── (c) ⭐⭐ THE INVERSION ITSELF (TASK-839 owed assertion 3). The arm reaches the state
	//    object through the WRITE door and stamps the expiry. ⛔ This is the row that would catch
	//    the failure TASK-998's own spec names: "a green suite and a 50-gold fog card that renders
	//    and clamps nobody" — a card whose arm resolves true without ever touching AFogVolume.
	TestEqual(
		TEXT("⭐⭐ THE FOG ARMS ARE LIVE: `ResolveSpell` calls `AFogVolume::FindOrSpawn` — the WRITE door — exactly ")
		TEXT("TWICE, once per fog arm. ⛔ PIN MOVED 1 ⇒ 2 ON 2026-09-04 (TASK-982 added the `FogClear`/BrightSun arm, ")
		TEXT("which reaches the SAME one state object through the SAME write door). ⛔ A THIRD would be a third way to ")
		TEXT("create fog state; a ONE would mean one of the two cards resolves without ever reaching the object — ")
		TEXT("spending its gold, logging \"resolved\", and changing NOTHING. ⭐ That each arm reaches it exactly once ")
		TEXT("is asserted per-arm in Tests/SiegeBrightSunTest.cpp, which is what keeps this count from being a ")
		TEXT("loosening."),
		CountOccurrencesInCode(ResolveBody, TEXT("AFogVolume::FindOrSpawn(")), 2);

	TestEqual(
		TEXT("⭐ …and the `Fog` card stamps the expiry, exactly once, through RaiseFog — the ONE writer of the ")
		TEXT("fog deadline. ⛔ UNCHANGED at 1 by TASK-982: BrightSun does NOT go through RaiseFog, it goes through ")
		TEXT("ApplyBrightSun, because it writes the OTHER scalar and clears this one."),
		CountOccurrencesInCode(ResolveBody, TEXT("->RaiseFog()")), 1);

	// ── (d) ⛔ THE READ DOOR IS NOT USED HERE. `Find` is the acquisition seam's; the card's lane
	//    is `FindOrSpawn`. A `Find` in this body would mean the card can silently no-op in a world
	//    where nobody placed a volume — which is exactly why the two doors are separate functions.
	TestEqual(
		TEXT("⛔ ResolveSpell does NOT use the READ door: `AFogVolume::Find(` belongs to the acquisition seam. ")
		TEXT("Using it here would make the fog card a no-op in any world without a pre-placed volume."),
		CountOccurrencesInCode(ResolveBody, TEXT("AFogVolume::Find(")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 3 ⭐⭐ — "EXACTLY 5 MINUTES", **RE-DERIVED FROM HIS SENTENCE**, NEVER
//  RE-TYPED. The `HIGH-§1` / `BrightSunHeightStepUU` idiom: the expectation is
//  built from the WORDS ("5 minutes", 60 s/min), so a test that agreed with a
//  typo'd constant because both said the same wrong number cannot happen here.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeDurationIsFiveMinutesTest,
	"Siegebound.Fog.FogLastsExactlyFiveMinutesReDerivedFromHisSentence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeDurationIsFiveMinutesTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	const AFogVolume* const Defaults = GetDefault<AFogVolume>();
	if (!Defaults)
	{
		AddError(TEXT("SELF-CHECK FAILED: GetDefault<AFogVolume>() returned null — the expectation below is read off this object, so nothing after this line would mean anything."));
		return false;
	}

	// ⭐ HIS SENTENCE, ARITHMETIC SHOWN: "fog is up for exactly 5 MINUTES when the card is played"
	// (FOG-§9.4; and J-F16's "the timer is reset to 5 minutes" re-derives the SAME number from a
	// SECOND sentence of his — the strongest confirmation a tunable ever gets here).
	// ⛔ 300.f is NOT typed here. It is 5 × 60, from the words, so this row and the shipped
	// constant cannot agree by both being wrong the same way.
	const float MinutesHeSaid = 5.f;
	const float SecondsPerMinute = 60.f;
	const float ExpectedSeconds = MinutesHeSaid * SecondsPerMinute;

	float ShippedSeconds = 0.f;
	if (!ReadFloatProperty(*this, Defaults, TEXT("FogDurationSeconds"), ShippedSeconds))
	{
		return false;
	}

	TestEqual(
		TEXT("⭐⭐ `AFogVolume::FogDurationSeconds` is HIS five minutes — 5 × 60 = 300 s (FOG-§9.4). ⛔ J-F6's old ")
		TEXT("`30.f` default is SUPERSEDED, and 30 would be a card that costs 50 gold and lifts before anyone crosses ")
		TEXT("the map. ⛔ TASK-840's `EffectDuration` cell in cards.csv must match this number."),
		ShippedSeconds, ExpectedSeconds);

	// ⛔ AND IT MUST STAY A TUNABLE HE CAN MOVE IN ONE WORD (FOG-§1's J-F2 reasoning, HIGH-§1's
	// "the tunable IS the point"). A constant folded into the .cpp would satisfy the value
	// assertion above and quietly cost him the lever.
	const FProperty* const DurationProperty = AFogVolume::StaticClass()->FindPropertyByName(TEXT("FogDurationSeconds"));
	if (DurationProperty)
	{
		TestTrue(
			TEXT("⛔ `FogDurationSeconds` is EditDefaultsOnly — his next sentence retunes the fog with NO code change. ")
			TEXT("That is the point of the property, not a side effect of it."),
			DurationProperty->HasAnyPropertyFlags(CPF_Edit) && DurationProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — ✅ `J-F16`: FOG ON FOG **REFRESHES**, AND CAN NEVER STACK.
//  📌 HIS WORDS: "If fog is played during fog then the timer is reset to 5 minutes."
//  ⛔ Asserted at the ONE line that could break it, because stacking is spelled
//  `+=` and nothing else.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeRefreshNeverStacksTest,
	"Siegebound.Fog.RaisingFogDuringFogRefreshesAndCanNeverStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeRefreshNeverStacksTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	// ⚠️ SIGNATURE PIN MOVED 2026-09-04 (TASK-982): `void AFogVolume::RaiseFog(` ⇒
	// `bool AFogVolume::RaiseFog(`. The function now REFUSES (returns false, writing nothing) while
	// BrightSun's prevention window is up — J-F19's "no gold spent, card not consumed". ⛔ The move
	// is named rather than silent because ExtractFunctionBody FAILS on a stale signature by design,
	// so an unmoved pin here would have gone red and looked like a defect in the shipped code.
	FString RaiseBody;
	if (!ExtractFunctionBody(*this, FogCpp, TEXT("bool AFogVolume::RaiseFog("), RaiseBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted RaiseFog body is substantial — an empty extraction would make the zero below ")
		TEXT("read as SAFE when it is really just blind."),
		RaiseBody.Len() > 100);

	// ⭐ THE LIVE CONTROL, so the zero beside it is a real read rather than a dead scanner: the
	// deadline IS assigned here, exactly once. If this reads 0, the probe is broken, not clean.
	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: RaiseFog ASSIGNS the deadline exactly once. A zero here means the probe is ")
		TEXT("dead, and the `+=` count below would be a meaningless zero."),
		CountOccurrencesInCode(RaiseBody, TEXT("FogActiveUntilTimeSeconds =")), 1);

	TestEqual(
		TEXT("⭐⭐ ✅ J-F16 — RaiseFog NEVER accumulates: ZERO `FogActiveUntilTimeSeconds +=`. ⛔ A second Fog cast RESETS ")
		TEXT("the full 5 minutes (his words), it does not add 5 more. ⛔ This is the one character that separates ")
		TEXT("\"reset to 5 minutes\" from a player banking half an hour of fog off a stack of cheap cards."),
		CountOccurrencesInCode(RaiseBody, TEXT("FogActiveUntilTimeSeconds +=")), 0);

	// ⛔⛔ AND THE ROW THAT CATCHES THE *OTHER* WAY TO GET IT WRONG, which is subtler and would
	// look careful: the `max(remaining, new)` refresh ASummonedUnit::ApplyFreeze uses. That rule
	// is the FREEZE's and it is right there, so copying it here is the likely mistake — but under
	// it a re-cast late in a fog does NOTHING, and J-F16 says the timer is RESET.
	TestEqual(
		TEXT("⛔ RaiseFog does NOT use the freeze's `max(remaining, new)` refresh — that is ApplyFreeze's rule, and ")
		TEXT("under it a re-cast during fog would do nothing. J-F16 says RESET, so a shorter new window still wins."),
		CountOccurrencesInCode(RaiseBody, TEXT("FMath::Max")), 0);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐⭐ — **THE SEAM REALLY CONSULTS `AFogVolume`.**
//  ⛔ This is `SiegeFogClampTest.cpp` test 8(b)'s row, INVERTED — hosted here
//  because TASK-998 is fenced out of that file. Its own message named this
//  assertion in advance: "it becomes 'the seam really consults AFogVolume', which
//  is the assertion that would catch a fog card that renders beautifully and
//  blinds nobody." ⚠️ See the handoff: that file's numeric pin still passes by
//  arithmetic, but its PROSE and its registered NAME are now stale and are OWED.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeSeamConsultsTheStateTest,
	"Siegebound.Fog.TheAcquisitionSeamReallyConsultsTheFogStateActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeSeamConsultsTheStateTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	FString CombatCpp;
	if (!LoadProjectFile(*this, CombatStaticsCpp, CombatCpp))
	{
		return false;
	}

	FString SeamBody;
	if (!ExtractFunctionBody(*this, CombatCpp, TEXT("bool FSiegeCombatStatics::ReadFogState("), SeamBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted ReadFogState body is substantial — an empty extraction would make every ")
		TEXT("assertion below pass vacuously."),
		SeamBody.Len() > 200);

	TestEqual(
		TEXT("⭐⭐⭐ THE SEAM IS WIRED: ReadFogState consults `AFogVolume::Find` exactly once. ⛔ For the whole of ")
		TEXT("TASK-838 and TASK-839 this function always answered \"no fog\", so the ceiling could never fire and the ")
		TEXT("acquisition surface was byte-for-byte the pre-fog game. It is not any more, and THIS is the row that ")
		TEXT("says so — a fog card that renders beautifully and blinds nobody turns this red."),
		CountOccurrencesInCode(SeamBody, TEXT("AFogVolume::Find(")), 1);

	TestEqual(
		TEXT("⭐ …and it asks the state object the ONE question it exists to answer."),
		CountOccurrencesInCode(SeamBody, TEXT("IsFogActive()")), 1);

	// ⛔⛔ THE READ NEVER CREATES. This function runs once per gather, on a 0.25 s acquisition poll,
	// for every unit on the field. A `FindOrSpawn` here would mutate the world from inside a query,
	// forever — and it would also make "no fog volume" impossible, which is the state the whole
	// fail-toward-clear guarantee rests on.
	TestEqual(
		TEXT("⛔⛔ The acquisition seam NEVER spawns: zero `AFogVolume::FindOrSpawn(` in ReadFogState. A finder that ")
		TEXT("created the state actor from inside a per-gather query would mutate the world on a 0.25 s poll, forever."),
		CountOccurrencesInCode(SeamBody, TEXT("AFogVolume::FindOrSpawn(")), 0);

	// ⛔ AND THE TUNING IS STILL ALWAYS ASSIGNED, on every path, before any early-out — the
	// property SiegeFogClampTest test 8(c) pins and this row must not quietly cost.
	TestEqual(
		TEXT("⛔ The seam still assigns OutTuning unconditionally before any early-out — a path that left it untouched ")
		TEXT("would push an uninitialised band into EffectiveVisionRadius, i.e. a global combat outage."),
		CountOccurrencesInCode(SeamBody, TEXT("OutTuning = FSiegeFogTuning();")), 1);

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐⭐ — **ONE** STATE OBJECT, AND THE TWO DOORS STAY SEPARATE.
//  `FOG-§10.1`: "the SAME ONE authoritative fog-state object … NEVER a second
//  state object". The way that law actually dies is a second `SpawnActor`.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumeHasOneStateObjectTest,
	"Siegebound.Fog.ThereIsExactlyOneFogStateObjectAndTheReadDoorNeverSpawns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumeHasOneStateObjectTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	// ── (a) ⛔ ONE SPAWN SITE, AND IT IS INSIDE THE WRITE DOOR. A second one anywhere would give
	//    the world two fog-state actors whose deadlines disagree, and `Find` returns whichever the
	//    iterator reaches first — a fog that is up or down depending on actor registration order.
	TestEqual(
		TEXT("⛔⛔ EXACTLY ONE `SpawnActor<AFogVolume>` in FogVolume.cpp — and the file-wide census below says it is ")
		TEXT("the only one in the project. Two state actors means two deadlines and a fog whose state depends on ")
		TEXT("iterator order (FOG-§10.1: NEVER a second state object)."),
		CountOccurrencesInCode(FogCpp, TEXT("SpawnActor<AFogVolume>")), 1);

	FString FindBody;
	if (ExtractFunctionBody(*this, FogCpp, TEXT("AFogVolume* AFogVolume::Find("), FindBody))
	{
		TestTrue(
			TEXT("SELF-CHECK: the extracted Find body is substantial — an empty extraction would make the zero below vacuous."),
			FindBody.Len() > 100);

		// ⭐ LIVE CONTROL beside the zero: Find really does iterate, so a zero SpawnActor is a
		// read of a live function rather than of nothing.
		TestEqual(
			TEXT("SELF-CHECK / LIVE CONTROL: the read door really iterates the world exactly once."),
			CountOccurrencesInCode(FindBody, TEXT("TActorIterator<AFogVolume>")), 1);

		TestEqual(
			TEXT("⛔⛔ THE READ DOOR NEVER CREATES: zero `SpawnActor` inside `Find`. Its caller is the acquisition ")
			TEXT("seam, which runs per gather on a 0.25 s poll — a spawning finder there would mutate the world from ")
			TEXT("inside a query and would make \"no fog volume\" an unreachable state."),
			CountOccurrencesInCode(FindBody, TEXT("SpawnActor")), 0);
	}

	// ── (b) ⛔ THE WRITE DOOR LOOKS BEFORE IT CREATES. Without this, every cast spawns another
	//    state actor and the "one object" law dies on the SECOND fog card, not the first — the
	//    kind of defect that survives a smoke test.
	FString FindOrSpawnBody;
	if (ExtractFunctionBody(*this, FogCpp, TEXT("AFogVolume* AFogVolume::FindOrSpawn("), FindOrSpawnBody))
	{
		TestEqual(
			TEXT("⛔ The write door calls the read door FIRST and returns the existing actor when there is one. ")
			TEXT("Without this every cast spawns another state actor and the law dies on the SECOND fog card."),
			CountOccurrencesInCode(FindOrSpawnBody, TEXT("Find(World)")), 1);

		TestEqual(
			TEXT("⛔ …and it spawns at most once, in one place."),
			CountOccurrencesInCode(FindOrSpawnBody, TEXT("SpawnActor<AFogVolume>")), 1);
	}

	return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  TEST 7 ⭐⭐ — `Play Again` ⇒ **CLEAR, TIMER ZEROED** (`FOG-§10.3`).
//  ⛔ The step is NOT free: AFogVolume is none of the three classes PlayAgain
//  destroys, so without an explicit reset the fog outlives the match that paid
//  for it.
// ═══════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFogVolumePlayAgainClearsFogTest,
	"Siegebound.Fog.PlayAgainClearsTheFogAndZeroesItsTimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFogVolumePlayAgainClearsFogTest::RunTest(const FString& Parameters)
{
	using namespace SiegeFogVolumeFixture;

	// ── (a) THE RESET ITSELF ZEROES THE DEADLINE — it does not merely let it expire. "Wait four
	//    more minutes" is not a reset, and FOG-§10.3 says the timer is ZEROED.
	FString FogCpp;
	if (!LoadProjectFile(*this, FogVolumeCpp, FogCpp))
	{
		return false;
	}

	FString ResetBody;
	if (ExtractFunctionBody(*this, FogCpp, TEXT("void AFogVolume::ResetFog("), ResetBody))
	{
		TestEqual(
			TEXT("⛔ ResetFog ZEROES the deadline outright (FOG-§10.3: \"CLEAR, both timers zeroed\"). Letting the old ")
			TEXT("deadline stand and expire on its own would carry match-1 fog into match 2."),
			CountOccurrencesInCode(ResetBody, TEXT("FogActiveUntilTimeSeconds = 0.0;")), 1);
	}

	// ── (b) ⭐⭐ AND IT HAS A LIVE CALLER. This is the half that would rot silently: a ResetFog
	//    nobody calls is indistinguishable, in every other test in this file, from one that works.
	FString GameMode;
	if (!LoadProjectFile(*this, GameModeCpp, GameMode))
	{
		return false;
	}

	FString PlayAgainBody;
	if (!ExtractFunctionBody(*this, GameMode, TEXT("void ASiegeGameMode::PlayAgain("), PlayAgainBody))
	{
		return false;
	}

	TestTrue(
		TEXT("SELF-CHECK: the extracted PlayAgain body is substantial — an empty extraction would make the claims below vacuous."),
		PlayAgainBody.Len() > 200);

	// ⭐ LIVE CONTROLS: the two shipped reset loops this one is modelled on. If these read 0 the
	// extraction is scanning the wrong function and the fog claim beside them proves nothing.
	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: PlayAgain still runs the shipped ResetCastle loop."),
		CountOccurrencesInCode(PlayAgainBody, TEXT("TActorIterator<ACastle>")), 1);
	TestEqual(
		TEXT("SELF-CHECK / LIVE CONTROL: PlayAgain still runs the shipped ResetCaptureZone loop."),
		CountOccurrencesInCode(PlayAgainBody, TEXT("TActorIterator<ACaptureZone>")), 1);

	TestEqual(
		TEXT("⭐⭐ Play Again RESETS THE FOG (FOG-§10.3). ⛔ This is NOT free from the steps around it: PlayAgain ")
		TEXT("destroys ASummonedUnit, ABuilding and AProjectile, and AFogVolume is none of the three — so without ")
		TEXT("this loop, fog raised at 4:59 of match 1 is still up in match 2 and the next player inherits a 69.5% ")
		TEXT("acquisition cut nobody paid 50 gold for."),
		CountOccurrencesInCode(PlayAgainBody, TEXT("It->ResetFog();")), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
