// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Siegebound/SiegeGraphicsSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  AUTOMATION TESTS for USiegeGraphicsSettingsSubsystem (TASK-1113; CONVENTIONS
 *  GFX-§3, GFX-§4, GFX-§5, GFX-§8, GFX-§10).
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  SCOPE STATEMENT (SC-§79 — what these tests CAN and CANNOT detect)
 *  ─────────────────────────────────────────────────────────────────────────
 *  ✅ THEY CAN DETECT: a drifted canonical group name · a group wired to the
 *     wrong engine setter · a group that does not round-trip · an out-of-range
 *     write that does not clamp · a delegate that fires on a no-op (or fails to
 *     fire on a real change) · a Custom state that is not reported · a null
 *     UGameUserSettings that crashes or returns an undocumented fallback · the
 *     sg.ResolutionQuality=0 sentinel being clamped on READ · a resolution scale
 *     that is not clamped on WRITE · a SaveSettings reached from the provisional
 *     video-mode path · a revert that does NOT re-apply the resolution · a frame
 *     rate ladder that snaps Unlimited to 30 · a Tier-D derivation table that
 *     changed silently · volumetric fog wired to the Effects group instead of
 *     Shadow.
 *
 *  ⛔ THEY CANNOT DETECT — AND THESE RUNGS ARE OWED ELSEWHERE:
 *     - Whether the SCREEN actually changed. Every engine apply is suppressed
 *       here (see the scratch-store note below), so the Apply / Save counters
 *       record the facade's DECISION — that it reached the call site — not the
 *       engine's effect. GFX-§4's real proof is a human pressing "Revert" and
 *       seeing the picture come back: TASK-1118 / TASK-1119, by pixels.
 *     - Whether Scalability::SetQualityLevels actually moved a CVar, and whether
 *       a moved CVar changed a rendered frame (GFX-§9 / SC-§94 require pixels
 *       AND the engine log; that obligation is entirely unspent by this file).
 *     - Anything about the widget: TASK-1115 / TASK-1118 / TASK-1120.
 *     - Whether RunHardwareBenchmark produces sane levels on this hardware. The
 *       benchmark is refused under suppression on purpose; GFX-§6's cold path
 *       has never been run on this machine (LastCPUBenchmarkResult = -1).
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  HERMETICITY — the tests never touch the machine's real graphics settings
 *  ─────────────────────────────────────────────────────────────────────────
 *  Every subsystem built below is pointed at a SCRATCH NewObject<UGameUserSettings>
 *  via SetGameUserSettingsForAutomationTests BEFORE anything reads or writes, and
 *  that call ALSO suppresses the three engine calls (ApplyNonResolutionSettings,
 *  ApplyResolutionSettings, SaveSettings) in the same breath — because applying
 *  against a scratch object would still push global CVars through
 *  Scalability::SetQualityLevels and still request a real display-mode change in
 *  the live editor. A test run therefore cannot read, write or apply
 *  Saved/Config/WindowsEditor/GameUserSettings.ini. The first test asserts the
 *  scratch object is not GEngine's, so the guarantee is mechanical, not a comment.
 *
 *  ⚠️ THE ENGINE PLUMBING THAT IS NOT OPTIONAL: UGameInstanceSubsystem is
 *  UCLASS(Abstract, Within = GameInstance), so a subsystem object MUST be
 *  constructed with a UGameInstance as its Outer or StaticAllocateObject's
 *  ClassWithin check trips. Hence the throwaway UGameInstance. Both objects are
 *  held by TStrongObjectPtr — an Outer chain does NOT keep an object alive.
 *  Initialize(FSubsystemCollectionBase&) is never called (a collection cannot be
 *  fabricated outside the engine's creation path), which is fine: nothing in this
 *  facade's contract depends on it except the boot-time stale-mode heal, and that
 *  path is exercised directly by driving the scratch object instead.
 *  UGameUserSettings itself is UCLASS(config=..., MinimalAPI) with no Within, and
 *  its constructor calls only SetToDefaults() — ⛔ it never calls LoadSettings(),
 *  so a bare NewObject touches no file and a test run cannot read, write or apply
 *  the machine's real Saved/Config/.../GameUserSettings.ini.
 *
 *  ⛔ BUT IT IS NOT "AT ENGINE DEFAULTS" (TASK-1114 NIT-2, corrected). The class
 *  is config-backed and FObjectInitializer's archetype copy from the CDO runs
 *  AFTER the C++ constructor, so a scratch instance can carry THIS MACHINE'S
 *  values (never its file). ⇒ ⛔ No assertion in this file may assume a particular
 *  starting value. Where one did, it is now seeded first — see NIT-3(a) in
 *  WindowModeContract and NIT-3(b) in DelegateNeverFiresOnNoOp — and every other
 *  test reads its starting value rather than assuming it.
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  SC-§83 — THE NAMED MUTATIONS. The guard I wrote most recently is the one I
 *  trust least, so each is named with the test that must go RED for it.
 *  ─────────────────────────────────────────────────────────────────────────
 *   M1  In SiegeGraphicsSettingsSubsystem.cpp, change GroupName_EffectsQuality
 *       to TEXT("VisualEffectQuality") (the engine's wrapper spelling — the
 *       single most likely real-world drift).      ⇒ RED: CanonicalNames
 *   M2  In ApplyQualityGroupLevelInternal, swap the GroupName_ShadowQuality and
 *       GroupName_ShadingQuality branches.         ⇒ RED: GroupRoundTrip
 *   M3  In ApplyQualityGroupLevelInternal, delete the `CurrentLevel ==
 *       ClampedLevel` early-return.                ⇒ RED: DelegateNeverFiresOnNoOp
 *   M4  In ApplyQualityGroupLevelInternal, replace FMath::Clamp(NewLevel, …)
 *       with NewLevel.                             ⇒ RED: DelegateNeverFiresOnNoOp
 *       ⛔ CORRECTED BY TASK-1114 WARN-2 — THIS DOES NOT REDDEN
 *       OutOfRangeWriteClamps, AND THE ORIGINAL PREDICTION THAT IT WOULD WAS
 *       WRONG. The engine clamps to the IDENTICAL band one layer down:
 *       FQualityLevels::Set<Group>Quality is FMath::Clamp(Value, 0,
 *       NumLevels-1) = 0..4 for every one of the ten groups
 *       (Scalability.cpp:1117-1160), and SetFromSingleQualityLevel clamps the
 *       overall half too (:1048-1058, :660/:667). So -7 still lands on 0 and 99
 *       on 4 with or without the facade's clamp ⇒ OutOfRangeWriteClamps stays
 *       GREEN under this mutation. The mutation IS caught, but by
 *       DelegateNeverFiresOnNoOp (test rows "an out-of-range write that clamps
 *       onto the CURRENT value must not broadcast"), where the facade's
 *       clamp-BEFORE-compare really is the only thing standing.
 *       ⚠️ Consequence for OutOfRangeWriteClamps' group half, recorded rather
 *       than quietly enjoyed: it is an ENGINE-BACKSTOPPED CONTRACT CHECK (board
 *       cl. 6 — an out-of-range write clamps rather than asserts), NOT evidence
 *       about this diff. ⛔ The one clamp with NO engine backstop is the
 *       resolution scale (Scalability.h:242 MinResolutionScale = 0.0f), and that
 *       one is load-bearing — see M6.
 *   M5  In GetResolutionScalePercent, wrap the return in
 *       FMath::Clamp(…, 50.f, 100.f) — i.e. clamp on READ, the TASK-1112 §1.3(b)
 *       trap exactly.                              ⇒ RED: SentinelIsNotClampedOnRead
 *   M6  In SetResolutionScaleNormalized, replace the clamp with the raw value.
 *                                                  ⇒ RED: ResolutionScaleClampsOnWrite
 *   M7  In RequestSaveSettings, delete the IsVideoModeChangePending() guard.
 *                                                  ⇒ RED: SaveUnreachableFromProvisionalVideoMode
 *   M8  🚨 In RevertVideoModeChange, delete the ApplyResolutionSettings(false)
 *       call (leaving the bare RevertVideoMode() the engine ships — the exact
 *       SC-§94 cl. A no-op this facade exists to not inherit).
 *                                                  ⇒ RED: RevertActuallyAppliesTheResolution
 *   M9  In SetFrameRateLimit, delete the `if (Rung <= 0.0f) continue;` guard so
 *       Unlimited snaps to the nearest numeric rung.
 *                                                  ⇒ RED: FrameRateLadderSnapsAndLabels
 *   M10 In ShouldEnableVolumetricFog, read GroupName_EffectsQuality instead of
 *       GroupName_ShadowQuality (i.e. follow GFX-§9's text over the engine's
 *       measured ownership).                       ⇒ RED: VolumetricFogFollowsShadowNotEffects
 *   M11 In GetFoliageQualityScale, change the Epic (case 3) return to 0.9f.
 *                                                  ⇒ RED: TierDFoliageDensityTable
 *   M12 In ResolveSettings, return GEngine->GetGameUserSettings() before checking
 *       bForceNullSettingsForAutomationTests.      ⇒ RED: NullSettingsDegradesToFallbacks
 *   M13 🚨 In AutoDetectQuality, delete the IsVideoModeChangePending() refusal
 *       (the block ABOVE the automation-suppression branch), leaving the bare
 *       RunHardwareBenchmark + ApplyHardwareBenchmarkResults pair — i.e. restore
 *       the state this row shipped in loop 0, where the engine's own
 *       SaveSettings() at GameUserSettings.cpp:1142 persists an unconfirmed
 *       ResolutionSizeX/Y + FullscreenMode entirely around RequestSaveSettings'
 *       guard.       ⇒ RED: AutoDetectRefusedDuringUnconfirmedVideoMode
 *       ✅⭐ CORRECTED 2026-09-07 FROM A **WITNESSED RED** — TASK-1117 EXECUTED
 *       THIS MUTATION AND IT REDDENED **EXACTLY ONE ASSERTION**: the REFUSAL
 *       COUNTER row, with the TestFalse above it GREEN. The claim struck from
 *       here said "the counter AND the return value"; qa/TASK-1114.md § LOOP 1
 *       WARN-8 derived that to be over-broad and TASK-1117's run then MEASURED
 *       it: under AUTOMATION the deleted guard falls into the SUPPRESSION
 *       branch, which returns false anyway, so the bool cannot discriminate.
 *       ⇒ ONE red row is the EXPECTED result here, not a half-failure — and TWO
 *       reds would mean the suppression branch MOVED.
 *       ⛔ It never reddens SaveSettingsCallCount: the engine's save is not at a
 *       counted facade call site, which is exactly why M1-M12 could not see this
 *       failure class at all (SC-§79). ⛔ This is the SECOND prediction in this
 *       lane measured narrower than written (M4 was the first) — a mutation table
 *       is evidence only where it has been RUN.
 *       ⚠️ A SECOND, WEAKER MUTATION WORTH THE HOST'S EYE: move the refusal
 *       BELOW the suppression branch instead of deleting it. The first TestFalse
 *       still passes (suppression returns false), and only the counter
 *       assertions catch it. That is the whole reason this test asserts the
 *       counter twice rather than the bool twice.
 *
 *   M14 In SetOverallScalabilityLevel, delete the ScaleBefore snapshot and the
 *       conditional BroadcastGraphicsSettingChanged(SettingName_ResolutionScale)
 *       tail, leaving only the OverallQuality broadcast — i.e. let one write move
 *       TWO player-visible values and announce ONE (TASK-1114 WARN-5).
 *                                                  ⇒ RED: CustomWhenGroupsDisagree
 *       ⚠️ A SECOND MUTATION FOR THE SAME LINES: make that second broadcast
 *       UNCONDITIONAL (drop the IsNearlyEqual compare).
 *                                                  ⇒ RED: CustomWhenGroupsDisagree
 *       ⛔ AND IT REDDENS ON A DIFFERENT ROW THAN THE ONE THAT LOOKS OBVIOUS.
 *       Re-pressing the SAME preset does NOT catch it — that returns early at the
 *       GetOverallScalabilityLevel() == ClampedLevel guard, before either
 *       broadcast, with or without the compare. The row that catches it is the
 *       REAL preset write whose SCALE DOES NOT MOVE: leaving Custom back to Epic
 *       when the scale is already at Epic's 100%. That is asserted explicitly.
 *       (Naming a mutation whose test is green either way is exactly the M4 error;
 *       this entry is written to not repeat it.)
 *
 *  ⛔ NO WITNESSED RED. None of M1-M14 has been executed: this row may not
 *  compile (the editor holds the DLL) and the suite is run by the host. Every
 *  mutation above is a DERIVED prediction from reading the code, not an observed
 *  transition — treat it as a claim to be checked, not as evidence.
 *  ⛔ M4's ORIGINAL PREDICTION WAS ONE OF THESE CLAIMS AND IT WAS FALSE (see M4).
 *  Eleven of the twelve reconcile against engine source; that one did not. Read
 *  the rest with the same suspicion.
 */

namespace SiegeGraphicsTestUtils
{
	/**
	 *  A graphics subsystem wired to a SCRATCH UGameUserSettings, plus the
	 *  throwaway UGameInstance it must live inside (Within = GameInstance).
	 */
	struct FScratchStore
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<UGameUserSettings> Settings;
		TStrongObjectPtr<USiegeGraphicsSettingsSubsystem> Graphics;

		bool IsValid() const { return GameInstance.IsValid() && Settings.IsValid() && Graphics.IsValid(); }
	};

	static FScratchStore MakeScratchStore()
	{
		FScratchStore Store;

		// GetTransientPackageAsObject() hands back a UObject* directly, so this
		// file needs no complete UPackage type.
		Store.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		Store.Settings.Reset(NewObject<UGameUserSettings>(GetTransientPackageAsObject()));

		// 🚨 A BARE NewObject<UGameUserSettings> IS ALREADY IN AN UNCONFIRMED VIDEO
		// MODE, AND THAT IS MEASURED, NOT DEFENSIVE. SetToDefaults() sets
		// FullscreenMode = GetDefaultWindowMode() == WindowedFullscreen (1) at
		// GameUserSettings.cpp:296/:831-834 but NEVER touches
		// LastConfirmedFullscreenMode, a plain int32 UPROPERTY defaulting to 0 ⇒
		// 1 != 0 on a brand-new object. Confirming here is REQUIRED, or every
		// save-refusal test below would be measuring this engine default instead
		// of the facade's guard.
		//
		// ⛔ SCOPE OF THAT CLAIM — CORRECTED BY TASK-1114 WARN-7. It is true of a
		// bare NewObject, which runs the constructor's SetToDefaults() and NEVER
		// LoadSettings(). It is ⛔ NOT true of a booted game: the engine heals the
		// zero-resolution first-launch case itself at GameUserSettings.cpp:628-632
		// (LoadSettings) and :470-480 (ValidateSettings), both during engine init.
		// ⇒ This ConfirmVideoMode() is a TEST-HARNESS necessity. It is not the
		// justification for Initialize()'s boot heal, which survives on the
		// abandoned/crashed-countdown case (non-zero staged resolution, disagreeing
		// confirmed triple) that neither engine heal looks at. Do not read one as
		// evidence for the other.
		if (Store.Settings.IsValid())
		{
			Store.Settings->ConfirmVideoMode();
		}

		if (Store.GameInstance.IsValid() && Store.Settings.IsValid())
		{
			Store.Graphics.Reset(NewObject<USiegeGraphicsSettingsSubsystem>(Store.GameInstance.Get()));
			if (Store.Graphics.IsValid())
			{
				// BEFORE anything reads or writes. This is what keeps the machine's
				// real GameUserSettings.ini — and the live editor's resolution —
				// untouched, and it suppresses the engine applies in the same call.
				Store.Graphics->SetGameUserSettingsForAutomationTests(Store.Settings.Get());
				Store.Graphics->ResetDiagnosticCountersForAutomationTests();
			}
		}

		return Store;
	}

	/** Puts every visible group at one level without going through the facade, so a test starts from a known, uniform state. */
	static void ForceUniformLevel(UGameUserSettings* Settings, int32 Level)
	{
		if (Settings)
		{
			Settings->SetOverallScalabilityLevel(Level);
		}
	}
}

/**
 *  ⛔ THE CANONICAL GROUP-NAME PIN (GFX-§10). A drifted name here silently
 *  disconnects a slider from the setting it claims to drive, with no error
 *  anywhere — the delegate payload, the ini section and the widget name simply
 *  stop referring to the same thing. Asserted character-for-character rather
 *  than trusted to review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsCanonicalNamesTest,
	"Siegebound.Graphics.CanonicalNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsCanonicalNamesTest::RunTest(const FString& Parameters)
{
	const TArray<FName> Names = USiegeGraphicsSettingsSubsystem::GetQualityGroupNames();

	// TEN, NOT ELEVEN. TASK-1112 measured eleven int32 groups on the engine;
	// LandscapeQuality is dropped because BaseScalability.ini has no
	// [LandscapeQuality@N] section at any level, [ScalabilitySettings] has no
	// PerfIndexThresholds_LandscapeQuality, and this project has no Landscape.
	TestEqual(TEXT("Exactly TEN quality groups ship (the engine's eleventh, LandscapeQuality, is deliberately dropped)"),
		Names.Num(), 10);

	// The pin, in GFX-§8 Tier-B display order.
	const TArray<FString> Expected = {
		TEXT("ViewDistanceQuality"),
		TEXT("AntiAliasingQuality"),
		TEXT("ShadowQuality"),
		TEXT("GlobalIlluminationQuality"),
		TEXT("ReflectionQuality"),
		TEXT("PostProcessQuality"),
		TEXT("TextureQuality"),
		TEXT("EffectsQuality"),
		TEXT("FoliageQuality"),
		TEXT("ShadingQuality")
	};

	if (Names.Num() == Expected.Num())
	{
		for (int32 Index = 0; Index < Names.Num(); ++Index)
		{
			TestEqualSensitive(*FString::Printf(TEXT("Group %d is exactly \"%s\""), Index, *Expected[Index]),
				Names[Index].ToString(), Expected[Index]);
		}
	}

	// ⛔ THE TWO SPELLING TRAPS, NAMED. The engine's own wrappers are
	// SetVisualEffectQuality and SetPostProcessingQuality; the ini, the sg.* CVar
	// and the FQualityLevels member are all spelled the OTHER way. This facade
	// canonicalises on the ini spelling, so these two strings must NOT appear.
	TestFalse(TEXT("\"VisualEffectQuality\" is NOT a canonical group name (the engine wrapper spelling stays out of the pin)"),
		Names.Contains(FName(TEXT("VisualEffectQuality"))));
	TestFalse(TEXT("\"PostProcessingQuality\" is NOT a canonical group name (likewise)"),
		Names.Contains(FName(TEXT("PostProcessingQuality"))));
	TestFalse(TEXT("\"LandscapeQuality\" is NOT shipped — it has no ini section and no Landscape behind it"),
		Names.Contains(FName(TEXT("LandscapeQuality"))));

	// A duplicate would make one slider silently drive another.
	TSet<FName> Unique(Names);
	TestEqual(TEXT("No canonical group name is duplicated"), Unique.Num(), Names.Num());

	// The band, pinned: 5 detents over [0, 4] (GFX-§5), Custom == -1 (engine-native).
	TestEqual(TEXT("MinQualityLevel is 0"), USiegeGraphicsSettingsSubsystem::MinQualityLevel, 0);
	TestEqual(TEXT("MaxQualityLevel is 4 (the fifth rung is the ini's @Cine)"), USiegeGraphicsSettingsSubsystem::MaxQualityLevel, 4);
	TestEqual(TEXT("NumQualityLevels is 5 — GFX-§5's five detents"), USiegeGraphicsSettingsSubsystem::NumQualityLevels, 5);
	TestEqual(TEXT("CustomQualityLevel is -1 — the engine's own 'the groups disagree' value"),
		USiegeGraphicsSettingsSubsystem::CustomQualityLevel, -1);

	// Every group has a player-facing label, and none of them is just the raw name.
	for (const FName& GroupName : Names)
	{
		const FText Label = USiegeGraphicsSettingsSubsystem::GetQualityGroupDisplayName(GroupName);
		TestFalse(*FString::Printf(TEXT("'%s' has a player-facing label"), *GroupName.ToString()), Label.IsEmpty());
	}

	// Level names come from the engine (Scalability.h:251), so assert they exist
	// rather than hard-coding localised strings that would drift.
	for (int32 Level = 0; Level <= 4; ++Level)
	{
		TestFalse(*FString::Printf(TEXT("Level %d has a display name"), Level),
			USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(Level).IsEmpty());
	}
	TestEqualSensitive(TEXT("Level -1 displays as \"Custom\""),
		USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(-1).ToString(), FString(TEXT("Custom")));

	// Hermeticity, made mechanical.
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (Store.IsValid() && GEngine)
	{
		TestTrue(TEXT("The scratch UGameUserSettings is NOT the engine's live one"),
			Store.Settings.Get() != GEngine->GetGameUserSettings());
	}

	return true;
}

/**
 *  EVERY GROUP ROUND-TRIPS 0 -> 4, AND THE NAMED GETTER AGREES WITH THE GENERIC
 *  ONE. The second half is the wiring guard: a group whose setter branch points
 *  at the wrong engine function still round-trips through the generic API (it
 *  writes and reads the same wrong field), so the two views have to be compared.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsGroupRoundTripTest,
	"Siegebound.Graphics.GroupRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsGroupRoundTripTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		for (int32 Level = 0; Level <= 4; ++Level)
		{
			Graphics->SetQualityGroupLevel(GroupName, Level);
			TestEqual(*FString::Printf(TEXT("'%s' round-trips level %d"), *GroupName.ToString(), Level),
				Graphics->GetQualityGroupLevel(GroupName), Level);
		}
	}

	// ⛔ THE WIRING GUARD. Each named getter must report the SAME value the
	// generic FName lookup does — a swapped branch shows up here and nowhere else.
	SiegeGraphicsTestUtils::ForceUniformLevel(Store.Settings.Get(), 3);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 0);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality, 1);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_EffectsQuality, 2);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_PostProcessQuality, 4);

	TestEqual(TEXT("GetShadowQuality agrees with the generic lookup"),
		Graphics->GetShadowQuality(), 0);
	TestEqual(TEXT("GetFoliageQuality agrees with the generic lookup"),
		Graphics->GetFoliageQuality(), 1);
	TestEqual(TEXT("GetEffectsQuality agrees with the generic lookup (canonical name -> engine's VisualEffect wrapper)"),
		Graphics->GetEffectsQuality(), 2);
	TestEqual(TEXT("GetPostProcessQuality agrees with the generic lookup (canonical name -> engine's PostProcessing wrapper)"),
		Graphics->GetPostProcessQuality(), 4);

	// And the untouched groups did not move — a setter that writes the wrong
	// field would show up as a neighbour changing.
	TestEqual(TEXT("ViewDistance was not disturbed by the four writes above"), Graphics->GetViewDistanceQuality(), 3);
	TestEqual(TEXT("Shading was not disturbed by the four writes above"), Graphics->GetShadingQuality(), 3);
	TestEqual(TEXT("Textures were not disturbed by the four writes above"), Graphics->GetTextureQuality(), 3);

	// The named setters drive the same path.
	Graphics->SetTextureQuality(1);
	TestEqual(TEXT("SetTextureQuality writes the TextureQuality group"),
		Graphics->GetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_TextureQuality), 1);

	return true;
}

/**
 *  AN OUT-OF-RANGE WRITE CLAMPS RATHER THAN ASSERTS (TASK-1113 cl. 6). A slider
 *  one float-rounding step out of band must land on the nearest legal detent,
 *  not take the game down.
 *
 *  ⛔ HONEST LABEL (TASK-1114 WARN-2): FOR THE TEN GROUPS AND THE OVERALL LEVEL
 *  THIS IS AN ENGINE-BACKSTOPPED CONTRACT CHECK, NOT EVIDENCE ABOUT THIS FACADE.
 *  Every band it asserts is enforced one layer down by the engine —
 *  FQualityLevels::Set<Group>Quality is FMath::Clamp(Value, 0, NumLevels-1)
 *  (Scalability.cpp:1117-1160) and SetFromSingleQualityLevel clamps both the
 *  groups (:1048-1058) and the render scale (:660/:667). ⇒ It stays GREEN with
 *  AND without the facade's own clamps, so deleting them would not redden it
 *  (that is M4, and M4's original prediction that it WOULD was wrong). What this
 *  test proves is that the CONTRACT holds — a player cannot crash the game with
 *  an out-of-band write — which is the board clause it answers, and it asserts
 *  the ENGINE for that, not us.
 *  ⚠️ The facade's group clamp is therefore belt-and-braces. It is still worth
 *  keeping: it is what makes the compare-before-write in
 *  ApplyQualityGroupLevelInternal compare the value that will ACTUALLY be
 *  stored, and DelegateNeverFiresOnNoOp is where that is load-bearing.
 *  ⛔ THE ONE CLAMP WITH NO ENGINE BACKSTOP IS THE RESOLUTION SCALE
 *  (Scalability.h:242 MinResolutionScale = 0.0f, so the engine really would let
 *  a 0% render through) — asserted by ResolutionScaleClampsOnWrite / M6, which
 *  IS evidence about this diff.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsOutOfRangeWriteClampsTest,
	"Siegebound.Graphics.OutOfRangeWriteClamps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsOutOfRangeWriteClampsTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		Graphics->SetQualityGroupLevel(GroupName, -7);
		TestEqual(*FString::Printf(TEXT("'%s' clamps -7 to 0"), *GroupName.ToString()),
			Graphics->GetQualityGroupLevel(GroupName), 0);

		Graphics->SetQualityGroupLevel(GroupName, 99);
		TestEqual(*FString::Printf(TEXT("'%s' clamps 99 to 4"), *GroupName.ToString()),
			Graphics->GetQualityGroupLevel(GroupName), 4);
	}

	// The overall preset clamps on the same band.
	Graphics->SetOverallScalabilityLevel(-4);
	TestEqual(TEXT("Overall clamps -4 to 0"), Graphics->GetOverallScalabilityLevel(), 0);
	Graphics->SetOverallScalabilityLevel(12);
	TestEqual(TEXT("Overall clamps 12 to 4"), Graphics->GetOverallScalabilityLevel(), 4);

	// An unknown group name is refused, not written, and does not corrupt a
	// neighbour or crash.
	const int32 ShadowBefore = Graphics->GetShadowQuality();
	Graphics->SetQualityGroupLevel(FName(TEXT("LandscapeQuality")), 0);
	TestEqual(TEXT("A write to the dropped LandscapeQuality group changes nothing"),
		Graphics->GetShadowQuality(), ShadowBefore);

	return true;
}

/**
 *  THE DELEGATE LAW: fires on a REAL change, ⛔ never on a no-op. Observed through
 *  the broadcast counter rather than a bound UFUNCTION — a dynamic multicast
 *  needs one and a UCLASS cannot be declared in a test .cpp.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsDelegateNeverFiresOnNoOpTest,
	"Siegebound.Graphics.DelegateNeverFiresOnNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsDelegateNeverFiresOnNoOpTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	SiegeGraphicsTestUtils::ForceUniformLevel(Store.Settings.Get(), 3);
	Graphics->ResetDiagnosticCountersForAutomationTests();

	// A REAL change broadcasts exactly once, carrying the canonical group name.
	Graphics->SetShadowQuality(1);
	TestEqual(TEXT("A real change broadcasts exactly once"), Graphics->GraphicsSettingsChangeBroadcastCount, 1);
	TestEqualSensitive(TEXT("The payload is the canonical group name"),
		Graphics->LastBroadcastSettingName.ToString(), FString(TEXT("ShadowQuality")));

	// A SAME-VALUE write is a complete no-op.
	Graphics->SetShadowQuality(1);
	TestEqual(TEXT("A same-value write does NOT broadcast"), Graphics->GraphicsSettingsChangeBroadcastCount, 1);

	// ⛔ AND THE SUBTLE ONE, WHICH ONLY WORKS IF THE CLAMP LANDS ON THE VALUE WE
	// ARE ALREADY AT. The clamp happens BEFORE the compare, so a slider pinned at
	// Low that keeps being asked for -3 stays silent instead of broadcasting on
	// every frame of a drag. (Asking for -3 while sitting at 1 is a REAL change to
	// 0 and must broadcast — both directions are asserted.)
	Graphics->SetShadowQuality(-3);
	TestEqual(TEXT("An out-of-range write that clamps onto a DIFFERENT value is a real change and DOES broadcast"),
		Graphics->GraphicsSettingsChangeBroadcastCount, 2);
	TestEqual(TEXT("...landing on the clamped 0"), Graphics->GetShadowQuality(), 0);

	Graphics->SetShadowQuality(-99);
	TestEqual(TEXT("⛔ An out-of-range write that clamps ONTO the current value does NOT broadcast"),
		Graphics->GraphicsSettingsChangeBroadcastCount, 2);

	Graphics->SetShadowQuality(1000);
	TestEqual(TEXT("A high out-of-range write clamps to 4 and DOES broadcast"),
		Graphics->GraphicsSettingsChangeBroadcastCount, 3);
	TestEqual(TEXT("...landing on the clamped 4"), Graphics->GetShadowQuality(), 4);

	Graphics->SetShadowQuality(1000);
	TestEqual(TEXT("⛔ Repeating the high out-of-range write does NOT broadcast"),
		Graphics->GraphicsSettingsChangeBroadcastCount, 3);

	// The non-group controls obey the same law.
	Graphics->SetVSyncEnabled(Graphics->IsVSyncEnabled());
	TestEqual(TEXT("A same-value VSync write does NOT broadcast"), Graphics->GraphicsSettingsChangeBroadcastCount, 3);
	Graphics->SetVSyncEnabled(!Graphics->IsVSyncEnabled());
	TestEqual(TEXT("A real VSync change DOES broadcast"), Graphics->GraphicsSettingsChangeBroadcastCount, 4);

	// ⚠️ SEED A KNOWN LADDER RUNG FIRST (TASK-1114 NIT-3(b)). Re-writing
	// GetFrameRateLimit() is a no-op ONLY if the inherited value is already on the
	// GFX-§5 ladder. A scratch object can carry this machine's config-backed CDO
	// values, so an off-ladder limit (100, say) would SNAP to 90 — a REAL change
	// that broadcasts, reddening this row on a differently-configured host and
	// looking exactly like a facade defect. Seed, then re-write the seeded value.
	Graphics->SetFrameRateLimit(60.0f);
	const int32 BroadcastsAfterSeed = Graphics->GraphicsSettingsChangeBroadcastCount;
	Graphics->SetFrameRateLimit(60.0f);
	TestEqual(TEXT("A same-value frame-limit write does NOT broadcast"),
		Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsAfterSeed);

	// ⚠️ Guarded: the overall level is Custom (-1) here, because ShadowQuality was
	// moved away from the rest above. A same-value write is only meaningful when
	// there IS a uniform value to re-write.
	const int32 OverallBefore = Graphics->GetOverallScalabilityLevel();
	if (OverallBefore >= 0)
	{
		Graphics->SetOverallScalabilityLevel(OverallBefore);
		TestEqual(TEXT("A same-value overall write does NOT broadcast"),
			Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsAfterSeed);
	}

	return true;
}

/**
 *  GFX-§5's CUSTOM STATE, and it is ENGINE-NATIVE (-1) rather than a parallel
 *  flag that could drift. Also checks the ten-group view, which is the one the
 *  player can actually see the inputs to.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsCustomWhenGroupsDisagreeTest,
	"Siegebound.Graphics.CustomWhenGroupsDisagree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsCustomWhenGroupsDisagreeTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	Graphics->SetOverallScalabilityLevel(3);
	TestEqual(TEXT("A uniform preset reports its level, not Custom"), Graphics->GetOverallScalabilityLevel(), 3);
	TestFalse(TEXT("A uniform preset is not Custom"), Graphics->IsOverallQualityCustom());
	TestFalse(TEXT("A uniform preset is not Custom across the visible ten either"),
		Graphics->IsOverallQualityCustomAcrossVisibleGroups());

	// One group disagrees ⇒ Custom, in both views.
	Graphics->SetShadowQuality(1);
	TestEqual(TEXT("Disagreeing groups report CustomQualityLevel (-1)"),
		Graphics->GetOverallScalabilityLevel(), USiegeGraphicsSettingsSubsystem::CustomQualityLevel);
	TestTrue(TEXT("IsOverallQualityCustom is true when the groups disagree"), Graphics->IsOverallQualityCustom());
	TestTrue(TEXT("IsOverallQualityCustomAcrossVisibleGroups is true when a VISIBLE group disagrees"),
		Graphics->IsOverallQualityCustomAcrossVisibleGroups());

	// ⚠️ AN ENGINE-NATIVE SURPRISE, PINNED SO NOBODY LATER "FIXES" IT AS A BUG.
	// FQualityLevels::GetSingleQualityLevel (Scalability.cpp:1083-1097) requires
	// ResolutionQuality to match the preset's canonical render scale as well as
	// all ELEVEN groups agreeing ⇒ dragging the Resolution Scale slider ALONE puts
	// the overall preset into Custom while every group slider still agrees. That
	// is defensible (the preset really is no longer stock Epic), but the panel
	// must not look broken, so both views are asserted here and TASK-1115 can
	// choose which one it labels with.
	// ⛔ FIRST, THE CASE WHERE THE SCALE DOES *NOT* MOVE. The scale is already at
	// Epic's 100% (the preset at the top of this test wrote it), so leaving Custom
	// back to Epic is a REAL preset write whose resolution scale is unchanged ⇒
	// EXACTLY ONE broadcast. This is the row that catches a WARN-5 announcement
	// made unconditional; re-pressing the same preset would NOT catch it, because
	// that returns early before either broadcast (M14).
	const int32 BroadcastsBeforeSameScalePreset = Graphics->GraphicsSettingsChangeBroadcastCount;
	const float ScaleBeforeSameScalePreset = Graphics->GetResolutionScaleNormalized();
	Graphics->SetOverallScalabilityLevel(3);
	TestTrue(TEXT("Leaving Custom back to Epic did NOT move the resolution scale"),
		FMath::IsNearlyEqual(ScaleBeforeSameScalePreset, Graphics->GetResolutionScaleNormalized(), UE_KINDA_SMALL_NUMBER));
	TestEqual(TEXT("⛔ A preset write whose scale does not move broadcasts ONCE, not twice"),
		Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsBeforeSameScalePreset + 1);

	Graphics->SetResolutionScalePercent(77.0f);
	TestTrue(TEXT("Resolution scale alone puts the OVERALL preset into Custom"),
		Graphics->IsOverallQualityCustom());
	TestFalse(TEXT("...while the visible-ten-group view still reports agreement"),
		Graphics->IsOverallQualityCustomAcrossVisibleGroups());

	// ⛔⛔ A PRESET PRESS MOVES THE RESOLUTION SCALE TOO, AND MUST SAY SO
	// (TASK-1114 WARN-5). The scale is at 77% here; pressing High (2) makes the
	// engine write ResolutionQuality from PerfIndexValues_ResolutionQuality
	// = 50 71 87 100 100 (Scalability.cpp:1047) ⇒ 87%. ONE write, TWO player-visible
	// values ⇒ TWO broadcasts, and a widget row bound to ResolutionScale that is
	// never told would display 77 forever.
	const float ScaleBeforePreset = Graphics->GetResolutionScaleNormalized();
	const int32 BroadcastsBeforePreset = Graphics->GraphicsSettingsChangeBroadcastCount;

	// Pressing a preset takes it back out of Custom.
	Graphics->SetOverallScalabilityLevel(2);
	TestFalse(TEXT("Pressing a preset leaves Custom"), Graphics->IsOverallQualityCustom());
	TestFalse(TEXT("Pressing a preset leaves Custom in the visible-ten view too"),
		Graphics->IsOverallQualityCustomAcrossVisibleGroups());

	// The engine really did overwrite the player's scale — asserted, so this is a
	// MEASURED consequence rather than a comment about one.
	TestTrue(TEXT("⛔ The preset press OVERWROTE the resolution scale the player set"),
		!FMath::IsNearlyEqual(ScaleBeforePreset, Graphics->GetResolutionScaleNormalized(), UE_KINDA_SMALL_NUMBER));
	TestEqual(TEXT("⛔ ...and it broadcast TWICE — OverallQuality AND ResolutionScale"),
		Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsBeforePreset + 2);
	TestEqual(TEXT("⛔ ...with the SECOND broadcast naming ResolutionScale, so the widget row knows"),
		Graphics->LastBroadcastSettingName, USiegeGraphicsSettingsSubsystem::SettingName_ResolutionScale);

	// ⚠️ AND THE NO-OP LAW STILL BINDS THE SECOND BROADCAST. Re-pressing the SAME
	// preset returns early before either broadcast — the extra announcement must not
	// become an unconditional one.
	const int32 BroadcastsAfterPreset = Graphics->GraphicsSettingsChangeBroadcastCount;
	Graphics->SetOverallScalabilityLevel(2);
	TestEqual(TEXT("Re-pressing the same preset broadcasts NOTHING — neither name"),
		Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsAfterPreset);

	return true;
}

/**
 *  A NULL UGameUserSettings DEGRADES TO THE DOCUMENTED FALLBACKS AND ⛔ NEVER
 *  CRASHES. This is the one path that cannot be reached any other way, and the
 *  one where a missing null check is a crash rather than a wrong pixel.
 *
 *  ⚠️ THE FALLBACKS PRESERVE THE AUTHORED BASELINE (Epic / full density / fog
 *  on) on purpose — a failed lookup must never silently downgrade the game.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsNullSettingsDegradesTest,
	"Siegebound.Graphics.NullSettingsDegradesToFallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsNullSettingsDegradesTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	// Drop the override, then force the null path.
	Graphics->SetGameUserSettingsForAutomationTests(nullptr);
	Graphics->SetForceNullGameUserSettingsForAutomationTests(true);
	Graphics->ResetDiagnosticCountersForAutomationTests();

	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		TestEqual(*FString::Printf(TEXT("'%s' falls back to Epic (3)"), *GroupName.ToString()),
			Graphics->GetQualityGroupLevel(GroupName), USiegeGraphicsSettingsSubsystem::FallbackQualityLevel);
	}

	TestEqual(TEXT("Overall falls back to Epic (3)"), Graphics->GetOverallScalabilityLevel(), 3);
	TestFalse(TEXT("Custom is false on the fallback path"), Graphics->IsOverallQualityCustom());
	TestFalse(TEXT("Custom-across-visible is false on the fallback path"), Graphics->IsOverallQualityCustomAcrossVisibleGroups());

	TestEqual(TEXT("Resolution scale falls back to 1.0 normalized"), Graphics->GetResolutionScaleNormalized(), 1.0f, 0.001f);
	TestEqual(TEXT("Resolution scale falls back to 100%"), Graphics->GetResolutionScalePercent(), 100.0f, 0.01f);
	TestFalse(TEXT("The sentinel is NOT reported on the fallback path"), Graphics->IsResolutionScaleProjectDefault());
	TestEqual(TEXT("The slider seed falls back to 100%"), Graphics->GetResolutionScalePercentForSlider(), 100.0f, 0.01f);

	TestEqual(TEXT("Screen resolution falls back to 1280x720"),
		Graphics->GetScreenResolution().ToString(), FIntPoint(1280, 720).ToString());
	TestEqual(TEXT("Window mode falls back to 1 (WindowedFullscreen)"), Graphics->GetWindowMode(), 1);
	TestFalse(TEXT("VSync falls back to false"), Graphics->IsVSyncEnabled());
	TestEqual(TEXT("Frame rate limit falls back to 0 (Unlimited)"), Graphics->GetFrameRateLimit(), 0.0f, 0.01f);

	// Tier D degrades toward the AUTHORED baseline, never toward less content.
	TestEqual(TEXT("Foliage density falls back to 1.0 (full density)"), Graphics->GetFoliageQualityScale(), 1.0f, 0.001f);
	TestEqual(TEXT("View distance scale falls back to 1.0"), Graphics->GetViewDistanceScale(), 1.0f, 0.001f);
	TestTrue(TEXT("Volumetric fog falls back to ON (the shipped look)"), Graphics->ShouldEnableVolumetricFog());

	// Never pending, and the stepper list is never empty.
	TestFalse(TEXT("No video-mode change is pending on the fallback path"), Graphics->IsVideoModeChangePending());
	TestTrue(TEXT("The supported-resolution list is never empty"), Graphics->GetSupportedScreenResolutionCount() >= 1);

	// Every mutation is a refused no-op that returns false and broadcasts nothing.
	Graphics->SetShadowQuality(0);
	Graphics->SetOverallScalabilityLevel(0);
	Graphics->SetResolutionScaleNormalized(0.5f);
	Graphics->SetScreenResolution(FIntPoint(640, 480));
	Graphics->SetWindowMode(2);
	Graphics->SetVSyncEnabled(true);
	Graphics->SetFrameRateLimit(60.0f);

	TestFalse(TEXT("ApplyQualitySettings returns false when unresolvable"), Graphics->ApplyQualitySettings());
	TestFalse(TEXT("ApplyVideoModeProvisional returns false when unresolvable"), Graphics->ApplyVideoModeProvisional());
	TestFalse(TEXT("ConfirmVideoModeChange returns false when unresolvable"), Graphics->ConfirmVideoModeChange());
	TestFalse(TEXT("RevertVideoModeChange returns false when unresolvable"), Graphics->RevertVideoModeChange());
	TestFalse(TEXT("AutoDetectQuality returns false when unresolvable"), Graphics->AutoDetectQuality());

	TestEqual(TEXT("NOTHING broadcast on the fallback path"), Graphics->GraphicsSettingsChangeBroadcastCount, 0);
	TestEqual(TEXT("NOTHING applied on the fallback path"), Graphics->ApplyNonResolutionSettingsCallCount, 0);
	TestEqual(TEXT("NOTHING saved on the fallback path"), Graphics->SaveSettingsCallCount, 0);

	Graphics->SetForceNullGameUserSettingsForAutomationTests(false);
	return true;
}

/**
 *  🚨 THE sg.ResolutionQuality=0 SENTINEL IS NOT CLAMPED ON READ.
 *
 *  BaseScalability.ini:39-42 defines 0 as "use the project's default screen
 *  percentage", NOT "render at 0%", and this machine's live ini is in exactly
 *  that state right now (TASK-1112 §1.6). A clamp on READ would rewrite it to a
 *  hard 50% the first time a player OPENED the graphics panel — changing their
 *  picture as a side effect of looking at the menu, with nothing in any log.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsSentinelNotClampedOnReadTest,
	"Siegebound.Graphics.SentinelIsNotClampedOnRead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsSentinelNotClampedOnReadTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	// Reproduce the loaded-ini state DIRECTLY on the engine object, bypassing the
	// facade — that is what a load does, and the facade can never write it.
	Store.Settings->SetResolutionScaleValueEx(0.0f);
	Graphics->ResetDiagnosticCountersForAutomationTests();

	TestTrue(TEXT("The sentinel is REPORTED as the project default, not as a percentage"),
		Graphics->IsResolutionScaleProjectDefault());
	TestEqual(TEXT("The RAW read is 0 — ⛔ not clamped up to 50"),
		Graphics->GetResolutionScalePercent(), 0.0f, 0.01f);
	TestEqual(TEXT("The RAW normalized read is 0 — ⛔ not clamped up to 0.5"),
		Graphics->GetResolutionScaleNormalized(), 0.0f, 0.001f);

	// The slider shows something legible without the read having written anything.
	TestEqual(TEXT("A slider seeded from the sentinel shows 100%, not 0% and not 50%"),
		Graphics->GetResolutionScalePercentForSlider(),
		USiegeGraphicsSettingsSubsystem::ResolutionScaleSentinelDisplayPercent, 0.01f);

	// ⛔ THE POINT OF THE WHOLE TEST: reading did not write.
	TestTrue(TEXT("After all those reads the sentinel is STILL live — opening the menu changed nothing"),
		Graphics->IsResolutionScaleProjectDefault());
	TestEqual(TEXT("No apply was triggered by reading"), Graphics->ApplyNonResolutionSettingsCallCount, 0);
	TestEqual(TEXT("No save was triggered by reading"), Graphics->SaveSettingsCallCount, 0);
	TestEqual(TEXT("No broadcast was triggered by reading"), Graphics->GraphicsSettingsChangeBroadcastCount, 0);

	// And once the player DOES move the slider, the sentinel is gone for good.
	Graphics->SetResolutionScalePercent(75.0f);
	TestFalse(TEXT("A real player write clears the sentinel"), Graphics->IsResolutionScaleProjectDefault());
	TestEqual(TEXT("...and lands on the requested 75%"), Graphics->GetResolutionScalePercent(), 75.0f, 0.5f);

	return true;
}

/**
 *  THE RESOLUTION SCALE CLAMPS ON WRITE — GFX-§5's 50-100% band is load-bearing,
 *  not cosmetic: the engine floor is 0.0 (Scalability.h:242), so an unclamped
 *  SetResolutionScaleNormalized(0) is a literal request for a 0% render.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsResolutionScaleClampsOnWriteTest,
	"Siegebound.Graphics.ResolutionScaleClampsOnWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsResolutionScaleClampsOnWriteTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	Graphics->SetResolutionScaleNormalized(0.0f);
	TestEqual(TEXT("A write of 0.0 clamps to the 50% floor — ⛔ never a 0% render"),
		Graphics->GetResolutionScalePercent(), 50.0f, 0.5f);
	TestFalse(TEXT("A clamped write does NOT recreate the project-default sentinel"),
		Graphics->IsResolutionScaleProjectDefault());

	Graphics->SetResolutionScaleNormalized(-3.0f);
	TestEqual(TEXT("A negative write clamps to 50%"), Graphics->GetResolutionScalePercent(), 50.0f, 0.5f);

	Graphics->SetResolutionScaleNormalized(5.0f);
	TestEqual(TEXT("A write above 1.0 clamps to 100%"), Graphics->GetResolutionScalePercent(), 100.0f, 0.5f);

	Graphics->SetResolutionScalePercent(73.0f);
	TestEqual(TEXT("An in-band percent write is exact (normalized == percent/100)"),
		Graphics->GetResolutionScalePercent(), 73.0f, 0.5f);
	TestEqual(TEXT("...and the normalized view agrees"),
		Graphics->GetResolutionScaleNormalized(), 0.73f, 0.005f);

	return true;
}

/**
 *  🚨 GFX-§4: SaveSettings IS UNREACHABLE FROM THE PROVISIONAL VIDEO-MODE PATH.
 *
 *  Saving an unconfirmed mode persists a screen the player may not be able to
 *  read, across restarts, with no in-game route back — and the likeliest way in
 *  is NOT the resolution control but an innocent quality-slider drag while the
 *  confirmation countdown is still on screen. That case is asserted explicitly.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsSaveUnreachableFromProvisionalTest,
	"Siegebound.Graphics.SaveUnreachableFromProvisionalVideoMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsSaveUnreachableFromProvisionalTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	// A fresh UGameUserSettings has ResolutionSizeX/Y == LastUserConfirmed*, so
	// nothing is pending before we stage anything.
	const FIntPoint Original = Graphics->GetScreenResolution();
	Graphics->ResetDiagnosticCountersForAutomationTests();
	TestFalse(TEXT("Nothing is pending on a fresh settings object"), Graphics->IsVideoModeChangePending());

	// STAGE a different mode. ⛔ Staging applies nothing and saves nothing.
	const FIntPoint Staged(Original.X + 16, Original.Y + 16);
	Graphics->SetScreenResolution(Staged);
	TestEqual(TEXT("Staging wrote the new resolution"),
		Graphics->GetScreenResolution().ToString(), Staged.ToString());
	TestTrue(TEXT("Staging alone makes a video-mode change PENDING"), Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("Staging did not apply"), Graphics->ApplyResolutionSettingsCallCount, 0);
	TestEqual(TEXT("Staging did not save"), Graphics->SaveSettingsCallCount, 0);

	// PROVISIONAL APPLY: applies, ⛔ does not save.
	TestTrue(TEXT("ApplyVideoModeProvisional succeeds"), Graphics->ApplyVideoModeProvisional());
	TestEqual(TEXT("The provisional apply reached ApplyResolutionSettings exactly once"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestEqual(TEXT("⛔ The provisional apply reached SaveSettings ZERO times"),
		Graphics->SaveSettingsCallCount, 0);
	TestTrue(TEXT("A confirmation is still outstanding"), Graphics->IsVideoModeChangePending());

	// ⛔ THE REAL TRAP: a quality change DURING the confirmation window must apply
	// but must NOT save, because SaveSettings would write ResolutionSizeX/Y too.
	Graphics->SetShadowQuality(Graphics->GetShadowQuality() == 0 ? 2 : 0);
	TestEqual(TEXT("The quality change DID apply during the confirmation window"),
		Graphics->ApplyNonResolutionSettingsCallCount, 1);
	TestEqual(TEXT("⛔ ...but it did NOT save — that would have persisted the unconfirmed mode"),
		Graphics->SaveSettingsCallCount, 0);
	TestTrue(TEXT("...and the refusal was counted rather than silent"),
		Graphics->RefusedSaveWhileVideoModePendingCount >= 1);

	// CONFIRM closes the window, releases the save, and flushes the deferred one.
	TestTrue(TEXT("ConfirmVideoModeChange succeeds"), Graphics->ConfirmVideoModeChange());
	TestFalse(TEXT("Nothing is pending after confirming"), Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("The save happens ONLY after the confirm"), Graphics->SaveSettingsCallCount, 1);
	TestEqual(TEXT("The confirmed mode is the staged one"),
		Graphics->GetScreenResolution().ToString(), Staged.ToString());

	return true;
}

/**
 *  🚨🚨 AUTO-DETECT IS REFUSED DURING AN UNCONFIRMED VIDEO MODE — the SECOND save
 *  route out of the provisional window, and the one that does not pass through this
 *  facade's guarded save site at all (TASK-1114 BLOCKER-1).
 *
 *  UGameUserSettings::ApplyHardwareBenchmarkResults' complete body is
 *  GameUserSettings.cpp:1132-1143 and it saves TWICE:
 *      :1139  Scalability::SaveState(GGameUserSettingsIni)   → [ScalabilityGroups]
 *      :1142  SaveSettings()                                 → SaveConfig(CPF_Config)
 *             at :683, which writes EVERY UPROPERTY(config) on the class —
 *             ResolutionSizeX/Y (GameUserSettings.h:465) and FullscreenMode (:500)
 *             among them.
 *  ⇒ Pressing Auto-Detect mid-countdown would persist a video mode the player may
 *  not be able to see, with RequestSaveSettings never consulted,
 *  SaveSettingsCallCount not moving and no log line to explain it. The original
 *  trace for this row read only as far as :1139 and concluded the opposite; that is
 *  why this test exists and why it asserts the REFUSAL rather than the save count
 *  (the save count would stay 0 either way — it is blind to this failure).
 *
 *  ⚠️ THE TWO FALSES ARE DISTINGUISHABLE BY THE COUNTER, and this test pins both:
 *  the GFX-§4 refusal INCREMENTS RefusedSaveWhileVideoModePendingCount; the
 *  automation-suppression early-out (which is why the benchmark never spins a GPU
 *  in a unit test) leaves it UNCHANGED. Without that discrimination the test would
 *  pass on the suppression branch alone and prove nothing — the guard sits ABOVE
 *  the suppression branch in AutoDetectQuality precisely so this is reachable.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsAutoDetectRefusedDuringPendingTest,
	"Siegebound.Graphics.AutoDetectRefusedDuringUnconfirmedVideoMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsAutoDetectRefusedDuringPendingTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	const FIntPoint Original = Graphics->GetScreenResolution();
	Graphics->ResetDiagnosticCountersForAutomationTests();
	TestFalse(TEXT("Nothing is pending on a fresh settings object"), Graphics->IsVideoModeChangePending());

	// Stage a different mode and apply it provisionally — the countdown is now live.
	Graphics->SetScreenResolution(FIntPoint(Original.X + 16, Original.Y + 16));
	TestTrue(TEXT("ApplyVideoModeProvisional succeeds"), Graphics->ApplyVideoModeProvisional());
	TestTrue(TEXT("A confirmation is outstanding"), Graphics->IsVideoModeChangePending());

	const int32 RefusalsBefore = Graphics->RefusedSaveWhileVideoModePendingCount;
	const int32 AppliesBefore = Graphics->ApplyNonResolutionSettingsCallCount;
	const int32 BroadcastsBefore = Graphics->GraphicsSettingsChangeBroadcastCount;

	// ⛔⛔ THE ASSERTION: Auto-Detect refuses, and it refuses for the GFX-§4 reason.
	TestFalse(TEXT("⛔ AutoDetectQuality is REFUSED while a video mode is unconfirmed"),
		Graphics->AutoDetectQuality());
	TestEqual(TEXT("⛔ ...and the refusal was COUNTED as a GFX-§4 save refusal, not silently suppressed"),
		Graphics->RefusedSaveWhileVideoModePendingCount, RefusalsBefore + 1);

	// The benchmark must not have run at all: nothing applied, nothing saved,
	// nothing broadcast. (SaveSettingsCallCount cannot rise here even on the
	// defective build — the engine's save is not at a counted facade call site —
	// so it is asserted for completeness, NOT as the discriminating instrument.)
	TestEqual(TEXT("The refused Auto-Detect applied nothing"),
		Graphics->ApplyNonResolutionSettingsCallCount, AppliesBefore);
	TestEqual(TEXT("The refused Auto-Detect broadcast nothing"),
		Graphics->GraphicsSettingsChangeBroadcastCount, BroadcastsBefore);
	TestEqual(TEXT("The refused Auto-Detect reached the facade's save site ZERO times"),
		Graphics->SaveSettingsCallCount, 0);
	TestTrue(TEXT("The staged mode is STILL unconfirmed — the refusal changed no state"),
		Graphics->IsVideoModeChangePending());

	// ⚠️ NOW CLOSE THE WINDOW. Auto-Detect still returns false under automation, but
	// for the OTHER reason — and the counter is what separates them. If this second
	// TestEqual ever fails, the GFX-§4 guard has fallen BELOW the suppression branch
	// and the assertion above is passing for the wrong reason.
	TestTrue(TEXT("ConfirmVideoModeChange succeeds"), Graphics->ConfirmVideoModeChange());
	TestFalse(TEXT("Nothing is pending after confirming"), Graphics->IsVideoModeChangePending());

	const int32 RefusalsAfterConfirm = Graphics->RefusedSaveWhileVideoModePendingCount;
	TestFalse(TEXT("AutoDetectQuality still returns false under automation suppression"),
		Graphics->AutoDetectQuality());
	TestEqual(TEXT("⛔ ...but WITHOUT counting a GFX-§4 refusal — the two falses are distinguishable"),
		Graphics->RefusedSaveWhileVideoModePendingCount, RefusalsAfterConfirm);

	return true;
}

/**
 *  🚨 THE REVERT ACTUALLY REVERTS — the SC-§94 cl. A guard.
 *
 *  UGameUserSettings::RevertVideoMode()'s COMPLETE body restores five member
 *  fields and broadcasts (GameUserSettings.cpp:276-285). It pushes NOTHING to the
 *  display. A facade that inherited that would leave the player staring at an
 *  unreadable screen while every property read-back reported the good mode — the
 *  instrument echoing the request instead of the result.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsRevertAppliesResolutionTest,
	"Siegebound.Graphics.RevertActuallyAppliesTheResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsRevertAppliesResolutionTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	const FIntPoint Original = Graphics->GetScreenResolution();
	const int32 OriginalWindowMode = Graphics->GetWindowMode();
	Graphics->ResetDiagnosticCountersForAutomationTests();

	Graphics->SetScreenResolution(FIntPoint(Original.X + 16, Original.Y + 16));
	TestTrue(TEXT("ApplyVideoModeProvisional succeeds"), Graphics->ApplyVideoModeProvisional());
	TestEqual(TEXT("One apply so far (the provisional one)"), Graphics->ApplyResolutionSettingsCallCount, 1);

	TestTrue(TEXT("RevertVideoModeChange succeeds"), Graphics->RevertVideoModeChange());

	// ⛔⛔ THE ASSERTION THIS WHOLE FILE EXISTS FOR: the revert path reached
	// ApplyResolutionSettings. A bare RevertVideoMode() would leave this at 1.
	TestEqual(TEXT("⛔ The REVERT path reached ApplyResolutionSettings — a bare RevertVideoMode() applies NOTHING"),
		Graphics->ApplyResolutionSettingsCallCount, 2);

	TestEqual(TEXT("The resolution is back to the last confirmed one"),
		Graphics->GetScreenResolution().ToString(), Original.ToString());
	TestEqual(TEXT("The window mode is back to the last confirmed one"), Graphics->GetWindowMode(), OriginalWindowMode);
	TestFalse(TEXT("Nothing is pending after reverting"), Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("The save is released by the revert too"), Graphics->SaveSettingsCallCount, 1);

	return true;
}

/**
 *  THE FRAME-RATE LADDER (GFX-§5): a STEPPER, because the spacing between 144 and
 *  165 is not the spacing between 30 and 60. ⛔ 0 means Unlimited and must never
 *  be snapped up to 30 — a control that silently capped an uncapped game would be
 *  the worst kind of lie in a performance menu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsFrameRateLadderTest,
	"Siegebound.Graphics.FrameRateLadderSnapsAndLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsFrameRateLadderTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	const TArray<float> Ladder = USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLadder();
	TestEqual(TEXT("The ladder has 8 rungs"), Ladder.Num(), 8);
	TestEqual(TEXT("The ladder starts at 30"), Ladder[0], 30.0f, 0.01f);
	TestEqual(TEXT("⛔ Unlimited (0) is the LAST rung, so a stepper walks 30 -> ... -> 240 -> Unlimited"),
		Ladder.Last(), 0.0f, 0.01f);
	TestEqual(TEXT("GetFrameRateLimitOptionCount agrees with the ladder"),
		USiegeGraphicsSettingsSubsystem::GetFrameRateLimitOptionCount(), Ladder.Num());

	TestEqualSensitive(TEXT("0 is labelled \"Unlimited\", never \"0 FPS\""),
		USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLabel(0.0f), FString(TEXT("Unlimited")));
	TestEqualSensitive(TEXT("144 is labelled \"144 FPS\""),
		USiegeGraphicsSettingsSubsystem::GetFrameRateLimitLabel(144.0f), FString(TEXT("144 FPS")));

	// Snapping.
	Graphics->SetFrameRateLimit(150.0f);
	TestEqual(TEXT("150 snaps to the nearest rung, 144"), Graphics->GetFrameRateLimit(), 144.0f, 0.5f);

	Graphics->SetFrameRateLimit(100000.0f);
	TestEqual(TEXT("A huge request snaps to the top numeric rung, 240"), Graphics->GetFrameRateLimit(), 240.0f, 0.5f);

	Graphics->SetFrameRateLimit(1.0f);
	TestEqual(TEXT("A tiny positive request snaps to 30, the bottom rung"), Graphics->GetFrameRateLimit(), 30.0f, 0.5f);

	// ⛔ THE ONE THAT MATTERS: Unlimited stays Unlimited.
	Graphics->SetFrameRateLimit(0.0f);
	TestEqual(TEXT("⛔ 0 (Unlimited) is preserved exactly — never snapped up to 30"),
		Graphics->GetFrameRateLimit(), 0.0f, 0.01f);
	TestEqual(TEXT("...and the stepper index points at the Unlimited rung"),
		Graphics->FindCurrentFrameRateLimitIndex(), Ladder.Num() - 1);

	Graphics->SetFrameRateLimit(-50.0f);
	TestEqual(TEXT("A negative request is treated as Unlimited, not as a limit"),
		Graphics->GetFrameRateLimit(), 0.0f, 0.01f);

	// Index round-trip across every rung.
	for (int32 Index = 0; Index < Ladder.Num(); ++Index)
	{
		Graphics->SetFrameRateLimitByIndex(Index);
		TestEqual(*FString::Printf(TEXT("Ladder index %d round-trips"), Index),
			Graphics->FindCurrentFrameRateLimitIndex(), Index);
	}

	// Out-of-range index is refused, not asserted.
	const float Before = Graphics->GetFrameRateLimit();
	Graphics->SetFrameRateLimitByIndex(99);
	TestEqual(TEXT("An out-of-range ladder index changes nothing"), Graphics->GetFrameRateLimit(), Before, 0.01f);

	return true;
}

/**
 *  THE WINDOW-MODE CONTRACT. Marshalled as int32 because GFX-§2(d) forbids an
 *  enum in a BlueprintImplementableEvent parameter, so the facade's own type must
 *  make that impossible downstream.
 *
 *  ⚠️ Deliberately exercises modes 1 and 2 and not 0: GetFullscreenMode() runs the
 *  value through GetPlatformFullscreenMode, which remaps everything to Fullscreen
 *  on platforms that do not support windowed mode (GameUserSettings.cpp:105-110).
 *  Windowed (2) is never remapped on a desktop target, so the round-trip is
 *  asserted where it is a statement about THIS facade rather than about the host.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsWindowModeContractTest,
	"Siegebound.Graphics.WindowModeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsWindowModeContractTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	TestEqual(TEXT("There are exactly 3 window modes"), USiegeGraphicsSettingsSubsystem::GetWindowModeCount(), 3);
	for (int32 Mode = 0; Mode < 3; ++Mode)
	{
		TestFalse(*FString::Printf(TEXT("Window mode %d has a label"), Mode),
			USiegeGraphicsSettingsSubsystem::GetWindowModeLabel(Mode).IsEmpty());
	}
	TestTrue(TEXT("An out-of-range window mode has no label rather than a wrong one"),
		USiegeGraphicsSettingsSubsystem::GetWindowModeLabel(3).IsEmpty());

	// ⚠️ ESTABLISH A KNOWN STARTING MODE (TASK-1114 NIT-3(a)). This scratch object
	// can carry the host's config-backed CDO values, so it may ALREADY be in
	// Windowed (2) — in which case SetWindowMode(2) is a no-op, nothing is staged,
	// and "A window-mode change makes a confirmation pending" goes red on a
	// differently-configured machine while the facade is perfectly correct.
	// Confirming after the seed makes the pending assertion below about the SECOND
	// write only.
	Graphics->SetWindowMode(1);
	Graphics->ConfirmVideoModeChange();
	TestFalse(TEXT("Nothing is pending after seeding a known window mode"), Graphics->IsVideoModeChangePending());

	Graphics->SetWindowMode(2);
	TestEqual(TEXT("Windowed (2) round-trips"), Graphics->GetWindowMode(), 2);
	TestTrue(TEXT("A window-mode change makes a confirmation pending"), Graphics->IsVideoModeChangePending());

	// Clamped, not asserted — and 2 is the clamp target, which is also the mode a
	// locked-out player can always see the menu in.
	Graphics->SetWindowMode(99);
	TestEqual(TEXT("An out-of-range window mode clamps into the band"), Graphics->GetWindowMode(), 2);

	Graphics->SetWindowMode(1);
	TestEqual(TEXT("WindowedFullscreen (1) round-trips"), Graphics->GetWindowMode(), 1);

	// The resolution stepper is never empty and its label/index API is coherent.
	const int32 Count = Graphics->GetSupportedScreenResolutionCount();
	TestTrue(TEXT("The supported-resolution list is never empty"), Count >= 1);
	TestTrue(TEXT("The current-resolution index is inside the list"),
		Graphics->FindCurrentScreenResolutionIndex() >= 0 && Graphics->FindCurrentScreenResolutionIndex() < Count);
	TestFalse(TEXT("Index 0 has a label"), Graphics->GetSupportedScreenResolutionLabel(0).IsEmpty());
	TestTrue(TEXT("An out-of-range index has no label rather than a wrong one"),
		Graphics->GetSupportedScreenResolutionLabel(Count + 5).IsEmpty());

	return true;
}

/**
 *  TIER D — THE PROJECT'S OWN LEVERS (TASK-1113 cl. 5, shipped HERE so TASK-1115
 *  and TASK-1122 never touch the same file).
 *
 *  These derivations are the ones most likely to be "tidied" later by someone who
 *  does not know what they were measured against, so each table is pinned.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsTierDDerivationsTest,
	"Siegebound.Graphics.TierDFoliageDensityTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsTierDDerivationsTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	// FOLIAGE DENSITY. Epic (3) is 1.0 because DA_BattlefieldScatter's AUTHORED
	// values ARE the Epic baseline (GFX-§9), and Cinematic never EXCEEDS it.
	const float ExpectedDensity[5] = { 0.25f, 0.50f, 0.75f, 1.00f, 1.00f };
	for (int32 Level = 0; Level <= 4; ++Level)
	{
		Graphics->SetFoliageQuality(Level);
		TestEqual(*FString::Printf(TEXT("Foliage density at level %d"), Level),
			Graphics->GetFoliageQualityScale(), ExpectedDensity[Level], 0.001f);
	}

	Graphics->SetFoliageQuality(3);
	TestEqual(TEXT("⛔ Epic is EXACTLY 1.0 — the authored baseline must be byte-for-byte unchanged"),
		Graphics->GetFoliageQualityScale(), 1.0f, 0.0001f);
	Graphics->SetFoliageQuality(4);
	TestTrue(TEXT("⛔ Cinematic never exceeds the authored baseline"),
		Graphics->GetFoliageQualityScale() <= 1.0f);

	// VIEW DISTANCE. 1.0 at every level in v1 — a MEASUREMENT, not a stub: the
	// engine's own r.ViewDistanceScale (0.4 at @0 -> 1.0 at @3) already moves the
	// HISM cull band, so a project-side multiplier would double-scale it.
	for (int32 Level = 0; Level <= 4; ++Level)
	{
		Graphics->SetViewDistanceQuality(Level);
		TestEqual(*FString::Printf(TEXT("Project-side view-distance scale is 1.0 at level %d (the engine's r.ViewDistanceScale owns this band)"), Level),
			Graphics->GetViewDistanceScale(), 1.0f, 0.0001f);
	}

	return true;
}

/**
 *  🚨 VOLUMETRIC FOG FOLLOWS THE SHADOW GROUP, NOT EFFECTS — and this test exists
 *  because GFX-§9's TEXT says Effects while the ENGINE says Shadow.
 *
 *  r.VolumetricFog is set in [ShadowQuality@N] (0 at @0/@1; 1 at @2/@3/@Cine with
 *  a graduated froxel grid). [EffectsQuality@*] never mentions it. Deriving from
 *  Effects would ship a live contradiction: a player at Shadows=Low +
 *  Effects=Epic gets r.VolumetricFog=0 from stock scalability while our named
 *  switch insists the fog is on — the fog GONE with the control saying otherwise,
 *  which is exactly the "control that lies" GFX-§9 was written to prevent.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsVolumetricFogGroupTest,
	"Siegebound.Graphics.VolumetricFogFollowsShadowNotEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsVolumetricFogGroupTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	// The engine's own boundary, rung by rung.
	const bool ExpectedFog[5] = { false, false, true, true, true };
	for (int32 Level = 0; Level <= 4; ++Level)
	{
		Graphics->SetShadowQuality(Level);
		const FString FogWhat = FString::Printf(
			TEXT("Volumetric fog at Shadows=%d matches the engine's own [ShadowQuality@%d] r.VolumetricFog"), Level, Level);
		if (ExpectedFog[Level])
		{
			TestTrue(*FogWhat, Graphics->ShouldEnableVolumetricFog());
		}
		else
		{
			TestFalse(*FogWhat, Graphics->ShouldEnableVolumetricFog());
		}
	}

	// ⛔ THE DISCRIMINATING PAIR. Both cases below have the SAME answer under a
	// Shadow derivation and OPPOSITE answers under an Effects derivation, so this
	// is what actually distinguishes the two wirings.
	Graphics->SetShadowQuality(0);
	Graphics->SetEffectsQuality(4);
	TestFalse(TEXT("⛔ Shadows=Low + Effects=Cinematic ⇒ fog OFF (the engine turns it off; an Effects-derived switch would say ON)"),
		Graphics->ShouldEnableVolumetricFog());

	Graphics->SetShadowQuality(3);
	Graphics->SetEffectsQuality(0);
	TestTrue(TEXT("⛔ Shadows=Epic + Effects=Low ⇒ fog ON (an Effects-derived switch would say OFF)"),
		Graphics->ShouldEnableVolumetricFog());

	return true;
}

/**
 *  TIER D IS PURE AND SIDE-EFFECT-FREE (TASK-1113 cl. 5, and a QA criterion in
 *  TASK-1114 cl. g). A read that applied, saved or broadcast would violate
 *  GFX-§9's "read at build time" contract by making a scatter build mutate the
 *  player's settings.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsTierDIsPureTest,
	"Siegebound.Graphics.TierDReadsArePure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsTierDIsPureTest::RunTest(const FString& Parameters)
{
	SiegeGraphicsTestUtils::FScratchStore Store = SiegeGraphicsTestUtils::MakeScratchStore();
	if (!Store.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics store."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Store.Graphics.Get();

	Graphics->SetOverallScalabilityLevel(2);
	Graphics->ResetDiagnosticCountersForAutomationTests();

	// Read every Tier-D getter many times, as a scatter build would.
	float DensitySum = 0.0f;
	for (int32 Iteration = 0; Iteration < 32; ++Iteration)
	{
		DensitySum += Graphics->GetFoliageQualityScale();
		DensitySum += Graphics->GetViewDistanceScale();
		Graphics->ShouldEnableVolumetricFog();
	}
	TestTrue(TEXT("The Tier-D reads returned usable values"), DensitySum > 0.0f);

	TestEqual(TEXT("⛔ Tier-D reads applied NOTHING"), Graphics->ApplyNonResolutionSettingsCallCount, 0);
	TestEqual(TEXT("⛔ Tier-D reads applied no resolution change"), Graphics->ApplyResolutionSettingsCallCount, 0);
	TestEqual(TEXT("⛔ Tier-D reads saved NOTHING"), Graphics->SaveSettingsCallCount, 0);
	TestEqual(TEXT("⛔ Tier-D reads broadcast NOTHING"), Graphics->GraphicsSettingsChangeBroadcastCount, 0);

	// And repeated reads are stable — a getter that mutated what it read would drift.
	const float FirstDensity = Graphics->GetFoliageQualityScale();
	for (int32 Iteration = 0; Iteration < 8; ++Iteration)
	{
		TestEqual(TEXT("Repeated Tier-D reads are stable"), Graphics->GetFoliageQualityScale(), FirstDensity, 0.0001f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
