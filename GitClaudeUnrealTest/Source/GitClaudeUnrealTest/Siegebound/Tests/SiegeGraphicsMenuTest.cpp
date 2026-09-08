// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Siegebound/SiegeGraphicsMenuWidget.h"
#include "Siegebound/SiegeGraphicsSettingsSubsystem.h"
#include "Siegebound/SiegeSettingsSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 *  ═══ AUTOMATION TESTS for USiegeGraphicsMenuWidget (TASK-1115 [GFX-PANEL]) ═══
 *  Law: GFX-§2 (the six conditions) · GFX-§5 (which control each option gets) ·
 *  GFX-§6 · GFX-§9 (the "applies next match" duty) · GFX-§10 (the pinned names).
 *  Board riders (3a)-(3e). QA gate: TASK-1116. Compile + suite host: TASK-1124.
 *
 *  ⛔ A SEPARATE FILE FROM Tests/SiegeGraphicsSettingsTest.cpp ON PURPOSE: that
 *  file is TASK-1113's and TASK-1118 holds a write lock on it (board `names:`).
 *  Two agents writing one test file is the collision this lane already ruled out
 *  at the source level.
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  SCOPE STATEMENT (SC-§79 — what these tests CAN and CANNOT detect)
 *  ─────────────────────────────────────────────────────────────────────────
 *  ✅ THEY CAN DETECT:
 *     • a code-authored tree that does not build, or that does not root itself
 *       on BackdropBorder
 *     • a backdrop that is not hit-test VISIBLE (GFX-§2(f)) — the click-through
 *       into QUIT
 *     • a missing or misnamed GFX-§10 widget: all thirty group widgets are
 *       looked up BY THE PINNED NAME through the WidgetTree, not through the
 *       C++ member, so a member wired to a widget carrying the wrong name fails
 *     • a quality slider that is NOT detented (MouseUsesStep false ⇒ GFX-§5's
 *       "never fake continuity over 5 states" silently broken for every mouse
 *       drag), and a resolution-scale slider that IS
 *     • ⭐ a write bound to OnValueChanged instead of to the capture-end events
 *       (board cl. 3a) — BOTH halves: the binding and the handler behaviour
 *     • ⭐ a `Back` that leaves a staged video mode pending (board cl. 3b) —
 *       WITH a positive control that first proves the soft-lock is real
 *     • ⭐ a double revert on the Back path (board cl. 3b: "never two")
 *     • ⭐⭐ TASK-1118: a 10-second confirmation that does NOT auto-revert on
 *       expiry — the one path in this feature no player ever exercises
 *       deliberately, and the one whose absence is a PERMANENT LOCKOUT
 *     • ⭐⭐ a display change applied with NO way to auto-revert it (the
 *       no-timer-manager branch), which is the same lockout through another door
 *     • ⭐ a "Keep" that does not persist, or a "Revert" that does not restore
 *     • ⭐ ANY save reaching disk before the player confirms (GFX-§4 cl. 4)
 *     • ⭐ a countdown left running after Back — and a stranded timer callback
 *       that reverts a mode the player has since settled
 *     • a confirmation prompt armed by a QUALITY change (board cl. 3), or left
 *       up after the player steps back to the mode they started in
 *     • ⭐ a "Custom" label taken from the ELEVEN-group engine answer instead of
 *       IsOverallQualityCustomAcrossVisibleGroups() (board cl. 3c)
 *     • a level<->slider-value mapping that truncates instead of rounding
 *     • a stepper that does not wrap, or that divides by zero on an empty set
 *     • the "applies at the NEXT match" notice appearing on a group whose lever
 *       is immediate, or missing from the one whose lever is deferred
 *     • the Shadow row's volumetric-fog line not tracking ShouldEnableVolumetricFog()
 *     • a null subsystem that crashes, or that disables Back
 *
 *  ⛔ THEY CANNOT DETECT — AND THESE RUNGS ARE OWED ELSEWHERE:
 *     • ⭐ GFX-§2(c), THE ORDER. Nothing here proves ConstructGraphicsTree() is
 *       called BEFORE Super::RebuildWidget(). Building it after would still let
 *       every assertion below pass while shipping a silently EMPTY panel — the
 *       exact failure GFX-§2(e) exists for. ⇒ That is a THREE-LINE CODE READ at
 *       SiegeGraphicsMenuWidget.cpp's RebuildWidget(), and a QA criterion on
 *       TASK-1116, not a test.
 *     • Whether anything is VISIBLE, legible, on-screen, correctly sized, or
 *       reachable by a mouse. No Slate tree is built here at all. MCP read-back
 *       has passed on visually-broken UMG on this project before (TASK-355:
 *       6/6 bindings correct while stacked in a 165x48 px box in a corner).
 *       ⇒ GFX-§2(e): a PIXEL / HUMAN check. The checklist is in
 *       handoffs/TASK-1115-programmer.md and is TASK-1125's script.
 *     • Whether a moved setting changed a rendered frame. Every engine apply is
 *       suppressed by the facade's automation seam, so the Apply/Save counters
 *       record the DECISION, not the effect (the TASK-1113 scope statement,
 *       inherited whole).
 *     • Whether the ten sliders actually respond to a DRAG. SSlider's input path
 *       is Slate's; these tests drive the panel's own entry points instead.
 *     • That NativeDestruct() runs on a real RemoveFromParent(). Test 4 proves
 *       the two-call-sites-one-revert PREDICATE by calling
 *       DiscardStagedVideoMode twice; that NativeDestruct is reached is engine
 *       behaviour, asserted nowhere.
 *     • 🚨⭐ TASK-1118: THAT THE WORLD'S TIMER MANAGER ACTUALLY CALLS
 *       TickVideoModeCountdown(). A bare NewObject widget has NO WORLD, so there
 *       is no timer manager to drive and none of these tests creates one. The
 *       tests below drive the tick BY HAND through the automation seam, which
 *       proves the countdown's ARITHMETIC, its EXPIRY BODY and every exit — but
 *       the SetTimer→callback edge itself is engine behaviour used identically at
 *       a dozen shipped sites in this project (HeroCharacter.cpp:742/:1272,
 *       GoldNode.cpp:417, BattlefieldScatter.cpp:2018/:2438, CaptureZone.cpp:95).
 *       ⇒ IT IS A DECLARED UNPROVEN PREMISE, not a covered one. The pixel/human
 *       check owed to TASK-1125 is: change Window Mode, TOUCH NOTHING, and watch
 *       it come back on its own.
 *     • Whether ten ticks of a 1 s timer take ten WALL-CLOCK seconds. The tests
 *       assert ten ticks; the period is FTimerManager's.
 *
 *  🚨 NO WITNESSED RED. Nothing in this file has been compiled or executed. The
 *  mutation table at the bottom is a set of DERIVED PREDICTIONS, exactly like
 *  TASK-1113's M1-M14 were — and M4 in that file is the standing proof that a
 *  derived prediction can be confidently wrong (SC-§83, SC-§90).
 */
namespace SiegeGraphicsMenuTestUtils
{
	/** A scratch facade + a scratch panel, neither of which can touch the machine's real ini or the live editor's resolution. */
	struct FScratchPanel
	{
		TStrongObjectPtr<UGameInstance>                  GameInstance;
		TStrongObjectPtr<UGameUserSettings>              Settings;
		TStrongObjectPtr<USiegeGraphicsSettingsSubsystem> Graphics;
		TStrongObjectPtr<USiegeGraphicsMenuWidget>       Panel;

		bool IsValid() const
		{
			return GameInstance.IsValid() && Settings.IsValid() && Graphics.IsValid() && Panel.IsValid();
		}
	};

	/**
	 *  ⛔ HERMETIC BY CONSTRUCTION. The facade is pointed at a scratch
	 *  UGameUserSettings and engine applies are suppressed IN THE SAME CALL, so a
	 *  run cannot read, write or apply Saved/Config/.../GameUserSettings.ini and
	 *  cannot change the live editor's resolution.
	 *
	 *  ⚠️ The ConfirmVideoMode() below is a TEST-HARNESS necessity, not a claim
	 *  about a booted game: a bare NewObject<UGameUserSettings> runs the
	 *  constructor's SetToDefaults() (FullscreenMode = 1) and NEVER LoadSettings(),
	 *  while LastConfirmedFullscreenMode is a plain int32 defaulting to 0 ⇒ 1 != 0
	 *  on a brand-new object. Without this, every video-mode assertion below would
	 *  be measuring that engine default instead of the panel's behaviour. (The
	 *  scope correction is TASK-1114 WARN-7's; do not read it as evidence about
	 *  the facade's boot heal.)
	 *
	 *  ⚠️ The panel is Initialize()d and its tree built WITHOUT taking a Slate
	 *  widget. UUserWidget::Initialize() is headless-safe for a NATIVE class:
	 *  no UWidgetBlueprintGeneratedClass ⇒ InitializeNativeClassData() is an empty
	 *  virtual, WidgetTree is NewObject'd, and NativeOnInitialized is skipped
	 *  because PlayerContext is invalid (UserWidget.cpp, UUserWidget::Initialize).
	 */
	static FScratchPanel MakeScratchPanel(bool bInjectSubsystem = true)
	{
		FScratchPanel Scratch;

		Scratch.GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackageAsObject()));
		Scratch.Settings.Reset(NewObject<UGameUserSettings>(GetTransientPackageAsObject()));

		if (Scratch.Settings.IsValid())
		{
			Scratch.Settings->ConfirmVideoMode();
		}

		if (Scratch.GameInstance.IsValid() && Scratch.Settings.IsValid())
		{
			Scratch.Graphics.Reset(NewObject<USiegeGraphicsSettingsSubsystem>(Scratch.GameInstance.Get()));
			if (Scratch.Graphics.IsValid())
			{
				Scratch.Graphics->SetGameUserSettingsForAutomationTests(Scratch.Settings.Get());
				Scratch.Graphics->ResetDiagnosticCountersForAutomationTests();
			}
		}

		Scratch.Panel.Reset(NewObject<USiegeGraphicsMenuWidget>(GetTransientPackageAsObject()));
		if (Scratch.Panel.IsValid())
		{
			if (bInjectSubsystem && Scratch.Graphics.IsValid())
			{
				Scratch.Panel->SetGraphicsSubsystemForAutomationTests(Scratch.Graphics.Get());
			}

			Scratch.Panel->Initialize();
			Scratch.Panel->ConstructGraphicsTree();
			Scratch.Panel->SeedAndBind();
		}

		return Scratch;
	}

	/**
	 *  ⭐ TASK-1118: a scratch panel whose countdown the SUITE drives.
	 *
	 *  ⚠️ THE SEAM IS NOT A SHORTCUT, IT IS THE ONLY DOOR. A bare NewObject widget
	 *  has no world ⇒ no FTimerManager, and the panel's own CanArmVideoModeCountdown()
	 *  then REFUSES to apply anything (correctly — an applied mode with no
	 *  auto-revert is the lockout itself). Without this switch every countdown
	 *  test would silently be measuring that refusal branch and the expiry path
	 *  would have zero coverage while looking covered. ⛔ Exactly one test below
	 *  leaves it OFF, on purpose, to assert the refusal.
	 */
	static void DriveCountdownManually(const FScratchPanel& Scratch)
	{
		if (Scratch.Panel.IsValid())
		{
			Scratch.Panel->SetVideoModeCountdownDrivenManuallyForAutomationTests(true);
		}
	}

	/** One ordinary ">" press on Window Mode — the cheapest real player action that arms the countdown. Returns the mode the player started in. */
	static int32 PressWindowModeNext(const FScratchPanel& Scratch)
	{
		const int32 ModeBefore = Scratch.Graphics->GetWindowMode();
		Scratch.Panel->HandleWindowModeNextClicked();
		return ModeBefore;
	}

	/** Look a pinned child up BY NAME through the tree — never through the C++ member, so a member holding a wrongly-named widget fails. */
	template <typename WidgetT>
	static WidgetT* FindPinned(const FScratchPanel& Scratch, const TCHAR* PinnedName)
	{
		if (!Scratch.Panel.IsValid() || Scratch.Panel->WidgetTree == nullptr)
		{
			return nullptr;
		}
		return Scratch.Panel->WidgetTree->FindWidget<WidgetT>(FName(PinnedName));
	}
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 1 — the tree builds, and every GFX-§10 pinned name is IN it
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuTreeTest,
	"Siegebound.GraphicsMenu.TreeBuildsEveryPinnedRow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuTreeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	TestNotNull(TEXT("Initialize() allocated a WidgetTree"), Panel->WidgetTree.Get());
	if (Panel->WidgetTree == nullptr)
	{
		return false;
	}

	// ⭐ THE ROOT. A null RootWidget is what Super::RebuildWidget() turns into an
	// SSpacer — a silently EMPTY panel that passes every property read-back
	// (GFX-§2(c)). This asserts the tree HAS a root; that it is built BEFORE
	// Super is a code read, declared in the scope statement above.
	UBorder* Backdrop = FindPinned<UBorder>(Scratch, TEXT("BackdropBorder"));
	TestNotNull(TEXT("BackdropBorder exists under its pinned name"), Backdrop);
	TestTrue(TEXT("⭐ WidgetTree->RootWidget IS BackdropBorder — the panel has a root"),
		Panel->WidgetTree->RootWidget.Get() == static_cast<UWidget*>(Backdrop));

	// ⭐ GFX-§2(f) — CORRECTNESS, NOT STYLING. A HIT_TEST_INVISIBLE plate ships a
	// live click-through into Play / Sandbox / Deck Builder / QUIT while the panel
	// looks modal, because this panel stacks on the LIVE settings panel which
	// stacks on the LIVE main menu and NEITHER is removed.
	if (Backdrop != nullptr)
	{
		TestEqual(TEXT("⭐ The backdrop is hit-test VISIBLE (GFX-§2(f): clicks must not reach QUIT behind it)"),
			static_cast<int32>(Backdrop->GetVisibility()), static_cast<int32>(ESlateVisibility::Visible));
	}

	TestNotNull(TEXT("RootScrollBox exists — board cl. (4): 19 rows do not fit 1080p"),
		FindPinned<UScrollBox>(Scratch, TEXT("RootScrollBox")));
	TestNotNull(TEXT("RootPanel exists"), FindPinned<UVerticalBox>(Scratch, TEXT("RootPanel")));
	TestNotNull(TEXT("TitleText exists"), FindPinned<UTextBlock>(Scratch, TEXT("TitleText")));
	TestNotNull(TEXT("AutoDetectButton exists (GFX-§6)"), FindPinned<UButton>(Scratch, TEXT("AutoDetectButton")));
	TestNotNull(TEXT("BackButton exists"), FindPinned<UButton>(Scratch, TEXT("BackButton")));
	TestNotNull(TEXT("BackLabelText exists"), FindPinned<UTextBlock>(Scratch, TEXT("BackLabelText")));
	TestNotNull(TEXT("VSyncCheckBox exists (GFX-§5: a boolean gets a checkbox)"),
		FindPinned<UCheckBox>(Scratch, TEXT("VSyncCheckBox")));

	// Tier A + Tier C
	TestNotNull(TEXT("OverallQualitySlider exists"), FindPinned<USlider>(Scratch, TEXT("OverallQualitySlider")));
	TestNotNull(TEXT("OverallQualityValueText exists"), FindPinned<UTextBlock>(Scratch, TEXT("OverallQualityValueText")));
	TestNotNull(TEXT("ResolutionScaleSlider exists"), FindPinned<USlider>(Scratch, TEXT("ResolutionScaleSlider")));
	TestNotNull(TEXT("ScreenResolutionPrevButton exists"), FindPinned<UButton>(Scratch, TEXT("ScreenResolutionPrevButton")));
	TestNotNull(TEXT("ScreenResolutionNextButton exists"), FindPinned<UButton>(Scratch, TEXT("ScreenResolutionNextButton")));
	TestNotNull(TEXT("WindowModePrevButton exists"), FindPinned<UButton>(Scratch, TEXT("WindowModePrevButton")));
	TestNotNull(TEXT("WindowModeNextButton exists"), FindPinned<UButton>(Scratch, TEXT("WindowModeNextButton")));
	TestNotNull(TEXT("FrameRateLimitPrevButton exists (GFX-§5: a stepper, never a slider — the ladder is unevenly spaced)"),
		FindPinned<UButton>(Scratch, TEXT("FrameRateLimitPrevButton")));
	TestNotNull(TEXT("FrameRateLimitNextButton exists"), FindPinned<UButton>(Scratch, TEXT("FrameRateLimitNextButton")));

	// ⭐⭐ TASK-1118 — GFX-§10 pins all four of these character-for-character.
	// Looked up THROUGH THE TREE by the pinned name, not through the C++ member,
	// so a member wired to a widget carrying the wrong name fails here: an
	// asset-authored WBP_GraphicsMenu binds on the NAME, and a drifted name would
	// leave the Keep button dead with everything else looking correct.
	UBorder* ConfirmBorder = FindPinned<UBorder>(Scratch, TEXT("VideoModeConfirmBorder"));
	TestNotNull(TEXT("⭐ VideoModeConfirmBorder exists under its pinned name (GFX-§4's revert prompt)"), ConfirmBorder);
	TestNotNull(TEXT("⭐ VideoModeConfirmText exists"), FindPinned<UTextBlock>(Scratch, TEXT("VideoModeConfirmText")));
	TestNotNull(TEXT("⭐ KeepSettingsButton exists"), FindPinned<UButton>(Scratch, TEXT("KeepSettingsButton")));
	TestNotNull(TEXT("⭐ RevertSettingsButton exists"), FindPinned<UButton>(Scratch, TEXT("RevertSettingsButton")));

	// ⛔ AND IT IS COLLAPSED UNTIL SOMETHING IS ACTUALLY PENDING. A prompt that
	// opened with the panel would ask "Keep these settings?" about a change nobody
	// made, and Collapsed rather than Hidden is what keeps it from reserving a
	// ten-line gap under the steppers for the 99 % of the time it is not asking.
	if (ConfirmBorder != nullptr)
	{
		TestEqual(TEXT("⭐⭐ The revert prompt is COLLAPSED on a freshly built tree — it appears only when a display change is outstanding"),
			static_cast<int32>(ConfirmBorder->GetVisibility()), static_cast<int32>(ESlateVisibility::Collapsed));
	}
	TestFalse(TEXT("⭐ ...and no countdown is running on a panel nobody has touched"),
		Panel->IsVideoModeCountdownActive());

	// ⭐ TIER B — THE TEN. Each triple is looked up by its DERIVED pinned name, so
	// a hand-typed member that drifted from the canonical group spelling fails
	// here rather than shipping a slider wired to nothing.
	const TArray<FName> GroupNames = USiegeGraphicsSettingsSubsystem::GetQualityGroupNames();
	TestEqual(TEXT("⭐ The panel loops the FACADE'S list — ten groups, not a second hand-written one"),
		GroupNames.Num(), 10);

	for (const FName& GroupName : GroupNames)
	{
		const FString Base = GroupName.ToString();

		USlider* Slider = FindPinned<USlider>(Scratch, *(Base + TEXT("Slider")));
		TestNotNull(*FString::Printf(TEXT("%sSlider exists under its GFX-§10 pinned name"), *Base), Slider);
		TestNotNull(*FString::Printf(TEXT("%sLabelText exists"), *Base),
			FindPinned<UTextBlock>(Scratch, *(Base + TEXT("LabelText"))));
		TestNotNull(*FString::Printf(TEXT("%sValueText exists — a detented slider with no label is unreadable"), *Base),
			FindPinned<UTextBlock>(Scratch, *(Base + TEXT("ValueText"))));

		// ⛔ THE PAIRING. FindGroupSlider walks the C++ member; FindPinned walks the
		// tree by name. If ResolveGroupRow ever paired ShadowQuality with
		// ReflectionQualitySlider, these two would disagree — and a mis-wired row
		// is INVISIBLE at runtime: the handle moves, a setting changes, and it is
		// the WRONG setting.
		TestTrue(*FString::Printf(TEXT("⭐ %s's member and its named widget are the SAME object (no mis-paired row)"), *Base),
			Panel->FindGroupSlider(GroupName) == Slider);
	}

	// ⛔ THE ELEVENTH GROUP IS ABSENT AND MUST STAY ABSENT. It has no
	// [LandscapeQuality@N] section at any level, no auto-detect threshold table and
	// no Landscape in this project ⇒ the slider would move and change nothing.
	// A draggable row that provably cannot change a pixel teaches the player the
	// menu lies (GFX-§9).
	TestNull(TEXT("⛔ There is NO LandscapeQualitySlider — a control that cannot change a pixel does not ship"),
		FindPinned<USlider>(Scratch, TEXT("LandscapeQualitySlider")));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 2 — GFX-§5: the detents are REAL for a mouse, and the continuous one
//           really is continuous
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE MEASUREMENT THIS TEST EXISTS FOR: USlider::MouseUsesStep DEFAULTS TO
 *  false (Slider.cpp:27), and SSlider::PositionToValue only snaps to StepSize
 *  INSIDE `if (bMouseUsesStep)` (SSlider.cpp:455). ⇒ StepSize 0.25 ALONE affects
 *  keyboard / gamepad navigation ONLY, and a mouse drag on a "5-detent" quality
 *  slider would glide through 0.37 and 0.61 — EXACTLY the "fake continuity over
 *  5 states" GFX-§5 forbids, while looking correct in every code review.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuDetentTest,
	"Siegebound.GraphicsMenu.DetentSlidersSnapAndTheContinuousOneDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuDetentTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	const float ExpectedStep = 1.0f / static_cast<float>(USiegeGraphicsSettingsSubsystem::MaxQualityLevel);

	TArray<USlider*> Detented;
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		Detented.Add(FindPinned<USlider>(Scratch, *(GroupName.ToString() + TEXT("Slider"))));
	}
	Detented.Add(FindPinned<USlider>(Scratch, TEXT("OverallQualitySlider")));

	TestEqual(TEXT("Eleven detented sliders: ten groups + the overall preset"), Detented.Num(), 11);

	for (USlider* Slider : Detented)
	{
		if (Slider == nullptr)
		{
			AddError(TEXT("A detented slider is missing — see TreeBuildsEveryPinnedRow."));
			continue;
		}

		const FString Name = Slider->GetName();
		TestEqual(*FString::Printf(TEXT("%s spans 0..1"), *Name), Slider->GetMinValue(), 0.0f);
		TestEqual(*FString::Printf(TEXT("%s spans 0..1"), *Name), Slider->GetMaxValue(), 1.0f);
		TestTrue(*FString::Printf(TEXT("%s has GFX-§5's StepSize 0.25 ⇒ five detents"), *Name),
			FMath::IsNearlyEqual(Slider->GetStepSize(), ExpectedStep, UE_KINDA_SMALL_NUMBER));

		// ⭐ THE ONE THAT MAKES THE STEP REAL FOR A MOUSE.
		TestTrue(*FString::Printf(TEXT("⭐ %s sets MouseUsesStep — without it a MOUSE DRAG is continuous and GFX-§5 is a comment"), *Name),
			Slider->MouseUsesStep);
	}

	// ── AND THE MIRROR IMAGE: the one control that really IS continuous ──────
	USlider* Scale = FindPinned<USlider>(Scratch, TEXT("ResolutionScaleSlider"));
	if (Scale != nullptr)
	{
		TestEqual(TEXT("ResolutionScaleSlider's floor is GFX-§5's honest 50%"),
			Scale->GetMinValue(), USiegeGraphicsSettingsSubsystem::MinResolutionScalePercent);
		TestEqual(TEXT("ResolutionScaleSlider's ceiling is 100%"),
			Scale->GetMaxValue(), USiegeGraphicsSettingsSubsystem::MaxResolutionScalePercent);
		TestTrue(TEXT("ResolutionScaleSlider steps by 1% for keyboard/pad"),
			FMath::IsNearlyEqual(Scale->GetStepSize(), 1.0f, UE_KINDA_SMALL_NUMBER));

		// ⛔ FALSE ON PURPOSE. Snapping the one genuinely continuous control would
		// be the same lie as failing to snap the five-state ones, in reverse.
		TestFalse(TEXT("⭐ ResolutionScaleSlider does NOT set MouseUsesStep — it is the one control that really is continuous"),
			Scale->MouseUsesStep);
	}
	else
	{
		AddError(TEXT("ResolutionScaleSlider is missing."));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 3 — the level <-> slider-value mapping
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuLevelMapTest,
	"Siegebound.GraphicsMenu.LevelSliderValueRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuLevelMapTest::RunTest(const FString& Parameters)
{
	for (int32 Level = USiegeGraphicsSettingsSubsystem::MinQualityLevel;
		Level <= USiegeGraphicsSettingsSubsystem::MaxQualityLevel; ++Level)
	{
		const float Value = USiegeGraphicsMenuWidget::LevelToSliderValue(Level);
		TestEqual(*FString::Printf(TEXT("Level %d round-trips through the slider value"), Level),
			USiegeGraphicsMenuWidget::SliderValueToLevel(Value), Level);
	}

	// ⭐ THE TRUNCATION TRAP. FMath::TruncToInt(0.9999f * 4) is 3, so a handle
	// parked on Cinematic by an imprecise drag would read as Epic and the panel
	// would write the WRONG level back on commit. Rounding is the fix and this is
	// the assertion that notices if it is ever swapped back.
	TestEqual(TEXT("⭐ 0.9999 rounds UP to Cinematic (4), it does not truncate to Epic (3)"),
		USiegeGraphicsMenuWidget::SliderValueToLevel(0.9999f), 4);
	TestEqual(TEXT("⭐ 0.2499 rounds to Medium (1), not down to Low"),
		USiegeGraphicsMenuWidget::SliderValueToLevel(0.2499f), 1);
	TestEqual(TEXT("Exactly between two detents rounds up (0.375 -> 2)"),
		USiegeGraphicsMenuWidget::SliderValueToLevel(0.375f), 2);

	// Clamps at both ends rather than asserting.
	TestEqual(TEXT("A negative value clamps to Low"), USiegeGraphicsMenuWidget::SliderValueToLevel(-3.0f), 0);
	TestEqual(TEXT("An over-range value clamps to Cinematic"), USiegeGraphicsMenuWidget::SliderValueToLevel(9.0f), 4);
	TestEqual(TEXT("A negative level clamps to the Low handle position"),
		USiegeGraphicsMenuWidget::LevelToSliderValue(-1), 0.0f);
	TestEqual(TEXT("An over-range level clamps to the Cinematic handle position"),
		USiegeGraphicsMenuWidget::LevelToSliderValue(99), 1.0f);

	// The Custom label is the LABEL's job, never the handle's (GFX-§5).
	TestEqual(TEXT("ComposeLevelValueText reports Custom when asked to"),
		USiegeGraphicsMenuWidget::ComposeLevelValueText(2, /*bCustom*/ true), FString(TEXT("Custom")));
	TestEqual(TEXT("...and otherwise defers to the ENGINE's own localised level name"),
		USiegeGraphicsMenuWidget::ComposeLevelValueText(2, /*bCustom*/ false),
		USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(2).ToString());

	// The Custom handle position: the mean, and it is the LEAST misleading place
	// for a handle that has to be somewhere.
	TestEqual(TEXT("AveragedLevel of an empty array is 0, not a divide by zero"),
		USiegeGraphicsMenuWidget::AveragedLevel(TArray<int32>()), 0);
	TestEqual(TEXT("AveragedLevel rounds"), USiegeGraphicsMenuWidget::AveragedLevel({ 0, 3 }), 2);
	TestEqual(TEXT("AveragedLevel of a uniform set is that level"),
		USiegeGraphicsMenuWidget::AveragedLevel({ 3, 3, 3, 3 }), 3);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 4 ⭐⭐ — board cl. (3b): the discard is issued ONCE, and only when owed
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE SHARED-HANDLER CONTRACT WITH TASK-1118 cl. (8), ASSERTED.
 *  DiscardStagedVideoMode is called from TWO sites (BackPressed and
 *  NativeDestruct) and must issue EXACTLY ONE RevertVideoModeChange() on the Back
 *  path — "never two", because a double revert re-applies a mode nobody asked
 *  for. The guard, not the ordering, is what guarantees it, so this test calls it
 *  TWICE in a row: that is the Back path's shape with no Slate in the way.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuDiscardOnceTest,
	"Siegebound.GraphicsMenu.DiscardStagedVideoModeRevertsExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuDiscardOnceTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();

	// ── NOTHING PENDING ⇒ NOTHING REVERTED ───────────────────────────────────
	// A discard that fired unconditionally would re-apply the resolution on every
	// single Back press — a mode change fired at a player who asked for nothing.
	Graphics->ResetDiagnosticCountersForAutomationTests();
	TestFalse(TEXT("Fixture self-check: no video-mode change is pending yet"), Graphics->IsVideoModeChangePending());
	TestFalse(TEXT("⛔ With nothing staged, the discard reverts NOTHING"),
		USiegeGraphicsMenuWidget::DiscardStagedVideoMode(Graphics));
	TestEqual(TEXT("...and reaches no resolution apply at all"), Graphics->ApplyResolutionSettingsCallCount, 0);

	// ── A NULL SUBSYSTEM IS NOT A CRASH ──────────────────────────────────────
	TestFalse(TEXT("A null subsystem returns false rather than crashing"),
		USiegeGraphicsMenuWidget::DiscardStagedVideoMode(nullptr));

	// ── STAGE A MODE (one stepper press is enough — the facade STAGES only) ───
	const int32 StagedMode = USiegeGraphicsMenuWidget::StepIndex(
		Graphics->GetWindowMode(), 1, USiegeGraphicsSettingsSubsystem::GetWindowModeCount());
	Graphics->SetWindowMode(StagedMode);

	TestTrue(TEXT("⭐ Fixture self-check: ONE stepper press makes IsVideoModeChangePending() true — the staged-vs-LastConfirmed comparison ALONE"),
		Graphics->IsVideoModeChangePending());

	Graphics->ResetDiagnosticCountersForAutomationTests();

	// ── CALL 1: reverts ──────────────────────────────────────────────────────
	TestTrue(TEXT("⭐ The first discard DOES revert"), USiegeGraphicsMenuWidget::DiscardStagedVideoMode(Graphics));
	TestEqual(TEXT("⭐ ...reaching the facade's revert, which applies the resolution EXACTLY once"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestFalse(TEXT("⭐ ...and the window is now CLOSED"), Graphics->IsVideoModeChangePending());

	// ── CALL 2 (the NativeDestruct half of the Back path): reverts NOTHING ────
	TestFalse(TEXT("⭐⭐ The SECOND discard returns false — board cl. (3b): EXACTLY ONE revert on the Back path, never two"),
		USiegeGraphicsMenuWidget::DiscardStagedVideoMode(Graphics));
	TestEqual(TEXT("⭐⭐ ...and issues NO second ApplyResolutionSettings (a double revert re-applies a mode nobody asked for)"),
		Graphics->ApplyResolutionSettingsCallCount, 1);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 5 ⭐⭐ — board cl. (3b-WIDENED): Back releases BOTH refusals
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE WHOLE DEFECT, END TO END, THROUGH THE PANEL'S OWN Back HANDLER — and
 *  with a POSITIVE CONTROL FIRST, because a test that has never seen the failure
 *  it guards is not evidence that the guard works (SC-§99).
 *
 *  The instrument is RefusedSaveWhileVideoModePendingCount and NOT the bool that
 *  AutoDetectQuality() returns. That is load-bearing: under the automation seam
 *  AutoDetectQuality returns false from the SUPPRESSION branch too, so the bool
 *  reads identically on a broken build and a fixed one. Only the counter tells
 *  the two falses apart, which is exactly what board cl. (8-WIDENED) says to
 *  assert.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuBackSoftLockTest,
	"Siegebound.GraphicsMenu.BackReleasesAutoDetectAndEverySave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuBackSoftLockTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	// ── STEP 1: stage a mode with one ordinary stepper press ─────────────────
	const int32 StagedMode = USiegeGraphicsMenuWidget::StepIndex(
		Graphics->GetWindowMode(), 1, USiegeGraphicsSettingsSubsystem::GetWindowModeCount());
	Graphics->SetWindowMode(StagedMode);
	Graphics->ResetDiagnosticCountersForAutomationTests();

	// ── STEP 2: THE POSITIVE CONTROL — prove the soft-lock is REAL ───────────
	// ⛔ Without these four assertions the test below could pass on a build where
	// nothing was ever broken, and would then be worth nothing.
	TestFalse(TEXT("POSITIVE CONTROL: with a mode staged, Auto-Detect is REFUSED"), Graphics->AutoDetectQuality());
	TestEqual(TEXT("POSITIVE CONTROL: ...and the refusal is COUNTED (this is the refusal branch, not the suppression branch)"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 1);

	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 1);
	TestEqual(TEXT("POSITIVE CONTROL: ...and an unrelated quality change does NOT reach a save"),
		Graphics->SaveSettingsCallCount, 0);
	TestTrue(TEXT("POSITIVE CONTROL: ...its save was refused too"),
		Graphics->RefusedSaveWhileVideoModePendingCount >= 2);

	// ── STEP 3: THE PLAYER PRESSES Back ──────────────────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->BackPressed();

	TestFalse(TEXT("⭐⭐ After Back, NO video-mode change is pending — the panel closed the window nothing below it could see"),
		Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("⭐ Back issued exactly one revert"), Graphics->ApplyResolutionSettingsCallCount, 1);

	// ── STEP 4: BOTH refusals are gone ───────────────────────────────────────
	// ⛔ THE COUNTER, NOT THE BOOL. AutoDetectQuality still returns false here
	// because the automation seam suppresses the benchmark — a test asserting the
	// bool would be GREEN on the broken build too.
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Graphics->AutoDetectQuality();
	TestEqual(TEXT("⭐⭐ Auto-Detect is NO LONGER refused — the refusal counter did not move (board cl. 3b-WIDENED's QA criterion)"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 0);

	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 3);
	TestTrue(TEXT("⭐⭐ ...and a quality change SAVES again"), Graphics->SaveSettingsCallCount >= 1);
	TestEqual(TEXT("⭐⭐ ...with no refusal on the way"), Graphics->RefusedSaveWhileVideoModePendingCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 6 ⭐⭐ — board cl. (3a): the continuous slider writes ON COMMIT ONLY
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 BOTH HALVES, BECAUSE NEITHER IS EVIDENCE ALONE:
 *    (a) THE BINDING — the write must be armed on the capture-end events and the
 *        label on OnValueChanged. A handler that never writes proves nothing if
 *        the write is bound somewhere else.
 *    (b) THE BEHAVIOUR — the value-changed path must reach no save. A binding
 *        proves nothing if the handler body was moved.
 *
 *  The cost of getting it wrong (gate F-5): SetResolutionScalePercent applies AND
 *  saves, and USlider fires OnValueChanged once per frame of a drag ⇒ a single
 *  50→100 drag becomes up to FIFTY SaveConfig FILE WRITES.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCommitOnlyTest,
	"Siegebound.GraphicsMenu.ResolutionScaleWritesOnCommitNotOnValueChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCommitOnlyTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	USlider* Scale = FindPinned<USlider>(Scratch, TEXT("ResolutionScaleSlider"));
	if (Scale == nullptr)
	{
		AddError(TEXT("ResolutionScaleSlider is missing — the cl. (3a) contract cannot be checked."));
		return false;
	}

	// ── (a) THE BINDING ──────────────────────────────────────────────────────
	TestTrue(TEXT("⭐ OnValueChanged is bound to the LABEL handler"),
		Scale->OnValueChanged.Contains(Panel, TEXT("HandleResolutionScaleValueChanged")));
	TestFalse(TEXT("⭐⭐ OnValueChanged is NOT bound to the WRITE handler — one drag would be up to 50 ini file writes"),
		Scale->OnValueChanged.Contains(Panel, TEXT("HandleResolutionScaleCommitted")));
	TestTrue(TEXT("⭐ OnMouseCaptureEnd carries the write"),
		Scale->OnMouseCaptureEnd.Contains(Panel, TEXT("HandleResolutionScaleCommitted")));

	// ⚠️ BOTH capture-end events, and the second is not belt-and-braces: a gamepad
	// or keyboard user NEVER produces a mouse capture end. SSlider commits on
	// OnFocusLost and then fires OnControllerCaptureEnd (SSlider.cpp:280-289), so
	// binding only the mouse event would silently drop every pad-driven change.
	TestTrue(TEXT("⭐ OnControllerCaptureEnd carries it too — a pad user never fires a MOUSE capture end"),
		Scale->OnControllerCaptureEnd.Contains(Panel, TEXT("HandleResolutionScaleCommitted")));

	// The same split, on a detented group slider: cl. (3a) is "not exempt in
	// principle, only in degree".
	if (USlider* Shadows = Panel->FindGroupSlider(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality))
	{
		TestTrue(TEXT("A group slider's OnValueChanged is the label handler"),
			Shadows->OnValueChanged.Contains(Panel, TEXT("HandleGroupSliderValueChanged")));
		TestFalse(TEXT("⭐ A group slider's OnValueChanged is NOT the write handler"),
			Shadows->OnValueChanged.Contains(Panel, TEXT("HandleGroupSliderCommitted")));
		TestTrue(TEXT("A group slider's capture end carries the write"),
			Shadows->OnMouseCaptureEnd.Contains(Panel, TEXT("HandleGroupSliderCommitted")));
	}

	// ── (b) THE BEHAVIOUR — fifty label updates, zero writes ─────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();

	for (int32 Percent = 100; Percent >= 51; --Percent)
	{
		Scale->SetValue(static_cast<float>(Percent));
		Panel->HandleResolutionScaleValueChanged(static_cast<float>(Percent));
	}

	TestEqual(TEXT("⭐⭐ FIFTY simulated drag frames reached ZERO saves (gate F-5: this is the fifty-file-writes defect)"),
		Graphics->SaveSettingsCallCount, 0);
	TestEqual(TEXT("⭐⭐ ...and ZERO applies"), Graphics->ApplyNonResolutionSettingsCallCount, 0);
	TestEqual(TEXT("⭐ ...and ZERO broadcasts, so nothing downstream was told either"),
		Graphics->GraphicsSettingsChangeBroadcastCount, 0);

	// ...but the LABEL tracked the drag the whole way, or the control is unreadable.
	if (UTextBlock* ScaleValue = FindPinned<UTextBlock>(Scratch, TEXT("ResolutionScaleValueText")))
	{
		TestEqual(TEXT("⭐ The label DID follow the drag — the write is deferred, the feedback is not"),
			ScaleValue->GetText().ToString(), FString(TEXT("51%")));
	}

	// ── AND THEN ONE COMMIT WRITES ONCE ──────────────────────────────────────
	Panel->HandleResolutionScaleCommitted();

	TestEqual(TEXT("⭐⭐ The commit issues EXACTLY ONE save"), Graphics->SaveSettingsCallCount, 1);
	TestEqual(TEXT("⭐ ...applied through the quality path, not the resolution path"),
		Graphics->ApplyNonResolutionSettingsCallCount, 1);
	TestTrue(TEXT("⭐ ...and the settled value is what landed"),
		FMath::IsNearlyEqual(Graphics->GetResolutionScalePercent(), 51.0f, 0.5f));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 7 — a commit writes ONLY the group whose handle moved
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuGroupCommitTest,
	"Siegebound.GraphicsMenu.GroupCommitWritesOnlyTheMovedGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuGroupCommitTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	// Put every visible group on a known rung, then re-seed the panel from it.
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		Graphics->SetQualityGroupLevel(GroupName, 3);
	}
	Panel->RefreshAllRows();

	// A commit with NOTHING moved must write nothing at all — otherwise every
	// stray click on a handle would cost an ini write.
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->HandleGroupSliderCommitted();
	TestEqual(TEXT("⭐ A commit with no handle moved writes NOTHING"), Graphics->SaveSettingsCallCount, 0);
	TestEqual(TEXT("...and broadcasts nothing"), Graphics->GraphicsSettingsChangeBroadcastCount, 0);

	// Move ONE handle, the way a drag would.
	const FName Moved = USiegeGraphicsSettingsSubsystem::GroupName_EffectsQuality;
	USlider* MovedSlider = Panel->FindGroupSlider(Moved);
	if (MovedSlider == nullptr)
	{
		AddError(TEXT("EffectsQualitySlider is missing."));
		return false;
	}

	Graphics->ResetDiagnosticCountersForAutomationTests();
	MovedSlider->SetValue(USiegeGraphicsMenuWidget::LevelToSliderValue(1));
	Panel->HandleGroupSliderCommitted();

	TestEqual(TEXT("⭐ The moved group was written"), Graphics->GetQualityGroupLevel(Moved), 1);
	TestEqual(TEXT("⭐ ...and exactly one save was issued, not ten"), Graphics->SaveSettingsCallCount, 1);

	// ⛔ THE MIS-PAIRING GUARD. If ResolveGroupRow ever paired a group name with a
	// neighbour's widgets, the WRONG setting would move and the panel would look
	// completely correct doing it.
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (GroupName == Moved)
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("⭐ %s did NOT move — one handle, one setting"), *GroupName.ToString()),
			Graphics->GetQualityGroupLevel(GroupName), 3);
	}

	// The row's own label followed, using the ENGINE's level name.
	if (UTextBlock* MovedValue = Panel->FindGroupValueText(Moved))
	{
		TestEqual(TEXT("The moved row's ValueText reads the engine's own name for level 1"),
			MovedValue->GetText().ToString(),
			USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(1).ToString());
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 8 ⭐⭐ — board cl. (3c): "Custom" comes from the VISIBLE groups
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE DEFECT THIS PREVENTS IS PERMANENT AND UNFIXABLE FROM THE PANEL:
 *  GetOverallScalabilityLevel() answers over ELEVEN groups AND requires
 *  ResolutionQuality to equal the preset's canonical render scale
 *  (Scalability.cpp:1083-1097). ⇒ nudging the resolution-scale bar — or
 *  Auto-Detect stranding the invisible LandscapeQuality — makes the preset read
 *  "Custom" FOREVER with all ten visible sliders in agreement and nothing on
 *  screen able to fix it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCustomLabelTest,
	"Siegebound.GraphicsMenu.CustomLabelIgnoresInvisibleInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCustomLabelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	UTextBlock* OverallValue = FindPinned<UTextBlock>(Scratch, TEXT("OverallQualityValueText"));
	if (OverallValue == nullptr)
	{
		AddError(TEXT("OverallQualityValueText is missing."));
		return false;
	}

	// ── ALL TEN VISIBLE GROUPS AGREE ON High (2) ─────────────────────────────
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		Graphics->SetQualityGroupLevel(GroupName, 2);
	}

	// ── ...AND THEN AN INVISIBLE INPUT DISAGREES ─────────────────────────────
	// 63% is not one of PerfIndexValues_ResolutionQuality's canonical rungs
	// (50 71 87 100 100), so the ENGINE's answer must become Custom while every
	// slider the player can see still says High.
	Graphics->SetResolutionScaleNormalized(0.63f);

	TestTrue(TEXT("FIXTURE SELF-CHECK: the ENGINE's eleven-input answer really is Custom here (if this fails the premise is gone, not the panel)"),
		Graphics->IsOverallQualityCustom());

	TestFalse(TEXT("⭐⭐ The PANEL's question is NOT Custom — it asks only about the ten groups a player can see (board cl. 3c)"),
		USiegeGraphicsMenuWidget::IsCustomForDisplay(Graphics));

	Panel->RefreshAllRows();
	TestNotEqual(TEXT("⭐⭐ ...so the row does NOT read 'Custom' with ten matching sliders on screen"),
		OverallValue->GetText().ToString(), FString(TEXT("Custom")));
	TestEqual(TEXT("⭐⭐ ...it reads the level the ten visible groups actually agree on"),
		OverallValue->GetText().ToString(),
		USiegeGraphicsSettingsSubsystem::GetQualityLevelDisplayName(2).ToString());

	// ── AND A REAL DISAGREEMENT AMONG THE VISIBLE TEN *DOES* READ Custom ─────
	// Without this the test above would also pass on a panel that never says
	// Custom at all, which would be a different lie (SC-§99).
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_TextureQuality, 0);
	TestTrue(TEXT("⭐ A genuine disagreement among the VISIBLE ten IS Custom"),
		USiegeGraphicsMenuWidget::IsCustomForDisplay(Graphics));

	Panel->RefreshAllRows();
	TestEqual(TEXT("⭐ ...and the row says so"), OverallValue->GetText().ToString(), FString(TEXT("Custom")));

	// A null subsystem is never "Custom" — it is nothing at all.
	TestFalse(TEXT("A null subsystem does not report Custom"),
		USiegeGraphicsMenuWidget::IsCustomForDisplay(nullptr));

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 9 — board cl. (5) / GFX-§9: the "next match start" notice
// ════════════════════════════════════════════════════════════════════════════

/**
 *  ⚠️ THIS TEST PINS A DELIBERATE DEVIATION FROM BOARD cl. (5)'s WORDING, and it
 *  is flagged for TASK-1116 in the handoff. cl. (5) names "Foliage, View Distance
 *  and Effects"; that list predates the GFX-§9 correction of 2026-09-07, which
 *  STRUCK the View-Distance and fog levers. Saying "applies next match" on
 *  ViewDistanceQuality would UNDERSELL a control the engine already moves this
 *  frame (r.ViewDistanceScale scales HISM cull distances immediately), and
 *  EffectsQuality has no scatter-driven lever at all. FoliageQuality is the one
 *  survivor — its cull band is set inside RunScatterPasses, once per match.
 *  SC-§101: a prescribed remedy is a claim, and this one was measured instead.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuNextMatchNoticeTest,
	"Siegebound.GraphicsMenu.NextMatchStartNoticeIsScatterDrivenOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuNextMatchNoticeTest::RunTest(const FString& Parameters)
{
	int32 DeferredCount = 0;
	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (USiegeGraphicsMenuWidget::DoesGroupApplyAtNextMatchStart(GroupName))
		{
			++DeferredCount;
			TestEqual(TEXT("⭐ The ONLY deferred group is Foliage — it is the one surviving Tier-D lever"),
				GroupName, USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality);
		}
	}
	TestEqual(TEXT("⭐ Exactly one group carries the next-match-start notice"), DeferredCount, 1);

	// The notice must actually SAY the thing. A control that silently does nothing
	// until later is worse than one that admits it (GFX-§9), and the admission is
	// the deliverable.
	const FString FoliageHint = USiegeGraphicsMenuWidget::ComposeGroupHint(
		USiegeGraphicsSettingsSubsystem::GroupName_FoliageQuality, /*bVolumetricFogEnabled*/ true);
	TestFalse(TEXT("⭐ The Foliage row HAS a hint line"), FoliageHint.IsEmpty());
	TestTrue(TEXT("⭐⭐ ...and it says the change lands at the NEXT match (GFX-§9's wording duty)"),
		FoliageHint.Contains(TEXT("NEXT match")));

	// Silence everywhere it would be false.
	TestTrue(TEXT("⛔ View Distance carries NO deferral notice — its engine CVar is immediate"),
		USiegeGraphicsMenuWidget::ComposeGroupHint(
			USiegeGraphicsSettingsSubsystem::GroupName_ViewDistanceQuality, true).IsEmpty());
	TestTrue(TEXT("⛔ Effects carries none either — the fog lever GFX-§9 once bound here was struck"),
		USiegeGraphicsMenuWidget::ComposeGroupHint(
			USiegeGraphicsSettingsSubsystem::GroupName_EffectsQuality, true).IsEmpty());
	TestTrue(TEXT("An unknown group name yields no hint rather than asserting"),
		USiegeGraphicsMenuWidget::ComposeGroupHint(FName(TEXT("LandscapeQuality")), true).IsEmpty());

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 10 — GFX-§9's ruled consumer: the Shadow row states the fog coupling
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuFogHintTest,
	"Siegebound.GraphicsMenu.ShadowHintTracksVolumetricFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuFogHintTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	const FName Shadows = USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality;

	const FString FogOn  = USiegeGraphicsMenuWidget::ComposeGroupHint(Shadows, /*bVolumetricFogEnabled*/ true);
	const FString FogOff = USiegeGraphicsMenuWidget::ComposeGroupHint(Shadows, /*bVolumetricFogEnabled*/ false);

	TestFalse(TEXT("The Shadows row carries a hint"), FogOn.IsEmpty());
	TestNotEqual(TEXT("⭐ ...and it CHANGES with the live fog state — a constant string would be a decoration, not a readout"),
		FogOn, FogOff);
	TestTrue(TEXT("Both wordings name the Low/Medium boundary the ENGINE owns"),
		FogOn.Contains(TEXT("Low and Medium")) && FogOff.Contains(TEXT("Low and Medium")));

	// ⭐ AND IT IS A REAL CALLER (SC-§36.1). ShouldEnableVolumetricFog() shipped in
	// TASK-1113 with no consumer; GFX-§9 rules THIS PANEL its read-only one, so a
	// refresh must actually put its answer on screen.
	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	UTextBlock* Hint = FindPinned<UTextBlock>(Scratch, TEXT("ShadowQualityHintText"));
	if (Hint == nullptr)
	{
		AddError(TEXT("ShadowQualityHintText is missing — GFX-§9's ruled consumer is absent."));
		return false;
	}

	Graphics->SetQualityGroupLevel(Shadows, 0);
	Scratch.Panel->RefreshAllRows();
	TestFalse(TEXT("FIXTURE SELF-CHECK: the facade says fog is OFF at Shadows=Low"), Graphics->ShouldEnableVolumetricFog());
	TestEqual(TEXT("⭐⭐ The panel's line reports the facade's answer at Shadows=Low"),
		Hint->GetText().ToString(), FogOff);

	Graphics->SetQualityGroupLevel(Shadows, 3);
	Scratch.Panel->RefreshAllRows();
	TestTrue(TEXT("FIXTURE SELF-CHECK: the facade says fog is ON at Shadows=Epic"), Graphics->ShouldEnableVolumetricFog());
	TestEqual(TEXT("⭐⭐ ...and the line follows it to Epic"), Hint->GetText().ToString(), FogOn);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 11 — stepper arithmetic
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuStepperTest,
	"Siegebound.GraphicsMenu.StepperIndexWrapsBothWays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuStepperTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Forward"), USiegeGraphicsMenuWidget::StepIndex(0, 1, 8), 1);
	TestEqual(TEXT("Backward"), USiegeGraphicsMenuWidget::StepIndex(5, -1, 8), 4);

	// ⭐ (-1 % 8) IS -1 IN C++, so the naive modulo hands a stepper a NEGATIVE
	// index and every "<" press at the start of a list becomes an out-of-range
	// refusal the player experiences as a dead button.
	TestEqual(TEXT("⭐ Stepping back from 0 wraps to the LAST entry, not to -1"),
		USiegeGraphicsMenuWidget::StepIndex(0, -1, 8), 7);
	TestEqual(TEXT("Stepping forward off the end wraps to 0"),
		USiegeGraphicsMenuWidget::StepIndex(7, 1, 8), 0);

	TestEqual(TEXT("A one-entry list is inert in both directions"),
		USiegeGraphicsMenuWidget::StepIndex(0, 1, 1), 0);
	TestEqual(TEXT("A one-entry list is inert in both directions"),
		USiegeGraphicsMenuWidget::StepIndex(0, -1, 1), 0);

	// ⛔ Never a divide by zero. The facade guarantees >= 1 for the resolution
	// list, so this is a floor, not an expected path.
	TestEqual(TEXT("⛔ An empty set returns 0 rather than dividing by zero"),
		USiegeGraphicsMenuWidget::StepIndex(3, 1, 0), 0);

	// The three window modes wrap over exactly three entries.
	TestEqual(TEXT("Window mode wraps over EWindowMode::NumWindowModes"),
		USiegeGraphicsMenuWidget::StepIndex(2, 1, USiegeGraphicsSettingsSubsystem::GetWindowModeCount()), 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 12 — a missing subsystem disables everything EXCEPT the way out
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuUnavailableTest,
	"Siegebound.GraphicsMenu.NullSubsystemDisablesEveryControlButBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuUnavailableTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	// ⛔ NO INJECTION, and a bare NewObject widget has no world ⇒
	// ResolveGraphicsSubsystem() returns null on every path. This is the one state
	// where a missing null check is a CRASH rather than a wrong pixel.
	FScratchPanel Scratch = MakeScratchPanel(/*bInjectSubsystem*/ false);
	if (!Scratch.Panel.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	for (const FName& GroupName : USiegeGraphicsSettingsSubsystem::GetQualityGroupNames())
	{
		if (USlider* Slider = Panel->FindGroupSlider(GroupName))
		{
			TestFalse(*FString::Printf(TEXT("%s is disabled with no subsystem"), *GroupName.ToString()),
				Slider->GetIsEnabled());
		}
	}

	if (USlider* Scale = FindPinned<USlider>(Scratch, TEXT("ResolutionScaleSlider")))
	{
		TestFalse(TEXT("The resolution scale is disabled"), Scale->GetIsEnabled());
	}
	if (UButton* AutoDetect = FindPinned<UButton>(Scratch, TEXT("AutoDetectButton")))
	{
		TestFalse(TEXT("Auto-Detect is disabled"), AutoDetect->GetIsEnabled());
	}

	// ⛔ THE EXCEPTION, AND IT IS THE POINT: a panel you cannot leave is worse than
	// a panel that cannot change anything — and this one sits on a hit-test
	// VISIBLE modal backdrop, so without Back the only way out is to quit.
	if (UButton* Back = FindPinned<UButton>(Scratch, TEXT("BackButton")))
	{
		TestTrue(TEXT("⭐ Back is STILL ENABLED — the backdrop is modal, so a dead Back is a trap"),
			Back->GetIsEnabled());
	}

	if (UTextBlock* Status = FindPinned<UTextBlock>(Scratch, TEXT("StatusText")))
	{
		TestTrue(TEXT("⭐ The panel SAYS why it is dead rather than looking broken"),
			Status->GetText().ToString().Contains(TEXT("unavailable")));
	}

	// Every click path is exercised with no subsystem: none may crash, and none
	// may leave a value behind.
	Panel->AutoDetectPressed();
	Panel->HandleGroupSliderCommitted();
	Panel->HandleOverallSliderCommitted();
	Panel->HandleResolutionScaleCommitted();
	Panel->HandleScreenResolutionNextClicked();
	Panel->HandleWindowModePrevClicked();
	Panel->HandleFrameRateLimitNextClicked();
	Panel->HandleVSyncChanged(true);
	Panel->RefreshAllRows();
	Panel->BackPressed();

	TestTrue(TEXT("⭐ Every control path survives a null subsystem without crashing"), true);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TEST 13 — TASK-1118 board cl. (2) + (5): THE EXPIRY REVERTS, ALONE
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE ONE PATH THE PLAYER THIS FEATURE PROTECTS CANNOT REACH BY CHOICE.
 *  They picked a mode their monitor cannot display; they can see NOTHING; they
 *  will not find the Revert button because they cannot find the screen. The
 *  timer is the entire recovery, so this test drives the ten ticks and asserts
 *  the revert lands with no click, no hover and no visible widget involved.
 *
 *  ⛔ POSITIVE CONTROL FIRST (SC-§99 / SC-§39): before asserting the countdown
 *  releases the soft-lock, the test proves the soft-lock is REAL — otherwise a
 *  green here would be worth nothing on a build where nothing was ever broken.
 *  ⛔ AND THE INSTRUMENT IS RefusedSaveWhileVideoModePendingCount, NOT the bool
 *  AutoDetectQuality() returns: under the automation seam that bool is false on
 *  a broken build AND a fixed one (the suppression branch), which is the exact
 *  blind spot qa/TASK-1114.md caught. Only the counter discriminates.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCountdownExpiryTest,
	"Siegebound.GraphicsMenu.VideoModeCountdownExpiryReverts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCountdownExpiryTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Scratch);

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	Graphics->ResetDiagnosticCountersForAutomationTests();

	// ── STEP 1: one ordinary stepper press ───────────────────────────────────
	const int32 ModeBefore = PressWindowModeNext(Scratch);

	TestNotEqual(TEXT("Fixture self-check: the press actually moved the staged window mode"),
		Graphics->GetWindowMode(), ModeBefore);
	TestEqual(TEXT("⭐⭐ TASK-1118 cl. (1): the display change was APPLIED PROVISIONALLY — the seam TASK-1115 left is filled"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestTrue(TEXT("⭐⭐ ...and the 10-second confirmation is ARMED"), Panel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐ ...starting at GFX-§4's ten seconds, not a taste"),
		Panel->GetVideoModeCountdownSecondsRemaining(), USiegeGraphicsMenuWidget::VideoModeConfirmSeconds, 0.001f);

	// ── STEP 2: ⛔ GFX-§4 cl. (4) — NOTHING REACHED DISK ─────────────────────
	// A saved-then-unviewable mode is the permanent lockout itself, so this
	// assertion is the whole of clause (4) and it holds for the entire window.
	TestEqual(TEXT("⭐⭐ GFX-§4 cl. (4): the provisional apply SAVED NOTHING"),
		Graphics->SaveSettingsCallCount, 0);

	// ── STEP 3: THE POSITIVE CONTROL — the soft-lock is REAL right now ───────
	TestTrue(TEXT("POSITIVE CONTROL: while the confirmation is outstanding, a video-mode change IS pending"),
		Graphics->IsVideoModeChangePending());
	TestFalse(TEXT("POSITIVE CONTROL: ...Auto-Detect is REFUSED"), Graphics->AutoDetectQuality());
	TestEqual(TEXT("POSITIVE CONTROL: ...and the refusal is COUNTED (the refusal branch, not the suppression branch)"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 1);

	// ── STEP 4: NINE TICKS — it must NOT fire early ──────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	for (int32 Tick = 0; Tick < 9; ++Tick)
	{
		Panel->TickVideoModeCountdown();
	}

	TestTrue(TEXT("⭐ After nine of ten ticks the countdown is STILL RUNNING — it does not fire early"),
		Panel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐ ...with exactly one second left"),
		Panel->GetVideoModeCountdownSecondsRemaining(), 1.0f, 0.001f);
	TestEqual(TEXT("⭐ ...and nothing has been reverted yet"), Graphics->ApplyResolutionSettingsCallCount, 0);
	TestEqual(TEXT("⭐ ...and STILL nothing saved, nine seconds in"), Graphics->SaveSettingsCallCount, 0);

	// ── STEP 5: THE TENTH TICK — ⛔ THIS IS THE ROW ──────────────────────────
	Panel->TickVideoModeCountdown();

	TestFalse(TEXT("⭐⭐⭐ board cl. (2): THE COUNTDOWN EXPIRED WITH NO PLAYER INPUT AND REVERTED — no video-mode change is pending"),
		Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("⭐⭐⭐ ...the mode the player could not see is GONE: the window mode is back to what it was"),
		Graphics->GetWindowMode(), ModeBefore);
	TestEqual(TEXT("⭐⭐ ...and the revert reached the display EXACTLY ONCE (cl. 9: the facade's revert applies; this class adds no second apply)"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestFalse(TEXT("⭐⭐ ...the prompt is gone with it"), Panel->IsVideoModeCountdownActive());

	// ── STEP 6: BOTH REFUSALS RELEASED (cl. 8-WIDENED's criterion) ───────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Graphics->AutoDetectQuality();
	TestEqual(TEXT("⭐⭐ Auto-Detect is no longer refused — the counter did not move"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 0);

	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 1);
	TestTrue(TEXT("⭐⭐ ...and a quality change SAVES again"), Graphics->SaveSettingsCallCount >= 1);

	// ── STEP 7: A STRANDED TICK AFTER EXPIRY IS INERT ───────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->TickVideoModeCountdown();
	Panel->TickVideoModeCountdown();
	TestEqual(TEXT("⭐ Ticks arriving after the countdown ended revert NOTHING"),
		Graphics->ApplyResolutionSettingsCallCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TEST 14 — board cl. (1) + (4): KEEP PERSISTS, AND ONLY THEN DOES IT SAVE
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuKeepTest,
	"Siegebound.GraphicsMenu.KeepPersistsTheModeAndSavesOnlyThen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuKeepTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Scratch);

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	Graphics->ResetDiagnosticCountersForAutomationTests();
	const int32 ModeBefore = PressWindowModeNext(Scratch);
	const int32 ChosenMode = Graphics->GetWindowMode();

	// ⛔ THE CLAUSE (4) ASSERTION, MADE BEFORE THE ANSWER: not one byte may reach
	// disk while the question is still open.
	TestEqual(TEXT("⭐⭐ GFX-§4 cl. (4): ZERO saves while the confirmation is outstanding"),
		Graphics->SaveSettingsCallCount, 0);
	TestTrue(TEXT("Fixture self-check: the countdown is armed"), Panel->IsVideoModeCountdownActive());

	// ── THE PLAYER PRESSES Keep ──────────────────────────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->KeepSettingsPressed();

	TestEqual(TEXT("⭐⭐ Keep PERSISTS the chosen mode — it is not rolled back"),
		Graphics->GetWindowMode(), ChosenMode);
	TestNotEqual(TEXT("⭐ ...which really is different from where the player started"),
		Graphics->GetWindowMode(), ModeBefore);
	TestFalse(TEXT("⭐⭐ ...the confirmation window is CLOSED"), Graphics->IsVideoModeChangePending());
	TestFalse(TEXT("⭐⭐ ...the prompt and its timer are gone"), Panel->IsVideoModeCountdownActive());
	TestTrue(TEXT("⭐⭐ ...and ONLY NOW does the mode reach disk"), Graphics->SaveSettingsCallCount >= 1);
	TestEqual(TEXT("⭐⭐ ...with NO further display apply — Keep confirms, it does not re-apply a mode already on screen"),
		Graphics->ApplyResolutionSettingsCallCount, 0);

	// ── A STRANDED TICK AFTER Keep MUST NOT UNDO THE PLAYER'S CHOICE ─────────
	// ⛔ THIS IS THE re-entrancy GUARD, AND IT IS THE WORST BUG THIS ROW COULD
	// SHIP: a timer callback already queued when Keep was pressed, reverting the
	// mode the player just chose to keep.
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->TickVideoModeCountdown();
	TestEqual(TEXT("⭐⭐⭐ A timer tick arriving AFTER Keep reverts NOTHING"),
		Graphics->ApplyResolutionSettingsCallCount, 0);
	TestEqual(TEXT("⭐⭐⭐ ...and the player still has the mode they kept"),
		Graphics->GetWindowMode(), ChosenMode);

	// ── AND Keep WITH NOTHING PENDING IS NOT A SPURIOUS SAVE ────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->KeepSettingsPressed();
	TestEqual(TEXT("⭐ Keep with nothing pending saves nothing (it is BlueprintCallable — a WBP could reach it)"),
		Graphics->SaveSettingsCallCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TEST 15 — board cl. (1): REVERT RESTORES THE PRIOR MODE, EVEN AFTER TWO
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuRevertButtonTest,
	"Siegebound.GraphicsMenu.RevertRestoresTheModeBeforeAnyChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuRevertButtonTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Scratch);

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	const int32 OriginalMode = Graphics->GetWindowMode();

	// ⛔ TWO PRESSES, AND THE SECOND ONE IS THE POINT. A provisional apply does
	// NOT move LastConfirmed*, so "the prior mode" is where the player was BEFORE
	// they started pressing — not the intermediate mode they passed through. A
	// revert that restored the intermediate would leave them somewhere they never
	// chose, which reads as the revert being broken.
	Panel->HandleWindowModeNextClicked();
	const int32 IntermediateMode = Graphics->GetWindowMode();
	Panel->HandleWindowModeNextClicked();

	TestNotEqual(TEXT("Fixture self-check: two presses landed somewhere else again"),
		Graphics->GetWindowMode(), IntermediateMode);
	TestTrue(TEXT("Fixture self-check: still one countdown, restarted rather than stacked"),
		Panel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐ A second display change RESTARTS the ten seconds — the player is not punished for pressing twice"),
		Panel->GetVideoModeCountdownSecondsRemaining(), USiegeGraphicsMenuWidget::VideoModeConfirmSeconds, 0.001f);

	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->RevertSettingsPressed();

	TestEqual(TEXT("⭐⭐ Revert restores the mode the player started in, NOT the one they passed through"),
		Graphics->GetWindowMode(), OriginalMode);
	TestFalse(TEXT("⭐⭐ ...the window is closed"), Graphics->IsVideoModeChangePending());
	TestFalse(TEXT("⭐⭐ ...the countdown is disarmed"), Panel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐⭐ cl. (9): EXACTLY ONE display apply on the revert — the facade's own; this class adds no second"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestEqual(TEXT("⭐ ...and nothing was refused on the way out"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TEST 16 — board cl. (8): Back DURING A LIVE COUNTDOWN, STILL ONE REVERT
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 THE RECONCILIATION, ASSERTED RATHER THAN ARGUED. TASK-1115 landed the
 *  Back-path discard first; TASK-1118 landed second and owns the seam between
 *  them. The invariant is board cl. (8)'s: EXACTLY ONE revert on the Back path,
 *  never two — a double revert re-applies a mode nobody asked for.
 *
 *  What TASK-1118 added to that path is a DISARM, which reverts nothing. This
 *  test proves it by counting the applies AND by then firing the timer by hand:
 *  if the disarm were missing, a stranded tick would revert a second time.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuBackDuringCountdownTest,
	"Siegebound.GraphicsMenu.BackDuringCountdownRevertsExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuBackDuringCountdownTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Scratch);

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	const int32 ModeBefore = PressWindowModeNext(Scratch);
	TestTrue(TEXT("Fixture self-check: a countdown is live"), Panel->IsVideoModeCountdownActive());

	// ── POSITIVE CONTROL: the soft-lock cl. (8-WIDENED) names is real ────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	TestFalse(TEXT("POSITIVE CONTROL: Auto-Detect is refused while the confirmation is open"),
		Graphics->AutoDetectQuality());
	TestEqual(TEXT("POSITIVE CONTROL: ...and counted"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 1);

	// ── THE PLAYER PRESSES Back MID-COUNTDOWN ────────────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->BackPressed();

	TestEqual(TEXT("⭐⭐⭐ board cl. (8): Back issued EXACTLY ONE revert — not two, with the countdown's own revert path also present"),
		Graphics->ApplyResolutionSettingsCallCount, 1);
	TestFalse(TEXT("⭐⭐ ...nothing is pending after Back"), Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("⭐⭐ ...and the player is back on the mode they started in"),
		Graphics->GetWindowMode(), ModeBefore);
	TestFalse(TEXT("⭐⭐ ...with the countdown DISARMED, so no timer outlives the panel"),
		Panel->IsVideoModeCountdownActive());
	// ⛔ gate qa/TASK-1119.md WARN-1's SECOND ROW, and it is here because the
	// stranded-tick block below CANNOT catch a missing disarm (see the mutation
	// table, clause (ii)): the disarm ZEROES the remainder as well as lowering the
	// flag, so this reads 10.0 the moment BackPressed() forgets to call it.
	TestEqual(TEXT("⭐⭐ ...and the disarm ZEROED the remainder too, so nothing is left counting toward an expiry the player already walked away from"),
		Panel->GetVideoModeCountdownSecondsRemaining(), 0.0f, 0.001f);

	// ── A TICK THAT ARRIVES AFTER Back MUST DO NOTHING ──────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->TickVideoModeCountdown();
	Panel->TickVideoModeCountdown();
	TestEqual(TEXT("⭐⭐⭐ A stranded timer callback after Back reverts NOTHING — that would be the second revert cl. (8) forbids"),
		Graphics->ApplyResolutionSettingsCallCount, 0);

	// ── THE SECOND TEARDOWN CALL SITE IS STILL A GUARDED NO-OP ──────────────
	TestFalse(TEXT("⭐⭐ NativeDestruct's discard finds the window closed and reverts nothing (the TASK-1115 arithmetic, unchanged)"),
		USiegeGraphicsMenuWidget::DiscardStagedVideoMode(Graphics));
	TestEqual(TEXT("⭐⭐ ...still exactly one revert on the whole Back path"),
		Graphics->ApplyResolutionSettingsCallCount, 0);

	// ── BOTH REFUSALS RELEASED ───────────────────────────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Graphics->AutoDetectQuality();
	TestEqual(TEXT("⭐⭐ cl. (8-WIDENED)'s QA criterion: after a staged-then-abandoned mode the refusal counter is STILL 0"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 0);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 2);
	TestTrue(TEXT("⭐⭐ ...and a quality save works again"), Graphics->SaveSettingsCallCount >= 1);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TEST 17 — THE NO-TIMER FAIL-SAFE, AND board cl. (3): QUALITY NEVER ARMS
// ════════════════════════════════════════════════════════════════════════════

/**
 *  🚨 TWO PROPERTIES, BOTH ABOUT THINGS THAT MUST **NOT** HAPPEN.
 *
 *  (a) ⛔ NO TIMER ⇒ NO PROVISIONAL APPLY. A display change applied where it
 *      cannot be auto-reverted IS GFX-§4's permanent lockout, arriving through
 *      the door nobody watches. This test is the ONLY one that leaves the
 *      automation seam OFF, so the panel really has no world and really has no
 *      timer manager — the production branch, exercised for real.
 *  (b) ⛔ board cl. (3): a QUALITY change never arms a confirmation. A confirm
 *      prompt on every slider drag would make the panel unusable, which is the
 *      opposite of GFX-§7's learnability requirement.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCountdownScopeTest,
	"Siegebound.GraphicsMenu.NoTimerNoApplyAndQualityNeverArms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCountdownScopeTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	// ── (a) THE FAIL-SAFE. ⛔ SEAM DELIBERATELY LEFT OFF ─────────────────────
	TestNull(TEXT("Fixture self-check: a bare NewObject panel really has NO WORLD, so there is really no timer manager"),
		Panel->GetWorld());

	Graphics->ResetDiagnosticCountersForAutomationTests();
	const int32 ModeBefore = PressWindowModeNext(Scratch);

	TestEqual(TEXT("⭐⭐⭐ With no way to auto-revert, the display change was NOT APPLIED — staged-only is recoverable, applied-and-unrevertable is the lockout"),
		Graphics->ApplyResolutionSettingsCallCount, 0);
	TestFalse(TEXT("⭐⭐ ...and no countdown was armed for a change that never happened"),
		Panel->IsVideoModeCountdownActive());
	TestTrue(TEXT("⭐ ...the change IS staged, which is TASK-1115's shipped, safe interim"),
		Graphics->IsVideoModeChangePending());

	// ...and the shipped Back path still releases it, so the fail-safe cannot
	// soft-lock the settings screen either.
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->BackPressed();
	TestFalse(TEXT("⭐⭐ ...and Back still discards it, so the fail-safe cannot soft-lock the panel"),
		Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("⭐ ...returning the player to the mode they started in"), Graphics->GetWindowMode(), ModeBefore);

	// ── (b) QUALITY NEVER ARMS ───────────────────────────────────────────────
	FScratchPanel Second = MakeScratchPanel();
	if (!Second.IsValid())
	{
		AddError(TEXT("Could not build the second scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Second);

	USiegeGraphicsSettingsSubsystem* QualityGraphics = Second.Graphics.Get();
	USiegeGraphicsMenuWidget* QualityPanel = Second.Panel.Get();

	QualityGraphics->ResetDiagnosticCountersForAutomationTests();

	// Move a group slider and commit it the way the player does.
	if (USlider* Shadows = QualityPanel->FindGroupSlider(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality))
	{
		const int32 CurrentLevel = QualityGraphics->GetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality);
		const int32 TargetLevel = (CurrentLevel == 0) ? 1 : 0;
		Shadows->SetValue(USiegeGraphicsMenuWidget::LevelToSliderValue(TargetLevel));
		QualityPanel->HandleGroupSliderCommitted();

		TestEqual(TEXT("Fixture self-check: the quality group really moved"),
			QualityGraphics->GetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality), TargetLevel);
	}

	TestFalse(TEXT("⭐⭐ board cl. (3): a QUALITY change arms NO confirmation — a prompt on every drag would make the panel unusable"),
		QualityPanel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐⭐ ...and reaches NO provisional display apply"),
		QualityGraphics->ApplyResolutionSettingsCallCount, 0);
	TestTrue(TEXT("⭐⭐ ...and SAVES IMMEDIATELY, because a quality setting cannot soft-lock anyone (GFX-§4)"),
		QualityGraphics->SaveSettingsCallCount >= 1);

	// The frame-rate stepper is the third Tier-C control and is deliberately NOT
	// a display-mode change — it cannot make the screen unreadable.
	QualityGraphics->ResetDiagnosticCountersForAutomationTests();
	QualityPanel->HandleFrameRateLimitNextClicked();
	TestFalse(TEXT("⭐ ...and neither does the frame-rate ladder: a frame cap cannot make a screen unreadable"),
		QualityPanel->IsVideoModeCountdownActive());
	TestEqual(TEXT("⭐ ...it reaches no provisional display apply either"),
		QualityGraphics->ApplyResolutionSettingsCallCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  ⭐ TEST 18 — STEPPING BACK TO THE STARTING MODE CANCELS THE QUESTION
// ════════════════════════════════════════════════════════════════════════════

/**
 *  A player who presses ">" until the value wraps all the way round arrives back
 *  at the mode they started in. There is then nothing to confirm — and a prompt
 *  left on screen would offer "Revert" for a change that no longer exists and
 *  "Keep" for a mode that was never left.
 *
 *  🚨 THE FIRST WRITING OF THIS TEST ASSERTED A BRANCH THAT COULD NOT RUN, AND
 *  THAT IS WHY IT IS WORTH READING TWICE (gate qa/TASK-1119.md BLOCKER-1). The
 *  panel asked IsVideoModeChangePending(), which is
 *  `bVideoModeChangePending || <staged differs from confirmed>` — an `||` with a
 *  LATCH on the left that the FIRST provisional apply raises and only
 *  Confirm/Revert lowers. So from press two onward the cancel branch was dead:
 *  the wrapping press re-applied the mode the player was already on and
 *  RESTARTED the ten seconds. Three artefacts — a comment, a handoff and this
 *  test — all agreed with each other and none agreed with the code, because all
 *  three were written from the same reading of a name.
 *
 *  ⛔ SO THIS TEST NOW PINS BOTH HALVES OF THE FIX, and the second half is the
 *  one a "just disarm it" patch would get wrong:
 *    (1) the countdown is CANCELLED (the branch is reached at all), and
 *    (2) the facade's window is CLOSED, so the save refusal is RELEASED. A
 *        disarm alone would hide the prompt and leave the latch up with nothing
 *        left to lower it ⇒ every save and Auto-Detect refused for the rest of
 *        the session, which is cl. (8-WIDENED)'s soft-lock through a new door.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCountdownCancelTest,
	"Siegebound.GraphicsMenu.SteppingBackToTheStartingModeCancelsTheCountdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCountdownCancelTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}
	DriveCountdownManually(Scratch);

	USiegeGraphicsSettingsSubsystem* Graphics = Scratch.Graphics.Get();
	USiegeGraphicsMenuWidget* Panel = Scratch.Panel.Get();

	const int32 ModeCount = USiegeGraphicsSettingsSubsystem::GetWindowModeCount();
	const int32 OriginalMode = Graphics->GetWindowMode();

	Graphics->ResetDiagnosticCountersForAutomationTests();

	// ── THE PRESSES BEFORE THE WRAP — an ordinary, LIVE countdown ────────────
	// ⛔ POSITIVE CONTROL FIRST. Without this block, "no countdown is active"
	// below would also pass on a panel that never armed one at all.
	for (int32 Press = 0; Press < ModeCount - 1; ++Press)
	{
		Panel->HandleWindowModeNextClicked();
	}

	TestNotEqual(TEXT("Fixture self-check: the presses before the wrap really left the starting mode"),
		Graphics->GetWindowMode(), OriginalMode);
	TestTrue(TEXT("POSITIVE CONTROL: a countdown IS live before the wrapping press"),
		Panel->IsVideoModeCountdownActive());
	TestTrue(TEXT("POSITIVE CONTROL: ...and the DIFFERENCE the branch reads is real right now"),
		Graphics->HasUnconfirmedVideoModeDifference());
	TestEqual(TEXT("⭐ GFX-§4 cl. (4): nothing has reached disk while the confirmation is open"),
		Graphics->SaveSettingsCallCount, 0);
	TestFalse(TEXT("POSITIVE CONTROL: ...and the save refusal (the soft-lock) is real right now"),
		Graphics->AutoDetectQuality());
	TestEqual(TEXT("POSITIVE CONTROL: ...and counted, on the refusal branch and not the suppression branch"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 1);
	TestEqual(TEXT("Fixture self-check: one provisional apply per real press so far"),
		Graphics->ApplyResolutionSettingsCallCount, ModeCount - 1);

	// ── THE WRAPPING PRESS — back to the mode the player started in ──────────
	// ⭐ TASK-1124 cl. (5c): snapshot the save count BEFORE the wrap, so the
	// flush the wrap-around's CloseVideoModeWindow performs can be pinned as a
	// DIFFERENCE rather than a total. A bare ">= 1" would be satisfied by any
	// earlier save and would therefore be green on both branches (SC-§104).
	const int32 SavesBeforeWrap = Graphics->SaveSettingsCallCount;

	Panel->HandleWindowModeNextClicked();

	TestEqual(TEXT("Fixture self-check: wrapping all the way round returns the staged mode to where it started"),
		Graphics->GetWindowMode(), OriginalMode);
	TestFalse(TEXT("⭐⭐⭐ BLOCKER-1: the branch reads the DIFFERENCE, not the LATCH — so the countdown is CANCELLED instead of re-armed over a mode the player never left"),
		Panel->IsVideoModeCountdownActive());
	TestFalse(TEXT("⭐⭐ ...with no difference left for the branch to read"),
		Graphics->HasUnconfirmedVideoModeDifference());
	TestEqual(TEXT("⭐ ...and the remainder is zeroed with it, so nothing is left counting toward an expiry nobody is waiting for"),
		Panel->GetVideoModeCountdownSecondsRemaining(), 0.0f, 0.001f);
	TestFalse(TEXT("⭐⭐⭐ ...AND THE FACADE'S WINDOW IS CLOSED: a disarm alone would hide the prompt and leave the latch up, refusing every save for the rest of the session"),
		Graphics->IsVideoModeChangePending());
	TestEqual(TEXT("⭐ ...and the wrapping press spends ONE apply — the closing revert's, NOT a fresh provisional one (ModeCount-1 real presses + 1 revert)"),
		Graphics->ApplyResolutionSettingsCallCount, ModeCount);

	// ── THE SOFT-LOCK IS RELEASED (cl. 8-WIDENED's criterion) ───────────────
	// ⚠️ The close flushes a save, exactly as Revert and Back already do — and
	// what it writes is the LAST CONFIRMED mode, which is the one the player is
	// looking at. GFX-§4 cl. (4) forbids saving an UNCONFIRMED mode; there is no
	// unconfirmed mode left here, which is the whole reason this branch fired.
	//
	// ⭐⭐ TASK-1124 cl. (5c) — THE ASSERTION THAT HAD TO GO ABOVE THE RESET.
	// The flush is CORRECT and REQUIRED, and until this line NOTHING pinned it:
	// the reset on the next statement erases the evidence, so every later row
	// measures a counter that starts at zero. Placed here, and here only.
	// ⛔ MEASURED BY TASK-1124, NOT DERIVED: without this line M22 reddens FOUR
	// rows; with it, FIVE. That is the whole reason it exists.
	TestTrue(TEXT("⭐⭐ TASK-1124 (5c): the wrap-around's CloseVideoModeWindow FLUSHED the deferred save — delete the discard and this save never happens"),
		Graphics->SaveSettingsCallCount > SavesBeforeWrap);

	Graphics->ResetDiagnosticCountersForAutomationTests();
	Graphics->AutoDetectQuality();
	TestEqual(TEXT("⭐⭐ Auto-Detect is no longer refused — the wrap-around CLOSED the window instead of stranding it open"),
		Graphics->RefusedSaveWhileVideoModePendingCount, 0);
	Graphics->SetQualityGroupLevel(USiegeGraphicsSettingsSubsystem::GroupName_ShadowQuality, 1);
	TestTrue(TEXT("⭐⭐ ...and a quality change SAVES again"), Graphics->SaveSettingsCallCount >= 1);
	TestEqual(TEXT("⭐⭐ ...on the mode the player started in, which is the one they can see"),
		Graphics->GetWindowMode(), OriginalMode);

	// ── AND A TICK QUEUED BEFORE THE WRAP IS INERT ──────────────────────────
	Graphics->ResetDiagnosticCountersForAutomationTests();
	Panel->TickVideoModeCountdown();
	Panel->TickVideoModeCountdown();
	TestEqual(TEXT("⭐ A tick queued before the wrapping press reverts NOTHING once the countdown is cancelled"),
		Graphics->ApplyResolutionSettingsCallCount, 0);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST 19 — the countdown's LINE: it names the number, and gets ONE right
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuCountdownTextTest,
	"Siegebound.GraphicsMenu.CountdownTextNamesTheNumberAndIsSingularAtOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuCountdownTextTest::RunTest(const FString& Parameters)
{
	const FString AtTen = USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(10);
	const FString AtTwo = USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(2);
	const FString AtOne = USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(1);
	const FString AtZero = USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(0);

	// GFX-§4 asks for a "Keep these settings?" prompt by name.
	TestTrue(TEXT("⭐ The prompt asks GFX-§4's question in GFX-§4's words"),
		AtTen.Contains(TEXT("Keep these settings?")));

	// ⛔ AND IT NAMES THE NUMBER. Without it a player who CAN see the screen has
	// no way to tell a working menu from a hung one.
	TestTrue(TEXT("⭐⭐ ...and it NAMES THE SECONDS LEFT, which is what makes it a countdown rather than a question"),
		AtTen.Contains(TEXT("10")));
	TestTrue(TEXT("⭐ ...and the number tracks"), AtTwo.Contains(TEXT("2")) && AtTwo.Contains(TEXT("seconds")));

	TestTrue(TEXT("⭐ Singular at one: '1 second'"), AtOne.Contains(TEXT("1 second")));
	TestFalse(TEXT("⭐ ...and never '1 seconds'"), AtOne.Contains(TEXT("1 seconds")));

	// At zero the revert is being issued in the same call stack.
	TestFalse(TEXT("⭐ At zero the line does not count into the negatives"), AtZero.Contains(TEXT("0")));
	TestTrue(TEXT("⭐ ...it says what is happening instead"), AtZero.Contains(TEXT("Reverting")));
	TestFalse(TEXT("⭐ A negative can only arrive from a bug, and still must not print a negative"),
		USiegeGraphicsMenuWidget::ComposeVideoModeCountdownText(-3).Contains(TEXT("-3")));

	return true;
}

/*
 *  ═════════════════════════════════════════════════════════════════════════
 *  SC-§83 — THE NAMED MUTATIONS. ⛔ NO WITNESSED RED: none of these was
 *  executed. Every row is a DERIVED PREDICTION, and TASK-1113's M4 is the
 *  standing proof that a prediction in a file like this can be confidently
 *  wrong. Re-point, do not trust.
 *
 *  ⭐ THE AUTHORED GUARDS ARE M1-M6; the rest are supporting.
 *
 *  M1  Delete `OutSlider->MouseUsesStep = bDetented;` in BuildSliderRow
 *      ⇒ RED: DetentSlidersSnapAndTheContinuousOneDoesNot (eleven assertions).
 *      ⚠️ NOTHING ELSE reddens — which is the point: the ten sliders would ship
 *      gliding smoothly through five states with every other test green.
 *  M2  Set MouseUsesStep = true unconditionally (i.e. also on the scale bar)
 *      ⇒ RED: the same test's final TestFalse, and ONLY that one.
 *  M3  Move the write from HandleResolutionScaleCommitted into
 *      HandleResolutionScaleValueChanged
 *      ⇒ RED: ResolutionScaleWritesOnCommitNotOnValueChange, the
 *      "FIFTY drag frames reached ZERO saves" assertion (it would read 50).
 *  M3b Bind the WRITE to OnValueChanged in SeedAndBind but leave the bodies
 *      alone ⇒ RED: the same test's BINDING half only. ⚠️ M3 and M3b are
 *      separate on purpose: either alone ships the fifty-writes defect, and a
 *      test that caught only one of them would look like it covered both.
 *  M4  Delete the IsVideoModeChangePending() guard from DiscardStagedVideoMode
 *      ⇒ RED: DiscardStagedVideoModeRevertsExactlyOnce (the second call would
 *      revert, ApplyResolutionSettingsCallCount reads 2). ⚠️ PREDICTED WEAKER
 *      VARIANT: the FIRST call's assertions all still pass, so only the last
 *      two rows redden — that is the "never two" half, and it is why the test
 *      asserts the counter twice rather than the bool twice.
 *  M5  Delete the DiscardStagedVideoMode call from BackPressed()
 *      ⇒ RED: BackReleasesAutoDetectAndEverySave, four assertions after "the
 *      player presses Back". ⚠️ NOT the positive-control block above it, which
 *      would stay green — that block is what proves the RED is about the fix.
 *  M6  Change IsCustomForDisplay to forward to IsOverallQualityCustom()
 *      ⇒ RED: CustomLabelIgnoresInvisibleInputs, the two ⭐⭐ assertions. The
 *      fixture self-check STAYS GREEN, which is how the reader tells "the panel
 *      regressed" from "the engine changed".
 *  M7  FMath::TruncToInt instead of RoundToInt in SliderValueToLevel
 *      ⇒ RED: LevelSliderValueRoundTrip (0.9999 -> 3), and probably also
 *      GroupCommitWritesOnlyTheMovedGroup.
 *  M8  Drop the `+ Count` from StepIndex ⇒ RED: StepperIndexWrapsBothWays.
 *  M9  Add ViewDistanceQuality to DoesGroupApplyAtNextMatchStart
 *      ⇒ RED: NextMatchStartNoticeIsScatterDrivenOnly (count 2, and the
 *      View-Distance emptiness assertion).
 *  M10 Make ComposeGroupHint return the same Shadow string regardless of
 *      bVolumetricFogEnabled ⇒ RED: ShadowHintTracksVolumetricFog's TestNotEqual
 *      and both ⭐⭐ rows. ⚠️ This is the mutation that turns a READOUT back into
 *      a DECORATION, which is the failure GFX-§9 named.
 *  M11 Set the backdrop to ESlateVisibility::HitTestInvisible
 *      ⇒ RED: TreeBuildsEveryPinnedRow's GFX-§2(f) assertion. ⛔ The live cost of
 *      this mutation is a click-through into QUIT, and no other test sees it.
 *  M12 Skip the group name derivation and hand-type one member wrong in
 *      ResolveGroupRow (e.g. ShadowQuality -> ReflectionQualitySlider)
 *      ⇒ RED: TreeBuildsEveryPinnedRow's pairing assertion AND
 *      GroupCommitWritesOnlyTheMovedGroup's "did NOT move" loop.
 *      ⚠️ The token-pasting macro makes this mutation require deleting the macro
 *      first — which is the whole reason it is a macro.
 *  M13 Enable nothing in ShowPanelUnavailable's Back exception (i.e. let Back be
 *      disabled with the rest) ⇒ ⛔ PREDICTED RED:
 *      NullSubsystemDisablesEveryControlButBack's ⭐ row. The live cost is a
 *      modal panel with no way out.
 *      🚨⛔ FALSIFIED BY EXECUTION — TASK-1124, 2026-09-08. ⛔ M13 IS A ZERO-RED:
 *      the whole suite stayed 552/0 with the re-enable deleted. ⛔ THE CAUSE IS NOT
 *      A WEAK ASSERTION, IT IS THAT BackButton IS NOT IN SetAllControlsEnabled'S
 *      Controls[] ARRAY AT ALL (SiegeGraphicsMenuWidget.cpp) — so Back is NEVER
 *      disabled, and "Back is still enabled" is TRUE ON BOTH BRANCHES.
 *      ⇒ SC-§104 cl. 5(a): an assertion equal on both branches measures nothing.
 *      ⛔ THE SHIPPED BEHAVIOUR IS CORRECT — the player really can always leave;
 *      what is wrong is the CLAIM that this pin proves it. The re-enable is
 *      defence-in-depth against a future edit that adds BackButton to Controls[],
 *      exactly like the scatter's BaseEndUU<=0 sentinel. ⛔ DO NOT DELETE IT, and
 *      ⛔ do not read this pin as covering it. Routed to the manager by TASK-1124.
 *
 *
 *  ═════════════════════════════════════════════════════════════════════════
 *  ⭐⭐ TASK-1118 [GFX-REVERT] — THE COUNTDOWN'S MUTATIONS (M14-M20)
 *
 *  ⛔ ADDENDUM B FORM: every prediction below is stated as "≥ N rows, INCLUDING
 *  <named rows>". A WIDER red set that contains the named rows is CONFIRMATION;
 *  a NARROWER one is a FINDING against the prediction, not against the code.
 *  That form is not decoration — four of TASK-1115's six predictions turned out
 *  wider than written (qa/TASK-1116.md NIT-3/4/5) and two in TASK-1113's table
 *  turned out narrower (M4, and M13 by execution).
 *  ⛔ NO WITNESSED RED: none of M14-M22 was executed. TASK-1124 holds the
 *  compiler.
 *
 *  ⚠️ FOUR OF THESE ROWS WERE RE-DERIVED BY THE GATE AND FOUR CAME BACK WRONG
 *  (qa/TASK-1119.md §3: M14 and M16 over-broad, M19 and M20 with NO RED AT ALL).
 *  They are corrected IN PLACE below rather than deleted, because a table with a
 *  known-false row is worse than no table — the next reader runs it, counts the
 *  "wrong" reds and doubts the CODE. ⛔ M20's absent red was not a bad
 *  prediction: it was BLOCKER-1 seen from the mutation side, and it is the one
 *  the loop-1 fix pays back.
 *  ═════════════════════════════════════════════════════════════════════════
 *
 *  M14 ⭐ Delete the Graphics->ApplyVideoModeProvisional() call from
 *      BeginVideoModeConfirmation (stage only, but STILL arm the countdown)
 *      ⇒ RED: ⛔ CORRECTED (gate WARN-2) — ≥ 1 row, and I expect EXACTLY one:
 *      VideoModeCountdownExpiryReverts' "APPLIED PROVISIONALLY" row (reads 0,
 *      expects 1).
 *      ⛔ THE ORIGINAL PREDICTION NAMED RevertRestoresTheModeBeforeAnyChange's
 *      "EXACTLY ONE display apply on the revert" AND THAT ROW STAYS GREEN. Every
 *      "exactly one apply" assertion in this file is preceded by a
 *      ResetDiagnosticCountersForAutomationTests(), so it measures the REVERT's
 *      own apply and is structurally blind to whether a provisional apply ever
 *      happened. ⚠️ COROLLARY WORTH MORE THAN THE CORRECTION: exactly ONE
 *      assertion in this file would notice the provisional apply going missing.
 *      ⚠️ LIVE COST: a menu that shows a ten-second countdown over a picture
 *      that never changed — the player answers a question about nothing.
 *
 *  M15 🚨⭐⭐⭐ THE ROW'S OWN MUTATION. In TickVideoModeCountdown, delete the
 *      RevertSettingsPressed() call at zero, leaving the label to count down to
 *      "Reverting now…" and stop there.
 *      ⇒ RED: ≥ 4 rows, ALL in VideoModeCountdownExpiryReverts, INCLUDING "THE
 *      COUNTDOWN EXPIRED WITH NO PLAYER INPUT AND REVERTED", "the window mode is
 *      back to what it was", "the revert reached the display EXACTLY ONCE" and
 *      "the prompt is gone with it". The nine-tick block ABOVE stays GREEN,
 *      which is what makes the red attributable to the expiry and not to the
 *      arithmetic.
 *      ⛔ THIS IS THE MUTATION THAT SHIPS THE PERMANENT LOCKOUT. Everything else
 *      in the panel still works; a player who picks an unviewable mode simply
 *      never gets it back, and no other test in either file sees it.
 *
 *  M16 ⭐⭐ Delete DisarmVideoModeCountdown() from BackPressed().
 *      ⇒ RED: ⛔ CORRECTED (gate WARN-1) — ≥ 2 rows in
 *      BackDuringCountdownRevertsExactlyOnce, and BOTH FIRE **IMMEDIATELY AFTER
 *      BackPressed()**, not after the by-hand ticks: "with the countdown DISARMED,
 *      so no timer outlives the panel" (the flag is never lowered) and "the
 *      disarm ZEROED the remainder too" (it reads 10.0, not 0.0 — the second row
 *      was ADDED in loop 1 precisely to widen this mutation honestly).
 *      ⛔ THE ORIGINAL PREDICTION WAS WRONG IN BOTH DIRECTIONS and the author
 *      asked to be told: it said the red would NOT appear until the by-hand ticks
 *      and named two stranded-tick rows. It appears immediately, and those two
 *      rows STAY GREEN — with 10.0 s left, two ticks reach 8.0 and never expire.
 *      ⛔ AND DRIVING TEN TICKS WOULD NOT HELP, which is the deeper finding: at
 *      zero the stranded tick calls RevertSettingsPressed → DiscardStagedVideoMode,
 *      whose IsVideoModeChangePending() guard finds the window ALREADY CLOSED by
 *      Back's own revert and returns false. The second revert cl. (8) forbids is
 *      prevented ONE LAYER DOWN, so no tick count can make those rows load-
 *      bearing against THIS mutation. See clause (ii) below.
 *
 *  M17 ⭐⭐ Make CanArmVideoModeCountdown() return true unconditionally (i.e.
 *      apply provisionally even where no timer manager exists).
 *      ⇒ RED: ≥ 2 rows in NoTimerNoApplyAndQualityNeverArms, INCLUDING "the
 *      display change was NOT APPLIED" and "no countdown was armed for a change
 *      that never happened".
 *      ⚠️ LIVE COST: the same permanent lockout as M15, through the other door —
 *      a mode on screen with nothing that will ever take it off.
 *
 *  M18 Reverse the two checks at the top of BeginVideoModeConfirmation, so
 *      ApplyVideoModeProvisional() runs BEFORE CanArmVideoModeCountdown().
 *      ⇒ RED: ≥ 1 row, INCLUDING NoTimerNoApplyAndQualityNeverArms' "NOT
 *      APPLIED" row. ⚠️ SEPARATE FROM M17 ON PURPOSE: M17 deletes the guard,
 *      M18 keeps it and merely moves it — and the second is the edit a future
 *      reader is far more likely to make while "tidying", because the function
 *      still LOOKS guarded.
 *
 *  M19 ⛔ RE-SPECIFIED (gate WARN-3) — THE ORIGINAL WAS INERT IN THIS HARNESS.
 *      It said: call BeginVideoModeConfirmation(Graphics) from
 *      HandleGroupSliderCommitted(). That mutation reddens NOTHING: on the second
 *      scratch panel nothing has been staged, so HasUnconfirmedVideoModeDifference()
 *      is false and the injected call takes the CANCEL BRANCH — it disarms,
 *      no-op-discards, logs Verbose and returns without applying or arming. The
 *      property is genuinely true of the shipped code; that edit simply cannot
 *      prove it. ⚠️ Worth keeping in view: the branch's own harmlessness is what
 *      made a mutation aimed past it look alive.
 *      ⇒ THE DISCRIMINATING PAIR, one per property Test 17(b) asserts:
 *      M19a — call ArmVideoModeCountdown() directly from HandleGroupSliderCommitted()
 *             ⇒ RED: ≥ 1 row in NoTimerNoApplyAndQualityNeverArms, INCLUDING
 *             "a QUALITY change arms NO confirmation" (and the frame-rate row
 *             stays green, which keeps the red attributable to the slider).
 *      M19b — call Graphics->ApplyVideoModeProvisional() from the same handler
 *             ⇒ RED: ≥ 1 row, INCLUDING "reaches NO provisional display apply".
 *      ⚠️ This is board cl. (3): a confirm prompt on every slider drag makes the
 *      panel unusable, which is the opposite of what GFX-§7 asks for.
 *
 *  M20 In BeginVideoModeConfirmation's wrap-around branch, `return false;`
 *      WITHOUT the DisarmVideoModeCountdown() above it.
 *      ⇒ RED: ⛔ CORRECTED (gate WARN-4) — ≥ 2 rows in
 *      SteppingBackToTheStartingModeCancelsTheCountdown, and I expect EXACTLY
 *      two: "the countdown is CANCELLED instead of re-armed" (the flag stays up)
 *      and "the remainder is zeroed with it" (it reads 10.0). ⚠️ The
 *      stranded-tick row is NOT among them — a countdown nothing disarmed still
 *      holds 10.0 s and two ticks only reach 8.0, so no expiry and no revert.
 *      Everything the FACADE holds stays right, because the discard below still
 *      runs; only the panel is wrong.
 *      🚨 UNDER THE PRE-LOOP-1 BYTES THIS MUTATION HAD **NO RED AT ALL**, and the
 *      reason was not the mutation: the branch was UNREACHABLE (BLOCKER-1), so
 *      deleting a line from it changed nothing anywhere. ⛔ A mutation with no red
 *      is evidence about REACHABILITY before it is evidence about the assertion —
 *      that is the durable lesson of this row and it is why M21 exists.
 *
 *  M21 🚨⭐⭐⭐ THE LOOP-1 FIX'S OWN MUTATION. In BeginVideoModeConfirmation's
 *      wrap-around branch, change the condition back to
 *      `!Graphics->IsVideoModeChangePending()`.
 *      ⇒ RED: ⛔ CORRECTED (board cl. 5b, and then MEASURED BY TASK-1124) — the
 *      count is ⛔ FIVE, not four. The sentence below already ENUMERATED five while
 *      claiming four; TASK-1124 executed it and observed exactly those five:
 *      ≥ 5 rows in SteppingBackToTheStartingModeCancelsTheCountdown, and it is
 *      EXACTLY five: "the branch reads the DIFFERENCE, not the LATCH"
 *      (re-armed ⇒ active), "the remainder is zeroed with it" (re-armed ⇒ 10.0),
 *      "AND THE FACADE'S WINDOW IS CLOSED" (the latch is still up) and BOTH
 *      release rows — "Auto-Detect is no longer refused" (the counter reads 1)
 *      and "a quality change SAVES again" (it does not).
 *      ⛔ THE APPLY-COUNT ROW STAYS GREEN, AND THAT COINCIDENCE IS WORTH KNOWING:
 *      ModeCount-1 provisional applies + one MORE provisional apply is the same
 *      number as ModeCount-1 + the closing revert's apply. A count alone cannot
 *      tell the fixed branch from the broken one; only the four rows above can.
 *      The "with no difference left" row also stays green — the difference really
 *      is gone; it is the PREDICATE that was looking at the wrong half.
 *      ⛔ THIS IS THE MUTATION THAT RESTORES THE SHIPPED DEFECT, and if it does
 *      not redden, the fix is not doing what this row believes it is doing.
 *
 *  M22 ⭐⭐ Delete the DiscardStagedVideoMode(Graphics) call from that same
 *      wrap-around branch, leaving the disarm.
 *      ⇒ RED: ≥ 4 rows in SteppingBackToTheStartingModeCancelsTheCountdown, and
 *      I expect EXACTLY four: "AND THE FACADE'S WINDOW IS CLOSED" (the latch
 *      stays up), the apply count (reads ModeCount-1 — the closing revert never
 *      happens), "Auto-Detect is no longer refused" (the counter reads 1) and
 *      "a quality change SAVES again" (it does not). The two prompt rows stay
 *      GREEN, which is the whole danger: ⛔ THE PANEL LOOKS COMPLETELY CORRECT.
 *      ⛔ LIVE COST — AND IT IS THE REASON THE DISCARD IS THERE: the prompt goes
 *      away, the picture is right, and the facade silently refuses EVERY save and
 *      Auto-Detect for the rest of the session with nothing on screen to explain
 *      it. That is qa/TASK-1114's soft-lock arriving through a door opened by the
 *      fix for a different defect.
 *
 *  ⛔ WHAT NO MUTATION IN THIS FILE CAN REACH, AND WHY IT IS NOT AN OVERSIGHT:
 *    (i) 🚨 THAT FTimerManager ACTUALLY CALLS TickVideoModeCountdown(). Deleting
 *        the SetTimer call in ArmVideoModeCountdown reddens NOTHING here — every
 *        test drives the tick by hand through the automation seam, because a bare
 *        NewObject widget has no world to hold a timer. ⇒ a CODE READ at
 *        ArmVideoModeCountdown and a PIXEL check (TASK-1125: change Window Mode,
 *        touch nothing, watch it come back), exactly as GFX-§2(c)'s ordering rung
 *        is a code read rather than a test. ⛔ It is the single most load-bearing
 *        unproven line in this row and it is named here rather than buried.
 *        ⛔ LOOP 1 ASKED WHETHER A CHEAP HONEST ASSERTION EXISTS AND THE ANSWER
 *        IS NO. "The handle is valid after arming" cannot be reached: the seam
 *        that makes every other countdown test possible RETURNS BEFORE the
 *        SetTimer block, and the only other way in needs the world the suite
 *        cannot build. Adding a handle getter would buy one assertion — "the
 *        handle is INVALID while driven manually" — which is a green row proving
 *        the seam works and NOTHING about the real timer, i.e. an artefact that
 *        reads as coverage and is not. Declined deliberately; the observation is
 *        TASK-1125's.
 *   (ii) Adding a REDUNDANT DiscardStagedVideoMode() call anywhere reddens
 *        nothing, because the function is guarded on IsVideoModeChangePending()
 *        and a second call is a no-op BY DESIGN. ⇒ "exactly one revert" is
 *        proved by COUNTING CALL SITES with grep (see
 *        handoffs/TASK-1118-programmer.md §"counted, not asserted"), not by a
 *        mutation. A test cannot see a call that does nothing.
 *        ⭐ AND THE SAME GUARD IS WHY M16's SECOND HALF CANNOT BE MADE LOAD-
 *        BEARING BY DRIVING MORE TICKS (loop 1, gate WARN-1): a stranded tick
 *        that DOES reach its expiry calls RevertSettingsPressed →
 *        DiscardStagedVideoMode, which finds the window already closed and
 *        reverts nothing. The "second revert" cl. (8) forbids is prevented ONE
 *        LAYER DOWN, so no test in this file can observe a missing disarm through
 *        the revert count — only through the countdown's own two fields, which is
 *        what M16's corrected rows now assert.
 *  (iii) Whether the prompt is VISIBLE, legible or on screen. No Slate tree is
 *        built here; GFX-§2(e) makes that a pixel/human check.
 *
 *  ─────────────────────────────────────────────────────────────────────────
 *  ⛔ CORRECTIONS TO M4 / M5 / M7 / M10, ADOPTED FROM qa/TASK-1116.md — NOT
 *  INVENTED HERE. The gate re-derived TASK-1115's table by hand and four rows
 *  came out different. They are corrected in place because a mutation table with
 *  a known-false row in it is worse than none: the next reader runs it, sees the
 *  "wrong" number of reds, and doubts the CODE.
 *  ⚠️ ALL FOUR REMAIN PREDICTIONS. The gate did not execute them either.
 *
 *   M4  (NIT-3) reddens FOUR rows, not two: test rows 458 and 460 — the
 *       nothing-pending pair — redden too, because an unguarded discard reverts
 *       when nothing is staged. The CALL 1 block stays green. ⇒ the guard is
 *       caught by four assertions and the test is STRONGER than its author said.
 *   M5  (NIT-4) reddens FIVE rows, not four. The positive-control block above
 *       still stays green, which is what makes the red attributable.
 *   M7  (NIT-6) the hedged second clause DOES NOT DERIVE:
 *       GroupCommitWritesOnlyTheMovedGroup stays GREEN under truncation, because
 *       LevelToSliderValue(1) is exactly 0.25f and 0.25f * 4.0f == 1.0f. ⛔ The
 *       word "probably" was doing an assertion's job (SC-§90).
 *   M10 (WARN-5) reddens the TestNotEqual ONLY. The two ⭐⭐ rows compare the
 *       panel's line against FogOff/FogOn, which the mutation has made IDENTICAL
 *       to that line, so they cannot discriminate and stay GREEN. ⇒ that single
 *       TestNotEqual is the ONLY assertion separating "readout" from
 *       "decoration". Same species as TASK-1113's M13, which TASK-1117 then
 *       measured to redden exactly one row.
 *  ─────────────────────────────────────────────────────────────────────────
 *  ⛔ WHAT NO MUTATION HERE CAN REACH, RESTATED SO THE TABLE IS NOT MISREAD AS
 *  COMPLETE: moving ConstructGraphicsTree() below Super::RebuildWidget()
 *  (GFX-§2(c)) reddens NOTHING in this file. It is a three-line code read at
 *  RebuildWidget() and a QA criterion on TASK-1116.
 *  ═════════════════════════════════════════════════════════════════════════
 */


// ════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1120 [GFX-FPS] — THE READOUT, THE TOGGLE AND THE IN-MATCH COUNTER
//
//  ⛔ WHY HERE AND NOT IN Tests/SiegeGraphicsSettingsTest.cpp (the board's
//  `names:` line): the code under test is the two WIDGETS, and this file owns
//  their harness — MakeScratchPanel(), FindPinned(), the hermetic
//  UGameUserSettings. The qa/TASK-1119.md F-6 ruling, applied unchanged, and
//  declared as a flagged decision in the handoff rather than left to be noticed.
//
//  ─────────────────────────────────────────────────────────────────────────
//  SCOPE (`SC-§79`) — WHAT THE FOUR TESTS BELOW **CANNOT** DETECT
//  ─────────────────────────────────────────────────────────────────────────
//  ⛔ THAT "VISIBLE ⇒ TIMER ARMED". A bare NewObject widget has NO WORLD and
//     therefore no FTimerManager, so ApplyFrameRateCounterPreference() takes its
//     no-timer-manager branch and IsCounterTimerArmed() reports false in BOTH
//     states. An assertion that the counter is armed when shown would be a test
//     that CANNOT FAIL HERE, which qa/TASK-1123's M8 has already taught this lane
//     is worse than no test — so it is NOT written. What IS asserted is the half
//     that does discriminate: the VISIBILITY enum, and the pure decision behind
//     it. The SetTimer→callback edge itself is engine behaviour used identically
//     at a dozen shipped sites in this project and is a DECLARED UNPROVEN
//     PREMISE, exactly as TASK-1118's countdown records it.
//  ⛔ THAT THE NUMBER IS CORRECT ON A REAL MACHINE. FSiegeFrameRateSample is
//     driven below with FABRICATED counter/clock pairs, on purpose: that makes
//     the arithmetic deterministic and the mutations meaningful. Whether
//     GFrameCounter advances once per drawn frame is measured from the engine
//     source (LaunchEngineLoop.cpp:6130-6131), not from a run.
//  ⛔ ANYTHING PIXEL. Whether the counter is legible over grass, clear of the
//     card bar, or actually on screen in L_Arena is `GFX-§2(e)`'s pixel/human
//     check and is owed to TASK-1125, not to this file.
//
//  🚨 NO WITNESSED RED. Nothing below has been compiled or executed.
// ════════════════════════════════════════════════════════════════════════════

namespace SiegeFrameRateTestUtils
{
	/**
	 *  ⛔ A DIFFERENT SCRATCH SLOT STRING FROM Tests/SiegeSettingsTest.cpp'S, AND
	 *  THAT IS THE POINT. Two files guarding one slot name would be two
	 *  independent owners of one piece of disk state; two names are two states.
	 *  Neither is the player's real SiegeSettings.sav — asserted below.
	 */
	static const TCHAR* SettingsScratchSlotName = TEXT("SiegeSettings_GraphicsMenuScratch");

	static void DeleteSettingsScratchSlot()
	{
		if (UGameplayStatics::DoesSaveGameExist(SettingsScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex))
		{
			UGameplayStatics::DeleteGameInSlot(SettingsScratchSlotName, USiegeSettingsSubsystem::SettingsUserIndex);
		}
	}

	/** Deletes on the way in AND out, so no test inherits or leaves disk state. */
	struct FSettingsScratchGuard
	{
		FSettingsScratchGuard()  { DeleteSettingsScratchSlot(); }
		~FSettingsScratchGuard() { DeleteSettingsScratchSlot(); }
	};

	/**
	 *  A settings store living inside the given game instance and redirected to
	 *  the scratch slot BEFORE any load or save — which is what keeps a test run
	 *  from touching Saved/SaveGames/SiegeSettings.sav.
	 *
	 *  ⚠️ UGameInstanceSubsystem is UCLASS(Within = GameInstance), so the outer is
	 *  a requirement and not a nicety: a bare NewObject lands in the transient
	 *  package and trips StaticAllocateObject's ClassWithin check.
	 */
	static USiegeSettingsSubsystem* MakeScratchSettings(UGameInstance* Outer)
	{
		if (Outer == nullptr)
		{
			return nullptr;
		}

		USiegeSettingsSubsystem* Settings = NewObject<USiegeSettingsSubsystem>(Outer);
		if (Settings != nullptr)
		{
			Settings->SetSlotNameForAutomationTests(SettingsScratchSlotName);
		}
		return Settings;
	}
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST — the measurement: the arithmetic, every refusal, and the window walk
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeFrameRateSampleMathTest,
	"Siegebound.GraphicsMenu.FrameRateSampleMathAndRefusals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeFrameRateSampleMathTest::RunTest(const FString& Parameters)
{
	// ---- the arithmetic -----------------------------------------------------
	{
		float Fps = 0.0f;
		float Ms  = 0.0f;
		TestTrue(TEXT("30 frames in 0.5 s is an honest window"),
			FSiegeFrameRateSample::ComputeOverWindow(30, 0.5, Fps, Ms));
		TestEqual(TEXT("30 frames / 0.5 s = 60 FPS"), Fps, 60.0f, 0.001f);
		TestEqual(TEXT("30 frames / 0.5 s = 16.667 ms per frame"), Ms, 16.6667f, 0.001f);
	}

	{
		// ⭐ THE FPS/ms SWAP DETECTOR. At 60 fps the two numbers are 60 and 16.7;
		// at 30 fps they are 30 and 33.3. ⛔ A version that assigned the outputs to
		// each other would satisfy neither pair, and picking a rate where the two
		// numbers are far apart in BOTH directions is what makes that so.
		float Fps = 0.0f;
		float Ms  = 0.0f;
		TestTrue(TEXT("15 frames in 0.5 s is an honest window"),
			FSiegeFrameRateSample::ComputeOverWindow(15, 0.5, Fps, Ms));
		TestEqual(TEXT("15 frames / 0.5 s = 30 FPS"), Fps, 30.0f, 0.001f);
		TestEqual(TEXT("15 frames / 0.5 s = 33.333 ms per frame"), Ms, 33.3333f, 0.001f);
	}

	// ---- the three refusals, and that they leave the outputs ALONE ----------
	// ⛔ THE SENTINELS ARE THE ASSERTION. "Returns false" is half the contract;
	// the other half is that the caller's previous numbers are untouched, so it
	// can keep showing the last honest reading instead of a fabricated zero.
	{
		float Fps = -7.0f;
		float Ms  = -7.0f;

		TestFalse(TEXT("⛔ ZERO FRAMES in a window is REFUSED (a minimised window, an alt-tab, a long hitch)"),
			FSiegeFrameRateSample::ComputeOverWindow(0, 0.5, Fps, Ms));
		TestEqual(TEXT("A refused window leaves OutFramesPerSecond untouched"), Fps, -7.0f, 0.001f);
		TestEqual(TEXT("A refused window leaves OutMilliseconds untouched"), Ms, -7.0f, 0.001f);

		TestFalse(TEXT("⛔ A ZERO-length window is REFUSED (the clock did not advance)"),
			FSiegeFrameRateSample::ComputeOverWindow(30, 0.0, Fps, Ms));
		TestFalse(TEXT("⛔ A NEGATIVE-length window is REFUSED"),
			FSiegeFrameRateSample::ComputeOverWindow(30, -0.5, Fps, Ms));
		TestEqual(TEXT("Neither refusal touched OutFramesPerSecond either"), Fps, -7.0f, 0.001f);
	}

	// ---- the text -----------------------------------------------------------
	{
		const FString Text = FSiegeFrameRateSample::ComposeReadoutText(60.0f, 16.6667f);
		TestTrue(TEXT("⭐ The readout names the FPS figure"), Text.Contains(TEXT("60")));
		TestTrue(TEXT("⭐ The readout names the frame time in ms — GFX-§7 asks for BOTH"),
			Text.Contains(TEXT("16.7")) && Text.Contains(TEXT("ms")));
		TestTrue(TEXT("The readout names its FPS unit"), Text.Contains(TEXT("FPS")));

		const FString Pending = FSiegeFrameRateSample::ComposePendingText();
		TestTrue(TEXT("The pending text says 'waiting', with dashes"), Pending.Contains(TEXT("--")));
		// ⛔ NOT "0 FPS": a zero reads as a measurement of a frozen game rather
		// than as the absence of a measurement.
		TestFalse(TEXT("⛔ The pending text contains NO digit — it is not a number"),
			Pending.Contains(TEXT("0")) || Pending.Contains(TEXT("1")) || Pending.Contains(TEXT("9")));
		TestNotEqual(TEXT("Pending and a real reading are different strings"),
			Pending, FSiegeFrameRateSample::ComposeReadoutText(60.0f, 16.6667f));
	}

	// ---- the window walk ----------------------------------------------------
	{
		FSiegeFrameRateSample Sample;
		FString Text;

		// The first call OPENS the window. It has nothing to subtract from, so a
		// reading here would be an invention.
		TestFalse(TEXT("⛔ The FIRST sample publishes nothing — it opens the window"),
			Sample.Advance(1000, 100.0, Text));
		TestTrue(TEXT("…and wrote no text"), Text.IsEmpty());

		TestTrue(TEXT("The second sample closes a 30-frame / 0.5 s window"),
			Sample.Advance(1030, 100.5, Text));
		TestEqual(TEXT("…and publishes 60 FPS"), Text,
			FSiegeFrameRateSample::ComposeReadoutText(60.0f, 16.6667f));

		// ⭐⭐ THE ANCHOR-ADVANCES-ON-REFUSAL ASSERTION, AND IT IS THE SHARPEST ROW
		// IN THIS FILE. A stalled half second is refused — and the window must
		// still MOVE ON. If the anchor stayed at 100.5 / 1030, the next window
		// below would measure 30 frames across 1.0 s and publish 30 FPS instead of
		// 60, i.e. ⛔ one bad half-second would halve the readout for as long as the
		// panel stayed open.
		TestFalse(TEXT("A stalled window (zero frames) publishes nothing"),
			Sample.Advance(1030, 101.0, Text));
		TestTrue(TEXT("⭐ After a refusal the NEXT window is measured from the refusal, not from before it"),
			Sample.Advance(1060, 101.5, Text));
		TestEqual(TEXT("⭐ …so it publishes 60 FPS again, not 30"), Text,
			FSiegeFrameRateSample::ComposeReadoutText(60.0f, 16.6667f));

		// ⛔ A BACKWARDS COUNTER MUST NOT WRAP. Unsigned subtraction of a smaller
		// from a larger would give ~1.8e19 frames and print a spectacular lie.
		TestFalse(TEXT("⛔ A counter that went BACKWARDS is refused, never wrapped"),
			Sample.Advance(500, 102.0, Text));

		// Reset re-opens: the next call publishes nothing again.
		Sample.Reset();
		TestFalse(TEXT("After Reset() the next sample opens a fresh window"),
			Sample.Advance(2000, 200.0, Text));
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST — the panel grows a readout line and an in-match toggle
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuFrameRateRowTest,
	"Siegebound.GraphicsMenu.FrameRateReadoutRowAndToggleExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuFrameRateRowTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	// ⛔ LOOKED UP BY THE PINNED NAME THROUGH THE TREE, never through the C++
	// member — so a member wired to a widget carrying the wrong name FAILS here.
	// `FrameRateReadoutText` is pinned by GFX-§10 character-for-character.
	UTextBlock* Readout = FindPinned<UTextBlock>(Scratch, TEXT("FrameRateReadoutText"));
	TestNotNull(TEXT("⭐ FrameRateReadoutText exists (GFX-§10 pinned, GFX-§7 cl. 1)"), Readout);

	if (Readout != nullptr)
	{
		// ⛔ SEEDED PENDING, NEVER WITH A NUMBER. The first honest reading is two
		// timer periods away; anything numeric here would be a fabrication the
		// player could not tell from a measurement.
		TestEqual(TEXT("⭐ The readout opens at the PENDING text, not at a fabricated number"),
			Readout->GetText().ToString(), FSiegeFrameRateSample::ComposePendingText());
	}

	TestNotNull(TEXT("⭐ ShowFrameRateCounterCheckBox exists — the in-match counter is opt-in from HERE"),
		FindPinned<UCheckBox>(Scratch, TEXT("ShowFrameRateCounterCheckBox")));
	TestNotNull(TEXT("ShowFrameRateCounterLabelText exists"),
		FindPinned<UTextBlock>(Scratch, TEXT("ShowFrameRateCounterLabelText")));
	// The hint is GFX-§7's argument on screen: it tells the player the menu number
	// is NOT the number to tune against.
	TestNotNull(TEXT("⭐ ShowFrameRateCounterHintText exists"),
		FindPinned<UTextBlock>(Scratch, TEXT("ShowFrameRateCounterHintText")));

	// The readout period is also the averaging window; a zero or negative value
	// would mean a timer that never fires or fires continuously.
	TestTrue(TEXT("The readout interval is a positive, sub-second window"),
		USiegeGraphicsMenuWidget::FrameRateReadoutIntervalSeconds > 0.0f
		&& USiegeGraphicsMenuWidget::FrameRateReadoutIntervalSeconds <= 1.0f);

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST — the toggle seeds from the store, writes to the store, and FILTERS
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeGraphicsMenuFrameRateToggleTest,
	"Siegebound.GraphicsMenu.FrameRateToggleSeedsWritesAndFilters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeGraphicsMenuFrameRateToggleTest::RunTest(const FString& Parameters)
{
	using namespace SiegeGraphicsMenuTestUtils;
	SiegeFrameRateTestUtils::FSettingsScratchGuard SlotGuard;

	// The hermeticity guarantee, made mechanical rather than promised.
	TestNotEqual(TEXT("⛔ This file's scratch slot is NOT the player's real settings slot"),
		FString(SiegeFrameRateTestUtils::SettingsScratchSlotName),
		FString(USiegeSettingsSubsystem::SettingsSlotName));

	FScratchPanel Scratch = MakeScratchPanel();
	if (!Scratch.IsValid())
	{
		AddError(TEXT("Could not build a scratch graphics panel."));
		return false;
	}

	USiegeSettingsSubsystem* Settings = SiegeFrameRateTestUtils::MakeScratchSettings(Scratch.GameInstance.Get());
	if (Settings == nullptr)
	{
		AddError(TEXT("Could not construct a scratch USiegeSettingsSubsystem inside the panel's game instance."));
		return false;
	}
	TStrongObjectPtr<USiegeSettingsSubsystem> SettingsKeepAlive(Settings);

	UCheckBox* Toggle = FindPinned<UCheckBox>(Scratch, TEXT("ShowFrameRateCounterCheckBox"));
	if (Toggle == nullptr)
	{
		AddError(TEXT("ShowFrameRateCounterCheckBox is missing — the rest of this test cannot run."));
		return false;
	}

	// ---- SEED ---------------------------------------------------------------
	// The store says ON before the row is seeded, so a checkbox that ends up
	// checked cannot have got there by construction default (which is unchecked).
	Settings->SetFrameRateCounterEnabled(true);
	Scratch.Panel->SetSettingsSubsystemForAutomationTests(Settings);
	Scratch.Panel->RefreshFrameRateCounterRow();

	TestTrue(TEXT("⭐ The toggle SEEDS from the persisted preference (on)"), Toggle->IsChecked());
	TestTrue(TEXT("The toggle is enabled while its store resolves"), Toggle->GetIsEnabled());

	// ---- WRITE --------------------------------------------------------------
	// ⛔ THE ASSERTION IS THE STORE'S STATE, NOT A CALL COUNT. A handler that
	// resolved the wrong subsystem, or wrote the wrong setting, would leave this
	// value where it was while any tally of "handler ran" stayed green.
	Scratch.Panel->HandleShowFrameRateCounterChanged(false);
	TestFalse(TEXT("⭐ Unticking the box WRITES the preference off, in the store"),
		Settings->IsFrameRateCounterEnabled());

	Scratch.Panel->HandleShowFrameRateCounterChanged(true);
	TestTrue(TEXT("⭐ …and ticking it writes it back on"),
		Settings->IsFrameRateCounterEnabled());

	// ⛔ AND IT WRITES **THIS** SETTING AND NOT ITS NEIGHBOUR. A copy-pasted
	// handler calling SetAssistantConfirmEnabled would satisfy nothing above but
	// would satisfy a "something changed" style assertion.
	TestTrue(TEXT("⛔ The toggle did NOT disturb the assistant confirm setting"),
		Settings->IsAssistantConfirmEnabled());

	// ---- RE-SEED ON THE RIGHT BROADCAST -------------------------------------
	Settings->SetFrameRateCounterEnabled(false);
	Scratch.Panel->HandleSettingsChanged(USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter);
	TestFalse(TEXT("⭐ A frame-counter broadcast re-seeds the checkbox"), Toggle->IsChecked());

	// ---- ⛔ AND **NOT** ON THE WRONG ONE -------------------------------------
	// The store is moved behind the panel's back, then an UNRELATED token is
	// broadcast. A handler that ignored the token would re-seed and tick the box;
	// the filter is what keeps it where it is. ⛔ This is the assertion that makes
	// the payload NAME load-bearing rather than decorative.
	Settings->SetFrameRateCounterEnabled(true);
	Scratch.Panel->HandleSettingsChanged(USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute);
	TestFalse(TEXT("⛔ An UNRELATED setting's broadcast does NOT re-seed this row (the token filter)"),
		Toggle->IsChecked());

	// ---- NO STORE: the row goes dead, and NOTHING ELSE DOES ------------------
	Scratch.Panel->SetSettingsSubsystemForAutomationTests(nullptr);
	Scratch.Panel->RefreshFrameRateCounterRow();

	TestFalse(TEXT("⛔ With no settings store the row is DISABLED"), Toggle->GetIsEnabled());
	TestFalse(TEXT("⛔ …and shows OFF — the fail-safe direction is no overlay"), Toggle->IsChecked());

	// ⭐ THE INVARIANT THIS ROW DELIBERATELY DOES NOT BREAK: the panel is NOT
	// declared dead because the OTHER subsystem is missing. Back must still work,
	// and so must every graphics control, because the graphics facade is fine.
	if (UButton* Back = FindPinned<UButton>(Scratch, TEXT("BackButton")))
	{
		TestTrue(TEXT("⭐ A missing SETTINGS store does not disable Back"), Back->GetIsEnabled());
	}
	if (USlider* Shadow = FindPinned<USlider>(Scratch, TEXT("ShadowQualitySlider")))
	{
		TestTrue(TEXT("⭐ …nor any control the GRAPHICS facade owns"), Shadow->GetIsEnabled());
	}

	return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  TEST — the IN-MATCH counter: the tree, the fail-safe, and the visibility
// ════════════════════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSiegeInMatchFrameRateCounterTest,
	"Siegebound.GraphicsMenu.InMatchCounterFollowsThePreference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSiegeInMatchFrameRateCounterTest::RunTest(const FString& Parameters)
{
	SiegeFrameRateTestUtils::FSettingsScratchGuard SlotGuard;

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GetTransientPackageAsObject()));
	if (!GameInstance.IsValid())
	{
		AddError(TEXT("Could not construct a throwaway UGameInstance."));
		return false;
	}

	USiegeSettingsSubsystem* Settings = SiegeFrameRateTestUtils::MakeScratchSettings(GameInstance.Get());
	if (Settings == nullptr)
	{
		AddError(TEXT("Could not construct a scratch USiegeSettingsSubsystem."));
		return false;
	}
	TStrongObjectPtr<USiegeSettingsSubsystem> SettingsKeepAlive(Settings);

	// ---- the pure decision, both polarities and the fail-safe ---------------
	// ⛔ nullptr ⇒ false. A lookup failure must never put a diagnostic overlay on
	// a shipped player's battlefield — the OPPOSITE polarity to the confirm
	// toggle's `true` fallback, which is why it is asserted rather than assumed.
	TestFalse(TEXT("⛔ No settings store ⇒ the counter does NOT show (fail-safe OFF)"),
		USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(nullptr));
	TestFalse(TEXT("⛔ A fresh store defaults to OFF (board cl. 4 — a counter is opt-IN)"),
		USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(Settings));

	Settings->SetFrameRateCounterEnabled(true);
	TestTrue(TEXT("⭐ An opted-in store ⇒ the counter shows"),
		USiegeFrameRateCounterWidget::ShouldShowFrameRateCounter(Settings));

	// ---- the widget ---------------------------------------------------------
	TStrongObjectPtr<USiegeFrameRateCounterWidget> Counter(
		NewObject<USiegeFrameRateCounterWidget>(GetTransientPackageAsObject()));
	if (!Counter.IsValid())
	{
		AddError(TEXT("Could not construct a USiegeFrameRateCounterWidget."));
		return false;
	}

	Counter->SetSettingsSubsystemForAutomationTests(Settings);
	Counter->Initialize();
	Counter->ConstructCounterTree();

	if (Counter->WidgetTree == nullptr)
	{
		AddError(TEXT("The counter has no WidgetTree after Initialize()."));
		return false;
	}

	UBorder* Root = Counter->WidgetTree->FindWidget<UBorder>(FName(TEXT("FrameRateCounterRoot")));
	TestNotNull(TEXT("FrameRateCounterRoot exists"), Root);
	TestNotNull(TEXT("FrameRateCounterBorder exists"),
		Counter->WidgetTree->FindWidget<UBorder>(FName(TEXT("FrameRateCounterBorder"))));

	// ⛔ THE SAME PINNED NAME AS THE PANEL'S CHILD, IN A DIFFERENT TREE — one word
	// for one thing, and GFX-§10's `FrameRateReadoutText` either way.
	UTextBlock* Readout = Counter->WidgetTree->FindWidget<UTextBlock>(
		FName(USiegeFrameRateCounterWidget::FrameRateReadoutWidgetName));
	TestNotNull(TEXT("⭐ FrameRateReadoutText exists in the in-match tree too"), Readout);
	if (Readout != nullptr)
	{
		TestEqual(TEXT("The in-match readout opens PENDING, not at a number"),
			Readout->GetText().ToString(), FSiegeFrameRateSample::ComposePendingText());
	}
	TestEqualSensitive(TEXT("The in-match readout carries the GFX-§10 pinned name"),
		FString(USiegeFrameRateCounterWidget::FrameRateReadoutWidgetName), FString(TEXT("FrameRateReadoutText")));

	if (Root == nullptr)
	{
		return false;
	}

	// 🚨⛔⛔ THE CLICK-THROUGH GUARD. This border FILLS the viewport in order to
	// align its content to the top-right corner, so `Visible` here would swallow
	// EVERY click in the match — placement, orders, the card bar. `Collapsed` is
	// not enough either: the widget must be non-hit-testable while SHOWN.
	// ⚠️ This is the exact OPPOSITE of GFX-§2(f)'s ruling for the menu backdrop,
	// which is deliberately `Visible` so that it DOES absorb clicks.
	TestEqual(TEXT("⛔ The counter root is HitTestInvisible as constructed — it can never eat a click"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::HitTestInvisible));

	// ---- OFF: collapsed, and the timer is not armed -------------------------
	Settings->SetFrameRateCounterEnabled(false);
	Counter->ApplyFrameRateCounterPreference();
	TestEqual(TEXT("⭐ Preference OFF ⇒ the counter is COLLAPSED"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::Collapsed));
	TestFalse(TEXT("⭐ Preference OFF ⇒ no sampling timer is armed"), Counter->IsCounterTimerArmed());

	// ---- ON: shown again, and STILL not hit-testable ------------------------
	Settings->SetFrameRateCounterEnabled(true);
	Counter->ApplyFrameRateCounterPreference();
	TestEqual(TEXT("⭐ Preference ON ⇒ the counter is shown, and shown HitTestInvisible"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::HitTestInvisible));

	// ---- the token filter, on the counter too -------------------------------
	// Move the store behind the widget's back and broadcast an UNRELATED token: a
	// handler that ignored the token would collapse the counter here.
	Settings->SetFrameRateCounterEnabled(false);
	Counter->HandleSettingsChanged(USiegeSettingsSubsystem::SettingName_AssistantConfirmBeforeExecute);
	TestEqual(TEXT("⛔ An unrelated setting's broadcast does NOT move the counter"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::HitTestInvisible));

	// …and the RIGHT token does.
	Counter->HandleSettingsChanged(USiegeSettingsSubsystem::SettingName_ShowFrameRateCounter);
	TestEqual(TEXT("⭐ The frame-counter broadcast collapses it"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::Collapsed));

	// ---- no store at all: the counter stays down ----------------------------
	Counter->SetSettingsSubsystemForAutomationTests(nullptr);
	Settings->SetFrameRateCounterEnabled(true); // ⛔ irrelevant: the widget can no longer see it
	Counter->ApplyFrameRateCounterPreference();
	TestEqual(TEXT("⛔ With no settings store the counter stays COLLAPSED, whatever the store says"),
		static_cast<int32>(Root->GetVisibility()), static_cast<int32>(ESlateVisibility::Collapsed));
	TestFalse(TEXT("⛔ …and no timer is armed"), Counter->IsCounterTimerArmed());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
