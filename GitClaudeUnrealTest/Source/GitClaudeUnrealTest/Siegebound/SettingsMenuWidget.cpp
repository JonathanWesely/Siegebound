// Copyright Epic Games, Inc. All Rights Reserved.

#include "SettingsMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
// TASK-1115 [GFX-PANEL]: complete type for CreateAndAddToViewport. This is the
// ONE new dependency this row adds to the settings screen, and it points at C++
// rather than at a .uasset because /Game/UI/WBP_GraphicsMenu is RESERVED and
// UNAUTHORED (GFX-§2).
#include "SiegeGraphicsMenuWidget.h"
#include "SiegeSettingsSubsystem.h"

namespace SiegeSettingsMenuText
{
	// ------------------------------------------------------------------------
	// ALL PLAYER-FACING TEXT ON THIS SCREEN IS GAME-AUTHORED AND LIVES HERE.
	// The label and the hint are quoted character-for-character from the
	// TASK-437 spec item (3).
	// ------------------------------------------------------------------------

	static const TCHAR* Title = TEXT("Settings");

	static const TCHAR* ConfirmLabel = TEXT("Confirm AI orders before they execute");

	/**
	 *  This hint is the MEASURED TRUTH from TASKBOARD SETTINGS+CONFIRM ruling 2,
	 *  stated plainly to the player. Four of the five stable eval failures were
	 *  wrong-place or wrong-count on orders the model otherwise understood, and
	 *  one invented a unit the player does not own.
	 *  ==> DO NOT SOFTEN THIS INTO MARKETING COPY. The sentence is doing work.
	 */
	static const TCHAR* ConfirmHint =
		TEXT("Shows the parsed order and its target circles for review. Recommended — the assistant can pick the wrong place or the wrong number.");

	/**
	 *  Shown INSTEAD of the hint when the settings subsystem cannot be resolved.
	 *  It is worded to match the actual fail-safe: TASK-443 spec item (4) says an
	 *  unresolvable subsystem makes the assistant behave as if confirm is ON,
	 *  because failing safe means MORE review, never less. So the row is shown
	 *  disabled AND CHECKED, and this line says why that is honest rather than
	 *  decorative.
	 */
	static const TCHAR* ConfirmUnavailable =
		TEXT("Settings are unavailable right now, so AI orders will always be shown for review.");

	static const TCHAR* Back = TEXT("Back");

	/**
	 *  TASK-1115 [GFX-PANEL]. GFX-§10 pins the widget NAMES; the label text is
	 *  this row's, and it is 🧑 Jonathan's own word for the submenu, verbatim:
	 *  "Make all of these graphics sliders a sub menu within 'Settings' called
	 *  'Graphics'".
	 */
	static const TCHAR* Graphics = TEXT("Graphics");
}

// The CONVENTIONS section 2 / section 8 pinned FIELD name. See the header for
// why this is not the subsystem's broadcast token.
const TCHAR* USettingsMenuWidget::ConfirmSettingName = TEXT("bAssistantConfirmBeforeExecute");

TSharedRef<SWidget> USettingsMenuWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING. UUserWidget::RebuildWidget() reads
	// WidgetTree->RootWidget AS IT STANDS at the moment it is called and returns
	// an SSpacer when it is null (engine source: UserWidget.cpp, UE 5.8). So a
	// code-authored tree MUST be constructed BEFORE Super::RebuildWidget();
	// building it afterwards and returning Super's result yields a silently
	// EMPTY widget that still passes every property readback.
	//
	// That is not a guess - it is the headline finding TASK-411's probe
	// rehearsal recorded for exactly this ruling, and it is precisely the class
	// of defect this project's "UMG verification is a pixel or human check"
	// law exists for.
	//
	// The task spec phrases the escape hatch as "construct each child only if
	// that member is still null AFTER Super::RebuildWidget()". The SEMANTIC
	// half of that - only ever construct a child that is still null - is
	// honoured exactly, below. The literal ORDERING half is not followable:
	// the BindWidgetOptional members are resolved in UUserWidget::Initialize(),
	// which has already run by the time RebuildWidget() is entered, so "still
	// null" is fully determined here; and deferring construction past Super
	// would produce the empty-widget defect above. Flagged for QA in the
	// handoff rather than resolved silently.
	//
	// ⚠️ WARN-437-1 (qa/TASK-439.md §5). Initialize() FIRST, and it closes the
	// last route to a blank panel. WidgetTree is allocated INSIDE Initialize()
	// (UserWidget.cpp:159-162), and Super::RebuildWidget() self-heals an
	// un-initialised widget with `if (!bInitialized) { Initialize(); }`
	// (UserWidget.cpp:1197-1200) — i.e. AFTER our tree-building has already run
	// and bailed out on a null WidgetTree, leaving Super to find RootWidget null
	// and hand back an SSpacer. Calling it here is free and safe: Initialize() is
	// public (UserWidget.h:297) and idempotent — it no-ops unless
	// `!bInitialized && !HasAnyFlags(RF_ClassDefaultObject)` (UserWidget.cpp:135-137).
	// Hardening, not a live bug: CreateWidgetInstance already initialises before
	// anything can take the widget, so no shipped path reaches here uninitialised.
	Initialize();
	ConstructSettingsTree();
	return Super::RebuildWidget();
}

void USettingsMenuWidget::ConstructSettingsTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] No WidgetTree - the settings panel cannot build its tree."));
		return;
	}

	// ------------------------------------------------------------------------
	// CONDITION (b), THE ESCAPE HATCH. If an asset-authored tree exists (a
	// future /Game/UI/WBP_SettingsMenu), it wins WHOLE: UMG has already
	// resolved the BindWidgetOptional members from it, so there is nothing to
	// construct and nothing to overwrite. Taking that fallback costs ONE art
	// task and ZERO C++ change - which is what makes this ruling a ruling and
	// not a one-way door.
	// ------------------------------------------------------------------------
	if (WidgetTree->RootWidget != nullptr)
	{
		UE_LOG(LogSiegeSettings, Log,
			TEXT("[SettingsMenu] An asset-authored tree is present - the code-authored branch is skipped (CONVENTIONS section 3, condition (b))."));
		return;
	}

	// ---- BackdropBorder: the modal plate, and the tree root -----------------
	if (BackdropBorder == nullptr)
	{
		BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackdropBorder"));
	}

	if (BackdropBorder == nullptr)
	{
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] Could not construct BackdropBorder - the settings panel has no root."));
		return;
	}

	// ⛔ THIS LINE IS CORRECTNESS, NOT STYLING, AND IT IS THE OPPOSITE OF
	// WBP_SessionMenu's BACKDROP. The panel is added ON TOP of WBP_MainMenu
	// (CONVENTIONS section 4) without removing it. A HIT_TEST_INVISIBLE plate
	// would let clicks fall straight through to Play / Sandbox / Deck Builder /
	// QUIT while the panel looks modal. ESlateVisibility::Visible makes the
	// border hit-testable, so it absorbs every click that is not on a control.
	//
	// Note this does NOT depend on the brush drawing anything: Slate hit-tests
	// on VISIBILITY and geometry, not on whether pixels were painted. The
	// dimming below is appearance; the click blocking is this line.
	//
	// The owning UUserWidget stays at its UMG default of SelfHitTestInvisible -
	// that means "I do not hit-test, my children do", which is exactly right:
	// the border is the child doing the absorbing.
	BackdropBorder->SetVisibility(ESlateVisibility::Visible);

	// Appearance only - a dim plate so the menu underneath reads as inactive.
	// A future WBP_SettingsMenu overrides all of this for free.
	BackdropBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	BackdropBorder->SetPadding(FMargin(0.f));
	BackdropBorder->SetHorizontalAlignment(HAlign_Center);
	BackdropBorder->SetVerticalAlignment(VAlign_Center);

	WidgetTree->RootWidget = BackdropBorder;

	// ---- RootPanel: the column ---------------------------------------------
	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] Could not construct RootPanel - the settings panel has no content column."));
		return;
	}

	BackdropBorder->SetContent(RootPanel);

	// ---- TitleText ----------------------------------------------------------
	if (TitleText == nullptr)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		if (TitleText != nullptr)
		{
			TitleText->SetText(FText::FromString(FString(SiegeSettingsMenuText::Title)));
			TitleText->SetFontSize(36.f);

			if (UVerticalBoxSlot* TitleSlot = RootPanel->AddChildToVerticalBox(TitleText))
			{
				TitleSlot->SetPadding(FMargin(24.f, 24.f, 24.f, 16.f));
				TitleSlot->SetHorizontalAlignment(HAlign_Center);
				TitleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- The one setting row ------------------------------------------------
	// The label is the CHECK BOX'S CONTENT rather than a sibling. That keeps the
	// tree to the pinned members only (no unpinned layout container), and it
	// makes the words part of the click target, which is the behaviour players
	// expect from a labelled toggle. It also means disabling the check box greys
	// the label with it - the "row renders disabled" requirement, for free.
	if (ConfirmToggleLabelText == nullptr)
	{
		ConfirmToggleLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmToggleLabelText"));
		if (ConfirmToggleLabelText != nullptr)
		{
			ConfirmToggleLabelText->SetText(FText::FromString(FString(SiegeSettingsMenuText::ConfirmLabel)));
			ConfirmToggleLabelText->SetFontSize(24.f);
			ConfirmToggleLabelText->SetAutoWrapText(true);
		}
	}

	if (ConfirmToggleCheckBox == nullptr)
	{
		ConfirmToggleCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("ConfirmToggleCheckBox"));
		if (ConfirmToggleCheckBox != nullptr)
		{
			if (ConfirmToggleLabelText != nullptr)
			{
				ConfirmToggleCheckBox->SetContent(ConfirmToggleLabelText);
			}

			if (UVerticalBoxSlot* ToggleSlot = RootPanel->AddChildToVerticalBox(ConfirmToggleCheckBox))
			{
				ToggleSlot->SetPadding(FMargin(24.f, 8.f, 24.f, 4.f));
				ToggleSlot->SetHorizontalAlignment(HAlign_Left);
				ToggleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (ConfirmToggleCheckBox == nullptr)
	{
		// Not fatal - Back must still work, so the panel is never a trap.
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] Could not construct ConfirmToggleCheckBox - the confirm setting cannot be changed from this screen."));

		// Without the check box the label has no parent. Adopt it directly so it
		// is never an orphan the tree owns but nothing ever shows, and so the
		// hint below still reads as a labelled row rather than a stray sentence.
		if (ConfirmToggleLabelText != nullptr)
		{
			if (UVerticalBoxSlot* OrphanLabelSlot = RootPanel->AddChildToVerticalBox(ConfirmToggleLabelText))
			{
				OrphanLabelSlot->SetPadding(FMargin(24.f, 8.f, 24.f, 4.f));
				OrphanLabelSlot->SetHorizontalAlignment(HAlign_Left);
				OrphanLabelSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (ConfirmToggleHintText == nullptr)
	{
		ConfirmToggleHintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmToggleHintText"));
		if (ConfirmToggleHintText != nullptr)
		{
			ConfirmToggleHintText->SetText(FText::FromString(FString(SiegeSettingsMenuText::ConfirmHint)));
			ConfirmToggleHintText->SetFontSize(18.f);
			ConfirmToggleHintText->SetAutoWrapText(true);
			ConfirmToggleHintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f, 1.f)));

			if (UVerticalBoxSlot* HintSlot = RootPanel->AddChildToVerticalBox(ConfirmToggleHintText))
			{
				HintSlot->SetPadding(FMargin(56.f, 0.f, 24.f, 16.f));
				HintSlot->SetHorizontalAlignment(HAlign_Fill);
				HintSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- GraphicsButton (TASK-1115) -----------------------------------------
	// ⛔ board cl. (6): ABOVE BackButton, so Back stays last. Same button idiom as
	// Back below (font 28, MakeMargin(24,12,24,12), HAlign_Fill) — the two entries
	// have to read as one list, not as a control and an afterthought.
	//
	// 🚨 THIS IS THE WHOLE ENTRY POINT FOR THE GRAPHICS FEATURE, AND IT COSTS ZERO
	// .uasset WRITES. /Game/UI/WBP_MainMenu is never opened: the Settings screen is
	// already code-authored, so the submenu hangs off C++ that this project owns
	// (GFX-§1 / GFX-§2).
	if (GraphicsLabelText == nullptr)
	{
		GraphicsLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraphicsLabelText"));
		if (GraphicsLabelText != nullptr)
		{
			GraphicsLabelText->SetText(FText::FromString(FString(SiegeSettingsMenuText::Graphics)));
			GraphicsLabelText->SetFontSize(28.f);
		}
	}

	if (GraphicsButton == nullptr)
	{
		GraphicsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("GraphicsButton"));
		if (GraphicsButton != nullptr)
		{
			if (GraphicsLabelText != nullptr)
			{
				if (UButtonSlot* GraphicsContentSlot = Cast<UButtonSlot>(GraphicsButton->SetContent(GraphicsLabelText)))
				{
					GraphicsContentSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 12.f));
					GraphicsContentSlot->SetHorizontalAlignment(HAlign_Center);
					GraphicsContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* GraphicsSlot = RootPanel->AddChildToVerticalBox(GraphicsButton))
			{
				GraphicsSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 8.f));
				GraphicsSlot->SetHorizontalAlignment(HAlign_Fill);
				GraphicsSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (GraphicsButton == nullptr)
	{
		// Not fatal — Back must still work, so the panel is never a trap. The
		// player simply has no graphics screen, which is exactly the state the
		// game shipped in before this row.
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] Could not construct GraphicsButton - the Graphics submenu cannot be opened from this screen."));
	}

	// ---- BackButton ---------------------------------------------------------
	// Geometry lifted from the shipped WBP_MainMenu button idiom this panel sits
	// on top of (font 28, MakeMargin(24,12,24,12), HAlign_Fill) rather than
	// invented, so the two screens read as one family.
	if (BackLabelText == nullptr)
	{
		BackLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackLabelText"));
		if (BackLabelText != nullptr)
		{
			BackLabelText->SetText(FText::FromString(FString(SiegeSettingsMenuText::Back)));
			BackLabelText->SetFontSize(28.f);
		}
	}

	if (BackButton == nullptr)
	{
		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
		if (BackButton != nullptr)
		{
			if (BackLabelText != nullptr)
			{
				if (UButtonSlot* BackContentSlot = Cast<UButtonSlot>(BackButton->SetContent(BackLabelText)))
				{
					BackContentSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 12.f));
					BackContentSlot->SetHorizontalAlignment(HAlign_Center);
					BackContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* BackSlot = RootPanel->AddChildToVerticalBox(BackButton))
			{
				BackSlot->SetPadding(FMargin(24.f, 12.f, 24.f, 24.f));
				BackSlot->SetHorizontalAlignment(HAlign_Fill);
				BackSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (BackButton == nullptr)
	{
		// This one IS worth an Error: without Back the only way out of a modal
		// backdrop is to quit the game.
		UE_LOG(LogSiegeSettings, Error,
			TEXT("[SettingsMenu] Could not construct BackButton - the settings panel cannot be dismissed from itself."));
	}
}

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Everything is bound HERE and not in NativeOnInitialized, deliberately and
	// for one mechanical reason: the code-authored children do not exist until
	// RebuildWidget() runs, and the engine order is
	//   Initialize() -> NativeOnInitialized() -> RebuildWidget() -> NativeConstruct()
	// (UserWidget.cpp: NativeConstruct is called from OnWidgetRebuilt). Binding
	// in NativeOnInitialized would silently bind nothing on the code-authored
	// path while working fine on a future WBP path - a difference that would
	// only ever show up as a dead check box in a build nobody could reproduce.
	// One binding site covers BOTH paths.
	SeedAndBind();
}

void USettingsMenuWidget::NativeDestruct()
{
	UnbindAll();

	Super::NativeDestruct();
}

void USettingsMenuWidget::SeedAndBind()
{
	// ------------------------------------------------------------------------
	// SEED, **THEN** BIND - in that order, and the order is the point.
	// qa/TASK-005 major-2: a control that is only BOUND, and is created holding
	// whatever value it was constructed with, shows a stale value until
	// something happens to change it. The check box below is constructed
	// Unchecked by UMG; the shipped default of the setting is TRUE. Binding
	// without seeding would therefore show every player "off" on a setting that
	// is on.
	//
	// Seeding first also removes any question about SetIsChecked() re-entering
	// our own handler. (It does not - engine source: UCheckBox::SetIsChecked
	// updates CheckedState and the Slate widget but only
	// SlateOnCheckStateChangedCallback, i.e. real user interaction, broadcasts
	// OnCheckStateChanged. Belt and braces: the delegate is removed before the
	// seed and re-added after, so this is true even if that ever changes.)
	// ------------------------------------------------------------------------
	if (ConfirmToggleCheckBox != nullptr)
	{
		ConfirmToggleCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleConfirmToggleChanged);
	}

	USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();
	if (Settings != nullptr)
	{
		if (ConfirmToggleCheckBox != nullptr)
		{
			ConfirmToggleCheckBox->SetIsEnabled(true);
		}
		if (ConfirmToggleHintText != nullptr)
		{
			ConfirmToggleHintText->SetText(FText::FromString(FString(SiegeSettingsMenuText::ConfirmHint)));
		}

		// In-memory getter. ⛔ Never a LoadGameFromSlot on a read path
		// (CONVENTIONS section 2) - this same getter is read at confirm time
		// mid-battle by TASK-443.
		ApplyConfirmValueToRow(Settings->IsAssistantConfirmEnabled());

		Settings->OnSettingsChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleSettingsChanged);
	}
	else
	{
		ShowRowUnavailable();
	}

	// Bind AFTER the seed.
	if (ConfirmToggleCheckBox != nullptr)
	{
		ConfirmToggleCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &USettingsMenuWidget::HandleConfirmToggleChanged);
	}

	// TASK-1115: bound unconditionally, for the same reason as Back. The graphics
	// panel reads USiegeGraphicsSettingsSubsystem, not USiegeSettingsSubsystem, so
	// a missing assistant-settings subsystem must not take the graphics screen
	// down with it — the two are unrelated (GFX-§3).
	if (GraphicsButton != nullptr)
	{
		GraphicsButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuWidget::HandleGraphicsClicked);
	}

	// Back is bound unconditionally and last: it must work even when the
	// settings subsystem is missing and the row above is dead. A panel you
	// cannot leave is worse than a panel that cannot change anything.
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USettingsMenuWidget::HandleBackClicked);
	}
}

void USettingsMenuWidget::UnbindAll()
{
	// Symmetric unbind (defensive - dynamic delegates tolerate dead objects,
	// but a dismissed panel must not keep reacting to settings broadcasts).
	if (USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem())
	{
		Settings->OnSettingsChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleSettingsChanged);
	}

	if (ConfirmToggleCheckBox != nullptr)
	{
		ConfirmToggleCheckBox->OnCheckStateChanged.RemoveDynamic(this, &USettingsMenuWidget::HandleConfirmToggleChanged);
	}

	if (GraphicsButton != nullptr)
	{
		GraphicsButton->OnClicked.RemoveDynamic(this, &USettingsMenuWidget::HandleGraphicsClicked);
	}

	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USettingsMenuWidget::HandleBackClicked);
	}
}

void USettingsMenuWidget::BackPressed()
{
	// ⛔ RemoveFromParent(self) AND NOTHING ELSE (CONVENTIONS section 4 / spec
	// item 4). It does NOT re-create or re-open WBP_MainMenu: TASK-438 adds this
	// panel ON TOP with AddToViewport(ZOrder 10) and never removes the menu, so
	// the menu underneath is already alive and already correct. Re-creating it
	// would put navigation state in the leaf plus a soft asset path to get
	// wrong, and would hand the player a menu that is not the object they left.
	UE_LOG(LogSiegeSettings, Log, TEXT("[SettingsMenu] Back pressed - dismissing the settings panel only."));
	RemoveFromParent();
}

void USettingsMenuWidget::ConfirmTogglePressed(bool bChecked)
{
	USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();
	if (Settings == nullptr)
	{
		// Null-safe: say why, disable the row, never crash. The value the player
		// just tried to set is deliberately NOT cached anywhere - a settings
		// screen that silently remembers a change it could not persist is worse
		// than one that admits it.
		ShowRowUnavailable();
		return;
	}

	UE_LOG(LogSiegeSettings, Log, TEXT("[SettingsMenu] Confirm-before-execute toggled to %s."),
		bChecked ? TEXT("ON") : TEXT("OFF"));

	// The subsystem owns the write AND the save (CONVENTIONS section 2:
	// save-on-change). It also owns the no-op rule: OnSettingsChanged fires only
	// on a real value change, so the round trip below cannot loop.
	Settings->SetAssistantConfirmEnabled(bChecked);
}

void USettingsMenuWidget::HandleConfirmToggleChanged(bool bIsChecked)
{
	ConfirmTogglePressed(bIsChecked);
}

void USettingsMenuWidget::HandleBackClicked()
{
	BackPressed();
}

void USettingsMenuWidget::GraphicsPressed()
{
	// ⛔ ZOrder 20 — ABOVE this panel's own 10 (TASK-438 adds this one at 10), and
	// ⛔ this panel is NOT removed. That is the whole navigation contract: the
	// graphics panel's Back is RemoveFromParent() on ITSELF, and the player lands
	// back on a settings panel that was never destroyed and is already correct.
	//
	// ⚠️ nullptr for the class parameter is the SHIPPING state, not an oversight:
	// /Game/UI/WBP_GraphicsMenu is RESERVED and UNAUTHORED (GFX-§2), so
	// CreateAndAddToViewport falls back to the C++ class and the code-authored
	// tree renders the panel. If that asset is ever authored, this one argument is
	// the only line that changes.
	USiegeGraphicsMenuWidget* Panel = USiegeGraphicsMenuWidget::CreateAndAddToViewport(
		GetOwningPlayer(), nullptr, /*ZOrder*/ 20);

	if (Panel == nullptr)
	{
		// Never fatal — the settings screen is untouched and the player is exactly
		// where they were. The reason is already logged by CreateAndAddToViewport.
		UE_LOG(LogSiegeSettings, Warning,
			TEXT("[SettingsMenu] Graphics pressed but no panel was created - the settings screen is unchanged."));
		return;
	}

	UE_LOG(LogSiegeSettings, Log,
		TEXT("[SettingsMenu] Graphics pressed - the graphics panel is open ON TOP of this one (ZOrder 20); this panel was NOT removed."));
}

void USettingsMenuWidget::HandleGraphicsClicked()
{
	GraphicsPressed();
}

void USettingsMenuWidget::HandleSettingsChanged(FName SettingName)
{
	// The broadcast token is not filtered on - see ConfirmSettingName in the
	// header. Re-read the live value instead; ApplyConfirmValueToRow suppresses
	// the BIE when nothing actually moved, so a broadcast for some future OTHER
	// setting costs one in-memory read and produces no spurious event.
	USiegeSettingsSubsystem* Settings = ResolveSettingsSubsystem();
	if (Settings == nullptr)
	{
		ShowRowUnavailable();
		return;
	}

	UE_LOG(LogSiegeSettings, Verbose, TEXT("[SettingsMenu] Settings changed broadcast ('%s') - refreshing the row."),
		*SettingName.ToString());

	ApplyConfirmValueToRow(Settings->IsAssistantConfirmEnabled());
}

void USettingsMenuWidget::ApplyConfirmValueToRow(bool bValue)
{
	if (ConfirmToggleCheckBox != nullptr)
	{
		// Does not re-enter HandleConfirmToggleChanged - see SeedAndBind.
		ConfirmToggleCheckBox->SetIsChecked(bValue);
	}

	// The delegate law: notify on a real change (and on the first seed, so a
	// future WBP_SettingsMenu never opens stale), never on a no-op.
	const bool bIsNew = !bHasPushedConfirmValue || (bValue != bLastPushedConfirmValue);

	bLastPushedConfirmValue = bValue;
	bHasPushedConfirmValue = true;

	if (bIsNew)
	{
		OnSettingsValueChanged(FString(ConfirmSettingName), bValue);
	}
}

void USettingsMenuWidget::ShowRowUnavailable()
{
	// LOG ONCE per widget instance. This can be reached from the seed, from a
	// click and from a broadcast, and a settings screen that spams the log every
	// time the player pokes a dead check box is a settings screen nobody reads
	// the log of.
	if (!bLoggedSubsystemUnavailable)
	{
		bLoggedSubsystemUnavailable = true;
		UE_LOG(LogSiegeSettings, Warning,
			TEXT("[SettingsMenu] USiegeSettingsSubsystem could not be resolved - the confirm row is disabled. The assistant fails safe and will still ask for confirmation."));
	}

	if (ConfirmToggleCheckBox != nullptr)
	{
		// Shown CHECKED and DISABLED, and that is honest rather than decorative:
		// TASK-443 spec item (4) fails safe by treating an unresolvable
		// subsystem as confirm = ON, because failing safe means MORE review,
		// never less. The row therefore shows what the game will actually do.
		ConfirmToggleCheckBox->SetIsChecked(true);
		ConfirmToggleCheckBox->SetIsEnabled(false);
	}

	// The forced state above is NOT an observed setting value, so clear the
	// "already pushed" latch: if a real value ever arrives after this, it must
	// notify even when it happens to equal the last genuine push.
	bHasPushedConfirmValue = false;

	if (ConfirmToggleHintText != nullptr)
	{
		ConfirmToggleHintText->SetText(FText::FromString(FString(SiegeSettingsMenuText::ConfirmUnavailable)));
	}
}

USiegeSettingsSubsystem* USettingsMenuWidget::ResolveSettingsSubsystem() const
{
	// Null-safe at every hop - the USessionMenuWidget::ResolveSessionSubsystem
	// shape, cloned. The subsystem lives on the GAME INSTANCE precisely so this
	// value survives the L_MainMenu -> L_Arena travel (CONVENTIONS section 2).
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USiegeSettingsSubsystem>() : nullptr;
}
