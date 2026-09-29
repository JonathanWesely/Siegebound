// Copyright Epic Games, Inc. All Rights Reserved.

#include "Siegebound/SiegeControlsHelpWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
// TASK-1576 (2026-09-28) — the owners of the two numbers the pages now SHOW, read and never typed
// (`HELP-§2`): ABuilding for the stack health factor (its public static StackHealthMultiplier) and
// USiegeMapMarkSubsystem for the map-circle cap (its public MaxMapMarks). Both are READ-ONLY to this
// file; see the derived-number block in the anonymous namespace below.
#include "Siegebound/Building.h"
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"
#include "Siegebound/SiegeMapMarkSubsystem.h"
// TASK-1432 — the ONLY new dependency this row takes. The overlay CALLS this subsystem from two
// places (open / close) and ⛔ never the other way round: `SiegeMenuInputSubsystem.{cpp,h}` is
// READ-ONLY to this row and is not modified by it.
#include "Siegebound/SiegeMenuInputSubsystem.h"

DEFINE_LOG_CATEGORY(LogSiegeControlsHelp);

namespace SiegeControlsHelpText
{
	// ════════════════════════════════════════════════════════════════════════════════════════
	//  ⛔⛔ THE LITERAL LAW FOR THIS WHOLE FILE (`HELP-§1`), STATED ONCE SO EVERY STRING BELOW
	//      CAN BE CHECKED AGAINST IT IN ONE READING:
	//
	//  ⛔ NOT ONE CHARACTER OF ANY KEY CHIP IS TYPED HERE. Every chip is composed by
	//     ComposeKeyChipLabel from FKey::GetDisplayName on an FKey that came out of the lane
	//     resolver — so a rebind or a Win+Space surfaces automatically and there is no second
	//     copy of the truth to drift.
	//
	//  ⚖️ AND THE RULE FOR PROSE, WHICH IS WHERE THE REAL JUDGEMENT LIVES: a key named in a
	//     DESCRIPTION is a defect only if that key CAN MOVE. `KBD-§4` tables the 26 LETTERS
	//     and nothing else (SiegeKeyboardLayoutStatics.cpp:57-63), so "right-click", "the
	//     mouse wheel", "Escape", "Enter", "Tab" and the digit keys are provably
	//     layout-invariant and naming them in prose can never become wrong.
	//
	//  ⭐⭐ AND WHERE A MOVABLE KEY MUST BE NAMED IN PROSE — TASK-707's detail lane needs the
	//     Attack/Defend order keys inside three sentences — IT IS WRITTEN AS A `{ActionId}`
	//     TOKEN AND SPLICED WITH THAT ROW'S OWN DERIVED CHIP by ResolveDetailTokens. ⛔ A typed
	//     `T` in a detail page would be the identical defect the chips exist to prevent, one
	//     line lower on the same screen — and it would be invisible on this QWERTY machine.
	//     ⇒ THE ONLY LETTER TYPED IN PROSE ANYWHERE IN THIS FILE IS THE ASSISTANT'S `Z`, AND IT
	//     IS THE SANCTIONED `KBD-§8` EXCEPTION — see the Interface.AssistantAccept row, which
	//     carries its own justification and its citation, in the one-liner AND in the detail.
	//
	//  ⭐ THE OVERLAY'S OWN KEY IS NEVER TYPED EITHER. The hint line is composed at refresh
	//     time from the Interface.ControlsHelp row's OWN derived chip (`HELP-§4`: "the menu
	//     documents its own key"), which is why HintFormat carries a {0} and not a key name.
	// ════════════════════════════════════════════════════════════════════════════════════════

	static const TCHAR* Title = TEXT("Controls");

	/**
	 *  The hint line, split around the Interface.ControlsHelp row's LIVE-DERIVED chip.
	 *  ⛔ Never a typed key name — the chip is spliced in at RefreshRows time (`HELP-§4`).
	 *
	 *  ⚠️ TWO CONSTANTS AND A CONCATENATION RATHER THAN ONE FORMAT STRING, DELIBERATELY:
	 *  FString::Printf statically requires a TCHAR ARRAY LITERAL, so a `const TCHAR*` format
	 *  constant fails the engine's own static_assert (the UAccountMenuWidget "Logged in as
	 *  <DisplayName>" note records the same trap). Concatenation has no such requirement.
	 */
	static const TCHAR* HintKeyPrefix = TEXT("Click any row for the full explanation. Press ");
	static const TCHAR* HintKeySuffix = TEXT(" again, or Close, to return to the fight. The battle keeps running.");

	/** Used when IA_ControlsHelp has not resolved: the honest degradation, ⛔ not a guess at a key. */
	static const TCHAR* HintNoKey =
		TEXT("Click any row for the full explanation. Press Close to return to the fight. The battle keeps running.");

	static const TCHAR* CloseLabel = TEXT("Close");

	// ---- The full-screen detail view (TASK-707) ----------------------------------------
	/**
	 *  ⛔ NO KEY NAME HERE EITHER. The Back control is a BUTTON, not a key, precisely because
	 *  the only key that could have been offered for "go back" is `Escape` — and `Escape` is
	 *  permanently untouchable (`AS-§6 A-2`, `HELP-§5`). The label therefore names the
	 *  DESTINATION, ⛔ never a keystroke.
	 */
	static const TCHAR* BackLabel = TEXT("Back to the controls list");

	// ⭐ TASK-1496 — `ScrollDownLabel` (the scroll control's label, ⭐ `TASK-1484`) stood here and
	// ⛔ went with its button at 🧑 his ruling of 2026-09-26. ⛔ Its own standing warning — ⛔ never
	// the bare glyph `<` or `>`, because `FindStepperPair` reads those two as a STEPPER PAIR — is
	// ⛔ NOT lost: it is re-stated at `DetailScrollButton`'s former declaration in the header,
	// where it now reads as ⛔ what would make the hazard live again.

	/** The heading over a detail page's related-controls blocks — Jonathan's "all the controls with it". */
	static const TCHAR* RelatedHeader = TEXT("All the controls that go with it");

	/** ⭐ ONE definition of the `{ActionId}` token's shape, read by both the writer and the resolver. */
	static const TCHAR* ActionTokenOpen  = TEXT("{");
	static const TCHAR* ActionTokenClose = TEXT("}");

	/**
	 *  ⭐ TASK-1576 (2026-09-28) — the opening of a DERIVED-NUMBER token, e.g. `{#MapMarks.Cap}`.
	 *  A second token kind in the same prose, told apart from a `{ActionId}` key token by the `#`.
	 *  ComposeDetailForDisplay replaces each one with a number READ from the property that owns it
	 *  (the derived-number block in the anonymous namespace below), BEFORE ResolveDetailTokens ever
	 *  sees the text, so no key-token code path changes. ⛔ A number token naming nothing in that
	 *  block is LEFT VISIBLE, exactly like a misspelled key token (`HELP-§2` mechanism 2), and the
	 *  suite's "no unresolved token" scans read the composed text, so they catch it.
	 */
	static const TCHAR* NumberTokenOpen = TEXT("{#");

	/** Lane D's chip: there is no key, and saying so beats an empty box (`HELP-§2` mechanism 2). */
	static const TCHAR* PointerChip = TEXT("Mouse click");

	/** A row whose action did not resolve and has no reference key. ⛔ A VISIBLE gap, never a hidden row. */
	static const TCHAR* NotBoundChip = TEXT("(not bound)");

	/** F-2's declared default presentation: ONE chip, slash-separated (handoffs/TASK-704-programmer.md §8). */
	static const TCHAR* KeySeparator = TEXT(" / ");

	/** ⛔ The `HELP-§2` gap string, pinned by law. ONE definition; the tests and TASK-707 both read it from here. */
	static const TCHAR* Undocumented = TEXT("(undocumented — TODO)");

	// ---- Category identifiers (FName keys) and their headers ---------------------------
	static const TCHAR* CategoryHero      = TEXT("Hero");
	static const TCHAR* CategoryCards     = TEXT("Cards");
	static const TCHAR* CategoryOrders    = TEXT("Orders");
	static const TCHAR* CategoryPickMode  = TEXT("PickMode");
	static const TCHAR* CategoryInterface = TEXT("Interface");

	static const TCHAR* HeaderHero      = TEXT("Your hero");
	static const TCHAR* HeaderCards     = TEXT("Cards and the HUD");
	static const TCHAR* HeaderOrders    = TEXT("Army orders");
	static const TCHAR* HeaderPickMode  = TEXT("Drawing the circles");
	static const TCHAR* HeaderInterface = TEXT("Interface");
}

namespace
{
	/**
	 *  /Game/Input/Actions/<Name>.<Name> — the exact composed object-path form the controller's
	 *  own pinned soft-ref block uses (SiegePlayerController.cpp:200-215).
	 *
	 *  ⛔ SOFT ON PURPOSE: a missing IA_* asset makes the row's chip read "(not bound)" and is
	 *  ⛔ never a crash and ⛔ never a hidden row (`HELP-§2` mechanism 2; the IA_CmdAmbush /
	 *  IA_CmdFollow / IA_AssistantConsole / IA_WarMap precedent).
	 */
	TSoftObjectPtr<UInputAction> MakeActionRef(const TCHAR* AssetName)
	{
		// ⚠️ The path is composed into an FString and handed over as a TCHAR* — the
		// FSoftObjectPath string-view constructor's unambiguous overload. The FSoftObjectPath
		// argument is BRACE-initialised per the MOST-VEXING-PARSE law (TASK-416), the same
		// shape UWarMapWidget's constructor uses for its own soft defaults.
		const FString ObjectPath = FString::Printf(TEXT("/Game/Input/Actions/%s.%s"), AssetName, AssetName);
		return TSoftObjectPtr<UInputAction>(FSoftObjectPath{ *ObjectPath });
	}

	/**
	 *  Makes a code-authored UButton mouse-clickable but ⛔ NOT keyboard-focusable.
	 *
	 *  ⚠️⚠️ WHY THIS IS A FIELD WRITE AND NOT THE OBVIOUS `InitIsFocusable(false)` CALL —
	 *  MEASURED IN THE 5.8 HEADERS, NOT ASSUMED:
	 *    • `UButton::InitIsFocusable` is **`protected`** (`Button.h:206`, inside the protected
	 *      section opened at `:187`). Its only engine caller is `UCommonButtonBase`, which is a
	 *      UButton SUBCLASS. Calling it from a UUserWidget is a hard C2248.
	 *    • `UButton` exposes `GetIsFocusable()` (`:156`) and ⛔ **no public setter** — its
	 *      UPROPERTY declares `Getter` with no `Setter` (`:68`).
	 *    • `UWidget` has ⛔ **no focusable API at all** in 5.8, and
	 *      `UUserWidget::SetIsFocusable` governs the USER WIDGET, ⛔ not the inner SButton.
	 *    • The field `IsFocusable` IS public (`Button.h:70`, inside the `public:` at `:36`) and
	 *      carries `UE_DEPRECATED(5.2, "…Please use the getter.")` — hence the pragma pair,
	 *      which is the same wrapping the engine uses for its own accesses.
	 *
	 *  ⭐ AND THE WRITE IS BYTE-EQUIVALENT TO THE CALL: `InitIsFocusable` is nothing but
	 *  `IsFocusable = InIsFocusable;` (`Button.cpp:250-252`), and `UButton::RebuildWidget`
	 *  reads the field once at `Button.cpp:84` (`.IsFocusable(IsFocusable)`). ⛔ THE ONLY
	 *  REQUIREMENT IS THAT IT HAPPEN BEFORE THE SWidget IS BUILT — which is why every caller
	 *  sits in the construction pass, ⛔ never in NativeConstruct.
	 *
	 *  ⚖️ A bespoke UButton subclass with a public wrapper was considered and REJECTED: it
	 *  would add a fourth reflected class to a file pair `HELP-§3` pins to three, and the
	 *  `BindWidgetOptional` members must stay typed `UButton` or an asset-authored
	 *  WBP_ControlsHelpRow using a plain Button would stop binding (the `HELP-§3` escape hatch).
	 *
	 *  ⭐ TASK-1480 (o) (2026-09-27, `qa/TASK-1497.md` NIT, ⛔ RULED: RETAIN) — ⛔ DELIBERATELY
	 *  KEPT WITH ⛔ ZERO LIVE CALLERS. `CloseButton`'s call was flipped at ⭐ `TASK-1432` and the
	 *  last two (`RowButton`, `BackButton`) at ⭐ `TASK-1478`; since then every `UButton` this file
	 *  builds goes through `ApplyButtonFocusable` below. ⛔ IT STAYS BECAUSE A TRIP-WIRE NAMES IT
	 *  AS THE REVERT: the one at `RowButton`'s flip in `USiegeControlsHelpRowWidget::ConstructRowTree`
	 *  (it opens "TRIP-WIRE — ⛔ READ `USiegeControlsHelpWidget::ApplyActiveView` BEFORE TOUCHING
	 *  THIS LINE" and ends "THIS WRITE MUST GO BACK TO `ApplyButtonNotFocusable` ⛔ IN THE SAME
	 *  DIFF"; both quotes span a line break in the source). ⛔ IT IS THE ONLY ONE THAT DOES:
	 *  `BackButton`'s flip in `USiegeControlsDetailWidget::ConstructDetailTree` carries a
	 *  trip-wire for the SAME collapse (it opens "THE TRIP-WIRE (`qa/TASK-1433.md` WARN-L1)") but
	 *  names no revert and never mentions this helper, and the TASK-1478 switcher block above
	 *  `USiegeControlsHelpWidget::ApplyActiveView` cites this comment only as a warning, in its
	 *  rejected (γ) (TASK-1480 QA loop 1, 2026-09-27, `qa/TASK-1481.md` W1). ⛔ Do NOT delete it
	 *  or fence it off as dead code: either is an executable change, and the revert the
	 *  `RowButton` wire prescribes would have nothing left to call.
	 */
	void ApplyButtonNotFocusable(UButton* Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Button->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	/**
	 *  ⭐⭐ TASK-1432 — THE MIRROR OF `ApplyButtonNotFocusable`, AND IT EXISTS SO THE DECISION
	 *  IS ⛔ WRITTEN AT THE SITE RATHER THAN ⛔ INFERRED FROM AN ABSENCE.
	 *
	 *  ⚠️ `UButton::IsFocusable` ⛔ ALREADY DEFAULTS TO `true` (`Button.h:70`), so this write
	 *  changes ⛔ no engine default — what it changes is the ⛔ READING of the call site. ~~THREE
	 *  buttons in this file are deliberately ⛔ NOT focus stops and each says so in one word;
	 *  the fourth is deliberately the screen's ⛔ ONLY focus stop, and~~ a ⛔ BLANK LINE there
	 *  would look exactly like somebody having forgotten the call. ⛔ A deliberate decision that
	 *  is invisible in the diff is a decision the next editor will undo by accident.
	 *  ⭐ TASK-1480 (o) (2026-09-27) — THE STRUCK COUNT EXPIRED AT ⭐ `TASK-1478`: every `UButton`
	 *  this file builds (`RowButton`, `BackButton`, `CloseButton` — the whole census of its
	 *  `ConstructWidget<UButton>` calls) now goes through THIS function and ⛔ none through its
	 *  opposite number, which is retained with no live caller (see its own comment above). The
	 *  reason for writing the call survives the count: a blank line at any of the three sites
	 *  would still read as a forgotten call.
	 *
	 *  ⚠️ SAME DEPRECATION WRAPPING, ⛔ SAME MEASURED REASON — see the long comment directly
	 *  above, which holds the whole audit: `InitIsFocusable` is `protected` (`Button.h:206`),
	 *  the UPROPERTY declares `Getter` with ⛔ no `Setter` (`:68`), `UWidget` has no focusable
	 *  API in 5.8, and `UUserWidget::SetIsFocusable` governs the USER WIDGET rather than the
	 *  inner `SButton`. ⛔ The public field is the supported route and it carries
	 *  `UE_DEPRECATED(5.2, …)` — hence the pragma pair.
	 *
	 *  ⛔ AND IT MUST RUN BEFORE THE `SWidget` IS BUILT: `UButton::RebuildWidget` reads the field
	 *  exactly once (`Button.cpp:84`), so the call belongs in the construction pass — ⛔ never in
	 *  `NativeConstruct`, for the identical reason its opposite number gives.
	 */
	void ApplyButtonFocusable(UButton* Button)
	{
		if (Button == nullptr)
		{
			return;
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Button->IsFocusable = true;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	// ------------------------------------------------------------------------------------
	//  THE TWO VIEWS THE OVERLAY SWITCHES BETWEEN (TASK-707).
	//
	//  ⚠️ THE ORDER IS THE CONTRACT: ConstructHelpTree adds PanelBorder first and DetailView
	//  second, so these two constants are the indices of those adds and ⛔ nothing else may
	//  reorder them. Named rather than typed at the call sites so a reader of
	//  ShowDetailForAction/ReturnToList sees WHICH view, not a bare 0 and 1.
	// ------------------------------------------------------------------------------------
	constexpr int32 ListViewIndex   = 0;
	constexpr int32 DetailViewIndex = 1;

	/**
	 *  The margin that insets the LIST panel from the screen edge.
	 *
	 *  ⭐ IT LIVES ON THE SWITCHER SLOT, ⛔ NOT ON BackdropBorder, AND THAT IS THE WHOLE REASON
	 *  THE DETAIL VIEW CAN BE FULL SCREEN. A padding on the backdrop would inset BOTH views
	 *  identically, and Jonathan asked for the detail page to be "on the entire screen" while the
	 *  list stays a readable plate. Per-slot padding is how one container gives its two children
	 *  different margins.
	 *
	 *  ⚠️ A FUNCTION, ⛔ NOT A NAMESPACE-SCOPE `const FMargin`: FMargin's four-argument
	 *  constructor is not constexpr (Margin.h:83), so an object here would add a dynamic static
	 *  initializer to this translation unit. The file already avoids that shape deliberately for
	 *  the registry's FKeys (the SiegeKeyboardLayoutStatics.cpp:22-30 precedent).
	 */
	FMargin MakeListPanelMargin()
	{
		return FMargin(140.f, 60.f, 140.f, 60.f);
	}

	// ------------------------------------------------------------------------------------
	//  ⭐ TASK-1496 — ⭐ `TASK-1484`'s TWO TUNING NUMBERS STOOD HERE AND ⛔ WENT WITH THE BUTTON.
	//
	//  ⛔ `DetailScrollPageFraction = 0.85f` (one press = that fraction of the box's OWN measured
	//  height) and ⛔ `DetailScrollFallbackStep = 320.f` (the degradation used only while the box
	//  had no cached geometry). ⛔ Recorded rather than deleted (`SC-§120`) because the ⛔ shape of
	//  the answer is worth keeping if a page ever DOES overflow: ⛔ a fraction of the MEASURED
	//  viewport, ⛔ never a pixel count, and ⛔ less than 1.0 so the reader keeps an overlapping
	//  line. ⛔ As measured at ⭐ `TASK-1494`, ⛔ no page overflows, so nothing reads them.
	// ------------------------------------------------------------------------------------

	// ════════════════════════════════════════════════════════════════════════════════════
	//  ⭐⭐ TASK-1576 (2026-09-28) — THE NUMBERS A PAGE SHOWS, EACH READ FROM ITS OWNER.
	//
	//  `HELP-§2`: "NO NUMBER IS RESTATED IN PROSE IF IT CAN BE READ FROM DATA ... Prefer deriving".
	//  The prose carries a `{#Name}` token (SiegeControlsHelpText::NumberTokenOpen) where a number
	//  goes; ComposeDetailForDisplay hands the text to SpliceDerivedNumbers, which asks the entry
	//  below for that token to READ the owning property at that moment, format it in player units
	//  (FText::AsNumber with fixed fractional digits, spliced by FText::Format), and put the result
	//  in place of the token. ⇒ a retune of the owner changes the page with ⛔ no text edit, and
	//  ⛔ no digit of either value is typed anywhere in this file.
	//
	//  ⛔ READS GO THROUGH EXISTING PUBLIC MEMBERS ONLY (TASK-1576 spec (3)). The row's other
	//  candidates (the discard fee, the four Rally values, the melee reach / cone / cooldown, and
	//  each building's own height limit) are NOT here because reading them needs a new accessor in
	//  an owner file: DiscardAllCost and the hero's Melee* / Rally* tunables are protected with no
	//  public getter, and the card-to-building-class resolution the game uses
	//  (ASiegePlayerController::ResolveCardActorClass / IsBuildingCard) is private. They are listed
	//  in handoffs/TASK-1576-programmer.md for the manager to board; ⛔ none is typed instead.
	//
	//  WHICH OBJECT EACH ENTRY READS, AND WHY (spec (2): the object the game uses, ⛔ never simply
	//  the easiest one to reach):
	//    • the map-circle cap: USiegeMapMarkSubsystem's class default object. The game's store is
	//      the instance ULocalPlayer's subsystem collection creates FROM THIS CLASS, and its
	//      MaxMapMarks is the CDO's value: the property is EditDefaultsOnly on a class with no
	//      Config specifier and no asset, nothing in Source/ assigns it, and no Blueprint child of
	//      the class exists (asset registry and loaded classes both read empty, TASK-1576). The
	//      live instance therefore cannot differ, and reading the default keeps the registry free
	//      of any world, which is what lets the suite compose every page headlessly.
	//    • the stack health factor: ABuilding::StackHealthMultiplier(1), the existing public
	//      static. ABuilding::ApplyStackUpgrade multiplies MaxHP by exactly this value on every
	//      upgrade (`MaxHP = OldMaxHP * StackHealthMultiplier(1);`), and the function reads
	//      ABuilding's own class default for EVERY building (StackHealthStep is game-wide by
	//      ruling; its own comment says a Blueprint child's value would be IGNORED). ⇒ this is the
	//      value the game applies, including the function's guard (a step below 1 applies 1).
	// ════════════════════════════════════════════════════════════════════════════════════

	/**
	 *  One number a page may show: the token the prose writes, and the function that reads the
	 *  owner and formats the value. Compose returns false when the owner cannot be read, and the
	 *  token is then LEFT VISIBLE (`HELP-§2` mechanism 2), ⛔ never replaced by a guessed value.
	 *  ⚠️ A plain aggregate of two constant pointers, so the table below is constant-initialised
	 *  and adds no dynamic initialiser to this translation unit (the MakeListPanelMargin note).
	 */
	struct FSiegeHelpDerivedNumber
	{
		const TCHAR* Token;
		bool (*Compose)(FText& OutNumberText);
	};

	/**
	 *  `{#MapMarks.Cap}` — how many circles the war map holds at once, e.g. "9 circles".
	 *  Owner: USiegeMapMarkSubsystem::MaxMapMarks (public UPROPERTY; the `AddMark` refusal compares
	 *  against it). Read from the class default object; the reason is in the block comment above.
	 */
	bool ComposeHelpMapMarkCap(FText& OutNumberText)
	{
		const USiegeMapMarkSubsystem* const MarkStoreDefaults = GetDefault<USiegeMapMarkSubsystem>();
		if (MarkStoreDefaults == nullptr)
		{
			return false;
		}

		const int32 HelpMapMarkCap = MarkStoreDefaults->MaxMapMarks;

		// Conversion: none. It is a count of circles, so it is shown as a whole number with no
		// fractional digits. The noun is chosen by the count itself (the plural argument), so a
		// retune to one reads "1 circle".
		FNumberFormattingOptions CountOptions;
		CountOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(0);

		FFormatNamedArguments NumberArgs;
		NumberArgs.Add(TEXT("Count"), FText::AsNumber(HelpMapMarkCap, &CountOptions));
		NumberArgs.Add(TEXT("PluralCount"), HelpMapMarkCap);
		OutNumberText = FText::Format(
			FText::FromString(FString(TEXT("{Count} {PluralCount}|plural(one=circle,other=circles)"))), NumberArgs);
		return true;
	}

	/**
	 *  `{#StackUpgrade.HealthFactor}` — what each stack upgrade multiplies a building's maximum
	 *  health by, e.g. "1.5". Owner: ABuilding::StackHealthStep (protected, no getter), read
	 *  through the existing PUBLIC static ABuilding::StackHealthMultiplier at one upgrade, which
	 *  is the factor ApplyStackUpgrade applies; the reason is in the block comment above.
	 */
	bool ComposeHelpStackHealthFactor(FText& OutNumberText)
	{
		const float HelpStackHealthFactor = ABuilding::StackHealthMultiplier(1);

		// Conversion: none. It is a multiplier, not a bonus or a distance, so it is shown as the
		// factor itself ("by 1.5"), with at most two fractional digits and no trailing zeros.
		FNumberFormattingOptions FactorOptions;
		FactorOptions.SetMinimumFractionalDigits(0).SetMaximumFractionalDigits(2);

		FFormatNamedArguments NumberArgs;
		NumberArgs.Add(TEXT("Factor"), FText::AsNumber(HelpStackHealthFactor, &FactorOptions));
		OutNumberText = FText::Format(FText::FromString(FString(TEXT("{Factor}"))), NumberArgs);
		return true;
	}

	/** Every number a page may show. ⛔ One entry per token; the prose names the token, this names the owner. */
	const FSiegeHelpDerivedNumber HelpDerivedNumbers[] =
	{
		{ TEXT("{#MapMarks.Cap}"),              &ComposeHelpMapMarkCap },
		{ TEXT("{#StackUpgrade.HealthFactor}"), &ComposeHelpStackHealthFactor }
	};

	/**
	 *  Replaces every known `{#Name}` token in DetailText with its freshly read, formatted number.
	 *  Text with no `{#` is returned untouched (the cheap gate ResolveDetailTokens also uses), and
	 *  an unknown or unreadable token stays in the text, visibly.
	 */
	FText SpliceDerivedNumbers(const FText& DetailText)
	{
		FString Working = DetailText.ToString();
		if (!Working.Contains(SiegeControlsHelpText::NumberTokenOpen, ESearchCase::CaseSensitive))
		{
			return DetailText;
		}

		bool bReplacedAny = false;
		for (const FSiegeHelpDerivedNumber& Number : HelpDerivedNumbers)
		{
			if (!Working.Contains(Number.Token, ESearchCase::CaseSensitive))
			{
				continue;
			}

			FText NumberText;
			if (!Number.Compose(NumberText))
			{
				continue;
			}

			Working.ReplaceInline(Number.Token, *NumberText.ToString(), ESearchCase::CaseSensitive);
			bReplacedAny = true;
		}

		return bReplacedAny ? FText::FromString(Working) : DetailText;
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  THE ACTION REGISTRY — handoffs/TASK-704-programmer.md §4, in category order.
//
//  ⚠️ THE ROW COUNT IS DELIBERATELY NOT STATED HERE ANY MORE. It read "24 rows" until TASK-823
//     appended three (Cards.StackUpgrade, Cards.PlacementResize, Interface.MapMarks); a count
//     transcribed into a comment is a fact that rots the next time the registry grows, and
//     GetActions().Num() is the answer that cannot. ⭐ Rows 25-27 are TASK-823's own prose,
//     authored at SOURCE from the shipped stack/wheel/mark code (`HELP-§2` mechanism 3) — every
//     sentence's citation rides in the comment above its string, exactly as 704's do.
//
//  ⛔ THE ORIGINAL ROW SET IS 704's, AND SO ARE ITS LANES AND ONE-LINERS, WITH ONE EXCEPTION.
//     For the original rows, every lane assignment below traces to that file's §1.1 audit and
//     every one-liner is its §4 text verbatim, EXCEPT Cards.Discard's: TASK-821 rewrote that
//     one-liner in place and moved its lane from PointerOnly to MappedAction (its own R-09
//     block says why). Rows 25-27 (Cards.StackUpgrade, Cards.PlacementResize,
//     Interface.MapMarks) carry TASK-823's own lanes and prose, authored at source.
//     TASK-1585 (2026-09-28): scoped the way the .h's OneLine doc already is. This line used to
//     claim 704's lanes and verbatim one-liners for every row, which the R-09 block refutes.
//  ⛔ THE QWERTY COLUMN IS A FALLBACK AND A TEST FIXTURE, ⛔ NEVER THE DISPLAYED TRUTH while
//     the action resolves in an active context (§1.2 / ResolveRowDisplayKeys).
//  ⛔ NO NUMBER FROM THE SHIPPED MECHANICS IS RESTATED IN ANY ONE-LINER (`HELP-§2`, the M7.7
//     "in 400" / AoERadius 700 lesson).
//
//  ════════ TASK-707: THE `Detail` COLUMN, AND THE FIVE RULES ITS TRANSFER FOLLOWED ════════
//  TASK-707 filled each original row's detail string from handoffs/TASK-704-programmer.md
//  §4's prose for that row. Not every string below is still 704's words: TASK-823 wrote its
//  three appended rows at source, and later rows rewrote others, among them TASK-821
//  (Cards.Discard, in place), TASK-870 (Interface.WarMap) and TASK-1541 (2026-09-27: the code
//  names the prose printed, put into plain words). 704 cited EVERY factual sentence at a
//  file:line it had personally read; ⛔ TASK-707's transfer re-authored, re-derived and
//  invented nothing, and where I disagreed with a sentence I FLAGGED it in
//  handoffs/TASK-707-programmer.md rather than "improving" it silently.
//
//  The five transfer rules, stated once so a reviewer can check any row in one reading. Each
//  application is enumerated per row in the TASK-707 handoff's transfer table:
//    T1  §4's `file:line` citations and its handoff-internal cross-references ("R-16..R-18")
//        move OUT of the player prose and INTO the C++ comment above the string. This is the
//        route TASK-706 §4(e) assigned to this task, and it is `HELP-§2`'s own instruction for
//        an unavoidable literal.
//    T2  Markdown emphasis (`**`, `*`, backticks) is dropped. It is markup for a .md file; a
//        backtick rendered on a UTextBlock is a stray character, ⛔ not the author's sentence.
//    T3  Pipeline marker emoji (⭐ ⚠️ ⛔ ✅ ⚖️) are dropped. They annotate the HANDOFF's reader.
//        ⚠️ `⛔` in particular would read to a PLAYER as "you may not do this", which on a
//        sentence like "the leash is what makes this HOLD" is actively misleading.
//    T4  ⭐⭐ A MOVABLE KEY NAMED IN PROSE BECOMES A `{ActionId}` TOKEN (`HELP-§1`).
//        §4's R-13/R-14/R-15/R-18 type the letters `T` and `E` for the Attack/Defend orders —
//        those are the two letters US-Dvorak moves to `Y` and `.`, so typing them here would be
//        the EXACT hardcoded-letter defect the key chips exist to prevent, one line lower on
//        the same screen. ResolveDetailTokens splices each token with the live derived chip, so
//        the sentence reads identically on QWERTY and correctly on every other layout.
//        ⛔ The assistant's `Z` is NOT tokenised: `KBD-§8`/`KBD-§0` ruling 1 pin every
//        human-facing accept-key string to the literal `Z`, and that row is the sanctioned
//        exception (704 §8 F-1, and bLiteralKeyLabel makes it one flag either way).
//    T5  Sentences addressed to an IMPLEMENTER rather than to a player — provenance of a
//        ruling, "the header says so on purpose", a C++ fragment, a defect post-mortem — move
//        into the comment beside the string, and any connective left dangling by a T1/T5
//        removal gets the MINIMUM grammatical repair. Every instance is listed in the handoff.
//  ⛔ NO TUNABLE'S VALUE IS RE-TYPED. 704 §4 deliberately NAMES tunables instead of restating
//     numbers (its U-5 / D-6, the M7.7 lesson). TASK-707 carried those names into the prose;
//     since TASK-1541 (2026-09-27) the prose describes each tunable in plain words ("a set
//     step", "the rally radius") and its code name sits in the comment beside the string.
//     The ONE number TYPED as a quantity anywhere below is the war map's 30 gold, because
//     Jonathan's own words are the source and 704 quoted them at the property
//     (CommanderNpc.h:297-311). Every other number a page shows is DERIVED at runtime: the
//     prose carries a `{#Name}` token and ComposeDetailForDisplay replaces it with the value read
//     from the property that owns it (TASK-1576, 2026-09-28: the map-circle cap and the stack
//     health factor; the derived-number block near the top of this file). The other digits
//     typed in the prose are names and list labels, not quantities: "Key 1", "stage-1", the
//     "1." to "3." stage labels and the chat box's "(1)" to "(4)". (Until TASK-1576 this read
//     "The ONE number stated anywhere below is the war map's 30 gold", which stopped being true
//     the moment a page showed a derived number.)
//  ⚠️ TASK-1480 (a) — A FILE-LEVEL DECLARATION, ADDED 2026-09-27 ON `qa/TASK-1433.md` WARN-L3,
//     BECAUSE THE PER-BLOCK ONES TAUGHT THE WRONG LESSON: three blocks below (R-08, R-19, R-24)
//     flag their remaining numbers ⛔ UNVERIFIED and the others say nothing, which reads as
//     "an unflagged block is trustworthy". ⛔ IT IS NOT. Every `file:line` NUMBER in this file's
//     `Citations (T1)` blocks is as of the row that wrote it and is ⛔ UNVERIFIED today, UNLESS
//     its own block says it was anchored BY TEXT or BY SYMBOL (e.g. R-02's whole block since
//     TASK-1480, and the one `bWantCursor` anchor each in R-08 / R-19 / R-24 since TASK-1432
//     QA loop 1). Treat a bare number as a lead: find the target by its symbol or its quoted
//     text, never by the digit (`CITE-BY-TEXT-RULED-2026-09-24`).
// ════════════════════════════════════════════════════════════════════════════════════════════

const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
{
	// Function-local static: the EKeys constants below resolve long after InputCore's own
	// statics exist (the SiegeKeyboardLayoutStatics.cpp:22-30 precedent — no cross-module
	// static-init order question).
	static const TArray<FSiegeControlsHelpAction> Actions = []()
	{
		using namespace SiegeControlsHelpText;

		TArray<FSiegeControlsHelpAction> Rows;

		// ⚠️ RESERVED UP FRONT so the references handed out below cannot be invalidated by a
		// regrow. Each reference is used only inside its own block, but a future edit should
		// not have to notice that.
		Rows.Reserve(32);

		auto AddRow = [&Rows](const TCHAR* InActionId, const TCHAR* InCategory, const TCHAR* InDisplayName,
			const TCHAR* InOneLine, ESiegeInputLane InLane) -> FSiegeControlsHelpAction&
		{
			FSiegeControlsHelpAction& NewRow = Rows.AddDefaulted_GetRef();
			NewRow.ActionId    = FName(InActionId);
			NewRow.Category    = FName(InCategory);
			NewRow.DisplayName = FText::FromString(FString(InDisplayName));
			NewRow.OneLine     = FText::FromString(FString(InOneLine));
			NewRow.Lane        = InLane;
			// ⛔ Detail is NOT set here: each row assigns its own below, from TASK-704 §4 under
			// the five transfer rules stated above. A row that ever forgets renders the pinned
			// "(undocumented — TODO)" string through ComposeDetailForDisplay — a VISIBLE gap,
			// ⛔ never a blank page (`HELP-§2` mechanism 2), and the suite fails it loudly.
			return NewRow;
		};

		// ─── CATEGORY: HERO ────────────────────────────────────────────────────────────────

		{
			// R-01 (704 §4). Lane A: four bindings on one action, three of them carrying
			// Negate/SwizzleAxis modifiers — which is exactly why they must never be rewritten
			// as a block (SiegeKeyboardLayoutStatics.cpp:227-233).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Move"), CategoryHero, TEXT("Move"),
				TEXT("Walk your hero around the battlefield."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Move")) };
			Row.QwertyReferenceKeys = { EKeys::W, EKeys::A, EKeys::S, EKeys::D };
			// 704 §4 R-01 detail. Citations (T1): the four-binding readback + the wholesale-rewrite
			// defect = handoffs/TASK-568-artist.md:23-49 and SiegeKeyboardLayoutStatics.cpp:227-233;
			// WalkSpeed = HeroCharacter.h:406; GetEffectiveWalkSpeed = HeroCharacter.h:343; the
			// positional-retarget contract = SiegeKeyboardLayoutStatics.h:53.
			Row.Detail = FText::FromString(FString(
				TEXT("Four separate keys on one control — forward, back, strafe left, strafe right. ")
				TEXT("The back and left keys carry a setting that reverses their direction, and the forward and back keys carry one that turns their push onto the forward-and-back line, ")
				TEXT("which is why these four keys must never be replaced as one set. ")
				TEXT("Your base walk speed is set on the hero; the speed you actually move at applies the Swift Boots ")
				TEXT("upgrade on top of that base, and the base itself is never changed.\n\n")
				TEXT("On a non-QWERTY layout these four keep their physical positions: the game changes which keys ")
				TEXT("the four answer to, not your muscle memory.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// the Negate and SwizzleAxis input modifiers, AHeroCharacter::WalkSpeed and
			// AHeroCharacter::GetEffectiveWalkSpeed(). Cited by symbol; the claims are unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording
			// this prose used to print — "bindings on one action", the "rows" carrying "a modifier",
			// "turns the input onto the forward axis", "rows must never be rewritten as a block", and
			// "the layout subsystem retargets the mapping context's keys". The mechanism stays here: each
			// IMC_Hero mapping's .Key is re-targeted one index at a time and the mappings array is never
			// rewritten, because rewriting it dropped the instanced Negate / SwizzleAxis modifiers (the
			// "ONLY `.Key` IS ASSIGNED" clause of the KBD-§1 loop in SiegeKeyboardLayoutStatics.cpp).
			// The claims are unchanged.
		}

		{
			// R-02. Mouse2D — a non-letter, so the fallback is an identity even if it is taken.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Look"), CategoryHero, TEXT("Look"),
				TEXT("Move the mouse to swing the camera."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Look")) };
			Row.QwertyReferenceKeys = { EKeys::Mouse2D };
			// 704 §4 R-02 detail. Citations (T1) — ⛔ RE-ANCHORED BY TEXT, ⛔ NOT RENUMBERED, at
			// TASK-1480 (a) (2026-09-27, `qa/TASK-1433.md` WARN-L3, `CITE-BY-TEXT-RULED-2026-09-24`);
			// every anchor below was opened and read at source for that row:
			//   the Negate_2 binding = handoffs/TASK-568-artist.md, its readback row naming `IA_Look`
			//   and `modifiers: [InputModifierNegate_2]` (the row pads its columns with runs of
			//   spaces, so grep either fragment, not the whole row), and
			//   handoffs/TASK-399-artist.md, its "`InputModifierNegate_2` | `IA_Look` / Mouse2D" row
			//   (X✗ Y✓ Z✗);
			//   the ignore-look pairing = `ASiegePlayerController::SetupInputComponent`'s IA_UICursor
			//   BindAction triple (Started → OnUICursorPressed; Completed AND Canceled →
			//   OnUICursorReleased), `ASiegePlayerController::OnUICursorPressed`'s
			//   `SetIgnoreLookInput(true);` ("paired 1:1 with ClearUICursorHold"), and
			//   `ASiegePlayerController::ClearUICursorHold`'s guarded `SetIgnoreLookInput(false);`;
			//   the GameAndUI/DoNotLock modes = `ASiegePlayerController::ApplyCursorInputState`'s
			//   `if (bWantCursor)` branch (`FInputModeGameAndUI InputMode;` then
			//   `SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock)`), where `bWantCursor`
			//   composes bInPlacementMode, bInTargetingMode and GroupPickStage among its terms.
			//   ⛔ WAS: `handoffs/TASK-568-artist.md:28` (that line is the IA_Move "3 S" row; the
			//   IA_Look row sits three lines lower), `SiegePlayerController.cpp:768-770 and :783-790`
			//   and `SiegePlayerController.cpp:4361-4370` — both controller ranges measured rotted by
			//   `qa/TASK-1433.md` WARN-L3 and landing on unrelated code again today. Quoted, not
			//   deleted, so a reader holding an older copy can still map them.
			Row.Detail = FText::FromString(FString(
				TEXT("Follows the mouse's up-down and left-right movement, with a setting that reverses the up-down direction. ")
				TEXT("Look is suspended while you hold the interface-cursor key — pressing the key switches camera look off ")
				TEXT("and its release switches it back on, one release for every press, so a click-drag on the HUD cannot ")
				TEXT("nudge the camera.\n\n")
				TEXT("It is not suspended in placement, targeting or a group pick: those modes let the game and the interface ")
				TEXT("both take input without locking the mouse to the window, so the mouse steers the cursor while movement keys ")
				TEXT("keep working.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// Negate_2, OnUICursorPressed, SetIgnoreLookInput(true), FInputModeGameAndUI and DoNotLock,
			// every one anchored by text in the citation block above. The claims are unchanged.
			// TASK-1574 (2026-09-28, the register pass): "Follows" and "a setting" replace "Bound to" and
			// "a modifier" (the IA_Look Mouse2D binding and its Negate_2 modifier, anchored above). The
			// claim is unchanged.
		}

		{
			// R-03.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Jump"), CategoryHero, TEXT("Jump"),
				TEXT("Jump."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Jump")) };
			Row.QwertyReferenceKeys = { EKeys::SpaceBar };
			// 704 §4 R-03 detail. Citations (T1): the template's press+release binding =
			// GitClaudeUnrealTestCharacter.cpp:59-60; the FellOutOfWorld death ruling =
			// HeroCharacter.h:138-151.
			Row.Detail = FText::FromString(FString(
				TEXT("The jump comes from the basic character the game was built on, and it reacts to both the press and the release.\n\n")
				TEXT("Falling out of the world is a death, not a despawn — instead of simply deleting your hero, which is what ")
				TEXT("would happen by default, the game deliberately treats the fall exactly like lethal damage, ")
				TEXT("so the standard respawn brings you back at your castle.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// AHeroCharacter::FellOutOfWorld, its skipped Super call, and Destroy(). The claim is unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "Inherited from the character template" (the jump is bound in
			// AGitClaudeUnrealTestCharacter's input setup, Jump on Started and StopJumping on Completed),
			// "bound on both press and release", "skips the engine's default handling" (the skipped
			// AActor::FellOutOfWorld Super call, which would Destroy() the pawn) and "goes down the same
			// path as lethal damage". The claims are unchanged.
		}

		{
			// R-04. ⚠️ HERO-SIDE ACTION: SprintAction is `protected` on AHeroCharacter
			// (HeroCharacter.h:388-402), a file this task does NOT own — so it is resolved by
			// SOFT PATH here and ⛔ no getter was added there (704 §3's fence note).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Sprint"), CategoryHero, TEXT("Sprint"),
				TEXT("Hold to run faster — it is a hold, not a toggle."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Sprint")) };
			Row.QwertyReferenceKeys = { EKeys::LeftShift };
			// 704 §4 R-04 detail. Citations (T1): the Started/Completed/Canceled binding =
			// HeroCharacter.cpp:292-294; SprintSpeed = HeroCharacter.h:410, WalkSpeed = :406;
			// GetEffectiveSprintSpeed/GetEffectiveWalkSpeed = HeroCharacter.h:346,343; the
			// bDead guard + unguarded release = HeroCharacter.cpp:323-338; melee independence =
			// HeroCharacter.cpp:340-360.
			Row.Detail = FText::FromString(FString(
				TEXT("It listens for the press, the release and a cancelled press, so the sprint can never stick on if the press is ")
				TEXT("interrupted. Pressing raises your top speed to your sprint speed and releasing returns it to ")
				TEXT("your walk speed; both include the Swift Boots move-speed bonus, worked out fresh each time, ")
				TEXT("rather than changing either base speed.\n\n")
				TEXT("A dead hero cannot start a sprint — starting checks for death first — but stopping has no such ")
				TEXT("check, so the sprint always releases.\n\n")
				TEXT("Sprinting and attacking are independent: sprint is a hold on one control, melee is a press on ")
				TEXT("another, and the melee swing never looks at whether you are sprinting.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// the Started / Completed / Canceled trigger events, SprintSpeed, WalkSpeed,
			// GetEffectiveSprintSpeed(), GetEffectiveWalkSpeed(), StartSprint's bDead early-out,
			// StopSprint and DoMeleeAttack. Cited by symbol; the claims are unchanged.
			// TASK-1574 (2026-09-28, the register pass): "one control" replaces "one action" (IA_Sprint
			// and IA_Attack, two separate input actions). The claim is unchanged.
		}

		{
			// R-05. Same hero-side fence as R-04.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Attack"), CategoryHero, TEXT("Attack"),
				TEXT("Swing at every enemy in front of you."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Attack")) };
			Row.QwertyReferenceKeys = { EKeys::LeftMouseButton };
			// 704 §4 R-05 detail. Citations (T1): MeleeRange = HeroCharacter.h:437,
			// MeleeHalfAngleDegrees = :441, MeleeCooldown = :445 (gate at HeroCharacter.cpp:354-360);
			// GetEffectiveMeleeDamage = HeroCharacter.h:234, applied at HeroCharacter.cpp:464;
			// no friendly fire = HeroCharacter.cpp:581-585; the melee-suppression API =
			// HeroCharacter.h:162-167 + HeroCharacter.cpp:342-346, set at SiegePlayerController.cpp:
			// 2723-2728 and released on every exit path at :3144-3148.
			// ⛔ NO NUMBER RESTATED (704 U-5, the M7.7 lesson), and since TASK-1541 (2026-09-27) no
			// code name either: the prose says "melee reach", "a cone in front of you" and "melee
			// cooldown" where it printed MeleeRange, MeleeHalfAngleDegrees and MeleeCooldown, and it no
			// longer prints GetEffectiveMeleeDamage(), SetMeleeSuppressed(true) or DoMeleeAttack.
			// ⛔ THE TARGET SET IS NAMED KIND BY KIND ("every enemy unit, hero, building and castle")
			// because those are exactly the team-agent kinds FSiegeCombatStatics::GatherHostileAgents
			// hands the swing: ACastle, ABuilding and its subclasses, AHeroCharacter, and ASummonedUnit
			// including miners. UTeamAgent is NotBlueprintable, so no asset can join the set. ⛔ NEVER
			// widen it to "everything on the enemy team": the enemy commander HAS a team but
			// deliberately does not implement ITeamAgent and names the melee cone among the sites it
			// stays out of (ACommanderNpc's class doc), and AGoldNode opts out the same way. The
			// "everything on the enemy team" wording was loop 0 of TASK-1541 and QA TASK-1546 B1 caught it.
			Row.Detail = FText::FromString(FString(
				TEXT("One swing damages every enemy unit, hero, building and castle within your melee reach and inside a ")
				TEXT("cone in front of you, and you can swing at most once per melee cooldown. ")
				TEXT("Damage per swing is worked out fresh each time — the base damage plus the Sharpened Blade ")
				TEXT("stacks. No friendly fire.\n\n")
				TEXT("The same physical click confirms placement, spell targeting and every group-order stage — ")
				TEXT("and when it does, melee is suppressed so the click does one thing only: ")
				TEXT("the swing is skipped entirely and does not even use up the cooldown.")));
		}

		{
			// R-06. Same hero-side fence as R-04.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Rally"), CategoryHero, TEXT("Rally"),
				TEXT("Give every nearby friendly unit a temporary speed boost."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Rally")) };
			Row.QwertyReferenceKeys = { EKeys::Q };
			// 704 §4 R-06 detail. Citations (T1): the buff loop = HeroCharacter.cpp:532-559; miners
			// included because AMinerUnit is a summoned-unit subclass = :535-536; the on-cooldown
			// broadcast = :525-529; RallyCooldown + OnRallyReady = :565, :568-572; the dead-hero
			// guard = :508-511.
			Row.Detail = FText::FromString(FString(
				TEXT("Speeds up every friendly summoned unit within the rally radius by the rally speed bonus for ")
				TEXT("the rally duration — units only, never the hero, never enemy units. Friendly miners are ")
				TEXT("included, since a miner is a kind of summoned unit.\n\n")
				TEXT("On cooldown the press does nothing, but it still tells the HUD how much cooldown is left ")
				TEXT("so the HUD can flash it; when the rally cooldown runs out, the HUD is told ")
				TEXT("that Rally is ready again. A dead hero cannot rally.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// ASummonedUnit, RallyRadius, RallySpeedBonus, RallyDuration, AMinerUnit,
			// OnRallyStateChanged(false, remaining), RallyCooldown, and OnRallyReady's (true, 0)
			// re-broadcast. Cited by symbol; the claims are unchanged.
		}

		// ─── CATEGORY: CARDS ───────────────────────────────────────────────────────────────

		{
			// R-07. ONE row carrying SIX actions and six reference keys (704's declared D-1) —
			// six near-identical rows would bury the fifteen commands around them, and the chip
			// composer renders all six from the live query.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.Play"), CategoryCards, TEXT("Play a card"),
				TEXT("Press a hand slot's number to play that card."), ESiegeInputLane::MappedAction);
			Row.Actions = {
				MakeActionRef(TEXT("IA_Card1")), MakeActionRef(TEXT("IA_Card2")), MakeActionRef(TEXT("IA_Card3")),
				MakeActionRef(TEXT("IA_Card4")), MakeActionRef(TEXT("IA_Card5")), MakeActionRef(TEXT("IA_Card6"))
			};
			Row.QwertyReferenceKeys = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
			// 704 §4 R-07 detail. Citations (T1): the slot-index payload = SiegePlayerController.cpp:
			// 482-489; type read from the data table = :858-868; placement + the M2 "card leaves the
			// hand only at confirm" ruling = :895-911, :900-903; the cancel keys = :682-698; targeting
			// = :913-919 and Gold Steal's instant resolve = :914-916,920; gold checked before type =
			// :879-889; the quiet ignore = :812-836; the slot-0 Footman fallback = :732-748.
			// ⚠️ THE `1` IN THE LAST SENTENCE IS DELIBERATELY LITERAL AND IT IS PROVABLY SAFE:
			// `KBD-§4` tables the 26 LETTERS and nothing else (SiegeKeyboardLayoutStatics.cpp:57-63),
			// so a DIGIT can never be retargeted by this system. ⛔ A `{Cards.Play}` token would be
			// WRONG here — this row's chip is all six keys, and the sentence is about the first one.
			Row.Detail = FText::FromString(FString(
				TEXT("Six keys, six hand slots, and each key carries its own slot's number. What happens next depends ")
				TEXT("on the card's type, read from the card list and never hard-wired into the game:\n\n")
				TEXT("• Unit / Building / Economy → placement mode. A ghost follows the cursor each frame and the ")
				TEXT("card leaves your hand only at confirm, so backing out costs nothing. Left-click confirms, ")
				TEXT("right-click or Escape cancels.\n\n")
				TEXT("• Spell → targeting mode, placement's twin, worked with the same clicks and keys and under the same rule: the card ")
				TEXT("leaves your hand only at the left-click confirm. The one exception is Gold Steal, which resolves instantly ")
				TEXT("with no reticle because it is a global effect.\n\n")
				TEXT("Gold is checked before the type is considered — if you are short of gold, that refusal comes ahead of any ")
				TEXT("refusal about the card's type. A press is quietly ignored — not refused — after match end, mid-placement or ")
				TEXT("mid-targeting. Key 1 has one leftover quirk: with the first hand slot empty it falls back to the ")
				TEXT("always-available Footman placement.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "bound with the slot index as the payload" (the BindAction
			// VarTypes overload that hands keys 2..6 their slot index), "read from the data table and
			// never from code" (the card's type comes from its DT_Cards row), "placement's sibling on the
			// same input surface", the quoted "card leaves the hand only at LMB confirm" law, "the
			// affordability refusal outranks the type refusal", "legacy quirk" and "hand slot 0" (key 1
			// is OnCard1Pressed, which plays slot 0 or falls back to the M1 Footman placement). The
			// `1` in "Key 1" stays literal for the reason given above. The claims are unchanged.
			// Jonathan's "all the controls with it": playing a card is only half the gesture — backing
			// out of the placement it starts is the other half, and it is a different row (704 R-10).
			Row.RelatedActionIds = { FName(TEXT("Cards.Cancel")), FName(TEXT("Cards.CursorHold")) };
		}

		{
			// R-08. Arguably the least discoverable control in the game: without it the HUD
			// cards are unclickable.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.CursorHold"), CategoryCards, TEXT("Show the mouse cursor"),
				TEXT("Hold to bring up the cursor so you can click the cards and buttons on your HUD."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_UICursor")) };
			Row.QwertyReferenceKeys = { EKeys::LeftAlt };
			// 704 §4 R-08 detail. Citations (T1): the hold + GameAndUI posture =
			// SiegePlayerController.cpp:756-771; the Completed+Canceled binding = :494-499; the guarded
			// release = :783-790; the compose-not-fight ruling = :775-780, and the one boolean
			// expression ⛔ CITED BY TEXT, ⛔ never again by line (TASK-1432 QA loop 1,
			// `qa/TASK-1433.md` WARN-2): `const bool bWantCursor = bInPlacementMode || ... ||
			// bUICursorHeld;`, the single statement inside
			// `ASiegePlayerController::ApplyCursorInputState`. ⛔ It ⛔ WAS cited as `:4357`, which
			// today lands in an unrelated formation comment — the rot this file's `§3` warns about.
			// ⚠️ SCOPE, DECLARED: ⛔ ONLY that anchor was re-measured in this pass. The line numbers
			// above it are ⛔ UNVERIFIED here; a full re-anchoring of this file's `Citations (T1)`
			// blocks is a real follow-up ~~and ⛔ nobody has boarded it~~. ⭐ TASK-1480 (a)
			// (2026-09-27): boarded, and answered with a FILE-LEVEL declaration at the TASK-707
			// transfer rules near the top of this file plus R-02's re-anchor by text, ⛔ not a sweep
			// of every block — so the numbers above are STILL unverified, now by a stated rule.
			Row.Detail = FText::FromString(FString(
				TEXT("A hold, not a toggle. Holding lets the game and the HUD both take input, with the cursor visible and camera ")
				TEXT("look suspended, so a click-drag on the HUD cannot nudge the camera.\n\n")
				TEXT("The release fires on a normal release and on a cancelled press, so the hold can never stick, however the key's ")
				TEXT("press is set up, and each hold is let go only once, so a double release cannot upset the ")
				TEXT("game's count of what is holding camera look off.\n\n")
				TEXT("Releasing while you are in placement mode leaves the cursor to placement mode — the two share ")
				TEXT("the cursor rather than fight over it.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// the GameAndUI input mode and the Completed / Canceled trigger events. The claims are unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "the action's trigger setup", "the release is guarded so a double
			// release cannot unbalance the ignore-look counter" (ClearUICursorHold acts only
			// `if (bUICursorHeld)`, so SetIgnoreLookInput(false) runs once per hold), "placement mode is
			// live" and "the two owners compose" (bWantCursor ORs the cursor owners in
			// ApplyCursorInputState). The claims are unchanged.
			// TASK-1585 (2026-09-28, qa/TASK-1575.md N2, taken): "a release only counts while the key is
			// really held" is now "each hold is let go only once". The guard is the game's hold record,
			// not the physical key: ClearUICursorHold acts only `if (bUICursorHeld)` and clears it, so
			// SetIgnoreLookInput(false) runs once per hold, and OnUICursorPressed sets the record only
			// when it is not already set. ClearUICursorHold has four other callers that can end a hold
			// while the key is still down, which is why "really held" overstated it. Same rule.
		}

		{
			// R-09. ⭐⭐ REWRITTEN IN PLACE BY TASK-821 (`CARDBAR-§9`), ⛔ NOT deleted and ⛔ NOT
			// replaced by a new row beside it. THE ROW ID IS LOAD-BEARING:
			// Tests/SiegeControlsHelpTest.cpp names "Cards.Discard" in its RequiredIds[] subset, so
			// a deletion or a rename turns the suite red. Rewriting keeps it green with zero test
			// edits, because that list asserts a REQUIRED SUBSET rather than an exact set.
			// ⚠️ MEASURED 2026-09-02 while doing this edit, and recorded because the law states a
			// SECOND reason that is not true as built: ⛔ NO other row carries "Cards.Discard" in
			// its RelatedActionIds (the 13 RelatedActionIds assignments in this file were read),
			// so the "a new id orphans its referrers" argument is currently hypothetical. The test
			// is the whole live reason — which is a stronger reason, not a weaker one.
			//
			// ⛔⛔ THE THREE SHIPPED FALSEHOODS THIS EDIT KILLS, named so a reader can check them off:
			//   1. the one-liner "Click a hand card's discard button…" — TASK-809 removes the six
			//      per-slot discard buttons, so the button it names stops existing;
			//   2. the PointerOnly lane and the "Mouse click" chip it renders — there is no pointer
			//      route AT ALL. Jonathan scrapped right-click on 2026-09-03 in his own words
			//      ("lets just scrap that right click feature to discard cards and keep it just to
			//      the 'H' key"), because right-click is already the placement-cancel gesture;
			//   3. the comment claiming no key binding exists — one does now: IA_DiscardAll, bound
			//      on ETriggerEvent::Started at SiegePlayerController.cpp:650-652.
			//
			// ⛔ LANE A, AND THE LANE *IS* THE LAYOUT STORY. The key is an Enhanced Input MAPPED
			// action, so the chip is READ BACK from the applied (already-retargeted) IMC_Hero
			// duplicate and the letter follows the player's layout with ⛔ zero conditional code
			// here. EKeys::H below is the FALLBACK and the test fixture ONLY
			// (SiegeControlsHelpWidget.h:188-194) — ⛔ never the displayed truth while the action
			// resolves, and ⛔ never typed into prose. On US-Dvorak this row's chip reads `D`
			// while the Cards.Play digits above it hold, and BOTH answers come out of the SAME
			// ResolveRowDisplayKeys call: the digits do not move because the 26-letter table has
			// no digit entry, ⛔ not because anything special-cases them.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.Discard"), CategoryCards, TEXT("Discard your whole hand"),
				TEXT("Bin every card in your hand at once and draw a full replacement — one flat fee, however many cards you were holding."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_DiscardAll")) };
			Row.QwertyReferenceKeys = { EKeys::H };
			// ⛔ bPointerOnly is deliberately NOT assigned any more: the struct default is false
			// (SiegeControlsHelpWidget.h:210-211) and every other MappedAction row leaves it alone.
			// The line that read `Row.bPointerOnly = true;` was falsehood 2 and it is GONE.
			//
			// TASK-821 detail. Citations (T1) — ⛔ every one of these was opened and read for this
			// edit, ⛔ none is copied from the task board:
			//   • the entry point, its guard ladder and its flat-fee rule = SiegePlayerController.h:
			//     547-585, implemented at SiegePlayerController.cpp:1172-1316;
			//   • the whole hand goes at once = the loop over DeckComponent->DiscardFromHand at
			//     :1287-1302, whose domain is the occupied-slot scan at :1237-1246;
			//   • ONE charge for the whole hand = the single SpendGold at :1269;
			//   • the net-zero refusal = :1269-1276 (SpendGold refuses below the fee with no change
			//     and no broadcast) reusing the shipped "Not enough gold" line at :1274;
			//   • the empty hand refuses BEFORE any gold moves = :1248-1255;
			//   • refused while placing / targeting = :1202-1220;
			//   • the replacement hand is drawn IMMEDIATELY = DeckComponent.cpp:206-240, where
			//     :231-232 push the card to the discard pile and redraw that slot in the SAME call;
			//   • the fee property itself = SiegePlayerController.h:1479-1496;
			//   • the key route = SiegePlayerController.cpp:235 (soft ref), :515 (resolve),
			//     :650-652 (bind) — mapped, ⛔ never a raw poll.
			// ⛔⛔ THE FEE IS NEVER TYPED — and since TASK-1541 (2026-09-27) it is not NAMED either: the
			// prose says "a single set amount" where it printed DiscardAllCost, and the suite's pin
			// moved with it to "charged once for the whole hand". DiscardAllCost's own header comment pins this
			// rule for this very row ("if it is ever shown to the player it is READ from here,
			// never typed") — a prose number rots the moment that line is retuned, which is the
			// M7.7 "in 400"/AoERadius-700 lesson. ⇒ there is ⛔ not one digit character in the
			// strings below, and the suite asserts exactly that for this row.
			// ⛔ AND THERE IS NO RIGHT-CLICK SENTENCE, nor the Alt-cursor caveat that would have
			// travelled with it: nothing in this gesture needs a cursor, and a help page teaching a
			// control that does not exist is the one defect this whole screen exists to remove.
			Row.Detail = FText::FromString(FString(
				TEXT("Pressing {Cards.Discard} bins every card in your hand in one gesture and deals a full ")
				TEXT("replacement hand immediately: each card goes to the discard pile and its slot redraws in ")
				TEXT("the same step, so you are never left holding an empty hand.\n\n")
				TEXT("The fee is a single set amount and it is charged once for the whole hand, flat. Dumping a ")
				TEXT("single dead card costs exactly what dumping a full hand costs, because this prices a hand ")
				TEXT("RESET rather than a per-card cycle — there is no longer any way to bin one card on its own ")
				TEXT("at any price.\n\n")
				TEXT("If you cannot afford it, nothing happens at all: no gold leaves you, no card moves, and you ")
				TEXT("get the same \"Not enough gold\" line every other card refusal uses. An empty hand is refused ")
				TEXT("before the fee is taken, so you can never pay to discard nothing.\n\n")
				TEXT("It is refused while you are placing a card or targeting a spell — binning the card you are ")
				TEXT("half-way through playing would hand the confirm a different one. Back out first with ")
				TEXT("{Cards.Cancel}, then discard.")));
			// TASK-1574 (2026-09-28, the register pass): "half-way through playing" replaces "half-way
			// through committing". The claim is unchanged, and the pinned "charged once for the whole
			// hand" sentence and both {…} tokens are untouched.
			// ⭐ THE RELATED CONTROL CHANGED WITH THE FEATURE, and the swap is the point:
			// Cards.CursorHold was listed because the discard USED to be a HUD button you had to
			// raise the cursor to click. There is no button and no cursor in this gesture any more,
			// so that block would now teach an irrelevance. Cards.Cancel is where the last
			// paragraph sends the player, and it is the only control this page still needs.
			// ⛔ This is DATA ON THIS ROW — Cards.Cancel's own page is ⛔ NOT touched (its
			// "right-click or Escape cancels" text is correct again now that right-click gained no
			// fifth meaning, and editing a correct page is how a help screen acquires its next
			// falsehood).
			Row.RelatedActionIds = { FName(TEXT("Cards.Cancel")) };
		}

		{
			// R-10. ONE action, TWO keys (IMC_Hero rows 10 and 11).
			// ⛔ `Escape` IS NAMED HERE AS DOCUMENTATION AND NOWHERE AS A HANDLER. It is the
			// shipped cancel gesture (AS-§6 A-2, a PERMANENT Jonathan ruling), it is not a
			// letter so `KBD-§4` can never move it, and this overlay never claims it.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.Cancel"), CategoryCards, TEXT("Cancel"),
				TEXT("Back out of whatever you are placing, targeting or circling — it never costs anything."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CancelPlace")) };
			Row.QwertyReferenceKeys = { EKeys::RightMouseButton, EKeys::Escape };
			// 704 §4 R-10 detail. Citations (T1): the one handler serving three modes =
			// SiegePlayerController.cpp:1065-1090; the raw double-cover = :501-506, :678-686; "nothing
			// is spent at cancel" = :900-903, :917-919; Escape as IMC_Hero row 11 =
			// handoffs/TASK-568-artist.md:32.
			// ⛔ T5 — 704's closing sentences ("Jonathan closed AS-§6 A-2 permanently: nothing may
			// absorb it. The controls overlay itself does not, and must not, claim it.") are an
			// instruction to THIS overlay's implementer, ⛔ not player prose. They are honoured in
			// code (zero key handlers in this file) and they live here rather than on screen.
			// ⚠️ `Escape` and "right-click" stay LITERAL and it is provably safe: neither is in the
			// 26-letter table (SiegeKeyboardLayoutStatics.cpp:57-63), so neither can ever move.
			Row.Detail = FText::FromString(FString(
				TEXT("One control, two keys, and it is the same gesture everywhere. It exits placement mode, exits ")
				TEXT("spell targeting, or aborts a group-order pick at any stage, leaving every existing group and ")
				TEXT("stance untouched.\n\n")
				TEXT("The same two keys are ALSO checked directly every frame, so cancelling still works even if the ")
				TEXT("game's setup for this control is missing — a deliberate backup, and a cancel that fires twice is harmless because ")
				TEXT("backing out of something you have already left changes nothing.\n\n")
				TEXT("Nothing has been spent at the moment you cancel: cards leave the hand only at confirm. ")
				TEXT("Escape is a built-in cancel key.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "One action", "polled directly every frame", "the input asset is
			// missing" (IA_CancelPlace), "the deliberate double-cover", "re-firing is harmless because the
			// exits are idempotent" and "a shipped, bound cancel key" (Escape is IMC_Hero's second
			// IA_CancelPlace key). The raw RMB / Escape poll is the backup for the bound action (the
			// "raw double-cover" in the citation block above). The claims are unchanged.
		}

		{
			// ⭐⭐ R-25 — THE STACK UPGRADE (TASK-823, the TOWER half of `CARDBAR-§9`; `STACK-§1`,
			// `STACK-§2`, `STACK-§5`). ⛔ A NEW ROW APPENDED AFTER THE SHIPPED CARD ROWS — ⛔ not
			// one shipped row above it is edited, re-indented or reordered by this task.
			//
			// ⛔ LANE B, AND THE LANE IS A MEASUREMENT RATHER THAN A GUESS: the gesture that
			// performs an upgrade is the placement CONFIRM, which is a RAW POLL of the left button
			// inside PlayerTick's placement branch — `if (WasInputKeyJustPressed(
			// EKeys::LeftMouseButton)) { TryConfirmPlacement(); }` — i.e. the identical lane and
			// the identical key as PickMode.Confirm. ⛔ No IA_* asset is named because none exists
			// on this path, and naming one would put a raw key on the Enhanced Input query lane.
			//
			// Citations (T1) — ⛔ EVERY ONE OF THESE WAS OPENED AND READ AT SOURCE FOR THIS ROW,
			// ⛔ none is taken from the task board and ⛔ none from another agent's handoff
			// (`HELP-§2` mechanism 3). Located by SYMBOL, ⛔ not by line number (`SC-§38`):
			//   • WHEN it turns blue = ASiegePlayerController::ResolvePlacementUpgradeState — seven
			//     ordered gates: a BUILDING card -> a live building under the cursor -> own team ->
			//     the SAME CardID -> CanStackHeight() -> gold -> Ready;
			//     ⚠️ TASK-1585 (2026-09-28): gate (5) read "CanScaleFootprint()" here. STACK-§8
			//     repointed it to ABuilding::CanStackHeight() on 2026-09-03 (the resolver's own
			//     "(5) THE EXCLUSION" comment), and every shipped class answers that one true,
			//     AClimbableTower included (STACK-§10) — so gate (5) turns nothing red today;
			//   • what blue DOES to the click = UpdatePlacementGhost's
			//     `case EPlacementUpgradeState::Ready:` arm (`bPlacementValid = true;`) and
			//     TryConfirmPlacement's `if (PlacementUpgradeState == EPlacementUpgradeState::Ready)
			//     { ConfirmStackUpgrade(*SiegeState); return; }` — it returns BEFORE every spawn
			//     rule, so nothing is built;
			//   • the colour = the ordered ternary `(PlacementUpgradeState == ...::Ready) ?
			//     UpgradeGhostColor : (bPlacementValid ? ValidGhostColor : InvalidGhostColor)`,
			//     written into the SAME "GhostColor" parameter as green and red;
			//   • the enemy case = gate (3)'s `HoveredBuilding->GetTeamId() != OwnTeam` -> None, so
			//     an enemy building keeps the SHIPPED clearance refusal and gains no new vocabulary;
			//   • the castle case = UpdatePlacementGhost's `Cast<ABuilding>(Hit.GetActor())`, which
			//     hands gate (2) a null for the castle (ACastle is class-disjoint); TASK-1585
			//     (2026-09-28): this read "gate (2)'s Cast", but the resolver takes an ABuilding
			//     pointer and never casts;
			//   • the HEIGHT series = ABuilding::StackHeightMultiplier(UpgradeCount, MaxMultiplier),
			//     whose whole answer is `FMath::Min(1 + Upgrades, Cap)` with Cap the ceiling the
			//     CALLER hands in: the building's OWN MaxStackHeightMultiplier, per class since
			//     STACK-§10 cl. 2 (read through GetMaxStackHeightMultiplier() at the cap notice)
			//     ⇒ ⭐ ADDITIVE and SATURATING, ⛔ never doubling. TASK-1585 (2026-09-28): this read
			//     "Cap read off the CDO's MaxStackHeightMultiplier", which was the one-argument form
			//     that read GetDefault<ABuilding>() until 2026-09-03 (the function's own comment);
			//   • the HEALTH series = ABuilding::StackHealthMultiplier, repeated multiplication by
			//     the CDO's StackHealthStep with ⛔ NO ceiling term in the loop;
			//   • Z only, X/Y inherited = ApplyStackUpgrade's `Scale.Z = AuthoredHeightScaleZ *
			//     StackHeightMultiplier(StackUpgradeCount, MaxStackHeightMultiplier);` with X and Y
			//     untouched (`J-4`) — TASK-1585 (2026-09-28): the second argument, the instance's
			//     own ceiling, was missing from this quotation;
			//   • GRANTED, ⛔ never healed = the same function's `MaxHP = OldMaxHP *
			//     StackHealthMultiplier(1);` followed by `CurrentHP += (MaxHP - OldMaxHP);`;
			//   • the cap notice = ConfirmStackUpgrade's `if (ABuilding::StackHeightMultiplier(
			//     UpgradesAfter, TargetHeightCap) <= ABuilding::StackHeightMultiplier(UpgradesBefore,
			//     TargetHeightCap))` -> BroadcastRefusal(StackHeightCapNoticeText()), with
			//     `const int32 TargetHeightCap = Target->GetMaxStackHeightMultiplier();` — at CONFIRM,
			//     ⛔ never per frame. TASK-1585 (2026-09-28): the quotation was one-argument;
			//   • the COST = PendingCost, taken from the card's own data-table row at
			//     EnterPlacementMode (`PendingCost = Row->Cost;`) and spent by ConfirmStackUpgrade's
			//     `SiegeState.SpendGold(PendingCost)` (`J-2`: ⛔ never a literal);
			//   • the two refusals = TryConfirmPlacement's `case EPlacementInvalidReason::Upgrade:`,
			//     choosing between the shipped "Not enough gold" line and
			//     StackNotStackableRefusalText() ("That building cannot be stacked"). ⚠️ TASK-1585
			//     (2026-09-28): only the gold one is reachable. The second is the NotStackable
			//     state's, whose doc in SiegePlayerController.h reads "NO SHIPPED CLASS PRODUCES
			//     THIS STATE", so the page teaches the gold refusal alone. Both refusals `return`
			//     before any gold moves and without ExitPlacementMode (the `if (!bPlacementValid)`
			//     branch's "refuse, spend NOTHING, STAY in placement mode");
			//   • the card leaves the hand at confirm = ConfirmStackUpgrade's
			//     `DeckComponent->ConfirmPlayFromHand(PendingHandSlot)` block.
			//
			// ⛔⛔ THERE IS NO STACK EXCLUSION TO TEACH, AND THIS PAGE TEACHES NONE (TASK-1585,
			// 2026-09-28). This paragraph used to say the exclusion was taught as a behaviour because
			// "the shipped path asks ABuilding::CanScaleFootprint() (AClimbableTower overrides it
			// false)". That stopped being true on 2026-09-03: STACK-§8 split the height question
			// into its own virtual, ABuilding::CanStackHeight(), which ALL THREE stack gates ask
			// (the resolver's gate (5), ConfirmStackUpgrade and ApplyStackUpgrade), and
			// AClimbableTower answers it TRUE with a per-class ceiling (STACK-§10, `J-13`,
			// TASK-944). CanScaleFootprint() is now the placement WHEEL's predicate only (the
			// Cards.PlacementResize row). ⇒ the "REFUSES TO BE STACKED" paragraph, and the player's
			// reason it gave ("it is the one you climb"), are DELETED by TASK-1585. The law that
			// survives is the code's: a CardID string compare on the stack path is an automatic QA
			// fail (STACK-§2), and ⛔ no building is named here. ⭐ TASK-1576 (2026-09-28): 🧑 Jonathan
			// answered Q-STACK-CAP-2026-09-28 with A, "show the numbers" (TASK-1589), so the page is
			// to show each building type's own height limit. ⛔ It does NOT show them yet, and the
			// reason is access, not choice: each value must be read from the class the game really
			// places for that card, found by the game's own card-to-class resolution, and that
			// resolution (ASiegePlayerController::ResolveCardActorClass with IsBuildingCard, plus the
			// BuildingEconomyCardIDs list it reads) is private to the controller. Reading it from here
			// needs a new accessor in an owner file, which TASK-1576's fence forbids (its spec (3) and
			// (8)), and a copy of the path rule here would be a second resolver that could drift. ⇒ the
			// per-building limits are OWED on his answer A (handoffs/TASK-1576-programmer.md), and
			// ⛔ this row still states no per-building limit. (Until TASK-1576 this sentence read
			// "Whether the page shows each building's height limit as a number is Jonathan's open
			// question Q-STACK-CAP-2026-09-28, owned by TASK-1576".)
			//
			// ⛔ NO TUNABLE'S VALUE IS TYPED — MaxStackHeightMultiplier and StackHealthStep were
			// NAMED here, as PickMode.Resize named its three radii (the M7.7 "in 400" lesson), until
			// TASK-1541 (2026-09-27) put both in player words: "a set maximum multiple" and "a set
			// factor" (loop 1: "factor", not "step", because StackHealthStep compounds as a power of the
			// upgrade count, and a step reads as an addition). ⚠️ The jargon cost F-3 flagged is paid by
			// the wording, still without inventing a number.
			// ⭐ TASK-1576 (2026-09-28): the health factor is now SHOWN, and it is still not typed. The
			// prose carries the `{#StackUpgrade.HealthFactor}` number token where it said "a set
			// factor", and ComposeDetailForDisplay replaces it with ABuilding::StackHealthMultiplier at
			// one upgrade, read when the page is composed: the factor ApplyStackUpgrade applies on
			// every upgrade, read off ABuilding's own class default for every building (the
			// derived-number block near the top of this file says why that is the object the game
			// uses). It renders "multiplies the building's maximum health by 1.5, compounding" at
			// today's value. The height half keeps "a set maximum multiple" (the per-building limits
			// are owed, above). Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.StackUpgrade"), CategoryCards, TEXT("Stack a tower taller"),
				TEXT("While you are placing a building, hover one you already own of the same card: the outline turns blue and the click makes that one taller instead of building a new one."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::LeftMouseButton };
			Row.Detail = FText::FromString(FString(
				TEXT("Hover one of your OWN buildings while holding the card that built it and the placement outline ")
				TEXT("turns BLUE. Blue means the click will UPGRADE that building instead of putting a new one down: ")
				TEXT("nothing is built and nothing moves.\n\n")
				TEXT("WHEN IT TURNS BLUE — all of these have to be true, and if any one is missing the outline stays ")
				TEXT("green or red exactly as it always has. You are placing a BUILDING card; the thing under the ")
				TEXT("cursor is a building rather than open ground, a unit or your castle; it belongs to YOU; and it ")
				TEXT("was built from the SAME card you are holding. An enemy building never turns blue — it stays red ")
				TEXT("on the ordinary too-close-to-another-building rule, with the message it has always given.\n\n")
				TEXT("WHAT AN UPGRADE BUYS. Height: each upgrade adds one more copy of the building's ORIGINAL ")
				TEXT("height, and it stops at a set maximum multiple of that original. Its width and length are ")
				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by {#StackUpgrade.HealthFactor}, ")
				TEXT("compounding, and that half has no ceiling at all — it keeps climbing after the height has ")
				TEXT("stopped. The health is GRANTED rather than repaired: a damaged tower stays exactly as damaged, ")
				TEXT("it is simply damaged out of a bigger pool.\n\n")
				TEXT("AT THE HEIGHT LIMIT the click still buys health, the outline stays blue, and one line on the ")
				TEXT("HUD tells you the height has maxed out the moment you confirm — the click never quietly stops ")
				TEXT("doing what it did the time before.\n\n")
				TEXT("WHAT IT COSTS is the card's own cost, the same gold placing it would have cost, and the card ")
				TEXT("leaves your hand at confirm exactly as a placement does. If you cannot afford it the outline is ")
				TEXT("RED rather than blue and the click answers with the same \"Not enough gold\" line every other ")
				TEXT("card refusal uses. Blue never promises a click that will be refused.\n\n")
				TEXT("Being refused for gold costs nothing and leaves you in placement mode. Back out entirely with ")
				TEXT("{Cards.Cancel}.\n\n")
				TEXT("The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an ")
				TEXT("upgrade keeps the building's existing width and length.")));
			// TASK-1574 (2026-09-28, the register pass): "the building's exact shape" replaces "the shape
			// of the mesh". The claim is unchanged, and no number is typed. (That sentence was in the
			// paragraph TASK-1585 deleted; see below.)
			// ⚠️ SC-§101, REPORTED AND NOT FIXED HERE: the paragraph that sentence sits in ("ONE KIND OF
			// BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB" … "Hovering one shows RED
			// with 'That building cannot be stacked'") is FALSE at source. AClimbableTower answers
			// CanStackHeight() true (its "THE HEIGHT (Z) ANSWER — **TRUE**" doc), and the NotStackable
			// state's doc in SiegePlayerController.h reads "NO SHIPPED CLASS PRODUCES THIS STATE". The
			// truth fix is TASK-973 (boarded 2026-09-03, never dispatched). A register pass keeps a false
			// sentence's claim; it does not repair it.
			// ⭐ FIXED BY TASK-1585 (2026-09-28), which superseded TASK-973. The code was right and the
			// text was wrong (`J-13`, TASK-944, STACK-§10), so the paragraph is DELETED, whole, and
			// nothing replaces it: no building is singled out and no height limit is stated here (the
			// cap's number, if any, is TASK-1576's on 🧑 Q-STACK-CAP-2026-09-28; ⭐ TASK-1576,
			// 2026-09-28: he answered A, "show the numbers", and the per-building limits are OWED
			// rather than shown, because the class resolution they must be read through is private to
			// ASiegePlayerController; see the TASK-1576 note in the citation block above). The sentence after
			// it read "A refusal of either kind costs nothing and leaves you in placement mode, so
			// another building — or another patch of ground — still works." With the stack refusal
			// gone, "either kind" pointed at nothing; and "another building or another patch of
			// ground still works" is FALSE for the gold refusal that remains, because an upgrade of
			// any other building of this card, and a placement on open ground, cost the same
			// PendingCost. It now reads "Being refused for gold costs nothing and leaves you in
			// placement mode", which is the `if (!bPlacementValid)` branch of TryConfirmPlacement
			// (refuses before any gold moves and returns without ExitPlacementMode), and the same
			// holds for ConfirmStackUpgrade's SpendGold refusal. The {Cards.Cancel} token is kept.
			// ⛔ Pinned by test 19 (NoPageTeachesARefutedStackOrWheelRule) in the help test file.
			// Jonathan's "all the controls with it": getting INTO placement is one row, sizing what
			// you are about to put down is another, and backing out is a third. ⛔ These are this
			// row's OWN outbound edges (`HELP-§7`) — ⛔ no other row's RelatedActionIds is touched
			// by this task, and every id here resolves to a real row.
			Row.RelatedActionIds = {
				FName(TEXT("Cards.Play")), FName(TEXT("Cards.PlacementResize")), FName(TEXT("Cards.Cancel"))
			};
		}

		{
			// ⭐⭐ R-26 — THE PLACEMENT FOOTPRINT WHEEL (TASK-823; `STACK-§4`, `MARK-§4` as amended).
			//
			// ⛔ LANE B: a RAW POLL of the wheel — `WasInputKeyJustPressed(EKeys::MouseScrollUp)` /
			// `...MouseScrollDown` inside ASiegePlayerController::ApplyPlacementFootprintWheel, the
			// same mechanism and the same keys as PickMode.Resize. ⛔ NO new InputAction exists for
			// it and none may be added (`MARK-§4`).
			//
			// Citations (T1) — read at SOURCE, located by SYMBOL (`SC-§38`):
			//   • the gesture and WHERE it lives = the `ApplyPlacementFootprintWheel();` call in
			//     PlayerTick's PLACEMENT branch, reached only past `if (!bInPlacementMode) {
			//     return; }` — the group-pick, targeting and war-map branches all `return` above it,
			//     so at most one of the wheel's three meanings can run in a frame;
			//   • one notch = one step, clamped = ASiegePlayerController::StepPlacementFootprintScale
			//     (`SafeCurrent + SafeStep * NotchDelta`, then `FMath::Clamp(..., SafeMin, SafeMax)`)
			//     driven by PlacementFootprintWheelStep / PlacementFootprintMin /
			//     PlacementFootprintMax;
			//   • ⛔ NO SHRINKING is a RULING, not a rounding = PlacementFootprintMin's own header
			//     comment (`J-3`), which also records that shrinking would let a building hide in a
			//     gap its mesh was never meant to fit;
			//   • WIDTH AND LENGTH ONLY = ASiegePlayerController::MakePlacementFootprintScale3D,
			//     which returns `FVector(SafeScale, SafeScale, 1.f)` — the Z axis belongs to the
			//     stack upgrade and the wheel never writes it;
			//   • every session starts at the floor = EnterPlacementMode's
			//     `PlacementFootprintScale = PlacementFootprintMin;` (⛔ never a literal);
			//   • the ghost and the spawned building are ONE value = UpdatePlacementGhost's
			//     `GhostActor->SetActorScale3D(MakePlacementFootprintScale3D(PlacementFootprintScale));`
			//     and TryConfirmPlacement's `const FTransform SpawnTransform(FRotator::ZeroRotator,
			//     PlacementLocation, MakePlacementFootprintScale3D(PlacementFootprintScale));`;
			//   • the room checks measure the SCALED ghost = TryGetPlacementFootprintRadius's
			//     CalcBounds through the component's live transform (`STACK-§6`), consumed by the
			//     clearance, obstacle and own-unit gates;
			//   • units and spells ignore it = EnterPlacementMode's `bPendingCardCanScaleFootprint =
			//     bPendingIsBuilding && CanCardActorScaleFootprint(...)`;
			//   • the climbable building is INERT rather than refused per notch =
			//     ApplyPlacementFootprintWheel's leading `if (!bPendingCardCanScaleFootprint) {
			//     return; }`. ⚠️ TASK-1585 (2026-09-28): this also cited that function's comment,
			//     "the refusal already has a voice ... on the click", as the source of the page's
			//     "that refusal speaks once, at the click". ⛔ Both are stale: the voice that comment
			//     names is the RED ghost plus "That building cannot be stacked", which is the
			//     NotStackable state that no shipped class produces since STACK-§8/§10 (2026-09-03),
			//     and it was a STACK refusal, never a footprint one. A climbable card placed on open
			//     ground is never refused at the click for its size; TryConfirmPlacement spawns it
			//     at MakePlacementFootprintScale3D(PlacementFootprintScale), the floor the wheel
			//     never moved.
			//     ⇒ the clause is deleted from the prose below. The stale comment in
			//     ApplyPlacementFootprintWheel is ⛔ outside TASK-1585's fence and is reported in
			//     handoffs/TASK-1585-programmer.md, not edited.
			//
			// ⭐⭐ THE DISAMBIGUATION `STACK-§4` DEMANDS IS IN THE FIRST PARAGRAPH, ⛔ not buried:
			// the game now has THREE wheel meanings on one physical gesture, and `HELP-§2`'s
			// standard is that a help screen conflating them is worse than no help screen. ⇒ this
			// page says WHICH MODE it belongs to before it says what it does, and names the other
			// two. ⛔ NO tunable value is typed.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Cards.PlacementResize"), CategoryCards, TEXT("Resize what you are placing"),
				TEXT("While a building's placement outline is up, scroll the mouse wheel to make it wider and longer — the wheel only does this during placement."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::MouseScrollUp, EKeys::MouseScrollDown };
			Row.Detail = FText::FromString(FString(
				TEXT("This is the mouse wheel's PLACEMENT meaning, and the game has three different ones. Here it ")
				TEXT("changes the SIZE of the building you are about to put down, and it does nothing at all unless a ")
				TEXT("building's placement outline is on screen. While you are drawing an army order the same wheel ")
				TEXT("resizes the order circles; on the war map it resizes one of your own map circles. One mode at a ")
				TEXT("time, never two.\n\n")
				TEXT("One notch changes the size by a set step, and the size always stays between a set floor and a set ")
				TEXT("ceiling. It will not go below the floor: you can make a building bigger than it ")
				TEXT("was drawn, never smaller. That is a decision rather than an oversight — shrinking would let a ")
				TEXT("building hide in a gap its shape was never meant to fit, and it would shrink the ground the ")
				TEXT("building blocks along with what you see.\n\n")
				TEXT("IT CHANGES WIDTH AND LENGTH ONLY. Height belongs to the stack upgrade, and the wheel never ")
				TEXT("touches it.\n\n")
				TEXT("Every placement starts back at the floor, so a size you dialled in for one building does not ")
				TEXT("follow you to the next card.\n\n")
				TEXT("WHAT YOU SEE IS WHAT YOU GET. The outline and the building that appears are the same size, and ")
				TEXT("the room checks — the gap from other buildings, from obstacles, and from your own units ")
				TEXT("standing on the spot — are all measured against the size ON SCREEN. A building you have ")
				TEXT("scrolled up is genuinely harder to fit and will be refused for want of room rather than ")
				TEXT("appearing on top of something.\n\n")
				TEXT("Unit and spell cards ignore the wheel completely. So does the one building you can climb: its ")
				TEXT("ladder is fixed to the building's exact shape, so it cannot be resized, and the wheel is simply dead ")
				TEXT("on it rather than nagging you once per notch.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// PlacementFootprintWheelStep, PlacementFootprintMin and PlacementFootprintMax, all three
			// cited by symbol above. Still no value typed.
			// TASK-1574 (2026-09-28, the register pass): "along with what you see" replaces "along with
			// the art", and "the building's exact shape" replaces "the shape of the mesh" (the X/Y
			// refusal: AClimbableTower answers CanScaleFootprint() false because a sideways scale moves
			// its ladder sockets off the climb line, per that override's own doc). The claims are
			// unchanged, and no number is typed.
			// TASK-1585 (2026-09-28, the (F) stack-truth census): the last sentence ended "…rather than
			// nagging you once per notch — that refusal speaks once, at the click." The trailing
			// clause is deleted because no refusal speaks at the click for a climbable building's
			// size (the citation block above says why). Everything before it is unchanged.
			// ⭐ THE TWO OTHER WHEEL MEANINGS, LINKED RATHER THAN RE-DESCRIBED (`HELP-§2`: one
			// definition, two renderings), plus the other thing a placement outline can do. ⛔ This
			// row's OWN outbound edges only (`HELP-§7`).
			Row.RelatedActionIds = {
				FName(TEXT("Cards.StackUpgrade")), FName(TEXT("PickMode.Resize")), FName(TEXT("Interface.MapMarks"))
			};
		}

		// ─── CATEGORY: ORDERS ──────────────────────────────────────────────────────────────

		{
			// R-11.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Orders.Attack"), CategoryOrders, TEXT("Attack (army order)"),
				TEXT("Send your whole army at the enemy castle, right now — no circle to draw."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CmdAttack")) };
			Row.QwertyReferenceKeys = { EKeys::T };
			// 704 §4 R-11 detail. Citations (T1): the handler + shared 'charge' path =
			// SiegePlayerController.cpp:1153-1162 → ApplyArmyWideStance at :1122-1151; the
			// release-BEFORE-latch ordering = :1130-1134; the Attack semantics = UnitCommand.h:19-21;
			// ignored after match end = :1144-1147; the latch + bHasIssuedCommand =
			// SiegePlayerController.cpp:1106-1114 and UnitCommand.h:39-43.
			Row.Detail = FText::FromString(FString(
				TEXT("Immediate, army-wide, and it releases every standing group order. It runs as one fixed ")
				TEXT("sequence, shared with the assistant's charge: abort any pick you are part-way through → clear all unit ")
				TEXT("groups → lock in the stance.\n\n")
				TEXT("The release comes BEFORE the stance is locked in, and that order matters — units check the ")
				TEXT("stance again at their next decision, so releasing afterwards would let a group about to be ")
				TEXT("destroyed go back to its station for one more decision.\n\n")
				TEXT("Under Attack, units march the enemy castle, clearing defenders inside the enemy spawn box ")
				TEXT("first; local self-defence aggro is unchanged. Ignored after match end.\n\n")
				TEXT("The stance is locked in — it stays until replaced — and the game records your first command ")
				TEXT("and keeps that record for the rest of the match, so the old behaviour from before your first command never ")
				TEXT("returns mid-match.")));
			// TASK-1541 (2026-09-27): player words replace the code name this prose used to print —
			// bHasIssuedCommand, cited above. The claim is unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "The sequence is one function" (ApplyArmyWideStance, shared with the
			// assistant's charge), "in-flight pick", "latch the stance" / "the latch" / "latched", "that
			// ordering is load-bearing", "re-read the stance on their next state tick", "re-assert its
			// station for one tick" and "the pre-command legacy behaviour" (the behaviour before
			// bHasIssuedCommand flips). "Decision" is the word the Defend page already uses for the
			// unit's periodic state update. The claims are unchanged.
		}

		{
			// R-12.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Orders.Defend"), CategoryOrders, TEXT("Defend (army order)"),
				TEXT("Pull your whole army back to your own castle and fight only what comes to it."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CmdDefend")) };
			Row.QwertyReferenceKeys = { EKeys::E };
			// 704 §4 R-12 detail. Citations (T1): the mirror handler = SiegePlayerController.cpp:
			// 1196-1199 through the same ApplyArmyWideStance at :1122-1151; the Defend semantics and
			// the CHANGED band = UnitCommand.h:21-22 and :23-31 (ResolveDefendEngagementRadius is the
			// only supported reader).
			// ⛔ T5 — 704's two implementer sentences are held here rather than shown to a player:
			// "the header says so on purpose", and "Reading it as a centre radius is the defect that
			// made Defend acquire nobody at the 9× castle" (UnitCommand.h:28-31). Both are notes to
			// whoever next reads DefendRadius, ⛔ not facts a player can act on.
			Row.Detail = FText::FromString(FString(
				TEXT("The exact mirror of Attack — same check, same two steps, same order, same stance locked in at the end, ")
				TEXT("all done by the very same sequence. Units fall back toward your own castle and engage only ")
				TEXT("enemies inside the defend band.\n\n")
				TEXT("What \"the band\" means CHANGED: the defend range is no longer a disc centred on the castle — it ")
				TEXT("is the band past the castle's wall face, and how far out a unit will pick a target is worked out at every ")
				TEXT("decision from how far the castle's walls actually reach from its centre, plus that band, by ")
				TEXT("each unit for itself.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// DefendRadius and ASummonedUnit::ResolveDefendEngagementRadius, both cited above. The
			// claim is unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "same guard, same two calls, … same final latch, through the same one
			// implementation" (the match-end guard, the abort-pick and clear-groups calls and the latch in
			// ApplyArmyWideStance), "the acquisition radius is derived" and "the castle's live colliding
			// half-width" (the castle's collision half-width, read live, in
			// ResolveDefendEngagementRadius). The claims are unchanged.
		}

		{
			// R-13. The leash is what makes this HOLD and not AMBUSH (SummonedUnit.cpp:1778-1794).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Orders.Hold"), CategoryOrders, TEXT("Hold"),
				TEXT("Pick a squad, give it a patch of ground to stand on and a patch to fight over — it disengages the moment its target leaves both."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CmdHold")) };
			Row.QwertyReferenceKeys = { EKeys::R };
			// 704 §4 R-13 detail. Citations (T1): the handler = SiegePlayerController.cpp:1165-1170;
			// the per-tick priority ladder = SummonedUnit.cpp:1757-1767, ladder at :1808-1864; the
			// HOLD leash (drop on leaving both zones) = :1778-1794; dead targets dropped for both
			// types = :1770-1774; "the zones ARE the leash" = :1766-1767; target stickiness =
			// :1762-1766, :1808-1810; the HOLD-only monotone upgrade = :1795-1805; the sunflower
			// station spread = :1851-1856.
			// T1 — 704's "described in R-16..R-18" is a HANDOFF cross-reference; on screen those three
			// rows are rendered underneath this page by RelatedActionIds, so the reference is the
			// page itself.
			Row.Detail = FText::FromString(FString(
				TEXT("Opens the three-circle pick — the three controls listed underneath this page. At the end you ")
				TEXT("have a group with a position zone and an attack zone, and its units run a strict priority ")
				TEXT("ladder at every decision: enemies in the attack zone first, else enemies in the position zone, ")
				TEXT("else walk to the unit's own station inside the position zone and wait.\n\n")
				TEXT("The leash is what makes this HOLD and not AMBUSH. A target that is alive but has left both ")
				TEXT("zones is dropped at that same decision — the unit disengages and returns toward its station. A dead ")
				TEXT("target is dropped for both orders.\n\n")
				TEXT("There is deliberately no separate leash range: the zones are the leash. Targets are sticky — ")
				TEXT("a live target still inside the zones is kept, and a new target is looked for only when the unit has none, which is ")
				TEXT("what stops the goal flipping back and forth. A single one-way upgrade exists, HOLD only: a ")
				TEXT("position-tier target yields to an attack-zone enemy the moment one appears, and never the ")
				TEXT("other way, so the two tiers cannot keep swapping. Stations are spread out in a spiral so ")
				TEXT("the squad does not mill at one point.\n\n")
				// ⭐⭐ T4 — 704 typed the letters `T` and `E` here. US-Dvorak moves them to `Y` and `.`,
				// so a typed letter would be the exact `HELP-§1` defect. The tokens resolve to the two
				// orders' OWN derived chips at compose time (ResolveDetailTokens).
				TEXT("Pressing {Orders.Attack} or {Orders.Defend} destroys this group.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "every state tick" / "that tick" / "every tick" (the unit's periodic
			// state update, called a "decision" as on the Defend page), "for both types" (the Hold and
			// Ambush group types), "a live, zone-valid target", "re-acquisition runs only when
			// target-less", "One monotone upgrade", "oscillate" and "a sunflower offset" (the golden-angle
			// station spread cited above). The claims are unchanged, and the {…} tokens are untouched.
			// ⭐ JONATHAN'S NAMED CONTENT, ANSWERED ON THIS PAGE: what the first, second and third
			// circles do (PickMode.Confirm), how to resize them (PickMode.Resize), how to exit the
			// command (PickMode.Cancel) — rendered from those rows' OWN text, so there is exactly one
			// copy of it in the whole feature (`HELP-§2`).
			Row.RelatedActionIds = {
				FName(TEXT("PickMode.Confirm")), FName(TEXT("PickMode.Resize")), FName(TEXT("PickMode.Cancel"))
			};
		}

		{
			// R-14. ⭐ THE ROW WHOSE KEY MOVES ON DVORAK — the QWERTY `F` position yields `U`
			// (Jonathan's own worked example, and the Tests file's keystone assertion).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Orders.Ambush"), CategoryOrders, TEXT("Ambush"),
				TEXT("Set a squad to wait at a position and then chase to the kill anything that enters its attack zone, even after it leaves."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CmdAmbush")) };
			Row.QwertyReferenceKeys = { EKeys::F };
			// 704 §4 R-14 detail. Citations (T1): the handler = SiegePlayerController.cpp:1172-1177
			// and the definitive semantics header = UnitCommand.h:56-62; ⭐ THE LEASH EXEMPTION, at
			// the line that implements it = SummonedUnit.cpp:1778 (the drop-test is wrapped in
			// `if (CurrentTarget && Group.Type == ESiegeGroupCommandType::Hold)`) with the intent
			// stated verbatim at :1788-1792; identical acquisition through the same two tiers =
			// :1808-1810; dead targets still dropped for both = :1770-1774; the HOLD-only monotone
			// upgrade gated by the same Type == Hold test = :1795-1805; the miner collapse and
			// Jonathan's own words = MinerUnit.h:26, :45, :200.
			// ⛔ T5 — the C++ fragment above is where the difference LIVES, but a raw `if (...)`
			// condition on a player's screen is noise; it is cited here and the sentence keeps the
			// behaviour it describes. T5 repair: "skips that whole block" → "skips the zone
			// drop-test entirely", because "that block" no longer has an antecedent on screen.
			Row.Detail = FText::FromString(FString(
				TEXT("Identical to HOLD in every respect — same three-circle pick, same priority ladder, same ")
				TEXT("stations — except the leash.\n\n")
				TEXT("The difference: while it has a live target, AMBUSH never drops it for leaving the zones. It keeps ")
				TEXT("the target until the kill, then the ladder resumes. Ambush picks its targets through exactly the same ")
				TEXT("two tiers; that exception only affects when a target it already has is let go.\n\n")
				TEXT("A dead target is still dropped, for both orders. The single one-way position→attack upgrade ")
				TEXT("is HOLD-only and does not happen for Ambush.\n\n")
				TEXT("For miners the two orders collapse into one behaviour: \"Ambush\" is the same thing as ")
				TEXT("\"hold\" for a miner.\n\n")
				// ⭐⭐ T4 — see R-13. Same two orders, same reason.
				TEXT("Pressing {Orders.Attack} or {Orders.Defend} destroys this group.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "skips the zone drop-test entirely while a live target exists" (the
			// T5 repair quoted above now reads "while it has a live target, AMBUSH never drops it for
			// leaving the zones"; the drop-test is the Hold-gated block cited above), "acquires",
			// "the exemption governs only when an already-held target is released", "for both types",
			// "monotone" and "does not run". The claims are unchanged, and the {…} tokens that test 10
			// derives through are untouched.
			// ⭐ JONATHAN'S OWN WORKED EXAMPLE IS THIS PAGE. His ask names exactly these three:
			// "what the first, second, and third circles do, how to resize them, how to exit the
			// command" — and each is a real registry row with a live-derived chip.
			Row.RelatedActionIds = {
				FName(TEXT("PickMode.Confirm")), FName(TEXT("PickMode.Resize")), FName(TEXT("PickMode.Cancel"))
			};
		}

		{
			// R-15. ⭐ The row that carries the core-loop fact the help screen exists to teach —
			// units spawn already following and nothing player-side auto-engages any more
			// (SummonedUnit.cpp:1286-1291). TASK-707's detail page owns that sentence.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Orders.Follow"), CategoryOrders, TEXT("Follow"),
				TEXT("Circle units to make them escort you — they walk with you and will not fight."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_CmdFollow")) };
			Row.QwertyReferenceKeys = { EKeys::C };
			// 704 §4 R-15 detail. Citations (T1): the one-stage flow = SiegePlayerController.cpp:
			// 1179-1193, :2893-2901, with the fail-loud tripwire at :2835-2847; Jonathan's quoted
			// reason = UnitCommand.h:66-70; the separate stage-1 prompt = SiegePlayerController.cpp:
			// 2744-2750; "the anchor is you" = UnitCommand.h:71-73 and SiegePlayerController.cpp:
			// 3325-3329; the live, never-cached station = SummonedUnit.cpp:1912-1921, :1937-1940;
			// followers never attack = :1886-1898; the Cleric still heals = :1900-1910; the
			// hero-death ruling = :1923-1935; Follow does not release other groups =
			// SiegePlayerController.cpp:1186-1189, :3293-3302.
			// ⭐⭐ THE SPAWN DEFAULT = SummonedUnit.cpp:1286-1291, call site :1277; the eligibility
			// predicate that spares Siege units and the enemy side = :1290-1291, :1304-1311;
			// unconditional enrolment = :1293-1295; the miner exception = MinerUnit.h:496-502.
			// ⛔ T5 — 704's "the core-loop fact this help screen exists to teach" is an instruction
			// about THIS screen's purpose, not a fact about the game; it is honoured by the sentence
			// being here at all.
			Row.Detail = FText::FromString(FString(
				TEXT("ONE circle, one stage. Deliberately not the three-stage flow with two stages switched off: ")
				TEXT("the pick opens at Select and confirms there, and the later stages can never be ")
				TEXT("reached. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ")
				TEXT("is just the circle used to indicate what units follow\". Its stage-1 prompt is worded ")
				TEXT("separately so it never promises a second stage.\n\n")
				TEXT("The anchor is you, not a piece of ground: a follow group stores no circle sizes, no centre points ")
				TEXT("and no markers on the ground, and the select circle is destroyed at confirm rather than left on the ")
				TEXT("map. Each unit's station is wherever you are right now plus that unit's own spot in the spiral, worked out fresh ")
				TEXT("moment to moment and never stored — which is exactly why following carries on by itself after your hero respawns.\n\n")
				TEXT("Followers never attack. Their target is cleared constantly, and following never uses any ")
				TEXT("of the target-finding or attacking steps. A following Cleric still heals — healing is not attacking, ")
				TEXT("and an escorting medic is the point of a support unit told to follow.\n\n")
				TEXT("If you die, followers hold position — no target, no march, no attack — and resume the instant ")
				TEXT("you have a living hero again, including a brand-new one after respawn.\n\n")
				// ⭐⭐ T4 — 704 typed "`T`/`E`" here too.
				TEXT("Unlike {Orders.Attack} and {Orders.Defend}, Follow does not release your other groups: it ")
				TEXT("adds the circled units to the one follow group, stealing them out of any Hold/Ambush group, ")
				TEXT("and units you did not circle keep their orders.\n\n")
				TEXT("THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, whichever ")
				TEXT("way it spawns, and nothing on your side starts a fight by itself any more — you personally order every ")
				TEXT("fight. Siege units (Ogre/Sapper) and the whole enemy side are unaffected, because the ")
				TEXT("rule for who may follow leaves them out. Enrolment is unconditional: a unit spawned after you ")
				TEXT("pressed {Orders.Attack} still spawns following — reinforcements do not inherit your last ")
				TEXT("order. Miners are the one exception and spawn mining.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "enters at Select", "structurally unreachable", "carries zero radii,
			// zero centres and no marker decals", "your live position plus that unit's own sunflower
			// offset, resolved every tick and never cached", "makes hero respawn work for free", "The
			// target is forced null every tick and the follow body calls none of the acquire/attack
			// functions", "a live pawn exists again", "on every spawn path", "nothing player-side
			// auto-engages" and "the eligibility predicate" (ASummonedUnit::IsFollowCommandEligible, the
			// gate TryAutoEnrollInFollowGroup asks). The claims are unchanged, and the {…} tokens are
			// untouched.
			// Follow uses ONE circle, but it uses the SAME pick surface — the same left-click confirm,
			// the same wheel resize, the same exit (704 R-15 + R-16's "For FOLLOW the flow ENDS HERE").
			Row.RelatedActionIds = {
				FName(TEXT("PickMode.Confirm")), FName(TEXT("PickMode.Resize")), FName(TEXT("PickMode.Cancel"))
			};
		}

		// ─── CATEGORY: PICK MODE — Jonathan's three named questions ────────────────────────

		{
			// R-16. Lane B: polled directly every frame while a pick is live
			// (SiegePlayerController.cpp:604-611), ⛔ not bound to an input action. A mouse
			// button is absent from the 26-letter table, so no translation applies.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("PickMode.Confirm"), CategoryPickMode, TEXT("Confirm the circle"),
				TEXT("Left-click to lock in the circle you are drawing and move to the next one."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::LeftMouseButton };
			// ⭐⭐ 704 §4 R-16 detail — THE FIRST OF JONATHAN'S THREE NAMED QUESTIONS: "what the
			// first, second, and third circles do". Citations (T1):
			//   the per-frame poll            = SiegePlayerController.cpp:604-611
			//   melee suppressed for the pick = :2717-2728 and HeroCharacter.h:162-167
			//   SELECT   opens at GroupSelectRadiusDefault  = SiegePlayerController.h:1277;
			//            membership at confirm              = SiegePlayerController.cpp:2851-2882;
			//            empty circle refused, stage HELD   = :2884-2891;
			//            per-order eligibility              = :2857-2862;
			//            transient, never a marker          = :2903-2907, :2988-2995;
			//            FOLLOW ends here                   = :2893-2901
			//   POSITION opens at GroupPositionRadiusDefault = SiegePlayerController.h:1281;
			//            station zone + 2nd-priority disc    = SiegePlayerController.h:1279 and
			//                                                  SiegePlayerController.cpp:2919-2935;
			//            survives as the marker              = :2926, :2947-2955
			//   ATTACK   opens at GroupAttackRadiusDefault   = SiegePlayerController.h:1285;
			//            1st-priority engage trigger         = SiegePlayerController.h:1283 and
			//                                                  SummonedUnit.cpp:1830;
			//            survives as the marker              = :2947-2955
			//   both markers die with the group = UnitCommand.h:149-155
			//   surface trace + sky hide + matching refuse = SiegePlayerController.cpp:2759-2782, :2822-2831
			//   stage tint white/green/red pushed as StageTint = :3253-3265
			//   the completion count = :2968-2978
			// ⚠️ 704 U-4 — THE COLOURS: the C++ pushes StageTint = white/green/red, and a comment
			// beside it claims the material "does not carry it YET". 704 measured that comment STALE
			// (handoffs/TASK-345-artist.md:39-41 records the parameter as authored and wired, and the
			// string StageTint is present in M_SpellReticle.uasset). ⇒ the colours are stated as the
			// C++ drives them; ⛔ no different colours were invented, and only pixels can close a
			// colour claim (`AS-§6` A(e)). Flagged in the TASK-707 handoff.
			// ⛔ T5 — "pushed through a material parameter named StageTint" is where a reader checks
			// the claim, ⛔ not something a player can use; it is cited here instead.
			// ⛔ NO RADIUS VALUE RESTATED (704 U-5, the M7.7 lesson). The three defaults were NAMED
			// here until TASK-1541 (2026-09-27); the prose now says "its own default size" for each of
			// GroupSelectRadiusDefault, GroupPositionRadiusDefault and GroupAttackRadiusDefault.
			Row.Detail = FText::FromString(FString(
				TEXT("Read straight from the mouse button every frame while a pick is open, rather than through the game's control setup. Your hero ")
				TEXT("does not swing on that click — melee is suppressed for the whole pick.\n\n")
				TEXT("THE THREE CIRCLES, IN ORDER — what each one actually does:\n\n")
				TEXT("1. SELECT — opens at its own default size. At confirm, every eligible unit inside it ")
				TEXT("(measured flat, ignoring height) joins the group. An empty circle is refused and you STAY in the stage — a different ")
				TEXT("circle can still succeed — with \"No units in the circle\" on the HUD. The eligibility test ")
				TEXT("differs by order: Hold and Ambush use the narrower rule for zone orders, while Follow is ")
				TEXT("wider — it also admits the Support Cleric and the Miner — and both exclude Siege units ")
				TEXT("(Ogre/Sapper) and the entire enemy side. This circle is only there while you pick: it is ")
				TEXT("destroyed at the final confirm and never becomes a marker. For FOLLOW the flow ENDS HERE.\n\n")
				TEXT("2. POSITION — opens at its own default size. This is the ground the squad stands on: ")
				TEXT("the station zone it spreads inside, and the second-priority engage disc. At the final ")
				TEXT("confirm this circle stays on the map as the group's permanent position marker.\n\n")
				TEXT("3. ATTACK — opens at its own default size. This is the first-priority engage trigger: an ")
				TEXT("enemy entering it is what the squad goes for first. It also stays as a permanent marker.\n\n")
				TEXT("Both surviving markers die with the group.\n\n")
				TEXT("The circle you are drawing sits on the ground under the cursor — flat floor, hill crown ")
				TEXT("or flank alike, never a flat plane — and it hides while the cursor is on the sky; the ")
				TEXT("confirm refuses in exactly the same case, so what you see is what the click does.\n\n")
				TEXT("The circles are colour-coded by stage: Select white, Position green, Attack red.\n\n")
				TEXT("The completion line reports the count that joined, not the count you circled, because ")
				TEXT("members that died part-way through are dropped.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "Polled directly every frame while a pick is live, not bound to an
			// input action" (the per-frame poll cited above), "(2D)" (the membership test is
			// FVector::DistSquared2D against the select radius, in the controller's Select-stage confirm
			// branch), "the narrower zone-order predicate" (IsGroupCommandEligible), "a transient pick
			// visual", "traces to the surface", "refuses on the same flag" and "mid-flow". No digit was
			// added: "(2D)" was removed and the stage labels are byte-identical. The claims are unchanged.
			// The other two halves of the same gesture, so this page is complete on its own.
			Row.RelatedActionIds = { FName(TEXT("PickMode.Resize")), FName(TEXT("PickMode.Cancel")) };
		}

		{
			// R-17. Lane B. ⛔ The wheel is polled, not bound: ASiegePlayerController::ApplyGroupPickWheel,
			// called only from PlayerTick's `GroupPickStage != EGroupPickStage::None` branch. It is
			// ONE of the wheel's three consumers `MARK-§4` names (as amended by `STACK-§4`): this pick
			// poll, the placement branch's footprint poll (ApplyPlacementFootprintWheel) and
			// UWarMapWidget::NativeOnMouseWheel while the map is open; it stays inert everywhere else.
			// TASK-1585 (2026-09-28): this read "it is inert everywhere except inside a pick
			// (SiegePlayerController.cpp:2787-2791, call site :599)", which the war-map wheel
			// (TASK-745) and the placement wheel (TASK-815) made false.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("PickMode.Resize"), CategoryPickMode, TEXT("Resize the circle"),
				TEXT("Scroll the mouse wheel to grow or shrink the circle you are currently drawing."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::MouseScrollUp, EKeys::MouseScrollDown };
			// ⭐⭐ 704 §4 R-17 detail — JONATHAN'S SECOND NAMED QUESTION: "how to resize them".
			// Citations (T1): the wheel handler = SiegePlayerController.cpp:2785-2806, with
			// GroupRadiusWheelStep / GroupRadiusMin / GroupRadiusMax at SiegePlayerController.h:1265,
			// :1269, :1273; each stage opening at its own default = SiegePlayerController.h:1258-1262
			// and SiegePlayerController.cpp:2710, :2909, :2929; the in-place decal resize = :2808-2817;
			// polled-not-bound = :2787-2791, call site :599 (⚠️ TASK-1585, 2026-09-28: this also cited
			// those lines for "inert outside a pick", which is stale — see the R-17 note above and
			// the TASK-1585 note below the string); the no-material degradation = :2808-2810.
			// ⛔ NONE OF THE THREE TUNABLES IS ⛔ ever re-typed as a number. This is 704's U-5 / D-6
			// applied verbatim and it is the M7.7 lesson: the shipped Notes column once said "in 400"
			// while the real radius was 700. ⚠️ Naming them read as jargon on screen (F-3 in
			// handoffs/TASK-707-programmer.md); TASK-1541 (2026-09-27) answered F-3 with player words
			// ("a set step", "a set smallest and largest size") in place of GroupRadiusWheelStep,
			// GroupRadiusMin and GroupRadiusMax — still without inventing a number.
			Row.Detail = FText::FromString(FString(
				TEXT("One notch changes the active circle's radius by a set step, and the radius always stays between ")
				TEXT("a set smallest and largest size. Each stage opens at its own default and resizing one ")
				TEXT("circle never touches an earlier one. The circle on the ground resizes in place as you scroll.\n\n")
				TEXT("The game reads the wheel directly rather than through its control setup, and it was checked that no control anywhere is set to the wheel. ")
				TEXT("During a pick the wheel resizes only the circle you are drawing. ")
				TEXT("It has two other jobs elsewhere: while you are placing a building it can resize that building, and on the war map it resizes one of your own map circles.\n\n")
				TEXT("If the circle's graphics are missing the radius still changes and the confirm still uses it — ")
				TEXT("you just cannot see the circle.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "The decal", "The wheel is polled, not bound, and it is verified
			// globally unbound elsewhere", "the circle material is missing" and "you just get no visual".
			// ⚠️ SC-§101, REPORTED AND NOT FIXED HERE: "it is inert everywhere except inside a pick" is
			// kept byte-identical, and it is FALSE at source since TASK-823. The wheel also resizes a
			// building's footprint in placement (ApplyPlacementFootprintWheel, the Cards.PlacementResize
			// page) and a map circle on the war map (UWarMapWidget::NativeOnMouseWheel, the
			// Interface.MapMarks page), and the Cards.PlacementResize page says so ("the game has three
			// different ones"). A register pass keeps a false claim; it does not repair it.
			// ⭐ FIXED BY TASK-1585 (2026-09-28). Old: "…and it was checked that no control anywhere else
			// is set to the wheel — it is inert everywhere except inside a pick." New, every clause
			// read at source:
			//   • "no control anywhere is set to the wheel": `MARK-§4`'s quoted shipped clause, "The
			//     wheel is verified globally unbound (no camera zoom exists)". "anywhere else" became
			//     "anywhere" because none of the three wheel jobs is a bound control: two are polls
			//     (WasInputKeyJustPressed on MouseScrollUp / MouseScrollDown) and one is a Slate event.
			//   • "During a pick the wheel resizes only the circle you are drawing": ApplyGroupPickWheel
			//     writes GroupPickRadius and the ACTIVE decal only; the pick branch `return`s before the
			//     placement branch is reached; and the war map cannot be open during a pick
			//     (BeginGroupPick is ignored while bWarMapOpen, and CanOpenWarMap refuses a pick).
			//   • "while you are placing a building it can resize that building":
			//     ApplyPlacementFootprintWheel, the placement branch's poll. "can", because it is
			//     inert for a card whose class answers CanScaleFootprint() false (AClimbableTower).
			//   • "on the war map it resizes one of your own map circles": UWarMapWidget::
			//     NativeOnMouseWheel -> TryResizeMarkAtLocal, the Interface.MapMarks page's own words.
			// ⛔ No number is typed. ⛔ Pinned by test 19 (NoPageTeachesARefutedStackOrWheelRule): the
			// refuted phrase may appear on no page, and this page must name "placing a building" and
			// "war map".
			Row.RelatedActionIds = { FName(TEXT("PickMode.Confirm")), FName(TEXT("PickMode.Cancel")) };
		}

		{
			// R-18. Lane B. ⛔ `Escape` NAMED AS DOCUMENTATION ONLY — see R-10. This is
			// Jonathan's "how to exit the command", and it is polled at the TOP of the pick
			// branch before anything else runs (SiegePlayerController.cpp:591-596).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("PickMode.Cancel"), CategoryPickMode, TEXT("Exit the command"),
				TEXT("Right-click or press Escape to abandon the order at any stage — it costs nothing and changes nothing."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::RightMouseButton, EKeys::Escape };
			// ⭐⭐ 704 §4 R-18 detail — JONATHAN'S THIRD NAMED QUESTION, IN HIS OWN WORDS: "how to
			// exit the command". Citations (T1): the poll at the TOP of the pick branch =
			// SiegePlayerController.cpp:591-596; the bound-action double-cover = :1082-1089; "changes
			// nothing" = :591, :1082-1084; the ONE teardown every exit funnels through = :3135-3141;
			// suppression released before any early-out = :3137-3148; stage + scratch cleared =
			// :3155-3161; circles destroyed (all of them on a cancel, only Select on a completed
			// flow) = :3163-3182; HUD prompt cleared = :3184-3187; free-look restored unless the
			// cursor key is still held = :3189-3191; the silent re-press ignore = :2676-2683.
			// ⛔ T5 — 704's opening line "This is Jonathan's 'how to exit the command'" is provenance
			// for the pipeline, ⛔ not player prose; it is recorded here.
			// ⭐⭐ T4 — 704's exit list typed "`T`/`E`". Tokenised: those two letters MOVE on Dvorak.
			// ⛔ T5 — "and EndPlay" dropped from the same list: it is an engine lifecycle callback,
			// not a thing a player does (SiegePlayerController.cpp:3135-3141 carries the full list).
			// ⚠️ "Right-click" and "Escape" stay LITERAL and are provably safe — neither is in the
			// 26-letter table, so neither can ever be retargeted.
			Row.Detail = FText::FromString(FString(
				TEXT("Checked every frame, first, before anything else in the pick happens, and also ")
				TEXT("reachable through the regular cancel control — a deliberate backup. Cancelling leaves ")
				TEXT("every existing group and stance unchanged.\n\n")
				TEXT("One clean-up step handles every way out of a pick — the final confirm, either ")
				TEXT("cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, losing control of your hero and match ")
				TEXT("reset. It lets your hero swing again before anything can cut the clean-up short, clears the stage and everything the pick ")
				TEXT("was holding, destroys every circle the pick still has — a cancel at any stage kills all live ")
				TEXT("circles, while a completed flow only loses its Select circle because the other two were ")
				TEXT("handed to the group — clears the HUD prompt so the stance display returns, and restores ")
				TEXT("free-look unless the interface-cursor key is still held, because anything that still needs the cursor keeps it.\n\n")
				TEXT("Re-pressing the same order key part-way through does nothing — it is quietly ignored; right-click or ")
				TEXT("Escape is how you cancel.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "Polled every frame at the top of the pick branch", "the bound cancel
			// action", "the deliberate double-cover", "The teardown is one function and every exit
			// funnels through it" (CancelGroupPick, cited above), "unpossess" (OnUnPossess calls it),
			// "releases the melee suppression before any early-out", "all pick scratch", "the flow still
			// owns", "the cursor owners compose" (bWantCursor ORs every cursor owner), "mid-flow", "a
			// silent ignore" and "the cancel surface". The claims are unchanged, and the {…} tokens are
			// untouched.
			Row.RelatedActionIds = { FName(TEXT("PickMode.Confirm")), FName(TEXT("PickMode.Resize")) };
		}

		// ─── CATEGORY: INTERFACE ───────────────────────────────────────────────────────────

		{
			// R-19.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.AssistantConsole"), CategoryInterface, TEXT("AI chat"),
				TEXT("Open the chat box and tell your assistant what to do in plain English — press it again to close."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_AssistantConsole")) };
			Row.QwertyReferenceKeys = { EKeys::Enter };
			// 704 §4 R-19 detail. Citations (T1): the toggle asking "is it open?" first =
			// SiegePlayerController.cpp:4440-4463; the open gate = :4378-4395; nothing shown until the
			// posture is granted = :4484-4501; "the console still works anywhere" (Jonathan's ruling,
			// verbatim in the code) = :4479-4482 and SiegeAssistantConsoleWidget.h:147-156; only a
			// genuine Enter commits = SiegeAssistantConsoleWidget.cpp:761-769; ⭐ the ENUMERATED
			// close-route contract (`AS-§6 A-2`) = SiegeAssistantConsoleWidget.h:111-126 with route 4
			// implemented at .cpp:771-819 and the confirm-prompt case at :817-818; ⛔ the permanent
			// `Escape` ruling = SiegeAssistantConsoleWidget.h:99-109 and .cpp:907-918; "a close is not
			// a cancel" = .cpp:637-641, :812-818; the controller owns the input mode =
			// SiegeAssistantConsoleWidget.h:138-143 and — ⛔ BY TEXT, ⛔ not by line (TASK-1432 QA
			// loop 1, `qa/TASK-1433.md` WARN-2) — `ASiegePlayerController::ApplyCursorInputState`,
			// whose `const bool bWantCursor = ... || bAssistantConsoleOpen || ...;` statement is the
			// whole of "the controller owns the input mode". ⛔ WAS `SiegePlayerController.cpp:4357-4375`.
			// ⚠️ ⛔ ONLY that anchor was re-measured; the remaining numbers in this block are
			// ⛔ UNVERIFIED here (same declared scope as the R-08 block above).
			// A faulted assistant disables only this box = SiegeAssistantConsoleWidget.h:144-146.
			// ⚠️ 704 U-6 stands: whether a focused text box swallows `Enter` before Enhanced Input
			// sees it is UNMEASURED since TASK-445 — and this text deliberately claims NEITHER
			// behaviour, so it stays true whichever way that lands.
			// ⚠️ `Enter` and `Escape` stay LITERAL: neither is a letter, so neither can be retargeted.
			Row.Detail = FText::FromString(FString(
				TEXT("A toggle, and it asks \"is it open?\" before \"may it open?\" — the close half is ")
				TEXT("deliberately never blocked, because a close that can be refused is a close that can strand your ")
				TEXT("cursor. Opening has conditions: it is refused while placement, spell targeting or a group pick is using ")
				TEXT("the cursor, or after match end, and nothing is created or shown until the cursor is handed over ")
				TEXT("— on a refusal literally nothing happens on screen.\n\n")
				TEXT("It works anywhere. Unlike the war map there is no proximity check, no commander to look for and no ")
				TEXT("range condition — Jonathan's ruling: \"the console still works anywhere\".\n\n")
				TEXT("Sending: type and press Enter — only a genuine Enter sends it; clicking or tabbing away or clearing ")
				TEXT("the box does not submit a half-typed sentence.\n\n")
				TEXT("CLOSING — the complete list: (1) press the open key again; (2) a spare close route the game ")
				TEXT("keeps on purpose, though nothing uses it today; (3) the box switching itself off when the assistant faults; ")
				TEXT("(4) Enter on an empty box — Jonathan's directive: \"if you press enter without anything ")
				TEXT("typed in the box then it will close\". Empty-Enter closes even with a confirm prompt up — ")
				TEXT("that is the point of the feature.\n\n")
				TEXT("Escape does NOT close the chat box, permanently. The box deliberately lets Escape through so the ")
				TEXT("placement, targeting and group-pick cancels keep working exactly as usual while the box ")
				TEXT("is open.\n\n")
				TEXT("Closing the box does not cancel anything by itself — the assistant decides what a close means, ")
				TEXT("and if it was waiting for you to confirm an order, it discards that order.\n\n")
				TEXT("The chat box never decides by itself who gets the mouse and keyboard; the game decides that in one place. A ")
				TEXT("faulted assistant disables this box and nothing else — no key, no card, no command changes.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// USiegeAssistantConsoleWidget::CancelPressed() (close route 2) and SetConsoleEnabled(false)
			// (route 3, the fault latch), per the enumerated close-route contract cited above. The
			// claims are unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "un-gated", "Opening is gated", "owns the cursor", "until the posture
			// is granted", "no NPC reference" (the war map's commander lookup), "only a genuine Enter
			// commits", "moving focus away", "left unabsorbed so the shipped … cancel routes keep firing
			// byte-identically", "broadcasts no cancellation", "the assistant's own state machine" and
			// "The console never sets the input mode itself; the controller owns that in one place"
			// (ApplyCursorInputState, cited above). Jonathan's two quotations are byte-identical. The
			// claims are unchanged.
			// TASK-1585 (2026-09-28, qa/TASK-1575.md N3 and N4, taken):
			//   N3 — "clicking away" is now "clicking or tabbing away". USiegeAssistantConsoleWidget::
			//   HandleTextCommitted returns on every commit method but ETextCommit::OnEnter, and
			//   OnUserMovedFocus is ANY loss of focus, so a click was one case of it, not the rule.
			//   N4 — "A close is not a cancel: closing the window cancels nothing — only the assistant
			//   itself may turn one into the other." read, in plain words, against the
			//   Interface.AssistantAccept block rendered under it ("or just close the box to discard
			//   it"). It is now "Closing the box does not cancel anything by itself — the assistant
			//   decides what a close means, and if it was waiting for you to confirm an order, it
			//   discards that order." Source: SiegeAssistantConsoleWidget.h's "A CLOSE IS NOT A CANCEL
			//   *IN THIS WIDGET*" (every close route reuses CloseConsole and none broadcasts a
			//   cancellation; CloseConsole broadcasts OnConsoleOpenChanged(false)), then
			//   USiegeAssistantComponent::HandleConsoleOpenChanged -> NotifyConsoleClosed, whose
			//   AwaitConfirm case clears the preview and the pending order and returns to Idle. The
			//   same rule, with the one case the Accept block names spelled out.
			// The confirm step has NO buttons at all (704 R-20), so the accept key belongs on this page.
			Row.RelatedActionIds = { FName(TEXT("Interface.AssistantAccept")) };
		}

		{
			// R-20. ⛔⛔ THE ONE SANCTIONED LETTER LITERAL IN THIS ENTIRE FILE, AND IT IS A
			// FLAGGED JONATHAN DECISION, NOT AN AGENT'S CHOICE (704 §8, F-1).
			//
			// `KBD-§8` / `KBD-§0` ruling 1 pin EVERY human-facing accept-key string to `Z` on
			// EVERY layout (SiegeKeyboardLayoutSubsystem.h:256-260), because the console's own
			// live status line two inches away reads "Press Z to accept, or close this box to
			// discard" (SiegeAssistantConsoleWidget.cpp:71, raised at :1374-1377). A help
			// screen that contradicted that prompt would be worse than either choice alone.
			//
			// ⭐ REVERSING IT IS EXACTLY THIS ONE FLAG: set bLiteralKeyLabel = false and the
			// chip derives like every other raw key (ResolveRowDisplayKeys' Lane-C branch
			// implements BOTH states, and the test file asserts BOTH). ⚠️ It would also mean
			// amending the console's own status line — a `KBD-§8` amendment, not a help edit.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.AssistantAccept"), CategoryInterface,
				TEXT("Accept the assistant's plan"),
				TEXT("When the assistant asks you to confirm an order, press Z to accept — or just close the box to discard it."),
				ESiegeInputLane::RawLetter);
			Row.QwertyReferenceKeys = { EKeys::Z };
			Row.bLiteralKeyLabel = true;
			// 704 §4 R-20 detail. Citations (T1): Jonathan's "no buttons" ruling, quoted verbatim in
			// the code = SiegeAssistantConsoleWidget.h:159-165; the live status line =
			// SiegeAssistantConsoleWidget.cpp:71, raised at :1374-1377; the PREVIEW catch =
			// SiegeAssistantConsoleWidget.h:167-170 and .cpp:845; the modifier rules =
			// .cpp:868-878 (Ctrl) and :875-876 (Shift accepted); the narrow grab = :854-866; the
			// accepted consequence = .h:186-190; the QWERTY-Z POSITION comparison = .cpp:952-969 with
			// the direction contract at SiegeKeyboardLayoutSubsystem.h:230-235.
			// ⛔⛔ THIS ROW'S `Z`s ARE THE ONE SANCTIONED LETTER LITERAL IN THE DETAIL LANE, and it is
			// the SAME flagged Jonathan decision that pins the chip (704 §8 F-1; `KBD-§8` /
			// `KBD-§0` ruling 1, SiegeKeyboardLayoutSubsystem.h:256-260). ⛔ NOT TOKENISED, on purpose:
			// two of the mentions are QUOTATIONS — of his own ruling and of the console's own live
			// status line two inches away — and a page that said "press ;" beside a prompt that says
			// "Press Z" would be worse than either choice alone. ⭐ The reversal is still exactly one
			// flag: bLiteralKeyLabel = false derives the chip, and the ACCEPT-KEY mentions in this
			// prose would then move to {Interface.AssistantAccept} tokens — a `KBD-§8` amendment,
			// ⛔ not a help edit.
			// ⛔⛔ CORRECTION, AND READ IT BEFORE ACTING ON THE PARAGRAPH ABOVE (708 W-1): ⛔ NOT EVERY
			// `Z` IN THIS PROSE IS AN ACCEPT KEY, SO THE REVERSAL DOES ⛔ NOT REACH ALL OF THEM. The
			// Ctrl+Z / Ctrl+Shift+Z mentions are SLATE'S OWN undo/redo, which no layout remap touches —
			// ⛔ tokenising THOSE would print a key the player does not press. ⭐ The carve-out is those
			// two Ctrl mentions ONLY: the trailing "Shift+Z is accepted" clause IS the accept key and
			// DOES follow the pin, so a reversal must SPLIT that sentence, ⛔ not exempt it wholesale.
			// ⇒ The engine citations and the full reasoning are in the ⛔ DO-NOT-TOKENISE block at
			// :948-965, immediately above the sentence it guards.
			Row.Detail = FText::FromString(FString(
				TEXT("There are no accept and cancel buttons; Jonathan removed both. His ruling: \"instead of it ")
				TEXT("being a cancel button and an accept button, lets make it to where there is no cancel button ")
				TEXT("(they just simply close the chat box), and instead of an accept button they press 'z'\". ")
				TEXT("While a prompt is up the status line reads, exactly: \"Press Z to accept, or close this box ")
				TEXT("to discard\".\n\n")
				TEXT("The key is caught on its way in, before the text box you are typing in gets it, ")
				TEXT("which is why it works at all: anything that listened for it after the box would never see a letter ")
				TEXT("the box had already typed.\n\n")
				// ⛔⛔ WARNING TO THE NEXT AUTHOR — THE TWO `Z`s IN THE SENTENCE BELOW ARE SLATE'S OWN
				// UNDO/REDO SHORTCUT, ⛔ NOT A LAYOUT-REMAPPED ACTION KEY, AND THEY MUST ⛔ NEVER BE
				// TOKENISED. (Source of the ruling: qa/TASK-708.md finding W-1.)
				// VERIFIED AT ENGINE SOURCE: Ctrl+Z undo is FGenericCommands::Undo's chord
				// FInputChord(EModifierKey::Control, EKeys::Z) (GenericCommands.cpp:19), mapped onto the
				// text layout at SlateEditableTextLayout.cpp:154-157; Ctrl+Shift+Z redo is
				// SlateEditableTextLayout.cpp:1170-1172. Slate dispatches BOTH itself against a hardcoded
				// EKeys::Z — USiegeKeyboardLayoutSubsystem never sees them and never retargets them — so
				// this `Z` does ⛔ NOT follow the accept key's pinned position.
				// ⭐ THEREFORE THE F-1 REVERSAL DESCRIBED ABOVE (:927-930) DOES NOT REACH THIS SENTENCE.
				// Setting bLiteralKeyLabel = false moves the ACCEPT-key mentions to
				// {Interface.AssistantAccept} tokens; doing the same to these two would print whatever the
				// accept key resolved to beside a shortcut the player still presses as Ctrl+Z — i.e. it
				// would INTRODUCE the printing-a-key-you-do-not-press defect that the tokenisation work
				// exists to prevent.
				// ⚠️ THE CARVE-OUT IS THE TWO Ctrl MENTIONS ONLY — the trailing "Shift+Z is accepted"
				// clause IS the accept key and DOES follow the pin. That boundary is stated ONCE, at
				// :934-936; ⛔ do not restate it here, because two copies drift apart.
				TEXT("Modified presses are not the accept key — Ctrl+Z and Ctrl+Shift+Z are the text box's own ")
				TEXT("undo and redo, and catching them would both kill undo and execute an order you never asked ")
				TEXT("for. Shift+Z is accepted — it is still \"the Z key\" to a human.\n\n")
				TEXT("Catching the key is kept as narrow as it can be: it happens only while the box is open, switched on, and a ")
				TEXT("confirm prompt is actually up, so you can still type the letter z the rest of the time. ")
				TEXT("Accepted consequence: while a prompt is up you cannot type z into the box.\n\n")
				TEXT("On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — it works out which key ")
				TEXT("sits there from your keyboard layout.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "caught in preview — it tunnels down the focus path from the root
			// before the focused text box, which is why a plain key handler could never see a printable
			// key the box already ate" (the NativeOnPreviewKeyDown catch cited above), "consuming them",
			// "The grab … fires only while the box is open, enabled", and "the comparison resolves
			// through the layout subsystem" (USiegeKeyboardLayoutSubsystem). Every Z mention, Jonathan's
			// quotation and the status-line quotation are byte-identical, so the one sanctioned letter
			// literal and the Ctrl carve-out above both still hold. The claims are unchanged.
			Row.RelatedActionIds = { FName(TEXT("Interface.AssistantConsole")) };
		}

		{
			// R-21. ⭐ `M` is the second Dvorak IDENTITY (the M position does not move), which is
			// what makes it the "holds" half of the acceptance assertion.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.WarMap"), CategoryInterface, TEXT("Open the map"),
				TEXT("Walk up to your commander in your castle, then press it to open the war map — press again to close."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_WarMap")) };
			Row.QwertyReferenceKeys = { EKeys::M };
			// 704 §4 R-21 detail. Citations (T1): close asked first and never gated =
			// SiegePlayerController.cpp:4767-4779; ⭐ the team resolved from your player state and
			// never guessed = :4718-4742; the distance test and radius both belong to the commander =
			// :4744-4765 and CommanderNpc.h:274-295; the gate checked BEFORE the posture = :4781-4791;
			// the out-of-range HUD line = :4797; the gate is the map's only = :4786-4790; the posture
			// rollback = :4820-4854; ⭐ the reveal discarded on close, Jonathan's words = :4857-4873;
			// the POLLED close and WHY it exists = ASiegePlayerController::PlayerTick's
			// `if (bWarMapOpen)` branch -> CloseWarMap(), located BY SYMBOL (`SC-§38`). ⚠️ The line
			// range this comment used to carry (`:643-671`) had already ROTTED — the branch now sits
			// near :744-755, and that number is a DATED HINT rather than the key.
			//
			// ⛔⛔ TASK-870 — THE REPAIR, AND WHAT BOUGHT IT. This page SHIPPED (in the commit that
			// carried this file wholesale) claiming the map is "closed by right-click or Escape".
			// 🧑 JONATHAN OBSERVED THE OPPOSITE, in his own words, in one click: "opening the war map
			// and right clicking empty ground does not cause it to close, the map seems to function
			// exactly as it should". ⇒ the RIGHT-CLICK half is FALSE and it is GONE. ⛔ The ESCAPE
			// half is UNTOUCHED: it was not in question, and it is deliberately NOT re-derived here.
			//
			// ⭐⭐ WHY NOBODY COULD SETTLE THIS BY READING — now law as `SC-§42`, written for this
			// exact event: UWarMapWidget::NativeOnMouseButtonDown returns FReply::Handled() on EVERY
			// right button while the map is open, and THREE independent readers reasoned from that to
			// the RIGHT answer with an instrument that COULD NOT CONFIRM IT. A HANDLED EVENT IS NOT
			// AN ACTIONED EVENT — `Handled` is a ROUTING declaration, never a behavioural one.
			// ⛔ THE POLL ITSELF IS NOT TOUCHED BY THIS TASK. The controller still polls RMB/Escape in
			// that branch; this is a TEXT repair, and whether the CODE should change is a separate
			// ruling that belongs to whoever owns the widget's FReply.
			//
			// ⭐ THE REMEDY'S SHAPE IS COPIED, ⛔ NOT INVENTED: TASK-821 repaired the same defect
			// class on Cards.Discard by REMOVING the false sentence rather than writing a denial, and
			// this file's test 14 keeps it removed. Silence + a related-controls edge is this
			// registry's own mechanism: Interface.MapMarks — the row that OWNS right-click on the war
			// map, and the row Jonathan's same sentence ruled TRUE — is rendered underneath this page
			// and answers the question in exactly one place (`HELP-§2`: one definition, two
			// renderings, nothing to drift).
			// ⚠️ "Escape" stays LITERAL — a non-letter, provably immovable (the table is A..Z only).
			Row.Detail = FText::FromString(FString(
				TEXT("A toggle, and closing is checked first and never blocked, for the same reason as the chat box.\n\n")
				TEXT("THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. Your team is read from ")
				TEXT("your own player record and never guessed: with no such record there is no honest answer and no ")
				TEXT("commander is picked, because a wrong guess on the wrong side would tie the map to the ")
				TEXT("enemy's commander and take the reveal's price from the wrong one. The distance test and its radius ")
				TEXT("both belong to the commander — he checks whether you are inside his own interaction range — and ")
				TEXT("the map key asks him rather than keeping its own copy of either.\n\n")
				TEXT("The gate is checked BEFORE anything about the cursor changes, deliberately, so an out-of-range ")
				TEXT("press cannot be felt as a one-frame flicker mid-fight. Out of range you get one HUD line ")
				TEXT("naming the reason — \"Walk up to your commander in the castle to use the war map\". This ")
				TEXT("gate is the map's and the map's only — it is never applied to the chat box.\n\n")
				TEXT("Nothing appears until the cursor is handed over, and if the map then fails to appear or ")
				TEXT("fails to report itself open, the cursor is handed back rather than left claimed by a map ")
				TEXT("that is not there.\n\n")
				TEXT("Closing the map DISCARDS the paid reveal, unconditionally — \"red dots vanish the moment the ")
				TEXT("map closes, even one second after paying — that is the mechanic\". Escape closes it too, ")
				TEXT("checked directly every frame; that check exists because a marker click opens the chat box, whose focused ")
				TEXT("text field would otherwise swallow the toggle key and type it into your sentence instead.")));
			// TASK-1541 (2026-09-27): player words replace the code names this prose used to print —
			// ACommanderNpc::IsPlayerInRange reading ACommanderNpc::InteractRadius. The claim is unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "never gated", "The team is resolved from your player state" (the
			// APlayerState read cited above), "no commander is returned", "a wrong default", "gate the
			// map on", "price the reveal off the wrong actor", "the controller re-implements neither",
			// "the cursor posture is touched", "the posture is granted", "the widget then fails to
			// create", "the posture is rolled back rather than left as a cursor owner with no UI" and
			// "polled every frame; that poll". Jonathan's quotation is byte-identical, and no refuted
			// mouse-button wording entered the page (test 17's fragment scan). The claims are unchanged.
			// Everything you can DO on the map, on the page for opening it (704 R-22, R-23).
			// ⛔⛔ TASK-870 EDGE CHANGE, DECLARED (`HELP-§7`): Interface.MapMarks is APPENDED, and the
			// two shipped ids are kept in their shipped order. ⭐ It is this row's OWN OUTBOUND edge —
			// ⛔ no other row's RelatedActionIds is touched by this task, and ⛔ nothing points INTO
			// this row that did not already. REASON: with the false close-sentence gone, this edge is
			// what puts the TRUE owner of right-click-on-the-war-map on this page, so a player who
			// arrives looking for that gesture is answered by the row that owns it rather than by a
			// second copy of its prose. ⚠️ Every id here resolves to a real row — a dangling entry
			// renders NOTHING and logs nothing (ComposeDetailContent `continue`s past it), and the
			// registry-wide walk that would catch it lives in test 9, EveryRowHasAuthoredDetail.
			Row.RelatedActionIds = {
				FName(TEXT("Interface.WarMapReveal")),
				FName(TEXT("Interface.WarMapMarker")),
				FName(TEXT("Interface.MapMarks"))
			};
		}

		{
			// R-22. Lane D — the map's Reveal button. ⛔ NO NUMBER IN THE ONE-LINER: the price is
			// ACommanderNpc::EnemyRevealCost (CommanderNpc.h:311) and TASK-707 renders it from
			// the live property or leaves the name (`HELP-§2`, the M7.7 lesson). Since TASK-1541
			// (2026-09-27) the page leaves no name either: the one-liner types no number and names no
			// property, and the detail calls the price "a fixed reveal fee" where it printed
			// EnemyRevealCost (the amendment below records the same change).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.WarMapReveal"), CategoryInterface, TEXT("Reveal enemy positions"),
				TEXT("Pay gold on the war map to reveal every enemy position — and they vanish again the moment you close it."), ESiegeInputLane::PointerOnly);
			Row.bPointerOnly = true;
			// 704 §4 R-22 detail. Citations (T1): ⭐ the price and Jonathan's own quoted words =
			// CommanderNpc.h:297-311 (EnemyRevealCost = 30 at :311, his sentence at :298-299); "a
			// mechanic rule, so a property default and never a card-table column" + the commander
			// only HOLDS the number = CommanderNpc.h:301-307; the cost read off your own team's
			// commander at purchase, failing closed with none = SiegePlayerController.cpp:5103-5121,
			// :5108-5119; the net-zero refusal, local and on the authority = :4982-4992, :5123-5134;
			// no early-out past the spend = :5136-5139; "THE AI NEVER SPENDS GOLD" =
			// CommanderNpc.h:304-307.
			// ⭐⭐ THE ONE NUMBER STATED IN THE WHOLE REGISTRY, AND IT IS STATED BECAUSE JONATHAN'S
			// OWN WORDS ARE THE SOURCE AND 704 QUOTED THEM AT THE PROPERTY. Everywhere else a
			// tunable is described, never typed (704 U-5, the M7.7 "in 400"/AoERadius-700 lesson; they
			// were NAMED until TASK-1541, 2026-09-27) — and the sentence around the quote calls it "a
			// fixed reveal fee" where it printed EnemyRevealCost, so the mechanism, not the number, is
			// what the page teaches.
			// ⭐ TASK-1576 (2026-09-28): read "STATED" above as TYPED. Two pages now SHOW a number
			// (the map-circle cap and the stack health factor), but each is read from its owner when
			// the page is composed, through a `{#Name}` token, and none is typed; this quoted 30 gold
			// remains the one quantity typed into the registry. (It is quoted, not derived, and the
			// reveal fee's owner, ACommanderNpc::EnemyRevealCost, was not a TASK-1576 candidate.)
			Row.Detail = FText::FromString(FString(
				TEXT("The price is a fixed reveal fee — Jonathan's own number: \"You can pay 30 gold to reveal all ")
				TEXT("enemy locations\". It is a rule of the game, so it is a fixed setting on the commander and never a line in the card ")
				TEXT("list, and the commander only holds the number: he never looks at anyone's gold and never ")
				TEXT("spends. The cost is read off your own team's commander at the moment of purchase and never ")
				TEXT("copied anywhere else; with no commander the purchase is simply refused rather than inventing a fallback ")
				TEXT("price.\n\n")
				TEXT("The refusal is net-zero: you are refused BEFORE any gold moves, both in the local check that ")
				TEXT("produces the HUD message and again where the purchase is actually settled. Past the spend there is deliberately no ")
				TEXT("way to stop half-way, so a partial spend is impossible — even an empty survey is a legitimate paid-for ")
				TEXT("answer.\n\n")
				TEXT("The spend is always initiated by your click, which is what keeps the standing ruling \"THE AI ")
				TEXT("NEVER SPENDS GOLD\" true.")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "a mechanic rule, so it is a property default and never a card-table
			// column" (ACommanderNpc::EnemyRevealCost, a UPROPERTY default, not a DT_Cards column), "the
			// commander class", "it never reads a balance", "never re-typed", "fails closed", "on the
			// authority" (the server-side re-check cited above) and "no early-out". The quoted 30 gold
			// and both quotations are byte-identical; no digit was added. The claims are unchanged.
			// You cannot reach the button without opening the map first (the proximity gate).
			Row.RelatedActionIds = { FName(TEXT("Interface.WarMap")) };
		}

		{
			// R-23. Lane D — a map marker click.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.WarMapMarker"), CategoryInterface, TEXT("Click a place on the map"),
				TEXT("Click a marker on the war map to drop that place's name into the chat box — you still send the sentence yourself."), ESiegeInputLane::PointerOnly);
			Row.bPointerOnly = true;
			// 704 §4 R-23 detail. Citations (T1): the pinned open-then-append order and WHY =
			// SiegePlayerController.cpp:4919-4930; the symbol moved opaquely = :4943-4951; the
			// checked return value = :4949-4957; ⭐⭐ the no-auto-submit ruling, verbatim in the
			// code = :4959-4965.
			Row.Detail = FText::FromString(FString(
				TEXT("Clicking a marker opens the chat box first, the normal way, then adds the ")
				TEXT("place's name. That order is fixed: adding the name never opens the box and never sends, and opening ")
				TEXT("clears the input field every time — so adding first and opening second would silently ")
				TEXT("eat your click.\n\n")
				TEXT("The name is passed along exactly as the map gave it — nothing on the way checks it against a ")
				TEXT("list of its own, because a second list there would drift out of step with the real one, ")
				TEXT("which lives elsewhere. Whether the name actually went in is checked; a refused insert is written to the game's log ")
				TEXT("and adds nothing rather than delivering the wrong thing.\n\n")
				TEXT("NOTHING IS SUBMITTED, AND THAT IS THE RULING: \"the map writes the symbol into the box and ")
				TEXT("THE PLAYER SENDS THE SENTENCE HIMSELF. An auto-submit would turn a click into an order\".")));
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "through the proper open path", "appends the symbol", "That order is
			// pinned", "the append", "on every open", "The symbol is moved opaquely — the controller never
			// spells it and must not learn which symbols exist, because a validation branch there would
			// be a second, drifting copy of a vocabulary it does not own", "The append's return value is
			// checked; a refused insert is logged" and "mis-delivering". "The place's name" is the
			// one-liner's own word for the symbol. The closing ruling is a quotation and is
			// byte-identical, "symbol" included. The claims are unchanged.
			// A marker click lands in the chat box, so the box's own page is the next thing to read.
			Row.RelatedActionIds = { FName(TEXT("Interface.WarMap")), FName(TEXT("Interface.AssistantConsole")) };
		}

		{
			// ⭐⭐ R-27 — THE MAP MARKS (TASK-823; `MARK-§0`..`MARK-§5`, and it is the HOLE the
			// `CARDBAR-§9` decomposition FOUND rather than a nice-to-have).
			//
			// ⛔⛔ WHY IT EXISTS AT ALL, stated so nobody reads it as scope creep: `STACK-§4` orders
			// that this screen distinguish all THREE wheel meanings, and the third one — the map's
			// numbered circles — had ⛔ NO ROW OF ANY KIND. The three shipped Interface.WarMap*
			// rows cover the map's own PLACE-NAME markers and ⛔ nothing of this feature: ⛔ not
			// placing a circle, ⛔ not resizing it, ⛔ not deleting it. ⇒ `STACK-§4`'s instruction
			// was ⛔ UNSATISFIABLE until this row existed. ⭐ This is `HELP-§2` mechanism 2's own
			// honest limit paying out exactly as it predicted: the marks feature shipped after this
			// screen was written, it was SURFACE-ABLE, and nobody surfaced it.
			//
			// ⚠️⚠️ THE LANE IS A JUDGMENT CALL AND IT IS DECLARED, ⛔ not slipped in (handoff
			// TASK-823, ruling H-1). These four gestures are SLATE EVENTS on a focused widget
			// (UWarMapWidget::NativeOnMouseButtonDown / ::NativeOnMouseWheel), ⛔ not controller
			// polls, and ESiegeInputLane's own comment describes RawNonLetter as a "RAW-polled key".
			// It is nevertheless the CORRECT lane, for the reason the lane actually encodes:
			//   (a) what the resolver must do here is "label the reference keys VERBATIM, with ZERO
			//       GetPositionalKey calls", which is Lane B's algorithm exactly — and the identity
			//       is PROVABLE, because the translation table holds A..Z and nothing else, so a
			//       mouse button and a wheel notch can never move;
			//   (b) ⛔ PointerOnly would be actively WRONG here even though these are UI clicks:
			//       ComposeKeyChipLabel answers the single "Mouse click" affordance for that lane
			//       REGARDLESS of the keys it is handed, so the WHEEL — the one gesture this row
			//       was boarded to make visible — would be structurally unable to appear on the
			//       row. ⇒ the row would silently fail the very requirement it exists to satisfy.
			// ⚖️ Lane D stays right for Interface.WarMapReveal and Interface.WarMapMarker, which
			// really are one click on one button, and neither of them is touched by this task.
			//
			// Citations (T1) — read at SOURCE, located by SYMBOL (`SC-§38`):
			//   • PLACE = UWarMapWidget::TryPlaceMarkAtLocal, reached from the left-button arm of
			//     NativeOnMouseButtonDown only after the map's own markers have been tested and
			//     missed; the number comes from USiegeMapMarkSubsystem::FindLowestFreeNumber;
			//   • RESIZE = UWarMapWidget::TryResizeMarkAtLocal from ::NativeOnMouseWheel, whose hit
			//     test is THIS event's cursor position and whose miss arm changes ⛔ nothing
			//     ("⛔ not the nearest circle, ⛔ not the last-touched one, ⛔ not a map zoom");
			//   • DELETE = the `InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton` arm ->
			//     TryDeleteMarkAtLocal, whose INDEX_NONE arm deletes nothing and says nothing;
			//   • NAME = the mark arm's `OnPlacePicked.Broadcast(PickedMarkSymbol)` — the SAME
			//     delegate the seven place markers use, so the chat box opens through the proper
			//     open path and then the symbol is appended, and ⛔ nothing is submitted;
			//   • MARKERS WIN = NativeOnMouseButtonDown's ordering comment and structure (the mark
			//     lane is entered only on `HitIndex == INDEX_NONE` from the marker hit test);
			//   • the hole stays = `M-1` implemented as RemoveMark(Number) plus
			//     FindLowestFreeNumber, and stated to the player by the shipped deleted-status line;
			//   • the cap = USiegeMapMarkSubsystem::MaxMapMarks, refused LOUDLY by the
			//     AddMark-failed arm, whose count is read from the store at the moment of refusal;
			//   • client-local and never replicated = `class USiegeMapMarkSubsystem : public
			//     ULocalPlayerSubsystem` and its M8 declaration;
			//   • survives map close, cleared at match reset = WarMapWidget's own note that reset is
			//     the store's job, and ASiegeGameMode's single ClearMarks() call site;
			//   • they are real places to the commander = USiegeAssistantSnapshot reading the mark
			//     store and publishing FSiegeMapMark::MakeSymbol(Number) into the place list.
			//
			// ⛔ NO NUMBER IS TYPED: the cap is "a limited number" in the prose (it printed MaxMapMarks
			// until TASK-1541, 2026-09-27, 🧑 his "plain words"). ⛔ And ⛔ no coordinate, radius or count
			// is described — the airlock is a property of the feature, not something this page needs
			// to explain.
			// ⭐ TASK-1576 (2026-09-28): the cap is now SHOWN, and it is still not typed. "You can hold a
			// limited number of circles at once" became "You can hold up to" + the `{#MapMarks.Cap}`
			// number token + "at once", and ComposeDetailForDisplay replaces the token with
			// USiegeMapMarkSubsystem::MaxMapMarks, read off the class default when the page is composed
			// and formatted as a whole count with its noun ("9 circles" at today's value; "1 circle" if
			// it were ever one). The count that is described is therefore the store's own cap, the one
			// the AddMark refusal compares against; ⛔ no coordinate or radius is described, as before.
			// Pinned by test 20 (ShownNumbersAreReadFromTheirOwners).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.MapMarks"), CategoryInterface, TEXT("Draw circles on the map"),
				TEXT("On the war map: left-click empty ground to drop a numbered circle, scroll on one to resize it, right-click one to delete it, and click one to name it to your commander."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = {
				EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::MouseScrollUp, EKeys::MouseScrollDown
			};
			Row.Detail = FText::FromString(FString(
				TEXT("Your own numbered circles, drawn on the war map. A circle is how you name a piece of ground to ")
				TEXT("your commander: it carries a number, that number is handed to him as a place, and an order ")
				TEXT("can then point at exactly the ground you meant.\n\n")
				TEXT("THE FOUR GESTURES, and all four work on the war map and nowhere else. Open it with ")
				TEXT("{Interface.WarMap}.\n\n")
				TEXT("• LEFT-CLICK EMPTY MAP drops a new circle there. It takes the lowest number not currently in ")
				TEXT("use.\n\n")
				TEXT("• SCROLL ON A CIRCLE resizes that circle. The wheel anywhere else on the map does nothing ")
				TEXT("whatsoever — not the nearest circle, not the last one you touched, and there is no map zoom for ")
				TEXT("it to fall into.\n\n")
				TEXT("• RIGHT-CLICK A CIRCLE deletes it. A right-click that hits no circle does nothing at all, ")
				TEXT("deliberately: it never guesses at the closest one.\n\n")
				TEXT("• LEFT-CLICK A CIRCLE writes its name into the chat box, opening the box first if it is not ")
				TEXT("already up. Nothing is sent — you still send the sentence yourself, exactly as when you click ")
				TEXT("one of the map's named places.\n\n")
				TEXT("THE MAP'S OWN PLACE MARKERS WIN. A click that lands on one of the places the map already draws ")
				TEXT("names that place, even if one of your circles is sitting underneath it. Without that rule one ")
				TEXT("big circle dropped over a landmark would quietly make it unclickable for the rest of the ")
				TEXT("match.\n\n")
				TEXT("NUMBERS ARE PERMANENT NAMES, NOT POSITIONS IN A LIST. Deleting a circle leaves a hole and the ")
				TEXT("survivors keep their numbers. That is deliberate: the map writes a name into your input box and ")
				TEXT("you send it in your own time, so renumbering would silently point a sentence you had already ")
				TEXT("typed at different ground.\n\n")
				TEXT("You can hold up to {#MapMarks.Cap} at once. At the limit a further click refuses out loud and ")
				TEXT("tells you how many you are already holding, rather than doing nothing and looking broken.\n\n")
				TEXT("The circles are yours alone. They live on your own machine, the enemy never sees them, they ")
				TEXT("survive closing and re-opening the map, and they are cleared when the match resets. They are ")
				TEXT("never saved.")));
			// TASK-1574 (2026-09-28, the register pass): "handed to him as a place" replaces "published to
			// him as a place" (USiegeAssistantSnapshot publishing FSiegeMapMark::MakeSymbol into the place
			// list, cited above). The claim is unchanged, no number is typed, and the "right-click" that
			// test 17 uses as its positive control is untouched.
			// You cannot reach any of this without opening the map, and the map's own place markers
			// are the OTHER clickable thing on the same screen — the one a player will confuse these
			// with, and the one that wins a contested click. PickMode.Resize is the wheel's other
			// meaning, named here for the same reason it is named in the prose above (`STACK-§4`).
			// ⛔ This row's OWN outbound edges only (`HELP-§7`); ⛔ no existing row's edges are
			// touched, and every id here resolves to a real row.
			Row.RelatedActionIds = {
				FName(TEXT("Interface.WarMap")), FName(TEXT("Interface.WarMapMarker")), FName(TEXT("PickMode.Resize"))
			};
		}

		{
			// R-24. ⭐ THE MENU DOCUMENTS ITS OWN KEY (`HELP-§4`). This row is also what the
			// hint line reads its chip from, so the overlay never types its own key either.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.ControlsHelp"), CategoryInterface, TEXT("Controls"),
				TEXT("Open this list of controls — press it again to close."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_ControlsHelp")) };
			Row.QwertyReferenceKeys = { EKeys::Tab };
			// 704 §4 R-24 detail. Citations (T1): the menu documents its own key = `HELP-§4`; it does
			// NOT pause and it is READ-ONLY on the world = `HELP-§5`; the two-route close list and
			// the shipped precedent it copies = `HELP-§5` and SiegeAssistantConsoleWidget.cpp:907-931;
			// the cursor composition and its owners = ⛔ `ASiegePlayerController::ApplyCursorInputState`
			// ⛔ BY NAME, and the one expression ⛔ BY TEXT: `const bool bWantCursor =
			// bInPlacementMode || ... || bControlsHelpOpen || bUICursorHeld;` — this row's own term
			// is the `|| bControlsHelpOpen`. ⛔ TASK-1432 QA loop 1 (`qa/TASK-1433.md` WARN-2): this
			// block ⛔ WAS `SiegePlayerController.cpp:4326-4376, the one expression at :4357`, and
			// `:4357` had rotted onto an unrelated formation comment. ⛔ THIS IS THE R-24 ROW'S OWN
			// CITATION BLOCK — the most-read one in the file — which is why it is anchored to text
			// and a name, the two things that do not move (this file's `§3` rule, applied to itself).
			// ⚠️ ⛔ ONLY that anchor was re-measured in this pass; the widget-sets-its-own-posture
			// defect = :222-239 is carried forward ⛔ UNVERIFIED.
			// ⭐ T4 — the overlay's OWN key is a token, for the same reason the hint line splices its
			// chip rather than typing it (`HELP-§4`, TASK-706 D-9): the menu documents its own key,
			// and typing "Tab" here would be the one hardcoded key on a screen built to have none.
			// ⛔ T5 — 704's closing "Status: the action asset and its Tab mapping both landed during
			// this task" is a pipeline status line (its §2b), ⛔ not player prose.
			Row.Detail = FText::FromString(FString(
				TEXT("A toggle. It documents its own key: {Interface.ControlsHelp} is tied to its spot on the keyboard like any ")
				TEXT("other key, so on a layout that moves it this entry updates with everything else.\n\n")
				TEXT("The overlay does not pause the game — single-player has no pause today and the enemy keeps ")
				TEXT("marching; a pause would be a new mechanic, not a side effect of a help screen. It is ")
				TEXT("for reading only and changes nothing in the game: opening it issues no order, cancels no group, plays no card and ")
				TEXT("moves no gold.\n\n")
				TEXT("It closes on {Interface.ControlsHelp} or its own Close button, and that is the complete ")
				TEXT("list — it never takes Escape for itself. Every normal cancel keeps working while it is open.\n\n")
				TEXT("Its cursor is handled in the one place the game decides who gets the cursor, and nowhere else — ")
				TEXT("everything else that uses the cursor keeps exactly the priority it already had.")));
			// TASK-1541 (2026-09-27): player words replace the code name this prose used to print —
			// ASiegePlayerController::ApplyCursorInputState(), cited by name above. The claim is unchanged.
			// TASK-1574 (2026-09-28, the register pass): player words replace the developer wording this
			// prose used to print — "a positional key" (a Lane A key, re-derived through the layout
			// subsystem), "this row re-derives", "read-only on the world", "it never claims Escape",
			// "Every shipped cancel route keeps firing", "who owns the cursor" and "the existing owners
			// keep their exact shipped precedence" (the other terms of bWantCursor, cited above). The
			// claims are unchanged, and both {…} tokens are untouched.
		}

		return Rows;
	}();

	return Actions;
}

const FSiegeControlsHelpAction* FSiegeControlsHelpRegistry::FindAction(FName ActionId)
{
	if (ActionId.IsNone())
	{
		return nullptr;
	}

	return GetActions().FindByPredicate(
		[ActionId](const FSiegeControlsHelpAction& Row) { return Row.ActionId == ActionId; });
}

TArray<FKey> FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(
	const FSiegeControlsHelpAction& Row,
	const TArray<FKey>& AppliedKeys,
	const USiegeKeyboardLayoutSubsystem* LayoutSubsystem)
{
	TArray<FKey> DisplayKeys;

	switch (Row.Lane)
	{
	case ESiegeInputLane::PointerOnly:
		// ⛔ NO KEY EXISTS. The chip composer renders the pointer affordance instead.
		return DisplayKeys;

	case ESiegeInputLane::RawNonLetter:
		// ⛔ ZERO CALLS, AND IT IS A PROOF RATHER THAN A PREFERENCE: the translation table
		// holds A..Z and nothing else (SiegeKeyboardLayoutStatics.cpp:57-63), so
		// GetPositionalKey on a mouse button / Escape / the wheel is a provable identity
		// (the contract is spelled out at SiegeKeyboardLayoutSubsystem.h:238-240). Calling it
		// would be dead code that only invites someone to "generalise" it onto Lane A.
		DisplayKeys = Row.QwertyReferenceKeys;
		return DisplayKeys;

	case ESiegeInputLane::RawLetter:
		if (Row.bLiteralKeyLabel)
		{
			// ⛔ `KBD-§8` / `KBD-§0` ruling 1: THE LOOKUP IS FOR THE COMPARISON; THE LITERAL IS
			// FOR THE HUMAN (SiegeKeyboardLayoutSubsystem.h:256-260). The player is on QWERTY
			// HARDWARE with a Dvorak SOFTWARE layout — their keycap reads `Z`.
			DisplayKeys = Row.QwertyReferenceKeys;
			return DisplayKeys;
		}

		// ⭐ THE F-1 REVERSAL, IMPLEMENTED SO THE FLAG IS GENUINELY ONE FLAG: a raw letter
		// that is NOT pinned derives like any other raw key — EXACTLY ONE translation, because
		// nothing in Enhanced Input ever touched this key.
		DisplayKeys.Reserve(Row.QwertyReferenceKeys.Num());
		for (const FKey& ReferenceKey : Row.QwertyReferenceKeys)
		{
			DisplayKeys.Add(LayoutSubsystem ? LayoutSubsystem->GetPositionalKey(ReferenceKey) : ReferenceKey);
		}
		return DisplayKeys;

	case ESiegeInputLane::MappedAction:
	default:
		break;
	}

	// ── LANE A, THE PRIMARY PATH ────────────────────────────────────────────────────────
	// ⛔⛔ ZERO GetPositionalKey CALLS, AND THIS IS THE WHOLE DOUBLE-TRANSLATE AUDIT.
	// USiegeKeyboardLayoutSubsystem rewrites the APPLIED IMC duplicate's `.Key` fields
	// wholesale (SiegeKeyboardLayoutStatics.cpp:236), and QueryKeysMappedToAction reads the
	// ACTIVE contexts (EnhancedInputSubsystemInterface.h:381-384) — which ARE that retargeted
	// duplicate (HeroCharacter.cpp:271-275). ⇒ THE KEY ARRIVING HERE IS ALREADY TRANSLATED.
	// A second call would apply the map twice: invisible on QWERTY, and on US-Dvorak
	// `F` -> `U` -> `G`, teaching the wrong key to the one player this feature exists for
	// (SiegePlayerController.h:1222-1226).
	//
	// ⭐ AND THIS ROUTE SURVIVES A REBIND: re-key IMC_Hero and the row follows with ZERO
	// registry edits, because the registry never held the displayed truth.
	if (AppliedKeys.Num() > 0)
	{
		DisplayKeys = AppliedKeys;
		return DisplayKeys;
	}

	// ── LANE A, THE FALLBACK ────────────────────────────────────────────────────────────
	// Reached only when NO active context maps this action — the asset has not landed, or the
	// hero's context is not applied yet. ⇒ Nothing has translated this key, so applying the
	// map here is the FIRST and ONLY translation, not a second one.
	// ⛔ A null subsystem degrades to the reference key unchanged (`KBD-§5`'s fail-safe law:
	// the worst outcome this feature may produce is "the game behaves as it did yesterday").
	DisplayKeys.Reserve(Row.QwertyReferenceKeys.Num());
	for (const FKey& ReferenceKey : Row.QwertyReferenceKeys)
	{
		DisplayKeys.Add(LayoutSubsystem ? LayoutSubsystem->GetPositionalKey(ReferenceKey) : ReferenceKey);
	}
	return DisplayKeys;
}

FText FSiegeControlsHelpRegistry::ComposeKeyChipLabel(const FSiegeControlsHelpAction& Row, const TArray<FKey>& DisplayKeys)
{
	if (Row.bPointerOnly || Row.Lane == ESiegeInputLane::PointerOnly)
	{
		return FText::FromString(FString(SiegeControlsHelpText::PointerChip));
	}

	FString Composed;
	for (const FKey& DisplayKey : DisplayKeys)
	{
		// ⛔ THE ONLY PLACE A KEY BECOMES CHARACTERS IN THIS FEATURE, and the characters come
		// from the FKey itself (`HELP-§1`). bLongDisplayName is passed EXPLICITLY as false —
		// the short form ("LMB", "RMB") is what fits a chip; ⚠️ the engine's own default is
		// true, so omitting it would silently change the chip.
		FString KeyLabel = DisplayKey.GetDisplayName(/*bLongDisplayName=*/false).ToString();
		if (KeyLabel.IsEmpty())
		{
			// A key with no display name would render a blank chip. Its FName is never empty,
			// so this can degrade but never vanish (`HELP-§2` mechanism 2).
			KeyLabel = DisplayKey.ToString();
		}

		if (KeyLabel.IsEmpty())
		{
			continue;
		}

		if (!Composed.IsEmpty())
		{
			Composed += SiegeControlsHelpText::KeySeparator;
		}
		Composed += KeyLabel;
	}

	if (Composed.IsEmpty())
	{
		// ⛔ AN UNRESOLVABLE ACTION DOES NOT HIDE ITS ROW — it says so out loud
		// (`HELP-§2` mechanism 2: a visible gap gets fixed, a silent omission does not).
		return FText::FromString(FString(SiegeControlsHelpText::NotBoundChip));
	}

	return FText::FromString(Composed);
}

FText FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(const FSiegeControlsHelpAction& Row)
{
	if (Row.OneLine.IsEmptyOrWhitespace())
	{
		return FText::FromString(FString(SiegeControlsHelpText::Undocumented));
	}
	return Row.OneLine;
}

FText FSiegeControlsHelpRegistry::ComposeDetailForDisplay(const FSiegeControlsHelpAction& Row)
{
	// ⛔ TASK-707 READS THIS, NEVER Row.Detail RAW — reading the field directly would render a
	// BLANK page for every row until the prose lands, which is precisely the silent omission
	// `HELP-§2` mechanism 2 forbids.
	if (Row.Detail.IsEmptyOrWhitespace())
	{
		return FText::FromString(FString(SiegeControlsHelpText::Undocumented));
	}

	// ⭐ TASK-1576 (2026-09-28): every `{#Name}` number token is replaced here with the number READ
	// from its owner at this call (the derived-number block in the anonymous namespace above). This
	// is the one function every rendering of a page reads (the page's own body and each related
	// block both come through it), so a number shows the same value wherever the page appears.
	// Row.Detail itself stays the TYPED template, which is what the suite scans for typed digits.
	return SpliceDerivedNumbers(Row.Detail);
}

FText FSiegeControlsHelpRegistry::GetCategoryDisplayText(FName Category)
{
	using namespace SiegeControlsHelpText;

	if (Category == FName(CategoryHero))      { return FText::FromString(FString(HeaderHero)); }
	if (Category == FName(CategoryCards))     { return FText::FromString(FString(HeaderCards)); }
	if (Category == FName(CategoryOrders))    { return FText::FromString(FString(HeaderOrders)); }
	if (Category == FName(CategoryPickMode))  { return FText::FromString(FString(HeaderPickMode)); }
	if (Category == FName(CategoryInterface)) { return FText::FromString(FString(HeaderInterface)); }

	// ⛔ An unknown category renders its own name rather than nothing — a new category added
	// later surfaces with an ugly header, ⛔ never with an invisible group of rows.
	return FText::FromName(Category);
}

const TCHAR* FSiegeControlsHelpRegistry::GetUndocumentedText()
{
	return SiegeControlsHelpText::Undocumented;
}

FString FSiegeControlsHelpRegistry::MakeActionToken(FName ActionId)
{
	// ⭐ ONE definition of the token's shape. The prose above writes "{Orders.Attack}" and this is
	// the only code that says what those braces mean, so the two cannot disagree.
	return FString(SiegeControlsHelpText::ActionTokenOpen)
		+ ActionId.ToString()
		+ FString(SiegeControlsHelpText::ActionTokenClose);
}

FText FSiegeControlsHelpRegistry::ResolveDetailTokens(const FText& DetailText, TFunctionRef<FText(FName)> ChipProvider)
{
	FString Working = DetailText.ToString();

	// ⚠️ THE CHEAP GATE FIRST, AND IT IS NOT MICRO-OPTIMISATION: without it, every detail page
	// would run a live Enhanced Input query for EVERY registry row even though most pages contain
	// no token at all. With it, a page pays only for the tokens it actually has.
	// ⚠️ The counts this comment used to carry ("all 24 … 20 of the 24") were removed by TASK-823
	// rather than re-typed: the registry grows, and a transcribed count is a fact that rots.
	if (!Working.Contains(SiegeControlsHelpText::ActionTokenOpen, ESearchCase::CaseSensitive))
	{
		return DetailText;
	}

	// ⛔ MATCHED AGAINST REAL REGISTRY IDS ONLY, ⛔ never against "anything in braces". Two
	// consequences, both deliberate:
	//   • an ordinary brace someone writes in prose later is left alone;
	//   • ⭐ a MISSPELLED token survives ON SCREEN rather than being silently deleted — a VISIBLE
	//     gap gets fixed, a silent omission does not (`HELP-§2` mechanism 2). The suite asserts
	//     that no shipped row leaves one behind, on both simulated layouts.
	bool bReplacedAny = false;
	for (const FSiegeControlsHelpAction& Row : GetActions())
	{
		const FString Token = MakeActionToken(Row.ActionId);

		// ⚠️ CASE-SENSITIVE, PASSED EXPLICITLY: the engine's default for both Contains and
		// ReplaceInline is ESearchCase::IgnoreCase, and an ActionId is a case-sensitive identity.
		if (!Working.Contains(Token, ESearchCase::CaseSensitive))
		{
			continue;
		}

		// ⛔⛔ THE ONE PLACE A KEY BECOMES CHARACTERS IN THE DETAIL LANE, and it is the SAME place
		// as the row lane: the provider hands back ComposeKeyChipLabel's answer over
		// ResolveRowDisplayKeys' derived FKeys (`HELP-§1`). ⛔ No letter is typed in prose.
		Working.ReplaceInline(*Token, *ChipProvider(Row.ActionId).ToString(), ESearchCase::CaseSensitive);
		bReplacedAny = true;
	}

	return bReplacedAny ? FText::FromString(Working) : DetailText;
}

FSiegeControlsDetailContent FSiegeControlsHelpRegistry::ComposeDetailContent(
	const FSiegeControlsHelpAction& Row,
	const USiegeKeyboardLayoutSubsystem* LayoutSubsystem,
	TFunctionRef<TArray<FKey>(const FSiegeControlsHelpAction&)> AppliedKeyProvider)
{
	// The one chip lane, reused for the page's own key, for every related block, and for every
	// {ActionId} token in the prose — so all three go through ResolveRowDisplayKeys' lane audit
	// and there is no second way for a key to reach the screen (`HELP-§1`).
	auto ComposeChipForRow = [&AppliedKeyProvider, LayoutSubsystem](const FSiegeControlsHelpAction& ChipRow) -> FText
	{
		const TArray<FKey> AppliedKeys = AppliedKeyProvider(ChipRow);
		return ComposeKeyChipLabel(ChipRow, ResolveRowDisplayKeys(ChipRow, AppliedKeys, LayoutSubsystem));
	};

	auto ComposeChipForId = [&ComposeChipForRow](FName ChipActionId) -> FText
	{
		const FSiegeControlsHelpAction* const ChipRow = FindAction(ChipActionId);

		// ⛔ A token naming an id that is not in the registry resolves to the explicit "not bound"
		// chip rather than to an empty string: the sentence stays readable and the gap is visible.
		return ChipRow != nullptr
			? ComposeChipForRow(*ChipRow)
			: FText::FromString(FString(SiegeControlsHelpText::NotBoundChip));
	};

	FSiegeControlsDetailContent Content;
	Content.ActionId = Row.ActionId;
	Content.Title    = Row.DisplayName;
	Content.KeyChip  = ComposeChipForRow(Row);
	Content.Summary  = ComposeOneLineForDisplay(Row);

	// ⛔ THROUGH ComposeDetailForDisplay, ⛔ NEVER Row.Detail RAW (TASK-706 §4d): the raw field
	// would render a BLANK page for an undocumented row instead of the pinned TODO string.
	Content.Body = ResolveDetailTokens(ComposeDetailForDisplay(Row), ComposeChipForId);

	// ⭐ JONATHAN'S "ALL THE CONTROLS WITH IT". ⛔ ONE LEVEL DEEP, STRUCTURALLY: this loop reads
	// each related row's OWN text and ⛔ never touches RelatedActionIds again, so the mutual
	// references the pick-mode rows carry cannot recurse and cannot repeat a page inside itself.
	Content.Related.Reserve(Row.RelatedActionIds.Num());
	for (const FName RelatedId : Row.RelatedActionIds)
	{
		// ⛔ A row never lists itself: a page that rendered its own prose twice would read as a
		// duplication bug even though the data caused it.
		if (RelatedId == Row.ActionId || RelatedId.IsNone())
		{
			continue;
		}

		const FSiegeControlsHelpAction* const RelatedRow = FindAction(RelatedId);
		if (RelatedRow == nullptr)
		{
			// ⛔ DROPPED, not rendered blank. An id that names nothing has no text to show and no
			// key to derive; a blank block would be the silent omission `HELP-§2` forbids.
			continue;
		}

		FSiegeControlsDetailEntry& Entry = Content.Related.AddDefaulted_GetRef();
		Entry.ActionId    = RelatedRow->ActionId;
		Entry.KeyChip     = ComposeChipForRow(*RelatedRow);
		Entry.DisplayName = RelatedRow->DisplayName;

		// ⭐ THE RELATED BLOCK RENDERS THAT ROW'S OWN DETAIL PROSE — ⛔ never a second copy of it.
		// That is what makes the Ambush page answer "what the first, second and third circles do,
		// how to resize them, how to exit the command" with ZERO duplicated text anywhere in the
		// feature: one definition, two renderings, nothing to drift (`HELP-§2`).
		Entry.Body = ResolveDetailTokens(ComposeDetailForDisplay(*RelatedRow), ComposeChipForId);
	}

	return Content;
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  USiegeControlsHelpRowWidget
// ════════════════════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> USiegeControlsHelpRowWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING (`HELP-§3`, the corrected TASK-444 shape, cloned from
	// UAccountMenuWidget / UDeckSlotEntryWidget). UUserWidget::RebuildWidget() reads
	// WidgetTree->RootWidget AS IT STANDS when called and returns an SSpacer when it is null
	// (UserWidget.cpp:1214) — so the code-authored tree MUST exist BEFORE Super, and anything
	// built after Super is silently discarded while every property readback still passes.
	//
	// Initialize() FIRST — the WARN-437-1 hardening: WidgetTree is allocated inside
	// Initialize(), which is public and idempotent, so the call is free.
	Initialize();
	ConstructRowTree();

	// The stamp may have landed BEFORE the tree existed (the BLOCKER 674-1 ordering hole):
	// re-apply it now that the children are real.
	ApplyStoredContent();

	return Super::RebuildWidget();
}

void USiegeControlsHelpRowWidget::ConstructRowTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Row '%s': no WidgetTree - the row cannot build its tree."), *ActionId.ToString());
		return;
	}

	// THE `HELP-§3` ESCAPE HATCH: an asset-authored /Game/UI/WBP_ControlsHelpRow wins WHOLE —
	// UMG has already resolved the BindWidgetOptional members from it, so there is nothing to
	// construct and nothing to overwrite. That asset is RESERVED and unauthored today.
	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	// ---- RowButton: the clickable surface AND the tree root ---------------------------
	// The WHOLE row is the button, which is what makes "clicking a row" (Jonathan's ask) true
	// of the description text as well as of the chip.
	if (RowButton == nullptr)
	{
		RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowButton"));
	}

	if (RowButton == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Row '%s': could not construct RowButton - the row has no root and cannot be clicked."), *ActionId.ToString());
		return;
	}

	// ═══ 🚨🚨 TASK-1478 [CONTROLS-HELP-NAVIGABLE] — ⛔ FLIPPED. THIS BUTTON ⛔ IS THE LIST'S RING ══
	//  ⛔ WAS: ~~"NOT FOCUSABLE, AND THAT IS CORRECTNESS RATHER THAN POLISH: `Tab` is Slate's own
	//  focus-next key. Clicking a row is the whole point of this screen, and a focusable SButton
	//  takes keyboard focus on that click — after which `Tab` would navigate focus instead of
	//  reaching Enhanced Input, and the overlay's own toggle key would stop closing it."~~
	//
	//  ⛔ STRUCK, ⛔ NOT DELETED (`SC-§120`), because it was ⛔ TRUE WHEN WRITTEN and the thing that
	//  falsified it lives in ⛔ THIS SAME FILE. ⭐ `TASK-1432` added
	//  `USiegeControlsHelpWidget::NativeOnPreviewKeyDown`, which claims this overlay's ⛔ DERIVED
	//  toggle key in the ⛔ PREVIEW (tunnelling) phase. Slate routes preview key events ⛔ down the
	//  whole ancestor chain of the focused widget ⛔ BEFORE the focused `SButton` sees the key and
	//  ⛔ BEFORE `FSlateApplication` attempts navigation, so a `Handled()` there short-circuits
	//  Slate's own `Tab`-is-focus-next rule outright.
	//  ⭐ AND THAT COVER IS A ⛔ PROPERTY, ⛔ NOT A COINCIDENCE OF TODAY'S TREE: ⛔ every button this
	//  row makes focusable is a ⛔ DESCENDANT of `USiegeControlsHelpWidget`, so that overlay is on
	//  the focus path ⛔ by construction for every one of them. ⛔ The close route therefore survives
	//  the flip ⛔ without the handler being touched — and it was ⛔ re-measured rather than assumed
	//  (⭐ `TASK-1478`; see that function's own note).
	//
	//  ⛔ AND THE ⛔ SECOND HALF OF THE OLD REFUSAL IS GONE TOO. The `CloseButton` site's per-site
	//  table said flipping this button would be ⛔ INERT, because `UWidgetTree::ForWidgetAndChildren`
	//  never entered a nested `UUserWidget`'s own tree. ⭐ `TASK-1474` made the walker ⛔ DESCEND
	//  (`USiegeMenuInputSubsystem::CollectNavStopsFromTree`, gated on `IsCodeAuthoredSubWidget`, and
	//  `USiegeControlsHelpRowWidget` is a `UCLASS()` in `Source/` ⇒ `CLASS_Native` ⇒ entered).
	//  ⇒ ⛔ THE ROW OBJECT IS NOW WALKED INTO and this button ⛔ IS collected. ⛔ Neither half of the
	//  old refusal was overruled on taste; ⛔ both were measured false by later rows.
	//
	//  ⭐ AND IT IS WHAT MAKES A ROW ⛔ REACHABLE AT ALL, which is the whole ask: `Enter`/`SpaceBar`
	//  on a focused `SButton` runs `ExecuteOnClick()` → `OnClicked` → `HandleRowButtonClicked` →
	//  `ActivateRow` → the overlay's `HandleRowActivated`. ⛔ That is the ONLY keyboard route into a
	//  detail page, and `NativeOnPreviewKeyDown` deliberately hands those two keys straight on.
	//
	//  🚨🚨 ⛔ TRIP-WIRE — ⛔ READ `USiegeControlsHelpWidget::ApplyActiveView` BEFORE TOUCHING THIS
	//  LINE. ⛔ This button lives in the switcher's ⛔ LIST branch. A focusable button parked in an
	//  ⛔ INACTIVE `SWidgetSwitcher` branch is ⛔ ADMITTED by the walker and ⛔ REFUSES focus
	//  (`SWidgetSwitcher::ValidatePathToChild` → `return InChild == GetActiveWidget().Get();`), and
	//  the subsystem's own words are that such a stop ⛔ SWALLOWS THE RING rather than being skipped.
	//  ⛔ THE ONLY THING PREVENTING THAT HERE IS `ApplyActiveView` COLLAPSING WHICHEVER BRANCH IS
	//  INACTIVE. ⇒ ⛔ IF THAT COLLAPSE IS REMOVED, BYPASSED OR MIS-TARGETED, THIS WRITE MUST GO BACK
	//  TO `ApplyButtonNotFocusable` ⛔ IN THE SAME DIFF.
	ApplyButtonFocusable(RowButton);

	RowButton->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.06f));

	WidgetTree->RootWidget = RowButton;

	// ---- RowContentBox: chip | text column --------------------------------------------
	if (RowContentBox == nullptr)
	{
		RowContentBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowContentBox"));
	}

	if (RowContentBox == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Row '%s': could not construct RowContentBox - the row renders empty."), *ActionId.ToString());
		return;
	}

	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(RowButton->SetContent(RowContentBox)))
	{
		ContentSlot->SetPadding(FMargin(12.f, 8.f, 12.f, 8.f));
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}

	// ---- KeyChipBorder + KeyChipText: the keycap ---------------------------------------
	if (KeyChipBorder == nullptr)
	{
		KeyChipBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("KeyChipBorder"));
		if (KeyChipBorder != nullptr)
		{
			KeyChipBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.95f));
			KeyChipBorder->SetPadding(FMargin(10.f, 5.f, 10.f, 5.f));
			KeyChipBorder->SetHorizontalAlignment(HAlign_Center);
			KeyChipBorder->SetVerticalAlignment(VAlign_Center);

			if (UHorizontalBoxSlot* ChipSlot = RowContentBox->AddChildToHorizontalBox(KeyChipBorder))
			{
				ChipSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
				ChipSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (KeyChipText == nullptr && KeyChipBorder != nullptr)
	{
		// ⛔ NO TEXT IS SET HERE. The chip's characters arrive ONLY through SetRowContent,
		// composed by FSiegeControlsHelpRegistry::ComposeKeyChipLabel from a derived FKey
		// (`HELP-§1`). A default string here would be a hardcoded key by another name.
		KeyChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("KeyChipText"));
		if (KeyChipText != nullptr)
		{
			KeyChipText->SetFontSize(20.f);
			KeyChipText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.72f, 1.f)));
			KeyChipBorder->SetContent(KeyChipText);
		}
	}

	// ---- RowTextBox: name over description ---------------------------------------------
	if (RowTextBox == nullptr)
	{
		RowTextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RowTextBox"));
		if (RowTextBox != nullptr)
		{
			if (UHorizontalBoxSlot* TextSlot = RowContentBox->AddChildToHorizontalBox(RowTextBox))
			{
				TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				TextSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (RowTextBox == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] Row '%s': could not construct RowTextBox - the row renders its chip only."), *ActionId.ToString());
		return;
	}

	if (ActionNameText == nullptr)
	{
		ActionNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ActionNameText"));
		if (ActionNameText != nullptr)
		{
			ActionNameText->SetFontSize(20.f);
			RowTextBox->AddChildToVerticalBox(ActionNameText);
		}
	}

	if (DescriptionText == nullptr)
	{
		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
		if (DescriptionText != nullptr)
		{
			DescriptionText->SetFontSize(16.f);
			DescriptionText->SetAutoWrapText(true);
			DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.84f, 0.88f, 1.f)));
			RowTextBox->AddChildToVerticalBox(DescriptionText);
		}
	}
}

void USiegeControlsHelpRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bound HERE and not in NativeOnInitialized (the settings-lane lifecycle trap, cloned via
	// UAccountMenuWidget/UDeckSlotEntryWidget): the code-authored children do not exist until
	// RebuildWidget() runs, and the engine order is Initialize() -> NativeOnInitialized() ->
	// RebuildWidget() -> NativeConstruct().
	if (RowButton != nullptr)
	{
		RowButton->OnClicked.AddUniqueDynamic(this, &USiegeControlsHelpRowWidget::HandleRowButtonClicked);
	}
}

void USiegeControlsHelpRowWidget::NativeDestruct()
{
	if (RowButton != nullptr)
	{
		RowButton->OnClicked.RemoveDynamic(this, &USiegeControlsHelpRowWidget::HandleRowButtonClicked);
	}

	Super::NativeDestruct();
}

void USiegeControlsHelpRowWidget::HandleRowButtonClicked()
{
	ActivateRow();
}

void USiegeControlsHelpRowWidget::ActivateRow()
{
	if (ActionId.IsNone())
	{
		// ⛔ An unstamped row must never report a click: a NAME_None arriving at
		// HandleRowActivated would be refused there anyway, but reporting it at all would make
		// a construction bug look like a player action.
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] A row was activated before SetRowContent stamped it - the click is dropped."));
		return;
	}

	// ⛔ THE MEANING LIVES IN THE RECEIVER (UDeckSlotEntryWidget's contract): this row reports
	// its identity and nothing else. It opens no view, changes no state, and knows nothing
	// about TASK-707's detail page.
	OnRowActivated.ExecuteIfBound(ActionId);
}

void USiegeControlsHelpRowWidget::SetRowContent(FName InActionId, const FText& InDisplayName, const FText& InOneLine, const FText& InKeyChip)
{
	// Stored UNCONDITIONALLY — ⛔ never gate the stamp on child liveness. The overlay stamps a
	// CreateWidget-fresh row BEFORE AddChild lazily triggers its rebuild, so at stamp time the
	// children are still null and a guarded SetText would silently render every row blank
	// (BLOCKER 674-1, paid for once already).
	ActionId          = InActionId;
	StoredDisplayName = InDisplayName;
	StoredOneLine     = InOneLine;
	StoredKeyChip     = InKeyChip;

	ApplyStoredContent();
}

void USiegeControlsHelpRowWidget::ApplyStoredContent()
{
	// The stamp fence keeps an un-stamped row from blanking an asset-authored label via the
	// `HELP-§3` escape hatch. Idempotent: safe at every rebuild.
	if (ActionId.IsNone())
	{
		return;
	}

	if (KeyChipText != nullptr)
	{
		KeyChipText->SetText(StoredKeyChip);
	}
	if (ActionNameText != nullptr)
	{
		ActionNameText->SetText(StoredDisplayName);
	}
	if (DescriptionText != nullptr)
	{
		DescriptionText->SetText(StoredOneLine);
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  USiegeControlsDetailWidget — THE FULL-SCREEN DETAIL VIEW (TASK-707)
//
//  ⛔ NO KEY HANDLER OF ANY KIND EXISTS BELOW. In particular there is no NativeOnKeyDown, no
//     NativeOnPreviewKeyDown, no Enhanced Input action and no FReply::Handled() on any key, so
//     `Escape` stays permanently unabsorbed (`AS-§6 A-2`, `HELP-§5`). The page's way back is a
//     BUTTON, and that is the entire reason it is a button.
//  ⛔ NO SetInputMode AND NO bShowMouseCursor. The overlay's ONE registration in
//     ApplyCursorInputState() covers this view too (TASK-074's law).
// ════════════════════════════════════════════════════════════════════════════════════════════

TSharedRef<SWidget> USiegeControlsDetailWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING — the same UserWidget.cpp:1214 SSpacer trap the row and the
	// overlay both document. Tree first, RootWidget set, THEN Super.
	Initialize();
	ConstructDetailTree();

	// The stamp may have landed BEFORE the tree existed (the BLOCKER 674-1 ordering hole) — and
	// here it usually HAS, because the overlay constructs this view once and stamps it on every
	// click, long before the switcher first builds it.
	ApplyStoredContent();

	return Super::RebuildWidget();
}

void USiegeControlsDetailWidget::ConstructDetailTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Detail view: no WidgetTree - the page cannot build its tree."));
		return;
	}

	// THE `HELP-§3` ESCAPE HATCH: an asset-authored /Game/UI/WBP_ControlsDetail wins WHOLE.
	// RESERVED and unauthored today.
	if (WidgetTree->RootWidget != nullptr)
	{
		return;
	}

	// ---- DetailBackdrop: the full-screen plate, and the tree root -----------------------
	if (DetailBackdrop == nullptr)
	{
		DetailBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailBackdrop"));
	}

	if (DetailBackdrop == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Detail view: could not construct DetailBackdrop - the page has no root."));
		return;
	}

	// ⚠️ `Visible`, ⛔ NOT SelfHitTestInvisible — and the difference from the overlay's BACKDROP is
	// the geometry, not a change of mind. USiegeControlsHelpWidget::BackdropBorder is
	// hit-test-transparent because it is a MARGIN around a small panel and the live match must
	// keep receiving the clicks that fall in it. This plate IS the page the player is reading, at
	// the size Jonathan asked for ("the entire screen"), and a plate you read must absorb its own
	// clicks or a click on a sentence places a card in the world underneath it. That is exactly
	// the rule the list's PanelBorder already follows; this one is simply screen-sized.
	DetailBackdrop->SetVisibility(ESlateVisibility::Visible);
	DetailBackdrop->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.97f));
	DetailBackdrop->SetPadding(FMargin(0.f));
	DetailBackdrop->SetHorizontalAlignment(HAlign_Fill);
	DetailBackdrop->SetVerticalAlignment(VAlign_Fill);

	WidgetTree->RootWidget = DetailBackdrop;

	// ---- DetailColumn ---------------------------------------------------------------------
	if (DetailColumn == nullptr)
	{
		DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DetailColumn"));
	}

	if (DetailColumn == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Detail view: could not construct DetailColumn - the page has no content column."));
		return;
	}

	DetailBackdrop->SetContent(DetailColumn);

	// ---- DetailHeaderBox: chip | title -----------------------------------------------------
	if (DetailHeaderBox == nullptr)
	{
		DetailHeaderBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DetailHeaderBox"));
		if (DetailHeaderBox != nullptr)
		{
			if (UVerticalBoxSlot* HeaderSlot = DetailColumn->AddChildToVerticalBox(DetailHeaderBox))
			{
				HeaderSlot->SetPadding(FMargin(56.f, 40.f, 56.f, 8.f));
				HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
				HeaderSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	if (DetailHeaderBox != nullptr)
	{
		if (DetailKeyChipBorder == nullptr)
		{
			DetailKeyChipBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailKeyChipBorder"));
			if (DetailKeyChipBorder != nullptr)
			{
				DetailKeyChipBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.95f));
				DetailKeyChipBorder->SetPadding(FMargin(14.f, 7.f, 14.f, 7.f));
				DetailKeyChipBorder->SetHorizontalAlignment(HAlign_Center);
				DetailKeyChipBorder->SetVerticalAlignment(VAlign_Center);

				if (UHorizontalBoxSlot* ChipSlot = DetailHeaderBox->AddChildToHorizontalBox(DetailKeyChipBorder))
				{
					ChipSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
					ChipSlot->SetVerticalAlignment(VAlign_Center);
				}
			}
		}

		if (DetailKeyChipText == nullptr && DetailKeyChipBorder != nullptr)
		{
			// ⛔ NO TEXT SET HERE. The chip's characters arrive ONLY through SetDetailContent, and
			// they were composed from a DERIVED FKey (`HELP-§1`). A default string here would be a
			// hardcoded key by another name.
			DetailKeyChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailKeyChipText"));
			if (DetailKeyChipText != nullptr)
			{
				DetailKeyChipText->SetFontSize(26.f);
				DetailKeyChipText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.72f, 1.f)));
				DetailKeyChipBorder->SetContent(DetailKeyChipText);
			}
		}

		if (DetailTitleText == nullptr)
		{
			DetailTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailTitleText"));
			if (DetailTitleText != nullptr)
			{
				DetailTitleText->SetFontSize(40.f);
				if (UHorizontalBoxSlot* TitleSlot = DetailHeaderBox->AddChildToHorizontalBox(DetailTitleText))
				{
					TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					TitleSlot->SetVerticalAlignment(VAlign_Center);
				}
			}
		}
	}

	// ---- DetailSummaryText: the one-liner, restated at the top ------------------------------
	if (DetailSummaryText == nullptr)
	{
		DetailSummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailSummaryText"));
		if (DetailSummaryText != nullptr)
		{
			DetailSummaryText->SetFontSize(22.f);
			DetailSummaryText->SetAutoWrapText(true);
			DetailSummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.80f, 0.45f, 1.f)));

			if (UVerticalBoxSlot* SummarySlot = DetailColumn->AddChildToVerticalBox(DetailSummaryText))
			{
				SummarySlot->SetPadding(FMargin(56.f, 0.f, 56.f, 16.f));
				SummarySlot->SetHorizontalAlignment(HAlign_Fill);
				SummarySlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- DetailScrollBox: the body and the related blocks scroll together -------------------
	// ⚠️ THE BODY MUST SCROLL. 704's detail prose runs to several paragraphs on the order pages,
	// and a page that silently clipped its last paragraph would be the anti-staleness law's own
	// failure mode wearing a layout bug's clothes.
	if (DetailScrollBox == nullptr)
	{
		DetailScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("DetailScrollBox"));
		if (DetailScrollBox != nullptr)
		{
			DetailScrollBox->SetOrientation(Orient_Vertical);
			DetailScrollBox->SetAlwaysShowScrollbar(true);

			// ⛔ STILL NOT FOCUSABLE (`SC-§120`, same amendment as `RowScrollBox`).
			// ⛔ WAS: ~~"the same `Tab`-is-Slate's-own-focus-key reasoning as the list."~~ — ⭐
			// `TASK-1478` struck that reasoning at the `RowButton` site. ⛔ THE CONCLUSION IS
			// UNCHANGED: a `UScrollBox` is ⛔ not one of `IsNavFocusStop`'s four admitted classes, so
			// flipping it moves ⛔ nothing. ⛔ AND NO `SetScrollWhenFocusChanges` COMPANION IS ADDED
			// HERE, deliberately: this page's ⛔ only stop is `BackButton`, which is a sibling of this
			// box in `DetailColumn` and ⛔ never inside it, so there is ⛔ no focus change within this
			// box to scroll to. ⛔ Adding one would be a write with no reachable effect.
			//
			// 🚨 ⭐⭐ TASK-1484 — ⛔ ONE PREMISE OF THAT LAST SENTENCE EXPIRED AND ⛔ THE CONCLUSION DID
			// NOT, so it is ⛔ AMENDED RATHER THAN STRUCK (`SC-§120`). ⛔ The page now has ⛔ TWO stops,
			// ⛔ not one — `DetailScrollButton` was added below. ⛔ BUT IT IS A ⛔ SIBLING OF THIS BOX
			// TOO, ⛔ deliberately and for this exact reason: a stop placed ⛔ INSIDE the box would make
			// `ScrollWhenFocusChanges` scroll ⛔ to that one widget and ⛔ nowhere else — ⛔ ONE JUMP to
			// wherever it sits, which on a long page ⛔ SKIPS everything between. ⇒ ⛔ there is ⛔ still
			// no focus change ⛔ within this box, ⛔ still nothing for a `SetScrollWhenFocusChanges`
			// companion to do here, and the body is paged by an ⛔ EXPLICIT offset write
			// (`AdvanceBodyScroll`) that can walk a page of ⛔ any length.
			//
			// 🚨 ⭐ TASK-1496 — ⛔ THAT PREMISE EXPIRED IN TURN AND ⛔ THE CONCLUSION ⛔ STILL DID NOT.
			// ⛔ 🧑 His ruling of 2026-09-26 removed `DetailScrollButton` and `AdvanceBodyScroll`, so
			// ⛔ the page is back to ⛔ ONE stop (`BackButton`) — ⛔ which is a ⛔ SIBLING of this box
			// exactly as the ⛔ original sentence above said. ⇒ ⛔ THE THREE READINGS ALL AGREE AND
			// ⛔ ALWAYS DID: ⛔ still ⛔ no focus change ⛔ within this box, ⛔ still ⛔ nothing for a
			// `SetScrollWhenFocusChanges` companion to do ⛔ HERE. 🚨 ⛔ DO NOT CONFUSE THIS WITH
			// `RowScrollBox`'s ⛔ OWN `SetScrollWhenFocusChanges(InstantScroll)` — ⛔ that one is the
			// ⛔ 28-ROW LIST's scroll-follows-focus, it has ⛔ 27 real focus changes inside it, and it
			// ⛔ STAYS.
			// ⛔⛔ AND THE THREE AUTHORED PROPERTIES ON THIS BOX WERE ⛔ UNTOUCHED BY ⛔ EITHER ROW —
			// `SetAlwaysShowScrollbar(true)` above, this `SetIsFocusable(false)`, and the
			// `WhenScrollingPossible` below: 🧑 THE WHEEL PATH IS ⛔ BYTE-FOR-BYTE WHAT SHIPPED, and
			// ⛔ this box itself is ⛔ NOT part of what `TASK-1496` removed.
			DetailScrollBox->SetIsFocusable(false);
			DetailScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);

			if (UVerticalBoxSlot* BodySlot = DetailColumn->AddChildToVerticalBox(DetailScrollBox))
			{
				BodySlot->SetPadding(FMargin(56.f, 0.f, 56.f, 12.f));
				BodySlot->SetHorizontalAlignment(HAlign_Fill);
				BodySlot->SetVerticalAlignment(VAlign_Fill);
				BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}

	if (DetailScrollBox == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Detail view: could not construct DetailScrollBox - the page can show no prose."));
	}
	else
	{
		if (DetailBodyText == nullptr)
		{
			DetailBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DetailBodyText"));
			if (DetailBodyText != nullptr)
			{
				DetailBodyText->SetFontSize(19.f);
				DetailBodyText->SetAutoWrapText(true);
				DetailBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.90f, 0.94f, 1.f)));

				if (UScrollBoxSlot* BodyTextSlot = Cast<UScrollBoxSlot>(DetailScrollBox->AddChild(DetailBodyText)))
				{
					BodyTextSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 18.f));
					BodyTextSlot->SetHorizontalAlignment(HAlign_Fill);
				}
			}
		}

		// ---- The related-controls heading, hidden until a page actually has some ------------
		if (RelatedHeaderText == nullptr)
		{
			RelatedHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RelatedHeaderText"));
			if (RelatedHeaderText != nullptr)
			{
				RelatedHeaderText->SetText(FText::FromString(FString(SiegeControlsHelpText::RelatedHeader)));
				RelatedHeaderText->SetFontSize(26.f);
				RelatedHeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.80f, 0.45f, 1.f)));

				// Collapsed until ApplyStoredContent proves there is something under it — an empty
				// heading is a promise the page does not keep.
				RelatedHeaderText->SetVisibility(ESlateVisibility::Collapsed);

				if (UScrollBoxSlot* RelatedHeaderSlot = Cast<UScrollBoxSlot>(DetailScrollBox->AddChild(RelatedHeaderText)))
				{
					RelatedHeaderSlot->SetPadding(FMargin(0.f, 10.f, 16.f, 10.f));
					RelatedHeaderSlot->SetHorizontalAlignment(HAlign_Left);
				}
			}
		}

		if (RelatedBox == nullptr)
		{
			RelatedBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RelatedBox"));
			if (RelatedBox != nullptr)
			{
				if (UScrollBoxSlot* RelatedSlot = Cast<UScrollBoxSlot>(DetailScrollBox->AddChild(RelatedBox)))
				{
					RelatedSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 24.f));
					RelatedSlot->SetHorizontalAlignment(HAlign_Fill);
				}
			}
		}
	}

	// ---- BackButton: the page's way out, and ⛔ deliberately not a key ----------------------
	if (BackLabelText == nullptr)
	{
		BackLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackLabelText"));
		if (BackLabelText != nullptr)
		{
			BackLabelText->SetText(FText::FromString(FString(SiegeControlsHelpText::BackLabel)));
			BackLabelText->SetFontSize(24.f);
		}
	}

	if (BackButton == nullptr)
	{
		BackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
		if (BackButton != nullptr)
		{
			// ═══ 🚨🚨 TASK-1478 [CONTROLS-HELP-NAVIGABLE] — ⛔ FLIPPED. THIS BUTTON ⛔ IS THE DETAIL
			//  PAGE'S ⛔ ENTIRE RING, and it is what makes 🧑 *"can you get into a row's detail page
			//  and back out ⛔ without the mouse?"* answerable with anything but ⛔ no.
			//
			//  ⛔ WAS: ~~"NOT FOCUSABLE — see USiegeControlsHelpRowWidget's RowButton comment."~~
			//  ⛔ STRUCK, ⛔ NOT DELETED (`SC-§120`). ⛔ BOTH halves of what it pointed at are gone,
			//  and ⛔ neither was overruled on taste: (i) that comment's `Tab`-is-Slate's-focus-key
			//  argument was ⛔ struck at its own site, because ⭐ `TASK-1432`'s
			//  `NativeOnPreviewKeyDown` claims the derived toggle key in the ⛔ PREVIEW phase on
			//  ⛔ every ancestor of the focused widget; (ii) the `CloseButton` site's ⛔ STRUCTURAL
			//  refusal (*"the walker never enters a nested `UUserWidget`'s tree"*) was ⛔ measured
			//  FALSE by ⭐ `TASK-1474`'s descent — `USiegeControlsDetailWidget` is a `UCLASS()` in
			//  `Source/` ⇒ `CLASS_Native` ⇒ `IsCodeAuthoredSubWidget` admits it ⇒ this tree ⛔ IS
			//  walked into.
			//
			//  ⭐ ⛔ EXACTLY ONE STOP (~~⭐ `TASK-1484`: ⛔ **TWO**~~ — ⛔ STRUCK at ⭐ `TASK-1496`; see
			//  the amendment at the foot of this block), ⛔ BY THE STRUCTURE OF `ConstructDetailTree`
			//  RATHER THAN BY A
			//  TALLY — which is what makes the count survive a page whose content changes: this is
			//  the ⛔ ONLY `UButton` this tree builds; `DetailScrollBox` is a `UScrollBox` (⛔ not an
			//  admitted class and authored non-focusable besides); `RebuildRelatedBlocks` constructs
			//  ⛔ only `UHorizontalBox` / `UBorder` / `UTextBlock` / `UVerticalBox`; everything else
			//  is a border, a text block or a box. ⇒ ⛔ StopSet(detail) = `{ BackButton }` for
			//  ⛔ every row, ⛔ every page, ⛔ however many related blocks it grows.
			//
			//  🚨🚨 ⭐ TASK-1496 — ⛔ THE COUNT WENT ⛔ 1 → 2 → ⛔ **1**, AND ⛔ THE GENERATING RULE WAS
			//  ⛔ THE SAME ONE AT EVERY VALUE ⇒ ⛔ state the ⛔ RULE, ⛔ never the integer (⚖️ *a gate
			//  written in indices has an expiry date nobody printed on it*).
			//  ⛔ ~~`DetailScrollButton` is the SECOND `UButton` this tree builds ⇒ StopSet(detail) =
			//  `{ BackButton, DetailScrollButton }`~~ — ⛔ STRUCK, ⛔ not deleted (`SC-§120`): 🧑 he
			//  ruled the control out on ⛔ 2026-09-26 after ⭐ `TASK-1494` measured that ⛔ no detail
			//  page overflows, so ⛔ this is once more the ⛔ ONLY `UButton` `ConstructDetailTree`
			//  builds ⇒ ⛔ **StopSet(detail) = `{ BackButton }`**, ⛔ for ⛔ every row and ⛔ every page.
			//  ⛔ `BackButton` was ⛔ STOP 0 under ⛔ BOTH counts — under two, because the second button
			//  was added ⛔ LAST to `DetailColumn` and `CollectNavStopsFromTree` walks depth-first
			//  ⛔ PRE-ORDER; under one, because it is the ⛔ only member. ⇒ ⭐ `TASK-1478`'s *"the ring
			//  lands on `BackButton`"* is ⛔ true across ⛔ all three values and was ⛔ never at risk.
			//
			// ═══ 🚨🚨🚨 ⛔ THE TRIP-WIRE (`qa/TASK-1433.md` WARN-L1) — PLANTED HERE BECAUSE ⛔ THIS
			//  IS THE LINE THAT TRIPS IT, AND THE FAILURE IT GUARDS IS ⛔ SILENT ══════════════════
			//
			//  ⛔ **IF `BackButton` IS FOCUSABLE — IT IS, ON THE LINE BELOW — THEN THIS SCREEN'S RING
			//  IS CORRECT ⛔ ONLY BECAUSE `USiegeControlsHelpWidget::ApplyActiveView` COLLAPSES
			//  WHICHEVER `ViewSwitcher` BRANCH IS ⛔ INACTIVE. ⛔ REMOVE, BYPASS OR MIS-TARGET THAT
			//  COLLAPSE AND THIS BUTTON BECOMES AN ⛔ ADMITTED-BUT-UNFOCUSABLE STOP THAT ⛔ SWALLOWS
			//  THE RING ⛔ EVERY TIME THE LIST IS UP — ⛔ with every visibility flag reading green and
			//  ⛔ nothing in any log.**
			//
			//  ⛔ THE MECHANISM, NAMED SO THE NEXT EDITOR DOES NOT HAVE TO HUNT IT:
			//    • `SWidget::ValidatePathToChild` is a ⛔ no-op `return true` for ⛔ every widget —
			//      ⛔ except `SWidgetSwitcher`, which overrides it as
			//      `return InChild == GetActiveWidget().Get();`
			//      (`Slate/Private/Widgets/Layout/SWidgetSwitcher.cpp:169-172`, re-read live).
			//    • `FSlateWindowHelper::FindPathToWidget` calls it under the engine's ⛔ OWN comment
			//      naming `SWidgetSwitcher`, then ⛔ empties the path and returns false.
			//    • ⇒ `FSlateApplication::SetUserFocus` ⛔ REFUSES, while `IsNavFocusStop` — which
			//      models ancestor ⛔ VISIBILITY only, and ⛔ says so in its own limb-2 comment —
			//      ⛔ STILL ADMITS the stop. `MoveFocus` re-reads the index from Slate every press,
			//      so the failed request leaves the index where it was and the ⛔ next press repeats
			//      it forever. ⛔ That is a SWALLOW, ⛔ not a wasted press.
			//
			//  ⛔ AND THE OTHER HALF OF WARN-L1 IS ⛔ RESOLVED HERE RATHER THAN ⛔ INHERITED. QA ruled
			//  that ⭐ `TASK-1432`'s ⛔ unregister-on-list→detail / re-register-on-detail→list edges
			//  agreed with a switcher-aware predicate ⛔ *"by a contingent authored fact, not by
			//  construction"*, and named ⛔ THIS FLIP as the counter-case: flip this line and such a
			//  predicate would ⛔ keep `{BackButton}` while those edges ⛔ unregistered the whole
			//  screen. ⇒ ⛔ THE UNREGISTER EDGE IS ⛔ GONE. Both view switches now ⛔ RE-REGISTER, so
			//  the invariant is ⛔ **registered ⟺ `bHelpOpen`** and the ring ⛔ FOLLOWS the view
			//  instead of ⛔ leaving with it. ⛔ The whole argument, with the alternatives that were
			//  measured and refused, is at `ApplyActiveView` and `ShowDetailForAction`.
			ApplyButtonFocusable(BackButton);

			if (BackLabelText != nullptr)
			{
				if (UButtonSlot* BackContentSlot = Cast<UButtonSlot>(BackButton->SetContent(BackLabelText)))
				{
					BackContentSlot->SetPadding(FMargin(28.f, 10.f, 28.f, 10.f));
					BackContentSlot->SetHorizontalAlignment(HAlign_Center);
					BackContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* BackSlot = DetailColumn->AddChildToVerticalBox(BackButton))
			{
				BackSlot->SetPadding(FMargin(56.f, 0.f, 56.f, 34.f));
				BackSlot->SetHorizontalAlignment(HAlign_Center);
				BackSlot->SetVerticalAlignment(VAlign_Bottom);
			}
		}
	}

	if (BackButton == nullptr)
	{
		// Worth an Error: ⛔ `Escape` may never be claimed (`AS-§6 A-2`), so without this button
		// the only ways off this page are the overlay's toggle key and its Close button — both of
		// which are BEHIND the page. That is a page the player can only leave by closing the whole
		// overlay, which is a degradation, ⛔ not a soft-lock, and it must be visible in the log.
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Detail view: could not construct BackButton - the page can only be left by closing the overlay."));
	}

	// ═══ ⭐ TASK-1496 [DETAIL-SCROLL-BUTTON-REMOVAL] — ⛔ `DetailScrollButton` WAS CONSTRUCTED HERE
	//  AND ⛔ IS GONE, 🧑 ON HIS RULING OF 2026-09-26 ═══════════════════════════════════════════
	//  ⛔ **A scroll affordance was added at ⭐ `TASK-1484` and ⛔ REMOVED at 🧑 his ruling
	//  ⛔ 2026-09-26 after ⛔ pixels measured that ⛔ NO DETAIL PAGE OVERFLOWS.** ⭐ `TASK-1494` took
	//  the model's ⛔ LONGEST page (`Cards.PlacementResize`) and found its lowest painted text row
	//  at ⛔ 516 of 615 with ⛔ 57 px of clear panel below it — ⛔ and that was ⛔ WITH the button
	//  present. ⛔ Six pages, ⛔ six different bottom rows in the model's ⛔ exact rank order ⇒
	//  ⛔ nothing is clipped and there is ⛔ no fold. ⇒ ⛔ the control was a ⛔ stop with ⛔ no work to
	//  do, ⛔ costing ~74 px of prose per page. 🧑 He was told he had been asked ⛔ twice on premises
	//  that ⛔ did not hold, and ruled ⛔ remove it. ⛔ The full record — what `TASK-1484` added and
	//  ⛔ why its mechanism was sound — is in `USiegeControlsDetailWidget`'s class comment
	//  (`SC-§120`: ⛔ struck, ⛔ not deleted).
	//
	// 🚨 ⛔ THE ⛔ THIRD TRIP-WIRE WENT WITH THIS BLOCK, AND ⛔ THAT IS ⛔ DELIBERATE — ⛔ SAID OUT
	//  LOUD BECAUSE ⭐ *a trip-wire that vanishes without a sentence is indistinguishable from one
	//  removed by accident.* ⛔ It read *"this button is focusable on the line below ⇒ this
	//  screen's ring is correct ⛔ only because `ApplyActiveView` collapses the inactive
	//  `ViewSwitcher` branch"*, and it guarded ⛔ exactly one line: `ApplyButtonFocusable(
	//  DetailScrollButton)`. ⛔ THAT FLIP NO LONGER EXISTS ⇒ ⛔ the wire has ⛔ nothing to guard.
	//  🚨 ⛔ THE OTHER ⛔ TWO WIRES ⛔ STAY, ⛔ UNTOUCHED AND ⛔ STILL LOAD-BEARING (⭐ `TASK-1479`
	//  made their absence a ⛔ BLOCKER): one at `RowButton`'s flip in
	//  `USiegeControlsHelpRowWidget::ConstructRowTree` (the switcher's ⛔ LIST branch) and one at
	//  `BackButton`'s flip ⛔ directly above (the ⛔ DETAIL branch). ⛔ Do ⛔ NOT remove either by
	//  analogy with this one: ⛔ their flips are ⛔ still here.
	//
	// ⛔ AND THE ⛔ STEPPER-PAIR HAZARD ⭐ `TASK-1484` OPENED IS ⛔ CLOSED BY THIS REMOVAL:
	//  `USiegeMenuInputSubsystem::FindStepperPair` needs ⛔ EXACTLY TWO buttons in the immediate
	//  parent, and `DetailColumn` now holds ⛔ ONE ⇒ the ⛔ name and ⛔ glyph discriminators that
	//  were load-bearing are ⛔ no longer reachable. 🚨 ⛔ Add a second `UButton` to `DetailColumn`
	//  and ⛔ that guard is live again — ⛔ read `FindStepperPair` first.
	// ═══════════════════════════════════════════════════════════════════════════════════════════
}

void USiegeControlsDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bound HERE, ⛔ not in NativeOnInitialized: the code-authored children do not exist until
	// RebuildWidget() has run (Initialize() -> NativeOnInitialized() -> RebuildWidget() ->
	// NativeConstruct()).
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USiegeControlsDetailWidget::HandleBackButtonClicked);
	}

	// ⭐ TASK-1496 — ⭐ `TASK-1484`'s second `AddUniqueDynamic` (`DetailScrollButton->OnClicked` →
	// `HandleScrollButtonClicked`) stood here and ⛔ went with its button. ⛔ `BackButton`'s bind
	// above is ⛔ the only one this page has ever needed again.
}

void USiegeControlsDetailWidget::NativeDestruct()
{
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USiegeControlsDetailWidget::HandleBackButtonClicked);
	}

	// ⭐ TASK-1496 — the matching `RemoveDynamic` for `DetailScrollButton` stood here and ⛔ went
	// with it. ⛔ The bind/unbind pair was removed ⛔ together, in ⛔ one diff.

	Super::NativeDestruct();
}

void USiegeControlsDetailWidget::HandleBackButtonClicked()
{
	RequestBack();
}

// ═══ ⭐ TASK-1496 — `HandleScrollButtonClicked()` AND `AdvanceBodyScroll()` WERE DEFINED HERE ══
//  ⛔ Both added at ⭐ `TASK-1484`; ⛔ both removed at 🧑 his ruling of 2026-09-26, ⛔ because the
//  ⛔ fold they answered does ⛔ not exist (⭐ `TASK-1494`'s five captures across two PIE sessions).
//  ⛔ `HandleScrollButtonClicked` was the `UFUNCTION` thunk — ⛔ the ⛔ ONE reflected symbol this
//  removal takes out (⛔ exec set ⛔ 10 → 9, ⛔ single removal `execHandleScrollButtonClicked`,
//  ⛔ zero additions). ⛔ `AdvanceBodyScroll` paged `DetailScrollBox` by a measured fraction of its
//  own visible height and ⛔ wrapped at the end, reading the offset ⛔ before and ⛔ after and
//  printing ⛔ both at `Log`. ⛔ Its instrument was ⛔ sound and its verdict was ⛔ honest — ⭐ that
//  is precisely ⛔ HOW the fold was refuted: the function ⛔ reported `NO MOVEMENT` on pages whose
//  prose ⛔ already fit, and ⭐ `TASK-1491`/`1494` then measured the pixels and agreed.
//  ⚖️ *An instrument built to be able to fail is the one that can retire the feature it serves.*
// ═══════════════════════════════════════════════════════════════════════════════════════════
void USiegeControlsDetailWidget::RequestBack()
{
	// ⛔ DELIBERATELY UNFENCED — see the header. A row refuses to report a click before it knows
	// its identity; a PAGE must never refuse to let the player leave, because `Escape` is not
	// available as a second way out (`AS-§6 A-2`) and a page with no exit is the one defect a
	// help screen must not have.
	OnBackRequested.ExecuteIfBound();
}

void USiegeControlsDetailWidget::SetDetailContent(const FSiegeControlsDetailContent& InContent)
{
	// Stored UNCONDITIONALLY — ⛔ never gate the stamp on child liveness. The overlay stamps this
	// view on the FIRST row click, which for a switcher's second child is typically BEFORE Slate
	// has ever built it, so a guarded SetText here would render the first page blank (the
	// BLOCKER 674-1 shape, paid for once already on the row widget).
	Content = InContent;

	ApplyStoredContent();
}

void USiegeControlsDetailWidget::ApplyStoredContent()
{
	// The stamp fence, cloned from USiegeControlsHelpRowWidget: it keeps an un-stamped page from
	// blanking an asset-authored label reached through the `HELP-§3` escape hatch. Idempotent and
	// safe at every rebuild.
	// ⛔ NOTE THE ASYMMETRY WITH RequestBack(), WHICH IS DELIBERATELY UNFENCED: an un-stamped page
	// may show nothing, but it must never REFUSE TO CLOSE — `Escape` is not a second way out.
	if (Content.ActionId.IsNone())
	{
		return;
	}

	if (DetailKeyChipText != nullptr)
	{
		DetailKeyChipText->SetText(Content.KeyChip);
	}
	if (DetailKeyChipBorder != nullptr)
	{
		// A pointer-only or unbound row still shows a chip ("Mouse click" / "(not bound)"), so the
		// plate is never empty — the chip composer guarantees it (`HELP-§2` mechanism 2).
		DetailKeyChipBorder->SetVisibility(
			Content.KeyChip.IsEmptyOrWhitespace() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (DetailTitleText != nullptr)
	{
		DetailTitleText->SetText(Content.Title);
	}
	if (DetailSummaryText != nullptr)
	{
		DetailSummaryText->SetText(Content.Summary);
	}
	if (DetailBodyText != nullptr)
	{
		// ⛔ Content.Body already came through ComposeDetailForDisplay, so an undocumented row
		// renders the pinned TODO string here — ⛔ never a blank page.
		DetailBodyText->SetText(Content.Body);
	}

	RebuildRelatedBlocks();

	// A fresh page starts at the top. Without this, clicking a short page after a long one would
	// open it scrolled past its own first paragraph.
	if (DetailScrollBox != nullptr)
	{
		DetailScrollBox->ScrollToStart();
	}
}

void USiegeControlsDetailWidget::RebuildRelatedBlocks()
{
	if (RelatedBox == nullptr)
	{
		return;
	}

	// ⛔ DESTROYED AND REBUILT EVERY STAMP. That is what makes "no page ever shows the previous
	// page's controls" structural, and it is the same reason RefreshRows() destroys its rows: a
	// surviving block is a stale derived key waiting to be read (`KBD-§0` ruling 2).
	RelatedBox->ClearChildren();

	if (RelatedHeaderText != nullptr)
	{
		RelatedHeaderText->SetVisibility(
			Content.Related.Num() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (WidgetTree == nullptr)
	{
		return;
	}

	for (const FSiegeControlsDetailEntry& Entry : Content.Related)
	{
		UHorizontalBox* const EntryBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (EntryBox == nullptr)
		{
			UE_LOG(LogSiegeControlsHelp, Warning,
				TEXT("[ControlsHelp] Detail view: could not construct the related block for '%s' - that control is missing from this page."),
				*Entry.ActionId.ToString());
			continue;
		}

		if (UVerticalBoxSlot* EntrySlot = RelatedBox->AddChildToVerticalBox(EntryBox))
		{
			EntrySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
			EntrySlot->SetHorizontalAlignment(HAlign_Fill);
		}

		// ---- the related control's own DERIVED chip ---------------------------------------
		if (UBorder* const ChipBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass()))
		{
			ChipBorder->SetBrushColor(FLinearColor(0.10f, 0.12f, 0.16f, 0.95f));
			ChipBorder->SetPadding(FMargin(10.f, 5.f, 10.f, 5.f));
			ChipBorder->SetHorizontalAlignment(HAlign_Center);
			ChipBorder->SetVerticalAlignment(VAlign_Center);

			if (UTextBlock* const ChipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
			{
				// ⛔ DERIVED, ⛔ never typed: Entry.KeyChip came from ComposeKeyChipLabel over
				// ResolveRowDisplayKeys' lane audit, exactly like a list row's chip (`HELP-§1`).
				ChipText->SetText(Entry.KeyChip);
				ChipText->SetFontSize(20.f);
				ChipText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.72f, 1.f)));
				ChipBorder->SetContent(ChipText);
			}

			if (UHorizontalBoxSlot* ChipSlot = EntryBox->AddChildToHorizontalBox(ChipBorder))
			{
				ChipSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
				ChipSlot->SetVerticalAlignment(VAlign_Top);
			}
		}

		// ---- name over that control's OWN detail prose --------------------------------------
		UVerticalBox* const TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (TextColumn == nullptr)
		{
			continue;
		}

		if (UHorizontalBoxSlot* TextSlot = EntryBox->AddChildToHorizontalBox(TextColumn))
		{
			TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TextSlot->SetVerticalAlignment(VAlign_Top);
		}

		if (UTextBlock* const NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			NameText->SetText(Entry.DisplayName);
			NameText->SetFontSize(22.f);
			TextColumn->AddChildToVerticalBox(NameText);
		}

		if (UTextBlock* const BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()))
		{
			// ⭐ THAT ROW'S OWN DETAIL TEXT, rendered a second time rather than copied a second
			// time (`HELP-§2`): this is how the Ambush page answers all three of Jonathan's named
			// questions without a single duplicated sentence existing in the feature.
			BodyText->SetText(Entry.Body);
			BodyText->SetFontSize(18.f);
			BodyText->SetAutoWrapText(true);
			BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.86f, 0.90f, 1.f)));
			TextColumn->AddChildToVerticalBox(BodyText);
		}
	}
}

// ════════════════════════════════════════════════════════════════════════════════════════════
//  USiegeControlsHelpWidget
// ════════════════════════════════════════════════════════════════════════════════════════════

USiegeControlsHelpWidget* USiegeControlsHelpWidget::CreateAndAddToViewport(
	APlayerController* OwningController,
	TSubclassOf<USiegeControlsHelpWidget> HelpClass,
	int32 ZOrder)
{
	if (!IsValid(OwningController))
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] CreateAndAddToViewport: no owning player controller - no overlay was created. Never fatal: a help screen is never a requirement."));
		return nullptr;
	}

	// ⚠️ `.Get()` ON BOTH ARMS IS LOAD-BEARING, NOT TIDYING - it is the fix for C2445, carried
	// verbatim from UWarMapWidget::CreateAndAddToViewport, which paid for the diagnosis.
	// TSubclassOf carries BOTH a non-explicit TSubclassOf(UClass*) constructor AND a
	// non-explicit operator UClass*(), so a conditional whose arms are TSubclassOf<T> and
	// UClass* has two equally good common types and the compiler must refuse to choose.
	const TSubclassOf<USiegeControlsHelpWidget> ResolvedClass =
		HelpClass ? HelpClass.Get() : USiegeControlsHelpWidget::StaticClass();

	USiegeControlsHelpWidget* Help = CreateWidget<USiegeControlsHelpWidget>(OwningController, ResolvedClass);
	if (Help == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] CreateAndAddToViewport: CreateWidget returned null for class '%s' - no overlay. Never fatal."),
			*GetNameSafe(ResolvedClass));
		return nullptr;
	}

	// Added CLOSED - NativeConstruct collapses it - so nothing appears on screen and nothing
	// becomes hit-testable until OpenHelp().
	Help->AddToViewport(ZOrder);

	UE_LOG(LogSiegeControlsHelp, Log,
		TEXT("[ControlsHelp] Created (class '%s', ZOrder %d), closed. %s"),
		*GetNameSafe(ResolvedClass), ZOrder,
		// ⚠️ `.Get()` AGAIN, AND FOR THE SAME REASON: comparing a TSubclassOf<T> directly
		// against a UClass* re-opens the two-viable-conversions ambiguity.
		(ResolvedClass.Get() == USiegeControlsHelpWidget::StaticClass())
			? TEXT("No WBP - the code-authored tree renders the list (HELP-§3; /Game/UI/WBP_ControlsHelp is RESERVED and unauthored).")
			: TEXT(""));

	return Help;
}

TSharedRef<SWidget> USiegeControlsHelpWidget::RebuildWidget()
{
	// ⚠️ ORDER IS LOAD-BEARING — see the row class's identical comment (`HELP-§3`,
	// UserWidget.cpp:1214's SSpacer trap).
	Initialize();
	ConstructHelpTree();
	return Super::RebuildWidget();
}

void USiegeControlsHelpWidget::ConstructHelpTree()
{
	if (WidgetTree == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] No WidgetTree - the overlay cannot build its tree."));
		return;
	}

	// THE `HELP-§3` ESCAPE HATCH — an asset-authored /Game/UI/WBP_ControlsHelp wins WHOLE.
	// RESERVED and unauthored today.
	if (WidgetTree->RootWidget != nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Log,
			TEXT("[ControlsHelp] An asset-authored tree is present - the code-authored branch is skipped (HELP-§3)."));
		return;
	}

	// ---- BackdropBorder: the full-screen container, and the tree root ------------------
	if (BackdropBorder == nullptr)
	{
		BackdropBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackdropBorder"));
	}

	if (BackdropBorder == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct BackdropBorder - the overlay has no root."));
		return;
	}

	// ⛔⛔ SelfHitTestInvisible IS CORRECTNESS, NOT STYLING, AND IT IS THE OPPOSITE CHOICE FROM
	// UAccountMenuWidget's Visible backdrop — DELIBERATELY, because the geometry is opposite.
	// That panel overlays a MENU whose Play/Quit buttons must not be clicked through; THIS one
	// overlays a LIVE, UNPAUSED MATCH (`HELP-§5`: single-player has no pause and the bot keeps
	// marching). A full-screen click-absorbing plate here would silently eat the very
	// placement / targeting / group-pick cancel clicks `HELP-§5` promises keep firing
	// byte-identically. ⇒ THE PLATE PASSES CLICKS THROUGH; the PANEL below absorbs its own.
	BackdropBorder->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BackdropBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
	// ⭐ ZERO padding here, ⛔ not the list margin (TASK-707): the margin moved DOWN onto the
	// switcher's list slot so the DETAIL view's slot can carry none and reach the screen edge.
	// See MakeListPanelMargin. The fallback path below restores it if the switcher never builds.
	BackdropBorder->SetPadding(FMargin(0.f));
	BackdropBorder->SetHorizontalAlignment(HAlign_Fill);
	BackdropBorder->SetVerticalAlignment(VAlign_Fill);

	WidgetTree->RootWidget = BackdropBorder;

	// ---- ViewSwitcher: list (child 0) vs full-screen detail (child 1) -------------------
	if (ViewSwitcher == nullptr)
	{
		ViewSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("ViewSwitcher"));
	}

	if (ViewSwitcher != nullptr)
	{
		BackdropBorder->SetContent(ViewSwitcher);
	}
	else
	{
		// ⛔ DEGRADE TO THE SHIPPED TASK-706 SHAPE, ⛔ never to a blank overlay: no switcher means
		// no detail view, so the LIST becomes the backdrop's direct content and takes back the
		// margin the switcher slot would have carried. ShowDetailForAction then logs and stays on
		// the list. A help screen is never a requirement.
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct ViewSwitcher - the list still renders, but the full-screen detail view is unavailable."));
		BackdropBorder->SetPadding(MakeListPanelMargin());
	}

	// ---- PanelBorder: the readable plate, and the ONLY thing that absorbs a click -------
	if (PanelBorder == nullptr)
	{
		PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	}

	if (PanelBorder == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct PanelBorder - the overlay has no panel."));
		return;
	}

	// Visible (hit-testable) so a click meant for a row cannot fall through into the world and
	// place a card. Its area is the panel only — see the backdrop comment above.
	PanelBorder->SetVisibility(ESlateVisibility::Visible);
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.92f));
	PanelBorder->SetPadding(FMargin(0.f));
	PanelBorder->SetHorizontalAlignment(HAlign_Fill);
	PanelBorder->SetVerticalAlignment(VAlign_Fill);

	// ⚠️ ADDED FIRST — this add IS ListViewIndex, and the order is the contract.
	if (ViewSwitcher != nullptr)
	{
		if (UWidgetSwitcherSlot* ListSlot = Cast<UWidgetSwitcherSlot>(ViewSwitcher->AddChild(PanelBorder)))
		{
			// The list keeps TASK-706's exact inset; only its OWNER changed.
			ListSlot->SetPadding(MakeListPanelMargin());
			ListSlot->SetHorizontalAlignment(HAlign_Fill);
			ListSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	else
	{
		BackdropBorder->SetContent(PanelBorder);
	}

	// ---- RootPanel ----------------------------------------------------------------------
	if (RootPanel == nullptr)
	{
		RootPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootPanel"));
	}

	if (RootPanel == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct RootPanel - the overlay has no content column."));
		return;
	}

	PanelBorder->SetContent(RootPanel);

	// ---- TitleText ------------------------------------------------------------------------
	if (TitleText == nullptr)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		if (TitleText != nullptr)
		{
			TitleText->SetText(FText::FromString(FString(SiegeControlsHelpText::Title)));
			TitleText->SetFontSize(34.f);

			if (UVerticalBoxSlot* TitleSlot = RootPanel->AddChildToVerticalBox(TitleText))
			{
				TitleSlot->SetPadding(FMargin(24.f, 20.f, 24.f, 4.f));
				TitleSlot->SetHorizontalAlignment(HAlign_Center);
				TitleSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- HintText -------------------------------------------------------------------------
	// ⛔ NO TEXT IS SET HERE. RefreshRows() composes it from the Interface.ControlsHelp row's
	// OWN DERIVED CHIP (`HELP-§4`: the menu documents its own key) — typing a key name here
	// would be the exact hardcoded-letter defect `HELP-§1` forbids.
	if (HintText == nullptr)
	{
		HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
		if (HintText != nullptr)
		{
			HintText->SetFontSize(16.f);
			HintText->SetAutoWrapText(true);
			HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.76f, 0.84f, 1.f)));

			if (UVerticalBoxSlot* HintSlot = RootPanel->AddChildToVerticalBox(HintText))
			{
				HintSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 12.f));
				HintSlot->SetHorizontalAlignment(HAlign_Center);
				HintSlot->SetVerticalAlignment(VAlign_Top);
			}
		}
	}

	// ---- RowScrollBox: the list -------------------------------------------------------------
	if (RowScrollBox == nullptr)
	{
		RowScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RowScrollBox"));
		if (RowScrollBox != nullptr)
		{
			RowScrollBox->SetOrientation(Orient_Vertical);
			RowScrollBox->SetAlwaysShowScrollbar(true);

			// ⛔ STILL NOT FOCUSABLE, ⛔ AND THE REASON IS NOW A DIFFERENT ONE (`SC-§120`).
			// ⛔ WAS: ~~"the same `Tab`-is-Slate's-focus-key reasoning as RowButton."~~ — that
			// reasoning was ⛔ struck at the `RowButton` site by ⭐ `TASK-1478`, so a citation of it
			// would now point at a struck sentence. ⛔ THE CONCLUSION IS UNCHANGED and rests on the
			// measurement the `CloseButton` site already recorded: a `UScrollBox` is ⛔ not one of
			// `IsNavFocusStop`'s four admitted classes, so flipping it would move ⛔ NOTHING in the
			// ring while ⛔ looking like progress. ⭐ `TASK-1432` (2) ruled the scroll boxes must not
			// become stops and that ruling ⛔ STANDS — ⭐ `TASK-1478` deliberately did ⛔ not blanket.
			RowScrollBox->SetIsFocusable(false);

			// ═══ ⭐⭐ TASK-1478 — A RING THAT LEAVES THE SCREEN IS NOT A RING ═══════════════════
			// ⛔ MEASURED IN THE 5.8 SOURCE, ⛔ NOT ASSUMED: ⛔ BOTH defaults are `NoScroll` — the
			// Slate one (`Slate/Public/Widgets/Layout/SScrollBox.h`, `_ScrollWhenFocusChanges`
			// initialiser in `FArguments`) ⛔ AND the UMG one (`UMG/Private/Components/ScrollBox.cpp`,
			// the `UScrollBox` constructor's initialiser list). ⇒ ⛔ WITH THE ROWS NOW FOCUSABLE AND
			// 27 REGISTRY ROWS IN A BOX THAT SHOWS A HANDFUL, the ring would walk ⛔ off the bottom
			// and 🧑 he would watch the outline ⛔ VANISH — navigable on paper, ⛔ not in his hands.
			// `SScrollBox::OnFocusChanging` calls `ScrollDescendantIntoView` ⛔ exactly when this
			// attribute is not `NoScroll`, so this one line is the whole of the fix.
			//
			// ⚖️ `InstantScroll` RATHER THAN `AnimatedScroll`, AND THE REASON IS THE ⛔ VERIFIER:
			// an animated scroll settles over several frames, so a 5b lane that injects `IA_MenuDown`
			// and reads the next frame would sample a position ⛔ mid-flight and a pixel check would
			// be frame-timing dependent. ⛔ A gate that can disagree with itself between frames is not
			// a gate. `NavigationDestination` is left at its `IntoView` default — the minimum scroll.
			//
			// ⛔ ZERO DELTA ON ANYTHING SHIPPED: this attribute fires ⛔ only on a focus change INSIDE
			// this box, and ⛔ before this row no child of it could take focus at all.
			RowScrollBox->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);

			// ⚠️ The wheel is CONSUMED by the list while the overlay is open, and that is
			// correct here rather than a conflict: the wheel's only shipped meaning is
			// resizing a pick circle (PickMode.Resize), and the pick's own reticle is
			// underneath a panel the player is currently reading. Closing the overlay restores
			// it exactly — nothing is rebound and no shipped poll is touched.
			RowScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);

			if (UVerticalBoxSlot* ListSlot = RootPanel->AddChildToVerticalBox(RowScrollBox))
			{
				ListSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 12.f));
				ListSlot->SetHorizontalAlignment(HAlign_Fill);
				ListSlot->SetVerticalAlignment(VAlign_Fill);
				ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	}

	if (RowScrollBox == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct RowScrollBox - the overlay can show no rows."));
	}

	// ---- CloseButton: one of the TWO close routes, and the complete list with TAB ----------
	if (CloseLabelText == nullptr)
	{
		CloseLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabelText"));
		if (CloseLabelText != nullptr)
		{
			CloseLabelText->SetText(FText::FromString(FString(SiegeControlsHelpText::CloseLabel)));
			CloseLabelText->SetFontSize(24.f);
		}
	}

	if (CloseButton == nullptr)
	{
		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		if (CloseButton != nullptr)
		{
			// ⭐⭐ TASK-1432 — ⛔ FOCUSABLE, AND IT IS THE ⛔ ONLY ONE OF THIS FILE'S FIVE
			// DE-FOCUS SITES THAT WAS FLIPPED. ⛔ was: `ApplyButtonNotFocusable(CloseButton);`
			//
			// ⚖️ WHY THIS ONE AND ⛔ NOT THE OTHER FOUR — argued per site, ⛔ never as a blanket:
			//   ✅ `CloseButton` (HERE) — ⛔ it is the ⛔ WAY OUT. A keyboard player who cannot
			//      reach it has no on-screen exit at all, and the screen's whole ask is *"close
			//      it without the mouse"*. ~~It is also the ⛔ ONLY admitted-class widget the ring
			//      can reach in this tree (see the two ⛔ MEASURED refusals below).~~
			//      ⛔ ⭐ `TASK-1478`: ⛔ NO LONGER THE ONLY ONE, and ⛔ no longer the FIRST. The list
			//      ring is now `{ RowButton ×RowWidgets.Num(), CloseButton }` in
			//      `UWidgetTree::ForEachWidget` pre-order, and ⛔ `RowScrollBox` precedes
			//      `CloseButton` in `RootPanel`'s slot order ⇒ ⛔ THE ROWS COME FIRST and this button
			//      is the ⛔ LAST stop, not stop 0. ⛔ `FocusFirstNavStop()` therefore opens the
			//      overlay with the ring on the ⛔ first row rather than on Close. ⛔ Declared, ⛔ not
			//      patched: placing it back on Close would need a focus call this file's own law
			//      (`RegisterAsMenuNavTarget`'s "no `SetKeyboardFocus`") forbids.
			//   ⛔ `RowScrollBox` (`SetIsFocusable(false)`) and ⛔ `DetailScrollBox` (same) — ⛔ NOT
			//      flipped. A `UScrollBox` is ⛔ not one of `IsNavFocusStop`'s four admitted
			//      classes (`UButton` / `UCheckBox` / `USlider` / `UEditableTextBox` —
			//      `SiegeMenuInputSubsystem.cpp:773-870`, re-read live), so flipping either
			//      would move ⛔ NOTHING in the ring while ⛔ looking like progress. They are
			//      ⛔ containers: scroll follows the focused row; the box is never the ring's home.
			// ═══ 🚨🚨 TASK-1478 — ⛔ THE LAST TWO BULLETS OF THIS TABLE ARE ⛔ NOW OUT OF DATE, AND
			//  THEY ARE ⛔ AMENDED IN PLACE RATHER THAN REWRITTEN (`SC-§120`). ⛔ `RowButton` AND
			//  ⛔ `BackButton` ARE ⛔ NOW FOCUSABLE — the per-site arguments live ⛔ at their own
			//  flip sites (`ConstructRowTree` and `ConstructDetailTree`), ⛔ not here, because that
			//  is where the next editor of each will be standing. ⛔ The two bullets below are kept
			//  ⛔ verbatim because their ⛔ MEASUREMENT is still correct and still load-bearing — it
			//  is the ⛔ PREMISE that expired, not the reading of `WidgetTree.cpp`: ⭐ `TASK-1474`
			//  added a ⛔ DESCENT (`CollectNavStopsFromTree`, gated on `IsCodeAuthoredSubWidget`)
			//  ⛔ beside that traversal, so "the walker reaches each row object and never enters it"
			//  ⛔ ceased to be true ⛔ without a single character of `ForWidgetAndChildren` changing.
			//  ⇒ ⚖️ ***⛔ A CORRECT MEASUREMENT OF THE WRONG FUNCTION READS EXACTLY LIKE A CORRECT
			//  CONCLUSION.*** ⛔ The scroll-box bullet above is ⛔ UNAFFECTED and still governs.
			//   ⛔ `RowButton` (in `USiegeControlsHelpRowWidget`) — ⛔ ~~NOT flipped~~ ⛔ **FLIPPED BY
			//      ⭐ `TASK-1478`**, and the reason it was refused here
			//      is ⛔ structural rather than stylistic: `UWidgetTree::ForWidgetAndChildren`
			//      descends into ⛔ named slots and ⛔ `UPanelWidget` children ⛔ ONLY
			//      (`UMG/Private/WidgetTree.cpp`, read at source).
			//      ⛔ TASK-1432 QA LOOP 1 (`qa/TASK-1433.md` NIT-2) — ⛔ THE MECHANISM WAS
			//      HALF-STATED AND IS CORRECTED IN PLACE (`SC-§120`, strike never delete), ⛔ not
			//      because the conclusion moved but because ⭐ `TASK-1474` is editing ⛔ exactly
			//      this traversal and will read ⛔ this comment first.
			//      ⛔ WAS: ~~"A `UUserWidget` is ⛔ neither, and the row widget declares ⛔ no
			//      `UNamedSlot`."~~ ⛔ The first half is ⛔ FALSE: `UUserWidget : public UWidget,
			//      public INamedSlotInterface` (`UMG/Public/Blueprint/UserWidget.h`), so
			//      `Cast<INamedSlotInterface>(RowWidget)` ⛔ SUCCEEDS and the ⛔ FIRST LIMB ⛔ IS
			//      ENTERED. ⛔ IT SIMPLY YIELDS NOTHING: `GetSlotNames` returns an ⛔ EMPTY array
			//      because the row class declares ⛔ no `UNamedSlot` (⛔ zero in this file pair).
			//      ⇒ ⛔ THE CONCLUSION IS UNCHANGED AND NOW RESTS ON ⛔ BOTH LIMBS FAILING FOR
			//      ⛔ DIFFERENT REASONS: limb 1 is entered and finds ⛔ no named slots; limb 2 is
			//      ⛔ never entered, a `UUserWidget` not being a `UPanelWidget`. ⛔ THE WALKER
			//      REACHES EACH ROW OBJECT AND ⛔ NEVER ENTERS IT, so flipping `RowButton` would
			//      be ⛔ INERT — ⛔ never collected, ⛔ not collected-then-refused. ⚠️ And the
			//      fragile half is limb 1: ⛔ one `UNamedSlot` added to the row class would open
			//      it. ~~Its own comment's `Tab`-is-Slate's-focus-key argument also still stands.~~
			//      ⛔ ⭐ `TASK-1478`: ⛔ THE FRAGILE HALF WAS NEVER THE ONE THAT BROKE. A ⛔ THIRD
			//      limb was ⛔ ADDED beside these two (`CollectNavStopsFromTree`'s
			//      `Cast<UUserWidget>` recursion, ⭐ `TASK-1474`), so ⛔ both limbs still fail
			//      exactly as measured and the row is entered ⛔ anyway. ⛔ And the `Tab` argument is
			//      ⛔ STRUCK at its own site: `NativeOnPreviewKeyDown` claims the toggle key in the
			//      ⛔ preview phase on ⛔ every ancestor of the focused widget.
			//   ⛔ `BackButton` (in `USiegeControlsDetailWidget`) — ⛔ ~~NOT flipped~~ ⛔ **FLIPPED BY
			//      ⭐ `TASK-1478`**, ⛔ same structural refusal, same measurement, ⛔ same expiry.
			//
			// ⚠️ THE `HELP-§3` ESCAPE HATCH IS UNCHANGED AND SO IS ITS CONSEQUENCE: if
			// /Game/UI/WBP_ControlsHelp is ever authored, `ConstructHelpTree` returns early and
			// ⛔ this write never runs — the asset's own `IsFocusable` wins WHOLE, exactly as
			// that law intends. ⛔ The author of that asset must make its Close control focusable
			// or the overlay goes back to being mouse-only.
			ApplyButtonFocusable(CloseButton);

			if (CloseLabelText != nullptr)
			{
				if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(CloseButton->SetContent(CloseLabelText)))
				{
					ContentSlot->SetPadding(FMargin(24.f, 10.f, 24.f, 10.f));
					ContentSlot->SetHorizontalAlignment(HAlign_Center);
					ContentSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			if (UVerticalBoxSlot* CloseSlot = RootPanel->AddChildToVerticalBox(CloseButton))
			{
				CloseSlot->SetPadding(FMargin(24.f, 0.f, 24.f, 20.f));
				CloseSlot->SetHorizontalAlignment(HAlign_Center);
				CloseSlot->SetVerticalAlignment(VAlign_Bottom);
			}
		}
	}

	if (CloseButton == nullptr)
	{
		// Worth an Error: ⛔ `Escape` may never be claimed (`AS-§6 A-2`), so without this
		// button the toggle key is the ONLY way out — and if IA_ControlsHelp has not landed,
		// there is none at all.
		UE_LOG(LogSiegeControlsHelp, Error,
			TEXT("[ControlsHelp] Could not construct CloseButton - the overlay can only be closed by its toggle key."));
	}

	// ---- DetailView: the switcher's SECOND child — Jonathan's "the entire screen" ----------
	// ⚠️ ADDED LAST — this add IS DetailViewIndex, and the order is the contract (see the two
	// constants). ⛔ Built here, once, rather than lazily on the first click: a widget created
	// mid-click would have to be added to a live switcher, and the switcher's own OnSlotAdded
	// re-reads the active index from Slate when that happens (WidgetSwitcher.cpp:105-112) —
	// i.e. lazy construction would make WHICH VIEW IS UP depend on WHEN the page was built.
	if (ViewSwitcher != nullptr)
	{
		if (DetailView == nullptr)
		{
			// A soft, null-safe detail class: /Game/UI/WBP_ControlsDetail is RESERVED and
			// unauthored, so this resolves null today and the code-authored page wins (`HELP-§3`).
			// ⚠️ Two statements, ⛔ not a ternary — a TSubclassOf-vs-UClass* conditional re-opens
			// the C2445 ambiguity UWarMapWidget documents and TASK-706 carried forward.
			UClass* ResolvedDetailClass = DetailWidgetClass.LoadSynchronous();
			if (ResolvedDetailClass == nullptr)
			{
				ResolvedDetailClass = USiegeControlsDetailWidget::StaticClass();
			}

			DetailView = WidgetTree->ConstructWidget<USiegeControlsDetailWidget>(
				TSubclassOf<USiegeControlsDetailWidget>(ResolvedDetailClass), TEXT("DetailView"));
		}

		if (DetailView != nullptr)
		{
			if (UWidgetSwitcherSlot* DetailSlot = Cast<UWidgetSwitcherSlot>(ViewSwitcher->AddChild(DetailView)))
			{
				// ⭐ ZERO padding — this is what makes the page "the entire screen" while the list
				// beside it keeps its plate margin.
				DetailSlot->SetPadding(FMargin(0.f));
				DetailSlot->SetHorizontalAlignment(HAlign_Fill);
				DetailSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		else
		{
			UE_LOG(LogSiegeControlsHelp, Error,
				TEXT("[ControlsHelp] Could not construct DetailView - the list still renders and row clicks are still reported, but no detail page opens."));
		}

		// ⛔ THE LIST IS THE VIEW THE OVERLAY OPENS ON, ALWAYS. Stated explicitly rather than
		// relied upon: UWidgetSwitcher::OnSlotAdded re-reads the active index from Slate when a
		// child is added to an already-built switcher, so "it defaults to 0" is only true of the
		// path this code happens to take today.
		// ⭐ TASK-1478 — ⛔ THROUGH `ApplyActiveView`, ⛔ not `SetActiveWidgetIndex` directly, so the
		// ⛔ INITIAL state carries the branch collapse like every later one. ⛔ This is now the
		// ⛔ ONE writer of the switcher's index in this file; see that function's own block.
		ApplyActiveView(ListViewIndex);
	}
}

void USiegeControlsHelpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &USiegeControlsHelpWidget::HandleCloseButtonClicked);
	}

	// ⭐ THE DETAIL VIEW'S BACK SEAM, BOUND IN EXACTLY ONE PLACE (TASK-707). Bound HERE for the
	// same reason as CloseButton: the code-authored DetailView does not exist until
	// RebuildWidget() has run.
	if (DetailView != nullptr)
	{
		DetailView->OnBackRequested.BindUObject(this, &USiegeControlsHelpWidget::HandleDetailBackRequested);
	}

	// Added to the viewport CLOSED. Written DIRECTLY rather than through ApplyOpenState
	// because bHelpOpen already reads false: ApplyOpenState would take its no-op early-out and
	// broadcast nothing, which is right for the flag and wrong for the visibility.
	SetVisibility(ESlateVisibility::Collapsed);
}

void USiegeControlsHelpWidget::NativeDestruct()
{
	if (CloseButton != nullptr)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &USiegeControlsHelpWidget::HandleCloseButtonClicked);
	}

	// Unbind before release: this widget is the detail view's bind target, and a page outliving
	// it through some future retain path must not call into a dead overlay.
	if (DetailView != nullptr)
	{
		DetailView->OnBackRequested.Unbind();
	}

	// Unbind every row's delegate before the rows go away — this widget is their bind target.
	ClearRowWidgets();

	// ⭐ TASK-1432 — THE NET, ⛔ NOT THE LICENCE. `ApplyOpenState(false)` is the ⛔ real falling
	// edge and every shipped close route runs through it; this second call exists for the ⛔ one
	// shape that route cannot cover — an overlay ⛔ destroyed while still open (a level travel, a
	// `RemoveFromParent` a future path forgets to precede with `CloseHelp`). A screen that
	// registered and never unregistered would strand the ring on a dead tree and leave the menu
	// vocabulary armed until the subsystem's backstop poll noticed, up to
	// `InMatchDemandPollSeconds` later — i.e. ⛔ a live match with menu keys applied.
	// ⛔ IDEMPOTENT BY THE SUBSYSTEM'S OWN CONTRACT: it removes by IDENTITY and logs (does not
	// warn) when the screen was not on the stack, which is exactly what makes this pairing safe
	// on the ordinary path where `ApplyOpenState(false)` already ran.
	UnregisterAsMenuNavTarget();

	Super::NativeDestruct();
}

void USiegeControlsHelpWidget::HandleCloseButtonClicked()
{
	CloseHelp();
}

void USiegeControlsHelpWidget::OpenHelp()
{
	// ⛔ NO SetInputMode AND NO bShowMouseCursor ANYWHERE IN THIS FUNCTION OR THIS FILE.
	// ASiegePlayerController::SetControlsHelpOpen owns the posture, as the SIXTH term in
	// ApplyCursorInputState's ONE composition. A direct posture call from a widget is the
	// defect that booted the arena input-dead and cost a playtest (TASK-074).

	// ⭐ THE FRESHNESS STEP, AND ITS ORDER IS THE POINT (`KBD-§8`: THE CALLER REFRESHES, THE
	// ACCESSOR READS). RefreshKeyboardLayout is idempotent and free when nothing moved, and it
	// re-targets the cached IMC duplicate IN PLACE — so the QueryKeysMappedToAction answers
	// RefreshRows is about to read are current even after a mid-session Win+Space
	// (`KBD-§0` ruling 2 puts that in scope; the poll is only 1 Hz).
	// ⛔ This widget does NOT bind OnKeyboardLayoutChanged — `KBD-§8` forbids it for a widget:
	// new lifetime state to unbind wrongly, for a value re-read at every open anyway.
	if (USiegeKeyboardLayoutSubsystem* LayoutSubsystem = ResolveKeyboardLayoutSubsystem())
	{
		LayoutSubsystem->RefreshKeyboardLayout();
	}

	// ⭐ EVERY OPEN STARTS ON THE LIST (TASK-707). CloseHelp() already returns first, so this is
	// belt AND braces — and it is worth having, because the one route that could ever open a
	// still-open overlay would otherwise reveal whatever page was last read.
	ReturnToList();

	// Rows are rebuilt from scratch BEFORE anything becomes visible, so no stale letter is ever
	// on screen for even one frame.
	RefreshRows();

	ApplyOpenState(true);
}

void USiegeControlsHelpWidget::CloseHelp()
{
	// ⭐ A close from inside the detail view returns to the list FIRST, so the next open always
	// starts where the player expects. ⛔ This was already the shape TASK-706 shipped, which is
	// why filling the detail view needed NO new close path: the toggle key and the Close button
	// are still the complete list of ways out (`HELP-§5`).
	ReturnToList();

	ApplyOpenState(false);
}

void USiegeControlsHelpWidget::ApplyOpenState(bool bOpen)
{
	// ⛔ NEVER BROADCAST ON A NO-OP (the delegate law: a delegate that fires on unchanged state
	// trains consumers to ignore it — and here it would bounce the controller's posture).
	if (bHelpOpen == bOpen)
	{
		return;
	}

	bHelpOpen = bOpen;

	// SelfHitTestInvisible, ⛔ not Visible: the root user widget must not become a full-screen
	// click absorber for the same reason BackdropBorder is not one (see ConstructHelpTree).
	SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	// ═══════════════════════════════════════════════════════════════════════════════════════
	//  🚨🚨 TASK-1432 — ⛔ REGISTER ON OPEN, ⛔ UNREGISTER ON CLOSE. ⛔ NEVER AT CONSTRUCTION,
	//  ⛔ NEVER BY A VISIBILITY TOGGLE ALONE.
	//
	//  ⛔ (1) WHY ⛔ HERE AND NOT `NativeConstruct`: this overlay is added to the viewport
	//  ⛔ CLOSED — `CreateAndAddToViewport` calls `AddToViewport` and `NativeConstruct`
	//  immediately collapses it. The subsystem's in-match arming backstop is ⛔ DISARM-ONLY and
	//  there is ⛔ NO RISING EDGE ON *SHOW* (`qa/TASK-1430.md` WARN-2), and
	//  `GetRegisteredNavTarget()` requires `IsVisible()`. ⇒ ⛔ A REGISTRATION TAKEN AT
	//  CONSTRUCTION WOULD ⛔ NEVER ARM: it would compile, review clean, pass the suite and
	//  ⛔ do nothing (`SC-§36.1` — the better the surface, the more complete the illusion).
	//
	//  ⛔ (2) WHY ⛔ AFTER `SetVisibility` AND NOT BEFORE — this is the ⛔ load-bearing ordering
	//  inside the function. `RegisterMenuNavTarget` ends by placing the ring on stop 0, and both
	//  the demand predicate and `IsNavFocusStop`'s `HasVisibleSlateAncestry` limb read the
	//  ⛔ CURRENT Slate visibility attribute. Registering one line earlier would ask those tests
	//  about a ⛔ Collapsed tree and be answered ⛔ "no stops" — a 0-stop ring on a screen that
	//  visibly has a button.
	//
	//  ⛔ (3) WHY ⛔ THIS FUNCTION AND NOT `OpenHelp`/`CloseHelp` — `ApplyOpenState` is the ⛔ ONE
	//  funnel both routes pass through, and its no-op early-out above means these calls fire
	//  ⛔ exactly once per REAL state change. Put on `OpenHelp`, a second open would register
	//  twice; put on `CloseHelp`, the controller's `EndPlay` close path would unregister a screen
	//  that was never open. ⛔ The edge is the state change, so the edge lives with the flag.
	//
	//  ⭐ (4) THE CONTROLLER'S POSTURE CALL CANNOT STEAL THE RING — ⛔ MEASURED AT ENGINE SOURCE,
	//  ⛔ not assumed. On the open route `OnControlsHelpPressed` runs `SetControlsHelpOpen(true)`
	//  → `ApplyCursorInputState()` ⛔ BEFORE `OpenHelp()`, and that applies `FInputModeGameAndUI`
	//  with ⛔ no `SetWidgetToFocus` — `FInputModeDataBase::SetFocusAndLocking` calls
	//  `SetUserFocus` ⛔ only when a widget was supplied (`Engine/Private/PlayerController.cpp`
	//  `:6313-6319`, `:6398-6406`). ⇒ ⛔ nothing touches focus after this line.
	//  ⭐ And the ⛔ CLOSE side is better than neutral: when the last cursor owner goes away the
	//  same function applies `FInputModeGameOnly`, which ⛔ unconditionally
	//  `SetUserFocus(ViewportWidget)` (`:6439-6452`) ⇒ ⛔ THE MATCH GETS ITS KEYBOARD BACK
	//  WITHOUT THIS WIDGET TOUCHING FOCUS AT ALL — which is why there is deliberately no focus
	//  call on the unregister path (and why the subsystem's own contract forbids one).
	//  ⚠️ DECLARED LIMIT: if ANOTHER cursor owner is still up at close (e.g. the Alt-held
	//  `IA_UICursor`), `bWantCursor` stays true, `FInputModeGameOnly` is not applied and that
	//  free focus restore ⛔ does not happen. Slate then drops focus from the collapsed button on
	//  its own. ⛔ Named rather than patched: a focus call here would stomp a nested registration.
	//
	//  ⛔ (5) TASK-1432 QA LOOP 1 — ⛔ THIS IS ⛔ NO LONGER THE ONLY REGISTRATION EDGE, AND A
	//  READER WHO ASSUMES IT IS WILL GET THE LIFETIME WRONG. `ShowDetailForAction` and
	//  `ReturnToList` carry the ⛔ SECOND edge, because this screen's only stop lives inside the
	//  switcher's list branch and a `SWidgetSwitcher` ⛔ REFUSES FOCUS to its inactive slot while
	//  ⛔ leaving every visibility flag untouched (`qa/TASK-1433.md` BLOCKER-1; the measurement
	//  and the three rejected alternatives are at `ShowDetailForAction`'s own block).
	//  ⇒ ⛔ THE INVARIANT, IN ONE LINE: ~~**registered ⟺ `bHelpOpen` AND the list view is up.**~~
	//  Clause (3) above is unchanged and still true — this function still owns the OPEN/CLOSE
	//  edge, exactly once per real change — it is simply ⛔ no longer the whole contract.
	//
	//  🚨 ⛔ (6) TASK-1478 — ⛔ THE INVARIANT IS ⛔ SIMPLER AGAIN, AND THIS FUNCTION IS ⛔ ONCE MORE
	//  THE ⛔ ONLY PLACE A REGISTRATION IS ⛔ TAKEN OR ⛔ GIVEN BACK. ⛔ **registered ⟺ `bHelpOpen`.**
	//  ⛔ The detail page now has a ring of its own (~~⭐ `TASK-1484`:
	//  `{ BackButton, DetailScrollButton }` — ⛔ was `{ BackButton }`~~ ⇒ ⭐ `TASK-1496`: ⛔ back to
	//  ⛔ `{ BackButton }`, 🧑 his ruling 2026-09-26; ⛔ the count changed ⛔ TWICE, ⛔ this
	//  invariant did ⛔ neither time — ⛔ **registered ⟺ `bHelpOpen`** is ⛔ untouched by this
	//  removal and ⛔ must stay that way), so there is ⛔ nothing left
	//  for a view switch to unregister; `ShowDetailForAction` and `ReturnToList` ⛔ REFRESH this
	//  registration rather than ⛔ ending it, and the ring ⛔ follows the view. ⛔ Clause (5)'s
	//  measurement of the switcher is ⛔ still exactly right — what answers it is now
	//  `ApplyActiveView`'s branch collapse instead of an absent registration.
	// ═══════════════════════════════════════════════════════════════════════════════════════
	if (bOpen)
	{
		// ═══ ⭐⭐ TASK-1478 — ⛔ THE DIVERGENCE DETECTOR, AND IT IS ⛔ NOT TAUTOLOGICAL HERE ══════
		// ⛔ `ApplyActiveView` writes the switcher index and the two branch visibilities from ⛔ one
		// parameter, so ⛔ inside that function a check of their agreement would assert what the
		// previous three lines just did. ⛔ HERE IS ⛔ FAR FROM THE WRITER — a different function, a
		// different edge, at the ⛔ one instant the screen becomes visible to the player — so this
		// catches the ⛔ ONE residual `ApplyActiveView`'s own block declares rather than denies: a
		// ⛔ FUTURE path that writes `ViewSwitcher->SetActiveWidgetIndex` ⛔ outside that function and
		// leaves index and visibility disagreeing. ⛔ The symptom of that bug with no check is an
		// ⛔ EMPTY OVERLAY (active branch collapsed) or a ⛔ SWALLOWED RING (inactive branch visible),
		// ⛔ neither of which prints anything.
		// ⛔ IT CANNOT FIRE TODAY — three call sites, all `ApplyActiveView` — and ⛔ IT CANNOT FIRE IN
		// SHIPPING: `DO_ENSURE` is `USE_ENSURES_IN_SHIPPING` (0 by default), and with it 0
		// `ensureMsgf` is `(LIKELY(!!(InExpression)))` — evaluated, ⛔ never reported, ⛔ never fatal.
		// ⛔ ASKED OF THE ⛔ ACTIVE branch only, via `IsDetailViewActive()`, which reads the ⛔ switcher
		// — ⛔ the same single source of truth every other reader on this screen uses.
		const UWidget* const DetailBranch = DetailView;
		const UWidget* const ListBranch   = PanelBorder;
		const UWidget* const ActiveBranch = IsDetailViewActive() ? DetailBranch : ListBranch;
		ensureMsgf(
			ActiveBranch == nullptr || ActiveBranch->GetVisibility() != ESlateVisibility::Collapsed,
			TEXT("[ControlsHelp] The ACTIVE ViewSwitcher branch is Collapsed as the overlay opens - something wrote SetActiveWidgetIndex outside ApplyActiveView. The overlay will render empty and hold no ring."));

		RegisterAsMenuNavTarget();
	}
	else
	{
		UnregisterAsMenuNavTarget();
	}

	UE_LOG(LogSiegeControlsHelp, Log, TEXT("[ControlsHelp] Overlay %s."), bOpen ? TEXT("opened") : TEXT("closed"));

	// ⭐ THIS IS WHAT LETS THE CLOSE BUTTON RELEASE THE CURSOR POSTURE. The controller binds
	// this and calls SetControlsHelpOpen(false) — the UWarMapWidget::OnMapOpenChanged
	// precedent, and the reason a posture flag can never survive a close route the controller
	// never saw.
	OnHelpOpenChanged.Broadcast(bOpen);
}

void USiegeControlsHelpWidget::RefreshRows()
{
	if (RowScrollBox == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] RefreshRows: no RowScrollBox - the overlay has no list to fill."));
		return;
	}

	// ⛔ EVERY ROW IS DESTROYED FIRST. That is what makes "no key string is cached across
	// opens" (`HELP-§1` / `KBD-§0` ruling 2) a STRUCTURAL property rather than a promise:
	// there is no surviving object that could hold a stale letter.
	ClearRowWidgets();

	const APlayerController* const OwningController = GetOwningPlayer();

	// ⛔ A null subsystem is a FAIL-SAFE, not an error (`KBD-§5`): the fallback lane degrades
	// to the reference keys unchanged and the overlay behaves exactly as it would on a
	// positionally-QWERTY host.
	const USiegeKeyboardLayoutSubsystem* const LayoutSubsystem = ResolveKeyboardLayoutSubsystem();
	if (LayoutSubsystem == nullptr && !bLoggedNoLayoutSubsystem)
	{
		bLoggedNoLayoutSubsystem = true;
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] No USiegeKeyboardLayoutSubsystem - unmapped rows fall back to their QWERTY reference keys. Mapped rows are UNAFFECTED (their keys come from the applied context)."));
	}

	// A soft, null-safe row class: /Game/UI/WBP_ControlsHelpRow is RESERVED and unauthored, so
	// this resolves null today and the code-authored row wins (`HELP-§3`). ⚠️ Written as two
	// statements, not a ternary: a TSubclassOf-vs-UClass* conditional re-opens the C2445
	// ambiguity UWarMapWidget documents.
	UClass* ResolvedRowClass = RowWidgetClass.LoadSynchronous();
	if (ResolvedRowClass == nullptr)
	{
		ResolvedRowClass = USiegeControlsHelpRowWidget::StaticClass();
	}

	FName CurrentCategory = NAME_None;
	FText ControlsHelpOwnChip;

	for (const FSiegeControlsHelpAction& Row : FSiegeControlsHelpRegistry::GetActions())
	{
		// ---- category header, emitted on every change of Category ----------------------
		if (Row.Category != CurrentCategory)
		{
			CurrentCategory = Row.Category;

			if (UTextBlock* HeaderText = WidgetTree
				? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
				: nullptr)
			{
				HeaderText->SetText(FSiegeControlsHelpRegistry::GetCategoryDisplayText(CurrentCategory));
				HeaderText->SetFontSize(22.f);
				HeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.80f, 0.45f, 1.f)));

				if (UScrollBoxSlot* HeaderSlot = Cast<UScrollBoxSlot>(RowScrollBox->AddChild(HeaderText)))
				{
					HeaderSlot->SetPadding(FMargin(4.f, 14.f, 4.f, 6.f));
					HeaderSlot->SetHorizontalAlignment(HAlign_Left);
				}
			}
		}

		// ---- ⭐ THE LABEL LANE, IN TWO CALLS AND NO OTHERS ------------------------------
		// (1) the LIVE query — ⚠️ already-retargeted keys, ⛔ zero GetPositionalKey calls;
		// (2) the pure lane resolver — where the single translation lives, and only on the
		//     fallback. Both are static and both are asserted directly by
		//     Tests/SiegeControlsHelpTest.cpp on a QWERTY machine.
		const TArray<FKey> AppliedKeys = QueryAppliedKeysForRow(Row, OwningController);
		const TArray<FKey> DisplayKeys = FSiegeControlsHelpRegistry::ResolveRowDisplayKeys(Row, AppliedKeys, LayoutSubsystem);
		const FText ChipText = FSiegeControlsHelpRegistry::ComposeKeyChipLabel(Row, DisplayKeys);

		// ⭐ `HELP-§4`: the menu documents its OWN key, and the hint line reads it from this
		// row rather than typing it.
		if (Row.ActionId == FName(TEXT("Interface.ControlsHelp")) && DisplayKeys.Num() > 0)
		{
			ControlsHelpOwnChip = ChipText;
		}

		USiegeControlsHelpRowWidget* const RowWidget = CreateWidget<USiegeControlsHelpRowWidget>(this, ResolvedRowClass);
		if (RowWidget == nullptr)
		{
			UE_LOG(LogSiegeControlsHelp, Warning,
				TEXT("[ControlsHelp] Could not create the row widget for '%s' - that row is missing from the list."),
				*Row.ActionId.ToString());
			continue;
		}

		// ⛔ STAMP BEFORE ADD: AddChild triggers the lazy rebuild, and SetRowContent stores
		// unconditionally so either order works (BLOCKER 674-1). Stamping first is still the
		// cheaper path — it writes the strings once instead of twice.
		RowWidget->SetRowContent(Row.ActionId, Row.DisplayName,
			FSiegeControlsHelpRegistry::ComposeOneLineForDisplay(Row), ChipText);

		// ⭐ THE TASK-707 SEAM, BOUND IN EXACTLY ONE PLACE (see the class comment §7a).
		RowWidget->OnRowActivated.BindUObject(this, &USiegeControlsHelpWidget::HandleRowActivated);

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(RowScrollBox->AddChild(RowWidget)))
		{
			RowSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 2.f));
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		RowWidgets.Add(RowWidget);
	}

	if (HintText != nullptr)
	{
		if (ControlsHelpOwnChip.IsEmptyOrWhitespace())
		{
			// IA_ControlsHelp has not resolved (TASK-705 not landed, or the asset is missing):
			// ⛔ the honest degradation, ⛔ never a guess at a key name.
			HintText->SetText(FText::FromString(FString(SiegeControlsHelpText::HintNoKey)));
		}
		else
		{
			// ⭐ The overlay's own key, spliced in from its OWN registry row's derived chip —
			// ⛔ never typed (`HELP-§4`: the menu documents its own key, and Tab is a
			// positional key like any other).
			HintText->SetText(FText::FromString(
				FString(SiegeControlsHelpText::HintKeyPrefix)
				+ ControlsHelpOwnChip.ToString()
				+ FString(SiegeControlsHelpText::HintKeySuffix)));
		}
	}

	UE_LOG(LogSiegeControlsHelp, Log,
		TEXT("[ControlsHelp] Rebuilt %d of %d registry rows with freshly derived key labels."),
		RowWidgets.Num(), FSiegeControlsHelpRegistry::GetActions().Num());
}

void USiegeControlsHelpWidget::ClearRowWidgets()
{
	for (const TObjectPtr<USiegeControlsHelpRowWidget>& RowWidget : RowWidgets)
	{
		if (RowWidget != nullptr)
		{
			// Unbind before release: this widget is the bind target, and a row outliving it
			// through some future retain path must not call into a dead overlay.
			RowWidget->OnRowActivated.Unbind();
			RowWidget->RemoveFromParent();
		}
	}
	RowWidgets.Reset();

	// Also drops the category headers, which are plain children with no member to clear.
	if (RowScrollBox != nullptr)
	{
		RowScrollBox->ClearChildren();
	}
}

void USiegeControlsHelpWidget::HandleRowActivated(FName InActionId)
{
	// ⛔ VALIDATE AGAINST THE REGISTRY BEFORE ROUTING ANYTHING ONWARD. An id that is not a real
	// row would reach TASK-707's detail view as a lookup miss and render a blank page — the
	// silent-omission failure `HELP-§2` mechanism 2 exists to prevent.
	const FSiegeControlsHelpAction* const Row = FSiegeControlsHelpRegistry::FindAction(InActionId);
	if (Row == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] A row reported the id '%s', which is not in the registry - the click is dropped and nothing is shown."),
			*InActionId.ToString());
		return;
	}

	SelectedActionId = InActionId;

	// ⭐ THE FULL-SCREEN PAGE OPENS HERE (§7c). ⛔ Called BEFORE the broadcast and its result is
	// deliberately not consulted: a failure to build the page must not stop an external consumer
	// from learning which row was clicked, and it must not leave SelectedActionId disagreeing
	// with what the player did.
	ShowDetailForAction(InActionId);

	// Broadcast LAST, so any external consumer observes a state the widget has already applied.
	OnRowSelected.Broadcast(InActionId);
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  🚨🚨🚨 TASK-1478 [CONTROLS-HELP-NAVIGABLE] — ⛔ THE SWITCHER ANSWER, AND IT IS THE WHOLE OF
//  THIS ROW'S HAZARD HALF. ⛔ READ IT BEFORE TOUCHING EITHER FLIP SITE
//  (`ConstructRowTree`'s `ApplyButtonFocusable(RowButton)` / `ConstructDetailTree`'s
//  `ApplyButtonFocusable(BackButton)`) AND ⛔ BEFORE ADDING A THIRD SWITCHER BRANCH.
//
//  ⛔ THE PROBLEM THE FLIPS CREATE, IN ONE SENTENCE: with both branches now holding focusable
//  `UButton`s, the walker — which reads `GetActiveNavTarget()`'s WHOLE `WidgetTree` and, since
//  ⭐ `TASK-1474`, ⛔ descends into nested code-authored `UUserWidget`s — collects ⛔ BOTH
//  branches' buttons at once. The ones in the ⛔ INACTIVE branch are ⛔ ADMITTED (every
//  visibility flag along their chain reads visible) and ⛔ REFUSE FOCUS
//  (`SWidgetSwitcher::ValidatePathToChild` → `return InChild == GetActiveWidget().Get();`), and
//  such a stop ⛔ SWALLOWS THE RING. ⛔ That is the exact defect `qa/TASK-1433.md` BLOCKER-1
//  named and ⭐ `TASK-1469` was boarded to remove from the Graphics screen.
//
//  ⚖️⚖️ THE FOUR SHAPES I MEASURED, AND ⛔ WHY THREE OF THEM ARE ⛔ NOT AVAILABLE:
//
//   ⛔ (α) REJECTED — *`RegisterMenuNavTarget(DetailView)` while the detail page is up, so the
//     walker walks the detail widget's OWN tree and the list branch is out of scope for free.*
//     ⛔ THIS IS THE ONE I WANTED AND IT IS ⛔ MEASURED DEAD: `GetRegisteredNavTarget()` accepts a
//     stack entry only if `Screen->IsInViewport() && Screen->IsVisible()`, and
//     `UWidget::IsInViewport()` is `bIsManagedByGameViewportSubsystem` + `IsWidgetAdded(this)`
//     (`UMG/Private/Components/Widget.cpp:344-350`) — a flag set ⛔ only by `AddToViewport` /
//     `AddToPlayerScreen`. ⛔ `DetailView` is `WidgetTree->ConstructWidget`'d into a slot and is
//     ⛔ NEVER added to the viewport ⇒ it would be ⛔ pushed onto the stack and ⛔ skipped on every
//     read, and navigation would fall through to `FindMainMenuWidget()`. ⇒ ⛔ INERT, ⛔ SILENTLY —
//     the failure class this row exists to catch. ⛔ I did not take this on trust from the
//     subsystem's comment; I read the engine function.
//   ⛔ (β) REJECTED — *teach `IsNavFocusStop` about `SWidgetSwitcher`.* ⛔ The general fix and the
//     right one eventually, but it is ⛔ one function in `SiegeMenuInputSubsystem.cpp`, which this
//     row ⛔ may not write (six rows' work, all `built`). ⛔ Escalation, not silence: it is named
//     in this row's handoff as the thing that would make the collapse below ⛔ belt-and-braces.
//   ⛔ (γ) REJECTED — *toggle `IsFocusable` on the buttons at the switch edges.* ⛔ It would work
//     on the walker and ⛔ lie to everyone else: `UButton::RebuildWidget` reads that field ⛔ exactly
//     once (`Button.cpp:84`), so a post-build write changes ⛔ what the predicate reports and
//     ⛔ nothing about the live `SButton` — leaving a button whose UMG field says "not focusable"
//     and whose Slate side takes focus on a mouse click. ⛔ This file's own `ApplyButtonNotFocusable`
//     comment is a 20-line warning against exactly that misreading; adding a runtime write would
//     make that comment false.
//   ✅ (δ) TAKEN — ⛔ COLLAPSE THE INACTIVE BRANCH. `HasVisibleSlateAncestry` walks the SLATE
//     parent chain and refuses anything under a `Collapsed`/`Hidden` ancestor, so a collapsed
//     branch's buttons are dropped by the ⛔ SHIPPED predicate with ⛔ no new predicate written.
//     ⭐ AND IT IS THE ⛔ SAME MECHANISM ⭐ `TASK-1469` LIMB 2 WAS BUILT FOR AND MEASURED ON
//     (`VideoModeConfirmBorder` Collapsed ⇒ `KeepSettingsButton`/`RevertSettingsButton` excluded),
//     ⛔ not a novel one.
//     ⭐⭐ AND THE REAL ARGUMENT FOR IT, WHICH IS ⛔ NOT "IT IS CONVENIENT": ⛔ THE COLLAPSE MAKES
//     THE UMG-VISIBLE STATE ⛔ AGREE WITH WHAT THE SWITCHER WAS ALREADY DOING. `SWidgetSwitcher`
//     already declines to arrange, render or hit-test the inactive branch; the ⛔ only thing that
//     was untrue of it was its ⛔ visibility ATTRIBUTE, and that attribute is ⛔ precisely what the
//     predicate reads. ⇒ ⛔ WE ARE NOT MODELLING THE SWITCHER; ⛔ WE ARE REMOVING THE NEED TO, by
//     stopping the tree from ⛔ lying about a branch the engine had already switched off.
//
//  ⛔⛔ AND THIS IS ⭐ `TASK-1432`'s ⛔ REJECTED (b) — *"collapse the branch so the existing ancestor
//  test does the work"* — ⛔ BEING TAKEN AFTER BEING REFUSED. ⛔ I am not pretending otherwise, and
//  its ⛔ TWO objections are answered ⛔ mechanically rather than waved past:
//    ⛔ OBJECTION 1, *"it invents a SECOND source of truth for which view is up."* ⛔ ANSWERED BY
//      SHAPE: ⛔ there is ⛔ no second state. `IsDetailViewActive()` still asks the ⛔ switcher and is
//      still the only reader-facing truth; the two visibility writes are a ⛔ DERIVED PROJECTION of
//      this function's ⛔ single parameter, ⛔ written in the same statement sequence that writes the
//      index. ⛔ There is nothing to forget to update, because a "restore" is ⛔ not a separate
//      action — ⛔ EVERY call writes ⛔ BOTH branches. ⛔ And this function holds the ⛔ ONLY
//      `ViewSwitcher->SetActiveWidgetIndex(...)` write in the file (⛔ grep it: ⛔ one write, and
//      the ⛔ three view switches — `ConstructHelpTree`, `ShowDetailForAction`, `ReturnToList` —
//      ⛔ all route through here).
//    ⛔ OBJECTION 2, *"the failure mode of a missed restore is a screen whose only on-screen exit is
//      invisible."* ⛔ ANSWERED BY CONSTRUCTION, ⛔ not by care: ⛔ `bDetail` below is true ⛔ only
//      when a ⛔ BUILT `DetailView` and a ⛔ BUILT `ViewSwitcher` both exist ⇒ ⛔ `PanelBorder` can be
//      collapsed ⛔ only in the same statement sequence that makes a real detail page ⛔ visible and
//      ⛔ active. ⛔ There is no reachable state in which the list is hidden and nothing replaces it.
//      ⛔ The degraded no-switcher shape `ConstructHelpTree` falls back to therefore ⛔ never
//      collapses anything at all.
//    ⚠️ ⛔ AND THE RESIDUAL IS DECLARED RATHER THAN DENIED: if a ⛔ FUTURE path writes
//      `SetActiveWidgetIndex` ⛔ outside this function, index and visibility ⛔ can diverge. ⛔ Two
//      things catch it: the `ensureMsgf` below (⛔ slot order) and the one in `ApplyOpenState` (⛔ the
//      active branch is not collapsed ⛔ at the moment the player sees the screen). ⛔ Neither can
//      fire today; ⛔ both fire the first time somebody adds that path.
//
//  ⛔ WHY THE ⛔ REGISTRATION IS ⛔ NOT DONE HERE: this function is called from `ConstructHelpTree`,
//  ⛔ where registering anything would be the `SC-§36.1` never-arms defect `ApplyOpenState`'s block
//  spends fifteen lines on. ⛔ It stays a ⛔ PURE VIEW-STATE WRITER; the two ⛔ EDGE-guarded
//  re-registrations stay at their call sites, where `bWasOnList` / `bWasOnDetail` / `bHelpOpen` are
//  in scope and mean something.
// ═══════════════════════════════════════════════════════════════════════════════════════════
void USiegeControlsHelpWidget::ApplyActiveView(int32 InViewIndex)
{
	// ⛔ THE DETAIL VIEW IS ONLY REACHABLE WHEN THERE IS ONE. Three conjuncts, ⛔ each load-bearing:
	// no switcher ⇒ the degraded single-view shape and ⛔ nothing may be collapsed; no `DetailView`
	// ⇒ the switcher has ⛔ one child and collapsing `PanelBorder` would empty the screen; and the
	// index must actually be the detail one. ⇒ ⛔ OBJECTION 2 ABOVE IS CLOSED ⛔ ON THIS LINE.
	const bool bDetail =
		(ViewSwitcher != nullptr) && (DetailView != nullptr) && (InViewIndex == DetailViewIndex);

	if (ViewSwitcher != nullptr)
	{
		// ⛔ AN `ensure`, ⛔ NOT A COMMENT, AND THE SPEC ASKED WHICH — THIS ONE EARNS IT BECAUSE IT
		// GUARDS AN INVARIANT THAT CAN ROT ⛔ WITHOUT ANYBODY EDITING THIS FUNCTION. `ListViewIndex`
		// and `DetailViewIndex` are ⛔ the ORDER of two `AddChild` calls a hundred lines away
		// (`ConstructHelpTree`, whose own comment already calls that order "the contract"). ⛔ Before
		// this row, swapping them would have shown the wrong PAGE — visible instantly. ⛔ AFTER this
		// row it would also collapse the ⛔ WRONG BRANCH, i.e. hide the list ⛔ while the list is up:
		// ⛔ the exact stranding mode `TASK-1432`'s REJECTED (b) feared. ⛔ A comment cannot catch a
		// reorder; this can.
		// ⛔ IT CANNOT FIRE TODAY (construction adds `PanelBorder` then `DetailView`, in that order,
		// and nothing else ever calls `AddChild` on this switcher) and ⛔ IT CANNOT FIRE IN SHIPPING:
		// `DO_ENSURE` is `USE_ENSURES_IN_SHIPPING` (`Core/Public/Misc/Build.h:305`), 0 by default,
		// and with it 0 `ensureMsgf` expands to ⛔ `(LIKELY(!!(InExpression)))`
		// (`AssertionMacros.h:470`) — the expression is evaluated, ⛔ nothing is reported and
		// ⛔ nothing crashes. The expression is two `TArray::IsValidIndex` lookups and two pointer
		// compares, so even a Shipping build that DID enable ensures pays nothing.
		// ⛔ NULL-SAFE BY THE ENGINE'S OWN GUARD: `UPanelWidget::GetChildAt` returns `nullptr` for an
		// out-of-range index (`PanelWidget.cpp:39-47`), so a half-built tree compares
		// `nullptr == nullptr` and ⛔ passes rather than tripping a false alarm on a real degradation.
		const UWidget* const ActualListBranch   = ViewSwitcher->GetChildAt(ListViewIndex);
		const UWidget* const ActualDetailBranch = ViewSwitcher->GetChildAt(DetailViewIndex);
		const UWidget* const ExpectedListBranch   = PanelBorder;
		const UWidget* const ExpectedDetailBranch = DetailView;

		ensureMsgf(
			ActualListBranch == ExpectedListBranch && ActualDetailBranch == ExpectedDetailBranch,
			TEXT("[ControlsHelp] ViewSwitcher slot order no longer matches ListViewIndex/DetailViewIndex - ApplyActiveView would collapse the WRONG branch and hide the list while it is up."));
	}

	// ⛔ BOTH BRANCHES, ⛔ EVERY CALL, ⛔ FROM THE ONE PARAMETER. That is what makes "a missed
	// restore" un-writeable rather than merely unlikely (see OBJECTION 1 above).
	//
	// ⛔ THE VALUES ARE THE AUTHORED ONES, ⛔ NOT NEW ONES, so the ACTIVE branch is byte-for-byte what
	// it was before this row: `PanelBorder` is authored `Visible` in `ConstructHelpTree` (⛔ and that
	// is correctness — the plate absorbs its OWN clicks so a row click cannot fall through into a
	// live match and place a card), and `DetailView` has never had its visibility written at all, so
	// it carries `UWidget`'s `Visible` default. ⇒ ⛔ ON EVERY FRAME THE PLAYER ACTUALLY SEES, ⛔ THIS
	// FUNCTION RESTORES THE EXACT STATE THAT SHIPPED. ⛔ The only changed frames are the ones where a
	// branch is not being rendered anyway.
	//
	// ⛔ AND THE COLLAPSE ITSELF IS VISUALLY INERT, ⛔ MEASURED: `SWidgetSwitcher::OnArrangeChildren`
	// arranges `GetActiveSlot()` and ⛔ nothing else, and `ComputeDesiredSize` reads ⛔ only the
	// ACTIVE slot's child visibility — which is ⛔ never the collapsed one. ⇒ collapsing the inactive
	// branch changes ⛔ no layout, ⛔ no pixel and ⛔ no hit test. ⛔ It changes exactly one thing:
	// what `HasVisibleSlateAncestry` answers about the buttons underneath it.
	if (PanelBorder != nullptr)
	{
		PanelBorder->SetVisibility(bDetail ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (DetailView != nullptr)
	{
		DetailView->SetVisibility(bDetail ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// ⛔ THE INDEX LAST, so the one value every reader of this screen greps for (`IsDetailViewActive`
	// asks it) flips only once the branch state behind it is already complete. ⛔ Nothing observes
	// between the writes: `UWidgetSwitcher::SetActiveWidgetIndex` broadcasts no delegate and this
	// function calls nothing re-entrant.
	// ⛔ IDEMPOTENT: `SetActiveWidgetIndex` opens with `if (ActiveWidgetIndex != Index)`, so a
	// list→list call costs nothing — which is what lets `OpenHelp()`/`CloseHelp()` keep calling
	// `ReturnToList()` unconditionally.
	if (ViewSwitcher != nullptr)
	{
		ViewSwitcher->SetActiveWidgetIndex(bDetail ? DetailViewIndex : ListViewIndex);
	}
}

void USiegeControlsHelpWidget::ShowDetailForAction(FName InActionId)
{
	// ⛔⛔ THE FENCES THIS FUNCTION LIVES INSIDE, RESTATED WHERE THEY APPLY:
	//   • ⛔ NO SetInputMode, ⛔ NO bShowMouseCursor, ⛔ NO second cursor owner — this overlay's
	//     ONE registration in ApplyCursorInputState() already covers the detail view (TASK-074).
	//   • ⛔ NO `Escape` handler, here or in USiegeControlsDetailWidget (`AS-§6 A-2`, `HELP-§5`).
	//   • ⛔ NOTHING IS MUTATED IN THE WORLD: this reads the registry, the layout subsystem and
	//     Enhanced Input, and writes only to a widget (`HELP-§5`, read-only on the world).
	//   • ⛔ It does NOT pause: the battle keeps running behind the page (row H2).

	// Defence in depth. HandleRowActivated has already validated the id, but this function is
	// `virtual` and `protected` — a subclass or a later caller must not be able to open a page
	// for a row that does not exist.
	const FSiegeControlsHelpAction* const Row = FSiegeControlsHelpRegistry::FindAction(InActionId);
	if (Row == nullptr)
	{
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] ShowDetailForAction('%s'): not a registry row - the list stays up and no page is opened."),
			*InActionId.ToString());
		return;
	}

	if (ViewSwitcher == nullptr || DetailView == nullptr)
	{
		// ⛔ THE LIST STAYS ON SCREEN. The alternative — switching to a view that does not exist —
		// is the blank screen `HELP-§2` mechanism 2 exists to prevent, and it would be a screen
		// with no way out because `Escape` is not available as one.
		UE_LOG(LogSiegeControlsHelp, Warning,
			TEXT("[ControlsHelp] ShowDetailForAction('%s'): no detail view was built - the row click is still reported, but the list stays up."),
			*InActionId.ToString());
		return;
	}

	const APlayerController* const OwningController = GetOwningPlayer();

	// ⛔ A null subsystem is a FAIL-SAFE, not an error (`KBD-§5`) — exactly as in RefreshRows.
	const USiegeKeyboardLayoutSubsystem* const LayoutSubsystem = ResolveKeyboardLayoutSubsystem();

	// ⭐ DERIVED AT CLICK TIME, ⛔ NOT REUSED FROM THE LIST'S PASS. Nothing composed for the row
	// list is carried over here: the page re-queries Enhanced Input and re-reads the layout
	// accessor, so a mid-session Win+Space between opening the overlay and clicking a row cannot
	// leave a stale letter on the page (`KBD-§0` ruling 2; the 1 Hz poll keeps the accessor
	// current, and `KBD-§8`'s "the caller refreshes" duty is discharged once per open in
	// OpenHelp — ⛔ this is a READ, not a second refresh).
	const FSiegeControlsDetailContent Content = FSiegeControlsHelpRegistry::ComposeDetailContent(
		*Row,
		LayoutSubsystem,
		[OwningController](const FSiegeControlsHelpAction& QueryRow) -> TArray<FKey>
		{
			// ⛔⛔ THE SAME ONE-TRANSLATION ROUTE AS THE ROW LIST, AND THE SAME ZERO
			// GetPositionalKey CALLS ON THE LANE-A PRIMARY PATH. The detail lane gets no second
			// way to reach a key (`HELP-§1`; SiegePlayerController.h:1222-1226).
			return USiegeControlsHelpWidget::QueryAppliedKeysForRow(QueryRow, OwningController);
		});

	// ⛔ THE EDGE IS MEASURED ⛔ BEFORE THE SWITCH, AND IT IS READ FROM THE SWITCHER RATHER THAN
	// FROM A MIRRORED BOOL — `IsDetailViewActive()`, for the reason that function's own comment
	// gives: one source of truth means the answer and the screen cannot disagree. See the
	// registration block below, which is what this value is for.
	const bool bWasOnList = !IsDetailViewActive();

	// STAMP BEFORE SWITCHING, so the page is never on screen for a frame carrying the previous
	// row's text.
	DetailView->SetDetailContent(Content);

	// ⭐ TASK-1478 — ⛔ THROUGH `ApplyActiveView`, which switches the index ⛔ AND collapses the list
	// branch in the same call. ⛔ The stamp above still happens FIRST, so the page is never on screen
	// for a frame carrying the previous row's text.
	ApplyActiveView(DetailViewIndex);

	UE_LOG(LogSiegeControlsHelp, Log,
		TEXT("[ControlsHelp] Detail page open for '%s' (%d related control(s))."),
		*InActionId.ToString(), Content.Related.Num());

	// ═══ 🚨🚨 TASK-1432 QA LOOP 1 — `qa/TASK-1433.md` BLOCKER-1: THE RING FOLLOWS THE VIEW ════
	//  ⛔ THE CLAIM THIS REPLACES WAS FALSE, AND I RE-MEASURED THE REFUTATION MYSELF RATHER THAN
	//  ACCEPTING IT. The first cut of this row asserted that switching to the detail page
	//  "collapses the list branch" so the walker drops `CloseButton` on its own. ⛔ It does not:
	//    • `Slate/Private/Widgets/Layout/SWidgetSwitcher.cpp` — `OnArrangeChildren` calls
	//      `ArrangeSingleChild` for `GetActiveSlot()` and ⛔ NOTHING ELSE; the only two
	//      occurrences of the string `Visibility` in the whole file are a ⛔ READ inside
	//      `ComputeDesiredSize` (`const EVisibility ChildVisibility = Widget->GetVisibility();`).
	//      ⛔ The switcher ⛔ WRITES no child's visibility anywhere. It declines to ARRANGE the
	//      inactive branch; it does not ⛔ COLLAPSE it.
	//    • `UMG/Private/Components/WidgetSwitcher.cpp` — `SetActiveWidgetIndexForSlateWidget`
	//      forwards a clamped index and touches no child.
	//    • `UMG/Private/Components/Widget.cpp` — `UWidget::IsVisible()` returns
	//      `SafeWidget->GetVisibility().IsVisible()`, i.e. the widget's ⛔ OWN visibility.
	//  ⇒ with the detail page up, `CloseButton` and every Slate ancestor still report their own
	//  visibility as visible ⇒ ⛔ `HasVisibleSlateAncestry` returns TRUE ⇒ ⛔ `IsNavFocusStop`
	//  ⛔ ADMITS a stop that ⛔ CANNOT TAKE FOCUS.
	//
	//  🚨 AND THE ⛔ FOURTH SITE IS THE DECISIVE ONE — the engine names this exact widget in its
	//  own comment. `SlateCore/Private/Application/SlateWindowHelper.cpp`,
	//  `FSlateWindowHelper::FindPathToWidget`: *"Even if the parent pointer is valid, and even if
	//  the visibility is visible, it's possible a widget shows and hides children without ever
	//  removing them this is the case with widgets like SWidgetSwitcher"* — followed by
	//  `if (!CurWidgetParent->ValidatePathToChild(CurWidget.Get())) { ...Empty(); return false; }`,
	//  and `SWidgetSwitcher::ValidatePathToChild` is `return InChild == GetActiveWidget().Get();`.
	//  ⇒ `FindPathToWidget` ⛔ FAILS for anything in the inactive slot ⇒ `SetUserFocus` fails.
	//  ⛔ `CloseButton` IS in the inactive slot here: the tree is
	//  `BackdropBorder → ViewSwitcher → [0] PanelBorder → RootPanel → CloseButton` (see
	//  `ConstructHelpTree`), so the detail page parks this screen's ONLY stop behind slot 0.
	//
	//  ⛔ AND THE SUBSYSTEM HAD ALREADY WRITTEN THE HAZARD DOWN — its `HasVisibleSlateAncestry`
	//  comment says in terms that it does not model `SWidgetSwitcher`, that such a stop *"would
	//  still be admitted here and would still refuse focus"*, and that this stays latent only
	//  because *"the only `UWidgetSwitcher` in the project is `SiegeControlsHelpWidget`'s
	//  `ViewSwitcher`, and that overlay never registers"*. ⛔ THIS ROW IS WHAT MAKES IT REGISTER.
	//  The consequence, also in that file's own words, is that a stop which cannot take focus
	//  ⛔ SWALLOWS the ring rather than being skipped — `MoveFocus` re-reads the index from Slate
	//  every press, so the failed request leaves the index where it was and the next press
	//  repeats it forever. ⛔ Reachable by 🧑 a mouse click on any row (`HELP-§5`: rows are
	//  clickable), which is a state a player reaches on purpose.
	//
	//  ⚖️ THE REMEDY ⭐ `TASK-1432` CHOSE — the registration's lifetime becomes ⛔ EXACTLY THE RING'S:
	//  ~~**registered ⟺ the overlay is open ⛔ AND the list view is up.**~~ A screen with no
	//  reachable stop does not hold the ring; the arrow keys were ⛔ INERT on the detail page instead
	//  of ⛔ DEAD on it, and both close routes were untouched.
	//   ⛔ REJECTED (a) — *teach the predicate about `SWidgetSwitcher`.* That is the general fix
	//     and it is the right one eventually, but it is ⛔ ONE FUNCTION IN A FILE THIS ROW MAY
	//     NOT WRITE (`SiegeMenuInputSubsystem.cpp`, held by `TASK-1472`, queued for `TASK-1474`)
	//     — and it would leave the hazard live in the meantime. ⭐ When it lands, these two edges
	//     become belt AND braces rather than wrong: they can only ever remove a stop that the
	//     fixed predicate would also have removed, so the two agree by construction.
	//   ⛔ REJECTED (b) — *collapse the list branch so the existing ancestor test does the work.*
	//     It would make the old sentence true, and it is the wrong trade: it invents a ⛔ SECOND
	//     source of truth for which view is up (the thing `IsDetailViewActive` exists to refuse),
	//     and the failure mode of a missed restore is a screen whose ⛔ only on-screen exit is
	//     invisible. ⛔ A cosmetic gain is not worth a new way to strand the player.
	//   ⛔ REJECTED (c) — *give the detail page a stop of its own instead.* ⛔ Not available
	//     in-fence: its `BackButton` lives inside `USiegeControlsDetailWidget`'s own
	//     `WidgetTree`, which the walker never enters (see the flip site's measurement), and the
	//     two ways to change that — re-rooting the detail class or adding a wrapper button —
	//     both change a shipped screen's visuals, which `AS-§6 A(e)`/`HELP-§6` forbid any agent
	//     adjudicating.
	//
	// ═══ 🚨🚨🚨 TASK-1478 — ⛔ THE REMEDY ABOVE IS ⛔ SUPERSEDED, AND THE ⛔ WHOLE BLOCK IS KEPT
	//  (`SC-§120`) BECAUSE ⛔ EVERY MEASUREMENT IN IT IS STILL TRUE. ⛔ What changed is not the
	//  physics; it is ⛔ WHICH REMEDIES WERE AVAILABLE ══════════════════════════════════════════
	//
	//  ⛔ (c) IS ⛔ NO LONGER TRUE, AND IT IS THE HINGE. Its premise — *"`BackButton` lives inside
	//  `USiegeControlsDetailWidget`'s own `WidgetTree`, ⛔ which the walker never enters"* — was
	//  ⛔ measured FALSE by ⭐ `TASK-1474`'s descent. ⇒ ⛔ THE DETAIL PAGE CAN HAVE A STOP OF ITS OWN
	//  ⛔ WITHOUT RE-ROOTING ANYTHING AND ⛔ WITHOUT MOVING ONE PIXEL: the button was always there and
	//  always visible; ⛔ only its `IsFocusable` flag had to change. ⛔ (c)'s own objection —
	//  *"both ways change a shipped screen's visuals"* — ⛔ does not apply to the third way, which is
	//  the one that existed all along and was invisible because the walker could not reach it.
	//
	//  ⛔ (a) STANDS AND IS STILL ⛔ OUT OF FENCE (⭐ `TASK-1478` may not write the subsystem either),
	//  so the switcher is handled ⛔ screen-side — see `ApplyActiveView`, which collapses the
	//  inactive branch so the ⛔ SHIPPED ancestor predicate does the work. ⛔ (b) is ⛔ TAKEN, with
	//  ⛔ both of its objections answered mechanically ⛔ at that function, ⛔ not here and ⛔ not
	//  waved past.
	//
	//  ⛔⛔ AND THE EDGE BELOW IS ⛔ INVERTED: it ⛔ WAS `UnregisterAsMenuNavTarget()`. ⛔ It is now a
	//  ⛔ RE-REGISTER, and that resolves `qa/TASK-1433.md` ⛔ WARN-L1 rather than inheriting it. QA
	//  ruled these two edges agreed with a switcher-aware predicate ⛔ *"by a contingent authored
	//  fact, not by construction"*, and named ⛔ THE BACKBUTTON FLIP as the counter-case: with it
	//  flipped, such a predicate would ⛔ keep `{BackButton}` while this edge ⛔ unregistered the whole
	//  screen — ⛔ and the failure would be SILENT. ⇒ ⛔ THERE IS NO UNREGISTER-ON-VIEW-SWITCH ANY
	//  MORE. ⛔ **THE INVARIANT IS NOW `registered ⟺ bHelpOpen`**, and the ⛔ view switch ⛔ REFRESHES
	//  that registration so the ring ⛔ FOLLOWS the view instead of ⛔ leaving with it.
	//
	//  ⛔ WHY A RE-REGISTER AND ⛔ NOT NOTHING AT ALL — the ring must be ⛔ MOVED, not merely allowed:
	//  the widget that held focus one line ago is a `RowButton` that is now in a ⛔ collapsed branch,
	//  so Slate drops focus and the page would open ⛔ RINGLESS. `RegisterMenuNavTarget` ⛔ ends in
	//  `FocusFirstNavStop()`, whose idempotence guard asks *"is anything ⛔ among the CURRENT stops
	//  already focused?"* — and the old holder is ⛔ no longer among them, so the guard falls through
	//  and focus lands on the detail page's stop 0, which ⛔ is `BackButton`. ⛔ Re-registering also
	//  ⛔ de-duplicates by identity (`NavTargetStack.RemoveAll` then `Add`), so no entry accumulates.
	//  ⭐ AND IT IS THE ⛔ 5b INSTRUMENT FOR FREE: `LogNavTargetRetarget` prints
	//  `menu nav target registered -> '<screen>' (registered screen), N focus stop(s)` on ⛔ every
	//  switch, so the per-page counts are ⛔ read off the log rather than asserted.
	//
	//  ⛔ GUARDED ON A ⛔ REAL EDGE, ⛔ NOT ON ARRIVAL: `bWasOnList` (measured above) means a second
	//  call for a page that is already up cannot re-place a ring that is already correct, and
	//  `bHelpOpen` means a call taken while the overlay is closed — this function is `virtual` and
	//  `protected`, so a later caller is possible — cannot register an invisible screen.
	//  ⛔ Placed AFTER the switch so nothing observes a state the switcher has not reached; the
	//  switcher's index and the branch visibilities are assigned synchronously, so
	//  `ValidatePathToChild` ⛔ and `HasVisibleSlateAncestry` both already answer with the new view by
	//  the time the subsystem is told.
	// ═══════════════════════════════════════════════════════════════════════════════════════════
	if (bWasOnList && bHelpOpen)
	{
		RegisterAsMenuNavTarget();
	}
}

void USiegeControlsHelpWidget::ReturnToList()
{
	// ⛔ IDEMPOTENT AND NULL-SAFE BY CONSTRUCTION, which is what lets CloseHelp() and OpenHelp()
	// both call it unconditionally: `UWidgetSwitcher::SetActiveWidgetIndex` opens with
	// `if (ActiveWidgetIndex != Index)` and does nothing otherwise, so calling it on an
	// already-listed overlay costs nothing and broadcasts nothing. (⛔ Re-anchored to that text
	// in TASK-1432 QA loop 1 — it was cited as `WidgetSwitcher.cpp:49-57`, which is off by one
	// in UE 5.8; the ⛔ predicate cannot drift the way a line number just did.)
	// ⛔ THIS IS NOT A CLOSE. The overlay's open state is untouched here — the two close routes
	// are still the toggle key and the Close button, and that is the complete list (`HELP-§5`).
	if (ViewSwitcher == nullptr)
	{
		return;
	}

	// ⛔ THE EDGE, MEASURED BEFORE THE SWITCH — the mirror image of `ShowDetailForAction`'s, and
	// read from the same single source of truth.
	const bool bWasOnDetail = IsDetailViewActive();

	// ⭐ TASK-1478 — ⛔ THROUGH `ApplyActiveView`, which restores `PanelBorder` to its authored
	// `Visible` ⛔ and collapses `DetailView` in the same call. ⛔ On the overwhelmingly common
	// list→list call (every `OpenHelp`, every ordinary `CloseHelp`) both writes are ⛔ no-ops onto
	// the values already there, and the index write short-circuits inside the engine.
	ApplyActiveView(ListViewIndex);

	// ═══ 🚨 TASK-1432 QA LOOP 1 — `qa/TASK-1433.md` BLOCKER-1, THE OTHER HALF OF THE EDGE ═════
	//  ⛔ THE RING COMES BACK WITH THE LIST. `RegisterMenuNavTarget` ends in `FocusFirstNavStop()`,
	//  so returning from the detail page ⛔ re-places the outline on ~~`CloseButton`~~ — the player
	//  leaves the page and the outline is where they left it, with no second key press.
	//  ⭐ TASK-1478 — ⛔ THE STOP IT LANDS ON CHANGED, AND IT IS DECLARED RATHER THAN DISCOVERED IN
	//  5b: stop 0 of the list is now the ⛔ FIRST `RowButton`, because `RowScrollBox` precedes
	//  `CloseButton` in `RootPanel`'s slot order and `GetMenuFocusStops` returns depth-first
	//  ⛔ PRE-ORDER. ⇒ ⛔ RETURNING FROM A DETAIL PAGE PUTS THE RING ON THE ⛔ TOP OF THE LIST,
	//  ⛔ NOT ON THE ROW THE PLAYER CAME FROM. ⚖️ ⛔ NOT FIXED HERE, ⛔ ON PURPOSE: remembering the
	//  originating row would need per-screen focus memory and a direct focus call, and this file's
	//  own law (`RegisterAsMenuNavTarget`'s "⛔ no `SetKeyboardFocus`") forbids the second. ⛔ It is
	//  a ring that is in the right PLACE and the wrong SPOT, which is a usability note for 🧑 him to
	//  rule on — ⛔ not a severed ring, and ⛔ not mine to adjudicate (`SC-§50`).
	//
	//  ⛔ AFTER THE SWITCH, AND THAT ORDER IS LOAD-BEARING for exactly the reason `ApplyOpenState`
	//  registers after its `SetVisibility`: `FocusFirstNavStop` reaches `FindPathToWidget`, which
	//  asks `SWidgetSwitcher::ValidatePathToChild` whether this child is the ACTIVE one. One line
	//  earlier the answer is still "no" and the focus request would fail silently.
	//
	//  ⛔ WHY THE CHOKE POINT AND NOT `HandleDetailBackRequested`: the Back seam is the only route
	//  that returns to the list with the overlay staying open ⛔ TODAY. Hooking it instead would
	//  leave a hole the day a second caller appears, and the failure that hole produces is a
	//  ⛔ silently ringless list — the precise defect class this epic exists to remove. The same
	//  argument `ApplyOpenState` makes for owning the open/close edge applies here: the edge lives
	//  with the state, and the state is the switcher's index.
	//
	//  ⛔ `bWasOnDetail` KEEPS IT AN EDGE: `OpenHelp()` and `CloseHelp()` both call this function
	//  unconditionally on every open and every close, and a list-to-list call must register
	//  nothing. `bHelpOpen` keeps it honest about `OpenHelp`'s call, which runs BEFORE
	//  `ApplyOpenState(true)` — at that moment the tree is still Collapsed and a registration
	//  would be asked about an invisible screen and answered "0 stops".
	//
	//  ⚠️ ONE TRANSIENT PAIR IS DECLARED RATHER THAN HIDDEN, because 5b will read it in the log:
	//  closing ⛔ FROM the detail page runs `CloseHelp()` → `ReturnToList()` (registers, ring on
	//  ~~`CloseButton`~~) → `ApplyOpenState(false)` (unregisters) in one call stack, so that ⛔ ONE
	//  route emits a `registered` line immediately followed by an `unregistered` line. ⛔ Both are
	//  TRUE of states the program really passes through and the end state is correct.
	//  ⛔ I did NOT reorder `CloseHelp` to suppress them: swapping a shipped close route's two
	//  statements to make a log tidier trades a real risk for a cosmetic gain~~, and the transient
	//  focus placement is onto the same button that already holds focus on the ordinary close~~.
	//  ⭐ TASK-1480 (e) (2026-09-27, `qa/TASK-1479.md` WARN-1) — ⛔ BOTH STRUCK HALVES EXPIRED AT
	//  ⭐ `TASK-1478`, for the reason the twin at the top of this block already gives: stop 0 of the
	//  list is now the ⛔ FIRST `RowButton`, so the transient registration rings ⛔ THAT button,
	//  ⛔ not `CloseButton`, and it is ⛔ not in general the button that held focus before the close
	//  (a list-view close leaves the ring wherever the player put it; this route lands it on row 1).
	//  ⛔ THE DECISION NOT TO REORDER `CloseHelp` STANDS on the reason that survives: the pair is two
	//  true log lines, the end state (overlay closed, screen unregistered) is still correct, and a
	//  shipped close route is still not worth reordering for a tidier log.
	// ═══════════════════════════════════════════════════════════════════════════════════════════
	if (bWasOnDetail && bHelpOpen)
	{
		RegisterAsMenuNavTarget();
	}
}

void USiegeControlsHelpWidget::HandleDetailBackRequested()
{
	ReturnToList();
}

bool USiegeControlsHelpWidget::IsDetailViewActive() const
{
	// ⛔ ASKED OF THE SWITCHER, ⛔ never of a mirrored bool: one source of truth means the answer
	// and the screen cannot disagree. No switcher ⇒ there is no detail view at all ⇒ false.
	return ViewSwitcher != nullptr && ViewSwitcher->GetActiveWidgetIndex() == DetailViewIndex;
}

USiegeKeyboardLayoutSubsystem* USiegeControlsHelpWidget::ResolveKeyboardLayoutSubsystem() const
{
	// Resolved through the WORLD's game instance rather than a cached pointer: the overlay
	// outlives nothing and re-resolving costs one map lookup per open.
	const UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	UGameInstance* const GameInstance = World->GetGameInstance();
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<USiegeKeyboardLayoutSubsystem>();
}

TArray<FKey> USiegeControlsHelpWidget::QueryAppliedKeysForRow(const FSiegeControlsHelpAction& Row, const APlayerController* OwningController)
{
	TArray<FKey> AppliedKeys;

	// ⛔ LANE A ONLY. Every other lane's key is either an identity by construction (Lane B), a
	// `KBD-§8` literal (Lane C) or absent (Lane D) — asking Enhanced Input about them would be
	// meaningless, and answering would be how a raw key quietly acquires a translation.
	if (Row.Lane != ESiegeInputLane::MappedAction || OwningController == nullptr)
	{
		return AppliedKeys;
	}

	const UEnhancedInputLocalPlayerSubsystem* const EnhancedInput =
		ULocalPlayer::GetSubsystemFromController<UEnhancedInputLocalPlayerSubsystem>(OwningController);
	if (EnhancedInput == nullptr)
	{
		// No local player / no subsystem ⇒ empty, and ResolveRowDisplayKeys takes its single
		// translation fallback. Never a crash (`KBD-§5`).
		return AppliedKeys;
	}

	for (const TSoftObjectPtr<UInputAction>& SoftAction : Row.Actions)
	{
		// ⛔ SOFT, NULL-SAFE: a missing IA_* asset makes this row's chip read "(not bound)" and
		// is ⛔ never a crash. This is exactly what lets IA_ControlsHelp land late (TASK-705)
		// with the overlay already shipped and honest about it.
		const UInputAction* const Action = SoftAction.LoadSynchronous();
		if (Action == nullptr)
		{
			continue;
		}

		// ⛔⛔ THE ONE-TRANSLATION LINE. QueryKeysMappedToAction returns the keys mapped in the
		// ACTIVE contexts (EnhancedInputSubsystemInterface.h:381-384), and the active context
		// is the RETARGETED duplicate the layout subsystem rewrote in place
		// (SiegeKeyboardLayoutStatics.cpp:236; applied at HeroCharacter.cpp:271-275).
		// ⇒ THIS QUERY *IS* THE TRANSLATION. ⛔ No GetPositionalKey call follows it, here or
		// anywhere on this path — that second call is the double-translate defect
		// (SiegePlayerController.h:1222-1226), invisible on QWERTY and instant on Dvorak.
		for (const FKey& MappedKey : EnhancedInput->QueryKeysMappedToAction(Action))
		{
			// AddUnique: one action bound to a key twice (or two of a row's six card actions
			// resolving to the same key through a mis-authored IMC) must not double a chip.
			if (MappedKey.IsValid())
			{
				AppliedKeys.AddUnique(MappedKey);
			}
		}
	}

	return AppliedKeys;
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  ⭐ TASK-1432 [MENU-NAV-CONTROLS-HELP] — THE SCREEN ANSWERS THE KEYBOARD.
//
//  Four short functions and ⛔ NO NEW STATE. Everything that actually walks the tree, reads
//  `IsFocusable`, places the ring and logs the stop count lives in `USiegeMenuInputSubsystem`
//  (TASK-1406/1429, ⛔ READ-ONLY to this row and ⛔ not modified by it). This file's entire
//  contribution is saying ⛔ WHEN this screen is the one the player is looking at — plus the one
//  key handler that keeps a ⛔ shipped close route alive once the screen can hold focus.
//
//  ⛔ NOT HERE, ON PURPOSE (and this file's own written law in `OpenHelp` stands unchanged):
//  ⛔ no `SetInputMode`, ⛔ no `bShowMouseCursor`, ⛔ no `SetKeyboardFocus`, ⛔ no navigation
//  rule table, ⛔ no input binding, ⛔ no mapping-context mutation (`KBD-§1`/`KBD-§2`,
//  `HELP-§1`'s read-only clause, `HELP-§5`'s cursor-ownership clause).
// ═══════════════════════════════════════════════════════════════════════════════════════════

void USiegeControlsHelpWidget::RegisterAsMenuNavTarget()
{
	USiegeMenuInputSubsystem* MenuInput = ResolveMenuInputSubsystem();
	if (MenuInput == nullptr)
	{
		// `Log`, ⛔ not `Warning`: the honest reading of a null here is "this world has no menu
		// input" (an Editor/designer world, or a cooked path where the subsystem declined), and
		// an overlay that warns every time it is previewed is an overlay whose log nobody reads.
		// The screen still works with the mouse exactly as it did before this row — ⛔ keyboard
		// navigation is UNAVAILABLE, ⛔ not broken.
		UE_LOG(LogSiegeControlsHelp, Log,
			TEXT("[ControlsHelp] No USiegeMenuInputSubsystem on this world - the overlay is mouse-only (keyboard navigation is unavailable, not broken)."));
		return;
	}

	// `this`, ⛔ never a child and ⛔ never a class default — the API takes the SCREEN and walks
	// its own `WidgetTree` from there.
	//
	// 🚨 ⛔ AND `this` IS ⛔ NOT A STYLE CHOICE — IT IS THE ⛔ ONLY LEGAL ARGUMENT, MEASURED.
	// ⭐ `TASK-1478` wanted to pass `DetailView` while the detail page was up (the ring would then
	// follow the view with ⛔ no visibility writes at all, because the walker would see ⛔ only that
	// widget's own tree). ⛔ REFUTED AT ENGINE SOURCE: `GetRegisteredNavTarget()` accepts a stack
	// entry only when `Screen->IsInViewport() && Screen->IsVisible()`, and `UWidget::IsInViewport()`
	// is `bIsManagedByGameViewportSubsystem` + `UGameViewportSubsystem::IsWidgetAdded(this)`
	// (`UMG/Private/Components/Widget.cpp:344-350`) — a flag set ⛔ only by `AddToViewport` /
	// `AddToPlayerScreen`. ⛔ `DetailView` is `ConstructWidget`'d into a switcher slot and is ⛔ never
	// added to the viewport ⇒ it would be ⛔ pushed onto the stack and ⛔ skipped on every read, and
	// navigation would silently fall back to `FindMainMenuWidget()`. ⇒ ⛔ A RETARGET HERE WOULD
	// ⛔ COMPILE, ⛔ REVIEW CLEAN, ⛔ LOG A "registered" LINE AND ⛔ DO NOTHING (`SC-§36.1`).
	// ⛔ DO NOT PASS A CHILD WIDGET TO THIS API.
	//
	// ⚠️ ~~The stop set it will find is ⛔ ONE widget: the `UButton` named `CloseButton`.~~
	// ⭐ `TASK-1478` — ⛔ THE STOP SET IS NOW ⛔ PAGE-DEPENDENT, and it is stated as a ⛔ PROPERTY
	// rather than a number because `RefreshRows` rebuilds the list from the registry on every open:
	//   • ⛔ LIST view up  ⇒ `{ RowButton of every live USiegeControlsHelpRowWidget, in RowScrollBox
	//     slot order } ∪ { CloseButton }`, with `CloseButton` ⛔ LAST — i.e. ⛔ `RowWidgets.Num() + 1`.
	//   • ⛔ DETAIL view up ⇒ `{ DetailView->BackButton }` — ⛔ exactly ONE.
	//     ⛔ ~~⭐ `TASK-1484`: `{ DetailView->BackButton, DetailView->DetailScrollButton }` — exactly
	//     TWO~~ ⛔ STRUCK at ⭐ `TASK-1496` (🧑 his ruling 2026-09-26, on ⭐ `TASK-1494`'s pixels:
	//     ⛔ no detail page overflows ⇒ the scroll control had ⛔ no work to do).
	//     ⛔ ONE ⛔ still by the STRUCTURE of `ConstructDetailTree` (it builds ⛔ that one `UButton`
	//     and ⛔ no other admitted-class widget, ⛔ however many related blocks a page grows) —
	//     ⛔ state the ⛔ RULE, ⛔ not the integer: this count has now read ⛔ 1 → 2 → 1 and the
	//     ⛔ generating rule was ⛔ correct at every value.
	//     ⛔ `BackButton` is ⛔ STILL STOP 0 — it was under ⛔ two (the scroll control was
	//     `DetailColumn`'s ⛔ LAST child and the walk is ⛔ PRE-ORDER) and it is under ⛔ one (it is
	//     the ⛔ only member) — so ⛔ `FocusFirstNavStop()` still lands where TASK-1478 said.
	//     ⛔ AND THE ⛔ STEPPER-PAIR HAZARD IS ⛔ GONE RATHER THAN ⛔ REFUSED: `FindStepperPair`'s
	//     count guard needs ⛔ EXACTLY TWO buttons in the immediate parent and `DetailColumn` now
	//     holds ⛔ ONE, so the name/glyph discriminators that used to be load-bearing are ⛔ no
	//     longer reachable. 🚨 ⛔ Restore a second `UButton` there and the guard is ⛔ live again.
	//   • ⛔ AND THE TWO ARE ⛔ DISJOINT AND ⛔ NEVER BOTH RETURNED: `ApplyActiveView` collapses the
	//     inactive branch, so `HasVisibleSlateAncestry` drops the other page's buttons. ⛔ That
	//     property is the ⛔ whole safety argument; see that function.
	// ⛔ Category headers, both `UScrollBox`es and every border / text block / box contribute
	// ⛔ nothing — a `UScrollBox` is not one of `IsNavFocusStop`'s four admitted classes.
	// ⛔ AND ⛔ NO STOP IS AN ANCESTOR OF ANOTHER (each `RowButton` contains only boxes and text;
	// `CloseButton` and `BackButton` contain one `UTextBlock` each), so `GetFocusedNavStop()`'s
	// ancestor-precedence hazard ⛔ cannot arise on this screen.
	MenuInput->RegisterMenuNavTarget(this);
}

void USiegeControlsHelpWidget::UnregisterAsMenuNavTarget()
{
	// ⛔ SILENT ON A NULL SUBSYSTEM, unlike Register. If there was nothing to register with there
	// is nothing to give back, and one of the two places this is reached is `NativeDestruct` —
	// where a second log line would say nothing a reader could act on.
	if (USiegeMenuInputSubsystem* MenuInput = ResolveMenuInputSubsystem())
	{
		MenuInput->UnregisterMenuNavTarget(this);
	}
}

USiegeMenuInputSubsystem* USiegeControlsHelpWidget::ResolveMenuInputSubsystem() const
{
	// ⚠️ THROUGH THE ⛔ WORLD, ⛔ NOT THE GAME INSTANCE — and that is the ⛔ ONE deliberate
	// difference from `ResolveKeyboardLayoutSubsystem` directly above, which really is a
	// `UGameInstanceSubsystem`. `USiegeMenuInputSubsystem` is a `UWorldSubsystem`
	// (`SiegeMenuInputSubsystem.h`, its class declaration) and declines Editor worlds outright,
	// which is why a null answer is ordinary rather than an error. ⛔ Copying the game-instance
	// shape here would return null on every world and the screen would be silently mouse-only
	// forever — a defect with no symptom except 🧑 his hands.
	const UWorld* const World = GetWorld();
	return World ? World->GetSubsystem<USiegeMenuInputSubsystem>() : nullptr;
}

bool USiegeControlsHelpWidget::IsOwnToggleKey(const FKey& InKey) const
{
	if (!InKey.IsValid())
	{
		return false;
	}

	// ⛔ THE ROW IS LOOKED UP, ⛔ NOT THE KEY (`HELP-§4`: the menu documents its own key). This is
	// the ⛔ SAME id `RefreshRows` splices the hint line from and the ⛔ SAME id the R-24 chip is
	// derived for, so there is ⛔ one source of truth for "this overlay's key" in this file and
	// this function is ⛔ a second READER of it, ⛔ never a second copy.
	const FSiegeControlsHelpAction* const ToggleRow =
		FSiegeControlsHelpRegistry::FindAction(FName(TEXT("Interface.ControlsHelp")));
	if (ToggleRow == nullptr)
	{
		return false;
	}

	// ⛔⛔ APPLIED KEYS, ⛔ NOT DISPLAY KEYS — `HELP-§1`'s lane audit, and getting it backwards
	// here would be the double-translate defect wearing a behaviour bug's clothes.
	// `QueryAppliedKeysForRow` answers from `QueryKeysMappedToAction` over the ACTIVE context,
	// which is the layout subsystem's already-retargeted duplicate ⇒ it IS the key the player
	// physically presses, and `FKeyEvent::GetKey()` reports exactly that. `ResolveRowDisplayKeys`
	// is the LABEL lane: on its fallback it answers the QWERTY REFERENCE key, which on a moved
	// layout is a key the player is not pressing.
	const TArray<FKey> ToggleKeys = QueryAppliedKeysForRow(*ToggleRow, GetOwningPlayer());
	return ToggleKeys.Contains(InKey);
}

// ═══════════════════════════════════════════════════════════════════════════════════════════
//  🚨 TASK-1478 — ⛔ THIS HANDLER IS ⛔ UNCHANGED, AND THAT IS A ⛔ MEASURED CONCLUSION, ⛔ NOT AN
//  OMISSION. ⭐ `TASK-1478` took this screen from ⛔ ONE focusable button to ⛔ `RowWidgets.Num()`
//  + 2 of them, and ⛔ more focusable stops means ⛔ more widgets that could convert the toggle key
//  into Slate navigation instead of a close. ⛔ SO THE COVER WAS RE-DERIVED RATHER THAN ASSUMED:
//
//   ⛔ (1) `Tab` IS Slate's own focus-next key, so this is the ⛔ real hazard, not a theoretical one
//     — and it is exactly what the ⛔ struck `RowButton` comment was afraid of before this handler
//     existed.
//   ⛔ (2) `FSlateApplication::ProcessKeyDownEvent` routes ⛔ PREVIEW key down ⛔ DOWN the focus path
//     (the ⛔ tunnelling phase, root → focused widget) ⛔ BEFORE the bubbling `OnKeyDown` phase and
//     ⛔ BEFORE `AttemptNavigation`. A `Handled()` returned here short-circuits ⛔ all of it.
//   ⛔ (3) The focus path is the ⛔ ANCESTOR CHAIN of the focused widget, and ⛔ every button this
//     row made focusable — each `RowButton`, `BackButton` — is a ⛔ DESCENDANT of this overlay's
//     root. ⇒ ⛔ THIS WIDGET IS ON THE PATH ⛔ FOR EVERY ONE OF THEM, ⛔ BY CONSTRUCTION. ⛔ The
//     cover is a property of the tree's SHAPE, ⛔ not of which particular button is focusable.
//   ⛔ (4) ⛔ AND THE MOUSE CASE IS COVERED BY THE SAME SENTENCE: clicking a row now gives its
//     `SButton` keyboard focus (it did not before), and that focus is ⛔ still inside this overlay.
//   ⛔ (5) ⛔ AND THE "NOTHING IN THE OVERLAY IS FOCUSED" CASE IS ⛔ UNCHANGED FROM BEFORE `TASK-1432`:
//     this handler never runs, the key reaches Enhanced Input, and `IA_ControlsHelp` closes the
//     overlay from the controller. ⛔ Both pages, ⛔ both routes.
//  ⇒ ⛔ `IA_ControlsHelp` STILL CLOSES FROM THE ⛔ DETAIL PAGE (`TASK-1436` P6, ⛔ BLOCKER-class):
//  `CloseHelp()` → `ReturnToList()` → `ApplyOpenState(false)`, and ⛔ `ApplyActiveView` cannot
//  interfere — it writes ⛔ visibility and an ⛔ index, ⛔ never input and ⛔ never focus.
// ═══════════════════════════════════════════════════════════════════════════════════════════
FReply USiegeControlsHelpWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// ⛔⛔ `Escape` FIRST, UNCONDITIONALLY, AND ⛔ BEFORE ANY OTHER TEST RUNS. `HELP-§5` is a
	// ⛔ CLOSED 🧑 Jonathan ruling and it names ⛔ THIS FUNCTION by name: the overlay may not
	// absorb `Escape` "⛔ not via NativeOnKeyDown, ⛔ not NativeOnPreviewKeyDown, ⛔ not an
	// Enhanced Input action, ⛔ not a Slate FReply::Handled() on EKeys::Escape". ⛔ Returning
	// `Handled` for `Escape` "harmlessly" is overturning that ruling and is an AUTOMATIC QA FAIL.
	//
	// ⛔ IT IS ITS OWN STATEMENT RATHER THAN A CLAUSE OF THE MATCH BELOW, AND THAT IS THE POINT:
	// the toggle key is ⛔ DERIVED, so if `IA_ControlsHelp` were ever remapped onto `Escape` the
	// derived match beneath would claim it and nothing else in this file would notice. ⛔ This
	// line is what makes the ruling hold under a remap nobody here can see.
	// ⭐ `SiegeControlsHelpTest.cpp` already pins that no registry row names `Escape`; this guard
	// is the runtime half of the same promise, and it does ⛔ not depend on that test staying
	// green.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	// ⛔ A REPEAT IS NOT A FRESH PRESS. The Enhanced Input lane this mirrors binds
	// `ETriggerEvent::Started` (`SiegePlayerController.cpp`, the `IA_ControlsHelp` bind), so a
	// held key toggles ONCE there and must toggle once here. In practice the overlay collapses
	// out of the focus path on the first press, so this is belt AND braces — worth having,
	// because it is the difference between mirroring the shipped contract and merely coinciding
	// with it today.
	if (!bHelpOpen || InKeyEvent.IsRepeat())
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	if (!IsOwnToggleKey(InKeyEvent.GetKey()))
	{
		// ⛔ EVERY OTHER KEY IS HANDED STRAIGHT ON, INCLUDING THE ONES THIS SCREEN DEPENDS ON:
		// `Enter` / `SpaceBar` must reach the focused `CloseButton`, where `SButton::OnKeyDown`
		// turns Slate's own Accept action into `ExecuteOnClick()` → `OnClicked` →
		// `HandleCloseButtonClicked` → `CloseHelp()` (`SButton.cpp:293-316`;
		// `NavigationConfig.cpp:32-34` for which keys Accept is). ⛔ THAT is this screen's
		// keyboard Accept — ⛔ NOT `IA_MenuAccept`, which is never built in a match because
		// TASK-1429 shipped the in-match menu context BELOW the hero and in-match `Enter` stays
		// with `IA_AssistantConsole`.
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	// ⭐ THE SHIPPED CLOSE ROUTE, PRESERVED. Identical in effect to the Close button:
	// `CloseHelp()` → `ReturnToList()` → `ApplyOpenState(false)` → unregister →
	// `OnHelpOpenChanged.Broadcast(false)` → `HandleControlsHelpOpenChanged` →
	// `SetControlsHelpOpen(false)` → `ApplyCursorInputState()`. ⛔ The controller's posture flag
	// cannot be left stuck by this path any more than by the button's.
	CloseHelp();
	return FReply::Handled();
}
