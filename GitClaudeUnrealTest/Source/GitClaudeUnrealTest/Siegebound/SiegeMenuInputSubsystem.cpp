// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeMenuInputSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
// TASK-1406 fence (c): the three classes the walker's vocabulary gained beyond UButton. Each is
// included for a real Cast/IsA below, not defensively.
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
// TASK-1409: the stepper-pair walk reads the focused button's PARENT panel directly
// (`UWidget::GetParent()` → `UPanelWidget::GetChildAt`). The type was already complete here
// transitively (`GetButtonLabel` casts to it); this include says so on purpose.
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/Package.h"
// TASK-1469 limb 2: `HasVisibleSlateAncestry` walks the SLATE parent chain
// (`SWidget::GetParentWidget()` → `SWidget::GetVisibility()` → `EVisibility::IsVisible()`). The
// type was already complete here transitively through `Framework/Application/SlateApplication.h`;
// this include says so on purpose, the same way the `Components/PanelWidget.h` line above does.
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY(LogSiegeMenuInput);

// ─── The names (TASK-1274 deliverable (1); CONVENTIONS naming — `/Game/Input/IMC_*`, `/Game/Input/Actions/IA_*`) ───
const TCHAR* USiegeMenuInputSubsystem::MenuMapName             = TEXT("L_MainMenu");
const TCHAR* USiegeMenuInputSubsystem::MenuMappingContextPath  = TEXT("/Game/Input/IMC_MainMenu.IMC_MainMenu");
const TCHAR* USiegeMenuInputSubsystem::MenuUpActionPath        = TEXT("/Game/Input/Actions/IA_MenuUp.IA_MenuUp");
const TCHAR* USiegeMenuInputSubsystem::MenuDownActionPath      = TEXT("/Game/Input/Actions/IA_MenuDown.IA_MenuDown");
const TCHAR* USiegeMenuInputSubsystem::MenuAcceptActionPath    = TEXT("/Game/Input/Actions/IA_MenuAccept.IA_MenuAccept");
const TCHAR* USiegeMenuInputSubsystem::MainMenuWidgetClassPath = TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C");

// ─── TASK-1409 (1): the three actions TASK-1408 authored. Object paths quoted from that row's
// handoff §(1) ("⛔ These are the literals TASK-1409 references. Character-for-character"). ───
const TCHAR* USiegeMenuInputSubsystem::MenuLeftActionPath  = TEXT("/Game/Input/Actions/IA_MenuLeft.IA_MenuLeft");
const TCHAR* USiegeMenuInputSubsystem::MenuRightActionPath = TEXT("/Game/Input/Actions/IA_MenuRight.IA_MenuRight");
const TCHAR* USiegeMenuInputSubsystem::MenuBackActionPath  = TEXT("/Game/Input/Actions/IA_MenuBack.IA_MenuBack");

namespace SiegeMenuStepper
{
	// ─── TASK-1409 (2): HOW A STEPPER PAIR IS RECOGNISED ───────────────────────────────────────
	// ⛔ NOT INVENTED — lifted from the only code in this project that builds one,
	// `USiegeGraphicsMenuWidget::BuildStepperRow` / `BuildStepButton`, which constructs the two
	// buttons as `FName(*(Base + TEXT("PrevButton")))` and `FName(*(Base + TEXT("NextButton")))`
	// and gives them the single-`UTextBlock` contents `TEXT("<")` and `TEXT(">")`.
	//
	// ⚠️ BOTH DISCRIMINATORS EXIST BECAUSE EITHER CAN LEGITIMATELY GO ABSENT: an asset-authored
	// row wins whole and is never rebuilt (that row's "condition (b)"), so a future
	// `WBP_GraphicsMenu` could keep the pinned NAMES while an artist restyles the glyphs — or
	// keep the glyphs under different names. Names are tried first because they are the pinned,
	// greppable contract; glyphs are the fallback.
	static const TCHAR* PrevNameSuffix = TEXT("PrevButton");
	static const TCHAR* NextNameSuffix = TEXT("NextButton");
	static const TCHAR* PrevGlyph      = TEXT("<");
	static const TCHAR* NextGlyph      = TEXT(">");
}

namespace SiegeMenuCheckBox
{
	/**
	 *  The word a log line uses for a check-box state. `Undetermined` is spelled out rather than
	 *  folded into "off": a tri-state box reading "off" in the log while `GetCheckedState()` says
	 *  otherwise is exactly the kind of almost-true line this epic keeps paying for.
	 */
	static const TCHAR* StateWord(ECheckBoxState State)
	{
		switch (State)
		{
		case ECheckBoxState::Checked:   return TEXT("on");
		case ECheckBoxState::Unchecked: return TEXT("off");
		default:                        return TEXT("undetermined");
		}
	}
}

bool USiegeMenuInputSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Game and PIE only. The engine default also admits EWorldType::Editor; an Editor world
	// must never arm input bindings or place Slate focus (and never touches L_MainMenu's
	// dirty state — acceptance (5)).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void USiegeMenuInputSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// ⛔ L_MainMenu ONLY. The world's package is "UEDPIE_0_L_MainMenu" in PIE and
	// "L_MainMenu" in a game; strip the PIE prefix, then compare the short asset name.
	//
	// ═══ 🚨 TASK-1429 (1) — THIS IS "THE BRANCH", AND IT IS NAMED RATHER THAN MOVED ═══════════
	// The row's ask was that the map test stop being a MAP test and become a DEMAND test. It does
	// — but NOT by editing these four lines. ⛔ THE STATEMENT BELOW IS UNTOUCHED, AND THAT IS THE
	// DELIVERABLE: on any map but `L_MainMenu` this function still returns HERE, before one
	// `LoadObject`, one `BindAction`, one `AddMappingContext` or one `SetTimer`. A match in which
	// no screen ever registers is byte-for-byte the match that shipped — there is no "harmless"
	// context applied at match start, because there is no code between this line and the return.
	//
	// ⭐ THE DEMAND TEST IS A SECOND, LATER DOOR, NOT AN EDIT TO THIS ONE: `RegisterMenuNavTarget`
	// → `ReconcileInMatchArming` → `ArmInMatchMenuVocabulary`. It can only be reached by a screen
	// that explicitly asked for menu navigation, and `DisarmInMatchMenuVocabulary` REMOVES the
	// context the moment the last such screen goes. ⛔ Nothing below this return is reachable
	// off-menu; everything TASK-1429 added lives in those three functions and in `Deinitialize`.
	// ⛔ The branch this `return` takes is therefore the DEFAULT-STATE branch, and it is empty.
	const FString MapName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(InWorld.GetOutermost()->GetName()));
	if (MapName != MenuMapName)
	{
		return;
	}

	// The local controller exists here: SpawnPlayActor runs before World->BeginPlay on both
	// the PIE (GameInstance.cpp:538 vs :566) and the LoadMap (UnrealEngine.cpp:16647 vs
	// :16661) paths. Measured class on this map: /Script/Engine.PlayerController.
	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: no local player controller at OnWorldBeginPlay — menu input actions NOT armed."),
			MenuMapName);
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!InputSubsystem || !InputComponent)
	{
		// DefaultInput.ini:81-82 pin EnhancedPlayerInput / EnhancedInputComponent, so a null
		// here means the config moved — say so instead of failing silently.
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: Enhanced Input unavailable on the local controller (subsystem %s, component %s) — menu input actions NOT armed."),
			MenuMapName,
			InputSubsystem ? TEXT("ok") : TEXT("null"),
			InputComponent ? TEXT("ok") : TEXT("null / not UEnhancedInputComponent"));
		return;
	}

	// Load the four authored assets. LoadObject (not a UPROPERTY slot on a Blueprint) because
	// no project class exists on this map to carry a slot — see the header. A missing asset
	// is logged by name (the Artist/asset side of the row) and the feature stays off.
	MenuMappingContext = LoadObject<UInputMappingContext>(nullptr, MenuMappingContextPath);
	MenuUpAction       = LoadObject<UInputAction>(nullptr, MenuUpActionPath);
	MenuDownAction     = LoadObject<UInputAction>(nullptr, MenuDownActionPath);
	MenuAcceptAction   = LoadObject<UInputAction>(nullptr, MenuAcceptActionPath);
	if (!MenuMappingContext || !MenuUpAction || !MenuDownAction || !MenuAcceptAction)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: input assets missing (IMC_MainMenu %s, IA_MenuUp %s, IA_MenuDown %s, IA_MenuAccept %s) — menu input actions NOT armed."),
			MenuMapName,
			MenuMappingContext ? TEXT("ok") : TEXT("MISSING"),
			MenuUpAction       ? TEXT("ok") : TEXT("MISSING"),
			MenuDownAction     ? TEXT("ok") : TEXT("MISSING"),
			MenuAcceptAction   ? TEXT("ok") : TEXT("MISSING"));
		return;
	}

	// ─── TASK-1409 (1): THE THREE NEW ACTIONS, LOADED **OUTSIDE** THE FATAL BLOCK ──────────────
	// ⛔ DEGRADE OPEN, and the placement is the whole of the mechanism: these three loads sit
	// BELOW the `return` above, so a missing IA_MenuLeft can never take IA_MenuUp / IA_MenuDown /
	// IA_MenuAccept down with it. Each key is bound only if ITS asset resolved; the others arm
	// regardless, and the absence is named once rather than inferred from a dead key.
	MenuLeftAction  = LoadObject<UInputAction>(nullptr, MenuLeftActionPath);
	MenuRightAction = LoadObject<UInputAction>(nullptr, MenuRightActionPath);
	MenuBackAction  = LoadObject<UInputAction>(nullptr, MenuBackActionPath);
	if (!MenuLeftAction || !MenuRightAction || !MenuBackAction)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] %s: navigation-extension input assets missing (IA_MenuLeft %s, IA_MenuRight %s, IA_MenuBack %s) — THOSE keys are inert; IA_MenuUp / IA_MenuDown / IA_MenuAccept stay armed."),
			MenuMapName,
			MenuLeftAction  ? TEXT("ok") : TEXT("MISSING"),
			MenuRightAction ? TEXT("ok") : TEXT("MISSING"),
			MenuBackAction  ? TEXT("ok") : TEXT("MISSING"));
	}

	// ⛔ Applied VERBATIM — no USiegeKeyboardLayoutSubsystem::GetPositionalContext call.
	// IMC_MainMenu carries no letter (KBD-§4), so there is nothing to retarget, and routing a
	// letterless context through the remap would only add a duplicate to reason about.
	InputSubsystem->AddMappingContext(MenuMappingContext, MenuMappingContextPriority);

	// ETriggerEvent::Started — the hero's idiom for one-shot actions (HeroCharacter.cpp:402,
	// :412, :435). A Boolean action with no explicit trigger fires Started ONCE on the
	// actuation edge, so a sustained injection (Aura's hold_seconds) or a held key is ONE
	// menu step, never a per-frame scroll.
	InputComponent->BindAction(MenuUpAction,     ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuUp);
	InputComponent->BindAction(MenuDownAction,   ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuDown);
	InputComponent->BindAction(MenuAcceptAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuAccept);
	bArmed = true;

	// TASK-1409 (1): the same idiom and the same `Started` event, one `if` each. ⛔ `bArmed` is
	// set ABOVE these three on purpose — it means "the shipped three work", and a missing new
	// asset must not be able to clear it (see the header's `IsArmed()`).
	if (MenuLeftAction)
	{
		InputComponent->BindAction(MenuLeftAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuLeft);
	}
	if (MenuRightAction)
	{
		InputComponent->BindAction(MenuRightAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuRight);
	}
	if (MenuBackAction)
	{
		InputComponent->BindAction(MenuBackAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuBack);
	}

	// The menu widget does not exist yet (OnWorldBeginPlay precedes GameMode->StartPlay,
	// World.cpp UWorld::BeginPlay); the first button is focused on the next tick.
	InWorld.GetTimerManager().SetTimerForNextTick(this, &USiegeMenuInputSubsystem::ApplyInitialFocus);

	// ─── TASK-1400 (MENU-REENTRY-FOCUS): THE RE-ENTRY RE-ARM ────────────────────────────────
	// 🧑 His words: "if you exit the deck builder and go back to the main menu, the outline is no
	// longer there ... exiting to the main menu results in that outline appearing at the top
	// option, not just when you start up the game."
	//
	// ⛔ The obvious fix -- re-fire against the menu we focused at boot -- WOULD NOT WORK, and that
	// is measured, not feared. BOTH return paths REPLACE the widget rather than re-showing it:
	//   • deck builder Exit  -- BP `CreateWidget(WBP_MainMenu_C) -> Is Valid ->
	//     AddToViewport(ZOrder 0) -> RemoveFromParent(self)`, no focus node (TASK-1399 §5.3);
	//   • `USessionMenuWidget::BackPressed` (its standalone branch: the `LoadClass<UUserWidget>` of
	//     `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget` → `AddToViewport` swap; cited as
	//     `SessionMenuWidget.cpp:151-165` until TASK-1480 (d)) -- the same shape.
	// ⇒ a re-arm that re-fires against a remembered pointer passes every static read and works
	// NOWHERE he actually goes.
	//
	// ⭐ WHY THIS SURVIVES A FRESH WIDGET: it holds no widget at all. `ApplyInitialFocus()` takes no
	// argument and caches nothing; `IsMenuUncovered()`, `GetMenuButtons()` and
	// `GetFocusedMenuButton()` each re-resolve through `GetAllWidgetsOfClass(..., TopLevelOnly)`
	// matched on the CLASS PATH string and filtered by `IsInViewport()`. A `RemoveFromParent`'d
	// instance drops out of that set; a freshly `AddToViewport`'d one appears in it. There is no
	// pointer to go stale -- the trigger is pointer-free BY CONSTRUCTION.
	//
	// ⛔ AND IT CANNOT STEAL FOCUS: `ApplyInitialFocus`'s own `!GetFocusedMenuButton()` guard is
	// LEFT EXACTLY AS IT WAS, so a repeat is a no-op whenever a menu button already holds focus,
	// and `IsMenuUncovered()` is false for the whole time any sub-screen is up. The only state this
	// poll ever acts in is "menu visible, uncovered, nothing on it focused" -- precisely 🧑 his gap.
	// ⚠️ Keeping that guard is also what keeps us out of TASK-1446's trap: `SetUserFocus`
	// early-returns false when the target is ALREADY focused (`SlateApplication.cpp:3028-3033`).
	//
	// ⛔ Armed HERE -- after the `L_MainMenu` map gate and the arming returns above -- so the timer
	// does not exist on any other map.
	InWorld.GetTimerManager().SetTimer(
		FocusReentryPollTimerHandle, this, &USiegeMenuInputSubsystem::ApplyInitialFocus,
		FocusReentryPollSeconds, /*bLoop=*/ true);

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s: IMC_MainMenu applied at priority %d on '%s'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)."),
		MenuMapName, MenuMappingContextPriority, *PC->GetName());

	// ⛔ A SECOND LINE, NOT AN EDIT TO THE ONE ABOVE. That string is quoted as a discriminator in
	// `qa/TASK-1274-verify.md`, `qa/TASK-1393-verify.md`, `qa/TASK-1395-verify.md`,
	// `qa/TASK-1399-verify.md` and four more pipeline files (measured by grep); widening it would
	// break every one of those readers for a cosmetic gain. The new bindings report themselves.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s: IA_MenuLeft / IA_MenuRight / IA_MenuBack bound (Started): %s / %s / %s."),
		MenuMapName,
		MenuLeftAction  ? TEXT("bound") : TEXT("ABSENT — key inert"),
		MenuRightAction ? TEXT("bound") : TEXT("ABSENT — key inert"),
		MenuBackAction  ? TEXT("bound") : TEXT("ABSENT — key inert"));
}

void USiegeMenuInputSubsystem::Deinitialize()
{
	// The world is going away with its controller, input component and applied contexts;
	// nothing to unbind — the bindings live on the controller's component, the context on
	// the local player's subsystem, both torn down by their owners.
	//
	// ⛔ TASK-1400: the re-entry poll is the ONE thing here that does NOT die with the world.
	// `UWorld::GetTimerManager()` resolves to the OWNING GAME INSTANCE's manager, which survives
	// the travel off `L_MainMenu`, so the looping timer is cleared explicitly. (The delegate is
	// weak-bound to `this`, so a leaked loop could never call into a dead subsystem -- but it would
	// still sit on the game instance's manager, and that is worth one line to avoid.)
	// ⚠️ Safe here: `DoesSupportWorldType` admits Game | PIE only, so every world that reaches this
	// point has an owning game instance; and `ClearTimer` on an unset handle is a no-op, which is
	// the ordinary case (every map but the menu never armed it).
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FocusReentryPollTimerHandle);
	}

	// ⛔ TASK-1429: AND THE IN-MATCH CONTEXT COMES OFF HERE, UNCONDITIONALLY AND FIRST.
	// A no-op on every world that never armed it (including `L_MainMenu`, which arms through the
	// other door and is reported by `bArmed`), so the shipped teardown is unchanged in the
	// ordinary case. ⚠️ It is not redundant with the falling edge: a world can be torn down with a
	// screen still registered (level travel out of a match with an overlay up, a `Play Again`
	// pressed on the end screen), and "the controller dies and takes its `UEnhancedPlayerInput`
	// with it" is a claim about engine teardown order that this row would rather not rest on when
	// one call states the intent. ⛔ It also retires `InMatchDemandPollTimerHandle`, which lives on
	// the GAME INSTANCE's timer manager and therefore outlives this world exactly as the
	// re-entry poll above does.
	DisarmInMatchMenuVocabulary(TEXT("Deinitialize"));
	BoundInMatchInputComponent.Reset();

	// TASK-1406: drop any screen still registered as the nav target. ⛔ NOT load-bearing -- the
	// stack is weak and this subsystem dies with the world -- but it is the same one-line hygiene
	// the four asset pointers below get, and it keeps a reader from having to derive that.
	NavTargetStack.Empty();

	// TASK-1471: the same one-line hygiene, for the same reason and with the same disclaimer — ⛔ NOT
	// load-bearing (the marks are weak and this subsystem dies with the world), but the two
	// collections are a pair and a reader should not have to derive that one of them is cleared here
	// and the other is not.
	SelfDrivingScreens.Empty();

	bArmed = false;
	MenuMappingContext = nullptr;
	MenuUpAction = nullptr;
	MenuDownAction = nullptr;
	MenuAcceptAction = nullptr;
	// TASK-1409: the same one-line hygiene as the four above; any of the three may already be
	// null (degrade open), and clearing a null pointer is the ordinary case, not an error.
	MenuLeftAction = nullptr;
	MenuRightAction = nullptr;
	MenuBackAction = nullptr;
	Super::Deinitialize();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  READ API
// ═════════════════════════════════════════════════════════════════════════════════════════

APlayerController* USiegeMenuInputSubsystem::GetLocalController() const
{
	const UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return (PC && PC->IsLocalPlayerController()) ? PC : nullptr;
}

UUserWidget* USiegeMenuInputSubsystem::FindMainMenuWidget() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Top-level only (IsInViewport): the SessionMenu's standalone Back re-creates
	// WBP_MainMenu (`USessionMenuWidget::BackPressed`'s `LoadClass<UUserWidget>` of
	// `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget`; cited as
	// `SessionMenuWidget.cpp:151-164` until TASK-1480 (d)), so the instance is resolved LIVE on
	// every call and never cached across inputs.
	TArray<UUserWidget*> TopLevel;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevel, UUserWidget::StaticClass(), /*TopLevelOnly*/ true);
	for (UUserWidget* Widget : TopLevel)
	{
		if (Widget && Widget->GetClass() && Widget->GetClass()->GetPathName() == MainMenuWidgetClassPath)
		{
			return Widget;
		}
	}
	return nullptr;
}

bool USiegeMenuInputSubsystem::IsMenuUncovered() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	bool bMenuPresent = false;
	TArray<UUserWidget*> TopLevel;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, TopLevel, UUserWidget::StaticClass(), /*TopLevelOnly*/ true);
	for (const UUserWidget* Widget : TopLevel)
	{
		if (!Widget || !Widget->IsVisible())
		{
			continue;
		}
		if (Widget->GetClass() && Widget->GetClass()->GetPathName() == MainMenuWidgetClassPath)
		{
			bMenuPresent = true;
		}
		else
		{
			// Any other visible top-level widget (deck builder, settings, login, session
			// panel) sits ON TOP of the menu — the menu is covered; the handlers stay inert.
			return false;
		}
	}
	return bMenuPresent;
}

void USiegeMenuInputSubsystem::GetMenuButtons(TArray<UButton*>& OutButtons) const
{
	OutButtons.Reset();
	const UUserWidget* Menu = FindMainMenuWidget();
	if (!Menu || !Menu->WidgetTree)
	{
		return;
	}

	// ForEachWidget walks panel children in slot order, so buttons the Blueprint's Construct
	// added to VerticalBox_0 at runtime are visited top-to-bottom (Button_0..6).
	Menu->WidgetTree->ForEachWidget([&OutButtons](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		if (Button && Button->GetIsFocusable() && Button->GetIsEnabled() && Button->IsVisible())
		{
			OutButtons.Add(Button);
		}
	});
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  TASK-1406 — THE ACTIVE NAV TARGET (fence (b)) AND THE FOCUS-STOP VOCABULARY (fence (c))
// ═════════════════════════════════════════════════════════════════════════════════════════

UUserWidget* USiegeMenuInputSubsystem::GetRegisteredNavTarget() const
{
	// Top-down: the most recently registered LIVE screen wins, so Settings → Graphics → Back
	// hands navigation back to Settings rather than to the main menu underneath both.
	//
	// ⚠️ EVERY ENTRY IS RE-VALIDATED ON EVERY READ, not trusted: weak pointers go null when the
	// screen is collected, but a screen can also be `RemoveFromParent`'d (or hidden) WITHOUT being
	// collected yet and without its close path having called Unregister. Skipping those here is
	// what makes a forgotten Unregister degrade to "the ring goes back to the menu" instead of
	// "the ring is stuck on an invisible tree and nothing errors".
	for (int32 Index = NavTargetStack.Num() - 1; Index >= 0; --Index)
	{
		UUserWidget* Screen = NavTargetStack[Index].Get();
		if (Screen && Screen->IsInViewport() && Screen->IsVisible())
		{
			return Screen;
		}
	}
	return nullptr;
}

UUserWidget* USiegeMenuInputSubsystem::GetActiveNavTarget() const
{
	// ⭐ HALF OF THE (2) REGRESSION CONTRACT, AND IT IS THIS ONE `if`: with NOTHING registered
	// `GetRegisteredNavTarget()` is null and this function IS `FindMainMenuWidget()` — the same
	// call, the same body, reached on the same path as before this row.
	if (UUserWidget* Registered = GetRegisteredNavTarget())
	{
		return Registered;
	}
	return FindMainMenuWidget();
}

bool USiegeMenuInputSubsystem::IsNavTargetActionable() const
{
	// ⭐ THE OTHER HALF OF THE (2) REGRESSION CONTRACT. `IsMenuUncovered()` is not edited by this
	// row at all; with nothing registered this predicate IS that predicate.
	if (GetRegisteredNavTarget() != nullptr)
	{
		// ⛔ FENCE (b), in one sentence: an explicit registration ANSWERS the coverage question.
		// "Is something on top of the main menu?" was only ever a proxy for "is the player still
		// looking at the main menu?", and a screen that registered itself has said it is not.
		// ⚠️ This is also why TASK-1425's HUD / FPS-counter case needs no special case: those are
		// always-visible top-level widgets (ZOrder 0 and 30) that would trip the OLD gate, and
		// they simply never register.
		return true;
	}
	return IsMenuUncovered();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  ⭐⭐ TASK-1471 — THE SELF-DRIVING SCREEN FLAG. Read the class comment's section of that name
//  before editing either function: together they are the whole of the policy, and the thing they
//  are answerable for is a measured one — the ring transiting the deck builder's "Reset to
//  Default" button, which wipes 🧑 his deck and auto-saves the wipe, on EVERY press.
// ═════════════════════════════════════════════════════════════════════════════════════════

UUserWidget* USiegeMenuInputSubsystem::GetSelfDrivingActiveTarget() const
{
	// ⛔ `GetRegisteredNavTarget()`, NEVER `GetActiveNavTarget()`. The latter falls back to
	// `WBP_MainMenu`, so reading it here would let a declaration silence the ring on the DEFAULT
	// target — a screen that declared, then closed, could mute the main menu. `GetRegisteredNavTarget`
	// re-validates live / in-viewport / visible on every read, so a declaration is honoured for
	// exactly as long as its screen actually owns navigation and not one press longer.
	UUserWidget* Registered = GetRegisteredNavTarget();
	if (!Registered)
	{
		// ⭐ THE DEFAULT AND THE REGRESSION BRANCH IN ONE LINE: nothing registered ⇒ nothing declared
		// ⇒ every gate below falls straight through to the code TASK-1406 / TASK-1409 shipped.
		return nullptr;
	}

	// ⚠️ Compared by POINTER against a screen already proven live. A stale entry's `Get()` is null and
	// `Registered` never is, so a dead mark cannot match; and a weak pointer carries a serial number,
	// so a recycled object index cannot match either.
	const bool bDeclared = SelfDrivingScreens.ContainsByPredicate([Registered](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return Entry.Get() == Registered;
	});

	return bDeclared ? Registered : nullptr;
}

bool USiegeMenuInputSubsystem::DeclineIfActiveTargetSelfDriving(const FString& Site) const
{
	UUserWidget* SelfDriver = GetSelfDrivingActiveTarget();
	if (!SelfDriver)
	{
		return false;
	}

	// ⛔ ONE LINE, NEVER A SILENT RETURN — the one outcome this epic forbids. It has to separate
	// three states a reader would otherwise have to guess between: "the action never arrived" (no
	// line at all), "it arrived and the generic ring declined because this screen drives itself"
	// (this line), and "it arrived and the ring moved" (the `focus moved` line). ⭐ It also names
	// what did NOT happen to the screen, because the reviewable claim of this row is that the
	// screen is still registered and still counted while the ring no longer walks it.
	// ⚠️ SHIPPING: `Log` verbosity is compiled out entirely under Shipping (`USE_LOGGING_IN_SHIPPING`
	// = 0 ⇒ `NO_LOGGING` = 1; no Target.cs override in this project). In a packaged build this line
	// does not exist — the same property every other `Log` line in this file carries, and a
	// precondition on the READER rather than a reason to raise the verbosity. ⛔ THE REFUSAL ITSELF
	// IS NOT A LOG SIDE EFFECT: it is the `return true` below, which exists in every configuration.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s declined: the active nav target '%s' (%s) drives its OWN navigation (TASK-1471) — the generic ring does not walk its tree, press its buttons or step its controls. It stays registered and its focus stops are still counted."),
		*Site, *SelfDriver->GetName(), *SelfDriver->GetClass()->GetName());

	return true;
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  🚨 TASK-1429 — THE IN-MATCH ARM. THE MAP TEST BECOMES A DEMAND TEST.
//
//  ⛔ READ THE CLASS COMMENT'S "THE IN-MATCH ARM" SECTION BEFORE EDITING ANYTHING BELOW. This is
//  the only code in the project that can decide whether the arrow keys belong to a menu or to a
//  live match, and a match where they belong to a menu is a REGRESSION, not a feature.
// ═════════════════════════════════════════════════════════════════════════════════════════

bool USiegeMenuInputSubsystem::IsInMatchScreenOpen() const
{
	// 🚨 THE PREDICATE, AND IT IS THIS LINE. "An in-match screen is open" means a screen CALLED
	// `RegisterMenuNavTarget(self)` and is still live, in the viewport and visible —
	// `GetRegisteredNavTarget()` re-validates all three on every read, which is what makes a
	// screen that was destroyed without unregistering fall out by itself.
	//
	// ⛔ WHAT IT DELIBERATELY IS NOT: a survey of what is on screen. Nothing becomes "open" here
	// by being drawn, by being on top, or by existing. That is the entire reason `WBP_HUD`
	// (ZOrder 0, never collapsed) and the FPS counter (ZOrder 30) need no special case — they
	// never register, so they can never arm anything.
	return GetRegisteredNavTarget() != nullptr;
}

void USiegeMenuInputSubsystem::ReconcileInMatchArming(const TCHAR* Reason, bool bMayArm)
{
	// ⛔ THE MENU MAP IS OUT, AND THE GUARD IS `bArmed` RATHER THAN A SECOND MAP TEST.
	// `bArmed` is assigned `true` in exactly one place — `OnWorldBeginPlay`, BELOW the
	// `MapName != MenuMapName` return — so it is true on `L_MainMenu` and on no other map. A
	// second copy of that gate here would be a second thing to keep in step with the first, and
	// this file has already paid for a predicate that existed twice (TASK-1469 limb 1(b)).
	// ⚠️ Running this on the menu map would be actively wrong, not merely redundant: the context
	// is ALREADY applied there by `OnWorldBeginPlay`, and a later disarm would REMOVE the main
	// menu's own vocabulary out from under it.
	if (bArmed)
	{
		return;
	}

	const bool bWantArmed = IsInMatchScreenOpen();

	if (bWantArmed && !bInMatchVocabularyArmed)
	{
		// ⛔ `bMayArm` IS A ONE-WAY VALVE, NOT A FLAG TO TIDY AWAY. The backstop poll passes false,
		// so the only code that can ever hand a match's keys to a menu is an explicit
		// registration edge. A backstop that could arm would be a timer with the authority to
		// take a live match away from the player, which is the one outcome this row exists to
		// make impossible.
		if (bMayArm)
		{
			ArmInMatchMenuVocabulary(Reason);
		}
		return;
	}

	if (!bWantArmed && bInMatchVocabularyArmed)
	{
		DisarmInMatchMenuVocabulary(Reason);
	}
}

bool USiegeMenuInputSubsystem::ArmInMatchMenuVocabulary(const TCHAR* Reason)
{
	// Idempotent. Two screens stacking (Settings → Graphics, help → console) hit this on every
	// registration; the second and later calls must do nothing at all, because `AddMappingContext`
	// is cheap but `BindAction` APPENDS.
	if (bInMatchVocabularyArmed)
	{
		return true;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = GetLocalController();
	if (!World || !PC)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY NOT ARMED (%s): no %s — the screen is registered and its keys stay with the match."),
			Reason, World ? TEXT("local player controller") : TEXT("world"));
		return false;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;
	UEnhancedInputComponent* InputComponent = Cast<UEnhancedInputComponent>(PC->InputComponent);
	if (!InputSubsystem || !InputComponent)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY NOT ARMED (%s): Enhanced Input unavailable on '%s' (subsystem %s, component %s)."),
			Reason, *PC->GetName(),
			InputSubsystem ? TEXT("ok") : TEXT("null"),
			InputComponent ? TEXT("ok") : TEXT("null / not UEnhancedInputComponent"));
		return false;
	}

	// ─── THE ASSETS ────────────────────────────────────────────────────────────────────────────
	// ⛔ LOADED HERE AND NOT AT `OnWorldBeginPlay`, WHICH IS THE POINT: off the menu map that
	// function returns before touching any of them, so in a match with nothing open these six
	// objects are never resolved. `if (!X)` rather than an unconditional load so a re-arm after a
	// disarm re-uses what is already held. ⛔ The two doors are mutually exclusive by map, so
	// these members can never be written by both in one world.
	if (!MenuMappingContext) { MenuMappingContext = LoadObject<UInputMappingContext>(nullptr, MenuMappingContextPath); }
	if (!MenuUpAction)       { MenuUpAction       = LoadObject<UInputAction>(nullptr, MenuUpActionPath); }
	if (!MenuDownAction)     { MenuDownAction     = LoadObject<UInputAction>(nullptr, MenuDownActionPath); }
	if (!MenuAcceptAction)   { MenuAcceptAction   = LoadObject<UInputAction>(nullptr, MenuAcceptActionPath); }
	if (!MenuMappingContext || !MenuUpAction || !MenuDownAction || !MenuAcceptAction)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY NOT ARMED (%s): input assets missing (IMC_MainMenu %s, IA_MenuUp %s, IA_MenuDown %s, IA_MenuAccept %s)."),
			Reason,
			MenuMappingContext ? TEXT("ok") : TEXT("MISSING"),
			MenuUpAction       ? TEXT("ok") : TEXT("MISSING"),
			MenuDownAction     ? TEXT("ok") : TEXT("MISSING"),
			MenuAcceptAction   ? TEXT("ok") : TEXT("MISSING"));
		return false;
	}

	// The same DEGRADE-OPEN placement TASK-1409 chose on the menu path: below the fatal block, so
	// a missing IA_MenuLeft can never take the core three down with it.
	if (!MenuLeftAction)  { MenuLeftAction  = LoadObject<UInputAction>(nullptr, MenuLeftActionPath); }
	if (!MenuRightAction) { MenuRightAction = LoadObject<UInputAction>(nullptr, MenuRightActionPath); }
	if (!MenuBackAction)  { MenuBackAction  = LoadObject<UInputAction>(nullptr, MenuBackActionPath); }

	// ─── THE BINDINGS, ONCE PER INPUT COMPONENT ────────────────────────────────────────────────
	// ⛔ `BindAction` APPENDS. Unlike `OnWorldBeginPlay`, which runs once per world, this function
	// can be reached many times in one match, so an unguarded bind would make every handler fire
	// N times per press after N opens. The ledger compares against the LIVE component rather than
	// holding a bool, so a component replaced under us re-binds instead of silently going dead.
	// ⛔ NOTHING IS EVER UNBOUND on disarm, and that is correct: with the context removed no key
	// maps to these actions, so the bindings are unreachable from a real key. They stay reachable
	// from `InjectInputForAction`, which bypasses key mappings entirely — see the handoff's
	// runtime properties, where that is a DECLARED part of the negative control and not a leak.
	if (BoundInMatchInputComponent.Get() != InputComponent)
	{
		InputComponent->BindAction(MenuUpAction,     ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuUp);
		InputComponent->BindAction(MenuDownAction,   ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuDown);
		InputComponent->BindAction(MenuAcceptAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuAccept);
		if (MenuLeftAction)  { InputComponent->BindAction(MenuLeftAction,  ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuLeft); }
		if (MenuRightAction) { InputComponent->BindAction(MenuRightAction, ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuRight); }
		if (MenuBackAction)  { InputComponent->BindAction(MenuBackAction,  ETriggerEvent::Started, this, &USiegeMenuInputSubsystem::HandleMenuBack); }
		BoundInMatchInputComponent = InputComponent;
	}

	// ─── THE APPLICATION ───────────────────────────────────────────────────────────────────────
	// ⛔ BELOW `AHeroCharacter::HeroMappingContextPriority` (1) BY ONE MEASURED KEY. See the
	// constant's declaration and the class comment's census: `Enter` is the only key `IMC_Hero`
	// and `IMC_MainMenu` both map, and applying above would take it from `IA_AssistantConsole` —
	// which `HELP-§4` makes 🧑 his call, not this row's. For the other eleven keys the number is a
	// behavioural no-op, so shipping below costs exactly the one key and stomps nothing.
	// ⛔ Applied VERBATIM, with no `GetPositionalContext` call, for the same reason the menu path
	// gives: `IMC_MainMenu` carries no letter (`KBD-§4`), so there is nothing to retarget.
	InputSubsystem->AddMappingContext(MenuMappingContext, InMatchMenuMappingContextPriority);
	bInMatchVocabularyArmed = true;

	// ─── THE BACKSTOP ──────────────────────────────────────────────────────────────────────────
	// Armed WITH the vocabulary and retired WITH it, so it does not exist in a match with nothing
	// open. It catches the one transition that announces itself to nobody: a registered screen
	// destroyed, removed or hidden without its paired `UnregisterMenuNavTarget`. ⭐ It passes
	// `bMayArm = false`; it can only give the keys back.
	World->GetTimerManager().SetTimer(
		InMatchDemandPollTimerHandle, this, &USiegeMenuInputSubsystem::PollInMatchDemand,
		InMatchDemandPollSeconds, /*bLoop=*/ true);

	// ⭐ THE LINE 5b READS. It names the door (IN-MATCH, not the map gate), the trigger, the target
	// and the priority it sits below, so a reader can tell a demand arm from `OnWorldBeginPlay`'s
	// boot arm without correlating timestamps — and the shadowed-key sentence rides on it so the
	// census cannot drift away from the code that depends on it.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY ARMED (%s): IMC_MainMenu applied at priority %d on '%s' for registered screen '%s'; %d screen(s) registered. Below IMC_Hero (priority 1) — shadows NO hero key; 'Enter' stays with IA_AssistantConsole."),
		Reason, InMatchMenuMappingContextPriority, *PC->GetName(),
		*GetNameSafe(GetRegisteredNavTarget()), NavTargetStack.Num());

	return true;
}

void USiegeMenuInputSubsystem::DisarmInMatchMenuVocabulary(const TCHAR* Reason)
{
	if (!bInMatchVocabularyArmed)
	{
		return;
	}

	// ⛔ CLEARED FIRST. `RemoveMappingContext` and `ClearTimer` can both re-enter this object's
	// world in principle, and a re-entrant reader must never see "armed" while the teardown is
	// half done.
	bInMatchVocabularyArmed = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InMatchDemandPollTimerHandle);
	}

	APlayerController* PC = GetLocalController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		: nullptr;

	// ⛔ REMOVED, NOT IGNORED — the row's (3), and it is the half that protects the match. Leaving
	// the context applied and relying on `IsNavTargetActionable()` to decline would still leave a
	// menu holding the keys; at this priority it holds none of the hero's, but "it steals nothing
	// today" is a property of the CENSUS, not of the design, and the design must not depend on it.
	if (InputSubsystem && MenuMappingContext)
	{
		InputSubsystem->RemoveMappingContext(MenuMappingContext);
	}
	else
	{
		// Not an error in the ordinary teardown case — if the controller is already gone its
		// `UEnhancedPlayerInput`, which is where `AppliedInputContextData` lives, went with it —
		// but it is the one path on which this function does not do what its name says, so it says
		// so rather than returning in silence.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY DISARMED (%s): the applied context was NOT removed explicitly (subsystem %s, context %s). Expected only when the controller is already gone, which takes its UEnhancedPlayerInput — and therefore the applied context — with it."),
			Reason, InputSubsystem ? TEXT("ok") : TEXT("null"), MenuMappingContext ? TEXT("ok") : TEXT("null"));
		return;
	}

	// ⚠️ THE LIVE-TARGET CLAUSE IS READ, NOT ASSERTED. The ordinary caller is the falling edge, where
	// it is "None" — but `Deinitialize` disarms a world that may still have a screen registered and
	// live, and a line that said "none live" there would be a fail-silent in the one place a reader
	// is trying to understand a teardown.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IN-MATCH MENU VOCABULARY DISARMED (%s): IMC_MainMenu REMOVED from '%s'; %d screen(s) on the stack, live nav target now '%s'. The match has its keys back."),
		Reason, *GetNameSafe(PC), NavTargetStack.Num(), *GetNameSafe(GetRegisteredNavTarget()));
}

void USiegeMenuInputSubsystem::PollInMatchDemand()
{
	// ⛔ `bMayArm = false` — see `ReconcileInMatchArming`. The whole value of this timer is that it
	// is incapable of arming anything.
	ReconcileInMatchArming(TEXT("backstop poll"), /*bMayArm=*/ false);
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  TASK-1469 LIMB 2 — EFFECTIVE (ANCESTOR-INHERITED) VISIBILITY
//
//  🚨 THE DEFECT THIS EXISTS FOR WAS MEASURED AT RUNTIME, NOT REASONED ABOUT.
//  `qa/TASK-1413-verify.md` §6: on the Graphics screen `IA_MenuDown` printed
//  `MoveFocus(+1): focus moved 17 -> 18 of 24 ('KeepSettingsButton')` SIX CONSECUTIVE TIMES and
//  `IA_MenuUp` printed `20 -> 19 ('RevertSettingsButton')` TWICE — neither direction crossed, and
//  `BackButton` (stop 23) was unreachable by pressing Down. Those two stops sit inside
//  `VideoModeConfirmBorder`, which `USiegeGraphicsMenuWidget::BuildVideoModeConfirmRow` builds and
//  immediately sets to `ESlateVisibility::Collapsed` — the buttons' OWN visibility stays `Visible`.
//
//  ⭐ WHY A SLATE-SIDE PARENT WALK AND NOT A UMG `GetParent()` WALK — AND THIS IS THE WHOLE
//  ARGUMENT FOR THE SHAPE: the predicate that ACTUALLY decides whether a widget can take focus is
//  `FSlateApplication::SetUserFocus` (`SlateApplication.cpp:2721-2747`), which succeeds only if
//  `FSlateWindowHelper::FindPathToWidget` finds the widget, which descends through
//  `FWidgetPath::SearchForWidgetRecursively` (`WidgetPath.inl:23-52`) using
//  `FArrangedChildren(VisibilityFilter)` with the default `EVisibility::Visible`. A Collapsed or
//  Hidden panel contributes NO arranged children, so nothing beneath it is reachable — and its own
//  failure message says so: *"Attempting to focus a widget that isn't in the tree and visible"*
//  (`SlateApplication.cpp:2743`). Walking the SLATE parent chain with the SAME visibility test
//  mirrors that rule; a UMG `UWidget::GetParent()` walk would not, because it stops dead at every
//  `UUserWidget` boundary and never sees the wrapper `SWidget`s the real path traverses.
//
//  ⇒ THE PROPERTY THAT MAKES THIS SAFE, STATED SO IT CAN BE CHECKED: every stop that has EVER
//  successfully taken focus necessarily satisfied `FindPathToWidget`, and therefore satisfies this
//  predicate. This clause can only remove stops that could never have been focused in the first
//  place. That is why the main menu's measured 7-stop ring (`qa/TASK-1413-verify.md` §1, walked
//  clean in both directions with the wrap) cannot move: all seven were focused, so all seven pass.
//
//  ⚠️ LIMIT, DECLARED RATHER THAN HOPED PAST (`SC-§101`): this is ancestor VISIBILITY only. It does
//  NOT model `SWidgetSwitcher`, which arranges only its ACTIVE child regardless of the inactive
//  children's own visibility — a stop parked in an inactive switcher slot would still be admitted
//  here and would still refuse focus. ~~No registered nav screen uses one today (the only
//  `UWidgetSwitcher` in the project is `SiegeControlsHelpWidget`'s `ViewSwitcher`, and that overlay
//  never registers), so this is a latent hazard for a FUTURE screen, not a live one.~~
//
//  ⛔⛔ TASK-1474 (2026-09-25) — THAT LAST SENTENCE IS ⛔ EXPIRING AND IS STRUCK RATHER THAN
//  DELETED (`SC-§120`). ⛔ `TASK-1432` REGISTERS `USiegeControlsHelpWidget` as a nav target, so the
//  project's only `UWidgetSwitcher` — its `ViewSwitcher`, which holds the row list in one slot and
//  `DetailView` in the other (`SiegeControlsHelpWidget.cpp`, the `ViewSwitcher->AddChild(DetailView)`
//  site) — is about to sit UNDER a registered screen. ⛔ THE HAZARD IS THEREFORE NO LONGER FUTURE;
//  it is one row away, and this row's descent is what puts candidates inside it.
//  ✅ ⛔ BUT IT IS STILL ⛔ NOT LIVE, AND THE REASON IS MEASURED, NOT ASSUMED: every `UButton` in
//  both switcher branches is authored `IsFocusable = false` — `RowButton` and `BackButton` both go
//  through that file's own `ApplyButtonNotFocusable`, and its `CloseButton` comment records that
//  they were deliberately NOT flipped. ⇒ `IsNavFocusStop`'s `GetIsFocusable()` clause refuses them
//  before the switcher question is ever asked, and the controls-help count is UNMOVED by this row.
//  🚨 ⛔ THE TRIP-WIRE, WRITTEN WHERE THE NEXT EDITOR WILL HIT IT: ⛔ THE MOMENT ANYONE FLIPS
//  `RowButton` OR `BackButton` FOCUSABLE, the inactive branch's buttons become admitted-but-
//  unfocusable stops, and per the `MoveFocus` note at `IsNavFocusStop`'s limb 2 such a stop does not
//  waste a press — it ⛔ SWALLOWS THE RING. ⛔ Modelling the switcher is ⛔ NOT this row's scope and
//  is ⛔ NOT attempted here; declaring it is.
bool USiegeMenuInputSubsystem::HasVisibleSlateAncestry(const UWidget* Widget)
{
	if (!Widget)
	{
		return false;
	}

	// ⛔ THE REALIZATION SHORT-CIRCUIT, AND IT IS THE ANSWER TO TASK-1469 (2a): an unrealized widget
	// has no cached `SWidget` at all, so it fails HERE and it ALSO already fails the caller's
	// `UWidget::IsVisible()` — which returns false on an invalid `GetCachedWidget()`
	// (`UMG/Private/Components/Widget.cpp`, `UWidget::IsVisible`). Realization is NOT a third limb;
	// it is a condition this code has always tested. See the handoff's A/B/C answer.
	const TSharedPtr<SWidget> SelfSlate = Widget->GetCachedWidget();
	if (!SelfSlate.IsValid())
	{
		return false;
	}

	// ⛔ STARTS AT THE PARENT, DELIBERATELY: the widget's OWN visibility is the caller's first
	// clause and stays exactly where TASK-1406 put it, so `SiegeMenuInputTest.cpp` and every reader
	// of that line see an unchanged predicate with a second one added beside it.
	// The chain terminates: `ParentWidgetPtr` is assigned by `FSlotBase::AttachWidget` to the
	// OWNING widget, so it is a tree, and its root (an `SWindow`) has no parent.
	for (TSharedPtr<SWidget> Ancestor = SelfSlate->GetParentWidget();
		Ancestor.IsValid();
		Ancestor = Ancestor->GetParentWidget())
	{
		// `EVisibility::IsVisible()` is `0 != (Value & VIS_Visible)` (`Layout/Visibility.h`), i.e.
		// true for `Visible` / `HitTestInvisible` / `SelfHitTestInvisible` and false for `Hidden`
		// and `Collapsed` — BIT-FOR-BIT the test `FArrangedChildren` applies with the
		// `EVisibility::Visible` filter (`DoesVisibilityPassFilter`). An always-visible decorative
		// ancestor marked `HitTestInvisible` therefore does not disqualify its children.
		if (!Ancestor->GetVisibility().IsVisible())
		{
			return false;
		}
	}

	return true;
}

bool USiegeMenuInputSubsystem::IsNavFocusStop(const UWidget* Widget)
{
	// Same three liveness conditions `GetMenuButtons()` has always applied, in the same order.
	if (!Widget || !Widget->GetIsEnabled() || !Widget->IsVisible())
	{
		return false;
	}

	// ─── TASK-1469 LIMB 2: `qa/TASK-1418.md`'s WALKER ASK, NOW BACKED BY RUNTIME EVIDENCE ──────
	// The clause above reads the widget's OWN Slate visibility and never its ancestors'. That is
	// what admitted `KeepSettingsButton` / `RevertSettingsButton` into the Graphics ring while
	// their `VideoModeConfirmBorder` was Collapsed, and a stop that cannot take focus SWALLOWS the
	// ring rather than being skipped — `MoveFocus` re-reads the index from Slate every press, so a
	// focus request that fails leaves the index where it was and the next press repeats it forever.
	// ⚠️ NOT A PERMANENT EXCLUSION: `ArmVideoModeCountdown()` sets that border back to `Visible`,
	// and on that frame both buttons become stops again. The ring tracks the screen.
	if (!HasVisibleSlateAncestry(Widget))
	{
		return false;
	}

	// ⛔ FENCE (c) — THE FOUR ADMITTED CLASSES (the measured control vocabulary, TASK-1398 §3 F4).
	// ⛔ AN AUTHORED `IsFocusable == false` IS HONOURED, NEVER STOMPED: it is deliberate in this
	// project — ~~`SiegeControlsHelpWidget.cpp:177` sets it on the help overlay's CloseButton, and~~
	// `:2240` / `:2787` (⛔ WAS — `SiegeControlsHelpWidget.cpp` line numbers, cited so until
	// TASK-1480; the by-text anchors are in the (m) note below) set it on the two `UScrollBox`es
	// (which this walker never admits anyway, a `UScrollBox` not being one of the four classes).
	// Overriding an author's opt-out would be a regression wearing a widening's clothes.
	// ⭐ TASK-1480 (m) (2026-09-27, `qa/TASK-1497.md` WARN, "the dangerous half") — ⛔ THE STRUCK
	// CLAUSE IS FALSE, NOT MERELY MIS-NUMBERED: `CloseButton` is FOCUSABLE — the
	// `ApplyButtonFocusable(CloseButton);` in `USiegeControlsHelpWidget::ConstructHelpTree`, whose
	// own comment dates the flip to TASK-1432 — and since TASK-1478 no `UButton` in that file is
	// opted out at all (its `ApplyButtonNotFocusable` helper has no live caller). ⛔ THE TWO
	// SCROLL-BOX OPT-OUTS STAND, re-anchored BY TEXT: `DetailScrollBox->SetIsFocusable(false);` in
	// `USiegeControlsDetailWidget::ConstructDetailTree` and `RowScrollBox->SetIsFocusable(false);`
	// in `USiegeControlsHelpWidget::ConstructHelpTree` — the `:2240` / `:2787` above, in that
	// order, kept only so an older copy can map them.
	if (const UButton* Button = Cast<const UButton>(Widget))
	{
		if (!Button->GetIsFocusable())
		{
			return false;
		}

		// ─── TASK-1469 LIMB 3: `qa/TASK-1410.md` WARN-2's STEPPER DOUBLE-STOP ──────────────────
		// `USiegeGraphicsMenuWidget::BuildStepperRow` builds `HorizontalBox[Label, PrevButton,
		// Value, NextButton]`, and admitting BOTH buttons gave the player SIX stops where the
		// screen shows THREE rows — Down stopped twice on every stepper and neither stop did
		// anything a player could predict from looking at it.
		//
		// ⭐ THE SHAPE, WITH ITS ARGUMENT (`SC-§101` — the row named the defect, not the remedy).
		// Three shapes were available: (i) admit neither member and give the row no stop at all —
		// rejected, it makes a real control keyboard-unreachable, which is the defect this epic
		// exists to remove; (ii) synthesise a stop for the parent `RowBox` — rejected, a
		// `UHorizontalBox` has no `SWidget` that supports keyboard focus, so `FocusWidget` would
		// refuse it (`SlateWidget->SupportsKeyboardFocus()`) and manufacture a SECOND severed ring;
		// (iii) admit exactly ONE member, which is this. TASK-1409 already shipped the vocabulary
		// that makes one stop sufficient: `StepFocusedStop` calls `FindStepperPair` on WHICHEVER
		// member holds focus and presses the correct one, so Left/Right drive the whole row from
		// the surviving stop with no change to that function at all.
		//
		// ⛔ THE `Prev` MEMBER IS THE ONE KEPT, FOR TWO REASONS: it is FIRST in slot order, so the
		// surviving stop sits where the row's first interactive control already was and the ring's
		// order is a strict SUBSET of the old order with nothing permuted (which is what makes the
		// count table attributable); and `BuildStepperRow` puts it immediately after the label, so
		// the outline lands beside the words that name the row.
		//
		// ⭐ REUSING `FindStepperPair` RATHER THAN WRITING A NEW STRUCTURAL RULE IS ITSELF THE
		// SAFETY PROPERTY. It demands exactly two `UButton` children of the immediate parent AND
		// either the authored `<Base>PrevButton`/`<Base>NextButton` names or the authored `<` / `>`
		// glyphs. That is precisely why `USettingsMenuWidget`'s footer — whose `RootPanel` holds
		// exactly two buttons, `GraphicsButton` and `BackButton` — is refused and that screen keeps
		// all three of its stops. A structure-only rule would have eaten `BackButton` there.
		const UButton* StepperNext = nullptr;
		{
			UButton* PairPrev = nullptr;
			UButton* PairNext = nullptr;
			if (FindStepperPair(Button, PairPrev, PairNext) && PairPrev != nullptr)
			{
				StepperNext = PairNext;
			}
		}
		if (StepperNext != nullptr && StepperNext == Button)
		{
			return false;
		}

		return true;
	}
	if (const UCheckBox* CheckBox = Cast<const UCheckBox>(Widget))
	{
		return CheckBox->GetIsFocusable();
	}
	if (const USlider* Slider = Cast<const USlider>(Widget))
	{
		// ⚠️ READ AS A MEMBER ON PURPOSE. UButton and UCheckBox deprecated the public field in 5.2
		// in favour of `GetIsFocusable()`; USlider did NOT — `Slider.h:94-96` still declares
		// `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction") bool IsFocusable;`
		// public, with no getter and no UE_DEPRECATED. The member is the supported route here, and
		// routing it through a getter that does not exist is what a copy-paste would have done.
		return Slider->IsFocusable;
	}
	if (Widget->IsA<UEditableTextBox>())
	{
		// ⛔ NO OPT-OUT EXISTS TO HONOUR, and none is invented: `EditableTextBox.h` carries zero
		// occurrences of "focusable" (measured), because a text box is keyboard-focusable by
		// construction. Returning true here is reporting that fact, not overriding an author.
		return true;
	}
	return false;
}

bool USiegeMenuInputSubsystem::IsCodeAuthoredSubWidget(const UUserWidget* SubWidget)
{
	if (!SubWidget || !SubWidget->WidgetTree)
	{
		return false;
	}

	// ⛔ `CLASS_Native` IS THE ENGINE'S OWN NOTION OF "DECLARED IN C++", NOT A PROXY FOR IT:
	// `ObjectMacros.h:223` documents it as "Class is a native class", the Kismet compiler CLEARS it
	// on every recompiled Blueprint class (`CLASS_RecompilerClear`, `ObjectMacros.h:296`), and
	// `UBlueprintGeneratedClass` finds its C++ base by walking up until the flag appears
	// (`BlueprintGeneratedClass.cpp:132`). ⇒ a `UWidgetBlueprintGeneratedClass` NEVER carries it and
	// a `UCLASS()` in `Source/` always does. ⛔ Deliberately not `Cast<UWidgetBlueprintGeneratedClass>`:
	// that asks WHICH generated-class type this is, and the question here is who WROTE the tree.
	const UClass* SubWidgetClass = SubWidget->GetClass();
	return SubWidgetClass != nullptr && SubWidgetClass->HasAnyClassFlags(CLASS_Native);
}

void USiegeMenuInputSubsystem::CollectNavStopsFromTree(const UWidgetTree* Tree, TArray<UWidget*>& OutStops, int32 Depth)
{
	if (!Tree || Depth > MaxNavDescentDepth)
	{
		return;
	}

	Tree->ForEachWidget([&OutStops, Depth](UWidget* Widget)
	{
		// ⛔ THE PREDICATE RUNS FIRST AND UNCHANGED, so this tree's own stops appear in exactly the
		// positions and the order TASK-1469 shipped. The descent is appended BENEATH the widget that
		// owns it, never reordered around it.
		if (USiegeMenuInputSubsystem::IsNavFocusStop(Widget))
		{
			OutStops.Add(Widget);
		}

		// ─── TASK-1474: THE DESCENT, AND IT IS THE ONLY BEHAVIOURAL CHANGE ON THIS ROW ─────────
		// ⚠️ THE NESTED USER WIDGET ITSELF IS STILL OFFERED TO THE PREDICATE ABOVE. That is the one
		// reason this is written out rather than swapped for `UWidgetTree::ForEachWidgetAndDescendants`:
		// that function SKIPS `Predicate(Widget)` for any non-root nested `UUserWidget` it enters
		// (`WidgetTree.cpp:230-240`, `return` inside the cast). Today the substitution is invisible —
		// a `UUserWidget` is not one of `IsNavFocusStop`'s four admitted classes — but it would
		// silently un-admit any user widget a future vocabulary admits, and an exclusion nobody wrote
		// down is the defect class this epic exists to remove.
		if (const UUserWidget* SubWidget = Cast<UUserWidget>(Widget))
		{
			if (IsCodeAuthoredSubWidget(SubWidget))
			{
				CollectNavStopsFromTree(SubWidget->WidgetTree, OutStops, Depth + 1);
			}
		}
	});
}

void USiegeMenuInputSubsystem::GetMenuFocusStops(TArray<UWidget*>& OutStops) const
{
	OutStops.Reset();
	const UUserWidget* Target = GetActiveNavTarget();
	if (!Target || !Target->WidgetTree)
	{
		return;
	}

	// ─── (4) THE ORDER, STATED WHERE IT IS PRODUCED ────────────────────────────────────────────
	// `UWidgetTree::ForEachWidget` is a DEPTH-FIRST PRE-ORDER walk: the root, then its named-slot
	// content, then each `UPanelWidget`'s children by `GetChildAt(0 .. N-1)` — i.e. SLOT order —
	// recursing into a child before moving to its next sibling (`WidgetTree.cpp`,
	// `UWidgetTree::ForEachWidget` → `UWidgetTree::ForWidgetAndChildren`).
	//
	// ⭐ This is the SAME traversal `GetMenuButtons()` has always used, which is why the main
	// menu's order is unchanged: `Overlay/VerticalBox/Button_0..6`, top-to-bottom, exactly the
	// sequence `qa/TASK-1399-verify.md` §5.4 read live.
	//
	// ⚠️ THE LIMIT, NAMED RATHER THAN HOPED PAST: slot order is AUTHORING order, not laid-out
	// visual order. For a `UVerticalBox` or a vertical `UScrollBox` built in code (the three
	// code-authored panels all build their trees with `AddChildTo*` in reading order) the two
	// coincide by construction. For a tree authored in the UMG designer they coincide only if the
	// author added the rows top-to-bottom, and for a `UCanvasPanel` there is no visual order at
	// all — its children are absolutely positioned, so slot order is arbitrary with respect to
	// what 🧑 he sees. This walker cannot detect that case and does not pretend to.
	//
	// ═══ ⭐⭐ TASK-1474 — THE DESCENT, AND WHAT THE RECEIVED ACCOUNT OF IT GOT WRONG ═══════════
	// ⛔ THE EXCLUSION EVERY DOCUMENT IN THIS CHAIN CITED WAS THE WRONG ONE. Four places in this
	// project say a nested `UUserWidget`'s tree is skipped because `ForWidgetAndChildren` "descends
	// into named slots and `UPanelWidget` children only, and a `UUserWidget` is neither". ⛔ THE
	// SECOND HALF IS FALSE: `class UUserWidget : public UWidget, public INamedSlotInterface`
	// (`UserWidget.h:280`), so the NAMED-SLOT LIMB IS ENTERED on every nested user widget
	// (`WidgetTree.cpp:249-263`).
	// ⇒ ⭐ THE REAL PRE-TASK-1474 EXCLUSION, MEASURED: `UUserWidget::GetSlotNames` returns the
	// generated class's `InstanceNamedSlots` (`UserWidget.cpp:1264-1282`) and `GetContentForSlot`
	// returns `NamedSlotBindings` — i.e. content the OUTER tree injected, never the sub-widget's own
	// interior — and this project authors ⛔ ZERO `UNamedSlot`s: 17/17 widget assets carry only the
	// `AvailableNamedSlots` class property (grep, binary), and `Source/` constructs none. ⛔ SO THE
	// OLD NUMBERS WERE RIGHT FOR A REASON NOBODY HAD WRITTEN DOWN, which is exactly why the "before"
	// column of this row's count table was re-measured instead of inherited.
	//
	// ⛔ WHAT THE DESCENT DOES *NOT* DO, AND IT IS THE HALF THE ROW'S TITLE DOES NOT SAY: the deck
	// builder declares itself SELF-DRIVING — `UDeckBuilderWidget::RegisterAsMenuNavTarget` calls
	// `MenuInput->RegisterSelfDrivingMenuNavTarget(this)` (TASK-1471, cited by text) — so `MoveFocus`
	// returns at `DeclineIfActiveTargetSelfDriving` BEFORE it ever calls this function on that
	// screen. The ten `SlotButton`s this descent collects therefore become ⛔ COUNTED (the
	// `LogNavTargetRetarget` instrument finally sees the deck bar) and ⛔ NOT ring-walked. Escalated
	// in `handoffs/TASK-1474-programmer.md`, not resolved here (`SC-§50`, `SC-§100`).
	CollectNavStopsFromTree(Target->WidgetTree, OutStops, /*Depth*/ 0);
}

UWidget* USiegeMenuInputSubsystem::GetFocusedNavStop() const
{
	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		return nullptr;
	}

	// ⚠️ SAME NOTION OF "FOCUSED" AS `GetFocusedMenuButton()`, DELIBERATELY UNCHANGED BY TASK-1406:
	// `UWidget::HasUserFocus(PC)` is EFocusCause-agnostic, so a stop focused by the MOUSE reads as
	// focused here exactly as a button does there (`qa/TASK-1451.md` WARN-2). THAT row neither
	// widened nor narrowed it — widening it would re-open the focus-steal surface TASK-1400's
	// guard closes, and that adjudication was not its.
	// ⛔ TASK-1469 DOES NOT TOUCH THAT AXIS EITHER, and the distinction matters to a reader of the
	// limb below: `EFocusCause` (mouse vs navigation) is untouched. What limb 1(a) changes is
	// EXACT-WIDGET vs WIDGET-OR-DESCENDANT, which is an orthogonal question the engine answers with
	// a second function it has always shipped.
	TArray<UWidget*> Stops;
	GetMenuFocusStops(Stops);

	// ═══ TASK-1469 LIMB 1(a) — THE TEXT-BOX READ, AND IT IS TWO PASSES ON PURPOSE ═════════════
	// ⛔ MEASURED AT ENGINE SOURCE BY `qa/TASK-1426.md` §6, CITED NOT RE-DERIVED:
	// `SEditableTextBox::OnFocusReceived` (`Slate/Private/Widgets/Input/SEditableTextBox.cpp:309-320`)
	// FORWARDS focus to its inner `SEditableText`, while `UWidget::HasUserFocus`
	// (`UMG/Private/Components/Widget.cpp` → `FSlateUser::HasFocus`,
	// `Slate/Private/Framework/Application/SlateUser.cpp:182-185`) is EXACT-WIDGET —
	// `GetFocusedWidget() == Widget`. ⇒ a `UEditableTextBox` VISIBLY wearing the ring reported
	// `false` here, this function returned null, and `MoveFocus` took the cold branch `Current = 0`
	// EVERY press. The engine has shipped the strict-descendant twin all along —
	// `UWidget::HasUserFocusedDescendants` → `FSlateUser::HasFocusedDescendants`
	// (`SlateUser.cpp:192-195`) — and this project never called it. `AccountMenuWidget.cpp`'s
	// guard 2 already asks both halves; this is the same `||`, moved to where every caller sees it.
	//
	// 🚨 WHY TWO PASSES AND NOT ONE `||` IN ONE LOOP — THE HAZARD IS IN THE ENGINE'S DEFINITION:
	// `FSlateUser::HasFocusedDescendants` is *"the focus path CONTAINS this widget and this widget
	// is not its last element"*, and the focus path is the whole ANCESTOR CHAIN of the focused
	// widget. So if one stop is ever an ancestor of another, BOTH answer true — and
	// `GetMenuFocusStops` returns depth-first PRE-ORDER, which puts the ancestor FIRST. A single
	// fused loop would therefore return the container in preference to the control actually
	// wearing the ring. Exact focus is authoritative, so it is asked of EVERY stop before the
	// descendant question is asked of any. The extra pass is over a TArray that is already in hand.
	for (UWidget* Stop : Stops)
	{
		if (Stop && Stop->HasUserFocus(PC))
		{
			return Stop;
		}
	}
	for (UWidget* Stop : Stops)
	{
		if (Stop && Stop->HasUserFocusedDescendants(PC))
		{
			return Stop;
		}
	}
	return nullptr;
}

void USiegeMenuInputSubsystem::LogNavTargetRetarget(const TCHAR* Event) const
{
	// ─── (5) ONE LINE PER RETARGET ─────────────────────────────────────────────────────────────
	// ⭐ WHY THE COUNT IS IN IT: a count of 0 on a screen that visibly HAS controls is the single
	// most diagnostic line this epic can emit. It discriminates "the action never arrived" (no
	// line at all) from "the walker reached the tree and the tree admitted nothing" (a line
	// reading 0), which is precisely the failure the vocabulary widening exists to prevent — and
	// it makes 🧑 his own `-game` sitting produce a POSITIVE line rather than a silence.
	//
	// ⚠️ SHIPPING: `Log` verbosity is compiled out entirely under Shipping
	// (`USE_LOGGING_IN_SHIPPING` = 0 ⇒ `NO_LOGGING` = 1; no Target.cs override in this project --
	// documented at `SiegeAssistantGrammar.cpp:226-270`). In a packaged build this line does not
	// exist. Same property as the Accept line; a precondition on the READER, never a reason to
	// raise the verbosity.
	TArray<UWidget*> Stops;
	GetMenuFocusStops(Stops);

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] menu nav target %s -> '%s' (%s), %d focus stop(s), %d screen(s) registered."),
		Event,
		*GetNameSafe(GetActiveNavTarget()),
		GetRegisteredNavTarget() ? TEXT("registered screen") : TEXT("default: WBP_MainMenu"),
		Stops.Num(),
		NavTargetStack.Num());
}

bool USiegeMenuInputSubsystem::FocusFirstNavStop()
{
	// ═══ 🚨 TASK-1471 GATE 4 OF 4 — THE PLACEMENT, WHICH IS THE EASY ONE TO FORGET ═══════════════
	// ⛔ Without this the flag would stop the ring MOVING and still PUT it there: `RegisterMenuNavTarget`
	// ends here, so a declared screen would open with the focus rectangle already painted on its stop
	// 0 — a control the player never pointed at, wearing the ring, on a screen whose own navigation
	// has just placed itself somewhere else. ⭐ Gated INSIDE this function rather than at its two call
	// sites so BOTH are covered: the registration above and `UnregisterMenuNavTarget`'s hand-back,
	// which re-places focus when a nested screen closes onto a still-registered one underneath.
	// ⚠️ ON THE DECK BUILDER THIS SUBTRACTS A CALL THAT WAS ALREADY BEING OVERWRITTEN, not a behaviour:
	// `UDeckBuilderWidget::NativeConstruct` runs `AcquireBuilderFocus()` immediately AFTER
	// `RegisterAsMenuNavTarget()` and re-takes the focus onto the widget itself with
	// `EFocusCause::SetDirectly` — its own comment calls the ordering load-bearing and names
	// TASK-1307's `IDENTITY=MATCH` post-flush read-back as the falsifier. That read-back is preserved:
	// the state it samples is the one `AcquireBuilderFocus` leaves, and that call is untouched.
	// ⛔ Returning false is honest rather than convenient: the contract is "true only when focus was
	// actually requested and taken", and nothing was requested.
	if (DeclineIfActiveTargetSelfDriving(TEXT("FocusFirstNavStop")))
	{
		return false;
	}

	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		return false;
	}

	TArray<UWidget*> Stops;
	GetMenuFocusStops(Stops);
	if (Stops.Num() == 0)
	{
		return false;
	}

	// Idempotent, the SAME shape as `ApplyInitialFocus`'s `!GetFocusedMenuButton()` guard and for
	// the same measured reason: `FSlateApplication::SetUserFocus` early-returns false when the
	// target is ALREADY focused (`SlateApplication.cpp:3028-3033`), so a re-request on a focused
	// stop is not a failure and must never be retried as one (TASK-1446).
	// ⛔ The list is walked inline rather than through `GetFocusedNavStop()` so the tree is
	// collected ONCE per call instead of twice.
	//
	// ═══ TASK-1469 LIMB 1(b) — THE TWIN EARLY-OUT, AND IT IS THE EASY ONE TO MISS ═════════════
	// ⛔ This is the SAME blind predicate `GetFocusedNavStop()` carried, inlined here for the
	// one-walk reason above, so it had the SAME defect and had to move WITH it. Left alone it
	// would be strictly WORSE than the bug it mirrors: a player standing in a text box reads as
	// "nothing is focused", the idempotence guard falls through, and `FocusWidget(Stops[0])` YANKS
	// THE RING OUT OF THE FIELD THEY ARE TYPING IN on every re-registration —
	// `RegisterMenuNavTarget` ends in this call and `UAccountMenuWidget::ApplyMode` re-registers on
	// every mode change. ⛔ ONE `||`, not two passes: this loop asks "is ANYTHING already focused?",
	// which is order-independent, so the ancestor-precedence hazard that forces two passes in
	// `GetFocusedNavStop()` cannot arise here — it never returns WHICH stop.
	for (const UWidget* Stop : Stops)
	{
		if (Stop && (Stop->HasUserFocus(PC) || Stop->HasUserFocusedDescendants(PC)))
		{
			return false;
		}
	}

	return FocusWidget(Stops[0]);
}

void USiegeMenuInputSubsystem::RegisterMenuNavTarget(UUserWidget* Screen)
{
	if (!Screen)
	{
		// Loud, because the one way to get this wrong from a Blueprint graph is to leave the pin
		// unwired, and a silent no-op there would look exactly like a working registration.
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] RegisterMenuNavTarget(null) ignored — pass the screen widget itself (`self`)."));
		return;
	}

	// Re-registering an already-registered screen MOVES it to the top instead of duplicating it,
	// and dead entries are compacted on the way through.
	NavTargetStack.RemoveAll([Screen](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Screen;
	});
	NavTargetStack.Add(Screen);

	// ═══ 🚨 TASK-1429 — THE RISING EDGE, AND IT IS THE ONLY ONE IN THE PROJECT ════════════════
	// ⛔ THIS CALL IS THE WHOLE OF "ARM IN-MATCH". Off the menu map it applies `IMC_MainMenu` on
	// demand; on the menu map it returns immediately (`bArmed`). ⛔ Placed AFTER the stack add so
	// the demand predicate it reads is the post-registration one, and BEFORE the log below so the
	// retarget line cannot describe a state the input layer has not reached yet.
	// ⚠️ A HELD KEY CAN COST ONE EXTRA STEP HERE, AND IT IS DECLARED RATHER THAN HIDDEN: a menu key
	// already down when the context arrives gets its `Started` edge on the first tick under the new
	// mapping, so opening a screen with `Down` held can move the ring one stop. The ring is being
	// placed on stop 0 two lines below in any case, so the cost is bounded at one menu step on a
	// screen the player just opened — never a gameplay action, because at this priority the
	// context claims no hero key.
	ReconcileInMatchArming(TEXT("RegisterMenuNavTarget"), /*bMayArm=*/ true);

	LogNavTargetRetarget(TEXT("registered"));

	// Place the ring, on `ApplyInitialFocus`'s contract: stop 0, only when nothing on this target
	// already holds focus. ⛔ Without this a registered screen opens RINGLESS and the first Down
	// skips stop 0 — navigable, and still 🧑 his complaint.
	FocusFirstNavStop();
}

void USiegeMenuInputSubsystem::RegisterSelfDrivingMenuNavTarget(UUserWidget* Screen)
{
	if (!Screen)
	{
		// The SAME loud null guard `RegisterMenuNavTarget` carries, and it is here rather than left to
		// that call because the mark must never be taken on a null pin: a silent no-op from a
		// Blueprint graph with an unwired pin would look exactly like a working declaration.
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] RegisterSelfDrivingMenuNavTarget(null) ignored — pass the screen widget itself (`self`)."));
		return;
	}

	// ⛔ THE DECLARATION IS RECORDED **BEFORE** THE REGISTRATION, AND THE ORDER IS THE WHOLE REASON
	// THIS IS A SEPARATE ENTRY POINT RATHER THAN A SETTER THE SCREEN CALLS AFTERWARDS:
	// `RegisterMenuNavTarget` ENDS in `FocusFirstNavStop()`, which would otherwise place the generic
	// ring on this screen's stop 0 before anything had a chance to say it should not. A screen cannot
	// fix that by calling a setter second, and a screen that called one first would be relying on an
	// ordering rule nobody enforces. Recording it here makes the rule structural.
	//
	// De-duplicated the same way the stack is, and for the same reason: a screen that declares twice
	// (a re-open of a reused `UUserWidget`, which survives `RemoveFromParent`) must hold ONE mark, and
	// dead marks are compacted on the way past rather than accumulating for the world's lifetime.
	SelfDrivingScreens.RemoveAll([Screen](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Screen;
	});
	SelfDrivingScreens.Add(Screen);

	// ⭐ AND THEN THE ORDINARY REGISTRATION, UNCHANGED AND UNCOPIED. The stack, the in-match arm and
	// the retarget line WITH THIS SCREEN'S FOCUS-STOP COUNT all happen exactly as they do for any
	// other screen — which is the half of this row that keeps the screen ENUMERABLE. ⛔ Not a
	// duplicated body: there is one registration path in this class and this call enters it.
	RegisterMenuNavTarget(Screen);
}

void USiegeMenuInputSubsystem::UnregisterMenuNavTarget(UUserWidget* Screen)
{
	if (!Screen)
	{
		UE_LOG(LogSiegeMenuInput, Warning,
			TEXT("[USiegeMenuInputSubsystem] UnregisterMenuNavTarget(null) ignored — pass the screen widget itself (`self`)."));
		return;
	}

	// Remove THIS screen by identity, not by popping the top: closes can happen out of order, and
	// popping would silently unregister somebody else's screen. Dead entries go at the same time.
	// ⚠️ `bWasRegistered` is measured BEFORE the purge and NOT taken from RemoveAll's count: the
	// same call also drops dead entries, so a non-zero count would report "unregistered" for a
	// screen that was never on the stack whenever anything stale happened to be sitting there.
	const bool bWasRegistered = NavTargetStack.ContainsByPredicate([Screen](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return Entry.Get() == Screen;
	});
	NavTargetStack.RemoveAll([Screen](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Screen;
	});

	// ═══ ⭐ TASK-1471 — THE DECLARATION IS DROPPED WITH THE REGISTRATION ══════════════════════
	// ⛔ HERE, AND NOWHERE ELSE, SO THE FLAG'S LIFETIME IS EXACTLY THE REGISTRATION'S. There is no
	// second un-declare call for a screen's teardown to forget, and a screen that re-opens later
	// starts undeclared unless it declares again. ⚠️ Deliberately NOT done by the plain
	// `RegisterMenuNavTarget`: `UAccountMenuWidget::ApplyMode` re-registers on every mode change, and
	// a re-registration is a re-assertion of ownership, not a change of navigation model.
	// ⛔ Unconditional, above the `!bWasRegistered` early-return, for the same reason the reconcile
	// below is: this is state, not a log decision, and the same `RemoveAll` idiom drops dead marks.
	SelfDrivingScreens.RemoveAll([Screen](const TWeakObjectPtr<UUserWidget>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Screen;
	});

	// ═══ 🚨 TASK-1429 — THE FALLING EDGE ══════════════════════════════════════════════════════
	// ⛔ ABOVE THE `!bWasRegistered` EARLY-RETURN ON PURPOSE. That return is about whether to LOG a
	// retarget; the input layer's state is a different question and must be reconciled either way.
	// The `RemoveAll` above also drops DEAD entries, so a call that found nothing to unregister can
	// still have emptied the stack — and that is exactly the state in which the match must get its
	// keys back. Skipping the reconcile here would make a forgotten-then-late unregister depend on
	// the backstop timer for something an edge already knew.
	// ⚠️ `bMayArm = true` rather than false: the contract is "make the applied state equal the
	// demand", and a nested screen (Settings → Graphics → Back) leaves a LIVE target behind, so
	// the honest answer on this edge is sometimes "stay armed" and never "disarm because something
	// closed". The one-way valve belongs to the backstop, not to an explicit edge.
	// ⛔ A KEY HELD ACROSS THIS DISARM FIRES NOTHING: all six bindings are `ETriggerEvent::Started`,
	// the edge has already passed, and at this priority no hero mapping was displaced, so nothing
	// is handed back and re-triggered either.
	ReconcileInMatchArming(TEXT("UnregisterMenuNavTarget"), /*bMayArm=*/ true);

	if (!bWasRegistered)
	{
		// Not an error — a screen may unregister from both `BackPressed` and `NativeDestruct`, and
		// the second call is the no-op that makes that pairing safe. Logged, not warned, so a
		// reader can still see it happened.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] UnregisterMenuNavTarget('%s'): not registered (already unregistered, or never was) — no change."),
			*GetNameSafe(Screen));
		return;
	}

	LogNavTargetRetarget(TEXT("unregistered"));

	// ⛔ FOCUS IS RE-PLACED ONLY WHEN ANOTHER REGISTERED SCREEN IS TAKING OVER (Graphics → Settings).
	// When the stack empties, the target reverts to the DEFAULT and TASK-1400's 0.2 s re-entry poll
	// owns the re-arm — it is the path that carries the coverage check, and a focus call from here
	// could land on the main menu while the closing screen is still drawn (an unregister runs from
	// `BackPressed`, BEFORE `RemoveFromParent`).
	if (GetRegisteredNavTarget() != nullptr)
	{
		FocusFirstNavStop();
	}
}

UButton* USiegeMenuInputSubsystem::GetFocusedMenuButton() const
{
	APlayerController* PC = GetLocalController();
	if (!PC)
	{
		return nullptr;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	for (UButton* Button : Buttons)
	{
		if (Button->HasUserFocus(PC))
		{
			return Button;
		}
	}
	return nullptr;
}

FString USiegeMenuInputSubsystem::GetButtonLabel(const UButton* Button)
{
	if (!Button)
	{
		return FString();
	}

	// The shipped WBP_MainMenu button idiom is a UButton whose content is one UTextBlock
	// (AccountMenuWidget.cpp:331 "font 28" family). Walk the content subtree for the first
	// text block rather than assuming depth 1, so a wrapping panel does not blind the read.
	TArray<const UWidget*> Pending;
	for (int32 Index = 0; Index < Button->GetChildrenCount(); ++Index)
	{
		Pending.Add(Button->GetChildAt(Index));
	}
	while (Pending.Num() > 0)
	{
		const UWidget* Current = Pending.Pop(EAllowShrinking::No);
		if (!Current)
		{
			continue;
		}
		if (const UTextBlock* Text = Cast<const UTextBlock>(Current))
		{
			return Text->GetText().ToString();
		}
		if (const UPanelWidget* Panel = Cast<const UPanelWidget>(Current))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				Pending.Add(Panel->GetChildAt(Index));
			}
		}
	}
	return FString();
}

int32 USiegeMenuInputSubsystem::WrapIndex(int32 Current, int32 Delta, int32 Count)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	// ((x % n) + n) % n: a true ring for negative steps too (Up from the top lands on the last).
	const int32 Raw = (Current + Delta) % Count;
	return (Raw + Count) % Count;
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  HANDLERS
// ═════════════════════════════════════════════════════════════════════════════════════════

void USiegeMenuInputSubsystem::HandleMenuUp()
{
	MoveFocus(-1);
}

void USiegeMenuInputSubsystem::HandleMenuDown()
{
	// TASK-1394 instrument. FIRST STATEMENT, unconditional, before MoveFocus is called: this is
	// the line that makes a SILENCE readable. Placed after any branch it would print nothing in
	// exactly the cases a reader must tell apart -- "the handler never ran" (the action did not
	// route here at all) vs "it ran and declined" (MoveFocus took an early exit). Same category
	// and verbosity as the already-proven-live IA_MenuAccept line below.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuDown -> HandleMenuDown() entered; calling MoveFocus(+1)."));

	MoveFocus(+1);
}

void USiegeMenuInputSubsystem::MoveFocus(int32 Delta)
{
	// ═══ 🚨 TASK-1471 GATE 1 OF 4 — **BEFORE THE MOVE**, AND THE POSITION IS THE REQUIREMENT ═════
	// ⛔ THIS IS THE LINE THAT STOPS THE MEASURED TRANSIT. `qa/TASK-1427-verify.md` read every deck
	// builder `IA_MenuDown` moving this ring onto stop 1 — `Overlay_19/VerticalBox_0/HorizontalBox_1/
	// Button_1`, "Reset to Default", which wipes 🧑 his deck and auto-saves it — WITHIN the press,
	// before the screen's own repair pulled focus back. This subsystem binds at `OnWorldBeginPlay`,
	// before any `NativeConstruct`, so its handler is FIRST in the shared component's array: the flag
	// has to be read here, ahead of the move, or it is reading about a move that already happened.
	// ⭐ ABOVE `IsNavTargetActionable()` AND PROVABLY UNABLE TO CHANGE ANY OTHER BRANCH: the gate is
	// non-null only when `GetRegisteredNavTarget()` is non-null, and `IsNavTargetActionable()` returns
	// true unconditionally in exactly that case. So for every press it can refuse, the gate below
	// would have passed; for every other press this one falls straight through.
	if (DeclineIfActiveTargetSelfDriving(FString::Printf(TEXT("MoveFocus(%+d)"), Delta)))
	{
		return;
	}

	// TASK-1406 fence (b): the predicate is the only thing that changed on this line. With nothing
	// registered `IsNavTargetActionable()` IS `IsMenuUncovered()` -- same call, same body.
	if (!IsNavTargetActionable())
	{
		// TASK-1394 instrument. Delta rides on EVERY exit line because MoveFocus is SHARED with
		// HandleMenuUp: a line that cannot tell Up from Down is not an instrument. The condition
		// itself is untouched -- the log is the whole of the addition.
		// ⚠️ TASK-1406 KEPT THIS STRING BYTE-IDENTICAL AND IS FLAGGING IT RATHER THAN IMPROVING IT:
		// the exit now means "the active nav target is not actionable", which on a REGISTERED
		// target can no longer be "covered" at all. TASK-1395 / TASK-1402 read these four strings
		// as discriminators, so re-wording one would break a downstream reader's grep for a
		// cosmetic gain. ⛔ Declared for the manager (`SC-§101`), not self-adjudicated.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: menu covered."), Delta);
		return;
	}

	// TASK-1406 fence (c): the WALKER is what changed here. `GetMenuFocusStops` admits UButton,
	// UCheckBox, USlider and UEditableTextBox instead of UButton alone, and it walks the ACTIVE
	// nav target's tree instead of `FindMainMenuWidget()`'s unconditionally. On the default target
	// both reduce to what they were: the main menu's stops are its seven buttons.
	TArray<UWidget*> Stops;
	GetMenuFocusStops(Stops);
	if (Stops.Num() == 0)
	{
		// TASK-1394 instrument.
		// ⚠️ Byte-identical for the same reason as the line above: it now means "no focus stops".
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d) declined: no menu buttons."), Delta);
		return;
	}

	// The current index is READ from Slate each time (never cached). Cold — no focus stop
	// focused, e.g. focus on the viewport or on a widget the mouse touched — reads as 0, the
	// top; so from cold Down ×2 lands on index 2 ("Deck Builder"), the row's test.
	int32 Current = 0;
	if (const UWidget* Focused = GetFocusedNavStop())
	{
		Current = Stops.IndexOfByKey(Focused);
	}

	const int32 Next = WrapIndex(Current, Delta, Stops.Num());
	if (Stops.IsValidIndex(Next))
	{
		// TASK-1394 instrument. Reported from the index MoveFocus CHOSE, never from FocusButton's
		// return value: FSlateApplication::SetUserFocus early-returns false when the target is
		// ALREADY focused (SlateApplication.cpp:3028-3033), so a false there is NOT a failure and
		// must never be logged as a refusal (TASK-1446).
		// ⚠️ Format string byte-identical; only the array it indexes widened.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] MoveFocus(%+d): focus moved %d -> %d of %d ('%s')."),
			Delta, Current, Next, Stops.Num(), *Stops[Next]->GetName());
		FocusWidget(Stops[Next]);
	}
}

void USiegeMenuInputSubsystem::HandleMenuAccept()
{
	// ═══ 🚨 TASK-1471 GATE 2 OF 4 — THIS IS THE ONE THAT ANSWERS "CLOSES OR NARROWS?" ════════════
	// ⛔ CLOSES. With the flag set, "Reset to Default" is not merely hard to reach from a
	// tile-focused state — there is no path through this function that presses ANY button on the
	// declared screen, from ANY focus state, cold or not. ⭐ That is a property of the control flow,
	// not of where the ring happens to be standing.
	//
	// ⛔⛔ TASK-1474 (2026-09-25) — THE MAP ABOVE WAS WRONG AND IS CORRECTED HERE RATHER THAN
	// DELETED (`SC-§120`). ⛔ was: *"The ONLY two statements in this class that can activate a
	// control on the active target are `SetCheckBoxChecked(...)` and `Focused->OnClicked.Broadcast()`,
	// and both live BELOW this return."* ⛔ THERE ARE ⛔ SEVEN, and five of them are ⛔ NOT below this
	// return — they are in `StepFocusedStop` / `StepSlider`, reached from `IA_MenuLeft` / `IA_MenuRight`:
	//    (1) `SetCheckBoxChecked(CheckBox, !CheckBox->IsChecked(), …)` — HandleMenuAccept, below this return
	//    (2) `Focused->OnClicked.Broadcast()`                         — HandleMenuAccept, below this return
	//    (3) `SetCheckBoxChecked(CheckBox, Direction > 0, …)`         — StepFocusedStop
	//    (4) `Member->OnClicked.Broadcast()`                          — StepFocusedStop
	//    (5) `Slider->SetValue(NewValue)`                             — StepSlider
	//    (6) `Slider->OnControllerCaptureEnd.Broadcast()`             — StepSlider
	//    (7) `Slider->OnMouseCaptureEnd.Broadcast()`                  — StepSlider
	// ⛔ NAMED BY STATEMENT, ⛔ NEVER BY LINE NUMBER: this enumeration is the map, and a map written
	// in indices goes stale on the next insertion with nothing to announce it.
	// ✅ ⭐ THE VERDICT "CLOSES" ⛔ SURVIVES INTACT, AND ON A ⛔ WIDER FOOTING THAN THE OLD SENTENCE
	// CLAIMED: (3)–(7) are ALL behind `StepFocusedStop`'s own `DeclineIfActiveTargetSelfDriving`
	// gate (TASK-1471 gate 3 of 4 — that function's first statement after its unconditional entry
	// instrument, and before any other branch), and `StepSlider` is private with exactly ONE caller
	// — `StepFocusedStop`, downstream of that gate. ⇒ every one of the seven is unreachable on a
	// declared screen. ⛔ WHAT WAS WRONG WAS THE
	// MAP, NOT THE CONCLUSION — and the map is what the next editor reads before deciding where a
	// new activation may safely go. ⚖️ *A count that is short by five reads as an exhaustive
	// enumeration and licenses the reader to skip the two functions it forgot.*
	// ⛔ FIRST STATEMENT: this handler has no entry instrument to sit behind, and the refusal must be
	// readable — an Accept that silently did nothing is the exact ambiguity TASK-1409 (4b) exists to
	// remove.
	if (DeclineIfActiveTargetSelfDriving(TEXT("IA_MenuAccept")))
	{
		return;
	}

	// TASK-1406 fence (b), same substitution as MoveFocus: with nothing registered this IS
	// `IsMenuUncovered()`.
	if (!IsNavTargetActionable())
	{
		return;
	}

	// ⛔ TASK-1406 KEPT ACCEPT AT `UButton` ONLY AND FLAGGED IT; TASK-1409 (2) + (4b) DISCHARGE
	// THAT FLAG. The walker's vocabulary widened there, Accept's widens here, and it widens by
	// exactly ONE class — the one (2) specifies. On the DEFAULT target every focus stop is still
	// a UButton, so both branches below are unreachable there and the shipped path is unchanged.
	// ⛔ A `USlider` and a `UEditableTextBox` deliberately get NO Accept semantics: "Accept on a
	// slider" has no agreed meaning and inventing one is the unreviewed-semantic shape this epic
	// removes. They fall to the one-line branch, which is the point of (4b).
	// ⚠️ `GetFocusedNavStop()` is called ONCE and held: it walks the whole target tree.
	UWidget* FocusedStop = GetFocusedNavStop();

	// TASK-1409 (2): "⛔ `UCheckBox` ⇒ ... ⛔ Accept toggles".
	if (UCheckBox* CheckBox = Cast<UCheckBox>(FocusedStop))
	{
		SetCheckBoxChecked(CheckBox, !CheckBox->IsChecked(), TEXT("IA_MenuAccept"));
		return;
	}

	UButton* Focused = Cast<UButton>(FocusedStop);
	if (!Focused)
	{
		// Cold Accept does nothing, deliberately: firing "Play (vs Bot)" on an Enter that
		// landed with no visible focus would travel to the arena with no indication of why.
		//
		// ─── TASK-1409 (4b), from `qa/TASK-1407.md` WARN-6 ─────────────────────────────────────
		// ⛔ ONE LINE, NOT A BEHAVIOUR CHANGE. This branch now has TWO causes — "nothing is
		// focused" and "a stop is focused but Accept has no semantics for it" — and before this
		// line a reader could not tell them apart, nor either of them from "the action never
		// arrived". That unreadable silence is what cost this epic four rows of guessing; it is
		// the one outcome forbidden, because it is indistinguishable from a broken ring.
		// ⚠️ The class name is materialised into a NAMED LOCAL rather than dereferenced inside a
		// ternary: mixing `*FString` (which is `TCHAR*`) with a `const TCHAR*` literal in a
		// conditional is legal but is exactly the shape a `UE_LIFETIMEBOUND` reader has to squint at.
		const FString StopClassName = FocusedStop
			? FocusedStop->GetClass()->GetName()
			: FString(TEXT("none focused"));
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept declined: focused stop '%s' (%s) has no Accept semantics (or nothing is focused)."),
			*GetNameSafe(FocusedStop), *StopClassName);
		return;
	}

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on '%s' (\"%s\")."),
		*Focused->GetName(), *GetButtonLabel(Focused));

	// The SAME delegate a mouse click fires: UButton::SlateHandleClicked (Button.cpp:278-280)
	// is exactly `OnClicked.Broadcast()`. Every Blueprint handler bound on the button runs
	// unchanged — ⛔ no OnClicked body is touched by this feature.
	Focused->OnClicked.Broadcast();
}

// ═════════════════════════════════════════════════════════════════════════════════════════
//  TASK-1409 — LEFT / RIGHT (the per-type semantics) AND BACK (the close request)
// ═════════════════════════════════════════════════════════════════════════════════════════

void USiegeMenuInputSubsystem::HandleMenuLeft()
{
	StepFocusedStop(-1);
}

void USiegeMenuInputSubsystem::HandleMenuRight()
{
	StepFocusedStop(+1);
}

void USiegeMenuInputSubsystem::StepFocusedStop(int32 Direction)
{
	// Entry instrument, FIRST STATEMENT, unconditional — TASK-1394's doctrine applied to the new
	// pair: placed after any branch it would print nothing in exactly the cases a reader must
	// tell apart ("the action never routed here" vs "it ran and declined"). `Direction` rides on
	// EVERY line below for the same reason `Delta` rides on MoveFocus's: one body serves Left and
	// Right, and a line that cannot tell them apart is not an instrument.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuLeft/IA_MenuRight -> StepFocusedStop(%+d) entered."), Direction);

	// ═══ 🚨 TASK-1471 GATE 3 OF 4 — AND IT IS THE MUTATING PAIR, SO IT IS NOT OPTIONAL ═══════════
	// ⛔ Directly after the entry instrument (which must stay unconditional and first) and before
	// everything else. Left/Right are the two actions that CHANGE A VALUE — a slider's, a check box's
	// or a stepper's — on whatever this ring is standing on. On a screen that drives itself, the ring
	// is standing somewhere the player never pointed it, so a step here mutates a control by accident.
	// ⚠️ ONE READING CHANGES WORDING HERE AND IT IS DECLARED RATHER THAN DISCOVERED: the single
	// `declined: no focus stop holds focus` line `qa/TASK-1427-verify.md` recorded in the deck builder
	// (an `IA_MenuRight` with a card tile focused) becomes this row's line instead. ⛔ The screen's OWN
	// `IA_MenuRight` handler is a separate binding on the same action and is untouched, so the
	// builder's own 2-D ring step is unaffected.
	if (DeclineIfActiveTargetSelfDriving(FString::Printf(TEXT("StepFocusedStop(%+d)"), Direction)))
	{
		return;
	}

	// The SAME gate the other handlers run (TASK-1406 fence (b)): registered ⇒ true, nothing
	// registered ⇒ `IsMenuUncovered()`, unchanged.
	if (!IsNavTargetActionable())
	{
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d) declined: nav target not actionable."), Direction);
		return;
	}

	UWidget* Focused = GetFocusedNavStop();
	if (!Focused)
	{
		// ⛔ NO "COLD" FALLBACK HERE, deliberately, and it is NOT the same call MoveFocus makes.
		// Up/Down treat cold as index 0 because moving a ring from nowhere to its top is
		// harmless and is what the player asked for. Left/Right MUTATE A VALUE — stepping "the
		// first slider on the screen" because nothing was focused would change a setting the
		// player never pointed at.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d) declined: no focus stop holds focus."), Direction);
		return;
	}

	// ─── (2) THE DISPATCH. THIS, NOT THE BINDING, IS THE DELIVERABLE. ──────────────────────────
	if (USlider* Slider = Cast<USlider>(Focused))
	{
		StepSlider(Slider, Direction);
		return;
	}

	if (UCheckBox* CheckBox = Cast<UCheckBox>(Focused))
	{
		// ⛔ SET, NEVER TOGGLE. Left is OFF and Right is ON, so the key is idempotent: holding
		// Right cannot flicker a setting, and a player who cannot see the box still knows which
		// key produces which state. Accept is the toggle (see `HandleMenuAccept`).
		SetCheckBoxChecked(CheckBox, Direction > 0,
			FString::Printf(TEXT("StepFocusedStop(%+d)"), Direction));
		return;
	}

	if (UButton* Button = Cast<UButton>(Focused))
	{
		UButton* PrevButton = nullptr;
		UButton* NextButton = nullptr;
		if (FindStepperPair(Button, PrevButton, NextButton))
		{
			UButton* Member = (Direction < 0) ? PrevButton : NextButton;
			if (Member)
			{
				UE_LOG(LogSiegeMenuInput, Log,
					TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): stepper pair at '%s' -> OnClicked.Broadcast() on '%s' (\"%s\")."),
					Direction, *Button->GetName(), *Member->GetName(), *GetButtonLabel(Member));

				// The SAME delegate a mouse click fires (`UButton::SlateHandleClicked` is exactly
				// `OnClicked.Broadcast()`), which is how the graphics steppers' own handlers run:
				// they are attached with `OnClicked.AddUniqueDynamic(...)` in that widget's bind
				// pass. ⛔ No stepper's handler body is touched by this feature.
				Member->OnClicked.Broadcast();
			}
			return;
		}

		// ⛔ (2): "a plain `UButton` ⇒ Left/Right do NOTHING — and that nothing is DELIBERATE,
		// logged at Verbose, never a silent swallow." Verbose rather than Log because this is the
		// COMMON case on the main menu (seven plain buttons); at Log it would drown the lines that
		// matter. ⚠️ Verbose needs `Log LogSiegeMenuInput Verbose` to appear at all.
		UE_LOG(LogSiegeMenuInput, Verbose,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): focused stop '%s' (\"%s\") is a plain UButton, not half of a stepper pair — Left/Right do nothing, deliberately."),
			Direction, *Button->GetName(), *GetButtonLabel(Button));
		return;
	}

	// ─── (4b), GENERALISED: THE ONE OUTCOME FORBIDDEN IS A SILENT SWALLOW ──────────────────────
	// Reached today by a focused `UEditableTextBox` — the fourth admitted class, for which
	// Left/Right means "move the caret", which belongs to Slate's own text handling and is not
	// something this subsystem can or should simulate. It gets a line naming the stop and its
	// class, the same shape as the deliberate `UButton` nothing above.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): focused stop '%s' (%s) has no Left/Right semantics — nothing done."),
		Direction, *Focused->GetName(), *Focused->GetClass()->GetName());
}

void USiegeMenuInputSubsystem::StepSlider(USlider* Slider, int32 Direction) const
{
	if (!Slider)
	{
		return;
	}

	const float MinValue = Slider->GetMinValue();
	const float MaxValue = Slider->GetMaxValue();
	if (MaxValue <= MinValue)
	{
		// Guarded rather than clamped: `FMath::Clamp` with an inverted range answers with the
		// bound rather than an error, so a mis-authored slider would silently snap to Max on the
		// first press. Say so instead.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d) declined: slider '%s' has an empty range [%.4f, %.4f]."),
			Direction, *Slider->GetName(), MinValue, MaxValue);
		return;
	}

	// ⛔ THE CONTROL'S OWN STEP WINS. `USlider::GetStepSize()` reads the live `SSlider` when one
	// exists and the UPROPERTY otherwise, so this is the same number the author set: the shipped
	// quality sliders declare `1/MaxQualityLevel` = 0.25 on a 0..1 range (one detent per press,
	// five detents) and resolution scale declares 1.0 on a 50..100 range (one percent per press).
	// `!(Step > 0)` rather than `Step <= 0` so a NaN takes the fallback instead of the multiply.
	const float DeclaredStep = Slider->GetStepSize();
	const bool bHasOwnStep = (DeclaredStep > 0.f);
	const float Step = bHasOwnStep ? DeclaredStep : (MaxValue - MinValue) * DefaultSliderStepFraction;

	const float OldValue = Slider->GetValue();
	const float NewValue = FMath::Clamp(OldValue + (Direction * Step), MinValue, MaxValue);

	// 🚨🚨 NO `OnValueChanged.Broadcast` HERE, AND THE ABSENCE IS THE MEASUREMENT:
	// `USlider::SetValue` calls `HandleOnValueChanged(InValue)` whenever the value actually
	// changes, and `HandleOnValueChanged` broadcasts `OnValueChanged` itself (`Slider.cpp`).
	// Adding a broadcast would fire every bound handler TWICE — on the graphics panel that is
	// the LABEL refreshed twice per press. ⛔ This is the exact opposite of the check-box path
	// three functions down, and the asymmetry is the engine's, not a choice: this project
	// measured it independently at `SiegeGraphicsMenuWidget::SeedAndBind`, whose comment reads
	// "USlider::SetValue(float) CALLS HandleOnValueChanged(InValue) and therefore BROADCASTS
	// OnValueChanged ... unlike UCheckBox::SetIsChecked, which does not".
	// ⚠️ READ THE COMMIT-EDGE BLOCK BELOW BEFORE CONCLUDING THAT "THE SLIDER NEEDS NO BROADCAST":
	// `OnValueChanged` is not the signal that WRITES on this project's sliders.
	Slider->SetValue(NewValue);

	// Read BACK rather than trusting the write: `GetValue()` returns the live `SSlider`'s value
	// when the Slate widget exists, so a value the widget refused (or re-snapped to a detent)
	// shows up here instead of being reported as the number we asked for. This is also the field
	// the runtime criterion's node read looks at.
	const float ReadBack = Slider->GetValue();

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): slider '%s' %.4f -> %.4f (step %.4f, %s; range [%.4f, %.4f]); read back %.4f."),
		Direction, *Slider->GetName(), OldValue, NewValue, Step,
		bHasOwnStep ? TEXT("the control's own StepSize") : TEXT("DefaultSliderStepFraction of the range"),
		MinValue, MaxValue, ReadBack);

	// ═════════════════════════════════════════════════════════════════════════════════════════
	// 🚨 THE COMMIT EDGE (TASK-1410 BLOCKER-1) — ⛔ MOVING THE HANDLE IS NOT CHANGING THE SETTING
	//
	// The paragraph above is true of the ENGINE and it is NOT the whole story, because this
	// PROJECT's sliders are TWO-PHASE and the second phase is a DIFFERENT delegate:
	//   • `OnValueChanged` → `Handle*SliderValueChanged` — ⛔ LABEL ONLY, explicitly forbidden to
	//     write ("⛔ LABEL ONLY — no facade call, no apply, no save" / "🚨 ⛔ THIS FUNCTION MUST
	//     NEVER WRITE", `SiegeGraphicsMenuWidget.cpp`), because a mouse drag fires it once per
	//     FRAME and a facade write there is up to fifty ini writes per drag.
	//   • `OnMouseCaptureEnd` + `OnControllerCaptureEnd` → `Handle*SliderCommitted` — ⛔ THE ONLY
	//     FACADE WRITERS. Every slider this project ships binds the SAME committed handler to
	//     BOTH of them (`SeedAndBind`; the graphics screen is the only screen with a `USlider`).
	//
	// `USlider::SetValue` reaches ONLY the first: `Slider.cpp`'s `SetValue` → `HandleOnValueChanged`
	// → `OnValueChanged.Broadcast`. The capture-end pair is driven exclusively from Slate —
	// `SSlider::OnMouseCaptureLost` and `SSlider::ResetControllerState` — neither of which a
	// programmatic set can reach. ⇒ WITHOUT THE BROADCAST BELOW, Right moves the handle AND the
	// label while the engine setting never changes, and the next `SeedAndBind` silently reverts it:
	// the same "implemented-looking and useless" failure `SetCheckBoxChecked` exists to prevent,
	// one door along, and WORSE, because the label lies too.
	//
	// ⛔ ONE EDGE, NOT BOTH: the same handler is on both, so firing both writes twice.
	// `OnControllerCaptureEnd` is the semantically honest one for a keyboard / gamepad step — it
	// is the edge Slate itself uses to end a PAD interaction — and what we fire is byte-for-byte
	// what the engine's own handler fires (`USlider::HandleOnControllerCaptureEnd` IS
	// `OnControllerCaptureEnd.Broadcast();`, `Slider.cpp`). Same doctrine as `OnClicked.Broadcast()`
	// on Accept and `OnCheckStateChanged.Broadcast()` on a box: fire the delegate the HUMAN input
	// path fires, never a bespoke one.
	//
	// ⚠️ CADENCE, DECLARED RATHER THAN HOPED: the three new actions bind `ETriggerEvent::Started`,
	// so a held key is ONE press, ONE step and ONE commit — never a per-frame stream. The write
	// cadence here is one facade write per deliberate key press, which is what cl. (3a) protects
	// (it forbids a write per FRAME of one continuous gesture, not a write per discrete act).
	// ═════════════════════════════════════════════════════════════════════════════════════════
	if (ReadBack == OldValue)
	{
		// ⛔ NO COMMIT EDGE ON A NO-OP — CONVENTIONS "Delegates (C++)", the same rule
		// `SetCheckBoxChecked` follows three functions down. The ordinary case is a press against
		// the end of the range. ⚠️ Compared EXACTLY rather than with a tolerance on purpose: this
		// is the same test the engine already used to decide whether `OnValueChanged` fired at all
		// (`Slider.cpp`: `if (Value != InValue)`), so the change edge and the commit edge stay in
		// one-to-one correspondence instead of drifting apart at some epsilon.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): slider '%s' did not move (%.4f, at a range bound) — no commit edge fired."),
			Direction, *Slider->GetName(), ReadBack);
		return;
	}

	// ⛔ A DEGRADE, NOT AN ASSUMPTION. A future screen could bind only the MOUSE edge, or be
	// genuinely one-phase (its `OnValueChanged` handler IS the write, which `SetValue` already
	// ran). Asking which edges are bound turns all three cases into named, logged outcomes
	// instead of one silent guess.
	const bool bControllerEdgeBound = Slider->OnControllerCaptureEnd.IsBound();
	const bool bMouseEdgeBound      = Slider->OnMouseCaptureEnd.IsBound();
	if (!bControllerEdgeBound && !bMouseEdgeBound)
	{
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): slider '%s' moved to %.4f; no capture-end handler is bound, so the owning screen is one-phase (its OnValueChanged handler already ran) or binds nothing — no commit edge to fire."),
			Direction, *Slider->GetName(), ReadBack);
		return;
	}

	const TCHAR* const EdgeName = bControllerEdgeBound ? TEXT("OnControllerCaptureEnd") : TEXT("OnMouseCaptureEnd");

	// ⛔ LOGGED BEFORE THE BROADCAST, not after: the handler on the other end runs the facade
	// write, the apply and the ini save, so if it ever asserts or never returns, the line printed
	// first is the only evidence that the commit was attempted at all.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): slider '%s' -> %s.Broadcast() (the commit edge the owning screen writes on; a programmatic SetValue does not reach it)."),
		Direction, *Slider->GetName(), EdgeName);

	if (bControllerEdgeBound)
	{
		Slider->OnControllerCaptureEnd.Broadcast();
	}
	else
	{
		Slider->OnMouseCaptureEnd.Broadcast();
	}

	// ⚠️ READ AGAIN AFTER the commit, and the second number is not redundant: a committed write
	// re-seeds the panel (`RefreshAllRows`), which can legitimately MOVE this handle — the overall
	// preset slider also rewrites the resolution-scale row. A verifier reading the node after the
	// press must be able to tell "the refresh corrected it" from "nothing happened".
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] StepFocusedStop(%+d): %s returned for slider '%s'; value after the commit is %.4f (was %.4f before it)."),
		Direction, EdgeName, *Slider->GetName(), Slider->GetValue(), ReadBack);
}

bool USiegeMenuInputSubsystem::SetCheckBoxChecked(UCheckBox* CheckBox, bool bNewChecked, const FString& Context) const
{
	if (!CheckBox)
	{
		return false;
	}

	// ⚠️ COMPARED AS A STATE, NOT AS A BOOL. `IsChecked()` folds `Undetermined` into false, so a
	// bool compare would read "already off" on an undetermined box and leave a tri-state control
	// stuck where an arrow key should resolve it.
	const ECheckBoxState OldState = CheckBox->GetCheckedState();
	const ECheckBoxState NewState = bNewChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	if (OldState == NewState)
	{
		// ⛔ NO BROADCAST ON A NO-OP — CONVENTIONS "Delegates (C++)": "Broadcast on every ACTUAL
		// value change ... never on refused/ignored mutations". Still logged, so a player holding
		// Right on an already-on box produces evidence rather than silence.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] %s: check box '%s' is already %s — no change, no broadcast."),
			*Context, *CheckBox->GetName(), SiegeMenuCheckBox::StateWord(OldState));
		return false;
	}

	CheckBox->SetIsChecked(bNewChecked);

	// 🚨 THE BROADCAST IS REQUIRED HERE AND FORBIDDEN ON THE SLIDER, MEASURED BOTH WAYS:
	// `UCheckBox::SetIsChecked` updates `CheckedState` and the Slate widget and then fires only
	// `BroadcastFieldValueChanged` / `BroadcastEnumPostStateChange` — it NEVER touches
	// `OnCheckStateChanged` (`CheckBox.cpp`). Without this line the box would visibly flip and
	// the setting behind it would never be applied: implemented-looking and useless, which is the
	// failure this epic exists to stop. What we fire is exactly what a mouse click fires —
	// `UCheckBox::SlateOnCheckStateChangedCallback` ends in `OnCheckStateChanged.Broadcast(...)`,
	// the same relationship `UButton::SlateHandleClicked` has with the Accept path above.
	CheckBox->OnCheckStateChanged.Broadcast(bNewChecked);

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] %s: check box '%s' %s -> %s; OnCheckStateChanged broadcast (the delegate a mouse click fires)."),
		*Context, *CheckBox->GetName(),
		SiegeMenuCheckBox::StateWord(OldState), SiegeMenuCheckBox::StateWord(NewState));
	return true;
}

bool USiegeMenuInputSubsystem::FindStepperPair(const UButton* Focused, UButton*& OutPrev, UButton*& OutNext)
{
	OutPrev = nullptr;
	OutNext = nullptr;
	if (!Focused)
	{
		return false;
	}

	UPanelWidget* Parent = Focused->GetParent();
	if (!Parent)
	{
		return false;
	}

	// ⛔ EXACTLY TWO BUTTONS IN THE IMMEDIATE PARENT, AND THIS GUARD IS A SAFETY PROPERTY, NOT
	// TIDINESS. Left/Right PRESS a button, and a press is irreversible from this subsystem's
	// point of view. A structure-only rule ("the other button in my row") would make an arrow key
	// press `Apply` or `Back` in any footer that happens to hold two buttons. The count guard
	// plus the two named discriminators below mean a panel has to look like a stepper in BOTH
	// shape and authoring before an arrow key can fire anything in it.
	TArray<UButton*> Siblings;
	for (int32 Index = 0; Index < Parent->GetChildrenCount(); ++Index)
	{
		if (UButton* Sibling = Cast<UButton>(Parent->GetChildAt(Index)))
		{
			Siblings.Add(Sibling);
		}
	}
	if (Siblings.Num() != 2)
	{
		return false;
	}

	UButton* First = Siblings[0];
	UButton* Second = Siblings[1];

	// ─── Discriminator 1: the AUTHORED NAMES, with a matching base ─────────────────────────────
	// `<Base>PrevButton` / `<Base>NextButton`, the literal shape
	// `USiegeGraphicsMenuWidget::BuildStepperRow` constructs. The bases must be EQUAL, so two
	// unrelated buttons that happen to end in those words are still refused.
	auto SplitSuffix = [](const FString& Name, const TCHAR* Suffix, FString& OutBase) -> bool
	{
		if (!Name.EndsWith(Suffix, ESearchCase::CaseSensitive))
		{
			return false;
		}
		OutBase = Name.LeftChop(FCString::Strlen(Suffix));
		return true;
	};

	const FString FirstName = First->GetName();
	const FString SecondName = Second->GetName();
	FString FirstBase;
	FString SecondBase;

	if (SplitSuffix(FirstName, SiegeMenuStepper::PrevNameSuffix, FirstBase) &&
		SplitSuffix(SecondName, SiegeMenuStepper::NextNameSuffix, SecondBase) &&
		FirstBase == SecondBase)
	{
		OutPrev = First;
		OutNext = Second;
		return true;
	}

	FirstBase.Reset();
	SecondBase.Reset();
	if (SplitSuffix(FirstName, SiegeMenuStepper::NextNameSuffix, FirstBase) &&
		SplitSuffix(SecondName, SiegeMenuStepper::PrevNameSuffix, SecondBase) &&
		FirstBase == SecondBase)
	{
		// Slot order reversed by the author — honour the NAMES, not the layout.
		OutPrev = Second;
		OutNext = First;
		return true;
	}

	// ─── Discriminator 2: the AUTHORED GLYPHS ──────────────────────────────────────────────────
	// `BuildStepButton` gives each stepper a single `UTextBlock` reading "<" or ">", which is
	// precisely what `GetButtonLabel` returns. Trimmed, because a label authored with padding
	// spaces is the same glyph.
	const FString FirstGlyph = GetButtonLabel(First).TrimStartAndEnd();
	const FString SecondGlyph = GetButtonLabel(Second).TrimStartAndEnd();
	if (FirstGlyph == SiegeMenuStepper::PrevGlyph && SecondGlyph == SiegeMenuStepper::NextGlyph)
	{
		OutPrev = First;
		OutNext = Second;
		return true;
	}
	if (FirstGlyph == SiegeMenuStepper::NextGlyph && SecondGlyph == SiegeMenuStepper::PrevGlyph)
	{
		OutPrev = Second;
		OutNext = First;
		return true;
	}

	return false;
}

void USiegeMenuInputSubsystem::HandleMenuBack()
{
	// Entry instrument, FIRST STATEMENT, unconditional — and on this handler it is the line that
	// matters most, because on the day this row lands EVERY press ends in the inert branch below.
	// Without this line "Back did nothing" and "Back never arrived" would be the same silence,
	// which is the confusion TASK-1454 exists to clear and cannot clear without evidence.
	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuBack -> HandleMenuBack() entered."));

	// ⛔ DELIBERATELY NOT GATED ON `IsNavTargetActionable()`, and this is the one place the three
	// handlers differ. That gate's purpose is to stop Up/Down/Accept DRIVING a tree the player is
	// not looking at. Back drives nothing: it either finds a close handler or logs. Running the
	// gate would buy no safety and would SUPPRESS the diagnostic line in the single most
	// important case — a sub-screen is open (so the menu reads as covered) and did not register.
	UUserWidget* Target = GetActiveNavTarget();
	if (!Target)
	{
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] IA_MenuBack declined: no active nav target."));
		return;
	}

	// ⭐ WHERE THE TARGET CAME FROM (`qa/TASK-1410.md` WARN-5) — the one bit the two lines below
	// could not previously supply. "Back reached `WBP_MainMenu_C_0`" is ambiguous on its own: it
	// reads the same whether the main menu REGISTERED itself or whether nothing registered at all
	// and this is the fallback firing while the player is looking at an unregistered sub-screen.
	// That second case is precisely what TASK-1454 has to find, and it is exactly the shape of
	// WARN-1's latent hazard, so the provenance rides on every line that names a target.
	// ⛔ Derived by COMPARISON rather than by re-deriving the target here: `GetActiveNavTarget()`
	// stays the single definition of "the active target", and this cannot drift away from it.
	const TCHAR* const TargetOrigin = (Target == GetRegisteredNavTarget())
		? TEXT("from the registration stack")
		: TEXT("the DEFAULT FALLBACK — nothing is registered, so this is the main menu and NOT necessarily the screen the player is looking at");

	// ─── (4): A TARGET THAT REGISTERED NO CLOSE HANDLER ⇒ BACK IS INERT, LOGGED ONCE ───────────
	// ⛔ NEVER GUESS A TEARDOWN. `RemoveFromParent()` from here would look like it worked on every
	// screen and would be WRONG on one this project already ships: `USiegeGraphicsMenuWidget`
	// disarms its confirm countdown and discards a staged video mode BEFORE removing itself, and
	// skipping that leaves the settings facade refusing every save for the rest of the session.
	if (!Target->Implements<USiegeMenuNavCloseTarget>())
	{
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] IA_MenuBack: active target '%s' (%s, %s) implements no ISiegeMenuNavCloseTarget — Back is INERT for it, and no teardown is guessed."),
			*Target->GetName(), *Target->GetClass()->GetName(), TargetOrigin);
		return;
	}

	UE_LOG(LogSiegeMenuInput, Log,
		TEXT("[USiegeMenuInputSubsystem] IA_MenuBack -> OnMenuNavBackRequested on '%s' (%s, %s)."),
		*Target->GetName(), *Target->GetClass()->GetName(), TargetOrigin);

	// ⛔ (3): THE SCREEN CLOSES ITSELF. This call runs the screen's own teardown — the C++ one-line
	// `_Implementation` forwarding to its `BackPressed()`, or a Blueprint's event graph — and the
	// screen is the thing that decides whether to unregister, what to discard, and whether to
	// close at all. The subsystem removes nothing and holds no expectation about what happens
	// next; the next read of `GetActiveNavTarget()` simply reflects whatever the screen did.
	ISiegeMenuNavCloseTarget::Execute_OnMenuNavBackRequested(Target);
}

void USiegeMenuInputSubsystem::ApplyInitialFocus()
{
	if (!IsMenuUncovered())
	{
		return;
	}

	TArray<UButton*> Buttons;
	GetMenuButtons(Buttons);
	if (Buttons.Num() > 0 && !GetFocusedMenuButton())
	{
		// ─── TASK-1400 deliverable (4) ──────────────────────────────────────────────────────
		// ⛔ UNCONDITIONAL AT ITS POINT: inside the branch that places focus, before the call, so
		// it reports the button this function CHOSE. It is never gated on `FocusButton`'s return
		// -- TASK-1446 measured that `FSlateApplication::SetUserFocus` early-returns false when the
		// target is ALREADY focused (`SlateApplication.cpp:3028-3033`), so a false there is not a
		// failure and must never be logged as one. Same category and verbosity as the Accept line
		// below, which TASK-1399 §3 proved live in 🧑 his own process.
		//
		// ⭐ WHY THE MENU INSTANCE NAME IS IN THE LINE, and it is the whole point of logging here:
		// this row's central claim is that the return paths build a BRAND-NEW `WBP_MainMenu`. A
		// boot placement therefore names one instance and a placement after an Exit names a
		// DIFFERENT one. ⇒ 🧑 his own sitting discriminates "re-armed on the fresh widget" from
		// "never re-armed" with no extra state and no instrumentation on his side -- two lines
		// naming two instances IS the measurement.
		//
		// ⚠️ A REPEAT IS SIGNAL, NOT SPAM: the guard above means a successful placement silences
		// the next poll. If this line repeats at the poll rate, the focus request is NOT taking --
		// which is exactly what a reader needs to see, and could not see before.
		//
		// ⚠️ SHIPPING: `Log` verbosity is compiled out entirely under Shipping
		// (`USE_LOGGING_IN_SHIPPING` = 0 => `NO_LOGGING` = 1; no Target.cs override in this
		// project -- documented at `SiegeAssistantGrammar.cpp:226-270`). In a packaged build this
		// line DOES NOT EXIST. The precedent Accept line shares the property; it is a precondition
		// on the READER, never a reason to change the verbosity.
		UE_LOG(LogSiegeMenuInput, Log,
			TEXT("[USiegeMenuInputSubsystem] ApplyInitialFocus: focus placed on the TOP option '%s' (\"%s\"), index 0 of %d, in menu instance '%s'."),
			*Buttons[0]->GetName(), *GetButtonLabel(Buttons[0]), Buttons.Num(), *GetNameSafe(FindMainMenuWidget()));

		FocusButton(Buttons[0]);
	}
}

bool USiegeMenuInputSubsystem::FocusButton(UButton* Button) const
{
	// TASK-1406: one level of indirection, ⛔ ZERO behaviour change. The body moved verbatim into
	// `FocusWidget` (the parameter type is the only edit), so TASK-1400's `ApplyInitialFocus` --
	// which this row does not touch at all -- sees exactly what it saw before.
	return FocusWidget(Button);
}

bool USiegeMenuInputSubsystem::FocusWidget(UWidget* Widget) const
{
	APlayerController* PC = GetLocalController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Widget || !LocalPlayer || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	// ⭐ `SupportsKeyboardFocus()` IS THE BACKSTOP THAT MAKES THE WIDENING SAFE: it is asked of the
	// underlying SWidget, so a stop whose Slate side cannot take keyboard focus is refused here
	// even if the UMG-side flags said yes. `SEditableTextBox`, `SCheckBox`, `SSlider` and `SButton`
	// all answer true when focusable, which is why these four classes are the admitted set.
	TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
	if (!SlateWidget.IsValid() || !SlateWidget->SupportsKeyboardFocus())
	{
		return false;
	}

	// EFocusCause::Navigation, NOT UWidget::SetUserFocus's SetDirectly: FSlateApplication::
	// SetUserFocus sets ShowFocus = (InCause == Navigation) (SlateApplication.cpp:3099), and
	// that flag is what makes SWidget::Paint draw the "FocusRectangle" brush (SWidget.cpp:1748)
	// — the visible focus state of deliverable (3). Same user-index resolution as
	// UWidget::SetUserFocus (Widget.cpp), same fallback to the local player's deferred
	// Slate operations if the immediate set is refused this frame.
	const int32 UserIndex = FSlateApplication::Get().GetUserIndexForController(LocalPlayer->GetControllerId());
	FReply& DelayedSlateOperations = LocalPlayer->GetSlateOperations();
	if (FSlateApplication::Get().SetUserFocus(UserIndex, SlateWidget, EFocusCause::Navigation))
	{
		DelayedSlateOperations.CancelFocusRequest();
		return true;
	}

	DelayedSlateOperations.SetUserFocus(SlateWidget.ToSharedRef(), EFocusCause::Navigation);
	return false;
}
