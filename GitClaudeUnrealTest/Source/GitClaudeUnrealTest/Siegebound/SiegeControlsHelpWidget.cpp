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
#include "Siegebound/SiegeKeyboardLayoutSubsystem.h"

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

	/** The heading over a detail page's related-controls blocks — Jonathan's "all the controls with it". */
	static const TCHAR* RelatedHeader = TEXT("All the controls that go with it");

	/** ⭐ ONE definition of the `{ActionId}` token's shape, read by both the writer and the resolver. */
	static const TCHAR* ActionTokenOpen  = TEXT("{");
	static const TCHAR* ActionTokenClose = TEXT("}");

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
//  ⛔ THE ORIGINAL ROW SET AND THE LANE COLUMN ARE 704's. Every lane assignment below
//     traces to that file's §1.1 audit, and every one-liner is its §4 text VERBATIM.
//  ⛔ THE QWERTY COLUMN IS A FALLBACK AND A TEST FIXTURE, ⛔ NEVER THE DISPLAYED TRUTH while
//     the action resolves in an active context (§1.2 / ResolveRowDisplayKeys).
//  ⛔ NO NUMBER FROM THE SHIPPED MECHANICS IS RESTATED IN ANY ONE-LINER (`HELP-§2`, the M7.7
//     "in 400" / AoERadius 700 lesson).
//
//  ════════ TASK-707: THE `Detail` COLUMN, AND THE FIVE RULES ITS TRANSFER FOLLOWED ════════
//  Every detail string below is handoffs/TASK-704-programmer.md §4's prose for that row. 704
//  cited EVERY factual sentence at a file:line it had personally read; ⛔ nothing here was
//  re-authored, re-derived or invented, and where I disagreed with a sentence I FLAGGED it in
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
//     numbers (its U-5 / D-6, the M7.7 lesson) and the names are carried through unchanged.
//     The ONE number stated anywhere below is the war map's 30 gold, because Jonathan's own
//     words are the source and 704 quoted them at the property (CommanderNpc.h:297-311).
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
				TEXT("Four separate bindings on one action — forward, back, strafe left, strafe right. ")
				TEXT("The back and left rows carry Negate modifiers and the forward/back rows carry SwizzleAxis, ")
				TEXT("which is why these four rows must never be rewritten as a block. ")
				TEXT("Base walk speed is AHeroCharacter::WalkSpeed; the speed actually applied composes the Swift Boots ")
				TEXT("upgrade on top and is GetEffectiveWalkSpeed() — the base is never mutated.\n\n")
				TEXT("On a non-QWERTY layout these four keep their physical positions: the layout subsystem retargets ")
				TEXT("the mapping context's keys, not your muscle memory.")));
		}

		{
			// R-02. Mouse2D — a non-letter, so the fallback is an identity even if it is taken.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Hero.Look"), CategoryHero, TEXT("Look"),
				TEXT("Move the mouse to swing the camera."), ESiegeInputLane::MappedAction);
			Row.Actions = { MakeActionRef(TEXT("IA_Look")) };
			Row.QwertyReferenceKeys = { EKeys::Mouse2D };
			// 704 §4 R-02 detail. Citations (T1): the Negate_2 binding = handoffs/TASK-568-artist.md:28
			// and handoffs/TASK-399-artist.md:137 (X✗ Y✓ Z✗); the ignore-look pairing =
			// SiegePlayerController.cpp:768-770 and :783-790; the GameAndUI/DoNotLock modes =
			// SiegePlayerController.cpp:4361-4370.
			Row.Detail = FText::FromString(FString(
				TEXT("Bound as a 2D mouse axis with a Negate_2 modifier on the Y channel. ")
				TEXT("Look is suspended while you hold the interface-cursor key — OnUICursorPressed calls ")
				TEXT("SetIgnoreLookInput(true), paired 1:1 with its release, so a click-drag on the HUD cannot ")
				TEXT("nudge the camera.\n\n")
				TEXT("It is not suspended in placement, targeting or a group pick: those modes use ")
				TEXT("FInputModeGameAndUI with DoNotLock, so the mouse steers the cursor while movement keys ")
				TEXT("keep working.")));
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
				TEXT("Inherited from the character template and bound on both press and release.\n\n")
				TEXT("Falling out of the world is a death, not a despawn — AHeroCharacter::FellOutOfWorld ")
				TEXT("deliberately does not call Super (which would Destroy() the pawn) and routes into the same ")
				TEXT("path as lethal damage, so the standard respawn brings you back at your castle.")));
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
				TEXT("Bound on Started, Completed and Canceled, so the sprint can never stick on if the press is ")
				TEXT("interrupted. Pressing raises the max walk speed to SprintSpeed and releasing returns it to ")
				TEXT("WalkSpeed; both compose the Swift Boots move-speed bonus live via GetEffectiveSprintSpeed() ")
				TEXT("and GetEffectiveWalkSpeed() rather than mutating the base.\n\n")
				TEXT("A dead hero cannot start a sprint — StartSprint early-outs on bDead — but StopSprint is ")
				TEXT("unguarded so the state always releases.\n\n")
				TEXT("Sprinting and attacking are independent: sprint is a hold on one action, melee is a press on ")
				TEXT("another, and nothing in DoMeleeAttack reads the sprint flag.")));
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
			// ⛔ NO NUMBER RESTATED: the three tunables stay NAMED (704 U-5, the M7.7 lesson).
			Row.Detail = FText::FromString(FString(
				TEXT("One swing damages all enemy team agents within MeleeRange and inside a ")
				TEXT("±MeleeHalfAngleDegrees forward cone, rate-limited to one swing per MeleeCooldown seconds. ")
				TEXT("Damage per swing is composed live — base plus the Sharpened Blade stacks — through ")
				TEXT("GetEffectiveMeleeDamage(). No friendly fire.\n\n")
				TEXT("The same physical click confirms placement, spell targeting and every group-order stage — ")
				TEXT("and when it does, melee is suppressed so the click does one thing only: ")
				TEXT("SetMeleeSuppressed(true) makes DoMeleeAttack a no-op that does not even consume the cooldown.")));
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
				TEXT("Buffs every same-team ASummonedUnit within RallyRadius by RallySpeedBonus for ")
				TEXT("RallyDuration seconds — units only, never the hero, never enemy units. Friendly miners are ")
				TEXT("included, since AMinerUnit is a summoned-unit subclass.\n\n")
				TEXT("On cooldown the press is a no-op but still broadcasts OnRallyStateChanged(false, remaining) ")
				TEXT("so the HUD can flash the time left; the cooldown length is RallyCooldown and OnRallyReady ")
				TEXT("re-broadcasts (true, 0) when it elapses. A dead hero cannot rally.")));
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
				TEXT("Six keys, six hand slots, bound with the slot index as the payload. What happens next depends ")
				TEXT("on the card's type, read from the data table and never from code:\n\n")
				TEXT("• Unit / Building / Economy → placement mode. A ghost follows the cursor each frame and the ")
				TEXT("card leaves your hand only at confirm, so backing out costs nothing. Left-click confirms, ")
				TEXT("right-click or Escape cancels.\n\n")
				TEXT("• Spell → targeting mode, placement's sibling on the same input surface; same \"card leaves ")
				TEXT("the hand only at LMB confirm\" law. The one exception is Gold Steal, which resolves instantly ")
				TEXT("with no reticle because it is a global effect.\n\n")
				TEXT("Gold is checked before the type is considered — the affordability refusal outranks the type ")
				TEXT("refusal. A press is quietly ignored — not refused — after match end, mid-placement or ")
				TEXT("mid-targeting. Key 1 has one legacy quirk: with hand slot 0 empty it falls back to the ")
				TEXT("always-available Footman placement.")));
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
			// release = :783-790; the compose-not-fight ruling = :775-780, the one boolean expression
			// at :4357.
			Row.Detail = FText::FromString(FString(
				TEXT("A hold, not a toggle. Holding puts the game in GameAndUI with the cursor visible and camera ")
				TEXT("look suspended, so a click-drag on the HUD cannot nudge the camera.\n\n")
				TEXT("Bound on Completed and Canceled so the hold can never stick regardless of the action's ")
				TEXT("trigger setup, and the release is guarded so a double release cannot unbalance the ")
				TEXT("ignore-look counter.\n\n")
				TEXT("Releasing while placement mode is live leaves the cursor to placement mode — the two owners ")
				TEXT("compose rather than fight.")));
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
			// ⛔⛔ THE FEE IS NAMED AND NEVER TYPED. DiscardAllCost's own header comment pins this
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
				TEXT("The fee is DiscardAllCost and it is charged once for the whole hand, flat. Dumping a ")
				TEXT("single dead card costs exactly what dumping a full hand costs, because this prices a hand ")
				TEXT("RESET rather than a per-card cycle — there is no longer any way to bin one card on its own ")
				TEXT("at any price.\n\n")
				TEXT("If you cannot afford it, nothing happens at all: no gold leaves you, no card moves, and you ")
				TEXT("get the same \"Not enough gold\" line every other card refusal uses. An empty hand is refused ")
				TEXT("before the fee is taken, so you can never pay to discard nothing.\n\n")
				TEXT("It is refused while you are placing a card or targeting a spell — binning the card you are ")
				TEXT("half-way through committing would hand the confirm a different one. Back out first with ")
				TEXT("{Cards.Cancel}, then discard.")));
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
				TEXT("One action, two keys, and it is the same gesture everywhere. It exits placement mode, exits ")
				TEXT("spell targeting, or aborts a group-order pick at any stage, leaving every existing group and ")
				TEXT("stance untouched.\n\n")
				TEXT("The same two keys are ALSO polled directly every frame, so cancelling still works even if the ")
				TEXT("input asset is missing — the deliberate double-cover, and re-firing is harmless because the ")
				TEXT("exits are idempotent.\n\n")
				TEXT("Nothing has been spent at the moment you cancel: cards leave the hand only at confirm. ")
				TEXT("Escape is a shipped, bound cancel key.")));
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
			//     the SAME CardID -> CanScaleFootprint() -> gold -> Ready;
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
			//   • the castle case = gate (2)'s `Cast<ABuilding>` (ACastle is class-disjoint);
			//   • the HEIGHT series = ABuilding::StackHeightMultiplier, whose whole answer is
			//     `FMath::Min(1 + Upgrades, Cap)` with Cap read off the CDO's
			//     MaxStackHeightMultiplier ⇒ ⭐ ADDITIVE and SATURATING, ⛔ never doubling;
			//   • the HEALTH series = ABuilding::StackHealthMultiplier, repeated multiplication by
			//     the CDO's StackHealthStep with ⛔ NO ceiling term in the loop;
			//   • Z only, X/Y inherited = ApplyStackUpgrade's `Scale.Z = AuthoredHeightScaleZ *
			//     StackHeightMultiplier(StackUpgradeCount);` with X and Y untouched (`J-4`);
			//   • GRANTED, ⛔ never healed = the same function's `MaxHP = OldMaxHP *
			//     StackHealthMultiplier(1);` followed by `CurrentHP += (MaxHP - OldMaxHP);`;
			//   • the cap notice = ConfirmStackUpgrade's `if (ABuilding::StackHeightMultiplier(
			//     UpgradesAfter) <= ABuilding::StackHeightMultiplier(UpgradesBefore))` ->
			//     BroadcastRefusal(StackHeightCapNoticeText()) — at CONFIRM, ⛔ never per frame;
			//   • the COST = PendingCost, taken from the card's own data-table row at
			//     EnterPlacementMode (`PendingCost = Row->Cost;`) and spent by ConfirmStackUpgrade's
			//     `SiegeState.SpendGold(PendingCost)` (`J-2`: ⛔ never a literal);
			//   • the two refusals = TryConfirmPlacement's `case EPlacementInvalidReason::Upgrade:`,
			//     choosing between the shipped "Not enough gold" line and
			//     StackNotStackableRefusalText() ("That building cannot be stacked");
			//   • the card leaves the hand at confirm = ConfirmStackUpgrade's
			//     `DeckComponent->ConfirmPlayFromHand(PendingHandSlot)` block.
			//
			// ⛔⛔ THE EXCLUSION IS TAUGHT AS A BEHAVIOUR, ⛔ NEVER AS A NAME LIST — and that is the
			// same law the CODE obeys: the shipped path asks ABuilding::CanScaleFootprint()
			// (AClimbableTower overrides it false) and a CardID string compare there is an
			// automatic QA fail. ⇒ naming a card in this prose would teach a rule the game does not
			// have and would go stale the day a second climbable building ships. ⭐ The PLAYER'S
			// reason is given ("it is the one you climb"); ⛔ no socket, ⛔ no rung plane and ⛔ no
			// standoff appears anywhere on screen.
			//
			// ⛔ NO TUNABLE'S VALUE IS TYPED — MaxStackHeightMultiplier and StackHealthStep are
			// NAMED, exactly as PickMode.Resize names its three radii (the M7.7 "in 400" lesson).
			// ⚠️ The jargon cost F-3 already flags for that row applies here too and is declared in
			// this task's handoff rather than solved by inventing a number.
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
				TEXT("height, and it stops at MaxStackHeightMultiplier times that original. Its width and length are ")
				TEXT("not touched. Health: each upgrade multiplies the building's maximum health by StackHealthStep, ")
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
				TEXT("ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. Its ladder is ")
				TEXT("fixed to the shape of the mesh, so stretching the building would take the ladder with it and ")
				TEXT("the climb would stop working. The game therefore asks each building whether it may be scaled at ")
				TEXT("all, rather than checking it against a list of names — so any climbable building added later is ")
				TEXT("protected by the same one rule. Hovering one shows RED with \"That building cannot be ")
				TEXT("stacked\".\n\n")
				TEXT("A refusal of either kind costs nothing and leaves you in placement mode, so another building — ")
				TEXT("or another patch of ground — still works. Back out entirely with {Cards.Cancel}.\n\n")
				TEXT("The size you dial in with the wheel applies to what you PLACE, not to what you GROW: an ")
				TEXT("upgrade keeps the building's existing width and length.")));
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
			//     return; }` and its comment ("the refusal already has a voice ... on the click").
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
				TEXT("One notch changes the size by PlacementFootprintWheelStep, between PlacementFootprintMin and ")
				TEXT("PlacementFootprintMax. It will not go below the floor: you can make a building bigger than it ")
				TEXT("was drawn, never smaller. That is a decision rather than an oversight — shrinking would let a ")
				TEXT("building hide in a gap its shape was never meant to fit, and it would shrink the ground the ")
				TEXT("building blocks along with the art.\n\n")
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
				TEXT("ladder is fixed to the shape of the mesh, so it cannot be resized, and the wheel is simply dead ")
				TEXT("on it rather than nagging you once per notch — that refusal speaks once, at the click.")));
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
				TEXT("Immediate, army-wide, and it releases every standing group order. The sequence is one ")
				TEXT("function, shared with the assistant's charge: abort any in-flight pick → clear all unit ")
				TEXT("groups → latch the stance.\n\n")
				TEXT("The release runs BEFORE the latch and that ordering is load-bearing — units re-read the ")
				TEXT("stance on their next state tick, so releasing after latching would let a group about to be ")
				TEXT("destroyed re-assert its station for one tick.\n\n")
				TEXT("Under Attack, units march the enemy castle, clearing defenders inside the enemy spawn box ")
				TEXT("first; local self-defence aggro is unchanged. Ignored after match end.\n\n")
				TEXT("The stance is latched — it persists until replaced — and bHasIssuedCommand flips true on ")
				TEXT("your first command and stays true for the match, so the pre-command legacy behaviour never ")
				TEXT("returns mid-match.")));
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
				TEXT("The exact mirror of Attack — same guard, same two calls, same order, same final latch, ")
				TEXT("through the same one implementation. Units fall back toward your own castle and engage only ")
				TEXT("enemies inside the defend band.\n\n")
				TEXT("What \"the band\" means CHANGED: DefendRadius is no longer a disc centred on the castle — it ")
				TEXT("is the band past the castle's wall face, and the acquisition radius is derived at every ")
				TEXT("decision from the castle's live colliding half-width plus that band, by ")
				TEXT("ASummonedUnit::ResolveDefendEngagementRadius.")));
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
				TEXT("ladder every state tick: enemies in the attack zone first, else enemies in the position zone, ")
				TEXT("else walk to the unit's own station inside the position zone and wait.\n\n")
				TEXT("The leash is what makes this HOLD and not AMBUSH. A target that is alive but has left both ")
				TEXT("zones is dropped that tick — the unit disengages and returns toward its station. A dead ")
				TEXT("target is dropped for both types.\n\n")
				TEXT("There is deliberately no separate leash range: the zones are the leash. Targets are sticky — ")
				TEXT("a live, zone-valid target is kept and re-acquisition runs only when target-less, which is ")
				TEXT("what stops the goal flipping every tick. One monotone upgrade exists, HOLD only: a ")
				TEXT("position-tier target yields to an attack-zone enemy the moment one appears, and never the ")
				TEXT("other way, so the two tiers cannot oscillate. Stations are spread by a sunflower offset so ")
				TEXT("the squad does not mill at one point.\n\n")
				// ⭐⭐ T4 — 704 typed the letters `T` and `E` here. US-Dvorak moves them to `Y` and `.`,
				// so a typed letter would be the exact `HELP-§1` defect. The tokens resolve to the two
				// orders' OWN derived chips at compose time (ResolveDetailTokens).
				TEXT("Pressing {Orders.Attack} or {Orders.Defend} destroys this group.")));
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
				TEXT("The difference: AMBUSH skips the zone drop-test entirely while a live target exists. It keeps ")
				TEXT("the target until the kill, then the ladder resumes. Ambush acquires through exactly the same ")
				TEXT("two tiers; the exemption governs only when an already-held target is released.\n\n")
				TEXT("A dead target is still dropped, for both types. The single monotone position→attack upgrade ")
				TEXT("is HOLD-only and does not run for Ambush.\n\n")
				TEXT("For miners the two orders collapse into one behaviour: \"Ambush\" is the same thing as ")
				TEXT("\"hold\" for a miner.\n\n")
				// ⭐⭐ T4 — see R-13. Same two orders, same reason.
				TEXT("Pressing {Orders.Attack} or {Orders.Defend} destroys this group.")));
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
				TEXT("the pick enters at Select and confirms there, and the later stages are structurally ")
				TEXT("unreachable. Jonathan's reason: \"There is only one mouse scroll circle used for this, and it ")
				TEXT("is just the circle used to indicate what units follow\". Its stage-1 prompt is worded ")
				TEXT("separately so it never promises a second stage.\n\n")
				TEXT("The anchor is you, not a piece of ground: a follow group carries zero radii, zero centres ")
				TEXT("and no marker decals, and the select circle is destroyed at confirm rather than left on the ")
				TEXT("map. The station is your live position plus that unit's own sunflower offset, resolved every ")
				TEXT("tick and never cached — which is exactly what makes hero respawn work for free.\n\n")
				TEXT("Followers never attack. The target is forced null every tick and the follow body calls none ")
				TEXT("of the acquire/attack functions. A following Cleric still heals — healing is not attacking, ")
				TEXT("and an escorting medic is the point of a support unit told to follow.\n\n")
				TEXT("If you die, followers hold position — no target, no march, no attack — and resume the instant ")
				TEXT("a live pawn exists again, including a brand-new one after respawn.\n\n")
				// ⭐⭐ T4 — 704 typed "`T`/`E`" here too.
				TEXT("Unlike {Orders.Attack} and {Orders.Defend}, Follow does not release your other groups: it ")
				TEXT("adds the circled units to the one follow group, stealing them out of any Hold/Ambush group, ")
				TEXT("and units you did not circle keep their orders.\n\n")
				TEXT("THE SPAWN DEFAULT: every follow-eligible Blue unit spawns already following you, on every ")
				TEXT("spawn path, and nothing player-side auto-engages any more — you personally order every ")
				TEXT("fight. Siege units (Ogre/Sapper) and the whole enemy side are unaffected, because the ")
				TEXT("eligibility predicate excludes them. Enrolment is unconditional: a unit spawned after you ")
				TEXT("pressed {Orders.Attack} still spawns following — reinforcements do not inherit your last ")
				TEXT("order. Miners are the one exception and spawn mining.")));
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
			// ⛔ NO RADIUS VALUE RESTATED: the three defaults stay NAMED (704 U-5, the M7.7 lesson).
			Row.Detail = FText::FromString(FString(
				TEXT("Polled directly every frame while a pick is live, not bound to an input action. Your hero ")
				TEXT("does not swing on that click — melee is suppressed for the whole pick.\n\n")
				TEXT("THE THREE CIRCLES, IN ORDER — what each one actually does:\n\n")
				TEXT("1. SELECT — opens at GroupSelectRadiusDefault. At confirm, every eligible unit inside it ")
				TEXT("(2D) joins the group. An empty circle is refused and you STAY in the stage — a different ")
				TEXT("circle can still succeed — with \"No units in the circle\" on the HUD. The eligibility test ")
				TEXT("differs by order: Hold and Ambush use the narrower zone-order predicate, while Follow is ")
				TEXT("wider — it also admits the Support Cleric and the Miner — and both exclude Siege units ")
				TEXT("(Ogre/Sapper) and the entire enemy side. This circle is a transient pick visual: it is ")
				TEXT("destroyed at the final confirm and never becomes a marker. For FOLLOW the flow ENDS HERE.\n\n")
				TEXT("2. POSITION — opens at GroupPositionRadiusDefault. This is the ground the squad stands on: ")
				TEXT("the station zone it spreads inside, and the second-priority engage disc. At the final ")
				TEXT("confirm this circle stays on the map as the group's permanent position marker.\n\n")
				TEXT("3. ATTACK — opens at GroupAttackRadiusDefault. This is the first-priority engage trigger: an ")
				TEXT("enemy entering it is what the squad goes for first. It also stays as a permanent marker.\n\n")
				TEXT("Both surviving markers die with the group.\n\n")
				TEXT("The circle you are drawing traces to the surface under the cursor — flat floor, hill crown ")
				TEXT("or flank alike, never a flat plane — and it hides while the cursor is on the sky; the ")
				TEXT("confirm refuses on the same flag, so what you see is what the click does.\n\n")
				TEXT("The circles are colour-coded by stage: Select white, Position green, Attack red.\n\n")
				TEXT("The completion line reports the count that joined, not the count you circled, because ")
				TEXT("members that died mid-flow are dropped.")));
			// The other two halves of the same gesture, so this page is complete on its own.
			Row.RelatedActionIds = { FName(TEXT("PickMode.Resize")), FName(TEXT("PickMode.Cancel")) };
		}

		{
			// R-17. Lane B. ⛔ The wheel is polled, not bound, and it is inert everywhere except
			// inside a pick (SiegePlayerController.cpp:2787-2791, call site :599).
			FSiegeControlsHelpAction& Row = AddRow(TEXT("PickMode.Resize"), CategoryPickMode, TEXT("Resize the circle"),
				TEXT("Scroll the mouse wheel to grow or shrink the circle you are currently drawing."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = { EKeys::MouseScrollUp, EKeys::MouseScrollDown };
			// ⭐⭐ 704 §4 R-17 detail — JONATHAN'S SECOND NAMED QUESTION: "how to resize them".
			// Citations (T1): the wheel handler = SiegePlayerController.cpp:2785-2806, with
			// GroupRadiusWheelStep / GroupRadiusMin / GroupRadiusMax at SiegePlayerController.h:1265,
			// :1269, :1273; each stage opening at its own default = SiegePlayerController.h:1258-1262
			// and SiegePlayerController.cpp:2710, :2909, :2929; the in-place decal resize = :2808-2817;
			// polled-not-bound and inert outside a pick = :2787-2791, call site :599; the
			// no-material degradation = :2808-2810.
			// ⛔ ALL THREE TUNABLES STAY NAMED, ⛔ never re-typed as numbers. This is 704's U-5 / D-6
			// applied verbatim and it is the M7.7 lesson: the shipped Notes column once said "in 400"
			// while the real radius was 700. ⚠️ It reads as jargon on screen and I have FLAGGED that
			// for Jonathan (F-3 in handoffs/TASK-707-programmer.md) rather than invent a number.
			Row.Detail = FText::FromString(FString(
				TEXT("One notch changes the active circle's radius by GroupRadiusWheelStep, clamped between ")
				TEXT("GroupRadiusMin and GroupRadiusMax. Each stage opens at its own default and resizing one ")
				TEXT("circle never touches an earlier one. The decal resizes in place as you scroll.\n\n")
				TEXT("The wheel is polled, not bound, and it is verified globally unbound elsewhere — it is inert ")
				TEXT("everywhere except inside a pick.\n\n")
				TEXT("If the circle material is missing the radius still changes and the confirm still uses it — ")
				TEXT("you just get no visual.")));
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
				TEXT("Polled every frame at the top of the pick branch, before anything else runs, and also ")
				TEXT("reachable through the bound cancel action — the deliberate double-cover. Cancelling leaves ")
				TEXT("every existing group and stance unchanged.\n\n")
				TEXT("The teardown is one function and every exit funnels through it — the final confirm, either ")
				TEXT("cancel route, {Orders.Attack} or {Orders.Defend}, match end, hero death, unpossess and match ")
				TEXT("reset. It releases the melee suppression before any early-out, clears the stage and all pick ")
				TEXT("scratch, destroys every circle the flow still owns — a cancel at any stage kills all live ")
				TEXT("circles, while a completed flow only loses its Select circle because the other two were ")
				TEXT("handed to the group — clears the HUD prompt so the stance display returns, and restores ")
				TEXT("free-look unless the interface-cursor key is still held, because the cursor owners compose.\n\n")
				TEXT("Re-pressing the same order key mid-flow does nothing — it is a silent ignore; right-click or ")
				TEXT("Escape is the cancel surface.")));
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
			// SiegeAssistantConsoleWidget.h:138-143 and SiegePlayerController.cpp:4357-4375; a faulted
			// assistant disables only this box = SiegeAssistantConsoleWidget.h:144-146.
			// ⚠️ 704 U-6 stands: whether a focused text box swallows `Enter` before Enhanced Input
			// sees it is UNMEASURED since TASK-445 — and this text deliberately claims NEITHER
			// behaviour, so it stays true whichever way that lands.
			// ⚠️ `Enter` and `Escape` stay LITERAL: neither is a letter, so neither can be retargeted.
			Row.Detail = FText::FromString(FString(
				TEXT("A toggle, and it asks \"is it open?\" before \"may it open?\" — the close half is ")
				TEXT("deliberately un-gated, because a close that can be refused is a close that can strand your ")
				TEXT("cursor. Opening is gated: it is refused while placement, spell targeting or a group pick owns ")
				TEXT("the cursor, or after match end, and nothing is created or shown until the posture is granted ")
				TEXT("— on a refusal literally nothing happens on screen.\n\n")
				TEXT("It works anywhere. Unlike the war map there is no proximity check, no NPC reference and no ")
				TEXT("range condition — Jonathan's ruling: \"the console still works anywhere\".\n\n")
				TEXT("Sending: type and press Enter — only a genuine Enter commits; moving focus away or clearing ")
				TEXT("the box does not submit a half-typed sentence.\n\n")
				TEXT("CLOSING — the complete list: (1) press the open key again; (2) CancelPressed() — public API ")
				TEXT("with no caller today, kept deliberately; (3) the fault latch SetConsoleEnabled(false); ")
				TEXT("(4) Enter on an empty box — Jonathan's directive: \"if you press enter without anything ")
				TEXT("typed in the box then it will close\". Empty-Enter closes even with a confirm prompt up — ")
				TEXT("that is the point of the feature.\n\n")
				TEXT("Escape does NOT close the chat box, permanently. It is left unabsorbed so the shipped ")
				TEXT("placement, targeting and group-pick cancel routes keep firing byte-identically while the box ")
				TEXT("is open.\n\n")
				TEXT("A close is not a cancel: closing the window broadcasts no cancellation — only the ")
				TEXT("assistant's own state machine may turn one into the other.\n\n")
				TEXT("The console never sets the input mode itself; the controller owns that in one place. A ")
				TEXT("faulted assistant disables this box and nothing else — no key, no card, no command changes.")));
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
				TEXT("The key is caught in preview — it tunnels down the focus path from the root before the ")
				TEXT("focused text box, which is why a plain key handler could never see a printable key the box ")
				TEXT("already ate.\n\n")
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
				TEXT("undo and redo, and consuming them would both kill undo and execute an order you never asked ")
				TEXT("for. Shift+Z is accepted — it is still \"the Z key\" to a human.\n\n")
				TEXT("The grab is as narrow as it can be: it fires only while the box is open, enabled, and a ")
				TEXT("confirm prompt is actually up, so you can still type the letter z the rest of the time. ")
				TEXT("Accepted consequence: while a prompt is up you cannot type z into the box.\n\n")
				TEXT("On a non-QWERTY layout the game listens at the QWERTY-Z PHYSICAL POSITION — the comparison ")
				TEXT("resolves through the layout subsystem.")));
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
				TEXT("A toggle, and close is asked first and never gated, for the same reason as the chat box.\n\n")
				TEXT("THE PROXIMITY GATE — and it is proximity to YOUR OWN commander. The team is resolved from ")
				TEXT("your player state and never guessed: with no player state there is no honest answer and no ")
				TEXT("commander is returned, because a wrong default on the wrong side would gate the map on the ")
				TEXT("enemy's commander and price the reveal off the wrong actor. The distance test and its radius ")
				TEXT("both belong to the commander — ACommanderNpc::IsPlayerInRange reading InteractRadius — and ")
				TEXT("the controller re-implements neither.\n\n")
				TEXT("The gate is checked BEFORE the cursor posture is touched, deliberately, so an out-of-range ")
				TEXT("press cannot be felt as a one-frame flicker mid-fight. Out of range you get one HUD line ")
				TEXT("naming the reason — \"Walk up to your commander in the castle to use the war map\". This ")
				TEXT("gate is the map's and the map's only — it is never applied to the chat box.\n\n")
				TEXT("Nothing appears until the posture is granted, and if the widget then fails to create or ")
				TEXT("fails to report itself open, the posture is rolled back rather than left as a cursor owner ")
				TEXT("with no UI.\n\n")
				TEXT("Closing the map DISCARDS the paid reveal, unconditionally — \"red dots vanish the moment the ")
				TEXT("map closes, even one second after paying — that is the mechanic\". Escape closes it too, ")
				TEXT("polled every frame; that poll exists because a marker click opens the chat box, whose focused ")
				TEXT("text field would otherwise swallow the toggle key and type it into your sentence instead.")));
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
			// the live property or leaves the name (`HELP-§2`, the M7.7 lesson).
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
			// tunable is NAMED (704 U-5, the M7.7 "in 400"/AoERadius-700 lesson) — and note that the
			// sentence around the quote still names EnemyRevealCost, so the mechanism, not the
			// number, is what the page teaches.
			Row.Detail = FText::FromString(FString(
				TEXT("The price is EnemyRevealCost — Jonathan's own number: \"You can pay 30 gold to reveal all ")
				TEXT("enemy locations\". It is a mechanic rule, so it is a property default and never a card-table ")
				TEXT("column, and the commander class only holds the number: it never reads a balance and never ")
				TEXT("spends. The cost is read off your own team's commander at the moment of purchase and never ")
				TEXT("re-typed; with no commander the purchase fails closed rather than inventing a fallback ")
				TEXT("price.\n\n")
				TEXT("The refusal is net-zero: you are refused BEFORE any gold moves, both in the local check that ")
				TEXT("produces the HUD message and again on the authority. Past the spend there is deliberately no ")
				TEXT("early-out, so a partial spend is impossible — even an empty survey is a legitimate paid-for ")
				TEXT("answer.\n\n")
				TEXT("The spend is always initiated by your click, which is what keeps the standing ruling \"THE AI ")
				TEXT("NEVER SPENDS GOLD\" true.")));
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
				TEXT("Clicking a marker opens the chat box first, through the proper open path, then appends the ")
				TEXT("symbol. That order is pinned: the append never opens the box and never submits, and opening ")
				TEXT("clears the input field on every open — so appending first and opening second would silently ")
				TEXT("eat your click.\n\n")
				TEXT("The symbol is moved opaquely — the controller never spells it and must not learn which ")
				TEXT("symbols exist, because a validation branch there would be a second, drifting copy of a ")
				TEXT("vocabulary it does not own. The append's return value is checked; a refused insert is logged ")
				TEXT("and inserts nothing rather than mis-delivering.\n\n")
				TEXT("NOTHING IS SUBMITTED, AND THAT IS THE RULING: \"the map writes the symbol into the box and ")
				TEXT("THE PLAYER SENDS THE SENTENCE HIMSELF. An auto-submit would turn a click into an order\".")));
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
			// ⛔ NO NUMBER IS TYPED: MaxMapMarks is NAMED. ⛔ And ⛔ no coordinate, radius or count
			// is described — the airlock is a property of the feature, not something this page needs
			// to explain.
			FSiegeControlsHelpAction& Row = AddRow(TEXT("Interface.MapMarks"), CategoryInterface, TEXT("Draw circles on the map"),
				TEXT("On the war map: left-click empty ground to drop a numbered circle, scroll on one to resize it, right-click one to delete it, and click one to name it to your commander."), ESiegeInputLane::RawNonLetter);
			Row.QwertyReferenceKeys = {
				EKeys::LeftMouseButton, EKeys::RightMouseButton, EKeys::MouseScrollUp, EKeys::MouseScrollDown
			};
			Row.Detail = FText::FromString(FString(
				TEXT("Your own numbered circles, drawn on the war map. A circle is how you name a piece of ground to ")
				TEXT("your commander: it carries a number, that number is published to him as a place, and an order ")
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
				TEXT("You can hold MaxMapMarks circles at once. At the limit a further click refuses out loud and ")
				TEXT("tells you how many you are already holding, rather than doing nothing and looking broken.\n\n")
				TEXT("The circles are yours alone. They live on your own machine, the enemy never sees them, they ")
				TEXT("survive closing and re-opening the map, and they are cleared when the match resets. They are ")
				TEXT("never saved.")));
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
			// the cursor composition and its owners = SiegePlayerController.cpp:4326-4376, the one
			// expression at :4357; the widget-sets-its-own-posture defect = :222-239.
			// ⭐ T4 — the overlay's OWN key is a token, for the same reason the hint line splices its
			// chip rather than typing it (`HELP-§4`, TASK-706 D-9): the menu documents its own key,
			// and typing "Tab" here would be the one hardcoded key on a screen built to have none.
			// ⛔ T5 — 704's closing "Status: the action asset and its Tab mapping both landed during
			// this task" is a pipeline status line (its §2b), ⛔ not player prose.
			Row.Detail = FText::FromString(FString(
				TEXT("A toggle. It documents its own key: {Interface.ControlsHelp} is a positional key like any ")
				TEXT("other, so on a layout that moves it this row re-derives with everything else.\n\n")
				TEXT("The overlay does not pause the game — single-player has no pause today and the enemy keeps ")
				TEXT("marching; a pause would be a new mechanic, not a side effect of a help screen. It is ")
				TEXT("read-only on the world: opening it issues no order, cancels no group, plays no card and ")
				TEXT("moves no gold.\n\n")
				TEXT("It closes on {Interface.ControlsHelp} or its own Close button, and that is the complete ")
				TEXT("list — it never claims Escape. Every shipped cancel route keeps firing while it is open.\n\n")
				TEXT("Cursor posture is added to ApplyCursorInputState()'s one composition and nowhere else — the ")
				TEXT("existing owners keep their exact shipped precedence.")));
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
	return Row.Detail;
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

	// ⛔ NOT FOCUSABLE, AND THAT IS CORRECTNESS RATHER THAN POLISH: `Tab` is Slate's own
	// focus-next key. Clicking a row is the whole point of this screen, and a focusable
	// SButton takes keyboard focus on that click — after which `Tab` would navigate focus
	// instead of reaching Enhanced Input, and the overlay's own toggle key would stop closing
	// it. A non-focusable button is still fully mouse-clickable.
	ApplyButtonNotFocusable(RowButton);

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

			// ⛔ NOT FOCUSABLE — the same `Tab`-is-Slate's-own-focus-key reasoning as the list.
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
			// ⛔ NOT FOCUSABLE — see USiegeControlsHelpRowWidget's RowButton comment.
			ApplyButtonNotFocusable(BackButton);

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
}

void USiegeControlsDetailWidget::NativeDestruct()
{
	if (BackButton != nullptr)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USiegeControlsDetailWidget::HandleBackButtonClicked);
	}

	Super::NativeDestruct();
}

void USiegeControlsDetailWidget::HandleBackButtonClicked()
{
	RequestBack();
}

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

			// ⛔ NOT FOCUSABLE — the same `Tab`-is-Slate's-focus-key reasoning as RowButton.
			RowScrollBox->SetIsFocusable(false);

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
			// ⛔ NOT FOCUSABLE — see the RowButton comment.
			ApplyButtonNotFocusable(CloseButton);

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
		ViewSwitcher->SetActiveWidgetIndex(ListViewIndex);
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

	// STAMP BEFORE SWITCHING, so the page is never on screen for a frame carrying the previous
	// row's text.
	DetailView->SetDetailContent(Content);
	ViewSwitcher->SetActiveWidgetIndex(DetailViewIndex);

	UE_LOG(LogSiegeControlsHelp, Log,
		TEXT("[ControlsHelp] Detail page open for '%s' (%d related control(s))."),
		*InActionId.ToString(), Content.Related.Num());
}

void USiegeControlsHelpWidget::ReturnToList()
{
	// ⛔ IDEMPOTENT AND NULL-SAFE BY CONSTRUCTION, which is what lets CloseHelp() and OpenHelp()
	// both call it unconditionally: UWidgetSwitcher::SetActiveWidgetIndex early-outs when the
	// index is unchanged (WidgetSwitcher.cpp:49-57), so calling it on an already-listed overlay
	// costs nothing and broadcasts nothing.
	// ⛔ THIS IS NOT A CLOSE. The overlay's open state is untouched here — the two close routes
	// are still the toggle key and the Close button, and that is the complete list (`HELP-§5`).
	if (ViewSwitcher == nullptr)
	{
		return;
	}

	ViewSwitcher->SetActiveWidgetIndex(ListViewIndex);
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
