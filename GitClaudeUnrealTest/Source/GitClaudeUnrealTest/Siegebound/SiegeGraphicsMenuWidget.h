// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Templates/SubclassOf.h"
#include "TimerManager.h"
#include "SiegeGraphicsMenuWidget.generated.h"

class APlayerController;
class UBorder;
class UButton;
class UCheckBox;
class UHorizontalBox;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class USiegeGraphicsSettingsSubsystem;
// TASK-1417 [MENU-NAV-GRAPHICS]: this screen CALLS TASK-1406's public registration
// API and never edits it — USiegeMenuInputSubsystem is that row's file.
class USiegeMenuInputSubsystem;
class USiegeSettingsSubsystem;

/**
 *  ⭐⭐ TASK-1120 [GFX-FPS] — THE WHOLE MEASUREMENT, AND IT COSTS NOTHING PER
 *  FRAME. `GFX-§7`.
 *
 *  ═══ WHY THIS SHAPE, AND WHAT WAS REJECTED ═══════════════════════════════
 *
 *  An honest average over a window needs to know how many frames were drawn in
 *  it. There are exactly three ways to learn that, and two of them are worse:
 *
 *   (a) ⛔ TICK AND COUNT OURSELVES. Rejected by the board in so many words: "a
 *       tick that exists only to print a number is a cost the number then
 *       reports". A widget tick that runs every frame to service a 2 Hz readout
 *       is the instrument corrupting its own measurement.
 *
 *   (b) ⚠️ READ THE ENGINE'S OWN AVERAGE, `GAverageFPS` / `GAverageMS`. These are
 *       real and they ARE maintained every frame whether we look or not
 *       (`UnrealEngine.cpp:798-829` — an exponential moving average with a 0.1
 *       smoothing factor, i.e. the numbers `stat fps` prints). ⛔ But they are
 *       declared `extern ENGINE_API` in PRIVATE .cpp files only — no public
 *       header in `Runtime/Engine/Public` declares either — so using them means
 *       re-declaring an engine global by hand in our module, and inheriting a
 *       smoothing constant we did not choose (≈10 frames ≈ 0.16 s, not the
 *       ~0.5 s the board asked for). ⛔ Rejected, but recorded rather than
 *       ignored: it is the near-miss a reader will otherwise re-propose.
 *
 *   (c) ✅ THE ONE TAKEN: `GFrameCounter`. It is PUBLIC (`CoreGlobals.h:532`,
 *       `extern CORE_API uint64 GFrameCounter;`) and it is incremented ONCE PER
 *       ENGINE TICK, unconditionally, at the end of `FEngineLoop::Tick`
 *       (`LaunchEngineLoop.cpp:6130-6131`, comment "Increment global frame
 *       counter. Once for each engine tick.") in EVERY build configuration —
 *       ⛔ unlike `stat fps` / `stat unit`, whose whole subsystem is compiled out
 *       of a Shipping build. Subtract two samples of it, subtract two samples of
 *       the clock, and the quotient IS the average frame rate over the window,
 *       by definition rather than by approximation.
 *
 *  ⇒ THE COST OF THE INSTRUMENT IS TWO GLOBAL READS AND A DIVISION, TWICE A
 *  SECOND. There is no per-frame work of any kind, which is what makes the
 *  number it prints honest: turning the counter ON does not measurably change
 *  the thing being counted.
 *
 *  ⚠️ IT IS A BOX AVERAGE, NOT AN EMA, AND THAT IS DELIBERATE: over a 0.5 s
 *  window it is exactly "frames drawn ÷ seconds elapsed", which is the number a
 *  player means by "FPS". An EMA would be smoother and would lie about the size
 *  of a stutter.
 *
 *  ⛔ NOT A USTRUCT AND NOT REFLECTED: it holds no UObject reference and needs
 *  no serialization, so it stays a plain member of whatever widget owns it.
 */
struct FSiegeFrameRateSample
{
	/**
	 *  ⛔ THE ARITHMETIC, AND IT IS A PURE STATIC SO THE SUITE CAN ASSERT IT
	 *  WITHOUT A WORLD, A WIDGET, A CLOCK OR A FRAME.
	 *
	 *  Returns false — and leaves BOTH outputs untouched — when the window cannot
	 *  yield an honest reading. ⛔ THE REFUSALS ARE THE POINT, not defensive
	 *  padding: `FramesElapsed == 0` happens for real (a minimised window, a
	 *  hitch longer than the window, an alt-tab), and dividing by it would put
	 *  `inf` or `nan` on the player's HUD in the exact moment their machine is
	 *  struggling — i.e. the one moment they are looking at it.
	 *
	 *  @param FramesElapsed        GFrameCounter now minus GFrameCounter then.
	 *  @param SecondsElapsed       wall seconds across the same window.
	 *  @param OutFramesPerSecond   frames ÷ seconds.
	 *  @param OutMilliseconds      seconds × 1000 ÷ frames — the mean time ONE
	 *                              frame took, which is the number that is linear
	 *                              in cost and therefore the one to tune against.
	 */
	static bool ComputeOverWindow(uint64 FramesElapsed, double SecondsElapsed, float& OutFramesPerSecond, float& OutMilliseconds);

	/** "62 FPS · 16.1 ms". FPS rounded to a whole number (nobody tunes on 0.1 fps); ms to one decimal (0.1 ms is ~0.6 % of a 60 Hz frame and IS worth seeing). */
	static FString ComposeReadoutText(float FramesPerSecond, float Milliseconds);

	/** What the readout says before the first window has closed, and on every refusal. ⛔ Never a stale number and never a zero — both would read as a measurement. */
	static FString ComposePendingText();

	/**
	 *  Closes the current window and opens the next one. Returns true and fills
	 *  OutText only when a window closed with an honest reading; the FIRST call
	 *  after Reset() always returns false because it has nothing to subtract from
	 *  (it is the sample that OPENS the first window).
	 *
	 *  ⚠️ Takes the counter and the clock as PARAMETERS rather than reading the
	 *  globals itself, so the suite drives it with fabricated numbers and the
	 *  arithmetic is testable end to end without a running engine.
	 */
	bool Advance(uint64 FrameCounterNow, double SecondsNow, FString& OutText);

	/** Forgets the open window. The next Advance() opens a new one and returns false. */
	void Reset();

	/** Reads the two engine globals. ⛔ The ONLY place in this project that touches either — one site to audit. */
	static void ReadEngineNow(uint64& OutFrameCounter, double& OutSeconds);

private:

	uint64 LastFrameCounter = 0;
	double LastSeconds = 0.0;
	bool bWindowOpen = false;
};

/**
 *  TASK-1115 [GFX-PANEL] — the Graphics submenu, the first playable rung of the
 *  GFX-§ lane. It is the FIRST caller of USiegeGraphicsSettingsSubsystem
 *  (TASK-1113 shipped that facade in 42734b7 with ZERO callers).
 *
 *  ============================================================================
 *  M8 DECLARATION: adds no replicated property, no new replicated class, no new
 *  relevancy tier. Every value this panel reads or writes lives in
 *  GameUserSettings.ini, which is PER MACHINE (GFX-§3) and never leaves it.
 *  ⛔ The one value in the facade that would be dangerous to replicate —
 *  GetFoliageQualityScale() — is NOT read here at all; the scatter consumer is
 *  TASK-1122, and the determinism prohibition lives on that getter.
 *  ============================================================================
 *
 *  ---------------------------------------------------------------------------
 *  WHY THERE IS NO .uasset — GFX-§2, THE FOURTH CODE-AUTHORED RULING
 *  ---------------------------------------------------------------------------
 *  CONVENTIONS GFX-§2 authorises a code-authored widget tree for THIS CLASS
 *  ONLY. It is the fourth such ruling (USiegeAssistantConsoleWidget,
 *  USettingsMenuWidget and UAccountMenuWidget hold the other three) and, exactly
 *  like them, it inherits from none of them and lends to none of them: a fifth
 *  widget may cite neither. /Game/UI/WBP_GraphicsMenu is RESERVED, NOT AUTHORED
 *  — measured by TASK-1112 §2(iii): a case-insensitive census of Content/ for
 *  *GraphicsMenu* returns ZERO files, and the complete WBP_*.uasset census is
 *  ten assets, none of them this one.
 *
 *  The six conditions of that ruling, ALL of which are QA criteria on TASK-1116:
 *   (a) SCOPE — this class only.
 *   (b) THE ESCAPE HATCH IS BUILT IN. Every child below is
 *       UPROPERTY(meta=(BindWidgetOptional)) with the exact GFX-§10 pinned name,
 *       and ConstructGraphicsTree() constructs a child only when it is still
 *       null. An asset-authored tree wins WHOLE — the code branch returns at its
 *       first line when WidgetTree->RootWidget is already set.
 *   (c) ⚠️ ORDER IS LOAD-BEARING — the tree is built BEFORE
 *       Super::RebuildWidget(). UUserWidget::RebuildWidget() reads
 *       WidgetTree->RootWidget AS IT STANDS and hands back an SSpacer when it is
 *       null, so building afterwards yields a SILENTLY EMPTY widget that still
 *       passes every property read-back. See RebuildWidget() in the .cpp; the
 *       call order there is the whole of this condition.
 *   (d) FString / int32 / bool ONLY in a BlueprintImplementableEvent parameter.
 *       ⛔ NEVER an enum, ⛔ NEVER a struct — and in particular ⛔ NEVER
 *       FIntPoint (board cl. 3e). This panel therefore consumes the facade's
 *       index + FString-label stepper API (GetSupportedScreenResolutionLabel /
 *       SetScreenResolutionByIndex) and never its FIntPoint overloads, and
 *       window mode crosses as int32 because the facade deliberately has no
 *       UENUM at all.
 *   (e) VERIFICATION IS A PIXEL / HUMAN CHECK. There is no .uasset to read back
 *       and MCP read-back has passed on visually-broken UMG here before
 *       (TASK-355: 6/6 bindings correct while stacked in a 165x48 px box).
 *       ⛔ NOTHING ON SCREEN IS VERIFIED BY THIS FILE'S AUTHOR. The checklist
 *       Jonathan must look at is in handoffs/TASK-1115-programmer.md.
 *   (f) THE BACKDROP IS HIT-TEST **VISIBLE**, and that is CORRECTNESS, NOT
 *       STYLING. This panel stacks on the LIVE settings panel, which stacks on
 *       the LIVE main menu, and NEITHER is removed. A HIT_TEST_INVISIBLE plate
 *       would ship a live click-through into Play / Sandbox / Deck Builder /
 *       QUIT while the panel looks modal.
 *
 *  ---------------------------------------------------------------------------
 *  NAVIGATION — THE TASK-438 SHAPE, CLONED ONE LEVEL DEEPER
 *  ---------------------------------------------------------------------------
 *      WBP_MainMenu --(Btn_Settings)--> USettingsMenuWidget  @ ZOrder 10
 *      USettingsMenuWidget --(GraphicsButton)--> THIS        @ ZOrder 20
 *  Neither parent is removed, so BackPressed() calls RemoveFromParent() on THIS
 *  widget and nothing else. ⛔ /Game/UI/WBP_MainMenu is NEVER OPENED by this
 *  lane.
 *
 *  ---------------------------------------------------------------------------
 *  🚨 THE `Back` SOFT-LOCK, AND WHY THIS CLASS IS THE ONLY LAYER THAT CAN FIX IT
 *  ---------------------------------------------------------------------------
 *  (board cl. 3b + 3b-WIDENED; qa/TASK-1114.md WARN-3 and LOOP-1 WARN-9)
 *
 *  USiegeGraphicsSettingsSubsystem::IsVideoModeChangePending() is true from the
 *  STAGED-vs-LastConfirmed comparison ALONE — SetScreenResolution() and
 *  SetWindowMode() stage without applying, so ONE stepper press is enough. While
 *  it is true the facade refuses:
 *      • every save            (RequestSaveSettings, cpp:1199-1207)
 *      • AND Auto-Detect       (AutoDetectQuality, cpp:522-529 — added by
 *                               TASK-1113 LOOP 1's blocker fix)
 *  …for the rest of the session, silently, with only a Log/Warning line. Both
 *  refusals are CORRECT: the benchmark's own ApplyHardwareBenchmarkResults saves
 *  the full config (GameUserSettings.cpp:1142) and would persist a video mode
 *  the player may not be able to see.
 *
 *  ⇒ Nothing below the widget knows the panel closed, so nothing below the
 *  widget can close the window. A stray stepper press followed by `Back` would
 *  otherwise soft-lock the WHOLE settings screen from two ordinary clicks.
 *  DiscardStagedVideoMode() is the fix, and it is called from EXACTLY TWO sites
 *  — BackPressed() and NativeDestruct() — which is ONE revert on the Back path
 *  because the function is guarded on IsVideoModeChangePending() and the first
 *  call clears it. See DiscardStagedVideoMode's own comment for the arithmetic.
 *
 *  ✅ RECONCILED BY TASK-1118, WHICH LANDED SECOND AND THEREFORE OWNS IT
 *  (board cl. 8; TASK-1115's own instruction was "call this one, or delete this
 *  one and own both"). ⛔ TASK-1118 CALLED THIS ONE. It added NO second discard
 *  and NO second revert: DiscardStagedVideoMode is still the ONE function in
 *  this class that calls the facade's RevertVideoModeChange(), and BackPressed()
 *  still reaches it exactly once. What TASK-1118 added to the Back path is
 *  DisarmVideoModeCountdown(), which stops a TIMER and hides a PROMPT and
 *  reverts NOTHING. See "THE COUNTDOWN" below for the arithmetic.
 *
 *  ---------------------------------------------------------------------------
 *  ⭐⭐ THE COUNTDOWN — TASK-1118 [GFX-REVERT], GFX-§4's WHOLE POINT
 *  ---------------------------------------------------------------------------
 *  A player who picks a mode their monitor cannot display CAN SEE NOTHING. So
 *  nothing on the recovery path may require a click, a hover, or a visible
 *  widget: the ten-second timer is the recovery, and the prompt is only the
 *  courtesy offered to the player who CAN see.
 *
 *      StepScreenResolution / StepWindowMode   (stage — unchanged)
 *          └─> BeginVideoModeConfirmation()
 *                  ├─ ⛔ !HasUnconfirmedVideoModeDifference() — the step was a
 *                  │  no-op, or the player stepped BACK to the mode they started
 *                  │  in ⇒ DISARM, DiscardStagedVideoMode, return.
 *                  │  ⛔ NOT IsVideoModeChangePending(): that one ors in a LATCH
 *                  │  raised by the first provisional apply, so asking it here
 *                  │  made this branch dead from press two onward and a wrapping
 *                  │  press re-applied and restarted the ten seconds
 *                  │  (qa/TASK-1119.md BLOCKER-1).
 *                  │  ⛔ And not merely "disarm": the latch has to be lowered or
 *                  │  the facade refuses every save for the session.
 *                  ├─ CanArmVideoModeCountdown() false (no world ⇒ no timer)
 *                  │  ⇒ ⛔ DO NOT APPLY. Staged-only is safe; an applied mode
 *                  │  with no auto-revert is the permanent lockout itself.
 *                  ├─ Graphics->ApplyVideoModeProvisional()   (apply, NEVER save)
 *                  └─ ArmVideoModeCountdown()                 (timer + prompt)
 *
 *      TickVideoModeCountdown()  — 1 s loop, ten ticks
 *          ├─ > 0 ⇒ repaint VideoModeConfirmText
 *          └─ = 0 ⇒ RevertSettingsPressed()   ⛔ the SAME body the button uses
 *
 *      KeepSettingsPressed()   ⇒ disarm, Graphics->ConfirmVideoModeChange()
 *                                 (which stamps LastConfirmed* AND saves — the
 *                                 save is the facade's, not ours: GFX-§4's
 *                                 "never SaveSettings on an unconfirmed mode"
 *                                 is enforced one layer down)
 *      RevertSettingsPressed() ⇒ disarm, DiscardStagedVideoMode(Graphics)
 *      BackPressed()           ⇒ disarm, DiscardStagedVideoMode(Graphics)
 *      NativeDestruct()        ⇒ disarm, DiscardStagedVideoMode(Graphics)
 *      the wrap-around branch  ⇒ disarm, DiscardStagedVideoMode(Graphics)
 *
 *  ⛔ FOUR CALLERS OF DiscardStagedVideoMode, ⛔ STILL EXACTLY ONE REVERT ON
 *  THE BACK PATH, and the arithmetic is unchanged from TASK-1115 because the
 *  guard is on STATE: BackPressed's call reverts and clears the window, and
 *  NativeDestruct's call then returns false. RevertSettingsPressed is not on
 *  the Back path — Back does not call it, and it cannot: pressing Back is not
 *  pressing Revert. Neither is the wrap-around branch: it is reached from a
 *  stepper press, and by the time Back is pressed it has already closed the
 *  window, so Back's own call is then the guarded no-op. ⛔ The invariant cl. (8)
 *  states is ONE REVERT PER BACK PRESS, not one call site — the thing it forbids
 *  is a HAND-ROLLED second revert body, which is exactly what calling this one
 *  function from four places avoids (qa/TASK-1119.md F-1).
 *
 *  ⛔ AND THE EXPIRY CANNOT DOUBLE-APPLY. The facade's RevertVideoModeChange()
 *  already does RevertVideoMode() + ApplyResolutionSettings(false) in that order
 *  (SiegeGraphicsSettingsSubsystem.cpp:900-925, re-read after TASK-1113's loop-1
 *  rewrite). This class adds NOTHING after it. A second apply on the expiry path
 *  would be a mode change fired at a player already looking at a black screen
 *  (TASK-1118 cl. 9; the manager's RULING B: the facade owns that follow-up
 *  ALONE).
 *
 *  ⛔ A STRANDED TICK IS INERT. TickVideoModeCountdown early-outs on
 *  bVideoModeCountdownActive, which the disarm clears BEFORE it touches the
 *  timer — so a callback already queued when the player pressed Back or Keep
 *  reverts nothing. Asserted, not assumed.
 *
 *  ---------------------------------------------------------------------------
 *  DEPENDENCY
 *  ---------------------------------------------------------------------------
 *  Reads/writes USiegeGraphicsSettingsSubsystem (TASK-1113) and NEVER an engine
 *  API directly — GFX-§3's seam clause ("the widget never calls an engine API
 *  directly") is why that facade exists. Null-safe throughout: no subsystem
 *  resolvable ⇒ every control renders DISABLED with a line saying why, logged
 *  ONCE, and never a crash (the USettingsMenuWidget::ShowRowUnavailable shape).
 *
 *  This file WILL NOT COMPILE ALONE and is not expected to; the host compile is
 *  TASK-1124's. ⛔ NOTHING IN THIS FILE HAS BEEN COMPILED OR EXECUTED — see
 *  "NO WITNESSED RED" in the handoff.
 */
UCLASS()
class GITCLAUDEUNREALTEST_API USiegeGraphicsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ════════════════════════════════════════════════════════════════════════
	//  ENTRY POINT
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  Creates the panel for OwningController and adds it to the viewport.
	 *
	 *  ⛔ THREE PARAMETERS, NONE DEFAULTED (SC-§33). A null MenuClass falls back
	 *  to this C++ class, which is the shipping state today: /Game/UI/
	 *  WBP_GraphicsMenu is RESERVED and UNAUTHORED.
	 *
	 *  @param OwningController the local player controller. Null ⇒ no panel,
	 *                          logged, ⛔ never fatal — a graphics menu that
	 *                          crashes the main menu is worse than no graphics
	 *                          menu.
	 *  @param MenuClass        /Game/UI/WBP_GraphicsMenu once it exists; null ⇒
	 *                          this class.
	 *  @param ZOrder           viewport Z order. USettingsMenuWidget passes 20 —
	 *                          ABOVE the settings panel's own 10.
	 */
	static USiegeGraphicsMenuWidget* CreateAndAddToViewport(
		APlayerController* OwningController,
		TSubclassOf<USiegeGraphicsMenuWidget> MenuClass,
		int32 ZOrder);

	// ════════════════════════════════════════════════════════════════════════
	//  PLAYER ACTIONS (BlueprintCallable so a future WBP_GraphicsMenu can drive
	//  the identical entry points instead of re-implementing them)
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  Dismisses THIS panel and nothing else — the settings panel underneath was
	 *  never removed and is already correct.
	 *
	 *  🚨 IT FIRST DISCARDS A STAGED, UNCONFIRMED VIDEO MODE. That is the board
	 *  cl. (3b-WIDENED) MUST, not tidiness: see the class comment.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void BackPressed();

	/** GFX-§6's Auto-Detect. Runs the hardware benchmark through the facade, then re-seeds every row (a benchmark moves ten sliders AND the resolution scale). */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void AutoDetectPressed();

	// ════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-1120 — THE READOUT (GFX-§7's first clause)
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  How often the panel's own readout re-reads the counter. ⛔ HALF A SECOND,
	 *  and both halves of that choice are deliberate:
	 *
	 *  ⛔ IT IS A TIMER PERIOD, NOT A TICK. The board forbids `NativeTick` here in
	 *  so many words, and the reason is not style: a per-frame callback that
	 *  exists only to print a frame-rate number would be a cost the number then
	 *  reports, which makes the instrument disagree with the thing it measures.
	 *
	 *  ⛔ AND IT IS ALSO THE SMOOTHING WINDOW, because the average is taken across
	 *  exactly the gap between two firings (`FSiegeFrameRateSample`). A shorter
	 *  period would give an unreadably twitchy number; a longer one would hide the
	 *  very hitches a player drags a slider to remove. ⇒ moving this constant
	 *  changes BOTH the repaint rate and the averaging window, on purpose — there
	 *  is no second number to keep in step with it.
	 */
	static constexpr float FrameRateReadoutIntervalSeconds = 0.5f;

	/**
	 *  The timer callback: close the window, publish the string, open the next.
	 *
	 *  ⛔ PUBLIC AND BlueprintCallable ONLY so the suite and a future
	 *  WBP_GraphicsMenu can drive the identical entry point. Nothing in the game
	 *  calls it except the timer.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void RefreshFrameRateReadout();

	/**
	 *  Re-seeds the in-match-counter toggle row from the persisted preference and
	 *  sets its enabled state. ⛔ Deliberately NOT part of RefreshAllRows(): that
	 *  function early-returns when the GRAPHICS facade is missing, and this row
	 *  reads a different subsystem entirely (`GFX-§3`'s named exception). Folding
	 *  it in would make one subsystem's absence silently blank the other's row.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void RefreshFrameRateCounterRow();

	// ════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-1118 — THE 10-SECOND "KEEP THESE SETTINGS?" (GFX-§4)
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  GFX-§4's countdown length. ⛔ TEN SECONDS, and it is the law's number, not
	 *  a taste.
	 *
	 *  🚨 ONE PLACE NAMES THIS NUMBER IN PROSE INSTEAD OF DERIVING IT:
	 *  SiegeGraphicsMenuText::DisplayHint ("...within 10 seconds...") in the .cpp.
	 *  ⛔ IF YOU MOVE THIS CONSTANT, MOVE THAT SENTENCE TOO — the prompt would
	 *  follow the constant and the hint would not, and neither the compiler nor
	 *  the suite can see the disagreement (gate qa/TASK-1119.md NIT-1).
	 */
	static constexpr float VideoModeConfirmSeconds = 10.0f;

	/**
	 *  The countdown's tick period. ⛔ ONE SECOND, and it is load-bearing twice:
	 *  it is the repaint rate of the visible timer AND the resolution at which
	 *  the expiry fires, because there is ONE timer doing both jobs. Two timers
	 *  would be two things to cancel and one of them would eventually be missed.
	 */
	static constexpr float VideoModeCountdownTickSeconds = 1.0f;

	/**
	 *  The player pressed "Keep". Disarms the countdown, then
	 *  ConfirmVideoModeChange() — which stamps LastConfirmed* and releases the
	 *  save. ⛔ THIS FUNCTION SAVES NOTHING ITSELF: GFX-§4's "never SaveSettings
	 *  on an unconfirmed video mode" is enforced inside the facade, and adding a
	 *  save here would put the rule in two places, one of which is silent when
	 *  broken.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void KeepSettingsPressed();

	/**
	 *  The player pressed "Revert" — AND the body the countdown's expiry calls.
	 *  ⛔ ONE BODY ON PURPOSE: a separate expiry path is a path that is written
	 *  once, never clicked, and therefore never noticed when it rots.
	 *
	 *  ⛔ It reverts through DiscardStagedVideoMode() and NOTHING ELSE — no
	 *  direct UGameUserSettings::RevertVideoMode(), no ApplyResolutionSettings
	 *  afterwards (TASK-1118 cl. 9).
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void RevertSettingsPressed();

	/**
	 *  One tick of the countdown. ⛔ PUBLIC, and for the same reason every input
	 *  thunk in this class is: the automation suite has NO WORLD and therefore no
	 *  timer manager, so this is the only way the EXPIRY PATH — the one path in
	 *  this feature that no player ever exercises deliberately — can be driven
	 *  and asserted at all (board cl. 2: "TEST THE EXPIRY PATH SPECIFICALLY").
	 *
	 *  ⛔ Inert unless the countdown is active, so a callback already queued when
	 *  the player pressed Back or Keep cannot revert anything.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void TickVideoModeCountdown();

	/** True while the "Keep these settings?" prompt is live. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsVideoModeCountdownActive() const { return bVideoModeCountdownActive; }

	/** Seconds left before the automatic revert. 0 when no countdown is running. */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	float GetVideoModeCountdownSecondsRemaining() const { return VideoModeCountdownSecondsRemaining; }

	/**
	 *  Re-reads EVERY value from the facade and pushes it onto the rows. Public
	 *  because it is also the delegate handler's body and the automation suite's
	 *  way to observe a seed without a Slate tree.
	 *
	 *  ⛔ IT WRITES NOTHING. A refresh that wrote back what it read would turn
	 *  every broadcast into a save.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void RefreshAllRows();

	/**
	 *  Fired at the widget for presentation ON TOP of what C++ already does to
	 *  the bound controls — once when a row is SEEDED (so a future
	 *  WBP_GraphicsMenu never opens stale: the qa/TASK-005 major-2 lesson) and
	 *  again whenever the observed value actually CHANGES. ⛔ Never on a no-op.
	 *
	 *  ⛔ FString / int32 ONLY — GFX-§2(d) and board cl. (3e). In particular
	 *  ⛔ NO FIntPoint: a resolution arrives here as its stepper INDEX plus its
	 *  already-composed label ("1920 x 1080"), because FIntPoint in a
	 *  BlueprintImplementableEvent parameter is a UHT COMPILE failure and nobody
	 *  in this lane holds a compiler.
	 *
	 *  @param SettingName  the facade's own FName token, as a string
	 *                      ("ShadowQuality", "ResolutionScale", …).
	 *  @param IntValue     the quality level, the stepper index, or 0/1 for a
	 *                      bool. -1 means "Custom" for the overall preset.
	 *  @param DisplayValue exactly what the row's ValueText now reads.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Siegebound|Graphics")
	void OnGraphicsRowChanged(const FString& SettingName, int32 IntValue, const FString& DisplayValue);

	// ════════════════════════════════════════════════════════════════════════
	//  PURE STATICS — the panel's whole decision layer, hoisted out of the
	//  widget so the automation suite can drive it with no world, no Slate and
	//  no viewport. Every one of these is exercised by
	//  Tests/SiegeGraphicsMenuTest.cpp.
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  🚨 THE board cl. (3b-WIDENED) FIX, AND THE ONLY REVERT SITE IN THIS CLASS.
	 *
	 *  Reverts a staged-but-unconfirmed video mode so the facade's two refusals
	 *  (every save, AND Auto-Detect) are released when the panel closes.
	 *
	 *  ⛔ GUARDED ON IsVideoModeChangePending(), WHICH IS WHAT MAKES THE
	 *  TWO-CALL-SITE SHAPE SAFE: BackPressed() calls it, then RemoveFromParent()
	 *  reaches NativeDestruct() which calls it again — and the second call sees a
	 *  closed window and returns false WITHOUT reverting. ⇒ EXACTLY ONE
	 *  RevertVideoModeChange() on the Back path, guaranteed by STATE rather than
	 *  by ordering (board cl. 3b: "never two").
	 *
	 *  ⛔ IT CALLS THE FACADE'S RevertVideoModeChange() AND NOTHING ELSE. Do NOT
	 *  unwrap it (the engine's own RevertVideoMode() applies NOTHING —
	 *  GameUserSettings.cpp:276-285) and do NOT add an ApplyResolutionSettings
	 *  after it; the facade already does both, in that order, and a second apply
	 *  is a mode change fired at a player who may already be looking at a black
	 *  screen (TASK-1118 cl. 9).
	 *
	 *  @return true when a revert was actually issued.
	 */
	static bool DiscardStagedVideoMode(USiegeGraphicsSettingsSubsystem* Graphics);

	/**
	 *  ⛔ board cl. (3c). The ONLY question this panel labels "Custom" from.
	 *
	 *  GetOverallScalabilityLevel() answers over ELEVEN groups (LandscapeQuality
	 *  included) AND requires ResolutionQuality to match the preset's canonical
	 *  scale — so Auto-Detect can strand the INVISIBLE eleventh, or a
	 *  resolution-scale drag can move the scale, and the preset then reads
	 *  "Custom" FOREVER with all ten visible sliders in agreement and nothing on
	 *  screen able to fix it.
	 *  ⛔ Do NOT hand-roll a second Custom rule here — that is the TASK-931
	 *  "N hides" defect one lane over. This forwards to the facade's own
	 *  IsOverallQualityCustomAcrossVisibleGroups().
	 */
	static bool IsCustomForDisplay(const USiegeGraphicsSettingsSubsystem* Graphics);

	/** Level (0-4) → GFX-§5 slider value on the 0..1 bar with StepSize 0.25. Clamps. Exact in float: 0, 0.25, 0.5, 0.75, 1.0. */
	static float LevelToSliderValue(int32 Level);

	/** GFX-§5 slider value (0..1) → level (0-4). Rounds to the nearest detent, then clamps. */
	static int32 SliderValueToLevel(float SliderValue);

	/** Stepper arithmetic, WRAPPING, shared by all three steppers. Count <= 0 returns 0. */
	static int32 StepIndex(int32 CurrentIndex, int32 Delta, int32 Count);

	/**
	 *  ⛔ board cl. (5) + GFX-§9: which quality groups change something that is
	 *  read at BUILD TIME and therefore only appears at the NEXT MATCH START.
	 *
	 *  ⚠️ MEASURED, NOT COPIED FROM THE BOARD (SC-§101). Board cl. (5) names
	 *  "Foliage, View Distance and Effects" — that list predates the GFX-§9
	 *  correction of 2026-09-07, which STRUCK two of the three project levers:
	 *    • View Distance — the ENGINE's [ViewDistanceQuality@N] already sets
	 *      r.ViewDistanceScale, which scales HISM cull distances IMMEDIATELY.
	 *      GetViewDistanceScale() is 1.0 at every level BY MEASUREMENT, so there
	 *      is no project-side deferral to announce. Saying "next match" here
	 *      would UNDERSELL a control that works now.
	 *    • Effects — the volumetric-fog lever was struck (wrong group AND wrong
	 *      symbol); [EffectsQuality@N] applies through stock scalability CVars,
	 *      immediately. There is no scatter-driven Effects lever at all.
	 *    • Foliage — the ONE surviving Tier-D lever. The battlefield scatter's
	 *      cull band is set inside the build path
	 *      (ASiegeBattlefieldScatter::RunScatterPasses, BattlefieldScatter.cpp:363),
	 *      which runs ONCE PER MATCH. That genuinely defers.
	 *  ⇒ FoliageQuality is the only true member today. Flagged for TASK-1116.
	 */
	static bool DoesGroupApplyAtNextMatchStart(FName GroupName);

	/**
	 *  The hint line under a group row, or an empty string for the seven groups
	 *  that need none. Pure so the wording is testable without a Slate tree.
	 *
	 *  @param bVolumetricFogEnabled the facade's ShouldEnableVolumetricFog() —
	 *         GFX-§9 rules the PANEL its read-only consumer, which is what gives
	 *         that shipped getter a real caller (SC-§36.1).
	 */
	static FString ComposeGroupHint(FName GroupName, bool bVolumetricFogEnabled);

	/** "Low" / … / "Cinematic", or "Custom". Level display names come from the ENGINE (Scalability::GetScalabilityNameFromQualityLevel) via the facade — never a hand-written array. */
	static FString ComposeLevelValueText(int32 Level, bool bCustom);

	/**
	 *  ⭐ The countdown's visible line: "Keep these settings? Reverting in N
	 *  seconds." Pure, so the wording AND the arithmetic are testable with no
	 *  Slate tree and no timer.
	 *
	 *  ⛔ IT MUST NAME THE NUMBER. A prompt that says only "Keep these settings?"
	 *  gives the player who CAN see the screen no reason to hurry and no way to
	 *  tell a hung menu from a working one; GFX-§4 asks for a countdown, not a
	 *  question. Singular at 1 ("1 second"), and 0 says the revert is happening
	 *  NOW rather than counting into the negatives.
	 */
	static FString ComposeVideoModeCountdownText(int32 SecondsRemaining);

	/**
	 *  Where the overall-preset handle sits while the panel reads "Custom".
	 *  The handle has to be SOMEWHERE; the rounded mean of the visible levels is
	 *  the least misleading place, and GFX-§5 puts the truth in the LABEL, not in
	 *  the handle position. An empty array returns 0.
	 */
	static int32 AveragedLevel(const TArray<int32>& Levels);

	// ════════════════════════════════════════════════════════════════════════
	//  TREE + TEST SEAMS
	// ════════════════════════════════════════════════════════════════════════

	/**
	 *  Builds the code-authored tree. Called from RebuildWidget() BEFORE
	 *  Super::RebuildWidget() — the order is condition (c) and it is load-bearing.
	 *  Returns immediately when an asset-authored tree is present (condition (b)).
	 *
	 *  ⚠️ PUBLIC, unlike USettingsMenuWidget::ConstructSettingsTree(), and for one
	 *  reason: the automation suite builds the tree with Initialize() +
	 *  ConstructGraphicsTree() so it can assert the 30 group widgets exist and
	 *  carry the right names WITHOUT taking a Slate widget.
	 *
	 *  ⚠️ A SECOND CALL IS A NO-OP, AND THE REASON IS THE ESCAPE HATCH, NOT THE
	 *  PER-CHILD NULL CHECKS: the first call assigns WidgetTree->RootWidget, and
	 *  the condition (b) guard at the top of the body returns immediately when
	 *  that is non-null. (The per-row horizontal boxes are NOT null-guarded
	 *  individually — they are unpinned layout, created fresh each time the body
	 *  actually runs — so the guard is what keeps a rebuild from doubling every
	 *  row. Do not remove it and rely on the members being non-null.)
	 */
	void ConstructGraphicsTree();

	/**
	 *  ⛔ AUTOMATION TESTS ONLY — nothing in the game may call this. Points this
	 *  panel at a scratch USiegeGraphicsSettingsSubsystem instead of resolving
	 *  one off the world's game instance, which a bare NewObject widget does not
	 *  have. The facade's own SetGameUserSettingsForAutomationTests idiom, cloned
	 *  one layer up.
	 */
	void SetGraphicsSubsystemForAutomationTests(USiegeGraphicsSettingsSubsystem* InGraphics);

	/**
	 *  ⛔ AUTOMATION TESTS ONLY — nothing in the game may call this. Same idiom as
	 *  the line above, for the OTHER subsystem this panel reads: the FPS-counter
	 *  preference lives in USiegeSettingsSubsystem (`GFX-§3`'s one named
	 *  exception), which a bare NewObject widget also cannot resolve.
	 */
	void SetSettingsSubsystemForAutomationTests(USiegeSettingsSubsystem* InSettings);

	/**
	 *  ⛔ AUTOMATION TESTS ONLY. Arms the countdown WITHOUT asking the world for
	 *  a timer, so the suite can drive TickVideoModeCountdown() by hand.
	 *
	 *  ⚠️ THIS SEAM IS WHY THE NO-TIMER REFUSAL IS TESTABLE AT ALL, AND IT IS THE
	 *  FACADE'S OWN bSuppressEngineApplyForAutomationTests IDIOM ONE LAYER UP. A
	 *  bare NewObject widget has no world; without the seam, EVERY countdown test
	 *  would be measuring the no-world branch instead of the countdown, and the
	 *  expiry path — the whole point of this row — would have no coverage at all.
	 *  With it, both branches are asserted: one test leaves it OFF and proves the
	 *  panel refuses to apply a mode it cannot auto-revert; the rest turn it ON
	 *  and drive the ten ticks.
	 */
	void SetVideoModeCountdownDrivenManuallyForAutomationTests(bool bInDrivenManually);

	/** Test accessor: the slider for a canonical group name, or null. */
	USlider* FindGroupSlider(FName GroupName) const;

	/** Test accessor: the live level text for a canonical group name, or null. */
	UTextBlock* FindGroupValueText(FName GroupName) const;

	/** Test accessor: the label for a canonical group name, or null. */
	UTextBlock* FindGroupLabelText(FName GroupName) const;

	/**
	 *  Seed-then-bind: reads every live value, pushes it to the rows, THEN arms
	 *  the delegates. This is NativeConstruct()'s whole body.
	 *
	 *  ⚠️ PUBLIC so the automation suite can arm the panel without a Slate tree —
	 *  there is no other way to observe WHICH EVENT each write is bound to, and
	 *  "the write is on commit, not on value-changed" (board cl. 3a) is a claim
	 *  about the BINDING, not only about the handler body. Idempotent: it unbinds
	 *  everything first.
	 */
	void SeedAndBind();

	/** Symmetric unbind of everything SeedAndBind armed. Public for the same reason. */
	void UnbindAll();

	// ════════════════════════════════════════════════════════════════════════
	//  INPUT THUNKS
	//
	//  ⚠️ PUBLIC, and deliberately: a slider cannot be dragged headlessly, so
	//  these are the only way an automation test can press this panel. Making
	//  them public is what lets the suite prove BOTH halves of board cl. (3a) —
	//  that the value-changed path writes NOTHING (behaviour) and that the write
	//  is armed on the capture-end events (binding). Neither half alone is
	//  evidence: a handler that never writes proves nothing if the write is bound
	//  elsewhere, and a binding proves nothing if the handler is empty.
	// ════════════════════════════════════════════════════════════════════════

	// 🚨 board cl. (3a). THE WRITE HAPPENS ON **COMMIT**, NEVER ON
	// OnValueChanged. USlider::OnValueChanged fires per frame of a drag and
	// every facade setter ends in a SaveConfig FILE WRITE
	// (GameUserSettings.cpp:683), so a 50→100 drag on the continuous
	// resolution-scale bar would be up to FIFTY ini writes (gate F-5). The
	// OnValueChanged thunks below update the LABEL and NOTHING ELSE; the
	// Committed thunks do the writing.
	//
	// ⚠️ ONE THUNK DRIVES ALL TEN GROUP SLIDERS. USlider's delegates carry no
	// sender, so instead of ten near-identical handlers (the copy-paste this row
	// exists to avoid) each thunk LOOPS the ten canonical names and acts on the
	// one whose slider disagrees with the facade. At most one can.
	//
	// 🚨 board cl. (3a). THE WRITE HAPPENS ON **COMMIT**, NEVER ON
	// OnValueChanged. USlider::OnValueChanged fires per frame of a drag and
	// every facade setter ends in a SaveConfig FILE WRITE
	// (GameUserSettings.cpp:683), so a 50→100 drag on the continuous
	// resolution-scale bar would be up to FIFTY ini writes (gate F-5). The
	// OnValueChanged thunks below update the LABEL and NOTHING ELSE; the
	// CaptureEnd thunks do the writing.
	//
	// ⚠️ ONE THUNK DRIVES ALL TEN GROUP SLIDERS. USlider's delegates carry no
	// sender, so instead of ten near-identical handlers (the copy-paste this row
	// exists to avoid) each thunk LOOPS the ten canonical names and acts on the
	// one whose slider disagrees with the facade. At most one can.

	/** All ten group sliders' OnValueChanged. ⛔ LABEL ONLY — writes nothing. */
	UFUNCTION()
	void HandleGroupSliderValueChanged(float NewValue);

	/** All ten group sliders' OnMouseCaptureEnd / OnControllerCaptureEnd. This is where a group is written. */
	UFUNCTION()
	void HandleGroupSliderCommitted();

	/** Overall preset slider, OnValueChanged. ⛔ LABEL ONLY. */
	UFUNCTION()
	void HandleOverallSliderValueChanged(float NewValue);

	/** Overall preset slider, commit. Writes the preset — and RE-SEEDS THE RESOLUTION-SCALE ROW, because the engine moves it too (board cl. 3d). */
	UFUNCTION()
	void HandleOverallSliderCommitted();

	/** Resolution-scale slider, OnValueChanged. ⛔ LABEL ONLY — this is the fifty-file-writes control. */
	UFUNCTION()
	void HandleResolutionScaleValueChanged(float NewValue);

	/** Resolution-scale slider, commit. The one write. */
	UFUNCTION()
	void HandleResolutionScaleCommitted();

	// ── BUTTON / CHECKBOX THUNKS ────────────────────────────────────────────

	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleAutoDetectClicked();
	UFUNCTION() void HandleScreenResolutionPrevClicked();
	UFUNCTION() void HandleScreenResolutionNextClicked();
	UFUNCTION() void HandleWindowModePrevClicked();
	UFUNCTION() void HandleWindowModeNextClicked();
	UFUNCTION() void HandleFrameRateLimitPrevClicked();
	UFUNCTION() void HandleFrameRateLimitNextClicked();
	UFUNCTION() void HandleVSyncChanged(bool bIsChecked);
	UFUNCTION() void HandleKeepSettingsClicked();
	UFUNCTION() void HandleRevertSettingsClicked();

	/** ⭐ TASK-1120: the in-match-counter toggle. Writes through USiegeSettingsSubsystem, ⛔ never through the graphics facade — the two stores are not interchangeable. */
	UFUNCTION() void HandleShowFrameRateCounterChanged(bool bIsChecked);

	/** USiegeGraphicsSettingsSubsystem::OnGraphicsSettingsChanged handler. Re-reads EVERYTHING rather than filtering on the token — see RefreshAllRows. */
	UFUNCTION()
	void HandleGraphicsSettingsChanged(FName SettingName);

	/**
	 *  ⭐ TASK-1120: USiegeSettingsSubsystem::OnSettingsChanged handler — a
	 *  DIFFERENT delegate on a DIFFERENT subsystem from the one above.
	 *
	 *  ⚠️ AND THIS ONE *DOES* FILTER ON THE TOKEN, which is the opposite of its
	 *  neighbour's documented rule, deliberately: that store carries settings this
	 *  panel does not own (the assistant confirm toggle), so re-seeding
	 *  unconditionally would push the checkbox on an unrelated broadcast. The
	 *  filter is also what makes the payload NAME load-bearing rather than
	 *  decorative — a setter that broadcast the wrong token would leave this row
	 *  stale, and a test asserts the token rather than a broadcast count.
	 */
	UFUNCTION()
	void HandleSettingsChanged(FName SettingName);

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/**
	 *  ⭐ TASK-1417 [MENU-NAV-GRAPHICS] — 🧑 "scrollable with the outline".
	 *
	 *  ⛔ THIS IS A FOCUS HOOK, NOT A KEY HANDLER. It reads nothing from the
	 *  keyboard, consumes nothing, returns nothing and decides no navigation:
	 *  USiegeMenuInputSubsystem owns Up / Down / Left / Right / Accept for the
	 *  whole menu (TASK-1406 / TASK-1409). All this override does is notice that
	 *  the ring landed somewhere inside RootScrollBox and ask the scroll box to
	 *  bring that row on screen. A `NativeOnKeyDown` here would be the
	 *  wrong-layer relapse the MENU-NAV epic exists to remove; there is none.
	 *
	 *  See ScrollFocusStopIntoView() for why the engine's own
	 *  `ScrollWhenFocusChanges` is ALSO set and which of the two actually fires.
	 */
	virtual void NativeOnFocusChanging(
		const FWeakWidgetPath& PreviousFocusPath,
		const FWidgetPath& NewWidgetPath,
		const FFocusEvent& InFocusEvent) override;
	//~ End UUserWidget interface

	/** Null-safe resolve: the automation override first, then this widget's world's game instance. */
	USiegeGraphicsSettingsSubsystem* ResolveGraphicsSubsystem() const;

	/** ⭐ TASK-1120: the same shape for the PROFILE-SCOPED store that owns bShowFrameRateCounter. Null is ordinary and never fatal — the row just renders disabled. */
	USiegeSettingsSubsystem* ResolveSettingsSubsystem() const;

	/** No subsystem: disable every control, say why in StatusText, log ONCE. Never a crash. */
	void ShowPanelUnavailable();

	// ════════════════════════════════════════════════════════════════════════
	//  PINNED CHILDREN — GFX-§10, character-for-character.
	//  All BindWidgetOptional, never BindWidget: an asset-authored
	//  WBP_GraphicsMenu using these exact names binds here and the code-authored
	//  branch never runs (condition (b)). Nothing below is ever hard-required.
	// ════════════════════════════════════════════════════════════════════════

	/** Hit-test VISIBLE modal plate; the tree root. See the class comment — GFX-§2(f), correctness not styling. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BackdropBorder;

	/** Caps the panel width so 19 rows do not stretch edge to edge on an ultrawide. Unpinned (GFX-§10 lists no width host); named in the handoff. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PanelSizeBox;

	/**
	 *  ⛔ board cl. (4): SCROLLING IS NOT OPTIONAL. Nineteen rows do not fit a
	 *  fixed panel at 1080p, and a panel whose bottom rows are off-screen is
	 *  INDISTINGUISHABLE FROM MISSING FEATURES. Unpinned by GFX-§10 (which
	 *  predates the row count); named in the handoff so an asset-authored tree
	 *  can adopt the same name.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> RootScrollBox;

	/** The panel column, INSIDE the scroll box. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootPanel;

	/** "Graphics". */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	/** The panel-wide status line — the surface that says WHY everything is disabled when the subsystem is missing. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** GFX-§6's Auto-Detect button. */
	/**
	 *  ⭐⭐ TASK-1120 — `GFX-§10`'s pinned readout, and `GFX-§7`'s first clause.
	 *  Live FPS **and** frame time in ms, re-composed twice a second off a timer.
	 *
	 *  ⚠️ IT SITS AT THE TOP OF THE COLUMN, ABOVE AUTO-DETECT, AND IT SCROLLS WITH
	 *  THE COLUMN. Declared rather than glossed: the panel is a UScrollBox with 18
	 *  rows, so dragging the Shadow slider two thirds of the way down scrolls this
	 *  line off the top. ⛔ THAT IS TOLERABLE ONLY BECAUSE IT IS THE SECONDARY HALF
	 *  OF `GFX-§7`. `L_MainMenu` has no ~340 trees, no ~24k grass instances, no
	 *  Lumen-lit battlefield and no volumetric fog, so this number is a sanity
	 *  check, ⛔ NOT the tuning instrument. The tuning instrument is
	 *  USiegeFrameRateCounterWidget at the bottom of this header, which draws in
	 *  `L_Arena`, never scrolls, and is the reason this row is not a lie.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRateReadoutText;

	/**
	 *  ⭐ TASK-1120 — the opt-in for the in-match counter. Derived names, and by
	 *  the gate's own `TASK-1119` F-8 ruling derived names stay UNPINNED: only
	 *  `bShowFrameRateCounter` and `SettingName_ShowFrameRateCounter` are pinned
	 *  by `GFX-§10`, and both are on the settings store, not here.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> ShowFrameRateCounterCheckBox;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShowFrameRateCounterLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShowFrameRateCounterHintText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UButton> AutoDetectButton;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AutoDetectLabelText;

	/** GFX-§6's "this will take a moment" line — the benchmark stalls and an unannounced stall reads as a hang. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AutoDetectHintText;

	// ── TIER A: the overall preset (GFX-§5: same 5 detents + a read-only Custom)
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<USlider> OverallQualitySlider;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverallQualityLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverallQualityValueText;

	// ════════════════════════════════════════════════════════════════════════
	//  TIER B — THE TEN QUALITY GROUPS.
	//  ⛔ THE NAMES ARE THE INI SPELLING (GFX-§10, RATIFIED 2026-09-07 on
	//  TASK-1113's F-2). `EffectsQuality*` and `PostProcessQuality*` —
	//  ⛔ NEVER `VisualEffect*`, ⛔ NEVER `PostProcessing*`. The engine's two
	//  divergent wrapper spellings live at exactly four commented sites inside
	//  SiegeGraphicsSettingsSubsystem.cpp and NOWHERE ELSE; either of them
	//  appearing here is a QA FAIL, not a nit.
	//  ⛔ THE ELEVENTH ENGINE GROUP, LandscapeQuality, IS DELIBERATELY ABSENT:
	//  it has no [LandscapeQuality@N] section at any level, no auto-detect
	//  threshold table, and no Landscape in this project ⇒ the slider would move
	//  and change nothing. A draggable row that provably cannot change a pixel
	//  teaches the player the menu lies (GFX-§9).
	// ════════════════════════════════════════════════════════════════════════

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    ViewDistanceQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ViewDistanceQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ViewDistanceQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    AntiAliasingQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AntiAliasingQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AntiAliasingQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    ShadowQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ShadowQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ShadowQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    GlobalIlluminationQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GlobalIlluminationQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GlobalIlluminationQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    ReflectionQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ReflectionQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ReflectionQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    PostProcessQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PostProcessQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PostProcessQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    TextureQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TextureQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TextureQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    EffectsQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EffectsQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> EffectsQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    FoliageQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FoliageQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FoliageQualityValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<USlider>    ShadingQualitySlider;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ShadingQualityLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ShadingQualityValueText;

	/** GFX-§9's ruled consumer for ShouldEnableVolumetricFog(): a read-only line beside the Shadows row. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShadowQualityHintText;

	/** board cl. (5) / GFX-§9: the "applies at the NEXT match start" notice. A control that silently does nothing until later is worse than one that admits it. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FoliageQualityHintText;

	// ── TIER C: resolution scale — the ONE genuinely continuous control ──────
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionScaleLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionScaleValueText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionScaleHintText;

	// ── TIER C: the display steppers (GFX-§5: unordered sets, NEVER a slider) ─
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScreenResolutionLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScreenResolutionValueText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    ScreenResolutionPrevButton;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    ScreenResolutionNextButton;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WindowModeLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WindowModeValueText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    WindowModePrevButton;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    WindowModeNextButton;

	/** The one line that describes what the two display steppers actually do. ⛔ REWRITTEN BY TASK-1118 — see the .cpp text block; the TASK-1115 wording described the staged-only interim. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplayHintText;

	// ── TASK-1118: GFX-§4's REVERT PROMPT (GFX-§10 pins all four names) ──────
	//
	// ⛔ COLLAPSED BEFORE THE PANEL IS SEEDED AND SHOWN ONLY BY
	// ArmVideoModeCountdown().
	//
	// ⛔ THE INVARIANT IS NOT "HOW MANY WRITERS" BUT "HOW MANY CAN SHOW IT", AND
	// THE ANSWER IS ONE. Its visibility has exactly FOUR writers in this file —
	// BuildVideoModeConfirmRow (Collapsed), SeedAndBind (Collapsed), the arm
	// (Visible) and the disarm (Collapsed) — and THREE OF THE FOUR ONLY EVER
	// COLLAPSE IT. A second writer that could make it Visible is how a prompt gets
	// left on screen over a mode nobody is being asked about.
	// ⛔ RefreshAllRows() is not one of them.
	//
	// ⚠️ SeedAndBind's is not redundant with BuildVideoModeConfirmRow's: the whole
	// of BuildVideoModeConfirmRow is skipped on an ASSET-AUTHORED tree
	// (ConstructGraphicsTree's condition (b)), so without the seed writer a future
	// WBP_GraphicsMenu binding these four pinned names would open with the prompt
	// already on screen and no countdown behind it (gate qa/TASK-1119.md NIT-3).
	//
	// ⚠️ THE PROMPT IS NOT THE MECHANISM. Board cl. (2): the player this row
	// protects cannot see, so the timer must fire with no widget involved. Every
	// widget below may be null — from an asset tree that omits it, or from a
	// failed construct — and the countdown still reverts on time.

	/** The prompt's plate, sited immediately under the two display steppers so it appears where the player just clicked. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> VideoModeConfirmBorder;

	/** "Keep these settings? Reverting in N seconds." — repainted every tick by ComposeVideoModeCountdownText. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VideoModeConfirmText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UButton> KeepSettingsButton;

	/** Unpinned by GFX-§10, but DERIVED from the pinned button name by the same "<Base>Button / <Base>LabelText" rule the rest of the file uses. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeepSettingsLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RevertSettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RevertSettingsLabelText;

	// ── TIER C: VSync + the frame-rate ladder ───────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UCheckBox>  VSyncCheckBox;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> VSyncLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FrameRateLimitLabelText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FrameRateLimitValueText;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    FrameRateLimitPrevButton;
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional)) TObjectPtr<UButton>    FrameRateLimitNextButton;

	/** Dismisses this panel only. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BackLabelText;

private:

	/** The three widgets of one group row, resolved from a canonical FName. Pointers TO the UPROPERTY members, so a row can be built and re-seeded in a loop. */
	struct FGroupRowWidgets
	{
		TObjectPtr<USlider>*    Slider = nullptr;
		TObjectPtr<UTextBlock>* Label  = nullptr;
		TObjectPtr<UTextBlock>* Value  = nullptr;

		bool IsComplete() const { return Slider && Label && Value; }
	};

	/** The ONE place a canonical group name is mapped to its three members. Ten branches, one per group, and a name test proves each pairing. */
	FGroupRowWidgets ResolveGroupRow(FName GroupName);

	/** Builds one group row into RootPanel. Called ten times from a loop over GetQualityGroupNames(). */
	void BuildQualityGroupRow(FName GroupName);

	// ── TREE BUILDERS ───────────────────────────────────────────────────────
	//
	// Five shapes cover all nineteen rows. Each one honours GFX-§2 condition (b)
	// the same way — it constructs a child ONLY when the member handed to it is
	// still null — so an asset-authored tree short-circuits every one of them,
	// and each names its widgets by DERIVING them from BaseName so a GFX-§10
	// triple cannot be mistyped.

	/**
	 *  Label + slider + live value, on one horizontal row.
	 *  @param bDetented true ⇒ USlider::MouseUsesStep is set, which is what makes
	 *         StepSize real for a MOUSE drag. ⛔ See the comment at that line: the
	 *         engine default is false and StepSize alone would give a smooth,
	 *         lying slider over five discrete states (GFX-§5).
	 */
	void BuildSliderRow(
		FName BaseName,
		TObjectPtr<USlider>& OutSlider,
		TObjectPtr<UTextBlock>& OutLabel,
		TObjectPtr<UTextBlock>& OutValue,
		const FString& LabelText,
		float MinValue,
		float MaxValue,
		float StepSize,
		bool bDetented);

	/** Label + "<" + value + ">", for the three GFX-§5 stepper controls (unordered or unevenly-spaced sets, where a slider would misrepresent the spacing). */
	void BuildStepperRow(
		FName BaseName,
		TObjectPtr<UTextBlock>& OutLabel,
		TObjectPtr<UTextBlock>& OutValue,
		TObjectPtr<UButton>& OutPrev,
		TObjectPtr<UButton>& OutNext,
		const FString& LabelText);

	/** One "<" / ">" button. Returns ExistingButton untouched when it is already bound from an asset tree. */
	UButton* BuildStepButton(FName ButtonName, UButton* ExistingButton, const TCHAR* Glyph, UHorizontalBox* RowBox);

	/** A full-width button whose label is its content — the shipped WBP_MainMenu / USettingsMenuWidget idiom. Used by Auto-Detect and Back. */
	void BuildLabelledButtonRow(
		FName BaseName,
		TObjectPtr<UButton>& OutButton,
		TObjectPtr<UTextBlock>& OutLabel,
		const FString& LabelText);

	/** An indented, wrapped, dimmed explanatory line under the row it describes. An empty HintText builds nothing. */
	void BuildHintRow(FName HintName, TObjectPtr<UTextBlock>& OutHint, const FString& HintText);

	/** TASK-1118: the revert prompt's plate + line + two buttons, built COLLAPSED. Condition (b)-tolerant like every other builder here. */
	void BuildVideoModeConfirmRow();

	/** One button inside the prompt's row box. Returns the existing button untouched when an asset tree already supplied it. */
	UButton* BuildConfirmButton(
		FName BaseName,
		TObjectPtr<UButton>& OutButton,
		TObjectPtr<UTextBlock>& OutLabel,
		const FString& LabelText,
		UHorizontalBox* RowBox);

	// ── COUNTDOWN INTERNALS (TASK-1118) ─────────────────────────────────────

	/**
	 *  ⭐ THE SEAM TASK-1115 LEFT, FILLED. Called by BOTH display steppers after
	 *  they stage. Decides — in this order — whether there is anything to confirm,
	 *  whether an auto-revert is possible at all, and only then applies.
	 *
	 *  @return true when a provisional apply happened and the countdown is armed.
	 */
	bool BeginVideoModeConfirmation(USiegeGraphicsSettingsSubsystem* Graphics);

	/**
	 *  🚨 THE FAIL-SAFE, AND IT IS NOT DEFENSIVE PROGRAMMING — IT IS GFX-§4.
	 *  Without a timer manager there is no auto-revert, and a provisional apply
	 *  with no auto-revert IS the permanent lockout this row exists to prevent.
	 *  So when this returns false the panel STAGES ONLY and applies nothing:
	 *  a display change that does not happen is recoverable; one that happens
	 *  and cannot be undone is not.
	 */
	bool CanArmVideoModeCountdown() const;

	/** Starts (or RESTARTS) the ten seconds, shows the prompt, scrolls it into view. */
	void ArmVideoModeCountdown();

	/** Stops the timer, hides the prompt, zeroes the remaining time. ⛔ REVERTS NOTHING — it is not a revert site and must never become one. */
	void DisarmVideoModeCountdown();

	/** Pushes the current remaining seconds onto VideoModeConfirmText. */
	void RefreshVideoModeCountdownText();

	// ── STEPPER BODIES ──────────────────────────────────────────────────────

	/** ⛔ STAGES a resolution — it applies nothing. TASK-1118's provisional apply + countdown is the seam marked in the body. */
	void StepScreenResolution(int32 Delta);

	/** ⛔ STAGES a window mode — see StepScreenResolution. int32 throughout; EWindowMode::Type never crosses this boundary. */
	void StepWindowMode(int32 Delta);

	/** Applies AND saves immediately: a frame-rate cap cannot make the screen unreadable, so GFX-§4's confirm-or-revert does not apply to it. */
	void StepFrameRateLimit(int32 Delta);

	/** Pushes one group's live level onto its slider + value text, and fires the BIE only when the pushed value is new for this widget instance. */
	void ApplyGroupLevelToRow(FName GroupName, int32 Level);

	/**
	 *  Enables or disables — in one call — every interactive control THE GRAPHICS
	 *  FACADE OWNS. Its only caller is the null-facade path.
	 *
	 *  ⛔ `ShowFrameRateCounterCheckBox` is NOT in it (TASK-1120), because that row
	 *  writes through USiegeSettingsSubsystem instead; its enabled state has one
	 *  writer, RefreshFrameRateCounterRow(). The reasoning is at the call site.
	 */
	void SetAllControlsEnabled(bool bEnabled);

	/** ⛔ Automation only. Non-null wins over the world's game instance in ResolveGraphicsSubsystem(). */
	UPROPERTY(Transient)
	TObjectPtr<USiegeGraphicsSettingsSubsystem> GraphicsOverrideForAutomationTests = nullptr;

	/** ⛔ Automation only. Non-null wins over the world's game instance in ResolveSettingsSubsystem(). */
	UPROPERTY(Transient)
	TObjectPtr<USiegeSettingsSubsystem> SettingsOverrideForAutomationTests = nullptr;

	// ── READOUT INTERNALS (TASK-1120) ───────────────────────────────────────

	/** ⭐ TASK-1120: builds the readout line and the in-match-counter toggle beneath it. Condition-(b) tolerant like every other builder here. */
	void BuildFrameRateReadoutRows();

	/** Starts the 0.5 s readout timer. No world (a bare NewObject widget) ⇒ no timer and the line keeps its pending text — never a crash, never a stale number. */
	void ArmFrameRateReadout();

	/** Stops the readout timer and forgets the open window. Called from NativeDestruct so a callback cannot arrive at a widget being torn down. */
	void DisarmFrameRateReadout();

	/** The one repeating 0.5 s timer behind the panel's readout. ⛔ SEPARATE from the countdown's handle: the two have different periods, different lifetimes and different exits, and sharing one would couple them. */
	FTimerHandle FrameRateReadoutTimerHandle;

	/** The open measurement window. Plain (non-UPROPERTY) member: it holds two numbers and a bool, no UObject. */
	FSiegeFrameRateSample FrameRateSample;

	/** Last level pushed per group — drives the "no BIE on a no-op" rule. */
	TMap<FName, int32> LastPushedGroupLevel;

	/** True while RefreshAllRows / a seed is pushing values, so a slider's own OnValueChanged echo cannot be mistaken for a player action. */
	bool bSuppressRowEcho = false;

	/** Latches the "subsystem unavailable" log to ONE line per widget instance. */
	bool bLoggedSubsystemUnavailable = false;

	// ── COUNTDOWN STATE (TASK-1118) ─────────────────────────────────────────

	/** The one repeating 1 s timer. Cleared by DisarmVideoModeCountdown() and by nothing else. */
	FTimerHandle VideoModeCountdownTimerHandle;

	/** Seconds left. Decremented by the tick period, never by delta time — so ten ticks is exactly ten seconds and the visible number never skips. */
	float VideoModeCountdownSecondsRemaining = 0.0f;

	/**
	 *  ⛔ THE RE-ENTRANCY GUARD, and the reason the disarm clears it FIRST. A
	 *  timer callback can already be queued when the player presses Keep, Revert
	 *  or Back; this flag is what makes that callback inert instead of a second
	 *  revert of a mode the player has since chosen.
	 */
	bool bVideoModeCountdownActive = false;

	/** ⛔ Automation only — see SetVideoModeCountdownDrivenManuallyForAutomationTests. */
	bool bDriveVideoModeCountdownManuallyForAutomationTests = false;

	// ════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-1417 [MENU-NAV-GRAPHICS] — THE SCREEN ANSWERS THE KEYBOARD, AND
	//  THE LIST FOLLOWS THE RING.
	//
	//  🧑 "we want to make sure everywhere in the menu can be scrollable with the
	//  outline and arrow keys such that an agent can navigate the entire menu."
	//
	//  ⛔ WHAT THIS ROW ADDS IS FOUR SHORT FUNCTIONS AND NO STATE. Everything that
	//  walks the tree, reads IsFocusable, places the ring, steps a slider and logs
	//  the count lives in USiegeMenuInputSubsystem (TASK-1406 / TASK-1409). This
	//  file's entire contribution is (a) saying WHEN this screen is the one the
	//  player is looking at and (b) keeping the focused row ON SCREEN.
	//
	//  ⛔ NO KEY HANDLER. ⛔ NO SetKeyboardFocus / SetUserFocus. ⛔ NO navigation
	//  rule table. ⛔ NO IsFocusable write. ⛔ NO second facade writer — this row
	//  adds ZERO calls to USiegeGraphicsSettingsSubsystem, zero quality-group
	//  value changes, zero detent changes and zero apply/save calls.
	//
	//  ════════════════════════════════════════════════════════════════════════
	//  ⛔ THE FOCUS STOPS THIS SCREEN OFFERS, IN WidgetTree TRAVERSAL ORDER
	//  (`UWidgetTree::ForEachWidget` = depth-first PRE-ORDER, slot order; and on
	//  this screen slot order == visual order BY CONSTRUCTION, because the whole
	//  column is a UVerticalBox filled in reading order inside a vertical
	//  UScrollBox). ⛔ ALL OF THEM ARE CONSTRUCTED IN C++ — there is NO
	//  `/Game/UI/WBP_GraphicsMenu` asset at all (GFX-§2 reserves the path and
	//  TASK-1461 measured it unauthored), so this screen cannot carry an
	//  asset-authored `IsFocusable=False` and cannot carry an EventGraph node
	//  that opts a control out. Every name below is route `C++`.
	//
	//     0. ShowFrameRateCounterCheckBox   UCheckBox   C++
	//     1. AutoDetectButton               UButton     C++
	//     2. OverallQualitySlider           USlider     C++
	//     3. ViewDistanceQualitySlider      USlider     C++
	//     4. AntiAliasingQualitySlider      USlider     C++
	//     5. ShadowQualitySlider            USlider     C++
	//     6. GlobalIlluminationQualitySlider USlider    C++
	//     7. ReflectionQualitySlider        USlider     C++
	//     8. PostProcessQualitySlider       USlider     C++
	//     9. TextureQualitySlider           USlider     C++
	//    10. EffectsQualitySlider           USlider     C++
	//    11. FoliageQualitySlider           USlider     C++
	//    12. ShadingQualitySlider           USlider     C++
	//    13. ResolutionScaleSlider          USlider     C++
	//    14. ScreenResolutionPrevButton     UButton     C++
	//    15. ScreenResolutionNextButton     UButton     C++
	//    16. WindowModePrevButton           UButton     C++
	//    17. WindowModeNextButton           UButton     C++
	//    18. KeepSettingsButton             UButton     C++   ⚠️ see below
	//    19. RevertSettingsButton           UButton     C++   ⚠️ see below
	//    20. VSyncCheckBox                  UCheckBox   C++
	//    21. FrameRateLimitPrevButton       UButton     C++
	//    22. FrameRateLimitNextButton       UButton     C++
	//    23. BackButton                     UButton     C++
	//
	//  ⇒ TWENTY-FOUR on the healthy path. ⛔ The ten quality groups appear in
	//  USiegeGraphicsSettingsSubsystem::GetQualityGroupNames() order, which is the
	//  order this panel builds them in — the list is never re-decided here.
	//
	//  ⚠️ THE TWO STATE-DEPENDENT ENTRIES (18, 19), DECLARED RATHER THAN HOPED
	//  PAST: KeepSettingsButton and RevertSettingsButton live inside
	//  VideoModeConfirmBorder, which is ESlateVisibility::Collapsed whenever no
	//  video-mode countdown is running (the 99 % case). ⛔ THE WALKER STILL
	//  ADMITS THEM, and that is MEASURED, not assumed: IsNavFocusStop() tests
	//  `Widget->IsVisible()`, and `UWidget::IsVisible()` returns
	//  `GetCachedWidget()->GetVisibility().IsVisible()` — the widget's OWN slate
	//  visibility (Widget.cpp), never its ancestors'. A Collapsed PARENT does not
	//  change a child's own EVisibility::Visible. ⇒ the ring stops twice on rows
	//  the player cannot see. ⛔ NOT FIXED HERE: the remedy is either the walker
	//  learning ancestor visibility (TASK-1406's file, fenced from this row) or
	//  mirroring the border's visibility onto the two buttons at the four
	//  documented WRITER sites — which would add two writers to the countdown's
	//  surface that spec (4) told this row to leave alone. ⛔ QA's call, not
	//  mine; the four-line patch is named in handoffs/TASK-1417-programmer.md.
	//
	//  ⚠️ THE STEPPER DOUBLE-STOP IS EXPECTED AND IS NOT THIS ROW'S DEFECT:
	//  each of the three stepper rows exposes BOTH its "<" and its ">" as
	//  separate UButtons, so the ring stops twice per stepper row (6 of the 24).
	//  qa/TASK-1410.md WARN-2 routes that to the walker.
	//
	//  ⚠️ THE UNHAPPY-PATH COUNT, so a low number is not mis-read as the defect:
	//  with no USiegeGraphicsSettingsSubsystem, ShowPanelUnavailable() calls
	//  SetAllControlsEnabled(false), which disables 12 of the names above
	//  (OverallQuality + ResolutionScale + VSync + AutoDetect + the six display
	//  stepper buttons + Keep + Revert) AND every group slider ⇒ 22 stops drop
	//  out and only ShowFrameRateCounterCheckBox (a different store) and
	//  BackButton (deliberately re-enabled) remain. ⛔ A count of 2 WITH the
	//  "[GraphicsMenu] USiegeGraphicsSettingsSubsystem could not be resolved"
	//  Warning in the same run is HEALTHY; a 2 WITHOUT it is the defect.
	//  ════════════════════════════════════════════════════════════════════════

	/**
	 *  Hand menu navigation to THIS screen. Called from NativeConstruct, AFTER
	 *  SeedAndBind() and ArmFrameRateReadout(), and paired with
	 *  UnregisterAsMenuNavTarget() on every exit.
	 *
	 *  ⛔ THE "AFTER" IS LOAD-BEARING, NOT STYLISTIC — the USettingsMenuWidget
	 *  hazard, only bigger here. RegisterMenuNavTarget() logs the stop count AND
	 *  places the ring on stop 0, and both read the tree's LIVE enabled state.
	 *  SeedAndBind() is what settles it: on the null-facade path it calls
	 *  ShowPanelUnavailable() -> SetAllControlsEnabled(false). Registering first
	 *  would log 24 for a screen that has 2 and could park the ring on a control
	 *  disabled a few lines later.
	 *
	 *  ⭐ Register also PLACES THE RING on stop 0, so this panel opens with
	 *  ShowFrameRateCounterCheckBox already outlined and the FIRST Down moves to
	 *  stop 1 (AutoDetectButton), not to stop 0. A reader expecting stop 0 after
	 *  one Down will mis-read a working screen as broken.
	 *
	 *  Null-safe: no world or no subsystem ⇒ one Log line and the panel behaves
	 *  exactly as it did before this row. Never fatal.
	 */
	void RegisterAsMenuNavTarget();

	/**
	 *  Give menu navigation back. Called from BOTH BackPressed() (BEFORE
	 *  RemoveFromParent, and BESIDE — never instead of —
	 *  DisarmVideoModeCountdown() + DiscardStagedVideoMode()) AND
	 *  NativeDestruct() (first statement, LIFO). The double call is deliberate
	 *  and safe: UnregisterMenuNavTarget removes by IDENTITY and logs-not-warns
	 *  a second call for a screen already gone.
	 *
	 *  ⭐ NESTING — THE HALF TASK-1415 DELIBERATELY LEFT FOR THIS ROW.
	 *  USettingsMenuWidget::GraphicsPressed() does NOT unregister itself when it
	 *  opens this panel, so the stack reads:
	 *      register(Settings)   -> [Settings]             ring on Settings
	 *      register(Graphics)   -> [Settings, Graphics]   ring on Graphics
	 *      unregister(Graphics) -> [Settings]             ring BACK on Settings
	 *  and TASK-1406's UnregisterMenuNavTarget re-places focus only when another
	 *  registered screen is taking over, which is exactly this case. ⇒ Back here
	 *  hands the ring to the SETTINGS panel underneath, ⛔ not to WBP_MainMenu
	 *  two layers down.
	 */
	void UnregisterAsMenuNavTarget();

	/**
	 *  Null-safe resolve of the menu-input subsystem. ⚠️ Unlike the two settings
	 *  stores this one lives on the WORLD, not the game instance, and it declines
	 *  Editor worlds outright (DoesSupportWorldType = Game | PIE), so a null
	 *  answer is ordinary rather than an error. ⛔ There is no automation
	 *  override: a bare NewObject widget has no world, and the suite has nothing
	 *  to assert here.
	 */
	USiegeMenuInputSubsystem* ResolveMenuInputSubsystem() const;

	/**
	 *  ⭐ 🧑 "scrollable with the outline" — THE WHOLE OF IT, IN TWO MECHANISMS
	 *  THAT AGREE, wired at construction and re-asserted in NativeConstruct.
	 *
	 *  ⛔ WHY IT IS NEEDED AT ALL, MEASURED: UScrollBox's constructor sets
	 *  `ScrollWhenFocusChanges(EScrollWhenFocusChanges::NoScroll)` (ScrollBox.cpp)
	 *  — the ENGINE DEFAULT IS TO NOT FOLLOW FOCUS. With 24 stops in a column
	 *  that does not fit 1080p, the ring would walk off the bottom and 🧑 he
	 *  would see it vanish. A ring the player cannot see is indistinguishable
	 *  from no ring.
	 */
	void ConfigureScrollFollowsFocus();

	/**
	 *  Asks RootScrollBox to bring FocusStop on screen. ⛔ Layout only — it
	 *  writes no setting, touches no timer and cannot re-enter the row echo.
	 *  Null-tolerant and headless-safe (UScrollBox::ScrollWidgetIntoView no-ops
	 *  when its Slate widget is invalid, which is every automation run).
	 */
	void ScrollFocusStopIntoView(UWidget* FocusStop);

	/**
	 *  The UWidget in THIS widget's tree whose cached Slate widget is exactly
	 *  SlateWidget, or null. Used to turn the focus path's leaf back into
	 *  something UScrollBox::ScrollWidgetIntoView can take.
	 */
	UWidget* FindOwnWidgetForSlateWidget(const TSharedRef<SWidget>& SlateWidget) const;
};

/**
 *  ⭐⭐⭐ TASK-1120 [GFX-FPS] — THE IN-MATCH COUNTER, AND IT IS THE LOAD-BEARING
 *  HALF OF `GFX-§7`, NOT AN EXTRA.
 *
 *  ═══ WHY THIS CLASS EXISTS AT ALL ════════════════════════════════════════
 *  🧑 Jonathan asked for a graphics menu so "the player decides what they want to
 *  adjust to optimize for performance versus visual quality". A readout that
 *  only ever appears in `L_MainMenu` would answer that ask with a number
 *  measured against ⛔ no ~340 scatter trees, ⛔ no ~24k grass instances, ⛔ no
 *  volumetric fog and ⛔ no Lumen-lit battlefield. It would be flattering, it
 *  would be stable, and every decision made against it would be wrong. ⇒ ⛔ THE
 *  COST LIVES IN `L_Arena`, SO THE INSTRUMENT GOES THERE.
 *
 *  ═══ WHERE IT DRAWS ══════════════════════════════════════════════════════
 *  A viewport widget created by `ASiegePlayerController::TryInitFrameRateCounter()`
 *  right after the HUD, at ZOrder 30 — above the HUD (0), above the victory
 *  screen (10) and above the war map (25), because a counter the victory screen
 *  covers is a counter that vanishes exactly when a player is comparing two
 *  matches. Top-right of the screen, clear of the card bar (bottom) and the
 *  castle health bars (top-centre).
 *
 *  ⛔ IT IS `HitTestInvisible`, ROOT AND CHILDREN. Its root Border fills the
 *  viewport in order to align its content to the top-right corner, so if it were
 *  hit-testable it would swallow ⛔ EVERY click in the match — placement, orders,
 *  the card bar, all of it. `HitTestInvisible` means "neither I nor my children
 *  are hit-testable", which is exactly the contract this needs. (`GFX-§2(f)`
 *  makes the OPPOSITE choice for the menu backdrop, on purpose: that one exists
 *  to absorb clicks. Same enum, opposite requirement — do not copy one to the
 *  other.)
 *
 *  ═══ WHAT IT COSTS ═══════════════════════════════════════════════════════
 *  ⛔ Nothing per frame, in either state, and that is structural rather than
 *  claimed:
 *   • `meta=(DisableNativeTick)` + no Blueprint tick + no animations ⇒ the widget
 *     never ticks (`UserWidget.h:117-128`: `EWidgetTickFrequency::Auto` already
 *     means "only if a BP tick, a latent action or an animation needs it"; the
 *     meta closes the native half too).
 *   • ON: one 0.5 s timer whose body is two global reads, a division and a
 *     SetText — see `FSiegeFrameRateSample`.
 *   • OFF (the default): `ESlateVisibility::Collapsed` and ⛔ NO TIMER AT ALL.
 *     Slate skips a collapsed widget in both layout and paint, so the cost of
 *     the counter a player has not asked for is the memory for one widget.
 *
 *  ═══ THE PREFERENCE ══════════════════════════════════════════════════════
 *  It SEEDS from `USiegeSettingsSubsystem::IsFrameRateCounterEnabled()` and THEN
 *  binds `OnSettingsChanged` (the qa/TASK-005 major-2 order: a bind-only widget
 *  shows whatever it was constructed with until something happens to change it).
 *  ⛔ The fallback with no resolvable settings store is `false` — a lookup
 *  failure must never put a diagnostic overlay on a shipped player's
 *  battlefield.
 *
 *  ⚠️ DECLARED, NOT IMPLIED: nothing in this file has been compiled, run or
 *  rendered. See "NO WITNESSED RED" in handoffs/TASK-1120-programmer.md.
 */
UCLASS(meta = (DisableNativeTick))
class GITCLAUDEUNREALTEST_API USiegeFrameRateCounterWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/**
	 *  Creates the counter and adds it to the viewport, ALREADY OBEYING the
	 *  preference — so with the pref off this call puts nothing on screen and
	 *  starts no timer.
	 *
	 *  ⛔ THREE PARAMETERS, NONE DEFAULTED (`SC-§33`). A null CounterClass falls
	 *  back to this C++ class, which is the shipping state: there is no
	 *  WidgetBlueprint for this and none is reserved.
	 *
	 *  @param OwningController the local player controller. Null ⇒ no counter,
	 *                          logged, ⛔ never fatal.
	 *  @param CounterClass     null ⇒ this class.
	 *  @param ZOrder           viewport Z order; the controller passes 30.
	 */
	static USiegeFrameRateCounterWidget* CreateAndAddToViewport(
		APlayerController* OwningController,
		TSubclassOf<USiegeFrameRateCounterWidget> CounterClass,
		int32 ZOrder);

	/** The readout period AND the averaging window — the same constant, for the same reason, as the panel's. */
	static constexpr float CounterIntervalSeconds = USiegeGraphicsMenuWidget::FrameRateReadoutIntervalSeconds;

	/**
	 *  Re-reads the preference, then shows-and-arms or collapses-and-disarms.
	 *
	 *  ⛔ THE ONE WRITER OF THIS WIDGET'S VISIBILITY AND THE ONE ARM/DISARM SITE.
	 *  Two places deciding "is the counter up?" is how a counter ends up visible
	 *  with a dead timer (a frozen number, which reads as a frozen GAME) or
	 *  collapsed with a live one (invisible cost). One function, both halves.
	 */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void ApplyFrameRateCounterPreference();

	/**
	 *  ⛔ THE DECISION, EXTRACTED AS A PURE STATIC SO IT IS ASSERTABLE WITHOUT A
	 *  WORLD — and so the fail-safe polarity is pinned in one place instead of
	 *  being re-derived at each call site.
	 *
	 *  `nullptr` ⇒ **false**. An unresolvable settings store must never put a
	 *  diagnostic overlay on a shipped player's battlefield; note this is the
	 *  OPPOSITE polarity to USiegeSettingsSubsystem::IsAssistantConfirmEnabled()'s
	 *  documented `true` fallback, and for the mirror-image reason.
	 */
	static bool ShouldShowFrameRateCounter(const USiegeSettingsSubsystem* Settings);

	/** The timer callback. Public + BlueprintCallable only so the suite can drive it; nothing in the game calls it except the timer. */
	UFUNCTION(BlueprintCallable, Category = "Siegebound|Graphics")
	void RefreshCounterText();

	/** Builds the code-authored tree. Public for the same reason USiegeGraphicsMenuWidget::ConstructGraphicsTree() is: the suite builds a scratch counter without a Slate widget. */
	void ConstructCounterTree();

	/** ⛔ AUTOMATION TESTS ONLY. Points this counter at a scratch settings store instead of the world's game instance, which a bare NewObject widget does not have. */
	void SetSettingsSubsystemForAutomationTests(USiegeSettingsSubsystem* InSettings);

	/**
	 *  USiegeSettingsSubsystem::OnSettingsChanged handler. ⛔ FILTERS on
	 *  SettingName_ShowFrameRateCounter: that store also carries the assistant
	 *  confirm toggle, and a confirm-toggle change must not re-arm a frame-rate
	 *  timer. ⭐ The filter is what makes the payload NAME load-bearing rather than
	 *  decorative, and a test asserts it by NAME rather than by broadcast count —
	 *  a count cannot tell a swapped payload from a correct one (qa/TASK-1119.md
	 *  § LOOP 1, the M21 coincidence).
	 *
	 *  Public because the suite drives it directly: a bare NewObject widget cannot
	 *  receive a real broadcast (there is no bound multicast without a live
	 *  subsystem), so the delegate edge itself is a declared unproven premise and
	 *  the HANDLER is what gets asserted.
	 */
	UFUNCTION()
	void HandleSettingsChanged(FName SettingName);

	/**
	 *  ⛔ DIAGNOSTICS + THE SUITE'S OBSERVATION POINT, and it is deliberately a
	 *  STATE and not a tally: is the readout timer armed right now?
	 *
	 *  ⚠️ THIS LANE HAS ALREADY PAID FOR THE DIFFERENCE. `qa/TASK-1119.md` § LOOP 1
	 *  found a broadcast COUNTER that read identically under the fixed and the
	 *  broken branch by arithmetic coincidence, and `qa/TASK-1114.md` found a
	 *  facade counter that read `0` on broken and fixed alike. A count of
	 *  arm-calls could not tell "armed once and left armed" from "armed and
	 *  disarmed"; ⛔ this bool can.
	 */
	UFUNCTION(BlueprintPure, Category = "Siegebound|Graphics")
	bool IsCounterTimerArmed() const { return bCounterTimerArmed; }

	/** The FName carried by the pinned readout in this tree — the SAME token as the panel's, on purpose. Two different trees, one word for one thing. */
	static const TCHAR* FrameRateReadoutWidgetName;

protected:

	//~ Begin UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End UUserWidget interface

	/** Fills the viewport and aligns its content top-right. ⛔ HitTestInvisible — see the class comment. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameRateCounterRoot;

	/** The dark plate behind the number, so it stays legible over snow, grass and fire alike. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameRateCounterBorder;

	/** ⛔ `GFX-§10`'s pinned name, in this tree too. */
	UPROPERTY(BlueprintReadOnly, Category = "Siegebound|Graphics", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRateReadoutText;

private:

	/** Null-safe: the automation override first, then this widget's world's game instance. Null is ordinary (a menu-level counter would never be created) and never fatal. */
	USiegeSettingsSubsystem* ResolveSettingsSubsystem() const;

	/** ⛔ Automation only. */
	UPROPERTY(Transient)
	TObjectPtr<USiegeSettingsSubsystem> SettingsOverrideForAutomationTests = nullptr;

	FTimerHandle CounterTimerHandle;

	/** ⛔ The observable STATE behind IsCounterTimerArmed(). Written in exactly one function. */
	bool bCounterTimerArmed = false;

	FSiegeFrameRateSample FrameRateSample;
};
