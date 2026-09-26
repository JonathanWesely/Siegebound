// Copyright Epic Games, Inc. All Rights Reserved.

#include "SessionMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"  // TASK-680: CreateWidget's owner static_assert needs the complete type
#include "UObject/UObjectGlobals.h"          // TASK-680: LoadClass<> (explicit IWYU - no compile verifies transitive pulls)
// TASK-1425 [MENU-NAV-SESSION]: complete type for RegisterMenuNavTarget /
// UnregisterMenuNavTarget. ⛔ This screen CALLS that public API and never edits
// the subsystem - the walker, the ring and the key bindings all live there.
#include "SiegeMenuInputSubsystem.h"
#include "SiegeSessionSubsystem.h"

void USessionMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Auto-wire the OPTIONAL named buttons once per widget instance. A WBP
	// that skips these names wires its own buttons to the three BlueprintCallable
	// wrappers instead - both routes are supported (class comment contract).
	if (HostButton)
	{
		HostButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleHostClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleJoinClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USessionMenuWidget::HandleBackClicked);
	}
}

void USessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind the subsystem text surfaces for the on-screen lifetime of the menu.
	// AddUniqueDynamic keeps repeated construct cycles single-bound.
	if (USiegeSessionSubsystem* Session = ResolveSessionSubsystem())
	{
		Session->OnSessionStatus.AddUniqueDynamic(this, &USessionMenuWidget::HandleSessionStatus);
		Session->OnSessionError.AddUniqueDynamic(this, &USessionMenuWidget::HandleSessionError);
	}
	else
	{
		// No crash, no dead button mystery: the menu says why it cannot work.
		ShowLocalError(TEXT("Session system unavailable."));
	}

	// ------------------------------------------------------------------------
	// TASK-1425 [MENU-NAV-SESSION] - and it is the LAST statement of
	// NativeConstruct ON PURPOSE, obeying the law TASK-1417 measured: REGISTER
	// AFTER the seed/bind call, never before it (registering against the
	// pre-seed tree logged 24 stops for a screen that had 2).
	//
	// ⭐ WHAT THAT LAW BUYS HERE IS SMALLER THAN ON SETTINGS OR GRAPHICS, AND
	// SAYING SO IS THE POINT: this screen's tree is ASSET-AUTHORED, so all
	// twelve widgets already exist and carry their authored enabled/visible
	// state before NativeConstruct runs, and the bind block above mutates NO
	// stop - ShowLocalError only pushes text into ErrorTextBlock (a UTextBlock,
	// not an admitted class) and fires a BlueprintImplementableEvent that
	// WBP_SessionMenu, a ZERO-GRAPH BP, does not implement. ⇒ the four stops are
	// four either side of the bind. The ordering is kept anyway because it costs
	// nothing and because the day someone adds a SetIsEnabled to the block above
	// is the day it starts mattering, silently.
	//
	// ⚠️ THE VIEWPORT ORDERING THIS DEPENDS ON, INHERITED FROM TASK-1415's
	// READING OF THE 5.8 SOURCE RATHER THAN RE-ASSERTED: AddToScreen sets
	// bIsManagedByGameViewportSubsystem and the slot BEFORE calling
	// TakeWidget(), and TakeWidget is what runs RebuildWidget -> OnWidgetRebuilt
	// -> NativeConstruct. So IsInViewport() and IsVisible() are ALREADY true
	// here, which is what GetRegisteredNavTarget() re-validates on every read.
	// ⇒ THE FALSIFIER IS ONE LINE OF LOG: a "registered" retarget naming
	// anything but this widget, or reporting 7 stops (the main menu's count),
	// means that ordering changed.
	//
	// ⛔ NOTHING ELSE IS ADDED HERE. No focus call of our own, no key handler,
	// no input-mode call - L_MainMenu's UIOnly + cursor posture is owned by
	// BP_MenuGameMode at level boot and survives viewport widget swaps (the
	// Input-mode ownership law already recorded at BackPressed below).
	// ------------------------------------------------------------------------
	RegisterAsMenuNavTarget();
}

void USessionMenuWidget::NativeDestruct()
{
	// TASK-1425: LIFO against NativeConstruct - navigation goes back BEFORE the
	// delegates come down, because registration was the last thing taken.
	// This is the CATCH-ALL half of the pair: BackPressed unregisters on the
	// ordinary standalone exit, and this one covers every other way the panel
	// can stop existing - the LeaveMatch level travel, viewport teardown, or a
	// caller that removes this widget without going through BackPressed. Both
	// firing is the normal case and is safe; see UnregisterAsMenuNavTarget.
	UnregisterAsMenuNavTarget();

	// Symmetric unbind (defensive - dynamic delegates tolerate dead objects,
	// but a destructed menu must not keep receiving session text).
	if (USiegeSessionSubsystem* Session = ResolveSessionSubsystem())
	{
		Session->OnSessionStatus.RemoveDynamic(this, &USessionMenuWidget::HandleSessionStatus);
		Session->OnSessionError.RemoveDynamic(this, &USessionMenuWidget::HandleSessionError);
	}

	Super::NativeDestruct();
}

void USessionMenuWidget::HostPressed()
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Host pressed."));
	Session->HostListenMatch();
}

void USessionMenuWidget::JoinPressed(const FString& AddressText)
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Join pressed (address text '%s')."), *AddressText);
	// Validation is the subsystem's job: an invalid address returns through
	// OnSessionError -> HandleSessionError -> OnSessionErrorShown, no travel.
	Session->JoinMatch(AddressText);
}

void USessionMenuWidget::BackPressed()
{
	USiegeSessionSubsystem* Session = ResolveSessionSubsystem();
	if (!Session)
	{
		ShowLocalError(TEXT("Session system unavailable."));
		return;
	}

	const UWorld* World = GetWorld();
	const bool bNetActive = World && (World->GetNetMode() != NM_Standalone);

	// A join whose connection is still PENDING keeps the menu world standalone
	// - detect it via the world context so Back can cancel it (doc section 5:
	// leave is the universal bail-out; gate (f) never-hang).
	bool bPendingConnection = false;
	if (GEngine && World)
	{
		if (const FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(World))
		{
			bPendingConnection = (WorldContext->PendingNetGame != nullptr);
		}
	}

	if (bNetActive || bPendingConnection)
	{
		UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - leaving session (NetActive=%d, PendingConnection=%d)."),
			bNetActive ? 1 : 0, bPendingConnection ? 1 : 0);
		Session->LeaveMatch();
		return;
	}

	// Plain standalone menu: Back dismisses the panel HERE, C++-side.
	// SUPERSEDED RATIONALE (2026-08-28, the SESSION-BACK ruling in
	// CONVENTIONS, off VID-002): the TASK-354 flagged decision read "Back is
	// WBP-side panel navigation", but the TASK-355 route-(A) zero-graph WBP
	// never authored that navigation - measured on pixels, three full press
	// cycles rendered and nobody closed the panel. The decision is REVERSED.
	// The half of the old rationale that SURVIVES is the LeaveMatch refusal:
	// reloading L_MainMenu on every standalone Back press would flicker-reset
	// the menu for no reason, so dismissal is a viewport widget swap, never
	// travel.
	//
	// The swap is the exact INVERSE of the TASK-355 open transition (the
	// main-menu Multiplayer entry: RemoveFromParent(self) ->
	// CreateWidget(WBP_SessionMenu_C) -> AddToViewport - the main menu is
	// fully REMOVED from the viewport, not hidden, so Back must re-create
	// it). Add-before-remove is LAW: the main menu enters the viewport BEFORE
	// this panel leaves it, so no frame renders with neither widget; a failed
	// class resolve leaves THIS panel up (ShowLocalError - one log, one error
	// line), never a zero-UI strand. Deliberately NO SetInputMode on any
	// path: L_MainMenu's posture (UIOnly + visible cursor) is owned by
	// BP_MenuGameMode at level boot (Input-mode ownership law) and survives
	// viewport widget swaps - the open transition made no input-mode call
	// either, and VID-002 shows the swapped-in panel fully hover- and
	// click-interactive under the surviving posture.
	UClass* MainMenuClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C"));
	APlayerController* OwningPlayer = GetOwningPlayer();
	UUserWidget* MainMenu = (MainMenuClass && OwningPlayer)
		? CreateWidget<UUserWidget>(OwningPlayer, MainMenuClass)
		: nullptr;
	if (!MainMenu)
	{
		// Panel STAYS up - the player keeps a live, clickable UI.
		ShowLocalError(TEXT("Main menu unavailable."));
		return;
	}

	UE_LOG(LogSiegeNet, Log, TEXT("[SessionMenu] Back pressed - no session active; returning to main menu."));
	MainMenu->AddToViewport();

	// ------------------------------------------------------------------------
	// TASK-1425 [MENU-NAV-SESSION]: UNREGISTER BESIDE THE EXISTING TEARDOWN,
	// NEVER INSTEAD OF IT - and on THIS screen the placement is load-bearing in
	// a way it was not on Settings or Graphics, because the parent is being
	// destroyed too. What the nav stack holds at each step:
	//
	//   before BackPressed .......... [SessionMenu]        (registered at
	//                                                       NativeConstruct)
	//   after the !MainMenu early
	//   return above ................ [SessionMenu]        - UNCHANGED, and
	//     this is WHY the call sits below that return: a failed LoadClass /
	//     CreateWidget leaves THIS panel up and clickable, so it must KEEP the
	//     ring. Unregistering above the guard would blind a panel that is still
	//     the only UI on screen.
	//   after MainMenu->AddToViewport() ... [SessionMenu]   - both widgets are
	//     in the viewport for these three statements. No tick runs between
	//     them, so nothing observes the overlap and the order of this call
	//     against AddToViewport is not itself load-bearing.
	//   after this line ............. []                    - EMPTY. The stack
	//     drops to zero, so UnregisterMenuNavTarget's own
	//     `if (GetRegisteredNavTarget() != nullptr) FocusFirstNavStop();` does
	//     NOT fire. ⛔ THAT IS THE DESIGN, NOT A GAP: a focus call from here
	//     could land on the main menu while this panel is still drawn.
	//   after RemoveFromParent() .... []                    - and NativeDestruct
	//     calls Unregister a second time, which the subsystem answers with one
	//     Log line ("not registered ... no change") and no state change.
	//
	// ⭐ AND THE RETURN LEG NEEDS NOTHING FROM THIS FILE - THE MECHANISM IS
	// TASK-1400's AND IT IS CITED RATHER THAN DUPLICATED (spec (2): "IF
	// TASK-1400's re-arm already covers this, SAY SO AND ADD NOTHING - two
	// mechanisms racing for the same ring is worse than one").
	//   • The re-arm is a 0.2 s LOOPING world timer armed in OnWorldBeginPlay
	//     after the L_MainMenu map gate (SiegeMenuInputSubsystem.cpp:205-234),
	//     firing ApplyInitialFocus.
	//   • It is POINTER-FREE BY CONSTRUCTION: IsMenuUncovered(),
	//     GetMenuButtons() and GetFocusedMenuButton() each re-resolve through
	//     GetAllWidgetsOfClass(..., TopLevelOnly), which filters on
	//     IsInViewport(), matched on the CLASS PATH STRING
	//     "/Game/UI/WBP_MainMenu.WBP_MainMenu_C". The class this function loads
	//     one screen above is that exact string. ⇒ the instance THIS FUNCTION
	//     JUST CREATED is the one the poll finds; the instance this screen was
	//     opened from is already gone and drops out of the same set.
	//   • It cannot fire early and steal the ring while this panel is open:
	//     with WBP_MainMenu removed from the viewport by the open transition,
	//     IsMenuUncovered() sees this panel as a visible top-level widget that
	//     is not the menu class and returns false on the spot.
	//   • It cannot fire between AddToViewport and RemoveFromParent: those are
	//     consecutive statements in one call, and a timer needs a tick.
	//   • Within <= 0.2 s of this function returning, the poll finds the fresh
	//     menu uncovered with nothing focused and focuses Buttons[0] - the TOP
	//     option, which is the runtime criterion - logging the placement WITH
	//     THE INSTANCE NAME, so the log discriminates "re-armed on the fresh
	//     widget" from "never re-armed" with no extra instrumentation.
	// ⇒ NO SECOND RE-ARM IS ADDED HERE. This file contributes exactly one thing
	// to the return leg: it stops claiming the ring.
	// ------------------------------------------------------------------------
	UnregisterAsMenuNavTarget();

	RemoveFromParent();
}

void USessionMenuWidget::HandleHostClicked()
{
	HostPressed();
}

void USessionMenuWidget::HandleJoinClicked()
{
	const FString Address = AddressTextBox ? AddressTextBox->GetText().ToString() : FString();
	JoinPressed(Address);
}

void USessionMenuWidget::HandleBackClicked()
{
	BackPressed();
}

void USessionMenuWidget::HandleSessionStatus(const FString& Message)
{
	if (StatusTextBlock)
	{
		StatusTextBlock->SetText(FText::FromString(Message));
	}
	if (ErrorTextBlock)
	{
		// A fresh status supersedes a stale error line (e.g. a corrected IP
		// after a rejected one) - only the bound widget is cleared; BIE
		// consumers decide their own presentation.
		ErrorTextBlock->SetText(FText::GetEmpty());
	}
	OnSessionStatusUpdated(Message);
}

void USessionMenuWidget::HandleSessionError(const FString& Message)
{
	if (ErrorTextBlock)
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
	}
	OnSessionErrorShown(Message);
}

// ═══════════════════════════════════════════════════════════════════════════
//  TASK-1425 [MENU-NAV-SESSION] — THE HANDSHAKE
//
//  Everything that ENUMERATES stops, reads IsFocusable, places the ring and
//  logs the count lives in USiegeMenuInputSubsystem (TASK-1406); this file's
//  entire contribution is saying WHEN this screen is the one the player is
//  looking at, and — on the one screen in the epic that REMOVES the main menu
//  instead of covering it — when it stops being.
//
//  ⛔ NETWORKING IS UNTOUCHED BY THIS ROW. HostPressed / JoinPressed /
//  ParseJoinAddress / LeaveMatch / USiegeSessionSubsystem / the pending-
//  connection probe and the bNetActive branch above are byte-identical; the
//  only statement added inside BackPressed is one UnregisterAsMenuNavTarget()
//  below the network branch's own `return`. Nothing this row writes can reach
//  a session, host, join or travel path (M8).
// ═══════════════════════════════════════════════════════════════════════════

void USessionMenuWidget::RegisterAsMenuNavTarget()
{
	USiegeMenuInputSubsystem* MenuInput = ResolveMenuInputSubsystem();
	if (MenuInput == nullptr)
	{
		// Log, not Warning: the honest reading of a null here is "this world has
		// no menu input" (an Editor/designer world, or a cooked path where the
		// subsystem declined), and a panel that warns every time it is previewed
		// is a panel whose log nobody reads. The screen still works with the
		// mouse exactly as it did before this row.
		UE_LOG(LogSiegeNet, Log,
			TEXT("[SessionMenu] No USiegeMenuInputSubsystem on this world - the multiplayer panel is mouse-only (keyboard navigation is unavailable, not broken)."));
		return;
	}

	// `this`, never a child and never a class default - the API takes the SCREEN
	// and walks its own WidgetTree from there.
	MenuInput->RegisterMenuNavTarget(this);
}

void USessionMenuWidget::UnregisterAsMenuNavTarget()
{
	// ⛔ SILENT ON A NULL SUBSYSTEM, unlike Register. If there was no subsystem
	// to register with there is nothing to give back, and the places this is
	// reached with a half-torn-down world are NativeDestruct and the LeaveMatch
	// travel - where a second log line would say nothing a reader could act on.
	if (USiegeMenuInputSubsystem* MenuInput = ResolveMenuInputSubsystem())
	{
		// Idempotent by the subsystem's own contract: it removes by IDENTITY and
		// logs (does not warn) when the screen was not on the stack, precisely so
		// the BackPressed + NativeDestruct pairing is safe to run twice.
		MenuInput->UnregisterMenuNavTarget(this);
	}
}

USiegeMenuInputSubsystem* USessionMenuWidget::ResolveMenuInputSubsystem() const
{
	// Null-safe at every hop - the ResolveSessionSubsystem shape below, cloned,
	// with ONE deliberate difference: this subsystem is a UWorldSubsystem, so it
	// is reached through the WORLD and not through the game instance. It also
	// declines Editor worlds outright (DoesSupportWorldType = Game | PIE only),
	// which is why a null answer is ordinary rather than an error.
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<USiegeMenuInputSubsystem>() : nullptr;
}

USiegeSessionSubsystem* USessionMenuWidget::ResolveSessionSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeSessionSubsystem>() : nullptr;
}

void USessionMenuWidget::ShowLocalError(const FString& Message)
{
	UE_LOG(LogSiegeNet, Warning, TEXT("[SessionMenu] %s"), *Message);
	if (ErrorTextBlock)
	{
		ErrorTextBlock->SetText(FText::FromString(Message));
	}
	OnSessionErrorShown(Message);
}
